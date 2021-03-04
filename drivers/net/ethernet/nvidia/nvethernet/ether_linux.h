/*
 * Copyright (c) 2018-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ETHER_LINUX_H
#define ETHER_LINUX_H

#include <linux/platform/tegra/ptp-notifier.h>
#include <linux/ptp_clock_kernel.h>
#include <linux/platform_device.h>
#include <linux/etherdevice.h>
#include <linux/net_tstamp.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/of_gpio.h>
#include <linux/of_mdio.h>
#include <linux/if_vlan.h>
#include <linux/thermal.h>
#include <linux/debugfs.h>
#include <linux/of_net.h>
#include <linux/module.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/of.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/version.h>
#if (KERNEL_VERSION(5, 4, 0) > LINUX_VERSION_CODE)
#include <linux/tegra-ivc.h>
#include <soc/tegra/chip-id.h>
#else
#include <soc/tegra/fuse.h>
#endif
#include <osi_core.h>
#include <osi_dma.h>
#include <mmc.h>
#include "ioctl.h"

/**
 * @brief Max number of Ethernet IRQs supported in HW
 */
#define ETHER_MAX_IRQS			4
/**
 * @brief Maximum index for IRQ numbers array.
 */
#define ETHER_IRQ_MAX_IDX		8
/**
 * @brief Size of Ethernet IRQ name.
 */
#define ETHER_IRQ_NAME_SZ		32

/**
 * @addtogroup Ethernet Transmit Queue Priority
 *
 * @brief Macros to define the default, maximum and invalid range of Transmit
 * queue priority. These macros are used to check the bounds of Tx queue
 * priority provided in the device tree.
 * @{
 */
#define ETHER_QUEUE_PRIO_DEFAULT	0U
#define ETHER_QUEUE_PRIO_MAX		7U
#define ETHER_QUEUE_PRIO_INVALID	0xFFU
/** @} */

/**
 * @brief Ethernet default PTP clock frequency
 */
#define ETHER_DFLT_PTP_CLK		312500000U

/**
 * @addtogroup CONFIG Ethernet configuration error codes
 *
 * @brief Error codes for fail/success.
 * @{
 */
#define EQOS_CONFIG_FAIL		-3
#define EQOS_CONFIG_SUCCESS		0
/** @} */

/**
 * @addtogroup ADDR Ethernet MAC address register count
 *
 * @brief MAC L2 address filter count
 * @{
 */
#define ETHER_ADDR_REG_CNT_128		128
#define ETHER_ADDR_REG_CNT_64		64
#define ETHER_ADDR_REG_CNT_32		32
#define ETHER_ADDR_REG_CNT_1		1
/** @} */

/**
 * @addtogroup HW MAC HW Filter Hash Table size
 *
 * @brief Represents Hash Table sizes.
 * @{
 */
#define HW_HASH_TBL_SZ_3		3
#define HW_HASH_TBL_SZ_2		2
#define HW_HASH_TBL_SZ_1		1
#define HW_HASH_TBL_SZ_0		0
/** @} */

/**
 * @brief Maximum buffer length per DMA descriptor (16KB).
 */
#define ETHER_TX_MAX_BUFF_SIZE	0x3FFF

/**
 * @brief Maximum skb frame(GSO/TSO) size (64KB)
 */
#define ETHER_TX_MAX_FRAME_SIZE	GSO_MAX_SIZE

/**
 * @brief Check if Tx data buffer length is within bounds.
 *
 * Algorithm: Check the data length if it is valid.
 *
 * @param[in] length: Tx data buffer length to check
 *
 * @retval true if length is valid
 * @retval false otherwise
 */
static inline bool valid_tx_len(unsigned int length)
{
	if (length > 0U && length <= ETHER_TX_MAX_FRAME_SIZE) {
		return true;
	} else {
		return false;
	}
}

/* Descriptors required for maximum contiguous TSO/GSO packet
 * one extra descriptor if there is linear buffer payload
 */
#define ETHER_TX_MAX_SPLIT	((GSO_MAX_SIZE / ETHER_TX_MAX_BUFF_SIZE) + 1)

/* Maximum possible descriptors needed for an SKB:
 * - Maximum number of SKB frags
 * - Maximum descriptors for contiguous TSO/GSO packet
 * - Possible context descriptor
 * - Possible TSO header descriptor
 */
