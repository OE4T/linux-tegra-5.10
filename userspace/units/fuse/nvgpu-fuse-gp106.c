/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <unit/unit.h>
#include <unit/io.h>
#include <nvgpu/posix/io.h>
#include <nvgpu/enabled.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/fuse.h>
#include <nvgpu/hal_init.h>
#include "common/fuse/fuse_gm20b.h"

#include "nvgpu-fuse-priv.h"
#include "nvgpu-fuse-gp106.h"


/* register definitions for this block */
#define GP106_FUSE_REG_BASE			0x00021000U
#define GP106_FUSE_STATUS_OPT_PRIV_SEC_EN	(GP106_FUSE_REG_BASE+0x434U)
#define GP106_FUSE_OPT_ADC_CAL_FUSE_REV		(GP106_FUSE_REG_BASE+0x64CU)
#define GP106_FUSE_OPT_ADC_CAL_GPC0		(GP106_FUSE_REG_BASE+0x650U)
#define GP106_FUSE_OPT_ADC_CAL_GPC1_DELTA	(GP106_FUSE_REG_BASE+0x654U)
#define GP106_FUSE_OPT_ADC_CAL_GPC2_DELTA	(GP106_FUSE_REG_BASE+0x658U)
#define GP106_FUSE_OPT_ADC_CAL_GPC3_DELTA	(GP106_FUSE_REG_BASE+0x65CU)
#define GP106_FUSE_OPT_ADC_CAL_GPC4_DELTA	(GP106_FUSE_REG_BASE+0x660U)
#define GP106_FUSE_OPT_ADC_CAL_GPC5_DELTA	(GP106_FUSE_REG_BASE+0x664U)
#define GP106_FUSE_OPT_ADC_CAL_SHARED_DELTA	(GP106_FUSE_REG_BASE+0x668U)
#define GP106_FUSE_OPT_ADC_CAL_SRAM_DELTA	(GP106_FUSE_REG_BASE+0x66CU)

/* for common init args */
struct fuse_test_args gp106_init_args = {
	.gpu_arch = 0x13,
	.gpu_impl = 0x6,
	.fuse_base_addr = GP106_FUSE_REG_BASE,
	.sec_fuse_addr = GP106_FUSE_STATUS_OPT_PRIV_SEC_EN,
};

/*
 * Verify fuse API check_priv_security() when security fuse is enabled.
 * Tests with secure debug enabled and disabled.
 */
int test_fuse_gp106_check_sec(struct unit_module *m,
			      struct gk20a *g, void *__args)
{
	int ret = UNIT_SUCCESS;
	int result;

	nvgpu_posix_io_writel_reg_space(g, GP106_FUSE_STATUS_OPT_PRIV_SEC_EN,
					0x1);

	result = g->ops.fuse.check_priv_security(g);
	if (result != 0) {
		unit_err(m, "%s: fuse_check_priv_security returned "
				"error %d\n", __func__, result);
		ret = UNIT_FAIL;
	}

	if (!nvgpu_is_enabled(g, NVGPU_SEC_PRIVSECURITY)) {
		unit_err(m, "%s: NVGPU_SEC_PRIVSECURITY disabled\n",
				__func__);
		ret = UNIT_FAIL;
	}

	if (!nvgpu_is_enabled(g, NVGPU_SEC_SECUREGPCCS)) {
		unit_err(m, "%s: NVGPU_SEC_SECUREGPCCS disabled\n",
				__func__);
		ret = UNIT_FAIL;
	}

	return ret;
}

/*
 * Verify fuse API check_priv_security() when security fuse is enabled.
 * GP106 always has security enabled.
 */
int test_fuse_gp106_check_non_sec(struct unit_module *m,
				  struct gk20a *g, void *__args)
{
	int ret = UNIT_SUCCESS;
	int result;

	nvgpu_posix_io_writel_reg_space(g, GP106_FUSE_STATUS_OPT_PRIV_SEC_EN,
					0x0);

	result = g->ops.fuse.check_priv_security(g);
	if (result != 0) {
		unit_err(m, "%s: fuse_check_priv_security returned "
				"error %d\n", __func__, result);
		ret = UNIT_FAIL;
	}

	if (!nvgpu_is_enabled(g, NVGPU_SEC_PRIVSECURITY)) {
		unit_err(m, "%s: NVGPU_SEC_PRIVSECURITY enabled\n", __func__);
		ret = UNIT_FAIL;
	}

	if (!nvgpu_is_enabled(g, NVGPU_SEC_SECUREGPCCS)) {
		unit_err(m, "%s: NVGPU_SEC_SECUREGPCCS enabled\n", __func__);
		ret = UNIT_FAIL;
	}

