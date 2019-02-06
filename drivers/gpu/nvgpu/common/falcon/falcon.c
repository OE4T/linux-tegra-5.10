/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <nvgpu/gk20a.h>
#include <nvgpu/timers.h>

#include "falcon_priv.h"

/* Delay depends on memory size and pwr_clk
 * delay = (MAX {IMEM_SIZE, DMEM_SIZE} * 64 + 1) / pwr_clk
 * Timeout set is 1msec & status check at interval 10usec
 */
#define MEM_SCRUBBING_TIMEOUT_MAX 1000
#define MEM_SCRUBBING_TIMEOUT_DEFAULT 10

int nvgpu_falcon_wait_idle(struct nvgpu_falcon *flcn)
{
	struct gk20a *g;
	struct nvgpu_falcon_ops *flcn_ops;
	struct nvgpu_timeout timeout;

	if (flcn == NULL) {
		return -EINVAL;
	}

	g = flcn->g;
	flcn_ops = &flcn->flcn_ops;

	if (flcn_ops->is_falcon_idle == NULL) {
		nvgpu_warn(g, "Invalid op on falcon 0x%x ", flcn->flcn_id);
		return -EINVAL;
	}

	nvgpu_timeout_init(g, &timeout, 2000, NVGPU_TIMER_RETRY_TIMER);

	/* wait for falcon idle */
	do {
		if (flcn_ops->is_falcon_idle(flcn)) {
			break;
		}

		if (nvgpu_timeout_expired_msg(&timeout,
			"waiting for falcon idle") != 0) {
			return -ETIMEDOUT;
		}

		nvgpu_usleep_range(100, 200);
	} while (true);

	return 0;
}

int nvgpu_falcon_mem_scrub_wait(struct nvgpu_falcon *flcn)
{
	struct nvgpu_falcon_ops *flcn_ops;
	struct nvgpu_timeout timeout;
	int status = 0;

	if (flcn == NULL) {
		return -EINVAL;
	}

	flcn_ops = &flcn->flcn_ops;

	if (flcn_ops->is_falcon_scrubbing_done == NULL) {
		nvgpu_warn(flcn->g, "Invalid op on falcon 0x%x ",
			flcn->flcn_id);
		return -EINVAL;
	}

	/* check IMEM/DMEM scrubbing complete status */
	nvgpu_timeout_init(flcn->g, &timeout,
		MEM_SCRUBBING_TIMEOUT_MAX /
		MEM_SCRUBBING_TIMEOUT_DEFAULT,
		NVGPU_TIMER_RETRY_TIMER);
	do {
		if (flcn_ops->is_falcon_scrubbing_done(flcn)) {
			goto exit;
		}
		nvgpu_udelay(MEM_SCRUBBING_TIMEOUT_DEFAULT);
	} while (nvgpu_timeout_expired(&timeout) == 0);

	if (nvgpu_timeout_peek_expired(&timeout) != 0) {
		status = -ETIMEDOUT;
	}

exit:
	return status;
}

int nvgpu_falcon_reset(struct nvgpu_falcon *flcn)
{
	struct nvgpu_falcon_ops *flcn_ops;
	struct gk20a *g;
	int status = 0;

	if (flcn == NULL) {
		return -EINVAL;
	}

	g = flcn->g;
	flcn_ops = &flcn->flcn_ops;

	if (flcn->flcn_engine_dep_ops.reset_eng != NULL) {
		/* falcon & engine reset */
		status = flcn->flcn_engine_dep_ops.reset_eng(g);
	} else if (flcn_ops->reset != NULL) {
		flcn_ops->reset(flcn);
	} else {
		nvgpu_warn(g, "Invalid op on falcon 0x%x ", flcn->flcn_id);
		status = -EINVAL;
	}

	if (status == 0) {
		status = nvgpu_falcon_mem_scrub_wait(flcn);
	}

	return status;
}

