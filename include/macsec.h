/*
 * Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.
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

#ifndef MACSEC_H
#define MACSEC_H

#include <osi_core.h>

/**
 * @addtogroup MACsec AMAP
 *
 * @brief MACsec controller register offsets
 * @{
 */
#define GCM_KEYTABLE_CONFIG		0x0000
#define GCM_KEYTABLE_DATA(x)		(0x0004 + (x * 4))
#define RX_ICV_ERR_CNTRL		0x4000
#define INTERRUPT_COMMON_SR		0x4004
#define TX_IMR				0x4008
#define TX_ISR				0x400C
#ifdef NET24
#define RX_IMR				0x4040
#define RX_ISR				0x4044
#else
#define RX_IMR				0x4048
#define RX_ISR				0x404C
#endif /* NET24 */
#ifdef NET30
#define INTERRUPT_MASK1_0	0x40A0
#define TX_SC_PN_EXHAUSTED_STATUS0_0	0x4024
#define TX_SC_PN_EXHAUSTED_STATUS1_0	0x4028
#define TX_SC_PN_THRESHOLD_STATUS0_0	0x4018
#define TX_SC_PN_THRESHOLD_STATUS1_0	0x401C
#define TX_SC_ERROR_INTERRUPT_STATUS_0	0x402C
#define RX_SC_PN_EXHAUSTED_STATUS0_0	0x405C
#define RX_SC_PN_EXHAUSTED_STATUS1_0	0x4060
#define RX_SC_REPLAY_ERROR_STATUS0_0	0x4090
#define RX_SC_REPLAY_ERROR_STATUS1_0	0x4094
#endif
#define STATS_CONFIG			0x9000
#ifdef NET30
#define STATS_CONTROL_0		0x900C
#define TX_PKTS_UNTG_LO_0		0x9010
#define TX_PKTS_UNTG_HI_0		0x9014
#define TX_OCTETS_PRTCTD_LO_0		0x9018
#define TX_OCTETS_PRTCTD_HI_0		0x901C
#define TX_PKTS_TOO_LONG_LO_0		0x9020
#define TX_PKTS_TOO_LONG_HI_0		0x9024
#define TX_PKTS_PROTECTED_SCx_LO_0(x)	(0x9028 + (x * 8))
#define TX_PKTS_PROTECTED_SCx_HI_0(x)	(0x902C  + (x * 8))
#define RX_PKTS_NOTG_LO_0		0x90B0
#define RX_PKTS_NOTG_HI_0		0x90B4
#define RX_PKTS_UNTG_LO_0		0x90A8
#define RX_PKTS_UNTG_HI_0		0x90AC
#define RX_PKTS_BADTAG_LO_0		0x90B8
#define RX_PKTS_BADTAG_HI_0		0x90BC
#define RX_PKTS_NOSA_LO_0		0x90C0
#define RX_PKTS_NOSA_HI_0		0x90C4
#define RX_PKTS_NOSAERROR_LO_0		0x90C8
#define RX_PKTS_NOSAERROR_HI_0		0x90CC
#define RX_PKTS_OVRRUN_LO_0		0x90D0
#define RX_PKTS_OVRRUN_HI_0		0x90D4
#define RX_OCTETS_VLDTD_LO_0		0x90D8
#define RX_OCTETS_VLDTD_HI_0		0x90DC
#define RX_PKTS_LATE_SCx_LO_0(x)	(0x90E0 + (x * 8))
#define RX_PKTS_LATE_SCx_HI_0(x)	(0x90E4 + (x * 8))
#define RX_PKTS_NOTVALID_SCx_LO_0(x)	(0x9160 + (x * 8))
#define RX_PKTS_NOTVALID_SCx_HI_0(x)	(0x9164 + (x * 8))
#define RX_PKTS_OK_SCx_LO_0(x)		(0x91E0 + (x * 8))
#define RX_PKTS_OK_SCx_HI_0(x)		(0x91E4 + (x * 8))

#define TX_INPKTS_CRCIN_NOTVALID_LO_0	0x9260
#define TX_INPKTS_CRCIN_NOTVALID_HI_0	0x9264
#define RX_INPKTS_CRCIN_NOTVALID_LO_0	0x9268
#define RX_INPKTS_CRCIN_NOTVALID_HI_0	0x926C


#endif
#define MACSEC_CONTROL0			0xD000
#define MACSEC_LUT_CONFIG		0xD004
#define MACSEC_LUT_DATA(x)		(0xD008 + (x * 4))
#ifndef NET24
#define TX_BYP_LUT_VALID		0xD024
#define TX_SCI_LUT_VALID		0xD028
#define RX_BYP_LUT_VALID		0xD02C
#define RX_SCI_LUT_VALID		0xD030
#endif /* NET24 */
#ifdef NET24
#define COMMON_IMR			0xD024
#define COMMON_ISR			0xD028
#else
#define COMMON_IMR			0xD054
#define COMMON_ISR			0xD058
#endif /* NET24 */
#ifdef NET30
#define TX_SC_KEY_INVALID_STS0_0	0XD064
#define TX_SC_KEY_INVALID_STS1_0	0XD068
#define RX_SC_KEY_INVALID_STS0_0	0XD080
#define RX_SC_KEY_INVALID_STS1_0	0XD084

#define TX_DEBUG_CONTROL_0		0xD098
#define TX_DEBUG_TRIGGER_EN_0		0xD09C
#define TX_DEBUG_STATUS_0		0xD0C4
#define DEBUG_BUF_CONFIG_0		0xD0C8
#define DEBUG_BUF_DATA_0(x)		(0xD0CC + (x * 4))
#define RX_DEBUG_CONTROL_0		0xD0DC
#define RX_DEBUG_TRIGGER_EN_0		0xD0E0
#define RX_DEBUG_STATUS_0		0xD0F8

