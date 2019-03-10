/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION. All rights reserved.
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

#ifndef OSI_CORE_H
#define OSI_CORE_H

#include "osi_common.h"

struct osi_core_priv_data;

/**
 *	struct osi_core_avb_algorithm - The OSI Core avb data sctructure per
 *	queue.
 *	@qindex: TX Queue/TC index
 *	@algo: AVB algorithm 1:CBS
 *	@credit_control: credit control When this bit is set, the accumulated
 *	credit. Parameter in the credit-based shaper algorithm logic is not
 *	reset to zero when there is positive credit and no packet to transmit
 *	in Channel
 *	@idle_slope: idleSlopeCredit value required for CBS
 *	@send_slope: sendSlopeCredit value required for CBS
 *	@hi_credit: hiCredit value required for CBS
 *	@low_credit: lowCredit value required for CBS
 *	@oper_mode: Transmit queue enable 01: avb 10: enable 00: disable
 */
struct  osi_core_avb_algorithm {
	unsigned int qindex;
	unsigned int algo;
	unsigned int credit_control;
	unsigned int idle_slope;
	unsigned int send_slope;
	unsigned int hi_credit;
	unsigned int low_credit;
	unsigned int oper_mode;
};

/**
 *	struct osi_core_ops - Core (MAC & MTL) operations.
 *	@poll_for_swr: Called to poll for software reset bit.
 *	@core_init: Called to initialize MAC and MTL registers.
 *	@start_mac: Called to start MAC Tx and Rx engine.
 *	@stop_mac: Called to stop MAC Tx and Rx engine.
 *	@handle_common_intr: Called to handle common interrupt.
 *	@set_mode: Called to set the mode at MAC (full/duplex).
 *	@set_speed: Called to set the speed (10/100/1000) at MAC.
 *	@pad_calibrate: Called to do pad caliberation.
 *	@set_mdc_clk_rate: Called to set MDC clock rate for MDIO operation.
 *	@flush_mtl_tx_queue: Called to flush MTL Tx queue.
 *	@config_mac_loopback: Called to configure MAC in loopback mode.
 *	@set_avb_algorithm: Called to set av parameter.
 *	@get_avb_algorithm: Called to get av parameter,
 */
struct osi_core_ops {
	/* initialize MAC/MTL/DMA Common registers */
	int (*poll_for_swr)(void *ioaddr);
	int (*core_init)(struct osi_core_priv_data *osi_core,
			 unsigned int tx_fifo_size,
			 unsigned int rx_fifo_size);
	void (*start_mac)(void *addr);
	void (*stop_mac)(void *addr);
	void (*handle_common_intr)(struct osi_core_priv_data *osi_core);
	void (*set_mode)(void *ioaddr, int mode);
	void (*set_speed)(void *ioaddr, int speed);
	int (*pad_calibrate)(void *ioaddr);
	void (*set_mdc_clk_rate)(struct osi_core_priv_data *osi_core,
				 unsigned long csr_clk_rate);
	int (*flush_mtl_tx_queue)(void *ioaddr, unsigned int qinx);
	int (*config_mac_loopback)(void *addr, unsigned int lb_mode);
	int (*set_avb_algorithm)(struct osi_core_priv_data *osi_core,
				 struct osi_core_avb_algorithm *avb);
	int (*get_avb_algorithm)(struct osi_core_priv_data *osi_core,
				 struct osi_core_avb_algorithm *avb);
};

/**
 *	struct osi_core_priv_data - The OSI Core (MAC & MTL) private data
				    structure.
 *	@base: Memory mapped base address of MAC IP.
 *	@osd:	Pointer to OSD private data structure.
 *	@ops: Address of HW Core operations structure.
 *	@num_mtl_queues: Number of MTL queues enabled in MAC.
 *	@mtl_queues: Array of MTL queues.
 *	@rxq_ctrl: List of MTL Rx queue mode that need to be enabled
 *	@mac: MAC HW type EQOS based on DT compatible.
 *	@mac_ver: MAC version.
 *	@mdc_cr: MDC clock rate.
 *	@mtu: MTU size
 *	@mac_addr: Ethernet MAC address.
 */
struct osi_core_priv_data {
	void *base;
	void *osd;
	struct osi_core_ops *ops;
	unsigned int num_mtl_queues;
	unsigned int mtl_queues[OSI_EQOS_MAX_NUM_CHANS];
	unsigned int rxq_ctrl[OSI_EQOS_MAX_NUM_CHANS];
	unsigned int mac;
	unsigned int mac_ver;
	unsigned int mdc_cr;
	unsigned int mtu;
	unsigned char mac_addr[OSI_ETH_ALEN];
};

