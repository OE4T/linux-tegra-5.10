/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION. All rights reserved.
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

#include "hsp-combo.h"

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/tegra-hsp.h>
#include <linux/sched/clock.h>

#include "soc/tegra/camrtc-commands.h"

struct camrtc_hsp {
	struct tegra_hsp_sm_pair *ivc_pair;
	struct device *parent;
	struct tegra_hsp_ss *ss;
	void (*group_notify)(struct device *dev, u16 group);
	struct tegra_hsp_sm_pair *cmd_pair;
	struct mutex mutex;
	struct completion emptied;
	wait_queue_head_t response_waitq;
	atomic_t response;
	long timeout;
};

static void camrtc_hsp_ss_notify(void *data, u32 value)
{
	struct camrtc_hsp *camhsp = data;
	u32 group = 0xFFFFU;

	if (camhsp->ss != NULL) {
		group = tegra_hsp_ss_status(camhsp->ss) >> 16U;
		tegra_hsp_ss_clr(camhsp->ss, group << 16U);
	}

	camhsp->group_notify(camhsp->parent, (u16)group);
}

void camrtc_hsp_group_ring(struct camrtc_hsp *camhsp,
		u16 group)
{
	if (camhsp->ss != NULL)
		tegra_hsp_ss_set(camhsp->ss, (u32)group);

	tegra_hsp_sm_pair_write(camhsp->ivc_pair, 1);
}
EXPORT_SYMBOL(camrtc_hsp_group_ring);

static void camrtc_hsp_full_notify(void *data, u32 value)
{
	struct camrtc_hsp *camhsp = data;

	atomic_set(&camhsp->response, value);
	wake_up(&camhsp->response_waitq);
}

static void camrtc_hsp_empty_notify(void *data, u32 empty_value)
{
	struct camrtc_hsp *camhsp = data;

	complete(&camhsp->emptied);
}

static long camrtc_hsp_wait_for_empty_pair(struct camrtc_hsp *camhsp,
		struct tegra_hsp_sm_pair *pair,
		long timeout)
{
	for (;;) {
		if (tegra_hsp_sm_pair_is_empty(pair))
			return timeout > 0 ? timeout : 1;

		if (timeout <= 0)
			return timeout;

		/*
		 * The reinit_completion() resets the completion to 0.
		 *
		 * The tegra_hsp_sm_pair_enable_empty_notify() guarantees that
		 * the empty notify gets called at least once even if the
		 * mailbox was already empty, so no empty events are missed
		 * even if the mailbox gets emptied between the calls to
		 * reinit_completion() and enable_empty_notify().
		 *
		 * The tegra_hsp_sm_pair_enable_empty_notify() may or may not
		 * do reference counting (on APE it does, elsewhere it does
		 * not). If the mailbox is initially empty, the emptied is
		 * already complete()d here, and the code ends up enabling
		 * empty notify twice, and when the mailbox gets empty,
		 * emptied gets complete() twice, and we always run the loop
		 * one extra time.
		 *
		 * Note that the complete() call above lets only one waiting
		 * task to run. The mailbox exchange is protected by a mutex,
		 * so only one task can be waiting here.
		 */
		reinit_completion(&camhsp->emptied);

		tegra_hsp_sm_pair_enable_empty_notify(pair);

		timeout = wait_for_completion_timeout(&camhsp->emptied, timeout);
	}
}

static int camrtc_hsp_send(struct camrtc_hsp *camhsp,
		int command, long *timeout)
{
	*timeout = camrtc_hsp_wait_for_empty_pair(camhsp,
			camhsp->cmd_pair, *timeout);
	if (*timeout <= 0) {
		dev_err(camhsp->parent,
			"command: 0x%08x: empty mailbox timeout\n", command);
		return -ETIMEDOUT;
	}

	atomic_set(&camhsp->response, -1);

	tegra_hsp_sm_pair_write(camhsp->cmd_pair, (u32)command);

	return 0;
}

static int camrtc_hsp_recv(struct camrtc_hsp *camhsp,
		int command, long *timeout)
{
	int response;

	*timeout = wait_event_timeout(
		camhsp->response_waitq,
		(response = atomic_xchg(&camhsp->response, -1)) >= 0,
		*timeout);
	if (*timeout <= 0) {
		dev_err(camhsp->parent,
			"command: 0x%08x: response timeout\n", command);
		return -ETIMEDOUT;
	}

	dev_dbg(camhsp->parent, "command: 0x%08x: response: 0x%08x\n",
		command, response);

	return response;
}