#endif /* NET30 */
#define MACSEC_CONTROL1			0xE000
#define GCM_AES_CNTRL			0xE004
#define TX_MTU_LEN			0xE008
#define TX_SOT_DELAY			0xE010
#define RX_MTU_LEN			0xE014
#define RX_SOT_DELAY			0xE01C
#ifdef NET30
#define MACSEC_TX_DVLAN_CONTROL_0	0xE00C
#define MACSEC_RX_DVLAN_CONTROL_0	0xE018
#endif
/** @} */

/**
 * @addtogroup GCM_KEYTABLE_CONFIG register
 *
 * @brief Bit definitions of GCM_KEYTABLE_CONFIG register
 * @{
 */
#define KT_CONFIG_UPDATE		OSI_BIT(31)
#define KT_CONFIG_CTLR_SEL		OSI_BIT(25)
#define KT_CONFIG_RW			OSI_BIT(24)
#define KT_CONFIG_INDEX_MASK		(OSI_BIT(4) | OSI_BIT(3) | OSI_BIT(2) |\
					 OSI_BIT(1) | OSI_BIT(0))
#define KT_ENTRY_VALID			OSI_BIT(0)
/** @} */

/**
 * @addtogroup GCM_KEYTABLE_DATA registers
 *
 * @brief Bit definitions of GCM_KEYTABLE_DATA register & helpful macros
 * @{
 */
#define KT_ENTRY_VALID			OSI_BIT(0)
#define MACSEC_KT_DATA_REG_CNT		13
#define MACSEC_KT_DATA_REG_SAK_CNT	8
#define MACSEC_KT_DATA_REG_H_CNT	4
/** @} */

/**
 * @addtogroup MACSEC_LUT_CONFIG register
 *
 * @brief Bit definitions of MACSEC_LUT_CONFIG register
 * @{
 */
#define LUT_CONFIG_UPDATE		OSI_BIT(31)
#define LUT_CONFIG_CTLR_SEL		OSI_BIT(25)
#define LUT_CONFIG_RW			OSI_BIT(24)
#define LUT_CONFIG_LUT_SEL_MASK		(OSI_BIT(18) | OSI_BIT(17) |\
					 OSI_BIT(16))
#define LUT_CONFIG_LUT_SEL_SHIFT	16
#define LUT_CONFIG_INDEX_MASK		(OSI_BIT(4) | OSI_BIT(3) | OSI_BIT(2) |\
					 OSI_BIT(1) | OSI_BIT(0))
/** @} */
/**
 * @addtogroup INTERRUPT_COMMON_STATUS register
 *
 * @brief Bit definitions of MACSEC_INTERRUPT_COMMON_STATUS register
 * @{
 */
#ifdef NET30
#define COMMON_SR_SFTY_ERR		OSI_BIT(2)
#endif
#define COMMON_SR_RX			OSI_BIT(1)
#define COMMON_SR_TX			OSI_BIT(0)
/** @} */

/**
 * @addtogroup MACSEC_CONTROL0 register
 *
 * @brief Bit definitions of MACSEC_CONTROL0 register
 * @{
 */
#define TX_LKUP_MISS_NS_INTR		OSI_BIT(24)
#define RX_LKUP_MISS_NS_INTR		OSI_BIT(23)
#define VALIDATE_FRAMES_MASK		(OSI_BIT(22) | OSI_BIT(21))
#define VALIDATE_FRAMES_DIS		0x0
#define VALIDATE_FRAMES_STRICT		OSI_BIT(22)
#define VALIDATE_FRAMES_CHECK		OSI_BIT(21)
#define RX_REPLAY_PROT_EN		OSI_BIT(20)
#define RX_LKUP_MISS_BYPASS		OSI_BIT(19)
#ifndef NET30
#define RX_SCI_LUT_EN			OSI_BIT(18)
#define RX_BYP_LUT_EN			OSI_BIT(17)
#endif
#define RX_EN				OSI_BIT(16)
#define TX_LKUP_MISS_BYPASS		OSI_BIT(3)
#ifndef NET30
#define TX_SCI_LUT_EN			OSI_BIT(2)
#define TX_BYP_LUT_EN			OSI_BIT(1)
#endif
#define TX_EN				OSI_BIT(0)
/** @} */

/**
 * @addtogroup MACSEC_CONTROL1 register
 *
 * @brief Bit definitions of MACSEC_CONTROL1 register
 * @{
 */
#define LOOPBACK_MODE_EN		OSI_BIT(31)
#define RX_MTU_CHECK_EN			OSI_BIT(16)
#define TX_LUT_PRIO_BYP			OSI_BIT(2)
#define TX_MTU_CHECK_EN			OSI_BIT(0)
/** @} */

/**
 * @addtogroup GCM_AES_CNTRL register
 *
 * @brief Bit definitions of GCM_AES_CNTRL register
 * @{
 */
#define RX_AES_MODE_MASK		(OSI_BIT(17) | OSI_BIT(16))
#define RX_AES_MODE_AES128		0x0
#define RX_AES_MODE_AES256		OSI_BIT(17)
#define TX_AES_MODE_MASK		(OSI_BIT(1) | OSI_BIT(0))
#define TX_AES_MODE_AES128		0x0
#define TX_AES_MODE_AES256		OSI_BIT(1)
/** @} */

/**
 * @addtogroup COMMON_IMR register
 *
 * @brief Bit definitions of MACSEC_INTERRUPT_MASK register
 * @{
 */
#define SECURE_REG_VIOL_INT_EN		OSI_BIT(31)
#define RX_UNINIT_KEY_SLOT_INT_EN	OSI_BIT(17)
#define RX_LKUP_MISS_INT_EN		OSI_BIT(16)
#define TX_UNINIT_KEY_SLOT_INT_EN	OSI_BIT(1)
#define TX_LKUP_MISS_INT_EN		OSI_BIT(0)
/** @} */

/**
 * @addtogroup TX_IMR register
 *
 * @brief Bit definitions of TX_INTERRUPT_MASK register
 * @{
 */
