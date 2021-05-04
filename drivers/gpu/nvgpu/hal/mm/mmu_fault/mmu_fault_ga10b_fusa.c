/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <nvgpu/dma.h>
#include <nvgpu/log.h>
#include <nvgpu/enabled.h>
#include <nvgpu/gmmu.h>
#include <nvgpu/bug.h>
#include <nvgpu/io.h>
#include <nvgpu/utils.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/mmu_fault.h>

#include "hal/mm/mmu_fault/mmu_fault_ga10b.h"

#include <nvgpu/hw/ga10b/hw_gmmu_ga10b.h>

static const char mmufault_invalid_str[] = "invalid";

static const char *const ga10b_fault_type_descs[] = {
	"invalid pde",
	"invalid pde size",
	"invalid pte",
	"limit violation",
	"unbound inst block",
	"priv violation",
	"write, ro violation",
	"read, wo violation",
	"pitch mask violation",
	"work creation",
	"unsupported aperture",
	"compression failure",
	"unsupported kind",
	"region violation",
	"poison",
	"atomic violation"
};

static const char *const ga10b_fault_client_type_descs[] = {
	"gpc",
	"hub",
};

static const char *const ga10b_hub_client_descs[] = {
	"vip", "ce0", "ce1", "dniso",
	"dispniso", "fe0", "fe", "fecs0",
	"fecs", "host", "host_cpu", "host_cpu_nb",
	"iso", "mmu", "nvdec0", "nvdec",
	"ce3", "nvenc1", "niso", "actrs",
	"p2p", "pd", "perf0/perf", "pmu",
	"rastertwod", "scc", "scc nb", "sec",
	"ssync", "grcopy/ce2", "xv", "mmu nb",
	"nvenc0/nvenc", "unused", "sked0/sked",
	"dont_care",
	"hsce0", "hsce1", "hsce2", "hsce3", "hsce4", "hsce5", "hsce6",
	"hsce7", "hsce8", "hsce9",
	"hshub", "ptp_x0", "ptp_x1", "ptp_x2", "ptp_x3",
	"ptp_x4", "ptp_x5", "ptp_x6", "ptp_x7",
	"nvenc2", "vpr scrubber0", "vpr scrubber1", "dwbif", "fbfalcon",
	"ce shim", "gsp",
	"nvdec1", "nvdec2", "nvjpg0", "nvdec3", "nvdec4", "ofa0",
	"hsce10", "hsce11", "hsce12", "hsce13", "hsce14", "hsce15",
	"ptp_x8", "ptp_x9", "ptp_x10", "ptp_x11", "ptp_x12",
	"ptp_x13", "ptp_x14", "ptp_x15",
	"fe1", "fe2", "fe3", "fe4", "fe5", "fe6", "fe7",
	"fecs1", "fecs2", "fecs3", "fecs4", "fecs5", "fecs6", "fecs7",
	"sked1", "sked2", "sked3", "sked4", "sked5", "sked6", "sked7",
	"esc"
};

static const char *const ga10b_gpc_client_descs[] = {
	"t1_0", "t1_1", "t1_2", "t1_3",
	"t1_4", "t1_5", "t1_6", "t1_7",
	"pe_0", "pe_1", "pe_2", "pe_3",
	"pe_4", "pe_5", "pe_6", "pe_7",
	"rast", "gcc", "gpccs",
	"prop_0", "prop_1", "prop_2", "prop_3",
	"t1_8", "t1_9", "t1_10", "t1_11",
	"t1_12", "t1_13", "t1_14", "t1_15",
	"tpccs_0", "tpccs_1", "tpccs_2", "tpccs_3",
	"tpccs_4", "tpccs_5", "tpccs_6", "tpccs_7",
	"pe_8", "pe_9", "tpccs_8", "tpccs_9",
	"t1_16", "t1_17", "t1_18", "t1_19",
	"pe_8",  "pe_9", "tpccs_8", "tpccs_9",
	"t1_16", "t1_17", "t1_18", "t1_19",
	"pe_10", "pe_11", "tpccs_10", "tpccs_11",
	"t1_20", "t1_21", "t1_22", "t1_23",
	"pe_12", "pe_13", "tpccs_12", "tpccs_13",
	"t1_24", "t1_25", "t1_26", "t1_27",
	"pe_14", "pe_15", "tpccs_14", "tpccs_15",
	"t1_28", "t1_29", "t1_30", "t1_31",
	"pe_16", "pe_17", "tpccs_16", "tpccs_17",
	"t1_32", "t1_33", "t1_34", "t1_35",
	"pe_18", "pe_19", "tpccs_18", "tpccs_19",
	"t1_36", "t1_37", "t1_38", "t1_39",
	"rop_0", "rop_1", "rop_2", "rop_3",
};

void ga10b_mm_mmu_fault_parse_mmu_fault_info(struct mmu_fault_info *mmufault)
{
	if (mmufault->mmu_engine_id == gmmu_fault_mmu_eng_id_bar2_v()) {
		mmufault->mmu_engine_id_type = NVGPU_MMU_ENGINE_ID_TYPE_BAR2;

	} else if (mmufault->mmu_engine_id ==
			gmmu_fault_mmu_eng_id_physical_v()) {
		mmufault->mmu_engine_id_type = NVGPU_MMU_ENGINE_ID_TYPE_PHYSICAL;
	} else {
		mmufault->mmu_engine_id_type = NVGPU_MMU_ENGINE_ID_TYPE_OTHER;
	}

	if (mmufault->fault_type > gmmu_fault_fault_type_atomic_violation_v()) {
		nvgpu_do_assert();
		mmufault->fault_type_desc = mmufault_invalid_str;
	} else {
		mmufault->fault_type_desc =
			 ga10b_fault_type_descs[mmufault->fault_type];
	}

	if (mmufault->client_type > gmmu_fault_client_type_hub_v()) {
		nvgpu_do_assert();
		mmufault->client_type_desc = mmufault_invalid_str;
	} else {
		mmufault->client_type_desc =
			 ga10b_fault_client_type_descs[mmufault->client_type];
	}

	mmufault->client_id_desc = mmufault_invalid_str;
	if (mmufault->client_type == gmmu_fault_client_type_hub_v()) {
		if (mmufault->client_id <= gmmu_fault_client_hub_esc_v()) {
			mmufault->client_id_desc =
				 ga10b_hub_client_descs[mmufault->client_id] ==
					NULL ? "TBD" :
				ga10b_hub_client_descs[mmufault->client_id];
		} else {
			nvgpu_do_assert();
		}
	} else if (mmufault->client_type ==
			gmmu_fault_client_type_gpc_v()) {
		if (mmufault->client_id <= gmmu_fault_client_gpc_rop_3_v()) {
			mmufault->client_id_desc =
				 ga10b_gpc_client_descs[mmufault->client_id] ==
					NULL ? "TBD" :
				ga10b_gpc_client_descs[mmufault->client_id];
		} else {
			nvgpu_do_assert();
		}
	} else {
		/* Nothing to do here */
	}
}
