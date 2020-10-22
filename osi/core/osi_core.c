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

#include <osi_core.h>
#include <osd.h>
#include <local_common.h>
#include <ivc_core.h>

nve32_t osi_write_phy_reg(struct osi_core_priv_data *const osi_core,
			  const nveu32_t phyaddr, const nveu32_t phyreg,
			  const nveu16_t phydata)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->write_phy_reg != OSI_NULL)) {
		return osi_core->ops->write_phy_reg(osi_core,
						    phyaddr, phyreg, phydata);
	}
	return -1;
}

nve32_t osi_read_phy_reg(struct osi_core_priv_data *const osi_core,
			 const nveu32_t phyaddr, const nveu32_t phyreg)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->read_phy_reg != OSI_NULL)) {
		return osi_core->ops->read_phy_reg(osi_core, phyaddr, phyreg);
	}

	return -1;
}

nve32_t osi_init_core_ops(struct osi_core_priv_data *const osi_core)
{
	/*
	 * Currently these osd_ops are optional to be filled in the OSD layer.
	 * If OSD updates these pointers, use the same. If not, fall back to the
	 * existing way of using osd_* API's.
	 * TODO: Once These API's are mandatory, return errors instead of
	 * default API usage.
	 */
	if (osi_core == OSI_NULL) {
		return -1;
	}
	if (osi_core->osd_ops.ops_log == OSI_NULL) {
		osi_core->osd_ops.ops_log = osd_log;
	}
	if (osi_core->osd_ops.udelay == OSI_NULL) {
		osi_core->osd_ops.udelay = osd_udelay;
	}
	if (osi_core->osd_ops.msleep == OSI_NULL) {
		osi_core->osd_ops.msleep = osd_msleep;
	}
	if (osi_core->osd_ops.usleep_range == OSI_NULL) {
		osi_core->osd_ops.usleep_range = osd_usleep_range;
	}

	if (osi_core->mac == OSI_MAC_HW_EQOS) {
		if (osi_core->use_virtualization == OSI_DISABLE) {
			/* Get EQOS HW ops */
			osi_core->ops = eqos_get_hw_core_ops();
			/* Explicitly set osi_core->safety_config = OSI_NULL if
			 * a particular MAC version does not need SW safety
			 * mechanisms like periodic read-verify.
			 */
			osi_core->safety_config =
					(void *)eqos_get_core_safety_config();
		} else {
#ifdef LINUX_IVC
			/* Get IVC HW ops */
			osi_core->ops = ivc_get_hw_core_ops();
			/* Explicitly set osi_core->safety_config = OSI_NULL if
			 * a particular MAC version does not need SW safety
			 * mechanisms like periodic read-verify.
			 */
			osi_core->safety_config =
					(void *)ivc_get_core_safety_config();
#endif
		}
		return 0;
	}

	return -1;
}

nve32_t osi_poll_for_mac_reset_complete(
			struct osi_core_priv_data *const osi_core)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->poll_for_swr != OSI_NULL)) {
		return osi_core->ops->poll_for_swr(osi_core);
	}

	return -1;
}

nve32_t osi_hw_core_init(struct osi_core_priv_data *const osi_core,
			 nveu32_t tx_fifo_size, nveu32_t rx_fifo_size)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->core_init != OSI_NULL)) {
		return osi_core->ops->core_init(osi_core, tx_fifo_size,
						rx_fifo_size);
	}

	return -1;
}

nve32_t osi_hw_core_deinit(struct osi_core_priv_data *const osi_core)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->core_deinit != OSI_NULL)) {
		osi_core->ops->core_deinit(osi_core);
		return 0;
	}

	return -1;
}

nve32_t osi_start_mac(struct osi_core_priv_data *const osi_core)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->start_mac != OSI_NULL)) {
		osi_core->ops->start_mac(osi_core);
		return 0;
	}

	return -1;
}

nve32_t osi_stop_mac(struct osi_core_priv_data *const osi_core)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->stop_mac != OSI_NULL)) {
		osi_core->ops->stop_mac(osi_core);
		return 0;
	}

	return -1;
}

nve32_t osi_common_isr(struct osi_core_priv_data *const osi_core)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->handle_common_intr != OSI_NULL)) {
		osi_core->ops->handle_common_intr(osi_core);
		return 0;
	}

	return -1;
}

