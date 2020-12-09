/*
 * Copyright (c) 2016-2020, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_TIMERS_H
#define NVGPU_TIMERS_H

#include <nvgpu/types.h>
#include <nvgpu/utils.h>

#ifndef __KERNEL__
#include <nvgpu/posix/timers.h>
#endif

struct gk20a;

/**
 * struct nvgpu_timeout - define a timeout.
 *
 * There are two types of timer supported:
 *
 *   o  NVGPU_TIMER_CPU_TIMER
 *        Timer uses the CPU to measure the timeout.
 *
 *   o  NVGPU_TIMER_RETRY_TIMER
 *        Instead of measuring a time limit keep track of the number of times
 *        something has been attempted. After said limit, "expire" the timer.
 *
 * Available flags:
 *
 *   o  NVGPU_TIMER_NO_PRE_SI
 *        By default when the system is not running on silicon the timeout
 *        code will ignore the requested timeout. Specifying this flag will
 *        override that behavior and honor the timeout regardless of platform.
 *
 *   o  NVGPU_TIMER_SILENT_TIMEOUT
 *        Do not print any messages on timeout. Normally a simple message is
 *        printed that specifies where the timeout occurred.
 */
struct nvgpu_timeout {
	/**
	 * GPU driver structure.
	 */
	struct gk20a		*g;
	/**
	 * Flags for timer.
	 */
	unsigned int		 flags;
	/**
	 * Timeout duration/count.
	 */
	union {
		s64		 time;
		struct {
			u32	 max_attempts;
			u32	 attempted;
		} retries;
	};
};

/**
 * Value for Bit 0 indicating a CPU timer.
 */
#define NVGPU_TIMER_CPU_TIMER		(0x0U)

/**
 * Value for Bit 0 indicating a retry timer.
 */
#define NVGPU_TIMER_RETRY_TIMER		(0x1U)

/**
 * @defgroup NVRM_GPU_NVGPU_DRIVER_TIMER_FLAGS Timer flags
 *
 * Bits 1 through 7 are reserved; bits 8 and up are flags:
 */

/**
 * @ingroup NVRM_GPU_NVGPU_DRIVER_TIMER_FLAGS
 *
 * Flag to enforce timeout check for pre silicon platforms.
 */
#define NVGPU_TIMER_NO_PRE_SI		BIT32(8)

/**
 * @ingroup NVRM_GPU_NVGPU_DRIVER_TIMER_FLAGS
 *
 * Flag to enforce silent timeout.
 */
#define NVGPU_TIMER_SILENT_TIMEOUT	BIT32(9)

/**
 * Mask value for timer flag bits.
 */
#define NVGPU_TIMER_FLAG_MASK		(NVGPU_TIMER_RETRY_TIMER |	\
					 NVGPU_TIMER_NO_PRE_SI |	\
					 NVGPU_TIMER_SILENT_TIMEOUT)

/**
 * @brief Initialize a timeout.
 *
 * @param g [in]	GPU driver structure.
 * @param timeout [in]	Timeout object to initialize.
 * @param duration [in]	Timeout duration/count.
 * @param flags [in]	Flags for timeout object.
 *
 * Initialize a timeout object referenced by \a timeout.
 *
 * @return Shall return 0 on success; otherwise, return error number to indicate
 * the error type.
 *
 * @retval -EINVAL invalid input parameter.
 */
int nvgpu_timeout_init(struct gk20a *g, struct nvgpu_timeout *timeout,
		       u32 duration, unsigned long flags);

/**
 * @brief Check the timeout status.
 *
 * @param timeout [in]	Timeout object to check the status.
 *
 * Checks the status of \a timeout and returns if the timeout has expired or
 * not.
 *
 * @return Boolean value to indicate the status of timeout.
 *
 * @retval TRUE if the timeout has expired.
 * @retval FALSE if the timeout has not expired.
 */
bool nvgpu_timeout_peek_expired(struct nvgpu_timeout *timeout);

/**
 * @brief Checks the timeout expiry according to the timer type.
 *
 * @param __timeout [in]	Timeout object to handle.
 *
 * This macro checks to see if a timeout has expired. For retry based timers,
 * a call of this macro increases the retry count by one and checks if the
 * retry count has reached maximum allowed retry limit. For CPU based timers,
 * a call of this macro checks whether the required duration has elapsed.
 * Refer to the documentation of the function #nvgpu_timeout_expired_msg_impl
 * for underlying implementation.
 */
