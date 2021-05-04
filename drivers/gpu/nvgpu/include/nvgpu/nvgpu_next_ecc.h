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
#ifndef NVGPU_NEXT_ECC_H
#define NVGPU_NEXT_ECC_H

		/* Leave extra tab to fit into nvgpu_ecc.fb structure */
		struct nvgpu_ecc_stat *mmu_l2tlb_ecc_corrected_unique_err_count;
		/** hubmmu l2tlb uncorrected unique error count. */
		struct nvgpu_ecc_stat *mmu_l2tlb_ecc_uncorrected_unique_err_count;
		/** hubmmu hubtlb corrected unique error count. */
		struct nvgpu_ecc_stat *mmu_hubtlb_ecc_corrected_unique_err_count;
		/** hubmmu hubtlb uncorrected unique error count. */
		struct nvgpu_ecc_stat *mmu_hubtlb_ecc_uncorrected_unique_err_count;
		/** hubmmu fillunit corrected unique error count. */
		struct nvgpu_ecc_stat *mmu_fillunit_ecc_corrected_unique_err_count;
		/** hubmmu fillunit uncorrected unique error count. */
		struct nvgpu_ecc_stat *mmu_fillunit_ecc_uncorrected_unique_err_count;

#endif /* NVGPU_NEXT_ECC_H */