nve32_t osi_set_mode(struct osi_core_priv_data *const osi_core,
		     const nve32_t mode)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->set_mode != OSI_NULL)) {
		return osi_core->ops->set_mode(osi_core, mode);
	}

	return -1;
}

nve32_t osi_set_speed(struct osi_core_priv_data *const osi_core,
		      const nve32_t speed)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->set_speed != OSI_NULL)) {
		osi_core->ops->set_speed(osi_core, speed);
		return 0;
	}

	return -1;
}

nve32_t osi_pad_calibrate(struct osi_core_priv_data *const osi_core)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->pad_calibrate != OSI_NULL) &&
	    (osi_core->base != OSI_NULL)) {
		return osi_core->ops->pad_calibrate(osi_core);
	}

	return -1;
}

nve32_t osi_config_fw_err_pkts(struct osi_core_priv_data *const osi_core,
			       const nveu32_t qinx, const nveu32_t fw_err)
{
	/* Configure Forwarding of Error packets */
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->config_fw_err_pkts != OSI_NULL)) {
		return osi_core->ops->config_fw_err_pkts(osi_core,
							 qinx, fw_err);
	}

	return -1;
}

nve32_t osi_l2_filter(struct osi_core_priv_data *const osi_core,
		      const struct osi_filter *filter)
{
	struct osi_core_ops *op;
	nve32_t ret = -1;

	if ((osi_core == OSI_NULL) || (osi_core->ops == OSI_NULL) ||
	    (osi_core->base == OSI_NULL) || (filter == OSI_NULL)) {
		return ret;
	}

	op = osi_core->ops;

	if ((op->config_mac_pkt_filter_reg != OSI_NULL)) {
		ret = op->config_mac_pkt_filter_reg(osi_core, filter);
	} else {
		OSI_CORE_INFO(osi_core->osd, OSI_LOG_ARG_INVALID,
			      "op->config_mac_pkt_filter_reg is null\n", 0ULL);
		return ret;
	}

	if (((filter->oper_mode & OSI_OPER_ADDR_UPDATE) != OSI_NONE) ||
	    ((filter->oper_mode & OSI_OPER_ADDR_DEL) != OSI_NONE)) {
		ret = -1;

		if ((filter->dma_routing == OSI_ENABLE) &&
		    (osi_core->dcs_en != OSI_ENABLE)) {
			OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
				     "DCS requested. Conflicts with DT config\n",
				     0ULL);
			return ret;
		}

		if ((op->update_mac_addr_low_high_reg != OSI_NULL)) {
			ret = op->update_mac_addr_low_high_reg(osi_core,
								filter);
		} else {
			OSI_CORE_INFO(osi_core->osd, OSI_LOG_ARG_INVALID,
				      "op->update_mac_addr_low_high_reg is null\n",
				      0ULL);
		}
	}

	return ret;
}

/**
 * @brief helper_l4_filter helper function for l4 filtering
 *
 * @param[in] osi_core: OSI Core private data structure.
 * @param[in] l_filter: filter structure
 * @param[in] type: filter type l3 or l4
 * @param[in] dma_routing_enable: dma routing enable (1) or disable (0)
 * @param[in] dma_chan: dma channel
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
static inline nve32_t helper_l4_filter(
				   struct osi_core_priv_data *const osi_core,
				   struct osi_l3_l4_filter l_filter,
				   nveu32_t type,
				   nveu32_t dma_routing_enable,
				   nveu32_t dma_chan)
{
	struct osi_core_ops *op = osi_core->ops;

	if (op->config_l4_filters != OSI_NULL) {
		if (op->config_l4_filters(osi_core,
					  l_filter.filter_no,
					  l_filter.filter_enb_dis,
					  type,
					  l_filter.src_dst_addr_match,
					  l_filter.perfect_inverse_match,
					  dma_routing_enable,
					  dma_chan) < 0) {
			return -1;
		}
	} else {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			     "op->config_l4_filters is NULL\n", 0ULL);
		return -1;
	}

	if (op->update_l4_port_no != OSI_NULL) {
		return op->update_l4_port_no(osi_core,
					     l_filter.filter_no,
					     l_filter.port_no,
					     l_filter.src_dst_addr_match);
	} else {
		OSI_CORE_INFO(osi_core->osd, OSI_LOG_ARG_INVALID,
			      "op->update_l4_port_no is NULL\n", 0ULL);
		return -1;
	}

}

/**
 * @brief helper_l3_filter helper function for l3 filtering
 *
 * @param[in] osi_core: OSI Core private data structure.
 * @param[in] l_filter: filter structure
 * @param[in] type: filter type l3 or l4
 * @param[in] dma_routing_enable: dma routing enable (1) or disable (0)
 * @param[in] dma_chan: dma channel
 *
 * @pre MAC needs to be out of reset and proper clock configured.
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on Success
 * @retval -1 on Failure
 */
