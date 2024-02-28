/*
 * Copyright (c) 2012-2021, NVIDIA CORPORATION.  All rights reserved.
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
#ifndef _hw_flcn_h_
#define _hw_flcn_h_

static inline u32 flcn_thi_int_stat_r(void)
{
	return 0x78;
}
static inline u32 flcn_thi_int_stat_clr_f(void)
{
	return 0x1;
}
static inline u32 flcn_slcg_override_high_a_r(void)
{
	return 0x00000088;
}
static inline u32 flcn_slcg_override_low_a_r(void)
{
	return 0x0000008c;
}
static inline u32 flcn_clk_override_r(void)
{
	return 0x00000e00;
}
static inline u32 flcn_irqmclr_r(void)
{
	return 0x1014;
}
static inline u32 flcn_irqmclr_swgen0_set_f(void)
{
	return 0x40;
}
static inline u32 flcn_irqmclr_swgen1_set_f(void)
{
	return 0x80;
}
static inline u32 flcn_irqsclr_r(void)
{
	return 0x1004;
}
static inline u32 flcn_irqsclr_swgen0_set_f(void)
{
	return 0x40;
}
static inline u32 flcn_irqsclr_swgen1_set_f(void)
{
	return 0x80;
}
static inline u32 flcn_irqstat_r(void)
{
	return 0x00001008;
}
static inline u32 flcn_irqmset_r(void)
{
	return 0x00001010;
}
static inline u32 flcn_irqmset_wdtmr_set_f(void)
{
	return 0x2;
}
static inline u32 flcn_irqmset_halt_set_f(void)
{
	return 0x10;
}
static inline u32 flcn_irqmset_exterr_set_f(void)
{
	return 0x20;
}
static inline u32 flcn_irqmset_swgen0_set_f(void)
{
	return 0x40;
}
static inline u32 flcn_irqmset_swgen1_set_f(void)
{
	return 0x80;
}
static inline u32 flcn_irqmset_ext_f(u32 v)
{
	return (v & 0xff) << 8;
}
static inline u32 flcn_irqdest_r(void)
{
	return 0x0000101c;
}
static inline u32 flcn_irqdest_host_halt_host_f(void)
{
	return 0x10;
}
static inline u32 flcn_irqdest_host_exterr_host_f(void)
{
	return 0x20;
}
static inline u32 flcn_irqdest_host_swgen0_host_f(void)
{
	return 0x40;
}
static inline u32 flcn_irqdest_host_swgen1_host_f(void)
{
	return 0x80;
}
static inline u32 flcn_irqdest_host_ext_f(u32 v)
{
	return (v & 0xff) << 8;
}
static inline u32 flcn_itfen_r(void)
{
	return 0x00001048;
}
static inline u32 flcn_itfen_ctxen_enable_f(void)
{
	return 0x1;
}
static inline u32 flcn_itfen_mthden_enable_f(void)
{
	return 0x2;
}
static inline u32 flcn_mailbox0_r(void)
{
	return 0x00001040;
}
static inline u32 flcn_mailbox1_r(void)
{
	return 0x00001044;
}
static inline u32 flcn_idlestate_r(void)
{
	return 0x0000104c;
}
static inline u32 flcn_debuginfo_r(void)
{
	return 0x00001094;
}
static inline u32 flcn_cgctl_r(void)
{
	return 0x000010a0;
}
static inline u32 flcn_exci_r(void)
{
	return 0x000010d0;
}
static inline u32 flcn_cpuctl_r(void)
{
	return 0x00001100;
}
static inline u32 flcn_cpuctl_startcpu_true_f(void)
{
	return 0x2;
}
static inline u32 flcn_bootvec_r(void)
{
	return 0x00001104;
}
static inline u32 flcn_bootvec_vec_f(u32 v)
{
	return (v & 0xffffffff) << 0;
}
static inline u32 flcn_dmactl_r(void)
{
	return 0x0000110c;
}
static inline u32 flcn_dmactl_dmem_scrubbing_m(void)
{
	return 0x1 << 1;
}
static inline u32 flcn_dmactl_imem_scrubbing_m(void)
{
	return 0x1 << 2;
}
static inline u32 flcn_dmatrfbase_r(void)
{
	return 0x00001110;
}
static inline u32 flcn_dmatrfmoffs_r(void)
{
	return 0x00001114;
}
static inline u32 flcn_dmatrfmoffs_offs_f(u32 v)
{
	return (v & 0xffff) << 0;
}
static inline u32 flcn_dmatrfcmd_r(void)
{
	return 0x00001118;
}
static inline u32 flcn_dmatrfcmd_idle_v(u32 r)
{
	return (r >> 1) & 0x1;
}
static inline u32 flcn_dmatrfcmd_idle_true_v(void)
{
	return 0x00000001;
}
static inline u32 flcn_dmatrfcmd_imem_f(u32 v)
{
	return (v & 0x1) << 4;
}
static inline u32 flcn_dmatrfcmd_imem_true_f(void)
{
	return 0x10;
}
static inline u32 flcn_dmatrfcmd_size_f(u32 v)
{
	return (v & 0x7) << 8;
}
static inline u32 flcn_dmatrfcmd_size_256b_f(void)
{
	return 0x600;
}
static inline u32 flcn_dmatrfcmd_dmactx_f(u32 v)
{
	return (v & 0x7) << 12;
}
static inline u32 flcn_dmatrffboffs_r(void)
{
	return 0x0000111c;
}
static inline u32 flcn_dmatrffboffs_offs_f(u32 v)
{
	return (v & 0xffffffff) << 0;
}
static inline u32 flcn_cg_r(void)
{
	return 0x000016d0;
}
static inline u32 flcn_cg_idle_cg_dly_cnt_f(u32 v)
{
	return (v & 0x3f) << 0;
}
static inline u32 flcn_cg_idle_cg_en_f(u32 v)
{
	return (v & 0x1) << 6;
}
static inline u32 flcn_cg_wakeup_dly_cnt_f(u32 v)
{
	return (v & 0xf) << 16;
}
static inline u32 sec_intf_crc_ctrl_r(void)
{
	return 0x0000e000;
}
static inline u32 flcn_hwcfg2_r(void)
{
	return 0x000010f4;
}
static inline u32 flcn_hwcfg2_dbgmode_m(void)
{
	return 0x1 << 3;
}
static inline u32 flcn_hwcfg2_mem_scrubbing_m(void)
{
	return 0x1 << 12;
}
static inline u32 flcn_hwcfg2_mem_scrubbing_v(u32 r)
{
	return (r >> 12) & 0x1;
}
static inline u32 flcn_hwcfg2_mem_scrubbing_done_v(void)
{
	return 0x0;
}
static inline u32 cbb_vic_sec_blf_write_ctl_r(void)
{
	return 0x00019508;
}
static inline u32 cbb_vic_sec_blf_ctl_r(void)
{
	return 0x00019510;
}
static inline u32 cbb_nvenc_sec_blf_write_ctl_r(void)
{
	return 0x000167a8;
}
static inline u32 cbb_nvenc_sec_blf_ctl_r(void)
{
	return 0x000167b0;
}
static inline u32 cbb_ofa_sec_blf_write_ctl_r(void)
{
	return 0x00016968;
}
static inline u32 cbb_ofa_sec_blf_ctl_r(void)
{
	return 0x00016970;
}
static inline u32 cbb_sec_blf_write_ctl_mstrid_1_f(void)
{
	return 0x00000002;
}
static inline u32 cbb_sec_blf_ctl_blf_lck_f(void)
{
	return 0x80000000;
}
#endif
