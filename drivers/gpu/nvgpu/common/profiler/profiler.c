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
#include <nvgpu/kmem.h>
#include <nvgpu/tsg.h>

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