static inline nve32_t helper_l3_filter(
				   struct osi_core_priv_data *const osi_core,
				   struct osi_l3_l4_filter l_filter,
				   nveu32_t type,
				   nveu32_t dma_routing_enable,
				   nveu32_t dma_chan)
{
	struct osi_core_ops *op = osi_core->ops;

	if ((op->config_l3_filters != OSI_NULL)) {
		if (op->config_l3_filters(osi_core,
					  l_filter.filter_no,
					  l_filter.filter_enb_dis,
					  type,
					  l_filter.src_dst_addr_match,
					  l_filter.perfect_inverse_match,
					  dma_routing_enable,
					  dma_chan) < 0) {
			return -1;
		}
	} else {
		OSI_CORE_INFO(osi_core->osd, OSI_LOG_ARG_INVALID,
			      "op->config_l3_filters is NULL\n", 0ULL);
		return -1;
	}

	if (type == OSI_IP6_FILTER) {
		if (op->update_ip6_addr != OSI_NULL) {
			return op->update_ip6_addr(osi_core, l_filter.filter_no,
					  l_filter.ip6_addr);
		} else {
			OSI_CORE_INFO(osi_core->osd, OSI_LOG_ARG_INVALID,
				      "op->update_ip6_addr is NULL\n", 0ULL);
			return -1;
		}
	} else if (type == OSI_IP4_FILTER) {
		if (op->update_ip4_addr != OSI_NULL) {
			return op->update_ip4_addr(osi_core,
						   l_filter.filter_no,
						   l_filter.ip4_addr,
						   l_filter.src_dst_addr_match);
		} else {
			OSI_CORE_INFO(osi_core->osd, OSI_LOG_ARG_INVALID,
				      "op->update_ip4_addr is NULL\n", 0ULL);
			return -1;
		}
	} else {
		return -1;
	}
}

nve32_t osi_l3l4_filter(struct osi_core_priv_data *const osi_core,
			const struct osi_l3_l4_filter l_filter,
			const nveu32_t type, const nveu32_t dma_routing_enable,
			const nveu32_t dma_chan, const nveu32_t is_l4_filter)
{
	nve32_t ret = -1;

	if ((osi_core == OSI_NULL) || (osi_core->ops == OSI_NULL) ||
	    (osi_core->base == OSI_NULL)) {
		return ret;
	}

	if ((dma_routing_enable == OSI_ENABLE) &&
	    (osi_core->dcs_en != OSI_ENABLE)) {
		OSI_CORE_ERR(osi_core->osd, OSI_LOG_ARG_INVALID,
			     "dma routing enabled but dcs disabled in DT\n",
			     0ULL);
		return ret;
	}

	if (is_l4_filter == OSI_ENABLE) {
		ret = helper_l4_filter(osi_core, l_filter, type,
				       dma_routing_enable, dma_chan);
	} else {
		ret = helper_l3_filter(osi_core, l_filter, type,
				       dma_routing_enable, dma_chan);
	}

	if (ret < 0) {
		OSI_CORE_INFO(osi_core->osd, OSI_LOG_ARG_INVALID,
			      "L3/L4 helper function failed\n", 0ULL);
		return ret;
	}

	if (osi_core->ops->config_l3_l4_filter_enable != OSI_NULL) {
		if (osi_core->l3l4_filter_bitmask != OSI_DISABLE) {
			ret = osi_core->ops->config_l3_l4_filter_enable(
								osi_core,
								OSI_ENABLE);
		} else {
			ret = osi_core->ops->config_l3_l4_filter_enable(
								osi_core,
								OSI_DISABLE);
		}

	} else {
		OSI_CORE_INFO(osi_core->osd, OSI_LOG_ARG_INVALID,
			      "op->config_l3_l4_filter_enable is NULL\n", 0ULL);
		ret = -1;
	}

	return ret;
}

