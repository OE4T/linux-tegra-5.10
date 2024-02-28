/* Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _NVI_DMP_MPU_H_
#define _NVI_DMP_MPU_H_

#define MPU_SMD_THLD_INIT		(0x000005DC)
#define MPU_SMD_DELAY_N_INIT		(0x0000006C)
#define MPU_SMD_TIMER_INIT		(0x00000258)
#define MPU_SMD_TIMER2_INIT		(0x000000C8)
#define MPU_SMD_EXE_STATE_INIT		(0x00000001)

#define FCFG_1				(1078)		/* 0x0436 */
#define FCFG_3				(1103)		/* 0x044F */
#define FCFG_2				(1082)		/* 0x043A */
#define FCFG_7				(1089)		/* 0x0441 */
#define CFG_OUT_PRESS			(1823)		/* 0x071F */
#define CFG_OUT_GYRO			(1755)		/* 0x06DB */
#define CFG_OUT_CPASS			(1789)		/* 0x06FD */
#define CFG_OUT_3QUAT			(1617)		/* 0x0651 */
#define CFG_OUT_6QUAT			(1652)		/* 0x0674 */
#define CFG_OUT_ACCL			(1721)		/* 0x06B9 */
#define CFG_OUT_STEPDET			(1587)		/* 0x0633 */
#define CFG_OUT_PQUAT			(1687)		/* 0x0697 */
#define CFG_PED_ENABLE			(1936)		/* 0x0790 */
#define CFG_PEDSTEP_DET			(2417)		/* 0x0971 */
#define CFG_FIFO_INT			(1934)		/* 0x078E */
#define CFG_AUTH			(1051)		/* 0x041B */
#define CFG_7				(1300)		/* 0x0514 */
#define CFG_PED_INT			(2406)		/* 0x0966 */
#define CFG_MOTION_BIAS			(1302)		/* 0x0516 */
#define OUT_6QUAT_DAT			(1662)		/* 0x067E */
#define OUT_ACCL_DAT			(1730)		/* 0x06C2 */
#define OUT_PRESS_DAT			(1832)		/* 0x0728 */
#define OUT_3QUAT_DAT			(1627)		/* 0x065B */
#define OUT_PQUAT_DAT			(1696)		/* 0x06A0 */
#define OUT_GYRO_DAT			(1764)		/* 0x06E4 */
#define OUT_CPASS_DAT			(1798)		/* 0x0706 */
#define SMD_TP2				(1265)		/* 0x04F1 */
#define SMD_TP1				(1244)		/* 0x04DC */

#define D_0_22				(22 + 512)
#define D_0_24				(24 + 512)

#define D_0_36				(36)
#define D_0_52				(52)
#define D_0_96				(96)
#define D_0_104				(104)
#define D_0_108				(108)
#define D_0_163				(163)
#define D_0_188				(188)
#define D_0_192				(192)
#define D_0_224				(224)
#define D_0_228				(228)
#define D_0_232				(232)
#define D_0_236				(236)

