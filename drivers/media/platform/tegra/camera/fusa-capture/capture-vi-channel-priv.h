/**
 * @file drivers/media/platform/tegra/camera/fusa-capture/
 *       capture-vi-channel-priv.h
 * @brief VI channel character device driver private header for T186/T194
 *
 * Copyright (c) 2017-2019 NVIDIA Corporation.  All rights reserved.
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

#ifndef __FUSA_CAPTURE_VI_CHANNEL_PRIV_H__
#define __FUSA_CAPTURE_VI_CHANNEL_PRIV_H__

#include <asm/ioctls.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/nvhost.h>

#include <media/fusa-capture/capture-vi.h>
#include <media/fusa-capture/capture-vi-channel.h>

/**
 * @todo This parameter is platform-dependent and should be retrieved from the
 *	 Device Tree.
 */
#define MAX_VI_CHANNELS 64

/**
 * @brief VI channel character device driver context
 */
struct vi_channel_drv {
	struct device *dev; /**< VI kernel @em device. */
	struct platform_device *ndev; /**< VI kernel @em platform_device. */
	struct mutex lock; /**< VI channel driver context lock */
	u8 num_channels; /**< No. of VI channel character devices. */
	const struct vi_channel_drv_ops *ops;
		/**< VI fops for Host1x syncpt/gos allocations. */
	struct tegra_vi_channel __rcu *channels[];
		/**< Allocated VI channel contexts. */
};

/**
 * @defgroup VI_CHANNEL_IOCTLS
 *
 * @brief VI channel character device IOCTLs
 *
 * Clients in the UMD may open sysfs character devices representing VI channels,
 * and perform configuration and enqueue buffers in capture requests to the
 * low-level RCE subsystem via these IOCTLs.
 *
 * @{
 */

/**
 * Setup the channel context and synchronization primitives, pin memory for the
 * capture descriptor queue, set up the buffer management table, initialize the
 * capture/capture-control IVC channels and request VI channel allocation in
 * RCE.
 *
 * @param[in]	ptr	Pointer to a struct @ref vi_capture_setup
 * @returns	0 (success), neg. errno (failure)
 */
#define VI_CAPTURE_SETUP	_IOW('I', 1, struct vi_capture_setup)

/**
 * Release the VI channel allocation in RCE, and all resources and contexts in
 * the KMD.
 *
 * @param[in]	reset_flags	uint32_t bitmask of
 *				@ref CAPTURE_CHANNEL_RESET_FLAGS
 * @returns	0 (success), neg. errno (failure)
 */
#define VI_CAPTURE_RELEASE	_IOW('I', 2, __u32)

/**
 * Execute a blocking capture-control IVC request to RCE.
 *
 * @param[in]	ptr	Pointer to a struct  @ref vi_capture_control_msg
 * @returns	0 (success), neg. errno (failure)
 */
#define VI_CAPTURE_SET_CONFIG	_IOW('I', 3, struct vi_capture_control_msg)

/**
 * Reset the VI channel in RCE synchronously w/ the KMD; all pending capture
 * descriptors in the queue are discarded and syncpoint values fast-forwarded to
 * unblock waiting clients.
 *
 * @param[in]	reset_flags	uint32_t bitmask of
 *				@ref CAPTURE_CHANNEL_RESET_FLAGS
 * @returns	0 (success), neg. errno (failure)
 */
#define VI_CAPTURE_RESET	_IOW('I', 4, __u32)

/**
 * Retrieve the ids and current values of the progress, embedded data and
 * line timer syncpoints, and VI HW channel(s) allocated by RCE.
 *
 * If successful, the queried values are written back to the input struct.
 *
 * @param[in,out]	ptr	Pointer to a struct @ref vi_capture_info
 * @returns	0 (success), neg. errno (failure)
 */
#define VI_CAPTURE_GET_INFO	_IOR('I', 5, struct vi_capture_info)

/**
 * Enqueue a capture request to RCE, the addresses to surface buffers in the
 * descriptor (referenced by the buffer_index) are pinned and patched.
 *
 * The payload shall be a pointer to a struct @ref vi_capture_req.
 *
 * @param[in]	ptr	Pointer to a struct @ref vi_capture_compand
 * @returns	0 (success), neg. errno (failure)
 */
