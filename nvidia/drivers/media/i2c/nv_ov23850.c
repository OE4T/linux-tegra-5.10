/*
 * ov23850.c - ov23850 sensor driver
 *
 * Copyright (c) 2013-2022, NVIDIA CORPORATION.  All rights reserved.
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

#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/version.h>

#include <linux/seq_file.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>

#include <media/tegra_v4l2_camera.h>
#include <media/tegra-v4l2-camera.h>
#include <media/camera_common.h>
#include <media/ov23850.h>
#include "ov23850_mode_tbls.h"
#define CREATE_TRACE_POINTS
#include <trace/events/ov23850.h>

#define OV23850_MAX_COARSE_DIFF		0x20

#define OV23850_GAIN_SHIFT		8
#define OV23850_MIN_GAIN		(1 << OV23850_GAIN_SHIFT)
#define OV23850_MAX_GAIN		(16 << OV23850_GAIN_SHIFT)
#define OV23850_MIN_FRAME_LENGTH	(0x0)
#define OV23850_MAX_FRAME_LENGTH	(0x7fff)
#define OV23850_MIN_EXPOSURE_COARSE	(0x0002)
#define OV23850_MAX_EXPOSURE_COARSE	\
	(OV23850_MAX_FRAME_LENGTH-OV23850_MAX_COARSE_DIFF)

#define OV23850_DEFAULT_GAIN		OV23850_MIN_GAIN
#define OV23850_DEFAULT_FRAME_LENGTH	(0x12C6)
#define OV23850_DEFAULT_EXPOSURE_COARSE	\
	(OV23850_DEFAULT_FRAME_LENGTH-OV23850_MAX_COARSE_DIFF)

#define OV23850_DEFAULT_MODE	OV23850_MODE_5632X3168
#define OV23850_DEFAULT_WIDTH	5632
#define OV23850_DEFAULT_HEIGHT	3168
#define OV23850_DEFAULT_DATAFMT	MEDIA_BUS_FMT_SBGGR10_1X10
#define OV23850_DEFAULT_CLK_FREQ	24000000

struct ov23850 {
	struct mutex			ov23850_camera_lock;
	struct camera_common_power_rail	power;
	int				numctrls;
	struct v4l2_ctrl_handler	ctrl_handler;
#if 0
	struct camera_common_eeprom_data eeprom[OV23850_EEPROM_NUM_BLOCKS];
	u8				eeprom_buf[OV23850_EEPROM_SIZE];
#endif
	struct i2c_client		*i2c_client;
	struct v4l2_subdev		*subdev;
	struct media_pad		pad;

	u32				frame_length;
	s32				group_hold_prev;
	bool				group_hold_en;
	struct regmap			*regmap;
	struct camera_common_data	*s_data;
	struct camera_common_pdata	*pdata;
	struct v4l2_ctrl		*ctrls[];
};

static const struct regmap_config sensor_regmap_config = {
	.reg_bits = 16,
	.val_bits = 8,
	.cache_type = REGCACHE_RBTREE,
};

static u16 ov23850_to_gain(u32 rep, int shift)
{
	u16 gain;
	int gain_int;
	int gain_dec;
	int min_int = (1 << shift);
	int step = 1;
	int num_step;
	int i;

	if (rep < 0x0100)
		rep = 0x0100;
	else if (rep > 0x0F80)
		rep = 0x0F80;

	/* last 4 bit of rep is
	 * decimal representation of gain */
	gain_int = (int)(rep >> shift);
	gain_dec = (int)(rep & ~(0xffff << shift));

	for (i = 1; gain_int >> i != 0; i++)
		;
	step = step << (5 - i);

	num_step = gain_dec * step / min_int;
	gain = 16 * gain_int + 16 * num_step / step;

	return gain;
}

static int ov23850_s_ctrl(struct v4l2_ctrl *ctrl);

static const struct v4l2_ctrl_ops ov23850_ctrl_ops = {
	.s_ctrl		= ov23850_s_ctrl,
};

