/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/bios.h>
#include <nvgpu/firmware.h>
#include <nvgpu/bitops.h>
#include <nvgpu/nvlink.h>
#include <nvgpu/enabled.h>
#include <nvgpu/io.h>
#include <nvgpu/utils.h>
#include <nvgpu/timers.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/top.h>
#include "nvlink_gv100.h"

#include <nvgpu/hw/gv100/hw_nvlinkip_discovery_gv100.h>
#include <nvgpu/hw/gv100/hw_ioctrl_gv100.h>
#include <nvgpu/hw/gv100/hw_minion_gv100.h>
#include <nvgpu/hw/gv100/hw_nvl_gv100.h>
#include <nvgpu/hw/gv100/hw_trim_gv100.h>

#define NVLINK_PLL_ON_TIMEOUT_MS	30
#define NVLINK_SUBLINK_TIMEOUT_MS	200

#define NVL_DEVICE(str) nvlinkip_discovery_common_device_##str##_v()

static const char *__gv100_device_type_to_str(u32 type)
{
	if (type == NVL_DEVICE(ioctrl))
		return "IOCTRL";
	if (type == NVL_DEVICE(dlpl))
		return "DL/PL";
	if (type == NVL_DEVICE(nvltlc))
		return "NVLTLC";
	if (type == NVL_DEVICE(ioctrlmif))
		return "IOCTRLMIF";
	if (type == NVL_DEVICE(nvlipt))
		return "NVLIPT";
	if (type == NVL_DEVICE(minion))
		return "MINION";
	if (type == NVL_DEVICE(dlpl_multicast))
		return "DL/PL MULTICAST";
	if (type == NVL_DEVICE(nvltlc_multicast))
		return "NVLTLC MULTICAST";
	if (type == NVL_DEVICE(ioctrlmif_multicast))
		return "IOCTRLMIF MULTICAST";
	if (type == NVL_DEVICE(nvltlc_multicast))
		return "NVLTLC MULTICAST";
	return "UNKNOWN";
}

/*
 * Function prototypes
 */
static u32 __gv100_nvlink_get_link_reset_mask(struct gk20a *g);
static int gv100_nvlink_rxcal_en(struct gk20a *g, unsigned long mask);


/*
 *******************************************************************************
 * IP specific functions                                                       *
 *******************************************************************************
 */

/*
 *-----------------------------------------------------------------------------*
 * MINION API
 *-----------------------------------------------------------------------------*
 */

/*
 * Check if minion is up
 */
static bool __gv100_nvlink_minion_is_running(struct gk20a *g)
{

	/* if minion is booted and not halted, it is running */
	if ((MINION_REG_RD32(g, minion_minion_status_r()) &
				minion_minion_status_status_f(1)) &&
	    (!minion_falcon_irqstat_halt_v(
			MINION_REG_RD32(g, minion_falcon_irqstat_r()))))
		return true;

	return false;
}

/*
 * Load minion FW and set up bootstrap
 */
static u32 gv100_nvlink_minion_load(struct gk20a *g)
{
	u32 err = 0;
	struct nvgpu_firmware *nvgpu_minion_fw = NULL;
	struct nvgpu_timeout timeout;
	u32 delay = GR_IDLE_CHECK_DEFAULT;
	u32 reg;

	nvgpu_log_fn(g, " ");

	if (__gv100_nvlink_minion_is_running(g))
		return 0;

	/* get mem unlock ucode binary */
	nvgpu_minion_fw = nvgpu_request_firmware(g, "minion.bin", 0);
	if (!nvgpu_minion_fw) {
		nvgpu_err(g, "minion ucode get fail");
		err = -ENOENT;
		goto exit;
	}

	/* nvdec falcon reset */
	nvgpu_falcon_reset(g->minion_flcn);

	/* Clear interrupts */
	g->ops.nvlink.intr.minion_clear_interrupts(g);

	err = nvgpu_nvlink_minion_load_ucode(g, nvgpu_minion_fw);
	if (err != 0) {
		goto exit;
	}

	/* set BOOTVEC to start of non-secure code */
	nvgpu_falcon_bootstrap(g->minion_flcn, 0x0);

	nvgpu_timeout_init(g, &timeout, gk20a_get_gr_idle_timeout(g),
		NVGPU_TIMER_CPU_TIMER);

	do {
		reg = MINION_REG_RD32(g, minion_minion_status_r());

		if (minion_minion_status_status_v(reg)) {
			/* Minion sequence completed, check status */
			if (minion_minion_status_status_v(reg) !=
					minion_minion_status_status_boot_v()) {
				nvgpu_err(g, "MINION init sequence failed: 0x%x",
					minion_minion_status_status_v(reg));
				err = -EINVAL;

				goto exit;
			}

			nvgpu_log(g, gpu_dbg_nvlink,
				"MINION boot successful: 0x%x", reg);
			err = 0;
			break;
		}

		nvgpu_usleep_range(delay, delay * 2);
		delay = min_t(unsigned long,
				delay << 1, GR_IDLE_CHECK_MAX);
	} while (!nvgpu_timeout_expired_msg(&timeout, " minion boot timeout"));

	/* Service interrupts */
	g->ops.nvlink.intr.minion_falcon_isr(g);

	if (nvgpu_timeout_peek_expired(&timeout)) {
		err = -ETIMEDOUT;
		goto exit;
	}

	g->ops.nvlink.intr.init_minion_intr(g);
	return err;

exit:
	nvgpu_nvlink_free_minion_used_mem(g, nvgpu_minion_fw);
	return err;
}

/*
 * Check if MINION command is complete
 */
static u32 gv100_nvlink_minion_command_complete(struct gk20a *g, u32 link_id)
{
	u32 reg;
	struct nvgpu_timeout timeout;
	u32 delay = GR_IDLE_CHECK_DEFAULT;


	nvgpu_timeout_init(g, &timeout, gk20a_get_gr_idle_timeout(g),
		NVGPU_TIMER_CPU_TIMER);

	do {
		reg = MINION_REG_RD32(g, minion_nvlink_dl_cmd_r(link_id));

		if (minion_nvlink_dl_cmd_ready_v(reg) == 1) {
			/* Command completed, check sucess */
			if (minion_nvlink_dl_cmd_fault_v(reg) == 1) {
				nvgpu_err(g, "minion cmd(%d) error: 0x%x",
					link_id, reg);

				reg = minion_nvlink_dl_cmd_fault_f(1);
				MINION_REG_WR32(g,
					minion_nvlink_dl_cmd_r(link_id), reg);

				return -EINVAL;
			}

			/* Commnand success */
			break;
		}
		nvgpu_usleep_range(delay, delay * 2);
		delay = min_t(unsigned long,
				delay << 1, GR_IDLE_CHECK_MAX);

	} while (!nvgpu_timeout_expired_msg(&timeout, " minion cmd timeout"));

	if (nvgpu_timeout_peek_expired(&timeout))
		return -ETIMEDOUT;

	nvgpu_log(g, gpu_dbg_nvlink, "minion cmd Complete");
	return 0;
}

/*
 * Send Minion command (can be async)
 */
int gv100_nvlink_minion_send_command(struct gk20a *g, u32 link_id,
				u32 command, u32 scratch_0, bool sync)
{
	int err = 0;

	/* Check last command succeded */
	err = gv100_nvlink_minion_command_complete(g, link_id);
	if (err != 0)
		return -EINVAL;

	nvgpu_log(g, gpu_dbg_nvlink,
		"sending MINION command 0x%x to link %d", command, link_id);

	if (command == minion_nvlink_dl_cmd_command_configeom_v())
		MINION_REG_WR32(g, minion_misc_0_r(),
				minion_misc_0_scratch_swrw_0_f(scratch_0));

	MINION_REG_WR32(g, minion_nvlink_dl_cmd_r(link_id),
		minion_nvlink_dl_cmd_command_f(command) |
		minion_nvlink_dl_cmd_fault_f(1));

	if (sync)
		err = gv100_nvlink_minion_command_complete(g, link_id);

	return err;
}

/* MINION API COMMANDS */

/*
 * Init UPHY
 */
