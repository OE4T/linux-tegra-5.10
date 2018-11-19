/*
 * Copyright (c) 2017-2018, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_FALCON_H
#define NVGPU_FALCON_H

#include <nvgpu/types.h>
#include <nvgpu/lock.h>

/*
 * Falcon Id Defines
 */
#define FALCON_ID_PMU       (0U)
#define FALCON_ID_GSPLITE   (1U)
#define FALCON_ID_FECS      (2U)
#define FALCON_ID_GPCCS     (3U)
#define FALCON_ID_NVDEC     (4U)
#define FALCON_ID_SEC2      (7U)
#define FALCON_ID_MINION    (10U)

/*
 * Falcon Base address Defines
 */
#define FALCON_NVDEC_BASE  0x00084000U
#define FALCON_PWR_BASE    0x0010a000U
#define FALCON_SEC_BASE    0x00087000U
#define FALCON_FECS_BASE   0x00409000U
#define FALCON_GPCCS_BASE  0x0041a000U

/* Falcon Register index */
#define FALCON_REG_R0		(0U)
#define FALCON_REG_R1		(1U)
#define FALCON_REG_R2		(2U)
#define FALCON_REG_R3		(3U)
#define FALCON_REG_R4		(4U)
#define FALCON_REG_R5		(5U)
#define FALCON_REG_R6		(6U)
#define FALCON_REG_R7		(7U)
#define FALCON_REG_R8		(8U)
#define FALCON_REG_R9		(9U)
#define FALCON_REG_R10		(10U)
#define FALCON_REG_R11		(11U)
#define FALCON_REG_R12		(12U)
#define FALCON_REG_R13		(13U)
#define FALCON_REG_R14		(14U)
#define FALCON_REG_R15		(15U)
#define FALCON_REG_IV0		(16U)
#define FALCON_REG_IV1		(17U)
#define FALCON_REG_UNDEFINED	(18U)
#define FALCON_REG_EV		(19U)
#define FALCON_REG_SP		(20U)
#define FALCON_REG_PC		(21U)
#define FALCON_REG_IMB		(22U)
#define FALCON_REG_DMB		(23U)
#define FALCON_REG_CSW		(24U)
#define FALCON_REG_CCR		(25U)
#define FALCON_REG_SEC		(26U)
#define FALCON_REG_CTX		(27U)
#define FALCON_REG_EXCI		(28U)
#define FALCON_REG_RSVD0	(29U)
#define FALCON_REG_RSVD1	(30U)
#define FALCON_REG_RSVD2	(31U)
#define FALCON_REG_SIZE		(32U)

#define FALCON_MAILBOX_0	0x0U
#define FALCON_MAILBOX_1	0x1U
#define FALCON_MAILBOX_COUNT 0x02U
#define FALCON_BLOCK_SIZE 0x100U

#define GET_IMEM_TAG(IMEM_ADDR) ((IMEM_ADDR) >> 8U)

#define GET_NEXT_BLOCK(ADDR) \
	(((((ADDR) + (FALCON_BLOCK_SIZE - 1U)) & ~(FALCON_BLOCK_SIZE-1U)) \
		/ FALCON_BLOCK_SIZE) << 8U)

/*
 * Falcon HWCFG request read types defines
 */
enum falcon_hwcfg_read {
	FALCON_IMEM_SIZE = 0,
	FALCON_DMEM_SIZE,
	FALCON_CORE_REV,
	FALCON_SECURITY_MODEL,
	FLACON_MAILBOX_COUNT
};

/*
 * Falcon HWCFG request write types defines
 */
enum falcon_hwcfg_write {
	FALCON_STARTCPU = 0,
	FALCON_STARTCPU_SECURE,
	FALCON_BOOTVEC,
	FALCON_ITF_EN
};

#define FALCON_MEM_SCRUBBING_TIMEOUT_MAX 1000U
#define FALCON_MEM_SCRUBBING_TIMEOUT_DEFAULT 10U

enum falcon_dma_dir {
	DMA_TO_FB = 0,
	DMA_FROM_FB
};

enum falcon_mem_type {
	MEM_DMEM = 0,
	MEM_IMEM
};

/* Falcon ucode header format
 * OS Code Offset
 * OS Code Size
 * OS Data Offset
 * OS Data Size
 * NumApps (N)
 * App   0 Code Offset
 * App   0 Code Size
 * .  .  .  .
 * App   N - 1 Code Offset
 * App   N - 1 Code Size
 * App   0 Data Offset
 * App   0 Data Size
 * .  .  .  .
 * App   N - 1 Data Offset
 * App   N - 1 Data Size
 * OS Ovl Offset
 * OS Ovl Size
*/
#define OS_CODE_OFFSET 0x0U
#define OS_CODE_SIZE   0x1U
#define OS_DATA_OFFSET 0x2U
#define OS_DATA_SIZE   0x3U
#define NUM_APPS       0x4U
#define APP_0_CODE_OFFSET 0x5U
#define APP_0_CODE_SIZE   0x6U

