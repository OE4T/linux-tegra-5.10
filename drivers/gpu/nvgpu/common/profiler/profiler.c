/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/pm_reservation.h>
#include <nvgpu/profiler.h>
#include <nvgpu/atomic.h>
#include <nvgpu/log.h>
#include <nvgpu/lock.h>
#include <nvgpu/kmem.h>
#include <nvgpu/tsg.h>
#include <nvgpu/nvgpu_init.h>
#include <nvgpu/gr/ctx.h>
#include <nvgpu/perfbuf.h>

static nvgpu_atomic_t unique_id = NVGPU_ATOMIC_INIT(0);
static int generate_unique_id(void)
{
	return nvgpu_atomic_add_return(1, &unique_id);
}

int nvgpu_profiler_alloc(struct gk20a *g,
	struct nvgpu_profiler_object **_prof,
	enum nvgpu_profiler_pm_reservation_scope scope)
{
	struct nvgpu_profiler_object *prof;
	*_prof = NULL;

	nvgpu_log(g, gpu_dbg_prof, " ");

	prof = nvgpu_kzalloc(g, sizeof(*prof));
	if (prof == NULL) {
		return -ENOMEM;
	}

	prof->prof_handle = generate_unique_id();
	prof->scope = scope;
	prof->g = g;

	nvgpu_mutex_init(&prof->ioctl_lock);
	nvgpu_init_list_node(&prof->prof_obj_entry);
	nvgpu_list_add(&prof->prof_obj_entry, &g->profiler_objects);

	nvgpu_log(g, gpu_dbg_prof, "Allocated profiler handle %u",
		prof->prof_handle);

	*_prof = prof;
	return 0;
}

void nvgpu_profiler_free(struct nvgpu_profiler_object *prof)
{
	struct gk20a *g = prof->g;

	nvgpu_log(g, gpu_dbg_prof, "Free profiler handle %u",
		prof->prof_handle);

	nvgpu_profiler_unbind_context(prof);
	nvgpu_profiler_free_pma_stream(prof);

	nvgpu_list_del(&prof->prof_obj_entry);
	nvgpu_kfree(g, prof);
}

int nvgpu_profiler_bind_context(struct nvgpu_profiler_object *prof,
		struct nvgpu_tsg *tsg)
{
	struct gk20a *g = prof->g;

	nvgpu_log(g, gpu_dbg_prof, "Request to bind tsgid %u with profiler handle %u",
		tsg->tsgid, prof->prof_handle);

	if (tsg->prof != NULL) {
		nvgpu_err(g, "TSG %u is already bound", tsg->tsgid);
		return -EINVAL;
	}

	if (prof->tsg != NULL) {
		nvgpu_err(g, "Profiler object %u already bound!", prof->prof_handle);
		return -EINVAL;
	}

	prof->tsg = tsg;
	tsg->prof = prof;

	nvgpu_log(g, gpu_dbg_prof, "Bind tsgid %u with profiler handle %u successful",
		tsg->tsgid, prof->prof_handle);

	prof->context_init = true;
	return 0;
}

int nvgpu_profiler_unbind_context(struct nvgpu_profiler_object *prof)
{
	struct gk20a *g = prof->g;
	struct nvgpu_tsg *tsg = prof->tsg;
	int i;

	if (prof->bound) {
		nvgpu_warn(g, "Unbinding resources for handle %u",
			prof->prof_handle);
		nvgpu_profiler_unbind_pm_resources(prof);
	}

	for (i = 0; i < NVGPU_PROFILER_PM_RESOURCE_TYPE_COUNT; i++) {
		if (prof->reserved[i]) {
			nvgpu_warn(g, "Releasing reserved resource %u for handle %u",
				i, prof->prof_handle);
			nvgpu_profiler_pm_resource_release(prof, i);
		}
	}

	if (!prof->context_init) {
		return -EINVAL;
	}

	if (tsg != NULL) {
		tsg->prof = NULL;
		prof->tsg = NULL;

		nvgpu_log(g, gpu_dbg_prof, "Unbind profiler handle %u and tsgid %u",
			prof->prof_handle, tsg->tsgid);
	}

	prof->context_init = false;
	return 0;
}

int nvgpu_profiler_pm_resource_reserve(struct nvgpu_profiler_object *prof,
	enum nvgpu_profiler_pm_resource_type pm_resource)
{
	struct gk20a *g = prof->g;
	enum nvgpu_profiler_pm_reservation_scope scope = prof->scope;
	u32 reservation_id = prof->prof_handle;
	int err;

	nvgpu_log(g, gpu_dbg_prof,
		"Request reservation for profiler handle %u, resource %u, scope %u",
		prof->prof_handle, pm_resource, prof->scope);

