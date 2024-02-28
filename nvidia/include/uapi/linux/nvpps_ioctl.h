/*
 * include/uapi/linux/nvpps_ioctl.h
 *
 * Copyright (c) 2018-2022, NVIDIA CORPORATION. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __UAPI_NVPPS_IOCTL_H__
#define __UAPI_NVPPS_IOCTL_H__

#include <linux/types.h>
#include <linux/ioctl.h>

struct nvpps_version {
	struct _version {
		__u32	major;
		__u32	minor;
	} version;
	struct _api {
		__u32	major;
		__u32	minor;
	} api;
};

#define NVPPS_VERSION_MAJOR	0
#define NVPPS_VERSION_MINOR	2
#define NVPPS_API_MAJOR		0
#define NVPPS_API_MINOR         4

struct nvpps_params {
	__u32	evt_mode;
	__u32	tsc_mode;
};


/* evt_mode */
#define NVPPS_MODE_GPIO		0x01
#define NVPPS_MODE_TIMER	0x02

/* tsc_mode */
#define NVPPS_TSC_NSEC		0
#define NVPPS_TSC_COUNTER	1


struct nvpps_timeevent {
	__u32	evt_nb;
	__u64	tsc;
	__u64	ptp;
	__s64	ptp_offset;
	__u64	tsc_res_ns;
	__u32	evt_mode;
	__u32	tsc_mode;
	__u64	irq_latency;
};

#ifndef _LINUX_TIME64_H
typedef __s64 time64_t;
typedef __u64 timeu64_t;

struct timespec64 {
	time64_t	tv_sec;			/* seconds */
	long		tv_nsec;		/* nanoseconds */
};
#endif

struct nvpps_timestamp_struct {
	clockid_t	clockid;
	struct timespec64 kernel_ts;
        struct timespec64 hw_ptp_ts;
	__u64		extra[2];
};


#define NVPPS_GETVERSION	_IOR('p', 0x1, struct nvpps_version *)
#define NVPPS_GETPARAMS		_IOR('p', 0x2, struct nvpps_params *)
#define NVPPS_SETPARAMS		_IOW('p', 0x3, struct nvpps_params *)
#define NVPPS_GETEVENT		_IOR('p', 0x4, struct nvpps_timeevent *)
#define NVPPS_GETTIMESTAMP	_IOWR('p', 0x5, struct nvpps_timestamp_struct *)

#endif /* __UAPI_NVPPS_IOCTL_H__ */
