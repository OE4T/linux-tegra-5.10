/*
 * BCMSDH Function Driver for the native SDIO/MMC driver in the Linux Kernel
 *
 * Copyright (C) 1999-2015, Broadcom Corporation
 * 
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * $Id: bcmsdh_sdmmc_linux.c 434777 2013-11-07 09:30:27Z $
 */

#include <typedefs.h>
#include "dynamic.h"
#include <bcmutils.h>
#include <sdio.h>	/* SDIO Device and Protocol Specs */
#include <bcmsdbus.h>	/* bcmsdh to/from specific controller APIs */
#include <sdiovar.h>	/* to get msglevel bit values */

#include <linux/sched.h>	/* request_irq() */

#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <dhd_linux.h>
#include <bcmsdh_sdmmc.h>
#include <dhd_dbg.h>
#if defined(CONFIG_WIFI_CONTROL_FUNC)
#include <linux/wlan_plat.h>
#endif
#include <linux_osl.h>

#ifdef	CONFIG_BCMDHD_CUSTOM_SYSFS_TEGRA
#include "dhd_custom_sysfs_tegra.h"
#endif

#if !defined(SDIO_VENDOR_ID_BROADCOM)
#define SDIO_VENDOR_ID_BROADCOM		0x02d0
#endif /* !defined(SDIO_VENDOR_ID_BROADCOM) */

#define SDIO_DEVICE_ID_BROADCOM_DEFAULT	0x0000

#if !defined(SDIO_DEVICE_ID_BROADCOM_4325_SDGWB)
#define SDIO_DEVICE_ID_BROADCOM_4325_SDGWB	0x0492	/* BCM94325SDGWB */
#endif /* !defined(SDIO_DEVICE_ID_BROADCOM_4325_SDGWB) */
#if !defined(SDIO_DEVICE_ID_BROADCOM_4325)
#define SDIO_DEVICE_ID_BROADCOM_4325	0x0493
#endif /* !defined(SDIO_DEVICE_ID_BROADCOM_4325) */
#if !defined(SDIO_DEVICE_ID_BROADCOM_4329)
#define SDIO_DEVICE_ID_BROADCOM_4329	0x4329
#endif /* !defined(SDIO_DEVICE_ID_BROADCOM_4329) */
#if !defined(SDIO_DEVICE_ID_BROADCOM_4319)
#define SDIO_DEVICE_ID_BROADCOM_4319	0x4319
#endif /* !defined(SDIO_DEVICE_ID_BROADCOM_4319) */
#if !defined(SDIO_DEVICE_ID_BROADCOM_4330)
#define SDIO_DEVICE_ID_BROADCOM_4330	0x4330
#endif /* !defined(SDIO_DEVICE_ID_BROADCOM_4330) */
#if !defined(SDIO_DEVICE_ID_BROADCOM_4334)
#define SDIO_DEVICE_ID_BROADCOM_4334    0x4334
#endif /* !defined(SDIO_DEVICE_ID_BROADCOM_4334) */
#if !defined(SDIO_DEVICE_ID_BROADCOM_4324)
#define SDIO_DEVICE_ID_BROADCOM_4324    0x4324
#endif /* !defined(SDIO_DEVICE_ID_BROADCOM_4324) */
#if !defined(SDIO_DEVICE_ID_BROADCOM_43239)
#define SDIO_DEVICE_ID_BROADCOM_43239    43239
#endif /* !defined(SDIO_DEVICE_ID_BROADCOM_43239) */
#if !defined(SDIO_DEVICE_ID_BROADCOM_4339)
#define SDIO_DEVICE_ID_BROADCOM_4339    4339
#endif /* !defined(SDIO_DEVICE_ID_BROADCOM_4339) */