	return ret;
}


/* Verify fuse API to read cal fuse revision */
int test_fuse_gp106_vin_cal_rev(struct unit_module *m,
				struct gk20a *g, void *__args)
{
	const u32 rev = 0x3;
	u32 val;
	int ret = UNIT_SUCCESS;

	nvgpu_posix_io_writel_reg_space(g, GP106_FUSE_OPT_ADC_CAL_FUSE_REV,
					rev);

	val = g->ops.fuse.read_vin_cal_fuse_rev(g);

	if (val != rev) {
		unit_err(m, "%s: cal fuse rev invalid 0x%x != 0x%x\n",
			 __func__, val, rev);
		ret = UNIT_FAIL;
	}

	return ret;
}

/* common function used for calculating calibration value from fuse */
static u32 calculate_cal_unsigned(u32 fuse_val, u8 int_start, u8 int_bits,
			   u8 frac_start, u8 frac_bits)
{
	u32 int_mask = (1 << int_bits) - 1;
	u32 frac_mask = (1 << frac_bits) - 1;
	u32 tmp;

	tmp = ((fuse_val >> int_start) & int_mask) << frac_bits;
	tmp += (fuse_val >> frac_start) & frac_mask;
	tmp = (tmp * 1000) >> frac_bits;

	return tmp;
}

/* calculate slope value from GPC0 fuse value */
static u32 gpc0_expected_slope(u32 gpc0_fuse, u32 this_fuse)
{
	return calculate_cal_unsigned(this_fuse, 10, 4, 0, 10);
}

/* calculate intercept value from GPC0 fuse value */
static u32 gpc0_expected_intercept(u32 gpc0_fuse, u32 this_fuse)
{
	return calculate_cal_unsigned(this_fuse, 16, 12, 14, 2);
}

/* calculate slope value from GPC0 and delta values (GPC1-5,etc) fuse value */
static u32 gpc1_expected_slope(u32 gpc0_fuse, u32 gpc1_fuse)
{
	u32 gpc0_slope = gpc0_expected_slope(gpc0_fuse, gpc0_fuse);
	u32 gpc1_delta = ((gpc1_fuse >> 10) & 0x1) * 1000;
	u32 gpc1_delta_positive = ((gpc1_fuse >> 11) & 0x1) == 0;

	if (gpc1_delta_positive) {
		return gpc0_slope + gpc1_delta;
	} else {
		return gpc0_slope - gpc1_delta;
	}
}

/*
 * calculate intercept value from GPC0 and delta values (GPC1-5,etc) fuse
 * value
 */
static u32 gpc1_expected_intercept(u32 gpc0_fuse, u32 gpc1_fuse)
{
	u32 gpc0_intercept = gpc0_expected_intercept(gpc0_fuse, gpc0_fuse);
	u32 gpc1_delta = calculate_cal_unsigned(gpc1_fuse, 14, 8, 12, 2);
	u32 gpc1_delta_positive = ((gpc1_fuse >> 22) & 0x1) == 0;

	if (gpc1_delta_positive) {
		return gpc0_intercept + gpc1_delta;
	} else {
		return gpc0_intercept - gpc1_delta;
	}
}

/* calculate slope value from GPC0 and delta SRAM fuse */
static u32 sram_expected_slope(u32 gpc0_fuse, u32 sram_fuse)
{
	/*
	 * same calculation as GPC1, et al, but for consistency, make
	 * a new function
	 */
	return gpc1_expected_slope(gpc0_fuse, sram_fuse);
}

/* calculate intercept value from GPC0 and delta SRAM fuse */
static u32 sram_expected_intercept(u32 gpc0_fuse, u32 sram_fuse)
{
	u32 gpc0_intercept = gpc0_expected_intercept(gpc0_fuse, gpc0_fuse);
	u32 sram_delta = calculate_cal_unsigned(sram_fuse, 13, 9, 12, 1);
	u32 sram_delta_positive = ((sram_fuse >> 22) & 0x1) == 0;

	if (sram_delta_positive) {
		return gpc0_intercept + sram_delta;
	} else {
		return gpc0_intercept - sram_delta;
	}
}

static s8 fuse_expected_gain(u32 this_fuse)
{
	return (s8)((this_fuse >> 16U) & 0x1fU);
}

static s8 fuse_expected_offset(u32 this_fuse)
{
	return (s8)(this_fuse & 0x7fU);
}

/*
 * Verify fuse API to read cal fuse revision
 *   Loops through table of fuse values and expected results
 *   Validates invalid data checks
 */
