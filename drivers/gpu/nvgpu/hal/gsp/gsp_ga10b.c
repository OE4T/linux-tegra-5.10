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

#include <nvgpu/falcon.h>
#include <nvgpu/mm.h>
#include <nvgpu/io.h>
#include <nvgpu/timers.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/bug.h>
#ifdef CONFIG_NVGPU_GSP_SCHEDULER
#include <nvgpu/gsp.h>
#endif

#include "gsp_ga10b.h"

#include <nvgpu/hw/ga10b/hw_pgsp_ga10b.h>

u32 ga10b_gsp_falcon2_base_addr(void)
{
	return pgsp_falcon2_gsp_base_r();
}

u32 ga10b_gsp_falcon_base_addr(void)
{
	return pgsp_falcon_irqsset_r();
}

int ga10b_gsp_engine_reset(struct gk20a *g)
{
	gk20a_writel(g, pgsp_falcon_engine_r(),
		pgsp_falcon_engine_reset_true_f());
	nvgpu_udelay(10);
	gk20a_writel(g, pgsp_falcon_engine_r(),
		pgsp_falcon_engine_reset_false_f());

	return 0;
}

#ifdef CONFIG_NVGPU_GSP_SCHEDULER
u32 ga10b_gsp_queue_head_r(u32 i)
{
	return pgsp_queue_head_r(i);
}

u32 ga10b_gsp_queue_head__size_1_v(void)
{
	return pgsp_queue_head__size_1_v();
}

u32 ga10b_gsp_queue_tail_r(u32 i)
{
	return pgsp_queue_tail_r(i);
}

u32 ga10b_gsp_queue_tail__size_1_v(void)
{
	return pgsp_queue_tail__size_1_v();
}

static u32 ga10b_gsp_get_irqmask(struct gk20a *g)
{
	return (gk20a_readl(g, pgsp_riscv_irqmask_r()) &
			gk20a_readl(g, pgsp_riscv_irqdest_r()));
}

static bool ga10b_gsp_is_interrupted(struct gk20a *g, u32 *intr)
{
	u32 supported_gsp_int = 0U;
	u32 intr_stat = gk20a_readl(g, pgsp_falcon_irqstat_r());

	supported_gsp_int = pgsp_falcon_irqstat_halt_true_f() |
			pgsp_falcon_irqstat_swgen1_true_f();

	if ((intr_stat & supported_gsp_int) != 0U) {
		*intr = intr_stat;
		return true;
	}

	return false;
}

static void ga10b_gsp_handle_swgen1_irq(struct gk20a *g)
{
	int err = 0;
	struct nvgpu_falcon *flcn = NULL;

	nvgpu_log_fn(g, " ");

	flcn = nvgpu_gsp_falcon_instance(g);
	if (flcn == NULL) {
		nvgpu_err(g, "GSP falcon instance not found");
		return;
	}

#ifdef CONFIG_NVGPU_FALCON_DEBUG
	err = nvgpu_falcon_dbg_buf_display(flcn);
	if (err != 0) {
		nvgpu_err(g, "nvgpu_falcon_debug_buffer_display failed err=%d",
		err);
	}
#endif
}

static void ga10b_gsp_clr_intr(struct gk20a *g, u32 intr)
{
	gk20a_writel(g, pgsp_riscv_irqmclr_r(), intr);
}

void ga10b_gsp_handle_interrupts(struct gk20a *g, u32 intr)
{
	nvgpu_log_fn(g, " ");

	/* swgen1 interrupt handle */
	if ((intr & pgsp_falcon_irqstat_swgen1_true_f()) != 0U) {
		ga10b_gsp_handle_swgen1_irq(g);
	}

	/* halt interrupt handle */
	if ((intr & pgsp_falcon_irqstat_halt_true_f()) != 0U) {
		nvgpu_err(g, "gsp halt intr not implemented");
	}
}

void ga10b_gsp_isr(struct gk20a *g)
{
	u32 intr = 0U;
	u32 mask = 0U;

	nvgpu_log_fn(g, " ");

	if (!ga10b_gsp_is_interrupted(g, &intr)) {
		nvgpu_err(g, "GSP interrupt not supported stat:0x%08x", intr);
		return;
	}

	nvgpu_gsp_isr_mutex_aquire(g);
	if (!nvgpu_gsp_is_isr_enable(g)) {
		goto exit;
	}

	mask = ga10b_gsp_get_irqmask(g);
	nvgpu_log_info(g, "received gsp interrupt: stat:0x%08x mask:0x%08x",
			intr, mask);

	if ((intr & mask) == 0U) {
		nvgpu_log_info(g,
			"clearing unhandled interrupt: stat:0x%08x mask:0x%08x",
			intr, mask);
		nvgpu_writel(g, pgsp_riscv_irqmclr_r(), intr);
		goto exit;
	}

	intr = intr & mask;
	ga10b_gsp_clr_intr(g, intr);

	ga10b_gsp_handle_interrupts(g, intr);

exit:
	nvgpu_gsp_isr_mutex_release(g);
}

static void ga10b_riscv_set_irq(struct gk20a *g, bool set_irq,
		u32 intr_mask, u32 intr_dest)
{
	if (set_irq) {
		gk20a_writel(g, pgsp_riscv_irqmset_r(), intr_mask);
		gk20a_writel(g, pgsp_riscv_irqdest_r(), intr_dest);
	} else {
		gk20a_writel(g, pgsp_riscv_irqmclr_r(), 0xffffffffU);
	}
}

void ga10b_gsp_enable_irq(struct gk20a *g, bool enable)
{
	u32 intr_mask;
	u32 intr_dest;

	nvgpu_log_fn(g, " ");

	/* clear before setting required irq */
	ga10b_riscv_set_irq(g, false, 0x0, 0x0);

	nvgpu_cic_mon_intr_stall_unit_config(g,
			NVGPU_CIC_INTR_UNIT_GSP, NVGPU_CIC_INTR_DISABLE);

	if (enable) {
		/* dest 0=falcon, 1=host; level 0=irq0, 1=irq1 */
		intr_dest = pgsp_riscv_irqdest_gptmr_f(0)    |
			pgsp_riscv_irqdest_wdtmr_f(1)    |
			pgsp_riscv_irqdest_mthd_f(0)     |
			pgsp_riscv_irqdest_ctxsw_f(0)    |
			pgsp_riscv_irqdest_halt_f(1)     |
			pgsp_riscv_irqdest_exterr_f(0)   |
			pgsp_riscv_irqdest_swgen0_f(1)   |
			pgsp_riscv_irqdest_swgen1_f(1)   |
			pgsp_riscv_irqdest_ext_f(0xff);

		/* 0=disable, 1=enable */
		intr_mask = pgsp_riscv_irqmset_gptmr_f(1)  |
			pgsp_riscv_irqmset_wdtmr_f(1)  |
			pgsp_riscv_irqmset_mthd_f(0)   |
			pgsp_riscv_irqmset_ctxsw_f(0)  |
			pgsp_riscv_irqmset_halt_f(1)   |
			pgsp_riscv_irqmset_exterr_f(1) |
			pgsp_riscv_irqmset_swgen0_f(1) |
			pgsp_riscv_irqmset_swgen1_f(1);

		/* set required irq */
		ga10b_riscv_set_irq(g, true, intr_mask, intr_dest);

		nvgpu_cic_mon_intr_stall_unit_config(g,
				NVGPU_CIC_INTR_UNIT_GSP, NVGPU_CIC_INTR_ENABLE);
	}
}
#endif /* CONFIG_NVGPU_GSP_SCHEDULER */
