/*
 * Copyright (c) 2018-2020, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/nvlink.h>
#include <nvgpu/nvlink_probe.h>
#include <nvgpu/enabled.h>

#ifdef CONFIG_NVGPU_NVLINK

int nvgpu_nvlink_early_init(struct gk20a *g)
{
	return g->ops.nvlink.early_init(g);
}

int nvgpu_nvlink_link_early_init(struct gk20a *g)
{
	u32 link_id;

	/*
	 * First check the topology and setup connectivity
	 * HACK: we are only enabling one link for now!!!
	 */
	link_id = (u32)(nvgpu_ffs(g->nvlink.discovered_links) - 1UL);
	g->nvlink.links[link_id].remote_info.is_connected = true;
	g->nvlink.links[link_id].remote_info.device_type =
							nvgpu_nvlink_endp_tegra;
	return g->ops.nvlink.link_early_init(g, BIT32(link_id));

}

int nvgpu_nvlink_interface_init(struct gk20a *g)
{
	int err;

	err = g->ops.nvlink.interface_init(g);
	return err;
}

int nvgpu_nvlink_interface_disable(struct gk20a *g)
{
	int err = 0;

	if (g->ops.nvlink.interface_disable != NULL) {
		err = g->ops.nvlink.interface_disable(g);
	}
	return err;
}

int nvgpu_nvlink_dev_shutdown(struct gk20a *g)
{
	int err;

	err = g->ops.nvlink.shutdown(g);
	return err;
}
#endif

int nvgpu_nvlink_remove(struct gk20a *g)
{
#ifdef CONFIG_NVGPU_NVLINK
	int err;

	if (!nvgpu_is_enabled(g, NVGPU_SUPPORT_NVLINK)) {
		return -ENODEV;
	}

	nvgpu_set_enabled(g, NVGPU_SUPPORT_NVLINK, false);

	err = nvgpu_nvlink_unregister_link(g);
	if (err != 0) {
		nvgpu_err(g, "failed on nvlink link unregistration");
		return err;
	}

	err = nvgpu_nvlink_unregister_device(g);
	if (err != 0) {
		nvgpu_err(g, "failed on nvlink device unregistration");
		return err;
	}

	nvgpu_kfree(g, g->nvlink.priv);

	return 0;
#else
	return -ENODEV;
#endif
}
