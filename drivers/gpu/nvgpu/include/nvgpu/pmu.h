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

#ifndef NVGPU_PMU_H
#define NVGPU_PMU_H

#include <nvgpu/kmem.h>
#include <nvgpu/allocator.h>
#include <nvgpu/lock.h>
#include <nvgpu/nvgpu_common.h>
#include <nvgpu/flcnif_cmn.h>
#include <nvgpu/pmu/pmuif/nvgpu_cmdif.h>
#include <nvgpu/falcon.h>
#include <nvgpu/timers.h>
#include <nvgpu/pmu/pmu_pg.h>
#include <nvgpu/pmu/seq.h>
#include <nvgpu/pmu/mutex.h>
#include <nvgpu/pmu/queue.h>
#include <nvgpu/pmu/msg.h>
#include <nvgpu/pmu/fw.h>

#define nvgpu_pmu_dbg(g, fmt, args...) \
	nvgpu_log(g, gpu_dbg_pmu, fmt, ##args)

struct nvgpu_clk_pmupstate;

/* defined by pmu hw spec */
#define GK20A_PMU_VA_SIZE		(512U * 1024U * 1024U)
#define GK20A_PMU_UCODE_SIZE_MAX	(256U * 1024U)
#define GK20A_PMU_SEQ_BUF_SIZE		4096U

#define GK20A_PMU_TRACE_BUFSIZE		0x4000U    /* 4K */
#define GK20A_PMU_DMEM_BLKSIZE2		8U

#define PMU_MODE_MISMATCH_STATUS_MAILBOX_R  6U
#define PMU_MODE_MISMATCH_STATUS_VAL        0xDEADDEADU

#define GK20A_PMU_UCODE_NB_MAX_OVERLAY	    32U
#define GK20A_PMU_UCODE_NB_MAX_DATE_LENGTH  64U

#define	GK20A_PMU_DMAIDX_UCODE		U32(0)
#define	GK20A_PMU_DMAIDX_VIRT		U32(1)
#define	GK20A_PMU_DMAIDX_PHYS_VID	U32(2)
#define	GK20A_PMU_DMAIDX_PHYS_SYS_COH	U32(3)
#define	GK20A_PMU_DMAIDX_PHYS_SYS_NCOH	U32(4)
#define	GK20A_PMU_DMAIDX_RSVD		U32(5)
#define	GK20A_PMU_DMAIDX_PELPG		U32(6)
#define	GK20A_PMU_DMAIDX_END		U32(7)

#define	PMU_BAR0_SUCCESS		0U
#define	PMU_BAR0_HOST_READ_TOUT		1U
#define	PMU_BAR0_HOST_WRITE_TOUT	2U
#define	PMU_BAR0_FECS_READ_TOUT		3U
#define	PMU_BAR0_FECS_WRITE_TOUT	4U
#define	PMU_BAR0_CMD_READ_HWERR		5U
#define	PMU_BAR0_CMD_WRITE_HWERR	6U
#define	PMU_BAR0_READ_HOSTERR		7U
#define	PMU_BAR0_WRITE_HOSTERR		8U
#define	PMU_BAR0_READ_FECSERR		9U
#define	PMU_BAR0_WRITE_FECSERR		10U
#define	ACR_BOOT_TIMEDOUT		11U
#define	ACR_BOOT_FAILED			12U

/* pmu load const defines */
#define PMU_BUSY_CYCLES_NORM_MAX		(1000U)

struct rpc_handler_payload {
	void *rpc_buff;
	bool is_mem_free_set;
	bool complete;
};

struct pmu_rpc_desc {
	void   *prpc;
	u16 size_rpc;
	u16 size_scratch;
};

struct pmu_in_out_payload_desc {
	void *buf;
	u32 offset;
	u32 size;
	u32 fb_size;
};

struct pmu_payload {
	struct pmu_in_out_payload_desc in;
	struct pmu_in_out_payload_desc out;
	struct pmu_rpc_desc rpc;
};

struct pmu_ucode_desc {
	u32 descriptor_size;
	u32 image_size;
	u32 tools_version;
	u32 app_version;
	char date[GK20A_PMU_UCODE_NB_MAX_DATE_LENGTH];
	u32 bootloader_start_offset;
	u32 bootloader_size;
	u32 bootloader_imem_offset;
	u32 bootloader_entry_point;
	u32 app_start_offset;
	u32 app_size;
	u32 app_imem_offset;
	u32 app_imem_entry;
	u32 app_dmem_offset;
	/* Offset from appStartOffset */
	u32 app_resident_code_offset;
	/* Exact size of the resident code
	 * ( potentially contains CRC inside at the end )
	 */
	u32 app_resident_code_size;
	/* Offset from appStartOffset */
	u32 app_resident_data_offset;
	/* Exact size of the resident code
	 * ( potentially contains CRC inside at the end )
	 */
	u32 app_resident_data_size;
	u32 nb_overlays;
	struct {u32 start; u32 size; } load_ovl[GK20A_PMU_UCODE_NB_MAX_OVERLAY];
	u32 compressed;
};

struct nvgpu_pmu {
	struct gk20a *g;
	struct nvgpu_falcon flcn;

	struct pmu_rtos_fw fw;

	struct nvgpu_pmu_lsfm *lsfm;

	struct nvgpu_mem trace_buf;

	struct pmu_super_surface *super_surface;

	struct pmu_sha1_gid gid_info;

	struct pmu_queues queues;
	struct pmu_sequences sequences;

	struct pmu_mutexes mutexes;

	struct nvgpu_allocator dmem;

	struct nvgpu_pmu_pg *pg;
	struct nvgpu_pmu_perfmon *pmu_perfmon;
	struct nvgpu_clk_pmupstate *clk_pmu;

	void (*remove_support)(struct nvgpu_pmu *pmu);
	void (*volt_rpc_handler)(struct gk20a *g,
			struct nv_pmu_rpc_header *rpc);
	void (*therm_event_handler)(struct gk20a *g, struct nvgpu_pmu *pmu,
		struct pmu_msg *msg, struct nv_pmu_rpc_header *rpc);
	bool sw_ready;

	struct nvgpu_mutex isr_mutex;
	bool isr_enabled;
};

/*!
 * Structure/object which single register write need to be done during PG init
 * sequence to set PROD values.
 */
struct pg_init_sequence_list {
	u32 regaddr;
	u32 writeval;
};

int nvgpu_pmu_lock_acquire(struct gk20a *g, struct nvgpu_pmu *pmu,
			   u32 id, u32 *token);
int nvgpu_pmu_lock_release(struct gk20a *g, struct nvgpu_pmu *pmu,
			   u32 id, u32 *token);

/* PMU init */
int nvgpu_init_pmu_support(struct gk20a *g);
int nvgpu_pmu_destroy(struct gk20a *g);
int nvgpu_pmu_super_surface_alloc(struct gk20a *g,
	struct nvgpu_mem *mem_surface, u32 size);

int nvgpu_early_init_pmu_sw(struct gk20a *g, struct nvgpu_pmu *pmu);

/* PMU reset */
int nvgpu_pmu_reset(struct gk20a *g);

/* PMU debug */
void nvgpu_pmu_dump_falcon_stats(struct nvgpu_pmu *pmu);
bool nvgpu_find_hex_in_string(char *strings, struct gk20a *g, u32 *hex_pos);

struct gk20a *gk20a_from_pmu(struct nvgpu_pmu *pmu);

void nvgpu_pmu_report_bar0_pri_err_status(struct gk20a *g, u32 bar0_status,
	u32 error_type);

#endif /* NVGPU_PMU_H */