#define D_1_2				(256 + 2)	/* 0x0102 */
#define D_1_4				(256 + 4)	/* 0x0104 */
#define D_1_8				(256 + 8)	/* 0x0108 */
#define D_1_10				(256 + 10)	/* 0x010A */
#define D_1_24				(256 + 24)	/* 0x0118 */
#define D_1_28				(256 + 28)	/* 0x011C */
#define D_1_36				(256 + 36)	/* 0x0124 */
#define D_1_40				(256 + 40)	/* 0x0128 */
#define D_1_44				(256 + 44)	/* 0x012C */
#define D_1_72				(256 + 72)	/* 0x0148 */
#define D_1_74				(256 + 74)	/* 0x014A */
#define D_1_79				(256 + 79)	/* 0x014F */
#define D_1_88				(256 + 88)	/* 0x0158 */
#define D_1_90				(256 + 90)	/* 0x015A */
#define D_1_92				(256 + 92)	/* 0x015C */
#define D_1_96				(256 + 96)	/* 0x0160 */
#define D_1_98				(256 + 98)	/* 0x0162 */
#define D_1_106				(256 + 106)	/* 0x016A */
#define D_1_108				(256 + 108)	/* 0x016C */
#define D_1_112				(256 + 112)	/* 0x0170 */
#define D_1_128				(256 + 144)
#define D_1_152				(256 + 12)
#define D_1_160				(256 + 160)	/* 0x01A0 */
#define D_1_176				(256 + 176)	/* 0x01B0 */
#define D_1_178				(256 + 178)	/* 0x01B2 */
#define D_1_218				(256 + 218)	/* 0x01DA */
#define D_1_232				(256 + 232)	/* 0x01E8 */
#define D_1_236				(256 + 236)	/* 0x01EC */
#define D_1_240				(256 + 240)	/* 0x01F0 */
#define D_1_244				(256 + 244)	/* 0x01F4 */
#define D_1_250				(256 + 250)	/* 0x01FA */
#define D_1_252				(256 + 252)	/* 0x01FC */
#define D_2_12				(512 + 12)
#define D_2_96				(512 + 96)
#define D_2_108				(512 + 108)
#define D_2_208				(512 + 208)
#define D_2_224				(512 + 224)
#define D_2_236				(512 + 236)
#define D_2_244				(512 + 244)
#define D_2_248				(512 + 248)
#define D_2_252				(512 + 252)
#define CPASS_BIAS_X			(30 * 16 + 4)	/* 0x01E4 */
#define CPASS_BIAS_Y			(30 * 16 + 8)	/* 0x01E8 */
#define CPASS_BIAS_Z			(30 * 16 + 12)	/* 0x01EC */
#define CPASS_MTX_00			(32 * 16 + 4)	/* 0x0204 */
#define CPASS_MTX_01			(32 * 16 + 8)	/* 0x0208 */
#define CPASS_MTX_02			(36 * 16 + 12)	/* 0x024C */
#define CPASS_MTX_10			(33 * 16)	/* 0x0210 */
#define CPASS_MTX_11			(33 * 16 + 4)	/* 0x0214 */
#define CPASS_MTX_12			(33 * 16 + 8)	/* 0x0218 */
#define CPASS_MTX_20			(33 * 16 + 12)	/* 0x021C */
#define CPASS_MTX_21			(34 * 16 + 4)	/* 0x0224 */
#define CPASS_MTX_22			(34 * 16 + 8)	/* 0x0228 */
#define D_EXT_GYRO_BIAS_X		(61 * 16)	/* 0x03D0 */
#define D_EXT_GYRO_BIAS_Y		(61 * 16 + 4)	/* 0x03D4 */
#define D_EXT_GYRO_BIAS_Z		(61 * 16 + 8)	/* 0x03D8 */
#define D_ACT0				(40 * 16)	/* 0x0280 */
#define D_ACSX				(40 * 16 + 4)	/* 0x0284 */
#define D_ACSY				(40 * 16 + 8)	/* 0x0288 */
#define D_ACSZ				(40 * 16 + 12)	/* 0x028C */

#define FLICK_MSG			(45 * 16 + 4)	/* 0x02D4 */
#define FLICK_COUNTER			(45 * 16 + 8)	/* 0x02D8 */
#define FLICK_LOWER			(45 * 16 + 12)	/* 0x02DC */
#define FLICK_UPPER			(46 * 16 + 12)	/* 0x02EC */

#define D_SMD_ENABLE			(18 * 16)	/* 0x0120 */
#define D_SMD_MOT_THLD			(21 * 16 + 8)	/* 0x0158 */
#define D_SMD_DELAY_THLD		(21 * 16 + 4)	/* 0x0154 */
#define D_SMD_DELAY2_THLD		(21 * 16 + 12)	/* 0x015C */
#define D_SMD_EXE_STATE			(22 * 16)	/* 0x0160 */
#define D_SMD_DELAY_CNTR		(21 * 16)	/* 0x0150 */

