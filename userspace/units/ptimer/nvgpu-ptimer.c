/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <unit/unit.h>
#include <unit/io.h>
#include <nvgpu/posix/io.h>

#include <nvgpu/gk20a.h>
#include <nvgpu/ptimer.h>
#include <hal/ptimer/ptimer_gk20a.h>
#include <nvgpu/hw/gk20a/hw_timer_gk20a.h>

#include "nvgpu-ptimer.h"

/*
 * Mock I/O
 */

/*
 * Write callback. Forward the write access to the mock IO framework.
 */
static void writel_access_reg_fn(struct gk20a *g,
				 struct nvgpu_reg_access *access)
{
	nvgpu_posix_io_writel_reg_space(g, access->addr, access->value);
}

/* Used to simulate wrap */
#define TIMER1_VALUES_SIZE 4
static u32 timer1_values[TIMER1_VALUES_SIZE];
static u32 timer1_index;

/*
 * Read callback. Get the register value from the mock IO framework.
 */
static void readl_access_reg_fn(struct gk20a *g,
				struct nvgpu_reg_access *access)
{
	/* Used to simulate wrap */
	if (access->addr == timer_time_1_r()) {
		BUG_ON(timer1_index >= TIMER1_VALUES_SIZE);
		access->value = timer1_values[timer1_index++];
	} else {
		access->value = nvgpu_posix_io_readl_reg_space(g, access->addr);
	}
}

static struct nvgpu_posix_io_callbacks test_reg_callbacks = {
	/* Write APIs all can use the same accessor. */
	.writel          = writel_access_reg_fn,
	.writel_check    = writel_access_reg_fn,
	.bar1_writel     = writel_access_reg_fn,
	.usermode_writel = writel_access_reg_fn,

	/* Likewise for the read APIs. */
	.__readl         = readl_access_reg_fn,
	.readl           = readl_access_reg_fn,
	.bar1_readl      = readl_access_reg_fn,
};

/* map the whole page */
#define PTIMER_REG_SPACE_START (timer_pri_timeout_r() & ~0xfff)
#define PTIMER_REG_SPACE_SIZE 0xfff

int test_setup_env(struct unit_module *m,
			  struct gk20a *g, void *args)
{
	/* Setup HAL */
	g->ops.ptimer.read_ptimer = gk20a_read_ptimer;
	g->ops.ptimer.isr = gk20a_ptimer_isr;

	/* Create ptimer register space */
	if (nvgpu_posix_io_add_reg_space(g, PTIMER_REG_SPACE_START,
					 PTIMER_REG_SPACE_SIZE) != 0) {
		unit_err(m, "%s: failed to create register space\n",
			 __func__);
		return UNIT_FAIL;
	}
	(void)nvgpu_posix_register_io(g, &test_reg_callbacks);

	return UNIT_SUCCESS;
}

int test_free_env(struct unit_module *m,
			 struct gk20a *g, void *args)
{
	/* Free register space */
	nvgpu_posix_io_delete_reg_space(g, PTIMER_REG_SPACE_START);

	return UNIT_SUCCESS;
}

int test_read_ptimer(struct unit_module *m,
			struct gk20a *g, void *args)
{
	int ret = UNIT_SUCCESS;
	u32 timer0; /* low bits */
	u32 timer1; /* high bits */
	u64 time;
	int err; /* return from API */

	/* Standard, successful, easy case where there's no wrap */
	timer0 = 1;
	timer1 = 2;
	nvgpu_posix_io_writel_reg_space(g, timer_time_0_r(), timer0);
	timer1_index = 0;
	timer1_values[timer1_index] = timer1;
	timer1_values[timer1_index + 1] = timer1;
	err = g->ops.ptimer.read_ptimer(g, &time);
	if ((err != 0) || (time != ((u64)timer1 << 32 | timer0))) {
		unit_err(m, "ptimer read_timer failed simple test, err=%d, time=0x%016llx\n",
			 err, time);
		ret = UNIT_FAIL;
	}

