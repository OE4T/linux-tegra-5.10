// SPDX-License-Identifier: GPL-2.0
/*
 * This file is part of NVIDIA MODS kernel driver.
 *
 * Copyright (c) 2017-2022, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA MODS kernel driver is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * NVIDIA MODS kernel driver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NVIDIA MODS kernel driver.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/uaccess.h>
#include <linux/dmaengine.h>
#include "mods_internal.h"

#define MODS_DMA_MAX_CHANNEL 32
struct mods_dma_chan_info {
	rwlock_t lock;
	bool    in_use;
	u32     id;
	struct  dma_chan *pch;
	bool    should_notify;
};

static DECLARE_BITMAP(dma_info_mask, MODS_DMA_MAX_CHANNEL);
static struct mods_dma_chan_info dma_info_chan_list[MODS_DMA_MAX_CHANNEL];
static DEFINE_SPINLOCK(dma_info_lock);

static int mods_get_dma_id(u32 *p_id)
{
	u32 id;

	spin_lock(&dma_info_lock);
	id = find_first_zero_bit(dma_info_mask, MODS_DMA_MAX_CHANNEL);
	if (id >= MODS_DMA_MAX_CHANNEL) {
		spin_unlock(&dma_info_lock);
		return -ENOSPC;
	}
	set_bit(id, dma_info_mask);
	spin_unlock(&dma_info_lock);

	*p_id = id;
	return OK;
}

static void mods_release_dma_id(u32 id)
{
	spin_lock(&dma_info_lock);
	clear_bit(id, dma_info_mask);
	spin_unlock(&dma_info_lock);
}

static int mods_get_chan_by_id(u32 id, struct mods_dma_chan_info **p_dma_chan)
{
	if (id > MODS_DMA_MAX_CHANNEL)
		return	-ERANGE;

	*p_dma_chan = &dma_info_chan_list[id];

	return OK;
}

void mods_init_dma(void)
{
	struct mods_dma_chan_info *p_chan_info;
	int i;

	for (i = 0; i < MODS_DMA_MAX_CHANNEL; i++) {
		p_chan_info = &dma_info_chan_list[i];
		rwlock_init(&(p_chan_info->lock));
		p_chan_info->in_use = false;
	}
}

static void mods_release_channel(u32 id)
{
	struct mods_dma_chan_info *p_mods_chan;
	struct dma_chan *pch = NULL;

	if (mods_get_chan_by_id(id, &p_mods_chan) != OK) {
		mods_error_printk("get dma channel failed, id %d\n", id);
		return;
	}

	write_lock(&(p_mods_chan->lock));
	if (p_mods_chan->in_use) {
		pch = p_mods_chan->pch;
		p_mods_chan->pch = NULL;
		mods_release_dma_id(id);
		p_mods_chan->in_use = false;
	}
	write_unlock(&(p_mods_chan->lock));

	/* do not call this when hold spin lock */
	if (pch) {
		dmaengine_terminate_sync(pch);
		dma_release_channel(pch);
	}
}

void mods_exit_dma(void)
{
	int i;

	for (i = 0; i < MODS_DMA_MAX_CHANNEL; i++)
		mods_release_channel(i);
}

static bool mods_chan_is_inuse(struct mods_dma_chan_info *p_mods_chan)
{
	bool in_use = false;

	read_lock(&(p_mods_chan->lock));
	if (p_mods_chan->in_use)
		in_use = true;
	read_unlock(&(p_mods_chan->lock));

	return in_use;
}

static int mods_get_inuse_chan_by_handle(struct MODS_DMA_HANDLE *p_handle,
					 struct mods_dma_chan_info **p_mods_chan)
{
	int ret;
	bool in_use;
	struct mods_dma_chan_info *p_mods_ch;

	ret = mods_get_chan_by_id(p_handle->dma_id, &p_mods_ch);
	if (ret != OK) {
		mods_error_printk("get dma channel failed, id %d\n",
				  p_handle->dma_id);
		return -ENODEV;
	}

