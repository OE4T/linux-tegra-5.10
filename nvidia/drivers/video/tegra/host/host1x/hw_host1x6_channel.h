/*
 * Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
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
/*
 * Function naming determines intended use:
 *
 *     <x>_r(void) : Returns the offset for register <x>.
 *
 *     <x>_o(void) : Returns the offset for element <x>.
 *
 *     <x>_w(void) : Returns the word offset for word (4 byte) element <x>.
 *
 *     <x>_<y>_s(void) : Returns size of field <y> of register <x> in bits.
 *
 *     <x>_<y>_f(u32 v) : Returns a value based on 'v' which has been shifted
 *         and masked to place it at field <y> of register <x>.  This value
 *         can be |'d with others to produce a full register value for
 *         register <x>.
 *
 *     <x>_<y>_m(void) : Returns a mask for field <y> of register <x>.  This
 *         value can be ~'d and then &'d to clear the value of field <y> for
 *         register <x>.
 *
 *     <x>_<y>_<z>_f(void) : Returns the constant value <z> after being shifted
 *         to place it at field <y> of register <x>.  This value can be |'d
 *         with others to produce a full register value for <x>.
 *
 *     <x>_<y>_v(u32 r) : Returns the value of field <y> from a full register
 *         <x> value 'r' after being shifted to place its LSB at bit 0.
 *         This value is suitable for direct comparison with other unshifted
 *         values appropriate for use in field <y> of register <x>.
 *
 *     <x>_<y>_<z>_v(void) : Returns the constant value for <z> defined for
 *         field <y> of register <x>.  This value is suitable for direct
 *         comparison with unshifted values appropriate for use in field <y>
 *         of register <x>.
 */
#ifndef _hw_host1x6_channel_h_
#define _hw_host1x6_channel_h_

static inline u32 host1x_channel_ch_aperture_start_r(void)
{
	return 0x0;
}
static inline u32 host1x_channel_ch_aperture_size_r(void)
{
	return 0x100;
}
static inline u32 host1x_channel_fifostat_r(void)
{
	return 0x24;
}
static inline u32 host1x_channel_fifostat_cfempty_v(u32 r)
{
	return (r >> 13) & 0x1;
}
static inline u32 host1x_channel_dmastart_r(void)
{
	return 0x0;
}
static inline u32 host1x_channel_dmastart_hi_r(void)
{
	return 0x4;
}
static inline u32 host1x_channel_dmaput_r(void)
{
	return 0x8;
}
static inline u32 host1x_channel_dmaput_hi_r(void)
{
	return 0xc;
}
static inline u32 host1x_channel_dmaget_r(void)
{
	return 0x10;
}
static inline u32 host1x_channel_dmaget_hi_r(void)
{
	return 0x14;
}
static inline u32 host1x_channel_dmaend_r(void)
{
	return 0x18;
}
static inline u32 host1x_channel_dmaend_hi_r(void)
{
	return 0x1c;
}
static inline u32 host1x_channel_dmactrl_r(void)
{
	return 0x20;
}
static inline u32 host1x_channel_dmactrl_dmastop_f(u32 v)
{
	return (v & 0x1) << 0;
}
static inline u32 host1x_channel_dmactrl_dmastop_v(u32 r)
{
	return (r >> 0) & 0x1;
}
static inline u32 host1x_channel_dmactrl_dmagetrst_f(u32 v)
{
	return (v & 0x1) << 1;
}
static inline u32 host1x_channel_dmactrl_dmainitget_f(u32 v)
{
	return (v & 0x1) << 2;
}
static inline u32 host1x_channel_rdata_r(void)
{
	return 0x28;
}
static inline u32 host1x_channel_cmdp_offset_r(void)
{
	return 0x30;
}
static inline u32 host1x_channel_cmdp_class_r(void)
{
	return 0x34;
}
static inline u32 host1x_channel_cmdp_channelstat_r(void)
{
	return 0x38;
}
static inline u32 host1x_channel_mlock_en_r(void)
{
	return 0x2000;
}
static inline u32 host1x_channel_ch_hipri_r(void)
{
	return 0x2010;
}
static inline u32 host1x_channel_filter_gbuffer_r(void)
{
	return 0x2020;
}
static inline u32 host1x_channel_ch_vm_0_r(void)
{
	return 0x644;
}
static inline u32 host1x_channel_intrmask_r(void)
{
	return 0x80;
}
static inline u32 host1x_channel_smmu_streamid_r(void)
{
	return 0x84;
}
static inline u32 host1x_channel_mlock_timeout_r(void)
{
	return 0x88;
}
#endif
