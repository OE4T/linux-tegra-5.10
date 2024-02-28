/* Copyright (c) 2014-2018, NVIDIA CORPORATION.  All rights reserved.
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


#ifndef _NVS_H_
#define _NVS_H_

#include <linux/device.h>
#include <linux/regulator/consumer.h>

#define NVS_STS_SHUTDOWN		(1 << 0)
#define NVS_STS_SUSPEND			(1 << 1)
#define NVS_STS_SYS_N			(2)

#define NVS_STS_SPEW_MSG		(1 << (NVS_STS_SYS_N + 0))
#define NVS_STS_SPEW_DATA		(1 << (NVS_STS_SYS_N + 1))
#define NVS_STS_SPEW_BUF		(1 << (NVS_STS_SYS_N + 2))
#define NVS_STS_SPEW_IRQ		(1 << (NVS_STS_SYS_N + 3))
#define NVS_STS_SPEW_MSK		(NVS_STS_SPEW_MSG | \
					NVS_STS_SPEW_DATA | \
					NVS_STS_SPEW_BUF | \
					NVS_STS_SPEW_IRQ)
#define NVS_STS_DBG_N			(NVS_STS_SYS_N + 4)
#define NVS_STS_EXT_N			(NVS_STS_DBG_N)
#define NVS_STS_MSK			((1 << NVS_STS_DBG_N) - 1)

#define NVS_CHANNEL_N_MAX		(5)
#define NVS_FLOAT_SIGNIFICANCE_MICRO	(1000000) /* IIO_VAL_INT_PLUS_MICRO */
#define NVS_FLOAT_SIGNIFICANCE_NANO	(1000000000) /* IIO_VAL_INT_PLUS_NANO */

/* from AOS sensors.h */
#define SENSOR_TYPE_ACCELEROMETER		(1)
#define SENSOR_TYPE_MAGNETIC_FIELD		(2)
#define SENSOR_TYPE_ORIENTATION			(3)
#define SENSOR_TYPE_GYROSCOPE			(4)
#define SENSOR_TYPE_LIGHT			(5)
#define SENSOR_TYPE_PRESSURE			(6)
#define SENSOR_TYPE_TEMPERATURE			(7)
#define SENSOR_TYPE_PROXIMITY			(8)
#define SENSOR_TYPE_GRAVITY			(9)
#define SENSOR_TYPE_LINEAR_ACCELERATION		(10)
#define SENSOR_TYPE_ROTATION_VECTOR		(11)
#define SENSOR_TYPE_RELATIVE_HUMIDITY		(12)
#define SENSOR_TYPE_AMBIENT_TEMPERATURE		(13)
#define SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED	(14)
#define SENSOR_TYPE_GAME_ROTATION_VECTOR	(15)
#define SENSOR_TYPE_GYROSCOPE_UNCALIBRATED	(16)
#define SENSOR_TYPE_SIGNIFICANT_MOTION		(17)
#define SENSOR_TYPE_STEP_DETECTOR		(18)
#define SENSOR_TYPE_STEP_COUNTER		(19)
#define SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR	(20)
#define SENSOR_TYPE_HEART_RATE			(21)
#define SENSOR_TYPE_TILT_DETECTOR		(22)
#define SENSOR_TYPE_WAKE_GESTURE		(23)
#define SENSOR_TYPE_GLANCE_GESTURE		(24)
#define SENSOR_TYPE_PICK_UP_GESTURE		(25)
#define SENSOR_TYPE_WRIST_TILT_GESTURE		(26)
#define SENSOR_TYPE_DEVICE_ORIENTATION		(27)
#define SENSOR_TYPE_POSE_6DOF			(28)
#define SENSOR_TYPE_STATIONARY_DETECT		(29)
#define SENSOR_TYPE_MOTION_DETECT		(30)
#define SENSOR_TYPE_HEART_BEAT			(31)
#define SENSOR_TYPE_DYNAMIC_SENSOR_META		(32)
#define SENSOR_TYPE_ADDITIONAL_INFO		(33)

