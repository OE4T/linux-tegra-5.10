/*
 * Copyright (c) 2017-2020, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_NVGPU_ERR_H
#define NVGPU_NVGPU_ERR_H

/**
 * @file
 *
 * Define indices for HW units and errors. Define structures used to carry error
 * information. Declare prototype for APIs that are used to report GPU HW errors
 * to the 3LSS framework.
 */

#include <nvgpu/types.h>

struct gk20a;
struct mmu_fault_info;

/**
 * This assigns an unique index for hw units in GPU.
 */
#define NVGPU_ERR_MODULE_HOST		(0U)
#define NVGPU_ERR_MODULE_SM		(1U)
#define NVGPU_ERR_MODULE_FECS		(2U)
#define NVGPU_ERR_MODULE_GPCCS		(3U)
#define NVGPU_ERR_MODULE_MMU		(4U)
#define NVGPU_ERR_MODULE_GCC		(5U)
#define NVGPU_ERR_MODULE_PMU		(6U)
#define NVGPU_ERR_MODULE_PGRAPH		(7U)
#define NVGPU_ERR_MODULE_LTC		(8U)
#define NVGPU_ERR_MODULE_HUBMMU		(9U)
#define NVGPU_ERR_MODULE_PRI		(10U)
#define NVGPU_ERR_MODULE_CE		(11U)

/**
 * This assigns an unique index for errors in HOST unit.
 */
#define GPU_HOST_PFIFO_BIND_ERROR		(0U)
#define GPU_HOST_PFIFO_SCHED_ERROR		(1U)
#define GPU_HOST_PFIFO_CHSW_ERROR		(2U)
#define GPU_HOST_PFIFO_MEMOP_TIMEOUT_ERROR	(3U)
#define GPU_HOST_PFIFO_LB_ERROR			(4U)
#define GPU_HOST_PBUS_SQUASH_ERROR		(5U)
#define GPU_HOST_PBUS_FECS_ERROR		(6U)
#define GPU_HOST_PBUS_TIMEOUT_ERROR		(7U)
#define GPU_HOST_PBDMA_TIMEOUT_ERROR		(8U)
#define GPU_HOST_PBDMA_EXTRA_ERROR		(9U)
#define GPU_HOST_PBDMA_GPFIFO_PB_ERROR		(10U)
#define GPU_HOST_PBDMA_METHOD_ERROR		(11U)
#define GPU_HOST_PBDMA_SIGNATURE_ERROR		(12U)
#define GPU_HOST_PBDMA_HCE_ERROR		(13U)
#define GPU_HOST_PBDMA_PREEMPT_ERROR		(14U)
#define GPU_HOST_PFIFO_CTXSW_TIMEOUT_ERROR	(15U)
#define GPU_HOST_PFIFO_FB_FLUSH_TIMEOUT_ERROR	(16U)
#define GPU_HOST_INVALID_ERROR			(17U)

/**
 * This assigns an unique index for errors in SM unit.
 */
#define GPU_SM_L1_TAG_ECC_CORRECTED			(0U)
#define GPU_SM_L1_TAG_ECC_UNCORRECTED			(1U)
#define GPU_SM_CBU_ECC_CORRECTED			(2U)
#define GPU_SM_CBU_ECC_UNCORRECTED			(3U)
#define GPU_SM_LRF_ECC_CORRECTED			(4U)
#define GPU_SM_LRF_ECC_UNCORRECTED			(5U)
#define GPU_SM_L1_DATA_ECC_CORRECTED			(6U)
#define GPU_SM_L1_DATA_ECC_UNCORRECTED			(7U)
#define GPU_SM_ICACHE_L0_DATA_ECC_CORRECTED		(8U)
#define GPU_SM_ICACHE_L0_DATA_ECC_UNCORRECTED		(9U)
#define GPU_SM_ICACHE_L1_DATA_ECC_CORRECTED		(10U)
#define GPU_SM_ICACHE_L1_DATA_ECC_UNCORRECTED		(11U)
#define GPU_SM_ICACHE_L0_PREDECODE_ECC_CORRECTED	(12U)
#define GPU_SM_ICACHE_L0_PREDECODE_ECC_UNCORRECTED	(13U)
#define GPU_SM_L1_TAG_MISS_FIFO_ECC_CORRECTED		(14U)
#define GPU_SM_L1_TAG_MISS_FIFO_ECC_UNCORRECTED		(15U)
#define GPU_SM_L1_TAG_S2R_PIXPRF_ECC_CORRECTED		(16U)
#define GPU_SM_L1_TAG_S2R_PIXPRF_ECC_UNCORRECTED	(17U)
#define GPU_SM_MACHINE_CHECK_ERROR			(18U)
#define GPU_SM_ICACHE_L1_PREDECODE_ECC_CORRECTED	(19U)
#define GPU_SM_ICACHE_L1_PREDECODE_ECC_UNCORRECTED	(20U)

