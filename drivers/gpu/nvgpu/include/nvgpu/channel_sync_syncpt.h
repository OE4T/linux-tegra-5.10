/*
 *
 * Nvgpu Channel Synchronization Abstraction (Syncpoints)
 *
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_CHANNEL_SYNC_SYNCPT_H
#define NVGPU_CHANNEL_SYNC_SYNCPT_H

#include <nvgpu/types.h>
#include <nvgpu/nvgpu_mem.h>
#include <nvgpu/channel_sync.h>

#define NVGPU_INVALID_SYNCPT_ID		(~U32(0U))

struct channel_gk20a;
struct nvgpu_channel_sync_syncpt;

#ifdef CONFIG_TEGRA_GK20A_NVHOST

/*
 * Returns the sync point id or negative number if no syncpt
 */
u32 nvgpu_channel_sync_get_syncpt_id(struct nvgpu_channel_sync_syncpt *s);

/*
 * Returns the sync point address of sync point or 0 if not supported
 */
u64 nvgpu_channel_sync_get_syncpt_address(struct nvgpu_channel_sync_syncpt *s);

/*
 * Generate a gpu wait cmdbuf from raw fence(can be syncpoints or semaphores).
 * Returns a gpu cmdbuf that performs the wait when executed.
 */
int nvgpu_channel_sync_wait_syncpt(struct nvgpu_channel_sync_syncpt *s,
	u32 id, u32 thresh, struct priv_cmd_entry *entry);

/*
 * Converts a valid struct nvgpu_channel_sync ptr to
 * struct nvgpu_channel_sync_syncpt ptr else return NULL.
 */
struct nvgpu_channel_sync_syncpt * __must_check
nvgpu_channel_sync_to_syncpt(struct nvgpu_channel_sync *sync);

/*
 * Constructs a struct nvgpu_channel_sync_syncpt and returns a
 * pointer to the struct nvgpu_channel_sync associated with it.
 */
struct nvgpu_channel_sync *
nvgpu_channel_sync_syncpt_create(struct channel_gk20a *c,
	bool user_managed);

#else

static inline u32 nvgpu_channel_sync_get_syncpt_id(
	struct nvgpu_channel_sync_syncpt *s)
{
	return NVGPU_INVALID_SYNCPT_ID;
}
static inline u64 nvgpu_channel_sync_get_syncpt_address(
	struct nvgpu_channel_sync_syncpt *s)
{
	return 0ULL;
}

static inline int nvgpu_channel_sync_wait_syncpt(
	struct nvgpu_channel_sync_syncpt *s,
	u32 id, u32 thresh, struct priv_cmd_entry *entry)
{
	return -EINVAL;
}

static inline struct nvgpu_channel_sync_syncpt * __must_check
nvgpu_channel_sync_to_syncpt(struct nvgpu_channel_sync *sync)
{
	return NULL;
}

static inline struct nvgpu_channel_sync *
nvgpu_channel_sync_syncpt_create(struct channel_gk20a *c, bool user_managed)
{
	return NULL;
}

#endif

#endif /* NVGPU_CHANNEL_SYNC_SYNCPT_H */
