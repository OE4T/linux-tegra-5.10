/*
 * GK20A Graphics
 *
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

#include <nvgpu/nvgpu_common.h>
#include <nvgpu/kmem.h>
#include <nvgpu/allocator.h>
#include <nvgpu/timers.h>
#include <nvgpu/soc.h>
#include <nvgpu/enabled.h>
#include <nvgpu/fifo.h>
#include <nvgpu/acr.h>
#include <nvgpu/pmu.h>
#include <nvgpu/ce.h>
#include <nvgpu/gmmu.h>
#include <nvgpu/ltc.h>
#include <nvgpu/cbc.h>
#include <nvgpu/ecc.h>
#include <nvgpu/vidmem.h>
#include <nvgpu/mm.h>
#include <nvgpu/soc.h>
#include <nvgpu/clk_arb.h>
#include <nvgpu/therm.h>
#include <nvgpu/mc.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/channel_sync.h>
#include <nvgpu/gr/gr.h>

#include <trace/events/gk20a.h>
#include <nvgpu/pmu/pmu_pstate.h>

bool is_nvgpu_gpu_state_valid(struct gk20a *g)
{
	u32 boot_0 = 0xffffffffU;

	boot_0 = nvgpu_mc_boot_0(g, NULL, NULL, NULL);
	if (boot_0 == 0xffffffffU) {
		nvgpu_err(g, "GPU has disappeared from bus!!");
		return false;
	}
	return true;
}

void __nvgpu_check_gpu_state(struct gk20a *g)
{
	if (!is_nvgpu_gpu_state_valid(g)) {
		nvgpu_err(g, "Rebooting system!!");
		nvgpu_kernel_restart(NULL);
	}
}

void __gk20a_warn_on_no_regs(void)
{
	WARN_ONCE(true, "Attempted access to GPU regs after unmapping!");
}

static void gk20a_mask_interrupts(struct gk20a *g)
{
	if (g->ops.mc.intr_mask != NULL) {
		g->ops.mc.intr_mask(g);
	}

	if (g->ops.mc.log_pending_intrs != NULL) {
		g->ops.mc.log_pending_intrs(g);
	}
}

int gk20a_prepare_poweroff(struct gk20a *g)
{
	int tmp_ret, ret = 0;

	nvgpu_log_fn(g, " ");

	if (g->ops.channel.suspend_all_serviceable_ch != NULL) {
		ret = g->ops.channel.suspend_all_serviceable_ch(g);
		if (ret != 0U) {
			return ret;
		}
	}

	/* disable elpg before gr or fifo suspend */
	if (g->support_ls_pmu) {
		ret = nvgpu_pmu_destroy(g);
	}

	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_SEC2_RTOS)) {
		tmp_ret = nvgpu_sec2_destroy(g);
		if ((tmp_ret != 0) && (ret == 0)) {
			ret = tmp_ret;
		}
	}

	tmp_ret = nvgpu_gr_suspend(g);
	if ((tmp_ret != 0) && (ret == 0)) {
		ret = tmp_ret;
	}
	tmp_ret = nvgpu_mm_suspend(g);
	if ((tmp_ret != 0) && (ret == 0)) {
		ret = tmp_ret;
	}
	tmp_ret = gk20a_fifo_suspend(g);
	if ((tmp_ret != 0) && (ret == 0)) {
		ret = tmp_ret;
	}

	nvgpu_falcon_sw_free(g, FALCON_ID_FECS);
	nvgpu_falcon_sw_free(g, FALCON_ID_GSPLITE);
	nvgpu_falcon_sw_free(g, FALCON_ID_NVDEC);
	nvgpu_falcon_sw_free(g, FALCON_ID_SEC2);
	nvgpu_falcon_sw_free(g, FALCON_ID_PMU);

	nvgpu_ce_suspend(g);

	/* Disable GPCPLL */
	if (g->ops.clk.suspend_clk_support != NULL) {
		g->ops.clk.suspend_clk_support(g);
	}
	if (g->ops.clk_arb.stop_clk_arb_threads != NULL) {
		g->ops.clk_arb.stop_clk_arb_threads(g);
	}
	gk20a_mask_interrupts(g);

	g->power_on = false;

	return ret;
}

