/*
 * Copyright (c) 2011-2019, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/pmu/mutex.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/io.h>

#include <nvgpu/hw/gk20a/hw_pwr_gk20a.h>

#include "pmu_hal_gk20a.h"

u32 gk20a_pmu_mutex_owner(struct gk20a *g, struct pmu_mutexes *mutexes, u32 id)
{
	struct pmu_mutex *mutex;

	mutex = &mutexes->mutex[id];

	return pwr_pmu_mutex_value_v(
		gk20a_readl(g, pwr_pmu_mutex_r(mutex->index)));
}

int gk20a_pmu_mutex_acquire(struct gk20a *g, struct pmu_mutexes *mutexes,
			    u32 id, u32 *token)
{
	struct pmu_mutex *mutex;
	u32 data, owner, max_retry;
	int ret = -EBUSY;

	mutex = &mutexes->mutex[id];

	owner = pwr_pmu_mutex_value_v(
		gk20a_readl(g, pwr_pmu_mutex_r(mutex->index)));

	max_retry = 40;
	do {
		data = pwr_pmu_mutex_id_value_v(
			gk20a_readl(g, pwr_pmu_mutex_id_r()));
		if (data == pwr_pmu_mutex_id_value_init_v() ||
		    data == pwr_pmu_mutex_id_value_not_avail_v()) {
			nvgpu_warn(g,
				"fail to generate mutex token: val 0x%08x",
				owner);
			nvgpu_usleep_range(20, 40);
			continue;
		}

		owner = data;
		gk20a_writel(g, pwr_pmu_mutex_r(mutex->index),
			pwr_pmu_mutex_value_f(owner));

		data = pwr_pmu_mutex_value_v(
			gk20a_readl(g, pwr_pmu_mutex_r(mutex->index)));

		if (owner == data) {
			nvgpu_log_info(g, "mutex acquired: id=%d, token=0x%x",
				mutex->index, *token);
			*token = owner;
			ret = 0;
			break;
		}

		nvgpu_log_info(g, "fail to acquire mutex idx=0x%08x",
			mutex->index);

		data = gk20a_readl(g, pwr_pmu_mutex_id_release_r());
		data = set_field(data,
			pwr_pmu_mutex_id_release_value_m(),
			pwr_pmu_mutex_id_release_value_f(owner));
		gk20a_writel(g, pwr_pmu_mutex_id_release_r(), data);

		nvgpu_usleep_range(20, 40);
	} while (max_retry-- > 0U);

	return ret;
}

void gk20a_pmu_mutex_release(struct gk20a *g, struct pmu_mutexes *mutexes,
			     u32 id, u32 *token)
{
	struct pmu_mutex *mutex;
	u32 owner, data;

	mutex = &mutexes->mutex[id];

	owner = pwr_pmu_mutex_value_v(
		gk20a_readl(g, pwr_pmu_mutex_r(mutex->index)));

	gk20a_writel(g, pwr_pmu_mutex_r(mutex->index),
		pwr_pmu_mutex_value_initial_lock_f());

	data = gk20a_readl(g, pwr_pmu_mutex_id_release_r());
	data = set_field(data, pwr_pmu_mutex_id_release_value_m(),
		pwr_pmu_mutex_id_release_value_f(owner));
	gk20a_writel(g, pwr_pmu_mutex_id_release_r(), data);

	nvgpu_log_info(g, "mutex released: id=%d, token=0x%x",
		mutex->index, *token);
}
