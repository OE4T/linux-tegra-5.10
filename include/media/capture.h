/*
 * Tegra Video Input capture operations
 *
 * Tegra Graphics Host VI
 *
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * Author: David Wang <davidw@nvidia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __VI_CAPTURE_H__
#define __VI_CAPTURE_H__

#if defined(__KERNEL__)
#include <linux/compiler.h>
#include <linux/types.h>
#else
#include <stdint.h>
#endif
#include <media/capture_common.h>
#include <media/capture_vi_channel.h>
#include "soc/tegra/camrtc-capture.h"
#include "soc/tegra/camrtc-capture-messages.h"

#define __VI_CAPTURE_ALIGN __aligned(8)

struct tegra_vi_channel;
struct capture_buffer_table;

/**
 * @brief VI channel capture context
 */
struct vi_capture {
	uint16_t channel_id; /**< RCE-assigned capture channel id */
	struct device *rtcpu_dev; /**< rtcpu device */
	struct tegra_vi_channel *vi_channel; /**< VI channel context */
	struct capture_buffer_table *buf_ctx;
		/**< Surface buffer management table */
	struct capture_common_buf requests; /**< Capture descriptors queue */
	size_t request_buf_size; /**< Size of capture descriptor queue [byte] */
	uint32_t queue_depth; /**< No. of capture descriptors in queue */
	uint32_t request_size; /**< Size of single capture descriptor [byte] */
	bool is_mem_pinned; /**< Whether capture request memory is pinned */

	struct capture_common_status_notifier progress_status_notifier;
		/**< Capture progress status notifier context */
	uint32_t progress_status_buffer_depth;
		/**< No. of capture descriptors */
	bool is_progress_status_notifier_set;
		/**< Whether progress_status_notifer has been initialized */

	uint32_t stream_id; /**< NVCSI PixelParser index [0-5] */
	uint32_t csi_port; /**< NVCSI ports A-H [0-7] */
	uint32_t virtual_channel_id; /**< CSI virtual channel id [0-15] */

	uint32_t num_gos_tables; /**< No. of cv devices in gos_tables */
	const dma_addr_t *gos_tables; /**< IOVA addresses of all GoS devices */

	struct syncpoint_info progress_sp; /**< syncpt for frame progress */
	struct syncpoint_info embdata_sp; /**< syncpt for embedded metadata */
	struct syncpoint_info linetimer_sp; /**< syncpt for frame line timer */

	struct completion control_resp;
		/**< completion for capture-control IVC response */
	struct completion capture_resp;
		/**<
		 * completion for capture requests (frame), if progress status
		 * notifier is not in use
		 */
	struct mutex control_msg_lock;
		/**< Lock for capture-control IVC control_resp_msg */
	struct CAPTURE_CONTROL_MSG control_resp_msg;
		/**< capture-control IVC resp msg written to by callback */

	struct mutex reset_lock; /**< Lock for reset/abort support (via IVC) */
	struct mutex unpins_list_lock; /**< Lock for unpins_list */
	struct capture_common_unpins **unpins_list;
		/**< List of capture request buffer unpins */

	uint64_t vi_channel_mask; /**< Bitmask of RCE-assigned VI channel(s) */
};

/**
 * @brief VI channel setup config (IOCTL)
 *
 * These fields are copied verbatim to a capture_channel_config struct
 * (superset of vi_capture_setup) for the CAPTURE_CHANNEL_SETUP_REQ IVC call.
 */
struct vi_capture_setup {
	uint32_t channel_flags; /**< Bitmask for CAPTURE_CHANNEL_FLAG_*'s */
	uint32_t error_mask_correctable;
		/**< Bitmask for correctable channel errors */
	uint64_t vi_channel_mask;
		/**< Bitmask of VI channels to consider for allocation by RCE */
	uint32_t queue_depth; /**< No. of capture descriptors in queue */
	uint32_t request_size; /**< Size of single capture descriptor */
	union {
		uint32_t mem; /**< Capture descriptors queue NvRm handle */
		uint64_t iova;
			/**<
			 * Capture descriptors queue base address (written back
			 * after pinning by KMD)
			 */
	};
	uint8_t slvsec_stream_main;
		/**< SLVS-EC main stream (hardcode to 0x00) */
	uint8_t slvsec_stream_sub;
		/**< SLVS-EC sub stream (hardcode to 0xFF - disabled) */
	uint16_t __pad_slvsec1;
	uint32_t error_mask_uncorrectable;
		/**< Bitmask for uncorrectable channel errors */
	uint64_t stop_on_error_notify_bits;
		/**<
		 * Bitmask for NOTIFY errors that force channel stop upon
		 * receipt
		 */
	uint64_t reserved[2];
} __VI_CAPTURE_ALIGN;

