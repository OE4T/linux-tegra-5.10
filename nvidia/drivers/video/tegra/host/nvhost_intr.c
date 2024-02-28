/*
 * drivers/video/tegra/host/nvhost_intr.c
 *
 * Tegra Graphics Host Interrupt Management
 *
 * Copyright (c) 2010-2020, NVIDIA CORPORATION. All rights reserved.
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

#include "nvhost_intr.h"
#include "dev.h"
#include "nvhost_acm.h"

#ifdef CONFIG_TEGRA_GRHOST_SYNC
#include "nvhost_sync.h"
#endif

#if IS_ENABLED(CONFIG_SYNC_FILE) && !IS_ENABLED(CONFIG_SYNC)
#include <linux/dma-fence.h>
#endif

#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <trace/events/nvhost.h>

#include "nvhost_channel.h"
#include "chip_support.h"

/*** Wait list management ***/

enum waitlist_state {
	WLS_PENDING,
	WLS_REMOVED,
	WLS_CANCELLED,
	WLS_HANDLED,
	WLS_CLEANUP
};

static inline bool __maybe_unused
nvhost_intr_is_virtual_dev(struct nvhost_intr_syncpt *sp)
{
	struct nvhost_intr *intr = intr_syncpt_to_intr(sp);
	struct nvhost_master *host = intr_to_dev(intr);

	return nvhost_dev_is_virtual(host->dev);
}

static void waiter_release(struct kref *kref)
{
	struct nvhost_waitlist *waiter =
		container_of(kref, struct nvhost_waitlist, refcount);

	nvhost_module_idle(waiter->host->dev);
	kfree(waiter);
}

int nvhost_intr_release_time(void *ref, struct nvhost_timespec *ts)
{
	struct nvhost_waitlist *waiter = ref;
	if (atomic_read(&waiter->state) == WLS_PENDING)
		return -EBUSY;

	*ts = waiter->isr_recv;
	return 0;
}

/**
 * add a waiter to a waiter queue, sorted by threshold
 * returns true if it was added at the head of the queue
 */
static bool add_waiter_to_queue(struct nvhost_waitlist *waiter,
				struct list_head *queue)
{
	struct nvhost_waitlist *pos;
	u32 thresh = waiter->thresh;

	list_for_each_entry_reverse(pos, queue, list)
		if ((s32)(pos->thresh - thresh) <= 0) {
			list_add(&waiter->list, &pos->list);
			return false;
		}

	list_add(&waiter->list, queue);
	return true;
}

/**
 * run through a waiter queue for a single sync point ID
 * and gather all completed waiters into lists by actions
 */
static void remove_completed_waiters(struct list_head *head, u32 sync,
			struct nvhost_timespec isr_recv,
			struct list_head *completed[NVHOST_INTR_ACTION_COUNT])
{
	struct list_head *dest;
	struct nvhost_waitlist *waiter, *next, *prev;

	list_for_each_entry_safe(waiter, next, head, list) {
		bool removed = false;

		if ((s32)(waiter->thresh - sync) > 0)
			break;

		waiter->isr_recv = isr_recv;
		dest = *(completed + waiter->action);

		/* consolidate submit cleanups */
		if (waiter->action == NVHOST_INTR_ACTION_SUBMIT_COMPLETE
			&& !list_empty(dest)) {
			prev = list_entry(dest->prev,
					struct nvhost_waitlist, list);
			if (prev->data == waiter->data) {
				prev->count++;
				removed = true;
			}
		}

		/* CANCELLED->HANDLED or PENDING->REMOVED
		 * Change state to CLEANUP, which will be deferred until
		 * action handlers are run; add the waiter to the head of the
		 * queue, so cleanup will precede any real work.
		 */
		if ((atomic_inc_return(&waiter->state) == WLS_HANDLED)
								|| removed) {
			atomic_set(&waiter->state, WLS_CLEANUP);
			list_move(&waiter->list, dest);
		} else
			list_move_tail(&waiter->list, dest);
	}
}

static void reset_threshold_interrupt(struct nvhost_intr *intr,
			       struct list_head *head,
			       unsigned int id)
{
	u32 thresh = list_first_entry(head,
				struct nvhost_waitlist, list)->thresh;

	intr_op().set_syncpt_threshold(intr, id, thresh);
	intr_op().enable_syncpt_intr(intr, id);
}