static struct v4l2_ctrl_config ctrl_config_list[] = {
/* Do not change the name field for the controls! */
	{
		.ops = &ov23850_ctrl_ops,
		.id = TEGRA_CAMERA_CID_GAIN,
		.name = "Gain",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.flags = V4L2_CTRL_FLAG_SLIDER,
		.min = OV23850_MIN_GAIN,
		.max = OV23850_MAX_GAIN,
		.def = OV23850_DEFAULT_GAIN,
		.step = 1,
	},
	{
		.ops = &ov23850_ctrl_ops,
		.id = TEGRA_CAMERA_CID_FRAME_LENGTH,
		.name = "Frame Length",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.flags = V4L2_CTRL_FLAG_SLIDER,
		.min = OV23850_MIN_FRAME_LENGTH,
		.max = OV23850_MAX_FRAME_LENGTH,
		.def = OV23850_DEFAULT_FRAME_LENGTH,
		.step = 1,
	},
	{
		.ops = &ov23850_ctrl_ops,
		.id = TEGRA_CAMERA_CID_COARSE_TIME,
		.name = "Coarse Time",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.flags = V4L2_CTRL_FLAG_SLIDER,
		.min = OV23850_MIN_EXPOSURE_COARSE,
		.max = OV23850_MAX_EXPOSURE_COARSE,
		.def = OV23850_DEFAULT_EXPOSURE_COARSE,
		.step = 1,
	},
	{
		.ops = &ov23850_ctrl_ops,
		.id = TEGRA_CAMERA_CID_COARSE_TIME_SHORT,
		.name = "Coarse Time Short",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.flags = V4L2_CTRL_FLAG_SLIDER,
		.min = OV23850_MIN_EXPOSURE_COARSE,
		.max = OV23850_MAX_EXPOSURE_COARSE,
		.def = OV23850_DEFAULT_EXPOSURE_COARSE,
		.step = 1,
	},
	{
		.ops = &ov23850_ctrl_ops,
		.id = TEGRA_CAMERA_CID_GROUP_HOLD,
		.name = "Group Hold",
		.type = V4L2_CTRL_TYPE_INTEGER_MENU,
		.min = 0,
		.max = ARRAY_SIZE(switch_ctrl_qmenu) - 1,
		.menu_skip_mask = 0,
		.def = 0,
		.qmenu_int = switch_ctrl_qmenu,
	},
	{
		.ops = &ov23850_ctrl_ops,
		.id = TEGRA_CAMERA_CID_HDR_EN,
		.name = "HDR enable",
		.type = V4L2_CTRL_TYPE_INTEGER_MENU,
		.min = 0,
		.max = 0,
		.menu_skip_mask = 0,
		.def = 0,
		.qmenu_int = switch_ctrl_qmenu,
	},
	{
		.ops = &ov23850_ctrl_ops,
		.id = TEGRA_CAMERA_CID_EEPROM_DATA,
		.name = "EEPROM Data",
		.type = V4L2_CTRL_TYPE_STRING,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
		.min = 0,
		.max = OV23850_EEPROM_STR_SIZE,
		.step = 2,
	},
	{
		.ops = &ov23850_ctrl_ops,
		.id = TEGRA_CAMERA_CID_OTP_DATA,
		.name = "OTP Data",
		.type = V4L2_CTRL_TYPE_STRING,
		.flags = V4L2_CTRL_FLAG_READ_ONLY,
		.min = 0,
		.max = OV23850_OTP_STR_SIZE,
		.step = 2,
	},
	{
		.ops = &ov23850_ctrl_ops,
		.id = TEGRA_CAMERA_CID_FUSE_ID,
		.name = "Fuse ID",
		.type = V4L2_CTRL_TYPE_STRING,
		.flags = V4L2_CTRL_FLAG_READ_ONLY,
		.min = 0,
		.max = OV23850_FUSE_ID_STR_SIZE,
		.step = 2,
	},
};

static inline void ov23850_get_frame_length_regs(ov23850_reg *regs,
				u32 frame_length)
{
	regs->addr = OV23850_FRAME_LENGTH_ADDR_MSB;
	regs->val = (frame_length >> 8) & 0x7f;
	(regs + 1)->addr = OV23850_FRAME_LENGTH_ADDR_LSB;
	(regs + 1)->val = (frame_length) & 0xff;
}

static inline void ov23850_get_coarse_time_regs(ov23850_reg *regs,
				u32 coarse_time)
{
	regs->addr = OV23850_COARSE_TIME_ADDR_MSB;
	regs->val = (coarse_time >> 8) & 0x7f;
	(regs + 1)->addr = OV23850_COARSE_TIME_ADDR_LSB;
	(regs + 1)->val = (coarse_time) & 0xff;
}

static inline void ov23850_get_coarse_time_short_regs(ov23850_reg *regs,
				u32 coarse_time)
{
	regs->addr = OV23850_COARSE_TIME_SHORT_ADDR_MSB;
	regs->val = (coarse_time >> 8) & 0xff;
	(regs + 1)->addr = OV23850_COARSE_TIME_SHORT_ADDR_LSB;
	(regs + 1)->val = (coarse_time) & 0xff;
}

static inline void ov23850_get_gain_reg(ov23850_reg *regs,
				u16 gain)
{
	regs->addr = OV23850_GAIN_ADDR_MSB;
	regs->val = (gain >> 8) & 0x07;
	(regs + 1)->addr = OV23850_GAIN_ADDR_LSB;
	(regs + 1)->val = (gain) & 0xff;
}

static inline void ov23850_get_gain_short_reg(ov23850_reg *regs,
				u16 gain)
{
	regs->addr = OV23850_GAIN_SHORT_ADDR_MSB;
	regs->val = (gain >> 8) & 0x07;
	(regs + 1)->addr = OV23850_GAIN_SHORT_ADDR_LSB;
	(regs + 1)->val = (gain) & 0xff;
}

static int test_mode;
module_param(test_mode, int, 0644);

static inline int ov23850_read_reg(struct camera_common_data *s_data,
				u16 addr, u8 *val)
{
	struct ov23850 *priv = (struct ov23850 *)s_data->priv;
	int err = 0;
	u32 reg_val = 0;

	err = regmap_read(priv->regmap, addr, &reg_val);
	*val = reg_val & 0xFF;

	return err;
}

static int ov23850_write_reg(struct camera_common_data *s_data,
			     u16 addr, u8 val)
{
	int err;
	struct ov23850 *priv = (struct ov23850 *)s_data->priv;
	struct device *dev = &priv->i2c_client->dev;

	err = regmap_write(priv->regmap, addr, val);
	if (err)
		dev_err(dev, "%s: i2c write failed, %x = %x\n",
			__func__, addr, val);

	return err;
}

static int ov23850_write_table(struct ov23850 *priv,
				const ov23850_reg table[])
{
	return regmap_util_write_table_8(priv->regmap,
					 table,
					 NULL, 0,
					 OV23850_TABLE_WAIT_MS,
					 OV23850_TABLE_END);
}

