/*
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

#ifndef NVGPU_STRING_H
#define NVGPU_STRING_H

#include <nvgpu/types.h>

#ifdef __KERNEL__
#include <linux/string.h>
#endif

struct gk20a;

/**
 * @brief Copy memory buffer
 *
 * @param destb [out]	Buffer into which data is to be copied.
 * @param srcb [in]	Buffer from which data is to be copied.
 * @param n [in]	Number of bytes to copy from src buffer to dest buffer.
 *
 * Copy memory from source buffer to destination buffer.
 */
void nvgpu_memcpy(u8 *destb, const u8 *srcb, size_t n);

/**
 * @brief Compare memory buffers
 *
 * @param b1 [in]	First buffer to use in memory comparison.
 * @param b2 [in]	Second buffer to use in memory comparison.
 * @param n [in]	Number of bytes to compare between two buffers.
 *
 * Compare the first \a n bytes of two memory buffers.  If the contents of the
 * two buffers match then zero is returned.  If the contents of b1 are less
 * than b2 then a value less than zero is returned.  If the contents of b1
 * are greater than b2 then a value greater than zero is returned.
 *
 * @return 0 if the comparison matches else a non-zero value is returned.
 *
 * @retval 0 if both buffers match.
 * @retval <0 if object pointed to by \a b1 is less than the object pointed to
 * by \a b2.
 * @retval >0 if object pointed to by \a b1 is greater than the object pointed
 * to by \a b2.
 */
int nvgpu_memcmp(const u8 *b1, const u8 *b2, size_t n);

/**
 * @brief Formats u32 into null-terminated string
 *
 * @param dst [in,out]	Buffer to copy the value into.
 * @param value [in]	Value to be added with the string.
 * @param size [in]	Size available in the destination buffer.
 * @param radix	[in]	Radix value to be used.
 *
 * Formats the integer variable \a value into a null terminated string.
 *
 * @return Returns number of digits added to string (not including '\0') if
 * successful, else 0.
 *
 * @retval 0 in case of invalid radix or insufficient space.
 */
int nvgpu_strnadd_u32(char *dst, const u32 value, size_t size, u32 radix);

/**
 * @brief Check that memory address is word (4-byte) aligned.
 *
 * @param g [in]	struct gk20a.
 * @param addr [in]	Memory address.
 *
 * Checks if the provided address is word aligned or not.
 *
 * @return Boolean value to indicate the alignment status of the address.
 *
 * @retval TRUE if \a addr is word aligned.
 * @retval FALSE if \a addr is not word aligned.
 */
bool nvgpu_mem_is_word_aligned(struct gk20a *g, u8 *addr);

#endif /* NVGPU_STRING_H */