#define ETHER_TX_DESC_THRESHOLD	(MAX_SKB_FRAGS + ETHER_TX_MAX_SPLIT + 2)

#define ETHER_TX_MAX_FRAME	(TX_DESC_CNT / ETHER_TX_DESC_THRESHOLD)
/**
 *@brief Returns count of available transmit descriptors
 *
 * Algorithm: Check the difference between current descriptor index
 * and the descriptor index to be cleaned.
 *
 * @param[in] tx_ring: Tx ring instance associated with channel number
 *
 * @note MAC needs to be initialized and Tx ring allocated.
 *
 * @returns Number of available descriptors in the given Tx ring.
 */
static inline int ether_avail_txdesc_cnt(struct osi_tx_ring *tx_ring)
{
	return ((tx_ring->clean_idx - tx_ring->cur_tx_idx - 1) &
		(TX_DESC_CNT - 1));
}

#ifdef THERMAL_CAL
/* @brief The DT binding for ethernet device has 5 thermal zones in steps of
 * 35 degress from -40C to 110C. Each zone corresponds to a state.
 */
#define ETHER_MAX_THERM_STATE		5UL
#endif /* THERMAL_CAL */

/**
 * @brief Timer to trigger Work queue periodically which read HW counters
 * and store locally. If data is at line rate, 2^32 entry get will filled in
 * 36 second for 1 G interface and 3.6 sec for 10 G interface.
 */
#define ETHER_STATS_TIMER		3U

#define ETHER_VM_IRQ_TX_CHAN_MASK(x)	BIT((x) * 2U)
#define ETHER_VM_IRQ_RX_CHAN_MASK(x)	BIT(((x) * 2U) + 1U)

/**
 * @brief DMA Transmit Channel NAPI
 */
struct ether_tx_napi {
	/** Transmit channel number */
	unsigned int chan;
	/** OSD private data */
	struct ether_priv_data *pdata;
	/** NAPI instance associated with transmit channel */
	struct napi_struct napi;
	/** SW timer associated with transmit channel */
	struct hrtimer tx_usecs_timer;
	/** SW timer flag associated with transmit channel */
	atomic_t tx_usecs_timer_armed;
};

/**
 *@brief DMA Receive Channel NAPI
 */
struct ether_rx_napi {
	/** Receive channel number */
	unsigned int chan;
	/** OSD private data */
	struct ether_priv_data *pdata;
	/** NAPI instance associated with transmit channel */
	struct napi_struct napi;
};

/**
 * @brief VM Based IRQ data
 */
struct ether_vm_irq_data {
	/** List of DMA Tx/Rx channel mask */
	unsigned int chan_mask;
	/** OSD private data */
	struct ether_priv_data *pdata;
};

/**
 * @brief Ethernet IVC context
 */
struct ether_ivc_ctxt {
	/** ivc cookie */
	struct tegra_hv_ivc_cookie *ivck;
	/** ivc lock */
	raw_spinlock_t ivck_lock;
	/** ivc work */
	struct work_struct ivc_work;
	/** wait for event */
	struct completion msg_complete;
	/** Flag to indicate ivc started or stopped */
	unsigned int ivc_state;
};

/**
 * @brief Ethernet driver private data
 */