static int gv100_nvlink_minion_init_uphy(struct gk20a *g, unsigned long mask,
					bool sync)
{
	int err = 0;
	u32 link_id, master_pll, slave_pll;
	u32 master_state, slave_state;

	unsigned long link_enable;

	link_enable = __gv100_nvlink_get_link_reset_mask(g);

	for_each_set_bit(link_id, &mask, 32) {
		master_pll = g->nvlink.links[link_id].pll_master_link_id;
		slave_pll = g->nvlink.links[link_id].pll_slave_link_id;

		master_state = nvl_link_state_state_init_v();
		slave_state = nvl_link_state_state_init_v();

		if (BIT(master_pll) & link_enable)
			master_state = nvl_link_state_state_v(
				g->ops.nvlink.link_get_state(g, master_pll));

		if (BIT(slave_pll) & link_enable)
			slave_state = nvl_link_state_state_v(
				g->ops.nvlink.link_get_state(g, slave_pll));

		if ((slave_state != nvl_link_state_state_init_v()) ||
		   (master_state != nvl_link_state_state_init_v())) {
			nvgpu_err(g, "INIT PLL can only be executed when both "
				"master and slave links are in init state");
			return -EINVAL;
		}

		/* Check if INIT PLL is done on link */
		if (!(BIT(master_pll) & g->nvlink.init_pll_done)) {
			err = gv100_nvlink_minion_send_command(g, master_pll,
						g->nvlink.initpll_cmd, 0, sync);
			if (err != 0) {
				nvgpu_err(g, " Error sending INITPLL to minion");
				return err;
			}

			g->nvlink.init_pll_done |= BIT(master_pll);
		}
	}

	err = g->ops.nvlink.setup_pll(g, mask);
	if (err != 0) {
		nvgpu_err(g, "Error setting up PLL");
		return err;
	}

	/* INITPHY commands */
	for_each_set_bit(link_id, &mask, 32) {
		err = gv100_nvlink_minion_send_command(g, link_id,
			minion_nvlink_dl_cmd_command_initphy_v(), 0, sync);
		if (err != 0) {
			nvgpu_err(g, "Error on INITPHY minion DL command %u",
					link_id);
			return err;
		}
	}

	return 0;
}

/*
 * Configure AC coupling
 */
static int gv100_nvlink_minion_configure_ac_coupling(struct gk20a *g,
	unsigned long mask, bool sync)
{
	int err = 0;
	u32 i;
	u32 temp;

	for_each_set_bit(i, &mask, 32) {

		temp = DLPL_REG_RD32(g, i, nvl_link_config_r());
		temp &= ~nvl_link_config_ac_safe_en_m();
		temp |= nvl_link_config_ac_safe_en_on_f();

		DLPL_REG_WR32(g, i, nvl_link_config_r(), temp);

		err = gv100_nvlink_minion_send_command(g, i,
			minion_nvlink_dl_cmd_command_setacmode_v(), 0, sync);

		if (err != 0)
			return err;
	}

	return err;
}

/*
 * Set Data ready
 */
int gv100_nvlink_minion_data_ready_en(struct gk20a *g,
					unsigned long link_mask, bool sync)
{
	int ret = 0;
	u32 link_id;

	for_each_set_bit(link_id, &link_mask, 32) {
		ret = gv100_nvlink_minion_send_command(g, link_id,
			minion_nvlink_dl_cmd_command_initlaneenable_v(), 0,
									sync);
		if (ret != 0) {
			nvgpu_err(g, "Failed initlaneenable on link %u",
								link_id);
			return ret;
		}
	}

	for_each_set_bit(link_id, &link_mask, 32) {
		ret = gv100_nvlink_minion_send_command(g, link_id,
			minion_nvlink_dl_cmd_command_initdlpl_v(), 0, sync);
		if (ret != 0) {
			nvgpu_err(g, "Failed initdlpl on link %u", link_id);
			return ret;
		}
	}
	return ret;
}

/*
 * Request that minion disable the lane
 */
static int gv100_nvlink_minion_lane_disable(struct gk20a *g, u32 link_id,
								bool sync)
{
	int err = 0;

	err = gv100_nvlink_minion_send_command(g, link_id,
			minion_nvlink_dl_cmd_command_lanedisable_v(), 0, sync);

	if (err != 0)
		nvgpu_err(g, " failed to disable lane on %d", link_id);

	return err;
}

/*
 * Request that minion shutdown the lane
 */
static int gv100_nvlink_minion_lane_shutdown(struct gk20a *g, u32 link_id,
								bool sync)
{
	int err = 0;

	err = gv100_nvlink_minion_send_command(g, link_id,
			minion_nvlink_dl_cmd_command_laneshutdown_v(), 0, sync);

	if (err != 0)
		nvgpu_err(g, " failed to shutdown lane on %d", link_id);

	return err;
}

/*******************************************************************************
 * Helper functions                                                            *
 *******************************************************************************
 */

static u32 __gv100_nvlink_get_link_reset_mask(struct gk20a *g)
{
	u32 reg_data;

	reg_data = IOCTRL_REG_RD32(g, ioctrl_reset_r());

	return ioctrl_reset_linkreset_v(reg_data);
}

static u32 __gv100_nvlink_state_load_hal(struct gk20a *g)
{
	unsigned long discovered = g->nvlink.discovered_links;

	g->ops.nvlink.intr.common_intr_enable(g, discovered);
	return gv100_nvlink_minion_load(g);
}

#define TRIM_SYS_NVLINK_CTRL(i) (trim_sys_nvlink0_ctrl_r() + 16*i)
#define TRIM_SYS_NVLINK_STATUS(i) (trim_sys_nvlink0_status_r() + 16*i)

int gv100_nvlink_setup_pll(struct gk20a *g, unsigned long link_mask)
{
	u32 reg;
	u32 i;
	u32 links_off;
	struct nvgpu_timeout timeout;
	u32 pad_ctrl = 0;
	u32 swap_ctrl = 0;
	u32 pll_id;

	reg = gk20a_readl(g, trim_sys_nvlink_uphy_cfg_r());
	reg = set_field(reg, trim_sys_nvlink_uphy_cfg_phy2clks_use_lockdet_m(),
			trim_sys_nvlink_uphy_cfg_phy2clks_use_lockdet_f(1));
	gk20a_writel(g, trim_sys_nvlink_uphy_cfg_r(), reg);

	if (g->ops.top.get_nvhsclk_ctrl_e_clk_nvl) {
		pad_ctrl = g->ops.top.get_nvhsclk_ctrl_e_clk_nvl(g);
	}
	if (g->ops.top.get_nvhsclk_ctrl_swap_clk_nvl) {
		swap_ctrl = g->ops.top.get_nvhsclk_ctrl_swap_clk_nvl(g);
	}

	for_each_set_bit(i, &link_mask, 32) {
		/* There are 3 PLLs for 6 links. We have 3 bits for each PLL.
		 * The PLL bit corresponding to a link is /2 of its master link.
                 */
		pll_id = g->nvlink.links[i].pll_master_link_id >> 1;
		pad_ctrl  |= BIT(pll_id);
		swap_ctrl |= BIT(pll_id);
	}

	if (g->ops.top.set_nvhsclk_ctrl_e_clk_nvl) {
		g->ops.top.set_nvhsclk_ctrl_e_clk_nvl(g, pad_ctrl);
	}
	if (g->ops.top.set_nvhsclk_ctrl_swap_clk_nvl) {
		g->ops.top.set_nvhsclk_ctrl_swap_clk_nvl(g, swap_ctrl);
	}

	for_each_set_bit(i, &link_mask, 32) {
		reg = gk20a_readl(g, TRIM_SYS_NVLINK_CTRL(i));
		reg = set_field(reg,
			trim_sys_nvlink0_ctrl_unit2clks_pll_turn_off_m(),
			trim_sys_nvlink0_ctrl_unit2clks_pll_turn_off_f(0));
		gk20a_writel(g, TRIM_SYS_NVLINK_CTRL(i), reg);
	}

	/* Poll for links to go up */
	links_off = link_mask;

	nvgpu_timeout_init(g, &timeout,
		NVLINK_PLL_ON_TIMEOUT_MS, NVGPU_TIMER_CPU_TIMER);
	do {
		for_each_set_bit(i, &link_mask, 32) {
			reg = gk20a_readl(g, TRIM_SYS_NVLINK_STATUS(i));
			if (trim_sys_nvlink0_status_pll_off_v(reg) == 0)
				links_off &= ~BIT(i);
		}
		nvgpu_udelay(5);

	} while((!nvgpu_timeout_expired_msg(&timeout, "timeout on pll on")) &&
								links_off);

	if (nvgpu_timeout_peek_expired(&timeout))
		return -ETIMEDOUT;

	return 0;
}