extern void wl_cfg80211_set_parent_dev(void *dev);
extern void sdioh_sdmmc_devintr_off(sdioh_info_t *sd);
extern void sdioh_sdmmc_devintr_on(sdioh_info_t *sd);
extern void* bcmsdh_probe(osl_t *osh, void *dev, void *sdioh, void *adapter_info, uint bus_type,
	uint bus_num, uint slot_num);
extern int bcmsdh_remove(bcmsdh_info_t *bcmsdh);
extern int wifi_platform_bus_enumerate(wifi_adapter_info_t *adapter, bool device_present);

int sdio_function_init(void);
void sdio_function_cleanup(void);

#define DESCRIPTION "bcmsdh_sdmmc Driver"
#define AUTHOR "Broadcom Corporation"

/* module param defaults */
static int clockoverride = 0;

module_param(clockoverride, int, 0644);
MODULE_PARM_DESC(clockoverride, "SDIO card clock override");

/* Maximum number of bcmsdh_sdmmc devices supported by driver */
#define BCMSDH_SDMMC_MAX_DEVICES 1

extern volatile bool dhd_mmc_suspend;

static int sdioh_probe(struct sdio_func *func)
{
	int host_idx = func->card->host->index;
	uint32 rca = func->card->rca;
	wifi_adapter_info_t *adapter;
	osl_t *osh = NULL;
	sdioh_info_t *sdioh = NULL;
#if defined(CONFIG_WIFI_CONTROL_FUNC)
	struct wifi_platform_data *plat_data;
#endif

	sd_err(("bus num (host idx)=%d, slot num (rca)=%d\n", host_idx, rca));
	adapter = dhd_wifi_platform_get_adapter(SDIO_BUS, host_idx, rca);
	if (adapter  != NULL) {
		sd_err(("found adapter info '%s'\n", adapter->name));
		/* SDIO card detection is completed
		 * so stop card detection here
		 */
		wifi_platform_bus_enumerate(adapter, 0);
	} else {
		sd_err(("can't find adapter info for this chip\n"));
	}

#ifdef WL_CFG80211
	wl_cfg80211_set_parent_dev(&func->dev);
#endif

	 /* allocate SDIO Host Controller state info */
	 osh = osl_attach(&func->dev, SDIO_BUS, TRUE);
	 if (osh == NULL) {
		 sd_err(("%s: osl_attach failed\n", __FUNCTION__));
		 goto fail;
	 }
	 osl_static_mem_init(osh, adapter);
	 sdioh = sdioh_attach(osh, func);
	 if (sdioh == NULL) {
		 sd_err(("%s: sdioh_attach failed\n", __FUNCTION__));
		 goto fail;
	 }
	 sdioh->bcmsdh = bcmsdh_probe(osh, &func->dev, sdioh, adapter, SDIO_BUS, host_idx, rca);
	 if (sdioh->bcmsdh == NULL) {
		 sd_err(("%s: bcmsdh_probe failed\n", __FUNCTION__));
		 goto fail;
	 }

	sdio_set_drvdata(func, sdioh);
	return 0;

fail:
	if (sdioh != NULL)
		sdioh_detach(osh, sdioh);
	if (osh != NULL)
		osl_detach(osh);
	return -ENOMEM;
}

static void sdioh_remove(struct sdio_func *func)
{
	sdioh_info_t *sdioh;
	osl_t *osh;

	sdioh = sdio_get_drvdata(func);
	if (sdioh == NULL) {
		sd_err(("%s: error, no sdioh handler found\n", __FUNCTION__));
		return;
	}

	osh = sdioh->osh;
	bcmsdh_remove(sdioh->bcmsdh);
	sdioh_detach(osh, sdioh);
	osl_detach(osh);
}

#ifdef CONFIG_BCMDYNAMIC

int bcmdhd_chipid;
int bcmdhd_custom_ampdu_ba_wsize;
extern int dhd_dpc_prio;
extern int dhd_rxf_prio;
int bcmdhd_custom_glom_setting;
int bcmdhd_custom_rxchain;
extern int firstread;
int bcmdhd_custom_ampdu_mpdu;
int bcmdhd_custom_pspretend_thr;
int bcmdhd_wifi_turnon_delay;
int bcmdhd_wifi_turnoff_delay;