int gk20a_finalize_poweron(struct gk20a *g)
{
	int err = 0;
#if defined(CONFIG_TEGRA_GK20A_NVHOST)
	u32 nr_pages;
#endif

	nvgpu_log_fn(g, " ");

	if (g->power_on) {
		return 0;
	}

	g->power_on = true;

	/*
	 * Before probing the GPU make sure the GPU's state is cleared. This is
	 * relevant for rebind operations.
	 */
	if ((g->ops.xve.reset_gpu != NULL) && !g->gpu_reset_done) {
		g->ops.xve.reset_gpu(g);
		g->gpu_reset_done = true;
	}

	/*
	 * Do this early so any early VMs that get made are capable of mapping
	 * buffers.
	 */
	err = nvgpu_pd_cache_init(g);
	if (err != 0) {
		return err;
	}

	/* init interface layer support for PMU falcon */
	err = nvgpu_falcon_sw_init(g, FALCON_ID_PMU);
	if (err != 0) {
		nvgpu_err(g, "failed to sw init FALCON_ID_PMU");
		goto exit;
	}
	err = nvgpu_falcon_sw_init(g, FALCON_ID_SEC2);
	if (err != 0) {
		nvgpu_err(g, "failed to sw init FALCON_ID_SEC2");
		goto done_pmu;
	}
	err = nvgpu_falcon_sw_init(g, FALCON_ID_NVDEC);
	if (err != 0) {
		nvgpu_err(g, "failed to sw init FALCON_ID_NVDEC");
		goto done_sec2;
	}
	err = nvgpu_falcon_sw_init(g, FALCON_ID_GSPLITE);
	if (err != 0) {
		nvgpu_err(g, "failed to sw init FALCON_ID_GSPLITE");
		goto done_nvdec;
	}
	err = nvgpu_falcon_sw_init(g, FALCON_ID_FECS);
	if (err != 0) {
		nvgpu_err(g, "failed to sw init FALCON_ID_FECS");
		goto done_gsp;
	}

	err = nvgpu_early_init_pmu_sw(g, &g->pmu);
	if (err != 0) {
		nvgpu_err(g, "failed to early init pmu sw");
		goto done;
	}

	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_SEC2_RTOS)) {
		err = nvgpu_init_sec2_setup_sw(g, &g->sec2);
		if (err != 0) {
			nvgpu_err(g, "failed to init sec2 sw setup");
			goto done;
		}
	}

	if (nvgpu_is_enabled(g, NVGPU_SEC_PRIVSECURITY)) {
		/* Init chip specific ACR properties */
		err = nvgpu_acr_init(g, &g->acr);
		if (err != 0) {
			nvgpu_err(g, "ACR init failed %d", err);
			goto done;
		}
	}

	if (g->ops.bios.init != NULL) {
		err = g->ops.bios.init(g);
	}
	if (err != 0) {
		goto done;
	}

	g->ops.bus.init_hw(g);

	if (g->ops.clk.disable_slowboot != NULL) {
		g->ops.clk.disable_slowboot(g);
	}

	g->ops.priv_ring.enable_priv_ring(g);

	/* TBD: move this after graphics init in which blcg/slcg is enabled.
	   This function removes SlowdownOnBoot which applies 32x divider
	   on gpcpll bypass path. The purpose of slowdown is to save power
	   during boot but it also significantly slows down gk20a init on
	   simulation and emulation. We should remove SOB after graphics power
	   saving features (blcg/slcg) are enabled. For now, do it here. */
	if (g->ops.clk.init_clk_support != NULL) {
		err = g->ops.clk.init_clk_support(g);
		if (err != 0) {
			nvgpu_err(g, "failed to init gk20a clk");
			goto done;
		}
	}

	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_NVLINK)) {
		err = g->ops.nvlink.init(g);
		if (err != 0) {
			nvgpu_err(g, "failed to init nvlink");
			goto done;
		}
	}

	if (g->ops.fb.init_fbpa != NULL) {
		err = g->ops.fb.init_fbpa(g);
		if (err != 0) {
			nvgpu_err(g, "failed to init fbpa");
			goto done;
		}
	}

	if (g->ops.fb.mem_unlock != NULL) {
		err = g->ops.fb.mem_unlock(g);
		if (err != 0) {
			nvgpu_err(g, "failed to unlock memory");
			goto done;
		}
	}

	err = g->ops.fifo.reset_enable_hw(g);

	if (err != 0) {
		nvgpu_err(g, "failed to reset gk20a fifo");
		goto done;
	}

	err = nvgpu_init_ltc_support(g);
	if (err != 0) {
		nvgpu_err(g, "failed to init ltc");
		goto done;
	}

	err = nvgpu_init_mm_support(g);
	if (err != 0) {
		nvgpu_err(g, "failed to init gk20a mm");
		goto done;
	}

	err = nvgpu_fifo_init_support(g);
	if (err != 0) {
		nvgpu_err(g, "failed to init gk20a fifo");
		goto done;
	}

	if (g->ops.therm.elcg_init_idle_filters != NULL) {
		g->ops.therm.elcg_init_idle_filters(g);
	}

	g->ops.mc.intr_enable(g);

	/*
	 *  Overwrite can_tpc_powergate to false if the chip is ES fused and
	 *  already optimized with some TPCs already floorswept
	 *  via fuse. We will not support TPC-PG in those cases.
	 */

	if (g->ops.fuse.fuse_status_opt_tpc_gpc(g, 0) != 0x0U) {
		g->can_tpc_powergate = false;
		g->tpc_pg_mask = 0x0;
	}

	nvgpu_mutex_acquire(&g->tpc_pg_lock);

	if (g->can_tpc_powergate) {
		if (g->ops.gr.powergate_tpc != NULL) {
			g->ops.gr.powergate_tpc(g);
		}
	}

	/* prepare portion of sw required for enable hw */
	err = nvgpu_gr_prepare_sw(g);
	if (err != 0) {
		nvgpu_err(g, "failed to prepare sw");
		nvgpu_mutex_release(&g->tpc_pg_lock);
		goto done;
	}

	err = nvgpu_gr_enable_hw(g);
	if (err != 0) {
		nvgpu_err(g, "failed to enable gr");
		nvgpu_mutex_release(&g->tpc_pg_lock);
		goto done;
	}

	if (nvgpu_is_enabled(g, NVGPU_SEC_PRIVSECURITY)) {
		/* construct ucode blob, load & bootstrap LSF's using HS ACR */
		err = nvgpu_acr_construct_execute(g, g->acr);
		if (err != 0) {
			nvgpu_mutex_release(&g->tpc_pg_lock);
			goto done;
		}
	}

	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_SEC2_RTOS)) {
		err = nvgpu_init_sec2_support(g);
		if (err != 0) {
			nvgpu_err(g, "failed to init sec2");
			nvgpu_mutex_release(&g->tpc_pg_lock);
			goto done;
		}
	}

	err = nvgpu_init_pmu_support(g);
	if (err != 0) {
		nvgpu_err(g, "failed to init gk20a pmu");
		nvgpu_mutex_release(&g->tpc_pg_lock);
		goto done;
	}

	err = nvgpu_gr_init_support(g);
	if (err != 0) {
		nvgpu_err(g, "failed to init gk20a gr");
		nvgpu_mutex_release(&g->tpc_pg_lock);
		goto done;
	}

	err = nvgpu_ecc_init_support(g);
	if (err != 0) {
		nvgpu_err(g, "failed to init ecc");
		nvgpu_mutex_release(&g->tpc_pg_lock);
		goto done;
	}

	nvgpu_mutex_release(&g->tpc_pg_lock);

	if (nvgpu_is_enabled(g, NVGPU_PMU_PSTATE)) {
		err = nvgpu_pmu_pstate_sw_setup(g);
		if (err != 0) {
			nvgpu_err(g, "failed to init pstates");
			nvgpu_mutex_release(&g->tpc_pg_lock);
			goto done;
		}

		err = nvgpu_pmu_pstate_pmu_setup(g);
		if (err != 0) {
			nvgpu_err(g, "failed to init pstates");
			goto done;
		}
	}

	if ((g->pmu.fw.ops.clk.clk_set_boot_clk != NULL) &&
			nvgpu_is_enabled(g, NVGPU_PMU_PSTATE)) {
		g->pmu.fw.ops.clk.clk_set_boot_clk(g);
	} else {
		err = nvgpu_clk_arb_init_arbiter(g);
		if (err != 0) {
			nvgpu_err(g, "failed to init clk arb");
			goto done;
		}
	}

	err = nvgpu_init_therm_support(g);
	if (err != 0) {
		nvgpu_err(g, "failed to init gk20a therm");
		goto done;
	}

	err = nvgpu_cbc_init_support(g);
	if (err != 0) {
		nvgpu_err(g, "failed to init cbc");
		goto done;
	}

	g->ops.chip_init_gpu_characteristics(g);

	/* Restore the debug setting */
	g->ops.fb.set_debug_mode(g, g->mmu_debug_ctrl);

	nvgpu_ce_init_support(g);

	if (g->ops.xve.available_speeds != NULL) {
		u32 speed;

		if (!nvgpu_is_enabled(g, NVGPU_SUPPORT_ASPM) &&
				(g->ops.xve.disable_aspm != NULL)) {
			g->ops.xve.disable_aspm(g);
		}

		g->ops.xve.available_speeds(g, &speed);

		/* Set to max speed */
		speed = BIT32(fls(speed) - 1U);
		err = g->ops.xve.set_speed(g, speed);
		if (err != 0) {
			nvgpu_err(g, "Failed to set PCIe bus speed!");
			goto done;
		}
	}