void nvgpu_falcon_set_irq(struct nvgpu_falcon *flcn, bool enable,
	u32 intr_mask, u32 intr_dest)
{
	struct nvgpu_falcon_ops *flcn_ops;
	struct gk20a *g;

	if (flcn == NULL) {
		return;
	}

	g = flcn->g;
	flcn_ops = &flcn->flcn_ops;

	if (flcn_ops->set_irq == NULL) {
		nvgpu_warn(g, "Invalid op on falcon 0x%x ", flcn->flcn_id);
		return;
	}

	if (!flcn->is_interrupt_enabled) {
		nvgpu_warn(g, "Interrupt not supported on flcn 0x%x ",
			flcn->flcn_id);
		/* Keep interrupt disabled */
		enable = false;
	}

	flcn_ops->set_irq(flcn, enable, intr_mask, intr_dest);
}

int nvgpu_falcon_wait_for_halt(struct nvgpu_falcon *flcn, unsigned int timeout)
{
	struct nvgpu_falcon_ops *flcn_ops;
	struct gk20a *g;
	struct nvgpu_timeout to;
	int status = 0;

	if (flcn == NULL) {
		return -EINVAL;
	}

	g = flcn->g;
	flcn_ops = &flcn->flcn_ops;

	if (flcn_ops->is_falcon_cpu_halted == NULL) {
		nvgpu_warn(flcn->g, "Invalid op on falcon 0x%x ",
			flcn->flcn_id);
		return -EINVAL;
	}

	nvgpu_timeout_init(g, &to, timeout, NVGPU_TIMER_CPU_TIMER);
	do {
		if (flcn_ops->is_falcon_cpu_halted(flcn)) {
			break;
		}

		nvgpu_udelay(10);
	} while (nvgpu_timeout_expired(&to) == 0);

	if (nvgpu_timeout_peek_expired(&to) != 0) {
		status = -ETIMEDOUT;
	}

	return status;
}

int nvgpu_falcon_clear_halt_intr_status(struct nvgpu_falcon *flcn,
	unsigned int timeout)
{
	struct gk20a *g;
	struct nvgpu_falcon_ops *flcn_ops;
	struct nvgpu_timeout to;
	int status = 0;

	if (flcn == NULL) {
		return -EINVAL;
	}

	g = flcn->g;
	flcn_ops = &flcn->flcn_ops;

	if (flcn_ops->clear_halt_interrupt_status == NULL) {
		nvgpu_warn(flcn->g, "Invalid op on falcon 0x%x ",
			flcn->flcn_id);
		return -EINVAL;
	}

	nvgpu_timeout_init(g, &to, timeout, NVGPU_TIMER_CPU_TIMER);
	do {
		if (flcn_ops->clear_halt_interrupt_status(flcn)) {
			break;
		}

		nvgpu_udelay(1);
	} while (nvgpu_timeout_expired(&to) == 0);

	if (nvgpu_timeout_peek_expired(&to) != 0) {
		status = -ETIMEDOUT;
	}

	return status;
}

int nvgpu_falcon_copy_from_emem(struct nvgpu_falcon *flcn,
	u32 src, u8 *dst, u32 size, u8 port)
{
	struct nvgpu_falcon_engine_dependency_ops *flcn_dops;
	int status = -EINVAL;

	if (flcn == NULL) {
		return -EINVAL;
	}

	flcn_dops = &flcn->flcn_engine_dep_ops;

	if (flcn_dops->copy_from_emem != NULL) {
		status = flcn_dops->copy_from_emem(flcn, src, dst, size, port);
	} else {
		nvgpu_warn(flcn->g, "Invalid op on falcon 0x%x ",
			flcn->flcn_id);
	}

	return status;
}

