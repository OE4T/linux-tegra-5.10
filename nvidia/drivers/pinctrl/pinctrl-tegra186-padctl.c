/*
 * Copyright (c) 2015-2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/mailbox_client.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/phy/phy.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/workqueue.h>
#include <linux/version.h>
#if KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE
#include <soc/tegra/fuse.h>
#include <soc/tegra/chip-id.h>
#else
#include <soc/tegra/fuse.h>
#endif
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/usb.h>
#include <soc/tegra/xusb.h>
#include <linux/tegra_prod.h>
#include <dt-bindings/pinctrl/pinctrl-tegra-padctl.h>
#include <linux/version.h>

#undef VERBOSE_DEBUG
#ifdef TRACE
#undef TRACE
#endif
#ifdef VERBOSE_DEBUG
#define TRACE(dev, fmt, args...)					\
	dev_dbg(dev, "%s(%d) " fmt "\n", __func__, __LINE__, ## args)
#else
#define TRACE(dev, fmt, args...)					\
	do {								\
		if (0)							\
			dev_dbg(dev, "%s(%d) " fmt "\n",		\
				__func__, __LINE__, ## args);		\
	} while (0)
#endif

#include "core.h"
#include "pinctrl-utils.h"

#define TEGRA_USB3_PHYS		(3)
#define TEGRA_UTMI_PHYS		(3)
#define TEGRA_HSIC_PHYS		(1)
#define TEGRA_CDP_PHYS		(3)

/* FUSE USB_CALIB registers */
/* FUSE_USB_CALIB_0 */
#define HS_CURR_LEVEL_PADX_SHIFT(x)		((x) ? (11 + (x - 1) * 6) : 0)
#define HS_CURR_LEVEL_PAD_MASK			(0x3f)
/* TODO: HS_TERM_RANGE_ADJ has bits overlap, check with hardware team */
#define HS_TERM_RANGE_ADJ_SHIFT			(7)
#define HS_TERM_RANGE_ADJ_MASK			(0xf)
#define HS_SQUELCH_SHIFT			(29)
#define HS_SQUELCH_MASK				(0x7)

/* FUSE_USB_CALIB_EXT_0 */
#define RPD_CTRL_SHIFT				(0)
#define RPD_CTRL_MASK				(0x1f)

/* Data contact detection timeout */
#define TDCD_TIMEOUT_MS				400

/* XUSB PADCTL registers */
#define XUSB_PADCTL_USB2_PAD_MUX		(0x4)
#define   PORT_HSIC				(0)
#define   PORT_XUSB				(1)

#define XUSB_PADCTL_USB2_PORT_CAP		(0x8)
#define XUSB_PADCTL_SS_PORT_CAP			(0xc)
#define   PORTX_CAP_SHIFT(x)			((x) * 4)
#define   PORT_CAP_MASK				(0x3)
#define     PORT_CAP_DISABLED			(0x0)
#define     PORT_CAP_HOST			(0x1)
#define     PORT_CAP_DEVICE			(0x2)
#define     PORT_CAP_OTG			(0x3)
#define   PORT_REVERSE_ID(x)			(1 << ((x) * 4 + 3))

#define XUSB_PADCTL_USB2_OC_MAP			(0x10)
#define XUSB_PADCTL_SS_OC_MAP			(0x14)
#define	  PORTX_OC_PIN_SHIFT(x)			((x) * 4)
#define	  PORT_OC_PIN_MASK			(0xf)
#define	    OC_PIN_DETECTION_DISABLED		(0xf)
#define	    OC_PIN_DETECTED(x)			(x)
#define	    OC_PIN_DETECTED_VBUS_PAD(x)		((x) + 4)

#define XUSB_PADCTL_VBUS_OC_MAP			(0x18)
#define	  VBUS_OC_MAP_SHIFT(x)			((x) * 5 + 1)
#define	  VBUS_OC_MAP_MASK			(0xf)
#define	    VBUS_OC_DETECTION_DISABLED		(0xf)
#define	    VBUS_OC_DETECTED(x)			(x)
#define	    VBUS_OC_DETECTED_VBUS_PAD(x)	((x) + 4)
#define	  VBUS_ENABLE(x)			(1 << (x) * 5)

#define	XUSB_PADCTL_OC_DET			(0x1c)
#define	  SET_OC_DETECTED(x)			(1 << (x))
#define	  OC_DETECTED(x)			(1 << (8 + (x)))
#define	  OC_DETECTED_VBUS_PAD(x)		(1 << (12 + (x)))
#define	  OC_DETECTED_VBUS_PAD_MASK		(0xf << 12)
#define	  OC_DETECTED_INT_EN			(1 << (20 + (x)))
#define	  OC_DETECTED_INT_EN_VBUS_PAD(x)	(1 << (24 + (x)))

#define XUSB_PADCTL_ELPG_PROGRAM		(0x20)
#define   USB2_PORT_WAKE_INTERRUPT_ENABLE(x)	(1 << (x))
#define   USB2_PORT_WAKEUP_EVENT(x)		(1 << ((x) + 7))
#define   SS_PORT_WAKE_INTERRUPT_ENABLE(x)	(1 << ((x) + 14))
#define   SS_PORT_WAKEUP_EVENT(x)		(1 << ((x) + 21))
#define   USB2_HSIC_PORT_WAKE_INTERRUPT_ENABLE(x)	(1 << ((x) + 28))
#define   USB2_HSIC_PORT_WAKEUP_EVENT(x)	(1 << ((x) + 30))
#define   ALL_WAKE_EVENTS						\
	(USB2_PORT_WAKEUP_EVENT(0) | USB2_PORT_WAKEUP_EVENT(1) |	\
	USB2_PORT_WAKEUP_EVENT(2) | SS_PORT_WAKEUP_EVENT(0) |		\
	SS_PORT_WAKEUP_EVENT(1) | SS_PORT_WAKEUP_EVENT(2) |		\
	USB2_HSIC_PORT_WAKEUP_EVENT(0))

#define XUSB_PADCTL_ELPG_PROGRAM_1		(0x24)
#define   SSPX_ELPG_CLAMP_EN(x)			(1 << (0 + (x) * 3))
#define   SSPX_ELPG_CLAMP_EN_EARLY(x)		(1 << (1 + (x) * 3))
#define   SSPX_ELPG_VCORE_DOWN(x)		(1 << (2 + (x) * 3))

#define USB2_BATTERY_CHRG_OTGPADX_CTL0(x)	(0x80 + (x) * 0x40)
#define   PD_CHG				(1 << 0)
#define   VDCD_DET_FILTER_EN			(1 << 4)
#define   VDAT_DET				(1 << 5)
#define   VDAT_DET_FILTER_EN			(1 << 8)
#define   OP_SINK_EN				(1 << 9)
#define   OP_SRC_EN				(1 << 10)
#define   ON_SINK_EN				(1 << 11)
#define   ON_SRC_EN				(1 << 12)
#define   OP_I_SRC_EN				(1 << 13)
#define   ZIP_FILTER_EN				(1 << 21)
#define   ZIN_FILTER_EN				(1 << 25)
#define   DCD_DETECTED				(1 << 26)
#define   SRP_DETECT_EN				(1 << 28)
#define   SRP_DETECTED				(1 << 29)
#define   SRP_INTR_EN				(1 << 30)
#define   GENERATE_SRP				(1 << 31)

#define USB2_BATTERY_CHRG_OTGPADX_CTL1(x)	(0x84 + (x) * 0x40)
#define   DIV_DET_EN				(1 << 4)
#define   PD_VREG				(1 << 6)
#define   VREG_LEV(x)				(((x) & 0x3) << 7)
#define   VREG_DIR(x)				(((x) & 0x3) << 11)
#define   VREG_DIR_IN				VREG_DIR(1)
#define   VREG_DIR_OUT				VREG_DIR(2)
#define   USBOP_RPD_OVRD			(1 << 16)
#define   USBOP_RPD_OVRD_VAL			(1 << 17)
#define   USBOP_RPU_OVRD			(1 << 18)
#define   USBOP_RPU_OVRD_VAL			(1 << 19)
#define   USBON_RPD_OVRD			(1 << 20)
#define   USBON_RPD_OVRD_VAL			(1 << 21)
#define   USBON_RPU_OVRD			(1 << 22)
#define   USBON_RPU_OVRD_VAL			(1 << 23)

#define XUSB_PADCTL_USB2_OTG_PADX_CTL0(x)	(0x88 + (x) * 0x40)
#define   HS_CURR_LEVEL(x)			((x) & 0x3f)
#define   TERM_SEL				(1 << 25)
#define   USB2_OTG_PD				(1 << 26)
#define   USB2_OTG_PD2				(1 << 27)
#define   USB2_OTG_PD2_OVRD_EN			(1 << 28)
#define   USB2_OTG_PD_ZI			(1 << 29)

#define XUSB_PADCTL_USB2_OTG_PADX_CTL1(x)	(0x8c + (x) * 0x40)
#define   USB2_OTG_PD_DR			(1 << 2)
#define   TERM_RANGE_ADJ(x)			(((x) & 0xf) << 3)
#define   RPD_CTRL(x)				(((x) & 0x1f) << 26)

#define XUSB_PADCTL_USB2_BATTERY_CHRG_TDCD_DBNC_TIMER_0	(0x280)
#define   TDCD_DBNC(x)				(((x) & 0x7ff) << 0)

#define XUSB_PADCTL_USB2_BIAS_PAD_CTL0		(0x284)
#define   BIAS_PAD_PD				(1 << 11)
#define   HS_SQUELCH_LEVEL(x)			(((x) & 0x7) << 0)

#define XUSB_PADCTL_USB2_BIAS_PAD_CTL1		(0x288)
#define   USB2_TRK_START_TIMER(x)		(((x) & 0x7f) << 12)
#define   USB2_TRK_DONE_RESET_TIMER(x)		(((x) & 0x7f) << 19)
#define   USB2_PD_TRK				(1 << 26)

#define XUSB_PADCTL_HSIC_PADX_CTL0(x)		(0x300 + (x) * 0x20)
#define   HSIC_PD_TX_DATA0			(1 << 1)
#define   HSIC_PD_TX_STROBE			(1 << 3)
#define   HSIC_PD_RX_DATA0			(1 << 4)
#define   HSIC_PD_RX_STROBE			(1 << 6)
#define   HSIC_PD_ZI_DATA0			(1 << 7)
#define   HSIC_PD_ZI_STROBE			(1 << 9)
#define   HSIC_RPD_DATA0			(1 << 13)
#define   HSIC_RPD_STROBE			(1 << 15)
#define   HSIC_RPU_DATA0			(1 << 16)
#define   HSIC_RPU_STROBE			(1 << 18)

#define XUSB_PADCTL_HSIC_PAD_TRK_CTL0		(0x340)
#define   HSIC_TRK_START_TIMER(x)		(((x) & 0x7f) << 5)
#define   HSIC_TRK_DONE_RESET_TIMER(x)		(((x) & 0x7f) << 12)
#define   HSIC_PD_TRK				(1 << 19)

#define USB2_VBUS_ID				(0x360)
#define   OTG_VBUS_SESS_VLD			(1 << 0)
#define   OTG_VBUS_SESS_VLD_ST_CHNG		(1 << 1)
#define   OTG_VBUS_SESS_VLD_CHNG_INTR_EN	(1 << 2)
#define   VBUS_VALID				(1 << 3)
#define   VBUS_VALID_ST_CHNG			(1 << 4)
#define   VBUS_VALID_CHNG_INTR_EN		(1 << 5)
#define   IDDIG					(1 << 6)
#define   IDDIG_A				(1 << 7)
#define   IDDIG_B				(1 << 8)
#define   IDDIG_C				(1 << 9)
#define   RID_MASK				(0xf << 6)
#define   IDDIG_ST_CHNG				(1 << 10)
#define   IDDIG_CHNG_INTR_EN			(1 << 11)
#define   VBUS_OVERRIDE				(1 << 14)
#define   ID_OVERRIDE_SHIFT			18
#define   ID_OVERRIDE_MASK			0xf
#define   ID_OVERRIDE(x)			(((x) & 0xf) << 18)
#define   ID_OVERRIDE_FLOATING			ID_OVERRIDE(8)
#define   ID_OVERRIDE_GROUNDED			ID_OVERRIDE(0)
#define   VBUS_WAKEUP				(1 << 22)
#define   VBUS_WAKEUP_ST_CHNG			(1 << 23)
#define   VBUS_WAKEUP_CHNG_INTR_EN		(1 << 24)

/* XUSB AO registers */
#define XUSB_AO_USB_DEBOUNCE_DEL		(0x4)
#define   UHSIC_LINE_DEB_CNT(x)			(((x) & 0xf) << 4)
#define   UTMIP_LINE_DEB_CNT(x)			((x) & 0xf)

#define XUSB_AO_UTMIP_TRIGGERS(x)		(0x40 + (x) * 4)
#define   CLR_WALK_PTR				(1 << 0)
#define   CAP_CFG				(1 << 1)
#define   CLR_WAKE_ALARM			(1 << 3)

#define XUSB_AO_UHSIC_TRIGGERS(x)		(0x60 + (x) * 4)
#define   HSIC_CLR_WALK_PTR			(1 << 0)
#define   HSIC_CLR_WAKE_ALARM			(1 << 3)
#define   HSIC_CAP_CFG				(1 << 4)

#define XUSB_AO_UTMIP_SAVED_STATE(x)		(0x70 + (x) * 4)
#define   SPEED(x)				((x) & 0x3)
#define     UTMI_HS				SPEED(0)
#define     UTMI_FS				SPEED(1)
#define     UTMI_LS				SPEED(2)
#define     UTMI_RST				SPEED(3)

#define XUSB_AO_UHSIC_SAVED_STATE(x)		(0x90 + (x) * 4)
#define   MODE(x)				((x) & 0x1)
#define   MODE_HS				MODE(1)
#define   MODE_RST				MODE(0)

#define XUSB_AO_UTMIP_SLEEPWALK_CFG(x)		(0xd0 + (x) * 4)
#define XUSB_AO_UHSIC_SLEEPWALK_CFG(x)		(0xf0 + (x) * 4)
#define   FAKE_USBOP_VAL			(1 << 0)
#define   FAKE_USBON_VAL			(1 << 1)
#define   FAKE_USBOP_EN				(1 << 2)
#define   FAKE_USBON_EN				(1 << 3)
#define   FAKE_STROBE_VAL			(1 << 0)
#define   FAKE_DATA_VAL				(1 << 1)
#define   FAKE_STROBE_EN			(1 << 2)
#define   FAKE_DATA_EN				(1 << 3)
#define   WAKE_WALK_EN				(1 << 14)
#define   MASTER_ENABLE				(1 << 15)
#define   LINEVAL_WALK_EN			(1 << 16)
#define   WAKE_VAL(x)				(((x) & 0xf) << 17)
#define     WAKE_VAL_NONE			WAKE_VAL(12)
#define     WAKE_VAL_ANY			WAKE_VAL(15)
#define     WAKE_VAL_DS10			WAKE_VAL(2)
#define   LINE_WAKEUP_EN			(1 << 21)
#define   MASTER_CFG_SEL			(1 << 22)

#define XUSB_AO_UTMIP_SLEEPWALK(x)		(0x100 + (x) * 4)
/* phase A */
#define   USBOP_RPD_A				(1 << 0)
#define   USBON_RPD_A				(1 << 1)
#define   AP_A					(1 << 4)
#define   AN_A					(1 << 5)
#define   HIGHZ_A				(1 << 6)
/* phase B */
#define   USBOP_RPD_B				(1 << 8)
#define   USBON_RPD_B				(1 << 9)
#define   AP_B					(1 << 12)
#define   AN_B					(1 << 13)
#define   HIGHZ_B				(1 << 14)
/* phase C */
#define   USBOP_RPD_C				(1 << 16)
#define   USBON_RPD_C				(1 << 17)
#define   AP_C					(1 << 20)
#define   AN_C					(1 << 21)
#define   HIGHZ_C				(1 << 22)
/* phase D */
#define   USBOP_RPD_D				(1 << 24)
#define   USBON_RPD_D				(1 << 25)
#define   AP_D					(1 << 28)
#define   AN_D					(1 << 29)
#define   HIGHZ_D				(1 << 30)

#define XUSB_AO_UHSIC_SLEEPWALK(x)		(0x120 + (x) * 4)
/* phase A */
#define   RPD_STROBE_A				(1 << 0)
#define   RPD_DATA0_A				(1 << 1)
#define   RPU_STROBE_A				(1 << 2)
#define   RPU_DATA0_A				(1 << 3)
/* phase B */
#define   RPD_STROBE_B				(1 << 8)
#define   RPD_DATA0_B				(1 << 9)
#define   RPU_STROBE_B				(1 << 10)
#define   RPU_DATA0_B				(1 << 11)
/* phase C */
#define   RPD_STROBE_C				(1 << 16)
#define   RPD_DATA0_C				(1 << 17)
#define   RPU_STROBE_C				(1 << 18)
#define   RPU_DATA0_C				(1 << 19)
/* phase D */
#define   RPD_STROBE_D				(1 << 24)
#define   RPD_DATA0_D				(1 << 25)
#define   RPU_STROBE_D				(1 << 26)
#define   RPU_DATA0_D				(1 << 27)

#define XUSB_AO_UTMIP_PAD_CFG(x)		(0x130 + (x) * 4)
#define   FSLS_USE_XUSB_AO			(1 << 3)
#define   TRK_CTRL_USE_XUSB_AO			(1 << 4)
#define   RPD_CTRL_USE_XUSB_AO			(1 << 5)
#define   RPU_USE_XUSB_AO			(1 << 6)
#define   VREG_USE_XUSB_AO			(1 << 7)
#define   USBOP_VAL_PD				(1 << 8)
#define   USBON_VAL_PD				(1 << 9)
#define   E_DPD_OVRD_EN				(1 << 10)
#define   E_DPD_OVRD_VAL			(1 << 11)

#define XUSB_AO_UHSIC_PAD_CFG(x)		(0x150 + (x) * 4)
#define   STROBE_VAL_PD				(1 << 0)
#define   DATA0_VAL_PD				(1 << 1)
#define   USE_XUSB_AO				(1 << 4)

enum tegra186_function {
	TEGRA186_FUNC_HSIC = 0,
	TEGRA186_FUNC_XUSB,
};

struct tegra_padctl_function {
	const char *name;
	const char * const *groups;
	unsigned int num_groups;
};

struct tegra_padctl_group {
	const unsigned int *funcs;
	unsigned int num_funcs;
};

struct tegra_padctl_soc {
	const struct pinctrl_pin_desc *pins;
	unsigned int num_pins;

	const struct tegra_padctl_function *functions;
	unsigned int num_functions;

	const struct tegra_padctl_pad *pads;
	unsigned int num_pads;

	unsigned int hsic_port_offset;

	const char * const *supply_names;
	unsigned int num_supplies;

	unsigned int num_oc_pins;
};

struct tegra_padctl_pad {
	const char *name;

	unsigned int offset;
	unsigned int shift;
	unsigned int mask;
	unsigned int iddq;

	const unsigned int *funcs;
	unsigned int num_funcs;
};

struct tegra_xusb_fuse_calibration {
	u32 hs_curr_level[TEGRA_UTMI_PHYS];
	u32 hs_squelch;
	u32 hs_term_range_adj;
	u32 rpd_ctrl;
};

enum xusb_port_cap {
	CAP_DISABLED = TEGRA_PADCTL_PORT_DISABLED,
	HOST_ONLY = TEGRA_PADCTL_PORT_HOST_ONLY,
	DEVICE_ONLY = TEGRA_PADCTL_PORT_DEVICE_ONLY,
	OTG = TEGRA_PADCTL_PORT_OTG_CAP,
};

struct tegra_xusb_usb3_port {
	enum xusb_port_cap port_cap;
	int oc_pin;
};

struct tegra_xusb_utmi_port {
	enum xusb_port_cap port_cap;
	int hs_curr_level_offset; /* deal with platform design deviation */
	bool poweron;
	int oc_pin;
};

struct tegra_xusb_hsic_port {
	bool pretend_connected;
};

struct padctl_context {
	u32 vbus_id;
	u32 usb2_pad_mux;
	u32 usb2_port_cap;
	u32 ss_port_cap;
};

struct tegra_padctl {
	struct device *dev;
	void __iomem *padctl_regs;
	void __iomem *ao_regs;

	struct reset_control *padctl_rst;

	struct clk *xusb_clk; /* xusb main clock */
	struct clk *utmipll; /* utmi pads */
	struct clk *usb2_trk_clk; /* utmi tracking circuit clock */
	struct clk *hsic_trk_clk; /* hsic tracking circuit clock */

	struct mutex lock;

	const struct tegra_padctl_soc *soc;
	struct tegra_xusb_fuse_calibration calib;
	struct tegra_prod *prod_list;
	struct pinctrl_dev *pinctrl;
	struct pinctrl_desc desc;

