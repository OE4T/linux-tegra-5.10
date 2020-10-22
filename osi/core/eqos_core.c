/*
 * Copyright (c) 2018-2020, NVIDIA CORPORATION. All rights reserved.
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

#include "../osi/common/common.h"
#include <osi_core.h>
#include <osd.h>
#include "eqos_core.h"
#include "eqos_mmc.h"

/**
 * @brief eqos_core_safety_config - EQOS MAC core safety configuration
 */
static struct core_func_safety eqos_core_safety_config;

/**
 * @brief eqos_core_safety_writel - Write to safety critical register.
 *
 * @note
 * Algorithm:
 *  - Acquire RW lock, so that eqos_validate_core_regs does not run while
 *    updating the safety critical register.
 *  - call osi_writel() to actually update the memory mapped register.
 *  - Store the same value in eqos_core_safety_config->reg_val[idx],
 *    so that this latest value will be compared when eqos_validate_core_regs
 *    is scheduled.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] val: Value to be written.
 * @param[in] addr: memory mapped register address to be written to.
 * @param[in] idx: Index of register corresponding to enum func_safety_core_regs.
 *
 * @pre MAC has to be out of reset, and clocks supplied.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: Yes
 */
static inline void eqos_core_safety_writel(
				struct osi_core_priv_data *const osi_core,
				nveu32_t val, void *addr,
				nveu32_t idx)
{
	struct core_func_safety *config = &eqos_core_safety_config;

	osi_lock_irq_enabled(&config->core_safety_lock);
	osi_writela(osi_core, val, addr);
	config->reg_val[idx] = (val & config->reg_mask[idx]);
	osi_unlock_irq_enabled(&config->core_safety_lock);
}

/**
 * @brief Initialize the eqos_core_safety_config.
 *
 * @note
 * Algorithm:
 *  - Populate the list of safety critical registers and provide
 *    the address of the register
 *  - Register mask (to ignore reserved/self-critical bits in the reg).
 *    See eqos_validate_core_regs which can be invoked periodically to compare
 *    the last written value to this register vs the actual value read when
 *    eqos_validate_core_regs is scheduled.
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 */
static void eqos_core_safety_init(struct osi_core_priv_data *const osi_core)
{
	struct core_func_safety *config = &eqos_core_safety_config;
	nveu8_t *base = (nveu8_t *)osi_core->base;
	nveu32_t val;
	nveu32_t i, idx;

	/* Initialize all reg address to NULL, since we may not use
	 * some regs depending on the number of MTL queues enabled.
	 */
	for (i = EQOS_MAC_MCR_IDX; i < EQOS_MAX_CORE_SAFETY_REGS; i++) {
		config->reg_addr[i] = OSI_NULL;
	}

	/* Store reg addresses to run periodic read MAC registers.*/
	config->reg_addr[EQOS_MAC_MCR_IDX] = base + EQOS_MAC_MCR;
	config->reg_addr[EQOS_MAC_PFR_IDX] = base + EQOS_MAC_PFR;
	for (i = 0U; i < OSI_EQOS_MAX_HASH_REGS; i++) {
		config->reg_addr[EQOS_MAC_HTR0_IDX + i] =
						 base + EQOS_MAC_HTR_REG(i);
	}
	config->reg_addr[EQOS_MAC_Q0_TXFC_IDX] = base +
						 EQOS_MAC_QX_TX_FLW_CTRL(0U);
	config->reg_addr[EQOS_MAC_RQC0R_IDX] = base + EQOS_MAC_RQC0R;
	config->reg_addr[EQOS_MAC_RQC1R_IDX] = base + EQOS_MAC_RQC1R;
	config->reg_addr[EQOS_MAC_RQC2R_IDX] = base + EQOS_MAC_RQC2R;
	config->reg_addr[EQOS_MAC_IMR_IDX] = base + EQOS_MAC_IMR;
	config->reg_addr[EQOS_MAC_MA0HR_IDX] = base + EQOS_MAC_MA0HR;
	config->reg_addr[EQOS_MAC_MA0LR_IDX] = base + EQOS_MAC_MA0LR;
	config->reg_addr[EQOS_MAC_TCR_IDX] = base + EQOS_MAC_TCR;
	config->reg_addr[EQOS_MAC_SSIR_IDX] = base + EQOS_MAC_SSIR;
	config->reg_addr[EQOS_MAC_TAR_IDX] = base + EQOS_MAC_TAR;
	config->reg_addr[EQOS_PAD_AUTO_CAL_CFG_IDX] = base +
						      EQOS_PAD_AUTO_CAL_CFG;
	/* MTL registers */
	config->reg_addr[EQOS_MTL_RXQ_DMA_MAP0_IDX] = base +
						      EQOS_MTL_RXQ_DMA_MAP0;
	for (i = 0U; i < osi_core->num_mtl_queues; i++) {
		idx = osi_core->mtl_queues[i];
		if (idx >= OSI_EQOS_MAX_NUM_CHANS) {
			continue;
		}

		config->reg_addr[EQOS_MTL_CH0_TX_OP_MODE_IDX + idx] = base +
						EQOS_MTL_CHX_TX_OP_MODE(idx);
		config->reg_addr[EQOS_MTL_TXQ0_QW_IDX + idx] = base +
						EQOS_MTL_TXQ_QW(idx);
		config->reg_addr[EQOS_MTL_CH0_RX_OP_MODE_IDX + idx] = base +
						EQOS_MTL_CHX_RX_OP_MODE(idx);
	}
	/* DMA registers */
	config->reg_addr[EQOS_DMA_SBUS_IDX] = base + EQOS_DMA_SBUS;

	/* Update the register mask to ignore reserved bits/self-clearing bits.
	 * MAC registers */
	config->reg_mask[EQOS_MAC_MCR_IDX] = EQOS_MAC_MCR_MASK;
	config->reg_mask[EQOS_MAC_PFR_IDX] = EQOS_MAC_PFR_MASK;
	for (i = 0U; i < OSI_EQOS_MAX_HASH_REGS; i++) {
		config->reg_mask[EQOS_MAC_HTR0_IDX + i] = EQOS_MAC_HTR_MASK;
	}
	config->reg_mask[EQOS_MAC_Q0_TXFC_IDX] = EQOS_MAC_QX_TXFC_MASK;
	config->reg_mask[EQOS_MAC_RQC0R_IDX] = EQOS_MAC_RQC0R_MASK;
	config->reg_mask[EQOS_MAC_RQC1R_IDX] = EQOS_MAC_RQC1R_MASK;
	config->reg_mask[EQOS_MAC_RQC2R_IDX] = EQOS_MAC_RQC2R_MASK;
	config->reg_mask[EQOS_MAC_IMR_IDX] = EQOS_MAC_IMR_MASK;
	config->reg_mask[EQOS_MAC_MA0HR_IDX] = EQOS_MAC_MA0HR_MASK;
	config->reg_mask[EQOS_MAC_MA0LR_IDX] = EQOS_MAC_MA0LR_MASK;
	config->reg_mask[EQOS_MAC_TCR_IDX] = EQOS_MAC_TCR_MASK;
	config->reg_mask[EQOS_MAC_SSIR_IDX] = EQOS_MAC_SSIR_MASK;
	config->reg_mask[EQOS_MAC_TAR_IDX] = EQOS_MAC_TAR_MASK;
	config->reg_mask[EQOS_PAD_AUTO_CAL_CFG_IDX] =
						EQOS_PAD_AUTO_CAL_CFG_MASK;
	/* MTL registers */
	config->reg_mask[EQOS_MTL_RXQ_DMA_MAP0_IDX] = EQOS_RXQ_DMA_MAP0_MASK;
	for (i = 0U; i < osi_core->num_mtl_queues; i++) {
		idx = osi_core->mtl_queues[i];
		if (idx >= OSI_EQOS_MAX_NUM_CHANS) {
			continue;
		}

		config->reg_mask[EQOS_MTL_CH0_TX_OP_MODE_IDX + idx] =
						EQOS_MTL_TXQ_OP_MODE_MASK;
		config->reg_mask[EQOS_MTL_TXQ0_QW_IDX + idx] =
						EQOS_MTL_TXQ_QW_MASK;
		config->reg_mask[EQOS_MTL_CH0_RX_OP_MODE_IDX + idx] =
						EQOS_MTL_RXQ_OP_MODE_MASK;
	}
	/* DMA registers */
	config->reg_mask[EQOS_DMA_SBUS_IDX] = EQOS_DMA_SBUS_MASK;

	/* Initialize current power-on-reset values of these registers */
	for (i = EQOS_MAC_MCR_IDX; i < EQOS_MAX_CORE_SAFETY_REGS; i++) {
		if (config->reg_addr[i] == OSI_NULL) {
			continue;
		}

		val = osi_readla(osi_core,
				 (nveu8_t *)config->reg_addr[i]);
		config->reg_val[i] = val & config->reg_mask[i];
	}

	osi_lock_init(&config->core_safety_lock);
}

/**
 * @brief Initialize the OSI core private data backup config array
 *
 * @note
 * Algorithm:
 *  - Populate the list of core registers to be saved during suspend.
 *    Fill the address of each register in structure.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 *
 * @param[in] osi_core: OSI core private data structure.
 */
static void eqos_core_backup_init(struct osi_core_priv_data *const osi_core)
{
	struct core_backup *config = &osi_core->backup_config;
	nveu8_t *base = (nveu8_t *)osi_core->base;
	nveu32_t i;

	/* MAC registers backup */
	config->reg_addr[EQOS_MAC_MCR_BAK_IDX] = base + EQOS_MAC_MCR;
	config->reg_addr[EQOS_MAC_EXTR_BAK_IDX] = base + EQOS_MAC_EXTR;
	config->reg_addr[EQOS_MAC_PFR_BAK_IDX] = base + EQOS_MAC_PFR;
	config->reg_addr[EQOS_MAC_VLAN_TAG_BAK_IDX] = base +
						EQOS_MAC_VLAN_TAG;
	config->reg_addr[EQOS_MAC_VLANTIR_BAK_IDX] = base + EQOS_MAC_VLANTIR;
	config->reg_addr[EQOS_MAC_RX_FLW_CTRL_BAK_IDX] = base +
						EQOS_MAC_RX_FLW_CTRL;
	config->reg_addr[EQOS_MAC_RQC0R_BAK_IDX] = base + EQOS_MAC_RQC0R;
	config->reg_addr[EQOS_MAC_RQC1R_BAK_IDX] = base + EQOS_MAC_RQC1R;
	config->reg_addr[EQOS_MAC_RQC2R_BAK_IDX] = base + EQOS_MAC_RQC2R;
	config->reg_addr[EQOS_MAC_ISR_BAK_IDX] = base + EQOS_MAC_ISR;
	config->reg_addr[EQOS_MAC_IMR_BAK_IDX] = base + EQOS_MAC_IMR;
	config->reg_addr[EQOS_MAC_PMTCSR_BAK_IDX] = base + EQOS_MAC_PMTCSR;
	config->reg_addr[EQOS_MAC_LPI_CSR_BAK_IDX] = base + EQOS_MAC_LPI_CSR;
	config->reg_addr[EQOS_MAC_LPI_TIMER_CTRL_BAK_IDX] = base +
						EQOS_MAC_LPI_TIMER_CTRL;
	config->reg_addr[EQOS_MAC_LPI_EN_TIMER_BAK_IDX] = base +
						EQOS_MAC_LPI_EN_TIMER;
	config->reg_addr[EQOS_MAC_ANS_BAK_IDX] = base + EQOS_MAC_ANS;
	config->reg_addr[EQOS_MAC_PCS_BAK_IDX] = base + EQOS_MAC_PCS;
	if (osi_core->mac_ver == OSI_EQOS_MAC_5_00) {
		config->reg_addr[EQOS_5_00_MAC_ARPPA_BAK_IDX] = base +
							EQOS_5_00_MAC_ARPPA;
	}
	config->reg_addr[EQOS_MMC_CNTRL_BAK_IDX] = base + EQOS_MMC_CNTRL;
	if (osi_core->mac_ver == OSI_EQOS_MAC_4_10) {
		config->reg_addr[EQOS_4_10_MAC_ARPPA_BAK_IDX] = base +
							EQOS_4_10_MAC_ARPPA;
	}
	config->reg_addr[EQOS_MAC_TCR_BAK_IDX] = base + EQOS_MAC_TCR;
	config->reg_addr[EQOS_MAC_SSIR_BAK_IDX] = base + EQOS_MAC_SSIR;
	config->reg_addr[EQOS_MAC_STSR_BAK_IDX] = base + EQOS_MAC_STSR;
	config->reg_addr[EQOS_MAC_STNSR_BAK_IDX] = base + EQOS_MAC_STNSR;
	config->reg_addr[EQOS_MAC_STSUR_BAK_IDX] = base + EQOS_MAC_STSUR;
	config->reg_addr[EQOS_MAC_STNSUR_BAK_IDX] = base + EQOS_MAC_STNSUR;
	config->reg_addr[EQOS_MAC_TAR_BAK_IDX] = base + EQOS_MAC_TAR;
	config->reg_addr[EQOS_DMA_BMR_BAK_IDX] = base + EQOS_DMA_BMR;
	config->reg_addr[EQOS_DMA_SBUS_BAK_IDX] = base + EQOS_DMA_SBUS;
	config->reg_addr[EQOS_DMA_ISR_BAK_IDX] = base + EQOS_DMA_ISR;
	config->reg_addr[EQOS_MTL_OP_MODE_BAK_IDX] = base + EQOS_MTL_OP_MODE;
	config->reg_addr[EQOS_MTL_RXQ_DMA_MAP0_BAK_IDX] = base +
						EQOS_MTL_RXQ_DMA_MAP0;

	for (i = 0; i < EQOS_MAX_HTR_REGS; i++) {
		config->reg_addr[EQOS_MAC_HTR_REG_BAK_IDX(i)] = base +
						EQOS_MAC_HTR_REG(i);
	}
	for (i = 0; i < OSI_EQOS_MAX_NUM_QUEUES; i++) {
		config->reg_addr[EQOS_MAC_QX_TX_FLW_CTRL_BAK_IDX(i)] = base +
						EQOS_MAC_QX_TX_FLW_CTRL(i);
	}
	for (i = 0; i < EQOS_MAX_MAC_ADDRESS_FILTER; i++) {
		config->reg_addr[EQOS_MAC_ADDRH_BAK_IDX(i)] = base +
						EQOS_MAC_ADDRH(i);
		config->reg_addr[EQOS_MAC_ADDRL_BAK_IDX(i)] = base +
						EQOS_MAC_ADDRL(i);
	}
	for (i = 0; i < EQOS_MAX_L3_L4_FILTER; i++) {
		config->reg_addr[EQOS_MAC_L3L4_CTR_BAK_IDX(i)] = base +
						EQOS_MAC_L3L4_CTR(i);
		config->reg_addr[EQOS_MAC_L4_ADR_BAK_IDX(i)] = base +
						EQOS_MAC_L4_ADR(i);
		config->reg_addr[EQOS_MAC_L3_AD0R_BAK_IDX(i)] = base +
						EQOS_MAC_L3_AD0R(i);
		config->reg_addr[EQOS_MAC_L3_AD1R_BAK_IDX(i)] = base +
						EQOS_MAC_L3_AD1R(i);
		config->reg_addr[EQOS_MAC_L3_AD2R_BAK_IDX(i)] = base +
						EQOS_MAC_L3_AD2R(i);
		config->reg_addr[EQOS_MAC_L3_AD3R_BAK_IDX(i)] = base +
						EQOS_MAC_L3_AD3R(i);
	}
	for (i = 0; i < OSI_EQOS_MAX_NUM_QUEUES; i++) {
		config->reg_addr[EQOS_MTL_CHX_TX_OP_MODE_BAK_IDX(i)] = base +
						EQOS_MTL_CHX_TX_OP_MODE(i);
		config->reg_addr[EQOS_MTL_TXQ_ETS_CR_BAK_IDX(i)] = base +
						EQOS_MTL_TXQ_ETS_CR(i);
		config->reg_addr[EQOS_MTL_TXQ_QW_BAK_IDX(i)] = base +
						EQOS_MTL_TXQ_QW(i);
		config->reg_addr[EQOS_MTL_TXQ_ETS_SSCR_BAK_IDX(i)] = base +
						EQOS_MTL_TXQ_ETS_SSCR(i);
		config->reg_addr[EQOS_MTL_TXQ_ETS_HCR_BAK_IDX(i)] = base +
						EQOS_MTL_TXQ_ETS_HCR(i);
		config->reg_addr[EQOS_MTL_TXQ_ETS_LCR_BAK_IDX(i)] = base +
						EQOS_MTL_TXQ_ETS_LCR(i);
		config->reg_addr[EQOS_MTL_CHX_RX_OP_MODE_BAK_IDX(i)] = base +
						EQOS_MTL_CHX_RX_OP_MODE(i);
	}

	/* Wrapper register backup */
	config->reg_addr[EQOS_CLOCK_CTRL_0_BAK_IDX] = base +
						EQOS_CLOCK_CTRL_0;
	config->reg_addr[EQOS_AXI_ASID_CTRL_BAK_IDX] = base +
						EQOS_AXI_ASID_CTRL;
	config->reg_addr[EQOS_PAD_CRTL_BAK_IDX] = base + EQOS_PAD_CRTL;
	config->reg_addr[EQOS_PAD_AUTO_CAL_CFG_BAK_IDX] = base +
						EQOS_PAD_AUTO_CAL_CFG;
}

/**
 * @brief eqos_config_flow_control - Configure MAC flow control settings
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] flw_ctrl: flw_ctrl settings
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_config_flow_control(
				    struct osi_core_priv_data *const osi_core,
				    const nveu32_t flw_ctrl)
{
	nveu32_t val;
	void *addr = osi_core->base;

	/* return on invalid argument */
	if (flw_ctrl > (OSI_FLOW_CTRL_RX | OSI_FLOW_CTRL_TX)) {
		OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			     "flw_ctr: invalid input\n", 0ULL);
		return -1;
	}

	/* Configure MAC Tx Flow control */
	/* Read MAC Tx Flow control Register of Q0 */
	val = osi_readla(osi_core,
			 (nveu8_t *)addr + EQOS_MAC_QX_TX_FLW_CTRL(0U));

	/* flw_ctrl BIT0: 1 is for tx flow ctrl enable
	 * flw_ctrl BIT0: 0 is for tx flow ctrl disable
	 */
	if ((flw_ctrl & OSI_FLOW_CTRL_TX) == OSI_FLOW_CTRL_TX) {
		/* Enable Tx Flow Control */
		val |= EQOS_MAC_QX_TX_FLW_CTRL_TFE;
		/* Mask and set Pause Time */
		val &= ~EQOS_MAC_PAUSE_TIME_MASK;
		val |= EQOS_MAC_PAUSE_TIME & EQOS_MAC_PAUSE_TIME_MASK;
	} else {
		/* Disable Tx Flow Control */
		val &= ~EQOS_MAC_QX_TX_FLW_CTRL_TFE;
	}

	/* Write to MAC Tx Flow control Register of Q0 */
	eqos_core_safety_writel(osi_core, val, (nveu8_t *)addr +
				EQOS_MAC_QX_TX_FLW_CTRL(0U),
				EQOS_MAC_Q0_TXFC_IDX);

	/* Configure MAC Rx Flow control*/
	/* Read MAC Rx Flow control Register */
	val = osi_readla(osi_core,
			 (nveu8_t *)addr + EQOS_MAC_RX_FLW_CTRL);

	/* flw_ctrl BIT1: 1 is for rx flow ctrl enable
	 * flw_ctrl BIT1: 0 is for rx flow ctrl disable
	 */
	if ((flw_ctrl & OSI_FLOW_CTRL_RX) == OSI_FLOW_CTRL_RX) {
		/* Enable Rx Flow Control */
		val |= EQOS_MAC_RX_FLW_CTRL_RFE;
	} else {
		/* Disable Rx Flow Control */
		val &= ~EQOS_MAC_RX_FLW_CTRL_RFE;
	}

	/* Write to MAC Rx Flow control Register */
	osi_writela(osi_core, val,
		    (nveu8_t *)addr + EQOS_MAC_RX_FLW_CTRL);

	return 0;
}

