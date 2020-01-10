/*
 * TU104 Clocks
 *
 * Copyright (c) 2016-2019, NVIDIA CORPORATION.  All rights reserved.
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

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include "os/linux/os_linux.h"
#endif

#include <nvgpu/kmem.h>
#include <nvgpu/io.h>
#include <nvgpu/list.h>
#include <nvgpu/clk_arb.h>
#include <nvgpu/soc.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/clk.h>
#include <nvgpu/pmu/perf.h>
#include <nvgpu/clk_arb.h>
#include <nvgpu/pmu/clk/clk.h>
#include <nvgpu/pmu/clk/clk_domain.h>
#include <nvgpu/hw/tu104/hw_trim_tu104.h>

#include "clk_tu104.h"



#define CLK_NAMEMAP_INDEX_GPCCLK	0x00
#define CLK_NAMEMAP_INDEX_XBARCLK	0x02
#define CLK_NAMEMAP_INDEX_SYSCLK	0x07	/* SYSPLL */
#define CLK_NAMEMAP_INDEX_DRAMCLK	0x20	/* DRAMPLL */

#define CLK_DEFAULT_CNTRL_SETTLE_RETRIES 10
#define CLK_DEFAULT_CNTRL_SETTLE_USECS   5

#define XTAL_CNTR_CLKS		27000	/* 1000usec at 27KHz XTAL */
#define XTAL_CNTR_DELAY		10000	/* we need acuracy up to the 10ms   */
#define XTAL_SCALE_TO_KHZ	1
#define NUM_NAMEMAPS    (3U)
#define XTAL4X_KHZ 108000
#define BOOT_GPCCLK_MHZ 645U

/**
 * Mapping between the clk domain and the various clock monitor registers
 * The rows represent clock domains starting from index 0 and column represent
 * the various registers each domain has, non available domains are set to 0
 * for easy accessing, refer nvgpu_clk_mon_init_domains() for valid domains.
 */
static  u32 clock_mon_map_tu104[CLK_CLOCK_MON_DOMAIN_COUNT]
				[CLK_CLOCK_MON_REG_TYPE_COUNT] = {
	{
		trim_gpcclk_fault_threshold_high_r(),
		trim_gpcclk_fault_threshold_low_r(),
		trim_gpcclk_fault_status_r(),
		trim_gpcclk_fault_priv_level_mask_r(),
	},
	{
		trim_xbarclk_fault_threshold_high_r(),
		trim_xbarclk_fault_threshold_low_r(),
		trim_xbarclk_fault_status_r(),
		trim_xbarclk_fault_priv_level_mask_r(),
	},
	{
		trim_sysclk_fault_threshold_high_r(),
		trim_sysclk_fault_threshold_low_r(),
		trim_sysclk_fault_status_r(),
		trim_sysclk_fault_priv_level_mask_r(),
	},
	{
		trim_hubclk_fault_threshold_high_r(),
		trim_hubclk_fault_threshold_low_r(),
		trim_hubclk_fault_status_r(),
		trim_hubclk_fault_priv_level_mask_r(),
	},
	{
		trim_dramclk_fault_threshold_high_r(),
		trim_dramclk_fault_threshold_low_r(),
		trim_dramclk_fault_status_r(),
		trim_dramclk_fault_priv_level_mask_r(),
	},
	{
		trim_hostclk_fault_threshold_high_r(),
		trim_hostclk_fault_threshold_low_r(),
		trim_hostclk_fault_status_r(),
		trim_hostclk_fault_priv_level_mask_r(),
	},
	{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0},
	{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0},
	{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0},
	{
		trim_utilsclk_fault_threshold_high_r(),
		trim_utilsclk_fault_threshold_low_r(),
		trim_utilsclk_fault_status_r(),
		trim_utilsclk_fault_priv_level_mask_r(),
	},
	{
		trim_pwrclk_fault_threshold_high_r(),
		trim_pwrclk_fault_threshold_low_r(),
		trim_pwrclk_fault_status_r(),
		trim_pwrclk_fault_priv_level_mask_r(),
	},
	{
		trim_nvdclk_fault_threshold_high_r(),
		trim_nvdclk_fault_threshold_low_r(),
		trim_nvdclk_fault_status_r(),
		trim_nvdclk_fault_priv_level_mask_r(),
	},
	{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0},
	{
		trim_xclk_fault_threshold_high_r(),
		trim_xclk_fault_threshold_low_r(),
		trim_xclk_fault_status_r(),
		trim_xclk_fault_priv_level_mask_r(),
	},
	{
		trim_nvl_commonclk_fault_threshold_high_r(),
		trim_nvl_commonclk_fault_threshold_low_r(),
		trim_nvl_commonclk_fault_status_r(),
		trim_nvl_commonclk_fault_priv_level_mask_r(),
	},
	{
		trim_pex_refclk_fault_threshold_high_r(),
		trim_pex_refclk_fault_threshold_low_r(),
		trim_pex_refclk_fault_status_r(),
		trim_pex_refclk_fault_priv_level_mask_r(),
	},
	{0,0,0,0}, {0,0,0,0}, {0,0,0,0}
};

