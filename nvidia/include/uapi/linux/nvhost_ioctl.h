/*
 * include/uapi/linux/nvhost_ioctl.h
 *
 * Tegra graphics host driver
 *
 * Copyright (c) 2009-2022, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __UAPI_LINUX_NVHOST_IOCTL_H
#define __UAPI_LINUX_NVHOST_IOCTL_H

#include <linux/ioctl.h>
#include <linux/types.h>

#if !defined(__KERNEL__)
#define __user
#endif

#define NVHOST_INVALID_SYNCPOINT 0xFFFFFFFF
#define NVHOST_NO_TIMEOUT (-1)
#define NVHOST_NO_CONTEXT 0x0
#define NVHOST_IOCTL_MAGIC 'H'
#define NVHOST_PRIORITY_LOW 50
#define NVHOST_PRIORITY_MEDIUM 100
#define NVHOST_PRIORITY_HIGH 150

#define NVHOST_TIMEOUT_FLAG_DISABLE_DUMP	0

#define NVHOST_SUBMIT_VERSION_V0		0x0
#define NVHOST_SUBMIT_VERSION_V1		0x1
#define NVHOST_SUBMIT_VERSION_V2		0x2
#define NVHOST_SUBMIT_VERSION_MAX_SUPPORTED	NVHOST_SUBMIT_VERSION_V2

struct nvhost_cmdbuf {
	__u32 mem;
	__u32 offset;
	__u32 words;
} __packed;

struct nvhost_cmdbuf_ext {
	__s32 pre_fence;
	__u32 reserved;
};

struct nvhost_reloc {
	__u32 cmdbuf_mem;
	__u32 cmdbuf_offset;
	__u32 target;
	__u32 target_offset;
};

struct nvhost_reloc_shift {
	__u32 shift;
} __packed;

#define NVHOST_RELOC_TYPE_DEFAULT	0
#define NVHOST_RELOC_TYPE_PITCH_LINEAR	1
#define NVHOST_RELOC_TYPE_BLOCK_LINEAR	2
struct nvhost_reloc_type {
	__u32 reloc_type;
	__u32 padding;
};

struct nvhost_waitchk {
	__u32 mem;
	__u32 offset;
	__u32 syncpt_id;
	__u32 thresh;
};

struct nvhost_syncpt_incr {
	__u32 syncpt_id;
	__u32 syncpt_incrs;
};

struct nvhost_get_param_args {
	__u32 value;
} __packed;

struct nvhost_get_param_arg {
	__u32 param;
	__u32 value;
};

struct nvhost_get_client_managed_syncpt_arg {
	__u64 name;
	__u32 param;
	__u32 value;
};

struct nvhost_free_client_managed_syncpt_arg {
	__u32 param;
	__u32 value;
};

struct nvhost_channel_open_args {
	__s32 channel_fd;
};

struct nvhost_set_syncpt_name_args {
	__u64 name;
	__u32 syncpt_id;
	__u32 padding;
};

struct nvhost_set_nvmap_fd_args {
	__u32 fd;
} __packed;

enum nvhost_clk_attr {
	NVHOST_CLOCK = 0,
	NVHOST_BW,
	NVHOST_PIXELRATE,
	NVHOST_BW_KHZ,
};

/*
 * moduleid[15:0]  => module id
 * moduleid[24:31] => nvhost_clk_attr
 */
#define NVHOST_MODULE_ID_BIT_POS	0
#define NVHOST_MODULE_ID_BIT_WIDTH	16
#define NVHOST_CLOCK_ATTR_BIT_POS	24
#define NVHOST_CLOCK_ATTR_BIT_WIDTH	8
struct nvhost_clk_rate_args {
	__u32 rate;
	__u32 moduleid;
};

struct nvhost_set_timeout_args {
	__u32 timeout;
} __packed;

struct nvhost_set_timeout_ex_args {
	__u32 timeout;
	__u32 flags;
};

struct nvhost_set_priority_args {
	__u32 priority;
} __packed;

struct nvhost_set_error_notifier {
	__u64 offset;
	__u64 size;
	__u32 mem;
	__u32 padding;
};

struct nvhost32_ctrl_module_regrdwr_args {
	__u32 id;
	__u32 num_offsets;
	__u32 block_size;
	__u32 offsets;
	__u32 values;
	__u32 write;
};

struct nvhost_ctrl_module_regrdwr_args {
	__u32 id;
	__u32 num_offsets;
	__u32 block_size;
	__u32 write;
	__u64 offsets;
	__u64 values;
};

