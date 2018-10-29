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

#include <unit/io.h>
#include <unit/unit.h>

#include <nvgpu/gk20a.h>
#include <nvgpu/gmmu.h>
#include <nvgpu/enabled.h>

#include <nvgpu/posix/kmem.h>
#include <nvgpu/posix/posix-fault-injection.h>

/*
 * Direct allocs are allocs large enough to just pass straight on to the
 * DMA allocator. Basically that means the size of the PD is larger than a page.
 */
struct pd_cache_alloc_direct_gen {
	u32 bytes;
	u32 nr;
	u32 nr_allocs_before_free;
	u32 nr_frees_before_alloc;
};

/*
 * Direct alloc testing: i.e larger than a page allocs.
 */
static struct pd_cache_alloc_direct_gen alloc_direct_1xPAGE = {
	.bytes = PAGE_SIZE,
	.nr    = 1U,
};
static struct pd_cache_alloc_direct_gen alloc_direct_1024xPAGE = {
	.bytes = PAGE_SIZE,
	.nr    = 1024U,
};
static struct pd_cache_alloc_direct_gen alloc_direct_1x16PAGE = {
	.bytes = 16U * PAGE_SIZE,
	.nr    = 1U,
};
static struct pd_cache_alloc_direct_gen alloc_direct_1024x16PAGE = {
	.bytes = 16U * PAGE_SIZE,
	.nr    = 1024U,
};
static struct pd_cache_alloc_direct_gen alloc_direct_1024xPAGE_x32x24 = {
	.bytes = PAGE_SIZE,
	.nr    = 1024U,
	.nr_allocs_before_free = 32U,
	.nr_frees_before_alloc = 24U
};
static struct pd_cache_alloc_direct_gen alloc_direct_1024xPAGE_x16x4 = {
	.bytes = PAGE_SIZE,
	.nr    = 1024U,
	.nr_allocs_before_free = 16U,
	.nr_frees_before_alloc = 4U
};
static struct pd_cache_alloc_direct_gen alloc_direct_1024xPAGE_x16x15 = {
	.bytes = PAGE_SIZE,
	.nr    = 1024U,
	.nr_allocs_before_free = 16U,
	.nr_frees_before_alloc = 15U
};
static struct pd_cache_alloc_direct_gen alloc_direct_1024xPAGE_x16x1 = {
	.bytes = PAGE_SIZE,
	.nr    = 1024U,
	.nr_allocs_before_free = 16U,
	.nr_frees_before_alloc = 1U
};

/*
 * Sub-page sized allocs. This will test the logic of the pd_caching.
 */
static struct pd_cache_alloc_direct_gen alloc_1x256B = {
	.bytes = 256U,
	.nr    = 1U,
};
static struct pd_cache_alloc_direct_gen alloc_1x512B = {
	.bytes = 512U,
	.nr    = 1U,
};
static struct pd_cache_alloc_direct_gen alloc_1x1024B = {
	.bytes = 1024U,
	.nr    = 1U,
};
static struct pd_cache_alloc_direct_gen alloc_1x2048B = {
	.bytes = 2048U,
	.nr    = 1U,
};
static struct pd_cache_alloc_direct_gen alloc_1024x256B_x16x15 = {
	.bytes = 256U,
	.nr    = 1024U,
	.nr_allocs_before_free = 16U,
	.nr_frees_before_alloc = 15U
};
static struct pd_cache_alloc_direct_gen alloc_1024x256B_x16x1 = {
	.bytes = 256U,
	.nr    = 1024U,
	.nr_allocs_before_free = 16U,
	.nr_frees_before_alloc = 1U
};
static struct pd_cache_alloc_direct_gen alloc_1024x256B_x32x1 = {
	.bytes = 256U,
	.nr    = 1024U,
	.nr_allocs_before_free = 32U,
	.nr_frees_before_alloc = 1U
};
static struct pd_cache_alloc_direct_gen alloc_1024x256B_x11x3 = {
	.bytes = 256U,
	.nr    = 1024U,
	.nr_allocs_before_free = 11U,
	.nr_frees_before_alloc = 3U
};

/*
 * Init a PD cache for us to use.
 */
static int init_pd_cache(struct unit_module *m,
			 struct gk20a *g, struct vm_gk20a *vm)
{
	int err;

