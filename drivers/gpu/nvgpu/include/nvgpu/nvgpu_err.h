/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
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
 * to 3LSS.
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
 * @brief Report error in HOST unit to 3LSS.
 *
 * @param g[in]		- The GPU driver struct.
 * @param hw_unit[in]	- Index of HW unit (HOST).
 * @param inst[in]	- Instance ID.
 * @param err_id[in]	- Error index.
 * @param intr_info[in]	- Content of interrupt status register.
 *
 * - In case of linux/posix implementation, it simply returns 0 since SDL is not
 *   supported for them.
 * - In case of nvgpu-qnx, it does the following:
 *   - Checks whether SDL is supported in the current GPU platform.
 *   - Validates the HW unit ID and error ID.
 *   - Forms error packet and checks whether it exceeds the max size.
 *   - Sends error packet to report error to 3LSS.
 *
 * @return None
 * @retval None
 */
void nvgpu_report_host_err(struct gk20a *g, u32 hw_unit,
	u32 inst, u32 err_id, u32 intr_info);

/**
 * @brief Report error in CE unit to 3LSS.
 *
 * @param g[in]		- The GPU driver struct.
 * @param hw_unit[in]	- Index of HW unit (CE).
 * @param inst[in]	- Instance ID.
 * @param err_id[in]	- Error index.
 * @param intr_info[in]	- Content of interrupt status register.
 *
 * - In case of linux/posix implementation, it simply returns 0 since SDL is not
 *   supported for them.
 * - In case of nvgpu-qnx, it does the following:
 *   - Checks whether SDL is supported in the current GPU platform.
 *   - Validates the HW unit ID and error ID.
 *   - Forms error packet and checks whether it exceeds the max size.
 *   - Sends error packet to report error to 3LSS.
 *
 * @return None
 * @retval None
 */
void nvgpu_report_ce_err(struct gk20a *g, u32 hw_unit,
	u32 inst, u32 err_id, u32 intr_info);

/**
 * @brief Report ECC error to 3LSS.
 *
 * @param g[in]		- The GPU driver struct.
 * @param hw_unit[in]	- Index of HW unit.
 * @param inst[in]	- Instance ID.
 * @param err_id[in]	- Error index.
 * @param err_addr[in]	- Error address.
 * @param err_count[in]	- Error count.
 *
 * - In case of linux/posix implementation, it simply returns 0 since SDL is not
 *   supported for them.
 * - In case of nvgpu-qnx, it does the following:
 *   - Checks whether SDL is supported in the current GPU platform.
 *   - Validates the HW unit ID and error ID.
 *   - Validates slice ID and TPC ID for LTC and SM units, respectively.
 *   - Forms error packet and checks whether it exceeds the max size.
 *   - Sends error packet to report error to 3LSS.
 *
 * @return None
 * @retval None
 */
void nvgpu_report_ecc_err(struct gk20a *g, u32 hw_unit, u32 inst,
		u32 err_id, u64 err_addr, u64 err_count);

/**
 * @brief Report CTXSW error to 3LSS.
 *
 * @param g[in]		- The GPU driver struct.
 * @param hw_unit[in]	- Index of HW unit (FECS).
 * @param err_id[in]	- Error index.
 * @param data[in]	- CTXSW error information.
 *
 * - In case of linux/posix implementation, it simply returns 0 since SDL is not
 *   supported for them.
 * - In case of nvgpu-qnx, it does the following:
 *   - Checks whether SDL is supported in the current GPU platform.
 *   - Validates the HW unit ID and error ID.
 *   - Forms error packet and checks whether it exceeds the max size.
 *   - Sends error packet to report error to 3LSS.
 *
 * @return None
 * @retval None
 */
void nvgpu_report_ctxsw_err(struct gk20a *g, u32 hw_unit, u32 err_id,
		void *data);

/**
 * @brief Report GR error to 3LSS.
 *
 * @param g[in]			- The GPU driver struct.
 * @param hw_unit[in]		- Index of HW unit.
 * @param inst[in]		- Instance ID.
 * @param err_id[in]		- Error index.
 * @param err_info[in]		- Error information.
 * @param sub_err_type[in]	- Sub error type.
 *
 * - In case of linux/posix implementation, it simply returns 0 since SDL is not
 *   supported for them.
 * - In case of nvgpu-qnx, it does the following:
 *   - Checks whether SDL is supported in the current GPU platform.
 *   - Validates the HW unit ID and error ID.
 *   - Forms error packet and checks whether it exceeds the max size.
 *   - Sends error packet to report error to 3LSS.
 *
 * @return None
 * @retval None
 */
void nvgpu_report_gr_err(struct gk20a *g, u32 hw_unit, u32 inst,
		u32 err_id, struct gr_err_info *err_info, u32 sub_err_type);

/**
 * @brief Report PMU error to 3LSS.
 *
 * @param g[in]			- The GPU driver struct.
 * @param hw_unit[in]		- Index of HW unit (PMU).
 * @param err_id[in]		- Error index.
 * @param sub_err_type[in]	- Sub error type.
 * @param status[in]		- Error information.
 *
 * - In case of linux/posix implementation, it simply returns 0 since SDL is not
 *   supported for them.
 * - In case of nvgpu-qnx, it does the following:
 *   - Checks whether SDL is supported in the current GPU platform.
 *   - Validates the HW unit ID and error ID.
 *   - Forms error packet and checks whether it exceeds the max size.
 *   - Sends error packet to report error to 3LSS.
 *
 * @return None
 * @retval None
 */
void nvgpu_report_pmu_err(struct gk20a *g, u32 hw_unit, u32 err_id,
	u32 sub_err_type, u32 status);

/**
 * @brief Report PRI error to 3LSS.
 *
 * @param g[in]			- The GPU driver struct.
 * @param hw_unit[in]		- Index of HW unit (PMU).
 * @param err_id[in]		- Error index.
 * @param sub_err_type[in]	- Sub error type.
 * @param status[in]		- Error information.
 *
 * - In case of linux/posix implementation, it simply returns 0 since SDL is not
 *   supported for them.
 * - In case of nvgpu-qnx, it does the following:
 *   - Checks whether SDL is supported in the current GPU platform.
 *   - Validates the HW unit ID and error ID.
 *   - Forms error packet and checks whether it exceeds the max size.
 *   - Sends error packet to report error to 3LSS.
 *
 * @return None
 * @retval None
 */
void nvgpu_report_pri_err(struct gk20a *g, u32 hw_unit, u32 inst,
		u32 err_id, u32 err_addr, u32 err_code);

/**
 * @brief Report MMU page fault error to 3LSS.
 *
 * @param g[in]			- The GPU driver struct.
 * @param hw_unit[in]		- Index of HW unit (PMU).
 * @param err_id[in]		- Error index.
 * @param fault_info[in]	- MMU page fault information.
 * @param sub_err_type[in]	- Sub error type.
 *
 * - In case of linux/posix implementation, it simply returns 0 since SDL is not
 *   supported for them.
 * - In case of nvgpu-qnx, it does the following:
 *   - Checks whether SDL is supported in the current GPU platform.
 *   - Validates the HW unit ID and error ID.
 *   - Forms error packet and checks whether it exceeds the max size.
 *   - Sends error packet to report error to 3LSS.
 *
 * @return None
 * @retval None
 */
void nvgpu_report_mmu_err(struct gk20a *g, u32 hw_unit,
		u32 err_id, struct mmu_fault_info *fault_info,
		u32 status, u32 sub_err_type);

#endif /* NVGPU_NVGPU_ERR_H */