#define SENSOR_STATUS_NO_CONTACT		(-1)
#define SENSOR_STATUS_UNRELIABLE		(0)
#define SENSOR_STATUS_ACCURACY_LOW		(1)
#define SENSOR_STATUS_ACCURACY_MEDIUM		(2)
#define SENSOR_STATUS_ACCURACY_HIGH		(3)

#define REPORTING_MODE_SHIFT			(1)
#define REPORTING_MODE_MASK			(0xE)
#define DATA_INJECTION_SHIFT			(4)
#define DATA_INJECTION_MASK			(0x10)
#define DYNAMIC_SENSOR_SHIFT			(5)
#define DYNAMIC_SENSOR_MASK			(0x20)
#define ADDITIONAL_INFO_SHIFT			(6)
#define ADDITIONAL_INFO_MASK			(0x40)
#define SENSOR_FLAG_WAKE_UP			(0x1)
#define SENSOR_FLAG_ON_CHANGE_MODE		(0x2)
#define SENSOR_FLAG_ONE_SHOT_MODE		(0x4)
#define SENSOR_FLAG_SPECIAL_REPORTING_MODE	(0x6)
#define SENSOR_FLAG_SUPPORTS_DATA_INJECTION	DATA_INJECTION_MASK
#define SENSOR_FLAG_DYNAMIC_SENSOR		DYNAMIC_SENSOR_MASK
#define SENSOR_FLAG_ADDITIONAL_INFO		ADDITIONAL_INFO_MASK
/* end AOS sensors.h */
/* unconfigurable flags */
#define SENSOR_FLAG_READONLY_MASK	(REPORTING_MODE_MASK)
/* nvs_dsm SENSOR_TYPE_DYNAMIC_SENSOR_META message */
#define NVS_DSM_MSG_FLAGS_CONNECT	(7)

/* IIO limits sizeof(struct nvs_dsm_msg) to 31 hence the chars */
struct nvs_dsm_msg {
	unsigned char ver;
	unsigned char flags;
	int dev_id;
	int snsr_id;
	unsigned char uuid[16];
};

enum nvs_float_significance {
	NVS_FLOAT_MICRO			= 0, /* IIO_VAL_INT_PLUS_MICRO */
	NVS_FLOAT_NANO,			/* IIO_VAL_INT_PLUS_NANO */
	NVS_FLOAT_N_MAX,
};

enum NVS_KIF {				/* Define kernel interface type */
	NVS_KIF_AUTO	=0,		/* AUTO		*/
	NVS_KIF_IIO,			/* IIO		*/
	NVS_KIF_INPUT,			/* INPUT	*/
	NVS_KIF_RELAY,			/* RELAYFS	*/
	NVS_KIF_N,			/* N-Count	*/
};

/* Set the default kernel interface type	*/
#ifndef NVS_CFG_KIF
#define NVS_CFG_KIF			NVS_KIF_AUTO
#endif

struct nvs_float {
	int ival;
	int fval;
};

struct sensor_cfg {
	const char *name;		/* sensor name */
	int snsr_id;			/* sensor ID */
	int kbuf_sz;			/* kernel buffer size (n bytes) */
	int timestamp_sz;		/* hub: timestamp size (n bytes) */
	int snsr_data_n;		/* hub: number of data bytes */
	unsigned int ch_n;		/* number of channels */
	int ch_sz;			/* channel size (n bytes) */
	void *ch_inf;			/* if hub then NULL */
	/* the following is for android struct sensor_t */
	const char *part;
	const char *vendor;
	int version;
	struct nvs_float max_range;
	struct nvs_float resolution;
	struct nvs_float milliamp;
	int delay_us_min;
	int delay_us_max;
	unsigned int fifo_rsrv_evnt_cnt;
	unsigned int fifo_max_evnt_cnt;
	unsigned int flags;
	/* end of android struct sensor_t data */
	unsigned char uuid[16];		/* UUID (RFC 4122 byte order) */
	signed char matrix[9];		/* device orientation on platform */
	/* interpolation calibration */
	int uncal_lo;
	int uncal_hi;
	int cal_lo;
	int cal_hi;
	/* thresholds */
	int thresh_lo;
	int thresh_hi;
	int report_n;			/* report count for on-change sensor */
	enum nvs_float_significance float_significance;
	/* global scale/offset allows for a 1st order polynomial on the data
	 * e.g. data * scale + offset
	 */
	struct nvs_float scale;
	struct nvs_float offset;
	unsigned int ch_n_max;		/* NVS_CHANNEL_N_MAX */
	/* channel scale/offset allows for a 1st order polynomial per channel
	 * e.g. channel_data * channel_scale + channel_offset
	 */
	struct nvs_float scales[NVS_CHANNEL_N_MAX];
	struct nvs_float offsets[NVS_CHANNEL_N_MAX];
};

