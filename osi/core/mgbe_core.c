/*
 * Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
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
#include "../osi/common/type.h"
#include <osi_common.h>
#include <osi_core.h>
#include "mgbe_core.h"
#include "core_local.h"
#include "xpcs.h"
#include "mgbe_mmc.h"

/**
 * @brief mgbe_poll_for_swr - Poll for software reset (SWR bit in DMA Mode)
 *
 * Algorithm: CAR reset will be issued through MAC reset pin.
 *	  Waits for SWR reset to be cleared in DMA Mode register.
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @note MAC needs to be out of reset and proper clock configured.
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t mgbe_poll_for_swr(struct osi_core_priv_data *const osi_core)
{
	void *addr = osi_core->base;
	nveu32_t retry = 1000;
	nveu32_t count;
	nveu32_t dma_bmr = 0;
	nve32_t cond = 1;
	nveu32_t pre_si = osi_core->pre_si;

	/* Performing software reset */
	if (pre_si == 1U) {
		osi_writel(OSI_ENABLE, (nveu8_t *)addr + MGBE_DMA_MODE);
	}

	/* Poll Until Poll Condition */
	count = 0;
	while (cond == 1) {
		if (count > retry) {
			return -1;
		}

		count++;

		dma_bmr = osi_readl((nveu8_t *)addr + MGBE_DMA_MODE);
		if ((dma_bmr & MGBE_DMA_MODE_SWR) == OSI_NONE) {
			cond = 0;
		} else {
			/* sleep if SWR is set */
			osi_core->osd_ops.msleep(1U);
		}
	}

	return 0;
}

/**
 * @brief mgbe_calculate_per_queue_fifo - Calculate per queue FIFO size
 *
 * Algorithm: Total Tx/Rx FIFO size which is read from
 *	MAC HW is being shared equally among the queues that are
 *	configured.
 *
 * @param[in] fifo_size: Total Tx/RX HW FIFO size.
 * @param[in] queue_count: Total number of Queues configured.
 *
 * @note MAC has to be out of reset.
 *
 * @retval Queue size that need to be programmed.
 */
static nveu32_t mgbe_calculate_per_queue_fifo(nveu32_t fifo_size,
						  nveu32_t queue_count)
{
	nveu32_t q_fifo_size = 0;  /* calculated fifo size per queue */
	nveu32_t p_fifo = 0; /* per queue fifo size program value */

	if (queue_count == 0U) {
		return 0U;
	}

	/* calculate Tx/Rx fifo share per queue */
	switch (fifo_size) {
	case 0:
	case 1:
	case 2:
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
		q_fifo_size = FIFO_SIZE_KB(64U);
		break;
	case 10:
		q_fifo_size = FIFO_SIZE_KB(128U);
		break;
	case 11:
		q_fifo_size = FIFO_SIZE_KB(256U);
		break;
	default:
		q_fifo_size = FIFO_SIZE_KB(1U);
		break;
	}

	q_fifo_size = q_fifo_size / queue_count;

	if (q_fifo_size < UINT_MAX) {
		p_fifo = (q_fifo_size / 256U) - 1U;
	}

	return p_fifo;
}

/**
 * @brief mgbe_config_l2_da_perfect_inverse_match - configure register for
 *	inverse or perfect match.
 *
 * Algorithm: This sequence is used to select perfect/inverse matching
 *	for L2 DA
 *
 * @param[in] base: Base address  from OSI core private data structure.
 * @param[in] perfect_inverse_match: 1 - inverse mode 0- perfect mode
 *
 * @note MAC should be init and started. see osi_start_mac()
 */
static inline void mgbe_config_l2_da_perfect_inverse_match(
					void *base,
					unsigned int perfect_inverse_match)
{
	unsigned int value = 0U;

	value = osi_readl((unsigned char *)base + MGBE_MAC_PFR);
	value &= ~MGBE_MAC_PFR_DAIF;
	if (perfect_inverse_match == OSI_INV_MATCH) {
		/* Set DA Inverse Filtering */
		value |= MGBE_MAC_PFR_DAIF;
	}
	osi_writel(value, (unsigned char *)base + MGBE_MAC_PFR);
}

/**
 * @brief mgbe_config_mac_pkt_filter_reg - configure mac filter register.
 *
 * Algorithm: This sequence is used to configure MAC in differnet pkt
 *	processing modes like promiscuous, multicast, unicast,
 *	hash unicast/multicast.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] filter: OSI filter structure.
 *
 * @note 1) MAC should be initialized and started. see osi_start_mac()
 *
 * @retval 0 always
 */
static int mgbe_config_mac_pkt_filter_reg(struct osi_core_priv_data *osi_core,
					  const struct osi_filter *filter)
{
	unsigned int value = 0U;
	int ret = 0;

	value = osi_readl((unsigned char *)osi_core->base + MGBE_MAC_PFR);

	/* Retain all other values */
	value &= (MGBE_MAC_PFR_DAIF | MGBE_MAC_PFR_DBF | MGBE_MAC_PFR_SAIF |
		  MGBE_MAC_PFR_SAF | MGBE_MAC_PFR_PCF | MGBE_MAC_PFR_VTFE |
		  MGBE_MAC_PFR_IPFE | MGBE_MAC_PFR_DNTU | MGBE_MAC_PFR_RA);

	if ((filter->oper_mode & OSI_OPER_EN_PROMISC) != OSI_DISABLE) {
		/* Set Promiscuous Mode Bit */
		value |= MGBE_MAC_PFR_PR;
	}

	if ((filter->oper_mode & OSI_OPER_DIS_PROMISC) != OSI_DISABLE) {
		/* Reset Promiscuous Mode Bit */
		value &= ~MGBE_MAC_PFR_PR;
	}

	if ((filter->oper_mode & OSI_OPER_EN_ALLMULTI) != OSI_DISABLE) {
		/* Set Pass All Multicast Bit */
		value |= MGBE_MAC_PFR_PM;
	}

	if ((filter->oper_mode & OSI_OPER_DIS_ALLMULTI) != OSI_DISABLE) {
		/* Reset Pass All Multicast Bit */
		value &= ~MGBE_MAC_PFR_PM;
	}

	if ((filter->oper_mode & OSI_OPER_EN_PERFECT) != OSI_DISABLE) {
		/* Set Hash or Perfect Filter Bit */
		value |= MGBE_MAC_PFR_HPF;
	}

	if ((filter->oper_mode & OSI_OPER_DIS_PERFECT) != OSI_DISABLE) {
		/* Reset Hash or Perfect Filter Bit */
		value &= ~MGBE_MAC_PFR_HPF;
	}

	osi_writel(value, (unsigned char *)osi_core->base + MGBE_MAC_PFR);

	if ((filter->oper_mode & OSI_OPER_EN_L2_DA_INV) != OSI_DISABLE) {
		mgbe_config_l2_da_perfect_inverse_match(osi_core->base,
							OSI_INV_MATCH);
	}

	if ((filter->oper_mode & OSI_OPER_DIS_L2_DA_INV) != OSI_DISABLE) {
		mgbe_config_l2_da_perfect_inverse_match(osi_core->base,
							OSI_PFT_MATCH);
	}

	return ret;
}

/**
 * @brief mgbe_update_mac_addr_helper - Function to update DCS and MBC
 *
 * Algorithm: This helper routine is to update passed prameter value
 *	based on DCS and MBC parameter. Validation of dma_chan as well as
 *	dsc_en status performed before updating DCS bits.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[out] value: unsigned int pointer which has value read from register.
 * @param[in] idx: filter index
 * @param[in] dma_routing_enable: dma channel routing enable(1)
 * @param[in] dma_chan: dma channel number
 * @param[in] addr_mask: filter will not consider byte in comparison
 *	      Bit 5: MAC_Address${i}_High[15:8]
 *	      Bit 4: MAC_Address${i}_High[7:0]
 *	      Bit 3: MAC_Address${i}_Low[31:24]
 *	      ..
 *	      Bit 0: MAC_Address${i}_Low[7:0]
 *
 * @note 1) MAC should be initialized and stated. see osi_start_mac()
 *	 2) osi_core->osd should be populated.
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static inline int mgbe_update_mac_addr_helper(
				struct osi_core_priv_data *osi_core,
				unsigned int *value,
				unsigned int idx,
				unsigned int dma_routing_enable,
				unsigned int dma_chan, unsigned int addr_mask)
{
	int ret = 0;
	/* PDC bit of MAC_Ext_Configuration register is not set so binary
	 * value representation.
	 */
	if (dma_routing_enable == OSI_ENABLE) {
		if ((dma_chan < OSI_MGBE_MAX_NUM_CHANS) &&
		    (osi_core->dcs_en == OSI_ENABLE)) {
			*value = ((dma_chan << MGBE_MAC_ADDRH_DCS_SHIFT) &
				  MGBE_MAC_ADDRH_DCS);
		} else if (dma_chan > OSI_MGBE_MAX_NUM_CHANS - 0x1U) {
			OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_OUTOFBOUND,
				"invalid dma channel\n",
				(unsigned long long)dma_chan);
			ret = -1;
			goto err_dma_chan;
		} else {
			/* Do nothing */
		}
	}

	/* Address mask validation */
	if (addr_mask <= MGBE_MAB_ADDRH_MBC_MAX_MASK && addr_mask > OSI_NONE) {
		*value = (*value |
			  ((addr_mask << MGBE_MAC_ADDRH_MBC_SHIFT) &
			   MGBE_MAC_ADDRH_MBC));
	}

err_dma_chan:
	return ret;
}


