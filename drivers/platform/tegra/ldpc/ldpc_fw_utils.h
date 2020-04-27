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

#ifndef LCPC_FW_UTILS_H
#define LCPC_FW_UTILS_H

#include "ldpc_dev.h"
#include "hwinc/nv_ref_dev_falcon_pri_ldpc.h"

#define DRF_BASE(drf)           (0?drf)
#define DRF_SHIFT(drf)          (((u32)DRF_BASE(drf)) % 32U)
#define DRF_DEF(d,r,f,c)        (((u32)(NV ## d ## r ## f ## c))<<DRF_SHIFT(NV ## d ## r ## f))

struct ldpc_bin_header
{
	u32 bin_magic;			// (0x10ae)
	u32 bin_ver;			// (1)
	u32 bin_size;			// (entire image size including this header)
	u32 data_offset;		// (Data start)
	u32 code_offset;		// (Code start)
	u32 os_bin_size;		// (Size OS code + data excluding headers)
};

struct ldpc_riscv_ucode
{
	struct ldpc_bin_header *bin_header;
	u32 *data_buf;
	u32 *code_buf;
};

u32 engineRead(struct ldpc_devdata *pdata, u32 r);
void engineWrite(struct ldpc_devdata *pdata, u32 r, u32 v);
int ldpc_load_firmware(struct ldpc_devdata *pdata);

#endif