struct nvgpu_falcon_dma_info {
	u32 fb_base;
	u32 fb_off;
	u32 flcn_mem_off;
	u32 size_in_bytes;
	enum falcon_dma_dir dir;
	u32 ctx_dma;
	enum falcon_mem_type flcn_mem;
	u32 is_wait_complete;
};

struct gk20a;
struct nvgpu_falcon;
struct nvgpu_falcon_bl_info;

/* Queue Type */
#define QUEUE_TYPE_DMEM 0x0U
#define QUEUE_TYPE_EMEM 0x1U

struct nvgpu_falcon_queue {

	/* Queue Type (queue_type) */
	u8 queue_type;

	/* used by nvgpu, for command LPQ/HPQ */
	struct nvgpu_mutex mutex;

	/* current write position */
	u32 position;
	/* physical dmem offset where this queue begins */
	u32 offset;
	/* logical queue identifier */
	u32 id;
	/* physical queue index */
	u32 index;
	/* in bytes */
	u32 size;
	/* open-flag */
	u32 oflag;

	/* queue type(DMEM-Q/FB-Q) specific ops */
	int (*rewind)(struct nvgpu_falcon *flcn,
		struct nvgpu_falcon_queue *queue);
	int (*pop)(struct nvgpu_falcon *flcn,
		struct nvgpu_falcon_queue *queue, void *data, u32 size,
		u32 *bytes_read);
	int (*push)(struct nvgpu_falcon *flcn,
		struct nvgpu_falcon_queue *queue, void *data, u32 size);
	bool (*has_room)(struct nvgpu_falcon *flcn,
		struct nvgpu_falcon_queue *queue, u32 size,
		bool *need_rewind);
	int (*tail)(struct nvgpu_falcon *flcn,
		struct nvgpu_falcon_queue *queue, u32 *tail, bool set);
	int (*head)(struct nvgpu_falcon *flcn,
		struct nvgpu_falcon_queue *queue, u32 *head, bool set);
};

struct nvgpu_falcon_version_ops {
	void (*start_cpu_secure)(struct nvgpu_falcon *flcn);
	void (*write_dmatrfbase)(struct nvgpu_falcon *flcn, u32 addr);
};

/* ops which are falcon engine specific */
struct nvgpu_falcon_engine_dependency_ops {
	int (*reset_eng)(struct gk20a *g);
	int (*queue_head)(struct gk20a *g, struct nvgpu_falcon_queue *queue,
		u32 *head, bool set);
	int (*queue_tail)(struct gk20a *g, struct nvgpu_falcon_queue *queue,
		u32 *tail, bool set);
	void (*msgq_tail)(struct gk20a *g, u32 *tail, bool set);
	int (*copy_from_emem)(struct nvgpu_falcon *flcn, u32 src, u8 *dst,
		u32 size, u8 port);
	int (*copy_to_emem)(struct nvgpu_falcon *flcn, u32 dst, u8 *src,
		u32 size, u8 port);
};

struct nvgpu_falcon_ops {
	int (*reset)(struct nvgpu_falcon *flcn);
	void (*set_irq)(struct nvgpu_falcon *flcn, bool enable);
	bool (*clear_halt_interrupt_status)(struct nvgpu_falcon *flcn);
	bool (*is_falcon_cpu_halted)(struct nvgpu_falcon *flcn);
	bool (*is_falcon_idle)(struct nvgpu_falcon *flcn);
	bool (*is_falcon_scrubbing_done)(struct nvgpu_falcon *flcn);
	int (*copy_from_dmem)(struct nvgpu_falcon *flcn, u32 src, u8 *dst,
		u32 size, u8 port);
	int (*copy_to_dmem)(struct nvgpu_falcon *flcn, u32 dst, u8 *src,
		u32 size, u8 port);
	int (*copy_from_imem)(struct nvgpu_falcon *flcn, u32 src, u8 *dst,
		u32 size, u8 port);
	int (*copy_to_imem)(struct nvgpu_falcon *flcn, u32 dst, u8 *src,
		u32 size, u8 port, bool sec, u32 tag);
	int (*dma_copy)(struct nvgpu_falcon *flcn,
			struct nvgpu_falcon_dma_info *dma_info);
	u32 (*mailbox_read)(struct nvgpu_falcon *flcn, u32 mailbox_index);
	void (*mailbox_write)(struct nvgpu_falcon *flcn, u32 mailbox_index,
		u32 data);
	int (*bootstrap)(struct nvgpu_falcon *flcn, u32 boot_vector);
	void (*dump_falcon_stats)(struct nvgpu_falcon *flcn);
	int (*bl_bootstrap)(struct nvgpu_falcon *flcn,
		struct nvgpu_falcon_bl_info *bl_info);
};

