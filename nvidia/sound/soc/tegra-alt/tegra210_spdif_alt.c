/*
 * tegra210_spdif_alt.c - Tegra210 SPDIF driver
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
#include <linux/version.h>
#if KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE
#include <soc/tegra/chip-id.h>
#else
#include <soc/tegra/fuse.h>
#endif
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinconf-tegra.h>

#include "tegra210_xbar_alt.h"
#include "tegra210_spdif_alt.h"

#define DRV_NAME "tegra210-spdif"

static const struct reg_default tegra210_spdif_reg_defaults[] = {
	{ TEGRA210_SPDIF_CIF_TXD_CTRL, 0x00001100},
	{ TEGRA210_SPDIF_CIF_RXD_CTRL, 0x00001100},
	{ TEGRA210_SPDIF_CIF_TXU_CTRL, 0x00001100},
	{ TEGRA210_SPDIF_CIF_RXU_CTRL, 0x00001100},
	{ TEGRA210_SPDIF_FLOWCTL_CTRL, 0x80000000},
	{ TEGRA210_SPDIF_TX_STEP, 0x00008000},
	{ TEGRA210_SPDIF_LCOEF_1_4_0, 0x0000002e},
	{ TEGRA210_SPDIF_LCOEF_1_4_1, 0x0000f9e6},
	{ TEGRA210_SPDIF_LCOEF_1_4_2, 0x000020ca},
	{ TEGRA210_SPDIF_LCOEF_1_4_3, 0x00007147},
	{ TEGRA210_SPDIF_LCOEF_1_4_4, 0x0000f17e},
	{ TEGRA210_SPDIF_LCOEF_1_4_5, 0x000001e0},
	{ TEGRA210_SPDIF_LCOEF_2_4_0, 0x00000117},
	{ TEGRA210_SPDIF_LCOEF_2_4_1, 0x0000f26b},
	{ TEGRA210_SPDIF_LCOEF_2_4_2, 0x00004c07},
};

static int tegra210_spdif_runtime_suspend(struct device *dev)
{
	struct tegra210_spdif *spdif = dev_get_drvdata(dev);
	int ret;

	regcache_cache_only(spdif->regmap, true);
	regcache_mark_dirty(spdif->regmap);
	if (!(tegra_platform_is_fpga())) {
		clk_disable_unprepare(spdif->clk_spdif_out);
		clk_disable_unprepare(spdif->clk_spdif_in);
	}

	return 0;
}

static int tegra210_spdif_runtime_resume(struct device *dev)
{
	struct tegra210_spdif *spdif = dev_get_drvdata(dev);
	int ret;

	if (!(tegra_platform_is_fpga())) {
		ret = clk_prepare_enable(spdif->clk_spdif_out);
		if (ret) {
			dev_err(dev, "spdif_out_clk_enable failed: %d\n", ret);
			return ret;
		}

		ret = clk_prepare_enable(spdif->clk_spdif_in);
		if (ret) {
			dev_err(dev, "spdif_in_clk_enable failed: %d\n", ret);
			return ret;
		}
	}

	regcache_cache_only(spdif->regmap, false);
	regcache_sync(spdif->regmap);

	return 0;
}

static int tegra210_spdif_set_dai_sysclk(struct snd_soc_dai *dai,
		int clk_id, unsigned int freq, int dir)
{
	struct device *dev = dai->dev;
	struct tegra210_spdif *spdif = snd_soc_dai_get_drvdata(dai);
	int spdif_out_clock_rate, spdif_in_clock_rate;
	int ret;

	switch (freq) {
	case 32000:
		spdif_out_clock_rate = 4096000;
		spdif_in_clock_rate = 48000000;
		break;
	case 44100:
		spdif_out_clock_rate = 5644800;
		spdif_in_clock_rate = 48000000;
		break;
	case 48000:
		spdif_out_clock_rate = 6144000;
		spdif_in_clock_rate = 48000000;
		break;
	case 88200:
		spdif_out_clock_rate = 11289600;
		spdif_in_clock_rate = 72000000;
		break;
	case 96000:
		spdif_out_clock_rate = 12288000;
		spdif_in_clock_rate = 72000000;
		break;
	case 176400:
		spdif_out_clock_rate = 22579200;
		spdif_in_clock_rate = 108000000;
		break;
	case 192000:
		spdif_out_clock_rate = 24576000;
		spdif_in_clock_rate = 108000000;
		break;
	default:
		return -EINVAL;
	}

	if (!(tegra_platform_is_fpga())) {
		if (dir == SND_SOC_CLOCK_OUT) {
			ret = clk_set_rate(spdif->clk_spdif_out,
					   spdif_out_clock_rate);
			if (ret) {
				dev_err(dev,
					"Can't set SPDIF Out clock rate: %d\n",
					ret);
				return ret;
			}
		} else {
			ret = clk_set_rate(spdif->clk_spdif_in,
					   spdif_in_clock_rate);
			if (ret) {
				dev_err(dev,
					"Can't set SPDIF In clock rate: %d\n",
					ret);
				return ret;
			}
		}
	}

	return 0;
}

static int tegra210_spdif_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params,
				 struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct tegra210_spdif *spdif = snd_soc_dai_get_drvdata(dai);
	int channels, audio_bits, bit_mode;
	struct tegra210_xbar_cif_conf cif_conf;

	memset(&cif_conf, 0, sizeof(struct tegra210_xbar_cif_conf));

	channels = params_channels(params);

	if (channels < 2) {
		dev_err(dev, "Doesn't support %d channels\n", channels);
		return -EINVAL;
	}

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		audio_bits = TEGRA210_AUDIOCIF_BITS_16;
		bit_mode = TEGRA210_SPDIF_BIT_MODE16;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		audio_bits = TEGRA210_AUDIOCIF_BITS_32;
		bit_mode = TEGRA210_SPDIF_BIT_MODERAW;
		break;
	default:
		return -EINVAL;
	}

	cif_conf.audio_channels = channels;
	cif_conf.client_channels = channels;
	cif_conf.audio_bits = audio_bits;
	cif_conf.client_bits = audio_bits;

	regmap_update_bits(spdif->regmap, TEGRA210_SPDIF_CTRL,
				TEGRA210_SPDIF_CTRL_BIT_MODE_MASK,
				bit_mode);

	/* As a CODEC DAI, CAPTURE is transmit */
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		tegra210_xbar_set_cif(spdif->regmap,
				      TEGRA210_SPDIF_CIF_TXD_CTRL,
				      &cif_conf);
	} else {
		tegra210_xbar_set_cif(spdif->regmap,
				      TEGRA210_SPDIF_CIF_RXD_CTRL,
				      &cif_conf);
	}

	return 0;
}

