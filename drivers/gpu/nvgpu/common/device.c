/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <nvgpu/gk20a.h>
#include <nvgpu/device.h>
#include <nvgpu/list.h>
#include <nvgpu/kmem.h>
#include <nvgpu/string.h>
#include <nvgpu/log.h>

static inline struct nvgpu_device *
nvgpu_device_from_dev_list_node(struct nvgpu_list_node *node)
{
	return (struct nvgpu_device *)
		((uintptr_t)node - offsetof(struct nvgpu_device,
					    dev_list_node));
};

struct nvgpu_device_list {
	/**
	 * Array of lists of devices; each list corresponds to one type of
	 * device. By having this as an array it's trivial to go from device
	 * enum type in the HW to the relevant devlist.
	 */
	struct nvgpu_list_node devlist_heads[NVGPU_MAX_DEVTYPE];

	/**
	 * Keep track of how many devices of each type exist.
	 */
	u32 dev_counts[NVGPU_MAX_DEVTYPE];
};

/*
 * Faciliate the parsing of the TOP array describing the devices present in the
 * GPU.
 */
static int nvgpu_device_parse_hw_table(struct gk20a *g)
{
	int ret = 0;
	u32 token = NVGPU_DEVICE_TOKEN_INIT;
	struct nvgpu_device *dev;
	struct nvgpu_list_node *devlist;

	while (true) {
		dev = g->ops.top.parse_next_device(g, &token);
		if (dev == NULL) {
			break;
		}

		nvgpu_log(g, gpu_dbg_info, "Parsed one device: %u", dev->type);

		/*
		 * Otherwise we have a device - let's add it to the right device
		 * list.
		 */
		devlist = &g->devs->devlist_heads[dev->type];

		nvgpu_list_add_tail(&dev->dev_list_node, devlist);
		g->devs->dev_counts[dev->type] += 1;
	}

	return ret;
}

/*
 * Faciliate reading the HW register table into a software abstraction. This is
 * done only on the first boot as the table will never change dynamically.
 */
int nvgpu_device_init(struct gk20a *g)
{
	u32 i;

	/*
	 * Ground work - make sure we aren't doing this again and that we have
	 * all the necessary data structures.
	 */
	if (g->devs != NULL) {
		return 0;
	}

	nvgpu_log(g, gpu_dbg_info, "Initialization GPU device list");

	g->devs = nvgpu_kzalloc(g, sizeof(*g->devs));
	if (g->devs == NULL) {
		return -ENOMEM;
	}

	for (i = 0; i < NVGPU_MAX_DEVTYPE; i++) {
		nvgpu_init_list_node(&g->devs->devlist_heads[i]);
	}

	return nvgpu_device_parse_hw_table(g);
}

static void nvgpu_device_cleanup_devtype(struct gk20a *g,
					 struct nvgpu_list_node *list)
{
	struct nvgpu_device *dev;

	while (!nvgpu_list_empty(list)) {
		dev = nvgpu_list_first_entry(list,
					     nvgpu_device,
					     dev_list_node);
		nvgpu_list_del(&dev->dev_list_node);
		nvgpu_kfree(g, dev);
	}
}

void nvgpu_device_cleanup(struct gk20a *g)
{
	u32 i;
	struct nvgpu_list_node *devlist;

	for (i = 0; i < NVGPU_MAX_DEVTYPE; i++) {
		devlist = &g->devs->devlist_heads[i];

		if (devlist == NULL) {
			continue;
		}

		nvgpu_device_cleanup_devtype(g, devlist);
	}

	nvgpu_kfree(g, g->devs);
	g->devs = NULL;
}

/*
 * Find the instance passed. Do this by simply traversing the linked list; it's
 * not particularly efficient, but we aren't expecting there to ever be _that_
 * many devices.
 *
 * Return a pointer to the device or NULL of the inst ID is out of range.
 */
static const struct nvgpu_device *dev_instance_from_devlist(
	struct nvgpu_list_node *devlist, u32 inst_id)
{
	u32 i = 0U;
	struct nvgpu_device *dev;

	nvgpu_list_for_each_entry(dev, devlist, nvgpu_device, dev_list_node) {
		if (inst_id == i) {
			return dev;
		}
		i++;
	}

	return NULL;
}

const struct nvgpu_device *nvgpu_device_get(struct gk20a *g,
		     u32 type, u32 inst_id)
{
	const struct nvgpu_device *dev;
	struct nvgpu_list_node *device_list;

	if (type >= NVGPU_MAX_DEVTYPE) {
		return NULL;
	}

	device_list = &g->devs->devlist_heads[type];
	dev = dev_instance_from_devlist(device_list, inst_id);

	if (dev == NULL) {
		return NULL;
	}

	return dev;
}

u32 nvgpu_device_count(struct gk20a *g, u32 type)
{
	if (type >= NVGPU_MAX_DEVTYPE) {
		return 0U;
	}

	return g->devs->dev_counts[type];
}

/*
 * Note: this kind of bleeds HW details into the core code. Eventually this
 * should be handled by a translation table. However, for now, HW has kept the
 * device type values consistent across chips and nvgpu already has this present
 * in core code.
 *
 * Once a per-chip translation table exists we can translate and then do a
 * comparison.
 */
bool nvgpu_device_is_ce(struct gk20a *g, const struct nvgpu_device *dev)
{
	if (dev->type == NVGPU_DEVTYPE_COPY0 ||
	    dev->type == NVGPU_DEVTYPE_COPY1 ||
	    dev->type == NVGPU_DEVTYPE_COPY2 ||
	    dev->type == NVGPU_DEVTYPE_LCE) {
		return true;
	}

	return false;
}

bool nvgpu_device_is_graphics(struct gk20a *g, const struct nvgpu_device *dev)
{
	return dev->type == NVGPU_DEVTYPE_GRAPHICS;
}
