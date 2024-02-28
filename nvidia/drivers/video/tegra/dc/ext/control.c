/*
 * control.c: tegradc ext control interface.
 *
 * Copyright (c) 2011-2019, NVIDIA CORPORATION, All rights reserved.
 *
 * Author: Robert Morell <rmorell@nvidia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

#include "tegra_dc_ext_priv.h"
#include "../dc_common.h"
#include "../dc_priv.h"

#ifdef CONFIG_COMPAT
struct tegra_dc_ext_control_output_edid32 {
	__u32 handle;
	__u32 size;
	__u32 data;	/* void *data; */
};

#define TEGRA_DC_EXT_CONTROL_GET_OUTPUT_EDID32 \
	_IOWR('C', 0x02, struct tegra_dc_ext_control_output_edid32)
#endif

static struct tegra_dc_ext_control g_control;

int tegra_dc_ext_process_hotplug(int output)
{
	return tegra_dc_ext_queue_hotplug(&g_control, output);
}

bool tegra_dc_ext_is_userspace_active(void)
{
	return !list_empty(&g_control.users);
}

int tegra_dc_ext_process_vblank(int output, u64 timestamp)
{
	return tegra_dc_ext_queue_vblank(&g_control, output, timestamp);
}

int tegra_dc_ext_process_modechange(int output)
{
	return tegra_dc_ext_queue_modechange(&g_control, output);
}

static int
get_output_properties(struct tegra_dc_ext_control_output_properties *properties)
{
	struct tegra_dc *dc;

	if (properties->handle >= tegra_dc_get_numof_dispheads())
		return -EINVAL;

	dc = tegra_dc_get_dc(properties->handle);
	if (dc == NULL)
		return -EINVAL;

	properties->associated_head = tegra_dc_get_head(dc);
	properties->head_mask = (1 << properties->associated_head);

	switch (tegra_dc_get_out(dc)) {
	case TEGRA_DC_OUT_FAKE_DSIA:
	case TEGRA_DC_OUT_FAKE_DSIB:
	case TEGRA_DC_OUT_FAKE_DSI_GANGED:
	case TEGRA_DC_OUT_DSI:
		properties->type = tegra_dc_is_ext_panel(dc) ?
					TEGRA_DC_EXT_HDSI : TEGRA_DC_EXT_DSI;
		break;
	case TEGRA_DC_OUT_RGB:
		properties->type = TEGRA_DC_EXT_LVDS;
		break;
	case TEGRA_DC_OUT_HDMI:
		properties->type = TEGRA_DC_EXT_HDMI;
		break;
	case TEGRA_DC_OUT_LVDS:
		properties->type = TEGRA_DC_EXT_LVDS;
		break;
	case TEGRA_DC_OUT_DP:
	case TEGRA_DC_OUT_FAKE_DP:
		properties->type = tegra_dc_is_ext_panel(dc) ?
					TEGRA_DC_EXT_DP : TEGRA_DC_EXT_EDP;
		break;
	case TEGRA_DC_OUT_NULL:
		properties->type = TEGRA_DC_EXT_NULL;
		break;
	default:
		return -EINVAL;
	}
	properties->connected = tegra_dc_get_connected(dc);

	return 0;
}

static int get_output_edid(struct tegra_dc_ext_control_output_edid *edid)
{
	struct tegra_dc *dc;
	size_t user_size = edid->size;
	struct tegra_dc_edid *dc_edid = NULL;
	int ret = 0;

	if (edid->handle >= tegra_dc_get_numof_dispheads())
		return -EINVAL;

	dc = tegra_dc_get_dc(edid->handle);

	dc_edid = tegra_dc_get_edid(dc);
	if (IS_ERR(dc_edid))
		return PTR_ERR(dc_edid);

	if (!dc_edid) {
		edid->size = 0;
	} else {
		edid->size = dc_edid->len;

		if (user_size < edid->size) {
			ret = -EFBIG;
			goto done;
		}

		if (copy_to_user(edid->data, dc_edid->buf, edid->size)) {
			ret = -EFAULT;
			goto done;
		}

	}

done:
	if (dc_edid)
		tegra_dc_put_edid(dc_edid);

	return ret;
}

static int set_event_mask(struct tegra_dc_ext_control_user *user, u32 mask)
{
	struct list_head *list, *tmp;

	if (mask & ~TEGRA_DC_EXT_EVENT_MASK_ALL)
		return -EINVAL;

	mutex_lock(&user->lock);

	user->event_mask = mask;

	list_for_each_safe(list, tmp, &user->event_list) {
		struct tegra_dc_ext_event_list *ev_list;
		ev_list = list_entry(list, struct tegra_dc_ext_event_list,
			list);
		if (!(mask & ev_list->event.type)) {
			list_del(list);
			kfree(ev_list);
		}
	}
	mutex_unlock(&user->lock);

	return 0;
}

