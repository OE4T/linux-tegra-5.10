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

static void gr_intr_gpc_gpcmmu_esr_regs(struct gk20a *g)
{
	u32 esr_reg = gr_gpc0_mmu_gpcmmu_global_esr_ecc_corrected_m() |
			gr_gpc0_mmu_gpcmmu_global_esr_ecc_uncorrected_m();

	nvgpu_posix_io_writel_reg_space(g,
		gr_gpc0_mmu_gpcmmu_global_esr_r(), esr_reg);
}

static void gr_intr_gpc_gpccs_esr_regs(struct gk20a *g)
{
	u32 esr_reg = gr_gpc0_gpccs_hww_esr_ecc_corrected_m() |
			gr_gpc0_gpccs_hww_esr_ecc_uncorrected_m();

	nvgpu_posix_io_writel_reg_space(g,
		gr_gpc0_gpccs_hww_esr_r(), esr_reg);
}

struct test_gr_intr_gpc_ecc_status {
	u32 status_val;
	u32 status_reg;
	u32 corr_reg;
	u32 uncorr_reg;
};

struct test_gr_intr_gpc_ecc_status gpc_ecc_reg[] = {
	[0] = { /* L1 tag ecc regs */
		.status_val = 0xFFF,
		.status_reg = gr_pri_gpc0_tpc0_sm_l1_tag_ecc_status_r(),
		.corr_reg = gr_pri_gpc0_tpc0_sm_l1_tag_ecc_corrected_err_count_r(),
		.uncorr_reg = gr_pri_gpc0_tpc0_sm_l1_tag_ecc_uncorrected_err_count_r(),
	      },
	[1] = { /* LRF ecc regs */
		.status_val = 0xFFFFFFF,
		.status_reg = gr_pri_gpc0_tpc0_sm_lrf_ecc_status_r(),
		.corr_reg = gr_pri_gpc0_tpc0_sm_lrf_ecc_corrected_err_count_r(),
		.uncorr_reg = gr_pri_gpc0_tpc0_sm_lrf_ecc_uncorrected_err_count_r(),
	      },
	[2] = { /* CBU ecc regs */
		.status_val = 0xF00FF,
		.status_reg = gr_pri_gpc0_tpc0_sm_cbu_ecc_status_r(),
		.corr_reg = gr_pri_gpc0_tpc0_sm_cbu_ecc_corrected_err_count_r(),
		.uncorr_reg = gr_pri_gpc0_tpc0_sm_cbu_ecc_uncorrected_err_count_r(),
	      },
	[3] = { /* L1 data regs */
		.status_val = 0xF0F,
		.status_reg = gr_pri_gpc0_tpc0_sm_l1_data_ecc_status_r(),
		.corr_reg = gr_pri_gpc0_tpc0_sm_l1_data_ecc_corrected_err_count_r(),
		.uncorr_reg = gr_pri_gpc0_tpc0_sm_l1_data_ecc_uncorrected_err_count_r(),
	      },
	[4] = { /* ICACHE regs */
		.status_val = 0xF00FF,
		.status_reg = gr_pri_gpc0_tpc0_sm_icache_ecc_status_r(),
		.corr_reg = gr_pri_gpc0_tpc0_sm_icache_ecc_corrected_err_count_r(),
		.uncorr_reg = gr_pri_gpc0_tpc0_sm_icache_ecc_uncorrected_err_count_r(),
	      },
	[5] = { /* MMU_L1TLB regs */
		.status_val = 0xF000F,
		.status_reg = gr_gpc0_mmu_l1tlb_ecc_status_r(),
		.corr_reg = gr_gpc0_mmu_l1tlb_ecc_corrected_err_count_r(),
		.uncorr_reg = gr_gpc0_mmu_l1tlb_ecc_uncorrected_err_count_r(),
	      },
	[6] = { /* GPCCS_FALCON regs */
		.status_val = 0xF33,
		.status_reg = gr_gpc0_gpccs_falcon_ecc_status_r(),
		.corr_reg = gr_gpc0_gpccs_falcon_ecc_corrected_err_count_r(),
		.uncorr_reg = gr_gpc0_gpccs_falcon_ecc_uncorrected_err_count_r(),
	      },
	[7] = { /* GCC_L15 regs */
		.status_val = 0xF33,
		.status_reg = gr_pri_gpc0_gcc_l15_ecc_status_r(),
		.corr_reg = gr_pri_gpc0_gcc_l15_ecc_corrected_err_count_r(),
		.uncorr_reg = gr_pri_gpc0_gcc_l15_ecc_uncorrected_err_count_r(),
	      },
};

