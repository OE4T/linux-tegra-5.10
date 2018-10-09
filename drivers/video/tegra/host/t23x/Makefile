ifdef CONFIG_TEGRA_T23X_GRHOST

GCOV_PROFILE := y
ccflags-y += -I$(srctree.nvidia)/drivers/video/tegra/host
ccflags-y += -Idrivers/video/tegra/camera
ccflags-y += -Idrivers/media/platform/tegra/camera
ccflags-y += -I$(srctree.nvidia)/include
ccflags-y += -Wno-multichar
ccflags-y += -Werror

nvhost-t23x-objs  = \
        t23x.o

obj-$(CONFIG_TEGRA_GRHOST) += nvhost-t23x.o

endif