nve32_t osi_config_rxcsum_offload(struct osi_core_priv_data *const osi_core,
				  const nveu32_t enable)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->config_rxcsum_offload != OSI_NULL)) {
		return osi_core->ops->config_rxcsum_offload(osi_core,
							    enable);
	}

	return -1;
}

nve32_t osi_set_systime_to_mac(struct osi_core_priv_data *const osi_core,
			       const nveu32_t sec, const nveu32_t nsec)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->set_systime_to_mac != OSI_NULL)) {
		return osi_core->ops->set_systime_to_mac(osi_core,
							 sec,
							 nsec);
	}

	return -1;
}

/**
 * @brief div_u64 - Calls a function which returns quotient
 *
 * @param[in] dividend: Dividend
 * @param[in] divisor: Divisor
 *
 * @pre MAC IP should be out of reset and need to be initialized as the
 *      requirements.
 *
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 * @returns Quotient
 */
static inline nveu64_t div_u64(nveu64_t dividend,
			       nveu64_t divisor)
{
	nveu64_t remain;

	return div_u64_rem(dividend, divisor, &remain);
}

nve32_t osi_adjust_freq(struct osi_core_priv_data *const osi_core, nve32_t ppb)
{
	nveu64_t adj;
	nveu64_t temp;
	nveu32_t diff = 0;
	nveu32_t addend;
	nveu32_t neg_adj = 0;
	nve32_t ret = -1;
	nve32_t ppb1 = ppb;

	if (osi_core == OSI_NULL) {
		return ret;
	}

	addend = osi_core->default_addend;
	if (ppb1 < 0) {
		neg_adj = 1U;
		ppb1 = -ppb1;
		adj = (nveu64_t)addend * (nveu32_t)ppb1;
	} else {
		adj = (nveu64_t)addend * (nveu32_t)ppb1;
	}

	/*
	 * div_u64 will divide the "adj" by "1000000000ULL"
	 * and return the quotient.
	 */
	temp = div_u64(adj, OSI_NSEC_PER_SEC);
	if (temp < UINT_MAX) {
		diff = (nveu32_t)temp;
	} else {
		OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID, "temp > UINT_MAX\n",
			     0ULL);
		return ret;
	}

	if (neg_adj == 0U) {
		if (addend <= (UINT_MAX - diff)) {
			addend = (addend + diff);
		} else {
			OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
				     "addend > UINT_MAX\n", 0ULL);
			return -1;
		}
	} else {
		if (addend > diff) {
			addend = addend - diff;
		} else if (addend < diff) {
			addend = diff - addend;
		} else {
			OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
				     "addend = diff\n", 0ULL);
		}
	}

	if ((osi_core->ops != OSI_NULL) && (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->config_addend != OSI_NULL)) {
		return osi_core->ops->config_addend(osi_core, addend);
	}

	OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID, "core: Invalid argument\n",
		     0ULL);

	return ret;
}

nve32_t osi_adjust_time(struct osi_core_priv_data *const osi_core,
			nvel64_t nsec_delta)
{
	nveu32_t neg_adj = 0;
	nveu32_t sec = 0, nsec = 0;
	nveu64_t quotient;
	nveu64_t reminder = 0;
	nveu64_t udelta = 0;
	nve32_t ret = -1;
	nvel64_t nsec_delta1 = nsec_delta;

	if (osi_core == OSI_NULL) {
		return ret;
	}

	if (nsec_delta1 < 0) {
		neg_adj = 1;
		nsec_delta1 = -nsec_delta1;
		udelta = (nveul64_t)nsec_delta1;
	} else {
		udelta = (nveul64_t)nsec_delta1;
	}

	quotient = div_u64_rem(udelta, OSI_NSEC_PER_SEC, &reminder);
	if (quotient <= UINT_MAX) {
		sec = (nveu32_t)quotient;
	} else {
		OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			     "quotient > UINT_MAX\n", 0ULL);
		return ret;
	}
	if (reminder <= UINT_MAX) {
		nsec = (nveu32_t)reminder;
	} else {
		OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
			     "reminder > UINT_MAX\n", 0ULL);
		return ret;
	}

	if ((osi_core->ops != OSI_NULL) && (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->adjust_mactime != OSI_NULL)) {
		return osi_core->ops->adjust_mactime(osi_core, sec, nsec,
					neg_adj,
					osi_core->ptp_config.one_nsec_accuracy);
	}

	OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID, "core: Invalid argument\n",
		     0ULL);

	return ret;
}

