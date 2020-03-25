/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */
#ifndef NVGPU_GV11B_IOSPACE_H
#define NVGPU_GV11B_IOSPACE_H

struct mock_iospace {
	const uint32_t *data;
	size_t size;
};

enum gv11b_reg_idx {
	gv11b_gr_reg_idx = 0,
	gv11b_fuse_reg_idx,
	gv11b_master_reg_idx,
	gv11b_top_reg_idx,
	gv11b_fifo_reg_idx,
	gv11b_pri_reg_idx,
	gv11b_pbdma_reg_idx,
	gv11b_ccsr_reg_idx,
	gv11b_last_reg_idx,
};

int gv11b_get_mock_iospace(int reg_idx, struct mock_iospace *iospace);

#endif
