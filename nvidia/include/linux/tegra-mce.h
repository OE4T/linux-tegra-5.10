/*
 * Copyright (c) 2014-2020, NVIDIA CORPORATION. All rights reserved.
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

#ifndef _LINUX_TEGRA_MCE_H
#define _LINUX_TEGRA_MCE_H

/* NOTE:
 * For correct version validation, below two defines need to be
 * updated whenever there is a new ARI implementation.
 */
#define CUR_ARI_VER_MAJOR	1
#define CUR_ARI_VER_MINOR	2

enum {
	TEGRA_MCE_XOVER_C1_C6, /* Only valid for Denver */
	TEGRA_MCE_XOVER_CC1_CC6,
	TEGRA_MCE_XOVER_CC1_CC7,

	TEGRA_MCE_XOVER_MAX = TEGRA_MCE_XOVER_CC1_CC7
};

enum {
	TEGRA_MCE_CSTATS_CLEAR,

	TEGRA_MCE_CSTATS_ENTRIES_SC7,
	TEGRA_MCE_CSTATS_ENTRIES_SC4,
	TEGRA_MCE_CSTATS_ENTRIES_SC3,
	TEGRA_MCE_CSTATS_ENTRIES_SC2,
	TEGRA_MCE_CSTATS_ENTRIES_CCP3,
	TEGRA_MCE_CSTATS_ENTRIES_A57_CC6,
	TEGRA_MCE_CSTATS_ENTRIES_A57_CC7,
	TEGRA_MCE_CSTATS_ENTRIES_D15_CC6,
	TEGRA_MCE_CSTATS_ENTRIES_D15_CC7,
	TEGRA_MCE_CSTATS_ENTRIES_D15_CORE0_C6,
	TEGRA_MCE_CSTATS_ENTRIES_D15_CORE1_C6,
	/* RESV: 12-13 */
	TEGRA_MCE_CSTATS_ENTRIES_D15_CORE0_C7 = 14,
	TEGRA_MCE_CSTATS_ENTRIES_D15_CORE1_C7,
	/* RESV: 16-17 */
	TEGRA_MCE_CSTATS_ENTRIES_A57_CORE0_C7 = 18,
	TEGRA_MCE_CSTATS_ENTRIES_A57_CORE1_C7,
	TEGRA_MCE_CSTATS_ENTRIES_A57_CORE2_C7,
	TEGRA_MCE_CSTATS_ENTRIES_A57_CORE3_C7,
	TEGRA_MCE_CSTATS_LAST_ENTRY_D15_CORE0,
	TEGRA_MCE_CSTATS_LAST_ENTRY_D15_CORE1,
	/* RESV: 24-25 */
	TEGRA_MCE_CSTATS_LAST_ENTRY_A57_CORE0,
	TEGRA_MCE_CSTATS_LAST_ENTRY_A57_CORE1,
	TEGRA_MCE_CSTATS_LAST_ENTRY_A57_CORE2,
	TEGRA_MCE_CSTATS_LAST_ENTRY_A57_CORE3,

	TEGRA_MCE_CSTATS_MAX = TEGRA_MCE_CSTATS_LAST_ENTRY_A57_CORE3,
};

enum {
	TEGRA_MCE_ENUM_D15_CORE0,
	TEGRA_MCE_D15_CORE1,
	/* RESV: 2-3 */
	TEGRA_MCE_ENUM_A57_0 = 4,
	TEGRA_MCE_ENUM_A57_1,
	TEGRA_MCE_ENUM_A57_2,
	TEGRA_MCE_ENUM_A57_3,

	TEGRA_MCE_ENUM_MAX = TEGRA_MCE_ENUM_A57_3,
};

enum {
	TEGRA_MCE_FEATURE_CCP3,
};

/* MCA support */
typedef union {
	struct {
		u8 cmd;
		u8 subidx;
		u8 idx;
		u8 inst;
	};
	struct {
		u32 low;
		u32 high;
	};
	u64 data;
} mca_cmd_t;

/* NOTE: These functions will return -ENOTSUPP if no implementation */
int tegra_mce_enter_cstate(u32 state, u32 wake_time);
int tegra_mce_update_cstate_info(u32 cluster, u32 ccplex, u32 system,
				 u8 force, u32 wake_mask, bool valid);
int tegra_mce_update_crossover_time(u32 type, u32 time);
int tegra_mce_read_cstate_stats(u32 state, u64 *stats);
int tegra_mce_write_cstate_stats(u32 state, u32 stats);
int tegra_mce_is_sc7_allowed(u32 state, u32 wake, u32 *allowed);
int tegra_mce_online_core(int cpu);
int tegra_mce_cc3_ctrl(u32 ndiv, u32 vindex, u8 enable);
int tegra_mce_echo_data(u64 data, u64 *matched);
int tegra_mce_read_versions(u32 *major, u32 *minor);
int tegra_mce_enum_features(u64 *features);
int tegra_mce_read_uncore_mca(mca_cmd_t cmd, u64 *data, u32 *error);
int tegra_mce_write_uncore_mca(mca_cmd_t cmd, u64 data, u32 *error);
int tegra_mce_read_uncore_perfmon(u32 req, u32 *data);
int tegra_mce_write_uncore_perfmon(u32 req, u32 data);
int tegra_mce_enable_latic(void);
int tegra_mce_write_dda_ctrl(u32 index, u64 value);
int tegra_mce_read_dda_ctrl(u32 index, u64 *value);

/* L3 cache ways read/write functions */
int tegra_mce_read_l3_cache_ways(u64 *value);
int tegra_mce_write_l3_cache_ways(u64 data, u64 *value);

int tegra_mce_read_rt_safe_mask(u64 *);
int tegra_mce_write_rt_safe_mask(u64);
int tegra_mce_read_rt_window_us(u64 *);
int tegra_mce_write_rt_window_us(u64);
int tegra_mce_read_rt_fwd_progress_us(u64 *);
int tegra_mce_write_rt_fwd_progress_us(u64);

struct tegra_mce_ops {
	int (*enter_cstate)(u32, u32);
	int (*update_cstate_info)(u32, u32, u32, u8, u32, bool);
	int (*update_crossover_time)(u32, u32);
	int (*read_cstate_stats)(u32, u64 *);
	int (*write_cstate_stats)(u32, u32);
	int (*is_sc7_allowed)(u32, u32, u32 *);
	int (*online_core)(int);
	int (*cc3_ctrl)(u32, u32, u8);
	int (*echo_data)(u64, u64*);
	int (*read_versions)(u32 *, u32 *);
	int (*enum_features)(u64 *);
	int (*read_uncore_mca)(mca_cmd_t, u64 *, u32 *);
	int (*write_uncore_mca)(mca_cmd_t, u64, u32 *);
	int (*read_uncore_perfmon)(u32, u32 *);
	int (*write_uncore_perfmon)(u32, u32);
	int (*enable_latic)(void);
	int (*write_dda_ctrl)(u32 index, u64 value);
	int (*read_dda_ctrl)(u32 index, u64 *value);
	int (*read_l3_cache_ways)(u64 *value);
	int (*write_l3_cache_ways)(u64 data, u64 *value);
	int (*read_rt_safe_mask)(u64 *);
	int (*write_rt_safe_mask)(u64);
	int (*read_rt_window_us)(u64 *);
	int (*write_rt_window_us)(u64);
	int (*read_rt_fwd_progress_us)(u64 *);
	int (*write_rt_fwd_progress_us)(u64);
};

void tegra_mce_set_ops(struct tegra_mce_ops *);
#endif /* _LINUX_TEGRA_MCE_H */
