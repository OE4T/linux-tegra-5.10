ifdef CONFIG_TEGRA_T19X_GRHOST

GCOV_PROFILE := y
ccflags-y += -I$(srctree.nvidia)/drivers/video/tegra/host
ccflags-y += -Idrivers/video/tegra/camera
ccflags-y += -Idrivers/media/platform/tegra/camera
ccflags-y += -I$(srctree.nvidia)/include
ccflags-y += -Wno-multichar
ccflags-y += -Werror

ccflags-$(CONFIG_TEGRA_T23X_GRHOST) += -I$(srctree.nvidia-t23x)/drivers/video/tegra/host

nvhost-t194-objs  = \
        t194.o

obj-$(CONFIG_TEGRA_GRHOST) += nvhost-t194.o

endif
