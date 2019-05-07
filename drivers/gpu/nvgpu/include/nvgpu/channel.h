/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_CHANNEL_H
#define NVGPU_CHANNEL_H

#include <nvgpu/list.h>
#include <nvgpu/lock.h>
#include <nvgpu/timers.h>
#include <nvgpu/cond.h>
#include <nvgpu/atomic.h>
#include <nvgpu/nvgpu_mem.h>
#include <nvgpu/allocator.h>
#include <nvgpu/debug.h>

#define NVGPU_INVALID_CHANNEL_ID	(~U32(0U))

struct gk20a;
struct dbg_session_gk20a;
struct nvgpu_fence_type;
struct nvgpu_profile;
struct nvgpu_channel_sync;
struct nvgpu_gpfifo_userdata;
struct nvgpu_gr_subctx;
struct nvgpu_gr_ctx;
struct gk20a_debug_output;

/* Flags to be passed to nvgpu_channel_setup_bind() */
#define NVGPU_SETUP_BIND_FLAGS_SUPPORT_VPR		BIT32(0)
#define NVGPU_SETUP_BIND_FLAGS_SUPPORT_DETERMINISTIC	BIT32(1)
#define NVGPU_SETUP_BIND_FLAGS_REPLAYABLE_FAULTS_ENABLE	BIT32(2)
#define NVGPU_SETUP_BIND_FLAGS_USERMODE_SUPPORT		BIT32(3)

/* Flags to be passed to nvgpu_submit_channel_gpfifo() */
#define NVGPU_SUBMIT_FLAGS_FENCE_WAIT			BIT32(0)
#define NVGPU_SUBMIT_FLAGS_FENCE_GET			BIT32(1)
#define NVGPU_SUBMIT_FLAGS_HW_FORMAT			BIT32(2)
#define NVGPU_SUBMIT_FLAGS_SYNC_FENCE			BIT32(3)
#define NVGPU_SUBMIT_FLAGS_SUPPRESS_WFI			BIT32(4)
#define NVGPU_SUBMIT_FLAGS_SKIP_BUFFER_REFCOUNTING	BIT32(5)

/*
 * The binary format of 'struct nvgpu_channel_fence' introduced here
 * should match that of 'struct nvgpu_fence' defined in uapi header, since
 * this struct is intended to be a mirror copy of the uapi struct. This is
 * not a hard requirement though because of nvgpu_get_fence_args conversion
 * function.
 */
struct nvgpu_channel_fence {
	u32 id;
	u32 value;
};

/*
 * The binary format of 'struct nvgpu_gpfifo_entry' introduced here
 * should match that of 'struct nvgpu_gpfifo' defined in uapi header, since
 * this struct is intended to be a mirror copy of the uapi struct. This is
 * a rigid requirement because there's no conversion function and there are
 * memcpy's present between the user gpfifo (of type nvgpu_gpfifo) and the
 * kern gpfifo (of type nvgpu_gpfifo_entry).
 */
struct nvgpu_gpfifo_entry {
	u32 entry0;
	u32 entry1;
};

struct gpfifo_desc {
	struct nvgpu_mem mem;
	u32 entry_num;

	u32 get;
	u32 put;

	bool wrap;

	/* if gpfifo lives in vidmem or is forced to go via PRAMIN, first copy
	 * from userspace to pipe and then from pipe to gpu buffer */
	void *pipe;
};

struct nvgpu_channel_hw_state {
	bool enabled;
	bool next;
	bool ctx_reload;
	bool busy;
	bool pending_acquire;
	bool eng_faulted;
	const char *status_string;
};

struct nvgpu_channel_dump_info {
	u32 chid;
	u32 tsgid;
	int pid;
	int refs;
	bool deterministic;
	struct nvgpu_channel_hw_state hw_state;
	struct {
		u64 pb_top_level_get;
		u64 pb_put;
		u64 pb_get;
		u64 pb_fetch;
		u32 pb_header;
		u32 pb_count;
		u64 sem_addr;
		u64 sem_payload;
		u32 sem_execute;
		u32 syncpointa;
		u32 syncpointb;
		u32 semaphorea;
		u32 semaphoreb;
		u32 semaphorec;
		u32 semaphored;
	} inst;
	struct {
		u32 value;
		u32 next;
		u64 addr;
	} sema;
};