/**
 * @brief eqos_config_fw_err_pkts - Configure forwarding of error packets
 *
 * @note
 * Algorithm:
 *  - When this bit is reset, the Rx queue drops packets with
 *    error status (CRC error, GMII_ER, watchdog timeout, or overflow).
 *    When this bit is set, all packets except the runt error packets
 *    are forwarded to the application or DMA.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] qinx: Queue index
 * @param[in] fw_err: Enable or Disable the forwarding of error packets
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_config_fw_err_pkts(
				   struct osi_core_priv_data *const osi_core,
				   const nveu32_t qinx,
				   const nveu32_t fw_err)
{
	void *addr = osi_core->base;
	nveu32_t val;

	/* Check for valid fw_err and qinx values */
	if (((fw_err != OSI_ENABLE) && (fw_err != OSI_DISABLE)) ||
	    (qinx >= OSI_EQOS_MAX_NUM_CHANS)) {
		OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			     "config_fw_err: invalid input\n", 0ULL);
		return -1;
	}

	/* Read MTL RXQ Operation_Mode Register */
	val = osi_readla(osi_core,
			 (nveu8_t *)addr + EQOS_MTL_CHX_RX_OP_MODE(qinx));

	/* fw_err, 1 is for enable and 0 is for disable */
	if (fw_err == OSI_ENABLE) {
		/* When fw_err bit is set, all packets except the runt error
		 * packets are forwarded to the application or DMA.
		 */
		val |= EQOS_MTL_RXQ_OP_MODE_FEP;
	} else if (fw_err == OSI_DISABLE) {
		/* When this bit is reset, the Rx queue drops packets with error
		 * status (CRC error, GMII_ER, watchdog timeout, or overflow)
		 */
		val &= ~EQOS_MTL_RXQ_OP_MODE_FEP;
	} else {
		/* Nothing here */
	}

	/* Write to FEP bit of MTL RXQ operation Mode Register to enable or
	 * disable the forwarding of error packets to DMA or application.
	 */
	eqos_core_safety_writel(osi_core, val, (nveu8_t *)addr +
				EQOS_MTL_CHX_RX_OP_MODE(qinx),
				EQOS_MTL_CH0_RX_OP_MODE_IDX + qinx);

	return 0;
}

/**
 * @brief eqos_poll_for_swr - Poll for software reset (SWR bit in DMA Mode)
 *
 * @note
 * Algorithm:
 *  - CAR reset will be issued through MAC reset pin.
 *    Waits for SWR reset to be cleared in DMA Mode register.
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @pre MAC needs to be out of reset and proper clock configured.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_poll_for_swr(struct osi_core_priv_data *const osi_core)
{
	void *addr = osi_core->base;
	nveu32_t retry = 1000;
	nveu32_t count;
	nveu32_t dma_bmr = 0;
	nve32_t cond = 1;
	nveu32_t pre_si = osi_core->pre_si;

	if (pre_si == OSI_ENABLE) {
		osi_writela(osi_core, 0x1U,
			    (nveu32_t *)addr + EQOS_DMA_BMR);
	}
	/* add delay of 10 usec */
	osi_core->osd_ops.usleep_range(9, 11);

	/* Poll Until Poll Condition */
	count = 0;
	while (cond == 1) {
		if (count > retry) {
			OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_HW_FAIL,
				     "poll_for_swr: timeout\n", 0ULL);
			return -1;
		}

		count++;


		dma_bmr = osi_readla(osi_core,
				     (nveu8_t *)addr + EQOS_DMA_BMR);
		if ((dma_bmr & EQOS_DMA_BMR_SWR) == 0U) {
			cond = 0;
		} else {
			osi_core->osd_ops.msleep(1U);
		}
	}

	return 0;
}

/**
 * @brief eqos_set_speed - Set operating speed
 *
 * @note
 * Algorithm:
 *  - Based on the speed (10/100/1000Mbps) MAC will be configured
 *    accordingly.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] speed:	Operating speed.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 */
static void eqos_set_speed(struct osi_core_priv_data *const osi_core,
			   const nve32_t speed)
{
	nveu32_t  mcr_val;
	void *base = osi_core->base;

	mcr_val = osi_readla(osi_core, (nveu8_t *)base + EQOS_MAC_MCR);
	switch (speed) {
	default:
		mcr_val &= ~EQOS_MCR_PS;
		mcr_val &= ~EQOS_MCR_FES;
		break;
	case OSI_SPEED_1000:
		mcr_val &= ~EQOS_MCR_PS;
		mcr_val &= ~EQOS_MCR_FES;
		break;
	case OSI_SPEED_100:
		mcr_val |= EQOS_MCR_PS;
		mcr_val |= EQOS_MCR_FES;
		break;
	case OSI_SPEED_10:
		mcr_val |= EQOS_MCR_PS;
		mcr_val &= ~EQOS_MCR_FES;
		break;
	}

	eqos_core_safety_writel(osi_core, mcr_val,
				(nveu8_t *)base + EQOS_MAC_MCR,
				EQOS_MAC_MCR_IDX);
}

/**
 * @brief eqos_set_mode - Set operating mode
 *
 * @note
 * Algorithm:
 *  - Based on the mode (HALF/FULL Duplex) MAC will be configured
 *    accordingly.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] mode:	Operating mode.
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_set_mode(struct osi_core_priv_data *const osi_core,
			     const nve32_t mode)
{
	void *base = osi_core->base;
	nveu32_t mcr_val;

	mcr_val = osi_readla(osi_core, (nveu8_t *)base + EQOS_MAC_MCR);
	if (mode == OSI_FULL_DUPLEX) {
		mcr_val |= EQOS_MCR_DM;
		/* DO (disable receive own) bit is not applicable, don't care */
		mcr_val &= ~EQOS_MCR_DO;
	} else if (mode == OSI_HALF_DUPLEX) {
		mcr_val &= ~EQOS_MCR_DM;
		/* Set DO (disable receive own) bit */
		mcr_val |= EQOS_MCR_DO;
	} else {
		OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			     "set_mode: invalid mode\n", 0ULL);
		return -1;
		/* Nothing here */
	}
	eqos_core_safety_writel(osi_core, mcr_val,
				(nveu8_t *)base + EQOS_MAC_MCR,
				EQOS_MAC_MCR_IDX);
	return 0;
}

/**
 * @brief eqos_calculate_per_queue_fifo - Calculate per queue FIFO size
 *
 * @note
 * Algorithm:
 *  - Total Tx/Rx FIFO size which is read from
 *    MAC HW is being shared equally among the queues that are
 *    configured.
 *
 * @param[in] fifo_size: Total Tx/RX HW FIFO size.
 * @param[in] queue_count: Total number of Queues configured.
 *
 * @pre MAC has to be out of reset.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 *
 * @retval Queue size that need to be programmed.
 */
static nveu32_t eqos_calculate_per_queue_fifo(nveu32_t fifo_size,
					      nveu32_t queue_count)
{
	nveu32_t q_fifo_size = 0;  /* calculated fifo size per queue */
	nveu32_t p_fifo = EQOS_256; /* per queue fifo size program value */

	if (queue_count == 0U) {
		return 0U;
	}

	/* calculate Tx/Rx fifo share per queue */
	switch (fifo_size) {
	case 0:
		q_fifo_size = FIFO_SIZE_B(128U);
		break;
	case 1:
		q_fifo_size = FIFO_SIZE_B(256U);
		break;
	case 2:
		q_fifo_size = FIFO_SIZE_B(512U);
		break;
	case 3:
		q_fifo_size = FIFO_SIZE_KB(1U);
		break;
	case 4:
		q_fifo_size = FIFO_SIZE_KB(2U);
		break;
	case 5:
		q_fifo_size = FIFO_SIZE_KB(4U);
		break;
	case 6:
		q_fifo_size = FIFO_SIZE_KB(8U);
		break;
	case 7:
		q_fifo_size = FIFO_SIZE_KB(16U);
		break;
	case 8:
		q_fifo_size = FIFO_SIZE_KB(32U);
		break;
	case 9:
		q_fifo_size = FIFO_SIZE_KB(36U);
		break;
	case 10:
		q_fifo_size = FIFO_SIZE_KB(128U);
		break;
	case 11:
		q_fifo_size = FIFO_SIZE_KB(256U);
		break;
	default:
		q_fifo_size = FIFO_SIZE_KB(36U);
		break;
	}

	q_fifo_size = q_fifo_size / queue_count;

	if (q_fifo_size >= FIFO_SIZE_KB(36U)) {
		p_fifo = EQOS_36K;
	} else if (q_fifo_size >= FIFO_SIZE_KB(32U)) {
		p_fifo = EQOS_32K;
	} else if (q_fifo_size >= FIFO_SIZE_KB(16U)) {
		p_fifo = EQOS_16K;
	} else if (q_fifo_size == FIFO_SIZE_KB(9U)) {
		p_fifo = EQOS_9K;
	} else if (q_fifo_size >= FIFO_SIZE_KB(8U)) {
		p_fifo = EQOS_8K;
	} else if (q_fifo_size >= FIFO_SIZE_KB(4U)) {
		p_fifo = EQOS_4K;
	} else if (q_fifo_size >= FIFO_SIZE_KB(2U)) {
		p_fifo = EQOS_2K;
	} else if (q_fifo_size >= FIFO_SIZE_KB(1U)) {
		p_fifo = EQOS_1K;
	} else if (q_fifo_size >= FIFO_SIZE_B(512U)) {
		p_fifo = EQOS_512;
	} else if (q_fifo_size >= FIFO_SIZE_B(256U)) {
		p_fifo = EQOS_256;
	} else {
		/* Nothing here */
	}

	return p_fifo;
}

/**
 * @brief eqos_pad_calibrate - PAD calibration
 *
 * @note
 * Algorithm:
 *  - Set field PAD_E_INPUT_OR_E_PWRD in reg ETHER_QOS_SDMEMCOMPPADCTRL_0
 *  - Delay for 1 usec.
 *  - Set AUTO_CAL_ENABLE and AUTO_CAL_START in reg
 *    ETHER_QOS_AUTO_CAL_CONFIG_0
 *  - Wait on AUTO_CAL_ACTIVE until it is 0
 *  - Re-program the value PAD_E_INPUT_OR_E_PWRD in
 *    ETHER_QOS_SDMEMCOMPPADCTRL_0 to save power
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @note
 *  - MAC should out of reset and clocks enabled.
 *  - RGMII and MDIO interface needs to be IDLE before performing PAD
 *    calibration.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_pad_calibrate(struct osi_core_priv_data *const osi_core)
{
	void *ioaddr = osi_core->base;
	nveu32_t retry = 1000;
	nveu32_t count;
	nve32_t cond = 1, ret = 0;
	nveu32_t value;

	/* 1. Set field PAD_E_INPUT_OR_E_PWRD in
	 * reg ETHER_QOS_SDMEMCOMPPADCTRL_0
	 */
	value = osi_readla(osi_core, (nveu8_t *)ioaddr + EQOS_PAD_CRTL);
	value |= EQOS_PAD_CRTL_E_INPUT_OR_E_PWRD;
	osi_writela(osi_core, value, (nveu8_t *)ioaddr + EQOS_PAD_CRTL);

	/* 2. delay for 1 usec */
	osi_core->osd_ops.usleep_range(1, 3);

	/* 3. Set AUTO_CAL_ENABLE and AUTO_CAL_START in
	 * reg ETHER_QOS_AUTO_CAL_CONFIG_0.
	 */
	value = osi_readla(osi_core,
			   (nveu8_t *)ioaddr + EQOS_PAD_AUTO_CAL_CFG);
	value |= EQOS_PAD_AUTO_CAL_CFG_START |
		 EQOS_PAD_AUTO_CAL_CFG_ENABLE;
	eqos_core_safety_writel(osi_core, value, (nveu8_t *)ioaddr +
				EQOS_PAD_AUTO_CAL_CFG,
				EQOS_PAD_AUTO_CAL_CFG_IDX);

	/* 4. Wait on 1 to 3 us before start checking for calibration done.
	 *    This delay is consumed in delay inside while loop.
	 */

	/* 5. Wait on AUTO_CAL_ACTIVE until it is 0. 10ms is the timeout */
	count = 0;
	while (cond == 1) {
		if (count > retry) {
			ret = -1;
			goto calibration_failed;
		}
		count++;
		osi_core->osd_ops.usleep_range(10, 12);
		value = osi_readla(osi_core, (nveu8_t *)ioaddr +
				   EQOS_PAD_AUTO_CAL_STAT);
		/* calibration done when CAL_STAT_ACTIVE is zero */
		if ((value & EQOS_PAD_AUTO_CAL_STAT_ACTIVE) == 0U) {
			cond = 0;
		}
	}

calibration_failed:
	/* 6. Re-program the value PAD_E_INPUT_OR_E_PWRD in
	 * ETHER_QOS_SDMEMCOMPPADCTRL_0 to save power
	 */
	value = osi_readla(osi_core, (nveu8_t *)ioaddr + EQOS_PAD_CRTL);
	value &=  ~EQOS_PAD_CRTL_E_INPUT_OR_E_PWRD;
	osi_writela(osi_core, value, (nveu8_t *)ioaddr + EQOS_PAD_CRTL);

	return ret;
}

/**
 * @brief eqos_flush_mtl_tx_queue - Flush MTL Tx queue
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] qinx: MTL queue index.
 *
 * @note
 *  - MAC should out of reset and clocks enabled.
 *  - hw core initialized. see osi_hw_core_init().
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_flush_mtl_tx_queue(
				   struct osi_core_priv_data *const osi_core,
				   const nveu32_t qinx)
{
	void *addr = osi_core->base;
	nveu32_t retry = 1000;
	nveu32_t count;
	nveu32_t value;
	nve32_t cond = 1;

	if (qinx >= OSI_EQOS_MAX_NUM_QUEUES) {
		OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			     "flush_mtl_tx_queue: invalid input\n", 0ULL);
		return -1;
	}

	/* Read Tx Q Operating Mode Register and flush TxQ */
	value = osi_readla(osi_core, (nveu8_t *)addr +
			   EQOS_MTL_CHX_TX_OP_MODE(qinx));
	value |= EQOS_MTL_QTOMR_FTQ;
	eqos_core_safety_writel(osi_core, value, (nveu8_t *)addr +
				EQOS_MTL_CHX_TX_OP_MODE(qinx),
				EQOS_MTL_CH0_TX_OP_MODE_IDX + qinx);

	/* Poll Until FTQ bit resets for Successful Tx Q flush */
	count = 0;
	while (cond == 1) {
		if (count > retry) {
			OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
				     "Poll FTQ bit timeout\n", 0ULL);
				return -1;
		}

		count++;
		osi_core->osd_ops.msleep(1);

		value = osi_readla(osi_core, (nveu8_t *)addr +
				   EQOS_MTL_CHX_TX_OP_MODE(qinx));

		if ((value & EQOS_MTL_QTOMR_FTQ_LPOS) == 0U) {
			cond = 0;
		}
	}

	return 0;
}

/**
 * @brief update_ehfc_rfa_rfd - Update EHFC, RFD and RSA values
 *
 * @note
 * Algorithm:
 *  - Caculates and stores the RSD (Threshold for Deactivating
 *    Flow control) and RSA (Threshold for Activating Flow Control) values
 *    based on the Rx FIFO size and also enables HW flow control
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 *
 * @param[in] rx_fifo: Rx FIFO size.
 * @param[out] value: Stores RFD and RSA values
 */
