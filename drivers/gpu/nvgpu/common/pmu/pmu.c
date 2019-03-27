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

#include <nvgpu/pmu.h>
#include <nvgpu/pmu/pmu_pg.h>
#include <nvgpu/dma.h>
#include <nvgpu/log.h>
#include <nvgpu/pmuif/nvgpu_gpmu_cmdif.h>
#include <nvgpu/pmuif/gpmu_super_surf_if.h>
#include <nvgpu/enabled.h>
#include <nvgpu/engine_queue.h>
#include <nvgpu/barrier.h>
#include <nvgpu/timers.h>
#include <nvgpu/bug.h>
#include <nvgpu/utils.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/string.h>
#include <nvgpu/power_features/cg.h>
#include <nvgpu/nvgpu_err.h>

static void pmu_report_error(struct gk20a *g, u32 err_type,
		u32 status, u32 pmu_err_type)
{
	int ret = 0;

	if (g->ops.pmu.err_ops.report_pmu_err != NULL) {
		ret = g->ops.pmu.err_ops.report_pmu_err(g,
			NVGPU_ERR_MODULE_PWR, err_type, status, pmu_err_type);
		if (ret != 0) {
			nvgpu_err(g, "Failed to report PMU error: %d",
					err_type);
		}
	}
}

static int pmu_enable_hw(struct nvgpu_pmu *pmu, bool enable)
{
	struct gk20a *g = pmu->g;
	int err = 0;

	nvgpu_log_fn(g, " %s ", g->name);

	if (enable) {
		/* bring PMU falcon/engine out of reset */
		g->ops.pmu.reset_engine(g, true);

		nvgpu_cg_slcg_pmu_load_enable(g);

		nvgpu_cg_blcg_pmu_load_enable(g);

		if (nvgpu_falcon_mem_scrub_wait(&pmu->flcn) != 0) {
			/* keep PMU falcon/engine in reset
			 * if IMEM/DMEM scrubbing fails
			 */
			g->ops.pmu.reset_engine(g, false);
			nvgpu_err(g, "Falcon mem scrubbing timeout");
			err = -ETIMEDOUT;
		}
	} else {
		/* keep PMU falcon/engine in reset */
		g->ops.pmu.reset_engine(g, false);
	}

	nvgpu_log_fn(g, "%s Done, status - %d ", g->name, err);
	return err;
}

static int pmu_enable(struct nvgpu_pmu *pmu, bool enable)
{
	struct gk20a *g = pmu->g;
	int err = 0;

	nvgpu_log_fn(g, " ");

	if (!enable) {
		if (!g->ops.pmu.is_engine_in_reset(g)) {
			g->ops.pmu.pmu_enable_irq(pmu, false);
			pmu_enable_hw(pmu, false);
		}
	} else {
		err = pmu_enable_hw(pmu, true);
		if (err != 0) {
			goto exit;
		}

		err = nvgpu_falcon_wait_idle(&pmu->flcn);
		if (err != 0) {
			goto exit;
		}
	}

exit:
	nvgpu_log_fn(g, "Done, status - %d ", err);
	return err;
}

int nvgpu_pmu_reset(struct gk20a *g)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	int err = 0;

	nvgpu_log_fn(g, " %s ", g->name);

	err = nvgpu_falcon_wait_idle(&pmu->flcn);
	if (err != 0) {
		goto exit;
	}

	err = pmu_enable(pmu, false);
	if (err != 0) {
		goto exit;
	}

	err = pmu_enable(pmu, true);

exit:
	nvgpu_log_fn(g, " %s Done, status - %d ", g->name, err);
	return err;
}