int nvgpu_falcon_copy_to_emem(struct nvgpu_falcon *flcn,
	u32 dst, u8 *src, u32 size, u8 port)
{
	struct nvgpu_falcon_engine_dependency_ops *flcn_dops;
	int status = -EINVAL;

	if (flcn == NULL) {
		return -EINVAL;
	}

	flcn_dops = &flcn->flcn_engine_dep_ops;

	if (flcn_dops->copy_to_emem != NULL) {
		status = flcn_dops->copy_to_emem(flcn, dst, src, size, port);
	} else {
		nvgpu_warn(flcn->g, "Invalid op on falcon 0x%x ",
			flcn->flcn_id);
	}

	return status;
}

static int falcon_memcpy_params_check(struct nvgpu_falcon *flcn,
		u32 offset, u32 size, enum falcon_mem_type mem_type)
{
	struct nvgpu_falcon_ops *flcn_ops = &flcn->flcn_ops;
	struct gk20a *g = flcn->g;
	u32 mem_size = 0;
	int ret = -EINVAL;

	if (size == 0U) {
		nvgpu_err(g, "size is zero");
		goto exit;
	}

	if ((offset & 0x3U) != 0U) {
		nvgpu_err(g, "offset (0x%08x) not 4-byte aligned", offset);
		goto exit;
	}

	if (flcn_ops->get_mem_size == NULL) {
		nvgpu_warn(flcn->g, "Invalid op on falcon 0x%x ",
			flcn->flcn_id);
		goto exit;
	}

	mem_size = flcn_ops->get_mem_size(flcn, mem_type);
	if (!(offset <= mem_size && (offset + size) <= mem_size)) {
		nvgpu_err(g, "flcn-id 0x%x, copy overflow ",
			flcn->flcn_id);
		nvgpu_err(g, "total size 0x%x, offset 0x%x, copy size 0x%x",
			mem_size, offset, size);
		goto exit;
	}

	ret = 0;

exit:
	return ret;
}

int nvgpu_falcon_copy_from_dmem(struct nvgpu_falcon *flcn,
	u32 src, u8 *dst, u32 size, u8 port)
{
	struct nvgpu_falcon_ops *flcn_ops;
	int status = -EINVAL;

	if (flcn == NULL) {
		goto exit;
	}

	flcn_ops = &flcn->flcn_ops;

	if (flcn_ops->copy_from_dmem == NULL) {
		nvgpu_warn(flcn->g, "Invalid op on falcon 0x%x ",
			flcn->flcn_id);
		goto exit;
	}

	if (falcon_memcpy_params_check(flcn, src, size, MEM_DMEM) != 0) {
		nvgpu_err(flcn->g, "incorrect parameters");
		goto exit;
	}

	nvgpu_mutex_acquire(&flcn->copy_lock);
	status = flcn_ops->copy_from_dmem(flcn, src, dst, size, port);
	nvgpu_mutex_release(&flcn->copy_lock);

exit:
	return status;
}

int nvgpu_falcon_copy_to_dmem(struct nvgpu_falcon *flcn,
	u32 dst, u8 *src, u32 size, u8 port)
{
	struct nvgpu_falcon_ops *flcn_ops;
	int status = -EINVAL;

	if (flcn == NULL) {
		goto exit;
	}

	flcn_ops = &flcn->flcn_ops;

	if (flcn_ops->copy_to_dmem == NULL) {
		nvgpu_warn(flcn->g, "Invalid op on falcon 0x%x ",
			flcn->flcn_id);
		goto exit;
	}

	if (falcon_memcpy_params_check(flcn, dst, size, MEM_DMEM) != 0) {
		nvgpu_err(flcn->g, "incorrect parameters");
		goto exit;
	}

	nvgpu_mutex_acquire(&flcn->copy_lock);
	status = flcn_ops->copy_to_dmem(flcn, dst, src, size, port);
	nvgpu_mutex_release(&flcn->copy_lock);

exit:
	return status;
}