void update_ehfc_rfa_rfd(nveu32_t rx_fifo, nveu32_t *value)
{
	if (rx_fifo >= EQOS_4K) {
		/* Enable HW Flow Control */
		*value |= EQOS_MTL_RXQ_OP_MODE_EHFC;

		switch (rx_fifo) {
		case EQOS_4K:
			/* Update RFD */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			*value |= (FULL_MINUS_2_5K <<
				   EQOS_MTL_RXQ_OP_MODE_RFD_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			/* Update RFA */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			*value |= (FULL_MINUS_1_5K <<
				   EQOS_MTL_RXQ_OP_MODE_RFA_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			break;
		case EQOS_8K:
			/* Update RFD */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			*value |= (FULL_MINUS_4_K <<
				   EQOS_MTL_RXQ_OP_MODE_RFD_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			/* Update RFA */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			*value |= (FULL_MINUS_6_K <<
				   EQOS_MTL_RXQ_OP_MODE_RFA_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			break;
		case EQOS_9K:
			/* Update RFD */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			*value |= (FULL_MINUS_3_K <<
				   EQOS_MTL_RXQ_OP_MODE_RFD_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			/* Update RFA */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			*value |= (FULL_MINUS_2_K <<
				   EQOS_MTL_RXQ_OP_MODE_RFA_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			break;
		case EQOS_16K:
			/* Update RFD */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			*value |= (FULL_MINUS_4_K <<
				   EQOS_MTL_RXQ_OP_MODE_RFD_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			/* Update RFA */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			*value |= (FULL_MINUS_10_K <<
				   EQOS_MTL_RXQ_OP_MODE_RFA_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			break;
		case EQOS_32K:
			/* Update RFD */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			*value |= (FULL_MINUS_4_K <<
				   EQOS_MTL_RXQ_OP_MODE_RFD_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			/* Update RFA */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			*value |= (FULL_MINUS_16_K <<
				   EQOS_MTL_RXQ_OP_MODE_RFA_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			break;
		default:
			/* Use 9K values */
			/* Update RFD */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			*value |= (FULL_MINUS_3_K <<
				   EQOS_MTL_RXQ_OP_MODE_RFD_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFD_MASK;
			/* Update RFA */
			*value &= ~EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			*value |= (FULL_MINUS_2_K <<
				   EQOS_MTL_RXQ_OP_MODE_RFA_SHIFT) &
				   EQOS_MTL_RXQ_OP_MODE_RFA_MASK;
			break;
		}
	}
}

/**
 * @brief eqos_configure_mtl_queue - Configure MTL Queue
 *
 * @note
 * Algorithm:
 *  - This takes care of configuring the  below
 *    parameters for the MTL Queue
 *    - Mapping MTL Rx queue and DMA Rx channel
 *    - Flush TxQ
 *    - Enable Store and Forward mode for Tx, Rx
 *    - Configure Tx and Rx MTL Queue sizes
 *    - Configure TxQ weight
 *    - Enable Rx Queues
 *
 * @param[in] qinx:	Queue number that need to be configured.
 * @param[in] osi_core: OSI core private data.
 * @param[in] tx_fifo: MTL TX queue size for a MTL queue.
 * @param[in] rx_fifo: MTL RX queue size for a MTL queue.
 *
 * @pre MAC has to be out of reset.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_configure_mtl_queue(nveu32_t qinx,
				    struct osi_core_priv_data *const osi_core,
				    nveu32_t tx_fifo,
				    nveu32_t rx_fifo)
{
	nveu32_t value = 0;
	nve32_t ret = 0;

	ret = eqos_flush_mtl_tx_queue(osi_core, qinx);
	if (ret < 0) {
		return ret;
	}

	value = (tx_fifo << EQOS_MTL_TXQ_SIZE_SHIFT);
	/* Enable Store and Forward mode */
	value |= EQOS_MTL_TSF;
	/* Enable TxQ */
	value |= EQOS_MTL_TXQEN;
	eqos_core_safety_writel(osi_core, value, (nveu8_t *)osi_core->base +
				EQOS_MTL_CHX_TX_OP_MODE(qinx),
				EQOS_MTL_CH0_TX_OP_MODE_IDX + qinx);

	/* read RX Q0 Operating Mode Register */
	value = osi_readla(osi_core, (nveu8_t *)osi_core->base +
			   EQOS_MTL_CHX_RX_OP_MODE(qinx));
	value |= (rx_fifo << EQOS_MTL_RXQ_SIZE_SHIFT);
	/* Enable Store and Forward mode */
	value |= EQOS_MTL_RSF;
	/* Update EHFL, RFA and RFD
	 * EHFL: Enable HW Flow Control
	 * RFA: Threshold for Activating Flow Control
	 * RFD: Threshold for Deactivating Flow Control
	 */
	update_ehfc_rfa_rfd(rx_fifo, &value);
	eqos_core_safety_writel(osi_core, value, (nveu8_t *)osi_core->base +
				EQOS_MTL_CHX_RX_OP_MODE(qinx),
				EQOS_MTL_CH0_RX_OP_MODE_IDX + qinx);

	/* Transmit Queue weight */
	value = osi_readla(osi_core, (nveu8_t *)osi_core->base +
			   EQOS_MTL_TXQ_QW(qinx));
	value |= (EQOS_MTL_TXQ_QW_ISCQW + qinx);
	eqos_core_safety_writel(osi_core, value, (nveu8_t *)osi_core->base +
				EQOS_MTL_TXQ_QW(qinx),
				EQOS_MTL_TXQ0_QW_IDX + qinx);

	/* Enable Rx Queue Control */
	value = osi_readla(osi_core, (nveu8_t *)osi_core->base +
			  EQOS_MAC_RQC0R);
	value |= ((osi_core->rxq_ctrl[qinx] & 0x3U) << (qinx * 2U));
	eqos_core_safety_writel(osi_core, value, (nveu8_t *)osi_core->base +
				EQOS_MAC_RQC0R, EQOS_MAC_RQC0R_IDX);

	return 0;
}

/**
 * @brief eqos_config_rxcsum_offload - Enable/Disable rx checksum offload in HW
 *
 * @note
 * Algorithm:
 *  - Read the MAC configuration register.
 *  - Enable the IP checksum offload engine COE in MAC receiver.
 *  - Update the MAC configuration register.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] enabled: Flag to indicate feature is to be enabled/disabled.
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_config_rxcsum_offload(
				      struct osi_core_priv_data *const osi_core,
				      const nveu32_t enabled)
{
	void *addr = osi_core->base;
	nveu32_t mac_mcr;

	if ((enabled != OSI_ENABLE) && (enabled != OSI_DISABLE)) {
		OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			     "rxsum_offload: invalid input\n", 0ULL);
		return -1;
	}

	mac_mcr = osi_readla(osi_core, (nveu8_t *)addr + EQOS_MAC_MCR);

	if (enabled == OSI_ENABLE) {
		mac_mcr |= EQOS_MCR_IPC;
	} else {
		mac_mcr &= ~EQOS_MCR_IPC;
	}

	eqos_core_safety_writel(osi_core, mac_mcr,
				(nveu8_t *)addr + EQOS_MAC_MCR,
				EQOS_MAC_MCR_IDX);

	return 0;
}

/**
 * @brief eqos_configure_rxq_priority - Configure Priorities Selected in
 *    the Receive Queue
 *
 * @note
 * Algorithm:
 *  - This takes care of mapping user priority to Rx queue.
 *    User provided priority mask updated to register. Valid input can have
 *    all TC(0xFF) in one queue to None(0x00) in rx queue.
 *    The software must ensure that the content of this field is mutually
 *    exclusive to the PSRQ fields for other queues, that is, the same
 *    priority is not mapped to multiple Rx queues.
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @pre MAC has to be out of reset.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 */
static void eqos_configure_rxq_priority(
				struct osi_core_priv_data *const osi_core)
{
	nveu32_t val;
	nveu32_t temp;
	nveu32_t qinx, mtlq;
	nveu32_t pmask = 0x0U;
	nveu32_t mfix_var1, mfix_var2;

	/* make sure EQOS_MAC_RQC2R is reset before programming */
	osi_writela(osi_core, OSI_DISABLE, (nveu8_t *)osi_core->base +
		    EQOS_MAC_RQC2R);

	for (qinx = 0; qinx < osi_core->num_mtl_queues; qinx++) {
		mtlq = osi_core->mtl_queues[qinx];
		/* check for PSRQ field mutual exclusive for all queues */
		if ((osi_core->rxq_prio[mtlq] <= 0xFFU) &&
		    (osi_core->rxq_prio[mtlq] > 0x0U) &&
		    ((pmask & osi_core->rxq_prio[mtlq]) == 0U)) {
			pmask |= osi_core->rxq_prio[mtlq];
			temp = osi_core->rxq_prio[mtlq];
		} else {
			OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
				     "Invalid rxq Priority for Q\n",
				     (nveul64_t)mtlq);
			continue;

		}

		val = osi_readla(osi_core, (nveu8_t *)osi_core->base +
				 EQOS_MAC_RQC2R);
		mfix_var1 = mtlq * (nveu32_t)EQOS_MAC_RQC2_PSRQ_SHIFT;
		mfix_var2 = (nveu32_t)EQOS_MAC_RQC2_PSRQ_MASK;
		mfix_var2 <<= mfix_var1;
		val &= ~mfix_var2;
		temp = temp << (mtlq * EQOS_MAC_RQC2_PSRQ_SHIFT);
		mfix_var1 = mtlq * (nveu32_t)EQOS_MAC_RQC2_PSRQ_SHIFT;
		mfix_var2 = (nveu32_t)EQOS_MAC_RQC2_PSRQ_MASK;
		mfix_var2 <<= mfix_var1;
		val |= (temp & mfix_var2);
		/* Priorities Selected in the Receive Queue 0 */
		eqos_core_safety_writel(osi_core, val,
					(nveu8_t *)osi_core->base +
					EQOS_MAC_RQC2R, EQOS_MAC_RQC2R_IDX);
	}
}

/**
 * @brief eqos_configure_mac - Configure MAC
 *
 * @note
 * Algorithm:
 *  - This takes care of configuring the  below
 *  parameters for the MAC
 *    - Programming the MAC address
 *    - Enable required MAC control fields in MCR
 *    - Enable JE/JD/WD/GPSLCE based on the MTU size
 *    - Enable Multicast and Broadcast Queue
 *    - Disable MMC interrupts and Configure the MMC counters
 *    - Enable required MAC interrupts
 *
 * @param[in, out] osi_core: OSI core private data structure.
 *
 * @pre MAC has to be out of reset.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 */
static void eqos_configure_mac(struct osi_core_priv_data *const osi_core)
{
	nveu32_t value;
	nveu32_t mac_ext;

	/* Update MAC address 0 high */
	value = (((nveu32_t)osi_core->mac_addr[5] << 8U) |
		 ((nveu32_t)osi_core->mac_addr[4]));
	eqos_core_safety_writel(osi_core, value,
				(nveu8_t *)osi_core->base +
				EQOS_MAC_MA0HR, EQOS_MAC_MA0HR_IDX);

	/* Update MAC address 0 Low */
	value = (((nveu32_t)osi_core->mac_addr[3] << 24U) |
		 ((nveu32_t)osi_core->mac_addr[2] << 16U) |
		 ((nveu32_t)osi_core->mac_addr[1] << 8U)  |
		 ((nveu32_t)osi_core->mac_addr[0]));
	eqos_core_safety_writel(osi_core, value, (nveu8_t *)osi_core->base +
				EQOS_MAC_MA0LR, EQOS_MAC_MA0LR_IDX);

	/* Read MAC Configuration Register */
	value = osi_readla(osi_core,
			   (nveu8_t *)osi_core->base + EQOS_MAC_MCR);
	/* Enable Automatic Pad or CRC Stripping */
	/* Enable CRC stripping for Type packets */
	/* Enable Full Duplex mode */
	/* Enable Rx checksum offload engine by default */
	value |= EQOS_MCR_ACS | EQOS_MCR_CST | EQOS_MCR_DM | EQOS_MCR_IPC;

	if ((osi_core->mtu > OSI_DFLT_MTU_SIZE) &&
	    (osi_core->mtu <= OSI_MTU_SIZE_9000)) {
		/* if MTU less than or equal to 9K use JE */
		value |= EQOS_MCR_JE;
		value |= EQOS_MCR_JD;
	} else if (osi_core->mtu > OSI_MTU_SIZE_9000) {
		/* if MTU greater 9K use GPSLCE */
		value |= EQOS_MCR_JD | EQOS_MCR_WD;
		value |= EQOS_MCR_GPSLCE;
		/* Read MAC Extension Register */
		mac_ext = osi_readla(osi_core, (nveu8_t *)osi_core->base +
				     EQOS_MAC_EXTR);
		/* Configure GPSL */
		mac_ext &= ~EQOS_MAC_EXTR_GPSL_MSK;
		mac_ext |= OSI_MAX_MTU_SIZE & EQOS_MAC_EXTR_GPSL_MSK;
		/* Write MAC Extension Register */
		osi_writela(osi_core, mac_ext, (nveu8_t *)osi_core->base +
			    EQOS_MAC_EXTR);
	} else {
		/* do nothing for default mtu size */
	}

	eqos_core_safety_writel(osi_core, value, (nveu8_t *)osi_core->base +
				EQOS_MAC_MCR, EQOS_MAC_MCR_IDX);

	/* Enable Multicast and Broadcast Queue, default is Q0 */
	value = osi_readla(osi_core,
			   (nveu32_t *)osi_core->base + EQOS_MAC_RQC1R);
	value |= EQOS_MAC_RQC1R_MCBCQEN;
	/* Routing Multicast and Broadcast to Q1 */
	value |= EQOS_MAC_RQC1R_MCBCQ1;
	eqos_core_safety_writel(osi_core, value, (nveu8_t *)osi_core->base +
				EQOS_MAC_RQC1R, EQOS_MAC_RQC1R_IDX);

	/* Disable all MMC interrupts */
	/* Disable all MMC Tx Interrupts */
	osi_writela(osi_core, 0xFFFFFFFFU, (nveu8_t *)osi_core->base +
		   EQOS_MMC_TX_INTR_MASK);
	/* Disable all MMC RX interrupts */
	osi_writela(osi_core, 0xFFFFFFFFU, (nveu8_t *)osi_core->base +
		    EQOS_MMC_RX_INTR_MASK);
	/* Disable MMC Rx interrupts for IPC */
	osi_writela(osi_core, 0xFFFFFFFFU, (nveu8_t *)osi_core->base +
		    EQOS_MMC_IPC_RX_INTR_MASK);

	/* Configure MMC counters */
	value = osi_readla(osi_core,
			   (nveu8_t *)osi_core->base + EQOS_MMC_CNTRL);
	value |= EQOS_MMC_CNTRL_CNTRST | EQOS_MMC_CNTRL_RSTONRD |
		 EQOS_MMC_CNTRL_CNTPRST | EQOS_MMC_CNTRL_CNTPRSTLVL;
	osi_writela(osi_core, value,
		    (nveu8_t *)osi_core->base + EQOS_MMC_CNTRL);

	/* Enable MAC interrupts */
	/* Read MAC IMR Register */
	value = osi_readla(osi_core,
			   (nveu8_t *)osi_core->base + EQOS_MAC_IMR);
	/* RGSMIIIE - RGMII/SMII interrupt Enable.
	 * LPIIE is not enabled. MMC LPI counters is maintained in HW */
	value |= EQOS_IMR_RGSMIIIE;

	eqos_core_safety_writel(osi_core, value, (nveu8_t *)osi_core->base +
				EQOS_MAC_IMR, EQOS_MAC_IMR_IDX);

	/* Enable VLAN configuration */
	value = osi_readla(osi_core,
			   (nveu8_t *)osi_core->base + EQOS_MAC_VLAN_TAG);
	/* Enable VLAN Tag stripping always
	 * Enable operation on the outer VLAN Tag, if present
	 * Disable double VLAN Tag processing on TX and RX
	 * Enable VLAN Tag in RX Status
	 * Disable VLAN Type Check
	 */
	if (osi_core->strip_vlan_tag == OSI_ENABLE) {
		value |= EQOS_MAC_VLANTR_EVLS_ALWAYS_STRIP;
	}
	value |= EQOS_MAC_VLANTR_EVLRXS | EQOS_MAC_VLANTR_DOVLTC;
	value &= ~EQOS_MAC_VLANTR_ERIVLT;
	osi_writela(osi_core, value,
		    (nveu8_t *)osi_core->base + EQOS_MAC_VLAN_TAG);

	value = osi_readla(osi_core,
			   (nveu8_t *)osi_core->base + EQOS_MAC_VLANTIR);
	/* Enable VLAN tagging through context descriptor */
	value |= EQOS_MAC_VLANTIR_VLTI;
	/* insert/replace C_VLAN in 13th & 14th bytes of transmitted frames */
	value &= ~EQOS_MAC_VLANTIRR_CSVL;
	osi_writela(osi_core, value,
		    (nveu8_t *)osi_core->base + EQOS_MAC_VLANTIR);

	/* Configure default flow control settings */
	if (osi_core->pause_frames != OSI_PAUSE_FRAMES_DISABLE) {
		osi_core->flow_ctrl = (OSI_FLOW_CTRL_TX | OSI_FLOW_CTRL_RX);
		if (eqos_config_flow_control(osi_core,
					     osi_core->flow_ctrl) != 0) {
			OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_HW_FAIL,
				     "Failed to set flow control configuration\n",
				     0ULL);
		}
	}
	/* USP (user Priority) to RxQ Mapping, only if DCS not enabled */
	if (osi_core->dcs_en != OSI_ENABLE) {
		eqos_configure_rxq_priority(osi_core);
	}
}

/**
 * @brief eqos_configure_dma - Configure DMA
 *
 * @note
 * Algorithm:
 *  - This takes care of configuring the  below
 *  parameters for the DMA
 *    - Programming different burst length for the DMA
 *    - Enable enhanced Address mode
 *    - Programming max read outstanding request limit
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @pre MAC has to be out of reset.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 */
static void eqos_configure_dma(struct osi_core_priv_data *const osi_core)
{
	nveu32_t value = 0;
	void *base = osi_core->base;

	/* AXI Burst Length 8*/
	value |= EQOS_DMA_SBUS_BLEN8;
	/* AXI Burst Length 16*/
	value |= EQOS_DMA_SBUS_BLEN16;
	/* Enhanced Address Mode Enable */
	value |= EQOS_DMA_SBUS_EAME;
	/* AXI Maximum Read Outstanding Request Limit = 31 */
	value |= EQOS_DMA_SBUS_RD_OSR_LMT;
	/* AXI Maximum Write Outstanding Request Limit = 31 */
	value |= EQOS_DMA_SBUS_WR_OSR_LMT;

	eqos_core_safety_writel(osi_core, value,
				(nveu8_t *)base + EQOS_DMA_SBUS,
				EQOS_DMA_SBUS_IDX);

	value = osi_readla(osi_core, (nveu8_t *)base + EQOS_DMA_BMR);
	value |= EQOS_DMA_BMR_DPSW;
	osi_writela(osi_core, value, (nveu8_t *)base + EQOS_DMA_BMR);
}

/**
 * @brief eqos_core_init - EQOS MAC, MTL and common DMA Initialization
 *
 * @note
 * Algorithm:
 *  - This function will take care of initializing MAC, MTL and
 *    common DMA registers.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] tx_fifo_size: MTL TX FIFO size
 * @param[in] rx_fifo_size: MTL RX FIFO size
 *
 * @pre
 *  - MAC should be out of reset. See osi_poll_for_mac_reset_complete()
 *          for details.
 *  - osi_core->base needs to be filled based on ioremap.
 *  - osi_core->num_mtl_queues needs to be filled.
 *  - osi_core->mtl_queues[qinx] need to be filled.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_core_init(struct osi_core_priv_data *const osi_core,
			      const nveu32_t tx_fifo_size,
			      const nveu32_t rx_fifo_size)
{
	nve32_t ret = 0;
	nveu32_t qinx = 0;
	nveu32_t value = 0;
	nveu32_t tx_fifo = 0;
	nveu32_t rx_fifo = 0;

	eqos_core_safety_init(osi_core);
	eqos_core_backup_init(osi_core);

	/* PAD calibration */
	ret = eqos_pad_calibrate(osi_core);
	if (ret < 0) {
		OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_HW_FAIL,
			     "eqos pad calibration failed\n", 0ULL);
		return ret;
	}

	/* reset mmc counters */
	osi_writela(osi_core, EQOS_MMC_CNTRL_CNTRST,
		    (nveu8_t *)osi_core->base + EQOS_MMC_CNTRL);

	/* AXI ASID CTRL for channel 0 to 3 */
	osi_writela(osi_core, EQOS_AXI_ASID_CTRL_VAL,
		    (nveu8_t *)osi_core->base + EQOS_AXI_ASID_CTRL);

	/* AXI ASID1 CTRL for channel 4 to 7 */
	if (osi_core->mac_ver > OSI_EQOS_MAC_5_00) {
		osi_writela(osi_core, EQOS_AXI_ASID1_CTRL_VAL,
			    (nveu8_t *)osi_core->base +
			    EQOS_AXI_ASID1_CTRL);
	}

	/* Mapping MTL Rx queue and DMA Rx channel */
	/* TODO: Need to add EQOS_MTL_RXQ_DMA_MAP1 for EQOS */
	if (osi_core->dcs_en == OSI_ENABLE) {
		value = EQOS_RXQ_TO_DMA_CHAN_MAP_DCS_EN;
	} else {
		value = EQOS_RXQ_TO_DMA_CHAN_MAP;
	}

	eqos_core_safety_writel(osi_core, value, (nveu8_t *)osi_core->base +
				EQOS_MTL_RXQ_DMA_MAP0,
				EQOS_MTL_RXQ_DMA_MAP0_IDX);

	if (osi_unlikely(osi_core->num_mtl_queues > OSI_EQOS_MAX_NUM_QUEUES)) {
		OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			     "Number of queues is incorrect\n", 0ULL);
		return -1;
	}

	/* Calculate value of Transmit queue fifo size to be programmed */
	tx_fifo = eqos_calculate_per_queue_fifo(tx_fifo_size,
						osi_core->num_mtl_queues);
	/* Calculate value of Receive queue fifo size to be programmed */
	rx_fifo = eqos_calculate_per_queue_fifo(rx_fifo_size,
						osi_core->num_mtl_queues);

	/* Configure MTL Queues */
	for (qinx = 0; qinx < osi_core->num_mtl_queues; qinx++) {
		if (osi_unlikely(osi_core->mtl_queues[qinx] >=
				 OSI_EQOS_MAX_NUM_QUEUES)) {
			OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
				     "Incorrect queues number\n", 0ULL);
			return -1;
		}
		ret = eqos_configure_mtl_queue(osi_core->mtl_queues[qinx],
					       osi_core, tx_fifo, rx_fifo);
		if (ret < 0) {
			return ret;
		}
	}

	/* configure EQOS MAC HW */
	eqos_configure_mac(osi_core);

	/* configure EQOS DMA */
	eqos_configure_dma(osi_core);

	/* initialize L3L4 Filters variable */
	osi_core->l3l4_filter_bitmask = OSI_NONE;

	return ret;
}

/**
 * @brief eqos_handle_mac_intrs - Handle MAC interrupts
 *
 * @note
 * Algorithm:
 *  - This function takes care of handling the
 *    MAC interrupts which includes speed, mode detection.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] dma_isr: DMA ISR register read value.
 *
 * @pre MAC interrupts need to be enabled
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 */
static void eqos_handle_mac_intrs(struct osi_core_priv_data *const osi_core,
				  nveu32_t dma_isr)
{
	nveu32_t mac_imr = 0;
	nveu32_t mac_pcs = 0;
	nveu32_t mac_isr = 0;
	nve32_t ret = 0;

	mac_isr = osi_readla(osi_core,
			     (nveu8_t *)osi_core->base + EQOS_MAC_ISR);

	/* Handle MAC interrupts */
	if ((dma_isr & EQOS_DMA_ISR_MACIS) != EQOS_DMA_ISR_MACIS) {
		return;
	}

	/* handle only those MAC interrupts which are enabled */
	mac_imr = osi_readla(osi_core,
			     (nveu8_t *)osi_core->base + EQOS_MAC_IMR);
	mac_isr = (mac_isr & mac_imr);
	/* RGMII/SMII interrupt */
	if ((mac_isr & EQOS_MAC_ISR_RGSMIIS) != EQOS_MAC_ISR_RGSMIIS) {
		return;
	}

	mac_pcs = osi_readla(osi_core,
			     (nveu8_t *)osi_core->base + EQOS_MAC_PCS);
	/* check whether Link is UP or NOT - if not return. */
	if ((mac_pcs & EQOS_MAC_PCS_LNKSTS) != EQOS_MAC_PCS_LNKSTS) {
		return;
	}

	/* check for Link mode (full/half duplex) */
	if ((mac_pcs & EQOS_MAC_PCS_LNKMOD) == EQOS_MAC_PCS_LNKMOD) {
		ret = eqos_set_mode(osi_core, OSI_FULL_DUPLEX);
		if (osi_unlikely(ret < 0)) {
			OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_HW_FAIL,
				     "set mode in full duplex failed\n", 0ULL);
		}
	} else {
		ret = eqos_set_mode(osi_core, OSI_HALF_DUPLEX);
		if (osi_unlikely(ret < 0)) {
			OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_HW_FAIL,
				     "set mode in half duplex failed\n", 0ULL);
		}
	}

	/* set speed at MAC level */
	/* TODO: set_tx_clk needs to be done */
	/* Maybe through workqueue for QNX */
	if ((mac_pcs & EQOS_MAC_PCS_LNKSPEED) == EQOS_MAC_PCS_LNKSPEED_10) {
		eqos_set_speed(osi_core, OSI_SPEED_10);
	} else if ((mac_pcs & EQOS_MAC_PCS_LNKSPEED) ==
		   EQOS_MAC_PCS_LNKSPEED_100) {
		eqos_set_speed(osi_core, OSI_SPEED_100);
	} else if ((mac_pcs & EQOS_MAC_PCS_LNKSPEED) ==
		   EQOS_MAC_PCS_LNKSPEED_1000) {
		eqos_set_speed(osi_core, OSI_SPEED_1000);
	} else {
		/* Nothing here */
	}
}

/**
 * @brief update_dma_sr_stats - stats for dma_status error
 *
 * @note
 * Algorithm:
 *  - increment error stats based on corresponding bit filed.
 *
 * @param[in, out] osi_core: OSI core private data structure.
 * @param[in] dma_sr: Dma status register read value
 * @param[in] qinx: Queue index
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 */
static inline void update_dma_sr_stats(
				struct osi_core_priv_data *const osi_core,
				nveu32_t dma_sr, nveu32_t qinx)
{
	nveu64_t val;

	if ((dma_sr & EQOS_DMA_CHX_STATUS_RBU) == EQOS_DMA_CHX_STATUS_RBU) {
		val = osi_core->xstats.rx_buf_unavail_irq_n[qinx];
		osi_core->xstats.rx_buf_unavail_irq_n[qinx] =
			osi_update_stats_counter(val, 1U);
	}
	if ((dma_sr & EQOS_DMA_CHX_STATUS_TPS) == EQOS_DMA_CHX_STATUS_TPS) {
		val = osi_core->xstats.tx_proc_stopped_irq_n[qinx];
		osi_core->xstats.tx_proc_stopped_irq_n[qinx] =
			osi_update_stats_counter(val, 1U);
	}
	if ((dma_sr & EQOS_DMA_CHX_STATUS_TBU) == EQOS_DMA_CHX_STATUS_TBU) {
		val = osi_core->xstats.tx_buf_unavail_irq_n[qinx];
		osi_core->xstats.tx_buf_unavail_irq_n[qinx] =
			osi_update_stats_counter(val, 1U);
	}
	if ((dma_sr & EQOS_DMA_CHX_STATUS_RPS) == EQOS_DMA_CHX_STATUS_RPS) {
		val = osi_core->xstats.rx_proc_stopped_irq_n[qinx];
		osi_core->xstats.rx_proc_stopped_irq_n[qinx] =
			osi_update_stats_counter(val, 1U);
	}
	if ((dma_sr & EQOS_DMA_CHX_STATUS_RWT) == EQOS_DMA_CHX_STATUS_RWT) {
		val = osi_core->xstats.rx_watchdog_irq_n;
		osi_core->xstats.rx_watchdog_irq_n =
			osi_update_stats_counter(val, 1U);
	}
	if ((dma_sr & EQOS_DMA_CHX_STATUS_FBE) == EQOS_DMA_CHX_STATUS_FBE) {
		val = osi_core->xstats.fatal_bus_error_irq_n;
		osi_core->xstats.fatal_bus_error_irq_n =
			osi_update_stats_counter(val, 1U);
	}
}

/**
 * @brief eqos_handle_common_intr - Handles common interrupt.
 *
 * @note
 * Algorithm:
 *  - Clear common interrupt source.
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 */
static void eqos_handle_common_intr(struct osi_core_priv_data *const osi_core)
{
	void *base = osi_core->base;
	nveu32_t dma_isr = 0;
	nveu32_t qinx = 0;
	nveu32_t i = 0;
	nveu32_t dma_sr = 0;
	nveu32_t dma_ier = 0;

	dma_isr = osi_readla(osi_core, (nveu8_t *)base + EQOS_DMA_ISR);
	if (dma_isr == 0U) {
		return;
	}

	//FIXME Need to check how we can get the DMA channel here instead of
	//MTL Queues
	if ((dma_isr & 0xFU) != 0U) {
		/* Handle Non-TI/RI interrupts */
		for (i = 0; i < osi_core->num_mtl_queues; i++) {
			qinx = osi_core->mtl_queues[i];
			if (qinx >= OSI_EQOS_MAX_NUM_CHANS) {
				continue;
			}

			/* read dma channel status register */
			dma_sr = osi_readla(osi_core, (nveu8_t *)base +
					    EQOS_DMA_CHX_STATUS(qinx));
			/* read dma channel interrupt enable register */
			dma_ier = osi_readla(osi_core, (nveu8_t *)base +
					     EQOS_DMA_CHX_IER(qinx));

			/* process only those interrupts which we
			 * have enabled.
			 */
			dma_sr = (dma_sr & dma_ier);

			/* mask off RI and TI */
			dma_sr &= ~(OSI_BIT(6) | OSI_BIT(0));
			if (dma_sr == 0U) {
				continue;
			}

			/* ack non ti/ri ints */
			osi_writela(osi_core, dma_sr, (nveu8_t *)base +
				    EQOS_DMA_CHX_STATUS(qinx));
			update_dma_sr_stats(osi_core, dma_sr, qinx);
		}
	}

	eqos_handle_mac_intrs(osi_core, dma_isr);
}

/**
 * @brief eqos_start_mac - Start MAC Tx/Rx engine
 *
 * @note
 * Algorithm:
 *  - Enable MAC Transmitter and Receiver
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @pre
 *  - MAC init should be complete. See osi_hw_core_init() and
 *    osi_hw_dma_init()
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 */
static void eqos_start_mac(struct osi_core_priv_data *const osi_core)
{
	nveu32_t value;
	void *addr = osi_core->base;

	value = osi_readla(osi_core, (nveu8_t *)addr + EQOS_MAC_MCR);
	/* Enable MAC Transmit */
	/* Enable MAC Receive */
	value |= EQOS_MCR_TE | EQOS_MCR_RE;
	eqos_core_safety_writel(osi_core, value,
				(nveu8_t *)addr + EQOS_MAC_MCR,
				EQOS_MAC_MCR_IDX);
}

/**
 * @brief eqos_stop_mac - Stop MAC Tx/Rx engine
 *
 * @note
 * Algorithm:
 *  - Disables MAC Transmitter and Receiver
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @pre MAC DMA deinit should be complete. See osi_hw_dma_deinit()
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: No
 * - De-initialization: Yes
 */
static void eqos_stop_mac(struct osi_core_priv_data *const osi_core)
{
	nveu32_t value;
	void *addr = osi_core->base;

	value = osi_readla(osi_core, (nveu8_t *)addr + EQOS_MAC_MCR);
	/* Disable MAC Transmit */
	/* Disable MAC Receive */
	value &= ~EQOS_MCR_TE;
	value &= ~EQOS_MCR_RE;
	eqos_core_safety_writel(osi_core, value,
				(nveu8_t *)addr + EQOS_MAC_MCR,
				EQOS_MAC_MCR_IDX);
}

/**
 * @brief eqos_config_l2_da_perfect_inverse_match - configure register for
 *  inverse or perfect match.
 *
 * @note
 * Algorithm:
 *  - This sequence is used to select perfect/inverse matching
 *    for L2 DA
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] perfect_inverse_match: 1 - inverse mode 0- perfect mode
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 always
 */
static inline nve32_t eqos_config_l2_da_perfect_inverse_match(
				struct osi_core_priv_data *const osi_core,
				nveu32_t perfect_inverse_match)
{
	nveu32_t value = 0U;

	value = osi_readla(osi_core,
			   (nveu8_t *)osi_core->base + EQOS_MAC_PFR);
	value &= ~EQOS_MAC_PFR_DAIF;
	if (perfect_inverse_match == OSI_INV_MATCH) {
		value |= EQOS_MAC_PFR_DAIF;
	}
	eqos_core_safety_writel(osi_core, value,
				(nveu8_t *)osi_core->base + EQOS_MAC_PFR,
				EQOS_MAC_PFR_IDX);

	return 0;
}

/**
 * @brief eqos_config_mac_pkt_filter_reg - configure mac filter register.
 *
 * @note
 * Algorithm:
 *  - This sequence is used to configure MAC in different pkt
 *    processing modes like promiscuous, multicast, unicast,
 *    hash unicast/multicast.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] filter: OSI filter structure.
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 always
 */
static nve32_t eqos_config_mac_pkt_filter_reg(
				struct osi_core_priv_data *const osi_core,
				const struct osi_filter *filter)
{
	nveu32_t value = 0U;
	nve32_t ret = 0;

	value = osi_readla(osi_core, (nveu8_t *)osi_core->base + EQOS_MAC_PFR);

	/*Retain all other values */
	value &= (EQOS_MAC_PFR_DAIF | EQOS_MAC_PFR_DBF | EQOS_MAC_PFR_SAIF |
		  EQOS_MAC_PFR_SAF | EQOS_MAC_PFR_PCF | EQOS_MAC_PFR_VTFE |
		  EQOS_MAC_PFR_IPFE | EQOS_MAC_PFR_DNTU | EQOS_MAC_PFR_RA);

	if ((filter->oper_mode & OSI_OPER_EN_PROMISC) != OSI_DISABLE) {
		value |= EQOS_MAC_PFR_PR;
	}

	if ((filter->oper_mode & OSI_OPER_DIS_PROMISC) != OSI_DISABLE) {
		value &= ~EQOS_MAC_PFR_PR;
	}

	if ((filter->oper_mode & OSI_OPER_EN_ALLMULTI) != OSI_DISABLE) {
		value |= EQOS_MAC_PFR_PM;
	}

	if ((filter->oper_mode & OSI_OPER_DIS_ALLMULTI) != OSI_DISABLE) {
		value &= ~EQOS_MAC_PFR_PM;
	}

	if ((filter->oper_mode & OSI_OPER_EN_PERFECT) != OSI_DISABLE) {
		value |= EQOS_MAC_PFR_HPF;
	}

	if ((filter->oper_mode & OSI_OPER_DIS_PERFECT) != OSI_DISABLE) {
		value &= ~EQOS_MAC_PFR_HPF;
	}


	eqos_core_safety_writel(osi_core, value, (nveu8_t *)osi_core->base +
				EQOS_MAC_PFR, EQOS_MAC_PFR_IDX);

	if ((filter->oper_mode & OSI_OPER_EN_L2_DA_INV) != 0x0U) {
		ret = eqos_config_l2_da_perfect_inverse_match(osi_core,
							      OSI_INV_MATCH);
	}

	if ((filter->oper_mode & OSI_OPER_DIS_L2_DA_INV) != 0x0U) {
		ret = eqos_config_l2_da_perfect_inverse_match(osi_core,
							      OSI_PFT_MATCH);
	}

	return ret;
}

/**
 * @brief eqos_update_mac_addr_helper - Function to update DCS and MBC
 *
 * @note
 * Algorithm:
 *  - This helper routine is to update passed parameter value
 *  based on DCS and MBC parameter. Validation of dma_chan as well as
 *  dsc_en status performed before updating DCS bits.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[out] value: nveu32_t pointer which has value read from register.
 * @param[in] idx: filter index
 * @param[in] dma_routing_enable: dma channel routing enable(1)
 * @param[in] dma_chan: dma channel number
 * @param[in] addr_mask: filter will not consider byte in comparison
 *            Bit 5: MAC_Address${i}_High[15:8]
 *            Bit 4: MAC_Address${i}_High[7:0]
 *            Bit 3: MAC_Address${i}_Low[31:24]
 *            ..
 *            Bit 0: MAC_Address${i}_Low[7:0]
 *
 * @pre
 *  - MAC should be initialized and started. see osi_start_mac()
 *  - osi_core->osd should be populated.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static inline nve32_t eqos_update_mac_addr_helper(
				struct osi_core_priv_data *const osi_core,
				nveu32_t *value,
				const nveu32_t idx,
				const nveu32_t dma_routing_enable,
				const nveu32_t dma_chan,
				const nveu32_t addr_mask)
{
	nve32_t ret = 0;
	/* PDC bit of MAC_Ext_Configuration register is not set so binary
	 * value representation.
	 */
	if (dma_routing_enable == OSI_ENABLE) {
		if ((dma_chan < OSI_EQOS_MAX_NUM_CHANS) &&
		    (osi_core->dcs_en == OSI_ENABLE)) {
			*value = ((dma_chan << EQOS_MAC_ADDRH_DCS_SHIFT) &
				  EQOS_MAC_ADDRH_DCS);
		} else if ((dma_chan == OSI_CHAN_ANY) ||
			   (dma_chan > (OSI_EQOS_MAX_NUM_CHANS - 0x1U))) {
			OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_OUTOFBOUND,
				     "invalid dma channel\n",
				     (nveul64_t)dma_chan);
			ret = -1;
			goto err_dma_chan;
		} else {
		/* Do nothing */
		}
	}

	/* Address mask is valid for address 1 to 31 index only */
	if ((addr_mask <= EQOS_MAX_MASK_BYTE) &&
	    (addr_mask > OSI_AMASK_DISABLE)) {
		if ((idx > 0U) && (idx < EQOS_MAX_MAC_ADDR_REG)) {
			*value = (*value |
				  ((addr_mask << EQOS_MAC_ADDRH_MBC_SHIFT) &
				   EQOS_MAC_ADDRH_MBC));
		} else {
			OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
				     "invalid address index for MBC\n",
				     0ULL);
			ret = -1;
		}
	}

