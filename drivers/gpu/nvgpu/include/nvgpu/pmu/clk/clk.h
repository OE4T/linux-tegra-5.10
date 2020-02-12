/*
 * general clock structures & definitions
 *
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

#ifndef NVGPU_PMU_CLK_H
#define NVGPU_PMU_CLK_H

#include <nvgpu/types.h>
#include <nvgpu/pmu/pmuif/ctrlboardobj.h>

/*!
 * Valid global VIN ID values
 */
#define	CTRL_CLK_VIN_ID_SYS		0x00000000U
#define	CTRL_CLK_VIN_ID_LTC		0x00000001U
#define	CTRL_CLK_VIN_ID_XBAR		0x00000002U
#define	CTRL_CLK_VIN_ID_GPC0		0x00000003U
#define	CTRL_CLK_VIN_ID_GPC1		0x00000004U
#define	CTRL_CLK_VIN_ID_GPC2		0x00000005U
#define	CTRL_CLK_VIN_ID_GPC3		0x00000006U
#define	CTRL_CLK_VIN_ID_GPC4		0x00000007U
#define	CTRL_CLK_VIN_ID_GPC5		0x00000008U
#define	CTRL_CLK_VIN_ID_GPCS		0x00000009U
#define	CTRL_CLK_VIN_ID_SRAM		0x0000000AU
#define	CTRL_CLK_VIN_ID_UNDEFINED	0x000000FFU

#define	CTRL_CLK_VIN_TYPE_DISABLED 0x00000000U
#define CTRL_CLK_VIN_TYPE_V20      0x00000002U

/* valid clock domain values */
#define CTRL_CLK_DOMAIN_MCLK                                (0x00000010U)
#define CTRL_CLK_DOMAIN_HOSTCLK                             (0x00000020U)
#define CTRL_CLK_DOMAIN_DISPCLK                             (0x00000040U)
#define CTRL_CLK_DOMAIN_GPC2CLK                             (0x00010000U)
#define CTRL_CLK_DOMAIN_XBAR2CLK                            (0x00040000U)
#define CTRL_CLK_DOMAIN_SYS2CLK                             (0x00800000U)
#define CTRL_CLK_DOMAIN_HUB2CLK                             (0x01000000U)
#define CTRL_CLK_DOMAIN_UTILSCLK                            (0x00040000U)
#define CTRL_CLK_DOMAIN_PWRCLK                              (0x00080000U)
#define CTRL_CLK_DOMAIN_NVDCLK                              (0x00100000U)
#define CTRL_CLK_DOMAIN_PCIEGENCLK                          (0x00200000U)
#define CTRL_CLK_DOMAIN_XCLK                                (0x04000000U)
#define CTRL_CLK_DOMAIN_NVL_COMMON                          (0x08000000U)
#define CTRL_CLK_DOMAIN_PEX_REFCLK                          (0x10000000U)

#define CTRL_CLK_DOMAIN_GPCCLK                              (0x00000001U)
#define CTRL_CLK_DOMAIN_XBARCLK                             (0x00000002U)
#define CTRL_CLK_DOMAIN_SYSCLK                              (0x00000004U)
#define CTRL_CLK_DOMAIN_HUBCLK                              (0x00000008U)


#define CTRL_CLK_FLL_REGIME_ID_INVALID                     ((u8)0x00000000)
#define CTRL_CLK_FLL_REGIME_ID_FFR                         ((u8)0x00000001)
#define CTRL_CLK_FLL_REGIME_ID_FR                          ((u8)0x00000002)

#define CTRL_CLK_CLK_DOMAIN_CLIENT_MAX_DOMAINS			16
#define CLK_CLOCK_MON_DOMAIN_COUNT				0x32U
#define CTRL_CLK_CLK_DELTA_MAX_VOLT_RAILS 4U
/*
 *  Try to get gpc2clk, mclk, sys2clk, xbar2clk work for Pascal
 *
 *  mclk is same for both
 *  gpc2clk is 17 for Pascal and 13 for Volta, making it 17
 *    as volta uses gpcclk
 *  sys2clk is 20 in Pascal and 15 in Volta.
 *    Changing for Pascal would break nvdclk of Volta
 *  xbar2clk is 19 in Pascal and 14 in Volta
 *    Changing for Pascal would break pwrclk of Volta
 */
