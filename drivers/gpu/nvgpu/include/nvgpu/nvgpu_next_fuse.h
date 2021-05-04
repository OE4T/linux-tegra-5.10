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

#ifndef NVGPU_NEXT_FUSE_H
#define NVGPU_NEXT_FUSE_H

/**
 * @file
 *
 * Declare device info specific struct and defines.
 */
#include <nvgpu/types.h>

struct nvgpu_fuse_feature_override_ecc {
	/** overide_ecc register feature */
	/** sm_lrf enable */
	bool sm_lrf_enable;
	/** sm_lrf override */
	bool sm_lrf_override;
	/** sm_l1_data enable */
	bool sm_l1_data_enable;
	/** sm_l1_data overide */
	bool sm_l1_data_override;
	/** sm_l1_tag enable */
	bool sm_l1_tag_enable;
	/** sm_l1_tag overide */
	bool sm_l1_tag_override;
	/** ltc enable */
	bool ltc_enable;
	/** ltc overide */
	bool ltc_override;
	/** dram enable */
	bool dram_enable;
	/** dram overide */
	bool dram_override;
	/** sm_cbu enable */
	bool sm_cbu_enable;
	/** sm_cbu overide */
	bool sm_cbu_override;

	/** override_ecc_1 register feature */
	/** sm_l0_icache enable */
	bool sm_l0_icache_enable;
	/** sm_l0_icache overide */
	bool sm_l0_icache_override;
	/** sm_l1_icache enable */
	bool sm_l1_icache_enable;
	/** sm_l1_icache overide */
	bool sm_l1_icache_override;
};

#endif /* NVGPU_NEXT_FUSE_H */