#define NVS_FN_DEV_VERSION		(1)
struct nvs_fn_dev {
/**
 * version of this structure.
 * Note that this is backward compatible with the non-versioned
 * structure since the first member was "release" and never
 * implemented thereby making that structure version 0 (from the
 * NULL pointer).
 * Populate with: NVS_FN_DEV_VERSION
 */
	unsigned int ver;
/**
 * sizeof this structure.
 * A little extra protection for ABI compatibility.
 * Populate with: sizeof(struct nvs_fn_dev)
 */
	unsigned int sizeof_struct;
/**
 * sts - status flags
 * used by both device and NVS layers
 * See NVS_STS_ defines
 */
	unsigned int *sts;
/**
 * errs - error counter
 * used by both device and NVS layers
 */
	unsigned int *errs;
/**
 * release - device is gone - driver cleanup
 * @client: clients private data
 * @snsr_id: sensor ID
 *
 * The device has been removed from the system.  All resources
 * for this device needs to be freed, possibly the driver
 * itself if it controls no more devices.
 */
	void (*release)(void *client, int snsr_id);
/**
 * enable - enable/disable the device
 * @client: clients private data
 * @snsr_id: sensor ID
 * @enable: 0 = off
 *          1 = on
 *          -1 = query status
 *
 * Returns device enable state or a negative error code.
 *
 * Note that the enable value may be a bitmap of the enabled
 * channel.
 */
	int (*enable)(void *client, int snsr_id, int enable);
/**
 * batch - see Android definition of batch
 * http://source.android.com/devices/sensors/batching.html
 * @client: clients private data
 * @snsr_id: sensor ID
 * @flags: see Android definition of flags (currently obsolete)
 * @period: period timeout in microseconds
 * @timeout: batch timeout in microseconds
 *
 * Returns 0 on success or a negative error code.
 *
 * Note that period should be implemented for setting delay if
 * batching is not supported.
 */
	int (*batch)(void *client, int snsr_id, int flags,
		     unsigned int period_us, unsigned int timeout_us);
/**
 * batch_read - see Android definition of batch
 * http://source.android.com/devices/sensors/batching.html
 * @client: clients private data
 * @snsr_id: sensor ID
 * @*period: pointer to sensor period in microseconds
 * @*timeout: pointer to batch timeout in microseconds
 *
 * Returns 0 on success or a negative error code.
 *
 * This call is specifically to read the actual rate period
 * and batch timeout set by the HW driver and not the requested
 * period and batch timeout passed with the above batch
 * function which may not necessarily be the same.
 *
 * The call may be made with either the period_us or timeout_us
 * pointer set to NULL, in which case the HW driver just ignores
 * that pointer without error.
 */
	int (*batch_read)(void *client, int snsr_id,
			  unsigned int *period_us, unsigned int *timeout_us);
/**
 * flush - see Android definition of flush
 * http://source.android.com/devices/sensors/batching.html
 * @client: clients private data
 * @snsr_id: sensor ID
 *
 * Returns 0 on success or a negative error code.
 *
 * Note that if not implemented at the device level, it is
 * implemented in the NVS layer.  In other words, if the device
 * does not support batching, leave this NULL.
 */
	int (*flush)(void *client, int snsr_id);
/**
 * resolution - set device resolution
 * @client: clients private data
 * @snsr_id: sensor ID
 * @resolution: resolution value
 *
 * Returns 0 on success or a negative error code.
 * If a value > 0 is returned then sensor_cfg->resolution is
 * updated as described in the below note.  This allows drivers
 * with multiple sensors to only have to implement the device
 * specific function for certain sensors and allow the NVS
 * layer to handle the others.
 *
 * Note that if not implemented, resolution changes will change
 * sensor_cfg->resolution.  If implemented, it is expected
 * that the resolution value will be device-specific. In other
 * words, only the device layer will understand the value which
 * will typically be used to change the mode.  This will
 * typically be an index mechanism.  See max_range for an
 * example.
 */
	int (*resolution)(void *client, int snsr_id, int resolution);
/**
 * max_range - set device max_range
 * @client: clients private data
 * @snsr_id: sensor ID
 * @max_range: max_range value
 *
 * Returns 0 on success or a negative error code.
 * If a value > 0 is returned then sensor_cfg->max_range is
 * updated as described in the below note.  This allows drivers
 * with multiple sensors to only have to implement the device
 * specific function for certain sensors and allow the NVS
 * layer to handle the others.
 *
 * Note that if not implemented, max_range changes will change
 * sensor_cfg->max_range.  If implemented, it is expected
 * that the max_range value will be device-specific. In other
 * words, only the device layer will understand the value which
 * will typically be used to change the mode.  This will
 * typically be an index mechanism.  For example, to select the
 * range for an accelerometer that can support 2g, 4g, and 16g,
 * the following values of 0, 1, and 2, can be written
 * respectively.  Once the index is written, the max_range can
 * be read to determine the value associated with the index.
 */
	int (*max_range)(void *client, int snsr_id, int max_range);
/**
 * scale - set device scale
 * @client: clients private data
 * @snsr_id: sensor ID
 * @channel: channel index
 * @scale: scale value
 *
 * Returns 0 on success or a negative error code.
 * If a value > 0 is returned then sensor_cfg->scale is updated
 * as described in the below note.  This allows drivers with
 * multiple sensors to only have to implement the device
 * specific function for certain sensors and allow the NVS
 * layer to handle the others.
 *
 * Note that if not implemented, scale changes will change
 * sensor_cfg->scale.  If implemented, it is expected
 * that the scale value will be device-specific. In other words,
 * only the device layer will understand the value which will
 * typically be used to change the mode.
 */
	int (*scale)(void *client, int snsr_id, int channel, int scale);
/**
 * offset - set device offset
 * @client: clients private data
 * @snsr_id: sensor ID
 * @channel: channel index
 * @offset: offset value
 *
 * Returns 0 on success or a negative error code.
 * If a value > 0 is returned then sensor_cfg->offset is updated
 * as described in the below note.  This allows drivers with
 * multiple sensors to only have to implement the device
 * specific function for certain sensors and allow the NVS
 * layer to handle the others.
 *
 * Note that if not implemented, offset changes will change
 * sensor_cfg->offset.  If implemented, it is expected
 * that the offset value will be device-specific. In other
 * words, only the device layer will understand the value which
 * will typically be used to set calibration.
 */
	int (*offset)(void *client, int snsr_id, int channel, int offset);
/**
 * thresh_lo - set device low threshold
 * @client: clients private data
 * @snsr_id: sensor ID
 * @thresh_lo: low threshold value
 *
 * Returns 0 on success or a negative error code.
 * If a value > 0 is returned then sensor_cfg->thresh_lo is
 * updated as described in the below note.  This allows drivers
 * with multiple sensors to only have to implement the device
 * specific function for certain sensors and allow the NVS
 * layer to handle the others.
 *
 * Note that if not implemented, thresh_lo changes will change
 * sensor_cfg->thresh_lo.  If implemented, it is expected
 * that the thresh_lo value will be device-specific. In other
 * words, only the device layer will understand the value.
 */
	int (*thresh_lo)(void *client, int snsr_id, int thresh_lo);
/**
 * thresh_hi - set device high threshold
 * @client: clients private data
 * @snsr_id: sensor ID
 * @thresh_hi: high threshold value
 *
 * Returns 0 on success or a negative error code.
 * If a value > 0 is returned then sensor_cfg->thresh_hi is
 * updated as described in the below note.  This allows drivers
 * with multiple sensors to only have to implement the device
 * specific function for certain sensors and allow the NVS
 * layer to handle the others.
 *
 * Note that if not implemented, thresh_hi changes will change
 * sensor_cfg->thresh_hi.  If implemented, it is expected
 * that the thresh_hi value will be device-specific. In other
 * words, only the device layer will understand the value.
 */
	int (*thresh_hi)(void *client, int snsr_id, int thresh_hi);
/**
 * reset - device reset
 * @client: clients private data
 * @snsr_id: sensor ID
 *
 * Returns 0 on success or a negative error code.
 *
 * Note a < 0 value for snsr_id is another reset option,
 * e.g. global device reset such as on a sensor hub.
 * Note mutex is locked for this function.
 */
	int (*reset)(void *client, int snsr_id);
/**
 * self_test - device self-test
 * @client: clients private data
 * @snsr_id: sensor ID
 * @buf: character buffer to write to
 *
 * Returns 0 on success or a negative error code if buf == NULL.
 * if buf != NULL, return number of characters.
 * Note mutex is locked for this function.
 */
	int (*self_test)(void *client, int snsr_id, char *buf);
/**
 * regs - device register dump
 * @client: clients private data
 * @snsr_id: sensor ID
 * @buf: character buffer to write to
 *
 * Returns buf count or a negative error code.
 */
	int (*regs)(void *client, int snsr_id, char *buf);
/**
 * nvs_write - nvs attribute write extension
 * @client: clients private data
 * @snsr_id: sensor ID
 * @nvs: value written to nvs attribute
 *
 * Returns 0 on success or a negative error code.
 *
 * Used to extend the functionality of the nvs attribute.
 */
	int (*nvs_write)(void *client, int snsr_id, unsigned int nvs);
/**
 * nvs_read - nvs attribute read extension
 * @client: clients private data
 * @snsr_id: sensor ID
 * @buf: character buffer to write to
 *
 * Returns buf count or a negative error code.
 *
 * Used to extend the functionality of the nvs attribute.
 */
	int (*nvs_read)(void *client, int snsr_id, char *buf);
};