bool bcmdhd_use_custom_ampdu_mpdu;
bool bcmdhd_use_custom_pspretend_thr;

bool bcmdhd_custom_rxcb;
bool bcmdhd_prop_txstatus_vsdb;
bool bcmdhd_vsdb_bw_allocate_enable;
bool bcmdhd_bcmsdioh_txglom;
bool bcmdhd_bcmsdioh_txglow_highspeed;
bool bcmdhd_use_wl_txbf;
bool bcmdhd_use_wl_frameburst;
bool bcmdhd_disable_roam_event;
bool bcmdhd_support_p2p_go_ps;
bool bcmdhd_wl11u;
bool bcmdhd_dhd_enable_lpc;

static void bcmdhd_dynamic_configure(int chipid)
{
	bcmdhd_chipid = chipid;
	if (chipid == 0x4324) {
		/* from the old cflags */
		bcmdhd_custom_ampdu_ba_wsize = 32;
		dhd_dpc_prio = dhd_rxf_prio = MAX_USER_RT_PRIO/2;
		/* set on/off delay */
		bcmdhd_wifi_turnon_delay = 400;
		bcmdhd_wifi_turnoff_delay = 400;

		bcmdhd_prop_txstatus_vsdb = true;
		bcmdhd_vsdb_bw_allocate_enable = true;

		/* defaults */
		bcmdhd_custom_glom_setting = DEFAULT_GLOM_VALUE;
		bcmdhd_custom_ampdu_mpdu = -1;
	} else if(chipid == 0x4354) {
		bcmdhd_custom_glom_setting = 8;
		bcmdhd_custom_rxchain = 1;
		bcmdhd_custom_rxcb=true;
		bcmdhd_custom_ampdu_ba_wsize = 64;
		firstread = 128;
		bcmdhd_use_custom_ampdu_mpdu = true;
		bcmdhd_custom_ampdu_mpdu = 16;
		bcmdhd_use_custom_pspretend_thr = true;
		bcmdhd_custom_pspretend_thr = 30;
		dhd_dpc_prio = dhd_rxf_prio = 99;
		/* set on/off delay */
		bcmdhd_wifi_turnon_delay = 200;
		bcmdhd_wifi_turnoff_delay = 200;

		bcmdhd_bcmsdioh_txglom = true;
		bcmdhd_use_wl_txbf = true;
		bcmdhd_use_wl_frameburst = true;
		bcmdhd_disable_roam_event = true;
		bcmdhd_support_p2p_go_ps = true;
		bcmdhd_wl11u = true;
		bcmdhd_dhd_enable_lpc = true;
	} else {
		BUG();
	}
}

#else
static void bcmdhd_dynamic_configure(int chipid) {}
#endif

static int bcmsdh_sdmmc_probe(struct sdio_func *func,
                              const struct sdio_device_id *id)
{
	int ret = 0;

	if (func == NULL)
		return -EINVAL;

	sd_err(("bcmsdh_sdmmc: %s Enter\n", __FUNCTION__));
	sd_info(("sdio_bcmsdh: func->class=%x\n", func->class));
	sd_info(("sdio_vendor: 0x%04x\n", func->vendor));
	sd_info(("sdio_device: 0x%04x\n", func->device));
	sd_info(("Function#: 0x%04x\n", func->num));

	bcmdhd_dynamic_configure(func->device);

	/* 4318 doesn't have function 2 */
	if ((func->num == 2) || (func->num == 1 && func->device == 0x4)) {
		ret = sdioh_probe(func);
		if (dhd_mmc_power_save_host(func->card->host))
			sd_err(("%s: card power save fail", __FUNCTION__));
	}

	return ret;
}

