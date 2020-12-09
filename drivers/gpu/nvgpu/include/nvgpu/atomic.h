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
#ifndef NVGPU_ATOMIC_H
#define NVGPU_ATOMIC_H

#ifdef __KERNEL__
#include <nvgpu/linux/atomic.h>
#else
#include <nvgpu/posix/atomic.h>
#endif

/** Initialize nvgpu atomic structure. */
#define NVGPU_ATOMIC_INIT(i)	nvgpu_atomic_init_impl(i)
/** Initialize 64 bit nvgpu atomic structure. */
#define NVGPU_ATOMIC64_INIT(i)	nvgpu_atomic64_init_impl(i)

/**
 * @brief Set atomic variable.
 *
 * @param v [in]	Structure holding atomic variable.
 * @param i [in]	Value to set.
 *
 * Sets the value \a i atomically in structure \a v.
 */
static inline void nvgpu_atomic_set(nvgpu_atomic_t *v, int i)
{
	nvgpu_atomic_set_impl(v, i);
}

/**
 * @brief Read atomic variable.
 *
 * @param v [in]	Structure holding atomic variable to read.
 *
 * Atomically reads the value from structure \a v.
 *
 * @return Value read from the atomic variable.
 */
static inline int nvgpu_atomic_read(nvgpu_atomic_t *v)
{
	return nvgpu_atomic_read_impl(v);
}

/**
 * @brief Increment the atomic variable.
 *
 * @param v [in]	Structure holding atomic variable.
 *
 * Atomically increments the value in structure \a v.
 */
static inline void nvgpu_atomic_inc(nvgpu_atomic_t *v)
{
	nvgpu_atomic_inc_impl(v);
}

/**
 * @brief Increment the atomic variable and return.
 *
 * @param v [in]	Structure holding atomic variable.
 *
 * Atomically increases the value in structure \a v and returns.
 *
 * @return Value in atomic variable incremented by 1.
 */
static inline int nvgpu_atomic_inc_return(nvgpu_atomic_t *v)
{
	return nvgpu_atomic_inc_return_impl(v);
}

/**
 * @brief Decrement the atomic variable.
 *
 * @param v [in]	Structure holding atomic variable.
 *
 * Atomically decrements the value in structure \a v.
 */
static inline void nvgpu_atomic_dec(nvgpu_atomic_t *v)
{
	nvgpu_atomic_dec_impl(v);
}

/**
 * @brief Decrement the atomic variable and return.
 *
 * @param v [in]	Structure holding atomic variable.
 *
 * Atomically decrements the value in structure v and returns.
 *
 * @return Value in atomic variable decremented by 1.
 */
static inline int nvgpu_atomic_dec_return(nvgpu_atomic_t *v)
{
	return nvgpu_atomic_dec_return_impl(v);
}

/**
 * @brief Compare and exchange the value in atomic variable.
 *
 * @param v [in]	Structure holding atomic variable.
 * @param old [in]	Value to compare.
 * @param new [in]	Value to store.
 *
 * Reads the value stored in \a v, replace the value with \a new if the read
 * value is equal to \a old. In cases where the current value in \a v is not
 * equal to \a old, this function acts as a read operation of atomic variable.
 *
 * @return Reads the value in \a v before the exchange operation and returns.
 * Read value is returned irrespective of whether the exchange operation is
 * carried out or not.
 */
static inline int nvgpu_atomic_cmpxchg(nvgpu_atomic_t *v, int old, int new)
{
	return nvgpu_atomic_cmpxchg_impl(v, old, new);
}

/**
 * @brief Exchange the value in atomic variable.
 *
 * @param v [in]	Structure holding atomic variable.
 * @param new [in]	Value to set.
 *
 * Atomically exchanges the value stored in \a v with \a new.
 *
 * @return Value in atomic variable before the operation.
 */
static inline int nvgpu_atomic_xchg(nvgpu_atomic_t *v, int new)
{
	return nvgpu_atomic_xchg_impl(v, new);
}

