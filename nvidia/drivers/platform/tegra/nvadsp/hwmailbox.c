/*
 * Copyright (c) 2014-2022, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/atomic.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/irqdomain.h>
#include <linux/platform_device.h>
#include <linux/tegra_nvadsp.h>
#include <linux/irqchip/tegra-agic.h>

#include "dev.h"


static struct platform_device *nvadsp_pdev;
static struct nvadsp_drv_data *nvadsp_drv_data;
/* Initialized to false by default */
static bool is_hwmbox_busy;
#ifdef CONFIG_MBOX_ACK_HANDLER
static int hwmbox_last_msg;
#endif

/*
 * Mailbox 0 is for receiving messages
 * from ADSP i.e. CPU <-- ADSP.
 */
#define INT_RECV_HWMBOX	INT_AMISC_MBOX_FULL0

static inline u32 recv_hwmbox(void)
{
	return nvadsp_drv_data->chip_data->hwmb.hwmbox0_reg;
}

/*
 * Mailbox 1 is for sending messages
 * to ADSP i.e. CPU --> ADSP
 */
#define INT_SEND_HWMBOX	INT_AMISC_MBOX_EMPTY1

static inline u32 send_hwmbox(void)
{
	return nvadsp_drv_data->chip_data->hwmb.hwmbox1_reg;
}


u32 hwmb_reg_idx(void)
{
	return nvadsp_drv_data->chip_data->hwmb.reg_idx;
}

u32 hwmbox_readl(u32 reg)
{
	return readl(nvadsp_drv_data->base_regs[hwmb_reg_idx()] + reg);
}

void hwmbox_writel(u32 val, u32 reg)
{
	writel(val, nvadsp_drv_data->base_regs[hwmb_reg_idx()] + reg);
}


