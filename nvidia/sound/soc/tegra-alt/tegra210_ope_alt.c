/*
 * tegra210_ope_alt.c - Tegra210 OPE driver
 *
 * Copyright (c) 2014-2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <linux/of_device.h>

#include "tegra210_xbar_alt.h"
#include "tegra210_ope_alt.h"

#define DRV_NAME "tegra210-ope"

static const struct reg_default tegra210_ope_reg_defaults[] = {
	{ TEGRA210_OPE_AXBAR_RX_INT_MASK, 0x00000001},
	{ TEGRA210_OPE_AXBAR_RX_CIF_CTRL, 0x00007700},
	{ TEGRA210_OPE_AXBAR_TX_INT_MASK, 0x00000001},
	{ TEGRA210_OPE_AXBAR_TX_CIF_CTRL, 0x00007700},
	{ TEGRA210_OPE_CG, 0x1},
};

static int tegra210_ope_runtime_suspend(struct device *dev)
{
	struct tegra210_ope *ope = dev_get_drvdata(dev);

	tegra210_peq_save(ope);

	regcache_cache_only(ope->mbdrc_regmap, true);
	regcache_cache_only(ope->peq_regmap, true);
	regcache_cache_only(ope->regmap, true);
	regcache_mark_dirty(ope->regmap);
	regcache_mark_dirty(ope->peq_regmap);
	regcache_mark_dirty(ope->mbdrc_regmap);

	return 0;
}

static int tegra210_ope_runtime_resume(struct device *dev)
{
	struct tegra210_ope *ope = dev_get_drvdata(dev);

	regcache_cache_only(ope->regmap, false);
	regcache_cache_only(ope->peq_regmap, false);
	regcache_cache_only(ope->mbdrc_regmap, false);
	regcache_sync(ope->regmap);
	regcache_sync(ope->peq_regmap);
	regcache_sync(ope->mbdrc_regmap);
	tegra210_peq_restore(ope);

	return 0;
}

static int tegra210_ope_set_audio_cif(struct tegra210_ope *ope,
				struct snd_pcm_hw_params *params,
				unsigned int reg)
{
	int channels, audio_bits;
	struct tegra210_xbar_cif_conf cif_conf;

	memset(&cif_conf, 0, sizeof(struct tegra210_xbar_cif_conf));

	channels = params_channels(params);
	if (channels < 2)
		return -EINVAL;

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		audio_bits = TEGRA210_AUDIOCIF_BITS_16;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		audio_bits = TEGRA210_AUDIOCIF_BITS_32;
		break;
	default:
		return -EINVAL;
	}

	cif_conf.audio_channels = channels;
	cif_conf.client_channels = channels;
	cif_conf.audio_bits = audio_bits;
	cif_conf.client_bits = audio_bits;

	tegra210_xbar_set_cif(ope->regmap, reg, &cif_conf);

	return 0;
}

static int tegra210_ope_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params,
				 struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct tegra210_ope *ope = snd_soc_dai_get_drvdata(dai);
	int ret;

	/* set RX cif and TX cif */
	ret = tegra210_ope_set_audio_cif(ope, params,
				TEGRA210_OPE_AXBAR_RX_CIF_CTRL);
	if (ret) {
		dev_err(dev, "Can't set OPE RX CIF: %d\n", ret);
		return ret;
	}

	ret = tegra210_ope_set_audio_cif(ope, params,
				TEGRA210_OPE_AXBAR_TX_CIF_CTRL);
	if (ret) {
		dev_err(dev, "Can't set OPE TX CIF: %d\n", ret);
		return ret;
	}

	tegra210_mbdrc_hw_params(dai->codec);

	return ret;
}

static int tegra210_ope_codec_probe(struct snd_soc_codec *codec)
{
	tegra210_peq_codec_init(codec);
	tegra210_mbdrc_codec_init(codec);

	return 0;
}

static struct regmap *tegra210_ope_init_regmap(struct device *dev)
{
	struct tegra210_ope *ope = dev_get_drvdata(dev);

	return ope->regmap;
}

static struct snd_soc_dai_ops tegra210_ope_dai_ops = {
	.hw_params	= tegra210_ope_hw_params,
};

