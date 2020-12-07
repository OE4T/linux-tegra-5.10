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
#ifndef NVGPU_GOPS_CLASS_H
#define NVGPU_GOPS_CLASS_H

#include <nvgpu/types.h>

/**
 * @file
 *
 * common.class unit HAL interface
 *
 */

/**
 * common.class unit HAL operations
 *
 * @see gpu_ops
 */
struct gops_class {
	/**
	 * @brief Checks if given class number is valid as per our GPU
	 *        architechture. This API is used by common.gr unit to
	 *        validate the class associated with the channel.
	 *
	 * List of valid class numbers:
	 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * 1. Graphics classes: (WAR: Lot of qnx safety tests are still using
	 *			 graphics 3d class. Until these tests get fixed,
	 *			 allowing 3d graphics class as valid class for
	 *			 safety build.)
	 *	a. VOLTA_A                   --> 0xC397U
	 * 2. Compute classes:
	 * 	a. VOLTA_COMPUTE_A           --> 0xC3C0U
	 * 3. DMA copy
	 *	a. KEPLER_DMA_COPY_A         --> 0xA0B5U
	 *	b. MAXWELL_DMA_COPY_A        --> 0xB0B5U
	 *	c. PASCAL_DMA_COPY_A         --> 0xC0B5U
	 *	d. VOLTA_DMA_COPY_A          --> 0xC3B5U
	 * 4. Inline to memory
	 *	a. KEPLER_INLINE_TO_MEMORY_B --> 0xA140U
	 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 *
	 * @param class_num [in]	Class number to be checked.
	 *
	 * @return true when \a class_num is one of the numbers in above list or
	 *	   false otherwise.
	 */
	bool (*is_valid)(u32 class_num);

	/**
	 * @brief Checks if given class number is valid compute class number
	 * 	  as per our GPU architechture. This API is used by common.gr
	 *        unit to set apart the compute class from other classes.
	 *        This is needed when the preemption mode is selected based
	 *        on the class type.
	 *
	 * List of valid compute class numbers:
	 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * 	a. VOLTA_COMPUTE_A          --> 0xC3C0U
	 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 *
	 * @param class_num [in]	Class number to be checked.
	 *
	 * @return true when \a class_num is one of the numbers in above list or
	 *	   false otherwise.
	 */
	bool (*is_valid_compute)(u32 class_num);

	/** @cond DOXYGEN_SHOULD_SKIP_THIS */
#ifdef CONFIG_NVGPU_GRAPHICS
	bool (*is_valid_gfx)(u32 class_num);
#endif
	/** @endcond DOXYGEN_SHOULD_SKIP_THIS */
};

#endif /* NVGPU_GOPS_CLASS_H */
