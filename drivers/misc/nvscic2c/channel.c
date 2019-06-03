/*
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
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

/**
 * nvscic2c LKM channel subsystem.
 * This subsystem is responsible to create
 * different supported C2C channel device nodes
 * and providing different file operations for these nodes.
 * This subsystem also provides one api to man context
 * which can be used to notify any global events like SDL.
 * Each active user will be notified for such events.
 */
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/printk.h>
#include <linux/string.h>

#include "channel.h"

/* Once channel device nodes created, we will use dev_* functions.
 * Until device is created we will use these wrappers.
 */
#define TAG              "nvscic2c: channel: "
#define C2C_ERROR(...)   pr_err(TAG __VA_ARGS__)
#define C2C_DEBUG(...)   pr_debug(TAG __VA_ARGS__)
#define C2C_INFO(...)    pr_info(TAG __VA_ARGS__)
#define C2C_ALERT(...)   pr_alert(TAG __VA_ARGS__)

/**
 * Channel device placeholder.
 * dev node private data will be set
 * with this.
 * Each supported C2C channel is added
 * as specific device node.
 * All device nodes are added into
 * nvscic2c class.
 * This also contains mutex to be used within
 * fops of channel device.
 * We also use specific waitq for channel.
 */
struct channel {
	struct channel_param param;
	struct cpu_buff ctrl;

	dev_t dev;
	struct cdev cdev;
	struct device *device;
	/*Name of device node to be created in /dev*/
	char node_name[MAX_NAME_SZ];

	/* poll/notifications.*/
	wait_queue_head_t waitq;

	/* serialise access to fops.*/
	struct mutex fops_lock;
	bool in_use;

	/* Should count events trigger
	 * from C2C Server thread.
	 */
	atomic_t c2c_server_event;
};


/* Internal API's for channel layer within nvscic2c module.*/
static void init_channel_hdr(struct channel *channel);
static void set_hdr_field32(void *pva, uint32_t off, uint32_t val);
static int allocate_channel_ctrl(struct cpu_buff *ctrl, struct device *pdev);
static void print_channel_info(struct channel *channel);
static void print_channel_xfer_type(struct device *dev, enum xfer_type type);
static int channel_fops_open(struct inode *inode, struct file *filp);
static int channel_fops_release(struct inode *inode, struct file *filp);
static int channel_fops_mmap(struct file *filp, struct vm_area_struct *vma);
static unsigned int channel_fops_poll(struct file *filp, poll_table *wait);
static long channel_fops_ioctl(struct file *filp, unsigned int cmd,
				unsigned long arg);
static int ioctl_get_info_impl(struct channel *channel,
				struct nvscic2c_info *get_info);

/*
 * set of channel file operations for each nvscic2c channel.
 * We do not support: read() and write() on nvscic2c channel
 * descriptors.
 */
static const struct file_operations channel_fops = {
	.owner          = THIS_MODULE,
	.open           = channel_fops_open,
	.release        = channel_fops_release,
	.mmap           = channel_fops_mmap,
	.unlocked_ioctl = channel_fops_ioctl,
	.poll           = channel_fops_poll,
	.llseek         = noop_llseek,
};

/**
 * channel_deinit
 *  => Free channel local control memory.
 *  => free channel place holder.
 */
int channel_deinit(channel_hdl hdl, struct device *pdev)
{
	int ret = 0;
	struct channel *channel = NULL;

	if ((hdl == NULL)
		|| (pdev == NULL))
		return ret;

	channel = (struct channel *)hdl;

	dev_dbg(pdev, "Channel ctrl mem:\n\t\t\tpa  :%pa\n"
#ifdef C2C_MAP
				"\t\t\tpva :0x%p\n"
#endif
				"\t\t\tsize:0x%016zx\n",
				&channel->ctrl.phys_addr,
#ifdef C2C_MAP
				channel->ctrl.pva,
#endif
				(size_t)channel->ctrl.size);
	if (channel->ctrl.pva != NULL) {
		kfree(channel->ctrl.pva);
		channel->ctrl.pva = NULL;
	}

	devm_kfree(pdev, channel);
	/* Local variable, setting just to tell the reader
	 * that we expect pointer to be set NULL by caller.
	 */
	channel = NULL;

	return ret;
}

