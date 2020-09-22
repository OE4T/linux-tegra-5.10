/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.	All rights reserved.
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
 * Chip to Chip communication support is to provide user
 * a functionality to transfer data between two chips connected
 * over PCIe.
 * This support is split into two parts:
 * 1) C2C server: Works as NTB client.
 * 2) Guest     : Supports application to use C2C server
 * for data transfer.
 * In Tegra Linux we LKM and guest library, which can be used by
 * application to transfer data between two chips.
 * This LKM parses nvscic2c node in DT and creates
 * file system device node for each valid C2C channel.
 * PCIe aperture is available as static address which is divided
 * between each C2C channels.
 * IVM is used by C2C server to configure as PCIe shared memory
 * for NTB device. Same IVM is reserved by LKM and divided between
 * each C2C channel as receive area.
 * LKM is also suppose to handle SDL message with C2C server over
 * private control channel using IVC.
 * This support is not there yet and to be added.
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <asm/io.h>
#include <linux/string.h>
#include <linux/mod_devicetable.h>

#include "cdev.h"

#define DRIVER_LICENSE      "GPL v2"
#define DRIVER_DESCRIPTION  "NvSciC2C kernel module to support" \
							" Chip to Chip transfer."
#define DRIVER_VERSION      "1.0"
#define DRIVER_RELDATE      "May 2019"
#define DRIVER_AUTHOR       "Nvidia Corporation"
#define DRIVER_NAME          MODULE_NAME

MODULE_DESCRIPTION(DRIVER_DESCRIPTION);
MODULE_LICENSE(DRIVER_LICENSE);
MODULE_VERSION(DRIVER_VERSION);
MODULE_AUTHOR(DRIVER_AUTHOR);

/*When module platform device not available.*/
#define TAG              "nvscic2c: module : "
#define C2C_ERROR(...)   pr_err(TAG __VA_ARGS__)
#define C2C_DEBUG(...)   pr_debug(TAG __VA_ARGS__)
#define C2C_INFO(...)    pr_info(TAG __VA_ARGS__)
#define C2C_ALERT(...)   pr_alert(TAG __VA_ARGS__)

/**
 * C2C driver is registered as platform driver.
 * At init we create dev nodes for all c2c channels.
 * we also allocate one driver specific platform context.
 * This api is to clear all these contexts and delete dev nodes.
 */
static int clear_c2c_context(struct c2c *ctx)
{
	int ret = 0;
	int i = 0;

	if (ctx == NULL)
		return ret;

	if (ctx->channels != NULL) {
		for (i = 0; i < ctx->dt_param.nr_channels; i++) {
			channel_remove_device(ctx->channels[i], ctx->c2c_dev,
						ctx->c2c_class);
			channel_deinit(ctx->channels[i], ctx->dev);
			ctx->channels[i] = NULL;
		}
	}

	dev_dbg(ctx->dev, "Post releasing channels.");

	if (ctx->c2c_class) {
		class_destroy(ctx->c2c_class);
		ctx->c2c_class = NULL;
	}

	if (ctx->c2c_dev) {
		unregister_chrdev_region(ctx->c2c_dev,
					ctx->dt_param.nr_channels);
		ctx->c2c_dev = 0;
	}

	if (ctx->ivmk != NULL) {
		tegra_hv_mempool_unreserve(ctx->ivmk);
		ctx->ivmk = NULL;
	}

	dt_release(ctx->dev, &ctx->dt_param);

	devm_kfree(ctx->dev, ctx);
	ctx = NULL;

	return ret;
}

/*
 * helper function to validate whether the offset is within the
 * range of PCIe shared memory/Aperture memory.
 */
