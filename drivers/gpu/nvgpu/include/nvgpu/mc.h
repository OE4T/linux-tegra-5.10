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

#if defined(CONFIG_NVGPU_NON_FUSA) && defined(CONFIG_NVGPU_NEXT)
#include "include/nvgpu/nvgpu_next_mc.h"
#endif

struct gk20a;

/**
 * @defgroup NVGPU_MC_UNIT_ENUMS
 *
 * Enumeration of all units intended to be used by enabling/disabling HAL
 * that requires unit as parameter. Units are added to the enumeration as
 * needed, so it is not complete.
 */

/**
 * @ingroup NVGPU_MC_UNIT_ENUMS
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

/**
 * @defgroup NVGPU_MC_INTR_TYPE_DEFINES
 *
 * Defines of all MC unit interrupt types.
 */

/**
 * @ingroup NVGPU_MC_INTR_TYPE_DEFINES
 */
/**
 * Index for accessing registers corresponding to stalling interrupts.
 */
#define NVGPU_MC_INTR_STALLING		0U
/**
 * Index for accessing registers corresponding to non-stalling
 * interrupts.
 */
#define NVGPU_MC_INTR_NONSTALLING	1U

/** Operations that will need to be executed on non stall workqueue. */
#define NVGPU_NONSTALL_OPS_WAKEUP_SEMAPHORE	BIT32(0)
#define NVGPU_NONSTALL_OPS_POST_EVENTS		BIT32(1)

/**
 * @defgroup NVGPU_MC_INTR_UNIT_DEFINES
 *
 * Defines of all units intended to be used by any interrupt related
 * HAL that requires unit as parameter.
 */

/**
 * @ingroup NVGPU_MC_INTR_UNIT_DEFINES
 */
/** MC interrupt for Bus unit. */
#define MC_INTR_UNIT_BUS	0
/** MC interrupt for PRIV_RING unit. */
#define MC_INTR_UNIT_PRIV_RING	1
/** MC interrupt for FIFO unit. */
#define MC_INTR_UNIT_FIFO	2
/** MC interrupt for LTC unit. */
#define MC_INTR_UNIT_LTC	3
/** MC interrupt for HUB unit. */
#define MC_INTR_UNIT_HUB	4
/** MC interrupt for GR unit. */
#define MC_INTR_UNIT_GR		5
/** MC interrupt for PMU unit. */
#define MC_INTR_UNIT_PMU	6
/** MC interrupt for CE unit. */
#define MC_INTR_UNIT_CE		7
/** MC interrupt for NVLINK unit. */
#define MC_INTR_UNIT_NVLINK	8
/** MC interrupt for FBPA unit. */
#define MC_INTR_UNIT_FBPA	9

/**
 * @defgroup NVGPU_MC_INTR_ENABLE_DEFINES
 *
 * Defines for MC unit interrupt enabling/disabling.
 */

/**
 * @ingroup NVGPU_MC_INTR_ENABLE_DEFINES
 * Value to be passed to mc.intr_*_unit_config to enable the interrupt.
 */
#define MC_INTR_ENABLE		true

/**
 * @ingroup NVGPU_MC_INTR_ENABLE_DEFINES
 * Value to be passed to mc.intr_*_unit_config to disable the interrupt.
 */
#define MC_INTR_DISABLE		false

/**
 * This struct holds the variables needed to manage the configuration and
 * interrupt handling of the units/engines.
 */
struct nvgpu_mc {
	/** Lock to access the MC interrupt registers */
	struct nvgpu_spinlock intr_lock;

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
	 * The condition variable that is signalled upon handling of the
	 * stalling interrupt. It is wait upon by the function
	 * #nvgpu_wait_for_deferred_interrupts.
	 */
	struct nvgpu_cond sw_irq_stall_last_handled_cond;

	/**
	 * Stalling interrupt status counter - Set to 1 on entering stalling
	 * interrupt handler and reset to 0 on exit.
	 */
	nvgpu_atomic_t sw_irq_stall_pending;

	/**
	 * The condition variable that is signalled upon handling of the
	 * non-stalling interrupt. It is wait upon by the function
	 * #nvgpu_wait_for_deferred_interrupts.
	 */
	struct nvgpu_cond sw_irq_nonstall_last_handled_cond;

