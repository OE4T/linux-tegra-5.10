/*
 * t19x-nvlink-endpt-isr.c:
 * This file contains interrupt handling code for the Tegra NVLINK controller.
 *
 * Copyright (c) 2017-2018, NVIDIA CORPORATION.  All rights reserved.
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

#include "t19x-nvlink-endpt.h"
#include "nvlink-hw.h"

/* Enable minion falcon Interrupts and route to Host */
void nvlink_config_minion_falcon_intr(struct tnvlink_dev *tdev)
{
	u32 reg_val = 0;

	/* Enable interrupts. Writing a '1' to any bit in IRQMSET
	 * will set the corresponding bit in IRQMASK.
	 */
	reg_val = BIT(CMINION_FALCON_IRQMSET_WDTMR) |
		BIT(CMINION_FALCON_IRQMSET_HALT) |
		BIT(CMINION_FALCON_IRQMSET_EXTERR);
	nvlw_minion_writel(tdev, CMINION_FALCON_IRQMSET, reg_val);

	/* interrrupts destination setting to HOST */
	reg_val = BIT(CMINION_FALCON_IRQDEST_HOST_WDTMR) |
		BIT(CMINION_FALCON_IRQDEST_HOST_HALT) |
		BIT(CMINION_FALCON_IRQDEST_HOST_EXTERR);
	/* Send the interrupts on the "normal" interrupt lines to host */
	reg_val &= ~(BIT(CMINION_FALCON_IRQDEST_TARGET_WDTMR) |
		BIT(CMINION_FALCON_IRQDEST_TARGET_HALT) |
		BIT(CMINION_FALCON_IRQDEST_TARGET_EXTERR));
	nvlw_minion_writel(tdev, CMINION_FALCON_IRQDEST, reg_val);
}

/* Configure NVLW interrupts */
static void nvlw_config_intr(struct tnvlink_dev *tdev)
{
	u32 reg_val = 0;

	/* Configure non link specific common registers */
	reg_val = BIT(NVLW_COMMON_INTR_0_MASK_FATAL);
	nvlw_tioctrl_writel(tdev, NVLW_COMMON_INTR_0_MASK, reg_val);

	reg_val = BIT(NVLW_COMMON_INTR_1_MASK_NONFATAL) |
			BIT(NVLW_COMMON_INTR_1_MASK_CORRECTABLE);
	nvlw_tioctrl_writel(tdev, NVLW_COMMON_INTR_1_MASK, reg_val);

	reg_val = BIT(NVLW_COMMON_INTR_2_MASK_INTRA) |
			BIT(NVLW_COMMON_INTR_2_MASK_INTRB);
	nvlw_tioctrl_writel(tdev, NVLW_COMMON_INTR_2_MASK, reg_val);

	/* Configure link specific registers */
	reg_val = BIT(NVLW_LINK_INTR_0_MASK_FATAL);
	nvlw_tioctrl_writel(tdev, NVLW_LINK_INTR_0_MASK, reg_val);

	reg_val = BIT(NVLW_LINK_INTR_1_MASK_NONFATAL) |
			BIT(NVLW_LINK_INTR_1_MASK_CORRECTABLE);
	nvlw_tioctrl_writel(tdev, NVLW_LINK_INTR_1_MASK, reg_val);

	reg_val = BIT(NVLW_LINK_INTR_2_MASK_INTRA) |
			BIT(NVLW_LINK_INTR_2_MASK_INTRB);
	nvlw_tioctrl_writel(tdev, NVLW_LINK_INTR_2_MASK, reg_val);
}

/* Initialize NVLIPT common interrupts */
static void nvlipt_config_common_intr(struct tnvlink_dev *tdev)
{
	u32 reg_val = 0;

	/* Allow all common types to be routed up
	 * and out on tree0 and 1
	 */
	reg_val = BIT(NVLIPT_INTR_CONTROL_COMMON_STALLENABLE) |
			BIT(NVLIPT_INTR_CONTROL_COMMON_NOSTALLENABLE);

	nvlw_nvlipt_writel(tdev, NVLIPT_INTR_CONTROL_COMMON, reg_val);
}

/* Initialize MINION common interrupts */
static void minion_config_common_intr(struct tnvlink_dev *tdev)
{
	u32 reg_val = 0;

	/* Tree 1 (non-stall) is disabled until there is a need */
	nvlw_minion_writel(tdev, MINION_MINION_INTR_NONSTALL_EN, 0);

	/* Tree 0 (stall) is where we route all MINION interrupts for now */
	reg_val = BIT(MINION_MINION_INTR_STALL_EN_FATAL) |
			BIT(MINION_MINION_INTR_STALL_EN_NONFATAL) |
			BIT(MINION_MINION_INTR_STALL_EN_FALCON_STALL) |
			BIT(MINION_MINION_INTR_STALL_EN_FALCON_NOSTALL);
	nvlw_minion_writel(tdev, MINION_MINION_INTR_STALL_EN, reg_val);
}

void nvlink_config_common_intr(struct tnvlink_dev *tdev)
{
	nvlw_config_intr(tdev);
	nvlipt_config_common_intr(tdev);
	minion_config_common_intr(tdev);
}

/* Enable MINION link interrupts */
static void nvlink_enable_minion_link_intr(struct tnvlink_dev *tdev)
{
	u32 reg_val = 0;

	/* Tree 0 (stall) only support for now */
	reg_val = nvlw_minion_readl(tdev, MINION_MINION_INTR_STALL_EN);
	reg_val |= MINION_MINION_INTR_STALL_EN_LINK(
			MINION_MINION_INTR_STALL_EN_LINK_ENABLE_ALL);
	nvlw_minion_writel(tdev, MINION_MINION_INTR_STALL_EN, reg_val);
}