#ifdef NET30
#define TX_DBG_BUF_CAPTURE_DONE_INT_EN	OSI_BIT(22)
#else
#define TX_SFTY_ERR_CORR_INT_EN		OSI_BIT(26)
#define TX_SFTY_ERR_UNCORR_INT_EN	OSI_BIT(25)
#define TX_DBG_BUF_CAPTURE_DONE_INT_EN	OSI_BIT(2)
#endif
#define TX_MTU_CHECK_FAIL_INT_EN	OSI_BIT(19)
#define TX_AES_GCM_BUF_OVF_INT_EN	OSI_BIT(18)
#define TX_SC_AN_NOT_VALID_INT_EN	OSI_BIT(17)
#define TX_MAC_CRC_ERROR_INT_EN		OSI_BIT(16)
#define TX_PN_EXHAUSTED_INT_EN		OSI_BIT(1)
#define TX_PN_THRSHLD_RCHD_INT_EN	OSI_BIT(0)
/** @} */

/**
 * @addtogroup RX_IMR register
 *
 * @brief Bit definitions of RX_INTERRUPT_MASK register
 * @{
 */
#ifdef NET30
#define RX_DBG_BUF_CAPTURE_DONE_INT_EN	OSI_BIT(22)
#else
#define RX_SFTY_ERR_CORR_INT_EN		OSI_BIT(26)
#define RX_SFTY_ERR_UNCORR_INT_EN	OSI_BIT(25)
#define RX_SC_AN_NOT_VALID_INT_EN	OSI_BIT(17)
#define RX_DBG_BUF_CAPTURE_DONE_INT_EN	OSI_BIT(2)
#endif
#define RX_ICV_ERROR_INT_EN			OSI_BIT(21)
#define RX_REPLAY_ERROR_INT_EN		OSI_BIT(20)
#define RX_MTU_CHECK_FAIL_INT_EN	OSI_BIT(19)
#define RX_AES_GCM_BUF_OVF_INT_EN	OSI_BIT(18)
#define RX_MAC_CRC_ERROR_INT_EN		OSI_BIT(16)
#define RX_PN_EXHAUSTED_INT_EN		OSI_BIT(1)
#ifdef NET24
#define RX_PN_THRSHLD_RCHD_INT_EN	OSI_BIT(0)
#endif /* NET24 */
/** @} */

/**
 * @addtogroup INTERRUPT_MASK1_0 register
 *
 * @brief Bit definitions of INTERRUPT_MASK1_0 register
 * @{
 */
#define SFTY_ERR_UNCORR_INT_EN		OSI_BIT(0)
/** @} */

/**
 * @addtogroup COMMON_ISR register
 *
 * @brief Bit definitions of MACSEC_INTERRUPT_STATUS register
 * @{
 */
#define SECURE_REG_VIOL		OSI_BIT(31)
#define RX_UNINIT_KEY_SLOT	OSI_BIT(17)
#define RX_LKUP_MISS		OSI_BIT(16)
#define TX_UNINIT_KEY_SLOT	OSI_BIT(1)
#define TX_LKUP_MISS		OSI_BIT(0)
/** @} */

/**
 * @addtogroup TX_ISR register
 *
 * @brief Bit definitions of TX_INTERRUPT_STATUS register
 * @{
 */
#ifdef NET30
#define TX_DBG_BUF_CAPTURE_DONE	OSI_BIT(22)
#else
#define TX_SFTY_ERR_CORR	OSI_BIT(26)
#define TX_SFTY_ERR_UNCORR	OSI_BIT(25)
#define TX_DBG_BUF_CAPTURE_DONE	OSI_BIT(2)
#endif
#define TX_MTU_CHECK_FAIL	OSI_BIT(19)
#define TX_AES_GCM_BUF_OVF	OSI_BIT(18)
#define TX_SC_AN_NOT_VALID	OSI_BIT(17)
#define TX_MAC_CRC_ERROR	OSI_BIT(16)
#define TX_PN_EXHAUSTED		OSI_BIT(1)
#define TX_PN_THRSHLD_RCHD	OSI_BIT(0)
/** @} */

/**
 * @addtogroup RX_ISR register
 *
 * @brief Bit definitions of RX_INTERRUPT_STATUS register
 * @{
 */
#ifdef NET30
#define RX_DBG_BUF_CAPTURE_DONE	OSI_BIT(22)
#else
#define RX_SFTY_ERR_CORR	OSI_BIT(26)
#define RX_SFTY_ERR_UNCORR	OSI_BIT(25)
#define RX_SC_AN_NOT_VALID	OSI_BIT(17)
#define RX_DBG_BUF_CAPTURE_DONE	OSI_BIT(2)
#endif
#define RX_ICV_ERROR		OSI_BIT(21)
#define RX_REPLAY_ERROR		OSI_BIT(20)
#define RX_MTU_CHECK_FAIL	OSI_BIT(19)
#define RX_AES_GCM_BUF_OVF	OSI_BIT(18)
#define RX_MAC_CRC_ERROR	OSI_BIT(16)
#define RX_PN_EXHAUSTED		OSI_BIT(1)
#ifdef NET24
#define RX_PN_THRSHLD_RCHD	OSI_BIT(0)
#endif /* NET24 */
/** @} */

/**
 * @addtogroup STATS_CONTROL_0 register
 *
 * @brief Bit definitions of STATS_CONTROL_0 register
 * @{
 */
#ifdef NET30
#define STATS_CONTROL0_RD_CPY			OSI_BIT(3)
#define STATS_CONTROL0_TK_CPY			OSI_BIT(2)
#define STATS_CONTROL0_CNT_RL_OVR_CPY		OSI_BIT(1)
#define STATS_CONTROL0_CNT_CLR			OSI_BIT(0)
#endif /* NET30 */
/** @} */

/**
 * @addtogroup DEBUG_BUF_CONFIG_0 register
 *
 * @brief Bit definitions of DEBUG_BUF_CONFIG_0 register
 * @{
 */
