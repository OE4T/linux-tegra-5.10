/*
 * drivers/thermal/pid_thermal_gov.c
 *
 * Copyright (c) 2013-2020, NVIDIA CORPORATION.  All rights reserved.

 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.

 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/thermal.h>
#include <linux/pid_thermal_gov.h>
#include <linux/version.h>

#include "thermal_core.h"

#define DRV_NAME	"pid_thermal_gov"

#define MAX_ERR_TEMP_DEFAULT		9000	/* in mC */
#define MAX_ERR_GAIN_DEFAULT		1000
#define GAIN_P_DEFAULT			1000
#define GAIN_D_DEFAULT			0
#define MAX_DOUT_DEFAULT		0
#define UP_COMPENSATION_DEFAULT		20
#define DOWN_COMPENSATION_DEFAULT	20

static struct pid_thermal_gov_params pm_default = {
	.max_err_temp		= MAX_ERR_TEMP_DEFAULT,
	.max_err_gain		= MAX_ERR_GAIN_DEFAULT,
	.gain_p			= GAIN_P_DEFAULT,
	.gain_d			= GAIN_D_DEFAULT,
	.max_dout		= MAX_DOUT_DEFAULT,
	.up_compensation	= UP_COMPENSATION_DEFAULT,
	.down_compensation	= DOWN_COMPENSATION_DEFAULT,
};

struct pid_thermal_gov_attribute {
	struct attribute attr;
	ssize_t (*show)(struct kobject *kobj, struct attribute *attr,
			char *buf);
	ssize_t (*store)(struct kobject *kobj, struct attribute *attr,
			 const char *buf, size_t count);
};

struct pid_thermal_governor {
	struct kobject kobj;
	struct pid_thermal_gov_params pm;
};

#define tz_to_gov(t)		\
	(t->governor_data)

#define kobj_to_gov(k)		\
	container_of(k, struct pid_thermal_governor, kobj)

#define attr_to_gov_attr(a)	\
	container_of(a, struct pid_thermal_gov_attribute, attr)

static ssize_t max_err_temp_show(struct kobject *kobj, struct attribute *attr,
				 char *buf)
{
	struct pid_thermal_governor *gov = kobj_to_gov(kobj);

	if (!gov)
		return -ENODEV;

	return sprintf(buf, "%d\n", gov->pm.max_err_temp);
}

static ssize_t max_err_temp_store(struct kobject *kobj, struct attribute *attr,
				  const char *buf, size_t count)
{
	struct pid_thermal_governor *gov = kobj_to_gov(kobj);
	int val;

	if (!gov)
		return -ENODEV;

	if (!sscanf(buf, "%d\n", &val))
		return -EINVAL;

	gov->pm.max_err_temp = val;
	return count;
}

static struct pid_thermal_gov_attribute max_err_temp_attr =
	__ATTR(max_err_temp, 0644, max_err_temp_show, max_err_temp_store);

static ssize_t max_err_gain_show(struct kobject *kobj, struct attribute *attr,
				 char *buf)
{
	struct pid_thermal_governor *gov = kobj_to_gov(kobj);

	if (!gov)
		return -ENODEV;

	return sprintf(buf, "%d\n", gov->pm.max_err_gain);
}

static ssize_t max_err_gain_store(struct kobject *kobj, struct attribute *attr,
				  const char *buf, size_t count)
{
	struct pid_thermal_governor *gov = kobj_to_gov(kobj);
	int val;

	if (!gov)
		return -ENODEV;

	if (!sscanf(buf, "%d\n", &val))
		return -EINVAL;

	gov->pm.max_err_gain = val;
	return count;
}

static struct pid_thermal_gov_attribute max_err_gain_attr =
	__ATTR(max_err_gain, 0644, max_err_gain_show, max_err_gain_store);

static ssize_t max_dout_show(struct kobject *kobj,
			     struct attribute *attr, char *buf)
{
	struct pid_thermal_governor *gov = kobj_to_gov(kobj);

	if (!gov)
		return -ENODEV;

	return sprintf(buf, "%lu\n", gov->pm.max_dout);
}

