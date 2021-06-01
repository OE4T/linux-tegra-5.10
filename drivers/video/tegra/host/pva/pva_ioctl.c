/*
 * PVA Ioctl Handling
 *
 * Copyright (c) 2016-2021, NVIDIA Corporation.  All rights reserved.
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

#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/nospec.h>

#include <asm/ioctls.h>
#include <asm/barrier.h>

#include <uapi/linux/nvdev_fence.h>
#include <uapi/linux/nvpva_ioctl.h>

#include "pva.h"
#include "pva_queue.h"
#include "dev.h"
#include "nvhost_buffer.h"
#include "nvhost_acm.h"
#include "pva_vpu_exe.h"
#include "nvpva_client.h"
/**
 * @brief pva_private - Per-fd specific data
 *
 * pdev		Pointer the pva device
 * queue	Pointer the struct nvhost_queue
 * buffer	Pointer to the struct nvhost_buffer
 */
struct pva_private {
	struct pva *pva;
	struct nvhost_queue *queue;
	struct nvhost_buffers *buffers;
	struct nvpva_client_context *client;
};

/**
 * @brief	Copy a single task from userspace to kernel space
 *
 * This function copies fields from ioctl_task and performs a deep copy
 * of the task to kernel memory. At the same time, input values shall
 * be validated. This allows using all the fields without manually performing
 * copies of the structure and performing checks later.
 *
 * @param ioctl_task	Pointer to a userspace task that is copied
 *				to kernel memory
 * @param task		Pointer to a task that should be created
 * @return		0 on Success or negative error code
 *
 */
static int pva_copy_task(struct pva_ioctl_submit_task *ioctl_task,
			 struct pva_submit_task *task)
{
	int err = 0;
	int copy_ret = 0;
	int i;

	if (ioctl_task->num_prefences > PVA_MAX_PREFENCES ||
	    ioctl_task->num_input_task_status > PVA_MAX_INPUT_STATUS ||
	    ioctl_task->num_output_task_status > PVA_MAX_OUTPUT_STATUS ||
	    ioctl_task->num_input_surfaces > PVA_MAX_INPUT_SURFACES ||
	    ioctl_task->num_output_surfaces > PVA_MAX_OUTPUT_SURFACES ||
	    ioctl_task->num_pointers > PVA_MAX_POINTERS ||
	    ioctl_task->primary_payload_size > PVA_MAX_PRIMARY_PAYLOAD_SIZE) {
		err = -EINVAL;
		goto err_out;
	}

	/*
	 * These fields are clear-text in the task descriptor. Just
	 * copy them.
	 */
	task->operation			= ioctl_task->operation;
	task->num_prefences		= ioctl_task->num_prefences;
	task->num_input_task_status	= ioctl_task->num_input_task_status;
	task->num_output_task_status	= ioctl_task->num_output_task_status;
	task->num_input_surfaces	= ioctl_task->num_input_surfaces;
	task->num_output_surfaces	= ioctl_task->num_output_surfaces;
	task->num_pointers		= ioctl_task->num_pointers;
	task->primary_payload_size	= ioctl_task->primary_payload_size;
	task->input_scalars		= ioctl_task->input_scalars;
	task->output_scalars		= ioctl_task->output_scalars;
	task->timeout			= ioctl_task->timeout;

	/* Copy the user primary_payload */
	if (task->primary_payload_size) {
		copy_ret = copy_from_user(task->primary_payload,
				(void __user *)(ioctl_task->primary_payload),
				ioctl_task->primary_payload_size);
		if (copy_ret) {
			err = -EFAULT;
			goto err_out;
		}
	}

#define COPY_FIELD(dst, src, num, type)					\
	do {								\
		if ((num) == 0) {					\
			break;						\
		}							\
		copy_ret = copy_from_user((dst),			\
				(void __user *)(src),			\
				(num) * sizeof(type));			\
		if (copy_ret) {						\
			err = -EFAULT;					\
			goto err_out;					\
		}							\
	} while (0)

