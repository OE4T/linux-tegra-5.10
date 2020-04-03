/*
 * GK20A syncpt cmdbuf
 *
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/nvgpu_mem.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/channel.h>
#include <nvgpu/priv_cmdbuf.h>

#include "syncpt_cmdbuf_gk20a.h"

#ifdef CONFIG_NVGPU_KERNEL_MODE_SUBMIT
void gk20a_syncpt_add_wait_cmd(struct gk20a *g,
		struct priv_cmd_entry *cmd, u32 off,
		u32 id, u32 thresh, u64 gpu_va)
{
	nvgpu_log_fn(g, " ");

	off = cmd->off + off;
	/* syncpoint_a */
	nvgpu_mem_wr32(g, cmd->mem, off++, 0x2001001CU);
	/* payload */
	nvgpu_mem_wr32(g, cmd->mem, off++, thresh);
	/* syncpoint_b */
	nvgpu_mem_wr32(g, cmd->mem, off++, 0x2001001DU);
	/* syncpt_id, switch_en, wait */
	nvgpu_mem_wr32(g, cmd->mem, off++, (id << 8U) | 0x10U);
}

u32 gk20a_syncpt_get_wait_cmd_size(void)
{
	return 4U;
}

u32 gk20a_syncpt_get_incr_per_release(void)
{
	return 2U;
}

void gk20a_syncpt_add_incr_cmd(struct gk20a *g,
		bool wfi_cmd, struct priv_cmd_entry *cmd,
		u32 id, u64 gpu_va)
{
	u32 off = cmd->off;

	nvgpu_log_fn(g, " ");
	if (wfi_cmd) {
		/* wfi */
		nvgpu_mem_wr32(g, cmd->mem, off++, 0x2001001EU);
		/* handle, ignored */
		nvgpu_mem_wr32(g, cmd->mem, off++, 0x00000000U);
	}
	/* syncpoint_a */
	nvgpu_mem_wr32(g, cmd->mem, off++, 0x2001001CU);
	/* payload, ignored */
	nvgpu_mem_wr32(g, cmd->mem, off++, 0U);
	/* syncpoint_b */
	nvgpu_mem_wr32(g, cmd->mem, off++, 0x2001001DU);
	/* syncpt_id, incr */
	nvgpu_mem_wr32(g, cmd->mem, off++, (id << 8U) | 0x1U);
	/* syncpoint_b */
	nvgpu_mem_wr32(g, cmd->mem, off++, 0x2001001DU);
	/* syncpt_id, incr */
	nvgpu_mem_wr32(g, cmd->mem, off++, (id << 8U) | 0x1U);

}

u32 gk20a_syncpt_get_incr_cmd_size(bool wfi_cmd)
{
	if (wfi_cmd) {
		return 8U;
	} else {
		return 6U;
	}
}
#endif

void gk20a_syncpt_free_buf(struct nvgpu_channel *c,
		struct nvgpu_mem *syncpt_buf)
{

}

int gk20a_syncpt_alloc_buf(struct nvgpu_channel *c,
		u32 syncpt_id, struct nvgpu_mem *syncpt_buf)
{
	return 0;
}
