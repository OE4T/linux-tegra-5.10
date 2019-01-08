/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <linux/mutex.h>
#ifdef CONFIG_TEGRA_NVLINK
#include <nvlink/common/tegra-nvlink.h>
#endif

#include <nvgpu/gk20a.h>
#include <nvgpu/nvlink.h>
#include <nvgpu/enabled.h>
#include <nvgpu/firmware.h>
#include "module.h"

#ifdef CONFIG_TEGRA_NVLINK
int nvgpu_nvlink_read_dt_props(struct gk20a *g)
{
	struct device_node *np;
	struct nvlink_device *ndev = g->nvlink.priv;
	u32 local_dev_id;
	u32 local_link_id;
	u32 remote_dev_id;
	u32 remote_link_id;
	bool is_master;

	/* Parse DT */
	np = nvgpu_get_node(g);
	if (!np)
		goto fail;

	np = of_get_child_by_name(np, "nvidia,nvlink");
	if (!np)
		goto fail;

	np = of_get_child_by_name(np, "endpoint");
	if (!np)
		goto fail;

	/* Parse DT structure to detect endpoint topology */
	of_property_read_u32(np, "local_dev_id", &local_dev_id);
	of_property_read_u32(np, "local_link_id", &local_link_id);
	of_property_read_u32(np, "remote_dev_id", &remote_dev_id);
	of_property_read_u32(np, "remote_link_id", &remote_link_id);
	is_master = of_property_read_bool(np, "is_master");

	/* Check that we are in dGPU mode */
	if (local_dev_id != NVLINK_ENDPT_GV100) {
		nvgpu_err(g, "Local nvlink device is not dGPU");
		return -EINVAL;
	}

	ndev->is_master = is_master;
	ndev->device_id = local_dev_id;
	ndev->link.link_id = local_link_id;
	ndev->link.remote_dev_info.device_id = remote_dev_id;
	ndev->link.remote_dev_info.link_id = remote_link_id;

	return 0;

fail:
	nvgpu_info(g, "nvlink endpoint not found or invaling in DT");
	return -ENODEV;
}

static int nvgpu_nvlink_ops_speed_config(struct nvlink_device *ndev)
{
	struct gk20a *g = (struct gk20a *) ndev->priv;
	int err;

	err = nvgpu_nvlink_speed_config(g);
	if (err != 0) {
		nvgpu_err(g, "Nvlink speed config failed.\n");
	} else {
		ndev->speed = g->nvlink.speed;
		nvgpu_log(g, gpu_dbg_nvlink, "Nvlink default speed set to %d\n",
					ndev->speed);
	}

	return err;
}

static int nvgpu_nvlink_ops_early_init(struct nvlink_device *ndev)
{
	struct gk20a *g = (struct gk20a *) ndev->priv;

	return nvgpu_nvlink_early_init(g);
}

static int nvgpu_nvlink_ops_link_early_init(struct nvlink_device *ndev)
{
	struct gk20a *g = (struct gk20a *) ndev->priv;

	return nvgpu_nvlink_link_early_init(g);
}

static int nvgpu_nvlink_ops_interface_init(struct nvlink_device *ndev)
{
	struct gk20a *g = (struct gk20a *) ndev->priv;

	return nvgpu_nvlink_interface_init(g);
}

static int nvgpu_nvlink_ops_interface_disable(struct nvlink_device *ndev)
{
	struct gk20a *g = (struct gk20a *) ndev->priv;

	return nvgpu_nvlink_interface_disable(g);
}

static int nvgpu_nvlink_ops_dev_shutdown(struct nvlink_device *ndev)
{
	struct gk20a *g = (struct gk20a *) ndev->priv;

	return nvgpu_nvlink_dev_shutdown(g);
}

static int nvgpu_nvlink_ops_reg_init(struct nvlink_device *ndev)
{
	struct gk20a *g = (struct gk20a *) ndev->priv;

	return nvgpu_nvlink_reg_init(g);
}