#if defined(CONFIG_TEGRA_GK20A_NVHOST)
	if (nvgpu_has_syncpoints(g) && g->syncpt_unit_size) {
		if (!nvgpu_mem_is_valid(&g->syncpt_mem)) {
			nr_pages = U32(DIV_ROUND_UP(g->syncpt_unit_size,
						    PAGE_SIZE));
			nvgpu_mem_create_from_phys(g, &g->syncpt_mem,
					g->syncpt_unit_base, nr_pages);
		}
	}
#endif

	if (g->ops.channel.resume_all_serviceable_ch != NULL) {
		g->ops.channel.resume_all_serviceable_ch(g);
	}

	goto exit;

done:
	nvgpu_falcon_sw_free(g, FALCON_ID_FECS);
done_gsp:
	nvgpu_falcon_sw_free(g, FALCON_ID_GSPLITE);
done_nvdec:
	nvgpu_falcon_sw_free(g, FALCON_ID_NVDEC);
done_sec2:
	nvgpu_falcon_sw_free(g, FALCON_ID_SEC2);
done_pmu:
	nvgpu_falcon_sw_free(g, FALCON_ID_PMU);
exit:
	if (err != 0) {
		g->power_on = false;
	}

	return err;
}

/*
 * Check if the device can go busy. Basically if the driver is currently
 * in the process of dying then do not let new places make the driver busy.
 */
