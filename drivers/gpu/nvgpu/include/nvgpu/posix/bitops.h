/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_POSIX_BITOPS_H
#define NVGPU_POSIX_BITOPS_H

#include <nvgpu/types.h>

/*
 * Assume an 8 bit byte, of course.
 */

/**
 * Number of bits in a byte.
 */
#define BITS_PER_BYTE	8UL

/**
 * Number of bits in data type long.
 */
#define BITS_PER_LONG 	((unsigned long)__SIZEOF_LONG__ * BITS_PER_BYTE)

/**
 * @brief Convert number of bits into long.
 *
 * @param bits [in]	Number of bits to convert.
 *
 * Converts the input param \a bits into equivalent number of long.
 */
#define BITS_TO_LONGS(bits)			\
	(nvgpu_safe_add_u64(bits, BITS_PER_LONG - 1UL) / BITS_PER_LONG)

/**
 * @brief Set the bit value.
 *
 * @param i [in]	Bit position to set.
 *
 * Invokes #BIT64 to set the value at bit position indicated by \a i.
 * Deprecated; use the explicit BITxx() macros instead.
 */
#define BIT(i)		BIT64(i)

/**
 * @brief Create a bit mask.
 *
 * @param hi [in]	MSB position of the bit mask.
 * @param lo [in]	LSB position of the bit mask.
 *
 * Creates a bit mask starting at bit position lo and ending at hi.
 */
#define GENMASK(hi, lo) \
	(((~0UL) - (1UL << (lo)) + 1UL) &	\
		(~0UL >> (BITS_PER_LONG - 1UL - (unsigned long)(hi))))

/*
 * Can't use BITS_TO_LONGS to declare arrays where we can't use BUG(), so if the
 * range is invalid, use -1 for the size which will generate a compiler error.
 */

/**
 * @brief Create an array of unsigned long to represent a bitmap.
 *
 * @param bmap [in]	Bitmap to create.
 * @param bits [in]	Number of bits.
 *
 * Creates an array of data type unsigned long by name \a bmap. The size of the
 * array is taken as \a bits value converted into an equivalent rounded up long
 * value if the rounded up value is less than or equal to #LONG_MAX. Otherwise
 * the array declaration will generate a compiler error.
 */
#define DECLARE_BITMAP(bmap, bits)					\
	unsigned long bmap[(((LONG_MAX - (bits)) < (BITS_PER_LONG - 1UL)) ? \
		-1 :							\
		(long int)(((bits) + BITS_PER_LONG - 1UL) /		\
						BITS_PER_LONG))]

/**
 * @brief Loop for each set bit.
 *
 * @param bit [in]	Each set bit, this is the loop index.
 * @param addr [in]	Starting of the bitmap.
 * @param size [in]	Size of the bitmap.
 */
#define for_each_set_bit(bit, addr, size) \
	for ((bit) = find_first_bit((addr), (size));		\
	     (bit) < (size);					\
	     (bit) = find_next_bit((addr), (size), (bit) + 1U))

/**
 * @brief Find first set bit.
 *
 * @param word [in]	Input of datatype long.
 *
 * Function returns the position of the first set bit in \a word.
 *
 * @return Returns one plus the index of the least significant 1-bit of input
 * param \a word. Returns 0 if input param is 0.
 */
unsigned long nvgpu_posix_ffs(unsigned long word);

/**
 * @brief Find last set bit.
 *
 * @param word [in]	Input of datatype long.
 *
 * Function returns the position of the last set bit in \a word.
 *
 * @return Returns one plus the index of the most significant 1-bit of input
 * param word. If input param is 0, returns 0.
 */
unsigned long nvgpu_posix_fls(unsigned long word);

/**
 * Wrapper define for #nvgpu_posix_ffs.
 */
#define nvgpu_ffs(word)	nvgpu_posix_ffs(word)

/**
 * Wrapper define for #nvgpu_posix_fls.
 */
#define nvgpu_fls(word)	nvgpu_posix_fls(word)

/**
 * @brief Find first zero bit.
 *
 * @param word [in]	Input value to search for the bit.
 *
 * Macro to find the position of first zero bit in input data \a word.
 *
 * @return Returns the bit position of first zero bit in the input data.
 */
#define ffz(word)	(nvgpu_ffs(~(word)) - 1UL)

/**
 * @brief Find the first set bit.
 *
 * @param addr [in]	Input value to search for set bit.
 * @param size [in]	Size of the input value in bits.
 *
 * Finds the first set bit position in the input data \a addr.
 *
 * @return Returns the position of first set bit.
 */
unsigned long find_first_bit(const unsigned long *addr, unsigned long size);

/**
 * @brief Finds the next set bit.
 *
 * @param addr [in]	Input value to search for next set bit.
 * @param size [in]	Size of the input value in bits.
 * @param offset [in]	Offset to start from the input data.
 *
 * Finds the next set bit position in the input data \a addr.
 *
 * @return Returns the position of next set bit.
 */