/**
 * This structure is used to store SM machine check related information.
 */
struct gr_sm_mcerr_info {
	/** PC which triggered the machine check error. */
	u64 hww_warp_esr_pc;

	/** Error status register. */
	u32 hww_warp_esr_status;

	/** Context which triggered error. */
	u32 curr_ctx;

	/** Channel to which the context belongs. */
	u32 chid;

	/** TSG to which the channel is bound. */
	u32 tsgid;

	u32 tpc, gpc, sm;
};

/**
 * This assigns an unique index for errors in FECS unit.
 */
#define GPU_FECS_FALCON_IMEM_ECC_CORRECTED	(0U)
#define GPU_FECS_FALCON_IMEM_ECC_UNCORRECTED	(1U)
#define GPU_FECS_FALCON_DMEM_ECC_CORRECTED	(2U)
#define GPU_FECS_FALCON_DMEM_ECC_UNCORRECTED	(3U)
#define GPU_FECS_CTXSW_WATCHDOG_TIMEOUT		(4U)
#define GPU_FECS_CTXSW_CRC_MISMATCH		(5U)
#define GPU_FECS_FAULT_DURING_CTXSW		(6U)
#define GPU_FECS_CTXSW_INIT_ERROR		(7U)
#define GPU_FECS_INVALID_ERROR			(8U)

/**
 * This structure is used to store CTXSW error related information.
 */
struct ctxsw_err_info {
	u32 curr_ctx;
	u32 ctxsw_status0;
	u32 ctxsw_status1;
	u32 chid;
	u32 mailbox_value;
};

/**
 * This assigns an unique index for errors in GPCCS unit.
 */
#define GPU_GPCCS_FALCON_IMEM_ECC_CORRECTED	(0U)
#define GPU_GPCCS_FALCON_IMEM_ECC_UNCORRECTED	(1U)
#define GPU_GPCCS_FALCON_DMEM_ECC_CORRECTED	(2U)
#define GPU_GPCCS_FALCON_DMEM_ECC_UNCORRECTED	(3U)

/**
 * This assigns an unique index for errors in MMU unit.
 */
#define GPU_MMU_L1TLB_SA_DATA_ECC_CORRECTED	(0U)
#define GPU_MMU_L1TLB_SA_DATA_ECC_UNCORRECTED	(1U)
#define GPU_MMU_L1TLB_FA_DATA_ECC_CORRECTED	(2U)
#define GPU_MMU_L1TLB_FA_DATA_ECC_UNCORRECTED	(3U)

/**
 * This assigns an unique index for errors in GCC unit.
 */
#define GPU_GCC_L15_ECC_CORRECTED		(0U)
#define GPU_GCC_L15_ECC_UNCORRECTED		(1U)

/**
 * This assigns an unique index for errors in PMU unit.
 */
#define GPU_PMU_FALCON_IMEM_ECC_CORRECTED	(0U)
#define GPU_PMU_FALCON_IMEM_ECC_UNCORRECTED	(1U)
#define GPU_PMU_FALCON_DMEM_ECC_CORRECTED	(2U)
#define GPU_PMU_FALCON_DMEM_ECC_UNCORRECTED	(3U)
#define GPU_PMU_BAR0_ERROR_TIMEOUT		(4U)

/**
 * This assigns an unique index for errors in PGRAPH unit.
 */
