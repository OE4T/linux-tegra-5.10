/*
 * Copyright (c) 2021-2022, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <soc/tegra/fuse.h>
#include <linux/of_address.h>
#include <linux/dma-buf.h>
#include <uapi/linux/tegra-soc-hwpm-uapi.h>

#include <tegra_hwpm_log.h>
#include <tegra_hwpm_io.h>
#include <tegra_hwpm.h>
#include <hal/t234/t234_hwpm_internal.h>

#include <hal/t234/hw/t234_pmasys_soc_hwpm.h>
#include <hal/t234/hw/t234_pmmsys_soc_hwpm.h>

int t234_hwpm_disable_triggers(struct tegra_soc_hwpm *hwpm)
{
	int ret = 0;
	bool timeout = false;
	u32 reg_val = 0U;
	u32 field_mask = 0U;
	u32 field_val = 0U;
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	hwpm_ip_perfmux *pma_perfmux = NULL;
	hwpm_ip_perfmux *rtr_perfmux = NULL;

	tegra_hwpm_fn(hwpm, " ");

	/* Currently, PMA has only one perfmux */
	pma_perfmux = &active_chip->chip_ips[
		active_chip->get_pma_int_idx(hwpm)]->perfmux_static_array[0U];
	/* Currently, RTR specific perfmux is added at index 0 */
	rtr_perfmux = &active_chip->chip_ips[
		active_chip->get_rtr_int_idx(hwpm)]->perfmux_static_array[0U];

	/* Disable PMA triggers */
	reg_val = tegra_hwpm_readl(hwpm, pma_perfmux,
		pmasys_trigger_config_user_r(0));
	reg_val = set_field(reg_val, pmasys_trigger_config_user_pma_pulse_m(),
		pmasys_trigger_config_user_pma_pulse_disable_f());
	tegra_hwpm_writel(hwpm, pma_perfmux,
		pmasys_trigger_config_user_r(0), reg_val);

	tegra_hwpm_writel(hwpm, pma_perfmux,
		pmasys_sys_trigger_start_mask_r(), 0);
	tegra_hwpm_writel(hwpm, pma_perfmux,
		pmasys_sys_trigger_start_maskb_r(), 0);
	tegra_hwpm_writel(hwpm, pma_perfmux,
		pmasys_sys_trigger_stop_mask_r(), 0);
	tegra_hwpm_writel(hwpm, pma_perfmux,
		pmasys_sys_trigger_stop_maskb_r(), 0);

	/* Wait for PERFMONs, ROUTER, and PMA to idle */
	timeout = HWPM_TIMEOUT(pmmsys_sys0router_perfmonstatus_merged_v(
		tegra_hwpm_readl(hwpm, rtr_perfmux,
			pmmsys_sys0router_perfmonstatus_r())) == 0U,
		"NV_PERF_PMMSYS_SYS0ROUTER_PERFMONSTATUS_MERGED_EMPTY");
	if (timeout && ret == 0) {
		ret = -EIO;
	}

	timeout = HWPM_TIMEOUT(pmmsys_sys0router_enginestatus_status_v(
		tegra_hwpm_readl(hwpm, rtr_perfmux,
			pmmsys_sys0router_enginestatus_r())) ==
		pmmsys_sys0router_enginestatus_status_empty_v(),
		"NV_PERF_PMMSYS_SYS0ROUTER_ENGINESTATUS_STATUS_EMPTY");
	if (timeout && ret == 0) {
		ret = -EIO;
	}

	field_mask = pmasys_enginestatus_status_m() |
		     pmasys_enginestatus_rbufempty_m();
	field_val = pmasys_enginestatus_status_empty_f() |
		pmasys_enginestatus_rbufempty_empty_f();
	timeout = HWPM_TIMEOUT((tegra_hwpm_readl(hwpm, pma_perfmux,
			pmasys_enginestatus_r()) & field_mask) == field_val,
		"NV_PERF_PMASYS_ENGINESTATUS");
	if (timeout && ret == 0) {
		ret = -EIO;
	}

	return ret;
}

int t234_hwpm_init_prod_values(struct tegra_soc_hwpm *hwpm)
{
	u32 reg_val = 0U;
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	/* Currently, PMA has only one perfmux */
	hwpm_ip_perfmux *pma_perfmux = &active_chip->chip_ips[
		active_chip->get_pma_int_idx(hwpm)]->perfmux_static_array[0U];

	tegra_hwpm_fn(hwpm, " ");

	reg_val = tegra_hwpm_readl(hwpm, pma_perfmux, pmasys_controlb_r());
	reg_val = set_field(reg_val,
			pmasys_controlb_coalesce_timeout_cycles_m(),
			pmasys_controlb_coalesce_timeout_cycles__prod_f());
	tegra_hwpm_writel(hwpm, pma_perfmux, pmasys_controlb_r(), reg_val);

	reg_val = tegra_hwpm_readl(hwpm, pma_perfmux,
		pmasys_channel_config_user_r(0));
	reg_val = set_field(reg_val,
		pmasys_channel_config_user_coalesce_timeout_cycles_m(),
		pmasys_channel_config_user_coalesce_timeout_cycles__prod_f());
	tegra_hwpm_writel(hwpm, pma_perfmux,
		pmasys_channel_config_user_r(0), reg_val);

	return 0;
}