static void action_submit_complete(struct nvhost_waitlist *waiter)
{
	struct nvhost_channel *channel;
	int nr_completed;

	if (!waiter) {
		pr_warn("%s: Empty Waiter\n", __func__);
		return;
	}

	nr_completed = waiter->count;
	channel = waiter->data;
	if (!channel || !channel->dev) {
		pr_warn("%s: Channel un-mapped\n", __func__);
		return;
	}

	nvhost_cdma_update(&channel->cdma);
	nvhost_module_idle_mult(channel->dev, nr_completed);

	/*  Add nr_completed to trace */
	trace_nvhost_channel_submit_complete(channel->dev->name,
			nr_completed, waiter->thresh);

	nvhost_putchannel(channel, nr_completed);

}

static void action_wakeup(struct nvhost_waitlist *waiter)
{
	wait_queue_head_t *wq = &waiter->wq;

	WARN_ON(atomic_xchg(&waiter->state, WLS_HANDLED) != WLS_REMOVED);
	wake_up(wq);
}

static void action_notify(struct nvhost_waitlist *waiter)
{
	struct nvhost_waitlist_external_notifier *notifier = waiter->data;
	struct nvhost_master *master = notifier->master;

	notifier->callback(notifier->private_data, waiter->count);

	nvhost_module_idle_mult(master->dev, waiter->count);
	if (!notifier->reuse)
		kfree(notifier);
	waiter->data = NULL;
}

static void action_wakeup_interruptible(struct nvhost_waitlist *waiter)
{
	wait_queue_head_t *wq = &waiter->wq;

	WARN_ON(atomic_xchg(&waiter->state, WLS_HANDLED) != WLS_REMOVED);
	wake_up_interruptible(wq);
}

static void action_signal_sync_pt(struct nvhost_waitlist *waiter)
{
#ifdef CONFIG_TEGRA_GRHOST_SYNC
#if IS_ENABLED(CONFIG_SYNC)
	struct nvhost_sync_pt *pt = waiter->data;
	ktime_t time = timespec_to_ktime(waiter->isr_recv.ts);
	nvhost_sync_pt_signal(pt, ktime_to_ns(time));
#elif IS_ENABLED(CONFIG_SYNC_FILE)
	struct dma_fence *fence = waiter->data;
	dma_fence_signal(fence);
#endif
#endif
}

typedef void (*action_handler)(struct nvhost_waitlist *waiter);

static action_handler action_handlers[NVHOST_INTR_ACTION_COUNT] = {
	action_signal_sync_pt,
	action_wakeup,
	action_wakeup_interruptible,
	action_notify,
	action_submit_complete,
	action_notify,
};

static void run_handlers(struct list_head *completed[NVHOST_INTR_ACTION_COUNT])
{
	int i;

	for (i = 0; i < NVHOST_INTR_ACTION_COUNT; ++i) {
		struct list_head *head = completed[i];
		action_handler handler = action_handlers[i];
		struct nvhost_waitlist *waiter, *next;

		if (!head)
			continue;

		list_for_each_entry_safe(waiter, next, head, list) {
			list_del(&waiter->list);
			/* already processed, just need to finish cleanup */
			if (atomic_read(&waiter->state) == WLS_CLEANUP) {
				goto cleanup;
			}
			handler(waiter);
			if (handler != action_wakeup_interruptible &&
			    handler != action_wakeup)
				WARN_ON(atomic_xchg(&waiter->state, WLS_HANDLED)
					!= WLS_REMOVED);

cleanup:
			kref_put(&waiter->refcount, waiter_release);
		}
	}
}

/**
 * Remove & handle all waiters that have completed for the given syncpt
 */
static int process_wait_list(struct nvhost_intr *intr,
			     struct nvhost_intr_syncpt *syncpt,
			     u32 threshold)
{
	struct list_head *completed[NVHOST_INTR_ACTION_COUNT] = {NULL};
	struct list_head high_prio_handlers[NVHOST_INTR_HIGH_PRIO_COUNT];
	bool run_low_prio_work = false;
	unsigned int i, j;
	int empty;

	/* take lock on waiter list */
	spin_lock(&syncpt->lock);

	/* keep high priority workers in local list */
	for (i = 0; i < NVHOST_INTR_HIGH_PRIO_COUNT; ++i) {
		INIT_LIST_HEAD(high_prio_handlers + i);
		completed[i] = high_prio_handlers + i;
	}

	/* .. and low priority workers in global list */
	for (j = 0; i < NVHOST_INTR_ACTION_COUNT; ++i, ++j)
		completed[i] = syncpt->low_prio_handlers + j;

	/* this functions fills completed data */
	remove_completed_waiters(&syncpt->wait_head, threshold,
		syncpt->isr_recv, completed);

	/* check if there are still waiters left */
	empty = list_empty(&syncpt->wait_head);

	/* if not, disable interrupt. If yes, update the inetrrupt */
	if (empty)
		intr_op().disable_syncpt_intr(intr, syncpt->id);
	else
		reset_threshold_interrupt(intr, &syncpt->wait_head,
					  syncpt->id);

	/* remove low priority handlers from this list */
	for (i = NVHOST_INTR_HIGH_PRIO_COUNT;
	     i < NVHOST_INTR_ACTION_COUNT; ++i) {
		if (!list_empty(completed[i]))
			run_low_prio_work = true;
		completed[i] = NULL;
	}

	/* release waiter lock */
	spin_unlock(&syncpt->lock);

	run_handlers(completed);

	/* schedule a separate task to handle low priority handlers */
	if (run_low_prio_work)
		queue_work(intr->low_prio_wq, &syncpt->low_prio_work);

	return empty;
}

