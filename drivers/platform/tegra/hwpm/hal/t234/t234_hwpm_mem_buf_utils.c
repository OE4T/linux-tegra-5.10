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

#include <linux/kernel.h>
#include <linux/dma-buf.h>
#include <uapi/linux/tegra-soc-hwpm-uapi.h>

#include <tegra_hwpm_log.h>
#include <tegra_hwpm_io.h>
#include <tegra_hwpm.h>
#include <hal/t234/t234_hwpm_internal.h>

#include <hal/t234/hw/t234_pmasys_soc_hwpm.h>
#include <hal/t234/hw/t234_pmmsys_soc_hwpm.h>

int t234_hwpm_disable_mem_mgmt(struct tegra_soc_hwpm *hwpm)
{
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	/* Currently, PMA has only one perfmux */
	hwpm_ip_perfmux *pma_perfmux = &active_chip->chip_ips[
		active_chip->get_pma_int_idx(hwpm)]->perfmux_static_array[0U];

	tegra_hwpm_fn(hwpm, " ");

	tegra_hwpm_writel(hwpm, pma_perfmux, pmasys_channel_outbase_r(0), 0);
	tegra_hwpm_writel(hwpm, pma_perfmux,
		pmasys_channel_outbaseupper_r(0), 0);
	tegra_hwpm_writel(hwpm, pma_perfmux, pmasys_channel_outsize_r(0), 0);
	tegra_hwpm_writel(hwpm, pma_perfmux,
		pmasys_channel_mem_bytes_addr_r(0), 0);

	return 0;
}

int t234_hwpm_enable_mem_mgmt(struct tegra_soc_hwpm *hwpm,
	struct tegra_soc_hwpm_alloc_pma_stream *alloc_pma_stream)
{
	u32 outbase_lo = 0;
	u32 outbase_hi = 0;
	u32 outsize = 0;
	u32 mem_bytes_addr = 0;
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	/* Currently, PMA has only one perfmux */
	hwpm_ip_perfmux *pma_perfmux = &active_chip->chip_ips[
		active_chip->get_pma_int_idx(hwpm)]->perfmux_static_array[0U];

	tegra_hwpm_fn(hwpm, " ");

	outbase_lo = alloc_pma_stream->stream_buf_pma_va &
			pmasys_channel_outbase_ptr_m();
	tegra_hwpm_writel(hwpm, pma_perfmux,
		pmasys_channel_outbase_r(0), outbase_lo);
	tegra_hwpm_dbg(hwpm, hwpm_verbose, "OUTBASE = 0x%x", outbase_lo);

	outbase_hi = (alloc_pma_stream->stream_buf_pma_va >> 32) &
			pmasys_channel_outbaseupper_ptr_m();
	tegra_hwpm_writel(hwpm, pma_perfmux,
		pmasys_channel_outbaseupper_r(0), outbase_hi);
	tegra_hwpm_dbg(hwpm, hwpm_verbose, "OUTBASEUPPER = 0x%x", outbase_hi);

	outsize = alloc_pma_stream->stream_buf_size &
			pmasys_channel_outsize_numbytes_m();
	tegra_hwpm_writel(hwpm, pma_perfmux,
		pmasys_channel_outsize_r(0), outsize);
	tegra_hwpm_dbg(hwpm, hwpm_verbose, "OUTSIZE = 0x%x", outsize);

	mem_bytes_addr = sg_dma_address(hwpm->mem_bytes_sgt->sgl) &
			pmasys_channel_mem_bytes_addr_ptr_m();
	tegra_hwpm_writel(hwpm, pma_perfmux,
		pmasys_channel_mem_bytes_addr_r(0), mem_bytes_addr);
	tegra_hwpm_dbg(hwpm, hwpm_verbose,
		"MEM_BYTES_ADDR = 0x%x", mem_bytes_addr);

	tegra_hwpm_writel(hwpm, pma_perfmux, pmasys_channel_mem_block_r(0),
		pmasys_channel_mem_block_valid_f(
			pmasys_channel_mem_block_valid_true_v()));

	return 0;
}

int t234_hwpm_invalidate_mem_config(struct tegra_soc_hwpm *hwpm)
{
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	/* Currently, PMA has only one perfmux */
	hwpm_ip_perfmux *pma_perfmux = &active_chip->chip_ips[
		active_chip->get_pma_int_idx(hwpm)]->perfmux_static_array[0U];

	tegra_hwpm_fn(hwpm, " ");

	tegra_hwpm_writel(hwpm, pma_perfmux, pmasys_channel_mem_block_r(0),
		pmasys_channel_mem_block_valid_f(
			pmasys_channel_mem_block_valid_false_v()));

	return 0;
}