	in_use = mods_chan_is_inuse(p_mods_ch);
	if (!in_use) {
		mods_error_printk("invalid dma channel: %d, not in use\n",
				  p_handle->dma_id);
		return -EINVAL;
	}
	*p_mods_chan = p_mods_ch;
	return OK;
}

static int mods_dma_sync_wait(struct MODS_DMA_HANDLE *p_handle,
			      mods_dma_cookie_t cookie)
{
	int ret = OK;
	struct mods_dma_chan_info *p_mods_chan;

	ret = mods_get_inuse_chan_by_handle(p_handle, &p_mods_chan);
	if (ret != OK)
		return ret;

	mods_debug_printk(DEBUG_TEGRADMA,
			  "Wait on chan: %p\n", p_mods_chan->pch);
	read_lock(&(p_mods_chan->lock));
	if (dma_sync_wait(p_mods_chan->pch, cookie) != DMA_COMPLETE)
		ret = -1;
	read_unlock(&(p_mods_chan->lock));

	return ret;
}

static int mods_dma_async_is_tx_complete(struct MODS_DMA_HANDLE *p_handle,
					 mods_dma_cookie_t cookie,
					 __u32 *p_is_complete)
{
	int ret = OK;
	struct mods_dma_chan_info *p_mods_chan;
	enum dma_status status;

	ret = mods_get_inuse_chan_by_handle(p_handle, &p_mods_chan);
	if (ret != OK)
		return ret;

	read_lock(&(p_mods_chan->lock));
	status = dma_async_is_tx_complete(p_mods_chan->pch, cookie, NULL, NULL);
	read_unlock(&(p_mods_chan->lock));

	if (status == DMA_COMPLETE)
		*p_is_complete = true;
	else if (status == DMA_IN_PROGRESS)
		*p_is_complete = false;
	else
		ret = -EINVAL;

	return ret;
}

int esc_mods_dma_request_channel(struct mods_client *client,
				 struct MODS_DMA_HANDLE *p_handle)
{
	struct dma_chan *chan;
	struct mods_dma_chan_info *p_mods_chan;
	dma_cap_mask_t mask;
	u32 id;
	int ret;

	LOG_ENT();

	ret = mods_get_dma_id(&id);
	if (ret != OK) {
		cl_error("no dma handle available\n");
		return ret;
	}

	ret = mods_get_chan_by_id(id, &p_mods_chan);
	if (ret != OK) {
		cl_error("get dma channel failed\n");
		return ret;
	}

	read_lock(&(p_mods_chan->lock));
	if (p_mods_chan->in_use) {
		cl_error("mods dma channel in use\n");
		read_unlock(&(p_mods_chan->lock));
		return -EBUSY;
	}
	read_unlock(&(p_mods_chan->lock));

	dma_cap_zero(mask);
	dma_cap_set(p_handle->dma_type, mask);
	chan = dma_request_channel(mask, NULL, NULL);
	if (!chan) {
		cl_error("dma channel is not available\n");
		mods_release_dma_id(id);
		return -EBUSY;
	}

	write_lock(&(p_mods_chan->lock));
	p_mods_chan->pch = chan;
	p_mods_chan->in_use = true;
	write_unlock(&(p_mods_chan->lock));

	p_handle->dma_id = id;
	cl_debug(DEBUG_TEGRADMA, "request get dma id: %d\n", id);
	LOG_EXT();

	return 0;
}

int esc_mods_dma_release_channel(struct mods_client *client,
				 struct MODS_DMA_HANDLE *p_handle)
{
	mods_release_channel(p_handle->dma_id);
	return OK;
}

int esc_mods_dma_set_config(struct mods_client *client,
			    struct MODS_DMA_CHANNEL_CONFIG *p_config)

