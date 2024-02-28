/*
 * Copyright (c) 2016-2022, NVIDIA CORPORATION & AFFILIATES.  All rights reserved.
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

#ifndef _TEGRA_ADSPFF_H_
#define _TEGRA_ADSPFF_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
* Defines
******************************************************************************/


/* TODO: fine tuning */
#define ADSPFF_MSG_QUEUE_WSIZE		1024
#define ADSPFF_WRITE_DATA_SIZE		512
#define ADSPFF_READ_DATA_SIZE		1024
#define ADSPFF_SHARED_BUFFER_SIZE	(128 * 1024)
#define ADSPFF_MAX_FILENAME_SIZE	(250)

/**
 * adspff_mbx_cmd:  commands exchanged using mailbox.
 *
 * @adspff_cmd_fopen:           open file on host
 * @adspff_cmd_fclose:		close file on host
 * @adspff_cmd_fwrite:		write data in an open file on host
 * @adspff_cmd_fread:		read data from an open file on host
 */

enum adspff_mbx_cmd {
	adspff_cmd_fopen = 0,
	adspff_cmd_fclose,
	adspff_cmd_fwrite,
	adspff_cmd_fread,
	adspff_cmd_fopen_recv,
	adspff_cmd_ack,
	adspff_cmd_fsize,
};


/******************************************************************************
* Types
******************************************************************************/

/* supported message payloads */
struct fopen_msg_t {
	uint8_t fname[ADSPFF_MAX_FILENAME_SIZE];
	uint8_t modes[3];
};

struct fwrite_msg_t {
	int64_t file;
	int32_t size;
};

struct fread_msg_t {
	int64_t file;
	int32_t size;
};

struct fclose_msg_t {
	int64_t file;
};

struct fopen_recv_msg_t {
	int64_t file;
};

struct fsize_msg_t {
	int64_t file;
};

struct ack_msg_t {
	int32_t size;
};

#pragma pack(4)
/* app message definition */
union adspff_message_t {
	msgq_message_t msgq_msg;
	struct {
		int32_t header[MSGQ_MESSAGE_HEADER_WSIZE];
		union {
			struct fopen_msg_t fopen_msg;
			struct fwrite_msg_t fwrite_msg;
			struct fread_msg_t	fread_msg;
			struct fclose_msg_t fclose_msg;
			struct fopen_recv_msg_t fopen_recv_msg;
			struct ack_msg_t ack_msg;
			struct fsize_msg_t fsize_msg;
		} payload;
	} msg;
};

/* app queue definition */
union adspff_msgq_t {
	msgq_t msgq;
	struct {
		int32_t header[MSGQ_HEADER_WSIZE];
		int32_t queue[ADSPFF_MSG_QUEUE_WSIZE];
	} app_msgq;
};
#pragma pack()

#define MSGQ_MSG_SIZE(x) \
(((sizeof(x) + sizeof(int32_t) - 1) & (~(sizeof(int32_t)-1))) >> 2)


/**
 * ADSPFF state structure shared between ADSP & CPU
 */
typedef struct {
	uint32_t write_index;
	uint32_t read_index;
	uint8_t data[ADSPFF_SHARED_BUFFER_SIZE];
} adspff_shared_buffer_t;

struct adspff_shared_state_t {
	uint16_t mbox_id;
	union adspff_msgq_t msgq_recv;
	union adspff_msgq_t msgq_send;
	adspff_shared_buffer_t write_buf;
	adspff_shared_buffer_t read_buf;
};

#define ADSPFF_SHARED_STATE(x) \
((struct adspff_shared_state_t *)x)

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef TEGRA_ADSPFF_H_ */