int nvgpu_falcon_copy_from_imem(struct nvgpu_falcon *flcn,
	u32 src, u8 *dst, u32 size, u8 port)
{
	struct nvgpu_falcon_ops *flcn_ops;
	int status = -EINVAL;

	if (flcn == NULL) {
		goto exit;
	}

	flcn_ops = &flcn->flcn_ops;

	if (flcn_ops->copy_from_imem == NULL) {
		nvgpu_warn(flcn->g, "Invalid op on falcon 0x%x ",
			flcn->flcn_id);
		goto exit;
	}

	if (falcon_memcpy_params_check(flcn, src, size, MEM_IMEM) != 0) {
		nvgpu_err(flcn->g, "incorrect parameters");
		goto exit;
	}

	nvgpu_mutex_acquire(&flcn->copy_lock);
	status = flcn_ops->copy_from_imem(flcn, src, dst, size, port);
	nvgpu_mutex_release(&flcn->copy_lock);

exit:
	return status;
}

int nvgpu_falcon_copy_to_imem(struct nvgpu_falcon *flcn,
	u32 dst, u8 *src, u32 size, u8 port, bool sec, u32 tag)
{
	struct nvgpu_falcon_ops *flcn_ops;
	int status = -EINVAL;

	if (flcn == NULL) {
		goto exit;
	}

	flcn_ops = &flcn->flcn_ops;

	if (flcn_ops->copy_to_imem == NULL) {
		nvgpu_warn(flcn->g, "Invalid op on falcon 0x%x ",
			flcn->flcn_id);
		goto exit;
	}

	if (falcon_memcpy_params_check(flcn, dst, size, MEM_IMEM) != 0) {
		nvgpu_err(flcn->g, "incorrect parameters");
		goto exit;
	}

	nvgpu_mutex_acquire(&flcn->copy_lock);
	status = flcn_ops->copy_to_imem(flcn, dst, src, size, port,
				sec, tag);
	nvgpu_mutex_release(&flcn->copy_lock);

exit:
	return status;
}

static void falcon_print_mem(struct nvgpu_falcon *flcn, u32 src,
	u32 size, enum falcon_mem_type mem_type)
{
	u32 buff[64] = {0};
	u32 total_block_read = 0;
	u32 byte_read_count = 0;
	u32 i = 0;
	int status = 0;

	nvgpu_info(flcn->g, " offset 0x%x  size %d bytes", src, size);

	total_block_read = size >> 8;
	do {
		byte_read_count =
			(total_block_read != 0U) ? (u32)sizeof(buff) : size;

		if (byte_read_count == 0U) {
			break;
		}

		if (mem_type == MEM_DMEM) {
			status = nvgpu_falcon_copy_from_dmem(flcn, src,
				(u8 *)buff, byte_read_count, 0);
		} else {
			status = nvgpu_falcon_copy_from_imem(flcn, src,
				(u8 *)buff, byte_read_count, 0);
		}

		if (status != 0) {
			nvgpu_err(flcn->g, "MEM print failed");
			break;
		}

		for (i = 0U; i < (byte_read_count >> 2U); i += 4U) {
			nvgpu_info(flcn->g, "%#06x: %#010x %#010x %#010x %#010x",
				src + (i << 2U), buff[i], buff[i+1U],
				buff[i+2U], buff[i+3U]);
		}

		src += byte_read_count;
		size -= byte_read_count;
	} while (total_block_read-- != 0U);
}

void nvgpu_falcon_print_dmem(struct nvgpu_falcon *flcn, u32 src, u32 size)
{
	if (flcn == NULL) {
		return;
	}

	nvgpu_info(flcn->g, " PRINT DMEM ");
	falcon_print_mem(flcn, src, size, MEM_DMEM);
}

void nvgpu_falcon_print_imem(struct nvgpu_falcon *flcn, u32 src, u32 size)
{
	if (flcn == NULL) {
		return;
	}

	nvgpu_info(flcn->g, " PRINT IMEM ");
	falcon_print_mem(flcn, src, size, MEM_IMEM);
}