int t234_hwpm_disable_slcg(struct tegra_soc_hwpm *hwpm)
{
	u32 field_mask = 0U;
	u32 field_val = 0U;
	u32 reg_val = 0U;
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	struct hwpm_ip *pma_ip = NULL, *rtr_ip = NULL;
	hwpm_ip_perfmux *pma_perfmux = NULL;
	hwpm_ip_perfmux *rtr_perfmux = NULL;

	tegra_hwpm_fn(hwpm, " ");

	if (active_chip == NULL) {
		return -ENODEV;
	}

	pma_ip = active_chip->chip_ips[active_chip->get_pma_int_idx(hwpm)];
	rtr_ip = active_chip->chip_ips[active_chip->get_rtr_int_idx(hwpm)];

	if ((pma_ip == NULL) || !(pma_ip->reserved)) {
		tegra_hwpm_err(hwpm, "PMA uninitialized");
		return -ENODEV;
	}

	if ((rtr_ip == NULL) || !(rtr_ip->reserved)) {
		tegra_hwpm_err(hwpm, "RTR uninitialized");
		return -ENODEV;
	}

	/* Currently, PMA has only one perfmux */
	pma_perfmux = &pma_ip->perfmux_static_array[0U];
	/* Currently, RTR specific perfmux is added at index 0 */
	rtr_perfmux = &rtr_ip->perfmux_static_array[0U];

	reg_val = tegra_hwpm_readl(hwpm, pma_perfmux, pmasys_cg2_r());
	reg_val = set_field(reg_val, pmasys_cg2_slcg_m(),
			pmasys_cg2_slcg_disabled_f());
	tegra_hwpm_writel(hwpm, pma_perfmux, pmasys_cg2_r(), reg_val);

	field_mask = pmmsys_sys0router_cg2_slcg_perfmon_m() |
		pmmsys_sys0router_cg2_slcg_router_m() |
		pmmsys_sys0router_cg2_slcg_m();
	field_val = pmmsys_sys0router_cg2_slcg_perfmon_disabled_f() |
		pmmsys_sys0router_cg2_slcg_router_disabled_f() |
		pmmsys_sys0router_cg2_slcg_disabled_f();
	reg_val = tegra_hwpm_readl(hwpm, rtr_perfmux,
		pmmsys_sys0router_cg2_r());
	reg_val = set_field(reg_val, field_mask, field_val);
	tegra_hwpm_writel(hwpm, rtr_perfmux,
		pmmsys_sys0router_cg2_r(), reg_val);

	return 0;
}

int t234_hwpm_enable_slcg(struct tegra_soc_hwpm *hwpm)
{
	u32 reg_val = 0U;
	u32 field_mask = 0U;
	u32 field_val = 0U;
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	struct hwpm_ip *pma_ip = NULL, *rtr_ip = NULL;
	hwpm_ip_perfmux *pma_perfmux = NULL;
	hwpm_ip_perfmux *rtr_perfmux = NULL;

	tegra_hwpm_fn(hwpm, " ");

	if (active_chip == NULL) {
		return -ENODEV;
	}

	pma_ip = active_chip->chip_ips[active_chip->get_pma_int_idx(hwpm)];
	rtr_ip = active_chip->chip_ips[active_chip->get_rtr_int_idx(hwpm)];

	if ((pma_ip == NULL) || !(pma_ip->reserved)) {
		tegra_hwpm_err(hwpm, "PMA uninitialized");
		return -ENODEV;
	}

	if ((rtr_ip == NULL) || !(rtr_ip->reserved)) {
		tegra_hwpm_err(hwpm, "RTR uninitialized");
		return -ENODEV;
	}

	/* Currently, PMA has only one perfmux */
	pma_perfmux = &pma_ip->perfmux_static_array[0U];
	/* Currently, RTR specific perfmux is added at index 0 */
	rtr_perfmux = &rtr_ip->perfmux_static_array[0U];

	reg_val = tegra_hwpm_readl(hwpm, pma_perfmux, pmasys_cg2_r());
	reg_val = set_field(reg_val, pmasys_cg2_slcg_m(),
		pmasys_cg2_slcg_enabled_f());
	tegra_hwpm_writel(hwpm, pma_perfmux, pmasys_cg2_r(), reg_val);

	field_mask = pmmsys_sys0router_cg2_slcg_perfmon_m() |
		pmmsys_sys0router_cg2_slcg_router_m() |
		pmmsys_sys0router_cg2_slcg_m();
	field_val = pmmsys_sys0router_cg2_slcg_perfmon__prod_f() |
		pmmsys_sys0router_cg2_slcg_router__prod_f() |
		pmmsys_sys0router_cg2_slcg__prod_f();
	reg_val = tegra_hwpm_readl(hwpm, rtr_perfmux,
		pmmsys_sys0router_cg2_r());
	reg_val = set_field(reg_val, field_mask, field_val);
	tegra_hwpm_writel(hwpm, rtr_perfmux,
		pmmsys_sys0router_cg2_r(), reg_val);

	return 0;
}
