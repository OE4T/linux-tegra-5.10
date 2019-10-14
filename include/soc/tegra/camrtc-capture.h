/*
 * Copyright (c) 2016-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

/**
 * @file camrtc-capture.h
 *
 * @brief Camera firmware API header
 */

#ifndef INCLUDE_CAMRTC_CAPTURE_H
#define INCLUDE_CAMRTC_CAPTURE_H

#include "camrtc-common.h"

#pragma GCC diagnostic error "-Wpadded"

#define CAPTURE_IVC_ALIGNOF		MK_ALIGN(8)
#define CAPTURE_DESCRIPTOR_ALIGNOF	MK_ALIGN(64)

#define CAPTURE_IVC_ALIGN		__aligned(CAPTURE_IVC_ALIGNOF)
#define CAPTURE_DESCRIPTOR_ALIGN	__aligned(CAPTURE_DESCRIPTOR_ALIGNOF)

/* Deprecated */
#define __CAPTURE_IVC_ALIGN		CAPTURE_IVC_ALIGN
#define __CAPTURE_DESCRIPTOR_ALIGN	CAPTURE_DESCRIPTOR_ALIGN

typedef uint64_t iova_t CAPTURE_IVC_ALIGN;

#define SYNCPOINT_ID_INVALID	MK_U32(0)
#define GOS_INDEX_INVALID	MK_U8(0xFF)

#pragma GCC diagnostic warning "-Wdeprecated-declarations"
#define CAMRTC_DEPRECATED __attribute__((deprecated))

/*Status Fence Support*/
#define STATUS_FENCE_SUPPORT

typedef struct syncpoint_info {
	uint32_t id;
	uint32_t threshold;	/* When storing a fence */
	uint8_t gos_sid;
	uint8_t gos_index;
	uint16_t gos_offset;
	uint32_t pad_;
	iova_t shim_addr;
} syncpoint_info_t CAPTURE_IVC_ALIGN;

/**
 * @defgroup StatsSize Statistics data size defines
 *
 * The size for each unit includes the standard ISP5 HW stats
 * header size.
 *
 * Size break down for each unit.
 *  FB = 32 byte header + (256 x 4) bytes. FB has 256 windows with 4 bytes
 *       of stats data per window.
 *  FM = 32 byte header + (64 x 64 x 2 x 4) bytes. FM can have 64 x 64 windows
 *       with each windows having 2 bytes of data for each color channel.
 *  AFM = 32 byte header + 8 byte statistics data per ROI.
 *  LAC = 32 byte header + ( (32 x 32) x ((4 + 2 + 2) x 4) )
 *        Each ROI has 32x32 windows with each window containing 8
 *        bytes of data per color channel.
 *  Hist = Header + (256 x 4 x 4) bytes since Hist unit has 256 bins and
 *         each bin collects 4 byte data for each color channel + 4 Dwords for
 *         excluded pixel count due to elliptical mask per color channel.
 *  Pru = 32 byte header + (8 x 4) bytes for bad pixel count and accumulated
 *        pixel adjustment for pixels both inside and outside the ROI.
 *  LTM = 32 byte header + (128 x 4) bytes for histogram data + (8 x 8 x 4 x 2)
 *        bytes for soft key average and count. Soft key statistics are
 *        collected by dividing the frame into a 8x8 array region.
 * @{
 */
/** Statistics unit hardware header size in bytes */
#define ISP5_STATS_HW_HEADER_SIZE	MK_SIZE(32)
/** Flicker band (FB) unit statistics data size in bytes */
#define ISP5_STATS_FB_MAX_SIZE		MK_SIZE(1056)
/** Focus Metrics (FM) unit statistics data size in bytes */
#define ISP5_STATS_FM_MAX_SIZE		MK_SIZE(32800)
/** Auto Focus Metrics (AFM) unit statistics data size in bytes */
#define ISP5_STATS_AFM_ROI_MAX_SIZE	MK_SIZE(40)
/** Local Average Clipping (LAC) unit statistics data size in bytes */
#define ISP5_STATS_LAC_ROI_MAX_SIZE	MK_SIZE(32800)
/** Histogram unit statistics data size in bytes */
#define ISP5_STATS_HIST_MAX_SIZE	MK_SIZE(4144)
/** Pixel Replacement Unit (PRU) unit statistics data size in bytes */
#define ISP5_STATS_OR_MAX_SIZE		MK_SIZE(64)
/** Local Tone Mapping (LTM) unit statistics data size in bytes */
#define ISP5_STATS_LTM_MAX_SIZE		MK_SIZE(1056)

/* Stats buffer addresses muse be aligned to 64 byte (ATOM) boundaries */
#define ISP5_ALIGN_STAT_OFFSET(_offset) \
	(((uint32_t)(_offset) + MK_U32(63)) & ~(MK_U32(63)))

/** Flicker band (FB) unit statistics data offset */
#define ISP5_STATS_FB_OFFSET		MK_SIZE(0)
/** Focus Metrics (FM) unit statistics data offset */
#define ISP5_STATS_FM_OFFSET \
	(ISP5_STATS_FB_OFFSET + ISP5_ALIGN_STAT_OFFSET(ISP5_STATS_FB_MAX_SIZE))
/** Auto Focus Metrics (AFM) unit statistics data offset */
#define ISP5_STATS_AFM_OFFSET \
	(ISP5_STATS_FM_OFFSET + ISP5_ALIGN_STAT_OFFSET(ISP5_STATS_FM_MAX_SIZE))
/** Local Average Clipping (LAC0) unit statistics data offset */
#define ISP5_STATS_LAC0_OFFSET \
	(ISP5_STATS_AFM_OFFSET + \
	(ISP5_ALIGN_STAT_OFFSET(ISP5_STATS_AFM_ROI_MAX_SIZE) * MK_SIZE(8)))
/** Local Average Clipping (LAC1) unit statistics data offset */
#define ISP5_STATS_LAC1_OFFSET \
	(ISP5_STATS_LAC0_OFFSET + \
	(ISP5_ALIGN_STAT_OFFSET(ISP5_STATS_LAC_ROI_MAX_SIZE) * MK_SIZE(4)))
/** Histogram unit (H0) statistics data offset */
#define ISP5_STATS_HIST0_OFFSET \
	(ISP5_STATS_LAC1_OFFSET + \
	(ISP5_ALIGN_STAT_OFFSET(ISP5_STATS_LAC_ROI_MAX_SIZE) * MK_SIZE(4)))
/** Histogram unit (H1) statistics data offset */
#define ISP5_STATS_HIST1_OFFSET \
	(ISP5_STATS_HIST0_OFFSET + \
	ISP5_ALIGN_STAT_OFFSET(ISP5_STATS_HIST_MAX_SIZE))
/** Pixel Replacement Unit (PRU) unit statistics data offset */
#define ISP5_STATS_OR_OFFSET \
	(ISP5_STATS_HIST1_OFFSET + \
	ISP5_ALIGN_STAT_OFFSET(ISP5_STATS_HIST_MAX_SIZE))
/** Local Tone Mapping (LTM) unit statistics data offset */
#define ISP5_STATS_LTM_OFFSET \
	(ISP5_STATS_OR_OFFSET + \
	ISP5_ALIGN_STAT_OFFSET(ISP5_STATS_OR_MAX_SIZE))
/** Total statistics data size in bytes */
#define ISP5_STATS_TOTAL_SIZE \
	(ISP5_STATS_LTM_OFFSET + ISP5_STATS_LTM_MAX_SIZE)
/**@}*/

#define ISP_NUM_GOS_TABLES	MK_U32(8)

#define VI_NUM_GOS_TABLES	MK_U32(12)
#define VI_NUM_ATOMP_SURFACES	4
#define VI_NUM_STATUS_SURFACES	1
#define VI_NUM_VI_PFSD_SURFACES	2

/**
 * @defgroup ViAtompSurface VI ATOMP surface related defines
 * @{
 */
/** Output surface plane 0 */
#define VI_ATOMP_SURFACE0	0
/** Output surface plane 1 */
#define VI_ATOMP_SURFACE1	1
/** Output surface plane 2 */
#define VI_ATOMP_SURFACE2	2

/** Sensor embedded data */
#define VI_ATOMP_SURFACE_EMBEDDED 3

/** RAW pixels */
#define VI_ATOMP_SURFACE_MAIN	VI_ATOMP_SURFACE0
/** PDAF pixels */
#define VI_ATOMP_SURFACE_PDAF	VI_ATOMP_SURFACE1

/** YUV - Luma plane */
#define VI_ATOMP_SURFACE_Y	VI_ATOMP_SURFACE0
/** Semi-planar - UV plane */
#define	VI_ATOMP_SURFACE_UV	VI_ATOMP_SURFACE1
/** Planar - U plane */
#define	VI_ATOMP_SURFACE_U	VI_ATOMP_SURFACE1
/** Planar - V plane */
#define	VI_ATOMP_SURFACE_V	VI_ATOMP_SURFACE2

/** @} */

/* SLVS-EC */
#define SLVSEC_STREAM_DISABLED	MK_U8(0xFF)

/**
 * @defgroup VICaptureChannelFlags
 * VI Capture channel specific flags
 */
/**@{*/
/** Channel takes input from Video Interface (VI) */
#define CAPTURE_CHANNEL_FLAG_VIDEO		MK_U32(0x0001)
/** Channel supports RAW Bayer output */
#define CAPTURE_CHANNEL_FLAG_RAW		MK_U32(0x0002)
/** Channel supports planar YUV output */
#define CAPTURE_CHANNEL_FLAG_PLANAR		MK_U32(0x0004)
/** Channel supports semi-planar YUV output */
#define CAPTURE_CHANNEL_FLAG_SEMI_PLANAR	MK_U32(0x0008)
/** Channel supports phase-detection auto-focus */
#define CAPTURE_CHANNEL_FLAG_PDAF		MK_U32(0x0010)
/** Channel outputs to Focus Metric Lite module (FML) */
#define CAPTURE_CHANNEL_FLAG_FMLITE		MK_U32(0x0020)
/** Channel outputs sensor embedded data */
#define CAPTURE_CHANNEL_FLAG_EMBDATA		MK_U32(0x0040)
/** Channel outputs to ISPA */
#define CAPTURE_CHANNEL_FLAG_ISPA		MK_U32(0x0080)
/** Channel outputs to ISPB */
#define CAPTURE_CHANNEL_FLAG_ISPB		MK_U32(0x0100)
/** Channel outputs directly to selected ISP (ISO mode) */
#define CAPTURE_CHANNEL_FLAG_ISP_DIRECT		MK_U32(0x0200)
/** Channel outputs to software ISP (reserved) */
#define CAPTURE_CHANNEL_FLAG_ISPSW		MK_U32(0x0400)
/** Channel treats all errors as stop-on-error and requires reset for recovery.*/
#define CAPTURE_CHANNEL_FLAG_RESET_ON_ERROR	MK_U32(0x0800)
/** Channel has line timer enabled */
#define CAPTURE_CHANNEL_FLAG_LINETIMER		MK_U32(0x1000)
/** Channel supports SLVSEC sensors */
#define CAPTURE_CHANNEL_FLAG_SLVSEC		MK_U32(0x2000)
/** Channel reports errors to HSM based on error_mask_correctable and error_mask_uncorrectable.*/
#define CAPTURE_CHANNEL_FLAG_ENABLE_HSM_ERROR_MASKS	MK_U32(0x4000)
/** Capture with VI PFSD enabled */
#define CAPTURE_CHANNEL_FLAG_ENABLE_VI_PFSD	MK_U32(0x8000)
/**@}*/

/**
 * @defgroup CaptureChannelErrMask
 * Bitmask for masking "Uncorrected errors" and "Errors with threshold".
 */
/**@{*/
/** VI Frame start error timeout */
#define CAPTURE_CHANNEL_ERROR_VI_FRAME_START_TIMEOUT	MK_BIT32(23)
/** VI Permanent Fault SW Diagnostics (PFSD) error */
#define CAPTURE_CHANNEL_ERROR_VI_PFSD_FAULT		MK_BIT32(22)
/** Embedded data incomplete */
#define CAPTURE_CHANNEL_ERROR_ERROR_EMBED_INCOMPLETE	MK_BIT32(21)
/** Pixel frame is incomplete */
#define CAPTURE_CHANNEL_ERROR_INCOMPLETE		MK_BIT32(20)
/** A Frame End appears from NVCSI before the normal number of pixels has appeared*/
#define CAPTURE_CHANNEL_ERROR_STALE_FRAME		MK_BIT32(19)
/** A start-of-frame matches a channel that is already in frame */
#define CAPTURE_CHANNEL_ERROR_COLLISION			MK_BIT32(18)
/** Pixels stopped, an FE was forced due to a latent LOAD event */
#define CAPTURE_CHANNEL_ERROR_FORCE_FE			MK_BIT32(17)
/** A LOAD command is received for a channel while that channel is currently in a frame.*/
#define CAPTURE_CHANNEL_ERROR_LOAD_FRAMED		MK_BIT32(16)
/** The pixel datatype changed in the middle of the line */
#define CAPTURE_CHANNEL_ERROR_DTYPE_MISMATCH		MK_BIT32(15)
/** Unexpected embedded data in frame */
#define CAPTURE_CHANNEL_ERROR_EMBED_INFRINGE		MK_BIT32(14)
/** Extra embedded bytes on line */
#define CAPTURE_CHANNEL_ERROR_EMBED_LONG_LINE		MK_BIT32(13)
/** Embedded bytes found between line start and line end*/
#define CAPTURE_CHANNEL_ERROR_EMBED_SPURIOUS		MK_BIT32(12)
/** Too many embeded lines in frame */
#define CAPTURE_CHANNEL_ERROR_EMBED_RUNAWAY		MK_BIT32(11)
/** Two embedded line starts without a line end in between */
#define CAPTURE_CHANNEL_ERROR_EMBED_MISSING_LE		MK_BIT32(10)
/** A line has fewer pixels than expected width */
#define CAPTURE_CHANNEL_ERROR_PIXEL_SHORT_LINE		MK_BIT32(9)
/** A line has more pixels than expected width, pixels dropped */
#define CAPTURE_CHANNEL_ERROR_PIXEL_LONG_LINE		MK_BIT32(8)
/** A pixel found between line end and line start markers, dropped */
#define CAPTURE_CHANNEL_ERROR_PIXEL_SPURIOUS		MK_BIT32(7)
/** Too many pixel lines in frame, extra lines dropped */
#define CAPTURE_CHANNEL_ERROR_PIXEL_RUNAWAY		MK_BIT32(6)
/** Two lines starts without a line end in between */
#define CAPTURE_CHANNEL_ERROR_PIXEL_MISSING_LE		MK_BIT32(5)
/**@}*/