void nvlink_enable_dl_interrupts(struct tnvlink_dev *tdev)
{
	u32 reg_val = 0;

	/* Clear interrupt register to get rid of any stale state. (W1C) */
	nvlw_nvl_writel(tdev, NVL_INTR, 0xffffffff);
	nvlw_nvl_writel(tdev, NVL_INTR_SW2, 0xffffffff);

	/* Recommend non-fatal interrupt line.
	 * This indicates that we have seen a significant number of bit errors
	 * and need help. This will be flagged while we are still in the process
	 * of transitioning to SWCFG.
	 */
	reg_val |= BIT(NVL_INTR_STALL_EN_TX_RECOVERY_LONG);

	/* Recommend fatal. Internal hardware parity fault. Reset required */
	reg_val |= BIT(NVL_INTR_STALL_EN_TX_FAULT_RAM);
	reg_val |= BIT(NVL_INTR_STALL_EN_TX_FAULT_INTERFACE);

	/* Recommend fatal, should never happen? */
	reg_val |= BIT(NVL_INTR_STALL_EN_TX_FAULT_SUBLINK_CHANGE);

	/* Recommend to not enable interrupt OR treat similar to RECOVERY_LONG.
	 * HW should end up failing and going to SWCFG.
	 */
	reg_val |= BIT(NVL_INTR_STALL_EN_RX_FAULT_SUBLINK_CHANGE);

	/* Recommend fatal, should not happen except through software error
	 * with AN0 injection mechanism
	 */
	reg_val |= BIT(NVL_INTR_STALL_EN_RX_FAULT_DL_PROTOCOL);

	/* Recommend fatal (after initialization completed).
	 * Internal hardware parity fault or other very bad condition,
	 * link unusable.
	 * EXCEPT during INIT->HWCFG where it indicates a failure
	 * to get to safe mode (may still be fatal but retry)
	 */
	reg_val |= BIT(NVL_INTR_STALL_EN_LTSSM_FAULT);

	/* Enable Stall interrupts */
	nvlw_nvl_writel(tdev, NVL_INTR_STALL_EN, reg_val);

	/* TODO: Check if below WAR is still needed */
	/* Configure the error threshold that generates interrupt as a WAR
	 * for bug 1710544
	 */
	reg_val = nvlw_nvl_readl(tdev, NVL_SL1_ERROR_RATE_CTRL);
	reg_val |= NVL_SL1_ERROR_RATE_CTRL_SHORT_THRESHOLD_MAN_F(0x2);
	reg_val |= NVL_SL1_ERROR_RATE_CTRL_LONG_THRESHOLD_MAN_F(0x2);
	nvlw_nvl_writel(tdev, NVL_SL1_ERROR_RATE_CTRL, reg_val);

	/* Don't hookup interrupts on NON-STALL line */
	nvlw_nvl_writel(tdev, NVL_INTR_NONSTALL_EN, 0);
}