struct nvgpu_setup_bind_args {
	u32 num_gpfifo_entries;
	u32 num_inflight_jobs;
	u32 userd_dmabuf_fd;
	u64 userd_dmabuf_offset;
	u32 gpfifo_dmabuf_fd;
	u64 gpfifo_dmabuf_offset;
	u32 work_submit_token;
	u32 flags;
};

struct notification {
	struct {
		u32 nanoseconds[2];
	} timestamp;
	u32 info32;
	u16 info16;
	u16 status;
};

struct priv_cmd_queue {
	struct nvgpu_mem mem;
	u32 size;	/* num of entries in words */
	u32 put;	/* put for priv cmd queue */
	u32 get;	/* get for priv cmd queue */
};

struct priv_cmd_entry {
	bool valid;
	struct nvgpu_mem *mem;
	u32 off;	/* offset in mem, in u32 entries */
	u64 gva;
	u32 get;	/* start of entry in queue */
	u32 size;	/* in words */
};

struct nvgpu_channel_job {
	struct nvgpu_mapped_buf **mapped_buffers;
	int num_mapped_buffers;
	struct nvgpu_fence_type *post_fence;
	struct priv_cmd_entry *wait_cmd;
	struct priv_cmd_entry *incr_cmd;
	struct nvgpu_list_node list;
};

static inline struct nvgpu_channel_job *
channel_gk20a_job_from_list(struct nvgpu_list_node *node)
{
	return (struct nvgpu_channel_job *)
	((uintptr_t)node - offsetof(struct nvgpu_channel_job, list));
};

struct nvgpu_channel_joblist {
	struct {
		bool enabled;
		unsigned int length;
		unsigned int put;
		unsigned int get;
		struct nvgpu_channel_job *jobs;
		struct nvgpu_mutex read_lock;
	} pre_alloc;

	struct {
		struct nvgpu_list_node jobs;
		struct nvgpu_spinlock lock;
	} dynamic;

	/*
	 * Synchronize abort cleanup (when closing a channel) and job cleanup
	 * (asynchronously from worker) - protect from concurrent access when
	 * job resources are being freed.
	 */
	struct nvgpu_mutex cleanup_lock;
};

struct nvgpu_channel_wdt {
	/* lock protects the running timer state */
	struct nvgpu_spinlock lock;
	struct nvgpu_timeout timer;
	bool running;
	u32 gp_get;
	u64 pb_get;

	/* lock not needed */
	u32 limit_ms;
	bool enabled;
	bool debug_dump;
};

/*
 * Track refcount actions, saving their stack traces. This number specifies how
 * many most recent actions are stored in a buffer. Set to 0 to disable. 128
 * should be enough to track moderately hard problems from the start.
 */
#define GK20A_CHANNEL_REFCOUNT_TRACKING 0
/* Stack depth for the saved actions. */
#define GK20A_CHANNEL_REFCOUNT_TRACKING_STACKLEN 8

/*
 * Because the puts and gets are not linked together explicitly (although they
 * should always come in pairs), it's not possible to tell which ref holder to
 * delete from the list when doing a put. So, just store some number of most
 * recent gets and puts in a ring buffer, to obtain a history.
 *
 * These are zeroed when a channel is closed, so a new one starts fresh.
 */

enum channel_gk20a_ref_action_type {
	channel_gk20a_ref_action_get,
	channel_gk20a_ref_action_put
};

#if GK20A_CHANNEL_REFCOUNT_TRACKING

#include <linux/stacktrace.h>

struct nvgpu_channel_ref_action {
	enum channel_gk20a_ref_action_type type;
	s64 timestamp_ms;
	/*
	 * Many of these traces will be similar. Simpler to just capture
	 * duplicates than to have a separate database for the entries.
	 */
	struct stack_trace trace;
	unsigned long trace_entries[GK20A_CHANNEL_REFCOUNT_TRACKING_STACKLEN];
};
#endif