struct nvgpu_falcon_bl_info {
	void *bl_src;
	u8 *bl_desc;
	u32 bl_desc_size;
	u32 bl_size;
	u32 bl_start_tag;
};

struct nvgpu_falcon {
	struct gk20a *g;
	u32 flcn_id;
	u32 flcn_base;
	u32 flcn_core_rev;
	bool is_falcon_supported;
	bool is_interrupt_enabled;
	u32 intr_mask;
	u32 intr_dest;
	bool isr_enabled;
	struct nvgpu_mutex isr_mutex;
	struct nvgpu_mutex copy_lock;
	struct nvgpu_falcon_ops flcn_ops;
	struct nvgpu_falcon_version_ops flcn_vops;
	struct nvgpu_falcon_engine_dependency_ops flcn_engine_dep_ops;
};

int nvgpu_falcon_wait_idle(struct nvgpu_falcon *flcn);
int nvgpu_falcon_wait_for_halt(struct nvgpu_falcon *flcn, unsigned int timeout);
int nvgpu_falcon_clear_halt_intr_status(struct nvgpu_falcon *flcn,
		unsigned int timeout);
int nvgpu_falcon_reset(struct nvgpu_falcon *flcn);
void nvgpu_falcon_set_irq(struct nvgpu_falcon *flcn, bool enable,
	u32 intr_mask, u32 intr_dest);
bool nvgpu_falcon_get_mem_scrubbing_status(struct nvgpu_falcon *flcn);
int nvgpu_falcon_mem_scrub_wait(struct nvgpu_falcon *flcn);
bool nvgpu_falcon_get_cpu_halted_status(struct nvgpu_falcon *flcn);
bool nvgpu_falcon_get_idle_status(struct nvgpu_falcon *flcn);
int nvgpu_falcon_copy_from_emem(struct nvgpu_falcon *flcn,
	u32 src, u8 *dst, u32 size, u8 port);
int nvgpu_falcon_copy_to_emem(struct nvgpu_falcon *flcn,
	u32 dst, u8 *src, u32 size, u8 port);
int nvgpu_falcon_copy_from_dmem(struct nvgpu_falcon *flcn,
	u32 src, u8 *dst, u32 size, u8 port);
int nvgpu_falcon_copy_to_dmem(struct nvgpu_falcon *flcn,
	u32 dst, u8 *src, u32 size, u8 port);
int nvgpu_falcon_copy_to_imem(struct nvgpu_falcon *flcn,
	u32 dst, u8 *src, u32 size, u8 port, bool sec, u32 tag);
int nvgpu_falcon_copy_from_imem(struct nvgpu_falcon *flcn,
	u32 src, u8 *dst, u32 size, u8 port);
int nvgpu_falcon_dma_copy(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_dma_info *dma_info);
u32 nvgpu_falcon_mailbox_read(struct nvgpu_falcon *flcn, u32 mailbox_index);
void nvgpu_falcon_mailbox_write(struct nvgpu_falcon *flcn, u32 mailbox_index,
	u32 data);
int nvgpu_falcon_bootstrap(struct nvgpu_falcon *flcn, u32 boot_vector);
void nvgpu_falcon_print_dmem(struct nvgpu_falcon *flcn, u32 src, u32 size);
void nvgpu_falcon_print_imem(struct nvgpu_falcon *flcn, u32 src, u32 size);
void nvgpu_falcon_dump_stats(struct nvgpu_falcon *flcn);
int nvgpu_falcon_bl_bootstrap(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_bl_info *bl_info);

/* queue public functions */
int nvgpu_falcon_queue_init(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue);
bool nvgpu_falcon_queue_is_empty(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue);
int nvgpu_falcon_queue_rewind(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue);
int nvgpu_falcon_queue_pop(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue, void *data, u32 size,
	u32 *bytes_read);
int nvgpu_falcon_queue_push(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue, void *data, u32 size);
void nvgpu_falcon_queue_free(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_queue *queue);

int nvgpu_falcon_sw_init(struct gk20a *g, u32 flcn_id);

#endif /* NVGPU_FALCON_H */
