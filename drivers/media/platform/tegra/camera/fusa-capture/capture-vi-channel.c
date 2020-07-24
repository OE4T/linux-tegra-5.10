/*
 * Copyright (c) 2017-2020 NVIDIA Corporation.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

/**
 * @file drivers/media/platform/tegra/camera/fusa-capture/capture-vi-channel.c
 *
 * @brief VI channel character device driver for the T186/T194 Camera RTCPU
 * platform.
 */

#include <asm/ioctls.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/of_platform.h>
#include <linux/module.h>
#include <linux/nvhost.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "nvhost_acm.h"
#include <media/fusa-capture/capture-vi.h>

#include <media/fusa-capture/capture-vi-channel.h>

/**
 * @brief VI channel character device driver context.
 */
struct vi_channel_drv {
	struct platform_device *vi_capture_pdev;
		/**< Capture VI driver platform device */
	bool use_legacy_path;
		/**< Flag to maintain backward-compatibility for T186 */
	struct device *dev; /**< VI kernel @em device */
	struct platform_device *ndev; /**< VI kernel @em platform_device */
	struct mutex lock; /**< VI channel driver context lock */
	u8 num_channels; /**< No. of VI channel character devices */
	const struct vi_channel_drv_ops *ops;
		/**< VI fops for Host1x syncpt/gos allocations */
	struct tegra_vi_channel __rcu *channels[];
		/**< Allocated VI channel contexts */
};

/**
 * @defgroup VI_CHANNEL_IOCTLS
 *
 * @brief VI channel character device IOCTL API
 *
 * Clients in the UMD may open sysfs character devices representing VI channels,
 * and perform configuration and enqueue buffers in capture requests to the
 * low-level RCE subsystem via these IOCTLs.
 *
 * @{
 */

/**
 * @brief Set up ISP channel resources and request FW channel allocation in RCE.
 *
 * Initialize the VI channel context and synchronization primitives, pin memory
 * for the capture descriptor queue, set up the buffer management table,
 * initialize the capture/capture-control IVC channels and request VI FW channel
 * allocation in RCE.
 *
 * @param[in]	ptr	Pointer to a struct @ref vi_capture_setup
 *
 * @returns	0 (success), neg. errno (failure)
 */
#define VI_CAPTURE_SETUP \
	_IOW('I', 1, struct vi_capture_setup)

/**
 * @brief Release the VI FW channel allocation in RCE, and all resources and
 * contexts in the KMD.
 *
 * @param[in]	reset_flags	uint32_t bitmask of
 *				@ref CAPTURE_CHANNEL_RESET_FLAGS

 * @returns	0 (success), neg. errno (failure)
 */
#define VI_CAPTURE_RELEASE \
	_IOW('I', 2, __u32)

/**
 * @brief Execute a blocking capture-control IVC request to RCE.
 *
 * @param[in]	ptr	Pointer to a struct  @ref vi_capture_control_msg
 *
 * @returns	0 (success), neg. errno (failure)
 */
#define VI_CAPTURE_SET_CONFIG \
	_IOW('I', 3, struct vi_capture_control_msg)

/**
 * @brief Reset the VI channel in RCE synchronously w/ the KMD; all pending
 * capture descriptors in the queue are discarded and syncpoint values
 * fast-forwarded to unblock waiting clients.
 *
 * @param[in]	reset_flags	uint32_t bitmask of
 *				@ref CAPTURE_CHANNEL_RESET_FLAGS

 * @returns	0 (success), neg. errno (failure)
 */
#define VI_CAPTURE_RESET \
	_IOW('I', 4, __u32)

/**
 * @brief Retrieve the ids and current values of the progress, embedded data and
 * line timer syncpoints, and VI HW channel(s) allocated by RCE.
 *
 * If successful, the queried values are written back to the input struct.
 *
 * @param[in,out]	ptr	Pointer to a struct @ref vi_capture_info
 *
 * @returns	0 (success), neg. errno (failure)
 */
