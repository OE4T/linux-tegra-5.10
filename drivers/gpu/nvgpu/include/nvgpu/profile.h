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

#ifndef NVGPU_PROFILE_H
#define NVGPU_PROFILE_H

/*
 * Number of entries in the kickoff latency buffer, used to calculate
 * the profiling and histogram. This number is calculated to be statistically
 * significative on a histogram on a 5% step
 */
#ifdef CONFIG_DEBUG_FS
#define FIFO_PROFILING_ENTRIES	16384U
#endif

enum {
	PROFILE_IOCTL_ENTRY = 0U,
	PROFILE_ENTRY,
	PROFILE_JOB_TRACKING,
	PROFILE_APPEND,
	PROFILE_END,
	PROFILE_IOCTL_EXIT,
	PROFILE_MAX
};

struct nvgpu_profile {
	u64 timestamp[PROFILE_MAX];
};

#ifdef CONFIG_DEBUG_FS
struct nvgpu_profile *nvgpu_profile_acquire(struct gk20a *g);
void nvgpu_profile_release(struct gk20a *g,
	struct nvgpu_profile *profile);
void nvgpu_profile_snapshot(struct nvgpu_profile *profile, int idx);
#else
static inline struct nvgpu_profile *
nvgpu_profile_acquire(struct gk20a *g)
{
	return NULL;
}
static inline void nvgpu_profile_release(struct gk20a *g,
	struct nvgpu_profile *profile)
{
}
static inline void nvgpu_profile_snapshot(
		struct nvgpu_profile *profile, int idx)
{
}
#endif

#endif /* NVGPU_PROFILE_H */