static int ov23850_power_on(struct camera_common_data *s_data)
{
	int err = 0;
	struct ov23850 *priv = (struct ov23850 *)s_data->priv;
	struct camera_common_power_rail *pw = &priv->power;
	struct device *dev = &priv->i2c_client->dev;

	dev_dbg(dev, "%s: power on\n", __func__);

	if (priv->pdata->power_on) {
		err = priv->pdata->power_on(pw);
		if (err)
			dev_err(dev, "%s failed.\n", __func__);
		else
			pw->state = SWITCH_ON;
		return err;
	}

	if (gpio_is_valid(pw->reset_gpio))
		gpio_set_value(pw->reset_gpio, 0);
	if (gpio_is_valid(pw->pwdn_gpio))
		gpio_set_value(pw->pwdn_gpio, 0);
	usleep_range(10, 20);

	if (pw->avdd)
		err = regulator_enable(pw->avdd);
	if (err)
		goto ov23850_avdd_fail;

	if (pw->dvdd)
		err = regulator_enable(pw->dvdd);
	if (err)
		goto ov23850_dvdd_fail;

	if (pw->iovdd)
		err = regulator_enable(pw->iovdd);
	if (err)
		goto ov23850_iovdd_fail;

	if (pw->vcmvdd)
		err = regulator_enable(pw->vcmvdd);
	if (err)
		goto ov23850_vcmvdd_fail;

	if (gpio_is_valid(pw->pwdn_gpio))
		gpio_set_value(pw->pwdn_gpio, 1);
	if (gpio_is_valid(pw->reset_gpio))
		gpio_set_value(pw->reset_gpio, 1);

	usleep_range(5350, 5360);	/* 5ms + 8192 EXTCLK cycles */

	pw->state = SWITCH_ON;
	return 0;

ov23850_vcmvdd_fail:
	regulator_disable(pw->iovdd);

ov23850_iovdd_fail:
	regulator_disable(pw->dvdd);

ov23850_dvdd_fail:
	regulator_disable(pw->avdd);

ov23850_avdd_fail:
	dev_err(dev, "%s failed.\n", __func__);
	return -ENODEV;
}

static int ov23850_power_off(struct camera_common_data *s_data)
{
	int err = 0;
	struct ov23850 *priv = (struct ov23850 *)s_data->priv;
	struct camera_common_power_rail *pw = &priv->power;
	struct device *dev = &priv->i2c_client->dev;

	dev_dbg(dev, "%s: power off\n", __func__);

	if (priv->pdata->power_off) {
		err = priv->pdata->power_off(pw);
		if (!err)
			goto power_off_done;
		else
			dev_err(dev, "%s failed.\n", __func__);
		return err;
	}

	usleep_range(1, 2);
	if (gpio_is_valid(pw->reset_gpio))
		gpio_set_value(pw->reset_gpio, 0);
	if (gpio_is_valid(pw->pwdn_gpio))
		gpio_set_value(pw->pwdn_gpio, 0);
	usleep_range(1, 2);

	if (pw->vcmvdd)
		regulator_disable(pw->vcmvdd);
	if (pw->iovdd)
		regulator_disable(pw->iovdd);
	if (pw->dvdd)
		regulator_disable(pw->dvdd);
	if (pw->avdd)
		regulator_disable(pw->avdd);

power_off_done:
	pw->state = SWITCH_OFF;
	return 0;
}

static int ov23850_power_get(struct ov23850 *priv)
{
	struct camera_common_power_rail *pw = &priv->power;
	struct camera_common_pdata *pdata = priv->pdata;
	struct device *dev = &priv->i2c_client->dev;
	const char *mclk_name;
	struct clk *parent;
	int err = 0;

	mclk_name = priv->pdata->mclk_name ?
		    priv->pdata->mclk_name : "cam_mclk1";
	pw->mclk = devm_clk_get(dev, mclk_name);
	if (IS_ERR(pw->mclk)) {
		dev_err(dev, "unable to get clock %s\n", mclk_name);
		return PTR_ERR(pw->mclk);
	}

	parent = devm_clk_get(dev, "pllp_grtba");
	if (IS_ERR(parent))
		dev_err(dev, "devm_clk_get failed for pllp_grtba");
	else
		clk_set_parent(pw->mclk, parent);

	/* ananlog 2.7v */
	err |= camera_common_regulator_get(dev,
			&pw->avdd, pdata->regulators.avdd);
	/* digital 1.2v */
	err |= camera_common_regulator_get(dev,
			&pw->dvdd, pdata->regulators.dvdd);
	/* IO 1.8v */
	err |= camera_common_regulator_get(dev,
			&pw->iovdd, pdata->regulators.iovdd);

	if (!err) {
		pw->reset_gpio = pdata->reset_gpio;
		pw->pwdn_gpio = pdata->pwdn_gpio;
	}

	pw->state = SWITCH_OFF;
	return err;
}

static int ov23850_set_gain(struct ov23850 *priv, s32 val);
static int ov23850_set_frame_length(struct ov23850 *priv, s32 val);
static int ov23850_set_coarse_time(struct ov23850 *priv, s32 val);
static int ov23850_set_coarse_time_short(struct ov23850 *priv, s32 val);

