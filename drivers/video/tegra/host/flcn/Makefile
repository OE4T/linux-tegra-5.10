GCOV_PROFILE := y
ccflags-y += -I$(srctree.nvidia)/drivers/video/tegra/host
ccflags-y += -Idrivers/video/tegra/host

ccflags-$(CONFIG_TEGRA_T23X_GRHOST) += -I$(srctree.nvidia-t23x)/drivers/video/tegra/host

nvhost-flcn-objs  = \
	flcn.o \

obj-y += nvhost-flcn.o
