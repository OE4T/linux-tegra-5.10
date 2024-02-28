/*
 * Copyright (c) 2013-2021 NVIDIA Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/atomic.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <asm/cacheflush.h>
#include <linux/list.h>
#include <linux/dma-mapping.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/clk.h>
#include <linux/reset.h>

#include "ote_protocol.h"

#define SET_ANSWER(a, r, ro)	{ a.result = r; a.result_origin = ro; }

struct tlk_device tlk_dev;
DEFINE_MUTEX(smc_lock);

static int te_create_free_cmd_list(struct tlk_device *dev)
{
	int cmd_desc_count, ret = 0;
	struct te_cmd_req_desc *req_desc = NULL, *tmp_req_desc = NULL;
	int bitmap_size;
	int req_buf_size;
	void *req_buf;

	/*
	 * TLK can map in the shared req/param buffers and do_smc
	 * only needs to send the offsets within each (with cache coherency
	 * being maintained by HW through an NS mapping).
	 */
	req_buf_size = (TE_TOTAL_PAGE_COUNT * PAGE_SIZE);
	req_buf = kmalloc(req_buf_size, GFP_KERNEL);
	if (!req_buf) {
		pr_err("%s: Failed to allocate param buffer!\n", __func__);
		ret = -ENOMEM;
		goto error;
	}

	/* requests in 1st page, params in 2nd, pagelists in 3rd and 4th pages */
	dev->req_addr = (struct te_request *)
			(req_buf + (0 * PAGE_SIZE));
	dev->param_addr = (struct te_oper_param *)
			(req_buf + (1 * PAGE_SIZE));
	dev->plist_addr = (uint64_t *)
			(req_buf + (2 * PAGE_SIZE));

	/* alloc param bitmap allocator */
	bitmap_size = BITS_TO_LONGS(TE_PARAM_MAX) * sizeof(long);
	dev->param_bitmap = kzalloc(bitmap_size, GFP_KERNEL);
	if (!dev->param_bitmap) {
		pr_err("%s: Failed to allocate param bitmap\n", __func__);
		ret = -ENOMEM;
		goto error;
	}

	/* alloc plist bitmap allocator */
	bitmap_size = BITS_TO_LONGS(TE_PLIST_MAX) * sizeof(long);
	dev->plist_bitmap = kzalloc(bitmap_size, GFP_KERNEL);
	if (!dev->plist_bitmap) {
		pr_err("%s: Failed to allocate plist bitmap\n", __func__);
		ret = -ENOMEM;
		goto error;
	}

	tlk_send_smc(TE_SMC_REGISTER_REQ_BUF,
			(uintptr_t)dev->req_addr, req_buf_size);

	if (!dev->req_addr || !dev->param_addr || !dev->plist_addr) {
		pr_err("%s: Bad dev request addr/param addr/plist addr!\n",
			__func__);
		ret = -ENOMEM;
		goto error;
	}

	for (cmd_desc_count = 0;
		cmd_desc_count < TE_CMD_DESC_MAX; cmd_desc_count++) {

		req_desc = kzalloc(sizeof(struct te_cmd_req_desc), GFP_KERNEL);
		if (req_desc == NULL) {
			pr_err("%s: Failed to allocate cmd req descriptor\n",
				__func__);
			ret = -ENOMEM;
			goto error;
		}
		req_desc->req_addr = dev->req_addr + cmd_desc_count;
		INIT_LIST_HEAD(&(req_desc->list));

		/* Add the cmd param descriptor to free list */
		list_add_tail(&req_desc->list, &(dev->free_cmd_list));
	}
	return 0;
error:
	pr_err("%s: Error, returning %d\n", __func__, ret);
	kfree(req_buf);
	kfree(dev->param_bitmap);
	kfree(dev->plist_bitmap);
	list_for_each_entry_safe(req_desc, tmp_req_desc,
			&(dev->free_cmd_list), list)
		kfree(req_desc);
	return ret;
}

static void te_release_free_cmd_list(struct tlk_device *dev)
{
	kfree(dev->req_addr);
	kfree(dev->param_bitmap);
	kfree(dev->plist_bitmap);
}