static void gv100_nvlink_prog_alt_clk(struct gk20a *g)
{
	u32 tmp;

	/* RMW registers need to be separate */
	tmp = gk20a_readl(g, trim_sys_nvl_common_clk_alt_switch_r());
	tmp &= ~trim_sys_nvl_common_clk_alt_switch_slowclk_m();
	tmp |= trim_sys_nvl_common_clk_alt_switch_slowclk_xtal4x_f();
	gk20a_writel(g, trim_sys_nvl_common_clk_alt_switch_r(), tmp);
}

static int gv100_nvlink_enable_links_pre_top(struct gk20a *g, u32 links)
{
	u32 link_id;
	unsigned long enabled_links = links;
	u32 tmp;
	u32 reg;
	u32 delay = ioctrl_reset_sw_post_reset_delay_microseconds_v();
	int err;

	nvgpu_log(g, gpu_dbg_nvlink, " enabling 0x%lx links", enabled_links);
	/* Take links out of reset */
	for_each_set_bit(link_id, &enabled_links, 32) {
		reg = IOCTRL_REG_RD32(g, ioctrl_reset_r());

		tmp = (BIT(link_id) |
			BIT(g->nvlink.links[link_id].pll_master_link_id));

		reg = set_field(reg, ioctrl_reset_linkreset_m(),
			ioctrl_reset_linkreset_f( ioctrl_reset_linkreset_v(reg) |
			tmp));

		IOCTRL_REG_WR32(g, ioctrl_reset_r(), reg);
		nvgpu_udelay(delay);

		reg = IOCTRL_REG_RD32(g, ioctrl_debug_reset_r());

		reg &= ~ioctrl_debug_reset_link_f(BIT(link_id));
		IOCTRL_REG_WR32(g, ioctrl_debug_reset_r(), reg);
		nvgpu_udelay(delay);

		reg |= ioctrl_debug_reset_link_f(BIT(link_id));
		IOCTRL_REG_WR32(g, ioctrl_debug_reset_r(), reg);
		nvgpu_udelay(delay);

		/* Before  doing any link initialization, run RXDET to check
		 * if link is connected on  other end.
		 */
		if (g->ops.nvlink.rxdet) {
			err = g->ops.nvlink.rxdet(g, link_id);
			if (err != 0)
				return err;
		}

		/* Enable Link DLPL for AN0 */
		reg = DLPL_REG_RD32(g, link_id, nvl_link_config_r());
		reg = set_field(reg, nvl_link_config_link_en_m(),
			nvl_link_config_link_en_f(1));
		DLPL_REG_WR32(g, link_id, nvl_link_config_r(), reg);

		/* This should be done by the NVLINK API */
		err = gv100_nvlink_minion_init_uphy(g, BIT(link_id), true);
		if (err != 0) {
			nvgpu_err(g, "Failed to init phy of link: %u", link_id);
			return err;
		}

		err = gv100_nvlink_rxcal_en(g, BIT(link_id));
		if (err != 0) {
			nvgpu_err(g, "Failed to RXcal on link: %u", link_id);
			return err;
		}

		err = gv100_nvlink_minion_data_ready_en(g, BIT(link_id), true);
		if (err != 0) {
			nvgpu_err(g, "Failed to set data ready link:%u",
				link_id);
			return err;
		}

		g->nvlink.enabled_links |= BIT(link_id);
	}

	nvgpu_log(g, gpu_dbg_nvlink, "enabled_links=0x%08x",
		g->nvlink.enabled_links);

	if (g->nvlink.enabled_links)
		return 0;

	nvgpu_err(g, " No links were enabled");
	return -EINVAL;
}

void gv100_nvlink_set_sw_war(struct gk20a *g, u32 link_id)
{
	u32 reg;

	/* WAR for HW bug 1888034 */
	reg = DLPL_REG_RD32(g, link_id, nvl_sl0_safe_ctrl2_tx_r());
	reg = set_field(reg, nvl_sl0_safe_ctrl2_tx_ctr_init_m(),
		nvl_sl0_safe_ctrl2_tx_ctr_init_init_f());
	reg = set_field(reg, nvl_sl0_safe_ctrl2_tx_ctr_initscl_m(),
		nvl_sl0_safe_ctrl2_tx_ctr_initscl_init_f());
	DLPL_REG_WR32(g, link_id, nvl_sl0_safe_ctrl2_tx_r(), reg);
}

static int gv100_nvlink_enable_links_post_top(struct gk20a *g, u32 links)
{
	u32 link_id;
	unsigned long enabled_links = (links & g->nvlink.enabled_links) &
			~g->nvlink.initialized_links;

	for_each_set_bit(link_id, &enabled_links, 32) {
		if (g->ops.nvlink.set_sw_war)
			g->ops.nvlink.set_sw_war(g, link_id);
		g->ops.nvlink.intr.init_nvlipt_intr(g, link_id);
		g->ops.nvlink.intr.enable_link_intr(g, link_id, true);

		g->nvlink.initialized_links |= BIT(link_id);
	};

	return 0;
}

static u32 gv100_nvlink_prbs_gen_en(struct gk20a *g, unsigned long mask)
{
	u32 reg;
	u32 link_id;

	for_each_set_bit(link_id, &mask, 32) {
		/* Write is required as part of HW sequence */
		DLPL_REG_WR32(g, link_id, nvl_sl1_rxslsm_timeout_2_r(), 0);

		reg = DLPL_REG_RD32(g, link_id, nvl_txiobist_config_r());
		reg = set_field(reg, nvl_txiobist_config_dpg_prbsseedld_m(),
			nvl_txiobist_config_dpg_prbsseedld_f(0x1));
		DLPL_REG_WR32(g, link_id, nvl_txiobist_config_r(), reg);

		reg = DLPL_REG_RD32(g, link_id, nvl_txiobist_config_r());
		reg = set_field(reg, nvl_txiobist_config_dpg_prbsseedld_m(),
			nvl_txiobist_config_dpg_prbsseedld_f(0x0));
		DLPL_REG_WR32(g, link_id, nvl_txiobist_config_r(), reg);
	}

	return 0;
}

static int gv100_nvlink_rxcal_en(struct gk20a *g, unsigned long mask)
{
	u32 link_id;
	struct nvgpu_timeout timeout;
	u32 reg;

	for_each_set_bit(link_id, &mask, 32) {
		/* Timeout from HW specs */
		nvgpu_timeout_init(g, &timeout,
			8*NVLINK_SUBLINK_TIMEOUT_MS, NVGPU_TIMER_CPU_TIMER);
		reg = DLPL_REG_RD32(g, link_id, nvl_br0_cfg_cal_r());
		reg = set_field(reg, nvl_br0_cfg_cal_rxcal_m(),
			nvl_br0_cfg_cal_rxcal_on_f());
		DLPL_REG_WR32(g, link_id, nvl_br0_cfg_cal_r(), reg);

		do {
			reg = DLPL_REG_RD32(g, link_id,
						nvl_br0_cfg_status_cal_r());

			if (nvl_br0_cfg_status_cal_rxcal_done_v(reg) == 1)
				break;
			nvgpu_udelay(5);
		} while(!nvgpu_timeout_expired_msg(&timeout,
						"timeout on rxcal"));

		if (nvgpu_timeout_peek_expired(&timeout))
			return -ETIMEDOUT;
	}

	return 0;
}

/*
 *******************************************************************************
 * Internal "ops" functions                                                    *
 *******************************************************************************
 */


/*
 * Main Nvlink init function. Calls into the Nvlink core API
 */
