/*
 * fifo common definitions (gr host)
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

#ifndef NVGPU_FIFO_COMMON_H
#define NVGPU_FIFO_COMMON_H

#define RC_TYPE_NO_RC			0U
#define RC_TYPE_MMU_FAULT		1U
#define RC_TYPE_PBDMA_FAULT		2U
#define RC_TYPE_GR_FAULT		3U
#define RC_TYPE_PREEMPT_TIMEOUT		4U
#define RC_TYPE_CTXSW_TIMEOUT		5U
#define RC_TYPE_RUNLIST_UPDATE_TIMEOUT	6U
#define RC_TYPE_FORCE_RESET		7U
#define RC_TYPE_SCHED_ERR		8U

struct gk20a;

struct nvgpu_channel_hw_state {
	bool enabled;
	bool next;
	bool ctx_reload;
	bool busy;
	bool pending_acquire;
	bool eng_faulted;
	const char *status_string;
};

int nvgpu_fifo_init_support(struct gk20a *g);
int nvgpu_fifo_setup_sw(struct gk20a *g);
int nvgpu_fifo_setup_sw_common(struct gk20a *g);
void nvgpu_fifo_cleanup_sw(struct gk20a *g);
void nvgpu_fifo_cleanup_sw_common(struct gk20a *g);

#endif /* NVGPU_FIFO_COMMON_H */