static int validate_offset(struct c2c *ctx, size_t offset, size_t q_sz)
{
	int ret = 0;

	/* no validations.*/

	if ((offset + q_sz) >
		(ctx->peer_mem.size)) {
		dev_err(ctx->dev, "c2c channel mem offset beyond pcie aperture");
		dev_err(ctx->dev, "offset=(0x%016zx), q_sz=(0x%016zx)",
			offset, q_sz);
		dev_err(ctx->dev, "aper_mem=(0x%pa), size=(0x%016zx)",
			&ctx->peer_mem.aper, (size_t)ctx->peer_mem.size);
		/* Not returning so that we can get spew from below
		 * as well if failed.
		 */
		ret = -EFAULT;
	}

	if ((offset + q_sz) >
		(ctx->self_mem.size)) {
		dev_err(ctx->dev, "c2c channel mem offset beyond pcie shared mem");
		dev_err(ctx->dev, "offset=(0x%016zx), q_sz=(0x%016zx)",
			offset, q_sz);
		dev_err(ctx->dev, "self_mem=(0x%pa), size=(0x%016zx)",
			&ctx->self_mem.phys_addr, (size_t)ctx->self_mem.size);
		ret = -EFAULT;
	}

	return ret;
}

/*
 * internal function to divide the pcie shared memory and pcie aperture
 * into segments which each c2c_channel(ivc channel) can use for xfers
 * across PCIe wire. We fragment overall memory as a shadow of each ivc
 * memory. As each IVC channel is bidirectional, we fragment PCIe aperture
 * and PCIe shared memory by identical offsets.
 *
 * To be called only once for each c2c channel.
 * The mappings for IVC are not supposed to change runtime.
 */
static int channel_mem_init(struct c2c *ctx, struct channel_param *init,
					size_t *pcie_off)
{
	int ret = 0;
	size_t ch_sz = 0;
	uint32_t align_mask = 0x0;

	if ((ctx == NULL)
		|| (init == NULL)
		|| (pcie_off == NULL)) {
		C2C_ERROR("Invalid params.");
		return -EINVAL;
	}

	/* Calculate total size of channel.*/
	ch_sz = (init->nframes * init->frame_size) + CH_HDR_SIZE;

	/* if any alignment adjust total size.*/
	align_mask = (init->align - 1);
	ch_sz      += align_mask;
	ch_sz      &= ~(align_mask);

	/* Check if offset goes beyond size. */
	ret = validate_offset(ctx, *pcie_off, ch_sz);
	if (ret) {
		dev_err(ctx->dev, "pcie memory required for (%s) more than available",
			init->name);
		return ret;
	}

	/* if the queue size fits, then assign the aperture and share mem.
	 * Please note LKM doesn't need to map IVM memory.
	 * Hence support for this is added under compile time macro.
	 * Later if we see any such use case we should enable this
	 * via compilation.
	 */
	init->peer_mem.aper      = (ctx->peer_mem.aper + *pcie_off);
#ifdef C2C_MAP
	init->peer_mem.pva       = (((uint8_t *)ctx->peer_mem.pva) + *pcie_off);
#endif
	init->peer_mem.size      = ch_sz;

	init->self_mem.phys_addr = (ctx->self_mem.phys_addr + *pcie_off);
#ifdef C2C_MAP
	init->self_mem.pva       = (((uint8_t *)ctx->self_mem.pva) + *pcie_off);
#endif
	init->self_mem.size      = ch_sz;

	/*for next channel allocation.*/
	*pcie_off += ch_sz;

	return ret;
}

/**
 * Initialize each C2C channel place holder.
 * Please note we follow delayed device node creation approach.
 * In this approach we first allocate channel place holders.
 * If allocation was successful then only we create device node
 * for channels else we free up resource and exit.
 */
static int init_c2c_channels(struct c2c *ctx)
{
	int ret = 0;
	int i = 0;
	size_t pcie_off = 0;

	if (ctx == NULL) {
		C2C_ERROR("Invalid C2C context.\n");
		ret = -EINVAL;
		return ret;
	}

	/* Traverse each parsed channel and allocate/initialize placeholder.*/
	for (i = 0; i < ctx->dt_param.nr_channels; i++) {
		struct channel_param init = {0};
		struct channel_dt_param *chn_param = NULL;

		chn_param = &(ctx->dt_param.chn_params[i]);
		strncpy(init.name, chn_param->ch_name,
			(MAX_NAME_SZ - 1));

		init.number            = i;
		strncpy(init.cfg_name, chn_param->cfg_name, (MAX_NAME_SZ - 1));
		init.align             = chn_param->align;
		init.event             = chn_param->event;
		init.nframes           = chn_param->nframes;
		init.frame_size        = chn_param->frame_size;
		init.xfer_type         = chn_param->xfer_type;
		init.edma_enabled      = chn_param->edma_enabled;
		init.c2c_ctx           = (void *)ctx;
		ret = channel_mem_init(ctx, &init, &pcie_off);
		if (ret != 0) {
			dev_err(ctx->dev,
				"failed to assign PCIe (aprt/mem) for channel (%s)\n",
				init.name);
			return ret;
		}

		ret = channel_init(&(ctx->channels[i]), &init, ctx->dev);
		if (ret != 0) {
			dev_err(ctx->dev, "failed to init channel (%d)\n", i);
			return ret;
		}
	}

	return ret;
}

