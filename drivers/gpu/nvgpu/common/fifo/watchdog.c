/*
 * Copyright (c) 2011-2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/gk20a.h>
#include <nvgpu/channel.h>
#include <nvgpu/watchdog.h>
#include <nvgpu/error_notifier.h>
#include <nvgpu/watchdog.h>

struct nvgpu_channel_wdt {
	struct gk20a *g;

	/* lock protects the running timer state */
	struct nvgpu_spinlock lock;
	struct nvgpu_timeout timer;
	bool running;
	u32 gp_get;
	u64 pb_get;

	/* lock not needed */
	u32 limit_ms;
	bool enabled;
	bool debug_dump;
};

struct nvgpu_channel_wdt *nvgpu_channel_wdt_alloc(struct nvgpu_channel *ch)
{
	struct gk20a *g = ch->g;
	struct nvgpu_channel_wdt *wdt = nvgpu_kzalloc(g, sizeof(*wdt));

	if (wdt == NULL) {
		return NULL;
	}

	wdt->g = g;
	nvgpu_spinlock_init(&wdt->lock);
	wdt->enabled = true;
	wdt->limit_ms = g->ch_wdt_init_limit_ms;
	wdt->debug_dump = true;

	return wdt;
}

void nvgpu_channel_wdt_destroy(struct nvgpu_channel_wdt *wdt)
{
	nvgpu_kfree(wdt->g, wdt);
}

void nvgpu_channel_wdt_enable(struct nvgpu_channel_wdt *wdt)
{
	wdt->enabled = true;
}

void nvgpu_channel_wdt_disable(struct nvgpu_channel_wdt *wdt)
{
	wdt->enabled = false;
}

bool nvgpu_channel_wdt_enabled(struct nvgpu_channel_wdt *wdt)
{
	return wdt->enabled;
}

void nvgpu_channel_wdt_set_limit(struct nvgpu_channel_wdt *wdt, u32 limit_ms)
{
	wdt->limit_ms = limit_ms;
}

u32 nvgpu_channel_wdt_limit(struct nvgpu_channel_wdt *wdt)
{
	return wdt->limit_ms;
}

void nvgpu_channel_wdt_set_debug_dump(struct nvgpu_channel_wdt *wdt, bool dump)
{
	wdt->debug_dump = dump;
}

static void nvgpu_channel_wdt_init(struct nvgpu_channel_wdt *wdt,
		struct nvgpu_channel *ch)
{
	struct gk20a *g = wdt->g;
	int ret;

	if (nvgpu_channel_check_unserviceable(ch)) {
		wdt->running = false;
		return;
	}

	ret = nvgpu_timeout_init(g, &wdt->timer,
			   wdt->limit_ms,
			   NVGPU_TIMER_CPU_TIMER);
	if (ret != 0) {
		nvgpu_err(g, "timeout_init failed: %d", ret);
		return;
	}

	wdt->gp_get = g->ops.userd.gp_get(g, ch);
	wdt->pb_get = g->ops.userd.pb_get(g, ch);
	wdt->running = true;
}

/**
 * Start a timeout counter (watchdog) on this channel.
 *
 * Trigger a watchdog to recover the channel after the per-platform timeout
 * duration (but strictly no earlier) if the channel hasn't advanced within
 * that time.
 *
 * If the timeout is already running, do nothing. This should be called when
 * new jobs are submitted. The timeout will stop when the last tracked job
 * finishes, making the channel idle.
 *
 * The channel's gpfifo read pointer will be used to determine if the job has
 * actually stuck at that time. After the timeout duration has expired, a
 * worker thread will consider the channel stuck and recover it if stuck.
 */
void nvgpu_channel_wdt_start(struct nvgpu_channel_wdt *wdt,
		struct nvgpu_channel *ch)
{
	if (!nvgpu_is_timeouts_enabled(wdt->g)) {
		return;
	}

	if (!wdt->enabled) {
		return;
	}

	nvgpu_spinlock_acquire(&wdt->lock);

	if (wdt->running) {
		nvgpu_spinlock_release(&wdt->lock);
		return;
	}
	nvgpu_channel_wdt_init(wdt, ch);
	nvgpu_spinlock_release(&wdt->lock);
}

/**
 * Stop a running timeout counter (watchdog) on this channel.
 *
 * Make the watchdog consider the channel not running, so that it won't get
 * recovered even if no progress is detected. Progress is not tracked if the
 * watchdog is turned off.
 *
 * No guarantees are made about concurrent execution of the timeout handler.
 * (This should be called from an update handler running in the same thread
 * with the watchdog.)
 */
bool nvgpu_channel_wdt_stop(struct nvgpu_channel_wdt *wdt)
{
	bool was_running;

	nvgpu_spinlock_acquire(&wdt->lock);
	was_running = wdt->running;
	wdt->running = false;
	nvgpu_spinlock_release(&wdt->lock);
	return was_running;
}

/**
 * Continue a previously stopped timeout
 *
 * Enable the timeout again but don't reinitialize its timer.
 *
 * No guarantees are made about concurrent execution of the timeout handler.
 * (This should be called from an update handler running in the same thread
 * with the watchdog.)
 */