static void nvhost_syncpt_low_prio_work(struct work_struct *work)
{
	struct nvhost_intr_syncpt *syncpt = container_of(work,
						     struct nvhost_intr_syncpt,
						     low_prio_work);
	struct list_head *completed[NVHOST_INTR_ACTION_COUNT] = {NULL};
	struct list_head low_prio_handlers[NVHOST_INTR_LOW_PRIO_COUNT];
	unsigned int i, j;

	/* go through low priority handlers.. */
	spin_lock(&syncpt->lock);
	for (i = 0, j = NVHOST_INTR_HIGH_PRIO_COUNT;
	     j < NVHOST_INTR_ACTION_COUNT;
	     i++, j++) {
		struct list_head *handler = low_prio_handlers + i;

		/* move entries from low priority queue into local queue */
		INIT_LIST_HEAD(handler);
		list_cut_position(handler,
				  &syncpt->low_prio_handlers[i],
				  syncpt->low_prio_handlers[i].prev);

		/* maintain local completed list */
		completed[j] = handler;
	}
	spin_unlock(&syncpt->lock);

	/* ..and run them */
	run_handlers(completed);
}

/*** host syncpt interrupt service functions ***/
void nvhost_syncpt_thresh_fn(void *dev_id)
{
	struct nvhost_intr_syncpt *syncpt = dev_id;
	unsigned int id = syncpt->id;
	struct nvhost_intr *intr = intr_syncpt_to_intr(syncpt);
	struct nvhost_master *dev = intr_to_dev(intr);
	int err;

	/* make sure host1x is powered */
	err = nvhost_module_busy(dev->dev);
	if (err) {
		WARN(1, "failed to powerON host1x.");
		return;
	}

	(void)process_wait_list(intr, syncpt,
				nvhost_syncpt_update_min(&dev->syncpt, id));

	nvhost_module_idle(dev->dev);
}

/*** host general interrupt service functions ***/


/*** Main API ***/


bool nvhost_intr_has_pending_jobs(struct nvhost_intr *intr, u32 id,
			void *exclude_data)
{
	struct nvhost_intr_syncpt *syncpt;
	struct nvhost_waitlist *waiter;
	bool res = false;

	syncpt = intr->syncpt + id;
	spin_lock(&syncpt->lock);
	list_for_each_entry(waiter, &syncpt->wait_head, list)
		if (((waiter->action ==
			NVHOST_INTR_ACTION_SUBMIT_COMPLETE) &&
			(waiter->data != exclude_data))) {
			res = true;
			break;
		}

	spin_unlock(&syncpt->lock);

	return res;
}

int nvhost_intr_add_action(struct nvhost_intr *intr, u32 id, u32 thresh,
			enum nvhost_intr_action action, void *data,
			void *_waiter,
			void **ref)
{
	struct nvhost_waitlist *waiter = _waiter;
	struct nvhost_intr_syncpt *syncpt;
	int queue_was_empty;
	int err;

	if (waiter == NULL) {
		pr_warn("%s: NULL waiter\n", __func__);
		return -EINVAL;
	}

	/* make sure host1x stays on */
	err = nvhost_module_busy(intr_to_dev(intr)->dev);
	if (err)
		return err;

	/* initialize a new waiter */
	INIT_LIST_HEAD(&waiter->list);
	init_waitqueue_head(&waiter->wq);
	kref_init(&waiter->refcount);
	if (ref)
		kref_get(&waiter->refcount);
	waiter->thresh = thresh;
	waiter->action = action;
	atomic_set(&waiter->state, WLS_PENDING);
	waiter->data = data;
	waiter->count = 1;
	waiter->host = intr_to_dev(intr);

	syncpt = intr->syncpt + id;

	spin_lock(&syncpt->lock);

	queue_was_empty = list_empty(&syncpt->wait_head);

	if (add_waiter_to_queue(waiter, &syncpt->wait_head)) {
		/* added at head of list - new threshold value */
		intr_op().set_syncpt_threshold(intr, id, thresh);

		/* added as first waiter - enable interrupt */
		if (queue_was_empty)
			intr_op().enable_syncpt_intr(intr, id);
	}

	spin_unlock(&syncpt->lock);

	if (ref)
		*ref = waiter;
	return 0;
}

