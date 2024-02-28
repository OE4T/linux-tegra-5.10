/* Copyright (c) 2015 - 2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/* The NVS = NVidia Sensor framework */
/* See nvs_iio.c and nvs.h for documentation */


#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/nvs.h>

#define STM_NAME			"ais328dq"
#define STM_VENDOR			"STMicroelectronics"
#define STM_VERSION			(1)
#define STM_KBUF_SIZE			(32)
#define STM_DELAY_US_MAX		(255000)
#define STM_HW_DELAY_POR_MS		(50)
#define STM_HW_DELAY_US			(100)
#define STM_POLL_DELAY_MS_DFLT		(200)
#define STM_ERR_CNT_MAX			(20)
/* HW registers */
#define STM_REG_WHO_AM_I		(0x0F)
#define STM_REG_WHO_AM_I_ID		(0x32)
#define STM_REG_CTRL1			(0x20)
#define STM_REG_CTRL1_XEN		(0)
#define STM_REG_CTRL1_YEN		(1)
#define STM_REG_CTRL1_ZEN		(2)
#define STM_REG_CTRL1_DR		(3)
#define STM_REG_CTRL1_PM		(5)
#define STM_REG_CTRL2			(0x21)
#define STM_REG_CTRL2_HPCF		(0)
#define STM_REG_CTRL2_HPEN1		(2)
#define STM_REG_CTRL2_HPEN2		(3)
#define STM_REG_CTRL2_FDS		(4)
#define STM_REG_CTRL2_HPM		(5)
#define STM_REG_CTRL2_BOOT		(7)
#define STM_REG_CTRL3			(0x22)
#define STM_REG_CTRL3_I1_CFG		(0)
#define STM_REG_CTRL3_LIR1		(2)
#define STM_REG_CTRL3_I2_CFG		(3)
#define STM_REG_CTRL3_LIR2		(5)
#define STM_REG_CTRL3_PP_OD		(6)
#define STM_REG_CTRL3_IHL		(7)
#define STM_REG_CTRL4			(0x23)
#define STM_REG_CTRL4_SIM		(0)
#define STM_REG_CTRL4_ST		(1)
#define STM_REG_CTRL4_ST_SIGN		(3)
#define STM_REG_CTRL4_FS		(4)
#define STM_REG_CTRL4_FS_MASK		(0x30)
#define STM_REG_CTRL4_BLE		(6)
#define STM_REG_CTRL4_BDU		(7)
#define STM_REG_CTRL5			(0x24)
#define STM_REG_CTRL5_TURNON0		(0)
#define STM_REG_CTRL5_TURNON1		(1)
#define STM_REG_HP_FILTER_RESET		(0x25)
#define STM_REG_REFERENCE		(0x26)
#define STM_REG_STATUS			(0x27)
#define STM_REG_STATUS_XDA		(0)
#define STM_REG_STATUS_YDA		(1)
#define STM_REG_STATUS_ZDA		(2)
#define STM_REG_STATUS_ZYXDA		(3)
#define STM_REG_STATUS_DA_MASK		(0x0F)
#define STM_REG_STATUS_XOR		(4)
#define STM_REG_STATUS_YOR		(5)
#define STM_REG_STATUS_ZOR		(6)
#define STM_REG_STATUS_ZYXOR		(7)
#define STM_REG_OUT_X_L			(0x28)
#define STM_REG_OUT_X_H			(0x29)
#define STM_REG_OUT_Y_L			(0x2A)
#define STM_REG_OUT_Y_H			(0x2B)
#define STM_REG_OUT_Z_L			(0x2C)
#define STM_REG_OUT_Z_H			(0x2D)
#define STM_REG_INT1_CFG		(0x30)
#define STM_REG_INT1_SRC		(0x31)
#define STM_REG_INT1_THS		(0x32)
#define STM_REG_INT1_DURATION		(0x33)
#define STM_REG_INT2_CFG		(0x34)
#define STM_REG_INT2_SRC		(0x35)
#define STM_REG_INT2_THS		(0x36)
#define STM_REG_INT2_DURATION		(0x37)