/* Enable TLC link interrupts */
static void nvlink_enable_tl_interrupts(struct tnvlink_dev *tdev)
{
	u32 reg_val = 0;

	/* Enable TLC RX0 error reporting */
	reg_val = BIT(NVLTLC_RX_ERR_LOG_EN_0_RXDLHDRPARITYERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_0_RXDLDATAPARITYERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_0_RXDLCTRLPARITYERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_0_RXRAMDATAPARITYERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_0_RXRAMHDRPARITYERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_0_RXINVALIDAEERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_0_RXINVALIDBEERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_0_RXINVALIDADDRALIGNERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_0_RXPKTLENERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_0_RSVCMDENCERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_0_RSVDATLENENCERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_0_RSVADDRTYPEERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_0_RSVRSPSTATUSERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_0_RSVPKTSTATUSERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_0_RSVCACHEATTRPROBEREQERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_0_RSVCACHEATTRPROBERSPERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_0_DATLENGTATOMICREQMAXERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_0_DATLENGTRMWREQMAXERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_0_DATLENLTATRRSPMINERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_0_INVALIDCACHEATTRPOERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_0_INVALIDCRERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_0_RXRESPSTATUSTARGETERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_0_RXRESPSTATUSUNSUPPORTEDREQUESTERR);
	nvlw_nvltlc_writel(tdev, NVLTLC_RX_ERR_LOG_EN_0, reg_val);

	reg_val = BIT(NVLTLC_RX_ERR_REPORT_EN_0_RXDLHDRPARITYERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_0_RXDLDATAPARITYERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_0_RXDLCTRLPARITYERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_0_RXRAMDATAPARITYERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_0_RXRAMHDRPARITYERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_0_RXINVALIDAEERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_0_RXINVALIDBEERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_0_RXINVALIDADDRALIGNERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_0_RXPKTLENERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_0_RSVCMDENCERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_0_RSVDATLENENCERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_0_RSVADDRTYPEERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_0_RSVRSPSTATUSERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_0_RSVPKTSTATUSERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_0_RSVCACHEATTRPROBEREQERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_0_RSVCACHEATTRPROBERSPERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_0_DATLENGTATOMICREQMAXERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_0_DATLENGTRMWREQMAXERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_0_DATLENLTATRRSPMINERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_0_INVALIDCACHEATTRPOERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_0_INVALIDCRERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_0_RXRESPSTATUSTARGETERR) |
		BIT(
		NVLTLC_RX_ERR_REPORT_EN_0_RXRESPSTATUSUNSUPPORTEDREQUESTERR);
	nvlw_nvltlc_writel(tdev, NVLTLC_RX_ERR_REPORT_EN_0, reg_val);

	reg_val = BIT(NVLTLC_RX_ERR_CONTAIN_EN_0_RXDLHDRPARITYERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_0_RXDLDATAPARITYERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_0_RXDLCTRLPARITYERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_0_RXRAMDATAPARITYERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_0_RXRAMHDRPARITYERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_0_RXINVALIDAEERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_0_RXINVALIDBEERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_0_RXINVALIDADDRALIGNERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_0_RXPKTLENERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_0_RSVCMDENCERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_0_RSVDATLENENCERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_0_RSVADDRTYPEERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_0_RSVRSPSTATUSERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_0_RSVPKTSTATUSERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_0_RSVCACHEATTRPROBEREQERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_0_RSVCACHEATTRPROBERSPERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_0_DATLENGTATOMICREQMAXERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_0_DATLENGTRMWREQMAXERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_0_DATLENLTATRRSPMINERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_0_INVALIDCACHEATTRPOERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_0_INVALIDCRERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_0_RXRESPSTATUSTARGETERR) |
		BIT(
		NVLTLC_RX_ERR_CONTAIN_EN_0_RXRESPSTATUSUNSUPPORTEDREQUESTERR);
	nvlw_nvltlc_writel(tdev, NVLTLC_RX_ERR_CONTAIN_EN_0, reg_val);

	/* Enable TLC RX1 error reporting */
	reg_val = NVLTLC_RX_ERR_LOG_EN_1_RXHDROVFERR_F(0xFF) |
		NVLTLC_RX_ERR_LOG_EN_1_RXDATAOVFERR_F(0xFF) |
		BIT(NVLTLC_RX_ERR_LOG_EN_1_STOMPDETERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_1_RXPOISONERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_1_RXUNSUPVCOVFERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_1_RXUNSUPNVLINKCREDITRELERR) |
		BIT(NVLTLC_RX_ERR_LOG_EN_1_RXUNSUPNCISOCCREDITRELERR);
	nvlw_nvltlc_writel(tdev, NVLTLC_RX_ERR_LOG_EN_1, reg_val);

	reg_val = NVLTLC_RX_ERR_REPORT_EN_1_RXHDROVFERR_F(0xFF) |
		NVLTLC_RX_ERR_REPORT_EN_1_RXDATAOVFERR_F(0xFF) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_1_STOMPDETERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_1_RXPOISONERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_1_RXUNSUPVCOVFERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_1_RXUNSUPNVLINKCREDITRELERR) |
		BIT(NVLTLC_RX_ERR_REPORT_EN_1_RXUNSUPNCISOCCREDITRELERR);
	nvlw_nvltlc_writel(tdev, NVLTLC_RX_ERR_REPORT_EN_1, reg_val);

	reg_val = NVLTLC_RX_ERR_CONTAIN_EN_1_RXHDROVFERR_F(0xFF) |
		NVLTLC_RX_ERR_CONTAIN_EN_1_RXDATAOVFERR_F(0xFF) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_1_STOMPDETERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_1_RXPOISONERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_1_RXUNSUPVCOVFERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_1_RXUNSUPNVLINKCREDITRELERR) |
		BIT(NVLTLC_RX_ERR_CONTAIN_EN_1_RXUNSUPNCISOCCREDITRELERR);
	nvlw_nvltlc_writel(tdev, NVLTLC_RX_ERR_CONTAIN_EN_1, reg_val);

	/* Enable TLC TX error reporting */
	reg_val = NVLTLC_TX_ERR_LOG_EN_0_TXHDRCREDITOVFERR_F(0xFF) |
		NVLTLC_TX_ERR_LOG_EN_0_TXDATACREDITOVFERR_F(0xFF) |
		BIT(NVLTLC_TX_ERR_LOG_EN_0_TXDLCREDITOVFERR) |
		BIT(NVLTLC_TX_ERR_LOG_EN_0_TXDLCREDITPARITYERR) |
		BIT(NVLTLC_TX_ERR_LOG_EN_0_TXRAMHDRPARITYERR) |
		BIT(NVLTLC_TX_ERR_LOG_EN_0_TXRAMDATAPARITYERR) |
		BIT(NVLTLC_TX_ERR_LOG_EN_0_TXUNSUPVCOVFERR) |
		BIT(NVLTLC_TX_ERR_LOG_EN_0_TXSTOMPDET) |
		BIT(NVLTLC_TX_ERR_LOG_EN_0_TXPOISONDET) |
		BIT(NVLTLC_TX_ERR_LOG_EN_0_TARGETERR) |
		BIT(NVLTLC_TX_ERR_LOG_EN_0_UNSUPPORTEDREQUESTERR);
	nvlw_nvltlc_writel(tdev, NVLTLC_TX_ERR_LOG_EN_0, reg_val);

	reg_val = NVLTLC_TX_ERR_REPORT_EN_0_TXHDRCREDITOVFERR_F(0xFF) |
		NVLTLC_TX_ERR_REPORT_EN_0_TXDATACREDITOVFERR_F(0xFF) |
		BIT(NVLTLC_TX_ERR_REPORT_EN_0_TXDLCREDITOVFERR) |
		BIT(NVLTLC_TX_ERR_REPORT_EN_0_TXDLCREDITPARITYERR) |
		BIT(NVLTLC_TX_ERR_REPORT_EN_0_TXRAMHDRPARITYERR) |
		BIT(NVLTLC_TX_ERR_REPORT_EN_0_TXRAMDATAPARITYERR) |
		BIT(NVLTLC_TX_ERR_REPORT_EN_0_TXUNSUPVCOVFERR) |
		BIT(NVLTLC_TX_ERR_REPORT_EN_0_TXSTOMPDET) |
		BIT(NVLTLC_TX_ERR_REPORT_EN_0_TXPOISONDET) |
		BIT(NVLTLC_TX_ERR_REPORT_EN_0_TARGETERR) |
		BIT(NVLTLC_TX_ERR_REPORT_EN_0_UNSUPPORTEDREQUESTERR);
	nvlw_nvltlc_writel(tdev, NVLTLC_TX_ERR_REPORT_EN_0, reg_val);

	reg_val = NVLTLC_TX_ERR_CONTAIN_EN_0_TXHDRCREDITOVFERR_F(0xFF) |
		NVLTLC_TX_ERR_CONTAIN_EN_0_TXDATACREDITOVFERR_F(0xFF) |
		BIT(NVLTLC_TX_ERR_CONTAIN_EN_0_TXDLCREDITOVFERR) |
		BIT(NVLTLC_TX_ERR_CONTAIN_EN_0_TXDLCREDITPARITYERR) |
		BIT(NVLTLC_TX_ERR_CONTAIN_EN_0_TXRAMHDRPARITYERR) |
		BIT(NVLTLC_TX_ERR_CONTAIN_EN_0_TXUNSUPVCOVFERR) |
		BIT(NVLTLC_TX_ERR_CONTAIN_EN_0_TXSTOMPDET) |
		BIT(NVLTLC_TX_ERR_CONTAIN_EN_0_TXPOISONDET) |
		BIT(NVLTLC_TX_ERR_CONTAIN_EN_0_TARGETERR) |
		BIT(NVLTLC_TX_ERR_CONTAIN_EN_0_UNSUPPORTEDREQUESTERR);
	nvlw_nvltlc_writel(tdev, NVLTLC_TX_ERR_CONTAIN_EN_0, reg_val);
}