struct te_oper_param *te_get_free_params(struct tlk_device *dev,
	unsigned int nparams)
{
	struct te_oper_param *params = NULL;
	int idx, nbits;

	if (nparams) {
		nbits = get_count_order(nparams);
		idx = bitmap_find_free_region(dev->param_bitmap,
				TE_PARAM_MAX, nbits);
		if (idx >= 0)
			params = dev->param_addr + idx;
	}
	return params;
}

void te_put_free_params(struct tlk_device *dev,
	struct te_oper_param *params, uint32_t nparams)
{
	int idx, nbits;

	idx = (params - dev->param_addr);
	nbits = get_count_order(nparams);
	bitmap_release_region(dev->param_bitmap, idx, nbits);
}

struct te_cmd_req_desc *te_get_free_cmd_desc(struct tlk_device *dev)
{
	struct te_cmd_req_desc *cmd_desc = NULL;

	if (!(list_empty(&(dev->free_cmd_list)))) {
		cmd_desc = list_first_entry(&(dev->free_cmd_list),
				struct te_cmd_req_desc, list);
		list_del(&(cmd_desc->list));
		list_add_tail(&cmd_desc->list, &(dev->used_cmd_list));
	}
	return cmd_desc;
}

void te_put_used_cmd_desc(struct tlk_device *dev,
	struct te_cmd_req_desc *cmd_desc)
{
	struct te_cmd_req_desc *param_desc, *tmp_param_desc;

	if (cmd_desc) {
		list_for_each_entry_safe(param_desc, tmp_param_desc,
				&(dev->used_cmd_list), list) {
			if (cmd_desc->req_addr == param_desc->req_addr) {
				list_del(&param_desc->list);
				list_add_tail(&param_desc->list,
					&(dev->free_cmd_list));
			}
		}
	}
}

static void __attribute__((unused)) te_print_cmd_list(
	struct tlk_device *dev, int used_list)
{
	struct te_cmd_req_desc *param_desc;

	if (!used_list) {
		pr_info("Printing free cmd list\n");
		if (!(list_empty(&(dev->free_cmd_list)))) {
			list_for_each_entry(param_desc, &(dev->free_cmd_list),
					list)
				pr_info("Phys addr for cmd req desc (%p)\n",
					param_desc->req_addr);
		}
	} else {
		pr_info("Printing used cmd list\n");
		if (!(list_empty(&(dev->used_cmd_list)))) {
			list_for_each_entry(param_desc, &(dev->used_cmd_list),
					list)
				pr_info("Phys addr for cmd req desc (%p)\n",
					param_desc->req_addr);
		}
	}
}


static void te_close_sessions(struct tlk_context *context)
{
	struct tlk_device *dev = context->dev;
	union te_cmd cmd;
	struct te_cmd_req_desc *cmd_desc = NULL;
	struct te_request *request;
	struct te_session *session, *tmp_session;

	if (list_empty(&context->session_list))
		return;

	cmd_desc = te_get_free_cmd_desc(dev);
	if (!cmd_desc) {
		pr_err("%s: failed to get cmd_desc\n", __func__);
		return;
	}

	request = cmd_desc->req_addr;

	list_for_each_entry_safe(session, tmp_session,
		&context->session_list, list) {

		memset(request, 0, sizeof(struct te_request));

		cmd.closesession.session_id = session->session_id;

		te_close_session(&cmd.closesession, request, context);
	}

	te_put_used_cmd_desc(dev, cmd_desc);
}

static int tlk_device_open(struct inode *inode, struct file *file)
{
	struct tlk_context *context;
	int ret = 0;

	context = kzalloc(sizeof(struct tlk_context), GFP_KERNEL);
	if (!context) {
		ret = -ENOMEM;
		goto error;
	}
	context->dev = &tlk_dev;
	INIT_LIST_HEAD(&context->session_list);

	file->private_data = context;
	return 0;
error:
	return ret;
}

static int tlk_device_release(struct inode *inode, struct file *file)
{
	struct tlk_context *context = file->private_data;

	/* close any open sessions */
	mutex_lock(&smc_lock);
	te_close_sessions(context);
	mutex_unlock(&smc_lock);

	kfree(file->private_data);
	file->private_data = NULL;
	return 0;
}

static int copy_params_from_user(struct te_request *req,
	struct te_operation *operation, struct te_oper_param *caller_params)
{
	struct te_oper_param *param_array;
	struct te_oper_param *user_param;
	uint32_t i;

	if (operation->list_count == 0)
		return 0;

