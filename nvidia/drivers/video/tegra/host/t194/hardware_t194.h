/*
 * drivers/video/tegra/host/t194/hardware_t194.h
 *
 * Tegra T194 HOST1X Register Definitions
 *
 * Copyright (c) 2016-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __NVHOST_HARDWARE_T194_H
#define __NVHOST_HARDWARE_T194_H

#include "host1x/hw_host1x6_sync.h"
#include "host1x/hw_host1x6_uclass.h"
#include "host1x/hw_host1x6_channel.h"
#include "host1x/hw_host1x5_actmon.h"

/* sync registers */
#define NV_HOST1X_SYNCPT_NB_PTS 704
#define NV_HOST1X_NB_MLOCKS 32

#define NV_HOST1X_MLOCK_ID_NVCSI	7
#define NV_HOST1X_MLOCK_ID_ISP		8
#define NV_HOST1X_MLOCK_ID_VI		16
#define NV_HOST1X_MLOCK_ID_VIC		17
#define NV_HOST1X_MLOCK_ID_NVENC	18
#define NV_HOST1X_MLOCK_ID_NVDEC	19
#define NV_HOST1X_MLOCK_ID_NVJPG	20
#define NV_HOST1X_MLOCK_ID_TSEC		21
#define NV_HOST1X_MLOCK_ID_TSECB	22
#define NV_HOST1X_MLOCK_ID_NVENC1	29
#define NV_HOST1X_MLOCK_ID_NVDEC1	31

#define HOST1X_THOST_ACTMON_NVENC	0x00000
#define HOST1X_THOST_ACTMON_VIC 	0x10000
#define HOST1X_THOST_ACTMON_NVDEC	0x20000
#define HOST1X_THOST_ACTMON_NVJPG	0x30000
#define HOST1X_THOST_ACTMON_NVENC1	0x40000
#define HOST1X_THOST_ACTMON_NVDEC1	0x50000

/* Generic support */
static inline u32 nvhost_class_host_wait_syncpt(
	unsigned indx, unsigned threshold)
{
	return (indx << 24) | (threshold & 0xffffff);
}

static inline u32 nvhost_class_host_load_syncpt_base(
	unsigned indx, unsigned threshold)
{
	return host1x_uclass_wait_syncpt_indx_f(indx)
		| host1x_uclass_wait_syncpt_thresh_f(threshold);
}

static inline u32 nvhost_class_host_incr_syncpt(
	unsigned cond, unsigned indx)
{
	return host1x_uclass_incr_syncpt_cond_f(cond)
		| host1x_uclass_incr_syncpt_indx_f(indx);
}

static inline void __iomem *host1x_channel_aperture(void __iomem *p, int ndx)
{
	p += host1x_channel_ch_aperture_start_r() +
		ndx * host1x_channel_ch_aperture_size_r();
	return p;
}

enum {
	NV_HOST_MODULE_HOST1X = 0,
	NV_HOST_MODULE_MPE = 1,
	NV_HOST_MODULE_GR3D = 6
};

/* cdma opcodes */
static inline u32 nvhost_opcode_setclass(
	unsigned class_id, unsigned offset, unsigned mask)
{
	return (0 << 28) | (offset << 16) | (class_id << 6) | mask;
}

static inline u32 nvhost_opcode_incr(unsigned offset, unsigned count)
{
	return (1 << 28) | (offset << 16) | count;
}

static inline u32 nvhost_opcode_nonincr(unsigned offset, unsigned count)
{
	return (2 << 28) | (offset << 16) | count;
}

static inline u32 nvhost_opcode_mask(unsigned offset, unsigned mask)
{
	return (3 << 28) | (offset << 16) | mask;
}

static inline u32 nvhost_opcode_imm(unsigned offset, unsigned value)
{
	return (4 << 28) | (offset << 16) | value;
}

static inline u32 nvhost_opcode_imm_incr_syncpt(unsigned cond, unsigned indx)
{
	return nvhost_opcode_imm(host1x_uclass_incr_syncpt_r(),
		nvhost_class_host_incr_syncpt(cond, indx));
}

static inline u32 nvhost_opcode_restart(unsigned address)
{
	return (5 << 28) | (address >> 4);
}

static inline u32 nvhost_opcode_gather(unsigned count)
{
	return (6 << 28) | count;
}

static inline u32 nvhost_opcode_gather_nonincr(unsigned offset,	unsigned count)
{
	return (6 << 28) | (offset << 16) | BIT(15) | count;
}

static inline u32 nvhost_opcode_gather_incr(unsigned offset, unsigned count)
{
	return (6 << 28) | (offset << 16) | BIT(15) | BIT(14) | count;
}

static inline u32 nvhost_opcode_gather_insert(unsigned offset, unsigned incr,
		unsigned count)
{
	return (6 << 28) | (offset << 16) | BIT(15) | (incr << 14) | count;
}

static inline u32 nvhost_opcode_setstreamid(unsigned streamid)
{
	return (7 << 28) | streamid;
}

static inline u32 nvhost_opcode_setpayload(unsigned payload)
{
	return (9 << 28) | payload;
}

static inline u32 nvhost_opcode_acquire_mlock(unsigned id)
{
	return (14 << 28) | id;
}

static inline u32 nvhost_opcode_release_mlock(unsigned id)
{
	return (14 << 28) | (1 << 24) | id;
}

static inline u32 nvhost_opcode_incr_w(unsigned int offset)
{
	/* 22-bit offset supported */
	return (10 << 28) | offset;
}

static inline u32 nvhost_opcode_nonincr_w(unsigned offset)
{
	/* 22-bit offset supported */
	return (11 << 28) | offset;
}

#define NVHOST_OPCODE_NOOP nvhost_opcode_nonincr(0, 0)

static inline u32 nvhost_mask2(unsigned x, unsigned y)
{
	return 1 | (1 << (y - x));
}

#endif /* __NVHOST_HARDWARE_T194_H */
