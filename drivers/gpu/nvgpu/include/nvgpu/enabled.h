/*
 * Copyright (c) 2017-2020, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_ENABLED_H
#define NVGPU_ENABLED_H

struct gk20a;

#include <nvgpu/types.h>

/**
 * @defgroup enabled
 * @ingroup unit-common-utils
 * @{
 */

/*
 * Available flags that describe what's enabled and what's not in the GPU. Each
 * flag here is defined by it's offset in a bitmap.
 */

/** Running FMODEL Simulation. */
#define NVGPU_IS_FMODEL				1U
/** Driver is shutting down. */
#define NVGPU_DRIVER_IS_DYING			2U
/** Load Falcons using DMA because it's faster. */
#define NVGPU_GR_USE_DMA_FOR_FW_BOOTSTRAP	3U
/** Use VAs for FECS Trace buffer (instead of PAs) */
#define NVGPU_FECS_TRACE_VA			4U
/** Can gate the power rail */
#define NVGPU_CAN_RAILGATE			5U
/** The OS is shutting down */
#define NVGPU_KERNEL_IS_DYING			6U
/** Enable FECS Tracing  */
#define NVGPU_FECS_TRACE_FEATURE_CONTROL	7U

/*
 * ECC flags
 */
/** SM LRF ECC is enabled */
#define NVGPU_ECC_ENABLED_SM_LRF		8U
/** SM SHM ECC is enabled */
#define NVGPU_ECC_ENABLED_SM_SHM		9U
/** TEX ECC is enabled */
#define NVGPU_ECC_ENABLED_TEX			10U
/** L2 ECC is enabled */
#define NVGPU_ECC_ENABLED_LTC			11U
/** SM L1 DATA ECC is enabled */
#define NVGPU_ECC_ENABLED_SM_L1_DATA		12U
/** SM L1 TAG ECC is enabled */
#define NVGPU_ECC_ENABLED_SM_L1_TAG		13U
/** SM CBU ECC is enabled */
#define NVGPU_ECC_ENABLED_SM_CBU		14U
/** SM ICAHE ECC is enabled */
#define NVGPU_ECC_ENABLED_SM_ICACHE		15U

/*
 * MM flags.
 */
/** Unified Memory address space */
#define NVGPU_MM_UNIFY_ADDRESS_SPACES		16U
/** false if vidmem aperture actually points to sysmem */
#define NVGPU_MM_HONORS_APERTURE		17U
/** unified or split memory with separate vidmem? */
#define NVGPU_MM_UNIFIED_MEMORY			18U
/** User-space managed address spaces support */
#define NVGPU_SUPPORT_USERSPACE_MANAGED_AS	20U
/** IO coherence support is available */
#define NVGPU_SUPPORT_IO_COHERENCE		21U
/** MAP_BUFFER_EX with partial mappings */
#define NVGPU_SUPPORT_PARTIAL_MAPPINGS		22U
/** MAP_BUFFER_EX with sparse allocations */
#define NVGPU_SUPPORT_SPARSE_ALLOCS		23U
/** Direct PTE kind control is supported (map_buffer_ex) */
#define NVGPU_SUPPORT_MAP_DIRECT_KIND_CTRL	24U
/** Support batch mapping */
#define NVGPU_SUPPORT_MAP_BUFFER_BATCH		25U
/** Use coherent aperture for sysmem. */
#define NVGPU_USE_COHERENT_SYSMEM		26U
/** Use physical scatter tables instead of IOMMU */
#define NVGPU_MM_USE_PHYSICAL_SG		27U
/** WAR for gm20b chips. */
#define NVGPU_MM_FORCE_128K_PMU_VM		28U
/** Some chips (those that use nvlink) bypass the IOMMU on tegra. */
#define NVGPU_MM_BYPASSES_IOMMU			29U

/*
 * Host flags
 */