	/*
	 * Make sure there's not already a pd_cache inited.
	 */
	if (g->mm.pd_cache != NULL) {
		unit_return_fail(m, "pd_cache already inited\n");
	}

	/*
	 * This is just enough init of the VM to get this code to work. Really
	 * these APIs should just take the gk20a struct...
	 */
	vm->mm = &g->mm;

	err = nvgpu_pd_cache_init(g);
	if (err != 0) {
		unit_return_fail(m, "nvgpu_pd_cache_init failed ??\n");
	}

	return UNIT_SUCCESS;
}

/*
 * Generate a test based on the args in @args. The test is very simple. It
 * allocates nr allocs of the passed size either all at once or in an
 * interleaved pattern.
 *
 * If nr_allocs_before_free is set then this value will determine how many
 * allocs to do before trying frees. If unset it will be simply be nr.
 *
 * If nr_free_before_alloc is set this will determine the number of frees to
 * do before swapping back to allocs. This way you can control the interleaving
 * pattern to some degree. If not set it defaults to nr_allocs_before_free.
 *
 * Anything left over after the last free loop will be freed in one big loop.
 */
static int test_pd_cache_alloc_gen(struct unit_module *m,
				   struct gk20a *g, void *args)
{
	u32 i, j;
	int err;
	int test_status = UNIT_SUCCESS;
	struct vm_gk20a vm;
	struct nvgpu_gmmu_pd *pds;
	struct pd_cache_alloc_direct_gen *test_spec = args;

	pds = malloc(sizeof(*pds) * test_spec->nr);
	if (pds == NULL) {
		unit_return_fail(m, "OOM in unit test ??\n");
	}

	err = init_pd_cache(m, g, &vm);
	if (err != UNIT_SUCCESS) {
		return err;
	}

	if (test_spec->nr_allocs_before_free == 0U) {
		test_spec->nr_allocs_before_free = test_spec->nr;
		test_spec->nr_frees_before_alloc = 0U;
	}

	/*
	 * This takes the test spec and executes some allocs/frees.
	 */
	i = 0U;
	while (i < test_spec->nr) {
		bool do_break = false;

		/*
		 * Do some allocs. Note the i++. Keep marching i along.
		 */
		for (j = 0U; j < test_spec->nr_allocs_before_free; j++) {
			struct nvgpu_gmmu_pd *c = &pds[i++];

			memset(c, 0, sizeof(*c));
			err = nvgpu_pd_alloc(&vm, c, test_spec->bytes);
			if (err != 0) {
				unit_err(m, "%s():%d Failed to do an alloc\n",
					 __func__, __LINE__);
				goto cleanup_err;
			}

			if (i >= test_spec->nr) {
				/* Break the while loop too! */
				do_break = true;
				break;
			}
		}

		/*
		 * And now the frees. The --i is done for the same reason as the
		 * i++ in the alloc loop.
		 */
		for (j = 0U; j < test_spec->nr_frees_before_alloc; j++) {
			struct nvgpu_gmmu_pd *c = &pds[--i];

			/*
			 * Can't easily verify this works directly. Will have to
			 * do that later...
			 */
			nvgpu_pd_free(&vm, c);
		}

		/*
		 * Without this we alloc/free and incr/decr i forever...
		 */
		if (do_break) {
			break;
		}
	}

	/*
	 * We may well have a lot more frees to do!
	 */
	while (i > 0) {
		i--;
		nvgpu_pd_free(&vm, &pds[i]);
	}

	/*
	 * After freeing everything all the pd_cache entries should be cleaned
	 * up. This is not super easy to verify because the pd_cache impl hides
	 * it's data structures within the C code itself.
	 *
	 * We can at least check that the mem field within the nvgpu_gmmu_pd
	 * struct is zeroed. That implies that the nvgpu_pd_free() routine did
	 * at least run through the cleanup code on this nvgpu_gmmu_pd.
	 */
	for (i = 0U; i < test_spec->nr; i++) {
		if (pds[i].mem != NULL) {
			unit_err(m, "%s():%d PD was not freed: %u\n",
				 __func__, __LINE__, i);
			test_status = UNIT_FAIL;
		}
	}

	free(pds);
	nvgpu_pd_cache_fini(g);
	return test_status;

cleanup_err:
	for (i = 0U; i < test_spec->nr; i++) {
		if (pds[i].mem != NULL) {
			nvgpu_pd_free(&vm, &pds[i]);
		}
	}

