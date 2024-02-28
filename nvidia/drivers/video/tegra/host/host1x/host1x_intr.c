/*
 * drivers/video/tegra/host/host1x/host1x_intr.c
 *
 * Tegra Graphics Host Interrupt Management
 *
 * Copyright (C) 2010 Google, Inc.
 * Copyright (c) 2010-2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>

#include "nvhost_ktime.h"
#include "nvhost_intr.h"
#include "dev.h"

/* Spacing between sync registers */
#define REGISTER_STRIDE 4

static void t20_intr_syncpt_intr_ack(struct nvhost_intr_syncpt *syncpt,
				     bool disable_intr);
static void t20_intr_enable_syncpt_intr(struct nvhost_intr *intr, u32 id);
static void t20_intr_set_syncpt_threshold(struct nvhost_intr *intr,
					  u32 id, u32 thresh);

static irqreturn_t syncpt_thresh_cascade_isr(int irq, void *syncpt_irq_ctx)
{
	struct nvhost_syncpt_irq_ctx *ctx = syncpt_irq_ctx;
	struct nvhost_master *dev = ctx->dev;
	struct nvhost_intr *intr = &dev->intr;
	unsigned long reg;
	int i, id;
	struct nvhost_timespec isr_recv;
	int start_reg = ctx->start_id / 32;
	int end_reg = DIV_ROUND_UP((ctx->end_id + 1), 32);

	nvhost_ktime_get_ts(&isr_recv);

	for (i = start_reg; i < end_reg; i++) {
		reg = host1x_sync_readl(dev,
				host1x_sync_syncpt_thresh_cpu0_int_status_r() +
				i * REGISTER_STRIDE);

		for_each_set_bit(id, &reg, 32) {
			struct nvhost_intr_syncpt *sp;
			int sp_id = i * 32 + id;
			int graphics_host_sp =
				nvhost_syncpt_graphics_host_sp(&dev->syncpt);

			if (sp_id < ctx->start_id)
				continue;
			if (sp_id > ctx->end_id)
				goto out;

			sp = intr->syncpt + sp_id;
			sp->isr_recv = isr_recv;

			/* handle graphics host syncpoint increments
			 * immediately
			 */
			if (sp_id == graphics_host_sp) {
				dev_warn(&dev->dev->dev, "%s(): syncpoint id %d incremented\n",
					 __func__, graphics_host_sp);
				nvhost_syncpt_patch_check(&dev->syncpt);
				t20_intr_syncpt_intr_ack(sp, false);
			} else {
				t20_intr_syncpt_intr_ack(sp, true);
				nvhost_syncpt_thresh_fn(sp);
			}
		}
	}

out:
	return IRQ_HANDLED;
}

static void t20_intr_set_host_clocks_per_usec(struct nvhost_intr *intr, u32 cpm)
{
	struct nvhost_master *dev = intr_to_dev(intr);
	/* write microsecond clock register */
	host1x_sync_writel(dev, host1x_sync_usec_clk_r(), cpm);
	/* set the ip_busy_timeout */
	host1x_sync_writel(dev,
			host1x_sync_ip_busy_timeout_r(), cpm * 500000);
}

static void t20_intr_set_syncpt_threshold(struct nvhost_intr *intr,
	u32 id, u32 thresh)
{
	struct nvhost_master *dev = intr_to_dev(intr);
	host1x_sync_writel(dev, (host1x_sync_syncpt_int_thresh_0_r() +
			id * REGISTER_STRIDE), thresh);
}

static void t20_intr_enable_syncpt_intr(struct nvhost_intr *intr, u32 id)
{
	struct nvhost_master *dev = intr_to_dev(intr);

	host1x_sync_writel(dev,
			host1x_sync_syncpt_thresh_int_enable_cpu0_r() +
			bit_word(id) * REGISTER_STRIDE, bit_mask(id));
}