/** GPU has syncpoints */
#define NVGPU_HAS_SYNCPOINTS			30U
/** sync fence FDs are available in, e.g., submit_gpfifo */
#define NVGPU_SUPPORT_SYNC_FENCE_FDS		31U
/** NVGPU_DBG_GPU_IOCTL_CYCLE_STATS is available */
#define NVGPU_SUPPORT_CYCLE_STATS		32U
/** NVGPU_DBG_GPU_IOCTL_CYCLE_STATS_SNAPSHOT is available */
#define NVGPU_SUPPORT_CYCLE_STATS_SNAPSHOT	33U
/** Both gpu driver and device support TSG */
#define NVGPU_SUPPORT_TSG			34U
/** Fast deterministic submits with no job tracking are supported */
#define NVGPU_SUPPORT_DETERMINISTIC_SUBMIT_NO_JOBTRACKING 35U
/** Deterministic submits are supported even with job tracking */
#define NVGPU_SUPPORT_DETERMINISTIC_SUBMIT_FULL	36U
/** NVGPU_IOCTL_CHANNEL_RESCHEDULE_RUNLIST is available */
#define NVGPU_SUPPORT_RESCHEDULE_RUNLIST	37U

/** NVGPU_GPU_IOCTL_GET_EVENT_FD is available */
#define NVGPU_SUPPORT_DEVICE_EVENTS		38U
/** FECS context switch tracing is available */
#define NVGPU_SUPPORT_FECS_CTXSW_TRACE		39U

/** NVGPU_GPU_IOCTL_SET_DETERMINISTIC_OPTS is available */
#define NVGPU_SUPPORT_DETERMINISTIC_OPTS	40U

/*
 * Security flags
 */
/** secure gpccs boot support */
#define NVGPU_SEC_SECUREGPCCS			41U
/** Priv Sec enabled */
#define NVGPU_SEC_PRIVSECURITY			42U
/** VPR is supported */
#define NVGPU_SUPPORT_VPR			43U

/*
 * Nvlink flags
 */

/** Nvlink enabled */
#define NVGPU_SUPPORT_NVLINK			45U
/*
 * PMU flags.
 */
/** perfmon enabled or disabled for PMU */
#define NVGPU_PMU_PERFMON			48U
/** PMU Pstates */
#define NVGPU_PMU_PSTATE			49U
/** Save ZBC reglist */
#define NVGPU_PMU_ZBC_SAVE			50U
/** Completed booting FECS */
#define NVGPU_PMU_FECS_BOOTSTRAP_DONE		51U
/** Supports Block Level Clock Gating */
#define NVGPU_GPU_CAN_BLCG			52U
/** Supports Second Level Clock Gating */
#define NVGPU_GPU_CAN_SLCG			53U
/** Supports Engine Level Clock Gating */
#define NVGPU_GPU_CAN_ELCG			54U
/** Clock control support */
#define NVGPU_SUPPORT_CLOCK_CONTROLS		55U
/** NVGPU_GPU_IOCTL_GET_VOLTAGE is available */
#define NVGPU_SUPPORT_GET_VOLTAGE		56U
/** NVGPU_GPU_IOCTL_GET_CURRENT is available */
#define NVGPU_SUPPORT_GET_CURRENT		57U
/** NVGPU_GPU_IOCTL_GET_POWER is available */
#define NVGPU_SUPPORT_GET_POWER			58U
/** NVGPU_GPU_IOCTL_GET_TEMPERATURE is available */
#define NVGPU_SUPPORT_GET_TEMPERATURE		59U
/** NVGPU_GPU_IOCTL_SET_THERM_ALERT_LIMIT is available */
#define NVGPU_SUPPORT_SET_THERM_ALERT_LIMIT	60U

/** whether to run PREOS binary on dGPUs */
#define NVGPU_PMU_RUN_PREOS			61U

/** set if ASPM is enabled; only makes sense for PCI */
#define NVGPU_SUPPORT_ASPM			62U
/** subcontexts are available */
#define NVGPU_SUPPORT_TSG_SUBCONTEXTS		63U
/** Simultaneous Compute and Graphics (SCG) is available */
#define NVGPU_SUPPORT_SCG			64U

/** GPU_VA address of a syncpoint is supported */
#define NVGPU_SUPPORT_SYNCPOINT_ADDRESS		65U
/** Allocating per-channel syncpoint in user space is supported */
#define NVGPU_SUPPORT_USER_SYNCPOINT		66U

/** USERMODE enable bit */
#define NVGPU_SUPPORT_USERMODE_SUBMIT		67U

