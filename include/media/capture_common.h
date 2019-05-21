/*
 * Tegra capture common operations
 *
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * Author: Sudhir Vyas <svyas@nvidia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __CAPTURE_COMMON_H__
#define __CAPTURE_COMMON_H__

#include <media/mc_common.h>

/* Progress status */
#define PROGRESS_STATUS_BUSY		(U32_C(0x1))
#define PROGRESS_STATUS_DONE		(U32_C(0x2))

#define BUFFER_READ	(U32_C(0x01))
#define BUFFER_WRITE	(U32_C(0x02))
#define BUFFER_ADD	(U32_C(0x04))
#define BUFFER_RDWR	(BUFFER_READ | BUFFER_WRITE)

/**
 * @struct capture_buffer_table
 * A structure holding buffer mapping relationship for a device
 */
struct capture_buffer_table;

/**
 * @brief Initialize the capture surface management table for SLAB allocations
 *
 * @param[in]	dev	Originating device (VI, ISP)
 * @returns	pointer to table on success, NULL on error
 */
struct capture_buffer_table *create_buffer_table(
	struct device *dev);

/**
 * @brief Release all capture buffers and free the management table
 *
 * @param[in,out]	tab	Surface buffer management table
 */
void destroy_buffer_table(
	struct capture_buffer_table *tab);

/**
 * @brief Perform a buffer management operation on a capture surface buffer
 *
 * @param[in,out]	tab	Surface buffer management table
 * @param[in]		memfd	FD or NvRm handle to buffer
 * @param[in]		flag	Surface BUFFER_* op bitmask
 * @returns		0 (success), errno (error)
 */
int capture_buffer_request(
	struct capture_buffer_table *tab,
	uint32_t memfd,
	uint32_t flag);

/**
 * @brief Add a capture surface buffer to the buffer management table
 *
 * @param[in,out]	t	Surface buffer management table
 * @param[in]		fd	FD or NvRm handle to buffer
 * @returns		0 (success), errno (error)
 */
static inline int capture_buffer_add(
	struct capture_buffer_table *t,
	uint32_t fd)
{
	return capture_buffer_request(t, fd, BUFFER_ADD | BUFFER_RDWR);
}

struct capture_mapping;

/**
 * @brief Decrement refcount for buffer mapping, and release it if it reaches
 * zero, unless it is a preserved mapping.
 *
 * @param[in,out]	t	Surface buffer management table
 * @param[in,out]	pin	Surface buffer to unpin
 */
void put_mapping(struct capture_buffer_table *t,
	struct capture_mapping *pin);

/**
 * @brief Capture surface buffer context
 */
struct capture_common_buf {
	struct dma_buf *buf; /**< dma_buf context */
	struct dma_buf_attachment *attach; /**< dma_buf attachment context */
	struct sg_table *sgt; /**< scatterlist table */
	dma_addr_t iova; /**< dma address */
};

/**
 * @brief List of buffers to unpin for a capture request
 */
struct capture_common_unpins {
	uint32_t num_unpins; /**< No. of entries in data[] */
	struct capture_mapping *data[]; /**< Surface buffers to unpin */
};

/**
 * @brief Pin and reloc struct for a capture request
 */
struct capture_common_pin_req {
	struct device *dev; /**< Originating device (vi, isp) */
	struct device *rtcpu_dev; /**< rtcpu device */
	struct capture_buffer_table *table; /**< Surface buffer mgmt. table */
	struct capture_common_unpins *unpins;
		/**< List of surface buffers to unpin */
	struct capture_common_buf *requests; /**< Capture descriptors queue */
	uint32_t request_size; /**< Size of single capture descriptor [byte] */
	uint32_t request_offset; /**< Offset to the capture descriptor [byte] */
	struct dma_buf *requests_mem; /**< Program descriptors (ISP) */
	uint32_t num_relocs; /**< No. of surface buffers to pin/reloc */
	uint32_t __user *reloc_user;
		/**<
		 * Offsets to surface buffer addresses to patch in capture
		 * descriptor
		 */
};

/**
 * @brief Progress status notifier handle
 */
struct capture_common_status_notifier {
	struct dma_buf *buf; /**< dma_buf handle */
	void *va; /**< buffer virtual mapping to kernel address space */
	uint32_t offset; /**< status notifier offset [byte] */
};

/**
 * @brief Setup the progress status notifier handle
 *
 * @param[in]	status_notifer	Progress status notifier handle
 * @param[in]	mem		FD or NvRm handle to buffer
 * @param[in]	buffer_size	Buffer size [byte]
 * @param[in]	mem_offset	Status notifier offset [byte]
 * @returns	0 (success), errno (error)
 */
int capture_common_setup_progress_status_notifier(
	struct capture_common_status_notifier *status_notifier,
	uint32_t mem,
	uint32_t buffer_size,
	uint32_t mem_offset);

/**
 * @brief Update the progress status for a capture request
 *
 * @param[in]	progress_status_notifier	Progress status notifier handle
 * @param[in]	buffer_slot			Capture descriptor index
 * @param[in]	buffer_depth			Capture descriptor queue size
 * @param[in]	new_val				Progress status to set
 * @returns	0 (success), errno (error)
 */
int capture_common_set_progress_status(
	struct capture_common_status_notifier *progress_status_notifier,
	uint32_t buffer_slot,
	uint32_t buffer_depth,
	uint8_t new_val);

/**
 * @brief Release the progress status notifier handle
 *
 * @param[in,out]	progress_status_notifier	Progress status notifier
 *							handle to release
 * @returns		0
 */
int capture_common_release_progress_status_notifier(
	struct capture_common_status_notifier *progress_status_notifier);

/**
 * @brief Pins buffer memory, returns dma_buf handles for unpinning
 *
 * @param[in]	dev		target device (rtcpu)
 * @param[in]	mem		FD or NvRm handle to buffer
 * @param[out]	unpin_data	struct w/ dma_buf handles for unpinning
 * @returns	0 (success), errno (error)
 */
int capture_common_pin_memory(struct device *dev,
	uint32_t mem, struct capture_common_buf *unpin_data);

/**
 * @brief Unpins buffer memory, releasing dma_buf resources.
 *
 * @param[in,out]	unpin_data	data handle to be unpinned
 */
void capture_common_unpin_memory(struct capture_common_buf *unpin_data);

/**
 * @brief Pins the physical address for each provided capture surface address
 * (no. of relocs) and patches the request capture descriptor with them.
 *
 * The returned req->unpins list is allocated and populated w/ each memory
 * pinning to be released upon completion of the capture.
 *
 * @param[in,out]	req	Capture descriptor capture_common_pin_req struct
 * @returns		0 (success), errno (error)
 */
int capture_common_request_pin_and_reloc(struct capture_common_pin_req *req);

#endif /* __CAPTURE_COMMON_H__*/
