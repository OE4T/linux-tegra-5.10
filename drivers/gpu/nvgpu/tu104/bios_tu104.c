/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/types.h>
#include <nvgpu/timers.h>
#include <nvgpu/io.h>
#include <nvgpu/gk20a.h>

#include "tu104/bios_tu104.h"

#include "nvgpu/hw/tu104/hw_gc6_tu104.h"

#define NV_DEVINIT_VERIFY_TIMEOUT_MS		1000
#define NV_DEVINIT_VERIFY_TIMEOUT_DELAY_US	10

#define NV_PGC6_AON_SECURE_SCRATCH_GROUP_05_0_GFW_BOOT_PROGRESS_MASK \
		0xFF
#define NV_PGC6_AON_SECURE_SCRATCH_GROUP_05_0_GFW_BOOT_PROGRESS_COMPLETED \
		0xFF

int tu104_bios_verify_devinit(struct gk20a *g)
{
	struct nvgpu_timeout timeout;
	u32 val;
	int err;

	err = nvgpu_timeout_init(g, &timeout, NV_DEVINIT_VERIFY_TIMEOUT_MS,
				   NVGPU_TIMER_CPU_TIMER);
	if (err != 0) {
		return err;
	}

	do {
		val = nvgpu_readl(g, gc6_aon_secure_scratch_group_05_r(0));
		val &= NV_PGC6_AON_SECURE_SCRATCH_GROUP_05_0_GFW_BOOT_PROGRESS_MASK;

		if (val == NV_PGC6_AON_SECURE_SCRATCH_GROUP_05_0_GFW_BOOT_PROGRESS_COMPLETED) {
			nvgpu_log_info(g, "devinit complete");
			return 0;
		}

		nvgpu_udelay(NV_DEVINIT_VERIFY_TIMEOUT_DELAY_US);
	} while (nvgpu_timeout_expired(&timeout) == 0);

	return -ETIMEDOUT;
}