#define STM_REG_INT_CFG_XLIE		(0)
#define STM_REG_INT_CFG_XHIE		(1)
#define STM_REG_INT_CFG_YLIE		(2)
#define STM_REG_INT_CFG_YHIE		(3)
#define STM_REG_INT_CFG_ZLIE		(4)
#define STM_REG_INT_CFG_ZHIE		(5)
#define STM_REG_INT_CFG_6D		(6)
#define STM_REG_INT_CFG_AOI		(7)
#define STM_REG_INT_SRC_XL		(0)
#define STM_REG_INT_SRC_XH		(1)
#define STM_REG_INT_SRC_YL		(2)
#define STM_REG_INT_SRC_YH		(3)
#define STM_REG_INT_SRC_ZL		(4)
#define STM_REG_INT_SRC_ZH		(5)
#define STM_REG_INT_SRC_IA		(6)

#define STM_I2C_AUTO_INC_AD		(0x80)

#define AXIS_X				(0)
#define AXIS_Y				(1)
#define AXIS_Z				(2)
#define AXIS_N				(3)

#define STM_ODR_OVERRIDE_CELLS		(3)

/* regulator names in order of powering on */
static char *stm_vregs[] = {
	"vdd",
	"vdd_IO",
};

static struct sensor_cfg stm_cfg_dflt = {
	.name			= "accelerometer",
	.kbuf_sz		= STM_KBUF_SIZE,
	.ch_n			= AXIS_N,
	.ch_sz			= -2,
	.part			= STM_NAME,
	.vendor			= STM_VENDOR,
	.version		= STM_VERSION,
	/* milliamp is dynamic based on delay */
	.milliamp		= {
		.ival		= 0,
		.fval		= 400000000,
	},
	.delay_us_min		= 1000,
	.delay_us_max		= 2000000,
	/* default matrix to get the attribute */
	.matrix[0]		= 1,
	.matrix[4]		= 1,
	.matrix[8]		= 1,
	.float_significance	= NVS_FLOAT_NANO
};

static unsigned short stm_i2c_addrs[] = {
	0x18,
	0x19,
};

struct stm_state {
	struct i2c_client *i2c;
	struct nvs_fn_if *nvs;
	void *nvs_st;
	struct sensor_cfg cfg;
	struct regulator_bulk_data vreg[ARRAY_SIZE(stm_vregs)];
	struct workqueue_struct *stm_work_queue;
	struct work_struct dw;
	u32* odr_override_tbl;		/* Sampling freq override table */
	unsigned int sts;		/* status flags */
	unsigned int errs;		/* error count */
	unsigned int enabled;		/* enable status */
	unsigned int delay_us;		/* requested sampling delay (us) */
	u16 i2c_addr;			/* I2C address */
	bool irq_dis;			/* interrupt host disable flag */

	u8 ru_ctrl2;			/* register user CTRL_REG2 */
	u8 ru_ctrl3;			/* register user CTRL_REG3 */
	u8 ru_ctrl4;			/* register user CTRL_REG3 */
	u8 ru_ctrl5;			/* register user CTRL_REG3 */
	u8 buf[7];			/* data buffer + status */
};

struct stm_rr {
	struct nvs_float max_range;
	struct nvs_float resolution;
	struct nvs_float scale;
};