nve32_t osi_ptp_configuration(struct osi_core_priv_data *const osi_core,
			      const nveu32_t enable)
{
	nve32_t ret = 0;
	nveu64_t temp = 0, temp1 = 0, temp2 = 0;
	nveu64_t ssinc = 0;

	if ((osi_core == OSI_NULL) || (osi_core->ops == OSI_NULL) ||
	    (osi_core->base == OSI_NULL) ||
	    (osi_core->ops->config_tscr == OSI_NULL) ||
	    (osi_core->ops->config_ssir == OSI_NULL) ||
	    (osi_core->ops->config_addend == OSI_NULL) ||
	    (osi_core->ops->set_systime_to_mac == OSI_NULL)) {
		return -1;
	}

	if (enable == OSI_DISABLE) {
		/* disable hw time stamping */
		/* Program MAC_Timestamp_Control Register */
		osi_core->ops->config_tscr(osi_core, OSI_DISABLE);
	} else {
		/* Program MAC_Timestamp_Control Register */
		osi_core->ops->config_tscr(osi_core,
					   osi_core->ptp_config.ptp_filter);

		/* Program Sub Second Increment Register */
		osi_core->ops->config_ssir(osi_core);

		/* formula for calculating addend value is
		 * TSAR = (2^32 * 1000) / (ptp_ref_clk_rate in MHz * SSINC)
		 * 2^x * y == (y << x), hence
		 * 2^32 * 1000 == (1000 << 32)
		 * so addend = (2^32 * 1000)/(ptp_ref_clk_rate in MHZ * SSINC);
		 */

		if (osi_core->mac_ver <= OSI_EQOS_MAC_4_10) {
			ssinc = OSI_PTP_SSINC_16;
		} else {
			ssinc = OSI_PTP_SSINC_4;
		}

		temp = ((nveu64_t)1000 << 32);
		temp = (nveu64_t)temp * 1000000U;

		temp1 = div_u64(temp,
			(nveu64_t)osi_core->ptp_config.ptp_ref_clk_rate);

		temp2 = div_u64(temp1, (nveu64_t)ssinc);

		if (temp2 < UINT_MAX) {
			osi_core->default_addend = (nveu32_t)temp2;
		} else {
			OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_INVALID,
				     "core: temp2 >= UINT_MAX\n", 0ULL);
			return -1;
		}

		/* Program addend value */
		ret = osi_core->ops->config_addend(osi_core,
					osi_core->default_addend);

		/* Set current time */
		ret = osi_core->ops->set_systime_to_mac(osi_core,
						osi_core->ptp_config.sec,
						osi_core->ptp_config.nsec);
	}

	return ret;
}

nve32_t osi_read_mmc(struct osi_core_priv_data *const osi_core)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->read_mmc != OSI_NULL)) {
		osi_core->ops->read_mmc(osi_core);
		return 0;
	}

	return -1;
}

nveu32_t osi_read_reg(struct osi_core_priv_data *const osi_core,
		     const nve32_t addr)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->read_reg != OSI_NULL) &&
	    (osi_core->base != OSI_NULL)) {
		return osi_core->ops->read_reg(osi_core, addr);
	}

	return 0;
}


nveu32_t osi_write_reg(struct osi_core_priv_data *const osi_core,
		      const nveu32_t val, const nve32_t addr)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->write_reg != OSI_NULL) &&
	    (osi_core->base != OSI_NULL)) {
		return osi_core->ops->write_reg(osi_core, val, addr);
	}

	return 0;
}