static int camrtc_hsp_sendrecv(struct camrtc_hsp *camhsp,
		int command, long *timeout)
{
	int ret = camrtc_hsp_send(camhsp, command, timeout);

	if (ret == 0)
		ret = camrtc_hsp_recv(camhsp, command, timeout);

	return ret;
}

static int camrtc_hsp_command(struct camrtc_hsp *camhsp,
		u32 command, long *timeout)
{
	int response;

	mutex_lock(&camhsp->mutex);
	response = camrtc_hsp_sendrecv(camhsp, command, timeout);
	mutex_unlock(&camhsp->mutex);

	return response;
}

static int camrtc_hsp_cmd_init(struct camrtc_hsp *camhsp, long *timeout)
{
	struct device *dev = camhsp->parent;
	u32 command = RTCPU_COMMAND(INIT, 0);
	int ret = camrtc_hsp_sendrecv(camhsp, command, timeout);

	if (ret < 0)
		return ret;

	if (ret != command) {
		dev_err(dev, "RTCPU sync problem (response=0x%08x)\n", ret);
		return -EIO;
	}

	return 0;
}

static int camrtc_hsp_cmd_fw_version(struct camrtc_hsp *camhsp, long *timeout)
{
	u32 command = RTCPU_COMMAND(FW_VERSION, RTCPU_DRIVER_SM5_VERSION);
	u32 response = camrtc_hsp_sendrecv(camhsp, command, timeout);

	if (response < 0)
		return response;

	if (RTCPU_GET_COMMAND_ID(response) != RTCPU_CMD_FW_VERSION ||
		RTCPU_GET_COMMAND_VALUE(response) < RTCPU_FW_SM4_VERSION) {
		dev_err(camhsp->parent,
			"RTCPU version mismatch (response=0x%08x)\n", response);
		return -EIO;
	}

	return (int)RTCPU_GET_COMMAND_VALUE(response);
}

/*
 * Resume: handshake and synchronize the HSP
 */
int camrtc_hsp_resume(struct camrtc_hsp *camhsp)
{
	long timeout = camhsp->timeout;
	int ret;

	mutex_lock(&camhsp->mutex);

	ret = camrtc_hsp_cmd_init(camhsp, &timeout);
	if (ret >= 0)
		ret = camrtc_hsp_cmd_fw_version(camhsp, &timeout);

	mutex_unlock(&camhsp->mutex);

	return ret;
}
EXPORT_SYMBOL(camrtc_hsp_resume);

/*
 * Suspend: set firmware to idle.
 */
int camrtc_hsp_suspend(struct camrtc_hsp *camhsp)
{
	long timeout = camhsp->timeout;
	u32 command = RTCPU_COMMAND(PM_SUSPEND, 0);
	int expect = RTCPU_COMMAND(PM_SUSPEND, RTCPU_PM_SUSPEND_SUCCESS);
	int err = camrtc_hsp_command(camhsp, command, &timeout);

	if (err == expect)
		return 0;

	dev_WARN(camhsp->parent, "PM_SUSPEND failed: 0x%08x\n", err);

	if (err >= 0)
		err = -EIO;
	return err;
}
EXPORT_SYMBOL(camrtc_hsp_suspend);

int camrtc_hsp_ch_setup(struct camrtc_hsp *camhsp, dma_addr_t iova)
{
	long timeout = camhsp->timeout;
	u32 command = RTCPU_COMMAND(CH_SETUP, iova >> 8);
	int response = camrtc_hsp_command(camhsp, command, &timeout);

	if (response < 0)
		return response;

	if (RTCPU_GET_COMMAND_ID(response) == RTCPU_CMD_ERROR) {
		u32 error = RTCPU_GET_COMMAND_VALUE(response);

		dev_dbg(camhsp->parent, "IOVM setup error: %u\n", error);

		return (int)error;
	}

	return 0;
}
EXPORT_SYMBOL(camrtc_hsp_ch_setup);

