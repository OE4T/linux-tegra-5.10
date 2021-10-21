/*
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
 *
 * tegra-soc-hwpm-ioctl.c:
 * This file adds IOCTL handlers for the Tegra SOC HWPM driver.
 */

#include <soc/tegra/fuse.h>
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

#include "include/hw/t234/hw_pmasys_soc_hwpm.h"
#include "include/hw/t234/hw_pmmsys_soc_hwpm.h"
#include "include/hw/t234/hw_addr_map_soc_hwpm.h"

#include "tegra-soc-hwpm.h"
#include "tegra-soc-hwpm-io.h"

#define LA_CLK_RATE 625000000UL

struct tegra_soc_hwpm_ioctl {
	const char *const name;
	const size_t struct_size;
	int (*handler)(struct tegra_soc_hwpm *, void *);
};

static int device_info_ioctl(struct tegra_soc_hwpm *hwpm,
			     void *ioctl_struct);
static int floorsweep_info_ioctl(struct tegra_soc_hwpm *hwpm,
			     void *ioctl_struct);
static int timer_relation_ioctl(struct tegra_soc_hwpm *hwpm,
				void *ioctl_struct);
static int reserve_resource_ioctl(struct tegra_soc_hwpm *hwpm,
				  void *ioctl_struct);
static int alloc_pma_stream_ioctl(struct tegra_soc_hwpm *hwpm,
				  void *ioctl_struct);
static int bind_ioctl(struct tegra_soc_hwpm *hwpm,
		      void *ioctl_struct);