/**
 * Create char device region to
 * add c2c dev nodes for all c2c channels.
 * Also create one class for nvscic2c.
 * Post this initialize each valid C2C channel place holder.
 * If initialization was ok, create channel device nodes.
 */
static int setup_c2c_devices(struct c2c *ctx)
{
	int i = 0;
	int ret = 0;
	size_t sz = 0;

	if (ctx == NULL) {
		C2C_ERROR("Invalid C2C context.\n");
		ret = -EINVAL;
		return ret;
	}

	/**
	 * First minor is taken as 0.
	 */
	ret = alloc_chrdev_region(&(ctx->c2c_dev), 0,
				ctx->dt_param.nr_channels, MODULE_NAME);
	if (ret != 0) {
		dev_err(ctx->dev, "alloc_chrdev_region() failed.\n");
		goto err;
	}

	ctx->c2c_class = class_create(THIS_MODULE, MODULE_NAME);
	if (IS_ERR(ctx->c2c_class)) {
		dev_err(ctx->dev, "failed to create c2c class: %ld\n",
			PTR_ERR(ctx->c2c_class));
		ret = PTR_ERR(ctx->c2c_class);
		goto err;
	}

	sz = (sizeof(channel_hdl) * (ctx->dt_param.nr_channels));
	ctx->channels = (channel_hdl *)devm_kzalloc(ctx->dev, sz, GFP_KERNEL);
	if (ctx->channels == NULL) {
		dev_err(ctx->dev, "failed to alloc c2c channel holder.\n");
		ret = -ENOMEM;
		goto err;
	}

	ret = init_c2c_channels(ctx);
	if (ret != 0) {
		dev_err(ctx->dev, "init_c2c_channels failed with: %d\n", ret);
		return ret;
	}

	for (i = 0; i < ctx->dt_param.nr_channels; i++) {
		ret = channel_add_device(ctx->channels[i], ctx->c2c_dev,
						ctx->c2c_class);
		if (ret != 0) {
			dev_err(ctx->dev, "channel_add_device failed with: %d for: %d\n",
				ret, i);
			goto err;
		}
	}

	return ret;

err:
	return ret;
}

/**
 * NTB device PCIe aperture was provided via DT.
 * Also IVM id was provided which used for NTB
 * translation at C2C server.
 * Initialize nvscic2c memories using these information
 * and also map in local pva if C2C_MAP was enabled
 * in makefile.
 */
static int init_c2c_memory(struct c2c *ctx)
{
	int ret = 0;

	if (ctx == NULL) {
		ret = -EINVAL;
		C2C_ERROR("Invalid C2C context.\n");
		return ret;
	}

	if (ctx->ivmk != NULL) {
		ret = -EALREADY;
		dev_err(ctx->dev, "Memory already initialized.");
		return ret;
	}

	ctx->peer_mem.aper = (phys_addr_t)ctx->dt_param.apt.base;
	ctx->peer_mem.size = (resource_size_t)ctx->dt_param.apt.size;
#ifdef C2C_MAP
	ctx->peer_mem.pva  = ioremap(ctx->dt_param.apt.base,
			ctx->dt_param.apt.size);
#endif

	ctx->ivmk = tegra_hv_mempool_reserve(ctx->dt_param.ivm);
	if (IS_ERR_OR_NULL(ctx->ivmk)) {
		if (IS_ERR(ctx->ivmk))
			dev_err(ctx->dev, "No mempool found\n");
		return -ENOMEM;
	}

	dev_dbg(ctx->dev, "ivm %pa\n", &ctx->ivmk->ipa);

	ctx->self_mem.phys_addr  = ctx->ivmk->ipa;
	ctx->self_mem.size       = (size_t)ctx->ivmk->size;
#ifdef C2C_MAP
	ctx->self_mem.pva        = ioremap(ctx->ivmk->ipa,
			ctx->ivmk->size);
#endif

	return ret;
}

