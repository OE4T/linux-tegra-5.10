/**
 * @file drivers/media/platform/tegra/camera/fusa-capture/
 *	 capture-isp-channel-priv.h
 * @brief ISP channel character device driver private header for T186/T194
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

#ifndef __FUSA_CAPTURE_ISP_CHANNEL_PRIV_H__
#define __FUSA_CAPTURE_ISP_CHANNEL_PRIV_H__

#include <asm/ioctls.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/nvhost.h>

#include <media/fusa-capture/capture-isp.h>
#include <media/fusa-capture/capture-isp-channel.h>

/**
 * @todo This parameter is platform-dependent and should be retrieved from the
 *	 Device Tree.
 */
#define MAX_ISP_CHANNELS 64

/**
 * @brief ISP channel character device driver context
 */
struct isp_channel_drv {
	struct device *dev; /**< ISP kernel @em device. */
	u8 num_channels; /**< No. of ISP channel character devices. */
	struct mutex lock; /**< ISP channel driver context lock. */
	struct platform_device *ndev; /**< ISP kernel @em platform_device. */
	const struct isp_channel_drv_ops *ops;
		/**< ISP fops for Host1x syncpt/gos allocations. */
	struct tegra_isp_channel *channels[];
		/**< Allocated ISP channel contexts. */
};

/**
 * @defgroup ISP_CHANNEL_IOCTLS
 *
 * @brief ISP channel character device IOCTLs
 *
 * Clients in the UMD may open sysfs character devices representing ISP
 * channels, and perform configuration, and enqueue buffers in capture and
 * program requests to the low-level RCE subsystem via these IOCTLs.
 *
 * @{
 */

/**
 * Setup the channel context and synchronization primitives, pin memory for the
 * capture and program descriptor queues, set up the buffer management table,
 * initialize the capture/capture-control IVC channels and request ISP channel
 * allocation in RCE.
 *
 * @param[in]	ptr	Pointer to a struct @ref isp_capture_setup
 * @returns	0 (success), neg. errno (failure)
 */
#define ISP_CAPTURE_SETUP \
	_IOW('I', 1, struct isp_capture_setup)

/**
 * Release the ISP channel allocation in RCE, and all resources and contexts in
 * the KMD.
 *
 * @param[in]	rel	uint32_t bitmask of @ref CAPTURE_CHANNEL_RESET_FLAGS
 * @returns	0 (success), neg. errno (failure)
 */
#define ISP_CAPTURE_RELEASE \
	_IOW('I', 2, __u32)

/**
 * Reset the ISP channel in RCE synchronously w/ the KMD; all pending
 * capture/program descriptors in the queue are discarded and syncpoint values
 * fast-forwarded to unblock waiting clients.
 *
 * @param[in]	rst	uint32_t bitmask of @ref CAPTURE_CHANNEL_RESET_FLAGS
 * @returns	0 (success), neg. errno (failure)
 */
#define ISP_CAPTURE_RESET \
	_IOW('I', 3, __u32)

/**
 * Retrieve the ids and current values of the progress, stats progress
 * syncpoints, and ISP HW channel(s) allocated by RCE.
 *
 * If successful, the queried values are written back to the input struct.
 *
 * @param[in,out]	ptr	Pointer to a struct @ref isp_capture_info
 * @returns	0 (success), neg. errno (failure)
 */
#define ISP_CAPTURE_GET_INFO \
	_IOR('I', 4, struct isp_capture_info)

/**
 * Enqueue a capture request to RCE, input and prefences are allocated, and the
 * addresses to surface buffers in the descriptor (referenced by the
 * buffer_index) are pinned and patched.
 *
 * @param[in]	ptr	Pointer to a struct @ref isp_capture_req
 * @returns	0 (success), neg. errno (failure)
 */
#define ISP_CAPTURE_REQUEST \
	_IOW('I', 5, struct isp_capture_req)

/**
 * Wait on the next completion of an enqueued frame, signalled by RCE. The
 * status in the frame's capture descriptor is safe to read when this completes
 * w/o a -ETIMEDOUT or other error.
 *
 * @note This call completes for the frame at the head of the FIFO queue, and is
 *	 not necessarily for the most recently enqueued capture request.
 *
 * @param[in]	status	uint32_t timeout [ms], 0 for indefinite
 * @returns	0 (success), neg. errno (failure)
 */
#define ISP_CAPTURE_STATUS \
	_IOW('I', 6, __u32)

/**
 * Enqueue a program request to RCE, the addresses to the push buffer in the
 * descriptor (referenced by the buffer_index) are pinned and patched.
 *
 * @param[in]	ptr	Pointer to a struct @ref isp_program_req
 * @returns	0 (success), neg. errno (failure)
 */
#define ISP_CAPTURE_PROGRAM_REQUEST \
	_IOW('I', 7, struct isp_program_req)

/**
 * Wait on the next completion of an enqueued program, signalled by RCE. The
 * program execution is finished and is safe to free when this call completes.
 *
 * @note This call completes for the program at the head of the FIFO queue, and
 *	 is not necessarily for the most recently enqueued program request.

 * @returns	0 (success), neg. errno (failure)
 */