	struct phy_provider *provider;
	struct phy *usb3_phys[TEGRA_USB3_PHYS];
	struct phy *utmi_phys[TEGRA_UTMI_PHYS];
	struct phy *hsic_phys[TEGRA_HSIC_PHYS];
	struct phy *cdp_phys[TEGRA_CDP_PHYS];
	struct tegra_xusb_hsic_port hsic_ports[TEGRA_HSIC_PHYS];
	struct tegra_xusb_utmi_port utmi_ports[TEGRA_UTMI_PHYS];
	int utmi_otg_port_base_1; /* one based utmi port number */
	struct tegra_xusb_usb3_port usb3_ports[TEGRA_USB3_PHYS];
	int usb3_otg_port_base_1; /* one based usb3 port number */

	struct work_struct mbox_req_work;
	struct tegra_xusb_mbox_msg mbox_req;
	struct mbox_client mbox_client;
	struct mbox_chan *mbox_chan;

	bool host_mode_phy_disabled; /* set true if mailbox is not available */
	unsigned int bias_pad_enable;
	/* TODO: should move to host controller driver? */
	struct regulator *vbus[TEGRA_UTMI_PHYS];
	struct regulator *vddio_hsic;

	bool otg_vbus_alwayson;

	struct regulator_bulk_data *supplies;
	struct padctl_context padctl_context;

	bool cdp_used;

	struct pinctrl *oc_pinctrl;
	/*
	 * array of pinctrl_state (of number num_oc_pins)
	 * for different OC states
	 */
	struct pinctrl_state **oc_tristate_enable;
	struct pinctrl_state **oc_passthrough_enable;
	struct pinctrl_state **oc_disable;
};

#ifdef VERBOSE_DEBUG
#define ao_writel(_padctl, _value, _offset)				\
{									\
	unsigned long v = _value, o = _offset;				\
	pr_debug("%s ao_writel %s(@0x%lx) with 0x%lx\n", __func__,	\
		#_offset, o, v);					\
	writel(v, _padctl->ao_regs + o);				\
}

#define ao_readl(_padctl, _offset)					\
({									\
	unsigned long v, o = _offset;					\
	v = readl(_padctl->ao_regs + o);				\
	pr_debug("%s ao_readl %s(@0x%lx) = 0x%lx\n", __func__,		\
		#_offset, o, v);					\
	v;								\
})
#else
static inline void ao_writel(struct tegra_padctl *padctl, u32 value,
				 unsigned long offset)
{
	writel(value, padctl->ao_regs + offset);
}

static inline u32 ao_readl(struct tegra_padctl *padctl,
			       unsigned long offset)
{
	return readl(padctl->ao_regs + offset);
}
#endif

#ifdef VERBOSE_DEBUG
#define padctl_writel(_padctl, _value, _offset)				\
{									\
	unsigned long v = _value, o = _offset;				\
	pr_debug("%s padctl_write %s(@0x%lx) with 0x%lx\n", __func__,	\
		#_offset, o, v);					\
	writel(v, _padctl->padctl_regs + o);				\
}

#define padctl_readl(_padctl, _offset)					\
({									\
	unsigned long v, o = _offset;					\
	v = readl(_padctl->padctl_regs + o);				\
	pr_debug("%s padctl_read %s(@0x%lx) = 0x%lx\n", __func__,	\
		#_offset, o, v);					\
	v;								\
})
#else
static inline void padctl_writel(struct tegra_padctl *padctl, u32 value,
				 unsigned long offset)
{
	writel(value, padctl->padctl_regs + offset);
}

static inline u32 padctl_readl(struct tegra_padctl *padctl,
			       unsigned long offset)
{
	return readl(padctl->padctl_regs + offset);
}
#endif

static int tegra186_padctl_regulators_init(struct tegra_padctl *padctl)
{
	struct device *dev = padctl->dev;
	size_t size;
	int err;
	int i;

	size = padctl->soc->num_supplies * sizeof(struct regulator_bulk_data);
	padctl->supplies = devm_kzalloc(dev, size, GFP_ATOMIC);
	if (!padctl->supplies) {
		dev_err(dev, "failed to alloc memory for regulators\n");
		return -ENOMEM;
	}

	for (i = 0; i < padctl->soc->num_supplies; i++)
		padctl->supplies[i].supply = padctl->soc->supply_names[i];

	err = devm_regulator_bulk_get(dev, padctl->soc->num_supplies,
					padctl->supplies);
	if (err) {
		dev_err(dev, "failed to request regulators %d\n", err);
		return err;
	}

	return 0;
}

static inline
struct tegra_padctl *mbox_work_to_padctl(struct work_struct *work)
{
	return container_of(work, struct tegra_padctl, mbox_req_work);
}

#define PIN_OTG_0	0
#define PIN_OTG_1	1
#define PIN_OTG_2	2
#define PIN_HSIC_0	3
#define PIN_USB3_0	4
#define PIN_USB3_1	5
#define PIN_USB3_2	6
#define PIN_CDP_0	7
#define PIN_CDP_1	8
#define PIN_CDP_2	9

static inline bool pad_is_otg(unsigned int pad)
{
	return pad >= PIN_OTG_0 && pad <= PIN_OTG_2;
}

static inline bool pad_is_hsic(unsigned int pad)
{
	return pad == PIN_HSIC_0;
}

static inline bool pad_is_usb3(unsigned int pad)
{
	return pad >= PIN_USB3_0 && pad <= PIN_USB3_2;
}

static inline bool pad_is_cdp(unsigned int pad)
{
	return pad >= PIN_CDP_0 && pad <= PIN_CDP_2;
}

static int tegra_padctl_get_groups_count(struct pinctrl_dev *pinctrl)
{
	struct tegra_padctl *padctl = pinctrl_dev_get_drvdata(pinctrl);

	TRACE(padctl->dev, "num_pins %u", padctl->soc->num_pins);
	return padctl->soc->num_pins;
}

static const char *tegra_padctl_get_group_name(struct pinctrl_dev *pinctrl,
						    unsigned int group)
{
	struct tegra_padctl *padctl = pinctrl_dev_get_drvdata(pinctrl);
	struct device *dev = padctl->dev;

	TRACE(dev, "group %u name %s", group, padctl->soc->pins[group].name);
	return padctl->soc->pins[group].name;
}

static int tegra_padctl_get_group_pins(struct pinctrl_dev *pinctrl,
				       unsigned group,
				       const unsigned **pins,
				       unsigned *num_pins)
{
	struct tegra_padctl *padctl = pinctrl_dev_get_drvdata(pinctrl);
	struct device *d = padctl->dev;

	*pins = &padctl->soc->pins[group].number;
	*num_pins = 1; /* one pin per group */

	TRACE(d, "group %u num_pins %u pins[0] %u", group, *num_pins, *pins[0]);

	return 0;
}

enum tegra_xusb_padctl_param {
	TEGRA_PADCTL_PORT_CAP,
	TEGRA_PADCTL_HSIC_PRETEND_CONNECTED,
	TEGRA_PADCTL_UTMI_HS_CURR_LEVEL_OFFSET,
	TEGRA_PADCTL_OC_PIN,
};

static const struct tegra_padctl_property {
	const char *name;
	enum tegra_xusb_padctl_param param;
} properties[] = {
	{"nvidia,port-cap", TEGRA_PADCTL_PORT_CAP},
	{"nvidia,pretend-connected", TEGRA_PADCTL_HSIC_PRETEND_CONNECTED},
	{"nvidia,hs_curr_level_offset", TEGRA_PADCTL_UTMI_HS_CURR_LEVEL_OFFSET},
	{"nvidia,oc-pin", TEGRA_PADCTL_OC_PIN},
};

#define TEGRA_XUSB_PADCTL_PACK(param, value) ((param) << 16 | (value & 0xffff))
#define TEGRA_XUSB_PADCTL_UNPACK_PARAM(config) ((config) >> 16)
#define TEGRA_XUSB_PADCTL_UNPACK_VALUE(config) ((config) & 0xffff)

static int tegra186_padctl_parse_subnode(struct tegra_padctl *padctl,
					   struct device_node *np,
					   struct pinctrl_map **maps,
					   unsigned int *reserved_maps,
					   unsigned int *num_maps)
{
	unsigned int i, reserve = 0, num_configs = 0;
	unsigned long config, *configs = NULL;
	const char *function, *group;
	struct property *prop;
	int err = 0;
	u32 value;

	err = of_property_read_string(np, "nvidia,function", &function);
	if (err < 0) {
		if (err != -EINVAL)
			goto out;

		function = NULL;
	}

	for (i = 0; i < ARRAY_SIZE(properties); i++) {
		err = of_property_read_u32(np, properties[i].name, &value);
		if (err < 0) {
			if (err == -EINVAL)
				continue;

			goto out;
		}

		config = TEGRA_XUSB_PADCTL_PACK(properties[i].param, value);

		err = pinctrl_utils_add_config(padctl->pinctrl, &configs,
					       &num_configs, config);
		if (err < 0)
			goto out;
	}

	if (function)
		reserve++;

	if (num_configs)
		reserve++;

	err = of_property_count_strings(np, "nvidia,lanes");
	if (err < 0)
		goto out;

	reserve *= err;

	err = pinctrl_utils_reserve_map(padctl->pinctrl, maps, reserved_maps,
					num_maps, reserve);
	if (err < 0)
		goto out;

	of_property_for_each_string(np, "nvidia,lanes", prop, group) {
		if (function) {
			err = pinctrl_utils_add_map_mux(padctl->pinctrl, maps,
					reserved_maps, num_maps, group,
					function);
			if (err < 0)
				goto out;
		}

		if (num_configs) {
			err = pinctrl_utils_add_map_configs(padctl->pinctrl,
					maps, reserved_maps, num_maps, group,
					configs, num_configs,
					PIN_MAP_TYPE_CONFIGS_GROUP);
			if (err < 0)
				goto out;
		}
	}

	err = 0;

out:
	kfree(configs);
	return err;
}

static int tegra_padctl_dt_node_to_map(struct pinctrl_dev *pinctrl,
				       struct device_node *parent,
				       struct pinctrl_map **maps,
				       unsigned int *num_maps)
{
	struct tegra_padctl *padctl = pinctrl_dev_get_drvdata(pinctrl);
	unsigned int reserved_maps = 0;
	struct device_node *np;
	int err;

	*num_maps = 0;
	*maps = NULL;

	for_each_child_of_node(parent, np) {
		/* If node status is disabled then ignore the node */
		if (!of_device_is_available(np))
			continue;

		err = tegra186_padctl_parse_subnode(padctl, np, maps,
						    &reserved_maps,
						    num_maps);
		if (err < 0) {
			pr_info("%s %d err %d\n", __func__, __LINE__, err);
			return err;
		}
	}

	return 0;
}

static const struct pinctrl_ops tegra_xusb_padctl_pinctrl_ops = {
	.get_groups_count = tegra_padctl_get_groups_count,
	.get_group_name = tegra_padctl_get_group_name,
	.get_group_pins = tegra_padctl_get_group_pins,
	.dt_node_to_map = tegra_padctl_dt_node_to_map,
/* API changed after 4.6.0-rc1 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 6, 0)
	.dt_free_map = pinctrl_utils_dt_free_map,
#else
	.dt_free_map = pinctrl_utils_free_map,
#endif
};

static int tegra186_padctl_get_functions_count(struct pinctrl_dev *pinctrl)
{
	struct tegra_padctl *padctl = pinctrl_dev_get_drvdata(pinctrl);

	TRACE(padctl->dev, "num_functions %u", padctl->soc->num_functions);
	return padctl->soc->num_functions;
}

static const char *
tegra186_padctl_get_function_name(struct pinctrl_dev *pinctrl,
				    unsigned int function)
{
	struct tegra_padctl *padctl = pinctrl_dev_get_drvdata(pinctrl);

	TRACE(padctl->dev, "function %u name %s", function,
					padctl->soc->functions[function].name);

	return padctl->soc->functions[function].name;
}

static int tegra186_padctl_get_function_groups(struct pinctrl_dev *pinctrl,
					       unsigned int function,
					       const char * const **groups,
					       unsigned * const num_groups)
{
	struct tegra_padctl *padctl = pinctrl_dev_get_drvdata(pinctrl);

	*num_groups = padctl->soc->functions[function].num_groups;
	*groups = padctl->soc->functions[function].groups;

	TRACE(padctl->dev, "function %u *num_groups %u groups %s",
				function, *num_groups, *groups[0]);
	return 0;
}

static int tegra186_padctl_pinmux_set(struct pinctrl_dev *pinctrl,
				      unsigned int function,
				      unsigned int group)
{
	struct tegra_padctl *padctl = pinctrl_dev_get_drvdata(pinctrl);
	const struct tegra_padctl_pad *pad;
	unsigned int i;
	u32 value;

	pad = &padctl->soc->pads[group];

	TRACE(padctl->dev, "group %u (%s) function %u num_funcs %d",
			group, pad->name, function, pad->num_funcs);

	for (i = 0; i < pad->num_funcs; i++) {
		TRACE(padctl->dev, "lane->funcs[%d] %d\n", i, pad->funcs[i]);
		if (pad->funcs[i] == function)
			break;
	}

	if (i >= pad->num_funcs)
		return -EINVAL;

	TRACE(padctl->dev, "group %s set to function %s",
			pad->name, padctl->soc->functions[function].name);

	if (pad_is_otg(group)) {
		value = padctl_readl(padctl, pad->offset);
		value &= ~(pad->mask << pad->shift);
		value |= (PORT_XUSB << pad->shift);
		padctl_writel(padctl, value, pad->offset);
	} else if (pad_is_hsic(group)) {
		value = padctl_readl(padctl, pad->offset);
		value &= ~(pad->mask << pad->shift);
		value |= (PORT_HSIC << pad->shift);
		padctl_writel(padctl, value, pad->offset);
	} else if (pad_is_cdp(group)) {
		if (function != TEGRA186_FUNC_XUSB)
			dev_warn(padctl->dev, "group %s isn't for xusb!",
				 pad->name);
	} else
		return -EINVAL;

	return 0;
}

static const struct pinmux_ops tegra186_padctl_pinmux_ops = {
	.get_functions_count = tegra186_padctl_get_functions_count,
	.get_function_name = tegra186_padctl_get_function_name,
	.get_function_groups = tegra186_padctl_get_function_groups,
	.set_mux = tegra186_padctl_pinmux_set,
};

static int tegra_padctl_pinconf_group_get(struct pinctrl_dev *pinctrl,
					  unsigned int group,
					  unsigned long *config)
{
	struct tegra_padctl *padctl = pinctrl_dev_get_drvdata(pinctrl);
	enum tegra_xusb_padctl_param param;
	int value = 0;

	param = TEGRA_XUSB_PADCTL_UNPACK_PARAM(*config);

	TRACE(padctl->dev, "group %u param 0x%x\n", group, param);

	switch (param) {

	default:
		dev_err(padctl->dev, "invalid configuration parameter: %04x\n",
			param);
		return -ENOTSUPP;
	}

	*config = TEGRA_XUSB_PADCTL_PACK(param, value);
	return 0;
}

static int tegra_padctl_pinconf_group_set(struct pinctrl_dev *pinctrl,
					       unsigned int group,
					       unsigned long *configs,
					       unsigned num_configs)
{
	struct tegra_padctl *padctl = pinctrl_dev_get_drvdata(pinctrl);
	struct device *dev = padctl->dev;
	enum tegra_xusb_padctl_param param;
	unsigned long value;
	int port;
	s16 offset;
	int i;

	for (i = 0; i < num_configs; i++) {
		param = TEGRA_XUSB_PADCTL_UNPACK_PARAM(configs[i]);
		value = TEGRA_XUSB_PADCTL_UNPACK_VALUE(configs[i]);

		TRACE(dev, "group %u config 0x%lx param 0x%x value 0x%lx",
			group, configs[i], param, value);

		switch (param) {
		case TEGRA_PADCTL_PORT_CAP:
			if (value > TEGRA_PADCTL_PORT_OTG_CAP) {
				dev_err(dev, "Invalid port-cap: %lu\n", value);
				return -EINVAL;
			}
			if (pad_is_usb3(group)) {
				int port = group - PIN_USB3_0;

				padctl->usb3_ports[port].port_cap = value;
				TRACE(dev, "USB3 port %d cap %lu",
				      port, value);
				if (value == OTG) {
					if (padctl->usb3_otg_port_base_1)
						dev_warn(dev, "enabling OTG on multiple USB3 ports\n");


					dev_info(dev, "using USB3 port %d for otg\n",
						 port);
					padctl->usb3_otg_port_base_1 = port + 1;
				}
			} else if (pad_is_otg(group)) {
				int port = group - PIN_OTG_0;

				padctl->utmi_ports[port].port_cap = value;
				TRACE(dev, "UTMI port %d cap %lu",
				      port, value);
				if (value == OTG) {
					if (padctl->utmi_otg_port_base_1)
						dev_warn(dev, "enabling OTG on multiple UTMI ports\n");

					dev_info(dev, "using UTMI port %d for otg\n",
						 port);

					padctl->utmi_otg_port_base_1 = port + 1;
				}
			} else {
				dev_err(dev, "port-cap not applicable for pin %d\n",
					group);
				return -EINVAL;
			}

			break;

		case TEGRA_PADCTL_HSIC_PRETEND_CONNECTED:
			if (!pad_is_hsic(group)) {
				dev_err(dev, "pretend-connected is not applicable for pin %d\n",
					group);
				return -EINVAL;
			}

			port = group - PIN_HSIC_0;
			padctl->hsic_ports[port].pretend_connected = value;
			TRACE(dev, "HSIC port %d pretend-connected %ld",
			      port, value);
			break;

		case TEGRA_PADCTL_UTMI_HS_CURR_LEVEL_OFFSET:
			if (!pad_is_otg(group)) {
				dev_err(dev, "hs_curr_level_offset is not applicable for pin %d\n",
					group);
				return -EINVAL;
			}

			port = group - PIN_OTG_0;
			offset = TEGRA_XUSB_PADCTL_UNPACK_VALUE(configs[i]);

			padctl->utmi_ports[port].hs_curr_level_offset = offset;
			TRACE(dev, "UTMI port %d hs_curr_level_offset %d",
			      port, offset);
			break;
		case TEGRA_PADCTL_OC_PIN:
			if (pad_is_usb3(group)) {
				int port = group - PIN_USB3_0;

				if (value >= padctl->soc->num_oc_pins) {
					dev_err(dev, "Invalid OC pin: %lu\n",
						value);
					return -EINVAL;
				}
				TRACE(dev, "USB3 port %d OC pin %lu",
				      port, value);
				padctl->usb3_ports[port].oc_pin = (int) value;
			} else if (pad_is_otg(group)) {
				int port = group - PIN_OTG_0;

				if (value >= padctl->soc->num_oc_pins) {
					dev_err(dev, "Invalid OC pin: %lu\n",
						value);
					return -EINVAL;
				}
				TRACE(dev, "USB2 port %d OC pin %lu",
				      port, value);

				padctl->utmi_ports[port].oc_pin = (int) value;
			}

			break;
		default:
			dev_err(dev, "invalid configuration parameter: %04x\n",
				param);
			return -ENOTSUPP;
		}
	}
	return 0;
}

#ifdef CONFIG_DEBUG_FS
static const char *strip_prefix(const char *s)
{
	const char *comma = strchr(s, ',');

	if (!comma)
		return s;

	return comma + 1;
}

static void
tegra_padctl_pinconf_group_dbg_show(struct pinctrl_dev *pinctrl,
				    struct seq_file *s,
				    unsigned int group)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(properties); i++) {
		unsigned long config, value;
		int err;

		config = TEGRA_XUSB_PADCTL_PACK(properties[i].param, 0);

		err = tegra_padctl_pinconf_group_get(pinctrl, group,
							  &config);
		if (err < 0)
			continue;

		value = TEGRA_XUSB_PADCTL_UNPACK_VALUE(config);

		seq_printf(s, "\n\t%s=%lu\n", strip_prefix(properties[i].name),
			   value);
	}
}

static void
tegra_padctl_pinconf_config_dbg_show(struct pinctrl_dev *pinctrl,
				     struct seq_file *s,
				     unsigned long config)
{
	enum tegra_xusb_padctl_param param;
	const char *name = "unknown";
	unsigned long value;
	unsigned int i;

	param = TEGRA_XUSB_PADCTL_UNPACK_PARAM(config);
	value = TEGRA_XUSB_PADCTL_UNPACK_VALUE(config);

	for (i = 0; i < ARRAY_SIZE(properties); i++) {
		if (properties[i].param == param) {
			name = properties[i].name;
			break;
		}
	}

	seq_printf(s, "%s=%lu", strip_prefix(name), value);
}
#endif

static const struct pinconf_ops tegra_padctl_pinconf_ops = {
	.pin_config_group_get = tegra_padctl_pinconf_group_get,
	.pin_config_group_set = tegra_padctl_pinconf_group_set,
#ifdef CONFIG_DEBUG_FS
	.pin_config_group_dbg_show = tegra_padctl_pinconf_group_dbg_show,
	.pin_config_config_dbg_show = tegra_padctl_pinconf_config_dbg_show,
#endif
};

static int usb3_phy_to_port(struct phy *phy)
{
	struct tegra_padctl *padctl = phy_get_drvdata(phy);
	unsigned int i;

	for (i = 0; i < TEGRA_USB3_PHYS; i++) {
		if (phy == padctl->usb3_phys[i])
			return i;
	}
	WARN_ON(1);

	return -EINVAL;
}

static int tegra186_usb3_phy_power_on(struct phy *phy)
{
	struct tegra_padctl *padctl = phy_get_drvdata(phy);
	int port = usb3_phy_to_port(phy);
	int pin;
	u32 reg;

	if (port < 0)
		return port;

	pin = padctl->usb3_ports[port].oc_pin;
	mutex_lock(&padctl->lock);

	dev_dbg(padctl->dev, "power on USB3 port %d\n", port);

	reg = padctl_readl(padctl, XUSB_PADCTL_SS_PORT_CAP);
	reg &= ~(PORT_CAP_MASK << PORTX_CAP_SHIFT(port));
	if (padctl->usb3_ports[port].port_cap == CAP_DISABLED)
		reg |= (PORT_CAP_DISABLED << PORTX_CAP_SHIFT(port));
	else if (padctl->usb3_ports[port].port_cap == DEVICE_ONLY)
		reg |= (PORT_CAP_DEVICE << PORTX_CAP_SHIFT(port));
	else if (padctl->usb3_ports[port].port_cap == HOST_ONLY)
		reg |= (PORT_CAP_HOST << PORTX_CAP_SHIFT(port));
	else if (padctl->usb3_ports[port].port_cap == OTG)
		reg |= (PORT_CAP_OTG << PORTX_CAP_SHIFT(port));
	padctl_writel(padctl, reg, XUSB_PADCTL_SS_PORT_CAP);

	/* setting SS OC map */
	if (pin >= 0) {
		reg = padctl_readl(padctl, XUSB_PADCTL_SS_OC_MAP);
		reg &= ~(PORT_OC_PIN_MASK << PORTX_OC_PIN_SHIFT(port));
		reg |= (OC_PIN_DETECTED_VBUS_PAD(pin) & PORT_OC_PIN_MASK) <<
			PORTX_OC_PIN_SHIFT(port);
		padctl_writel(padctl, reg, XUSB_PADCTL_SS_OC_MAP);
	}

	reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM_1);
	reg &= ~SSPX_ELPG_VCORE_DOWN(port);
	padctl_writel(padctl, reg, XUSB_PADCTL_ELPG_PROGRAM_1);

	usleep_range(100, 200);

	reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM_1);
	reg &= ~SSPX_ELPG_CLAMP_EN_EARLY(port);
	padctl_writel(padctl, reg, XUSB_PADCTL_ELPG_PROGRAM_1);

	usleep_range(100, 200);

	reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM_1);
	reg &= ~SSPX_ELPG_CLAMP_EN(port);
	padctl_writel(padctl, reg, XUSB_PADCTL_ELPG_PROGRAM_1);

	mutex_unlock(&padctl->lock);
	return 0;
}

static int tegra186_usb3_phy_power_off(struct phy *phy)
{
	struct tegra_padctl *padctl = phy_get_drvdata(phy);
	struct device *dev = padctl->dev;
	int port = usb3_phy_to_port(phy);
	u32 reg;

	if (port < 0)
		return port;

	mutex_lock(&padctl->lock);

	dev_dbg(dev, "power off USB3 port %d\n", port);

	reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM_1);
	reg |= SSPX_ELPG_CLAMP_EN_EARLY(port);
	padctl_writel(padctl, reg, XUSB_PADCTL_ELPG_PROGRAM_1);

	usleep_range(100, 200);

	reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM_1);
	reg |= SSPX_ELPG_CLAMP_EN(port);
	padctl_writel(padctl, reg, XUSB_PADCTL_ELPG_PROGRAM_1);

	usleep_range(250, 350);

	reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM_1);
	reg |= SSPX_ELPG_VCORE_DOWN(port);
	padctl_writel(padctl, reg, XUSB_PADCTL_ELPG_PROGRAM_1);

	mutex_unlock(&padctl->lock);

	return 0;
}