void *nvhost_intr_alloc_waiter(void)
{
	return kzalloc(sizeof(struct nvhost_waitlist),
			GFP_KERNEL);
}

static int __nvhost_intr_register_notifier(struct platform_device *pdev,
				  u32 id, u32 thresh,
				  enum nvhost_intr_action action,
				  void (*callback)(void *, int),
				  void *private_data)
{
	struct nvhost_waitlist *waiter;
	struct nvhost_waitlist_external_notifier *notifier;
	struct nvhost_master *master = nvhost_get_host(pdev);
	int err = 0;

	if (!callback)
		return -EINVAL;

	waiter = kzalloc(sizeof(*waiter), GFP_KERNEL);
	if (!waiter) {
		nvhost_err(&pdev->dev, "failed to allocate waiter");
		err = -ENOMEM;
		goto err_alloc_waiter;
	}
	notifier = kzalloc(sizeof(*notifier), GFP_KERNEL);
	if (!notifier) {
		nvhost_err(&pdev->dev, "failed to allocate notifier");
		err = -ENOMEM;
		goto err_alloc_notifier;
	}

	notifier->master = master;
	notifier->callback = callback;
	notifier->private_data = private_data;

	/* make sure host1x stays on */
	err = nvhost_module_busy(master->dev);
	if (err)
		goto err_busy;

	err = nvhost_intr_add_action(&master->intr,
				     id, thresh,
				     action,
				     notifier,
				     waiter,
				     NULL);

	return err;

err_busy:
	kfree(notifier);
err_alloc_notifier:
	kfree(waiter);
err_alloc_waiter:
	return err;
}

int nvhost_intr_register_notifier(struct platform_device *pdev,
				  u32 id, u32 thresh,
				  void (*callback)(void *, int),
				  void *private_data)
{
	return __nvhost_intr_register_notifier(pdev, id, thresh,
				  NVHOST_INTR_ACTION_NOTIFY,
				  callback, private_data);
}
EXPORT_SYMBOL(nvhost_intr_register_notifier);

int nvhost_intr_register_fast_notifier(struct platform_device *pdev,
				  u32 id, u32 thresh,
				  void (*callback)(void *, int),
				  void *private_data)
{
	return __nvhost_intr_register_notifier(pdev, id, thresh,
				  NVHOST_INTR_ACTION_FAST_NOTIFY,
				  callback, private_data);
}
EXPORT_SYMBOL(nvhost_intr_register_fast_notifier);

void nvhost_intr_put_ref(struct nvhost_intr *intr, u32 id, void *ref)
{
	struct nvhost_waitlist *waiter = ref;
	struct nvhost_intr_syncpt *syncpt;
	struct nvhost_master *host = intr_to_dev(intr);

	while (atomic_cmpxchg(&waiter->state,
				WLS_PENDING, WLS_CANCELLED) == WLS_REMOVED)
		schedule();

	syncpt = intr->syncpt + id;
	(void)process_wait_list(intr, syncpt,
				nvhost_syncpt_update_min(&host->syncpt, id));

	kref_put(&waiter->refcount, waiter_release);
}


/*** Init & shutdown ***/