	/* Wrap timer1 once */
	timer0 = 1;
	nvgpu_posix_io_writel_reg_space(g, timer_time_0_r(), timer0);
	timer1 = 3;
	timer1_index = 0;
	timer1_values[timer1_index] = timer1 + 1;
	timer1_values[timer1_index + 1] = timer1;
	timer1_values[timer1_index + 2] = timer1;
	timer1_values[timer1_index + 3] = timer1 - 1;
	err = g->ops.ptimer.read_ptimer(g, &time);
	if ((err != 0) || (time != ((u64)timer1 << 32 | timer0))) {
		unit_err(m, "ptimer read_timer failed single wrap test, err=%d, time=0x%016llx\n",
			 err, time);
		ret = UNIT_FAIL;
	}

	/* Wrap timer1 every time to timeout */
	timer0 = 1;
	nvgpu_posix_io_writel_reg_space(g, timer_time_0_r(), timer0);
	timer1_index = 0;
	timer1_values[timer1_index] = 4;
	timer1_values[timer1_index + 1] = 3;
	timer1_values[timer1_index + 2] = 2;
	timer1_values[timer1_index + 3] = 1;
	err = g->ops.ptimer.read_ptimer(g, &time);
	if (err == 0) {
		unit_err(m, "ptimer read_timer failed multiple wrap test\n");
		ret = UNIT_FAIL;
	}

	/* branch testing */
	err = g->ops.ptimer.read_ptimer(g, NULL);
	if (err == 0) {
		unit_err(m, "ptimer read_timer failed branch test\n");
		ret = UNIT_FAIL;
	}

	return ret;
}

static u32 received_error_code;
static void mock_decode_error_code(struct gk20a *g, u32 error_code)
{
	received_error_code = error_code;
}

int test_ptimer_isr(struct unit_module *m,
			struct gk20a *g, void *args)
{
	int ret = UNIT_SUCCESS;
	int val0, val1;
	u32 fecs_errcode = 0xa5;

	/* initialize regs to defaults */
	nvgpu_posix_io_writel_reg_space(g, timer_pri_timeout_save_0_r(), 0);
	nvgpu_posix_io_writel_reg_space(g, timer_pri_timeout_save_1_r(), 0);
	nvgpu_posix_io_writel_reg_space(g, timer_pri_timeout_fecs_errcode_r(),
					0);

	/* all zero test */
	g->ops.ptimer.isr(g);
	val0 = nvgpu_posix_io_readl_reg_space(g, timer_pri_timeout_save_0_r());
	val1 = nvgpu_posix_io_readl_reg_space(g, timer_pri_timeout_save_1_r());
	if ((val0 != 0) || (val1 != 0)) {
		unit_err(m, "ptimer isr failed to clear regs\n");
		ret = UNIT_FAIL;
	}

	/* set fecs bits */
	nvgpu_posix_io_writel_reg_space(g, timer_pri_timeout_save_0_r(),
					((u32)1 << 31));
	nvgpu_posix_io_writel_reg_space(g, timer_pri_timeout_fecs_errcode_r(),
					fecs_errcode);
	g->ops.ptimer.isr(g);
	val0 = nvgpu_posix_io_readl_reg_space(g, timer_pri_timeout_save_0_r());
	val1 = nvgpu_posix_io_readl_reg_space(g, timer_pri_timeout_save_1_r());
	if ((val0 != 0) || (val1 != 0)) {
		unit_err(m, "ptimer isr failed to clear regs\n");
		ret = UNIT_FAIL;
	}

	/* with fecs set and a decode HAL to call */
	g->ops.priv_ring.decode_error_code = mock_decode_error_code;
	nvgpu_posix_io_writel_reg_space(g, timer_pri_timeout_save_0_r(),
					((u32)1 << 31));
	nvgpu_posix_io_writel_reg_space(g, timer_pri_timeout_fecs_errcode_r(),
					fecs_errcode);
	g->ops.ptimer.isr(g);
	if (received_error_code != fecs_errcode) {
		unit_err(m, "ptimer isr failed pass err code to HAL\n");
		ret = UNIT_FAIL;
	}
	val0 = nvgpu_posix_io_readl_reg_space(g, timer_pri_timeout_save_0_r());
	val1 = nvgpu_posix_io_readl_reg_space(g, timer_pri_timeout_save_1_r());
	if ((val0 != 0) || (val1 != 0)) {
		unit_err(m, "ptimer isr failed to clear regs\n");
		ret = UNIT_FAIL;
	}

