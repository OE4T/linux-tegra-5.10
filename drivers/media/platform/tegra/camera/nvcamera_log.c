/*
 * nvcamera_log.c - general tracing function for vi and isp API calls
 *
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
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


#include "nvcamera_log.h"
#include <uapi/linux/nvhost_events.h>

#if defined(CONFIG_EVENTLIB) && defined(KERNEL_EVENTLIB_TRACES)
#include <linux/keventlib.h>

void nv_camera_log(struct platform_device *pdev,
				u64 timestamp,
				u32 type)
{
	struct nvhost_device_data *pdata = platform_get_drvdata(pdev);
	struct nv_camera_task_log task_log;

	if (!pdata->eventlib_id)
		return;

	/*
	 * Write task start event
	 */
	task_log.class_id = pdata->class;
	task_log.pid = current->tgid;
	task_log.tid = current->pid;

	keventlib_write(pdata->eventlib_id,
			&task_log,
			sizeof(task_log),
			type,
			timestamp);
}
#else

void nv_camera_log(struct platform_device *pdev,
				u64 timestamp,
				u32 type)
{
}
#endif
EXPORT_SYMBOL(nv_camera_log);