/**
 * @brief Describes RTCPU side resources for a capture pipe-line.
 */
struct capture_channel_config {
	/**
	 * A bitmask describing the set of non-shareable
	 * HW resources that the capture channel will need. These HW resources
	 * will be assigned to the new capture channel and will be owned by the
	 * channel until it is released with CAPTURE_CHANNEL_RELEASE_REQ.
	 *
	 * The HW resources that can be assigned to a channel include a VI
	 * channel, ISPBUF A/B interface (T18x only), Focus Metric Lite module (FML).
	 *
	 * VI channels can have different capabilities. The flags are checked
	 * against the VI channel capabilities to make sure the allocated VI
	 * channel meets the requirements.
	 *
	 * See @ref VICaptureChannelFlags "Capture Channel Flags".
	 */
	uint32_t channel_flags;

	/** rtcpu internal data field - Should be set to zero */
	uint32_t channel_id;
	/**
	 * A bit mask indicating which VI channels to consider for allocation.
	 * This allows the client to enforce allocation of HW VI channel in particular range for its own
	 * purpose.
	 *
	 * Beware that client VM may have restricted range of available VI channels.
	 *
	 * In most of the cases client can set to ~0ULL to let RTCPU to allocate any available channel
	 * permitted for client VM.
	 *
	 * This mask is expected to be useful for following use-cases:
	 * 1. Debugging functionality of particular HW VI channel.
	 * 2. Verify that RTCPU enforces VI channel permissions defined in VM DT.
	 */
	uint64_t vi_channel_mask;
	/**
	 * Base address of a memory mapped ring buffer containing capture requests.
	 * The size of the buffer is queue_depth * request_size
	 */
	iova_t requests;
	/**
	 * Maximum number of capture requests in the requests queue.
	 * Determines the size of the ring buffer.
	 */
	uint32_t queue_depth;
	/** Size of the buffer reserved for each capture request. */
	uint32_t request_size;

	/** SLVS-EC main stream */
	uint8_t slvsec_stream_main;
	/** SLVS-EC sub stream */
	uint8_t slvsec_stream_sub;
	/** Reserved */
	uint16_t reserved1;

#define HAVE_VI_GOS_TABLES
	/**
	 * GoS tables can only be programmed when there are no
	 * active channels. For subsequent channels we check that
	 * the channel configuration matches with the active
	 * configuration.
	 *
	 * Number of Grid of Semaphores (GOS) tables
	 */
	uint32_t num_vi_gos_tables;
	/** VI GOS tables */
	iova_t vi_gos_tables[VI_NUM_GOS_TABLES];

	/** Capture progress syncpoint info */
	struct syncpoint_info progress_sp;
	/** Embedded data syncpoint info */
	struct syncpoint_info embdata_sp;
	/** VI line timer syncpoint info */
	struct syncpoint_info linetimer_sp;

	/**
	 * User-defined HSM error reporting policy is specified by error masks bits
	 *
	 * CAPTURE_CHANNEL_FLAG_ENABLE_HSM_ERROR_MASKS must be set to enable these error masks,
	 * otherwise default HSM reporting policy is used.
	 *
	 * VI-falcon reports error to EC/HSM as uncorrected if error is not masked
	 * in "Uncorrected" mask.
	 * VI-falcon reports error to EC/HSM as corrected if error is masked
	 * in "Uncorrected" mask and not masked in "Errors with threshold" mask.
	 * VI-falcon does not report error to EC/HSM if error masked
	 * in both "Uncorrected" and "Errors with threshold" masks.
	 */
	/**
	 * Error mask for "uncorrected" errors. See @ref CaptureChannelErrMask "Channel Error bitmask".
	 * There map to the uncorrected error line in HSM
	 */
	uint32_t error_mask_uncorrectable;
	/**
	 * Error mask for "errors with threshold".
	 * See @ref CaptureChannelErrMask "Channel Error bitmask".
	 * These map to the corrected error line in HSM */
	uint32_t error_mask_correctable;

	/**
	 * Capture will stop for errors selected in this bit masks.
	 * Bit definitions are same as in CAPTURE_STATUS_NOTIFY_BIT_* macros.
	 */
	uint64_t stop_on_error_notify_bits;

} CAPTURE_IVC_ALIGN;

/**
 * @brief VI Channel configuration
 *
 * VI unit register programming for capturing a frame.
 */
struct vi_channel_config {
	/** DT override enabled flag */
	unsigned dt_enable:1;
	/** Embedded data enabled flag */
	unsigned embdata_enable:1;
	/** Flush notice enabled flag */
	unsigned flush_enable:1;
	/** Periodic flush notice enabled flag */
	unsigned flush_periodic:1;
	/** Line timer enabled flag */
	unsigned line_timer_enable:1;
	/** Periodic line timer notice enabled flag */
	unsigned line_timer_periodic:1;
	/** Enable PIXFMT writing pixels flag */
	unsigned pixfmt_enable:1;
	/** Flag to enable merging adjacent RAW8/RAW10 pixels */
	unsigned pixfmt_wide_enable:1;
	/** Flag to enable big or little endian. 0 - Big Endian, 1 - Little Endian */
	unsigned pixfmt_wide_endian:1;
	/** Flag to enable Phase Detection Auto Focus (PDAF) pixel replacement */
	unsigned pixfmt_pdaf_replace_enable:1;
	/** ISPA buffer enabled */
	unsigned ispbufa_enable:1;
	/** ISPB buffer enabled. Not valid for T186 & T194 */
	unsigned ispbufb_enable:1;
	/** FM lite unit enable flag */
	unsigned fmlite_enable:1;
	/** VI Companding module enable flag */
	unsigned compand_enable:1;
	/** Reserved bits */
	unsigned __pad_flags:18;

	/* VI channel selector */
	struct match_rec {
		/** Datatype to be sent to the channel */
		uint8_t datatype;
		/** Bits of datatype to match on */
		uint8_t datatype_mask;
		/** CSIMUX source to send to this channel */
		uint8_t stream;
		/** Bits of STREAM to match on. */
		uint8_t stream_mask;
		/** Virtual channel to be sent to this channel */
		uint16_t vc;
		/** Bits of VIRTUAL_CHANNEL_MASK to match on */
		uint16_t vc_mask;
		/** Frame id to be sent to this channel */
		uint16_t frameid;
		/** Bits of FRAME_ID to match on. */
		uint16_t frameid_mask;
		/** Data in the first pixel of a line to match on */
		uint16_t dol;
		/** Bits of DOL to match on */
		uint16_t dol_mask;
	} match;

	/** DOL header select */
	uint8_t dol_header_sel;
	/** Data type override */
	uint8_t dt_override;
	/** DPCM mode to be used. Currently DPCM is not used */
	uint8_t dpcm_mode;
	/** Reserved */
	uint8_t __pad_dol_dt_dpcm;

	struct vi_frame_config {
		/** Pixel width of frame before cropping */
		uint16_t frame_x;
		/** Line height of frame */
		uint16_t frame_y;
		/** Maximum number of embedded data bytes on a line */
		uint32_t embed_x;
		/** Number of embedded lines in frame */
		uint32_t embed_y;
		struct skip_rec {
			/**
			 * Number of packets to skip on output at start of line.
			 * Counted in groups of 8 pixels
			 */
			uint16_t x;
			/** Number of lines to skip at top of the frame */
			uint16_t y;
		} skip;
		struct crop_rec {
			/** Line width in pixels after which no packets will be transmitted */
			uint16_t x;
			/** Height in lines after which no lines will be transmitted */
			uint16_t y;
		} crop;
	} frame;

	/** Pixel line count at which a flush notice is sent out */
	uint16_t flush;
	/** Line count at which to trip the first flush event */
	uint16_t flush_first;

	/** Pixel line count at which a notification is sent out*/
	uint16_t line_timer;
	/** Line count at which to trip the first line timer event */
	uint16_t line_timer_first;

	/* Pixel formatter */
	struct pixfmt_rec {
		/** Pixel memory format for the VI channel */
		uint16_t format;
		/** Reserved */
		uint16_t __pad;
		struct pdaf_rec {
			/** Within a line, X pixel position at which PDAF separation begins */
			uint16_t crop_left;
			/** Within a line, X pixel position at which PDAF separation ends*/
			uint16_t crop_right;
			/** Line at which PDAF separation begins */
			uint16_t crop_top;
			/** line at which PDAF separation ends */
			uint16_t crop_bottom;
			/** Within a line, X pixel position at which PDAF replacement begins*/
			uint16_t replace_crop_left;
			/** Within a line, X pixel position at which PDAF replacement ends*/
			uint16_t replace_crop_right;
			/** Line at which PDAF replacement begins */
			uint16_t replace_crop_top;
			/** Line at which PDAF repalcement ends */
			uint16_t replace_crop_bottom;
			/** X coordinate of last PDAF pixel within the PDAF crop window */
			uint16_t last_pixel_x;
			/** Y coordinate of last PDAF pixel within the PDAF crop window */
			uint16_t last_pixel_y;
			/** Value to replace PDAF pixel with */
			uint16_t replace_value;
			/** Memory format in which the PDAF pixels will be written in */
			uint8_t format;
			/** Reserved */
			uint8_t __pad_pdaf;
		} pdaf;
	} pixfmt;

	/* Pixel DPCM */
	struct dpcm_rec {
		/** Number of pixels in the strip */
		uint16_t strip_width;
		/** Number of packets in overfetch region */
		uint16_t strip_overfetch;

		/** Not for T186 or earlier */
		/** Number of packets in first generated chunk (no OVERFETCH region in first chunk) */
		uint16_t chunk_first;
		/** Number of packets in “body” chunks (including OVERFETCH region, if enabled) */
		uint16_t chunk_body;
		/** Number of “body” chunks to emit */
		uint16_t chunk_body_count;
		/**
		 * Number of packets in chunk immediately after “body” chunks (including OVERFETCH
		 * region, if enabled)
		 */
		uint16_t chunk_penultimate;
		/** Number of packets in final generated chunk (including OVERFETCH region, if enabled) */
		uint16_t chunk_last;
		/** Reserved */
		uint16_t __pad;
		/** Maximum value to truncate input data to */
		uint32_t clamp_high;
		/** Minimum value to truncate input data to */
		uint32_t clamp_low;
	} dpcm;

	/* Atom packer */
	struct atomp_rec {
		struct surface_rec {
			/** Lower 32-bits of the surface base address */
			uint32_t offset;
			/** Lower 8-bits of the surface base address */
			uint32_t offset_hi;
		} surface[VI_NUM_ATOMP_SURFACES];
		/** Line stride of the surface in bytes */
		uint32_t surface_stride[VI_NUM_ATOMP_SURFACES];
		/** DPCM chunk stride (distance from start of chunk to end of chunk) */
		uint32_t dpcm_chunk_stride;
	} atomp;

	/** Reserved */
	uint16_t __pad[2];

} CAPTURE_IVC_ALIGN;

/**
 * @brief Engine status buffer base address.
 */
struct engine_status_surface {
	/** Lower 32-bits of the surface base address */
	uint32_t offset;
	/** Upper 8-bits of the surface base address */
	uint32_t offset_hi;
} CAPTURE_IVC_ALIGN;

/**
 * @brief NVCSI error status
 *
 * Represents error reported from CSI source used by capture descriptor.
 *
 */
struct nvcsi_error_status {
	/**
	 * NVCSI @ref NvCsiStreamErrors "errors" reported for stream used by capture descriptor
	 *
	 * Stream error affects multiple virtual channel.
	 * It will be is reported only once, for the first capture channel
	 * which retrieved the error report.
	 *
	 * This error cause data packet drops and should trigger VI errors in
	 * affected virtual channels.
	 */
	uint32_t nvcsi_stream_bits;
	/**
	 * @name NvCsiStreamErrors
	 * NVCSI Stream error bits
 	 * @defgroup NvCsiStreamErrors NVCSI Stream error bits
	 */
	/** @{ */
#define NVCSI_STREAM_ERR_STAT_PH_BOTH_CRC_ERR			MK_BIT32(1)
#define NVCSI_STREAM_ERR_STAT_PH_ECC_MULTI_BIT_ERR		MK_BIT32(0)
	/** @} */

	/**
	 * NVCSI @ref NvcsiVirtualChannelErrors "errors" reported for stream virtual channel used by capture descriptor
	 * These errors are expected to be forwarded to VI and also reported by VI as CSIMUX Frame CSI_FAULT errors
	 */
	uint32_t nvcsi_virtual_channel_bits;
	/**
	 * @name NvCsiVirtualChannelErrors
	 * NVCSI Virtual Channel error bits
 	 * @defgroup NvCsiVirtualChannelErrors NVCSI Virtual Channel error bits
	 */
	/** @{ */
#define NVCSI_VC_ERR_INTR_STAT_PH_SINGLE_CRC_ERR_VC0		MK_BIT32(4)
#define NVCSI_VC_ERR_INTR_STAT_PD_WC_SHORT_ERR_VC0		MK_BIT32(3)
#define NVCSI_VC_ERR_INTR_STAT_PD_CRC_ERR_VC0			MK_BIT32(2)
#define NVCSI_VC_ERR_INTR_STAT_PH_ECC_SINGLE_BIT_ERR_VC0	MK_BIT32(1)
#define NVCSI_VC_ERR_INTR_STAT_PPFSM_TIMEOUT_VC0		MK_BIT32(0)
	/** @} */

