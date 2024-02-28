/*
 * drivers/video/tegra/host/nvhost_cdma.h
 *
 * Tegra Graphics Host Command DMA
 *
 * Copyright (c) 2010-2022, NVIDIA Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __NVHOST_CDMA_H
#define __NVHOST_CDMA_H

#include <linux/sched.h>
#include <linux/semaphore.h>

#include <linux/nvhost.h>
#include <linux/list.h>

struct nvhost_syncpt;
struct nvhost_userctx_timeout;
struct nvhost_job;
struct mem_mgr;
struct mem_handle;

/* Number of gathers we allow to be queued up per channel. Must be a
 * power of two. Currently sized such that pushbuffer is 4KB (512*8B). */
#define NVHOST_GATHER_QUEUE_SIZE 512

  /* 8 bytes per slot. (This number does not include the final RESTART.) */
#define PUSH_BUFFER_SIZE (NVHOST_GATHER_QUEUE_SIZE * 8)

   /* 4K page containing GATHERed methods to increment channel syncpts
     * and replaces the original timed out contexts GATHER slots */
#define SYNCPT_INCR_BUFFER_SIZE_WORDS   (4096 / sizeof(u32))

/*
 * cdma
 *
 * This is in charge of a host command DMA channel.
 * Sends ops to a push buffer, and takes responsibility for unpinning
 * (& possibly freeing) of memory after those ops have completed.
 * Producer:
 *	begin
 *		push - send ops to the push buffer
 *	end - start command DMA and enqueue handles to be unpinned
 * Consumer:
 *	update - call to update sync queue and push buffer, unpin memory
 */

struct push_buffer {
	u32 *mapped;			/* mapped pushbuffer memory */
	dma_addr_t dma_addr;		/* dma address of pushbuffer */
	u32 fence;			/* index we've written */
	u32 cur;			/* index to write to */
};

struct buffer_timeout {
	struct delayed_work wq;		/* work queue */
	bool initialized;		/* timer one-time setup flag */
	struct nvhost_job_syncpt *sp;	/* buffer syncpoint information */
	ktime_t start_ktime;		/* starting time */
	/* context timeout information */
	int clientid;
	bool timeout_debug_dump;
	int num_syncpts;
	int timeout;
	bool allow_dependency;
};

enum cdma_event {
	CDMA_EVENT_NONE,		/* not waiting for any event */
	CDMA_EVENT_SYNC_QUEUE_EMPTY,	/* wait for empty sync queue */
	CDMA_EVENT_PUSH_BUFFER_SPACE	/* wait for space in push buffer */
};

/*
 * Notes on CDMA locking and synchronization :-
 *
 * We have various mutexes and semaphores to handle synchronization between
 * multiple threads and to protect CDMA resources and events
 * They are detailed as below
 *
 * 1) nvhost_cdma->lock
 * type : rw_semaphore
 *
 * We take a read lock in job submit path and job cleanup path so that these two
 * threads can run parallely - this is essential for performance and submit time
 * latencies to be consistent
 *
 * We take a write lock in timeout handler so that we block job submission and
 * job cleanup when recovery routine is triggered
 *
 * We take write lock for dumping debug data as well
 *
 * We should really avoid adding this lock to new places and prefer adding finer
 * and specific locks like below ones
 *
 * 2) nvhost_cdma->push_buffer_lock
 * type : mutex
 *
 * We use this lock to protect handling of CDMA_EVENT_PUSH_BUFFER_SPACE event
 *
 * 3) nvhost_cdma->sync_queue_lock
 * type : mutex
 *
 * We use this lock to protect accesses to nvhost_cdma->sync_queue list and
 * to protect handling of CDMA_EVENT_SYNC_QUEUE_EMPTY event
 *
 * 4) nvhost_cdma->timeout_lock
 * type : mutex
 *
 * We use this flag to protect timeout flags/settings and enabling/disabling
 * of timeout
 *
 * 5) nvhost_channel->submitlock
 * type : mutex
 *
 * We use this lock to serialize all submits on a channel/CDMA
 * This lock also protects interleaving of CDMA commands in case a channel is
 * shared between multiple users
 */

struct nvhost_cdma {
	struct semaphore sem;		/* signalled when event occurs */
	struct rw_semaphore lock;	/* controls access to shared state */
	struct mutex push_buffer_lock;	/* protect push buffer space event */
	struct mutex sync_queue_lock;	/* protect sync_queue accesses */
	struct mutex timeout_lock;	/* control access to timeout flag */
	enum cdma_event event;		/* event that sem is waiting for */
	unsigned int slots_used;	/* pb slots used in current submit */
	unsigned int slots_free;	/* pb slots free in current submit */
	unsigned int first_get;		/* DMAGET value, where submit begins */
	unsigned int last_put;		/* last value written to DMAPUT */
	struct push_buffer push_buffer;	/* channel's push buffer */
	struct list_head sync_queue;	/* job queue */
	struct buffer_timeout timeout;	/* channel's timeout state/wq */
	struct platform_device *pdev;	/* pointer to host1x device */
	bool running;
	bool torndown;
};

#define cdma_to_channel(cdma) container_of(cdma, struct nvhost_channel, cdma)
#define cdma_to_dev(cdma) nvhost_get_host(cdma->pdev)
#define pb_to_cdma(pb) container_of(pb, struct nvhost_cdma, push_buffer)

void nvhost_push_buffer_destroy(struct push_buffer *pb);
int nvhost_push_buffer_alloc(struct push_buffer *pb);
u32 nvhost_push_buffer_putptr(struct push_buffer *pb);
dma_addr_t nvhost_push_buffer_start(struct push_buffer *pb);
dma_addr_t nvhost_push_buffer_end(struct push_buffer *pb);

int	nvhost_cdma_init(struct platform_device *pdev,
			 struct nvhost_cdma *cdma);
void	nvhost_cdma_deinit(struct nvhost_cdma *cdma);
void	nvhost_cdma_stop(struct nvhost_cdma *cdma);
int	nvhost_cdma_begin(struct nvhost_cdma *cdma, struct nvhost_job *job);
void	nvhost_cdma_push(struct nvhost_cdma *cdma, u32 op1, u32 op2);
void	nvhost_cdma_push_gather(struct nvhost_cdma *cdma,
		u32 *cpuva, dma_addr_t iova,
		u32 offset, u32 op1, u32 op2);
void	nvhost_cdma_end(struct nvhost_cdma *cdma,
		struct nvhost_job *job);
void	nvhost_cdma_update(struct nvhost_cdma *cdma);
void	nvhost_cdma_peek(struct nvhost_cdma *cdma,
		u32 dmaget, int slot, u32 *out);
unsigned int nvhost_cdma_wait_locked(struct nvhost_cdma *cdma,
		enum cdma_event event);
void nvhost_cdma_finalize_job_incrs(struct platform_device *pdev,
					struct nvhost_job_syncpt *sp);
void nvhost_cdma_update_sync_queue(struct nvhost_cdma *cdma,
		struct nvhost_syncpt *syncpt, struct platform_device *dev);
#endif