#define nvgpu_timeout_expired(__timeout)				\
	nvgpu_timeout_expired_msg_impl(__timeout, NVGPU_GET_IP, "")

/**
 * @brief Checks the timeout expiry and uses the input params for debug message.
 *
 * @param __timeout [in]	Timeout object to handle.
 * @param fmt [in]		Format of the variable arguments.
 * @param args... [in]		Variable arguments.
 *
 * Along with handling the timeout, this macro also takes in a variable list
 * of arguments which is used in constructing the debug message for a timeout.
 * Refer to #nvgpu_timeout_expired for further details.
 */
#define nvgpu_timeout_expired_msg(__timeout, fmt, args...)		\
	nvgpu_timeout_expired_msg_impl(__timeout, NVGPU_GET_IP,	fmt, ##args)

/*
 * Waits and delays.
 */

/**
 * @brief Sleep for millisecond intervals.
 *
 * @param msecs [in]	Milliseconds to sleep.
 *
 * Function sleeps for requested millisecond duration.
 */
void nvgpu_msleep(unsigned int msecs);

/**
 * @brief Sleep for a duration in the range of input parameters.
 *
 * @param min_us [in]	Minimum duration to sleep in microseconds.
 * @param max_us [in]	Maximum duration to sleep in microseconds.
 *
 * Sleeps for a value in the range between min_us and max_us. The underlying
 * implementation is OS dependent. In nvgpu posix implementation, the sleep is
 * always for min_us duration and the function sleeps only if the input min_us
 * value is greater than or equal to 1000 microseconds, else the function just
 * delays execution for requested duration of time without sleeping.
 */
void nvgpu_usleep_range(unsigned int min_us, unsigned int max_us);

/**
 * @brief Delay execution.
 *
 * @param usecs [in]	Delay duration in microseconds.
 *
 * Delays the execution for requested duration of time in microseconds. In
 * posix implementation, if the requested duration is greater than or equal to
 * 1000 microseconds, the function sleeps.
 */
void nvgpu_udelay(unsigned int usecs);

/*
 * Timekeeping.
 */

/**
 * @brief Current time in milliseconds.
 *
 * @return Current time value as milliseconds.
 */
s64 nvgpu_current_time_ms(void);

/**
 * @brief Current time in nanoseconds.
 *
 * @return Current time value as nanoseconds.
 */
s64 nvgpu_current_time_ns(void);

/**
 * @brief Current time in microseconds.
 *
 * @return Current time value as microseconds.
 */
s64 nvgpu_current_time_us(void);
#ifdef CONFIG_NVGPU_NON_FUSA
u64 nvgpu_us_counter(void);

/**
 * @brief Get GPU cycles.
 *
 * @param None.
 *
 * @return 64 bit number which has GPU cycles.
 */
u64 nvgpu_get_cycles(void);

/**
 * @brief Current high resolution time stamp in microseconds.
 *
 * @return Returns the high resolution time stamp value converted into
 * microseconds.
 */
u64 nvgpu_hr_timestamp_us(void);

/**
 * @brief Current high resolution time stamp.
 *
 * @return High resolution time stamp value.
 */
u64 nvgpu_hr_timestamp(void);
#endif

/**
 * @brief OS specific implementation to provide precise microsecond delay
 *
 * @param usecs [in]		Delay in microseconds.
 *
 * - Wait using nanospin_ns until usecs expires. Log error if API returns non
 *   zero value once wait time expires.
 *
 * @return None.
 */
void nvgpu_delay_usecs(unsigned int usecs);

#ifdef __KERNEL__
/**
 * @brief Private handler of kernel timeout, should not be used directly.
 *
 * @param timeout [in]	Timeout object.
 * @param caller [in]	Instruction pointer of the caller.
 * @param fmt [in]	Format of the variable length argument.
 * @param ... [in]	Variable length arguments.
 *
 * Implements the timeout handler functionality for kernel. Differentiates
 * between a CPU timer and a retry timer and handles accordingly.
 *
 * @return Shall return 0 if the timeout has not expired; otherwise, an error
 * number indicating a timeout is returned.
 */
int nvgpu_timeout_expired_msg_impl(struct nvgpu_timeout *timeout,
			      void *caller, const char *fmt, ...);
#endif

#endif /* NVGPU_TIMERS_H */