/**
 * @brief Increment the atomic variable and test for zero.
 *
 * @param v [in]	Structure holding atomic variable.
 *
 * Atomically increments the value stored in \a v and compare the result with
 * zero.
 *
 * @return Boolean value indicating the status of comparison operation done
 * after incrementing the atomic variable.
 *
 * @retval TRUE if the increment operation results in zero.
 * @retval FALSE if the increment operation results in a non-zero value.
 */
static inline bool nvgpu_atomic_inc_and_test(nvgpu_atomic_t *v)
{
	return nvgpu_atomic_inc_and_test_impl(v);
}

/**
 * @brief Decrement the atomic variable and test for zero.
 *
 * @param v [in]	Structure holding atomic variable.
 *
 * Atomically decrements the value stored in \a v and compare the result with
 * zero.
 *
 * @return Boolean value indicating the status of comparison operation done
 * after decrementing the atomic variable.
 *
 * @retval TRUE if the decrement operation results in zero.
 * @retval FALSE if the decrement operation results in a non-zero value.
 */
static inline bool nvgpu_atomic_dec_and_test(nvgpu_atomic_t *v)
{
	return nvgpu_atomic_dec_and_test_impl(v);
}

/**
 * @brief Subtract integer from atomic variable and test.
 *
 * @param i [in]	Value to subtract.
 * @param v [in]	Structure holding atomic variable.
 *
 * Atomically subtracts \a i from the value stored in \a v and compare the
 * result with zero.
 *
 * @return Boolean value indicating the status of comparison operation done
 * after the subtraction operation on atomic variable.
 *
 * @retval TRUE if the operation results in zero.
 * @retval FALSE if the operation results in a non-zero value.
 */
static inline bool nvgpu_atomic_sub_and_test(int i, nvgpu_atomic_t *v)
{
	return nvgpu_atomic_sub_and_test_impl(i, v);
}

/**
 * @brief Add integer to atomic variable.
 *
 * @param i [in]	Value to add.
 * @param v [in]	Structure holding atomic variable.
 *
 * Atomically adds \a i to the value stored in structure \a v.
 */
static inline void nvgpu_atomic_add(int i, nvgpu_atomic_t *v)
{
	nvgpu_atomic_add_impl(i, v);
}

/**
 * @brief Subtract integer form atomic variable and return.
 *
 * @param i [in]	Value to subtract.
 * @param v [in]	Structure holding atomic variable.
 *
 * Atomically subtracts \a i from the value stored in structure \a v and
 * returns.
 *
 * @return \a i subtracted from \a v is returned.
 */
static inline int nvgpu_atomic_sub_return(int i, nvgpu_atomic_t *v)
{
	return nvgpu_atomic_sub_return_impl(i, v);
}

/**
 * @brief Subtract integer from atomic variable.
 *
 * @param i [in]	Value to set.
 * @param v [in]	Structure holding atomic variable.
 *
 * Atomically subtracts \a i from the value stored in structure \a v.
 */
static inline void nvgpu_atomic_sub(int i, nvgpu_atomic_t *v)
{
	nvgpu_atomic_sub_impl(i, v);
}

/**
 * @brief Add integer to atomic variable and return.
 *
 * @param i [in]	Value to add.
 * @param v [in]	Structure holding atomic variable.
 *
 * Atomically adds \a i to the value in structure \a v and returns.
 *
 * @return Current value in \a v added with \a i is returned.
 */
static inline int nvgpu_atomic_add_return(int i, nvgpu_atomic_t *v)
{
	return nvgpu_atomic_add_return_impl(i, v);
}

/**
 * @brief Add integer to atomic variable on unless condition.
 *
 * @param v [in]	Structure holding atomic variable.
 * @param a [in]	Value to add.
 * @param u [in]	Value to check.
 *
 * Atomically adds \a a to \a v if \a v is not \a u.
 *
 * @return Value in the atomic variable before the operation.
 */
