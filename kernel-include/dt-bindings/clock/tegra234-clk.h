/*
 * Copyright (c) 2018-2020, NVIDIA CORPORATION. All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef BPMP_ABI_MACH_T234_CLOCK_H
#define BPMP_ABI_MACH_T234_CLOCK_H

/**
 * @file
 * @defgroup bpmp_clock_ids Clock ID's
 * @{
 */
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_ACTMON */
#define TEGRA234_CLK_ACTMON			1U
/** @brief output of gate CLK_ENB_ADSP */
#define TEGRA234_CLK_ADSP			2U
/** @brief output of gate CLK_ENB_ADSPNEON */
#define TEGRA234_CLK_ADSPNEON			3U
/** output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_AHUB */
#define TEGRA234_CLK_AHUB			4U
/** @brief output of gate CLK_ENB_APB2APE */
#define TEGRA234_CLK_APB2APE			5U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_APE */
#define TEGRA234_CLK_APE			6U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_AUD_MCLK */
#define TEGRA234_CLK_AUD_MCLK			7U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_AXI_CBB */
#define TEGRA234_CLK_AXI_CBB			8U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_CAN1 */
#define TEGRA234_CLK_CAN1			9U
/** @brief output of gate CLK_ENB_CAN1_HOST */
#define TEGRA234_CLK_CAN1_HOST			10U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_CAN2 */
#define TEGRA234_CLK_CAN2			11U
/** @brief output of gate CLK_ENB_CAN2_HOST */
#define TEGRA234_CLK_CAN2_HOST			12U
/** @brief output of gate CLK_ENB_CEC */
#define TEGRA234_CLK_CEC			13U
/** @brief output of divider CLK_RST_CONTROLLER_CLK_M_DIVIDE */
#define TEGRA234_CLK_CLK_M			14U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_DMIC1 */
#define TEGRA234_CLK_DMIC1			15U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_DMIC2 */
#define TEGRA234_CLK_DMIC2			16U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_DMIC3 */
#define TEGRA234_CLK_DMIC3			17U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_DMIC4 */
#define TEGRA234_CLK_DMIC4			18U
/** @brief output of gate CLK_ENB_DPAUX */
#define TEGRA234_CLK_DPAUX			19U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_NVJPG1 */
#define TEGRA234_CLK_NVJPG1			20U
/**
 * @brief output of mux controlled by CLK_RST_CONTROLLER_ACLK_BURST_POLICY
 * divided by the divider controlled by ACLK_CLK_DIVISOR in
 * CLK_RST_CONTROLLER_SUPER_ACLK_DIVIDER
 */
#define TEGRA234_CLK_ACLK			21U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_MSS_ENCRYPT switch divider output */
#define TEGRA234_CLK_MSS_ENCRYPT		22U
/** @brief clock recovered from EAVB input */
#define TEGRA234_CLK_EQOS_RX_INPUT		23U
/** @brief Output of gate CLK_ENB_IQC2 */
#define TEGRA234_CLK_IQC2			24U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_AON_APB switch divider output */
#define TEGRA234_CLK_AON_APB			25U
/** @brief CLK_RST_CONTROLLER_AON_NIC_RATE divider output */
#define TEGRA234_CLK_AON_NIC			26U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_AON_CPU_NIC switch divider output */
#define TEGRA234_CLK_AON_CPU_NIC		27U
/** @brief PLL controlled by CLK_RST_CONTROLLER_PLLA1_BASE for use by audio clocks */
#define TEGRA234_CLK_PLLA1			28U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_DSPK1 */
#define TEGRA234_CLK_DSPK1			29U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_DSPK2 */
#define TEGRA234_CLK_DSPK2			30U
/**
 * @brief controls the EMC clock frequency.
 * @details Doing a clk_set_rate on this clock will select the
 * appropriate clock source, program the source rate and execute a
 * specific sequence to switch to the new clock source for both memory
 * controllers. This can be used to control the balance between memory
 * throughput and memory controller power.
 */
#define TEGRA234_CLK_EMC			31U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_EQOS_AXI_CLK_0 divider gated output */
#define TEGRA234_CLK_EQOS_AXI			32U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_EQOS_PTP_REF_CLK_0 divider gated output */
#define TEGRA234_CLK_EQOS_PTP_REF		33U
/** @brief output of gate CLK_ENB_EQOS_RX */
#define TEGRA234_CLK_EQOS_RX			34U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_EQOS_TX_CLK divider gated output */
#define TEGRA234_CLK_EQOS_TX			35U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_EXTPERIPH1 */
#define TEGRA234_CLK_EXTPERIPH1			36U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_EXTPERIPH2 */
#define TEGRA234_CLK_EXTPERIPH2			37U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_EXTPERIPH3 */
#define TEGRA234_CLK_EXTPERIPH3			38U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_EXTPERIPH4 */
#define TEGRA234_CLK_EXTPERIPH4			39U
/** @brief output of gate CLK_ENB_FUSE */
#define TEGRA234_CLK_FUSE			40U
/** @brief GPC2CLK-div-2 */
#define TEGRA234_CLK_GPCCLK			41U
/** @brief TODO */
#define TEGRA234_CLK_GPU_PWR			42U
/** output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_HDA2CODEC_2X */
#define TEGRA234_CLK_HDA			43U
/** output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_HDA2CODEC_2X */
#define TEGRA234_CLK_HDA2CODEC_2X		44U
/** @brief output of gate CLK_ENB_HDA2HDMICODEC */
#define TEGRA234_CLK_HDA2HDMICODEC		45U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_HOST1X */
#define TEGRA234_CLK_HOST1X			46U
/** @brief xusb_hs_hsicp_clk */
#define TEGRA234_CLK_XUSB_HS_HSICP		47U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_I2C1 */
#define TEGRA234_CLK_I2C1			48U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_I2C2 */
#define TEGRA234_CLK_I2C2			49U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_I2C3 */
#define TEGRA234_CLK_I2C3			50U
/** output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_I2C4 */
#define TEGRA234_CLK_I2C4			51U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_I2C6 */
#define TEGRA234_CLK_I2C6			52U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_I2C7 */
#define TEGRA234_CLK_I2C7			53U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_I2C8 */
#define TEGRA234_CLK_I2C8			54U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_I2C9 */
#define TEGRA234_CLK_I2C9			55U
/** output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_I2S1 */
#define TEGRA234_CLK_I2S1			56U
/** @brief clock recovered from I2S1 input */
#define TEGRA234_CLK_I2S1_SYNC_INPUT		57U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_I2S2 */
#define TEGRA234_CLK_I2S2			58U
/** @brief clock recovered from I2S2 input */
#define TEGRA234_CLK_I2S2_SYNC_INPUT		59U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_I2S3 */
#define TEGRA234_CLK_I2S3			60U
/** @brief clock recovered from I2S3 input */
#define TEGRA234_CLK_I2S3_SYNC_INPUT		61U
/** output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_I2S4 */
#define TEGRA234_CLK_I2S4			62U
/** @brief clock recovered from I2S4 input */
#define TEGRA234_CLK_I2S4_SYNC_INPUT		63U
/** output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_I2S5 */
#define TEGRA234_CLK_I2S5			64U
/** @brief clock recovered from I2S5 input */
#define TEGRA234_CLK_I2S5_SYNC_INPUT		65U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_I2S6 */
#define TEGRA234_CLK_I2S6			66U
/** @brief clock recovered from I2S6 input */
#define TEGRA234_CLK_I2S6_SYNC_INPUT		67U
/** @brief output of gate CLK_ENB_IQC1 */
#define TEGRA234_CLK_IQC1			68U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_ISP */
#define TEGRA234_CLK_ISP			69U
/**
 * @brief A fake clock which must be enabled during
 * KFUSE read operations to ensure adequate VDD_CORE voltage
 */
