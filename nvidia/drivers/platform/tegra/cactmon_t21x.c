/*
 * Copyright (C) 2016-2017, NVIDIA Corporation. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/reset.h>
#include <linux/delay.h>
#include <linux/platform/tegra/emc_bwmgr.h>
#include <linux/platform/tegra/actmon_common.h>

static struct actmon_drv_data *actmon;

#define aclk(x)	((struct clk *) x)

/************ START OF REG DEFINITION **************/
#define ACTMON_GLB_STATUS			0x00
#define ACTMON_GLB_PERIOD_CTRL			0x04

#define ACTMON_DEV_CTRL				0x00
#define ACTMON_DEV_CTRL_ENB			(0x1 << 31)
#define ACTMON_DEV_CTRL_UP_WMARK_ENB		(0x1 << 30)
#define ACTMON_DEV_CTRL_DOWN_WMARK_ENB		(0x1 << 29)
#define ACTMON_DEV_CTRL_UP_WMARK_NUM_SHIFT	26
#define ACTMON_DEV_CTRL_UP_WMARK_NUM_MASK	(0x7 <<	26)
#define ACTMON_DEV_CTRL_DOWN_WMARK_NUM_SHIFT	23
#define ACTMON_DEV_CTRL_DOWN_WMARK_NUM_MASK	(0x7 <<	23)
#define ACTMON_DEV_CTRL_AVG_UP_WMARK_ENB	(0x1 << 21)
#define ACTMON_DEV_CTRL_AVG_DOWN_WMARK_ENB	(0x1 << 20)
#define ACTMON_DEV_CTRL_PERIODIC_ENB		(0x1 << 18)
#define ACTMON_DEV_CTRL_K_VAL_SHIFT		10
#define ACTMON_DEV_CTRL_K_VAL_MASK		(0x7 << 10)

#define ACTMON_DEV_UP_WMARK			0x04
#define ACTMON_DEV_DOWN_WMARK			0x08
#define ACTMON_DEV_INIT_AVG			0x0c
#define ACTMON_DEV_AVG_UP_WMARK			0x10
#define ACTMON_DEV_AVG_DOWN_WMARK			0x14

#define ACTMON_DEV_COUNT_WEGHT			0x18
#define ACTMON_DEV_COUNT			0x1c
#define ACTMON_DEV_AVG_COUNT			0x20

#define ACTMON_DEV_INTR_STATUS			0x24
#define ACTMON_DEV_INTR_UP_WMARK		(0x1 << 31)
#define ACTMON_DEV_INTR_DOWN_WMARK		(0x1 << 30)
#define ACTMON_DEV_INTR_AVG_DOWN_WMARK		(0x1 << 25)
#define ACTMON_DEV_INTR_AVG_UP_WMARK		(0x1 << 24)

/************ END OF REG DEFINITION **************/

/******** actmon register operations start **********/
static void set_prd_t21x(u32 val, void __iomem *base)
{
	__raw_writel(val, base + ACTMON_GLB_PERIOD_CTRL);
}

static u32 get_glb_intr_st(void __iomem *base)
{
	return __raw_readl(base + ACTMON_GLB_STATUS);
}

/********* actmon register operations end ***********/

/******** actmon device register operations start **********/
static void set_init_avg(u32 val, void __iomem *base)
{
	__raw_writel(val, base + ACTMON_DEV_INIT_AVG);
}

static void set_avg_up_wm(u32 val, void __iomem *base)
{
	__raw_writel(val, base + ACTMON_DEV_AVG_UP_WMARK);
}

static void set_avg_dn_wm(u32 val, void __iomem *base)
{
	__raw_writel(val, base + ACTMON_DEV_AVG_DOWN_WMARK);
}

static void set_dev_up_wm(u32 val, void __iomem *base)
{
	__raw_writel(val, base + ACTMON_DEV_UP_WMARK);
}

static void set_dev_dn_wm(u32 val, void __iomem *base)
{
	__raw_writel(val, base + ACTMON_DEV_DOWN_WMARK);
}

static void set_cnt_wt(u32 val, void __iomem *base)
{
	__raw_writel(val, base + ACTMON_DEV_COUNT_WEGHT);
}