static int ov23850_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_common_data *s_data = to_camera_common_data(&client->dev);
	struct ov23850 *priv = (struct ov23850 *)s_data->priv;
	struct v4l2_control control;
	int err;

	dev_dbg(&client->dev, "%s++\n", __func__);
	trace_ov23850_s_stream(sd->name, enable, s_data->mode);
	if (!enable) {
		err = ov23850_write_table(priv,
			mode_table[OV23850_MODE_STOP_STREAM]);
		if (err)
			return err;

		/* Wait for one frame to make sure sensor is set to
		 * software standby in V-blank
		 *
		 * delay = frame length rows * Tline (10 us)
		 */
		usleep_range(priv->frame_length * 10,
			priv->frame_length * 10 + 1000);
		return 0;
	}

	err = ov23850_write_table(priv, mode_table[OV23850_MODE_COMMON]);
	if (err)
		goto exit;

	if (s_data->mode < 0)
		return -EINVAL;

	err = ov23850_write_table(priv, mode_table[s_data->mode]);
	if (err)
		goto exit;

	if (s_data->override_enable) {
		/* write list of override regs for the asking frame length, */
		/* coarse integration time, and gain.                       */
		control.id = TEGRA_CAMERA_CID_GAIN;
		err = v4l2_g_ctrl(&priv->ctrl_handler, &control);
		err |= ov23850_set_gain(priv, control.value);
		if (err)
			dev_dbg(&client->dev, "%s: error gain override\n",
				__func__);

		control.id = TEGRA_CAMERA_CID_FRAME_LENGTH;
		err = v4l2_g_ctrl(&priv->ctrl_handler, &control);
		err |= ov23850_set_frame_length(priv, control.value);
		if (err)
			dev_dbg(&client->dev,
				"%s: error frame length override\n", __func__);

		control.id = TEGRA_CAMERA_CID_COARSE_TIME;
		err = v4l2_g_ctrl(&priv->ctrl_handler, &control);
		err |= ov23850_set_coarse_time(priv, control.value);
		if (err)
			dev_dbg(&client->dev,
				"%s: error coarse time override\n", __func__);

		control.id = TEGRA_CAMERA_CID_COARSE_TIME_SHORT;
		err = v4l2_g_ctrl(&priv->ctrl_handler, &control);
		err |= ov23850_set_coarse_time_short(priv, control.value);
		if (err)
			dev_dbg(&client->dev,
				"%s: error coarse time short override\n",
				__func__);
	}

	err = ov23850_write_table(priv, mode_table[OV23850_MODE_START_STREAM]);
	if (err)
		goto exit;

	if (test_mode)
		err = ov23850_write_table(priv,
			mode_table[OV23850_MODE_TEST_PATTERN]);

	return 0;
exit:
	dev_dbg(&client->dev, "%s: error setting stream\n", __func__);
	return err;
}

static int ov23850_g_input_status(struct v4l2_subdev *sd, u32 *status)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_common_data *s_data = to_camera_common_data(&client->dev);
	struct ov23850 *priv = (struct ov23850 *)s_data->priv;
	struct camera_common_power_rail *pw = &priv->power;

	*status = pw->state == SWITCH_ON;
	return 0;
}

static struct v4l2_subdev_video_ops ov23850_subdev_video_ops = {
	.s_stream	= ov23850_s_stream,
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	.g_mbus_config	= camera_common_g_mbus_config,
#endif
	.g_input_status	= ov23850_g_input_status,
};

static struct v4l2_subdev_core_ops ov23850_subdev_core_ops = {
	.s_power	= camera_common_s_power,
};

static int ov23850_get_fmt(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_format *format)
{
	return camera_common_g_fmt(sd, &format->format);
}

static int ov23850_set_fmt(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
	struct v4l2_subdev_format *format)
{
	int ret;

	if (format->which == V4L2_SUBDEV_FORMAT_TRY)
		ret = camera_common_try_fmt(sd, &format->format);
	else
		ret = camera_common_s_fmt(sd, &format->format);

	return ret;
}

static struct v4l2_subdev_pad_ops ov23850_subdev_pad_ops = {
	.set_fmt = ov23850_set_fmt,
	.get_fmt = ov23850_get_fmt,
	.enum_mbus_code = camera_common_enum_mbus_code,
	.enum_frame_size	= camera_common_enum_framesizes,
	.enum_frame_interval	= camera_common_enum_frameintervals,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
	.get_mbus_config	= camera_common_get_mbus_config,
#endif
};

static struct v4l2_subdev_ops ov23850_subdev_ops = {
	.core	= &ov23850_subdev_core_ops,
	.video	= &ov23850_subdev_video_ops,
	.pad	= &ov23850_subdev_pad_ops,
};

const static struct of_device_id ov23850_of_match[] = {
	{ .compatible = "ovti,ov23850", },
	{ },
};

static struct camera_common_sensor_ops ov23850_common_ops = {
	.power_on = ov23850_power_on,
	.power_off = ov23850_power_off,
	.write_reg = ov23850_write_reg,
	.read_reg = ov23850_read_reg,
};

static int ov23850_set_group_hold(struct ov23850 *priv)
{
	struct device *dev = &priv->i2c_client->dev;
	int err;
	int gh_prev = switch_ctrl_qmenu[priv->group_hold_prev];

	if (priv->group_hold_en == true && gh_prev == SWITCH_OFF) {
		err = ov23850_write_reg(priv->s_data,
				       OV23850_GROUP_HOLD_ADDR, 0x00);
		if (err)
			goto fail;
		priv->group_hold_prev = 1;
	} else if (priv->group_hold_en == false && gh_prev == SWITCH_ON) {
		err = ov23850_write_reg(priv->s_data,
				       OV23850_GROUP_HOLD_ADDR, 0x10);
		err |= ov23850_write_reg(priv->s_data,
				       OV23850_GROUP_HOLD_ADDR, 0xE0);
		if (err)
			goto fail;
		priv->group_hold_prev = 0;
	}

	return 0;

fail:
	dev_dbg(dev, "%s: Group hold control error\n", __func__);
	return err;
}

static int ov23850_set_gain(struct ov23850 *priv, s32 val)
{
	struct device *dev = &priv->i2c_client->dev;
	ov23850_reg reg_list[2];
	ov23850_reg reg_list_short[2];
	int err;
	u16 gain;
	int i = 0;

	/* translate value */
	gain = ov23850_to_gain((u32)val, OV23850_GAIN_SHIFT);

	dev_dbg(dev, "%s: val: %d\n", __func__, gain);

	ov23850_get_gain_reg(reg_list, gain);
	ov23850_get_gain_short_reg(reg_list_short, gain);
	ov23850_set_group_hold(priv);

	/* writing long gain */
	for (i = 0; i < 2; i++) {
		err = ov23850_write_reg(priv->s_data, reg_list[i].addr,
			 reg_list[i].val);
		if (err)
			goto fail;
	}
	/* writing short gain */
	for (i = 0; i < 2; i++) {
		err = ov23850_write_reg(priv->s_data, reg_list_short[i].addr,
			 reg_list_short[i].val);
		if (err)
			goto fail;
	}

	return 0;

fail:
	dev_dbg(dev, "%s: GAIN control error\n", __func__);
	return err;
}