#define TEGRA234_CLK_KFUSE			70U
/** output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_MAUD */
#define TEGRA234_CLK_MAUD			71U
/** @brief output of gate CLK_ENB_MIPI_CAL */
#define TEGRA234_CLK_MIPI_CAL			72U
/** @brief output of the divider CLK_RST_CONTROLLER_CLK_SOURCE_MPHY_CORE_PLL_FIXED */
#define TEGRA234_CLK_MPHY_CORE_PLL_FIXED	73U
/** @brief output of gate CLK_ENB_MPHY_L0_RX_ANA */
#define TEGRA234_CLK_MPHY_L0_RX_ANA		74U
/** @brief output of gate CLK_ENB_MPHY_L0_RX_LS_BIT */
#define TEGRA234_CLK_MPHY_L0_RX_LS_BIT		75U
/** @brief output of divider-gate CLK_ENB_MPHY_L0_RX_SYMB */
#define TEGRA234_CLK_MPHY_L0_RX_SYMB		76U
/** @brief output of gate CLK_ENB_MPHY_L0_TX_LS_3XBIT */
#define TEGRA234_CLK_MPHY_L0_TX_LS_3XBIT	77U
/** @brief output of gate CLK_ENB_MPHY_L0_TX_SYMB */
#define TEGRA234_CLK_MPHY_L0_TX_SYMB		78U
/** @brief output of gate CLK_ENB_MPHY_L1_RX_ANA */
#define TEGRA234_CLK_MPHY_L1_RX_ANA		79U
/** @brief output of the divider CLK_RST_CONTROLLER_CLK_SOURCE_MPHY_TX_1MHZ_REF */
#define TEGRA234_CLK_MPHY_TX_1MHZ_REF		80U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_NVCSI */
#define TEGRA234_CLK_NVCSI			81U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_NVCSILP */
#define TEGRA234_CLK_NVCSILP			82U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_NVDEC */
#define TEGRA234_CLK_NVDEC			83U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_NVDISPLAYHUB switch divider output */
#define TEGRA234_CLK_NVDISPLAYHUB		84U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_NVDISPLAY_DISP switch divider output */
#define TEGRA234_CLK_NVDISPLAY_DISP		85U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_NVDISPLAY_P0 switch divider output */
#define TEGRA234_CLK_NVDISPLAY_P0		86U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_NVDISPLAY_P1 switch divider output */
#define TEGRA234_CLK_NVDISPLAY_P1		87U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_NVDISPLAY_P2 switch divider output */
#define TEGRA234_CLK_NVDISPLAY_P2		88U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_NVENC */
#define TEGRA234_CLK_NVENC			89U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_NVJPG */
#define TEGRA234_CLK_NVJPG			90U
/** @brief input from Tegra's XTAL_IN */
#define TEGRA234_CLK_OSC			91U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_AON_TOUCH switch divider output */
#define TEGRA234_CLK_AON_TOUCH			92U
/** PLL controlled by CLK_RST_CONTROLLER_PLLA_BASE for use by audio clocks */
#define TEGRA234_CLK_PLLA			93U
/** @brief PLL controlled by CLK_RST_CONTROLLER_PLLAON_BASE for use by IP blocks in the AON domain */
#define TEGRA234_CLK_PLLAON			94U
/** @brief PLLD */
#define TEGRA234_CLK_PLLD			95U
/** @brief PLLD2 */
#define TEGRA234_CLK_PLLD2			96U
/** @brief PLLD3 */
#define TEGRA234_CLK_PLLD3			97U
/** @brief PLLDP */
#define TEGRA234_CLK_PLLDP			98U
/** @brief PLLD4 */
#define TEGRA234_CLK_PLLD4			99U
/** Fixed 100MHz PLL for PCIe, SATA and superspeed USB */
#define TEGRA234_CLK_PLLE			100U
/** @brief PLLP vco output */
#define TEGRA234_CLK_PLLP			101U
/** @brief PLLP clk output */
#define TEGRA234_CLK_PLLP_OUT0			102U
/** Fixed frequency 960MHz PLL for USB and EAVB */
#define TEGRA234_CLK_UTMIPLL			103U
/** @brief output of the divider CLK_RST_CONTROLLER_PLLA_OUT */
#define TEGRA234_CLK_PLLA_OUT0			104U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_PWM1 */
#define TEGRA234_CLK_PWM1			105U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_PWM2 */
#define TEGRA234_CLK_PWM2			106U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_PWM3 */
#define TEGRA234_CLK_PWM3			107U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_PWM4 */
#define TEGRA234_CLK_PWM4			108U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_PWM5 */
#define TEGRA234_CLK_PWM5			109U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_PWM6 */
#define TEGRA234_CLK_PWM6			110U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_PWM7 */
#define TEGRA234_CLK_PWM7			111U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_PWM8 */
#define TEGRA234_CLK_PWM8			112U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_RCE_CPU_NIC output */
#define TEGRA234_CLK_RCE_CPU_NIC		113U
/** @brief CLK_RST_CONTROLLER_RCE_NIC_RATE divider output */
#define TEGRA234_CLK_RCE_NIC			114U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_SATA */
#define TEGRA234_CLK_SATA			115U
/** @brief output of gate CLK_ENB_SATA_OOB */
#define TEGRA234_CLK_SATA_OOB			116U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_AON_I2C_SLOW switch divider output */
#define TEGRA234_CLK_AON_I2C_SLOW		117U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_SCE_CPU_NIC */
#define TEGRA234_CLK_SCE_CPU_NIC		118U
/** @brief output of divider CLK_RST_CONTROLLER_SCE_NIC_RATE */
#define TEGRA234_CLK_SCE_NIC			119U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC1 */
#define TEGRA234_CLK_SDMMC1			120U
/** @brief Logical clk for setting the UPHY PLL3 rate */
#define TEGRA234_CLK_UPHY_PLL3			121U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC3 */
#define TEGRA234_CLK_SDMMC3			122U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC4 */
#define TEGRA234_CLK_SDMMC4			123U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_SE switch divider gated output */
#define TEGRA234_CLK_SE				124U
/** @brief output of mux controlled by SOR0_CLK_SEL1 and SOR0_CLK_SEL0 */
#define TEGRA234_CLK_SOR0_OUT			125U
/** @brief output of mux controlled by SOR0_CLK_SRC */
#define TEGRA234_CLK_SOR0_REF			126U
/** @brief SOR0 brick output which feeds into SOR0_CLK_SEL0 mux */
#define TEGRA234_CLK_SOR0_PAD_CLKOUT		127U
/** @brief output of mux controlled by SOR1_CLK_SEL1 and SOR1_CLK_SEL0 */
#define TEGRA234_CLK_SOR1_OUT			128U
/** @brief output of mux controlled by SOR1_CLK_SRC */
#define TEGRA234_CLK_SOR1_REF			129U
/** @brief SOR1 brick output which feeds into SOR1_CLK_SEL0 mux */
#define TEGRA234_CLK_SOR1_PAD_CLKOUT		130U
/** @brief output of gate CLK_ENB_SOR_SAFE */
#define TEGRA234_CLK_SOR_SAFE			131U
/** @brief Interface clock from IQC pad (1) */
#define TEGRA234_CLK_IQC1_IN			132U
/** @brief Interface clock from IQC pad (2) */
#define TEGRA234_CLK_IQC2_IN			133U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_DMIC5 */
#define TEGRA234_CLK_DMIC5			134U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_SPI1 */
#define TEGRA234_CLK_SPI1			135U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_SPI2 */
#define TEGRA234_CLK_SPI2			136U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_SPI3 */
#define TEGRA234_CLK_SPI3			137U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_I2C_SLOW */
#define TEGRA234_CLK_I2C_SLOW			138U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_AUDIO_SYNC_CLK_DMIC1 */
#define TEGRA234_CLK_SYNC_DMIC1			139U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_AUDIO_SYNC_CLK_DMIC2 */
#define TEGRA234_CLK_SYNC_DMIC2			140U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_AUDIO_SYNC_CLK_DMIC3 */
#define TEGRA234_CLK_SYNC_DMIC3			141U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_AUDIO_SYNC_CLK_DMIC4 */
#define TEGRA234_CLK_SYNC_DMIC4			142U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_AUDIO_SYNC_CLK_DSPK1 */
#define TEGRA234_CLK_SYNC_DSPK1			143U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_AUDIO_SYNC_CLK_DSPK2 */
#define TEGRA234_CLK_SYNC_DSPK2			144U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_AUDIO_SYNC_CLK_I2S1 */
#define TEGRA234_CLK_SYNC_I2S1			145U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_AUDIO_SYNC_CLK_I2S2 */
#define TEGRA234_CLK_SYNC_I2S2			146U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_AUDIO_SYNC_CLK_I2S3 */
#define TEGRA234_CLK_SYNC_I2S3			147U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_AUDIO_SYNC_CLK_I2S4 */
#define TEGRA234_CLK_SYNC_I2S4			148U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_AUDIO_SYNC_CLK_I2S5 */
#define TEGRA234_CLK_SYNC_I2S5			149U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_AUDIO_SYNC_CLK_I2S6 */
#define TEGRA234_CLK_SYNC_I2S6			150U
/** @brief controls MPHY_FORCE_LS_MODE upon enable & disable */
#define TEGRA234_CLK_MPHY_FORCE_LS_MODE		151U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_TACH0 */
#define TEGRA234_CLK_TACH0			152U
/** output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_TSEC */
#define TEGRA234_CLK_TSEC			153U
/** output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_PKA */
#define TEGRA234_CLK_TSEC_PKA			154U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_UARTA */
#define TEGRA234_CLK_UARTA			155U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_UARTB */
#define TEGRA234_CLK_UARTB			156U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_UARTC */
#define TEGRA234_CLK_UARTC			157U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_UARTD */
#define TEGRA234_CLK_UARTD			158U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_UARTE */
#define TEGRA234_CLK_UARTE			159U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_UARTF */
#define TEGRA234_CLK_UARTF			160U
/** @brief output of gate CLK_ENB_PEX1_CORE_6 */
#define TEGRA234_CLK_PEX1_CORE_6		161U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_UART_FST_MIPI_CAL */
#define TEGRA234_CLK_UART_FST_MIPI_CAL		162U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_UFSDEV_REF */
#define TEGRA234_CLK_UFSDEV_REF			163U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_UFSHC_CG_SYS */
#define TEGRA234_CLK_UFSHC			164U
/** @brief output of gate CLK_ENB_USB2_TRK */
#define TEGRA234_CLK_USB2_TRK			165U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_VI */
#define TEGRA234_CLK_VI				166U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_VIC */
#define TEGRA234_CLK_VIC			167U
/** @brief pva0_axi */
#define TEGRA234_CLK_PVA0_AXI			168U
/** @brief PVA0_vps0_clk */
#define TEGRA234_CLK_PVA0_VPS0			169U
/** @brief PVA0_vps1_clk */
#define TEGRA234_CLK_PVA0_VPS1			170U
/** @brief output of gate CLK_ENB_PEX2_CORE_7 */
#define TEGRA234_CLK_PEX2_CORE_7		171U
/** @brief output of gate CLK_ENB_PEX2_CORE_8 */
#define TEGRA234_CLK_PEX2_CORE_8		172U
/** @brief output of gate CLK_ENB_PEX2_CORE_9 */
#define TEGRA234_CLK_PEX2_CORE_9		173U
/** @brief dla0_falcon_clk */
#define TEGRA234_CLK_DLA0_FALCON		174U
/** @brief dla0_core_clk */
#define TEGRA234_CLK_DLA0_CORE			175U
/** @brief dla1_falcon_clk */
#define TEGRA234_CLK_DLA1_FALCON		176U
/** @brief dla1_core_clk */
#define TEGRA234_CLK_DLA1_CORE			177U
/** @brief output of mux controlled by SOR2_CLK_SEL1 and SOR2_CLK_SEL0 */
#define TEGRA234_CLK_SOR2_OUT			178U
/** @brief output of mux controlled by SOR2_CLK_SRC */
#define TEGRA234_CLK_SOR2_REF			179U
/** @brief SOR2 brick output which feeds into SOR2_CLK_SEL0 mux */
#define TEGRA234_CLK_SOR2_PAD_CLKOUT		180U
/** @brief output of mux controlled by SOR3_CLK_SEL1 and SOR3_CLK_SEL0 */
#define TEGRA234_CLK_SOR3_OUT			181U
/** @brief output of mux controlled by SOR3_CLK_SRC */
#define TEGRA234_CLK_SOR3_REF			182U
/** @brief SOR3 brick output which feeds into SOR3_CLK_SEL0 mux */
#define TEGRA234_CLK_SOR3_PAD_CLKOUT		183U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_NVDISPLAY_P3 switch divider output */
#define TEGRA234_CLK_NVDISPLAY_P3		184U
/** @brief output of gate CLK_ENB_PEX2_CORE_10 */
#define TEGRA234_CLK_PEX2_CORE_10		187U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_UARTI switch divider output (uarti_r_clk) */
#define TEGRA234_CLK_UARTI			188U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_UARTJ switch divider output (uartj_r_clk) */
#define TEGRA234_CLK_UARTJ			189U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_UARTH switch divider output */
#define TEGRA234_CLK_UARTH			190U
/** @brief ungated version of fuse clk */
#define TEGRA234_CLK_FUSE_SERIAL		191U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_QSPI0 switch divider output (qspi0_2x_pm_clk) */
#define TEGRA234_CLK_QSPI0_2X_PM		192U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_QSPI1 switch divider output (qspi1_2x_pm_clk) */
#define TEGRA234_CLK_QSPI1_2X_PM		193U
/** @brief output of the divider QSPI_CLK_DIV2_SEL in CLK_RST_CONTROLLER_CLK_SOURCE_QSPI0 (qspi0_pm_clk) */
#define TEGRA234_CLK_QSPI0_PM			194U
/** @brief output of the divider QSPI_CLK_DIV2_SEL in CLK_RST_CONTROLLER_CLK_SOURCE_QSPI1 (qspi1_pm_clk) */
#define TEGRA234_CLK_QSPI1_PM			195U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_VI_CONST switch divider output */
#define TEGRA234_CLK_VI_CONST			196U
/** @brief NAFLL clock source for BPMP */
#define TEGRA234_CLK_NAFLL_BPMP			197U
/** @brief NAFLL clock source for SCE */
#define TEGRA234_CLK_NAFLL_SCE			198U
/** @brief NAFLL clock source for NVDEC */
#define TEGRA234_CLK_NAFLL_NVDEC		199U
/** @brief NAFLL clock source for NVJPG */
#define TEGRA234_CLK_NAFLL_NVJPG		200U
/** @brief NAFLL clock source for TSEC */
#define TEGRA234_CLK_NAFLL_TSEC			201U
/** @brief NAFLL clock source for TSECB */
#define TEGRA234_CLK_NAFLL_TSECB		202U /* TODO: remove */
/** @brief NAFLL clock source for VI */
#define TEGRA234_CLK_NAFLL_VI			203U
/** @brief NAFLL clock source for SE */
#define TEGRA234_CLK_NAFLL_SE			204U
/** @brief NAFLL clock source for NVENC */
#define TEGRA234_CLK_NAFLL_NVENC		205U
/** @brief NAFLL clock source for ISP */
#define TEGRA234_CLK_NAFLL_ISP			206U
/** @brief NAFLL clock source for VIC */
#define TEGRA234_CLK_NAFLL_VIC			207U
/** @brief NAFLL clock source for NVDISPLAYHUB */
#define TEGRA234_CLK_NAFLL_NVDISPLAYHUB		208U /* TODO: remove */
/** @brief NAFLL clock source for AXICBB */
#define TEGRA234_CLK_NAFLL_AXICBB		209U
/** @brief NAFLL clock source for NVJPG1 */
#define TEGRA234_CLK_NAFLL_NVJPG1		210U
/** @brief NAFLL clock source for PVA_CORE */
#define TEGRA234_CLK_NAFLL_PVA_CORE		211U
/** @brief NAFLL clock source for PVA_VPS */
#define TEGRA234_CLK_NAFLL_PVA_VPS		212U
/** @brief NAFLL clock source for CVNAS */
#define TEGRA234_CLK_NAFLL_CVNAS		213U /* TODO: remove */
/** @brief NAFLL clock source for RCE */
#define TEGRA234_CLK_NAFLL_RCE			214U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_LA switch divider output (la_r_clk) */
#define TEGRA234_CLK_LA				215U

