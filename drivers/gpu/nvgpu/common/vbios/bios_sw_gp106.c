/*
 * Copyright (c) 2015-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/bios.h>
#include <nvgpu/kmem.h>
#include <nvgpu/nvgpu_common.h>
#include <nvgpu/timers.h>
#include <nvgpu/falcon.h>
#include <nvgpu/enabled.h>
#include <nvgpu/io.h>
#include <nvgpu/gk20a.h>

#include "hal/top/top_gp106.h"

#include "bios_sw_gp106.h"

#define PMU_BOOT_TIMEOUT_DEFAULT	100 /* usec */
#define PMU_BOOT_TIMEOUT_MAX		2000000 /* usec */
#define BIOS_OVERLAY_NAME "bios-%04x.rom"
#define BIOS_OVERLAY_NAME_FORMATTED "bios-xxxx.rom"
#define ROM_FILE_PAYLOAD_OFFSET 0xa00
#define BIOS_SIZE 0x90000

int gp106_bios_devinit(struct gk20a *g)
{
	int err = 0;
	bool devinit_completed;
	struct nvgpu_timeout timeout;
	u32 top_scratch1_reg;

	nvgpu_log_fn(g, " ");

	if (nvgpu_falcon_reset(&g->pmu.flcn) != 0) {
		err = -ETIMEDOUT;
		goto out;
	}

	err = nvgpu_falcon_copy_to_imem(&g->pmu.flcn,
			g->bios.devinit.bootloader_phys_base,
			g->bios.devinit.bootloader,
			g->bios.devinit.bootloader_size,
			0, 0, g->bios.devinit.bootloader_phys_base >> 8);
	if (err != 0) {
		nvgpu_err(g, "bios devinit bootloader copy failed %d", err);
		goto out;
	}

	err = nvgpu_falcon_copy_to_imem(&g->pmu.flcn, g->bios.devinit.phys_base,
			g->bios.devinit.ucode,
			g->bios.devinit.size,
			0, 1, g->bios.devinit.phys_base >> 8);
	if (err != 0) {
		nvgpu_err(g, "bios devinit ucode copy failed %d", err);
		goto out;
	}

	err = nvgpu_falcon_copy_to_dmem(&g->pmu.flcn, g->bios.devinit.dmem_phys_base,
			g->bios.devinit.dmem,
			g->bios.devinit.dmem_size,
			0);
	if (err != 0) {
		nvgpu_err(g, "bios devinit dmem copy failed %d", err);
		goto out;
	}

	err = nvgpu_falcon_copy_to_dmem(&g->pmu.flcn, g->bios.devinit_tables_phys_base,
			g->bios.devinit_tables,
			g->bios.devinit_tables_size,
			0);
	if (err != 0) {
		nvgpu_err(g, "fbios devinit tables copy failed %d", err);
		goto out;
	}

	err = nvgpu_falcon_copy_to_dmem(&g->pmu.flcn, g->bios.devinit_script_phys_base,
			g->bios.bootscripts,
			g->bios.bootscripts_size,
			0);
	if (err != 0) {
		nvgpu_err(g, "bios devinit bootscripts copy failed %d", err);
		goto out;
	}

	err = nvgpu_falcon_bootstrap(&g->pmu.flcn,
					g->bios.devinit.code_entry_point);
	if (err != 0) {
		nvgpu_err(g, "falcon bootstrap failed %d", err);
		goto out;
	}

	nvgpu_timeout_init(g, &timeout,
			   PMU_BOOT_TIMEOUT_MAX /
				PMU_BOOT_TIMEOUT_DEFAULT,
			   NVGPU_TIMER_RETRY_TIMER);
	do {
		top_scratch1_reg = g->ops.top.read_top_scratch1_reg(g);
		devinit_completed = ((g->ops.falcon.is_falcon_cpu_halted(
				&g->pmu.flcn) != 0U) &&
				(g->ops.top.top_scratch1_devinit_completed(g,
				top_scratch1_reg)) != 0U);

		nvgpu_udelay(PMU_BOOT_TIMEOUT_DEFAULT);
	} while (!devinit_completed && (nvgpu_timeout_expired(&timeout) == 0));

	if (nvgpu_timeout_peek_expired(&timeout) != 0) {
		err = -ETIMEDOUT;
		goto out;
	}

	err = nvgpu_falcon_clear_halt_intr_status(&g->pmu.flcn,
		nvgpu_get_poll_timeout(g));
	if (err != 0) {
		nvgpu_err(g, "falcon_clear_halt_intr_status failed %d", err);
		goto out;
	}

out:
	nvgpu_log_fn(g, "done");
	return err;
}

int gp106_bios_preos_wait_for_halt(struct gk20a *g)
{
	return nvgpu_falcon_wait_for_halt(&g->pmu.flcn,
					PMU_BOOT_TIMEOUT_MAX / 1000);
}