#define VI_CAPTURE_GET_INFO \
	_IOR('I', 5, struct vi_capture_info)

/**
 * @brief Enqueue a capture request to RCE, the addresses to surface buffers in
 * the descriptor (referenced by the buffer_index) are pinned and patched.
 *
 * The payload shall be a pointer to a struct @ref vi_capture_req.
 *
 * @param[in]	ptr	Pointer to a struct @ref vi_capture_compand
 *
 * @returns	0 (success), neg. errno (failure)
 */
#define VI_CAPTURE_REQUEST \
	_IOW('I', 6, struct vi_capture_req)

/**
 * Wait on the next completion of an enqueued frame, signalled by RCE. The
 * status in the frame's capture descriptor is safe to read when this completes
 * w/o a -ETIMEDOUT or other error.
 *
 * @note This call completes for the frame at the head of the FIFO queue, and is
 * not necessarily for the most recently enqueued capture request.
 *
 * @param[in]	timeout_ms	uint32_t timeout [ms], 0 for indefinite
 *
 * @returns	0 (success), neg. errno (failure)
 */
#define VI_CAPTURE_STATUS \
	_IOW('I', 7, __u32)

/**
 * Set global VI pixel companding config, this applies to all VI channels in
 * which this is functionality enabled.
 *
 * @note Pixel companding must be explicitly enabled in each channel by setting
 *	 the compand_enable bit in the @ref vi_channel_config for every capture
 *	 descriptor.
 *
 * @param[in]	ptr	Pointer to a struct @ref vi_capture_compand
 * @returns	0 (success), neg. errno (failure)
 */
#define VI_CAPTURE_SET_COMPAND \
	_IOW('I', 8, struct vi_capture_compand)

/**
 * @brief Set up the capture progress status notifier array, which is a
 * replacement for the blocking @ref VI_CAPTURE_STATUS call; allowing for
 * out-of-order frame completion notifications.
 *
 * The values written by the KMD are any of the
 * @ref CAPTURE_PROGRESS_NOTIFIER_STATES.
 *
 * @param[in]	ptr	Pointer to a struct @ref vi_capture_progress_status_req
 *
 * @returns	0 (success), neg. errno (failure)
 */
#define VI_CAPTURE_SET_PROGRESS_STATUS_NOTIFIER \
	_IOW('I', 9, struct vi_capture_progress_status_req)

/**
 * @brief Perform an operation on the surface buffer by setting the bitwise
 * @a flag field with @ref CAPTURE_BUFFER_OPS flags.
 *
 * @param[in]	ptr	Pointer to a struct @ref vi_buffer_req
 * @returns	0 (success), neg. errno (failure)
 */
#define VI_CAPTURE_BUFFER_REQUEST \
	_IOW('I', 10, struct vi_buffer_req)

/** @} */

void vi_capture_request_unpin(
	struct tegra_vi_channel *chan,
	uint32_t buffer_index)
{
	struct vi_capture *capture = chan->capture_data;
	struct capture_common_unpins *unpins;
	int i = 0;

	mutex_lock(&capture->unpins_list_lock);
	unpins = capture->unpins_list[buffer_index];
	if (unpins != NULL) {
		for (i = 0; i < unpins->num_unpins; i++)
			put_mapping(capture->buf_ctx, unpins->data[i]);
		capture->unpins_list[buffer_index] = NULL;
		kfree(unpins);
	}
	mutex_unlock(&capture->unpins_list_lock);
}
EXPORT_SYMBOL(vi_capture_request_unpin);

static struct vi_channel_drv *chdrv_;
static DEFINE_MUTEX(chdrv_lock);