static u32 nvgpu_check_for_dc_fault(u32 data)
{
	return (trim_fault_status_dc_v(data) ==
		trim_fault_status_dc_true_v()) ?
				trim_fault_status_dc_m() : 0U;
}

static u32 nvgpu_check_for_lower_threshold_fault(u32 data)
{
	return (trim_fault_status_lower_threshold_v(data) ==
		trim_fault_status_lower_threshold_true_v()) ?
				trim_fault_status_lower_threshold_m() : 0U;
}

static u32 nvgpu_check_for_higher_threshold_fault(u32 data)
{
	return (trim_fault_status_higher_threshold_v(data) ==
		trim_fault_status_higher_threshold_true_v()) ?
				trim_fault_status_higher_threshold_m() : 0U;
}

static u32 nvgpu_check_for_overflow_err(u32 data)
{
	return (trim_fault_status_overflow_v(data) ==
		trim_fault_status_overflow_true_v()) ?
				trim_fault_status_overflow_m() : 0U;
}

static int nvgpu_clk_mon_get_fault(struct gk20a *g, u32 i, u32 data,
		struct clk_domains_mon_status_params *clk_mon_status)
{
	u32 reg_address;
	int status = 0;

	/* Fields for faults are same for all clock domains */
	clk_mon_status->clk_mon_list[i].clk_domain_fault_status =
		((nvgpu_check_for_dc_fault(data)) |
		(nvgpu_check_for_lower_threshold_fault(data)) |
		(nvgpu_check_for_higher_threshold_fault(data)) |
		(nvgpu_check_for_overflow_err(data)));
	nvgpu_err(g, "FMON faulted domain 0x%x value 0x%x",
		clk_mon_status->clk_mon_list[i].clk_api_domain,
		clk_mon_status->clk_mon_list[i].
		clk_domain_fault_status);

	/* Get the low threshold limit */
	reg_address = clock_mon_map_tu104[i][FMON_THRESHOLD_LOW];
	data = nvgpu_readl(g, reg_address);
	clk_mon_status->clk_mon_list[i].low_threshold =
		trim_fault_threshold_low_count_v(data);

	/* Get the high threshold limit */
	reg_address = clock_mon_map_tu104[i][FMON_THRESHOLD_HIGH];
	data = nvgpu_readl(g, reg_address);
	clk_mon_status->clk_mon_list[i].high_threshold =
		trim_fault_threshold_high_count_v(data);

	return status;
}

bool nvgpu_clk_mon_check_master_fault_status(struct gk20a *g)
{
	u32 fmon_master_status = nvgpu_readl(g, trim_fmon_master_status_r());

	if (trim_fmon_master_status_fault_out_v(fmon_master_status) ==
			trim_fmon_master_status_fault_out_true_v()) {
		return true;
	}
	return false;
}

