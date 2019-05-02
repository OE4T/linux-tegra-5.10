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

#define NVGPU_ERR_MODULE_HOST		0U
#define NVGPU_ERR_MODULE_SM		1U
#define NVGPU_ERR_MODULE_FECS		2U
#define NVGPU_ERR_MODULE_GPCCS		3U
#define NVGPU_ERR_MODULE_MMU		4U
#define NVGPU_ERR_MODULE_GCC		5U
#define NVGPU_ERR_MODULE_PWR		6U
#define NVGPU_ERR_MODULE_PGRAPH		7U
#define NVGPU_ERR_MODULE_LTC		8U
#define NVGPU_ERR_MODULE_HUBMMU		9U
#define NVGPU_ERR_MODULE_PRI		10U
#define NVGPU_ERR_MODULE_CE		11U

#define GPU_HOST_PFIFO_BIND_ERROR		0U
#define GPU_HOST_PFIFO_SCHED_ERROR		1U
#define GPU_HOST_PFIFO_CHSW_ERROR		2U
#define GPU_HOST_PFIFO_MEMOP_TIMEOUT_ERROR	3U
#define GPU_HOST_PFIFO_LB_ERROR			4U
#define GPU_HOST_PBUS_SQUASH_ERROR		5U
#define GPU_HOST_PBUS_FECS_ERROR		6U
#define GPU_HOST_PBUS_TIMEOUT_ERROR		7U
#define GPU_HOST_PBDMA_TIMEOUT_ERROR		8U
#define GPU_HOST_PBDMA_EXTRA_ERROR		9U
#define GPU_HOST_PBDMA_GPFIFO_PB_ERROR		10U
#define GPU_HOST_PBDMA_METHOD_ERROR		11U
#define GPU_HOST_PBDMA_SIGNATURE_ERROR		12U
#define GPU_HOST_PBDMA_HCE_ERROR		13U
#define GPU_HOST_PBDMA_PREEMPT_ERROR		14U
#define GPU_HOST_PFIFO_CTXSW_TIMEOUT_ERROR	15U
#define GPU_HOST_INVALID_ERROR			16U

#define GPU_SM_L1_TAG_ECC_CORRECTED		0U
#define GPU_SM_L1_TAG_ECC_UNCORRECTED		1U
#define GPU_SM_CBU_ECC_CORRECTED		2U
#define GPU_SM_CBU_ECC_UNCORRECTED		3U
#define GPU_SM_LRF_ECC_CORRECTED		4U
#define GPU_SM_LRF_ECC_UNCORRECTED		5U
#define GPU_SM_L1_DATA_ECC_CORRECTED		6U
#define GPU_SM_L1_DATA_ECC_UNCORRECTED		7U
#define GPU_SM_ICACHE_L0_DATA_ECC_CORRECTED	8U
#define GPU_SM_ICACHE_L0_DATA_ECC_UNCORRECTED	9U
#define GPU_SM_ICACHE_L1_DATA_ECC_CORRECTED	10U
#define GPU_SM_ICACHE_L1_DATA_ECC_UNCORRECTED	11U
#define GPU_SM_ICACHE_L0_PREDECODE_ECC_CORRECTED	12U
#define GPU_SM_ICACHE_L0_PREDECODE_ECC_UNCORRECTED	13U
#define GPU_SM_L1_TAG_MISS_FIFO_ECC_CORRECTED		14U
#define GPU_SM_L1_TAG_MISS_FIFO_ECC_UNCORRECTED		15U
#define GPU_SM_L1_TAG_S2R_PIXPRF_ECC_CORRECTED		16U
#define GPU_SM_L1_TAG_S2R_PIXPRF_ECC_UNCORRECTED	17U
#define GPU_SM_MACHINE_CHECK_ERROR			18U
struct gr_sm_mcerr_info {
	u64 hww_warp_esr_pc;	/* PC which triggered the machine check error */
	u32 hww_warp_esr_status;/* Error status register */
	u32 curr_ctx;		/* Context which triggered error */
	u32 chid;		/* Channel to which the context belongs */
	u32 tsgid;		/* TSG to which the channel is bound */
	u32 tpc, gpc, sm;
};
#define GPU_SM_ICACHE_L1_PREDECODE_ECC_CORRECTED	19U
#define GPU_SM_ICACHE_L1_PREDECODE_ECC_UNCORRECTED	20U

#define GPU_FECS_FALCON_IMEM_ECC_CORRECTED	0U
#define GPU_FECS_FALCON_IMEM_ECC_UNCORRECTED	1U
#define GPU_FECS_FALCON_DMEM_ECC_CORRECTED	2U
#define GPU_FECS_FALCON_DMEM_ECC_UNCORRECTED	3U
#define GPU_FECS_CTXSW_WATCHDOG_TIMEOUT		4U
#define GPU_FECS_CTXSW_CRC_MISMATCH		5U
#define GPU_FECS_FAULT_DURING_CTXSW		6U
#define GPU_FECS_CTXSW_INIT_ERROR		7U
#define GPU_FECS_INVALID_ERROR			8U
struct ctxsw_err_info {
	u32 curr_ctx;
	u32 ctxsw_status0;
	u32 ctxsw_status1;
	u32 chid;
	u32 mailbox_value;
};