/**
 * channel_init
 *  => Check if channel place holder already initialized.
 *  => Allocate channel placeholder.
 *  => initialize properties from DT parsed init param.
 *  => Allocate local channel control memory.
 */
int channel_init(channel_hdl *hdl, struct channel_param *init,
		struct device *pdev)
{
	int ret = 0;
	struct channel *channel = NULL;

	if ((hdl == NULL)
		|| (init == NULL)
		|| (pdev == NULL)) {
		C2C_ERROR("Invalid Params.\n");
		ret = -EINVAL;
		goto err;
	}

	if (*hdl) {
		C2C_ERROR("Channel already initialized.\n");
		ret = -EALREADY;
		goto err;
	}

	channel = devm_kzalloc(pdev, sizeof(struct channel), GFP_KERNEL);
	if (channel == NULL) {
		C2C_ERROR("devm_kzalloc failed for channel place holder.\n");
		ret = -ENOMEM;
		goto err;
	}

	channel->ctrl.pva = NULL;
	channel->in_use   = false;

	strncpy(channel->param.name, init->name, (MAX_NAME_SZ - 1));
	channel->param.number         = init->number;
	channel->param.c2c_ctx        = init->c2c_ctx;
	strncpy(channel->param.cfg_name, init->cfg_name, (MAX_NAME_SZ - 1));
	channel->param.xfer_type      = init->xfer_type;
	channel->param.edma_enabled   = init->edma_enabled;

	memcpy(&(channel->param.self_mem), &(init->self_mem),
			 sizeof(struct cpu_buff));
	memcpy(&(channel->param.peer_mem), &(init->peer_mem),
			 sizeof(struct pci_mmio));

	channel->param.event          = init->event;
	channel->param.nframes        = init->nframes;
	channel->param.frame_size     = init->frame_size;

	/**
	 * Each valid C2C channel has internal control memory.
	 * This memory is used to store internal data like state, counters etc.
	 */
	ret = allocate_channel_ctrl(&channel->ctrl, pdev);
	if (ret != 0) {
		C2C_ERROR("allocate_channel_ctrl failed with: (%d)", ret);
		goto err;
	}

	/* initialise the channel device internals.*/
	mutex_init(&(channel->fops_lock));
	init_waitqueue_head(&(channel->waitq));
	atomic_set(&(channel->c2c_server_event), 0);


	init_channel_hdr(channel);
	*hdl = channel;

	return ret;

err:
	channel_deinit((channel_hdl)channel, pdev);
	return ret;
}


/**
 * channel_remove_device
 *  => Delete C2C channel device node.
 */
int channel_remove_device(channel_hdl hdl,
			dev_t c2c_dev, struct class *c2c_class)
{
	int ret = 0;
	struct channel *channel = NULL;

	if (hdl == NULL) {
		C2C_ERROR("Invalid channel handle.\n");
		ret = -EINVAL;
		return ret;
	}

	channel = (struct channel *)hdl;

	/* remove the channel device.*/
	if (channel->device != NULL) {
		cdev_del(&channel->cdev);
		device_del(channel->device);
		channel->device = NULL;
	}

	return ret;
}

/**
 * channel_add_device
 *  => Create C2C channel device node.
 *  => Set device private data to channel place holder.
 */