int nvgpu_clk_mon_check_status(struct gk20a *g,
		struct clk_domains_mon_status_params *clk_mon_status,
		u32 domain_mask)
{
	u32 reg_address, bit_pos;
	u32 data;
	int status;

	clk_mon_status->clk_mon_domain_mask = domain_mask;
	/*
	 * Parse through each domain and check for faults, each bit set
	 * represents a domain here
	 */
	for (bit_pos = 0U; bit_pos < (sizeof(domain_mask) * BITS_PER_BYTE);
			bit_pos++) {
		if (nvgpu_test_bit(bit_pos, (void *)&domain_mask)) {
			clk_mon_status->clk_mon_list[bit_pos].clk_api_domain =
					BIT(bit_pos);

			reg_address = clock_mon_map_tu104[bit_pos]
					[FMON_FAULT_STATUS];
			data = nvgpu_readl(g, reg_address);

			clk_mon_status->clk_mon_list[bit_pos].
				clk_domain_fault_status = 0U;
			/* Check FMON fault status, field is same for all */
			if (trim_fault_status_fault_out_v(data) ==
				trim_fault_status_fault_out_true_v()) {
				status = nvgpu_clk_mon_get_fault(g, bit_pos,
						data, clk_mon_status);
				if (status != 0) {
					nvgpu_err(g, "Failed to get status");
					return -EINVAL;
				}
			}
		}
	}
	return 0;
}

u32 tu104_crystal_clk_hz(struct gk20a *g)
{
	return (XTAL4X_KHZ * 1000);
}

unsigned long tu104_clk_measure_freq(struct gk20a *g, u32 api_domain)
{
	struct clk_gk20a *clk = &g->clk;
	u32 freq_khz;
	u32 i;
	struct namemap_cfg *c = NULL;

	for (i = 0; i < clk->namemap_num; i++) {
		if (api_domain == clk->namemap_xlat_table[i]) {
			c = &clk->clk_namemap[i];
			break;
		}
	}

	if (c == NULL) {
		return 0;
	}
	if (c->is_counter != 0U) {
		freq_khz = c->scale * tu104_get_rate_cntr(g, c);
	} else {
		freq_khz = 0U;
		 /* TODO: PLL read */
	}

	/* Convert to HZ */
	return (freq_khz * 1000UL);
}

static void nvgpu_gpu_gpcclk_counter_init(struct gk20a *g)
{
	u32 data;

	data = gk20a_readl(g, trim_gpc_bcast_fr_clk_cntr_ncgpcclk_cfg_r());
	data |= trim_gpc_bcast_fr_clk_cntr_ncgpcclk_cfg_update_cycle_init_f() |
		trim_gpc_bcast_fr_clk_cntr_ncgpcclk_cfg_cont_update_enabled_f() |
		trim_gpc_bcast_fr_clk_cntr_ncgpcclk_cfg_start_count_disabled_f() |
		trim_gpc_bcast_fr_clk_cntr_ncgpcclk_cfg_reset_asserted_f() |
		trim_gpc_bcast_fr_clk_cntr_ncgpcclk_cfg_source_gpcclk_noeg_f();
	gk20a_writel(g,trim_gpc_bcast_fr_clk_cntr_ncgpcclk_cfg_r(), data);
	/*
	* Based on the clock counter design, it takes 16 clock cycles of the
	* "counted clock" for the counter to completely reset. Considering
	* 27MHz as the slowest clock during boot time, delay of 16/27us (~1us)
	* should be sufficient. See Bug 1953217.
	*/
	nvgpu_udelay(1);
	data = gk20a_readl(g, trim_gpc_bcast_fr_clk_cntr_ncgpcclk_cfg_r());
	data = set_field(data, trim_gpc_bcast_fr_clk_cntr_ncgpcclk_cfg_reset_m(),
			trim_gpc_bcast_fr_clk_cntr_ncgpcclk_cfg_reset_deasserted_f());
	gk20a_writel(g,trim_gpc_bcast_fr_clk_cntr_ncgpcclk_cfg_r(), data);
	/*
	* Enable clock counter.
	* Note : Need to write un-reset and enable signal in different
	* register writes as the source (register block) and destination
	* (FR counter) are on the same clock and far away from each other,
	* so the signals can not reach in the same clock cycle hence some
	* delay is required between signals.
	*/
	data = gk20a_readl(g, trim_gpc_bcast_fr_clk_cntr_ncgpcclk_cfg_r());
	data |= trim_gpc_bcast_fr_clk_cntr_ncgpcclk_cfg_start_count_enabled_f();
	gk20a_writel(g,trim_gpc_bcast_fr_clk_cntr_ncgpcclk_cfg_r(), data);
}