#define DEBUG_BUF_CONFIG_0_UPDATE		OSI_BIT(31)
#define DEBUG_BUF_CONFIG_0_CTLR_SEL		OSI_BIT(25)
#define DEBUG_BUF_CONFIG_0_RW			OSI_BIT(24)
#define DEBUG_BUF_CONFIG_0_IDX_MASK		(OSI_BIT(0) | OSI_BIT(1) | \
						OSI_BIT(2) | OSI_BIT(3))
/** @} */

/**
 * @addtogroup TX_DEBUG_TRIGGER_EN_0 register
 *
 * @brief Bit definitions of TX_DEBUG_TRIGGER_EN_0 register
 * @{
 */
#define TX_DBG_CAPTURE		OSI_BIT(10)
#define TX_DBG_ICV_CORRUPT	OSI_BIT(9)
#define TX_DBG_CRC_CORRUPT	OSI_BIT(8)
#define TX_DBG_DATA_MATCH	OSI_BIT(7)
#define TX_DBG_LKUP_MATCH	OSI_BIT(6)
#define TX_DBG_CRCOUT_MATCH	OSI_BIT(5)
#define TX_DBG_CRCIN_MATCH	OSI_BIT(4)
#define TX_DBG_ICV_MATCH	OSI_BIT(3)
#define TX_DBG_KEY_NOT_VALID	OSI_BIT(2)
#define TX_DBG_AN_NOT_VALID	OSI_BIT(1)
#define TX_DBG_LKUP_MISS	OSI_BIT(0)
/** @} */

/**
 * @addtogroup TX_DEBUG_STATUS_0 register
 *
 * @brief Bit definitions of TX_DEBUG_STATUS_0 register
 * @{
 */
#define TX_DBG_STS_CAPTURE		OSI_BIT(10)
#define TX_DBG_STS_ICV_CORRUPT		OSI_BIT(9)
#define TX_DBG_STS_CRC_CORRUPT		OSI_BIT(8)
#define TX_DBG_STS_DATA_MATCH		OSI_BIT(7)
#define TX_DBG_STS_LKUP_MATCH		OSI_BIT(6)
#define TX_DBG_STS_CRCOUT_MATCH		OSI_BIT(5)
#define TX_DBG_STS_CRCIN_MATCH		OSI_BIT(4)
#define TX_DBG_STS_ICV_MATCH		OSI_BIT(3)
#define TX_DBG_STS_KEY_NOT_VALID	OSI_BIT(2)
#define TX_DBG_STS_AN_NOT_VALID		OSI_BIT(1)
#define TX_DBG_STS_LKUP_MISS		OSI_BIT(0)
/** @} */

/**
 * @addtogroup RX_DEBUG_TRIGGER_EN_0 register
 *
 * @brief Bit definitions of RX_DEBUG_TRIGGER_EN_0 register
 * @{
 */
#define RX_DBG_CAPTURE		OSI_BIT(10)
#define RX_DBG_ICV_ERROR	OSI_BIT(9)
#define RX_DBG_CRC_CORRUPT	OSI_BIT(8)
#define RX_DBG_DATA_MATCH	OSI_BIT(7)
#define RX_DBG_BYP_LKUP_MATCH	OSI_BIT(6)
#define RX_DBG_CRCOUT_MATCH	OSI_BIT(5)
#define RX_DBG_CRCIN_MATCH	OSI_BIT(4)
#define RX_DBG_REPLAY_ERR	OSI_BIT(3)
#define RX_DBG_KEY_NOT_VALID	OSI_BIT(2)
#define RX_DBG_LKUP_MISS	OSI_BIT(0)
/** @} */

/**
 * @addtogroup RX_DEBUG_STATUS_0 register
 *
 * @brief Bit definitions of RX_DEBUG_STATUS_0 register
 * @{
 */
#define RX_DBG_STS_CAPTURE		OSI_BIT(10)
#define RX_DBG_STS_ICV_ERROR		OSI_BIT(9)
#define RX_DBG_STS_CRC_CORRUPT		OSI_BIT(8)
#define RX_DBG_STS_DATA_MATCH		OSI_BIT(7)
#define RX_DBG_STS_BYP_LKUP_MATCH	OSI_BIT(6)
#define RX_DBG_STS_CRCOUT_MATCH		OSI_BIT(5)
#define RX_DBG_STS_CRCIN_MATCH		OSI_BIT(4)
#define RX_DBG_STS_REPLAY_ERR		OSI_BIT(3)
#define RX_DBG_STS_KEY_NOT_VALID	OSI_BIT(2)
#define RX_DBG_STS_LKUP_MISS		OSI_BIT(0)
/** @} */

/**
 * @addtogroup TX_DEBUG_STATUS_0 register
 *
 * @brief Bit definitions of TX_DEBUG_STATUS_0 register
 * @{
 */
#define TX_DBG_STS_CAPTURE		OSI_BIT(10)
#define TX_DBG_STS_ICV_CORRUPT		OSI_BIT(9)
#define TX_DBG_STS_CRC_CORRUPT		OSI_BIT(8)
#define TX_DBG_STS_DATA_MATCH		OSI_BIT(7)
#define TX_DBG_STS_LKUP_MATCH		OSI_BIT(6)
#define TX_DBG_STS_CRCOUT_MATCH		OSI_BIT(5)
#define TX_DBG_STS_CRCIN_MATCH		OSI_BIT(4)
#define TX_DBG_STS_ICV_MATCH		OSI_BIT(3)
#define TX_DBG_STS_KEY_NOT_VALID	OSI_BIT(2)
#define TX_DBG_STS_AN_NOT_VALID		OSI_BIT(1)
#define TX_DBG_STS_LKUP_MISS		OSI_BIT(0)
/** @} */

/**
 * @addtogroup TX_DEBUG_CONTROL_0 register
 *
 * @brief Bit definitions of TX_DEBUG_CONTROL_0 register
 * @{
 */
#define TX_DEBUG_CONTROL_0_START_CAP	OSI_BIT(31)
/** @} */

