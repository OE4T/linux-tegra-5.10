/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
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
#ifndef NVGPU_GOPS_PTIMER_H
#define NVGPU_GOPS_PTIMER_H

#include <nvgpu/types.h>

/**
 * @file
 *
 * ptimer unit HAL interface
 *
 */
struct gk20a;
struct nvgpu_cpu_time_correlation_sample;

/**
 * ptimer unit HAL operations
 *
 * @see gpu_ops
 */
struct gops_ptimer {
	/**
	 * @brief Handles specific type of PRI errors
	 *
	 * @param g [in]		GPU driver struct pointer
	 *
	 * 1. ISR is called when one of the below PRI error occurs:
	 *      - PRI_SQUASH: error due to pri access while target block is
	 *		      in reset
	 *      - PRI_FECSERR: FECS detected an error while processing a PRI
	 * 		      request
	 *      - PRI_TIMEOUT: non-existent host register / timeout waiting for
	 *                    FECS
	 * 2. In the ISR, we read PRI_TIMEOUT_SAVE registers - that is, SAVE_0,
	 *    SAVE_1 and FECS_ERRCODE, which contain information about the first
	 *    PRI error since the previous error was cleared.
	 * 3. We extract the address of the first PRI access that resulted in
	 *    error from PRI_TIMEOUT_SAVE_0 register. Note this address field
	 *    has 4-byte granularity and is multiplied by 4 to obtain the byte
	 *    address. Also we find out if the PRI access was a write or a read
	 *    based on whether PRI_TIMEOUT_SAVE_0_WRITE is true or false
	 *    respectively.
	 * 4. We read PRI_TIMEOUT_SAVE_1 which contains the PRI write data for
	 *    the failed request. Note data is set to
	 *    NV_PTIMER_PRI_TIMEOUT_SAVE_1_DATA_WAS_READ when the failed request
	 *    was a read.
	 * 5. NV_PTIMER_PRI_TIMEOUT_SAVE_0_FECS_TGT field indicates if fecs was
	 *    the target of the PRI access. If FECS_TGT is TRUE, all other
	 *    fields in the PRI_TIMEOUT_SAVE_* registers are unreliable except
	 *    the PRI_TIMEOUT_SAVE_0_TO field and the PRI_TIMEOUT_FECS_ERRCODE
	 *    So if FECS_TGT is set, we read PRI_TIMEOUT_FECS_ERRCODE and call
	 *    priv ring HAL to decode the error.
	 * 6. We clear SAVE_0 and SAVE_1 registers so that the next pri access
	 *    error can be recorded.
	 */
	void (*isr)(struct gk20a *g);

	/** @cond DOXYGEN_SHOULD_SKIP_THIS */

	/**
	 * Private HAL
	 */
	int (*read_ptimer)(struct gk20a *g, u64 *value);

	/**
	 * NON-FUSA HAL
	 */
#ifdef CONFIG_NVGPU_IOCTL_NON_FUSA
	int (*get_timestamps_zipper)(struct gk20a *g,
			u32 source_id, u32 count,
			struct nvgpu_cpu_time_correlation_sample *samples);
#endif
#ifdef CONFIG_NVGPU_DEBUGGER
	int (*config_gr_tick_freq)(struct gk20a *g);
#endif
#ifdef CONFIG_NVGPU_PROFILER
	void (*get_timer_reg_offsets)(u32 *timer0_offset, u32 *timer1_offset);
#endif

	/** @endcond DOXYGEN_SHOULD_SKIP_THIS */
};

#endif /* NVGPU_GOPS_PTIMER_H */
