ifdef CONFIG_TEGRA_T23X_GRHOST

GCOV_PROFILE := y
ccflags-y += -I$(srctree.nvidia-t23x)/drivers/video/tegra/host
ccflags-y += -I$(srctree.nvidia)/drivers/video/tegra/host
ccflags-y += -I$(srctree.nvidia)/drivers/video/tegra/host/pva
ccflags-y += -I$(srctree.nvidia)/include
ccflags-y += -Werror

nvhost-pva-t23x-objs  = \
		pva_isr_t23x.o

obj-$(CONFIG_TEGRA_GRHOST_PVA) += nvhost-pva-t23x.o

endif
