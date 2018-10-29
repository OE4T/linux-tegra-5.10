/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
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

#ifdef CONFIG_TEGRA_NVLINK

#include <nvgpu/nvgpu_common.h>
#include <nvgpu/bitops.h>
#include <nvgpu/nvlink.h>
#include <nvgpu/enabled.h>
#include <nvgpu/io.h>
#include <nvgpu/timers.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/bios.h>

#include "gv100/nvlink_gv100.h"
#include "nvlink_tu104.h"

#include <nvgpu/hw/tu104/hw_minion_tu104.h>
#include <nvgpu/hw/tu104/hw_nvl_tu104.h>

int tu104_nvlink_rxdet(struct gk20a *g, u32 link_id)
{
	int ret = 0;
	u32 reg;
	struct nvgpu_timeout timeout;

	ret = gv100_nvlink_minion_send_command(g, link_id,
			0x00000005U, 0, true);
	if (ret != 0) {
		nvgpu_err(g, "Error during INITRXTERM minion DLCMD on link %u",
				link_id);
		return ret;
	}

	ret = gv100_nvlink_minion_send_command(g, link_id,
			minion_nvlink_dl_cmd_command_turing_rxdet_v(), 0, true);
	if (ret != 0) {
		nvgpu_err(g, "Error during RXDET minion DLCMD on link %u",
				link_id);
		return ret;
	}

	ret = nvgpu_timeout_init(g, &timeout, NV_NVLINK_REG_POLL_TIMEOUT_MS,
							NVGPU_TIMER_CPU_TIMER);
	if (ret != 0) {
		nvgpu_err(g, "Error during timeout init");
		return ret;
	}

	do {
		reg = DLPL_REG_RD32(g, link_id, nvl_sl0_link_rxdet_status_r());
		if (nvl_sl0_link_rxdet_status_sts_v(reg) ==
				nvl_sl0_link_rxdet_status_sts_found_v()) {
			nvgpu_log(g, gpu_dbg_nvlink,
					"RXDET successful on link %u", link_id);
			return ret;
		}
		if (nvl_sl0_link_rxdet_status_sts_v(reg) ==
				nvl_sl0_link_rxdet_status_sts_timeout_v()) {
			nvgpu_log(g, gpu_dbg_nvlink,
					"RXDET failed on link %u", link_id);
			break;
		}
		nvgpu_udelay(NV_NVLINK_TIMEOUT_DELAY_US);
	} while (!nvgpu_timeout_expired_msg(
				&timeout,
				"RXDET status check timed out on link %u",
				link_id));
	return -ETIMEDOUT;
}

int tu104_nvlink_setup_pll(struct gk20a *g, unsigned long link_mask)
{
	int ret = 0;
	u32 link_id;
	u32 reg;
	struct nvgpu_timeout timeout;

	for_each_set_bit(link_id, &link_mask, 32) {
		ret = gv100_nvlink_minion_send_command(g, link_id,
			minion_nvlink_dl_cmd_command_txclkswitch_pll_v(),
			0, true);
		if (ret != 0) {
			nvgpu_err(g, "Error: TXCLKSWITCH_PLL dlcmd on link %u",
								link_id);
			return ret;
		}

		ret = nvgpu_timeout_init(g, &timeout,
			NV_NVLINK_REG_POLL_TIMEOUT_MS, NVGPU_TIMER_CPU_TIMER);
		if (ret != 0) {
			nvgpu_err(g, "Error during timeout init");
			return ret;
		}
		do {
			reg = DLPL_REG_RD32(g, link_id, nvl_clk_status_r());
			if (nvl_clk_status_txclk_sts_v(reg) ==
					nvl_clk_status_txclk_sts_pll_clk_v()) {
				nvgpu_log(g, gpu_dbg_nvlink,
					"PLL SETUP successful on link %u",
					link_id);
				break;
			}
			nvgpu_udelay(NV_NVLINK_TIMEOUT_DELAY_US);
		} while ((!nvgpu_timeout_expired_msg(&timeout,
					"Timed out setting pll on link %u",
					link_id)));
		if (nvgpu_timeout_peek_expired(&timeout)) {
			return -ETIMEDOUT;
		}
	}
	return ret;
}

u32 tu104_nvlink_link_get_tx_sublink_state(struct gk20a *g, u32 link_id)
{
	u32 reg;
	struct nvgpu_timeout timeout;
	int err = 0;

	err = nvgpu_timeout_init(g, &timeout, NV_NVLINK_REG_POLL_TIMEOUT_MS,
							NVGPU_TIMER_CPU_TIMER);
	if (err != 0) {
		nvgpu_err(g, "Failed to init timeout: %d", err);
		goto result;
	}

	/* Poll till substate value becomes STABLE */
	do {
		reg = DLPL_REG_RD32(g, link_id, nvl_sl0_slsm_status_tx_r());
		if (nvl_sl0_slsm_status_tx_substate_v(reg) ==
				nvl_sl0_slsm_status_tx_substate_stable_v()) {
			return nvl_sl0_slsm_status_tx_primary_state_v(reg);
		}
		nvgpu_udelay(NV_NVLINK_TIMEOUT_DELAY_US);
	} while (!nvgpu_timeout_expired_msg(&timeout,
				"Timeout on TX SLSM substate = stable check"));

	nvgpu_log(g, gpu_dbg_nvlink, "TX SLSM primary state :%u, substate:%u",
				nvl_sl0_slsm_status_tx_primary_state_v(reg),
				nvl_sl0_slsm_status_tx_substate_v(reg));

result:
	return nvl_sl0_slsm_status_tx_primary_state_unknown_v();
}