static void nvgpu_gpu_sysclk_counter_init(struct gk20a *g)
{
	u32 data;

	data = gk20a_readl(g, trim_sys_fr_clk_cntr_sysclk_cfg_r());
	data |= trim_sys_fr_clk_cntr_sysclk_cfg_update_cycle_init_f() |
		trim_sys_fr_clk_cntr_sysclk_cfg_cont_update_enabled_f() |
		trim_sys_fr_clk_cntr_sysclk_cfg_start_count_disabled_f() |
		trim_sys_fr_clk_cntr_sysclk_cfg_reset_asserted_f() |
		trim_sys_fr_clk_cntr_sysclk_cfg_source_sys_noeg_f();
	gk20a_writel(g,trim_sys_fr_clk_cntr_sysclk_cfg_r(), data);

	nvgpu_udelay(1);

	data = gk20a_readl(g, trim_sys_fr_clk_cntr_sysclk_cfg_r());
	data = set_field(data, trim_sys_fr_clk_cntr_sysclk_cfg_reset_m(),
			trim_sys_fr_clk_cntr_sysclk_cfg_reset_deasserted_f());
	gk20a_writel(g,trim_sys_fr_clk_cntr_sysclk_cfg_r(), data);

	data = gk20a_readl(g, trim_sys_fr_clk_cntr_sysclk_cfg_r());
	data |= trim_sys_fr_clk_cntr_sysclk_cfg_start_count_enabled_f();
	gk20a_writel(g,trim_sys_fr_clk_cntr_sysclk_cfg_r(), data);
}

static void nvgpu_gpu_xbarclk_counter_init(struct gk20a *g)
{
	u32 data;

	data = gk20a_readl(g, trim_sys_fll_fr_clk_cntr_xbarclk_cfg_r());
	data |= trim_sys_fll_fr_clk_cntr_xbarclk_cfg_update_cycle_init_f() |
		trim_sys_fll_fr_clk_cntr_xbarclk_cfg_cont_update_enabled_f() |
		trim_sys_fll_fr_clk_cntr_xbarclk_cfg_start_count_disabled_f() |
		trim_sys_fll_fr_clk_cntr_xbarclk_cfg_reset_asserted_f() |
		trim_sys_fll_fr_clk_cntr_xbarclk_cfg_source_xbar_nobg_f();
	gk20a_writel(g,trim_sys_fll_fr_clk_cntr_xbarclk_cfg_r(), data);

	nvgpu_udelay(1);

	data = gk20a_readl(g, trim_sys_fll_fr_clk_cntr_xbarclk_cfg_r());
	data = set_field(data, trim_sys_fll_fr_clk_cntr_xbarclk_cfg_reset_m(),
			trim_sys_fll_fr_clk_cntr_xbarclk_cfg_reset_deasserted_f());
	gk20a_writel(g,trim_sys_fll_fr_clk_cntr_xbarclk_cfg_r(), data);

	data = gk20a_readl(g, trim_sys_fll_fr_clk_cntr_xbarclk_cfg_r());
	data |= trim_sys_fll_fr_clk_cntr_xbarclk_cfg_start_count_enabled_f();
	gk20a_writel(g,trim_sys_fll_fr_clk_cntr_xbarclk_cfg_r(), data);
}

