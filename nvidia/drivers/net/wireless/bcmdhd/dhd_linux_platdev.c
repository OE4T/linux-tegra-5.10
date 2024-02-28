/*
 * Linux platform device for DHD WLAN adapter
 *
 * Copyright (C) 1999-2015, Broadcom Corporation
 * 
 * Portions contributed by Nvidia
 * Copyright (C) 2015-2019 NVIDIA Corporation. All rights reserved.
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
 * $Id: dhd_linux_platdev.c 401742 2013-05-13 15:03:21Z $
 */
#include <typedefs.h>
#include "dynamic.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <bcmutils.h>
#include <linux_osl.h>
#include <dhd_dbg.h>
#include <dngl_stats.h>
#include <dhd.h>
#include <dhd_bus.h>
#include <dhd_linux.h>
#include <wl_android.h>
#if defined(CONFIG_WIFI_CONTROL_FUNC)
#include <linux/wlan_plat.h>
#endif
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/mmc/host.h>
#include <host/sdhci.h>
#ifdef CONFIG_BCMDHD_CUSTOM_SYSFS_TEGRA
#include "dhd_custom_sysfs_tegra.h"
#include "dhd_custom_sysfs_tegra_stat.h"
#endif
#ifdef CONFIG_TEGRA_SYS_EDP
#include <soc/tegra/sysedp.h>
#endif

#if !defined(CONFIG_WIFI_CONTROL_FUNC)
struct wifi_platform_data {
	int (*set_power)(int val);
	int (*set_reset)(int val);
	int (*set_carddetect)(int val);
	void *(*mem_prealloc)(int section, unsigned long size);
	int (*get_mac_addr)(unsigned char *buf);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 58))
	void *(*get_country_code)(char *ccode, u32 flags);
#else
	void *(*get_country_code)(char *ccode);
#endif
};
#endif /* CONFIG_WIFI_CONTROL_FUNC */

struct cntry_locales_custom {
	char iso_abbrev[WLC_CNTRY_BUF_SZ];	/* ISO 3166-1 country abbreviation */
	char custom_locale[WLC_CNTRY_BUF_SZ];	/* Custom firmware locale */
	int32 custom_locale_rev;		/* Custom local revisin default -1 */
};

#define WIFI_PLAT_NAME		"bcmdhd_wlan"
#define WIFI_PLAT_NAME2		"bcm4329_wlan"
#define WIFI_PLAT_EXT		"bcmdhd_wifi_platform"

bool cfg_multichip = FALSE;
bcmdhd_wifi_platdata_t *dhd_wifi_platdata = NULL;
static int wifi_plat_dev_probe_ret = 0;
static bool is_power_on = FALSE;
#if defined(DHD_OF_SUPPORT)
static bool dts_enabled = TRUE;
extern struct resource dhd_wlan_resources;
extern struct wifi_platform_data dhd_wlan_control;
#else
static bool dts_enabled = FALSE;
struct resource dhd_wlan_resources = {0};
struct wifi_platform_data dhd_wlan_control = {0};
#endif /* CONFIG_OF && !defined(CONFIG_ARCH_MSM) */

static int dhd_wifi_platform_load(void);

extern void* wl_cfg80211_get_dhdp(void);

#ifdef ENABLE_4335BT_WAR
extern int bcm_bt_lock(int cookie);
extern void bcm_bt_unlock(int cookie);
static int lock_cookie_wifi = 'W' | 'i'<<8 | 'F'<<16 | 'i'<<24;	/* cookie is "WiFi" */
#endif /* ENABLE_4335BT_WAR */

bool builtin_roam_disabled;

wifi_adapter_info_t* dhd_wifi_platform_get_adapter(uint32 bus_type, uint32 bus_num, uint32 slot_num)
{
	int i;

	if (dhd_wifi_platdata == NULL)
		return NULL;

	for (i = 0; i < dhd_wifi_platdata->num_adapters; i++) {
		wifi_adapter_info_t *adapter = &dhd_wifi_platdata->adapters[i];
		if ((adapter->bus_type == -1 || adapter->bus_type == bus_type) &&
			(adapter->bus_num == -1 || adapter->bus_num == bus_num) &&
			(adapter->slot_num == -1 || adapter->slot_num == slot_num)) {
			DHD_TRACE(("found adapter info '%s'\n", adapter->name));
			return adapter;
		}
	}
	return NULL;
}