static void nvlink_enable_sync2x_interrupts(struct tnvlink_dev *tdev)
{
	u32 reg_val = 0;

	reg_val = nvlw_sync2x_readl(tdev, NVSYNC2X_ECCPARITY_CTRL);
	reg_val |= BIT(NVSYNC2X_ECCPARITY_CTRL_RX_PARITY_CTRL2_ENB) |
		BIT(NVSYNC2X_ECCPARITY_CTRL_TX_ECCENB0) |
		BIT(NVSYNC2X_ECCPARITY_CTRL_TX_PARITY_CTRL1_ENB) |
		BIT(NVSYNC2X_ECCPARITY_CTRL_TX_ECCPARITYCOUNTERENB0) |
		BIT(NVSYNC2X_ECCPARITY_CTRL_TX_ECCPARITYCOUNTSINGLEBIT0);
	nvlw_sync2x_writel(tdev, NVSYNC2X_ECCPARITY_CTRL, reg_val);

	/* Set TX ECC parity error limit */
	reg_val = nvlw_sync2x_readl(tdev, NVSYNC2X_TX_ECCPARITY_ERROR_LIMIT);
	reg_val &= ~NVSYNC2X_TX_ECCPARITY_ERROR_LIMIT_ERROR_LIMIT_F(~0);
	reg_val |= NVSYNC2X_TX_ECCPARITY_ERROR_LIMIT_ERROR_LIMIT_F(0x100);
	nvlw_sync2x_writel(tdev, NVSYNC2X_TX_ECCPARITY_ERROR_LIMIT, reg_val);

	/* Enable logging of the errors */
	reg_val = nvlw_sync2x_readl(tdev, NVSYNC2X_ERR_LOG_EN_0);
	reg_val |= BIT(NVSYNC2X_ERR_LOG_EN_0_RXPARITYCTRL2ERR) |
		BIT(NVSYNC2X_ERR_LOG_EN_0_TXECCPARITYLIMITERR) |
		BIT(NVSYNC2X_ERR_LOG_EN_0_TXECCHDRDOUBLEBITERR) |
		BIT(NVSYNC2X_ERR_LOG_EN_0_TXECCDATADOUBLEBITERR) |
		BIT(NVSYNC2X_ERR_LOG_EN_0_TXPARITYCTRL0ERR) |
		BIT(NVSYNC2X_ERR_LOG_EN_0_TXPARITYCTRL1ERR);
	nvlw_sync2x_writel(tdev, NVSYNC2X_ERR_LOG_EN_0, reg_val);

	/* Enable interrupt generation on logged errors */
	reg_val = nvlw_sync2x_readl(tdev, NVSYNC2X_ERR_REPORT_EN_0);
	reg_val |= BIT(NVSYNC2X_ERR_REPORT_EN_0_RXPARITYCTRL2ERR) |
		BIT(NVSYNC2X_ERR_REPORT_EN_0_TXECCPARITYLIMITERR) |
		BIT(NVSYNC2X_ERR_REPORT_EN_0_TXECCHDRDOUBLEBITERR) |
		BIT(NVSYNC2X_ERR_REPORT_EN_0_TXECCDATADOUBLEBITERR) |
		BIT(NVSYNC2X_ERR_REPORT_EN_0_TXPARITYCTRL0ERR) |
		BIT(NVSYNC2X_ERR_REPORT_EN_0_TXPARITYCTRL1ERR);
	nvlw_sync2x_writel(tdev, NVSYNC2X_ERR_REPORT_EN_0, reg_val);

	/* Enable freezing of the link interface on logged errors */
	reg_val = nvlw_sync2x_readl(tdev, NVSYNC2X_ERR_CONTAIN_EN_0);
	reg_val |= BIT(NVSYNC2X_ERR_CONTAIN_EN_0_RXPARITYCTRL2ERR) |
		BIT(NVSYNC2X_ERR_CONTAIN_EN_0_TXECCHDRDOUBLEBITERR) |
		BIT(NVSYNC2X_ERR_CONTAIN_EN_0_TXECCDATADOUBLEBITERR) |
		BIT(NVSYNC2X_ERR_CONTAIN_EN_0_TXPARITYCTRL0ERR) |
		BIT(NVSYNC2X_ERR_CONTAIN_EN_0_TXPARITYCTRL1ERR);
	nvlw_sync2x_writel(tdev, NVSYNC2X_ERR_CONTAIN_EN_0, reg_val);
}

