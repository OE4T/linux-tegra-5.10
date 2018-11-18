/*
 * Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
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

#ifndef __CDEV_H__
#define __CDEV_H__

#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/tegra-ivc.h>

#include "dt.h"
#include "utils.h"
#include "channel.h"

/* Platform private driver data. */
struct c2c {
	/* device - self. */
	struct device *dev;

	/* parameters of all the valid channels parsed from DT. */
	struct dt_param dt_param;

	/* place-holder for all C2C channel contexts. */
	channel_hdl *channels;
	/*All supported channel place holder map.
	 * Will be used to notify user for SDL events.
	 */
	channel_hdl *db_channel_map[MAX_CHANNELS];

	/* Total PCIe aperture for inter-connect xfer, tx area. */
	struct pci_mmio peer_mem;

	/* Total PCIe shared mem for inter-connect xfer, rx area. */
	struct cpu_buff self_mem;

	/*nvscic2c drivers class info for device model.*/
	dev_t c2c_dev;
	struct class *c2c_class;

	/* IVM is used as PCIe shared memory.
	 * IVM id is supplied via DT.
	 * we use tegra_hv calls to access IVM memory details.
	 */
	struct tegra_hv_ivm_cookie *ivmk;
};
#endif /* __CDEV_H__ */
