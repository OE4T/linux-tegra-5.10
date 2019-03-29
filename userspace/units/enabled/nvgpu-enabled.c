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
#include <unit/io.h>
#include <unit/unit.h>
#include <nvgpu/enabled.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/posix/posix-fault-injection.h>

static unsigned long *original_enabled_flags;

/*
 * Test nvgpu_init_enabled_flags():
 *
 * 1. With memory failure, check that init fails with -ENOMEM
 * 2. Init works in regular scenario
 *
 * This test should be first to execute. Newly, inited enabled_flags will be
 * used by rest of the tests in this module.
 *
 * Note: Since, enabled_flags are allocated during gk20a init, we store
 * original allocated memory address in test_init_free_parameters structure.
 * This pointer should be restored before exiting this module.
 */
static int test_nvgpu_init_enabled_flags(struct unit_module *m,
					struct gk20a *g, void *args)
{
	int err;
	struct nvgpu_posix_fault_inj *kmem_fi =
		nvgpu_kmem_get_fault_injection();
	original_enabled_flags = g->enabled_flags;

	/*
	 * Test 1 - Enable SW fault injection and check that init function
	 * fails with -ENOMEM.
	 */

	nvgpu_posix_enable_fault_injection(kmem_fi, true, 0);
	err = nvgpu_init_enabled_flags(g);
	if (err != -ENOMEM) {
		g->enabled_flags = original_enabled_flags;
		unit_return_fail(m,
			"enabled_flags init didn't fail as expected\n");
	}
	nvgpu_posix_enable_fault_injection(kmem_fi, false, 0);

	/*
	 * Test 2 - Check that enabled_flags are inited successfully
	 * Use these flags (allocated memory) for next tests in module.
	 */

	err = nvgpu_init_enabled_flags(g);
	if (err != 0) {
		g->enabled_flags = original_enabled_flags;
		unit_return_fail(m, "enabled_flags init failed\n");
	}
	return UNIT_SUCCESS;
}

/*
 * Test nvgpu_is_enabled():
 *
 * As enabled_flags inited(using kzalloc) by previous test, all flags should
 * be disabled (set to 0).
*/
static int test_nvgpu_enabled_flags_false_check(struct unit_module *m,
					struct gk20a *g, void *args)
{
	u32 i;
	u32 n = NVGPU_MAX_ENABLED_BITS;

	for (i = 1U; i < n; i++) { /* First flag is index 1 */
		if (nvgpu_is_enabled(g, i)) {
			g->enabled_flags = original_enabled_flags;
			unit_return_fail(m,
			     "enabled_flag %u inited to non-zero value\n", i);
		}
	}

	return UNIT_SUCCESS;
}

/*
 * Test nvgpu_set_enabled():
 *
 * 1. Set given flag, check that flag is enabled
 * 2. Reset given flag, check that flag is disabled
 *
 * Flag status is checked using nvgpu_is_enabled() function. This function is
 * verified in previous test.
 */
static int test_nvgpu_set_enabled(struct unit_module *m,
					struct gk20a *g, void *args)
{
	u32 i;
	u32 n = NVGPU_MAX_ENABLED_BITS;

	for (i = 1U; i < n; i++) { /* First flag is index 1 */
		nvgpu_set_enabled(g, i, true);
		if (!nvgpu_is_enabled(g, i)) {
			g->enabled_flags = original_enabled_flags;
			unit_return_fail(m,
				"enabled_flag %u could not be enabled\n", i);
		}

		nvgpu_set_enabled(g, i, false);
		if (nvgpu_is_enabled(g, i)) {
			g->enabled_flags = original_enabled_flags;
			unit_return_fail(m,
				"enabled_flag %u could not be disabled\n", i);
		}
	}

	return UNIT_SUCCESS;
}

/*
 * Test nvgpu_free_enabled_flags(): Free memory allocated in init test.
 *
 * Note: Not many alternatives to verify that memory is actually freed as
 * pages are cached and accessible until replaced.
 */
static int test_nvgpu_free_enabled_flags(struct unit_module *m,
					struct gk20a *g, void *args)
{
	nvgpu_free_enabled_flags(g);
	g->enabled_flags = original_enabled_flags;
	return UNIT_SUCCESS;
}

struct unit_module_test enabled_tests[] = {
	/*
	 * Init test should run first in order to use newly allocated memory.
	 */
	UNIT_TEST(init, test_nvgpu_init_enabled_flags, NULL, 0),

	UNIT_TEST(enabled_flags_false_check, test_nvgpu_enabled_flags_false_check, NULL, 0),
	UNIT_TEST(set_enabled, test_nvgpu_set_enabled, NULL, 0),

	UNIT_TEST(free, test_nvgpu_free_enabled_flags, NULL, 0),
};

UNIT_MODULE(enabled, enabled_tests, UNIT_PRIO_NVGPU_TEST);