struct tegra_vi_channel *vi_channel_open_ex(
	unsigned int channel,
	bool is_mem_pinned)
{
	struct tegra_vi_channel *chan;
	struct vi_channel_drv *chan_drv;
	int err;

	if (mutex_lock_interruptible(&chdrv_lock))
		return ERR_PTR(-ERESTARTSYS);

	chan_drv = chdrv_;

	if (chan_drv == NULL || channel >= chan_drv->num_channels) {
		mutex_unlock(&chdrv_lock);
		return ERR_PTR(-ENODEV);
	}
	mutex_unlock(&chdrv_lock);

	chan = kzalloc(sizeof(*chan), GFP_KERNEL);
	if (unlikely(chan == NULL))
		return ERR_PTR(-ENOMEM);

	chan->drv = chan_drv;
	if (chan_drv->use_legacy_path) {
		chan->dev = chan_drv->dev;
		chan->ndev = chan_drv->ndev;
	} else {
		chan->vi_capture_pdev = chan_drv->vi_capture_pdev;
	}
	chan->ops = chan_drv->ops;

	err = vi_capture_init(chan, is_mem_pinned);
	if (err < 0)
		goto error;

	mutex_lock(&chan_drv->lock);
	if (rcu_access_pointer(chan_drv->channels[channel]) != NULL) {
		mutex_unlock(&chan_drv->lock);
		err = -EBUSY;
		goto rcu_err;
	}

	rcu_assign_pointer(chan_drv->channels[channel], chan);
	mutex_unlock(&chan_drv->lock);

	return chan;

rcu_err:
	vi_capture_shutdown(chan);
error:
	kfree(chan);
	return ERR_PTR(err);
}
EXPORT_SYMBOL(vi_channel_open_ex);

int vi_channel_close_ex(
	unsigned int channel,
	struct tegra_vi_channel *chan)
{
	struct vi_channel_drv *chan_drv = chan->drv;

	vi_capture_shutdown(chan);

	mutex_lock(&chan_drv->lock);

	WARN_ON(rcu_access_pointer(chan_drv->channels[channel]) != chan);
	RCU_INIT_POINTER(chan_drv->channels[channel], NULL);

	mutex_unlock(&chan_drv->lock);
	kfree_rcu(chan, rcu);

	return 0;
}
EXPORT_SYMBOL(vi_channel_close_ex);

/**
 * @brief Open a VI channel character device node; pass parameters to
 * @ref vi_channel_open_ex subroutine to complete initialization.
 *
 * This is the @a open file operation handler for a VI channel node.
 *
 * @param[in]	inode	VI channel character device inode struct
 * @param[in]	file	VI channel character device file struct
 *
 * @returns	0 (success), neg. errno (failure)
 */
static int vi_channel_open(
	struct inode *inode,
	struct file *file)
{
	unsigned int channel = iminor(inode);
	struct tegra_vi_channel *chan;

	chan = vi_channel_open_ex(channel, true);
	if (IS_ERR(chan))
		return PTR_ERR(chan);

	file->private_data = chan;

	return nonseekable_open(inode, file);
}

/**
 * @brief Release a VI channel character device node; pass parameters to
 * @ref vi_channel_close_ex subroutine to complete release.
 *
 * This is the @a release file operation handler for a VI channel node.
 *
 * @param[in]	inode	VI channel character device inode struct
 * @param[in]	file	VI channel character device file struct
 *
 * @returns	0
 */
static int vi_channel_release(
	struct inode *inode,
	struct file *file)
{
	struct tegra_vi_channel *chan = file->private_data;
	unsigned int channel = iminor(inode);

	vi_channel_close_ex(channel, chan);

	return 0;
}

/**
 * @brief Process an IOCTL call on a VI channel character device.
 *
 * Depending on the specific IOCTL, the argument (@a arg) may be a pointer to a
 * defined struct payload that is copied from or back to user-space. This memory
 * is allocated and mapped from user-space and must be kept available until
 * after the IOCTL call completes.
 *
 * This is the @a ioctl file operation handler for a VI channel node.
 *
 * @param[in]		file	VI channel character device file struct
 * @param[in]		cmd	VI channel IOCTL command
 * @param[in,out]	arg	IOCTL argument; numerical value or pointer
 *
 * @returns		0 (success), neg. errno (failure)
 */
