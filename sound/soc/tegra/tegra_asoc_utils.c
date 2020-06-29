// SPDX-License-Identifier: GPL-2.0-only
/*
 * tegra_asoc_utils.c - Harmony machine ASoC driver
 *
 * Author: Stephen Warren <swarren@nvidia.com>
 * Copyright (C) 2010,2012 - NVIDIA, Inc.
 */

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>

#include "tegra_asoc_utils.h"

/*
 * this will be used for platforms from Tegra210 onwards.
 * odd rates: sample rates multiple of 11.025kHz
 * even_rates: sample rates multiple of 8kHz
 */
enum rate_type {
	ODD_RATE,
	EVEN_RATE,
	NUM_RATE_TYPE,
};
unsigned int tegra210_pll_base_rate[NUM_RATE_TYPE] = {338688000, 368640000};
unsigned int tegra186_pll_base_rate[NUM_RATE_TYPE] = {270950400, 245760000};
unsigned int default_pll_out_rate[NUM_RATE_TYPE] = {45158400, 49152000};

int tegra_asoc_utils_set_rate(struct tegra_asoc_utils_data *data, int srate,
			      int mclk)
{
	int new_baseclock;
	bool clk_change;
	int err;

	switch (srate) {
	case 11025:
	case 22050:
	case 44100:
	case 88200:
		if (data->soc == TEGRA_ASOC_UTILS_SOC_TEGRA20)
			new_baseclock = 56448000;
		else if (data->soc == TEGRA_ASOC_UTILS_SOC_TEGRA30)
			new_baseclock = 564480000;
		else
			new_baseclock = 282240000;
		break;
	case 8000:
	case 16000:
	case 32000:
	case 48000:
	case 64000:
	case 96000:
		if (data->soc == TEGRA_ASOC_UTILS_SOC_TEGRA20)
			new_baseclock = 73728000;
		else if (data->soc == TEGRA_ASOC_UTILS_SOC_TEGRA30)
			new_baseclock = 552960000;
		else
			new_baseclock = 368640000;
		break;
	default:
		return -EINVAL;
	}

	clk_change = ((new_baseclock != data->set_baseclock) ||
			(mclk != data->set_mclk));
	if (!clk_change)
		return 0;

	data->set_baseclock = 0;
	data->set_mclk = 0;

<<<<<<< HEAD
	clk_disable_unprepare(data->clk_aud_mclk);
	clk_disable_unprepare(data->clk_pll_out);
	clk_disable_unprepare(data->clk_pll);
=======
	clk_disable_unprepare(data->clk_cdev1);
>>>>>>> v5.8-rc3

	err = clk_set_rate(data->clk_pll, new_baseclock);
	if (err) {
		dev_err(data->dev, "Can't set base pll rate: %d\n", err);
		return err;
	}

	err = clk_set_rate(data->clk_pll_out, mclk);
	if (err) {
		dev_err(data->dev, "Can't set pll_out rate: %d\n", err);
		return err;
	}

	/* Don't set cdev1/extern1 rate; it's locked to pll_out */

<<<<<<< HEAD
	err = clk_prepare_enable(data->clk_pll);
	if (err) {
		dev_err(data->dev, "Can't enable pll: %d\n", err);
		return err;
	}

	err = clk_prepare_enable(data->clk_pll_out);
	if (err) {
		dev_err(data->dev, "Can't enable pll_out: %d\n", err);
		return err;
	}

	err = clk_prepare_enable(data->clk_aud_mclk);
=======
	err = clk_prepare_enable(data->clk_cdev1);
>>>>>>> v5.8-rc3
	if (err) {
		dev_err(data->dev, "Can't enable aud_mclk: %d\n", err);
		return err;
	}

	data->set_baseclock = new_baseclock;
	data->set_mclk = mclk;

	return 0;
}
EXPORT_SYMBOL_GPL(tegra_asoc_utils_set_rate);