static void t20_intr_disable_syncpt_intr(struct nvhost_intr *intr, u32 id)
{
	struct nvhost_master *dev = intr_to_dev(intr);

	host1x_sync_writel(dev,
			host1x_sync_syncpt_thresh_int_disable_r() +
			bit_word(id) * REGISTER_STRIDE, bit_mask(id));

	/* clear status for both cpu's */
	host1x_sync_writel(dev,
		host1x_sync_syncpt_thresh_cpu0_int_status_r() +
		bit_word(id) * REGISTER_STRIDE, bit_mask(id));
}

static void t20_intr_disable_all_syncpt_intrs(struct nvhost_intr *intr)
{
	struct nvhost_master *dev = intr_to_dev(intr);
	u32 reg;

	for (reg = 0; reg < bit_word(nvhost_syncpt_nb_hw_pts(&dev->syncpt))
			* REGISTER_STRIDE; reg += REGISTER_STRIDE) {
		/* disable interrupts for both cpu's */
		host1x_sync_writel(dev,
				host1x_sync_syncpt_thresh_int_disable_r() +
				reg, 0xffffffffu);

		/* clear status for both cpu's */
		host1x_sync_writel(dev,
			host1x_sync_syncpt_thresh_cpu0_int_status_r() + reg,
			0xffffffffu);
	}
}

/*
 * Check if some client erroneously added extra increments and we have
 * MIN > MAX situation
 * If yes, set MIN == MAX explicitly
 */
static void t20_intr_handle_extra_increments(struct nvhost_master *dev,
					 unsigned int id)
{
	u32 min, max;

	if (nvhost_syncpt_client_managed(&dev->syncpt, id))
		return;

	min = nvhost_syncpt_update_min(&dev->syncpt, id);
	max = nvhost_syncpt_read_maxval(dev->dev, id);
	if ((s32)(min - max) > 0)
		nvhost_syncpt_set_min_eq_max(&dev->syncpt, id);
}

/*
 * Acknowledge that the syncpoint interrupt is handled. If disable_intr is set,
 * the syncpoint interrupt is also disabled.
 */
static void t20_intr_syncpt_intr_ack(struct nvhost_intr_syncpt *syncpt,
				     bool disable_intr)
{
	unsigned int id = syncpt->id;
	struct nvhost_intr *intr = intr_syncpt_to_intr(syncpt);
	struct nvhost_master *dev = intr_to_dev(intr);

	u32 reg = bit_word(id) * REGISTER_STRIDE;

	if (disable_intr)
		host1x_sync_writel(dev,
		       host1x_sync_syncpt_thresh_int_disable_r() + reg,
		       bit_mask(id));

	t20_intr_handle_extra_increments(dev, id);

	host1x_sync_writel(dev,
		host1x_sync_syncpt_thresh_cpu0_int_status_r() + reg,
		bit_mask(id));
}

/**
 * Host general interrupt service function
 * Handles read / write failures
 */
static irqreturn_t t20_intr_host1x_isr(int irq, void *dev_id)
{
	struct nvhost_intr *intr = dev_id;
	struct nvhost_master *dev = intr_to_dev(intr);
	unsigned long stat;
	u32 ext_stat;
	u32 addr;
	unsigned long intstat;
	int i;

	intstat = host1x_sync_readl(dev, host1x_sync_intstatus_r());
	intr->intstatus = intstat;

	/* Handle host1x interrupt in ISR */
	stat = host1x_sync_readl(dev, host1x_sync_hintstatus_r());
	ext_stat = host1x_sync_readl(dev, host1x_sync_hintstatus_ext_r());

	for_each_set_bit(i, &stat, 32) {
		if (intr->host_isr[i])
			intr->host_isr[i](stat, intr->host_isr_priv[i]);
	}

	if (host1x_sync_hintstatus_ext_ip_read_int_v(ext_stat)) {
		addr = host1x_sync_readl(dev,
				host1x_sync_ip_read_timeout_addr_r());
		pr_err("Host read timeout at address %x\n", addr);
	}

	if (host1x_sync_hintstatus_ext_ip_write_int_v(ext_stat)) {
		addr = host1x_sync_readl(dev,
				host1x_sync_ip_write_timeout_addr_r());
		pr_err("Host write timeout at address %x\n", addr);
	}

	host1x_sync_writel(dev, host1x_sync_hintstatus_ext_r(), ext_stat);
	host1x_sync_writel(dev, host1x_sync_hintstatus_r(), stat);

	host1x_sync_writel(dev, host1x_sync_intstatus_r(), intstat);
	return IRQ_HANDLED;
}