err_dma_chan:
	return ret;
}

/**
 * @brief eqos_update_mac_addr_low_high_reg- Update L2 address in filter
 *    register
 *
 * @note
 * Algorithm:
 *  - This routine update MAC address to register for filtering
 *    based on dma_routing_enable, addr_mask and src_dest. Validation of
 *    dma_chan as well as DCS bit enabled in RXQ to DMA mapping register
 *    performed before updating DCS bits.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] filter: OSI filter structure.
 *
 * @pre
 *  - MAC should be initialized and started. see osi_start_mac()
 *  - osi_core->osd should be populated.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_update_mac_addr_low_high_reg(
				struct osi_core_priv_data *const osi_core,
				const struct osi_filter *filter)
{
	nveu32_t idx = filter->index;
	nveu32_t dma_routing_enable = filter->dma_routing;
	nveu32_t dma_chan = filter->dma_chan;
	nveu32_t addr_mask = filter->addr_mask;
	nveu32_t src_dest = filter->src_dest;
	nveu32_t value = 0x0U;
	nve32_t ret = 0;

	if (idx > (EQOS_MAX_MAC_ADDRESS_FILTER -  0x1U)) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			     "invalid MAC filter index\n", 0ULL);
		return -1;
	}

	ret = eqos_update_mac_addr_helper(osi_core, &value, idx,
					  dma_routing_enable, dma_chan,
					  addr_mask);
	/* Check return value from helper code */
	if (ret == -1) {
		return ret;
	}

	/* Setting Source/Destination Address match valid for 1 to 32 index */
	if (((idx > 0U) && (idx < EQOS_MAX_MAC_ADDR_REG)) &&
	    ((src_dest == OSI_SA_MATCH) || (src_dest == OSI_DA_MATCH))) {
		value = (value | ((src_dest << EQOS_MAC_ADDRH_SA_SHIFT) &
			EQOS_MAC_ADDRH_SA));
	}

	/* Update AE bit if OSI_OPER_ADDR_UPDATE is set */
	if ((filter->oper_mode & OSI_OPER_ADDR_UPDATE) ==
	     OSI_OPER_ADDR_UPDATE) {
		value |= EQOS_MAC_ADDRH_AE;
	}

	osi_writela(osi_core, ((nveu32_t)filter->mac_address[4] |
		    ((nveu32_t)filter->mac_address[5] << 8) | value),
		    (nveu8_t *)osi_core->base + EQOS_MAC_ADDRH((idx)));

	osi_writela(osi_core, ((nveu32_t)filter->mac_address[0] |
		    ((nveu32_t)filter->mac_address[1] << 8) |
		    ((nveu32_t)filter->mac_address[2] << 16) |
		    ((nveu32_t)filter->mac_address[3] << 24)),
		    (nveu8_t *)osi_core->base +  EQOS_MAC_ADDRL((idx)));

	return ret;
}

