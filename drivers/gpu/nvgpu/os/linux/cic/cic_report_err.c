/*
 * Copyright (c) 2021-2022, NVIDIA Corporation.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <nvgpu/cic_mon.h>
#include <nvgpu/types.h>

#ifdef CONFIG_NVGPU_ENABLE_MISC_EC
#include <linux/tegra-epl.h>
#include "os/linux/os_linux.h"
#endif

struct gk20a;

int nvgpu_cic_mon_report_err_safety_services(struct gk20a *g, u32 err_id)
{
	int ret = 0U;

#ifdef CONFIG_NVGPU_ENABLE_MISC_EC
	struct device *dev = dev_from_gk20a(g);

	/**
	 * MISC_EC_SW_ERR_CODE_0 register has been allocated for NvGPU
	 * to report GPU HW errors to Safety_Services via MISC_EC interface.
	 */
	ret = epl_report_misc_ec_error(dev, MISC_EC_SW_ERR_CODE_0, err_id);
	if (ret != 0) {
		nvgpu_err(g, "Error reporting to Safety_Services failed");
		nvgpu_err(g, "ret (%d). err (0x%x)", ret, err_id);
	} else {
		nvgpu_err(g, "Reported err (0x%x) to Safety_Services",
					err_id);
	}
#endif

	return ret;
}