#define GPU_PGRAPH_FE_EXCEPTION			(0U)
#define GPU_PGRAPH_MEMFMT_EXCEPTION		(1U)
#define GPU_PGRAPH_PD_EXCEPTION			(2U)
#define GPU_PGRAPH_SCC_EXCEPTION		(3U)
#define GPU_PGRAPH_DS_EXCEPTION			(4U)
#define GPU_PGRAPH_SSYNC_EXCEPTION		(5U)
#define GPU_PGRAPH_MME_EXCEPTION		(6U)
#define GPU_PGRAPH_SKED_EXCEPTION		(7U)
#define GPU_PGRAPH_BE_EXCEPTION			(8U)
#define GPU_PGRAPH_MPC_EXCEPTION		(9U)
#define GPU_PGRAPH_ILLEGAL_ERROR		(10U)
#define GPU_PGRAPH_GPC_GFX_EXCEPTION		(11U)
#define GPU_PGRAPH_MME_FE1_EXCEPTION		(12U)

/** Sub-errors in GPU_PGRAPH_BE_EXCEPTION. */
#define GPU_PGRAPH_BE_EXCEPTION_CROP		(0U)
#define GPU_PGRAPH_BE_EXCEPTION_ZROP		(1U)

/** Sub-errors in GPU_PGRAPH_GPC_GFX_EXCEPTION. */
#define GPU_PGRAPH_GPC_GFX_EXCEPTION_PROP	(0U)
#define GPU_PGRAPH_GPC_GFX_EXCEPTION_ZCULL	(1U)
#define GPU_PGRAPH_GPC_GFX_EXCEPTION_SETUP	(2U)
#define GPU_PGRAPH_GPC_GFX_EXCEPTION_PES0	(3U)
#define GPU_PGRAPH_GPC_GFX_EXCEPTION_PES1	(4U)
#define GPU_PGRAPH_GPC_GFX_EXCEPTION_TPC_PE	(5U)

/** Sub-errors in GPU_PGRAPH_ILLEGAL_ERROR. */
#define GPU_PGRAPH_ILLEGAL_NOTIFY		(0U)
#define GPU_PGRAPH_ILLEGAL_METHOD		(1U)
#define GPU_PGRAPH_ILLEGAL_CLASS		(2U)
#define GPU_PGRAPH_CLASS_ERROR			(3U)

/**
 * This structure is used to store GR exception related information.
 */
struct gr_exception_info {
	/** Context which triggered the exception. */
	u32 curr_ctx;

	/** Channel bound to the context. */
	u32 chid;

	/** TSG to which the channel is bound. */
	u32 tsgid;

	u32 status;
};

/**
 * This assigns an unique index for errors in LTC unit.
 */
#define GPU_LTC_CACHE_DSTG_ECC_CORRECTED	(0U)
#define GPU_LTC_CACHE_DSTG_ECC_UNCORRECTED	(1U)
#define GPU_LTC_CACHE_TSTG_ECC_CORRECTED	(2U)
#define GPU_LTC_CACHE_TSTG_ECC_UNCORRECTED	(3U)
#define GPU_LTC_CACHE_RSTG_ECC_CORRECTED	(4U)
#define GPU_LTC_CACHE_RSTG_ECC_UNCORRECTED	(5U)
#define GPU_LTC_CACHE_DSTG_BE_ECC_CORRECTED	(6U)
#define GPU_LTC_CACHE_DSTG_BE_ECC_UNCORRECTED	(7U)

/**
 * This assigns an unique index for errors in HUBMMU unit.
 */
#define GPU_HUBMMU_L2TLB_SA_DATA_ECC_CORRECTED		(0U)
#define GPU_HUBMMU_L2TLB_SA_DATA_ECC_UNCORRECTED	(1U)
#define GPU_HUBMMU_TLB_SA_DATA_ECC_CORRECTED		(2U)
#define GPU_HUBMMU_TLB_SA_DATA_ECC_UNCORRECTED		(3U)
#define GPU_HUBMMU_PTE_DATA_ECC_CORRECTED		(4U)
#define GPU_HUBMMU_PTE_DATA_ECC_UNCORRECTED		(5U)
#define GPU_HUBMMU_PDE0_DATA_ECC_CORRECTED		(6U)
#define GPU_HUBMMU_PDE0_DATA_ECC_UNCORRECTED		(7U)
#define GPU_HUBMMU_PAGE_FAULT_ERROR			(8U)

/** Sub-errors in GPU_HUBMMU_PAGE_FAULT_ERROR. */
#define GPU_HUBMMU_REPLAYABLE_FAULT_OVERFLOW		(0U)
#define GPU_HUBMMU_REPLAYABLE_FAULT_NOTIFY		(1U)
#define GPU_HUBMMU_NONREPLAYABLE_FAULT_OVERFLOW		(2U)
#define GPU_HUBMMU_NONREPLAYABLE_FAULT_NOTIFY		(3U)
#define GPU_HUBMMU_OTHER_FAULT_NOTIFY			(4U)