/**
 * @brief eqos_config_l3_l4_filter_enable - register write to enable L3/L4
 *  filters.
 *
 * @note
 * Algorithm:
 *  - This routine to enable/disable L4/l4 filter
 *
 * @param[in] base: Base address from OSI core private data structure.
 * @param[in] filter_enb_dis: enable/disable
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_config_l3_l4_filter_enable(
				struct osi_core_priv_data *const osi_core,
				const nveu32_t filter_enb_dis)
{
	nveu32_t value = 0U;
	void *base = osi_core->base;
	value = osi_readla(osi_core, (nveu8_t *)base + EQOS_MAC_PFR);
	value &= ~(EQOS_MAC_PFR_IPFE);
	value |= ((filter_enb_dis << 20) & EQOS_MAC_PFR_IPFE);
	eqos_core_safety_writel(osi_core, value, (nveu8_t *)base + EQOS_MAC_PFR,
				EQOS_MAC_PFR_IDX);

	return 0;
}

/**
 * @brief eqos_update_ip4_addr - configure register for IPV4 address filtering
 *
 * @note
 * Algorithm:
 *  - This sequence is used to update IPv4 source/destination
 *    Address for L3 layer filtering
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] filter_no: filter index
 * @param[in] addr: ipv4 address
 * @param[in] src_dst_addr_match: 0 - source addr otherwise - dest addr
 *
 * @pre 1) MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_update_ip4_addr(struct osi_core_priv_data *const osi_core,
				    const nveu32_t filter_no,
				    const nveu8_t addr[],
				    const nveu32_t src_dst_addr_match)
{
	void *base = osi_core->base;
	nveu32_t value = 0U;
	nveu32_t temp = 0U;

	if (addr == OSI_NULL) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			     "invalid address\n", 0ULL);
		return -1;
	}

	if (filter_no > (EQOS_MAX_L3_L4_FILTER - 0x1U)) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_OUTOFBOUND,
			     "invalid filter index for L3/L4 filter\n",
			     (nveul64_t)filter_no);
		return -1;
	}

	value = addr[3];
	temp = (nveu32_t)addr[2] << 8;
	value |= temp;
	temp = (nveu32_t)addr[1] << 16;
	value |= temp;
	temp = (nveu32_t)addr[0] << 24;
	value |= temp;
	if (src_dst_addr_match == OSI_SOURCE_MATCH) {
		osi_writela(osi_core, value, (nveu8_t *)base +
			    EQOS_MAC_L3_AD0R(filter_no));
	} else {
		osi_writela(osi_core, value, (nveu8_t *)base +
			    EQOS_MAC_L3_AD1R(filter_no));
	}

	return 0;
}

/**
 * @brief eqos_update_ip6_addr - add ipv6 address in register
 *
 * @note
 * Algorithm:
 *  - This sequence is used to update IPv6 source/destination
 *    Address for L3 layer filtering
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] filter_no: filter index
 * @param[in] addr: ipv6 address
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_update_ip6_addr(struct osi_core_priv_data *const osi_core,
				    const nveu32_t filter_no,
				    const nveu16_t addr[])
{
	void *base = osi_core->base;
	nveu32_t value = 0U;
	nveu32_t temp = 0U;

	if (addr == OSI_NULL) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			     "invalid address\n", 0ULL);
		return -1;
	}

	if (filter_no > (EQOS_MAX_L3_L4_FILTER - 0x1U)) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			     "invalid filter index for L3/L4 filter\n",
			     (nveul64_t)filter_no);
		return -1;
	}

	/* update Bits[31:0] of 128-bit IP addr */
	value = addr[7];
	temp = (nveu32_t)addr[6] << 16;
	value |= temp;
	osi_writela(osi_core, value, (nveu8_t *)base +
		    EQOS_MAC_L3_AD0R(filter_no));
	/* update Bits[63:32] of 128-bit IP addr */
	value = addr[5];
	temp = (nveu32_t)addr[4] << 16;
	value |= temp;
	osi_writela(osi_core, value, (nveu8_t *)base +
		    EQOS_MAC_L3_AD1R(filter_no));
	/* update Bits[95:64] of 128-bit IP addr */
	value = addr[3];
	temp = (nveu32_t)addr[2] << 16;
	value |= temp;
	osi_writela(osi_core, value, (nveu8_t *)base +
		    EQOS_MAC_L3_AD2R(filter_no));
	/* update Bits[127:96] of 128-bit IP addr */
	value = addr[1];
	temp = (nveu32_t)addr[0] << 16;
	value |= temp;
	osi_writela(osi_core, value, (nveu8_t *)base +
		    EQOS_MAC_L3_AD3R(filter_no));

	return 0;
}

/**
 * @brief eqos_update_l4_port_no -program source  port no
 *
 * @note
 * Algorithm:
 *  - sequence is used to update Source Port Number for
 *    L4(TCP/UDP) layer filtering.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] filter_no: filter index
 * @param[in] port_no: port number
 * @param[in] src_dst_port_match: 0 - source port, otherwise - dest port
 *
 * @pre
 *  - MAC should be initialized and started. see osi_start_mac()
 *  - osi_core->osd should be populated.
 *  - DCS bits should be enabled in RXQ to DMA mapping register
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_update_l4_port_no(
				  struct osi_core_priv_data *const osi_core,
				  const nveu32_t filter_no,
				  const nveu16_t port_no,
				  const nveu32_t src_dst_port_match)
{
	void *base = osi_core->base;
	nveu32_t value = 0U;
	nveu32_t temp = 0U;

	if (filter_no > (EQOS_MAX_L3_L4_FILTER - 0x1U)) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_OUTOFBOUND,
			     "invalid filter index for L3/L4 filter\n",
			     (nveul64_t)filter_no);
		return -1;
	}

	value = osi_readla(osi_core,
			   (nveu8_t *)base + EQOS_MAC_L4_ADR(filter_no));
	if (src_dst_port_match == OSI_SOURCE_MATCH) {
		value &= ~EQOS_MAC_L4_SP_MASK;
		value |= ((nveu32_t)port_no  & EQOS_MAC_L4_SP_MASK);
	} else {
		value &= ~EQOS_MAC_L4_DP_MASK;
		temp = port_no;
		value |= ((temp << EQOS_MAC_L4_DP_SHIFT) & EQOS_MAC_L4_DP_MASK);
	}
	osi_writela(osi_core, value,
		    (nveu8_t *)base +  EQOS_MAC_L4_ADR(filter_no));

	return 0;
}

/**
 * @brief eqos_set_dcs - check and update dma routing register
 *
 * @note
 * Algorithm:
 *  - Check for request for DCS_enable as well as validate chan
 *    number and dcs_enable is set. After validation, this sequence is used
 *    to configure L3((IPv4/IPv6) filters for address matching.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] value: nveu32_t value for caller
 * @param[in] dma_routing_enable: filter based dma routing enable(1)
 * @param[in] dma_chan: dma channel for routing based on filter
 *
 * @pre
 *  - MAC should be initialized and started. see osi_start_mac()
 *  - DCS bit of RxQ should be enabled for dynamic channel selection
 *    in filter support
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 *@return updated nveu32_t value
 */
static inline nveu32_t eqos_set_dcs(
				struct osi_core_priv_data *const osi_core,
				nveu32_t value,
				nveu32_t dma_routing_enable,
				nveu32_t dma_chan)
{
	nveu32_t t_val = value;

	if ((dma_routing_enable == OSI_ENABLE) && (dma_chan <
	    OSI_EQOS_MAX_NUM_CHANS) && (osi_core->dcs_en ==
	    OSI_ENABLE)) {
		t_val |= ((dma_routing_enable <<
			  EQOS_MAC_L3L4_CTR_DMCHEN0_SHIFT) &
			  EQOS_MAC_L3L4_CTR_DMCHEN0);
		t_val |= ((dma_chan <<
			  EQOS_MAC_L3L4_CTR_DMCHN0_SHIFT) &
			  EQOS_MAC_L3L4_CTR_DMCHN0);
	}

	return t_val;
}

/**
 * @brief eqos_helper_l3l4_bitmask - helper function to set L3L4
 * bitmask.
 *
 * @note
 * Algorithm:
 *  - set bit corresponding to L3l4 filter index
 *
 * @param[out] bitmask: bit mask OSI core private data structure.
 * @param[in] filter_no: filter index
 * @param[in] value:  0 - disable  otherwise - l3/l4 filter enabled
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 *
 */
static inline void eqos_helper_l3l4_bitmask(nveu32_t *bitmask,
					    nveu32_t filter_no,
					    nveu32_t value)
{
	nveu32_t temp;

	/* Set bit mask for index */
	temp = OSI_ENABLE;
	temp = temp << filter_no;
	/* check against all bit fields for L3L4 filter enable */
	if ((value & EQOS_MAC_L3L4_CTRL_ALL) != OSI_DISABLE) {
		*bitmask |= temp;
	} else {
		*bitmask &= ~temp;
	}
}

/**
 * @brief eqos_config_l3_filters - config L3 filters.
 *
 * @note
 * Algorithm:
 *  - Check for DCS_enable as well as validate channel
 *    number and if dcs_enable is set. After validation, code flow
 *    is used to configure L3((IPv4/IPv6) filters resister
 *    for address matching.
 *
 * @param[in, out] osi_core: OSI core private data structure.
 * @param[in] filter_no: filter index
 * @param[in] enb_dis:  1 - enable otherwise - disable L3 filter
 * @param[in] ipv4_ipv6_match: 1 - IPv6, otherwise - IPv4
 * @param[in] src_dst_addr_match: 0 - source, otherwise - destination
 * @param[in] perfect_inverse_match: normal match(0) or inverse map(1)
 * @param[in] dma_routing_enable: filter based dma routing enable(1)
 * @param[in] dma_chan: dma channel for routing based on filter
 *
 * @pre
 *  - MAC should be initialized and started. see osi_start_mac()
 *  - osi_core->osd should be populated.
 *  - DCS bit of RxQ should be enabled for dynamic channel selection
 *    in filter support
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_config_l3_filters(
				  struct osi_core_priv_data *const osi_core,
				  const nveu32_t filter_no,
				  const nveu32_t enb_dis,
				  const nveu32_t ipv4_ipv6_match,
				  const nveu32_t src_dst_addr_match,
				  const nveu32_t perfect_inverse_match,
				  const nveu32_t dma_routing_enable,
				  const nveu32_t dma_chan)
{
	nveu32_t value = 0U;
	void *base = osi_core->base;

	if (filter_no > (EQOS_MAX_L3_L4_FILTER - 0x1U)) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_OUTOFBOUND,
			     "invalid filter index for L3/L4 filter\n",
			     (nveul64_t)filter_no);
		return -1;
	}

	if ((dma_routing_enable == OSI_ENABLE) &&
	    (dma_chan > (OSI_EQOS_MAX_NUM_CHANS - 1U))) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_OUTOFBOUND,
			     "Wrong DMA channel\n", (nveul64_t)dma_chan);
		return -1;
	}

	value = osi_readla(osi_core, (nveu8_t *)base +
			  EQOS_MAC_L3L4_CTR(filter_no));
	value &= ~EQOS_MAC_L3L4_CTR_L3PEN0;
	value |= (ipv4_ipv6_match  & EQOS_MAC_L3L4_CTR_L3PEN0);
	osi_writela(osi_core, value, (nveu8_t *)base +
		    EQOS_MAC_L3L4_CTR(filter_no));

	/* For IPv6 either SA/DA can be checked not both */
	if (ipv4_ipv6_match == OSI_IPV6_MATCH) {
		if (enb_dis == OSI_ENABLE) {
			if (src_dst_addr_match == OSI_SOURCE_MATCH) {
				/* Enable L3 filters for IPv6 SOURCE addr
				 *  matching
				 */
				value = osi_readla(osi_core, (nveu8_t *)base +
						  EQOS_MAC_L3L4_CTR(filter_no));
				value &= ~EQOS_MAC_L3_IP6_CTRL_CLEAR;
				value |= ((EQOS_MAC_L3L4_CTR_L3SAM0 |
					  (perfect_inverse_match <<
					   EQOS_MAC_L3L4_CTR_L3SAI_SHIFT)) &
					  ((EQOS_MAC_L3L4_CTR_L3SAM0 |
					  EQOS_MAC_L3L4_CTR_L3SAIM0)));
				value |= eqos_set_dcs(osi_core, value,
						      dma_routing_enable,
						      dma_chan);
				osi_writela(osi_core, value, (nveu8_t *)base +
					    EQOS_MAC_L3L4_CTR(filter_no));

			} else {
				/* Enable L3 filters for IPv6 DESTINATION addr
				 * matching
				 */
				value = osi_readla(osi_core, (nveu8_t *)base +
						 EQOS_MAC_L3L4_CTR(filter_no));
				value &= ~EQOS_MAC_L3_IP6_CTRL_CLEAR;
				value |= ((EQOS_MAC_L3L4_CTR_L3DAM0 |
					  (perfect_inverse_match <<
					   EQOS_MAC_L3L4_CTR_L3DAI_SHIFT)) &
					  ((EQOS_MAC_L3L4_CTR_L3DAM0 |
					  EQOS_MAC_L3L4_CTR_L3DAIM0)));
				value |= eqos_set_dcs(osi_core, value,
						      dma_routing_enable,
						      dma_chan);
				osi_writela(osi_core, value, (nveu8_t *)base +
					    EQOS_MAC_L3L4_CTR(filter_no));
			}
		} else {
			/* Disable L3 filters for IPv6 SOURCE/DESTINATION addr
			 * matching
			 */
			value = osi_readla(osi_core, (nveu8_t *)base +
					  EQOS_MAC_L3L4_CTR(filter_no));
			value &= ~(EQOS_MAC_L3_IP6_CTRL_CLEAR |
				   EQOS_MAC_L3L4_CTR_L3PEN0);
			osi_writela(osi_core, value, (nveu8_t *)base +
				   EQOS_MAC_L3L4_CTR(filter_no));
		}
	} else {
		if (src_dst_addr_match == OSI_SOURCE_MATCH) {
			if (enb_dis == OSI_ENABLE) {
				/* Enable L3 filters for IPv4 SOURCE addr
				 * matching
				 */
				value = osi_readla(osi_core, (nveu8_t *)base +
						 EQOS_MAC_L3L4_CTR(filter_no));
				value &= ~EQOS_MAC_L3_IP4_SA_CTRL_CLEAR;
				value |= ((EQOS_MAC_L3L4_CTR_L3SAM0 |
					  (perfect_inverse_match <<
					   EQOS_MAC_L3L4_CTR_L3SAI_SHIFT)) &
					  ((EQOS_MAC_L3L4_CTR_L3SAM0 |
					  EQOS_MAC_L3L4_CTR_L3SAIM0)));
				value |= eqos_set_dcs(osi_core, value,
						      dma_routing_enable,
						      dma_chan);
				osi_writela(osi_core, value, (nveu8_t *)base +
					    EQOS_MAC_L3L4_CTR(filter_no));
			} else {
				/* Disable L3 filters for IPv4 SOURCE addr
				 * matching
				 */
				value = osi_readla(osi_core, (nveu8_t *)base +
						  EQOS_MAC_L3L4_CTR(filter_no));
				value &= ~EQOS_MAC_L3_IP4_SA_CTRL_CLEAR;
				osi_writela(osi_core, value, (nveu8_t *)base +
					    EQOS_MAC_L3L4_CTR(filter_no));
			}
		} else {
			if (enb_dis == OSI_ENABLE) {
				/* Enable L3 filters for IPv4 DESTINATION addr
				 * matching
				 */
				value = osi_readla(osi_core, (nveu8_t *)base +
						 EQOS_MAC_L3L4_CTR(filter_no));
				value &= ~EQOS_MAC_L3_IP4_DA_CTRL_CLEAR;
				value |= ((EQOS_MAC_L3L4_CTR_L3DAM0 |
					  (perfect_inverse_match <<
					   EQOS_MAC_L3L4_CTR_L3DAI_SHIFT)) &
					  ((EQOS_MAC_L3L4_CTR_L3DAM0 |
					  EQOS_MAC_L3L4_CTR_L3DAIM0)));
				value |= eqos_set_dcs(osi_core, value,
						      dma_routing_enable,
						      dma_chan);
				osi_writela(osi_core, value, (nveu8_t *)base +
					    EQOS_MAC_L3L4_CTR(filter_no));
			} else {
				/* Disable L3 filters for IPv4 DESTINATION addr
				 * matching
				 */
				value = osi_readla(osi_core, (nveu8_t *)base +
						   EQOS_MAC_L3L4_CTR(filter_no));
				value &= ~EQOS_MAC_L3_IP4_DA_CTRL_CLEAR;
				osi_writela(osi_core, value, (nveu8_t *)base +
					    EQOS_MAC_L3L4_CTR(filter_no));
			}
		}
	}

	/* Set bit corresponding to filter index if value is non-zero */
	eqos_helper_l3l4_bitmask(&osi_core->l3l4_filter_bitmask,
				 filter_no, value);

	return 0;
}