/**
 * @brief mgbe_update_mac_addr_low_high_reg- Update L2 address in filter
 *	  register
 *
 * Algorithm: This routine update MAC address to register for filtering
 *	based on dma_routing_enable, addr_mask and src_dest. Validation of
 *	dma_chan as well as DCS bit enabled in RXQ to DMA mapping register
 *	performed before updating DCS bits.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] filter: OSI filter structure.
 *
 * @note 1) MAC should be initialized and stated. see osi_start_mac()
 *	 2) osi_core->osd should be populated.
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static int mgbe_update_mac_addr_low_high_reg(
				struct osi_core_priv_data *const osi_core,
				const struct osi_filter *filter)
{
	unsigned int idx = filter->index;
	unsigned int dma_routing_enable = filter->dma_routing;
	unsigned int dma_chan = filter->dma_chan;
	unsigned int addr_mask = filter->addr_mask;
	unsigned int src_dest = filter->src_dest;
	const unsigned char *addr = filter->mac_address;
	unsigned int value = 0x0U;
	int ret = 0;

	/* check for valid index (0 to 31) */
	if (idx >= OSI_MGBE_MAX_MAC_ADDRESS_FILTER) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			"invalid MAC filter index\n",
			idx);
		return -1;
	}

	/* High address clean should happen for filter index >= 0 */
	if (addr == OSI_NULL) {
		osi_writel(OSI_DISABLE, (unsigned char *)osi_core->base +
			   MGBE_MAC_ADDRH((idx)));
		return 0;
	}

	ret = mgbe_update_mac_addr_helper(osi_core, &value, idx,
					  dma_routing_enable, dma_chan,
					  addr_mask);
	if (ret == -1) {
		/* return on helper error */
		return ret;
	}

	/* Setting Source/Destination Address match valid for 1 to 31 index */
	if ((src_dest == OSI_SA_MATCH || src_dest == OSI_DA_MATCH)) {
		value = (value | ((src_dest << MGBE_MAC_ADDRH_SA_SHIFT) &
			 MGBE_MAC_ADDRH_SA));
	}

	osi_writel(((unsigned int)addr[4] |
		   ((unsigned int)addr[5] << 8) |
		   MGBE_MAC_ADDRH_AE |
		   value),
		   (unsigned char *)osi_core->base + MGBE_MAC_ADDRH((idx)));

	osi_writel(((unsigned int)addr[0] |
		   ((unsigned int)addr[1] << 8) |
		   ((unsigned int)addr[2] << 16) |
		   ((unsigned int)addr[3] << 24)),
		   (unsigned char *)osi_core->base +  MGBE_MAC_ADDRL((idx)));

	return ret;
}

/**
 * @brief mgbe_poll_for_l3l4crtl - Poll for L3_L4 filter register operations.
 *
 * Algorithm: Waits for waits for transfer busy bit to be cleared in
 * L3_L4 address control register to complete filter register operations.
 *
 * @param[in] addr: MGBE virtual base address.
 *
 * @note MAC needs to be out of reset and proper clock configured.
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static int mgbe_poll_for_l3l4crtl(struct osi_core_priv_data *osi_core)
{
	unsigned int retry = 10;
	unsigned int count;
	unsigned int l3l4_addr_ctrl = 0;
	int cond = 1;

	/* Poll Until L3_L4_Address_Control XB is clear */
	count = 0;
	while (cond == 1) {
		if (count > retry) {
			/* Return error after max retries */
			return -1;
		}

		count++;

		l3l4_addr_ctrl = osi_readl((unsigned char *)osi_core->base +
					   MGBE_MAC_L3L4_ADDR_CTR);
		if ((l3l4_addr_ctrl & MGBE_MAC_L3L4_ADDR_CTR_XB) == OSI_NONE) {
			/* Set cond to 0 to exit loop */
			cond = 0;
		} else {
			/* wait for 10 usec for XB clear */
			osi_core->osd_ops.udelay(MGBE_MAC_XB_WAIT);
		}
	}

	return 0;
}

/**
 * @brief mgbe_l3l4_filter_write - L3_L4 filter register write.
 *
 * Algorithm: writes L3_L4 filter register
 *
 * @param[in] base: MGBE virtual base address.
 * @param[in] filter_no: MGBE  L3_L4 filter number
 * @param[in] filter_type: MGBE L3_L4 filter register type.
 * @param[in] value: MGBE  L3_L4 filter register value
 *
 * @note MAC needs to be out of reset and proper clock configured.
 */
static void mgbe_l3l4_filter_write(struct osi_core_priv_data *osi_core,
				   unsigned int filter_no,
				   unsigned int filter_type,
				   unsigned int value)
{
	void *base = osi_core->base;
	unsigned int addr = 0;

	/* Write MAC_L3_L4_Data register value */
	osi_writel(value, (unsigned char *)base + MGBE_MAC_L3L4_DATA);

	/* Program MAC_L3_L4_Address_Control */
	addr = osi_readl((unsigned char *)base + MGBE_MAC_L3L4_ADDR_CTR);

	/* update filter number */
	addr &= ~(MGBE_MAC_L3L4_ADDR_CTR_IDDR_FNUM);
	addr |= ((filter_no << MGBE_MAC_L3L4_ADDR_CTR_IDDR_FNUM_SHIFT) &
		  MGBE_MAC_L3L4_ADDR_CTR_IDDR_FNUM);

	/* update filter type */
	addr &= ~(MGBE_MAC_L3L4_ADDR_CTR_IDDR_FTYPE);
	addr |= ((filter_type << MGBE_MAC_L3L4_ADDR_CTR_IDDR_FTYPE_SHIFT) &
		  MGBE_MAC_L3L4_ADDR_CTR_IDDR_FTYPE);

	/* Set TT filed 0 for write */
	addr &= ~(MGBE_MAC_L3L4_ADDR_CTR_TT);

	/* Set XB bit to initiate write */
	addr |= MGBE_MAC_L3L4_ADDR_CTR_XB;

	/* Write MGBE_MAC_L3L4_ADDR_CTR */
	osi_writel(addr, (unsigned char *)base + MGBE_MAC_L3L4_ADDR_CTR);

	/* Wait untile XB bit reset */
	if (mgbe_poll_for_l3l4crtl(osi_core) < 0) {
		OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_HW_FAIL,
			     "Fail to write L3_L4_Address_Control\n",
			     filter_type);
	}
}

/**
 * @brief mgbe_l3l4_filter_read - L3_L4 filter register read.
 *
 * Algorithm: writes L3_L4 filter register
 *
 * @param[in] base: MGBE virtual base address.
 * @param[in] filter_no: MGBE  L3_L4 filter number
 * @param[in] filter_type: MGBE L3_L4 filter register type.
 * @param[in] *value: Pointer MGBE L3_L4 filter register value
 *
 * @note MAC needs to be out of reset and proper clock configured.
 */
static void mgbe_l3l4_filter_read(struct osi_core_priv_data *osi_core,
				  unsigned int filter_no,
				  unsigned int filter_type,
				  unsigned int *value)
{
	void *base = osi_core->base;
	unsigned int addr = 0;

	/* Program MAC_L3_L4_Address_Control */
	addr = osi_readl((unsigned char *)base + MGBE_MAC_L3L4_ADDR_CTR);

	/* update filter number */
	addr &= ~(MGBE_MAC_L3L4_ADDR_CTR_IDDR_FNUM);
	addr |= ((filter_no << MGBE_MAC_L3L4_ADDR_CTR_IDDR_FNUM_SHIFT) &
		  MGBE_MAC_L3L4_ADDR_CTR_IDDR_FNUM);

	/* update filter type */
	addr &= ~(MGBE_MAC_L3L4_ADDR_CTR_IDDR_FTYPE);
	addr |= ((filter_type << MGBE_MAC_L3L4_ADDR_CTR_IDDR_FTYPE_SHIFT) &
		  MGBE_MAC_L3L4_ADDR_CTR_IDDR_FTYPE);

	/* Set TT field 1 for read */
	addr |= MGBE_MAC_L3L4_ADDR_CTR_TT;

	/* Set XB bit to initiate write */
	addr |= MGBE_MAC_L3L4_ADDR_CTR_XB;

	/* Write MGBE_MAC_L3L4_ADDR_CTR */
	osi_writel(addr, (unsigned char *)base + MGBE_MAC_L3L4_ADDR_CTR);

	/* Wait untile XB bit reset */
	if (mgbe_poll_for_l3l4crtl(osi_core) < 0) {
		OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_HW_FAIL,
			    "Fail to read L3L4 Address\n",
			    filter_type);
		return;
	}

	/* Read the MGBE_MAC_L3L4_DATA for filter register data */
	*value = osi_readl((unsigned char *)base + MGBE_MAC_L3L4_DATA);
}

/**
 * @brief mgbe_update_ip4_addr - configure register for IPV4 address filtering
 *
 * Algorithm:  This sequence is used to update IPv4 source/destination
 *	Address for L3 layer filtering
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] filter_no: filter index
 * @param[in] addr: ipv4 address
 * @param[in] src_dst_addr_match: 0 - source addr otherwise - dest addr
 *
 * @note 1) MAC should be init and started. see osi_start_mac()
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static int mgbe_update_ip4_addr(struct osi_core_priv_data *osi_core,
				const unsigned int filter_no,
				const unsigned char addr[],
				const unsigned int src_dst_addr_match)
{
	unsigned int value = 0U;
	unsigned int temp = 0U;

	if (addr == OSI_NULL) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			"invalid address\n",
			0ULL);
		return -1;
	}

	if (filter_no >= OSI_MGBE_MAX_L3_L4_FILTER) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_OUTOFBOUND,
			"invalid filter index for L3/L4 filter\n",
			(unsigned long long)filter_no);
		return -1;
	}

	/* validate src_dst_addr_match argument */
	if (src_dst_addr_match != OSI_SOURCE_MATCH &&
	    src_dst_addr_match != OSI_INV_MATCH) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			"Invalid src_dst_addr_match value\n",
			src_dst_addr_match);
		return -1;
	}

	value = addr[3];
	temp = (unsigned int)addr[2] << 8;
	value |= temp;
	temp = (unsigned int)addr[1] << 16;
	value |= temp;
	temp = (unsigned int)addr[0] << 24;
	value |= temp;
	if (src_dst_addr_match == OSI_SOURCE_MATCH) {
		mgbe_l3l4_filter_write(osi_core,
				       filter_no,
				       MGBE_MAC_L3_AD0R,
				       value);
	} else {
		mgbe_l3l4_filter_write(osi_core,
				       filter_no,
				       MGBE_MAC_L3_AD1R,
				       value);
	}

	return 0;
}

