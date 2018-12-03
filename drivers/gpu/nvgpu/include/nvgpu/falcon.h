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

#ifndef NVGPU_FALCON_H
#define NVGPU_FALCON_H

#include <nvgpu/types.h>
#include <nvgpu/lock.h>

/*
 * Falcon Id Defines
 */
#define FALCON_ID_PMU       (0U)
#define FALCON_ID_FECS      (2U)
#define FALCON_ID_GPCCS     (3U)

#ifdef CONFIG_NVGPU_DGPU
#define FALCON_ID_GSPLITE   (1U)
#define FALCON_ID_NVDEC     (4U)
#define FALCON_ID_SEC2      (7U)
#define FALCON_ID_MINION    (10U)
#endif

#define FALCON_ID_END	    (11U)
#define FALCON_ID_INVALID   0xFFFFFFFFU

#define FALCON_MAILBOX_0	0x0U
#define FALCON_MAILBOX_1	0x1U
#define FALCON_MAILBOX_COUNT 0x02U
#define FALCON_BLOCK_SIZE 0x100U

#define GET_IMEM_TAG(IMEM_ADDR) ((IMEM_ADDR) >> 8U)

#define GET_NEXT_BLOCK(ADDR) \
	(((((ADDR) + (FALCON_BLOCK_SIZE - 1U)) & ~(FALCON_BLOCK_SIZE-1U)) \
		/ FALCON_BLOCK_SIZE) << 8U)

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

struct gk20a;
struct nvgpu_falcon;

enum falcon_mem_type {
	MEM_DMEM = 0,
	MEM_IMEM
};

struct nvgpu_falcon_bl_info {
	void *bl_src;
	u8 *bl_desc;
	u32 bl_desc_size;
	u32 bl_size;
	u32 bl_start_tag;
};

/* ops which are falcon engine specific */
struct nvgpu_falcon_engine_dependency_ops {
	int (*reset_eng)(struct gk20a *g);
	int (*copy_from_emem)(struct gk20a *g, u32 src, u8 *dst,
		u32 size, u8 port);
	int (*copy_to_emem)(struct gk20a *g, u32 dst, u8 *src,
		u32 size, u8 port);
};

struct nvgpu_falcon {
	struct gk20a *g;
	u32 flcn_id;
	u32 flcn_base;
	bool is_falcon_supported;
	bool is_interrupt_enabled;
	bool emem_supported;
	struct nvgpu_mutex imem_lock;
	struct nvgpu_mutex dmem_lock;
	struct nvgpu_mutex emem_lock;
	struct nvgpu_falcon_engine_dependency_ops flcn_engine_dep_ops;
};

int nvgpu_falcon_wait_idle(struct nvgpu_falcon *flcn);
int nvgpu_falcon_wait_for_halt(struct nvgpu_falcon *flcn, unsigned int timeout);
int nvgpu_falcon_clear_halt_intr_status(struct nvgpu_falcon *flcn,
		unsigned int timeout);
int nvgpu_falcon_reset(struct nvgpu_falcon *flcn);
void nvgpu_falcon_set_irq(struct nvgpu_falcon *flcn, bool enable,
	u32 intr_mask, u32 intr_dest);
int nvgpu_falcon_mem_scrub_wait(struct nvgpu_falcon *flcn);
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
u32 nvgpu_falcon_mailbox_read(struct nvgpu_falcon *flcn, u32 mailbox_index);
void nvgpu_falcon_mailbox_write(struct nvgpu_falcon *flcn, u32 mailbox_index,
	u32 data);
int nvgpu_falcon_bootstrap(struct nvgpu_falcon *flcn, u32 boot_vector);
void nvgpu_falcon_print_dmem(struct nvgpu_falcon *flcn, u32 src, u32 size);
void nvgpu_falcon_print_imem(struct nvgpu_falcon *flcn, u32 src, u32 size);
void nvgpu_falcon_dump_stats(struct nvgpu_falcon *flcn);
int nvgpu_falcon_bl_bootstrap(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_bl_info *bl_info);
void nvgpu_falcon_get_ctls(struct nvgpu_falcon *flcn, u32 *sctl, u32 *cpuctl);
int nvgpu_falcon_get_mem_size(struct nvgpu_falcon *flcn,
			      enum falcon_mem_type type, u32 *size);
u32 nvgpu_falcon_get_id(struct nvgpu_falcon *flcn);

struct nvgpu_falcon *nvgpu_falcon_get_instance(struct gk20a *g, u32 flcn_id);
int nvgpu_falcon_sw_init(struct gk20a *g, u32 flcn_id);
void nvgpu_falcon_sw_free(struct gk20a *g, u32 flcn_id);

#endif /* NVGPU_FALCON_H */