static u32 nvgpu_nvlink_ops_get_link_mode(struct nvlink_device *ndev)
{
	struct gk20a *g = (struct gk20a *) ndev->priv;
	u32 mode;

	mode = nvgpu_nvlink_get_link_mode(g);

	switch (mode) {
	case nvgpu_nvlink_link_off:
		return NVLINK_LINK_OFF;
	case nvgpu_nvlink_link_hs:
		return NVLINK_LINK_HS;
	case nvgpu_nvlink_link_safe:
		return NVLINK_LINK_SAFE;
	case nvgpu_nvlink_link_fault:
		return NVLINK_LINK_FAULT;
	case nvgpu_nvlink_link_rcvy_ac:
		return NVLINK_LINK_RCVY_AC;
	case nvgpu_nvlink_link_rcvy_sw:
		return NVLINK_LINK_RCVY_SW;
	case nvgpu_nvlink_link_rcvy_rx:
		return NVLINK_LINK_RCVY_RX;
	case nvgpu_nvlink_link_detect:
		return NVLINK_LINK_DETECT;
	case nvgpu_nvlink_link_reset:
		return NVLINK_LINK_RESET;
	case nvgpu_nvlink_link_enable_pm:
		return NVLINK_LINK_ENABLE_PM;
	case nvgpu_nvlink_link_disable_pm:
		return NVLINK_LINK_DISABLE_PM;
	case nvgpu_nvlink_link_disable_err_detect:
		return NVLINK_LINK_DISABLE_ERR_DETECT;
	case nvgpu_nvlink_link_lane_disable:
		return NVLINK_LINK_LANE_DISABLE;
	case nvgpu_nvlink_link_lane_shutdown:
		return NVLINK_LINK_LANE_SHUTDOWN;
	default:
		nvgpu_log(g, gpu_dbg_info | gpu_dbg_nvlink,
						"unsupported mode %u", mode);
	}

	return NVLINK_LINK_OFF;
}

static u32 nvgpu_nvlink_ops_get_link_state(struct nvlink_device *ndev)
{
	struct gk20a *g = (struct gk20a *) ndev->priv;

	return nvgpu_nvlink_get_link_state(g);
}

static int nvgpu_nvlink_ops_set_link_mode(struct nvlink_device *ndev, u32 mode)
{
	struct gk20a *g = (struct gk20a *) ndev->priv;
	u32 mode_sw;

	switch (mode) {
	case NVLINK_LINK_OFF:
		mode_sw = nvgpu_nvlink_link_off;
		break;
	case NVLINK_LINK_HS:
		mode_sw = nvgpu_nvlink_link_hs;
		break;
	case NVLINK_LINK_SAFE:
		mode_sw = nvgpu_nvlink_link_safe;
		break;
	case NVLINK_LINK_FAULT:
		mode_sw = nvgpu_nvlink_link_fault;
		break;
	case NVLINK_LINK_RCVY_AC:
		mode_sw = nvgpu_nvlink_link_rcvy_ac;
		break;
	case NVLINK_LINK_RCVY_SW:
		mode_sw = nvgpu_nvlink_link_rcvy_sw;
		break;
	case NVLINK_LINK_RCVY_RX:
		mode_sw = nvgpu_nvlink_link_rcvy_rx;
		break;
	case NVLINK_LINK_DETECT:
		mode_sw = nvgpu_nvlink_link_detect;
		break;
	case NVLINK_LINK_RESET:
		mode_sw = nvgpu_nvlink_link_reset;
		break;
	case NVLINK_LINK_ENABLE_PM:
		mode_sw = nvgpu_nvlink_link_enable_pm;
		break;
	case NVLINK_LINK_DISABLE_PM:
		mode_sw = nvgpu_nvlink_link_disable_pm;
		break;
	case NVLINK_LINK_DISABLE_ERR_DETECT:
		mode_sw = nvgpu_nvlink_link_disable_err_detect;
		break;
	case NVLINK_LINK_LANE_DISABLE:
		mode_sw = nvgpu_nvlink_link_lane_disable;
		break;
	case NVLINK_LINK_LANE_SHUTDOWN:
		mode_sw = nvgpu_nvlink_link_lane_shutdown;
		break;
	default:
		mode_sw = nvgpu_nvlink_link_off;
	}

	return nvgpu_nvlink_set_link_mode(g, mode_sw);
}