int test_fuse_gp106_vin_cal_slope_intercept(struct unit_module *m,
					    struct gk20a *g, void *__args)
{
	int result;
	u32 slope, intercept;
	s8 gain, offset;
	int ret = UNIT_SUCCESS;

	/* table for storing fuse values and expected results */
	struct vin_test_struct {
		u32 vin_id;
		u32 fuse_addr;
		u32 gpc0_fuse_val;
		u32 fuse_val;
		u32 (*expected_slope)(u32 gpc0_fuse, u32 this_fuse);
		u32 (*expected_intercept)(u32 gpc0_fuse, u32 this_fuse);
	};
	struct vin_test_struct vin_test_table[] = {
		{
		 CTRL_CLK_VIN_ID_GPC0, GP106_FUSE_OPT_ADC_CAL_GPC0,
		 0x00214421, 0x00214421,
		 gpc0_expected_slope, gpc0_expected_intercept,
		},
		{
		 CTRL_CLK_VIN_ID_GPC1, GP106_FUSE_OPT_ADC_CAL_GPC1_DELTA,
		 0x00214421, 0x00214421,
		 gpc1_expected_slope, gpc1_expected_intercept,
		},
		{
		 CTRL_CLK_VIN_ID_GPC2, GP106_FUSE_OPT_ADC_CAL_GPC2_DELTA,
		 0x00000000, 0x00614c21,
		 gpc1_expected_slope, gpc1_expected_intercept,
		},
		{
		 CTRL_CLK_VIN_ID_GPC3, GP106_FUSE_OPT_ADC_CAL_GPC3_DELTA,
		 0x00214421, 0xaaaaaaaa,
		 gpc1_expected_slope, gpc1_expected_intercept,
		},
		{
		 CTRL_CLK_VIN_ID_GPC4, GP106_FUSE_OPT_ADC_CAL_GPC4_DELTA,
		 0x00214421, 0x55555555,
		 gpc1_expected_slope, gpc1_expected_intercept,
		},
		{
		 CTRL_CLK_VIN_ID_GPC5, GP106_FUSE_OPT_ADC_CAL_GPC5_DELTA,
		 0x00214421, 0xefffffff,
		 gpc1_expected_slope, gpc1_expected_intercept,
		},
		{
		 CTRL_CLK_VIN_ID_SYS, GP106_FUSE_OPT_ADC_CAL_SHARED_DELTA,
		 0x00214421, 0xfffffffe,
		 gpc1_expected_slope, gpc1_expected_intercept,
		},
		{
		 CTRL_CLK_VIN_ID_XBAR, GP106_FUSE_OPT_ADC_CAL_SHARED_DELTA,
		 0x00214421, 0x11111111,
		 gpc1_expected_slope, gpc1_expected_intercept,
		},
		{
		 CTRL_CLK_VIN_ID_LTC, GP106_FUSE_OPT_ADC_CAL_SHARED_DELTA,
		 0x00214421, 0x00000001,
		 gpc1_expected_slope, gpc1_expected_intercept,
		},
		{
		 CTRL_CLK_VIN_ID_SRAM, GP106_FUSE_OPT_ADC_CAL_SRAM_DELTA,
		 0x00214421, 0xaaaaaaaa,
		 sram_expected_slope, sram_expected_intercept,
		},
		{
		 CTRL_CLK_VIN_ID_SRAM, GP106_FUSE_OPT_ADC_CAL_SRAM_DELTA,
		 0x00214421, 0x55555555,
		 sram_expected_slope, sram_expected_intercept,
		},
	};
	int vin_table_len = sizeof(vin_test_table)/sizeof(vin_test_table[0]);
	int i;

