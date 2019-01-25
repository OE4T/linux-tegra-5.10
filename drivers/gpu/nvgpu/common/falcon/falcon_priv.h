/*
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_FALCON_PRIV_H
#define NVGPU_FALCON_PRIV_H

#include <nvgpu/lock.h>
#include <nvgpu/types.h>

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

struct gk20a;
struct nvgpu_falcon;
struct nvgpu_falcon_queue;

enum falcon_mem_type {
	MEM_DMEM = 0,
	MEM_IMEM
};

struct nvgpu_falcon_queue {
	struct gk20a *g;
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

	/* members unique to the FB version of the falcon queues */
	struct {
		/* Holds super surface base address */
		struct nvgpu_mem *super_surface_mem;

		/*
		 * Holds the offset of queue data (0th element).
		 * This is used for FB Queues to hold a offset of
		 * Super Surface for this queue.
		 */
		 u32 fb_offset;

		/*
		 * Define the size of a single queue element.
		 * queues_size above is used for the number of
		 * queue elements.
		 */
		u32 element_size;

		/* To keep track of elements in use */
		u64 element_in_use;

		/*
		 * Define a pointer to a local (SYSMEM) allocated
		 * buffer to hold a single queue element
		 * it is being assembled.
		 */
		 u8 *work_buffer;
		 struct nvgpu_mutex work_buffer_mutex;

		/*
		 * Tracks how much of the current FB Queue MSG queue
		 * entry have been read. This is needed as functions read
		 * the MSG queue as a byte stream, rather
		 * than reading a whole MSG at a time.
		 */
		u32 read_position;

		/*
		 * Tail as tracked on the nvgpu "side".  Because the queue
		 * elements and its associated payload (which is also moved
		 * PMU->nvgpu through the FB CMD Queue) can't be free-ed until
		 * the command is complete, response is received and any "out"
		 * payload delivered to the client, it is necessary for the
		 * nvgpu to track it's own version of "tail".  This one is
		 * incremented as commands and completed entries are found
		 * following tail.
		 */
		u32 tail;
	} fbq;

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

/* ops which are falcon engine specific */
struct nvgpu_falcon_engine_dependency_ops {
	int (*reset_eng)(struct gk20a *g);
	int (*queue_head)(struct gk20a *g, struct nvgpu_falcon_queue *queue,
		u32 *head, bool set);
	int (*queue_tail)(struct gk20a *g, struct nvgpu_falcon_queue *queue,
		u32 *tail, bool set);
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
	u32 (*mailbox_read)(struct nvgpu_falcon *flcn, u32 mailbox_index);
	void (*mailbox_write)(struct nvgpu_falcon *flcn, u32 mailbox_index,
		u32 data);
	int (*bootstrap)(struct nvgpu_falcon *flcn, u32 boot_vector);
	void (*dump_falcon_stats)(struct nvgpu_falcon *flcn);
	int (*bl_bootstrap)(struct nvgpu_falcon *flcn,
		struct nvgpu_falcon_bl_info *bl_info);
	void (*get_falcon_ctls)(struct nvgpu_falcon *flcn, u32 *sctl,
		u32 *cpuctl);
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
	struct nvgpu_falcon_engine_dependency_ops flcn_engine_dep_ops;
};

#endif /* NVGPU_FALCON_PRIV_H */