static struct stm_rr stm_rr_tbl[] = {
	/* all accelerometer values are in g's  fval = NVS_FLOAT_NANO */
	{
		.max_range		= {
			.ival		= 19,
			.fval		= 613300000,
		},
		.resolution		= {
			.ival		= 0,
			.fval		= 598550,
		},
		.scale			= {
			.ival		= 0,
			.fval		= 598550,
		},
	},
	{
		.max_range		= {
			.ival		= 39,
			.fval		= 226600000,
		},
		.resolution		= {
			.ival		= 0,
			.fval		= 1197101,
		},
		.scale			= {
			.ival		= 0,
			.fval		= 1197101,
		},
	},
	{
		.max_range		= {
			.ival		= 78,
			.fval		= 453200000,
		},
		.resolution		= {
			.ival		= 0,
			.fval		= 2394202,
		},
		.scale			= {
			.ival		= 0,
			.fval		= 2394202,
		},
	},
	{
		.max_range		= {
			.ival		= 78,
			.fval		= 453200000,
		},
		.resolution		= {
			.ival		= 0,
			.fval		= 2394202,
		},
		.scale			= {
			.ival		= 0,
			.fval		= 2394202,
		},
	},
};

struct stm_odr {
	unsigned int us;
	u8 hw;
};


static struct stm_odr stm_odr_tbl[] = {
	{ 2000000, 0x58, },
	{ 1000000, 0x78, },
	{ 500000, 0x98, },
	{ 200000, 0xB8, },
	{ 100000, 0xD8, },
	{ 20000, 0x20, },
	{ 10000, 0x28, },
	{ 2500, 0x30, },
	{ 1000, 0x38, },
};


static void stm_err(struct stm_state *st)
{
	st->errs++;
	if (!st->errs)
		st->errs--;
}

static int stm_i2c_rd(struct stm_state *st, u8 reg, u16 len, u8 *val)
{
	struct i2c_msg msg[2];

	if (len > 1)
		reg |= STM_I2C_AUTO_INC_AD;
	msg[0].addr = st->i2c_addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &reg;
	msg[1].addr = st->i2c_addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = len;
	msg[1].buf = val;
	if (i2c_transfer(st->i2c->adapter, msg, 2) != 2) {
		stm_err(st);
		return -EIO;
	}

	return 0;
}

static int stm_i2c_wr(struct stm_state *st, u8 reg, u8 val)
{
	struct i2c_msg msg;
	u8 buf[2];

	if (st->i2c_addr) {
		buf[0] = reg;
		buf[1] = val;
		msg.addr = st->i2c_addr;
		msg.flags = 0;
		msg.len = 2;
		msg.buf = buf;
		if (i2c_transfer(st->i2c->adapter, &msg, 1) != 1) {
			stm_err(st);
			return -EIO;
		}
	}

	return 0;
}

static int stm_cmd(struct stm_state *st, int enable)
{
	u8 ctrl1 = enable;
	int i;
	int ret = 0;

	if (enable) {
		for (i = 0; i < ARRAY_SIZE(stm_odr_tbl) - 1; i++) {
			if (st->delay_us >= stm_odr_tbl[i].us)
				break;
		}
		ctrl1 |= stm_odr_tbl[i].hw;
		ret |= stm_i2c_wr(st, STM_REG_CTRL2, st->ru_ctrl2);
		ret |= stm_i2c_wr(st, STM_REG_CTRL3, st->ru_ctrl3);
		ret |= stm_i2c_wr(st, STM_REG_CTRL4, st->ru_ctrl4);
		ret |= stm_i2c_wr(st, STM_REG_CTRL5, st->ru_ctrl5);
	}
	ret |= stm_i2c_wr(st, STM_REG_CTRL1, ctrl1);
	return ret;
}

