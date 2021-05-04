/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_NEXT_CIC_H
#define NVGPU_NEXT_CIC_H

/**
 * @file
 *
 * Declare intr specific struct and defines.
 */
#include <nvgpu/types.h>
#include <nvgpu/static_analysis.h>

#define U32_BITS		32U
#define DIV_BY_U32_BITS(x)	((x) / U32_BITS)
#define MOD_BY_U32_BITS(x)	((x) % U32_BITS)

#define RESET_ID_TO_REG_IDX(x)		DIV_BY_U32_BITS((x))
#define RESET_ID_TO_REG_BIT(x)		MOD_BY_U32_BITS((x))
#define RESET_ID_TO_REG_MASK(x)		BIT32(RESET_ID_TO_REG_BIT((x)))

#define GPU_VECTOR_TO_LEAF_REG(i)	DIV_BY_U32_BITS((i))
#define GPU_VECTOR_TO_LEAF_BIT(i)	MOD_BY_U32_BITS((i))
#define GPU_VECTOR_TO_LEAF_MASK(i) (BIT32(GPU_VECTOR_TO_LEAF_BIT(i)))
#define GPU_VECTOR_TO_SUBTREE(i)	((GPU_VECTOR_TO_LEAF_REG(i)) / 2U)
#define GPU_VECTOR_TO_LEAF_SHIFT(i) \
	(nvgpu_safe_mult_u32(((GPU_VECTOR_TO_LEAF_REG(i)) % 2U), 32U))

#define HOST2SOC_0_SUBTREE		0U
#define HOST2SOC_1_SUBTREE		1U
#define HOST2SOC_2_SUBTREE		2U
#define HOST2SOC_3_SUBTREE		3U
#define HOST2SOC_NUM_SUBTREE		4U

#define HOST2SOC_SUBTREE_TO_TOP_IDX(i)	((i) / 32U)
#define HOST2SOC_SUBTREE_TO_TOP_BIT(i)	((i) % 32U)
#define HOST2SOC_SUBTREE_TO_LEAF0(i) \
		(nvgpu_safe_mult_u32((i), 2U))
#define HOST2SOC_SUBTREE_TO_LEAF1(i) \
		(nvgpu_safe_add_u32((nvgpu_safe_mult_u32((i), 2U)), 1U))

#define STALL_SUBTREE_TOP_IDX		0U
#define STALL_SUBTREE_TOP_BITS \
		((BIT32(HOST2SOC_SUBTREE_TO_TOP_BIT(HOST2SOC_1_SUBTREE))) | \
		 (BIT32(HOST2SOC_SUBTREE_TO_TOP_BIT(HOST2SOC_2_SUBTREE))) | \
		 (BIT32(HOST2SOC_SUBTREE_TO_TOP_BIT(HOST2SOC_3_SUBTREE))))

/**
 * These should not contradict NVGPU_CIC_INTR_UNIT_* defines.
 */
#define NVGPU_CIC_INTR_UNIT_MMU_FAULT_ECC_ERROR			10U
#define NVGPU_CIC_INTR_UNIT_MMU_NON_REPLAYABLE_FAULT_ERROR	11U
#define NVGPU_CIC_INTR_UNIT_MMU_REPLAYABLE_FAULT_ERROR		12U
#define NVGPU_CIC_INTR_UNIT_MMU_NON_REPLAYABLE_FAULT		13U
#define NVGPU_CIC_INTR_UNIT_MMU_REPLAYABLE_FAULT		14U
#define NVGPU_CIC_INTR_UNIT_MMU_INFO_FAULT			15U
#define NVGPU_CIC_INTR_UNIT_RUNLIST_TREE_0			16U
#define NVGPU_CIC_INTR_UNIT_RUNLIST_TREE_1			17U
#define NVGPU_CIC_INTR_UNIT_GR_STALL				18U
#define NVGPU_CIC_INTR_UNIT_CE_STALL				19U
#define NVGPU_CIC_INTR_UNIT_MAX					20U

#define NVGPU_CIC_INTR_VECTORID_SIZE_MAX				32U
#define NVGPU_CIC_INTR_VECTORID_SIZE_ONE				1U

#define RUNLIST_INTR_TREE_0					0U
#define RUNLIST_INTR_TREE_1					1U

void nvgpu_cic_intr_unit_vectorid_init(struct gk20a *g, u32 unit, u32 *vectorid,
		u32 num_entries);
bool nvgpu_cic_intr_is_unit_info_valid(struct gk20a *g, u32 unit);
bool nvgpu_cic_intr_get_unit_info(struct gk20a *g, u32 unit, u32 *subtree,
		u64 *subtree_mask);

#endif /* NVGPU_NEXT_CIC_H */
