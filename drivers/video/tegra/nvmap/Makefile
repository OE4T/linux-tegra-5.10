GCOV_PROFILE := y


subdir-ccflags-y := -Werror

ifeq ($(findstring stable,$(NV_BUILD_KERNEL_OPTIONS)),)
ccflags-y += -DNVMAP_PROCRANK
endif

ccflags-y += -I$(srctree.nvidia)/include/

obj-y += nvmap.o
obj-y += nvmap_alloc.o
obj-y += nvmap_cache.o
obj-y += nvmap_dev.o
obj-y += nvmap_dmabuf.o
obj-y += nvmap_fault.o
obj-y += nvmap_handle.o
obj-y += nvmap_heap.o
obj-y += nvmap_ioctl.o
obj-y += nvmap_init.o
obj-y += nvmap_tag.o
obj-y += nvmap_mm.o
obj-y += nvmap_stats.o
obj-y += nvmap_carveout.o
obj-y += nvmap_kasan_wrapper.o
obj-$(CONFIG_NVMAP_SCIIPC) += nvmap_sci_ipc.o

obj-$(CONFIG_NVMAP_PAGE_POOLS) += nvmap_pp.o

ifneq ($(filter y,$(CONFIG_ARCH_TEGRA_19x_SOC) $(CONFIG_ARCH_TEGRA_194_SOC)),)
obj-y += nvmap_init_t19x.o
obj-y += nvmap_dmabuf_t19x.o
obj-y += nvmap_cache_t19x.o
endif

KASAN_SANITIZE_nvmap_kasan_wrapper.o := n