static int intr_debug_dump(struct nvhost_intr *intr, struct output *o)
{
	struct nvhost_master *dev = intr_to_dev(intr);
	int i;

	nvhost_debug_output(o, "\n---- host general irq ----\n\n");
	nvhost_debug_output(o, "sync_hintmask_ext = 0x%08x\n",
		host1x_sync_readl(dev, host1x_sync_hintmask_ext_r()));
	nvhost_debug_output(o, "sync_hintmask = 0x%08x\n",
		host1x_sync_readl(dev, host1x_sync_hintmask_r()));
	nvhost_debug_output(o, "sync_intc0mask = 0x%08x\n",
		host1x_sync_readl(dev, host1x_sync_intc0mask_r()));
	nvhost_debug_output(o, "sync_intmask = 0x%08x\n",
		host1x_sync_readl(dev, host1x_sync_intmask_r()));

	nvhost_debug_output(o, "\n---- host syncpt irq mask ----\n\n");
	for (i = 0; i < DIV_ROUND_UP(nvhost_syncpt_nb_hw_pts(&dev->syncpt), 16);
			i++)
		nvhost_debug_output(o, "syncpt_thresh_int_mask(%d) = 0x%08x\n",
			i, host1x_sync_readl(dev,
				host1x_sync_syncpt_thresh_int_mask_r() +
				i * REGISTER_STRIDE));

	nvhost_debug_output(o, "\n---- host syncpt irq status ----\n\n");
	for (i = 0; i < DIV_ROUND_UP(nvhost_syncpt_nb_hw_pts(&dev->syncpt), 32);
			i++)
		nvhost_debug_output(o, "syncpt_thresh_cpu0_int_status(%d) = 0x%08x\n",
			i, host1x_sync_readl(dev,
				host1x_sync_syncpt_thresh_cpu0_int_status_r() +
				i * REGISTER_STRIDE));

	nvhost_debug_output(o, "\n---- host syncpt thresh ----\n\n");
	for (i = 0; i < nvhost_syncpt_nb_hw_pts(&dev->syncpt); i++) {
		u32 reg = host1x_sync_readl(dev,
				host1x_sync_syncpt_thresh_int_mask_r() +
				bit_word(i * 2) * REGISTER_STRIDE);
		if (!(reg & bit_mask(i * 2)))
			continue;

		nvhost_debug_output(o, "syncpt_int_thresh_thresh_0(%d) = %u\n",
			i, host1x_sync_readl(dev,
				host1x_sync_syncpt_int_thresh_0_r() +
				i * REGISTER_STRIDE));
	}

	return 0;
}

static void intr_enable_host_irq(struct nvhost_intr *intr, int irq)
{
	struct nvhost_master *dev = intr_to_dev(intr);
	long val;

	val = host1x_sync_readl(dev, host1x_sync_hintmask_r());
	val |= BIT(irq);
	host1x_sync_writel(dev, host1x_sync_hintmask_r(), val);
}

static void intr_disable_host_irq(struct nvhost_intr *intr, int irq)
{
	struct nvhost_master *dev = intr_to_dev(intr);
	long val;

	val = host1x_sync_readl(dev, host1x_sync_hintmask_r());
	val &= ~BIT(irq);
	host1x_sync_writel(dev, host1x_sync_hintmask_r(), val);
}

