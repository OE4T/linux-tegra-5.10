GCOV_PROFILE := y
ccflags-y += -I$(srctree.nvidia)/drivers/video/tegra/host
ccflags-y += -Idrivers/video/tegra/host
ccflags-y += -Werror

nvhost-vhost-objs  = \
	vhost.o \
	vhost_client.o

obj-$(CONFIG_TEGRA_GRHOST) += nvhost-vhost.o