/**
 * @brief mgbe_update_ip6_addr - add ipv6 address in register
 *
 * Algorithm: This sequence is used to update IPv6 source/destination
 *	      Address for L3 layer filtering
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] filter_no: filter index
 * @param[in] addr: ipv6 adderss
 *
 * @note 1) MAC should be init and started. see osi_start_mac()
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static int mgbe_update_ip6_addr(struct osi_core_priv_data *osi_core,
				const unsigned int filter_no,
				const unsigned short addr[])
{
	unsigned int value = 0U;
	unsigned int temp = 0U;

	if (addr == OSI_NULL) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			"invalid address\n",
			0ULL);
		return -1;
	}

	if (filter_no >= OSI_MGBE_MAX_L3_L4_FILTER) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			"invalid filter index for L3/L4 filter\n",
			(unsigned long long)filter_no);
		return -1;
	}

	/* update Bits[31:0] of 128-bit IP addr */
	value = addr[7];
	temp = (unsigned int)addr[6] << 16;
	value |= temp;
	mgbe_l3l4_filter_write(osi_core, filter_no, MGBE_MAC_L3_AD0R, value);
	/* update Bits[63:32] of 128-bit IP addr */
	value = addr[5];
	temp = (unsigned int)addr[4] << 16;
	value |= temp;
	mgbe_l3l4_filter_write(osi_core, filter_no, MGBE_MAC_L3_AD1R, value);
	/* update Bits[95:64] of 128-bit IP addr */
	value = addr[3];
	temp = (unsigned int)addr[2] << 16;
	value |= temp;
	mgbe_l3l4_filter_write(osi_core, filter_no, MGBE_MAC_L3_AD2R, value);
	/* update Bits[127:96] of 128-bit IP addr */
	value = addr[1];
	temp = (unsigned int)addr[0] << 16;
	value |= temp;
	mgbe_l3l4_filter_write(osi_core, filter_no, MGBE_MAC_L3_AD3R, value);

	return 0;
}

/**
 * @brief mgbe_config_l3_l4_filter_enable - register write to enable L3/L4
 *	filters.
 *
 * Algorithm: This routine to enable/disable L3/l4 filter
 *
 * @param[in] osi_core: OSI core private data structure.
 * @note MAC should be init and started. see osi_start_mac()
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static int mgbe_config_l3_l4_filter_enable(
					struct osi_core_priv_data *const osi_core,
					unsigned int filter_enb_dis)
{
	unsigned int value = 0U;
	void *base = osi_core->base;

	/* validate filter_enb_dis argument */
	if (filter_enb_dis != OSI_ENABLE && filter_enb_dis != OSI_DISABLE) {
		OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			"Invalid filter_enb_dis value\n",
			filter_enb_dis);
		return -1;
	}

	value = osi_readl((unsigned char *)base + MGBE_MAC_PFR);
	value &= ~(MGBE_MAC_PFR_IPFE);
	value |= ((filter_enb_dis << MGBE_MAC_PFR_IPFE_SHIFT) &
		  MGBE_MAC_PFR_IPFE);
	osi_writel(value, (unsigned char *)base + MGBE_MAC_PFR);

	return 0;
}

/**
 * @brief mgbe_update_l4_port_no -program source  port no
 *
 * Algorithm: sequence is used to update Source Port Number for
 *	L4(TCP/UDP) layer filtering.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] filter_no: filter index
 * @param[in] port_no: port number
 * @param[in] src_dst_port_match: 0 - source port, otherwise - dest port
 *
 * @note 1) MAC should be init and started. see osi_start_mac()
 *	 2) osi_core->osd should be populated
 *	 3) DCS bits should be enabled in RXQ to DMA mapping register
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static int mgbe_update_l4_port_no(struct osi_core_priv_data *osi_core,
				  unsigned int filter_no,
				  unsigned short port_no,
				  unsigned int src_dst_port_match)
{
	unsigned int value = 0U;
	unsigned int temp = 0U;

	if (filter_no >= OSI_MGBE_MAX_L3_L4_FILTER) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_OUTOFBOUND,
			"invalid filter index for L3/L4 filter\n",
			(unsigned long long)filter_no);
		return -1;
	}

	mgbe_l3l4_filter_read(osi_core, filter_no, MGBE_MAC_L4_ADDR, &value);
	if (src_dst_port_match == OSI_SOURCE_MATCH) {
		value &= ~MGBE_MAC_L4_ADDR_SP_MASK;
		value |= ((unsigned int)port_no  & MGBE_MAC_L4_ADDR_SP_MASK);
	} else {
		value &= ~MGBE_MAC_L4_ADDR_DP_MASK;
		temp = port_no;
		value |= ((temp << MGBE_MAC_L4_ADDR_DP_SHIFT) &
			  MGBE_MAC_L4_ADDR_DP_MASK);
	}
	mgbe_l3l4_filter_write(osi_core, filter_no, MGBE_MAC_L4_ADDR, value);

	return 0;
}

/**
 * @brief mgbe_set_dcs - check and update dma routing register
 *
 * Algorithm: Check for request for DCS_enable as well as validate chan
 *	number and dcs_enable is set. After validation, this sequence is used
 *	to configure L3((IPv4/IPv6) filters for address matching.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] value: unsigned int value for caller
 * @param[in] dma_routing_enable: filter based dma routing enable(1)
 * @param[in] dma_chan: dma channel for routing based on filter
 *
 * @note 1) MAC IP should be out of reset and need to be initialized
 *	 as the requirements.
 *	 2) DCS bit of RxQ should be enabled for dynamic channel selection
 *	    in filter support
 *
 * @retval updated unsigned int value param
 */
static inline unsigned int mgbe_set_dcs(struct osi_core_priv_data *osi_core,
					unsigned int value,
					unsigned int dma_routing_enable,
					unsigned int dma_chan)
{
	if ((dma_routing_enable == OSI_ENABLE) && (dma_chan <
	    OSI_MGBE_MAX_NUM_CHANS) && (osi_core->dcs_en ==
	    OSI_ENABLE)) {
		value |= ((dma_routing_enable <<
			  MGBE_MAC_L3L4_CTR_DMCHEN0_SHIFT) &
			  MGBE_MAC_L3L4_CTR_DMCHEN0);
		value |= ((dma_chan <<
			  MGBE_MAC_L3L4_CTR_DMCHN0_SHIFT) &
			  MGBE_MAC_L3L4_CTR_DMCHN0);
	}

	return value;
}

/**
 * @brief mgbe_helper_l3l4_bitmask - helper function to set L3L4
 * bitmask.
 *
 * Algorithm: set bit corresponding to L3l4 filter index
 *
 * @param[in] bitmask: bit mask OSI core private data structure.
 * @param[in] filter_no: filter index
 * @param[in] value:  0 - disable  otherwise - l3/l4 filter enabled
 *
 * @note 1) MAC should be init and started. see osi_start_mac()
 */
static inline void mgbe_helper_l3l4_bitmask(unsigned int *bitmask,
					    unsigned int filter_no,
					    unsigned int value)
{
	unsigned int temp;

	temp = OSI_ENABLE;
	temp = temp << filter_no;

	/* check against all bit fields for L3L4 filter enable */
	if ((value & MGBE_MAC_L3L4_CTRL_ALL) != OSI_DISABLE) {
		/* Set bit mask for index */
		*bitmask |= temp;
	} else {
		/* Reset bit mask for index */
		*bitmask &= ~temp;
	}
}

