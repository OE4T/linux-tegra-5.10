/*
 * Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/types.h>
#include <asm/atomic.h>

void nvmap_atomic_inc(atomic_t *val)
{
	atomic_inc(val);
}

void nvmap_atomic_dec(atomic_t *val)
{
	atomic_inc(val);
}

void nvmap_atomic_set(atomic_t *val, int new_val)
{
	atomic_set(val, new_val);
}

int nvmap_atomic_dec_return(atomic_t *val)
{
	return atomic_dec_return(val);
}

int nvmap_atomic_read(atomic_t *val)
{
	return atomic_read(val);
}