	/* Copy the fields */
	COPY_FIELD(task->input_surfaces, ioctl_task->input_surfaces,
			task->num_input_surfaces,
			struct pva_surface);
	COPY_FIELD(task->output_surfaces, ioctl_task->output_surfaces,
			task->num_output_surfaces,
			struct pva_surface);
	COPY_FIELD(task->prefences, ioctl_task->prefences, task->num_prefences,
			struct nvdev_fence);
	COPY_FIELD(task->input_task_status, ioctl_task->input_task_status,
			task->num_input_task_status,
			struct pva_status_handle);
	COPY_FIELD(task->output_task_status, ioctl_task->output_task_status,
			task->num_output_task_status,
			struct pva_status_handle);
	COPY_FIELD(task->pointers, ioctl_task->pointers,
			task->num_pointers, struct pva_memory_handle);

	COPY_FIELD(task->pvafences, ioctl_task->pvafences,
			sizeof(task->pvafences), u8);
	COPY_FIELD(task->num_pvafences, ioctl_task->num_pvafences,
		   sizeof(task->num_pvafences), u8);
	COPY_FIELD(task->num_pva_ts_buffers, ioctl_task->num_pva_ts_buffers,
		   sizeof(task->num_pva_ts_buffers), u8);

	for (i = 0; i < PVA_MAX_FENCE_TYPES; i++) {
		if ((task->num_pvafences[i] > PVA_MAX_FENCES_PER_TYPE) ||
			(task->num_pva_ts_buffers[i] >
			 PVA_MAX_FENCES_PER_TYPE)) {
			err = -EINVAL;
			goto err_out;
		}
	}


#undef COPY_FIELD

err_out:
	return err;
}

/**
 * @brief	Submit a task to PVA
 *
 * This function takes the given list of tasks, converts
 * them into kernel internal representation and submits
 * them to the task queue. On success, it populates
 * the post-fence structures in userspace and returns 0.
 *
 * @param priv	PVA Private data
 * @param arg	ioctl data
 * @return	0 on Success or negative error code
 *
 */
