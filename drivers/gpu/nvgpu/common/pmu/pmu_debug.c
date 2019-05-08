/*
 * Copyright (c) 2016-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/pmu.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/pmu/fw.h>
#include <nvgpu/dma.h>
#include <nvgpu/pmu/debug.h>

bool nvgpu_find_hex_in_string(char *strings, struct gk20a *g, u32 *hex_pos)
{
	u32 i = 0, j = (u32)strlen(strings);

	for (; i < j; i++) {
		if (strings[i] == '%') {
			if (strings[i + 1U] == 'x' || strings[i + 1U] == 'X') {
				*hex_pos = i;
				return true;
			}
		}
	}
	*hex_pos = U32_MAX;
	return false;
}

static void print_pmu_trace(struct nvgpu_pmu *pmu)
{
	struct gk20a *g = pmu->g;
	u32 i = 0, j = 0, k, l, m;
	int count;
	char part_str[40], buf[0x40];
	void *tracebuffer;
	char *trace;
	u32 *trace1;

	/* allocate system memory to copy pmu trace buffer */
	tracebuffer = nvgpu_kzalloc(g, GK20A_PMU_TRACE_BUFSIZE);
	if (tracebuffer == NULL) {
		return;
	}

	/* read pmu traces into system memory buffer */
	nvgpu_mem_rd_n(g, &pmu->trace_buf, 0, tracebuffer,
		GK20A_PMU_TRACE_BUFSIZE);

	trace = (char *)tracebuffer;
	trace1 = (u32 *)tracebuffer;

	nvgpu_err(g, "dump PMU trace buffer");
	for (i = 0U; i < GK20A_PMU_TRACE_BUFSIZE; i += 0x40U) {
		for (j = 0U; j < 0x40U; j++) {
			if (trace1[(i / 4U) + j] != 0U) {
				break;
			}
		}
		if (j == 0x40U) {
			break;
		}
		count = scnprintf(buf, 0x40, "Index %x: ", trace1[(i / 4U)]);
		l = 0;
		m = 0;
		while (nvgpu_find_hex_in_string((trace+i+20+m), g, &k)) {
			if (k >= 40U) {
				break;
			}
			(void) strncpy(part_str, (trace+i+20+m), k);
			part_str[k] = '\0';
			count += scnprintf((buf + count), 0x40, "%s0x%x",
					part_str, trace1[(i / 4U) + 1U + l]);
			l++;
			m += k + 2U;
		}

		(void) scnprintf((buf + count), 0x40, "%s", (trace+i+20+m));
		nvgpu_err(g, "%s", buf);
	}

	nvgpu_kfree(g, tracebuffer);
}

void nvgpu_pmu_dump_falcon_stats(struct nvgpu_pmu *pmu)
{
	struct gk20a *g = pmu->g;

	nvgpu_falcon_dump_stats(&pmu->flcn);
	g->ops.pmu.pmu_dump_falcon_stats(pmu);

	/* Print PMU F/W debug prints */
	print_pmu_trace(pmu);

	nvgpu_err(g, "pmu state: %d", nvgpu_pmu_get_fw_state(g, pmu));
	nvgpu_err(g, "elpg state: %d", pmu->pg->elpg_stat);

	/* PMU may crash due to FECS crash. Dump FECS status */
	g->ops.gr.falcon.dump_stats(g);
}

int nvgpu_pmu_debug_init(struct gk20a *g, struct nvgpu_pmu *pmu)
{
	struct mm_gk20a *mm = &g->mm;
	struct vm_gk20a *vm = mm->pmu.vm;
	int err = 0;

	err = nvgpu_dma_alloc_map(vm, GK20A_PMU_TRACE_BUFSIZE,
			&pmu->trace_buf);
	if (err != 0) {
		nvgpu_err(g, "failed to allocate pmu trace buffer\n");
	}

	return err;
}

void nvgpu_pmu_debug_deinit(struct gk20a *g, struct nvgpu_pmu *pmu)
{
	struct mm_gk20a *mm = &g->mm;
	struct vm_gk20a *vm = mm->pmu.vm;

	if (nvgpu_mem_is_valid(&pmu->trace_buf)) {
		nvgpu_dma_unmap_free(vm, &pmu->trace_buf);
	}
}
