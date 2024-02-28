/*
 * dc_reg.h: tegra dc register definitions.
 *
 * Copyright (C) 2010 Google, Inc.
 * Author: Erik Gilling <konkers@android.com>
 *
 * Copyright (c) 2010-2018, NVIDIA CORPORATION, All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __DRIVERS_VIDEO_TEGRA_DC_DC_REG_H
#define __DRIVERS_VIDEO_TEGRA_DC_DC_REG_H

#define DC_CMD_GENERAL_INCR_SYNCPT		0x000
#define  VPULSE3_COND				(6 << 10)
#define IMMEDIATE_COND				(0 << 10)
#define DC_CMD_GENERAL_INCR_SYNCPT_CNTRL	0x001
#define DC_CMD_GENERAL_INCR_SYNCPT_ERROR	0x002
#define DC_CMD_WIN_A_INCR_SYNCPT		0x008
#define DC_CMD_WIN_A_INCR_SYNCPT_CNTRL		0x009
#define DC_CMD_WIN_A_INCR_SYNCPT_ERROR		0x00a
#define DC_CMD_WIN_B_INCR_SYNCPT		0x010
#define DC_CMD_WIN_B_INCR_SYNCPT_CNTRL		0x011
#define DC_CMD_WIN_B_INCR_SYNCPT_ERROR		0x012
#define DC_CMD_WIN_C_INCR_SYNCPT		0x018
#define DC_CMD_WIN_C_INCR_SYNCPT_CNTRL		0x019
#define DC_CMD_WIN_C_INCR_SYNCPT_ERROR		0x01a
#define DC_CMD_CONT_SYNCPT_VSYNC		0x028
#define DC_CMD_DISPLAY_COMMAND_OPTION0		0x031
#define  MSF_POLARITY_HIGH			(0 << 0)
#define  MSF_POLARITY_LOW			(1 << 0)
#define  MSF_DISABLE				(0 << 1)
#define  MSF_ENABLE				(1 << 1)
#define  MSF_LSPI				(0 << 2)
#define  MSF_LDC				(1 << 2)
#define  MSF_LSDI				(2 << 2)

#define DC_CMD_DISPLAY_COMMAND			0x032
#define  DISP_COMMAND_RAISE		(1 << 0)
#define  DISP_CTRL_MODE_STOP		(0 << 5)
#define  DISP_CTRL_MODE_C_DISPLAY	(1 << 5)
#define  DISP_CTRL_MODE_NC_DISPLAY	(2 << 5)
#define  DISP_COMMAND_RAISE_VECTOR(x)	(((x) & 0x1f) << 22)
#define  DISP_COMMAND_RAISE_CHANNEL_ID(x)	(((x) & 0xf) << 27)

#define DC_CMD_SIGNAL_RAISE			0x033
#define DC_CMD_DISPLAY_POWER_CONTROL		0x036
#define  PW0_ENABLE		(1 << 0)
#define  PW1_ENABLE		(1 << 2)
#define  PW2_ENABLE		(1 << 4)
#define  PW3_ENABLE		(1 << 6)
#define  PW4_ENABLE		(1 << 8)
#define  PM0_ENABLE		(1 << 16)
#define  PM1_ENABLE		(1 << 18)
#define  SPI_ENABLE		(1 << 24)
#define  HSPI_ENABLE		(1 << 25)

#define DC_CMD_INT_STATUS			0x037
#define DC_CMD_INT_MASK				0x038
#define DC_CMD_INT_ENABLE			0x039
#define DC_CMD_INT_TYPE				0x03a
#define DC_CMD_INT_POLARITY			0x03b
#define  CTXSW_INT		(1 << 0)
#define  FRAME_END_INT		(1 << 1)
#define  V_BLANK_INT		(1 << 2)
#define  H_BLANK_INT		(1 << 3)
#define  V_PULSE3_INT		(1 << 4)
#define  V_PULSE2_INT		(1 << 5)
#define  SPI_BUSY_INT		(1 << 7)
#define  WIN_A_UF_INT		(1 << 8)
#define  WIN_B_UF_INT		(1 << 9)
#define  WIN_C_UF_INT		(1 << 10)
#define  MSF_INT		(1 << 12)
#define  SSF_INT		(1 << 13)
#define  WIN_A_OF_INT		(1 << 14)
#define  WIN_B_OF_INT		(1 << 15)
#define  WIN_C_OF_INT		(1 << 16)
#define  GPIO_0_INT		(1 << 18)
#define  GPIO_1_INT		(1 << 19)
#define  GPIO_2_INT		(1 << 20)

#define  SMARTDIM_INT		(1 << 24) /* Used only on Nvdisplay */

#define  NVDISP_UF_INT		(1 << 23)
#define  HC_UF_INT		(1 << 23) /* Cursor or WinH */
#define  WIN_D_UF_INT		(1 << 24)
#define  WIN_T_UF_INT		(1 << 25)

#define DC_CMD_SIGNAL_RAISE1			0x03c
#define DC_CMD_SIGNAL_RAISE2			0x03d
#define DC_CMD_SIGNAL_RAISE3			0x03e
#define DC_CMD_STATE_ACCESS			0x040
#define  READ_MUX_ASSEMBLY	(0 << 0)
#define  READ_MUX_ACTIVE	(1 << 0)
#define  WRITE_MUX_ASSEMBLY	(0 << 2)
#define  WRITE_MUX_ACTIVE	(1 << 2)

#define DC_CMD_STATE_CONTROL			0x041
#define  GENERAL_ACT_REQ	(1 << 0)
#define  WIN_A_ACT_REQ		(1 << 1)
#define  WIN_B_ACT_REQ		(1 << 2)
#define  WIN_C_ACT_REQ		(1 << 3)
#define  WIN_D_ACT_REQ		(1 << 4)
#define  WIN_H_ACT_REQ		(1 << 5)
#define  CURSOR_ACT_REQ		(1 << 7)
#define  GENERAL_UPDATE		(1 << 8)
#define  WIN_A_UPDATE		(1 << 9)
#define  WIN_B_UPDATE		(1 << 10)
#define  WIN_C_UPDATE		(1 << 11)
#define  WIN_D_UPDATE		(1 << 12)
#define  WIN_H_UPDATE		(1 << 13)
#define  CURSOR_UPDATE		(1 << 15)
#define  COMMON_ACT_REQ		(1 << 16)
#define  COMMON_UPDATE		(1 << 17)
#define  NC_HOST_TRIG		(1 << 24)

#define DC_CMD_DISPLAY_WINDOW_HEADER		0x042
#define  WINDOW_A_SELECT		(1 << 4)
#define  WINDOW_B_SELECT		(1 << 5)
#define  WINDOW_C_SELECT		(1 << 6)
#define  WINDOW_D_SELECT		(1 << 7)
#define  WINDOW_H_SELECT		(1 << 8)

#define DC_CMD_REG_ACT_CONTROL			0x043
#define CURSOR_ACT_CNTR_SEL_V	0
#define CURSOR_ACT_CNTR_SEL_H	1
#define CURSOR_ACT_CNTR_SEL		7

#define  WIN_ACT_CNTR_SEL_HCOUNTER(x)	(1 << ((x) * 2 + 2))

#define DC_COM_CRC_CONTROL			0x300
#define  CRC_ALWAYS_ENABLE		(1 << 3)
#define  CRC_ALWAYS_DISABLE		(0 << 3)
#define  CRC_INPUT_DATA_ACTIVE_DATA	(1 << 2)
#define  CRC_INPUT_DATA_FULL_FRAME	(0 << 2)
#define  CRC_WAIT_TWO_VSYNC		(1 << 1)
#define  CRC_WAIT_ONE_VSYNC		(0 << 1)
#define  CRC_ENABLE_ENABLE		(1 << 0)
#define  CRC_ENABLE_DISABLE		(0 << 0)
#define DC_COM_CRC_CHECKSUM			0x301
#define DC_COM_PIN_OUTPUT_ENABLE0		0x302
#define DC_COM_PIN_OUTPUT_ENABLE1		0x303
#define DC_COM_PIN_OUTPUT_ENABLE2		0x304
#define DC_COM_PIN_OUTPUT_ENABLE3		0x305
#define  PIN_OUTPUT_LSPI_OUTPUT_EN		(1 << 8)
#define  PIN_OUTPUT_LSPI_OUTPUT_DIS		(1 << 8)
#define DC_COM_PIN_OUTPUT_POLARITY0		0x306