static long vi_channel_ioctl(
	struct file *file,
	unsigned int cmd,
	unsigned long arg)
{
	struct tegra_vi_channel *chan = file->private_data;
	struct vi_capture *capture = chan->capture_data;
	void __user *ptr = (void __user *)arg;
	int err = -EFAULT;

	switch (_IOC_NR(cmd)) {
	case _IOC_NR(VI_CAPTURE_SETUP): {
		struct vi_capture_setup setup;

		if (copy_from_user(&setup, ptr, sizeof(setup)))
			break;

		if (chan->drv->use_legacy_path == false) {
			vi_get_nvhost_device(chan, &setup);
			if (chan->dev == NULL) {
				dev_err(&chan->vi_capture_pdev->dev,
					"%s: channel device is NULL",
					__func__);
				return -EINVAL;
			}
		}

		capture->buf_ctx = create_buffer_table(chan->dev);
		if (capture->buf_ctx == NULL) {
			dev_err(chan->dev, "vi buffer setup failed");
			break;
		}

		/* pin the capture descriptor ring buffer */
		err = capture_common_pin_memory(capture->rtcpu_dev,
				setup.mem, &capture->requests);
		if (err < 0) {
			dev_err(chan->dev,
				"%s: memory setup failed\n", __func__);
			destroy_buffer_table(capture->buf_ctx);
			return -EFAULT;
		}

		/* allocate for unpin list based on queue depth */
		capture->unpins_list = kcalloc(setup.queue_depth,
				sizeof(struct capture_common_unpins *),
				GFP_KERNEL);
		if (unlikely(capture->unpins_list == NULL)) {
			dev_err(chan->dev, "failed to allocate unpins array\n");
			capture_common_unpin_memory(&capture->requests);
			destroy_buffer_table(capture->buf_ctx);
			return -ENOMEM;
		}

		setup.iova = capture->requests.iova;
		err = vi_capture_setup(chan, &setup);
		if (err < 0) {
			dev_err(chan->dev, "vi capture setup failed\n");
			kfree(capture->unpins_list);
			capture_common_unpin_memory(&capture->requests);
			destroy_buffer_table(capture->buf_ctx);
			return err;
		}
		break;
	}

	case _IOC_NR(VI_CAPTURE_RESET): {
		uint32_t reset_flags;
		int i;

		if (copy_from_user(&reset_flags, ptr, sizeof(reset_flags)))
			break;

		err = vi_capture_reset(chan, reset_flags);
		if (err < 0)
			dev_err(chan->dev, "vi capture reset failed\n");
		else {
			for (i = 0; i < capture->queue_depth; i++)
				vi_capture_request_unpin(chan, i);
		}
		break;
	}

	case _IOC_NR(VI_CAPTURE_RELEASE): {
		uint32_t reset_flags;
		int i;

		if (copy_from_user(&reset_flags, ptr, sizeof(reset_flags)))
			break;

		err = vi_capture_release(chan, reset_flags);
		if (err < 0)
			dev_err(chan->dev, "vi capture release failed\n");
		else {
			for (i = 0; i < capture->queue_depth; i++)
				vi_capture_request_unpin(chan, i);
			capture_common_unpin_memory(&capture->requests);
			kfree(capture->unpins_list);
			destroy_buffer_table(capture->buf_ctx);
		}
		break;
	}

	case _IOC_NR(VI_CAPTURE_GET_INFO): {
		struct vi_capture_info info;

		err = vi_capture_get_info(chan, &info);
		if (err < 0)
			dev_err(chan->dev, "vi capture get info failed\n");
		if (copy_to_user(ptr, &info, sizeof(info)))
			err = -EFAULT;
		break;
	}

	case _IOC_NR(VI_CAPTURE_SET_CONFIG): {
		struct vi_capture_control_msg msg;

		if (copy_from_user(&msg, ptr, sizeof(msg)))
			break;
		err = vi_capture_control_message(chan, &msg);
		if (err < 0)
			dev_err(chan->dev, "vi capture set config failed\n");
		break;
	}

	case _IOC_NR(VI_CAPTURE_REQUEST): {
		struct vi_capture_req req;
		struct capture_common_pin_req cap_common_req;

		if (copy_from_user(&req, ptr, sizeof(req)))
			break;

		if (req.num_relocs == 0) {
			dev_err(chan->dev, "request must have non-zero relocs\n");
			return -EINVAL;
		}

		/* pin and reloc */
		memset(&cap_common_req, 0, sizeof(cap_common_req));
		cap_common_req.table = capture->buf_ctx;
		cap_common_req.dev = chan->dev;
		cap_common_req.rtcpu_dev = capture->rtcpu_dev;
		cap_common_req.unpins = NULL;
		cap_common_req.requests = &capture->requests;
		cap_common_req.request_size = capture->request_size;
		cap_common_req.request_offset = req.buffer_index *
				capture->request_size;
		cap_common_req.num_relocs = req.num_relocs;
		cap_common_req.reloc_user = (uint32_t __user *)
				(uintptr_t)req.reloc_relatives;

		err = capture_common_request_pin_and_reloc(&cap_common_req);
		if (err < 0) {
			dev_err(chan->dev, "request relocation failed\n");
			return err;
		}

		/* assign the unpins list to the capture to be unpinned and */
		/* freed at capture completion (vi_capture_request_unpin) */
		mutex_lock(&capture->unpins_list_lock);
		capture->unpins_list[req.buffer_index] = cap_common_req.unpins;
		mutex_unlock(&capture->unpins_list_lock);

		err = vi_capture_request(chan, &req);
		if (err < 0) {
			dev_err(chan->dev,
				"vi capture request submit failed\n");
			vi_capture_request_unpin(chan, req.buffer_index);
		}
		break;
	}

	case _IOC_NR(VI_CAPTURE_STATUS): {
		uint32_t timeout_ms;

		if (copy_from_user(&timeout_ms, ptr, sizeof(timeout_ms)))
			break;
		err = vi_capture_status(chan, timeout_ms);
		if (err < 0)
			dev_err(chan->dev,
				"vi capture get status failed\n");
		break;
	}

	case _IOC_NR(VI_CAPTURE_SET_COMPAND): {
		struct vi_capture_compand compand;

		if (copy_from_user(&compand, ptr, sizeof(compand)))
			break;
		err = vi_capture_set_compand(chan, &compand);
		if (err < 0)
			dev_err(chan->dev,
				"setting compand failed\n");
		break;
	}

	case _IOC_NR(VI_CAPTURE_SET_PROGRESS_STATUS_NOTIFIER): {
		struct vi_capture_progress_status_req req;

		if (copy_from_user(&req, ptr, sizeof(req)))
			break;
		err = vi_capture_set_progress_status_notifier(chan, &req);
		if (err < 0)
			dev_err(chan->dev,
					"setting progress status buffer failed\n");
		break;
	}

	case _IOC_NR(VI_CAPTURE_BUFFER_REQUEST): {
		struct vi_buffer_req req;

		if (copy_from_user(&req, ptr, sizeof(req)) != 0U)
			break;

		err = capture_buffer_request(
			capture->buf_ctx, req.mem, req.flag);
		if (err < 0)
			dev_err(chan->dev, "vi buffer request failed\n");
		break;
	}

	default: {
		dev_err(chan->dev, "%s:Unknown ioctl\n", __func__);
		return -ENOIOCTLCMD;
	}
	}

	return err;
}

