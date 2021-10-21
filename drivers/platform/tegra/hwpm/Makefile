#
# Tegra SOC HWPM
#

GCOV_PROFILE := y

ccflags-y += -I$(srctree.nvidia)/drivers/platform/tegra/hwpm/include/regops/t234
ccflags-y += -I$(srctree.nvidia)/drivers/platform/tegra/hwpm/include/hw/t234
ccflags-y += -I$(srctree.nvidia)/drivers/platform/tegra/hwpm

obj-y += tegra-soc-hwpm.o
obj-y += tegra-soc-hwpm-io.o
obj-y += tegra-soc-hwpm-ioctl.o
obj-y += tegra-soc-hwpm-log.o
obj-y += tegra-soc-hwpm-ip.o
obj-$(CONFIG_DEBUG_FS) += tegra-soc-hwpm-debugfs.o