static int nvgpu_init_pmu_setup_sw(struct gk20a *g)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	struct mm_gk20a *mm = &g->mm;
	struct vm_gk20a *vm = mm->pmu.vm;
	unsigned int i;
	int err = 0;
	u8 *ptr;

	nvgpu_log_fn(g, " ");

	/* start with elpg disabled until first enable call */
	pmu->pmu_pg.elpg_refcnt = 0;

	/* Create thread to handle PMU state machine */
	nvgpu_init_task_pg_init(g);
	if (pmu->sw_ready) {
		for (i = 0; i < pmu->mutex_cnt; i++) {
			pmu->mutex[i].id    = i;
			pmu->mutex[i].index = i;
		}
		nvgpu_pmu_seq_init(pmu);

		nvgpu_log_fn(g, "skip init");
		goto skip_init;
	}

	/* no infoRom script from vbios? */

	/* TBD: sysmon subtask */

	pmu->mutex_cnt = g->ops.pmu.pmu_mutex_size();
	pmu->mutex = nvgpu_kzalloc(g, pmu->mutex_cnt *
		sizeof(struct pmu_mutex));
	if (pmu->mutex == NULL) {
		err = -ENOMEM;
		goto err;
	}

	for (i = 0; i < pmu->mutex_cnt; i++) {
		pmu->mutex[i].id    = i;
		pmu->mutex[i].index = i;
	}

	pmu->seq = nvgpu_kzalloc(g, PMU_MAX_NUM_SEQUENCES *
		sizeof(struct pmu_sequence));
	if (pmu->seq == NULL) {
		err = -ENOMEM;
		goto err_free_mutex;
	}

	nvgpu_pmu_seq_init(pmu);

	err = nvgpu_dma_alloc_map_sys(vm, GK20A_PMU_SEQ_BUF_SIZE,
			&pmu->seq_buf);
	if (err != 0) {
		nvgpu_err(g, "failed to allocate memory");
		goto err_free_seq;
	}

	ptr = (u8 *)pmu->seq_buf.cpu_va;

	/* TBD: remove this if ZBC save/restore is handled by PMU
	 * end an empty ZBC sequence for now
	 */
	ptr[0] = 0x16; /* opcode EXIT */
	ptr[1] = 0; ptr[2] = 1; ptr[3] = 0;
	ptr[4] = 0; ptr[5] = 0; ptr[6] = 0; ptr[7] = 0;

	pmu->seq_buf.size = GK20A_PMU_SEQ_BUF_SIZE;

	if (g->ops.pmu.alloc_super_surface != NULL) {
		err = g->ops.pmu.alloc_super_surface(g,
				&pmu->super_surface_buf,
				sizeof(struct nv_pmu_super_surface));
		if (err != 0) {
			goto err_free_seq_buf;
		}
	}

	err = nvgpu_dma_alloc_map(vm, GK20A_PMU_TRACE_BUFSIZE,
			&pmu->trace_buf);
	if (err != 0) {
		nvgpu_err(g, "failed to allocate pmu trace buffer\n");
		goto err_free_super_surface;
	}

	pmu->sw_ready = true;

skip_init:
	nvgpu_log_fn(g, "done");
	return 0;
 err_free_super_surface:
	if (g->ops.pmu.alloc_super_surface != NULL) {
 		 nvgpu_dma_unmap_free(vm, &pmu->super_surface_buf);
	}
 err_free_seq_buf:
	nvgpu_dma_unmap_free(vm, &pmu->seq_buf);
 err_free_seq:
	nvgpu_kfree(g, pmu->seq);
 err_free_mutex:
	nvgpu_kfree(g, pmu->mutex);
 err:
	nvgpu_log_fn(g, "fail");
	return err;
}

int nvgpu_init_pmu_support(struct gk20a *g)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	int err = 0;

	nvgpu_log_fn(g, " ");

	if (pmu->pmu_pg.initialized) {
		return 0;
	}

	if (!g->support_ls_pmu) {
		goto exit;
	}

	err = nvgpu_init_pmu_setup_sw(g);
	if (err != 0) {
		goto exit;
	}

	if (nvgpu_is_enabled(g, NVGPU_SEC_PRIVSECURITY)) {

		if (nvgpu_is_enabled(g, NVGPU_SUPPORT_SEC2_RTOS)) {
			/* Reset PMU engine */
			err = nvgpu_falcon_reset(&g->pmu.flcn);

			/* Bootstrap PMU from SEC2 RTOS*/
			err = nvgpu_sec2_bootstrap_ls_falcons(g, &g->sec2,
				FALCON_ID_PMU);
			if (err != 0) {
				goto exit;
			}
		}

		/*
		 * clear halt interrupt to avoid PMU-RTOS ucode
		 * hitting breakpoint due to PMU halt
		 */
		err = nvgpu_falcon_clear_halt_intr_status(&g->pmu.flcn,
			nvgpu_get_poll_timeout(g));
		if (err != 0) {
			goto exit;
		}

		if (g->ops.pmu.setup_apertures != NULL) {
			g->ops.pmu.setup_apertures(g);
		}

		if (g->ops.pmu.update_lspmu_cmdline_args != NULL) {
			g->ops.pmu.update_lspmu_cmdline_args(g);
		}

		if (g->ops.pmu.pmu_enable_irq != NULL) {
			nvgpu_mutex_acquire(&g->pmu.isr_mutex);
			g->ops.pmu.pmu_enable_irq(&g->pmu, true);
			g->pmu.isr_enabled = true;
			nvgpu_mutex_release(&g->pmu.isr_mutex);
		}

		/*Once in LS mode, cpuctl_alias is only accessible*/
		if (g->ops.pmu.secured_pmu_start != NULL) {
			g->ops.pmu.secured_pmu_start(g);
		}
	} else {
		/* prepare blob for non-secure PMU boot */
		err = nvgpu_pmu_prepare_ns_ucode_blob(g);

		/* Do non-secure PMU boot */
		err = g->ops.pmu.pmu_setup_hw_and_bootstrap(g);
		if (err != 0) {
			goto exit;
		}
	}

	nvgpu_pmu_state_change(g, PMU_STATE_STARTING, false);

