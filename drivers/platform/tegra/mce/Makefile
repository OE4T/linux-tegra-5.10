#
# MCE Driver code.
#

GCOV_PROFILE := y

ccflags-y += -I$(srctree)/drivers/platform/tegra/include

ccflags-y += -Werror

obj-$(CONFIG_TEGRA_MCE) += tegra-mce.o

obj-$(CONFIG_ARCH_TEGRA_18x_SOC) += tegra18x-mce.o
obj-$(CONFIG_ARCH_TEGRA_19x_SOC) += tegra19x-mce.o

ifeq ($(CONFIG_ARCH_TEGRA_23x_SOC),y)
obj-y += tegra19x-mce.o
endif