	/**
	 * NVCSI errors reported for CIL interface used by capture descriptor
	 */
	/** NVCSI CIL A  @ref NvCsiCilErrors "errors" */
	uint32_t cil_a_error_bits;
	/** NVCSI CIL B  @ref NvCsiCilErrors "errors" */
	uint32_t cil_b_error_bits;
	/**
	 * @name NvCsiCilErrors
	 * NVCSI CIL error bits
 	 * @defgroup NvCsiCilErrors NVCSI CIL error bits
	 */
	/** @{ */
#define NVCSI_ERR_CIL_DATA_LANE_SOT_2LSB_ERR1		MK_BIT32(16)
#define NVCSI_ERR_CIL_DATA_LANE_SOT_2LSB_ERR0		MK_BIT32(15)
#define NVCSI_ERR_CIL_DATA_LANE_ESC_MODE_SYNC_ERR1	MK_BIT32(14)
#define NVCSI_ERR_CIL_DATA_LANE_ESC_MODE_SYNC_ERR0	MK_BIT32(13)
#define NVCSI_ERR_DPHY_CIL_LANE_ALIGN_ERR		MK_BIT32(12)
#define NVCSI_ERR_DPHY_CIL_DESKEW_CALIB_ERR_CTRL	MK_BIT32(11)
#define NVCSI_ERR_DPHY_CIL_DESKEW_CALIB_ERR_LANE1	MK_BIT32(10)
#define NVCSI_ERR_DPHY_CIL_DESKEW_CALIB_ERR_LANE0	MK_BIT32(9)
#define NVCSI_ERR_CIL_DATA_LANE_RXFIFO_FULL_ERR1	MK_BIT32(8)
#define NVCSI_ERR_CIL_DATA_LANE_CTRL_ERR1		MK_BIT32(7)
#define NVCSI_ERR_CIL_DATA_LANE_SOT_MB_ERR1		MK_BIT32(6)
#define NVCSI_ERR_CIL_DATA_LANE_SOT_SB_ERR1		MK_BIT32(5)
#define NVCSI_ERR_CIL_DATA_LANE_RXFIFO_FULL_ERR0	MK_BIT32(4)
#define NVCSI_ERR_CIL_DATA_LANE_CTRL_ERR0		MK_BIT32(3)
#define NVCSI_ERR_CIL_DATA_LANE_SOT_MB_ERR0             MK_BIT32(2)
#define NVCSI_ERR_CIL_DATA_LANE_SOT_SB_ERR0		MK_BIT32(1)
#define NVCSI_ERR_DPHY_CIL_CLK_LANE_CTRL_ERR		MK_BIT32(0)
	/** @} */
};


/**
 * @brief Frame capture status record
 */
struct capture_status {
	/** CSI stream number */
	uint8_t src_stream;
	/** CSI virtual channel number */
	uint8_t virtual_channel;
	/** Frame sequence number */
	uint16_t frame_id;
	/** Capture status @ref CaptureStatusCodes "codes" */
	uint32_t status;
/**
 * @name CaptureStatusCodes
 * Capture status codes.
 * @defgroup CaptureStatusCodes Capture status codes.
 */
/** @{ */
/** Capture status unknown */
#define CAPTURE_STATUS_UNKNOWN			MK_U32(0)
/** Capture status success */
#define CAPTURE_STATUS_SUCCESS			MK_U32(1)
/** Csimux frame error */
#define CAPTURE_STATUS_CSIMUX_FRAME		MK_U32(2)
/** Csimux stream error */
#define CAPTURE_STATUS_CSIMUX_STREAM		MK_U32(3)
/** Data-specific fault in a channel */
#define CAPTURE_STATUS_CHANSEL_FAULT		MK_U32(4)
/** Data-specific fault in a channel. FE packet was force inserted.*/
#define CAPTURE_STATUS_CHANSEL_FAULT_FE		MK_U32(5)
/** SOF matches a channel that is already in a frame */
#define CAPTURE_STATUS_CHANSEL_COLLISION	MK_U32(6)
/** Frame End appears from NVCSI before the normal number of pixels has appeared */
#define CAPTURE_STATUS_CHANSEL_SHORT_FRAME	MK_U32(7)
/** Single surface packer has overflowed */
#define CAPTURE_STATUS_ATOMP_PACKER_OVERFLOW	MK_U32(8)
/** Frame interrupted mid-frame */
#define CAPTURE_STATUS_ATOMP_FRAME_TRUNCATED	MK_U32(9)
/** Frame interrupted without writing any data out */
#define CAPTURE_STATUS_ATOMP_FRAME_TOSSED	MK_U32(10)
/** ISP buffer FIFO overflowed */
#define CAPTURE_STATUS_ISPBUF_FIFO_OVERFLOW	MK_U32(11)
/** Capture status out of sync */
#define CAPTURE_STATUS_SYNC_FAILURE		MK_U32(12)
/** VI notified backend down */
#define CAPTURE_STATUS_NOTIFIER_BACKEND_DOWN	MK_U32(13)
/** Falcon error */
#define CAPTURE_STATUS_FALCON_ERROR		MK_U32(14)
/** Data does not match any active channel */
#define CAPTURE_STATUS_CHANSEL_NOMATCH		MK_U32(15)
/** @} */
	/** Start of Frame (SOF) timestamp */
	uint64_t sof_timestamp;
	/** End of Frame (EOF) timestamp */
	uint64_t eof_timestamp;
	/** Falcon error data */
	uint32_t err_data;

	/** Channel encountered unrecoverable error and must be reset */
#define CAPTURE_STATUS_FLAG_CHANNEL_IN_ERROR			MK_BIT32(1)
	uint32_t flags;

	/**
	 * VI error notifications logged in capture channel since previous capture
	 * (for more information refer to T194_VI5_Notify define_change.xlsx)
	 *
	 * Also see @ref ViNotifyErrorTag "VI notify error bitmask"
	 */
	uint64_t notify_bits;

	/**
	 * @name ViNotifyErrorTag
	 * VI notify error bitmask
	 */
	/** @{ */
	/** CSIMUX Frame (tag 0x2) notifications */
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_FRAME_RESEVED_0			MK_BIT64(1)
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_FRAME_FS_FAULT				MK_BIT64(2)
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_FRAME_FORCE_FE_FAULT			MK_BIT64(3)
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_FRAME_FE_FRAME_ID_FAULT		MK_BIT64(4)
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_FRAME_PXL_ENABLE_FAULT			MK_BIT64(5)

	/** Reserved for deinterleaved CSI streams on request from nvmedia team */
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_FRAME_RESERVED_1			MK_BIT64(6)
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_FRAME_RESERVED_2			MK_BIT64(7)
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_FRAME_RESERVED_3			MK_BIT64(8)
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_FRAME_RESERVED_4			MK_BIT64(9)
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_FRAME_RESERVED_5			MK_BIT64(10)
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_FRAME_RESERVED_6			MK_BIT64(11)
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_FRAME_RESERVED_7			MK_BIT64(12)
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_FRAME_RESERVED_8			MK_BIT64(13)
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_FRAME_RESERVED_9			MK_BIT64(14)

	/** CSI Faults. These errors report corresponding NVCSI errors */
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_FRAME_CSI_FAULT_PPFSM_TIMEOUT		MK_BIT64(15)
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_FRAME_CSI_FAULT_PH_ECC_SINGLE_BIT_ERR	MK_BIT64(16)
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_FRAME_CSI_FAULT_PD_CRC_ERR		MK_BIT64(17)
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_FRAME_CSI_FAULT_PD_WC_SHORT_ERR	MK_BIT64(18)
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_FRAME_CSI_FAULT_PH_SINGLE_CRC_ERR	MK_BIT64(19)
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_FRAME_CSI_FAULT_EMBEDDED_LINE_CRC_ERR	MK_BIT64(20)

	/**
	 * CSIMUX Stream (tag 0x3) notifications
	 */
	/**
	 * Spurious data was received before frame start.
	 * Can be badly corrupted frame or some random bits.
	 * This error doesn't have effect on captured frame
	 */
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_STREAM_SPURIOUS_DATA			MK_BIT64(21)

	/** Uncorrectable FIFO errors */
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_STREAM_FIFO_OVERFLOW			MK_BIT64(22)
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_STREAM_FIFO_LOF			MK_BIT64(23)

	/**
	 * Illegal data packet was encountered and dropped by CSIMUX.
	 * This error may have no effect on capture result or trigger other error if
	 * frame got corrupted.
	 */
	#define CAPTURE_STATUS_NOTIFY_BIT_CSIMUX_STREAM_FIFO_BADPKT			MK_BIT64(24)

	/**
	 * Timeout from frame descriptor activation to frame start.
	 * See also frame_start_timeout in struct capture_descriptor
	 */
	#define CAPTURE_STATUS_NOTIFY_BIT_FRAME_START_TIMEOUT			MK_BIT64(25)

	/**
	 * Timeout from frame start to frame completion.
	 * See also frame_completion_timeout in struct capture_descriptor
	 */
	#define CAPTURE_STATUS_NOTIFY_BIT_FRAME_COMPLETION_TIMEOUT		MK_BIT64(26)

	/**
	 * CHANSEL FAULT (TAG 0x9) Notifications
	 */
	#define CAPTURE_STATUS_NOTIFY_BIT_CHANSEL_PIXEL_MISSING_LE		MK_BIT64(30)
	#define CAPTURE_STATUS_NOTIFY_BIT_CHANSEL_PIXEL_RUNAWAY			MK_BIT64(31)
	#define CAPTURE_STATUS_NOTIFY_BIT_CHANSEL_PIXEL_SPURIOUS		MK_BIT64(32)
	#define CAPTURE_STATUS_NOTIFY_BIT_CHANSEL_PIXEL_LONG_LINE		MK_BIT64(33)
	#define CAPTURE_STATUS_NOTIFY_BIT_CHANSEL_PIXEL_SHORT_LINE		MK_BIT64(34)
	#define CAPTURE_STATUS_NOTIFY_BIT_CHANSEL_EMBED_MISSING_LE		MK_BIT64(35)
	#define CAPTURE_STATUS_NOTIFY_BIT_CHANSEL_EMBED_RUNAWAY			MK_BIT64(36)
	#define CAPTURE_STATUS_NOTIFY_BIT_CHANSEL_EMBED_SPURIOUS		MK_BIT64(37)
	#define CAPTURE_STATUS_NOTIFY_BIT_CHANSEL_EMBED_LONG_LINE		MK_BIT64(38)
	#define CAPTURE_STATUS_NOTIFY_BIT_CHANSEL_EMBED_INFRINGE		MK_BIT64(39)
	#define CAPTURE_STATUS_NOTIFY_BIT_CHANSEL_DTYPE_MISMATCH		MK_BIT64(40)
	#define CAPTURE_STATUS_NOTIFY_BIT_CHANSEL_RESERVED_0			MK_BIT64(41)

	/**
	 * CHANSEL PIX_SHORT (TAG 0xD) Notifications
	 */
	#define CAPTURE_STATUS_NOTIFY_BIT_CHANSEL_PIX_SHORT			MK_BIT64(42)

	/**
	 * CHANSEL EMB_SHORT (TAG 0xD) Notification.
	 */
	#define CAPTURE_STATUS_NOTIFY_BIT_CHANSEL_EMB_SHORT			MK_BIT64(43)

	/** Permanent Fault Software Diagnostics (PFSD) */
	#define CAPTURE_STATUS_NOTIFY_BIT_PFSD_FAULT				MK_BIT64(44)

	/**
	 * CHANSEL FAULT_FE (TAG 0xA) Notification
	 */
	#define CAPTURE_STATUS_NOTIFY_BIT_CHANSEL_FAULT_FE			MK_BIT64(45)

	/**
	 * CHANSEL NOMATCH (TAG 0xB) Notification
	 * One or more frames from CSI could not be matched with capture descriptors enqueued in VI.
	 * This error is usually caused by missing capture descriptor.
	 * This error doesn't have effect on next captured frame
	 */
	#define CAPTURE_STATUS_NOTIFY_BIT_CHANSEL_NO_MATCH			MK_BIT64(46)

	/**
	 * CHANSEL COLLISION (TAG 0xC) Notification
	 */
	#define CAPTURE_STATUS_NOTIFY_BIT_CHANSEL_COLLISION			MK_BIT64(47)

	/**
	 * CHANSEL LOAD_FRAMED (TAG 0xE) Notification
	 */
	#define CAPTURE_STATUS_NOTIFY_BIT_CHANSEL_LOAD_FRAMED			MK_BIT64(48)

	/** ATOMP_PACKER_OVERFLOW   (TAG 0xf) */
	#define CAPTURE_STATUS_NOTIFY_BIT_ATOMP_PACKER_OVERFLOW			MK_BIT64(49)

	/** ATOMP_FRAME_TRUNCATED   (TAG 0x15) Frame not finished */
	#define CAPTURE_STATUS_NOTIFY_BIT_ATOMP_FRAME_TRUNCATED			MK_BIT64(50)

	/** ATOMP_FRAME_TOSSED   (TAG 0x16) Frame data not written */
	#define CAPTURE_STATUS_NOTIFY_BIT_ATOMP_FRAME_TOSSED			MK_BIT64(51)

	/** Non-classified error */
	#define CAPTURE_STATUS_NOTIFY_BIT_NON_CLASSIFIED_0			MK_BIT64(63)
	/** @} */

	/**
	 * NVCSI error status.
	 *
	 * Error bits representing errors which were reported by NVCSI since
	 * previous capture.
	 *
	 * Multiple errors of same kind are collated into single bit.
	 *
	 * NVCSI error status is likely, but not guaranteed to affect current frame:
	 *
	 * 1. NVCSI error status is retrieved at end-of-frame VI event. NVCSI may already
	 * retrieve next frame data at this time.
	 *
	 * 2. NVCSI Error may also indicate error from older CSI data if there were frame
	 * skips between captures.
	 *
	 */
	struct nvcsi_error_status nvcsi_err_status;

} CAPTURE_IVC_ALIGN;