#define D_WF_GESTURE_TIME_THRSH		(25 * 16 + 8)	/* 0x0198 */
#define D_WF_GESTURE_TILT_ERROR		(25 * 16 + 12)	/* 0x019C */
#define D_WF_GESTURE_TILT_THRSH		(26 * 16 + 8)	/* 0x01A8 */
#define D_WF_GESTURE_TILT_REJECT_THRSH	(26 * 16 + 12)	/* 0x01AC */

#define D_AUTH_OUT			(992)
#define D_AUTH_IN			(996)
#define D_AUTH_A			(1000)
#define D_AUTH_B			(1004)

#define D_PEDSTD_BP_B			(768 + 0x1C)	/* 0x031C */
#define D_PEDSTD_BP_A4			(768 + 0x40)	/* 0x0340 */
#define D_PEDSTD_BP_A3			(768 + 0x44)	/* 0x0344 */
#define D_PEDSTD_BP_A2			(768 + 0x48)	/* 0x0348 */
#define D_PEDSTD_BP_A1			(768 + 0x4C)	/* 0x034C */
#define D_PEDSTD_SB			(768 + 0x28)	/* 0x0328 */
#define D_PEDSTD_SB_TIME		(768 + 0x2C)	/* 0x032C */
#define D_PEDSTD_PEAKTHRSH		(768 + 0x98)	/* 0x0398 */
#define D_PEDSTD_TIML			(768 + 0x2A)	/* 0x032A */
#define D_PEDSTD_TIMH			(768 + 0x2E)	/* 0x032E */
#define D_PEDSTD_PEAK			(768 + 0x94)	/* 0x0394 */
#define D_PEDSTD_STEPCTR		(768 + 0x60)	/* 0x0360 */
#define D_PEDSTD_STEPCTR2		(58 * 16 + 8)	/* 0x03A8 */
#define D_PEDSTD_TIMECTR		(964)
#define D_PEDSTD_DECI			(768 + 0xA0)	/* 0x03A0 */
#define D_PEDSTD_SB2			(60 * 16 + 14)	/* 0x03CE */
#define D_STPDET_TIMESTAMP		(28 * 16 + 8)	/* 0x01C8 */
#define D_PEDSTD_DRIVE_STATE		(58)

#define D_HOST_NO_MOT			(976)
#define D_ACCEL_BIAS			(660)

#define D_BM_BATCH_CNTR			(27 * 16 + 4)	/* 0x01B4 */
#define D_BM_BATCH_THLD			(27 * 16 + 12)	/* 0x01BC */
#define D_BM_ENABLE			(28 * 16 + 6)	/* 0x01C6 */
#define D_BM_NUMWORD_TOFILL		(28 * 16 + 4)	/* 0x01C4 */

#define D_SO_DATA			(4 * 16 + 2)	/* 0x0042 */

#define D_P_HW_ID			(22 * 16 + 10)	/* 0x016A */
#define D_P_INIT			(22 * 16 + 2)	/* 0x0162 */

#define D_DMP_RUN_CNTR			(24 * 16)	/* 0x0180 */

#define D_ODR_S0			(45 * 16 + 8)	/* 0x02D8 */
#define D_ODR_S1			(45 * 16 + 12)	/* 0x02DC */
#define D_ODR_S2			(45 * 16 + 10)	/* 0x02DA */
#define D_ODR_S3			(45 * 16 + 14)	/* 0x02DE */
#define D_ODR_S4			(46 * 16 + 8)	/* 0x02E8 */
#define D_ODR_S5			(46 * 16 + 12)	/* 0x02EC */
#define D_ODR_S6			(46 * 16 + 10)	/* 0x02EA */
#define D_ODR_S7			(46 * 16 + 14)	/* 0x02EE */
#define D_ODR_S8			(42 * 16 + 8)	/* 0x02A8 */
#define D_ODR_S9			(42 * 16 + 12)	/* 0x02AC */

