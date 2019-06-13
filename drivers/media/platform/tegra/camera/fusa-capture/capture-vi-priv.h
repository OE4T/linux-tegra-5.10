/**
 * @file drivers/media/platform/tegra/camera/fusa-capture/capture-vi-priv.h
 * @brief VI channel operations private header for T186/T194
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

#ifndef __FUSA_CAPTURE_VI_PRIV_H__
#define __FUSA_CAPTURE_VI_PRIV_H__

#include <linux/completion.h>
#include <linux/nvhost.h>
#include <linux/tegra-capture-ivc.h>
#include <uapi/linux/nvhost_events.h>
#include <media/fusa-capture/capture-vi.h>

#define CAPTURE_CHANNEL_INVALID_ID	U16_C(0xFFFF)
#define CAPTURE_CHANNEL_INVALID_MASK	U64_C(0x0)
#define NVCSI_STREAM_INVALID_ID		U32_C(0xFFFF)
#define NVCSI_TPG_INVALID_ID		U32_C(0xFFFF)

/**
 * @brief Initialize a VI syncpt. and get its GoS backing.
 *
 * @param[in]	chan	VI channel context
 * @param[in]	name	Syncpt name
 * @param[in]	enable	Whether to initialize or just clear @a sp
 * @param[out]	sp	Syncpt. handle
 *
 * @returns	0 (success), neg. errno (failure)
 */
static int vi_capture_setup_syncpt(
	struct tegra_vi_channel *chan,
	const char *name,
	bool enable,
	struct syncpoint_info *sp);

/**
 * @brief Release a VI syncpt. and clear its handle.
 *
 * @param[in]	chan	VI channel context
 * @param[out]	sp	Syncpt. handle
 */
static void vi_capture_release_syncpt(
	struct tegra_vi_channel *chan,
	struct syncpoint_info *sp);

/**
 * @brief Set up the VI channel progress, embedded data and line timer syncpts.
 *
 * @param[in]	chan	VI channel context
 * @param[in]	flags	Bitmask for channel flags, see
 *			@ref CAPTURE_CHANNEL_FLAGS
 *
 * @returns	0 (success), neg. errno (failure)
 */
static int vi_capture_setup_syncpts(
	struct tegra_vi_channel *chan,
	uint32_t flags);

/**
 * @brief Release the VI channel progress, embedded data and line timer syncpts.
 *
 * @param[in]	chan	VI channel context
 */
static void vi_capture_release_syncpts(
	struct tegra_vi_channel *chan);

/**
 * @brief Read the value of a VI channel syncpt.
 *
 * @param[in]	chan	VI channel context
 * @param[in]	sp	Syncpt. handle
 * @param[out]	val	Syncpt. value
 *
 * @returns	0 (success), neg. errno (failure)
 */
static int vi_capture_read_syncpt(
	struct tegra_vi_channel *chan,
	struct syncpoint_info *sp,
	uint32_t *val);

/**
 * @brief VI channel callback function for @em capture IVC messages.
 *
 * @param[in]	ivc_resp	IVC @ref CAPTURE_MSG from RCE
 * @param[in]	pcontext	VI channel capture context
 */
static void vi_capture_ivc_status_callback(
	const void *ivc_resp,
	const void *pcontext);

/**
 * @brief Send a @em capture-control IVC message to RCE on a VI channel, and
 *	  block w/ timeout, waiting for the RCE response.
 *
 * @param[in]	chan	VI channel context
 * @param[in]	msg	IVC message payload
 * @param[in]	size	Size of @a msg [byte]
 * @param[in]	resp_id	IVC message identifier, see @CAPTURE_MSG_IDS
 *
 * @returns	0 (success), neg. errno (failure)
 */
static int vi_capture_ivc_send_control(
	struct tegra_vi_channel *chan,
	const struct CAPTURE_CONTROL_MSG *msg,
	size_t size,
	uint32_t resp_id);

/**
 * @brief VI channel callback function for @em capture-control IVC messages,
 *	  this unblocks the channel's @em capture-control completion.
 *
 * @param[in]	ivc_resp	IVC @ref CAPTURE_CONTROL_MSG from RCE
 * @param[in]	pcontext	VI channel capture context
 */
static void vi_capture_ivc_control_callback(
	const void *ivc_resp,
	const void *pcontext);

/**
 * @brief Disable the VI channel's NVCSI TPG stream in RCE (called by @em Abort
 *	  functionality).
 *
 * @param[in]	chan	VI channel context
 *
 * @returns	0 (success), neg. errno (failure)
 */
static int csi_stream_tpg_disable(
	struct tegra_vi_channel *chan);

/**
 * @brief Disable the VI channel's NVCSI stream in RCE (called by @em Abort
 *	  functionality).
 *
 * @param[in]	chan	VI channel context
 *
 * @returns	0 (success), neg. errno (failure)
 */
static int csi_stream_close(
	struct tegra_vi_channel *chan);

#endif /* __FUSA_CAPTURE_VI_PRIV_H__ */