static int tegra186_usb3_phy_enable_wakelogic(struct tegra_padctl *padctl,
					      int port)
{
	struct device *dev = padctl->dev;
	u32 reg;

	dev_dbg(dev, "enable wakelogic USB3 port %d\n", port);

	reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM_1);
	reg |= SSPX_ELPG_CLAMP_EN_EARLY(port);
	padctl_writel(padctl, reg, XUSB_PADCTL_ELPG_PROGRAM_1);

	usleep_range(100, 200);

	reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM_1);
	reg |= SSPX_ELPG_CLAMP_EN(port);
	padctl_writel(padctl, reg, XUSB_PADCTL_ELPG_PROGRAM_1);

	usleep_range(250, 350);

	return 0;
}

static int tegra186_usb3_phy_disable_wakelogic(struct tegra_padctl *padctl,
					      int port)
{
	struct device *dev = padctl->dev;
	u32 reg;

	dev_dbg(dev, "disable wakelogic USB3 port %d\n", port);

	reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM_1);
	reg &= ~SSPX_ELPG_CLAMP_EN_EARLY(port);
	padctl_writel(padctl, reg, XUSB_PADCTL_ELPG_PROGRAM_1);

	usleep_range(100, 200);

	reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM_1);
	reg &= ~SSPX_ELPG_CLAMP_EN(port);
	padctl_writel(padctl, reg, XUSB_PADCTL_ELPG_PROGRAM_1);

	return 0;
}

static int tegra186_usb3_phy_init(struct phy *phy)
{
	struct tegra_padctl *padctl = phy_get_drvdata(phy);
	int port = usb3_phy_to_port(phy);

	if (port < 0)
		return port;

	mutex_lock(&padctl->lock);

	dev_dbg(padctl->dev, "phy init USB3 port %d\n", port);

	mutex_unlock(&padctl->lock);

	return 0;
}

static int tegra186_usb3_phy_exit(struct phy *phy)
{
	struct tegra_padctl *padctl = phy_get_drvdata(phy);
	struct device *dev = padctl->dev;
	int port = usb3_phy_to_port(phy);

	if (port < 0)
		return port;

	mutex_lock(&padctl->lock);

	dev_dbg(dev, "phy exit USB3 port %d\n", port);

	mutex_unlock(&padctl->lock);

	return 0;
}

static const struct phy_ops usb3_phy_ops = {
	.init = tegra186_usb3_phy_init,
	.exit = tegra186_usb3_phy_exit,
	.power_on = tegra186_usb3_phy_power_on,
	.power_off = tegra186_usb3_phy_power_off,
	.owner = THIS_MODULE,
};
static inline bool is_usb3_phy(struct phy *phy)
{
	return phy->ops == &usb3_phy_ops;
}

static int utmi_phy_to_port(struct phy *phy)
{
	struct tegra_padctl *padctl = phy_get_drvdata(phy);
	unsigned int i;

	for (i = 0; i < TEGRA_UTMI_PHYS; i++) {
		if (phy == padctl->utmi_phys[i])
			return i;
	}
	WARN_ON(1);

	return -EINVAL;
}

static int cdp_phy_to_port(struct phy *phy)
{
	struct tegra_padctl *padctl = phy_get_drvdata(phy);
	unsigned int i;

	for (i = 0; i < TEGRA_CDP_PHYS; i++) {
		if (phy == padctl->cdp_phys[i])
			return i;
	}
	WARN_ON(1);

	return -EINVAL;
}

static int tegra186_utmi_phy_enable_sleepwalk(struct tegra_padctl *padctl,
				       int port, enum usb_device_speed speed)
{
	u32 reg;

	dev_dbg(padctl->dev, "enable sleepwalk UTMI port %d speed %d\n",
		port, speed);

	/* ensure sleepwalk logic is disabled */
	reg = ao_readl(padctl, XUSB_AO_UTMIP_SLEEPWALK_CFG(port));
	reg &= ~MASTER_ENABLE;
	ao_writel(padctl, reg, XUSB_AO_UTMIP_SLEEPWALK_CFG(port));

	/* ensure sleepwalk logics are in low power mode */
	reg = ao_readl(padctl, XUSB_AO_UTMIP_SLEEPWALK_CFG(port));
	reg |= MASTER_CFG_SEL;
	ao_writel(padctl, reg, XUSB_AO_UTMIP_SLEEPWALK_CFG(port));

	/* set debounce time */
	reg = ao_readl(padctl, XUSB_AO_USB_DEBOUNCE_DEL);
	reg &= ~UTMIP_LINE_DEB_CNT(~0);
	reg |= UTMIP_LINE_DEB_CNT(1);
	ao_writel(padctl, reg, XUSB_AO_USB_DEBOUNCE_DEL);

	/* ensure fake events of sleepwalk logic are desiabled */
	reg = ao_readl(padctl, XUSB_AO_UTMIP_SLEEPWALK_CFG(port));
	reg &= ~(FAKE_USBOP_VAL | FAKE_USBON_VAL |
		FAKE_USBOP_EN | FAKE_USBON_EN);
	ao_writel(padctl, reg, XUSB_AO_UTMIP_SLEEPWALK_CFG(port));

	/* ensure wake events of sleepwalk logic are not latched */
	reg = ao_readl(padctl, XUSB_AO_UTMIP_SLEEPWALK_CFG(port));
	reg &= ~LINE_WAKEUP_EN;
	ao_writel(padctl, reg, XUSB_AO_UTMIP_SLEEPWALK_CFG(port));

	/* disable wake event triggers of sleepwalk logic */
	reg = ao_readl(padctl, XUSB_AO_UTMIP_SLEEPWALK_CFG(port));
	reg &= ~WAKE_VAL(~0);
	reg |= WAKE_VAL_NONE;
	ao_writel(padctl, reg, XUSB_AO_UTMIP_SLEEPWALK_CFG(port));

	/* power down the line state detectors of the pad */
	reg = ao_readl(padctl, XUSB_AO_UTMIP_PAD_CFG(port));
	reg |= (USBOP_VAL_PD | USBON_VAL_PD);
	ao_writel(padctl, reg, XUSB_AO_UTMIP_PAD_CFG(port));

	/* save state per speed */
	reg = ao_readl(padctl, XUSB_AO_UTMIP_SAVED_STATE(port));
	reg &= ~SPEED(~0);
	if (speed == USB_SPEED_HIGH)
		reg |= UTMI_HS;
	else if (speed == USB_SPEED_FULL)
		reg |= UTMI_FS;
	else if (speed == USB_SPEED_LOW)
		reg |= UTMI_LS;
	else
		reg |= UTMI_RST;
	ao_writel(padctl, reg, XUSB_AO_UTMIP_SAVED_STATE(port));

	/* enable the trigger of the sleepwalk logic */
	reg = ao_readl(padctl, XUSB_AO_UTMIP_SLEEPWALK_CFG(port));
	reg |= LINEVAL_WALK_EN;
	reg &= ~WAKE_WALK_EN;
	ao_writel(padctl, reg, XUSB_AO_UTMIP_SLEEPWALK_CFG(port));

	/* reset the walk pointer and clear the alarm of the sleepwalk logic,
	 * as well as capture the configuration of the USB2.0 pad
	 */
	reg = ao_readl(padctl, XUSB_AO_UTMIP_TRIGGERS(port));
	reg |= (CLR_WALK_PTR | CLR_WAKE_ALARM | CAP_CFG);
	ao_writel(padctl, reg, XUSB_AO_UTMIP_TRIGGERS(port));

	/* setup the pull-ups and pull-downs of the signals during the four
	 * stages of sleepwalk.
	 * if device is connected, program sleepwalk logic to maintain a J and
	 * keep driving K upon seeing remote wake.
	 */
	reg = (USBOP_RPD_A | USBOP_RPD_B | USBOP_RPD_C | USBOP_RPD_D);
	reg |= (USBON_RPD_A | USBON_RPD_B | USBON_RPD_C | USBON_RPD_D);
	if (speed == USB_SPEED_UNKNOWN) {
		reg |= (HIGHZ_A | HIGHZ_B | HIGHZ_C | HIGHZ_D);
	} else if ((speed == USB_SPEED_HIGH) || (speed == USB_SPEED_FULL)) {
		/* J state: D+/D- = high/low, K state: D+/D- = low/high */
		reg |= HIGHZ_A;
		reg |= (AP_A);
		reg |= (AN_B | AN_C | AN_D);
	} else if (speed == USB_SPEED_LOW) {
		/* J state: D+/D- = low/high, K state: D+/D- = high/low */
		reg |= HIGHZ_A;
		reg |= AN_A;
		reg |= (AP_B | AP_C | AP_D);
	}
	ao_writel(padctl, reg, XUSB_AO_UTMIP_SLEEPWALK(port));

	/* power up the line state detectors of the pad */
	reg = ao_readl(padctl, XUSB_AO_UTMIP_PAD_CFG(port));
	reg &= ~(USBOP_VAL_PD | USBON_VAL_PD);
	ao_writel(padctl, reg, XUSB_AO_UTMIP_PAD_CFG(port));

	usleep_range(150, 200);

	/* switch the electric control of the USB2.0 pad to XUSB_AO */
	reg = ao_readl(padctl, XUSB_AO_UTMIP_PAD_CFG(port));
	reg |= (FSLS_USE_XUSB_AO | TRK_CTRL_USE_XUSB_AO |
		RPD_CTRL_USE_XUSB_AO | RPU_USE_XUSB_AO | VREG_USE_XUSB_AO);
	ao_writel(padctl, reg, XUSB_AO_UTMIP_PAD_CFG(port));

	/* set the wake signaling trigger events */
	reg = ao_readl(padctl, XUSB_AO_UTMIP_SLEEPWALK_CFG(port));
	reg &= ~WAKE_VAL(~0);
	reg |= WAKE_VAL_ANY;
	ao_writel(padctl, reg, XUSB_AO_UTMIP_SLEEPWALK_CFG(port));

	/* enable the wake detection */
	reg = ao_readl(padctl, XUSB_AO_UTMIP_SLEEPWALK_CFG(port));
	reg |= (MASTER_ENABLE | LINE_WAKEUP_EN);
	ao_writel(padctl, reg, XUSB_AO_UTMIP_SLEEPWALK_CFG(port));

	return 0;
}

static int tegra186_utmi_phy_disable_sleepwalk(struct tegra_padctl *padctl,
					       int port)
{
	u32 reg;

	dev_dbg(padctl->dev, "disable sleepwalk UTMI port %d\n",  port);

	/* disable the wake detection */
	reg = ao_readl(padctl, XUSB_AO_UTMIP_SLEEPWALK_CFG(port));
	reg &= ~(MASTER_ENABLE | LINE_WAKEUP_EN);
	ao_writel(padctl, reg, XUSB_AO_UTMIP_SLEEPWALK_CFG(port));

	/* switch the electric control of the USB2.0 pad to XUSB vcore logic */
	reg = ao_readl(padctl, XUSB_AO_UTMIP_PAD_CFG(port));
	reg &= ~(FSLS_USE_XUSB_AO | TRK_CTRL_USE_XUSB_AO |
		RPD_CTRL_USE_XUSB_AO | RPU_USE_XUSB_AO | VREG_USE_XUSB_AO);
	ao_writel(padctl, reg, XUSB_AO_UTMIP_PAD_CFG(port));

	/* disable wake event triggers of sleepwalk logic */
	reg = ao_readl(padctl, XUSB_AO_UTMIP_SLEEPWALK_CFG(port));
	reg &= ~WAKE_VAL(~0);
	reg |= WAKE_VAL_NONE;
	ao_writel(padctl, reg, XUSB_AO_UTMIP_SLEEPWALK_CFG(port));

	/* power down the line state detectors of the port */
	reg = ao_readl(padctl, XUSB_AO_UTMIP_PAD_CFG(port));
	reg |= (USBOP_VAL_PD | USBON_VAL_PD);
	ao_writel(padctl, reg, XUSB_AO_UTMIP_PAD_CFG(port));

	/* clear alarm of the sleepwalk logic */
	reg = ao_readl(padctl, XUSB_AO_UTMIP_TRIGGERS(port));
	reg |= CLR_WAKE_ALARM;
	ao_writel(padctl, reg, XUSB_AO_UTMIP_TRIGGERS(port));

	return 0;
}

static void tegra186_utmi_bias_pad_power_on(struct tegra_padctl *padctl)
{
	u32 reg;

	if (padctl->bias_pad_enable++ > 0)
		return;

	reg = padctl_readl(padctl, XUSB_PADCTL_USB2_BIAS_PAD_CTL1);
	reg &= ~USB2_TRK_START_TIMER(~0);
	reg |= USB2_TRK_START_TIMER(0x1e);
	reg &= ~USB2_TRK_DONE_RESET_TIMER(~0);
	reg |= USB2_TRK_DONE_RESET_TIMER(0xa);
	padctl_writel(padctl, reg, XUSB_PADCTL_USB2_BIAS_PAD_CTL1);

	reg = padctl_readl(padctl, XUSB_PADCTL_USB2_BIAS_PAD_CTL0);
	reg &= ~BIAS_PAD_PD;
	reg &= ~HS_SQUELCH_LEVEL(~0);
	reg |= HS_SQUELCH_LEVEL(padctl->calib.hs_squelch);
	padctl_writel(padctl, reg, XUSB_PADCTL_USB2_BIAS_PAD_CTL0);

	udelay(1);

	reg = padctl_readl(padctl, XUSB_PADCTL_USB2_BIAS_PAD_CTL1);
	reg &= ~USB2_PD_TRK;
	padctl_writel(padctl, reg, XUSB_PADCTL_USB2_BIAS_PAD_CTL1);
}

static void tegra186_utmi_bias_pad_power_off(struct tegra_padctl *padctl)
{
	u32 reg;

	if (WARN_ON(padctl->bias_pad_enable == 0))
		return;

	if (--padctl->bias_pad_enable > 0)
		return;

	if (!padctl->cdp_used) {
		/* only turn BIAS pad off when host CDP isn't enabled */
		reg = padctl_readl(padctl, XUSB_PADCTL_USB2_BIAS_PAD_CTL0);
		reg |= BIAS_PAD_PD;
		padctl_writel(padctl, reg, XUSB_PADCTL_USB2_BIAS_PAD_CTL0);
	}

	reg = padctl_readl(padctl, XUSB_PADCTL_USB2_BIAS_PAD_CTL1);
	reg |= USB2_PD_TRK;
	padctl_writel(padctl, reg, XUSB_PADCTL_USB2_BIAS_PAD_CTL1);
}

void tegra18x_phy_xusb_utmi_pad_power_on(struct phy *phy)
{
	struct tegra_padctl *padctl;
	int port;
	u32 reg;

	if (!phy)
		return;

	padctl = phy_get_drvdata(phy);
	port = utmi_phy_to_port(phy);

	if (padctl->utmi_ports[port].poweron)
		return;

	tegra186_utmi_bias_pad_power_on(padctl);

	udelay(2);

	reg = padctl_readl(padctl, XUSB_PADCTL_USB2_OTG_PADX_CTL0(port));
	reg &= ~USB2_OTG_PD;
	padctl_writel(padctl, reg, XUSB_PADCTL_USB2_OTG_PADX_CTL0(port));

	reg = padctl_readl(padctl, XUSB_PADCTL_USB2_OTG_PADX_CTL1(port));
	reg &= ~USB2_OTG_PD_DR;
	padctl_writel(padctl, reg, XUSB_PADCTL_USB2_OTG_PADX_CTL1(port));

	padctl->utmi_ports[port].poweron = true;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_utmi_pad_power_on);

void tegra18x_phy_xusb_utmi_pad_power_down(struct phy *phy)
{
	struct tegra_padctl *padctl;
	int port;
	u32 reg;

	if (!phy)
		return;

	padctl = phy_get_drvdata(phy);
	port = utmi_phy_to_port(phy);

	if (!padctl->utmi_ports[port].poweron)
		return;

	reg = padctl_readl(padctl, XUSB_PADCTL_USB2_OTG_PADX_CTL0(port));
	reg |= USB2_OTG_PD;
	padctl_writel(padctl, reg, XUSB_PADCTL_USB2_OTG_PADX_CTL0(port));

	reg = padctl_readl(padctl, XUSB_PADCTL_USB2_OTG_PADX_CTL1(port));
	reg |= USB2_OTG_PD_DR;
	padctl_writel(padctl, reg, XUSB_PADCTL_USB2_OTG_PADX_CTL1(port));

	udelay(2);

	tegra186_utmi_bias_pad_power_off(padctl);
	padctl->utmi_ports[port].poweron = false;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_utmi_pad_power_down);

