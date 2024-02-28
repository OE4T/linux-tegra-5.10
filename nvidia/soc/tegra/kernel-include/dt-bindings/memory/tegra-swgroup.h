/*
 * Copyright (c) 2014-2020, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef _DT_BINDINGS_MEMORY_TEGRA_SWGROUP_H
#define _DT_BINDINGS_MEMORY_TEGRA_SWGROUP_H

#define TEGRA_SWGROUP_INVALID	0xff	/* 0x238 */
#define TEGRA_SWGROUP_AFI	0	/* 0x238 */
#define TEGRA_SWGROUP_AVPC	1	/* 0x23c */
#define TEGRA_SWGROUP_DC	2	/* 0x240 */
#define TEGRA_SWGROUP_DCB	3	/* 0x244 */
#define TEGRA_SWGROUP_EPP	4	/* 0x248 */
#define TEGRA_SWGROUP_G2	5	/* 0x24c */
#define TEGRA_SWGROUP_HC	6	/* 0x250 */
#define TEGRA_SWGROUP_HDA	7	/* 0x254 */
#define TEGRA_SWGROUP_ISP	8	/* 0x258 */
#define TEGRA_SWGROUP_ISP2	8
#define TEGRA_SWGROUP_DC14	9	/* 0x490 *//* Exceptional non-linear */
#define TEGRA_SWGROUP_DC12	10	/* 0xa88 *//* Exceptional non-linear */
#define TEGRA_SWGROUP_MPE	11	/* 0x264 */
#define TEGRA_SWGROUP_MSENC	11
#define TEGRA_SWGROUP_NVENC	11
#define TEGRA_SWGROUP_NV	12	/* 0x268 */
#define TEGRA_SWGROUP_NV2	13	/* 0x26c */
#define TEGRA_SWGROUP_PPCS	14	/* 0x270 */
#define TEGRA_SWGROUP_SATA2	15	/* 0x274 */
#define TEGRA_SWGROUP_SATA	16	/* 0x278 */
#define TEGRA_SWGROUP_VDE	17	/* 0x27c */
#define TEGRA_SWGROUP_VI	18	/* 0x280 */
#define TEGRA_SWGROUP_VII2C	18	/* 0x280 */
#define TEGRA_SWGROUP_VIC	19	/* 0x284 */
#define TEGRA_SWGROUP_XUSB_HOST	20	/* 0x288 */
#define TEGRA_SWGROUP_XUSB_DEV	21	/* 0x28c */
#define TEGRA_SWGROUP_A9AVP	22	/* 0x290 */
#define TEGRA_SWGROUP_TSEC	23	/* 0x294 */
#define TEGRA_SWGROUP_PPCS1	24	/* 0x298 */
#define TEGRA_SWGROUP_SDMMC1A	25	/* 0xa94 *//* Linear shift again */
#define TEGRA_SWGROUP_SDMMC2A	26	/* 0xa98 */
#define TEGRA_SWGROUP_SDMMC3A	27	/* 0xa9c */
#define TEGRA_SWGROUP_SDMMC4A	28	/* 0xaa0 */
#define TEGRA_SWGROUP_ISP2B	29	/* 0xaa4 */
#define TEGRA_SWGROUP_GPU	30	/* 0xaa8, DO NOT USE THIS */
#define TEGRA_SWGROUP_GPUB	31	/* 0xaac */
#define TEGRA_SWGROUP_PPCS2	32	/* 0xab0 */
#define TEGRA_SWGROUP_NVDEC	33	/* 0xab4 */
#define TEGRA_SWGROUP_APE	34	/* 0xab8 */
#define TEGRA_SWGROUP_SE	35	/* 0xabc */
#define TEGRA_SWGROUP_NVJPG	36	/* 0xac0 */
#define TEGRA_SWGROUP_HC1	37	/* 0xac4 */
#define TEGRA_SWGROUP_SE1	38	/* 0xac8 */
#define TEGRA_SWGROUP_AXIAP	39	/* 0xacc */
#define TEGRA_SWGROUP_ETR	40	/* 0xad0 */
#define TEGRA_SWGROUP_TSECB	41	/* 0xad4 */
#define TEGRA_SWGROUP_TSEC1	42	/* 0xad8 */
#define TEGRA_SWGROUP_TSECB1	43	/* 0xadc */
#define TEGRA_SWGROUP_NVDEC1	44	/* 0xae0 */
/*	Reserved		45 */
#define TEGRA_SWGROUP_AXIS	46	/* 0xae8 */
#define TEGRA_SWGROUP_EQOS	47	/* 0xaec */
#define TEGRA_SWGROUP_UFSHC	48	/* 0xaf0 */
#define TEGRA_SWGROUP_NVDISPLAY	49	/* 0xaf4 */
#define TEGRA_SWGROUP_BPMP	50	/* 0xaf8 */
#define TEGRA_SWGROUP_AON	51	/* 0xafc */
#define TEGRA_SWGROUP_SMMU_TEST	52
#define TEGRA_SWGROUP_ISP2B1	53	/* 0x808, SE2 on t210b01 */
/*	Reserved		50 */