/**
 * @brief The compand configuration describes a piece-wise linear
 * tranformation function used by the VI companding module.
 */
#define VI_NUM_COMPAND_KNEEPTS MK_SIZE(10)
struct vi_compand_config {
	/** Input position for this knee point */
	uint32_t base[VI_NUM_COMPAND_KNEEPTS];
	/** Scale above this knee point */
	uint32_t scale[VI_NUM_COMPAND_KNEEPTS];
	/** Output offset for this knee point */
	uint32_t offset[VI_NUM_COMPAND_KNEEPTS];
} CAPTURE_IVC_ALIGN;

/** FM-Lite unit, PDAF, Syncgen units are currently not used in T194 */
#define VI_AFM_NUM_ROI			MK_SIZE(8)
#define VI_AFM_NUM_TRANSFER_KNOTS	MK_SIZE(11)

/**
 * @brief Focus Metrics lite (FMLite) unit configuration
 */
struct vi_fmlite_config {
	/** Atomically load the FM configuration from shadow registers to active registers */
	uint32_t vfm_prog;
	/** Control register */
	uint32_t vfm_ctrl;
	/** Black level for long and short exposure */
	uint32_t vfm_black_level;
	/** HDR sample map for long and short exposure */
	uint32_t vfm_hdr_sample_map;
	/** HDR scale for long and short exposure */
	uint32_t vfm_hdr_scale;
	/** Saturation for long and short exposure */
	uint32_t vfm_hdr_sat;
	/** Horizontal Increment value. Defined as (2^20 * SOURCE_WIDTH) / DEST_WIDTH */
	uint32_t vfm_h_pi;
	/** Vertical Phase Increment_value. Defined as (2^20 * SOURCE_HEIGHT) / DEST_HEIGHT */
	uint32_t vfm_v_pi;
	/** Horizontal down-scaling cropping parameter */
	uint32_t vfm_offset;
	/** Destination image framing */
	uint32_t vfm_size;
	/** Horizontal Scaler filter C0 & C1 coefficient */
	uint32_t vfm_hf_c0;
	/** Horizontal Scaler filter C2 coefficient */
	uint32_t vfm_hf_c1;
	/** Horizontal Scaler filter b0, b1, & b2 coefficient */
	uint32_t vfm_hf_c2;
	/** Verical Scaler filter a0 & a1 mantissa and exponent */
	uint32_t vfm_vf_c0;
	/** Verical Scaler filter a2 mantissa and exponent */
	uint32_t vfm_vf_c1;
	/** Verical Scaler filter b0, b1, & b2 coefficient */
	uint32_t vfm_vf_c2;
	/** Verical Scaler filter b0(minus), b1(minus), & b2(minus) coefficient */
	uint32_t vfm_vf_c3;
	/** Verical Scaler filter b0(plus), b1(plus), & b2(plus) coefficient */
	uint32_t vfm_vf_c4;
	/** Control register */
	uint32_t ctrl;
	/** Color register */
	uint32_t color;
	/** Transfer slope */
	uint32_t transfer_slope;
	/** Horizontal scale & placement of the spline interpolation */
	uint32_t transfer_x;
	/** Vertical scale & placement of the spline interpolation */
	uint32_t transfer_y;
	/** Enables cubic spline for low and high inputs */
	uint32_t transfer_cubic_ctrl;
	/** Transfer knots values, maximum +1.0 */
	uint32_t transfer_knots[VI_AFM_NUM_TRANSFER_KNOTS];
	/** 8 ROI region */
	uint32_t roi_pos[VI_AFM_NUM_ROI];
	/** 8 ROI region size */
	uint32_t roi_size[VI_AFM_NUM_ROI];
	/** Trapezoid envelope function enable for each ROI */
	uint32_t trap_en;
	/** Trapezoid horizontal down counter origin for each ROI */
	uint32_t hstart[VI_AFM_NUM_ROI];
	/** Trapezoid vertical down counter origin for each ROI */
	uint32_t vstart[VI_AFM_NUM_ROI];
	/** Trapezoid slope for each ROI */
	uint32_t slope[VI_AFM_NUM_ROI];
	/** Convolution matrix coefficients 0 and 1 */
	uint32_t coeff01;
	/** Convolution matrix coefficients 2 and 3 */
	uint32_t coeff23;
	/** Convolution matrix coefficients 4 and 5 */
	uint32_t coeff45;
	/** FMLite error status */
	uint32_t error;
} CAPTURE_IVC_ALIGN;

/**
 * @brief Focus Metrics lite (FMLite) unit result
 *
 * Total size is 72 bytes
 */
struct vi_fmlite_result {
	/** Error status */
	uint32_t error;
	/** Reserved */
	uint32_t __pad;
	/** 8 ROI region */
	uint64_t roi[VI_AFM_NUM_ROI];
} CAPTURE_IVC_ALIGN;

/*
 * @brief VI Phase Detection Auto Focus (PDAF) configuration
 *
 * The PDAF data consists of special pixels that will be extracted from a frame
 * and written to a separate surface. The PDAF pattern is shared by all capture channels
 * and should be configured before enabling PDAF pixel extraction for a specific capture.
 *
 * Pixel { x, y } will be ouput to the PDAF surface (surface1) if the
 * bit at position (x % 32) in pattern[y % 32] is set.
 *
 * Pixel { x, y } in the main output surface (surface0) will be
 * replaced by a default pixel value if the bit at position (x % 32)
 * in pattern_replace[y % 32] is set.
 */
#define VI_PDAF_PATTERN_SIZE 32
struct vi_pdaf_config {
	/**
	 * Pixel bitmap, by line PATTERN[y0][x0] is set if the pixel (x % 32) == x0, (y % 32) == y0
	 * should be output to the PDAF surface
	 */
	uint32_t pattern[VI_PDAF_PATTERN_SIZE];
	/**
	 * Pixel bitmap to be used for Replace the pdaf pixel, by line
	 * PATTERN_REPLACE[y0][x0] is set if the pixel (x % 32) == x0, (y % 32) == y0
	 * should be output to the PDAF surface
	 */
	uint32_t pattern_replace[VI_PDAF_PATTERN_SIZE];
} CAPTURE_IVC_ALIGN;

/*
 * @brief VI SYNCGEN unit configuration.
 */
struct vi_syncgen_config {
	/**
	 * Half cycle - Unsigned floating point.
	 * Decimal point position is given by FRAC_BITS in HCLK_DIV_FMT.
	 * Frequency of HCLK = SYNCGEN_CLK / (HALF_CYCLE * 2)
	 */
	uint32_t hclk_div;
	/** Number of fractional bits of HALF_CYCLE */
	uint8_t hclk_div_fmt;
	/** Horizontal sync signal */
	uint8_t xhs_width;
	/** Vertical sync signal */
	uint8_t xvs_width;
	/** Cycles to delay after XVS before assert XHS */
	uint8_t xvs_to_xhs_delay;
	/** Resevred - UNUSED */
	uint16_t cvs_interval;
	/** Reserved */
	uint16_t __pad1;
	/** Reserved */
	uint32_t __pad2;
} CAPTURE_IVC_ALIGN;


/**
 * @brief VI PFSD Configuration.
 *
 * PDAF replacement function is used in PFSD mode. Pixels within ROI are replaced
 * by test pattern, and output pixels from the ROI are compared against expected
 * values.
 */
struct vi_pfsd_config {
	/**
	 * @brief Area in which the pixels are replaced with test pattern
	 *
	 * Note that all coordinates are inclusive.
	 */
	struct replace_roi_rec {
		/** left pixel column of the replacement ROI */
		uint16_t left;
		/** right pixel column of the replacement ROI (inclusive) */
		uint16_t right;
		/** top pixel row of the replacement ROI */
		uint16_t top;
		/** bottom pixel row of the replacement ROI (inclusive) */
		uint16_t bottom;
	} replace_roi;

	/** test pattern used to replace pixels within the ROI */
	uint32_t replace_value;

	/**
	 * Count of items in the @see expected array.
	 * If zero, PFSD will not be performed for this frame
	 */
	uint32_t expected_count;

	/**
	 * Array of area definitions in output surfaces that shall be verified.
	 * For YUV422 semi-planar, [0] is Y surface and [1] is UV surface.
	 */
	struct {
		/** Byte offset for the roi from beginning of the surface */
		uint32_t offset;
		/** Number of bytes that need to be read from the output surface */
		uint32_t len;
		/** Expected value. The 4 byte pattern is repeated until @see len
		 * bytes have been compared
		 */
		uint8_t value[4];
	} expected[VI_NUM_VI_PFSD_SURFACES];

} CAPTURE_IVC_ALIGN;

/**
 * @defgroup CaptureFrameFlags Captue frame specific flags
 */
/** @{ */
/** Enables capture status reporting for the channel */
#define CAPTURE_FLAG_STATUS_REPORT_ENABLE	MK_BIT32(0)
/** Enables error reporting for the channel */
#define CAPTURE_FLAG_ERROR_REPORT_ENABLE	MK_BIT32(1)
/** @} */

/**
 * @brief VI frame capture context.
 */
struct capture_descriptor {
	/** VI frame sequence number*/
	uint32_t sequence;
	/** VI capture frame specific flags. See @ref CaptureFrameFlags "Capture Frame Flags" */
	uint32_t capture_flags;
	/** Task descriptor frame start timeout in milliseconds */
	uint16_t frame_start_timeout;
	/** Task descriptor frame complete timeout in milliseconds */
	uint16_t frame_completion_timeout;

#define CAPTURE_PREFENCE_ARRAY_SIZE		2

	/** @deprecated  */
	uint32_t prefence_count CAMRTC_DEPRECATED;
	/** @deprecated */
	struct syncpoint_info prefence[CAPTURE_PREFENCE_ARRAY_SIZE] CAMRTC_DEPRECATED;

	/** VI Channel configuration */
	struct vi_channel_config ch_cfg;
	/** Focus Metrics lite (FMLite) unit configuration */
	struct vi_fmlite_config fm_cfg;
	/** VI PFSD Configuration */
	struct vi_pfsd_config pfsd_cfg;

	/** Engine result record – written by Falcon */
	struct engine_status_surface engine_status;

	/** FMLITE result – written by RCE */
	struct vi_fmlite_result fm_result;

	/** Capture result record – written by RCE */
	struct capture_status status;

	/** Reserved */
	uint32_t __pad32[12];

} CAPTURE_DESCRIPTOR_ALIGN;

/**
 * @brief - Event data used for event injection
 */
struct event_inject_msg {
	/** UMD populates with capture status events. RCE converts to reg offset */
	uint32_t tag;
	/** Timestamp of event */
	uint32_t stamp;
	/** Bits [0:31] of event data */
	uint32_t data;
	/** Bits [32:63] of event data */
	uint32_t data_ext;
};

#define VI_HSM_CHANSEL_ERROR_MASK_BIT_NOMATCH MK_U32(1)
/**
 * @brief VI EC/HSM global CHANSEL error masking
 */
struct vi_hsm_chansel_error_mask_config {
	/** "Errors with threshold" bit mask */
	uint32_t chansel_correctable_mask;
	/** "Uncorrected error" bit mask */
	uint32_t chansel_uncorrectable_mask;
} CAPTURE_IVC_ALIGN;

/**
 * NvPhy attributes
 */
/**
 * @defgroup NvPhyType NvCSI Physical stream type
 * @{
 */
#define NVPHY_TYPE_CSI		MK_U32(0)
#define NVPHY_TYPE_SLVSEC	MK_U32(1)
/**@}*/

/**
 * NVCSI attributes
 */
/**
 * @defgroup NvCsiPort NvCSI Port
 * @{
 */
#define NVCSI_PORT_A		MK_U32(0x0)
#define NVCSI_PORT_B		MK_U32(0x1)
#define NVCSI_PORT_C		MK_U32(0x2)
#define NVCSI_PORT_D		MK_U32(0x3)
#define NVCSI_PORT_E		MK_U32(0x4)
#define NVCSI_PORT_F		MK_U32(0x5)
#define NVCSI_PORT_G		MK_U32(0x6)
#define NVCSI_PORT_H		MK_U32(0x7)
#define NVCSI_PORT_UNSPECIFIED	MK_U32(0xFFFFFFFF)
/**@}*/

/**
 * @defgroup NvCsiStream NVCSI stream id
 * @{
 */
#define NVCSI_STREAM_0		MK_U32(0x0)
#define NVCSI_STREAM_1		MK_U32(0x1)
#define NVCSI_STREAM_2		MK_U32(0x2)
#define NVCSI_STREAM_3		MK_U32(0x3)
#define NVCSI_STREAM_4		MK_U32(0x4)
#define NVCSI_STREAM_5		MK_U32(0x5)
/**@}*/

/**
 * @defgroup NvCsiVirtualChannel NVCSI virtual channels
 * @{
 */
#define NVCSI_VIRTUAL_CHANNEL_0		MK_U32(0x0)
#define NVCSI_VIRTUAL_CHANNEL_1		MK_U32(0x1)
#define NVCSI_VIRTUAL_CHANNEL_2		MK_U32(0x2)
#define NVCSI_VIRTUAL_CHANNEL_3		MK_U32(0x3)
#define NVCSI_VIRTUAL_CHANNEL_4		MK_U32(0x4)
#define NVCSI_VIRTUAL_CHANNEL_5		MK_U32(0x5)
#define NVCSI_VIRTUAL_CHANNEL_6		MK_U32(0x6)
#define NVCSI_VIRTUAL_CHANNEL_7		MK_U32(0x7)
#define NVCSI_VIRTUAL_CHANNEL_8		MK_U32(0x8)
#define NVCSI_VIRTUAL_CHANNEL_9		MK_U32(0x9)
#define NVCSI_VIRTUAL_CHANNEL_10	MK_U32(0xA)
#define NVCSI_VIRTUAL_CHANNEL_11	MK_U32(0xB)
#define NVCSI_VIRTUAL_CHANNEL_12	MK_U32(0xC)
#define NVCSI_VIRTUAL_CHANNEL_13	MK_U32(0xD)
#define NVCSI_VIRTUAL_CHANNEL_14	MK_U32(0xE)
#define NVCSI_VIRTUAL_CHANNEL_15	MK_U32(0xF)
/**@}*/