static ssize_t max_dout_store(struct kobject *kobj, struct attribute *attr,
			      const char *buf, size_t count)
{
	struct pid_thermal_governor *gov = kobj_to_gov(kobj);
	unsigned long val;

	if (!gov)
		return -ENODEV;

	if (!sscanf(buf, "%lu\n", &val))
		return -EINVAL;

	gov->pm.max_dout = val;
	return count;
}

static struct pid_thermal_gov_attribute max_dout_attr =
	__ATTR(max_dout, 0644, max_dout_show, max_dout_store);

static ssize_t gain_p_show(struct kobject *kobj, struct attribute *attr,
			   char *buf)
{
	struct pid_thermal_governor *gov = kobj_to_gov(kobj);

	if (!gov)
		return -ENODEV;

	return sprintf(buf, "%d\n", gov->pm.gain_p);
}

static ssize_t gain_p_store(struct kobject *kobj, struct attribute *attr,
			    const char *buf, size_t count)
{
	struct pid_thermal_governor *gov = kobj_to_gov(kobj);
	int val;

	if (!gov)
		return -ENODEV;

	if (!sscanf(buf, "%d\n", &val))
		return -EINVAL;

	gov->pm.gain_p = val;
	return count;
}

static struct pid_thermal_gov_attribute gain_p_attr =
	__ATTR(gain_p, 0644, gain_p_show, gain_p_store);

static ssize_t gain_d_show(struct kobject *kobj, struct attribute *attr,
			   char *buf)
{
	struct pid_thermal_governor *gov = kobj_to_gov(kobj);

	if (!gov)
		return -ENODEV;

	return sprintf(buf, "%d\n", gov->pm.gain_d);
}

static ssize_t gain_d_store(struct kobject *kobj, struct attribute *attr,
			    const char *buf, size_t count)
{
	struct pid_thermal_governor *gov = kobj_to_gov(kobj);
	int val;

	if (!gov)
		return -ENODEV;

	if (!sscanf(buf, "%d\n", &val))
		return -EINVAL;

	gov->pm.gain_d = val;
	return count;
}

static struct pid_thermal_gov_attribute gain_d_attr =
	__ATTR(gain_d, 0644, gain_d_show, gain_d_store);

static ssize_t up_compensation_show(struct kobject *kobj,
				    struct attribute *attr, char *buf)
{
	struct pid_thermal_governor *gov = kobj_to_gov(kobj);

	if (!gov)
		return -ENODEV;

	return sprintf(buf, "%lu\n", gov->pm.up_compensation);
}

static ssize_t up_compensation_store(struct kobject *kobj,
				     struct attribute *attr, const char *buf,
				     size_t count)
{
	struct pid_thermal_governor *gov = kobj_to_gov(kobj);
	unsigned long val;

	if (!gov)
		return -ENODEV;

	if (!sscanf(buf, "%lu\n", &val))
		return -EINVAL;

	gov->pm.up_compensation = val;
	return count;
}

static struct pid_thermal_gov_attribute up_compensation_attr =
	__ATTR(up_compensation, 0644,
	       up_compensation_show, up_compensation_store);

static ssize_t down_compensation_show(struct kobject *kobj,
				      struct attribute *attr, char *buf)
{
	struct pid_thermal_governor *gov = kobj_to_gov(kobj);

	if (!gov)
		return -ENODEV;

	return sprintf(buf, "%lu\n", gov->pm.down_compensation);
}

static ssize_t down_compensation_store(struct kobject *kobj,
				       struct attribute *attr, const char *buf,
				       size_t count)
{
	struct pid_thermal_governor *gov = kobj_to_gov(kobj);
	unsigned long val;

	if (!gov)
		return -ENODEV;

	if (!sscanf(buf, "%lu\n", &val))
		return -EINVAL;

	gov->pm.down_compensation = val;
	return count;
}

static struct pid_thermal_gov_attribute down_compensation_attr =
	__ATTR(down_compensation, 0644,
	       down_compensation_show, down_compensation_store);

static struct attribute *pid_thermal_gov_default_attrs[] = {
	&max_err_temp_attr.attr,
	&max_err_gain_attr.attr,
	&gain_p_attr.attr,
	&gain_d_attr.attr,
	&max_dout_attr.attr,
	&up_compensation_attr.attr,
	&down_compensation_attr.attr,
	NULL,
};