/* this is the priv element of struct nvhost_channel */
struct nvgpu_channel {
	struct gk20a *g; /* set only when channel is active */

	struct nvgpu_list_node free_chs;

	struct nvgpu_spinlock ref_obtain_lock;
	nvgpu_atomic_t ref_count;
	struct nvgpu_cond ref_count_dec_wq;
#if GK20A_CHANNEL_REFCOUNT_TRACKING
	/*
	 * Ring buffer for most recent refcount gets and puts. Protected by
	 * ref_actions_lock when getting or putting refs (i.e., adding
	 * entries), and when reading entries.
	 */
	struct nvgpu_channel_ref_action ref_actions[
		GK20A_CHANNEL_REFCOUNT_TRACKING];
	size_t ref_actions_put; /* index of next write */
	struct nvgpu_spinlock ref_actions_lock;
#endif

	struct nvgpu_hw_semaphore *hw_sema;

	nvgpu_atomic_t bound;

	u32 chid;
	u32 tsgid;
	pid_t pid;
	pid_t tgid;
	struct nvgpu_mutex ioctl_lock;

	struct nvgpu_list_node ch_entry; /* channel's entry in TSG */

	struct nvgpu_channel_joblist joblist;
	struct nvgpu_allocator fence_allocator;

	struct vm_gk20a *vm;

	struct gpfifo_desc gpfifo;

	struct nvgpu_mem usermode_userd; /* Used for Usermode Submission */
	struct nvgpu_mem usermode_gpfifo;
	struct nvgpu_mem inst_block;

	u64 userd_iova;

	struct nvgpu_mem *userd_mem;	/* kernel mode userd */
	u32 userd_offset;		/* in bytes from start of userd_mem */

	struct priv_cmd_queue priv_cmd_q;

	struct nvgpu_cond notifier_wq;
	struct nvgpu_cond semaphore_wq;

	/* kernel watchdog to kill stuck jobs */
	struct nvgpu_channel_wdt wdt;

	/* for job cleanup handling in the background worker */
	struct nvgpu_list_node worker_item;

#if defined(CONFIG_GK20A_CYCLE_STATS)
	struct {
		void *cyclestate_buffer;
		u32 cyclestate_buffer_size;
		struct nvgpu_mutex cyclestate_buffer_mutex;
	} cyclestate;

	struct nvgpu_mutex cs_client_mutex;
	struct gk20a_cs_snapshot_client *cs_client;
#endif
	struct nvgpu_mutex dbg_s_lock;
	struct nvgpu_list_node dbg_s_list;

	struct nvgpu_mutex sync_lock;
	struct nvgpu_channel_sync *sync;
	struct nvgpu_channel_sync *user_sync;

#ifdef CONFIG_TEGRA_GR_VIRTUALIZATION
	u64 virt_ctx;
#endif

	struct nvgpu_gr_subctx *subctx;

	struct nvgpu_spinlock unserviceable_lock;
	bool unserviceable;

	/* Any operating system specific data. */
	void *os_priv;

	u32 obj_class;	/* we support only one obj per channel */

	u32 ctxsw_timeout_accumulated_ms;
	u32 ctxsw_timeout_gpfifo_get;
	u32 ctxsw_timeout_max_ms;
	bool ctxsw_timeout_debug_dump;

	u32 subctx_id;
	u32 runqueue_sel;

	u32 runlist_id;

	bool mmu_nack_handled;
	bool referenceable;
	bool vpr;
	bool deterministic;
	/* deterministic, but explicitly idle and submits disallowed */
	bool deterministic_railgate_allowed;
	bool cde;
	bool usermode_submit_enabled;
	bool has_os_fence_framework_support;

	bool is_privileged_channel;
};

static inline struct nvgpu_channel *
channel_gk20a_from_free_chs(struct nvgpu_list_node *node)
{
	return (struct nvgpu_channel *)
		   ((uintptr_t)node - offsetof(struct nvgpu_channel, free_chs));
};