/** Multiple WPR support */
#define NVGPU_SUPPORT_MULTIPLE_WPR		68U

/** SEC2 RTOS support*/
#define NVGPU_SUPPORT_SEC2_RTOS			69U

/** PMU RTOS FBQ support*/
#define NVGPU_SUPPORT_PMU_RTOS_FBQ		70U

/** ZBC STENCIL support*/
#define NVGPU_SUPPORT_ZBC_STENCIL		71U

/** PLATFORM_ATOMIC support */
#define NVGPU_SUPPORT_PLATFORM_ATOMIC		72U

/** SEC2 VM support */
#define NVGPU_SUPPORT_SEC2_VM			73U

/** GSP VM support */
#define NVGPU_SUPPORT_GSP_VM			74U

/** GFXP preemption support */
#define NVGPU_SUPPORT_PREEMPTION_GFXP		75U

/** PMU Super surface */
#define NVGPU_SUPPORT_PMU_SUPER_SURFACE		76U

/** Reduced profile of nvgpu driver */
#define NVGPU_DRIVER_REDUCED_PROFILE		77U

/** NVGPU_GPU_IOCTL_SET_MMU_DEBUG_MODE is available */
#define NVGPU_SUPPORT_SET_CTX_MMU_DEBUG_MODE	78U

/** DGPU Thermal Alert */
#define NVGPU_SUPPORT_DGPU_THERMAL_ALERT	79U

/** Fault recovery support */
#define NVGPU_SUPPORT_FAULT_RECOVERY		80U

/** SW Quiesce */
#define NVGPU_DISABLE_SW_QUIESCE		81U

/** DGPU PCIe Script Update */
#define NVGPU_SUPPORT_DGPU_PCIE_SCRIPT_EXECUTE	82U

/** FMON feature Enable */
#define NVGPU_FMON_SUPPORT_ENABLE		83U

/** Copy Engine diversity enable bit */
#define NVGPU_SUPPORT_COPY_ENGINE_DIVERSITY	84U

/** SM diversity enable bit */
#define NVGPU_SUPPORT_SM_DIVERSITY		85U

/** SM RAMS ECC is enabled */
#define NVGPU_ECC_ENABLED_SM_RAMS		86U

/** Enable compression */
#define NVGPU_SUPPORT_COMPRESSION		87U

/** SM TTU is enabled */
#define NVGPU_SUPPORT_SM_TTU			88U

/** PLC Compression */
#define NVGPU_SUPPORT_PLC			89U

/*
 * Must be greater than the largest bit offset in the above list.
 */
#define NVGPU_MAX_ENABLED_BITS			90U

/**
 * @brief Check if the passed flag is enabled.
 *
 * @param g [in]	The GPU.
 * @param flag [in]	Which flag to check.
 *
 * @retrun true if the passed \a flag is true; false otherwise.
 */
bool nvgpu_is_enabled(struct gk20a *g, u32 flag);

/**
 * @brief Set the state of a flag.
 *
 * @param g [in]	The GPU.
 * @param flag [in]	Which flag to modify.
 * @param state [in]	The state to set the \a flag to.
 *
 * Set the state of the passed \a flag to \a state.
 *
 * This is generally a somewhat low level operation with lots of potential
 * side effects. Be weary about where and when you use this. Typically a bunch
 * of calls to this early in the driver boot sequence makes sense (as
 * information is determined about the GPU at run time). Calling this in steady
 * state operation is probably an incorrect thing to do.
 */
void nvgpu_set_enabled(struct gk20a *g, u32 flag, bool state);

/**
 * @brief Allocate the memory for the enabled flags.
 *
 * @param g [in]	The GPU superstructure.
 *
 * @return 0 for success, < 0 for error.
 * @retval -ENOMEM if fails to allocate the necessary memory.
 */
int nvgpu_init_enabled_flags(struct gk20a *g);

/**
 * @brief Free the memory for the enabled flags. Called during driver exit.
 *
 * @param g [in]	The GPU superstructure.
 */
void nvgpu_free_enabled_flags(struct gk20a *g);

/**
 * @}
 */
#endif /* NVGPU_ENABLED_H */