#define oc_debug(u) \
		dev_dbg(u->dev, "%s(%d):OC_DET %#x, VBUS_OC_MAP %#x, "\
			"USB2_OC_MAP %#x, SS_OC_MAP %#x\n",\
			__func__, __LINE__,\
			padctl_readl(u, XUSB_PADCTL_OC_DET), \
			padctl_readl(u, XUSB_PADCTL_VBUS_OC_MAP), \
			padctl_readl(u, XUSB_PADCTL_USB2_OC_MAP), \
			padctl_readl(u, XUSB_PADCTL_SS_OC_MAP));

/* should only be called with a UTMI phy and with padctl->lock held */
static void tegra186_enable_vbus_oc(struct phy *phy)
{
	struct tegra_padctl *padctl;
	int port, pin;
	u32 reg;

	if (!phy)
		return;

	padctl = phy_get_drvdata(phy);
	port = utmi_phy_to_port(phy);

	if (!padctl->oc_pinctrl) {
		dev_dbg(padctl->dev, "%s no OC pinctrl device\n", __func__);
		return;
	}

	if (port < 0) {
		dev_warn(padctl->dev, "%s wrong port %d\n", __func__, port);
		return;
	}

	pin = padctl->utmi_ports[port].oc_pin;
	if (pin < 0) {
		dev_dbg(padctl->dev, "%s no OC support for port %d\n", __func__,
			port);
		return;
	}

	dev_dbg(padctl->dev, "enable VBUS/OC on UTMI port %d, pin %d\n", port,
		pin);

	/* initialize OC: step 7 in PG p.1272 */
	reg = padctl_readl(padctl, XUSB_PADCTL_USB2_OC_MAP);
	reg &= ~(PORT_OC_PIN_MASK << PORTX_OC_PIN_SHIFT(port));
	reg |= OC_PIN_DETECTION_DISABLED << PORTX_OC_PIN_SHIFT(port);
	padctl_writel(padctl, reg, XUSB_PADCTL_USB2_OC_MAP);

	/* need to disable VBUS_ENABLEx_OC_MAP before enabling VBUS */
	reg = padctl_readl(padctl, XUSB_PADCTL_VBUS_OC_MAP);
	reg &= ~(VBUS_OC_MAP_MASK << VBUS_OC_MAP_SHIFT(pin));
	reg |= VBUS_OC_DETECTION_DISABLED << VBUS_OC_MAP_SHIFT(pin);
	padctl_writel(padctl, reg, XUSB_PADCTL_VBUS_OC_MAP);

	/* WAR: disable UTMIPLL power down, not needed for current clk
	 * framework */

	/* clear false OC_DETECTED VBUS_PADx */
	reg = padctl_readl(padctl, XUSB_PADCTL_OC_DET);
	reg &= ~OC_DETECTED_VBUS_PAD_MASK;
	reg |= OC_DETECTED_VBUS_PAD(pin);
	padctl_writel(padctl, reg, XUSB_PADCTL_OC_DET);

	udelay(100);

	/* WAR: enable UTMIPLL power down, not needed for current clk
	 * framework */

	/* Enable VBUS */
	reg = padctl_readl(padctl, XUSB_PADCTL_VBUS_OC_MAP);
	reg |= VBUS_ENABLE(pin);
	padctl_writel(padctl, reg, XUSB_PADCTL_VBUS_OC_MAP);

	/* vbus has been supplied to device. A finite time (>10ms) for OC
	 * detection pin to be pulled-up */
	msleep(20);

	/* check and clear if there is any stray OC */
	reg = padctl_readl(padctl, XUSB_PADCTL_OC_DET);
	if (reg & OC_DETECTED_VBUS_PAD(pin)) {
		/* clear stray OC */
		dev_dbg(padctl->dev,
			 "clear stray OC on port %d pin %d, OC_DET=%#x\n",
			 port, pin, reg);

		reg = padctl_readl(padctl, XUSB_PADCTL_VBUS_OC_MAP);
		reg &= ~VBUS_ENABLE(pin);

		reg = padctl_readl(padctl, XUSB_PADCTL_OC_DET);
		reg &= ~OC_DETECTED_VBUS_PAD_MASK;
		reg |= OC_DETECTED_VBUS_PAD(pin);
		padctl_writel(padctl, reg, XUSB_PADCTL_OC_DET);

		/* Enable VBUS back after clearing stray OC */
		reg = padctl_readl(padctl, XUSB_PADCTL_VBUS_OC_MAP);
		reg |= VBUS_ENABLE(pin);
		padctl_writel(padctl, reg, XUSB_PADCTL_VBUS_OC_MAP);
	}

	/* change the OC_MAP source and enable OC interrupt */
	reg = padctl_readl(padctl, XUSB_PADCTL_USB2_OC_MAP);
	reg &= ~(PORT_OC_PIN_MASK << PORTX_OC_PIN_SHIFT(port));
	reg |= (OC_PIN_DETECTED_VBUS_PAD(pin) & PORT_OC_PIN_MASK) <<
		PORTX_OC_PIN_SHIFT(port);
	padctl_writel(padctl, reg, XUSB_PADCTL_USB2_OC_MAP);

	reg = padctl_readl(padctl, XUSB_PADCTL_OC_DET);
	reg &= ~OC_DETECTED_VBUS_PAD_MASK;
	reg |= OC_DETECTED_INT_EN_VBUS_PAD(pin);
	padctl_writel(padctl, reg, XUSB_PADCTL_OC_DET);

	reg = padctl_readl(padctl, XUSB_PADCTL_VBUS_OC_MAP);
	reg &= ~(VBUS_OC_MAP_MASK << VBUS_OC_MAP_SHIFT(pin));
	reg |= (VBUS_OC_DETECTED_VBUS_PAD(pin) & VBUS_OC_MAP_MASK) <<
		VBUS_OC_MAP_SHIFT(pin);
	padctl_writel(padctl, reg, XUSB_PADCTL_VBUS_OC_MAP);

	oc_debug(padctl);
}

/* should only be called with a UTMI phy and with padctl->lock held */
static void tegra186_disable_vbus_oc(struct phy *phy)
{
	struct tegra_padctl *padctl;
	int port, pin;
	u32 reg;

	if (!phy)
		return;

	padctl = phy_get_drvdata(phy);
	port = utmi_phy_to_port(phy);

	if (!padctl->oc_pinctrl || port < 0)
		return;

	pin = padctl->utmi_ports[port].oc_pin;
	if (pin < 0)
		return;

	dev_dbg(padctl->dev, "disable VBUS/OC on UTMI port %d, pin %d\n",
		port, pin);

	/* disable VBUS PAD interrupt for this port */
	reg = padctl_readl(padctl, XUSB_PADCTL_OC_DET);
	reg &= ~OC_DETECTED_INT_EN_VBUS_PAD(pin);
	padctl_writel(padctl, reg, XUSB_PADCTL_OC_DET);

	/* clear VBUS OC MAP, disable VBUS. Skip doing so if it's OTG port and
	 * OTG vbus always on is set. */
	reg = padctl_readl(padctl, XUSB_PADCTL_VBUS_OC_MAP);
	reg &= ~(VBUS_OC_MAP_MASK << VBUS_OC_MAP_SHIFT(pin));
	reg |= VBUS_OC_DETECTION_DISABLED << VBUS_OC_MAP_SHIFT(pin);
	reg &= ~VBUS_ENABLE(pin);
	padctl_writel(padctl, reg, XUSB_PADCTL_VBUS_OC_MAP);
}

static int tegra186_utmi_phy_power_on(struct phy *phy)
{
	struct tegra_padctl *padctl = phy_get_drvdata(phy);
	struct device *dev = padctl->dev;
	int port = utmi_phy_to_port(phy);
	char prod_name[] = "prod_c_utmiX";
	int err;
	u32 reg;

	if (port < 0)
		return port;

	dev_dbg(dev, "power on UTMI port %d\n",  port);

	sprintf(prod_name, "prod_c_utmi%d", port);
	err = tegra_prod_set_by_name(&padctl->padctl_regs, prod_name,
				     padctl->prod_list);
	if (err) {
		dev_info(dev, "failed to apply prod for utmi pad%d (%d)\n",
			port, err);
	}

	sprintf(prod_name, "prod_c_bias");
	err = tegra_prod_set_by_name(&padctl->padctl_regs, prod_name,
				     padctl->prod_list);
	if (err)
		dev_info(dev, "failed to apply prod for bias pad (%d)\n", err);

	reg = padctl_readl(padctl, XUSB_PADCTL_USB2_PORT_CAP);
	reg &= ~(PORT_CAP_MASK << PORTX_CAP_SHIFT(port));
	if (padctl->utmi_ports[port].port_cap == CAP_DISABLED)
		reg |= (PORT_CAP_DISABLED << PORTX_CAP_SHIFT(port));
	else if (padctl->utmi_ports[port].port_cap == DEVICE_ONLY)
		reg |= (PORT_CAP_DEVICE << PORTX_CAP_SHIFT(port));
	else if (padctl->utmi_ports[port].port_cap == HOST_ONLY)
		reg |= (PORT_CAP_HOST << PORTX_CAP_SHIFT(port));
	else if (padctl->utmi_ports[port].port_cap == OTG)
		reg |= (PORT_CAP_OTG << PORTX_CAP_SHIFT(port));
	padctl_writel(padctl, reg, XUSB_PADCTL_USB2_PORT_CAP);

	reg = padctl_readl(padctl, XUSB_PADCTL_USB2_OTG_PADX_CTL0(port));
	reg &= ~USB2_OTG_PD_ZI;
	reg |= TERM_SEL;
	reg &= ~HS_CURR_LEVEL(~0);
	if (padctl->utmi_ports[port].hs_curr_level_offset) {
		int hs_current_level;

		dev_dbg(dev, "UTMI port %d apply hs_curr_level_offset %d\n",
			port, padctl->utmi_ports[port].hs_curr_level_offset);

		hs_current_level = (int) padctl->calib.hs_curr_level[port] +
			padctl->utmi_ports[port].hs_curr_level_offset;

		if (hs_current_level < 0)
			hs_current_level = 0;
		if (hs_current_level > 0x3f)
			hs_current_level = 0x3f;

		reg |= HS_CURR_LEVEL(hs_current_level);
	} else
		reg |= HS_CURR_LEVEL(padctl->calib.hs_curr_level[port]);
	padctl_writel(padctl, reg, XUSB_PADCTL_USB2_OTG_PADX_CTL0(port));

	reg = padctl_readl(padctl, XUSB_PADCTL_USB2_OTG_PADX_CTL1(port));
	reg &= ~TERM_RANGE_ADJ(~0);
	reg |= TERM_RANGE_ADJ(padctl->calib.hs_term_range_adj);
	reg &= ~RPD_CTRL(~0);
	reg |= RPD_CTRL(padctl->calib.rpd_ctrl);
	padctl_writel(padctl, reg, XUSB_PADCTL_USB2_OTG_PADX_CTL1(port));

	/* enable VBUS OC support only on non-OTG port */
	if (port != padctl->utmi_otg_port_base_1 - 1) {
		mutex_lock(&padctl->lock);
		tegra186_enable_vbus_oc(phy);
		mutex_unlock(&padctl->lock);
	}

	return 0;
}

static int tegra186_utmi_phy_power_off(struct phy *phy)
{
	struct tegra_padctl *padctl = phy_get_drvdata(phy);
	int port = utmi_phy_to_port(phy);

	if (port < 0)
		return port;

	dev_dbg(padctl->dev, "power off UTMI port %d\n", port);

	return 0;
}

int tegra18x_utmi_vbus_enable(struct phy *phy)
{
	struct tegra_padctl *padctl = phy_get_drvdata(phy);
	int port = utmi_phy_to_port(phy);
	int rc;

	if (port < 0)
		return port;

	dev_dbg(padctl->dev, "enable vbus-%d\n", port);

	mutex_lock(&padctl->lock);

	/* only enable regulator when OC is disabled for host only ports */
	/* OC is disabled when either oc_pinctrl is NULL or oc_pin is not
	 * defined (-1)
	 */
	if (padctl->vbus[port] && (!padctl->oc_pinctrl ||
			padctl->utmi_ports[port].oc_pin < 0) &&
			padctl->utmi_ports[port].port_cap == HOST_ONLY) {
		rc = regulator_enable(padctl->vbus[port]);
		if (rc) {
			dev_err(padctl->dev, "enable port %d vbus failed %d\n",
				port, rc);
			mutex_unlock(&padctl->lock);
			return rc;
		}
	}

	mutex_unlock(&padctl->lock);

	return 0;
}
EXPORT_SYMBOL_GPL(tegra18x_utmi_vbus_enable);

static int tegra186_utmi_phy_init(struct phy *phy)
{
	struct tegra_padctl *padctl = phy_get_drvdata(phy);
	int port = utmi_phy_to_port(phy);

	if (port < 0)
		return port;

	mutex_lock(&padctl->lock);

	dev_dbg(padctl->dev, "phy init UTMI port %d\n",  port);

	mutex_unlock(&padctl->lock);

	return 0;
}

static int tegra186_utmi_phy_exit(struct phy *phy)
{
	struct tegra_padctl *padctl = phy_get_drvdata(phy);
	int port = utmi_phy_to_port(phy);
	int rc;

	if (port < 0)
		return port;

	dev_dbg(padctl->dev, "phy exit UTMI port %d\n",  port);

	mutex_lock(&padctl->lock);

	if (padctl->vbus[port] && regulator_is_enabled(padctl->vbus[port]) &&
			padctl->utmi_ports[port].port_cap == HOST_ONLY) {
		rc = regulator_disable(padctl->vbus[port]);
		if (rc) {
			dev_err(padctl->dev, "disable port %d vbus failed %d\n",
				port, rc);
			mutex_unlock(&padctl->lock);
			return rc;
		}
	}

	mutex_unlock(&padctl->lock);

	return 0;
}

static const struct phy_ops utmi_phy_ops = {
	.init = tegra186_utmi_phy_init,
	.exit = tegra186_utmi_phy_exit,
	.power_on = tegra186_utmi_phy_power_on,
	.power_off = tegra186_utmi_phy_power_off,
	.owner = THIS_MODULE,
};

static int tegra186_cdp_phy_set_cdp(struct phy *phy, bool enable)
{
	struct tegra_padctl *padctl = phy_get_drvdata(phy);
	int port = cdp_phy_to_port(phy);
	u32 reg;

	dev_info(padctl->dev, "%sable UTMI port %d Tegra CDP\n",
		 enable ? "en" : "dis", port);
	if (enable) {
		reg = padctl_readl(padctl,
				   USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
		reg &= ~PD_CHG;
		padctl_writel(padctl, reg,
			      USB2_BATTERY_CHRG_OTGPADX_CTL0(port));

		reg = padctl_readl(padctl,
				   XUSB_PADCTL_USB2_OTG_PADX_CTL0(port));
		reg |= (USB2_OTG_PD2 | USB2_OTG_PD2_OVRD_EN);
		padctl_writel(padctl, reg,
			      XUSB_PADCTL_USB2_OTG_PADX_CTL0(port));

		reg = padctl_readl(padctl,
				   USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
		reg |= ON_SRC_EN;
		padctl_writel(padctl, reg,
			      USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
	} else {
		reg = padctl_readl(padctl,
				   USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
		reg |= PD_CHG;
		padctl_writel(padctl, reg,
			      USB2_BATTERY_CHRG_OTGPADX_CTL0(port));

		reg = padctl_readl(padctl,
				   XUSB_PADCTL_USB2_OTG_PADX_CTL0(port));
		reg &= ~(USB2_OTG_PD2 | USB2_OTG_PD2_OVRD_EN);
		padctl_writel(padctl, reg,
			      XUSB_PADCTL_USB2_OTG_PADX_CTL0(port));

		reg = padctl_readl(padctl,
				   USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
		reg &= ~ON_SRC_EN;
		padctl_writel(padctl, reg,
			      USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
	}

	return 0;
}

static int tegra186_cdp_phy_power_on(struct phy *phy)
{
	return tegra186_cdp_phy_set_cdp(phy, true);
}

static int tegra186_cdp_phy_power_off(struct phy *phy)
{
	return tegra186_cdp_phy_set_cdp(phy, false);
}

static const struct phy_ops cdp_phy_ops = {
	.power_on = tegra186_cdp_phy_power_on,
	.power_off = tegra186_cdp_phy_power_off,
	.owner = THIS_MODULE,
};

static inline bool is_utmi_phy(struct phy *phy)
{
	return phy->ops == &utmi_phy_ops;
}

static int hsic_phy_to_port(struct phy *phy)
{
	struct tegra_padctl *padctl = phy_get_drvdata(phy);
	unsigned int i;

	for (i = 0; i < TEGRA_HSIC_PHYS; i++) {
		if (phy == padctl->hsic_phys[i])
			return i;
	}
	WARN_ON(1);

	return -EINVAL;
}

enum hsic_pad_pupd {
	PUPD_DISABLE = 0,
	PUPD_IDLE,
	PUPD_RESET
};

static int tegra186_hsic_phy_pupd_set(struct tegra_padctl *padctl, int pad,
				      enum hsic_pad_pupd pupd)
{
	struct device *dev = padctl->dev;
	u32 reg;

	if (pad >= 1) {
		dev_err(dev, "%s invalid HSIC pad number %u\n", __func__, pad);
		return -EINVAL;
	}

	dev_dbg(dev, "%s pad %u pupd %d\n", __func__, pad, pupd);

	reg = padctl_readl(padctl, XUSB_PADCTL_HSIC_PADX_CTL0(pad));
	reg &= ~(HSIC_RPD_DATA0 | HSIC_RPU_DATA0);
	reg &= ~(HSIC_RPU_STROBE | HSIC_RPD_STROBE);
	if (pupd == PUPD_IDLE) {
		reg |= (HSIC_RPD_DATA0 | HSIC_RPU_STROBE);
	} else if (pupd == PUPD_RESET) {
		reg |= (HSIC_RPD_DATA0 | HSIC_RPD_STROBE);
	} else if (pupd != PUPD_DISABLE) {
		dev_err(dev, "%s invalid pupd %d\n", __func__, pupd);
		return -EINVAL;
	}
	padctl_writel(padctl, reg, XUSB_PADCTL_HSIC_PADX_CTL0(pad));

	return 0;
}

static ssize_t hsic_power_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct tegra_padctl *padctl = platform_get_drvdata(pdev);
	int pad = 0;
	u32 reg;
	int on;

	reg = padctl_readl(padctl, XUSB_PADCTL_HSIC_PADX_CTL0(pad));

	if (reg & (HSIC_RPD_DATA0 | HSIC_RPD_STROBE))
		on = 0; /* bus in reset */
	else
		on = 1;

	return sprintf(buf, "%d\n", on);
}

static ssize_t hsic_power_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t n)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct tegra_padctl *padctl = platform_get_drvdata(pdev);
	struct tegra_xusb_mbox_msg msg;
	unsigned int on;
	int port;
	int rc;

	if (kstrtouint(buf, 10, &on))
		return -EINVAL;

	if (padctl->host_mode_phy_disabled) {
		dev_err(dev, "doesn't support HSIC PHY because mailbox is not available\n");
		return -EINVAL;
	}

	if (on)
		msg.cmd = MBOX_CMD_AIRPLANE_MODE_DISABLED;
	else
		msg.cmd = MBOX_CMD_AIRPLANE_MODE_ENABLED;

	port = padctl->soc->hsic_port_offset;
	msg.data = BIT(port + 1);
	rc = mbox_send_message(padctl->mbox_chan, &msg);
	if (rc < 0)
		dev_err(dev, "failed to send message to firmware %d\n", rc);

	if (on)
		rc = tegra186_hsic_phy_pupd_set(padctl, 0, PUPD_IDLE);
	else
		rc = tegra186_hsic_phy_pupd_set(padctl, 0, PUPD_RESET);

	return n;
}
static DEVICE_ATTR(hsic_power, S_IRUGO | S_IWUSR,
		   hsic_power_show, hsic_power_store);

static ssize_t otg_vbus_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct tegra_padctl *padctl = platform_get_drvdata(pdev);
	int port = padctl->utmi_otg_port_base_1 - 1;

	if (!padctl->utmi_otg_port_base_1)
		return sprintf(buf, "No UTMI OTG port\n");

	return sprintf(buf, "OTG port %d vbus always-on: %s\n",
			port, padctl->otg_vbus_alwayson ? "yes" : "no");
}

static ssize_t otg_vbus_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t n)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct tegra_padctl *padctl = platform_get_drvdata(pdev);
	int port = padctl->utmi_otg_port_base_1 - 1;
	unsigned int on;
	int err = 0;

	if (kstrtouint(buf, 10, &on))
		return -EINVAL;

	if (!padctl->utmi_otg_port_base_1) {
		dev_err(dev, "No UTMI OTG port\n");
		return -EINVAL;
	}

	if (on && !padctl->otg_vbus_alwayson) {
		err = tegra18x_phy_xusb_utmi_vbus_power_on(
				padctl->utmi_phys[port]);
		if (!err)
			padctl->otg_vbus_alwayson = true;
	} else if (!on && padctl->otg_vbus_alwayson) {
		/* pre-set this to make vbus power off really work */
		padctl->otg_vbus_alwayson = false;
		err = tegra18x_phy_xusb_utmi_vbus_power_off(
				padctl->utmi_phys[port]);
		if (!err)
			padctl->otg_vbus_alwayson = false;
		else
			padctl->otg_vbus_alwayson = true;
	}

	if (err)
		dev_err(dev, "failed to %s OTG port %d vbus always-on: %d\n",
				on ? "enable" : "disable", port, err);

	return n;
}