struct nvhost32_submit_args {
	__u32 submit_version;
	__u32 num_syncpt_incrs;
	__u32 num_cmdbufs;
	__u32 num_relocs;
	__u32 num_waitchks;
	__u32 timeout;
	__u32 syncpt_incrs;
	__u32 cmdbufs;
	__u32 relocs;
	__u32 reloc_shifts;
	__u32 waitchks;
	__u32 waitbases;
	__u32 class_ids;

	__u32 pad[2];		/* future expansion */

	__u32 fences;
	__u32 fence;		/* Return value */
} __packed;

#define NVHOST_SUBMIT_FLAG_SYNC_FENCE_FD	0
#define NVHOST_SUBMIT_MAX_NUM_SYNCPT_INCRS	10

struct nvhost_submit_args {
	__u32 submit_version;
	__u32 num_syncpt_incrs;
	__u32 num_cmdbufs;
	__u32 num_relocs;
	__u32 num_waitchks;
	__u32 timeout;
	__u32 flags;
	__u32 fence;		/* Return value */
	__u64 syncpt_incrs;
	__u64 cmdbuf_exts;

	__u32 checksum_methods;
	__u32 checksum_falcon_methods;

	__u64 pad[1];		/* future expansion */

	__u64 reloc_types;
	__u64 cmdbufs;
	__u64 relocs;
	__u64 reloc_shifts;
	__u64 waitchks;
	__u64 waitbases;
	__u64 class_ids;
	__u64 fences;
};

struct nvhost_set_ctxswitch_args {
	__u32 num_cmdbufs_save;
	__u32 num_save_incrs;
	__u32 save_incrs;
	__u32 save_waitbases;
	__u32 cmdbuf_save;
	__u32 num_cmdbufs_restore;
	__u32 num_restore_incrs;
	__u32 restore_incrs;
	__u32 restore_waitbases;
	__u32 cmdbuf_restore;
	__u32 num_relocs;
	__u32 relocs;
	__u32 reloc_shifts;

	__u32 pad;
};

struct nvhost_channel_buffer {
	__u32 dmabuf_fd;	/* in */
	__u32 reserved0;	/* reserved, must be 0 */
	__u64 reserved1[2];	/* reserved, must be 0 */
	__u64 address;		/* out, device view to the buffer */
};

struct nvhost_channel_unmap_buffer_args {
	__u32 num_buffers;	/* in, number of buffers to unmap */
	__u32 reserved;		/* reserved, must be 0 */
	__u64 table_address;	/* pointer to beginning of buffer */
};

struct nvhost_channel_map_buffer_args {
	__u32 num_buffers;	/* in, number of buffers to map */
	__u32 reserved;		/* reserved, must be 0 */
	__u64 table_address;	/* pointer to beginning of buffer */
};

#define NVHOST_IOCTL_CHANNEL_ATTACH_SYNCPT_ATTACH       (1 << 0)
struct nvhost_channel_attach_syncpt_args {
	__s32 syncpt_fd;
	__u32 flags;
};

#define NVHOST_IOCTL_CHANNEL_GET_SYNCPOINTS	\
	_IOR(NVHOST_IOCTL_MAGIC, 2, struct nvhost_get_param_args)
#define NVHOST_IOCTL_CHANNEL_GET_WAITBASES	\
	_IOR(NVHOST_IOCTL_MAGIC, 3, struct nvhost_get_param_args)
#define NVHOST_IOCTL_CHANNEL_GET_MODMUTEXES	\
	_IOR(NVHOST_IOCTL_MAGIC, 4, struct nvhost_get_param_args)
#define NVHOST_IOCTL_CHANNEL_SET_NVMAP_FD	\
	_IOW(NVHOST_IOCTL_MAGIC, 5, struct nvhost_set_nvmap_fd_args)
#define NVHOST_IOCTL_CHANNEL_NULL_KICKOFF	\
	_IOR(NVHOST_IOCTL_MAGIC, 6, struct nvhost_get_param_args)
#define NVHOST_IOCTL_CHANNEL_GET_CLK_RATE		\
	_IOWR(NVHOST_IOCTL_MAGIC, 9, struct nvhost_clk_rate_args)
#define NVHOST_IOCTL_CHANNEL_SET_CLK_RATE		\
	_IOW(NVHOST_IOCTL_MAGIC, 10, struct nvhost_clk_rate_args)
