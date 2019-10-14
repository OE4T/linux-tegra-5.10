/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_ECC_H
#define NVGPU_ECC_H

/**
 * @file
 * @page unit-ecc Unit ECC(Error Control Codes)
 *
 * Acronyms
 * ========
 * ECC     - Error Control Codes
 * SEC     - Single Error Correction
 * SEC-DED - Standard single-error correction with double-error detection
 * SED     - Single Error Detection
 *
 * Overview
 * ========
 * The memories within the GPU are protected using data integrity protection
 * mechanism like ecc or parity. This unit is responsible for allocating,
 * initializing and maintaining error counters for all memories which support
 * ecc/parity protection.
 *
 * + Initialization:
 *   This unit allocates and initializes error counters (corrected and
 *   uncorrected) for each memory and concatenates them into a list.
 *
 * Data Structures
 * ===============
 *
 * The data structures exposed by the ECC unit, conveys to the user information
 * regarding the corrected, uncorrected errors encountered in the constituent
 * memories in the GPU hardware units like (gr, ltc, pmu, etc).
 *
 * The following are the list of data structures:
 *
 * + struct nvgpu_ecc_stat
 *
 *
 * + struct nvgpu_ecc
 *
 *
 * Static Design
 * =============
 *
 *	TODO
 *
 * Dynamic Design
 * =============
 *
 *	TODO
 *
 * External APIs
 * -------------
 *
 */

#include <nvgpu/types.h>
#include <nvgpu/list.h>

#define NVGPU_ECC_STAT_NAME_MAX_SIZE	100

struct gk20a;

/**
 * This struct holds the ecc/parity error information associated with each
 * memory. The error information includes a string that can be used to
 * uniquely identity the memory, error type. In addition it has a  32 bit
 * counter to track the number of instances of the errors.
 */
struct nvgpu_ecc_stat {
	/** The unique name associated with error */
	char name[NVGPU_ECC_STAT_NAME_MAX_SIZE];
	/** The 32-bit error counter */
	u32 counter;
	/**
	 * The embedded list element, this is used to link the counters into
	 * linked list.
	 */
	struct nvgpu_list_node node;
};

/**
 * @brief	Helper function to get struct nvgpu_ecc_stat from list node.
 *
 * @param node [in] List element node.
 *
 * @return Pointer to struct nvgpu_ecc_stat.
 *
 */
static inline struct nvgpu_ecc_stat *nvgpu_ecc_stat_from_node(
		struct nvgpu_list_node *node)
{
	return (struct nvgpu_ecc_stat *)(
			(uintptr_t)node - offsetof(struct nvgpu_ecc_stat, node)
		);
}

/**
 * The structure contains the error statistics assocaited with constituent
 * memories within each gpu hardware unit. All statistics are linked together
 * into a list, the head of this list is stored in stats_list.
 */
struct nvgpu_ecc {
	/**
	 * Contains error statistics for each memory contained within the gr
	 * unit.
	 */
	struct {
		/** SM register file SEC count. */
		struct nvgpu_ecc_stat **sm_lrf_ecc_single_err_count;
		/** SM register file DED count. */
		struct nvgpu_ecc_stat **sm_lrf_ecc_double_err_count;

		/** SM shared memory SEC count. */
		struct nvgpu_ecc_stat **sm_shm_ecc_sec_count;
		/** SM shared memory SED count. */
		struct nvgpu_ecc_stat **sm_shm_ecc_sed_count;
		/** SM shared memory DED count. */
		struct nvgpu_ecc_stat **sm_shm_ecc_ded_count;