int gv100_nvlink_init(struct gk20a *g)
{
	int err = 0;

	if (!nvgpu_is_enabled(g, NVGPU_SUPPORT_NVLINK))
		return -ENODEV;

	err = nvgpu_nvlink_enumerate(g);
	if (err != 0) {
		nvgpu_err(g, "failed to enumerate nvlink");
		goto fail;
	}

	/* Set HSHUB and SG_PHY */
	nvgpu_set_enabled(g, NVGPU_MM_USE_PHYSICAL_SG, true);

	err = g->ops.fb.enable_nvlink(g);
	if (err != 0) {
		nvgpu_err(g, "failed switch to nvlink sysmem");
		goto fail;
	}

	return err;

fail:
	nvgpu_set_enabled(g, NVGPU_MM_USE_PHYSICAL_SG, false);
	nvgpu_set_enabled(g, NVGPU_SUPPORT_NVLINK, false);
	return err;
}

/*
 * Query internal device topology and discover devices in nvlink local
 * infrastructure. Initialize register base and offsets
 */
int gv100_nvlink_discover_link(struct gk20a *g)
{
	u32 i;
	u32 ioctrl_entry_addr;
	u8 ioctrl_device_type;
	u32 table_entry;
	u32 ioctrl_info_entry_type;
	u8 ioctrl_discovery_size;
	bool is_chain = false;
	u8 nvlink_num_devices = 0;
	unsigned long available_links = 0;
	struct nvgpu_nvlink_device_list *device_table;
	u32 err = 0;

	/*
	 * Process Entry 0 & 1 of IOCTRL table to find table size
	 */
	if (g->nvlink.ioctrl_table && g->nvlink.ioctrl_table[0].pri_base_addr) {
		ioctrl_entry_addr = g->nvlink.ioctrl_table[0].pri_base_addr;
		table_entry = gk20a_readl(g, ioctrl_entry_addr);
		ioctrl_info_entry_type = nvlinkip_discovery_common_device_v(table_entry);
	} else {
		nvgpu_err(g, " Bad IOCTRL PRI Base addr");
		return -EINVAL;
	}

	if (ioctrl_info_entry_type == NVL_DEVICE(ioctrl)) {
		ioctrl_entry_addr = g->nvlink.ioctrl_table[0].pri_base_addr + 4;
		table_entry = gk20a_readl(g, ioctrl_entry_addr);
		ioctrl_discovery_size = nvlinkip_discovery_common_ioctrl_length_v(table_entry);
		nvgpu_log(g, gpu_dbg_nvlink, "IOCTRL size: %d", ioctrl_discovery_size);
	} else {
		nvgpu_err(g, " First entry of IOCTRL_DISCOVERY invalid");
		return -EINVAL;
	}

	device_table = nvgpu_kzalloc(g, ioctrl_discovery_size *
			sizeof(struct nvgpu_nvlink_device_list));
	if (!device_table) {
		nvgpu_err(g, " Unable to allocate nvlink device table");
		return -ENOMEM;
	}

	for (i = 0; i < ioctrl_discovery_size; i++) {
		ioctrl_entry_addr =
			g->nvlink.ioctrl_table[0].pri_base_addr + 4*i;
		table_entry = gk20a_readl(g, ioctrl_entry_addr);

		nvgpu_log(g, gpu_dbg_nvlink, "parsing ioctrl %d: 0x%08x", i, table_entry);

		ioctrl_info_entry_type = nvlinkip_discovery_common_entry_v(table_entry);

		if (ioctrl_info_entry_type ==
				nvlinkip_discovery_common_entry_invalid_v())
			continue;

		if (ioctrl_info_entry_type ==
				nvlinkip_discovery_common_entry_enum_v()) {

			nvgpu_log(g, gpu_dbg_nvlink, "IOCTRL entry %d is ENUM", i);

			ioctrl_device_type =
				nvlinkip_discovery_common_device_v(table_entry);

			if (nvlinkip_discovery_common_chain_v(table_entry) !=
				nvlinkip_discovery_common_chain_enable_v()) {

				nvgpu_log(g, gpu_dbg_nvlink,
					"IOCTRL entry %d is ENUM but no chain",
					i);
				err = -EINVAL;
				break;
			}

			is_chain = true;
			device_table[nvlink_num_devices].valid = true;
			device_table[nvlink_num_devices].device_type =
				ioctrl_device_type;
			device_table[nvlink_num_devices].device_id =
				nvlinkip_discovery_common_id_v(table_entry);
			device_table[nvlink_num_devices].device_version =
				nvlinkip_discovery_common_version_v(
								table_entry);
			continue;
		}

		if (ioctrl_info_entry_type ==
				nvlinkip_discovery_common_entry_data1_v()) {
			nvgpu_log(g, gpu_dbg_nvlink, "IOCTRL entry %d is DATA1", i);

			if (is_chain) {
				device_table[nvlink_num_devices].pri_base_addr =
					nvlinkip_discovery_common_pri_base_v(
						table_entry) << 12;

				device_table[nvlink_num_devices].intr_enum =
					nvlinkip_discovery_common_intr_v(
						table_entry);

				device_table[nvlink_num_devices].reset_enum =
					nvlinkip_discovery_common_reset_v(
						table_entry);

				nvgpu_log(g, gpu_dbg_nvlink, "IOCTRL entry %d type = %d base: 0x%08x intr: %d reset: %d",
					i,
					device_table[nvlink_num_devices].device_type,
					device_table[nvlink_num_devices].pri_base_addr,
					device_table[nvlink_num_devices].intr_enum,
					device_table[nvlink_num_devices].reset_enum);

				if (device_table[nvlink_num_devices].device_type ==
					NVL_DEVICE(dlpl)) {
					device_table[nvlink_num_devices].num_tx =
						nvlinkip_discovery_common_dlpl_num_tx_v(table_entry);
					device_table[nvlink_num_devices].num_rx =
						nvlinkip_discovery_common_dlpl_num_rx_v(table_entry);

					nvgpu_log(g, gpu_dbg_nvlink, "DLPL tx: %d rx: %d",
						device_table[nvlink_num_devices].num_tx,
						device_table[nvlink_num_devices].num_rx);
				}

				if (nvlinkip_discovery_common_chain_v(table_entry) !=
					nvlinkip_discovery_common_chain_enable_v()) {

					is_chain = false;
					nvlink_num_devices++;
				}
			}
			continue;
		}

		if (ioctrl_info_entry_type ==
				nvlinkip_discovery_common_entry_data2_v()) {

			nvgpu_log(g, gpu_dbg_nvlink, "IOCTRL entry %d is DATA2", i);

			if (is_chain) {
				if (nvlinkip_discovery_common_dlpl_data2_type_v(table_entry)) {
					device_table[nvlink_num_devices].pll_master =
						nvlinkip_discovery_common_dlpl_data2_master_v(table_entry);
					device_table[nvlink_num_devices].pll_master_id =
						nvlinkip_discovery_common_dlpl_data2_masterid_v(table_entry);
					nvgpu_log(g, gpu_dbg_nvlink, "PLL info: Master: %d, Master ID: %d",
						device_table[nvlink_num_devices].pll_master,
						device_table[nvlink_num_devices].pll_master_id);
				}

				if (nvlinkip_discovery_common_chain_v(table_entry) !=
					nvlinkip_discovery_common_chain_enable_v()) {

					is_chain = false;
					nvlink_num_devices++;
				}
			}
			continue;
		}
	}

	g->nvlink.device_table = device_table;
	g->nvlink.num_devices = nvlink_num_devices;

	/*
	 * Print table
	 */
	for (i = 0; i < nvlink_num_devices; i++) {
		if (device_table[i].valid) {
			nvgpu_log(g, gpu_dbg_nvlink, "Device %d - %s", i,
				__gv100_device_type_to_str(
						device_table[i].device_type));
			nvgpu_log(g, gpu_dbg_nvlink, "+Link/Device Id: %d", device_table[i].device_id);
			nvgpu_log(g, gpu_dbg_nvlink, "+Version: %d", device_table[i].device_version);
			nvgpu_log(g, gpu_dbg_nvlink, "+Base Addr: 0x%08x", device_table[i].pri_base_addr);
			nvgpu_log(g, gpu_dbg_nvlink, "+Intr Enum: %d", device_table[i].intr_enum);
			nvgpu_log(g, gpu_dbg_nvlink, "+Reset Enum: %d", device_table[i].reset_enum);
			if ((device_table[i].device_type == NVL_DEVICE(dlpl)) ||
			    (device_table[i].device_type == NVL_DEVICE(nvlink))) {
				nvgpu_log(g, gpu_dbg_nvlink, "+TX: %d", device_table[i].num_tx);
				nvgpu_log(g, gpu_dbg_nvlink, "+RX: %d", device_table[i].num_rx);
				nvgpu_log(g, gpu_dbg_nvlink, "+PLL Master: %d", device_table[i].pll_master);
				nvgpu_log(g, gpu_dbg_nvlink, "+PLL Master ID: %d", device_table[i].pll_master_id);
			}
		}
	}

	for (i = 0; i < nvlink_num_devices; i++) {
		if (device_table[i].valid) {

			if (device_table[i].device_type == NVL_DEVICE(ioctrl)) {

				g->nvlink.ioctrl_type =
					device_table[i].device_type;
				g->nvlink.ioctrl_base =
					device_table[i].pri_base_addr;
				continue;
			}

			if (device_table[i].device_type == NVL_DEVICE(dlpl)) {

				g->nvlink.dlpl_type =
					device_table[i].device_type;
				g->nvlink.dlpl_base[device_table[i].device_id] =
					device_table[i].pri_base_addr;
				g->nvlink.links[device_table[i].device_id].valid = true;
				g->nvlink.links[device_table[i].device_id].g = g;
				g->nvlink.links[device_table[i].device_id].dlpl_version =
					device_table[i].device_version;
				g->nvlink.links[device_table[i].device_id].dlpl_base =
					device_table[i].pri_base_addr;
				g->nvlink.links[device_table[i].device_id].intr_enum =
					device_table[i].intr_enum;
				g->nvlink.links[device_table[i].device_id].reset_enum =
					device_table[i].reset_enum;
				g->nvlink.links[device_table[i].device_id].link_id =
					device_table[i].device_id;

				/* initiate the PLL master and slave link id to max */
				g->nvlink.links[device_table[i].device_id].pll_master_link_id =
					NVLINK_MAX_LINKS_SW;
				g->nvlink.links[device_table[i].device_id].pll_slave_link_id =
					NVLINK_MAX_LINKS_SW;

				/* Update Pll master */
				if (device_table[i].pll_master)
					g->nvlink.links[device_table[i].device_id].pll_master_link_id =
						g->nvlink.links[device_table[i].device_id].link_id;
				else {
					g->nvlink.links[device_table[i].device_id].pll_master_link_id =
						device_table[i].pll_master_id;
					g->nvlink.links[device_table[i].device_id].pll_slave_link_id =
						g->nvlink.links[device_table[i].device_id].link_id;
					g->nvlink.links[device_table[i].pll_master_id].pll_slave_link_id =
						g->nvlink.links[device_table[i].device_id].link_id;
				}

				available_links |= BIT(device_table[i].device_id);
				continue;
			}

			if (device_table[i].device_type == NVL_DEVICE(nvltlc)) {

				g->nvlink.tl_type = device_table[i].device_type;
				g->nvlink.tl_base[device_table[i].device_id] =
					device_table[i].pri_base_addr;
				g->nvlink.links[device_table[i].device_id].tl_base =
					device_table[i].pri_base_addr;
				g->nvlink.links[device_table[i].device_id].tl_version =
					device_table[i].device_version;
				continue;
			}

			if (device_table[i].device_type == NVL_DEVICE(nvltlc)) {

				g->nvlink.tl_type = device_table[i].device_type;
				g->nvlink.tl_base[device_table[i].device_id] =
					device_table[i].pri_base_addr;
				g->nvlink.links[device_table[i].device_id].tl_base =
					device_table[i].pri_base_addr;
				g->nvlink.links[device_table[i].device_id].tl_version =
					device_table[i].device_version;
				continue;
			}

			if (device_table[i].device_type == NVL_DEVICE(ioctrlmif)) {

				g->nvlink.mif_type = device_table[i].device_type;
				g->nvlink.mif_base[device_table[i].device_id] =
					device_table[i].pri_base_addr;
				g->nvlink.links[device_table[i].device_id].mif_base =
					device_table[i].pri_base_addr;
				g->nvlink.links[device_table[i].device_id].mif_version =
					device_table[i].device_version;
				continue;
			}

			if (device_table[i].device_type == NVL_DEVICE(nvlipt)) {

				g->nvlink.ipt_type =
					device_table[i].device_type;
				g->nvlink.ipt_base =
					device_table[i].pri_base_addr;
				g->nvlink.ipt_version =
					device_table[i].device_version;
				continue;
			}

			if (device_table[i].device_type == NVL_DEVICE(minion)) {

				g->nvlink.minion_type =
					device_table[i].device_type;
				g->nvlink.minion_base =
					device_table[i].pri_base_addr;
				g->nvlink.minion_version =
					device_table[i].device_version;
				continue;
			}

			if (device_table[i].device_type == NVL_DEVICE(dlpl_multicast)) {

				g->nvlink.dlpl_multicast_type =
					device_table[i].device_type;
				g->nvlink.dlpl_multicast_base =
					device_table[i].pri_base_addr;
				g->nvlink.dlpl_multicast_version =
					device_table[i].device_version;
				continue;
			}
			if (device_table[i].device_type == NVL_DEVICE(nvltlc_multicast)) {

				g->nvlink.tl_multicast_type =
					device_table[i].device_type;
				g->nvlink.tl_multicast_base =
					device_table[i].pri_base_addr;
				g->nvlink.tl_multicast_version =
					device_table[i].device_version;
				continue;
			}

			if (device_table[i].device_type == NVL_DEVICE(ioctrlmif_multicast)) {

				g->nvlink.mif_multicast_type =
					device_table[i].device_type;
				g->nvlink.mif_multicast_base =
					device_table[i].pri_base_addr;
				g->nvlink.mif_multicast_version =
					device_table[i].device_version;
				continue;
			}

		}
	}

	g->nvlink.discovered_links = (u32) available_links;

	nvgpu_log(g, gpu_dbg_nvlink, "Nvlink Tree:");
	nvgpu_log(g, gpu_dbg_nvlink, "+ Available Links: 0x%08lx", available_links);
	nvgpu_log(g, gpu_dbg_nvlink, "+ Per-Link Devices:");

	for_each_set_bit(i, &available_links, 32) {
		nvgpu_log(g, gpu_dbg_nvlink, "-- Link %d Dl/Pl Base: 0x%08x TLC Base: 0x%08x MIF Base: 0x%08x",
			i, g->nvlink.dlpl_base[i], g->nvlink.tl_base[i], g->nvlink.mif_base[i]);
	}

	nvgpu_log(g, gpu_dbg_nvlink, "+ IOCTRL Base: 0x%08x", g->nvlink.ioctrl_base);
	nvgpu_log(g, gpu_dbg_nvlink, "+ NVLIPT Base: 0x%08x", g->nvlink.ipt_base);
	nvgpu_log(g, gpu_dbg_nvlink, "+ MINION Base: 0x%08x", g->nvlink.minion_base);
	nvgpu_log(g, gpu_dbg_nvlink, "+ DLPL MCAST Base: 0x%08x", g->nvlink.dlpl_multicast_base);
	nvgpu_log(g, gpu_dbg_nvlink, "+ TLC MCAST Base: 0x%08x", g->nvlink.tl_multicast_base);
	nvgpu_log(g, gpu_dbg_nvlink, "+ MIF MCAST Base: 0x%08x", g->nvlink.mif_multicast_base);

	if (!g->nvlink.minion_version) {
		nvgpu_err(g, "Unsupported MINION version");

		nvgpu_kfree(g, device_table);
		g->nvlink.device_table = NULL;
		g->nvlink.num_devices = 0;
		return -EINVAL;
	}

	return err;
}

