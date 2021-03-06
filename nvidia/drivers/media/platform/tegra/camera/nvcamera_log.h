/*
 * nvcamera_log.h - general tracing function for vi and isp API calls
 *
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __NVCAMERA_LOG_H
#define __NVCAMERA_LOG_H

#include <linux/types.h>

struct platform_device;

void nv_camera_log_submit(struct platform_device *pdev,
		u32 syncpt_id,
		u32 syncpt_thresh,
		u32 channel_id,
		u64 timestamp);

void nv_camera_log(struct platform_device *pdev,
		u64 timestamp,
		u32 type);

#endif