int gp106_bios_preos(struct gk20a *g)
{
	int err = 0;

	nvgpu_log_fn(g, " ");

	if (nvgpu_falcon_reset(&g->pmu.flcn) != 0) {
		err = -ETIMEDOUT;
		goto out;
	}

	if (g->ops.bios.preos_reload_check != NULL) {
		g->ops.bios.preos_reload_check(g);
	}

	err = nvgpu_falcon_copy_to_imem(&g->pmu.flcn,
			g->bios.preos.bootloader_phys_base,
			g->bios.preos.bootloader,
			g->bios.preos.bootloader_size,
			0, 0, g->bios.preos.bootloader_phys_base >> 8);
	if (err != 0) {
		nvgpu_err(g, "bios preos bootloader copy failed %d", err);
		goto out;
	}

	err = nvgpu_falcon_copy_to_imem(&g->pmu.flcn, g->bios.preos.phys_base,
			g->bios.preos.ucode,
			g->bios.preos.size,
			0, 1, g->bios.preos.phys_base >> 8);
	if (err != 0) {
		nvgpu_err(g, "bios preos ucode copy failed %d", err);
		goto out;
	}

	err = nvgpu_falcon_copy_to_dmem(&g->pmu.flcn, g->bios.preos.dmem_phys_base,
			g->bios.preos.dmem,
			g->bios.preos.dmem_size,
			0);
	if (err != 0) {
		nvgpu_err(g, "bios preos dmem copy failed %d", err);
		goto out;
	}

	err = nvgpu_falcon_bootstrap(&g->pmu.flcn,
					g->bios.preos.code_entry_point);
	if (err != 0) {
		nvgpu_err(g, "falcon bootstrap failed %d", err);
		goto out;
	}

	err = g->ops.bios.preos_wait_for_halt(g);
	if (err != 0) {
		nvgpu_err(g, "preos_wait_for_halt failed %d", err);
		goto out;
	}

	err = nvgpu_falcon_clear_halt_intr_status(&g->pmu.flcn,
			nvgpu_get_poll_timeout(g));
	if (err != 0) {
		nvgpu_err(g, "falcon_clear_halt_intr_status failed %d", err);
		goto out;
	}

out:
	nvgpu_log_fn(g, "done");
	return err;
}

int gp106_bios_init(struct gk20a *g)
{
	unsigned int i;
	int err;

	nvgpu_log_fn(g, " ");

	if (g->bios_is_init) {
		return 0;
	}

	nvgpu_log_info(g, "reading bios from EEPROM");
	g->bios.size = BIOS_SIZE;
	g->bios.data = nvgpu_vmalloc(g, BIOS_SIZE);
	if (g->bios.data == NULL) {
		return -ENOMEM;
	}

	if (g->ops.xve.disable_shadow_rom != NULL) {
		g->ops.xve.disable_shadow_rom(g);
	}
	for (i = 0U; i < g->bios.size/4U; i++) {
		u32 val = be32_to_cpu(gk20a_readl(g, 0x300000U + i*4U));

		g->bios.data[(i*4U)] = (val >> 24U) & 0xffU;
		g->bios.data[(i*4U)+1U] = (val >> 16U) & 0xffU;
		g->bios.data[(i*4U)+2U] = (val >> 8U) & 0xffU;
		g->bios.data[(i*4U)+3U] = val & 0xffU;
	}
	if (g->ops.xve.enable_shadow_rom != NULL) {
		g->ops.xve.enable_shadow_rom(g);
	}

	err = nvgpu_bios_parse_rom(g);
	if (err != 0) {
		goto free_firmware;
	}

	if (g->bios.vbios_version < g->vbios_min_version) {
		nvgpu_err(g, "unsupported VBIOS version %08x",
				g->bios.vbios_version);
		err = -EINVAL;
		goto free_firmware;
	} else {
		nvgpu_info(g, "VBIOS version %08x", g->bios.vbios_version);
	}

	if ((g->vbios_compatible_version != 0U) &&
	    (g->bios.vbios_version != g->vbios_compatible_version)) {
		nvgpu_err(g, "VBIOS version %08x is not officially supported.",
			g->bios.vbios_version);
		nvgpu_err(g, "Update to VBIOS %08x, or use at your own risks.",
			g->vbios_compatible_version);
	}

	nvgpu_log_fn(g, "done");

	if (g->ops.bios.devinit != NULL) {
		err = g->ops.bios.devinit(g);
		if (err != 0) {
			nvgpu_err(g, "devinit failed");
			goto free_firmware;
		}
	}

	if (nvgpu_is_enabled(g, NVGPU_PMU_RUN_PREOS) &&
	    (g->ops.bios.preos != NULL)) {
		err = g->ops.bios.preos(g);
		if (err != 0) {
			nvgpu_err(g, "pre-os failed");
			goto free_firmware;
		}
	}

	if (g->ops.bios.verify_devinit != NULL) {
		err = g->ops.bios.verify_devinit(g);
		if (err != 0) {
			nvgpu_err(g, "devinit status verification failed");
			goto free_firmware;
		}
	}

	g->bios_is_init = true;

	return 0;
free_firmware:
	if (g->bios.data != NULL) {
		nvgpu_vfree(g, g->bios.data);
	}
	return err;
}