#define PRINT_HWMBOX(x) \
	dev_info(&nvadsp_pdev->dev, "%s: 0x%x\n", #x, hwmbox_readl(x))

void dump_mailbox_regs(void)
{
	dev_info(&nvadsp_pdev->dev, "dumping hwmailbox registers ...\n");
	PRINT_HWMBOX(recv_hwmbox());
	PRINT_HWMBOX(send_hwmbox());
}

static void hwmboxq_init(struct hwmbox_queue *queue)
{
	queue->head = 0;
	queue->tail = 0;
	queue->count = 0;
	init_completion(&queue->comp);
	spin_lock_init(&queue->lock);
}

/* Must be called with queue lock held in non-interrupt context */
static inline bool
is_hwmboxq_empty(struct hwmbox_queue *queue)
{
	return (queue->count == 0);
}

/* Must be called with queue lock held in non-interrupt context */
static inline bool
is_hwmboxq_full(struct hwmbox_queue *queue)
{
	return (queue->count == HWMBOX_QUEUE_SIZE);
}

/* Must be called with queue lock held in non-interrupt context */
static status_t hwmboxq_enqueue(struct hwmbox_queue *queue,
					    uint32_t data)
{
	int ret = 0;

	if (is_hwmboxq_full(queue)) {
		ret = -EBUSY;
		goto comp;
	}
	queue->array[queue->tail] = data;
	queue->tail = (queue->tail + 1) & HWMBOX_QUEUE_SIZE_MASK;
	queue->count++;

	if (is_hwmboxq_full(queue))
		goto comp;
	else
		goto out;

 comp:
	reinit_completion(&queue->comp);
 out:
	return ret;
}

status_t nvadsp_hwmbox_send_data(uint16_t mid, uint32_t data, uint32_t flags)
{
	spinlock_t *lock = &nvadsp_drv_data->hwmbox_send_queue.lock;
	unsigned long lockflags;
	int ret = 0;

	if (flags & NVADSP_MBOX_SMSG) {
		data = PREPARE_HWMBOX_SMSG(mid, data);
		pr_debug("nvadsp_mbox_send: data: 0x%x\n", data);
	}

	/* TODO handle LMSG */

	spin_lock_irqsave(lock, lockflags);

	if (!is_hwmbox_busy) {
		is_hwmbox_busy = true;
		pr_debug("nvadsp_mbox_send: empty mailbox. write to mailbox.\n");
#ifdef CONFIG_MBOX_ACK_HANDLER
		hwmbox_last_msg = data;
#endif
		hwmbox_writel(data, send_hwmbox());
	} else {
		pr_debug("nvadsp_mbox_send: enqueue data\n");
		ret = hwmboxq_enqueue(&nvadsp_drv_data->hwmbox_send_queue,
				      data);
	}
	spin_unlock_irqrestore(lock, lockflags);
	return ret;
}

/* Must be called with queue lock held in non-interrupt context */
static status_t hwmboxq_dequeue(struct hwmbox_queue *queue,
					    uint32_t *data)
{
	int ret = 0;

	if (is_hwmboxq_empty(queue)) {
		ret = -EBUSY;
		goto out;
	}

	if (is_hwmboxq_full(queue))
		complete_all(&nvadsp_drv_data->hwmbox_send_queue.comp);

	*data = queue->array[queue->head];
	queue->head = (queue->head + 1) & HWMBOX_QUEUE_SIZE_MASK;
	queue->count--;

 out:
	return ret;
}

static irqreturn_t hwmbox_send_empty_int_handler(int irq, void *devid)
{
	spinlock_t *lock = &nvadsp_drv_data->hwmbox_send_queue.lock;
	struct device *dev = &nvadsp_pdev->dev;
	unsigned long lockflags;
	uint32_t data;
	int ret;

	if (!is_hwmbox_busy)
		return IRQ_HANDLED;

	spin_lock_irqsave(lock, lockflags);

	data = hwmbox_readl(send_hwmbox());
	if (data != PREPARE_HWMBOX_EMPTY_MSG())
		dev_err(dev, "last mailbox sent failed with 0x%x\n", data);

#ifdef CONFIG_MBOX_ACK_HANDLER
	{
		uint16_t last_mboxid = HWMBOX_SMSG_MID(hwmbox_last_msg);
		struct nvadsp_mbox *mbox = nvadsp_drv_data->mboxes[last_mboxid];

		if (mbox) {
			nvadsp_mbox_handler_t ack_handler = mbox->ack_handler;

			if (ack_handler) {
				uint32_t msg = HWMBOX_SMSG_MSG(hwmbox_last_msg);

				ack_handler(msg, mbox->hdata);
			}
		}
	}
#endif
	ret = hwmboxq_dequeue(&nvadsp_drv_data->hwmbox_send_queue,
			      &data);
	if (ret == 0) {
#ifdef CONFIG_MBOX_ACK_HANDLER
		hwmbox_last_msg = data;
#endif
		hwmbox_writel(data, send_hwmbox());
		dev_dbg(dev, "Writing 0x%x to SEND_HWMBOX\n", data);
	} else {
		is_hwmbox_busy = false;
	}
	spin_unlock_irqrestore(lock, lockflags);

	return IRQ_HANDLED;
}

static irqreturn_t hwmbox_recv_full_int_handler(int irq, void *devid)
{
	uint32_t data;
	int ret;

	data = hwmbox_readl(recv_hwmbox());
	hwmbox_writel(PREPARE_HWMBOX_EMPTY_MSG(), recv_hwmbox());

	if (IS_HWMBOX_MSG_SMSG(data)) {
		uint16_t mboxid = HWMBOX_SMSG_MID(data);
		struct nvadsp_mbox *mbox = nvadsp_drv_data->mboxes[mboxid];

		if (!mbox) {
			dev_info(&nvadsp_pdev->dev,
				 "Failed to get mbox for mboxid: %u\n",
				 mboxid);
			goto out;
		}

		if (mbox->handler) {
			mbox->handler(HWMBOX_SMSG_MSG(data), mbox->hdata);
		} else {
			ret = nvadsp_mboxq_enqueue(&mbox->recv_queue,
						   HWMBOX_SMSG_MSG(data));
			if (ret) {
				dev_info(&nvadsp_pdev->dev,
					 "Failed to deliver msg 0x%x to"
					 " mbox id %u\n",
					 HWMBOX_SMSG_MSG(data), mboxid);
				goto out;
			}
		}
	} else if (IS_HWMBOX_MSG_LMSG(data)) {
		/* TODO */
	}
 out:
	return IRQ_HANDLED;
}

void nvadsp_free_hwmbox_interrupts(struct platform_device *pdev)
{

	struct nvadsp_drv_data *drv = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	int recv_virq, send_virq;

	recv_virq = drv->agic_irqs[MBOX_RECV_VIRQ];
	send_virq = drv->agic_irqs[MBOX_SEND_VIRQ];

	devm_free_irq(dev, recv_virq, pdev);
	devm_free_irq(dev, send_virq, pdev);
}

int nvadsp_setup_hwmbox_interrupts(struct platform_device *pdev)
{
	struct nvadsp_drv_data *drv = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	u32 empty_int_ie = drv->chip_data->hwmb.empty_int_ie;
	int recv_virq, send_virq;
	int ret;

	recv_virq = drv->agic_irqs[MBOX_RECV_VIRQ];
	send_virq = drv->agic_irqs[MBOX_SEND_VIRQ];

	ret = devm_request_irq(dev, recv_virq, hwmbox_recv_full_int_handler,
			  IRQF_TRIGGER_RISING, "hwmbox0_recv_full", pdev);
	if (ret)
		goto err;

	if (empty_int_ie)
		hwmbox_writel(0x0, send_hwmbox() + empty_int_ie);
	ret = devm_request_irq(dev, send_virq, hwmbox_send_empty_int_handler,
			  IRQF_TRIGGER_RISING,
			  "hwmbox1_send_empty", pdev);
	if (empty_int_ie)
		hwmbox_writel(0x1, send_hwmbox() + empty_int_ie);
	if (ret)
		goto free_interrupts;

	return ret;

 free_interrupts:
	nvadsp_free_hwmbox_interrupts(pdev);
 err:
	return ret;
}

int __init nvadsp_hwmbox_init(struct platform_device *pdev)
{
	struct nvadsp_drv_data *drv = platform_get_drvdata(pdev);
	int ret = 0;

	nvadsp_pdev = pdev;
	nvadsp_drv_data = drv;

	hwmboxq_init(&drv->hwmbox_send_queue);

	return ret;
}