/**
 * @brief VI capture info (resp. to query)
 */
struct vi_capture_info {
	struct vi_capture_syncpts {
		uint32_t progress_syncpt; /**< Progress syncpt id */
		uint32_t progress_syncpt_val; /**< Progress syncpt value */
		uint32_t emb_data_syncpt; /**< Embedded metadata syncpt id */
		uint32_t emb_data_syncpt_val;
			/**< Embedded metadata syncpt value */
		uint32_t line_timer_syncpt; /**< Line timer syncpt id */
		uint32_t line_timer_syncpt_val; /**< Line timer syncpt value */
	} syncpts;
	uint32_t hw_channel_id; /**< RCE-assigned capture channel id */
	uint32_t __pad;
	uint64_t vi_channel_mask; /**< Bitmask of RCE-assigned VI channel(s) */
} __VI_CAPTURE_ALIGN;

/**
 * @brief Container for CAPTURE_CONTROL_MSG req./resp. from FuSa UMD (IOCTL)
 *
 * The response and request pointers may be to the same memory allocation; in
 * which case the request message will be overwritten by the response.
 */
struct vi_capture_control_msg {
	uint64_t ptr; /**< Pointer to capture-control message req. */
	uint32_t size; /**< Size of req./resp. msg. [byte] */
	uint32_t __pad;
	uint64_t response; /**< Pointer to capture-control message resp. */
} __VI_CAPTURE_ALIGN;

/**
 * @brief VI capture request (IOCTL)
 */
struct vi_capture_req {
	uint32_t buffer_index; /**< Capture descriptor index */
	uint32_t num_relocs; /**< No. of surface buffers to pin/reloc */
	uint64_t reloc_relatives;
	/**<
	 * Offsets to surface buffer addresses to patch in capture
	 * descriptor
	 */
} __VI_CAPTURE_ALIGN;

/**
 * @brief VI capture progress status setup config (IOCTL)
 */
struct vi_capture_progress_status_req {
	uint32_t mem; /**< NvRm handle to buffer region start */
	uint32_t mem_offset; /**< Status notifier offset [byte] */
	uint32_t buffer_depth; /**< Capture descriptor queue size */
	uint32_t __pad[3];
} __VI_CAPTURE_ALIGN;

/**
 * @brief Add VI capture surface buffer to management (IOCTL)
 */
struct vi_buffer_req {
	uint32_t mem; /**< NvRm handle to buffer */
	uint32_t flag; /**< Surface BUFFER_* op bitmask */
} __VI_CAPTURE_ALIGN;

/**
 * The compand configuration describes a piece-wise linear
 * tranformation function used by the VI companding module.
 */
#define VI_CAPTURE_NUM_COMPAND_KNEEPTS 10

/**
 * @brief VI compand setup config (IOCTL)
 */
struct vi_capture_compand {
	uint32_t base[VI_CAPTURE_NUM_COMPAND_KNEEPTS];
		/**< kneept base param */
	uint32_t scale[VI_CAPTURE_NUM_COMPAND_KNEEPTS];
		/**< kneept scale param */
	uint32_t offset[VI_CAPTURE_NUM_COMPAND_KNEEPTS];
		/**< kneept offset param */
} __VI_CAPTURE_ALIGN;

/**
 * @brief Initialize a VI channel capture context (at channel open)
 *
 * The VI channel context is already partially-initialized by the calling
 * function, the channel capture context is allocated and linked here.
 *
 * @param[in,out]	chan		Allocated VI channel context,
 *					partially-initialized
 * @param[in]		is_mem_pinned	Whether capture request memory is pinned
 * @returns		0 (success), errno (error)
 */
int vi_capture_init(
	struct tegra_vi_channel *chan,
	bool is_mem_pinned);

/**
 * @brief De-initialize a VI capture channel, closing open VI/NVCSI streams, and
 * freeing the buffer management table and channel capture context.
 *
 * The VI channel context is not freed in this function, only the capture
 * context is.
 *
 * The VI/NVCSI channel/streams (allocated by RCE) should normally be released
 * when this function is called, but they may still be active due to programming
 * error or client UMD crash.
 *
 * @param[in,out]	chan		VI channel context
 */
void vi_capture_shutdown(
	struct tegra_vi_channel *chan);

