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

#ifndef NVGPU_PBDMA_STATUS_H
#define NVGPU_PBDMA_STATUS_H

#include <nvgpu/types.h>

#define PBDMA_STATUS_ID_TYPE_CHID 0U
#define PBDMA_STATUS_ID_TYPE_TSGID 1U
#define PBDMA_STATUS_ID_TYPE_INVALID (~U32(0U))

#define PBDMA_STATUS_NEXT_ID_TYPE_CHID PBDMA_STATUS_ID_TYPE_CHID
#define PBDMA_STATUS_NEXT_ID_TYPE_TSGID PBDMA_STATUS_ID_TYPE_TSGID
#define PBDMA_STATUS_NEXT_ID_TYPE_INVALID PBDMA_STATUS_ID_TYPE_INVALID

#define PBDMA_STATUS_ID_INVALID (~U32(0U))
#define PBDMA_STATUS_NEXT_ID_INVALID PBDMA_STATUS_ID_INVALID

enum nvgpu_pbdma_status_chsw_status {
	NVGPU_PBDMA_CHSW_STATUS_INVALID,
	NVGPU_PBDMA_CHSW_STATUS_VALID,
	NVGPU_PBDMA_CHSW_STATUS_LOAD,
	NVGPU_PBDMA_CHSW_STATUS_SAVE,
	NVGPU_PBDMA_CHSW_STATUS_SWITCH,
};

struct nvgpu_pbdma_status_info {
	u32 pbdma_reg_status;
	u32 pbdma_channel_status;
	u32 id;
	u32 id_type;
	u32 next_id;
	u32 next_id_type;

	enum nvgpu_pbdma_status_chsw_status chsw_status;
};

bool nvgpu_pbdma_status_is_chsw_switch(struct nvgpu_pbdma_status_info
		*pbdma_status);
bool nvgpu_pbdma_status_is_chsw_load(struct nvgpu_pbdma_status_info
		*pbdma_status);
bool nvgpu_pbdma_status_is_chsw_save(struct nvgpu_pbdma_status_info
		*pbdma_status);
bool nvgpu_pbdma_status_is_chsw_valid(struct nvgpu_pbdma_status_info
		*pbdma_status);
bool nvgpu_pbdma_status_is_id_type_tsg(struct nvgpu_pbdma_status_info
		*pbdma_status);
bool nvgpu_pbdma_status_is_next_id_type_tsg(struct nvgpu_pbdma_status_info
		*pbdma_status);

#endif /* NVGPU_PBDMA_STATUS_H */