static void gr_intr_gpc_ecc_err_regs(struct gk20a *g)
{
	u32 cnt = 20U;
	int i;
	int arry_cnt = sizeof(gpc_ecc_reg)/
			sizeof(struct test_gr_intr_gpc_ecc_status);

	for (i = 0; i < arry_cnt; i++) {

		nvgpu_posix_io_writel_reg_space(g,
				gpc_ecc_reg[i].corr_reg, cnt);
		nvgpu_posix_io_writel_reg_space(g,
				gpc_ecc_reg[i].uncorr_reg, cnt);
		nvgpu_posix_io_writel_reg_space(g,
				gpc_ecc_reg[i].status_reg,
				gpc_ecc_reg[i].status_val);
	}
}

static void gr_test_enable_gpc_exception_intr(struct gk20a *g)
{
	/* Set exception pending */
	nvgpu_posix_io_writel_reg_space(g, gr_intr_r(),
				gr_intr_exception_pending_f());

	/* Set gpc exception */
	nvgpu_posix_io_writel_reg_space(g, gr_exception_r(),
						gr_exception_gpc_m());

	/* Set gpc exception1 */
	nvgpu_posix_io_writel_reg_space(g, gr_exception1_r(),
					gr_exception1_gpc_0_pending_f());
}

static int gr_test_gpc_exception_intr(struct gk20a *g)
{
	int err = UNIT_SUCCESS;

	/* enable gpc exception interrupt bit */
	gr_test_enable_gpc_exception_intr(g);

	/* Call interrupt routine */
	err = g->ops.gr.intr.stall_isr(g);

	return err;
}

static void gr_test_set_gpc_exceptions(struct gk20a *g, bool full)
{
	u32 gpc_exception = 0;

	/* Set exceptions for gpcmmu/gcc/tpc */
	gpc_exception = gr_gpc0_gpccs_gpc_exception_gpcmmu_m() |
			gr_gpc0_gpccs_gpc_exception_gpccs_m() |
			gr_gpcs_gpccs_gpc_exception_en_gcc_f(1U) |
			gr_gpcs_gpccs_gpc_exception_en_tpc_f(1U);
	if (full) {
	/* Set exceptions for prop/zcull/setup/pes/gpccs */
		gpc_exception |= gr_gpc0_gpccs_gpc_exception_prop_m() |
			gr_gpc0_gpccs_gpc_exception_zcull_m() |
			gr_gpc0_gpccs_gpc_exception_setup_m() |
			gr_gpc0_gpccs_gpc_exception_pes0_m() |
			gr_gpc0_gpccs_gpc_exception_pes1_m();
	}

	nvgpu_posix_io_writel_reg_space(g, gr_gpc0_gpccs_gpc_exception_r(),
					gpc_exception);
}

#define TPC_EXCEPTION_TEX	(0x1U << 0U)
#define TPC_EXCEPTION_SM	(0x1U << 1U)
static void gr_test_set_tpc_exceptions(struct gk20a *g)
{
	u32 tpc_exception = 0;

	/* Tpc exceptions for mpc/pe */
	tpc_exception = gr_gpc0_tpc0_tpccs_tpc_exception_mpc_m() |
			gr_gpc0_tpc0_tpccs_tpc_exception_pe_m();

	/* Tpc exceptions for tex/sm */
	tpc_exception |= TPC_EXCEPTION_TEX | TPC_EXCEPTION_SM;

	nvgpu_posix_io_writel_reg_space(g, gr_gpc0_tpc0_tpccs_tpc_exception_r(),
					tpc_exception);
}

#define TPC_SM0_ESR_SEL	(0x1U << 0U)
#define TPC_SM1_ESR_SEL	(0x1U << 1U)
static void gr_test_set_tpc_esr_sm(struct gk20a *g)
{
	nvgpu_posix_io_writel_reg_space(g,
		gr_gpc0_tpc0_sm_tpc_esr_sm_sel_r(),
		TPC_SM0_ESR_SEL | TPC_SM1_ESR_SEL);
}

static int test_gr_intr_gpc_exceptions(struct unit_module *m,
		struct gk20a *g, void *args)
{
	int err;

	/**
	 * Negative test to verify gpc_exception interrupt without
	 * enabling any gpc_exception.
	 */
	err = gr_test_gpc_exception_intr(g);
	if (err != 0) {
		unit_return_fail(m, "isr failed without gpc exceptions\n");
	}