int nvgpu_can_busy(struct gk20a *g)
{
	/* Can't do anything if the system is rebooting/shutting down
	 * or the driver is restarting
	 */
	if (nvgpu_is_enabled(g, NVGPU_KERNEL_IS_DYING) ||
		nvgpu_is_enabled(g, NVGPU_DRIVER_IS_DYING)) {
		return 0;
	} else {
		return 1;
	}
}

int gk20a_wait_for_idle(struct gk20a *g)
{
	int wait_length = 150; /* 3 second overall max wait. */
	int target_usage_count = 0;

	if (g == NULL) {
		return -ENODEV;
	}

	while ((nvgpu_atomic_read(&g->usage_count) != target_usage_count)
			&& (wait_length-- >= 0)) {
		nvgpu_msleep(20);
	}

	if (wait_length < 0) {
		nvgpu_warn(g, "Timed out waiting for idle (%d)!\n",
			   nvgpu_atomic_read(&g->usage_count));
		return -ETIMEDOUT;
	}

	return 0;
}

void gk20a_init_gpu_characteristics(struct gk20a *g)
{
#ifdef NVGPU_REDUCED
	nvgpu_set_enabled(g, NVGPU_DRIVER_REDUCED_PROFILE, true);
#endif
	nvgpu_set_enabled(g, NVGPU_SUPPORT_MAP_DIRECT_KIND_CTRL, true);
	nvgpu_set_enabled(g, NVGPU_SUPPORT_MAP_BUFFER_BATCH, true);
	nvgpu_set_enabled(g, NVGPU_SUPPORT_SPARSE_ALLOCS, true);

	/*
	 * Fast submits are supported as long as the user doesn't request
	 * anything that depends on job tracking. (Here, fast means strictly no
	 * metadata, just the gpfifo contents are copied and gp_put updated).
	 */
	nvgpu_set_enabled(g,
			NVGPU_SUPPORT_DETERMINISTIC_SUBMIT_NO_JOBTRACKING,
			true);

	/*
	 * Sync framework requires deferred job cleanup, wrapping syncs in FDs,
	 * and other heavy stuff, which prevents deterministic submits. This is
	 * supported otherwise, provided that the user doesn't request anything
	 * that depends on deferred cleanup.
	 */
	if (!nvgpu_channel_sync_needs_os_fence_framework(g)) {
		nvgpu_set_enabled(g,
				NVGPU_SUPPORT_DETERMINISTIC_SUBMIT_FULL,
				true);
	}

	nvgpu_set_enabled(g, NVGPU_SUPPORT_TSG, true);

	if (g->ops.clk_arb.check_clk_arb_support != NULL) {
		if (g->ops.clk_arb.check_clk_arb_support(g)) {
			nvgpu_set_enabled(g, NVGPU_SUPPORT_CLOCK_CONTROLS,
					true);
		}
	}

	g->ops.gr.init.detect_sm_arch(g);

	if (g->ops.gr.init_cyclestats != NULL) {
		g->ops.gr.init_cyclestats(g);
	}

	g->ops.gr.get_rop_l2_en_mask(g);
}