int channel_add_device(channel_hdl hdl,
			dev_t c2c_dev, struct class *c2c_class)
{
	int ret = 0;
	struct channel *channel = NULL;

	if (hdl == NULL) {
		C2C_ERROR("Invalid channel handle.\n");
		ret = -EINVAL;
		return ret;
	}
	channel = (struct channel *)hdl;

	/* create the nvscic2c channel device - interface for user-space sw.*/
	channel->dev = MKDEV(MAJOR(c2c_dev), channel->param.number);
	cdev_init(&(channel->cdev), &(channel_fops));
	channel->cdev.owner = THIS_MODULE;
	ret = cdev_add(&(channel->cdev), channel->dev, 1);
	if (ret != 0) {
		C2C_ERROR("cdev_add() failed\n");
		goto err;
	}

	snprintf(channel->node_name, (MAX_NAME_SZ - 1), "%s_%d",
			MODULE_NAME, channel->param.number);
	channel->device = device_create(c2c_class, NULL,
					channel->dev, channel,
					channel->node_name);
	if (IS_ERR(channel->device)) {
		ret = PTR_ERR(channel->device);
		C2C_ERROR("(%s): device_create() failed\n",
			channel->param.name);
		goto err;
	}
	dev_set_drvdata(channel->device, channel);

	/*Post this dev_* api's should be used for printing.*/

	print_channel_info(channel);

	return ret;
err:
	channel_remove_device(hdl, c2c_dev, c2c_class);
	return ret;
}

/*
 * This function is exposed by channel sub-module,
 * to be called when user of channel device needs to be
 * notified.
 * This will be used later when we add SDL handling.
 */
int channel_handle_server_msg(channel_hdl hdl)
{
	int ret = 0;
	struct channel *channel = NULL;

	if (hdl == NULL) {
		C2C_ERROR("Invalid Params\n");
		ret = -EINVAL;
		return ret;
	}
	channel = (struct channel *)hdl;

	atomic_inc(&(channel->c2c_server_event));
	wake_up_interruptible_all(&(channel->waitq));

	return ret;
}

/**
 * Allocate channel control memory for each supported C2C channels.
 * Also book-keep physical address.
 * Physical address is returned to user applications via devctl.
 * User can map this using mmap call.
 * In case memory is already allocated then return failure.
 * Please note we allocate one page physical memory.
 */
static int allocate_channel_ctrl(struct cpu_buff *ctrl,
		struct device *pdev)
{
	int ret = 0;

	if ((ctrl == NULL)
		|| (pdev == NULL)) {
		C2C_ERROR("Invalid placeholder for channel control.");
		ret = -EINVAL;
		return ret;
	}

	if (ctrl->pva != NULL) {
		C2C_ERROR("Channel control memory already allocated.");
		ret = -EALREADY;
		return ret;
	}

	/*Allocate local control memory of size one page.*/
	ctrl->size = PAGE_SIZE;
	/**
	 * struct devres {
	 *  struct devres_node node;
	 *  u8 __aligned(ARCH_KMALLOC_MINALIGN) data[];
	 * };
	 * This internal counter memory is mapped by user space library.
	 * While mapping we use PFN for this physical page.
	 * In case of devm_kmalloc we get offset of data[],
	 * But when we generate PFN for mapping at user space level
	 * It gets mapped from beginning.
	 * To map exactly from offset of data[] we need to pass pg_off
	 * remap_pfn_range which is not possible because struct devres is
	 * not public.
	 * Hence for this particular case we will be using kzalloc instead of
	 * devm_kzalloc.
	 */
	ctrl->pva  = kzalloc(ctrl->size, GFP_KERNEL);
	if (ctrl->pva == NULL) {
		C2C_ERROR("devm_kzalloc failed for channel place holder.\n");
		ret = -ENOMEM;
		return ret;
	}
	ctrl->phys_addr = virt_to_phys(ctrl->pva);

	return ret;
}

/**
 * Print channel device basic information.
 */