int camrtc_hsp_ping(struct camrtc_hsp *camhsp,
		u32 data, long timeout)
{
	u32 command = RTCPU_COMMAND(PING, data & 0xffffffU);
	int response;

	if (timeout == 0)
		timeout = camhsp->timeout;

	response = camrtc_hsp_command(camhsp, command, &timeout);
	if (response >= 0)
		response = RTCPU_GET_COMMAND_VALUE(response);

	return response;
}
EXPORT_SYMBOL(camrtc_hsp_ping);

int camrtc_hsp_get_fw_hash(struct camrtc_hsp *camhsp,
		u8 hash[], size_t hash_size)
{
	int ret, i;
	u32 value;
	long timeout = camhsp->timeout;

	for (i = 0; i < hash_size; i++) {
		ret = camrtc_hsp_command(camhsp, RTCPU_COMMAND(FW_HASH, i),
				&timeout);

		value = RTCPU_GET_COMMAND_VALUE(ret);

		if (ret < 0 ||
			RTCPU_GET_COMMAND_ID(ret) != RTCPU_CMD_FW_HASH ||
			value > 0xFFU) {
			dev_warn(camhsp->parent,
				"FW_HASH failed: 0x%08x\n", ret);
			return -EIO;
		}

		hash[i] = value;
	}

	return 0;
}
EXPORT_SYMBOL(camrtc_hsp_get_fw_hash);

struct camrtc_hsp *camrtc_hsp_create(
	struct device *dev,
	void (*group_notify)(struct device *dev, u16 group),
	long cmd_timeout)
{
	struct camrtc_hsp *camhsp;
	int ret = -EINVAL;
	struct device_node *np = dev->of_node;
	char const *obtain = "";

	camhsp = kzalloc(sizeof(*camhsp), GFP_KERNEL);
	if (camhsp == NULL)
		return ERR_PTR(-ENOMEM);

	camhsp->parent = get_device(dev);
	camhsp->group_notify = group_notify;
	camhsp->timeout = cmd_timeout;
	mutex_init(&camhsp->mutex);
	init_waitqueue_head(&camhsp->response_waitq);
	init_completion(&camhsp->emptied);
	atomic_set(&camhsp->response, -1);

	np = of_get_compatible_child(np, "nvidia,tegra186-hsp-mailbox");
	if (!of_device_is_available(np)) {
		of_node_put(np);
		ret = -ENODEV;
		goto fail;
	}

	camhsp->cmd_pair = of_tegra_hsp_sm_pair_by_name(np, obtain = "cmd-pair",
			camrtc_hsp_full_notify,
			camrtc_hsp_empty_notify,
			camhsp);
	if (IS_ERR(camhsp->cmd_pair)) {
		ret = PTR_ERR(camhsp->cmd_pair);
		goto put_fail;
	}

	camhsp->ivc_pair = of_tegra_hsp_sm_pair_by_name(np, obtain = "ivc-pair",
			camrtc_hsp_ss_notify,
			NULL,
			camhsp);
	if (IS_ERR(camhsp->ivc_pair)) {
		ret = PTR_ERR(camhsp->ivc_pair);
		goto put_fail;
	}

	of_node_put(np);

	return 0;

put_fail:
	if (ret != -EPROBE_DEFER)
		dev_err(dev, "%s: failed to obtain %s: %d\n",
			np->name, obtain, ret);
	of_node_put(np);
fail:
	camrtc_hsp_free(camhsp);

	return ERR_PTR(ret);
}
EXPORT_SYMBOL(camrtc_hsp_create);

void camrtc_hsp_free(struct camrtc_hsp *camhsp)
{
	if (IS_ERR_OR_NULL(camhsp))
		return;

	if (!IS_ERR_OR_NULL(camhsp->cmd_pair))
		tegra_hsp_sm_pair_free(camhsp->cmd_pair);
	if (!IS_ERR_OR_NULL(camhsp->ivc_pair))
		tegra_hsp_sm_pair_free(camhsp->ivc_pair);
	if (!IS_ERR_OR_NULL(camhsp->ss))
		tegra_hsp_ss_free(camhsp->ss);
	put_device(camhsp->parent);
	kfree(camhsp);
}
EXPORT_SYMBOL(camrtc_hsp_free);