#define DC_COM_PIN_OUTPUT_POLARITY1		0x307
#define  LHS_OUTPUT_POLARITY_LOW	(1 << 30)
#define  LVS_OUTPUT_POLARITY_LOW	(1 << 28)
#define  LSC0_OUTPUT_POLARITY_LOW	(1 << 24)

#define DC_COM_PIN_OUTPUT_POLARITY2		0x308

#define DC_COM_PIN_OUTPUT_POLARITY3		0x309
#define  LSPI_OUTPUT_POLARITY_LOW	(1 << 8)

#define DC_COM_PIN_OUTPUT_DATA0			0x30a
#define DC_COM_PIN_OUTPUT_DATA1			0x30b
#define DC_COM_PIN_OUTPUT_DATA2			0x30c
#define DC_COM_PIN_OUTPUT_DATA3			0x30d
#define DC_COM_PIN_INPUT_ENABLE0		0x30e
#define DC_COM_PIN_INPUT_ENABLE1		0x30f
#define DC_COM_PIN_INPUT_ENABLE2		0x310
#define DC_COM_PIN_INPUT_ENABLE3		0x311
#define  PIN_INPUT_LSPI_INPUT_EN		(1 << 8)
#define  PIN_INPUT_LSPI_INPUT_DIS		(1 << 8)
#define DC_COM_PIN_INPUT_DATA0			0x312
#define DC_COM_PIN_INPUT_DATA1			0x313
#define DC_COM_PIN_OUTPUT_SELECT0		0x314
#define DC_COM_PIN_OUTPUT_SELECT1		0x315
#define DC_COM_PIN_OUTPUT_SELECT2		0x316
#define DC_COM_PIN_OUTPUT_SELECT3		0x317
#define DC_COM_PIN_OUTPUT_SELECT4		0x318
#define DC_COM_PIN_OUTPUT_SELECT5		0x319
#define DC_COM_PIN_OUTPUT_SELECT6		0x31a

#define PIN5_LM1_LCD_M1_OUTPUT_MASK	(7 << 4)
#define PIN5_LM1_LCD_M1_OUTPUT_M1	(0 << 4)
#define PIN5_LM1_LCD_M1_OUTPUT_LD21	(2 << 4)
#define PIN5_LM1_LCD_M1_OUTPUT_PM1	(3 << 4)

#define  PIN1_LHS_OUTPUT		(1 << 30)
#define  PIN1_LVS_OUTPUT		(1 << 28)

#define DC_COM_PIN_MISC_CONTROL			0x31b
#define DC_COM_PM0_CONTROL			0x31c
#define DC_COM_PM0_DUTY_CYCLE			0x31d
#define DC_COM_PM1_CONTROL			0x31e
#define DC_COM_PM1_DUTY_CYCLE			0x31f

#define PM_PERIOD_SHIFT                 18
#define PM_CLK_DIVIDER_SHIFT		4

#define DC_COM_SPI_CONTROL			0x320
#define DC_COM_SPI_START_BYTE			0x321
#define DC_COM_HSPI_WRITE_DATA_AB		0x322
#define DC_COM_HSPI_WRITE_DATA_CD		0x323
#define DC_COM_HSPI_CS_DC			0x324
#define DC_COM_SCRATCH_REGISTER_A		0x325
#define DC_COM_SCRATCH_REGISTER_B		0x326
#define DC_COM_GPIO_CTRL			0x327
#define DC_COM_GPIO_DEBOUNCE_COUNTER		0x328
#define DC_COM_CRC_CHECKSUM_LATCHED		0x329

#define DC_COM_CMU_CSC_KRR			0x32a
#define DC_COM_CMU_CSC_KGR			0x32b
#define DC_COM_CMU_CSC_KBR			0x32c
#define DC_COM_CMU_CSC_KRG			0x32d
#define DC_COM_CMU_CSC_KGG			0x32e
#define DC_COM_CMU_CSC_KBG			0x32f
#define DC_COM_CMU_CSC_KRB			0x330
#define DC_COM_CMU_CSC_KGB			0x331
#define DC_COM_CMU_CSC_KBB			0x332
#define DC_COM_CMU_LUT1				0x336
#define  LUT1_ADDR(x)			(((x) & 0x0ff) << 0)
#define  LUT1_DATA(x)			(((x) & 0xfff) << 16)
#define  LUT1_READ_DATA(x)		(((x) >> 16) & 0xfff)
#define DC_COM_CMU_LUT2				0x337
#define  LUT2_ADDR(x)			(((x) & 0x3ff) << 0)
#define  LUT2_DATA(x)			(((x) & 0x0ff) << 16)
#define  LUT2_READ_DATA(x)		(((x) >> 16) & 0x0ff)
#define DC_COM_CMU_LUT1_READ			0x338
#define  LUT1_READ_ADDR(x)		(((x) & 0x0ff) << 8)
#define  LUT1_READ_EN			(1 << 0)
#define DC_COM_CMU_LUT2_READ			0x339
#define  LUT2_READ_ADDR(x)		(((x) & 0x3ff) << 8)
#define  LUT2_READ_EN			(1 << 0)

#define DC_COM_DSC_TOP_CTL			0x33e
#define DSC_VALID_TIMEOUT_COUNT(x)	(((x) & 0xffff) << 4)
#define DSC_DUAL_ENABLE			(1U << 4)
#define DSC_AUTO_RESET			(1U << 3)
#define  DSC_SLCG_OVERRIDE		(1U << 2)
#define DSC_ENABLE			(1U << 1)
#define DSC_SOFT_RESET			0x1

#define DC_COM_DSC_DELAY			0x33f
#define DSC_VALID_WRAP_OUTPUT_DELAY(x)	(((x) & 0xffff) << 16)
#define DSC_VALID_OUTPUT_DELAY(x)	((x) & 0xffff)
#define DSC_ENC_FIFO_SIZE		0x17f
#define DSC_START_PIXEL_POS		18

#define DSC_COM_DSC_COMMON_CTRL			0x340
#define DSC_VALID_CHUNK_SIZE(x)		(((x) & 0xffff) << 16)
#define DSC_BLOCK_PRED_ENABLE		BIT(10)
#define DSC_VALID_BITS_PER_PIXEL(x)	((x) & 0x3ff)

#define DSC_COM_DSC_SLICE_INFO			0x341
#define DSC_VALID_SLICE_HEIGHT(x)	(((x) & 0xffff) << 16)
#define DSC_VALID_SLICE_WIDTH(x)	((x) & 0xffff)

#define DSC_COM_DSC_RC_RELAY_INFO		0x342
#define DSC_VALID_INITIAL_DEC_DELAY(x)		(((x) & 0xffff) << 16)
#define DSC_VALID_INITIAL_XMIT_DELAY(x)		((x) & 0x3ff)

#define DC_COM_DSC_RC_SCALE_INFO		0x343
#define DSC_VALID_SCALE_DECR_INTERVAL(x)	(((x) & 0xffff) << 6)
#define DSC_VALID_INITIAL_SCALE_VALUE(x)	((x) & 0x3f)

#define DC_COM_DSC_RC_SCALE_INFO_2		0x344
#define DSC_VALID_SCALE_INCR_INTERVAL(x)	((x) & 0xffff)

