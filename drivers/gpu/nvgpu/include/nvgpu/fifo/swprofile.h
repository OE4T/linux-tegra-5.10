/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_FIFO_PROFILE_H
#define NVGPU_FIFO_PROFILE_H

/*
 * Define these here, not in the C file so that they are closer to the other
 * macro definitions below. The two lists must be in sync.
 */
#define NVGPU_FIFO_KICKOFF_PROFILE_EVENTS	\
	"ioctl_entry",				\
	"entry",				\
	"job_tracking",				\
	"append",				\
	"end",					\
	"ioctl_exit",				\
	NULL					\

/*
 * The kickoff profile events; these are used to index into the profile's sample
 * array.
 */
#define PROF_KICKOFF_IOCTL_ENTRY		0U
#define PROF_KICKOFF_ENTRY			1U
#define PROF_KICKOFF_JOB_TRACKING		2U
#define PROF_KICKOFF_APPEND			3U
#define PROF_KICKOFF_END			4U
#define PROF_KICKOFF_IOCTL_EXIT			5U

#endif