#define VI_CAPTURE_REQUEST	_IOW('I', 6, struct vi_capture_req)

/**
 * Wait on the next completion of an enqueued frame, signalled by RCE. The
 * status in the frame's capture descriptor is safe to read when this completes
 * w/o a -ETIMEDOUT or other error.
 *
 * @note This call completes for the frame at the head of the FIFO queue, and is
 *	 not necessarily for the most recently enqueued capture request.
 *
 * @param[in]	timeout_ms	uint32_t timeout [ms], 0 for indefinite
 * @returns	0 (success), neg. errno (failure)
 */
#define VI_CAPTURE_STATUS	_IOW('I', 7, __u32)

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
#define VI_CAPTURE_SET_COMPAND	_IOW('I', 8, struct vi_capture_compand)

/**
 * Set up the capture progress status notifier array, which is a replacement for
 * the blocking @ref VI_CAPTURE_STATUS call; allowing for out-of-order frame
 * completion notifications.
 *
 * The values are any of @ref CAPTURE_PROGRESS_NOTIFIER_STATES.
 *
 * @param[in]	ptr	Pointer to a struct @ref vi_capture_progress_status_req
 * @returns	0 (success), neg. errno (failure)
 */
#define VI_CAPTURE_SET_PROGRESS_STATUS_NOTIFIER \
	_IOW('I', 9, struct vi_capture_progress_status_req)

/**
 * Perform an operation on the surface buffer by setting the bitwise \a flag
 * field with \ref CAPTURE_BUFFER_OPS flags.
 *
 * @param[in]	ptr	Pointer to a struct @ref vi_buffer_req
 * @returns	0 (success), neg. errno (failure)
 */
#define VI_CAPTURE_BUFFER_REQUEST \
	_IOW('I', 10, struct vi_buffer_req)

/** @} */

/**
 * @brief Initialize the VI channel driver device (major).
 */
static int __init vi_channel_drv_init(void);

/**
 * @brief De-initialize the VI channel driver device (major).
 */
static void __exit vi_channel_drv_exit(void);

/**
 * @brief Power on VI and dependent camera subsystem hardware resources via
 *	  Host1x.
 *
 * The VI channel is registered as an NvHost VI client, and the reference count
 * is incremented by one.
 *
 * If the module reference count becomes 1, the camera subsystem is
 * unpowergated.
 *
 * @param[in]	chan	ISP channel context
 * @returns	0 (success), neg. errno (failure)
 */
static int vi_channel_power_on_vi_device(
	struct tegra_vi_channel *chan);

/**
 * @brief Power off VI and dependent camera subsystem hardware resources via
 *	  Host1x.
 *
 * The VI channel is registered as an NvHost VI client, and the module reference
 * count is decreased by one.
 *
 * If the module reference count becomes 0, the camera subsystem is powergated.
 *
 * @param[in]	chan	VI channel context
 */
static void vi_channel_power_off_vi_device(
	struct tegra_vi_channel *chan);

/**
 * @brief Open a VI channel character device node; pass parameters to
 *	  @ref vi_channel_open_ex subroutine to complete initialization.
 *
 * This is the @a open file operation handler for a VI channel node.
 *
 * @param[in]	inode	VI channel character device inode struct
 * @param[in]	file	VI channel character device file struct
 * @returns	0 (success), neg. errno (failure)
 */
static int vi_channel_open(
	struct inode *inode,
	struct file *file);

/**
 * @brief Release a VI channel character device node; pass parameters to
 *	  @ref vi_channel_close_ex subroutine to complete release.
 *
 * This is the @a release file operation handler for a VI channel node.
 *
 * @param[in]	inode	VI channel character device inode struct
 * @param[in]	file	VI channel character device file struct
 * @returns	0
 */
static int vi_channel_release(
	struct inode *inode,
	struct file *file);

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
 * @returns		0 (success), neg. errno (failure)
 */
static long vi_channel_ioctl(
	struct file *file,
	unsigned int cmd,
	unsigned long arg);

#endif /* __FUSA_CAPTURE_VI_CHANNEL_PRIV_H__ */
