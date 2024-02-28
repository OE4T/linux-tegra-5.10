/*
 * arch/arm/mach-tegra/include/mach/latency_allowance.h
 *
 * Copyright (C) 2011-2015, NVIDIA CORPORATION. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _MACH_TEGRA_LATENCY_ALLOWANCE_H_
#define _MACH_TEGRA_LATENCY_ALLOWANCE_H_

#define FIRST_DISP_CLIENT_ID	TEGRA_LA_DISPLAY_0A
#define DISP_CLIENT_LA_ID(id)	(id - FIRST_DISP_CLIENT_ID)


/* Note:- When adding new display realted IDs, please add them adjacent/amongst
	  the existing display related IDs. This is required because certain
	  display related macros/strcuts assume that all display related
	  tegra_la_ids are adjacent to each other.

	  Please observe the same guidelines as display clients, when adding new
	  camera clients. All camera clients need to be located adjacent to each
	  other in tegra_la_id. This is required because certain camera related
	  macros/structs assume that all camera related tegra_la_ids are
	  adjacent to each other. */
enum tegra_la_id {
	TEGRA_LA_AFIR = 0,			/* T30 specific */
	TEGRA_LA_AFIW,				/* T30 specific */
	TEGRA_LA_AVPC_ARM7R,
	TEGRA_LA_AVPC_ARM7W,
	TEGRA_LA_DISPLAY_0A,
	TEGRA_LA_DISPLAY_0B,
	TEGRA_LA_DISPLAY_0C,
	TEGRA_LA_DISPLAY_1B,			/* T30 specific */
	TEGRA_LA_DISPLAY_HC,
	TEGRA_LA_DISPLAY_0AB,
	TEGRA_LA_DISPLAY_0BB,
	TEGRA_LA_DISPLAY_0CB,
	TEGRA_LA_DISPLAY_1BB,			/* T30 specific */
	TEGRA_LA_DISPLAY_HCB,
	TEGRA_LA_DISPLAY_T,			/* T14x specific */
	TEGRA_LA_DISPLAYD,			/* T14x specific */
	TEGRA_LA_EPPUP,
	TEGRA_LA_EPPU,
	TEGRA_LA_EPPV,
	TEGRA_LA_EPPY,
	TEGRA_LA_G2PR,
	TEGRA_LA_G2SR,
	TEGRA_LA_G2DR,
	TEGRA_LA_G2DW,
	TEGRA_LA_GPUSRD,			/* T12x specific */
	TEGRA_LA_GPUSWR,			/* T12x specific */
	TEGRA_LA_HOST1X_DMAR,
	TEGRA_LA_HOST1XR,
	TEGRA_LA_HOST1XW,
	TEGRA_LA_HDAR,
	TEGRA_LA_HDAW,
	TEGRA_LA_ISPW,
	TEGRA_LA_MPCORER,
	TEGRA_LA_MPCOREW,
	TEGRA_LA_MPCORE_LPR,
	TEGRA_LA_MPCORE_LPW,
	TEGRA_LA_MPE_UNIFBR,			/* T30 specific */
	TEGRA_LA_MPE_IPRED,			/* T30 specific */
	TEGRA_LA_MPE_AMEMRD,			/* T30 specific */
	TEGRA_LA_MPE_CSRD,			/* T30 specific */
	TEGRA_LA_MPE_UNIFBW,			/* T30 specific */
	TEGRA_LA_MPE_CSWR,			/* T30 specific */
	TEGRA_LA_FDCDRD,
	TEGRA_LA_IDXSRD,
	TEGRA_LA_TEXSRD,
	TEGRA_LA_TEXL2SRD = TEGRA_LA_TEXSRD,	/* T11x, T14x specific */
	TEGRA_LA_FDCDWR,
	TEGRA_LA_FDCDRD2,
	TEGRA_LA_IDXSRD2,			/* T30 specific */
	TEGRA_LA_TEXSRD2,			/* T30 specific */
	TEGRA_LA_FDCDWR2,
	TEGRA_LA_PPCS_AHBDMAR,
	TEGRA_LA_PPCS_AHBSLVR,
	TEGRA_LA_PPCS_AHBDMAW,
	TEGRA_LA_PPCS_AHBSLVW,
	TEGRA_LA_PTCR,
	TEGRA_LA_SATAR,				/* T30, T19x */
	TEGRA_LA_SATAW,				/* T30, T19x */
	TEGRA_LA_VDE_BSEVR,
	TEGRA_LA_VDE_MBER,
	TEGRA_LA_VDE_MCER,
	TEGRA_LA_VDE_TPER,
	TEGRA_LA_VDE_BSEVW,
	TEGRA_LA_VDE_DBGW,
	TEGRA_LA_VDE_MBEW,
	TEGRA_LA_VDE_TPMW,
	TEGRA_LA_VI_RUV,			/* T30 specific */
	TEGRA_LA_VI_WSB,
	TEGRA_LA_VI_WU,
	TEGRA_LA_VI_WV,
	TEGRA_LA_VI_WY,

