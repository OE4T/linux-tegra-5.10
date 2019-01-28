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

#define RTCPU_FW_CURRENT_VERSION (RTCPU_FW_SM5_VERSION)

#define RTCPU_IVC_SANS_TRACE 1U
#define RTCPU_IVC_WITH_TRACE 2U

#define RTCPU_FW_HASH_SIZE 20U

#define RTCPU_PM_SUSPEND_SUCCESS (0x100U)
#define RTCPU_PM_SUSPEND_FAILURE (0x001U)

#endif /* INCLUDE_CAMRTC_COMMANDS_H */
