GCOV_PROFILE := y
ccflags-y += -I$(srctree.nvidia)/drivers/video/tegra/host/pva
ccflags-y += -I$(srctree.nvidia)/drivers/video/tegra/host
ccflags-y += -I$(srctree.nvidia)/drivers/video/tegra/host/pva/fw_include
ccflags-y += -I$(srctree.nvidia)/include
ccflags-y += -I$(srctree.nvidia)/include/linux
ccflags-y += -Werror

# When using the upstream host1x driver, the Makefile must define the
# srctree.host1x path in order to find the necessary header files for
# the upstream host1x driver.
ifdef CONFIG_TEGRA_HOST1X_NEXT
ccflags-y += -I$(srctree.host1x)/include
ccflags-y += -DCONFIG_TEGRA_HOST1X
endif

ifeq ($(CONFIG_TEGRA_FUSE_UPSTREAM),y)
ccflags-y += -DCONFIG_TEGRA_FUSE_UPSTREAM
endif

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
		nvpva_elf_parser.o \
		pva_vpu_exe.o \
		nvpva_client.o \
		nvpva_queue.o \
		pva_dma.o \
		nvpva_buffer.o \
		pva_vpu_ocd.o

obj-$(CONFIG_TEGRA_GRHOST_PVA) += nvhost-pva.o

ifdef CONFIG_TEGRA_T23X_GRHOST
nvhost-pva-t23x-objs  = \
		pva_isr_t23x.o \
		pva_mailbox_t23x.o \
		pva_interface_regs_t23x.o \
		pva_version_config_t23x.o \
		pva_ccq_t23x.o


obj-$(CONFIG_TEGRA_GRHOST_PVA) += nvhost-pva-t23x.o

endif