static void set_intr_st(u32 val, void __iomem *base)
{
	__raw_writel(val, base + ACTMON_DEV_INTR_STATUS);
}

static u32 get_intr_st(void __iomem *base)
{
	return __raw_readl(base + ACTMON_DEV_INTR_STATUS);
}

static void init_dev_cntrl(struct actmon_dev *dev, void __iomem *base)
{
	u32 val = 0;

	val |= ACTMON_DEV_CTRL_PERIODIC_ENB;
	val |= (((dev->avg_window_log2 - 1) << ACTMON_DEV_CTRL_K_VAL_SHIFT)
		& ACTMON_DEV_CTRL_K_VAL_MASK);
	val |= (((dev->down_wmark_window - 1) <<
		ACTMON_DEV_CTRL_DOWN_WMARK_NUM_SHIFT) &
		ACTMON_DEV_CTRL_DOWN_WMARK_NUM_MASK);
	val |=  (((dev->up_wmark_window - 1) <<
		ACTMON_DEV_CTRL_UP_WMARK_NUM_SHIFT) &
		ACTMON_DEV_CTRL_UP_WMARK_NUM_MASK);

	__raw_writel(val, base + ACTMON_DEV_CTRL);
}

static void enb_dev_intr_all(void __iomem *base)
{
	u32 val = __raw_readl(base + ACTMON_DEV_CTRL);

	val |= (ACTMON_DEV_CTRL_UP_WMARK_ENB |
			ACTMON_DEV_CTRL_DOWN_WMARK_ENB |
			ACTMON_DEV_CTRL_AVG_UP_WMARK_ENB |
			ACTMON_DEV_CTRL_AVG_DOWN_WMARK_ENB);

	__raw_writel(val, base + ACTMON_DEV_CTRL);
}

static void enb_dev_intr(u32 val, void __iomem *base)
{
	__raw_writel(val, base + ACTMON_DEV_CTRL);
}

static u32 get_dev_intr(void __iomem *base)
{
	return __raw_readl(base + ACTMON_DEV_CTRL);
}

static u32 get_avg_cnt(void __iomem *base)
{
	return __raw_readl(base + ACTMON_DEV_AVG_COUNT);
}

static u32 get_raw_cnt(void __iomem *base)
{
	return __raw_readl(base + ACTMON_DEV_COUNT);
}

static void enb_dev_wm(u32 *val)
{
	*val |= (ACTMON_DEV_CTRL_UP_WMARK_ENB |
			ACTMON_DEV_CTRL_DOWN_WMARK_ENB);
}

static void disb_dev_up_wm(u32 *val)
{
	*val &= ~ACTMON_DEV_CTRL_UP_WMARK_ENB;
}

static void disb_dev_dn_wm(u32 *val)
{
	*val &= ~ACTMON_DEV_CTRL_DOWN_WMARK_ENB;
}

/*********end of actmon device register operations***********/
static void actmon_dev_reg_ops_init(struct actmon_dev *adev)
{
	adev->ops.set_init_avg = set_init_avg;
	adev->ops.set_avg_up_wm = set_avg_up_wm;
	adev->ops.set_avg_dn_wm = set_avg_dn_wm;
	adev->ops.set_dev_up_wm = set_dev_up_wm;
	adev->ops.set_dev_dn_wm = set_dev_dn_wm;
	adev->ops.set_cnt_wt = set_cnt_wt;
	adev->ops.set_intr_st = set_intr_st;
	adev->ops.get_intr_st = get_intr_st;
	adev->ops.init_dev_cntrl = init_dev_cntrl;
	adev->ops.enb_dev_intr_all = enb_dev_intr_all;
	adev->ops.enb_dev_intr = enb_dev_intr;
	adev->ops.get_dev_intr_enb = get_dev_intr;
	adev->ops.get_avg_cnt = get_avg_cnt;
	adev->ops.get_raw_cnt = get_raw_cnt;
	adev->ops.enb_dev_wm = enb_dev_wm;
	adev->ops.disb_dev_up_wm = disb_dev_up_wm;
	adev->ops.disb_dev_dn_wm = disb_dev_dn_wm;
}

