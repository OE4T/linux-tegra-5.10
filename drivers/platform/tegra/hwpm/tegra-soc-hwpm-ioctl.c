/*
 * tegra-soc-hwpm-ioctl.c:
 * This file adds IOCTL handlers for the Tegra SOC HWPM driver.
 *
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
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

#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/of_address.h>
/* FIXME: Is this include needed for struct resource? */
#if 0
#include <linux/ioport.h>
#endif
#include <linux/mm.h>
#include <linux/vmalloc.h>

#include <uapi/linux/tegra-soc-hwpm-uapi.h>

#include "tegra-soc-hwpm.h"
#include "tegra-soc-hwpm-io.h"

struct tegra_soc_hwpm_ioctl {
	const char *const name;
	const size_t struct_size;
	int (*handler)(struct tegra_soc_hwpm *, void *);
};

static int device_info_ioctl(struct tegra_soc_hwpm *hwpm,
			     void *ioctl_struct);
static int timer_relation_ioctl(struct tegra_soc_hwpm *hwpm,
				void *ioctl_struct);
static int reserve_resource_ioctl(struct tegra_soc_hwpm *hwpm,
				  void *ioctl_struct);
static int alloc_pma_stream_ioctl(struct tegra_soc_hwpm *hwpm,
				  void *ioctl_struct);
static int bind_ioctl(struct tegra_soc_hwpm *hwpm,
		      void *ioctl_struct);
static int query_whitelist_ioctl(struct tegra_soc_hwpm *hwpm,
				 void *ioctl_struct);
static int exec_reg_ops_ioctl(struct tegra_soc_hwpm *hwpm,
			      void *ioctl_struct);
static int update_get_put_ioctl(struct tegra_soc_hwpm *hwpm,
				void *ioctl_struct);

static const struct tegra_soc_hwpm_ioctl ioctls[] = {
	[TEGRA_SOC_HWPM_IOCTL_DEVICE_INFO] = {
		.name			= "device_info",
		.struct_size		= sizeof(struct tegra_soc_hwpm_device_info),
		.handler		= device_info_ioctl,
	},
	[TEGRA_SOC_HWPM_IOCTL_GET_GPU_CPU_TIME_CORRELATION_INFO] = {
		.name			= "timer_relation",
		.struct_size		= sizeof(struct tegra_soc_hwpm_timer_relation),
		.handler		= timer_relation_ioctl,
	},
	[TEGRA_SOC_HWPM_IOCTL_RESERVE_RESOURCE] = {
		.name			= "reserve_resource",
		.struct_size		= sizeof(struct tegra_soc_hwpm_reserve_resource),
		.handler		= reserve_resource_ioctl,
	},
	[TEGRA_SOC_HWPM_IOCTL_ALLOC_PMA_STREAM] = {
		.name			= "alloc_pma_stream",
		.struct_size		= sizeof(struct tegra_soc_hwpm_alloc_pma_stream),
		.handler		= alloc_pma_stream_ioctl,
	},
	[TEGRA_SOC_HWPM_IOCTL_BIND] = {
		.name			= "bind",
		.struct_size		= 0,
		.handler		= bind_ioctl,
	},
	[TEGRA_SOC_HWPM_IOCTL_QUERY_WHITELIST] = {
		.name			= "query_whitelist",
		.struct_size		= sizeof(struct tegra_soc_hwpm_query_whitelist),
		.handler		= query_whitelist_ioctl,
	},
	[TEGRA_SOC_HWPM_IOCTL_EXEC_REG_OPS] = {
		.name			= "exec_reg_ops",
		.struct_size		= sizeof(struct tegra_soc_hwpm_exec_reg_ops),
		.handler		= exec_reg_ops_ioctl,
	},
	[TEGRA_SOC_HWPM_IOCTL_UPDATE_GET_PUT] = {
		.name			= "update_get_put",
		.struct_size		= sizeof(struct tegra_soc_hwpm_update_get_put),
		.handler		= update_get_put_ioctl,
	},
};

static int device_info_ioctl(struct tegra_soc_hwpm *hwpm,
			     void *ioctl_struct)
{
/* FIXME: Implement IOCTL */
#if 0
	struct tegra_soc_hwpm_device_info *device_info =
			(struct tegra_soc_hwpm_device_info *)ioctl_struct;
#endif

	tegra_soc_hwpm_err("The DEVICE_INFO IOCTL is currently not implemented");
	return -ENXIO;
}

static int timer_relation_ioctl(struct tegra_soc_hwpm *hwpm,
				void *ioctl_struct)
{
/* FIXME: Implement IOCTL */
#if 0
	struct tegra_soc_hwpm_timer_relation *timer_relation =
			(struct tegra_soc_hwpm_timer_relation *)ioctl_struct;
#endif

	tegra_soc_hwpm_err("The GET_GPU_CPU_TIME_CORRELATION_INFO IOCTL is"
			   " currently not implemented");
	return -ENXIO;

}

static u32 **get_mc_fake_regs(struct tegra_soc_hwpm *hwpm,
			      struct hwpm_resource_aperture *aperture)
{
	if (!hwpm->fake_registers_enabled)
		return NULL;
	if (!aperture) {
		tegra_soc_hwpm_err("aperture is NULL");
		return NULL;
	}

	switch (aperture->start_pa) {
	case NV_ADDRESS_MAP_MC0_BASE:
		return &mc_fake_regs[0];
	case NV_ADDRESS_MAP_MC1_BASE:
		return &mc_fake_regs[1];
	case NV_ADDRESS_MAP_MC2_BASE:
		return &mc_fake_regs[2];
	case NV_ADDRESS_MAP_MC3_BASE:
		return &mc_fake_regs[3];
	case NV_ADDRESS_MAP_MC4_BASE:
		return &mc_fake_regs[4];
	case NV_ADDRESS_MAP_MC5_BASE:
		return &mc_fake_regs[5];
	case NV_ADDRESS_MAP_MC6_BASE:
		return &mc_fake_regs[6];
	case NV_ADDRESS_MAP_MC7_BASE:
		return &mc_fake_regs[7];
	case NV_ADDRESS_MAP_MC8_BASE:
		return &mc_fake_regs[8];
	case NV_ADDRESS_MAP_MC9_BASE:
		return &mc_fake_regs[9];
	case NV_ADDRESS_MAP_MC10_BASE:
		return &mc_fake_regs[10];
	case NV_ADDRESS_MAP_MC11_BASE:
		return &mc_fake_regs[11];
	case NV_ADDRESS_MAP_MC12_BASE:
		return &mc_fake_regs[12];
	case NV_ADDRESS_MAP_MC13_BASE:
		return &mc_fake_regs[13];
	case NV_ADDRESS_MAP_MC14_BASE:
		return &mc_fake_regs[14];
	case NV_ADDRESS_MAP_MC15_BASE:
		return &mc_fake_regs[15];
	default:
		return NULL;
	}
}

static void set_mc_fake_regs(struct tegra_soc_hwpm *hwpm,
			     struct hwpm_resource_aperture *aperture,
			     bool set_null)
{
	u32 *fake_regs = NULL;

	if (!aperture) {
		tegra_soc_hwpm_err("aperture is NULL");
		return;
	}

	switch (aperture->start_pa) {
	case NV_ADDRESS_MAP_MC0_BASE:
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[0];
		mss_channel_map[0].fake_registers = fake_regs;
		mss_iso_niso_hub_map[0].fake_registers = fake_regs;
		mss_mcf_map[0].fake_registers = fake_regs;
		break;
	case NV_ADDRESS_MAP_MC1_BASE:
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[1];
		mss_channel_map[1].fake_registers = fake_regs;
		mss_iso_niso_hub_map[1].fake_registers = fake_regs;
		mss_mcf_map[1].fake_registers = fake_regs;
		break;
	case NV_ADDRESS_MAP_MC2_BASE:
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[2];
		mss_channel_map[2].fake_registers = fake_regs;
		mss_iso_niso_hub_map[2].fake_registers = fake_regs;
		mss_mcf_map[2].fake_registers = fake_regs;
		break;
	case NV_ADDRESS_MAP_MC3_BASE:
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[3];
		mss_channel_map[3].fake_registers = fake_regs;
		mss_iso_niso_hub_map[3].fake_registers = fake_regs;
		mss_mcf_map[3].fake_registers = fake_regs;
		break;
	case NV_ADDRESS_MAP_MC4_BASE:
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[4];
		mss_channel_map[4].fake_registers = fake_regs;
		mss_iso_niso_hub_map[4].fake_registers = fake_regs;
		mss_mcf_map[4].fake_registers = fake_regs;
		break;
	case NV_ADDRESS_MAP_MC5_BASE:
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[5];
		mss_channel_map[5].fake_registers = fake_regs;
		mss_iso_niso_hub_map[5].fake_registers = fake_regs;
		mss_mcf_map[5].fake_registers = fake_regs;
		break;
	case NV_ADDRESS_MAP_MC6_BASE:
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[6];
		mss_channel_map[6].fake_registers = fake_regs;
		mss_iso_niso_hub_map[6].fake_registers = fake_regs;
		mss_mcf_map[6].fake_registers = fake_regs;
		break;
	case NV_ADDRESS_MAP_MC7_BASE:
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[7];
		mss_channel_map[7].fake_registers = fake_regs;
		mss_iso_niso_hub_map[7].fake_registers = fake_regs;
		mss_mcf_map[7].fake_registers = fake_regs;
		break;
	case NV_ADDRESS_MAP_MC8_BASE:
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[8];
		mss_channel_map[8].fake_registers = fake_regs;
		mss_iso_niso_hub_map[8].fake_registers = fake_regs;
		break;
	case NV_ADDRESS_MAP_MC9_BASE:
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[9];
		mss_channel_map[9].fake_registers = fake_regs;
		break;
	case NV_ADDRESS_MAP_MC10_BASE:
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[10];
		mss_channel_map[10].fake_registers = fake_regs;
		break;
	case NV_ADDRESS_MAP_MC11_BASE:
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[11];
		mss_channel_map[11].fake_registers = fake_regs;
		break;
	case NV_ADDRESS_MAP_MC12_BASE:
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[12];
		mss_channel_map[12].fake_registers = fake_regs;
		break;
	case NV_ADDRESS_MAP_MC13_BASE:
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[13];
		mss_channel_map[13].fake_registers = fake_regs;
		break;
	case NV_ADDRESS_MAP_MC14_BASE:
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[14];
		mss_channel_map[14].fake_registers = fake_regs;
		break;
	case NV_ADDRESS_MAP_MC15_BASE:
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[15];
		mss_channel_map[15].fake_registers = fake_regs;
		break;
	default:
		break;
	}
}

