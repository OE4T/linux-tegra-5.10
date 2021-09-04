/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/firmware.h>
#include <nvgpu/falcon.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/log.h>
#include <nvgpu/gsp.h>

#include "gsp_priv.h"
#include "gsp_bootstrap.h"

void nvgpu_gsp_isr_support(struct gk20a *g, bool enable)
{
	nvgpu_log_fn(g, " ");

	/* Enable irq*/
	nvgpu_mutex_acquire(&g->gsp->isr_mutex);
	g->ops.gsp.enable_irq(g, enable);
	g->gsp->isr_enabled = enable;
	nvgpu_mutex_release(&g->gsp->isr_mutex);
}

void nvgpu_gsp_sw_deinit(struct gk20a *g)
{
	if (g->gsp != NULL) {

		nvgpu_gsp_isr_support(g, false);

		nvgpu_mutex_destroy(&g->gsp->isr_mutex);
#ifdef CONFIG_NVGPU_FALCON_DEBUG
		nvgpu_falcon_dbg_buf_destroy(g->gsp->gsp_flcn);
#endif
		nvgpu_kfree(g, g->gsp);
		g->gsp = NULL;
	}
}

int nvgpu_gsp_sw_init(struct gk20a *g)
{
	int err = 0;
	struct nvgpu_gsp *gsp;

	nvgpu_log_fn(g, " ");

	if (g->gsp != NULL) {
		/*
		 * Recovery/unrailgate case, we do not need to do gsp init as
		 * gsp is set during cold boot & doesn't execute gsp clean up as
		 * part of power off sequence, so reuse to perform faster boot.
		 */
		return err;
	}

	/* Init struct holding the gsp software state */
	g->gsp = (struct nvgpu_gsp *)nvgpu_kzalloc(g, sizeof(struct nvgpu_gsp));
	if (g->gsp == NULL) {
		err = -ENOMEM;
		goto exit;
	}

	gsp = g->gsp;

	gsp->g = g;

	/* gsp falcon software state */
	gsp->gsp_flcn = &g->gsp_flcn;

	/* enable debug buffer support */
#ifdef CONFIG_NVGPU_FALCON_DEBUG
	if ((g->ops.gsp.gsp_get_queue_head != NULL) &&
			(g->ops.gsp.gsp_get_queue_tail != NULL)) {
		err = nvgpu_falcon_dbg_buf_init(
			gsp->gsp_flcn, GSP_DMESG_BUFFER_SIZE,
			g->ops.gsp.gsp_get_queue_head(GSP_DEBUG_BUFFER_QUEUE),
			g->ops.gsp.gsp_get_queue_tail(GSP_DEBUG_BUFFER_QUEUE));
		if (err != 0) {
			nvgpu_err(g, "GSP debug init failed");
			goto exit;
		}
	}
#endif
	/* Init isr mutex */
	nvgpu_mutex_init(&gsp->isr_mutex);

exit:
	return err;
}

int nvgpu_gsp_bootstrap(struct gk20a *g)
{
	int err = 0;

	nvgpu_log_fn(g, " ");

	err = gsp_bootstrap_ns(g, g->gsp);
	if (err != 0) {
		nvgpu_err(g, "GSP bootstrap failed");
		goto de_init;
	}

	return err;
de_init:
	nvgpu_gsp_sw_deinit(g);
	return err;
}

void nvgpu_gsp_isr_mutex_aquire(struct gk20a *g)
{
	struct nvgpu_gsp *gsp = g->gsp;

	nvgpu_mutex_acquire(&gsp->isr_mutex);
}

void nvgpu_gsp_isr_mutex_release(struct gk20a *g)
{
	struct nvgpu_gsp *gsp = g->gsp;

	nvgpu_mutex_release(&gsp->isr_mutex);
}

bool nvgpu_gsp_is_isr_enable(struct gk20a *g)
{
	struct nvgpu_gsp *gsp = g->gsp;

	return gsp->isr_enabled;
}

struct nvgpu_falcon *nvgpu_gsp_falcon_instance(struct gk20a *g)
{
	struct nvgpu_gsp *gsp = g->gsp;

	return gsp->gsp_flcn;
}