struct nvs_fn_if {
	int (*probe)(void **handle, void *dev_client, struct device *dev,
		     struct nvs_fn_dev *fn_dev, struct sensor_cfg *snsr_cfg);
	int (*remove)(void *handle);
	void (*shutdown)(void *handle);
	void (*nvs_mutex_lock)(void *handle);
	void (*nvs_mutex_unlock)(void *handle);
	int (*suspend)(void *handle);
	int (*resume)(void *handle);
	int (*handler)(void *handle, void *buffer, s64 ts);
};

extern const char * const nvs_float_significances[];

struct nvs_fn_if *nvs_auto(int);
struct nvs_fn_if *nvs_relay(void);
struct nvs_fn_if *nvs_iio(void);
struct nvs_fn_if *nvs_input(void);
int nvs_of_dt(const struct device_node *np, struct sensor_cfg *cfg,
	      const char *dev_name);
int nvs_vreg_dis(struct device *dev, struct regulator_bulk_data *vreg);
int nvs_vregs_disable(struct device *dev, struct regulator_bulk_data *vregs,
		      unsigned int vregs_n);
int nvs_vreg_en(struct device *dev, struct regulator_bulk_data *vreg);
int nvs_vregs_enable(struct device *dev, struct regulator_bulk_data *vregs,
		     unsigned int vregs_n);
void nvs_vregs_exit(struct device *dev, struct regulator_bulk_data *vregs,
		   unsigned int vregs_n);
int nvs_vregs_init(struct device *dev, struct regulator_bulk_data *vregs,
		   unsigned int vregs_n, char **vregs_name);
int nvs_vregs_sts(struct regulator_bulk_data *vregs, unsigned int vregs_n);
s64 nvs_timestamp(void);
int nvs_dsm_relay(int dev_id, bool connect, int snsr_id, unsigned char *uuid);
int nvs_dsm_iio(int dev_id, bool connect, int snsr_id, unsigned char *uuid);
int nvs_dsm_input(int dev_id, bool connect, int snsr_id, unsigned char *uuid);

#endif /* _NVS_H_ */
