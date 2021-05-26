subdir-ccflags-y += -Werror

obj-$(CONFIG_CPU_IDLE_TEGRA18X)         += cpuidle-tegra18x.o

ifneq (,$(filter y,$(CONFIG_ARCH_TEGRA_19x_SOC) $(CONFIG_ARCH_TEGRA_194_SOC)))
obj-$(CONFIG_CPU_IDLE_TEGRA19X)         += cpuidle-tegra19x.o
endif
