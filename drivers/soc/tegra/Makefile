subdir-ccflags-y += -Werror

obj-y += tegra-bpmp-dvfs.o
ifeq ($(CONFIG_NV_TEGRA_BPMP),y)
obj-$(CONFIG_DEBUG_FS) += tegra-bpmp-ec.o
endif
obj-$(CONFIG_ARCH_TEGRA_210_SOC)	+= pm-tegra210.o
obj-$(CONFIG_TEGRA_PROC_POWER_MODEL)	+= tegra-ppm.o