/**
 * @brief mgbe_config_l3_filters - config L3 filters.
 *
 * Algorithm: Check for DCS_enable as well as validate channel
 *	number and if dcs_enable is set. After validation, code flow
 *	is used to configure L3((IPv4/IPv6) filters resister
 *	for address matching.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] filter_no: filter index
 * @param[in] enb_dis:  1 - enable otherwise - disable L3 filter
 * @param[in] ipv4_ipv6_match: 1 - IPv6, otherwise - IPv4
 * @param[in] src_dst_addr_match: 0 - source, otherwise - destination
 * @param[in] perfect_inverse_match: normal match(0) or inverse map(1)
 * @param[in] dma_routing_enable: filter based dma routing enable(1)
 * @param[in] dma_chan: dma channel for routing based on filter
 *
 * @note 1) MAC should be init and started. see osi_start_mac()
 *	 2) osi_core->osd should be populated
 *	 3) DCS bit of RxQ should be enabled for dynamic channel selection
 *	    in filter support
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static int mgbe_config_l3_filters(struct osi_core_priv_data *osi_core,
				  unsigned int filter_no,
				  unsigned int enb_dis,
				  unsigned int ipv4_ipv6_match,
				  unsigned int src_dst_addr_match,
				  unsigned int perfect_inverse_match,
				  unsigned int dma_routing_enable,
				  unsigned int dma_chan)
{
	unsigned int value = 0U;

	if (filter_no >= OSI_MGBE_MAX_L3_L4_FILTER) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_OUTOFBOUND,
			"invalid filter index for L3/L4 filter\n",
			(unsigned long long)filter_no);
		return -1;
	}
	/* validate enb_dis argument */
	if (enb_dis != OSI_ENABLE && enb_dis != OSI_DISABLE) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			"Invalid filter_enb_dis value\n",
			enb_dis);
		return -1;
	}
	/* validate ipv4_ipv6_match argument */
	if (ipv4_ipv6_match != OSI_IPV6_MATCH &&
	    ipv4_ipv6_match != OSI_IPV4_MATCH) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			"Invalid ipv4_ipv6_match value\n",
			ipv4_ipv6_match);
		return -1;
	}
	/* validate src_dst_addr_match argument */
	if (src_dst_addr_match != OSI_SOURCE_MATCH &&
	    src_dst_addr_match != OSI_INV_MATCH) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			"Invalid src_dst_addr_match value\n",
			src_dst_addr_match);
		return -1;
	}
	/* validate perfect_inverse_match argument */
	if (perfect_inverse_match != OSI_ENABLE &&
	    perfect_inverse_match != OSI_DISABLE) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			"Invalid perfect_inverse_match value\n",
			perfect_inverse_match);
		return -1;
	}
	if ((dma_routing_enable == OSI_ENABLE) &&
	    (dma_chan > OSI_MGBE_MAX_NUM_CHANS - 1U)) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_OUTOFBOUND,
			"Wrong DMA channel\n",
			(unsigned long long)dma_chan);
		return -1;
	}

	mgbe_l3l4_filter_read(osi_core, filter_no, MGBE_MAC_L3L4_CTR, &value);
	value &= ~MGBE_MAC_L3L4_CTR_L3PEN0;
	value |= (ipv4_ipv6_match  & MGBE_MAC_L3L4_CTR_L3PEN0);

	/* For IPv6 either SA/DA can be checked not both */
	if (ipv4_ipv6_match == OSI_IPV6_MATCH) {
		if (enb_dis == OSI_ENABLE) {
			if (src_dst_addr_match == OSI_SOURCE_MATCH) {
				/* Enable L3 filters for IPv6 SOURCE addr
				 *  matching
				 */
				value &= ~MGBE_MAC_L3_IP6_CTRL_CLEAR;
				value |= ((MGBE_MAC_L3L4_CTR_L3SAM0 |
					  perfect_inverse_match <<
					  MGBE_MAC_L3L4_CTR_L3SAIM0_SHIFT) &
					  ((MGBE_MAC_L3L4_CTR_L3SAM0 |
					  MGBE_MAC_L3L4_CTR_L3SAIM0)));
				value |= mgbe_set_dcs(osi_core, value,
						      dma_routing_enable,
						      dma_chan);

			} else {
				/* Enable L3 filters for IPv6 DESTINATION addr
				 * matching
				 */
				value &= ~MGBE_MAC_L3_IP6_CTRL_CLEAR;
				value |= ((MGBE_MAC_L3L4_CTR_L3DAM0 |
					  perfect_inverse_match <<
					  MGBE_MAC_L3L4_CTR_L3DAIM0_SHIFT) &
					  ((MGBE_MAC_L3L4_CTR_L3DAM0 |
					  MGBE_MAC_L3L4_CTR_L3DAIM0)));
				value |= mgbe_set_dcs(osi_core, value,
						      dma_routing_enable,
						      dma_chan);
			}
		} else {
			/* Disable L3 filters for IPv6 SOURCE/DESTINATION addr
			 * matching
			 */
			value &= ~(MGBE_MAC_L3_IP6_CTRL_CLEAR |
				   MGBE_MAC_L3L4_CTR_L3PEN0);
		}
	} else {
		if (src_dst_addr_match == OSI_SOURCE_MATCH) {
			if (enb_dis == OSI_ENABLE) {
				/* Enable L3 filters for IPv4 SOURCE addr
				 * matching
				 */
				value &= ~MGBE_MAC_L3_IP4_SA_CTRL_CLEAR;
				value |= ((MGBE_MAC_L3L4_CTR_L3SAM0 |
					  perfect_inverse_match <<
					  MGBE_MAC_L3L4_CTR_L3SAIM0_SHIFT) &
					  ((MGBE_MAC_L3L4_CTR_L3SAM0 |
					  MGBE_MAC_L3L4_CTR_L3SAIM0)));
				value |= mgbe_set_dcs(osi_core, value,
						      dma_routing_enable,
						      dma_chan);
			} else {
				/* Disable L3 filters for IPv4 SOURCE addr
				 * matching
				 */
				value &= ~MGBE_MAC_L3_IP4_SA_CTRL_CLEAR;
			}
		} else {
			if (enb_dis == OSI_ENABLE) {
				/* Enable L3 filters for IPv4 DESTINATION addr
				 * matching
				 */
				value &= ~MGBE_MAC_L3_IP4_DA_CTRL_CLEAR;
				value |= ((MGBE_MAC_L3L4_CTR_L3DAM0 |
					  perfect_inverse_match <<
					  MGBE_MAC_L3L4_CTR_L3DAIM0_SHIFT) &
					  ((MGBE_MAC_L3L4_CTR_L3DAM0 |
					  MGBE_MAC_L3L4_CTR_L3DAIM0)));
				value |= mgbe_set_dcs(osi_core, value,
						      dma_routing_enable,
						      dma_chan);
			} else {
				/* Disable L3 filters for IPv4 DESTINATION addr
				 * matching
				 */
				value &= ~MGBE_MAC_L3_IP4_DA_CTRL_CLEAR;
			}
		}
	}
	mgbe_l3l4_filter_write(osi_core, filter_no, MGBE_MAC_L3L4_CTR, value);

	/* Set bit corresponding to filter index if value is non-zero */
	mgbe_helper_l3l4_bitmask(&osi_core->l3l4_filter_bitmask,
				 filter_no, value);

	return 0;
}

/**
 * @brief mgbe_config_l4_filters - Config L4 filters.
 *
 * Algorithm: This sequence is used to configure L4(TCP/UDP) filters for
 *	SA and DA Port Number matching
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] filter_no: filter index
 * @param[in] enb_dis: 1 - enable, otherwise - disable L4 filter
 * @param[in] tcp_udp_match: 1 - udp, 0 - tcp
 * @param[in] src_dst_port_match: 0 - source port, otherwise - dest port
 * @param[in] perfect_inverse_match: normal match(0) or inverse map(1)
 * @param[in] dma_routing_enable: filter based dma routing enable(1)
 * @param[in] dma_chan: dma channel for routing based on filter
 *
 * @note 1) MAC should be init and started. see osi_start_mac()
 *	 2) osi_core->osd should be populated
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static int mgbe_config_l4_filters(struct osi_core_priv_data *osi_core,
				  unsigned int filter_no,
				  unsigned int enb_dis,
				  unsigned int tcp_udp_match,
				  unsigned int src_dst_port_match,
				  unsigned int perfect_inverse_match,
				  unsigned int dma_routing_enable,
				  unsigned int dma_chan)
{
	unsigned int value = 0U;

	if (filter_no >= OSI_MGBE_MAX_L3_L4_FILTER) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_OUTOFBOUND,
			"invalid filter index for L3/L4 filter\n",
			(unsigned long long)filter_no);
		return -1;
	}
	/* validate enb_dis argument */
	if (enb_dis != OSI_ENABLE && enb_dis != OSI_DISABLE) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			"Invalid filter_enb_dis value\n",
			enb_dis);
		return -1;
	}
	/* validate tcp_udp_match argument */
	if (tcp_udp_match != OSI_ENABLE && tcp_udp_match != OSI_DISABLE) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			"Invalid tcp_udp_match value\n",
			tcp_udp_match);
		return -1;
	}
	/* validate src_dst_port_match argument */
	if (src_dst_port_match != OSI_SOURCE_MATCH &&
	    src_dst_port_match != OSI_INV_MATCH) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			"Invalid src_dst_port_match value\n",
			src_dst_port_match);
		return -1;
	}
	/* validate perfect_inverse_match argument */
	if (perfect_inverse_match != OSI_ENABLE &&
	    perfect_inverse_match != OSI_DISABLE) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			"Invalid perfect_inverse_match value\n",
			perfect_inverse_match);
		return -1;
	}
	if ((dma_routing_enable == OSI_ENABLE) &&
	    (dma_chan > OSI_MGBE_MAX_NUM_CHANS - 1U)) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_OUTOFBOUND,
			"Wrong DMA channel\n",
			(unsigned int)dma_chan);
		return -1;
	}

	mgbe_l3l4_filter_read(osi_core, filter_no, MGBE_MAC_L3L4_CTR, &value);
	value &= ~MGBE_MAC_L3L4_CTR_L4PEN0;
	value |= ((tcp_udp_match << 16) & MGBE_MAC_L3L4_CTR_L4PEN0);

	if (src_dst_port_match == OSI_SOURCE_MATCH) {
		if (enb_dis == OSI_ENABLE) {
			/* Enable L4 filters for SOURCE Port No matching */
			value &= ~MGBE_MAC_L4_SP_CTRL_CLEAR;
			value |= ((MGBE_MAC_L3L4_CTR_L4SPM0 |
				  perfect_inverse_match <<
				  MGBE_MAC_L3L4_CTR_L4SPIM0_SHIFT) &
				  (MGBE_MAC_L3L4_CTR_L4SPM0 |
				  MGBE_MAC_L3L4_CTR_L4SPIM0));
			value |= mgbe_set_dcs(osi_core, value,
					      dma_routing_enable,
					      dma_chan);
		} else {
			/* Disable L4 filters for SOURCE Port No matching  */
			value &= ~MGBE_MAC_L4_SP_CTRL_CLEAR;
		}
	} else {
		if (enb_dis == OSI_ENABLE) {
			/* Enable L4 filters for DESTINATION port No
			 * matching
			 */
			value &= ~MGBE_MAC_L4_DP_CTRL_CLEAR;
			value |= ((MGBE_MAC_L3L4_CTR_L4DPM0 |
				  perfect_inverse_match <<
				  MGBE_MAC_L3L4_CTR_L4DPIM0_SHIFT) &
				  (MGBE_MAC_L3L4_CTR_L4DPM0 |
				  MGBE_MAC_L3L4_CTR_L4DPIM0));
			value |= mgbe_set_dcs(osi_core, value,
					      dma_routing_enable,
					      dma_chan);
		} else {
			/* Disable L4 filters for DESTINATION port No
			 * matching
			 */
			value &= ~MGBE_MAC_L4_DP_CTRL_CLEAR;
		}
	}
	mgbe_l3l4_filter_write(osi_core, filter_no, MGBE_MAC_L3L4_CTR, value);

	/* Set bit corresponding to filter index if value is non-zero */
	mgbe_helper_l3l4_bitmask(&osi_core->l3l4_filter_bitmask,
				 filter_no, value);

	return 0;
}