int t234_hwpm_stream_mem_bytes(struct tegra_soc_hwpm *hwpm)
{
	u32 reg_val = 0U;
	u32 *mem_bytes_kernel_u32 = (u32 *)(hwpm->mem_bytes_kernel);
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	/* Currently, PMA has only one perfmux */
	hwpm_ip_perfmux *pma_perfmux = &active_chip->chip_ips[
		active_chip->get_pma_int_idx(hwpm)]->perfmux_static_array[0U];

	tegra_hwpm_fn(hwpm, " ");

	*mem_bytes_kernel_u32 = TEGRA_SOC_HWPM_MEM_BYTES_INVALID;

	reg_val = tegra_hwpm_readl(hwpm, pma_perfmux,
		pmasys_channel_control_user_r(0));
	reg_val = set_field(reg_val,
		pmasys_channel_control_user_update_bytes_m(),
		pmasys_channel_control_user_update_bytes_doit_f());
	tegra_hwpm_writel(hwpm, pma_perfmux,
		pmasys_channel_control_user_r(0), reg_val);

	return 0;
}

int t234_hwpm_disable_pma_streaming(struct tegra_soc_hwpm *hwpm)
{
	u32 reg_val = 0U;
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	/* Currently, PMA has only one perfmux */
	hwpm_ip_perfmux *pma_perfmux = &active_chip->chip_ips[
		active_chip->get_pma_int_idx(hwpm)]->perfmux_static_array[0U];

	tegra_hwpm_fn(hwpm, " ");

	/* Disable PMA streaming */
	reg_val = tegra_hwpm_readl(hwpm, pma_perfmux,
		pmasys_trigger_config_user_r(0));
	reg_val = set_field(reg_val,
		pmasys_trigger_config_user_record_stream_m(),
		pmasys_trigger_config_user_record_stream_disable_f());
	tegra_hwpm_writel(hwpm, pma_perfmux,
		pmasys_trigger_config_user_r(0), reg_val);

	reg_val = tegra_hwpm_readl(hwpm, pma_perfmux,
		pmasys_channel_control_user_r(0));
	reg_val = set_field(reg_val,
		pmasys_channel_control_user_stream_m(),
		pmasys_channel_control_user_stream_disable_f());
	tegra_hwpm_writel(hwpm, pma_perfmux,
		pmasys_channel_control_user_r(0), reg_val);

	return 0;
}

int t234_hwpm_update_mem_bytes_get_ptr(struct tegra_soc_hwpm *hwpm,
	u64 mem_bump)
{
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	/* Currently, PMA has only one perfmux */
	hwpm_ip_perfmux *pma_perfmux = &active_chip->chip_ips[
		active_chip->get_pma_int_idx(hwpm)]->perfmux_static_array[0U];

	tegra_hwpm_fn(hwpm, " ");

	if (mem_bump > (u64)U32_MAX) {
		tegra_hwpm_err(hwpm, "mem_bump is out of bounds");
		return -EINVAL;
	}

	tegra_hwpm_writel(hwpm, pma_perfmux,
		pmasys_channel_mem_bump_r(0), mem_bump);

	return 0;
}

u64 t234_hwpm_get_mem_bytes_put_ptr(struct tegra_soc_hwpm *hwpm)
{
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	/* Currently, PMA has only one perfmux */
	hwpm_ip_perfmux *pma_perfmux = &active_chip->chip_ips[
		active_chip->get_pma_int_idx(hwpm)]->perfmux_static_array[0U];

	tegra_hwpm_fn(hwpm, " ");

	return (u64)tegra_hwpm_readl(hwpm, pma_perfmux,
			pmasys_channel_mem_head_r(0));
}

bool t234_hwpm_membuf_overflow_status(struct tegra_soc_hwpm *hwpm)
{
	u32 reg_val, field_val;
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	/* Currently, PMA has only one perfmux */
	hwpm_ip_perfmux *pma_perfmux = &active_chip->chip_ips[
		active_chip->get_pma_int_idx(hwpm)]->perfmux_static_array[0U];

	tegra_hwpm_fn(hwpm, " ");

	reg_val = tegra_hwpm_readl(hwpm, pma_perfmux,
		pmasys_channel_status_secure_r(0));
	field_val = pmasys_channel_status_secure_membuf_status_v(
		reg_val);

	return (field_val ==
		pmasys_channel_status_secure_membuf_status_overflowed_v());
}