/**
 * @defgroup NvCsiConfigFlags NvCSI Configuration Flags
 * @{
 */
/** NVCSI config flags */
#define NVCSI_CONFIG_FLAG_BRICK		MK_BIT32(0)
/** NVCSI config flags */
#define NVCSI_CONFIG_FLAG_CIL		MK_BIT32(1)
/** Enable user-provided error handling configuration */
#define NVCSI_CONFIG_FLAG_ERROR		MK_BIT32(2)
/**@}*/

/**
 * @brief Number of lanes/trios per brick
 */
#define NVCSI_BRICK_NUM_LANES	MK_U32(4)
/**
 * @brief Number of override exception data types
 */
#define NVCSI_NUM_NOOVERRIDE_DT	MK_U32(5)

/**
 * @defgroup NvCsiPhyType NVCSI physical types
 * @{
 */
/** NVCSI D-PHY physical layer */
#define NVCSI_PHY_TYPE_DPHY	MK_U32(0)
/** NVCSI D-PHY physical layer */
#define NVCSI_PHY_TYPE_CPHY	MK_U32(1)
/** @} */

/**
 * @defgroup NvCsiLaneSwizzle NVCSI lane swizzles
 * @{
 */
/** 00000 := A0 A1 B0 B1 -->  A0 A1 B0 B1 */
#define NVCSI_LANE_SWIZZLE_A0A1B0B1	MK_U32(0x00)
/** 00001 := A0 A1 B0 B1 -->  A0 A1 B1 B0 */
#define NVCSI_LANE_SWIZZLE_A0A1B1B0	MK_U32(0x01)
/** 00010 := A0 A1 B0 B1 -->  A0 B0 B1 A1 */
#define NVCSI_LANE_SWIZZLE_A0B0B1A1	MK_U32(0x02)
/** 00011 := A0 A1 B0 B1 -->  A0 B0 A1 B1 */
#define NVCSI_LANE_SWIZZLE_A0B0A1B1	MK_U32(0x03)
/** 00100 := A0 A1 B0 B1 -->  A0 B1 A1 B0 */
#define NVCSI_LANE_SWIZZLE_A0B1A1B0	MK_U32(0x04)
/** 00101 := A0 A1 B0 B1 -->  A0 B1 B0 A1 */
#define NVCSI_LANE_SWIZZLE_A0B1B0A1	MK_U32(0x05)
/** 00110 := A0 A1 B0 B1 -->  A1 A0 B0 B1 */
#define NVCSI_LANE_SWIZZLE_A1A0B0B1	MK_U32(0x06)
/** 00111 := A0 A1 B0 B1 -->  A1 A0 B1 B0 */
#define NVCSI_LANE_SWIZZLE_A1A0B1B0	MK_U32(0x07)
/** 01000 := A0 A1 B0 B1 -->  A1 B0 B1 A0 */
#define NVCSI_LANE_SWIZZLE_A1B0B1A0	MK_U32(0x08)
/** 01001 := A0 A1 B0 B1 -->  A1 B0 A0 B1 */
#define NVCSI_LANE_SWIZZLE_A1B0A0B1	MK_U32(0x09)
/** 01010 := A0 A1 B0 B1 -->  A1 B1 A0 B0 */
#define NVCSI_LANE_SWIZZLE_A1B1A0B0	MK_U32(0x0A)
/** 01011 := A0 A1 B0 B1 -->  A1 B1 B0 A0 */
#define NVCSI_LANE_SWIZZLE_A1B1B0A0	MK_U32(0x0B)
/** 01100 := A0 A1 B0 B1 -->  B0 A1 A0 B1 */
#define NVCSI_LANE_SWIZZLE_B0A1A0B1	MK_U32(0x0C)
/** 01101 := A0 A1 B0 B1 -->  B0 A1 B1 A0 */
#define NVCSI_LANE_SWIZZLE_B0A1B1A0	MK_U32(0x0D)
/** 01110 := A0 A1 B0 B1 -->  B0 A0 B1 A1 */
#define NVCSI_LANE_SWIZZLE_B0A0B1A1	MK_U32(0x0E)
/** 01111 := A0 A1 B0 B1 -->  B0 A0 A1 B1 */
#define NVCSI_LANE_SWIZZLE_B0A0A1B1	MK_U32(0x0F)
/** 10000 := A0 A1 B0 B1 -->  B0 B1 A1 A0 */
#define NVCSI_LANE_SWIZZLE_B0B1A1A0	MK_U32(0x10)
/** 10001 := A0 A1 B0 B1 -->  B0 B1 A0 A1 */
#define NVCSI_LANE_SWIZZLE_B0B1A0A1	MK_U32(0x11)
/** 10010 := A0 A1 B0 B1 -->  B1 A1 B0 A0 */
#define NVCSI_LANE_SWIZZLE_B1A1B0A0	MK_U32(0x12)
/** 10011 := A0 A1 B0 B1 -->  B1 A1 A0 B0 */
#define NVCSI_LANE_SWIZZLE_B1A1A0B0	MK_U32(0x13)
/** 10100 := A0 A1 B0 B1 -->  B1 B0 A0 A1 */
#define NVCSI_LANE_SWIZZLE_B1B0A0A1	MK_U32(0x14)
/** 10101 := A0 A1 B0 B1 -->  B1 B0 A1 A0 */
#define NVCSI_LANE_SWIZZLE_B1B0A1A0	MK_U32(0x15)
/** 10110 := A0 A1 B0 B1 -->  B1 A0 A1 B0 */
#define NVCSI_LANE_SWIZZLE_B1A0A1B0	MK_U32(0x16)
/** 10111 := A0 A1 B0 B1 -->  B1 A0 B0 A1 */
#define NVCSI_LANE_SWIZZLE_B1A0B0A1	MK_U32(0x17)
/** @} */

/**
 * @defgroup NvCsiDPhyPolarity NVCSI D-phy polarity
 * @{
 */
#define NVCSI_DPHY_POLARITY_NOSWAP	MK_U32(0)
#define NVCSI_DPHY_POLARITY_SWAP	MK_U32(1)
/** @} */

/**
 * @defgroup NvCsiCPhyPolarity NVCSI C-phy polarity
 * @{
 */
/* 000 := A B C --> A B C */
#define NVCSI_CPHY_POLARITY_ABC	MK_U32(0x00)
/* 001 := A B C --> A C B */
#define NVCSI_CPHY_POLARITY_ACB	MK_U32(0x01)
/* 010 := A B C --> B C A */
#define NVCSI_CPHY_POLARITY_BCA	MK_U32(0x02)
/* 011 := A B C --> B A C */
#define NVCSI_CPHY_POLARITY_BAC	MK_U32(0x03)
/* 100 := A B C --> C A B */
#define NVCSI_CPHY_POLARITY_CAB	MK_U32(0x04)
/* 101 := A B C --> C B A */
#define NVCSI_CPHY_POLARITY_CBA	MK_U32(0x05)
/** @} */

/**
 * @brief NvCSI Brick configuration
 */
struct nvcsi_brick_config {
	/** Select PHY @ref NvCsiPhyType "mode" for both partitions */
	uint32_t phy_mode;
	/** @ref NvCsiLaneSwizzle "Lane swizzles" control for bricks. Valid for C-PHY and D-PHY modes */
	uint32_t lane_swizzle;
	/**
	 * Lane polarity control. Value depends on PhyMode
	 * See @ref NvCsiDPhyPolarity "D-Phy Polarity" @ref NvCsiCPhyPolarity "C_Phy Polarity"
	 */
	uint8_t lane_polarity[NVCSI_BRICK_NUM_LANES];
	/** Reserved */
	uint32_t __pad32;
} CAPTURE_IVC_ALIGN;

/**
 * @brief NvCSI Control and Interface Logic Configuration
 */
struct nvcsi_cil_config {
	/** Number of data lanes used (0-4) */
	uint8_t num_lanes;
	/** LP bypass mode (boolean) */
	uint8_t lp_bypass_mode;
	/** Set MIPI THS-SETTLE timing */
	uint8_t t_hs_settle;
	/** Set MIPI TCLK-SETTLE timing */
	uint8_t t_clk_settle;
	/** NVCSI CIL clock rate [kHz] */
	uint32_t cil_clock_rate;
	/** MIPI clock rate for D-Phy. Symbol rate for C-Phy [kHz] */
	uint32_t mipi_clock_rate;
	/** Reserved */
	uint32_t __pad32;
} CAPTURE_IVC_ALIGN;

/**
 * @defgroup HsmCsimuxErrors Bitmask for CSIMUX errors reported to HSM
 */
/** @{ */
/** Error bit indicating next packet after a frame end was not a frame start */
#define VI_HSM_CSIMUX_ERROR_MASK_BIT_SPURIOUS_EVENT MK_BIT32(0)
/** Error bit indicating FIFO for the stream has over flowed */
#define VI_HSM_CSIMUX_ERROR_MASK_BIT_OVERFLOW MK_BIT32(1)
/** Error bit indicating frame start packet lost due to FIFO overflow */
#define VI_HSM_CSIMUX_ERROR_MASK_BIT_LOF MK_BIT32(2)
/** Error bit indicating that an illegal packet has been sent from NVCSI */
#define VI_HSM_CSIMUX_ERROR_MASK_BIT_BADPKT MK_BIT32(3)
/** @} */

/**
 * @brief VI EC/HSM error masking configuration
 */
struct vi_hsm_csimux_error_mask_config {
	/** Mask correctable CSIMUX. See @ref HsmCsimuxErrors "CSIMUX error bitmask". */
	uint32_t error_mask_correctable;
	/** Mask uncorrectable CSIMUX. See @ref HsmCsimuxErrors "CSIMUX error bitmask". */
	uint32_t error_mask_uncorrectable;
} CAPTURE_IVC_ALIGN;

/**
 * @defgroup NvCsiStreamErr NVCSI stream novc+vc error flags
 * @{
 */
/** Multi bit error in the DPHY packet header */
#define NVCSI_INTR_FLAG_STREAM_NOVC_ERR_PH_ECC_MULTI_BIT	MK_BIT32(0)
/** Error bit indicating both of the CPHY packet header CRC check fail */
#define NVCSI_INTR_FLAG_STREAM_NOVC_ERR_PH_BOTH_CRC		MK_BIT32(1)
/** Error bit indicating VC Pixel Parser (PP) FSM timeout for a pixel line.*/
#define NVCSI_INTR_FLAG_STREAM_VC_ERR_PPFSM_TIMEOUT		MK_BIT32(2)
/** Error bit indicating VC has packet with single bit ECC error in the packet header*/
#define NVCSI_INTR_FLAG_STREAM_VC_ERR_PH_ECC_SINGLE_BIT		MK_BIT32(3)
/** Error bit indicating VC has packet payload crc check fail */
#define NVCSI_INTR_FLAG_STREAM_VC_ERR_PD_CRC			MK_BIT32(4)
/** Error bit indicating VC has packet terminate before getting the expect word count data. */
#define NVCSI_INTR_FLAG_STREAM_VC_ERR_PD_WC_SHORT		MK_BIT32(5)
/** Error bit indicating VC has one of the CPHY packet header CRC check fail. */
#define NVCSI_INTR_FLAG_STREAM_VC_ERR_PH_SINGLE_CRC		MK_BIT32(6)
/** @} */

/**
 * @defgroup NvCsiCilIntrErr NVCSI phy/cil interrupt error flags
 * @{
 */
#define NVCSI_INTR_FLAG_CIL_INTR_DPHY_ERR_CLK_LANE_CTRL		MK_BIT32(0)
#define NVCSI_INTR_FLAG_CIL_INTR_DATA_LANE_ERR0_SOT_SB		MK_BIT32(1)
#define NVCSI_INTR_FLAG_CIL_INTR_DATA_LANE_ERR0_SOT_MB		MK_BIT32(2)
#define NVCSI_INTR_FLAG_CIL_INTR_DATA_LANE_ERR0_CTRL		MK_BIT32(3)
#define NVCSI_INTR_FLAG_CIL_INTR_DATA_LANE_ERR0_RXFIFO_FULL	MK_BIT32(4)
#define NVCSI_INTR_FLAG_CIL_INTR_DATA_LANE_ERR1_SOT_SB		MK_BIT32(5)
#define NVCSI_INTR_FLAG_CIL_INTR_DATA_LANE_ERR1_SOT_MB		MK_BIT32(6)
#define NVCSI_INTR_FLAG_CIL_INTR_DATA_LANE_ERR1_CTRL		MK_BIT32(7)
#define NVCSI_INTR_FLAG_CIL_INTR_DATA_LANE_ERR1_RXFIFO_FULL	MK_BIT32(8)
#define NVCSI_INTR_FLAG_CIL_INTR_DPHY_DESKEW_CALIB_ERR_LANE0	MK_BIT32(9)
#define NVCSI_INTR_FLAG_CIL_INTR_DPHY_DESKEW_CALIB_ERR_LANE1	MK_BIT32(10)
#define NVCSI_INTR_FLAG_CIL_INTR_DPHY_DESKEW_CALIB_ERR_CTRL	MK_BIT32(11)
#define NVCSI_INTR_FLAG_CIL_INTR_DPHY_LANE_ALIGN_ERR		MK_BIT32(12)
#define NVCSI_INTR_FLAG_CIL_INTR_DATA_LANE_ERR0_ESC_MODE_SYNC	MK_BIT32(13)
#define NVCSI_INTR_FLAG_CIL_INTR_DATA_LANE_ERR1_ESC_MODE_SYNC	MK_BIT32(14)
#define NVCSI_INTR_FLAG_CIL_INTR_DATA_LANE_ERR0_SOT_2LSB_FULL	MK_BIT32(15)
#define NVCSI_INTR_FLAG_CIL_INTR_DATA_LANE_ERR1_SOT_2LSB_FULL	MK_BIT32(16)
/** @} */