int tegra_asoc_utils_set_ac97_rate(struct tegra_asoc_utils_data *data)
{
	const int pll_rate = 73728000;
	const int ac97_rate = 24576000;
	int err;

<<<<<<< HEAD
	clk_disable_unprepare(data->clk_aud_mclk);
	clk_disable_unprepare(data->clk_pll_out);
	clk_disable_unprepare(data->clk_pll);
=======
	clk_disable_unprepare(data->clk_cdev1);
>>>>>>> v5.8-rc3

	/*
	 * AC97 rate is fixed at 24.576MHz and is used for both the host
	 * controller and the external codec
	 */
	err = clk_set_rate(data->clk_pll, pll_rate);
	if (err) {
		dev_err(data->dev, "Can't set pll_a rate: %d\n", err);
		return err;
	}

	err = clk_set_rate(data->clk_pll_out, ac97_rate);
	if (err) {
		dev_err(data->dev, "Can't set pll_a_out0 rate: %d\n", err);
		return err;
	}

	/* Don't set cdev1/extern1 rate; it's locked to pll_a_out0 */

<<<<<<< HEAD
	err = clk_prepare_enable(data->clk_pll);
	if (err) {
		dev_err(data->dev, "Can't enable pll_a: %d\n", err);
		return err;
	}

	err = clk_prepare_enable(data->clk_pll_out);
	if (err) {
		dev_err(data->dev, "Can't enable pll_a_out0: %d\n", err);
		return err;
	}

	err = clk_prepare_enable(data->clk_aud_mclk);
=======
	err = clk_prepare_enable(data->clk_cdev1);
>>>>>>> v5.8-rc3
	if (err) {
		dev_err(data->dev, "Can't enable cdev1: %d\n", err);
		return err;
	}

	data->set_baseclock = pll_rate;
	data->set_mclk = ac97_rate;

	return 0;
}
EXPORT_SYMBOL_GPL(tegra_asoc_utils_set_ac97_rate);