	for (i = 0; i < vin_table_len; i++) {
		u32 gpc0_fuse_val = vin_test_table[i].gpc0_fuse_val;
		u32 this_fuse_val = vin_test_table[i].fuse_val;
		u32 expected_slope, expected_intercept;
		s8 expected_gain, expected_offset;

		nvgpu_posix_io_writel_reg_space(g, GP106_FUSE_OPT_ADC_CAL_GPC0,
						gpc0_fuse_val);
		nvgpu_posix_io_writel_reg_space(g,
					vin_test_table[i].fuse_addr,
					this_fuse_val);

		result = g->ops.fuse.read_vin_cal_slope_intercept_fuse(g,
						vin_test_table[i].vin_id,
						&slope,	&intercept);
		if (result != 0) {
			unit_err(m, "%s: read_vin_cal_slope_intercept_fuse "
				" returned error %d, i = %d\n",
				__func__, result, i);
			ret = UNIT_FAIL;
		}

		expected_slope = vin_test_table[i].expected_slope(
							gpc0_fuse_val,
							this_fuse_val);
		expected_intercept = vin_test_table[i].expected_intercept(
							gpc0_fuse_val,
							this_fuse_val);
		if (slope != expected_slope) {
			unit_err(m, "%s: read_vin_cal_slope_intercept_fuse "
				 " reported bad slope 0x%x != 0x%x, i=%d\n",
				 __func__, slope, expected_slope, i);
			ret = UNIT_FAIL;
		}
		if (intercept != expected_intercept) {
			unit_err(m, "%s: read_vin_cal_slope_intercept_fuse "
				" reported bad intercept 0x%x != 0x%x, i=%d\n",
				__func__, intercept, expected_intercept, i);
			ret = UNIT_FAIL;
		}

		result = g->ops.fuse.read_vin_cal_gain_offset_fuse(g,
						vin_test_table[i].vin_id,
						&gain, &offset);
		if (result != 0) {
			unit_err(m, "%s: read_vin_cal_gain_offset_fuse "
				" returned error %d, i = %d\n",
				__func__, result, i);
			ret = UNIT_FAIL;
		}

		expected_gain = fuse_expected_gain(this_fuse_val);
		if (gain != expected_gain) {
			unit_err(m, "%s: read_vin_cal_gain_offset_fuse "
				 " reported bad gain 0x%x != 0x%x, i=%d\n",
				 __func__, gain, expected_gain, i);
			ret = UNIT_FAIL;
		}

		expected_offset = fuse_expected_offset(this_fuse_val);
		if (offset != expected_offset) {
			unit_err(m, "%s: read_vin_cal_gain_offset_fuse "
				 " reported bad offset 0x%x != 0x%x, i=%d\n",
				 __func__, offset, expected_offset, i);
			ret = UNIT_FAIL;
		}
	}

	/* test invalid GPC0 data special case */
	nvgpu_posix_io_writel_reg_space(g, GP106_FUSE_OPT_ADC_CAL_GPC0,
					~0U);
	result = g->ops.fuse.read_vin_cal_slope_intercept_fuse(g,
						CTRL_CLK_VIN_ID_GPC0,
						&slope, &intercept);
	if (result == 0) {
		unit_err(m, "%s: read_vin_cal_slope_intercept_fuse did NOT "
			 " return error for bad GPC0 data\n", __func__);
		ret = UNIT_FAIL;
	}
	result = g->ops.fuse.read_vin_cal_gain_offset_fuse(g,
							CTRL_CLK_VIN_ID_GPC0,
							&gain, &offset);
	if (result == 0) {
		unit_err(m, "%s: read_vin_cal_gain_offset_fuse did NOT "
			 " return error for bad GPC0 data\n", __func__);
		ret = UNIT_FAIL;
	}
	/* restore valid data */
	nvgpu_posix_io_writel_reg_space(g, GP106_FUSE_OPT_ADC_CAL_GPC0,
					0U);

	/* test invalid GPC1 data for the bad delta data case */
	nvgpu_posix_io_writel_reg_space(g, GP106_FUSE_OPT_ADC_CAL_GPC1_DELTA,
					~0U);
	result = g->ops.fuse.read_vin_cal_slope_intercept_fuse(g,
						CTRL_CLK_VIN_ID_GPC1,
						&slope,	&intercept);
	if (result == 0) {
		unit_err(m, "%s: read_vin_cal_slope_intercept_fuse did NOT "
			 " return error for bad GPC1 value\n", __func__);
		ret = UNIT_FAIL;
	}
	/* restore valid data */
	nvgpu_posix_io_writel_reg_space(g, GP106_FUSE_OPT_ADC_CAL_GPC1_DELTA,
					0U);
	/* test invalid VIN ID */
	result = g->ops.fuse.read_vin_cal_slope_intercept_fuse(g,
							       ~0U,
							       &slope,
							       &intercept);
	if (result == 0) {
		unit_err(m, "%s: read_vin_cal_slope_intercept_fuse did NOT "
			 " return error for invalid VIN ID\n", __func__);
		ret = UNIT_FAIL;
	}

	/* test API with invalid VIN id  */
	result = g->ops.fuse.read_vin_cal_gain_offset_fuse(g, ~0U,
							   &gain, &offset);
	if (result == 0) {
		unit_err(m, "%s: read_vin_cal_gain_offset_fuse did NOT "
			 " return error for invalid VIN id\n", __func__);
		ret = UNIT_FAIL;
	}

	return ret;
}
