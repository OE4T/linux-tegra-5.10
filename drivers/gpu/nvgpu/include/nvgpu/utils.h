/*
 * Copyright (c) 2018-2020, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_UTILS_H
#define NVGPU_UTILS_H

#include <nvgpu/types.h>
#include <nvgpu/static_analysis.h>

#ifdef __KERNEL__
#include <nvgpu/linux/utils.h>
#else
#include <nvgpu/posix/utils.h>
#endif

/*
 * PAGE_SIZE is OS specific and can vary across OSes. Depending on the OS it maybe
 * defined to 4K or 64K.
 */
#define NVGPU_CPU_PAGE_SIZE	PAGE_SIZE

/**
 * @file
 *
 * SW utilities
 *
 * @defgroup unit-common-utils
 */

/**
 * @defgroup bit-utils
 * @ingroup unit-common-utils
 * @{
 */

/** Stringification macro. */
#define nvgpu_stringify(x)          #x

/**
 * @brief Higher 32 bits from 64 bit.
 *
 * @param n [in]	Input value.
 *
 * Returns the most significant 32 bits of the 64 bit input value.
 *
 * @return Most significant 32 bits of \a n.
 */
static inline u32 u64_hi32(u64 n)
{
	return nvgpu_safe_cast_u64_to_u32(n >> 32);
}

/**
 * @brief Lower 32 bits from 64 bit.
 *
 * @param n [in]	Input value.
 *
 * Returns the least significant 32 bits of the 64 bit input value.
 *
 * @return Least significant 32 bits of \a n.
 */
static inline u32 u64_lo32(u64 n)
{
	return nvgpu_safe_cast_u64_to_u32(n & ~(u32)0);
}

/**
 * @brief 64 bit from two 32 bit values.
 *
 * @param hi [in]	Higher 32 bits.
 * @param lo [in]	Lower 32 bits.
 *
 * Returns a 64 bit value by combining the two 32 bit input values.
 *
 * @return 64 bit value of which the least significant 32 bits are \a lo and
 * most significant 32 bits are \a hi.
 */
static inline u64 hi32_lo32_to_u64(u32 hi, u32 lo)
{
	return  (((u64)hi) << 32) | (u64)lo;
}

/**
 * @brief Sets a particular field value in input data.
 *
 * @param val [in]	Value to set the field in.
 * @param mask [in]	Mask for the field.
 * @param field [in]	Field value.
 *
 * Uses the \a mask value to clear the bits that are part of the field and
 * sets the value mentioned in \a field in those bit positions.
 *
 * @return Returns \a val with updated field.
 */
static inline u32 set_field(u32 val, u32 mask, u32 field)
{
	return ((val & ~mask) | field);
}

/**
 * @brief Gets a particular field value from input data.
 *
 * @param reg [in]	Value to get the field from.
 * @param mask [in]	Mask for the field.
 *
 * Returns the field value at mask position in reg.
 *
 * @return Field value from \a reg according to the \a mask.
 */
static inline u32 get_field(u32 reg, u32 mask)
{
	return (reg & mask);
}

/*
 * MISRA Rule 11.6 compliant IP address generator.
 */
/**
 * Instruction pointer address generator. Used to get the virtual address
 * of the current instruction.
 */
#define NVGPU_GET_IP		\
	({ __label__ label_here; label_here: &&label_here; })

/**
 * @}
 */

#endif /* NVGPU_UTILS_H */