#define CLKWHICH_GPCCLK		1U
#define CLKWHICH_XBARCLK	2U
#define CLKWHICH_SYSCLK		3U
#define CLKWHICH_HUBCLK		4U
#define CLKWHICH_MCLK		5U
#define CLKWHICH_HOSTCLK	6U
#define CLKWHICH_DISPCLK	7U
#define CLKWHICH_XCLK		12U
#define CLKWHICH_XBAR2CLK	14U
#define CLKWHICH_SYS2CLK	15U
#define CLKWHICH_HUB2CLK	16U
#define CLKWHICH_GPC2CLK	17U
#define CLKWHICH_PWRCLK		19U
#define CLKWHICH_NVDCLK		20U
#define CLKWHICH_PCIEGENCLK	26U


/*!
 * Mask of all GPC VIN IDs supported by RM
 */
#define CTRL_CLK_LUT_NUM_ENTRIES_MAX   (128U)
#define CTRL_CLK_LUT_NUM_ENTRIES_GV10x (128U)
#define CTRL_CLK_LUT_NUM_ENTRIES_GP10x (100U)
#define CTRL_CLK_VIN_STEP_SIZE_UV (6250U)
#define CTRL_CLK_LUT_MIN_VOLTAGE_UV (450000U)
#define CTRL_CLK_FLL_TYPE_DISABLED 0U

#define CTRL_CLK_FLL_LUT_VSELECT_LOGIC			   (0x00000000U)
#define CTRL_CLK_FLL_LUT_VSELECT_MIN			   (0x00000001U)
#define CTRL_CLK_FLL_LUT_VSELECT_SRAM			   (0x00000002U)

#define CTRL_CLK_VIN_SW_OVERRIDE_VIN_USE_HW_REQ  (0x00000000U)
#define CTRL_CLK_VIN_SW_OVERRIDE_VIN_USE_MIN     (0x00000001U)
#define CTRL_CLK_VIN_SW_OVERRIDE_VIN_USE_SW_REQ  (0x00000003U)

struct gk20a;
struct nvgpu_avfsfllobjs;
struct nvgpu_clk_domains;
struct nvgpu_clk_progs;
struct nvgpu_clk_vf_points;
struct nvgpu_clk_mclk_state;
struct nvgpu_clk_slave_freq;
struct ctrl_perf_change_seq_change_input;


struct ctrl_clk_domain_clk_mon_item {
	u32 clk_api_domain;
	u32 clk_freq_Mhz;
	u32 low_threshold_percentage;
	u32 high_threshold_percentage;
};

struct ctrl_clk_domain_clk_mon_list {
	u8 num_domain;
	struct ctrl_clk_domain_clk_mon_item
		clk_domain[CTRL_CLK_CLK_DOMAIN_CLIENT_MAX_DOMAINS];
};

struct ctrl_clk_clk_domain_list_item_v1 {
	u32  clk_domain;
	u32  clk_freq_khz;
	u8   regime_id;
	u8   source;
};

struct ctrl_clk_clk_domain_list {
	u8 num_domains;
	struct ctrl_clk_clk_domain_list_item_v1
		clk_domains[CTRL_BOARDOBJ_MAX_BOARD_OBJECTS];
};

struct clk_domain_mon_status {
	u32 clk_api_domain;
	u32 low_threshold;
	u32 high_threshold;
	u32 clk_domain_fault_status;
};

struct clk_domains_mon_status_params {
	u32 clk_mon_domain_mask;
	struct clk_domain_mon_status
		clk_mon_list[CLK_CLOCK_MON_DOMAIN_COUNT];
};

struct ctrl_clk_vin_sw_override_list_item {
	u8 override_mode;
	u32 voltage_uV;
};

struct ctrl_clk_vin_sw_override_list {
	struct ctrl_boardobjgrp_mask_e32 volt_rails_mask;
	struct ctrl_clk_vin_sw_override_list_item
		volt[4];
};

union ctrl_clk_freq_delta_data {
	s32 delta_khz;
	s16 delta_percent;
};
struct ctrl_clk_freq_delta {
	u8 type;
	union ctrl_clk_freq_delta_data data;
};