static inline int nvgpu_atomic_add_unless(nvgpu_atomic_t *v, int a, int u)
{
	return nvgpu_atomic_add_unless_impl(v, a, u);
}

/**
 * @brief Set the 64 bit atomic variable.
 *
 * @param v [in]	Structure holding atomic variable.
 * @param i [in]	Value to set.
 *
 * Atomically sets the 64 bit value \a i in structure \a v.
 */
static inline void nvgpu_atomic64_set(nvgpu_atomic64_t *v, long x)
{
	return  nvgpu_atomic64_set_impl(v, x);
}

/**
 * @brief Read the 64 bit atomic variable.
 *
 * @param v [in]	Structure holding atomic variable.
 *
 * Atomically reads the 64 bit value in structure \a v.
 *
 * @return Value read from the atomic variable.
 */
static inline long nvgpu_atomic64_read(nvgpu_atomic64_t *v)
{
	return  nvgpu_atomic64_read_impl(v);
}

/**
 * @brief Addition of 64 bit integer to atomic variable.
 *
 * @param x [in]	Value to add.
 * @param v [in]	Structure holding atomic variable.
 *
 * Atomically adds the 64 bit value \a x to \a v.
 */
static inline void nvgpu_atomic64_add(long x, nvgpu_atomic64_t *v)
{
	nvgpu_atomic64_add_impl(x, v);
}

/**
 * @brief Increment the 64 bit atomic variable.
 *
 * @param v [in]	Structure holding atomic variable.
 *
 * Atomically increments the value in structure \a v.
 */
static inline void nvgpu_atomic64_inc(nvgpu_atomic64_t *v)
{
	nvgpu_atomic64_inc_impl(v);
}

/**
 * @brief  Increment the 64 bit atomic variable and return.
 *
 * @param v [in]	Structure holding atomic variable.
 *
 * Atomically increments the value in structure \a v and returns.
 *
 * @return Value in atomic variable incremented by 1.
 */
static inline long nvgpu_atomic64_inc_return(nvgpu_atomic64_t *v)
{
	return nvgpu_atomic64_inc_return_impl(v);
}

/**
 * @brief Decrement the 64 bit atomic variable.
 *
 * @param v [in]	Structure holding atomic variable.
 *
 * Atomically decrements the value in structure \a v.
 */
static inline void nvgpu_atomic64_dec(nvgpu_atomic64_t *v)
{
	nvgpu_atomic64_dec_impl(v);
}

/**
 * @brief Decrement the 64 bit atomic variable and return.
 *
 * @param v [in]	Structure holding atomic variable.
 *
 * Atomically decrements the value in structure \a v and returns.
 *
 * @return Value in atomic variable decremented by 1.
 */
static inline long nvgpu_atomic64_dec_return(nvgpu_atomic64_t *v)
{
	return nvgpu_atomic64_dec_return_impl(v);
}

/**
 * @brief Exchange the 64 bit atomic variable value.
 *
 * @param v [in]	Structure holding atomic variable.
 * @param new [in]	Value to set.
 *
 * Atomically exchanges the value \a new with the value in \a v.
 *
 * @return Value in the atomic variable before the exchange operation.
 */
static inline long nvgpu_atomic64_xchg(nvgpu_atomic64_t *v, long new)
{
	return nvgpu_atomic64_xchg_impl(v, new);
}

/**
 * @brief Compare and exchange the 64 bit atomic variable value.
 *
 * @param v [in]	Structure holding atomic variable.
 * @param old [in]	Value to compare.
 * @param new [in]	Value to exchange.

 * Reads the value stored in \a v, replace the value with \a new if the read
 * value is equal to \a old. In cases where the current value in \a v is not
 * equal to \a old, this function acts as a read operation of atomic variable.
 *
 * @return Returns the original value in atomic variable.
 */
static inline long nvgpu_atomic64_cmpxchg(nvgpu_atomic64_t *v, long old,
					long new)
{
	return nvgpu_atomic64_cmpxchg_impl(v, old, new);
}