#define TWO_U32_OF_U64(x)	((x) & 0xffffffff) ((x) >> 32)
#define TEGRA_SWGROUP_BIT(x)	(1ULL << TEGRA_SWGROUP_##x)
#define TEGRA_SWGROUP_CELLS(x)	TWO_U32_OF_U64(TEGRA_SWGROUP_BIT(x))

#define TEGRA_SWGROUP_CELLS2(x1, x2)	 \
				TWO_U32_OF_U64( TEGRA_SWGROUP_BIT(x1) | \
						TEGRA_SWGROUP_BIT(x2))
#define TEGRA_SWGROUP_CELLS3(x1, x2, x3)	 \
				TWO_U32_OF_U64( TEGRA_SWGROUP_BIT(x1) | \
						TEGRA_SWGROUP_BIT(x2) | \
						TEGRA_SWGROUP_BIT(x3))
#define TEGRA_SWGROUP_CELLS4(x1, x2, x3, x4)	 \
				TWO_U32_OF_U64( TEGRA_SWGROUP_BIT(x1) | \
						TEGRA_SWGROUP_BIT(x2) | \
						TEGRA_SWGROUP_BIT(x3) | \
						TEGRA_SWGROUP_BIT(x4))
#define TEGRA_SWGROUP_CELLS5(x1, x2, x3, x4, x5)	\
				TWO_U32_OF_U64( TEGRA_SWGROUP_BIT(x1) | \
						TEGRA_SWGROUP_BIT(x2) | \
						TEGRA_SWGROUP_BIT(x3) | \
						TEGRA_SWGROUP_BIT(x4) | \
						TEGRA_SWGROUP_BIT(x5))
#define TEGRA_SWGROUP_CELLS9(x1, x2, x3, x4, x5, x6, x7, x8, x9) \
				TWO_U32_OF_U64( TEGRA_SWGROUP_BIT(x1) | \
						TEGRA_SWGROUP_BIT(x2) | \
						TEGRA_SWGROUP_BIT(x3) | \
						TEGRA_SWGROUP_BIT(x4) | \
						TEGRA_SWGROUP_BIT(x5) | \
						TEGRA_SWGROUP_BIT(x6) | \
						TEGRA_SWGROUP_BIT(x7) | \
						TEGRA_SWGROUP_BIT(x8) | \
						TEGRA_SWGROUP_BIT(x9))
#define TEGRA_SWGROUP_CELLS12(x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12) \
				TWO_U32_OF_U64( TEGRA_SWGROUP_BIT(x1) | \
						TEGRA_SWGROUP_BIT(x2) | \
						TEGRA_SWGROUP_BIT(x3) | \
						TEGRA_SWGROUP_BIT(x4) | \
						TEGRA_SWGROUP_BIT(x5) | \
						TEGRA_SWGROUP_BIT(x6) | \
						TEGRA_SWGROUP_BIT(x7) | \
						TEGRA_SWGROUP_BIT(x8) | \
						TEGRA_SWGROUP_BIT(x9) | \
						TEGRA_SWGROUP_BIT(x10)| \
						TEGRA_SWGROUP_BIT(x11)| \
						TEGRA_SWGROUP_BIT(x12))
