/*
 * adsp_shared_struct.h
 *
 * A header file containing shared data structures shared with ADSP OS
 *
 * Copyright (C) 2015-2021 NVIDIA Corporation. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ADSP_SHARED_STRUCT
#define __ADSP_SHARED_STRUCT
#include <linux/tegra_nvadsp.h>

#define APP_LOADER_MBOX_ID		1

#define ADSP_APP_FLAG_START_ON_BOOT	0x1

#define ADSP_OS_LOAD_TIMEOUT		5000 /* 5000 ms */

#define DRAM_DEBUG_LOG_SIZE		0x4000 /* 16 KB */

#define NVADSP_NAME_SZ			128

struct app_mem_size {
	uint64_t dram;
	uint64_t dram_shared;
	uint64_t dram_shared_wc;
	uint64_t aram;
	uint64_t aram_x;
} __packed;

struct adsp_shared_app {
	char name[NVADSP_NAME_SZ];
	struct app_mem_size mem_size;
	int32_t mod_ptr;
	int32_t flags;
	int32_t dram_data_ptr;
	int32_t shared_data_ptr;
	int32_t shared_wc_data_ptr;
	char version[16];
} __packed;

/* ADSP app loader message queue */
struct run_app_instance_data {
	uint32_t adsp_mod_ptr;
	uint64_t host_ref;
	uint32_t adsp_ref;
	uint32_t dram_data_ptr;
	uint32_t dram_shared_ptr;
	uint32_t dram_shared_wc_ptr;
	uint32_t aram_ptr;
	uint32_t aram_flag;
	uint32_t aram_x_ptr;
	uint32_t aram_x_flag;
	struct app_mem_size mem_size;
	nvadsp_app_args_t app_args;
	uint32_t stack_size;
	uint32_t message;
} __packed;

struct app_loader_data {
	int32_t header[MSGQ_MESSAGE_HEADER_WSIZE];
	struct run_app_instance_data	app_init;
} __packed;

union app_loader_message {
	msgq_message_t msgq_msg;
	struct app_loader_data data;
} __aligned(4);

struct adsp_os_message_header {
	int32_t				header[MSGQ_MESSAGE_HEADER_WSIZE];
	uint32_t			message;
} __packed;

/* ADSP app complete message queue */
struct app_complete_status_data {
	struct adsp_os_message_header	header;
	uint64_t			host_ref;
	uint32_t			adsp_ref;
	int32_t				status;
} __packed;

struct adsp_static_app_data {
	struct adsp_os_message_header	header;
	struct adsp_shared_app		shared_app;
} __packed;

union app_complete_status_message {
	msgq_message_t			msgq_msg;
	struct app_complete_status_data	complete_status_data;
	struct adsp_static_app_data static_app_data;
} __aligned(4);


/*ADSP message pool structure */
#define ADSP_MAX_MSGQ_SIZE		8192
#define ADSP_MAX_MSGQ_WSIZE		(ADSP_MAX_MSGQ_SIZE / sizeof(int32_t))
#define ADSP_MSGQ_MAX_QUEUE_WSIZE	\
	(ADSP_MAX_MSGQ_WSIZE - (int32_t)MSGQ_HEADER_WSIZE)

union app_loader_msgq {
	msgq_t msgq;
	struct {
		int32_t header[MSGQ_HEADER_WSIZE];
		int32_t queue[ADSP_MSGQ_MAX_QUEUE_WSIZE];
	};
};

/* ADSP APP shared message pool */
#pragma pack(8)
struct nvadsp_app_shared_msg_pool {
	union app_loader_msgq		app_loader_send_message;
	union app_loader_msgq		app_loader_recv_message;
};
#pragma pack()

/*ADSP shated OS args */
struct nvadsp_os_args {
	uint64_t        adsp_freq_hz;
	int32_t		timer_prescalar;
	char		logger[DRAM_DEBUG_LOG_SIZE];
	uint32_t        dynamic_app_support;
	uint32_t	chip_id;
	char		reserved[120];
} __packed;

/* ARM MODE REGS */
struct arm_mode_regs_shared {
	uint32_t fiq_r13, fiq_r14;
	uint32_t irq_r13, irq_r14;
	uint32_t svc_r13, svc_r14;
	uint32_t abt_r13, abt_r14;
	uint32_t und_r13, und_r14;
	uint32_t sys_r13, sys_r14;
} __packed;

/* ARM FAULT FRAME  */
struct arm_fault_frame_shared {
	uint32_t spsr;
	uint32_t usp;
	uint32_t ulr;
	uint32_t r[13];
	uint32_t pc;
} __packed;

/* ADSP ARM EXCEPTION CONTEXT  */
struct nvadsp_exception_context {
	struct arm_fault_frame_shared frame;
	struct arm_mode_regs_shared regs;
	uint32_t stack_addr;
	uint32_t stack_dump[32];
	uint32_t exception_reason;
} __packed;

/* ADSP OS info/status. Keep in sync with firmware. */
#define MAX_OS_VERSION_BUF    32
struct nvadsp_os_info {
	char            version[MAX_OS_VERSION_BUF];
	char            reserved[128];
} __packed;

/* ADSP OS shared memory */
#pragma pack(8)
struct nvadsp_shared_mem {
	struct nvadsp_app_shared_msg_pool app_shared_msg_pool;
	struct nvadsp_os_args os_args;
	struct nvadsp_os_info os_info;
	struct nvadsp_exception_context exception_context;
};
#pragma pack()


#endif /* __ADSP_SHARED_STRUCT */