/* Enable NVLIPT Link interrupts */
static void nvlink_enable_nvlipt_interrupts(struct tnvlink_dev *tdev)
{
	u32 reg_val;

	/*
	 * Enable stall/nonstall interrupts to host for this link.
	 * This is a rollup of all interrupts in all devices for
	 * this link and is required for any interrupts to be handled by SW.
	 */
	reg_val = nvlw_nvlipt_readl(tdev, NVLIPT_INTR_CONTROL_LINK0);
	reg_val |= BIT(NVLIPT_INTR_CONTROL_LINK0_STALLENABLE);
	reg_val |= BIT(NVLIPT_INTR_CONTROL_LINK0_NOSTALLENABLE);
	nvlw_nvlipt_writel(tdev, NVLIPT_INTR_CONTROL_LINK0, reg_val);

	reg_val = nvlw_nvlipt_readl(tdev, NVLIPT_ERR_UC_MASK_LINK0);
	reg_val &= ~(BIT(NVLIPT_ERR_UC_MASK_LINK0_DATAPOISONED) |
		BIT(NVLIPT_ERR_UC_MASK_LINK0_FLOWCONTROL) |
		BIT(NVLIPT_ERR_UC_MASK_LINK0_TARGETERROR) |
		BIT(NVLIPT_ERR_UC_MASK_LINK0_RECEIVEROVERFLOW) |
		BIT(NVLIPT_ERR_UC_MASK_LINK0_MALFORMEDPACKET) |
		BIT(NVLIPT_ERR_UC_MASK_LINK0_STOMPEDPACKETRECEIVED) |
		BIT(NVLIPT_ERR_UC_MASK_LINK0_UNSUPPORTEDREQUEST) |
		BIT(NVLIPT_ERR_UC_MASK_LINK0_UCINTERNAL));
	nvlw_nvlipt_writel(tdev, NVLIPT_ERR_UC_MASK_LINK0, reg_val);

	reg_val = nvlw_nvlipt_readl(tdev, NVLIPT_ERR_UC_SEVERITY_LINK0);
	reg_val |= BIT(NVLIPT_ERR_UC_SEVERITY_LINK0_DATAPOISONED) |
		BIT(NVLIPT_ERR_UC_SEVERITY_LINK0_FLOWCONTROL) |
		BIT(NVLIPT_ERR_UC_SEVERITY_LINK0_TARGETERROR) |
		BIT(NVLIPT_ERR_UC_SEVERITY_LINK0_RECEIVEROVERFLOW) |
		BIT(NVLIPT_ERR_UC_SEVERITY_LINK0_MALFORMEDPACKET) |
		BIT(NVLIPT_ERR_UC_SEVERITY_LINK0_STOMPEDPACKETRECEIVED) |
		BIT(NVLIPT_ERR_UC_SEVERITY_LINK0_UNSUPPORTEDREQUEST) |
		BIT(NVLIPT_ERR_UC_SEVERITY_LINK0_UCINTERNAL);
	nvlw_nvlipt_writel(tdev, NVLIPT_ERR_UC_SEVERITY_LINK0, reg_val);

	reg_val = nvlw_nvlipt_readl(tdev, NVLIPT_ERR_C_MASK_LINK0);
	reg_val &= ~BIT(NVLIPT_ERR_C_MASK_LINK0_CINTERNAL);
	nvlw_nvlipt_writel(tdev, NVLIPT_ERR_C_MASK_LINK0, reg_val);

	reg_val = nvlw_nvlipt_readl(tdev, NVLIPT_ERR_CONTROL_LINK0);
	reg_val |= BIT(NVLIPT_ERR_CONTROL_LINK0_FATALENABLE) |
		BIT(NVLIPT_ERR_CONTROL_LINK0_CORRECTABLEENABLE);
	nvlw_nvlipt_writel(tdev, NVLIPT_ERR_CONTROL_LINK0, reg_val);
}

/* Enable link interrupts */
void nvlink_enable_link_interrupts(struct tnvlink_dev *tdev)
{
	nvlink_enable_minion_link_intr(tdev);
	nvlink_enable_dl_interrupts(tdev);
	nvlink_enable_tl_interrupts(tdev);
	nvlink_enable_sync2x_interrupts(tdev);
	nvlink_enable_nvlipt_interrupts(tdev);
}

/* Disable NVLIPT Link interrupts */
static void nvlink_disable_nvlipt_interrupts(struct tnvlink_dev *tdev)
{
	u32 reg_val;

	reg_val = nvlw_nvlipt_readl(tdev, NVLIPT_INTR_CONTROL_LINK0);
	reg_val &= ~BIT(NVLIPT_INTR_CONTROL_LINK0_STALLENABLE);
	reg_val &= ~BIT(NVLIPT_INTR_CONTROL_LINK0_NOSTALLENABLE);
	nvlw_nvlipt_writel(tdev, NVLIPT_INTR_CONTROL_LINK0, reg_val);
}

/* Disable MINION FALCON interrupts */
static void nvlink_minion_disable_falcon_interrupts(struct tnvlink_dev *tdev)
{
	u32 reg_data;

	reg_data = nvlw_minion_readl(tdev, MINION_MINION_INTR_STALL_EN);
	reg_data &= ~BIT(MINION_MINION_INTR_STALL_EN_FATAL);
	reg_data &= ~BIT(MINION_MINION_INTR_STALL_EN_NONFATAL);
	reg_data &= ~BIT(MINION_MINION_INTR_STALL_EN_FALCON_STALL);
	reg_data &= ~BIT(MINION_MINION_INTR_STALL_EN_FALCON_NOSTALL);
	nvlw_minion_writel(tdev, MINION_MINION_INTR_STALL_EN, reg_data);
}

/* Service MINION Falcon interrupts */
void minion_service_falcon_intr(struct tnvlink_dev *tdev)
{
	u32 irq_stat = 0;
	u32 irq_mask = 0;
	u32 interrupts = 0;
	u32 clear_bits = 0;

	/*
	 * Get the current IRQ status and mask for the sources not directed to
	 * host
	 */
	irq_stat = nvlw_minion_readl(tdev, CMINION_FALCON_IRQSTAT);

	irq_mask = nvlw_minion_readl(tdev, CMINION_FALCON_IRQMASK);

	interrupts = irq_stat & irq_mask;

	/* Exit if there is nothing to do */
	if (interrupts == 0)
		return;

	/* Service the pending interrupt(s) */
	if (interrupts & BIT(CMINION_FALCON_IRQSTAT_WDTMR)) {
		nvlink_err("Received MINION Falcon WDTMR interrupt");
		clear_bits |= BIT(CMINION_FALCON_IRQSTAT_WDTMR);
	}
	if (interrupts & BIT(CMINION_FALCON_IRQSTAT_HALT)) {
		nvlink_err("Received MINION Falcon HALT interrupt");
		clear_bits |= BIT(CMINION_FALCON_IRQSTAT_HALT);
	}
	if (interrupts & BIT(CMINION_FALCON_IRQSTAT_EXTERR)) {
		nvlink_err("Received MINION Falcon EXTERR interrupt");
		clear_bits |= BIT(CMINION_FALCON_IRQSTAT_EXTERR);
	}

	/* We are considering all falcon interrupts as fatal.
	 * Disable MINION Falcon interrupts.
	 */
	nvlink_minion_disable_falcon_interrupts(tdev);

	nvlink_err("MINION Falcon interrupts disabled due to fatal interrupt");

	/* Clear interrupt (W1C) */
	nvlw_minion_writel(tdev, CMINION_FALCON_IRQSCLR, clear_bits);
}