#define TEGRA_SWGROUP_MAX	64

#define SWGIDS_ERROR_CODE	(~0ULL)
#define swgids_is_error(x)	((x) == SWGIDS_ERROR_CODE)

/*
 * The above definitions are for the older tegra chips using the Tegra SMMU. For
 * tegra chips using the ARM SMMU the following is used. The notion of bit maps
 * is removed since they are not very scalable.
 */

/* Host clients. */
#define TEGRA_SID_HC		0x1U	/* 1 */
#define TEGRA_SID_VIC		0x3U	/* 3 */
#define TEGRA_SID_VI		0x4U	/* 4 */
#define TEGRA_SID_ISP		0x5U	/* 5 */
#define TEGRA_SID_NVDEC		0x6U	/* 6 */
#define TEGRA_SID_NVENC		0x7U	/* 7 */
#define TEGRA_SID_NVJPG		0x8U	/* 8 */
#define TEGRA_SID_NVDISPLAY	0x9U	/* 9 */
#define TEGRA_SID_TSEC		0xaU	/* 10 */
#define TEGRA_SID_TSECB		0xbU	/* 11 */
#define TEGRA_SID_SE		0xcU	/* 12 */
#define TEGRA_SID_SE1		0xdU	/* 13 */

/* GPU clients. */
#define TEGRA_SID_GPUB		0x10U	/* 16 */

/* Other SoC clients */
#define TEGRA_SID_AFI		0x11U	/* 17 */
#define TEGRA_SID_HDA		0x12U	/* 18 */
#define TEGRA_SID_ETR		0x13U	/* 19 */
#define TEGRA_SID_EQOS		0x14U	/* 20 */
#define TEGRA_SID_UFSHC		0x15U	/* 21 */
#define TEGRA_SID_AON		0x16U	/* 22 */
#define TEGRA_SID_SDMMC4A	0x17U	/* 23 */
#define TEGRA_SID_SDMMC3A	0x18U	/* 24 */
#define TEGRA_SID_SDMMC2A	0x19U	/* 25 */
#define TEGRA_SID_SDMMC1A	0x1aU	/* 26 */
#define TEGRA_SID_XUSB_HOST	0x1bU	/* 27 */
#define TEGRA_SID_XUSB_DEV	0x1cU	/* 28 */
#define TEGRA_SID_SATA2		0x1dU	/* 29 */
#define TEGRA_SID_APE		0x1eU	/* 30 */

/*
 * The BPMP has hard coded their SID value in their FW which is not built
 * in the normal Linux tree. As a result, changing the SID requires a
 * considerable amount of work.
 */
#define TEGRA_SID_BPMP		0x32U	/* 50 */

/* For smmu tests */
#define TEGRA_SID_SMMU_TEST	0x33U	/* 51 */

/*
 * This is the t18x specific component of the new SID dt-binding.
 */
#define TEGRA_SID_NVCSI		0x2U	/* 2 */
#define TEGRA_SID_SE2		0xeU	/* 14 */
#define TEGRA_SID_SE3		0xfU	/* 15 */
#define TEGRA_SID_SCE		0x1fU	/* 31 */

/* The GPC DMA clients. */
#define TEGRA_SID_GPCDMA_0	0x20U	/* 32 */
#define TEGRA_SID_GPCDMA_1	0x21U	/* 33 */
#define TEGRA_SID_GPCDMA_2	0x22U	/* 34 */
#define TEGRA_SID_GPCDMA_3	0x23U	/* 35 */
#define TEGRA_SID_GPCDMA_4	0x24U	/* 36 */
#define TEGRA_SID_GPCDMA_5	0x25U	/* 37 */
#define TEGRA_SID_GPCDMA_6	0x26U	/* 38 */
#define TEGRA_SID_GPCDMA_7	0x27U	/* 39 */