static int get_capabilities(struct tegra_dc_ext_control_capabilities *caps)
{
	caps->caps = TEGRA_DC_EXT_CAPABILITIES_CURSOR_MODE |
			TEGRA_DC_EXT_CAPABILITIES_BLOCKLINEAR |
			TEGRA_DC_EXT_CAPABILITIES_CURSOR_TWO_COLOR;

	if (tegra_dc_is_nvdisplay())
		caps->caps |= TEGRA_DC_EXT_CAPABILITIES_NVDISPLAY;

	return 0;
}

static int tegra_dc_control_get_caps(struct tegra_dc_ext_caps *caps,
				u32 nr_elements)
{
	int i, ret = 0;

	for (i = 0; i < nr_elements; i++) {

		switch (caps[i].data_type) {
		case TEGRA_DC_EXT_CONTROL_CAP_TYPE_IMP:
		{
			struct tegra_dc_ext_imp_caps imp_caps;

			if (!tegra_dc_is_nvdisplay()) {
				pr_err("%s: IMP caps only valid for nvdisp\n",
					__func__);
				return -EPERM;
			}

			if (copy_from_user(&imp_caps,
				(void __user *)(uintptr_t)caps[i].data,
				sizeof(imp_caps))) {
				pr_err("%s: Can't copy IMP caps from user\n",
					__func__);
				return -EFAULT;
			}

			ret = tegra_nvdisp_get_imp_caps(&imp_caps);
			if (ret) {
				pr_err("%s: Can't get IMP caps\n", __func__);
				return ret;
			}

			if (copy_to_user((void __user *)(uintptr_t)
				caps[i].data, &imp_caps,
				sizeof(imp_caps))) {
				pr_err("%s: Can't copy IMP caps to user\n",
					__func__);
				return -EFAULT;
			}

			break;
		}
		default:
			return -EINVAL;
		}
	}

	return ret;
}

static long tegra_dc_ext_control_ioctl(struct file *filp, unsigned int cmd,
				       unsigned long arg)
{
	void __user *user_arg = (void __user *)arg;
	struct tegra_dc_ext_control_user *user = filp->private_data;

	switch (cmd) {
	case TEGRA_DC_EXT_CONTROL_GET_NUM_OUTPUTS:
	{
		u32 num = tegra_dc_ext_get_num_outputs();

		if (copy_to_user(user_arg, &num, sizeof(num)))
			return -EFAULT;

		return 0;
	}
	case TEGRA_DC_EXT_CONTROL_GET_OUTPUT_PROPERTIES:
	{
		struct tegra_dc_ext_control_output_properties args;
		int ret;

		if (copy_from_user(&args, user_arg, sizeof(args)))
			return -EFAULT;

		ret = get_output_properties(&args);

		if (copy_to_user(user_arg, &args, sizeof(args)))
			return -EFAULT;

		return ret;
	}
#ifdef CONFIG_COMPAT
	case TEGRA_DC_EXT_CONTROL_GET_OUTPUT_EDID32:
	{
		struct tegra_dc_ext_control_output_edid32 args;
		struct tegra_dc_ext_control_output_edid tmp;
		int ret;

		if (copy_from_user(&args, user_arg, sizeof(args)))
			return -EFAULT;

		/* translate 32-bit version to 64-bit */
		tmp.handle = args.handle;
		tmp.size = args.size;
		tmp.data = compat_ptr(args.data);

		ret = get_output_edid(&tmp);

		/* convert it back to 32-bit version, tmp.data not modified */
		args.handle = tmp.handle;
		args.size = tmp.size;

		if (copy_to_user(user_arg, &args, sizeof(args)))
			return -EFAULT;

		return ret;
	}
#endif
	case TEGRA_DC_EXT_CONTROL_GET_OUTPUT_EDID:
	{
		struct tegra_dc_ext_control_output_edid args;
		int ret;

		if (copy_from_user(&args, user_arg, sizeof(args)))
			return -EFAULT;

		ret = get_output_edid(&args);

		if (copy_to_user(user_arg, &args, sizeof(args)))
			return -EFAULT;

		return ret;
	}
	case TEGRA_DC_EXT_CONTROL_SET_EVENT_MASK:
		return set_event_mask(user, (u32) arg);
	case TEGRA_DC_EXT_CONTROL_GET_CAPABILITIES:
	{
		struct tegra_dc_ext_control_capabilities args = {0};
		int ret;

		ret = get_capabilities(&args);

		if (copy_to_user(user_arg, &args, sizeof(args)))
			return -EFAULT;

		return ret;
	}
	case TEGRA_DC_EXT_CONTROL_SCRNCAPT_PAUSE:
	/* TODO: Screen Capture support has been verified only for NVDisplay
	 *       with T18x. Dependency check on tegra_dc_is_nvdisplay() will
	 *       be kept until verification with older DC is made. Check on
	 *       the pause ioctl would be enough since other ioctl will be
	 *       rejected without the pause. */
#if defined(CONFIG_TEGRA_DC_SCREEN_CAPTURE)
	{
		struct tegra_dc_ext_control_scrncapt_pause  args;
		int ret;

		if (!tegra_dc_is_nvdisplay())
			return -EINVAL;

		if (copy_from_user(&args, user_arg, sizeof(args)))
			return -EFAULT;
		ret = tegra_dc_scrncapt_pause(user, &args);
		if (copy_to_user(user_arg, &args, sizeof(args)))
			return -EFAULT;
		return ret;
	}
#else
		return -EINVAL;
#endif
	case TEGRA_DC_EXT_CONTROL_SCRNCAPT_RESUME:
#if defined(CONFIG_TEGRA_DC_SCREEN_CAPTURE)
	{
		struct tegra_dc_ext_control_scrncapt_resume  args;
		int ret;

		if (copy_from_user(&args, user_arg, sizeof(args)))
			return -EFAULT;
		ret = tegra_dc_scrncapt_resume(user, &args);
		return ret;
	}
#else
		return -EINVAL;
#endif
	case TEGRA_DC_EXT_CONTROL_GET_FRAME_LOCK_PARAMS:
	{
		struct tegra_dc_ext_control_frm_lck_params args;
		int ret;

		ret = tegra_dc_common_get_frm_lock_params(&args);
		if (ret)
			return ret;

		if (copy_to_user(user_arg, &args, sizeof(args)))
			return -EFAULT;

		return 0;
	}
	case TEGRA_DC_EXT_CONTROL_SET_FRAME_LOCK_PARAMS:
	{
		struct tegra_dc_ext_control_frm_lck_params args;
		int ret;

		if (copy_from_user(&args, user_arg, sizeof(args)))
			return -EFAULT;

		ret = tegra_dc_common_set_frm_lock_params(&args);
		if (ret)
			return ret;

		return 0;
	}
	case TEGRA_DC_EXT_CONTROL_GET_CAP_INFO:
	{
		int ret = 0;
		struct tegra_dc_ext_caps *caps = NULL;
		u32 nr_elements = 0;

		ret = tegra_dc_ext_cpy_caps_from_user(user_arg, &caps,
							&nr_elements);
		if (ret)
			return ret;

		ret = tegra_dc_control_get_caps(caps, nr_elements);
		kfree(caps);

		return ret;
	}

	default:
		return -EINVAL;
	}
}

