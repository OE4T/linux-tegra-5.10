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
#ifndef NVGPU_NEXT_GOPS_MC_H
#define NVGPU_NEXT_GOPS_MC_H

	/* Leave extra tab to fit into gops_mc structure */

	/**
	 * @brief Reset HW engines.
	 *
	 * @param g [in]		The GPU driver struct.
	 * @param devtype [in]		Type of device.
	 *
	 * This function is invoked to reset the engines while initializing
	 * GR, CE and other engines during #nvgpu_finalize_poweron.
	 *
	 * Steps:
	 * - Compute reset mask for all engines of given devtype.
	 * - Disable given HW engines.
	 *   - Acquire g->mc.enable_lock spinlock.
	 *   - Read mc_device_enable_r register and clear the bits in read value
	 *     corresponding to HW engines to be disabled.
	 *   - Write mc_device_enable_r with the updated value.
	 *   - Poll mc_device_enable_r to confirm register write success.
	 *   - Release g->mc.enable_lock spinlock.
	 * - If GR engines are being reset, reset GPCs.
	 * - Enable the HW engines.
	 *   - Acquire g->mc.enable_lock spinlock.
	 *   - Read mc_device_enable_r register and set the bits in read value
	 *     corresponding to HW engines to be enabled.
	 *   - Write mc_device_enable_r with the updated value.
	 *   - Poll mc_device_enable_r to confirm register write success.
	 *   - Release g->mc.enable_lock spinlock.
	 */
	int (*reset_engines_all)(struct gk20a *g, u32 devtype);
#ifdef CONFIG_NVGPU_HAL_NON_FUSA
	void (*elpg_enable)(struct gk20a *g);
#endif
	bool (*intr_get_unit_info)(struct gk20a *g, u32 unit);

#endif /* NVGPU_NEXT_GOPS_MC_H */