{
	struct dma_slave_config config;
	struct mods_dma_chan_info *p_mods_chan;
	int ret;

	LOG_ENT();
	ret = mods_get_inuse_chan_by_handle(&p_config->handle, &p_mods_chan);
	if (ret != OK)
		return ret;

	config.direction = p_config->direction;
	config.src_addr = p_config->src_addr;
	config.dst_addr = p_config->dst_addr;
	config.src_addr_width = p_config->src_addr_width;
	config.dst_addr_width = p_config->dst_addr_width;
	config.src_maxburst = p_config->src_maxburst;
	config.dst_maxburst = p_config->dst_maxburst;
	config.device_fc = (p_config->device_fc == 0) ? false : true;
	config.slave_id = p_config->slave_id;

	cl_debug(DEBUG_TEGRADMA,
		 "ch: %d dir [%d], addr[%p -> %p], burst [%d %d]",
		 p_config->handle.dma_id,
		 config.direction,
		 (void *)config.src_addr, (void *)config.dst_addr,
		 config.src_maxburst, config.dst_maxburst);
	cl_debug(DEBUG_TEGRADMA,
		 "width [%d %d] slave id %d\n",
		 config.src_addr_width, config.dst_addr_width,
		 config.slave_id);

	write_lock(&(p_mods_chan->lock));
	ret = dmaengine_slave_config(p_mods_chan->pch, &config);
	write_unlock(&(p_mods_chan->lock));

	LOG_EXT();

	return ret;
}


int esc_mods_dma_submit_request(struct mods_client *client,
				struct MODS_DMA_TX_DESC *p_mods_desc)
{
	int ret = OK;
	struct mods_dma_chan_info *p_mods_chan;
	struct dma_async_tx_descriptor *desc;
	struct dma_device       *dev;
	enum dma_ctrl_flags     flags;
	mods_dma_cookie_t cookie = 0;

	LOG_ENT();
	flags = DMA_CTRL_ACK | DMA_PREP_INTERRUPT;

	ret = mods_get_inuse_chan_by_handle(&p_mods_desc->handle, &p_mods_chan);
	if (ret != OK)
		return ret;

	if (p_mods_desc->mode != MODS_DMA_SINGLE) {
		cl_error("unsupported mode: %d\n", p_mods_desc->mode);
		return -EINVAL;
	}

	cl_debug(DEBUG_TEGRADMA, "submit on chan %p\n", p_mods_chan->pch);

	write_lock(&(p_mods_chan->lock));
	if (p_mods_desc->data_dir == MODS_DMA_MEM_TO_MEM) {
		dev = p_mods_chan->pch->device;
		desc = dev->device_prep_dma_memcpy(
						p_mods_chan->pch,
						p_mods_desc->phys,
						p_mods_desc->phys_2,
						p_mods_desc->length,
						flags);
	} else {
		cl_debug(DEBUG_TEGRADMA,
			 "Phys Addr [%p], len [%d], dir [%d]\n",
			 (void *)p_mods_desc->phys,
			 p_mods_desc->length,
			 p_mods_desc->data_dir);
		desc = dmaengine_prep_slave_single(p_mods_chan->pch,
						   p_mods_desc->phys,
						   p_mods_desc->length,
						   p_mods_desc->data_dir,
						   flags);
	}

	if (desc == NULL) {
		cl_error("unable to get desc for Tx\n");
		ret = -EIO;
		goto failed;
	}

	desc->callback = NULL;
	desc->callback_param = NULL;
	cookie = dmaengine_submit(desc);

failed:
	write_unlock(&(p_mods_chan->lock));
	if (dma_submit_error(cookie)) {
		cl_error("submit cookie: %x\n", cookie);
		return -EIO;
	}

	p_mods_desc->cookie = cookie;

	LOG_EXT();
	return ret;
}

int esc_mods_dma_async_issue_pending(struct mods_client *client,
				     struct MODS_DMA_HANDLE *p_handle)
{
	int ret = OK;
	struct mods_dma_chan_info *p_mods_chan;