static void bcmsdh_sdmmc_remove(struct sdio_func *func)
{
	if (func == NULL) {
		sd_err(("%s is called with NULL SDIO function pointer\n", __FUNCTION__));
		return;
	}
	sd_trace(("bcmsdh_sdmmc: %s Enter\n", __FUNCTION__));
	sd_info(("sdio_bcmsdh: func->class=%x\n", func->class));
	sd_info(("sdio_vendor: 0x%04x\n", func->vendor));
	sd_info(("sdio_device: 0x%04x\n", func->device));
	sd_info(("Function#: 0x%04x\n", func->num));

	if (func->card->sdio_func[1])
		dhd_mmc_power_restore_host(func->card->host);

	if ((func->num == 2) || (func->num == 1 && func->device == 0x4))
		sdioh_remove(func);

	if (dhd_mmc_power_save_host(func->card->host))
		sd_err(("%s: card power save fail", __FUNCTION__));
}

/* devices we support, null terminated */
static const struct sdio_device_id bcmsdh_sdmmc_ids[] = {
#if 0
	{ 	.class	= SDIO_CLASS_NONE,
		.vendor	= SDIO_VENDOR_ID_BROADCOM,
		.device	= SDIO_ANY_ID
	},
#elif 1
	/* BCM4354 */
	{ 	.class	= SDIO_CLASS_NONE,
		.vendor	= SDIO_VENDOR_ID_BROADCOM,
		.device	= 0x4354
	},
	/* BCM43241 */
	{ 	.class	= SDIO_CLASS_NONE,
		.vendor	= SDIO_VENDOR_ID_BROADCOM,
		.device	= 0x4324
	},
	/* BCM4339 */
	{ 	.class	= SDIO_CLASS_NONE,
		.vendor	= SDIO_VENDOR_ID_BROADCOM,
		.device	= 0x4339
	},

#else
	/* BCM43341 */
	{ 	.class	= SDIO_CLASS_NONE,
		.vendor	= SDIO_VENDOR_ID_BROADCOM,
		.device	= 0xa94d
	},
#endif
	{ /* end: all zeroes */                         },
};

MODULE_DEVICE_TABLE(sdio, bcmsdh_sdmmc_ids);

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 39)) && defined(CONFIG_PM)
static int bcmsdh_sdmmc_suspend(struct device *pdev)
{
	int err;
	sdioh_info_t *sdioh;
	struct sdio_func *func = dev_to_sdio_func(pdev);
	mmc_pm_flag_t sdio_flags;

	sd_err(("%s Enter\n", __FUNCTION__));
	if (func->num != 2)
		return 0;

	sdioh = sdio_get_drvdata(func);
	err = bcmsdh_suspend(sdioh->bcmsdh);
	if (err)
		return err;

	sdio_flags = sdio_get_host_pm_caps(func);
	if (!(sdio_flags & MMC_PM_KEEP_POWER)) {
		sd_err(("%s: can't keep power while host is suspended\n", __FUNCTION__));
		return  -EINVAL;
	}

#ifdef	CONFIG_BCMDHD_CUSTOM_SYSFS_TEGRA
	tegra_sysfs_suspend();
#endif

	/* keep power while host suspended */
	err = sdio_set_host_pm_flags(func, MMC_PM_KEEP_POWER);
	if (err) {
		sd_err(("%s: error while trying to keep power\n", __FUNCTION__));
		return err;
	}
#if defined(OOB_INTR_ONLY)
	bcmsdh_oob_intr_set(sdioh->bcmsdh, FALSE);
#endif 
	dhd_mmc_suspend = TRUE;
	smp_mb();

	return 0;
}

