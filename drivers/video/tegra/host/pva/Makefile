BRINGUP := y

ifneq ($(BRINGUP), y)
V2 := $(CONFIG_TEGRA_GRHOST_PVA)
endif

obj-$(BRINGUP) = bringup/

GCOV_PROFILE := y
ccflags-$(V2) += -I$(srctree.nvidia-t23x)/drivers/video/tegra/host
ccflags-$(V2) += -I$(srctree.nvidia-t23x)/drivers/video/tegra/host/pva
ccflags-$(V2) += -I$(srctree.nvidia)/drivers/video/tegra/host
ccflags-$(V2) += -I$(srctree.nvidia)/include
ccflags-$(V2) += -Werror


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

obj-$(V2) += nvhost-pva.o