	free(pds);
	nvgpu_pd_cache_fini(g);

	return UNIT_FAIL;
}

/*
 * Test free on empty PD cache. But make it interesting by doing a valid alloc
 * and freeing that alloc twice. Also verify NULL doesn't cause issues.
 */
static int test_pd_free_empty_pd(struct unit_module *m,
				 struct gk20a *g, void *args)
{
	int err;
	struct vm_gk20a vm;
	struct nvgpu_gmmu_pd pd;

	err = init_pd_cache(m, g, &vm);
	if (err != UNIT_SUCCESS) {
		return err;
	}

	/* First test cached frees. */
	err = nvgpu_pd_alloc(&vm, &pd, 2048U);
	if (err != 0) {
		unit_return_fail(m, "PD alloc failed");
	}

	/*
	 * nvgpu_pd_free() has no return value so we can't check this directly.
	 * So we will make sure we don't crash.
	 */
	nvgpu_pd_free(&vm, &pd);
	nvgpu_pd_free(&vm, &pd);

	pd.mem = NULL;
	nvgpu_pd_free(&vm, &pd);

	/* And now direct frees. */
	memset(&pd, 0U, sizeof(pd));
	err = nvgpu_pd_alloc(&vm, &pd, PAGE_SIZE);
	if (err != 0) {
		unit_return_fail(m, "PD alloc failed");
	}

	nvgpu_pd_free(&vm, &pd);
	nvgpu_pd_free(&vm, &pd);

	pd.mem = NULL;
	nvgpu_pd_free(&vm, &pd);

	nvgpu_pd_cache_fini(g);

	return UNIT_SUCCESS;
}

/*
 * Test invalid nvgpu_pd_alloc() calls. Invalid bytes, invalid pd_cache, etc.
 */
static int test_pd_alloc_invalid_input(struct unit_module *m,
				       struct gk20a *g, void *args)
{
	int err;
	struct vm_gk20a vm;
	struct nvgpu_gmmu_pd pd;
	u32 i, garbage[] = { 0U, 128U, 255U, 4095U, 3000U, 128U, 2049U };

	if (g->mm.pd_cache != NULL) {
		unit_return_fail(m, "pd_cache already inited\n");
	}

	/* Obviously shouldn't work pd_cache is not init'ed. */
	err = nvgpu_pd_alloc(&vm, &pd, 2048U);
	if (err == 0) {
		unit_return_fail(m, "pd_alloc worked on NULL pd_cache\n");
	}

	err = init_pd_cache(m, g, &vm);
	if (err != UNIT_SUCCESS) {
		return err;
	}

	/* Test garbage input. */
	for (i = 0U; i < (sizeof(garbage) / sizeof(garbage[0])); i++) {
		err = nvgpu_pd_alloc(&vm, &pd, garbage[i]);
		if (err == 0) {
			unit_return_fail(m, "PD alloc success: %u (failed)\n",
					 garbage[i]);
		}
	}

	nvgpu_pd_cache_fini(g);

	return UNIT_SUCCESS;
}

static int test_pd_alloc_direct_fi(struct unit_module *m,
				   struct gk20a *g, void *args)
{
	int err;
	struct vm_gk20a vm;
	struct nvgpu_gmmu_pd pd;
	struct nvgpu_posix_fault_inj *kmem_fi =
		nvgpu_kmem_get_fault_injection();
	struct nvgpu_posix_fault_inj *dma_fi =
		nvgpu_dma_alloc_get_fault_injection();

	err = init_pd_cache(m, g, &vm);
	if (err != UNIT_SUCCESS) {
		return err;
	}

	/*
	 * The alloc_direct() call is easy: there's two places we can fail. One
	 * is allocating the nvgpu_mem struct, the next is the DMA alloc into
	 * the nvgpu_mem struct. Inject faults for these and verify we A) don't
	 * crash and that the allocs are recorded as failures.
	 */

	nvgpu_posix_enable_fault_injection(kmem_fi, true, 0);
	err = nvgpu_pd_alloc(&vm, &pd, PAGE_SIZE);
	if (err == 0) {
		unit_return_fail(m, "pd_alloc() success with kmem OOM\n");
	}
	nvgpu_posix_enable_fault_injection(kmem_fi, false, 0);

