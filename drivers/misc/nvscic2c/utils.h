/*
 * Copyright (c) 2019 - 2020, NVIDIA CORPORATION. All rights reserved.
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

#ifndef __UTILS_H__
#define __UTILS_H__

#include <linux/mm.h>
#include <linux/types.h>
#include <linux/tegra-ivc.h>
#include <uapi/misc/nvscic2c-ioctl.h>

#define MODULE_NAME      "nvscic2c"

/* Maximum c2c channels supported. Just setting a limit. */
#define MAX_CHANNELS           (16)

/* Maximum length of any string used - channel name, DT node name, etc. */
#define MAX_NAME_SZ            (32)

/* Memory Mapped Memory for PCI BAR regions. */
struct pci_mmio {
	/* BAR aperture.*/
	phys_addr_t     aper;

	/* IPA/PA for the BAR aperture.*/
	void __iomem    *pva;

	/* size of the BAR aperture.*/
	resource_size_t size;
};

/* CPU-only accessible memory which is not PCIe aper or PCIe
 * shared memory. Typically will contain information of memory
 * allocated via kalloc()/kzalloc*().
 */
struct cpu_buff {
	/* cpu address(va). */
	void *pva;

	/* physical address. */
	uint64_t phys_addr;

	/* size of the memory allocated. */
	size_t size;
};

/**
 * Different type of EVENTS.
 * These events are used to notify application running
 * on remote SOC.
 */
enum event_type {
	INVALID_EVENT = 0,
	DOORBELL,
	SYNCPOINT,
	MAX_EVENT
};

/**
 * PCIe aperture is static for NTB device.
 * This is passed via dt file.
 * We parse this and store in this DS format.
 * We are using present bool variable to check if apt
 * was present in DT node.
 * Please note we have one bool field explicitly to check
 * if entry was present in DT or not, as checking size for this
 * is not appropriate.
 */
struct c2c_static_apt {
	bool     present;
	uint64_t base;
	uint32_t size;
};

/**
 * C2C channel parameters as parsed from DT file. These are populated/set
 * by the values as read from the nvscic2c DT node in DT files.
 */
struct channel_dt_param {
	/* channel name. */
	char ch_name[MAX_NAME_SZ];

	/* align channel total tx and rx memory to this value.
	 * Assuming it to be a power of 2.
	 */
	uint32_t  align;

	char cfg_name[MAX_NAME_SZ];

	/*Whether doorbell or syncpoint is in use for channel.*/
	enum event_type event;

	enum xfer_type xfer_type;
	bool edma_enabled;

	/* chunking information for the channel PCIe shared mem to
	 * fragmented into.
	 */
	uint32_t  nframes;
	uint32_t  frame_size;
};

#endif /*__UITLS_H__*/
