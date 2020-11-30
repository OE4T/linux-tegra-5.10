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

#ifndef INCLUDED_OSI_MACSEC_H
#define INCLUDED_OSI_MACSEC_H

#include <osi_core.h>

//////////////////////////////////////////////////////////////////////////
	/* MACsec OSI data structures */
//////////////////////////////////////////////////////////////////////////

/**
 * @addtogroup TX/RX BYP/SCI LUT helpers macros
 *
 * @brief Helper macros for LUT programming
 * @{
 */
#define SCI_LEN				8
#define KEY_LEN_128			16
#define KEY_LEN_256			32
#define AN0_VALID			OSI_BIT(0)
#define AN1_VALID			OSI_BIT(1)
#define AN2_VALID			OSI_BIT(2)
#define AN3_VALID			OSI_BIT(3)
#define MAX_NUM_SC			8
#define MAX_NUM_SA			4
#define CURR_AN_MAX			3 /* 0-3 */
#define KEY_INDEX_MAX			31 /* 0-31 */
#define PN_MAX_DEFAULT			0xFFFFFFFF
#define PN_THRESHOLD_DEFAULT		0xC0000000
#define TCI_DEFAULT			0x1
#define VLAN_IN_CLEAR_DEFAULT		0x0
#define SC_INDEX_MAX			15 /* 0-15 */
#define ETHTYPE_LEN			2
#define LUT_BYTE_PATTERN_MAX		4
#define LUT_BYTE_PATTERN_MAX_OFFSET	63 /* 0-63 */
#define VLAN_PCP_MAX			7 /* 0-7 */
#define VLAN_ID_MAX			4095 /* 1-4095 */

#define LUT_SEL_BYPASS			0U
#define LUT_SEL_SCI			1U
#define LUT_SEL_SC_PARAM		2U
#define LUT_SEL_SC_STATE		3U
#define LUT_SEL_SA_STATE		4U
#define LUT_SEL_MAX			4U /* 0-4 */

/* LUT input fields flags bit offsets */
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

/** @} */

/**
 * @addtogroup Generic table CONFIG register helpers macros
 *
 * @brief Helper macros for generic table CONFIG register programming
 * @{
 */
#define CTLR_SEL_TX			0U
#define CTLR_SEL_RX			1U
#define CTLR_SEL_MAX			1U
#define NUM_CTLR			2U
#define LUT_READ			0U
#define LUT_WRITE			1U
#define RW_MAX				1U
#define TABLE_INDEX_MAX			31U /* 0-31 */
#define BYP_LUT_MAX_INDEX		TABLE_INDEX_MAX
#define SC_LUT_MAX_INDEX		15U
#define SA_LUT_MAX_INDEX		TABLE_INDEX_MAX

/** @} */

/**
 * @addtogroup Debug buffer table CONFIG register helpers macros
 *
 * @brief Helper macros for debug buffer table CONFIG register programming
 * @{
 */
#define DBG_TBL_READ	LUT_READ
#define DBG_TBL_WRITE	LUT_WRITE
/* Num of Tx debug buffers */
#define TX_DBG_BUF_IDX_MAX		12U
/* Num of Rx debug buffers */
#define RX_DBG_BUF_IDX_MAX		13U
#define DBG_BUF_IDX_MAX			RX_DBG_BUF_IDX_MAX
/** flag - encoding various debug event bits */
#define TX_DBG_LKUP_MISS_EVT		OSI_BIT(0)
#define TX_DBG_AN_NOT_VALID_EVT		OSI_BIT(1)
#define TX_DBG_KEY_NOT_VALID_EVT	OSI_BIT(2)
#define TX_DBG_CRC_CORRUPT_EVT		OSI_BIT(3)
#define TX_DBG_ICV_CORRUPT_EVT		OSI_BIT(4)
#define TX_DBG_CAPTURE_EVT		OSI_BIT(5)
#define RX_DBG_LKUP_MISS_EVT		OSI_BIT(6)
#define RX_DBG_KEY_NOT_VALID_EVT	OSI_BIT(7)
#define RX_DBG_REPLAY_ERR_EVT		OSI_BIT(8)
#define RX_DBG_CRC_CORRUPT_EVT		OSI_BIT(9)
#define RX_DBG_ICV_ERROR_EVT		OSI_BIT(10)
#define RX_DBG_CAPTURE_EVT		OSI_BIT(11)

/** @} */

/* AES cipthers 128 bit or 256 bit */
#define MACSEC_CIPHER_AES128	0U
#define MACSEC_CIPHER_AES256	1U

/* macsec tx/rx enable */
#define OSI_MACSEC_TX_EN	OSI_BIT(0)
#define OSI_MACSEC_RX_EN	OSI_BIT(1)

/* MACsec sectag + ICV + 2B ethertype adds upto 34B */
#define MACSEC_TAG_ICV_LEN		34
/* Add 8B for double VLAN tags (4B each),
 * 14B for L2 SA/DA/ethertype
 * 4B for FCS
 */

