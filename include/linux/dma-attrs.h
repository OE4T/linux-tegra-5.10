/*
 * Copyright (c) 2020 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef _DMA_ATTR_H
#define _DMA_ATTR_H

#include <linux/bitmap.h>
#include <linux/bitops.h>
#include <linux/bug.h>

#define DEFINE_DMA_ATTRS(attrs) unsigned long attrs = 0
#define __DMA_ATTR(attrs) attrs
typedef unsigned long dma_attr;

/**
 * dma_set_attr - set a specific attribute
 * @attr: attribute to set
 * @attrs: struct dma_attrs (may be NULL)
 */
#define dma_set_attr(attr, attrs) (attrs |= attr)

/**
 * dma_get_attr - check for a specific attribute
 * @attr: attribute to set
 * @attrs: struct dma_attrs (may be NULL)
 */
#define dma_get_attr(attr, attrs) (attrs & attr)

#endif /* _DMA_ATTR_H */