/* Service MINION FATAL notification interrupt */
static bool minion_service_fatal_intr(struct tnvlink_dev *tdev)
{
	nvlink_dbg("Received MINION Falcon FATAL notification interrupt");

	/* Disable interrupts - cannot recover */
	nvlink_minion_disable_falcon_interrupts(tdev);

	nvlink_err("MINION Falcon interrupts disabled due to fatal"
			" notification interrupt");

	/* Clear interrupt (W1C) */
	nvlw_minion_writel(tdev, MINION_MINION_INTR,
				BIT(MINION_MINION_INTR_FATAL));

	return 0;
}

/* Service MINION NONFATAL notification interrupt */
static bool minion_service_non_fatal_intr(struct tnvlink_dev *tdev)
{
	nvlink_dbg("Received MINION Falcon NONFATAL notification interrupt");

	/* Clear interrupt (W1C) */
	nvlw_minion_writel(tdev, MINION_MINION_INTR,
			BIT(MINION_MINION_INTR_NONFATAL));
	return 0;
}

/* Disable MINION link interrupts */
static void minion_disable_link_intr(struct tnvlink_dev *tdev)
{
	u32 intr_en;

	/* Tree 0 (stall) only support for now */
	intr_en = nvlw_minion_readl(tdev, MINION_MINION_INTR_STALL_EN);
	intr_en &= ~MINION_MINION_INTR_STALL_EN_LINK(
		MINION_MINION_INTR_STALL_EN_LINK_ENABLE_ALL);
	nvlw_minion_writel(tdev, MINION_MINION_INTR_STALL_EN, intr_en);
}

/* Service MINION link interrupts */
static bool minion_service_link_intr(struct tnvlink_dev *tdev)
{
	u32 link_intr = nvlw_minion_readl(tdev, MINION_NVLINK_LINK_INTR);
	bool fatal_interrupt = false;
	int intr_code;

	nvlink_dbg("NVLink MINION Link Interrupt: MINION_NVLINK_LINK_INTR 0x%x",
						link_intr);

	intr_code = (link_intr & MINION_NVLINK_LINK_INTR_CODE_MASK) >>
			MINION_NVLINK_LINK_INTR_CODE_SHIFT;

	switch (intr_code) {

	/* The following are considered NON-FATAL by arch */
	case MINION_NVLINK_LINK_INTR_CODE_SWREQ:
		nvlink_dbg("Received NON-FATAL INTR_CODE = SWREQ");
		break;

	/* The following are considered FATAL by arch */
	case MINION_NVLINK_LINK_INTR_CODE_NA:
		nvlink_dbg("Received FATAL INTR_CODE = NA");
		fatal_interrupt = true;
		break;

	case MINION_NVLINK_LINK_INTR_CODE_DLREQ:
		nvlink_dbg("Received FATAL INTR_CODE = DLREQ");
		fatal_interrupt = true;
		break;

	default:
		nvlink_dbg("Received UNKNOWN INTR_CODE = 0x%x", intr_code);
		fatal_interrupt = true;
		break;
	}

	/* On fatal interrupts, disable interrupts for that link */
	if (fatal_interrupt) {
		minion_disable_link_intr(tdev);
		nvlink_err("NVLink MINION link interrupts disabled due to fatal"
			" MINION error: INTR_CODE = 0x%x", intr_code);
	}

	/* Clear the interrupt state and move on */
	link_intr |= BIT(MINION_NVLINK_LINK_INTR_STATE);
	nvlw_minion_writel(tdev, MINION_NVLINK_LINK_INTR, link_intr);

	return true;
}

/* Service MINION interrupts */
static bool nvlink_minion_service_intr(struct tnvlink_dev *tdev)
{
	u32 interrupts;
	u32 interrupting_links;

	/* Currently we only handle tree 0 */
	/* Filter any interrupts against selected tree */
	interrupts = nvlw_minion_readl(tdev, MINION_MINION_INTR) &
			nvlw_minion_readl(tdev, MINION_MINION_INTR_STALL_EN);

	/* Service Falcon interrupts before we process engine interrutps */
	if (interrupts & (BIT(MINION_MINION_INTR_FALCON_STALL) |
			BIT(MINION_MINION_INTR_FALCON_NOSTALL)))
		minion_service_falcon_intr(tdev);

	/* Process ucode->driver FATAL notifications */
	if (interrupts & BIT(MINION_MINION_INTR_FATAL))
		minion_service_fatal_intr(tdev);

	/* Process ucode->driver NONFATAL notifications */
	if (interrupts & BIT(MINION_MINION_INTR_NONFATAL))
		minion_service_non_fatal_intr(tdev);

	/* Process interrupting links */
	interrupting_links = MINION_MINION_INTR_LINK_V(interrupts);

	if (interrupting_links & 1)
		minion_service_link_intr(tdev);

	interrupts = nvlw_minion_readl(tdev, MINION_MINION_INTR) &
			nvlw_minion_readl(tdev, MINION_MINION_INTR_STALL_EN);

	return (interrupts == 0);
}

/* Disable DL/PL interrupts */
void nvlink_disable_dl_interrupts(struct tnvlink_dev *tdev)
{
	nvlw_nvl_writel(tdev, NVL_INTR_NONSTALL_EN, 0);
	nvlw_nvl_writel(tdev, NVL_INTR_STALL_EN, 0);
}

/* Disable TLC interrupts */
static void nvlink_disable_tl_interrupts(struct tnvlink_dev *tdev)
{
	/* Disable TLC RX interrupts */
	nvlw_nvltlc_writel(tdev, NVLTLC_RX_ERR_REPORT_EN_0, 0);
	nvlw_nvltlc_writel(tdev, NVLTLC_RX_ERR_REPORT_EN_1, 0);

	/* Disable TLC TX interrupts */
	nvlw_nvltlc_writel(tdev, NVLTLC_TX_ERR_REPORT_EN_0, 0);
}

/* Handles errors reported on a link. This will disable link interrupts
 *  for fatal, non-injected interrupts on the device that reports them
 */