int tu104_init_clk_support(struct gk20a *g)
{
	struct clk_gk20a *clk = &g->clk;


	nvgpu_log_fn(g, " ");

	nvgpu_mutex_init(&clk->clk_mutex);

	clk->clk_namemap = (struct namemap_cfg *)
		nvgpu_kzalloc(g, sizeof(struct namemap_cfg) * NUM_NAMEMAPS);

	if (clk->clk_namemap == NULL) {
		nvgpu_mutex_destroy(&clk->clk_mutex);
		return -ENOMEM;
	}

	clk->namemap_xlat_table = nvgpu_kcalloc(g, NUM_NAMEMAPS, sizeof(u32));

	if (clk->namemap_xlat_table == NULL) {
		nvgpu_kfree(g, clk->clk_namemap);
		nvgpu_mutex_destroy(&clk->clk_mutex);
		return -ENOMEM;
	}

	clk->clk_namemap[0] = (struct namemap_cfg) {
		.namemap = CLK_NAMEMAP_INDEX_GPCCLK,
		.is_enable = 1,
		.is_counter = 1,
		.g = g,
		.cntr = {
			.reg_ctrl_addr = trim_gpc_bcast_fr_clk_cntr_ncgpcclk_cfg_r(),
			.reg_ctrl_idx  = trim_gpc_bcast_fr_clk_cntr_ncgpcclk_cfg_source_gpcclk_noeg_f(),
			.reg_cntr_addr[0] = trim_gpc_bcast_fr_clk_cntr_ncgpcclk_cnt0_r(),
			.reg_cntr_addr[1] = trim_gpc_bcast_fr_clk_cntr_ncgpcclk_cnt1_r()
		},
		.name = "gpcclk",
		.scale = 1
	};

	nvgpu_gpu_gpcclk_counter_init(g);
	clk->namemap_xlat_table[0] = CTRL_CLK_DOMAIN_GPCCLK;

	clk->clk_namemap[1] = (struct namemap_cfg) {
		.namemap = CLK_NAMEMAP_INDEX_SYSCLK,
		.is_enable = 1,
		.is_counter = 1,
		.g = g,
		.cntr = {
			.reg_ctrl_addr = trim_sys_fr_clk_cntr_sysclk_cfg_r(),
			.reg_ctrl_idx  = trim_sys_fr_clk_cntr_sysclk_cfg_source_sys_noeg_f(),
			.reg_cntr_addr[0] = trim_sys_fr_clk_cntr_sysclk_cntr0_r(),
			.reg_cntr_addr[1] = trim_sys_fr_clk_cntr_sysclk_cntr1_r()
		},
		.name = "sysclk",
		.scale = 1
	};

	nvgpu_gpu_sysclk_counter_init(g);
	clk->namemap_xlat_table[1] = CTRL_CLK_DOMAIN_SYSCLK;

	clk->clk_namemap[2] = (struct namemap_cfg) {
		.namemap = CLK_NAMEMAP_INDEX_XBARCLK,
		.is_enable = 1,
		.is_counter = 1,
		.g = g,
		.cntr = {
			.reg_ctrl_addr = trim_sys_fll_fr_clk_cntr_xbarclk_cfg_r(),
			.reg_ctrl_idx  = trim_sys_fll_fr_clk_cntr_xbarclk_cfg_source_xbar_nobg_f(),
			.reg_cntr_addr[0] = trim_sys_fll_fr_clk_cntr_xbarclk_cntr0_r(),
			.reg_cntr_addr[1] = trim_sys_fll_fr_clk_cntr_xbarclk_cntr1_r()
		},
		.name = "xbarclk",
		.scale = 1
	};

	nvgpu_gpu_xbarclk_counter_init(g);
	clk->namemap_xlat_table[2] = CTRL_CLK_DOMAIN_XBARCLK;

	clk->namemap_num = NUM_NAMEMAPS;

	clk->g = g;

	return 0;
}