/*
 * Query IOCTRL for device discovery
 */
int gv100_nvlink_discover_ioctrl(struct gk20a *g)
{
	int ret = 0;
	u32 i;
	struct nvgpu_nvlink_ioctrl_list *ioctrl_table;
	u32 ioctrl_num_entries = 0;

	if (g->ops.top.get_num_engine_type_entries) {
		ioctrl_num_entries = g->ops.top.get_num_engine_type_entries(g,
							NVGPU_ENGINE_IOCTRL);
		nvgpu_log_info(g, "ioctrl_num_entries: %d", ioctrl_num_entries);
	}

	if (ioctrl_num_entries == 0) {
		nvgpu_err(g, "No NVLINK IOCTRL entry found in dev_info table");
		return -EINVAL;
	}

	ioctrl_table = nvgpu_kzalloc(g, ioctrl_num_entries *
				sizeof(struct nvgpu_nvlink_ioctrl_list));
	if (!ioctrl_table) {
		nvgpu_err(g, "Failed to allocate memory for nvlink io table");
		return -ENOMEM;
	}

	for (i = 0; i < ioctrl_num_entries; i++) {
		struct nvgpu_device_info dev_info;

		ret = g->ops.top.get_device_info(g, &dev_info,
						NVGPU_ENGINE_IOCTRL, i);
		if (ret) {
			nvgpu_err(g, "Failed to parse dev_info table"
					"for engine %d",
					NVGPU_ENGINE_IOCTRL);
			nvgpu_kfree(g, ioctrl_table);
			return -EINVAL;
		}

		ioctrl_table[i].valid = true;
		ioctrl_table[i].intr_enum = dev_info.intr_id;
		ioctrl_table[i].reset_enum = dev_info.reset_id;
		ioctrl_table[i].pri_base_addr = dev_info.pri_base;
		nvgpu_log(g, gpu_dbg_nvlink,
			"Dev %d: Pri_Base = 0x%0x Intr = %d Reset = %d",
			i, ioctrl_table[i].pri_base_addr,
			ioctrl_table[i].intr_enum,
			ioctrl_table[i].reset_enum);
	}
	g->nvlink.ioctrl_table = ioctrl_table;
	g->nvlink.io_num_entries = ioctrl_num_entries;

	return 0;
}

