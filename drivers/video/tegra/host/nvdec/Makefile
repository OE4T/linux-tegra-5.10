GCOV_PROFILE := y
ccflags-y += -I$(srctree.nvidia)/drivers/video/tegra/host
ccflags-y += -Idrivers/video/tegra/host
ccflags-y += -Iarch/arm/mach-tegra

ccflags-$(CONFIG_TEGRA_T23X_GRHOST) += -I$(srctree.nvidia-t23x)/drivers/video/tegra/host

nvhost-nvdec-objs  = \
	nvdec.o

obj-$(CONFIG_TEGRA_GRHOST) += nvhost-nvdec.o