int nvgpu_falcon_bootstrap(struct nvgpu_falcon *flcn, u32 boot_vector)
{
	struct nvgpu_falcon_ops *flcn_ops;
	int status = -EINVAL;

	if (flcn == NULL) {
		return -EINVAL;
	}

	flcn_ops = &flcn->flcn_ops;

	if (flcn_ops->bootstrap != NULL) {
		status = flcn_ops->bootstrap(flcn, boot_vector);
	} else {
		nvgpu_warn(flcn->g, "Invalid op on falcon 0x%x ",
			flcn->flcn_id);
	}

	return status;
}

u32 nvgpu_falcon_mailbox_read(struct nvgpu_falcon *flcn, u32 mailbox_index)
{
	struct nvgpu_falcon_ops *flcn_ops;
	u32 data = 0;

	if (flcn == NULL) {
		goto exit;
	}

	flcn_ops = &flcn->flcn_ops;

	if (flcn_ops->mailbox_read == NULL) {
		nvgpu_warn(flcn->g, "Invalid op on falcon 0x%x ",
			flcn->flcn_id);
		goto exit;
	}

	if (mailbox_index >= FALCON_MAILBOX_COUNT) {
		nvgpu_err(flcn->g, "incorrect mailbox id %d", mailbox_index);
		goto exit;
	}

	data = flcn_ops->mailbox_read(flcn, mailbox_index);

exit:
	return data;
}

void nvgpu_falcon_mailbox_write(struct nvgpu_falcon *flcn, u32 mailbox_index,
		u32 data)
{
	struct nvgpu_falcon_ops *flcn_ops;

	if (flcn == NULL) {
		goto exit;
	}

	flcn_ops = &flcn->flcn_ops;

	if (flcn_ops->mailbox_write == NULL) {
		nvgpu_warn(flcn->g, "Invalid op on falcon 0x%x ",
			flcn->flcn_id);
		goto exit;
	}

	if (mailbox_index >= FALCON_MAILBOX_COUNT) {
		nvgpu_err(flcn->g, "incorrect mailbox id %d", mailbox_index);
		goto exit;
	}

	flcn_ops->mailbox_write(flcn, mailbox_index, data);

exit:
	return;
}

void nvgpu_falcon_dump_stats(struct nvgpu_falcon *flcn)
{
	struct nvgpu_falcon_ops *flcn_ops;

	if (flcn == NULL) {
		return;
	}

	flcn_ops = &flcn->flcn_ops;

	if (flcn_ops->dump_falcon_stats != NULL) {
		flcn_ops->dump_falcon_stats(flcn);
	} else {
		nvgpu_warn(flcn->g, "Invalid op on falcon 0x%x ",
			flcn->flcn_id);
	}
}

int nvgpu_falcon_bl_bootstrap(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_bl_info *bl_info)
{
	struct nvgpu_falcon_ops *flcn_ops;
	int status = -EINVAL;
	u32 imem_size;

	if (flcn == NULL) {
		goto exit;
	}

	flcn_ops = &flcn->flcn_ops;

	if (flcn_ops->bl_bootstrap == NULL) {
		nvgpu_warn(flcn->g, "Invalid op on falcon 0x%x ",
			flcn->flcn_id);
		goto exit;
	}

	imem_size = flcn_ops->get_mem_size(flcn, MEM_IMEM);
	if (bl_info->bl_size > imem_size) {
		nvgpu_err(flcn->g, "bootloader size greater than IMEM size");
		goto exit;
	}

	status = flcn_ops->bl_bootstrap(flcn, bl_info);

exit:
	return status;
}