struct ctrl_clk_clk_delta {
	struct ctrl_clk_freq_delta freq_delta;
	int volt_deltauv[CTRL_CLK_CLK_DELTA_MAX_VOLT_RAILS];
};

struct nv_pmu_clk_lut_device_desc {
	u8 vselect_mode;
	u16 hysteresis_threshold;
};


struct nv_pmu_clk_regime_desc {
	u8 regime_id;
	u8 target_regime_id_override;
	u16 fixed_freq_regime_limit_mhz;
};

struct ctrl_clk_vf_pair {
	u16 freq_mhz;
	u32 voltage_uv;
};

struct nvgpu_set_fll_clk {
	u32 voltuv;
	u16 gpc2clkmhz;
	u8 current_regime_id_gpc;
	u8 target_regime_id_gpc;
	u16 sys2clkmhz;
	u8 current_regime_id_sys;
	u8 target_regime_id_sys;
	u16 xbar2clkmhz;
	u8 current_regime_id_xbar;
	u8 target_regime_id_xbar;
	u16 nvdclkmhz;
	u8 current_regime_id_nvd;
	u8 target_regime_id_nvd;
	u16 hostclkmhz;
	u8 current_regime_id_host;
	u8 target_regime_id_host;
};

struct nvgpu_clk_pmupstate {
	struct nvgpu_avfsvinobjs *avfs_vinobjs;
	struct nvgpu_avfsfllobjs *avfs_fllobjs;
	struct nvgpu_clk_domains *clk_domainobjs;
	struct nvgpu_clk_progs *clk_progobjs;
	struct nvgpu_clk_vf_points *clk_vf_pointobjs;

	/* clk_domain unit functions */
	int (*get_fll)(struct gk20a *g, struct nvgpu_set_fll_clk *setfllclk);
	void (*set_p0_clks)(struct gk20a *g, u8 *gpcclk_domain,
		u32 *gpcclk_clkmhz, struct nvgpu_clk_slave_freq *vf_point,
		struct ctrl_perf_change_seq_change_input *change_input);
	struct nvgpu_clk_domain *(*clk_get_clk_domain)
			(struct nvgpu_clk_pmupstate *pclk, u8 idx);
	int (*clk_domain_clk_prog_link)(struct gk20a *g,
			struct nvgpu_clk_pmupstate *pclk);

	/* clk_vin unit functions */
	struct nvgpu_vin_device *(*clk_get_vin)
			(struct nvgpu_avfsvinobjs *pvinobjs, u8 idx);

	/* clk_fll unit functions */
	u8 (*find_regime_id)(struct gk20a *g, u32 domain, u16 clkmhz);
	int (*set_regime_id)(struct gk20a *g, u32 domain, u8 regimeid);
	int (*get_regime_id)(struct gk20a *g, u32 domain, u8 *regimeid);
	u8 (*get_fll_lut_vf_num_entries)(struct nvgpu_clk_pmupstate *pclk);
	u32 (*get_fll_lut_min_volt)(struct nvgpu_clk_pmupstate *pclk);
	u32 (*get_fll_lut_step_size)(struct nvgpu_clk_pmupstate *pclk);

	/* clk_vf_point functions */
	int (*nvgpu_clk_vf_point_cache)(struct gk20a *g);
};

int clk_init_pmupstate(struct gk20a *g);
void clk_free_pmupstate(struct gk20a *g);
int nvgpu_clk_get_fll_clks(struct gk20a *g,
		struct nvgpu_set_fll_clk *setfllclk);
int nvgpu_pmu_clk_domain_freq_to_volt(struct gk20a *g, u8 clkdomain_idx,
	u32 *pclkmhz, u32 *pvoltuv, u8 railidx);
int nvgpu_clk_domain_get_from_index(struct gk20a *g, u32 *domain, u32 index);
u32 nvgpu_clk_mon_init_domains(struct gk20a *g);
int nvgpu_pmu_clk_pmu_setup(struct gk20a *g);
int nvgpu_pmu_clk_sw_setup(struct gk20a *g);
int nvgpu_pmu_clk_init(struct gk20a *g);
void nvgpu_pmu_clk_deinit(struct gk20a *g);

#endif /* NVGPU_PMU_CLK_H */