#define MACSEC_CMD_TZ_CONFIG		0x1
#define MACSEC_CMD_TZ_KT_RESET		0x2

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
	unsigned int curr_an;
};

/**
 * @brief MACsec SC Param LUT entry outputs structure
 */
struct sc_param_outputs {
	/** Key index start */
	unsigned int key_index_start;
	/** PN max for given AN, after which HW will roll over to next AN */
	unsigned int pn_max;
	/** PN threshold to trigger irq when threshold is reached */
	unsigned int pn_threshold;
	/** PN window */
	unsigned int pn_window;
	/** SC identifier */
	unsigned char sci[SCI_LEN];
	/** Default TCI value V=1, ES=0, SC = 1
	  * TCI 3 bits V=0, ES=0, SC=1 */
	unsigned char tci;
	/** vlan in clear config */
	unsigned char vlan_in_clear;
};

/**
 * @brief MACsec SCI LUT entry outputs structure
 */
struct sci_lut_outputs {
	/** SC index to use */
	unsigned int sc_index;
	/** SC identifier */
	unsigned char sci[SCI_LEN];
	/** AN's valid */
	unsigned int an_valid;
};

/**
 * @brief MACsec LUT config data structure
 */
struct macsec_table_config {
	/** Controller select
	 * 0 - Tx
	 * 1 - Rx
	 */
	unsigned short ctlr_sel;
	/** Read or write operation select
	 * 0 - Read
	 * 1 - Write
	 */
	unsigned short rw;
	/** Table entry index */
	unsigned short index;
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
 * @brief MACsec BYP/SCI LUT entry inputs structure
 */
struct lut_inputs {
	/** MAC DA to compare */
	unsigned char da[OSI_ETH_ALEN];
	/** MAC SA to compare */
	unsigned char sa[OSI_ETH_ALEN];
	/** Ethertype to compare */
	unsigned char ethtype[ETHTYPE_LEN];
	/** 4-Byte pattern to compare */
	unsigned char byte_pattern[LUT_BYTE_PATTERN_MAX];
	/** Offset for 4-Byte pattern to compare */
	unsigned int byte_pattern_offset[LUT_BYTE_PATTERN_MAX];
	/** VLAN PCP to compare */
	unsigned int vlan_pcp;
	/** VLAN ID to compare */
	unsigned int vlan_id;
};

/**
 * @brief MACsec secure channel basic information
 */
struct osi_macsec_sc_info {
	/** Secure channel identifier */
	unsigned char sci[SCI_LEN];
	/** Secure association key */
	unsigned char sak[KEY_LEN_128];
	/** current AN */
	unsigned char curr_an;
	/** Next PN to use for the current AN */
	unsigned int next_pn;
	/** bitmap of valid AN */
	unsigned int an_valid;
	/** SC LUT index */
	unsigned int sc_idx_start;
};

/**
 * @brief MACsec HW controller LUT's overall status
 */
struct osi_macsec_lut_status {
	/** List of max SC's supported */
	struct osi_macsec_sc_info sc_info[MAX_NUM_SC];
	/** next available BYP LUT index */
	unsigned int next_byp_idx;
	/** next available SC LUT index */
	unsigned int next_sc_idx;
};

/**
 * @brief MACsec LUT config data structure
 */
struct osi_macsec_lut_config {
	/** Generic table config */
	struct macsec_table_config table_config;
	/** LUT select */
	unsigned short lut_sel;
	/** flag - encoding various valid bits for above fields */
	unsigned int flags;
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
	unsigned short rw;
	/** debug data buffer */
	unsigned int dbg_buf[4];
	/** flag - encoding various debug event bits */
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
	int (*deinit)(struct osi_core_priv_data *const osi_core);
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
	/** macsec cipher config */
	int (*cipher_config)(struct osi_core_priv_data *const osi_core,
			     unsigned int cipher);
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
 * @note
 * Algorithm:
 * - Init osi_core macsec ops and lut status structure members
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @pre
 *
 * @note
 * Traceability Details:
 * - SWUD_ID:
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure
 */
int osi_init_macsec_ops(struct osi_core_priv_data *const osi_core);

/**
 * @brief Initialize the macsec controller
 *
 * @note
 * Algorithm:
 * - Configure MTU, controller configs, interrupts, clear all LUT's and
 *    set BYP LUT entries for MKPDU and BC packets
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @pre
 * - MACSEC should be out of reset and clocks are enabled
 *
 * @note
 * Traceability Details:
 * - SWUD_ID:
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
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
 * @retval 0 on success
 * @retval -1 on failure
 */
int osi_macsec_deinit(struct osi_core_priv_data *const osi_core);

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
 * @param[in] enable: Pointer to netlink message.
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

#endif /* INCLUDED_OSI_MACSEC_H */