static int pva_submit(struct pva_private *priv, void *arg)
{
	struct pva_ioctl_submit_args *ioctl_tasks_header =
		(struct pva_ioctl_submit_args *)arg;
	struct pva_ioctl_submit_task *ioctl_tasks = NULL;
	struct pva_submit_tasks tasks_header;
	struct pva_submit_task *task = NULL;
	int err = 0;
	int i;
	int j;
	int k;
	int threshold;

	memset(&tasks_header, 0, sizeof(tasks_header));

	/* Sanity checks for the task heaader */
	if (ioctl_tasks_header->num_tasks > PVA_MAX_TASKS) {
		err = -EINVAL;
		goto err_check_num_tasks;
	}

	ioctl_tasks_header->num_tasks =
		array_index_nospec(ioctl_tasks_header->num_tasks,
					PVA_MAX_TASKS + 1);

	if (ioctl_tasks_header->version > 0) {
		err = -ENOSYS;
		goto err_check_version;
	}

	/* Allocate memory for the UMD representation of the tasks */
	ioctl_tasks = kcalloc(ioctl_tasks_header->num_tasks,
			sizeof(*ioctl_tasks), GFP_KERNEL);
	if (!ioctl_tasks) {
		err = -ENOMEM;
		goto err_alloc_task_mem;
	}

	/* Copy the tasks from userspace */
	err = copy_from_user(ioctl_tasks,
			(void __user *)ioctl_tasks_header->tasks,
			ioctl_tasks_header->num_tasks * sizeof(*ioctl_tasks));
	if (err < 0) {
		err = -EFAULT;
		goto err_copy_tasks;
	}

	/* Go through the tasks and make a KMD representation of them */
	for (i = 0; i < ioctl_tasks_header->num_tasks; i++) {

		struct nvhost_queue_task_mem_info task_mem_info;

		/* Allocate memory for the task and dma */
		err = nvhost_queue_alloc_task_memory(priv->queue,
							&task_mem_info);
		task = task_mem_info.kmem_addr;
		if ((err < 0) || !task)
			goto err_get_task_buffer;

		err = pva_copy_task(ioctl_tasks + i, task);
		if (err < 0)
			goto err_copy_tasks;

		INIT_LIST_HEAD(&task->node);
		/* Obtain an initial reference */
		kref_init(&task->ref);

		task->pva = priv->pva;
		task->queue = priv->queue;
		task->buffers = priv->buffers;

		task->dma_addr = task_mem_info.dma_addr;
		task->va = task_mem_info.va;
		task->pool_index = task_mem_info.pool_index;

		tasks_header.tasks[i] = task;
		tasks_header.num_tasks += 1;
	}

	/* Populate header structure */
	tasks_header.flags = ioctl_tasks_header->flags;

	/* ..and submit them */
	err = nvhost_queue_submit(priv->queue, &tasks_header);

	if (err < 0) {
		goto err_submit_task;
	}

	/* Copy fences back to userspace */
	for (i = 0; i < ioctl_tasks_header->num_tasks; i++) {
		struct nvpva_fence __user *pvafences =
			(struct nvpva_fence __user *)
			ioctl_tasks[i].pvafences;

		struct platform_device *host1x_pdev =
			to_platform_device(priv->queue->vm_pdev->dev.parent);

		task = tasks_header.tasks[i];

		threshold = tasks_header.task_thresh[i] - task->fence_num + 1;

		/* Return post-fences */
		for (k = 0; k < PVA_MAX_FENCE_TYPES; k++) {
			u32 increment = 0;
			struct nvdev_fence *fence;

			if ((task->num_pvafences[k] == 0) ||
				(k == PVA_FENCE_PRE)) {
				continue;
			}

			switch (k) {
			case PVA_FENCE_SOT_V:
				increment = 1;
				break;
			case PVA_FENCE_SOT_R:
				increment = 1;
				break;
			case PVA_FENCE_POST:
				increment = 1;
				break;
			default:

				break;
			};

			for (j = 0; j < task->num_pvafences[k]; j++) {
				fence = &task->pvafences[k][j].fence;

				switch (fence->type) {
				case NVDEV_FENCE_TYPE_SYNCPT: {
					fence->syncpoint_index =
						priv->queue->syncpt_id;
					fence->syncpoint_value =
						threshold;
					threshold += increment;
					break;
				}
				case NVDEV_FENCE_TYPE_SYNC_FD: {
					struct nvhost_ctrl_sync_fence_info pts;

					pts.id = priv->queue->syncpt_id;
					pts.thresh = threshold;
					threshold += increment;
					err = nvhost_fence_create_fd(
						host1x_pdev,
						&pts, 1,
						"fence_pva",
						&fence->sync_fd);

					break;
				}
				case NVDEV_FENCE_TYPE_SEMAPHORE:
					break;
				default:
					err = -ENOSYS;
					nvhost_warn(&priv->pva->pdev->dev,
						    "Bad fence type");
				}
			}
		}

		err = copy_to_user(pvafences,
				   task->pvafences,
				   sizeof(task->pvafences));
		if (err < 0) {
			nvhost_warn(&priv->pva->pdev->dev,
				    "Failed to copy  pva fences to userspace");
			break;
		}
		/* Drop the reference */
		kref_put(&task->ref, pva_task_free);
	}

	kfree(ioctl_tasks);
	return 0;

err_submit_task:
err_get_task_buffer:
err_copy_tasks:
	for (i = 0; i < tasks_header.num_tasks; i++) {
		task = tasks_header.tasks[i];
		/* Drop the reference */
		kref_put(&task->ref, pva_task_free);
	}
err_alloc_task_mem:
	kfree(ioctl_tasks);
err_check_version:
err_check_num_tasks:
	return err;
}


static int pva_pin(struct pva_private *priv, void *arg)
{
	u32 *handles;
	int err = 0;
	int i = 0;
	struct dma_buf *dmabufs[PVA_MAX_PIN_BUFFERS];
	struct pva_pin_unpin_args *buf_list = (struct pva_pin_unpin_args *)arg;
	u32 count = buf_list->num_buffers;

	if (count > PVA_MAX_PIN_BUFFERS)
		return -EINVAL;

	handles = kcalloc(count, sizeof(u32), GFP_KERNEL);
	if (!handles)
		return -ENOMEM;

	if (copy_from_user(handles, (void __user *)buf_list->buffers,
			(count * sizeof(u32)))) {
		err = -EFAULT;
		goto pva_buffer_cpy_err;
	}

	/* get the dmabuf pointer from the fd handle */
	for (i = 0; i < count; i++) {
		dmabufs[i] = dma_buf_get(handles[i]);
		if (IS_ERR_OR_NULL(dmabufs[i])) {
			err = -EFAULT;
			goto pva_buffer_get_err;
		}
	}

	err = nvhost_buffer_pin(priv->buffers, dmabufs, count);

pva_buffer_get_err:
	count = i;
	for (i = 0; i < count; i++)
		dma_buf_put(dmabufs[i]);

pva_buffer_cpy_err:
	kfree(handles);
	return err;
}

