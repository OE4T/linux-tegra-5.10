/*
 * Copyright (c) 2020-2021, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/engines.h>
#include <nvgpu/device.h>
#include <nvgpu/runlist.h>
#include <nvgpu/pbdma.h>
#include <nvgpu/gk20a.h>

static void nvgpu_runlist_init_engine_info(struct gk20a *g,
		struct nvgpu_runlist *runlist,
		const struct nvgpu_device *dev)
{
	u32 i = 0U;

	/*
	 * runlist_pri_base, chram_bar0_offset and pbdma_info
	 * will get over-written with same info, if multiple engines
	 * are present on same runlist. Required optimization will be
	 * done as part of JIRA NVGPU-4980
	 */
	runlist->nvgpu_next.runlist_pri_base =
			dev->next.rl_pri_base;
	runlist->nvgpu_next.chram_bar0_offset =
		g->ops.runlist.get_chram_bar0_offset(g, dev->next.rl_pri_base);

	nvgpu_log(g, gpu_dbg_info, "runlist[%d]: runlist_pri_base 0x%x",
		runlist->id, runlist->nvgpu_next.runlist_pri_base);
	nvgpu_log(g, gpu_dbg_info, "runlist[%d]: chram_bar0_offset 0x%x",
		runlist->id, runlist->nvgpu_next.chram_bar0_offset);

	runlist->nvgpu_next.pbdma_info = &dev->next.pbdma_info;
	for  (i = 0U; i < PBDMA_PER_RUNLIST_SIZE; i++) {
		nvgpu_log(g, gpu_dbg_info,
			"runlist[%d]: pbdma_id[%d] %d pbdma_pri_base[%d] 0x%x",
			runlist->id, i,
			runlist->nvgpu_next.pbdma_info->pbdma_id[i], i,
			runlist->nvgpu_next.pbdma_info->pbdma_pri_base[i]);
	}

	runlist->nvgpu_next.rl_dev_list[dev->next.rleng_id] = dev;
}

static u32 nvgpu_runlist_get_pbdma_mask(struct gk20a *g,
					struct nvgpu_runlist *runlist)
{
	u32 pbdma_mask = 0U;
	u32 i;
	u32 pbdma_id;

	nvgpu_assert(runlist != NULL);

	for ( i = 0U; i < PBDMA_PER_RUNLIST_SIZE; i++) {
		pbdma_id = runlist->nvgpu_next.pbdma_info->pbdma_id[i];
		if (pbdma_id != NVGPU_INVALID_PBDMA_ID)
			pbdma_mask |= BIT32(pbdma_id);
	}
	return pbdma_mask;
}

void nvgpu_next_runlist_init_enginfo(struct gk20a *g, struct nvgpu_fifo *f)
{
	struct nvgpu_runlist *runlist;
	const struct nvgpu_device *dev;
	u32 i, j;

	nvgpu_log_fn(g, " ");

	if (g->is_virtual) {
		return;
	}

	for (i = 0U; i < f->num_runlists; i++) {
		runlist = &f->active_runlists[i];

		nvgpu_log(g, gpu_dbg_info, "Configuring runlist %u (%u)", runlist->id, i);

		for (j = 0U; j < f->num_engines; j++) {
			dev = f->active_engines[j];

			if (dev->runlist_id == runlist->id) {
				runlist->eng_bitmask |= BIT32(dev->engine_id);
				nvgpu_runlist_init_engine_info(g, runlist, dev);
			}
		}

		runlist->pbdma_bitmask = nvgpu_runlist_get_pbdma_mask(g, runlist);

		nvgpu_log(g, gpu_dbg_info, "  Active engine bitmask: 0x%x", runlist->eng_bitmask);
		nvgpu_log(g, gpu_dbg_info, "          PBDMA bitmask: 0x%x", runlist->pbdma_bitmask);
	}

	nvgpu_log_fn(g, "done");
}
