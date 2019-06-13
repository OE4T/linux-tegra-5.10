/**
 * @file drivers/media/platform/tegra/camera/fusa-capture/capture-isp-priv.h
 * @brief ISP channel operations private header for T186/T194
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

#ifndef __FUSA_CAPTURE_ISP_PRIV_H__
#define __FUSA_CAPTURE_ISP_PRIV_H__

#include <linux/completion.h>
#include <linux/nvhost.h>
#include <linux/of_platform.h>
#include <linux/tegra-capture-ivc.h>
#include <media/mc_common.h>

#include <media/fusa-capture/capture-common.h>
#include <media/fusa-capture/capture-isp.h>
#include <media/fusa-capture/capture-isp-channel.h>

#define CAPTURE_CHANNEL_ISP_INVALID_ID U16_C(0xFFFF)

/**
 * @brief ISP channel capture/program queue context
 */
struct isp_desc_rec {
	struct capture_common_buf requests;
	size_t request_buf_size;
	uint32_t queue_depth;
		/**< No. of capture/program descriptors in queue. */
	uint32_t request_size;
		/**< Size of single capture/program descriptor [byte]. */

	uint32_t progress_status_buffer_depth;
		/**< No. of capture/program descriptors. */

	struct mutex unpins_list_lock; /**< Lock for unpins_list. */
	struct capture_common_unpins **unpins_list;
		/**< List of capture request buffer unpins. */
};

/**
 * @brief ISP channel capture context
 */
struct isp_capture {
	uint16_t channel_id; /**< RCE-assigned capture channel id. */
	struct device *rtcpu_dev; /**< rtcpu device. */
	struct tegra_isp_channel *isp_channel; /**< ISP channel context. */
	struct capture_buffer_table *buffer_ctx;
		/**< Surface buffer management table. */

	struct isp_desc_rec capture_desc_ctx;
		/**< Capture descriptor queue context. */
	struct isp_desc_rec program_desc_ctx;
		/**< Program descriptor queue context. */

	struct capture_common_status_notifier progress_status_notifier;
		/**< Capture progress status notifier context. */
	bool is_progress_status_notifier_set;
		/**< Whether progress_status_notifer has been initialized. */

#ifdef HAVE_ISP_GOS_TABLES
	uint32_t num_gos_tables; /**< No. of cv devices in gos_tables. */
	const dma_addr_t *gos_tables; /**< IOVA addresses of all GoS devices. */
#endif

	struct syncpoint_info progress_sp; /**< Syncpt for frame progress. */
	struct syncpoint_info stats_progress_sp;
		/**< Syncpt for stats progress. */

	struct completion control_resp;
		/**< Completion for capture-control IVC response. */
	struct completion capture_resp;
		/**<
		 * Completion for capture requests (frame), if progress status
		 * notifier is not in use.
		 */
	struct completion capture_program_resp;
		/**<
		 * Completion for program requests (frame), if progress status
		 * notifier is not in use.
		 */

	struct mutex control_msg_lock;
		/**< Lock for capture-control IVC control_resp_msg. */
	struct CAPTURE_CONTROL_MSG control_resp_msg;
		/**< capture-control IVC resp msg written to by callback. */

	struct mutex reset_lock;
		/**< Channel lock for reset/abort support (via RCE). */
	bool reset_capture_program_flag;
		/**< Reset flag to drain pending program requests. */
	bool reset_capture_flag;
		/**< Reset flag to drain pending capture requests. */
};

/**
 * @brief Initialize an ISP syncpt. and get its GoS backing.
 *
 * @param[in]	chan	ISP channel context
 * @param[in]	name	Syncpt name
 * @param[in]	enable	Whether to initialize or just clear @a sp
 * @param[out]	sp	Syncpt. handle
 *
 * @returns	0 (success), neg. errno (failure)
 */
static int isp_capture_setup_syncpt(
	struct tegra_isp_channel *chan,
	const char *name,
	bool enable,
	struct syncpoint_info *sp);

/**
 * @brief Release an ISP syncpt. and clear its handle.
 *
 * @param[in]	chan	ISP channel context
 * @param[out]	sp	Syncpt. handle
 */
static void isp_capture_release_syncpt(
	struct tegra_isp_channel *chan,
	struct syncpoint_info *sp);

/**
 * @brief Set up the ISP channel progress and stats progress syncpts.
 *
 * @param[in]	chan	ISP channel context
 *
 * @returns	0 (success), neg. errno (failure)
 */
static int isp_capture_setup_syncpts(
	struct tegra_isp_channel *chan);

/**
 * @brief Release the ISP channel progress and stats progress syncpts.
 *
 * @param[in]	chan	ISP channel context
 *
 * @returns	0 (success), neg. errno (failure)
 */
static void isp_capture_release_syncpts(
	struct tegra_isp_channel *chan);

/**
 * @brief Read the value of an ISP channel syncpt.
 *
 * @param[in]	chan	ISP channel context
 * @param[in]	sp	Syncpt. handle
 * @param[out]	val	Syncpt. value
 *
 * @returns	0 (success), neg. errno (failure)
 */
static int isp_capture_read_syncpt(
	struct tegra_isp_channel *chan,
	struct syncpoint_info *sp,
	uint32_t *val);

