/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * /dev/nvhost-as-gpu device
 *
 * Opening a '/dev/nvhost-as-gpu' device node creates a new address
 * space. nvgpu channels (for the same module) can then be bound to such an
 * address space to define the addresses it has access to.
 *
 * Once a nvgpu channel has been bound to an address space it cannot be
 * unbound. There is no support for allowing an nvgpu channel to change from
 * one address space to another (or from one to none).
 *
 * As long as there is an open device file to the address space, or any bound
 * nvgpu channels it will be valid.  Once all references to the address space
 * are removed the address space is deleted.
 *
 */

#ifndef _UAPI__LINUX_NVGPU_AS_H__
#define _UAPI__LINUX_NVGPU_AS_H__

#include "nvgpu-uapi-common.h"

#define NVGPU_AS_IOCTL_MAGIC 'A'

/*
 * Allocating an address space range:
 *
 * Address ranges created with this ioctl are reserved for later use with
 * fixed-address buffer mappings.
 *
 * If _FLAGS_FIXED_OFFSET is specified then the new range starts at the 'offset'
 * given.  Otherwise the address returned is chosen to be a multiple of 'align.'
 *
 */
struct nvgpu32_as_alloc_space_args {
	__u32 pages;     /* in, pages */
	__u32 page_size; /* in, bytes */
	__u32 flags;     /* in */
#define NVGPU_AS_ALLOC_SPACE_FLAGS_FIXED_OFFSET 0x1
#define NVGPU_AS_ALLOC_SPACE_FLAGS_SPARSE 0x2
	union {
		__u64 offset; /* inout, byte address valid iff _FIXED_OFFSET */
		__u64 align;  /* in, alignment multiple (0:={1 or n/a}) */
	} o_a;
};

struct nvgpu_as_alloc_space_args {
	__u64 pages;     /* in, pages */
	__u32 page_size; /* in, bytes */
	__u32 flags;     /* in */
	union {
		__u64 offset; /* inout, byte address valid iff _FIXED_OFFSET */
		__u64 align;  /* in, alignment multiple (0:={1 or n/a}) */
	} o_a;
	__u32 padding[2];     /* in */
};

/*
 * Releasing an address space range:
 *
 * The previously allocated region starting at 'offset' is freed.  If there are
 * any buffers currently mapped inside the region the ioctl will fail.
 */
struct nvgpu_as_free_space_args {
	__u64 offset; /* in, byte address */
	__u64 pages;     /* in, pages */
	__u32 page_size; /* in, bytes */
	__u32 padding[3];
};

/*
 * Binding a nvgpu channel to an address space:
 *
 * A channel must be bound to an address space before allocating a gpfifo
 * in nvgpu.  The 'channel_fd' given here is the fd used to allocate the
 * channel.  Once a channel has been bound to an address space it cannot
 * be unbound (except for when the channel is destroyed).
 */
struct nvgpu_as_bind_channel_args {
	__u32 channel_fd; /* in */
};

/*
 * Mapping nvmap buffers into an address space:
 *
 * The start address is the 'offset' given if _FIXED_OFFSET is specified.
 * Otherwise the address returned is a multiple of 'align.'
 *
 * If 'page_size' is set to 0 the nvmap buffer's allocation alignment/sizing
 * will be used to determine the page size (largest possible).  The page size
 * chosen will be returned back to the caller in the 'page_size' parameter in
 * that case.
 */
#define NVGPU_AS_MAP_BUFFER_FLAGS_FIXED_OFFSET	    (1 << 0)
#define NVGPU_AS_MAP_BUFFER_FLAGS_CACHEABLE	    (1 << 2)
#define NVGPU_AS_MAP_BUFFER_FLAGS_IO_COHERENT	    (1 << 4)
#define NVGPU_AS_MAP_BUFFER_FLAGS_UNMAPPED_PTE	    (1 << 5)
#define NVGPU_AS_MAP_BUFFER_FLAGS_MAPPABLE_COMPBITS (1 << 6)
#define NVGPU_AS_MAP_BUFFER_FLAGS_L3_ALLOC          (1 << 7)
#define NVGPU_AS_MAP_BUFFER_FLAGS_DIRECT_KIND_CTRL  (1 << 8)
#define NVGPU_AS_MAP_BUFFER_FLAGS_PLATFORM_ATOMIC   (1 << 9)
#define NVGPU_AS_MAP_BUFFER_FLAGS_ACCESS_NO_WRITE   (1 << 10)