static int stm_pm(struct stm_state *st, bool enable)
{
	int ret = 0;

	if (enable) {
		ret = nvs_vregs_enable(&st->i2c->dev, st->vreg,
				       ARRAY_SIZE(stm_vregs));
		if (ret > 0)
			mdelay(STM_HW_DELAY_POR_MS);
	} else {
		if (st->i2c->irq) {
			ret = nvs_vregs_sts(st->vreg, ARRAY_SIZE(stm_vregs));
			if ((ret < 0) || (ret == ARRAY_SIZE(stm_vregs))) {
				ret = stm_i2c_wr(st, STM_REG_CTRL1, 0);
			} else if (ret > 0) {
				ret = nvs_vregs_enable(&st->i2c->dev, st->vreg,
						       ARRAY_SIZE(stm_vregs));
				mdelay(STM_HW_DELAY_POR_MS);
				ret = stm_i2c_wr(st, STM_REG_CTRL1, 0);
			}
		}
		ret |= nvs_vregs_disable(&st->i2c->dev, st->vreg,
					 ARRAY_SIZE(stm_vregs));
	}
	if (ret > 0)
		ret = 0;
	if (ret) {
		dev_err(&st->i2c->dev, "%s pwr=%x ERR=%d\n",
			__func__, enable, ret);
	} else {
		if (st->sts & NVS_STS_SPEW_MSG)
			dev_info(&st->i2c->dev, "%s pwr=%x\n",
				 __func__, enable);
	}
	return ret;
}

static void stm_pm_exit(struct stm_state *st)
{
	stm_pm(st, false);
	nvs_vregs_exit(&st->i2c->dev, st->vreg, ARRAY_SIZE(stm_vregs));
}

static int stm_pm_init(struct stm_state *st)
{
	int ret;

	st->delay_us = STM_DELAY_US_MAX;
	nvs_vregs_init(&st->i2c->dev,
		       st->vreg, ARRAY_SIZE(stm_vregs), stm_vregs);
	ret = stm_pm(st, true);
	return ret;
}

static int stm_rd(struct stm_state *st, s64 ts)
{
	int ret;

	ret = stm_i2c_rd(st, STM_REG_STATUS, sizeof(st->buf), st->buf);
	if (ret)
		return ret;

	if (st->buf[0] & STM_REG_STATUS_DA_MASK) {
		st->nvs->handler(st->nvs_st, &st->buf[1], ts);
	} else {
		ret = -EAGAIN;
	}
	return ret;
}

static void stm_read(struct stm_state *st, s64 ts)
{
	st->nvs->nvs_mutex_lock(st->nvs_st);
	if (st->enabled)
		stm_rd(st, ts);
	st->nvs->nvs_mutex_unlock(st->nvs_st);
}

static void stm_work(struct work_struct *ws)
{
	struct stm_state *st = container_of((struct work_struct *)ws,
					    struct stm_state, dw);
	s64 ts1=0, ts2=0;
	unsigned long delay_value = 0;
	u64 ts_diff = 0;

	usleep_range(st->delay_us, st->delay_us);
	while (st->enabled) {
		ts1 = nvs_timestamp();
		stm_read(st, ts1);
		ts2 = nvs_timestamp();
		ts_diff = (ts2 - ts1)/1000;
		if (st->delay_us > (unsigned int)ts_diff) {
			delay_value = st->delay_us - (unsigned int)ts_diff;
			usleep_range(delay_value, delay_value);
		}
	}
}

static irqreturn_t stm_irq_thread(int irq, void *dev_id)
{
	s64 ts = nvs_timestamp();
	struct stm_state *st = (struct stm_state *)dev_id;

	if (st->sts & NVS_STS_SPEW_IRQ)
		dev_info(&st->i2c->dev, "%s\n", __func__);
	stm_read(st, ts);
	return IRQ_HANDLED;
}

static void stm_disable_irq(struct stm_state *st)
{
	if (!st->irq_dis) {
		disable_irq_nosync(st->i2c->irq);
		st->irq_dis = true;
	}
}

static void stm_enable_irq(struct stm_state *st)
{
	if (st->irq_dis) {
		enable_irq(st->i2c->irq);
		st->irq_dis = false;
	}
}

static int stm_dis(struct stm_state *st)
{
	if (st->i2c->irq)
		stm_disable_irq(st);
	st->enabled = 0;
	return 0;
}

