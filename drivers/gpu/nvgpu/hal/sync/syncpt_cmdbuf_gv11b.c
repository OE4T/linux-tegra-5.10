/*
 * GV11B syncpt cmdbuf
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
#include <nvgpu/mm.h>
#include <nvgpu/vm.h>
#include <nvgpu/gmmu.h>
#include <nvgpu/nvgpu_mem.h>
#include <nvgpu/dma.h>
#include <nvgpu/lock.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/channel.h>
#include <nvgpu/priv_cmdbuf.h>
#include <nvgpu/nvhost.h>
#include <nvgpu/static_analysis.h>

#include "syncpt_cmdbuf_gv11b.h"

#ifdef CONFIG_NVGPU_KERNEL_MODE_SUBMIT
void gv11b_syncpt_add_wait_cmd(struct gk20a *g,
		struct priv_cmd_entry *cmd, u32 off,
		u32 id, u32 thresh, u64 gpu_va_base)
{
	u64 gpu_va = gpu_va_base +
		nvgpu_nvhost_syncpt_unit_interface_get_byte_offset(id);

	nvgpu_log_fn(g, " ");

	off = cmd->off + off;

	/* sema_addr_lo */
	nvgpu_mem_wr32(g, cmd->mem, off++, 0x20010017);
	nvgpu_mem_wr32(g, cmd->mem, off++,
			nvgpu_safe_cast_u64_to_u32(gpu_va & 0xffffffffU));

	/* sema_addr_hi */
	nvgpu_mem_wr32(g, cmd->mem, off++, 0x20010018);
	nvgpu_mem_wr32(g, cmd->mem, off++,
			nvgpu_safe_cast_u64_to_u32((gpu_va >> 32U) & 0xffU));

	/* payload_lo */
	nvgpu_mem_wr32(g, cmd->mem, off++, 0x20010019);
	nvgpu_mem_wr32(g, cmd->mem, off++, thresh);

	/* payload_hi : ignored */
	nvgpu_mem_wr32(g, cmd->mem, off++, 0x2001001a);
	nvgpu_mem_wr32(g, cmd->mem, off++, 0U);

	/* sema_execute : acq_strict_geq | switch_en | 32bit */
	nvgpu_mem_wr32(g, cmd->mem, off++, 0x2001001b);
	nvgpu_mem_wr32(g, cmd->mem, off, 0x2U | ((u32)1U << 12U));
}

u32 gv11b_syncpt_get_wait_cmd_size(void)
{
	return 10U;
}

u32 gv11b_syncpt_get_incr_per_release(void)
{
	return 1U;
}

void gv11b_syncpt_add_incr_cmd(struct gk20a *g,
		bool wfi_cmd, struct priv_cmd_entry *cmd,
		u32 id, u64 gpu_va)
{
	u32 off = cmd->off;

	nvgpu_log_fn(g, " ");

	/* sema_addr_lo */
	nvgpu_mem_wr32(g, cmd->mem, off++, 0x20010017);
	nvgpu_mem_wr32(g, cmd->mem, off++,
			nvgpu_safe_cast_u64_to_u32(gpu_va & 0xffffffffU));

	/* sema_addr_hi */
	nvgpu_mem_wr32(g, cmd->mem, off++, 0x20010018);
	nvgpu_mem_wr32(g, cmd->mem, off++,
			nvgpu_safe_cast_u64_to_u32((gpu_va >> 32U) & 0xffU));

	/* payload_lo */
	nvgpu_mem_wr32(g, cmd->mem, off++, 0x20010019);
	nvgpu_mem_wr32(g, cmd->mem, off++, 0);

	/* payload_hi : ignored */
	nvgpu_mem_wr32(g, cmd->mem, off++, 0x2001001a);
	nvgpu_mem_wr32(g, cmd->mem, off++, 0);

	/* sema_execute : release | wfi | 32bit */
	nvgpu_mem_wr32(g, cmd->mem, off++, 0x2001001b);
	nvgpu_mem_wr32(g, cmd->mem, off, (0x1U |
					((u32)(wfi_cmd ? 0x1U : 0x0U) << 20U)));
}

u32 gv11b_syncpt_get_incr_cmd_size(bool wfi_cmd)
{
	return 10U;
}
#endif
