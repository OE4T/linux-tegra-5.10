/*
 * FIFO
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

#include <trace/events/gk20a.h>

#include <nvgpu/dma.h>
#include <nvgpu/fifo.h>
#include <nvgpu/runlist.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/channel.h>
#include <nvgpu/tsg.h>
#include <nvgpu/vm_area.h>

#include <gk20a/fifo_gk20a.h>

/* TODO: move to pbdma and userd when available */
static int nvgpu_pbdma_setup_sw(struct gk20a *g)
{
	struct fifo_gk20a *f = &g->fifo;
	int err;

	if (g->ops.fifo.init_pbdma_info != NULL) {
		err = g->ops.fifo.init_pbdma_info(f);
		if (err != 0) {
			nvgpu_err(g, "failed to init pbdma support");
			return err;
		}
	}

	return 0;
}

static void nvgpu_pbdma_cleanup_sw(struct gk20a *g)
{
	struct fifo_gk20a *f = &g->fifo;

	nvgpu_kfree(g, f->pbdma_map);
	f->pbdma_map = NULL;
}

static int nvgpu_userd_setup_sw(struct gk20a *g)
{
	struct fifo_gk20a *f = &g->fifo;
	int err;

	f->userd_entry_size = g->ops.fifo.userd_entry_size(g);

	err = gk20a_fifo_init_userd_slabs(g);
	if (err != 0) {
		nvgpu_err(g, "failed to init userd support");
		return err;
	}

	return 0;
}

static void nvgpu_userd_cleanup_sw(struct gk20a *g)
{
	struct fifo_gk20a *f = &g->fifo;

	gk20a_fifo_free_userd_slabs(g);
	if (f->userd_gpu_va != 0ULL) {
		(void) nvgpu_vm_area_free(g->mm.bar1.vm, f->userd_gpu_va);
		f->userd_gpu_va = 0ULL;
	}
}

void nvgpu_fifo_cleanup_sw_common(struct gk20a *g)
{
	struct fifo_gk20a *f = &g->fifo;

	nvgpu_log_fn(g, " ");

	nvgpu_userd_cleanup_sw(g);
	nvgpu_channel_cleanup_sw(g);
	nvgpu_tsg_cleanup_sw(g);
	nvgpu_runlist_cleanup_sw(g);
	nvgpu_engine_cleanup_sw(g);
	nvgpu_pbdma_cleanup_sw(g);

	f->deferred_reset_pending = false;
	nvgpu_mutex_destroy(&f->deferred_reset_mutex);
	nvgpu_mutex_destroy(&f->engines_reset_mutex);
	nvgpu_mutex_destroy(&f->intr.isr.mutex);
}

void nvgpu_fifo_cleanup_sw(struct gk20a *g)
{
	nvgpu_channel_worker_deinit(g);
	nvgpu_fifo_cleanup_sw_common(g);
}

static void nvgpu_fifo_remove_support(struct fifo_gk20a *f)
{
	struct gk20a *g = f->g;

	g->ops.fifo.cleanup_sw(g);
}

static int nvgpu_fifo_init_locks(struct gk20a *g, struct fifo_gk20a *f)
{
	int err;

	err = nvgpu_mutex_init(&f->intr.isr.mutex);
	if (err != 0) {
		goto destroy_0;
	}

	err = nvgpu_mutex_init(&f->engines_reset_mutex);
	if (err != 0) {
		goto destroy_1;
	}

	err = nvgpu_mutex_init(&f->deferred_reset_mutex);
	if (err != 0) {
		goto destroy_2;
	}

	return 0;

destroy_2:
	nvgpu_mutex_destroy(&f->engines_reset_mutex);

destroy_1:
	nvgpu_mutex_destroy(&f->intr.isr.mutex);

destroy_0:
	nvgpu_err(g, "failed to init mutex");
	return err;
}