/*
 *******************************************************************************
 * NVLINK API FUNCTIONS                                                       *
 *******************************************************************************
 */

/*
 * Performs link level initialization like phy inits, AN0 and interrupts
 */

int gv100_nvlink_link_early_init(struct gk20a *g, unsigned long mask)
{
	int err;

	err = gv100_nvlink_enable_links_pre_top(g, mask);
	if (err != 0) {
		nvgpu_err(g, "Pre topology failed for links %lx", mask);
		return err;
	}

	nvgpu_log(g, gpu_dbg_nvlink, "pretopology enabled: 0x%lx",
			mask & g->nvlink.enabled_links);
	err = gv100_nvlink_enable_links_post_top(g, mask);

	return err;
}

/*
 * Performs memory interface initialization
 */

int gv100_nvlink_interface_init(struct gk20a *g)
{
	unsigned long mask = g->nvlink.enabled_links;
	u32 link_id;
	int err;

	for_each_set_bit(link_id, &mask, 32) {
		g->ops.nvlink.intr.init_mif_intr(g, link_id);
		g->ops.nvlink.intr.mif_intr_enable(g, link_id, true);
	}

	err = g->ops.fb.init_nvlink(g);
	if (err != 0) {
		nvgpu_err(g, "failed to setup nvlinks for sysmem");
		return err;
	}

	return 0;
}

int gv100_nvlink_interface_disable(struct gk20a *g)
{
	return 0;
}

/*
 * Shutdown device. This should tear down Nvlink connection.
 * For now return.
 */
int gv100_nvlink_shutdown(struct gk20a *g)
{
	nvgpu_falcon_sw_free(g, FALCON_ID_MINION);

	return 0;
}

/*
 * Get link state
 */
u32 gv100_nvlink_link_get_state(struct gk20a *g, u32 link_id)
{
	return DLPL_REG_RD32(g, link_id, nvl_link_state_r()) &
			nvl_link_state_state_m();
}

/* Get link mode */
u32 gv100_nvlink_link_get_mode(struct gk20a *g, u32 link_id)
{
	u32 state;
	if (!(BIT(link_id) & g->nvlink.discovered_links))
		return nvgpu_nvlink_link__last;

	state = nvl_link_state_state_v(
			g->ops.nvlink.link_get_state(g, link_id));

	if (state == nvl_link_state_state_init_v())
		return nvgpu_nvlink_link_off;
	if (state == nvl_link_state_state_hwcfg_v())
		return nvgpu_nvlink_link_detect;
	if (state == nvl_link_state_state_swcfg_v())
		return nvgpu_nvlink_link_safe;
	if (state == nvl_link_state_state_active_v())
		return nvgpu_nvlink_link_hs;
	if (state == nvl_link_state_state_fault_v())
		return nvgpu_nvlink_link_fault;
	if (state == nvl_link_state_state_rcvy_ac_v())
		return nvgpu_nvlink_link_rcvy_ac;
	if (state == nvl_link_state_state_rcvy_sw_v())
		return nvgpu_nvlink_link_rcvy_sw;
	if (state == nvl_link_state_state_rcvy_rx_v())
		return nvgpu_nvlink_link_rcvy_rx;

	return nvgpu_nvlink_link_off;
}

/* Set Link mode */
int gv100_nvlink_link_set_mode(struct gk20a *g, u32 link_id, u32 mode)
{
	u32 state;
	u32 reg;
	int err = 0;

	nvgpu_log(g, gpu_dbg_nvlink, "link :%d, mode:%u", link_id, mode);

	if (!(BIT(link_id) & g->nvlink.enabled_links))
		return -EINVAL;

	state = nvl_link_state_state_v(
			g->ops.nvlink.link_get_state(g, link_id));

	switch (mode) {
	case nvgpu_nvlink_link_safe:
		if (state == nvl_link_state_state_swcfg_v()) {
			nvgpu_warn(g, "link is already in safe mode");
			break;
		}
		if (state == nvl_link_state_state_hwcfg_v()) {
			nvgpu_warn(g, "link is transitioning to safe mode");
			break;
		}

		if (state == nvl_link_state_state_init_v()) {
			/* Off to Safe transition */
			reg = DLPL_REG_RD32(g, link_id, nvl_link_change_r());
			reg = set_field(reg, nvl_link_change_newstate_m(),
				nvl_link_change_newstate_hwcfg_f());
			reg = set_field(reg, nvl_link_change_oldstate_mask_m(),
				nvl_link_change_oldstate_mask_dontcare_f());
			reg = set_field(reg, nvl_link_change_action_m(),
				nvl_link_change_action_ltssm_change_f());
			DLPL_REG_WR32(g, link_id, nvl_link_change_r(), reg);
		} else if (state == nvl_link_state_state_active_v()) {
			/* TODO:
			 * Disable PM first since we are moving out active
			 * state
			 */
			reg = DLPL_REG_RD32(g, link_id, nvl_link_change_r());
			reg = set_field(reg, nvl_link_change_newstate_m(),
				nvl_link_change_newstate_swcfg_f());
			reg = set_field(reg, nvl_link_change_oldstate_mask_m(),
				nvl_link_change_oldstate_mask_dontcare_f());
			reg = set_field(reg, nvl_link_change_action_m(),
				nvl_link_change_action_ltssm_change_f());
			DLPL_REG_WR32(g, link_id, nvl_link_change_r(), reg);
		}
		break;

	case nvgpu_nvlink_link_hs:
		if (state == nvl_link_state_state_active_v()) {
			nvgpu_err(g, "link is already in active mode");
			break;
		}
		if (state == nvl_link_state_state_init_v()) {
			nvgpu_err(g, "link cannot be taken from init state");
			return -EPERM;
		}

		reg = DLPL_REG_RD32(g, link_id, nvl_link_change_r());
		reg = set_field(reg, nvl_link_change_newstate_m(),
				nvl_link_change_newstate_active_f());
		reg = set_field(reg, nvl_link_change_oldstate_mask_m(),
			nvl_link_change_oldstate_mask_dontcare_f());
		reg = set_field(reg, nvl_link_change_action_m(),
			nvl_link_change_action_ltssm_change_f());
		DLPL_REG_WR32(g, link_id, nvl_link_change_r(), reg);
		break;

	case nvgpu_nvlink_link_off:
		if (state == nvl_link_state_state_active_v()) {
			nvgpu_err(g, "link cannot be taken from active to init");
			return -EPERM;
		}
		if (state == nvl_link_state_state_init_v()) {
			nvgpu_err(g, "link already in init state");
		}

		/* GV100 UPHY is handled by MINION */
		break;
		/* 1/8 th mode not supported */
	case nvgpu_nvlink_link_enable_pm:
	case nvgpu_nvlink_link_disable_pm:
		return -EPERM;
	case nvgpu_nvlink_link_disable_err_detect:
		/* Disable Link interrupts */
		g->ops.nvlink.intr.dlpl_intr_enable(g, link_id, false);
		break;
	case nvgpu_nvlink_link_lane_disable:
		err = gv100_nvlink_minion_lane_disable(g, link_id, true);
		break;
	case nvgpu_nvlink_link_lane_shutdown:
		err = gv100_nvlink_minion_lane_shutdown(g, link_id, true);
		break;
	default:
		nvgpu_err(g, "Unhandled mode %x", mode);
		break;
	}

	return err;
}

