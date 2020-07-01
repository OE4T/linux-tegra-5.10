ifdef CONFIG_ARCH_TEGRA_19x_SOC

ccflags-y += -Werror
ccflags-y += -I$(srctree)/drivers/iommu

obj-$(CONFIG_TEGRA_ARM_SMMU_T19x) += arm-smmu-t19x.o
obj-$(CONFIG_ARM_SMMU) += arm-smmu-regs.o
obj-$(CONFIG_ARM_SMMU) += of_tegra-smmu.o
obj-y += dma-override.o

endif

ifneq (,$(filter y,$(CONFIG_ARCH_TEGRA_19x_SOC)$(CONFIG_ARCH_TEGRA_23x_SOC)))
obj-$(CONFIG_ARM_SMMU_DEBUG) += arm-smmu-debug.o
endif

obj-$(CONFIG_ARM_SMMU_SUSPEND) += arm-smmu-suspend.o
ifeq ($(CONFIG_ARCH_TEGRA_21x_SOC),y)
obj-$(CONFIG_OF_IOMMU) += of_tegra-smmu.o
endif