static int reserve_resource_ioctl(struct tegra_soc_hwpm *hwpm,
				  void *ioctl_struct)
{
	int ret = 0;
	struct tegra_soc_hwpm_reserve_resource *reserve_resource =
			(struct tegra_soc_hwpm_reserve_resource *)ioctl_struct;
	u32 resource = reserve_resource->resource;
	struct hwpm_resource_aperture *aperture = NULL;
	int aprt_idx = 0;

	if (hwpm->bind_completed) {
		tegra_soc_hwpm_err("The RESERVE_RESOURCE IOCTL can only be"
				" called before the BIND IOCTL.");
		return -EPERM;
	}

	/*
	 * FIXME: Tell IPs which are being profiled to power up IP and
	 * disable power management
	 */
	/* Map reserved apertures and allocate fake register arrays if needed */
	for (aprt_idx = 0;
	     aprt_idx < hwpm_resources[resource].map_size;
	     aprt_idx++) {
		aperture = &(hwpm_resources[resource].map[aprt_idx]);
		if ((aperture->dt_aperture == TEGRA_SOC_HWPM_PMA_DT) ||
		    (aperture->dt_aperture == TEGRA_SOC_HWPM_RTR_DT)) {
			/* PMA and RTR apertures are handled in open(fd) */
			continue;
		} else if (IS_PERFMON(aperture->dt_aperture)) {
			struct resource *res = NULL;
			u64 num_regs = 0;

			tegra_soc_hwpm_dbg("Found PERFMON(0x%llx - 0x%llx)",
				aperture->start_pa, aperture->end_pa);

			hwpm->dt_apertures[aperture->dt_aperture] =
					of_iomap(hwpm->np, aperture->dt_aperture);
			if (!hwpm->dt_apertures[aperture->dt_aperture]) {
				tegra_soc_hwpm_err("Couldn't map PERFMON(%d)",
					aperture->dt_aperture);
				ret = -ENOMEM;
				goto fail;
			}

			res = platform_get_resource(hwpm->pdev,
							IORESOURCE_MEM,
							aperture->dt_aperture);
			if ((!res) || (res->start == 0) || (res->end == 0)) {
				tegra_soc_hwpm_err("Invalid resource for PERFMON(%d)",
					aperture->dt_aperture);
				ret = -ENOMEM;
				goto fail;
			}
			aperture->start_pa = res->start;
			aperture->end_pa = res->end;

			if (hwpm->fake_registers_enabled) {
				num_regs = (aperture->end_pa + 1 - aperture->start_pa) /
					sizeof(*aperture->fake_registers);
				aperture->fake_registers =
					(u32 *)kzalloc(sizeof(*aperture->fake_registers) *
										num_regs,
							GFP_KERNEL);
				if (!aperture->fake_registers) {
					tegra_soc_hwpm_err("Aperture(0x%llx - 0x%llx):"
						" Couldn't allocate memory for fake"
						" registers",
						aperture->start_pa,
						aperture->end_pa);
					ret = -ENOMEM;
					goto fail;
				}
			}
		} else { /* IP apertures */
			if (hwpm->fake_registers_enabled) {
				u64 num_regs = 0;
				u32 **fake_regs = get_mc_fake_regs(hwpm, aperture);
				if (!fake_regs)
					fake_regs = &aperture->fake_registers;

				num_regs = (aperture->end_pa + 1 - aperture->start_pa) /
					sizeof(*(*fake_regs));
				*fake_regs =
					(u32 *)kzalloc(sizeof(*(*fake_regs)) * num_regs,
							GFP_KERNEL);
				if (!(*fake_regs)) {
					tegra_soc_hwpm_err("Aperture(0x%llx - 0x%llx):"
						" Couldn't allocate memory for fake"
						" registers",
						aperture->start_pa,
						aperture->end_pa);
					ret = -ENOMEM;
					goto fail;
				}

				set_mc_fake_regs(hwpm, aperture, false);
			}
		}
	}

	hwpm_resources[resource].reserved = true;
	goto success;

fail:
	for (aprt_idx = 0;
	     aprt_idx < hwpm_resources[resource].map_size;
	     aprt_idx++) {
		aperture = &(hwpm_resources[resource].map[aprt_idx]);
		if ((aperture->dt_aperture == TEGRA_SOC_HWPM_PMA_DT) ||
		    (aperture->dt_aperture == TEGRA_SOC_HWPM_RTR_DT)) {
			/* PMA and RTR apertures are handled in open(fd) */
			continue;
		} else if (IS_PERFMON(aperture->dt_aperture)) {
			if (hwpm->dt_apertures[aperture->dt_aperture]) {
				iounmap(hwpm->dt_apertures[aperture->dt_aperture]);
				hwpm->dt_apertures[aperture->dt_aperture] = NULL;
			}

			aperture->start_pa = 0;
			aperture->end_pa = 0;

			if (aperture->fake_registers) {
				kfree(aperture->fake_registers);
				aperture->fake_registers = NULL;
			}
		} else { /* IP apertures */
			if (aperture->fake_registers) {
				kfree(aperture->fake_registers);
				aperture->fake_registers = NULL;
				set_mc_fake_regs(hwpm, aperture, true);
			}
		}
	}

	hwpm_resources[resource].reserved = false;

success:
	return ret;

}