/**
 * @brief Open a VI channel in RCE, sending channel configuration to request a
 * HW channel allocation. Syncpts are allocated by the KMD in this routine.
 *
 * @param[in,out]	chan	VI channel context
 * @param[in]		setup	VI channel setup config
 * @returns		0 (success), errno (error)
 */
int vi_capture_setup(
	struct tegra_vi_channel *chan,
	struct vi_capture_setup *setup);

/**
 * @brief Reset an opened VI channel, all pending capture requests to RCE are
 * discarded.
 *
 * The channel's progress syncpoint is advanced to the threshold of the latest
 * capture request to unblock any waiting observers.
 *
 * A reset barrier may be enqueued in the capture IVC channel to flush stale
 * capture descriptors, in case of abnormal channel termination.
 *
 * @param[in]	chan		VI channel context
 * @param[in]	reset_flags	Bitmask for VI channel reset options
 *				(CAPTURE_CHANNEL_RESET_FLAG_*)
 * @returns	0 (success), errno (error)
 */
int vi_capture_reset(
	struct tegra_vi_channel *chan,
	uint32_t reset_flags);

/**
 * @brief Release an opened VI channel; the RCE channel allocation, syncpts and
 * ivc channel callbacks are released.
 *
 * @param[in]	chan		VI channel context
 * @param[in]	reset_flags	Bitmask for VI channel reset options
 *				(CAPTURE_CHANNEL_RESET_FLAG_*)
 * @returns	0 (success), errno (error)
 */
int vi_capture_release(
	struct tegra_vi_channel *chan,
	uint32_t reset_flags);

/**
 * @brief Release the TPG and/or NVCSI stream on a VI channel, if they are
 * active.
 *
 * This function normally does not execute except in the event of abnormal UMD
 * termination, as it is the client's responsibility to open and close NVCSI and
 * TPG sources.
 *
 * @param[in]	chan	VI channel context
 * @returns	0 (success), errno (error)
 */
int csi_stream_release(
	struct tegra_vi_channel *chan);

/**
 * @brief Query VI channel syncpt ids and values and RCE-assigned VI channel
 * nos.
 *
 * @param[in]	chan	VI channel context
 * @param[out]	info	VI channel info response
 * @returns	0 (success), errno (error)
 */
int vi_capture_get_info(
	struct tegra_vi_channel *chan,
	struct vi_capture_info *info);

/**
 * @brief Send a capture-control IVC message to RCE and wait for a response.
 *
 * This is a blocking call, with the possibility of timeout.
 *
 * @param[in]		chan	VI channel context
 * @param[in,out]	msg	capture-control IVC container w/ req./resp. pair
 * @returns		0 (success), errno (error)
 */
int vi_capture_control_message(
	struct tegra_vi_channel *chan,
	struct vi_capture_control_msg *msg);

/**
 * @brief Send a capture request (for a frame) via the capture IVC channel to
 * RCE.
 *
 * This is a non-blocking call.
 *
 * @param[in]	chan	VI channel context
 * @param[in]	msg	VI capture request (frame)
 * @returns	0 (success), errno (error)
 */
int vi_capture_request(
	struct tegra_vi_channel *chan,
	struct vi_capture_req *req);

/**
 * @brief Wait on receipt of the capture status of the last capture request to
 * RCE. The RCE VI driver sends a CAPTURE_STATUS_IND notification at frame
 * completion.
 *
 * This is a blocking call, with the possibility of timeout.
 *
 * @todo The capture progress status notifier is expected to replace this
 * functionality in the future, deprecating it.
 *
 * @param[in]	chan		VI channel context
 * @param[in]	timeout_ms	Time to wait for status completion [ms], set to
 *				0 for infinite
 * @returns	0 (success), errno (error)
 */
int vi_capture_status(
	struct tegra_vi_channel *chan,
	int32_t timeout_ms);

/**
 * @brief Setup VI compand in RCE
 *
 * @param[in]	chan	VI channel context
 * @param[in]	compand	VI compand setup config
 * @returns	0 (success), errno (error)
 */
int vi_capture_set_compand(
	struct tegra_vi_channel *chan,
	struct vi_capture_compand *compand);

/**
 * @brief Setup VI channel capture status progress notifer
 *
 * @param[in]	chan	VI channel context
 * @param[in]	req	VI capture progress status setup config
 * @returns	0 (success), errno (error)
 */
int vi_capture_set_progress_status_notifier(
	struct tegra_vi_channel *chan,
	struct vi_capture_progress_status_req *req);

#endif /* __VI_CAPTURE_H__ */