void nvgpu_falcon_get_ctls(struct nvgpu_falcon *flcn, u32 *sctl, u32 *cpuctl)
{
	struct nvgpu_falcon_ops *flcn_ops;

	if (flcn == NULL) {
		return;
	}

	flcn_ops = &flcn->flcn_ops;

	if (flcn_ops->get_falcon_ctls != NULL) {
		flcn_ops->get_falcon_ctls(flcn, sctl, cpuctl);
	} else {
		nvgpu_warn(flcn->g, "Invalid op on falcon 0x%x ",
			flcn->flcn_id);
	}
}

int nvgpu_falcon_get_dmem_size(struct nvgpu_falcon *flcn, u32 *dmem_size)
{
	struct nvgpu_falcon_ops *flcn_ops;

	if (flcn == NULL) {
		return -EINVAL;
	}

	flcn_ops = &flcn->flcn_ops;

	if (flcn_ops->get_mem_size != NULL) {
		*dmem_size = flcn_ops->get_mem_size(flcn, MEM_DMEM);
	} else {
		nvgpu_warn(flcn->g, "Invalid op on falcon 0x%x ",
			flcn->flcn_id);
	}

	return 0;
}

struct gk20a *nvgpu_falcon_to_gk20a(struct nvgpu_falcon *flcn)
{
	return flcn->g;
}

u32 nvgpu_falcon_get_id(struct nvgpu_falcon *flcn)
{
	return flcn->flcn_id;
}

static struct nvgpu_falcon **falcon_get_instance(struct gk20a *g, u32 flcn_id)
{
	struct nvgpu_falcon **flcn_p = NULL;

	switch (flcn_id) {
	case FALCON_ID_PMU:
		flcn_p = &g->pmu.flcn;
		break;
	case FALCON_ID_SEC2:
		flcn_p = &g->sec2.flcn;
		break;
	case FALCON_ID_FECS:
		flcn_p = &g->fecs_flcn;
		break;
	case FALCON_ID_GPCCS:
		flcn_p = &g->gpccs_flcn;
		break;
	case FALCON_ID_NVDEC:
		flcn_p = &g->nvdec_flcn;
		break;
	case FALCON_ID_MINION:
		flcn_p = &g->minion_flcn;
		break;
	case FALCON_ID_GSPLITE:
		flcn_p = &g->gsp_flcn;
		break;
	default:
		nvgpu_err(g, "Invalid/Unsupported falcon ID %x", flcn_id);
		break;
	};

	return flcn_p;
}

int nvgpu_falcon_sw_init(struct gk20a *g, u32 flcn_id)
{
	struct nvgpu_falcon **flcn_p = NULL, *flcn = NULL;
	struct gpu_ops *gops = &g->ops;
	int err;

	flcn_p = falcon_get_instance(g, flcn_id);
	if (flcn_p == NULL) {
		return -ENODEV;
	}

	flcn = (struct nvgpu_falcon *)
	       nvgpu_kmalloc(g, sizeof(struct nvgpu_falcon));

	if (flcn == NULL) {
		return -ENOMEM;
	}

	err = nvgpu_mutex_init(&flcn->copy_lock);
	if (err != 0) {
		nvgpu_err(g, "Error in flcn.copy_lock mutex initialization");
		nvgpu_kfree(g, flcn);
		return err;
	}

	flcn->flcn_id = flcn_id;
	flcn->g = g;

	*flcn_p = flcn;

	/* call to HAL method to assign flcn base & ops to selected falcon */
	return gops->falcon.falcon_hal_sw_init(flcn);
}

void nvgpu_falcon_sw_free(struct gk20a *g, u32 flcn_id)
{
	struct nvgpu_falcon **flcn_p = NULL, *flcn = NULL;
	struct gpu_ops *gops = &g->ops;

	flcn_p = falcon_get_instance(g, flcn_id);
	if ((flcn_p == NULL) || (*flcn_p == NULL)) {
		return;
	}

	flcn = *flcn_p;
	gops->falcon.falcon_hal_sw_free(flcn);
	nvgpu_mutex_destroy(&flcn->copy_lock);
	nvgpu_kfree(g, flcn);
	*flcn_p = NULL;
}