void* wifi_platform_prealloc(wifi_adapter_info_t *adapter, int section, unsigned long size)
{
	void *alloc_ptr = NULL;
	struct wifi_platform_data *plat_data;

	if (!adapter)
		return NULL;
	plat_data = adapter->wifi_plat_data;
	if (plat_data && plat_data->mem_prealloc) {
		alloc_ptr = plat_data->mem_prealloc(section, size);
		if (alloc_ptr) {
			DHD_INFO(("success alloc section %d\n", section));
			if (size != 0L)
				bzero(alloc_ptr, size);
			return alloc_ptr;
		}
		DHD_ERROR(("%s: failed to alloc static mem section %d\n", __FUNCTION__, section));
	}

	return NULL;
}

void* wifi_platform_get_prealloc_func_ptr(wifi_adapter_info_t *adapter)
{
	struct wifi_platform_data *plat_data;

	if (!adapter || !adapter->wifi_plat_data)
		return NULL;
	plat_data = adapter->wifi_plat_data;
	return plat_data->mem_prealloc;
}

int wifi_platform_get_irq_number(wifi_adapter_info_t *adapter, unsigned long *irq_flags_ptr)
{
	if (adapter == NULL)
		return -1;
	if (irq_flags_ptr)
		*irq_flags_ptr = adapter->intr_flags;
	return adapter->irq_num;
}

int wifi_platform_set_power(wifi_adapter_info_t *adapter, bool on, unsigned long msec)
{
	int err = 0;
	struct wifi_platform_data *plat_data;

	if (!adapter)
		return -EINVAL;
	plat_data = adapter->wifi_plat_data;

	DHD_ERROR(("%s = %d\n", __FUNCTION__, on));
#ifdef ENABLE_4335BT_WAR
	if (on) {
		printk("WiFi: trying to acquire BT lock\n");
		if (bcm_bt_lock(lock_cookie_wifi) != 0)
			printk("** WiFi: timeout in acquiring bt lock**\n");
		printk("%s: btlock acquired\n", __FUNCTION__);
	}
	else {
		/* For a exceptional case, release btlock */
		bcm_bt_unlock(lock_cookie_wifi);
	}
#endif /* ENABLE_4335BT_WAR */

#ifdef CONFIG_TEGRA_SYS_EDP
	if (on)
		sysedp_set_state(adapter->sysedpc, on);
#endif
	if (plat_data && plat_data->set_power)
		err = plat_data->set_power(on);
	else {
		if (gpio_is_valid(adapter->wlan_pwr))
			gpio_direction_output(adapter->wlan_pwr, on);
		if (gpio_is_valid(adapter->wlan_rst))
			gpio_direction_output(adapter->wlan_rst, on);
	}

#ifdef CONFIG_TEGRA_SYS_EDP
	if (!on)
		sysedp_set_state(adapter->sysedpc, on);
#endif
	if (msec && !err)
		OSL_SLEEP(msec);

	if (on && !err)
		is_power_on = TRUE;
	else
		is_power_on = FALSE;

	return err;
}

int wifi_dts_set_carddetect(wifi_adapter_info_t *adapter, bool device_present)
{
	struct platform_device *pdev = NULL;
	struct sdhci_host *host =  NULL;

	if (!adapter->sdhci_host)
		return -EINVAL;

	pdev = of_find_device_by_node(adapter->sdhci_host);
	if (!pdev)
		return -EINVAL;

	host = platform_get_drvdata(pdev);
	if (!host)
		return -EINVAL;

	DHD_INFO(("%s Calling %s card detect\n", __func__, mmc_hostname(host->mmc)));
	if (device_present == 1) {
		host->mmc->rescan_disable = 0;
		host->mmc->rescan_entered = 0;
		mmc_detect_change(host->mmc, 0);
	} else {
		host->mmc->detect_change = 0;
		host->mmc->rescan_disable = 1;
	}

	return 0;
}

int wifi_platform_bus_enumerate(wifi_adapter_info_t *adapter, bool device_present)
{
	int err = 0;
	struct wifi_platform_data *plat_data;

	if (!adapter)
		return -EINVAL;
	plat_data = adapter->wifi_plat_data;

	DHD_ERROR(("%s device present %d\n", __FUNCTION__, device_present));
	if (plat_data && plat_data->set_carddetect) {
		err = plat_data->set_carddetect(device_present);
	} else {
		err = wifi_dts_set_carddetect(adapter, device_present);
	}

	return err;
}

extern int wifi_get_mac_addr(unsigned char *buf);

int wifi_platform_get_mac_addr(wifi_adapter_info_t *adapter, unsigned char *buf)
{
	struct wifi_platform_data *plat_data;

	DHD_ERROR(("%s\n", __FUNCTION__));
	if (!buf || !adapter)
		return -EINVAL;

	/* The MAC address search order is:
	 * Userspace command (e.g. ifconfig)
	 * DTB (from NCT/EEPROM)
	 * File (FCT/rootfs)
	 * OTP
	 */
	if (wifi_get_mac_addr(buf) == 0)
		return 0;

	plat_data = adapter->wifi_plat_data;
	if (plat_data && plat_data->get_mac_addr)
		return plat_data->get_mac_addr(buf);

	return -EOPNOTSUPP;
}

