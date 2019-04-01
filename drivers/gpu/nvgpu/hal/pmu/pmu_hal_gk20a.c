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

void gk20a_pmu_dump_elpg_stats(struct nvgpu_pmu *pmu)
{
	struct gk20a *g = gk20a_from_pmu(pmu);

	nvgpu_pmu_dbg(g, "pwr_pmu_idle_mask_supp_r(3): 0x%08x",
		gk20a_readl(g, pwr_pmu_idle_mask_supp_r(3)));
	nvgpu_pmu_dbg(g, "pwr_pmu_idle_mask_1_supp_r(3): 0x%08x",
		gk20a_readl(g, pwr_pmu_idle_mask_1_supp_r(3)));
	nvgpu_pmu_dbg(g, "pwr_pmu_idle_ctrl_supp_r(3): 0x%08x",
		gk20a_readl(g, pwr_pmu_idle_ctrl_supp_r(3)));
	nvgpu_pmu_dbg(g, "pwr_pmu_pg_idle_cnt_r(0): 0x%08x",
		gk20a_readl(g, pwr_pmu_pg_idle_cnt_r(0)));
	nvgpu_pmu_dbg(g, "pwr_pmu_pg_intren_r(0): 0x%08x",
		gk20a_readl(g, pwr_pmu_pg_intren_r(0)));

	nvgpu_pmu_dbg(g, "pwr_pmu_idle_count_r(3): 0x%08x",
		gk20a_readl(g, pwr_pmu_idle_count_r(3)));
	nvgpu_pmu_dbg(g, "pwr_pmu_idle_count_r(4): 0x%08x",
		gk20a_readl(g, pwr_pmu_idle_count_r(4)));
	nvgpu_pmu_dbg(g, "pwr_pmu_idle_count_r(7): 0x%08x",
		gk20a_readl(g, pwr_pmu_idle_count_r(7)));
}

void gk20a_pmu_dump_falcon_stats(struct nvgpu_pmu *pmu)
{
	struct gk20a *g = gk20a_from_pmu(pmu);
	unsigned int i;

	for (i = 0; i < pwr_pmu_mailbox__size_1_v(); i++) {
		nvgpu_err(g, "pwr_pmu_mailbox_r(%d) : 0x%x",
			i, gk20a_readl(g, pwr_pmu_mailbox_r(i)));
	}

	for (i = 0; i < pwr_pmu_debug__size_1_v(); i++) {
		nvgpu_err(g, "pwr_pmu_debug_r(%d) : 0x%x",
			i, gk20a_readl(g, pwr_pmu_debug_r(i)));
	}

	i = gk20a_readl(g, pwr_pmu_bar0_error_status_r());
	nvgpu_err(g, "pwr_pmu_bar0_error_status_r : 0x%x", i);
	if (i != 0U) {
		nvgpu_err(g, "pwr_pmu_bar0_addr_r : 0x%x",
			gk20a_readl(g, pwr_pmu_bar0_addr_r()));
		nvgpu_err(g, "pwr_pmu_bar0_data_r : 0x%x",
			gk20a_readl(g, pwr_pmu_bar0_data_r()));
		nvgpu_err(g, "pwr_pmu_bar0_timeout_r : 0x%x",
			gk20a_readl(g, pwr_pmu_bar0_timeout_r()));
		nvgpu_err(g, "pwr_pmu_bar0_ctl_r : 0x%x",
			gk20a_readl(g, pwr_pmu_bar0_ctl_r()));
	}

	i = gk20a_readl(g, pwr_pmu_bar0_fecs_error_r());
	nvgpu_err(g, "pwr_pmu_bar0_fecs_error_r : 0x%x", i);

	i = gk20a_readl(g, pwr_falcon_exterrstat_r());
	nvgpu_err(g, "pwr_falcon_exterrstat_r : 0x%x", i);
	if (pwr_falcon_exterrstat_valid_v(i) ==
			pwr_falcon_exterrstat_valid_true_v()) {
		nvgpu_err(g, "pwr_falcon_exterraddr_r : 0x%x",
			gk20a_readl(g, pwr_falcon_exterraddr_r()));
	}
}