static int stm_disable(struct stm_state *st)
{
	int ret;

	ret = stm_dis(st);
	if (!ret)
		stm_pm(st, false);
	return ret;
}

static int stm_enable(void *client, int snsr_id, int enable)
{
	struct stm_state *st = (struct stm_state *)client;
	int ret;

	if (enable < 0)
		/* just return enable status */
		return st->enabled;

	if (enable) {
		ret = stm_pm(st, true);
		if (!ret) {
			ret = stm_cmd(st, enable);
			if (ret) {
				stm_disable(st);
			} else {
				st->enabled = enable;
				if (st->i2c->irq)
					stm_enable_irq(st);
				else
					queue_work(st->stm_work_queue, &st->dw);
			}
		}
	} else {
		ret = stm_disable(st);
	}
	return ret;
}

static int stm_batch(void *client, int snsr_id, int flags,
		     unsigned int period_us, unsigned int timeout_us)
{
	struct stm_state *st = (struct stm_state *)client;
	int ret = 0;

	if (timeout_us)
		/* timeout not supported (no HW FIFO) */
		return -EINVAL;

	if (period_us < st->cfg.delay_us_min)
		period_us = st->cfg.delay_us_min;
	if (period_us != st->delay_us) {
		st->delay_us = period_us;
		if (st->enabled)
			ret = stm_cmd(st, st->enabled);
	}
	return ret;
}

static int stm_max_range(void *client, int snsr_id, int max_range)
{
	struct stm_state *st = (struct stm_state *)client;

	if (max_range < 0 || max_range >= ARRAY_SIZE(stm_rr_tbl))
		return -EINVAL;

	st->cfg.max_range.ival =  stm_rr_tbl[max_range].max_range.ival;
	st->cfg.max_range.fval =  stm_rr_tbl[max_range].max_range.fval;
	st->cfg.resolution.ival =  stm_rr_tbl[max_range].resolution.ival;
	st->cfg.resolution.fval =  stm_rr_tbl[max_range].resolution.fval;
	st->cfg.scale.ival =  stm_rr_tbl[max_range].scale.ival;
	st->cfg.scale.fval =  stm_rr_tbl[max_range].scale.fval;
	if (max_range == 2)
		max_range = 3;
	st->ru_ctrl4 &= ~STM_REG_CTRL4_FS_MASK;
	st->ru_ctrl4 |= max_range << STM_REG_CTRL4_FS;
	if (st->enabled)
		stm_enable(st, snsr_id, st->enabled);
	return 0;
}

static int stm_reset(void *client, int snsr_id)
{
	struct stm_state *st = (struct stm_state *)client;
	unsigned int enabled = st->enabled;
	int ret;

	stm_dis(st);
	stm_pm(st, true);
	ret = stm_i2c_wr(st, STM_REG_CTRL2, 1 << STM_REG_CTRL2_BOOT);
	stm_enable(st, snsr_id, enabled);
	return ret;
}

static int stm_selftest(void *client, int snsr_id, char *buf)
{
	struct stm_state *st = (struct stm_state *)client;
	unsigned int enabled = st->enabled;
	ssize_t t;
	int ret;

	stm_dis(st);
	/* set self-test bit when enabled */
	st->ru_ctrl4 |= (1 << STM_REG_CTRL4_ST);
	/* enable */
	stm_enable(st, 0, 7);
	/* need to put a data acquire delay loop here */
	ret = stm_rd(st, nvs_timestamp());
	/* disable */
	stm_dis(st);
	st->ru_ctrl4 &= ~(1 << STM_REG_CTRL4_ST);
	if (buf) {
		if (ret < 0) {
			t = sprintf(buf, "ERR: %d\n", ret);
		} else {
			if (ret > 0)
				t = sprintf(buf, "%d FAIL", ret);
			else
				t = sprintf(buf, "%d PASS", ret);
			t += sprintf(buf + t, "   xyz: %hd %hd %hd\n",
				     st->buf[1],
				     st->buf[3],
				     st->buf[5]);
		}
	}
	/* restore */
	stm_enable(st, 0, enabled);
	if (buf)
		return t;

	return ret;
}

