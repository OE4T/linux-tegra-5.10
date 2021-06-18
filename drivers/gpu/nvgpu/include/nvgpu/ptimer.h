/*
 * Copyright (c) 2017-2021, NVIDIA CORPORATION.  All rights reserved.
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
#ifndef NVGPU_PTIMER_H
#define NVGPU_PTIMER_H

/**
 * @file
 *
 * Declares structs, defines and APIs exposed by ptimer unit.
 */
#include <nvgpu/types.h>
#include <nvgpu/static_analysis.h>

struct gk20a;

struct nvgpu_cpu_time_correlation_sample {
	u64 cpu_timestamp;
	u64 gpu_timestamp;
};

/**
 * PTIMER_REF_FREQ_HZ corresponds to a period of 32 nanoseconds.
 * 32 ns is the resolution of ptimer.
 */
#define PTIMER_REF_FREQ_HZ                      31250000U

/**
 * @brief Scales the ptimer based timeout value as per the ptimer scale factor.
 *        Units like common.fifo use this API to scale the timeouts and
 *        scheduling timeslices enforced by it using the GPU ptimer.
 *
 * @param timeout [in]		Time value captured using ptimer reference clock
 * @param scaled_timeout [out]	Scaled time value after scaling with scale factor
 *
 * 1. The ptimer has a resolution of 32 ns and so requires a reference frequency
 *    of:
 * ~~~~~~~~~~~~~~~~~~~~~
 * 1 / 32ns = 31.25 MHz
 * ~~~~~~~~~~~~~~~~~~~~~
 *
 * 2. If the source frequency to ptimer is different than the above reference
 *    frequency, we need to get the scaling factor as:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * scale_factor = ptimer_ref_freq / ptimer_src_freq
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * 3. The scale_factor is multiplied by 10, so that we get an additional digit
 * decimal precision.
 *
 * 4. For example,
 * - On Maxwell, the ptimer source frequency is 19.2 MHz.
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  scale_factor = (31250000  * 10)/ 19200000 = 16
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * - On Volta, ptimer_source frequency = 31250000 Hz = ptimer_ref_frequency.
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  scale_factor = 10
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * 5. When the ptimer source frequency is not same as expected ptimer reference
 *    frequency, we need to scale the ptimer based time value. The scaled value
 *    is calculated as follows:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Scaled valued = timeout / scale_factor.
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * 6. To retain 1 digit decimal precision, the above equation is calculated
 *    after multiplication by 10. And because of that maximum acceptable
 *    value of \a timeout can be (U32_MAX / 10).
 *
 * @return 0 in case of success, < 0 in case of failure.
 * @retval -EINVAL in case invalid \a timeout value is passed.
 */
int nvgpu_ptimer_scale(struct gk20a *g, u32 timeout, u32 *scaled_timeout);

#ifdef CONFIG_NVGPU_IOCTL_NON_FUSA
int nvgpu_get_timestamps_zipper(struct gk20a *g,
		u32 source_id, u32 count,
		struct nvgpu_cpu_time_correlation_sample *samples);
int nvgpu_ptimer_init(struct gk20a *g);
#endif /* CONFIG_NVGPU_IOCTL_NON_FUSA */
#endif
