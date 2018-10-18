/*
 * Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
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
#include "soc/tegra/camrtc-commands.h"

struct camrtc_hsp {
	struct device *parent;
	struct tegra_hsp_sm_pair *ivc_pair;
	struct tegra_hsp_ss *ss;
	void (*group_notify)(struct device *dev, u16 group);
	struct tegra_hsp_sm_pair *cmd_pair;
	struct mutex mutex;
	struct completion emptied;
	wait_queue_head_t response_waitq;
	atomic_t response;
	u32 timeout;
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

static void camrtc_hsp_full_notify(void *data, u32 response)
{
	struct camrtc_hsp *camhsp = data;

	atomic_set(&camhsp->response, response);
	wake_up(&camhsp->response_waitq);
}

static void camrtc_hsp_empty_notify(void *data, u32 empty_value)
{
	struct camrtc_hsp *camhsp = data;

	complete(&camhsp->emptied);
}

static long camrtc_hsp_wait_for_empty(struct camrtc_hsp *camhsp,
				long timeout)
{
	for (;;) {
		if (tegra_hsp_sm_pair_is_empty(camhsp->cmd_pair))
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
		 * task to run. The mailbxo exchange is protected by a mutex,
		 * so only one task can be waiting here.
		 */
		reinit_completion(&camhsp->emptied);
		tegra_hsp_sm_pair_enable_empty_notify(camhsp->cmd_pair);

		timeout = wait_for_completion_timeout(&camhsp->emptied, timeout);
	}
}

static int camrtc_hsp_mbox_exchange(struct camrtc_hsp *camhsp,
					u32 command, long *timeout)
{
#define INVALID_RESPONSE (0x80000000U)

	*timeout = camrtc_hsp_wait_for_empty(camhsp, *timeout);
	if (*timeout <= 0) {
		dev_err(camhsp->parent, "command: 0x%08x: empty mailbox timeout\n",
			command);
		return -ETIMEDOUT;
	}

	atomic_set(&camhsp->response, INVALID_RESPONSE);

	tegra_hsp_sm_pair_write(camhsp->cmd_pair, command);

	*timeout = wait_event_timeout(
		camhsp->response_waitq,
		atomic_read(&camhsp->response) != INVALID_RESPONSE,
		*timeout);
	if (*timeout <= 0) {
		dev_err(camhsp->parent, "command: 0x%08x: response timeout\n",
			command);
		return -ETIMEDOUT;
	}

	return (int)atomic_read(&camhsp->response);
}

int camrtc_hsp_command(struct camrtc_hsp *camhsp,
		u32 command, long *timeout)
{
	int response;
	long left = 2 * HZ;

	if (timeout == NULL)
		timeout = &left;

	mutex_lock(&camhsp->mutex);
	response = camrtc_hsp_mbox_exchange(camhsp, command, timeout);
	mutex_unlock(&camhsp->mutex);

	return response;
}
EXPORT_SYMBOL(camrtc_hsp_command);

int camrtc_hsp_prefix_command(struct camrtc_hsp *camhsp,
		u32 prefix, u32 command, long *timeout)
{
	int response;
	long left = 2 * HZ;

	if (timeout == NULL)
		timeout = &left;

	mutex_lock(&camhsp->mutex);

	prefix = RTCPU_COMMAND(PREFIX, prefix);
	response = camrtc_hsp_mbox_exchange(camhsp, prefix, timeout);

	if (RTCPU_GET_COMMAND_ID(response) == RTCPU_CMD_PREFIX)
		response = camrtc_hsp_mbox_exchange(camhsp, command, timeout);

	mutex_unlock(&camhsp->mutex);

	return response;
}
EXPORT_SYMBOL(camrtc_hsp_prefix_command);

struct camrtc_hsp *camrtc_hsp_create(
	struct device *dev,
	void (*group_notify)(struct device *dev, u16 group))
{
	struct camrtc_hsp *camhsp;
	struct device_node *hsp_node;
	int ret = -EINVAL;

	hsp_node = of_get_child_by_name(dev->of_node, "hsp");
	if (hsp_node == NULL)
		return ERR_PTR(-ENOENT);

	camhsp = kzalloc(sizeof(*camhsp), GFP_KERNEL);
	if (camhsp == NULL)
		return ERR_PTR(-ENOMEM);

	camhsp->parent = get_device(dev);
	camhsp->group_notify = group_notify;
	mutex_init(&camhsp->mutex);
	init_waitqueue_head(&camhsp->response_waitq);
	init_completion(&camhsp->emptied);
	atomic_set(&camhsp->response, INVALID_RESPONSE);

	camhsp->cmd_pair = of_tegra_hsp_sm_pair_by_name(hsp_node,
			"cmd-pair", camrtc_hsp_full_notify,
			camrtc_hsp_empty_notify, camhsp);
	if (IS_ERR(camhsp->cmd_pair)) {
		ret = PTR_ERR(camhsp->cmd_pair);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "failed to obtain cmd-pair: %d\n", ret);
		camhsp->cmd_pair = NULL;
		goto fail;
	}

	camhsp->ivc_pair = of_tegra_hsp_sm_pair_by_name(hsp_node,
			"ivc-pair", camrtc_hsp_ss_notify, NULL, camhsp);
	if (IS_ERR(camhsp->ivc_pair)) {
		ret = PTR_ERR(camhsp->ivc_pair);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "failed to obtain ivc-pair: %d\n", ret);
		camhsp->ivc_pair = NULL;
		goto fail;
	}

	camhsp->ss = of_tegra_hsp_ss_by_name(hsp_node, "ivc-ss");
	if (IS_ERR(camhsp->ss)) {
		ret = PTR_ERR(camhsp->ss);
		if (ret != -EPROBE_DEFER && ret != -EINVAL)
			dev_err(dev, "failed to obtain ivc-ss: %d\n", ret);
		camhsp->ss = NULL;
		if (ret != -EINVAL)
			goto fail;
	}

	of_node_put(hsp_node);
	return camhsp;

fail:
	camrtc_hsp_free(camhsp);
	of_node_put(hsp_node);
	return ERR_PTR(ret);
}
EXPORT_SYMBOL(camrtc_hsp_create);

void camrtc_hsp_free(struct camrtc_hsp *camhsp)
{
	if (IS_ERR_OR_NULL(camhsp))
		return;

	tegra_hsp_sm_pair_free(camhsp->cmd_pair);
	tegra_hsp_sm_pair_free(camhsp->ivc_pair);
	tegra_hsp_ss_free(camhsp->ss);
	put_device(camhsp->parent);
	kfree(camhsp);
}
EXPORT_SYMBOL(camrtc_hsp_free);
