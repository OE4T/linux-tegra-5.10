/*
 * Copyright (c) 2019, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef INCLUDE_CAMRTC_DIAG_MESSAGES_H
#define INCLUDE_CAMRTC_DIAG_MESSAGES_H

#include <camrtc-common.h>
#include <camrtc-capture.h>
#include <camrtc-diag.h>

#pragma GCC diagnostic error "-Wpadded"

/**
 * @defgroup MessageType Message types for RCE diagnostics channel
 * @{
 */
#define CAMRTC_DIAG_ISP5_SDL_SETUP_REQ		MK_U32(0x01)
#define CAMRTC_DIAG_ISP5_SDL_SETUP_RESP		MK_U32(0x02)
/**@}*/

/**
 * @defgroup ResultCodes Diagnostics channel Result codes
 * @{
 */
#define CAMRTC_DIAG_SUCCESS			MK_U32(0x00)
#define CAMRTC_DIAG_ERROR_INVAL			MK_U32(0x01)
#define CAMRTC_DIAG_ERROR_NOTSUP		MK_U32(0x02)
#define CAMRTC_DIAG_ERROR_BUSY			MK_U32(0x03)
#define CAMRTC_DIAG_ERROR_TIMEOUT		MK_U32(0x04)
#define CAMRTC_DIAG_ERROR_UNKNOWN		MK_U32(0xFF)
/**@}*/

/**
 * @brief camrtc_diag_isp5_sdl_setup_req - ISP5 SDL periodic diagnostics setup request.
 *
 * Submit the pinned addresses of the ISP5 SDL test vectors binary to RCE to
 * enable periodic diagnostics.
 */
struct camrtc_diag_isp5_sdl_setup_req {
	/** Binary base address (RCE STREAMID). */
	iova_t rce_iova;
	/** Binary base address (ISP STREAMID). */
	iova_t isp_iova;
	/** Total size of the test binary. */
	uint32_t size;
	/** Period [ms] b/w diagnostics tests submitted in batch, zero for no repeat. */
	uint32_t period;
} CAMRTC_DIAG_IVC_ALIGN;

/**
 * @brief camrtc_diag_isp5_sdl_setup_resp - ISP5 SDL periodic diagnostics setup response.
 *
 * Setup status is returned in the result field.
 */
struct camrtc_diag_isp5_sdl_setup_resp {
	/** SDL setup status returned to caller. */
	uint32_t result;
	/** Reserved. */
	uint32_t __pad32[1];
} CAMRTC_DIAG_IVC_ALIGN;

/**
 * @brief camrtc_diag_msg - Message definition for camrtc diagnostics
 */
struct camrtc_diag_msg {
	/** @ref MessageType "Message type". */
	uint32_t msg_type;
	/** ID associated w/ request. */
	uint32_t transaction_id;
	union {
		/** SDL setup req structure. */
		struct camrtc_diag_isp5_sdl_setup_req isp5_sdl_setup_req;
		/** SDL setup resp structure. */
		struct camrtc_diag_isp5_sdl_setup_resp isp5_sdl_setup_resp;
	};
} CAMRTC_DIAG_IVC_ALIGN;

#pragma GCC diagnostic ignored "-Wpadded"

#endif /* INCLUDE_CAMRTC_DIAG_MESSAGES_H */