static int stm_regs(void *client, int snsr_id, char *buf)
{
	struct stm_state *st = (struct stm_state *)client;
	ssize_t t;
	u8 val[2];
	u8 i;
	int ret;

	t = sprintf(buf, "registers:\n");
	ret = stm_i2c_rd(st, STM_REG_WHO_AM_I, 1, val);
	if (ret)
		t += sprintf(buf + t, "0x%hhx=ERR\n", STM_REG_WHO_AM_I);
	else
		t += sprintf(buf + t, "0x%hhx=0x%hhx\n",
			     STM_REG_WHO_AM_I, val[0]);
	for (i = STM_REG_CTRL1; i < STM_REG_HP_FILTER_RESET; i++) {
		ret = stm_i2c_rd(st, i, 1, val);
		if (ret)
			t += sprintf(buf + t, "0x%hhx=ERR\n", i);
		else
			t += sprintf(buf + t, "0x%hhx=0x%hhx\n", i, val[0]);
	}
	for (i = STM_REG_REFERENCE; i <= STM_REG_STATUS; i++) {
		ret = stm_i2c_rd(st, i, 1, val);
		if (ret)
			t += sprintf(buf + t, "0x%hhx=ERR\n", i);
		else
			t += sprintf(buf + t, "0x%hhx=0x%hhx\n", i, val[0]);
	}
	for (i = STM_REG_OUT_X_L; i < STM_REG_OUT_Z_H; i += 2) {
		ret = stm_i2c_rd(st, i, 2, val);
		if (ret)
			t += sprintf(buf + t, "0x%hhx:0x%hhx=ERR\n",
				     i, i + 1);
		else
			t += sprintf(buf + t, "0x%hhx:0x%hhx=0x%hx\n",
				     i, i + 1, *((u16 *)val));
	}
	for (i = STM_REG_INT1_CFG; i <= STM_REG_INT2_DURATION; i++) {
		ret = stm_i2c_rd(st, i, 1, val);
		if (ret)
			t += sprintf(buf + t, "0x%hhx=ERR\n", i);
		else
			t += sprintf(buf + t, "0x%hhx=0x%hhx\n", i, val[0]);
	}
	return t;
}


static struct nvs_fn_dev stm_fn_dev = {
	.enable				= stm_enable,
	.batch				= stm_batch,
	.max_range			= stm_max_range,
	.reset				= stm_reset,
	.self_test			= stm_selftest,
	.regs				= stm_regs,
};

#ifdef CONFIG_PM_SLEEP
static int stm_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct stm_state *st = i2c_get_clientdata(client);
	int ret = 0;

	st->sts |= NVS_STS_SUSPEND;
	if (st->nvs && st->nvs_st)
		ret = st->nvs->suspend(st->nvs_st);
	if (st->sts & NVS_STS_SPEW_MSG)
		dev_info(&client->dev, "%s\n", __func__);
	return ret;
}

static int stm_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct stm_state *st = i2c_get_clientdata(client);
	int ret = 0;

	if (st->nvs && st->nvs_st)
		ret = st->nvs->resume(st->nvs_st);
	st->sts &= ~NVS_STS_SUSPEND;
	if (st->sts & NVS_STS_SPEW_MSG)
		dev_info(&client->dev, "%s\n", __func__);
	return ret;
}
#endif /* CONFIG_PM_SLEEP */
static SIMPLE_DEV_PM_OPS(stm_pm_ops, stm_suspend, stm_resume);

static void stm_shutdown(struct i2c_client *client)
{
	struct stm_state *st = i2c_get_clientdata(client);

	st->sts |= NVS_STS_SHUTDOWN;
	if (st->nvs && st->nvs_st)
		st->nvs->shutdown(st->nvs_st);
	if (st->sts & NVS_STS_SPEW_MSG)
		dev_info(&client->dev, "%s\n", __func__);
}