static struct gk20a *gk20a_from_refcount(struct nvgpu_ref *refcount)
{
	return (struct gk20a *)((uintptr_t)refcount -
				offsetof(struct gk20a, refcount));
}

/*
 * Free the gk20a struct.
 */
static void gk20a_free_cb(struct nvgpu_ref *refcount)
{
	struct gk20a *g = gk20a_from_refcount(refcount);

	nvgpu_log(g, gpu_dbg_shutdown, "Freeing GK20A struct!");

	nvgpu_ce_destroy(g);

	nvgpu_cbc_remove_support(g);

	nvgpu_ecc_remove_support(g);

	if (g->remove_support != NULL) {
		g->remove_support(g);
	}

	nvgpu_ltc_remove_support(g);

	if (g->free != NULL) {
		g->free(g);
	}
}

/**
 * gk20a_get() - Increment ref count on driver
 *
 * @g The driver to increment
 * This will fail if the driver is in the process of being released. In that
 * case it will return NULL. Otherwise a pointer to the driver passed in will
 * be returned.
 */
struct gk20a * __must_check gk20a_get(struct gk20a *g)
{
	int success;

	/*
	 * Handle the possibility we are still freeing the gk20a struct while
	 * gk20a_get() is called. Unlikely but plausible race condition. Ideally
	 * the code will never be in such a situation that this race is
	 * possible.
	 */
	success = nvgpu_ref_get_unless_zero(&g->refcount);

	nvgpu_log(g, gpu_dbg_shutdown, "GET: refs currently %d %s",
		nvgpu_atomic_read(&g->refcount.refcount),
			(success != 0) ? "" : "(FAILED)");

	return (success != 0) ? g : NULL;
}

/**
 * gk20a_put() - Decrement ref count on driver
 *
 * @g - The driver to decrement
 *
 * Decrement the driver ref-count. If neccesary also free the underlying driver
 * memory
 */
void gk20a_put(struct gk20a *g)
{
	/*
	 * Note - this is racy, two instances of this could run before the
	 * actual kref_put(0 runs, you could see something like:
	 *
	 *  ... PUT: refs currently 2
	 *  ... PUT: refs currently 2
	 *  ... Freeing GK20A struct!
	 */
	nvgpu_log(g, gpu_dbg_shutdown, "PUT: refs currently %d",
		nvgpu_atomic_read(&g->refcount.refcount));

	nvgpu_ref_put(&g->refcount, gk20a_free_cb);
}
