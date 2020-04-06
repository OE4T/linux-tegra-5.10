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

#include <nvgpu/kmem.h>
#include <nvgpu/log.h>
#include <nvgpu/nvhost.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/channel.h>
#include <nvgpu/channel_user_syncpt.h>
#include <nvgpu/string.h>
#include "channel_user_syncpt_priv.h"

static int user_sync_build_debug_name(struct nvgpu_channel *ch,
		char *buf, size_t capacity)
{
	struct gk20a *g = ch->g;
	int n;

	(void)strncpy(buf, g->name, capacity);
	capacity = nvgpu_safe_sub_u64(capacity, strlen(g->name));

	(void)strncat(buf, "_", capacity);
	capacity = nvgpu_safe_sub_u64(capacity, strlen("_"));
	/*
	 * however, nvgpu_strnadd_u32 expects capacity to include the
	 * terminating byte, so add it back
	 */
	capacity = nvgpu_safe_add_u64(capacity, 1);

	n = nvgpu_strnadd_u32(&buf[strlen(buf)], ch->chid,
				capacity, 10);
	if (n == 0) {
		nvgpu_err(g, "strnadd failed!");
		return -EINVAL;
	}
	capacity = nvgpu_safe_sub_u64(capacity, n);
	/* nul byte */
	capacity = nvgpu_safe_sub_u64(capacity, 1);

	(void)strncat(buf, "_user", capacity);
	/* make sure it didn't get truncated */
	capacity = nvgpu_safe_sub_u64(capacity, strlen("_user"));

	return 0;
}

struct nvgpu_channel_user_syncpt *
nvgpu_channel_user_syncpt_create(struct nvgpu_channel *ch)
{
	struct gk20a *g = ch->g;
	struct nvgpu_channel_user_syncpt *s;
	char syncpt_name[32] = {0}; /* e.g. gp10b_42_user */
	int err;

	s = nvgpu_kzalloc(ch->g, sizeof(*s));
	if (s == NULL) {
		return NULL;
	}

	s->ch = ch;
	s->nvhost = g->nvhost;

	err = user_sync_build_debug_name(ch, syncpt_name,
			sizeof(syncpt_name) - 1);
	if (err < 0) {
		goto err_free;
	}

	s->syncpt_id = nvgpu_nvhost_get_syncpt_client_managed(s->nvhost,
					syncpt_name);

	/**
	 * This is a WAR to handle invalid value of a syncpt.
	 * Once nvhost update the return value as NVGPU_INVALID_SYNCPT_ID,
	 * we can remove the zero check.
	 */
	if ((s->syncpt_id == 0U) ||
			(s->syncpt_id == NVGPU_INVALID_SYNCPT_ID)) {
		nvgpu_err(g, "failed to get free syncpt");
		goto err_free;
	}

	err = g->ops.sync.syncpt.alloc_buf(ch, s->syncpt_id, &s->syncpt_buf);
	if (err != 0) {
		nvgpu_err(g, "failed to allocate syncpoint buffer");
		goto err_put;
	}

	nvgpu_nvhost_syncpt_set_min_eq_max_ext(s->nvhost, s->syncpt_id);

	return s;
err_put:
	nvgpu_nvhost_syncpt_put_ref_ext(s->nvhost, s->syncpt_id);
err_free:
	nvgpu_kfree(g, s);
	return NULL;
}

u32 nvgpu_channel_user_syncpt_get_id(struct nvgpu_channel_user_syncpt *s)
{
	return s->syncpt_id;
}

u64 nvgpu_channel_user_syncpt_get_address(struct nvgpu_channel_user_syncpt *s)
{
	return s->syncpt_buf.gpu_va;
}

void nvgpu_channel_user_syncpt_set_safe_state(struct nvgpu_channel_user_syncpt *s)
{
	nvgpu_nvhost_syncpt_set_safe_state(s->nvhost, s->syncpt_id);
}

void nvgpu_channel_user_syncpt_destroy(struct nvgpu_channel_user_syncpt *s)
{
	struct gk20a *g = s->ch->g;

	g->ops.sync.syncpt.free_buf(s->ch, &s->syncpt_buf);

	nvgpu_nvhost_syncpt_set_min_eq_max_ext(s->nvhost, s->syncpt_id);
	nvgpu_nvhost_syncpt_put_ref_ext(s->nvhost, s->syncpt_id);
	nvgpu_kfree(g, s);
}
