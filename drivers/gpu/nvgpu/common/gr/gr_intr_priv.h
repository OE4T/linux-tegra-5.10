/*
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_GR_INTR_PRIV_H
#define NVGPU_GR_INTR_PRIV_H

#include <nvgpu/types.h>
#include <nvgpu/lock.h>

struct nvgpu_channel;

struct nvgpu_gr_intr_info {
	u32 notify;
	u32 semaphore;
	u32 illegal_notify;
	u32 illegal_method;
	u32 illegal_class;
	u32 fecs_error;
	u32 class_error;
	u32 fw_method;
	u32 exception;
};

struct nvgpu_gr_tpc_exception {
	bool tex_exception;
	bool sm_exception;
	bool mpc_exception;
};

struct nvgpu_gr_isr_data {
	u32 addr;
	u32 data_lo;
	u32 data_hi;
	u32 curr_ctx;
	struct nvgpu_channel *ch;
	u32 offset;
	u32 sub_chan;
	u32 class_num;
};

struct gr_channel_map_tlb_entry {
	u32 curr_ctx;
	u32 chid;
	u32 tsgid;
};

struct nvgpu_gr_intr {

#define GR_CHANNEL_MAP_TLB_SIZE		2U /* must of power of 2 */
	struct gr_channel_map_tlb_entry chid_tlb[GR_CHANNEL_MAP_TLB_SIZE];
	u32 channel_tlb_flush_index;
	struct nvgpu_spinlock ch_tlb_lock;

};

#endif /* NVGPU_GR_INTR_PRIV_H */

