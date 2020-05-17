# Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.

obj-$(CONFIG_TEGRA_5G_LDPC) += ldpc/

obj-$(CONFIG_TEGRA_DCE) += dce/

obj-$(CONFIG_TEGRA_MCE) += mce/
obj-$(CONFIG_TEGRA_PSC) += psc/

ifneq ($(filter y,$(CONFIG_ARCH_TEGRA_18x_SOC) $(CONFIG_ARCH_TEGRA_19x_SOC)),)
obj-y += cache/
endif