static int pva_unpin(struct pva_private *priv, void *arg)
{
	u32 *handles;
	int i = 0;
	int err = 0;
	struct dma_buf *dmabufs[PVA_MAX_PIN_BUFFERS];
	struct pva_pin_unpin_args *buf_list = (struct pva_pin_unpin_args *)arg;
	u32 count = buf_list->num_buffers;

	if (count > PVA_MAX_PIN_BUFFERS)
		return -EINVAL;

	handles = kcalloc(count, sizeof(u32), GFP_KERNEL);
	if (!handles)
		return -ENOMEM;

	if (copy_from_user(handles, (void __user *)buf_list->buffers,
			(count * sizeof(u32)))) {
		err = -EFAULT;
		goto pva_buffer_cpy_err;
	}

	/* get the dmabuf pointer and clean valid ones */
	for (i = 0; i < count; i++) {
		dmabufs[i] = dma_buf_get(handles[i]);
		if (IS_ERR_OR_NULL(dmabufs[i]))
			continue;
	}

	nvhost_buffer_unpin(priv->buffers, dmabufs, count);

	for (i = 0; i < count; i++) {
		if (IS_ERR_OR_NULL(dmabufs[i]))
			continue;

		dma_buf_put(dmabufs[i]);
	}

pva_buffer_cpy_err:
	kfree(handles);
	return err;
}

static int pva_register_vpu_exec(struct pva_private *priv, void *arg,
				 const void __user *user_arg)
{
	struct nvpva_vpu_exe_register_in_arg *reg_in =
		(struct nvpva_vpu_exe_register_in_arg *)arg;
	struct nvpva_vpu_exe_register_out_arg *reg_out =
		(struct nvpva_vpu_exe_register_out_arg *)arg;
	void *exec_data;
	size_t copy_size;
	int err = 0;
	uint16_t exe_id;

	exec_data = kmalloc(reg_in->size, GFP_KERNEL);
	if (!exec_data) {
		err = -ENOMEM;
		goto out;
	}
	copy_size = copy_from_user(
		exec_data, (const void __user *)(user_arg + sizeof(*reg_in)),
		reg_in->size);
	if (copy_size) {
		nvhost_err(
			&priv->pva->pdev->dev,
			"failed to copy all executable data; size failed to copy: %lu/%u.",
			copy_size, reg_in->size);
		goto free_mem;
	}
	err = pva_load_vpu_app(&priv->client->elf_ctx, exec_data, reg_in->size,
			       &exe_id);
	if (err) {
		nvhost_err(&priv->pva->pdev->dev, "failed to register vpu app");
		goto free_mem;
	}

	reg_out->exe_id = exe_id;
	reg_out->num_of_symbols =
		priv->client->elf_ctx.elf_images->elf_img[exe_id].num_symbols;
	reg_out->symbol_size_total =
		priv->client->elf_ctx.elf_images->elf_img[exe_id]
			.symbol_size_total;

free_mem:
	kfree(exec_data);
out:
	return err;
}

static int pva_unregister_vpu_exec(struct pva_private *priv, void *arg)
{
	return 0;
}

static int pva_get_symbol_id(struct pva_private *priv, void *arg)
{
	return 0;
}