static int bcmsdh_sdmmc_resume(struct device *pdev)
{
	sdioh_info_t *sdioh;
	struct sdio_func *func = dev_to_sdio_func(pdev);

	sd_err(("%s Enter\n", __FUNCTION__));
	if (func->num != 2)
		return 0;

	sdioh = sdio_get_drvdata(func);
	dhd_mmc_suspend = FALSE;
#if defined(OOB_INTR_ONLY)
	bcmsdh_resume(sdioh->bcmsdh);
#endif 
#ifdef	CONFIG_BCMDHD_CUSTOM_SYSFS_TEGRA
	tegra_sysfs_resume();
#endif

	smp_mb();
	return 0;
}

#ifdef CONFIG_BCMDHD_CUSTOM_SYSFS_TEGRA
static int bcmsdh_sdmmc_resume_noirq(struct device *pdev)
{
	tegra_sysfs_resume_capture();
	return 0;
}
#endif

static const struct dev_pm_ops bcmsdh_sdmmc_pm_ops = {
	.suspend	= bcmsdh_sdmmc_suspend,
	.resume		= bcmsdh_sdmmc_resume,
#ifdef CONFIG_BCMDHD_CUSTOM_SYSFS_TEGRA
	.resume_noirq   = bcmsdh_sdmmc_resume_noirq,
#endif
};
#endif  /* (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 39)) && defined(CONFIG_PM) */

#if defined(BCMLXSDMMC)
static struct semaphore *notify_semaphore = NULL;

static int dummy_probe(struct sdio_func *func,
                              const struct sdio_device_id *id)
{
	if (func && (func->num != 2)) {
		return 0;
	}

	if (notify_semaphore)
		up(notify_semaphore);
	return 0;
}

static void dummy_remove(struct sdio_func *func)
{
}

static struct sdio_driver dummy_sdmmc_driver = {
	.probe		= dummy_probe,
	.remove		= dummy_remove,
	.name		= "dummy_sdmmc",
	.id_table	= bcmsdh_sdmmc_ids,
	};

int sdio_func_reg_notify(void* semaphore)
{
	notify_semaphore = semaphore;
	return sdio_register_driver(&dummy_sdmmc_driver);
}

void sdio_func_unreg_notify(void)
{
	OSL_SLEEP(15);
	sdio_unregister_driver(&dummy_sdmmc_driver);
}

#endif /* defined(BCMLXSDMMC) */

static struct sdio_driver bcmsdh_sdmmc_driver = {
	.probe		= bcmsdh_sdmmc_probe,
	.remove		= bcmsdh_sdmmc_remove,
	.name		= "bcmsdh_sdmmc",
	.id_table	= bcmsdh_sdmmc_ids,
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 39)) && defined(CONFIG_PM)
	.drv = {
	.pm	= &bcmsdh_sdmmc_pm_ops,
	},
#endif /* (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 39)) && defined(CONFIG_PM) */
	};

struct sdos_info {
	sdioh_info_t *sd;
	spinlock_t lock;
};

/* Interrupt enable/disable */
SDIOH_API_RC
sdioh_interrupt_set(sdioh_info_t *sd, bool enable)
{
	if (!sd)
		return BCME_BADARG;

	sd_trace(("%s: %s\n", __FUNCTION__, enable ? "Enabling" : "Disabling"));
	return SDIOH_API_RC_SUCCESS;
}

#ifdef BCMSDH_MODULE
static int __init
bcmsdh_module_init(void)
{
	int error = 0;
	error = sdio_function_init();
	return error;
}

static void __exit
bcmsdh_module_cleanup(void)
{
	sdio_function_cleanup();
}

module_init(bcmsdh_module_init);
module_exit(bcmsdh_module_cleanup);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION(DESCRIPTION);
MODULE_AUTHOR(AUTHOR);

#endif /* BCMSDH_MODULE */
/*
 * module init
*/
int bcmsdh_register_client_driver(void)
{
	return sdio_register_driver(&bcmsdh_sdmmc_driver);
}

/*
 * module cleanup
*/
void bcmsdh_unregister_client_driver(void)
{
	sdio_unregister_driver(&bcmsdh_sdmmc_driver);
}