/**
 * @defgroup NvCsiCilIntr0Err NVCSI phy/cil interrupt0 error flags
 * @{
 */
#define NVCSI_INTR_FLAG_CIL_INTR0_DPHY_ERR_CLK_LANE_CTRL	MK_BIT32(0)
#define NVCSI_INTR_FLAG_CIL_INTR0_DATA_LANE_ERR0_SOT_SB		MK_BIT32(1)
#define NVCSI_INTR_FLAG_CIL_INTR0_DATA_LANE_ERR0_SOT_MB		MK_BIT32(2)
#define NVCSI_INTR_FLAG_CIL_INTR0_DATA_LANE_ERR0_CTRL		MK_BIT32(3)
#define NVCSI_INTR_FLAG_CIL_INTR0_DATA_LANE_ERR0_RXFIFO_FULL	MK_BIT32(4)
#define NVCSI_INTR_FLAG_CIL_INTR0_DATA_LANE_ERR1_SOT_SB		MK_BIT32(5)
#define NVCSI_INTR_FLAG_CIL_INTR0_DATA_LANE_ERR1_SOT_MB		MK_BIT32(6)
#define NVCSI_INTR_FLAG_CIL_INTR0_DATA_LANE_ERR1_CTRL		MK_BIT32(7)
#define NVCSI_INTR_FLAG_CIL_INTR0_DATA_LANE_ERR1_RXFIFO_FULL	MK_BIT32(8)
#define NVCSI_INTR_FLAG_CIL_INTR0_DATA_LANE_ERR0_SOT_2LSB_FULL	MK_BIT32(9)
#define NVCSI_INTR_FLAG_CIL_INTR0_DATA_LANE_ERR1_SOT_2LSB_FULL	MK_BIT32(10)
#define NVCSI_INTR_FLAG_CIL_INTR0_DATA_LANE_ERR0_ESC_MODE_SYNC	MK_BIT32(19)
#define NVCSI_INTR_FLAG_CIL_INTR0_DATA_LANE_ERR1_ESC_MODE_SYNC	MK_BIT32(20)
#define NVCSI_INTR_FLAG_CIL_INTR0_DPHY_DESKEW_CALIB_DONE_LANE0	MK_BIT32(22)
#define NVCSI_INTR_FLAG_CIL_INTR0_DPHY_DESKEW_CALIB_DONE_LANE1	MK_BIT32(23)
#define NVCSI_INTR_FLAG_CIL_INTR0_DPHY_DESKEW_CALIB_DONE_CTRL	MK_BIT32(24)
#define NVCSI_INTR_FLAG_CIL_INTR0_DPHY_DESKEW_CALIB_ERR_LANE0	MK_BIT32(25)
#define NVCSI_INTR_FLAG_CIL_INTR0_DPHY_DESKEW_CALIB_ERR_LANE1	MK_BIT32(26)
#define NVCSI_INTR_FLAG_CIL_INTR0_DPHY_DESKEW_CALIB_ERR_CTRL	MK_BIT32(27)
#define NVCSI_INTR_FLAG_CIL_INTR0_DPHY_LANE_ALIGN_ERR		MK_BIT32(28)
#define NVCSI_INTR_FLAG_CIL_INTR0_CPHY_CLK_CAL_DONE_TRIO0	MK_BIT32(29)
#define NVCSI_INTR_FLAG_CIL_INTR0_CPHY_CLK_CAL_DONE_TRIO1	MK_BIT32(30)
/** @} */

/**
 * @defgroup NvCsiCilIntr0Err NVCSI phy/cil interrupt1 error flags
 * @{
 */
#define NVCSI_INTR_FLAG_CIL_INTR1_DATA_LANE_ESC_CMD_REC0	MK_BIT32(0)
#define NVCSI_INTR_FLAG_CIL_INTR1_DATA_LANE_ESC_DATA_REC0	MK_BIT32(1)
#define NVCSI_INTR_FLAG_CIL_INTR1_DATA_LANE_ESC_CMD_REC1	MK_BIT32(2)
#define NVCSI_INTR_FLAG_CIL_INTR1_DATA_LANE_ESC_DATA_REC1	MK_BIT32(3)
#define NVCSI_INTR_FLAG_CIL_INTR1_REMOTERST_TRIGGER_INT0	MK_BIT32(4)
#define NVCSI_INTR_FLAG_CIL_INTR1_ULPS_TRIGGER_INT0		MK_BIT32(5)
#define NVCSI_INTR_FLAG_CIL_INTR1_LPDT_INT0			MK_BIT32(6)
#define NVCSI_INTR_FLAG_CIL_INTR1_REMOTERST_TRIGGER_INT1	MK_BIT32(7)
#define NVCSI_INTR_FLAG_CIL_INTR1_ULPS_TRIGGER_INT1		MK_BIT32(8)
#define NVCSI_INTR_FLAG_CIL_INTR1_LPDT_INT1			MK_BIT32(9)
#define NVCSI_INTR_FLAG_CIL_INTR1_DPHY_CLK_LANE_ULPM_REQ	MK_BIT32(10)
/** @} */

/**
 * @defgroup NvCsiIntrCfg NVCSI interrupt config bit masks
 * @{
 */
#define NVCSI_INTR_CONFIG_MASK_HOST1X		MK_U32(0x1)
#define NVCSI_INTR_CONFIG_MASK_STATUS2VI	MK_U32(0xffff)
#define NVCSI_INTR_CONFIG_MASK_STREAM_NOVC	MK_U32(0x3)
#define NVCSI_INTR_CONFIG_MASK_STREAM_VC	MK_U32(0x7c)
#define NVCSI_INTR_CONFIG_MASK_CIL_INTR		MK_U32(0x1ffff)
#define NVCSI_INTR_CONFIG_MASK_CIL_INTR0	MK_U32(0x7fd807ff)
#define NVCSI_INTR_CONFIG_MASK_CIL_INTR1	MK_U32(0x7ff)
/** @} */

/**
 * @defgroup NvCsiIntrCfgShift NVCSI interrupt config bit shifts
 * @{
 */
#define NVCSI_INTR_CONFIG_SHIFT_STREAM_NOVC	MK_U32(0x0)
#define NVCSI_INTR_CONFIG_SHIFT_STREAM_VC	MK_U32(0x2)
/** @} */

/**
 * @brief User-defined error configuration.
 *
 * Flag NVCSI_CONFIG_FLAG_ERROR must be set to enable these settings,
 * otherwise default settings will be used.
 */
struct nvcsi_error_config {
	/** Mask Host1x timeout interrupt*/
	uint32_t host1x_intr_mask;
	/** Host1x Interrupt error type. 0 - Corrected error, 1 - Uncorrected error. */
	uint32_t host1x_intr_type;
	/** Mask status2vi NOTIFY reporting */
	uint32_t status2vi_notify_mask;
	/** Mask stream intrs */
	uint32_t stream_intr_mask;
	/** CSI stream Interrupt error type. 0 - Corrected error, 1 - Uncorrected error. */
	uint32_t stream_intr_type;
	/** Mask cil intrs */
	uint32_t cil_intr_mask;
	/** CIL interrupt error type. 0 - Corrected error, 1 - Uncorrected error. */
	uint32_t cil_intr_type;
	/** Mask cil intr0 intrs */
	uint32_t cil_intr0_mask;
	/** Mask cil intr1 intrs */
	uint32_t cil_intr1_mask;
	/** Reserved */
	uint32_t __pad32;
	/** VI EC/HSM error masking configuration */
	struct vi_hsm_csimux_error_mask_config csimux_config;
} CAPTURE_IVC_ALIGN;

/**
 * @defgroup NvCsiDataType NVCSI datatypes
 * @{
 */
#define NVCSI_DATATYPE_UNSPECIFIED	MK_U32(0)
#define NVCSI_DATATYPE_YUV420_8		MK_U32(24)
#define NVCSI_DATATYPE_YUV420_10	MK_U32(25)
#define NVCSI_DATATYPE_LEG_YUV420_8	MK_U32(26)
#define NVCSI_DATATYPE_YUV420CSPS_8	MK_U32(28)
#define NVCSI_DATATYPE_YUV420CSPS_10	MK_U32(29)
#define NVCSI_DATATYPE_YUV422_8		MK_U32(30)
#define NVCSI_DATATYPE_YUV422_10	MK_U32(31)
#define NVCSI_DATATYPE_RGB444		MK_U32(32)
#define NVCSI_DATATYPE_RGB555		MK_U32(33)
#define NVCSI_DATATYPE_RGB565		MK_U32(34)
#define NVCSI_DATATYPE_RGB666		MK_U32(35)
#define NVCSI_DATATYPE_RGB888		MK_U32(36)
#define NVCSI_DATATYPE_RAW6		MK_U32(40)
#define NVCSI_DATATYPE_RAW7		MK_U32(41)
#define NVCSI_DATATYPE_RAW8		MK_U32(42)
#define NVCSI_DATATYPE_RAW10		MK_U32(43)
#define NVCSI_DATATYPE_RAW12		MK_U32(44)
#define NVCSI_DATATYPE_RAW14		MK_U32(45)
#define NVCSI_DATATYPE_RAW16		MK_U32(46)
#define NVCSI_DATATYPE_RAW20		MK_U32(47)
#define NVCSI_DATATYPE_USER_1		MK_U32(48)
#define NVCSI_DATATYPE_USER_2		MK_U32(49)
#define NVCSI_DATATYPE_USER_3		MK_U32(50)
#define NVCSI_DATATYPE_USER_4		MK_U32(51)
#define NVCSI_DATATYPE_USER_5		MK_U32(52)
#define NVCSI_DATATYPE_USER_6		MK_U32(53)
#define NVCSI_DATATYPE_USER_7		MK_U32(54)
#define NVCSI_DATATYPE_USER_8		MK_U32(55)
#define NVCSI_DATATYPE_UNKNOWN		MK_U32(64)
/** @} */

/* DEPRECATED - to be removed */
/** T210 (also exists in T186) */
#define NVCSI_PATTERN_GENERATOR_T210	MK_U32(1)
/** T186 only */
#define NVCSI_PATTERN_GENERATOR_T186	MK_U32(2)
/** T194 only */
#define NVCSI_PATTERN_GENERATOR_T194	MK_U32(3)

/* DEPRECATED - to be removed */
#define NVCSI_DATA_TYPE_Unspecified		MK_U32(0)
#define NVCSI_DATA_TYPE_YUV420_8		MK_U32(24)
#define NVCSI_DATA_TYPE_YUV420_10		MK_U32(25)
#define NVCSI_DATA_TYPE_LEG_YUV420_8		MK_U32(26)
#define NVCSI_DATA_TYPE_YUV420CSPS_8		MK_U32(28)
#define NVCSI_DATA_TYPE_YUV420CSPS_10		MK_U32(29)
#define NVCSI_DATA_TYPE_YUV422_8		MK_U32(30)
#define NVCSI_DATA_TYPE_YUV422_10		MK_U32(31)
#define NVCSI_DATA_TYPE_RGB444			MK_U32(32)
#define NVCSI_DATA_TYPE_RGB555			MK_U32(33)
#define NVCSI_DATA_TYPE_RGB565			MK_U32(34)
#define NVCSI_DATA_TYPE_RGB666			MK_U32(35)
#define NVCSI_DATA_TYPE_RGB888			MK_U32(36)
#define NVCSI_DATA_TYPE_RAW6			MK_U32(40)
#define NVCSI_DATA_TYPE_RAW7			MK_U32(41)
#define NVCSI_DATA_TYPE_RAW8			MK_U32(42)
#define NVCSI_DATA_TYPE_RAW10			MK_U32(43)
#define NVCSI_DATA_TYPE_RAW12			MK_U32(44)
#define NVCSI_DATA_TYPE_RAW14			MK_U32(45)
#define NVCSI_DATA_TYPE_RAW16			MK_U32(46)
#define NVCSI_DATA_TYPE_RAW20			MK_U32(47)
#define NVCSI_DATA_TYPE_Unknown			MK_U32(64)

/* NVCSI DPCM ratio */
#define NVCSI_DPCM_RATIO_BYPASS		MK_U32(0)
#define NVCSI_DPCM_RATIO_10_8_10	MK_U32(1)
#define NVCSI_DPCM_RATIO_10_7_10	MK_U32(2)
#define NVCSI_DPCM_RATIO_10_6_10	MK_U32(3)
#define NVCSI_DPCM_RATIO_12_8_12	MK_U32(4)
#define NVCSI_DPCM_RATIO_12_7_12	MK_U32(5)
#define NVCSI_DPCM_RATIO_12_6_12	MK_U32(6)
#define NVCSI_DPCM_RATIO_14_10_14	MK_U32(7)
#define NVCSI_DPCM_RATIO_14_8_14	MK_U32(8)
#define NVCSI_DPCM_RATIO_12_10_12	MK_U32(9)

/**
 * @defgroup NvCsiParamType NvCSI Parameter Type
 * @{
 */
#define NVCSI_PARAM_TYPE_UNSPECIFIED	MK_U32(0)
#define NVCSI_PARAM_TYPE_DPCM		MK_U32(1)
#define NVCSI_PARAM_TYPE_DT_OVERRIDE	MK_U32(2)
#define NVCSI_PARAM_TYPE_WATCHDOG	MK_U32(3)
/**@}*/

struct nvcsi_dpcm_config {
	uint32_t dpcm_ratio;
	uint32_t __pad32;
} CAPTURE_IVC_ALIGN;

/**
 * @brief NvCSI data type (DT) override configuration
 */
struct nvcsi_dt_override_config {
	/** Flag to enable DT override */
	uint8_t enable_override;
	/** Reserved */
	uint8_t __pad8[7];
	/** NvCSI data type */
	uint32_t override_type;
	/** RCE exception type */
	uint32_t exception_type[NVCSI_NUM_NOOVERRIDE_DT];
} CAPTURE_IVC_ALIGN;

/**
 * @brief NvCSI watchdog configuration
 */