static u32 gv100_nvlink_link_sublink_check_change(struct gk20a *g, u32 link_id)
{
	struct nvgpu_timeout timeout;
	u32 reg;

	nvgpu_timeout_init(g, &timeout,
			NVLINK_SUBLINK_TIMEOUT_MS, NVGPU_TIMER_CPU_TIMER);
	/* Poll for sublink status */
	do {
		reg = DLPL_REG_RD32(g, link_id, nvl_sublink_change_r());

		if (nvl_sublink_change_status_v(reg) ==
				nvl_sublink_change_status_done_v())
			break;
		if (nvl_sublink_change_status_v(reg) ==
				nvl_sublink_change_status_fault_v()) {
			nvgpu_err(g, "Fault detected in sublink change");
			return -EFAULT;
		}
		nvgpu_udelay(5);
	} while(!nvgpu_timeout_expired_msg(&timeout, "timeout on sublink rdy"));

	if (nvgpu_timeout_peek_expired(&timeout))
		return -ETIMEDOUT;
	return-0;
}

int gv100_nvlink_link_set_sublink_mode(struct gk20a *g, u32 link_id,
					bool is_rx_sublink, u32 mode)
{
	int err = 0;
	u32 rx_sublink_state = nvgpu_nvlink_sublink_rx__last;
	u32 tx_sublink_state = nvgpu_nvlink_sublink_tx__last;
	u32 reg;

	if (!(BIT(link_id) & g->nvlink.enabled_links))
		return -EINVAL;

	err = gv100_nvlink_link_sublink_check_change(g, link_id);
	if (err != 0)
		return err;

	if (is_rx_sublink)
		rx_sublink_state = g->ops.nvlink.get_rx_sublink_state(g,
								link_id);
	else
		tx_sublink_state = g->ops.nvlink.get_tx_sublink_state(g,
								link_id);

	switch (mode) {
	case nvgpu_nvlink_sublink_tx_hs:
		if (tx_sublink_state ==
			nvl_sl0_slsm_status_tx_primary_state_hs_v()) {
			nvgpu_err(g, " TX already in HS");
			break;
		} else if (tx_sublink_state ==
				nvl_sl0_slsm_status_tx_primary_state_off_v()) {
			nvgpu_err(g, "TX cannot be do from OFF to HS");
			return -EPERM;
		}

		reg = DLPL_REG_RD32(g, link_id, nvl_sublink_change_r());
		reg = set_field(reg, nvl_sublink_change_newstate_m(),
			nvl_sublink_change_newstate_hs_f());
		reg = set_field(reg, nvl_sublink_change_sublink_m(),
				nvl_sublink_change_sublink_tx_f());
		reg = set_field(reg, nvl_sublink_change_action_m(),
			nvl_sublink_change_action_slsm_change_f());
		DLPL_REG_WR32(g, link_id, nvl_sublink_change_r(), reg);

		err = gv100_nvlink_link_sublink_check_change(g, link_id);
		if (err != 0) {
			nvgpu_err(g, "Error in TX to HS");
			return err;
		}
		break;
	case nvgpu_nvlink_sublink_tx_common:
		err = gv100_nvlink_minion_init_uphy(g, BIT(link_id), true);
		break;
	case nvgpu_nvlink_sublink_tx_common_disable:
		/* NOP */
		break;
	case nvgpu_nvlink_sublink_tx_data_ready:
		err = gv100_nvlink_minion_data_ready_en(g, BIT(link_id), true);
		break;
	case nvgpu_nvlink_sublink_tx_prbs_en:
		err = gv100_nvlink_prbs_gen_en(g, BIT(link_id));
		break;
	case nvgpu_nvlink_sublink_tx_safe:
		if (tx_sublink_state ==
				nvl_sl0_slsm_status_tx_primary_state_safe_v()) {
			nvgpu_err(g, "TX already SAFE: %d", link_id);
			break;
		}

		reg = DLPL_REG_RD32(g, link_id, nvl_sublink_change_r());
		reg = set_field(reg, nvl_sublink_change_newstate_m(),
			nvl_sublink_change_newstate_safe_f());
		reg = set_field(reg, nvl_sublink_change_sublink_m(),
			nvl_sublink_change_sublink_tx_f());
		reg = set_field(reg, nvl_sublink_change_action_m(),
			nvl_sublink_change_action_slsm_change_f());
		DLPL_REG_WR32(g, link_id, nvl_sublink_change_r(), reg);

		err = gv100_nvlink_link_sublink_check_change(g, link_id);
		if (err != 0) {
			nvgpu_err(g, "Error in TX to SAFE");
			return err;
		}
		break;
	case nvgpu_nvlink_sublink_tx_off:
		if (tx_sublink_state ==
				nvl_sl0_slsm_status_tx_primary_state_off_v()) {
			nvgpu_err(g, "TX already OFF: %d", link_id);
			break;
		} else if (tx_sublink_state ==
			nvl_sl0_slsm_status_tx_primary_state_hs_v()) {
			nvgpu_err(g, " TX cannot go off from HS %d", link_id);
			return -EPERM;
		}

		reg = DLPL_REG_RD32(g, link_id, nvl_sublink_change_r());
		reg = set_field(reg, nvl_sublink_change_newstate_m(),
			nvl_sublink_change_newstate_off_f());
		reg = set_field(reg, nvl_sublink_change_sublink_m(),
			nvl_sublink_change_sublink_tx_f());
		reg = set_field(reg, nvl_sublink_change_action_m(),
			nvl_sublink_change_action_slsm_change_f());
		DLPL_REG_WR32(g, link_id, nvl_sublink_change_r(), reg);

		err = gv100_nvlink_link_sublink_check_change(g, link_id);
		if (err != 0) {
			nvgpu_err(g, "Error in TX to OFF");
			return err;
		}
		break;

	/* RX modes */
	case nvgpu_nvlink_sublink_rx_hs:
	case nvgpu_nvlink_sublink_rx_safe:
		break;
	case nvgpu_nvlink_sublink_rx_off:
		if (rx_sublink_state ==
				nvl_sl1_slsm_status_rx_primary_state_off_v()) {
			nvgpu_err(g, "RX already OFF: %d", link_id);
			break;
		} else if (rx_sublink_state ==
			nvl_sl1_slsm_status_rx_primary_state_hs_v()) {
			nvgpu_err(g, " RX cannot go off from HS %d", link_id);
			return -EPERM;
		}

		reg = DLPL_REG_RD32(g, link_id, nvl_sublink_change_r());
		reg = set_field(reg, nvl_sublink_change_newstate_m(),
			nvl_sublink_change_newstate_off_f());
		reg = set_field(reg, nvl_sublink_change_sublink_m(),
			nvl_sublink_change_sublink_rx_f());
		reg = set_field(reg, nvl_sublink_change_action_m(),
			nvl_sublink_change_action_slsm_change_f());
		DLPL_REG_WR32(g, link_id, nvl_sublink_change_r(), reg);

		err = gv100_nvlink_link_sublink_check_change(g, link_id);
		if (err != 0) {
			nvgpu_err(g, "Error in RX to OFF");
			return err;
		}
		break;
	case nvgpu_nvlink_sublink_rx_rxcal:
		err = gv100_nvlink_rxcal_en(g, BIT(link_id));
		break;

	default:
		if ((is_rx_sublink) && ((mode < nvgpu_nvlink_sublink_rx_hs) ||
				(mode >= nvgpu_nvlink_sublink_rx__last))) {
			nvgpu_err(g, "Unsupported RX mode %u", mode);
			return -EINVAL;
		}
		if (mode >= nvgpu_nvlink_sublink_tx__last) {
			nvgpu_err(g, "Unsupported TX mode %u", mode);
			return -EINVAL;
		}
		nvgpu_err(g, "MODE %u", mode);
	}

	if (err != 0)
		nvgpu_err(g, " failed on set_sublink_mode");
	return err;
}

