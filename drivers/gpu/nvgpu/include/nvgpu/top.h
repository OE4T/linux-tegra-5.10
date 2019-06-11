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

#ifndef NVGPU_TOP_H
#define NVGPU_TOP_H
/**
 * @file
 *
 * Declare device info specific struct and defines.
 */
#include <nvgpu/types.h>

struct gk20a;

/** Engine enum type graphics as defined by h/w. */
#define NVGPU_ENGINE_GRAPHICS		0U
/** Obsolete from Pascal and beyond architecture. */
#define NVGPU_ENGINE_COPY0		1U
/** Obsolete from Pascal and beyond architecture. */
#define NVGPU_ENGINE_COPY1		2U
/** Obsolete from Pascal and beyond architecture. */
#define NVGPU_ENGINE_COPY2		3U
#define NVGPU_ENGINE_IOCTRL		18U
/** Engine enum type lce as defined by h/w. */
#define NVGPU_ENGINE_LCE		19U

/**
 * Structure definition for storing information for the devices and the engines
 * available on the chip.
 */
struct nvgpu_device_info {
	/** Engine enum type defined by device info h/w register. */
	u32 engine_type;
	/**
	 * Specifies instance of a device, allowing s/w to distinguish between
	 * multiple copies of a device present on the chip.
	 */
	u32 inst_id;
	/**
	 * Used to determine the start of the h/w register address space
	 * for #inst_id 0.
	 */
	u32 pri_base;
	/**
	 * Contains valid mmu fault id read from device info h/w register or
	 * invalid mmu fault id, U32_MAX.
	 */
	u32 fault_id;
	/** Engine id read from device info h/w register. */
	u32 engine_id;
	/** Runlist id read from device info h/w register. */
	u32 runlist_id;
	/** Intr id read from device info h/w register. */
	u32 intr_id;
	/** Reset id read from device info h/w register. */
	u32 reset_id;
};

#endif /* NVGPU_TOP_H */