/**
 * @brief eqos_config_l4_filters - Config L4 filters.
 *
 * @note
 * Algorithm:
 *  - This sequence is used to configure L4(TCP/UDP) filters for
 *    SA and DA Port Number matching
 *
 * @param[in, out] osi_core: OSI core private data structure.
 * @param[in] filter_no: filter index
 * @param[in] enb_dis: 1 - enable, otherwise - disable L4 filter
 * @param[in] tcp_udp_match: 1 - udp, 0 - tcp
 * @param[in] src_dst_port_match: 0 - source port, otherwise - dest port
 * @param[in] perfect_inverse_match: normal match(0) or inverse map(1)
 * @param[in] dma_routing_enable: filter based dma routing enable(1)
 * @param[in] dma_chan: dma channel for routing based on filter
 *
 * @pre
 *  - MAC should be initialized and started. see osi_start_mac()
 *  - osi_core->osd should be populated.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_config_l4_filters(
				  struct osi_core_priv_data *const osi_core,
				  const nveu32_t filter_no,
				  const nveu32_t enb_dis,
				  const nveu32_t tcp_udp_match,
				  const nveu32_t src_dst_port_match,
				  const nveu32_t perfect_inverse_match,
				  const nveu32_t dma_routing_enable,
				  const nveu32_t dma_chan)
{
	void *base = osi_core->base;
	nveu32_t value = 0U;

	if (filter_no > (EQOS_MAX_L3_L4_FILTER - 0x1U)) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_OUTOFBOUND,
			     "invalid filter index for L3/L4 filter\n",
			     (nveul64_t)filter_no);
		return -1;
	}

	if ((dma_routing_enable == OSI_ENABLE) &&
	    (dma_chan > (OSI_EQOS_MAX_NUM_CHANS - 1U))) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_OUTOFBOUND,
			     "Wrong DMA channel\n", (nveu32_t)dma_chan);
		return -1;
	}

	value = osi_readla(osi_core, (nveu8_t *)base +
			   EQOS_MAC_L3L4_CTR(filter_no));
	value &= ~EQOS_MAC_L3L4_CTR_L4PEN0;
	value |= ((tcp_udp_match << 16) & EQOS_MAC_L3L4_CTR_L4PEN0);
	osi_writela(osi_core, value, (nveu8_t *)base +
		    EQOS_MAC_L3L4_CTR(filter_no));

	if (src_dst_port_match == OSI_SOURCE_MATCH) {
		if (enb_dis == OSI_ENABLE) {
			/* Enable L4 filters for SOURCE Port No matching */
			value = osi_readla(osi_core, (nveu8_t *)base +
					   EQOS_MAC_L3L4_CTR(filter_no));
			value &= ~EQOS_MAC_L4_SP_CTRL_CLEAR;
			value |= ((EQOS_MAC_L3L4_CTR_L4SPM0 |
				  (perfect_inverse_match <<
				   EQOS_MAC_L3L4_CTR_L4SPI_SHIFT)) &
				  (EQOS_MAC_L3L4_CTR_L4SPM0 |
				  EQOS_MAC_L3L4_CTR_L4SPIM0));
			value |= eqos_set_dcs(osi_core, value,
					      dma_routing_enable,
					      dma_chan);
			osi_writela(osi_core, value, (nveu8_t *)base +
				    EQOS_MAC_L3L4_CTR(filter_no));
		} else {
			/* Disable L4 filters for SOURCE Port No matching  */
			value = osi_readla(osi_core, (nveu8_t *)base +
					   EQOS_MAC_L3L4_CTR(filter_no));
			value &= ~EQOS_MAC_L4_SP_CTRL_CLEAR;
			osi_writela(osi_core, value, (nveu8_t *)base +
				    EQOS_MAC_L3L4_CTR(filter_no));
		}
	} else {
		if (enb_dis == OSI_ENABLE) {
			/* Enable L4 filters for DESTINATION port No
			 * matching
			 */
			value = osi_readla(osi_core, (nveu8_t *)base +
					   EQOS_MAC_L3L4_CTR(filter_no));
			value &= ~EQOS_MAC_L4_DP_CTRL_CLEAR;
			value |= ((EQOS_MAC_L3L4_CTR_L4DPM0 |
				  (perfect_inverse_match <<
				   EQOS_MAC_L3L4_CTR_L4DPI_SHIFT)) &
				  (EQOS_MAC_L3L4_CTR_L4DPM0 |
				  EQOS_MAC_L3L4_CTR_L4DPIM0));
			value |= eqos_set_dcs(osi_core, value,
					      dma_routing_enable,
					      dma_chan);
			osi_writela(osi_core, value, (nveu8_t *)base +
				    EQOS_MAC_L3L4_CTR(filter_no));
		} else {
			/* Disable L4 filters for DESTINATION port No
			 * matching
			 */
			value = osi_readla(osi_core, (nveu8_t *)base +
					   EQOS_MAC_L3L4_CTR(filter_no));
			value &= ~EQOS_MAC_L4_DP_CTRL_CLEAR;
			osi_writela(osi_core, value, (nveu8_t *)base +
				    EQOS_MAC_L3L4_CTR(filter_no));
		}
	}
	/* Set bit corresponding to filter index if value is non-zero */
	eqos_helper_l3l4_bitmask(&osi_core->l3l4_filter_bitmask,
				 filter_no, value);

	return 0;
}

/**
 * @brief eqos_poll_for_tsinit_complete - Poll for time stamp init complete
 *
 * @note
 * Algorithm:
 *  - Read TSINIT value from MAC TCR register until it is
 *    equal to zero.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in, out] mac_tcr: Address to store time stamp control register read
 *  value
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static inline nve32_t eqos_poll_for_tsinit_complete(
				struct osi_core_priv_data *const osi_core,
				nveu32_t *mac_tcr)
{
	nveu32_t retry = 1000;
	nveu32_t count;
	nve32_t cond = 1;

	/* Wait for previous(if any) Initialize Timestamp value
	 * update to complete
	 */
	count = 0;
	while (cond == 1) {
		if (count > retry) {
			OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_HW_FAIL,
				     "poll_for_tsinit: timeout\n", 0ULL);
			return -1;
		}
		/* Read and Check TSINIT in MAC_Timestamp_Control register */
		*mac_tcr = osi_readla(osi_core, (nveu8_t *)osi_core->base +
				      EQOS_MAC_TCR);
		if ((*mac_tcr & EQOS_MAC_TCR_TSINIT) == 0U) {
			cond = 0;
		}
		count++;
		osi_core->osd_ops.udelay(1000U);
	}

	return 0;
}

/**
 * @brief eqos_set_systime_to_mac - Set system time
 *
 * @note
 * Algorithm:
 *  - Updates system time (seconds and nano seconds)
 *    in hardware registers
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] sec: Seconds to be configured
 * @param[in] nsec: Nano Seconds to be configured
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_set_systime_to_mac(
				      struct osi_core_priv_data *const osi_core,
				      const nveu32_t sec,
				      const nveu32_t nsec)
{
	void *addr = osi_core->base;
	nveu32_t mac_tcr;
	nve32_t ret;

	ret = eqos_poll_for_tsinit_complete(osi_core, &mac_tcr);
	if (ret == -1) {
		return -1;
	}

	/* write seconds value to MAC_System_Time_Seconds_Update register */
	osi_writela(osi_core, sec, (nveu8_t *)addr + EQOS_MAC_STSUR);

	/* write nano seconds value to MAC_System_Time_Nanoseconds_Update
	 * register
	 */
	osi_writela(osi_core, nsec, (nveu8_t *)addr + EQOS_MAC_STNSUR);

	/* issue command to update the configured secs and nsecs values */
	mac_tcr |= EQOS_MAC_TCR_TSINIT;
	eqos_core_safety_writel(osi_core, mac_tcr,
				(nveu8_t *)addr + EQOS_MAC_TCR,
				EQOS_MAC_TCR_IDX);

	ret = eqos_poll_for_tsinit_complete(osi_core, &mac_tcr);
	if (ret == -1) {
		return -1;
	}

	return 0;
}

/**
 * @brief eqos_poll_for_addend_complete - Poll for addend value write complete
 *
 * @note
 * Algorithm:
 *  - Read TSADDREG value from MAC TCR register until it is
 *    equal to zero.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in, out] mac_tcr: Address to store time stamp control register read
 *  value
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static inline nve32_t eqos_poll_for_addend_complete(
				struct osi_core_priv_data *const osi_core,
				nveu32_t *mac_tcr)
{
	nveu32_t retry = 1000;
	nveu32_t count;
	nve32_t cond = 1;

	/* Wait for previous(if any) addend value update to complete */
	/* Poll */
	count = 0;
	while (cond == 1) {
		if (count > retry) {
			OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_HW_FAIL,
				     "poll_for_addend: timeout\n", 0ULL);
			return -1;
		}
		/* Read and Check TSADDREG in MAC_Timestamp_Control register */
		*mac_tcr = osi_readla(osi_core,
				      (nveu8_t *)osi_core->base + EQOS_MAC_TCR);
		if ((*mac_tcr & EQOS_MAC_TCR_TSADDREG) == 0U) {
			cond = 0;
		}
		count++;
		osi_core->osd_ops.udelay(1000U);
	}

	return 0;
}

/**
 * @brief eqos_config_addend - Configure addend
 *
 * @note
 * Algorithm:
 *  - Updates the Addend value in HW register
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] addend: Addend value to be configured
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_config_addend(struct osi_core_priv_data *const osi_core,
				  const nveu32_t addend)
{
	nveu32_t mac_tcr;
	nve32_t ret;

	ret = eqos_poll_for_addend_complete(osi_core, &mac_tcr);
	if (ret == -1) {
		return -1;
	}

	/* write addend value to MAC_Timestamp_Addend register */
	eqos_core_safety_writel(osi_core, addend,
				(nveu8_t *)osi_core->base + EQOS_MAC_TAR,
				EQOS_MAC_TAR_IDX);

	/* issue command to update the configured addend value */
	mac_tcr |= EQOS_MAC_TCR_TSADDREG;
	eqos_core_safety_writel(osi_core, mac_tcr,
				(nveu8_t *)osi_core->base + EQOS_MAC_TCR,
				EQOS_MAC_TCR_IDX);

	ret = eqos_poll_for_addend_complete(osi_core, &mac_tcr);
	if (ret == -1) {
		return -1;
	}

	return 0;
}

/**
 * @brief eqos_poll_for_update_ts_complete - Poll for update time stamp
 *
 * @note
 * Algorithm:
 *  - Read time stamp update value from TCR register until it is
 *    equal to zero.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in, out] mac_tcr: Address to store time stamp control register read
 *  value
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static inline nve32_t eqos_poll_for_update_ts_complete(
				struct osi_core_priv_data *const osi_core,
				nveu32_t *mac_tcr)
{
	nveu32_t retry = 1000;
	nveu32_t count;
	nve32_t cond = 1;

	/* Wait for previous(if any) time stamp  value update to complete */
	count = 0;
	while (cond == 1) {
		if (count > retry) {
			OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_HW_FAIL,
				     "poll_for_update_ts: timeout\n", 0ULL);
			return -1;
		}
		/* Read and Check TSUPDT in MAC_Timestamp_Control register */
		*mac_tcr = osi_readla(osi_core, (nveu8_t *)osi_core->base +
				      EQOS_MAC_TCR);
		if ((*mac_tcr & EQOS_MAC_TCR_TSUPDT) == 0U) {
			cond = 0;
		}
		count++;
		osi_core->osd_ops.udelay(1000U);
	}

	return 0;

}

/**
 * @brief eqos_adjust_mactime - Adjust MAC time with system time
 *
 * @note
 * Algorithm:
 *  - Update MAC time with system time
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] sec: Seconds to be configured
 * @param[in] nsec: Nano seconds to be configured
 * @param[in] add_sub: To decide on add/sub with system time
 * @param[in] one_nsec_accuracy: One nano second accuracy
 *
 * @pre
 *  - MAC should be initialized and started. see osi_start_mac()
 *  - osi_core->ptp_config.one_nsec_accuracy need to be set to 1
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_adjust_mactime(struct osi_core_priv_data *const osi_core,
				   const nveu32_t sec, const nveu32_t nsec,
				   const nveu32_t add_sub,
				   const nveu32_t one_nsec_accuracy)
{
	void *addr = osi_core->base;
	nveu32_t mac_tcr;
	nveu32_t value = 0;
	nveul64_t temp = 0;
	nveu32_t sec1 = sec;
	nveu32_t nsec1 = nsec;
	nve32_t ret;

	ret = eqos_poll_for_update_ts_complete(osi_core, &mac_tcr);
	if (ret == -1) {
		return -1;
	}

	if (add_sub != 0U) {
		/* If the new sec value needs to be subtracted with
		 * the system time, then MAC_STSUR reg should be
		 * programmed with (2^32 – <new_sec_value>)
		 */
		temp = (TWO_POWER_32 - sec1);
		if (temp < UINT_MAX) {
			sec1 = (nveu32_t)temp;
		} else {
			/* do nothing here */
		}

		/* If the new nsec value need to be subtracted with
		 * the system time, then MAC_STNSUR.TSSS field should be
		 * programmed with, (10^9 - <new_nsec_value>) if
		 * MAC_TCR.TSCTRLSSR is set or
		 * (2^32 - <new_nsec_value> if MAC_TCR.TSCTRLSSR is reset)
		 */
		if (one_nsec_accuracy == OSI_ENABLE) {
			if (nsec1 < UINT_MAX) {
				nsec1 = (TEN_POWER_9 - nsec1);
			}
		} else {
			if (nsec1 < UINT_MAX) {
				nsec1 = (TWO_POWER_31 - nsec1);
			}
		}
	}

	/* write seconds value to MAC_System_Time_Seconds_Update register */
	osi_writela(osi_core, sec, (nveu8_t *)addr + EQOS_MAC_STSUR);

	/* write nano seconds value and add_sub to
	 * MAC_System_Time_Nanoseconds_Update register
	 */
	value |= nsec1;
	value |= add_sub << EQOS_MAC_STNSUR_ADDSUB_SHIFT;
	osi_writela(osi_core, value, (nveu8_t *)addr + EQOS_MAC_STNSUR);

	/* issue command to initialize system time with the value
	 * specified in MAC_STSUR and MAC_STNSUR
	 */
	mac_tcr |= EQOS_MAC_TCR_TSUPDT;
	eqos_core_safety_writel(osi_core, mac_tcr,
				(nveu8_t *)addr + EQOS_MAC_TCR,
				EQOS_MAC_TCR_IDX);

	ret = eqos_poll_for_update_ts_complete(osi_core, &mac_tcr);
	if (ret == -1) {
		return -1;
	}

	return 0;
}

/**
 * @brief eqos_config_tscr - Configure Time Stamp Register
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] ptp_filter: PTP rx filter parameters
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 */
static void eqos_config_tscr(struct osi_core_priv_data *const osi_core,
			     const nveu32_t ptp_filter)
{
	void *addr = osi_core->base;
	nveu32_t mac_tcr = 0U, i = 0U, temp = 0U;

	if (ptp_filter == OSI_DISABLE) {
		/* Disabling the MAC time stamping */
		mac_tcr = OSI_DISABLE;
		eqos_core_safety_writel(osi_core, mac_tcr,
					(nveu8_t *)addr + EQOS_MAC_TCR,
					EQOS_MAC_TCR_IDX);
		return;
	}

	mac_tcr = (OSI_MAC_TCR_TSENA	|
		   OSI_MAC_TCR_TSCFUPDT |
		   OSI_MAC_TCR_TSCTRLSSR);

	for (i = 0U; i < 32U; i++) {
		temp = ptp_filter & OSI_BIT(i);

		switch (temp) {
		case OSI_MAC_TCR_SNAPTYPSEL_1:
			mac_tcr |= OSI_MAC_TCR_SNAPTYPSEL_1;
			break;
		case OSI_MAC_TCR_SNAPTYPSEL_2:
			mac_tcr |= OSI_MAC_TCR_SNAPTYPSEL_2;
			break;
		case OSI_MAC_TCR_TSIPV4ENA:
			mac_tcr |= OSI_MAC_TCR_TSIPV4ENA;
			break;
		case OSI_MAC_TCR_TSIPV6ENA:
			mac_tcr |= OSI_MAC_TCR_TSIPV6ENA;
			break;
		case OSI_MAC_TCR_TSEVENTENA:
			mac_tcr |= OSI_MAC_TCR_TSEVENTENA;
			break;
		case OSI_MAC_TCR_TSMASTERENA:
			mac_tcr |= OSI_MAC_TCR_TSMASTERENA;
			break;
		case OSI_MAC_TCR_TSVER2ENA:
			mac_tcr |= OSI_MAC_TCR_TSVER2ENA;
			break;
		case OSI_MAC_TCR_TSIPENA:
			mac_tcr |= OSI_MAC_TCR_TSIPENA;
			break;
		case OSI_MAC_TCR_AV8021ASMEN:
			mac_tcr |= OSI_MAC_TCR_AV8021ASMEN;
			break;
		case OSI_MAC_TCR_TSENALL:
			mac_tcr |= OSI_MAC_TCR_TSENALL;
			break;
		default:
			/* To avoid MISRA violation */
			mac_tcr |= mac_tcr;
			break;
		}
	}

	eqos_core_safety_writel(osi_core, mac_tcr,
				(nveu8_t *)addr + EQOS_MAC_TCR,
				EQOS_MAC_TCR_IDX);
}

/**
 * @brief eqos_config_ssir - Configure SSIR
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 */
static void eqos_config_ssir(struct osi_core_priv_data *const osi_core)
{
	nveul64_t val;
	nveu32_t mac_tcr;
	void *addr = osi_core->base;
	nveu32_t ptp_clock = osi_core->ptp_config.ptp_clock;

	mac_tcr = osi_readla(osi_core, (nveu8_t *)addr + EQOS_MAC_TCR);


	if ((mac_tcr & EQOS_MAC_TCR_TSCFUPDT) == EQOS_MAC_TCR_TSCFUPDT) {
		if (osi_core->mac_ver <= OSI_EQOS_MAC_4_10) {
			val = OSI_PTP_SSINC_16;
		} else {
			val = OSI_PTP_SSINC_4;
		}
	} else {
		/* convert the PTP required clock frequency to nano second for
		 * COARSE correction.
		 * Formula: ((1/ptp_clock) * 1000000000)
		 */
		val = ((1U * OSI_NSEC_PER_SEC) / ptp_clock);
	}

	/* 0.465ns accurecy */
	if ((mac_tcr & EQOS_MAC_TCR_TSCTRLSSR) == 0U) {
		if (val < UINT_MAX) {
			val = (val * 1000U) / 465U;
		}
	}

	val |= val << EQOS_MAC_SSIR_SSINC_SHIFT;
	/* update Sub-second Increment Value */
	if (val < UINT_MAX) {
		eqos_core_safety_writel(osi_core, (nveu32_t)val,
					(nveu8_t *)addr + EQOS_MAC_SSIR,
					EQOS_MAC_SSIR_IDX);
	}
}

/**
 * @brief eqos_core_deinit - EQOS MAC core deinitialization
 *
 * @note
 * Algorithm:
 *  - This function will take care of deinitializing MAC
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @pre Required clks and resets has to be enabled
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: No
 * - De-initialization: Yes
 */
static void eqos_core_deinit(struct osi_core_priv_data *const osi_core)
{
	/* Stop the MAC by disabling both MAC Tx and Rx */
	eqos_stop_mac(osi_core);
}

/**
 * @brief poll_for_mii_idle Query the status of an ongoing DMA transfer
 *
 * @param[in] osi_core: OSI Core private data structure.
 *
 * @pre MAC needs to be out of reset and proper clock configured.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on Success
 * @retval -1 on Failure
 */
static inline nve32_t poll_for_mii_idle(struct osi_core_priv_data *osi_core)
{
	/* half sec timeout */
	nveu32_t retry = 50000;
	nveu32_t mac_gmiiar;
	nveu32_t count;
	nve32_t cond = 1;

	count = 0;
	while (cond == 1) {
		if (count > retry) {
			OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_HW_FAIL,
				     "MII operation timed out\n", 0ULL);
			return -1;
		}
		count++;

		mac_gmiiar = osi_readla(osi_core, (nveu8_t *)osi_core->base +
				        EQOS_MAC_MDIO_ADDRESS);
		if ((mac_gmiiar & EQOS_MAC_GMII_BUSY) == 0U) {
			/* exit loop */
			cond = 0;
		} else {
			/* wait on GMII Busy set */
			osi_core->osd_ops.udelay(10U);
		}
	}

	return 0;
}