static void nvgpu_nvlink_ops_get_tx_sublink_state(struct nvlink_device *ndev,
					u32 *tx_sublink_state)
{
	struct gk20a *g = (struct gk20a *) ndev->priv;

	return nvgpu_nvlink_get_tx_sublink_state(g, tx_sublink_state);
}

static void nvgpu_nvlink_ops_get_rx_sublink_state(struct nvlink_device *ndev,
					u32 *rx_sublink_state)
{
	struct gk20a *g = (struct gk20a *) ndev->priv;

	return nvgpu_nvlink_get_rx_sublink_state(g, rx_sublink_state);
}

static u32 nvgpu_nvlink_ops_get_sublink_mode(struct nvlink_device *ndev,
					bool is_rx_sublink)
{
	struct gk20a *g = (struct gk20a *) ndev->priv;
	u32 mode;

	mode = nvgpu_nvlink_get_sublink_mode(g, is_rx_sublink);

	switch (mode) {
	case nvgpu_nvlink_sublink_tx_hs:
		return NVLINK_TX_HS;
	case nvgpu_nvlink_sublink_tx_off:
		return NVLINK_TX_OFF;
	case nvgpu_nvlink_sublink_tx_single_lane:
		return NVLINK_TX_SINGLE_LANE;
	case nvgpu_nvlink_sublink_tx_safe:
		return NVLINK_TX_SAFE;
	case nvgpu_nvlink_sublink_tx_enable_pm:
		return NVLINK_TX_ENABLE_PM;
	case nvgpu_nvlink_sublink_tx_disable_pm:
		return NVLINK_TX_DISABLE_PM;
	case nvgpu_nvlink_sublink_tx_common:
		return NVLINK_TX_COMMON;
	case nvgpu_nvlink_sublink_tx_common_disable:
		return NVLINK_TX_COMMON_DISABLE;
	case nvgpu_nvlink_sublink_tx_data_ready:
		return NVLINK_TX_DATA_READY;
	case nvgpu_nvlink_sublink_tx_prbs_en:
		return NVLINK_TX_PRBS_EN;
	case nvgpu_nvlink_sublink_rx_hs:
		return NVLINK_RX_HS;
	case nvgpu_nvlink_sublink_rx_enable_pm:
		return NVLINK_RX_ENABLE_PM;
	case nvgpu_nvlink_sublink_rx_disable_pm:
		return NVLINK_RX_DISABLE_PM;
	case nvgpu_nvlink_sublink_rx_single_lane:
		return NVLINK_RX_SINGLE_LANE;
	case nvgpu_nvlink_sublink_rx_safe:
		return NVLINK_RX_SAFE;
	case nvgpu_nvlink_sublink_rx_off:
		return NVLINK_RX_OFF;
	case nvgpu_nvlink_sublink_rx_rxcal:
		return NVLINK_RX_RXCAL;
	default:
		nvgpu_log(g, gpu_dbg_nvlink, "Unsupported mode: %u", mode);
		break;
	}

	if (is_rx_sublink)
		return NVLINK_RX_OFF;
	return NVLINK_TX_OFF;
}

