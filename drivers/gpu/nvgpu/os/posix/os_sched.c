/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include <nvgpu/os_sched.h>

#include <unistd.h>
#include <pthread.h>

#define CURRENT_NAME_LEN 30

int nvgpu_current_pid(struct gk20a *g)
{
	/*
	 * In the kernel this gets us the PID of the calling process for IOCTLs.
	 * But since we are in userspace this doesn't quite mean the same thing.
	 * This simply returns the PID of the currently running process.
	 */
	return (int)getpid();
}

int nvgpu_current_tid(struct gk20a *g)
{
	/*
	 * In POSIX thread ID is not the same as a process ID. In Linux threads
	 * and processes are represented by the same thing, but userspace can't
	 * really rely on that.
	 *
	 * We can, however, get a pthread_t for a given thread. But this
	 * pthread_t need not have any relation to the underlying system's
	 * representation of "threads".
	 */
	return (int)pthread_self();
}

void nvgpu_print_current_impl(struct gk20a *g, const char *func_name, int line,
		void *ctx, enum nvgpu_log_type type)
{
	const char *log_message = "(unknown process)";

#if defined(__NVGPU_POSIX__)
#ifdef _GNU_SOURCE
	char current_tname[CURRENT_NAME_LEN];

	/* pthread_getname_np() will return null terminated string on success */
	if (pthread_getname_np(0, current_tname, CURRENT_NAME_LEN) == 0) {
		log_message = current_tname;
	}
#else
	type = NVGPU_ERROR;
#endif
#else
	char current_tname[CURRENT_NAME_LEN];

	/* pthread_getname_np() will return null terminated string on success */
	if (pthread_getname_np(0, current_tname, CURRENT_NAME_LEN) == 0) {
		log_message = current_tname;
	}
#endif

	switch (type) {
	case NVGPU_ERROR:
		nvgpu_err(g, "%s", log_message);
		break;
	case NVGPU_WARNING:
		nvgpu_warn(g, "%s", log_message);
		break;
	case NVGPU_DEBUG:
		nvgpu_log(g, 0U, "%s", log_message);
		break;
	case NVGPU_INFO:
		nvgpu_info(g, "%s", log_message);
		break;
	default:
		nvgpu_err(g, "%s", log_message);
		break;
	}
}
