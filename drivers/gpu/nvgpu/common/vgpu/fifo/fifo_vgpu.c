/*
 * Virtualized GPU Fifo
 *
 * Copyright (c) 2014-2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/trace.h>
#include <nvgpu/kmem.h>
#include <nvgpu/dma.h>
#include <nvgpu/atomic.h>
#include <nvgpu/bug.h>
#include <nvgpu/barrier.h>
#include <nvgpu/io.h>
#include <nvgpu/error_notifier.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/channel.h>
#include <nvgpu/fifo.h>
#include <nvgpu/runlist.h>
#include <nvgpu/string.h>
#include <nvgpu/vm_area.h>
#include <nvgpu/vgpu/vgpu_ivc.h>
#include <nvgpu/vgpu/vgpu.h>

#include <hal/fifo/tsg_gk20a.h>

#include "fifo_vgpu.h"
#include "channel_vgpu.h"
#include "tsg_vgpu.h"

void vgpu_fifo_cleanup_sw(struct gk20a *g)
{
	u32 i;
	struct nvgpu_fifo *f = &g->fifo;

	for (i = 0U; i < f->max_engines; i++) {
		if (f->host_engines[i] == NULL) {
			continue;
		}

		/*
		 * Cast to (void *) to get rid of the constness.
		 */
		nvgpu_kfree(g, (void *)f->host_engines[i]);
	}
	nvgpu_fifo_cleanup_sw_common(g);
}

int vgpu_fifo_setup_sw(struct gk20a *g)
{
	struct nvgpu_fifo *f = &g->fifo;
	struct vgpu_priv_data *priv = vgpu_get_priv_data(g);
	int err = 0;

	nvgpu_log_fn(g, " ");

	if (f->sw_ready) {
		nvgpu_log_fn(g, "skip init");
		return 0;
	}

	err = nvgpu_fifo_setup_sw_common(g);
	if (err != 0) {
		nvgpu_err(g, "fifo sw setup failed, err=%d", err);
		return err;
	}

#ifdef CONFIG_NVGPU_KERNEL_MODE_SUBMIT
	err = nvgpu_channel_worker_init(g);
	if (err) {
		goto clean_up;
	}
#endif

	f->channel_base = priv->constants.channel_base;

	f->sw_ready = true;

	nvgpu_log_fn(g, "done");
	return 0;

#ifdef CONFIG_NVGPU_KERNEL_MODE_SUBMIT
clean_up:
	/* FIXME: unmap from bar1 */
	nvgpu_fifo_cleanup_sw_common(g);
#endif

	return err;
}

int vgpu_init_fifo_setup_hw(struct gk20a *g)
{
#ifdef CONFIG_NVGPU_USERD
	struct nvgpu_fifo *f = &g->fifo;
	u32 v, v1 = 0x33, v2 = 0x55;
	struct nvgpu_mem *mem = &f->userd_slabs[0];
	u32 bar1_vaddr;
	volatile u32 *cpu_vaddr;
	int err;

	nvgpu_log_fn(g, " ");

	/* allocate and map first userd slab for bar1 test. */
	err = nvgpu_dma_alloc_sys(g, NVGPU_CPU_PAGE_SIZE, mem);
	if (err != 0) {
		nvgpu_err(g, "userd allocation failed, err=%d", err);
		return err;
	}
	mem->gpu_va = g->ops.mm.bar1_map_userd(g, mem, 0);
	f->userd_gpu_va = mem->gpu_va;

	/* test write, read through bar1 @ userd region before
	 * turning on the snooping */

	cpu_vaddr = mem->cpu_va;
	bar1_vaddr = mem->gpu_va;

	nvgpu_log_info(g, "test bar1 @ vaddr 0x%x",
		   bar1_vaddr);

	v = gk20a_bar1_readl(g, bar1_vaddr);

	*cpu_vaddr = v1;
	nvgpu_mb();

	if (v1 != gk20a_bar1_readl(g, bar1_vaddr)) {
		nvgpu_err(g, "bar1 broken @ gk20a!");
		return -EINVAL;
	}

	gk20a_bar1_writel(g, bar1_vaddr, v2);

	if (v2 != gk20a_bar1_readl(g, bar1_vaddr)) {
		nvgpu_err(g, "bar1 broken @ gk20a!");
		return -EINVAL;
	}

	/* is it visible to the cpu? */
	if (*cpu_vaddr != v2) {
		nvgpu_err(g, "cpu didn't see bar1 write @ %p!",
			cpu_vaddr);
	}

	/* put it back */
	gk20a_bar1_writel(g, bar1_vaddr, v);

	nvgpu_log_fn(g, "done");
#endif
	return 0;
}

int vgpu_fifo_isr(struct gk20a *g, struct tegra_vgpu_fifo_intr_info *info)
{
	struct nvgpu_channel *ch = nvgpu_channel_from_id(g, info->chid);

	nvgpu_log_fn(g, " ");

	nvgpu_err(g, "fifo intr (%d) on ch %u",
		info->type, info->chid);

	switch (info->type) {
	case TEGRA_VGPU_FIFO_INTR_PBDMA:
		g->ops.channel.set_error_notifier(ch,
			NVGPU_ERR_NOTIFIER_PBDMA_ERROR);
		break;
	case TEGRA_VGPU_FIFO_INTR_CTXSW_TIMEOUT:
		g->ops.channel.set_error_notifier(ch,
			NVGPU_ERR_NOTIFIER_FIFO_ERROR_IDLE_TIMEOUT);
		break;
	case TEGRA_VGPU_FIFO_INTR_MMU_FAULT:
		vgpu_tsg_set_ctx_mmu_error(g, info->chid);
		nvgpu_channel_abort(ch, false);
		break;
	default:
		WARN_ON(1);
		break;
	}

	nvgpu_channel_put(ch);
	return 0;
}
