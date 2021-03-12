/*
 * Copyright (c) 2017-2021, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/io.h>

#include "os_linux.h"

u32 nvgpu_os_readl(uintptr_t addr)
{
	return readl((void __iomem *)addr);
}

void nvgpu_os_writel(u32 v, uintptr_t addr)
{
	writel(v, (void __iomem *)addr);
}

void nvgpu_os_writel_relaxed(u32 v, uintptr_t addr)
{
	writel_relaxed(v, (void __iomem *)addr);
}