/**
 * @brief mgbe_config_vlan_filter_reg - config vlan filter register
 *
 * Algorithm: This sequence is used to enable/disable VLAN filtering and
 *	also selects VLAN filtering mode- perfect/hash
 *
 * @param[in] osi_core: Base address  from OSI core private data structure.
 * @param[in] filter_enb_dis: vlan filter enable/disable
 * @param[in] perfect_hash_filtering: perfect or hash filter
 * @param[in] perfect_inverse_match: normal or inverse filter
 *
 * @note 1) MAC should be init and started. see osi_start_mac()
 *	 2) osi_core->osd should be populated
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static int mgbe_config_vlan_filtering(struct osi_core_priv_data *osi_core,
				      unsigned int filter_enb_dis,
				      unsigned int perfect_hash_filtering,
				      unsigned int perfect_inverse_match)
{
	unsigned int value;
	unsigned char *base = osi_core->base;

	/* validate perfect_inverse_match argument */
	if (perfect_hash_filtering == OSI_HASH_FILTER_MODE) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_OPNOTSUPP,
			"VLAN hash filter is not supported, VTHM not updated\n",
			0ULL);
		return -1;
	}
	if (perfect_hash_filtering != OSI_PERFECT_FILTER_MODE) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			"Invalid perfect_hash_filtering value\n",
			perfect_hash_filtering);
		return -1;
	}

	/* validate filter_enb_dis argument */
	if (filter_enb_dis != OSI_ENABLE && filter_enb_dis != OSI_DISABLE) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			"Invalid filter_enb_dis value\n",
			filter_enb_dis);
		return -1;
	}

	/* validate perfect_inverse_match argument */
	if (perfect_inverse_match != OSI_ENABLE &&
	    perfect_inverse_match != OSI_DISABLE) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			"Invalid perfect_inverse_match value\n",
			perfect_inverse_match);
		return -1;
	}

	/* Read MAC PFR value set VTFE bit */
	value = osi_readl(base + MGBE_MAC_PFR);
	value &= ~(MGBE_MAC_PFR_VTFE);
	value |= ((filter_enb_dis << MGBE_MAC_PFR_VTFE_SHIFT) &
		  MGBE_MAC_PFR_VTFE);
	osi_writel(value, base + MGBE_MAC_PFR);

	/* Read MAC VLAN TR register value set VTIM bit */
	value = osi_readl(base + MGBE_MAC_VLAN_TR);
	value &= ~(MGBE_MAC_VLAN_TR_VTIM | MGBE_MAC_VLAN_TR_VTHM);
	value |= ((perfect_inverse_match << MGBE_MAC_VLAN_TR_VTIM_SHIFT) &
		  MGBE_MAC_VLAN_TR_VTIM);
	osi_writel(value, base + MGBE_MAC_VLAN_TR);

	return 0;
}

/**
 * @brief mgbe_flush_mtl_tx_queue - Flush MTL Tx queue
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] qinx: MTL queue index.
 *
 * @note 1) MAC should out of reset and clocks enabled.
 *	 2) hw core initialized. see osi_hw_core_init().
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t mgbe_flush_mtl_tx_queue(struct osi_core_priv_data *const osi_core,
				   const nveu32_t qinx)
{
	void *addr = osi_core->base;
	nveu32_t retry = 1000;
	nveu32_t count;
	nveu32_t value;
	nve32_t cond = 1;

	if (qinx >= OSI_MGBE_MAX_NUM_QUEUES) {
		return -1;
	}

	/* Read Tx Q Operating Mode Register and flush TxQ */
	value = osi_readl((nveu8_t *)addr +
			  MGBE_MTL_CHX_TX_OP_MODE(qinx));
	value |= MGBE_MTL_QTOMR_FTQ;
	osi_writel(value, (nveu8_t *)addr +
		   MGBE_MTL_CHX_TX_OP_MODE(qinx));

	/* Poll Until FTQ bit resets for Successful Tx Q flush */
	count = 0;
	while (cond == 1) {
		if (count > retry) {
			return -1;
		}

		count++;

		value = osi_readl((nveu8_t *)addr +
				  MGBE_MTL_CHX_TX_OP_MODE(qinx));
		if ((value & MGBE_MTL_QTOMR_FTQ_LPOS) == OSI_NONE) {
			cond = 0;
		} else {
			osi_core->osd_ops.msleep(1);
		}
	}

	return 0;
}

/**
 * @brief mgbe_config_mac_loopback - Configure MAC to support loopback
 *
 * @param[in] addr: Base address indicating the start of
 *            memory mapped IO region of the MAC.
 * @param[in] lb_mode: Enable or Disable MAC loopback mode
 *
 * @note MAC should be init and started. see osi_start_mac()
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static int mgbe_config_mac_loopback(struct osi_core_priv_data *const osi_core,
				    unsigned int lb_mode)
{
	unsigned int value;
	void *addr = osi_core->base;

	/* don't allow only if loopback mode is other than 0 or 1 */
	if (lb_mode != OSI_ENABLE && lb_mode != OSI_DISABLE) {
		return -1;
	}

	/* Read MAC Configuration Register */
	value = osi_readl((unsigned char *)addr + MGBE_MAC_RMCR);
	if (lb_mode == OSI_ENABLE) {
		/* Enable Loopback Mode */
		value |= MGBE_MAC_RMCR_LM;
	} else {
		value &= ~MGBE_MAC_RMCR_LM;
	}

	osi_writel(value, (unsigned char *)addr + MGBE_MAC_RMCR);

	return 0;
}

/**
 * @brief mgbe_config_arp_offload - Enable/Disable ARP offload
 *
 * Algorithm:
 *      1) Read the MAC configuration register
 *      2) If ARP offload is to be enabled, program the IP address in
 *      ARPPA register
 *      3) Enable/disable the ARPEN bit in MCR and write back to the MCR.
 *
 * @param[in] addr: MGBE virtual base address.
 * @param[in] enable: Flag variable to enable/disable ARP offload
 * @param[in] ip_addr: IP address of device to be programmed in HW.
 *            HW will use this IP address to respond to ARP requests.
 *
 * @note 1) MAC should be init and started. see osi_start_mac()
 *       2) Valid 4 byte IP address as argument ip_addr
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static int mgbe_config_arp_offload(struct osi_core_priv_data *const osi_core,
				   const unsigned int enable,
				   const unsigned char *ip_addr)
{
	unsigned int mac_rmcr;
	unsigned int val;
	void *addr = osi_core->base;

	if (enable != OSI_ENABLE && enable != OSI_DISABLE) {
		return -1;
	}

	mac_rmcr = osi_readl((unsigned char *)addr + MGBE_MAC_RMCR);

	if (enable == OSI_ENABLE) {
		val = (((unsigned int)ip_addr[0]) << 24) |
		       (((unsigned int)ip_addr[1]) << 16) |
		       (((unsigned int)ip_addr[2]) << 8) |
		       (((unsigned int)ip_addr[3]));

		osi_writel(val, (unsigned char *)addr + MGBE_MAC_ARPPA);

		mac_rmcr |= MGBE_MAC_RMCR_ARPEN;
	} else {
		mac_rmcr &= ~MGBE_MAC_RMCR_ARPEN;
	}

	osi_writel(mac_rmcr, (unsigned char *)addr + MGBE_MAC_RMCR);

	return 0;
}

/**
 * @brief mgbe_config_rxcsum_offload - Enable/Disale rx checksum offload in HW
 *
 * Algorithm:
 *      1) Read the MAC configuration register.
 *      2) Enable the IP checksum offload engine COE in MAC receiver.
 *      3) Update the MAC configuration register.
 *
 * @param[in] addr: MGBE virtual base address.
 * @param[in] enabled: Flag to indicate feature is to be enabled/disabled.
 *
 * @note MAC should be init and started. see osi_start_mac()
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static int mgbe_config_rxcsum_offload(
				struct osi_core_priv_data *const osi_core,
				unsigned int enabled)
{
	void *addr = osi_core->base;
	unsigned int mac_rmcr;

	if (enabled != OSI_ENABLE && enabled != OSI_DISABLE) {
		return -1;
	}

	mac_rmcr = osi_readl((unsigned char *)addr + MGBE_MAC_RMCR);
	if (enabled == OSI_ENABLE) {
		mac_rmcr |= MGBE_MAC_RMCR_IPC;
	} else {
		mac_rmcr &= ~MGBE_MAC_RMCR_IPC;
	}

	osi_writel(mac_rmcr, (unsigned char *)addr + MGBE_MAC_RMCR);

	return 0;
}

/**
 * @brief mgbe_configure_mtl_queue - Configure MTL Queue
 *
 * Algorithm: This takes care of configuring the  below
 *	parameters for the MTL Queue
 *	1) Mapping MTL Rx queue and DMA Rx channel
 *	2) Flush TxQ
 *	3) Enable Store and Forward mode for Tx, Rx
 *	4) Configure Tx and Rx MTL Queue sizes
 *	5) Configure TxQ weight
 *	6) Enable Rx Queues
 *
 * @param[in] qinx: Queue number that need to be configured.
 * @param[in] osi_core: OSI core private data.
 * @param[in] tx_fifo: MTL TX queue size for a MTL queue.
 * @param[in] rx_fifo: MTL RX queue size for a MTL queue.
 *
 * @note MAC has to be out of reset.
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t mgbe_configure_mtl_queue(nveu32_t qinx,
				    struct osi_core_priv_data *osi_core,
				    nveu32_t tx_fifo,
				    nveu32_t rx_fifo)
{
	nveu32_t value = 0;
	nve32_t ret = 0;

	/* Program ETSALG (802.1Qaz) and RAA in MTL_Operation_Mode
	 * register to initialize the MTL operation in case
	 * of multiple Tx and Rx queues default : ETSALG WRR RAA SP
	 */

	/* Program the priorities mapped to the Selected Traffic
	 * Classes in MTL_TC_Prty_Map0-3 registers. This register is
	 * to tell traffic class x should be blocked from transmitting
	 * for the specified pause time when a PFC packet is received
	 * with priorities matching the priorities programmed in this field
	 * default: 0x0
	 */

	/* Program the Transmit Selection Algorithm (TSA) in
	 * MTL_TC[n]_ETS_Control register for all the Selected
	 * Traffic Classes (n=0, 1, â€¦, Max selected number of TCs - 1)
	 * Setting related to CBS will come here for TC.
	 * default: 0x0 SP
	 */
	ret = mgbe_flush_mtl_tx_queue(osi_core, qinx);
	if (ret < 0) {
		return ret;
	}

	value = (tx_fifo << MGBE_MTL_TXQ_SIZE_SHIFT);
	/* Enable Store and Forward mode */
	value |= MGBE_MTL_TSF;
	/*TTC  not applicable for TX*/
	/* Enable TxQ */
	value |= MGBE_MTL_TXQEN;
	osi_writel(value, (nveu8_t *)
		   osi_core->base + MGBE_MTL_CHX_TX_OP_MODE(qinx));

	/* read RX Q0 Operating Mode Register */
	value = osi_readl((nveu8_t *)osi_core->base +
			  MGBE_MTL_CHX_RX_OP_MODE(qinx));
	value |= (rx_fifo << MGBE_MTL_RXQ_SIZE_SHIFT);
	/* Enable Store and Forward mode */
	value |= MGBE_MTL_RSF;

	osi_writel(value, (nveu8_t *)osi_core->base +
		   MGBE_MTL_CHX_RX_OP_MODE(qinx));
	/* Transmit Queue weight */
	value = osi_readl((nveu8_t *)osi_core->base +
			  MGBE_MTL_TXQ_QW(qinx));
	value |= (MGBE_MTL_TXQ_QW_ISCQW + qinx);
	osi_writel(value, (nveu8_t *)osi_core->base +
		   MGBE_MTL_TXQ_QW(qinx));
	/* Enable Rx Queue Control */
	value = osi_readl((nveu8_t *)osi_core->base +
			  MGBE_MAC_RQC0R);
	value |= ((osi_core->rxq_ctrl[qinx] & MGBE_MAC_RXQC0_RXQEN_MASK) <<
		  (MGBE_MAC_RXQC0_RXQEN_SHIFT(qinx)));
	osi_writel(value, (nveu8_t *)osi_core->base +
		   MGBE_MAC_RQC0R);
	return 0;
}