#define ISP_CAPTURE_PROGRAM_STATUS \
	_IOW('I', 8, __u32)

/**
 * Enqueue a joint capture and program request to RCE; this is equivalent to
 * calling @ref ISP_CAPTURE_PROGRAM_REQUEST and @ref ISP_CAPTURE_REQUEST
 * sequentially, but the number of KMD<->RCE IVC transmissions is reduced to one
 * in each direction for every frame.
 *
 * @param[in]	ptr	Pointer to a struct @ref isp_capture_req_ex
 * @returns	0 (success), neg. errno (failure)
 */
#define ISP_CAPTURE_REQUEST_EX \
	_IOW('I', 9, struct isp_capture_req_ex)

/**
 * Set up the combined capture and program progress status notifier array, which
 * is a replacement for the blocking @ref ISP_CAPTURE_STATUS and
 * @ref ISP_CAPTURE_PROGRAM_STATUS calls; allowing for out-of-order frame
 * completion notifications.
 *
 * The values are any of @ref CAPTURE_PROGRESS_NOTIFIER_STATES.
 *
 * @param[in]	ptr	Pointer to a struct @ref isp_capture_progress_status_req
 * @returns	0 (success), neg. errno (failure)
 */
#define ISP_CAPTURE_SET_PROGRESS_STATUS_NOTIFIER \
	_IOW('I', 10, struct isp_capture_progress_status_req)

/**
 * Perform an operation on the surface buffer by setting the bitwise \a flag
 * field with \ref CAPTURE_BUFFER_OPS flags.
 *
 * @param[in]	ptr	Pointer to a struct @ref isp_buffer_req.
 * @returns	0 (success), neg. errno (failure)
 */
#define ISP_CAPTURE_BUFFER_REQUEST \
	_IOW('I', 11, struct isp_buffer_req)

/** @} */

/**
 * @brief Initialize the ISP channel driver device (major).
 */
static int __init isp_channel_drv_init(void);

/**
 * @brief De-initialize the ISP channel driver device (major).
 */
static void __exit isp_channel_drv_exit(void);

/**
 * @brief Power on ISP and dependent camera subsystem hardware resources via
 *	  Host1x.
 *
 * The ISP channel is registered as an NvHost ISP client, and the reference
 * count is incremented by one.
 *
 * If the module reference count becomes 1, the camera subsystem is
 * unpowergated.
 *
 * @param[in]	chan	ISP channel context
 * @returns	0 (success), neg. errno (failure)
 */
static int isp_channel_power_on(
	struct tegra_isp_channel *chan);

/**
 * @brief Power off ISP and dependent camera subsystem hardware resources via
 *	  Host1x.
 *
 * The ISP channel is registered as an NvHost ISP client, and the module
 * reference count is decreased by one.
 *
 * If the module reference count becomes 0, the camera subsystem is powergated.
 *
 * @param[in]	chan	ISP channel context
 */
static void isp_channel_power_off(
	struct tegra_isp_channel *chan);

/**
 * @brief Open an ISP channel character device node, power on the camera
 *	  subsystem and initialize the channel driver context.
 *
 * The act of opening an ISP channel character device node does not entail the
 * reservation of an ISP channel, ISP_CAPTURE_SETUP must be called afterwards
 * to request an allocation by RCE.
 *
 * This is the @a open file operation handler for an ISP channel node.
 *
 * @param[in]	inode	ISP channel character device inode struct
 * @param[in]	file	ISP channel character device file struct
 * @returns	0 (success), neg. errno (failure)
 */
static int isp_channel_open(
	struct inode *inode,
	struct file *file);

/**
 * @brief Release an ISP channel character device node, power off the camera
 *	  subsystem and free the ISP channel driver context.
 *
 * Under normal operation, ISP_CAPTURE_RESET followed by ISP_CAPTURE_RELEASE
 * should be called before releasing the file handle on the device node.
 *
 * If the user-mode client crashes, the operating system will call this
 * @em release handler to perform both of those actions as part of the @em Abort
 * functionality.
 *
 * This is the @a release file operation handler for an ISP channel node.
 *
 * @param[in]	inode	ISP channel character device inode struct
 * @param[in]	file	ISP channel character device file struct
 * @returns	0
 */
static int isp_channel_release(
	struct inode *inode,
	struct file *file);

/**
 * @brief Process an IOCTL call on an ISP channel character device.
 *
 * Depending on the specific IOCTL, the argument (@a arg) may be a pointer to a
 * defined struct payload that is copied from or back to user-space. This memory
 * is allocated and mapped from user-space and must be kept available until
 * after the IOCTL call completes.
 *
 * This is the @a ioctl file operation handler for an ISP channel node.
 *
 * @param[in]		file	ISP channel character device file struct
 * @param[in]		cmd	ISP channel IOCTL command
 * @param[in,out]	arg	IOCTL argument; numerical value or pointer
 * @returns		0 (success), neg. errno (failure)
 */
static long isp_channel_ioctl(
	struct file *file,
	unsigned int cmd,
	unsigned long arg);

#endif /* __FUSA_CAPTURE_ISP_CHANNEL_PRIV_H__ */