exit:
	return err;
}

static void pmu_read_init_msg_fb(struct gk20a *g, struct nvgpu_pmu *pmu,
	u32 element_index, u32 size, void *buffer)
{
	u32 fbq_msg_queue_ss_offset = 0U;

	fbq_msg_queue_ss_offset = (u32)offsetof(
		struct nv_pmu_super_surface,
		fbq.msg_queue.element[element_index]);

	nvgpu_mem_rd_n(g, &pmu->super_surface_buf, fbq_msg_queue_ss_offset,
		buffer, size);
}

static int pmu_process_init_msg_fb(struct gk20a *g, struct nvgpu_pmu *pmu,
	struct pmu_msg *msg)
{
	u32 tail = 0U;
	int err = 0;

	nvgpu_log_fn(g, " ");

	g->ops.pmu.pmu_msgq_tail(pmu, &tail, QUEUE_GET);

	pmu_read_init_msg_fb(g, pmu, tail, PMU_MSG_HDR_SIZE,
		(void *)&msg->hdr);

	if (msg->hdr.unit_id != PMU_UNIT_INIT) {
		nvgpu_err(g, "FB MSG Q: expecting init msg");
		err = -EINVAL;
		goto exit;
	}

	pmu_read_init_msg_fb(g, pmu, tail, msg->hdr.size,
		(void *)&msg->hdr);

	if (msg->msg.init.msg_type != PMU_INIT_MSG_TYPE_PMU_INIT) {
		nvgpu_err(g, "FB MSG Q: expecting pmu init msg");
		err = -EINVAL;
		goto exit;
	}

	/* Queue is not yet constructed, so inline next element code here.*/
	tail++;
	if (tail >= NV_PMU_FBQ_MSG_NUM_ELEMENTS) {
		tail = 0U;
	}

	g->ops.pmu.pmu_msgq_tail(pmu, &tail, QUEUE_SET);

exit:
	return err;
}

static int pmu_process_init_msg_dmem(struct gk20a *g, struct nvgpu_pmu *pmu,
	struct pmu_msg *msg)
{
	u32 tail = 0U;
	int err = 0;

	nvgpu_log_fn(g, " ");

	g->ops.pmu.pmu_msgq_tail(pmu, &tail, QUEUE_GET);

	err = nvgpu_falcon_copy_from_dmem(&pmu->flcn, tail,
		(u8 *)&msg->hdr, PMU_MSG_HDR_SIZE, 0);
	if (err != 0) {
		nvgpu_err(g, "PMU falcon DMEM copy failed");
		goto exit;
	}
	if (msg->hdr.unit_id != PMU_UNIT_INIT) {
		nvgpu_err(g, "expecting init msg");
		err = -EINVAL;
		goto exit;
	}

	err = nvgpu_falcon_copy_from_dmem(&pmu->flcn, tail + PMU_MSG_HDR_SIZE,
		(u8 *)&msg->msg, (u32)msg->hdr.size - PMU_MSG_HDR_SIZE, 0);
	if (err != 0) {
		nvgpu_err(g, "PMU falcon DMEM copy failed");
		goto exit;
	}

	if (msg->msg.init.msg_type != PMU_INIT_MSG_TYPE_PMU_INIT) {
		nvgpu_err(g, "expecting pmu init msg");
		err = -EINVAL;
		goto exit;
	}

	tail += ALIGN(msg->hdr.size, PMU_DMEM_ALIGNMENT);
	g->ops.pmu.pmu_msgq_tail(pmu, &tail, QUEUE_SET);

exit:
	return err;
}