static unsigned long actmon_dev_get_rate(struct actmon_dev *adev)
{
	return tegra_bwmgr_get_emc_rate();
}

static unsigned long actmon_dev_post_change_rate(
		struct actmon_dev *adev, void *cclk)
{
	struct clk_notifier_data *clk_data = cclk;

	return clk_data->new_rate;
}

static void actmon_dev_set_rate(struct actmon_dev *adev,
		unsigned long freq)
{
	struct tegra_bwmgr_client *bwclnt = (struct tegra_bwmgr_client *)
		adev->clnt;

	tegra_bwmgr_set_emc(bwclnt, freq * 1000,
			TEGRA_BWMGR_SET_EMC_FLOOR);
}

static int cactmon_bwmgr_register_t21x(
		struct actmon_dev *adev, struct platform_device *pdev)
{
	struct tegra_bwmgr_client *bwclnt = (struct tegra_bwmgr_client *)
		adev->clnt;
	struct device *mon_dev = &pdev->dev;
	int ret = 0;

	bwclnt = tegra_bwmgr_register(TEGRA_BWMGR_CLIENT_MON);
	if (IS_ERR_OR_NULL(bwclnt)) {
		ret = -ENODEV;
		dev_err(mon_dev, "emc bw manager registration failed for %s\n",
				adev->dn->name);
		return ret;
	}

	adev->clnt = bwclnt;

	return ret;
}

static void cactmon_bwmgr_unregister_t21x(
		struct actmon_dev *adev, struct platform_device *pdev)
{
	struct tegra_bwmgr_client *bwclnt = (struct tegra_bwmgr_client *)
		adev->clnt;
	struct device *mon_dev = &pdev->dev;

	if (bwclnt) {
		dev_dbg(mon_dev, "unregistering BW manager for %s\n",
				adev->dn->name);
		tegra_bwmgr_unregister(bwclnt);
		adev->clnt = NULL;
	}
}

static int actmon_dev_platform_init_t21x(struct actmon_dev *adev,
		struct platform_device *pdev)
{
	int ret = 0;

	ret = cactmon_bwmgr_register_t21x(adev, pdev);
	if (ret)
		goto end;

	adev->dev_name = adev->dn->name;
	adev->max_freq = tegra_bwmgr_get_max_emc_rate();
	tegra_bwmgr_set_emc((struct tegra_bwmgr_client *)adev->clnt,
			    adev->max_freq, TEGRA_BWMGR_SET_EMC_FLOOR);
	adev->max_freq /= 1000;
	actmon_dev_reg_ops_init(adev);
	adev->actmon_dev_set_rate = actmon_dev_set_rate;
	adev->actmon_dev_get_rate = actmon_dev_get_rate;

	if (adev->rate_change_nb.notifier_call) {
		ret = tegra_bwmgr_notifier_register(&adev->rate_change_nb);
		if (ret) {
			pr_err("Tegra BWMGR notifier register failed for %s\n",
					adev->dev_name);
			return ret;
		}
	}

	adev->actmon_dev_post_change_rate = actmon_dev_post_change_rate;

end:
	return ret;
}

/********* actmon register operations end ***********/

static void actmon_reg_ops_init(struct platform_device *pdev)
{
	struct actmon_drv_data *d = platform_get_drvdata(pdev);

	d->ops.set_sample_prd = set_prd_t21x;
	d->ops.set_glb_intr = NULL;
	d->ops.get_glb_intr_st = get_glb_intr_st;
}

static void cactmon_free_resource_t21x(
		struct actmon_dev *adev, struct platform_device *pdev)
{
	int ret;

	if (adev->rate_change_nb.notifier_call) {
		ret = tegra_bwmgr_notifier_unregister(&adev->rate_change_nb);
		if (ret) {
			pr_err("Failed to register bw manager rate change \
				notifier for %s\n", adev->dev_name);
		}
	}
	cactmon_bwmgr_unregister_t21x(adev, pdev);
}

static int cactmon_reset_deinit_t21x(struct platform_device *pdev)
{
	struct actmon_drv_data *actmon = platform_get_drvdata(pdev);
	struct device *mon_dev = &pdev->dev;
	int ret = -EINVAL;

	if (actmon->actmon_rst) {
		ret = reset_control_assert(actmon->actmon_rst);
		if (ret)
			dev_err(mon_dev, "failed to assert actmon\n");
	}

	return ret;
}