static int ov23850_set_frame_length(struct ov23850 *priv, s32 val)
{
	struct device *dev = &priv->i2c_client->dev;
	ov23850_reg reg_list[2];
	int err;
	u32 frame_length;
	int i = 0;

	frame_length = val;

	dev_dbg(dev, "%s: val: %d\n", __func__, frame_length);

	ov23850_get_frame_length_regs(reg_list, frame_length);
	ov23850_set_group_hold(priv);

	for (i = 0; i < 2; i++) {
		err = ov23850_write_reg(priv->s_data, reg_list[i].addr,
			 reg_list[i].val);
		if (err)
			goto fail;
	}

	priv->frame_length = frame_length;

	return 0;

fail:
	dev_dbg(dev, "%s: FRAME_LENGTH control error\n", __func__);
	return err;
}

static int ov23850_set_coarse_time(struct ov23850 *priv, s32 val)
{
	struct device *dev = &priv->i2c_client->dev;
	ov23850_reg reg_list[2];
	int err;
	u32 coarse_time;
	int i = 0;

	coarse_time = val;

	dev_dbg(dev, "%s: val: %d\n", __func__, coarse_time);

	ov23850_get_coarse_time_regs(reg_list, coarse_time);
	ov23850_set_group_hold(priv);

	for (i = 0; i < 2; i++) {
		err = ov23850_write_reg(priv->s_data, reg_list[i].addr,
			 reg_list[i].val);
		if (err)
			goto fail;
	}

	return 0;

fail:
	dev_dbg(dev, "%s: COARSE_TIME control error\n", __func__);
	return err;
}

static int ov23850_set_coarse_time_short(struct ov23850 *priv, s32 val)
{
	struct device *dev = &priv->i2c_client->dev;
	ov23850_reg reg_list[2];
	int err;
	struct v4l2_control hdr_control;
	int hdr_en;
	u32 coarse_time_short;
	int i = 0;

	/* check hdr enable ctrl */
	hdr_control.id = TEGRA_CAMERA_CID_HDR_EN;

	err = camera_common_g_ctrl(priv->s_data, &hdr_control);
	if (err < 0) {
		dev_err(dev, "could not find device ctrl.\n");
		return err;
	}

	hdr_en = switch_ctrl_qmenu[hdr_control.value];
	if (hdr_en == SWITCH_OFF)
		return 0;

	coarse_time_short = val;

	dev_dbg(dev, "%s: val: %d\n", __func__, coarse_time_short);

	ov23850_get_coarse_time_short_regs(reg_list, coarse_time_short);
	ov23850_set_group_hold(priv);

	for (i = 0; i < 2; i++) {
		err  = ov23850_write_reg(priv->s_data, reg_list[i].addr,
			 reg_list[i].val);
		if (err)
			goto fail;
	}

	return 0;

fail:
	dev_dbg(dev, "%s: COARSE_TIME_SHORT control error\n", __func__);
	return err;
}

#if 0
static int ov23850_eeprom_device_release(struct ov23850 *priv)
{
	int i;

	for (i = 0; i < OV23850_EEPROM_NUM_BLOCKS; i++) {
		if (priv->eeprom[i].i2c_client != NULL) {
			i2c_unregister_device(priv->eeprom[i].i2c_client);
			priv->eeprom[i].i2c_client = NULL;
		}
	}

	return 0;
}

static int ov23850_eeprom_device_init(struct ov23850 *priv)
{
	char *dev_name = "eeprom_ov23850";
	static struct regmap_config eeprom_regmap_config = {
		.reg_bits = 8,
		.val_bits = 8,
	};
	int i;
	int err;

	for (i = 0; i < OV23850_EEPROM_NUM_BLOCKS; i++) {
		priv->eeprom[i].adap = i2c_get_adapter(
				priv->i2c_client->adapter->nr);
		memset(&priv->eeprom[i].brd, 0, sizeof(priv->eeprom[i].brd));
		strncpy(priv->eeprom[i].brd.type, dev_name,
				sizeof(priv->eeprom[i].brd.type));
		priv->eeprom[i].brd.addr = OV23850_EEPROM_ADDRESS + i;
		priv->eeprom[i].i2c_client = i2c_new_device(
				priv->eeprom[i].adap, &priv->eeprom[i].brd);

		priv->eeprom[i].regmap = devm_regmap_init_i2c(
			priv->eeprom[i].i2c_client, &eeprom_regmap_config);
		if (IS_ERR(priv->eeprom[i].regmap)) {
			err = PTR_ERR(priv->eeprom[i].regmap);
			ov23850_eeprom_device_release(priv);
			return err;
		}
	}

	return 0;
}

static int ov23850_read_eeprom(struct ov23850 *priv)
{
	int err, i;
	struct v4l2_ctrl *ctrl;

	ctrl = v4l2_ctrl_find(&priv->ctrl_handler,
			TEGRA_CAMERA_CID_EEPROM_DATA);
	if (!ctrl) {
		dev_err(&priv->i2c_client->dev,
			"could not find device ctrl.\n");
		return -EINVAL;
	}

	for (i = 0; i < OV23850_EEPROM_NUM_BLOCKS; i++) {
		err = regmap_bulk_read(priv->eeprom[i].regmap, 0,
			&priv->eeprom_buf[i * OV23850_EEPROM_BLOCK_SIZE],
			OV23850_EEPROM_BLOCK_SIZE);
		if (err)
			return err;
	}

	for (i = 0; i < OV23850_EEPROM_SIZE; i++)
		sprintf(&ctrl->p_new.p_char[i*2], "%02x",
			priv->eeprom_buf[i]);
	return 0;
}