/*
 * VM map buffer IOCTL
 *
 * This ioctl maps a buffer - generally a dma_buf FD - into the VM's address
 * space. Usage of this API is as follows.
 *
 * @flags  [IN]
 *
 *   These are the flags passed to the IOCTL to modify the IOCTL behavior. The
 *   following flags are supported:
 *
 *   %NVGPU_AS_MAP_BUFFER_FLAGS_FIXED_OFFSET
 *
 *     Specify that the mapping already has an address. The mapping address
 *     must reside in an area already reserved with the as_alloc_space IOCTL.
 *     If this flag is set then the @offset field must be populated with the
 *     address to map to.
 *
 *   %NVGPU_AS_MAP_BUFFER_FLAGS_CACHEABLE
 *
 *     Specify that a mapping shall be GPU cachable.
 *
 *   %NVGPU_AS_MAP_BUFFER_FLAGS_IO_COHERENT
 *
 *     Specify that a mapping shall be IO coherent.
 *
 *     DEPRECATED: do not use! This will be removed in a future update.
 *
 *   %NVGPU_AS_MAP_BUFFER_FLAGS_UNMAPPED_PTE
 *
 *     Specify that a mapping shall be marked as invalid but otherwise
 *     populated. This flag doesn't actually make a lot of sense. The
 *     only reason to specify it is for testing replayable faults but
 *     an actual useful implementation of such a feature would likely
 *     not use this.
 *
 *     DEPRECATED: do not use! This will be removed in a future update.
 *
 *   %NVGPU_AS_MAP_BUFFER_FLAGS_MAPPABLE_COMPBITS
 *
 *     Deprecated.
 *
 *   %NVGPU_AS_MAP_BUFFER_FLAGS_DIRECT_KIND_CTRL
 *
 *     Set when userspace plans to pass in @compr_kind and @incompr_kind
 *     instead of letting the kernel work out kind fields.
 *
 *   %NVGPU_AS_MAP_BUFFER_FLAGS_PLATFORM_ATOMIC
 *
 *     Specify that a mapping should use platform atomics.
 *
 * @kind  [IN]
 *
 *   Specify the kind to use for the mapping.
 *
 * @compr_kind  [IN]
 * @incompr_kind  [IN]
 *
 *   Specify the compressible and incompressible kinds to be used for the
 *   mapping. Requires that %NVGPU_AS_MAP_BUFFER_FLAGS_DIRECT_KIND_CTRL is
 *   set in @flags. The kernel will attempt to use @comp_kind and if for
 *   some reason that is not possible will then fall back to using the
 *   @incompr_kind.
 *
 * @dmabuf_fd  [IN]
 *
 *   FD pointing to the dmabuf that will be mapped into the GMMU.
 *
 * @page_size  [IN]
 *
 *   Specify the page size for the mapping. Must be set to a valid, supported
 *   page size. If left unset this IOCTL will return -EINVAL. In general, a
 *   small page size mapping will always be supported, but in certain cases of
 *   compression this will not be the case.
 *
 * @buffer_offset  [IN]
 *
 *   Specify an offset into the physical buffer to begin the mapping at. For
 *   example imagine a DMA buffer 32KB long. However, you wish to only map
 *   this buffer starting at 8KB. In such a case you would pass 8KB as the
 *   @buffer_offset. This is only available with fixed address mappings. All
 *   regular (non-fixed) mappings require this field to be set to 0. This field
 *   is in bytes.
 *
 * @mapping_size  [IN]
 *
 *   The size of the mapping in bytes. This is from the @buffer_offset position.
 *   So for example, assuming you have a 32KB physical buffer and you want to
 *   map only 8KB of it, starting at some offset, then you would specify 8192 in
 *   this field. Of course this size + the buffer_offset must be less than the
 *   length of the physical buffer; otherwise -EINVAL is returned. This is only
 *   supported for fixed mappings.
 *
 * @offset  [IN, OUT]
 *
 *   The offset of the buffer in the GPU virtual address space. In other words
 *   the virtual address of the buffer. If the
 *   %NVGPU_AS_MAP_BUFFER_FLAGS_FIXED_OFFSET flag is set then this field must be
 *   populated by userspace. In all cases the ultimate mapped address is
 *   returned in this field. The field is in bytes.
 */
struct nvgpu_as_map_buffer_ex_args {
	/* NVGPU_AS_MAP_BUFFER_FLAGS_DIRECT_KIND_CTRL must be set */
	__u32 flags;		/* in/out */

	/*
	 * - If both compr_kind and incompr_kind are set
	 *   (i.e., value is other than NV_KIND_INVALID),
	 *   kernel attempts to use compr_kind first.
	 *
	 * - If compr_kind is set, kernel attempts to allocate
	 *   comptags for the buffer. If successful,
	 *   compr_kind is used as the PTE kind.
	 *
	 * - If incompr_kind is set, kernel uses incompr_kind as the
	 *   PTE kind, if compr_kind cannot be used. Comptags are not
	 *   allocated.
	 *
	 * - If neither compr_kind or incompr_kind is set, the
	 *   map call will fail.
	 */
#define NV_KIND_INVALID -1
	__s16 compr_kind;
	__s16 incompr_kind;