		/** TEX pipe0 total SEC count. */
		struct nvgpu_ecc_stat **tex_ecc_total_sec_pipe0_count;
		/** TEX pipe0 total DED count. */
		struct nvgpu_ecc_stat **tex_ecc_total_ded_pipe0_count;
		/** TEX pipe0 unique SEC count. */
		struct nvgpu_ecc_stat **tex_unique_ecc_sec_pipe0_count;
		/** TEX pipe0 unique DED count. */
		struct nvgpu_ecc_stat **tex_unique_ecc_ded_pipe0_count;
		/** TEX pipe1 total SEC count. */
		struct nvgpu_ecc_stat **tex_ecc_total_sec_pipe1_count;
		/** TEX pipe1 total DED count. */
		struct nvgpu_ecc_stat **tex_ecc_total_ded_pipe1_count;
		/** TEX pipe1 unique SEC count. */
		struct nvgpu_ecc_stat **tex_unique_ecc_sec_pipe1_count;
		/** TEX pipe1 unique DED count. */
		struct nvgpu_ecc_stat **tex_unique_ecc_ded_pipe1_count;

		/** SM l1-tag correct error count. */
		struct nvgpu_ecc_stat **sm_l1_tag_ecc_corrected_err_count;
		/** SM l1-tag uncorrected error count. */
		struct nvgpu_ecc_stat **sm_l1_tag_ecc_uncorrected_err_count;
		/** SM CBU corrected error count. */
		struct nvgpu_ecc_stat **sm_cbu_ecc_corrected_err_count;
		/** SM CBU uncorrected error count. */
		struct nvgpu_ecc_stat **sm_cbu_ecc_uncorrected_err_count;
		/** SM l1-data corrected error count. */
		struct nvgpu_ecc_stat **sm_l1_data_ecc_corrected_err_count;
		/** SM l1-data uncorrected error count. */
		struct nvgpu_ecc_stat **sm_l1_data_ecc_uncorrected_err_count;
		/** SM icache corrected error count. */
		struct nvgpu_ecc_stat **sm_icache_ecc_corrected_err_count;
		/** SM icache uncorrected error count. */
		struct nvgpu_ecc_stat **sm_icache_ecc_uncorrected_err_count;

		/** GCC l1.5-cache corrected error count. */
		struct nvgpu_ecc_stat *gcc_l15_ecc_corrected_err_count;
		/** GCC l1.5-cache uncorrected error count. */
		struct nvgpu_ecc_stat *gcc_l15_ecc_uncorrected_err_count;

		/** GPCCS falcon i-mem, d-mem corrected error count. */
		struct nvgpu_ecc_stat *gpccs_ecc_corrected_err_count;
		/** GPCCS falcone i-mem, d-mem uncorrected error count. */
		struct nvgpu_ecc_stat *gpccs_ecc_uncorrected_err_count;

		/** GMMU l1tlb corrected error count. */
		struct nvgpu_ecc_stat *mmu_l1tlb_ecc_corrected_err_count;
		/** GMMU l1tlb uncorrected error count. */
		struct nvgpu_ecc_stat *mmu_l1tlb_ecc_uncorrected_err_count;

		/** FECS falcon i-mem, d-mem corrected error count. */
		struct nvgpu_ecc_stat *fecs_ecc_corrected_err_count;
		/** FECS falcon i-mem, d-mem uncorrected error count. */
		struct nvgpu_ecc_stat *fecs_ecc_uncorrected_err_count;
	} gr;

	/**
	 * Contains error statistics for each memory contained within the ltc
	 * unit.
	 */
	struct {
		/** ltc-lts sec count. */
		struct nvgpu_ecc_stat **ecc_sec_count;
		/** ltc-lts ded count. */
		struct nvgpu_ecc_stat **ecc_ded_count;
	} ltc;

	/**
	 * Contains error statistics for each memory contained within the fb
	 * unit.
	 */
	struct {
		/** hubmmu l2tlb corrected error count. */
		struct nvgpu_ecc_stat *mmu_l2tlb_ecc_corrected_err_count;
		/** hubmmu l2tlb uncorrected error count. */
		struct nvgpu_ecc_stat *mmu_l2tlb_ecc_uncorrected_err_count;
		/** hubmmu hubtlb corrected error count. */
		struct nvgpu_ecc_stat *mmu_hubtlb_ecc_corrected_err_count;
		/** hubmmu hubtlb uncorrected error count. */
		struct nvgpu_ecc_stat *mmu_hubtlb_ecc_uncorrected_err_count;
		/** hubmmu fillunit corrected error count. */
		struct nvgpu_ecc_stat *mmu_fillunit_ecc_corrected_err_count;
		/** hubmmu fillunit uncorrected error count. */
		struct nvgpu_ecc_stat *mmu_fillunit_ecc_uncorrected_err_count;
	} fb;

