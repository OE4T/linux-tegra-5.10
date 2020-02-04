/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef DCE_CLIENT_IPC_INTERNAL_H
#define DCE_CLIENT_IPC_INTERNAL_H

#include <linux/platform/tegra/dce/dce-client-ipc.h>

struct tegra_dce_client_ipc {
	bool valid;
	void *data;
	uint32_t type;
	uint32_t int_type;
	struct tegra_dce *d;
	struct dce_ivc_channel *ch;
	struct completion recv_wait;
	tegra_dce_client_ipc_callback_t callback_fn;
};

void dce_client_ipc_wakeup(struct tegra_dce *d,	u32 ch_type);

int dce_client_ipc_wait(struct tegra_dce *d, u32 w_type, u32 ch_type);

#endif