static void print_channel_info(struct channel *channel)
{
	struct channel_param *param = NULL;

	if (channel == NULL) {
		C2C_ERROR("Invalid C2C device context.\n");
		return;
	}

	param = &channel->param;

	dev_dbg(channel->device, "********************************************");
	dev_dbg(channel->device, "Channel Name: %s\n", param->name);
	dev_dbg(channel->device, "frames: (%u) of sz: (%u)\n",
			param->nframes, param->frame_size);
	print_channel_xfer_type(channel->device, param->xfer_type);
	dev_dbg(channel->device, "NvSciIpc(IVC) cfg name: %s\n",
		param->cfg_name);
	dev_dbg(channel->device, "Channel Peer mem:\n\t\t\tapt :%pa\n"
#ifdef C2C_MAP
				"\t\t\tpva :0x%p\n"
#endif
				"\t\t\tsize:0x%016zx\n",
			&param->peer_mem.aper,
#ifdef C2C_MAP
			param->peer_mem.pva,
#endif
			(size_t)param->peer_mem.size);
	dev_dbg(channel->device, "Channel Self mem:\n\t\t\tpa  :%pa\n"
#ifdef C2C_MAP
			"\t\t\tpva :0x%p\n"
#endif
			"\t\t\tsize:0x%016zx\n",
			&param->self_mem.phys_addr,
#ifdef C2C_MAP
			param->self_mem.pva,
#endif
			(size_t)param->self_mem.size);
	dev_dbg(channel->device, "Channel ctrl mem:\n\t\t\tpa  :%pa\n"
#ifdef C2C_MAP
				"\t\t\tpva :0x%p\n"
#endif
				"\t\t\tsize:0x%016zx\n",
				&channel->ctrl.phys_addr,
#ifdef C2C_MAP
				channel->ctrl.pva,
#endif
				(size_t)channel->ctrl.size);
	dev_dbg(channel->device, "********************************************");
}

/**
 * Printing string literals for different enum values
 * of channel transfer type.
 */
static void print_channel_xfer_type(struct device *dev, enum xfer_type type)
{
	/*No Validations.*/
	switch (type) {
	case XFER_TYPE_CPU:
		dev_dbg(dev, "Channel xfer type: Cpu\n");
		break;
	case XFER_TYPE_BULK_PRODUCER:
		dev_dbg(dev, "Channel xfer type: Bulk_producer\n");
		break;
	case XFER_TYPE_BULK_CONSUMER:
		dev_dbg(dev, "Channel xfer type: Bulk_consumer\n");
		break;
	case XFER_TYPE_BULK_PRODUCER_PCIE_READ:
		dev_dbg(dev, "Channel xfer type: Bulk_producer_pcie_read\n");
		break;
	case XFER_TYPE_BULK_CONSUMER_PCIE_READ:
		dev_dbg(dev, "Channel xfer type: Bulk_consumer_pcie_read\n");
		break;
	default:
		dev_dbg(dev, "Channel xfer type: Invalid\n");
		break;
	}
}

/**
 * Each supported C2C channel has fixed size header information.
 * We reset basic fields in this channel header.
 * Please note that we also have local copy of this channel header
 * known as control information.
 * This is done to avoid PCIe read.
 */
static void init_channel_hdr(struct channel *channel)
{
	void *mem = NULL;

	/**
	 * Enums are not shared with LKM.
	 * LKM doesn't need as well.
	 * Hence initialized with values direct.
	 * Moreover goal here is to have some default value in range,
	 * Guest is any way going to initiate handshake.
	 */
	/*Reset header field in local control memory.*/
	mem = channel->ctrl.pva;
	set_hdr_field32(mem, CH_HDR_TX_CNTR_OFF, 0x0);
	set_hdr_field32(mem, CH_HDR_RX_CNTR_OFF, 0x0);
	set_hdr_field32(mem, CH_HDR_W_SLEEP_OFF, 0x0);
	set_hdr_field32(mem, CH_HDR_R_SLEEP_OFF, 0x1);
	set_hdr_field32(mem, CH_HDR_STATE_OFF, 0x1);

	/*Reset header field in PCIe shared memory.*/
	mem = channel->param.self_mem.pva;
	set_hdr_field32(mem, CH_HDR_TX_CNTR_OFF, 0x0);
	set_hdr_field32(mem, CH_HDR_RX_CNTR_OFF, 0x0);
	set_hdr_field32(mem, CH_HDR_W_SLEEP_OFF, 0x0);
	set_hdr_field32(mem, CH_HDR_R_SLEEP_OFF, 0x1);
	set_hdr_field32(mem, CH_HDR_STATE_OFF, 0x1);
}

