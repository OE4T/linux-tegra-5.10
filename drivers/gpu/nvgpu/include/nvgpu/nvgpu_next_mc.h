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

#ifndef NVGPU_NEXT_MC_H
#define NVGPU_NEXT_MC_H

/**
 * @file
 *
 * Declare intr specific struct.
 */
#include <nvgpu/types.h>
#include <nvgpu/cic.h>

struct nvgpu_intr_unit_info {
	/**
	 * top bit 0 -> subtree 0 -> leaf0, leaf1 -> leaf 0, 1
	 * top bit 1 -> subtree 1 -> leaf0, leaf1 -> leaf 2, 3
	 * top bit 2 -> subtree 2 -> leaf0, leaf1 -> leaf 4, 5
	 * top bit 3 -> subtree 3 -> leaf0, leaf1 -> leaf 6, 7
	 */
	/**
	 * h/w defined vectorids for the s/w defined intr unit.
	 * Upto 32 vectorids (32 bits of a leaf register) are supported for
	 * the intr units that support multiple vector ids.
	 */
	u32 vectorid[NVGPU_CIC_INTR_VECTORID_SIZE_MAX];
	/** number of vectorid supported by the intr unit */
	u32 vectorid_size;
	u32 subtree;	/** subtree number corresponding to vectorid */
	u64 subtree_mask; /** leaf1_leaf0 value for the intr unit */
	/**
	 * This flag will be set to true after all the fields
	 * of nvgpu_intr_unit_info are configured.
	 */
	bool valid;
};

struct nvgpu_next_mc {
	/**
	 * intr info array indexed by s/w defined intr unit name
	 */
	struct nvgpu_intr_unit_info intr_unit_info[NVGPU_CIC_INTR_UNIT_MAX];
	/**
	 * Leaf mask per subtree. Subtree is a pair of leaf registers.
	 * Each subtree corresponds to a bit in intr_top register.
	 */
	u64 subtree_mask_restore[HOST2SOC_NUM_SUBTREE];
};
#endif /* NVGPU_NEXT_MC_H */
