/*
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/bug.h>
#include <nvgpu/posix/posix-nvhost.h>

static inline u32 nvgpu_nvhost_syncpt_nb_hw_pts(
		struct nvgpu_nvhost_dev *nvgpu_syncpt_dev)
{
	return nvgpu_syncpt_dev->nb_hw_pts;
}

void nvgpu_free_nvhost_dev(struct gk20a *g) {

	if (g->nvhost_dev != NULL) {
		nvgpu_kfree(g, g->nvhost_dev);
		g->nvhost_dev = NULL;
	}
}

int nvgpu_get_nvhost_dev(struct gk20a *g)
{
	int ret = 0;

	g->nvhost_dev = nvgpu_kzalloc(g, sizeof(struct nvgpu_nvhost_dev));
	if (g->nvhost_dev == NULL) {
		return -ENOMEM;
	}

	g->nvhost_dev->host1x_sp_base = 0x60000000;
	g->nvhost_dev->host1x_sp_size = 0x400000;
	g->nvhost_dev->nb_hw_pts = 704U;
	ret = nvgpu_nvhost_syncpt_unit_interface_get_aperture(
				g->nvhost_dev, &g->syncpt_unit_base,
				&g->syncpt_unit_size);
	if (ret != 0) {
		nvgpu_err(g, "Failed to get syncpt interface");
		goto fail_nvgpu_get_nvhost_dev;
	}
	g->syncpt_size =
		nvgpu_nvhost_syncpt_unit_interface_get_byte_offset(1);

	return 0;

fail_nvgpu_get_nvhost_dev:
	nvgpu_free_nvhost_dev(g);

	return ret;
}

int nvgpu_nvhost_syncpt_unit_interface_get_aperture(
		struct nvgpu_nvhost_dev *nvgpu_syncpt_dev,
		u64 *base, size_t *size)
{
	if (nvgpu_syncpt_dev == NULL || base == NULL || size == NULL) {
		return -ENOSYS;
	}

	*base = (u64)nvgpu_syncpt_dev->host1x_sp_base;
	*size = nvgpu_syncpt_dev->host1x_sp_size;

	return 0;

}

u32 nvgpu_nvhost_syncpt_unit_interface_get_byte_offset(u32 syncpt_id)
{
	return nvgpu_safe_mult_u32(syncpt_id, 0x1000U);
}

void nvgpu_nvhost_syncpt_set_min_eq_max_ext(
	struct nvgpu_nvhost_dev *nvhost_dev, u32 id)
{
	BUG();
}

void nvgpu_nvhost_syncpt_put_ref_ext(
	struct nvgpu_nvhost_dev *nvhost_dev, u32 id)
{
	BUG();
}

u32 nvgpu_nvhost_get_syncpt_client_managed(
	struct nvgpu_nvhost_dev *nvhost_dev,
	const char *syncpt_name)
{
	return 0U;
}

void nvgpu_nvhost_syncpt_set_safe_state(
	struct nvgpu_nvhost_dev *nvhost_dev, u32 id)
{
	BUG();
}