/**
 * This assigns an unique index for errors in PRI unit.
 */
#define GPU_PRI_TIMEOUT_ERROR		(0U)
#define GPU_PRI_ACCESS_VIOLATION	(1U)

/**
 * This assigns an unique index for errors in CE unit.
 */
#define GPU_CE_LAUNCH_ERROR			(0U)
#define GPU_CE_BLOCK_PIPE			(1U)
#define GPU_CE_NONBLOCK_PIPE			(2U)
#define GPU_CE_INVALID_CONFIG			(3U)
#define GPU_CE_METHOD_BUFFER_FAULT		(4U)

/**
 * This structure is used to store GR error related information.
 */
struct gr_err_info {
	struct gr_sm_mcerr_info *sm_mcerr_info;
	struct gr_exception_info *exception_info;
};

/**
 * This macro is used to initialize the members of nvgpu_hw_err_inject_info
 * struct.
 */
#define NVGPU_ECC_ERR(err_name, inject_fn, addr, val)	\
{							\
		.name = (err_name),			\
		.inject_hw_fault = (inject_fn),		\
		.get_reg_addr = (addr),			\
		.get_reg_val = (val)			\
}

/**
 * This structure carries the information required for HW based error injection
 * for a given error.
 */
struct nvgpu_hw_err_inject_info {
	/** String representation of error. */
	const char *name;
	void (*inject_hw_fault)(struct gk20a *g,
			struct nvgpu_hw_err_inject_info *err, u32 err_info);
	u32 (*get_reg_addr)(void);
	u32 (*get_reg_val)(u32 val);
};

/**
 * This structure contains a pointer to an array containing HW based error
 * injection information and the size of that array.
 */
struct nvgpu_hw_err_inject_info_desc {
	struct nvgpu_hw_err_inject_info *info_ptr;
	u32 info_size;
};

/**
 * @brief Report HOST (PFIFO/PBDMA/PBUS) related errors to 3LSS.
 *
 * @param g [in]		- The GPU driver struct.
 * @param hw_unit [in]		- Index of HW unit (HOST).
 *				  - NVGPU_ERR_MODULE_HOST
 * @param inst [in]		- Instance ID.
 *				  - In case of multiple instances of the same HW
 *				    unit (e.g., there are multiple instances of
 *				    PBDMA), it is used to identify the instance
 *				    that encountered a fault.
 * @param err_id [in]		- Error index.
 *				  - Min: GPU_HOST_PFIFO_BIND_ERROR
 *				  - Max: GPU_HOST_PFIFO_FB_FLUSH_TIMEOUT_ERROR
 * @param intr_info [in]	- Content of interrupt status register.
 *
 * - Checks whether SDL is supported in the current GPU platform. If SDL is not
 *   supported, it simply returns.
 * - Validates both \a hw_unit and \a err_id indices. In case of a failure,
 *   invokes #nvgpu_sdl_handle_report_failure() api.
 * - Gets the current time of a clock. In case of a failure, invokes
 *   #nvgpu_sdl_handle_report_failure() api.
 * - Gets error description from internal look-up table using \a hw_unit and
 *   \a err_id indices.
 * - Forms error packet using details such as time-stamp, \a hw_unit, \a err_id,
 *   criticality of the error, \a inst, error description, \a intr_info and
 *   size of the error packet.
 * - Performs compile-time assert check to ensure that the size of the error
 *   packet does not exceed the maximum allowable size specified in
 *   #MAX_ERR_MSG_SIZE.
 * - Sends the error packet to report the error to the 3LSS framework. In case
 *   of a failure, invokes #nvgpu_sdl_handle_report_failure() api.
 *
 * @return	None
 */
void nvgpu_report_host_err(struct gk20a *g, u32 hw_unit,
	u32 inst, u32 err_id, u32 intr_info);

