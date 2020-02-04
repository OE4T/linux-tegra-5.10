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

#include <dce.h>
#include <dce-ipc.h>
#include <dce-util-common.h>
#include <dce-client-ipc-internal.h>

#define DCE_IPC_HANDLES_MAX 6U
#define DCE_CLIENT_IPC_HANDLE_INVALID 0U
#define DCE_CLIENT_IPC_HANDLE_VALID ((u32)BIT(31))

struct tegra_dce_client_ipc client_handles[DCE_CLIENT_IPC_TYPE_MAX];

static uint32_t dce_interface_type_map[DCE_CLIENT_IPC_TYPE_MAX] = {
	[DCE_CLIENT_IPC_TYPE_CPU_RM] = DCE_IPC_TYPE_DISPRM,
	[DCE_CLIENT_IPC_TYPE_HDCP_KMD] = DCE_IPC_TYPE_HDCP,
};

static inline uint32_t dce_client_get_type(uint32_t int_type)
{
	uint32_t lc = 0;

	for (lc = 0; lc < DCE_CLIENT_IPC_TYPE_MAX; lc++)
		if (dce_interface_type_map[lc] == int_type)
			break;

	return lc;
}

static inline u32 client_handle_to_index(u32 handle)
{
	return (u32)(handle & ~DCE_CLIENT_IPC_HANDLE_VALID);
}

static inline bool is_client_handle_valid(u32 handle)
{
	bool valid = false;

	if ((handle & DCE_CLIENT_IPC_HANDLE_VALID) == 0U)
		goto out;

	if (client_handle_to_index(handle) >= DCE_CLIENT_IPC_TYPE_MAX)
		goto out;

	valid = true;

out:
	return valid;
}

struct tegra_dce_client_ipc *dce_client_ipc_lookup_handle(u32 handle)
{
	struct tegra_dce_client_ipc *cl = NULL;

	if (!is_client_handle_valid(handle))
		goto out;

	cl = &client_handles[client_handle_to_index(handle)];

out:
	return cl;
}


static int dce_client_ipc_handle_alloc(u32 *handle)
{
	u32 index;
	int ret = -1;

	if (handle == NULL)
		return ret;

	for (index = 0; index < DCE_CLIENT_IPC_TYPE_MAX; index++) {
		if (client_handles[index].valid == false) {
			client_handles[index].valid = true;
			*handle = (u32)(index | DCE_CLIENT_IPC_HANDLE_VALID);
			ret = 0;
			break;
		}
	}

	return ret;
}

static int dce_client_ipc_handle_free(u32 handle)
{
	struct tegra_dce *d;
	struct tegra_dce_client_ipc *cl;

	if (!is_client_handle_valid(handle))
		return -EINVAL;

	cl = &client_handles[client_handle_to_index(handle)];

	if (cl->valid == false)
		return -EINVAL;

	d = cl->d;
	d->d_clients[cl->type] = NULL;
	memset(cl, 0, sizeof(struct tegra_dce_client_ipc));

	return 0;
}

int tegra_dce_register_ipc_client(u32 type,
		tegra_dce_client_ipc_callback_t callback_fn,
		void *data, u32 *handlep)
{
	int ret;
	uint32_t int_type;
	struct tegra_dce *d;
	struct tegra_dce_client_ipc *cl;
	u32 handle = DCE_CLIENT_IPC_HANDLE_INVALID;

	int_type = dce_interface_type_map[type];

	d = dce_ipc_get_dce_from_ch(int_type);
	if (d == NULL) {
		ret = -EINVAL;
		goto out;
	}

	if (handlep == NULL) {
		dce_err(d, "Invalid handle pointer");
		ret = -EINVAL;
		goto out;
	}

	ret = dce_client_ipc_handle_alloc(&handle);
	if (ret)
		goto out;

	cl = &client_handles[client_handle_to_index(handle)];

	cl->d = d;
	cl->type = type;
	cl->data = data;
	cl->int_type = int_type;
	cl->callback_fn = callback_fn;

	d->d_clients[type] = cl;
	init_completion(&cl->recv_wait);

out:
	if (ret != 0) {
		dce_client_ipc_handle_free(handle);
		handle = DCE_CLIENT_IPC_HANDLE_INVALID;
	}

	*handlep = handle;

	return ret;
}
EXPORT_SYMBOL(tegra_dce_register_ipc_client);

int tegra_dce_unregister_ipc_client(u32 handle)
{
	return dce_client_ipc_handle_free(handle);
}
EXPORT_SYMBOL(tegra_dce_unregister_ipc_client);

int tegra_dce_client_ipc_send_recv(u32 handle, struct dce_ipc_message *msg)
{
	int ret;
	struct tegra_dce_client_ipc *cl;

	if (msg == NULL) {
		ret = -1;
		goto out;
	}

	cl = dce_client_ipc_lookup_handle(handle);
	if (cl == NULL) {
		ret = -1;
		goto out;
	}

	ret = dce_ipc_send_message_sync(cl->d, cl->int_type, msg);

out:
	return ret;
}
EXPORT_SYMBOL(tegra_dce_client_ipc_send_recv);

static int dce_client_ipc_wait_rpc(struct tegra_dce *d, u32 int_type)
{
	uint32_t type;
	struct tegra_dce_client_ipc *cl;

	type = dce_client_get_type(int_type);
	if (type >= DCE_CLIENT_IPC_TYPE_MAX) {
		dce_err(d, "Failed to retrieve client info for int_type: [%d]",
			int_type);
		return -EINVAL;
	}

	cl = d->d_clients[type];
	if ((cl == NULL) || (cl->int_type != int_type)) {
		dce_err(d, "Failed to retrieve client info for int_type: [%d]",
			int_type);
		return -EINVAL;
	}

	wait_for_completion(&cl->recv_wait);

	return 0;
}

int dce_client_ipc_wait(struct tegra_dce *d, u32 w_type, u32 ch_type)
{
	int ret = 0;

	switch (w_type) {
	case DCE_IPC_WAIT_TYPE_SYNC:
		ret = dce_admin_ipc_wait(d, w_type);
		break;
	case DCE_IPC_WAIT_TYPE_RPC:
		ret = dce_client_ipc_wait_rpc(d, ch_type);
		break;
	default:
		dce_err(d, "Invalid wait type [%d]", w_type);
		break;
	}

	return ret;
}

void dce_client_ipc_wakeup(struct tegra_dce *d, u32 ch_type)
{
	uint32_t type;
	struct tegra_dce_client_ipc *cl;

	type = dce_client_get_type(ch_type);
	if (type == DCE_CLIENT_IPC_TYPE_MAX) {
		dce_err(d, "Failed to retrieve client info for ch_type: [%d]",
			ch_type);
		return;
	}

	cl = d->d_clients[type];
	if ((cl == NULL) || (cl->int_type != ch_type)) {
		dce_err(d, "Failed to retrieve client info for ch_type: [%d]",
			ch_type);
		return;
	}

	complete(&cl->recv_wait);
}