static int ov23850_write_eeprom(struct ov23850 *priv,
				char *string)
{
	struct device *dev = &priv->i2c_client->dev;
	int err;
	int i;
	u8 curr[3];
	unsigned long data;

	for (i = 0; i < OV23850_EEPROM_SIZE; i++) {
		curr[0] = string[i*2];
		curr[1] = string[i*2+1];
		curr[2] = '\0';

		err = kstrtol(curr, 16, &data);
		if (err) {
			dev_err(dev, "invalid eeprom string\n");
			return -EINVAL;
		}

		priv->eeprom_buf[i] = (u8)data;
		err = regmap_write(priv->eeprom[i >> 8].regmap,
				   i & 0xFF, (u8)data);
		if (err)
			return err;
		msleep(20);
	}
	return 0;
}
#endif

static int ov23850_read_otp_manual(struct ov23850 *priv,
				u8 *buf, u16 addr_start, u16 addr_end)
{
	struct device *dev = &priv->i2c_client->dev;
	u8 status;
	int i;
	int err;
	int size = addr_end - addr_start + 1;
	u8 isp;
	u16 addr_start_capped = addr_start;

	if (addr_start > 0x6A00)
		addr_start_capped = 0x69FF;

	usleep_range(10000, 11000);
	err = ov23850_write_table(priv, mode_table[OV23850_MODE_START_STREAM]);
	if (err)
		return err;

	err = ov23850_read_reg(priv->s_data, OV23850_OTP_ISP_CTRL_ADDR, &isp);
	if (err)
		return err;
	err = ov23850_write_reg(priv->s_data, OV23850_OTP_ISP_CTRL_ADDR,
				isp & 0xfe);
	if (err)
		return err;

	err = ov23850_write_reg(priv->s_data, OV23850_OTP_MODE_CTRL_ADDR, 0x40);
	if (err)
		return err;


	err = ov23850_write_reg(priv->s_data, OV23850_OTP_START_REG_ADDR_MSB,
				(addr_start_capped >> 8) & 0xff);
	if (err)
		return err;
	err = ov23850_write_reg(priv->s_data, OV23850_OTP_START_REG_ADDR_LSB,
				addr_start_capped & 0xff);
	if (err)
		return err;

	err = ov23850_write_reg(priv->s_data, OV23850_OTP_END_REG_ADDR_MSB,
				(addr_end >> 8) & 0xff);
	if (err)
		return err;
	err = ov23850_write_reg(priv->s_data, OV23850_OTP_END_REG_ADDR_LSB,
				addr_end & 0xff);
	if (err)
		return err;

	err = ov23850_write_reg(priv->s_data, OV23850_OTP_LOAD_CTRL_ADDR, 0x01);
	if (err)
		return err;

	usleep_range(10000, 11000);
	for (i = 0; i < size; i++) {
		err = ov23850_read_reg(priv->s_data, addr_start + i, &buf[i]);
		if (err)
			return err;

		err = ov23850_read_reg(priv->s_data,
				       OV23850_OTP_LOAD_CTRL_ADDR, &status);
		if (err)
			return err;

		if (status & OV23850_OTP_RD_BUSY_MASK) {
			dev_err(dev, "another OTP read in progress\n");
			return err;
		} else if (status & OV23850_OTP_BIST_ERROR_MASK) {
			dev_err(dev, "fuse id read error\n");
			return err;
		}
	}

	err = ov23850_write_table(priv,
			mode_table[OV23850_MODE_STOP_STREAM]);
	if (err)
		return err;

	err = ov23850_read_reg(priv->s_data, OV23850_OTP_ISP_CTRL_ADDR, &isp);
	if (err)
		return err;
	err = ov23850_write_reg(priv->s_data, OV23850_OTP_ISP_CTRL_ADDR,
				isp | 0x01);
	if (err)
		return err;

	return 0;
}

static int ov23850_otp_setup(struct ov23850 *priv)
{
	struct device *dev = &priv->i2c_client->dev;
	int err;
	int i;
	struct v4l2_ctrl *ctrl;
	u8 otp_buf[OV23850_OTP_SIZE];

	err = ov23850_read_otp_manual(priv,
				otp_buf,
				OV23850_OTP_START_ADDR,
				OV23850_OTP_END_ADDR);
	if (err)
		return -ENODEV;

	ctrl = v4l2_ctrl_find(&priv->ctrl_handler, TEGRA_CAMERA_CID_OTP_DATA);
	if (!ctrl) {
		dev_err(dev, "could not find device ctrl.\n");
		return -EINVAL;
	}

	for (i = 0; i < OV23850_OTP_SIZE; i++)
		sprintf(&ctrl->p_new.p_char[i*2], "%02x",
			otp_buf[i]);
	ctrl->p_cur.p_char = ctrl->p_new.p_char;

	return 0;
}

static int ov23850_fuse_id_setup(struct ov23850 *priv)
{
	struct device *dev = &priv->i2c_client->dev;
	int err;
	int i;
	struct v4l2_ctrl *ctrl;
	u8 fuse_id[OV23850_FUSE_ID_SIZE];

	err = ov23850_read_otp_manual(priv,
				fuse_id,
				OV23850_FUSE_ID_OTP_START_ADDR,
				OV23850_FUSE_ID_OTP_END_ADDR);
	if (err)
		return -ENODEV;

	ctrl = v4l2_ctrl_find(&priv->ctrl_handler, TEGRA_CAMERA_CID_FUSE_ID);
	if (!ctrl) {
		dev_err(dev, "could not find device ctrl.\n");
		return -EINVAL;
	}

	for (i = 0; i < OV23850_FUSE_ID_SIZE; i++)
		sprintf(&ctrl->p_new.p_char[i*2], "%02x",
			fuse_id[i]);
	ctrl->p_cur.p_char = ctrl->p_new.p_char;

	return 0;
}