/**
 *	osi_poll_for_swr - Poll Software reset bit in MAC HW
 *	@osi: OSI Core private data structure.
 *
 *	Algorithm: Invokes EQOS routine to check for SWR (software reset)
 *	bit in DMA Basic mooe register to make sure IP reset was successful.
 *
 *	Dependencies: MAC needs to be reset with Soft or hardreset.
 *
 *	Protection: None
 *
 *	Return: 0 - success, -1 - failure
 */

static inline int osi_poll_for_swr(struct osi_core_priv_data *osi_core)
{
	int ret = 0;

	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->poll_for_swr != OSI_NULL)) {
		ret = osi_core->ops->poll_for_swr(osi_core->base);
	}

	return ret;
}

/**
 *      osi_set_mdc_clk_rate - Derive MDC clock based on provided AXI_CBB clk.
 *      @osi: OSI core private data structure.
 *      @csr_clk_rate: CSR (AXI CBB) clock rate.
 *
 *      Algorithm: MDC clock rate will be populated in OSI core private data
 *	structure based on AXI_CBB clock rate.
 *
 *      Dependencies: OSD layer needs get the AXI CBB clock rate with OSD clock
 *      API (ex - clk_get_rate())
 *
 *      Return: None
 */
static inline void osi_set_mdc_clk_rate(struct osi_core_priv_data *osi_core,
					unsigned long csr_clk_rate)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->set_mdc_clk_rate != OSI_NULL)) {
		osi_core->ops->set_mdc_clk_rate(osi_core, csr_clk_rate);
	}
}

/**
 *      osi_hw_core_init - EQOS MAC, MTL and common DMA initialization.
 *      @osi: OSI core private data structure.
 *
 *      Algorithm: Invokes EQOS MAC, MTL and common DMA register init code.
 *
 *      Dependencies: MAC has to be out of reset.
 *
 *      Protection: None
 *
 *      Return: 0 - success, -1 - failure
 */
static inline int osi_hw_core_init(struct osi_core_priv_data *osi_core,
				   unsigned int tx_fifo_size,
				   unsigned int rx_fifo_size)
{
	int ret = 0;

	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->core_init != OSI_NULL)) {
		ret = osi_core->ops->core_init(osi_core, tx_fifo_size,
					       rx_fifo_size);
	}

	return ret;
}

/**
 *      osi_start_mac - Start MAC Tx/Rx engine
 *      @osi_core: OSI core private data.
 *
 *      Algorimthm: Enable MAC Tx and Rx engine.
 *      Dependencies: None
 *      Protection: None
 *      Return: None
 */
static inline void osi_start_mac(struct osi_core_priv_data *osi_core)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->start_mac != OSI_NULL)) {
		osi_core->ops->start_mac(osi_core->base);
	}
}

/**
 *      osi_stop_mac - Stop MAC Tx/Rx engine
 *      @osi_core: OSI core private data.
 *
 *      Algorimthm: Stop MAC Tx and Rx engine
 *      Dependencies: None
 *      Protection: None
 *      Return: None
 */
static inline void osi_stop_mac(struct osi_core_priv_data *osi_core)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->stop_mac != OSI_NULL)) {
		osi_core->ops->stop_mac(osi_core->base);
	}
}

/**
 *      osi_common_isr - Common ISR.
 *      @osi_core: OSI core private data structure.
 *
 *      Algorithm: Takes care of handling the
 *      common interrupts accordingly as per the
 *      MAC IP
 *
 *      Dependencies: MAC IP should be out of reset
 *      and need to be initialized as the requirements
 *
 *      Protection: None
 *
 *      Return: 0 - success, -1 - failure
 */
static inline void osi_common_isr(struct osi_core_priv_data *osi_core)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->handle_common_intr != OSI_NULL)) {
		osi_core->ops->handle_common_intr(osi_core);
	}
}

/**
 *      osi_set_mode - Set FD/HD mode.
 *      @osi: OSI private data structure.
 *      @mode: Operating mode.
 *
 *      Algorithm: Takes care of  setting HD or FD mode
 *      accordingly as per the MAC IP.
 *
 *      Dependencies: MAC IP should be out of reset
 *      and need to be initialized as the requirements
 *
 *      Protection: None
 *
 *      Return: NONE
 */
static inline void osi_set_mode(struct osi_core_priv_data *osi_core, int mode)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->set_mode != OSI_NULL)) {
		osi_core->ops->set_mode(osi_core->base, mode);
	}
}