nve32_t osi_get_mac_version(struct osi_core_priv_data *const osi_core,
		            nveu32_t *mac_ver)
{
	nveu32_t macver;

	macver = osi_read_reg(osi_core, (nve32_t) MAC_VERSION) &
			      MAC_VERSION_SNVER_MASK;
	if (is_valid_mac_version(macver) == 0) {
		return -1;
	}

	*mac_ver = macver;
	return 0;
}

void osi_get_hw_features(struct osi_core_priv_data *const osi_core,
		      struct osi_hw_features *hw_feat)
{
	nveu32_t mac_hfr0;
	nveu32_t mac_hfr1;
	nveu32_t mac_hfr2;

	if (hw_feat != OSI_NULL) {
		/* TODO: need to add HFR3 */
		mac_hfr0 = osi_read_reg(osi_core, EQOS_MAC_HFR0);
		mac_hfr1 = osi_read_reg(osi_core, EQOS_MAC_HFR1);
		mac_hfr2 = osi_read_reg(osi_core, EQOS_MAC_HFR2);

		hw_feat->mii_sel =
			((mac_hfr0 >> 0) & EQOS_MAC_HFR0_MIISEL_MASK);
		hw_feat->gmii_sel =
			((mac_hfr0 >> 1U) & EQOS_MAC_HFR0_GMIISEL_MASK);
		hw_feat->hd_sel =
			((mac_hfr0 >> 2U) & EQOS_MAC_HFR0_HDSEL_MASK);
		hw_feat->pcs_sel =
			((mac_hfr0 >> 3U) & EQOS_MAC_HFR0_PCSSEL_MASK);
		hw_feat->sma_sel =
			((mac_hfr0 >> 5U) & EQOS_MAC_HFR0_SMASEL_MASK);
		hw_feat->rwk_sel =
			((mac_hfr0 >> 6U) & EQOS_MAC_HFR0_RWKSEL_MASK);
		hw_feat->mgk_sel =
			((mac_hfr0 >> 7U) & EQOS_MAC_HFR0_MGKSEL_MASK);
		hw_feat->mmc_sel =
			((mac_hfr0 >> 8U) & EQOS_MAC_HFR0_MMCSEL_MASK);
		hw_feat->arp_offld_en =
			((mac_hfr0 >> 9U) & EQOS_MAC_HFR0_ARPOFFLDEN_MASK);
		hw_feat->ts_sel =
			((mac_hfr0 >> 12U) & EQOS_MAC_HFR0_TSSSEL_MASK);
		hw_feat->eee_sel =
			((mac_hfr0 >> 13U) & EQOS_MAC_HFR0_EEESEL_MASK);
		hw_feat->tx_coe_sel =
			((mac_hfr0 >> 14U) & EQOS_MAC_HFR0_TXCOESEL_MASK);
		hw_feat->rx_coe_sel =
			((mac_hfr0 >> 16U) & EQOS_MAC_HFR0_RXCOE_MASK);
		hw_feat->mac_addr_sel =
			((mac_hfr0 >> 18U) & EQOS_MAC_HFR0_ADDMACADRSEL_MASK);
		hw_feat->mac_addr32_sel =
			((mac_hfr0 >> 23U) & EQOS_MAC_HFR0_MACADR32SEL_MASK);
		hw_feat->mac_addr64_sel =
			((mac_hfr0 >> 24U) & EQOS_MAC_HFR0_MACADR64SEL_MASK);
		hw_feat->tsstssel =
			((mac_hfr0 >> 25U) & EQOS_MAC_HFR0_TSINTSEL_MASK);
		hw_feat->sa_vlan_ins =
			((mac_hfr0 >> 27U) & EQOS_MAC_HFR0_SAVLANINS_MASK);
		hw_feat->act_phy_sel =
			((mac_hfr0 >> 28U) & EQOS_MAC_HFR0_ACTPHYSEL_MASK);
		hw_feat->rx_fifo_size =
			((mac_hfr1 >> 0) & EQOS_MAC_HFR1_RXFIFOSIZE_MASK);
		hw_feat->tx_fifo_size =
			((mac_hfr1 >> 6U) & EQOS_MAC_HFR1_TXFIFOSIZE_MASK);
		hw_feat->adv_ts_hword =
			((mac_hfr1 >> 13U) & EQOS_MAC_HFR1_ADVTHWORD_MASK);
		hw_feat->addr_64 =
			((mac_hfr1 >> 14U) & EQOS_MAC_HFR1_ADDR64_MASK);
		hw_feat->dcb_en =
			((mac_hfr1 >> 16U) & EQOS_MAC_HFR1_DCBEN_MASK);
		hw_feat->sph_en =
			((mac_hfr1 >> 17U) & EQOS_MAC_HFR1_SPHEN_MASK);
		hw_feat->tso_en =
			((mac_hfr1 >> 18U) & EQOS_MAC_HFR1_TSOEN_MASK);
		hw_feat->dma_debug_gen =
			((mac_hfr1 >> 19U) & EQOS_MAC_HFR1_DMADEBUGEN_MASK);
		hw_feat->av_sel =
			((mac_hfr1 >> 20U) & EQOS_MAC_HFR1_AVSEL_MASK);
		hw_feat->hash_tbl_sz =
			((mac_hfr1 >> 24U) & EQOS_MAC_HFR1_HASHTBLSZ_MASK);
		hw_feat->l3l4_filter_num =
			((mac_hfr1 >> 27U) & EQOS_MAC_HFR1_L3L4FILTERNUM_MASK);
		hw_feat->rx_q_cnt =
			((mac_hfr2 >> 0) & EQOS_MAC_HFR2_RXQCNT_MASK);
		hw_feat->tx_q_cnt =
			((mac_hfr2 >> 6U) & EQOS_MAC_HFR2_TXQCNT_MASK);
		hw_feat->rx_ch_cnt =
			((mac_hfr2 >> 12U) & EQOS_MAC_HFR2_RXCHCNT_MASK);
		hw_feat->tx_ch_cnt =
			((mac_hfr2 >> 18U) & EQOS_MAC_HFR2_TXCHCNT_MASK);
		hw_feat->pps_out_num =
			((mac_hfr2 >> 24U) & EQOS_MAC_HFR2_PPSOUTNUM_MASK);
		hw_feat->aux_snap_num =
			((mac_hfr2 >> 28U) & EQOS_MAC_HFR2_AUXSNAPNUM_MASK);
	}
}