int tegra_asoc_utils_set_tegra210_rate(struct tegra_asoc_utils_data *data,
				       unsigned int sample_rate)
{
	unsigned int new_pll_base, pll_out, aud_mclk = 0;
	int err;

	switch (sample_rate) {
	case 11025:
	case 22050:
	case 44100:
	case 88200:
	case 176400:
		new_pll_base = data->pll_base_rate[ODD_RATE];
		pll_out = default_pll_out_rate[ODD_RATE];
		break;
	case 8000:
	case 16000:
	case 32000:
	case 48000:
	case 96000:
	case 192000:
		new_pll_base = data->pll_base_rate[EVEN_RATE];
		pll_out = default_pll_out_rate[EVEN_RATE];
		break;
	default:
		return -EINVAL;
	}

	/* reduce pll_out rate to support lower sampling rates */
	if (sample_rate <= 11025)
		pll_out = pll_out >> 1;
	if (data->mclk_fs)
		aud_mclk = sample_rate * data->mclk_fs;

	if (data->set_baseclock != new_pll_base) {
		err = clk_set_rate(data->clk_pll, new_pll_base);
		if (err) {
			dev_err(data->dev, "Can't set clk_pll rate: %d\n",
				err);
			return err;
		}
		data->set_baseclock = new_pll_base;
	}

	if (data->set_pll_out != pll_out) {
		err = clk_set_rate(data->clk_pll_out, pll_out);
		if (err) {
			dev_err(data->dev, "Can't set clk_pll_out rate: %d\n",
				err);
			return err;
		}
		data->set_pll_out = pll_out;
	}

	if (data->set_mclk != aud_mclk) {
		err = clk_set_rate(data->clk_aud_mclk, aud_mclk);
		if (err) {
			dev_err(data->dev, "Can't set clk_cdev1 rate: %d\n",
				err);
			return err;
		}
		data->set_mclk = aud_mclk;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(tegra_asoc_utils_set_tegra210_rate);

int tegra_asoc_utils_clk_enable(struct tegra_asoc_utils_data *data)
{
	int err;

	err = clk_prepare_enable(data->clk_aud_mclk);
	if (err) {
		dev_err(data->dev, "Can't enable clock aud_mclk\n");
		return err;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(tegra_asoc_utils_clk_enable);

void tegra_asoc_utils_clk_disable(struct tegra_asoc_utils_data *data)
{
	clk_disable_unprepare(data->clk_aud_mclk);
}
EXPORT_SYMBOL_GPL(tegra_asoc_utils_clk_disable);

int tegra_asoc_utils_init(struct tegra_asoc_utils_data *data,
			  struct device *dev)
{
	struct clk *clk_out_1, *clk_extern1;
	int ret;

	data->dev = dev;

	if (of_machine_is_compatible("nvidia,tegra20"))
		data->soc = TEGRA_ASOC_UTILS_SOC_TEGRA20;
	else if (of_machine_is_compatible("nvidia,tegra30"))
		data->soc = TEGRA_ASOC_UTILS_SOC_TEGRA30;
	else if (of_machine_is_compatible("nvidia,tegra114"))
		data->soc = TEGRA_ASOC_UTILS_SOC_TEGRA114;
	else if (of_machine_is_compatible("nvidia,tegra124"))
		data->soc = TEGRA_ASOC_UTILS_SOC_TEGRA124;
	else if (of_machine_is_compatible("nvidia,tegra210"))
		data->soc = TEGRA_ASOC_UTILS_SOC_TEGRA210;
	else if (of_machine_is_compatible("nvidia,tegra186"))
		data->soc = TEGRA_ASOC_UTILS_SOC_TEGRA186;
	else if (of_machine_is_compatible("nvidia,tegra194"))
		data->soc = TEGRA_ASOC_UTILS_SOC_TEGRA194;
	else {
		dev_err(data->dev, "SoC unknown to Tegra ASoC utils\n");
		return -EINVAL;
	}

<<<<<<< HEAD
	data->clk_pll = devm_clk_get(dev, "pll_a");
	if (IS_ERR(data->clk_pll)) {
		dev_err(data->dev, "Can't retrieve clk pll_a\n");
		return PTR_ERR(data->clk_pll);
	}

	data->clk_pll_out = devm_clk_get(dev, "pll_a_out0");
	if (IS_ERR(data->clk_pll_out)) {
		dev_err(data->dev, "Can't retrieve clk pll_a_out0\n");
		return PTR_ERR(data->clk_pll_out);
	}

	data->clk_aud_mclk = devm_clk_get(dev, "extern1");
	if (IS_ERR(data->clk_aud_mclk)) {
		dev_err(data->dev, "Can't retrieve clk aud_mclk\n");
		return PTR_ERR(data->clk_aud_mclk);
	}

	if (data->soc < TEGRA_ASOC_UTILS_SOC_TEGRA210) {
		ret = tegra_asoc_utils_set_rate(data, 44100, 256 * 44100);
		if (ret)
			return ret;
	}

	if (data->soc < TEGRA_ASOC_UTILS_SOC_TEGRA186)
		data->pll_base_rate = tegra210_pll_base_rate;
	else
		data->pll_base_rate = tegra186_pll_base_rate;
=======
	data->clk_pll_a = devm_clk_get(dev, "pll_a");
	if (IS_ERR(data->clk_pll_a)) {
		dev_err(data->dev, "Can't retrieve clk pll_a\n");
		return PTR_ERR(data->clk_pll_a);
	}

	data->clk_pll_a_out0 = devm_clk_get(dev, "pll_a_out0");
	if (IS_ERR(data->clk_pll_a_out0)) {
		dev_err(data->dev, "Can't retrieve clk pll_a_out0\n");
		return PTR_ERR(data->clk_pll_a_out0);
	}

	data->clk_cdev1 = devm_clk_get(dev, "mclk");
	if (IS_ERR(data->clk_cdev1)) {
		dev_err(data->dev, "Can't retrieve clk cdev1\n");
		return PTR_ERR(data->clk_cdev1);
	}

	/*
	 * If clock parents are not set in DT, configure here to use clk_out_1
	 * as mclk and extern1 as parent for Tegra30 and higher.
	 */
	if (!of_find_property(dev->of_node, "assigned-clock-parents", NULL) &&
	    data->soc > TEGRA_ASOC_UTILS_SOC_TEGRA20) {
		dev_warn(data->dev,
			 "Configuring clocks for a legacy device-tree\n");
		dev_warn(data->dev,
			 "Please update DT to use assigned-clock-parents\n");
		clk_extern1 = devm_clk_get(dev, "extern1");
		if (IS_ERR(clk_extern1)) {
			dev_err(data->dev, "Can't retrieve clk extern1\n");
			return PTR_ERR(clk_extern1);
		}

		ret = clk_set_parent(clk_extern1, data->clk_pll_a_out0);
		if (ret < 0) {
			dev_err(data->dev,
				"Set parent failed for clk extern1\n");
			return ret;
		}

		clk_out_1 = devm_clk_get(dev, "pmc_clk_out_1");
		if (IS_ERR(clk_out_1)) {
			dev_err(data->dev, "Can't retrieve pmc_clk_out_1\n");
			return PTR_ERR(clk_out_1);
		}

		ret = clk_set_parent(clk_out_1, clk_extern1);
		if (ret < 0) {
			dev_err(data->dev,
				"Set parent failed for pmc_clk_out_1\n");
			return ret;
		}

		data->clk_cdev1 = clk_out_1;
	}

	/*
	 * FIXME: There is some unknown dependency between audio mclk disable
	 * and suspend-resume functionality on Tegra30, although audio mclk is
	 * only needed for audio.
	 */
	ret = clk_prepare_enable(data->clk_cdev1);
	if (ret) {
		dev_err(data->dev, "Can't enable cdev1: %d\n", ret);
		return ret;
	}
>>>>>>> v5.8-rc3

	return 0;
}
EXPORT_SYMBOL_GPL(tegra_asoc_utils_init);

MODULE_AUTHOR("Stephen Warren <swarren@nvidia.com>");
MODULE_DESCRIPTION("Tegra ASoC utility code");
MODULE_LICENSE("GPL");
