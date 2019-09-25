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


#include <stdlib.h>
#include <unistd.h>

#include <unit/unit.h>
#include <unit/io.h>

#include <nvgpu/posix/io.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/gr/gr.h>
#include <nvgpu/channel.h>
#include <nvgpu/runlist.h>
#include <nvgpu/dma.h>

#include <nvgpu/hw/gv11b/hw_gr_gv11b.h>

#include "hal/gr/intr/gr_intr_gv11b.h"
#include "hal/gr/intr/gr_intr_gp10b.h"

#include "common/gr/gr_priv.h"

#include "../nvgpu-gr.h"

struct test_gr_intr_sw_mthd_exceptions {
	int trapped_addr;
	int data[2];
};

static int test_gr_intr_setup(struct unit_module *m,
		struct gk20a *g, void *args)
{
	int err = 0;

	/* Allocate and Initialize GR */
	err = test_gr_init_setup(m, g, args);
	if (err != 0) {
		unit_return_fail(m, "gr init setup failed\n");
	}

	err = test_gr_init_prepare(m, g, args);
	if (err != 0) {
		unit_return_fail(m, "gr init prepare failed\n");
	}

	err = test_gr_init_support(m, g, args);
	if (err != 0) {
		unit_return_fail(m, "gr init support failed\n");
	}

	return UNIT_SUCCESS;
}

static int test_gr_intr_cleanup(struct unit_module *m,
		struct gk20a *g, void *args)
{
	int err = 0;

	/* Cleanup GR */
	err = test_gr_remove_support(m, g, args);
	if (err != 0) {
		unit_return_fail(m, "gr remove support failed\n");
	}

	err = test_gr_remove_setup(m, g, args);
	if (err != 0) {
		unit_return_fail(m, "gr remove setup failed\n");
	}

	return UNIT_SUCCESS;
}

static int test_gr_intr_without_channel(struct unit_module *m,
		struct gk20a *g, void *args)
{
	int err;

	/* Set exception for FE, MEMFMT, PD, SCC, DS, SSYNC, MME, SKED */
	nvgpu_posix_io_writel_reg_space(g, gr_exception_r(),
			gr_exception_fe_m() | gr_exception_memfmt_m() |
			gr_exception_pd_m() | gr_exception_scc_m() |
			gr_exception_ds_m() | gr_exception_ssync_m() |
			gr_exception_mme_m() | gr_exception_sked_m());

	err = g->ops.gr.intr.stall_isr(g);
	if (err != 0) {
		unit_return_fail(m, "stall_isr failed\n");
	}

	return UNIT_SUCCESS;
}

struct test_gr_intr_sw_mthd_exceptions sw_excep[] = {
	[0] = {
		.trapped_addr = NVC0C0_SET_SHADER_EXCEPTIONS,
		.data[0] = NVA297_SET_SHADER_EXCEPTIONS_ENABLE_FALSE,
		.data[1] = NVA297_SET_SHADER_EXCEPTIONS_ENABLE_TRUE,
	      },
	[1] = {
		.trapped_addr = NVC3C0_SET_SKEDCHECK,
		.data[0] = NVC397_SET_SKEDCHECK_18_ENABLE,
		.data[1] = NVC397_SET_SKEDCHECK_18_DISABLE,
	      },
	[2] = {
		.trapped_addr = NVC3C0_SET_SHADER_CUT_COLLECTOR,
		.data[0] = NVC397_SET_SHADER_CUT_COLLECTOR_STATE_ENABLE,
		.data[1] = NVC397_SET_SHADER_CUT_COLLECTOR_STATE_DISABLE,
	      },
	[3] = {
		.trapped_addr = 0,
		.data[0] = 0,
		.data[1] = 0,
	      }
};

static int test_gr_intr_sw_exceptions(struct unit_module *m,
		struct gk20a *g, void *args)
{
	int err;
	int i, j, data_cnt;
	int arry_cnt = sizeof(sw_excep)/
			sizeof(struct test_gr_intr_sw_mthd_exceptions);

	/* Set illegal method pending */
	nvgpu_posix_io_writel_reg_space(g, gr_intr_r(),
				gr_intr_illegal_method_pending_f());

	for (i = 0; i < arry_cnt; i++) {
		/* method & sub channel */
		nvgpu_posix_io_writel_reg_space(g, gr_trapped_addr_r(),
					sw_excep[i].trapped_addr);
		data_cnt = (i < (arry_cnt - 1)) ? 2 : 1;

		for (j = 0; j < data_cnt; j++) {
			/* data */
			nvgpu_posix_io_writel_reg_space(g,
				gr_trapped_data_lo_r(), sw_excep[i].data[j]);

			err = g->ops.gr.intr.stall_isr(g);
			if (err != 0) {
				unit_return_fail(m, "stall isr failed\n");
			}
		}
	}

	return UNIT_SUCCESS;
}

struct unit_module_test nvgpu_gr_intr_tests[] = {
	UNIT_TEST(gr_intr_setup, test_gr_intr_setup, NULL, 0),
	UNIT_TEST(gr_intr_channel_free, test_gr_intr_without_channel, NULL, 0),
	UNIT_TEST(gr_intr_sw_method, test_gr_intr_sw_exceptions, NULL, 0),
	UNIT_TEST(gr_intr_cleanup, test_gr_intr_cleanup, NULL, 0),
};

UNIT_MODULE(nvgpu_gr_intr, nvgpu_gr_intr_tests, UNIT_PRIO_NVGPU_TEST);