static int alloc_pma_stream_ioctl(struct tegra_soc_hwpm *hwpm,
				  void *ioctl_struct)
{
	int ret = 0;
	u32 reg_val = 0;
	u32 outbase_lo = 0;
	u32 outbase_hi = 0;
	u32 outsize = 0;
	u32 mem_bytes_addr = 0;
	struct tegra_soc_hwpm_alloc_pma_stream *alloc_pma_stream =
			(struct tegra_soc_hwpm_alloc_pma_stream *)ioctl_struct;

	if (hwpm->bind_completed) {
		tegra_soc_hwpm_err("The ALLOC_PMA_STREAM IOCTL can only be"
				" called before the BIND IOCTL.");
		return -EPERM;
	}

	if (alloc_pma_stream->stream_buf_size == 0) {
		tegra_soc_hwpm_err("stream_buf_size is 0");
		return -EINVAL;
	}
	if (alloc_pma_stream->stream_buf_fd == 0) {
		tegra_soc_hwpm_err("Invalid stream_buf_fd");
		return -EINVAL;
	}
	if (alloc_pma_stream->mem_bytes_buf_fd == 0) {
		tegra_soc_hwpm_err("Invalid mem_bytes_buf_fd");
		return -EINVAL;
	}

	/* Memory map stream buffer */
	hwpm->stream_dma_buf = dma_buf_get(alloc_pma_stream->stream_buf_fd);
	if (IS_ERR(hwpm->stream_dma_buf)) {
		tegra_soc_hwpm_err("Unable to get stream dma_buf");
		ret = PTR_ERR(hwpm->stream_dma_buf);
		goto fail;
	}
	hwpm->stream_attach = dma_buf_attach(hwpm->stream_dma_buf, hwpm->dev);
	if (IS_ERR(hwpm->stream_attach)) {
		tegra_soc_hwpm_err("Unable to attach stream dma_buf");
		ret = PTR_ERR(hwpm->stream_attach);
		goto fail;
	}
	hwpm->stream_sgt = dma_buf_map_attachment(hwpm->stream_attach,
						  DMA_FROM_DEVICE);
	if (IS_ERR(hwpm->stream_sgt)) {
		tegra_soc_hwpm_err("Unable to map stream attachment");
		ret = PTR_ERR(hwpm->stream_sgt);
		goto fail;
	}
	alloc_pma_stream->stream_buf_pma_va =
					sg_dma_address(hwpm->stream_sgt->sgl);
	if (alloc_pma_stream->stream_buf_pma_va == 0) {
		tegra_soc_hwpm_err("Invalid stream buffer SMMU IOVA");
		ret = -ENXIO;
		goto fail;
	}
	tegra_soc_hwpm_dbg("stream_buf_pma_va = 0x%llx",
			   alloc_pma_stream->stream_buf_pma_va);

	/* Memory map mem bytes buffer */
	hwpm->mem_bytes_dma_buf =
				dma_buf_get(alloc_pma_stream->mem_bytes_buf_fd);
	if (IS_ERR(hwpm->mem_bytes_dma_buf)) {
		tegra_soc_hwpm_err("Unable to get mem bytes dma_buf");
		ret = PTR_ERR(hwpm->mem_bytes_dma_buf);
		goto fail;
	}
	hwpm->mem_bytes_attach = dma_buf_attach(hwpm->mem_bytes_dma_buf,
						hwpm->dev);
	if (IS_ERR(hwpm->mem_bytes_attach)) {
		tegra_soc_hwpm_err("Unable to attach mem bytes dma_buf");
		ret = PTR_ERR(hwpm->mem_bytes_attach);
		goto fail;
	}
	hwpm->mem_bytes_sgt = dma_buf_map_attachment(hwpm->mem_bytes_attach,
						     DMA_FROM_DEVICE);
	if (IS_ERR(hwpm->mem_bytes_sgt)) {
		tegra_soc_hwpm_err("Unable to map mem bytes attachment");
		ret = PTR_ERR(hwpm->mem_bytes_sgt);
		goto fail;
	}
	hwpm->mem_bytes_kernel = dma_buf_vmap(hwpm->mem_bytes_dma_buf);
	if (!hwpm->mem_bytes_kernel) {
		tegra_soc_hwpm_err("Unable to map mem_bytes buffer into kernel VA space");
		ret = -ENOMEM;
		goto fail;
	}
	memset(hwpm->mem_bytes_kernel, 0, 32);

	outbase_lo = alloc_pma_stream->stream_buf_pma_va & 0xffffffffULL;
	outbase_lo >>= NV_PERF_PMASYS_CHANNEL_OUTBASE_PTR_SHIFT;
	reg_val = HWPM_REG_F(NV_PERF_PMASYS_CHANNEL_OUTBASE_PTR,
			     outbase_lo);
	hwpm_writel(hwpm,
		    TEGRA_SOC_HWPM_PMA_DT,
		    NV_PERF_PMASYS_CHANNEL_OUTBASE_CH0,
		    reg_val);
	tegra_soc_hwpm_dbg("OUTBASE = 0x%x", reg_val);

	outbase_hi = (alloc_pma_stream->stream_buf_pma_va >> 32) & 0xff;
	outbase_hi >>= NV_PERF_PMASYS_CHANNEL_OUTBASEUPPER_PTR_SHIFT;
	reg_val = HWPM_REG_F(NV_PERF_PMASYS_CHANNEL_OUTBASEUPPER_PTR,
			     outbase_hi);
	hwpm_writel(hwpm,
		    TEGRA_SOC_HWPM_PMA_DT,
		    NV_PERF_PMASYS_CHANNEL_OUTBASEUPPER_CH0,
		    reg_val);
	tegra_soc_hwpm_dbg("OUTBASEUPPER = 0x%x", reg_val);

	outsize = alloc_pma_stream->stream_buf_size >>
				NV_PERF_PMASYS_CHANNEL_OUTSIZE_NUMBYTES_SHIFT;
	reg_val = HWPM_REG_F(NV_PERF_PMASYS_CHANNEL_OUTSIZE_NUMBYTES,
			     outsize);
	hwpm_writel(hwpm,
		    TEGRA_SOC_HWPM_PMA_DT,
		    NV_PERF_PMASYS_CHANNEL_OUTSIZE_CH0,
		    reg_val);
	tegra_soc_hwpm_dbg("OUTSIZE = 0x%x", reg_val);

	mem_bytes_addr = sg_dma_address(hwpm->mem_bytes_sgt->sgl) & 0xffffffffULL;
	mem_bytes_addr >>= NV_PERF_PMASYS_CHANNEL_MEM_BYTES_ADDR_PTR_SHIFT;
	reg_val = HWPM_REG_F(NV_PERF_PMASYS_CHANNEL_MEM_BYTES_ADDR_PTR,
			     mem_bytes_addr);
	hwpm_writel(hwpm,
		    TEGRA_SOC_HWPM_PMA_DT,
		    NV_PERF_PMASYS_CHANNEL_MEM_BYTES_ADDR_CH0,
		    reg_val);
	tegra_soc_hwpm_dbg("MEM_BYTES_ADDR = 0x%x", reg_val);

	reg_val = HWPM_REG_F(NV_PERF_PMASYS_CHANNEL_MEM_BLOCK_VALID,
			     NV_PERF_PMASYS_CHANNEL_MEM_BLOCK_VALID_TRUE);
	hwpm_writel(hwpm,
		    TEGRA_SOC_HWPM_PMA_DT,
		    NV_PERF_PMASYS_CHANNEL_MEM_BLOCK_CH0,
		    reg_val);

	return 0;

fail:
	reg_val = HWPM_REG_F(NV_PERF_PMASYS_CHANNEL_MEM_BLOCK_VALID,
				NV_PERF_PMASYS_CHANNEL_MEM_BLOCK_VALID_FALSE);
	hwpm_writel(hwpm,
		    TEGRA_SOC_HWPM_PMA_DT,
		    NV_PERF_PMASYS_CHANNEL_MEM_BLOCK_CH0,
		    reg_val);
	hwpm_writel(hwpm,
		    TEGRA_SOC_HWPM_PMA_DT,
		    NV_PERF_PMASYS_CHANNEL_OUTBASE_CH0,
		    0);
	hwpm_writel(hwpm,
		    TEGRA_SOC_HWPM_PMA_DT,
		    NV_PERF_PMASYS_CHANNEL_OUTBASEUPPER_CH0,
		    0);
	hwpm_writel(hwpm,
		    TEGRA_SOC_HWPM_PMA_DT,
		    NV_PERF_PMASYS_CHANNEL_OUTSIZE_CH0,
		    0);
	hwpm_writel(hwpm,
		    TEGRA_SOC_HWPM_PMA_DT,
		    NV_PERF_PMASYS_CHANNEL_MEM_BYTES_ADDR_CH0,
		    0);

	alloc_pma_stream->stream_buf_pma_va = 0;

	if (hwpm->stream_sgt && (!IS_ERR(hwpm->stream_sgt))) {
		dma_buf_unmap_attachment(hwpm->stream_attach,
					 hwpm->stream_sgt,
					 DMA_FROM_DEVICE);
	}
	hwpm->stream_sgt = NULL;

	if (hwpm->stream_attach && (!IS_ERR(hwpm->stream_attach))) {
		dma_buf_detach(hwpm->stream_dma_buf, hwpm->stream_attach);
	}
	hwpm->stream_attach = NULL;

	if (hwpm->stream_dma_buf && (!IS_ERR(hwpm->stream_dma_buf))) {
		dma_buf_put(hwpm->stream_dma_buf);
	}
	hwpm->stream_dma_buf = NULL;

	if (hwpm->mem_bytes_kernel) {
		dma_buf_vunmap(hwpm->mem_bytes_dma_buf,
			       hwpm->mem_bytes_kernel);
		hwpm->mem_bytes_kernel = NULL;
	}
	if (hwpm->mem_bytes_sgt && (!IS_ERR(hwpm->mem_bytes_sgt))) {
		dma_buf_unmap_attachment(hwpm->mem_bytes_attach,
					 hwpm->mem_bytes_sgt,
					 DMA_FROM_DEVICE);
	}
	hwpm->mem_bytes_sgt = NULL;

	if (hwpm->mem_bytes_attach && (!IS_ERR(hwpm->mem_bytes_attach))) {
		dma_buf_detach(hwpm->mem_bytes_dma_buf, hwpm->mem_bytes_attach);
	}
	hwpm->mem_bytes_attach = NULL;

	if (hwpm->mem_bytes_dma_buf && (!IS_ERR(hwpm->mem_bytes_dma_buf))) {
		dma_buf_put(hwpm->mem_bytes_dma_buf);
	}
	hwpm->mem_bytes_dma_buf = NULL;

	return ret;
}