	TEGRA_LA_MSENCSRD,			/* T11x, T14x specific */
	TEGRA_LA_MSENCSWR,			/* T11x, T14x specific */
	TEGRA_LA_XUSB_HOSTR,			/* T11x, T19x */
	TEGRA_LA_XUSB_HOSTW,			/* T11x, T19x */
	TEGRA_LA_XUSB_DEVR,			/* T11x, T19x */
	TEGRA_LA_XUSB_DEVW,			/* T11x, T19x */
	TEGRA_LA_FDCDRD3,			/* T11x specific */
	TEGRA_LA_FDCDRD4,			/* T11x specific */
	TEGRA_LA_FDCDWR3,			/* T11x specific */
	TEGRA_LA_FDCDWR4,			/* T11x specific */
	TEGRA_LA_EMUCIFR,			/* T11x, T14x specific */
	TEGRA_LA_EMUCIFW,			/* T11x, T14x specific */
	TEGRA_LA_TSECSRD,			/* T11x, T14x, T19x */
	TEGRA_LA_TSECSWR,			/* T11x, T14x, T19x */

	TEGRA_LA_VI_W,				/* T14x specific */
	TEGRA_LA_ISP_RA,			/* T14x specific */
	TEGRA_LA_ISP_WA,			/* T14x specific */
	TEGRA_LA_ISP_WB,			/* T14x specific */
	TEGRA_LA_ISP_RAB,			/* T12x specific */
	TEGRA_LA_ISP_WAB,			/* T12x specific */
	TEGRA_LA_ISP_WBB,			/* T12x specific */
	TEGRA_LA_BBCR,				/* T14x specific */
	TEGRA_LA_BBCW,				/* T14x specific */
	TEGRA_LA_BBCLLR,			/* T14x specific */
	TEGRA_LA_SDMMCR,			/* T12x, T19x */
	TEGRA_LA_SDMMCRA,			/* T12x, T19x */
	TEGRA_LA_SDMMCRAA,			/* T12x specific */
	TEGRA_LA_SDMMCRAB,			/* T12x, T19x */
	TEGRA_LA_SDMMCW,			/* T12x, T19x */
	TEGRA_LA_SDMMCWA,			/* T12x, T19x */
	TEGRA_LA_SDMMCWAA,			/* T12x specific */
	TEGRA_LA_SDMMCWAB,			/* T12x, T19x */
	TEGRA_LA_VICSRD,			/* T12x, T19x */
	TEGRA_LA_VICSWR,			/* T12x, T19x */

	TEGRA_LA_TSECBSRD,			/* T21x specific */
	TEGRA_LA_TSECBSWR,			/* T21x specific */

	TEGRA_LA_NVDECR,			/* T21x specific */
	TEGRA_LA_NVDECW,			/* T21x specific */

	TEGRA_LA_AONR,				/* T18x, T19x */
	TEGRA_LA_AONW,				/* T18x, T19x */
	TEGRA_LA_AONDMAR,			/* T18x, T19x */
	TEGRA_LA_AONDMAW,			/* T18x, T19x */
	TEGRA_LA_APEDMAR,			/* T18x, T19x */
	TEGRA_LA_APEDMAW,			/* T18x, T19x */
	TEGRA_LA_APER,				/* T18x, T19x */
	TEGRA_LA_APEW,				/* T18x, T19x */
	TEGRA_LA_AXISR,				/* T18x, T19x */
	TEGRA_LA_AXISW,				/* T18x, T19x */
	TEGRA_LA_BPMPR,				/* T18x, T19x */
	TEGRA_LA_BPMPW,				/* T18x, T19x */
	TEGRA_LA_BPMPDMAR,			/* T18x, T19x */
	TEGRA_LA_BPMPDMAW,			/* T18x, T19x */
	TEGRA_LA_EQOSR,				/* T18x, T19x */
	TEGRA_LA_EQOSW,				/* T18x, T19x */
	TEGRA_LA_ETRR,				/* T18x, T19x */
	TEGRA_LA_ETRW,				/* T18x, T19x */
	TEGRA_LA_GPUSRD2,			/* T18x specific */
	TEGRA_LA_GPUSWR2,			/* T18x specific */
	TEGRA_LA_NVDISPLAYR,			/* T18x, T19x */
	TEGRA_LA_NVENCSRD,			/* T18x, T19x */
	TEGRA_LA_NVENCSWR,			/* T18x, T19x */
	TEGRA_LA_NVJPGSRD,			/* T18x, T19x */
	TEGRA_LA_NVJPGSWR,			/* T18x, T19x */
	TEGRA_LA_SCER,				/* T18x, T19x */
	TEGRA_LA_SCEW,				/* T18x, T19x */
	TEGRA_LA_SCEDMAR,			/* T18x, T19x */
	TEGRA_LA_SCEDMAW,			/* T18x, T19x */
	TEGRA_LA_SESRD,				/* T18x, T19x */
	TEGRA_LA_SESWR,				/* T18x, T19x */
	TEGRA_LA_UFSHCR,			/* T18x, T19x */
	TEGRA_LA_UFSHCW,			/* T18x, T19x */