struct ether_priv_data {
 	/** OSI core private data */
	struct osi_core_priv_data *osi_core;
	/** OSI DMA private data */
	struct osi_dma_priv_data *osi_dma;
	/** HW supported feature list */
	struct osi_hw_features hw_feat;
	/** Array of DMA Transmit channel NAPI */
	struct ether_tx_napi *tx_napi[OSI_EQOS_MAX_NUM_CHANS];
	/** Array of DMA Receive channel NAPI */
	struct ether_rx_napi *rx_napi[OSI_EQOS_MAX_NUM_CHANS];
	/** Network device associated with driver */
	struct net_device *ndev;
	/** Base device associated with driver */
	struct device *dev;
	/** Reset for the MAC */
	struct reset_control *mac_rst;
	/** PLLREFE clock */
	struct clk *pllrefe_clk;
	/** Clock from AXI */
	struct clk *axi_clk;
	/** Clock from AXI CBB */
	struct clk *axi_cbb_clk;
	/** Receive clock (which will be driven from the PHY) */
	struct clk *rx_clk;
	/** PTP reference clock from AXI */
	struct clk *ptp_ref_clk;
	/** Transmit clock */
	struct clk *tx_clk;
	/** Pointer to PHY device tree node */
	struct device_node *phy_node;
	/** Pointer to MDIO device tree node */
	struct device_node *mdio_node;
	/** Pointer to MII bus instance */
	struct mii_bus *mii;
	/** Pointer to the PHY device */
	struct phy_device *phydev;
	/** Interface type assciated with MAC (SGMII/RGMII/...)
	 * this information will be provided with phy-mode DT entry */
	phy_interface_t interface;
	/** Previous detected link */
	unsigned int oldlink;
	/** PHY link speed */
	int speed;
	/** Previous detected mode */
	int oldduplex;
	/** Reset for PHY */
	int phy_reset;
	/** Rx IRQ alloc mask */
	unsigned int rx_irq_alloc_mask;
	/** Tx IRQ alloc mask */
	unsigned int tx_irq_alloc_mask;
	/** Common IRQ alloc mask */
	unsigned int common_irq_alloc_mask;
	/** Common IRQ number for MAC */
	int common_irq;
	/** Array of DMA Transmit channel IRQ numbers */
	int tx_irqs[ETHER_MAX_IRQS];
	/** Array of DMA Receive channel IRQ numbers */
	int rx_irqs[ETHER_MAX_IRQS];
	/** Array of VM IRQ numbers */
	int vm_irqs[OSI_MAX_VM_IRQS];
	/** memory allocation mask */
	unsigned long long dma_mask;
	/** Current state of features enabled in HW*/
	netdev_features_t hw_feat_cur_state;
	/** MAC loopback mode */
	unsigned int mac_loopback_mode;
	/** Array of MTL queue TX priority */
	unsigned int txq_prio[OSI_EQOS_MAX_NUM_CHANS];

#ifdef THERMAL_CAL
 	/** Pointer to thermal cooling device which this driver registers
	 * with the kernel. Kernel will invoke the callback ops for this
	 * cooling device when temperate in thermal zone defined in DT
	 * binding for this driver is tripped */
	struct thermal_cooling_device *tcd;
	/** Atomic variable to hold the current temperature zone
	 * whcih has triggered */
	atomic_t therm_state;
#endif /* THERMAL_CAL */
	/** Spin lock for Tx/Rx interrupt enable registers */
	raw_spinlock_t rlock;
	/** max address register count, 2*mac_addr64_sel */
	int num_mac_addr_regs;
	/** Last address reg filter index added in last call*/
	int last_filter_index;
	/** vlan hash filter 1: hash, 0: perfect */
	unsigned int vlan_hash_filtering;
	/** L2 filter mode */
	unsigned int l2_filtering_mode;
	/** PTP clock operations structure */
	struct ptp_clock_info ptp_clock_ops;
	/** PTP system clock */
	struct ptp_clock *ptp_clock;
	/** PTP reference clock supported by platform */
	unsigned int ptp_ref_clock_speed;
	/** HW tx time stamping enable */
	unsigned int hwts_tx_en;
	/** HW rx time stamping enable */
	unsigned int hwts_rx_en;
	/** Max MTU supported by platform */
	unsigned int max_platform_mtu;
	/** Spin lock for PTP registers */
	raw_spinlock_t ptp_lock;
	/** Clocks enable check */
	bool clks_enable;
	/** Promiscuous mode support, configuration in DT */
	unsigned int promisc_mode;
	/** Delayed work queue to read RMON counters periodically */
	struct delayed_work ether_stats_work;
	/** Flag to check if EEE LPI is enabled for the MAC */
	unsigned int eee_enabled;
	/** Flag to check if EEE LPI is active currently */
	unsigned int eee_active;
	/** Flag to check if EEE LPI is enabled for MAC transmitter */
	unsigned int tx_lpi_enabled;
	/** Time (usec) MAC waits to enter LPI after Tx complete */
	unsigned int tx_lpi_timer;
	/** Flag which decides stats is enabled(1) or disabled(0) */
	unsigned int use_stats;
	/** ivc context */
	struct ether_ivc_ctxt ictxt;
	/** VM channel info data associated with VM IRQ */
	struct ether_vm_irq_data *vm_irq_data;
#ifdef CONFIG_DEBUG_FS
	/** Debug fs directory pointer */
	struct dentry *dbgfs_dir;
	/** HW features dump debug fs pointer */
	struct dentry *dbgfs_hw_feat;
	/** Descriptor dump debug fs pointer */
	struct dentry *dbgfs_desc_dump;
	/** Register dump debug fs pointer */
	struct dentry *dbgfs_reg_dump;
#endif
};