#define DC_COM_DSC_RC_BPGOFF_INFO		0x345
#define DSC_VALID_SLICE_BPG_OFFSET(x)	(((x) & 0xffff) << 16)
#define DSC_VALID_NFL_BPG_OFFSET(x)	((x) & 0xffff)

#define DC_COM_DSC_RC_OFFSET_INFO		0x346
#define DSC_DEF_8BPP_INITIAL_OFFSET		6144
#define DSC_DEF_12BPP_INITIAL_OFFSET		2048
#define DSC_VALID_FINAL_OFFSET(x)	(((x) & 0xffff) << 16)
#define DSC_VALID_INITIAL_OFFSET(x) 	((x) & 0xffff)

#define DC_COM_DSC_RC_FLATNESS_INFO		0x347
#define DSC_DEF_8BPP_FIRST_LINE_BPG_OFFS	12
#define DSC_DEF_12BPP_FIRST_LINE_BPG_OFFS	15
#define DSC_VALID_FIRST_LINE_BPG_OFFS(x)	(((x) & 0x1f) << 10)
#define DSC_VALID_FLATNESS_MAX_QP(x)		(((x) & 0x1f) << 5)
#define DSC_VALID_FLATNESS_MIN_QP(x)		((x) & 0x1f)

#define DC_COM_DSC_RC_PARAM_SET			0x348
#define DSC_VALID_RC_TGT_OFFSET_LO(x)		(((x) & 0xf) << 18)
#define DSC_VALID_RC_TGT_OFFSET_HI(x)		(((x) & 0xf) << 14)
#define DSC_VALID_RC_QUANT_INCR_LIMIT1(x)	(((x) & 0x1f) << 9)
#define DSC_VALID_RC_QUANT_INCR_LIMIT0(x)	(((x) & 0x1f) << 4)
#define DSC_VALID_RC_EDGE_FACTOR(x)		((x) & 0xf)

#define SET_DSC_RC_BUF_THRESH(thresh, shift)	\
	(((thresh) & 0xff) << shift)		\

#define DSC_VALID_RC_BUF_THRESH_3(x)	(((x) & 0xff) << 24)
#define DSC_VALID_RC_BUF_THRESH_2(x)	(((x) & 0xff) << 16)
#define DSC_VALID_RC_BUF_THRESH_1(x)	(((x) & 0xff) << 8)
#define DSC_VALID_RC_BUF_THRESH_0(x)	((x) & 0xff)

#define DC_COM_DSC_RC_BUF_THRESH_0		0x349
#define DSC_VALID_RC_MODEL_SIZE(x)	((x) & 0xffff)
#define DSC_DEF_RC_MODEL_SIZE			8192

#define DC_COM_DSC_RC_BUF_THRESH_1		0x34a

#define DC_COM_DSC_RC_BUF_THRESH_2		0x34b

#define DC_COM_DSC_RC_BUF_THRESH_3		0x34c

#define SET_RC_RANGE_MIN_QP(x)		((x) & 0x1f)
#define SET_RC_RANGE_MAX_QP(x)		(((x) & 0x1f) << 5)
#define SET_RC_RANGE_BPG_OFFSET(x)	(((x) & 0x3f) << 10)

#define DSC_VALID_RC_RANGE_PARAM_HI(x)	(((x) & 0xffff) << 16)
#define DSC_VALID_RC_RANGE_PARAM_LO(x)	((x) & 0xffff)

#define DC_COM_DSC_RC_RANGE_CFG_0		0x34d

#define DC_COM_DSC_RC_RANGE_CFG_1		0x34e

#define DC_COM_DSC_RC_RANGE_CFG_2		0x34f

#define DC_COM_DSC_RC_RANGE_CFG_3		0x350

#define DC_COM_DSC_RC_RANGE_CFG_4		0x351

#define DC_COM_DSC_RC_RANGE_CFG_5		0x352

#define DC_COM_DSC_RC_RANGE_CFG_6		0x353

#define DC_COM_DSC_RC_RANGE_CFG_7		0x354

#define DC_COM_DSC_UNIT_SET			0x355
#define DSC_FLATNESS_FIX_EN			BIT(7)
#define DSC_CHECK_FLATNESS2			BIT(6)
#define DSC_LINEBUF_DEPTH_8_BIT			BIT(2)
#define DSC_VALID_SLICE_NUM_MINUS1_IN_LINE(x)	((x) & 0x3)

#define DC_COM_DSC_CRC_CONTROL			0x356
#define DSC_CRC_ALWAYS				BIT(1)
#define DSC_CRC_ENABLE				BIT(0)

#define DC_COM_DSC_CRC_CHECKSUM			0x357

#define DC_COM_DSC_STATUS			0x358
#define DSC_VALID_STATUS_SLICEID(x)	(((x) >> 16) & 0x3)
#define DSC_VALID_STATUS_HINDEX(x)	((x) & 0xffff)

#define DC_COM_DSC_STATUS_2			0x359
#define DSC_STATUS_BUSY			BIT(16)
#define DSC_VALID_STATUS_VINDEX(x)	((x) & 0xffff)

#define DC_COM_RG_DPCA				0x366

#define DC_DISP_DISP_SIGNAL_OPTIONS0		0x400
#define  H_PULSE_0_ENABLE		(1 << 8)
#define  H_PULSE_1_ENABLE		(1 << 10)
#define  H_PULSE_2_ENABLE		(1 << 12)
#define  V_PULSE_0_ENABLE		(1 << 16)
#define  V_PULSE_1_ENABLE		(1 << 18)
#define  V_PULSE_2_ENABLE		(1 << 19)
#define  V_PULSE_3_ENABLE		(1 << 20)
#define  M0_ENABLE			(1 << 24)
#define  M1_ENABLE			(1 << 26)

#define DC_DISP_DISP_SIGNAL_OPTIONS1		0x401
#define  DI_ENABLE			(1 << 16)
#define  PP_ENABLE			(1 << 18)

#define DC_DISP_DISP_WIN_OPTIONS		0x402
#define  CURSOR_ENABLE			(1 << 16)
#define  SOR_ENABLE                     (1 << 25)
#define  SOR1_ENABLE			(1 << 26)
#define  SOR1_TIMING_CYA		(1 << 27)
#define  TVO_ENABLE			(1 << 28)
#define  DSI_ENABLE			(1 << 29)
#define  HDMI_ENABLE			(1 << 30)

#define DC_DISP_MEM_HIGH_PRIORITY		0x403
#define DC_DISP_MEM_HIGH_PRIORITY_TIMER		0x404
#define DC_DISP_DISP_TIMING_OPTIONS		0x405
#define  VSYNC_H_POSITION(x)		((x) & 0xfff)

#define DC_DISP_REF_TO_SYNC			0x406
#define DC_DISP_SYNC_WIDTH			0x407
#define DC_DISP_BACK_PORCH			0x408
#define DC_DISP_DISP_ACTIVE			0x409
#define DC_DISP_FRONT_PORCH			0x40a
#define DC_DISP_H_PULSE0_CONTROL		0x40b
#define DC_DISP_H_PULSE0_POSITION_A		0x40c
#define DC_DISP_H_PULSE0_POSITION_B		0x40d
#define DC_DISP_H_PULSE0_POSITION_C		0x40e
#define DC_DISP_H_PULSE0_POSITION_D		0x40f
#define DC_DISP_H_PULSE1_CONTROL		0x410
#define DC_DISP_H_PULSE1_POSITION_A		0x411
#define DC_DISP_H_PULSE1_POSITION_B		0x412
#define DC_DISP_H_PULSE1_POSITION_C		0x413
#define DC_DISP_H_PULSE1_POSITION_D		0x414
#define DC_DISP_H_PULSE2_CONTROL		0x415
#define DC_DISP_H_PULSE2_POSITION_A		0x416
#define DC_DISP_H_PULSE2_POSITION_B		0x417
#define DC_DISP_H_PULSE2_POSITION_C		0x418
#define DC_DISP_H_PULSE2_POSITION_D		0x419
#define DC_DISP_V_PULSE0_CONTROL		0x41a
#define DC_DISP_V_PULSE0_POSITION_A		0x41b
#define DC_DISP_V_PULSE0_POSITION_B		0x41c
#define DC_DISP_V_PULSE0_POSITION_C		0x41d
#define DC_DISP_V_PULSE1_CONTROL		0x41e
#define DC_DISP_V_PULSE1_POSITION_A		0x41f
#define DC_DISP_V_PULSE1_POSITION_B		0x420
#define DC_DISP_V_PULSE1_POSITION_C		0x421
#define DC_DISP_V_PULSE2_CONTROL		0x422
#define  V_PULSE2_H_POSITION(x)		(((x) & 0x1fff) << 16)
#define  V_PULSE2_LAST(x)		(((x) & 0x1) << 8)
#define DC_DISP_V_PULSE2_POSITION_A		0x423
#define  V_PULSE2_START_A(x)		(((x) & 0x1fff) << 0)
#define  V_PULSE2_END_A(x)		(((x) & 0x1fff) << 16)