static DEVICE_ATTR(otg_vbus, S_IRUGO | S_IWUSR, otg_vbus_show, otg_vbus_store);

static struct attribute *padctl_attrs[] = {
	&dev_attr_hsic_power.attr,
	&dev_attr_otg_vbus.attr,
	NULL,
};
static struct attribute_group padctl_attr_group = {
	.attrs = padctl_attrs,
};

static int tegra186_hsic_phy_pretend_connected(struct tegra_padctl *padctl,
					       int port)
{
	struct device *dev = padctl->dev;
	struct tegra_xusb_mbox_msg msg;
	int rc;

	if (!padctl->hsic_ports[port].pretend_connected)
		return 0; /* pretend-connected is not enabled */

	msg.cmd = MBOX_CMD_HSIC_PRETEND_CONNECT;
	msg.data = BIT(padctl->soc->hsic_port_offset + port + 1);
	rc = mbox_send_message(padctl->mbox_chan, &msg);
	if (rc < 0)
		dev_err(dev, "failed to send message to firmware %d\n", rc);

	return rc;
}

static int tegra186_hsic_phy_enable_sleepwalk(struct tegra_padctl *padctl,
					      int port)
{
	u32 reg;

	dev_dbg(padctl->dev, "enable sleepwalk HSIC port %d\n",  port);

	/* ensure sleepwalk logic is disabled */
	reg = ao_readl(padctl, XUSB_AO_UHSIC_SLEEPWALK_CFG(port));
	reg &= ~MASTER_ENABLE;
	ao_writel(padctl, reg, XUSB_AO_UHSIC_SLEEPWALK_CFG(port));

	/* ensure sleepwalk logics are in low power mode */
	reg = ao_readl(padctl, XUSB_AO_UHSIC_SLEEPWALK_CFG(port));
	reg |= MASTER_CFG_SEL;
	ao_writel(padctl, reg, XUSB_AO_UHSIC_SLEEPWALK_CFG(port));

	/* set debounce time */
	reg = ao_readl(padctl, XUSB_AO_USB_DEBOUNCE_DEL);
	reg &= ~UHSIC_LINE_DEB_CNT(~0);
	reg |= UHSIC_LINE_DEB_CNT(1);
	ao_writel(padctl, reg, XUSB_AO_USB_DEBOUNCE_DEL);

	/* ensure fake events of sleepwalk logic are desiabled */
	reg = ao_readl(padctl, XUSB_AO_UHSIC_SLEEPWALK_CFG(port));
	reg &= ~(FAKE_STROBE_VAL | FAKE_DATA_VAL |
		FAKE_STROBE_EN | FAKE_DATA_EN);
	ao_writel(padctl, reg, XUSB_AO_UHSIC_SLEEPWALK_CFG(port));

	/* ensure wake events of sleepwalk logic are not latched */
	reg = ao_readl(padctl, XUSB_AO_UHSIC_SLEEPWALK_CFG(port));
	reg &= ~LINE_WAKEUP_EN;
	ao_writel(padctl, reg, XUSB_AO_UHSIC_SLEEPWALK_CFG(port));

	/* disable wake event triggers of sleepwalk logic */
	reg = ao_readl(padctl, XUSB_AO_UHSIC_SLEEPWALK_CFG(port));
	reg &= ~WAKE_VAL(~0);
	reg |= WAKE_VAL_NONE;
	ao_writel(padctl, reg, XUSB_AO_UHSIC_SLEEPWALK_CFG(port));

	/* power down the line state detectors of the port */
	reg = ao_readl(padctl, XUSB_AO_UHSIC_PAD_CFG(port));
	reg |= (STROBE_VAL_PD | DATA0_VAL_PD);
	ao_writel(padctl, reg, XUSB_AO_UHSIC_PAD_CFG(port));

	/* save state, HSIC always comes up as HS */
	reg = ao_readl(padctl, XUSB_AO_UHSIC_SAVED_STATE(port));
	reg &= ~MODE(~0);
	reg |= MODE_HS;
	ao_writel(padctl, reg, XUSB_AO_UHSIC_SAVED_STATE(port));

	/* enable the trigger of the sleepwalk logic */
	reg = ao_readl(padctl, XUSB_AO_UHSIC_SLEEPWALK_CFG(port));
	reg |= (WAKE_WALK_EN | LINEVAL_WALK_EN);
	ao_writel(padctl, reg, XUSB_AO_UHSIC_SLEEPWALK_CFG(port));

	/* reset the walk pointer and clear the alarm of the sleepwalk logic,
	 * as well as capture the configuration of the USB2.0 port
	 */
	reg = ao_readl(padctl, XUSB_AO_UHSIC_TRIGGERS(port));
	reg |= (HSIC_CLR_WALK_PTR | HSIC_CLR_WAKE_ALARM | HSIC_CAP_CFG);
	ao_writel(padctl, reg, XUSB_AO_UHSIC_TRIGGERS(port));

	/* setup the pull-ups and pull-downs of the signals during the four
	 * stages of sleepwalk.
	 * maintain a HSIC IDLE and keep driving HSIC RESUME upon remote wake
	 */
	reg = (RPD_DATA0_A | RPU_DATA0_B | RPU_DATA0_C | RPU_DATA0_D);
	reg |= (RPU_STROBE_A | RPD_STROBE_B | RPD_STROBE_C | RPD_STROBE_D);
	ao_writel(padctl, reg, XUSB_AO_UHSIC_SLEEPWALK(port));

	/* power up the line state detectors of the port */
	reg = ao_readl(padctl, XUSB_AO_UHSIC_PAD_CFG(port));
	reg &= ~(DATA0_VAL_PD | STROBE_VAL_PD);
	ao_writel(padctl, reg, XUSB_AO_UHSIC_PAD_CFG(port));

	usleep_range(150, 200);

	/* switch the electric control of the USB2.0 pad to XUSB_AO */
	reg = ao_readl(padctl, XUSB_AO_UHSIC_PAD_CFG(port));
	reg |= USE_XUSB_AO;
	ao_writel(padctl, reg, XUSB_AO_UHSIC_PAD_CFG(port));

	/* set the wake signaling trigger events */
	reg = ao_readl(padctl, XUSB_AO_UHSIC_SLEEPWALK_CFG(port));
	reg &= ~WAKE_VAL(~0);
	reg |= WAKE_VAL_DS10;
	ao_writel(padctl, reg, XUSB_AO_UHSIC_SLEEPWALK_CFG(port));

	/* enable the wake detection */
	reg = ao_readl(padctl, XUSB_AO_UHSIC_SLEEPWALK_CFG(port));
	reg |= (MASTER_ENABLE | LINE_WAKEUP_EN);
	ao_writel(padctl, reg, XUSB_AO_UHSIC_SLEEPWALK_CFG(port));

	return 0;
}

static int tegra186_hsic_phy_disable_sleepwalk(struct tegra_padctl *padctl,
					       int port)
{
	u32 reg;

	dev_dbg(padctl->dev, "disable sleepwalk HSIC port %d\n",  port);

	/* disable the wake detection */
	reg = ao_readl(padctl, XUSB_AO_UHSIC_SLEEPWALK_CFG(port));
	reg &= ~(MASTER_ENABLE | LINE_WAKEUP_EN);
	ao_writel(padctl, reg, XUSB_AO_UHSIC_SLEEPWALK_CFG(port));

	/* switch the electric control of the USB2.0 pad to XUSB vcore logic */
	reg = ao_readl(padctl, XUSB_AO_UHSIC_PAD_CFG(port));
	reg &= ~USE_XUSB_AO;
	ao_writel(padctl, reg, XUSB_AO_UHSIC_PAD_CFG(port));

	/* disable wake event triggers of sleepwalk logic */
	reg = ao_readl(padctl, XUSB_AO_UHSIC_SLEEPWALK_CFG(port));
	reg &= ~WAKE_VAL(~0);
	reg |= WAKE_VAL_NONE;
	ao_writel(padctl, reg, XUSB_AO_UHSIC_SLEEPWALK_CFG(port));

	/* power down the line state detectors of the port */
	reg = ao_readl(padctl, XUSB_AO_UHSIC_PAD_CFG(port));
	reg |= (STROBE_VAL_PD | DATA0_VAL_PD);
	ao_writel(padctl, reg, XUSB_AO_UHSIC_PAD_CFG(port));

	/* clear alarm of the sleepwalk logic */
	reg = ao_readl(padctl, XUSB_AO_UHSIC_TRIGGERS(port));
	reg |= HSIC_CLR_WAKE_ALARM;
	ao_writel(padctl, reg, XUSB_AO_UHSIC_TRIGGERS(port));

	return 0;
}

static int tegra186_hsic_phy_power_on(struct phy *phy)
{
	struct tegra_padctl *padctl = phy_get_drvdata(phy);
	struct device *dev = padctl->dev;
	int port = hsic_phy_to_port(phy);
	char prod_name[] = "prod_c_hsicX";
	int rc;
	u32 reg;

	dev_dbg(dev, "power on HSIC port %d\n", port);
	if (port < 0)
		return port;

	sprintf(prod_name, "prod_c_hsic%d", port);
	rc = tegra_prod_set_by_name(&padctl->padctl_regs, prod_name,
				    padctl->prod_list);
	if (rc) {
		dev_info(dev, "failed to apply prod for hsic pad%d (%d)\n",
			 port, rc);
	}

	rc = regulator_enable(padctl->vddio_hsic);
	if (rc) {
		dev_err(dev, "enable hsic %d power failed %d\n",
			port, rc);
		return rc;
	}

	rc = clk_prepare_enable(padctl->hsic_trk_clk);
	if (rc) {
		dev_err(dev, "failed to enable HSIC tracking clock %d\n",
			rc);
	}

	reg = padctl_readl(padctl, XUSB_PADCTL_HSIC_PAD_TRK_CTL0);
	reg &= ~HSIC_TRK_START_TIMER(~0);
	reg |= HSIC_TRK_START_TIMER(0x1e);
	reg &= ~HSIC_TRK_DONE_RESET_TIMER(~0);
	reg |= HSIC_TRK_DONE_RESET_TIMER(0xa);
	padctl_writel(padctl, reg, XUSB_PADCTL_HSIC_PAD_TRK_CTL0);

	reg = padctl_readl(padctl, XUSB_PADCTL_HSIC_PADX_CTL0(port));
	reg &= ~(HSIC_PD_TX_DATA0 | HSIC_PD_TX_STROBE |
		HSIC_PD_RX_DATA0 | HSIC_PD_RX_STROBE |
		HSIC_PD_ZI_DATA0 | HSIC_PD_ZI_STROBE);
	padctl_writel(padctl, reg, XUSB_PADCTL_HSIC_PADX_CTL0(port));

	udelay(1);

	reg = padctl_readl(padctl, XUSB_PADCTL_HSIC_PAD_TRK_CTL0);
	reg &= ~HSIC_PD_TRK;
	padctl_writel(padctl, reg, XUSB_PADCTL_HSIC_PAD_TRK_CTL0);

	usleep_range(50, 60);

	clk_disable_unprepare(padctl->hsic_trk_clk);

	return 0;
}

static int tegra186_hsic_phy_power_off(struct phy *phy)
{
	struct tegra_padctl *padctl = phy_get_drvdata(phy);
	int port = hsic_phy_to_port(phy);
	int rc;
	u32 reg;

	dev_dbg(padctl->dev, "power off HSIC port %d\n", port);
	if (port < 0)
		return port;

	rc = regulator_disable(padctl->vddio_hsic);
	if (rc) {
		dev_err(padctl->dev, "disable hsic %d power failed %d\n",
			port, rc);
	}
	reg = padctl_readl(padctl, XUSB_PADCTL_HSIC_PADX_CTL0(port));
	reg |= (HSIC_PD_TX_DATA0 | HSIC_PD_TX_STROBE |
		HSIC_PD_RX_DATA0 | HSIC_PD_RX_STROBE |
		HSIC_PD_ZI_DATA0 | HSIC_PD_ZI_STROBE);
	padctl_writel(padctl, reg, XUSB_PADCTL_HSIC_PADX_CTL0(port));

	reg = padctl_readl(padctl, XUSB_PADCTL_HSIC_PAD_TRK_CTL0);
	reg |= HSIC_PD_TRK;
	padctl_writel(padctl, reg, XUSB_PADCTL_HSIC_PAD_TRK_CTL0);

	return 0;
}

static int tegra186_hsic_phy_init(struct phy *phy)
{
	struct tegra_padctl *padctl = phy_get_drvdata(phy);
	int port = hsic_phy_to_port(phy);

	mutex_lock(&padctl->lock);

	dev_dbg(padctl->dev, "phy init HSIC port %d\n", port);

	mutex_unlock(&padctl->lock);

	return 0;
}

static int tegra186_hsic_phy_exit(struct phy *phy)
{
	struct tegra_padctl *padctl = phy_get_drvdata(phy);
	int port = hsic_phy_to_port(phy);

	mutex_lock(&padctl->lock);

	dev_dbg(padctl->dev, "phy exit HSIC port %d\n", port);

	mutex_unlock(&padctl->lock);

	return 0;
}

static const struct phy_ops hsic_phy_ops = {
	.init = tegra186_hsic_phy_init,
	.exit = tegra186_hsic_phy_exit,
	.power_on = tegra186_hsic_phy_power_on,
	.power_off = tegra186_hsic_phy_power_off,
	.owner = THIS_MODULE,
};

static inline bool is_hsic_phy(struct phy *phy)
{
	return phy->ops == &hsic_phy_ops;
}

static void tegra_xusb_phy_mbox_work(struct work_struct *work)
{
	struct tegra_padctl *padctl = mbox_work_to_padctl(work);
	struct tegra_xusb_mbox_msg *msg = &padctl->mbox_req;
	struct tegra_xusb_mbox_msg resp;
	u32 ports;
	int ret = 0;

	dev_dbg(padctl->dev, "mailbox command %d\n", msg->cmd);
	resp.cmd = 0;
	switch (msg->cmd) {
	case MBOX_CMD_START_HSIC_IDLE:
	case MBOX_CMD_STOP_HSIC_IDLE:
		ports = msg->data >> (padctl->soc->hsic_port_offset + 1);
		resp.data = msg->data;
		resp.cmd = MBOX_CMD_ACK;
		if (msg->cmd == MBOX_CMD_START_HSIC_IDLE)
			tegra186_hsic_phy_pupd_set(padctl, 0, PUPD_IDLE);
		else
			tegra186_hsic_phy_pupd_set(padctl, 0, PUPD_DISABLE);
		break;
	default:
		break;
	}

	if (resp.cmd) {
		ret = mbox_send_message(padctl->mbox_chan, &resp);
		if (ret < 0)
			dev_err(padctl->dev, "mbox_send_message failed\n");
	}
}

static bool is_phy_mbox_message(u32 cmd)
{
	switch (cmd) {
	case MBOX_CMD_START_HSIC_IDLE:
	case MBOX_CMD_STOP_HSIC_IDLE:
		return true;
	default:
		return false;
	}
}

static void tegra_xusb_phy_mbox_rx(struct mbox_client *cl, void *data)
{
	struct tegra_padctl *padctl = dev_get_drvdata(cl->dev);
	struct tegra_xusb_mbox_msg *msg = data;

	if (is_phy_mbox_message(msg->cmd)) {
		padctl->mbox_req = *msg;
		schedule_work(&padctl->mbox_req_work);
	}
}

static struct phy *tegra186_padctl_xlate(struct device *dev,
					 struct of_phandle_args *args)
{
	struct tegra_padctl *padctl = dev_get_drvdata(dev);
	unsigned int index = args->args[0];
	unsigned int phy_index;
	struct phy *phy = NULL;

	if (args->args_count <= 0)
		return ERR_PTR(-EINVAL);

	dev_dbg(dev, "%s index %d\n", __func__, index);

	if ((index >= TEGRA_PADCTL_PHY_USB3_BASE) &&
		(index < TEGRA_PADCTL_PHY_USB3_BASE + 16)) {

		phy_index = index - TEGRA_PADCTL_PHY_USB3_BASE;
		if (phy_index < TEGRA_USB3_PHYS)
			phy = padctl->usb3_phys[phy_index];

	} else if ((index >= TEGRA_PADCTL_PHY_UTMI_BASE) &&
		(index < TEGRA_PADCTL_PHY_UTMI_BASE + 16)) {

		phy_index = index - TEGRA_PADCTL_PHY_UTMI_BASE;
		if (phy_index < TEGRA_UTMI_PHYS)
			phy = padctl->utmi_phys[phy_index];

	} else if ((index >= TEGRA_PADCTL_PHY_HSIC_BASE) &&
		(index < TEGRA_PADCTL_PHY_HSIC_BASE + 16)) {

		phy_index = index - TEGRA_PADCTL_PHY_HSIC_BASE;
		if (phy_index < TEGRA_HSIC_PHYS)
			phy = padctl->hsic_phys[phy_index];

	} else if ((index >= TEGRA_PADCTL_PHY_CDP_BASE) &&
		(index < TEGRA_PADCTL_PHY_CDP_BASE + 16)) {

		phy_index = index -  TEGRA_PADCTL_PHY_CDP_BASE;
		if (phy_index < TEGRA_CDP_PHYS)
			phy = padctl->cdp_phys[phy_index];
		padctl->cdp_used = true;
	}

	return (phy) ? phy : ERR_PTR(-EINVAL);
}

static const struct pinctrl_pin_desc tegra186_pins[] = {
	PINCTRL_PIN(PIN_OTG_0,  "otg-0"),
	PINCTRL_PIN(PIN_OTG_1,  "otg-1"),
	PINCTRL_PIN(PIN_OTG_2,  "otg-2"),
	PINCTRL_PIN(PIN_HSIC_0, "hsic-0"),
	PINCTRL_PIN(PIN_USB3_0, "usb3-0"),
	PINCTRL_PIN(PIN_USB3_1, "usb3-1"),
	PINCTRL_PIN(PIN_USB3_2, "usb3-2"),
	PINCTRL_PIN(PIN_CDP_0,  "cdp-0"),
	PINCTRL_PIN(PIN_CDP_1,  "cdp-1"),
	PINCTRL_PIN(PIN_CDP_2,  "cdp-2"),
};


static const char * const tegra186_hsic_groups[] = {
	"hsic-0",
};

static const char * const tegra186_xusb_groups[] = {
	"otg-0",
	"otg-1",
	"otg-2",
	"cdp-0",
	"cdp-1",
	"cdp-2",
};

