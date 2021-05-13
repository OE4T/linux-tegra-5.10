GCOV_PROFILE := y


subdir-ccflags-y := -Werror

ifeq ($(findstring stable,$(NV_BUILD_KERNEL_OPTIONS)),)
ccflags-y += -DNVMAP_PROCRANK
endif

ccflags-y += -I$(srctree.nvidia)/include/

nvmap-y := nvmap_core.o \
	nvmap_alloc.o \
	nvmap_cache.o \
	nvmap_dev.o \
	nvmap_dmabuf.o \
	nvmap_fault.o \
	nvmap_handle.o \
	nvmap_heap.o \
	nvmap_ioctl.o \
	nvmap_init.o \
	nvmap_tag.o \
	nvmap_mm.o \
	nvmap_stats.o \
	nvmap_carveout.o \
	nvmap_kasan_wrapper.o
nvmap-$(CONFIG_NVMAP_SCIIPC) += nvmap_sci_ipc.o

ifeq ($(CONFIG_NVMAP_PAGE_POOLS), y)
nvmap-y += nvmap_pp.o
endif

ifneq ($(filter y,$(CONFIG_ARCH_TEGRA_19x_SOC) $(CONFIG_ARCH_TEGRA_194_SOC)),)
nvmap-y += nvmap_init_t19x.o
nvmap-y += nvmap_dmabuf_t19x.o
nvmap-y += nvmap_cache_t19x.o
endif

KASAN_SANITIZE_nvmap_kasan_wrapper.o := n

ifneq ($(findstring stable,$(NV_BUILD_KERNEL_OPTIONS)),)
nvmap-y += nvmap_cache_maint.o
nvmap-y += nvmap_pp.o
ccflags-y += -DNVMAP_LOADABLE_MODULE
ccflags-y += -DCONFIG_NVMAP_PAGE_POOLS
obj-m +=nvmap.o
else
obj-y +=nvmap.o
endif