#define DC_DISP_V_PULSE3_CONTROL		0x424
#define DC_DISP_V_PULSE3_POSITION_A		0x425
#define DC_DISP_M0_CONTROL			0x426
#define DC_DISP_M1_CONTROL			0x427
#define DC_DISP_DI_CONTROL			0x428
#define DC_DISP_PP_CONTROL			0x429
#define DC_DISP_PP_SELECT_A			0x42a
#define DC_DISP_PP_SELECT_B			0x42b
#define DC_DISP_PP_SELECT_C			0x42c
#define DC_DISP_PP_SELECT_D			0x42d

#define  PULSE_MODE_NORMAL		(0 << 3)
#define  PULSE_MODE_ONE_CLOCK		(1 << 3)
#define  PULSE_POLARITY_HIGH		(0 << 4)
#define  PULSE_POLARITY_LOW		(1 << 4)
#define  PULSE_QUAL_ALWAYS		(0 << 6)
#define  PULSE_QUAL_VACTIVE		(2 << 6)
#define  PULSE_QUAL_VACTIVE1		(3 << 6)
#define  PULSE_LAST_START_A		(0 << 8)
#define  PULSE_LAST_END_A		(1 << 8)
#define  PULSE_LAST_START_B		(2 << 8)
#define  PULSE_LAST_END_B		(3 << 8)
#define  PULSE_LAST_START_C		(4 << 8)
#define  PULSE_LAST_END_C		(5 << 8)
#define  PULSE_LAST_START_D		(6 << 8)
#define  PULSE_LAST_END_D		(7 << 8)

#define  PULSE_START(x)			((x) & 0xfff)
#define  PULSE_END(x)			(((x) & 0xfff) << 16)

#define DC_DISP_DISP_CLOCK_CONTROL		0x42e
#define  PIXEL_CLK_DIVIDER_PCD1		(0 << 8)
#define  PIXEL_CLK_DIVIDER_PCD1H	(1 << 8)
#define  PIXEL_CLK_DIVIDER_PCD2		(2 << 8)
#define  PIXEL_CLK_DIVIDER_PCD3		(3 << 8)
#define  PIXEL_CLK_DIVIDER_PCD4		(4 << 8)
#define  PIXEL_CLK_DIVIDER_PCD6		(5 << 8)
#define  PIXEL_CLK_DIVIDER_PCD8		(6 << 8)
#define  PIXEL_CLK_DIVIDER_PCD9		(7 << 8)
#define  PIXEL_CLK_DIVIDER_PCD12	(8 << 8)
#define  PIXEL_CLK_DIVIDER_PCD16	(9 << 8)
#define  PIXEL_CLK_DIVIDER_PCD18	(10 << 8)
#define  PIXEL_CLK_DIVIDER_PCD24	(11 << 8)
#define  PIXEL_CLK_DIVIDER_PCD13	(12 << 8)
#define  SHIFT_CLK_DIVIDER(x)		((x) & 0xff)

#define DC_DISP_DISP_INTERFACE_CONTROL		0x42f
#define  DISP_DATA_FORMAT_DF1P1C	(0 << 0)
#define  DISP_DATA_FORMAT_DF1P2C24B	(1 << 0)
#define  DISP_DATA_FORMAT_DF1P2C18B	(2 << 0)
#define  DISP_DATA_FORMAT_DF1P2C16B	(3 << 0)
#define  DISP_DATA_FORMAT_DF2S		(5 << 0)
#define  DISP_DATA_FORMAT_DF3S		(6 << 0)
#define  DISP_DATA_FORMAT_DFSPI		(7 << 0)
#define  DISP_DATA_FORMAT_DF1P3C24B	(8 << 0)
#define  DISP_DATA_FORMAT_DF1P3C18B	(9 << 0)
#define  DISP_DATA_ALIGNMENT_MSB	(0 << 8)
#define  DISP_DATA_ALIGNMENT_LSB	(1 << 8)
#define  DISP_DATA_ORDER_RED_BLUE	(0 << 9)
#define  DISP_DATA_ORDER_BLUE_RED	(1 << 9)

#define DC_DISP_DISP_COLOR_CONTROL		0x430
#define  BASE_COLOR_SIZE666		(0 << 0)
#define  BASE_COLOR_SIZE111		(1 << 0)
#define  BASE_COLOR_SIZE222		(2 << 0)
#define  BASE_COLOR_SIZE333		(3 << 0)
#define  BASE_COLOR_SIZE444		(4 << 0)
#define  BASE_COLOR_SIZE555		(5 << 0)
#define  BASE_COLOR_SIZE565		(6 << 0)
#define  BASE_COLOR_SIZE332		(7 << 0)
#define  BASE_COLOR_SIZE888		(8 << 0)
#define  DITHER_CONTROL_DISABLE		(0 << 8)
#define  DITHER_CONTROL_ORDERED		(2 << 8)

#ifdef CONFIG_TEGRA_DC_TEMPORAL_DITHER
#define  DITHER_CONTROL_TEMPORAL	(3 << 8)
#else
#define  DITHER_CONTROL_ERRDIFF		(3 << 8)
#endif

#define  CMU_DISABLE			(0 << 20)
#define  CMU_ENABLE			(1 << 20)

#define DC_DISP_SHIFT_CLOCK_OPTIONS		0x431
#define DC_DISP_DATA_ENABLE_OPTIONS		0x432
#define   DE_SELECT_ACTIVE_BLANK	0x0
#define   DE_SELECT_ACTIVE		0x1
#define   DE_SELECT_ACTIVE_IS		0x2
#define   DE_CONTROL_ONECLK		(0 << 2)
#define   DE_CONTROL_NORMAL		(1 << 2)
#define   DE_CONTROL_EARLY_EXT		(2 << 2)
#define   DE_CONTROL_EARLY		(3 << 2)
#define   DE_CONTROL_ACTIVE_BLANK	(4 << 2)

#define DC_DISP_SERIAL_INTERFACE_OPTIONS	0x433
#define DC_DISP_LCD_SPI_OPTIONS			0x434
#define DC_DISP_BORDER_COLOR			0x435
#define DC_DISP_COLOR_KEY0_LOWER		0x436
#define DC_DISP_COLOR_KEY0_UPPER		0x437
#define DC_DISP_COLOR_KEY1_LOWER		0x438
#define DC_DISP_COLOR_KEY1_UPPER		0x439

#define DC_DISP_CURSOR_FOREGROUND		0x43c
#define DC_DISP_CURSOR_BACKGROUND		0x43d
#define   CURSOR_COLOR(_r, _g, _b) ((_r) | ((_g) << 8) | ((_b) << 16))