/**
 * @brief eqos_write_phy_reg - Write to a PHY register through MAC over MDIO bus
 *
 * @note
 * Algorithm:
 *  - Before proceeding for reading for PHY register check whether any MII
 *    operation going on MDIO bus by polling MAC_GMII_BUSY bit.
 *  - Program data into MAC MDIO data register.
 *  - Populate required parameters like phy address, phy register etc,,
 *    in MAC MDIO Address register. write and GMII busy bits needs to be set
 *    in this operation.
 *  - Write into MAC MDIO address register poll for GMII busy for MDIO
 *  operation to complete.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] phyaddr: PHY address (PHY ID) associated with PHY
 * @param[in] phyreg: Register which needs to be write to PHY.
 * @param[in] phydata: Data to write to a PHY register.
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_write_phy_reg(struct osi_core_priv_data *const osi_core,
				  const nveu32_t phyaddr,
				  const nveu32_t phyreg,
				  const nveu16_t phydata)
{
	nveu32_t mac_gmiiar;
	nveu32_t mac_gmiidr;
	nve32_t ret = 0;

	/* Wait for any previous MII read/write operation to complete */
	ret = poll_for_mii_idle(osi_core);
	if (ret < 0) {
		/* poll_for_mii_idle fail */
		return ret;
	}

	mac_gmiidr = osi_readla(osi_core, (nveu8_t *)osi_core->base +
			        EQOS_MAC_MDIO_DATA);
	mac_gmiidr = ((mac_gmiidr & EQOS_MAC_GMIIDR_GD_WR_MASK) |
		      ((phydata) & EQOS_MAC_GMIIDR_GD_MASK));

	osi_writela(osi_core, mac_gmiidr, (nveu8_t *)osi_core->base +
		    EQOS_MAC_MDIO_DATA);

	/* initiate the MII write operation by updating desired */
	/* phy address/id (0 - 31) */
	/* phy register offset */
	/* CSR Clock Range (20 - 35MHz) */
	/* Select write operation */
	/* set busy bit */
	mac_gmiiar = osi_readla(osi_core, (nveu8_t *)osi_core->base +
			        EQOS_MAC_MDIO_ADDRESS);
	mac_gmiiar = (mac_gmiiar & (EQOS_MDIO_PHY_REG_SKAP |
		      EQOS_MDIO_PHY_REG_C45E));
	mac_gmiiar = (mac_gmiiar | ((phyaddr) << EQOS_MDIO_PHY_ADDR_SHIFT) |
		      ((phyreg) << EQOS_MDIO_PHY_REG_SHIFT) |
		      ((osi_core->mdc_cr) << EQOS_MDIO_PHY_REG_CR_SHIF) |
		      (EQOS_MDIO_PHY_REG_WRITE) | EQOS_MAC_GMII_BUSY);

	osi_writela(osi_core, mac_gmiiar, (nveu8_t *)osi_core->base +
		    EQOS_MAC_MDIO_ADDRESS);

	/* wait for MII write operation to complete */
	ret = poll_for_mii_idle(osi_core);
	if (ret < 0) {
		/* poll_for_mii_idle fail */
		return ret;
	}

	return ret;
}

/**
 * @brief eqos_read_phy_reg - Read from a PHY register through MAC over MDIO bus
 *
 * @note
 * Algorithm:
 *  -  Before proceeding for reading for PHY register check whether any MII
 *    operation going on MDIO bus by polling MAC_GMII_BUSY bit.
 *  - Populate required parameters like phy address, phy register etc,,
 *    in program it in MAC MDIO Address register. Read and GMII busy bits
 *    needs to be set in this operation.
 *  - Write into MAC MDIO address register poll for GMII busy for MDIO
 *    operation to complete. After this data will be available at MAC MDIO
 *    data register.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] phyaddr: PHY address (PHY ID) associated with PHY
 * @param[in] phyreg: Register which needs to be read from PHY.
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval data from PHY register on success
 * @retval -1 on failure
 */
static nve32_t eqos_read_phy_reg(struct osi_core_priv_data *const osi_core,
				 const nveu32_t phyaddr,
				 const nveu32_t phyreg)
{
	nveu32_t mac_gmiiar;
	nveu32_t mac_gmiidr;
	nveu32_t data;
	nve32_t ret = 0;

	/* wait for any previous MII read/write operation to complete */
	ret = poll_for_mii_idle(osi_core);
	if (ret < 0) {
		/* poll_for_mii_idle fail */
		return ret;
	}

	mac_gmiiar = osi_readla(osi_core, (nveu8_t *)osi_core->base +
			        EQOS_MAC_MDIO_ADDRESS);
	/* initiate the MII read operation by updating desired */
	/* phy address/id (0 - 31) */
	/* phy register offset */
	/* CSR Clock Range (20 - 35MHz) */
	/* Select read operation */
	/* set busy bit */
	mac_gmiiar = (mac_gmiiar & (EQOS_MDIO_PHY_REG_SKAP |
		      EQOS_MDIO_PHY_REG_C45E));
	mac_gmiiar = mac_gmiiar | ((phyaddr) << EQOS_MDIO_PHY_ADDR_SHIFT) |
		     ((phyreg) << EQOS_MDIO_PHY_REG_SHIFT) |
		     ((osi_core->mdc_cr) << EQOS_MDIO_PHY_REG_CR_SHIF) |
		     (EQOS_MDIO_PHY_REG_GOC_READ) | EQOS_MAC_GMII_BUSY;
	osi_writela(osi_core, mac_gmiiar, (nveu8_t *)osi_core->base +
		    EQOS_MAC_MDIO_ADDRESS);

	/* wait for MII write operation to complete */
	ret = poll_for_mii_idle(osi_core);
	if (ret < 0) {
		/* poll_for_mii_idle fail */
		return ret;
	}

	mac_gmiidr = osi_readla(osi_core, (nveu8_t *)osi_core->base +
			        EQOS_MAC_MDIO_DATA);
	data = (mac_gmiidr & EQOS_MAC_GMIIDR_GD_MASK);

	return (nve32_t)data;
}

/**
 * @brief eqos_read_reg - Read a reg
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] reg: Register address.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: Yes
 * @retval data from register on success
 */
static nveu32_t eqos_read_reg(struct osi_core_priv_data *const osi_core,
                              const nve32_t reg) {
	return osi_readla(osi_core, (nveu8_t *)osi_core->base + reg);
}

/**
 * @brief eqos_write_reg - Write a reg
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] val:  Value to be written.
 * @param[in] reg: Register address.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: Yes
 * @retval 0
 */
static nveu32_t eqos_write_reg(struct osi_core_priv_data *const osi_core,
                               const nveu32_t val,
                               const nve32_t reg) {
	osi_writela(osi_core, val, (nveu8_t *)osi_core->base + reg);
	return 0;
}

#ifndef OSI_STRIPPED_LIB
/**
 * @brief eqos_disable_tx_lpi - Helper function to disable Tx LPI.
 *
 * @note
 * Algorithm:
 *  - Clear the bits to enable Tx LPI, Tx LPI automate, LPI Tx Timer and
 *    PHY Link status in the LPI control/status register
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @pre MAC has to be out of reset, and clocks supplied.
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 */
static inline void eqos_disable_tx_lpi(
				struct osi_core_priv_data *const osi_core)
{
	void *addr = osi_core->base;
	nveu32_t lpi_csr = 0;

	/* Disable LPI control bits */
	lpi_csr = osi_readla(osi_core,
			     (nveu8_t *)addr + EQOS_MAC_LPI_CSR);
	lpi_csr &= ~(EQOS_MAC_LPI_CSR_LPITE | EQOS_MAC_LPI_CSR_LPITXA |
		     EQOS_MAC_LPI_CSR_PLS | EQOS_MAC_LPI_CSR_LPIEN);
	osi_writela(osi_core, lpi_csr,
		    (nveu8_t *)addr + EQOS_MAC_LPI_CSR);
}

/**
 * @brief Read-validate HW registers for functional safety.
 *
 * @note
 * Algorithm:
 *  - Reads pre-configured list of MAC/MTL configuration registers
 *    and compares with last written value for any modifications.
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @pre
 *  - MAC has to be out of reset.
 *  - osi_hw_core_init has to be called. Internally this would initialize
 *    the safety_config (see osi_core_priv_data) based on MAC version and
 *    which specific registers needs to be validated periodically.
 *  - Invoke this call if (osi_core_priv_data->safety_config != OSI_NULL)
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_validate_core_regs(
				struct osi_core_priv_data *const osi_core)
{
	struct core_func_safety *config =
		(struct core_func_safety *)osi_core->safety_config;
	nveu32_t cur_val;
	nveu32_t i;

	osi_lock_irq_enabled(&config->core_safety_lock);
	for (i = EQOS_MAC_MCR_IDX; i < EQOS_MAX_CORE_SAFETY_REGS; i++) {
		if (config->reg_addr[i] == OSI_NULL) {
			continue;
		}
		cur_val = osi_readla(osi_core,
				     (nveu8_t *)config->reg_addr[i]);
		cur_val &= config->reg_mask[i];

		if (cur_val == config->reg_val[i]) {
			continue;
		} else {
			/* Register content differs from what was written.
			 * Return error and let safety manager (NVGaurd etc.)
			 * take care of corrective action.
			 */
			osi_unlock_irq_enabled(&config->core_safety_lock);
			OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
				     "register mismatch\n", 0ULL);
			return -1;
		}
	}
	osi_unlock_irq_enabled(&config->core_safety_lock);

	return 0;
}

/**
 * @brief eqos_config_rx_crc_check - Configure CRC Checking for Rx Packets
 *
 * @note
 * Algorithm:
 *  - When this bit is set, the MAC receiver does not check the CRC
 *    field in the received packets. When this bit is reset, the MAC receiver
 *    always checks the CRC field in the received packets.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] crc_chk: Enable or disable checking of CRC field in received pkts
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_config_rx_crc_check(
				    struct osi_core_priv_data *const osi_core,
				    const nveu32_t crc_chk)
{
	void *addr = osi_core->base;
	nveu32_t val;

	/* return on invalid argument */
	if (crc_chk != OSI_ENABLE && crc_chk != OSI_DISABLE) {
		OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			     "rx_crc: invalid input\n", 0ULL);
		return -1;
	}

	/* Read MAC Extension Register */
	val = osi_readla(osi_core, (nveu8_t *)addr + EQOS_MAC_EXTR);

	/* crc_chk: 1 is for enable and 0 is for disable */
	if (crc_chk == OSI_ENABLE) {
		/* Enable Rx packets CRC check */
		val &= ~EQOS_MAC_EXTR_DCRCC;
	} else if (crc_chk == OSI_DISABLE) {
		/* Disable Rx packets CRC check */
		val |= EQOS_MAC_EXTR_DCRCC;
	} else {
		/* Nothing here */
	}

	/* Write to MAC Extension Register */
	osi_writela(osi_core, val, (nveu8_t *)addr + EQOS_MAC_EXTR);

	return 0;
}

/**
 * @brief eqos_config_tx_status - Configure MAC to forward the tx pkt status
 *
 * @note
 * Algorithm:
 *  - When DTXSTS bit is reset, the Tx packet status received
 *    from the MAC is forwarded to the application.
 *    When DTXSTS bit is set, the Tx packet status received from the MAC
 *    are dropped in MTL.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] tx_status: Enable or Disable the forwarding of tx pkt status
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_config_tx_status(struct osi_core_priv_data *const osi_core,
				     const nveu32_t tx_status)
{
	void *addr = osi_core->base;
	nveu32_t val;

	/* don't allow if tx_status is other than 0 or 1 */
	if (tx_status != OSI_ENABLE && tx_status != OSI_DISABLE) {
		OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			     "tx_status: invalid input\n", 0ULL);
		return -1;
	}

	/* Read MTL Operation Mode Register */
	val = osi_readla(osi_core, (nveu8_t *)addr + EQOS_MTL_OP_MODE);

	if (tx_status == OSI_ENABLE) {
		/* When DTXSTS bit is reset, the Tx packet status received
		 * from the MAC are forwarded to the application.
		 */
		val &= ~EQOS_MTL_OP_MODE_DTXSTS;
	} else if (tx_status == OSI_DISABLE) {
		/* When DTXSTS bit is set, the Tx packet status received from
		 * the MAC are dropped in the MTL
		 */
		val |= EQOS_MTL_OP_MODE_DTXSTS;
	} else {
		/* Nothing here */
	}

	/* Write to DTXSTS bit of MTL Operation Mode Register to enable or
	 * disable the Tx packet status
	 */
	osi_writela(osi_core, val, (nveu8_t *)addr + EQOS_MTL_OP_MODE);

	return 0;
}

/**
 * @brief eqos_set_avb_algorithm - Set TxQ/TC avb config
 *
 * @note
 * Algorithm:
 *  - Check if queue index is valid
 *  - Update operation mode of TxQ/TC
 *   - Set TxQ operation mode
 *   - Set Algo and Credit control
 *   - Set Send slope credit
 *   - Set Idle slope credit
 *   - Set Hi credit
 *   - Set low credit
 *  - Update register values
 *
 * @param[in] osi_core: OSI core priv data structure
 * @param[in] avb: structure having configuration for avb algorithm
 *
 * @pre
 *  - MAC should be initialized and started. see osi_start_mac()
 *  - osi_core->osd should be populated.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_set_avb_algorithm(
				struct osi_core_priv_data *const osi_core,
				const struct osi_core_avb_algorithm *const avb)
{
	nveu32_t value;
	nve32_t ret = -1;
	nveu32_t qinx;

	if (avb == OSI_NULL) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			     "avb structure is NULL\n",	0ULL);
		return ret;
	}

	/* queue index in range */
	if (avb->qindex >= OSI_EQOS_MAX_NUM_QUEUES) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			     "Invalid Queue index\n", (nveul64_t)avb->qindex);
		return ret;
	}

	/* queue oper_mode in range check*/
	if (avb->oper_mode >= OSI_MTL_QUEUE_MODEMAX) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			     "Invalid Queue mode\n", (nveul64_t)avb->qindex);
		return ret;
	}

	/* can't set AVB mode for queue 0 */
	if ((avb->qindex == 0U) && (avb->oper_mode == OSI_MTL_QUEUE_AVB)) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_OPNOTSUPP,
			     "Not allowed to set AVB for Q0\n",
			     (nveul64_t)avb->qindex);
		return ret;
	}

	qinx = avb->qindex;
	value = osi_readla(osi_core, (nveu8_t *)osi_core->base +
			   EQOS_MTL_CHX_TX_OP_MODE(qinx));
	value &= ~EQOS_MTL_TXQEN_MASK;
	/* Set TxQ/TC mode as per input struct after masking 3 bit */
	value |= (avb->oper_mode << EQOS_MTL_TXQEN_MASK_SHIFT) &
		  EQOS_MTL_TXQEN_MASK;
	eqos_core_safety_writel(osi_core, value, (nveu8_t *)osi_core->base +
				EQOS_MTL_CHX_TX_OP_MODE(qinx),
				EQOS_MTL_CH0_TX_OP_MODE_IDX + qinx);

	/* Set Algo and Credit control */
	if (avb->algo == OSI_MTL_TXQ_AVALG_CBS) {
		value = (avb->credit_control << EQOS_MTL_TXQ_ETS_CR_CC_SHIFT) &
			 EQOS_MTL_TXQ_ETS_CR_CC;
	}
	value |= (avb->algo << EQOS_MTL_TXQ_ETS_CR_AVALG_SHIFT) &
		  EQOS_MTL_TXQ_ETS_CR_AVALG;
	osi_writela(osi_core, value, (nveu8_t *)osi_core->base +
		    EQOS_MTL_TXQ_ETS_CR(qinx));

	if (avb->algo == OSI_MTL_TXQ_AVALG_CBS) {
		/* Set Send slope credit */
		value = avb->send_slope & EQOS_MTL_TXQ_ETS_SSCR_SSC_MASK;
		osi_writela(osi_core, value, (nveu8_t *)osi_core->base +
			    EQOS_MTL_TXQ_ETS_SSCR(qinx));

		/* Set Idle slope credit*/
		value = osi_readla(osi_core, (nveu8_t *)osi_core->base +
				   EQOS_MTL_TXQ_QW(qinx));
		value &= ~EQOS_MTL_TXQ_ETS_QW_ISCQW_MASK;
		value |= avb->idle_slope & EQOS_MTL_TXQ_ETS_QW_ISCQW_MASK;
		eqos_core_safety_writel(osi_core, value,
					(nveu8_t *)osi_core->base +
					EQOS_MTL_TXQ_QW(qinx),
					EQOS_MTL_TXQ0_QW_IDX + qinx);

		/* Set Hi credit */
		value = avb->hi_credit & EQOS_MTL_TXQ_ETS_HCR_HC_MASK;
		osi_writela(osi_core, value, (nveu8_t *)osi_core->base +
			    EQOS_MTL_TXQ_ETS_HCR(qinx));

		/* low credit  is -ve number, osi_write need a nveu32_t
		 * take only 28:0 bits from avb->low_credit
		 */
		value = avb->low_credit & EQOS_MTL_TXQ_ETS_LCR_LC_MASK;
		osi_writela(osi_core, value, (nveu8_t *)osi_core->base +
			    EQOS_MTL_TXQ_ETS_LCR(qinx));
	}

	return 0;
}

/**
 * @brief eqos_get_avb_algorithm - Get TxQ/TC avb config
 *
 * @note
 * Algorithm:
 *  - Check if queue index is valid
 *  - read operation mode of TxQ/TC
 *   - read TxQ operation mode
 *   - read Algo and Credit control
 *   - read Send slope credit
 *   - read Idle slope credit
 *   - read Hi credit
 *   - read low credit
 *  - updated pointer
 *
 * @param[in] osi_core: OSI core priv data structure
 * @param[out] avb: structure pointer having configuration for avb algorithm
 *
 * @pre
 *  - MAC should be initialized and started. see osi_start_mac()
 *  - osi_core->osd should be populated.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_get_avb_algorithm(struct osi_core_priv_data *const osi_core,
				      struct osi_core_avb_algorithm *const avb)
{
	nveu32_t value;
	nve32_t ret = -1;
	nveu32_t qinx = 0U;

	if (avb == OSI_NULL) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			     "avb structure is NULL\n", 0ULL);
		return ret;
	}

	if (avb->qindex >= OSI_EQOS_MAX_NUM_QUEUES) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			     "Invalid Queue index\n", (nveul64_t)avb->qindex);
		return ret;
	}

	qinx = avb->qindex;
	value = osi_readla(osi_core, (nveu8_t *)osi_core->base +
			   EQOS_MTL_CHX_TX_OP_MODE(qinx));

	/* Get TxQ/TC mode as per input struct after masking 3:2 bit */
	value = (value & EQOS_MTL_TXQEN_MASK) >> EQOS_MTL_TXQEN_MASK_SHIFT;
	avb->oper_mode = value;

	/* Get Algo and Credit control */
	value = osi_readla(osi_core, (nveu8_t *)osi_core->base +
			   EQOS_MTL_TXQ_ETS_CR(qinx));
	avb->credit_control = (value & EQOS_MTL_TXQ_ETS_CR_CC) >>
		   EQOS_MTL_TXQ_ETS_CR_CC_SHIFT;
	avb->algo = (value & EQOS_MTL_TXQ_ETS_CR_AVALG) >>
		     EQOS_MTL_TXQ_ETS_CR_AVALG_SHIFT;

	/* Get Send slope credit */
	value = osi_readla(osi_core, (nveu8_t *)osi_core->base +
			   EQOS_MTL_TXQ_ETS_SSCR(qinx));
	avb->send_slope = value & EQOS_MTL_TXQ_ETS_SSCR_SSC_MASK;

	/* Get Idle slope credit*/
	value = osi_readla(osi_core, (nveu8_t *)osi_core->base +
			   EQOS_MTL_TXQ_QW(qinx));
	avb->idle_slope = value & EQOS_MTL_TXQ_ETS_QW_ISCQW_MASK;

	/* Get Hi credit */
	value = osi_readla(osi_core, (nveu8_t *)osi_core->base +
			   EQOS_MTL_TXQ_ETS_HCR(qinx));
	avb->hi_credit = value & EQOS_MTL_TXQ_ETS_HCR_HC_MASK;

	/* Get Low credit for which bit 31:29 are unknown
	 * return 28:0 valid bits to application
	 */
	value = osi_readla(osi_core, (nveu8_t *)osi_core->base +
			   EQOS_MTL_TXQ_ETS_LCR(qinx));
	avb->low_credit = value & EQOS_MTL_TXQ_ETS_LCR_LC_MASK;

	return 0;
}