u32 gv100_nvlink_link_get_sublink_mode(struct gk20a *g, u32 link_id,
							bool is_rx_sublink)
{
	u32 state;

	if (!(BIT(link_id) & g->nvlink.discovered_links)) {
		if (!is_rx_sublink)
			return nvgpu_nvlink_sublink_tx__last;
		return nvgpu_nvlink_sublink_rx__last;
	}

	if (!is_rx_sublink) {
		state = g->ops.nvlink.get_tx_sublink_state(g, link_id);
		if (state == nvl_sl0_slsm_status_tx_primary_state_hs_v())
			return nvgpu_nvlink_sublink_tx_hs;
		if (state == nvl_sl0_slsm_status_tx_primary_state_eighth_v())
			return nvgpu_nvlink_sublink_tx_single_lane;
		if (state == nvl_sl0_slsm_status_tx_primary_state_safe_v())
			return nvgpu_nvlink_sublink_tx_safe;
		if (state == nvl_sl0_slsm_status_tx_primary_state_off_v())
			return nvgpu_nvlink_sublink_tx_off;
		return nvgpu_nvlink_sublink_tx__last;
	} else {
		state = g->ops.nvlink.get_rx_sublink_state(g, link_id);
		if (state == nvl_sl1_slsm_status_rx_primary_state_hs_v())
			return nvgpu_nvlink_sublink_rx_hs;
		if (state == nvl_sl1_slsm_status_rx_primary_state_eighth_v())
			return nvgpu_nvlink_sublink_rx_single_lane;
		if (state == nvl_sl1_slsm_status_rx_primary_state_safe_v())
			return nvgpu_nvlink_sublink_rx_safe;
		if (state == nvl_sl1_slsm_status_rx_primary_state_off_v())
			return nvgpu_nvlink_sublink_rx_off;
		return nvgpu_nvlink_sublink_rx__last;
	}
	return nvgpu_nvlink_sublink_tx__last;
}

/*
 * Get TX sublink state
 */
u32 gv100_nvlink_link_get_tx_sublink_state(struct gk20a *g, u32 link_id)
{
	u32 reg = DLPL_REG_RD32(g, link_id, nvl_sl0_slsm_status_tx_r());

	return nvl_sl0_slsm_status_tx_primary_state_v(reg);
}

/*
 * Get RX sublink state
 */
u32 gv100_nvlink_link_get_rx_sublink_state(struct gk20a *g, u32 link_id)
{
	u32 reg = DLPL_REG_RD32(g, link_id, nvl_sl1_slsm_status_rx_r());

	return nvl_sl1_slsm_status_rx_primary_state_v(reg);
}

/* Hardcode the link_mask while we wait for VBIOS link_disable_mask field
 * to be updated.
 */
void gv100_nvlink_get_connected_link_mask(u32 *link_mask)
{
	*link_mask = GV100_CONNECTED_LINK_MASK;
}
/*
 * Performs nvlink device level initialization by discovering the topology
 * taking device out of reset, boot minion, set clocks up and common interrupts
 */
int gv100_nvlink_early_init(struct gk20a *g)
{
	int err = 0;
	u32 mc_reset_nvlink_mask;

	if (!nvgpu_is_enabled(g, NVGPU_SUPPORT_NVLINK))
		return -EINVAL;

	err = nvgpu_bios_get_lpwr_nvlink_table_hdr(g);
	if (err != 0) {
		nvgpu_err(g, "Failed to read LWPR_NVLINK_TABLE header\n");
		goto exit;
	}

	err = nvgpu_bios_get_nvlink_config_data(g);
	if (err != 0) {
		nvgpu_err(g, "failed to read nvlink vbios data");
		goto exit;
	}

	err = g->ops.nvlink.discover_ioctrl(g);
	if (err != 0)
		goto exit;

	/* Enable NVLINK in MC */
	mc_reset_nvlink_mask = BIT32(g->nvlink.ioctrl_table[0].reset_enum);
	nvgpu_log(g, gpu_dbg_nvlink, "mc_reset_nvlink_mask: 0x%x",
							mc_reset_nvlink_mask);
	g->ops.mc.reset(g, mc_reset_nvlink_mask);

	err = g->ops.nvlink.discover_link(g);
	if ((err != 0) || (g->nvlink.discovered_links == 0)) {
		nvgpu_err(g, "No links available");
		goto exit;
	}

	err = nvgpu_falcon_sw_init(g, FALCON_ID_MINION);
	if (err != 0) {
		nvgpu_err(g, "failed to sw init FALCON_ID_MINION");
		goto exit;
	}

	g->nvlink.discovered_links &= ~g->nvlink.link_disable_mask;
	nvgpu_log(g, gpu_dbg_nvlink, "link_disable_mask = 0x%08x (from VBIOS)",
		g->nvlink.link_disable_mask);

	/* Links in reset should be removed from initialized link sw state */
	g->nvlink.initialized_links &= __gv100_nvlink_get_link_reset_mask(g);

	/* VBIOS link_disable_mask should be sufficient to find the connected
	 * links. As VBIOS is not updated with correct mask, we parse the DT
	 * node where we hardcode the link_id. DT method is not scalable as same
	 * DT node is used for different dGPUs connected over PCIE.
	 * Remove the DT parsing of link id and use HAL to get link_mask based
	 * on the GPU. This is temporary WAR while we get the VBIOS updated with
	 * correct mask.
	 */
	g->ops.nvlink.get_connected_link_mask(&(g->nvlink.connected_links));

	nvgpu_log(g, gpu_dbg_nvlink, "connected_links = 0x%08x",
						g->nvlink.connected_links);

	/* Track only connected links */
	g->nvlink.discovered_links &= g->nvlink.connected_links;

	nvgpu_log(g, gpu_dbg_nvlink, "discovered_links = 0x%08x (combination)",
		g->nvlink.discovered_links);

	if (hweight32(g->nvlink.discovered_links) > 1) {
		nvgpu_err(g, "more than one link enabled");
		err = -EINVAL;
		goto nvlink_init_exit;
	}

	err = __gv100_nvlink_state_load_hal(g);
	if (err != 0) {
		nvgpu_err(g, " failed Nvlink state load");
		goto nvlink_init_exit;
	}
	err = gv100_nvlink_minion_configure_ac_coupling(g,
					g->nvlink.ac_coupling_mask, true);
	if (err != 0) {
		nvgpu_err(g, " failed Nvlink state load");
		goto nvlink_init_exit;
	}

	/* Program clocks */
	gv100_nvlink_prog_alt_clk(g);

nvlink_init_exit:
	nvgpu_falcon_sw_free(g, FALCON_ID_MINION);
exit:
	return err;
}

int gv100_nvlink_speed_config(struct gk20a *g)
{
	g->nvlink.speed = nvgpu_nvlink_speed_20G;
	g->nvlink.initpll_ordinal = INITPLL_1;
	g->nvlink.initpll_cmd = minion_nvlink_dl_cmd_command_initpll_1_v();
	return 0;
}

u32 gv100_nvlink_falcon_base_addr(struct gk20a *g)
{
	return g->nvlink.minion_base;
}
#endif /* CONFIG_TEGRA_NVLINK */