static int nvgpu_nvlink_ops_set_sublink_mode(struct nvlink_device *ndev,
						bool is_rx_sublink, u32 mode)
{
	struct gk20a *g = (struct gk20a *) ndev->priv;
	u32 mode_sw;

	if (!is_rx_sublink) {
		switch (mode) {
		case NVLINK_TX_HS:
			mode_sw = nvgpu_nvlink_sublink_tx_hs;
			break;
		case NVLINK_TX_ENABLE_PM:
			mode_sw = nvgpu_nvlink_sublink_tx_enable_pm;
			break;
		case NVLINK_TX_DISABLE_PM:
			mode_sw = nvgpu_nvlink_sublink_tx_disable_pm;
			break;
		case NVLINK_TX_SINGLE_LANE:
			mode_sw = nvgpu_nvlink_sublink_tx_single_lane;
			break;
		case NVLINK_TX_SAFE:
			mode_sw = nvgpu_nvlink_sublink_tx_safe;
			break;
		case NVLINK_TX_OFF:
			mode_sw = nvgpu_nvlink_sublink_tx_off;
			break;
		case NVLINK_TX_COMMON:
			mode_sw = nvgpu_nvlink_sublink_tx_common;
			break;
		case NVLINK_TX_COMMON_DISABLE:
			mode_sw = nvgpu_nvlink_sublink_tx_common_disable;
			break;
		case NVLINK_TX_DATA_READY:
			mode_sw = nvgpu_nvlink_sublink_tx_data_ready;
			break;
		case NVLINK_TX_PRBS_EN:
			mode_sw = nvgpu_nvlink_sublink_tx_prbs_en;
			break;
		default:
			return -EINVAL;
		}
	} else {
		switch (mode) {
		case NVLINK_RX_HS:
			mode_sw = nvgpu_nvlink_sublink_rx_hs;
			break;
		case NVLINK_RX_ENABLE_PM:
			mode_sw = nvgpu_nvlink_sublink_rx_enable_pm;
			break;
		case NVLINK_RX_DISABLE_PM:
			mode_sw = nvgpu_nvlink_sublink_rx_disable_pm;
			break;
		case NVLINK_RX_SINGLE_LANE:
			mode_sw = nvgpu_nvlink_sublink_rx_single_lane;
			break;
		case NVLINK_RX_SAFE:
			mode_sw = nvgpu_nvlink_sublink_rx_safe;
			break;
		case NVLINK_RX_OFF:
			mode_sw = nvgpu_nvlink_sublink_rx_off;
			break;
		case NVLINK_RX_RXCAL:
			mode_sw = nvgpu_nvlink_sublink_rx_rxcal;
			break;
		default:
			return -EINVAL;
		}
	}

	return nvgpu_nvlink_set_sublink_mode(g, is_rx_sublink, mode_sw);
}

int nvgpu_nvlink_setup_ndev(struct gk20a *g)
{
	struct nvlink_device *ndev;

	/* Allocating structures */
	ndev = nvgpu_kzalloc(g, sizeof(struct nvlink_device));
	if (!ndev) {
		nvgpu_err(g, "OOM while allocating nvlink device struct");
		return -ENOMEM;
	}
	ndev->priv = (void *) g;
	g->nvlink.priv = (void *) ndev;

	return 0;
}

int nvgpu_nvlink_init_ops(struct gk20a *g)
{
	struct nvlink_device *ndev = (struct nvlink_device *) g->nvlink.priv;

	if (!ndev)
		return -EINVAL;

	/* Fill in device struct */
	ndev->dev_ops.dev_early_init = nvgpu_nvlink_ops_early_init;
	ndev->dev_ops.dev_interface_init = nvgpu_nvlink_ops_interface_init;
	ndev->dev_ops.dev_reg_init = nvgpu_nvlink_ops_reg_init;
	ndev->dev_ops.dev_interface_disable =
					nvgpu_nvlink_ops_interface_disable;
	ndev->dev_ops.dev_shutdown = nvgpu_nvlink_ops_dev_shutdown;
	ndev->dev_ops.dev_speed_config = nvgpu_nvlink_ops_speed_config;

	/* Fill in the link struct */
	ndev->link.device_id = ndev->device_id;
	ndev->link.mode = NVLINK_LINK_OFF;
	ndev->link.is_sl_supported = false;
	ndev->link.link_ops.get_link_mode = nvgpu_nvlink_ops_get_link_mode;
	ndev->link.link_ops.set_link_mode = nvgpu_nvlink_ops_set_link_mode;
	ndev->link.link_ops.get_sublink_mode =
					nvgpu_nvlink_ops_get_sublink_mode;
	ndev->link.link_ops.set_sublink_mode =
					nvgpu_nvlink_ops_set_sublink_mode;
	ndev->link.link_ops.get_link_state = nvgpu_nvlink_ops_get_link_state;
	ndev->link.link_ops.get_tx_sublink_state =
					nvgpu_nvlink_ops_get_tx_sublink_state;
	ndev->link.link_ops.get_rx_sublink_state =
					nvgpu_nvlink_ops_get_rx_sublink_state;
	ndev->link.link_ops.link_early_init =
					nvgpu_nvlink_ops_link_early_init;

	return 0;
}