static int cactmon_reset_init_t21x(struct platform_device *pdev)
{
	struct actmon_drv_data *actmon = platform_get_drvdata(pdev);
	struct device *mon_dev = &pdev->dev;
	int ret = 0;

	actmon->actmon_rst = devm_reset_control_get(mon_dev, "actmon");
 	if (IS_ERR(actmon->actmon_rst)) {
 		ret = PTR_ERR(actmon->actmon_rst);
		dev_err(mon_dev, "can not get actmon reset%d\n", ret);
		goto end;
 	}

 	/* Reset ACTMON */
	ret = reset_control_assert(actmon->actmon_rst);
	if (ret) {
		dev_err(mon_dev, "failed to deassert actmon\n");
		goto end;
	}

 	udelay(10);
	ret = reset_control_deassert(actmon->actmon_rst);
	if (ret)
		dev_err(mon_dev, "failed to deassert actmon\n");

end:
	return ret;
}

static int cactmon_clk_disable_t21x(struct platform_device *pdev)
{
	struct actmon_drv_data *actmon = platform_get_drvdata(pdev);
	struct device *mon_dev = &pdev->dev;
	int ret = 0;

	if (actmon->actmon_clk) {
		clk_disable_unprepare(actmon->actmon_clk);
		actmon->actmon_clk = NULL;
		dev_dbg(mon_dev, "actmon clocks disabled\n");
	}

	return ret;
}

static int cactmon_clk_enable_t21x(struct platform_device *pdev)
{
	struct actmon_drv_data *actmon = platform_get_drvdata(pdev);
	struct device *mon_dev = &pdev->dev;
	int ret = 0;

	actmon->actmon_clk = devm_clk_get(mon_dev, "actmon");
	if (IS_ERR_OR_NULL(actmon->actmon_clk)) {
		dev_err(mon_dev, "unable to find actmon clock\n");
		ret = PTR_ERR(actmon->actmon_clk);
		return ret;
	}

	ret = clk_prepare_enable(actmon->actmon_clk);
	if (ret) {
		dev_err(mon_dev, "unable to enable actmon clock\n");
		return ret;
	}
	actmon->freq = clk_get_rate(actmon->actmon_clk) / 1000;

	return ret;
}

static int __init actmon_platform_init_t21x(struct platform_device *pdev)
{
	actmon->clock_init = cactmon_clk_enable_t21x;
	actmon->clock_deinit = cactmon_clk_disable_t21x;
	actmon->reset_init = cactmon_reset_init_t21x;
	actmon->reset_deinit = cactmon_reset_deinit_t21x;
	actmon->dev_free_resource = cactmon_free_resource_t21x;
	actmon->actmon_dev_platform_init = actmon_dev_platform_init_t21x;
	actmon_reg_ops_init(pdev);
	return 0;
}

static int __init tegra21x_actmon_probe(struct platform_device *pdev)
{
	int ret = 0;

	actmon = devm_kzalloc(&pdev->dev, sizeof(*actmon),
				GFP_KERNEL);
	if (!actmon) {
		ret = -ENOMEM;
		goto err_out;
	}
	platform_set_drvdata(pdev, actmon);
	actmon_platform_init_t21x(pdev);
	actmon->pdev = pdev;
	ret = tegra_actmon_register(actmon);
err_out:
	return ret;
}

static int tegra21x_actmon_remove(struct platform_device *pdev)
{
	tegra_actmon_remove(pdev);
	return 0;
}

static const struct of_device_id tegra21x_actmon_of[] = {
	{ .compatible = "nvidia,tegra210-cactmon", .data = NULL, },
	{},
};

static struct platform_driver tegra21x_actmon_driver __refdata = {
	.probe		= tegra21x_actmon_probe,
	.remove		= tegra21x_actmon_remove,
	.driver	= {
		.name	= "tegra21x_actmon",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(tegra21x_actmon_of),
	},
};

module_platform_driver(tegra21x_actmon_driver);