void *wifi_platform_get_country_code(wifi_adapter_info_t *adapter, char *ccode)
{
	/* get_country_code was added after 2.6.39 */
#if	(LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39))
	struct wifi_platform_data *plat_data;

	if (!ccode || !adapter)
		return NULL;
	plat_data = adapter->wifi_plat_data;

	DHD_TRACE(("%s\n", __FUNCTION__));
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 58))
	if (plat_data && plat_data->get_country_code) {
		return plat_data->get_country_code(ccode,0);
	}
#else
	if (plat_data && plat_data->get_country_code) {
		return plat_data->get_country_code(ccode);

#endif  /* (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 58)) */
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39)) */

	return NULL;
}

#ifdef NV_COUNTRY_CODE
static int wifi_platform_get_country_code_map(struct device_node *node,
						wifi_adapter_info_t *adapter)
{
	struct device_node *np_country;
	struct device_node *child;
	struct cntry_locales_custom *country;
	int n_country;
	int ret;
	int i;
	const char *strptr;

	np_country = of_get_child_by_name(node, "country_code_map");
	if (!np_country) {
		DHD_ERROR(("%s: could not get country_code_map\n", __func__));
		return -1;
	}

	n_country = of_get_child_count(np_country);
	if (!n_country) {
		DHD_ERROR(("%s: n_country\n", __func__));
		return -1;
	}

	country = kmalloc(n_country * sizeof(struct cntry_locales_custom), GFP_KERNEL);
	if (!country) {
		DHD_ERROR(("%s: fail to allocate memory\n", __func__));
		return -1;
	}
	memset(country, 0, n_country * sizeof(struct cntry_locales_custom));

	i = 0;
	for_each_child_of_node(np_country, child) {
		ret = of_property_read_string(child, "iso_abbrev", &strptr);
		if (ret) {
			DHD_ERROR(("%s:read error iso_abbrev %s\n", __func__, child->name));
			goto fail;
		} else {
			strncpy(country[i].iso_abbrev, strptr, 3);
		}

		ret = of_property_read_string(child, "custom_locale", &strptr);
		if (ret) {
			DHD_ERROR(("%s:read error custom_locale  %s\n", __func__, child->name));
			goto fail;
		} else {
			strncpy(country[i].custom_locale, strptr, 3);
		}

		ret = of_property_read_u32(child, "custom_locale_rev", &country[i].custom_locale_rev);
		if (ret) {
			DHD_ERROR(("%s:read error custom_locale_rev %s\n", __func__, child->name));
			goto fail;
		}
		i++;
	}

	adapter->country_code_map = country;
	adapter->n_country = n_country;
	return 0;
fail:
	kfree(country);
	adapter->country_code_map = NULL;
	adapter->n_country = 0;
	return -1;
}

static void wifi_platform_free_country_code_map(wifi_adapter_info_t *adapter)
{
	if (adapter->country_code_map) {
		kfree(adapter->country_code_map);
		adapter->country_code_map = NULL;
	}
	adapter->n_country = 0;
}
#endif

static inline bool is_antenna_tuned(void)
{
	struct device_node *np;

	np = of_find_node_by_name(NULL, "wifi-antenna-tuning");
	return of_device_is_available(np);
}

