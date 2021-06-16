ifdef CONFIG_ARCH_TEGRA_19x_SOC

ccflags-y += -I$(srctree)/drivers/pci/dwc
ifeq ($(CONFIG_TEGRA_T23X_GRHOST),y)
ccflags-y += -I$(srctree.nvidia)/include
endif

obj-$(CONFIG_PCIE_TEGRA) += pcie-tegra.o

endif