static const struct file_operations vi_channel_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.unlocked_ioctl = vi_channel_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = vi_channel_ioctl,
#endif
	.open = vi_channel_open,
	.release = vi_channel_release,
};

/* Character device */
static struct class *vi_channel_class;
static int vi_channel_major;

int vi_channel_drv_register(
	struct platform_device *ndev,
	unsigned int max_vi_channels)
{
	struct vi_channel_drv *chan_drv;
	int err = 0;
	unsigned int i;

	chan_drv = devm_kzalloc(&ndev->dev, sizeof(*chan_drv) +
			max_vi_channels * sizeof(struct tegra_vi_channel *),
			GFP_KERNEL);
	if (unlikely(chan_drv == NULL))
		return -ENOMEM;

	if (strstr(ndev->name, "tegra-capture-vi") == NULL) {
		chan_drv->use_legacy_path = true;
		chan_drv->dev = &ndev->dev;
		chan_drv->ndev = ndev;
	} else {
		chan_drv->use_legacy_path = false;
		chan_drv->dev = NULL;
		chan_drv->ndev = NULL;
		chan_drv->vi_capture_pdev = ndev;
	}
	chan_drv->num_channels = max_vi_channels;
	mutex_init(&chan_drv->lock);

	mutex_lock(&chdrv_lock);
	if (chdrv_ != NULL) {
		mutex_unlock(&chdrv_lock);
		WARN_ON(1);
		err = -EBUSY;
		goto error;
	}
	chdrv_ = chan_drv;
	mutex_unlock(&chdrv_lock);

	for (i = 0; i < chan_drv->num_channels; i++) {
		dev_t devt = MKDEV(vi_channel_major, i);

		struct device *dev =
			(chan_drv->use_legacy_path)?chan_drv->dev :
			&chan_drv->vi_capture_pdev->dev;
		device_create(vi_channel_class, dev, devt, NULL,
				"capture-vi-channel%u", i);
	}

	return 0;

error:
	return err;
}
EXPORT_SYMBOL(vi_channel_drv_register);

