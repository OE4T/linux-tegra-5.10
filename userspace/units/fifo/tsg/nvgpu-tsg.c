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

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <unit/io.h>
#include <unit/unit.h>

#include <nvgpu/channel.h>
#include <nvgpu/tsg.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/fifo/userd.h>
#include <nvgpu/fuse.h>
#include <nvgpu/dma.h>
#include <nvgpu/gr/ctx.h>

#include "common/gr/ctx_priv.h"

#include "hal/fifo/tsg_gk20a.h"

#include "hal/init/hal_gv11b.h"

#include "../nvgpu-fifo-gv11b.h"

#ifdef TSG_UNIT_DEBUG
#define unit_verbose	unit_info
#else
#define unit_verbose(unit, msg, ...) \
	do { \
		if (0) { \
			unit_info(unit, msg, ##__VA_ARGS__); \
		} \
	} while (0)
#endif

struct test_tsg_args {
	bool init_done;
	struct nvgpu_tsg *tsg;
	struct nvgpu_channel *ch;
};

struct test_tsg_args test_args = {
	.init_done = false,
};

/* structure set by stubs */
struct test_stub_rc {
	u32 branches;
	u32 chid;
	u32 tsgid;
	u32 count;
};

struct test_stub_rc stub_rc = {
	.branches = 0,
};

static inline void reset_stub_rc(void)
{
	memset(&stub_rc, 0, sizeof(stub_rc));
}

static int branches_strn(char *dst, size_t size,
		const char *labels[], u32 branches)
{
	int i;
	char *p = dst;
	int len;

	for (i = 0; i < 32; i++) {
		if (branches & BIT(i)) {
			len = snprintf(p, size, "%s ", labels[i]);
			size -= len;
			p += len;
		}
	}

	return (p - dst);
}

/* not MT-safe */
static char *branches_str(u32 branches, const char *labels[])
{
	static char buf[256];

	memset(buf, 0, sizeof(buf));
	branches_strn(buf, sizeof(buf), labels, branches);
	return buf;
}

/* test implementations of some hals */
static u32 stub_gv11b_gr_init_get_no_of_sm(struct gk20a *g)
{
	return 8;
}

#ifdef NVGPU_USERD
static int stub_userd_setup_sw(struct gk20a *g)
{
	struct nvgpu_fifo *f = &g->fifo;
	int err;

	f->userd_entry_size = g->ops.userd.entry_size(g);

	err = nvgpu_userd_init_slabs(g);
	if (err != 0) {
		nvgpu_err(g, "failed to init userd support");
		return err;
	}

	return 0;
}
#endif

static int test_fifo_init_support(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct test_tsg_args *t = args;
	int err;

	if (t->init_done) {
		unit_return_fail(m, "init already done");
	}

	err = test_fifo_setup_gv11b_reg_space(m, g);
	if (err != 0) {
		goto fail;
	}

	gv11b_init_hal(g);
	g->ops.fifo.init_fifo_setup_hw = NULL;
	g->ops.gr.init.get_no_of_sm = stub_gv11b_gr_init_get_no_of_sm;
	g->ops.tsg.init_eng_method_buffers = NULL;

#ifdef NVGPU_USERD
	/*
	 * Regular USERD init requires bar1.vm to be initialized
	 * Use a stub in unit tests, since it will be disabled in
	 * safety build anyway.
	 */
	g->ops.userd.setup_sw = stub_userd_setup_sw;
#endif

	err = nvgpu_fifo_init_support(g);
	if (err != 0) {
		test_fifo_cleanup_gv11b_reg_space(m, g);
		goto fail;
	}

	/* Do not allocate from vidmem */
	nvgpu_set_enabled(g, NVGPU_MM_UNIFIED_MEMORY, true);

	t->init_done = true;

	return UNIT_SUCCESS;

fail:
	return UNIT_FAIL;
}

#define F_TSG_OPEN_ACQUIRE_CH_FAIL	BIT(0)
#define F_TSG_OPEN_SM_FAIL		BIT(1)
#define F_TSG_OPEN_LAST			BIT(2)

static const char const *f_tsg_open[] = {
	"acquire_ch_fail",
	"sm_fail",
};

static u32 stub_gr_init_get_no_of_sm_0(struct gk20a *g)
{
	return 0;
}

static int test_tsg_open(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct nvgpu_fifo *f = &g->fifo;
	struct gpu_ops gops = g->ops;
	u32 num_channels = f->num_channels;
	struct nvgpu_tsg *tsg;
	u32 branches;
	int rc = UNIT_FAIL;
	u32 fail = F_TSG_OPEN_ACQUIRE_CH_FAIL |
			F_TSG_OPEN_SM_FAIL;

	for (branches = 0U; branches < F_TSG_OPEN_LAST; branches++) {

		reset_stub_rc();
		unit_verbose(m, "%s branches=%s\n", __func__,
			branches_str(branches, f_tsg_open));

		f->num_channels =
			branches & F_TSG_OPEN_ACQUIRE_CH_FAIL ?
			0U : num_channels;

		g->ops.gr.init.get_no_of_sm =
			branches & F_TSG_OPEN_SM_FAIL ?
			stub_gr_init_get_no_of_sm_0 : gops.gr.init.get_no_of_sm;

		tsg = nvgpu_tsg_open(g, getpid());

		if (branches & fail) {
			if (tsg != NULL) {
				goto done;
			}
		} else {
			if (tsg == NULL) {
				goto done;
			}
			nvgpu_ref_put(&tsg->refcount, nvgpu_tsg_release);
			tsg = NULL;
		}
	}
	rc = UNIT_SUCCESS;

done:
	if (rc != UNIT_SUCCESS) {
		unit_err(m, "%s branches=%s\n", __func__,
			branches_str(branches, f_tsg_open));
	}

	if (tsg != NULL) {
		nvgpu_ref_put(&tsg->refcount, nvgpu_tsg_release);
	}
	g->ops = gops;
	f->num_channels = num_channels;
	return rc;
}

#define F_TSG_RELEASE_GR_CTX		BIT(0)
#define F_TSG_RELEASE_MEM		BIT(1)
#define F_TSG_RELEASE_VM		BIT(2)
#define F_TSG_RELEASE_UNHOOK_EVENTS	BIT(3)
#define F_TSG_RELEASE_ENG_BUFS		BIT(4)
#define F_TSG_RELEASE_SM_ERR_STATES	BIT(5)
#define F_TSG_RELEASE_LAST		BIT(6)

static const char const *f_tsg_release[] = {
	"gr_ctx",
	"mem",
	"vm",
	"unhook_events",
	"eng_bufs",
	"sm_err_states"
};

static void stub_tsg_deinit_eng_method_buffers(struct gk20a *g,
		struct nvgpu_tsg *tsg)
{
	stub_rc.branches |= F_TSG_RELEASE_ENG_BUFS;
	stub_rc.tsgid = tsg->tsgid;
}

static void stub_gr_setup_free_gr_ctx(struct gk20a *g,
		struct vm_gk20a *vm, struct nvgpu_gr_ctx *gr_ctx)
{
	stub_rc.count++;
}


static int test_tsg_release(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct nvgpu_fifo *f = &g->fifo;
	struct gpu_ops gops = g->ops;
	struct nvgpu_tsg *tsg;
	struct nvgpu_list_node ev1, ev2;
	struct vm_gk20a vm;
	u32 branches;
	int rc = UNIT_FAIL;
	struct nvgpu_mem mem;
	u32 free_gr_ctx_mask =
		F_TSG_RELEASE_GR_CTX|F_TSG_RELEASE_MEM|F_TSG_RELEASE_VM;

	for (branches = 0U; branches < F_TSG_RELEASE_LAST; branches++) {

		reset_stub_rc();
		if (!(branches & F_TSG_RELEASE_GR_CTX) &&
				(branches & F_TSG_RELEASE_MEM)) {
			unit_verbose(m, "%s branches=%s (pruned)\n", __func__,
				branches_str(branches, f_tsg_release));
			continue;
		}
		unit_verbose(m, "%s branches=%s\n", __func__,
			branches_str(branches, f_tsg_release));

		tsg = nvgpu_tsg_open(g, getpid());
		if (tsg == NULL || tsg->gr_ctx == NULL ||
			tsg->gr_ctx->mem.aperture != APERTURE_INVALID) {
			goto done;
		}

		if (!(branches & F_TSG_RELEASE_GR_CTX)) {
			nvgpu_free_gr_ctx_struct(g, tsg->gr_ctx);
			tsg->gr_ctx = NULL;
		}

		if (branches & F_TSG_RELEASE_MEM) {
			nvgpu_dma_alloc(g, PAGE_SIZE, &mem);
			tsg->gr_ctx->mem = mem;
		}

		if (branches & F_TSG_RELEASE_VM) {
			tsg->vm = &vm;
			/* prevent nvgpu_vm_remove */
			nvgpu_ref_init(&vm.ref);
			nvgpu_ref_get(&vm.ref);
		} else {
			tsg->vm = NULL;
		}

		if ((branches & free_gr_ctx_mask) == free_gr_ctx_mask) {
			g->ops.gr.setup.free_gr_ctx =
				stub_gr_setup_free_gr_ctx;
		}

		if (branches & F_TSG_RELEASE_UNHOOK_EVENTS) {
			nvgpu_list_add(&ev1, &tsg->event_id_list);
			nvgpu_list_add(&ev2, &tsg->event_id_list);
		}

		g->ops.tsg.deinit_eng_method_buffers =
			branches & F_TSG_RELEASE_ENG_BUFS ?
			stub_tsg_deinit_eng_method_buffers : NULL;

		if (branches & F_TSG_RELEASE_SM_ERR_STATES) {
			if (tsg->sm_error_states == NULL) {
				goto done;
			}
		} else {
			nvgpu_kfree(g, tsg->sm_error_states);
			tsg->sm_error_states = NULL;
		}

		nvgpu_ref_put(&tsg->refcount, nvgpu_tsg_release);

		if ((branches & free_gr_ctx_mask) == free_gr_ctx_mask) {
			if (tsg->gr_ctx != NULL) {
				goto done;
			}
		} else {
			g->ops.gr.setup.free_gr_ctx =
				gops.gr.setup.free_gr_ctx;

			if (branches & F_TSG_RELEASE_MEM) {
				nvgpu_dma_free(g, &mem);
			}

			if (tsg->gr_ctx != NULL) {
				nvgpu_free_gr_ctx_struct(g, tsg->gr_ctx);
				tsg->gr_ctx = NULL;
			}

			if (stub_rc.count > 0) {
				goto done;
			}
		}

		if (branches & F_TSG_RELEASE_UNHOOK_EVENTS) {
			if (!nvgpu_list_empty(&tsg->event_id_list)) {
				goto done;
			}
		}

		if (branches & F_TSG_RELEASE_ENG_BUFS) {
			if (((stub_rc.branches &
					F_TSG_RELEASE_ENG_BUFS) == 0) ||
				stub_rc.tsgid != tsg->tsgid) {
				goto done;
			}
		}

		if (f->tsg[tsg->tsgid].in_use || tsg->gr_ctx != NULL ||
			tsg->vm != NULL || tsg->sm_error_states != NULL) {
			goto done;
		}
	}
	rc = UNIT_SUCCESS;

done:
	if (rc != UNIT_SUCCESS) {
		unit_err(m, "%s branches=%s\n", __func__,
			branches_str(branches, f_tsg_release));
	}

	g->ops = gops;
	return rc;
}

static int test_fifo_remove_support(struct unit_module *m,
		struct gk20a *g, void *args)
{
	struct test_tsg_args *t = args;

	if (!t->init_done) {
		unit_return_fail(m, "missing init support");
	}

	if (g->fifo.remove_support) {
		g->fifo.remove_support(&g->fifo);
	}

	return UNIT_SUCCESS;
}

struct unit_module_test nvgpu_tsg_tests[] = {
	UNIT_TEST(init_support, test_fifo_init_support, &test_args, 0),
	UNIT_TEST(open, test_tsg_open, &test_args, 0),
	UNIT_TEST(release, test_tsg_release, &test_args, 0),
	UNIT_TEST(remove_support, test_fifo_remove_support, &test_args, 0),
};

UNIT_MODULE(nvgpu_tsg, nvgpu_tsg_tests, UNIT_PRIO_NVGPU_TEST);
