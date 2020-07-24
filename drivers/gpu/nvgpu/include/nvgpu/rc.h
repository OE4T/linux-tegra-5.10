/*
 * Copyright (c) 2011-2019, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_RC_H
#define NVGPU_RC_H

#include <nvgpu/types.h>

#define RC_TYPE_NO_RC			0U
#define RC_TYPE_MMU_FAULT		1U
#define RC_TYPE_PBDMA_FAULT		2U
#define RC_TYPE_GR_FAULT		3U
#define RC_TYPE_PREEMPT_TIMEOUT		4U
#define RC_TYPE_CTXSW_TIMEOUT		5U
#define RC_TYPE_RUNLIST_UPDATE_TIMEOUT	6U
#define RC_TYPE_FORCE_RESET		7U
#define RC_TYPE_SCHED_ERR		8U

#define INVAL_ID			(~U32(0U))

/*
 * Requires a string literal for the format - notice the string
 * concatination.
 */
#define dbg_rec(g, fmt, args...)					\
	nvgpu_log((g), gpu_dbg_rec, "REC | " fmt, ##args)


struct gk20a;
struct nvgpu_fifo;
struct nvgpu_tsg;
struct nvgpu_channel;
struct nvgpu_pbdma_status_info;
struct mmu_fault_info;

static inline const char *nvgpu_rc_type_to_str(unsigned int rc_type)
{
	const char *str = NULL;

	switch (rc_type) {
	case RC_TYPE_NO_RC:
		str = "None";
		break;
	case RC_TYPE_MMU_FAULT:
		str = "MMU fault";
		break;
	case RC_TYPE_PBDMA_FAULT:
		str = "PBDMA fault";
		break;
	case RC_TYPE_GR_FAULT:
		str = "GR fault";
		break;
	case RC_TYPE_PREEMPT_TIMEOUT:
		str = "Preemption timeout";
		break;
	case RC_TYPE_CTXSW_TIMEOUT:
		str = "CTXSW timeout";
		break;
	case RC_TYPE_RUNLIST_UPDATE_TIMEOUT:
		str = "RL Update timeout";
		break;
	case RC_TYPE_FORCE_RESET:
		str = "Force reset";
		break;
	case RC_TYPE_SCHED_ERR:
		str = "Sched err";
		break;
	default:
		str = "Unknown";
		break;
	}

	return str;
}

void nvgpu_rc_ctxsw_timeout(struct gk20a *g, u32 eng_bitmask,
				struct nvgpu_tsg *tsg, bool debug_dump);

void nvgpu_rc_pbdma_fault(struct gk20a *g, u32 pbdma_id, u32 error_notifier,
			struct nvgpu_pbdma_status_info *pbdma_status);

void nvgpu_rc_runlist_update(struct gk20a *g, u32 runlist_id);

void nvgpu_rc_preempt_timeout(struct gk20a *g, struct nvgpu_tsg *tsg);
void nvgpu_rc_gr_fault(struct gk20a *g,
			struct nvgpu_tsg *tsg, struct nvgpu_channel *ch);
void nvgpu_rc_sched_error_bad_tsg(struct gk20a *g);
void nvgpu_rc_tsg_and_related_engines(struct gk20a *g, struct nvgpu_tsg *tsg,
			 bool debug_dump, u32 rc_type);
void nvgpu_rc_mmu_fault(struct gk20a *g, u32 act_eng_bitmask,
			u32 id, unsigned int id_type, unsigned int rc_type,
			 struct mmu_fault_info *mmufault);

void nvgpu_rc_fifo_recover(struct gk20a *g,
			u32 eng_bitmask, /* if zero, will be queried from HW */
			u32 hw_id, /* if ~0, will be queried from HW */
			bool id_is_tsg, /* ignored if hw_id == ~0 */
			bool id_is_known, bool debug_dump, u32 rc_type);

#endif /* NVGPU_RC_H */