#define DC_DISP_CURSOR_START_ADDR		0x43e
#define DC_DISP_CURSOR_START_ADDR_NS		0x43f
#define CURSOR_START_ADDR_MASK	(((1 << 22) - 1) << 10)
#define CURSOR_START_ADDR(_addr)	((_addr) >> 10)

#define	CURSOR_START_ADDR_LOW(_addr) ((_addr & 0xffffffff) >> 10)

#define   CURSOR_SIZE_32		(0x0 << 24)
#define   CURSOR_SIZE_64		(0x1 << 24)
#define   CURSOR_SIZE_128		(0x2 << 24)
#define   CURSOR_SIZE_256		(0x3 << 24)

#define DC_DISP_CURSOR_POSITION			0x440
#define H_CURSOR_POSITION_SIZE			14 /* For T21x */
#define   CURSOR_POSITION(_x, _y, h_size)		\
	(((_x) & ((1 << h_size) - 1)) |		\
	(((_y) & ((1 << h_size) - 1)) << 16))

#define DC_DISP_CURSOR_POSITION_NS		0x441
#define DC_DISP_INIT_SEQ_CONTROL		0x442
#define DC_DISP_SPI_INIT_SEQ_DATA_A		0x443
#define DC_DISP_SPI_INIT_SEQ_DATA_B		0x444
#define DC_DISP_SPI_INIT_SEQ_DATA_C		0x445
#define DC_DISP_SPI_INIT_SEQ_DATA_D		0x446

#define DC_DISP_DC_MCCIF_FIFOCTRL		0x480
#define DC_DISP_MCCIF_DISPLAY0A_HYST		0x481
#define DC_DISP_MCCIF_DISPLAY0B_HYST		0x482
#define DC_DISP_MCCIF_DISPLAY0C_HYST		0x483
#define DC_DISP_MCCIF_DISPLAY1B_HYST		0x484
#define DC_DISP_DAC_CRT_CTRL			0x4c0
#define DC_DISP_DISP_MISC_CONTROL		0x4c1
#define   UF_LINE_FLUSH                         (1 << 1)

#define	DC_DISP_BLEND_CURSOR_CONTROL		0x4f1
#define  WINH_CURS_SELECT(x)		(((x) & 0x1) << 28)
#define  CURSOR_COMP_MODE(x)		(((x) & 0x1) << 25)
#define  CURSOR_MODE_SELECT(x)		(((x) & 0x1) << 24)
#define  CURSOR_DST_BLEND_FACTOR_SELECT(x) ((x) << 16)
#define  CURSOR_SRC_BLEND_FACTOR_SELECT(x) ((x) << 8)
#define  CURSOR_ALPHA(a)		((a) & 0xff)
#define DC_DISP_CORE_HEAD_SET_CONTROL_CURSOR	0x43b
#define  CURSOR_FORMAT_T_A1R5G5B5		(233)
#define  CURSOR_FORMAT_T_A8R8G8B8		(207)
#define  SET_CONTROL_CURSOR_FORMAT(x)		(((x) & 0xFF) << 1)
#define  SET_CONTROL_CURSOR_FORMAT_MASK		(0xFF << 1)
#define  SET_CONTROL_CURSOR_DE_GAMMA(x)		(((x) & 0x3) << 27)
#define  SET_CONTROL_CURSOR_DE_GAMMA_MASK	(0x3 << 27)

#define DC_DISP_DISPLAY_SPARE0	0x4f7
#define DC_DISP_SPARE0_VALID_OVERFLOW_THRES(x)	(((x) & 0x3ff) << 4)
#define DC_DISP_DEF_OVERFLOW_THRES	0x354
#define DC_DISP_SPARE0_RC_SOLUTION_MODE(x)	((x) << 2)
#define DC_SPARE0_RC_SOLUTION_2	0x1
#define DC_SPARE0_RC_SOLUTION_3	0x2

#define DC_WIN_COLOR_PALETTE(x)			(0x500 + (x))

#define DC_DISP_INTERLACE_CONTROL		0x4e5
#define INTERLACE_MODE_ENABLE 1
#define INTERLACE_MODE_DISABLE 0
#define INTERLACE_START_FIELD_1 (0 << 1)
#define INTERLACE_START_FIELD_2 (1 << 1)
#define INTERLACE_STATUS_FIELD_1 (0 << 2)
#define INTERLACE_STATUS_FIELD_2 (1 << 2)

#define DC_DISP_INTERLACE_FIELD2_REF_TO_SYNC	0x4e6
#define DC_DISP_INTERLACE_FIELD2_SYNC_WIDTH		0x4e7
#define DC_DISP_INTERLACE_FIELD2_BACK_PORCH		0x4e8
#define DC_DISP_INTERLACE_FIELD2_FRONT_PORCH	0x4e9
#define DC_DISP_INTERLACE_FIELD2_DISP_ACTIVE	0x4ea

#define DC_DISP_CURSOR_START_ADDR_HI		0x4ec
#define DC_DISP_CURSOR_START_ADDR_HI_NS		0x4ed

#define DC_WIN_PALETTE_COLOR_EXT		0x600
#define DC_WIN_H_FILTER_P(x)			(0x601 + (x))
#define DC_WIN_CSC_YOF				0x611
#define DC_WIN_CSC_KYRGB			0x612
#define DC_WIN_CSC_KUR				0x613
#define DC_WIN_CSC_KVR				0x614
#define DC_WIN_CSC_KUG				0x615
#define DC_WIN_CSC_KVG				0x616
#define DC_WIN_CSC_KUB				0x617
#define DC_WIN_CSC_KVB				0x618
#define DC_WIN_V_FILTER_P(x)			(0x619 + (x))
#define DC_WIN_WIN_OPTIONS			0x700
#define  H_DIRECTION_DECREMENT(x)	((x) << 0)
#define  V_DIRECTION_DECREMENT(x)	((x) << 2)
#define  WIN_SCAN_COLUMN		(1 << 4)
#define  COLOR_EXPAND			(1 << 6)
#define  H_FILTER_ENABLE(x)		((x) << 8)
#define  V_FILTER_ENABLE(x)		((x) << 10)
#define  CP_ENABLE			(1 << 16)
#define  CSC_ENABLE			(1 << 18)
#define  DV_ENABLE			(1 << 20)
#define  INTERLACE_ENABLE	(1 << 23)
#define  INTERLACE_DISABLE  (0 << 23)
#define  WIN_ENABLE			(1 << 30)

#define DC_WIN_BYTE_SWAP			0x701
#define  BYTE_SWAP_NOSWAP		0
#define  BYTE_SWAP_SWAP2		1
#define  BYTE_SWAP_SWAP4		2
#define  BYTE_SWAP_SWAP4HW		3

#define DC_WIN_BUFFER_CONTROL			0x702
#define  BUFFER_CONTROL_HOST		0
#define  BUFFER_CONTROL_VI		1
#define  BUFFER_CONTROL_EPP		2
#define  BUFFER_CONTROL_MPEGE		3
#define  BUFFER_CONTROL_SB2D		4

#define DC_WIN_COLOR_DEPTH			0x703

#define DC_WIN_POSITION				0x704
#define  H_POSITION(x)		(((x) & 0x1fff) << 0)
#define  V_POSITION(x)		(((x) & 0x1fff) << 16)

#define DC_WIN_SIZE				0x705
#define  H_SIZE(x)		(((x) & 0x1fff) << 0)
#define  V_SIZE(x)		(((x) & 0x1fff) << 16)

#define DC_WIN_PRESCALED_SIZE			0x706
#define  H_PRESCALED_SIZE(x)	(((x) & 0x7fff) << 0)
#define  V_PRESCALED_SIZE(x)	(((x) & 0x1fff) << 16)

#define DC_WIN_H_INITIAL_DDA			0x707
#define DC_WIN_V_INITIAL_DDA			0x708
#define DC_WIN_DDA_INCREMENT			0x709
#define  H_DDA_INC(x)		(((x) & 0xffff) << 0)
#define  V_DDA_INC(x)		(((x) & 0xffff) << 16)

