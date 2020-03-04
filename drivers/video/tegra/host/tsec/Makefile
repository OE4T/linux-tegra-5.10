GCOV_PROFILE := y
ccflags-y += -I$(srctree.nvidia)/drivers/video/tegra/host
ccflags-y += -I$(srctree.nvidia)/drivers/video/tegra/dc
ccflags-y += -Idrivers/video/tegra/host
ccflags-y += -Idrivers/video/tegra/dc
ccflags-y += -Werror

ccflags-$(CONFIG_TEGRA_T23X_GRHOST) += -I$(srctree.nvidia-t23x)/drivers/video/tegra/host

nvhost-tsec-objs  = \
	tsec.o

obj-$(CONFIG_TEGRA_GRHOST) += nvhost-tsec.o