/**
 * api to update 32 bit memory field in header.
 */
static void set_hdr_field32(void *pva, uint32_t off, uint32_t val)
{
/* We are not mapping IVM in lkm right now.
 * We dont have need to as library is written such
 * a way that lib can handle its own.
 * Still code is there if we figure out
 * any case where we might need to set header fields.
 * Hence mapping is added under compile time macro.
 */
#ifdef C2C_MAP
	uint8_t *addr = NULL;

	/* no validation. Make sure offset is within the base + size.*/

	addr = (uint8_t *)pva + off;
	*((uint32_t *)addr) = val;
#endif
}

static int channel_fops_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	struct channel *channel =
		container_of(inode->i_cdev, struct channel, cdev);

	mutex_lock(&(channel->fops_lock));
	if (channel->in_use) {
		/* already in use.*/
		ret = -EBUSY;
	} else {
		channel->in_use = true;
	}
	mutex_unlock(&(channel->fops_lock));

	atomic_set(&(channel->c2c_server_event), 0);

	filp->private_data = channel;

	return ret;
}

static int channel_fops_release(struct inode *inode, struct file *filp)
{
	int ret = 0;

	struct channel *channel = filp->private_data;

	if (WARN_ON(!(channel != NULL)))
		return -EFAULT;

	mutex_lock(&(channel->fops_lock));
	if (channel->in_use)
		channel->in_use = false;
	mutex_unlock(&(channel->fops_lock));

	filp->private_data = NULL;

	return ret;
}

static int channel_fops_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int ret = 0;
	uint64_t mmap_type = vma->vm_pgoff;
	uint64_t memaddr = 0x0;
	uint64_t memsize = 0x0;
	struct channel *channel = filp->private_data;

	if (WARN_ON(!(channel != NULL)))
		return -EFAULT;

	if (WARN_ON(!(vma != NULL)))
		return -EFAULT;

	mutex_lock(&(channel->fops_lock));

	switch (mmap_type) {
	case PEER_MEM_MMAP:
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
		memaddr = channel->param.peer_mem.aper;
		memsize = channel->param.peer_mem.size;
		break;
	case SELF_MEM_MMAP:
		memaddr = channel->param.self_mem.phys_addr;
		memsize = channel->param.self_mem.size;
		break;
	case CTRL_MEM_MMAP:
		memaddr = channel->ctrl.phys_addr;
		memsize = channel->ctrl.size;
		break;
	case LINK_MEM_MMAP:
		ret = -EOPNOTSUPP;
		goto exit;
	default:
		dev_err(channel->device, "unrecognised mmap type: (%llu)\n",
			mmap_type);
		goto exit;
	}

	if ((vma->vm_end - vma->vm_start) != memsize) {
		dev_err(channel->device, "mmap type: (%llu), memsize mismatch\n",
			mmap_type);
		goto exit;
	}

	vma->vm_pgoff     = 0;
	vma->vm_flags     |= (VM_DONTCOPY); // fork() not supported.
	ret = remap_pfn_range(vma, vma->vm_start,
				PFN_DOWN(memaddr),
				memsize, vma->vm_page_prot);
	if (ret != 0) {
		dev_err(channel->device, "mmap() failed, mmap type:(%llu)\n",
			mmap_type);
	}

