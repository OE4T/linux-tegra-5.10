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

#include <linux/crypto.h>
#include <osi_macsec.h>
#include <linux/printk.h>
#include "macsec.h"
#include "../osi/common/common.h"

/**
 * @brief poll_for_dbg_buf_update - Query the status of a debug buffer update.
 *
 * @param[in] osi_core: OSI Core private data structure.
 *
 * @retval 0 on Success
 * @retval -1 on Failure
 */
static int poll_for_dbg_buf_update(struct osi_core_priv_data *const osi_core)
{
	int retry = 5000;
	unsigned int dbg_buf_config;

	while (retry > 0) {
		dbg_buf_config = osi_readl(
			(unsigned char *)osi_core->macsec_base +
			 DEBUG_BUF_CONFIG_0);
		if ((dbg_buf_config & DEBUG_BUF_CONFIG_0_UPDATE) == 0U) {
			break;
		}
		/* wait on UPDATE bit to reset */
		osi_core->osd_ops.udelay(10U);
		retry--;
	}
	/** timeout */
	if (retry <= 0) {
		pr_err("%s(): timeout!\n", __func__);
		return -1;
	}

	return 0;

}

/**
 * @brief write_dbg_buf_data - Commit.debug buffer to HW
 *
 * @param[in] osi_core: OSI Core private data structure.
 * @param[in] dbg_buf: Pointer to debug buffer data to be written
 *
 * @retval none
 */
static inline void write_dbg_buf_data(
				struct osi_core_priv_data *const osi_core,
				unsigned int const *const dbg_buf)
{

	unsigned char *base = (unsigned char *)osi_core->macsec_base;
	int i;

	/* Commit the dbg buffer to HW */
	for (i = 0; i < DBG_BUF_LEN; i++) {
		/* pr_err("%s: dbg_buf_data[%d]: 0x%x\n", __func__,
			 i, dbg_buf[i]); */
		osi_writel(dbg_buf[i], base + DEBUG_BUF_DATA_0(i));
	}
}

/**
 * @brief read_dbg_buf_data - Read.debug buffer from HW
 *
 * @param[in] osi_core: OSI Core private data structure.
 * @param[in] dbg_buf: Pointer to debug buffer data to be read
 *
 * @retval none
 */
static inline void read_dbg_buf_data(
				struct osi_core_priv_data *const osi_core,
				unsigned int *dbg_buf)
{

	unsigned char *base = (unsigned char *)osi_core->macsec_base;
	int i;

	/* Read debug buffer from HW */
	for (i = 0; i < DBG_BUF_LEN; i++) {
		dbg_buf[i] = osi_readl(base + DEBUG_BUF_DATA_0(1));
		pr_err("%s: dbg_buf_data[%d]: 0x%x\n", __func__, i, dbg_buf[i]);
	}
}

/**
 * @brief tx_dbg_trigger_evts - Enable/Disable TX debug trigger events.
 *
 * @param[in] osi_core: OSI Core private data structure.
 * @param[in] dbg_buf_config: Pointer to debug buffer config data structure.
 *
 * @retval None
 */
static void tx_dbg_trigger_evts(
		struct osi_core_priv_data *const osi_core,
		struct osi_macsec_dbg_buf_config *const dbg_buf_config)
{

	unsigned char *base = (unsigned char *)osi_core->macsec_base;
	unsigned int flags = 0;
	unsigned int tx_trigger_evts, debug_ctrl_reg;

	if (dbg_buf_config->rw == DBG_TBL_WRITE) {
		flags = dbg_buf_config->flags;
		tx_trigger_evts = osi_readl(base + TX_DEBUG_TRIGGER_EN_0);
		if (flags & TX_DBG_LKUP_MISS_EVT) {
			tx_trigger_evts |= TX_DBG_LKUP_MISS;
		} else {
			tx_trigger_evts &= ~TX_DBG_LKUP_MISS;
		}

		if (flags & TX_DBG_AN_NOT_VALID_EVT) {
			tx_trigger_evts |= TX_DBG_AN_NOT_VALID;
		} else {
			tx_trigger_evts &= ~TX_DBG_AN_NOT_VALID;
		}

		if (flags & TX_DBG_KEY_NOT_VALID_EVT) {
			tx_trigger_evts |= TX_DBG_KEY_NOT_VALID;
		} else {
			tx_trigger_evts &= ~TX_DBG_KEY_NOT_VALID;
		}

		if (flags & TX_DBG_CRC_CORRUPT_EVT) {
			tx_trigger_evts |= TX_DBG_CRC_CORRUPT;
		} else {
			tx_trigger_evts &= ~TX_DBG_CRC_CORRUPT;
		}

		if (flags & TX_DBG_ICV_CORRUPT_EVT) {
			tx_trigger_evts |= TX_DBG_ICV_CORRUPT;
		} else {
			tx_trigger_evts &= ~TX_DBG_ICV_CORRUPT;
		}

		if (flags & TX_DBG_CAPTURE_EVT) {
			tx_trigger_evts |= TX_DBG_CAPTURE;
		} else {
			tx_trigger_evts &= ~TX_DBG_CAPTURE;
		}

		pr_err("%s: tx_trigger_evts 0x%x", __func__, tx_trigger_evts);
		osi_writel(tx_trigger_evts, base + TX_DEBUG_TRIGGER_EN_0);
		if (tx_trigger_evts != OSI_NONE) {
			/** Start the tx debug buffer capture */
			debug_ctrl_reg = osi_readl(base + TX_DEBUG_CONTROL_0);
			debug_ctrl_reg |= TX_DEBUG_CONTROL_0_START_CAP;
			pr_err("%s: debug_ctrl_reg 0x%x", __func__,
			       debug_ctrl_reg);
			osi_writel(debug_ctrl_reg, base + TX_DEBUG_CONTROL_0);
		}
	} else {
		tx_trigger_evts = osi_readl(base + TX_DEBUG_STATUS_0);
		pr_err("%s: tx_trigger_evts 0x%x", __func__, tx_trigger_evts);
		if (tx_trigger_evts & TX_DBG_LKUP_MISS) {
			flags |= TX_DBG_LKUP_MISS_EVT;
		}
		if (tx_trigger_evts & TX_DBG_AN_NOT_VALID) {
			flags |= TX_DBG_AN_NOT_VALID_EVT;
		}
		if (tx_trigger_evts & TX_DBG_KEY_NOT_VALID) {
			flags |= TX_DBG_KEY_NOT_VALID_EVT;
		}
		if (tx_trigger_evts & TX_DBG_CRC_CORRUPT) {
			flags |= TX_DBG_CRC_CORRUPT_EVT;
		}
		if (tx_trigger_evts & TX_DBG_ICV_CORRUPT) {
			flags |= TX_DBG_ICV_CORRUPT_EVT;
		}
		if (tx_trigger_evts & TX_DBG_CAPTURE) {
			flags |= TX_DBG_CAPTURE_EVT;
		}
		dbg_buf_config->flags = flags;
	}
}

/**
 * @brief rx_dbg_trigger_evts - Enable/Disable RX debug trigger events.
 *
 * @param[in] osi_core: OSI Core private data structure.
 * @param[in] dbg_buf_config: Pointer to debug buffer config data structure.
 *
 * @retval None
 */
static void rx_dbg_trigger_evts(
		struct osi_core_priv_data *const osi_core,
		struct osi_macsec_dbg_buf_config *const dbg_buf_config)
{

	unsigned char *base = (unsigned char *)osi_core->macsec_base;
	unsigned int flags = 0;
	unsigned int rx_trigger_evts, debug_ctrl_reg;

	if (dbg_buf_config->rw == DBG_TBL_WRITE) {
		flags = dbg_buf_config->flags;
		rx_trigger_evts = osi_readl(base + RX_DEBUG_TRIGGER_EN_0);
		if (flags & RX_DBG_LKUP_MISS_EVT) {
			rx_trigger_evts |= RX_DBG_LKUP_MISS;
		} else {
			rx_trigger_evts &= ~RX_DBG_LKUP_MISS;
		}

		if (flags & RX_DBG_KEY_NOT_VALID_EVT) {
			rx_trigger_evts |= RX_DBG_KEY_NOT_VALID;
		} else {
			rx_trigger_evts &= ~RX_DBG_KEY_NOT_VALID;
		}

		if (flags & RX_DBG_REPLAY_ERR_EVT) {
			rx_trigger_evts |= RX_DBG_REPLAY_ERR;
		} else {
			rx_trigger_evts &= ~RX_DBG_REPLAY_ERR;
		}

		if (flags & RX_DBG_CRC_CORRUPT_EVT) {
			rx_trigger_evts |= RX_DBG_CRC_CORRUPT;
		} else {
			rx_trigger_evts &= ~RX_DBG_CRC_CORRUPT;
		}

		if (flags & RX_DBG_ICV_ERROR_EVT) {
			rx_trigger_evts |= RX_DBG_ICV_ERROR;
		} else {
			rx_trigger_evts &= ~RX_DBG_ICV_ERROR;
		}

		if (flags & RX_DBG_CAPTURE_EVT) {
			rx_trigger_evts |= RX_DBG_CAPTURE;
		} else {
			rx_trigger_evts &= ~RX_DBG_CAPTURE;
		}
		pr_err("%s: rx_trigger_evts 0x%x", __func__, rx_trigger_evts);
		osi_writel(rx_trigger_evts, base + RX_DEBUG_TRIGGER_EN_0);
		if (rx_trigger_evts != OSI_NONE) {
			/** Start the tx debug buffer capture */
			debug_ctrl_reg = osi_readl(base + RX_DEBUG_CONTROL_0);
			debug_ctrl_reg |= RX_DEBUG_CONTROL_0_START_CAP;
			pr_err("%s: debug_ctrl_reg 0x%x", __func__,
			       debug_ctrl_reg);
			osi_writel(debug_ctrl_reg, base + RX_DEBUG_CONTROL_0);
		}
	} else {
		rx_trigger_evts = osi_readl(base + RX_DEBUG_STATUS_0);
		pr_err("%s: rx_trigger_evts 0x%x", __func__, rx_trigger_evts);
		if (rx_trigger_evts & RX_DBG_LKUP_MISS) {
			flags |= RX_DBG_LKUP_MISS_EVT;
		}
		if (rx_trigger_evts & RX_DBG_KEY_NOT_VALID) {
			flags |= RX_DBG_KEY_NOT_VALID_EVT;
		}
		if (rx_trigger_evts & RX_DBG_REPLAY_ERR) {
			flags |= RX_DBG_REPLAY_ERR_EVT;
		}
		if (rx_trigger_evts & RX_DBG_CRC_CORRUPT) {
			flags |= RX_DBG_CRC_CORRUPT_EVT;
		}
		if (rx_trigger_evts & RX_DBG_ICV_ERROR) {
			flags |= RX_DBG_ICV_ERROR_EVT;
		}
		if (rx_trigger_evts & RX_DBG_CAPTURE) {
			flags |= RX_DBG_CAPTURE_EVT;
		}
		dbg_buf_config->flags = flags;
	}

}

/**
 * @brief macsec_dbg_buf_config - Read/Write debug buffers.
 *
 * @param[in] osi_core: OSI Core private data structure.
 * @param[in] dbg_buf_config: Pointer to debug buffer config data structure.
 *
 * @retval 0 on Success
 * @retval -1 on Failure
 */
static int macsec_dbg_buf_config(struct osi_core_priv_data *const osi_core,
		struct osi_macsec_dbg_buf_config *const dbg_buf_config)
{

	unsigned char *base = (unsigned char *)osi_core->macsec_base;
//	unsigned int  en_flags;
	unsigned int dbg_config_reg = 0;
	int ret = 0;

	/* Validate inputs */
	if ((dbg_buf_config->rw > RW_MAX) ||
		(dbg_buf_config->ctlr_sel > CTLR_SEL_MAX)) {
		pr_err("%s(): Params validation failed\n", __func__);
		return -1;
	}

	if ((dbg_buf_config->ctlr_sel == CTLR_SEL_TX &&
		dbg_buf_config->index > TX_DBG_BUF_IDX_MAX) ||
		(dbg_buf_config->ctlr_sel == CTLR_SEL_RX &&
		dbg_buf_config->index > RX_DBG_BUF_IDX_MAX)) {
		pr_err("%s(): Wrong index %d\n", __func__,
			dbg_buf_config->index);
		return -1;
	}
	//en_flags = osi_readl(base + TX_DEBUG_TRIGGER_EN_0);
	/** disable all trigger events */
	//osi_writel(0, base + TX_DEBUG_TRIGGER_EN_0);

	/* Wait for previous debug table update to finish */
	ret = poll_for_dbg_buf_update(osi_core);
	if (ret < 0) {
		return ret;
	}

	pr_err("%s: ctrl: %hu rw: %hu idx: %hu flags: %#x\n", __func__,
			dbg_buf_config->ctlr_sel, dbg_buf_config->rw,
			dbg_buf_config->index, dbg_buf_config->flags);

	dbg_config_reg = osi_readl(base + DEBUG_BUF_CONFIG_0);

	if (dbg_buf_config->ctlr_sel) {
		dbg_config_reg |= DEBUG_BUF_CONFIG_0_CTLR_SEL;
	} else {
		dbg_config_reg &= ~DEBUG_BUF_CONFIG_0_CTLR_SEL;
	}

	if (dbg_buf_config->rw) {
		dbg_config_reg |= DEBUG_BUF_CONFIG_0_RW;
		/** Write data to debug buffer */
		write_dbg_buf_data(osi_core, dbg_buf_config->dbg_buf);
	} else {
		dbg_config_reg &= ~DEBUG_BUF_CONFIG_0_RW;
	}

	dbg_config_reg &= ~DEBUG_BUF_CONFIG_0_IDX_MASK;
	dbg_config_reg |= dbg_buf_config->index ;
	dbg_config_reg |= DEBUG_BUF_CONFIG_0_UPDATE;
	pr_err("%s: dbg_config_reg 0x%x\n", __func__, dbg_config_reg);
	osi_writel(dbg_config_reg, base + DEBUG_BUF_CONFIG_0);
	ret = poll_for_dbg_buf_update(osi_core);
	if (ret < 0) {
		return ret;
	}

	if (!dbg_buf_config->rw) {
		read_dbg_buf_data(osi_core, dbg_buf_config->dbg_buf);
	}
	/** Enable Tx trigger events */
//	osi_writel(en_flags, base + TX_DEBUG_TRIGGER_EN_0);
	return 0;
}


int macsec_dbg_events_config(
		struct osi_core_priv_data *const osi_core,
		struct osi_macsec_dbg_buf_config *const dbg_buf_config)
{

	int ret = 0;

	pr_err("%s():", __func__);