	param_array = (struct te_oper_param *)(uintptr_t)req->params;
	if (param_array == NULL) {
		pr_err("param_array empty\n");
		return 1;
	}

	user_param = (struct te_oper_param *)(uintptr_t)operation->list_head;
	for (i = 0; i < operation->list_count && user_param != NULL; i++) {
		if (copy_from_user(param_array + i, user_param,
					sizeof(struct te_oper_param))) {
			pr_err("Failed to copy operation parameter:%d, %p, " \
				"list_count: %d\n",
				i, user_param, operation->list_count);
			return 1;
		}
		user_param = (struct te_oper_param *)(uintptr_t)
			param_array[i].next_ptr_user;
	}

	memcpy(caller_params, param_array,
			sizeof(struct te_oper_param) * operation->list_count);

	return 0;
}

static int copy_params_to_user(struct te_request *req,
	struct te_operation *operation, struct te_oper_param *caller_params)
{
	struct te_oper_param *param_array;
	struct te_oper_param *user_param;
	uint32_t i;

	if (operation->list_count == 0)
		return 0;

	param_array = (struct te_oper_param *)(uintptr_t)req->params;
	if (param_array == NULL) {
		pr_err("param_array empty\n");
		return 1;
	}

	user_param =
		(struct te_oper_param *)(uintptr_t)operation->list_head;
	for (i = 0; i < req->params_size; i++) {
		/* clear flags */
		param_array[i].type &= ~TE_PARAM_TYPE_ALL_FLAGS;

		switch(param_array[i].type) {
		/*
		 * Restore the memory base address as it can be overridden
		 * while sending it to secure world
		 */
		case TE_PARAM_TYPE_MEM_RO:
		case TE_PARAM_TYPE_MEM_RW:
		case TE_PARAM_TYPE_PERSIST_MEM_RO:
		case TE_PARAM_TYPE_PERSIST_MEM_RW:
			param_array[i].u.Mem.base = caller_params[i].u.Mem.base;
		}

		if (copy_to_user(user_param, param_array + i,
					sizeof(struct te_oper_param))) {
			pr_err("Failed to copy back parameter:%d %p\n", i,
				user_param);
			return 1;
		}
		user_param = (struct te_oper_param *)(uintptr_t)
			param_array[i].next_ptr_user;
	}
	return 0;
}