/**
 * @brief Addition of 64 bit integer to atomic variable and return.
 *
 * @param x [in]	Value to add.
 * @param v [in]	Structure holding atomic variable.
 *
 * Atomically add the value x with v and returns.
 *
 * @return Current value in atomic variable added with \a x.
 */
static inline long nvgpu_atomic64_add_return(long x, nvgpu_atomic64_t *v)
{
	return nvgpu_atomic64_add_return_impl(x, v);
}

/**
 * @brief Addition of 64 bit atomic variable on unless condition.
 *
 * @param v [in]	Structure holding atomic variable.
 * @param a [in]	Value to add.
 * @param u [in]	Value to check.
 *
 * Atomically adds 64 bit value \a a to \a v if \a v is not \a u.
 *
 * @return Value in atomic variable before the operation.
 */
static inline long nvgpu_atomic64_add_unless(nvgpu_atomic64_t *v, long a,
						long u)
{
	return nvgpu_atomic64_add_unless_impl(v, a, u);
}

/**
 * @brief Subtraction of 64 bit integer from atomic variable.
 *
 * @param x [in]	Value to subtract.
 * @param v [in]	Structure holding atomic variable.
 *
 * Atomically subtracts the 64 bit value \a x from structure \a v.
 */
static inline void nvgpu_atomic64_sub(long x, nvgpu_atomic64_t *v)
{
	nvgpu_atomic64_sub_impl(x, v);
}

/**
 * @brief Increment the 64 bit atomic variable and test.
 *
 * @param v [in]	Structure holding atomic variable.
 *
 * Atomically increments the value stored in \a v and compare the result with
 * zero.
 *
 * @return Boolean value indicating the status of comparison operation done
 * after incrementing the atomic variable.
 *
 * @retval TRUE if the operation results in zero.
 * @retval FALSE if the operation results in a non-zero value.
 */
static inline bool nvgpu_atomic64_inc_and_test(nvgpu_atomic64_t *v)
{
	return nvgpu_atomic64_inc_and_test_impl(v);
}

/**
 * @brief Decrement the 64 bit atomic variable and test.
 *
 * @param v [in]	Structure holding atomic variable.
 *
 * Atomically decrements the value stored in \a v and compare the result with
 * zero.
 *
 * @return Boolean value indicating the status of comparison operation done
 * after decrementing the atomic variable.
 *
 * @retval TRUE if the operation results in zero.
 * @retval FALSE if the operation results in a non-zero value.
 */
static inline bool nvgpu_atomic64_dec_and_test(nvgpu_atomic64_t *v)
{
	return nvgpu_atomic64_dec_and_test_impl(v);
}

/**
 * @brief Subtract 64 bit integer from atomic variable and test.
 *
 * @param x [in]	Value to subtract.
 * @param v [in]	Structure holding atomic variable.
 *
 * Atomically subtracts \a x from \a v and compare the result with zero.
 *
 * @return Boolean value indicating the status of comparison operation done
 * after the subtraction operation on atomic variable.
 *
 * @retval TRUE if the operation results in zero.
 * @retval FALSE if the operation results in a non-zero value.
 */
static inline bool nvgpu_atomic64_sub_and_test(long x, nvgpu_atomic64_t *v)
{
	return nvgpu_atomic64_sub_and_test_impl(x, v);
}

/**
 * @brief Subtract 64 bit integer from atomic variable and return.
 *
 * @param x [in]	Value to subtract.
 * @param v [in]	Structure holding atomic variable.
 *
 * Atomically subtracts \a x from \a v and returns.
 *
 * @return \a x subtracted from the current value in atomic variable.
 */
static inline long nvgpu_atomic64_sub_return(long x, nvgpu_atomic64_t *v)
{
	return nvgpu_atomic64_sub_return_impl(x, v);
}

#endif /* NVGPU_ATOMIC_H */
