/*
 * Copyright (c) 2017-2020, NVIDIA CORPORATION.  All rights reserved.
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
 * @brief Computes the ptimer scaling factor. This API allows setting the ptimer
 *        appropriately before using it to enforce different timeouts or
 *        scheduling timeslices.
 *
 * @param ptimer_src_freq [in]		source frequency to ptimer
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
 * Scale_factor = ptimer_ref_freq / ptimer_src_freq
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * 3. The scale_factor is multiplied by 10, so that we get an additional digit
 * decimal precision.
 *
 * 4. For example,
 * - On Maxwell, the ptimer source frequency is 19.2 MHz.
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  scaling_factor_10x = (31250000  * 10)/ 19200000 = 16
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * - On Volta, ptimer_source frequency = 31250000 Hz = ptimer_ref_frequency.
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  scaling_factor_10x = 10
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @return Scale factor between ptimer reference and source frequency with
 * 	one digit decimal precision.
 */
static inline u32 ptimer_scalingfactor10x(u32 ptimer_src_freq)
{
	nvgpu_assert(ptimer_src_freq != 0U);
	return nvgpu_safe_cast_u64_to_u32((U64(PTIMER_REF_FREQ_HZ) * U64(10))
						/ U64(ptimer_src_freq));
}

/**
 * @brief Scales back the ptimer based timeout value as per the scale factor.
 *        Units like common.fifo use this API to scale the timeouts and
 *        scheduling timeslices enforced by it using the GPU ptimer.
 *
 * @param timeout [in]		Time value captured using ptimer reference clock
 * @param scale10x [in]		The scale factor multiplied by 10 to be used for
 * 				scaling the ptimer based timeout value.
 *
 * 1. When the ptimer source frequency is not same as expected ptimer reference
 *    frequency, we need to scale the ptimer based time value. The scaled value
 *    is calculated as follows:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Scaled valued = timeout / scale_factor.
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * 2. To retain 1 digit decimal precision, the above equation is calculated
 *    after multiplication by 10.
 *
 * @return Scaled \a timeout value as per \a scale10x
 */
static inline u32 scale_ptimer(u32 timeout , u32 scale10x)
{
	nvgpu_assert(scale10x != 0U);
	if ((nvgpu_safe_mult_u32(timeout, 10U) % scale10x) >= (scale10x/2U)) {
		return ((timeout * 10U) / scale10x) + 1U;
	} else {
		return (timeout * 10U) / scale10x;
	}
}

#ifdef CONFIG_NVGPU_IOCTL_NON_FUSA
int nvgpu_get_timestamps_zipper(struct gk20a *g,
		u32 source_id, u32 count,
		struct nvgpu_cpu_time_correlation_sample *samples);
#endif /* CONFIG_NVGPU_IOCTL_NON_FUSA */
#endif