static struct snd_soc_dai_driver tegra210_ope_dais[] = {
	{
		.name = "OPE IN",
		.playback = {
			.stream_name = "OPE Receive",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S8 |
				SNDRV_PCM_FMTBIT_S16_LE |
				SNDRV_PCM_FMTBIT_S32_LE,
		},
	},
	{
		.name = "OPE OUT",
		.capture = {
			.stream_name = "OPE Transmit",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S8 |
				SNDRV_PCM_FMTBIT_S16_LE |
				SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &tegra210_ope_dai_ops,
	}
};

static const struct snd_soc_dapm_widget tegra210_ope_widgets[] = {
	SND_SOC_DAPM_AIF_IN("OPE RX", NULL, 0, SND_SOC_NOPM,
				0, 0),
	SND_SOC_DAPM_AIF_OUT("OPE TX", NULL, 0, TEGRA210_OPE_ENABLE,
				TEGRA210_OPE_EN_SHIFT, 0),
};

static const struct snd_soc_dapm_route tegra210_ope_routes[] = {
	{ "OPE RX",       NULL, "OPE Receive" },
	{ "OPE TX",       NULL, "OPE RX" },
	{ "OPE Transmit", NULL, "OPE TX" },
};

static const struct snd_kcontrol_new tegra210_ope_controls[] = {
	SOC_SINGLE("direction peq to mbdrc", TEGRA210_OPE_DIRECTION,
				TEGRA210_OPE_DIRECTION_SHIFT, 1, 0),
};

static struct snd_soc_codec_driver tegra210_ope_codec = {
	.probe = tegra210_ope_codec_probe,
	.idle_bias_off = 1,
	.get_regmap = tegra210_ope_init_regmap,
	.component_driver = {
		.dapm_widgets = tegra210_ope_widgets,
		.num_dapm_widgets = ARRAY_SIZE(tegra210_ope_widgets),
		.dapm_routes = tegra210_ope_routes,
		.num_dapm_routes = ARRAY_SIZE(tegra210_ope_routes),
		.controls = tegra210_ope_controls,
		.num_controls = ARRAY_SIZE(tegra210_ope_controls),
	},
};

static bool tegra210_ope_wr_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case TEGRA210_OPE_AXBAR_RX_INT_MASK:
	case TEGRA210_OPE_AXBAR_RX_INT_SET:
	case TEGRA210_OPE_AXBAR_RX_INT_CLEAR:
	case TEGRA210_OPE_AXBAR_RX_CIF_CTRL:

	case TEGRA210_OPE_AXBAR_TX_INT_MASK:
	case TEGRA210_OPE_AXBAR_TX_INT_SET:
	case TEGRA210_OPE_AXBAR_TX_INT_CLEAR:
	case TEGRA210_OPE_AXBAR_TX_CIF_CTRL:

	case TEGRA210_OPE_ENABLE:
	case TEGRA210_OPE_SOFT_RESET:
	case TEGRA210_OPE_CG:
	case TEGRA210_OPE_DIRECTION:
		return true;
	default:
		return false;
	};
}

static bool tegra210_ope_rd_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case TEGRA210_OPE_AXBAR_RX_STATUS:
	case TEGRA210_OPE_AXBAR_RX_INT_STATUS:
	case TEGRA210_OPE_AXBAR_RX_INT_MASK:
	case TEGRA210_OPE_AXBAR_RX_INT_SET:
	case TEGRA210_OPE_AXBAR_RX_INT_CLEAR:
	case TEGRA210_OPE_AXBAR_RX_CIF_CTRL:

	case TEGRA210_OPE_AXBAR_TX_STATUS:
	case TEGRA210_OPE_AXBAR_TX_INT_STATUS:
	case TEGRA210_OPE_AXBAR_TX_INT_MASK:
	case TEGRA210_OPE_AXBAR_TX_INT_SET:
	case TEGRA210_OPE_AXBAR_TX_INT_CLEAR:
	case TEGRA210_OPE_AXBAR_TX_CIF_CTRL:

	case TEGRA210_OPE_ENABLE:
	case TEGRA210_OPE_SOFT_RESET:
	case TEGRA210_OPE_CG:
	case TEGRA210_OPE_STATUS:
	case TEGRA210_OPE_INT_STATUS:
	case TEGRA210_OPE_DIRECTION:
		return true;
	default:
		return false;
	};
}