#ifndef OSI_STRIPPED_LIB
nve32_t osi_validate_core_regs(struct osi_core_priv_data *const osi_core)
{
	nve32_t ret = -1;

	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->validate_regs != OSI_NULL) &&
	    (osi_core->safety_config != OSI_NULL)) {
		ret = osi_core->ops->validate_regs(osi_core);
		return ret;
	}

	return ret;
}

nve32_t osi_flush_mtl_tx_queue(struct osi_core_priv_data *const osi_core,
			       const nveu32_t qinx)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->flush_mtl_tx_queue != OSI_NULL)) {
		return osi_core->ops->flush_mtl_tx_queue(osi_core, qinx);
	}

	return -1;
}

nve32_t osi_set_avb(struct osi_core_priv_data *const osi_core,
		    const struct osi_core_avb_algorithm *avb)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->set_avb_algorithm != OSI_NULL)) {
		return osi_core->ops->set_avb_algorithm(osi_core, avb);
	}

	return -1;
}

nve32_t osi_get_avb(struct osi_core_priv_data *const osi_core,
		    struct osi_core_avb_algorithm *avb)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->get_avb_algorithm != OSI_NULL)) {
		return osi_core->ops->get_avb_algorithm(osi_core, avb);
	}

	return -1;
}

nve32_t osi_configure_txstatus(struct osi_core_priv_data *const osi_core,
			       const nveu32_t tx_status)
{
	/* Configure Drop Transmit Status */
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->config_tx_status != OSI_NULL)) {
		return osi_core->ops->config_tx_status(osi_core,
						       tx_status);
	}

	return -1;
}

nve32_t osi_config_rx_crc_check(struct osi_core_priv_data *const osi_core,
				const nveu32_t crc_chk)
{
	/* Configure CRC Checking for Received Packets */
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->config_rx_crc_check != OSI_NULL)) {
		return osi_core->ops->config_rx_crc_check(osi_core,
							  crc_chk);
	}

	return -1;
}