static int bind_ioctl(struct tegra_soc_hwpm *hwpm,
		      void *ioctl_struct)
{
	int ret = 0;
	int res_idx = 0;
	int aprt_idx = 0;
	u32 wlist_idx = 0;
	struct hwpm_resource_aperture *aperture = NULL;

	for (res_idx = 0; res_idx < TERGA_SOC_HWPM_NUM_RESOURCES; res_idx++) {
		if (!hwpm_resources[res_idx].reserved)
			continue;
		tegra_soc_hwpm_dbg("Found reserved IP(%d)", res_idx);

		for (aprt_idx = 0;
		     aprt_idx < hwpm_resources[res_idx].map_size;
		     aprt_idx++) {
			aperture = &(hwpm_resources[res_idx].map[aprt_idx]);

			/* Zero out necessary registers */
			if (aperture->wlist) {
				for (wlist_idx = 0;
				     wlist_idx < aperture->wlist_size;
				     wlist_idx++) {
					if (aperture->wlist[wlist_idx].zero_in_init) {
						ioctl_writel(hwpm,
							aperture,
							aperture->start_pa +
								aperture->wlist[wlist_idx].reg,
							0);
					}
				}
			} else {
				tegra_soc_hwpm_err("NULL whitelist in aperture(0x%llx - 0x%llx)",
						   aperture->start_pa,
						   aperture->end_pa);
			}

			/*
			 * Enable reporting of PERFMON status to
			 * NV_PERF_PMMSYS_SYS0ROUTER_PERFMONSTATUS_MERGED
			 */
			if (IS_PERFMON(aperture->dt_aperture)) {
				tegra_soc_hwpm_dbg("Found PERFMON(0x%llx - 0x%llx)",
						   aperture->start_pa,
						   aperture->end_pa);
				ret = DRIVER_REG_RMW(hwpm,
					aperture->dt_aperture,
					NV_PERF_PMMSYS_SYS0_ENGINESTATUS,
					NV_PERF_PMMSYS_SYS0_ENGINESTATUS_ENABLE,
					NV_PERF_PMMSYS_SYS0_ENGINESTATUS_ENABLE_OUT,
					false);
				if (ret < 0) {
					tegra_soc_hwpm_err("Unable to set PMM ENGINESTATUS_ENABLE"
						" for PERFMON(0x%llx - 0x%llx)",
						aperture->start_pa,
						aperture->end_pa);
					return -EIO;
				}
			}
		}
	}

	hwpm->bind_completed = true;
	return 0;
}

static int query_whitelist_ioctl(struct tegra_soc_hwpm *hwpm,
				 void *ioctl_struct)
{
	int ret = 0;
	int res_idx = 0;
	int aprt_idx = 0;
	struct hwpm_resource_aperture *aperture = NULL;
	struct tegra_soc_hwpm_query_whitelist *query_whitelist =
			(struct tegra_soc_hwpm_query_whitelist *)ioctl_struct;

	if (!hwpm->bind_completed) {
		tegra_soc_hwpm_err("The QUERY_WHITELIST IOCTL can only be called"
				   " after the BIND IOCTL.");
		return -EPERM;
	}

	if (!query_whitelist->whitelist) { /* Return whitelist_size */
		if (hwpm->full_wlist_size >= 0) {
			query_whitelist->whitelist_size = hwpm->full_wlist_size;
			return 0;
		}

		hwpm->full_wlist_size = 0;
		for (res_idx = 0; res_idx < TERGA_SOC_HWPM_NUM_RESOURCES; res_idx++) {
			if (!(hwpm_resources[res_idx].reserved))
				continue;
			tegra_soc_hwpm_dbg("Found reserved IP(%d)", res_idx);

			for (aprt_idx = 0;
			     aprt_idx < hwpm_resources[res_idx].map_size;
			     aprt_idx++) {
				aperture = &(hwpm_resources[res_idx].map[aprt_idx]);
				if (aperture->wlist) {
					hwpm->full_wlist_size += aperture->wlist_size;
				} else {
					tegra_soc_hwpm_err("NULL whitelist in aperture(0x%llx - 0x%llx)",
							   aperture->start_pa,
							   aperture->end_pa);
				}

			}
		}

		query_whitelist->whitelist_size = hwpm->full_wlist_size;
	} else { /* Fill in whitelist array */
		unsigned long user_va =
				(unsigned long)(query_whitelist->whitelist);
		unsigned long offset = user_va & ~PAGE_MASK;
		u64 wlist_buf_size = 0;
		u64 num_pages = 0;
		long pinned_pages = 0;
		struct page **pages = NULL;
		long page_idx = 0;
		void *full_wlist = NULL;
		u64 *full_wlist_u64 = NULL;
		u32 full_wlist_idx = 0;
		u32 aprt_wlist_idx = 0;

		if (hwpm->full_wlist_size < 0) {
			tegra_soc_hwpm_err("Invalid whitelist size");
			return -EINVAL;
		}
		wlist_buf_size = hwpm->full_wlist_size *
					sizeof(*(query_whitelist->whitelist));

		/* Memory map user buffer into kernel address space */
		num_pages = DIV_ROUND_UP(offset + wlist_buf_size, PAGE_SIZE);
		pages = (struct page **)kzalloc(sizeof(*pages) * num_pages,
						GFP_KERNEL);
		if (!pages) {
			tegra_soc_hwpm_err("Couldn't allocate memory for pages array");
			ret = -ENOMEM;
			goto wlist_unmap;
		}
		pinned_pages = get_user_pages(user_va & PAGE_MASK,
					      num_pages,
					      0,
					      pages,
					      NULL);
		if (pinned_pages != num_pages) {
			tegra_soc_hwpm_err("Requested %llu pages / Got %ld pages",
					num_pages, pinned_pages);
			ret = -ENOMEM;
			goto wlist_unmap;
		}
		full_wlist = vmap(pages, num_pages, VM_MAP, PAGE_KERNEL);
		if (!full_wlist) {
			tegra_soc_hwpm_err("Couldn't map whitelist buffer into"
					   " kernel address space");
			ret = -ENOMEM;
			goto wlist_unmap;
		}
		full_wlist_u64 = (u64 *)(full_wlist + offset);

		/* Fill in whitelist buffer */
		for (res_idx = 0, full_wlist_idx = 0;
		     res_idx < TERGA_SOC_HWPM_NUM_RESOURCES;
		     res_idx++) {
			if (!(hwpm_resources[res_idx].reserved))
				continue;
			tegra_soc_hwpm_dbg("Found reserved IP(%d)", res_idx);

			for (aprt_idx = 0;
			     aprt_idx < hwpm_resources[res_idx].map_size;
			     aprt_idx++) {
				aperture = &(hwpm_resources[res_idx].map[aprt_idx]);
				if (aperture->wlist) {
					for (aprt_wlist_idx = 0;
					     aprt_wlist_idx < aperture->wlist_size;
					     aprt_wlist_idx++, full_wlist_idx++) {
						full_wlist_u64[full_wlist_idx] =
							aperture->start_pa +
							aperture->wlist[aprt_wlist_idx].reg;
					}
				} else {
					tegra_soc_hwpm_err("NULL whitelist in aperture(0x%llx - 0x%llx)",
							   aperture->start_pa,
							   aperture->end_pa);
				}
			}
		}

wlist_unmap:
		if (full_wlist)
			vunmap(full_wlist);
		if (pinned_pages > 0) {
			for (page_idx = 0; page_idx < pinned_pages; page_idx++) {
				set_page_dirty(pages[page_idx]);
				put_page(pages[page_idx]);
			}
		}
		if (pages)
			kfree(pages);
	}


	return ret;
}