/**
 * @addtogroup RX_DEBUG_CONTROL_0 register
 *
 * @brief Bit definitions of RX_DEBUG_CONTROL_0 register
 * @{
 */
#define RX_DEBUG_CONTROL_0_START_CAP	OSI_BIT(31)
/** @} */

#define MTU_LENGTH_MASK			0xFFFF
/* MACsec sectag + ICV adds upto 32B */
#define MACSEC_TAG_ICV_LEN		32
/* Add 8B for double VLAN tags (4B each),
 * 14B for L2 SA/DA/ethertype, TODO - as per IAS SA/DA is ignored from length
 * 4B for FCS
 */
#define MTU_ADDONS			(8 + 14 + 4)
#define DVLAN_TAG_ETHERTYPE	0x88A8
#ifndef NET24
/**
 * @addtogroup TX/RX_BYP/SCI_LUT_VALID register
 *
 * @brief Bit definitions of LUT_VALID registers
 * @{
 */
#define TX_BYP_LUT_VALID_ENTRY(x)	OSI_BIT(x)
#define TX_BYP_LUT_VALID_NONE		0x0
#define TX_SCI_LUT_VALID_ENTRY(x)	OSI_BIT(x)
#define TX_SCI_LUT_VALID_NONE		0x0
#define RX_BYP_LUT_VALID_ENTRY(x)	OSI_BIT(x)
#define RX_BYP_LUT_VALID_NONE		0x0
#define RX_SCI_LUT_VALID_ENTRY(x)	OSI_BIT(x)
#define RX_SCI_LUT_VALID_NONE		0x0
/** @} */
#endif /* NET24 */

/**
 * @addtogroup TX/RX LUT bit fields in LUT_DATA registers
 *
 * @brief Helper macros for LUT data programming
 * @{
 */
#define MACSEC_LUT_DATA_REG_CNT		7
/* Bit Offsets for LUT DATA[x] registers for various lookup field masks */
/* DA mask bits in LUT_DATA[1] register */
#define LUT_DA_BYTE0_INACTIVE		OSI_BIT(16)
#define LUT_DA_BYTE1_INACTIVE		OSI_BIT(17)
#define LUT_DA_BYTE2_INACTIVE		OSI_BIT(18)
#define LUT_DA_BYTE3_INACTIVE		OSI_BIT(19)
#define LUT_DA_BYTE4_INACTIVE		OSI_BIT(20)
#define LUT_DA_BYTE5_INACTIVE		OSI_BIT(21)
/* SA mask bits in LUT_DATA[3] register */
#define LUT_SA_BYTE0_INACTIVE		OSI_BIT(6)
#define LUT_SA_BYTE1_INACTIVE		OSI_BIT(7)
#define LUT_SA_BYTE2_INACTIVE		OSI_BIT(8)
#define LUT_SA_BYTE3_INACTIVE		OSI_BIT(9)
#define LUT_SA_BYTE4_INACTIVE		OSI_BIT(10)
#define LUT_SA_BYTE5_INACTIVE		OSI_BIT(11)
/* Ether type mask in LUT_DATA[3] register */
#define LUT_ETHTYPE_INACTIVE		OSI_BIT(28)
/* VLAN PCP mask in LUT_DATA[4] register */
#define LUT_VLAN_PCP_INACTIVE		OSI_BIT(0)
/* VLAN ID mask in LUT_DATA[4] register */
#define LUT_VLAN_ID_INACTIVE		OSI_BIT(13)
/* VLAN mask in LUT_DATA[4] register */
#define LUT_VLAN_ACTIVE			OSI_BIT(14)
/* Byte pattern masks in LUT_DATA[4] register */
#define LUT_BYTE0_PATTERN_INACTIVE	OSI_BIT(29)
/* Byte pattern masks in LUT_DATA[5] register */
#define LUT_BYTE1_PATTERN_INACTIVE	OSI_BIT(12)
#define LUT_BYTE2_PATTERN_INACTIVE	OSI_BIT(27)
/* Byte pattern masks in LUT_DATA[6] register */
#define LUT_BYTE3_PATTERN_INACTIVE	OSI_BIT(10)
/* Preemptible packet in LUT_DATA[6] register */
#define LUT_PREEMPT			OSI_BIT(11)
/* Preempt mask in LUT_DATA[6] register */
#define LUT_PREEMPT_INACTIVE		OSI_BIT(12)
/* Controlled port mask in LUT_DATA[6] register */
#define LUT_CONTROLLED_PORT		OSI_BIT(13)
#ifdef NET30
/* DVLAN packet in LUT_DATA[6] register */
#define BYP_LUT_DVLAN_PKT			OSI_BIT(14)
/* DVLAN outer/inner tag select in LUT_DATA[6] register */
#define BYP_LUT_DVLAN_OUTER_INNER_TAG_SEL	OSI_BIT(15)
#else
/* BYP LUT entry valid in LUT_DATA[6] register */
#define BYP_LUT_ENTRY_VALID		OSI_BIT(14)
#endif /* NET30 */
/* AN valid bits for SCI LUT in LUT_DATA[6] register */
#define LUT_AN0_VALID			OSI_BIT(13)
#define LUT_AN1_VALID			OSI_BIT(14)
#define LUT_AN2_VALID			OSI_BIT(15)
#define LUT_AN3_VALID			OSI_BIT(16)
#ifdef NET30
/* DVLAN packet in LUT_DATA[6] register */
#define TX_SCI_LUT_DVLAN_PKT			OSI_BIT(21)
/* DVLAN outer/inner tag select in LUT_DATA[6] register */
#define TX_SCI_LUT_DVLAN_OUTER_INNER_TAG_SEL	OSI_BIT(22)
#else
/* SCI LUT entry valid in LUT_DATA[6] register */
#define TX_SCI_LUT_ENTRY_VALID		OSI_BIT(21)
#endif /* NET30 */
/* SA State LUT entry valid in LUT_DATA[0] register */
#define SA_STATE_LUT_ENTRY_VALID	OSI_BIT(0)

