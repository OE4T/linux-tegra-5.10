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

#include <osd.h>
#include "eqos_common.h"

void osi_get_hw_features(void *base, struct osi_hw_features *hw_feat)
{
	unsigned int mac_hfr0;
	unsigned int mac_hfr1;
	unsigned int mac_hfr2;

	/* TODO: need to add HFR3 */
	mac_hfr0 = osi_readl((unsigned char *)base + EQOS_MAC_HFR0);
	mac_hfr1 = osi_readl((unsigned char *)base + EQOS_MAC_HFR1);
	mac_hfr2 = osi_readl((unsigned char *)base + EQOS_MAC_HFR2);

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

int osi_get_mac_version(void *addr, unsigned int *mac_ver)
{
	unsigned int macver;
	int ret = 0;

	macver = osi_readl((unsigned char *)addr + MAC_VERSION) &
		MAC_VERSION_SNVER_MASK;
	if (is_valid_mac_version(macver) == 0) {
		return -1;
	}

	*mac_ver = macver;
	return ret;
}

void osi_memset(void *s, unsigned int c, unsigned long count)
{
	unsigned char *xs = s;

	while (count != 0UL) {
		if (c < OSI_UCHAR_MAX) {
			*xs++ = (unsigned char)c;
		}
		count--;
	}
}

/**
 *@brief div_u64_rem - updates remainder and returns Quotient
 *
 * Algorithm: Dividend will be divided by divisor and stores the
 *	 remainder value and returns quotient
 *
 * @param[in] dividend: Dividend value
 * @param[in] divisor: Divisor value
 * @param[out] remain: Remainder
 *
 * @note MAC IP should be out of reset and need to be initialized as the
 *	 requirements
 *
 * @returns Quotient
 */
static inline unsigned long div_u64_rem(unsigned long dividend,
					unsigned long divisor,
					unsigned long *remain)
{
	unsigned long ret = 0;

	if (divisor != 0U) {
		*remain = dividend % divisor;
		ret = dividend / divisor;
	} else {
		ret = 0;
	}
	return ret;
}

void common_get_systime_from_mac(void *addr, unsigned int mac,
				 unsigned int *sec, unsigned int *nsec)
{
	unsigned long temp;
	unsigned long remain;
	unsigned long long ns;

	if (mac == OSI_MAC_HW_EQOS) {
		ns = eqos_get_systime_from_mac(addr);
	} else {
		/* Non EQOS HW is supported yet */
		return;
	}

	temp = div_u64_rem((unsigned long)ns, OSI_NSEC_PER_SEC, &remain);
	if (temp < UINT_MAX) {
		*sec = (unsigned int) temp;
	} else {
		/* do nothing here */
	}
	if (remain < UINT_MAX) {
		*nsec = (unsigned int)remain;
	} else {
		/* do nothing here */
	}
}