u32 tu104_get_rate_cntr(struct gk20a *g, struct namemap_cfg *c) {
	u32 cntr = 0;
	u64 cntr_start = 0;
	u64 cntr_stop = 0;
	u64 start_time, stop_time;
	const int max_iterations = 3;
	int i = 0;

	struct clk_gk20a *clk = &g->clk;

	if ((c == NULL) || (c->cntr.reg_ctrl_addr == 0U) ||
		(c->cntr.reg_cntr_addr[0] == 0U) ||
		(c->cntr.reg_cntr_addr[1]) == 0U) {
			return 0;
	}

	nvgpu_mutex_acquire(&clk->clk_mutex);

	for (i = 0; i < max_iterations; i++) {
		/*
		 * Read the counter values. Counter is 36 bits, 32
		 * bits on addr[0] and 4 lsb on addr[1] others zero.
		 */
		cntr_start = (u64)nvgpu_readl(g,
				c->cntr.reg_cntr_addr[0]);
		cntr_start += ((u64)nvgpu_readl(g,
				c->cntr.reg_cntr_addr[1]) << 32);
		start_time = (u64)nvgpu_hr_timestamp_us();
		nvgpu_udelay(XTAL_CNTR_DELAY);
		stop_time = (u64)nvgpu_hr_timestamp_us();
		cntr_stop = (u64)nvgpu_readl(g,
				c->cntr.reg_cntr_addr[0]);
		cntr_stop += ((u64)nvgpu_readl(g,
				c->cntr.reg_cntr_addr[1]) << 32);

		if (cntr_stop > cntr_start) {
			/*
			 * Calculate the difference with Acutal time
			 * and convert to KHz
			 */
			cntr = (u32)(((cntr_stop - cntr_start) /
				(stop_time - start_time)) * 1000U);
			nvgpu_mutex_release(&clk->clk_mutex);
			return cntr;
		}
		/* Else wrap around detected. Hence, retry. */
	}

	nvgpu_mutex_release(&clk->clk_mutex);
	/* too many iterations, bail out */
	nvgpu_err(g, "failed to get clk rate");
	return -EBUSY;
}

#ifdef CONFIG_NVGPU_CLK_ARB
int tu104_clk_domain_get_f_points(
	struct gk20a *g,
	u32 clkapidomain,
	u32 *pfpointscount,
	u16 *pfreqpointsinmhz)
{
	int status = -EINVAL;
	struct nvgpu_clk_domain *pdomain;
	u8 i;
	struct nvgpu_clk_pmupstate *pclk = g->pmu->clk_pmu;
	if (pfpointscount == NULL) {
		return -EINVAL;
	}

	if ((pfreqpointsinmhz == NULL) && (*pfpointscount != 0U)) {
		return -EINVAL;
	}
	BOARDOBJGRP_FOR_EACH(&(pclk->clk_domainobjs->super.super),
			struct nvgpu_clk_domain *, pdomain, i) {
		if (pdomain->api_domain == clkapidomain) {
			status = pdomain->clkdomainclkgetfpoints(g, pclk,
				pdomain, pfpointscount,
				pfreqpointsinmhz,
				CLK_PROG_VFE_ENTRY_LOGIC);
			return status;
		}
	}
	return status;
}
#endif

void tu104_suspend_clk_support(struct gk20a *g)
{
	nvgpu_mutex_destroy(&g->clk.clk_mutex);
}
#ifdef CONFIG_NVGPU_CLK_ARB
unsigned long tu104_clk_maxrate(struct gk20a *g, u32 api_domain)
{
	u16 min_mhz = 0, max_mhz = 0;
	int status;

	if (nvgpu_is_enabled(g, NVGPU_PMU_PSTATE)) {
		status = nvgpu_clk_arb_get_arbiter_clk_range(g, api_domain,
				&min_mhz, &max_mhz);
		if (status != 0) {
			nvgpu_err(g, "failed to fetch clock range");
			return 0U;
		}
	} else {
		if (api_domain == NVGPU_CLK_DOMAIN_GPCCLK) {
			max_mhz = BOOT_GPCCLK_MHZ;
		}
	}

	return (max_mhz * 1000UL * 1000UL);
}

void tu104_get_change_seq_time(struct gk20a *g, s64 *change_time)
{
	struct change_seq_pmu *change_seq_pmu = &g->perf_pmu->changeseq_pmu;
	s64 diff = change_seq_pmu->stop_time - change_seq_pmu->start_time;

	*change_time = diff;
}
#endif
void tu104_change_host_clk_source(struct gk20a *g)
{
	nvgpu_writel(g, trim_sys_ind_clk_sys_core_clksrc_r(),
			trim_sys_ind_clk_sys_core_clksrc_hostclk_fll_f());
}
