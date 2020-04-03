/*
 * GK20A sema cmdbuf
 *
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/log.h>
#include <nvgpu/nvgpu_mem.h>
#include <nvgpu/semaphore.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/channel.h>
#include <nvgpu/priv_cmdbuf.h>

#include "sema_cmdbuf_gk20a.h"

u32 gk20a_sema_get_wait_cmd_size(void)
{
	return 8U;
}

u32 gk20a_sema_get_incr_cmd_size(void)
{
	return 10U;
}

void gk20a_sema_add_cmd(struct gk20a *g, struct nvgpu_semaphore *s,
		u64 sema_va, struct priv_cmd_entry *cmd,
		u32 off, bool acquire, bool wfi)
{
	nvgpu_log_fn(g, " ");

	/* semaphore_a */
	nvgpu_mem_wr32(g, cmd->mem, off++, 0x20010004U);
	/* offset_upper */
	nvgpu_mem_wr32(g, cmd->mem, off++, (u32)(sema_va >> 32) & 0xffU);
	/* semaphore_b */
	nvgpu_mem_wr32(g, cmd->mem, off++, 0x20010005U);
	/* offset */
	nvgpu_mem_wr32(g, cmd->mem, off++, (u32)sema_va & 0xffffffff);

	if (acquire) {
		/* semaphore_c */
		nvgpu_mem_wr32(g, cmd->mem, off++, 0x20010006U);
		/* payload */
		nvgpu_mem_wr32(g, cmd->mem, off++,
			       nvgpu_semaphore_get_value(s));
		/* semaphore_d */
		nvgpu_mem_wr32(g, cmd->mem, off++, 0x20010007U);
		/* operation: acq_geq, switch_en */
		nvgpu_mem_wr32(g, cmd->mem, off++, 0x4U | BIT32(12));
	} else {
		/* semaphore_c */
		nvgpu_mem_wr32(g, cmd->mem, off++, 0x20010006U);
		/* payload */
		nvgpu_mem_wr32(g, cmd->mem, off++,
			       nvgpu_semaphore_get_value(s));
		/* semaphore_d */
		nvgpu_mem_wr32(g, cmd->mem, off++, 0x20010007U);
		/* operation: release, wfi */
		nvgpu_mem_wr32(g, cmd->mem, off++,
				0x2UL | ((wfi ? 0x0UL : 0x1UL) << 20));
		/* non_stall_int */
		nvgpu_mem_wr32(g, cmd->mem, off++, 0x20010008U);
		/* ignored */
		nvgpu_mem_wr32(g, cmd->mem, off++, 0U);
	}
}
