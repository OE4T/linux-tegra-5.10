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

#ifndef NVGPU_NEXT_TOP_H
#define NVGPU_NEXT_TOP_H

#include <nvgpu/pbdma.h>

struct nvgpu_device_next {
	/**
	 * True if the device is an method engine behind host.
	 */
	bool engine;

	/**
	 * Runlist Engine ID; only valid if #engine is true.
	 */
	u32 rleng_id;

	/**
	 * Runlist PRI base - byte aligned based address. CHRAM offset can
	 * be computed from this.
	 */
	u32 rl_pri_base;

	/**
	 * PBDMA info for this device. It may contain multiple PBDMAs as
	 * there can now be multiple PBDMAs per runlist.
	 *
	 * This is in some ways awkward; devices seem to be more directly
	 * linked to runlists; runlists in turn have PBDMAs. Granted that
	 * means there's a computable relation between devices and PBDMAs
	 * it may make sense to not have this link.
	 */
	struct nvgpu_next_pbdma_info pbdma_info;

};

#endif