static int stm_remove(struct i2c_client *client)
{
	struct stm_state *st = i2c_get_clientdata(client);

	if (st != NULL) {
		stm_shutdown(client);
		if (st->nvs && st->nvs_st)
			st->nvs->remove(st->nvs_st);
		stm_pm_exit(st);
		destroy_workqueue(st->stm_work_queue);
	}
	dev_info(&client->dev, "%s\n", __func__);
	return 0;
}

static int stm_id_dev(struct stm_state *st, const char *name)
{
	u8 val;
	int ret = 0;

	ret = stm_i2c_rd(st, STM_REG_WHO_AM_I, 1, &val);
	if (!ret) {
		if (val == STM_REG_WHO_AM_I_ID)
			dev_info(&st->i2c->dev, "%s %s found\n",
				 __func__, name);
		else
			dev_info(&st->i2c->dev, "%s %hhx response @ I2C=%x\n",
				 __func__, val, st->i2c->addr);
	}
	return ret;
}

static int stm_id_i2c(struct stm_state *st,
		      const struct i2c_device_id *id)
{
	int i;
	int ret;

	for (i = 0; i < ARRAY_SIZE(stm_i2c_addrs); i++) {
		if (st->i2c->addr == stm_i2c_addrs[i])
			break;
	}

	if (i < ARRAY_SIZE(stm_i2c_addrs)) {
		st->i2c_addr = st->i2c->addr;
		ret = stm_id_dev(st, id->name);
	} else {
		for (i = 0; i < ARRAY_SIZE(stm_i2c_addrs); i++) {
			st->i2c_addr = stm_i2c_addrs[i];
			ret = stm_id_dev(st, id->name);
			if (!ret)
				break;
		}
	}
	if (ret)
		st->i2c_addr = 0;
	return ret;
}

static int stm_of_dt(struct stm_state *st, struct device_node *dn)
{
	int count;
	int i;
	int j;

	memcpy(&st->cfg, &stm_cfg_dflt, sizeof(st->cfg));
	if (dn) {
		/* device specific parameters */
		of_property_read_u8(dn, "CTRL_REG2", &st->ru_ctrl2);
		of_property_read_u8(dn, "CTRL_REG3", &st->ru_ctrl3);
		of_property_read_u8(dn, "CTRL_REG4", &st->ru_ctrl4);
		of_property_read_u8(dn, "CTRL_REG5", &st->ru_ctrl5);
		count = of_property_count_elems_of_size(dn, "stm_odr_override",
						      sizeof(u32));
		if (count > 0) {
			if ((count % STM_ODR_OVERRIDE_CELLS) != 0) {
				dev_err(&st->i2c->dev,
					"%s: Invalid ODR override table length\n",
					__func__);
				return -EINVAL;
			}
			st->odr_override_tbl = devm_kzalloc(&st->i2c->dev,
						sizeof(u32) * count, GFP_KERNEL);
			if (IS_ERR_OR_NULL(st->odr_override_tbl))
				return -ENOMEM;
			if (of_property_read_u32_array(dn, "stm_odr_override",
						st->odr_override_tbl, count)) {
				dev_err(&st->i2c->dev,
					" %s Fetching odr override table failed\n",
					__func__);
				return -EINVAL;
			}
			for (i = 0; i < count; i += STM_ODR_OVERRIDE_CELLS) {
				for (j = 0; j < ARRAY_SIZE(stm_odr_tbl); j++) {
					if (st->odr_override_tbl[i] == stm_odr_tbl[j].us) {
						stm_odr_tbl[j].us = st->odr_override_tbl[i+1];
						stm_odr_tbl[j].hw = (u8)st->odr_override_tbl[i+2];
						break;
					}
				}
			}
		}
	} else {
		dev_info(&st->i2c->dev, "%s dev.of_node=NULL\n", __func__);
	}
	stm_max_range(st, 0, (st->ru_ctrl4 & STM_REG_CTRL4_FS_MASK) >>
			      STM_REG_CTRL4_FS);
	return 0;
}