/**
 * @brief mgbe_configure_mac - Configure MAC
 *
 * Algorithm: This takes care of configuring the  below
 *	parameters for the MAC
 *	1) Programming the MAC address
 *	2) Enable required MAC control fields in MCR
 *	3) Enable Multicast and Broadcast Queue
 *	4) Disable MMC nve32_terrupts and Configure the MMC counters
 *	5) Enable required MAC nve32_terrupts
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @note MAC has to be out of reset.
 */
static void mgbe_configure_mac(struct osi_core_priv_data *osi_core)
{
	nveu32_t value;

	/* Update MAC address 0 high */
	value = (((nveu32_t)osi_core->mac_addr[5] << 8U) |
		 ((nveu32_t)osi_core->mac_addr[4]));
	osi_writel(value, (nveu8_t *)osi_core->base + MGBE_MAC_MA0HR);

	/* Update MAC address 0 Low */
	value = (((nveu32_t)osi_core->mac_addr[3] << 24U) |
		 ((nveu32_t)osi_core->mac_addr[2] << 16U) |
		 ((nveu32_t)osi_core->mac_addr[1] << 8U)  |
		 ((nveu32_t)osi_core->mac_addr[0]));
	osi_writel(value, (nveu8_t *)osi_core->base + MGBE_MAC_MA0LR);

	/* TODO: Need to check if we need to enable anything in Tx configuration
	 * value = osi_readl((nveu8_t *)osi_core->base + MGBE_MAC_TMCR);
	 */
	/* Read MAC Rx Configuration Register */
	value = osi_readl((nveu8_t *)osi_core->base + MGBE_MAC_RMCR);
	/* Enable Automatic Pad or CRC Stripping */
	/* Enable CRC stripping for Type packets */
	/* Enable Rx checksum offload engine by default */
	value |= MGBE_MAC_RMCR_ACS | MGBE_MAC_RMCR_CST | MGBE_MAC_RMCR_IPC;

	/* Jumbo Packet Enable */
	if (osi_core->mtu > OSI_DFLT_MTU_SIZE &&
	    osi_core->mtu <= OSI_MTU_SIZE_9000) {
		value |= MGBE_MAC_RMCR_JE;
	} else if (osi_core->mtu > OSI_MTU_SIZE_9000){
		/* if MTU greater 9K use GPSLCE */
		value |= MGBE_MAC_RMCR_GPSLCE | MGBE_MAC_RMCR_WD;
		value &= ~MGBE_MAC_RMCR_GPSL_MSK;
		value |=  ((OSI_MAX_MTU_SIZE << 16) & MGBE_MAC_RMCR_GPSL_MSK);
	} else {
		value &= ~MGBE_MAC_RMCR_JE;
		value &= ~MGBE_MAC_RMCR_GPSLCE;
		value &= ~MGBE_MAC_RMCR_WD;
	}

	osi_writel(value, (unsigned char *)osi_core->base + MGBE_MAC_RMCR);

	value = osi_readl((unsigned char *)osi_core->base + MGBE_MAC_TMCR);
	/* Jabber Disable */
	if (osi_core->mtu > OSI_DFLT_MTU_SIZE) {
		value |= MGBE_MAC_TMCR_JD;
	}
	osi_writel(value, (unsigned char *)osi_core->base + MGBE_MAC_TMCR);

	/* Enable Multicast and Broadcast Queue, default is Q1 */
	value = osi_readl((unsigned char *)osi_core->base + MGBE_MAC_RQC1R);
	value |= MGBE_MAC_RQC1R_MCBCQEN;
	/* Routing Multicast and Broadcast to Q1 */
	value |= MGBE_MAC_RQC1R_MCBCQ1;
	osi_writel(value, (unsigned char *)osi_core->base + MGBE_MAC_RQC1R);

	/* Disable all MMC nve32_terrupts */
	/* Disable all MMC Tx nve32_terrupts */
	osi_writel(OSI_NONE, (nveu8_t *)osi_core->base +
		   MGBE_MMC_TX_INTR_EN);
	/* Disable all MMC RX nve32_terrupts */
	osi_writel(OSI_NONE, (nveu8_t *)osi_core->base +
		   MGBE_MMC_RX_INTR_EN);

	/* Configure MMC counters */
	value = osi_readl((nveu8_t *)osi_core->base + MGBE_MMC_CNTRL);
	value |= MGBE_MMC_CNTRL_CNTRST | MGBE_MMC_CNTRL_RSTONRD |
		 MGBE_MMC_CNTRL_CNTMCT | MGBE_MMC_CNTRL_CNTPRST;
	osi_writel(value, (nveu8_t *)osi_core->base + MGBE_MMC_CNTRL);

	/* Enable MAC nve32_terrupts */
	/* Read MAC IMR Register */
	value = osi_readl((nveu8_t *)osi_core->base + MGBE_MAC_IER);
	/* RGSMIIIM - RGMII/SMII nve32_terrupt Enable */
	/* TODO: LPI need to be enabled during EEE implementation */
	value |= MGBE_IMR_RGSMIIIE;
	osi_writel(value, (nveu8_t *)osi_core->base + MGBE_MAC_IER);

	/* TODO: USP (user Priority) to RxQ Mapping */
}

/**
 * @brief mgbe_configure_dma - Configure DMA
 *
 * Algorithm: This takes care of configuring the  below
 *	parameters for the DMA
 *	1) Programming different burst length for the DMA
 *	2) Enable enhanced Address mode
 *	3) Programming max read outstanding request limit
 *
 * @param[in] base: MGBE virtual base address.
 *
 * @note MAC has to be out of reset.
 */
static void mgbe_configure_dma(void *base)
{
	nveu32_t value = 0;

	/* AXI Burst Length 8*/
	value |= MGBE_DMA_SBUS_BLEN8;
	/* AXI Burst Length 16*/
	value |= MGBE_DMA_SBUS_BLEN16;
	/* Enhanced Address Mode Enable */
	value |= MGBE_DMA_SBUS_EAME;
	/* AXI Maximum Read Outstanding Request Limit = 31 */
	value |= MGBE_DMA_SBUS_RD_OSR_LMT;
	/* AXI Maximum Write Outstanding Request Limit = 31 */
	value |= MGBE_DMA_SBUS_WR_OSR_LMT;

	osi_writel(value, (nveu8_t *)base + MGBE_DMA_SBUS);
}

/**
 * @brief mgbe_core_init - MGBE MAC, MTL and common DMA Initialization
 *
 * Algorithm: This function will take care of initializing MAC, MTL and
 *	common DMA registers.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] tx_fifo_size: MTL TX FIFO size
 * @param[in] rx_fifo_size: MTL RX FIFO size
 *
 * @note 1) MAC should be out of reset. See osi_poll_for_swr() for details.
 *	 2) osi_core->base needs to be filled based on ioremap.
 *	 3) osi_core->num_mtl_queues needs to be filled.
 *	 4) osi_core->mtl_queues[qinx] need to be filled.
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t mgbe_core_init(struct osi_core_priv_data *osi_core,
			  nveu32_t tx_fifo_size,
			  nveu32_t rx_fifo_size)
{
	nve32_t ret = 0;
	nveu32_t qinx = 0;
	nveu32_t value = 0;
	nveu32_t tx_fifo = 0;
	nveu32_t rx_fifo = 0;

	/* reset mmc counters */
	osi_writel(MGBE_MMC_CNTRL_CNTRST, (nveu8_t *)osi_core->base +
		   MGBE_MMC_CNTRL);

	/* Mapping MTL Rx queue and DMA Rx channel */
	value = osi_readl((nveu8_t *)osi_core->base +
			  MGBE_MTL_RXQ_DMA_MAP0);
	value |= MGBE_RXQ_TO_DMA_CHAN_MAP0;
	osi_writel(value, (nveu8_t *)osi_core->base +
		   MGBE_MTL_RXQ_DMA_MAP0);

	value = osi_readl((nveu8_t *)osi_core->base +
			  MGBE_MTL_RXQ_DMA_MAP1);
	value |= MGBE_RXQ_TO_DMA_CHAN_MAP1;
	osi_writel(value, (nveu8_t *)osi_core->base +
		   MGBE_MTL_RXQ_DMA_MAP1);

	value = osi_readl((nveu8_t *)osi_core->base +
			  MGBE_MTL_RXQ_DMA_MAP2);
	value |= MGBE_RXQ_TO_DMA_CHAN_MAP2;
	osi_writel(value, (nveu8_t *)osi_core->base +
		   MGBE_MTL_RXQ_DMA_MAP2);

	/* TODO: DCS enable */

	/* Calculate value of Transmit queue fifo size to be programmed */
	tx_fifo = mgbe_calculate_per_queue_fifo(tx_fifo_size,
						osi_core->num_mtl_queues);

	/* Calculate value of Receive queue fifo size to be programmed */
	rx_fifo = mgbe_calculate_per_queue_fifo(rx_fifo_size,
						osi_core->num_mtl_queues);

	/* Configure MTL Queues */
	/* TODO: Iterate over Number MTL queues need to be removed */
	for (qinx = 0; qinx < osi_core->num_mtl_queues; qinx++) {
		ret = mgbe_configure_mtl_queue(osi_core->mtl_queues[qinx],
					       osi_core, tx_fifo, rx_fifo);
		if (ret < 0) {
			return ret;
		}
	}

	/* configure MGBE MAC HW */
	mgbe_configure_mac(osi_core);

	/* configure MGBE DMA */
	mgbe_configure_dma(osi_core->base);

	/* XPCS initialization */
	return xpcs_init(osi_core);
}

