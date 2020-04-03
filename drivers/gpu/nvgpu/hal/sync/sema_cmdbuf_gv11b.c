/*
 * GV11B sema cmdbuf
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

#include <nvgpu/nvgpu_mem.h>
#include <nvgpu/semaphore.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/channel.h>
#include <nvgpu/priv_cmdbuf.h>

#include "sema_cmdbuf_gv11b.h"

u32 gv11b_sema_get_wait_cmd_size(void)
{
	return 10U;
}

u32 gv11b_sema_get_incr_cmd_size(void)
{
	return 12U;
}

void gv11b_sema_add_cmd(struct gk20a *g,
		struct nvgpu_semaphore *s, u64 sema_va,
		struct priv_cmd_entry *cmd,
		u32 off, bool acquire, bool wfi)
{
	nvgpu_log_fn(g, " ");

	/* sema_addr_lo */
	nvgpu_mem_wr32(g, cmd->mem, off++, 0x20010017);
	nvgpu_mem_wr32(g, cmd->mem, off++, sema_va & 0xffffffffULL);

	/* sema_addr_hi */
	nvgpu_mem_wr32(g, cmd->mem, off++, 0x20010018);
	nvgpu_mem_wr32(g, cmd->mem, off++, (sema_va >> 32ULL) & 0xffULL);

	/* payload_lo */
	nvgpu_mem_wr32(g, cmd->mem, off++, 0x20010019);
	nvgpu_mem_wr32(g, cmd->mem, off++, nvgpu_semaphore_get_value(s));

	/* payload_hi : ignored */
	nvgpu_mem_wr32(g, cmd->mem, off++, 0x2001001a);
	nvgpu_mem_wr32(g, cmd->mem, off++, 0);

	if (acquire) {
		/* sema_execute : acq_strict_geq | switch_en | 32bit */
		nvgpu_mem_wr32(g, cmd->mem, off++, 0x2001001b);
		nvgpu_mem_wr32(g, cmd->mem, off++, U32(0x2) | BIT32(12));
	} else {
		/* sema_execute : release | wfi | 32bit */
		nvgpu_mem_wr32(g, cmd->mem, off++, 0x2001001b);
		nvgpu_mem_wr32(g, cmd->mem, off++,
			U32(0x1) | ((wfi ? U32(0x1) : U32(0x0)) << 20U));

		/* non_stall_int : payload is ignored */
		nvgpu_mem_wr32(g, cmd->mem, off++, 0x20010008);
		nvgpu_mem_wr32(g, cmd->mem, off++, 0);
	}
}