	/**
	 * Non-stalling interrupt status counter - Set to 1 on entering
	 * non-stalling interrupt handler and reset to 0 on exit.
	 */
	nvgpu_atomic_t sw_irq_nonstall_pending;

	/** nvgpu interrupts enabled status from host OS perspective */
	bool irqs_enabled;

	/**
	 * Interrupt line for stalling interrupts.
	 * Can be same as irq_nonstall in case of PCI.
	 */
	u32 irq_stall;

	/** Interrupt line for non-stalling interrupts. */
	u32 irq_nonstall;

	/** @cond DOXYGEN_SHOULD_SKIP_THIS */
#if defined(CONFIG_NVGPU_NON_FUSA) && defined(CONFIG_NVGPU_NEXT)
	struct nvgpu_next_mc nvgpu_next;
#endif
	/** @endcond DOXYGEN_SHOULD_SKIP_THIS */
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

/**
 * @brief Clear the GPU device interrupts at master level.
 *
 * @param g [in]	The GPU driver struct.
 *
 * This function is invoked before powering on, powering off or finishing
 * SW quiesce of nvgpu driver.
 *
 * Steps:
 * - Acquire the spinlock g->mc.intr_lock.
 * - Write U32_MAX to the stalling interrupts enable clear register.
 *   mc_intr_en_clear_r are write only registers which clear
 *   the corresponding bit in INTR_EN whenever a 1 is written
 *   to it.
 * - Set g->mc.intr_mask_restore[NVGPU_MC_INTR_STALLING] and
 *   g->mc.intr_mask_restore[NVGPU_MC_INTR_NONSTALLING] to 0.
 * - Write U32_MAX to the non-stalling interrupts enable clear register.
 * - Release the spinlock g->mc.intr_lock.
 */
void nvgpu_mc_intr_mask(struct gk20a *g);

#ifdef CONFIG_NVGPU_NON_FUSA
void nvgpu_mc_log_pending_intrs(struct gk20a *g);
void nvgpu_mc_intr_enable(struct gk20a *g);
#endif

/**
 * @brief Enable the stalling interrupts for GPU unit at the master
 *        level.
 *
 * @param g [in]	The GPU driver struct.
 * @param unit [in]	Value designating the GPU HW unit/engine
 *                      controlled by MC. Supported values are:
 *			  - #MC_INTR_UNIT_BUS
 *			  - #MC_INTR_UNIT_PRIV_RING
 *			  - #MC_INTR_UNIT_FIFO
 *			  - #MC_INTR_UNIT_LTC
 *			  - #MC_INTR_UNIT_HUB
 *			  - #MC_INTR_UNIT_GR
 *			  - #MC_INTR_UNIT_PMU
 *			  - #MC_INTR_UNIT_CE
 *			  - #MC_INTR_UNIT_NVLINK
 *			  - #MC_INTR_UNIT_FBPA
 * @param enable [in]	Boolean control to enable/disable the stalling
 *			interrupt. Supported values are:
 *			  - #MC_INTR_ENABLE
 *			  - #MC_INTR_DISABLE
 *
 * This function is invoked during individual unit's init before
 * enabling that unit's interrupts.
 *
 * Steps:
 * - Acquire the spinlock g->mc.intr_lock.
 * - Get the interrupt bitmask for \a unit.
 * - If interrupt is to be enabled
 *   - Set interrupt bitmask in
 *     #intr_mask_restore[#NVGPU_MC_INTR_STALLING].
 *   - Write the interrupt bitmask to the register
 *     mc_intr_en_set_r(#NVGPU_MC_INTR_STALLING).
 * - Else
 *   - Clear interrupt bitmask in
 *     #intr_mask_restore[#NVGPU_MC_INTR_STALLING].
 *   - Write the interrupt bitmask to the register
 *     mc_intr_en_clear_r(#NVGPU_MC_INTR_STALLING).
 * - Release the spinlock g->mc.intr_lock.
 */
void nvgpu_mc_intr_stall_unit_config(struct gk20a *g, u32 unit, bool enable);

/**
 * @brief Enable the non-stalling interrupts for GPU unit at the master
 *        level.
 *
 * @param g [in]	The GPU driver struct.
 * @param unit [in]	Value designating the GPU HW unit/engine
 *                      controlled by MC. Supported values are:
 *			  - #MC_INTR_UNIT_BUS
 *			  - #MC_INTR_UNIT_PRIV_RING
 *			  - #MC_INTR_UNIT_FIFO
 *			  - #MC_INTR_UNIT_LTC
 *			  - #MC_INTR_UNIT_HUB
 *			  - #MC_INTR_UNIT_GR
 *			  - #MC_INTR_UNIT_PMU
 *			  - #MC_INTR_UNIT_CE
 *			  - #MC_INTR_UNIT_NVLINK
 *			  - #MC_INTR_UNIT_FBPA
 * @param enable [in]	Boolean control to enable/disable the stalling
 *			interrupt. Supported values are:
 *			  - #MC_INTR_ENABLE
 *			  - #MC_INTR_DISABLE
 *
 * This function is invoked during individual unit's init before
 * enabling that unit's interrupts.
 *
 * Steps:
 * - Acquire the spinlock g->mc.intr_lock.
 * - Get the interrupt bitmask for \a unit.
 * - If interrupt is to be enabled
 *   - Set interrupt bitmask in
 *     #intr_mask_restore[#NVGPU_MC_INTR_NONSTALLING].
 *   - Write the interrupt bitmask to the register
 *     mc_intr_en_set_r(#NVGPU_MC_INTR_NONSTALLING).
 * - Else
 *   - Clear interrupt bitmask in
 *     #intr_mask_restore[#NVGPU_MC_INTR_NONSTALLING].
 *   - Write the interrupt bitmask to the register
 *     mc_intr_en_clear_r(#NVGPU_MC_INTR_NONSTALLING).
 * - Release the spinlock g->mc.intr_lock.
 */
void nvgpu_mc_intr_nonstall_unit_config(struct gk20a *g, u32 unit, bool enable);

/**
 * @brief Disable/Pause the stalling interrupts.
 *
 * @param g [in]	The GPU driver struct.
 *
 * This function is invoked to disable the stalling interrupts before
 * the ISR is executed.
 *
 * Steps:
 * - Acquire the spinlock g->mc.intr_lock.
 * - Write U32_MAX to the stalling interrupts enable clear register
 *   (mc_intr_en_clear_r(#NVGPU_MC_INTR_STALLING)).
 * - Release the spinlock g->mc.intr_lock.
 */
void nvgpu_mc_intr_stall_pause(struct gk20a *g);

/**
 * @brief Enable/Resume the stalling interrupts.
 *
 * @param g [in]	The GPU driver struct.
 *
 * This function is invoked to enable the stalling interrupts after
 * the ISR is executed.
 *
 * Steps:
 * - Acquire the spinlock g->mc.intr_lock.
 * - Enable the stalling interrupts as configured during #intr_stall_unit_config
 *   Write #intr_mask_restore[#NVGPU_MC_INTR_STALLING] to the stalling
 *   interrupts enable set register (mc_intr_en_set_r(#NVGPU_MC_INTR_STALLING)).
 * - Release the spinlock g->mc.intr_lock.
 */
void nvgpu_mc_intr_stall_resume(struct gk20a *g);

/**
 * @brief Disable/Pause the non-stalling interrupts.
 *
 * @param g [in]	The GPU driver struct.
 *
 * This function is invoked to disable the non-stalling interrupts
 * before the ISR is executed.
 *
 * Steps:
 * - Acquire the spinlock g->mc.intr_lock.
 * - Write U32_MAX to the non-stalling interrupts enable clear register
 *   (mc_intr_en_clear_r(#NVGPU_MC_INTR_NONSTALLING)).
 * - Release the spinlock g->mc.intr_lock.
 */
void nvgpu_mc_intr_nonstall_pause(struct gk20a *g);

/**
 * @brief Enable/Resume the non-stalling interrupts.
 *
 * @param g [in]	The GPU driver struct.
 *
 * This function is invoked to enable the non-stalling interrupts after
 * the ISR is executed.
 *
 * Steps:
 * - Acquire the spinlock g->mc.intr_lock.
 * - Enable the stalling interrupts as configured during
 *   #intr_nonstall_unit_config.
 *   Write #intr_mask_restore[#NVGPU_MC_INTR_NONSTALLING]
 *   to the non-stalling interrupts enable set register
 *   (mc_intr_en_set_r(#NVGPU_MC_INTR_NONSTALLING)).
 * - Release the spinlock g->mc.intr_lock.
 */
void nvgpu_mc_intr_nonstall_resume(struct gk20a *g);

#endif