	nvgpu_posix_enable_fault_injection(dma_fi, true, 0);
	err = nvgpu_pd_alloc(&vm, &pd, PAGE_SIZE);
	if (err == 0) {
		unit_return_fail(m, "pd_alloc() success with DMA OOM\n");
	}
	nvgpu_posix_enable_fault_injection(dma_fi, false, 0);

	nvgpu_pd_cache_fini(g);
	return UNIT_SUCCESS;
}

static int test_pd_alloc_fi(struct unit_module *m,
			    struct gk20a *g, void *args)
{
	int err;
	struct vm_gk20a vm;
	struct nvgpu_gmmu_pd pd;
	struct nvgpu_posix_fault_inj *kmem_fi =
		nvgpu_kmem_get_fault_injection();
	struct nvgpu_posix_fault_inj *dma_fi =
		nvgpu_dma_alloc_get_fault_injection();

	err = init_pd_cache(m, g, &vm);
	if (err != UNIT_SUCCESS) {
		return err;
	}

	/*
	 * nvgpu_pd_alloc_new() is effectively the same. We know we will hit the
	 * faults in the new alloc since we have no prior allocs. Therefor we
	 * won't hit a partial alloc and miss the DMA/kmem allocs.
	 */
	nvgpu_posix_enable_fault_injection(kmem_fi, true, 0);
	err = nvgpu_pd_alloc(&vm, &pd, 2048U);
	if (err == 0) {
		unit_return_fail(m, "pd_alloc() success with kmem OOM\n");
	}
	nvgpu_posix_enable_fault_injection(kmem_fi, false, 0);

	nvgpu_posix_enable_fault_injection(dma_fi, true, 0);
	err = nvgpu_pd_alloc(&vm, &pd, 2048U);
	if (err == 0) {
		unit_return_fail(m, "pd_alloc() success with DMA OOM\n");
	}
	nvgpu_posix_enable_fault_injection(dma_fi, false, 0);

	nvgpu_pd_cache_fini(g);
	return UNIT_SUCCESS;
}

/*
 * Test nvgpu_pd_cache_init() - make sure that:
 *
 *   1. Check that init with a memory failure returns -ENOMEM and that the
 *      pd_cache is not initialized.
 *   2. Initial init works.
 *   3. That re-init doesn't re-allocate any resources.
 */
static int test_pd_cache_init(struct unit_module *m,
			      struct gk20a *g, void *args)
{
	int err, i;
	struct nvgpu_pd_cache *cache;
	struct nvgpu_posix_fault_inj *kmem_fi =
		nvgpu_kmem_get_fault_injection();

	/*
	 * Test 1 - do some SW fault injection to make sure we hit the -ENOMEM
	 * potential when initializing the pd cache.
	 */
	nvgpu_posix_enable_fault_injection(kmem_fi, true, 0);
	err = nvgpu_pd_cache_init(g);
	if (err != -ENOMEM) {
		unit_return_fail(m, "OOM condition didn't lead to -ENOMEM\n");
	}

	if (g->mm.pd_cache != NULL) {
		unit_return_fail(m, "PD cache init'ed with no mem\n");
	}
	nvgpu_posix_enable_fault_injection(kmem_fi, false, 0);

	/*
	 * Test 2: Make sure that the init function initializes the necessary
	 * pd_cache data structure within the GPU @g. Just checks some internal
	 * data structures for their presence to make sure this code path has
	 * run.
	 */
	err = nvgpu_pd_cache_init(g);
	if (err != 0) {
		unit_return_fail(m, "PD cache failed to init!\n");
	}

	if (g->mm.pd_cache == NULL) {
		unit_return_fail(m, "PD cache data structure not inited!\n");
	}

	/*
	 * Test 3: make sure that any re-init call doesn't blow away a
	 * previously inited pd_cache.
	 */
	cache = g->mm.pd_cache;
	for (i = 0; i < 5; i++) {
		nvgpu_pd_cache_init(g);
	}

	if (g->mm.pd_cache != cache) {
		unit_return_fail(m, "PD cache got re-inited!\n");
	}

	/*
	 * Leave the PD cache inited at this point...
	 */
	return UNIT_SUCCESS;
}

/*
 * Test nvgpu_pd_cache_fini() - make sure that:
 *
 *   1. An actually allocated cache is cleaned up.
 *   2. If there is no cache this code doesn't crash.
 *
 * Note: this inherits the already inited pd_cache from test_pd_cache_init().
 */
