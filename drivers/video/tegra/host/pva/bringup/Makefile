GCOV_PROFILE := y
ccflags-y += -I$(srctree.nvidia-t23x)/drivers/video/tegra/host
ccflags-y += -I$(srctree.nvidia-t23x)/drivers/video/tegra/host/pva/bringup
ccflags-y += -I$(srctree.nvidia)/drivers/video/tegra/host
ccflags-y += -I$(srctree.nvidia)/include
ccflags-y += -Werror


nvhost-pva-objs  = \
		pva.o \
		pva_ioctl.o \
		pva_mailbox.o \
		pva_interface_regs_t19x.o \
		pva_version_config_t19x.o \
		pva_mailbox_t19x.o \
		pva_isr.o \
		pva_queue.o \
		pva_debug.o \
		pva_trace.o \
		pva_abort.o \
		pva_ccq_t19x.o \
		pva_selftest.o

obj-$(CONFIG_TEGRA_GRHOST_PVA) += nvhost-pva.o