	/**
	 * Contains error statistics for each memory contained within the pmu
	 * unit.
	 */
	struct {
		/** PMU falcon imem, dmem corrected error count. */
		struct nvgpu_ecc_stat *pmu_ecc_corrected_err_count;
		/** PMU falcon imem, dmem uncorrected error count. */
		struct nvgpu_ecc_stat *pmu_ecc_uncorrected_err_count;
	} pmu;

	/**
	 * Contains error statistics for each memory contained within the fbpa
	 * unit.
	 */
	struct {
		/** fbpa sec count. */
		struct nvgpu_ecc_stat *fbpa_ecc_sec_err_count;
		/** fbpa ded count. */
		struct nvgpu_ecc_stat *fbpa_ecc_ded_err_count;
	} fbpa;

	/** Contains the head to the list of error statistics. */
	struct nvgpu_list_node stats_list;
	/** Contains the number of error statistics. */
	int stats_count;
	/** Flag stores the initialization status of ECC unit. */
	bool initialized;
};

/**
 * @brief Allocate and initialize error counter specified by name for all
 * gpc-tpc instances.
 *
 * @param g [in] The GPU driver struct.
 * @param stat [out] Pointer to array of pointers of error counters.
 * @param name [in] Unique name for error counter.
 *
 * Calculates the total number of tpcs across all gpcs within the gr unit.
 * Then allocates, initializes memory to hold error counters associated with all
 * tpcs, which is then added to the stats_list in struct nvgpu_ecc.
 *
 * @return 0 in case of success, less than 0 for failure.
 */
int nvgpu_ecc_counter_init_per_tpc(struct gk20a *g,
		struct nvgpu_ecc_stat ***stat, const char *name);
/*
 * @brief Allocate and initalize counter for memories common across a TPC.
 *
 * @param stat [in] Address of pointer to struct nvgpu_ecc_stat.
 *
 */