static int exec_reg_ops_ioctl(struct tegra_soc_hwpm *hwpm,
			      void *ioctl_struct)
{
	int ret = 0;
	struct tegra_soc_hwpm_exec_reg_ops *exec_reg_ops =
			(struct tegra_soc_hwpm_exec_reg_ops *)ioctl_struct;
	struct hwpm_resource_aperture *aperture = NULL;
	int op_idx = 0;
	struct tegra_soc_hwpm_reg_op *reg_op = NULL;

	if (!hwpm->bind_completed) {
		tegra_soc_hwpm_err("The EXEC_REG_OPS IOCTL can only be called"
				   " after the BIND IOCTL.");
		return -EPERM;
	}
	switch (exec_reg_ops->mode) {
	case TEGRA_SOC_HWPM_REG_OP_MODE_FAIL_ON_FIRST:
	case TEGRA_SOC_HWPM_REG_OP_MODE_CONT_ON_ERR:
		break;

	default:
		tegra_soc_hwpm_err("Invalid reg ops mode(%u)",
				   exec_reg_ops->mode);
		return -EINVAL;
	}

	for (op_idx = 0; op_idx < exec_reg_ops->op_count; op_idx++) {
#define REG_OP_FAIL(op_status, msg, ...)				\
	do {								\
		tegra_soc_hwpm_err(msg, ##__VA_ARGS__);			\
		reg_op->status =					\
			TEGRA_SOC_HWPM_REG_OP_STATUS_ ## op_status;	\
		exec_reg_ops->b_all_reg_ops_passed = false;		\
		if (exec_reg_ops->mode ==				\
		    TEGRA_SOC_HWPM_REG_OP_MODE_FAIL_ON_FIRST) {		\
			return -EINVAL;					\
		}							\
	} while (0)

		tegra_soc_hwpm_dbg("reg op: idx(%d), phys(0x%llx), cmd(%u)",
				op_idx, reg_op->phys_addr, reg_op->cmd);
		reg_op = &(exec_reg_ops->ops[op_idx]);

		/* The whitelist check is done here */
		aperture = find_hwpm_aperture(hwpm, reg_op->phys_addr, true);
		if (!aperture) {
			REG_OP_FAIL(INSUFFICIENT_PERMISSIONS,
				"Invalid register address(0x%llx)",
				reg_op->phys_addr);
			continue;
		}

		switch (reg_op->cmd) {
		case TEGRA_SOC_HWPM_REG_OP_CMD_RD32:
			reg_op->reg_val_lo = ioctl_readl(hwpm,
							aperture,
							reg_op->phys_addr);
			reg_op->status = TEGRA_SOC_HWPM_REG_OP_STATUS_SUCCESS;
			break;

		case TEGRA_SOC_HWPM_REG_OP_CMD_RD64:
			reg_op->reg_val_lo = ioctl_readl(hwpm,
							 aperture,
							 reg_op->phys_addr);
			reg_op->reg_val_hi = ioctl_readl(hwpm,
							 aperture,
							 reg_op->phys_addr + 4);
			reg_op->status = TEGRA_SOC_HWPM_REG_OP_STATUS_SUCCESS;
			break;

		/* Read Modify Write operation */
		case TEGRA_SOC_HWPM_REG_OP_CMD_WR32:
			ret = IOCTL_REG_RMW(hwpm,
					    aperture,
					    reg_op->phys_addr,
					    reg_op->mask_lo,
					    reg_op->reg_val_lo);
			if (ret < 0) {
				REG_OP_FAIL(INVALID,
					"WR32 REGOP failed for register(0x%llx)",
					reg_op->phys_addr);
			} else {
				reg_op->status = TEGRA_SOC_HWPM_REG_OP_STATUS_SUCCESS;
			}
			break;

		/* Read Modify Write operation */
		case TEGRA_SOC_HWPM_REG_OP_CMD_WR64:
			/* Lower 32 bits */
			ret = IOCTL_REG_RMW(hwpm,
					aperture,
					reg_op->phys_addr,
					reg_op->mask_lo,
					reg_op->reg_val_lo);
			if (ret < 0) {
				REG_OP_FAIL(INVALID,
					"WR64 REGOP failed for register(0x%llx)",
					reg_op->phys_addr);
				continue;
			}

			/* Upper 32 bits */
			ret = IOCTL_REG_RMW(hwpm,
					aperture,
					reg_op->phys_addr + 4,
					reg_op->mask_hi,
					reg_op->reg_val_hi);
			if (ret < 0) {
				REG_OP_FAIL(INVALID,
					"WR64 REGOP failed for register(0x%llx)",
					reg_op->phys_addr + 4);
			} else {
				reg_op->status = TEGRA_SOC_HWPM_REG_OP_STATUS_SUCCESS;
			}

			break;

		default:
			REG_OP_FAIL(INVALID_CMD,
				    "Invalid reg op command(%u)",
				    reg_op->cmd);
			break;
		}

	}

	exec_reg_ops->b_all_reg_ops_passed = true;
	return 0;
}

static int update_get_put_ioctl(struct tegra_soc_hwpm *hwpm,
				void *ioctl_struct)
{
	int ret = 0;
	u32 reg_val = 0;
	u32 field_val = 0;
	u32 *mem_bytes_kernel_u32 = NULL;
	struct tegra_soc_hwpm_update_get_put *update_get_put =
			(struct tegra_soc_hwpm_update_get_put *)ioctl_struct;

	if (!hwpm->bind_completed) {
		tegra_soc_hwpm_err("The UPDATE_GET_PUT IOCTL can only be called"
				   " after the BIND IOCTL.");
		return -EPERM;
	}
	if (!hwpm->mem_bytes_kernel) {
		tegra_soc_hwpm_err("mem_bytes buffer is not mapped in the driver");
		return -ENXIO;
	}

	/* Update SW get pointer */
	hwpm_writel(hwpm,
		    TEGRA_SOC_HWPM_PMA_DT,
		    NV_PERF_PMASYS_CHANNEL_MEM_BUMP_CH0,
		    update_get_put->mem_bump);

	/* Stream MEM_BYTES value to MEM_BYTES buffer */
	if (update_get_put->b_stream_mem_bytes) {
		mem_bytes_kernel_u32 = (u32 *)(hwpm->mem_bytes_kernel);
		*mem_bytes_kernel_u32 = TEGRA_SOC_HWPM_MEM_BYTES_INVALID;
		ret = DRIVER_REG_RMW(hwpm,
				TEGRA_SOC_HWPM_PMA_DT,
				NV_PERF_PMASYS_CHANNEL_CONTROL_USER_CH0,
				NV_PERF_PMASYS_CHANNEL_CONTROL_USER_UPDATE_BYTES,
				NV_PERF_PMASYS_CHANNEL_CONTROL_USER_UPDATE_BYTES_DOIT,
				false);
		if (ret < 0) {
			tegra_soc_hwpm_err("Failed to stream mem_bytes to buffer");
			return -EIO;
		}
	}

	/* Read HW put pointer */
	if (update_get_put->b_read_mem_head) {
		update_get_put->mem_head =
				hwpm_readl(hwpm,
					TEGRA_SOC_HWPM_PMA_DT,
					NV_PERF_PMASYS_CHANNEL_MEM_HEAD_CH0);
		tegra_soc_hwpm_dbg("MEM_HEAD = 0x%llx",
				   update_get_put->mem_head);
	}

	/* Check overflow error status */
	if (update_get_put->b_check_overflow) {
		reg_val = hwpm_readl(hwpm,
				TEGRA_SOC_HWPM_PMA_DT,
				NV_PERF_PMASYS_CHANNEL_STATUS_SECURE_CH0);
		field_val =
			HWPM_REG_V(NV_PERF_PMASYS_CHANNEL_STATUS_SECURE_MEMBUF_STATUS,
				   reg_val);
		update_get_put->b_overflowed =
			(field_val ==
				NV_PERF_PMASYS_CHANNEL_STATUS_SECURE_MEMBUF_STATUS_OVERFLOWED);
		tegra_soc_hwpm_dbg("OVERFLOWED = %u",
				   update_get_put->b_overflowed);
	}

	return 0;
}

static long tegra_soc_hwpm_ioctl(struct file *file,
				 unsigned int cmd,
				 unsigned long arg)
{
	int ret = 0;
	enum tegra_soc_hwpm_ioctl_num ioctl_num = _IOC_NR(cmd);
	u32 ioc_dir = _IOC_DIR(cmd);
	u32 arg_size = _IOC_SIZE(cmd);
	struct tegra_soc_hwpm *hwpm = NULL;
	void *arg_copy = NULL;

	if (!file) {
		tegra_soc_hwpm_err("Invalid file");
		ret = -ENODEV;
		goto fail;
	}
	if ((_IOC_TYPE(cmd) != TEGRA_SOC_HWPM_IOC_MAGIC) ||
	    (ioctl_num < 0) ||
	    (ioctl_num >= TERGA_SOC_HWPM_NUM_IOCTLS)) {
		tegra_soc_hwpm_err("Unsupported IOCTL call");
		ret = -EINVAL;
		goto fail;
	}
	if (arg_size != ioctls[ioctl_num].struct_size) {
		tegra_soc_hwpm_err("Invalid userspace struct");
		ret = -EINVAL;
		goto fail;
	}

	hwpm = file->private_data;
	if (!hwpm) {
		tegra_soc_hwpm_err("Invalid hwpm struct");
		ret = -ENODEV;
		goto fail;
	}

	/* Only allocate a buffer if the IOCTL needs a buffer */
	if (!(ioc_dir & _IOC_NONE)) {
		arg_copy = kzalloc(arg_size, GFP_KERNEL);
		if (!arg_copy) {
			tegra_soc_hwpm_err("Can't allocate memory for kernel struct");
			ret = -ENOMEM;
			goto fail;
		}
	}

	if (ioc_dir & _IOC_WRITE) {
		if (copy_from_user(arg_copy, (void __user *)arg, arg_size)) {
			tegra_soc_hwpm_err("Failed to copy data from userspace"
					   " struct into kernel struct");
			ret = -EFAULT;
			goto fail;
		}
	}

	/*
	 * We don't goto fail here because even if the IOCTL fails, we have to
	 * call copy_to_user() to pass back any valid output params to
	 * userspace.
	 */
	ret = ioctls[ioctl_num].handler(hwpm, arg_copy);

	if (ioc_dir & _IOC_READ) {
		if (copy_to_user((void __user *)arg, arg_copy, arg_size)) {
			tegra_soc_hwpm_err("Failed to copy data from kernel"
					   " struct into userspace struct");
			ret = -EFAULT;
			goto fail;
		}
	}

	if (ret < 0)
		goto fail;

	tegra_soc_hwpm_dbg("The %s IOCTL completed successfully!",
			   ioctls[ioctl_num].name);
	goto cleanup;

fail:
	tegra_soc_hwpm_err("The %s IOCTL failed(%d)!",
			   ioctls[ioctl_num].name, ret);
cleanup:
	if (arg_copy)
		kfree(arg_copy);

	return ret;
}

static int tegra_soc_hwpm_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	unsigned int minor = iminor(inode);
	struct tegra_soc_hwpm *hwpm = NULL;
	struct resource *res = NULL;
	u64 num_regs = 0;

	if (!inode) {
		tegra_soc_hwpm_err("Invalid inode");
		return -EINVAL;
	}
	if (!filp) {
		tegra_soc_hwpm_err("Invalid file");
		return -EINVAL;
	}
	if (minor > 0) {
		tegra_soc_hwpm_err("Incorrect minor number");
		return -EBADFD;
	}

	hwpm = container_of(inode->i_cdev, struct tegra_soc_hwpm, cdev);
	if (!hwpm) {
		tegra_soc_hwpm_err("Invalid hwpm struct");
		return -EINVAL;
	}
	filp->private_data = hwpm;

	/* FIXME: Enable clock and reset programming */
#if 0
	ret = reset_control_assert(hwpm->hwpm_rst);
	if (ret < 0) {
		tegra_soc_hwpm_err("hwpm reset assert failed");
		ret = -ENODEV;
		goto fail;
	}
	ret = reset_control_assert(hwpm->la_rst);
	if (ret < 0) {
		tegra_soc_hwpm_err("la reset assert failed");
		ret = -ENODEV;
		goto fail;
	}
	ret = clk_prepare_enable(hwpm->la_clk);
	if (ret < 0) {
		tegra_soc_hwpm_err("la clock enable failed");
		ret = -ENODEV;
		goto fail;
	}
	ret = reset_control_deassert(hwpm->la_rst);
	if (ret < 0) {
		tegra_soc_hwpm_err("la reset deassert failed");
		ret = -ENODEV;
		goto fail;
	}
	ret = reset_control_deassert(hwpm->hwpm_rst);
	if (ret < 0) {
		tegra_soc_hwpm_err("hwpm reset deassert failed");
		ret = -ENODEV;
		goto fail;
	}
#endif

	/* Map PMA and RTR apertures */
	hwpm->dt_apertures[TEGRA_SOC_HWPM_PMA_DT] =
				of_iomap(hwpm->np, TEGRA_SOC_HWPM_PMA_DT);
	if (!hwpm->dt_apertures[TEGRA_SOC_HWPM_PMA_DT]) {
		tegra_soc_hwpm_err("Couldn't map the PMA aperture");
		ret = -ENOMEM;
		goto fail;
	}
	res = platform_get_resource(hwpm->pdev,
					IORESOURCE_MEM,
					TEGRA_SOC_HWPM_PMA_DT);
	if ((!res) || (res->start == 0) || (res->end == 0)) {
		tegra_soc_hwpm_err("Invalid resource for PMA");
		ret = -ENOMEM;
		goto fail;
	}
	pma_map[1].start_pa = res->start;
	pma_map[1].end_pa = res->end;
	cmd_slice_rtr_map[0].start_pa = res->start;
	cmd_slice_rtr_map[0].end_pa = res->end;
	if (hwpm->fake_registers_enabled) {
		num_regs = (res->end + 1 - res->start) / sizeof(*pma_fake_regs);
		pma_fake_regs = (u32 *)kzalloc(sizeof(*pma_fake_regs) * num_regs,
						GFP_KERNEL);
		if (!pma_fake_regs) {
			tegra_soc_hwpm_err("Couldn't allocate memory for PMA"
					   " fake registers");
			ret = -ENOMEM;
			goto fail;
		}
		pma_map[1].fake_registers = pma_fake_regs;
		cmd_slice_rtr_map[0].fake_registers = pma_fake_regs;
	}

	hwpm->dt_apertures[TEGRA_SOC_HWPM_RTR_DT] =
				of_iomap(hwpm->np, TEGRA_SOC_HWPM_RTR_DT);
	if (!hwpm->dt_apertures[TEGRA_SOC_HWPM_RTR_DT]) {
		tegra_soc_hwpm_err("Couldn't map the RTR aperture");
		ret = -ENOMEM;
		goto fail;
	}
	res = platform_get_resource(hwpm->pdev,
				    IORESOURCE_MEM,
				    TEGRA_SOC_HWPM_RTR_DT);
	if ((!res) || (res->start == 0) || (res->end == 0)) {
		tegra_soc_hwpm_err("Invalid resource for RTR");
		ret = -ENOMEM;
		goto fail;
	}
	cmd_slice_rtr_map[1].start_pa = res->start;
	cmd_slice_rtr_map[1].end_pa = res->end;
	if (hwpm->fake_registers_enabled) {
		num_regs = (res->end + 1 - res->start) /
				sizeof(*cmd_slice_rtr_map[1].fake_registers);
		cmd_slice_rtr_map[1].fake_registers =
			(u32 *)kzalloc(sizeof(*cmd_slice_rtr_map[1].fake_registers) *
									num_regs,
					GFP_KERNEL);
		if (!cmd_slice_rtr_map[1].fake_registers) {
			tegra_soc_hwpm_err("Couldn't allocate memory for RTR"
					   " fake registers");
			ret = -ENOMEM;
			goto fail;
		}
	}

	/* FIXME: Remove after verification */
	/* Disable SLCG */
	ret = DRIVER_REG_RMW(hwpm,
			     TEGRA_SOC_HWPM_PMA_DT,
			     NV_PERF_PMASYS_CG2,
			     NV_PERF_PMASYS_CG2_SLCG,
			     NV_PERF_PMASYS_CG2_SLCG_DISABLED,
			     false);
	if (ret < 0) {
		tegra_soc_hwpm_err("Unable to disable PMA SLCG");
		ret = -EIO;
		goto fail;
	}
	ret = DRIVER_REG_RMW(hwpm,
			     TEGRA_SOC_HWPM_RTR_DT,
			     NV_PERF_PMMSYS_SYS0ROUTER_CG2,
			     NV_PERF_PMMSYS_SYS0ROUTER_CG2_SLCG,
			     NV_PERF_PMMSYS_SYS0ROUTER_CG2_SLCG_DISABLED,
			     false);
	if (ret < 0) {
		tegra_soc_hwpm_err("Unable to disable ROUTER SLCG");
		ret = -EIO;
		goto fail;
	}

	/* Program PROD values */
	ret = DRIVER_REG_RMW(hwpm,
			TEGRA_SOC_HWPM_PMA_DT,
			NV_PERF_PMASYS_CONTROLB,
			NV_PERF_PMASYS_CONTROLB_COALESCE_TIMEOUT_CYCLES,
			NV_PERF_PMASYS_CONTROLB_COALESCE_TIMEOUT_CYCLES__PROD,
			false);
	if (ret < 0) {
		tegra_soc_hwpm_err("Unable to program PROD value");
		ret = -EIO;
		goto fail;
	}
	ret = DRIVER_REG_RMW(hwpm,
		TEGRA_SOC_HWPM_PMA_DT,
		NV_PERF_PMASYS_CHANNEL_CONFIG_USER_CH0,
		NV_PERF_PMASYS_CHANNEL_CONFIG_USER_COALESCE_TIMEOUT_CYCLES,
		NV_PERF_PMASYS_CHANNEL_CONFIG_USER_COALESCE_TIMEOUT_CYCLES__PROD,
		false);
	if (ret < 0) {
		tegra_soc_hwpm_err("Unable to program PROD value");
		ret = -EIO;
		goto fail;
	}

	/* Initialize SW state */
	hwpm->bind_completed = false;
	hwpm->full_wlist_size = -1;

	return 0;

fail:
	if (hwpm->dt_apertures[TEGRA_SOC_HWPM_PMA_DT]) {
		iounmap(hwpm->dt_apertures[TEGRA_SOC_HWPM_PMA_DT]);
		hwpm->dt_apertures[TEGRA_SOC_HWPM_PMA_DT] = NULL;
	}
	pma_map[1].start_pa = 0;
	pma_map[1].end_pa = 0;
	cmd_slice_rtr_map[0].start_pa = 0;
	cmd_slice_rtr_map[0].end_pa = 0;
	if (pma_fake_regs) {
		kfree(pma_fake_regs);
		pma_fake_regs = NULL;
		pma_map[1].fake_registers = NULL;
		cmd_slice_rtr_map[0].fake_registers = NULL;
	}

	if (hwpm->dt_apertures[TEGRA_SOC_HWPM_RTR_DT]) {
		iounmap(hwpm->dt_apertures[TEGRA_SOC_HWPM_RTR_DT]);
		hwpm->dt_apertures[TEGRA_SOC_HWPM_RTR_DT] = NULL;
	}
	cmd_slice_rtr_map[1].start_pa = 0;
	cmd_slice_rtr_map[1].end_pa = 0;
	if (cmd_slice_rtr_map[1].fake_registers) {
		kfree(cmd_slice_rtr_map[1].fake_registers);
		cmd_slice_rtr_map[1].fake_registers = NULL;
	}
	tegra_soc_hwpm_err("%s failed", __func__);

	return ret;
}