	if (prof->reserved[pm_resource]) {
		nvgpu_err(g, "Profiler handle %u already has the reservation",
			prof->prof_handle);
		return -EEXIST;
	}

	if (prof->bound) {
		nvgpu_log(g, gpu_dbg_prof,
			"PM resources alredy bound with profiler handle %u,"
			" unbinding for new reservation",
			prof->prof_handle);
		err = nvgpu_profiler_unbind_pm_resources(prof);
		if (err != 0) {
			nvgpu_err(g, "Profiler handle %u failed to unbound, err %d",
				prof->prof_handle, err);
			return err;
		}
	}

	err = g->ops.pm_reservation.acquire(g, reservation_id, pm_resource,
			scope, 0);
	if (err != 0) {
		nvgpu_err(g, "Profiler handle %u denied the reservation, err %d",
			prof->prof_handle, err);
		return err;
	}

	prof->reserved[pm_resource] = true;

	nvgpu_log(g, gpu_dbg_prof,
		"Granted reservation for profiler handle %u, resource %u, scope %u",
		prof->prof_handle, pm_resource, prof->scope);

	return 0;
}

int nvgpu_profiler_pm_resource_release(struct nvgpu_profiler_object *prof,
	enum nvgpu_profiler_pm_resource_type pm_resource)
{
	struct gk20a *g = prof->g;
	u32 reservation_id = prof->prof_handle;
	int err;

	nvgpu_log(g, gpu_dbg_prof,
		"Release reservation for profiler handle %u, resource %u, scope %u",
		prof->prof_handle, pm_resource, prof->scope);

	if (!prof->reserved[pm_resource]) {
		nvgpu_log(g, gpu_dbg_prof,
			"Profiler handle %u resource is not reserved",
			prof->prof_handle);
		return -EINVAL;
	}

	if (prof->bound) {
		nvgpu_log(g, gpu_dbg_prof,
			"PM resources alredy bound with profiler handle %u,"
			" unbinding for reservation release",
			prof->prof_handle);
		err = nvgpu_profiler_unbind_pm_resources(prof);
		if (err != 0) {
			nvgpu_err(g, "Profiler handle %u failed to unbound, err %d",
				prof->prof_handle, err);
			return err;
		}
	}

	err = g->ops.pm_reservation.release(g, reservation_id, pm_resource, 0);
	if (err != 0) {
		nvgpu_err(g, "Profiler handle %u does not have valid reservation, err %d",
			prof->prof_handle, err);
		prof->reserved[pm_resource] = false;
		return err;
	}

	prof->reserved[pm_resource] = false;

	nvgpu_log(g, gpu_dbg_prof,
		"Released reservation for profiler handle %u, resource %u, scope %u",
		prof->prof_handle, pm_resource, prof->scope);

	return 0;
}

static int nvgpu_profiler_bind_smpc(struct nvgpu_profiler_object *prof)
{
	struct gk20a *g = prof->g;
	int err;

	if (prof->scope == NVGPU_PROFILER_PM_RESERVATION_SCOPE_DEVICE) {
		if (prof->ctxsw[NVGPU_PROFILER_PM_RESOURCE_TYPE_SMPC]) {
			err = g->ops.gr.update_smpc_ctxsw_mode(g, prof->tsg, true);
			if (err != 0) {
				return err;
			}

			if (nvgpu_is_enabled(g, NVGPU_SUPPORT_SMPC_GLOBAL_MODE)) {
				err = g->ops.gr.update_smpc_global_mode(g, false);
			}
		} else {
			if (nvgpu_is_enabled(g, NVGPU_SUPPORT_SMPC_GLOBAL_MODE)) {
				err = g->ops.gr.update_smpc_global_mode(g, true);
			} else {
				err = -EINVAL;
			}
		}
	} else {
		err = g->ops.gr.update_smpc_ctxsw_mode(g, prof->tsg, true);
	}

	return err;
}

static int nvgpu_profiler_unbind_smpc(struct nvgpu_profiler_object *prof)
{
	struct gk20a *g = prof->g;
	int err;

	if (prof->scope == NVGPU_PROFILER_PM_RESERVATION_SCOPE_DEVICE) {
		if (prof->ctxsw[NVGPU_PROFILER_PM_RESOURCE_TYPE_SMPC]) {
			err = g->ops.gr.update_smpc_ctxsw_mode(g, prof->tsg, false);
		} else {
			if (nvgpu_is_enabled(g, NVGPU_SUPPORT_SMPC_GLOBAL_MODE)) {
				err = g->ops.gr.update_smpc_global_mode(g, false);
			} else {
				err = -EINVAL;
			}
		}
	} else {
		err = g->ops.gr.update_smpc_ctxsw_mode(g, prof->tsg, false);
	}

	return err;
}

