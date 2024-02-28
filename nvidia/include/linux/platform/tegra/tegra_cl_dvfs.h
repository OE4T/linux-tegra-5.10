/*
 * include/linux/platform/tegra/tegra_cl_dvfs.h
 *
 * Copyright (c) 2012-2015 NVIDIA Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _TEGRA_CL_DVFS_H_
#define _TEGRA_CL_DVFS_H_

struct tegra_cl_dvfs;
struct pinctrl_dev;

#define MAX_CL_DVFS_VOLTAGES		33

enum tegra_cl_dvfs_force_mode {
	TEGRA_CL_DVFS_FORCE_NONE = 0,
	TEGRA_CL_DVFS_FORCE_FIXED = 1,
	TEGRA_CL_DVFS_FORCE_AUTO = 2,
};

enum tegra_cl_dvfs_pmu_if {
	TEGRA_CL_DVFS_PMU_I2C,
	TEGRA_CL_DVFS_PMU_PWM,
};

enum tegra_cl_dvfs_pwm_bus {
	TEGRA_CL_DVFS_PWM_1WIRE_BUFFER,
	TEGRA_CL_DVFS_PWM_1WIRE_DIRECT,
	TEGRA_CL_DVFS_PWM_2WIRE,
};

/* CL DVFS plaform flags*/
/* set if output to PMU can be disabled only between I2C transactions */
#define TEGRA_CL_DVFS_FLAGS_I2C_WAIT_QUIET	(0x1UL << 0)
/* dynamic output registers update is supported */
#define TEGRA_CL_DVFS_DYN_OUTPUT_CFG		(0x1UL << 1)
/* monitor data new synchronization can not be used */
#define TEGRA_CL_DVFS_DATA_NEW_NO_USE		(0x1UL << 2)
/* set if control settings are overridden when CPU is idle */
#define TEGRA_CL_DVFS_HAS_IDLE_OVERRIDE		(0x1UL << 3)
/* set if calibration should be deferred for voltage matching force value */
#define TEGRA_CL_DVFS_DEFER_FORCE_CALIBRATE	(0x1UL << 4)
/* set if request scale is applied in open loop (not set: enforce 1:1 scale) */
#define TEGRA_CL_DVFS_SCALE_IN_OPEN_LOOP	(0x1UL << 5)
/* set if min output is forced during calibration */
#define TEGRA_CL_DVFS_CALIBRATE_FORCE_VMIN	(0x1UL << 6)



struct tegra_cl_dvfs_cfg_param {
	unsigned long	sample_rate;

	enum tegra_cl_dvfs_force_mode force_mode;
	u8		cf;
	u8		ci;
	s8		cg;
	bool		cg_scale;

	u8		droop_cut_value;
	u8		droop_restore_ramp;
	u8		scale_out_ramp;
};

struct voltage_reg_map {
	u8		reg_value;
	int		reg_uV;
};

struct tegra_cl_dvfs_platform_data {
	const char *dfll_clk_name;
	u32 flags;

	enum tegra_cl_dvfs_pmu_if pmu_if;
	union {
		struct {
			unsigned long		fs_rate;
			unsigned long		hs_rate; /* if 0 - no hs mode */
			u8			hs_master_code;
			u8			reg;
			u16			slave_addr;
			bool			addr_10;
			u32			sel_mul;
			u32			sel_offs;
		} pmu_i2c;
		struct {
			unsigned long		pwm_rate;
			bool			delta_mode;
			int			min_uV;
			int			step_uV;
			int			init_uV;

			enum tegra_cl_dvfs_pwm_bus pwm_bus;
			int			pwm_pingroup;
			int			pwm_clk_pingroup;
			int			out_gpio;
			bool			out_enable_high;
			struct platform_device	*dfll_bypass_dev;
			struct pinctrl_dev	*pinctrl_dev;
		} pmu_pwm;
	} u;

	struct voltage_reg_map	*vdd_map;
	int			vdd_map_size;
	int			pmu_undershoot_gb;
	int			resume_ramp_delay;
	int			tune_ramp_delay;

	struct tegra_cl_dvfs_cfg_param		*cfg_param;
};

#ifdef CONFIG_ARCH_TEGRA_HAS_CL_DVFS
int tegra_init_cl_dvfs(void);
int tegra_cl_dvfs_debug_init(struct clk *dfll_clk);
void tegra_cl_dvfs_resume(struct tegra_cl_dvfs *cld);

/* functions below are called only within DFLL clock interface DFLL lock held */
void tegra_cl_dvfs_disable(struct tegra_cl_dvfs *cld);
int tegra_cl_dvfs_enable(struct tegra_cl_dvfs *cld);
int tegra_cl_dvfs_lock(struct tegra_cl_dvfs *cld);
int tegra_cl_dvfs_unlock(struct tegra_cl_dvfs *cld);
int tegra_cl_dvfs_request_rate(struct tegra_cl_dvfs *cld, unsigned long rate);
unsigned long tegra_cl_dvfs_request_get(struct tegra_cl_dvfs *cld);

#else
static inline int tegra_init_cl_dvfs(void)
{ return -ENOSYS; }
static inline int tegra_cl_dvfs_debug_init(struct clk *dfll_clk)
{ return -ENOSYS; }
static inline void tegra_cl_dvfs_resume(struct tegra_cl_dvfs *cld)
{}

static inline void tegra_cl_dvfs_disable(struct tegra_cl_dvfs *cld)
{}
static inline int tegra_cl_dvfs_enable(struct tegra_cl_dvfs *cld)
{ return -ENOSYS; }
static inline int tegra_cl_dvfs_lock(struct tegra_cl_dvfs *cld)
{ return -ENOSYS; }
static inline int tegra_cl_dvfs_unlock(struct tegra_cl_dvfs *cld)
{ return -ENOSYS; }
static inline int tegra_cl_dvfs_request_rate(
	struct tegra_cl_dvfs *cld, unsigned long rate)
{ return -ENOSYS; }
static inline unsigned long tegra_cl_dvfs_request_get(struct tegra_cl_dvfs *cld)
{ return 0; }
#endif

#endif