	TEGRA_LA_AXIAPR,			/* T19x specific */
	TEGRA_LA_AXIAPW,			/* T19x specific */
	TEGRA_LA_CIFLL_WR,			/* T19x specific */
	TEGRA_LA_DLA0FALRDB,			/* T19x specific */
	TEGRA_LA_DLA0RDA,			/* T19x specific */
	TEGRA_LA_DLA0FALWRB,			/* T19x specific */
	TEGRA_LA_DLA0WRA,			/* T19x specific */
	TEGRA_LA_DLA0RDA1,			/* T19x specific */
	TEGRA_LA_DLA1RDA1,			/* T19x specific */
	TEGRA_LA_DLA1FALRDB,			/* T19x specific */
	TEGRA_LA_DLA1RDA,			/* T19x specific */
	TEGRA_LA_DLA1FALWRB,			/* T19x specific */
	TEGRA_LA_DLA1WRA,			/* T19x specific */
	TEGRA_LA_HOST1XDMAR,			/* T19x specific */
	TEGRA_LA_ISPFALR,			/* T19x specific */
	TEGRA_LA_ISPRA,				/* T19x specific */
	TEGRA_LA_ISPWA,				/* T19x specific */
	TEGRA_LA_ISPWB,				/* T19x specific */
	TEGRA_LA_ISPFALW,			/* T19x specific */
	TEGRA_LA_ISPRA1,			/* T19x specific */
	TEGRA_LA_MIU0R,				/* T19x specific */
	TEGRA_LA_MIU0W,				/* T19x specific */
	TEGRA_LA_MIU1R,				/* T19x specific */
	TEGRA_LA_MIU1W,				/* T19x specific */
	TEGRA_LA_MIU2R,				/* T19x specific */
	TEGRA_LA_MIU2W,				/* T19x specific */
	TEGRA_LA_MIU3R,				/* T19x specific */
	TEGRA_LA_MIU3W,				/* T19x specific */
	TEGRA_LA_MIU4R,				/* T19x specific */
	TEGRA_LA_MIU4W,				/* T19x specific */
	TEGRA_LA_MIU5R,				/* T19x specific */
	TEGRA_LA_MIU5W,				/* T19x specific */
	TEGRA_LA_MIU6R,				/* T19x specific */
	TEGRA_LA_MIU6W,				/* T19x specific */
	TEGRA_LA_MIU7R,				/* T19x specific */
	TEGRA_LA_MIU7W,				/* T19x specific */
	TEGRA_LA_NVDECSRD,			/* T19x specific */
	TEGRA_LA_NVDECSWR,			/* T19x specific */
	TEGRA_LA_NVDEC1SRD,			/* T19x specific */
	TEGRA_LA_NVDECSRD1,			/* T19x specific */
	TEGRA_LA_NVDEC1SRD1,			/* T19x specific */
	TEGRA_LA_NVDEC1SWR,			/* T19x specific */
	TEGRA_LA_NVENC1SRD,			/* T19x specific */
	TEGRA_LA_NVENC1SWR,			/* T19x specific */
	TEGRA_LA_NVENC1SRD1,			/* T19x specific */
	TEGRA_LA_NVENCSRD1,			/* T19x specific */
	TEGRA_LA_PCIE0R,			/* T19x specific */
	TEGRA_LA_PCIE0W,			/* T19x specific */
	TEGRA_LA_PCIE1R,			/* T19x specific */
	TEGRA_LA_PCIE1W,			/* T19x specific */
	TEGRA_LA_PCIE2AR,			/* T19x specific */
	TEGRA_LA_PCIE2AW,			/* T19x specific */
	TEGRA_LA_PCIE3R,			/* T19x specific */
	TEGRA_LA_PCIE3W,			/* T19x specific */
	TEGRA_LA_PCIE4R,			/* T19x specific */
	TEGRA_LA_PCIE4W,			/* T19x specific */
	TEGRA_LA_PCIE5R,			/* T19x specific */
	TEGRA_LA_PCIE5W,			/* T19x specific */
	TEGRA_LA_PCIE0R1,			/* T19x specific */
	TEGRA_LA_PCIE5R1,			/* T19x specific */
	TEGRA_LA_PVA0RDA,			/* T19x specific */
	TEGRA_LA_PVA0RDB,			/* T19x specific */
	TEGRA_LA_PVA0RDC,			/* T19x specific */
	TEGRA_LA_PVA0WRA,			/* T19x specific */
	TEGRA_LA_PVA0WRB,			/* T19x specific */
	TEGRA_LA_PVA0WRC,			/* T19x specific */
	TEGRA_LA_PVA0RDA1,			/* T19x specific */
	TEGRA_LA_PVA0RDB1,			/* T19x specific */
	TEGRA_LA_PVA1RDA,			/* T19x specific */
	TEGRA_LA_PVA1RDB,			/* T19x specific */
	TEGRA_LA_PVA1RDC,			/* T19x specific */
	TEGRA_LA_PVA1WRA,			/* T19x specific */
	TEGRA_LA_PVA1WRB,			/* T19x specific */
	TEGRA_LA_PVA1WRC,			/* T19x specific */
	TEGRA_LA_PVA1RDA1,			/* T19x specific */
	TEGRA_LA_PVA1RDB1,			/* T19x specific */
	TEGRA_LA_RCEDMAR,			/* T19x specific */
	TEGRA_LA_RCEDMAW,			/* T19x specific */
	TEGRA_LA_RCER,				/* T19x specific */
	TEGRA_LA_RCEW,				/* T19x specific */
	TEGRA_LA_TSECSRDB,			/* T19x specific */
	TEGRA_LA_TSECSWRB,			/* T19x specific */
	TEGRA_LA_VIW,				/* T19x specific */
	TEGRA_LA_VICSRD1,			/* T19x specific */
	TEGRA_LA_VIFALR,			/* T19x specific */
	TEGRA_LA_VIFALW,			/* T19x specific */
	TEGRA_LA_WCAM,				/* T19x specific */
	TEGRA_LA_NVLRHP,			/* T19x specific */
	TEGRA_LA_DGPU,				/* T19x specific */
	TEGRA_LA_IGPU,				/* T19x specific */

