/* include/uapi/misc/nvscic2c-ioctl.h
 *
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * This header is BSD licensed so anyone can use the definitions to implement
 * compatible drivers/servers.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of NVIDIA CORPORATION nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL NVIDIA CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __UAPI_NVSIC2C_IOCTL_H__
#define __UAPI_NVSIC2C_IOCTL_H__

#include <linux/ioctl.h>

#if !defined(__KERNEL__)
#define __user
#endif

/*
 * Offests for flow-control/state fields in the nvscic2c dev channel
 * header. All fields are at the moment are 32-bit wide.
 *
 * IMPORTANT: If these change from 32-bit to other, change must be
 * made here too. Also, must be in-sync with user-space SW(same host) and
 * nvscic2c on remote host.
 *
 * These are used for writing/updating channel header fields of
 * peer host over PCIe by CPU or for reading the same fields that remote
 * updated on our PCIe shared mem over PCIe or for reading from the
 * control memory that each channel privately manages and updates.
 *
 * (CH_HDR_RESERVED_OFF+4bytes) to start data on 8-byte boundary for better
 *  perf with PCIe Wr.
 */
#define CH_HDR_TX_CNTR_OFF  (0x00U)
#define CH_HDR_RX_CNTR_OFF  (0x04U)
#define CH_HDR_W_SLEEP_OFF  (0x08U)
#define CH_HDR_R_SLEEP_OFF  (0x0CU)
#define CH_HDR_STATE_OFF    (0x10U)
#define CH_HDR_RESERVED_OFF (0x14U)
#define CH_DATA_PAYLOAD_OFF (0x18U)
#define CH_HDR_SIZE         (CH_DATA_PAYLOAD_OFF)

#define MAX_NAME_SZ         (32)

/*
 * bulk data transfer channels could be uni-directional. If there is no
 * use-case for bi-directional data xfer but we still create a full-duplex
 * single nvscic2c bulk channel, we end up leaving lot of PCIe shared memory
 * being un-utilised.
 */
enum xfer_type {
	/* it's not a bulk data xfer device but plain CPU channel.
	 * data dir: Self<->Peer. (default, do not change this value).
	 */
	XFER_TYPE_CPU = 0,

	/* This device supports only bulk transfer,
	 * data dir: Peer->Self. We do write over PCIe for data.
	 */
	XFER_TYPE_BULK_PRODUCER,

	/* This device supports only bulk transfer,
	 * data dir: Self->Peer. We do write over PCIe for data.
	 */
	XFER_TYPE_BULK_CONSUMER,

	/* This device supports only bulk transfer,
	 * data dir: Peer->Self but we use Self capability to read
	 * data over PCIe(typically DMA).
	 */
	XFER_TYPE_BULK_PRODUCER_PCIE_READ,

	/* This device supports only bulk transfer,
	 * data dir: Self->Peer but we use peer capability to read
	 * data over PCIe(typically DMA).
	 */
	XFER_TYPE_BULK_CONSUMER_PCIE_READ,

	/* Invalid.*/
	XFER_TYPE_INVALID
};

/**
 * PCIe aperture and PCIe shared memory
 * are divided in different C2C channels.
 * Data structure represents channel's
 * physical address and size.
 */
struct channel_mem_info {
	/* would be one of the enum nvscic2c_mem_type.*/
	uint32_t offset;

	/* size of this memory type device would like user-space to map.*/
	uint32_t size;
};

/**
 * DT parsing is done by Resmgr.
 * C2C channels are of different type Cpu/Bulk.
 * Different type of memories are involved with
 * C2C channels.
 * Each C2C channel uses NvSciIpc(IVC) channel
 * in background for notification and DMA purpose.
 * NvSciIpc channel name is available in DT file.
 * Info should be supplied by Resmgr to guest app.
 */
struct nvscic2c_info {
	enum xfer_type              xfer_type;
	bool                        edma_enabled;
	char                        cfg_name[MAX_NAME_SZ];
	uint32_t                    nframes;
	uint32_t                    frame_size;
	struct channel_mem_info     peer;
	struct channel_mem_info     self;
	struct channel_mem_info     ctrl;
	struct channel_mem_info     link;
};

/* IOCTL magic number - seen available in ioctl-number.txt*/
#define NVSCIC2C_IOCTL_MAGIC    0xC2

#define NVSCIC2C_IOCTL_GET_INFO \
	_IOWR(NVSCIC2C_IOCTL_MAGIC, 1, struct nvscic2c_info)

#define NVSCIC2C_IOCTL_NUMBER_MAX 1

#endif /*__UAPI_NVSIC2C_IOCTL_H__*/
