/*
 * drivers/video/tegra/host/gk20a/fifo_gk20a.h
 *
 * GK20A graphics copy engine (gr host)
 *
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
#ifndef NVGPU_GK20A_CE2_GK20A_H
#define NVGPU_GK20A_CE2_GK20A_H

struct channel_gk20a;
struct tsg_gk20a;
struct gk20a;

void gk20a_ce2_isr(struct gk20a *g, u32 inst_id, u32 pri_base);
u32 gk20a_ce2_nonstall_isr(struct gk20a *g, u32 inst_id, u32 pri_base);

#define NVGPU_CE_INVAL_CTX_ID	~U32(0U)

/* CE command utility macros */
#define NVGPU_CE_LOWER_ADDRESS_OFFSET_MASK 0xffffffffU
#define NVGPU_CE_UPPER_ADDRESS_OFFSET_MASK 0xffU

#define NVGPU_CE_MAX_INFLIGHT_JOBS 32U
#define NVGPU_CE_MAX_COMMAND_BUFF_BYTES_PER_KICKOFF 256U

/* dma launch_flags */
	/* location */
#define NVGPU_CE_SRC_LOCATION_COHERENT_SYSMEM			BIT32(0)
#define NVGPU_CE_SRC_LOCATION_NONCOHERENT_SYSMEM		BIT32(1)
#define NVGPU_CE_SRC_LOCATION_LOCAL_FB				BIT32(2)
#define NVGPU_CE_DST_LOCATION_COHERENT_SYSMEM			BIT32(3)
#define NVGPU_CE_DST_LOCATION_NONCOHERENT_SYSMEM		BIT32(4)
#define NVGPU_CE_DST_LOCATION_LOCAL_FB				BIT32(5)
	/* memory layout */
#define NVGPU_CE_SRC_MEMORY_LAYOUT_PITCH			BIT32(6)
#define NVGPU_CE_SRC_MEMORY_LAYOUT_BLOCKLINEAR			BIT32(7)
#define NVGPU_CE_DST_MEMORY_LAYOUT_PITCH			BIT32(8)
#define NVGPU_CE_DST_MEMORY_LAYOUT_BLOCKLINEAR			BIT32(9)
	/* transfer type */
#define NVGPU_CE_DATA_TRANSFER_TYPE_PIPELINED			BIT32(10)
#define NVGPU_CE_DATA_TRANSFER_TYPE_NON_PIPELINED		BIT32(11)

/* CE operation mode */
#define NVGPU_CE_PHYS_MODE_TRANSFER	BIT32(0)
#define NVGPU_CE_MEMSET			BIT32(1)

/* CE app state machine flags */
enum {
	NVGPU_CE_ACTIVE                    = (1 << 0),
	NVGPU_CE_SUSPEND                   = (1 << 1),
};

/* gpu context state machine flags */
enum {
	NVGPU_CE_GPU_CTX_ALLOCATED         = (1 << 0),
	NVGPU_CE_GPU_CTX_DELETED           = (1 << 1),
};

/* global ce app db */
struct gk20a_ce_app {
	bool initialised;
	struct nvgpu_mutex app_mutex;
	int app_state;

	struct nvgpu_list_node allocated_contexts;
	u32 ctx_count;
	u32 next_ctx_id;
};

/* ce context db */
struct gk20a_gpu_ctx {
	struct gk20a *g;
	u32 ctx_id;
	struct nvgpu_mutex gpu_ctx_mutex;
	int gpu_ctx_state;

	/* tsg related data */
	struct tsg_gk20a *tsg;

	/* channel related data */
	struct channel_gk20a *ch;
	struct vm_gk20a *vm;

	/* cmd buf mem_desc */
	struct nvgpu_mem cmd_buf_mem;
	struct gk20a_fence *postfences[NVGPU_CE_MAX_INFLIGHT_JOBS];

	struct nvgpu_list_node list;

	u32 cmd_buf_read_queue_offset;
};

static inline struct gk20a_gpu_ctx *
gk20a_gpu_ctx_from_list(struct nvgpu_list_node *node)
{
	return (struct gk20a_gpu_ctx *)
		((uintptr_t)node - offsetof(struct gk20a_gpu_ctx, list));
};

/* global CE app related apis */
int gk20a_init_ce_support(struct gk20a *g);
void gk20a_ce_suspend(struct gk20a *g);
void gk20a_ce_destroy(struct gk20a *g);

/* CE app utility functions */
u32 gk20a_ce_create_context(struct gk20a *g,
		int runlist_id,
		int timeslice,
		int runlist_level);
int gk20a_ce_execute_ops(struct gk20a *g,
		u32 ce_ctx_id,
		u64 src_buf,
		u64 dst_buf,
		u64 size,
		unsigned int payload,
		u32 launch_flags,
		u32 request_operation,
		u32 submit_flags,
		struct gk20a_fence **gk20a_fence_out);
void gk20a_ce_delete_context_priv(struct gk20a *g,
		u32 ce_ctx_id);
void gk20a_ce_delete_context(struct gk20a *g,
		u32 ce_ctx_id);
u32 gk20a_ce_prepare_submit(u64 src_buf,
		u64 dst_buf,
		u64 size,
		u32 *cmd_buf_cpu_va,
		u32 max_cmd_buf_size,
		unsigned int payload,
		u32 launch_flags,
		u32 request_operation,
		u32 dma_copy_class);

#endif /*NVGPU_GK20A_CE2_GK20A_H*/