	TEGRA_LA_MAX_ID
};

enum disp_win_type {
	TEGRA_LA_DISP_WIN_TYPE_FULL,
	TEGRA_LA_DISP_WIN_TYPE_FULLA,
	TEGRA_LA_DISP_WIN_TYPE_FULLB,
	TEGRA_LA_DISP_WIN_TYPE_SIMPLE,
	TEGRA_LA_DISP_WIN_TYPE_CURSOR,
	TEGRA_LA_DISP_WIN_TYPE_NUM_TYPES
};

struct disp_client {
	enum disp_win_type win_type;
	unsigned int mccif_size_bytes;
	unsigned int line_buf_sz_bytes;
};

struct dc_to_la_params {
	unsigned int thresh_lwm_bytes;
	unsigned int spool_up_buffering_adj_bytes;
	unsigned int drain_time_usec_fp;
	unsigned int total_dc0_bw;
	unsigned int total_dc1_bw;
};

struct la_to_dc_params {
	unsigned int fp_factor;
	unsigned int (*la_real_to_fp)(unsigned int val);
	unsigned int (*la_fp_to_real)(unsigned int val);
	unsigned int static_la_minus_snap_arb_to_row_srt_emcclks_fp;
	unsigned int dram_width_bits;
	unsigned int disp_catchup_factor_fp;
};

int tegra_set_disp_latency_allowance(enum tegra_la_id id,
					unsigned long emc_freq_hz,
					unsigned int bandwidth_in_mbps,
					struct dc_to_la_params disp_params);

int tegra_check_disp_latency_allowance(enum tegra_la_id id,
				       unsigned long emc_freq_hz,
				       unsigned int bw_mbps,
				       struct dc_to_la_params disp_params);

int tegra_set_latency_allowance(enum tegra_la_id id,
				unsigned int bandwidth_in_mbps);

int tegra_set_camera_ptsa(enum tegra_la_id id,
			unsigned int bw_mbps,
			int is_hiso);

void tegra_latency_allowance_update_tick_length(unsigned int new_ns_per_tick);

int tegra_enable_latency_scaling(enum tegra_la_id id,
				    unsigned int threshold_low,
				    unsigned int threshold_mid,
				    unsigned int threshold_high);

void tegra_disable_latency_scaling(enum tegra_la_id id);

void mc_pcie_init(void);

struct la_to_dc_params tegra_get_la_to_dc_params(void);

extern const struct disp_client *tegra_la_disp_clients_info;

#endif /* _MACH_TEGRA_LATENCY_ALLOWANCE_H_ */