unsigned long find_next_bit(const unsigned long *addr, unsigned long size,
			    unsigned long offset);

/**
 * @brief Finds the first zero bit.
 *
 * @param addr [in]	Input value to search.
 * @param size [in]	Size of the input value in bits.
 *
 * Finds the first zero bit position in the input data \a addr.
 *
 * @return Returns the position of first zero bit.
 */
unsigned long find_first_zero_bit(const unsigned long *addr,
				  unsigned long size);

/**
 * @brief Test the bit value at given position.
 *
 * @param nr [in]	Bit position to check.
 * @param addr [in]	Input data stream.
 *
 * Checks if the bit at position mentioned by \a nr in \a addr is set or not.
 *
 * @return Returns true if bit position \a nr is set, else returns false.
 */
bool nvgpu_test_bit(unsigned int nr, const volatile unsigned long *addr);

/**
 * @brief Test and set the bit at given position.
 *
 * @param nr [in]	Bit position to test and set.
 * @param addr [in]	Input data stream.
 *
 * Tests and sets the bit at position \a nr in \a addr.
 *
 * @return Returns true if the bit position was already set, else returns false.
 */
bool nvgpu_test_and_set_bit(unsigned int nr, volatile unsigned long *addr);

/**
 * @brief Test and clear the bit at given position.
 *
 * @param nr [in]	Bit position to test and clear.
 * @param addr [in]	Input data stream.
 *
 * Tests and clears the bit at position \a nr in \a addr.
 *
 * @return Returns true if the bit position was already set, else returns false.
 */
bool nvgpu_test_and_clear_bit(unsigned int nr, volatile unsigned long *addr);

/*
 * These two are atomic.
 */

/**
 * @brief Sets the bit at given position.
 *
 * @param nr [in]	Bit position to set.
 * @param addr [in]	Input data stream.
 *
 * Sets the bit atomically at bit position \a nr in \a addr.
 */
void nvgpu_set_bit(unsigned int nr, volatile unsigned long *addr);

/**
 * @brief Clears the bit at given position.
 *
 * @param nr [in]	Bit position to clear.
 * @param addr [in]	Input data stream.
 *
 * Clears the bit atomically at bit position \a nr in \a addr.
 */
void nvgpu_clear_bit(unsigned int nr, volatile unsigned long *addr);

/**
 * @brief Sets a bitmap.
 *
 * @param map [in,out]	Input data to set bitmap.
 * @param start [in]	Start position of the bitmap.
 * @param len [in]	Length of the bitmap.
 *
 * Sets a bitmap of length \a len starting from bit position \a start in
 * \a map.
 */
void nvgpu_bitmap_set(unsigned long *map, unsigned int start, unsigned int len);

/**
 * @brief Clears a bitmap.
 *
 * @param map [in,out]	Input data to clear bitmap.
 * @param start [in]	Start position of the bitmap.
 * @param len [in]	Length of the bitmap.
 *
 * Clears a bitmap of length \a len starting from bit position \a start in
 * \a map.
 */
void nvgpu_bitmap_clear(unsigned long *map, unsigned int start,
			unsigned int len);

/**
 * @brief Find first bitmap space from an offset.
 *
 * @param map [in]		Input data stream.
 * @param size [in]		Size of the input data.
 * @param start [in]		Start position in input.
 * @param nr [in]		Number of bits in bitmap.
 * @param align_mask [in]	Align mask for start.
 * @param align_offset [in]	Align offset from Input data.
 *
 * Finds the first space of contiguous zeros in input data stream which can
 * accommodate a bitmap of length \a nr starting from position \a start.
 *
 * @return Returns the position at which the bitmap starts if enough free space
 * is present in the input stream to accommodate the bitmap; otherwise, return
 * the size of the input data.
 */
unsigned long bitmap_find_next_zero_area_off(unsigned long *map,
					     unsigned long size,
					     unsigned long start,
					     unsigned int nr,
					     unsigned long align_mask,
					     unsigned long align_offset);

/**
 * @brief Find first bitmap space.
 *
 * @param map [in]		Input data stream.
 * @param size [in]		Size of the input data.
 * @param start [in]		Start position in input.
 * @param nr [in]		Number of bits in bitmap.
 * @param align_mask [in]	Align mask for start.
 *
 * Searches a bitmap for the first space of contiguous zeros that is large
 * enough to accommodate the requested number of bits.
 *
 * @return Returns the position at which the bitmap starts if enough free space
 * is present in the input stream to accommodate the bitmap; otherwise, return
 * the size of the input data.
 */
unsigned long bitmap_find_next_zero_area(unsigned long *map,
					 unsigned long size,
					 unsigned long start,
					 unsigned int nr,
					 unsigned long align_mask);

#endif /* NVGPU_POSIX_BITOPS_H */
