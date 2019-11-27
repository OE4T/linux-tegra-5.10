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
#include <stdlib.h>
#include <unit/unit.h>
#include <unit/io.h>
#include <nvgpu/types.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/hal_init.h>
#include <nvgpu/posix/io.h>
#include <nvgpu/posix/posix-fault-injection.h>
#include <nvgpu/posix/posix-nvhost.h>
#include <nvgpu/channel.h>
#include <nvgpu/channel_sync.h>

#include "../fifo/nvgpu-fifo-gv11b.h"
#include "nvgpu-sync.h"

#define NV_PMC_BOOT_0_ARCHITECTURE_GV110        (0x00000015 << \
						NVGPU_GPU_ARCHITECTURE_SHIFT)
#define NV_PMC_BOOT_0_IMPLEMENTATION_B          0xB

static struct nvgpu_channel *ch;

int test_sync_init(struct unit_module *m, struct gk20a *g, void *args)
{
	int ret = 0;

	test_fifo_setup_gv11b_reg_space(m, g);

	nvgpu_set_enabled(g, NVGPU_HAS_SYNCPOINTS, true);

	/*
	 * HAL init parameters for gv11b
	 */
	g->params.gpu_arch = NV_PMC_BOOT_0_ARCHITECTURE_GV110;
	g->params.gpu_impl = NV_PMC_BOOT_0_IMPLEMENTATION_B;

	/*
	 * HAL init required for getting
	 * the sync ops initialized.
	 */
	ret = nvgpu_init_hal(g);
	if (ret != 0) {
		return -ENODEV;
	}

	ret = nvgpu_get_nvhost_dev(g);
	if (ret != 0) {
		unit_return_fail(m, "nvgpu_sync_early_init failed\n");
	}

	ch = nvgpu_kzalloc(g, sizeof(struct nvgpu_channel));
	if (ch == NULL) {
		unit_return_fail(m, "sync channel creation failure");
	}

	ch->g = g;

	return UNIT_SUCCESS;
}

int test_sync_create_sync(struct unit_module *m, struct gk20a *g, void *args)
{
	struct nvgpu_channel_sync *sync = NULL;

	sync = nvgpu_channel_sync_create(ch, true);

	if (sync != NULL) {
		unit_return_fail(m, "expected failure in creating sync points");
	}

	return UNIT_SUCCESS;
}

int test_sync_deinit(struct unit_module *m, struct gk20a *g, void *args)
{
	if (ch != NULL) {
		nvgpu_kfree(g, ch);
	}

	if (g->nvhost_dev == NULL) {
		unit_return_fail(m ,"no valid nvhost device exists\n");
	}

	nvgpu_free_nvhost_dev(g);

	test_fifo_cleanup_gv11b_reg_space(m, g);

	return UNIT_SUCCESS;
}

struct unit_module_test nvgpu_sync_tests[] = {
	UNIT_TEST(sync_init, test_sync_init, NULL, 0),
	UNIT_TEST(sync_deinit, test_sync_create_sync, NULL, 0),
	UNIT_TEST(sync_deinit, test_sync_deinit, NULL, 0),
};

UNIT_MODULE(nvgpu-sync, nvgpu_sync_tests, UNIT_PRIO_NVGPU_TEST);