#define TEGRA186_FUNCTION(_name)					\
	{								\
		.name = #_name,						\
		.num_groups = ARRAY_SIZE(tegra186_##_name##_groups),	\
		.groups = tegra186_##_name##_groups,			\
	}

static struct tegra_padctl_function tegra186_functions[] = {
	TEGRA186_FUNCTION(hsic),
	TEGRA186_FUNCTION(xusb),
};

static const unsigned int tegra186_otg_functions[] = {
	TEGRA186_FUNC_XUSB
};

static const unsigned int tegra186_hsic_functions[] = {
	TEGRA186_FUNC_HSIC,
};

#define TEGRA186_PAD(_name, _offset, _shift, _mask, _funcs)	\
	{								\
		.name = _name,						\
		.offset = _offset,					\
		.shift = _shift,					\
		.mask = _mask,						\
		.num_funcs = ARRAY_SIZE(tegra186_##_funcs##_functions),	\
		.funcs = tegra186_##_funcs##_functions,			\
	}

static const struct tegra_padctl_pad tegra186_pads[] = {
	TEGRA186_PAD("otg-0",  0x004,  0, 0x3, otg),
	TEGRA186_PAD("otg-1",  0x004,  2, 0x3, otg),
	TEGRA186_PAD("otg-2",  0x004,  4, 0x3, otg),
	TEGRA186_PAD("hsic-0", 0x004, 20, 0x1, hsic),

};

static const char * const tegra186_supply_names[] = {
	"avdd_usb",		/* 3.3V, vddp_usb */
	"vclamp_usb",		/* 1.8V, vclamp_usb_init */
	"avdd_pll_erefeut",	/* 1.8V, pll_utmip_avdd */
};

static const struct tegra_padctl_soc tegra186_soc = {
	.num_pins = ARRAY_SIZE(tegra186_pins),
	.pins = tegra186_pins,
	.num_functions = ARRAY_SIZE(tegra186_functions),
	.functions = tegra186_functions,
	.num_pads = ARRAY_SIZE(tegra186_pads),
	.pads = tegra186_pads,
	.hsic_port_offset = 6,
	.supply_names = tegra186_supply_names,
	.num_supplies = ARRAY_SIZE(tegra186_supply_names),
	.num_oc_pins = 2,
};

static const struct of_device_id tegra_padctl_of_match[] = {
	{.compatible = "nvidia,tegra186-xusb-padctl", .data = &tegra186_soc},
	{ }
};
MODULE_DEVICE_TABLE(of, tegra_padctl_of_match);

static int tegra_xusb_read_fuse_calibration(struct tegra_padctl *padctl)
{
	unsigned int i;
	u32 reg;

	tegra_fuse_readl(FUSE_SKU_USB_CALIB_0, &reg);
	dev_info(padctl->dev, "FUSE_SKU_USB_CALIB_0 0x%x\n", reg);

	for (i = 0; i < TEGRA_UTMI_PHYS; i++) {
		padctl->calib.hs_curr_level[i] =
			(reg >> HS_CURR_LEVEL_PADX_SHIFT(i)) &
			HS_CURR_LEVEL_PAD_MASK;
	}
	padctl->calib.hs_squelch = (reg >> HS_SQUELCH_SHIFT) & HS_SQUELCH_MASK;
	padctl->calib.hs_term_range_adj = (reg >> HS_TERM_RANGE_ADJ_SHIFT) &
					HS_TERM_RANGE_ADJ_MASK;

	tegra_fuse_readl(FUSE_USB_CALIB_EXT_0, &reg);
	dev_info(padctl->dev, "FUSE_USB_CALIB_EXT_0 0x%x\n", reg);

	padctl->calib.rpd_ctrl = (reg >> RPD_CTRL_SHIFT) & RPD_CTRL_MASK;

	return 0;
}

static int tegra_xusb_select_vbus_en_state(struct tegra_padctl *padctl,
			int pin, bool tristate)
{
	int err;

	if (tristate)
		err = pinctrl_select_state(
				padctl->oc_pinctrl,
				padctl->oc_tristate_enable[pin]);
	else
		err = pinctrl_select_state(
				padctl->oc_pinctrl,
				padctl->oc_passthrough_enable[pin]);

	if (err < 0) {
		dev_err(padctl->dev,
			"setting pin %d OC state failed: %d\n",
			pin, err);
	}
	return err;
}

static int tegra_xusb_setup_usb(struct tegra_padctl *padctl)
{
	struct phy *phy;
	unsigned int i;

	for (i = 0; i < TEGRA_USB3_PHYS; i++) {
		if (padctl->usb3_ports[i].port_cap == CAP_DISABLED)
			continue;
		if (padctl->host_mode_phy_disabled &&
			(padctl->usb3_ports[i].port_cap == HOST_ONLY))
			continue; /* no mailbox support */

		phy = devm_phy_create(padctl->dev, NULL, &usb3_phy_ops);
		if (IS_ERR(phy))
			return PTR_ERR(phy);

		padctl->usb3_phys[i] = phy;
		phy_set_drvdata(phy, padctl);
	}

	for (i = 0; i < TEGRA_UTMI_PHYS; i++) {
		char reg_name[sizeof("vbus-N")];

		if (padctl->host_mode_phy_disabled &&
			(padctl->utmi_ports[i].port_cap == HOST_ONLY))
			continue; /* no mailbox support */

		sprintf(reg_name, "vbus-%d", i);
		padctl->vbus[i] = devm_regulator_get_optional(padctl->dev,
							    reg_name);
		if (IS_ERR(padctl->vbus[i])) {
			if (PTR_ERR(padctl->vbus[i]) == -EPROBE_DEFER)
				return -EPROBE_DEFER;

			padctl->vbus[i] = NULL;
		}

		phy = devm_phy_create(padctl->dev, NULL, &utmi_phy_ops);
		if (IS_ERR(phy))
			return PTR_ERR(phy);

		padctl->utmi_phys[i] = phy;
		phy_set_drvdata(phy, padctl);
	}

	if (padctl->host_mode_phy_disabled)
		goto skip_hsic; /* no mailbox support */

	padctl->vddio_hsic = devm_regulator_get(padctl->dev, "vddio-hsic");
	if (IS_ERR(padctl->vddio_hsic))
		return PTR_ERR(padctl->vddio_hsic);

	for (i = 0; i < TEGRA_HSIC_PHYS; i++) {
		phy = devm_phy_create(padctl->dev, NULL, &hsic_phy_ops);
		if (IS_ERR(phy))
			return PTR_ERR(phy);

		padctl->hsic_phys[i] = phy;
		phy_set_drvdata(phy, padctl);
	}

	for (i = 0; i < TEGRA_CDP_PHYS; i++) {
		phy = devm_phy_create(padctl->dev, NULL, &cdp_phy_ops);
		if (IS_ERR(phy))
			return PTR_ERR(phy);

		padctl->cdp_phys[i] = phy;
		phy_set_drvdata(phy, padctl);
	}

skip_hsic:
	return 0;
}

static int tegra_xusb_setup_oc(struct tegra_padctl *padctl)
{
	int i;
	bool oc_enabled = false;

	/* check oc_pin properties from USB3 and UTMI phy */
	for (i = 0; i < TEGRA_USB3_PHYS; i++) {
		if (padctl->usb3_ports[i].oc_pin >= 0) {
			oc_enabled = true;
			break;
		}
	}
	for (i = 0; i < TEGRA_UTMI_PHYS; i++) {
		if (padctl->utmi_ports[i].oc_pin >= 0) {
			oc_enabled = true;
			break;
		}
	}
	if (!oc_enabled) {
		dev_dbg(padctl->dev, "No OC pin defined for USB3/UTMI phys\n");
		return -EINVAL;
	}

	/* getting pinctrl for controlling OC pins */
	padctl->oc_pinctrl = devm_pinctrl_get(padctl->dev);
	if (IS_ERR_OR_NULL(padctl->oc_pinctrl)) {
		dev_info(padctl->dev, "Missing OC pinctrl device: %ld\n",
			 PTR_ERR(padctl->oc_pinctrl));
		return PTR_ERR(padctl->oc_pinctrl);
	}

	/* OC enable state */
	padctl->oc_tristate_enable = devm_kcalloc(padctl->dev,
			padctl->soc->num_oc_pins,
			sizeof(struct pinctrl_state *), GFP_KERNEL);
	if (!padctl->oc_tristate_enable)
		return -ENOMEM;
	for (i = 0; i < padctl->soc->num_oc_pins; i++) {
		char state_name[sizeof("vbus_enX_sfio_tristate")];

		sprintf(state_name, "vbus_en%d_sfio_tristate", i);
		padctl->oc_tristate_enable[i] = pinctrl_lookup_state(
				padctl->oc_pinctrl, state_name);
		if (IS_ERR(padctl->oc_tristate_enable[i])) {
			dev_info(padctl->dev,
				 "Missing OC pin %d pinctrl state %s: %ld\n",
				 i, state_name,
				 PTR_ERR(padctl->oc_tristate_enable[i]));
			return PTR_ERR(padctl->oc_tristate_enable[i]);
		}
	}

	/* OC enable passthrough state */
	padctl->oc_passthrough_enable = devm_kcalloc(padctl->dev,
			padctl->soc->num_oc_pins,
			sizeof(struct pinctrl_state *), GFP_KERNEL);
	if (!padctl->oc_passthrough_enable)
		return -ENOMEM;
	for (i = 0; i < padctl->soc->num_oc_pins; i++) {
		char state_name[sizeof("vbus_enX_sfio_passthrough")];

		sprintf(state_name, "vbus_en%d_sfio_passthrough", i);
		padctl->oc_passthrough_enable[i] = pinctrl_lookup_state(
				padctl->oc_pinctrl, state_name);
		if (IS_ERR(padctl->oc_passthrough_enable[i])) {
			dev_info(padctl->dev,
				 "Missing OC pin %d pinctrl state %s: %ld\n",
				 i, state_name,
				 PTR_ERR(padctl->oc_passthrough_enable[i]));
			return PTR_ERR(padctl->oc_passthrough_enable[i]);
		}
	}

	/* OC disable state */
	padctl->oc_disable = devm_kcalloc(padctl->dev,
			padctl->soc->num_oc_pins,
			sizeof(struct pinctrl_state *), GFP_KERNEL);
	if (!padctl->oc_disable)
		return -ENOMEM;
	for (i = 0; i < padctl->soc->num_oc_pins; i++) {
		char state_name[sizeof("vbus_enX_default")];

		sprintf(state_name, "vbus_en%d_default", i);
		padctl->oc_disable[i] = pinctrl_lookup_state(
				padctl->oc_pinctrl, state_name);
		if (IS_ERR(padctl->oc_disable[i])) {
			dev_info(padctl->dev,
				 "Missing OC pin %d pinctrl state %s: %ld\n",
				 i, state_name,
				 PTR_ERR(padctl->oc_disable[i]));
			return PTR_ERR(padctl->oc_disable[i]);
		}
	}

	return 0;
}

#ifdef DEBUG
#define reg_dump(_dev, _base, _reg) \
	dev_dbg(_dev, "%s @%x = 0x%x\n", #_reg, _reg, ioread32(_base + _reg))
#else
#define reg_dump(_dev, _base, _reg)	do {} while (0)
#endif

/* initializations to be done at cold boot and SC7 exit */
static void tegra186_padctl_init(struct tegra_padctl *padctl)
{
	int i;
	u32 reg;

	for (i = 0; i < TEGRA_UTMI_PHYS; i++) {
		reg = padctl_readl(padctl, USB2_BATTERY_CHRG_OTGPADX_CTL1(i));
		reg |= PD_VREG;
		padctl_writel(padctl, reg, USB2_BATTERY_CHRG_OTGPADX_CTL1(i));

		if (padctl->utmi_ports[i].port_cap == CAP_DISABLED) {
			reg = ao_readl(padctl, XUSB_AO_UTMIP_PAD_CFG(i));
			reg |= (E_DPD_OVRD_EN | E_DPD_OVRD_VAL);
			ao_writel(padctl, reg, XUSB_AO_UTMIP_PAD_CFG(i));
		}
	}
}

static void tegra186_padctl_save(struct tegra_padctl *padctl)
{
	padctl->padctl_context.vbus_id = padctl_readl(padctl, USB2_VBUS_ID);
	padctl->padctl_context.usb2_pad_mux =
				padctl_readl(padctl, XUSB_PADCTL_USB2_PAD_MUX);
	padctl->padctl_context.usb2_port_cap =
				padctl_readl(padctl, XUSB_PADCTL_USB2_PORT_CAP);
	padctl->padctl_context.ss_port_cap =
				padctl_readl(padctl, XUSB_PADCTL_SS_PORT_CAP);
}

static void tegra186_padctl_restore(struct tegra_padctl *padctl)
{
	padctl_writel(padctl, padctl->padctl_context.usb2_pad_mux,
		      XUSB_PADCTL_USB2_PAD_MUX);
	padctl_writel(padctl, padctl->padctl_context.usb2_port_cap,
		      XUSB_PADCTL_USB2_PORT_CAP);
	padctl_writel(padctl, padctl->padctl_context.ss_port_cap,
		      XUSB_PADCTL_SS_PORT_CAP);
	padctl_writel(padctl, padctl->padctl_context.vbus_id, USB2_VBUS_ID);
}

static int tegra186_padctl_suspend(struct device *dev)
{
	struct tegra_padctl *padctl = dev_get_drvdata(dev);

	dev_dbg(dev, "%s\n", __func__);

	tegra186_padctl_save(padctl);

	return 0;
}

static int tegra186_padctl_resume(struct device *dev)
{
	struct tegra_padctl *padctl = dev_get_drvdata(dev);

	dev_dbg(dev, "%s\n", __func__);

	tegra186_padctl_init(padctl);
	tegra186_padctl_restore(padctl);

	return 0;
}

static const struct dev_pm_ops tegra186_padctl_pm_ops = {
	.suspend_noirq = tegra186_padctl_suspend,
	.resume_noirq = tegra186_padctl_resume,
};

static int tegra186_padctl_probe(struct platform_device *pdev)
{
	struct tegra_padctl *padctl;
	const struct of_device_id *match;
	struct device *dev = &pdev->dev;
	struct resource *res;
	int err;
	int i;

	padctl = devm_kzalloc(dev, sizeof(*padctl), GFP_KERNEL);
	if (!padctl)
		return -ENOMEM;

	platform_set_drvdata(pdev, padctl);
	mutex_init(&padctl->lock);
	padctl->dev = dev;

	match = of_match_node(tegra_padctl_of_match, pdev->dev.of_node);
	if (!match)
		return -ENODEV;
	padctl->soc = match->data;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "padctl");
	padctl->padctl_regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(padctl->padctl_regs))
		return PTR_ERR(padctl->padctl_regs);
	dev_info(dev, "padctl mmio start %pa end %pa\n",
		 &res->start, &res->end);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ao");
	padctl->ao_regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(padctl->ao_regs))
		return PTR_ERR(padctl->ao_regs);
	dev_info(dev, "ao mmio start %pa end %pa\n", &res->start, &res->end);

	padctl->prod_list = devm_tegra_prod_get(&pdev->dev);
	if (IS_ERR(padctl->prod_list)) {
		dev_warn(&pdev->dev, "Prod-settings not available\n");
		padctl->prod_list = NULL;
	}

	if (tegra_platform_is_silicon()) {
		err = tegra_xusb_read_fuse_calibration(padctl);
		if (err < 0)
			return err;
	}

	/* overcurrent disabled by default */
	for (i = 0; i < TEGRA_USB3_PHYS; i++)
		padctl->usb3_ports[i].oc_pin = -1;
	for (i = 0; i < TEGRA_UTMI_PHYS; i++)
		padctl->utmi_ports[i].oc_pin = -1;

	padctl->padctl_rst = devm_reset_control_get(dev, "padctl_rst");
	if (IS_ERR(padctl->padctl_rst)) {
		dev_err(padctl->dev, "failed to get padctl reset\n");
		return PTR_ERR(padctl->padctl_rst);
	}

	padctl->xusb_clk = devm_clk_get(dev, "xusb_clk");
	if (IS_ERR(padctl->xusb_clk)) {
		dev_err(dev, "failed to get xusb_clk clock\n");
		return PTR_ERR(padctl->xusb_clk);
	}

	padctl->utmipll = devm_clk_get(dev, "utmipll");
	if (IS_ERR(padctl->utmipll)) {
		dev_err(dev, "failed to get utmipll clock\n");
		return PTR_ERR(padctl->utmipll);
	}

	padctl->usb2_trk_clk = devm_clk_get(dev, "usb2_trk");
	if (IS_ERR(padctl->usb2_trk_clk)) {
		dev_err(dev, "failed to get usb2_trk clock\n");
		return PTR_ERR(padctl->usb2_trk_clk);
	}

	padctl->hsic_trk_clk = devm_clk_get(dev, "hsic_trk");
	if (IS_ERR(padctl->hsic_trk_clk)) {
		dev_err(dev, "failed to get hsic_trk clock\n");
		return PTR_ERR(padctl->hsic_trk_clk);
	}

	err = tegra186_padctl_regulators_init(padctl);
	if (err < 0)
		return err;

	err = regulator_bulk_enable(padctl->soc->num_supplies,
				    padctl->supplies);
	if (err) {
		dev_err(dev, "failed to enable regulators %d\n", err);
		return err;
	}

	err = clk_prepare_enable(padctl->xusb_clk);
	if (err) {
		dev_err(dev, "failed to enable xusb_clk %d\n", err);
		goto disable_regulators;
	}

	err = clk_prepare_enable(padctl->utmipll);
	if (err) {
		dev_err(dev, "failed to enable UTMIPLL %d\n", err);
		goto disable_xusb_clk;
	}

	err = clk_prepare_enable(padctl->usb2_trk_clk);
	if (err) {
		dev_err(dev, "failed to enable USB2 tracking clock %d\n",
			err);
		goto disable_utmipll;
	}

	err = reset_control_deassert(padctl->padctl_rst);
	if (err) {
		dev_err(dev, "failed to deassert padctl_rst %d\n", err);
		goto disable_usb2_trk;
	}

	memset(&padctl->desc, 0, sizeof(padctl->desc));
	padctl->desc.name = dev_name(dev);
	padctl->desc.pins = padctl->soc->pins;
	padctl->desc.npins = padctl->soc->num_pins;
	padctl->desc.pctlops = &tegra_xusb_padctl_pinctrl_ops;
	padctl->desc.pmxops = &tegra186_padctl_pinmux_ops;
	padctl->desc.confops = &tegra_padctl_pinconf_ops;
	padctl->desc.owner = THIS_MODULE;

	padctl->pinctrl = pinctrl_register(&padctl->desc, &pdev->dev, padctl);
	if (!padctl->pinctrl) {
		dev_err(&pdev->dev, "failed to register pinctrl\n");
		err = -ENODEV;
		goto assert_padctl_rst;
	}

	INIT_WORK(&padctl->mbox_req_work, tegra_xusb_phy_mbox_work);
	padctl->mbox_client.dev = dev;
	padctl->mbox_client.tx_block = true;
	padctl->mbox_client.tx_tout = 0;
	padctl->mbox_client.rx_callback = tegra_xusb_phy_mbox_rx;
	padctl->mbox_chan = mbox_request_channel(&padctl->mbox_client, 0);
	if (IS_ERR(padctl->mbox_chan)) {
		err = PTR_ERR(padctl->mbox_chan);
		if (err == -EPROBE_DEFER) {
			dev_info(&pdev->dev, "mailbox is not ready yet\n");
			goto unregister;
		} else {
			dev_warn(&pdev->dev,
				 "failed to get mailbox, USB Host PHY support disabled\n");
			padctl->host_mode_phy_disabled = true;
		}
	}

	err = tegra_xusb_setup_usb(padctl);
	if (err)
		goto free_mailbox;

	tegra186_padctl_init(padctl);

	err = tegra_xusb_setup_oc(padctl);
	if (err)
		padctl->oc_pinctrl = NULL;
	else
		dev_info(&pdev->dev, "VBUS over-current detection enabled\n");

	/* when oc_pinctrl is not NULL, over-current detection is enabled
	 * for at least one port */
	if (padctl->oc_pinctrl)
		/* switch VBUS pin states to enable OC */
		for (i = 0; i < TEGRA_UTMI_PHYS; i++) {
			int ocpin = padctl->utmi_ports[i].oc_pin;
			bool isotg =
				(padctl->utmi_ports[i].port_cap == OTG);
			if (ocpin >= 0) {
				/* this OC pin is in use, enable the pin
				 * as SFIO input pin for OC detection,
				 * for OTG port, the default state is
				 * device mode and VBUS off.
				 */
				err = tegra_xusb_select_vbus_en_state(
						padctl, ocpin, !isotg);
				if (err < 0)
					goto restore_oc_pin;
			}
		}

	padctl->provider = devm_of_phy_provider_register(dev,
					tegra186_padctl_xlate);
	if (IS_ERR(padctl->provider)) {
		err = PTR_ERR(padctl->provider);
		dev_err(&pdev->dev, "failed to register PHYs: %d\n", err);
		goto restore_oc_pin;
	}

	err = sysfs_create_group(&pdev->dev.kobj, &padctl_attr_group);
	if (err) {
		dev_err(&pdev->dev, "cannot create sysfs group: %d\n", err);
		goto restore_oc_pin;
	}

	return 0;

restore_oc_pin:
	if (padctl->oc_pinctrl)
		for (i--; i >= 0; i--) {
			err = pinctrl_select_state(padctl->oc_pinctrl,
						     padctl->oc_disable[i]);
			if (err < 0)
				dev_err(dev,
					"set pin %d OC disable failed: %d\n",
					i, err);
		}


free_mailbox:
	if (!IS_ERR(padctl->mbox_chan)) {
		cancel_work_sync(&padctl->mbox_req_work);
		mbox_free_channel(padctl->mbox_chan);
	}
unregister:
	pinctrl_unregister(padctl->pinctrl);
assert_padctl_rst:
	reset_control_assert(padctl->padctl_rst);
disable_usb2_trk:
	clk_disable_unprepare(padctl->usb2_trk_clk);
disable_utmipll:
	clk_disable_unprepare(padctl->utmipll);
disable_xusb_clk:
	clk_disable_unprepare(padctl->xusb_clk);
disable_regulators:
	regulator_bulk_disable(padctl->soc->num_supplies, padctl->supplies);

	return err;
}