	/* Validate inputs */
	if ((dbg_buf_config->rw > RW_MAX) ||
		(dbg_buf_config->ctlr_sel > CTLR_SEL_MAX)) {
		pr_err("%s(): Params validation failed", __func__);
		return -1;
	}
	switch (dbg_buf_config->ctlr_sel) {
	case CTLR_SEL_TX:
		tx_dbg_trigger_evts(osi_core, dbg_buf_config);
		break;
	case CTLR_SEL_RX:
		rx_dbg_trigger_evts(osi_core, dbg_buf_config);
		break;
	}

	return ret;
}

/**
 * @brief update_macsec_mmc_val - function to read register and return value
 *				 to callee
 * Algorithm: Read the registers, check for boundary, if more, reset
 *	  counters else return same to caller.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] offset: HW register offset
 *
 * @note
 *	1) MAC/MACSEC should be init and started.
 *
 * @retval value on current MMC counter value.
 */
static inline unsigned long long update_macsec_mmc_val(
			struct osi_core_priv_data *osi_core,
			unsigned long offset)
{

	unsigned long long temp;
	unsigned int value_lo, value_hi;

	value_lo = osi_readl((unsigned char *)osi_core->macsec_base + offset);
	value_hi = osi_readl((unsigned char *)osi_core->macsec_base +
						 (offset + 4U));
	temp = (value_lo | value_hi << 31);

	return temp;
}


/**
 * @brief macsec_read_mmc - To read statitics registers and update structure
 *	   variable
 *
 * Algorithm: Pass register offset and old value to helper function and
 *	   update structure.
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @note
 *	1) MAC/MACSEC should be init and started.
 */
void macsec_read_mmc(struct osi_core_priv_data *const osi_core)
{
	struct osi_macsec_mmc_counters *mmc = &osi_core->macsec_mmc;
	unsigned short i;

	mmc->tx_pkts_untaged =
		update_macsec_mmc_val(osi_core, TX_PKTS_UNTG_LO_0);
	mmc->tx_pkts_too_long =
		update_macsec_mmc_val(osi_core, TX_PKTS_TOO_LONG_LO_0);
	mmc->tx_octets_protected =
		update_macsec_mmc_val(osi_core, TX_OCTETS_PRTCTD_LO_0);
	mmc->rx_pkts_no_tag =
		update_macsec_mmc_val(osi_core, RX_PKTS_NOTG_LO_0);
	mmc->rx_pkts_untagged =
		update_macsec_mmc_val(osi_core, RX_PKTS_UNTG_LO_0);
	mmc->rx_pkts_bad_tag =
		update_macsec_mmc_val(osi_core, RX_PKTS_BADTAG_LO_0);
	mmc->rx_pkts_no_sa_err =
		update_macsec_mmc_val(osi_core, RX_PKTS_NOSAERROR_LO_0);
	mmc->rx_pkts_no_sa =
		update_macsec_mmc_val(osi_core, RX_PKTS_NOSA_LO_0);
	mmc->rx_pkts_overrun =
		update_macsec_mmc_val(osi_core, RX_PKTS_OVRRUN_LO_0);
	mmc->rx_octets_validated =
		update_macsec_mmc_val(osi_core, RX_OCTETS_VLDTD_LO_0);

	for (i = 0; i <= SC_INDEX_MAX; i++) {
		mmc->tx_pkts_protected[i] =
			update_macsec_mmc_val(osi_core,
					      TX_PKTS_PROTECTED_SCx_LO_0(i));
		mmc->rx_pkts_late[i] =
			update_macsec_mmc_val(osi_core,
					      RX_PKTS_LATE_SCx_LO_0(i));
		mmc->rx_pkts_delayed[i] = mmc->rx_pkts_late[i];
		mmc->rx_pkts_not_valid[i] =
			update_macsec_mmc_val(osi_core,
					      RX_PKTS_NOTVALID_SCx_LO_0(i));
		mmc->in_pkts_invalid[i] = mmc->rx_pkts_not_valid[i];
		mmc->rx_pkts_unchecked[i] = mmc->rx_pkts_not_valid[i];
		mmc->rx_pkts_ok[i] =
			update_macsec_mmc_val(osi_core,
					      RX_PKTS_OK_SCx_LO_0(i));
	}
}

int macsec_enable(struct osi_core_priv_data *osi_core, unsigned int enable)
{
	unsigned int val;
	unsigned char *base = (unsigned char *)osi_core->macsec_base;

	val = osi_readl(base + MACSEC_CONTROL0);
	pr_err("Read MACSEC_CONTROL0: 0x%x\n", val);

	if ((enable & OSI_MACSEC_TX_EN) == OSI_MACSEC_TX_EN) {
		pr_err("\tEnabling macsec TX");
		val |= (TX_EN);
	} else {
		pr_err("\tDisabling macsec TX");
		val &= ~(TX_EN);
	}

	if ((enable & OSI_MACSEC_RX_EN) == OSI_MACSEC_RX_EN) {
		pr_err("\tEnabling macsec RX");
		val |= (RX_EN);
	} else {
		pr_err("\tDisabling macsec RX");
		val &= ~(RX_EN);
	}

	pr_err("Write MACSEC_CONTROL0: 0x%x\n", val);
	osi_writel(val, base + MACSEC_CONTROL0);

	return 0;
}

/**
 * @brief poll_for_kt_update - Query the status of a KT update.
 *
 * @param[in] osi_core: OSI Core private data structure.
 *
 * @retval 0 on Success
 * @retval -1 on Failure
 */
static inline int poll_for_kt_update(struct osi_core_priv_data *osi_core)
{
	/* half sec timeout */
	unsigned int retry = 50000;
	unsigned int kt_config;
	unsigned int count;
	int cond = 1;

	count = 0;
	while (cond == 1) {
		if (count > retry) {
			OSI_CORE_ERR(osi_core->osd,
				OSI_LOG_ARG_HW_FAIL,
				"KT update timed out\n",
				0ULL);
			return -1;
		}

		count++;

		kt_config = osi_readl((unsigned char *)osi_core->tz_base +
				       GCM_KEYTABLE_CONFIG);
		if ((kt_config & KT_CONFIG_UPDATE) == 0U) {
			/* exit loop */
			cond = 0;
		} else {
			/* wait on UPDATE bit to reset */
			osi_core->osd_ops.udelay(10U);
		}
	}

	return 0;
}

static int kt_key_read(struct osi_core_priv_data *const osi_core,
			struct osi_macsec_kt_config *const kt_config)
{
	unsigned int kt_key[MACSEC_KT_DATA_REG_CNT] = {0};
	int i, j;

	for (i = 0; i < MACSEC_KT_DATA_REG_CNT; i++) {
		kt_key[i] = osi_readl((unsigned char *)osi_core->tz_base +
				      GCM_KEYTABLE_DATA(i));
	}

	if ((kt_key[MACSEC_KT_DATA_REG_CNT - 1] & KT_ENTRY_VALID) ==
	     KT_ENTRY_VALID) {
		kt_config->flags |= LUT_FLAGS_ENTRY_VALID;
	}

	for (i = 0; i < MACSEC_KT_DATA_REG_SAK_CNT; i++) {
		for (j = 0; j < INTEGER_LEN; j++) {
			kt_config->entry.sak[i * 4 + j] =
			(kt_key[i] >> (j * 8) & 0xFF);
		}
	}

	for (i = 0; i < MACSEC_KT_DATA_REG_H_CNT; i++) {
		for (j = 0; j < INTEGER_LEN; j++) {
			kt_config->entry.h[i * 4 + j] =
			(kt_key[i + MACSEC_KT_DATA_REG_SAK_CNT] >> (j * 8)
			 & 0xFF);
		}
	}

	return 0;
}

static int kt_key_write(struct osi_core_priv_data *const osi_core,
			struct osi_macsec_kt_config *const kt_config)
{
	unsigned int kt_key[MACSEC_KT_DATA_REG_CNT] = {0};
	struct kt_entry entry = kt_config->entry;
	int i, j;

	/* write SAK */
	for (i = 0; i < MACSEC_KT_DATA_REG_SAK_CNT; i++) {
		/* 4-bytes in each register */
		for (j = 0; j < INTEGER_LEN; j++) {
			kt_key[i] |= (entry.sak[i * 4 + j] << (j * 8));
		}
	}
	/* write H-key */
	for (i = 0; i < MACSEC_KT_DATA_REG_H_CNT; i++) {
		/* 4-bytes in each register */
		for (j = 0; j < INTEGER_LEN; j++) {
			kt_key[i + MACSEC_KT_DATA_REG_SAK_CNT] |=
				(entry.h[i * 4 + j] << (j * 8));
		}
	}

	if ((kt_config->flags & LUT_FLAGS_ENTRY_VALID) ==
	     LUT_FLAGS_ENTRY_VALID) {
		kt_key[MACSEC_KT_DATA_REG_CNT - 1] |= KT_ENTRY_VALID;
	}

	for (i = 0; i < MACSEC_KT_DATA_REG_CNT; i++) {
		/* pr_err("%s: kt_key[%d]: 0x%x\n", __func__, i, kt_key[i]); */
		osi_writel(kt_key[i], (unsigned char *)osi_core->tz_base +
				      GCM_KEYTABLE_DATA(i));
	}

	return 0;
}

static int macsec_kt_config(struct osi_core_priv_data *const osi_core,
			    struct osi_macsec_kt_config *const kt_config)
{
	int ret = 0;
	unsigned int kt_config_reg = 0;
	unsigned char *base = (unsigned char *)osi_core->tz_base;

	/* Validate KT config */
	if ((kt_config->table_config.ctlr_sel > CTLR_SEL_MAX) ||
	    (kt_config->table_config.rw > RW_MAX) ||
	    (kt_config->table_config.index > TABLE_INDEX_MAX)) {
		/* TODO - validate using a local cache if index is
		 * already active */
		return -1;
	}

	/* Wait for previous KT update to finish */
	ret = poll_for_kt_update(osi_core);
	if (ret < 0) {
		return ret;
	}

	/* pr_err("%s: ctrl: %hu rw: %hu idx: %hu flags: %#x\n", __func__,
		kt_config->table_config.ctlr_sel,
		kt_config->table_config.rw, kt_config->table_config.index,
		kt_config->flags); */
	kt_config_reg = osi_readl(base + GCM_KEYTABLE_CONFIG);
	if (kt_config->table_config.ctlr_sel) {
		kt_config_reg |= KT_CONFIG_CTLR_SEL;
	} else {
		kt_config_reg &= ~KT_CONFIG_CTLR_SEL;
	}

	if (kt_config->table_config.rw) {
		kt_config_reg |= KT_CONFIG_RW;
		/* For write operation, load the lut_data registers */
		ret = kt_key_write(osi_core, kt_config);
		if (ret < 0) {
			return ret;
		}
	} else {
		kt_config_reg &= ~KT_CONFIG_RW;
	}

	kt_config_reg &= ~KT_CONFIG_INDEX_MASK;
	kt_config_reg |= (kt_config->table_config.index);

	kt_config_reg |= KT_CONFIG_UPDATE;
	osi_writel(kt_config_reg, base + GCM_KEYTABLE_CONFIG);

	/* Wait for this KT update to finish */
	ret = poll_for_kt_update(osi_core);
	if (ret < 0) {
		return ret;
	}

	if (!kt_config->table_config.rw) {
		ret = kt_key_read(osi_core, kt_config);
		if (ret < 0) {
			return ret;
		}
	}
	return ret;
}

/**
 * @brief poll_for_lut_update - Query the status of a LUT update.
 *
 * @param[in] osi_core: OSI Core private data structure.
 *
 * @retval 0 on Success
 * @retval -1 on Failure
 */
static inline int poll_for_lut_update(struct osi_core_priv_data *osi_core)
{
	/* half sec timeout */
	unsigned int retry = 50000;
	unsigned int lut_config;
	unsigned int count;
	int cond = 1;

	count = 0;
	while (cond == 1) {
		if (count > retry) {
			OSI_CORE_ERR(osi_core->osd,
				OSI_LOG_ARG_HW_FAIL,
				"LUT update timed out\n",
				0ULL);
			return -1;
		}

		count++;

		lut_config = osi_readl((unsigned char *)osi_core->macsec_base +
				       MACSEC_LUT_CONFIG);
		if ((lut_config & LUT_CONFIG_UPDATE) == 0U) {
			/* exit loop */
			cond = 0;
		} else {
			/* wait on UPDATE bit to reset */
			osi_core->osd_ops.udelay(10U);
		}
	}

	return 0;
}

static inline void read_lut_data(struct osi_core_priv_data *const osi_core,
				 unsigned int *const lut_data)
{
	unsigned char *base = (unsigned char *)osi_core->macsec_base;
	int i;

	/* Commit the LUT entry to HW */
	for (i = 0; i < MACSEC_LUT_DATA_REG_CNT; i++) {
		lut_data[i] = osi_readl(base + MACSEC_LUT_DATA(i));
		//pr_err("%s: lut_data[%d]: 0x%x\n", __func__, i, lut_data[i]);
	}
}