/**
 * @brief Patch the descriptor GoS SID (@a gos_relative) and syncpt. shim
 *	  address (@a sp_relative) with the ISP IOVA-mapped addresses of a
 *	  syncpt. (@a fence_offset).
 *
 * @param[in]	chan		ISP channel context
 * @param[in]	fence_offset	Syncpt. offset from joint descriptor queue
 *				[byte]
 * @param[in]	gos_relative	GoS SID offset from @a fence_offset [byte]
 * @param[in]	sp_relative	Shim address from @a fence_offset [byte]
 */
static int isp_capture_populate_fence_info(
	struct tegra_isp_channel *chan,
	int fence_offset,
	uint32_t gos_relative,
	uint32_t sp_relative);

/**
 * @brief Patch the inputfence syncpts. of a capture descriptor w/ ISP
 *	  IOVA-mapped addresses.
 *
 * @param[in]	chan		ISP channel context
 * @param[in]	req		ISP capture request
 * @param[in]	request_offset	Descriptor offset from joint descriptor queue
 *				[byte].
 */
static int isp_capture_setup_inputfences(
	struct tegra_isp_channel *chan,
	struct isp_capture_req *req,
	int request_offset);

/**
 * @brief Patch the prefence syncpts. of a capture descriptor w/ ISP
 *	  IOVA-mapped addresses.
 *
 * @param[in]	chan		ISP channel context
 * @param[in]	req		ISP capture request
 * @param[in]	request_offset	Descriptor offset from joint descriptor queue
 *				[byte].
 */
static int isp_capture_setup_prefences(
	struct tegra_isp_channel *chan,
	struct isp_capture_req *req,
	int request_offset);

/**
 * @brief Unpin and free the list of pinned capture_mapping's associated with an
 *	  ISP capture request.
 *
 * @param[in]	chan		ISP channel context
 * @param[in]	buffer_index	Capture descriptor queue index
 */
static void isp_capture_request_unpin(
	struct tegra_isp_channel *chan,
	uint32_t buffer_index);

/**
 * @brief Unpin and free the list of pinned capture_mapping's associated with an
 *	  ISP program request.
 *
 * @param[in]	chan		ISP channel context
 * @param[in]	buffer_index	Program descriptor queue index
 */
static void isp_capture_program_request_unpin(
	struct tegra_isp_channel *chan,
	uint32_t buffer_index);

/**
 * @brief Prepare and submit a pin and relocation request for a program
 *	  descriptor, the resultant mappings are added to the channel program
 *	  descriptor queue's @em unpins_list.
 *
 * @param[in]	chan	ISP channel context
 * @param[in]	req	ISP program request
 */
static int isp_capture_program_prepare(
	struct tegra_isp_channel *chan,
	struct isp_program_req *req);

/**
 * @brief Unpin an ISP capture request and flush the memory.
 *
 * @param[in]	capture		ISP channel capture context
 * @param[in]	buffer_index	Capture descriptor queue index
 */
static inline void isp_capture_ivc_capture_cleanup(
	struct isp_capture *capture,
	uint32_t buffer_index);

/**
 * @brief Signal completion or write progress status to notifier for ISP capture
 *	  indication from RCE.
 *
 * If the ISP channel's progress status notifier is not set, the capture
 * completion will be signalled.
 *
 * @param[in]	capture		ISP channel capture context
 * @param[in]	buffer_index	Capture descriptor queue index
 */
static inline void isp_capture_ivc_capture_signal(
	struct isp_capture *capture,
	uint32_t buffer_index);

/**
 * @brief Unpin an ISP program request and flush the memory.
 *
 * @param[in]	capture		ISP channel capture context
 * @param[in]	buffer_index	Program descriptor queue index
 */
static inline void isp_capture_ivc_program_cleanup(
	struct isp_capture *capture,
	uint32_t buffer_index);

/**
 * @brief Signal completion or write progress status to notifier for ISP program
 *	  indication from RCE.
 *
 * If the ISP channel's progress status notifier is not set, the program
 * completion will be signalled.
 *
 * @param[in]	capture		ISP channel capture context
 * @param[in]	buffer_index	Program descriptor queue index
 */
static inline void isp_capture_ivc_program_signal(
	struct isp_capture *capture,
	uint32_t buffer_index);

/**
 * @brief ISP channel callback function for @em capture IVC messages.
 *
 * @param[in]	ivc_resp	IVC @ref CAPTURE_MSG from RCE
 * @param[in]	pcontext	ISP channel capture context
 */
static void isp_capture_ivc_status_callback(
	const void *ivc_resp,
	const void *pcontext);

/**
 * @brief Send a @em capture-control IVC message to RCE on an ISP channel, and
 *	  block w/ timeout, waiting for the RCE response.
 *
 * @param[in]	chan	ISP channel context
 * @param[in]	msg	IVC message payload
 * @param[in]	size	Size of @a msg [byte]
 * @param[in]	resp_id	IVC message identifier, see @CAPTURE_MSG_IDS
 *
 * @returns	0 (success), neg. errno (failure)
 */
static int isp_capture_ivc_send_control(
	struct tegra_isp_channel *chan,
	const struct CAPTURE_CONTROL_MSG *msg,
	size_t size,
	uint32_t resp_id);

/**
 * @brief ISP channel callback function for @em capture-control IVC messages,
 *	  this unblocks the channel's @em capture-control completion.
 *
 * @param[in]	ivc_resp	IVC @ref CAPTURE_CONTROL_MSG from RCE
 * @param[in]	pcontext	ISP channel capture context
 */
static void isp_capture_ivc_control_callback(
	const void *ivc_resp,
	const void *pcontext);

#endif /* __FUSA_CAPTURE_ISP_PRIV_H__ */