static void t20_intr_resume(struct nvhost_intr *intr)
{
	struct nvhost_master *dev = intr_to_dev(intr);
	u32 val;

	/* increase the auto-ack timout to the maximum value. 2d will hang
	 * otherwise on ap20.
	 */
	host1x_sync_writel(dev, host1x_sync_ctxsw_timeout_cfg_r(), 0xff);

	/* enable graphics host syncpoint interrupt */
	t20_intr_set_syncpt_threshold(intr,
			nvhost_syncpt_graphics_host_sp(&dev->syncpt),
			1);
	t20_intr_enable_syncpt_intr(intr,
			nvhost_syncpt_graphics_host_sp(&dev->syncpt));

	/* enable extra interrupt sources IP_READ_INT and IP_WRITE_INT */
	host1x_sync_writel(dev, host1x_sync_hintmask_ext_r(),
			BIT(30) | BIT(31));

	/* enable extra interrupt sources */
	val = host1x_sync_readl(dev, host1x_sync_hintmask_r());
	val |= BIT(31);
	host1x_sync_writel(dev, host1x_sync_hintmask_r(), val);

	/* enable host module interrupt to CPU0 */
	host1x_sync_writel(dev, host1x_sync_intc0mask_r(), BIT(0));

	/* master enable for general (not syncpt) host interrupts */
	host1x_sync_writel(dev, host1x_sync_intmask_r(), BIT(0));
}

static void t20_intr_suspend(struct nvhost_intr *intr)
{
	struct nvhost_master *dev = intr_to_dev(intr);

	/* master disable for general (not syncpt) host interrupts */
	host1x_sync_writel(dev, host1x_sync_intmask_r(), 0);

	/* disable graphics host syncpoint interrupt */
	t20_intr_disable_syncpt_intr(intr,
			nvhost_syncpt_graphics_host_sp(&dev->syncpt));
}

static int t20_intr_init(struct nvhost_intr *intr)
{
	struct nvhost_master *dev = intr_to_dev(intr);
	int nb_syncpt_irqs = nvhost_syncpt_nb_irqs(&dev->syncpt);
	int err, i;

	intr_op().disable_all_syncpt_intrs(intr);

	for (i = 0; i < nb_syncpt_irqs; i++) {
		err = devm_request_threaded_irq(&dev->dev->dev,
					intr->syncpt_irqs[i], NULL,
					syncpt_thresh_cascade_isr,
					IRQF_ONESHOT, "host_syncpt",
					&intr->syncpt_irq_ctx[i]);
		if (err) {
			nvhost_err(&dev->dev->dev,
			   "failed to request host_syncpt irq %u with err=%d",
			   intr->syncpt_irqs[i], err);
			return err;
		}
	}

	/* master disable for general (not syncpt) host interrupts */
	host1x_sync_writel(dev, host1x_sync_intmask_r(), 0);

	/* clear status & extstatus */
	host1x_sync_writel(dev, host1x_sync_hintstatus_ext_r(),
			0xfffffffful);
	host1x_sync_writel(dev, host1x_sync_hintstatus_r(),
			0xfffffffful);

	err = request_threaded_irq(intr->general_irq, NULL,
				t20_intr_host1x_isr,
				IRQF_ONESHOT, "host_status", intr);

	return err;
}

static void t20_intr_deinit(struct nvhost_intr *intr)
{
	free_irq(intr->general_irq, intr);
}

static const struct nvhost_intr_ops host1x_intr_ops = {
	.init = t20_intr_init,
	.deinit = t20_intr_deinit,
	.resume = t20_intr_resume,
	.suspend = t20_intr_suspend,
	.set_host_clocks_per_usec = t20_intr_set_host_clocks_per_usec,
	.set_syncpt_threshold = t20_intr_set_syncpt_threshold,
	.enable_syncpt_intr = t20_intr_enable_syncpt_intr,
	.disable_syncpt_intr = t20_intr_disable_syncpt_intr,
	.disable_all_syncpt_intrs = t20_intr_disable_all_syncpt_intrs,
	.debug_dump = intr_debug_dump,
	.enable_host_irq = intr_enable_host_irq,
	.disable_host_irq = intr_disable_host_irq,
};
