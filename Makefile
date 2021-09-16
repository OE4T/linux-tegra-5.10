old-dtb := $(dtb-y)
old-dtbo := $(dtbo-y)
dtb-y :=
dtbo-y :=
makefile-path := platform/t23x/concord/kernel-dts

BUILD_ENABLE=n
ifneq ($(filter y,$(CONFIG_ARCH_TEGRA_23x_SOC)),)
BUILD_ENABLE=y
endif

dtb-$(BUILD_ENABLE) += tegra234-p3701-0000-p3737-0000.dtb
dtb-$(BUILD_ENABLE) += tegra234-p3701-0000-as-p3770-p3737-0000.dtb
dtb-$(BUILD_ENABLE) += tegra234-p3701-0000-as-p3769-p3737-0000.dtb
dtb-$(BUILD_ENABLE) += tegra234-p3701-0000-as-p3767-p3737-0000.dtb
dtbo-$(BUILD_ENABLE) += tegra234-p3737-overlay-pcie.dtbo
dtbo-$(BUILD_ENABLE) += tegra234-p3737-audio-codec-rt5658.dtbo
dtbo-$(BUILD_ENABLE) += tegra234-p3737-audio-codec-rt5658-40pin.dtbo

ifneq ($(dtb-y),)
dtb-y := $(addprefix $(makefile-path)/,$(dtb-y))
endif
ifneq ($(dtbo-y),)
dtbo-y := $(addprefix $(makefile-path)/,$(dtbo-y))
endif

dtb-y += $(old-dtb)
dtbo-y += $(old-dtbo)