#define TEGRA234_CLK_UNUSED_216			216U

/** @brief NAFLL clock source for GPU */
#define TEGRA234_CLK_NAFLL_GPU			218U /* TODO: remove */
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC_LEGACY_TM switch divider output */
#define TEGRA234_CLK_SDMMC_LEGACY_TM		219U
/** @brief output of gate CLK_ENB_PEX0_CORE_0 */
#define TEGRA234_CLK_PEX0_CORE_0		220U
/** @brief output of gate CLK_ENB_PEX0_CORE_1 */
#define TEGRA234_CLK_PEX0_CORE_1		221U
/** @brief output of gate CLK_ENB_PEX0_CORE_2 */
#define TEGRA234_CLK_PEX0_CORE_2		222U
/** @brief output of gate CLK_ENB_PEX0_CORE_3 */
#define TEGRA234_CLK_PEX0_CORE_3		223U
/** @brief output of gate CLK_ENB_PEX0_CORE_4 */
#define TEGRA234_CLK_PEX0_CORE_4		224U
/** @brief output of gate CLK_ENB_PEX1_CORE_5 */
#define TEGRA234_CLK_PEX1_CORE_5		225U
/** @brief PCIE endpoint mode, HSIO UPHY PLL1 */
#define TEGRA234_CLK_PEX_REF1			226U
/** @brief PCIE endpoint mode, HSIO UPHY PLL2 */
#define TEGRA234_CLK_PEX_REF2			227U
/** @brief NVHS UPHY reference clock input */
#define TEGRA234_CLK_NVHS_REF			228U
/** @brief NVCSI_CIL clock for partition A */
#define TEGRA234_CLK_CSI_A			229U
/** @brief NVCSI_CIL clock for partition B */
#define TEGRA234_CLK_CSI_B			230U
/** @brief NVCSI_CIL clock for partition C */
#define TEGRA234_CLK_CSI_C			231U
/** @brief NVCSI_CIL clock for partition D */
#define TEGRA234_CLK_CSI_D			232U
/** @brief NVCSI_CIL clock for partition E */
#define TEGRA234_CLK_CSI_E			233U
/** @brief NVCSI_CIL clock for partition F */
#define TEGRA234_CLK_CSI_F			234U
/** @brief NVCSI_CIL clock for partition G */
#define TEGRA234_CLK_CSI_G			235U
/** @brief NVCSI_CIL clock for partition H */
#define TEGRA234_CLK_CSI_H			236U
/** @brief PLL controlled by CLK_RST_CONTROLLER_PLLC4_BASE */
#define TEGRA234_CLK_PLLC4			237U
/** @brief output of gate CLK_ENB_PLLC4_OUT */
#define TEGRA234_CLK_PLLC4_OUT			238U
/** @brief PLLC4 VCO followed by DIV3 path */
#define TEGRA234_CLK_PLLC4_OUT1			239U
/** @brief PLLC4 VCO followed by DIV5 path */
#define TEGRA234_CLK_PLLC4_OUT2			240U
/** @brief output of the mux controlled by PLLC4_CLK_SEL */
#define TEGRA234_CLK_PLLC4_MUXED		241U
/** @brief PLLC4 VCO followed by DIV2 path */
#define TEGRA234_CLK_PLLC4_VCO_DIV2		242U
/** @brief PLL controlled by CLK_RST_CONTROLLER_PLLNVHS_BASE */
#define TEGRA234_CLK_PLLNVHS			243U
/** @brief CSI pad brick input from partition A */
#define TEGRA234_CLK_CSI_A_PAD			244U
/** @brief CSI pad brick input from partition B */
#define TEGRA234_CLK_CSI_B_PAD			245U
/** @brief CSI pad brick input from partition C */
#define TEGRA234_CLK_CSI_C_PAD			246U
/** @brief CSI pad brick input from partition D */
#define TEGRA234_CLK_CSI_D_PAD			247U
/** @brief CSI pad brick input from partition E */
#define TEGRA234_CLK_CSI_E_PAD			248U
/** @brief CSI pad brick input from partition F */
#define TEGRA234_CLK_CSI_F_PAD			249U
/** @brief CSI pad brick input from partition G */
#define TEGRA234_CLK_CSI_G_PAD			250U
/** @brief CSI pad brick input from partition H */
#define TEGRA234_CLK_CSI_H_PAD			251U
/** @brief output of the gate CLK_ENB_SLVSEC */
#define TEGRA234_CLK_SLVSEC			252U
/** @brief output of the gate CLK_ENB_SLVSEC_PADCTRL */
#define TEGRA234_CLK_SLVSEC_PADCTRL		253U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_PEX_SATA_USB_RX_BYP switch divider output */
#define TEGRA234_CLK_PEX_SATA_USB_RX_BYP	254U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_PEX_USB_PAD_PLL0_MGMT switch divider output */
#define TEGRA234_CLK_PEX_USB_PAD_PLL0_MGMT	255U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_PEX_USB_PAD_PLL1_MGMT switch divider output */
#define TEGRA234_CLK_PEX_USB_PAD_PLL1_MGMT	256U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_PEX_USB_PAD_PLL2_MGMT switch divider output */
#define TEGRA234_CLK_PEX_USB_PAD_PLL2_MGMT	257U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_PEX_USB_PAD_PLL3_MGMT switch divider output */
#define TEGRA234_CLK_PEX_USB_PAD_PLL3_MGMT	258U
/** @brief CLK_RST_CONTROLLER_CLOCK_SOURCE_NVLINK_SYSCLK switch divider output */
#define TEGRA234_CLK_NVLINK_SYS			259U
/** @brief output ofthe gate CLK_ENB_RX_NVLINK */
#define TEGRA234_CLK_NVLINK_RX			260U
/** @brief output of the gate CLK_ENB_TX_NVLINK */
#define TEGRA234_CLK_NVLINK_TX			261U
/** @brief output of the fixed (DIV2) divider CLK_RST_CONTROLLER_NVLINK_TX_DIV_CLK_DIVISOR */
#define TEGRA234_CLK_NVLINK_TX_DIV		262U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_NVHS_RX_BYP switch divider output */
#define TEGRA234_CLK_NVHS_RX_BYP_REF		263U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_NVHS_PLL0_MGMT switch divider output */
#define TEGRA234_CLK_NVHS_PLL0_MGMT		264U
/** @brief xusb_core_dev_clk */
#define TEGRA234_CLK_XUSB_CORE_DEV		265U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_XUSB_CORE_HOST switch divider output  */
#define TEGRA234_CLK_XUSB_CORE_MUX		266U
/** @brief xusb_core_host_clk */
#define TEGRA234_CLK_XUSB_CORE_HOST		267U
/** @brief xusb_core_superspeed_clk */
#define TEGRA234_CLK_XUSB_CORE_SS		268U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_XUSB_FALCON switch divider output */
#define TEGRA234_CLK_XUSB_FALCON		269U
/** @brief xusb_falcon_host_clk */
#define TEGRA234_CLK_XUSB_FALCON_HOST		270U
/** @brief xusb_falcon_superspeed_clk */
#define TEGRA234_CLK_XUSB_FALCON_SS		271U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_XUSB_FS switch divider output */
#define TEGRA234_CLK_XUSB_FS			272U
/** @brief xusb_fs_host_clk */
#define TEGRA234_CLK_XUSB_FS_HOST		273U
/** @brief xusb_fs_dev_clk */
#define TEGRA234_CLK_XUSB_FS_DEV		274U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_XUSB_SS switch divider output */
#define TEGRA234_CLK_XUSB_SS			275U
/** @brief xusb_ss_dev_clk */
#define TEGRA234_CLK_XUSB_SS_DEV		276U
/** @brief xusb_ss_superspeed_clk */
#define TEGRA234_CLK_XUSB_SS_SUPERSPEED		277U
/** @brief HPLL for display hub clock */
#define TEGRA234_CLK_PLLDISPHUB			278U
/** @brief Invalid (do not use) */
#define TEGRA234_CLK_PLLDISPHUB_DIV		279U
/** @brief NAFLL clock source for CPU cluster 0 */
#define TEGRA234_CLK_NAFLL_CLUSTER0		280U
/** @brief NAFLL clock source for CPU cluster 1 */
#define TEGRA234_CLK_NAFLL_CLUSTER1		281U
/** @brief NAFLL clock source for CPU cluster 2 */
#define TEGRA234_CLK_NAFLL_CLUSTER2		282U
/** @brief NAFLL clock source for CPU cluster 3 */
#define TEGRA234_CLK_NAFLL_CLUSTER3		283U /* TODO: remove */
/** @brief CLK_RST_CONTROLLER_CAN1_CORE_RATE divider output */
#define TEGRA234_CLK_CAN1_CORE			284U
/** @brief CLK_RST_CONTROLLER_CAN2_CORE_RATE divider outputt */
#define TEGRA234_CLK_CAN2_CORE			285U
/** @brief CLK_RST_CONTROLLER_PLLA1_OUT1 switch divider output */
#define TEGRA234_CLK_PLLA1_OUT1			286U
/** @brief NVHS PLL hardware power sequencer (overrides 'manual' programming of PLL) */
#define TEGRA234_CLK_PLLNVHS_HPS		287U
/** @brief PLL controlled by CLK_RST_CONTROLLER_PLLREFE_BASE */
#define TEGRA234_CLK_PLLREFE_VCOOUT		288U
/** @brief 32K input clock provided by PMIC */
#define TEGRA234_CLK_CLK_32K			289U
/** @brief Clock recovered from SPDIFIN input */
#define TEGRA234_CLK_SPDIFIN_SYNC_INPUT		290U
/** @brief Fixed 48MHz clock divided down from utmipll */
#define TEGRA234_CLK_UTMIPLL_CLKOUT48		291U
/** @brief Fixed 480MHz clock divided down from utmipll */
#define TEGRA234_CLK_UTMIPLL_CLKOUT480		292U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_CVNAS switch divider output */
#define TEGRA234_CLK_CVNAS			293U
/** @brief PLL controlled by CLK_RST_CONTROLLER_PLLNVCSI_BASE  */
#define TEGRA234_CLK_PLLNVCSI			294U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_PVA0_CPU_AXI switch divider output */
#define TEGRA234_CLK_PVA0_CPU_AXI		295U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_PVA0_VPS switch divider output */
#define TEGRA234_CLK_PVA0_VPS			297U
/** @brief DLA0_CORE_NAFLL */
#define TEGRA234_CLK_NAFLL_DLA0_CORE		299U
/** @brief DLA0_FALCON_NAFLL */
#define TEGRA234_CLK_NAFLL_DLA0_FALCON		300U
/** @brief DLA1_CORE_NAFLL */
#define TEGRA234_CLK_NAFLL_DLA1_CORE		301U
/** @brief DLA1_FALCON_NAFLL */
#define TEGRA234_CLK_NAFLL_DLA1_FALCON		302U
/** @brief UTMI PLL HW power sequencer */
#define TEGRA234_CLK_UTMIPLL_HPS		304U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_I2C5 */
#define TEGRA234_CLK_I2C5			305U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_SE switch divider free running clk */
#define TEGRA234_CLK_FR_SE			306U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_BPMP_CPU_NIC switch divider output */
#define TEGRA234_CLK_BPMP_CPU_NIC		307U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_BPMP_APB switch divider output */
#define TEGRA234_CLK_BPMP_APB			308U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_TSC switch divider output */
#define TEGRA234_CLK_TSC			309U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_EMC switch divider output */
#define TEGRA234_CLK_EMCSA			310U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_EMCSB switch divider output */
#define TEGRA234_CLK_EMCSB			311U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_EMCSC switch divider output */
#define TEGRA234_CLK_EMCSC			312U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_EMCSD switch divider output */
#define TEGRA234_CLK_EMCSD			313U
/** @brief PLL controlled by CLK_RST_CONTROLLER_PLLC_BASE */
#define TEGRA234_CLK_PLLC			314U
/** @brief PLL controlled by CLK_RST_CONTROLLER_PLLC2_BASE */
#define TEGRA234_CLK_PLLC2			315U
/** @brief PLL controlled by CLK_RST_CONTROLLER_PLLC3_BASE */
#define TEGRA234_CLK_PLLC3			316U
/** @brief CLK_RST_CONTROLLER_TSC_HS_SUPER_CLK_DIVIDER skip divider output */
#define TEGRA234_CLK_TSC_REF			317U
/** @brief Dummy clock to ensure minimum SoC voltage for fuse burning */
#define TEGRA234_CLK_FUSE_BURN			318U
/** @brief Monitored branch of PEX0_CORE_0 clock */
#define TEGRA234_CLK_PEX0_CORE_0M		319U
/** @brief Monitored branch of PEX0_CORE_1 clock */
#define TEGRA234_CLK_PEX0_CORE_1M		320U
/** @brief Monitored branch of PEX0_CORE_2 clock */
#define TEGRA234_CLK_PEX0_CORE_2M		321U
/** @brief Monitored branch of PEX0_CORE_3 clock */
#define TEGRA234_CLK_PEX0_CORE_3M		322U
/** @brief Monitored branch of PEX0_CORE_4 clock */
#define TEGRA234_CLK_PEX0_CORE_4M		323U
/** @brief Monitored branch of PEX1_CORE_5 clock */
#define TEGRA234_CLK_PEX1_CORE_5M		324U
/** @brief NVHS UPHY PLL0-based NVLINK TX clock output */
#define TEGRA234_CLK_NVLINK_PLL_TXCLK		325U
/** @brief PLLE hardware power sequencer (overrides 'manual' programming of PLL) */
#define TEGRA234_CLK_PLLE_HPS			326U
/** @brief CLK_ENB_PLLREFE_OUT gate output */
#define TEGRA234_CLK_PLLREFE_VCOOUT_GATED	327U
/** @brief TEGRA234_CLK_SOR_SAFE clk source (PLLP_OUT0 divided by 17) */
#define TEGRA234_CLK_PLLP_DIV17			328U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_SOC_THERM switch divider output */
#define TEGRA234_CLK_SOC_THERM			329U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_TSENSOR switch divider output */
#define TEGRA234_CLK_TSENSOR			330U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_SEU1 switch divider free running clk */
#define TEGRA234_CLK_FR_SEU1			331U
/** @brief PLL controlled by CLK_RST_CONTROLLER_PLLBPMPCAM_BASE */
#define TEGRA234_CLK_PLLBPMPCAM			332U
/** @brief NAFLL clock source for OFA */
#define TEGRA234_CLK_NAFLL_OFA			333U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_OFA switch divider output */
#define TEGRA234_CLK_OFA			334U
/** @brief NAFLL clock source for SEU1 */
#define TEGRA234_CLK_NAFLL_SEU1			335U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_SEU1 switch divider gated output */
#define TEGRA234_CLK_SEU1			336U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_SPI4 */
#define TEGRA234_CLK_SPI4			337U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_SPI5 */
#define TEGRA234_CLK_SPI5			338U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_DCE_CPU_NIC */
#define TEGRA234_CLK_DCE_CPU_NIC		339U
/** @brief output of divider CLK_RST_CONTROLLER_DCE_NIC_RATE */
#define TEGRA234_CLK_DCE_NIC			340U
/** @brief NAFLL clock source for DCE */
#define TEGRA234_CLK_NAFLL_DCE			341U
/** @brief ungated version of TX symbol clock after fixed 1/2 divider */
#define TEGRA234_CLK_MPHY_L0_TX_DIV2		344U
/** @brief output of divider CLK_RST_CONTROLLER_CLK_SOURCE_MPHY_L0_TX_LS_SYMB */
#define TEGRA234_CLK_MPHY_L0_TX_SYMB_SRC	345U
/** @brief output of gate CLK_ENB_MPHY_L0_TX_2X_SYMB */
#define TEGRA234_CLK_MPHY_L0_TX_2X_SYMB		346U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_TACH1 */
#define TEGRA234_CLK_TACH1			365U
/** @brief GBE_UPHY_MGBES_APP_CLK switch divider gated output */
#define TEGRA234_CLK_MGBES_APP			366U
/** @brief Logical clk for setting GBE UPHY PLL2 TX_REF rate */
#define TEGRA234_CLK_UPHY_GBE_PLL2_TX_REF	367U
/** @brief Logical clk for setting GBE UPHY PLL2 XDIG rate */
#define TEGRA234_CLK_UPHY_GBE_PLL2_XDIG		368U
/** @brief clock recovered from MGBE0 lane RX input */
#define TEGRA234_CLK_MGBE0_RX_PCS_INPUT		369U
/** @brief clock recovered from MGBE1 lane RX input */
#define TEGRA234_CLK_MGBE1_RX_PCS_INPUT		370U
/** @brief clock recovered from MGBE2 lane RX input */
#define TEGRA234_CLK_MGBE2_RX_PCS_INPUT		371U
/** @brief clock recovered from MGBE3 lane RX input */
#define TEGRA234_CLK_MGBE3_RX_PCS_INPUT		372U
/** @brief output of mux controlled by GBE_UPHY_MGBE0_RX_PCS_CLK_SRC_SEL */
#define TEGRA234_CLK_MGBE0_RX_PCS		373U
/** @brief GBE_UPHY_MGBE0_TX_CLK divider gated ouput */
#define TEGRA234_CLK_MGBE0_TX			374U
/** @brief GBE_UPHY_MGBE0_TX_PCS_CLK divider gated ouput */
#define TEGRA234_CLK_MGBE0_TX_PCS		375U
/** @brief GBE_UPHY_MGBE0_MAC_CLK divider ouput */
#define TEGRA234_CLK_MGBE0_MAC_DIVIDER		376U
/** @brief GBE_UPHY_MGBE0_MAC_CLK gate ouput */
#define TEGRA234_CLK_MGBE0_MAC			377U
/** @brief GBE_UPHY_MGBE0_MACSEC_CLK gate ouput */
#define TEGRA234_CLK_MGBE0_MACSEC		378U
/** @brief GBE_UPHY_MGBE0_EEE_PCS_CLK gate ouput */
#define TEGRA234_CLK_MGBE0_EEE_PCS		379U
/** @brief GBE_UPHY_MGBE0_APP_CLK gate ouput */
#define TEGRA234_CLK_MGBE0_APP			380U
/** @brief GBE_UPHY_MGBE0_PTP_REF_CLK divider gated ouput */
#define TEGRA234_CLK_MGBE0_PTP_REF		381U
/** @brief output of mux controlled by GBE_UPHY_MGBE1_RX_PCS_CLK_SRC_SEL */
#define TEGRA234_CLK_MGBE1_RX_PCS		382U
/** @brief GBE_UPHY_MGBE1_TX_CLK divider gated ouput */
#define TEGRA234_CLK_MGBE1_TX			383U
/** @brief GBE_UPHY_MGBE1_TX_PCS_CLK divider gated ouput */
#define TEGRA234_CLK_MGBE1_TX_PCS		384U
/** @brief GBE_UPHY_MGBE1_MAC_CLK divider ouput */
#define TEGRA234_CLK_MGBE1_MAC_DIVIDER		385U
/** @brief GBE_UPHY_MGBE1_MAC_CLK gate ouput */
#define TEGRA234_CLK_MGBE1_MAC			386U
/** @brief GBE_UPHY_MGBE1_MACSEC_CLK gate ouput */
#define TEGRA234_CLK_MGBE1_MACSEC		387U
/** @brief GBE_UPHY_MGBE1_EEE_PCS_CLK gate ouput */
#define TEGRA234_CLK_MGBE1_EEE_PCS		388U
/** @brief GBE_UPHY_MGBE1_APP_CLK gate ouput */
#define TEGRA234_CLK_MGBE1_APP			389U
/** @brief GBE_UPHY_MGBE1_PTP_REF_CLK divider gated ouput */
#define TEGRA234_CLK_MGBE1_PTP_REF		390U
/** @brief output of mux controlled by GBE_UPHY_MGBE2_RX_PCS_CLK_SRC_SEL */
#define TEGRA234_CLK_MGBE2_RX_PCS		391U
/** @brief GBE_UPHY_MGBE2_TX_CLK divider gated ouput */
#define TEGRA234_CLK_MGBE2_TX			392U
/** @brief GBE_UPHY_MGBE2_TX_PCS_CLK divider gated ouput */
#define TEGRA234_CLK_MGBE2_TX_PCS		393U
/** @brief GBE_UPHY_MGBE2_MAC_CLK divider ouput */
#define TEGRA234_CLK_MGBE2_MAC_DIVIDER		394U
/** @brief GBE_UPHY_MGBE2_MAC_CLK gate ouput */
#define TEGRA234_CLK_MGBE2_MAC			395U
/** @brief GBE_UPHY_MGBE2_MACSEC_CLK gate ouput */
#define TEGRA234_CLK_MGBE2_MACSEC		396U
/** @brief GBE_UPHY_MGBE2_EEE_PCS_CLK gate ouput */
#define TEGRA234_CLK_MGBE2_EEE_PCS		397U
/** @brief GBE_UPHY_MGBE2_APP_CLK gate ouput */
#define TEGRA234_CLK_MGBE2_APP			398U
/** @brief GBE_UPHY_MGBE2_PTP_REF_CLK divider gated ouput */
#define TEGRA234_CLK_MGBE2_PTP_REF		399U
/** @brief output of mux controlled by GBE_UPHY_MGBE3_RX_PCS_CLK_SRC_SEL */
#define TEGRA234_CLK_MGBE3_RX_PCS		400U
/** @brief GBE_UPHY_MGBE3_TX_CLK divider gated ouput */
#define TEGRA234_CLK_MGBE3_TX			401U
/** @brief GBE_UPHY_MGBE3_TX_PCS_CLK divider gated ouput */
#define TEGRA234_CLK_MGBE3_TX_PCS		402U
/** @brief GBE_UPHY_MGBE3_MAC_CLK divider ouput */
#define TEGRA234_CLK_MGBE3_MAC_DIVIDER		403U
/** @brief GBE_UPHY_MGBE3_MAC_CLK gate ouput */
#define TEGRA234_CLK_MGBE3_MAC			404U
/** @brief GBE_UPHY_MGBE3_MACSEC_CLK gate ouput */
#define TEGRA234_CLK_MGBE3_MACSEC		405U
/** @brief GBE_UPHY_MGBE3_EEE_PCS_CLK gate ouput */
#define TEGRA234_CLK_MGBE3_EEE_PCS		406U
/** @brief GBE_UPHY_MGBE3_APP_CLK gate ouput */
#define TEGRA234_CLK_MGBE3_APP			407U
/** @brief GBE_UPHY_MGBE3_PTP_REF_CLK divider gated ouput */
#define TEGRA234_CLK_MGBE3_PTP_REF		408U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_GBE_RX_BYP switch divider output */
#define TEGRA234_CLK_GBE_RX_BYP_REF		409U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_GBE_PLL0_MGMT switch divider output */
#define TEGRA234_CLK_GBE_PLL0_MGMT		410U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_GBE_PLL1_MGMT switch divider output */
#define TEGRA234_CLK_GBE_PLL1_MGMT		411U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_GBE_PLL2_MGMT switch divider output */
#define TEGRA234_CLK_GBE_PLL2_MGMT		412U
/** @brief output of gate CLK_ENB_EQOS_MACSEC_RX */
#define TEGRA234_CLK_EQOS_MACSEC_RX		413U
/** @brief output of gate CLK_ENB_EQOS_MACSEC_TX */
#define TEGRA234_CLK_EQOS_MACSEC_TX		414U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_EQOS_TX_CLK divider ungated output */
#define TEGRA234_CLK_EQOS_TX_DIVIDER		415U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_NVHS_PLL1_MGMT switch divider output */
#define TEGRA234_CLK_NVHS_PLL1_MGMT		416U
/** @brief CLK_RST_CONTROLLER_CLK_SOURCE_EMCHUB mux output */
#define TEGRA234_CLK_EMCHUB			417U
/** @brief clock recovered from I2S7 input */
#define TEGRA234_CLK_I2S7_SYNC_INPUT		418U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_AUDIO_SYNC_CLK_I2S7 */
#define TEGRA234_CLK_SYNC_I2S7			419U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_I2S7 */
#define TEGRA234_CLK_I2S7			420U
/** @brief Monitored output of I2S7 pad macro mux */
#define TEGRA234_CLK_I2S7M			421U
/** @brief clock recovered from I2S8 input */
#define TEGRA234_CLK_I2S8_SYNC_INPUT		422U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_AUDIO_SYNC_CLK_I2S8 */
#define TEGRA234_CLK_SYNC_I2S8			423U
/** @brief output of mux controlled by CLK_RST_CONTROLLER_CLK_SOURCE_I2S8 */
#define TEGRA234_CLK_I2S8			424U
/** @brief Monitored output of I2S8 pad macro mux */
#define TEGRA234_CLK_I2S8M			425U
/** @brief NAFLL clock source for GPU GPC0 */
#define TEGRA234_CLK_NAFLL_GPC0			426U
/** @brief NAFLL clock source for GPU GPC1 */
#define TEGRA234_CLK_NAFLL_GPC1			427U
/** @brief NAFLL clock source for GPU SYSCLK */
#define TEGRA234_CLK_NAFLL_GPUSYS		428U
/** @brief NAFLL clock source for CPU cluster 0 DSUCLK */
#define TEGRA234_CLK_NAFLL_DSU0			429U
/** @brief NAFLL clock source for CPU cluster 1 DSUCLK */
#define TEGRA234_CLK_NAFLL_DSU1			430U
/** @brief NAFLL clock source for CPU cluster 2 DSUCLK */
#define TEGRA234_CLK_NAFLL_DSU2			431U
/** @brief output of gate CLK_ENB_SCE_CPU */
#define TEGRA234_CLK_SCE_CPU			432U
/** @brief output of gate CLK_ENB_RCE_CPU */
#define TEGRA234_CLK_RCE_CPU			433U
/** @brief output of gate CLK_ENB_DCE_CPU */
#define TEGRA234_CLK_DCE_CPU			434U