static int test_pd_cache_fini(struct unit_module *m,
			      struct gk20a *g, void *args)
{
	if (g->mm.pd_cache == NULL) {
		unit_return_fail(m, "Missing an init'ed pd_cache\n");
	}

	/*
	 * Test 1: make sure the function pointer is NULL as that implies we
	 * made it to the nvgpu_kfree().
	 */
	nvgpu_pd_cache_fini(g);
	if (g->mm.pd_cache != NULL) {
		unit_return_fail(m, "Failed to cleanup pd_cache\n");
	}

	/*
	 * Test 2: this one is hard to test for functionality - just make sure
	 * we don't crash.
	 */
	nvgpu_pd_cache_fini(g);

	return UNIT_SUCCESS;
}

/*
 * Init the global env - just make sure we don't try and allocate from VIDMEM
 * when doing dma allocs.
 */
static int test_pd_cache_env_init(struct unit_module *m,
				  struct gk20a *g, void *args)
{
	__nvgpu_set_enabled(g, NVGPU_MM_UNIFIED_MEMORY, true);

	return UNIT_SUCCESS;
}

struct unit_module_test pd_cache_tests[] = {
	UNIT_TEST(env_init,				test_pd_cache_env_init, NULL),
	UNIT_TEST(init,					test_pd_cache_init, NULL),
	UNIT_TEST(fini,					test_pd_cache_fini, NULL),

	/*
	 * Direct allocs.
	 */
	UNIT_TEST(alloc_direct_1xPAGE,			test_pd_cache_alloc_gen, &alloc_direct_1xPAGE),
	UNIT_TEST(alloc_direct_1024xPAGE,		test_pd_cache_alloc_gen, &alloc_direct_1024xPAGE),
	UNIT_TEST(alloc_direct_1x16PAGE,		test_pd_cache_alloc_gen, &alloc_direct_1x16PAGE),
	UNIT_TEST(alloc_direct_1024x16PAGE,		test_pd_cache_alloc_gen, &alloc_direct_1024x16PAGE),
	UNIT_TEST(alloc_direct_1024xPAGE_x32x24,	test_pd_cache_alloc_gen, &alloc_direct_1024xPAGE_x32x24),
	UNIT_TEST(alloc_direct_1024xPAGE_x16x4,		test_pd_cache_alloc_gen, &alloc_direct_1024xPAGE_x16x4),
	UNIT_TEST(alloc_direct_1024xPAGE_x16x15,	test_pd_cache_alloc_gen, &alloc_direct_1024xPAGE_x16x15),
	UNIT_TEST(alloc_direct_1024xPAGE_x16x1,		test_pd_cache_alloc_gen, &alloc_direct_1024xPAGE_x16x1),

	/*
	 * Cached allocs.
	 */
	UNIT_TEST(alloc_1x256B,				test_pd_cache_alloc_gen, &alloc_1x256B),
	UNIT_TEST(alloc_1x512B,				test_pd_cache_alloc_gen, &alloc_1x512B),
	UNIT_TEST(alloc_1x1024B,			test_pd_cache_alloc_gen, &alloc_1x1024B),
	UNIT_TEST(alloc_1x2048B,			test_pd_cache_alloc_gen, &alloc_1x2048B),
	UNIT_TEST(alloc_1024x256B_x16x15,		test_pd_cache_alloc_gen, &alloc_1024x256B_x16x15),
	UNIT_TEST(alloc_1024x256B_x16x1,		test_pd_cache_alloc_gen, &alloc_1024x256B_x16x1),
	UNIT_TEST(alloc_1024x256B_x32x1,		test_pd_cache_alloc_gen, &alloc_1024x256B_x32x1),
	UNIT_TEST(alloc_1024x256B_x11x3,		test_pd_cache_alloc_gen, &alloc_1024x256B_x11x3),

	/*
	 * Error path testing.
	 */
	UNIT_TEST(free_empty,				test_pd_free_empty_pd, NULL),
	UNIT_TEST(invalid_pd_alloc,			test_pd_alloc_invalid_input, NULL),
	UNIT_TEST(alloc_direct_oom,			test_pd_alloc_direct_fi, NULL),
	UNIT_TEST(alloc_oom,				test_pd_alloc_fi, NULL),
};

UNIT_MODULE(pd_cache, pd_cache_tests, UNIT_PRIO_NVGPU_TEST);
