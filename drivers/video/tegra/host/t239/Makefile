ifeq ($(CONFIG_TEGRA_T23X_GRHOST),y)

GCOV_PROFILE := y
ccflags-y += -I$(srctree.nvidia)/drivers/video/tegra/host
ccflags-y += -I$(srctree.nvidia-t23x)/drivers/video/tegra/host
ccflags-y += -Idrivers/video/tegra/camera
ccflags-y += -Idrivers/media/platform/tegra/camera
ccflags-y += -I$(srctree.nvidia)/include
ccflags-y += -Wno-multichar
ccflags-y += -Werror

nvhost-t239-objs  = \
        t239.o

obj-$(CONFIG_TEGRA_GRHOST) += nvhost-t239.o

endif