	/**
	 * Negative test to verify gpc_exception interrupt with
	 * enabling all gpc_exceptions, but without setting the ecc status
	 * registers
	 */
	gr_test_set_gpc_exceptions(g, true);
	gr_test_set_tpc_exceptions(g);

	err = gr_test_gpc_exception_intr(g);
	if (err != 0) {
		unit_return_fail(m, "gpc exceptions without ecc status failed\n");
	}

	/**
	 * Negative test to verify gpc_exception interrupt with
	 * enabling all gpc_exceptions and by setting the ecc status
	 * registers
	 */
	gr_test_set_gpc_exceptions(g, false);
	gr_test_set_tpc_exceptions(g);
	gr_test_set_tpc_esr_sm(g);

	gr_intr_gpc_gpcmmu_esr_regs(g);
	gr_intr_gpc_gpccs_esr_regs(g);
	gr_intr_gpc_ecc_err_regs(g);

	err = gr_test_gpc_exception_intr(g);
	if (err != 0) {
		unit_return_fail(m, "stall isr failed\n");
	}

	return UNIT_SUCCESS;
}

static void gr_intr_fecs_ecc_err_regs(struct gk20a *g)
{
	u32 cnt = 20U;
	u32 ecc_status =
	 gr_fecs_falcon_ecc_status_corrected_err_imem_m() |
	 gr_fecs_falcon_ecc_status_corrected_err_dmem_m() |
	 gr_fecs_falcon_ecc_status_uncorrected_err_imem_m() |
	 gr_fecs_falcon_ecc_status_uncorrected_err_dmem_m() |
	 gr_fecs_falcon_ecc_status_corrected_err_total_counter_overflow_m() |
	 gr_fecs_falcon_ecc_status_uncorrected_err_total_counter_overflow_m();

	nvgpu_posix_io_writel_reg_space(g,
		gr_fecs_falcon_ecc_corrected_err_count_r(), cnt);
	nvgpu_posix_io_writel_reg_space(g,
		gr_fecs_falcon_ecc_uncorrected_err_count_r(), cnt);
	nvgpu_posix_io_writel_reg_space(g,
		gr_fecs_falcon_ecc_status_r(), ecc_status);
}

static int test_gr_intr_fecs_exceptions(struct unit_module *m,
		struct gk20a *g, void *args)
{
	int err, i;
	u32 fecs_status[6] = {
		gr_fecs_host_int_enable_ctxsw_intr0_enable_f() |
		gr_fecs_host_int_enable_ctxsw_intr1_enable_f(),
		gr_fecs_host_int_enable_fault_during_ctxsw_enable_f(),
		gr_fecs_host_int_enable_umimp_firmware_method_enable_f(),
		gr_fecs_host_int_enable_umimp_illegal_method_enable_f(),
		gr_fecs_host_int_enable_watchdog_enable_f(),
		gr_fecs_host_int_enable_ecc_corrected_enable_f() |
		gr_fecs_host_int_enable_ecc_uncorrected_enable_f(),
	};

	for (i = 0; i < 6; i++) {
		/* Set fecs error pending */
		nvgpu_posix_io_writel_reg_space(g, gr_intr_r(),
					gr_intr_fecs_error_pending_f());

		/* Set fecs host register status */
		nvgpu_posix_io_writel_reg_space(g, gr_fecs_host_int_status_r(),
					fecs_status[i]);

		/* Set fecs ecc registers */
		if (i == 5) {
			gr_intr_fecs_ecc_err_regs(g);
		}

		err = g->ops.gr.intr.stall_isr(g);
		if (err != 0) {
			unit_return_fail(m, "failed in fecs error interrupts\n");
		}
	}
	return UNIT_SUCCESS;
}

struct unit_module_test nvgpu_gr_intr_tests[] = {
	UNIT_TEST(gr_intr_setup, test_gr_intr_setup, NULL, 0),
	UNIT_TEST(gr_intr_channel_free, test_gr_intr_without_channel, NULL, 0),
	UNIT_TEST(gr_intr_sw_method, test_gr_intr_sw_exceptions, NULL, 0),
	UNIT_TEST(gr_intr_fecs_exceptions, test_gr_intr_fecs_exceptions, NULL, 0),
	UNIT_TEST(gr_intr_gpc_exceptions, test_gr_intr_gpc_exceptions, NULL, 0),
	UNIT_TEST(gr_intr_cleanup, test_gr_intr_cleanup, NULL, 0),
};

UNIT_MODULE(nvgpu_gr_intr, nvgpu_gr_intr_tests, UNIT_PRIO_NVGPU_TEST);