static int nvgpu_profiler_bind_hwpm(struct nvgpu_profiler_object *prof, bool streamout)
{
	struct gk20a *g = prof->g;
	int err = 0;
	u32 mode = streamout ? NVGPU_GR_CTX_HWPM_CTXSW_MODE_STREAM_OUT_CTXSW :
			       NVGPU_GR_CTX_HWPM_CTXSW_MODE_CTXSW;

	if (prof->scope == NVGPU_PROFILER_PM_RESERVATION_SCOPE_DEVICE) {
		if (prof->ctxsw[NVGPU_PROFILER_PM_RESOURCE_TYPE_HWPM_LEGACY]) {
			err = g->ops.gr.update_hwpm_ctxsw_mode(g, prof->tsg, 0, mode);
		} else {
			if (g->ops.gr.reset_hwpm_pmm_registers != NULL) {
				g->ops.gr.reset_hwpm_pmm_registers(g);
			}
			g->ops.gr.init_hwpm_pmm_register(g);
		}
	} else {
		err = g->ops.gr.update_hwpm_ctxsw_mode(g, prof->tsg, 0, mode);
	}

	return err;
}

static int nvgpu_profiler_unbind_hwpm(struct nvgpu_profiler_object *prof)
{
	struct gk20a *g = prof->g;
	int err = 0;
	u32 mode = NVGPU_GR_CTX_HWPM_CTXSW_MODE_NO_CTXSW;

	if (prof->scope == NVGPU_PROFILER_PM_RESERVATION_SCOPE_DEVICE) {
		if (prof->ctxsw[NVGPU_PROFILER_PM_RESOURCE_TYPE_HWPM_LEGACY]) {
			err = g->ops.gr.update_hwpm_ctxsw_mode(g, prof->tsg, 0, mode);
		}
	} else {
		err = g->ops.gr.update_hwpm_ctxsw_mode(g, prof->tsg, 0, mode);
	}

	return err;
}

static int nvgpu_profiler_bind_hwpm_streamout(struct nvgpu_profiler_object *prof)
{
	struct gk20a *g = prof->g;
	int err;

	err = nvgpu_profiler_bind_hwpm(prof, true);
	if (err) {
		return err;
	}

	err = g->ops.perfbuf.perfbuf_enable(g, prof->pma_buffer_va, prof->pma_buffer_size);
	if (err) {
		nvgpu_profiler_unbind_hwpm(prof);
		return err;
	}

	g->ops.perf.bind_mem_bytes_buffer_addr(g, prof->pma_bytes_available_buffer_va);
	return 0;
}

static int nvgpu_profiler_unbind_hwpm_streamout(struct nvgpu_profiler_object *prof)
{
	struct gk20a *g = prof->g;
	int err;

	g->ops.perf.bind_mem_bytes_buffer_addr(g, 0ULL);

	err = g->ops.perfbuf.perfbuf_disable(g);
	if (err) {
		return err;
	}

	err = nvgpu_profiler_unbind_hwpm(prof);
	if (err) {
		return err;
	}

	return 0;
}

int nvgpu_profiler_bind_pm_resources(struct nvgpu_profiler_object *prof)
{
	struct gk20a *g = prof->g;
	int err;

	nvgpu_log(g, gpu_dbg_prof,
		"Request to bind PM resources with profiler handle %u",
		prof->prof_handle);

	if (prof->bound) {
		nvgpu_err(g, "PM resources are already bound with profiler handle %u",
			prof->prof_handle);
		return -EINVAL;
	}

	if (!prof->reserved[NVGPU_PROFILER_PM_RESOURCE_TYPE_HWPM_LEGACY] &&
	    !prof->reserved[NVGPU_PROFILER_PM_RESOURCE_TYPE_SMPC]) {
		nvgpu_err(g, "No PM resources reserved for profiler handle %u",
			prof->prof_handle);
		return -EINVAL;
	}

	err = gk20a_busy(g);
	if (err) {
		nvgpu_err(g, "failed to poweron");
		return err;
	}

	if (prof->reserved[NVGPU_PROFILER_PM_RESOURCE_TYPE_HWPM_LEGACY]) {
		if (prof->reserved[NVGPU_PROFILER_PM_RESOURCE_TYPE_PMA_STREAM]) {
			err = nvgpu_profiler_bind_hwpm_streamout(prof);
			if (err != 0) {
				nvgpu_err(g,
					"failed to bind HWPM streamout with profiler handle %u",
					prof->prof_handle);
				goto fail;
			}

			nvgpu_log(g, gpu_dbg_prof,
				"HWPM streamout bound with profiler handle %u",
				prof->prof_handle);
		} else {
			err = nvgpu_profiler_bind_hwpm(prof, false);
			if (err != 0) {
				nvgpu_err(g,
					"failed to bind HWPM with profiler handle %u",
					prof->prof_handle);
				goto fail;
			}

			nvgpu_log(g, gpu_dbg_prof,
				"HWPM bound with profiler handle %u",
				prof->prof_handle);
		}
	}

	if (prof->reserved[NVGPU_PROFILER_PM_RESOURCE_TYPE_SMPC]) {
		err = nvgpu_profiler_bind_smpc(prof);
		if (err) {
			nvgpu_err(g, "failed to bind SMPC with profiler handle %u",
				prof->prof_handle);
			goto fail;
		}

		nvgpu_log(g, gpu_dbg_prof,
			"SMPC bound with profiler handle %u", prof->prof_handle);
	}

	prof->bound = true;

fail:
	gk20a_idle(g);
	return err;
}

