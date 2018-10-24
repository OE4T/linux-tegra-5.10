/*
 * Copyright (c) 2015-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef INCLUDE_CAMRTC_COMMANDS_H
#define INCLUDE_CAMRTC_COMMANDS_H

#include "camrtc-common.h"

/* Messages used with "nvidia,tegra-camrtc-hsp-vm" protocol */

#define CAMRTC_HSP_MSG(_id, _param) \
	(((uint32_t)(_id) << 24U) | ((uint32_t)(_param) & 0xffffffU))
#define CAMRTC_HSP_MSG_ID(_msg) \
	(((_msg) >> 24U) & 0x7fU)
#define CAMRTC_HSP_MSG_PARAM(_msg) \
	((uint32_t)(_msg) & 0xffffffU)

#define CAMRTC_HSP_IRQ		0x00U

#define CAMRTC_HSP_HELLO	0x40U
#define CAMRTC_HSP_BYE		0x41U
#define CAMRTC_HSP_RESUME	0x42U
#define CAMRTC_HSP_SUSPEND	0x43U
#define CAMRTC_HSP_CH_SETUP	0x44U
#define CAMRTC_HSP_PING		0x45U
#define CAMRTC_HSP_FW_HASH	0x46U
#define CAMRTC_HSP_PROTOCOL	0x47U

#define CAMRTC_HSP_UNKNOWN	0x7FU

/* Shared semaphore bits (FW->VM) */
#define CAMRTC_HSP_SS_FW_MASK   0xFFFFU
#define CAMRTC_HSP_SS_FW_SHIFT  0U

/* Shared semaphore bits (VM->FW) */
#define CAMRTC_HSP_SS_VM_MASK   0x7FFF0000U
#define CAMRTC_HSP_SS_VM_SHIFT  16U

/* Bits used by IVC channels */
#define CAMRTC_HSP_SS_IVC_MASK  0xFFU

/* Commands used with "nvidia,tegra-hsp-mailbox" protocol */
#define RTCPU_COMMAND(id, value) \
	(((uint32_t)RTCPU_CMD_ ## id << U32_C(24)) | ((uint32_t)value))

#define RTCPU_GET_COMMAND_ID(value) \
	((((uint32_t)value) >> U32_C(24)) & U32_C(0x7f))

#define RTCPU_GET_COMMAND_VALUE(value) \
	(((uint32_t)value) & U32_C(0xffffff))

#define RTCPU_CMD_INIT		U32_C(0)
#define RTCPU_CMD_FW_VERSION	U32_C(1)
#define RTCPU_CMD_IVC_READY	U32_C(2)
#define RTCPU_CMD_PING		U32_C(3)
#define RTCPU_CMD_PM_SUSPEND	U32_C(4)
#define RTCPU_CMD_FW_HASH	U32_C(5)
#define RTCPU_CMD_CH_SETUP	U32_C(6)
#define RTCPU_CMD_PREFIX	U32_C(0x7d)
#define RTCPU_CMD_DOORBELL	U32_C(0x7e)
#define RTCPU_CMD_ERROR		U32_C(0x7f)

#define RTCPU_FW_DB_VERSION 0U
#define RTCPU_FW_VERSION 1U
#define RTCPU_FW_SM2_VERSION 2U
#define RTCPU_FW_SM3_VERSION 3U
/* SM4 firmware can restore itself after suspend */
#define RTCPU_FW_SM4_VERSION 4U

/* SM5 firmware supports IVC synchronization  */
#define RTCPU_FW_SM5_VERSION 5U
/* SM5 driver supports IVC synchronization  */
#define RTCPU_DRIVER_SM5_VERSION 5U

/* SM6 firmware/driver supports camrtc-hsp-vm protocol  */
#define RTCPU_FW_SM6_VERSION 6U
#define RTCPU_DRIVER_SM6_VERSION 6U

#define RTCPU_IVC_SANS_TRACE 1U
#define RTCPU_IVC_WITH_TRACE 2U

#define RTCPU_FW_HASH_SIZE 20U

#define RTCPU_FW_HASH_ERROR (0xFFFFFFU)

#define RTCPU_PM_SUSPEND_SUCCESS (0x100U)
#define RTCPU_PM_SUSPEND_FAILURE (0x001U)

#define RTCPU_FW_CURRENT_VERSION (RTCPU_FW_SM6_VERSION)

#define RTCPU_FW_INVALID_VERSION (0xFFFFFFU)

#define RTCPU_RESUME_ERROR (0xFFFFFFU)

#endif /* INCLUDE_CAMRTC_COMMANDS_H */
