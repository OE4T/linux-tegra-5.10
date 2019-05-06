/*
 * Copyright (c) 2014-2019, NVIDIA CORPORATION.  All rights reserved.
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
#ifndef NVGPU_TSG_H
#define NVGPU_TSG_H

#include <nvgpu/lock.h>
#include <nvgpu/kref.h>
#include <nvgpu/rwsem.h>
#include <nvgpu/list.h>
#include <nvgpu/cond.h>

#define NVGPU_INVALID_TSG_ID (U32_MAX)

#define NVGPU_TSG_TIMESLICE_LOW_PRIORITY_US	1300U
#define NVGPU_TSG_TIMESLICE_MEDIUM_PRIORITY_US	2600U
#define NVGPU_TSG_TIMESLICE_HIGH_PRIORITY_US	5200U
#define NVGPU_TSG_TIMESLICE_MIN_US		1000U
#define NVGPU_TSG_TIMESLICE_MAX_US		50000U
#define NVGPU_TSG_TIMESLICE_DEFAULT_US		(128U << 3U)

struct gk20a;
struct nvgpu_channel;
struct nvgpu_gr_ctx;
struct nvgpu_channel_hw_state;

enum nvgpu_event_id_type;

struct nvgpu_tsg_sm_error_state {
	u32 hww_global_esr;
	u32 hww_warp_esr;
	u64 hww_warp_esr_pc;
	u32 hww_global_esr_report_mask;
	u32 hww_warp_esr_report_mask;
};

struct nvgpu_tsg {
	struct gk20a *g;

	struct vm_gk20a *vm;
	struct nvgpu_mem *eng_method_buffers;


	struct nvgpu_gr_ctx *gr_ctx;
	struct nvgpu_ref refcount;

	struct nvgpu_list_node ch_list;
	struct nvgpu_list_node event_id_list;
	struct nvgpu_rwsem ch_list_lock;
	struct nvgpu_mutex event_id_list_lock;
	u32 num_active_channels;

	unsigned int timeslice_us;
	unsigned int timeslice_timeout;
	unsigned int timeslice_scale;

	u32 interleave_level;
	u32 tsgid;

	u32 runlist_id;
	pid_t tgid;
	u32  num_active_tpcs;
	u8   tpc_pg_enabled;
	bool tpc_num_initialized;
	bool in_use;
	bool abortable;

	struct nvgpu_tsg_sm_error_state *sm_error_states;

#define NVGPU_SM_EXCEPTION_TYPE_MASK_NONE		(0x0U)
#define NVGPU_SM_EXCEPTION_TYPE_MASK_FATAL		(0x1U << 0)
	u32 sm_exception_mask_type;
	struct nvgpu_mutex sm_exception_mask_lock;
};

int nvgpu_tsg_open_common(struct gk20a *g, struct nvgpu_tsg *tsg, pid_t pid);
struct nvgpu_tsg *nvgpu_tsg_open(struct gk20a *g, pid_t pid);
void nvgpu_tsg_release_common(struct gk20a *g, struct nvgpu_tsg *tsg);
void nvgpu_tsg_release(struct nvgpu_ref *ref);

int gk20a_init_tsg_support(struct gk20a *g, u32 tsgid);
int nvgpu_tsg_setup_sw(struct gk20a *g);
void nvgpu_tsg_cleanup_sw(struct gk20a *g);

struct nvgpu_tsg *tsg_gk20a_from_ch(struct nvgpu_channel *ch);

void nvgpu_tsg_disable(struct nvgpu_tsg *tsg);
int nvgpu_tsg_bind_channel(struct nvgpu_tsg *tsg,
			struct nvgpu_channel *ch);
struct nvgpu_tsg *nvgpu_tsg_get_from_id(struct gk20a *g, u32 tsgid);
struct nvgpu_tsg *nvgpu_tsg_check_and_get_from_id(struct gk20a *g, u32 tsgid);
int nvgpu_tsg_unbind_channel(struct nvgpu_tsg *tsg, struct nvgpu_channel *ch);
int nvgpu_tsg_unbind_channel_common(struct nvgpu_tsg *tsg,
		struct nvgpu_channel *ch);
int nvgpu_tsg_unbind_channel_check_hw_state(struct nvgpu_tsg *tsg,
		struct nvgpu_channel *ch);
void nvgpu_tsg_unbind_channel_check_ctx_reload(struct nvgpu_tsg *tsg,
		struct nvgpu_channel *ch,
		struct nvgpu_channel_hw_state *hw_state);
int nvgpu_tsg_force_reset_ch(struct nvgpu_channel *ch,
				u32 err_code, bool verbose);
void nvgpu_tsg_set_ctx_mmu_error(struct gk20a *g,
		struct nvgpu_tsg *tsg);
bool nvgpu_tsg_mark_error(struct gk20a *g, struct nvgpu_tsg *tsg);

void nvgpu_tsg_post_event_id(struct nvgpu_tsg *tsg,
			     enum nvgpu_event_id_type event_id);
bool nvgpu_tsg_check_ctxsw_timeout(struct nvgpu_tsg *tsg,
		bool *debug_dump, u32 *ms);
int gk20a_tsg_set_runlist_interleave(struct nvgpu_tsg *tsg, u32 level);
int nvgpu_tsg_set_timeslice(struct nvgpu_tsg *tsg, u32 timeslice_us);
u32 nvgpu_tsg_get_timeslice(struct nvgpu_tsg *tsg);
u32 nvgpu_tsg_default_timeslice_us(struct gk20a *g);
void nvgpu_tsg_enable_sched(struct gk20a *g, struct nvgpu_tsg *tsg);
void nvgpu_tsg_disable_sched(struct gk20a *g, struct nvgpu_tsg *tsg);
int nvgpu_tsg_set_interleave(struct nvgpu_tsg *tsg, u32 level);
int gk20a_tsg_set_priority(struct gk20a *g, struct nvgpu_tsg *tsg,
				u32 priority);
int gk20a_tsg_alloc_sm_error_states_mem(struct gk20a *g,
					struct nvgpu_tsg *tsg,
					u32 num_sm);
void gk20a_tsg_update_sm_error_state_locked(struct nvgpu_tsg *tsg,
			u32 sm_id,
			struct nvgpu_tsg_sm_error_state *sm_error_state);
int gk20a_tsg_set_sm_exception_type_mask(struct nvgpu_channel *ch,
		u32 exception_mask);

struct gk20a_event_id_data {
	struct gk20a *g;

	int id; /* ch or tsg */
	int pid;
	u32 event_id;

	bool event_posted;

	struct nvgpu_cond event_id_wq;
	struct nvgpu_mutex lock;
	struct nvgpu_list_node event_id_node;
};

static inline struct gk20a_event_id_data *
gk20a_event_id_data_from_event_id_node(struct nvgpu_list_node *node)
{
	return (struct gk20a_event_id_data *)
		((uintptr_t)node - offsetof(struct gk20a_event_id_data, event_id_node));
};

void nvgpu_tsg_set_error_notifier(struct gk20a *g, struct nvgpu_tsg *tsg,
		u32 error_notifier);
bool nvgpu_tsg_ctxsw_timeout_debug_dump_state(struct nvgpu_tsg *tsg);
void nvgpu_tsg_set_ctxsw_timeout_accumulated_ms(struct nvgpu_tsg *tsg, u32 ms);
void nvgpu_tsg_abort(struct gk20a *g, struct nvgpu_tsg *tsg, bool preempt);
void nvgpu_tsg_reset_faulted_eng_pbdma(struct gk20a *g, struct nvgpu_tsg *tsg,
		bool eng, bool pbdma);
#endif /* NVGPU_TSG_H */