	/* Set save0 timeout bit to get a branch covered */
	nvgpu_posix_io_writel_reg_space(g, timer_pri_timeout_save_0_r(),
					((u32)1 << 1));
	nvgpu_posix_io_writel_reg_space(g, timer_pri_timeout_fecs_errcode_r(),
					0);
	g->ops.ptimer.isr(g);
	val0 = nvgpu_posix_io_readl_reg_space(g, timer_pri_timeout_save_0_r());
	val1 = nvgpu_posix_io_readl_reg_space(g, timer_pri_timeout_save_1_r());
	if ((val0 != 0) || (val1 != 0)) {
		unit_err(m, "ptimer isr failed to clear regs\n");
		ret = UNIT_FAIL;
	}

	return ret;
}

int test_ptimer_scaling(struct unit_module *m,
			struct gk20a *g, void *args)
{
	int ret = UNIT_SUCCESS;
	u32 val;

	val = scale_ptimer(100, 20);
	if (val != 50) {
		unit_err(m, "ptimer scale calculation incorrect\n");
		ret = UNIT_FAIL;
	}

	val = scale_ptimer(111, 20);
	if (val != 56) {
		unit_err(m, "ptimer scale calculation incorrect\n");
		ret = UNIT_FAIL;
	}

	val = scale_ptimer(U32_MAX/10, 20);
	if (val != (U32_MAX/20)+1) {
		unit_err(m, "ptimer scale calculation incorrect\n");
		ret = UNIT_FAIL;
	}

	val = scale_ptimer(0, U32_MAX);
	if (val != 0) {
		unit_err(m, "ptimer scale calculation incorrect\n");
		ret = UNIT_FAIL;
	}

	val = scale_ptimer(100, 1);
	if (val != 1001) {
		unit_err(m, "ptimer scale calculation incorrect\n");
		ret = UNIT_FAIL;
	}

	val = scale_ptimer(10, 6);
	if (val != 17) {
		unit_err(m, "ptimer scale calculation incorrect\n");
		ret = UNIT_FAIL;
	}

	val = ptimer_scalingfactor10x(100);
	if (val != (PTIMER_REF_FREQ_HZ*10/100)) {
		unit_err(m, "ptimer scale calculation incorrect\n");
		ret = UNIT_FAIL;
	}

	val = ptimer_scalingfactor10x(97);
	if (val != (PTIMER_REF_FREQ_HZ*10/97)) {
		unit_err(m, "ptimer scale calculation incorrect\n");
		ret = UNIT_FAIL;
	}

	val = ptimer_scalingfactor10x(100);
	if (val != (PTIMER_REF_FREQ_HZ*10/100)) {
		unit_err(m, "ptimer scale calculation incorrect\n");
		ret = UNIT_FAIL;
	}

	val = ptimer_scalingfactor10x(PTIMER_REF_FREQ_HZ);
	if (val != 10) {
		unit_err(m, "ptimer scale calculation incorrect\n");
		ret = UNIT_FAIL;
	}

	return ret;
}

struct unit_module_test ptimer_tests[] = {
	UNIT_TEST(ptimer_setup_env,	test_setup_env,		NULL, 0),
	UNIT_TEST(ptimer_read_ptimer,	test_read_ptimer,	NULL, 0),
	UNIT_TEST(ptimer_isr,		test_ptimer_isr,	NULL, 0),
	UNIT_TEST(ptimer_scaling,	test_ptimer_scaling,	NULL, 0),
	UNIT_TEST(ptimer_free_env,	test_free_env,		NULL, 0),
};

UNIT_MODULE(ptimer, ptimer_tests, UNIT_PRIO_NVGPU_TEST);