#define D_ODR_CNTR_S0			(45 * 16)	/* 0x02D0 */
#define D_ODR_CNTR_S1			(45 * 16 + 4)	/* 0x02D4 */
#define D_ODR_CNTR_S2			(45 * 16 + 2)	/* 0x02D2 */
#define D_ODR_CNTR_S3			(45 * 16 + 6)	/* 0x02D6 */
#define D_ODR_CNTR_S4			(46 * 16)	/* 0x02E0 */
#define D_ODR_CNTR_S5			(46 * 16 + 4)	/* 0x02E4 */
#define D_ODR_CNTR_S6			(46 * 16 + 2)	/* 0x02E2 */
#define D_ODR_CNTR_S7			(46 * 16 + 6)	/* 0x02E6 */
#define D_ODR_CNTR_S8			(42 * 16)	/* 0x02A0 */
#define D_ODR_CNTR_S9			(42 * 16 + 4)	/* 0x02A4 */

#define D_FS_LPQ0			(59 * 16)	/* 0x03B0 */
#define D_FS_LPQ1			(59 * 16 + 4)	/* 0x03B4 */
#define D_FS_LPQ2			(59 * 16 + 8)	/* 0x03B8 */
#define D_FS_LPQ3			(59 * 16 + 12)	/* 0x03BC */

#define D_FS_Q0				(12 * 16)	/* 0x00C0 */
#define D_FS_Q1				(12 * 16 + 4)	/* 0x00C4 */
#define D_FS_Q2				(12 * 16 + 8)	/* 0x00C8 */
#define D_FS_Q3				(12 * 16 + 12)	/* 0x00CC */

#define D_FS_9Q0			(2 * 16)	/* 0x0020 */
#define D_FS_9Q1			(2 * 16 + 4)	/* 0x0024 */
#define D_FS_9Q2			(2 * 16 + 8)	/* 0x0028 */
#define D_FS_9Q3			(2 * 16 + 12)	/* 0x002C */

