GCOV_PROFILE := y
ccflags-y += -I$(srctree.nvidia)/drivers/video/tegra/host
ccflags-y += -Idrivers/video/tegra/host
ccflags-y += -Werror

ccflags-$(CONFIG_TEGRA_T23X_GRHOST) += -I$(srctree.nvidia-t23x)/drivers/video/tegra/host

nvhost-vhost-objs  = \
	vhost.o \
	vhost_client.o

obj-$(CONFIG_TEGRA_GRHOST) += nvhost-vhost.o