int nvgpu_nvlink_register_device(struct gk20a *g)
{
	struct nvlink_device *ndev = (struct nvlink_device *) g->nvlink.priv;

	if (!ndev)
		return -ENODEV;

	return nvlink_register_device(ndev);
}

int nvgpu_nvlink_unregister_device(struct gk20a *g)
{
	struct nvlink_device *ndev = (struct nvlink_device *) g->nvlink.priv;

	if (!ndev)
		return -ENODEV;

	return nvlink_unregister_device(ndev);
}

int nvgpu_nvlink_register_link(struct gk20a *g)
{
	struct nvlink_device *ndev = (struct nvlink_device *) g->nvlink.priv;

	if (!ndev)
		return -ENODEV;

	return nvlink_register_link(&ndev->link);
}

int nvgpu_nvlink_unregister_link(struct gk20a *g)
{
	struct nvlink_device *ndev = (struct nvlink_device *) g->nvlink.priv;

	if (!ndev)
		return -ENODEV;

	return nvlink_unregister_link(&ndev->link);
}

int nvgpu_nvlink_enumerate(struct gk20a *g)
{
	struct nvlink_device *ndev = (struct nvlink_device *) g->nvlink.priv;

	if (!ndev)
		return -ENODEV;

	return nvlink_enumerate(ndev);
}

int nvgpu_nvlink_train(struct gk20a *g, u32 link_id, bool from_off)
{
	struct nvlink_device *ndev = (struct nvlink_device *) g->nvlink.priv;

	if (!ndev)
		return -ENODEV;

	/* Check if the link is connected */
	if (!g->nvlink.links[link_id].remote_info.is_connected)
		return -ENODEV;

	if (from_off)
		return nvlink_transition_intranode_conn_off_to_safe(ndev);

	return nvlink_train_intranode_conn_safe_to_hs(ndev);
}

void nvgpu_nvlink_free_minion_used_mem(struct gk20a *g,
					struct nvgpu_firmware *nvgpu_minion_fw)
{
	struct nvlink_device *ndev = (struct nvlink_device *) g->nvlink.priv;
	struct minion_hdr *minion_hdr = &ndev->minion_hdr;

	nvgpu_kfree(g, minion_hdr->app_code_offsets);
	nvgpu_kfree(g, minion_hdr->app_code_sizes);
	nvgpu_kfree(g, minion_hdr->app_data_offsets);
	nvgpu_kfree(g, minion_hdr->app_data_sizes);

	if (nvgpu_minion_fw) {
		nvgpu_release_firmware(g, nvgpu_minion_fw);
		ndev->minion_img = NULL;
	}
}

/*
 * Load minion FW
 */
u32 nvgpu_nvlink_minion_load_ucode(struct gk20a *g,
					struct nvgpu_firmware *nvgpu_minion_fw)
{
	u32 err = 0;
	struct nvlink_device *ndev = (struct nvlink_device *) g->nvlink.priv;
	struct minion_hdr *minion_hdr = &ndev->minion_hdr;
	u32 data_idx = 0;
	u32 app = 0;

	nvgpu_log_fn(g, " ");

	/* Read ucode header */
	minion_hdr->os_code_offset = nvgpu_nvlink_minion_extract_word(
							nvgpu_minion_fw,
							data_idx);
	data_idx += 4;
	minion_hdr->os_code_size = nvgpu_nvlink_minion_extract_word(
							nvgpu_minion_fw,
							data_idx);
	data_idx += 4;
	minion_hdr->os_data_offset = nvgpu_nvlink_minion_extract_word(
							nvgpu_minion_fw,
							data_idx);
	data_idx += 4;
	minion_hdr->os_data_size = nvgpu_nvlink_minion_extract_word(
							nvgpu_minion_fw,
							data_idx);
	data_idx += 4;
	minion_hdr->num_apps = nvgpu_nvlink_minion_extract_word(
							nvgpu_minion_fw,
							data_idx);
	data_idx += 4;