static int ov23850_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ov23850 *priv =
		container_of(ctrl->handler, struct ov23850, ctrl_handler);
	struct device *dev = &priv->i2c_client->dev;
	int err = 0;

	if (priv->power.state == SWITCH_OFF)
		return 0;

	switch (ctrl->id) {
	case TEGRA_CAMERA_CID_GAIN:
		err = ov23850_set_gain(priv, ctrl->val);
		break;
	case TEGRA_CAMERA_CID_FRAME_LENGTH:
		err = ov23850_set_frame_length(priv, ctrl->val);
		break;
	case TEGRA_CAMERA_CID_COARSE_TIME:
		err = ov23850_set_coarse_time(priv, ctrl->val);
		break;
	case TEGRA_CAMERA_CID_COARSE_TIME_SHORT:
		err = ov23850_set_coarse_time_short(priv, ctrl->val);
		break;
	case TEGRA_CAMERA_CID_GROUP_HOLD:
		if (switch_ctrl_qmenu[ctrl->val] == SWITCH_ON) {
			priv->group_hold_en = true;
		} else {
			priv->group_hold_en = false;
			err = ov23850_set_group_hold(priv);
		}
		break;
	case TEGRA_CAMERA_CID_HDR_EN:
		break;
	default:
		dev_err(dev, "%s: unknown ctrl id.\n", __func__);
		return -EINVAL;
	}

	return err;
}

static int ov23850_ctrls_init(struct ov23850 *priv)
{
	struct i2c_client *client = priv->i2c_client;
	struct v4l2_ctrl *ctrl;
	int numctrls;
	int err;
	int i;

	dev_dbg(&client->dev, "%s++\n", __func__);

	numctrls = ARRAY_SIZE(ctrl_config_list);
	v4l2_ctrl_handler_init(&priv->ctrl_handler, numctrls);

	for (i = 0; i < numctrls; i++) {
		ctrl = v4l2_ctrl_new_custom(&priv->ctrl_handler,
			&ctrl_config_list[i], NULL);
		if (ctrl == NULL) {
			dev_err(&client->dev, "Failed to init %s ctrl\n",
				ctrl_config_list[i].name);
			continue;
		}

		if (ctrl_config_list[i].type == V4L2_CTRL_TYPE_STRING &&
			ctrl_config_list[i].flags & V4L2_CTRL_FLAG_READ_ONLY) {
			ctrl->p_new.p_char = devm_kzalloc(&client->dev,
				ctrl_config_list[i].max + 1, GFP_KERNEL);
		}
		priv->ctrls[i] = ctrl;
	}

	priv->numctrls = numctrls;
	priv->subdev->ctrl_handler = &priv->ctrl_handler;
	if (priv->ctrl_handler.error) {
		dev_err(&client->dev, "Error %d adding controls\n",
			priv->ctrl_handler.error);
		err = priv->ctrl_handler.error;
		goto error;
	}

	err = v4l2_ctrl_handler_setup(&priv->ctrl_handler);
	if (err) {
		dev_err(&client->dev,
			"Error %d setting default controls\n", err);
		goto error;
	}

	err = camera_common_s_power(priv->subdev, true);
	if (err) {
		dev_err(&client->dev,
			"Error %d during power on\n", err);
		err = -ENODEV;
		goto error;
	}

	err = ov23850_otp_setup(priv);
	if (err) {
		dev_err(&client->dev,
			"Error %d reading otp data\n", err);
		goto error_hw;
	}

	err = ov23850_fuse_id_setup(priv);
	if (err) {
		dev_err(&client->dev,
			"Error %d reading fuse id data\n", err);
		goto error_hw;
	}

	camera_common_s_power(priv->subdev, false);
	return 0;

error_hw:
	camera_common_s_power(priv->subdev, false);
error:
	v4l2_ctrl_handler_free(&priv->ctrl_handler);
	return err;
}

MODULE_DEVICE_TABLE(of, ov23850_of_match);

static struct camera_common_pdata *ov23850_parse_dt(struct i2c_client *client)
{
	struct device_node *np = client->dev.of_node;
	struct camera_common_pdata *board_priv_pdata;
	const struct of_device_id *match;
	int gpio;
	int err;

	match = of_match_device(ov23850_of_match, &client->dev);
	if (!match) {
		dev_err(&client->dev, "Failed to find matching dt id\n");
		return NULL;
	}

	board_priv_pdata = devm_kzalloc(&client->dev,
			   sizeof(*board_priv_pdata), GFP_KERNEL);

	err = of_property_read_string(np, "mclk",
				      &board_priv_pdata->mclk_name);
	if (err) {
		dev_err(&client->dev, "mclk not in DT\n");
		goto error;
	}

	gpio = of_get_named_gpio(np, "pwdn-gpios", 0);
	if (gpio < 0) {
		dev_err(&client->dev, "pwdn gpios not in DT\n");
		goto error;
	}
	board_priv_pdata->pwdn_gpio = (unsigned int)gpio;

	gpio = of_get_named_gpio(np, "reset-gpios", 0);
	if (gpio < 0) {
		dev_err(&client->dev, "reset gpios not in DT\n");
		goto error;
	}
	board_priv_pdata->reset_gpio = (unsigned int)gpio;