static int lut_read_inputs(struct osi_macsec_lut_config *const lut_config,
			   unsigned int *const lut_data)
{
	struct lut_inputs entry = {0};
	unsigned int flags = 0;

	/* MAC DA */
	if ((lut_data[1] & LUT_DA_BYTE0_INACTIVE) != LUT_DA_BYTE0_INACTIVE) {
		entry.da[0] = lut_data[0] & 0xFF;
		flags |= LUT_FLAGS_DA_BYTE0_VALID;
	}

	if ((lut_data[1] & LUT_DA_BYTE1_INACTIVE) != LUT_DA_BYTE1_INACTIVE) {
		entry.da[1] = lut_data[0] >> 8 & 0xFF;
		flags |= LUT_FLAGS_DA_BYTE1_VALID;
	}

	if ((lut_data[1] & LUT_DA_BYTE2_INACTIVE) != LUT_DA_BYTE2_INACTIVE) {
		entry.da[2] = lut_data[0] >> 16 & 0xFF;
		flags |= LUT_FLAGS_DA_BYTE2_VALID;
	}

	if ((lut_data[1] & LUT_DA_BYTE3_INACTIVE) != LUT_DA_BYTE3_INACTIVE) {
		entry.da[3] = lut_data[0] >> 24  & 0xFF;
		flags |= LUT_FLAGS_DA_BYTE3_VALID;
	}

	if ((lut_data[1] & LUT_DA_BYTE4_INACTIVE) != LUT_DA_BYTE4_INACTIVE) {
		entry.da[4] = lut_data[1] & 0xFF;
		flags |= LUT_FLAGS_DA_BYTE4_VALID;
	}

	if ((lut_data[1] & LUT_DA_BYTE5_INACTIVE) != LUT_DA_BYTE5_INACTIVE) {
		entry.da[5] = lut_data[1] >> 8 & 0xFF;
		flags |= LUT_FLAGS_DA_BYTE5_VALID;
	}

	/* MAC SA */
	if ((lut_data[3] & LUT_SA_BYTE0_INACTIVE) != LUT_SA_BYTE0_INACTIVE) {
		entry.sa[0] = lut_data[1] >> 22 & 0xFF;
		flags |= LUT_FLAGS_SA_BYTE0_VALID;
	}

	if ((lut_data[3] & LUT_SA_BYTE1_INACTIVE) != LUT_SA_BYTE1_INACTIVE) {
		entry.sa[1] = (lut_data[1] >> 30) |
			      ((lut_data[2] & 0x3F) << 2);
		flags |= LUT_FLAGS_SA_BYTE1_VALID;
	}

	if ((lut_data[3] & LUT_SA_BYTE2_INACTIVE) != LUT_SA_BYTE2_INACTIVE) {
		entry.sa[2] = lut_data[2] >> 6 & 0xFF;
		flags |= LUT_FLAGS_SA_BYTE2_VALID;
	}

	if ((lut_data[3] & LUT_SA_BYTE3_INACTIVE) != LUT_SA_BYTE3_INACTIVE) {
		entry.sa[3] = lut_data[2] >> 14 & 0xFF;
		flags |= LUT_FLAGS_SA_BYTE3_VALID;
	}

	if ((lut_data[3] & LUT_SA_BYTE4_INACTIVE) != LUT_SA_BYTE4_INACTIVE) {
		entry.sa[4] = lut_data[2] >> 22 & 0xFF;
		flags |= LUT_FLAGS_SA_BYTE4_VALID;
	}

	if ((lut_data[3] & LUT_SA_BYTE5_INACTIVE) != LUT_SA_BYTE5_INACTIVE) {
		entry.sa[5] = (lut_data[2] >> 30) |
			      ((lut_data[3] & 0x3F) << 2);
		flags |= LUT_FLAGS_SA_BYTE5_VALID;
	}

	/* Ether type */
	if ((lut_data[3] & LUT_ETHTYPE_INACTIVE) != LUT_ETHTYPE_INACTIVE) {
		entry.ethtype[0] = lut_data[3] >> 12 & 0xFF;
		entry.ethtype[1] = lut_data[3] >> 20 & 0xFF;
		flags |= LUT_FLAGS_ETHTYPE_VALID;
	}

	/* VLAN */
	if ((lut_data[4] & LUT_VLAN_ACTIVE) == LUT_VLAN_ACTIVE) {
		flags |= LUT_FLAGS_VLAN_VALID;
		/* VLAN PCP */
		if ((lut_data[4] & LUT_VLAN_PCP_INACTIVE) !=
		    LUT_VLAN_PCP_INACTIVE) {
			flags |= LUT_FLAGS_VLAN_PCP_VALID;
			entry.vlan_pcp = lut_data[3] >> 29;
		}

		/* VLAN ID */
		if ((lut_data[4] & LUT_VLAN_ID_INACTIVE) !=
		    LUT_VLAN_ID_INACTIVE) {
			flags |= LUT_FLAGS_VLAN_ID_VALID;
			entry.vlan_id = lut_data[4] >> 1 & 0xFFF;
		}
	}

	/* Byte patterns */
	if ((lut_data[4] & LUT_BYTE0_PATTERN_INACTIVE) !=
	    LUT_BYTE0_PATTERN_INACTIVE) {
		flags |= LUT_FLAGS_BYTE0_PATTERN_VALID;
		entry.byte_pattern[0] = lut_data[4] >> 15 & 0xFF;
		entry.byte_pattern_offset[0] = lut_data[4] >> 23 & 0x3F;
	}
	if ((lut_data[5] & LUT_BYTE1_PATTERN_INACTIVE) !=
	    LUT_BYTE1_PATTERN_INACTIVE) {
		flags |= LUT_FLAGS_BYTE1_PATTERN_VALID;
		entry.byte_pattern[1] = (lut_data[4] >> 30) |
					((lut_data[5] & 0x3F) << 2);
		entry.byte_pattern_offset[1] = lut_data[5] >> 6 & 0x3F;
	}
	if ((lut_data[5] & LUT_BYTE2_PATTERN_INACTIVE) !=
	    LUT_BYTE2_PATTERN_INACTIVE) {
		flags |= LUT_FLAGS_BYTE2_PATTERN_VALID;
		entry.byte_pattern[2] = lut_data[5] >> 13 & 0xFF;
		entry.byte_pattern_offset[2] = lut_data[5] >> 21 & 0x3F;
	}
	if ((lut_data[6] & LUT_BYTE3_PATTERN_INACTIVE) !=
	    LUT_BYTE3_PATTERN_INACTIVE) {
		flags |= LUT_FLAGS_BYTE3_PATTERN_VALID;
		entry.byte_pattern[3] = (lut_data[5] >> 28) |
					((lut_data[6] & 0xF) << 4);
		entry.byte_pattern_offset[3] = lut_data[6] >> 4 & 0x3F;
	}

	/* Preempt mask */
	if ((lut_data[6] & LUT_PREEMPT_INACTIVE) != LUT_PREEMPT_INACTIVE) {
		flags |= LUT_FLAGS_PREEMPT_VALID;
		if ((lut_data[6] & LUT_PREEMPT) == LUT_PREEMPT) {
			flags |= LUT_FLAGS_PREEMPT;
		}
	}

	lut_config->lut_in = entry;
	lut_config->flags = flags;

	return 0;
}

static int byp_lut_read(struct osi_core_priv_data *const osi_core,
			struct osi_macsec_lut_config *const lut_config)
{
	unsigned int lut_data[MACSEC_LUT_DATA_REG_CNT] = {0};
	unsigned int flags = 0, val = 0;
	unsigned int index = lut_config->table_config.index;
	unsigned char *addr = (unsigned char *)osi_core->macsec_base;
	unsigned char *paddr = OSI_NULL;

	read_lut_data(osi_core, lut_data);

	if (lut_read_inputs(lut_config, lut_data) != 0) {
		pr_err("LUT inputs error\n");
		return -1;
	}

	/* Lookup output */
	if ((lut_data[6] & LUT_CONTROLLED_PORT) == LUT_CONTROLLED_PORT) {
		flags |= LUT_FLAGS_CONTROLLED_PORT;
	}

	if ((lut_data[6] & BYP_LUT_DVLAN_PKT) == BYP_LUT_DVLAN_PKT) {
		flags |= LUT_FLAGS_DVLAN_PKT;
	}

	if ((lut_data[6] & BYP_LUT_DVLAN_OUTER_INNER_TAG_SEL) ==
		BYP_LUT_DVLAN_OUTER_INNER_TAG_SEL) {
		flags |= LUT_FLAGS_DVLAN_OUTER_INNER_TAG_SEL;
	}

	switch (lut_config->table_config.ctlr_sel) {
	case CTLR_SEL_TX:
		paddr = addr+TX_BYP_LUT_VALID;
		break;
	case CTLR_SEL_RX:
		paddr = addr+RX_BYP_LUT_VALID;
		break;
	default:
		pr_err("Unknown controller select\n");
		return -1;
	}
	val = osi_readl(paddr);
	if (val & (1U << index)) {
		flags |= LUT_FLAGS_ENTRY_VALID;
	}

	lut_config->flags |= flags;

	return 0;
}

static int sci_lut_read(struct osi_core_priv_data *const osi_core,
			struct osi_macsec_lut_config *const lut_config)
{
	unsigned int lut_data[MACSEC_LUT_DATA_REG_CNT] = {0};
	unsigned int flags = 0;
	unsigned char *addr = (unsigned char *)osi_core->macsec_base;
	unsigned int val = 0;
	unsigned int index = lut_config->table_config.index;

	if (index > SC_LUT_MAX_INDEX) {
		return -1;
	}
	read_lut_data(osi_core, lut_data);

	switch (lut_config->table_config.ctlr_sel) {
	case CTLR_SEL_TX:
		if (lut_read_inputs(lut_config, lut_data) != 0) {
			pr_err("LUT inputs error\n");
			return -1;
		}
		if ((lut_data[6] & LUT_AN0_VALID) == LUT_AN0_VALID) {
			lut_config->sci_lut_out.an_valid |= AN0_VALID;
		}
		if ((lut_data[6] & LUT_AN1_VALID) == LUT_AN1_VALID) {
			lut_config->sci_lut_out.an_valid |= AN1_VALID;
		}
		if ((lut_data[6] & LUT_AN2_VALID) == LUT_AN2_VALID) {
			lut_config->sci_lut_out.an_valid |= AN2_VALID;
		}
		if ((lut_data[6] & LUT_AN3_VALID) == LUT_AN3_VALID) {
			lut_config->sci_lut_out.an_valid |= AN3_VALID;
		}

		lut_config->sci_lut_out.sc_index = lut_data[6] >> 17 & 0xF;

		if ((lut_data[6] & TX_SCI_LUT_DVLAN_PKT) ==
		    TX_SCI_LUT_DVLAN_PKT) {
			lut_config->flags |= LUT_FLAGS_DVLAN_PKT;
		}
		if ((lut_data[6] & TX_SCI_LUT_DVLAN_OUTER_INNER_TAG_SEL) ==
			TX_SCI_LUT_DVLAN_OUTER_INNER_TAG_SEL) {
			lut_config->flags |=
				LUT_FLAGS_DVLAN_OUTER_INNER_TAG_SEL;
		}

		val = osi_readl(addr+TX_SCI_LUT_VALID);
		if (val & (1U << index)) {
			lut_config->flags |= LUT_FLAGS_ENTRY_VALID;
		}
		break;
	case CTLR_SEL_RX:
		lut_config->sci_lut_out.sci[0] = lut_data[0] & 0xFF;
		lut_config->sci_lut_out.sci[1] = lut_data[0] >> 8 & 0xFF;
		lut_config->sci_lut_out.sci[2] = lut_data[0] >> 16 & 0xFF;
		lut_config->sci_lut_out.sci[3] = lut_data[0] >> 24 & 0xFF;
		lut_config->sci_lut_out.sci[4] = lut_data[1] & 0xFF;
		lut_config->sci_lut_out.sci[5] = lut_data[1] >> 8 & 0xFF;
		lut_config->sci_lut_out.sci[6] = lut_data[1] >> 16 & 0xFF;
		lut_config->sci_lut_out.sci[7] = lut_data[1] >> 24 & 0xFF;

		lut_config->sci_lut_out.sc_index = lut_data[2] >> 10 & 0xF;
		if ((lut_data[2] & RX_SCI_LUT_PREEMPT_INACTIVE) !=
		    RX_SCI_LUT_PREEMPT_INACTIVE) {
			flags |= LUT_FLAGS_PREEMPT_VALID;
			if ((lut_data[2] & RX_SCI_LUT_PREEMPT) ==
			    RX_SCI_LUT_PREEMPT) {
				flags |= LUT_FLAGS_PREEMPT;
			}
		}

		val = osi_readl(addr+RX_SCI_LUT_VALID);
		if (val & (1U << index)) {
			lut_config->flags |= LUT_FLAGS_ENTRY_VALID;
		}
		break;
	default:
		pr_err("Unknown controller selected\n");
		return -1;
	}

	/* Lookup output */
	return 0;
}

static int sc_param_lut_read(struct osi_core_priv_data *const osi_core,
			     struct osi_macsec_lut_config *const lut_config)
{
	unsigned int lut_data[MACSEC_LUT_DATA_REG_CNT] = {0};

	read_lut_data(osi_core, lut_data);

	switch (lut_config->table_config.ctlr_sel) {
	case CTLR_SEL_TX:
		lut_config->sc_param_out.key_index_start = lut_data[0] & 0x1F;
		lut_config->sc_param_out.pn_max = (lut_data[0] >> 5) |
						   (lut_data[1] << 27);
		lut_config->sc_param_out.pn_threshold = (lut_data[1] >> 5) |
							(lut_data[2] << 27);
		lut_config->sc_param_out.tci = (lut_data[2] >> 5) & 0x3;
		lut_config->sc_param_out.sci[0] = lut_data[2] >> 8 & 0xFF;
		lut_config->sc_param_out.sci[1] = lut_data[2] >> 16 & 0xFF;
		lut_config->sc_param_out.sci[2] = lut_data[2] >> 24 & 0xFF;
		lut_config->sc_param_out.sci[3] = lut_data[3] & 0xFF;
		lut_config->sc_param_out.sci[4] = lut_data[3] >> 8 & 0xFF;
		lut_config->sc_param_out.sci[5] = lut_data[3] >> 16 & 0xFF;
		lut_config->sc_param_out.sci[6] = lut_data[3] >> 24 & 0xFF;
		lut_config->sc_param_out.sci[7] = lut_data[4] & 0xFF;
		lut_config->sc_param_out.vlan_in_clear =
						(lut_data[4] >> 8) & 0x1;
		break;
	case CTLR_SEL_RX:
		lut_config->sc_param_out.key_index_start = lut_data[0] & 0x1F;
		lut_config->sc_param_out.pn_window = (lut_data[0] >> 5) |
						     (lut_data[1] << 27);
		lut_config->sc_param_out.pn_max = (lut_data[1] >> 5) |
						     (lut_data[2] << 27);
		break;
	default:
		pr_err("Unknown controller selected\n");
		return -1;
	}

	/* Lookup output */
	return 0;
}

static int sc_state_lut_read(struct osi_core_priv_data *const osi_core,
			     struct osi_macsec_lut_config *const lut_config)
{
	unsigned int lut_data[MACSEC_LUT_DATA_REG_CNT] = {0};

	read_lut_data(osi_core, lut_data);
	lut_config->sc_state_out.curr_an = lut_data[0];

	return 0;
}

static int sa_state_lut_read(struct osi_core_priv_data *const osi_core,
			     struct osi_macsec_lut_config *const lut_config)
{
	unsigned int lut_data[MACSEC_LUT_DATA_REG_CNT] = {0};

	read_lut_data(osi_core, lut_data);

	switch (lut_config->table_config.ctlr_sel) {
	case CTLR_SEL_TX:
		lut_config->sa_state_out.next_pn = lut_data[0];
		if ((lut_data[1] & SA_STATE_LUT_ENTRY_VALID) ==
		    SA_STATE_LUT_ENTRY_VALID) {
			lut_config->flags |= LUT_FLAGS_ENTRY_VALID;
		}
		break;
	case CTLR_SEL_RX:
		lut_config->sa_state_out.next_pn = lut_data[0];
		lut_config->sa_state_out.lowest_pn = lut_data[1];
		break;
	default:
		pr_err("Unknown controller selected\n");
		return -1;
	}

	/* Lookup output */
	return 0;
}

static int lut_data_read(struct osi_core_priv_data *const osi_core,
			 struct osi_macsec_lut_config *const lut_config)
{
	switch (lut_config->lut_sel) {
	case LUT_SEL_BYPASS:
		if (byp_lut_read(osi_core, lut_config) != 0) {
			pr_err("BYP LUT read err\n");
			return -1;
		}
		break;
	case LUT_SEL_SCI:
		if (sci_lut_read(osi_core, lut_config) != 0) {
			pr_err("SCI LUT read err\n");
			return -1;
		}
		break;
	case LUT_SEL_SC_PARAM:
		if (sc_param_lut_read(osi_core, lut_config) != 0) {
			pr_err("SC param LUT read err\n");
			return -1;
		}
		break;
	case LUT_SEL_SC_STATE:
		if (sc_state_lut_read(osi_core, lut_config) != 0) {
			pr_err("SC state LUT read err\n");
			return -1;
		}
		break;
	case LUT_SEL_SA_STATE:
		if (sa_state_lut_read(osi_core, lut_config) != 0) {
			pr_err("SA state LUT read err\n");
			return -1;
		}
		break;
	default:
		//Unsupported LUT
		return -1;
	}