/**
 * @brief Report error in CE unit to 3LSS.
 *
 * @param g [in]		- The GPU driver struct.
 * @param hw_unit [in]		- Index of HW unit (CE).
 *				  - NVGPU_ERR_MODULE_CE
 * @param inst [in]		- Instance ID.
 *				  - In case of multiple instances of the same HW
 *				    unit (e.g., there are multiple instances of
 *				    CE), it is used to identify the instance
 *				    that encountered a fault.
 * @param err_id [in]		- Error index.
 *				  - Min: GPU_CE_LAUNCH_ERROR
 *				  - Max: GPU_CE_METHOD_BUFFER_FAULT
 * @param intr_info [in]	- Content of interrupt status register.
 *
 * - Checks whether SDL is supported in the current GPU platform. If SDL is not
 *   supported, it simply returns.
 * - Validates both \a hw_unit and \a err_id indices. In case of a failure,
 *   invokes #nvgpu_sdl_handle_report_failure() api.
 * - Gets the current time of a clock. In case of a failure, invokes
 *   #nvgpu_sdl_handle_report_failure() api.
 * - Gets error description from internal look-up table using \a hw_unit and
 *   \a err_id indices.
 * - Forms error packet using details such as time-stamp, \a hw_unit, \a err_id,
 *   criticality of the error, \a inst, error description, \a intr_info and
 *   size of the error packet.
 * - Performs compile-time assert check to ensure that the size of the error
 *   packet does not exceed the maximum allowable size specified in
 *   #MAX_ERR_MSG_SIZE.
 * - Sends the error packet to report the error to the 3LSS framework. In case
 *   of a failure, invokes #nvgpu_sdl_handle_report_failure() api.
 *
 * @return	None
 */
void nvgpu_report_ce_err(struct gk20a *g, u32 hw_unit,
	u32 inst, u32 err_id, u32 intr_info);

/**
 * @brief Report ECC related errors to 3LSS.
 *
 * @param g [in]		- The GPU driver struct.
 * @param hw_unit [in]		- Index of HW unit.
 *				  - List of valid HW unit IDs
 *				    - NVGPU_ERR_MODULE_SM
 *				    - NVGPU_ERR_MODULE_FECS
 *				    - NVGPU_ERR_MODULE_GPCCS
 *				    - NVGPU_ERR_MODULE_MMU
 *				    - NVGPU_ERR_MODULE_GCC
 *				    - NVGPU_ERR_MODULE_PMU
 *				    - NVGPU_ERR_MODULE_LTC
 *				    - NVGPU_ERR_MODULE_HUBMMU
 * @param inst [in]		- Instance ID.
 *				  - In case of multiple instances of the same HW
 *				    unit (e.g., there are multiple instances of
 *				    SM), it is used to identify the instance
 *				    that encountered a fault.
 * @param err_id [in]		- Error index.
 *				  - For SM:
 *				    - Min: GPU_SM_L1_TAG_ECC_CORRECTED
 *				    - Max: GPU_SM_ICACHE_L1_PREDECODE_ECC_UNCORRECTED
 *				  - For FECS:
 *				    - Min: GPU_FECS_FALCON_IMEM_ECC_CORRECTED
 *				    - Max: GPU_FECS_INVALID_ERROR
 *				  - For GPCCS:
 *				    - Min: GPU_GPCCS_FALCON_IMEM_ECC_CORRECTED
 *				    - Max: GPU_GPCCS_FALCON_DMEM_ECC_UNCORRECTED
 *				  - For MMU:
 *				    - Min: GPU_MMU_L1TLB_SA_DATA_ECC_CORRECTED
 *				    - Max: GPU_MMU_L1TLB_FA_DATA_ECC_UNCORRECTED
 *				  - For GCC:
 *				    - Min: GPU_GCC_L15_ECC_CORRECTED
 *				    - Max: GPU_GCC_L15_ECC_UNCORRECTED
 *				  - For PMU:
 *				    - Min: GPU_PMU_FALCON_IMEM_ECC_CORRECTED
 *				    - Max: GPU_PMU_FALCON_DMEM_ECC_UNCORRECTED
 *				  - For LTC:
 *				    - Min: GPU_LTC_CACHE_DSTG_ECC_CORRECTED
 *				    - Max: GPU_LTC_CACHE_DSTG_BE_ECC_UNCORRECTED
 *				  - For HUBMMU:
 *				    - Min: GPU_HUBMMU_L2TLB_SA_DATA_ECC_CORRECTED
 *				    - Max: GPU_HUBMMU_PDE0_DATA_ECC_UNCORRECTED
 * @param err_addr [in]		- Error address.
 *				  - This is the location at which correctable or
 *				    uncorrectable error has occurred.
 * @param err_count [in]	- Error count.
 *
 * - Checks whether SDL is supported in the current GPU platform. If SDL is not
 *   supported, it simply returns.
 * - Validates both \a hw_unit and \a err_id indices. In case of a failure,
 *   invokes #nvgpu_sdl_handle_report_failure() api.
 * - Gets the current time of a clock. In case of a failure, invokes
 *   #nvgpu_sdl_handle_report_failure() api.
 * - Gets error description from internal look-up table using \a hw_unit and
 *   \a err_id indices.
 * - Forms error packet using details such as time-stamp, \a hw_unit, \a err_id,
 *   criticality of the error, \a inst, \a err_addr, \a err_count, error
 *   description, and size of the error packet.
 * - Performs compile-time assert check to ensure that the size of the error
 *   packet does not exceed the maximum allowable size specified in
 *   #MAX_ERR_MSG_SIZE.
 * - Sends the error packet to report the error to the 3LSS framework. In case
 *   of a failure, invokes #nvgpu_sdl_handle_report_failure() api.
 *
 * @return	None
 */