int nvgpu_pmu_process_init_msg(struct nvgpu_pmu *pmu,
			struct pmu_msg *msg)
{
	struct gk20a *g = gk20a_from_pmu(pmu);
	struct pmu_v *pv = &g->ops.pmu_ver;
	union pmu_init_msg_pmu *init;
	struct pmu_sha1_gid_data gid_data;
	int err = 0;
	u32 i = 0U;
	u32 j = 0U;

	nvgpu_log_fn(g, " ");

	nvgpu_pmu_dbg(g, "init received\n");

	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_PMU_RTOS_FBQ)) {
		err = pmu_process_init_msg_fb(g, pmu, msg);
	} else {
		err = pmu_process_init_msg_dmem(g, pmu, msg);
	}

	/* error check for above init message process*/
	if (err != 0) {
		goto exit;
	}

	init = pv->get_pmu_msg_pmu_init_msg_ptr(&(msg->msg.init));
	if (!pmu->gid_info.valid) {
		u32 *gid_hdr_data = (u32 *)(gid_data.signature);

		err = nvgpu_falcon_copy_from_dmem(&pmu->flcn,
			pv->get_pmu_init_msg_pmu_sw_mg_off(init),
			(u8 *)&gid_data,
			(u32)sizeof(struct pmu_sha1_gid_data), 0);
		if (err != 0) {
			nvgpu_err(g, "PMU falcon DMEM copy failed");
			goto exit;
		}

		pmu->gid_info.valid =
			(*gid_hdr_data == PMU_SHA1_GID_SIGNATURE);

		if (pmu->gid_info.valid) {

			BUG_ON(sizeof(pmu->gid_info.gid) !=
				sizeof(gid_data.gid));

			nvgpu_memcpy((u8 *)pmu->gid_info.gid,
				(u8 *)gid_data.gid,
				sizeof(pmu->gid_info.gid));
		}
	}


	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_PMU_RTOS_FBQ)) {
		pmu->queue_type = QUEUE_TYPE_FB;
		for (i = 0; i < PMU_QUEUE_COUNT; i++) {
			err = nvgpu_pmu_queue_init_fb(pmu, i, init);
			if (err != 0) {
				for (j = 0; j < i; j++) {
					nvgpu_pmu_queue_free(pmu, j);
				}
				nvgpu_err(g, "PMU queue init failed");
				return err;
			}
		}
	} else {
		pmu->queue_type = QUEUE_TYPE_DMEM;
		for (i = 0; i < PMU_QUEUE_COUNT; i++) {
			err = nvgpu_pmu_queue_init(pmu, i, init);
			if (err != 0) {
				for (j = 0; j < i; j++) {
					nvgpu_pmu_queue_free(pmu, j);
				}
				nvgpu_err(g, "PMU queue init failed");
				return err;
			}
		}
	}

	if (!nvgpu_alloc_initialized(&pmu->dmem)) {
		/* Align start and end addresses */
		u32 start = ALIGN(pv->get_pmu_init_msg_pmu_sw_mg_off(init),
			PMU_DMEM_ALLOC_ALIGNMENT);
		u32 end = (U32(pv->get_pmu_init_msg_pmu_sw_mg_off(init)) +
			U32(pv->get_pmu_init_msg_pmu_sw_mg_size(init))) &
			~(PMU_DMEM_ALLOC_ALIGNMENT - 1U);
		u32 size = end - start;

		nvgpu_bitmap_allocator_init(g, &pmu->dmem, "gk20a_pmu_dmem",
			start, size, PMU_DMEM_ALLOC_ALIGNMENT, 0);
	}

	if (g->ops.pmu.create_ssmd_lookup_table != NULL) {
		g->ops.pmu.create_ssmd_lookup_table(pmu);
	}

	pmu->pmu_ready = true;

	nvgpu_pmu_state_change(g, PMU_STATE_INIT_RECEIVED, true);
exit:
	nvgpu_pmu_dbg(g, "init received end, err %x", err);
	return err;
}

void nvgpu_pmu_state_change(struct gk20a *g, u32 pmu_state,
		bool post_change_event)
{
	struct nvgpu_pmu *pmu = &g->pmu;

	nvgpu_pmu_dbg(g, "pmu_state - %d", pmu_state);

	pmu->pmu_state = pmu_state;

	if (post_change_event) {
		pmu->pmu_pg.pg_init.state_change = true;
		nvgpu_cond_signal(&pmu->pmu_pg.pg_init.wq);
	}

	/* make status visible */
	nvgpu_smp_mb();
}

