/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/firmware.h>

#ifdef CONFIG_TEGRA_NVLINK
/*
 * WAR: use this function to find detault link, as only one is supported
 * on the library for now
 * Returns NVLINK_MAX_LINKS_SW on failure
 */
static u32 nvgpu_nvlink_get_link(struct gk20a *g)
{
	u32 link_id;

	if (!g)
		return NVLINK_MAX_LINKS_SW;

	/* Lets find the detected link */
	if (g->nvlink.initialized_links)
		link_id = ffs(g->nvlink.initialized_links) - 1;
	else
		return NVLINK_MAX_LINKS_SW;

	if (g->nvlink.links[link_id].remote_info.is_connected)
		return link_id;

	return NVLINK_MAX_LINKS_SW;
}

int nvgpu_nvlink_speed_config(struct gk20a *g)
{
	return g->ops.nvlink.speed_config(g);
}

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
	link_id = ffs(g->nvlink.discovered_links) - 1;
	g->nvlink.links[link_id].remote_info.is_connected = true;
	g->nvlink.links[link_id].remote_info.device_type =
							nvgpu_nvlink_endp_tegra;
	return g->ops.nvlink.link_early_init(g, BIT(link_id));

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

	if (g->ops.nvlink.interface_disable)
		err = g->ops.nvlink.interface_disable(g);
	return err;
}

int nvgpu_nvlink_dev_shutdown(struct gk20a *g)
{
	int err;

	err = g->ops.nvlink.shutdown(g);
	return err;
}

u32 nvgpu_nvlink_get_link_mode(struct gk20a *g)
{
	u32 link_id;

	link_id = nvgpu_nvlink_get_link(g);
	if (link_id == NVLINK_MAX_LINKS_SW)
		return -EINVAL;

	return g->ops.nvlink.link_get_mode(g, link_id);
}

u32 nvgpu_nvlink_get_link_state(struct gk20a *g)
{
	u32 link_id;

	link_id = nvgpu_nvlink_get_link(g);
	if (link_id == NVLINK_MAX_LINKS_SW)
		return -EINVAL;

	return g->ops.nvlink.link_get_state(g, link_id);
}

int nvgpu_nvlink_set_link_mode(struct gk20a *g, u32 mode)
{

	u32 link_id;

	link_id = nvgpu_nvlink_get_link(g);
	if (link_id == NVLINK_MAX_LINKS_SW)
		return -EINVAL;

	return g->ops.nvlink.link_set_mode(g, link_id, mode);
}

void nvgpu_nvlink_get_tx_sublink_state(struct gk20a *g, u32 *state)
{
	u32 link_id;

	link_id = nvgpu_nvlink_get_link(g);
	if (link_id == NVLINK_MAX_LINKS_SW)
		return;
	if (state)
		*state = g->ops.nvlink.get_tx_sublink_state(g, link_id);
}

void nvgpu_nvlink_get_rx_sublink_state(struct gk20a *g, u32 *state)
{
	u32 link_id;

	link_id = nvgpu_nvlink_get_link(g);
	if (link_id == NVLINK_MAX_LINKS_SW)
		return;
	if (state)
		*state = g->ops.nvlink.get_rx_sublink_state(g, link_id);
}

u32 nvgpu_nvlink_get_sublink_mode(struct gk20a *g, bool is_rx_sublink)
{
	u32 link_id;

	link_id = nvgpu_nvlink_get_link(g);
	if (link_id == NVLINK_MAX_LINKS_SW)
		return -EINVAL;

	return g->ops.nvlink.get_sublink_mode(g, link_id, is_rx_sublink);


}

int nvgpu_nvlink_set_sublink_mode(struct gk20a *g,
						bool is_rx_sublink, u32 mode)
{
	u32 link_id;

	link_id = nvgpu_nvlink_get_link(g);
	if (link_id == NVLINK_MAX_LINKS_SW)
		return -EINVAL;


	return g->ops.nvlink.set_sublink_mode(g, link_id, is_rx_sublink, mode);
}

/* Extract a WORD from the MINION ucode */
u32 nvgpu_nvlink_minion_extract_word(struct nvgpu_firmware *fw, u32 idx)
{
	u32 out_data = 0;
	u8 byte = 0;
	u32 i = 0;

	for (i = 0; i < 4; i++) {
		byte = fw->data[idx + i];
		out_data |= ((u32)byte) << (8 * i);
	}

	return out_data;
}

#endif

int nvgpu_nvlink_remove(struct gk20a *g)
{
#ifdef CONFIG_TEGRA_NVLINK
	int err;

	if (!nvgpu_is_enabled(g, NVGPU_SUPPORT_NVLINK))
		return -ENODEV;

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