void nvgpu_report_ecc_err(struct gk20a *g, u32 hw_unit, u32 inst,
		u32 err_id, u64 err_addr, u64 err_count);

/**
 * @brief Helper function to report FB MMU ECC errors to 3LSS.
 *
 * @param g [in]		- The GPU driver struct.
 * @param err_id [in]		- Error index.
 *				  - Min: GPU_HUBMMU_L2TLB_SA_DATA_ECC_CORRECTED
 *				  - Max: GPU_HUBMMU_PDE0_DATA_ECC_UNCORRECTED
 * @param err_addr [in]		- Error address.
 *				  - This is the location at which correctable or
 *				    uncorrectable error has occurred.
 * @param err_count [in]	- Error count.
 *
 * Calls nvgpu_report_ecc_err with hw_unit=NVGPU_ERR_MODULE_HUBMMU and inst=0.
 *
 * @return	None
 */
static inline void nvgpu_report_fb_ecc_err(struct gk20a *g, u32 err_id, u64 err_addr,
		u64 err_count)
{
	nvgpu_report_ecc_err(g, NVGPU_ERR_MODULE_HUBMMU, 0, err_id, err_addr,
			err_count);
}

/**
 * @brief Report CTXSW error to 3LSS.
 *
 * @param g [in]	- The GPU driver struct.
 * @param hw_unit [in]	- Index of HW unit (FECS).
 *			  - NVGPU_ERR_MODULE_FECS
 * @param err_id [in]	- Error index.
 *			  - Min: GPU_FECS_CTXSW_WATCHDOG_TIMEOUT
 *			  - Max: GPU_FECS_CTXSW_INIT_ERROR
 * @param data [in]	- CTXSW error information.
 *			  - This can be type-casted to struct ctxsw_err_info.
 *
 * - Checks whether SDL is supported in the current GPU platform. If SDL is not
 *   supported, it simply returns.
 * - Validates both \a hw_unit and \a err_id indices. In case of a failure,
 *   invokes #nvgpu_sdl_handle_report_failure() api.
 * - Gets the current time of a clock. In case of a failure, invokes
 *   #nvgpu_sdl_handle_report_failure() api.
 * - Gets error description from internal look-up table using \a hw_unit and
 *   \a err_id indices.
 * - Forms error packet using details such as time-stamp, \a hw_unit, \a err_id,
 *   criticality of the error, CTXSW error information, error description, and
 *   size of the error packet.
 * - Performs compile-time assert check to ensure that the size of the error
 *   packet does not exceed the maximum allowable size specified in
 *   #MAX_ERR_MSG_SIZE.
 * - Sends the error packet to report the error to the 3LSS framework. In case
 *   of a failure, invokes #nvgpu_sdl_handle_report_failure() api.
 *
 * @return	None
 */
void nvgpu_report_ctxsw_err(struct gk20a *g, u32 hw_unit, u32 err_id,
		void *data);