int nvgpu_profiler_unbind_pm_resources(struct nvgpu_profiler_object *prof)
{
	struct gk20a *g = prof->g;
	int err;

	if (!prof->bound) {
		nvgpu_err(g, "No PM resources bound to profiler handle %u",
			prof->prof_handle);
		return -EINVAL;
	}

	err = gk20a_busy(g);
	if (err) {
		nvgpu_err(g, "failed to poweron");
		return err;
	}

	if (prof->reserved[NVGPU_PROFILER_PM_RESOURCE_TYPE_HWPM_LEGACY]) {
		if (prof->reserved[NVGPU_PROFILER_PM_RESOURCE_TYPE_PMA_STREAM]) {
			err = nvgpu_profiler_unbind_hwpm_streamout(prof);
			if (err) {
				nvgpu_err(g,
					"failed to unbind HWPM streamout from profiler handle %u",
					prof->prof_handle);
				goto fail;
			}

			nvgpu_log(g, gpu_dbg_prof,
				"HWPM streamout unbound from profiler handle %u",
				prof->prof_handle);
		} else {
			err = nvgpu_profiler_unbind_hwpm(prof);
			if (err) {
				nvgpu_err(g,
					"failed to unbind HWPM from profiler handle %u",
					prof->prof_handle);
				goto fail;
			}

			nvgpu_log(g, gpu_dbg_prof,
				"HWPM unbound from profiler handle %u",
				prof->prof_handle);
		}
	}

	if (prof->reserved[NVGPU_PROFILER_PM_RESOURCE_TYPE_SMPC]) {
		err = nvgpu_profiler_unbind_smpc(prof);
		if (err) {
			nvgpu_err(g,
				"failed to unbind SMPC from profiler handle %u",
				prof->prof_handle);
			goto fail;
		}

		nvgpu_log(g, gpu_dbg_prof,
			"SMPC unbound from profiler handle %u",	prof->prof_handle);
	}

	prof->bound = false;

fail:
	gk20a_idle(g);
	return err;
}

int nvgpu_profiler_alloc_pma_stream(struct nvgpu_profiler_object *prof)
{
	struct gk20a *g = prof->g;
	int err;

	err = nvgpu_profiler_pm_resource_reserve(prof,
			NVGPU_PROFILER_PM_RESOURCE_TYPE_PMA_STREAM);
	if (err) {
		nvgpu_err(g, "failed to reserve PMA stream");
		return err;
	}

	err = nvgpu_perfbuf_init_vm(g);
	if (err) {
		nvgpu_err(g, "failed to initialize perfbuf VM");
		nvgpu_profiler_pm_resource_release(prof,
				NVGPU_PROFILER_PM_RESOURCE_TYPE_PMA_STREAM);
		return err;
	}

	return 0;
}

void nvgpu_profiler_free_pma_stream(struct nvgpu_profiler_object *prof)
{
	struct gk20a *g = prof->g;
	struct mm_gk20a *mm = &g->mm;

	if (prof->pma_buffer_va == 0U) {
		return;
	}

	nvgpu_vm_unmap(mm->perfbuf.vm, prof->pma_bytes_available_buffer_va, NULL);
	prof->pma_bytes_available_buffer_va = 0U;

	nvgpu_vm_unmap(mm->perfbuf.vm, prof->pma_buffer_va, NULL);
	prof->pma_buffer_va = 0U;
	prof->pma_buffer_size = 0U;

	nvgpu_perfbuf_deinit_vm(g);

	nvgpu_profiler_pm_resource_release(prof,
			NVGPU_PROFILER_PM_RESOURCE_TYPE_PMA_STREAM);
}