static ssize_t pid_thermal_gov_show(struct kobject *kobj,
				    struct attribute *attr, char *buf)
{
	struct pid_thermal_gov_attribute *gov_attr = attr_to_gov_attr(attr);

	if (!gov_attr->show)
		return -EIO;

	return gov_attr->show(kobj, attr, buf);
}

static ssize_t pid_thermal_gov_store(struct kobject *kobj,
				     struct attribute *attr, const char *buf,
				     size_t len)
{
	struct pid_thermal_gov_attribute *gov_attr = attr_to_gov_attr(attr);

	if (!gov_attr->store)
		return -EIO;

	return gov_attr->store(kobj, attr, buf, len);
}

static const struct sysfs_ops pid_thermal_gov_sysfs_ops = {
	.show	= pid_thermal_gov_show,
	.store	= pid_thermal_gov_store,
};

static struct kobj_type pid_thermal_gov_ktype = {
	.default_attrs	= pid_thermal_gov_default_attrs,
	.sysfs_ops	= &pid_thermal_gov_sysfs_ops,
};

static int pid_thermal_gov_bind(struct thermal_zone_device *tz)
{
	struct pid_thermal_governor *gov;
	struct pid_thermal_gov_params *params;
	int ret;

	gov = kzalloc(sizeof(struct pid_thermal_governor), GFP_KERNEL);
	if (!gov) {
		dev_err(&tz->device, "%s: Can't alloc governor data\n",
			DRV_NAME);
		return -ENOMEM;
	}

	ret = kobject_init_and_add(&gov->kobj, &pid_thermal_gov_ktype,
				   &tz->device.kobj, DRV_NAME);
	if (ret) {
		dev_err(&tz->device, "%s: Can't init kobject\n", DRV_NAME);
		kobject_put(&gov->kobj);
		kfree(gov);
		return ret;
	}

	params = (struct pid_thermal_gov_params *)tz->tzp->governor_params;
	gov->pm = params ? *params : pm_default;
	tz_to_gov(tz) = gov;

	return 0;
}

static void pid_thermal_gov_unbind(struct thermal_zone_device *tz)
{
	struct pid_thermal_governor *gov = tz_to_gov(tz);

	if (!gov)
		return;

	kobject_put(&gov->kobj);
	kfree(gov);
}

static void pid_thermal_gov_update_passive(struct thermal_zone_device *tz,
					   enum thermal_trip_type trip_type,
					   unsigned long old,
					   unsigned long new)
{
	if ((trip_type != THERMAL_TRIP_PASSIVE) &&
			(trip_type != THERMAL_TRIPS_NONE))
		return;

	if ((old == THERMAL_NO_TARGET) && (new != THERMAL_NO_TARGET))
		tz->passive++;
	else if ((old != THERMAL_NO_TARGET) && (new == THERMAL_NO_TARGET))
		tz->passive--;
}

static unsigned long
pid_thermal_gov_get_target(struct thermal_zone_device *tz,
			   struct thermal_cooling_device *cdev,
			   enum thermal_trip_type trip_type,
			   int trip_temp)
{
	struct pid_thermal_governor *gov = tz_to_gov(tz);
	int last_temperature = tz->passive ? tz->last_temperature : trip_temp;
	int passive_delay = tz->passive ? tz->passive_delay : MSEC_PER_SEC;
	s64 proportional, derivative, sum_err, max_err;
	unsigned long max_state, cur_state, target, compensation;

	if (cdev->ops->get_max_state(cdev, &max_state) < 0)
		return 0;

	if (cdev->ops->get_cur_state(cdev, &cur_state) < 0)
		return 0;

	max_err = (s64)gov->pm.max_err_temp * (s64)gov->pm.max_err_gain;

	/* Calculate proportional term */
	proportional = (s64)tz->temperature - (s64)trip_temp;
	proportional *= gov->pm.gain_p;

	/* Calculate derivative term */
	derivative = (s64)tz->temperature - (s64)last_temperature;
	derivative *= gov->pm.gain_d;
	derivative *= MSEC_PER_SEC;
	derivative = div64_s64(derivative, passive_delay);
	if (gov->pm.max_dout) {
		s64 max_dout = div64_s64(max_err * gov->pm.max_dout, 100);
		if (derivative < 0)
			derivative = -min_t(s64, abs(derivative), max_dout);
		else
			derivative = min_t(s64, derivative, max_dout);
	}

	sum_err = max_t(s64, proportional + derivative, 0);
	sum_err = min_t(s64, sum_err, max_err);
	sum_err = sum_err * max_state + max_err - 1;
	target = (unsigned long)div64_s64(sum_err, max_err);

	/* Apply compensation */
	if (target == cur_state)
		return target;

	if (target > cur_state) {
		compensation = DIV_ROUND_UP(gov->pm.up_compensation *
					    (target - cur_state), 100);
		target = min(cur_state + compensation, max_state);
	} else if (target < cur_state) {
		compensation = DIV_ROUND_UP(gov->pm.down_compensation *
					    (cur_state - target), 100);
		target = (cur_state > compensation) ?
			 (cur_state - compensation) : 0;
	}

	return target;
}

