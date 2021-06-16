/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This file contains list of SMMU Stream IDs used in Tegra234.
 * There is duplicate copy of this file present in
 * hardware/nvidia/soc/t23x/kernel-include/dt-bindings/memory/tegra234-smmu-streamid.h
 * make sure to update both to keep it in sync.
 */


#ifndef _DT_BINDINGS_MEMORY_TEGRA234_SMMU_STREAMID_H
#define _DT_BINDINGS_MEMORY_TEGRA234_SMMU_STREAMID_H

#define TEGRA_SID_INVALID	0x0
#define TEGRA_SID_PASSTHROUGH	0x7f

#define TEGRA_SID_AON		0x1
#define TEGRA_SID_APE		0x2
#define TEGRA_SID_GPCDMA_0	0x3
#define TEGRA_SID_BPMP		0x4
#define TEGRA_SID_DCE		0x5
#define TEGRA_SID_NVDLA0	0x6
#define TEGRA_SID_NVDLA1	0x7
#define TEGRA_SID_EQOS		0x8
#define TEGRA_SID_ETR		0x9
#define TEGRA_SID_HDA		0xA
#define TEGRA_SID_HC		0xB
#define TEGRA_SID_ISP		0xC
#define TEGRA_SID_MGBE		0xD
#define TEGRA_SID_NVDEC		0xE
#define TEGRA_SID_NVDISPLAY_NISO	0xF
#define TEGRA_SID_NVDISPLAY	0x10
#define TEGRA_SID_NVENC		0x11
#define TEGRA_SID_NVJPG1	0x12
#define TEGRA_SID_NVJPG		0x13
#define TEGRA_SID_OFA		0x14
#define TEGRA_SID_PCIE0		0x15
#define TEGRA_SID_PCIE1		0x16
#define TEGRA_SID_PCIE2		0x17
#define TEGRA_SID_PCIE3		0x18
#define TEGRA_SID_PCIE4		0x19
#define TEGRA_SID_PCIE5		0x1A
#define TEGRA_SID_PCIE6		0x1B
#define TEGRA_SID_PCIE7		0x1C
#define TEGRA_SID_PCIE8		0x1D
#define TEGRA_SID_PCIE9		0x1E
#define TEGRA_SID_PCIE10	0x1F
#define TEGRA_SID_PSC		0x20
#define TEGRA_SID_PVA0		0x21
#define TEGRA_SID_PMA		0x22
#define TEGRA_SID_RCE		0x23
#define TEGRA_SID_SCE		0x24
#define TEGRA_SID_SDMMC1A	0x25
#define TEGRA_SID_SDMMC4A	0x26
#define TEGRA_SID_SE		0x27
#define TEGRA_SID_SE1		0x28
#define TEGRA_SID_SE2		0x29
#define TEGRA_SID_SEU1		0x2A
#define TEGRA_SID_TSEC		0x2B
#define TEGRA_SID_UFSHC		0x2D
#define TEGRA_SID_VIC		0x2E
#define TEGRA_SID_VI		0x2F
#define TEGRA_SID_VIFALC	0x30
#define TEGRA_SID_VI2		0x31
#define TEGRA_SID_VI2FALC	0x32
#define TEGRA_SID_XSPI0		0x33
#define TEGRA_SID_XSPI1		0x34
#define TEGRA_SID_XUSB_HOST	0x35
#define TEGRA_SID_XUSB_DEV	0x36
#define TEGRA_SID_FSI		0x37

#define TEGRA_SID_SMMU_TEST	0x39

/* PVA virtualization clients. */
#define TEGRA_SID_PVA_VM0	0x3A
#define TEGRA_SID_PVA_VM1	0x3B
#define TEGRA_SID_PVA_VM2	0x3C
#define TEGRA_SID_PVA_VM3	0x3D
#define TEGRA_SID_PVA_VM4	0x3E
#define TEGRA_SID_PVA_VM5	0x3F
#define TEGRA_SID_PVA_VM6	0x40
#define TEGRA_SID_PVA_VM7	0x41

/* Host1x virtualization clients. */
#define TEGRA_SID_HOST1X_CTX0	0x51
#define TEGRA_SID_HOST1X_CTX1	0x52
#define TEGRA_SID_HOST1X_CTX2	0x53
#define TEGRA_SID_HOST1X_CTX3	0x54
#define TEGRA_SID_HOST1X_CTX4	0x55
#define TEGRA_SID_HOST1X_CTX5	0x56
#define TEGRA_SID_HOST1X_CTX6	0x57
#define TEGRA_SID_HOST1X_CTX7	0x58

/* Host1x command buffers */
#define TEGRA_SID_HC_VM0	0x59
#define TEGRA_SID_HC_VM1	0x5A
#define TEGRA_SID_HC_VM2	0x5B
#define TEGRA_SID_HC_VM3	0x5C
#define TEGRA_SID_HC_VM4	0x5D
#define TEGRA_SID_HC_VM5	0x5E
#define TEGRA_SID_HC_VM6	0x5F
#define TEGRA_SID_HC_VM7	0x60

#define TEGRA_SID_XUSB_VF0	0x61
#define TEGRA_SID_XUSB_VF1	0x62
#define TEGRA_SID_XUSB_VF2	0x63
#define TEGRA_SID_XUSB_VF3	0x64

/* EQOS virtual functions */
#define TEGRA_SID_EQOS_VF1	0x65
#define TEGRA_SID_EQOS_VF2	0x66
#define TEGRA_SID_EQOS_VF3	0x67
#define TEGRA_SID_EQOS_VF4	0x68

/* SE data buffers */
#define TEGRA_SID_SE_VM0	0x69U
#define TEGRA_SID_SE_VM1	0x6AU
#define TEGRA_SID_SE_VM2	0x6BU

/* The GPC DMA clients. */
#define TEGRA_SID_GPCDMA_1	0x6CU
#define TEGRA_SID_GPCDMA_2	0x6DU
#define TEGRA_SID_GPCDMA_3	0x6EU
#define TEGRA_SID_GPCDMA_4	0x6FU

#define TEGRA_SID_RCE_VM2	0x70U

#define TEGRA_SID_VI_VM2	0x71U

#endif /* _DT_BINDINGS_MEMORY_TEGRA234_SMMU_STREAMID_H */