/* The APE DMA Clients. */
#define TEGRA_SID_APE_1		0x28U	/* 40 */
#define TEGRA_SID_APE_2		0x29U	/* 41 */

/* The Camera RTCPU. */
#define TEGRA_SID_RCE		0x2aU	/* 42 */

/* The Camera RTCPU on Host1x address space. */
#define TEGRA_SID_RCE_1X	0x2bU	/* 43 */

/* The APE DMA Clients. */
#define TEGRA_SID_APE_3		0x2cU	/* 44 */

/* The Camera RTCPU running on APE */
#define TEGRA_SID_APE_CAM	0x2dU	/* 45 */
#define TEGRA_SID_APE_CAM_1X	0x2eU	/* 46 */

/* Host1x virtualization clients. */
#define TEGRA_SID_HOST1X_CTX0	0x38U	/* 56 */
#define TEGRA_SID_HOST1X_CTX1	0x39U	/* 57 */
#define TEGRA_SID_HOST1X_CTX2	0x3aU	/* 58 */
#define TEGRA_SID_HOST1X_CTX3	0x3bU	/* 59 */
#define TEGRA_SID_HOST1X_CTX4	0x3cU	/* 60 */
#define TEGRA_SID_HOST1X_CTX5	0x3dU	/* 61 */
#define TEGRA_SID_HOST1X_CTX6	0x3eU	/* 62 */
#define TEGRA_SID_HOST1X_CTX7	0x3fU	/* 63 */

/* Host1x command buffers */
#define TEGRA_SID_HC_VM0	0x40U
#define TEGRA_SID_HC_VM1	0x41U
#define TEGRA_SID_HC_VM2	0x42U
#define TEGRA_SID_HC_VM3	0x43U
#define TEGRA_SID_HC_VM4	0x44U
#define TEGRA_SID_HC_VM5	0x45U
#define TEGRA_SID_HC_VM6	0x46U
#define TEGRA_SID_HC_VM7	0x47U

/* SE data buffers */
#define TEGRA_SID_SE_VM0	0x48U
#define TEGRA_SID_SE_VM1	0x49U
#define TEGRA_SID_SE_VM2	0x4aU
#define TEGRA_SID_SE_VM3	0x4bU
#define TEGRA_SID_SE_VM4	0x4cU
#define TEGRA_SID_SE_VM5	0x4dU
#define TEGRA_SID_SE_VM6	0x4eU
#define TEGRA_SID_SE_VM7	0x4fU

/* XUSB virtual functions */
#define TEGRA_SID_XUSB_VF0	0x5dU
#define TEGRA_SID_XUSB_VF1	0x5eU
#define TEGRA_SID_XUSB_VF2	0x5fU
#define TEGRA_SID_XUSB_VF3	0x60U

/* Special clients */
#define TEGRA_SID_PASSTHROUGH	0x7fU
#define TEGRA_SID_INVALID	0x0U

/*
 * These macros will be removed once the bitmap problem is sorted out. Until
 * then this is equivalent to TEGRA_SWGROUP_CELLS() only with TEGRA_SID_*
 * instead of TEGRA_SWGROUP_*.
 */
#define TEGRA_SID(x) 		(TEGRA_SID_ ## x)

/*
 * These are unique id's that the IOMMU uses to put different
 * devices into the same IOMMU group and shared address space
 * Add this to a devices iommu-group-id property
 */
#define TEGRA_IOMMU_GROUP_HOST1X		0x1
#define TEGRA_IOMMU_GROUP_APE			0x2
#define TEGRA_IOMMU_GROUP_RTCPU			0x3
#define TEGRA_IOMMU_GROUP_SE			0x4


#endif /* _DT_BINDINGS_MEMORY_TEGRA_SWGROUP_H */