static int wifi_plat_dev_drv_probe(struct platform_device *pdev)
{
	struct resource *resource;
	wifi_adapter_info_t *adapter;
	struct device_node *node;
	struct irq_data *irq_data;
	u32 irq_flags;
	int ret;

	/* Android style wifi platform data device ("bcmdhd_wlan" or "bcm4329_wlan")
	 * is kept for backward compatibility and supports only 1 adapter
	 */
	ASSERT(dhd_wifi_platdata != NULL);
	ASSERT(dhd_wifi_platdata->num_adapters == 1);
	adapter = &dhd_wifi_platdata->adapters[0];
	adapter->wifi_plat_data = (struct wifi_platform_data *)(pdev->dev.platform_data);

	if (pdev->dev.of_node) {
		node = pdev->dev.of_node;

		adapter->wlan_pwr = of_get_named_gpio(node, "wlan-pwr-gpio", 0);
		adapter->wlan_rst = of_get_named_gpio(node, "wlan-rst-gpio", 0);
		adapter->fw_path = of_get_property(node, "fw_path", NULL);
		adapter->nv_path = of_get_property(node, "nv_path", NULL);
		adapter->clm_blob_path = of_get_property(node, "clm_blob_path", NULL);
		adapter->sdhci_host = of_parse_phandle(node, "sdhci-host", 0);
		of_property_read_u32(node, "pwr-retry-cnt", &adapter->pwr_retry_cnt);
		builtin_roam_disabled = device_property_read_bool(&pdev->dev, "builtin-roam-disabled");

		if (is_antenna_tuned())
			adapter->nv_path = of_get_property(node,
						"tuned_nv_path", NULL);

		if (gpio_is_valid(adapter->wlan_pwr)) {
			ret = devm_gpio_request(&pdev->dev, adapter->wlan_pwr,
						"wlan_pwr");
			if (ret)
				DHD_ERROR(("Failed to request wlan_pwr gpio %d\n", adapter->wlan_pwr));
		}

		if (gpio_is_valid(adapter->wlan_rst)) {
			ret = devm_gpio_request(&pdev->dev, adapter->wlan_rst,
						"wlan_rst");
			if (ret)
				DHD_ERROR(("Failed to request wlan_rst gpio %d\n", adapter->wlan_rst));
		}

		/* This is to get the irq for the OOB */
		adapter->irq_num = platform_get_irq(pdev, 0);
		if (adapter->irq_num > -1) {
			irq_data = irq_get_irq_data(adapter->irq_num);
			irq_flags = irqd_get_trigger_type(irq_data);
			adapter->intr_flags = irq_flags & IRQF_TRIGGER_MASK;
		}
#ifdef CONFIG_TEGRA_SYS_EDP
		if (of_property_read_string(node, "edp-consumer-name", &adapter->edp_name)) {
			adapter->sysedpc = NULL;
			DHD_ERROR(("%s: property 'edp-consumer-name' missing or invalid\n",
									__FUNCTION__));
		} else {
			adapter->sysedpc = sysedp_create_consumer(node,
							adapter->edp_name);
		}
#endif
#ifdef NV_COUNTRY_CODE
		if (wifi_platform_get_country_code_map(node, adapter))
			DHD_ERROR(("%s:platform country code map is not available\n", __func__));
#endif
	} else {
		resource = platform_get_resource_byname(pdev, IORESOURCE_IRQ, "bcmdhd_wlan_irq");
		if (resource == NULL)
			resource = platform_get_resource_byname(pdev, IORESOURCE_IRQ, "bcm4329_wlan_irq");
		if (resource) {
			adapter->irq_num = resource->start;
			adapter->intr_flags = resource->flags & IRQF_TRIGGER_MASK;
		}
#ifdef CONFIG_TEGRA_SYS_EDP
		adapter->sysedpc = sysedp_create_consumer(pdev->dev.of_node,
							  "wifi");
#endif
	}

	wifi_plat_dev_probe_ret = dhd_wifi_platform_load();
	return wifi_plat_dev_probe_ret;
}

static int wifi_plat_dev_drv_remove(struct platform_device *pdev)
{
	wifi_adapter_info_t *adapter;

	/* Android style wifi platform data device ("bcmdhd_wlan" or "bcm4329_wlan")
	 * is kept for backward compatibility and supports only 1 adapter
	 */
	ASSERT(dhd_wifi_platdata != NULL);
	ASSERT(dhd_wifi_platdata->num_adapters == 1);
	adapter = &dhd_wifi_platdata->adapters[0];
	if (is_power_on) {
#ifdef BCMPCIE
		wifi_platform_bus_enumerate(adapter, FALSE);
		wifi_platform_set_power(adapter, FALSE, WIFI_TURNOFF_DELAY);
#else
		wifi_platform_set_power(adapter, FALSE, WIFI_TURNOFF_DELAY);
		wifi_platform_bus_enumerate(adapter, FALSE);
#endif /* BCMPCIE */
	}

#ifdef NV_COUNTRY_CODE
	wifi_platform_free_country_code_map(adapter);
#endif
#ifdef CONFIG_TEGRA_SYS_EDP
	sysedp_free_consumer(adapter->sysedpc);
	adapter->sysedpc = NULL;
#endif

	return 0;
}

static int wifi_plat_dev_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
	DHD_TRACE(("##> %s\n", __FUNCTION__));
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 39)) && defined(OOB_INTR_ONLY) && \
	defined(BCMSDIO)
	bcmsdh_oob_intr_set(0);
#endif /* (OOB_INTR_ONLY) */
	return 0;
}

static int wifi_plat_dev_drv_resume(struct platform_device *pdev)
{
	DHD_TRACE(("##> %s\n", __FUNCTION__));
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 39)) && defined(OOB_INTR_ONLY) && \
	defined(BCMSDIO)
	if (dhd_os_check_if_up(wl_cfg80211_get_dhdp()))
		bcmsdh_oob_intr_set(1);