#define DC_WIN_LINE_STRIDE			0x70a
#define  LINE_STRIDE(x)		(x)
#define  UV_LINE_STRIDE(x)	(((x) & 0xffff) << 16)
#define  GET_LINE_STRIDE(x)	((x) & 0xffff)
#define  GET_UV_LINE_STRIDE(x)	(((x) >> 16) & 0xffff)

#define DC_WINBUF_BLEND_LAYER_CONTROL		0x716
#define  WIN_K1(x)			(((x) & 0xff) << 8)
#define  WIN_K2(x)			(((x) & 0xff) << 16)
#define  WIN_BLEND_ENABLE		(0 << 24)
#define  WIN_BLEND_BYPASS		(1 << 24)

#define DC_WINBUF_BLEND_MATCH_SELECT           0x717
#define  WIN_BLEND_FACT_SRC_COLOR_MATCH_SEL_ZERO		(0 << 0)
#define  WIN_BLEND_FACT_SRC_COLOR_MATCH_SEL_ONE			(1 << 0)
#define  WIN_BLEND_FACT_SRC_COLOR_MATCH_SEL_K1			(2 << 0)
#define  WIN_BLEND_FACT_SRC_COLOR_MATCH_SEL_K1_TIMES_DST	(3 << 0)
#define  WIN_BLEND_FACT_SRC_COLOR_MATCH_SEL_NEG_K1_TIMES_DST	(4 << 0)
#define  WIN_BLEND_FACT_SRC_COLOR_MATCH_SEL_K1_TIMES_SRC	(5 << 0)
#define  WIN_BLEND_FACT_DST_COLOR_MATCH_SEL_ZERO		(0 << 4)
#define  WIN_BLEND_FACT_DST_COLOR_MATCH_SEL_ONE			(1 << 4)
#define  WIN_BLEND_FACT_DST_COLOR_MATCH_SEL_K1			(2 << 4)
#define  WIN_BLEND_FACT_DST_COLOR_MATCH_SEL_K2			(3 << 4)
#define  WIN_BLEND_FACT_DST_COLOR_MATCH_SEL_K1_TIMES_DST	(4 << 4)
#define  WIN_BLEND_FACT_DST_COLOR_MATCH_SEL_NEG_K1_TIMES_DST	(5 << 4)
#define  WIN_BLEND_FACT_DST_COLOR_MATCH_SEL_NEG_K1_TIMES_SRC	(6 << 4)
#define  WIN_BLEND_FACT_DST_COLOR_MATCH_SEL_NEG_K1		(7 << 4)
#define  WIN_BLEND_FACT_SRC_ALPHA_MATCH_SEL_ZERO		(0 << 8)
#define  WIN_BLEND_FACT_SRC_ALPHA_MATCH_SEL_K1			(1 << 8)
#define  WIN_BLEND_FACT_SRC_ALPHA_MATCH_SEL_K2			(2 << 8)
#define  WIN_BLEND_FACT_DST_ALPHA_MATCH_SEL_ZERO		(0 << 12)
#define  WIN_BLEND_FACT_DST_ALPHA_MATCH_SEL_ONE			(1 << 12)
#define  WIN_BLEND_FACT_DST_ALPHA_MATCH_SEL_NEG_K1_TIMES_SRC	(2 << 12)
#define  WIN_BLEND_FACT_DST_ALPHA_MATCH_SEL_K2			(3 << 12)

#define DC_WINBUF_BLEND_ALPHA_1BIT		0x719
#define  WIN_ALPHA_1BIT_WEIGHT0(x)	(((x) & 0xff) << 0)
#define  WIN_ALPHA_1BIT_WEIGHT1(x)	(((x) & 0xff) << 8)

#define DC_WIN_BUF_STRIDE			0x70b
#define DC_WIN_UV_BUF_STRIDE			0x70c
#define DC_WIN_BUFFER_ADDR_MODE			0x70d
#define  DC_WIN_BUFFER_ADDR_MODE_LINEAR		(0 << 0)
#define  DC_WIN_BUFFER_ADDR_MODE_LINEAR_UV	(0 << 16)
#define  DC_WIN_BUFFER_ADDR_MODE_TILE		(1 << 0)
#define  DC_WIN_BUFFER_ADDR_MODE_TILE_UV	(1 << 16)

#define DC_WIN_DV_CONTROL			0x70e
#define DC_WIN_BLEND_NOKEY			0x70f
#define DC_WIN_BLEND_1WIN			0x710
#define DC_WIN_BLEND_2WIN_X			0x711
#define DC_WIN_BLEND_2WIN_Y			0x712
#define DC_WIN_BLEND_3WIN_XY			0x713
#define  CKEY_NOKEY			(0 << 0)
#define  CKEY_KEY0			(1 << 0)
#define  CKEY_KEY1			(2 << 0)
#define  CKEY_KEY01			(3 << 0)
#define  BLEND_CONTROL_FIX		(0 << 2)
#define  BLEND_CONTROL_ALPHA		(1 << 2)
#define  BLEND_CONTROL_DEPENDANT	(2 << 2)
#define  BLEND_CONTROL_PREMULT		(3 << 2)
#define  BLEND_WEIGHT0(x)		(((x) & 0xff) << 8)
#define  BLEND_WEIGHT1(x)		(((x) & 0xff) << 16)
#define  BLEND(key, control, weight0, weight1)			\
	  (CKEY_ ## key | BLEND_CONTROL_ ## control |		\
	   BLEND_WEIGHT0(weight0) | BLEND_WEIGHT1(weight1))

#define DC_WIN_BUFFER_SURFACE_KIND		0x80b
#define DC_WIN_BUFFER_SURFACE_PITCH		(0 << 0)
#define DC_WIN_BUFFER_SURFACE_TILED		(1 << 0)
#define DC_WIN_BUFFER_SURFACE_BL_16B2		(1 << 1)

#define  BLOCK_HEIGHT_SHIFT     4

#define DC_WIN_GLOBAL_ALPHA			0x715
#define  GLOBAL_ALPHA_ENABLE		0x10000

#define DC_WINBUF_START_ADDR			0x800
#define DC_WINBUF_START_ADDR_NS			0x801
#define DC_WINBUF_START_ADDR_U			0x802
#define DC_WINBUF_START_ADDR_U_NS		0x803
#define DC_WINBUF_START_ADDR_V			0x804
#define DC_WINBUF_START_ADDR_V_NS		0x805
#define DC_WINBUF_ADDR_H_OFFSET			0x806
#define DC_WINBUF_ADDR_H_OFFSET_NS		0x807
#define DC_WINBUF_ADDR_V_OFFSET			0x808
#define DC_WINBUF_ADDR_V_OFFSET_NS		0x809
#define DC_WINBUF_UFLOW_STATUS			0x80a

#define DC_WINBUF_START_ADDR_HI	0x80d
#define DC_WINBUF_START_ADDR_HI_U	0x80f
#define DC_WINBUF_START_ADDR_HI_V	0x811

#define DC_WINBUF_START_ADDR_FIELD2		0x813
#define DC_WINBUF_START_ADDR_FIELD2_U	0x815
#define DC_WINBUF_START_ADDR_FIELD2_V	0x817

#define DC_WINBUF_START_ADDR_FIELD2_HI	0x819
#define DC_WINBUF_START_ADDR_FIELD2_HI_U	0x81b
#define DC_WINBUF_START_ADDR_FIELD2_HI_V	0x81d

#define DC_WINBUF_ADDR_H_OFFSET_FIELD2	0x81f
#define DC_WINBUF_ADDR_V_OFFSET_FIELD2	0x821