exit:
	mutex_unlock(&(channel->fops_lock));
	return ret;
}

static unsigned int channel_fops_poll(struct file *filp, poll_table *wait)
{
	uint32_t ret = 0;
	struct channel *channel = filp->private_data;

	if (WARN_ON(!(channel != NULL)))
		return -EFAULT;

	mutex_lock(&(channel->fops_lock));

	/* add all waitq if they are different for read, write.*/
	poll_wait(filp, &(channel->waitq), wait);

	/* wake up read, write (& exception - those who want to use) fd on
	 * getting event.
	 */
	if (atomic_read(&(channel->c2c_server_event))) {
		/* Pending event. */
		atomic_dec(&channel->c2c_server_event);
		ret = (POLLPRI | POLLIN | POLLOUT);
	}

	mutex_unlock(&(channel->fops_lock));
	return ret;
}

static long channel_fops_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	long ret = 0;
	uint8_t buf[256] = {0};
	struct channel *channel = filp->private_data;

	if (WARN_ON(!(channel != NULL)))
		return -EFAULT;

	/* validate the cmd */
	if ((_IOC_TYPE(cmd) != NVSCIC2C_IOCTL_MAGIC) ||
		(_IOC_NR(cmd) == 0) ||
		(_IOC_NR(cmd) > NVSCIC2C_IOCTL_NUMBER_MAX) ||
		(_IOC_SIZE(cmd) > 256)) {
		dev_err(channel->device, "Incorrect ioctl cmd/cmd params/magic\n");
		return -ENOTTY;
	}

	(void) memset(buf, 0, sizeof(buf));
	if (_IOC_DIR(cmd) & _IOC_WRITE) {
		if (copy_from_user(buf, (void __user *)arg, _IOC_SIZE(cmd)))
			return -EFAULT;
	}

	switch (cmd) {
	case NVSCIC2C_IOCTL_GET_INFO:
		ret = ioctl_get_info_impl(channel,
			(struct nvscic2c_info *) buf);
		break;
	default:
		dev_err(channel->device, "unrecognised nvscic2c ioctl cmd: 0x%x\n",
			cmd);
		ret = -ENOTTY;
		break;
	}

	/* copy the cmd result back to user if it was kernel->user: get_info.*/
	if ((ret == 0) && (_IOC_DIR(cmd) & _IOC_READ))
		ret = copy_to_user((void __user *)arg, buf, _IOC_SIZE(cmd));

	return ret;
}

/*
 * helper function to implement NVSCIC2C_IOCTL_GET_INFO ioctl call.
 *
 * All important channel dev node properites required for user-space
 * to map the channel memory and work without going to LKM for data
 * xfer are exported in this ioctl implementation.
 *
 * Because we export 3 different memory for a single nvscic2c channel
 * device,
 * export the memory regions as masked offsets.
 */
static int ioctl_get_info_impl(struct channel *channel,
				struct nvscic2c_info *get_info)
{
	/* No Validations.*/
	/* actual offsets of 3 mem are not shared as we have to support
	 * multiple mmap for a single nvscic2c char device.
	 */
	strncpy(get_info->cfg_name, channel->param.cfg_name, (MAX_NAME_SZ - 1));
	get_info->nframes      = channel->param.nframes;
	get_info->frame_size   = channel->param.frame_size;
	get_info->xfer_type    = channel->param.xfer_type;
	get_info->edma_enabled = channel->param.edma_enabled;
	get_info->peer.offset  = (PEER_MEM_MMAP << PAGE_SHIFT);
	get_info->peer.size    = channel->param.peer_mem.size;
	get_info->self.offset  = (SELF_MEM_MMAP << PAGE_SHIFT);
	get_info->self.size    = channel->param.self_mem.size;
	get_info->ctrl.offset  = (CTRL_MEM_MMAP << PAGE_SHIFT);
	get_info->ctrl.size    = channel->ctrl.size;

	return 0;
}
