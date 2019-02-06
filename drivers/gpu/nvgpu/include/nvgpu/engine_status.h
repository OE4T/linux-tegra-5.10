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

#ifndef NVGPU_ENGINE_STATUS_H
#define NVGPU_ENGINE_STATUS_H

#define ENGINE_STATUS_CTX_ID_TYPE_CHID 0U
#define ENGINE_STATUS_CTX_ID_TYPE_TSGID 1U
#define ENGINE_STATUS_CTX_ID_TYPE_INVALID (~U32(0U))

#define ENGINE_STATUS_CTX_NEXT_ID_TYPE_CHID ENGINE_STATUS_CTX_ID_TYPE_CHID
#define ENGINE_STATUS_CTX_NEXT_ID_TYPE_TSGID ENGINE_STATUS_CTX_ID_TYPE_TSGID
#define ENGINE_STATUS_CTX_NEXT_ID_TYPE_INVALID ENGINE_STATUS_CTX_ID_TYPE_INVALID

#define ENGINE_STATUS_CTX_ID_INVALID (~U32(0U))
#define ENGINE_STATUS_CTX_NEXT_ID_INVALID ENGINE_STATUS_CTX_ID_INVALID

enum nvgpu_engine_status_ctx_status {
	NVGPU_CTX_STATUS_INVALID,
	NVGPU_CTX_STATUS_VALID,
	NVGPU_CTX_STATUS_CTXSW_LOAD,
	NVGPU_CTX_STATUS_CTXSW_SAVE,
	NVGPU_CTX_STATUS_CTXSW_SWITCH,
};

struct nvgpu_engine_status_info {
	u32 reg_data;
	u32 ctx_id;
	u32 ctxsw_state;
	u32 ctx_id_type;
	u32 ctx_next_id;
	u32 ctx_next_id_type;

	bool is_faulted;
	bool is_busy;
	bool ctxsw_in_progress;
	bool in_reload_status;
	enum nvgpu_engine_status_ctx_status ctxsw_status;
};

bool nvgpu_engine_status_is_ctxsw_switch(struct nvgpu_engine_status_info
		*engine_status);
bool nvgpu_engine_status_is_ctxsw_load(struct nvgpu_engine_status_info
		*engine_status);
bool nvgpu_engine_status_is_ctxsw_save(struct nvgpu_engine_status_info
		*engine_status);
bool nvgpu_engine_status_is_ctxsw(struct nvgpu_engine_status_info
		*engine_status);
bool nvgpu_engine_status_is_ctxsw_invalid(struct nvgpu_engine_status_info
		*engine_status);
bool nvgpu_engine_status_is_ctxsw_valid(struct nvgpu_engine_status_info
		*engine_status);
bool nvgpu_engine_status_is_ctx_type_tsg(struct nvgpu_engine_status_info
		*engine_status);
bool nvgpu_engine_status_is_next_ctx_type_tsg(struct nvgpu_engine_status_info
		*engine_status);
void nvgpu_engine_status_get_ctx_id_type(struct nvgpu_engine_status_info
		*engine_status, u32 *ctx_id, u32 *ctx_type);
void nvgpu_engine_status_get_next_ctx_id_type(struct nvgpu_engine_status_info
		*engine_status, u32 *ctx_next_id,
		u32 *ctx_next_type);

#endif /* NVGPU_ENGINE_STATUS_H */
