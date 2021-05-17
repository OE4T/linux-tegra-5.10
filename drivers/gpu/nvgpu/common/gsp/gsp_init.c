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

static void nvgpu_gsp_sw_deinit(struct gk20a *g)
{
	if (g->gsp != NULL) {
		nvgpu_kfree(g, g->gsp);
	}
}

int nvgpu_gsp_sw_init(struct gk20a *g)
{
	int err = 0;

	nvgpu_log_fn(g, " ");

	/* Init struct holding the gsp software state */
	g->gsp = (struct nvgpu_gsp *)nvgpu_kzalloc(g, sizeof(struct nvgpu_gsp));
	if (g->gsp == NULL) {
		err = -ENOMEM;
		goto de_init;
	}

	/* gsp falcon software state */
	g->gsp->gsp_flcn = &g->gsp_flcn;

	return err;
de_init:
	nvgpu_gsp_sw_deinit(g);
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
