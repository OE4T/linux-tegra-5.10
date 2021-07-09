/*
 * Copyright (c) 2020-2021, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/log.h>
#include <nvgpu/enabled.h>
#include <nvgpu/io.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/soc.h>
#include <os/linux/module.h>
#include <os/linux/os_linux.h>

#define MSS_NVLINK_INTERNAL_NUM                 8U
#define MSS_NVLINK_GLOBAL_CREDIT_CONTROL_0      0x00000010
#define MSS_NVLINK_MCF_MEMORY_TYPE_CONTROL_0    0x00000040

#define MSS_NVLINK_SIZE                         0x00001000
#define MSS_NVLINK_1_BASE                       0x01f20000
#define MSS_NVLINK_2_BASE                       0x01f40000
#define MSS_NVLINK_3_BASE                       0x01f60000
#define MSS_NVLINK_4_BASE                       0x01f80000
#define MSS_NVLINK_5_BASE                       0x01fa0000
#define MSS_NVLINK_6_BASE                       0x01fc0000
#define MSS_NVLINK_7_BASE                       0x01fe0000
#define MSS_NVLINK_8_BASE                       0x01e00000
#define MSS_NVLINK_INIT_CREDITS                 0x00000001U
#define MSS_NVLINK_FORCE_COH_SNP		0x3U

void ga10b_init_nvlink_soc_credits(struct gk20a *g)
{
	u32 i = 0U;
	u32 val = MSS_NVLINK_INIT_CREDITS;
	struct device *dev = dev_from_gk20a(g);

	u32 nvlink_base[MSS_NVLINK_INTERNAL_NUM] = {
		MSS_NVLINK_1_BASE, MSS_NVLINK_2_BASE, MSS_NVLINK_3_BASE,
		MSS_NVLINK_4_BASE, MSS_NVLINK_5_BASE, MSS_NVLINK_6_BASE,
		MSS_NVLINK_7_BASE, MSS_NVLINK_8_BASE
	};

	void __iomem *mssnvlink_control[MSS_NVLINK_INTERNAL_NUM];

	if (nvgpu_platform_is_simulation(g)) {
		nvgpu_log(g, gpu_dbg_info, "simulation platform: "
			"nvlink soc credits not required");
		return;
	}

	if (nvgpu_platform_is_silicon(g)) {
		nvgpu_log(g, gpu_dbg_info,
			"nvlink soc credits init done by bpmp on silicon");
		return;
	}
	/* init nvlink soc credits and force snoop */
	for (i = 0U; i < MSS_NVLINK_INTERNAL_NUM; i++) {
		mssnvlink_control[i] = nvgpu_devm_ioremap(dev,
			nvlink_base[i], MSS_NVLINK_SIZE);
	}

	nvgpu_log(g, gpu_dbg_info, "init nvlink soc credits");

	for (i = 0U; i < MSS_NVLINK_INTERNAL_NUM; i++) {
		writel_relaxed(val, (*(mssnvlink_control + i) +
				 MSS_NVLINK_GLOBAL_CREDIT_CONTROL_0));
	}

	/*
	 * Set force snoop, always snoop all nvlink memory transactions
	 * (both coherent and non-coherent)
	 */
	nvgpu_log(g, gpu_dbg_info, "set force snoop");

	for (i = 0U; i < MSS_NVLINK_INTERNAL_NUM; i++) {
		val = readl_relaxed((*(mssnvlink_control + i) +
			MSS_NVLINK_MCF_MEMORY_TYPE_CONTROL_0));
		val &= ~(MSS_NVLINK_FORCE_COH_SNP);
		val |= MSS_NVLINK_FORCE_COH_SNP;
		writel_relaxed(val, *(mssnvlink_control + i) +
			MSS_NVLINK_MCF_MEMORY_TYPE_CONTROL_0);
	}
}