/* direct versions of DC_WINBUF_UFLOW_STATUS */
#define DC_WINBUF_AD_UFLOW_STATUS		0xbca
#define DC_WINBUF_BD_UFLOW_STATUS		0xdca
#define DC_WINBUF_CD_UFLOW_STATUS		0xfca
#define DC_WINBUF_DD_UFLOW_STATUS		0x0ca
#define DC_WINBUF_HD_UFLOW_STATUS		0x1ca
#define DC_WINBUF_TD_UFLOW_STATUS		0x14a

#define DC_WINBUF_BLEND_LAYER_CONTROL		0x716
#define  WIN_DEPTH(x)			(((x) & 0xff) << 0)
#define  WIN_K1(x)			(((x) & 0xff) << 8)
#define  WIN_K2(x)			(((x) & 0xff) << 16)
#define  WIN_BLEND_ENABLE		(0 << 24)
#define  WIN_BLEND_BYPASS		(1 << 24)
#define  WIN_CKEY_SEL_NONE		(0 << 25)
#define  WIN_CKEY_SEL_WINA_KEY0		(1 << 25)
#define  WIN_CKEY_SEL_WINA_KEY1		(2 << 25)
#define  WIN_CKEY_SEL_WINB_KEY0		(3 << 25)
#define  WIN_CKEY_SEL_WINB_KEY1		(4 << 25)
#define  WIN_CKEY_SEL_WINC_KEY0		(5 << 25)
#define  WIN_CKEY_SEL_WINC_KEY1		(6 << 25)

#define DC_WINBUF_BLEND_MATCH_SELECT		0x717
#define  WIN_BLEND_FACT_SRC_COLOR_MATCH_SEL_ZERO \
					(0 << 0)
#define  WIN_BLEND_FACT_SRC_COLOR_MATCH_SEL_ONE \
					(1 << 0)
#define  WIN_BLEND_FACT_SRC_COLOR_MATCH_SEL_K1 \
					(2 << 0)
#define  WIN_BLEND_FACT_SRC_COLOR_MATCH_SEL_K1_TIMES_DST \
					(3 << 0)
#define  WIN_BLEND_FACT_SRC_COLOR_MATCH_SEL_NEG_K1_TIMES_DST \
					(4 << 0)
#define  WIN_BLEND_FACT_SRC_COLOR_MATCH_SEL_K1_TIMES_SRC \
					(5 << 0)
#define  WIN_BLEND_FACT_DST_COLOR_MATCH_SEL_ZERO \
					(0 << 4)
#define  WIN_BLEND_FACT_DST_COLOR_MATCH_SEL_ONE \
					(1 << 4)
#define  WIN_BLEND_FACT_DST_COLOR_MATCH_SEL_K1 \
					(2 << 4)
#define  WIN_BLEND_FACT_DST_COLOR_MATCH_SEL_K2 \
					(3 << 4)
#define  WIN_BLEND_FACT_DST_COLOR_MATCH_SEL_K1_TIMES_DST \
					(4 << 4)
#define  WIN_BLEND_FACT_DST_COLOR_MATCH_SEL_NEG_K1_TIMES_DST \
					(5 << 4)
#define  WIN_BLEND_FACT_DST_COLOR_MATCH_SEL_NEG_K1_TIMES_SRC \
					(6 << 4)
#define  WIN_BLEND_FACT_DST_COLOR_MATCH_SEL_NEG_K1 \
					(7 << 4)
#define  WIN_BLEND_FACT_SRC_ALPHA_MATCH_SEL_ZERO \
					(0 << 8)
#define  WIN_BLEND_FACT_SRC_ALPHA_MATCH_SEL_K1 \
					(1 << 8)
#define  WIN_BLEND_FACT_SRC_ALPHA_MATCH_SEL_K2 \
					(2 << 8)
#define  WIN_BLEND_FACT_DST_ALPHA_MATCH_SEL_ZERO \
					(0 << 12)
#define  WIN_BLEND_FACT_DST_ALPHA_MATCH_SEL_ONE \
					(1 << 12)
#define  WIN_BLEND_FACT_DST_ALPHA_MATCH_SEL_NEG_K1_TIMES_SRC \
					(2 << 12)
#define  WIN_BLEND_FACT_DST_ALPHA_MATCH_SEL_K2 \
					(3 << 12)

#define DC_WINBUF_BLEND_NOMATCH_SELECT		0x718
#define  WIN_BLEND_FACT_SRC_COLOR_NOMATCH_SEL_ZERO \
					(0 << 0)
#define  WIN_BLEND_FACT_SRC_COLOR_NOMATCH_SEL_ONE \
					(1 << 0)
#define  WIN_BLEND_FACT_SRC_COLOR_NOMATCH_SEL_K1 \
					(2 << 0)
#define  WIN_BLEND_FACT_SRC_COLOR_NOMATCH_SEL_K1_TIMES_DST \
					(3 << 0)
#define  WIN_BLEND_FACT_SRC_COLOR_NOMATCH_SEL_NEG_K1_TIMES_DST \
					(4 << 0)
#define  WIN_BLEND_FACT_SRC_COLOR_NOMATCH_SEL_K1_TIMES_SRC \
					(5 << 0)
#define  WIN_BLEND_FACT_DST_COLOR_NOMATCH_SEL_ZERO \
					(0 << 4)
#define  WIN_BLEND_FACT_DST_COLOR_NOMATCH_SEL_ONE \
					(1 << 4)
#define  WIN_BLEND_FACT_DST_COLOR_NOMATCH_SEL_K1 \
					(2 << 4)
#define  WIN_BLEND_FACT_DST_COLOR_NOMATCH_SEL_K2 \
					(3 << 4)
#define  WIN_BLEND_FACT_DST_COLOR_NOMATCH_SEL_K1_TIMES_DST \
					(4 << 4)
#define  WIN_BLEND_FACT_DST_COLOR_NOMATCH_SEL_NEG_K1_TIMES_DST \
					(5 << 4)
#define  WIN_BLEND_FACT_DST_COLOR_NOMATCH_SEL_NEG_K1_TIMES_SRC \
					(6 << 4)
#define  WIN_BLEND_FACT_DST_COLOR_NOMATCH_SEL_NEG_K1 \
					(7 << 4)
#define  WIN_BLEND_FACT_SRC_ALPHA_NOMATCH_SEL_ZERO \
					(0 << 8)
#define  WIN_BLEND_FACT_SRC_ALPHA_NOMATCH_SEL_K1 \
					(1 << 8)
#define  WIN_BLEND_FACT_SRC_ALPHA_NOMATCH_SEL_K2 \
					(2 << 8)
#define  WIN_BLEND_FACT_DST_ALPHA_NOMATCH_SEL_ZERO \
					(0 << 12)
#define  WIN_BLEND_FACT_DST_ALPHA_NOMATCH_SEL_ONE \
					(1 << 12)
#define  WIN_BLEND_FACT_DST_ALPHA_NOMATCH_SEL_NEG_K1_TIMES_SRC \
					(2 << 12)
#define  WIN_BLEND_FACT_DST_ALPHA_NOMATCH_SEL_K2 \
					(3 << 12)

#define DC_WINBUF_BLEND_ALPHA_1BIT		0x719
#define  WIN_ALPHA_1BIT_WEIGHT0(x)	(((x) & 0xff) << 0)
#define  WIN_ALPHA_1BIT_WEIGHT1(x)	(((x) & 0xff) << 8)

#define DC_WINBUF_MEMFETCH_CONTROL		0x82b
#define  MEMFETCH_CLK_GATE_OVERRIDE	(1U << 1)
#define  MEMFETCH_RESET			(1U << 0)

