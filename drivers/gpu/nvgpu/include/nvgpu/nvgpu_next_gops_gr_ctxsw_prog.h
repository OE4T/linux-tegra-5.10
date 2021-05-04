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
#ifndef NVGPU_NEXT_GOPS_GR_CTXSW_PROG_H
#define NVGPU_NEXT_GOPS_GR_CTXSW_PROG_H

#ifdef CONFIG_NVGPU_DEBUGGER
	u32 (*hw_get_main_header_size)(void);
	u32 (*hw_get_gpccs_header_stride)(void);
	u32 (*get_compute_sysreglist_offset)(u32 *fecs_hdr);
	u32 (*get_gfx_sysreglist_offset)(u32 *fecs_hdr);
	u32 (*get_ltsreglist_offset)(u32 *fecs_hdr);
	u32 (*get_compute_gpcreglist_offset)(u32 *gpccs_hdr);
	u32 (*get_gfx_gpcreglist_offset)(u32 *gpccs_hdr);
	u32 (*get_compute_tpcreglist_offset)(u32 *gpccs_hdr, u32 tpc_num);
	u32 (*get_gfx_tpcreglist_offset)(u32 *gpccs_hdr, u32 tpc_num);
	u32 (*get_compute_ppcreglist_offset)(u32 *gpccs_hdr);
	u32 (*get_gfx_ppcreglist_offset)(u32 *gpccs_hdr);
	u32 (*get_compute_etpcreglist_offset)(u32 *gpccs_hdr);
	u32 (*get_gfx_etpcreglist_offset)(u32 *gpccs_hdr);
#endif

#endif /* NVGPU_NEXT_GOPS_GR_CTXSW_PROG_H */