static long te_handle_trustedapp_ioctl(struct file *file,
	unsigned int ioctl_num, unsigned long ioctl_param)
{
	long err = 0;
	union te_cmd cmd;
	struct te_operation *operation = NULL;
	struct te_oper_param *params = NULL;
	struct te_oper_param *caller_params = NULL;
	struct te_request *request;
	void __user *ptr_user_answer = NULL;
	struct te_answer answer;
	struct te_cmd_req_desc *cmd_desc = NULL;
	struct tlk_context *context = file->private_data;
	struct tlk_device *dev = context->dev;

	if (copy_from_user(&cmd, (void __user *)ioctl_param,
				sizeof(union te_cmd))) {
		pr_err("Failed to copy command request\n");
		err = -EFAULT;
		goto error;
	}

	memset(&answer, 0, sizeof(struct te_answer));

	switch (ioctl_num) {
	case TE_IOCTL_OPEN_CLIENT_SESSION:
		operation = &cmd.opensession.operation;
		ptr_user_answer = (void *)(uintptr_t)cmd.opensession.answer;

		cmd_desc = te_get_free_cmd_desc(dev);
		params = te_get_free_params(dev, operation->list_count);

		if (!cmd_desc || (operation->list_count && !params)) {
			SET_ANSWER(answer,
				   OTE_ERROR_OUT_OF_MEMORY,
				   OTE_RESULT_ORIGIN_COMMS);
			pr_err("failed to get cmd_desc/params\n");
			goto error;
		}

		request = cmd_desc->req_addr;
		memset(request, 0, sizeof(struct te_request));

		request->params = (uintptr_t)params;
		request->params_size = operation->list_count;

		if (operation->list_count > 0) {
			caller_params = kmalloc(sizeof(struct te_oper_param) *
					operation->list_count, GFP_KERNEL);
			if (!caller_params) {
				pr_err("%s: failed to allocate caller params\n", __func__);
				err = -ENOMEM;
				goto error;
			}
		}

		if (copy_params_from_user(request, operation, caller_params)) {
			err = -EFAULT;
			pr_err("%s: failed to copy params from user\n", __func__);
			goto error;
		}

		te_open_session(&cmd.opensession, request, context);

		SET_ANSWER(answer, request->result, request->result_origin);
		answer.session_id = request->session_id;
		break;

	case TE_IOCTL_CLOSE_CLIENT_SESSION:
		ptr_user_answer = (void *)(uintptr_t)cmd.closesession.answer;
		cmd_desc = te_get_free_cmd_desc(dev);
		if (!cmd_desc) {
			SET_ANSWER(answer,
				   OTE_ERROR_OUT_OF_MEMORY,
				   OTE_RESULT_ORIGIN_COMMS);
			pr_err("failed to get cmd_desc\n");
			goto error;
		}

		request = cmd_desc->req_addr;
		memset(request, 0, sizeof(struct te_request));

		/* close session cannot fail */
		te_close_session(&cmd.closesession, request, context);
		break;

	case TE_IOCTL_LAUNCH_OPERATION:
		operation = &cmd.launchop.operation;
		ptr_user_answer = (void *)(uintptr_t)cmd.launchop.answer;

		cmd_desc = te_get_free_cmd_desc(dev);
		params = te_get_free_params(dev, operation->list_count);

		if (!cmd_desc || (operation->list_count && !params)) {
			SET_ANSWER(answer,
				   OTE_ERROR_OUT_OF_MEMORY,
				   OTE_RESULT_ORIGIN_COMMS);
			pr_err("failed to get cmd_desc/params\n");
			goto error;
		}

		request = cmd_desc->req_addr;
		memset(request, 0, sizeof(struct te_request));

		request->params = (uintptr_t)params;
		request->params_size = operation->list_count;

		if (operation->list_count > 0) {
			caller_params = kmalloc(sizeof(struct te_oper_param) *
					operation->list_count, GFP_KERNEL);
			if (!caller_params) {
				pr_err("%s: failed to allocate caller params\n", __func__);
				err = -ENOMEM;
				goto error;
			}
		}

		if (copy_params_from_user(request, operation, caller_params)) {
			err = -EFAULT;
			pr_info("%s: failed to copy params from user\n", __func__);
			goto error;
		}

		te_launch_operation(&cmd.launchop, request, context);

		SET_ANSWER(answer, request->result, request->result_origin);
		break;

	default:
		pr_err("Invalid IOCTL Cmd\n");
		err = -EINVAL;
		goto error;
	}
	if (ptr_user_answer && !err) {
		if (copy_to_user(ptr_user_answer, &answer,
			sizeof(struct te_answer))) {
			pr_err("Failed to copy answer\n");
			err = -EFAULT;
		}
	}
	if (request->params && !err) {
		if (copy_params_to_user(request, operation, caller_params)) {
			pr_err("Failed to copy return params\n");
			err = -EFAULT;
		}
	}

error:
	if (cmd_desc)
		te_put_used_cmd_desc(dev, cmd_desc);
	if (params)
		te_put_free_params(dev, params, operation->list_count);
	kfree(caller_params);
	return err;
}

static long tlk_device_ioctl(struct file *file, unsigned int ioctl_num,
	unsigned long ioctl_param)
{
	int err;

	switch (ioctl_num) {
	case TE_IOCTL_OPEN_CLIENT_SESSION:
	case TE_IOCTL_CLOSE_CLIENT_SESSION:
	case TE_IOCTL_LAUNCH_OPERATION:
		mutex_lock(&smc_lock);
		err = te_handle_trustedapp_ioctl(file, ioctl_num, ioctl_param);
		mutex_unlock(&smc_lock);
		break;

	case TE_IOCTL_SS_CMD:
		err = te_handle_ss_ioctl(file, ioctl_num, ioctl_param);
		break;

	default:
		pr_err("%s: Invalid IOCTL (0x%x) id 0x%x max 0x%lx\n",
			__func__, ioctl_num, _IOC_NR(ioctl_num),
			(unsigned long)TE_IOCTL_MAX_NR);
		err = -EINVAL;
		break;
	}

	return err;
}

static struct device_node *get_tlk_device_node(void)
{
	struct device_node *node = NULL;
	node = of_find_compatible_node(NULL, NULL, "android,tlk-driver");
	if (!node)
		pr_debug("TLK node not present in FDT\n");
	return node;
}