	return 0;
}

static inline void commit_lut_data(struct osi_core_priv_data *const osi_core,
				   unsigned int const *const lut_data)
{
	unsigned char *base = (unsigned char *)osi_core->macsec_base;
	int i;

	/* Commit the LUT entry to HW */
	for (i = 0; i < MACSEC_LUT_DATA_REG_CNT; i++) {
		//pr_err("%s: lut_data[%d]: 0x%x\n", __func__, i, lut_data[i]);
		osi_writel(lut_data[i], base + MACSEC_LUT_DATA(i));
	}
}

static void rx_sa_state_lut_config(
				struct osi_macsec_lut_config *const lut_config,
				unsigned int *const lut_data)
{
	struct sa_state_outputs entry = lut_config->sa_state_out;

	lut_data[0] |= entry.next_pn;
	lut_data[1] |= entry.lowest_pn;
}

static void tx_sa_state_lut_config(
				struct osi_macsec_lut_config *const lut_config,
				unsigned int *const lut_data)
{
	unsigned int flags = lut_config->flags;
	struct sa_state_outputs entry = lut_config->sa_state_out;

	lut_data[0] |= entry.next_pn;
	if ((flags & LUT_FLAGS_ENTRY_VALID) == LUT_FLAGS_ENTRY_VALID) {
		lut_data[1] |= SA_STATE_LUT_ENTRY_VALID;
	}

}

static int sa_state_lut_config(struct osi_core_priv_data *const osi_core,
			       struct osi_macsec_lut_config *const lut_config)
{
	unsigned int lut_data[MACSEC_LUT_DATA_REG_CNT] = {0};
	struct macsec_table_config table_config = lut_config->table_config;

	switch (table_config.ctlr_sel) {
	case CTLR_SEL_TX:
		tx_sa_state_lut_config(lut_config, lut_data);
		break;
	case CTLR_SEL_RX:
		rx_sa_state_lut_config(lut_config, lut_data);
		break;
	default:
		return -1;
	}

	commit_lut_data(osi_core, lut_data);

	return 0;
}

static int sc_state_lut_config(struct osi_core_priv_data *const osi_core,
			       struct osi_macsec_lut_config *const lut_config)
{
	unsigned int lut_data[MACSEC_LUT_DATA_REG_CNT] = {0};
	struct sc_state_outputs entry = lut_config->sc_state_out;

	lut_data[0] |= entry.curr_an;
	commit_lut_data(osi_core, lut_data);

	return 0;
}

static void rx_sc_param_lut_config(
			struct osi_macsec_lut_config *const lut_config,
			unsigned int *const lut_data)
{
	struct sc_param_outputs entry = lut_config->sc_param_out;

	lut_data[0] |= entry.key_index_start;
	lut_data[0] |= entry.pn_window << 5;
	lut_data[1] |= entry.pn_window >> 27;
	lut_data[1] |= entry.pn_max << 5;
	lut_data[2] |= entry.pn_max >> 27;
}

static void tx_sc_param_lut_config(
			struct osi_macsec_lut_config *const lut_config,
			unsigned int *const lut_data)
{
	struct sc_param_outputs entry = lut_config->sc_param_out;

	lut_data[0] |= entry.key_index_start;
	lut_data[0] |= entry.pn_max << 5;
	lut_data[1] |= entry.pn_max >> 27;
	lut_data[1] |= entry.pn_threshold << 5;
	lut_data[2] |= entry.pn_threshold >> 27;
	lut_data[2] |= entry.tci << 5;
	lut_data[2] |= entry.sci[0] << 8;
	lut_data[2] |= entry.sci[1] << 16;
	lut_data[2] |= entry.sci[2] << 24;
	lut_data[3] |= entry.sci[3];
	lut_data[3] |= entry.sci[4] << 8;
	lut_data[3] |= entry.sci[5] << 16;
	lut_data[3] |= entry.sci[6] << 24;
	lut_data[4] |= entry.sci[7];
	lut_data[4] |= entry.vlan_in_clear << 8;
}

static int sc_param_lut_config(struct osi_core_priv_data *const osi_core,
			       struct osi_macsec_lut_config *const lut_config)
{
	unsigned int lut_data[MACSEC_LUT_DATA_REG_CNT] = {0};
	struct macsec_table_config table_config = lut_config->table_config;
	struct sc_param_outputs entry = lut_config->sc_param_out;

	if (entry.key_index_start > KEY_INDEX_MAX) {
		return -1;
	}

	switch (table_config.ctlr_sel) {
	case CTLR_SEL_TX:
		tx_sc_param_lut_config(lut_config, lut_data);
		break;
	case CTLR_SEL_RX:
		rx_sc_param_lut_config(lut_config, lut_data);
		break;
	}

	commit_lut_data(osi_core, lut_data);

	return 0;
}

static int lut_config_inputs(struct osi_macsec_lut_config *const lut_config,
			     unsigned int *const lut_data)
{
	struct lut_inputs entry = lut_config->lut_in;
	unsigned int flags;
	int i;

	for (i = 0; i < LUT_BYTE_PATTERN_MAX; i++) {
		if (entry.byte_pattern_offset[i] >
		    LUT_BYTE_PATTERN_MAX_OFFSET) {
			return -1;
		}
	}

	if ((entry.vlan_pcp > VLAN_PCP_MAX) ||
	    (entry.vlan_id > VLAN_ID_MAX)) {
		return -1;
	}

	/* TODO - validate if LUT_FLAGS_VLAN_VALID is incorrectly set
	 * when LUT_FLAGS_VLAN_PCP_VALID/VLAN_ID_VALID is not set.
	 */

	/* TODO - validate if Byte pattern and byte pattern are both provided */

	flags = lut_config->flags;
	/* MAC DA */
	if ((flags & LUT_FLAGS_DA_BYTE0_VALID) == LUT_FLAGS_DA_BYTE0_VALID) {
		lut_data[0] |= entry.da[0];
		lut_data[1] &= ~LUT_DA_BYTE0_INACTIVE;
	} else {
		lut_data[1] |= LUT_DA_BYTE0_INACTIVE;
	}

	if ((flags & LUT_FLAGS_DA_BYTE1_VALID) == LUT_FLAGS_DA_BYTE1_VALID) {
		lut_data[0] |= entry.da[1] << 8;
		lut_data[1] &= ~LUT_DA_BYTE1_INACTIVE;
	} else {
		lut_data[1] |= LUT_DA_BYTE1_INACTIVE;
	}

	if ((flags & LUT_FLAGS_DA_BYTE2_VALID) == LUT_FLAGS_DA_BYTE2_VALID) {
		lut_data[0] |= entry.da[2] << 16;
		lut_data[1] &= ~LUT_DA_BYTE2_INACTIVE;
	} else {
		lut_data[1] |= LUT_DA_BYTE2_INACTIVE;
	}

	if ((flags & LUT_FLAGS_DA_BYTE3_VALID) == LUT_FLAGS_DA_BYTE3_VALID) {
		lut_data[0] |= entry.da[3] << 24;
		lut_data[1] &= ~LUT_DA_BYTE3_INACTIVE;
	} else {
		lut_data[1] |= LUT_DA_BYTE3_INACTIVE;
	}

	if ((flags & LUT_FLAGS_DA_BYTE4_VALID) == LUT_FLAGS_DA_BYTE4_VALID) {
		lut_data[1] |= entry.da[4];
		lut_data[1] &= ~LUT_DA_BYTE4_INACTIVE;
	} else {
		lut_data[1] |= LUT_DA_BYTE4_INACTIVE;
	}

	if ((flags & LUT_FLAGS_DA_BYTE5_VALID) == LUT_FLAGS_DA_BYTE5_VALID) {
		lut_data[1] |= entry.da[5] << 8;
		lut_data[1] &= ~LUT_DA_BYTE5_INACTIVE;
	} else {
		lut_data[1] |= LUT_DA_BYTE5_INACTIVE;
	}

	/* MAC SA */
	if ((flags & LUT_FLAGS_SA_BYTE0_VALID) == LUT_FLAGS_SA_BYTE0_VALID) {
		lut_data[1] |= entry.sa[0] << 22;
		lut_data[3] &= ~LUT_SA_BYTE0_INACTIVE;
	} else {
		lut_data[3] |= LUT_SA_BYTE0_INACTIVE;
	}

	if ((flags & LUT_FLAGS_SA_BYTE1_VALID) == LUT_FLAGS_SA_BYTE1_VALID) {
		lut_data[1] |= entry.sa[1] << 30;
		lut_data[2] |= (entry.sa[1] >> 2);
		lut_data[3] &= ~LUT_SA_BYTE1_INACTIVE;
	} else {
		lut_data[3] |= LUT_SA_BYTE1_INACTIVE;
	}

	if ((flags & LUT_FLAGS_SA_BYTE2_VALID) == LUT_FLAGS_SA_BYTE2_VALID) {
		lut_data[2] |= entry.sa[2] << 6;
		lut_data[3] &= ~LUT_SA_BYTE2_INACTIVE;
	} else {
		lut_data[3] |= LUT_SA_BYTE2_INACTIVE;
	}

	if ((flags & LUT_FLAGS_SA_BYTE3_VALID) == LUT_FLAGS_SA_BYTE3_VALID) {
		lut_data[2] |= entry.sa[3] << 14;
		lut_data[3] &= ~LUT_SA_BYTE3_INACTIVE;
	} else {
		lut_data[3] |= LUT_SA_BYTE3_INACTIVE;
	}

	if ((flags & LUT_FLAGS_SA_BYTE4_VALID) == LUT_FLAGS_SA_BYTE4_VALID) {
		lut_data[2] |= entry.sa[4] << 22;
		lut_data[3] &= ~LUT_SA_BYTE4_INACTIVE;
	} else {
		lut_data[3] |= LUT_SA_BYTE4_INACTIVE;
	}

	if ((flags & LUT_FLAGS_SA_BYTE5_VALID) == LUT_FLAGS_SA_BYTE5_VALID) {
		lut_data[2] |= entry.sa[5] << 30;
		lut_data[3] |= (entry.sa[5] >> 2);
		lut_data[3] &= ~LUT_SA_BYTE5_INACTIVE;
	} else {
		lut_data[3] |= LUT_SA_BYTE5_INACTIVE;
	}

	/* Ether type */
	if ((flags & LUT_FLAGS_ETHTYPE_VALID) == LUT_FLAGS_ETHTYPE_VALID) {
		lut_data[3] |= entry.ethtype[0] << 12;
		lut_data[3] |= entry.ethtype[1] << 20;
		lut_data[3] &= ~LUT_ETHTYPE_INACTIVE;
	} else {
		lut_data[3] |= LUT_ETHTYPE_INACTIVE;
	}

	/* VLAN */
	if ((flags & LUT_FLAGS_VLAN_VALID) == LUT_FLAGS_VLAN_VALID) {
		/* VLAN PCP */
		if ((flags & LUT_FLAGS_VLAN_PCP_VALID) ==
		    LUT_FLAGS_VLAN_PCP_VALID) {
			lut_data[3] |= entry.vlan_pcp << 29;
			lut_data[4] &= ~LUT_VLAN_PCP_INACTIVE;
		} else {
			lut_data[4] |= LUT_VLAN_PCP_INACTIVE;
		}

		/* VLAN ID */
		if ((flags & LUT_FLAGS_VLAN_ID_VALID) ==
		    LUT_FLAGS_VLAN_ID_VALID) {
			lut_data[4] |= entry.vlan_id << 1;
			lut_data[4] &= ~LUT_VLAN_ID_INACTIVE;
		} else {
			lut_data[4] |= LUT_VLAN_ID_INACTIVE;
		}
		lut_data[4] |= LUT_VLAN_ACTIVE;
	} else {
		lut_data[4] |= LUT_VLAN_PCP_INACTIVE;
		lut_data[4] |= LUT_VLAN_ID_INACTIVE;
		lut_data[4] &= ~LUT_VLAN_ACTIVE;
	}

	/* Byte patterns */
	if ((flags & LUT_FLAGS_BYTE0_PATTERN_VALID) ==
	    LUT_FLAGS_BYTE0_PATTERN_VALID) {
		lut_data[4] |= entry.byte_pattern[0] << 15;
		lut_data[4] |= entry.byte_pattern_offset[0] << 23;
		lut_data[4] &= ~LUT_BYTE0_PATTERN_INACTIVE;
	} else {
		lut_data[4] |= LUT_BYTE0_PATTERN_INACTIVE;
	}
	if ((flags & LUT_FLAGS_BYTE1_PATTERN_VALID) ==
	    LUT_FLAGS_BYTE1_PATTERN_VALID) {
		lut_data[4] |= entry.byte_pattern[1] << 30;
		lut_data[5] |= entry.byte_pattern[1] >> 2;
		lut_data[5] |= entry.byte_pattern_offset[1] << 6;
		lut_data[5] &= ~LUT_BYTE1_PATTERN_INACTIVE;
	} else {
		lut_data[5] |= LUT_BYTE1_PATTERN_INACTIVE;
	}

	if ((flags & LUT_FLAGS_BYTE2_PATTERN_VALID) ==
	    LUT_FLAGS_BYTE2_PATTERN_VALID) {
		lut_data[5] |= entry.byte_pattern[2] << 13;
		lut_data[5] |= entry.byte_pattern_offset[2] << 21;
		lut_data[5] &= ~LUT_BYTE2_PATTERN_INACTIVE;
	} else {
		lut_data[5] |= LUT_BYTE2_PATTERN_INACTIVE;
	}

	if ((flags & LUT_FLAGS_BYTE3_PATTERN_VALID) ==
	    LUT_FLAGS_BYTE3_PATTERN_VALID) {
		lut_data[5] |= entry.byte_pattern[3] << 28;
		lut_data[6] |= entry.byte_pattern[3] >> 4;
		lut_data[6] |= entry.byte_pattern_offset[3] << 4;
		lut_data[6] &= ~LUT_BYTE3_PATTERN_INACTIVE;
	} else {
		lut_data[6] |= LUT_BYTE3_PATTERN_INACTIVE;
	}

	/* Preempt mask */
	if ((flags & LUT_FLAGS_PREEMPT_VALID) == LUT_FLAGS_PREEMPT_VALID) {
		if ((flags & LUT_FLAGS_PREEMPT) == LUT_FLAGS_PREEMPT) {
			lut_data[6] |= LUT_PREEMPT;
		} else {
			lut_data[6] &= ~LUT_PREEMPT;
		}
		lut_data[6] &= ~LUT_PREEMPT_INACTIVE;
	} else {
		lut_data[6] |= LUT_PREEMPT_INACTIVE;
	}

	return 0;
}

