/*
 * Copyright (c) 2017-2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <linux/device.h>
#include <linux/dma-buf.h>
#include <linux/scatterlist.h>

#include <nvgpu/comptags.h>
#include <nvgpu/enabled.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/linux/vm.h>
#include <nvgpu/bug.h>
#include <nvgpu/fence.h>

#include "platform_gk20a.h"
#include "dmabuf.h"
#include "dmabuf_priv.h"
#include "os_linux.h"
#include "dmabuf_vidmem.h"

enum nvgpu_aperture gk20a_dmabuf_aperture(struct gk20a *g,
					  struct dma_buf *dmabuf)
{
#ifdef CONFIG_NVGPU_DGPU
	struct gk20a *buf_owner = nvgpu_vidmem_buf_owner(dmabuf);
	bool unified_memory = nvgpu_is_enabled(g, NVGPU_MM_UNIFIED_MEMORY);

	if (buf_owner == NULL) {
		/* Not nvgpu-allocated, assume system memory */
		return APERTURE_SYSMEM;
	} else if ((buf_owner == g) && unified_memory) {
		/* Looks like our video memory, but this gpu doesn't support
		 * it. Warn about a bug and bail out */
		nvgpu_do_assert_print(g,
			"dmabuf is our vidmem but we don't have local vidmem");
		return APERTURE_INVALID;
	} else if (buf_owner != g) {
		/* Someone else's vidmem */
		return APERTURE_INVALID;
	} else {
		/* Yay, buf_owner == g */
		return APERTURE_VIDMEM;
	}
#else
	return APERTURE_SYSMEM;
#endif
}

struct sg_table *gk20a_mm_pin(struct device *dev, struct dma_buf *dmabuf,
			      struct dma_buf_attachment **attachment)
{
#ifdef CONFIG_NVGPU_DMABUF_HAS_DRVDATA
	return gk20a_mm_pin_has_drvdata(dev, dmabuf, attachment);
#else
	struct dma_buf_attachment *attach = NULL;
	struct gk20a *g = get_gk20a(dev);
	struct sg_table *sgt = NULL;

	attach = dma_buf_attach(dmabuf, dev);
	if (IS_ERR(attach)) {
		nvgpu_err(g, "Failed to attach dma_buf (err = %ld)!",
			  PTR_ERR(attach));
		return ERR_CAST(attach);
	}

	sgt = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(sgt)) {
		dma_buf_detach(dmabuf, attach);
		nvgpu_err(g, "Failed to map attachment (err = %ld)!",
			  PTR_ERR(sgt));
		return ERR_CAST(sgt);
	}

	*attachment = attach;
	return sgt;
#endif
}

void gk20a_mm_unpin(struct device *dev, struct dma_buf *dmabuf,
		    struct dma_buf_attachment *attachment,
		    struct sg_table *sgt)
{
#ifdef CONFIG_NVGPU_DMABUF_HAS_DRVDATA
	gk20a_mm_unpin_has_drvdata(dev, dmabuf, attachment, sgt);
#else
	dma_buf_unmap_attachment(attachment, sgt, DMA_BIDIRECTIONAL);
	dma_buf_detach(dmabuf, attachment);
#endif
}
