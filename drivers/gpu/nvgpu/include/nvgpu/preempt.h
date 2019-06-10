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
#ifndef NVGPU_PREEMPT_H
#define NVGPU_PREEMPT_H

#include <nvgpu/types.h>

/**
 * @file
 *
 * Preemption interface.
 */
struct gk20a;
struct nvgpu_channel;
struct nvgpu_tsg;

/**
 * @brief Get preemption timeout (ms). This timeout is defined by s/w.
 *
 * @param g[in]		The GPU driver struct to query preempt timeout for.
 *
 * @return Maximum amount of time in ms to wait for preemption completion,
 *         i.e. context non resident on PBDMAs and engines.
 */
u32 nvgpu_preempt_get_timeout(struct gk20a *g);

/**
 * @brief Preempts TSG if channel is bound to TSG.
 *
 * @param g[in]		The GPU driver struct which owns this channel.
 * @param ch[in]	Pointer to channel to be preempted.
 *
 * Preempts TSG if channel is bound to TSG. Preemption implies that the
 * context's state is saved out and also that the context cannot remain parked
 * either in Host or in any engine.
 *
 * After triggering a preempt request for channel's TSG, pbdmas and engines
 * are polled to make sure preemption completed, i.e. context is not loaded
 * on any pbdma or engine.
 *
 * @return 0 in case of success, <0 in case of failure
 * @retval 0 if channel was not bound to TSG.
 * @retval 0 if TSG was not loaded on pbdma or engine.
 * @retval 0 if TSG was loaded (pbdma or engine) and could be preempted.
 * @retval non-zero value if preemption did not complete within s/w defined
 *         timeout.
 */
int nvgpu_preempt_channel(struct gk20a *g, struct nvgpu_channel *ch);

/**
 * Called from recovery handling for volta onwards. This will
 * not be part of safety build after recovery is not supported in safety build.
 */
void nvgpu_preempt_poll_tsg_on_pbdma(struct gk20a *g,
		struct nvgpu_tsg *tsg);
#endif /* NVGPU_PREEMPT_H */