static int tegra186_padctl_remove(struct platform_device *pdev)
{
	struct tegra_padctl *padctl = platform_get_drvdata(pdev);
	int i;
	int err;

	/* switch all VBUS_ENx pins back to default state */
	if (padctl->oc_pinctrl)
		for (i = 0; i < padctl->soc->num_oc_pins; i++) {
			err = pinctrl_select_state(padctl->oc_pinctrl,
					     padctl->oc_disable[i]);
			if (err < 0)
				dev_err(&pdev->dev,
					"set pin %d OC disable failed: %d\n",
					i, err);
		}

	sysfs_remove_group(&pdev->dev.kobj, &padctl_attr_group);

	if (!IS_ERR(padctl->mbox_chan)) {
		cancel_work_sync(&padctl->mbox_req_work);
		mbox_free_channel(padctl->mbox_chan);
	}

	pinctrl_unregister(padctl->pinctrl);

	reset_control_assert(padctl->padctl_rst);
	clk_disable_unprepare(padctl->usb2_trk_clk);
	clk_disable_unprepare(padctl->utmipll);
	clk_disable_unprepare(padctl->xusb_clk);

	regulator_bulk_disable(padctl->soc->num_supplies, padctl->supplies);
	padctl->cdp_used = false;

	return 0;
}

static struct platform_driver tegra186_padctl_driver = {
	.driver = {
		.name = "tegra186-padctl",
		.of_match_table = tegra_padctl_of_match,
		.pm = &tegra186_padctl_pm_ops,
	},
	.probe = tegra186_padctl_probe,
	.remove = tegra186_padctl_remove,
};
module_platform_driver(tegra186_padctl_driver);

/* Tegra Generic PHY Extensions */
int tegra18x_phy_xusb_enable_sleepwalk(struct phy *phy,
				    enum usb_device_speed speed)
{
	struct tegra_padctl *padctl = phy_get_drvdata(phy);
	int port;

	if (is_utmi_phy(phy)) {
		port = utmi_phy_to_port(phy);
		if (port < 0)
			return -EINVAL;
		return tegra186_utmi_phy_enable_sleepwalk(padctl, port, speed);
	} else if (is_hsic_phy(phy)) {
		port = hsic_phy_to_port(phy);
		if (port < 0)
			return -EINVAL;
		return tegra186_hsic_phy_enable_sleepwalk(padctl, port);
	} else if (is_usb3_phy(phy)) {
		port = usb3_phy_to_port(phy);
		if (port < 0)
			return -EINVAL;
		return tegra186_usb3_phy_enable_wakelogic(padctl, port);
	} else
		return -EINVAL;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_enable_sleepwalk);

int tegra18x_phy_xusb_disable_sleepwalk(struct phy *phy)
{
	struct tegra_padctl *padctl = phy_get_drvdata(phy);
	int port;

	if (is_utmi_phy(phy)) {
		port = utmi_phy_to_port(phy);
		if (port < 0)
			return -EINVAL;
		return tegra186_utmi_phy_disable_sleepwalk(padctl, port);
	} else if (is_hsic_phy(phy)) {
		port = hsic_phy_to_port(phy);
		if (port < 0)
			return -EINVAL;
		return tegra186_hsic_phy_disable_sleepwalk(padctl, port);
	} else if (is_usb3_phy(phy)) {
		port = usb3_phy_to_port(phy);
		if (port < 0)
			return -EINVAL;
		return tegra186_usb3_phy_disable_wakelogic(padctl, port);
	} else
		return -EINVAL;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_disable_sleepwalk);

static int tegra186_padctl_vbus_override(struct tegra_padctl *padctl,
					 bool on)
{
	u32 reg;

	reg = padctl_readl(padctl, USB2_VBUS_ID);
	if (on)
		reg |= VBUS_OVERRIDE;
	else
		reg &= ~VBUS_OVERRIDE;
	padctl_writel(padctl, reg, USB2_VBUS_ID);

	return 0;
}

int tegra18x_phy_xusb_set_vbus_override(struct phy *phy)
{
	struct tegra_padctl *padctl;

	if (!phy)
		return 0;

	padctl = phy_get_drvdata(phy);

	return tegra186_padctl_vbus_override(padctl, true);
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_set_vbus_override);

int tegra18x_phy_xusb_clear_vbus_override(struct phy *phy)
{
	struct tegra_padctl *padctl;

	if (!phy)
		return 0;

	padctl = phy_get_drvdata(phy);

	return tegra186_padctl_vbus_override(padctl, false);
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_clear_vbus_override);

static int tegra186_padctl_id_override(struct tegra_padctl *padctl,
					 bool grounded)
{
	u32 reg;

	reg = padctl_readl(padctl, USB2_VBUS_ID);
	if (grounded) {
		reg &= ~ID_OVERRIDE(~0);
		reg |= ID_OVERRIDE_GROUNDED;
	} else {
		reg &= ~ID_OVERRIDE(~0);
		reg |= ID_OVERRIDE_FLOATING;
	}
	padctl_writel(padctl, reg, USB2_VBUS_ID);

	return 0;
}

int tegra18x_phy_xusb_set_id_override(struct phy *phy)
{
	struct tegra_padctl *padctl;

	if (!phy)
		return 0;

	padctl = phy_get_drvdata(phy);

	return tegra186_padctl_id_override(padctl, true);
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_set_id_override);

int tegra18x_phy_xusb_clear_id_override(struct phy *phy)
{
	struct tegra_padctl *padctl;

	if (!phy)
		return 0;

	padctl = phy_get_drvdata(phy);

	return tegra186_padctl_id_override(padctl, false);
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_clear_id_override);

static enum tegra_xusb_vbus_rid tegra_phy_xusb_parse_rid(u32 rid_value)
{
	rid_value &= RID_MASK;

	if (rid_value == IDDIG)
		return VBUS_ID_RID_FLOAT;
	else if (rid_value == IDDIG_A)
		return VBUS_ID_RID_A;
	else if (rid_value == IDDIG_B)
		return VBUS_ID_RID_B;
	else if (rid_value == IDDIG_C)
		return VBUS_ID_RID_C;
	else if (rid_value == 0)
		return VBUS_ID_RID_GND;

	return VBUS_ID_RID_UNDEFINED;
}

bool tegra18x_phy_xusb_has_otg_cap(struct phy *phy)
{
	struct tegra_padctl *padctl;

	if (!phy)
		return false;

	padctl = phy_get_drvdata(phy);
	if (is_utmi_phy(phy)) {
		if ((padctl->utmi_otg_port_base_1) &&
		    padctl->utmi_phys[padctl->utmi_otg_port_base_1 - 1] == phy)
			return true;
	} else if (is_usb3_phy(phy)) {
		if ((padctl->usb3_otg_port_base_1) &&
		    padctl->usb3_phys[padctl->usb3_otg_port_base_1 - 1] == phy)
			return true;
	}

	return false;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_has_otg_cap);

static int tegra186_usb3_phy_set_wake(struct tegra_padctl *padctl,
				      int port, bool enable)
{
	u32 reg;

	mutex_lock(&padctl->lock);
	if (enable) {
		dev_dbg(padctl->dev, "enable USB3 port %d wake\n", port);

		reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM);
		reg &= ~ALL_WAKE_EVENTS;
		reg |= SS_PORT_WAKEUP_EVENT(port);
		padctl_writel(padctl, reg, XUSB_PADCTL_ELPG_PROGRAM);

		usleep_range(10, 20);

		reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM);
		reg &= ~ALL_WAKE_EVENTS;
		reg |= SS_PORT_WAKE_INTERRUPT_ENABLE(port);
		padctl_writel(padctl, reg, XUSB_PADCTL_ELPG_PROGRAM);
	} else {
		dev_dbg(padctl->dev, "disable USB3 port %d wake\n", port);

		reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM);
		reg &= ~ALL_WAKE_EVENTS;
		reg &= ~SS_PORT_WAKE_INTERRUPT_ENABLE(port);
		padctl_writel(padctl, reg, XUSB_PADCTL_ELPG_PROGRAM);

		usleep_range(10, 20);

		reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM);
		reg &= ~ALL_WAKE_EVENTS;
		reg |= SS_PORT_WAKEUP_EVENT(port);
		padctl_writel(padctl, reg, XUSB_PADCTL_ELPG_PROGRAM);
	}
	mutex_unlock(&padctl->lock);

	return 0;
}

static int tegra186_utmi_phy_set_wake(struct tegra_padctl *padctl,
				      int port, bool enable)
{
	u32 reg;

	mutex_lock(&padctl->lock);
	if (enable) {
		dev_dbg(padctl->dev, "enable UTMI port %d wake\n", port);

		reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM);
		reg &= ~ALL_WAKE_EVENTS;
		reg |= USB2_PORT_WAKEUP_EVENT(port);
		padctl_writel(padctl, reg, XUSB_PADCTL_ELPG_PROGRAM);

		usleep_range(10, 20);

		reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM);
		reg &= ~ALL_WAKE_EVENTS;
		reg |= USB2_PORT_WAKE_INTERRUPT_ENABLE(port);
		padctl_writel(padctl, reg, XUSB_PADCTL_ELPG_PROGRAM);

	} else {
		dev_dbg(padctl->dev, "disable UTMI port %d wake\n", port);

		reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM);
		reg &= ~ALL_WAKE_EVENTS;
		reg &= ~USB2_PORT_WAKE_INTERRUPT_ENABLE(port);
		padctl_writel(padctl, reg, XUSB_PADCTL_ELPG_PROGRAM);

		usleep_range(10, 20);

		reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM);
		reg &= ~ALL_WAKE_EVENTS;
		reg |= USB2_PORT_WAKEUP_EVENT(port);
		padctl_writel(padctl, reg, XUSB_PADCTL_ELPG_PROGRAM);
	}
	mutex_unlock(&padctl->lock);

	return 0;
}

static int tegra186_hsic_phy_set_wake(struct tegra_padctl *padctl,
				      int port, bool enable)
{
	u32 reg;

	mutex_lock(&padctl->lock);
	if (enable) {
		dev_dbg(padctl->dev, "enable HSIC port %d wake\n", port);

		reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM);
		reg &= ~ALL_WAKE_EVENTS;
		reg |= USB2_HSIC_PORT_WAKEUP_EVENT(port);
		padctl_writel(padctl, reg, XUSB_PADCTL_ELPG_PROGRAM);

		usleep_range(10, 20);

		reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM);
		reg |= USB2_HSIC_PORT_WAKE_INTERRUPT_ENABLE(port);
		padctl_writel(padctl, reg, XUSB_PADCTL_ELPG_PROGRAM);

	} else {
		dev_dbg(padctl->dev, "disable HSIC port %d wake\n", port);

		reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM);
		reg &= ~ALL_WAKE_EVENTS;
		reg &= ~USB2_HSIC_PORT_WAKE_INTERRUPT_ENABLE(port);
		padctl_writel(padctl, reg, XUSB_PADCTL_ELPG_PROGRAM);

		usleep_range(10, 20);

		reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM);
		reg &= ~ALL_WAKE_EVENTS;
		reg |= USB2_HSIC_PORT_WAKEUP_EVENT(port);
		padctl_writel(padctl, reg, XUSB_PADCTL_ELPG_PROGRAM);
	}
	mutex_unlock(&padctl->lock);

	return 0;
}

int tegra18x_phy_xusb_enable_wake(struct phy *phy)
{
	struct tegra_padctl *padctl;
	int port;

	if (!phy)
		return 0;

	padctl = phy_get_drvdata(phy);

	if (is_utmi_phy(phy)) {
		port = utmi_phy_to_port(phy);
		if (port < 0)
			return -EINVAL;
		return tegra186_utmi_phy_set_wake(padctl, port, true);
	} else if (is_hsic_phy(phy)) {
		port = hsic_phy_to_port(phy);
		if (port < 0)
			return -EINVAL;
		return tegra186_hsic_phy_set_wake(padctl, port, true);
	} else if (is_usb3_phy(phy)) {
		port = usb3_phy_to_port(phy);
		if (port < 0)
			return -EINVAL;
		return tegra186_usb3_phy_set_wake(padctl, port, true);
	} else
		return -EINVAL;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_enable_wake);

int tegra18x_phy_xusb_disable_wake(struct phy *phy)
{
	struct tegra_padctl *padctl;
	int port;

	if (!phy)
		return 0;

	padctl = phy_get_drvdata(phy);

	if (is_utmi_phy(phy)) {
		port = utmi_phy_to_port(phy);
		if (port < 0)
			return -EINVAL;
		return tegra186_utmi_phy_set_wake(padctl, port, false);
	} else if (is_hsic_phy(phy)) {
		port = hsic_phy_to_port(phy);
		if (port < 0)
			return -EINVAL;
		return tegra186_hsic_phy_set_wake(padctl, port, false);
	} else if (is_usb3_phy(phy)) {
		port = usb3_phy_to_port(phy);
		if (port < 0)
			return -EINVAL;
		return tegra186_usb3_phy_set_wake(padctl, port, false);
	}
		return -EINVAL;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_disable_wake);


static int tegra186_usb3_phy_remote_wake_detected(struct tegra_padctl *padctl,
						  int port)
{
	u32 reg;

	reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM);
	if ((reg & SS_PORT_WAKE_INTERRUPT_ENABLE(port)) &&
			(reg & SS_PORT_WAKEUP_EVENT(port)))
		return true;
	else
		return false;
}

static int tegra186_utmi_phy_remote_wake_detected(struct tegra_padctl *padctl,
						  int port)
{
	u32 reg;

	reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM);
	if ((reg & USB2_PORT_WAKE_INTERRUPT_ENABLE(port)) &&
			(reg & USB2_PORT_WAKEUP_EVENT(port)))
		return true;
	else
		return false;
}

static int tegra186_hsic_phy_remote_wake_detected(struct tegra_padctl *padctl,
						  int port)
{
	u32 reg;

	reg = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM);
	if ((reg & USB2_HSIC_PORT_WAKE_INTERRUPT_ENABLE(port)) &&
			(reg & USB2_HSIC_PORT_WAKEUP_EVENT(port)))
		return true;
	else
		return false;
}

int tegra18x_phy_xusb_remote_wake_detected(struct phy *phy)
{
	struct tegra_padctl *padctl;
	int port;

	if (!phy)
		return 0;

	padctl = phy_get_drvdata(phy);

	if (is_utmi_phy(phy)) {
		port = utmi_phy_to_port(phy);
		if (port < 0)
			return -EINVAL;
		return tegra186_utmi_phy_remote_wake_detected(padctl, port);
	} else if (is_hsic_phy(phy)) {
		port = hsic_phy_to_port(phy);
		if (port < 0)
			return -EINVAL;
		return tegra186_hsic_phy_remote_wake_detected(padctl, port);
	} else if (is_usb3_phy(phy)) {
		port = usb3_phy_to_port(phy);
		if (port < 0)
			return -EINVAL;
		return tegra186_usb3_phy_remote_wake_detected(padctl, port);
	}
		return -EINVAL;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_remote_wake_detected);


int tegra18x_phy_xusb_pretend_connected(struct phy *phy)
{
	struct tegra_padctl *padctl;
	int port;

	if (!phy)
		return 0;

	padctl = phy_get_drvdata(phy);

	/* applicable to HSIC only */
	if (is_hsic_phy(phy)) {
		port = hsic_phy_to_port(phy);
		if (port < 0)
			return -EINVAL;
		return tegra186_hsic_phy_pretend_connected(padctl, port);
	}

	return -EINVAL;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_pretend_connected);

void tegra18x_phy_xusb_set_dcd_debounce_time(struct phy *phy, u32 val)
{
	struct tegra_padctl *padctl;
	u32 reg;

	if (!phy)
		return;

	padctl = phy_get_drvdata(phy);

	reg = padctl_readl(padctl,
			XUSB_PADCTL_USB2_BATTERY_CHRG_TDCD_DBNC_TIMER_0);
	reg &= ~TDCD_DBNC(~0);
	reg |= TDCD_DBNC(val);
	padctl_writel(padctl, reg,
			XUSB_PADCTL_USB2_BATTERY_CHRG_TDCD_DBNC_TIMER_0);
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_set_dcd_debounce_time);