	err = of_property_read_string(np, "avdd-reg",
			&board_priv_pdata->regulators.avdd);
	if (err) {
		dev_err(&client->dev, "avdd-reg not in DT\n");
		goto error;
	}
	err = of_property_read_string(np, "dvdd-reg",
			&board_priv_pdata->regulators.dvdd);
	if (err) {
		dev_err(&client->dev, "dvdd-reg not in DT\n");
		goto error;
	}
	err = of_property_read_string(np, "iovdd-reg",
			&board_priv_pdata->regulators.iovdd);
	if (err) {
		dev_err(&client->dev, "iovdd-reg not in DT\n");
		goto error;
	}
	err = of_property_read_string(np, "vcmvdd-reg",
			&board_priv_pdata->regulators.vcmvdd);
	if (err) {
		dev_err(&client->dev, "vcmdd-reg not in DT\n");
		goto error;
	}

	return board_priv_pdata;

error:
	devm_kfree(&client->dev, board_priv_pdata);
	return NULL;
}

static int ov23850_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	dev_dbg(&client->dev, "%s:\n", __func__);
	return 0;
}

static const struct v4l2_subdev_internal_ops ov23850_subdev_internal_ops = {
	.open = ov23850_open,
};

static const struct media_entity_operations ov23850_media_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static int ov23850_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct camera_common_data *common_data;
	struct ov23850 *priv;
	int err;

	dev_info(&client->dev, "probing v4l2 sensor\n");

	common_data = devm_kzalloc(&client->dev,
			    sizeof(struct camera_common_data), GFP_KERNEL);

	priv = devm_kzalloc(&client->dev,
			    sizeof(struct ov23850) + sizeof(struct v4l2_ctrl *)
			    * ARRAY_SIZE(ctrl_config_list), GFP_KERNEL);

	priv->regmap = devm_regmap_init_i2c(client, &sensor_regmap_config);
	if (IS_ERR(priv->regmap)) {
		dev_err(&client->dev,
			"regmap init failed: %ld\n", PTR_ERR(priv->regmap));
		return -ENODEV;
	}


	priv->pdata = ov23850_parse_dt(client);
	if (!priv->pdata) {
		dev_err(&client->dev, "unable to get platform data\n");
		return -EFAULT;
	}

	common_data->ops		= &ov23850_common_ops;
	common_data->ctrl_handler	= &priv->ctrl_handler;
	common_data->dev		= &client->dev;
	common_data->frmfmt		= &ov23850_frmfmt[0];
	common_data->colorfmt		= camera_common_find_datafmt(
					  OV23850_DEFAULT_DATAFMT);
	common_data->power		= &priv->power;
	common_data->ctrls		= priv->ctrls;
	common_data->priv		= (void *)priv;
	common_data->numctrls		= ARRAY_SIZE(ctrl_config_list);
	common_data->numfmts		= ARRAY_SIZE(ov23850_frmfmt);
	common_data->def_mode		= OV23850_DEFAULT_MODE;
	common_data->def_width		= OV23850_DEFAULT_WIDTH;
	common_data->def_height		= OV23850_DEFAULT_HEIGHT;
	common_data->fmt_width		= common_data->def_width;
	common_data->fmt_height		= common_data->def_height;
	common_data->def_clk_freq	= OV23850_DEFAULT_CLK_FREQ;

	priv->i2c_client		= client;
	priv->s_data			= common_data;
	priv->subdev			= &common_data->subdev;
	priv->subdev->dev		= &client->dev;
	priv->s_data->dev		= &client->dev;
	priv->group_hold_prev		= 0;

	err = ov23850_power_get(priv);
	if (err)
		return err;

	err = camera_common_initialize(common_data, "ov23850");
	if (err) {
		dev_err(&client->dev, "Failed to initialize ov23850\n");
		return err;
	}

	v4l2_i2c_subdev_init(&common_data->subdev, client,
			     &ov23850_subdev_ops);

	err = ov23850_ctrls_init(priv);
	if (err)
		return err;

#if 0
	/* eeprom interface */
	err = ov23850_eeprom_device_init(priv);
	if (err)
		dev_err(&client->dev,
			"Failed to allocate eeprom register map: %d\n", err);
#endif

	priv->subdev->internal_ops = &ov23850_subdev_internal_ops;
	priv->subdev->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
		     V4L2_SUBDEV_FL_HAS_EVENTS;

#if defined(CONFIG_MEDIA_CONTROLLER)
	priv->pad.flags = MEDIA_PAD_FL_SOURCE;
	priv->subdev->entity.ops = &ov23850_media_ops;
	err = tegra_media_entity_init(&priv->subdev->entity, 1,
				&priv->pad, true, true);
	if (err < 0) {
		dev_err(&client->dev, "unable to init media entity\n");
		return err;
	}
#endif

	err = v4l2_async_register_subdev(priv->subdev);
	if (err)
		return err;

	dev_info(&client->dev, "Detected OV23850 sensor\n");

	return 0;
}

static int
ov23850_remove(struct i2c_client *client)
{
	struct camera_common_data *s_data = to_camera_common_data(&client->dev);
	struct ov23850 *priv = (struct ov23850 *)s_data->priv;

	v4l2_async_unregister_subdev(priv->subdev);
#if defined(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&priv->subdev->entity);
#endif
	v4l2_ctrl_handler_free(&priv->ctrl_handler);
	camera_common_cleanup(s_data);

	return 0;
}

static const struct i2c_device_id ov23850_id[] = {
	{ "ov23850", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, ov23850_id);

static struct i2c_driver ov23850_i2c_driver = {
	.driver = {
		.name = "ov23850",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ov23850_of_match),
	},
	.probe = ov23850_probe,
	.remove = ov23850_remove,
	.id_table = ov23850_id,
};

module_i2c_driver(ov23850_i2c_driver);

MODULE_DESCRIPTION("I2C driver for OmniVision OV23850");
MODULE_AUTHOR("David Wang <davidw@nvidia.com>");
MODULE_LICENSE("GPL v2");