	__u32 dmabuf_fd;	/* in */
	__u32 page_size;	/* inout, 0:= best fit to buffer */

	__u64 buffer_offset;	/* in, offset of mapped buffer region */
	__u64 mapping_size;	/* in, size of mapped buffer region */

	__u64 offset;		/* in/out, we use this address if flag
				 * FIXED_OFFSET is set. This will fail
				 * if space is not properly allocated. The
				 * actual virtual address to which we mapped
				 * the buffer is returned in this field. */
};

/*
 * Get info about buffer compbits. Requires that buffer is mapped with
 * NVGPU_AS_MAP_BUFFER_FLAGS_MAPPABLE_COMPBITS.
 *
 * The compbits for a mappable buffer are organized in a mappable
 * window to the compbits store. In case the window contains comptags
 * for more than one buffer, the buffer comptag line index may differ
 * from the window comptag line index.
 */
struct nvgpu_as_get_buffer_compbits_info_args {

	/* in: address of an existing buffer mapping */
	__u64 mapping_gva;

	/* out: size of compbits mapping window (bytes) */
	__u64 compbits_win_size;

	/* out: comptag line index of the window start */
	__u32 compbits_win_ctagline;

	/* out: comptag line index of the buffer mapping */
	__u32 mapping_ctagline;

/* Buffer uses compbits */
#define NVGPU_AS_GET_BUFFER_COMPBITS_INFO_FLAGS_HAS_COMPBITS    (1 << 0)

/* Buffer compbits are mappable */
#define NVGPU_AS_GET_BUFFER_COMPBITS_INFO_FLAGS_MAPPABLE        (1 << 1)

/* Buffer IOVA addresses are discontiguous */
#define NVGPU_AS_GET_BUFFER_COMPBITS_INFO_FLAGS_DISCONTIG_IOVA  (1 << 2)

	/* out */
	__u32 flags;

	__u32 reserved1;
};

/*
 * Map compbits of a mapped buffer to the GPU address space. The
 * compbits mapping is automatically unmapped when the buffer is
 * unmapped.
 *
 * The compbits mapping always uses small pages, it is read-only, and
 * is GPU cacheable. The mapping is a window to the compbits
 * store. The window may not be exactly the size of the cache lines
 * for the buffer mapping.
 */
struct nvgpu_as_map_buffer_compbits_args {

	/* in: address of an existing buffer mapping */
	__u64 mapping_gva;

	/* in: gva to the mapped compbits store window when
	 * FIXED_OFFSET is set. Otherwise, ignored and should be be 0.
	 *
	 * For FIXED_OFFSET mapping:
	 * - If compbits are already mapped compbits_win_gva
	 *   must match with the previously mapped gva.
	 * - The user must have allocated enough GVA space for the
	 *   mapping window (see compbits_win_size in
	 *   nvgpu_as_get_buffer_compbits_info_args)
	 *
	 * out: gva to the mapped compbits store window */
	__u64 compbits_win_gva;

	/* in: reserved, must be 0
	   out: physical or IOMMU address for mapping */
	union {
		/* contiguous iova addresses */
		__u64 mapping_iova;

		/* buffer to receive discontiguous iova addresses (reserved) */
		__u64 mapping_iova_buf_addr;
	};

	/* in: Buffer size (in bytes) for discontiguous iova
	 * addresses. Reserved, must be 0. */
	__u64 mapping_iova_buf_size;

#define NVGPU_AS_MAP_BUFFER_COMPBITS_FLAGS_FIXED_OFFSET        (1 << 0)
	__u32 flags;
	__u32 reserved1;
};

/*
 * Unmapping a buffer:
 *
 * To unmap a previously mapped buffer set 'offset' to the offset returned in
 * the mapping call.  This includes where a buffer has been mapped into a fixed
 * offset of a previously allocated address space range.
 */
struct nvgpu_as_unmap_buffer_args {
	__u64 offset; /* in, byte address */
};


struct nvgpu_as_va_region {
	__u64 offset;
	__u32 page_size;
	__u32 reserved;
	__u64 pages;
};

struct nvgpu_as_get_va_regions_args {
	__u64 buf_addr; /* Pointer to array of nvgpu_as_va_region:s.
			 * Ignored if buf_size is 0 */
	__u32 buf_size; /* in:  userspace buf size (in bytes)
			   out: kernel buf size    (in bytes) */
	__u32 reserved;
};

struct nvgpu_as_map_buffer_batch_args {
	__u64 unmaps; /* ptr to array of nvgpu_as_unmap_buffer_args */
	__u64 maps;   /* ptr to array of nvgpu_as_map_buffer_ex_args */
	__u32 num_unmaps; /* in: number of unmaps
			   * out: on error, number of successful unmaps */
	__u32 num_maps;   /* in: number of maps
			   * out: on error, number of successful maps */
	__u64 reserved;
};

