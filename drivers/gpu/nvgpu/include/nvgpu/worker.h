/*
 * Copyright (c) 2016-2019, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_WORKER_H
#define NVGPU_WORKER_H

#include <nvgpu/atomic.h>
#include <nvgpu/cond.h>
#include <nvgpu/list.h>
#include <nvgpu/lock.h>
#include <nvgpu/thread.h>
#include <nvgpu/types.h>

struct gk20a;
struct nvgpu_worker;

/*
 * nvgpu_worker is a fifo based produce-consumer worker for the
 * nvgpu driver. Its meant to provide a generic implementation with hooks
 * to allow each application to implement specific use cases for producing
 * and consuming the work. The user of this API enqueues new work and awakens
 * the worker thread. On being awakened, the worker thread checks for pending
 * work or a user provided terminating condition. The generic poll
 * implementation also provides for early terminating conditions as well as
 * pre and post processing hooks. Below is the generic interface of
 * nvgpu_worker consume function.
 *
 * static int nvgpu_worker_poll_work(void *arg)
 * {
 * 	struct nvgpu_worker *worker = (struct nvgpu_worker *)arg;
 * 	int get = 0;
 *
 * 	worker->ops->pre_process(worker);
 *
 * 	while (!nvgpu_thread_should_stop(&worker->poll_task)) {
 * 		int ret;
 *
 * 		ret = NVGPU_COND_WAIT_INTERRUPTIBLE(
 * 				&worker->wq,
 * 				nvgpu_worker_pending(worker, get) ||
 * 				worker->ops->wakeup_condition(worker),
 * 				worker->ops->wakeup_timeout(worker));
 *
 * 		if (worker->ops->wakeup_early_exit(worker)) {
 * 			break;
 * 		}
 *
 * 		if (ret == 0) {
 * 			worker->ops->process_item(worker, &get);
 * 		}
 *
 * 		worker->ops->post_process(worker);
 * 	}
 * 	return 0;
 * }
 */

struct nvgpu_worker_ops {
	/*
	 * This interface is used to pass any callback to be invoked the first
	 * time the background thread is launched.
	 */
	void (*pre_process)(struct nvgpu_worker *worker);

	/*
	 * This interface is used to pass any callback for early terminating
	 * the worker thread after the thread has been awakened.
	 */
	bool (*wakeup_early_exit)(struct nvgpu_worker *worker);

	/*
	 * This interface is used to pass any post processing callback for
	 * the worker thread after wakeup. The worker thread executes this
	 * callback everytime before sleeping again.
	 */
	void (*wakeup_post_process)(struct nvgpu_worker *worker);

	/*
	 * This interface is used to handle each of the individual work_item
	 * just after the background thread has been awakened. This should
	 * always point to a valid callback function.
	 */
	void (*wakeup_process_item)(struct nvgpu_list_node *work_item);

	/*
	 * Any additional condition that is 'OR'ed with worker_pending_items
	 */
	bool (*wakeup_condition)(struct nvgpu_worker *worker);

	/*
	 * Used to pass any timeout condition for wakeup
	 */
	u32 (*wakeup_timeout)(struct nvgpu_worker *worker);
};

struct nvgpu_worker {
	struct gk20a *g;
	char thread_name[64];
	nvgpu_atomic_t put;
	struct nvgpu_thread poll_task;
	struct nvgpu_cond wq;
	struct nvgpu_list_node items;
	struct nvgpu_spinlock items_lock;
	struct nvgpu_mutex start_lock;
	const struct nvgpu_worker_ops *ops;
};

bool nvgpu_worker_should_stop(struct nvgpu_worker *worker);

/*
 * Append a work item to the worker's list.
 *
 * This adds work item to the end of the list and wakes the worker
 * up immediately. If the work item already existed in the list, it's not added,
 * because in that case it has been scheduled already but has not yet been
 * processed.
 */
int nvgpu_worker_enqueue(struct nvgpu_worker *worker,
		struct nvgpu_list_node *work_item);

/*
 * This API is used to initialize the worker with a name thats a conjunction of
 * the two parameters.{worker_name}_{gpu_name}
 */
void nvgpu_worker_init_name(struct nvgpu_worker *worker,
		const char* worker_name, const char *gpu_name);

/*
 * Initialize the worker's metadata and start the background thread.
 */
int nvgpu_worker_init(struct gk20a *g, struct nvgpu_worker *worker,
		const struct nvgpu_worker_ops *ops);
void nvgpu_worker_deinit(struct nvgpu_worker *worker);

#endif /* NVGPU_WORKER_H */