static int tegra_dc_ext_control_open(struct inode *inode, struct file *filp)
{
	struct tegra_dc_ext_control_user *user;
	struct tegra_dc_ext_control *control;

	user = kzalloc(sizeof(*user), GFP_KERNEL);
	if (!user)
		return -ENOMEM;

	control = container_of(inode->i_cdev, struct tegra_dc_ext_control,
		cdev);
	user->control = control;;

	INIT_LIST_HEAD(&user->event_list);
	mutex_init(&user->lock);

	filp->private_data = user;

	mutex_lock(&control->lock);
	list_add(&user->list, &control->users);
	mutex_unlock(&control->lock);

	return 0;
}

static int tegra_dc_ext_control_release(struct inode *inode, struct file *filp)
{
	struct tegra_dc_ext_control_user *user = filp->private_data;
	struct tegra_dc_ext_control *control = user->control;

	/* This will free any pending events for this user */
	set_event_mask(user, 0);

	mutex_lock(&control->lock);
	list_del(&user->list);
	mutex_unlock(&control->lock);

	kfree(user);

	return 0;
}

static const struct file_operations tegra_dc_ext_event_devops = {
	.owner =		THIS_MODULE,
	.open =			tegra_dc_ext_control_open,
	.release =		tegra_dc_ext_control_release,
	.read =			tegra_dc_ext_event_read,
	.poll =			tegra_dc_ext_event_poll,
	.unlocked_ioctl =	tegra_dc_ext_control_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl =		tegra_dc_ext_control_ioctl
#endif
};

int tegra_dc_ext_control_init(void)
{
	struct tegra_dc_ext_control *control = &g_control;
	int ret;

	cdev_init(&control->cdev, &tegra_dc_ext_event_devops);
	control->cdev.owner = THIS_MODULE;
	ret = cdev_add(&control->cdev, tegra_dc_ext_devno, 1);
	if (ret)
		return ret;

	control->dev = device_create(tegra_dc_ext_class,
	     NULL, tegra_dc_ext_devno, NULL, "tegra_dc_ctrl");
	if (IS_ERR(control->dev)) {
		ret = PTR_ERR(control->dev);
		cdev_del(&control->cdev);
	}

	mutex_init(&control->lock);

	INIT_LIST_HEAD(&control->users);

	return ret;
}

int tegra_dc_ext_process_bandwidth_renegotiate(int output,
						struct tegra_dc_bw_data *bw)
{
	return tegra_dc_ext_queue_bandwidth_renegotiate(&g_control, output, bw);
}