#define NVGPU_ECC_COUNTER_INIT_PER_TPC(stat)		\
	do {						\
		int err = 0;				\
		err = nvgpu_ecc_counter_init_per_tpc(g,	\
				&g->ecc.gr.stat, #stat);\
		if (err != 0) {				\
			return err;			\
		}					\
	} while (false)

/**
 * @brief Allocate and initialize error counter specified by name for all gpc
 * instances.
 *
 * @param g [in] The GPU driver struct.
 * @param stat [out] Pointer to array of tpc error counters.
 * @param name [in] Unique name for error counter.
 *
 * Calculates the total number of gpcs within the gr unit. Then allocates,
 * initializes memory to hold error counters associated with all gpcs, which is
 * then added to the stats_list in struct nvgpu_ecc.
 *
 * @return 0 in case of success, less than 0 for failure.
 */
int nvgpu_ecc_counter_init_per_gpc(struct gk20a *g,
		struct nvgpu_ecc_stat **stat, const char *name);
/*
 * @brief Allocate and initalize counters for memories shared across a GPC.
 *
 * @param stat [in] Address of pointer to struct nvgpu_ecc_stat.
 *
 */
#define NVGPU_ECC_COUNTER_INIT_PER_GPC(stat) \
	nvgpu_ecc_counter_init_per_gpc(g, &g->ecc.gr.stat, #stat)

/**
 * @brief Allocates, initializes an error counter with specified name.
 *
 * @param g [in] The GPU driver struct.
 * @param stat [out] Pointer to array of tpc error counters.
 * @param name [in] Unique name for error counter.
 *
 * Allocate memory for one error counter, initializes the counter with 0 and the
 * specified string identifier. Finally the counter is added to the status_list
 * of struct nvgpu_ecc.
 *
 * @return 0 in case of success, less than 0 for failure.
 */
int nvgpu_ecc_counter_init(struct gk20a *g,
		struct nvgpu_ecc_stat **stat, const char *name);
/*
 * @brief Allocate and initalize counters for memories shared within GR.
 *
 * @param stat [in] Address of pointer to struct nvgpu_ecc_stat.
 *
 */
#define NVGPU_ECC_COUNTER_INIT_GR(stat) \
	nvgpu_ecc_counter_init(g, &g->ecc.gr.stat, #stat)
/*
 * @brief Allocate and initalize counters for memories within FB.
 *
 * @param stat [in] Address of pointer to struct nvgpu_ecc_stat.
 *
 */
#define NVGPU_ECC_COUNTER_INIT_FB(stat) \
	nvgpu_ecc_counter_init(g, &g->ecc.fb.stat, #stat)
/*
 * @brief Allocate and initalize counter for memories within PMU.
 *
 * @param stat [in] Address of pointer to struct nvgpu_ecc_stat.
 *
 */
#define NVGPU_ECC_COUNTER_INIT_PMU(stat) \
	nvgpu_ecc_counter_init(g, &g->ecc.pmu.stat, #stat)

/**
 * @brief Allocate and initialize a error counters for all ltc-lts instances.
 *
 * @param g [in] The GPU driver struct.
 * @param stat [out] Pointer to array of tpc error counters.
 * @param name [in] Unique name for error counter.
 *
 * Calculates the total number of ltc-lts instances, allocates memory for each
 * instance of error counter, initializes the counter with 0 and the specified
 * string identifier. Finally the counter is added to the stats_list of
 * struct nvgpu_ecc.
 *
 * @return 0 in case of success, less than 0 for failure.
 */
int nvgpu_ecc_counter_init_per_lts(struct gk20a *g,
		struct nvgpu_ecc_stat ***stat, const char *name);
/*
 * @brief Allocate and initalize counters for memories within ltc-lts
 *
 * @param stat [in] Address of pointer to struct nvgpu_ecc_stat.
 *
 */
#define NVGPU_ECC_COUNTER_INIT_PER_LTS(stat) \
	nvgpu_ecc_counter_init_per_lts(g, &g->ecc.ltc.stat, #stat)

/**
 * @brief Allocate and initialize error counters for all fbpa instances.
 *
 * @param g [in] The GPU driver struct.
 * @param stat [out] Pointer to array of tpc error counters.
 * @param name [in] Unique name for error counter.
 *
 * Calculates the total number of fbpa instances, allocates memory for each
 * instance of error counter, initializes the counter with 0 and the specified
 * string identifier. Finally the counter is added to the stats_list of
 * struct nvgpu_ecc.
 *
 * @return 0 in case of success, less than 0 for failure.
 */
int nvgpu_ecc_counter_init_per_fbpa(struct gk20a *g,
		struct nvgpu_ecc_stat **stat, const char *name);
#define NVGPU_ECC_COUNTER_INIT_PER_FBPA(stat) \
	nvgpu_ecc_counter_init_per_fbpa(g, &g->ecc.fbpa.stat, #stat)

/**
 * @brief Release memory associated with all error counters.
 *
 * @param g [in] The GPU driver struct.
 *
 * Releases memory associated with all error counters associated with a hardware
 * unit, this is done for every instance of the hardware unit.
 */
void nvgpu_ecc_free(struct gk20a *g);

/**
 * @brief Allocates and initializes error counters for memories within gpu
 * hardware units.
 *
 * @param g [in] The GPU driver struct.
 *
 * @return 0 in case of success, less than 0 for failure.
 */
int nvgpu_ecc_init_support(struct gk20a *g);

/**
 * @brief Destroys, frees up memory allocated to ecc/parity error counters.
 *
 * @param g [in] The GPU driver struct.
 */
void nvgpu_ecc_remove_support(struct gk20a *g);

#ifdef CONFIG_NVGPU_SYSFS
int nvgpu_ecc_sysfs_init(struct gk20a *g);
void nvgpu_ecc_sysfs_remove(struct gk20a *g);
#endif

#endif