static inline struct nvgpu_channel *
channel_gk20a_from_ch_entry(struct nvgpu_list_node *node)
{
	return (struct nvgpu_channel *)
	   ((uintptr_t)node - offsetof(struct nvgpu_channel, ch_entry));
};

static inline struct nvgpu_channel *
channel_gk20a_from_worker_item(struct nvgpu_list_node *node)
{
	return (struct nvgpu_channel *)
	   ((uintptr_t)node - offsetof(struct nvgpu_channel, worker_item));
};

static inline bool gk20a_channel_as_bound(struct nvgpu_channel *ch)
{
	return (ch->vm != NULL);
}
int channel_gk20a_commit_va(struct nvgpu_channel *c);
int gk20a_init_channel_support(struct gk20a *g, u32 chid);
int nvgpu_channel_setup_sw(struct gk20a *g);
void nvgpu_channel_cleanup_sw(struct gk20a *g);

/* must be inside gk20a_busy()..gk20a_idle() */
void gk20a_channel_close(struct nvgpu_channel *ch);
void nvgpu_channel_kill(struct nvgpu_channel *ch);

void nvgpu_channel_set_ctx_mmu_error(struct gk20a *g,
		struct nvgpu_channel *ch);
bool nvgpu_channel_mark_error(struct gk20a *g, struct nvgpu_channel *ch);

bool nvgpu_channel_update_and_check_ctxsw_timeout(struct nvgpu_channel *ch,
		u32 timeout_delta_ms, bool *progress);

void nvgpu_channel_recover(struct gk20a *g, struct nvgpu_channel *ch,
	bool verbose, u32 rc_type);

void gk20a_channel_abort(struct nvgpu_channel *ch, bool channel_preempt);
void nvgpu_channel_abort_clean_up(struct nvgpu_channel *ch);
void gk20a_channel_semaphore_wakeup(struct gk20a *g, bool post_events);
int gk20a_channel_alloc_priv_cmdbuf(struct nvgpu_channel *c, u32 orig_size,
			     struct priv_cmd_entry *e);
int gk20a_free_priv_cmdbuf(struct nvgpu_channel *c, struct priv_cmd_entry *e);

int gk20a_enable_channel_tsg(struct gk20a *g, struct nvgpu_channel *ch);
int gk20a_disable_channel_tsg(struct gk20a *g, struct nvgpu_channel *ch);

int nvgpu_channel_suspend_all_serviceable_ch(struct gk20a *g);
void nvgpu_channel_resume_all_serviceable_ch(struct gk20a *g);

void gk20a_channel_deterministic_idle(struct gk20a *g);
void gk20a_channel_deterministic_unidle(struct gk20a *g);

int nvgpu_channel_worker_init(struct gk20a *g);
void nvgpu_channel_worker_deinit(struct gk20a *g);

struct nvgpu_channel *gk20a_get_channel_from_file(int fd);
void gk20a_channel_update(struct nvgpu_channel *c);

/* returns ch if reference was obtained */
struct nvgpu_channel *__must_check _gk20a_channel_get(struct nvgpu_channel *ch,
						      const char *caller);
#define gk20a_channel_get(ch) _gk20a_channel_get(ch, __func__)


void _gk20a_channel_put(struct nvgpu_channel *ch, const char *caller);
#define gk20a_channel_put(ch) _gk20a_channel_put(ch, __func__)

/* returns NULL if could not take a ref to the channel */
struct nvgpu_channel *__must_check nvgpu_channel_from_id__func(
		struct gk20a *g, u32 chid, const char *caller);
#define nvgpu_channel_from_id(g, chid)	\
	nvgpu_channel_from_id__func(g, chid, __func__)

int gk20a_wait_channel_idle(struct nvgpu_channel *ch);

/* runlist_id -1 is synonym for NVGPU_ENGINE_GR runlist id */
struct nvgpu_channel *gk20a_open_new_channel(struct gk20a *g,
		u32 runlist_id,
		bool is_privileged_channel,
		pid_t pid, pid_t tid);

int nvgpu_channel_setup_bind(struct nvgpu_channel *c,
		struct nvgpu_setup_bind_args *args);