#endif /* (OOB_INTR_ONLY) */
	return 0;
}

static const struct of_device_id wifi_device_dt_match[] = {
	{ .compatible = "android,bcmdhd_wlan", },
	{},
};

static struct platform_driver wifi_platform_dev_driver = {
	.probe          = wifi_plat_dev_drv_probe,
	.remove         = wifi_plat_dev_drv_remove,
	.suspend        = wifi_plat_dev_drv_suspend,
	.resume         = wifi_plat_dev_drv_resume,
	.driver         = {
	.name   = WIFI_PLAT_NAME,
	.of_match_table = wifi_device_dt_match,
	}
};

static struct platform_driver wifi_platform_dev_driver_legacy = {
	.probe          = wifi_plat_dev_drv_probe,
	.remove         = wifi_plat_dev_drv_remove,
	.suspend        = wifi_plat_dev_drv_suspend,
	.resume         = wifi_plat_dev_drv_resume,
	.driver         = {
	.name	= WIFI_PLAT_NAME2,
	}
};

static int wifi_platdev_match(struct device *dev, void *data)
{
	char *name = (char*)data;
	struct platform_device *pdev = to_platform_device(dev);

	if (strcmp(pdev->name, name) == 0) {
		DHD_ERROR(("found wifi platform device %s\n", name));
		return TRUE;
	}

	return FALSE;
}

static int wifi_ctrlfunc_register_drv(void)
{
	int err = 0;
	struct device *dev1, *dev2;
	struct device_node *dt_node;
	wifi_adapter_info_t *adapter;

	dev1 = bus_find_device(&platform_bus_type, NULL, WIFI_PLAT_NAME, wifi_platdev_match);
	dev2 = bus_find_device(&platform_bus_type, NULL, WIFI_PLAT_NAME2, wifi_platdev_match);
	dt_node = of_find_compatible_node(NULL, NULL, "android,bcmdhd_wlan");

	if (dev1 == NULL && dev2 == NULL && dt_node == NULL) {
		DHD_ERROR(("no wifi platform data, skip\n"));
		return -ENXIO;
	}

	/* multi-chip support not enabled, build one adapter information for
	 * DHD (either SDIO, USB or PCIe)
	 */
	adapter = kzalloc(sizeof(wifi_adapter_info_t), GFP_KERNEL);
	adapter->name = "DHD generic adapter";
	adapter->bus_type = -1;
	adapter->bus_num = -1;
	adapter->slot_num = -1;
	adapter->irq_num = -1;
	is_power_on = FALSE;
	wifi_plat_dev_probe_ret = 0;
	dhd_wifi_platdata = kzalloc(sizeof(bcmdhd_wifi_platdata_t), GFP_KERNEL);
	dhd_wifi_platdata->num_adapters = 1;
	dhd_wifi_platdata->adapters = adapter;

	if (dev1 || dt_node) {
		err = platform_driver_register(&wifi_platform_dev_driver);
		if (err) {
			DHD_ERROR(("%s: failed to register wifi ctrl func driver\n",
				__FUNCTION__));
			return err;
		}
	}
	if (dev2) {
		err = platform_driver_register(&wifi_platform_dev_driver_legacy);
		if (err) {
			DHD_ERROR(("%s: failed to register wifi ctrl func legacy driver\n",
				__FUNCTION__));
			return err;
		}
	}

	if (dts_enabled) {
		struct resource *resource;
		adapter->wifi_plat_data = (void *)&dhd_wlan_control;
		resource = &dhd_wlan_resources;
		adapter->irq_num = resource->start;
		adapter->intr_flags = resource->flags & IRQF_TRIGGER_MASK;
		wifi_plat_dev_probe_ret = dhd_wifi_platform_load();
	}

	/* return probe function's return value if registeration succeeded */
	return wifi_plat_dev_probe_ret;
}

void wifi_ctrlfunc_unregister_drv(void)
{

	struct device *dev1, *dev2;
	struct device_node *dt_node;
	dev1 = bus_find_device(&platform_bus_type, NULL, WIFI_PLAT_NAME, wifi_platdev_match);
	dev2 = bus_find_device(&platform_bus_type, NULL, WIFI_PLAT_NAME2, wifi_platdev_match);
	dt_node = of_find_compatible_node(NULL, NULL, "android,bcmdhd_wlan");
	if (dev1 == NULL && dev2 == NULL && dt_node == NULL)
		return;

	DHD_ERROR(("unregister wifi platform drivers\n"));
	if (dev1 || dt_node)
		platform_driver_unregister(&wifi_platform_dev_driver);
	if (dev2)
		platform_driver_unregister(&wifi_platform_dev_driver_legacy);
	if (dts_enabled) {
		wifi_adapter_info_t *adapter;
		adapter = &dhd_wifi_platdata->adapters[0];
		if (is_power_on) {
			wifi_platform_set_power(adapter, FALSE, WIFI_TURNOFF_DELAY);
			wifi_platform_bus_enumerate(adapter, FALSE);
		}
	}

	kfree(dhd_wifi_platdata->adapters);
	dhd_wifi_platdata->adapters = NULL;
	dhd_wifi_platdata->num_adapters = 0;
	kfree(dhd_wifi_platdata);
	dhd_wifi_platdata = NULL;
}