#define NVHOST_IOCTL_CHANNEL_SET_TIMEOUT	\
	_IOW(NVHOST_IOCTL_MAGIC, 11, struct nvhost_set_timeout_args)
#define NVHOST_IOCTL_CHANNEL_GET_TIMEDOUT	\
	_IOR(NVHOST_IOCTL_MAGIC, 12, struct nvhost_get_param_args)
#define NVHOST_IOCTL_CHANNEL_SET_PRIORITY	\
	_IOW(NVHOST_IOCTL_MAGIC, 13, struct nvhost_set_priority_args)
#define	NVHOST32_IOCTL_CHANNEL_MODULE_REGRDWR	\
	_IOWR(NVHOST_IOCTL_MAGIC, 14, struct nvhost32_ctrl_module_regrdwr_args)
#define NVHOST32_IOCTL_CHANNEL_SUBMIT		\
	_IOWR(NVHOST_IOCTL_MAGIC, 15, struct nvhost32_submit_args)
#define NVHOST_IOCTL_CHANNEL_GET_SYNCPOINT	\
	_IOWR(NVHOST_IOCTL_MAGIC, 16, struct nvhost_get_param_arg)
#define NVHOST_IOCTL_CHANNEL_GET_WAITBASE	\
	_IOWR(NVHOST_IOCTL_MAGIC, 17, struct nvhost_get_param_arg)
#define NVHOST_IOCTL_CHANNEL_SET_TIMEOUT_EX	\
	_IOWR(NVHOST_IOCTL_MAGIC, 18, struct nvhost_set_timeout_ex_args)
#define NVHOST_IOCTL_CHANNEL_GET_CLIENT_MANAGED_SYNCPOINT	\
	_IOWR(NVHOST_IOCTL_MAGIC, 19, struct nvhost_get_client_managed_syncpt_arg)
#define NVHOST_IOCTL_CHANNEL_FREE_CLIENT_MANAGED_SYNCPOINT	\
	_IOWR(NVHOST_IOCTL_MAGIC, 20, struct nvhost_free_client_managed_syncpt_arg)
#define NVHOST_IOCTL_CHANNEL_GET_MODMUTEX	\
	_IOWR(NVHOST_IOCTL_MAGIC, 23, struct nvhost_get_param_arg)
#define NVHOST_IOCTL_CHANNEL_SET_CTXSWITCH	\
	_IOWR(NVHOST_IOCTL_MAGIC, 25, struct nvhost_set_ctxswitch_args)

/* ioctls added for 64bit compatibility */
#define NVHOST_IOCTL_CHANNEL_SUBMIT	\
	_IOWR(NVHOST_IOCTL_MAGIC, 26, struct nvhost_submit_args)
#define	NVHOST_IOCTL_CHANNEL_MODULE_REGRDWR	\
	_IOWR(NVHOST_IOCTL_MAGIC, 27, struct nvhost_ctrl_module_regrdwr_args)

#define	NVHOST_IOCTL_CHANNEL_MAP_BUFFER	\
	_IOWR(NVHOST_IOCTL_MAGIC, 28, struct nvhost_channel_map_buffer_args)
#define	NVHOST_IOCTL_CHANNEL_UNMAP_BUFFER	\
	_IOWR(NVHOST_IOCTL_MAGIC, 29, struct nvhost_channel_unmap_buffer_args)

#define NVHOST_IOCTL_CHANNEL_SET_SYNCPOINT_NAME	\
	_IOW(NVHOST_IOCTL_MAGIC, 30, struct nvhost_set_syncpt_name_args)
#define NVHOST_IOCTL_CHANNEL_ATTACH_SYNCPT \
	_IOWR(NVHOST_IOCTL_MAGIC, 31, struct nvhost_channel_attach_syncpt_args)

#define NVHOST_IOCTL_CHANNEL_SET_ERROR_NOTIFIER  \
	_IOWR(NVHOST_IOCTL_MAGIC, 111, struct nvhost_set_error_notifier)
#define NVHOST_IOCTL_CHANNEL_OPEN	\
	_IOR(NVHOST_IOCTL_MAGIC,  112, struct nvhost_channel_open_args)

#define NVHOST_IOCTL_CHANNEL_LAST	\
	_IOC_NR(NVHOST_IOCTL_CHANNEL_OPEN)
#define NVHOST_IOCTL_CHANNEL_MAX_ARG_SIZE sizeof(struct nvhost_submit_args)

struct nvhost_ctrl_syncpt_read_args {
	__u32 id;
	__u32 value;
};