static void nvlink_handle_link_errors(struct tnvlink_dev *tdev,
				struct nvlink_link_error_masks *err_masks,
				u64 inforom_mask)
{
	if (err_masks->dl)
		nvlink_disable_dl_interrupts(tdev);

	/* Log publicly if a fatal NVLink error has occurred - these are never
	 * expected.
	 */
	if (inforom_mask) {
		nvlink_err("fatal error detected, inforom 0x%llx",
					inforom_mask);
		/* TODO :
		 * Log a set of fatal errors that have occurred on the given
		 * link to the NVL object in the InfoROM.
		 */
	}
}

int nvlink_service_dl_interrupts(struct tnvlink_dev *tdev,
				 bool *retrain_from_safe)
{
	u32 nonfatal_mask = 0;
	u32 fatal_mask = 0;
	u32 inforom_mask = 0;
	u32 intr_status = 0;
	int ret = 0;
	struct nvlink_link_error_masks err_masks = {0};

	*retrain_from_safe = false;

	/*
	 * Mask DLPL intr register while reading it. This ensures that we
	 * operate only on enabled interrupt bits. HW triggers interrupts when
	 * (NVL_INTR & NVL_INTR_STALL_EN) is non-zero.
	 * Hence, SW needs to follow the same masking logic to filter out
	 * interrupts.
	 */
	intr_status = nvlw_nvl_readl(tdev, NVL_INTR) &
			nvlw_nvl_readl(tdev, NVL_INTR_STALL_EN);

	if (intr_status & BIT(NVL_INTR_TX_REPLAY)) {
		nvlink_err("Non Fatal: TX Replay DL interrupt hit on link");
		nonfatal_mask |= BIT(NVL_INTR_TX_REPLAY);
	}

	if (intr_status & BIT(NVL_INTR_TX_RECOVERY_SHORT)) {
		nvlink_err("Non Fatal: TX Recovery Short DL interrupt hit"
				" on link");
		nonfatal_mask |= BIT(NVL_INTR_TX_RECOVERY_SHORT);
	}

	if (intr_status & BIT(NVL_INTR_TX_RECOVERY_LONG)) {
		nvlink_err("Fatal: TX Recovery Long DL interrupt hit on link");
		nvlink_err("Retraining from SAFE");
		nonfatal_mask |= BIT(NVL_INTR_TX_RECOVERY_LONG);
		inforom_mask |= BIT(DL_TX_RECOVERY_LONG);
		*retrain_from_safe = true;
	}

	if (intr_status & BIT(NVL_INTR_TX_FAULT_RAM)) {
		nvlink_err("Fatal: TX Fault RAM DL interrupt hit on link");
		nvlink_err("Reset Required");
		fatal_mask |= BIT(NVL_INTR_TX_FAULT_RAM);
		inforom_mask |= BIT(DL_TX_FAULT_RAM);
	}

	if (intr_status & BIT(NVL_INTR_TX_FAULT_INTERFACE)) {
		nvlink_err("Fatal: TX Fault Interface DL interrupt hit on"
				" link");
		nvlink_err("Reset Required");
		fatal_mask |= BIT(NVL_INTR_TX_FAULT_INTERFACE);
		inforom_mask |= BIT(DL_TX_FAULT_INTERFACE);
	}

	if (intr_status & BIT(NVL_INTR_TX_FAULT_SUBLINK_CHANGE)) {
		nvlink_err("Fatal: TX Fault Sublink Change DL interrupt hit"
				" on link ");
		fatal_mask |= BIT(NVL_INTR_TX_FAULT_SUBLINK_CHANGE);
		inforom_mask |= BIT(DL_TX_FAULT_SUBLINK_CHANGE);
	}

	if (intr_status & BIT(NVL_INTR_RX_FAULT_SUBLINK_CHANGE)) {
		nvlink_err("Fatal: RX Fault Sublink Change DL interrupt hit"
				" on link");
		fatal_mask |= BIT(NVL_INTR_RX_FAULT_SUBLINK_CHANGE);
		inforom_mask |= BIT(DL_RX_FAULT_SUBLINK_CHANGE);
	}

	if (intr_status & BIT(NVL_INTR_RX_FAULT_DL_PROTOCOL)) {
		nvlink_err("Fatal: RX Fault DL Protocol interrupt hit on link");
		fatal_mask |= BIT(NVL_INTR_RX_FAULT_DL_PROTOCOL);
		inforom_mask |= BIT(DL_RX_FAULT_DL_PROTOCOL);
	}

	if (intr_status & BIT(NVL_INTR_RX_SHORT_ERROR_RATE)) {
		nvlink_err("Non Fatal: RX Short Error Rate DL interrupt hit"
				" on link ");
		nonfatal_mask |= BIT(NVL_INTR_RX_SHORT_ERROR_RATE);
	}

	if (intr_status & BIT(NVL_INTR_RX_LONG_ERROR_RATE)) {
		nvlink_err("Non Fatal: RX Long Error Rate Change DL interrupt"
				" hit on link");
		nonfatal_mask |= BIT(NVL_INTR_RX_LONG_ERROR_RATE);
	}

	if (intr_status & BIT(NVL_INTR_RX_ILA_TRIGGER)) {
		nvlink_err("Non Fatal: RX internal Logic Analyzer DL interrupt"
				" hit on link. Ignore");
		nonfatal_mask |= BIT(NVL_INTR_RX_ILA_TRIGGER);
	}

	if (intr_status & BIT(NVL_INTR_LTSSM_FAULT)) {
		nvlink_err("Fatal: LTSSM Fault DL interrupt hit on link ");
		fatal_mask |= BIT(NVL_INTR_LTSSM_FAULT);
		inforom_mask |= BIT(DL_LTSSM_FAULT);
	}

	if (intr_status & BIT(NVL_INTR_LTSSM_PROTOCOL)) {
		nvlink_err("Non Fatal: LTSSM Protocol DL interrupt hit on link."
				" Ignore for now");
		nonfatal_mask |= BIT(NVL_INTR_LTSSM_PROTOCOL);
	}

	if ((fatal_mask | nonfatal_mask) != 0) {
		intr_status &= ~(nonfatal_mask | fatal_mask);

		if (intr_status) {
			/* did not log all interrupts received */
			nvlink_err("Unable to service enabled interrupts for"
				" link");
			ret = -1;
		}
	}

	if (fatal_mask)
		*retrain_from_safe = false;

	/*
	 * NOTE: _TX_RECOVERY_LONG is non-fatal if handled by SW, but still
	 * should be logged to inforom here.
	 */
	if (fatal_mask | inforom_mask) {
		err_masks.dl = fatal_mask;
		nvlink_handle_link_errors(tdev, &err_masks, inforom_mask);
	}

	if (*retrain_from_safe) {
		if (tdev->rm_shim_enabled) {
			/*
			 * FIXME: Fix the following hack. The proper solution
			 * would be to increment the error recoveries count
			 * after RM notifies us that the link has been
			 * retrained.
			 */
			nvlink_dbg("We've encountered an error which requires"
				" link retraining but we 're in RM shim driver"
				" mode. And in RM shim driver mode we don't"
				" know when RM will retrain the link. Currently"
				" we just assume that RM will retrain the link"
				" successfully. Therefore, we blindly increment"
				" the successful error recoveries count.");
			tdev->tlink.error_recoveries++;
		} else {
			if (nvlink_retrain_link(tdev, false)) {
				nvlink_err("Fatal: Unable to retrain Link from"
					" SAFE mode");
				ret = -1;
			}
		}
	}

	/* Clear interrupt register (W1C) */
	nvlw_nvl_writel(tdev, NVL_INTR, (nonfatal_mask | fatal_mask));

	/* Always clear SW2 to cover sideband "err" interfaces to NVLIPT */
	nvlw_nvl_writel(tdev, NVL_INTR_SW2, 0xffffffff);

	return ret;
}

