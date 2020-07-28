ccflags-y += -I$(srctree.nvidia)/drivers/video/tegra/host
ccflags-y += -I$(srctree.nvidia-t23x)/drivers/video/tegra/host

nvhost-$(CONFIG_TEGRA_T23X_GRHOST) += \
	t23x/t23x.o

nvhost-$(CONFIG_TEGRA_GRHOST_OFA) += ofa/ofa.o

obj-y += pva/
obj-y += nvdla/