static int bcmdhd_wifi_plat_dev_drv_probe(struct platform_device *pdev)
{
	dhd_wifi_platdata = (bcmdhd_wifi_platdata_t *)(pdev->dev.platform_data);

	return dhd_wifi_platform_load();
}

static int bcmdhd_wifi_plat_dev_drv_remove(struct platform_device *pdev)
{
	int i;
	wifi_adapter_info_t *adapter;
	ASSERT(dhd_wifi_platdata != NULL);

	/* power down all adapters */
	for (i = 0; i < dhd_wifi_platdata->num_adapters; i++) {
		adapter = &dhd_wifi_platdata->adapters[i];
		wifi_platform_set_power(adapter, FALSE, WIFI_TURNOFF_DELAY);
		wifi_platform_bus_enumerate(adapter, FALSE);
	}
	return 0;
}

static struct platform_driver dhd_wifi_platform_dev_driver = {
	.probe          = bcmdhd_wifi_plat_dev_drv_probe,
	.remove         = bcmdhd_wifi_plat_dev_drv_remove,
	.driver         = {
	.name   = WIFI_PLAT_EXT,
	}
};

int dhd_wifi_platform_register_drv(void)
{
	int err = 0;
	struct device *dev;

	/* register Broadcom wifi platform data driver if multi-chip is enabled,
	 * otherwise use Android style wifi platform data (aka wifi control function)
	 * if it exists
	 *
	 * to support multi-chip DHD, Broadcom wifi platform data device must
	 * be added in kernel early boot (e.g. board config file).
	 */
	if (cfg_multichip) {
		dev = bus_find_device(&platform_bus_type, NULL, WIFI_PLAT_EXT, wifi_platdev_match);
		if (dev == NULL) {
			DHD_ERROR(("bcmdhd wifi platform data device not found!!\n"));
			return -ENXIO;
		}
		err = platform_driver_register(&dhd_wifi_platform_dev_driver);
	} else {
		err = wifi_ctrlfunc_register_drv();

		/* no wifi ctrl func either, load bus directly and ignore this error */
		if (err) {
			if (err == -ENXIO) {
				/* wifi ctrl function does not exist */
				err = dhd_wifi_platform_load();
			} else {
				/* unregister driver due to initialization failure */
				wifi_ctrlfunc_unregister_drv();
			}
		}
	}

	return err;
}