int te_is_secos_dev_enabled(void)
{
	static int tlk_dev_status = 0;
	struct device_node *node = NULL;

	if (unlikely(tlk_dev_status == 0)) {
		node = get_tlk_device_node();
		tlk_dev_status = (node && of_device_is_available(node));
	}

	return tlk_dev_status;
}

/*
 * tlk_driver function definitions.
 */
static const struct file_operations tlk_device_fops = {
	.owner = THIS_MODULE,
	.open = tlk_device_open,
	.release = tlk_device_release,
	.unlocked_ioctl = tlk_device_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = tlk_device_ioctl,
#endif
};

struct miscdevice tlk_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "tlk_device",
	.fops = &tlk_device_fops,
};

static int tlk_driver_probe(struct platform_device *pdev)
{
	struct reset_control *rst;
	struct clk *clk;
	int ret;

	clk = devm_clk_get(&pdev->dev, "nvdec");
	if (!IS_ERR(clk)) {
		ret = clk_prepare_enable(clk);
		if (ret)
			return ret;
	} else if (PTR_ERR(clk) != -ENOENT) {
		return PTR_ERR(clk);
	} else {
		pr_err("%s: nvdec clock not available\n", __func__);
	}

	rst = devm_reset_control_get(&pdev->dev, "nvdec");
	if (!IS_ERR(rst)) {
		ret = reset_control_deassert(rst);
		if (ret)
			return ret;
	} else if (PTR_ERR(rst) != -ENOENT) {
		return PTR_ERR(rst);
	} else {
		pr_err("%s: nvdec reset not available\n", __func__);
	}

	platform_set_drvdata(pdev, clk);

	ret = of_platform_populate(pdev->dev.of_node, NULL, NULL, &pdev->dev);
	if (ret) {
		pr_err("%s: of_platform_populate failed\n", __func__);
		goto disable_clk;
	}

	if (!te_is_secos_dev_enabled()) {
		ret = -ENODEV;
		goto disable_clk;
	}

	return 0;

disable_clk:
	if (!IS_ERR(clk))
		clk_disable_unprepare(clk);

	return ret;
}

static int tlk_driver_remove(struct platform_device *pdev) {
	struct clk *clk = platform_get_drvdata(pdev);

	if (!IS_ERR(clk))
		clk_disable_unprepare(clk);

	return 0;
}

static const struct of_device_id tlk_driver_of_match[] = {
	{ .compatible = "android,tlk-driver",},
	{},
};

static struct platform_driver tlk_driver = {
	.probe = tlk_driver_probe,
	.remove = tlk_driver_remove,
	.driver = {
		.name = "tlk-driver",
		.owner = THIS_MODULE,
		.of_match_table = tlk_driver_of_match,
	}
};

static int __init tlk_driver_init(void)
{
	if (get_tlk_device_node()) {
		int ret;

		INIT_LIST_HEAD(&(tlk_dev.used_cmd_list));
		INIT_LIST_HEAD(&(tlk_dev.free_cmd_list));

		ret = te_create_free_cmd_list(&tlk_dev);
		if (ret != 0) {
			pr_err("%s: failed to create free_list\n", __func__);
			return ret;
		}
	}

	return platform_driver_register(&tlk_driver);
}

static void __exit tlk_driver_exit(void)
{
	platform_driver_unregister(&tlk_driver);

	if (get_tlk_device_node())
		te_release_free_cmd_list(&tlk_dev);
}

/* Initialize early so that other device drivers can use it during boot */
subsys_initcall(tlk_driver_init);
module_exit(tlk_driver_exit);

static int __init tlk_driver_misc_init(void)
{
	int ret;

	if (!te_is_secos_dev_enabled()) {
		pr_info("%s: tlk not enabled on this device\n", __func__);
		return -ENODEV;
	}

	ret = misc_register(&tlk_misc_device);
	if (ret)
		pr_err("%s: misc_register failed: %d\n", __func__, ret);

	return ret;
}

static void __exit tlk_driver_misc_exit(void)
{
	if (te_is_secos_dev_enabled())
		misc_deregister(&tlk_misc_device);
}

module_init(tlk_driver_misc_init);
module_exit(tlk_driver_misc_exit);