#define DC_WINBUF_CDE_CONTROL			0x82f
#define  CLEARRESPONSEONFRAMESTART	(1U << 13)
#define  CLEARREQUESTONFRAMESTART	(1U << 12)
#define  ENABLESURFACE0			(1U << 0)
#define DC_WINBUF_CDE_COMPTAG_BASE_0		0x830
#define DC_WINBUF_CDE_COMPTAG_BASEHI_0		0x832
#define DC_WINBUF_CDE_ZBC_COLOR_0		0x834
#define DC_WINBUF_CDE_SURFACE_OFFSET_0		0x835
#define DC_WINBUF_CDE_CTB_ENTRY_0		0x836
#define DC_WINBUF_CDE_CG_SW_OVR			0x837
#define DC_WINBUF_CDE_PM_CONTROL		0x838
#define DC_WINBUF_CDE_PM_COUNTER		0x839

#define DC_DISP_SD_CONTROL			0x4c2
#define  SD_ENABLE_NORMAL		(1 << 0)
#define  SD_ENABLE_ONESHOT		(2 << 0)
#define  SD_USE_VID_LUMA		(1 << 2)
#define  SD_BIN_WIDTH(x)		(((x) & 0x3) << 3)
#define  SD_BIN_WIDTH_VAL(val)		(((val) & (0x3 << 3)) >> 3)
#define  SD_BIN_WIDTH_ONE		(0 << 3)
#define  SD_BIN_WIDTH_TWO		(1 << 3)
#define  SD_BIN_WIDTH_FOUR		(2 << 3)
#define  SD_BIN_WIDTH_EIGHT		(3 << 3)
#define  SD_BIN_WIDTH_MASK		(3 << 3)
#define  SD_AGGRESSIVENESS(x)		(((x) & 0x7) << 5)
#define  SD_HW_UPDATE_DLY(x)		(((x) & 0x3) << 8)
#define  SD_ONESHOT_ENABLE		(1 << 10)
#define  SD_CORRECTION_MODE_AUTO	(0 << 11)
#define  SD_CORRECTION_MODE_MAN		(1 << 11)
#define  SD_K_LIMIT_ENABLE		(1 << 12)
#define  SD_WINDOW_ENABLE		(1 << 13)
#define  SD_SOFT_CLIPPING_ENABLE	(1 << 14)
#define  SD_SMOOTH_K_ENABLE		(1 << 15)
#define  SD_VSYNC			(0 << 28)
#define  SD_VPULSE2			(1 << 28)
#define  SD_BIAS0(x)			(((x) & 0x3) << 29)

#define NUM_BIN_WIDTHS 4
#define STEPS_PER_AGG_LVL 64
#define STEPS_PER_AGG_CHG_LOG2 5
#define STEPS_PER_AGG_CHG (1<<STEPS_PER_AGG_CHG_LOG2)
#define ADJ_PHASE_STEP 8
#define K_STEP 4

#define DC_DISP_SD_CSC_COEFF			0x4c3
#define  SD_CSC_COEFF_R(x)		(((x) & 0xf) << 4)
#define  SD_CSC_COEFF_G(x)		(((x) & 0xf) << 12)
#define  SD_CSC_COEFF_B(x)		(((x) & 0xf) << 20)

#define DC_DISP_SD_LUT(i)			(0x4c4 + i)
#define DC_DISP_SD_LUT_NUM			9
#define  SD_LUT_R(x)			(((x) & 0xff) << 0)
#define  SD_LUT_G(x)			(((x) & 0xff) << 8)
#define  SD_LUT_B(x)			(((x) & 0xff) << 16)

#define DC_DISP_SD_FLICKER_CONTROL		0x4cd
#define  SD_FC_TIME_LIMIT(x)		(((x) & 0xff) << 0)
#define  SD_FC_THRESHOLD(x)		(((x) & 0xff) << 8)

#define DC_DISP_SD_PIXEL_COUNT			0x4ce

#define DC_DISP_SD_HISTOGRAM(i)			(0x4cf + i)
#define DC_DISP_SD_HISTOGRAM_NUM		8
#define  SD_HISTOGRAM_BIN(val, offset)	((val >> offset) & 0xff)
#define  SD_HISTOGRAM_BIN_0(val)	(((val) & (0xff << 0)) >> 0)
#define  SD_HISTOGRAM_BIN_1(val)	(((val) & (0xff << 8)) >> 8)
#define  SD_HISTOGRAM_BIN_2(val)	(((val) & (0xff << 16)) >> 16)
#define  SD_HISTOGRAM_BIN_3(val)	(((val) & (0xff << 24)) >> 24)

#define DC_DISP_SD_BL_PARAMETERS		0x4d7
#define  SD_BLP_TIME_CONSTANT(x)	(((x) & 0x7ff) << 0)
#define  SD_BLP_STEP(x)			(((x) & 0xff) << 16)

#define DC_DISP_SD_BL_TF(i)			(0x4d8 + i)
#define DC_DISP_SD_BL_TF_NUM			4
#define  SD_BL_TF_POINT_0(x)		(((x) & 0xff) << 0)
#define  SD_BL_TF_POINT_1(x)		(((x) & 0xff) << 8)
#define  SD_BL_TF_POINT_2(x)		(((x) & 0xff) << 16)
#define  SD_BL_TF_POINT_3(x)		(((x) & 0xff) << 24)

#define DC_DISP_SD_BL_CONTROL			0x4dc
#define  SD_BLC_MODE_MAN		(0 << 0)
#define  SD_BLC_MODE_AUTO		(1 << 1)
#define  SD_BLC_BRIGHTNESS(val)		(((val) & (0xff << 8)) >> 8)
#define  BRIGHTNESS_THEORETICAL_MAX 0xffu

#define DC_DISP_SD_HW_K_VALUES			0x4dd
#define  SD_HW_K_R(val)			(((val) & (0x3ff << 0)) >> 0)
#define  SD_HW_K_G(val)			(((val) & (0x3ff << 10)) >> 10)
#define  SD_HW_K_B(val)			(((val) & (0x3ff << 20)) >> 20)

#define DC_DISP_SD_MAN_K_VALUES			0x4de
#define  SD_MAN_K_R(x)			(((x) & 0x3ff) << 0)
#define  SD_MAN_K_G(x)			(((x) & 0x3ff) << 10)
#define  SD_MAN_K_B(x)			(((x) & 0x3ff) << 20)

#define DC_DISP_SD_K_LIMIT			0x4df
#define  SD_K_LIMIT(x)			(((x) & 0x3ff) << 0)

#define DC_DISP_SD_WINDOW_POSITION		0x4e0
#define  SD_WIN_H_POSITION(x)		(((x) & 0x1fff) << 0)
#define  SD_WIN_V_POSITION(x)		(((x) & 0x1fff) << 16)

#define DC_DISP_SD_WINDOW_SIZE			0x4e1
#define  SD_WIN_H_SIZE(x)		(((x) & 0x1fff) << 0)
#define  SD_WIN_V_SIZE(x)		(((x) & 0x1fff) << 16)

#define DC_DISP_SD_SOFT_CLIPPING		0x4e2
#define  SD_SOFT_CLIPPING_THRESHOLD(x)	(((x) & 0xff) << 0)
#define  SD_SOFT_CLIPPING_RECIP(x)	(((x) & 0xffff) << 16)

#define DC_DISP_SD_SMOOTH_K			0x4e3
#define  SD_SMOOTH_K_INCR(x)		(((x) & 0x3fff) << 0)

#define  NUM_AGG_PRI_LVLS		4
#define  SD_AGG_PRI_LVL(x)		((x) >> 3)
#define  SD_GET_AGG(x)			((x) & 0x7)

#define DC_DISP_BLEND_BACKGROUND_COLOR		0x4e4

#define DC_DISP_DISPLAY_DBG_TIMING		0x4f6
#define  DBG_H_BLANK			(1 << 31)
#define  DBG_H_COUNT_SHIFT		(16)
#define  DBG_H_COUNT_MASK		(0x7fff << DBG_H_COUNT_SHIFT)
#define  DBG_V_BLANK			(1 << 15)
#define  DBG_V_COUNT_SHIFT		(0)
#define  DBG_V_COUNT_MASK		(0x7fff << DBG_V_COUNT_SHIFT)

#endif
