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
#ifndef NVGPU_NEXT_GOPS_FUSE_H
#define NVGPU_NEXT_GOPS_FUSE_H

	/* Leave extra tab to fit into gops_fuse structure */

	void (*write_feature_override_ecc)(struct gk20a *g, u32 val);
	void (*write_feature_override_ecc_1)(struct gk20a *g, u32 val);
	void (*read_feature_override_ecc)(struct gk20a *g,
			struct nvgpu_fuse_feature_override_ecc *ecc_feature);
	u32 (*fuse_opt_sm_ttu_en)(struct gk20a *g);
	u32 (*opt_sec_source_isolation_en)(struct gk20a *g);

#endif /* NVGPU_NEXT_GOPS_FUSE_H */