static int query_allowlist_ioctl(struct tegra_soc_hwpm *hwpm,
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
	[TEGRA_SOC_HWPM_IOCTL_FLOORSWEEP_INFO] = {
		.name			= "floorsweep_info",
		.struct_size		= sizeof(struct tegra_soc_hwpm_ip_floorsweep_info),
		.handler		= floorsweep_info_ioctl,
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
	[TEGRA_SOC_HWPM_IOCTL_QUERY_ALLOWLIST] = {
		.name			= "query_allowlist",
		.struct_size		= sizeof(struct tegra_soc_hwpm_query_allowlist),
		.handler		= query_allowlist_ioctl,
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
	struct tegra_soc_hwpm_device_info *device_info =
			(struct tegra_soc_hwpm_device_info *)ioctl_struct;

	device_info->chip = tegra_get_chip_id();
	device_info->chip_revision = tegra_get_major_rev();
	device_info->revision = tegra_chip_get_revision();
	device_info->platform = tegra_get_platform();

	tegra_soc_hwpm_dbg("chip id 0x%x", device_info->chip);
	tegra_soc_hwpm_dbg("chip_revision 0x%x", device_info->chip_revision);
	tegra_soc_hwpm_dbg("revision 0x%x", device_info->revision);
	tegra_soc_hwpm_dbg("platform 0x%x", device_info->platform);

	return 0;
}

static int floorsweep_info_ioctl(struct tegra_soc_hwpm *hwpm,
			     void *ioctl_struct)
{
	u32 i = 0U;
	struct tegra_soc_hwpm_ip_floorsweep_info *fs_info =
		(struct tegra_soc_hwpm_ip_floorsweep_info *)ioctl_struct;

	if (fs_info->num_queries > TEGRA_SOC_HWPM_IP_QUERIES_MAX) {
		tegra_soc_hwpm_err("Number of queries exceed max limit of %u",
			TEGRA_SOC_HWPM_IP_QUERIES_MAX);
		return -EINVAL;
	}

	for (i = 0U; i < fs_info->num_queries; i++) {
		if (fs_info->ip_fsinfo[i].ip_type < TERGA_SOC_HWPM_NUM_IPS) {
			fs_info->ip_fsinfo[i].status =
				TEGRA_SOC_HWPM_IP_STATUS_VALID;
			fs_info->ip_fsinfo[i].ip_inst_mask =
				hwpm->ip_fs_info[fs_info->ip_fsinfo[i].ip_type];
		} else {
			fs_info->ip_fsinfo[i].ip_inst_mask = 0ULL;
			fs_info->ip_fsinfo[i].status =
				TEGRA_SOC_HWPM_IP_STATUS_INVALID;
		}
		tegra_soc_hwpm_dbg(
			"Query %d: ip_type %d: ip_status: %d inst_mask 0x%llx",
			i, fs_info->ip_fsinfo[i].ip_type,
			fs_info->ip_fsinfo[i].status,
			fs_info->ip_fsinfo[i].ip_inst_mask);
	}

	return 0;
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
	case addr_map_mc0_base_r():
		return &mc_fake_regs[0];
	case addr_map_mc1_base_r():
		return &mc_fake_regs[1];
	case addr_map_mc2_base_r():
		return &mc_fake_regs[2];
	case addr_map_mc3_base_r():
		return &mc_fake_regs[3];
	case addr_map_mc4_base_r():
		return &mc_fake_regs[4];
	case addr_map_mc5_base_r():
		return &mc_fake_regs[5];
	case addr_map_mc6_base_r():
		return &mc_fake_regs[6];
	case addr_map_mc7_base_r():
		return &mc_fake_regs[7];
	case addr_map_mc8_base_r():
		return &mc_fake_regs[8];
	case addr_map_mc9_base_r():
		return &mc_fake_regs[9];
	case addr_map_mc10_base_r():
		return &mc_fake_regs[10];
	case addr_map_mc11_base_r():
		return &mc_fake_regs[11];
	case addr_map_mc12_base_r():
		return &mc_fake_regs[12];
	case addr_map_mc13_base_r():
		return &mc_fake_regs[13];
	case addr_map_mc14_base_r():
		return &mc_fake_regs[14];
	case addr_map_mc15_base_r():
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
	case addr_map_mc0_base_r():
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[0];
		t234_mss_channel_map[0].fake_registers = fake_regs;
		t234_mss_iso_niso_hub_map[0].fake_registers = fake_regs;
		t234_mss_mcf_map[0].fake_registers = fake_regs;
		break;
	case addr_map_mc1_base_r():
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[1];
		t234_mss_channel_map[1].fake_registers = fake_regs;
		t234_mss_iso_niso_hub_map[1].fake_registers = fake_regs;
		t234_mss_mcf_map[1].fake_registers = fake_regs;
		break;
	case addr_map_mc2_base_r():
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[2];
		t234_mss_channel_map[2].fake_registers = fake_regs;
		t234_mss_iso_niso_hub_map[2].fake_registers = fake_regs;
		t234_mss_mcf_map[2].fake_registers = fake_regs;
		break;
	case addr_map_mc3_base_r():
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[3];
		t234_mss_channel_map[3].fake_registers = fake_regs;
		t234_mss_iso_niso_hub_map[3].fake_registers = fake_regs;
		t234_mss_mcf_map[3].fake_registers = fake_regs;
		break;
	case addr_map_mc4_base_r():
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[4];
		t234_mss_channel_map[4].fake_registers = fake_regs;
		t234_mss_iso_niso_hub_map[4].fake_registers = fake_regs;
		t234_mss_mcf_map[4].fake_registers = fake_regs;
		break;
	case addr_map_mc5_base_r():
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[5];
		t234_mss_channel_map[5].fake_registers = fake_regs;
		t234_mss_iso_niso_hub_map[5].fake_registers = fake_regs;
		t234_mss_mcf_map[5].fake_registers = fake_regs;
		break;
	case addr_map_mc6_base_r():
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[6];
		t234_mss_channel_map[6].fake_registers = fake_regs;
		t234_mss_iso_niso_hub_map[6].fake_registers = fake_regs;
		t234_mss_mcf_map[6].fake_registers = fake_regs;
		break;
	case addr_map_mc7_base_r():
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[7];
		t234_mss_channel_map[7].fake_registers = fake_regs;
		t234_mss_iso_niso_hub_map[7].fake_registers = fake_regs;
		t234_mss_mcf_map[7].fake_registers = fake_regs;
		break;
	case addr_map_mc8_base_r():
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[8];
		t234_mss_channel_map[8].fake_registers = fake_regs;
		t234_mss_iso_niso_hub_map[8].fake_registers = fake_regs;
		break;
	case addr_map_mc9_base_r():
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[9];
		t234_mss_channel_map[9].fake_registers = fake_regs;
		break;
	case addr_map_mc10_base_r():
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[10];
		t234_mss_channel_map[10].fake_registers = fake_regs;
		break;
	case addr_map_mc11_base_r():
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[11];
		t234_mss_channel_map[11].fake_registers = fake_regs;
		break;
	case addr_map_mc12_base_r():
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[12];
		t234_mss_channel_map[12].fake_registers = fake_regs;
		break;
	case addr_map_mc13_base_r():
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[13];
		t234_mss_channel_map[13].fake_registers = fake_regs;
		break;
	case addr_map_mc14_base_r():
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[14];
		t234_mss_channel_map[14].fake_registers = fake_regs;
		break;
	case addr_map_mc15_base_r():
		fake_regs = (!hwpm->fake_registers_enabled || set_null) ?
							NULL : mc_fake_regs[15];
		t234_mss_channel_map[15].fake_registers = fake_regs;
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
	struct tegra_soc_hwpm_ip_ops *ip_ops;
	int err;

	if (hwpm->bind_completed) {
		tegra_soc_hwpm_err("The RESERVE_RESOURCE IOCTL can only be"
				" called before the BIND IOCTL.");
		return -EPERM;
	}

	if (resource >= TERGA_SOC_HWPM_NUM_RESOURCES) {
		tegra_soc_hwpm_err("Requested resource %d is out of bounds.",
			resource);
		return -EINVAL;
	}

	if ((resource < TERGA_SOC_HWPM_NUM_IPS) &&
		(hwpm->ip_fs_info[resource] == 0)) {
		tegra_soc_hwpm_dbg("Requested resource %d unavailable.",
			resource);
		return 0;
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
		} else if ((aperture->dt_aperture == TEGRA_SOC_HWPM_SYS0_PERFMON_DT) ||
			((hwpm->ip_fs_info[resource] & aperture->index_mask) != 0U)) {
			if (IS_PERFMON(aperture->dt_aperture)) {
				struct resource *res = NULL;
				u64 num_regs = 0;

				tegra_soc_hwpm_dbg("Found PERFMON(0x%llx - 0x%llx)",
					aperture->start_pa, aperture->end_pa);
				ip_ops = &hwpm->ip_info[aperture->dt_aperture];
				if (ip_ops && (*ip_ops->hwpm_ip_pm)) {
					err = (*ip_ops->hwpm_ip_pm)
							(ip_ops->ip_dev, true);
					if (err) {
						tegra_soc_hwpm_err(
							"Disable Runtime PM(%d) Failed",
								aperture->dt_aperture);
					}
				} else {
					tegra_soc_hwpm_dbg(
						"No Runtime PM(%d) for IP",
								aperture->dt_aperture);
				}
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
		} else {
			tegra_soc_hwpm_dbg("resource %d index_mask %d not available",
				resource, aperture->index_mask);
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
		} else if ((aperture->dt_aperture == TEGRA_SOC_HWPM_SYS0_PERFMON_DT) ||
			((hwpm->ip_fs_info[resource] & aperture->index_mask) != 0U)) {
			if (IS_PERFMON(aperture->dt_aperture)) {
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
		tegra_soc_hwpm_err(
			"Unable to map mem_bytes buffer into kernel VA space");
		ret = -ENOMEM;
		goto fail;
	}
	memset(hwpm->mem_bytes_kernel, 0, 32);

	outbase_lo = alloc_pma_stream->stream_buf_pma_va &
			pmasys_channel_outbase_ptr_m();
	hwpm_writel(hwpm, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_channel_outbase_r(0) - addr_map_pma_base_r(),
		outbase_lo);
	tegra_soc_hwpm_dbg("OUTBASE = 0x%x", reg_val);

	outbase_hi = (alloc_pma_stream->stream_buf_pma_va >> 32) &
			pmasys_channel_outbaseupper_ptr_m();
	hwpm_writel(hwpm, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_channel_outbaseupper_r(0) - addr_map_pma_base_r(),
		outbase_hi);
	tegra_soc_hwpm_dbg("OUTBASEUPPER = 0x%x", reg_val);

	outsize = alloc_pma_stream->stream_buf_size &
			pmasys_channel_outsize_numbytes_m();
	hwpm_writel(hwpm, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_channel_outsize_r(0) - addr_map_pma_base_r(),
		outsize);
	tegra_soc_hwpm_dbg("OUTSIZE = 0x%x", reg_val);

	mem_bytes_addr = sg_dma_address(hwpm->mem_bytes_sgt->sgl) &
			pmasys_channel_mem_bytes_addr_ptr_m();
	hwpm_writel(hwpm, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_channel_mem_bytes_addr_r(0) - addr_map_pma_base_r(),
		mem_bytes_addr);
	tegra_soc_hwpm_dbg("MEM_BYTES_ADDR = 0x%x", reg_val);

	hwpm_writel(hwpm, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_channel_mem_block_r(0) - addr_map_pma_base_r(),
		pmasys_channel_mem_block_valid_f(
			pmasys_channel_mem_block_valid_true_v()));

	return 0;

fail:
	hwpm_writel(hwpm, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_channel_mem_block_r(0) - addr_map_pma_base_r(),
		pmasys_channel_mem_block_valid_f(
			pmasys_channel_mem_block_valid_false_v()));
	hwpm_writel(hwpm, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_channel_outbase_r(0) - addr_map_pma_base_r(), 0);
	hwpm_writel(hwpm, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_channel_outbaseupper_r(0) - addr_map_pma_base_r(), 0);
	hwpm_writel(hwpm, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_channel_outsize_r(0) - addr_map_pma_base_r(), 0);
	hwpm_writel(hwpm, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_channel_mem_bytes_addr_r(0) - addr_map_pma_base_r(), 0);

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
	struct hwpm_resource_aperture *aperture = NULL;

	for (res_idx = 0; res_idx < TERGA_SOC_HWPM_NUM_RESOURCES; res_idx++) {
		if (!hwpm_resources[res_idx].reserved)
			continue;
		tegra_soc_hwpm_dbg("Found reserved IP(%d)", res_idx);

		for (aprt_idx = 0;
		     aprt_idx < hwpm_resources[res_idx].map_size;
		     aprt_idx++) {
			aperture = &(hwpm_resources[res_idx].map[aprt_idx]);

			if ((res_idx == TEGRA_SOC_HWPM_RESOURCE_PMA) ||
					(res_idx == TEGRA_SOC_HWPM_RESOURCE_CMD_SLICE_RTR) ||
					(res_idx == TEGRA_SOC_HWPM_SYS0_PERFMON_DT) ||
					((hwpm->ip_fs_info[res_idx] & aperture->index_mask) != 0U)) {

				/* Zero out necessary registers */
				if (aperture->alist) {
					tegra_soc_hwpm_zero_alist_regs(hwpm, aperture);
				} else {
					tegra_soc_hwpm_err(
					"NULL allowlist in aperture(0x%llx - 0x%llx)",
						aperture->start_pa, aperture->end_pa);
				}

				/*
				 * Enable reporting of PERFMON status to
				 * NV_PERF_PMMSYS_SYS0ROUTER_PERFMONSTATUS_MERGED
				 */
				if (IS_PERFMON(aperture->dt_aperture)) {
					tegra_soc_hwpm_dbg("Found PERFMON(0x%llx - 0x%llx)",
							   aperture->start_pa,
							   aperture->end_pa);
					ret = reg_rmw(hwpm, NULL, aperture->dt_aperture,
						pmmsys_sys0_enginestatus_r(0) -
							addr_map_rpg_pm_base_r(),
						pmmsys_sys0_enginestatus_enable_m(),
						pmmsys_sys0_enginestatus_enable_out_f(),
						false, false);
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
	}

	hwpm->bind_completed = true;
	return 0;
}

static int query_allowlist_ioctl(struct tegra_soc_hwpm *hwpm,
				 void *ioctl_struct)
{
	int ret = 0;
	int res_idx = 0;
	int aprt_idx = 0;
	struct hwpm_resource_aperture *aperture = NULL;
	struct tegra_soc_hwpm_query_allowlist *query_allowlist =
			(struct tegra_soc_hwpm_query_allowlist *)ioctl_struct;

	if (!hwpm->bind_completed) {
		tegra_soc_hwpm_err("The QUERY_ALLOWLIST IOCTL can only be called"
				   " after the BIND IOCTL.");
		return -EPERM;
	}

	if (query_allowlist->allowlist != NULL) {
		/* Concatenate allowlists and return */
		ret = tegra_soc_hwpm_update_allowlist(hwpm, ioctl_struct);
		return ret;
	}

	 /* Return allowlist_size */
	if (hwpm->full_alist_size >= 0) {
		query_allowlist->allowlist_size = hwpm->full_alist_size;
		return 0;
	}

	hwpm->full_alist_size = 0;
	for (res_idx = 0; res_idx < TERGA_SOC_HWPM_NUM_RESOURCES; res_idx++) {
		if (!(hwpm_resources[res_idx].reserved))
			continue;
		tegra_soc_hwpm_dbg("Found reserved IP(%d)", res_idx);

		for (aprt_idx = 0;
		     aprt_idx < hwpm_resources[res_idx].map_size;
		     aprt_idx++) {
			aperture = &(hwpm_resources[res_idx].map[aprt_idx]);
			if (aperture->alist) {
				hwpm->full_alist_size += aperture->alist_size;
			} else {
				tegra_soc_hwpm_err(
				"NULL allowlist in aperture(0x%llx - 0x%llx)",
					aperture->start_pa, aperture->end_pa);
			}
		}
	}

	query_allowlist->allowlist_size = hwpm->full_alist_size;
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
	u64 upadted_pa = 0ULL;

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

		reg_op = &(exec_reg_ops->ops[op_idx]);
		tegra_soc_hwpm_dbg("reg op: idx(%d), phys(0x%llx), cmd(%u)",
				op_idx, reg_op->phys_addr, reg_op->cmd);

		/* The allowlist check is done here */
		aperture = find_hwpm_aperture(hwpm, reg_op->phys_addr,
						true, true, &upadted_pa);
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
							upadted_pa);
			reg_op->status = TEGRA_SOC_HWPM_REG_OP_STATUS_SUCCESS;
			break;

		case TEGRA_SOC_HWPM_REG_OP_CMD_RD64:
			reg_op->reg_val_lo = ioctl_readl(hwpm,
							 aperture,
							 upadted_pa);
			reg_op->reg_val_hi = ioctl_readl(hwpm,
							 aperture,
							 upadted_pa + 4);
			reg_op->status = TEGRA_SOC_HWPM_REG_OP_STATUS_SUCCESS;
			break;

		/* Read Modify Write operation */
		case TEGRA_SOC_HWPM_REG_OP_CMD_WR32:
			ret = reg_rmw(hwpm, aperture, aperture->dt_aperture,
				upadted_pa, reg_op->mask_lo,
				reg_op->reg_val_lo, true, aperture->is_ip);
			if (ret < 0) {
				REG_OP_FAIL(WR_FAILED,
					"WR32 REGOP failed for register(0x%llx)",
					upadted_pa);
			} else {
				reg_op->status = TEGRA_SOC_HWPM_REG_OP_STATUS_SUCCESS;
			}
			break;

		/* Read Modify Write operation */
		case TEGRA_SOC_HWPM_REG_OP_CMD_WR64:
			/* Lower 32 bits */
			ret = reg_rmw(hwpm, aperture, aperture->dt_aperture,
				upadted_pa, reg_op->mask_lo,
				reg_op->reg_val_lo, true, aperture->is_ip);
			if (ret < 0) {
				REG_OP_FAIL(WR_FAILED,
					"WR64 REGOP failed for register(0x%llx)",
					upadted_pa);
				continue;
			}

			/* Upper 32 bits */
			ret = reg_rmw(hwpm, aperture, aperture->dt_aperture,
				upadted_pa + 4, reg_op->mask_hi,
				reg_op->reg_val_hi, true, aperture->is_ip);
			if (ret < 0) {
				REG_OP_FAIL(WR_FAILED,
					"WR64 REGOP failed for register(0x%llx)",
					upadted_pa + 4);
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
	hwpm_writel(hwpm, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_channel_mem_bump_r(0) - addr_map_pma_base_r(),
		update_get_put->mem_bump);

	/* Stream MEM_BYTES value to MEM_BYTES buffer */
	if (update_get_put->b_stream_mem_bytes) {
		mem_bytes_kernel_u32 = (u32 *)(hwpm->mem_bytes_kernel);
		*mem_bytes_kernel_u32 = TEGRA_SOC_HWPM_MEM_BYTES_INVALID;
		ret = reg_rmw(hwpm, NULL, TEGRA_SOC_HWPM_PMA_DT,
			pmasys_channel_control_user_r(0) - addr_map_pma_base_r(),
			pmasys_channel_control_user_update_bytes_m(),
			pmasys_channel_control_user_update_bytes_doit_f(),
			false, false);
		if (ret < 0) {
			tegra_soc_hwpm_err("Failed to stream mem_bytes to buffer");
			return -EIO;
		}
	}

	/* Read HW put pointer */
	if (update_get_put->b_read_mem_head) {
		update_get_put->mem_head = hwpm_readl(hwpm,
			TEGRA_SOC_HWPM_PMA_DT,
			pmasys_channel_mem_head_r(0) - addr_map_pma_base_r());
		tegra_soc_hwpm_dbg("MEM_HEAD = 0x%llx",
				   update_get_put->mem_head);
	}

	/* Check overflow error status */
	if (update_get_put->b_check_overflow) {
		reg_val = hwpm_readl(hwpm, TEGRA_SOC_HWPM_PMA_DT,
			pmasys_channel_status_secure_r(0) -
			addr_map_pma_base_r());
		field_val = pmasys_channel_status_secure_membuf_status_v(
			reg_val);
		update_get_put->b_overflowed = (field_val ==
			pmasys_channel_status_secure_membuf_status_overflowed_v());
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

/*
 * Normally there is a 1-to-1 mapping between an MMIO aperture and a
 * hwpm_resource_aperture struct. But the PMA MMIO aperture is used in
 * multiple hwpm_resource_aperture structs. Therefore, we have to share the fake
 * register array between these hwpm_resource_aperture structs. This is why we
 * have to define the fake register array globally. For all other 1-to-1
 * mapping apertures the fake register arrays are directly embedded inside the
 * hwpm_resource_aperture structs.
 */
static u32 *pma_fake_regs;

static int tegra_soc_hwpm_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	unsigned int minor = iminor(inode);
	struct tegra_soc_hwpm *hwpm = NULL;
	struct resource *res = NULL;
	u32 field_mask = 0U;
	u32 field_val = 0U;
	u32 i;
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

	if (tegra_platform_is_silicon()) {
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
		/* Set required parent for la_clk */
		if (hwpm->la_clk && hwpm->la_parent_clk) {
			ret = clk_set_parent(hwpm->la_clk, hwpm->la_parent_clk);
			if (ret < 0) {
				tegra_soc_hwpm_err("la clk set parent failed");
				ret = -ENODEV;
				goto fail;
			}
		}
		/* set la_clk rate to 625 MHZ */
		ret = clk_set_rate(hwpm->la_clk, LA_CLK_RATE);
		if (ret < 0) {
			tegra_soc_hwpm_err("la clock set rate failed");
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
	}

	/* Initialize IP floorsweep info */
	tegra_soc_hwpm_dbg("Initialize IP fs info");
	for (i = 0U; i < TERGA_SOC_HWPM_NUM_IPS; i++) {
		hwpm->ip_fs_info[i] = 0ULL;
	}
	if (tegra_platform_is_vsp()) {
		/* Static IP instances as per VSP netlist */
		hwpm->ip_fs_info[TEGRA_SOC_HWPM_IP_VIC] = 0x1;
		hwpm->ip_fs_info[TEGRA_SOC_HWPM_IP_MSS_CHANNEL] = 0xF;
		hwpm->ip_fs_info[TEGRA_SOC_HWPM_IP_MSS_GPU_HUB] = 0x1;
		hwpm->ip_fs_info[TEGRA_SOC_HWPM_IP_MSS_ISO_NISO_HUBS] = 0x1;
		hwpm->ip_fs_info[TEGRA_SOC_HWPM_IP_MSS_MCF] = 0x1;
		hwpm->ip_fs_info[TEGRA_SOC_HWPM_IP_MSS_NVLINK] = 0x1;
	}
	if (tegra_platform_is_silicon()) {
		/* Static IP instances corresponding to silicon */
		// hwpm->ip_fs_info[TEGRA_SOC_HWPM_IP_VI] = 0x3;
		hwpm->ip_fs_info[TEGRA_SOC_HWPM_IP_ISP] = 0x1;
		hwpm->ip_fs_info[TEGRA_SOC_HWPM_IP_VIC] = 0x1;
		hwpm->ip_fs_info[TEGRA_SOC_HWPM_IP_OFA] = 0x1;
		hwpm->ip_fs_info[TEGRA_SOC_HWPM_IP_PVA] = 0x1;
		hwpm->ip_fs_info[TEGRA_SOC_HWPM_IP_NVDLA] = 0x3;
		// hwpm->ip_fs_info[TEGRA_SOC_HWPM_IP_MGBE] = 0xF;
		hwpm->ip_fs_info[TEGRA_SOC_HWPM_IP_SCF] = 0x1;
		hwpm->ip_fs_info[TEGRA_SOC_HWPM_IP_NVDEC] = 0x1;
		hwpm->ip_fs_info[TEGRA_SOC_HWPM_IP_NVENC] = 0x1;
		// hwpm->ip_fs_info[TEGRA_SOC_HWPM_IP_PCIE] = 0x32;
		// hwpm->ip_fs_info[TEGRA_SOC_HWPM_IP_DISPLAY] = 0x1;
		hwpm->ip_fs_info[TEGRA_SOC_HWPM_IP_MSS_CHANNEL] = 0xFFFF;
		hwpm->ip_fs_info[TEGRA_SOC_HWPM_IP_MSS_GPU_HUB] = 0x1;
		hwpm->ip_fs_info[TEGRA_SOC_HWPM_IP_MSS_ISO_NISO_HUBS] = 0x1;
		hwpm->ip_fs_info[TEGRA_SOC_HWPM_IP_MSS_MCF] = 0x1;
		hwpm->ip_fs_info[TEGRA_SOC_HWPM_IP_MSS_NVLINK] = 0x1;
	}

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
	t234_pma_map[1].start_pa = res->start;
	t234_pma_map[1].end_pa = res->end;
	t234_cmd_slice_rtr_map[0].start_pa = res->start;
	t234_cmd_slice_rtr_map[0].end_pa = res->end;
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
		t234_pma_map[1].fake_registers = pma_fake_regs;
		t234_cmd_slice_rtr_map[0].fake_registers = pma_fake_regs;
	}

	hwpm_resources[TEGRA_SOC_HWPM_RESOURCE_PMA].reserved = true;

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
	t234_cmd_slice_rtr_map[1].start_pa = res->start;
	t234_cmd_slice_rtr_map[1].end_pa = res->end;
	if (hwpm->fake_registers_enabled) {
		num_regs = (res->end + 1 - res->start) /
				sizeof(*t234_cmd_slice_rtr_map[1].fake_registers);
		t234_cmd_slice_rtr_map[1].fake_registers =
			(u32 *)kzalloc(sizeof(*t234_cmd_slice_rtr_map[1].fake_registers) *
									num_regs,
					GFP_KERNEL);
		if (!t234_cmd_slice_rtr_map[1].fake_registers) {
			tegra_soc_hwpm_err("Couldn't allocate memory for RTR"
					   " fake registers");
			ret = -ENOMEM;
			goto fail;
		}
	}
	hwpm_resources[TEGRA_SOC_HWPM_RESOURCE_CMD_SLICE_RTR].reserved = true;

	/* Disable SLCG */
	ret = reg_rmw(hwpm, NULL, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_cg2_r() - addr_map_pma_base_r(),
		pmasys_cg2_slcg_m(), pmasys_cg2_slcg_disabled_f(),
		false, false);
	if (ret < 0) {
		tegra_soc_hwpm_err("Unable to disable PMA SLCG");
		ret = -EIO;
		goto fail;
	}

	field_mask = pmmsys_sys0router_cg2_slcg_perfmon_m() |
		pmmsys_sys0router_cg2_slcg_router_m() |
		pmmsys_sys0router_cg2_slcg_m();
	field_val = pmmsys_sys0router_cg2_slcg_perfmon_disabled_f() |
		pmmsys_sys0router_cg2_slcg_router_disabled_f() |
		pmmsys_sys0router_cg2_slcg_disabled_f();
	ret = reg_rmw(hwpm, NULL, TEGRA_SOC_HWPM_RTR_DT,
		pmmsys_sys0router_cg2_r() - addr_map_rtr_base_r(),
		field_mask, field_val, false, false);
	if (ret < 0) {
		tegra_soc_hwpm_err("Unable to disable ROUTER SLCG");
		ret = -EIO;
		goto fail;
	}

	/* Program PROD values */
	ret = reg_rmw(hwpm, NULL, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_controlb_r() - addr_map_pma_base_r(),
		pmasys_controlb_coalesce_timeout_cycles_m(),
		pmasys_controlb_coalesce_timeout_cycles__prod_f(),
		false, false);
	if (ret < 0) {
		tegra_soc_hwpm_err("Unable to program PROD value");
		ret = -EIO;
		goto fail;
	}

	ret = reg_rmw(hwpm, NULL, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_channel_config_user_r(0) - addr_map_pma_base_r(),
		pmasys_channel_config_user_coalesce_timeout_cycles_m(),
		pmasys_channel_config_user_coalesce_timeout_cycles__prod_f(),
		false, false);
	if (ret < 0) {
		tegra_soc_hwpm_err("Unable to program PROD value");
		ret = -EIO;
		goto fail;
	}

	/* Initialize SW state */
	hwpm->bind_completed = false;
	hwpm->full_alist_size = -1;

	return 0;

fail:
	if (hwpm->dt_apertures[TEGRA_SOC_HWPM_PMA_DT]) {
		iounmap(hwpm->dt_apertures[TEGRA_SOC_HWPM_PMA_DT]);
		hwpm->dt_apertures[TEGRA_SOC_HWPM_PMA_DT] = NULL;
	}
	t234_pma_map[1].start_pa = 0;
	t234_pma_map[1].end_pa = 0;
	t234_cmd_slice_rtr_map[0].start_pa = 0;
	t234_cmd_slice_rtr_map[0].end_pa = 0;
	if (pma_fake_regs) {
		kfree(pma_fake_regs);
		pma_fake_regs = NULL;
		t234_pma_map[1].fake_registers = NULL;
		t234_cmd_slice_rtr_map[0].fake_registers = NULL;
	}
	hwpm_resources[TEGRA_SOC_HWPM_RESOURCE_PMA].reserved = false;

	if (hwpm->dt_apertures[TEGRA_SOC_HWPM_RTR_DT]) {
		iounmap(hwpm->dt_apertures[TEGRA_SOC_HWPM_RTR_DT]);
		hwpm->dt_apertures[TEGRA_SOC_HWPM_RTR_DT] = NULL;
	}
	t234_cmd_slice_rtr_map[1].start_pa = 0;
	t234_cmd_slice_rtr_map[1].end_pa = 0;
	if (t234_cmd_slice_rtr_map[1].fake_registers) {
		kfree(t234_cmd_slice_rtr_map[1].fake_registers);
		t234_cmd_slice_rtr_map[1].fake_registers = NULL;
	}
	hwpm_resources[TEGRA_SOC_HWPM_RESOURCE_CMD_SLICE_RTR].reserved = false;
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
	struct tegra_soc_hwpm_ip_ops *ip_ops;
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
	err = reg_rmw(hwpm, NULL, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_trigger_config_user_r(0) - addr_map_pma_base_r(),
		pmasys_trigger_config_user_pma_pulse_m(),
		pmasys_trigger_config_user_pma_pulse_disable_f(),
		false, false);
	RELEASE_FAIL("Unable to disable PMA triggers");

	hwpm_writel(hwpm, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_sys_trigger_start_mask_r() - addr_map_pma_base_r(), 0);
	hwpm_writel(hwpm, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_sys_trigger_start_maskb_r() - addr_map_pma_base_r(), 0);
	hwpm_writel(hwpm, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_sys_trigger_stop_mask_r() - addr_map_pma_base_r(), 0);
	hwpm_writel(hwpm, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_sys_trigger_stop_maskb_r() - addr_map_pma_base_r(), 0);

	/* Wait for PERFMONs, ROUTER, and PMA to idle */
	timeout = HWPM_TIMEOUT(pmmsys_sys0router_perfmonstatus_merged_v(
		hwpm_readl(hwpm, TEGRA_SOC_HWPM_RTR_DT,
			pmmsys_sys0router_perfmonstatus_r() -
			addr_map_rtr_base_r())) == 0U,
		"NV_PERF_PMMSYS_SYS0ROUTER_PERFMONSTATUS_MERGED_EMPTY");
	if (timeout && ret == 0) {
		ret = -EIO;
	}

	timeout = HWPM_TIMEOUT(pmmsys_sys0router_enginestatus_status_v(
		hwpm_readl(hwpm, TEGRA_SOC_HWPM_RTR_DT,
			pmmsys_sys0router_enginestatus_r() -
			addr_map_rtr_base_r())) ==
		pmmsys_sys0router_enginestatus_status_empty_v(),
		"NV_PERF_PMMSYS_SYS0ROUTER_ENGINESTATUS_STATUS_EMPTY");
	if (timeout && ret == 0) {
		ret = -EIO;
	}

	field_mask = pmasys_enginestatus_status_m() |
		     pmasys_enginestatus_rbufempty_m();
	field_val = pmasys_enginestatus_status_empty_f() |
		pmasys_enginestatus_rbufempty_empty_f();
	timeout = HWPM_TIMEOUT((hwpm_readl(hwpm, TEGRA_SOC_HWPM_PMA_DT,
			pmasys_enginestatus_r() -
			addr_map_pma_base_r()) & field_mask) == field_val,
		"NV_PERF_PMASYS_ENGINESTATUS");
	if (timeout && ret == 0) {
		ret = -EIO;
	}

	hwpm_resources[TEGRA_SOC_HWPM_RESOURCE_PMA].reserved = false;
	hwpm_resources[TEGRA_SOC_HWPM_RESOURCE_CMD_SLICE_RTR].reserved = false;

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
				if ((res_idx == TEGRA_SOC_HWPM_RESOURCE_PMA) ||
					(res_idx == TEGRA_SOC_HWPM_RESOURCE_CMD_SLICE_RTR) ||
					(res_idx == TEGRA_SOC_HWPM_SYS0_PERFMON_DT) ||
					((hwpm->ip_fs_info[res_idx] & aperture->index_mask) != 0U)) {
					tegra_soc_hwpm_dbg("Found PERFMON(0x%llx - 0x%llx)",
							   aperture->start_pa,
							   aperture->end_pa);
					err = reg_rmw(hwpm, NULL, aperture->dt_aperture,
						pmmsys_control_r(0) - addr_map_rpg_pm_base_r(),
						pmmsys_control_mode_m(),
						pmmsys_control_mode_disable_f(),
						false, false);
					RELEASE_FAIL("Unable to disable PERFMON(0x%llx - 0x%llx)",
						     aperture->start_pa,
						     aperture->end_pa);
					ip_ops = &hwpm->ip_info[aperture->dt_aperture];
					if (ip_ops && (*ip_ops->hwpm_ip_pm)) {
						err = (*ip_ops->hwpm_ip_pm)
									(ip_ops->ip_dev, false);
						if (err) {
							tegra_soc_hwpm_err(
								"Enable Runtime PM(%d) Failed",
									aperture->dt_aperture);
						}
					} else {
						tegra_soc_hwpm_dbg(
							"No Runtime PM(%d) for IP",
									aperture->dt_aperture);
					}
				}
			}
		}
	}

	/* Stream MEM_BYTES to clear pipeline */
	if (hwpm->mem_bytes_kernel) {
		mem_bytes_kernel_u32 = (u32 *)(hwpm->mem_bytes_kernel);
		*mem_bytes_kernel_u32 = TEGRA_SOC_HWPM_MEM_BYTES_INVALID;
		err = reg_rmw(hwpm, NULL, TEGRA_SOC_HWPM_PMA_DT,
			pmasys_channel_control_user_r(0) - addr_map_pma_base_r(),
			pmasys_channel_control_user_update_bytes_m(),
			pmasys_channel_control_user_update_bytes_doit_f(),
			false, false);
		RELEASE_FAIL("Unable to stream MEM_BYTES");
		timeout = HWPM_TIMEOUT(*mem_bytes_kernel_u32 !=
				       TEGRA_SOC_HWPM_MEM_BYTES_INVALID,
			     "MEM_BYTES streaming");
		if (timeout && ret == 0)
			ret = -EIO;
	}

	/* Disable PMA streaming */
	err = reg_rmw(hwpm, NULL, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_trigger_config_user_r(0) - addr_map_pma_base_r(),
		pmasys_trigger_config_user_record_stream_m(),
		pmasys_trigger_config_user_record_stream_disable_f(),
		false, false);
	RELEASE_FAIL("Unable to disable PMA streaming");

	err = reg_rmw(hwpm, NULL, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_channel_control_user_r(0) - addr_map_pma_base_r(),
		pmasys_channel_control_user_stream_m(),
		pmasys_channel_control_user_stream_disable_f(),
		false, false);
	RELEASE_FAIL("Unable to disable PMA streaming");

	/* Memory Management */
	hwpm_writel(hwpm, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_channel_outbase_r(0) - addr_map_pma_base_r(), 0);
	hwpm_writel(hwpm, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_channel_outbaseupper_r(0) - addr_map_pma_base_r(), 0);
	hwpm_writel(hwpm, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_channel_outsize_r(0) - addr_map_pma_base_r(), 0);
	hwpm_writel(hwpm, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_channel_mem_bytes_addr_r(0) - addr_map_pma_base_r(), 0);

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

	/* Enable SLCG */
	err = reg_rmw(hwpm, NULL, TEGRA_SOC_HWPM_PMA_DT,
		pmasys_cg2_r() - addr_map_pma_base_r(),
		pmasys_cg2_slcg_m(),
		pmasys_cg2_slcg_enabled_f(), false, false);
	RELEASE_FAIL("Unable to enable PMA SLCG");

	field_mask = pmmsys_sys0router_cg2_slcg_perfmon_m() |
		pmmsys_sys0router_cg2_slcg_router_m() |
		pmmsys_sys0router_cg2_slcg_m();
	field_val = pmmsys_sys0router_cg2_slcg_perfmon__prod_f() |
		pmmsys_sys0router_cg2_slcg_router__prod_f() |
		pmmsys_sys0router_cg2_slcg__prod_f();
	err = reg_rmw(hwpm, NULL, TEGRA_SOC_HWPM_RTR_DT,
		pmmsys_sys0router_cg2_r() - addr_map_rtr_base_r(),
		field_mask, field_val, false, false);
	RELEASE_FAIL("Unable to enable ROUTER SLCG");

	/* Unmap PMA and RTR apertures */
	tegra_soc_hwpm_dbg("Unmapping apertures");
	if (hwpm->dt_apertures[TEGRA_SOC_HWPM_PMA_DT]) {
		iounmap(hwpm->dt_apertures[TEGRA_SOC_HWPM_PMA_DT]);
		hwpm->dt_apertures[TEGRA_SOC_HWPM_PMA_DT] = NULL;
	}
	t234_pma_map[1].start_pa = 0;
	t234_pma_map[1].end_pa = 0;
	t234_cmd_slice_rtr_map[0].start_pa = 0;
	t234_cmd_slice_rtr_map[0].end_pa = 0;
	if (pma_fake_regs) {
		kfree(pma_fake_regs);
		pma_fake_regs = NULL;
		t234_pma_map[1].fake_registers = NULL;
		t234_cmd_slice_rtr_map[0].fake_registers = NULL;
	}
	if (hwpm->dt_apertures[TEGRA_SOC_HWPM_RTR_DT]) {
		iounmap(hwpm->dt_apertures[TEGRA_SOC_HWPM_RTR_DT]);
		hwpm->dt_apertures[TEGRA_SOC_HWPM_RTR_DT] = NULL;
	}
	t234_cmd_slice_rtr_map[1].start_pa = 0;
	t234_cmd_slice_rtr_map[1].end_pa = 0;
	if (t234_cmd_slice_rtr_map[1].fake_registers) {
		kfree(t234_cmd_slice_rtr_map[1].fake_registers);
		t234_cmd_slice_rtr_map[1].fake_registers = NULL;
	}

	/* Reset resource and aperture state */
	for (res_idx = 0; res_idx < TERGA_SOC_HWPM_NUM_RESOURCES; res_idx++) {
		if (!hwpm_resources[res_idx].reserved)
			continue;
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

	if (tegra_platform_is_silicon()) {
		err = reset_control_assert(hwpm->hwpm_rst);
		RELEASE_FAIL("hwpm reset assert failed");
		err = reset_control_assert(hwpm->la_rst);
		RELEASE_FAIL("la reset assert failed");
		clk_disable_unprepare(hwpm->la_clk);
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