struct nvgpu_as_get_sync_ro_map_args {
	__u64 base_gpuva;
	__u32 sync_size;
	__u32 padding;
};

/*
 * VM mapping modify IOCTL
 *
 * This ioctl changes the kind of an existing mapped buffer region.
 *
 * Usage of this API is as follows.
 *
 * @compr_kind  [IN]
 *
 *   Specify the new compressed kind to be used for the mapping.  This
 *   parameter is only valid if compression resources are allocated to the
 *   underlying physical buffer. If NV_KIND_INVALID is specified then the
 *   fallback incompr_kind parameter is used.
 *
 * @incompr_kind  [IN]
 *
 *   Specify the new kind to be used for the mapping if compression is not
 *   to be used.  If NV_KIND_INVALID is specified then incompressible fallback
 *   is not allowed.
 *
 * @buffer_offset  [IN]
 *
 *   Specifies the beginning offset of the region within the existing buffer
 *   for which the kind should be modified.  This field is in bytes.
 *
 * @buffer_size  [IN]
 *
 *   Specifies the size of the region within the existing buffer for which the
 *   kind should be updated.  This field is in bytes.  Note that the region
 *   described by <buffer_offset, buffer_offset + buffer_size> must reside
 *   entirely within the existing buffer.
 *
 * @map_address  [IN]
 *
 *   The address of the existing buffer in the GPU virtual address space
 *   specified in bytes.
 */
struct nvgpu_as_mapping_modify_args {
	__s16 compr_kind;       /* in */
	__s16 incompr_kind;     /* in */

	__u64 buffer_offset;	/* in, offset of mapped buffer region */
	__u64 buffer_size;	/* in, size of mapped buffer region */

	__u64 map_address;	/* in, base virtual address of mapped buffer */
};

#define NVGPU_AS_IOCTL_BIND_CHANNEL \
	_IOWR(NVGPU_AS_IOCTL_MAGIC, 1, struct nvgpu_as_bind_channel_args)
#define NVGPU32_AS_IOCTL_ALLOC_SPACE \
	_IOWR(NVGPU_AS_IOCTL_MAGIC, 2, struct nvgpu32_as_alloc_space_args)
#define NVGPU_AS_IOCTL_FREE_SPACE \
	_IOWR(NVGPU_AS_IOCTL_MAGIC, 3, struct nvgpu_as_free_space_args)
#define NVGPU_AS_IOCTL_UNMAP_BUFFER \
	_IOWR(NVGPU_AS_IOCTL_MAGIC, 5, struct nvgpu_as_unmap_buffer_args)
#define NVGPU_AS_IOCTL_ALLOC_SPACE \
	_IOWR(NVGPU_AS_IOCTL_MAGIC, 6, struct nvgpu_as_alloc_space_args)
#define NVGPU_AS_IOCTL_MAP_BUFFER_EX \
	_IOWR(NVGPU_AS_IOCTL_MAGIC, 7, struct nvgpu_as_map_buffer_ex_args)
#define NVGPU_AS_IOCTL_GET_VA_REGIONS \
	_IOWR(NVGPU_AS_IOCTL_MAGIC, 8, struct nvgpu_as_get_va_regions_args)
#define NVGPU_AS_IOCTL_GET_BUFFER_COMPBITS_INFO \
	_IOWR(NVGPU_AS_IOCTL_MAGIC, 9, struct nvgpu_as_get_buffer_compbits_info_args)
#define NVGPU_AS_IOCTL_MAP_BUFFER_COMPBITS \
	_IOWR(NVGPU_AS_IOCTL_MAGIC, 10, struct nvgpu_as_map_buffer_compbits_args)
#define NVGPU_AS_IOCTL_MAP_BUFFER_BATCH	\
	_IOWR(NVGPU_AS_IOCTL_MAGIC, 11, struct nvgpu_as_map_buffer_batch_args)
#define NVGPU_AS_IOCTL_GET_SYNC_RO_MAP	\
	_IOR(NVGPU_AS_IOCTL_MAGIC,  12, struct nvgpu_as_get_sync_ro_map_args)
#define NVGPU_AS_IOCTL_MAPPING_MODIFY	\
	_IOWR(NVGPU_AS_IOCTL_MAGIC,  13, struct nvgpu_as_mapping_modify_args)

#define NVGPU_AS_IOCTL_LAST		\
	_IOC_NR(NVGPU_AS_IOCTL_MAPPING_MODIFY)
#define NVGPU_AS_IOCTL_MAX_ARG_SIZE	\
	sizeof(struct nvgpu_as_map_buffer_ex_args)

#endif