/**
 * @brief mgbe_handle_mac_nve32_trs - Handle MAC nve32_terrupts
 *
 * Algorithm: This function takes care of handling the
 *	MAC nve32_terrupts which includes speed, mode detection.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] dma_isr: DMA ISR register read value.
 *
 * @note MAC nve32_terrupts need to be enabled
 */
static void mgbe_handle_mac_intrs(struct osi_core_priv_data *osi_core,
				  nveu32_t dma_isr)
{
#if 0 /* TODO: Need to re-visit */
	mac_isr = osi_readl((nveu8_t *)osi_core->base + MGBE_MAC_ISR);

	/* Handle MAC interrupts */
	if ((dma_isr & MGBE_DMA_ISR_MACIS) != MGBE_DMA_ISR_MACIS) {
		return;
	}

	/* TODO: Duplex/speed settigs - Its not same as EQOS for MGBE */
#endif
}

/**
 * @brief mgbe_update_dma_sr_stats - stats for dma_status error
 *
 * Algorithm: increament error stats based on corresponding bit filed.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] dma_sr: Dma status register read value
 * @param[in] qinx: Queue index
 */
static inline void mgbe_update_dma_sr_stats(struct osi_core_priv_data *osi_core,
					    nveu32_t dma_sr, nveu32_t qinx)
{
	nveu64_t val;

	if ((dma_sr & MGBE_DMA_CHX_STATUS_RBU) == MGBE_DMA_CHX_STATUS_RBU) {
		val = osi_core->xstats.rx_buf_unavail_irq_n[qinx];
		osi_core->xstats.rx_buf_unavail_irq_n[qinx] =
			osi_update_stats_counter(val, 1U);
	}
	if ((dma_sr & MGBE_DMA_CHX_STATUS_TPS) == MGBE_DMA_CHX_STATUS_TPS) {
		val = osi_core->xstats.tx_proc_stopped_irq_n[qinx];
		osi_core->xstats.tx_proc_stopped_irq_n[qinx] =
			osi_update_stats_counter(val, 1U);
	}
	if ((dma_sr & MGBE_DMA_CHX_STATUS_TBU) == MGBE_DMA_CHX_STATUS_TBU) {
		val = osi_core->xstats.tx_buf_unavail_irq_n[qinx];
		osi_core->xstats.tx_buf_unavail_irq_n[qinx] =
			osi_update_stats_counter(val, 1U);
	}
	if ((dma_sr & MGBE_DMA_CHX_STATUS_RPS) == MGBE_DMA_CHX_STATUS_RPS) {
		val = osi_core->xstats.rx_proc_stopped_irq_n[qinx];
		osi_core->xstats.rx_proc_stopped_irq_n[qinx] =
			osi_update_stats_counter(val, 1U);
	}
	if ((dma_sr & MGBE_DMA_CHX_STATUS_FBE) == MGBE_DMA_CHX_STATUS_FBE) {
		val = osi_core->xstats.fatal_bus_error_irq_n;
		osi_core->xstats.fatal_bus_error_irq_n =
			osi_update_stats_counter(val, 1U);
	}
}

/**
 * @brief mgbe_handle_common_intr - Handles common nve32_terrupt.
 *
 * Algorithm: Clear common nve32_terrupt source.
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @note MAC should be init and started. see osi_start_mac()
 */
static void mgbe_handle_common_intr(struct osi_core_priv_data *osi_core)
{
	void *base = osi_core->base;
	nveu32_t dma_isr = 0;
	nveu32_t qinx = 0;
	nveu32_t i = 0;
	nveu32_t dma_sr = 0;
	nveu32_t dma_ier = 0;

	dma_isr = osi_readl((nveu8_t *)base + MGBE_DMA_ISR);
	if (dma_isr == OSI_NONE) {
		return;
	}

	//FIXME Need to check how we can get the DMA channel here instead of
	//MTL Queues
	if ((dma_isr & MGBE_DMA_ISR_DCH0_DCH15_MASK) != OSI_NONE) {
		/* Handle Non-TI/RI nve32_terrupts */
		for (i = 0; i < osi_core->num_mtl_queues; i++) {
			qinx = osi_core->mtl_queues[i];

			if (qinx >= OSI_MGBE_MAX_NUM_CHANS) {
				continue;
			}

			/* read dma channel status register */
			dma_sr = osi_readl((nveu8_t *)base +
					   MGBE_DMA_CHX_STATUS(qinx));
			/* read dma channel nve32_terrupt enable register */
			dma_ier = osi_readl((nveu8_t *)base +
					    MGBE_DMA_CHX_IER(qinx));

			/* process only those nve32_terrupts which we
			 * have enabled.
			 */
			dma_sr = (dma_sr & dma_ier);

			/* mask off RI and TI */
			dma_sr &= ~(MGBE_DMA_CHX_STATUS_TI |
				    MGBE_DMA_CHX_STATUS_RI);
			if (dma_sr == OSI_NONE) {
				continue;
			}

			/* ack non ti/ri nve32_ts */
			osi_writel(dma_sr, (nveu8_t *)base +
				   MGBE_DMA_CHX_STATUS(qinx));
			mgbe_update_dma_sr_stats(osi_core, dma_sr, qinx);
		}
	}

	mgbe_handle_mac_intrs(osi_core, dma_isr);
}

/**
 * @brief mgbe_pad_calibrate - PAD calibration
 *
 * Algorithm: Since PAD calibration not applicable for MGBE
 * it returns zero.
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @retval zero always
 */
static nve32_t mgbe_pad_calibrate(struct osi_core_priv_data *const osi_core)
{
	return 0;
}

/**
 * @brief mgbe_start_mac - Start MAC Tx/Rx engine
 *
 * Algorithm: Enable MAC Transmitter and Receiver
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @note 1) MAC init should be complete. See osi_hw_core_init() and
 *	 osi_hw_dma_init()
 */
static void mgbe_start_mac(struct osi_core_priv_data *const osi_core)
{
	nveu32_t value;
	void *addr = osi_core->base;

	value = osi_readl((nveu8_t *)addr + MGBE_MAC_TMCR);
	/* Enable MAC Transmit */
	value |= MGBE_MAC_TMCR_TE;
	osi_writel(value, (nveu8_t *)addr + MGBE_MAC_TMCR);

	value = osi_readl((nveu8_t *)addr + MGBE_MAC_RMCR);
	/* Enable MAC Receive */
	value |= MGBE_MAC_RMCR_RE;
	osi_writel(value, (nveu8_t *)addr + MGBE_MAC_RMCR);
}

/**
 * @brief mgbe_stop_mac - Stop MAC Tx/Rx engine
 *
 * Algorithm: Disables MAC Transmitter and Receiver
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @note MAC DMA deinit should be complete. See osi_hw_dma_deinit()
 */
static void mgbe_stop_mac(struct osi_core_priv_data *const osi_core)
{
	nveu32_t value;
	void *addr = osi_core->base;

	value = osi_readl((nveu8_t *)addr + MGBE_MAC_TMCR);
	/* Disable MAC Transmit */
	value &= ~MGBE_MAC_TMCR_TE;
	osi_writel(value, (nveu8_t *)addr + MGBE_MAC_TMCR);

	value = osi_readl((nveu8_t *)addr + MGBE_MAC_RMCR);
	/* Disable MAC Receive */
	value &= ~MGBE_MAC_RMCR_RE;
	osi_writel(value, (nveu8_t *)addr + MGBE_MAC_RMCR);
}

/**
 * @brief mgbe_core_deinit - MGBE MAC core deinitialization
 *
 * Algorithm: This function will take care of deinitializing MAC
 *
 * @param[in] osi_core: OSI core private data structure.
 *
 * @note Required clks and resets has to be enabled
 */
static void mgbe_core_deinit(struct osi_core_priv_data *osi_core)
{
	/* Stop the MAC by disabling both MAC Tx and Rx */
	mgbe_stop_mac(osi_core);
}

/**
 * @brief mgbe_set_speed - Set operating speed
 *
 * Algorithm: Based on the speed (2.5G/5G/10G) MAC will be configured
 *        accordingly.
 *
 * @param[in] osi_core:	OSI core private data.
 * @param[in] speed:    Operating speed.
 *
 * @note MAC should be init and started. see osi_start_mac()
 */
