/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include <uapi/misc/nvscic2c-ioctl.h>

#include "utils.h"

/*
 * C2C channels are known internal.
 * main ctx sees C2C channels in form of opaque.
 * This is opaque handle for C2C channel.
 * Please note this typedef is purely internal
 * to module.
 */
typedef void *channel_hdl;

/*
 * Masked offsets to return to user, allowing them to mmap
 * different memory segments of channel in user-space.
 */
enum mem_mmap_type {
	/* Invalid.*/
	MEM_MMAP_INVALID = 0,
	/* Map Peer PCIe aperture: For Tx across PCIe.*/
	PEER_MEM_MMAP,
	/* Map Self PCIe shared memory: For Rx across PCIe.*/
	SELF_MEM_MMAP,
	/* Map Self memory(not exposed via PCIe).*/
	CTRL_MEM_MMAP,
	/* Map Link memory segment to query link status with Peer.*/
	LINK_MEM_MMAP,
	/* Maximum. */
	MEM_MAX_MMAP,
};

/**
 * C2C channel internal parameters.
 * These are available in DT,
 * parsed by main ctx and supplied through init call.
 */
struct channel_param {
	/*Will be used as minor number.*/
	int number;
	char name[MAX_NAME_SZ];

	/*Channel memory is aligned with this.*/
	uint32_t align;

	/*Notification type: (Doorbell/Syncpoint)*/
	enum event_type event;

	/*ivc(NvSciIpc) channel name in nvsciipc.cfg*/
	char cfg_name[MAX_NAME_SZ];
	/* PCIe apt and shared memory fragmented
	 * into number of nframes with each frame
	 * of size frame_size.
	 */
	uint32_t nframes;
	uint32_t frame_size;

	/*Channel type: Cpu/Bulk (Producer/Consumer)*/
	enum xfer_type xfer_type;
	/*enabled if local edma engine is in use.*/
	bool edma_enabled;

	/*offset of PCIe aprt and shared mem for channel*/
	struct pci_mmio peer_mem;
	struct cpu_buff self_mem;

	/*main ctx, not in use right now.*/
	void *c2c_ctx;
};

/**
 * channel_deinit
 * In shutdown we need to release all channel resources.
 * This api deinit's channel resources.
 *
 * hdl  [in] : c2c channel handle.
 * pdev [in] : module device node.
 *
 * Return   : 0 (success) errno (failure)
 */
int channel_deinit(channel_hdl hdl, struct device *pdev);

/**
 * channel_init
 * For each ivc there is one c2c channel.
 * To create c2c channel main ctx had to provide few init parameters.
 * This api is to initialize channel place holder using init param.
 *
 * hdl [out] : place holder handle for c2c channel.
 * init [in] : pointer to channel init params.
 * pdev [in] : module device node.
 *
 * Return    : 0 (success) errno (failure)
 */
int channel_init(channel_hdl *hdl, struct channel_param *init, struct device *pdev);

/**
 * channel_remove_device
 * Remove C2C channel device node.
 *
 * hdl [out]     : place holder handle for c2c channel.
 * c2c_dev [in]  : holds C2C chrdev region.
 * c2c_class[in] : C2C class under which each device node is created.
 *
 * Return    : 0 (success) errno (failure)
 */
int channel_remove_device(channel_hdl hdl,
			dev_t c2c_dev, struct class *c2c_class);

/**
 * channel_add_device
 * Device node is created for different valid C2C channels.
 * We have used delayed device creation model,
 * In which we first make sure that place holders are created
 * for each valid C2C channel.
 * This api is used to create device nodes post
 * initialization of channel place holders.
 *
 * hdl [out]     : place holder handle for c2c channel.
 * c2c_dev [in]  : holds C2C chrdev region.
 * c2c_class[in] : C2C class under which each device node is created.
 *
 * Return    : 0 (success) errno (failure)
 */
int channel_add_device(channel_hdl hdl,
			dev_t c2c_dev, struct class *c2c_class);

/**
 * channel_handle_server_msg
 * channel layer exposes different channel devices.
 * In case any global event like SDL event
 * received each user should be notified.
 * This api is called by main context
 * to notify channel user.
 *
 * hdl [out] : place holder handle for c2c channel.
 *
 * Return    : 0 (success) errno (failure)
 */
int channel_handle_server_msg(channel_hdl hdl);
#endif /*__CHANNEL_H__*/