/**
 * Probe function for the nvscic2c driver.
 *
 * The functions invoked here are private to nvscic2c driver
 *
 * Parse nvscic2c dt node and validate parameter supplied.
 * Reserve the IVM used for NTB translation at C2C server.
 * Initialize place holders for difference C2C channels
 * and create device nodes for same.
 * set nvscic2c context as platform data for this module.
 */
static int c2c_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct c2c *ctx = NULL;

	if (pdev == NULL) {
		C2C_ERROR("(%s): platform device INVAL\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	/* allocate private driver data. */
	ctx = devm_kzalloc(&pdev->dev, sizeof(struct c2c), GFP_KERNEL);
	if (ctx == NULL) {
		C2C_ERROR("devm_kzalloc failed for C2C context.\n");
		ret = -ENOMEM;
		goto err;
	}
	ctx->dev = &(pdev->dev);
	platform_set_drvdata(pdev, ctx);

	/* parse DT node and populate all we can. */
	ret = dt_parse(&(pdev->dev), &(ctx->dt_param));
	if (ret != 0) {
		dev_err(&pdev->dev,
			"failed to parse device tree\n");
		goto err;
	}

	/* we must have at least one and not more than supported channels. */
	if ((ctx->dt_param.nr_channels <= 0)
		|| (ctx->dt_param.nr_channels > MAX_CHANNELS)) {
		dev_err(&pdev->dev, "Invalid C2C channel count :(%u)\n",
			ctx->dt_param.nr_channels);
		ret = -EINVAL;
		goto err;
	}

	if (ctx->dt_param.ivm != -1) {
		ret = init_c2c_memory(ctx);
		if (ret != 0) {
			dev_err(&pdev->dev,
				"init_c2c_memory failed with: %d.\n", ret);
			goto err;
		}
	} else {
		dev_err(&pdev->dev,
			"IVM is mandatory as CO is not supported yet.\n");
		ret = -EINVAL;
		goto err;
	}

	ret = setup_c2c_devices(ctx);
	if (ret != 0) {
		dev_err(&pdev->dev,
			"setup_c2c_devices failed with: %d\n", ret);
		goto err;
	}

	dev_info(&(pdev->dev), "Loaded module\n");
	return ret;

err:
	clear_c2c_context(ctx);
	ctx = NULL;
	return ret;
}


/* unload module function for the nvscic2c driver.
 * gets called only when driver is compiled as a
 * LKM.
 */
static int c2c_remove(struct platform_device *pdev)
{
	int ret = 0;
	struct c2c *ctx = NULL;

	if (pdev == NULL)
		goto exit;

	ctx = (struct c2c *)platform_get_drvdata(pdev);
	if (ctx == NULL)
		goto exit;

	ret = clear_c2c_context(ctx);

exit:
	C2C_INFO("Unloaded module\n");
	return ret;
}

/* C2C driver registration as platform driver. */
static const struct of_device_id c2c_of_match[] = {
	{ .compatible = "nvidia,nvscic2c", },
	{},
};
MODULE_DEVICE_TABLE(of, c2c_of_match);

static struct platform_driver c2c_driver = {
	.probe  = c2c_probe,
	.remove = c2c_remove,
	.driver = {
		.name = MODULE_NAME,
		.of_match_table = c2c_of_match,
	},
};

/* driver entry point, register ourselves as platform driver. */
static int __init c2c_init(void)
{
	return platform_driver_register(&(c2c_driver));
}

/*
 * for it to be loadable.
 */
#if IS_MODULE(CONFIG_NVSCIC2C)
static void __exit c2c_deinit(void)
{
	platform_driver_unregister(&(c2c_driver));
}

module_init(c2c_init);
module_exit(c2c_deinit);
#else
late_initcall(c2c_init);
#endif