void nvgpu_channel_wdt_restart_all_channels(struct gk20a *g);

bool channel_gk20a_is_prealloc_enabled(struct nvgpu_channel *c);
void channel_gk20a_joblist_lock(struct nvgpu_channel *c);
void channel_gk20a_joblist_unlock(struct nvgpu_channel *c);
bool channel_gk20a_joblist_is_empty(struct nvgpu_channel *c);

int channel_gk20a_update_runlist(struct nvgpu_channel *c, bool add);
void gk20a_channel_get_timescale_from_timeslice(struct gk20a *g,
		unsigned int timeslice_period,
		unsigned int *__timeslice_timeout, unsigned int *__timeslice_scale);

void gk20a_wait_until_counter_is_N(
	struct nvgpu_channel *ch, nvgpu_atomic_t *counter, int wait_value,
	struct nvgpu_cond *c, const char *caller, const char *counter_name);
int channel_gk20a_alloc_job(struct nvgpu_channel *c,
		struct nvgpu_channel_job **job_out);
void channel_gk20a_free_job(struct nvgpu_channel *c,
		struct nvgpu_channel_job *job);
u32 nvgpu_get_gp_free_count(struct nvgpu_channel *c);
u32 nvgpu_gp_free_count(struct nvgpu_channel *c);
int gk20a_channel_add_job(struct nvgpu_channel *c,
				 struct nvgpu_channel_job *job,
				 bool skip_buffer_refcounting);
void free_priv_cmdbuf(struct nvgpu_channel *c,
			     struct priv_cmd_entry *e);
void gk20a_channel_clean_up_jobs(struct nvgpu_channel *c,
					bool clean_all);

void gk20a_channel_free_usermode_buffers(struct nvgpu_channel *c);
u32 nvgpu_get_gpfifo_entry_size(void);

int nvgpu_submit_channel_gpfifo_user(struct nvgpu_channel *c,
				struct nvgpu_gpfifo_userdata userdata,
				u32 num_entries,
				u32 flags,
				struct nvgpu_channel_fence *fence,
				struct nvgpu_fence_type **fence_out,
				struct nvgpu_profile *profile);

int nvgpu_submit_channel_gpfifo_kernel(struct nvgpu_channel *c,
				struct nvgpu_gpfifo_entry *gpfifo,
				u32 num_entries,
				u32 flags,
				struct nvgpu_channel_fence *fence,
				struct nvgpu_fence_type **fence_out);

#ifdef CONFIG_DEBUG_FS
void trace_write_pushbuffers(struct nvgpu_channel *c, u32 count);
#else
static inline void trace_write_pushbuffers(struct nvgpu_channel *c, u32 count)
{
}
#endif

void gk20a_channel_set_unserviceable(struct nvgpu_channel *ch);
bool gk20a_channel_check_unserviceable(struct nvgpu_channel *ch);

static inline u64 gk20a_channel_userd_addr(struct nvgpu_channel *c)
{
	return nvgpu_mem_get_addr(c->g, c->userd_mem) + c->userd_offset;
}

static inline u64 gk20a_channel_userd_gpu_va(struct nvgpu_channel *c)
{
	struct nvgpu_mem *mem = c->userd_mem;
	return (mem->gpu_va != 0ULL) ? mem->gpu_va + c->userd_offset : 0ULL;
}

int nvgpu_channel_alloc_inst(struct gk20a *g, struct nvgpu_channel *ch);
void nvgpu_channel_free_inst(struct gk20a *g, struct nvgpu_channel *ch);
void nvgpu_channel_set_error_notifier(struct gk20a *g, struct nvgpu_channel *ch,
			u32 error_notifier);
int nvgpu_channel_set_syncpt(struct nvgpu_channel *ch);
struct nvgpu_channel *nvgpu_channel_refch_from_inst_ptr(struct gk20a *g,
			u64 inst_ptr);
void nvgpu_channel_debug_dump_all(struct gk20a *g,
		 struct gk20a_debug_output *o);
int nvgpu_channel_deferred_reset_engines(struct gk20a *g,
		struct nvgpu_channel *ch);

#endif
