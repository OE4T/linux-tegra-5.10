#
# Tegra SOC HWPM
#

GCOV_PROFILE := y

ccflags-y += -I$(srctree.nvidia)/drivers/platform/tegra/hwpm
ccflags-y += -I$(srctree.nvidia)/drivers/platform/tegra/hwpm/include
ccflags-y += -I$(srctree.nvidia)/include

#
# Control IP config
# TODO: Set IP flag and include IP source file as per build config
#
ccflags-y += -DCONFIG_SOC_HWPM_IP_VI=1
ccflags-y += -DCONFIG_SOC_HWPM_IP_ISP
ccflags-y += -DCONFIG_SOC_HWPM_IP_VIC
ccflags-y += -DCONFIG_SOC_HWPM_IP_OFA
ccflags-y += -DCONFIG_SOC_HWPM_IP_PVA
ccflags-y += -DCONFIG_SOC_HWPM_IP_NVDLA
ccflags-y += -DCONFIG_SOC_HWPM_IP_MGBE
ccflags-y += -DCONFIG_SOC_HWPM_IP_SCF
ccflags-y += -DCONFIG_SOC_HWPM_IP_NVDEC
ccflags-y += -DCONFIG_SOC_HWPM_IP_NVENC
ccflags-y += -DCONFIG_SOC_HWPM_IP_PCIE
ccflags-y += -DCONFIG_SOC_HWPM_IP_DISPLAY=1
ccflags-y += -DCONFIG_SOC_HWPM_IP_MSS_CHANNEL
ccflags-y += -DCONFIG_SOC_HWPM_IP_MSS_GPU_HUB
ccflags-y += -DCONFIG_SOC_HWPM_IP_MSS_ISO_NISO_HUBS
ccflags-y += -DCONFIG_SOC_HWPM_IP_MSS_MCF

obj-$(CONFIG_DEBUG_FS) += os/linux/tegra_hwpm_debugfs.o
obj-y += os/linux/tegra_hwpm_linux.o
obj-y += os/linux/tegra_hwpm_io.o
obj-y += os/linux/tegra_hwpm_ip.o
obj-y += os/linux/tegra_hwpm_ioctl.o
obj-y += os/linux/tegra_hwpm_log.o

obj-y += common/tegra_hwpm_alist_utils.o
obj-y += common/tegra_hwpm_mem_buf_utils.o
obj-y += common/tegra_hwpm_regops_utils.o
obj-y += common/tegra_hwpm_resource_utils.o
obj-y += common/tegra_hwpm_init.o

obj-y += hal/t234/t234_hwpm_alist_utils.o
obj-y += hal/t234/t234_hwpm_aperture_utils.o
obj-y += hal/t234/t234_hwpm_interface_utils.o
obj-y += hal/t234/t234_hwpm_ip_utils.o
obj-y += hal/t234/t234_hwpm_mem_buf_utils.o
obj-y += hal/t234/t234_hwpm_regops_utils.o
obj-y += hal/t234/t234_hwpm_regops_allowlist.o
obj-y += hal/t234/t234_hwpm_resource_utils.o

obj-y += hal/t234/ip/pma/t234_hwpm_ip_pma.o
obj-y += hal/t234/ip/rtr/t234_hwpm_ip_rtr.o

obj-y += hal/t234/ip/display/t234_hwpm_ip_display.o
obj-y += hal/t234/ip/isp/t234_hwpm_ip_isp.o
obj-y += hal/t234/ip/mgbe/t234_hwpm_ip_mgbe.o
obj-y += hal/t234/ip/mss_channel/t234_hwpm_ip_mss_channel.o
obj-y += hal/t234/ip/mss_gpu_hub/t234_hwpm_ip_mss_gpu_hub.o
obj-y += hal/t234/ip/mss_iso_niso_hubs/t234_hwpm_ip_mss_iso_niso_hubs.o
obj-y += hal/t234/ip/mss_mcf/t234_hwpm_ip_mss_mcf.o
obj-y += hal/t234/ip/nvdec/t234_hwpm_ip_nvdec.o
obj-y += hal/t234/ip/nvdla/t234_hwpm_ip_nvdla.o
obj-y += hal/t234/ip/nvenc/t234_hwpm_ip_nvenc.o
obj-y += hal/t234/ip/ofa/t234_hwpm_ip_ofa.o
obj-y += hal/t234/ip/pcie/t234_hwpm_ip_pcie.o
obj-y += hal/t234/ip/pva/t234_hwpm_ip_pva.o
obj-y += hal/t234/ip/scf/t234_hwpm_ip_scf.o
obj-y += hal/t234/ip/vi/t234_hwpm_ip_vi.o
obj-y += hal/t234/ip/vic/t234_hwpm_ip_vic.o