int vi_channel_drv_fops_register(
	const struct vi_channel_drv_ops *ops)
{
	int err = 0;
	struct vi_channel_drv *chan_drv;

	chan_drv = chdrv_;
	if (chan_drv == NULL) {
		err = -EPROBE_DEFER;
		goto error;
	}

	mutex_lock(&chdrv_lock);
	if (chan_drv->ops == NULL)
		chan_drv->ops = ops;
	else
		dev_warn(chan_drv->dev, "fops function table already registered\n");
	mutex_unlock(&chdrv_lock);

	return 0;

error:
	return err;
}
EXPORT_SYMBOL(vi_channel_drv_fops_register);

void vi_channel_drv_unregister(
	struct device *dev)
{
	struct vi_channel_drv *chan_drv;
	unsigned int i;

	mutex_lock(&chdrv_lock);
	chan_drv = chdrv_;
	chdrv_ = NULL;
	WARN_ON(chan_drv->dev != dev);
	mutex_unlock(&chdrv_lock);

	for (i = 0; i < chan_drv->num_channels; i++) {
		dev_t devt = MKDEV(vi_channel_major, i);

		device_destroy(vi_channel_class, devt);
	}

	devm_kfree(chan_drv->dev, chan_drv);
}
EXPORT_SYMBOL(vi_channel_drv_unregister);

/**
 * @brief Initialize the VI channel driver device (major).
 *
 * @returns	0 (success), PTR_ERR or neg. VI channel major no. (failure)
 */
static int __init vi_channel_drv_init(void)
{
	vi_channel_class = class_create(THIS_MODULE, "capture-vi-channel");
	if (IS_ERR(vi_channel_class))
		return PTR_ERR(vi_channel_class);

	vi_channel_major = register_chrdev(0, "capture-vi-channel",
						&vi_channel_fops);
	if (vi_channel_major < 0) {
		class_destroy(vi_channel_class);
		return vi_channel_major;
	}

	return 0;
}

/**
 * @brief De-initialize the VI channel driver device (major).
 */
static void __exit vi_channel_drv_exit(void)
{
	unregister_chrdev(vi_channel_major, "capture-vi-channel");
	class_destroy(vi_channel_class);
}

subsys_initcall(vi_channel_drv_init);
module_exit(vi_channel_drv_exit);
