/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/ctxsw_trace.h>
#include <nvgpu/vgpu/tegra_vgpu.h>
#include <nvgpu/vgpu/vgpu_ivm.h>
#include <nvgpu/gk20a.h>

#include <linux/mm.h>

#include "vgpu/fecs_trace_vgpu.h"

void vgpu_fecs_trace_data_update(struct gk20a *g)
{
	gk20a_ctxsw_trace_wake_up(g, 0);
}

int vgpu_alloc_user_buffer(struct gk20a *g, void **buf, size_t *size)
{
	struct vgpu_fecs_trace *vcst = (struct vgpu_fecs_trace *)g->fecs_trace;

	*buf = vcst->buf;
	*size = vgpu_ivm_get_size(vcst->cookie);
	return 0;
}

int vgpu_mmap_user_buffer(struct gk20a *g, struct vm_area_struct *vma)
{
	struct vgpu_fecs_trace *vcst = (struct vgpu_fecs_trace *)g->fecs_trace;
	unsigned long size = vgpu_ivm_get_size(vcst->cookie);
	unsigned long vsize = vma->vm_end - vma->vm_start;

	size = min(size, vsize);
	size = round_up(size, PAGE_SIZE);

	return remap_pfn_range(vma, vma->vm_start,
			vgpu_ivm_get_ipa(vcst->cookie) >> PAGE_SHIFT,
			size,
			vma->vm_page_prot);
}
