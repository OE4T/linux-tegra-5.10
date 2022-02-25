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

#include <uapi/linux/tegra-soc-hwpm-uapi.h>

#include <tegra_hwpm_log.h>
#include <tegra_hwpm.h>
#include <tegra_hwpm_common.h>

int tegra_hwpm_exec_regops(struct tegra_soc_hwpm *hwpm,
	struct tegra_soc_hwpm_exec_reg_ops *exec_reg_ops)
{
	struct tegra_soc_hwpm_chip *active_chip = hwpm->active_chip;
	int op_idx = 0;
	struct tegra_soc_hwpm_reg_op *reg_op = NULL;
	int ret = 0;

	tegra_hwpm_fn(hwpm, " ");

	switch (exec_reg_ops->mode) {
	case TEGRA_SOC_HWPM_REG_OP_MODE_FAIL_ON_FIRST:
	case TEGRA_SOC_HWPM_REG_OP_MODE_CONT_ON_ERR:
		break;

	default:
		tegra_hwpm_err(hwpm, "Invalid reg ops mode(%u)",
				   exec_reg_ops->mode);
		return -EINVAL;
	}

	if (exec_reg_ops->op_count > TEGRA_SOC_HWPM_REG_OPS_SIZE) {
		tegra_hwpm_err(hwpm, "Reg_op count=%d exceeds max count",
				   exec_reg_ops->op_count);
		return -EINVAL;
	}

	if (active_chip->exec_reg_ops == NULL) {
		tegra_hwpm_err(hwpm, "exec_reg_ops uninitialized");
		return -ENODEV;
	}

	/*
	 * Initialize flag to true assuming all regops will pass
	 * If any regop fails, the flag will be reset to false.
	 */
	exec_reg_ops->b_all_reg_ops_passed = true;

	for (op_idx = 0; op_idx < exec_reg_ops->op_count; op_idx++) {
		reg_op = &(exec_reg_ops->ops[op_idx]);
		tegra_hwpm_dbg(hwpm, hwpm_verbose,
			"reg op: idx(%d), phys(0x%llx), cmd(%u)",
			op_idx, reg_op->phys_addr, reg_op->cmd);

		ret = active_chip->exec_reg_ops(hwpm, reg_op);
		if (ret < 0) {
			tegra_hwpm_err(hwpm, "exec_reg_ops %d failed", op_idx);
			exec_reg_ops->b_all_reg_ops_passed = false;
			if (exec_reg_ops->mode ==
				TEGRA_SOC_HWPM_REG_OP_MODE_FAIL_ON_FIRST) {
				return -EINVAL;
			}
		}
	}

	return 0;
}
