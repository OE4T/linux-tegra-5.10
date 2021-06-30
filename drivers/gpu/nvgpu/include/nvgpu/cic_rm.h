/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_CIC_RM_H
#define NVGPU_CIC_RM_H

#include <nvgpu/log.h>

/**
 * @brief Wait for the stalling interrupts to complete.
 *
 * @param g [in]	The GPU driver struct.
 * @param timeout [in]  Timeout
 *
 * Steps:
 * - Get the stalling interrupts atomic count.
 * - Wait for #timeout duration on the condition variable
 *   #sw_irq_stall_last_handled_cond until #sw_irq_stall_last_handled
 *   becomes greater than or equal to previously read stalling
 *   interrupt atomic count.
 *
 * @retval 0 if wait completes successfully.
 * @retval -ETIMEDOUT if wait completes without stalling interrupts
 * completing.
 */
int nvgpu_cic_rm_wait_for_stall_interrupts(struct gk20a *g, u32 timeout);


/**
 * @brief Wait for the non-stalling interrupts to complete.
 *
 * @param g [in]	The GPU driver struct.
 * @param timeout [in]  Timeout
 *
 * Steps:
 * - Get the non-stalling interrupts atomic count.
 * - Wait for #timeout duration on the condition variable
 *   #sw_irq_nonstall_last_handled_cond until #sw_irq_nonstall_last_handled
 *   becomes greater than or equal to previously read non-stalling
 *   interrupt atomic count.
 *
 * @retval 0 if wait completes successfully.
 * @retval -ETIMEDOUT if wait completes without nonstalling interrupts
 * completing.
 */
int  nvgpu_cic_rm_wait_for_nonstall_interrupts(struct gk20a *g, u32 timeout);

/**
 * @brief Wait for the interrupts to complete.
 *
 * @param g [in]	The GPU driver struct.
 *
 * While freeing the channel or entering SW quiesce state, nvgpu driver needs
 * to wait until all scheduled interrupt handlers have completed. This is
 * because the interrupt handlers could access data structures after freeing.
 * Steps:
 * - Wait for stalling interrupts to complete with timeout disabled.
 * - Wait for non-stalling interrupts to complete with timeout disabled.
 */
void nvgpu_cic_rm_wait_for_deferred_interrupts(struct gk20a *g);

#ifdef CONFIG_NVGPU_NON_FUSA
void nvgpu_cic_rm_log_pending_intrs(struct gk20a *g);
#endif

#endif /* NVGPU_CIC_RM_H */