struct nvcsi_watchdog_config {
	/** Enable/disable the pixel parser watchdog */
	uint8_t enable;
	/** Reserved */
	uint8_t __pad8[3];
	/** The watchdog timer timeout period */
	uint32_t period;
} CAPTURE_IVC_ALIGN;

/**
 * NVCSI - TPG attributes
 */
/**
@brief Number of vertical color bars in TPG (t186)
*/
#define NVCSI_TPG_NUM_COLOR_BARS MK_U32(8)

/**
 * @brief NvCSI test pattern generator (TPG) configuration for T186
 */
struct nvcsi_tpg_config_t186 {
	/** NvCSI stream number */
	uint8_t stream_id;
	/** DEPRECATED - to be removed */
	uint8_t stream;
	/** NvCSI virtual channel ID */
	uint8_t virtual_channel_id;
	/** DEPRECATED - to be removed */
	uint8_t virtual_channel;
	/** Initial frame number */
	uint16_t initial_frame_number;
	/** Reserved */
	uint16_t __pad16;
	/** Enable frame number generation */
	uint32_t enable_frame_counter;
	/** NvCSI datatype */
	uint32_t datatype;
	/** DEPRECATED - to be removed */
	uint32_t data_type;
	/** Width of the generated test image */
	uint16_t image_width;
	/** Height of the generated test image */
	uint16_t image_height;
	/** Pixel value for each horizontal color bar (format according to DT) */
	uint32_t pixel_values[NVCSI_TPG_NUM_COLOR_BARS];
} CAPTURE_IVC_ALIGN;

/**
 * @brief NvCsiTpgFlag Test pattern generator (TPG) flags for t194
 * @{
 */
#define NVCSI_TPG_FLAG_PATCH_MODE	MK_U16(1)
#define NVCSI_TPG_FLAG_PHASE_INCREMENT	MK_U16(2)
#define NVCSI_TPG_FLAG_AUTO_STOP	MK_U16(4)
/** @} */

/**
 * @brief NvCSI test pattern generator (TPG) configuration for T194
 */
struct nvcsi_tpg_config_t194 {
	/** NvCSI Virtual channel ID */
	uint8_t virtual_channel_id;
	/** NvCSI datatype * */
	uint8_t datatype;
	/** @ref NvCsiTpgFlag "NvCSI TPG flag" */
	uint16_t flags;
	/** Starting framen number for TPG */
	uint16_t initial_frame_number;
	/** Maximum number for frames to be generated by TPG */
	uint16_t maximum_frame_number;
	/** Width of the generated frame in pixels */
	uint16_t image_width;
	/** Height of the generated frame in pixels */
	uint16_t image_height;
	/** Embedded data line width in bytes */
	uint32_t embedded_line_width;
	/** Line count of the embedded data before the pixel frame. */
	uint32_t embedded_lines_top;
	/** Line count of the embedded data after the pixel frame. */
	uint32_t embedded_lines_bottom;
	/* The lane count for the VC. */
	uint32_t lane_count;
	/** Initial phase */
	uint32_t initial_phase;
	/** Initial horizontal frequency for red channel */
	uint32_t red_horizontal_init_freq;
	/** Initial vertical frequency for red channel */
	uint32_t red_vertical_init_freq;
	/** Rate of change of the horizontal frequency for red channel */
	uint32_t red_horizontal_freq_rate;
	/** Rate of change of the vertical frequency for red channel */
	uint32_t red_vertical_freq_rate;
	/** Initial horizontal frequency for green channel */
	uint32_t green_horizontal_init_freq;
	/** Initial vertical frequency for green channel */
	uint32_t green_vertical_init_freq;
	/** Rate of change of the horizontal frequency for green channel */
	uint32_t green_horizontal_freq_rate;
	/** Rate of change of the vertical frequency for green channel */
	uint32_t green_vertical_freq_rate;
	/** Initial horizontal frequency for blue channel */
	uint32_t blue_horizontal_init_freq;
	/** Initial vertical frequency for blue channel */
	uint32_t blue_vertical_init_freq;
	/** Rate of change of the horizontal frequency for blue channel */
	uint32_t blue_horizontal_freq_rate;
	/** Rate of change of the vertical frequency for blue channel */
	uint32_t blue_vertical_freq_rate;
} CAPTURE_IVC_ALIGN;

/**
 * @brief Commong NvCSI test pattern generator (TPG) configuration
 */
union nvcsi_tpg_config {
	/** TPG configuration for T186 */
	struct nvcsi_tpg_config_t186 t186;
	/** TPG configuration for T186 */
	struct nvcsi_tpg_config_t194 t194;
	/** Reserved size */
	uint32_t reserved[32];
};

/**
 * @brief TPG rate configuration, low level parameters
 */
struct nvcsi_tpg_rate_config {
	/** Horizontal blanking (clocks) */
	uint32_t hblank;
	/** Vertical blanking (clocks) */
	uint32_t vblank;
	/** T194 only: Interval between pixels (clocks) */
	uint32_t pixel_interval;
	/** Reserved */
	uint32_t reserved;
} CAPTURE_IVC_ALIGN;

/**
 * ISP capture settings
 */

/**
 * @defgroup IspErrorMask ISP Channel error mask
 * @deprecated
 */
/** @{ */
#define CAPTURE_ISP_CHANNEL_ERROR_DMA_PBUF_ERR		MK_BIT32(0)
#define CAPTURE_ISP_CHANNEL_ERROR_DMA_SBUF_ERR		MK_BIT32(1)
#define CAPTURE_ISP_CHANNEL_ERROR_DMA_SEQ_ERR		MK_BIT32(2)
#define CAPTURE_ISP_CHANNEL_ERROR_FRAMEID_ERR		MK_BIT32(3)
#define CAPTURE_ISP_CHANNEL_ERROR_TIMEOUT		MK_BIT32(4)
#define CAPTURE_ISP_CHANNEL_ERROR_ALL			MK_U32(0x001F)
/** @} */

/**
 * @defgroup ISPProcessChannelFlags ISP process channel specific flags
 */
/**@{*/
/** Channel reset on error */
#define CAPTURE_ISP_CHANNEL_FLAG_RESET_ON_ERROR	MK_U32(0x0001)
/**@}*/

/**
 * @brief Describes RTCPU side resources for a ISP capture pipe-line.
 *
 * Following structure defines ISP channel specific configuration;
 */
struct capture_channel_isp_config {
	/** Unique ISP process channel ID */
	uint8_t channel_id;
	/** Reserved */
	uint8_t __pad_chan[3];
	/** ISP channel specific @ref ISPProcessChannelFlags "flags" */
	uint32_t channel_flags;
	/**
	 * Base address of ISP capture descriptor ring buffer.
	 * The size of the buffer is request_queue_depth * request_size
	 */
	iova_t requests;
	/** Number of ISP process requests in the ring buffer */
	uint32_t request_queue_depth;
	/** Size of each ISP process request (@ref isp_capture_descriptor) */
	uint32_t request_size;

	/**
	 * Base address of ISP program descriptor ring buffer.
	 * The size of the buffer is program_queue_depth * program_size
	 */
	iova_t programs;
	/** Number of ISP program requests in the ring buffer */
	uint32_t program_queue_depth;
	/** Size of each ISP process request (@ref isp_program_descriptor) */
	uint32_t program_size;
	/** ISP Process output buffer syncpoint info */
	struct syncpoint_info progress_sp;
	/** Statistics buffer syncpoint info */
	struct syncpoint_info stats_progress_sp;

	/** @deprecated */
	uint32_t error_mask_correctable CAMRTC_DEPRECATED;
	/** @deprecated */
	uint32_t error_mask_uncorrectable CAMRTC_DEPRECATED;

#define HAVE_ISP_GOS_TABLES
	/** Number of active ISP GOS tables in isp_gos_tables[] */
	uint32_t num_isp_gos_tables;
	/** Reserved */
	uint32_t __pad_chan2;
	/**
	 * GoS tables can only be programmed when there are no
	 * active channels. For subsequent channels we check that
	 * the channel configuration matches with the active
	 * configuration.
	 */
	iova_t isp_gos_tables[ISP_NUM_GOS_TABLES];
} CAPTURE_IVC_ALIGN;

/**
 * @defgroup IspProcesStatus ISP process status codes
 */
/** @{ */
/** ISP frame processing status unknown */
#define CAPTURE_ISP_STATUS_UNKNOWN		MK_U32(0)
/** ISP frame processing succeeded */
#define CAPTURE_ISP_STATUS_SUCCESS		MK_U32(1)
/** ISP frame processing encountered an error */
#define CAPTURE_ISP_STATUS_ERROR		MK_U32(2)
/** @} */

/**
 * @brief ISP process request status
 */
struct capture_isp_status {
	/** ISP channel id */
	uint8_t chan_id;
	/** Reserved */
	uint8_t __pad;
	/** Frame sequence number */
	uint16_t frame_id;
	/** @ref IspProcessStatus "Process status" */
	uint32_t status;
	/** Error bit mask. Zero in case of SUCCESS, non-zero value case of ERROR */
	uint32_t error_mask;
	/** Reserved */
	uint32_t __pad2;
} CAPTURE_IVC_ALIGN;

/**
 * @defgroup IspProgramStatus ISP program status codes
 */
/** @{ */
/** ISP program status unknown */
#define CAPTURE_ISP_PROGRAM_STATUS_UNKNOWN	MK_U32(0)
/** ISP program was used successfully for frame processing */
#define CAPTURE_ISP_PROGRAM_STATUS_SUCCESS	MK_U32(1)
/** ISP program encountered an error */
#define CAPTURE_ISP_PROGRAM_STATUS_ERROR	MK_U32(2)
/** ISP program has expired and is not being used by any active process requests */
#define CAPTURE_ISP_PROGRAM_STATUS_STALE	MK_U32(3)
/** @} */

/**
 * @brief ISP program request status
 */
struct capture_isp_program_status {
	/** ISP channel id */
	uint8_t chan_id;
	/** ISP program settings id */
	uint8_t settings_id;
	/** Reserved */
	uint16_t __pad_id;
	/** @ref IspProgramStatus "Program status" */
	uint32_t status;
	/** Error bit mask. Zero in case of SUCCESS, non-zero value case of ERROR */
	uint32_t error_mask;
	/** Reserved */
	uint32_t __pad2;
} CAPTURE_IVC_ALIGN;

/**
 * @defgroup IspActivateFlag ISP program activation flag
 */
/** @{ */
/** Program request will when the frame sequence id reaches a certain threshold */
#define CAPTURE_ACTIVATE_FLAG_ON_SEQUENCE_ID	MK_U32(0x1)
/** Program request will be activate when the frame settings id reaches a certain threshold */
#define CAPTURE_ACTIVATE_FLAG_ON_SETTINGS_ID	MK_U32(0x2)
/** Each Process request is coupled with a Program request */
#define CAPTURE_ACTIVATE_FLAG_COUPLED		MK_U32(0x4)
/** @} */

/**
 * @brief Describes ISP program structure;
 */
struct isp_program_descriptor {
	/** ISP settings_id which uniquely identifies isp_program. */
	uint8_t settings_id;
	/**
	 * VI channel bound to the isp channel.
	 * In case of mem_isp_mem set this to CAPTURE_NO_VI_ISP_BINDING
	 */
	uint8_t vi_channel_id;
#define CAPTURE_NO_VI_ISP_BINDING MK_U8(0xFF)
	/** Reserved */
	uint8_t __pad_sid[2];
	/**
	 * Capture sequence id, frame id; Given ISP program will be used from this frame ID onwards
	 * until new ISP program does replace it.
	 */
	uint32_t sequence;

	/**
	 * Offset to memory mapped ISP program buffer from ISP program descriptor base address,
	 * which contains the ISP configs and PB1 containing HW settings. Ideally the offset is
	 * the size(ATOM aligned) of ISP program descriptor only, as each isp_program would be placed
	 * just after it's corresponding ISP program descriptor in memory.
	 */
	uint32_t isp_program_offset;
	/** Size of isp program structure */
	uint32_t isp_program_size;

	/**
	 * Base address of memory mapped ISP PB1 containing isp HW settings.
	 * This has to be 64 bytes aligned
	 */
	iova_t isp_pb1_mem;

	/** ISP program request status written by RCE */
	struct capture_isp_program_status isp_program_status;

	/** Activation condition for given ISP program. See @ref IspActivateFlag "Activation flags" */
	uint32_t activate_flags;

	/** Pad to aligned size */
	uint32_t __pad[5];
} CAPTURE_DESCRIPTOR_ALIGN;

/**
 * @brief ISP program size (ATOM aligned).
 *
 * NvCapture UMD makes sure to place isp_program just after above program
 * descriptor buffer for each request, so that KMD and RCE can co-locate
 * isp_program and it's corresponding program descriptor in memory.
 */
#define ISP_PROGRAM_MAX_SIZE 16512

/**
 * @brief ISP image surface info
 */
struct image_surface {
	/** Lower 32-bit of the buffer's base address */
	uint32_t offset;
	/** Upper 8-bit of the buffer's base address */
	uint32_t offset_hi;
	/** The surface stride in bytes */
	uint32_t surface_stride;
	/** Reserved */
	uint32_t __pad_surf;
} CAPTURE_IVC_ALIGN;

/**
 * @brief Output image surface info
 */
struct stats_surface {
	/** Lower 32-bit of the statistics buffer base address */
	uint32_t offset;
	/** Upper 8-bit of the statistics buffer base address */
	uint32_t offset_hi;
} CAPTURE_IVC_ALIGN;

/**
 * @defgroup IspProcessFlag ISP process frame specific flags.
 */
/** @{ */
/** Enables process status reporting for the channel */
#define CAPTURE_ISP_FLAG_STATUS_REPORT_ENABLE	MK_BIT32(0)
/** Enables error reporting for the channel */
#define CAPTURE_ISP_FLAG_ERROR_REPORT_ENABLE	MK_BIT32(1)
/** Enables process and program request binding for the channel */
#define CAPTURE_ISP_FLAG_ISP_PROGRAM_BINDING	MK_BIT32(2)
/** @} */