static bool tegra210_ope_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case TEGRA210_OPE_AXBAR_RX_STATUS:
	case TEGRA210_OPE_AXBAR_RX_INT_SET:
	case TEGRA210_OPE_AXBAR_RX_INT_STATUS:

	case TEGRA210_OPE_AXBAR_TX_STATUS:
	case TEGRA210_OPE_AXBAR_TX_INT_SET:
	case TEGRA210_OPE_AXBAR_TX_INT_STATUS:

	case TEGRA210_OPE_SOFT_RESET:
	case TEGRA210_OPE_STATUS:
	case TEGRA210_OPE_INT_STATUS:
		return true;
	default:
		return false;
	};
}

static const struct regmap_config tegra210_ope_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = TEGRA210_OPE_DIRECTION,
	.writeable_reg = tegra210_ope_wr_reg,
	.readable_reg = tegra210_ope_rd_reg,
	.volatile_reg = tegra210_ope_volatile_reg,
	.reg_defaults = tegra210_ope_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(tegra210_ope_reg_defaults),
	.cache_type = REGCACHE_FLAT,
};

static const struct of_device_id tegra210_ope_of_match[] = {
	{ .compatible = "nvidia,tegra210-ope" },
	{},
};

static int tegra210_ope_platform_probe(struct platform_device *pdev)
{
	struct tegra210_ope *ope;
	struct resource *mem;
	void __iomem *regs;
	int ret = 0;
	const struct of_device_id *match;

	pr_info("OPE platform probe\n");

	match = of_match_device(tegra210_ope_of_match, &pdev->dev);
	if (!match) {
		dev_err(&pdev->dev, "Error: No device match found\n");
		return -ENODEV;
	}

	ope = devm_kzalloc(&pdev->dev, sizeof(*ope), GFP_KERNEL);
	if (!ope)
		return -ENOMEM;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	regs = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(regs))
		return PTR_ERR(regs);
	ope->regmap = devm_regmap_init_mmio(&pdev->dev, regs,
					    &tegra210_ope_regmap_config);
	if (IS_ERR(ope->regmap)) {
		dev_err(&pdev->dev, "regmap init failed\n");
		return PTR_ERR(ope->regmap);
	}
	regcache_cache_only(ope->regmap, true);

	dev_set_drvdata(&pdev->dev, ope);

	ret = tegra210_peq_init(pdev, TEGRA210_PEQ_IORESOURCE_MEM);
	if (ret < 0) {
		dev_err(&pdev->dev, "peq init failed\n");
		return ret;
	}
	regcache_cache_only(ope->peq_regmap, true);

	ret = tegra210_mbdrc_init(pdev, TEGRA210_MBDRC_IORESOURCE_MEM);
	if (ret < 0) {
		dev_err(&pdev->dev, "mbdrc init failed\n");
		return ret;
	}
	regcache_cache_only(ope->mbdrc_regmap, true);

	pm_runtime_enable(&pdev->dev);
	ret = snd_soc_register_codec(&pdev->dev, &tegra210_ope_codec,
				     tegra210_ope_dais,
				     ARRAY_SIZE(tegra210_ope_dais));
	if (ret != 0) {
		dev_err(&pdev->dev, "Could not register CODEC: %d\n", ret);
		pm_runtime_disable(&pdev->dev);
		return ret;
	}

	pr_info("OPE platform probe successful\n");

	return 0;
}

static int tegra210_ope_platform_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);

	pm_runtime_disable(&pdev->dev);
	if (!pm_runtime_status_suspended(&pdev->dev))
		tegra210_ope_runtime_suspend(&pdev->dev);

	return 0;
}

static const struct dev_pm_ops tegra210_ope_pm_ops = {
	SET_RUNTIME_PM_OPS(tegra210_ope_runtime_suspend,
			   tegra210_ope_runtime_resume, NULL)
	SET_LATE_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				     pm_runtime_force_resume)
};

static struct platform_driver tegra210_ope_driver = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = tegra210_ope_of_match,
		.pm = &tegra210_ope_pm_ops,
	},
	.probe = tegra210_ope_platform_probe,
	.remove = tegra210_ope_platform_remove,
};
module_platform_driver(tegra210_ope_driver)

MODULE_AUTHOR("Sumit Bhattacharya <sumitb@nvidia.com>");
MODULE_DESCRIPTION("Tegra210 OPE ASoC driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME);
MODULE_DEVICE_TABLE(of, tegra210_ope_of_match);
