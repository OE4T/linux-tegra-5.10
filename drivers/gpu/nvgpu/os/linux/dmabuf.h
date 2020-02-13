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

#ifndef NVGPU_DMABUF_H
#define NVGPU_DMABUF_H

struct sg_table;
struct dma_buf;
struct dma_buf_attachment;
struct device;

struct sg_table *gk20a_mm_pin(struct device *dev, struct dma_buf *dmabuf,
			      struct dma_buf_attachment **attachment);
void gk20a_mm_unpin(struct device *dev, struct dma_buf *dmabuf,
		    struct dma_buf_attachment *attachment,
		    struct sg_table *sgt);

#endif
