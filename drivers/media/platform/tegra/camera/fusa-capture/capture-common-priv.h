/**
 * @file drivers/media/platform/tegra/camera/fusa-capture/capture-common-priv.h
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

#ifndef __FUSA_CAPTURE_COMMON_PRIV_H__
#define __FUSA_CAPTURE_COMMON_PRIV_H__

#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/nvhost.h>
#include <media/fusa-capture/capture-common.h>

#define fmt(_f) "%s:%d:" _f "\n", __func__, __LINE__

/**
 * @brief Capture buffer management table
 */
struct capture_buffer_table {
	struct device *dev; /**< Originating device (VI, ISP). */
	struct kmem_cache *cache; /**< SLAB allocator cache. */
	rwlock_t hlock; /**< Reader/writer lock on table contents. */
	DECLARE_HASHTABLE(hhead, 4U); /**< Buffer hashtable head. */
};

/**
 * @brief Capture surface NvRm and IOVA addresses handle
 */
union capture_surface {
	uint64_t raw; /**< Pinned VI or ISP IOVA address. */
	struct {
		uint32_t offset; /**< NvRm handle (upper 32 bits). */
		uint32_t hmem;
			/**<
			 * Offset of surface or pushbuffer address in descriptor
			 * (lower 32 bits) [byte].
			 */
	};
};

/**
 * @brief Capture buffer mapping (pinned)
 */
struct capture_mapping {
	struct hlist_node hnode; /**< Hash table node struct. */
	atomic_t refcnt; /**< Capture mapping reference count. */
	struct dma_buf *buf; /** Capture mapping dma_buf. */
	struct dma_buf_attachment *atch;
		/**< dma_buf attachment (VI or ISP device). */
	struct sg_table *sgt; /**< Scatterlist to dma_buf attachment. */
	unsigned int flag; /**< Bitmask access flag. */
};

/**
 * @brief Determine whether all the bits of @a other are set in @a self.
 *
 * @param[in]	self	Bitmask flag to be compared
 * @param[in]	other	Bitmask value(s) to compare
 * @returns	true (compatible), false (not compatible)
 */
static inline bool flag_compatible(
	unsigned int self,
	unsigned int other);

/**
 * @brief Determine whether BUFFER_RDWR is set in @a flag.
 *
 * @param[in]	flag	Bitmask flag to be compared
 * @returns	true (BUFFER_RDWR set), false (BUFFER_RDWR not set)
 */
static inline unsigned int flag_access_mode(
	unsigned int flag);

/**
 * @brief Map capture common buffer access flag to a Linux dma_data_direction.
 *
 * @param[in]	flag	Bitmask access flag of capture common buffer
 * @returns	true (compatible), false (not compatible)
 */
static inline enum dma_data_direction flag_dma_direction(
	unsigned int flag);

/**
 * @brief Retrieve the scatterlist IOVA address of the capture surface mapping.
 *
 * @param[in]	pin	The capture_mapping of the buffer
 * @returns	Physical address of scatterlist mapping
 */
static inline dma_addr_t mapping_iova(
	const struct capture_mapping *pin);

/**
 * @brief Retrieve the dma_buf pointer of a capture surface mapping.
 *
 * @param[in]	pin	The capture_mapping of the buffer
 * @returns	Pointer to the capture_mapping dma_buf
 */
static inline struct dma_buf *mapping_buf(
	const struct capture_mapping *pin);

/**
 * @brief Determine whether BUFFER_ADD is set in the capture surface mapping's
 *	  access flag.
 *
 * @param[in]	pin	The capture_mapping of the buffer
 * @returns	true (BUFFER_ADD set), false (BUFFER_ADD not set)
 */
static inline bool mapping_preserved(
	const struct capture_mapping *pin);

/**
 * @brief Set or unset the BUFFER_ADD bit in the capture surface mapping's
 *	  access flag, and correspondingly increment or decrement the mapping's
 *	  refcnt.
 *
 * @param[in]	pin	The capture_mapping of the buffer
 * @param[in]	val	The capture_mapping of the buffer
 * @returns	true (BUFFER_ADD set), false (BUFFER_ADD not set)
 */
static inline void set_mapping_preservation(
	struct capture_mapping *pin,
	bool val);

/**
 * @brief Iteratively search a capture buffer management table to find the entry
 *	  with @a buf, and @a flag bits set in the capture mapping.
 *
 * On success, the capture mapping is incremented by one if it is non-zero.
 *
 * @param[in]	tab	The capture buffer management table
 * @param[in]	buf	The mapping dma_buf pointer to match
 * @param[in]	flag	The mapping bitmask access flag to compare
 * @returns	capture_mapping pointer (success), NULL (failure)
 */
static struct capture_mapping *find_mapping(
	struct capture_buffer_table *tab,
	struct dma_buf *buf,
	unsigned int flag);

/**
 * @brief Add an NvRm buffer to the buffer management table and initialize its
 *	  refcnt to 1.
 *
 * @param[in]	tab	The capture buffer management table
 * @param[in]	fd	The NvRm handle
 * @param[in]	flag	The mapping bitmask access flag to set
 * @returns	capture_mapping pointer (success), PTR_ERR (failure)
 */
static struct capture_mapping *get_mapping(
	struct capture_buffer_table *tab,
	uint32_t fd,
	unsigned int flag);

#endif /* __FUSA_CAPTURE_COMMON_PRIV_H__ */