static int pid_thermal_gov_throttle(struct thermal_zone_device *tz, int trip)
{
	struct thermal_instance *instance;
	enum thermal_trip_type trip_type;
	int trip_temp, hyst = 0;
	unsigned long target;

	tz->ops->get_trip_type(tz, trip, &trip_type);
	tz->ops->get_trip_temp(tz, trip, &trip_temp);
	if (tz->ops->get_trip_hyst)
		tz->ops->get_trip_hyst(tz, trip, &hyst);

	mutex_lock(&tz->lock);

	list_for_each_entry(instance, &tz->thermal_instances, tz_node) {
		if ((instance->trip != trip) ||
				((tz->temperature < trip_temp) &&
				 (instance->target == THERMAL_NO_TARGET)))
			continue;

		if (instance->upper == instance->lower) {
			target = instance->upper;
		} else {
			target = pid_thermal_gov_get_target(tz, instance->cdev,
							trip_type, trip_temp);
			target = min(max(target, instance->lower),
				     instance->upper);
		}

		if ((tz->temperature < trip_temp - hyst) &&
				(instance->target == instance->lower) &&
				(target == instance->lower))
			target = THERMAL_NO_TARGET;

		if (instance->target == target)
			continue;

		pid_thermal_gov_update_passive(tz, trip_type, instance->target,
					       target);
		instance->target = target;
		instance->cdev->updated = false;
	}

	list_for_each_entry(instance, &tz->thermal_instances, tz_node)
		thermal_cdev_update(instance->cdev);

	mutex_unlock(&tz->lock);

	return 0;
}

static int pid_thermal_gov_of_parse(struct thermal_zone_params *tzp,
				    struct device_node *np)
{
	u32 val;
	struct pid_thermal_gov_params *gpm;

	gpm = kzalloc(sizeof(struct pid_thermal_gov_params), GFP_KERNEL);
	if (!gpm)
		return -ENOMEM;

	*gpm = pm_default; /* start with default parameters */

	/* parse and populate governor_params explicitly specified in DT */
	if (!of_property_read_u32(np, "max_err_temp", &val))
		gpm->max_err_temp = val;
	if (!of_property_read_u32(np, "max_err_gain", &val))
		gpm->max_err_gain = val;
	if (!of_property_read_u32(np, "gain_p", &val))
		gpm->gain_p = val;
	if (!of_property_read_u32(np, "gain_d", &val))
		gpm->gain_d = val;
	if (!of_property_read_u32(np, "max_dout", &val))
		gpm->max_dout = val;
	if (!of_property_read_u32(np, "up_compensation", &val))
		gpm->up_compensation = val;
	if (!of_property_read_u32(np, "down_compensation", &val))
		gpm->down_compensation = val;

	tzp->governor_params = gpm;
	return 0;
}

static struct thermal_governor pid_thermal_gov = {
	.name		= DRV_NAME,
	.bind_to_tz	= pid_thermal_gov_bind,
	.unbind_from_tz	= pid_thermal_gov_unbind,
	.throttle	= pid_thermal_gov_throttle,
	.of_parse	= pid_thermal_gov_of_parse,
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0)
int pid_thermal_gov_register(void)
{
	return thermal_register_governor(&pid_thermal_gov);
}

void pid_thermal_gov_unregister(void)
{
	thermal_unregister_governor(&pid_thermal_gov);
}
#else
THERMAL_GOVERNOR_DECLARE(pid_thermal_gov);
#endif