static int stm_probe(struct i2c_client *client,
		     const struct i2c_device_id *id)
{
	struct stm_state *st;
	unsigned long irqflags;
	int ret;

	st = devm_kzalloc(&client->dev, sizeof(*st), GFP_KERNEL);
	if (st == NULL) {
		dev_err(&client->dev, "%s devm_kzalloc ERR\n", __func__);
		return -ENOMEM;
	}

	i2c_set_clientdata(client, st);
	st->i2c = client;
	ret = stm_of_dt(st, client->dev.of_node);
	if (ret) {
		dev_err(&client->dev, "%s _of_dt ERR\n", __func__);
		goto stm_probe_exit;
	}

	stm_pm_init(st);
	ret = stm_id_i2c(st, id);
	if (ret) {
		dev_err(&client->dev, "%s _id_i2c ERR\n", __func__);
		ret = -ENODEV;
		goto stm_probe_exit;
	}

	stm_pm(st, false);
	ret = nvs_of_dt(client->dev.of_node, &st->cfg, NULL);
	if (ret < 0)
		dev_info(&client->dev, "%s nvs_of_dt ERR\n", __func__);
	stm_fn_dev.errs = &st->errs;
	stm_fn_dev.sts = &st->sts;
	st->nvs = nvs_iio();
	if (st->nvs == NULL) {
		ret = -ENODEV;
		goto stm_probe_exit;
	}

	ret = st->nvs->probe(&st->nvs_st, st, &client->dev,
			     &stm_fn_dev, &st->cfg);
	if (ret) {
		dev_err(&client->dev, "%s nvs_probe ERR\n", __func__);
		ret = -ENODEV;
		goto stm_probe_exit;
	}

	if (client->irq) {
		if (st->ru_ctrl3 & (1 << STM_REG_CTRL3_IHL))
			irqflags = IRQF_TRIGGER_FALLING | IRQF_ONESHOT;
		else
			irqflags = IRQF_TRIGGER_RISING | IRQF_ONESHOT;
		ret = request_threaded_irq(client->irq, NULL, stm_irq_thread,
					   irqflags, STM_NAME, st);
		if (ret) {
			dev_err(&client->dev, "%s req_threaded_irq ERR %d\n",
				__func__, ret);
			ret = -ENOMEM;
			goto stm_probe_exit;
		}
	} else {
		st->stm_work_queue = create_workqueue("stm_poll");
		if (!st->stm_work_queue)
			return -ENODEV;
		INIT_WORK(&st->dw, stm_work);
	}

	dev_info(&client->dev, "%s done\n", __func__);
	return 0;

stm_probe_exit:
	stm_remove(client);
	return ret;
}

static const struct i2c_device_id stm_i2c_device_id[] = {
	{ STM_NAME, 0 },
	{}
};

MODULE_DEVICE_TABLE(i2c, stm_i2c_device_id);

static const struct of_device_id stm_of_match[] = {
	{ .compatible = "stm,ais328dq", },
	{}
};

MODULE_DEVICE_TABLE(of, stm_of_match);

static struct i2c_driver stm_driver = {
	.class				= I2C_CLASS_HWMON,
	.probe				= stm_probe,
	.remove				= stm_remove,
	.shutdown			= stm_shutdown,
	.driver				= {
		.name			= STM_NAME,
		.owner			= THIS_MODULE,
		.of_match_table		= of_match_ptr(stm_of_match),
		.pm			= &stm_pm_ops,
	},
	.id_table			= stm_i2c_device_id,
};

module_i2c_driver(stm_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("AIS328DQ driver");
MODULE_AUTHOR("NVIDIA Corporation");