	nvgpu_log(g, gpu_dbg_nvlink,
		"MINION Ucode Header Info:");
	nvgpu_log(g, gpu_dbg_nvlink,
		"-------------------------");
	nvgpu_log(g, gpu_dbg_nvlink,
		"  - OS Code Offset = %u", minion_hdr->os_code_offset);
	nvgpu_log(g, gpu_dbg_nvlink,
		"  - OS Code Size = %u", minion_hdr->os_code_size);
	nvgpu_log(g, gpu_dbg_nvlink,
		"  - OS Data Offset = %u", minion_hdr->os_data_offset);
	nvgpu_log(g, gpu_dbg_nvlink,
		"  - OS Data Size = %u", minion_hdr->os_data_size);
	nvgpu_log(g, gpu_dbg_nvlink,
		"  - Num Apps = %u", minion_hdr->num_apps);

	/* Allocate offset/size arrays for all the ucode apps */
	minion_hdr->app_code_offsets = nvgpu_kcalloc(g,
						minion_hdr->num_apps,
						sizeof(u32));
	if (!minion_hdr->app_code_offsets) {
		nvgpu_err(g, "Couldn't allocate MINION app_code_offsets array");
		return -ENOMEM;
	}

	minion_hdr->app_code_sizes = nvgpu_kcalloc(g,
						minion_hdr->num_apps,
						sizeof(u32));
	if (!minion_hdr->app_code_sizes) {
		nvgpu_err(g, "Couldn't allocate MINION app_code_sizes array");
		return -ENOMEM;
	}

	minion_hdr->app_data_offsets = nvgpu_kcalloc(g,
						minion_hdr->num_apps,
						sizeof(u32));
	if (!minion_hdr->app_data_offsets) {
		nvgpu_err(g, "Couldn't allocate MINION app_data_offsets array");
		return -ENOMEM;
	}

	minion_hdr->app_data_sizes = nvgpu_kcalloc(g,
						minion_hdr->num_apps,
						sizeof(u32));
	if (!minion_hdr->app_data_sizes) {
		nvgpu_err(g, "Couldn't allocate MINION app_data_sizes array");
		return -ENOMEM;
	}

	/* Get app code offsets and sizes */
	for (app = 0; app < minion_hdr->num_apps; app++) {
		minion_hdr->app_code_offsets[app] =
				nvgpu_nvlink_minion_extract_word(
							nvgpu_minion_fw,
							data_idx);
		data_idx += 4;
		minion_hdr->app_code_sizes[app] =
				nvgpu_nvlink_minion_extract_word(
							nvgpu_minion_fw,
							data_idx);
		data_idx += 4;

		nvgpu_log(g, gpu_dbg_nvlink,
			"  - App Code:");
		nvgpu_log(g, gpu_dbg_nvlink,
			"      - App #%d: Code Offset = %u, Code Size = %u",
			app,
			minion_hdr->app_code_offsets[app],
			minion_hdr->app_code_sizes[app]);
	}

	/* Get app data offsets and sizes */
	for (app = 0; app < minion_hdr->num_apps; app++) {
		minion_hdr->app_data_offsets[app] =
				nvgpu_nvlink_minion_extract_word(
							nvgpu_minion_fw,
							data_idx);
		data_idx += 4;
		minion_hdr->app_data_sizes[app] =
				nvgpu_nvlink_minion_extract_word(
							nvgpu_minion_fw,
							data_idx);
		data_idx += 4;

		nvgpu_log(g, gpu_dbg_nvlink,
			"  - App Data:");
		nvgpu_log(g, gpu_dbg_nvlink,
			"      - App #%d: Data Offset = %u, Data Size = %u",
			app,
			minion_hdr->app_data_offsets[app],
			minion_hdr->app_data_sizes[app]);
	}

	minion_hdr->ovl_offset = nvgpu_nvlink_minion_extract_word(
							nvgpu_minion_fw,
							data_idx);
	data_idx += 4;
	minion_hdr->ovl_size = nvgpu_nvlink_minion_extract_word(
							nvgpu_minion_fw,
							data_idx);
	data_idx += 4;