/* Rx LUT macros */
/* Preemptible packet in LUT_DATA[2] register for Rx SCI */
#define RX_SCI_LUT_PREEMPT		OSI_BIT(8)
/* Preempt mask in LUT_DATA[2] register for Rx SCI */
#define RX_SCI_LUT_PREEMPT_INACTIVE	OSI_BIT(9)
#ifndef NET30
/* Rx SCI LUT entry valid in LUT_DATA[2] register */
#define RX_SCI_LUT_ENTRY_VALID		OSI_BIT(14)
#endif /* NET30 */
/* TODO - Invalidate the valid bits for above LUT_ENTRY_VALIDs in write path */
/** @} */

//////////////////////////////////////////////////////////////////////////
	/* MACsec OSI data structures */
//////////////////////////////////////////////////////////////////////////

/**
 * @addtogroup TX/RX BYP/SCI LUT INPUT macros
 *
 * @brief Helper macros for LUT input programming
 * @{
 */
//TODO - Aggregate all the macros inlined in the struct itself
/** @} */

/**
 * @brief MACsec secure channel basic information
 */
struct osi_macsec_sc_info {
	/** Secure channel identifier */
#define SCI_LEN		8
	unsigned char sci[SCI_LEN];
	/** Secure association key */
#define KEY_LEN_128	16
#define KEY_LEN_256	32
	unsigned char sak[KEY_LEN_128];
	/** current AN */
	unsigned char curr_an;
	/** Next PN to use for the current AN */
	unsigned int next_pn;
	/** == BELOW FIELDS ARE FILLED BY OSI LAYER == */
	/** bitmap of valid AN */
#define AN0_VALID	OSI_BIT(0)
#define AN1_VALID	OSI_BIT(1)
#define AN2_VALID	OSI_BIT(2)
#define AN3_VALID	OSI_BIT(3)
	unsigned int an_valid;
	/** SC LUT index */
	unsigned int sc_idx_start;
};

/**
 * @brief MACsec HW controller LUT's overall status
 */
struct osi_macsec_lut_status {
	/** List of max SC's supported */
#define MAX_NUM_SC	8
#define MAX_NUM_SA	4
	struct osi_macsec_sc_info sc_info[MAX_NUM_SC];
	/** next available BYP LUT index */
	unsigned int next_byp_idx;
	/** next available SC LUT index */
	unsigned int next_sc_idx;
};

/**
 * @brief MACsec SA State LUT entry outputs structure
 */
struct sa_state_outputs {
	/** Next PN to use */
	unsigned int next_pn;
	/** Lowest PN to use */
	unsigned int lowest_pn;
};

/**
 * @brief MACsec SC State LUT entry outputs structure
 */
struct sc_state_outputs {
	/** Current AN to use */
#define CURR_AN_MAX			3 /* 0-3 */
	unsigned int curr_an;
};

/**
 * @brief MACsec SC Param LUT entry outputs structure
 */
struct sc_param_outputs {
	/** Key index start */
#define KEY_INDEX_MAX			31 /* 0-31 */
	unsigned int key_index_start;
	/** PN max for given AN, after which HW will roll over to next AN */
#define PN_MAX_DEFAULT			0xFFFFFFFF
	unsigned int pn_max;
	/** PN threshold to trigger irq when threshold is reached */
#define PN_THRESHOLD_DEFAULT		0xC0000000
	unsigned int pn_threshold;
	/** PN window */
	unsigned int pn_window;
	/** SC identifier */
	unsigned char sci[SCI_LEN];
	/** Default TCI value V=1, ES=0, SC = 1 */
#define TCI_DEFAULT		0x1
	/** TCI 3 bits V=0, ES=0, SC=1 */
	unsigned char tci;
#define VLAN_IN_CLEAR_DEFAULT		0x0
	/** vlan in clear config */
	unsigned char vlan_in_clear;
};

/**
 * @brief MACsec SCI LUT entry outputs structure
 */
struct sci_lut_outputs {
	/** SC index to use */
#define SC_INDEX_MAX			15 /* 0-15 */
	unsigned int sc_index;
	/** SC identifier */
	unsigned char sci[SCI_LEN];
	/** AN's valid */
	unsigned int an_valid;
};

/**
 * @brief MACsec BYP/SCI LUT entry inputs structure
 */
struct lut_inputs {
	/** MAC DA to compare */
	unsigned char da[OSI_ETH_ALEN];
	/** MAC SA to compare */
	unsigned char sa[OSI_ETH_ALEN];
	/** Ethertype to compare */
#define ETHTYPE_LEN			2
	unsigned char ethtype[ETHTYPE_LEN];
	/** 4-Byte pattern to compare */
#define LUT_BYTE_PATTERN_MAX		4
	unsigned char byte_pattern[LUT_BYTE_PATTERN_MAX];
	/** Offset for 4-Byte pattern to compare */
#define LUT_BYTE_PATTERN_MAX_OFFSET	63 /* 0-63 */
	unsigned int byte_pattern_offset[LUT_BYTE_PATTERN_MAX];
	/** VLAN PCP to compare */
#define VLAN_PCP_MAX			7 /* 0-7 */
	unsigned int vlan_pcp;
	/** VLAN ID to compare */
#define VLAN_ID_MAX			4095 /* 1-4095 */
	unsigned int vlan_id;
};

/**
 * @brief MACsec LUT config data structure
 */
struct macsec_table_config {
	/** Controller select
	 * 0 - Tx
	 * 1 - Rx
	 */
#define CTLR_SEL_TX			0U
#define CTLR_SEL_RX			1U
#define CTLR_SEL_MAX			1U
#define NUM_CTLR			2U
	unsigned short ctlr_sel;
	/** Read or write operation select
	 * 0 - Read
	 * 1 - Write
	 */
#define LUT_READ			0U
#define LUT_WRITE			1U
#define RW_MAX				1U
	unsigned short rw;
	/** Table entry index */
#define TABLE_INDEX_MAX			31U /* 0-31 */
#define BYP_LUT_MAX_INDEX		TABLE_INDEX_MAX
#define SC_LUT_MAX_INDEX		15U
#define SA_LUT_MAX_INDEX		TABLE_INDEX_MAX
	unsigned short index;
};

