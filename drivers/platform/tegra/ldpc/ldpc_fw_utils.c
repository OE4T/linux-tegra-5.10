/*
 * Copyright (c) 2020, NVIDIA Corporation. All rights reserved.
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

#include <linux/io.h>

#include "ldpc_fw_utils.h"

static void __iomem *get_aperture(struct ldpc_devdata *pdata, int index)
{
	return pdata->aperture[index];
}

u32 engineRead(struct ldpc_devdata *pdata, u32 r)
{
	void __iomem *addr = get_aperture(pdata, 0) + r;
	u32 v;

	v = readl(addr);

	return v;
}

void engineWrite(struct ldpc_devdata *pdata, u32 r, u32 v)
{
	void __iomem *addr = get_aperture(pdata, 0) + r;

	writel(v, addr);
}