#define GPU_GPCCS_FALCON_IMEM_ECC_CORRECTED	0U
#define GPU_GPCCS_FALCON_IMEM_ECC_UNCORRECTED	1U
#define GPU_GPCCS_FALCON_DMEM_ECC_CORRECTED	2U
#define GPU_GPCCS_FALCON_DMEM_ECC_UNCORRECTED	3U

#define GPU_MMU_L1TLB_ECC_CORRECTED		0U
#define GPU_MMU_L1TLB_ECC_UNCORRECTED		1U
#define GPU_MMU_L1TLB_SA_DATA_ECC_CORRECTED	2U
#define GPU_MMU_L1TLB_SA_DATA_ECC_UNCORRECTED	3U
#define GPU_MMU_L1TLB_FA_DATA_ECC_CORRECTED	4U
#define GPU_MMU_L1TLB_FA_DATA_ECC_UNCORRECTED	5U

#define GPU_GCC_L15_ECC_CORRECTED		0U
#define GPU_GCC_L15_ECC_UNCORRECTED		1U

#define GPU_PMU_FALCON_IMEM_ECC_CORRECTED	0U
#define GPU_PMU_FALCON_IMEM_ECC_UNCORRECTED	1U
#define GPU_PMU_FALCON_DMEM_ECC_CORRECTED	2U
#define GPU_PMU_FALCON_DMEM_ECC_UNCORRECTED	3U
#define GPU_PMU_BAR0_ERROR_TIMEOUT		4U

#define GPU_PGRAPH_FE_EXCEPTION			0U
#define GPU_PGRAPH_MEMFMT_EXCEPTION		1U
#define GPU_PGRAPH_PD_EXCEPTION			2U
#define GPU_PGRAPH_SCC_EXCEPTION		3U
#define GPU_PGRAPH_DS_EXCEPTION			4U
#define GPU_PGRAPH_SSYNC_EXCEPTION		5U
#define GPU_PGRAPH_MME_EXCEPTION		6U
#define GPU_PGRAPH_SKED_EXCEPTION		7U
#define GPU_PGRAPH_BE_EXCEPTION			8U
#define GPU_PGRAPH_MPC_EXCEPTION		9U
struct gr_exception_info {
	u32 curr_ctx;	/* Context which triggered the exception */
	u32 chid;	/* Channel bound to the context */
	u32 tsgid;	/* TSG to which the channel is bound */
	u32 status;	/* Exception status */
};

#define GPU_LTC_CACHE_DSTG_ECC_CORRECTED	0U
#define GPU_LTC_CACHE_DSTG_ECC_UNCORRECTED	1U
#define GPU_LTC_CACHE_TSTG_ECC_CORRECTED	2U
#define GPU_LTC_CACHE_TSTG_ECC_UNCORRECTED	3U
#define GPU_LTC_CACHE_RSTG_ECC_CORRECTED	4U
#define GPU_LTC_CACHE_RSTG_ECC_UNCORRECTED	5U
#define GPU_LTC_CACHE_DSTG_BE_ECC_CORRECTED	6U
#define GPU_LTC_CACHE_DSTG_BE_ECC_UNCORRECTED	7U

#define GPU_HUBMMU_L2TLB_SA_DATA_ECC_CORRECTED		0U
#define GPU_HUBMMU_L2TLB_SA_DATA_ECC_UNCORRECTED	1U
#define GPU_HUBMMU_TLB_SA_DATA_ECC_CORRECTED		2U
#define GPU_HUBMMU_TLB_SA_DATA_ECC_UNCORRECTED		3U
#define GPU_HUBMMU_PTE_DATA_ECC_CORRECTED		4U
#define GPU_HUBMMU_PTE_DATA_ECC_UNCORRECTED		5U
#define GPU_HUBMMU_PDE0_DATA_ECC_CORRECTED		6U
#define GPU_HUBMMU_PDE0_DATA_ECC_UNCORRECTED		7U

#define GPU_PRI_TIMEOUT_ERROR		0U
#define GPU_PRI_ACCESS_VIOLATION	1U

#define GPU_CE_LAUNCH_ERROR			0U
#define GPU_CE_BLOCK_PIPE			1U
#define GPU_CE_NONBLOCK_PIPE			2U
#define GPU_CE_INVALID_CONFIG			3U
#define GPU_CE_METHOD_BUFFER_FAULT		4U

struct gr_err_info {
	struct gr_sm_mcerr_info *sm_mcerr_info;
	struct gr_exception_info *exception_info;
};

void nvgpu_report_host_error(struct gk20a *g,
		u32 inst, u32 err_id, u32 intr_info);

void nvgpu_report_ce_error(struct gk20a *g, u32 inst,
		u32 err_type, u32 status);

void nvgpu_hubmmu_report_ecc_error(struct gk20a *g, u32 inst,
		u32 err_type, u64 err_addr, u64 err_cnt);

void nvgpu_ltc_report_ecc_error(struct gk20a *g, u32 ltc, u32 slice,
		u32 err_type, u64 err_addr, u64 err_cnt);

void nvgpu_pmu_report_ecc_error(struct gk20a *g, u32 inst,
		u32 err_type, u64 err_addr, u64 err_cnt);

void nvgpu_gr_report_ecc_error(struct gk20a *g, u32 hw_module,
		u32 gpc, u32 tpc, u32 err_type,
		u64 err_addr, u64 err_cnt);
#endif