/**
 * @brief ISP capture descriptor
 */
struct isp_capture_descriptor {
	/** Process request sequence number, frame id */
	uint32_t sequence;
	/** ISP frame specific flags. See @ref IspProcessFlag "ISP process flags" */
	uint32_t capture_flags;

	/** 1 MR port, max 3 input surfaces */
#define ISP_MAX_INPUT_SURFACES		MK_U32(3)

	/** Input images surfaces */
	struct image_surface input_mr_surfaces[ISP_MAX_INPUT_SURFACES];

	/**
	 * 3 MW ports, max 2 surfaces (multiplanar) per port.
	 */
#define ISP_MAX_OUTPUTS			MK_U32(3)
#define ISP_MAX_OUTPUT_SURFACES		MK_U32(2)

	struct {
		/** Memory write port output surfaces */
		struct image_surface surfaces[ISP_MAX_OUTPUT_SURFACES];
		/* TODO: Should we have here just image format enum value + block height instead?
		Dither settings would logically be part of ISP program */
		/** Image format definition for output surface */
		uint32_t image_def;
		/** Width of the output surface in pixels */
		uint16_t width;
		/** Height of the output surface in pixels */
		uint16_t height;
	} outputs_mw[ISP_MAX_OUTPUTS];

	/** Flicker band (FB) statistics buffer */
	struct stats_surface fb_surface;
	/** Focus metrics (FM) statistics buffer */
	struct stats_surface fm_surface;
	/** Auto Focus Metrics (AFM) statistics buffer */
	struct stats_surface afm_surface;
	/** Local Average Clipping (LAC0) unit 0 statistics buffer */
	struct stats_surface lac0_surface;
	/** Local Average Clipping (LAC1) unit 1 statistics buffer */
	struct stats_surface lac1_surface;
	/** Histogram (H0) unit 0 statistics buffer */
	struct stats_surface h0_surface;
	/** Histogram (H1) unit 1 statistics buffer */
	struct stats_surface h1_surface;
	/** Pixel Replacement Unit (PRU) statistics buffer */
	struct stats_surface pru_bad_surface;
	/** Local Tone Mapping statistics buffer */
	struct stats_surface ltm_surface;

	/** Surfaces related configuration */
	struct {
		/** Input image surface width in pixels */
		uint16_t mr_width;
		/** Input image surface height in pixels */
		uint16_t mr_height;

		/** Height of slices used for processing the image */
		uint16_t slice_height;
		/** Width of first VI chunk in a line */
		uint16_t chunk_width_first;
		/**
		 * Width of VI chunks in the middle of a line, and/or width of
		 *  ISP tiles in middle of a slice
		 */
		uint16_t chunk_width_middle;
		/** Width of overfetch area in the beginning of VI chunks */
		uint16_t chunk_overfetch_width;
		/** Width of the leftmost ISP tile in a slice */
		uint16_t tile_width_first;
		/** Input image cfa */
		uint8_t mr_image_cfa;
		/** Reserved */
		uint8_t _pad;
		/** MR unit input image format value */
		uint32_t mr_image_def;
		/* TODO: should this be exposed to user mode? */
		/** MR unit input image format value */
		uint32_t mr_image_def1;
		/** SURFACE_CTL_MR register value */
		uint32_t surf_ctrl;
		/** Byte stride between start of lines. Must be ATOM aligned */
		uint32_t surf_stride_line;
		/** Byte stride between start of DPCM chunks. Must be ATOM aligned */
		uint32_t surf_stride_chunk;
	} surface_configs;

	/** Reserved */
	uint32_t __pad2;
	/** Base address of ISP PB2 memory */
	iova_t isp_pb2_mem;
	/* TODO: Isn't PB2 size constant, do we need this? */
	/** Size of the pushbuffer 2 memory */
	uint32_t isp_pb2_size;
	/** Reserved */
	uint32_t __pad_pb;

	/** Frame processing timeout in microseconds */
	uint32_t frame_timeout;

	union {
		/** DEPRECATED - Do not use */
		uint32_t prefence_count CAMRTC_DEPRECATED;
		/**
		 * Number of inputfences for given capture request.
		 * These fences are exclusively associated with ISP input ports and
		 * they support subframe sychronization.
		 */
		uint32_t num_inputfences;
	};

	union {
		/** DEPRECATED - Do not use */
		struct syncpoint_info progress_prefence[ISP_MAX_INPUT_SURFACES] CAMRTC_DEPRECATED;
		/** Progress syncpoint for each one of inputfences */
		struct syncpoint_info inputfences[ISP_MAX_INPUT_SURFACES];
	};

	/* GID-STKHLDREQPLCL123-3812735 */
#define ISP_MAX_PREFENCES	MK_U32(14)

	/**
	 * Number of traditional prefences for given capture request.
	 * They are generic, so can be used for any pre-condition but do not
	 * support subframe synchronization
	 */
	uint32_t num_prefences;
	/** Reserved */
	uint32_t __pad_prefences;

	/** Syncpoint for each one of prefences */
	struct syncpoint_info prefences[ISP_MAX_PREFENCES];

	/** Engine result record – written by Falcon */
	struct engine_status_surface engine_status;

	/** Frame processing result record – written by RTCPU */
	struct capture_isp_status status;

	/* Information regarding the ISP program bound to this capture */
	uint32_t program_buffer_index;

	/** Reserved */
	uint32_t __pad[3];
} CAPTURE_DESCRIPTOR_ALIGN;

/**
 * @brief PB2 size (ATOM aligned).
 *
 * NvCapture UMD makes sure to place PB2 just after above capture
 * descriptor buffer for each request, so that KMD and RCE can co-locate
 * PB2 and it's corresponding capture descriptor in memory.
 */
#define ISP_PB2_MAX_SIZE		MK_SIZE(512)

/**
 * @brief Size allocated for the ISP program push buffer
 */
#define NVISP5_ISP_PROGRAM_PB_SIZE	MK_SIZE(16384)

/**
* @brief Size allocated for the push buffer containing output & stats
* surface definitions. Final value TBD
*/
#define NVISP5_SURFACE_PB_SIZE		MK_SIZE(512)

/**
 * @ brief Downscaler configuration information that is needed for building ISP config buffer.
 *
 * These registers cannot be included in push buffer but they must be provided in a structure
 * that RCE can parse. Format of the fields is same as in corresponding ISP registers.
 */
struct isp5_downscaler_configbuf {
	/**
	* Horizontal pixel increment, in U5.20 format. I.e. 2.5 means downscaling
	* by factor of 2.5. Corresponds to ISP_DM_H_PI register
	*/
	uint32_t pixel_incr_h;
	/**
	* Vertical pixel increment, in U5.20 format. I.e. 2.5 means downscaling
	* by factor of 2.5. Corresponds to ISP_DM_v_PI register
	*/
	uint32_t pixel_incr_v;

	/**
	* Offset of the first source image pixel to be used.
	* Topmost 16 bits - the leftmost column to be used
	* Lower 16 bits - the topmost line to be used
	*/
	uint32_t offset;

	/**
	* Size of the scaled destination image in pixels
	* Topmost 16 bits - height of destination image
	* Lowest 16 bits - Width of destination image
	*/
	uint32_t destsize;
};

/**
 * @brief ISP sub-units enabled bits.
 */
#define ISP5BLOCK_ENABLED_PRU_OUTLIER_REJECTION		MK_BIT32(0)
#define	ISP5BLOCK_ENABLED_PRU_STATS			MK_BIT32(1)
#define	ISP5BLOCK_ENABLED_PRU_HDR			MK_BIT32(2)
#define	ISP5BLOCK_ENABLED_AP_DEMOSAIC			MK_BIT32(4)
#define	ISP5BLOCK_ENABLED_AP_CAR			MK_BIT32(5)
#define	ISP5BLOCK_ENABLED_AP_LTM_MODIFY			MK_BIT32(6)
#define	ISP5BLOCK_ENABLED_AP_LTM_STATS			MK_BIT32(7)
#define	ISP5BLOCK_ENABLED_AP_FOCUS_METRIC		MK_BIT32(8)
#define	ISP5BLOCK_ENABLED_FLICKERBAND			MK_BIT32(9)
#define	ISP5BLOCK_ENABLED_HISTOGRAM0			MK_BIT32(10)
#define	ISP5BLOCK_ENABLED_HISTOGRAM1			MK_BIT32(11)
#define	ISP5BLOCK_ENABLED_DOWNSCALER0_HOR		MK_BIT32(12)
#define	ISP5BLOCK_ENABLED_DOWNSCALER0_VERT		MK_BIT32(13)
#define	ISP5BLOCK_ENABLED_DOWNSCALER1_HOR		MK_BIT32(14)
#define	ISP5BLOCK_ENABLED_DOWNSCALER1_VERT		MK_BIT32(15)
#define	ISP5BLOCK_ENABLED_DOWNSCALER2_HOR		MK_BIT32(16)
#define	ISP5BLOCK_ENABLED_DOWNSCALER2_VERT		MK_BIT32(17)
#define	ISP5BLOCK_ENABLED_SHARPEN0			MK_BIT32(18)
#define	ISP5BLOCK_ENABLED_SHARPEN1			MK_BIT32(19)
#define	ISP5BLOCK_ENABLED_LAC0_REGION0			MK_BIT32(20)
#define	ISP5BLOCK_ENABLED_LAC0_REGION1			MK_BIT32(21)
#define	ISP5BLOCK_ENABLED_LAC0_REGION2			MK_BIT32(22)
#define	ISP5BLOCK_ENABLED_LAC0_REGION3			MK_BIT32(23)
#define	ISP5BLOCK_ENABLED_LAC1_REGION0			MK_BIT32(24)
#define	ISP5BLOCK_ENABLED_LAC1_REGION1			MK_BIT32(25)
#define	ISP5BLOCK_ENABLED_LAC1_REGION2			MK_BIT32(26)
#define	ISP5BLOCK_ENABLED_LAC1_REGION3			MK_BIT32(27)

/**
 * @brief ISP overfetch requirements.
 *
 * ISP kernel needs access to pixels outside the active area of a tile
 * to ensure continuous processing across tile borders. The amount of data
 * needed depends on features enabled and some ISP parameters so this
 * is program dependent.
 *
 * ISP extrapolates values outside image borders, so overfetch is needed only
 * for borders between tiles.
 */

struct isp_overfetch {
	/** Number of pixels needed from the left side of tile */
	uint8_t left;
	/** Number of pixels needed from the right side of tile */
	uint8_t right;
	/** Number of pixels needed from above the tile */
	uint8_t top;
	/** Number of pixels needed from below the tile */
	uint8_t bottom;
	/**
	 * Number of pixels needed by PRU unit from left and right sides of the tile.
	 * This is needed to adjust tile border locations so that they align correctly
	 * at demosaic input.
	 */
	uint8_t pru_ovf_h;
	/**
	 * Alignment requirement for tile width. Minimum alignment is 2 pixels, but
	 * if CAR is used this must be set to half of LPF kernel width.
	 */
	uint8_t alignment;
	/** Reserved */
	uint8_t _pad1[2];
};

/**
 * @brief ISP program buffer
 *
 * Settings needed by RCE ISP driver to generate config buffer.
 * Content and format of these fields is the same as corresponding ISP config buffer fields.
 * See T19X_ISP_Microcode.docx for detailed description.
 */
struct isp5_program {
	/**
	 * Sources for LS, AP and PRU blocks.
	 * Format is same as in ISP's XB_SRC_0 register
	 */
	uint32_t xbsrc0;

	/**
	 * Sources for AT[0-2] and TF[0-1] blocks
	 * Format is same as in ISP's XB_SRC_1 register
	 */
	uint32_t xbsrc1;

	/**
	 * Sources for DS[0-2] and MW[0-2] blocks
	 * Format is same as in ISP's XB_SRC_2 register
	 */
	uint32_t xbsrc2;

	/**
	 * Sources for FB, LAC[0-1] and HIST[0-1] blocks
	 * Format is same as in ISP's XB_SRC_3 register
	 */
	uint32_t xbsrc3;

	/**
	 * Bitmask to describe which of ISP blocks are enabled.
	 * See microcode documentation for details.
	 */
	uint32_t enables_config;

	/**
	 * AFM configuration. See microcode documentation for details.
	 */
	uint32_t afm_ctrl;

	/**
	 * Mask for stats blocks enabled.
	 */
	uint32_t stats_aidx_flag;

	/**
	 * Size used for the push buffer in 4-byte words.
	 */
	uint32_t pushbuffer_size;

	/**
	 * Horizontal pixel increment for downscalers, in
	 * U5.20 format. I.e. 2.5 means downscaling
	 * by factor of 2.5. Corresponds to ISP_DM_H_PI register.
	 * This is needed by ISP Falcon firmware to program
	 * tile starting state correctly.
	 */
	uint32_t ds0_pixel_incr_h;
	uint32_t ds1_pixel_incr_h;
	uint32_t ds2_pixel_incr_h;

	/** ISP overfetch requirements */
	struct isp_overfetch overfetch;

	/** Reserved */
	uint32_t _pad1[3];

	/**
	 * Push buffer containing ISP settings related to this program.
	 * No relocations will be done for this push buffer; all registers
	 * that contain memory addresses that require relocation must be
	 * specified in the capture descriptor ISP payload.
	 */
	uint32_t pushbuffer[NVISP5_ISP_PROGRAM_PB_SIZE / sizeof(uint32_t)]
			CAPTURE_DESCRIPTOR_ALIGN;

} CAPTURE_DESCRIPTOR_ALIGN;

/**
 * @brief ISP Program ringbuffer element
 *
 * Each element in the ISP program ringbuffer contains a program descriptor immediately followed
 * isp program.
 */
struct isp5_program_entry {
	/** ISP capture descriptor */
	struct isp_program_descriptor prog_desc;
	/** ISP program buffer */
	struct isp5_program isp_prog;
} CAPTURE_DESCRIPTOR_ALIGN;

#pragma GCC diagnostic ignored "-Wpadded"

#endif /* INCLUDE_CAMRTC_CAPTURE_H */