int nvgpu_pmu_destroy(struct gk20a *g)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	struct pmu_pg_stats_data pg_stat_data = { 0 };
	u32 i;

	nvgpu_log_fn(g, " ");

	if (!g->support_ls_pmu) {
		return 0;
	}
	nvgpu_kill_task_pg_init(g);

	nvgpu_pmu_get_pg_stats(g,
		PMU_PG_ELPG_ENGINE_ID_GRAPHICS,	&pg_stat_data);

	if (nvgpu_pmu_disable_elpg(g) != 0) {
		nvgpu_err(g, "failed to set disable elpg");
	}
	pmu->pmu_pg.initialized = false;

	/* update the s/w ELPG residency counters */
	g->pg_ingating_time_us += (u64)pg_stat_data.ingating_time;
	g->pg_ungating_time_us += (u64)pg_stat_data.ungating_time;
	g->pg_gating_cnt += pg_stat_data.gating_cnt;

	nvgpu_mutex_acquire(&pmu->isr_mutex);
	g->ops.pmu.pmu_enable_irq(pmu, false);
	pmu->isr_enabled = false;
	nvgpu_mutex_release(&pmu->isr_mutex);

	for (i = 0U; i < PMU_QUEUE_COUNT; i++) {
		nvgpu_pmu_queue_free(pmu, i);
	}

	nvgpu_pmu_state_change(g, PMU_STATE_OFF, false);
	pmu->pmu_ready = false;
	pmu->perfmon_ready = false;
	pmu->pmu_pg.zbc_ready = false;
	g->pmu_lsf_pmu_wpr_init_done = false;
	nvgpu_set_enabled(g, NVGPU_PMU_FECS_BOOTSTRAP_DONE, false);

	nvgpu_log_fn(g, "done");
	return 0;
}

void nvgpu_pmu_surface_describe(struct gk20a *g, struct nvgpu_mem *mem,
		struct flcn_mem_desc_v0 *fb)
{
	fb->address.lo = u64_lo32(mem->gpu_va);
	fb->address.hi = u64_hi32(mem->gpu_va);
	fb->params = ((u32)mem->size & 0xFFFFFFU);
	fb->params |= (GK20A_PMU_DMAIDX_VIRT << 24U);
}

int nvgpu_pmu_vidmem_surface_alloc(struct gk20a *g, struct nvgpu_mem *mem,
		u32 size)
{
	struct mm_gk20a *mm = &g->mm;
	struct vm_gk20a *vm = mm->pmu.vm;
	int err;

	err = nvgpu_dma_alloc_map_vid(vm, size, mem);
	if (err != 0) {
		nvgpu_err(g, "memory allocation failed");
		return -ENOMEM;
	}

	return 0;
}

int nvgpu_pmu_sysmem_surface_alloc(struct gk20a *g, struct nvgpu_mem *mem,
		u32 size)
{
	struct mm_gk20a *mm = &g->mm;
	struct vm_gk20a *vm = mm->pmu.vm;
	int err;

	err = nvgpu_dma_alloc_map_sys(vm, size, mem);
	if (err != 0) {
		nvgpu_err(g, "failed to allocate memory\n");
		return -ENOMEM;
	}

	return 0;
}

struct gk20a *gk20a_from_pmu(struct nvgpu_pmu *pmu)
{
	return pmu->g;
}

int nvgpu_pmu_wait_ready(struct gk20a *g)
{
	int status = 0;

	status = pmu_wait_message_cond_status(&g->pmu,
		nvgpu_get_poll_timeout(g),
		&g->pmu.pmu_ready, (u8)true);
	if (status != 0) {
		nvgpu_err(g, "PMU is not ready yet");
	}

	return status;
}

void nvgpu_pmu_get_cmd_line_args_offset(struct gk20a *g,
	u32 *args_offset)
{
	struct nvgpu_pmu *pmu = &g->pmu;
	u32 dmem_size = 0;
	int err = 0;

	err = nvgpu_falcon_get_mem_size(&pmu->flcn, MEM_DMEM, &dmem_size);
	if (err != 0) {
		nvgpu_err(g, "dmem size request failed");
		*args_offset = 0;
		return;
	}

	*args_offset = dmem_size - g->ops.pmu_ver.get_pmu_cmdline_args_size(pmu);
}

void nvgpu_pmu_report_bar0_pri_err_status(struct gk20a *g, u32 bar0_status,
	u32 error_type)
{
	pmu_report_error(g,
		GPU_PMU_BAR0_ERROR_TIMEOUT, bar0_status, error_type);
	return;
}
