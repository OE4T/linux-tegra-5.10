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

#include <unit/io.h>

#include <nvgpu/falcon.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/kmem.h>
#include <nvgpu/hw/gm20b/hw_falcon_gm20b.h>

#include "falcon_utf.h"

struct utf_falcon utf_falcons[FALCON_ID_END];

static struct utf_falcon *get_utf_falcon_from_id(struct gk20a *g, u32 falcon_id)
{
	struct utf_falcon *flcn = NULL;

	switch (falcon_id) {
	case FALCON_ID_PMU:
	case FALCON_ID_FECS:
	case FALCON_ID_GPCCS:
#ifdef CONFIG_NVGPU_DGPU
	case FALCON_ID_GSPLITE:
	case FALCON_ID_NVDEC:
	case FALCON_ID_SEC2:
	case FALCON_ID_MINION:
#endif
		flcn = &utf_falcons[falcon_id];
		break;
	default:
		break;
	}

	return flcn;
}

static struct utf_falcon *get_utf_falcon_from_addr(struct gk20a *g, u32 addr)
{
	struct utf_falcon *flcn = NULL;
	u32 flcn_base;
	u32 i;

	for (i = 0; i < FALCON_ID_END; i++) {
		if (utf_falcons[i].flcn == NULL) {
			continue;
		}

		flcn_base = utf_falcons[i].flcn->flcn_base;
		if ((addr >= flcn_base) &&
		    (addr < (flcn_base + UTF_FALCON_MAX_REG_OFFSET))) {
			flcn = get_utf_falcon_from_id(g, i);
			break;
		}
	}

	return flcn;
}

static void falcon_writel_access_reg_fn(struct gk20a *g,
					struct utf_falcon *flcn,
					struct nvgpu_reg_access *access)
{
	u32 addr_mask = falcon_falcon_dmemc_offs_m() |
			falcon_falcon_dmemc_blk_m();
	u32 flcn_base;
	u32 ctrl_r;
	u32 offset;

	flcn_base = flcn->flcn->flcn_base;

	if (access->addr == (flcn_base + falcon_falcon_imemd_r(0))) {
		ctrl_r = nvgpu_posix_io_readl_reg_space(g,
				flcn_base + falcon_falcon_imemc_r(0));

		if (ctrl_r & falcon_falcon_imemc_aincw_f(1)) {
			offset = ctrl_r & addr_mask;

			*((u32 *) ((u8 *)flcn->imem + offset)) = access->value;
			offset += 4U;

			ctrl_r &= ~(addr_mask);
			ctrl_r |= offset;
			nvgpu_posix_io_writel_reg_space(g,
				flcn_base + falcon_falcon_imemc_r(0), ctrl_r);
		}
	} else if (access->addr == (flcn_base + falcon_falcon_dmemd_r(0))) {
		ctrl_r = nvgpu_posix_io_readl_reg_space(g,
				flcn_base + falcon_falcon_dmemc_r(0));

		if (ctrl_r & falcon_falcon_dmemc_aincw_f(1)) {
			offset = ctrl_r & addr_mask;

			*((u32 *) ((u8 *)flcn->dmem + offset)) = access->value;
			offset += 4U;

			ctrl_r &= ~(addr_mask);
			ctrl_r |= offset;
			nvgpu_posix_io_writel_reg_space(g,
				flcn_base + falcon_falcon_dmemc_r(0), ctrl_r);
		}
	}

	nvgpu_posix_io_writel_reg_space(g, access->addr, access->value);
}

static void falcon_readl_access_reg_fn(struct gk20a *g,
				       struct utf_falcon *flcn,
				       struct nvgpu_reg_access *access)
{
	u32 addr_mask = falcon_falcon_dmemc_offs_m() |
			falcon_falcon_dmemc_blk_m();
	u32 flcn_base;
	u32 ctrl_r;
	u32 offset;

	flcn_base = flcn->flcn->flcn_base;

	if (access->addr == (flcn_base + falcon_falcon_imemd_r(0))) {
		ctrl_r = nvgpu_posix_io_readl_reg_space(g,
				flcn_base + falcon_falcon_imemc_r(0));

		if (ctrl_r & falcon_falcon_dmemc_aincr_f(1)) {
			offset = ctrl_r & addr_mask;

			access->value = *((u32 *) ((u8 *)flcn->imem + offset));
			offset += 4U;

			ctrl_r &= ~(addr_mask);
			ctrl_r |= offset;
			nvgpu_posix_io_writel_reg_space(g,
				flcn_base + falcon_falcon_imemc_r(0), ctrl_r);
		}
	} else if (access->addr == (flcn_base + falcon_falcon_dmemd_r(0))) {
		ctrl_r = nvgpu_posix_io_readl_reg_space(g,
				flcn_base + falcon_falcon_dmemc_r(0));

		if (ctrl_r & falcon_falcon_dmemc_aincr_f(1)) {
			offset = ctrl_r & addr_mask;

			access->value = *((u32 *) ((u8 *)flcn->dmem + offset));
			offset += 4U;

			ctrl_r &= ~(addr_mask);
			ctrl_r |= offset;
			nvgpu_posix_io_writel_reg_space(g,
				flcn_base + falcon_falcon_dmemc_r(0), ctrl_r);
		}
	} else if (access->addr == (flcn_base + falcon_falcon_dmemc_r(0))) {
		ctrl_r = nvgpu_posix_io_readl_reg_space(g,
				flcn_base + falcon_falcon_dmemc_r(0));

		offset = access->value & addr_mask;
		access->value = offset * 4U;
	} else {
		access->value = nvgpu_posix_io_readl_reg_space(g, access->addr);
	}
}