static struct snd_soc_dai_ops tegra210_spdif_dai_ops = {
	.hw_params	= tegra210_spdif_hw_params,
	.set_sysclk	= tegra210_spdif_set_dai_sysclk,
};

static struct snd_soc_dai_driver tegra210_spdif_dais[] = {
	{
		.name = "CIF",
		.playback = {
			.stream_name = "CIF Receive",
			.channels_min = 1,
			.channels_max = 16,
			.rates = SNDRV_PCM_RATE_8000_96000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
		.capture = {
			.stream_name = "CIF Transmit",
			.channels_min = 1,
			.channels_max = 16,
			.rates = SNDRV_PCM_RATE_8000_96000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
	},
	{
		.name = "DAP",
		.playback = {
			.stream_name = "DAP Receive",
			.channels_min = 1,
			.channels_max = 16,
			.rates = SNDRV_PCM_RATE_8000_96000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
		.capture = {
			.stream_name = "DAP Transmit",
			.channels_min = 1,
			.channels_max = 16,
			.rates = SNDRV_PCM_RATE_8000_96000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
		.ops = &tegra210_spdif_dai_ops,
	}
};

static int tegra210_spdif_loopback_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct tegra210_spdif *spdif = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.integer.value[0] = spdif->loopback;

	return 0;
}

static int tegra210_spdif_loopback_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct tegra210_spdif *spdif = snd_soc_codec_get_drvdata(codec);

	spdif->loopback = ucontrol->value.integer.value[0];

	pm_runtime_get_sync(codec->dev);
	regmap_update_bits(spdif->regmap, TEGRA210_SPDIF_CTRL,
		TEGRA210_SPDIF_CTRL_LBK_EN_ENABLE_MASK,
		spdif->loopback << TEGRA210_SPDIF_CTRL_LBK_EN_ENABLE_SHIFT);
	pm_runtime_put(codec->dev);

	return 0;
}

static const struct snd_kcontrol_new tegra210_spdif_controls[] = {
	SOC_SINGLE_EXT("Loopback", SND_SOC_NOPM, 0, 1, 0,
		tegra210_spdif_loopback_get, tegra210_spdif_loopback_put),
};

static const struct snd_soc_dapm_widget tegra210_spdif_widgets[] = {
	SND_SOC_DAPM_AIF_IN("CIF RX", NULL, 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("CIF TX", NULL, 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("DAP RX", NULL, 0, TEGRA210_SPDIF_CTRL, 29, 0),
	SND_SOC_DAPM_AIF_OUT("DAP TX", NULL, 0, TEGRA210_SPDIF_CTRL, 28, 0),
};

static const struct snd_soc_dapm_route tegra210_spdif_routes[] = {
	{ "CIF RX",       NULL, "CIF Receive"},
	{ "DAP TX",       NULL, "CIF RX"},
	{ "DAP Transmit", NULL, "DAP TX"},

	{ "DAP RX",       NULL, "DAP Receive"},
	{ "CIF TX",       NULL, "DAP RX"},
	{ "CIF Transmit", NULL, "CIF TX"},
};

static struct snd_soc_codec_driver tegra210_spdif_codec = {
	.idle_bias_off = 1,
	.component_driver = {
		.dapm_widgets = tegra210_spdif_widgets,
		.num_dapm_widgets = ARRAY_SIZE(tegra210_spdif_widgets),
		.dapm_routes = tegra210_spdif_routes,
		.num_dapm_routes = ARRAY_SIZE(tegra210_spdif_routes),
		.controls = tegra210_spdif_controls,
		.num_controls = ARRAY_SIZE(tegra210_spdif_controls),
	},
};

static bool tegra210_spdif_wr_rd_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case TEGRA210_SPDIF_CTRL:
	case TEGRA210_SPDIF_STROBE_CTRL:
	case TEGRA210_SPDIF_CIF_TXD_CTRL:
	case TEGRA210_SPDIF_CIF_RXD_CTRL:
	case TEGRA210_SPDIF_CIF_TXU_CTRL:
	case TEGRA210_SPDIF_CIF_RXU_CTRL:
	case TEGRA210_SPDIF_CH_STA_RX_A:
	case TEGRA210_SPDIF_CH_STA_RX_B:
	case TEGRA210_SPDIF_CH_STA_RX_C:
	case TEGRA210_SPDIF_CH_STA_RX_D:
	case TEGRA210_SPDIF_CH_STA_RX_E:
	case TEGRA210_SPDIF_CH_STA_RX_F:
	case TEGRA210_SPDIF_CH_STA_TX_A:
	case TEGRA210_SPDIF_CH_STA_TX_B:
	case TEGRA210_SPDIF_CH_STA_TX_C:
	case TEGRA210_SPDIF_CH_STA_TX_D:
	case TEGRA210_SPDIF_CH_STA_TX_E:
	case TEGRA210_SPDIF_CH_STA_TX_F:
	case TEGRA210_SPDIF_FLOWCTL_CTRL:
	case TEGRA210_SPDIF_TX_STEP:
	case TEGRA210_SPDIF_FLOW_STATUS:
	case TEGRA210_SPDIF_FLOW_TOTAL:
	case TEGRA210_SPDIF_FLOW_OVER:
	case TEGRA210_SPDIF_FLOW_UNDER:
	case TEGRA210_SPDIF_LCOEF_1_4_0:
	case TEGRA210_SPDIF_LCOEF_1_4_1:
	case TEGRA210_SPDIF_LCOEF_1_4_2:
	case TEGRA210_SPDIF_LCOEF_1_4_3:
	case TEGRA210_SPDIF_LCOEF_1_4_4:
	case TEGRA210_SPDIF_LCOEF_1_4_5:
	case TEGRA210_SPDIF_LCOEF_2_4_0:
	case TEGRA210_SPDIF_LCOEF_2_4_1:
	case TEGRA210_SPDIF_LCOEF_2_4_2:
		return true;
	default:
		return false;
	};
}

static const struct regmap_config tegra210_spdif_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = TEGRA210_SPDIF_LCOEF_2_4_2,
	.writeable_reg = tegra210_spdif_wr_rd_reg,
	.readable_reg = tegra210_spdif_wr_rd_reg,
	.reg_defaults = tegra210_spdif_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(tegra210_spdif_reg_defaults),
	.cache_type = REGCACHE_FLAT,
};

static const struct of_device_id tegra210_spdif_of_match[] = {
	{ .compatible = "nvidia,tegra210-spdif" },
	{},
};

static int tegra210_spdif_platform_probe(struct platform_device *pdev)
{
	struct tegra210_spdif *spdif;
	struct device_node *np = pdev->dev.of_node;
	struct resource *mem;
	void __iomem *regs;
	const struct of_device_id *match;
	const char *prod_name;
	int ret;

	match = of_match_device(tegra210_spdif_of_match, &pdev->dev);
	if (!match) {
		dev_err(&pdev->dev, "Error: No device match found\n");
		return -ENODEV;
	}

	spdif = devm_kzalloc(&pdev->dev, sizeof(*spdif), GFP_KERNEL);
	if (!spdif)
		return -ENOMEM;

	dev_set_drvdata(&pdev->dev, spdif);

	if (!(tegra_platform_is_fpga())) {
		spdif->clk_spdif_out = devm_clk_get(&pdev->dev, "spdif_out");
		if (IS_ERR(spdif->clk_spdif_out)) {
			dev_err(&pdev->dev, "Can't retrieve spdif clock\n");
			return PTR_ERR(spdif->clk_spdif_out);
		}

		spdif->clk_spdif_in = devm_clk_get(&pdev->dev, "spdif_in");
		if (IS_ERR(spdif->clk_spdif_in)) {
			dev_err(&pdev->dev, "Can't retrieve spdif clock\n");
			return PTR_ERR(spdif->clk_spdif_in);
		}
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	regs = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(regs))
		return PTR_ERR(regs);
	spdif->regmap = devm_regmap_init_mmio(&pdev->dev, regs,
					      &tegra210_spdif_regmap_config);
	if (IS_ERR(spdif->regmap)) {
		dev_err(&pdev->dev, "regmap init failed\n");
		return PTR_ERR(spdif->regmap);
	}
	regcache_cache_only(spdif->regmap, true);

	pm_runtime_enable(&pdev->dev);
	ret = snd_soc_register_codec(&pdev->dev, &tegra210_spdif_codec,
				     tegra210_spdif_dais,
				     ARRAY_SIZE(tegra210_spdif_dais));
	if (ret != 0) {
		dev_err(&pdev->dev, "Could not register CODEC: %d\n", ret);
		pm_runtime_disable(&pdev->dev);
		return ret;
	}

	if (of_property_read_string(np, "prod-name", &prod_name) == 0) {
		ret = tegra_pinctrl_config_prod(&pdev->dev, prod_name);
		if (ret < 0)
			dev_warn(&pdev->dev, "Failed to set %s setting\n",
				 prod_name);
	}

	return 0;
}

static int tegra210_spdif_platform_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);

	pm_runtime_disable(&pdev->dev);
	if (!pm_runtime_status_suspended(&pdev->dev))
		tegra210_spdif_runtime_suspend(&pdev->dev);

	return 0;
}

static const struct dev_pm_ops tegra210_spdif_pm_ops = {
	SET_RUNTIME_PM_OPS(tegra210_spdif_runtime_suspend,
			   tegra210_spdif_runtime_resume, NULL)
	SET_LATE_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				     pm_runtime_force_resume)
};

static struct platform_driver tegra210_spdif_driver = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = tegra210_spdif_of_match,
		.pm = &tegra210_spdif_pm_ops,
	},
	.probe = tegra210_spdif_platform_probe,
	.remove = tegra210_spdif_platform_remove,
};
module_platform_driver(tegra210_spdif_driver);

MODULE_AUTHOR("Arun Shamanna Lakshmi <aruns@nvidia.com>");
MODULE_AUTHOR("Songhee Baek <sbaek@nvidia.com>");
MODULE_DESCRIPTION("Tegra210 SPDIF ASoC driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" DRV_NAME);
MODULE_DEVICE_TABLE(of, tegra210_spdif_of_match);
