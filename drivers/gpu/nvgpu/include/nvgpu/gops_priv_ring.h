/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
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
#ifndef NVGPU_GOPS_PRIV_RING_H
#define NVGPU_GOPS_PRIV_RING_H

#include <nvgpu/types.h>

/**
 * @file
 *
 * common.priv_ring interface.
 */
struct gk20a;

/**
 * common.priv_ring unit hal operations.
 *
 * This structure stores priv_ring unit hal pointers.
 *
 * @see gpu_ops
 */
struct gops_priv_ring {
	/**
	 * @brief Enable priv ring h/w register access for s/w.
	 *
	 * @param g [in]		Pointer to GPU driver struct.
	 *
	 * This function enables PRIvilege Ring to access h/w functionality.
	 * This function loads slcg priv ring prod values through
	 * #nvgpu_cg_slcg_priring_load_enable, then initiate priv ring
	 * enumeration and wait for priv ring enumeration complete to
	 * accept s/w register. This function then enables the PRIV_RING
	 * unit stalling interrupt at MC level.
	 *
	 * @return 0 in case of success, < 0 in case of failure.
	 */

	int (*enable_priv_ring)(struct gk20a *g);
	/**
	 * @brief ISR handler for priv ring error.
	 *
	 * @param g [in]		Pointer to GPU driver struct.
	 *
	 * This functions handles interrupts related to priv ring faults.
	 * Priv ring faults are related to priv ring connection errors and
	 * global register write errors.
	 */

	void (*isr)(struct gk20a *g);

	/**
	 * @brief Unit level interrupt handler for priv ring
	 *
	 * @param g [in]		Pointer to GPU driver struct.
	 * @param status0 [in]		Value of interrupt status register
	 *
	 * This function handles interrupts associated with priv ring
	 * status0 interrupt register.
	 */

	void (*isr_handle_0)(struct gk20a *g, u32 status0);

	/**
	 * @brief Unit level interrupt handler for priv ring
	 *
	 * @param g [in]		Pointer to GPU driver struct.
	 * @param status1 [in]		Value of interrupt status register
	 *
	 * This function handles interrupts associated with priv ring
	 * status1 interrupt register.
	 */

	void (*isr_handle_1)(struct gk20a *g, u32 status1);

	/**
	 * @brief Sets Priv ring timeout value in cycles.
	 *
	 * @param g [in]		Pointer to GPU driver struct.
	 *
	 * This functions sets h/w specified timeout value in the number of
	 * cycles after sending a priv request. If timeout is exceeded then
	 * timeout error reported back.
	 */

	void (*set_ppriv_timeout_settings)(struct gk20a *g);
	/**
	 * @brief Returns number of enumerated Level Two Cache (LTC) chiplets.
	 *
	 * @param g [in]		Pointer to GPU driver struct.
	 *
	 * This function returns number of enumerated ltc chiplets after
	 * floor-sweeping.
	 *
	 * @return U32 Number of ltc units.
	 */

	u32 (*enum_ltc)(struct gk20a *g);
	/**
	 * @brief Returns number of enumerated Graphics Processing Cluster (GPC)
	 * chiplets.
	 *
	 * @param g [in]		Pointer to GPU driver struct.
	 *
	 * This function returns number of enumerated gpc chiplets after
	 * floor-sweeping.
	 *
	 * @return U32 Number of gpc units.
	 */
	u32 (*get_gpc_count)(struct gk20a *g);

	/**
	 * @brief Returns number of enumerated Frame Buffer Partitions (FBP).
	 *
	 * @param g [in]		Pointer to GPU driver struct.
	 *
	 * This function returns number of enumerated fbp chiplets after
	 * floor-sweeping.
	 *
	 * @return U32 Number of fbp units.
	 */
	u32 (*get_fbp_count)(struct gk20a *g);

	/** @cond DOXYGEN_SHOULD_SKIP_THIS */
	void (*decode_error_code)(struct gk20a *g, u32 error_code);
	/** @endcond DOXYGEN_SHOULD_SKIP_THIS */

#if defined(CONFIG_NVGPU_NEXT)
#include "include/nvgpu/nvgpu_next_gops_priv_ring.h"
#endif

};

#endif /* NVGPU_GOPS_PRIV_RING_H */

