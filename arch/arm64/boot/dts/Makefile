# SPDX-License-Identifier: GPL-2.0
subdir-y += actions
subdir-y += al
subdir-y += allwinner
subdir-y += altera
subdir-y += amd
subdir-y += amlogic
subdir-y += apm
subdir-y += arm
subdir-y += bitmain
subdir-y += broadcom
subdir-y += cavium
subdir-y += exynos
subdir-y += freescale
subdir-y += hisilicon
subdir-y += intel
subdir-y += lg
subdir-y += marvell
subdir-y += mediatek
subdir-y += nvidia
subdir-y += qcom
subdir-y += realtek
subdir-y += renesas
subdir-y += rockchip
subdir-y += socionext
subdir-y += sprd
subdir-y += synaptics
subdir-y += ti
subdir-y += xilinx
subdir-y += zte

# NVIDIA downstream .dtb build
tegra-dtstree := $(srctree)/../../hardware/nvidia

DTC_INCLUDE += $(srctree)/arch/arm64/boot/dts/nvidia
DTC_INCLUDE += $(tegra-dtstree)/soc/tegra/kernel-include
DTC_INCLUDE += $(tegra-dtstree)/platform/tegra/common/kernel-dts

DTC_INCLUDE += $(tegra-dtstree)/soc/t210/kernel-dts
DTC_INCLUDE += $(tegra-dtstree)/platform/t210/common/kernel-dts
DTC_INCLUDE += $(tegra-dtstree)/platform/t210b01/common/kernel-dts

DTC_INCLUDE += $(tegra-dtstree)/soc/t18x/kernel-include
DTC_INCLUDE += $(tegra-dtstree)/soc/t18x/kernel-dts
DTC_INCLUDE += $(tegra-dtstree)/platform/t18x/common/kernel-dts

DTC_INCLUDE += $(tegra-dtstree)/soc/t19x/kernel-include
DTC_INCLUDE += $(tegra-dtstree)/soc/t19x/kernel-dts
DTC_INCLUDE += $(tegra-dtstree)/platform/t19x/common/kernel-dts

ifeq ($(CONFIG_ARCH_TEGRA_23x_SOC),y)
DTC_INCLUDE += $(tegra-dtstree)/soc/t23x/kernel-include
DTC_INCLUDE += $(tegra-dtstree)/soc/t23x/kernel-dts
DTC_INCLUDE += $(tegra-dtstree)/platform/t23x/common/kernel-dts
endif

DTC_CPP_FLAGS += -DLINUX_VERSION=500

ifeq ($(NV_BUILD_SYSTEM_TYPE),embedded-linux)
dtb-$(CONFIG_ARCH_TEGRA_194_SOC) += nvidia/tegra194-e3550-0001-b01-A.dtb
dtb-$(CONFIG_ARCH_TEGRA_194_SOC) += nvidia/tegra194-e3550-0001-b01-B.dtb
dtb-$(CONFIG_ARCH_TEGRA_194_SOC) += nvidia/tegra194-e3550-0001-b03-A.dtb
dtb-$(CONFIG_ARCH_TEGRA_194_SOC) += nvidia/tegra194-e3550-0001-b03-B.dtb
dtb-$(CONFIG_ARCH_TEGRA_186_SOC) += nvidia/tegra186-vcm31-p2382-010-a01-00-base.dtb
else ifeq ($(NV_BUILD_SYSTEM_TYPE),l4t)
dtb-$(CONFIG_ARCH_TEGRA_23x_SOC) += nvidia/tegra234-sim-vdk.dtb
dtb-$(CONFIG_ARCH_TEGRA_194_SOC) += nvidia/tegra194-p2888-0001-p2822-0000.dtb
dtb-$(CONFIG_ARCH_TEGRA_186_SOC) += nvidia/tegra186-quill-p3310-1000-c03-00-base.dtb
dtb-$(CONFIG_ARCH_TEGRA_186_SOC) += nvidia/tegra186-quill-p3310-1000-a00-00-base.dtb
dtb-$(CONFIG_ARCH_TEGRA_210_SOC) += nvidia/tegra210-jetson-tx1-p2597-2180-a01-devkit.dtb
dtb-$(CONFIG_ARCH_TEGRA_210_SOC) += nvidia/tegra210-jetson-cv-base-p2597-2180-a00.dtb
else
dtb-$(CONFIG_ARCH_TEGRA_23x_SOC) += nvidia/tegra234-sim-vdk.dtb
dtb-$(CONFIG_ARCH_TEGRA_194_SOC) += nvidia/tegra194-p2888-0001-p2822-0000.dtb
dtb-$(CONFIG_ARCH_TEGRA_194_SOC) += nvidia/tegra194-e3550-0001-b01-A.dtb
dtb-$(CONFIG_ARCH_TEGRA_194_SOC) += nvidia/tegra194-e3550-0001-b01-B.dtb
dtb-$(CONFIG_ARCH_TEGRA_194_SOC) += nvidia/tegra194-e3550-0001-b03-A.dtb
dtb-$(CONFIG_ARCH_TEGRA_194_SOC) += nvidia/tegra194-e3550-0001-b03-B.dtb
dtb-$(CONFIG_ARCH_TEGRA_186_SOC) += nvidia/tegra186-vcm31-p2382-010-a01-00-base.dtb
dtb-$(CONFIG_ARCH_TEGRA_186_SOC) += nvidia/tegra186-quill-p3310-1000-c03-00-base.dtb
dtb-$(CONFIG_ARCH_TEGRA_186_SOC) += nvidia/tegra186-quill-p3310-1000-a00-00-base.dtb
dtb-$(CONFIG_ARCH_TEGRA_210_SOC) += nvidia/tegra210-jetson-tx1-p2597-2180-a01-devkit.dtb
dtb-$(CONFIG_ARCH_TEGRA_210_SOC) += nvidia/tegra210-jetson-cv-base-p2597-2180-a00.dtb
endif