#define KEY_CFG_OUT_ACCL		CFG_OUT_ACCL
#define KEY_CFG_OUT_GYRO		CFG_OUT_GYRO
#define KEY_CFG_OUT_3QUAT		CFG_OUT_3QUAT
#define KEY_CFG_OUT_6QUAT		CFG_OUT_6QUAT
#define KEY_CFG_OUT_PQUAT		CFG_OUT_PQUAT
#define KEY_CFG_PED_ENABLE		CFG_PED_ENABLE
#define KEY_CFG_FIFO_INT		CFG_FIFO_INT
#define KEY_CFG_AUTH			CFG_AUTH
#define KEY_FCFG_1			FCFG_1
#define KEY_FCFG_3			FCFG_3
#define KEY_FCFG_2			FCFG_2
#define KEY_FCFG_7			FCFG_7
#define KEY_CFG_7			CFG_7
#define KEY_CFG_MOTION_BIAS		CFG_MOTION_BIAS
#define KEY_CFG_PEDSTEP_DET		CFG_PEDSTEP_DET
#define KEY_D_0_22			D_0_22
#define KEY_D_0_96			D_0_96
#define KEY_D_0_104			D_0_104
#define KEY_D_0_108			D_0_108
#define KEY_D_1_36			D_1_36
#define KEY_D_1_40			D_1_40
#define KEY_D_1_44			D_1_44
#define KEY_D_1_72			D_1_72
#define KEY_D_1_74			D_1_74
#define KEY_D_1_79			D_1_79
#define KEY_D_1_88			D_1_88
#define KEY_D_1_90			D_1_90
#define KEY_D_1_92			D_1_92
#define KEY_D_1_160			D_1_160
#define KEY_D_1_176			D_1_176
#define KEY_D_1_218			D_1_218
#define KEY_D_1_232			D_1_232
#define KEY_D_1_250			D_1_250
#define KEY_DMP_SH_TH_Y			DMP_SH_TH_Y
#define KEY_DMP_SH_TH_X			DMP_SH_TH_X
#define KEY_DMP_SH_TH_Z			DMP_SH_TH_Z
#define KEY_DMP_ORIENT			DMP_ORIENT
#define KEY_D_AUTH_OUT			D_AUTH_OUT
#define KEY_D_AUTH_IN			D_AUTH_IN
#define KEY_D_AUTH_A			D_AUTH_A
#define KEY_D_AUTH_B			D_AUTH_B
#define KEY_CPASS_BIAS_X		CPASS_BIAS_X
#define KEY_CPASS_BIAS_Y		CPASS_BIAS_Y
#define KEY_CPASS_BIAS_Z		CPASS_BIAS_Z
#define KEY_CPASS_MTX_00		CPASS_MTX_00
#define KEY_CPASS_MTX_01		CPASS_MTX_01
#define KEY_CPASS_MTX_02		CPASS_MTX_02
#define KEY_CPASS_MTX_10		CPASS_MTX_10
#define KEY_CPASS_MTX_11		CPASS_MTX_11
#define KEY_CPASS_MTX_12		CPASS_MTX_12
#define KEY_CPASS_MTX_20		CPASS_MTX_20
#define KEY_CPASS_MTX_21		CPASS_MTX_21
#define KEY_CPASS_MTX_22		CPASS_MTX_22
#define KEY_D_ACT0			D_ACT0
#define KEY_D_ACSX			D_ACSX
#define KEY_D_ACSY			D_ACSY
#define KEY_D_ACSZ			D_ACSZ
#define KEY_D_PEDSTD_BP_B		D_PEDSTD_BP_B
#define KEY_D_PEDSTD_BP_A4		D_PEDSTD_BP_A4
#define KEY_D_PEDSTD_BP_A3		D_PEDSTD_BP_A3
#define KEY_D_PEDSTD_BP_A2		D_PEDSTD_BP_A2
#define KEY_D_PEDSTD_BP_A1		D_PEDSTD_BP_A1
#define KEY_D_PEDSTD_SB			D_PEDSTD_SB
#define KEY_D_PEDSTD_SB_TIME		D_PEDSTD_SB_TIME
#define KEY_D_PEDSTD_PEAKTHRSH		D_PEDSTD_PEAKTHRSH
#define KEY_D_PEDSTD_TIML		D_PEDSTD_TIML
#define KEY_D_PEDSTD_TIMH		D_PEDSTD_TIMH
#define KEY_D_PEDSTD_PEAK		D_PEDSTD_PEAK
#define KEY_D_PEDSTD_STEPCTR		D_PEDSTD_STEPCTR
#define KEY_D_PEDSTD_STEPCTR2		D_PEDSTD_STEPCTR2
#define KEY_D_PEDSTD_TIMECTR		D_PEDSTD_TIMECTR
#define KEY_D_PEDSTD_DECI		D_PEDSTD_DECI
#define KEY_D_PEDSTD_SB2		D_PEDSTD_SB2
#define KEY_D_PEDSTD_DRIVE_STATE	D_PEDSTD_DRIVE_STATE
#define KEY_D_STPDET_TIMESTAMP		D_STPDET_TIMESTAMP
#define KEY_D_HOST_NO_MOT		D_HOST_NO_MOT
#define KEY_D_ACCEL_BIAS		D_ACCEL_BIAS
#define KEY_CFG_EXT_GYRO_BIAS_X		D_EXT_GYRO_BIAS_X
#define KEY_CFG_EXT_GYRO_BIAS_Y		D_EXT_GYRO_BIAS_Y
#define KEY_CFG_EXT_GYRO_BIAS_Z		D_EXT_GYRO_BIAS_Z
#define KEY_CFG_PED_INT			CFG_PED_INT
#define KEY_SMD_ENABLE			D_SMD_ENABLE
#define KEY_SMD_ACCEL_THLD		D_SMD_MOT_THLD
#define KEY_SMD_DELAY_THLD		D_SMD_DELAY_THLD
#define KEY_SMD_DELAY2_THLD		D_SMD_DELAY2_THLD
#define KEY_SMD_ENABLE_TESTPT1		SMD_TP1
#define KEY_SMD_ENABLE_TESTPT2		SMD_TP2
#define KEY_SMD_EXE_STATE		D_SMD_EXE_STATE
#define KEY_SMD_DELAY_CNTR		D_SMD_DELAY_CNTR
#define KEY_BM_ENABLE			D_BM_ENABLE
#define KEY_BM_BATCH_CNTR		D_BM_BATCH_CNTR
#define KEY_BM_BATCH_THLD		D_BM_BATCH_THLD
#define KEY_BM_NUMWORD_TOFILL		D_BM_NUMWORD_TOFILL
#define KEY_SO_DATA			D_SO_DATA
#define KEY_P_INIT			D_P_INIT
#define KEY_CFG_OUT_CPASS		CFG_OUT_CPASS
#define KEY_CFG_OUT_PRESS		CFG_OUT_PRESS
#define KEY_CFG_OUT_STEPDET		CFG_OUT_STEPDET
#define KEY_CFG_3QUAT_ODR		D_ODR_S1
#define KEY_CFG_6QUAT_ODR		D_ODR_S2
#define KEY_CFG_PQUAT6_ODR		D_ODR_S3
#define KEY_CFG_ACCL_ODR		D_ODR_S4
#define KEY_CFG_GYRO_ODR		D_ODR_S5
#define KEY_CFG_CPASS_ODR		D_ODR_S6
#define KEY_CFG_PRESS_ODR		D_ODR_S7
#define KEY_CFG_9QUAT_ODR		D_ODR_S8
#define KEY_CFG_PQUAT9_ODR		D_ODR_S9
#define KEY_ODR_CNTR_3QUAT		D_ODR_CNTR_S1
#define KEY_ODR_CNTR_6QUAT		D_ODR_CNTR_S2
#define KEY_ODR_CNTR_PQUAT		D_ODR_CNTR_S3
#define KEY_ODR_CNTR_ACCL		D_ODR_CNTR_S4
#define KEY_ODR_CNTR_GYRO		D_ODR_CNTR_S5
#define KEY_ODR_CNTR_CPASS		D_ODR_CNTR_S6
#define KEY_ODR_CNTR_PRESS		D_ODR_CNTR_S7
#define KEY_ODR_CNTR_9QUAT		D_ODR_CNTR_S8
#define KEY_ODR_CNTR_PQUAT9		D_ODR_CNTR_S9
#define KEY_DMP_RUN_CNTR		D_DMP_RUN_CNTR
#define KEY_DMP_LPQ0			D_FS_LPQ0
#define KEY_DMP_LPQ1			D_FS_LPQ1
#define KEY_DMP_LPQ2			D_FS_LPQ2
#define KEY_DMP_LPQ3			D_FS_LPQ3
#define KEY_DMP_Q0			D_FS_Q0
#define KEY_DMP_Q1			D_FS_Q1
#define KEY_DMP_Q2			D_FS_Q2
#define KEY_DMP_Q3			D_FS_Q3
#define KEY_DMP_9Q0			D_FS_9Q0
#define KEY_DMP_9Q1			D_FS_9Q1
#define KEY_DMP_9Q2			D_FS_9Q2
#define KEY_DMP_9Q3			D_FS_9Q3
#define KEY_TEST_01			OUT_ACCL_DAT
#define KEY_TEST_02			OUT_GYRO_DAT
#define KEY_TEST_03			OUT_CPASS_DAT
#define KEY_TEST_04			OUT_PRESS_DAT
#define KEY_TEST_05			OUT_3QUAT_DAT
#define KEY_TEST_06			OUT_6QUAT_DAT
#define KEY_TEST_07			OUT_PQUAT_DAT

#endif /* _NVI_DMP_MPU_H_ */