int nvhost_intr_init(struct nvhost_intr *intr, u32 irq_gen, u32 irq_sync[8])
{
	unsigned int id, i, err;
	struct nvhost_intr_syncpt *syncpt;
	struct nvhost_master *host = intr_to_dev(intr);
	u32 nb_pts = nvhost_syncpt_nb_hw_pts(&host->syncpt);
	u32 nb_syncpt_irqs = nvhost_syncpt_nb_irqs(&host->syncpt);
	u32 pts_per_irq = nb_pts / nb_syncpt_irqs;

	mutex_init(&intr->mutex);
	intr->general_irq = irq_gen;
	memcpy(intr->syncpt_irqs, irq_sync, 8*sizeof(u32));
	for (i = 0; i < nb_syncpt_irqs; i++) {
		intr->syncpt_irq_ctx[i].start_id = i * pts_per_irq;
		intr->syncpt_irq_ctx[i].end_id = (i + 1) * pts_per_irq - 1;
		intr->syncpt_irq_ctx[i].dev = host;
	}

	if (intr->syncpt_irq_ctx[nb_syncpt_irqs-1].end_id != (nb_pts - 1)) {
		nvhost_dbg_info(
		    "additional %d syncpoints from %d are mapped to last irq",
		    (nb_pts - 1) -
			intr->syncpt_irq_ctx[nb_syncpt_irqs - 1].end_id,
		    intr->syncpt_irq_ctx[nb_syncpt_irqs - 1].end_id + 1);
		intr->syncpt_irq_ctx[nb_syncpt_irqs - 1].end_id = nb_pts - 1;
	}

	intr->low_prio_wq = create_singlethread_workqueue("host_low_prio_wq");
	if (!intr->low_prio_wq) {
		nvhost_err(&host->dev->dev,
			   "failed to create low prio waitqueue");
		return -EINVAL;
	}

	for (id = 0, syncpt = intr->syncpt;
	     id < nb_pts;
	     ++id, ++syncpt) {
		syncpt->intr = &host->intr;
		syncpt->id = id;
		spin_lock_init(&syncpt->lock);
		INIT_LIST_HEAD(&syncpt->wait_head);
		snprintf(syncpt->thresh_irq_name,
			sizeof(syncpt->thresh_irq_name),
			"host_sp_%02d", id);
		for (i = 0; i < NVHOST_INTR_LOW_PRIO_COUNT; ++i)
			INIT_LIST_HEAD(syncpt->low_prio_handlers + i);
		INIT_WORK(&syncpt->low_prio_work,
			  nvhost_syncpt_low_prio_work);
	}

	err = intr_op().init(intr);
	if (err) {
		destroy_workqueue(intr->low_prio_wq);
		return err;
	}

	return 0;
}

void nvhost_intr_deinit(struct nvhost_intr *intr)
{
	nvhost_intr_stop(intr);
	intr_op().deinit(intr);
	destroy_workqueue(intr->low_prio_wq);
}

int nvhost_intr_start(struct nvhost_intr *intr, u32 hz)
{
	int err = 0;

	mutex_lock(&intr->mutex);

	intr_op().resume(intr);
	intr_op().set_host_clocks_per_usec(intr, (hz + 1000000 - 1)/1000000);

	mutex_unlock(&intr->mutex);
	return err;
}

int nvhost_intr_stop(struct nvhost_intr *intr)
{
	unsigned int id;
	struct nvhost_intr_syncpt *syncpt;
	u32 nb_pts = nvhost_syncpt_nb_hw_pts(&intr_to_dev(intr)->syncpt);

	mutex_lock(&intr->mutex);

	for (id = 0, syncpt = intr->syncpt;
	     id < nb_pts;
	     ++id, ++syncpt) {
		struct nvhost_waitlist *waiter, *next;

		intr_op().disable_syncpt_intr(intr, id);

		list_for_each_entry_safe(waiter, next, &syncpt->wait_head, list) {
			if (atomic_cmpxchg(&waiter->state, WLS_CANCELLED, WLS_HANDLED)
				== WLS_CANCELLED) {
				list_del(&waiter->list);
				kref_put(&waiter->refcount, waiter_release);
			}
		}

		if (!list_empty(&syncpt->wait_head)) {  /* output diagnostics */
			intr_op().enable_syncpt_intr(intr, id);
			mutex_unlock(&intr->mutex);
			return -EBUSY;
		}
	}

	intr_op().suspend(intr);

	mutex_unlock(&intr->mutex);

	return 0;
}

void nvhost_intr_enable_host_irq(struct nvhost_intr *intr, int irq,
				 void (*host_isr)(u32, void *),
				 void *priv)
{
	if (!irq)
		return;

	intr->host_isr[irq] = host_isr;
	intr->host_isr_priv[irq] = priv;
	intr_op().enable_host_irq(intr, irq);
}

void nvhost_intr_disable_host_irq(struct nvhost_intr *intr, int irq)
{
	if (!irq)
		return;

	intr_op().disable_host_irq(intr, irq);
	intr->host_isr[irq] = NULL;
	intr->host_isr_priv[irq] = NULL;
}
