/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_MMU_FAULT_H
#define NVGPU_MMU_FAULT_H

#include <nvgpu/types.h>

#define NVGPU_MMU_FAULT_NONREPLAY_INDX	0U
#define NVGPU_MMU_FAULT_REPLAY_INDX		1U

/* replay and nonreplay faults */
#define NVGPU_MMU_FAULT_TYPE_NUM			2U

#define NVGPU_MMU_FAULT_NONREPLAY_REG_INDX		0U
#define NVGPU_MMU_FAULT_REPLAY_REG_INDX			1U
#define NVGPU_MMU_FAULT_BUF_DISABLED			0U
#define NVGPU_MMU_FAULT_BUF_ENABLED			1U

struct nvgpu_channel;

struct mmu_fault_info {
	u64	inst_ptr;
	u32	inst_aperture;
	u64	fault_addr;
	u32	fault_addr_aperture;
	u32	timestamp_lo;
	u32	timestamp_hi;
	u32	mmu_engine_id;
	u32	gpc_id;
	u32	client_type;
	u32	client_id;
	u32	fault_type;
	u32	access_type;
	u32	protected_mode;
	bool	replayable_fault;
	u32	replay_fault_en;
	bool	valid;
	u32	faulted_pbdma;
	u32	faulted_engine;
	u32	faulted_subid;
	u32	chid;
	struct nvgpu_channel *refch;
	const char *client_type_desc;
	const char *fault_type_desc;
	const char *client_id_desc;
};

#endif /* NVGPU_MMU_FAULT_H */