void nvgpu_channel_wdt_continue(struct nvgpu_channel_wdt *wdt)
{
	nvgpu_spinlock_acquire(&wdt->lock);
	wdt->running = true;
	nvgpu_spinlock_release(&wdt->lock);
}

/**
 * Reset the counter of a timeout that is in effect.
 *
 * If this channel has an active timeout, act as if something happened on the
 * channel right now.
 *
 * Rewinding a stopped counter is irrelevant; this is a no-op for non-running
 * timeouts. Stopped timeouts can only be started (which is technically a
 * rewind too) or continued (where the stop is actually pause).
 */
static void nvgpu_channel_wdt_rewind(struct nvgpu_channel_wdt *wdt,
		struct nvgpu_channel *ch)
{
	nvgpu_spinlock_acquire(&wdt->lock);
	if (wdt->running) {
		nvgpu_channel_wdt_init(wdt, ch);
	}
	nvgpu_spinlock_release(&wdt->lock);
}

/**
 * Rewind the timeout on each non-dormant channel.
 *
 * Reschedule the timeout of each active channel for which timeouts are running
 * as if something was happened on each channel right now. This should be
 * called when a global hang is detected that could cause a false positive on
 * other innocent channels.
 */
void nvgpu_channel_wdt_restart_all_channels(struct gk20a *g)
{
	struct nvgpu_fifo *f = &g->fifo;
	u32 chid;

	for (chid = 0; chid < f->num_channels; chid++) {
		struct nvgpu_channel *ch = nvgpu_channel_from_id(g, chid);

		if (ch != NULL) {
			if (!nvgpu_channel_check_unserviceable(ch)) {
				nvgpu_channel_wdt_rewind(ch->wdt, ch);
			}
			nvgpu_channel_put(ch);
		}
	}
}

/**
 * Check if a timed out channel has hung and recover it if it has.
 *
 * Test if this channel has really got stuck at this point by checking if its
 * {gp,pb}_get has advanced or not. If no {gp,pb}_get action happened since
 * when the watchdog was started and it's timed out, force-reset the channel.
 *
 * The gpu is implicitly on at this point, because the watchdog can only run on
 * channels that have submitted jobs pending for cleanup.
 */
static void nvgpu_channel_wdt_handler(struct nvgpu_channel_wdt *wdt,
		struct nvgpu_channel *ch)
{
	struct gk20a *g = wdt->g;
	u32 gp_get;
	u32 new_gp_get;
	u64 pb_get;
	u64 new_pb_get;

	nvgpu_log_fn(g, " ");

	if (nvgpu_channel_check_unserviceable(ch)) {
		/* channel is already recovered */
		if (nvgpu_channel_wdt_stop(wdt) == true) {
			nvgpu_info(g, "chid: %d unserviceable but wdt was ON",
			ch->chid);
		}
		return;
	}

	/* Get status but keep timer running */
	nvgpu_spinlock_acquire(&wdt->lock);
	gp_get = wdt->gp_get;
	pb_get = wdt->pb_get;
	nvgpu_spinlock_release(&wdt->lock);

	new_gp_get = g->ops.userd.gp_get(g, ch);
	new_pb_get = g->ops.userd.pb_get(g, ch);

	if (new_gp_get != gp_get || new_pb_get != pb_get) {
		/* Channel has advanced, timer keeps going but resets */
		nvgpu_channel_wdt_rewind(wdt, ch);
	} else if (!nvgpu_timeout_peek_expired(&wdt->timer)) {
		/* Seems stuck but waiting to time out */
	} else {
		nvgpu_err(g, "Job on channel %d timed out", ch->chid);

		/* force reset calls gk20a_debug_dump but not this */
		if (wdt->debug_dump) {
			gk20a_gr_debug_dump(g);
		}

#ifdef CONFIG_NVGPU_CHANNEL_TSG_CONTROL
		if (g->ops.tsg.force_reset(ch,
		    NVGPU_ERR_NOTIFIER_FIFO_ERROR_IDLE_TIMEOUT,
		    wdt->debug_dump) != 0) {
			nvgpu_err(g, "failed tsg force reset for chid: %d",
				ch->chid);
		}
#endif
	}
}

/**
 * Test if the per-channel watchdog is on; check the timeout in that case.
 *
 * Each channel has an expiration time based watchdog. The timer is
 * (re)initialized in two situations: when a new job is submitted on an idle
 * channel and when the timeout is checked but progress is detected. The
 * watchdog timeout limit is a coarse sliding window.
 *
 * The timeout is stopped (disabled) after the last job in a row finishes
 * and marks the channel idle.
 */
void nvgpu_channel_wdt_check(struct nvgpu_channel_wdt *wdt,
		struct nvgpu_channel *ch)
{
	bool running;

	nvgpu_spinlock_acquire(&wdt->lock);
	running = wdt->running;
	nvgpu_spinlock_release(&wdt->lock);

	if (running) {
		nvgpu_channel_wdt_handler(wdt, ch);
	}
}