/**
 * @brief Set ethtool operations
 *
 * @param[in] ndev: network device instance
 *
 * @note Network device needs to created.
 */
void ether_set_ethtool_ops(struct net_device *ndev);
/**
 * @brief Creates Ethernet sysfs group
 *
 * @param[in] pdata: Ethernet driver private data
 *
 * @retval 0 - success,
 * @retval "negative value" - failure.
 */
int ether_sysfs_register(struct ether_priv_data *pdata);

/**
 * @brief Removes Ethernet sysfs group
 *
 * @param[in] pdata: Ethernet driver private data
 *
 * @note  nvethernet sysfs group need to be registered during probe.
 */
void ether_sysfs_unregister(struct ether_priv_data *pdata);

/**
 * @brief Function to register ptp clock driver
 *
 * Algorithm: This function is used to register the ptp clock driver to kernel
 *
 * @param[in] pdata: Pointer to private data structure.
 *
 * @note Driver probe need to be completed successfully with ethernet
 * 	 network device created
 *
 * @retval 0 on success
 * @retval "negative value" on Failure
 */
int ether_ptp_init(struct ether_priv_data *pdata);

/**
 * @brief Function to unregister ptp clock driver
 *
 * Algorithm: This function is used to remove ptp clock driver from kernel.
 *
 * @param[in] pdata: Pointer to private data structure.
 *
 * @note PTP clock driver need to be successfully registered during init
 */
void ether_ptp_remove(struct ether_priv_data *pdata);

/**
 * @brief Function to handle PTP settings.
 *
 * Algorithm: This function is used to handle the hardware PTP settings.
 *
 * @param[in] pdata Pointer to private data structure.
 * @param[in] ifr Interface request structure used for socket ioctl
 *
 * @note PTP clock driver need to be successfully registered during
 * 	 initialization and HW need to support PTP functionality.
 *
 * @retval 0 on success
 * @retval "negative value" on Failure
 */
int ether_handle_hwtstamp_ioctl(struct ether_priv_data *pdata,
				struct ifreq *ifr);
int ether_handle_priv_ts_ioctl(struct ether_priv_data *pdata,
			       struct ifreq *ifr);
int ether_conf_eee(struct ether_priv_data *pdata, unsigned int tx_lpi_enable);

#if IS_ENABLED(CONFIG_NVETHERNET_SELFTESTS)
void ether_selftest_run(struct net_device *dev,
		        struct ethtool_test *etest, u64 *buf);
void ether_selftest_get_strings(struct ether_priv_data *pdata, u8 *data);
int ether_selftest_get_count(struct ether_priv_data *pdata);
#else
static inline void ether_selftest_run(struct net_device *dev,
				      struct ethtool_test *etest, u64 *buf)
{
}
static inline void ether_selftest_get_strings(struct ether_priv_data *pdata,
					      u8 *data)
{
}
static inline int ether_selftest_get_count(struct ether_priv_data *pdata)
{
	return -EOPNOTSUPP;
}
#endif /* CONFIG_NVETHERNET_SELFTESTS */

/**
 * @brief ether_assign_osd_ops - Assigns OSD ops for OSI
 *
 * @param[in] osi_core: OSI CORE data structure
 * @param[in] osi_dma: OSI DMA data structure.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 */
void ether_assign_osd_ops(struct osi_core_priv_data *osi_core,
			  struct osi_dma_priv_data *osi_dma);

/**
 * @brief osd_send_cmd - OSD ivc send cmd
 *
 * @param[in] priv: OSD private data
 * @param[in] func: data
 * @param[in] len: length of the data
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: Yes
 */
int osd_ivc_send_cmd(void *priv, void *data, unsigned int len);
#endif /* ETHER_LINUX_H */