/**
 * @brief MACsec LUT config data structure
 */
struct osi_macsec_lut_config {
	/** Generic table config */
	struct macsec_table_config table_config;
	/** LUT select */
#define LUT_SEL_BYPASS			0U
#define LUT_SEL_SCI			1U
#define LUT_SEL_SC_PARAM		2U
#define LUT_SEL_SC_STATE		3U
#define LUT_SEL_SA_STATE		4U
#define LUT_SEL_MAX			4U /* 0-4 */
	unsigned short lut_sel;
	/** flag - encoding various valid bits for above fields */
	unsigned int flags;
//LUT input fields flags bit offsets
#define LUT_FLAGS_DA_BYTE0_VALID	OSI_BIT(0)
#define LUT_FLAGS_DA_BYTE1_VALID	OSI_BIT(1)
#define LUT_FLAGS_DA_BYTE2_VALID	OSI_BIT(2)
#define LUT_FLAGS_DA_BYTE3_VALID	OSI_BIT(3)
#define LUT_FLAGS_DA_BYTE4_VALID	OSI_BIT(4)
#define LUT_FLAGS_DA_BYTE5_VALID	OSI_BIT(5)
#define LUT_FLAGS_DA_VALID		(OSI_BIT(0) | OSI_BIT(1) | OSI_BIT(2) |\
					 OSI_BIT(3) | OSI_BIT(4) | OSI_BIT(5))
#define LUT_FLAGS_SA_BYTE0_VALID	OSI_BIT(6)
#define LUT_FLAGS_SA_BYTE1_VALID	OSI_BIT(7)
#define LUT_FLAGS_SA_BYTE2_VALID	OSI_BIT(8)
#define LUT_FLAGS_SA_BYTE3_VALID	OSI_BIT(9)
#define LUT_FLAGS_SA_BYTE4_VALID	OSI_BIT(10)
#define LUT_FLAGS_SA_BYTE5_VALID	OSI_BIT(11)
#define LUT_FLAGS_SA_VALID		(OSI_BIT(6) | OSI_BIT(7) | OSI_BIT(8) |\
					 OSI_BIT(9) | OSI_BIT(10) | OSI_BIT(11))
#define LUT_FLAGS_ETHTYPE_VALID		OSI_BIT(12)
#define LUT_FLAGS_VLAN_PCP_VALID	OSI_BIT(13)
#define LUT_FLAGS_VLAN_ID_VALID		OSI_BIT(14)
#define LUT_FLAGS_VLAN_VALID		OSI_BIT(15)
#define LUT_FLAGS_BYTE0_PATTERN_VALID	OSI_BIT(16)
#define LUT_FLAGS_BYTE1_PATTERN_VALID	OSI_BIT(17)
#define LUT_FLAGS_BYTE2_PATTERN_VALID	OSI_BIT(18)
#define LUT_FLAGS_BYTE3_PATTERN_VALID	OSI_BIT(19)
#define LUT_FLAGS_PREEMPT		OSI_BIT(20)
#define LUT_FLAGS_PREEMPT_VALID		OSI_BIT(21)
#define LUT_FLAGS_CONTROLLED_PORT	OSI_BIT(22)
#define LUT_FLAGS_DVLAN_PKT			OSI_BIT(23)
#define LUT_FLAGS_DVLAN_OUTER_INNER_TAG_SEL	OSI_BIT(24)
#define LUT_FLAGS_ENTRY_VALID		OSI_BIT(31)
	/** LUT inputs */
	struct lut_inputs lut_in;
	/** SCI LUT outputs */
	struct sci_lut_outputs sci_lut_out;
	/** SC Param outputs */
	struct sc_param_outputs sc_param_out;
	/** SC State outputs */
	struct sc_state_outputs sc_state_out;
	/** SA State outputs */
	struct sa_state_outputs sa_state_out;
};

/**
 * @brief MACsec KT entry structure
 */
struct kt_entry {
	/** SAK key - max 256bit */
	unsigned char sak[KEY_LEN_256];
	/** H-key */
	unsigned char h[KEY_LEN_128];
};

/**
 * @brief MACsec KT config data structure
 */
struct osi_macsec_kt_config {
	/** Generic table config */
	struct macsec_table_config table_config;
	/** KT entry */
	struct kt_entry entry;
	/** flag - encoding various valid bits */
	unsigned int flags;
};

/**
 * @brief MACsec Debug buffer data structure
 */
struct osi_macsec_dbg_buf_config {
	/** Controller select
	 * 0 - Tx
	 * 1 - Rx
	 */
	unsigned short ctlr_sel;
		/** Read or write operation select
		 * 0 - Read
		 * 1 - Write
		 */
#define DBG_TBL_READ	LUT_READ
#define DBG_TBL_WRITE	LUT_WRITE
	unsigned short rw;
	/* Num of Tx debug buffers */
#define TX_DBG_BUF_IDX_MAX		12U
	/* Num of Rx debug buffers */
#define RX_DBG_BUF_IDX_MAX		13U
#define DBG_BUF_IDX_MAX			RX_DBG_BUF_IDX_MAX
	/** debug data buffer */
	unsigned int dbg_buf[4];
	/** flag - encoding various debug event bits */
#define TX_DBG_LKUP_MISS_EVT		OSI_BIT(0)
#define TX_DBG_AN_NOT_VALID_EVT		OSI_BIT(1)
#define TX_DBG_KEY_NOT_VALID_EVT	OSI_BIT(2)
#define TX_DBG_CRC_CORRUPT_EVT		OSI_BIT(3)
#define TX_DBG_ICV_CORRUPT_EVT		OSI_BIT(4)
#define TX_DBG_CAPTURE_EVT			OSI_BIT(5)
#define RX_DBG_LKUP_MISS_EVT		OSI_BIT(6)
#define RX_DBG_KEY_NOT_VALID_EVT	OSI_BIT(7)
#define RX_DBG_REPLAY_ERR_EVT		OSI_BIT(8)
#define RX_DBG_CRC_CORRUPT_EVT		OSI_BIT(9)
#define RX_DBG_ICV_ERROR_EVT		OSI_BIT(10)
#define RX_DBG_CAPTURE_EVT			OSI_BIT(11)
	unsigned int flags;
	/** debug buffer index */
	unsigned int index;
};