static ssize_t tegra_soc_hwpm_read(struct file *file,
				   char __user *ubuf,
				   size_t count,
				   loff_t *offp)
{
	return 0;
}

/* FIXME: Fix double release bug */
static int tegra_soc_hwpm_release(struct inode *inode, struct file *filp)
{
	int err = 0;
	int ret = 0;
	bool timeout = false;
	int res_idx = 0;
	int aprt_idx = 0;
	u32 field_mask = 0;
	u32 field_val = 0;
	u32 *mem_bytes_kernel_u32 = NULL;
	struct tegra_soc_hwpm *hwpm = NULL;
	struct hwpm_resource_aperture *aperture = NULL;
#define RELEASE_FAIL(msg, ...)					\
	do {							\
		if (err < 0) {					\
			tegra_soc_hwpm_err(msg, ##__VA_ARGS__);	\
			if (ret == 0)				\
				ret = err;			\
		}						\
	} while (0)

	if (!inode) {
		tegra_soc_hwpm_err("Invalid inode");
		return -EINVAL;
	}
	if (!filp) {
		tegra_soc_hwpm_err("Invalid file");
		return -EINVAL;
	}

	hwpm = container_of(inode->i_cdev, struct tegra_soc_hwpm, cdev);
	if (!hwpm) {
		tegra_soc_hwpm_err("Invalid hwpm struct");
		return -EINVAL;
	}

	/* Disable PMA triggers */
	err = DRIVER_REG_RMW(hwpm,
			TEGRA_SOC_HWPM_PMA_DT,
			NV_PERF_PMASYS_TRIGGER_CONFIG_USER_CH0,
			NV_PERF_PMASYS_TRIGGER_CONFIG_USER_PMA_PULSE,
			NV_PERF_PMASYS_TRIGGER_CONFIG_USER_PMA_PULSE_DISABLE,
			false);
	RELEASE_FAIL("Unable to disable PMA triggers");

	hwpm_writel(hwpm,
		    TEGRA_SOC_HWPM_PMA_DT,
		    NV_PERF_PMASYS_SYS_TRIGGER_START_MASK,
		    0);
	hwpm_writel(hwpm,
		    TEGRA_SOC_HWPM_PMA_DT,
		    NV_PERF_PMASYS_SYS_TRIGGER_START_MASKB,
		    0);
	hwpm_writel(hwpm,
		    TEGRA_SOC_HWPM_PMA_DT,
		    NV_PERF_PMASYS_SYS_TRIGGER_STOP_MASK,
		    0);
	hwpm_writel(hwpm,
		    TEGRA_SOC_HWPM_PMA_DT,
		    NV_PERF_PMASYS_SYS_TRIGGER_STOP_MASKB,
		    0);

	/* Wait for PERFMONs, ROUTER, and PMA to idle */
	timeout = HWPM_TIMEOUT(HWPM_REG_CHECK_F(hwpm_readl(hwpm,
						TEGRA_SOC_HWPM_RTR_DT,
						NV_PERF_PMMSYS_SYS0ROUTER_PERFMONSTATUS),
					NV_PERF_PMMSYS_SYS0ROUTER_PERFMONSTATUS_MERGED,
					NV_PERF_PMMSYS_SYS0ROUTER_PERFMONSTATUS_MERGED_EMPTY),
			"NV_PERF_PMMSYS_SYS0ROUTER_PERFMONSTATUS_MERGED_EMPTY");
	if (timeout && ret == 0) {
		ret = -EIO;
	}
	timeout = HWPM_TIMEOUT(HWPM_REG_CHECK_F(hwpm_readl(hwpm,
						TEGRA_SOC_HWPM_RTR_DT,
						NV_PERF_PMMSYS_SYS0ROUTER_ENGINESTATUS),
					NV_PERF_PMMSYS_SYS0ROUTER_ENGINESTATUS_STATUS,
					NV_PERF_PMMSYS_SYS0ROUTER_ENGINESTATUS_STATUS_EMPTY),
			"NV_PERF_PMMSYS_SYS0ROUTER_ENGINESTATUS_STATUS_EMPTY");
	if (timeout && ret == 0) {
		ret = -EIO;
	}
	field_mask = NV_PERF_PMASYS_ENGINESTATUS_STATUS_MASK |
		     NV_PERF_PMASYS_ENGINESTATUS_RBUFEMPTY_MASK;
	field_val = HWPM_REG_F(NV_PERF_PMASYS_ENGINESTATUS_STATUS,
			       NV_PERF_PMASYS_ENGINESTATUS_STATUS_EMPTY);
	field_val |= HWPM_REG_F(NV_PERF_PMASYS_ENGINESTATUS_RBUFEMPTY,
				NV_PERF_PMASYS_ENGINESTATUS_RBUFEMPTY_EMPTY);
	timeout = HWPM_TIMEOUT(HWPM_REG_CHECK(hwpm_readl(hwpm,
						TEGRA_SOC_HWPM_PMA_DT,
						NV_PERF_PMASYS_ENGINESTATUS),
				    field_mask,
				    field_val),
			"NV_PERF_PMASYS_ENGINESTATUS");
	if (timeout && ret == 0) {
		ret = -EIO;
	}

	/* Disable all PERFMONs */
	tegra_soc_hwpm_dbg("Disabling PERFMONs");
	for (res_idx = 0; res_idx < TERGA_SOC_HWPM_NUM_RESOURCES; res_idx++) {
		if (!hwpm_resources[res_idx].reserved)
			continue;
		tegra_soc_hwpm_dbg("Found reserved IP(%d)", res_idx);

		for (aprt_idx = 0;
		     aprt_idx < hwpm_resources[res_idx].map_size;
		     aprt_idx++) {
			aperture = &(hwpm_resources[res_idx].map[aprt_idx]);
			if (IS_PERFMON(aperture->dt_aperture)) {
				tegra_soc_hwpm_dbg("Found PERFMON(0x%llx - 0x%llx)",
						   aperture->start_pa,
						   aperture->end_pa);
				err = DRIVER_REG_RMW(hwpm,
					aperture->dt_aperture,
					NV_PERF_PMMSYS_CONTROL,
					NV_PERF_PMMSYS_CONTROL_MODE,
					NV_PERF_PMMSYS_CONTROL_MODE_DISABLE,
					false);
				RELEASE_FAIL("Unable to disable PERFMON(0x%llx - 0x%llx)",
					     aperture->start_pa,
					     aperture->end_pa);
			}
		}
	}

	/* Stream MEM_BYTES to clear pipeline */
	if (hwpm->mem_bytes_kernel) {
		mem_bytes_kernel_u32 = (u32 *)(hwpm->mem_bytes_kernel);
		*mem_bytes_kernel_u32 = TEGRA_SOC_HWPM_MEM_BYTES_INVALID;
		err = DRIVER_REG_RMW(hwpm,
			TEGRA_SOC_HWPM_PMA_DT,
			NV_PERF_PMASYS_CHANNEL_CONTROL_USER_CH0,
			NV_PERF_PMASYS_CHANNEL_CONTROL_USER_UPDATE_BYTES,
			NV_PERF_PMASYS_CHANNEL_CONTROL_USER_UPDATE_BYTES_DOIT,
			false);
		RELEASE_FAIL("Unable to stream MEM_BYTES");
		timeout = HWPM_TIMEOUT(*mem_bytes_kernel_u32 !=
				       TEGRA_SOC_HWPM_MEM_BYTES_INVALID,
			     "MEM_BYTES streaming");
		if (timeout && ret == 0)
			ret = -EIO;
	}

	/* Disable PMA streaming */
	err = DRIVER_REG_RMW(hwpm,
		TEGRA_SOC_HWPM_PMA_DT,
		NV_PERF_PMASYS_TRIGGER_CONFIG_USER_CH0,
		NV_PERF_PMASYS_TRIGGER_CONFIG_USER_RECORD_STREAM,
		NV_PERF_PMASYS_TRIGGER_CONFIG_USER_RECORD_STREAM_DISABLE,
		false);
	RELEASE_FAIL("Unable to disable PMA streaming");
	err = DRIVER_REG_RMW(hwpm,
			TEGRA_SOC_HWPM_PMA_DT,
			NV_PERF_PMASYS_CHANNEL_CONTROL_USER_CH0,
			NV_PERF_PMASYS_CHANNEL_CONTROL_USER_STREAM,
			NV_PERF_PMASYS_CHANNEL_CONTROL_USER_STREAM_DISABLE,
			false);
	RELEASE_FAIL("Unable to disable PMA streaming");

	/* Memory Management */
	hwpm_writel(hwpm,
		    TEGRA_SOC_HWPM_PMA_DT,
		    NV_PERF_PMASYS_CHANNEL_OUTBASE_CH0,
		    0);
	hwpm_writel(hwpm,
		    TEGRA_SOC_HWPM_PMA_DT,
		    NV_PERF_PMASYS_CHANNEL_OUTBASEUPPER_CH0,
		    0);
	hwpm_writel(hwpm,
		    TEGRA_SOC_HWPM_PMA_DT,
		    NV_PERF_PMASYS_CHANNEL_OUTSIZE_CH0,
		    0);
	hwpm_writel(hwpm,
		    TEGRA_SOC_HWPM_PMA_DT,
		    NV_PERF_PMASYS_CHANNEL_MEM_BYTES_ADDR_CH0,
		    0);

	if (hwpm->stream_sgt && (!IS_ERR(hwpm->stream_sgt))) {
		dma_buf_unmap_attachment(hwpm->stream_attach,
					 hwpm->stream_sgt,
					 DMA_FROM_DEVICE);
	}
	hwpm->stream_sgt = NULL;

	if (hwpm->stream_attach && (!IS_ERR(hwpm->stream_attach))) {
		dma_buf_detach(hwpm->stream_dma_buf, hwpm->stream_attach);
	}
	hwpm->stream_attach = NULL;

	if (hwpm->stream_dma_buf && (!IS_ERR(hwpm->stream_dma_buf))) {
		dma_buf_put(hwpm->stream_dma_buf);
	}
	hwpm->stream_dma_buf = NULL;

	if (hwpm->mem_bytes_kernel) {
		dma_buf_vunmap(hwpm->mem_bytes_dma_buf,
			       hwpm->mem_bytes_kernel);
		hwpm->mem_bytes_kernel = NULL;
	}

	if (hwpm->mem_bytes_sgt && (!IS_ERR(hwpm->mem_bytes_sgt))) {
		dma_buf_unmap_attachment(hwpm->mem_bytes_attach,
					 hwpm->mem_bytes_sgt,
					 DMA_FROM_DEVICE);
	}
	hwpm->mem_bytes_sgt = NULL;

	if (hwpm->mem_bytes_attach && (!IS_ERR(hwpm->mem_bytes_attach))) {
		dma_buf_detach(hwpm->mem_bytes_dma_buf, hwpm->mem_bytes_attach);
	}
	hwpm->mem_bytes_attach = NULL;

	if (hwpm->mem_bytes_dma_buf && (!IS_ERR(hwpm->mem_bytes_dma_buf))) {
		dma_buf_put(hwpm->mem_bytes_dma_buf);
	}
	hwpm->mem_bytes_dma_buf = NULL;

	/* FIXME: Enable clock and reset programming */
	/* FIXME: Tell IPs which are being profiled to re-enable power management */
#if 0
	err = reset_control_assert(hwpm->hwpm_rst);
	RELEASE_FAIL("hwpm reset assert failed");
	err = reset_control_assert(hwpm->la_rst);
	RELEASE_FAIL("la reset assert failed");
	clk_disable_unprepare(hwpm->la_clk);
#endif

	/* FIXME: Remove after verification */
	/* Enable SLCG */
	err = DRIVER_REG_RMW(hwpm,
			     TEGRA_SOC_HWPM_PMA_DT,
			     NV_PERF_PMASYS_CG2,
			     NV_PERF_PMASYS_CG2_SLCG,
			     NV_PERF_PMASYS_CG2_SLCG_ENABLED,
			     false);
	RELEASE_FAIL("Unable to enable PMA SLCG");
	err = DRIVER_REG_RMW(hwpm,
			     TEGRA_SOC_HWPM_RTR_DT,
			     NV_PERF_PMMSYS_SYS0ROUTER_CG2,
			     NV_PERF_PMMSYS_SYS0ROUTER_CG2_SLCG,
			     NV_PERF_PMMSYS_SYS0ROUTER_CG2_SLCG_ENABLED,
			     false);
	RELEASE_FAIL("Unable to enable ROUTER SLCG");

	/* Unmap PMA and RTR apertures */
	tegra_soc_hwpm_dbg("Unmapping apertures");
	if (hwpm->dt_apertures[TEGRA_SOC_HWPM_PMA_DT]) {
		iounmap(hwpm->dt_apertures[TEGRA_SOC_HWPM_PMA_DT]);
		hwpm->dt_apertures[TEGRA_SOC_HWPM_PMA_DT] = NULL;
	}
	pma_map[1].start_pa = 0;
	pma_map[1].end_pa = 0;
	cmd_slice_rtr_map[0].start_pa = 0;
	cmd_slice_rtr_map[0].end_pa = 0;
	if (pma_fake_regs) {
		kfree(pma_fake_regs);
		pma_fake_regs = NULL;
		pma_map[1].fake_registers = NULL;
		cmd_slice_rtr_map[0].fake_registers = NULL;
	}
	if (hwpm->dt_apertures[TEGRA_SOC_HWPM_RTR_DT]) {
		iounmap(hwpm->dt_apertures[TEGRA_SOC_HWPM_RTR_DT]);
		hwpm->dt_apertures[TEGRA_SOC_HWPM_RTR_DT] = NULL;
	}
	cmd_slice_rtr_map[1].start_pa = 0;
	cmd_slice_rtr_map[1].end_pa = 0;
	if (cmd_slice_rtr_map[1].fake_registers) {
		kfree(cmd_slice_rtr_map[1].fake_registers);
		cmd_slice_rtr_map[1].fake_registers = NULL;
	}

	/* Reset resource and aperture state */
	for (res_idx = 0; res_idx < TERGA_SOC_HWPM_NUM_RESOURCES; res_idx++) {
		if (!hwpm_resources[res_idx].reserved)
			continue;
		tegra_soc_hwpm_dbg("Found reserved IP(%d)", res_idx);
		hwpm_resources[res_idx].reserved = false;

		for (aprt_idx = 0;
		     aprt_idx < hwpm_resources[res_idx].map_size;
		     aprt_idx++) {
			aperture = &(hwpm_resources[res_idx].map[aprt_idx]);
			if ((aperture->dt_aperture == TEGRA_SOC_HWPM_PMA_DT) ||
			    (aperture->dt_aperture == TEGRA_SOC_HWPM_RTR_DT)) {
				/* PMA and RTR apertures are handled separately */
				continue;
			} else if (IS_PERFMON(aperture->dt_aperture)) {
				if (hwpm->dt_apertures[aperture->dt_aperture]) {
					iounmap(hwpm->dt_apertures[aperture->dt_aperture]);
					hwpm->dt_apertures[aperture->dt_aperture] = NULL;
				}

				aperture->start_pa = 0;
				aperture->end_pa = 0;

				if (aperture->fake_registers) {
					kfree(aperture->fake_registers);
					aperture->fake_registers = NULL;
				}
			} else { /* IP apertures */
				if (aperture->fake_registers) {
					kfree(aperture->fake_registers);
					aperture->fake_registers = NULL;
					set_mc_fake_regs(hwpm, aperture, true);
				}
			}
		}
	}

	return ret;
}

/* File ops for device node */
const struct file_operations tegra_soc_hwpm_ops = {
	.owner = THIS_MODULE,
	.open = tegra_soc_hwpm_open,
	.read = tegra_soc_hwpm_read,
	.release = tegra_soc_hwpm_release,
	.unlocked_ioctl = tegra_soc_hwpm_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = tegra_soc_hwpm_ioctl,
#endif
};
