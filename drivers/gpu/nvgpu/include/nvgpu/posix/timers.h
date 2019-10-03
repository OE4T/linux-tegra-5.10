/*
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_POSIX_TIMERS_H
#define NVGPU_POSIX_TIMERS_H

#include <sys/time.h>
#include <time.h>

#include <nvgpu/types.h>
#include <nvgpu/log.h>


#define nvgpu_timeout_expired_msg_cpu(timeout, caller, fmt, arg...)	\
({									\
	struct nvgpu_timeout *t_ptr = (timeout);			\
	int ret = 0;							\
	if (nvgpu_current_time_ns() > t_ptr->time) {			\
		if ((t_ptr->flags & NVGPU_TIMER_SILENT_TIMEOUT) == 0U) { \
			nvgpu_err(t_ptr->g, "Timeout detected @ %p" fmt, \
							caller, ##arg);	\
		}							\
		ret = -ETIMEDOUT;					\
	}								\
	(int)ret;							\
})

#define nvgpu_timeout_expired_msg_retry(timeout, caller, fmt, arg...)	\
({									\
	struct nvgpu_timeout *t_ptr = (timeout);			\
	int ret = 0;							\
	if (t_ptr->retries.attempted >= t_ptr->retries.max_attempts) {	\
		if ((t_ptr->flags & NVGPU_TIMER_SILENT_TIMEOUT) == 0U) { \
			nvgpu_err(t_ptr->g, "No more retries @ %p" fmt,	\
							caller, ##arg);	\
		}							\
		ret = -ETIMEDOUT;					\
	} else {							\
		t_ptr->retries.attempted++;				\
	}								\
	(int)ret;							\
})

#define nvgpu_timeout_expired_msg_impl(timeout, caller, fmt, arg...)	\
({									\
	int ret = 0;							\
	if (((timeout)->flags & NVGPU_TIMER_RETRY_TIMER) != 0U) {	\
		ret = nvgpu_timeout_expired_msg_retry((timeout), caller,\
							fmt, ##arg);	\
	} else {							\
		ret = nvgpu_timeout_expired_msg_cpu((timeout), caller,	\
							fmt, ##arg);	\
	}								\
	(int)ret;							\
})

#endif