struct nvhost_ctrl_syncpt_incr_args {
	__u32 id;
} __packed;

struct nvhost_ctrl_syncpt_wait_args {
	__u32 id;
	__u32 thresh;
	__s32 timeout;
} __packed;

struct nvhost_ctrl_syncpt_waitex_args {
	__u32 id;
	__u32 thresh;
	__s32 timeout;
	__u32 value;
};

struct nvhost_ctrl_syncpt_waitmex_args {
	__u32 id;
	__u32 thresh;
	__s32 timeout;
	__u32 value;
	__u32 tv_sec;
	__u32 tv_nsec;
	__u32 clock_id;
	__u32 reserved;
};

struct nvhost_ctrl_sync_fence_info {
	__u32 id;
	__u32 thresh;
};

struct nvhost32_ctrl_sync_fence_create_args {
	__u32 num_pts;
	__u64 pts; /* struct nvhost_ctrl_sync_fence_info* */
	__u64 name; /* const char* */
	__s32 fence_fd; /* fd of new fence */
};

struct nvhost_ctrl_sync_fence_create_args {
	__u32 num_pts;
	__s32 fence_fd; /* fd of new fence */
	__u64 pts; /* struct nvhost_ctrl_sync_fence_info* */
	__u64 name; /* const char* */
};

struct nvhost_ctrl_sync_fence_name_args {
	__u64 name; /* const char* for name */
	__s32 fence_fd; /* fd of fence */
};

struct nvhost_ctrl_module_mutex_args {
	__u32 id;
	__u32 lock;
};

enum nvhost_module_id {
	NVHOST_MODULE_NONE = -1,
	NVHOST_MODULE_DISPLAY_A = 0,
	NVHOST_MODULE_DISPLAY_B,
	NVHOST_MODULE_VI,
	NVHOST_MODULE_VI2,
	NVHOST_MODULE_ISP,
	NVHOST_MODULE_ISPB,
	NVHOST_MODULE_MPE,
	NVHOST_MODULE_MSENC,
	NVHOST_MODULE_TSEC,
	NVHOST_MODULE_TSECB,
	NVHOST_MODULE_GPU,
	NVHOST_MODULE_VIC,
	NVHOST_MODULE_NVDEC,
	NVHOST_MODULE_NVJPG,
	NVHOST_MODULE_VII2C,
	NVHOST_MODULE_NVENC1,
	NVHOST_MODULE_NVDEC1,
	NVHOST_MODULE_NVCSI,
	NVHOST_MODULE_NVJPG1,
	NVHOST_MODULE_OFA,
	NVHOST_MODULE_INVALID
};

struct nvhost_characteristics {
#define NVHOST_CHARACTERISTICS_GFILTER (1 << 0)
#define NVHOST_CHARACTERISTICS_RESOURCE_PER_CHANNEL_INSTANCE (1 << 1)
#define NVHOST_CHARACTERISTICS_SUPPORT_PREFENCES (1 << 2)
	__u64 flags;

	__u32 num_mlocks;
	__u32 num_syncpts;

	__u32 syncpts_base;
	__u32 syncpts_limit;

	__u32 num_hw_pts;
	__u32 padding;
};

struct nvhost_ctrl_get_characteristics {
	__u64 nvhost_characteristics_buf_size;
	__u64 nvhost_characteristics_buf_addr;
};

struct nvhost_ctrl_check_module_support_args {
	__u32 module_id;
	__u32 value;
};

struct nvhost_ctrl_poll_fd_create_args {
	__s32 fd;
	__u32 padding;
};

struct nvhost_ctrl_poll_fd_trigger_event_args {
	__s32 fd;
	__u32 id;
	__u32 thresh;
	__u32 padding;
};

struct nvhost_ctrl_alloc_syncpt_args {
	__u32 flags; 		/* in  */
	__s32 fd;		/* out */
	__u32 syncpt_id;	/* out */
	__u32 padding;
};

struct nvhost_ctrl_sync_file_extract {
	__s32 fd;
	__u32 num_fences;
	__u64 fences_ptr;
};

struct nvhost_ctrl_syncpt_dmabuf_args {
	__s32 syncpt_fd;
	__s32 dmabuf_fd;
	__u32 is_full_shim;
	__u32 nb_syncpts;
	__u32 syncpt_page_size;
	__u32 padding;
};

#define NVHOST_IOCTL_CTRL_SYNCPT_READ		\
	_IOWR(NVHOST_IOCTL_MAGIC, 1, struct nvhost_ctrl_syncpt_read_args)