/**
 * @brief Report SM and PGRAPH related errors to 3LSS.
 *
 * @param g [in]		- The GPU driver struct.
 * @param hw_unit [in]		- Index of HW unit.
 *				  - List of valid HW unit IDs
 *				    - NVGPU_ERR_MODULE_SM
 *				    - NVGPU_ERR_MODULE_PGRAPH
 * @param inst [in]		- Instance ID.
 *				  - In case of multiple instances of the same HW
 *				    unit (e.g., there are multiple instances of
 *				    SM), it is used to identify the instance
 *				    that encountered a fault.
 * @param err_id [in]		- Error index.
 *				  - For SM:
 *				    - GPU_SM_MACHINE_CHECK_ERROR
 *				  - For PGRAPH:
 *				    - Min: GPU_PGRAPH_FE_EXCEPTION
 *				    - Max: GPU_PGRAPH_GPC_GFX_EXCEPTION
 * @param err_info [in]		- Error information.
 *				  - For SM: Machine Check Error Information.
 *				  - For PGRAPH: Exception Information.
 * @param sub_err_type [in]	- Sub error type.
 *				  - This is a sub-error of the error that is
 *				    being reported.
 *
 * - Checks whether SDL is supported in the current GPU platform. If SDL is not
 *   supported, it simply returns.
 * - Validates both \a hw_unit and \a err_id indices. In case of a failure,
 *   invokes #nvgpu_sdl_handle_report_failure() api.
 * - Gets the current time of a clock. In case of a failure, invokes
 *   #nvgpu_sdl_handle_report_failure() api.
 * - Gets error description from internal look-up table using \a hw_unit and
 *   \a err_id indices.
 * - Forms error packet using details such as time-stamp, \a hw_unit, \a err_id,
 *   criticality of the error, status, \a sub_err_type, error description, and
 *   size of the error packet. It also includes appropriate \a err_info
 *   depending on \a hw_unit type (SM/PGRAPH).
 * - Performs compile-time assert check to ensure that the size of the error
 *   packet does not exceed the maximum allowable size specified in
 *   #MAX_ERR_MSG_SIZE.
 * - Sends the error packet to report the error to the 3LSS framework. In case
 *   of a failure, invokes #nvgpu_sdl_handle_report_failure() api.
 *
 * @return	None
 */
void nvgpu_report_gr_err(struct gk20a *g, u32 hw_unit, u32 inst,
		u32 err_id, struct gr_err_info *err_info, u32 sub_err_type);

/**
 * @brief Report PMU related errors to 3LSS.
 *
 * @param g [in]		- The GPU driver struct.
 * @param hw_unit [in]		- Index of HW unit (PMU).
 *				  - NVGPU_ERR_MODULE_PMU
 * @param err_id [in]		- Error index.
 *				  - GPU_PMU_BAR0_ERROR_TIMEOUT
 * @param sub_err_type [in]	- Sub error type.
 *				  - This is a sub-error of the error that is
 *				    being reported.
 * @param status [in]		- Error information.
 *
 * - Checks whether SDL is supported in the current GPU platform. If SDL is not
 *   supported, it simply returns.
 * - Validates both \a hw_unit and \a err_id indices. In case of a failure,
 *   invokes #nvgpu_sdl_handle_report_failure() api.
 * - Gets the current time of a clock. In case of a failure, invokes
 *   #nvgpu_sdl_handle_report_failure() api.
 * - Gets error description from internal look-up table using \a hw_unit and
 *   \a err_id indices.
 * - Forms error packet using details such as time-stamp, \a hw_unit, \a err_id,
 *   criticality of the error, \a status, \a sub_err_type, error description,
 *   and size of the error packet.
 * - Performs compile-time assert check to ensure that the size of the error
 *   packet does not exceed the maximum allowable size specified in
 *   #MAX_ERR_MSG_SIZE.
 * - Sends the error packet to report the error to the 3LSS framework. In case
 *   of a failure, invokes #nvgpu_sdl_handle_report_failure() api.
 *
 * @return	None
 */
void nvgpu_report_pmu_err(struct gk20a *g, u32 hw_unit, u32 err_id,
	u32 sub_err_type, u32 status);