nve32_t osi_config_vlan_filtering(struct osi_core_priv_data *const osi_core,
				  const nveu32_t filter_enb_dis,
				  const nveu32_t perfect_hash_filtering,
				  const nveu32_t perfect_inverse_match)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->config_vlan_filtering != OSI_NULL)) {
		return osi_core->ops->config_vlan_filtering(
							osi_core,
							filter_enb_dis,
							perfect_hash_filtering,
							perfect_inverse_match);
	}

	return -1;
}

nve32_t  osi_update_vlan_id(struct osi_core_priv_data *const osi_core,
			    const nveu32_t vid)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->update_vlan_id != OSI_NULL)) {
		return osi_core->ops->update_vlan_id(osi_core,
						     vid);
	}

	return -1;
}

nve32_t osi_get_systime_from_mac(struct osi_core_priv_data *const osi_core,
				 nveu32_t *sec,
				 nveu32_t *nsec)
{
	if ((osi_core != OSI_NULL) && (osi_core->dma_base != OSI_NULL)) {
		common_get_systime_from_mac(osi_core->dma_base,
					    osi_core->mac, sec,
					    nsec);
	} else {
		return -1;
	}

	return 0;
}

nve32_t osi_reset_mmc(struct osi_core_priv_data *const osi_core)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->reset_mmc != OSI_NULL)) {
		osi_core->ops->reset_mmc(osi_core);
		return 0;
	}

	return -1;
}

nve32_t osi_configure_eee(struct osi_core_priv_data *const osi_core,
			  nveu32_t tx_lpi_enabled, nveu32_t tx_lpi_timer)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->configure_eee != OSI_NULL) &&
	    (tx_lpi_timer <= OSI_MAX_TX_LPI_TIMER) &&
	    (tx_lpi_timer >= OSI_MIN_TX_LPI_TIMER) &&
	    (tx_lpi_timer % OSI_MIN_TX_LPI_TIMER == OSI_NONE)) {
		osi_core->ops->configure_eee(osi_core, tx_lpi_enabled,
					     tx_lpi_timer);
		return 0;
	}

	return -1;
}

nve32_t osi_save_registers(struct osi_core_priv_data *const osi_core)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->save_registers != OSI_NULL)) {
		/* Call MAC save registers callback and return the value */
		return osi_core->ops->save_registers(osi_core);
	}

	return -1;
}

nve32_t osi_restore_registers(struct osi_core_priv_data *const osi_core)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->restore_registers != OSI_NULL)) {
		/* Call MAC restore registers callback and return the value */
		return osi_core->ops->restore_registers(osi_core);
	}

	return -1;
}

nve32_t osi_configure_flow_control(struct osi_core_priv_data *const osi_core,
				   const nveu32_t flw_ctrl)
{
	/* Configure Flow control settings */
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->base != OSI_NULL) &&
	    (osi_core->ops->config_flow_control != OSI_NULL)) {
		return osi_core->ops->config_flow_control(osi_core,
							  flw_ctrl);
	}

	return -1;
}

nve32_t osi_config_arp_offload(struct osi_core_priv_data *const osi_core,
			       const nveu32_t flags,
			       const nveu8_t *ip_addr)
{
	if (osi_core != OSI_NULL && osi_core->ops != OSI_NULL &&
	    (osi_core->base != OSI_NULL) && (ip_addr != OSI_NULL) &&
	    (osi_core->ops->config_arp_offload != OSI_NULL)) {
		return osi_core->ops->config_arp_offload(osi_core,
							 flags, ip_addr);
	}

	return -1;
}

nve32_t osi_set_mdc_clk_rate(struct osi_core_priv_data *const osi_core,
			     const nveu64_t csr_clk_rate)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->set_mdc_clk_rate != OSI_NULL)) {
		osi_core->ops->set_mdc_clk_rate(osi_core, csr_clk_rate);
		return 0;
	}

	return -1;
}

nve32_t osi_config_mac_loopback(struct osi_core_priv_data *const osi_core,
				const nveu32_t lb_mode)
{
	/* Configure MAC loopback */
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->config_mac_loopback != OSI_NULL)) {
		return osi_core->ops->config_mac_loopback(osi_core,
							  lb_mode);
	}

	return -1;
}
#endif /* !OSI_STRIPPED_LIB */