int nvgpu_fifo_setup_sw_common(struct gk20a *g)
{
	struct fifo_gk20a *f = &g->fifo;
	int err = 0;

	nvgpu_log_fn(g, " ");

	f->g = g;

	err = nvgpu_fifo_init_locks(g, f);
	if (err != 0) {
		nvgpu_err(g, "failed to init mutexes");
	}

	err = nvgpu_channel_setup_sw(g);
	if (err != 0) {
		nvgpu_err(g, "failed to init channel support");
		goto clean_up;
	}

	err = nvgpu_tsg_setup_sw(g);
	if (err != 0) {
		nvgpu_err(g, "failed to init tsg support");
		goto clean_up_channel;
	}

	err = nvgpu_pbdma_setup_sw(g);
	if (err != 0) {
		nvgpu_err(g, "failed to init pbdma support");
		goto clean_up_tsg;
	}

	err = nvgpu_engine_setup_sw(g);
	if (err != 0) {
		nvgpu_err(g, "failed to init engine support");
		goto clean_up_pbdma;
	}

	err = nvgpu_runlist_setup_sw(g);
	if (err != 0) {
		nvgpu_err(g, "failed to init runlist support");
		goto clean_up_engine;
	}

	err = nvgpu_userd_setup_sw(g);
	if (err != 0) {
		nvgpu_err(g, "failed to init userd support");
		goto clean_up_runlist;
	}

	f->remove_support = nvgpu_fifo_remove_support;

	nvgpu_log_fn(g, "done");
	return 0;

clean_up_runlist:
	nvgpu_runlist_cleanup_sw(g);

clean_up_engine:
	nvgpu_engine_cleanup_sw(g);

clean_up_pbdma:
	nvgpu_pbdma_cleanup_sw(g);

clean_up_tsg:
	nvgpu_tsg_cleanup_sw(g);

clean_up_channel:
	nvgpu_channel_cleanup_sw(g);

clean_up:
	nvgpu_err(g, "init fifo support failed");
	return err;
}

int nvgpu_fifo_setup_sw(struct gk20a *g)
{
	struct fifo_gk20a *f = &g->fifo;
	int err = 0;
	u32 size;
	u32 num_pages;

	nvgpu_log_fn(g, " ");

	if (f->sw_ready) {
		nvgpu_log_fn(g, "skip init");
		return 0;
	}

	err = nvgpu_fifo_setup_sw_common(g);
	if (err != 0) {
		nvgpu_err(g, "fail: err: %d", err);
		return err;
	}

	size = f->num_channels * f->userd_entry_size;
	num_pages = DIV_ROUND_UP(size, PAGE_SIZE);
	err = nvgpu_vm_area_alloc(g->mm.bar1.vm,
			num_pages, PAGE_SIZE, &f->userd_gpu_va, 0);
	if (err != 0) {
		nvgpu_err(g, "userd gpu va allocation failed, err=%d", err);
		goto clean_up_sw_common;
	}

	err = nvgpu_channel_worker_init(g);
	if (err != 0) {
		nvgpu_err(g, "worker init fail, err=%d", err);
		goto clean_up_vm_area;
	}

	f->sw_ready = true;

	nvgpu_log_fn(g, "done");
	return 0;

clean_up_vm_area:
	(void) nvgpu_vm_area_free(g->mm.bar1.vm, f->userd_gpu_va);
	f->userd_gpu_va = 0ULL;

clean_up_sw_common:
	nvgpu_fifo_cleanup_sw_common(g);
	return err;
}

int nvgpu_fifo_init_support(struct gk20a *g)
{
	int err;

	err = g->ops.fifo.setup_sw(g);
	if (err != 0) {
		nvgpu_err(g, "fifo sw setup failed, err=%d", err);
		return err;
	}

	if (g->ops.fifo.init_fifo_setup_hw != NULL) {
		err = g->ops.fifo.init_fifo_setup_hw(g);
		if (err != 0) {
			nvgpu_err(g, "fifo hw setup failed, err=%d", err);
			return err;
		}
	}

	return err;
}