/**
 * @brief Report PRI related errors to 3LSS.
 *
 * @param g [in]		- The GPU driver struct.
 * @param hw_unit [in]		- Index of HW unit (PRI).
 *				  - NVGPU_ERR_MODULE_PRI
 * @param inst [in]		- Instance ID.
 *				  - In case of multiple instances of the same HW
 *				    unit, it is used to identify the instance
 *				    that encountered a fault.
 * @param err_id [in]		- Error index.
 *				  - Min: GPU_PRI_TIMEOUT_ERROR
 *				  - Max: GPU_PRI_ACCESS_VIOLATION
 * @param err_addr [in]		- Error address.
 *				  - This is the address of the first PRI access
 *				    that resulted in an error.
 * @param err_code [in]		- Error code.
 *				  - This is an unique code associated with the
 *				    error that is being reported.
 *
 * - Checks whether SDL is supported in the current GPU platform. If SDL is not
 *   supported, it simply returns.
 * - Validates both \a hw_unit and \a err_id indices. In case of a failure,
 *   invokes #nvgpu_sdl_handle_report_failure() api.
 * - Gets the current time of a clock. In case of a failure, invokes
 *   #nvgpu_sdl_handle_report_failure() api.
 * - Gets error description from internal look-up table using \a hw_unit and
 *   \a err_id indices.
 * - Forms error packet using details such as time-stamp, \a hw_unit, \a err_id,
 *   criticality of the error, \a inst, \a err_addr, error description,
 *   \a err_code, and size of the error packet.
 * - Performs compile-time assert check to ensure that the size of the error
 *   packet does not exceed the maximum allowable size specified in
 *   #MAX_ERR_MSG_SIZE.
 * - Sends the error packet to report the error to the 3LSS framework. In case
 *   of a failure, invokes #nvgpu_sdl_handle_report_failure() api.
 *
 * @return	None
 */
void nvgpu_report_pri_err(struct gk20a *g, u32 hw_unit, u32 inst,
		u32 err_id, u32 err_addr, u32 err_code);

/**
 * @brief Report MMU page fault error to 3LSS.
 *
 * @param g [in]		- The GPU driver struct.
 * @param hw_unit [in]		- Index of HW unit (HUBMMU).
 *				  - NVGPU_ERR_MODULE_HUBMMU
 * @param err_id [in]		- Error index.
 *				  - GPU_HUBMMU_PAGE_FAULT_ERROR
 * @param fault_info [in]	- MMU page fault information.
 * @param status [in]		- Error information.
 * @param sub_err_type [in]	- Sub error type.
 *				  - This is a sub-error of the error that is
 *				    being reported.
 *
 * - Checks whether SDL is supported in the current GPU platform. If SDL is not
 *   supported, it simply returns.
 * - Validates both \a hw_unit and \a err_id indices. In case of a failure,
 *   invokes #nvgpu_sdl_handle_report_failure() api.
 * - Gets the current time of a clock. In case of a failure, invokes
 *   #nvgpu_sdl_handle_report_failure() api.
 * - Gets error description from internal look-up table using \a hw_unit and
 *   \a err_id indices.
 * - Forms error packet using details such as time-stamp, \a hw_unit, \a err_id,
 *   criticality of the error, \a sub_err_type, error description, and size of
 *   the error packet. It also includes page fault information, if it is
 *   available.
 * - Performs compile-time assert check to ensure that the size of the error
 *   packet does not exceed the maximum allowable size specified in
 *   #MAX_ERR_MSG_SIZE.
 * - Sends the error packet to report the error to the 3LSS framework. In case
 *   of a failure, invokes #nvgpu_sdl_handle_report_failure() api.
 *
 * @return	None
 */
void nvgpu_report_mmu_err(struct gk20a *g, u32 hw_unit,
		u32 err_id, struct mmu_fault_info *fault_info,
		u32 status, u32 sub_err_type);

/**
 * @brief Wrapper function to report ctxsw error.
 *
 * @param g [in]		- The GPU driver struct.
 * @param err_type [in]		- Error index.
 *				  - Min: GPU_FECS_CTXSW_WATCHDOG_TIMEOUT
 *				  - Max: GPU_FECS_CTXSW_INIT_ERROR
 * @param chid [in]		- Channel ID.
 * @param mailbox_value [in]	- Mailbox value.
 *
 * - Creates an instance of #ctxsw_err_info structure.
 * - Fills the details related to current context, ctxsw status, mailbox value,
 *   channel ID in #ctxsw_err_info strucutre.
 * - Invokes #nvgpu_report_ctxsw_err() and passes #ctxsw_err_info.
 *
 * @return	None
 */
void gr_intr_report_ctxsw_error(struct gk20a *g, u32 err_type, u32 chid,
		u32 mailbox_value);

#endif /* NVGPU_NVGPU_ERR_H */
