/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/device.h>
#include <nvgpu/engines.h>
#include <nvgpu/runlist.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/fifo.h>

int nvgpu_next_engine_init_one_dev(struct gk20a *g,
				   const struct nvgpu_device *dev)
{
	struct nvgpu_device *dev_rw = (struct nvgpu_device *)dev;

	/*
	 * Currently due to the nature of the nvgpu_next repo, this will still
	 * be called even on non-ga10b systems. Eventually this code will fold into
	 * the nvgpu-linux repo, at which point this logic will be present in
	 * nvgpu_engine_init_one_dev().
	 *
	 * In any event, the purpose of this is to make sure we _don't_ execute
	 * this code pre-ga10b. We can check for HALs that only exist on ga10x to
	 * short circuit.
	 */
	if (g->ops.runlist.get_engine_id_from_rleng_id == NULL) {
		return 0;
	}

	/*
	 * Init PBDMA info for this device; needs FIFO to be alive to do this.
	 * SW expects at least pbdma instance0 to be valid.
	 *
	 * See JIRA NVGPU-4980 for multiple pbdma support.
	 */
	g->ops.runlist.get_pbdma_info(g,
				      dev->next.rl_pri_base,
				      &dev_rw->next.pbdma_info);
	if (dev->next.pbdma_info.pbdma_id[ENGINE_PBDMA_INSTANCE0] ==
	    NVGPU_INVALID_PBDMA_ID) {
		nvgpu_err(g, "busted pbdma info: no pbdma for engine id:%d",
			  dev->engine_id);
		return -EINVAL;
	}

	dev_rw->pbdma_id = dev->next.pbdma_info.pbdma_id[ENGINE_PBDMA_INSTANCE0];

	nvgpu_log(g, gpu_dbg_device, "Parsed engine: ID: %u", dev->engine_id);
	nvgpu_log(g, gpu_dbg_device, "  inst_id %u,  runlist_id: %u,  fault id %u",
                  dev->inst_id, dev->runlist_id, dev->fault_id);
	nvgpu_log(g, gpu_dbg_device, "  intr_id %u,  reset_id %u",
                  dev->intr_id, dev->reset_id);
	nvgpu_log(g, gpu_dbg_device, "  engine_type %u",
                  dev->type);
	nvgpu_log(g, gpu_dbg_device, "  reset_id 0x%08x, rleng_id 0x%x",
                  dev->reset_id, dev->next.rleng_id);
	nvgpu_log(g, gpu_dbg_device, "  runlist_pri_base 0x%x",
		  dev->next.rl_pri_base);

	return 0;
}
