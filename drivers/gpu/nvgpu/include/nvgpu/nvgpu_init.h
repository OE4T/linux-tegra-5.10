/*
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_INIT_H
#define NVGPU_INIT_H

struct gk20a;
struct nvgpu_ref;

/**
 * @file
 * @page unit-init Unit Init
 *
 * Overview
 * ========
 *
 * The Init unit is called by OS (QNX, Linux) to initialize or teardown the
 * driver. The Init unit ensures all the other sub-units are initialized so the
 * driver is able to provide general functionality to the application.
 *
 * Static Design
 * =============
 *
 * HAL Initialization
 * ------------------
 * The HAL must be initialized before the nvgpu_finalize_poweron() is called.
 * This is accomplished by calling nvgpu_detect_chip() which will determine
 * which GPU is in the system and configure the HAL interfaces.
 *
 * Common Initialization
 * ---------------------
 * The main driver initialization occurs by calling nvgpu_finalize_poweron()
 * which will initialize all of the common units in the driver and must be done
 * before the driver is ready to provide full functionality.
 *
 * Common Teardown
 * ---------------
 * If the GPU is unused, the driver can be torn down by calling
 * nvgpu_prepare_poweroff().
 *
 * External APIs
 * -------------
 * + nvgpu_detect_chip() - Called to initialize the HAL.
 * + nvgpu_finalize_poweron() - Called to initialize nvgpu driver.
 * + nvgpu_prepare_poweroff() - Called before powering off GPU HW.
 * + nvgpu_init_gpu_characteristics() - Called during HAL init for enable flag
 * processing.
 *
 * Dynamic Design
 * ==============
 * After initialization, the Init unit provides a number of APIs to track state
 * and usage count.
 *
 * External APIs
 * -------------
 * + nvgpu_get() / nvgpu_put() - Maintains ref count for usage.
 * + nvgpu_can_busy() - Check to make sure driver is ready to go busy.
 * + nvgpu_check_gpu_state() - Restart if the state is invalid.
 */

/**
 * @brief Final driver initialization
 *
 * @param g [in] The GPU
 *
 * Initializes GPU units in the GPU driver. Each sub-unit is responsible for HW
 * initialization.
 *
 * Note: Requires the GPU is already powered on and the HAL is initialized.
 *
 * @return 0 in case of success, < 0 in case of failure.
 */
int nvgpu_finalize_poweron(struct gk20a *g);

/**
 * @brief Prepare driver for poweroff
 *
 * @param g [in] The GPU
 *
 * Prepare the driver subsystems and HW for powering off GPU.
 *
 * @return 0 in case of success, < 0 in case of failure.
 */
int nvgpu_prepare_poweroff(struct gk20a *g);

/**
 * @brief Enter SW Quiesce state
 *
 * @param g [in] The GPU
 *
 * Enters SW quiesce state:
 * - set sw_quiesce_pending: When this flag is set, interrupt
 *   handlers exit after masking interrupts. This should help mitigate
 *   an interrupt storm.
 * - wake up thread to complete quiescing.
 *
 * The thread performs the following:
 * - set NVGPU_DRIVER_IS_DYING to prevent allocation of new resources
 * - disable interrupts
 * - disable fifo scheduling
 * - preempt all runlists
 * - set error notifier for all active channels
 *
 * @note: For channels with usermode submit enabled, userspace can
 * still ring doorbell, but this will not trigger any work on
 * engines since fifo scheduling is disabled.
 */
void nvgpu_sw_quiesce(struct gk20a *g);

/**
 * @brief Start GPU idle
 *
 * @param g [in] The GPU
 *
 * Set #NVGPU_DRIVER_IS_DYING to prevent allocation of new resources.
 * User API call will fail once this flag is set, as gk20a_busy will fail.
 */
void nvgpu_start_gpu_idle(struct gk20a *g);

/**
 * @brief Disable interrupt handlers
 *
 * @param g [in] The GPU
 *
 * Disable interrupt handlers.
 */
void nvgpu_disable_irqs(struct gk20a *g);

/**
 * @brief Check if the device can go busy
 *
 * @param g [in] The GPU
 *
 * @return 1 if it ok to go busy, 0 if it is not ok to go busy.
 */
int nvgpu_can_busy(struct gk20a *g);

/**
 * @brief Increment ref count on driver.
 *
 * @param g [in] The GPU
 *
 * This will fail if the driver is in the process of being released.
 *
 * @return pointer to g if successful, otherwise 0.
 */
struct gk20a * __must_check nvgpu_get(struct gk20a *g);

/**
 * @brief Decrement ref count on driver.
 *
 * @param g [in] The GPU
 *
 * Will free underlying driver memory if driver is no longer in use.
 */
void nvgpu_put(struct gk20a *g);

/**
 * @brief Check driver state and restart if the state is invalid
 *
 * @param g [in] The GPU
 *
 * If driver state is invalid, makes OS call to restart driver.
 */
void nvgpu_check_gpu_state(struct gk20a *g);

/**
 * @brief Configure initial GPU "enable" state and setup SM arch.
 *
 * @param g [in] The GPU
 *
 * This is called during HAL initialization.
 */
void nvgpu_init_gpu_characteristics(struct gk20a *g);

#endif /* NVGPU_INIT_H */