/**
 * @brief MACsec core operations structure
 */
struct macsec_core_ops {
	/** macsec init */
	int (*init)(struct osi_core_priv_data *const osi_core);
	/** macsec de-init */
	void (*deinit)(struct osi_core_priv_data *const osi_core);
	/** NS irq handler */
	void (*handle_ns_irq)(struct osi_core_priv_data *const osi_core);
	/** S irq handler */
	void (*handle_s_irq)(struct osi_core_priv_data *const osi_core);
	/** macsec lut config */
	int (*lut_config)(struct osi_core_priv_data *const osi_core,
			  struct osi_macsec_lut_config *const lut_config);
	/** macsec kt config */
	int (*kt_config)(struct osi_core_priv_data *const osi_core,
			 struct osi_macsec_kt_config *const kt_config);
	/** macsec loopback config */
	int (*loopback_config)(struct osi_core_priv_data *const osi_core,
			       unsigned int enable);
	/** macsec enable */
	int (*macsec_en)(struct osi_core_priv_data *const osi_core,
			 unsigned int enable);
	/** macsec config SA in HW LUT */
	int (*config)(struct osi_core_priv_data *const osi_core,
		      struct osi_macsec_sc_info *const sc,
		      unsigned int enable, unsigned short ctlr);
	/** macsec read mmc counters */
	void (*read_mmc)(struct osi_core_priv_data *const osi_core);
	/** macsec debug buffer config */
	int (*dbg_buf_config)(struct osi_core_priv_data *const osi_core,
			struct osi_macsec_dbg_buf_config *const dbg_buf_config);
	/** macsec debug buffer config */
	int (*dbg_events_config)(struct osi_core_priv_data *const osi_core,
			struct osi_macsec_dbg_buf_config *const dbg_buf_config);

};

//////////////////////////////////////////////////////////////////////////
	/* OSI interface API prototypes */
//////////////////////////////////////////////////////////////////////////

/**
 * @brief initializing the macsec core operations
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @retval 0 on success
 * @retval -1 on failure
 */
int osi_init_macsec_ops(struct osi_core_priv_data *const osi_core);

/**
 * @brief initialize the macsec controller
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @retval 0 on success
 * @retval -1 on failure
 */
int osi_macsec_init(struct osi_core_priv_data *const osi_core);

/**
 * @brief De-initialize the macsec controller
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 */
void osi_macsec_deinit(struct osi_core_priv_data *const osi_core);

/**
 * @brief Non-secure irq handler
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @retval None
 */
void osi_macsec_ns_isr(struct osi_core_priv_data *const osi_core);

/**
 * @brief Secure irq handler
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @retval None
 */
void osi_macsec_s_isr(struct osi_core_priv_data *const osi_core);

/**
 * @brief MACsec Lookup table config
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] lut_config: OSI macsec LUT config data structure.
 *
 * @retval 0 on success
 * @retval -1 on failure
 */
int osi_macsec_lut_config(struct osi_core_priv_data *const osi_core,
			  struct osi_macsec_lut_config *const lut_confg);

/**
 * @brief MACsec key table config
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] kt_config: OSI macsec KT config data structure.
 *
 * @retval 0 on success
 * @retval -1 on failure
 */
int osi_macsec_kt_config(struct osi_core_priv_data *const osi_core,
			 struct osi_macsec_kt_config *const kt_config);

/**
 * @brief MACsec Loopback config
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] enable: Loopback enable/disable flag.
 *
 * @retval 0 on success
 * @retval -1 on failure
 */
int osi_macsec_loopback(struct osi_core_priv_data *const osi_core,
			unsigned int enable);

/**
 * @brief MACsec Enable
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] enable: macsec controller Tx/Rx enable/disable flag.
 *
 * @retval 0 on success
 * @retval -1 on failure
 */
int osi_macsec_en(struct osi_core_priv_data *const osi_core,
		  unsigned int enable);

/**
 * @brief MACsec update secure channel/association in controller
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] tx_sa: Pointer to osi_macsec_sc_info struct for the tx SA.
 * @param[in] enable: flag to indicate enable/disable for the Tx SA.
 *
 * @retval 0 on success
 * @retval -1 on failure
 */
int osi_macsec_config(struct osi_core_priv_data *const osi_core,
		      struct osi_macsec_sc_info *const sc,
		      unsigned int enable, unsigned short ctlr);

/**
 * @brief MACsec read statistics counters
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @retval 0 on success
 * @retval -1 on failure
 */
int osi_macsec_read_mmc(struct osi_core_priv_data *const osi_core);

/**
 * @brief MACsec debug buffer config
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] dbg_buf_config: OSI macsec debug buffer config data structure.
 *
 * @retval 0 on success
 * @retval -1 on failure
 */
int osi_macsec_dbg_buf_config(
		struct osi_core_priv_data *const osi_core,
		struct osi_macsec_dbg_buf_config *const dbg_buf_config);

/**
 * @brief MACsec debug events config
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] dbg_buf_config: OSI macsec debug buffer config data structure.
 *
 * @retval 0 on success
 * @retval -1 on failure
 */
int osi_macsec_dbg_events_config(
		struct osi_core_priv_data *const osi_core,
		struct osi_macsec_dbg_buf_config *const dbg_buf_config);

#endif /* MACSEC_H */