/* Get status of TL interrupts */
static void nvltlc_get_intr_status(struct tnvlink_dev *tdev,
				u32 *tlc_tx_err_status0,
				u32 *tlc_rx_err_status0,
				u32 *tlc_rx_err_status1)
{
	*tlc_tx_err_status0 = nvlw_nvltlc_readl(tdev, NVLTLC_TX_ERR_STATUS_0);
	*tlc_rx_err_status0 = nvlw_nvltlc_readl(tdev, NVLTLC_RX_ERR_STATUS_0);
	*tlc_rx_err_status1 = nvlw_nvltlc_readl(tdev, NVLTLC_RX_ERR_STATUS_1);
}

/* Service NVLIPT interrupts */
static void nvlink_service_nvlipt_interrupts(struct tnvlink_dev *tdev)
{
	u32 nvlipt_err_uc_active_bits = 0;

	nvlipt_err_uc_active_bits = BIT(NVLIPT_ERR_UC_STATUS_LINK0_DLPROTOCOL) |
			BIT(NVLIPT_ERR_UC_STATUS_LINK0_DATAPOISONED) |
			BIT(NVLIPT_ERR_UC_STATUS_LINK0_FLOWCONTROL) |
			BIT(NVLIPT_ERR_UC_STATUS_LINK0_RESPONSETIMEOUT) |
			BIT(NVLIPT_ERR_UC_STATUS_LINK0_TARGETERROR) |
			BIT(NVLIPT_ERR_UC_STATUS_LINK0_UNEXPECTEDRESPONSE) |
			BIT(NVLIPT_ERR_UC_STATUS_LINK0_RECEIVEROVERFLOW) |
			BIT(NVLIPT_ERR_UC_STATUS_LINK0_MALFORMEDPACKET) |
			BIT(NVLIPT_ERR_UC_STATUS_LINK0_STOMPEDPACKETRECEIVED) |
			BIT(NVLIPT_ERR_UC_STATUS_LINK0_UNSUPPORTEDREQUEST) |
			BIT(NVLIPT_ERR_UC_STATUS_LINK0_UCINTERNAL);

	/*
	 * Interrupt handling (mask/handle/unmask) happens in the leaf handlers,
	 * here we simply assume all interrupts were handled and clear the
	 * roll ups.
	 */
	nvlw_nvlipt_writel(tdev, NVLIPT_ERR_UC_FIRST_LINK0,
			nvlipt_err_uc_active_bits);
	nvlw_nvlipt_writel(tdev, NVLIPT_ERR_UC_STATUS_LINK0,
			nvlipt_err_uc_active_bits);
}

static u32 nvlink_service_link(struct tnvlink_dev *tdev)
{
	u32 tlc_tx_err_status0;
	u32 tlc_rx_err_status0;
	u32 tlc_rx_err_status1;
	bool retrain_from_safe = false;

	/*
	 * Cache the error log register for clients.  Need to cache it here
	 * because if the link is retrained during the DL interrupt handler
	 * it will clear the TL interrupt status.
	 */
	nvltlc_get_intr_status(tdev, &tlc_tx_err_status0,
					&tlc_rx_err_status0,
					&tlc_rx_err_status1);
	tdev->tlink.tlc_tx_err_status0 |= tlc_tx_err_status0;
	tdev->tlink.tlc_rx_err_status0 |= tlc_rx_err_status0;
	tdev->tlink.tlc_rx_err_status1 |= tlc_rx_err_status1;

	nvlink_service_dl_interrupts(tdev, &retrain_from_safe);

	/* NVLIPT is the IP top level, it goes last */
	nvlink_service_nvlipt_interrupts(tdev);

	return 0;
}

/* Disable link interrupts */
void nvlink_disable_link_interrupts(struct tnvlink_dev *tdev)
{
	minion_disable_link_intr(tdev);
	nvlink_disable_tl_interrupts(tdev);
	nvlink_disable_dl_interrupts(tdev);
	nvlink_disable_nvlipt_interrupts(tdev);
}

irqreturn_t t19x_nvlink_endpt_isr(int irq, void *dev_id)
{
	struct tnvlink_dev *tdev = dev_id;

	nvlink_dbg("Interrupt received! IRQ # = %d", irq);

	/* Service MINION first (per arch) */
	nvlink_minion_service_intr(tdev);

	nvlink_service_link(tdev);

	return IRQ_HANDLED;
}