u32 tu104_nvlink_link_get_rx_sublink_state(struct gk20a *g, u32 link_id)
{
	u32 reg;
	struct nvgpu_timeout timeout;
	int err = 0;

	err = nvgpu_timeout_init(g, &timeout, NV_NVLINK_REG_POLL_TIMEOUT_MS,
							NVGPU_TIMER_CPU_TIMER);
	if (err != 0) {
		nvgpu_err(g, "Failed to init timeout: %d", err);
		goto result;
	}

	/* Poll till substate value becomes STABLE */
	do {
		reg = DLPL_REG_RD32(g, link_id, nvl_sl1_slsm_status_rx_r());
		if (nvl_sl1_slsm_status_rx_substate_v(reg) ==
				nvl_sl1_slsm_status_rx_substate_stable_v()) {
			return nvl_sl1_slsm_status_rx_primary_state_v(reg);
		}
		nvgpu_udelay(NV_NVLINK_TIMEOUT_DELAY_US);
	} while (!nvgpu_timeout_expired_msg(&timeout,
				"Timeout on RX SLSM substate = stable check"));

	nvgpu_log(g, gpu_dbg_nvlink, "RX SLSM primary state :%u, substate:%u",
				nvl_sl1_slsm_status_rx_primary_state_v(reg),
				nvl_sl1_slsm_status_rx_substate_v(reg));

result:
	return nvl_sl1_slsm_status_rx_primary_state_unknown_v();
}

int tu104_nvlink_minion_data_ready_en(struct gk20a *g,
					unsigned long link_mask, bool sync)
{
	int ret = 0;
	u32 link_id;

	/* On Volta, the order of INIT* DLCMDs was arbitrary.
	 * On Turing, the INIT* DLCMDs need to be executed in the following
	 * order -
	 * INITDLPL -> INITL -> INITLANEENABLE.
	 * INITDLPL_TO_CHIPA is needed additionally when connected  to 2.0 dev.
	 */
	for_each_set_bit(link_id, &link_mask, 32) {
		ret = gv100_nvlink_minion_send_command(g, link_id,
			minion_nvlink_dl_cmd_command_initdlpl_v(), 0,
									sync);
		if (ret != 0) {
			nvgpu_err(g, "Minion initdlpl failed on link %u",
								link_id);
			return ret;
		}
	}
	for_each_set_bit(link_id, &link_mask, 32) {
		ret = gv100_nvlink_minion_send_command(g, link_id,
			minion_nvlink_dl_cmd_command_turing_initdlpl_to_chipa_v(),
								0, sync);
		if (ret != 0) {
			nvgpu_err(g, "Minion initdlpl_to_chipA failed on link\
								%u", link_id);
			return ret;
		}
	}
	for_each_set_bit(link_id, &link_mask, 32) {
		ret = gv100_nvlink_minion_send_command(g, link_id,
			minion_nvlink_dl_cmd_command_inittl_v(), 0,
									sync);
		if (ret != 0) {
			nvgpu_err(g, "Minion inittl failed on link %u",
								link_id);
			return ret;
		}
	}
	for_each_set_bit(link_id, &link_mask, 32) {
		ret = gv100_nvlink_minion_send_command(g, link_id,
			minion_nvlink_dl_cmd_command_initlaneenable_v(), 0,
									sync);
		if (ret != 0) {
			nvgpu_err(g, "Minion initlaneenable failed on link %u",
								link_id);
			return ret;
		}
	}
	return ret;
}

void tu104_nvlink_get_connected_link_mask(u32 *link_mask)
{
	*link_mask = TU104_CONNECTED_LINK_MASK;
}

int tu104_nvlink_speed_config(struct gk20a *g)
{
	int ret = 0;

	ret = nvgpu_bios_get_lpwr_nvlink_table_hdr(g);
	if (ret != 0) {
		nvgpu_err(g, "Failed to read LWPR_NVLINK_TABLE header\n");
		return ret;
	}

	switch (g->nvlink.initpll_ordinal) {
	case INITPLL_1:
		g->nvlink.speed = nvgpu_nvlink_speed_20G;
		g->nvlink.initpll_cmd =
				minion_nvlink_dl_cmd_command_initpll_1_v();
		break;
	case INITPLL_7:
		g->nvlink.speed = nvgpu_nvlink_speed_16G;
		g->nvlink.initpll_cmd =
				minion_nvlink_dl_cmd_command_initpll_7_v();
		break;
	default:
		nvgpu_err(g, "Nvlink initpll %d from VBIOS not supported.",
					g->nvlink.initpll_ordinal);
		ret = -EINVAL;
		break;
	}

	return ret;
}
#endif /* CONFIG_TEGRA_NVLINK */