	ndev->minion_img = &(nvgpu_minion_fw->data[data_idx]);
	minion_hdr->ucode_data_size = nvgpu_minion_fw->size - data_idx;

	nvgpu_log(g, gpu_dbg_nvlink,
		"  - Overlay Offset = %u", minion_hdr->ovl_offset);
	nvgpu_log(g, gpu_dbg_nvlink,
		"  - Overlay Size = %u", minion_hdr->ovl_size);
	nvgpu_log(g, gpu_dbg_nvlink,
		"  - Ucode Data Size = %u", minion_hdr->ucode_data_size);

	/* Copy Non Secure IMEM code */
	nvgpu_falcon_copy_to_imem(g->minion_flcn, 0,
		(u8 *)&ndev->minion_img[minion_hdr->os_code_offset],
		minion_hdr->os_code_size, 0, false,
		GET_IMEM_TAG(minion_hdr->os_code_offset));

	/* Copy Non Secure DMEM code */
	nvgpu_falcon_copy_to_dmem(g->minion_flcn, 0,
		(u8 *)&ndev->minion_img[minion_hdr->os_data_offset],
		minion_hdr->os_data_size, 0);

	/* Load the apps securely */
	for (app = 0; app < minion_hdr->num_apps; app++) {
		u32 app_code_start = minion_hdr->app_code_offsets[app];
		u32 app_code_size = minion_hdr->app_code_sizes[app];
		u32 app_data_start = minion_hdr->app_data_offsets[app];
		u32 app_data_size = minion_hdr->app_data_sizes[app];

		if (app_code_size)
			nvgpu_falcon_copy_to_imem(g->minion_flcn,
				app_code_start,
				(u8 *)&ndev->minion_img[app_code_start],
				app_code_size, 0, true,
				GET_IMEM_TAG(app_code_start));

		if (app_data_size)
			nvgpu_falcon_copy_to_dmem(g->minion_flcn,
				app_data_start,
				(u8 *)&ndev->minion_img[app_data_start],
				app_data_size, 0);
	}

	return err;
}

#endif /* CONFIG_TEGRA_NVLINK */

void nvgpu_mss_nvlink_init_credits(struct gk20a *g)
{
		/* MSS_NVLINK_1_BASE */
		void __iomem *soc1 = ioremap(0x01f20010, 4096);
		/* MSS_NVLINK_2_BASE */
		void __iomem *soc2 = ioremap(0x01f40010, 4096);
		/* MSS_NVLINK_3_BASE */
		void __iomem *soc3 = ioremap(0x01f60010, 4096);
		/* MSS_NVLINK_4_BASE */
		void __iomem *soc4 = ioremap(0x01f80010, 4096);
		u32 val;

		nvgpu_log(g, gpu_dbg_info, "init nvlink soc credits");

		val = readl_relaxed(soc1);
		writel_relaxed(val, soc1);
		val = readl_relaxed(soc1 + 4);
		writel_relaxed(val, soc1 + 4);

		val = readl_relaxed(soc2);
		writel_relaxed(val, soc2);
		val = readl_relaxed(soc2 + 4);
		writel_relaxed(val, soc2 + 4);

		val = readl_relaxed(soc3);
		writel_relaxed(val, soc3);
		val = readl_relaxed(soc3 + 4);
		writel_relaxed(val, soc3 + 4);

		val = readl_relaxed(soc4);
		writel_relaxed(val, soc4);
		val = readl_relaxed(soc4 + 4);
		writel_relaxed(val, soc4 + 4);
}

int nvgpu_nvlink_deinit(struct gk20a *g)
{
#ifdef CONFIG_TEGRA_NVLINK
	struct nvlink_device *ndev = g->nvlink.priv;
	int err;

	if (!nvgpu_is_enabled(g, NVGPU_SUPPORT_NVLINK))
		return -ENODEV;

	err = nvlink_shutdown(ndev);
	if (err) {
		nvgpu_err(g, "failed to shut down nvlink");
		return err;
	}

	nvgpu_nvlink_remove(g);

	return 0;
#endif
	return -ENODEV;
}