/**
 * @brief eqos_config_arp_offload - Enable/Disable ARP offload
 *
 * @note
 * Algorithm:
 *  - Read the MAC configuration register
 *  - If ARP offload is to be enabled, program the IP address in
 *    ARPPA register
 *  - Enable/disable the ARPEN bit in MCR and write back to the MCR.
 *
 * @param[in] osi_core: OSI core priv data structure
 * @param[in] enable: Flag variable to enable/disable ARP offload
 * @param[in] ip_addr: IP address of device to be programmed in HW.
 *        HW will use this IP address to respond to ARP requests.
 *
 * @pre
 *  - MAC should be initialized and started. see osi_start_mac()
 *  - Valid 4 byte IP address as argument ip_addr
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_config_arp_offload(
				struct osi_core_priv_data *const osi_core,
				const nveu32_t enable,
				const nveu8_t *ip_addr)
{
	void *addr = osi_core->base;
	nve32_t mac_ver = osi_core->mac_ver;
	nve32_t mac_mcr;
	nve32_t val;

	if (enable != OSI_ENABLE && enable != OSI_DISABLE) {
		OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			     "arp_offload: invalid input\n", 0ULL);
		return -1;
	}

	mac_mcr = osi_readla(osi_core, (nveu8_t *)addr + EQOS_MAC_MCR);

	if (enable == OSI_ENABLE) {
		val = (((nveu32_t)ip_addr[0]) << 24) |
		      (((nveu32_t)ip_addr[1]) << 16) |
		      (((nveu32_t)ip_addr[2]) << 8) |
		      (((nveu32_t)ip_addr[3]));

		if (mac_ver == OSI_EQOS_MAC_4_10) {
			osi_writela(osi_core, val, (nveu8_t *)addr +
				    EQOS_4_10_MAC_ARPPA);
		} else if (mac_ver == OSI_EQOS_MAC_5_00) {
			osi_writela(osi_core, val, (nveu8_t *)addr +
				    EQOS_5_00_MAC_ARPPA);
		} else {
			/* Unsupported MAC ver */
			OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
				     "arp_offload: invalid HW\n", 0ULL);
			return -1;
		}

		mac_mcr |= EQOS_MCR_ARPEN;
	} else {
		mac_mcr &= ~EQOS_MCR_ARPEN;
	}

	eqos_core_safety_writel(osi_core, mac_mcr,
				(nveu8_t *)addr + EQOS_MAC_MCR,
				EQOS_MAC_MCR_IDX);

	return 0;
}

/**
 * @brief eqos_config_vlan_filter_reg - config vlan filter register
 *
 * @note
 * Algorithm:
 *  - This sequence is used to enable/disable VLAN filtering and
 *    also selects VLAN filtering mode- perfect/hash
 *
 * @param[in] osi_core: OSI core priv data structure
 * @param[in] filter_enb_dis: vlan filter enable/disable
 * @param[in] perfect_hash_filtering: perfect or hash filter
 * @param[in] perfect_inverse_match: normal or inverse filter
 *
 * @pre
 *  - MAC should be initialized and started. see osi_start_mac()
 *  - osi_core->osd should be populated.
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_config_vlan_filtering(
				struct osi_core_priv_data *const osi_core,
				const nveu32_t filter_enb_dis,
				const nveu32_t perfect_hash_filtering,
				const nveu32_t perfect_inverse_match)
{
	nveu32_t value;
	void *base = osi_core->base;

	if (filter_enb_dis != OSI_ENABLE && filter_enb_dis != OSI_DISABLE) {
		OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			     "vlan_filter: invalid input\n", 0ULL);
		return -1;
	}

	if (perfect_hash_filtering != OSI_ENABLE &&
	    perfect_hash_filtering != OSI_DISABLE) {
		OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			     "vlan_filter: invalid input\n", 0ULL);
		return -1;
	}

	if (perfect_inverse_match != OSI_ENABLE &&
	    perfect_inverse_match != OSI_DISABLE) {
		OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			     "vlan_filter: invalid input\n", 0ULL);
		return -1;
	}

	value = osi_readla(osi_core, (nveu8_t *)base + EQOS_MAC_PFR);
	value &= ~(EQOS_MAC_PFR_VTFE);
	value |= ((filter_enb_dis << EQOS_MAC_PFR_SHIFT) & EQOS_MAC_PFR_VTFE);
	eqos_core_safety_writel(osi_core, value, (nveu8_t *)base + EQOS_MAC_PFR,
				EQOS_MAC_PFR_IDX);

	value = osi_readla(osi_core, (nveu8_t *)base + EQOS_MAC_VLAN_TR);
	value &= ~(EQOS_MAC_VLAN_TR_VTIM | EQOS_MAC_VLAN_TR_VTHM);
	value |= ((perfect_inverse_match << EQOS_MAC_VLAN_TR_VTIM_SHIFT) &
		  EQOS_MAC_VLAN_TR_VTIM);
	if (perfect_hash_filtering == OSI_HASH_FILTER_MODE) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_OPNOTSUPP,
			     "VLAN hash filter is not supported, no update of VTHM\n",
			     0ULL);
	}

	osi_writela(osi_core, value, (nveu8_t *)base + EQOS_MAC_VLAN_TR);
	return 0;
}

/**
 * @brief eqos_update_vlan_id - update VLAN ID in Tag register
 *
 * @param[in] osi_core: OSI core priv data structure
 * @param[in] vid: VLAN ID to be programmed.
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 always
 */
static inline nve32_t eqos_update_vlan_id(
				struct osi_core_priv_data *const osi_core,
				nveu32_t vid)
{
	/* Don't add VLAN ID to TR register which is eventually set TR
	 * to 0x0 and allow all tagged packets
	 */

	return 0;
}

/**
 * @brief eqos_configure_eee - Configure the EEE LPI mode
 *
 * @note
 * Algorithm:
 *  - This routine configures EEE LPI mode in the MAC.
 *  - The time (in microsecond) to wait before resuming transmission after
 *    exiting from LPI,
 *  - The time (in millisecond) to wait before LPI pattern can be
 *    transmitted after PHY link is up) are not configurable. Default values
 *    are used in this routine.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] tx_lpi_enabled: enable (1)/disable (0) flag for Tx lpi
 * @param[in] tx_lpi_timer: Tx LPI entry timer in usec. Valid upto
 *            OSI_MAX_TX_LPI_TIMER in steps of 8usec.
 *
 * @pre
 *  - Required clks and resets has to be enabled.
 *  - MAC/PHY should be initialized
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 */
static void eqos_configure_eee(struct osi_core_priv_data *const osi_core,
			       const nveu32_t tx_lpi_enabled,
			       const nveu32_t tx_lpi_timer)
{
	nveu32_t lpi_csr = 0;
	nveu32_t lpi_timer_ctrl = 0;
	nveu32_t lpi_entry_timer = 0;
	nveu32_t lpi_1US_tic_counter = OSI_LPI_1US_TIC_COUNTER_DEFAULT;
	nveu8_t *addr =  (nveu8_t *)osi_core->base;

	if (tx_lpi_enabled != OSI_DISABLE) {
		/* Configure the following timers.
		 * 1. LPI LS timer - minimum time (in milliseconds) for
		 * which the link status from PHY should be up before
		 * the LPI pattern can be transmitted to the PHY.
		 * Default 1sec
		 * 2. LPI TW timer - minimum time (in microseconds) for
		 * which MAC waits after it stops transmitting LPI
		 * pattern before resuming normal tx. Default 21us
		 * 3. LPI entry timer - Time in microseconds that MAC
		 * will wait to enter LPI mode after all tx is complete.
		 * Default 1sec
		 */
		lpi_timer_ctrl |= ((OSI_DEFAULT_LPI_LS_TIMER &
				   OSI_LPI_LS_TIMER_MASK) <<
				   OSI_LPI_LS_TIMER_SHIFT);
		lpi_timer_ctrl |= (OSI_DEFAULT_LPI_TW_TIMER &
				   OSI_LPI_TW_TIMER_MASK);
		osi_writela(osi_core, lpi_timer_ctrl,
			    addr + EQOS_MAC_LPI_TIMER_CTRL);

		lpi_entry_timer |= (tx_lpi_timer &
				    OSI_LPI_ENTRY_TIMER_MASK);
		osi_writela(osi_core, lpi_entry_timer,
			    addr + EQOS_MAC_LPI_EN_TIMER);
		/* Program the MAC_1US_Tic_Counter as per the frequency of the
		 * clock used for accessing the CSR slave
		 */
		/* fix cert error */
		if (osi_core->csr_clk_speed > 1U) {
			lpi_1US_tic_counter  = ((osi_core->csr_clk_speed - 1U) &
						 OSI_LPI_1US_TIC_COUNTER_MASK);
		}
		osi_writela(osi_core, lpi_1US_tic_counter,
			    addr + EQOS_MAC_1US_TIC_CNTR);

		/* Set LPI timer enable and LPI Tx automate, so that MAC
		 * can enter/exit Tx LPI on its own using timers above.
		 * Set LPI Enable & PHY links status (PLS) up.
		 */
		lpi_csr = osi_readla(osi_core, addr + EQOS_MAC_LPI_CSR);
		lpi_csr |= (EQOS_MAC_LPI_CSR_LPITE |
			    EQOS_MAC_LPI_CSR_LPITXA |
			    EQOS_MAC_LPI_CSR_PLS |
			    EQOS_MAC_LPI_CSR_LPIEN);
		osi_writela(osi_core, lpi_csr, addr + EQOS_MAC_LPI_CSR);
	} else {
		/* Disable LPI control bits */
		eqos_disable_tx_lpi(osi_core);
	}
}

/**
 * @brief Function to store a backup of MAC register space during SOC suspend.
 *
 * @note
 * Algorithm:
 *  - Read registers to be backed up as per struct core_backup and
 *    store the register values in memory.
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on Success
 */
static inline nve32_t eqos_save_registers(
				struct osi_core_priv_data *const osi_core)
{
	nveu32_t i;
	struct core_backup *config = &osi_core->backup_config;

	for (i = 0; i < EQOS_MAX_BAK_IDX; i++) {
		if (config->reg_addr[i] != OSI_NULL) {
			config->reg_val[i] = osi_readla(osi_core,
							config->reg_addr[i]);
		}
	}

	return 0;
}

/**
 * @brief Function to restore the backup of MAC registers during SOC resume.
 *
 * @note
 * Algorithm:
 *  - Restore the register values from the in memory backup taken using
 *    eqos_save_registers().
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 *
 * @retval 0 on Success
 */
static inline nve32_t eqos_restore_registers(
				struct osi_core_priv_data *const osi_core)
{
	nveu32_t i;
	struct core_backup *config = &osi_core->backup_config;

	for (i = 0; i < EQOS_MAX_BAK_IDX; i++) {
		if (config->reg_addr[i] != OSI_NULL) {
			osi_writela(osi_core, config->reg_val[i],
				    config->reg_addr[i]);
		}
	}

	return 0;
}

/**
 * @brief eqos_set_mdc_clk_rate - Derive MDC clock based on provided AXI_CBB clk
 *
 * @note
 * Algorithm:
 *  - MDC clock rate will be populated OSI core private data structure
 *    based on AXI_CBB clock rate.
 *
 * @param[in, out] osi_core: OSI core private data structure.
 * @param[in] csr_clk_rate: CSR (AXI CBB) clock rate.
 *
 * @pre OSD layer needs get the AXI CBB clock rate with OSD clock API
 *   (ex - clk_get_rate())
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 */
static void eqos_set_mdc_clk_rate(struct osi_core_priv_data *const osi_core,
				  const nveu64_t csr_clk_rate)
{
	nveu64_t csr_clk_speed = csr_clk_rate / 1000000UL;

	/* store csr clock speed used in programming
	 * LPI 1us tick timer register
	 */
	if (csr_clk_speed <= UINT_MAX) {
		osi_core->csr_clk_speed = (nveu32_t)csr_clk_speed;
	}
	if (csr_clk_speed > 500UL) {
		osi_core->mdc_cr = EQOS_CSR_500_800M;
	} else if (csr_clk_speed > 300UL) {
		osi_core->mdc_cr = EQOS_CSR_300_500M;
	} else if (csr_clk_speed > 250UL) {
		osi_core->mdc_cr = EQOS_CSR_250_300M;
	} else if (csr_clk_speed > 150UL) {
		osi_core->mdc_cr = EQOS_CSR_150_250M;
	} else if (csr_clk_speed > 100UL) {
		osi_core->mdc_cr = EQOS_CSR_100_150M;
	} else if (csr_clk_speed > 60UL) {
		osi_core->mdc_cr = EQOS_CSR_60_100M;
	} else if (csr_clk_speed > 35UL) {
		osi_core->mdc_cr = EQOS_CSR_35_60M;
	} else {
		/* for CSR < 35mhz */
		osi_core->mdc_cr = EQOS_CSR_20_35M;
	}
}

/**
 * @brief eqos_config_mac_loopback - Configure MAC to support loopback
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] lb_mode: Enable or Disable MAC loopback mode
 *
 * @pre MAC should be initialized and started. see osi_start_mac()
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_config_mac_loopback(
				struct osi_core_priv_data *const osi_core,
				const nveu32_t lb_mode)
{
	nveu32_t clk_ctrl_val;
	nveu32_t mcr_val;
	void *addr = osi_core->base;

	/* don't allow only if loopback mode is other than 0 or 1 */
	if (lb_mode != OSI_ENABLE && lb_mode != OSI_DISABLE) {
		return -1;
	}

	/* Read MAC Configuration Register */
	mcr_val = osi_readla(osi_core, (nveu32_t *)addr + EQOS_MAC_MCR);

	/* Read EQOS wrapper clock control 0 register */
	clk_ctrl_val = osi_readla(osi_core,
				  (nveu32_t *)addr + EQOS_CLOCK_CTRL_0);

	if (lb_mode == OSI_ENABLE) {
		/* Enable Loopback Mode */
		mcr_val |= EQOS_MAC_ENABLE_LM;
		/* Enable RX_CLK_SEL so that TX Clock is fed to RX domain */
		clk_ctrl_val |= EQOS_RX_CLK_SEL;
	} else if (lb_mode == OSI_DISABLE){
		/* Disable Loopback Mode */
		mcr_val &= ~EQOS_MAC_ENABLE_LM;
		/* Disable RX_CLK_SEL so that TX Clock is fed to RX domain */
		clk_ctrl_val &= ~EQOS_RX_CLK_SEL;
	} else {
		/* Nothing here */
	}

	/* Write to EQOS wrapper clock control 0 register */
	osi_writela(osi_core, clk_ctrl_val,
		    (nveu8_t *)addr + EQOS_CLOCK_CTRL_0);

	/* Write to MAC Configuration Register */
	eqos_core_safety_writel(osi_core, mcr_val,
				(nveu8_t *)addr + EQOS_MAC_MCR,
				EQOS_MAC_MCR_IDX);

	return 0;
}
#endif /* !OSI_STRIPPED_LIB */


/**
 * @brief eqos_get_core_safety_config - EQOS MAC safety configuration
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 */
void *eqos_get_core_safety_config(void)
{
	return &eqos_core_safety_config;
}

/**
 * @brief eqos_get_hw_core_ops - EQOS MAC get core operations
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 */
struct osi_core_ops *eqos_get_hw_core_ops(void)
{
	static struct osi_core_ops eqos_core_ops = {
		.poll_for_swr = eqos_poll_for_swr,
		.core_init = eqos_core_init,
		.core_deinit = eqos_core_deinit,
		.start_mac = eqos_start_mac,
		.stop_mac = eqos_stop_mac,
		.handle_common_intr = eqos_handle_common_intr,
		.set_mode = eqos_set_mode,
		.set_speed = eqos_set_speed,
		.pad_calibrate = eqos_pad_calibrate,
		.config_fw_err_pkts = eqos_config_fw_err_pkts,
		.config_rxcsum_offload = eqos_config_rxcsum_offload,
		.config_mac_pkt_filter_reg = eqos_config_mac_pkt_filter_reg,
		.update_mac_addr_low_high_reg =
					 eqos_update_mac_addr_low_high_reg,
		.config_l3_l4_filter_enable = eqos_config_l3_l4_filter_enable,
		.config_l3_filters = eqos_config_l3_filters,
		.update_ip4_addr = eqos_update_ip4_addr,
		.update_ip6_addr = eqos_update_ip6_addr,
		.config_l4_filters = eqos_config_l4_filters,
		.update_l4_port_no = eqos_update_l4_port_no,
		.set_systime_to_mac = eqos_set_systime_to_mac,
		.config_addend = eqos_config_addend,
		.adjust_mactime = eqos_adjust_mactime,
		.config_tscr = eqos_config_tscr,
		.config_ssir = eqos_config_ssir,
		.read_mmc = eqos_read_mmc,
		.write_phy_reg = eqos_write_phy_reg,
		.read_phy_reg = eqos_read_phy_reg,
		.read_reg = eqos_read_reg,
		.write_reg = eqos_write_reg,
#ifndef OSI_STRIPPED_LIB
		.config_tx_status = eqos_config_tx_status,
		.config_rx_crc_check = eqos_config_rx_crc_check,
		.config_flow_control = eqos_config_flow_control,
		.config_arp_offload = eqos_config_arp_offload,
		.validate_regs = eqos_validate_core_regs,
		.flush_mtl_tx_queue = eqos_flush_mtl_tx_queue,
		.set_avb_algorithm = eqos_set_avb_algorithm,
		.get_avb_algorithm = eqos_get_avb_algorithm,
		.config_vlan_filtering = eqos_config_vlan_filtering,
		.update_vlan_id = eqos_update_vlan_id,
		.reset_mmc = eqos_reset_mmc,
		.configure_eee = eqos_configure_eee,
		.save_registers = eqos_save_registers,
		.restore_registers = eqos_restore_registers,
		.set_mdc_clk_rate = eqos_set_mdc_clk_rate,
		.config_mac_loopback = eqos_config_mac_loopback,
#endif /* !OSI_STRIPPED_LIB */
	};

	return &eqos_core_ops;
}