#define TEGRA234_MAX_PUBLIC_CLK_ID		434U

/** @} */

/* FIXME: alias to be removed after kernel DT is updated */
#define TEGRA234_CLK_TACH			TEGRA234_CLK_TACH0
#define TEGRA234_CLK_NAFLL_DLA_FALCON		TEGRA234_CLK_NAFLL_DLA0_FALCON
#define TEGRA234_CLK_NAFLL_DLA			TEGRA234_CLK_NAFLL_DLA0_CORE
#define TEGRA234_CLK_DLA0_CORE_MUX		TEGRA234_CLK_DLA0_CORE
#define TEGRA234_CLK_DLA0_FALCON_MUX		TEGRA234_CLK_DLA0_FALCON
#define TEGRA234_CLK_DLA1_CORE_MUX		TEGRA234_CLK_DLA1_CORE
#define TEGRA234_CLK_DLA1_FALCON_MUX		TEGRA234_CLK_DLA1_FALCON
#define TEGRA234_CLK_QSPI0			TEGRA234_CLK_QSPI0_2X_PM
#define TEGRA234_CLK_QSPI1			TEGRA234_CLK_QSPI1_2X_PM
#define TEGRA234_CLK_NAFLL_SE1			TEGRA234_CLK_NAFLL_SEU1
#define TEGRA234_CLK_SE1			TEGRA234_CLK_SEU1

#endif