static long pva_ioctl(struct file *file, unsigned int cmd,
			unsigned long arg)
{
	struct pva_private *priv = file->private_data;
	u8 buf[NVHOST_PVA_IOCTL_MAX_ARG_SIZE] __aligned(sizeof(u64));
	int err = 0;

	nvhost_dbg_fn("");

	if ((_IOC_TYPE(cmd) != NVPVA_IOCTL_MAGIC) ||
	    (_IOC_NR(cmd) == 0) ||
	    (_IOC_NR(cmd) > NVPVA_IOCTL_NUMBER_MAX) ||
	    (_IOC_SIZE(cmd) > sizeof(buf)))
		return -ENOIOCTLCMD;

	if (_IOC_DIR(cmd) & _IOC_WRITE) {
		if (copy_from_user(buf, (void __user *)arg, _IOC_SIZE(cmd))) {
			dev_err(&priv->pva->pdev->dev,
				"failed copy ioctl buffer from user; size: %u",
				_IOC_SIZE(cmd));
			return -EFAULT;
		}
	}

	switch (cmd) {
	case NVPVA_IOCTL_REGISTER_VPU_EXEC:
		err = pva_register_vpu_exec(priv, buf, (void __user *)arg);
		break;
	case NVPVA_IOCTL_UNREGISTER_VPU_EXEC:
		err = pva_unregister_vpu_exec(priv, buf);
		break;
	case NVPVA_IOCTL_GET_SYMBOL_ID:
		err = pva_get_symbol_id(priv, buf);
		break;
	case NVPVA_IOCTL_PIN:
		err = pva_pin(priv, buf);
		break;
	case NVPVA_IOCTL_UNPIN:
		err = pva_unpin(priv, buf);
		break;
	case NVPVA_IOCTL_SUBMIT:
		err = pva_submit(priv, buf);
		break;
	default:
		return -ENOIOCTLCMD;
	}

	if ((err == 0) && (_IOC_DIR(cmd) & _IOC_READ))
		err = copy_to_user((void __user *)arg, buf, _IOC_SIZE(cmd));

	return err;
}

static int pva_open(struct inode *inode, struct file *file)
{
	struct nvhost_device_data *pdata = container_of(inode->i_cdev,
					struct nvhost_device_data, ctrl_cdev);
	struct platform_device *pdev = pdata->pdev;
	struct pva *pva = pdata->private_data;
	struct pva_private *priv;
	int err = 0;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (priv == NULL) {
		err = -ENOMEM;
		goto err_alloc_priv;
	}

	file->private_data = priv;
	priv->pva = pva;

	/* add the pva client to nvhost */
	err = nvhost_module_add_client(pdev, priv);
	if (err < 0)
		goto err_add_client;

	priv->queue = nvhost_queue_alloc(pva->pool, MAX_PVA_TASK_COUNT,
		pva->submit_task_mode == PVA_SUBMIT_MODE_CHANNEL_CCQ);
	if (IS_ERR(priv->queue)) {
		err = PTR_ERR(priv->queue);
		goto err_alloc_queue;
	}

	priv->buffers = nvhost_buffer_init(priv->queue->vm_pdev);
	if (IS_ERR(priv->buffers)) {
		err = PTR_ERR(priv->buffers);
		goto err_alloc_buffer;
	}

	priv->client = nvpva_client_context_alloc(pva, current->pid);
	if (priv->client == NULL) {
		err = -ENOMEM;
		dev_err(&pdev->dev, "failed to allocate client context");
		goto err_alloc_context;
	}

	return nonseekable_open(inode, file);

err_alloc_context:
	nvhost_buffer_release(priv->buffers);
err_alloc_buffer:
	nvhost_queue_put(priv->queue);
err_alloc_queue:
	nvhost_module_remove_client(pdev, priv);
err_add_client:
	kfree(priv);
err_alloc_priv:
	return err;
}

static int pva_release(struct inode *inode, struct file *file)
{
	struct pva_private *priv = file->private_data;

	/*
	 * Release handle to the queue (on-going tasks have their
	 * own references to the queue
	 */
	nvhost_queue_put(priv->queue);

	/* Release handle to nvhost_acm */
	nvhost_module_remove_client(priv->pva->pdev, priv);

	/* Release the handle to buffer structure */
	nvhost_buffer_release(priv->buffers);

	nvpva_client_context_free(priv->pva, priv->client);

	/* Finally, release the private data */
	kfree(priv);

	return 0;
}

const struct file_operations tegra_pva_ctrl_ops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.unlocked_ioctl = pva_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = pva_ioctl,
#endif
	.open = pva_open,
	.release = pva_release,
};