static int mgbe_set_speed(struct osi_core_priv_data *const osi_core, const int speed)
{
	unsigned int value = 0;

	value = osi_readl((unsigned char *)osi_core->base + MGBE_MAC_TMCR);

	switch (speed) {
	case OSI_SPEED_2500:
		value |= MGBE_MAC_TMCR_SS_2_5G;
		break;
	case OSI_SPEED_5000:
		value |= MGBE_MAC_TMCR_SS_5G;
		break;
	case OSI_SPEED_10000:
		value &= ~MGBE_MAC_TMCR_SS_10G;
		break;
	default:
		/* setting default to 10G */
		value &= ~MGBE_MAC_TMCR_SS_10G;
		break;
	}

	osi_writel(value, (unsigned char *)osi_core->base + MGBE_MAC_TMCR);

	return xpcs_start(osi_core);
}

static int mgbe_mdio_busy_wait(struct osi_core_priv_data *const osi_core)
{
	/* half second timeout */
	unsigned int retry = 50000;
	unsigned int mac_gmiiar;
	unsigned int count;
	int cond = 1;

	count = 0;
	while (cond == 1) {
		if (count > retry) {
			return -1;
		}

		count++;

		mac_gmiiar = osi_readl((unsigned char *)osi_core->base +
				       MGBE_MDIO_SCCD);
		if ((mac_gmiiar & MGBE_MDIO_SCCD_SBUSY) == 0U) {
			cond = 0;
		} else {
			osi_core->osd_ops.udelay(10U);
		}
	}

	return 0;
}

/**
 * @brief mgbe_write_phy_reg - Write to a PHY register over MDIO bus.
 *
 * Algorithm: Write into a PHY register through MGBE MDIO bus.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] phyaddr: PHY address (PHY ID) associated with PHY
 * @param[in] phyreg: Register which needs to be write to PHY.
 * @param[in] phydata: Data to write to a PHY register.
 *
 * @note MAC should be init and started. see osi_start_mac()
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static int mgbe_write_phy_reg(struct osi_core_priv_data *osi_core,
			      unsigned int phyaddr,
			      unsigned int phyreg,
			      unsigned short phydata)
{
	int ret = 0;
	unsigned int reg;

	if (osi_core == OSI_NULL) {
		OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID, "osi_core is NULL\n",
			0ULL);
		return -1;
	}

	/* Wait for any previous MII read/write operation to complete */
	ret = mgbe_mdio_busy_wait(osi_core);
	if (ret < 0) {
		OSI_CORE_ERR(osi_core->osd,
			OSI_LOG_ARG_HW_FAIL,
			"MII operation timed out\n",
			0ULL);
		return ret;
	}

	/* set MDIO address register */
	/* set device address */
	reg = ((phyreg >> MGBE_MDIO_C45_DA_SHIFT) & MGBE_MDIO_SCCA_DA_MASK) <<
	       MGBE_MDIO_SCCA_DA_SHIFT;
	/* set port address and register address */
	reg |= (phyaddr << MGBE_MDIO_SCCA_PA_SHIFT) |
		(phyreg & MGBE_MDIO_SCCA_RA_MASK);
	osi_writel(reg, (unsigned char *)osi_core->base + MGBE_MDIO_SCCA);

	/* Program Data register */
	reg = phydata |
	      (MGBE_MDIO_SCCD_CMD_WR << MGBE_MDIO_SCCD_CMD_SHIFT) |
	      MGBE_MDIO_SCCD_SBUSY;

	/**
	 * On FPGA AXI/APB clock is 13MHz. To achive maximum MDC clock
	 * of 2.5MHz need to enable CRS and CR to be set to 1.
	 * On Silicon AXI/APB clock is 408MHz. To achive maximum MDC clock
	 * of 2.5MHz only CR need to be set to 5.
	 */
	if (osi_core->pre_si) {
		reg |= (MGBE_MDIO_SCCD_CRS |
			((0x1U & MGBE_MDIO_SCCD_CR_MASK) <<
			MGBE_MDIO_SCCD_CR_SHIFT));
	} else {
		reg &= ~MGBE_MDIO_SCCD_CRS;
		reg |= ((0x5U & MGBE_MDIO_SCCD_CR_MASK) <<
			MGBE_MDIO_SCCD_CR_SHIFT);
	}

	osi_writel(reg, (unsigned char *)osi_core->base + MGBE_MDIO_SCCD);

	/* wait for MII write operation to complete */
	ret = mgbe_mdio_busy_wait(osi_core);
	if (ret < 0) {
		OSI_CORE_ERR(osi_core->osd,
			OSI_LOG_ARG_HW_FAIL,
			"MII operation timed out\n",
			0ULL);
		return ret;
	}

	return 0;
}

/**
 * @brief mgbe_read_phy_reg - Read from a PHY register over MDIO bus.
 *
 * Algorithm: Write into a PHY register through MGBE MDIO bus.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] phyaddr: PHY address (PHY ID) associated with PHY
 * @param[in] phyreg: Register which needs to be read from PHY.
 *
 * @note MAC should be init and started. see osi_start_mac()
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static int mgbe_read_phy_reg(struct osi_core_priv_data *osi_core,
			     unsigned int phyaddr,
			     unsigned int phyreg)
{
	unsigned int reg;
	unsigned int data;
	int ret = 0;

	ret = mgbe_mdio_busy_wait(osi_core);
	if (ret < 0) {
		OSI_CORE_ERR(osi_core->osd,
			OSI_LOG_ARG_HW_FAIL,
			"MII operation timed out\n",
			0ULL);
		return ret;
	}

	/* set MDIO address register */
	/* set device address */
	reg = ((phyreg >> MGBE_MDIO_C45_DA_SHIFT) & MGBE_MDIO_SCCA_DA_MASK) <<
	       MGBE_MDIO_SCCA_DA_SHIFT;
	/* set port address and register address */
	reg |= (phyaddr << MGBE_MDIO_SCCA_PA_SHIFT) |
		(phyreg & MGBE_MDIO_SCCA_RA_MASK);
	osi_writel(reg, (unsigned char *)osi_core->base + MGBE_MDIO_SCCA);

	/* Program Data register */
	reg = (MGBE_MDIO_SCCD_CMD_RD << MGBE_MDIO_SCCD_CMD_SHIFT) |
	       MGBE_MDIO_SCCD_SBUSY;

	 /**
         * On FPGA AXI/APB clock is 13MHz. To achive maximum MDC clock
         * of 2.5MHz need to enable CRS and CR to be set to 1.
         * On Silicon AXI/APB clock is 408MHz. To achive maximum MDC clock
         * of 2.5MHz only CR need to be set to 5.
         */
        if (osi_core->pre_si) {
                reg |= (MGBE_MDIO_SCCD_CRS |
                        ((0x1U & MGBE_MDIO_SCCD_CR_MASK) <<
                        MGBE_MDIO_SCCD_CR_SHIFT));
        } else {
                reg &= ~MGBE_MDIO_SCCD_CRS;
                reg |= ((0x5U & MGBE_MDIO_SCCD_CR_MASK) <<
                        MGBE_MDIO_SCCD_CR_SHIFT);
        }

	osi_writel(reg, (unsigned char *)osi_core->base + MGBE_MDIO_SCCD);

	ret = mgbe_mdio_busy_wait(osi_core);
	if (ret < 0) {
		OSI_CORE_ERR(osi_core->osd,
			OSI_LOG_ARG_HW_FAIL,
			"MII operation timed out\n",
			0ULL);
		return ret;
	}

	reg = osi_readl((unsigned char *)osi_core->base + MGBE_MDIO_SCCD);

	data = (reg & MGBE_MDIO_SCCD_SDATA_MASK);
        return (int)data;
}

/**
 * @brief mgbe_init_core_ops - Initialize MGBE MAC core operations
 */
void mgbe_init_core_ops(struct core_ops *ops)
{
	ops->poll_for_swr = mgbe_poll_for_swr;
	ops->core_init = mgbe_core_init;
	ops->core_deinit = mgbe_core_deinit;
	ops->validate_regs = OSI_NULL;
	ops->start_mac = mgbe_start_mac;
	ops->stop_mac = mgbe_stop_mac;
	ops->handle_common_intr = mgbe_handle_common_intr;
	/* only MGBE supports full duplex */
	ops->set_mode = OSI_NULL;
	/* by default speed is 10G */
	ops->set_speed = mgbe_set_speed;
	ops->pad_calibrate = mgbe_pad_calibrate;
	ops->set_mdc_clk_rate = OSI_NULL;
	ops->flush_mtl_tx_queue = mgbe_flush_mtl_tx_queue;
	ops->config_mac_loopback = mgbe_config_mac_loopback;
	ops->set_avb_algorithm = OSI_NULL;
	ops->get_avb_algorithm = OSI_NULL;
	ops->config_fw_err_pkts = OSI_NULL;
	ops->config_tx_status = OSI_NULL;
	ops->config_rx_crc_check = OSI_NULL;
	ops->config_flow_control = OSI_NULL;
	ops->config_arp_offload = mgbe_config_arp_offload;
	ops->config_rxcsum_offload = mgbe_config_rxcsum_offload;
	ops->config_mac_pkt_filter_reg = mgbe_config_mac_pkt_filter_reg;
	ops->update_mac_addr_low_high_reg = mgbe_update_mac_addr_low_high_reg;
	ops->config_l3_l4_filter_enable = mgbe_config_l3_l4_filter_enable;
	ops->config_l3_filters = mgbe_config_l3_filters;
	ops->update_ip4_addr = mgbe_update_ip4_addr;
	ops->update_ip6_addr = mgbe_update_ip6_addr;
	ops->config_l4_filters = mgbe_config_l4_filters;
	ops->update_l4_port_no = mgbe_update_l4_port_no;
	ops->config_vlan_filtering = mgbe_config_vlan_filtering,
	ops->update_vlan_id = OSI_NULL;
	ops->set_systime_to_mac = OSI_NULL;
	ops->config_addend = OSI_NULL;
	ops->adjust_mactime = OSI_NULL,
	ops->config_tscr = OSI_NULL;
	ops->config_ssir = OSI_NULL;
	ops->write_phy_reg = mgbe_write_phy_reg;
	ops->read_phy_reg = mgbe_read_phy_reg;
	ops->read_mmc = mgbe_read_mmc;
	ops->reset_mmc = mgbe_reset_mmc;
};