void tegra18x_phy_xusb_utmi_pad_charger_detect_on(struct phy *phy)
{
	struct tegra_padctl *padctl;
	int port;
	u32 reg;

	if (!phy)
		return;

	padctl = phy_get_drvdata(phy);
	port = utmi_phy_to_port(phy);

	/* power up necessary stuff */
	tegra18x_phy_xusb_utmi_pad_power_on(phy);

	reg = padctl_readl(padctl, XUSB_PADCTL_USB2_OTG_PADX_CTL0(port));
	reg &= ~USB2_OTG_PD_ZI;
	padctl_writel(padctl, reg, XUSB_PADCTL_USB2_OTG_PADX_CTL0(port));

	reg = padctl_readl(padctl, XUSB_PADCTL_USB2_OTG_PADX_CTL0(port));
	reg |= (USB2_OTG_PD2 | USB2_OTG_PD2_OVRD_EN);
	padctl_writel(padctl, reg, XUSB_PADCTL_USB2_OTG_PADX_CTL0(port));

	reg = padctl_readl(padctl, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
	reg &= ~PD_CHG;
	padctl_writel(padctl, reg, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));

	/* Set DP/DN Pull up/down to zero by default */
	reg = padctl_readl(padctl, USB2_BATTERY_CHRG_OTGPADX_CTL1(port));
	reg &= ~(USBOP_RPD_OVRD_VAL | USBOP_RPU_OVRD_VAL |
		 USBON_RPD_OVRD_VAL | USBON_RPU_OVRD_VAL);
	reg |= (USBOP_RPD_OVRD | USBOP_RPU_OVRD |
		USBON_RPD_OVRD | USBON_RPU_OVRD);
	padctl_writel(padctl, reg, USB2_BATTERY_CHRG_OTGPADX_CTL1(port));

	/* Disable DP/DN as src/sink */
	reg = padctl_readl(padctl, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
	reg &= ~(OP_SRC_EN | ON_SINK_EN |
		 ON_SRC_EN | OP_SINK_EN);
	padctl_writel(padctl, reg, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_utmi_pad_charger_detect_on);

void tegra18x_phy_xusb_utmi_pad_charger_detect_off(struct phy *phy)
{
	struct tegra_padctl *padctl;
	int port;
	u32 reg;

	if (!phy)
		return;

	padctl = phy_get_drvdata(phy);
	port = utmi_phy_to_port(phy);

	reg = padctl_readl(padctl, USB2_BATTERY_CHRG_OTGPADX_CTL1(port));
	reg &= ~(USBOP_RPD_OVRD | USBOP_RPU_OVRD |
		 USBON_RPD_OVRD | USBON_RPU_OVRD);
	padctl_writel(padctl, reg, USB2_BATTERY_CHRG_OTGPADX_CTL1(port));

	/* power down necessary stuff */
	reg = padctl_readl(padctl, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
	reg |= PD_CHG;
	padctl_writel(padctl, reg, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));

	reg = padctl_readl(padctl, XUSB_PADCTL_USB2_OTG_PADX_CTL0(port));
	reg &= ~(USB2_OTG_PD2 | USB2_OTG_PD2_OVRD_EN);
	padctl_writel(padctl, reg, XUSB_PADCTL_USB2_OTG_PADX_CTL0(port));

	tegra18x_phy_xusb_utmi_pad_power_down(phy);
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_utmi_pad_charger_detect_off);

void tegra18x_phy_xusb_utmi_pad_enable_detect_filters(struct phy *phy)
{
	struct tegra_padctl *padctl;
	int port;
	u32 reg;

	if (!phy)
		return;

	padctl = phy_get_drvdata(phy);
	port = utmi_phy_to_port(phy);

	reg = padctl_readl(padctl, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
	reg |= (VDCD_DET_FILTER_EN | VDAT_DET_FILTER_EN |
		ZIP_FILTER_EN | ZIN_FILTER_EN);
	padctl_writel(padctl, reg, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_utmi_pad_enable_detect_filters);

void tegra18x_phy_xusb_utmi_pad_disable_detect_filters(struct phy *phy)
{
	struct tegra_padctl *padctl;
	int port;
	u32 reg;

	if (!phy)
		return;

	padctl = phy_get_drvdata(phy);
	port = utmi_phy_to_port(phy);

	reg = padctl_readl(padctl, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
	reg &= ~(VDCD_DET_FILTER_EN | VDAT_DET_FILTER_EN |
		 ZIP_FILTER_EN | ZIN_FILTER_EN);
	padctl_writel(padctl, reg, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_utmi_pad_disable_detect_filters);

void tegra18x_phy_xusb_utmi_pad_set_protection_level(struct phy *phy, int level,
						  enum tegra_vbus_dir dir)
{
	struct tegra_padctl *padctl;
	int port;
	u32 reg;

	if (!phy)
		return;

	padctl = phy_get_drvdata(phy);
	port = utmi_phy_to_port(phy);

	reg = padctl_readl(padctl, USB2_BATTERY_CHRG_OTGPADX_CTL1(port));
	if (level < 0) {
		/* disable pad protection */
		reg |= PD_VREG;
		reg &= ~VREG_LEV(~0);
		reg &= ~VREG_DIR(~0);
	} else {
		reg &= ~PD_VREG;

		reg &= ~VREG_DIR(~0);
		if (padctl->utmi_ports[port].port_cap == HOST_ONLY ||
				dir == TEGRA_VBUS_SOURCE)
			reg |= VREG_DIR_OUT;
		else if (padctl->utmi_ports[port].port_cap == DEVICE_ONLY ||
				dir == TEGRA_VBUS_SINK)
			reg |= VREG_DIR_IN;
		reg &= ~VREG_LEV(~0);
		reg |= VREG_LEV(level);
	}
	padctl_writel(padctl, reg, USB2_BATTERY_CHRG_OTGPADX_CTL1(port));
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_utmi_pad_set_protection_level);

bool tegra18x_phy_xusb_utmi_pad_dcd(struct phy *phy)
{
	struct tegra_padctl *padctl;
	int port;
	u32 reg;
	int dcd_timeout_ms = 0;
	bool ret = false;

	if (!phy)
		return false;

	padctl = phy_get_drvdata(phy);
	port = utmi_phy_to_port(phy);

	/* data contact detection */
	/* Turn on IDP_SRC */
	reg = padctl_readl(padctl, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
	reg |= OP_I_SRC_EN;
	padctl_writel(padctl, reg, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));

	/* Turn on D- pull-down resistor */
	reg = padctl_readl(padctl, USB2_BATTERY_CHRG_OTGPADX_CTL1(port));
	reg |= USBON_RPD_OVRD_VAL;
	padctl_writel(padctl, reg, USB2_BATTERY_CHRG_OTGPADX_CTL1(port));

	/* Wait for TDCD_DBNC */
	usleep_range(10000, 120000);

	while (dcd_timeout_ms < TDCD_TIMEOUT_MS) {
		reg = padctl_readl(padctl,
				   USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
		if (reg & DCD_DETECTED) {
			dev_dbg(padctl->dev, "USB2 port %d DCD successful\n",
				port);
			ret = true;
			break;
		}
		usleep_range(20000, 22000);
		dcd_timeout_ms += 22;
	}

	if (!ret)
		dev_info(padctl->dev, "%s: DCD timeout %d ms\n", __func__,
			 dcd_timeout_ms);

	/* Turn off IP_SRC, clear DCD DETECTED*/
	reg = padctl_readl(padctl, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
	reg &= ~OP_I_SRC_EN;
	reg |= DCD_DETECTED;
	padctl_writel(padctl, reg, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));

	/* Turn off D- pull-down resistor */
	reg = padctl_readl(padctl, USB2_BATTERY_CHRG_OTGPADX_CTL1(port));
	reg &= ~USBON_RPD_OVRD_VAL;
	padctl_writel(padctl, reg, USB2_BATTERY_CHRG_OTGPADX_CTL1(port));

	return ret;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_utmi_pad_dcd);

u32 tegra18x_phy_xusb_noncompliant_div_detect(struct phy *phy)
{
	struct tegra_padctl *padctl;
	int port;
	u32 reg;

	if (!phy)
		return 0;

	padctl = phy_get_drvdata(phy);
	port = utmi_phy_to_port(phy);

	reg = padctl_readl(padctl, USB2_BATTERY_CHRG_OTGPADX_CTL1(port));
	reg |= DIV_DET_EN;
	padctl_writel(padctl, reg, USB2_BATTERY_CHRG_OTGPADX_CTL1(port));

	udelay(10);

	reg = padctl_readl(padctl, USB2_BATTERY_CHRG_OTGPADX_CTL1(port));
	reg &= ~DIV_DET_EN;
	padctl_writel(padctl, reg, USB2_BATTERY_CHRG_OTGPADX_CTL1(port));

	return reg;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_noncompliant_div_detect);

bool tegra18x_phy_xusb_utmi_pad_primary_charger_detect(struct phy *phy)
{
	struct tegra_padctl *padctl;
	int port;
	u32 reg;
	bool ret;

	if (!phy)
		return false;

	padctl = phy_get_drvdata(phy);
	port = utmi_phy_to_port(phy);

	/* Source D+ to D- */
	reg = padctl_readl(padctl, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
	reg |= OP_SRC_EN | ON_SINK_EN;
	padctl_writel(padctl, reg, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));

	/* Wait for TVDPSRC_ON */
	msleep(40);

	reg = padctl_readl(padctl, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
	ret = !!(reg & VDAT_DET);

	/* Turn off OP_SRC, ON_SINK, clear VDAT, ZIN status change */
	reg = padctl_readl(padctl, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
	reg &= ~(OP_SRC_EN | ON_SINK_EN);
	padctl_writel(padctl, reg, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));

	return ret;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_utmi_pad_primary_charger_detect);

bool tegra18x_phy_xusb_utmi_pad_secondary_charger_detect(struct phy *phy)
{
	struct tegra_padctl *padctl;
	int port;
	u32 reg;
	bool ret;

	if (!phy)
		return false;

	padctl = phy_get_drvdata(phy);
	port = utmi_phy_to_port(phy);

	/* Source D- to D+ */
	reg = padctl_readl(padctl, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
	reg |= ON_SRC_EN | OP_SINK_EN;
	padctl_writel(padctl, reg, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));

	/* Wait for TVDPSRC_ON */
	msleep(40);

	reg = padctl_readl(padctl, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
	ret = !(reg & VDAT_DET);

	/* Turn off ON_SRC, OP_SINK, clear VDAT, ZIP status change */
	reg = padctl_readl(padctl, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
	reg &= ~(ON_SRC_EN | OP_SINK_EN);
	padctl_writel(padctl, reg, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));

	return ret;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_utmi_pad_secondary_charger_detect);

/*
 * This function will fource vbus on whatever under
 * over-current SFIO or regulator GPIO control,
 * and also without caring about regulator refcnt.
 */
int tegra18x_phy_xusb_utmi_vbus_power_on(struct phy *phy)
{
	struct tegra_padctl *padctl;
	int port;
	int rc = 0;
	int status;

	if (!phy)
		return -EINVAL;

	padctl = phy_get_drvdata(phy);
	port = utmi_phy_to_port(phy);

	mutex_lock(&padctl->lock);
	if (padctl->oc_pinctrl && padctl->utmi_ports[port].oc_pin >= 0) {
		tegra_xusb_select_vbus_en_state(padctl,
			padctl->utmi_ports[port].oc_pin, true);
		tegra186_enable_vbus_oc(padctl->utmi_phys[port]);
	} else {
		status = regulator_is_enabled(padctl->vbus[port]);
		if (padctl->vbus[port] && !status) {
			rc = regulator_enable(padctl->vbus[port]);
			if (rc)
				dev_err(padctl->dev, "enable port %d vbus failed %d\n",
					port, rc);
		}
		dev_dbg(padctl->dev, "%s: port %d regulator status: %d->%d\n",
			 __func__, port, status,
			 regulator_is_enabled(padctl->vbus[port]));
	}
	mutex_unlock(&padctl->lock);
	return rc;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_utmi_vbus_power_on);

/*
 * This function will fource vbus off whatever under
 * over-current SFIO or regulator GPIO control,
 * and also without caring about regulator refcnt;
 * the only exception is for 'otg vbus always on' case.
 */
int tegra18x_phy_xusb_utmi_vbus_power_off(struct phy *phy)
{
	struct tegra_padctl *padctl;
	int port;
	int rc = 0;
	int status;

	if (!phy)
		return -EINVAL;

	padctl = phy_get_drvdata(phy);
	port = utmi_phy_to_port(phy);

	if (port == padctl->utmi_otg_port_base_1 - 1
			&& padctl->otg_vbus_alwayson) {
		dev_dbg(padctl->dev, "%s: port %d vbus cannot off due to alwayson\n",
			 __func__, port);
		return -EINVAL;
	}

	mutex_lock(&padctl->lock);
	if (padctl->oc_pinctrl && padctl->utmi_ports[port].oc_pin >= 0) {
		tegra_xusb_select_vbus_en_state(padctl,
			padctl->utmi_ports[port].oc_pin, false);
		tegra186_disable_vbus_oc(padctl->utmi_phys[port]);
	} else {
		status = regulator_is_enabled(padctl->vbus[port]);
		if (padctl->vbus[port] && status) {
			rc = regulator_disable(padctl->vbus[port]);
			if (rc)
				dev_err(padctl->dev, "disable port %d vbus failed %d\n",
					port, rc);
		}
		dev_dbg(padctl->dev, "%s: port %d regulator status: %d->%d\n",
			 __func__, port, status,
			 regulator_is_enabled(padctl->vbus[port]));
	}
	mutex_unlock(&padctl->lock);
	return rc;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_utmi_vbus_power_off);

int tegra18x_phy_xusb_overcurrent_detected(struct phy *phy)
{
	struct tegra_padctl *padctl;
	int port;
	bool detected = false;
	u32 reg;
	int pin;

	if (!phy)
		return 0;

	padctl = phy_get_drvdata(phy);
	if (!is_utmi_phy(phy))
		return -EINVAL;

	port = utmi_phy_to_port(phy);
	if (port < 0)
		return -EINVAL;

	pin = padctl->utmi_ports[port].oc_pin;
	if (pin < 0)
		return -EINVAL;

	reg = padctl_readl(padctl, XUSB_PADCTL_OC_DET);

	detected = !!(reg & OC_DETECTED_VBUS_PAD(pin));
	if (detected) {
		reg &= ~OC_DETECTED_VBUS_PAD_MASK;
		reg &= ~OC_DETECTED_INT_EN_VBUS_PAD(pin);
		padctl_writel(padctl, reg, XUSB_PADCTL_OC_DET);
	}

	return detected;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_overcurrent_detected);

void tegra18x_phy_xusb_handle_overcurrent(struct phy *phy)
{
	struct tegra_padctl *padctl;
	u32 reg;
	unsigned i;
	int pin;

	if (!phy)
		return;

	padctl = phy_get_drvdata(phy);
	if (!is_utmi_phy(phy))
		return;

	oc_debug(padctl);
	mutex_lock(&padctl->lock);
	reg = padctl_readl(padctl, XUSB_PADCTL_OC_DET);

	for (i = 0; i < TEGRA_UTMI_PHYS; i++) {
		pin = padctl->utmi_ports[i].oc_pin;
		if (pin < 0)
			continue;

		if (reg & OC_DETECTED_VBUS_PAD(pin)) {
			dev_info(padctl->dev, "%s: clear port %d pin %d OC\n",
				 __func__, i, pin);
			tegra186_enable_vbus_oc(padctl->utmi_phys[i]);
		}
	}
	mutex_unlock(&padctl->lock);
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_handle_overcurrent);

static int tegra186_usb3_phy_reverse_id(struct tegra_padctl *padctl,
					int port, bool enable)
{
	u32 reg;

	mutex_lock(&padctl->lock);
	reg = padctl_readl(padctl, XUSB_PADCTL_SS_PORT_CAP);
	if (enable)
		reg |= PORT_REVERSE_ID(port);
	else
		reg &= ~PORT_REVERSE_ID(port);
	padctl_writel(padctl, reg, XUSB_PADCTL_SS_PORT_CAP);
	mutex_unlock(&padctl->lock);

	return 0;
}

static int tegra186_utmi_phy_reverse_id(struct tegra_padctl *padctl,
					int port, bool enable)
{
	u32 reg;

	mutex_lock(&padctl->lock);
	reg = padctl_readl(padctl, XUSB_PADCTL_USB2_PORT_CAP);
	if (enable)
		reg |= PORT_REVERSE_ID(port);
	else
		reg &= ~PORT_REVERSE_ID(port);
	padctl_writel(padctl, reg, XUSB_PADCTL_USB2_PORT_CAP);
	mutex_unlock(&padctl->lock);

	return 0;
}

int tegra18x_phy_xusb_set_reverse_id(struct phy *phy)
{
	struct tegra_padctl *padctl;
	int port;

	if (!phy)
		return -EINVAL;

	padctl = phy_get_drvdata(phy);

	/* applicable to SS/UTMI only */
	if (is_utmi_phy(phy)) {
		port = utmi_phy_to_port(phy);
		if (port < 0)
			return -EINVAL;
		return tegra186_utmi_phy_reverse_id(padctl, port, true);
	} else if (is_usb3_phy(phy)) {
		port = usb3_phy_to_port(phy);
		if (port < 0)
			return -EINVAL;
		return tegra186_usb3_phy_reverse_id(padctl, port, true);
	}

	return -EINVAL;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_set_reverse_id);

int tegra18x_phy_xusb_clear_reverse_id(struct phy *phy)
{
	struct tegra_padctl *padctl;
	int port;

	if (!phy)
		return -EINVAL;

	padctl = phy_get_drvdata(phy);

	/* applicable to SS/UTMI only */
	if (is_utmi_phy(phy)) {
		port = utmi_phy_to_port(phy);
		if (port < 0)
			return -EINVAL;
		return tegra186_utmi_phy_reverse_id(padctl, port, false);
	} else if (is_usb3_phy(phy)) {
		port = usb3_phy_to_port(phy);
		if (port < 0)
			return -EINVAL;
		return tegra186_usb3_phy_reverse_id(padctl, port, false);
	}

	return -EINVAL;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_clear_reverse_id);

int tegra18x_phy_xusb_generate_srp(struct phy *phy)
{
	struct tegra_padctl *padctl;
	u32 reg;
	int port;

	if (!phy)
		return -EINVAL;

	padctl = phy_get_drvdata(phy);

	/* applicable only to UTMI */
	if (is_utmi_phy(phy)) {
		port = utmi_phy_to_port(phy);
		if (port < 0)
			return -EINVAL;
		mutex_lock(&padctl->lock);
		reg = padctl_readl(padctl,
				   USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
		reg |= GENERATE_SRP;
		padctl_writel(padctl, reg,
			      USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
		mutex_unlock(&padctl->lock);

		return 0;
	}

	return -EINVAL;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_generate_srp);

static int tegra186_utmi_phy_srp_detect(struct tegra_padctl *padctl,
					int port, bool enable)
{
	u32 reg;

	reg = padctl_readl(padctl, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
	if (enable)
		reg |= SRP_DETECT_EN | SRP_INTR_EN;
	else
		reg &= ~(SRP_DETECT_EN | SRP_INTR_EN);
	padctl_writel(padctl, reg, USB2_BATTERY_CHRG_OTGPADX_CTL0(port));

	return 0;
}

int tegra18x_phy_xusb_enable_srp_detect(struct phy *phy)
{
	struct tegra_padctl *padctl;
	int port;

	if (!phy)
		return -EINVAL;

	padctl = phy_get_drvdata(phy);

	/* applicable only to UTMI */
	if (is_utmi_phy(phy)) {
		port = utmi_phy_to_port(phy);
		if (port < 0)
			return -EINVAL;
		return tegra186_utmi_phy_srp_detect(padctl, port, true);
	}

	return -EINVAL;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_enable_srp_detect);

int tegra18x_phy_xusb_disable_srp_detect(struct phy *phy)
{
	struct tegra_padctl *padctl;
	int port;

	if (!phy)
		return -EINVAL;

	padctl = phy_get_drvdata(phy);

	/* applicable only to UTMI */
	if (is_utmi_phy(phy)) {
		port = utmi_phy_to_port(phy);
		if (port < 0)
			return -EINVAL;
		return tegra186_utmi_phy_srp_detect(padctl, port, false);
	}

	return -EINVAL;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_disable_srp_detect);

bool tegra18x_phy_xusb_srp_detected(struct phy *phy)
{
	struct tegra_padctl *padctl;
	u32 reg;
	int port;

	if (!phy)
		return false;

	padctl = phy_get_drvdata(phy);

	/* applicable only to UTMI */
	if (is_utmi_phy(phy)) {
		port = utmi_phy_to_port(phy);
		if (port < 0)
			return false;

		reg = padctl_readl(padctl,
				   USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
		dev_dbg(padctl->dev, "USB2_BATTERY_CHRG_OTGPADX_CTL0:%#x\n",
			reg);
		if (reg & SRP_DETECTED) {
			padctl_writel(padctl, reg,
					USB2_BATTERY_CHRG_OTGPADX_CTL0(port));
			return true;
		}
	}

	return false;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_srp_detected);

int tegra18x_phy_xusb_enable_otg_int(struct phy *phy)
{
	struct tegra_padctl *padctl;
	u32 reg;

	if (!phy)
		return -EINVAL;

	padctl = phy_get_drvdata(phy);

	mutex_lock(&padctl->lock);
	reg = padctl_readl(padctl, USB2_VBUS_ID);
	reg |= VBUS_VALID_CHNG_INTR_EN | OTG_VBUS_SESS_VLD_CHNG_INTR_EN |
		IDDIG_CHNG_INTR_EN | VBUS_WAKEUP_CHNG_INTR_EN;
	padctl_writel(padctl, reg, USB2_VBUS_ID);
	mutex_unlock(&padctl->lock);

	return 0;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_enable_otg_int);

int tegra18x_phy_xusb_disable_otg_int(struct phy *phy)
{
	struct tegra_padctl *padctl;
	u32 reg;

	if (!phy)
		return -EINVAL;

	padctl = phy_get_drvdata(phy);

	mutex_lock(&padctl->lock);
	reg = padctl_readl(padctl, USB2_VBUS_ID);
	reg &= ~(VBUS_VALID_CHNG_INTR_EN | OTG_VBUS_SESS_VLD_CHNG_INTR_EN |
		 IDDIG_CHNG_INTR_EN | VBUS_WAKEUP_CHNG_INTR_EN);
	padctl_writel(padctl, reg, USB2_VBUS_ID);
	mutex_unlock(&padctl->lock);

	return 0;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_disable_otg_int);

int tegra18x_phy_xusb_ack_otg_int(struct phy *phy)
{
	struct tegra_padctl *padctl;
	u32 reg;

	if (!phy)
		return -EINVAL;

	padctl = phy_get_drvdata(phy);

	reg = padctl_readl(padctl, USB2_VBUS_ID);
	padctl_writel(padctl, reg, USB2_VBUS_ID);

	return 0;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_ack_otg_int);

int tegra18x_phy_xusb_get_otg_vbus_id(struct phy *phy,
		struct tegra_xusb_otg_vbus_id *info)
{
	struct tegra_padctl *padctl;
	u32 reg;

	if (!phy || !info)
		return -EINVAL;

	padctl = phy_get_drvdata(phy);

	reg = padctl_readl(padctl, USB2_VBUS_ID);

	info->iddig_chg = !!(reg & IDDIG_ST_CHNG);
	info->iddig = tegra_phy_xusb_parse_rid(reg);
	dev_dbg(padctl->dev, "%s: iddig_chg=%d, iddig=%d\n",
		__func__, info->iddig_chg, info->iddig);

	info->vbus_sess_vld_chg = !!(reg & OTG_VBUS_SESS_VLD_ST_CHNG);
	info->vbus_sess_vld = !!(reg & OTG_VBUS_SESS_VLD);
	dev_dbg(padctl->dev, "%s: vbus_sess_vld_chg=%d, vbus_sess_vld=%d\n",
		__func__, info->vbus_sess_vld_chg, info->vbus_sess_vld);

	info->vbus_vld_chg = !!(reg & VBUS_VALID_ST_CHNG);
	info->vbus_vld = !!(reg & VBUS_VALID);
	dev_dbg(padctl->dev, "%s: vbus_vld_chg=%d, vbus_vld=%d\n",
		__func__, info->vbus_vld_chg, info->vbus_vld);

	info->vbus_wakeup_chg = !!(reg & VBUS_WAKEUP_ST_CHNG);
	info->vbus_wakeup = !!(reg & VBUS_WAKEUP);
	dev_dbg(padctl->dev, "%s: vbus_wakeup_chg=%d, vbus_wakeup=%d\n",
		__func__, info->vbus_wakeup_chg, info->vbus_wakeup);

	info->vbus_override = !!(reg & VBUS_OVERRIDE);
	info->id_override = (reg >> ID_OVERRIDE_SHIFT) & ID_OVERRIDE_MASK;
	dev_dbg(padctl->dev, "%s: vbus_override=%d, id_override=%d\n",
		__func__, info->vbus_override, info->id_override);

	return 0;
}
EXPORT_SYMBOL_GPL(tegra18x_phy_xusb_get_otg_vbus_id);

MODULE_AUTHOR("JC Kuo <jckuo@nvidia.com>");
MODULE_DESCRIPTION("Tegra 186 XUSB PADCTL driver");
MODULE_LICENSE("GPL v2");
