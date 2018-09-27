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

#ifndef __NVMAP_DEBUGFS_H
#define __NVMAP_DEBUGFS_H

#include <linux/debugfs.h>

static inline unsigned int NVMAP_IRUSR(void) {
	return  S_IRUSR;
}

static inline unsigned int NVMAP_IWUSR(void) {
	return  S_IWUSR;
}

static inline unsigned int NVMAP_IRUGO(void) {
	return  S_IRUGO;
}

static inline unsigned int NVMAP_IWUGO(void) {
	return  S_IWUGO;
}

#define NVMAP_L1_CACHE_BYTES	L1_CACHE_BYTES

#endif