/**
 *      osi_set_speed - Set operating speed.
 *      @osi: OSI private data structure.
 *      @speed: Operating speed.
 *
 *      Algorithm: Takes care of  setting the operating
 *      speed accordingly as per the MAC IP.
 *
 *      Dependencies: MAC IP should be out of reset
 *      and need to be initialized as the requirements
 *
 *      Protection: None
 *
 *      Return: NONE
 */
static inline void osi_set_speed(struct osi_core_priv_data *osi_core, int speed)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->set_speed != OSI_NULL)) {
		osi_core->ops->set_speed(osi_core->base, speed);
	}
}

/**
 *      osi_pad_calibrate - PAD calibration
 *      @osi: OSI core private data structure.
 *
 *      Algorithm: Takes care of  doing the pad calibration
 *      accordingly as per the MAC IP.
 *
 *      Dependencies: RGMII and MDIO interface needs to be IDLE
 *      before performing PAD calibration.
 *
 *      Protection: None
 *
 *      Return: 0 - success, -1 - failure
 */
static inline int osi_pad_calibrate(struct osi_core_priv_data *osi_core)
{
	int ret = 0;

	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->pad_calibrate != OSI_NULL)) {
		ret = osi_core->ops->pad_calibrate(osi_core);
	}

	return ret;
}

/**
 *      osi_flush_mtl_tx_queue - Flushing a MTL Tx Queue.
 *      @osi_core: OSI private data structure.
 *      @qinx: MTL queue index.
 *
 *      Algorithm: Invokes EQOS flush Tx queue routine.
 *
 *      Dependencies: MAC IP should be out of reset
 *      and need to be initialized as the requirements
 *
 *      Protection: None
 *
 *      Return: 0 - success, -1 - failure.
 */
static inline int osi_flush_mtl_tx_queue(struct osi_core_priv_data *osi_core,
					 unsigned int qinx)
{
	int ret = 0;

	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->flush_mtl_tx_queue != OSI_NULL)) {
		ret = osi_core->ops->flush_mtl_tx_queue(osi_core->base, qinx);
	}

	return ret;
}

/**
 *	osi_config_mac_loopback - Configure MAC loopback
 *	@osi: OSI private data structure.
 *	@lb_mode: Enable or disable MAC loopback
 *
 *	Algorithm: Configure the MAC to support the loopback.
 *
 *	Dependencies: MAC IP should be out of reset
 *	and need to be initialized as the requirements
 *
 *	Protection: None
 *
 *      Return: 0 - success, -1 - failure.
 */
static inline int osi_config_mac_loopback(struct osi_core_priv_data *osi_core,
					   unsigned int lb_mode)
{
	int ret = -1;

	/* Configure MAC LoopBack */
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->config_mac_loopback != OSI_NULL)) {
		ret = osi_core->ops->config_mac_loopback(osi_core->base,
							 lb_mode);
	}

	return ret;
}

/**
 *	osi_set_avb - Set CBS algo and parameters
 *	@osi: OSI core private data structure.
 *	@avb: osi core avb data structure.
 *
 *	Algorithm: Set AVB algo and  populated parameter from osi_core_avb
 *	structure for TC/TQ
 *
 *      Dependencies: MAC IP should be out of reset and need to be initialized
 *	as the requirements
 *
 *	Return: Success = 0; failure = -1;
 */
static inline int osi_set_avb(struct osi_core_priv_data *osi_core,
			      struct osi_core_avb_algorithm *avb)
{
	int ret = -1;

	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->set_avb_algorithm != OSI_NULL)) {
		ret = osi_core->ops->set_avb_algorithm(osi_core, avb);
	}

	return ret;
}

/**
 *	osi_get_avb - Set CBS algo and parameters
 *	@osi: OSI core private data structure.
 *	@avb: osi core avb data structure.
 *
 *	Algorithm: get AVB algo and  populated parameter from osi_core_avb
 *	structure for TC/TQ
 *
 *      Dependencies: MAC IP should be out of reset and need to be initialized
 *	as the requirements
 *
 *	Return: Success = 0; failure = -1;
 */
static inline int osi_get_avb(struct osi_core_priv_data *osi_core,
			      struct osi_core_avb_algorithm *avb)
{
	int ret = -1;

	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->get_avb_algorithm != OSI_NULL)) {
		ret = osi_core->ops->get_avb_algorithm(osi_core, avb);
	}

	return ret;
}

int osi_write_phy_reg(struct osi_core_priv_data *osi_core, unsigned int phyaddr,
		      unsigned int phyreg, unsigned short phydata);
int osi_read_phy_reg(struct osi_core_priv_data *osi_core, unsigned int phyaddr,
		     unsigned int phyreg);
void osi_init_core_ops(struct osi_core_priv_data *osi_core);
#endif /* OSI_CORE_H */