	LOG_ENT();
	ret = mods_get_inuse_chan_by_handle(p_handle, &p_mods_chan);
	if (ret != OK)
		return ret;

	cl_debug(DEBUG_TEGRADMA, "issue pending on chan: %p\n",
		 p_mods_chan->pch);
	read_lock(&(p_mods_chan->lock));
	dma_async_issue_pending(p_mods_chan->pch);
	read_unlock(&(p_mods_chan->lock));
	LOG_EXT();

	return ret;
}

int esc_mods_dma_wait(struct mods_client *client,
		      struct MODS_DMA_WAIT_DESC *p_wait_desc)
{
	int ret;

	LOG_ENT();
	if (p_wait_desc->type == MODS_DMA_SYNC_WAIT)
		ret = mods_dma_sync_wait(&p_wait_desc->handle,
					 p_wait_desc->cookie);
	else if (p_wait_desc->type == MODS_DMA_ASYNC_WAIT)
		ret = mods_dma_async_is_tx_complete(&p_wait_desc->handle,
						    p_wait_desc->cookie,
						    &p_wait_desc->tx_complete);
	else
		ret = -EINVAL;

	LOG_EXT();
	return  ret;
}

int esc_mods_dma_alloc_coherent(struct mods_client *client,
				struct MODS_DMA_COHERENT_MEM_HANDLE *p)
{
	dma_addr_t    p_phys_addr;
	void          *p_cpu_addr;

	LOG_ENT();

	p_cpu_addr = dma_alloc_coherent(NULL,
					p->num_bytes,
					&p_phys_addr,
					GFP_KERNEL);

	cl_debug(DEBUG_MEM,
		 "num_bytes=%d, p_cpu_addr=%p, p_phys_addr=%p\n",
		 p->num_bytes,
		 (void *)p_cpu_addr,
		 (void *)p_phys_addr);

	if (!p_cpu_addr) {
		cl_error(
			"FAILED!!!num_bytes=%d, p_cpu_addr=%p, p_phys_addr=%p\n",
			p->num_bytes,
			(void *)p_cpu_addr,
			(void *)p_phys_addr);
		LOG_EXT();
		return -1;
	}

	memset(p_cpu_addr, 0x00, p->num_bytes);

	p->memory_handle_phys = (u64)p_phys_addr;
	p->memory_handle_virt = (u64)p_cpu_addr;

	LOG_EXT();
	return 0;
}

int esc_mods_dma_free_coherent(struct mods_client *client,
			       struct MODS_DMA_COHERENT_MEM_HANDLE *p)
{
	LOG_ENT();

	cl_debug(DEBUG_MEM,
		 "num_bytes = %d, p_cpu_addr=%p, p_phys_addr=%p\n",
		 p->num_bytes,
		 (void *)(p->memory_handle_virt),
		 (void *)(p->memory_handle_phys));

	dma_free_coherent(NULL,
		p->num_bytes,
		(void *)(p->memory_handle_virt),
		(dma_addr_t)(p->memory_handle_phys));

	p->memory_handle_phys = (u64)0;
	p->memory_handle_virt = (u64)0;

	LOG_EXT();
	return 0;
}

int esc_mods_dma_copy_to_user(struct mods_client *client,
			      struct MODS_DMA_COPY_TO_USER *p)
{
	int retval;

	LOG_ENT();

	cl_debug(DEBUG_MEM,
		 "memory_handle_dst=%p, memory_handle_src=%p, num_bytes=%d\n",
		 (void *)(p->memory_handle_dst),
		 (void *)(p->memory_handle_src),
		 p->num_bytes);

	retval = copy_to_user((void __user *)p->memory_handle_dst,
			      (void *)p->memory_handle_src,
			      p->num_bytes);

	LOG_EXT();

	return retval;
}
