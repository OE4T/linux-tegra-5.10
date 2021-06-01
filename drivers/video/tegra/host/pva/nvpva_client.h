/*
 * Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NVPVA_CLIENT_H
#define NVPVA_CLIENT_H

#include <linux/kref.h>
#include <linux/mutex.h>
#include "pva_vpu_exe.h"

struct nvpva_client_context {
	/* PID of client process which uses this context */
	pid_t pid;

	/* This tracks how many queues active now */
	u32 active_queue_requests;

	/* Data structure to track pinned buffers for this client */
	struct nvhost_buffers *buffers;

	/* Data structure to track elf context for vpu parsing */
	struct nvpva_elf_context elf_ctx;
};

struct pva;
int nvpva_client_context_init(struct pva *pva);
void nvpva_client_context_deinit(struct pva *pva);
struct nvpva_client_context *nvpva_client_context_alloc(struct pva *pva,
							pid_t pid);
void nvpva_client_context_free(struct pva *pva,
			       struct nvpva_client_context *client);

#endif /* NVPVA_CLIENT_H */