static int rx_sci_lut_config(struct osi_macsec_lut_config *const lut_config,
			     unsigned int *const lut_data)
{
	unsigned int flags = lut_config->flags;
	struct sci_lut_outputs entry = lut_config->sci_lut_out;

	if (entry.sc_index > SC_INDEX_MAX) {
		return -1;
	}

	lut_data[0] |= (entry.sci[0] |
			(entry.sci[1] << 8) |
			(entry.sci[2] << 16) |
			(entry.sci[3] << 24));
	lut_data[1] |= (entry.sci[4] |
			(entry.sci[5] << 8) |
			(entry.sci[6] << 16) |
			(entry.sci[7] << 24));

	/* Preempt mask */
	if ((flags & LUT_FLAGS_PREEMPT_VALID) == LUT_FLAGS_PREEMPT_VALID) {
		if ((flags & LUT_FLAGS_PREEMPT) == LUT_FLAGS_PREEMPT) {
			lut_data[2] |= RX_SCI_LUT_PREEMPT;
		} else {
			lut_data[2] &= ~RX_SCI_LUT_PREEMPT;
		}
		lut_data[2] &= ~RX_SCI_LUT_PREEMPT_INACTIVE;
	} else {
		lut_data[2] |= RX_SCI_LUT_PREEMPT_INACTIVE;
	}

	lut_data[2] |= entry.sc_index << 10;

	return 0;
}

static int tx_sci_lut_config(struct osi_macsec_lut_config *const lut_config,
			     unsigned int *const lut_data)
{
	unsigned int flags = lut_config->flags;
	struct sci_lut_outputs entry = lut_config->sci_lut_out;
	unsigned int an_valid = entry.an_valid;

	if (lut_config_inputs(lut_config, lut_data) != 0) {
		pr_err("LUT inputs error\n");
		return -1;
	}

	/* Lookup result fields */
	if ((an_valid & AN0_VALID) == AN0_VALID) {
		lut_data[6] |= LUT_AN0_VALID;
	}
	if ((an_valid & AN1_VALID) == AN1_VALID) {
		lut_data[6] |= LUT_AN1_VALID;
	}
	if ((an_valid & AN2_VALID) == AN2_VALID) {
		lut_data[6] |= LUT_AN2_VALID;
	}
	if ((an_valid & AN3_VALID) == AN3_VALID) {
		lut_data[6] |= LUT_AN3_VALID;
	}

	lut_data[6] |= entry.sc_index << 17;

	if ((flags & LUT_FLAGS_DVLAN_PKT) == LUT_FLAGS_DVLAN_PKT) {
		lut_data[6] |= TX_SCI_LUT_DVLAN_PKT;
	}

	if ((flags & LUT_FLAGS_DVLAN_OUTER_INNER_TAG_SEL) ==
		LUT_FLAGS_DVLAN_OUTER_INNER_TAG_SEL) {
		lut_data[6] |= TX_SCI_LUT_DVLAN_OUTER_INNER_TAG_SEL;
	}
	return 0;
}

static int sci_lut_config(struct osi_core_priv_data *const osi_core,
			  struct osi_macsec_lut_config *const lut_config)
{
	unsigned int lut_data[MACSEC_LUT_DATA_REG_CNT] = {0};
	struct macsec_table_config table_config = lut_config->table_config;
	struct sci_lut_outputs entry = lut_config->sci_lut_out;
	unsigned char *addr = (unsigned char *)osi_core->macsec_base;
	unsigned int val = 0;
	unsigned int index = lut_config->table_config.index;

	if ((entry.sc_index > SC_INDEX_MAX) ||
		(lut_config->table_config.index > SC_LUT_MAX_INDEX)) {
		return -1;
	}

	switch (table_config.ctlr_sel) {
	case CTLR_SEL_TX:
		if (tx_sci_lut_config(lut_config, lut_data) < 0) {
			pr_err("Failed to config tx sci LUT\n");
			return -1;
		}
		commit_lut_data(osi_core, lut_data);

		if ((lut_config->flags & LUT_FLAGS_ENTRY_VALID) ==
			LUT_FLAGS_ENTRY_VALID) {
			val = osi_readl(addr+TX_SCI_LUT_VALID);
			val |= (1 << index);
			osi_writel(val, addr+TX_SCI_LUT_VALID);
		} else {
			val = osi_readl(addr+TX_SCI_LUT_VALID);
			val &= ~(1 << index);
			osi_writel(val, addr+TX_SCI_LUT_VALID);
		}

		break;
	case CTLR_SEL_RX:
		if (rx_sci_lut_config(lut_config, lut_data) < 0) {
			pr_err("Failed to config rx sci LUT\n");
			return -1;
		}
		commit_lut_data(osi_core, lut_data);

		if ((lut_config->flags & LUT_FLAGS_ENTRY_VALID) ==
			LUT_FLAGS_ENTRY_VALID) {
			val = osi_readl(addr+RX_SCI_LUT_VALID);
			val |= (1 << index);
			osi_writel(val, addr+RX_SCI_LUT_VALID);
		} else {
			val = osi_readl(addr+RX_SCI_LUT_VALID);
			val &= ~(1 << index);
			osi_writel(val, addr+RX_SCI_LUT_VALID);
		}

		break;
	default:
		pr_err("Unknown controller select\n");
		return -1;
	}
	return 0;
}

static int byp_lut_config(struct osi_core_priv_data *const osi_core,
			  struct osi_macsec_lut_config *const lut_config)
{
	unsigned int lut_data[MACSEC_LUT_DATA_REG_CNT] = {0};
	unsigned int flags = lut_config->flags;
	unsigned char *addr = (unsigned char *)osi_core->macsec_base;
	unsigned int val = 0;
	unsigned int index = lut_config->table_config.index;

	if (lut_config_inputs(lut_config, lut_data) != 0) {
		pr_err("LUT inputs error\n");
		return -1;
	}

	/* Lookup output */
	if ((flags & LUT_FLAGS_CONTROLLED_PORT) ==
		LUT_FLAGS_CONTROLLED_PORT) {
		lut_data[6] |= LUT_CONTROLLED_PORT;
	}

	if ((flags & LUT_FLAGS_DVLAN_PKT) == LUT_FLAGS_DVLAN_PKT) {
		lut_data[6] |= BYP_LUT_DVLAN_PKT;
	}

	if ((flags & LUT_FLAGS_DVLAN_OUTER_INNER_TAG_SEL) ==
		LUT_FLAGS_DVLAN_OUTER_INNER_TAG_SEL) {
		lut_data[6] |= BYP_LUT_DVLAN_OUTER_INNER_TAG_SEL;
	}

	commit_lut_data(osi_core, lut_data);

	switch (lut_config->table_config.ctlr_sel) {
	case CTLR_SEL_TX:
		if ((flags & LUT_FLAGS_ENTRY_VALID) ==
		     LUT_FLAGS_ENTRY_VALID) {
			val = osi_readl(addr+TX_BYP_LUT_VALID);
			val |= (1 << index);
			osi_writel(val, addr+TX_BYP_LUT_VALID);
		} else {
			val = osi_readl(addr+TX_BYP_LUT_VALID);
			val &= ~(1 << index);
			osi_writel(val, addr+TX_BYP_LUT_VALID);
		}
		break;

	case CTLR_SEL_RX:
		if ((flags & LUT_FLAGS_ENTRY_VALID) ==
		     LUT_FLAGS_ENTRY_VALID) {
			val = osi_readl(addr+RX_BYP_LUT_VALID);
			val |= (1 << index);
			osi_writel(val, addr+RX_BYP_LUT_VALID);
		} else {
			val = osi_readl(addr+RX_BYP_LUT_VALID);
			val &= ~(1 << index);
			osi_writel(val, addr+RX_BYP_LUT_VALID);
		}

		break;
	default:
		pr_err("Unknown controller select\n");
		return -1;
	}

	return 0;
}

static inline int lut_data_write(struct osi_core_priv_data *const osi_core,
			       struct osi_macsec_lut_config *const lut_config)
{
	switch (lut_config->lut_sel) {
	case LUT_SEL_BYPASS:
		if (byp_lut_config(osi_core, lut_config) != 0) {
			pr_err("BYP LUT config err\n");
			return -1;
		}
		break;
	case LUT_SEL_SCI:
		if (sci_lut_config(osi_core, lut_config) != 0) {
			pr_err("SCI LUT config err\n");
			return -1;
		}
		break;
	case LUT_SEL_SC_PARAM:
		if (sc_param_lut_config(osi_core, lut_config) != 0) {
			pr_err("SC param LUT config err\n");
			return -1;
		}
		break;
	case LUT_SEL_SC_STATE:
		if (sc_state_lut_config(osi_core, lut_config) != 0) {
			pr_err("SC state LUT config err\n");
			return -1;
		}
		break;
	case LUT_SEL_SA_STATE:
		if (sa_state_lut_config(osi_core, lut_config) != 0) {
			pr_err("SA state LUT config err\n");
			return -1;
		}
		break;
	default:
		//Unsupported LUT
		return -1;
	}

	return 0;
}

static int macsec_lut_config(struct osi_core_priv_data *const osi_core,
			     struct osi_macsec_lut_config *const lut_config)
{
	int ret = 0;
	unsigned int lut_config_reg;
	unsigned char *base = (unsigned char *)osi_core->macsec_base;

	/* Validate LUT config */
	if ((lut_config->table_config.ctlr_sel > CTLR_SEL_MAX) ||
	    (lut_config->table_config.rw > RW_MAX) ||
	    (lut_config->table_config.index > TABLE_INDEX_MAX) ||
	    (lut_config->lut_sel > LUT_SEL_MAX)) {
		pr_err("Validating LUT config failed. ctrl: %hu,"
			" rw: %hu, index: %hu, lut_sel: %hu",
			lut_config->table_config.ctlr_sel,
			lut_config->table_config.rw,
			lut_config->table_config.index, lut_config->lut_sel);
		/* TODO - validate using a local cache
		 * if index is already active */
		return -1;
	}

	/* Wait for previous LUT update to finish */
	ret = poll_for_lut_update(osi_core);
	if (ret < 0) {
		return ret;
	}

/*	pr_err("%s: LUT: %hu ctrl: %hu rw: %hu idx: %hu flags: %#x\n", __func__,
		lut_config->lut_sel, lut_config->table_config.ctlr_sel,
		lut_config->table_config.rw, lut_config->table_config.index,
		lut_config->flags); */

	lut_config_reg = osi_readl(base + MACSEC_LUT_CONFIG);
	if (lut_config->table_config.ctlr_sel) {
		lut_config_reg |= LUT_CONFIG_CTLR_SEL;
	} else {
		lut_config_reg &= ~LUT_CONFIG_CTLR_SEL;
	}

	if (lut_config->table_config.rw) {
		lut_config_reg |= LUT_CONFIG_RW;
		/* For write operation, load the lut_data registers */
		ret = lut_data_write(osi_core, lut_config);
		if (ret < 0) {
			return ret;
		}
	} else {
		lut_config_reg &= ~LUT_CONFIG_RW;
	}

	lut_config_reg &= ~LUT_CONFIG_LUT_SEL_MASK;
	lut_config_reg |= (lut_config->lut_sel << LUT_CONFIG_LUT_SEL_SHIFT);

	lut_config_reg &= ~LUT_CONFIG_INDEX_MASK;
	lut_config_reg |= (lut_config->table_config.index);

	lut_config_reg |= LUT_CONFIG_UPDATE;
	osi_writel(lut_config_reg, base + MACSEC_LUT_CONFIG);

	/* Wait for this LUT update to finish */
	ret = poll_for_lut_update(osi_core);
	if (ret < 0) {
		return ret;
	}