#define NVHOST_IOCTL_CTRL_SYNCPT_INCR		\
	_IOW(NVHOST_IOCTL_MAGIC, 2, struct nvhost_ctrl_syncpt_incr_args)
#define NVHOST_IOCTL_CTRL_SYNCPT_WAIT		\
	_IOW(NVHOST_IOCTL_MAGIC, 3, struct nvhost_ctrl_syncpt_wait_args)

#define NVHOST_IOCTL_CTRL_MODULE_MUTEX		\
	_IOWR(NVHOST_IOCTL_MAGIC, 4, struct nvhost_ctrl_module_mutex_args)
#define NVHOST32_IOCTL_CTRL_MODULE_REGRDWR	\
	_IOWR(NVHOST_IOCTL_MAGIC, 5, struct nvhost32_ctrl_module_regrdwr_args)

#define NVHOST_IOCTL_CTRL_SYNCPT_WAITEX		\
	_IOWR(NVHOST_IOCTL_MAGIC, 6, struct nvhost_ctrl_syncpt_waitex_args)

#define NVHOST_IOCTL_CTRL_GET_VERSION	\
	_IOR(NVHOST_IOCTL_MAGIC, 7, struct nvhost_get_param_args)

#define NVHOST_IOCTL_CTRL_SYNCPT_READ_MAX	\
	_IOWR(NVHOST_IOCTL_MAGIC, 8, struct nvhost_ctrl_syncpt_read_args)

#define NVHOST_IOCTL_CTRL_SYNCPT_WAITMEX	\
	_IOWR(NVHOST_IOCTL_MAGIC, 9, struct nvhost_ctrl_syncpt_waitmex_args)

#define NVHOST32_IOCTL_CTRL_SYNC_FENCE_CREATE	\
	_IOWR(NVHOST_IOCTL_MAGIC, 10, struct nvhost32_ctrl_sync_fence_create_args)
#define NVHOST_IOCTL_CTRL_SYNC_FENCE_CREATE	\
	_IOWR(NVHOST_IOCTL_MAGIC, 11, struct nvhost_ctrl_sync_fence_create_args)
#define NVHOST_IOCTL_CTRL_MODULE_REGRDWR	\
	_IOWR(NVHOST_IOCTL_MAGIC, 12, struct nvhost_ctrl_module_regrdwr_args)
#define NVHOST_IOCTL_CTRL_SYNC_FENCE_SET_NAME  \
	_IOWR(NVHOST_IOCTL_MAGIC, 13, struct nvhost_ctrl_sync_fence_name_args)
#define NVHOST_IOCTL_CTRL_GET_CHARACTERISTICS  \
	_IOWR(NVHOST_IOCTL_MAGIC, 14, struct nvhost_ctrl_get_characteristics)
#define NVHOST_IOCTL_CTRL_CHECK_MODULE_SUPPORT  \
	_IOWR(NVHOST_IOCTL_MAGIC, 15, struct nvhost_ctrl_check_module_support_args)
#define NVHOST_IOCTL_CTRL_POLL_FD_CREATE	\
	_IOR(NVHOST_IOCTL_MAGIC, 16, struct nvhost_ctrl_poll_fd_create_args)
#define NVHOST_IOCTL_CTRL_POLL_FD_TRIGGER_EVENT        \
	_IOW(NVHOST_IOCTL_MAGIC, 17, struct nvhost_ctrl_poll_fd_trigger_event_args)
#define NVHOST_IOCTL_CTRL_ALLOC_SYNCPT		\
	_IOWR(NVHOST_IOCTL_MAGIC, 18, struct nvhost_ctrl_alloc_syncpt_args)

#define NVHOST_IOCTL_CTRL_SYNC_FILE_EXTRACT     \
	_IOWR(NVHOST_IOCTL_MAGIC, 19, struct nvhost_ctrl_sync_file_extract)

#define NVHOST_IOCTL_CTRL_GET_SYNCPT_DMABUF_FD     \
	_IOWR(NVHOST_IOCTL_MAGIC, 20, struct nvhost_ctrl_syncpt_dmabuf_args)

#define NVHOST_IOCTL_CTRL_LAST			\
	_IOC_NR(NVHOST_IOCTL_CTRL_GET_SYNCPT_DMABUF_FD)
#define NVHOST_IOCTL_CTRL_MAX_ARG_SIZE	\
	sizeof(struct nvhost_ctrl_syncpt_waitmex_args)

#endif
