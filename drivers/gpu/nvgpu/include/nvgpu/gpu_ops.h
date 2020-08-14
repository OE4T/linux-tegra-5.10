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
#ifndef NVGPU_GOPS_OPS_H
#define NVGPU_GOPS_OPS_H

#include <nvgpu/types.h>
#include <nvgpu/gops/acr.h>
#include <nvgpu/gops/bios.h>
#include <nvgpu/gops/cbc.h>
#include <nvgpu/gops/clk_arb.h>
#include <nvgpu/gops/debugger.h>
#include <nvgpu/gops/profiler.h>
#include <nvgpu/gops/cyclestats.h>
#include <nvgpu/gops/fbp.h>
#include <nvgpu/gops/floorsweep.h>
#include <nvgpu/gops/sbr.h>
#include <nvgpu/gops/func.h>
#include <nvgpu/gops/nvdec.h>
#include <nvgpu/gops/pramin.h>
#include <nvgpu/gops/clk.h>
#include <nvgpu/gops/xve.h>
#include <nvgpu/gops/nvlink.h>
#include <nvgpu/gops/sec2.h>
#include <nvgpu/gops/gsp.h>
#include <nvgpu/gops/class.h>
#include <nvgpu/gops/ce.h>
#include <nvgpu/gops/ptimer.h>
#include <nvgpu/gops/top.h>
#include <nvgpu/gops/bus.h>
#include <nvgpu/gops/gr.h>
#include <nvgpu/gops/falcon.h>
#include <nvgpu/gops/fifo.h>
#include <nvgpu/gops/fuse.h>
#include <nvgpu/gops/ltc.h>
#include <nvgpu/gops/ramfc.h>
#include <nvgpu/gops/ramin.h>
#include <nvgpu/gops/runlist.h>
#include <nvgpu/gops/userd.h>
#include <nvgpu/gops/engine.h>
#include <nvgpu/gops/pbdma.h>
#include <nvgpu/gops/sync.h>
#include <nvgpu/gops/channel.h>
#include <nvgpu/gops/tsg.h>
#include <nvgpu/gops/usermode.h>
#include <nvgpu/gops/mm.h>
#include <nvgpu/gops/netlist.h>
#include <nvgpu/gops/priv_ring.h>
#include <nvgpu/gops/therm.h>
#include <nvgpu/gops/fb.h>
#include <nvgpu/gops/mc.h>
#include <nvgpu/gops/cg.h>
#include <nvgpu/gops/pmu.h>
#include <nvgpu/gops/ecc.h>
#include <nvgpu/gops/grmgr.h>

struct gk20a;
struct nvgpu_debug_context;
struct nvgpu_mem;

struct gops_debug {
	void (*show_dump)(struct gk20a *g,
			struct nvgpu_debug_context *o);
};

/**
 * @addtogroup unit-common-nvgpu
 * @{
 */

/**
 * @brief HAL methods
 *
 * gpu_ops contains function pointers for the unit HAL interfaces. gpu_ops
 * should only contain function pointers! Non-function pointer members should go
 * in struct gk20a or be implemented with the boolean flag API defined in
 * nvgpu/enabled.h. Each unit should have its own sub-struct in the gpu_ops
 * struct.
 */
struct gpu_ops {

	struct gops_acr acr;
	struct gops_sbr sbr;
	struct gops_func func;
	struct gops_ecc ecc;
	struct gops_ltc ltc;
#ifdef CONFIG_NVGPU_COMPRESSION
	struct gops_cbc cbc;
#endif
	struct gops_ce ce;
	struct gops_gr gr;
	struct gops_class gpu_class;
	struct gops_fb fb;
	struct gops_nvdec nvdec;
	struct gops_cg cg;
	struct gops_fifo fifo;
	struct gops_fuse fuse;
	struct gops_ramfc ramfc;
	struct gops_ramin ramin;
	struct gops_runlist runlist;
	struct gops_userd userd;
	struct gops_engine engine;
	struct gops_pbdma pbdma;
	struct gops_sync sync;
	struct gops_channel channel;
	struct gops_tsg tsg;
	struct gops_usermode usermode;
	struct gops_engine_status engine_status;
	struct gops_pbdma_status pbdma_status;
	struct gops_netlist netlist;
	struct gops_mm mm;
	/*
	 * This function is called to allocate secure memory (memory
	 * that the CPU cannot see). The function should fill the
	 * context buffer descriptor (especially fields destroy, sgt,
	 * size).
	 */
	int (*secure_alloc)(struct gk20a *g, struct nvgpu_mem *desc_mem,
			size_t size,
			void (**fn)(struct gk20a *g, struct nvgpu_mem *mem));

#ifdef CONFIG_NVGPU_DGPU
	struct gops_pramin pramin;
#endif
	struct gops_therm therm;
	struct gops_pmu pmu;
	struct gops_clk clk;
#ifdef CONFIG_NVGPU_DGPU
	struct gops_clk_mon clk_mon;
#endif
#ifdef CONFIG_NVGPU_CLK_ARB
	struct gops_clk_arb clk_arb;
#endif
	struct gops_pmu_perf pmu_perf;
#ifdef CONFIG_NVGPU_DEBUGGER
	struct gops_regops regops;
#endif
	struct gops_mc mc;
	struct gops_debug debug;
#ifdef CONFIG_NVGPU_DEBUGGER
	struct gops_debugger debugger;
	struct gops_perf perf;
	struct gops_perfbuf perfbuf;
#endif
#ifdef CONFIG_NVGPU_PROFILER
	struct gops_pm_reservation pm_reservation;
#endif

	u32 (*get_litter_value)(struct gk20a *g, int value);
	int (*chip_init_gpu_characteristics)(struct gk20a *g);

	struct gops_bus bus;
	struct gops_ptimer ptimer;
	struct gops_bios bios;
#ifdef CONFIG_NVGPU_CYCLESTATS
	struct gops_css css;
#endif
#ifdef CONFIG_NVGPU_DGPU
	struct gops_xve xve;
#endif
	struct gops_falcon falcon;
	struct gops_fbp fbp;
	struct gops_priv_ring priv_ring;
	struct gops_nvlink nvlink;
	struct gops_top top;
	struct gops_sec2 sec2;
	struct gops_gsp gsp;
#ifdef CONFIG_NVGPU_TPC_POWERGATE
	struct gops_tpc tpc;
#endif
	void (*semaphore_wakeup)(struct gk20a *g, bool post_events);

	struct gops_grmgr grmgr;

};

#endif /* NVGPU_GOPS_OPS_H */