	if (!lut_config->table_config.rw) {
		ret = lut_data_read(osi_core, lut_config);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static inline void handle_rx_sc_invalid_key(
		struct osi_core_priv_data *const osi_core)
{
	unsigned char *addr = (unsigned char *)osi_core->macsec_base;
	unsigned int clear = 0;

	pr_err("%s()\n", __func__);

	/** check which SC/AN had triggered and clear */
	/* rx_sc0_7 */
	clear = osi_readl(addr + RX_SC_KEY_INVALID_STS0_0);
	osi_writel(clear, addr + RX_SC_KEY_INVALID_STS0_0);
	/* rx_sc8_15 */
	clear = osi_readl(addr + RX_SC_KEY_INVALID_STS1_0);
	osi_writel(clear, addr + RX_SC_KEY_INVALID_STS1_0);
}

static inline void handle_tx_sc_invalid_key(
			struct osi_core_priv_data *const osi_core)
{
	unsigned char *addr = (unsigned char *)osi_core->macsec_base;
	unsigned int clear = 0;

	pr_err("%s()\n", __func__);

	/** check which SC/AN had triggered and clear */
	/* tx_sc0_7 */
	clear = osi_readl(addr + TX_SC_KEY_INVALID_STS0_0);
	osi_writel(clear, addr + TX_SC_KEY_INVALID_STS0_0);
	/* tx_sc8_15 */
	clear = osi_readl(addr + TX_SC_KEY_INVALID_STS1_0);
	osi_writel(clear, addr + TX_SC_KEY_INVALID_STS1_0);
}

static inline void handle_safety_err_irq(
				struct osi_core_priv_data *const osi_core)
{
	pr_err("%s()\n", __func__);
}

static inline void handle_rx_sc_replay_err(
				struct osi_core_priv_data *const osi_core)
{
	unsigned char *addr = (unsigned char *)osi_core->macsec_base;
	unsigned int clear = 0;

	/* pr_err("%s()\n", __func__); */

	/* rx_sc0_7 */
	clear = osi_readl(addr + RX_SC_REPLAY_ERROR_STATUS0_0);
	osi_writel(clear, addr + RX_SC_REPLAY_ERROR_STATUS0_0);
	/* rx_sc8_15 */
	clear = osi_readl(addr + RX_SC_REPLAY_ERROR_STATUS1_0);
	osi_writel(clear, addr + RX_SC_REPLAY_ERROR_STATUS1_0);
}

static inline void handle_rx_pn_exhausted(
			struct osi_core_priv_data *const osi_core)
{
	unsigned char *addr = (unsigned char *)osi_core->macsec_base;
	unsigned int clear = 0;

	/* pr_err("%s()\n", __func__); */

	/* TODO: Do you need re-enable SC/AN? */

	/* check which SC/AN had triggered and clear
	 */
	/* rx_sc0_7 */
	clear = osi_readl(addr + RX_SC_PN_EXHAUSTED_STATUS0_0);
	osi_writel(clear, addr + RX_SC_PN_EXHAUSTED_STATUS0_0);
	/* rx_sc8_15 */
	clear = osi_readl(addr + RX_SC_PN_EXHAUSTED_STATUS1_0);
	osi_writel(clear, addr + RX_SC_PN_EXHAUSTED_STATUS1_0);
}

static inline void handle_tx_sc_err(struct osi_core_priv_data *const osi_core)
{
	unsigned char *addr = (unsigned char *)osi_core->macsec_base;
	unsigned int clear = 0;

	/* pr_err("%s()\n", __func__); */

	/* TODO: Do you need re-enable SC/AN? */

	clear = osi_readl(addr + TX_SC_ERROR_INTERRUPT_STATUS_0);

	osi_writel(clear, addr + TX_SC_ERROR_INTERRUPT_STATUS_0);

}

static inline void handle_tx_pn_threshold(
			struct osi_core_priv_data *const osi_core)
{
	unsigned char *addr = (unsigned char *)osi_core->macsec_base;
	unsigned int clear = 0;

	/* pr_err("%s()\n", __func__); */

	/* TODO: Do you need re-enable SC/AN? */

	/* check which SC/AN had triggered and clear
	 */
	/* tx_sc0_7 */
	clear = osi_readl(addr + TX_SC_PN_THRESHOLD_STATUS0_0);
	osi_writel(clear, addr + TX_SC_PN_THRESHOLD_STATUS0_0);
	/* tx_sc8_15 */
	clear = osi_readl(addr + TX_SC_PN_THRESHOLD_STATUS1_0);
	osi_writel(clear, addr + TX_SC_PN_THRESHOLD_STATUS1_0);
}

static inline void handle_tx_pn_exhausted(
			struct osi_core_priv_data *const osi_core)
{
	unsigned char *addr = (unsigned char *)osi_core->macsec_base;
	unsigned int clear = 0;

	/* pr_err("%s()\n", __func__); */

	/* TODO: Do you need re-enable SC/AN? */

	/* check which SC/AN had triggered and clear
	 */
	/* tx_sc0_7 */
	clear = osi_readl(addr + TX_SC_PN_EXHAUSTED_STATUS0_0);
	osi_writel(clear, addr + TX_SC_PN_EXHAUSTED_STATUS0_0);
	/* tx_sc8_15 */
	clear = osi_readl(addr + TX_SC_PN_EXHAUSTED_STATUS1_0);
	osi_writel(clear, addr + TX_SC_PN_EXHAUSTED_STATUS1_0);
}

static inline void handle_tx_irq(struct osi_core_priv_data *const osi_core)
{
	unsigned int tx_isr, clear = 0;
	unsigned char *addr = (unsigned char *)osi_core->macsec_base;

	tx_isr = osi_readl(addr + TX_ISR);
	pr_err("%s(): tx_isr 0x%x\n", __func__, tx_isr);
	if ((tx_isr & TX_DBG_BUF_CAPTURE_DONE) == TX_DBG_BUF_CAPTURE_DONE) {
		osi_core->macsec_irq_stats.tx_dbg_capture_done++;
		clear |= TX_DBG_BUF_CAPTURE_DONE;
	}

	if ((tx_isr & TX_MTU_CHECK_FAIL) == TX_MTU_CHECK_FAIL) {
		osi_core->macsec_irq_stats.tx_mtu_check_fail++;
		clear |= TX_MTU_CHECK_FAIL;
	}

	if ((tx_isr & TX_AES_GCM_BUF_OVF) == TX_AES_GCM_BUF_OVF) {
		osi_core->macsec_irq_stats.tx_aes_gcm_buf_ovf++;
		clear |= TX_AES_GCM_BUF_OVF;
	}

	if ((tx_isr & TX_SC_AN_NOT_VALID) == TX_SC_AN_NOT_VALID) {
		osi_core->macsec_irq_stats.tx_sc_an_not_valid++;
		handle_tx_sc_err(osi_core);
		clear |= TX_SC_AN_NOT_VALID;
	}

	if ((tx_isr & TX_MAC_CRC_ERROR) == TX_MAC_CRC_ERROR) {
		osi_core->macsec_irq_stats.tx_mac_crc_error++;
		clear |= TX_MAC_CRC_ERROR;
	}

	if ((tx_isr & TX_PN_THRSHLD_RCHD) == TX_PN_THRSHLD_RCHD) {
		osi_core->macsec_irq_stats.tx_pn_threshold++;
		/* TODO - need to check which SC/AN had triggered */
		handle_tx_pn_threshold(osi_core);
		clear |= TX_PN_THRSHLD_RCHD;
	}

	if ((tx_isr & TX_PN_EXHAUSTED) == TX_PN_EXHAUSTED) {
		osi_core->macsec_irq_stats.tx_pn_exhausted++;
		/* TODO - need to check which SC/AN had triggered */
		handle_tx_pn_exhausted(osi_core);
		clear |= TX_PN_EXHAUSTED;
	}
	if (clear) {
		pr_err("%s(): write tx_isr 0x%x\n", __func__, clear);
		osi_writel(clear, addr + TX_ISR);
	}
}

static inline void handle_rx_irq(struct osi_core_priv_data *const osi_core)
{
	unsigned int rx_isr, clear = 0;
	unsigned char *addr = (unsigned char *)osi_core->macsec_base;

	rx_isr = osi_readl(addr + RX_ISR);
	pr_err("%s(): rx_isr 0x%x\n", __func__, rx_isr);

	if ((rx_isr & RX_DBG_BUF_CAPTURE_DONE) == RX_DBG_BUF_CAPTURE_DONE) {
		osi_core->macsec_irq_stats.rx_dbg_capture_done++;
		clear |= RX_DBG_BUF_CAPTURE_DONE;
	}

	if ((rx_isr & RX_ICV_ERROR) == RX_ICV_ERROR) {
		osi_core->macsec_irq_stats.rx_icv_err_threshold++;
		clear |= RX_ICV_ERROR;
	}

	if ((rx_isr & RX_REPLAY_ERROR) == RX_REPLAY_ERROR) {
		osi_core->macsec_irq_stats.rx_replay_error++;
		handle_rx_sc_replay_err(osi_core);
		clear |= RX_REPLAY_ERROR;
	}

	if ((rx_isr & RX_MTU_CHECK_FAIL) == RX_MTU_CHECK_FAIL) {
		osi_core->macsec_irq_stats.rx_mtu_check_fail++;
		clear |= RX_MTU_CHECK_FAIL;
	}

	if ((rx_isr & RX_AES_GCM_BUF_OVF) == RX_AES_GCM_BUF_OVF) {
		osi_core->macsec_irq_stats.rx_aes_gcm_buf_ovf++;
		clear |= RX_AES_GCM_BUF_OVF;
	}

	if ((rx_isr & RX_MAC_CRC_ERROR) == RX_MAC_CRC_ERROR) {
		osi_core->macsec_irq_stats.rx_mac_crc_error++;
		clear |= RX_MAC_CRC_ERROR;
	}

	if ((rx_isr & RX_PN_EXHAUSTED) == RX_PN_EXHAUSTED) {
		osi_core->macsec_irq_stats.rx_pn_exhausted++;
		/* TODO - need to check which SC/AN had triggered */
		handle_rx_pn_exhausted(osi_core);
		clear |= RX_PN_EXHAUSTED;
	}
	if (clear) {
		pr_err("%s(): write rx_isr 0x%x\n", __func__, clear);
		osi_writel(clear, addr + RX_ISR);
	}
}

static inline void handle_common_irq(struct osi_core_priv_data *const osi_core)
{
	unsigned int common_isr, clear = 0;
	unsigned char *addr = (unsigned char *)osi_core->macsec_base;

	common_isr = osi_readl(addr + COMMON_ISR);
	pr_err("%s(): common_isr 0x%x\n", __func__, common_isr);

	if ((common_isr & SECURE_REG_VIOL) == SECURE_REG_VIOL) {
		osi_core->macsec_irq_stats.secure_reg_viol++;
		clear |= SECURE_REG_VIOL;
	}

	if ((common_isr & RX_UNINIT_KEY_SLOT) == RX_UNINIT_KEY_SLOT) {
		osi_core->macsec_irq_stats.rx_uninit_key_slot++;
		clear |= RX_UNINIT_KEY_SLOT;
		handle_rx_sc_invalid_key(osi_core);
	}

	if ((common_isr & RX_LKUP_MISS) == RX_LKUP_MISS) {
		osi_core->macsec_irq_stats.rx_lkup_miss++;
		clear |= RX_LKUP_MISS;
	}

	if ((common_isr & TX_UNINIT_KEY_SLOT) == TX_UNINIT_KEY_SLOT) {
		osi_core->macsec_irq_stats.tx_uninit_key_slot++;
		clear |= TX_UNINIT_KEY_SLOT;
		handle_tx_sc_invalid_key(osi_core);
	}

	if ((common_isr & TX_LKUP_MISS) == TX_LKUP_MISS) {
		osi_core->macsec_irq_stats.tx_lkup_miss++;
		clear |= TX_LKUP_MISS;
	}
	if (clear) {
		osi_writel(clear, addr + COMMON_ISR);
	}
}

static void macsec_handle_ns_irq(struct osi_core_priv_data *const osi_core)
{
	unsigned int irq_common_sr, common_isr;
	unsigned char *addr = (unsigned char *)osi_core->macsec_base;

	irq_common_sr = osi_readl(addr + INTERRUPT_COMMON_SR);
	pr_err("%s(): common_sr 0x%x\n", __func__, irq_common_sr);
	if ((irq_common_sr & COMMON_SR_TX) == COMMON_SR_TX) {
		handle_tx_irq(osi_core);
	}

	if ((irq_common_sr & COMMON_SR_RX) == COMMON_SR_RX) {
		handle_rx_irq(osi_core);
	}

	if ((irq_common_sr & COMMON_SR_SFTY_ERR) == COMMON_SR_SFTY_ERR) {
		handle_safety_err_irq(osi_core);
	}

	common_isr = osi_readl(addr + COMMON_ISR);
	if (common_isr != OSI_NONE) {
		handle_common_irq(osi_core);
	}
}

static void macsec_handle_s_irq(struct osi_core_priv_data *const osi_core)
{
	unsigned int common_isr;
	unsigned char *addr = (unsigned char *)osi_core->macsec_base;

	pr_err("%s()\n", __func__);

	common_isr = osi_readl(addr + COMMON_ISR);
	if (common_isr != OSI_NONE) {
		handle_common_irq(osi_core);
	}

	return;
}

static int macsec_loopback_config(struct osi_core_priv_data *const osi_core,
				  unsigned int enable)
{
	unsigned char *base = (unsigned char *)osi_core->macsec_base;
	unsigned int val;

	val = osi_readl(base + MACSEC_CONTROL1);
	pr_err("Read MACSEC_CONTROL1: 0x%x\n", val);

	if (enable == OSI_ENABLE) {
		val |= LOOPBACK_MODE_EN;
	} else if (enable == OSI_DISABLE) {
		val &= ~LOOPBACK_MODE_EN;
	} else {
		return -1;
	}

	pr_err("Write MACSEC_CONTROL1: 0x%x\n", val);
	osi_writel(val, base + MACSEC_CONTROL1);
	return 0;
}

static int clear_lut(struct osi_core_priv_data *const osi_core)
{
	struct osi_macsec_lut_config lut_config = {0};
	struct osi_macsec_kt_config kt_config = {0};
	struct macsec_table_config *table_config = &lut_config.table_config;
	int i, j;
	int ret = 0;

	table_config->rw = LUT_WRITE;
	/* Clear all the LUT's which have a dedicated LUT valid bit per entry */

	/* Tx/Rx BYP LUT */
	lut_config.lut_sel = LUT_SEL_BYPASS;
	for (i = 0; i <= CTLR_SEL_MAX; i++) {
		table_config->ctlr_sel = i;
		for (j = 0; j <= BYP_LUT_MAX_INDEX; j++) {
			table_config->index = j;
			ret = macsec_lut_config(osi_core, &lut_config);
			if (ret < 0) {
				pr_err("Error clearing CTLR:LUT:INDEX:  %d:%d:%d\n",
					i, lut_config.lut_sel, j);
				return ret;
			}
		}
	}

	/* Tx/Rx SCI LUT */
	lut_config.lut_sel = LUT_SEL_SCI;
	for (i = 0; i <= CTLR_SEL_MAX; i++) {
		table_config->ctlr_sel = i;
		for (j = 0; j <= SC_LUT_MAX_INDEX; j++) {
			table_config->index = j;
			ret = macsec_lut_config(osi_core, &lut_config);
			if (ret < 0) {
				pr_err("Error clearing CTLR:LUT:INDEX:  %d:%d:%d\n",
					i, lut_config.lut_sel, j);
				return ret;
			}
		}
	}

	/* Tx/Rx SC param LUT */
	lut_config.lut_sel = LUT_SEL_SC_PARAM;
	for (i = 0; i <= CTLR_SEL_MAX; i++) {
		table_config->ctlr_sel = i;
		for (j = 0; j <= SC_LUT_MAX_INDEX; j++) {
			table_config->index = j;
			ret = macsec_lut_config(osi_core, &lut_config);
			if (ret < 0) {
				pr_err("Error clearing CTLR:LUT:INDEX:  %d:%d:%d\n",
					i, lut_config.lut_sel, j);
				return ret;
			}
		}
	}

	/* Tx/Rx SC state */
	lut_config.lut_sel = LUT_SEL_SC_STATE;
	for (i = 0; i <= CTLR_SEL_MAX; i++) {
		table_config->ctlr_sel = i;
		for (j = 0; j <= SC_LUT_MAX_INDEX; j++) {
			table_config->index = j;
			ret = macsec_lut_config(osi_core, &lut_config);
			if (ret < 0) {
				pr_err("Error clearing CTLR:LUT:INDEX:  %d:%d:%d\n",
					i, lut_config.lut_sel, j);
				return ret;
			}
		}
	}

	/* Tx SA state LUT */
	lut_config.lut_sel = LUT_SEL_SA_STATE;
	table_config->ctlr_sel = CTLR_SEL_TX;
	for (j = 0; j <= SA_LUT_MAX_INDEX; j++) {
		table_config->index = j;
		ret = macsec_lut_config(osi_core, &lut_config);
		if (ret < 0) {
			pr_err("Error clearing Tx LUT:INDEX:  %d:%d\n",
				lut_config.lut_sel, j);
			return ret;
		}
	}

	/* Rx SA state LUT */
	lut_config.lut_sel = LUT_SEL_SA_STATE;
	table_config->ctlr_sel = CTLR_SEL_RX;
	for (j = 0; j <= SA_LUT_MAX_INDEX; j++) {
		table_config->index = j;
		ret = macsec_lut_config(osi_core, &lut_config);
		if (ret < 0) {
			pr_err("Error clearing Rx LUT:INDEX:  %d:%d\n",
				lut_config.lut_sel, j);
			return ret;
		}
	}

	/* Key table */
	table_config = &kt_config.table_config;
	table_config->rw = LUT_WRITE;
	for (i = 0; i <= CTLR_SEL_MAX; i++) {
		table_config->ctlr_sel = i;
		for (j = 0; j <= TABLE_INDEX_MAX; j++) {
			table_config->index = j;
			ret = macsec_kt_config(osi_core, &kt_config);
			if (ret < 0) {
				pr_err("Error clearing KT CTLR:INDEX: %d:%d\n",
					i, j);
				return ret;
			}
		}
	}

	return ret;
}

static int macsec_deinit(struct osi_core_priv_data *const osi_core)
{
	int i;

	for (i = CTLR_SEL_TX; i <= CTLR_SEL_RX; i++) {
		osi_memset(&osi_core->macsec_lut_status[i], OSI_NONE,
			   sizeof(struct osi_macsec_lut_status));
	}
	return 0;
}

static int macsec_init(struct osi_core_priv_data *const osi_core)
{
	unsigned int val = 0;
	struct osi_macsec_lut_config lut_config = {0};
	struct macsec_table_config *table_config = &lut_config.table_config;
	/* Store MAC address in reverse, per HW design */
	unsigned char mac_da_mkpdu[OSI_ETH_ALEN] = {0x3, 0x0, 0x0,
						    0xC2, 0x80, 0x01};
	unsigned char mac_da_bc[OSI_ETH_ALEN] = {0xFF, 0xFF, 0xFF,
						 0xFF, 0xFF, 0xFF};
	unsigned int mtu = osi_core->mtu + MACSEC_TAG_ICV_LEN;
	unsigned char *addr = (unsigned char *)osi_core->macsec_base;
	int ret = 0;
	int i, j;

	/* 1. Set MTU */
	val = osi_readl(addr + TX_MTU_LEN);
	pr_err("Read TX_MTU_LEN: 0x%x\n", val);
	val &= ~(MTU_LENGTH_MASK);
	val |= (mtu & MTU_LENGTH_MASK);
	pr_err("Write TX_MTU_LEN: 0x%x\n", val);
	osi_writel(val, addr + TX_MTU_LEN);

	val = osi_readl(addr + RX_MTU_LEN);
	pr_err("Read RX_MTU_LEN: 0x%x\n", val);
	val &= ~(MTU_LENGTH_MASK);
	val |= (mtu & MTU_LENGTH_MASK);
	pr_err("Write RX_MTU_LEN: 0x%x\n", val);
	osi_writel(val, addr + RX_MTU_LEN);

	/* 2. Set essential MACsec control configuration */
	val = osi_readl(addr + MACSEC_CONTROL0);
	pr_err("Read MACSEC_CONTROL0: 0x%x\n", val);
	val |= (TX_LKUP_MISS_NS_INTR | RX_LKUP_MISS_NS_INTR |
		TX_LKUP_MISS_BYPASS | RX_LKUP_MISS_BYPASS);
	val &= ~(VALIDATE_FRAMES_MASK);
	val |= VALIDATE_FRAMES_STRICT;
	val |= RX_REPLAY_PROT_EN;
	pr_err("Write MACSEC_CONTROL0: 0x%x\n", val);
	osi_writel(val, addr + MACSEC_CONTROL0);

	val = osi_readl(addr + MACSEC_CONTROL1);
	pr_err("Read MACSEC_CONTROL1: 0x%x\n", val);
	val |= (RX_MTU_CHECK_EN | TX_LUT_PRIO_BYP | TX_MTU_CHECK_EN);
	pr_err("Write MACSEC_CONTROL1: 0x%x\n", val);
	osi_writel(val, addr + MACSEC_CONTROL1);

	/* set DVLAN tag ethertype */

	/* val = DVLAN_TAG_ETHERTYPE;
	pr_err("Write MACSEC_TX_DVLAN_CONTROL_0: 0x%x\n", val);
	osi_writel(val, addr + MACSEC_TX_DVLAN_CONTROL_0);
	pr_err("Write MACSEC_RX_DVLAN_CONTROL_0: 0x%x\n", val);
	osi_writel(val, addr + MACSEC_RX_DVLAN_CONTROL_0); */

	val = osi_readl(addr + STATS_CONTROL_0);
	pr_err("Read STATS_CONTROL_0: 0x%x\n", val);
	/* set STATS rollover bit */
	val |= STATS_CONTROL0_CNT_RL_OVR_CPY;
	pr_err("Write STATS_CONTROL_0: 0x%x\n", val);
	osi_writel(val, addr + STATS_CONTROL_0);

	/* 3. Enable default interrupts needed */
	val = osi_readl(addr + TX_IMR);
	pr_err("Read TX_IMR: 0x%x\n", val);
	val |= (TX_DBG_BUF_CAPTURE_DONE_INT_EN |
		TX_MTU_CHECK_FAIL_INT_EN | TX_MAC_CRC_ERROR_INT_EN |
		TX_SC_AN_NOT_VALID_INT_EN | TX_AES_GCM_BUF_OVF_INT_EN |
		TX_PN_EXHAUSTED_INT_EN | TX_PN_THRSHLD_RCHD_INT_EN);
	pr_err("Write TX_IMR: 0x%x\n", val);
	osi_writel(val, addr + TX_IMR);

	val = osi_readl(addr + RX_IMR);
	pr_err("Read RX_IMR: 0x%x\n", val);

	val |= (RX_DBG_BUF_CAPTURE_DONE_INT_EN |
		RX_ICV_ERROR_INT_EN | RX_REPLAY_ERROR_INT_EN |
		RX_MTU_CHECK_FAIL_INT_EN | RX_MAC_CRC_ERROR_INT_EN |
		RX_AES_GCM_BUF_OVF_INT_EN |
		RX_PN_EXHAUSTED_INT_EN
		);
	pr_err("Write RX_IMR: 0x%x\n", val);
	osi_writel(val, addr + RX_IMR);

	val = osi_readl(addr + COMMON_IMR);
	pr_err("Read COMMON_IMR: 0x%x\n", val);

	val |= (SECURE_REG_VIOL_INT_EN | RX_UNINIT_KEY_SLOT_INT_EN |
		RX_LKUP_MISS_INT_EN | TX_UNINIT_KEY_SLOT_INT_EN |
		TX_LKUP_MISS_INT_EN);
	pr_err("Write COMMON_IMR: 0x%x\n", val);
	osi_writel(val, addr + COMMON_IMR);

	/* 4. TODO - Route safety intr to LIC */
	val = osi_readl(addr + INTERRUPT_MASK1_0);
	pr_err("Read INTERRUPT_MASK1_0: 0x%x\n", val);
	val |= SFTY_ERR_UNCORR_INT_EN;
	pr_err("Write INTERRUPT_MASK1_0: 0x%x\n", val);
	osi_writel(val, addr + INTERRUPT_MASK1_0);

	/* 5. Set AES mode
	 * Default power on reset is AES-GCM128, leave it.
	 */

	/* 6. Invalidate LUT entries */
	ret = clear_lut(osi_core);
	if (ret < 0) {
		pr_err("Invalidating all LUT's failed\n");
		return ret;
	}

	/* 7. Set default BYP for MKPDU/BC packets */
	table_config->rw = LUT_WRITE;
	lut_config.lut_sel = LUT_SEL_BYPASS;
	lut_config.flags |= (LUT_FLAGS_DA_VALID |
			     LUT_FLAGS_ENTRY_VALID);
	for (j = 0; j < OSI_ETH_ALEN; j++) {
		lut_config.lut_in.da[j] = mac_da_bc[j];
	}

	for (i = CTLR_SEL_TX; i <= CTLR_SEL_RX; i++) {
		table_config->ctlr_sel = (unsigned short) i;
		table_config->index =
				osi_core->macsec_lut_status[i].next_byp_idx;
		ret = macsec_lut_config(osi_core, &lut_config);
		if (ret < 0) {
			pr_err("Failed to set BYP for BC addr");
			goto exit;
		} else {
			osi_core->macsec_lut_status[i].next_byp_idx++;
		}
	}

	for (j = 0; j < OSI_ETH_ALEN; j++) {
		lut_config.lut_in.da[j] = mac_da_mkpdu[j];
	}

	for (i = CTLR_SEL_TX; i <= CTLR_SEL_RX; i++) {
		table_config->ctlr_sel = (unsigned short) i;
		table_config->index =
				osi_core->macsec_lut_status[i].next_byp_idx;
		ret = macsec_lut_config(osi_core, &lut_config);
		if (ret < 0) {
			pr_err("Failed to set BYP for MKPDU multicast DA");
			goto exit;
		} else {
			osi_core->macsec_lut_status[i].next_byp_idx++;
		}
	}

exit:
	return ret;
}

static struct osi_macsec_sc_info *find_existing_sc(
				struct osi_core_priv_data *const osi_core,
				struct osi_macsec_sc_info *const sc,
				unsigned short ctlr)
{
	struct osi_macsec_lut_status *lut_status =
					&osi_core->macsec_lut_status[ctlr];
	int i;

	for (i = 0; i < lut_status->next_sc_idx; i++) {
		if (osi_memcmp(lut_status->sc_info[i].sci, sc->sci, SCI_LEN) ==
				OSI_NONE) {
			return &lut_status->sc_info[i];
		}
	}

	return OSI_NULL;
}

static int del_upd_sc(struct osi_core_priv_data *const osi_core,
		      struct osi_macsec_sc_info *existing_sc,
		      struct osi_macsec_sc_info *const sc,
		      unsigned short ctlr)
{
	struct osi_macsec_lut_config lut_config = {0};
	struct osi_macsec_kt_config kt_config = {0};
	struct macsec_table_config *table_config;
	int ret;
	/* All input/output fields are already zero'd in declaration.
	 * Write all 0's to LUT index to clear everything
	 */

	table_config = &lut_config.table_config;
	table_config->ctlr_sel = ctlr;
	table_config->rw = LUT_WRITE;

	/* If curr_an of existing SC is same as AN being deleted, then remove
	 * SCI LUT entry as well. If not, it means some other AN is already
	 * enabled, so leave the SC configuration as is.
	 */
	if (existing_sc->curr_an == sc->curr_an) {
		/* 1. SCI LUT */
		lut_config.lut_sel = LUT_SEL_SCI;
		table_config->index = existing_sc->sc_idx_start;
		ret = macsec_lut_config(osi_core, &lut_config);
		if (ret < 0) {
			pr_err("%s: Failed to del SCI LUT", __func__);
			pr_err("%s: index = %hu", __func__, sc->sc_idx_start);
			goto err_sci;
		}

		/* 2. SC Param LUT */
		lut_config.lut_sel = LUT_SEL_SC_PARAM;
		ret = macsec_lut_config(osi_core, &lut_config);
		if (ret < 0) {
			pr_err("%s: Failed to del SC param", __func__);
			goto err_sc_param;
		}

		/* 3. SC state LUT */
		lut_config.lut_sel = LUT_SEL_SC_STATE;
		ret = macsec_lut_config(osi_core, &lut_config);
		if (ret < 0) {
			pr_err("%s: Failed to del SC state", __func__);
			goto err_sc_state;
		}
	}

	/* 4. SA State LUT */
	lut_config.lut_sel = LUT_SEL_SA_STATE;
	table_config->index = (existing_sc->sc_idx_start * MAX_NUM_SA) +
			       sc->curr_an;
	ret = macsec_lut_config(osi_core, &lut_config);
	if (ret < 0) {
		pr_err("%s: Failed to del SA state", __func__);
		goto err_sa_state;
	}

	/* 5. Key LUT */
	table_config = &kt_config.table_config;
	table_config->ctlr_sel = ctlr;
	table_config->rw = LUT_WRITE;
	/* Each SC has MAX_NUM_SA's supported in HW */
	table_config->index = (existing_sc->sc_idx_start * MAX_NUM_SA) +
			       sc->curr_an;
	ret = macsec_kt_config(osi_core, &kt_config);
	if (ret < 0) {
		pr_err("%s: Failed to del SAK", __func__);
		goto err_kt;
	}

	existing_sc->an_valid &= ~OSI_BIT(sc->curr_an);

	return 0;

err_kt:
	/* TODO cleanup SA state LUT */
err_sa_state:
	/* TODO Cleanup SC state LUT */
err_sc_state:
	/* TODO Cleanup SC param LUT */
err_sc_param:
	/* TODO cleanup SC LUT */
err_sci:
	return -1;
}

static int add_upd_sc(struct osi_core_priv_data *const osi_core,
		      struct osi_macsec_sc_info *const sc,
		      unsigned short ctlr)
{
	struct osi_macsec_lut_config lut_config = {0};
	struct osi_macsec_kt_config kt_config = {0};
	struct macsec_table_config *table_config;
	int ret, i;
#if 1 /* HKEY GENERATION */
	/* TODO - Move this to OSD */
	struct crypto_cipher *tfm;
	unsigned char hkey[KEY_LEN_128];
	unsigned char zeros[KEY_LEN_128] = {0};

	tfm = crypto_alloc_cipher("aes", 0, CRYPTO_ALG_ASYNC);
	if (crypto_cipher_setkey(tfm, sc->sak, KEY_LEN_128)) {
		pr_err("%s: Failed to set cipher key for H generation",
			__func__);
		return -1;
	}
	crypto_cipher_encrypt_one(tfm, hkey, zeros);
	pr_err("\n%s: Generated H key: ", __func__);
	for (i = 0; i < KEY_LEN_128; i++) {
		pr_cont(" %02x", hkey[i]);
	}
	pr_err("\n");
	crypto_free_cipher(tfm);
#endif /* HKEY GENERATION */

	/* 1. Key LUT */
	table_config = &kt_config.table_config;
	table_config->ctlr_sel = ctlr;
	table_config->rw = LUT_WRITE;
	/* Each SC has MAX_NUM_SA's supported in HW */
	table_config->index = (sc->sc_idx_start * MAX_NUM_SA) + sc->curr_an;
	kt_config.flags |= LUT_FLAGS_ENTRY_VALID;

	/* Program in reverse order as per HW design */
	for (i = 0; i < KEY_LEN_128; i++) {
		kt_config.entry.sak[i] = sc->sak[KEY_LEN_128 - 1 - i];
		kt_config.entry.h[i] = hkey[KEY_LEN_128 - 1 - i];
	}
	ret = macsec_kt_config(osi_core, &kt_config);
	if (ret < 0) {
		pr_err("%s: Failed to set SAK", __func__);
		return -1;
	}

	table_config = &lut_config.table_config;
	table_config->ctlr_sel = ctlr;
	table_config->rw = LUT_WRITE;

	/* 2. SA state LUT */
	lut_config.lut_sel = LUT_SEL_SA_STATE;
	table_config->index = (sc->sc_idx_start * MAX_NUM_SA) + sc->curr_an;
	lut_config.sa_state_out.next_pn = sc->next_pn;
	/* TODO - LLPN might have to be updated out of band for replay prot*/
	lut_config.sa_state_out.lowest_pn = sc->next_pn;
	lut_config.flags |= LUT_FLAGS_ENTRY_VALID;
	ret = macsec_lut_config(osi_core, &lut_config);
	if (ret < 0) {
		pr_err("%s: Failed to set SA state", __func__);
		goto err_sa_state;
	}

	/* 3. SC state LUT */
	lut_config.flags = OSI_NONE;
	lut_config.lut_sel = LUT_SEL_SC_STATE;
	table_config->index = sc->sc_idx_start;
	lut_config.sc_state_out.curr_an = sc->curr_an;
	ret = macsec_lut_config(osi_core, &lut_config);
	if (ret < 0) {
		pr_err("%s: Failed to set SC state", __func__);
		goto err_sc_state;
	}

	/* 4. SC param LUT */
	lut_config.flags = OSI_NONE;
	lut_config.lut_sel = LUT_SEL_SC_PARAM;
	table_config->index = sc->sc_idx_start;
	/* Program in reverse order as per HW design */
	for (i = 0; i < SCI_LEN; i++) {
		lut_config.sc_param_out.sci[i] = sc->sci[SCI_LEN - 1 - i];
	}
	lut_config.sc_param_out.key_index_start =
						(sc->sc_idx_start * MAX_NUM_SA);
	lut_config.sc_param_out.pn_max = PN_MAX_DEFAULT;
	lut_config.sc_param_out.pn_threshold = PN_THRESHOLD_DEFAULT;
	lut_config.sc_param_out.pn_window = PN_MAX_DEFAULT;
	lut_config.sc_param_out.tci = TCI_DEFAULT;
	lut_config.sc_param_out.vlan_in_clear = VLAN_IN_CLEAR_DEFAULT;
	ret = macsec_lut_config(osi_core, &lut_config);
	if (ret < 0) {
		pr_err("%s: Failed to set SC param", __func__);
		goto err_sc_param;
	}

	/* 5. SCI LUT */
	lut_config.flags = OSI_NONE;
	lut_config.lut_sel = LUT_SEL_SCI;
	table_config->index = sc->sc_idx_start;
	/* Program in reverse order as per HW design */
	for (i = 0; i < OSI_ETH_ALEN; i++) {
		/* Extract the mac sa from the SCI itself */
		lut_config.lut_in.sa[i] = sc->sci[OSI_ETH_ALEN - 1 - i];
	}
	lut_config.flags |= LUT_FLAGS_SA_VALID;
	lut_config.sci_lut_out.sc_index = sc->sc_idx_start;
	for (i = 0; i < SCI_LEN; i++) {
		lut_config.sci_lut_out.sci[i] = sc->sci[SCI_LEN - 1 - i];
	}
	lut_config.sci_lut_out.an_valid = sc->an_valid;

	lut_config.flags |= LUT_FLAGS_ENTRY_VALID;
	ret = macsec_lut_config(osi_core, &lut_config);
	if (ret < 0) {
		pr_err("%s: Failed to set SCI LUT", __func__);
		goto err_sci;
	}

	return 0;

err_sci:
	//TODO cleanup SC param
err_sc_param:
	//TODO cleanup SC state
err_sc_state:
	//TODO Cleanup SA state LUT
err_sa_state:
	//TODO Cleanup KT
	return -1;
}

static int macsec_config(struct osi_core_priv_data *const osi_core,
			 struct osi_macsec_sc_info *const sc,
			 unsigned int enable, unsigned short ctlr)
{
	struct osi_macsec_sc_info *existing_sc = OSI_NULL, *new_sc = OSI_NULL;
	struct osi_macsec_sc_info tmp_sc;
	struct osi_macsec_sc_info *tmp_sc_p = &tmp_sc;
	struct osi_macsec_lut_status *lut_status;
	int i;

	/* Validate inputs */
	if ((enable != OSI_ENABLE && enable != OSI_DISABLE) ||
	    (ctlr != CTLR_SEL_TX && ctlr != CTLR_SEL_RX)) {
		return -1;
	}

	/* TODO - lock needed. Multiple instances of supplicant can request
	 * to add a macsec config simultaneously.
	 */
	lut_status = &osi_core->macsec_lut_status[ctlr];

	/* 1. Find if SC is already existing in HW */
	existing_sc = find_existing_sc(osi_core, sc, ctlr);
	if (existing_sc == OSI_NULL) {
		if (enable == OSI_DISABLE) {
			pr_err("%s: trying to delete non-existing SC ?",
				__func__);
			return -1;
		} else {
			pr_err("%s: Adding new SC/SA: ctlr: %hu", __func__,
				ctlr);
			if (lut_status->next_sc_idx >= MAX_NUM_SC) {
				pr_err("%s: Err: Reached max SC LUT entries!",
				       __func__);
				return -1;
			}

			new_sc = &lut_status->sc_info[lut_status->next_sc_idx];
			osi_memcpy(new_sc->sci, sc->sci, SCI_LEN);
			osi_memcpy(new_sc->sak, sc->sak, KEY_LEN_128);
			new_sc->curr_an = sc->curr_an;
			new_sc->next_pn = sc->next_pn;

			new_sc->sc_idx_start = lut_status->next_sc_idx;
			new_sc->an_valid |= OSI_BIT(sc->curr_an);

			pr_err("%s: Adding new SC\n"
			       "\tsci: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n"
			       "\tan: %u\n"
			       "\tpn: %u"
			       "\tsc_idx_start: %u"
			       "\tan_valid: %#x", __func__,
				new_sc->sci[0], new_sc->sci[1], new_sc->sci[2],
				new_sc->sci[3], new_sc->sci[4], new_sc->sci[5],
				new_sc->sci[6], new_sc->sci[7],
				new_sc->curr_an, new_sc->next_pn,
				new_sc->sc_idx_start,
				new_sc->an_valid);
			pr_err("\tkey: ");
			for (i = 0; i < 16; i++) {
				pr_cont(" %02x", new_sc->sak[i]);
			}
			pr_err("");

			if (add_upd_sc(osi_core, new_sc, ctlr) != OSI_NONE) {
				pr_err("%s: failed to add new SC", __func__);
				/* TODO - remove new_sc from lut_status[] ?
				 * not needed for now, as next_sc_idx is not
				 * incremented.
				 */
				return -1;
			} else {
				/* Update lut status */
				lut_status->next_sc_idx++;
				pr_err("%s: Added new SC ctlr: %u "
				       "nxt_sc_idx: %u",
				       __func__, ctlr,
				       lut_status->next_sc_idx);
				return 0;
			}
		}
	} else {
		pr_err("%s: Updating existing SC", __func__);
		if (enable == OSI_DISABLE) {
			pr_err("%s: Deleting existing SA", __func__);
			if (del_upd_sc(osi_core, existing_sc, sc, ctlr) !=
			    OSI_NONE) {
				pr_err("%s: failed to del SA", __func__);
				return -1;
			} else {
				if (existing_sc->an_valid == OSI_NONE) {
					lut_status->next_sc_idx--;
					osi_memset(existing_sc, OSI_NONE,
						   sizeof(*existing_sc));
				}

				return 0;
			}
		} else {
			/* Take backup copy.
			 * Don't directly commit SC changes until LUT's are
			 * programmed successfully */
			*tmp_sc_p = *existing_sc;
			osi_memcpy(tmp_sc_p->sak, sc->sak, KEY_LEN_128);
			tmp_sc_p->curr_an = sc->curr_an;
			tmp_sc_p->next_pn = sc->next_pn;

			tmp_sc_p->an_valid |= OSI_BIT(sc->curr_an);

			pr_err("%s: Adding new SA to SC\n"
			      "\tsci: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n"
			      "\tan: %u\n"
			      "\tpn: %u"
			      "\tsc_idx_start: %u"
			      "\tan_valid: %#x", __func__,
				tmp_sc_p->sci[0], tmp_sc_p->sci[1],
				tmp_sc_p->sci[2], tmp_sc_p->sci[3],
				tmp_sc_p->sci[4], tmp_sc_p->sci[5],
				tmp_sc_p->sci[6], tmp_sc_p->sci[7],
				tmp_sc_p->curr_an, tmp_sc_p->next_pn,
				tmp_sc_p->sc_idx_start,
				tmp_sc_p->an_valid);
			pr_err("\tkey: ");
			for (i = 0; i < 16; i++) {
				pr_cont(" %02x", tmp_sc_p->sak[i]);
			}
			pr_err("");

			if (add_upd_sc(osi_core, tmp_sc_p, ctlr) != OSI_NONE) {
				pr_err("%s: failed to add new SA", __func__);
				/* TODO - remove new_sc from lut_status[] ?
				 * not needed for now, as next_sc_idx is not
				 * incremented.
				 */
				return -1;
			} else {
				/* Update lut status */
				lut_status->next_sc_idx++;
				pr_err("%s: Added new SA ctlr: %u",
				       __func__, ctlr);
				/* Now commit the changes */
				*existing_sc = *tmp_sc_p;
				return 0;
			}
		}
		return -1;
	}
}

static struct macsec_core_ops macsec_ops = {
	.init = macsec_init,
	.deinit = macsec_deinit,
	.handle_ns_irq = macsec_handle_ns_irq,
	.handle_s_irq = macsec_handle_s_irq,
	.lut_config = macsec_lut_config,
	.kt_config = macsec_kt_config,
	.loopback_config = macsec_loopback_config,
	.macsec_en = macsec_enable,
	.config = macsec_config,
	.read_mmc = macsec_read_mmc,
	.dbg_buf_config = macsec_dbg_buf_config,
	.dbg_events_config = macsec_dbg_events_config,
};

static struct osi_macsec_lut_status lut_status[NUM_CTLR];

int osi_init_macsec_ops(struct osi_core_priv_data *const osi_core)
{
	if (osi_core->macsec_base == OSI_NULL) {
		return -1;
	} else {
		osi_core->macsec_ops = &macsec_ops;
		osi_core->macsec_lut_status = lut_status;
		return 0;
	}
}

int osi_macsec_init(struct osi_core_priv_data *const osi_core)
{
	if (osi_core != OSI_NULL && osi_core->macsec_ops != OSI_NULL &&
	    osi_core->macsec_ops->init != OSI_NULL) {
		return osi_core->macsec_ops->init(osi_core);
	}

	return -1;
}

int osi_macsec_deinit(struct osi_core_priv_data *const osi_core)
{
	if (osi_core != OSI_NULL && osi_core->macsec_ops != OSI_NULL &&
	    osi_core->macsec_ops->deinit != OSI_NULL) {
		return osi_core->macsec_ops->deinit(osi_core);
	}
	return -1;
}

void osi_macsec_ns_isr(struct osi_core_priv_data *const osi_core)
{
	if (osi_core != OSI_NULL && osi_core->macsec_ops != OSI_NULL &&
	    osi_core->macsec_ops->handle_ns_irq != OSI_NULL) {
		osi_core->macsec_ops->handle_ns_irq(osi_core);
	}
}

void osi_macsec_s_isr(struct osi_core_priv_data *const osi_core)
{
	if (osi_core != OSI_NULL && osi_core->macsec_ops != OSI_NULL &&
	    osi_core->macsec_ops->handle_s_irq != OSI_NULL) {
		osi_core->macsec_ops->handle_s_irq(osi_core);
	}
}

int osi_macsec_lut_config(struct osi_core_priv_data *const osi_core,
			  struct osi_macsec_lut_config *const lut_config)
{
	if (osi_core != OSI_NULL && osi_core->macsec_ops != OSI_NULL &&
	    osi_core->macsec_ops->lut_config != OSI_NULL) {
		return osi_core->macsec_ops->lut_config(osi_core, lut_config);
	}

	return -1;
}

int osi_macsec_kt_config(struct osi_core_priv_data *const osi_core,
			 struct osi_macsec_kt_config *const kt_config)
{
	if (osi_core != OSI_NULL && osi_core->macsec_ops != OSI_NULL &&
	    osi_core->macsec_ops->kt_config != OSI_NULL &&
	    kt_config != OSI_NULL) {
		return osi_core->macsec_ops->kt_config(osi_core, kt_config);
	}

	return -1;
}

int osi_macsec_loopback(struct osi_core_priv_data *const osi_core,
			unsigned int enable)
{

	if (osi_core != OSI_NULL && osi_core->macsec_ops != OSI_NULL &&
	    osi_core->macsec_ops->loopback_config != OSI_NULL) {
		return osi_core->macsec_ops->loopback_config(osi_core, enable);
	}

	return -1;
}

int osi_macsec_en(struct osi_core_priv_data *const osi_core,
		  unsigned int enable)
{
	if (((enable & OSI_MACSEC_TX_EN) != OSI_MACSEC_TX_EN) &&
	    ((enable & OSI_MACSEC_RX_EN) != OSI_MACSEC_RX_EN) &&
	    (enable != OSI_DISABLE)) {
		return -1;
	}

	if (osi_core != OSI_NULL && osi_core->macsec_ops != OSI_NULL &&
	    osi_core->macsec_ops->macsec_en != OSI_NULL) {
		return osi_core->macsec_ops->macsec_en(osi_core, enable);
	}

	return -1;
}

int osi_macsec_config(struct osi_core_priv_data *const osi_core,
		      struct osi_macsec_sc_info *const sc,
		      unsigned int enable, unsigned short ctlr)
{
	if ((enable != OSI_ENABLE && enable != OSI_DISABLE) ||
	    (ctlr != CTLR_SEL_TX && ctlr != CTLR_SEL_RX)) {
		return -1;
	}

	if (osi_core != OSI_NULL && osi_core->macsec_ops != OSI_NULL &&
	    osi_core->macsec_ops->config != OSI_NULL && sc != OSI_NULL) {
		return osi_core->macsec_ops->config(osi_core, sc,
						    enable, ctlr);
	}

	return -1;
}

int osi_macsec_read_mmc(struct osi_core_priv_data *const osi_core)
{
	if (osi_core != OSI_NULL && osi_core->macsec_ops != OSI_NULL &&
	    osi_core->macsec_ops->read_mmc != OSI_NULL) {
		osi_core->macsec_ops->read_mmc(osi_core);
		return 0;
	}

	return -1;
}

int osi_macsec_dbg_buf_config(
		struct osi_core_priv_data *const osi_core,
		struct osi_macsec_dbg_buf_config *const dbg_buf_config)
{

	if (osi_core != OSI_NULL && osi_core->macsec_ops != OSI_NULL &&
		osi_core->macsec_ops->dbg_buf_config != OSI_NULL) {
		return osi_core->macsec_ops->dbg_buf_config(osi_core,
							dbg_buf_config);
	}

	return -1;
}

int osi_macsec_dbg_events_config(
		struct osi_core_priv_data *const osi_core,
		struct osi_macsec_dbg_buf_config *const dbg_buf_config)
{

	if (osi_core != OSI_NULL && osi_core->macsec_ops != OSI_NULL &&
		osi_core->macsec_ops->dbg_events_config != OSI_NULL) {
		return osi_core->macsec_ops->dbg_events_config(osi_core,
							dbg_buf_config);
	}

	return -1;
}