static void writel_access_reg_fn(struct gk20a *g,
				 struct nvgpu_reg_access *access)
{
	struct utf_falcon *flcn = NULL;

	flcn = get_utf_falcon_from_addr(g, access->addr);
	if (flcn != NULL) {
		falcon_writel_access_reg_fn(g, flcn, access);
	} else {
		nvgpu_posix_io_writel_reg_space(g, access->addr, access->value);
	}
	nvgpu_posix_io_record_access(g, access);
}

static void readl_access_reg_fn(struct gk20a *g,
				struct nvgpu_reg_access *access)
{
	struct utf_falcon *flcn = NULL;

	flcn = get_utf_falcon_from_addr(g, access->addr);
	if (flcn != NULL) {
		falcon_readl_access_reg_fn(g, flcn, access);
	} else {
		access->value = nvgpu_posix_io_readl_reg_space(g, access->addr);
	}
}

static struct nvgpu_posix_io_callbacks utf_falcon_reg_callbacks = {
	.writel          = writel_access_reg_fn,
	.writel_check    = writel_access_reg_fn,
	.bar1_writel     = writel_access_reg_fn,
	.usermode_writel = writel_access_reg_fn,

	.__readl         = readl_access_reg_fn,
	.readl           = readl_access_reg_fn,
	.bar1_readl      = readl_access_reg_fn,
};

void nvgpu_utf_falcon_register_io(struct gk20a *g)
{
	nvgpu_posix_register_io(g, &utf_falcon_reg_callbacks);
}

int nvgpu_utf_falcon_init(struct unit_module *m, struct gk20a *g, u32 flcn_id)
{
	struct utf_falcon *utf_flcn;
	struct nvgpu_falcon *flcn;
	u32 flcn_size;
	u32 flcn_base;
	u32 hwcfg_r, hwcfg1_r, ports;
	int err = 0;

	if (utf_falcons[flcn_id].flcn != NULL) {
		unit_err(m, "Falcon already initialized!\n");
		return -EINVAL;
	}

	err = nvgpu_falcon_sw_init(g, flcn_id);
	if (err != 0) {
		unit_err(m, "nvgpu Falcon init failed!\n");
		return err;
	}

	flcn = nvgpu_falcon_get_instance(g, flcn_id);

	utf_flcn = &utf_falcons[flcn_id];
	utf_flcn->flcn = flcn;

	flcn_base = flcn->flcn_base;
	if (nvgpu_posix_io_add_reg_space(g,
					 flcn_base,
					 UTF_FALCON_MAX_REG_OFFSET) != 0) {
		unit_err(m, "Falcon add reg space failed!\n");
		nvgpu_falcon_sw_free(g, flcn_id);
		return -ENOMEM;
	}

	/*
	 * Initialize IMEM & DMEM size that will be needed by NvGPU for
	 * bounds check.
	 */
	hwcfg_r = flcn_base + falcon_falcon_hwcfg_r();
	flcn_size = UTF_FALCON_IMEM_DMEM_SIZE / FALCON_BLOCK_SIZE;
	flcn_size = (flcn_size << 9) | flcn_size;
	nvgpu_posix_io_writel_reg_space(g, hwcfg_r, flcn_size);

	/* set imem and dmem ports count. */
	hwcfg1_r = flcn_base + falcon_falcon_hwcfg1_r();
	ports = (1 << 8) | (1 << 12);
	nvgpu_posix_io_writel_reg_space(g, hwcfg1_r, ports);

	utf_flcn->imem = (u32 *) nvgpu_kzalloc(g, UTF_FALCON_IMEM_DMEM_SIZE);
	if (utf_flcn->imem == NULL) {
		err = -ENOMEM;
		unit_err(m, "Falcon imem alloc failed!\n");
		goto out;
	}

	utf_flcn->dmem = (u32 *) nvgpu_kzalloc(g, UTF_FALCON_IMEM_DMEM_SIZE);
	if (utf_flcn->dmem == NULL) {
		err = -ENOMEM;
		unit_err(m, "Falcon dmem alloc failed!\n");
		goto clean_imem;
	}

	return 0;

clean_imem:
	nvgpu_kfree(g, utf_flcn->imem);
out:
	nvgpu_posix_io_delete_reg_space(g, flcn_base);
	nvgpu_falcon_sw_free(g, flcn_id);

	return err;
}

void nvgpu_utf_falcon_free(struct gk20a *g, u32 flcn_id)
{
	struct utf_falcon *utf_flcn;

	utf_flcn = &utf_falcons[flcn_id];

	if (utf_flcn->flcn == NULL)
		return;

	nvgpu_kfree(g, utf_flcn->dmem);
	nvgpu_kfree(g, utf_flcn->imem);
	nvgpu_posix_io_delete_reg_space(g, utf_flcn->flcn->flcn_base);
	nvgpu_falcon_sw_free(g, flcn_id);
	utf_flcn->flcn = NULL;
}
