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

#ifndef NVGPU_MC_H
#define NVGPU_MC_H

/**
 * @file
 * @page unit-mc Unit Master Control (MC)
 *
 * Overview
 * ========
 *
 * The Master Control (MC) unit is responsible for configuring HW units/engines
 * in the GPU.
 *
 * It provides interfaces to nvgpu driver to access the GPU chip details and
 * program HW units/engines through following registers:
 *
 *   + Boot registers: Setup by BIOS and read by nvgpu driver.
 *     - Has the information about architecture, implementation and revision.
 *
 *   + Interrupt registers: These allow to control the interrupts for the local
 *     devices. Interrupts are set by an event and are cleared by software.
 *
 *     Various interrupts sources are: Graphics, Copy*, NVENC*, NVDEC, SEC,
 *     PFIFO, HUB, PFB, THERMAL, HDACODEC, PTIMER, PMGR, NVLINK, DFD, PMU,
 *     LTC, PDISP, PBUS, XVE, PRIV_RING, SOFTWARE.
 *
 *     + There are two interrupt status registers:
 *       + mc_intr_r(0) is for stalling interrupts routed to CPU.
 *       + mc_intr_r(1) is for non-stalling interrupts routed to CPU.
 *     + There are two interrupt enable registers, which can be updated
 *	 through interrupt set/clear (mc_intr_set_r/mc_intr_clear_r)
 *       registers.
 *       + mc_intr_en_r(0) is for stalling interrupts routed to CPU.
 *       + mc_intr_en_r(1) is for non-stalling interrupts routed to CPU.
 *     + Register mc_intr_ltc_r indicates which of the FB partitions
 *       are reporting an LTC interrupt.
 *
 *   + Configuration registers: These are used to configure each of the HW
 *     units/engines after reset.
 *     - Master Control Enable Register (mc_enable_r()) is used to enable/
 *       disable engines.
 *
 * Data Structures
 * ===============
 *
 * + struct nvgpu_mc
 *   This struct holds the variables needed to manage the configuration and
 *   interrupt handling of the units/engines.
 *
 *
 * Static Design
 * =============
 *
 * nvgpu initialization
 * --------------------
 * Before initializing nvgpu driver, the MC unit interface to get the chip
 * version details is invoked. Interrupts are enabled at MC level in
 * #nvgpu_finalize_poweron and the engines are reset.
 *
 * nvgpu teardown
 * --------------
 * During #nvgpu_prepare_poweroff, all interrupts are disabled at MC level
 * by calling the interface from the MC unit.
 *
 * External APIs
 * -------------
 * Most of the static interfaces are HAL functions. They are documented
 * here.
 *   + include/nvgpu/gops_mc.h
 *
 * Dynamic Design
 * ==============
 *
 * At runtime, the stalling and non-stalling interrupts are inquired through
 * MC unit interface. Then corresponding handlers that are exported by the
 * MC unit are invoked. While in ISRs, interrupts are disabled and they
 * are re-enabled after ISRs through interfaces provided by the MC unit.
 *
 * For quiesce state handling, interrupts will have to be disabled that is
 * again supported through MC unit interface.
 *
 * External APIs
 * -------------
 * Some of the dynamic interfaces are HAL functions. They are documented
 * here.
 *   + include/nvgpu/gops_mc.h
 *
 * Following interface is common function.
 *   + nvgpu_wait_for_deferred_interrupts()
 */


#include <nvgpu/types.h>
#include <nvgpu/cond.h>
#include <nvgpu/atomic.h>
#include <nvgpu/lock.h>

struct gk20a;

/**
 * Enumeration of all units intended to be used by any HAL that requires
 * unit as parameter. Units are added to the enumeration as needed, so
 * it is not complete.
 */
enum nvgpu_unit {
	/** FIFO Engine */
	NVGPU_UNIT_FIFO,
	/** Performance Monitoring unit */
	NVGPU_UNIT_PERFMON,
	/** Graphics Engine */
	NVGPU_UNIT_GRAPH,
	/** BLPG and BLCG controllers within Graphics Engine */
	NVGPU_UNIT_BLG,
#ifdef CONFIG_NVGPU_HAL_NON_FUSA
	NVGPU_UNIT_PWR,
#endif
#ifdef CONFIG_NVGPU_DGPU
	NVGPU_UNIT_NVDEC,
#endif
};

