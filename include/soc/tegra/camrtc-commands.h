/*
 * Copyright (c) 2015-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

/**
 * @file camrtc-commands.h
 *
 * @brief Commands used with "nvidia,tegra-camrtc-hsp-vm" & "nvidia,tegra-hsp-mailbox"
 * protocol
 */

#ifndef INCLUDE_CAMRTC_COMMANDS_H
#define INCLUDE_CAMRTC_COMMANDS_H

#include "camrtc-common.h"

/**
 * @defgroup HspVmMsgs Definitions for "nvidia,tegra-camrtc-hsp-vm" protocol
 * @{
 */
#define CAMRTC_HSP_MSG(_id, _param) ( \
	((uint32_t)(_id) << MK_U32(24)) | \
	((uint32_t)(_param) & MK_U32(0xffffff)))
#define CAMRTC_HSP_MSG_ID(_msg) \
	(((_msg) >> MK_U32(24)) & MK_U32(0x7f))
#define CAMRTC_HSP_MSG_PARAM(_msg) \
	((uint32_t)(_msg) & MK_U32(0xffffff))

#define CAMRTC_HSP_IRQ			MK_U32(0x00)

#define CAMRTC_HSP_HELLO		MK_U32(0x40)
#define CAMRTC_HSP_BYE			MK_U32(0x41)
#define CAMRTC_HSP_RESUME		MK_U32(0x42)
#define CAMRTC_HSP_SUSPEND		MK_U32(0x43)
#define CAMRTC_HSP_CH_SETUP		MK_U32(0x44)
#define CAMRTC_HSP_PING			MK_U32(0x45)
#define CAMRTC_HSP_FW_HASH		MK_U32(0x46)
#define CAMRTC_HSP_PROTOCOL		MK_U32(0x47)
#define CAMRTC_HSP_RESERVED_5E		MK_U32(0x5E) /* bug 200395605 */
#define CAMRTC_HSP_UNKNOWN		MK_U32(0x7F)

/** Shared semaphore bits (FW->VM) */
#define CAMRTC_HSP_SS_FW_MASK		MK_U32(0xFFFF)
#define CAMRTC_HSP_SS_FW_SHIFT		MK_U32(0)

/** Shared semaphore bits (VM->FW) */
#define CAMRTC_HSP_SS_VM_MASK		MK_U32(0x7FFF0000)
#define CAMRTC_HSP_SS_VM_SHIFT		MK_U32(16)

/** Bits used by IVC channels */
#define CAMRTC_HSP_SS_IVC_MASK		MK_U32(0xFF)

/** @} */

/**
 * @defgroup HspMailboxMsgs Definitions for "nvidia,tegra-hsp-mailbox" protocol
 * @{
 */
#define RTCPU_COMMAND(id, value) \
	(((RTCPU_CMD_ ## id) << MK_U32(24)) | ((uint32_t)value))

#define RTCPU_GET_COMMAND_ID(value) \
	((((uint32_t)value) >> MK_U32(24)) & MK_U32(0x7f))

#define RTCPU_GET_COMMAND_VALUE(value) \
	(((uint32_t)value) & MK_U32(0xffffff))

#define RTCPU_CMD_INIT			MK_U32(0)
#define RTCPU_CMD_FW_VERSION		MK_U32(1)
#define RTCPU_CMD_IVC_READY		MK_U32(2)
#define RTCPU_CMD_PING			MK_U32(3)
#define RTCPU_CMD_PM_SUSPEND		MK_U32(4)
#define RTCPU_CMD_FW_HASH		MK_U32(5)
#define RTCPU_CMD_CH_SETUP		MK_U32(6)
#define RTCPU_CMD_RESERVED_5E		MK_U32(0x5E) /* bug 200395605 */
#define RTCPU_CMD_PREFIX		MK_U32(0x7d)
#define RTCPU_CMD_DOORBELL		MK_U32(0x7e)
#define RTCPU_CMD_ERROR			MK_U32(0x7f)

#define RTCPU_FW_DB_VERSION		MK_U32(0)
#define RTCPU_FW_VERSION		MK_U32(1)
#define RTCPU_FW_SM2_VERSION		MK_U32(2)
#define RTCPU_FW_SM3_VERSION		MK_U32(3)
/** SM4 firmware can restore itself after suspend */
#define RTCPU_FW_SM4_VERSION		MK_U32(4)

/** SM5 firmware supports IVC synchronization  */
#define RTCPU_FW_SM5_VERSION		MK_U32(5)
/** SM5 driver supports IVC synchronization  */
#define RTCPU_DRIVER_SM5_VERSION	MK_U32(5)

/** SM6 firmware/driver supports camrtc-hsp-vm protocol	 */
#define RTCPU_FW_SM6_VERSION		MK_U32(6)
#define RTCPU_DRIVER_SM6_VERSION	MK_U32(6)

#define RTCPU_IVC_SANS_TRACE		MK_U32(1)
#define RTCPU_IVC_WITH_TRACE		MK_U32(2)

#define RTCPU_FW_HASH_SIZE		MK_U32(20)

#define RTCPU_FW_HASH_ERROR		MK_U32(0xFFFFFF)

#define RTCPU_PM_SUSPEND_SUCCESS	MK_U32(0x100)
#define RTCPU_PM_SUSPEND_FAILURE	MK_U32(0x001)

#define RTCPU_FW_CURRENT_VERSION	RTCPU_FW_SM6_VERSION

#define RTCPU_FW_INVALID_VERSION	MK_U32(0xFFFFFF)

#define RTCPU_RESUME_ERROR		MK_U32(0xFFFFFF)

/** @} */

#endif /* INCLUDE_CAMRTC_COMMANDS_H */
