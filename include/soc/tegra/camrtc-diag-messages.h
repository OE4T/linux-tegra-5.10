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

#include <camrtc-capture.h>
#include <camrtc-diag.h>

#pragma GCC diagnostic error "-Wpadded"

/**
 * Message types for RCE diagnostics channel
 */

#define CAMRTC_DIAG_ISP5_SDL_SETUP_REQ		U32_C(0x01)
#define CAMRTC_DIAG_ISP5_SDL_SETUP_RESP		U32_C(0x02)

/**
 * Result codes
 */
#define CAMRTC_DIAG_SUCCESS			U32_C(0x00)
#define CAMRTC_DIAG_ERROR_INVAL			U32_C(0x01)
#define CAMRTC_DIAG_ERROR_NOTSUP		U32_C(0x02)
#define CAMRTC_DIAG_ERROR_BUSY			U32_C(0x03)
#define CAMRTC_DIAG_ERROR_TIMEOUT		U32_C(0x04)
#define CAMRTC_DIAG_ERROR_UNKNOWN		U32_C(0xFF)

/**
 * Setup ISP5 SDL periodic diagnostics
 *
 * Submit the pinned addresses of the ISP5 SDL test vectors binary to RCE to
 * enable periodic diagnostics.
 *
 * @param rce_iova	Binary base address (RCE STREAMID)
 * @param isp_iova	Binary base address (ISP STREAMID)
 * @param size		Total size of the test binary
 * @param period	Period [ms] b/w diag. tests submitted in batch,
 *			0 for no repeat
 */
struct camrtc_diag_isp5_sdl_setup_req {
	iova_t rce_iova;
	iova_t isp_iova;
	uint32_t size;
	uint32_t period;
} CAMRTC_DIAG_IVC_ALIGN;

struct camrtc_diag_isp5_sdl_setup_resp {
	uint32_t result;
	uint32_t __pad32[1];
} CAMRTC_DIAG_IVC_ALIGN;

/**
 * Message definition for camrtc diagnostics
 *
 * @param msg_type		Message data type
 * @param transaction_id	ID associated w/ request
 */
struct camrtc_diag_msg {
	uint32_t msg_type;
	uint32_t transaction_id;
	union {
		struct camrtc_diag_isp5_sdl_setup_req isp5_sdl_setup_req;
		struct camrtc_diag_isp5_sdl_setup_resp isp5_sdl_setup_resp;
	};
} CAMRTC_DIAG_IVC_ALIGN;

#pragma GCC diagnostic ignored "-Wpadded"

#endif /* INCLUDE_CAMRTC_DIAG_MESSAGES_H */