#ifdef BCMPCIE
static int dhd_wifi_platform_load_pcie(void)
{
	int err = 0;
	int i;
	wifi_adapter_info_t *adapter;

	BCM_REFERENCE(i);
	BCM_REFERENCE(adapter);

	if (dhd_wifi_platdata == NULL) {
		err = dhd_bus_register();
	} else {
#ifndef CUSTOMER_HW5
		if (dhd_download_fw_on_driverload) {
#else
		{
#endif	/* CUSTOMER_HW5 */
			/* power up all adapters */
			for (i = 0; i < dhd_wifi_platdata->num_adapters; i++) {
				int retry = POWERUP_MAX_RETRY;
				adapter = &dhd_wifi_platdata->adapters[i];

				DHD_ERROR(("Power-up adapter '%s'\n", adapter->name));
				DHD_INFO((" - irq %d [flags %d], firmware: %s, nvram: %s\n",
					adapter->irq_num, adapter->intr_flags, adapter->fw_path,
					adapter->nv_path));
				DHD_INFO((" - bus type %d, bus num %d, slot num %d\n\n",
					adapter->bus_type, adapter->bus_num, adapter->slot_num));

				do {
					err = wifi_platform_set_power(adapter,
						TRUE, WIFI_TURNON_DELAY);
					if (err) {
						DHD_ERROR(("failed to power up %s,"
							" %d retry left\n",
							adapter->name, retry));
#ifdef CONFIG_BCMDHD_CUSTOM_SYSFS_TEGRA
						TEGRA_SYSFS_HISTOGRAM_STAT_INC(wifi_on_retry);
#endif
						/* WL_REG_ON state unknown, Power off forcely */
						wifi_platform_set_power(adapter,
							FALSE, WIFI_TURNOFF_DELAY);
						continue;
					} else {
						err = wifi_platform_bus_enumerate(adapter, TRUE);
						if (err) {
							DHD_ERROR(("failed to enumerate bus %s, "
								"%d retry left\n",
								adapter->name, retry));
#ifdef CONFIG_BCMDHD_CUSTOM_SYSFS_TEGRA
							TEGRA_SYSFS_HISTOGRAM_STAT_INC(wifi_on_retry);
#endif
							wifi_platform_set_power(adapter, FALSE,
								WIFI_TURNOFF_DELAY);
						} else {
							break;
						}
					}
				} while (retry--);

				if (!retry) {
					DHD_ERROR(("failed to power up %s, max retry reached**\n",
						adapter->name));
#ifdef CONFIG_BCMDHD_CUSTOM_SYSFS_TEGRA
					TEGRA_SYSFS_HISTOGRAM_STAT_INC(wifi_on_fail);
#endif
					return -ENODEV;
				}
			}
#ifdef CONFIG_BCMDHD_CUSTOM_SYSFS_TEGRA
			TEGRA_SYSFS_HISTOGRAM_STAT_INC(wifi_on_success);
#endif
		}

		err = dhd_bus_register();

		if (err) {
			DHD_ERROR(("%s: pcie_register_driver failed\n", __FUNCTION__));
#ifndef CUSTOMER_HW5
			if (dhd_download_fw_on_driverload) {
#else
			{
#endif	/* CUSTOMER_HW5 */
				/* power down all adapters */
				for (i = 0; i < dhd_wifi_platdata->num_adapters; i++) {
					adapter = &dhd_wifi_platdata->adapters[i];
					wifi_platform_bus_enumerate(adapter, FALSE);
					wifi_platform_set_power(adapter,
						FALSE, WIFI_TURNOFF_DELAY);
				}
			}
		}
	}

	return err;
}
#else
static int dhd_wifi_platform_load_pcie(void)
{
	return 0;
}
#endif /* BCMPCIE  */


void dhd_wifi_platform_unregister_drv(void)
{
	if (cfg_multichip)
		platform_driver_unregister(&dhd_wifi_platform_dev_driver);
	else
		wifi_ctrlfunc_unregister_drv();
}

extern int dhd_watchdog_prio;
extern int dhd_dpc_prio;
extern uint dhd_deferred_tx;
#if defined(BCMLXSDMMC)
extern struct semaphore dhd_registration_sem;
#endif 

#ifdef BCMSDIO
void dhd_mmc_power_restore_sdhci_host(struct device_node *dn)
{
	struct platform_device *pdev = NULL;
	struct sdhci_host *host =  NULL;

	if (!dn) {
		DHD_ERROR(("%s: sdhci_host is NULL\n", __FUNCTION__));
		return;
	}

	pdev = of_find_device_by_node(dn);
	if (IS_ERR_OR_NULL(pdev)) {
		DHD_ERROR(("%s: pdev=%ld\n", __FUNCTION__, PTR_ERR(pdev)));
		return;
	}

	host = platform_get_drvdata(pdev);
	if (IS_ERR_OR_NULL(host)) {
		DHD_ERROR(("%s: mmc_host=%ld\n", __FUNCTION__, PTR_ERR(host)));
		return;
	}

	if (dhd_mmc_power_restore_host(host->mmc)) {
		DHD_ERROR(("%s: mmc_restore fail\n", __FUNCTION__));
		return;
	}
}

static int dhd_wifi_platform_load_sdio(void)
{
	int i;
	int err = 0;
	wifi_adapter_info_t *adapter;

	BCM_REFERENCE(i);
	BCM_REFERENCE(adapter);
	/* Sanity check on the module parameters
	 * - Both watchdog and DPC as tasklets are ok
	 * - If both watchdog and DPC are threads, TX must be deferred
	 */
	if (!(dhd_watchdog_prio < 0 && dhd_dpc_prio < 0) &&
		!(dhd_watchdog_prio >= 0 && dhd_dpc_prio >= 0 && dhd_deferred_tx))
		return -EINVAL;

#if defined(BCMLXSDMMC)
	sema_init(&dhd_registration_sem, 0);
#endif
#if defined(BCMLXSDMMC)
	if (dhd_wifi_platdata == NULL) {
		DHD_ERROR(("DHD wifi platform data is required for Android build\n"));
		return -EINVAL;
	}

	/* power up all adapters */
	for (i = 0; i < dhd_wifi_platdata->num_adapters; i++) {
		bool chip_up = FALSE;
		int retry;
		static struct semaphore dhd_chipup_sem;

		adapter = &dhd_wifi_platdata->adapters[i];

		retry = adapter->pwr_retry_cnt;
		DHD_ERROR(("Power-up adapter '%s'\n", adapter->name));
		DHD_INFO((" - irq %d [flags %d], firmware: %s, nvram: %s\n",
			adapter->irq_num, adapter->intr_flags, adapter->fw_path, adapter->nv_path));
		DHD_INFO((" - bus type %d, bus num %d, slot num %d\n\n",
			adapter->bus_type, adapter->bus_num, adapter->slot_num));

		dhd_mmc_power_restore_sdhci_host(adapter->sdhci_host);

		do {
			sema_init(&dhd_chipup_sem, 0);
			err = wifi_platform_set_power(adapter, TRUE, WIFI_TURNON_DELAY);
			if (err) {
				/* WL_REG_ON state unknown, Power off forcely */
				wifi_platform_set_power(adapter, FALSE, WIFI_TURNOFF_DELAY);
				continue;
			} else {
				wifi_platform_bus_enumerate(adapter, TRUE);
				err = 0;
			}

			err = dhd_bus_reg_sdio_notify(&dhd_chipup_sem);
			if (err) {
				DHD_ERROR(("%s dhd_bus_reg_sdio_notify fail(%d)\n\n",
					__FUNCTION__, err));
				return err;
			}

			if (down_timeout(&dhd_chipup_sem, msecs_to_jiffies(POWERUP_WAIT_MS)) == 0) {
				dhd_bus_unreg_sdio_notify();
				chip_up = TRUE;
				break;
			}

			DHD_ERROR(("failed to power up %s, %d retry left\n", adapter->name, retry));
#ifdef CONFIG_BCMDHD_CUSTOM_SYSFS_TEGRA
			TEGRA_SYSFS_HISTOGRAM_STAT_INC(wifi_on_retry);
#endif
			dhd_bus_unreg_sdio_notify();
			wifi_platform_set_power(adapter, FALSE, WIFI_TURNOFF_DELAY);
			wifi_platform_bus_enumerate(adapter, FALSE);
		} while (retry--);

		if (!chip_up) {
			DHD_ERROR(("failed to power up %s, max retry reached**\n", adapter->name));
#ifdef CONFIG_BCMDHD_CUSTOM_SYSFS_TEGRA
			TEGRA_SYSFS_HISTOGRAM_STAT_INC(wifi_on_fail);
#endif
			return -ENODEV;
		}
#ifdef CONFIG_BCMDHD_CUSTOM_SYSFS_TEGRA
		TEGRA_SYSFS_HISTOGRAM_STAT_INC(wifi_on_success);
#endif

	}

	err = dhd_bus_register();

	if (err) {
		DHD_ERROR(("%s: sdio_register_driver failed\n", __FUNCTION__));
		goto fail;
	}


	/*
	 * Wait till MMC sdio_register_driver callback called and made driver attach.
	 * It's needed to make sync up exit from dhd insmod  and
	 * Kernel MMC sdio device callback registration
	 */
	err = down_timeout(&dhd_registration_sem, msecs_to_jiffies(DHD_REGISTRATION_TIMEOUT));
	if (err) {
		DHD_ERROR(("%s: sdio_register_driver timeout or error \n", __FUNCTION__));
		dhd_bus_unregister();
		goto fail;
	}

	return err;

fail:
	/* power down all adapters */
	for (i = 0; i < dhd_wifi_platdata->num_adapters; i++) {
		adapter = &dhd_wifi_platdata->adapters[i];
		wifi_platform_set_power(adapter, FALSE, WIFI_TURNOFF_DELAY);
		wifi_platform_bus_enumerate(adapter, FALSE);
	}
#else

	/* x86 bring-up PC needs no power-up operations */
	err = dhd_bus_register();

#endif 

	return err;
}
#else /* BCMSDIO */
static int dhd_wifi_platform_load_sdio(void)
{
	return 0;
}
#endif /* BCMSDIO */

static int dhd_wifi_platform_load_usb(void)
{
	return 0;
}

/* net_if_lock lock protects platform driver probe from IFUP */
DEFINE_MUTEX(net_if_lock);

static int dhd_wifi_platform_load()
{
	int err = 0;

	mutex_lock(&net_if_lock);
	wl_android_init();

	if ((err = dhd_wifi_platform_load_usb()))
		goto end;
	else if ((err = dhd_wifi_platform_load_sdio()))
		goto end;
	else
		err = dhd_wifi_platform_load_pcie();

end:
	if (err)
		wl_android_exit();
	else
		wl_android_post_init();

	mutex_unlock(&net_if_lock);

	return err;
}
