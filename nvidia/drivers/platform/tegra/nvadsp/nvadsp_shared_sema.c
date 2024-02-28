/*
 * nvadsp_shared_sema.c
 *
 * ADSP Shared Semaphores
 *
 * Copyright (C) 2014 NVIDIA Corporation. All rights reserved.
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

#include <linux/tegra_nvadsp.h>

nvadsp_shared_sema_t *
nvadsp_shared_sema_init(uint8_t nvadsp_shared_sema_id)
{
	return NULL;
}

status_t nvadsp_shared_sema_destroy(nvadsp_shared_sema_t *sema)
{
	return -ENOENT;
}

status_t nvadsp_shared_sema_acquire(nvadsp_shared_sema_t *sema)
{
	return -ENOENT;
}

status_t nvadsp_shared_sema_release(nvadsp_shared_sema_t *sema)
{
	return -ENOENT;
}