/** Bit offset of the Architecture field in the HW version register */
#define NVGPU_GPU_ARCHITECTURE_SHIFT 4U

/** Index for accessing registers corresponding to stalling interrupts */
#define NVGPU_MC_INTR_STALLING		0U
/** Index for accessing registers corresponding to non-stalling interrupts */
#define NVGPU_MC_INTR_NONSTALLING	1U

/** Operations that will need to be executed on non stall workqueue. */
#define NVGPU_NONSTALL_OPS_WAKEUP_SEMAPHORE	BIT32(0)
#define NVGPU_NONSTALL_OPS_POST_EVENTS		BIT32(1)

/**
 * This struct holds the variables needed to manage the configuration and
 * interrupt handling of the units/engines.
 */
struct nvgpu_mc {
	/** Lock to access the mc_enable_r */
	struct nvgpu_spinlock enable_lock;

	/**
	 * Bitmask of the stalling/non-stalling interrupts enabled.
	 * This is used to enable/disable the interrupts at runtime.
	 * intr_mask_restore[2] & intr_mask_restore[3] are applicable
	 * when GSP exists.
	 */
	u32 intr_mask_restore[4];

	/**
	 * Below are the counters & condition varibles needed to keep track of
	 * the deferred interrupts.
	 */

	/**
	 * Stalling interrupt counter - incremented on receipt of the stalling
	 * interrupt in #isr_stall and read in the function
	 * #nvgpu_wait_for_deferred_interrupts.
	 */
	nvgpu_atomic_t hw_irq_stall_count;

	/**
	 * Non-stalling interrupt counter - incremented on receipt of the
	 * non-stalling interrupt in #isr_nonstall and read in the function
	 * #nvgpu_wait_for_deferred_interrupts.
	 */
	nvgpu_atomic_t hw_irq_nonstall_count;

	/**
	 * The condition variable that is signalled upon handling of the
	 * stalling interrupt. It is wait upon by the function
	 * #nvgpu_wait_for_deferred_interrupts.
	 */
	struct nvgpu_cond sw_irq_stall_last_handled_cond;

	/**
	 * Stalling interrupt status counter - updated on handling of the
	 * stalling interrupt.
	 */
	nvgpu_atomic_t sw_irq_stall_last_handled;

	/**
	 * The condition variable that is signalled upon handling of the
	 * non-stalling interrupt. It is wait upon by the function
	 * #nvgpu_wait_for_deferred_interrupts.
	 */
	struct nvgpu_cond sw_irq_nonstall_last_handled_cond;

	/**
	 * Non-stalling interrupt status counter - updated on handling of the
	 * non-stalling interrupt.
	 */
	nvgpu_atomic_t sw_irq_nonstall_last_handled;

	/** nvgpu interrupts enabled status from host OS perspective */
	bool irqs_enabled;

	/**
	 * Interrupt line for stalling interrupts.
	 * Can be same as irq_nonstall in case of PCI.
	 */
	u32 irq_stall;

	/** Interrupt line for non-stalling interrupts. */
	u32 irq_nonstall;
};

/**
 * @brief Wait for the interrupts to complete.
 *
 * @param g [in]	The GPU driver struct.
 *
 * While freeing the channel or entering SW quiesce state, nvgpu driver needs
 * to waits until all interrupt handlers that have been scheduled to run have
 * completed as those could access channel after freeing.
 * Steps:
 * - Get the stalling and non-stalling interrupts atomic count.
 * - Wait on the condition variable #sw_irq_stall_last_handled_cond until
 *   #sw_irq_stall_last_handled becomes greater than or equal to previously
 *   read stalling interrupt atomic count.
 * - Wait on the condition variable #sw_irq_nonstall_last_handled_cond until
 *   #sw_irq_nonstall_last_handled becomes greater than or equal to previously
 *   read non-stalling interrupt atomic count.
 */
void nvgpu_wait_for_deferred_interrupts(struct gk20a *g);

#endif
