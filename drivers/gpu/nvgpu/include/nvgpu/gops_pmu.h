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

#ifndef NVGPU_GOPS_PMU_H
#define NVGPU_GOPS_PMU_H

#include <nvgpu/types.h>

struct gk20a;
struct nvgpu_pmu;
struct nvgpu_hw_err_inject_info_desc;

/**
 * PMU unit & engine HAL operations.
 *
 * This structure stores the PMU unit & engine HAL function pointers.
 *
 * @see gpu_ops
 */
struct gops_pmu {
	int (*pmu_early_init)(struct gk20a *g);
	int (*pmu_rtos_init)(struct gk20a *g);
	int (*pmu_destroy)(struct gk20a *g, struct nvgpu_pmu *pmu);
	int (*pmu_pstate_sw_setup)(struct gk20a *g);
	int (*pmu_pstate_pmu_setup)(struct gk20a *g);
	struct nvgpu_hw_err_inject_info_desc * (*get_pmu_err_desc)
		(struct gk20a *g);
	bool (*is_pmu_supported)(struct gk20a *g);
	u32 (*falcon_base_addr)(void);
	/* reset */
	int (*pmu_reset)(struct gk20a *g);
	void (*reset_engine)(struct gk20a *g, bool do_reset);
	bool (*is_engine_in_reset)(struct gk20a *g);
	/* secure boot */
	void (*setup_apertures)(struct gk20a *g);
	void (*write_dmatrfbase)(struct gk20a *g, u32 addr);
	bool (*is_debug_mode_enabled)(struct gk20a *g);
	void (*secured_pmu_start)(struct gk20a *g);
	void (*flcn_setup_boot_config)(struct gk20a *g);
	bool (*validate_mem_integrity)(struct gk20a *g);
#ifdef CONFIG_NVGPU_LS_PMU
	/* ISR */
	void (*pmu_enable_irq)(struct nvgpu_pmu *pmu, bool enable);
	bool (*pmu_is_interrupted)(struct nvgpu_pmu *pmu);
	void (*pmu_isr)(struct gk20a *g);
	void (*set_irqmask)(struct gk20a *g);
	u32 (*get_irqdest)(struct gk20a *g);
	void (*handle_ext_irq)(struct gk20a *g, u32 intr);
	/* non-secure */
	int (*pmu_ns_bootstrap)(struct gk20a *g, struct nvgpu_pmu *pmu,
		u32 args_offset);
	/* queue */
	u32 (*pmu_get_queue_head)(u32 i);
	u32 (*pmu_get_queue_head_size)(void);
	u32 (*pmu_get_queue_tail_size)(void);
	u32 (*pmu_get_queue_tail)(u32 i);
	int (*pmu_queue_head)(struct gk20a *g, u32 queue_id,
		u32 queue_index, u32 *head, bool set);
	int (*pmu_queue_tail)(struct gk20a *g, u32 queue_id,
		u32 queue_index, u32 *tail, bool set);
	void (*pmu_msgq_tail)(struct nvgpu_pmu *pmu,
		u32 *tail, bool set);
	/* mutex */
	u32 (*pmu_mutex_size)(void);
	u32 (*pmu_mutex_owner)(struct gk20a *g,
		struct pmu_mutexes *mutexes, u32 id);
	int (*pmu_mutex_acquire)(struct gk20a *g,
		struct pmu_mutexes *mutexes, u32 id,
		u32 *token);
	void (*pmu_mutex_release)(struct gk20a *g,
		struct pmu_mutexes *mutexes, u32 id,
		u32 *token);
	/* perfmon */
	void (*pmu_init_perfmon_counter)(struct gk20a *g);
	void (*pmu_pg_idle_counter_config)(struct gk20a *g, u32 pg_engine_id);
	u32  (*pmu_read_idle_counter)(struct gk20a *g, u32 counter_id);
	u32  (*pmu_read_idle_intr_status)(struct gk20a *g);
	void (*pmu_clear_idle_intr_status)(struct gk20a *g);
	void (*pmu_reset_idle_counter)(struct gk20a *g, u32 counter_id);
	/* PG */
	void (*pmu_setup_elpg)(struct gk20a *g);
	/* debug */
	void (*pmu_dump_elpg_stats)(struct nvgpu_pmu *pmu);
	void (*pmu_dump_falcon_stats)(struct nvgpu_pmu *pmu);
	void (*dump_secure_fuses)(struct gk20a *g);
#endif
	void (*pmu_clear_bar0_host_err_status)(struct gk20a *g);
	int (*bar0_error_status)(struct gk20a *g, u32 *bar0_status,
		u32 *etype);
};

#endif
