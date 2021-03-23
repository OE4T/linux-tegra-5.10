GCOV_PROFILE := y

ccflags-y += -I$(srctree.nvidia)/drivers/video/tegra/host
ccflags-y += -I$(srctree.nvidia)/drivers/media/platform/tegra
ccflags-y += -I$(srctree.nvidia)/include
ccflags-y += -Werror

nvhost-capture-objs = capture-support.o

obj-$(CONFIG_TEGRA_GRHOST) += nvhost-capture.o

ifdef CONFIG_TEGRA_T23X_GRHOST

ccflags-y += -I$(srctree.nvidia-t23x)/drivers/video/tegra/host

endif
