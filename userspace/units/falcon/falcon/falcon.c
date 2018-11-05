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

#include <unit/unit.h>
#include <unit/io.h>

#include <nvgpu/gk20a.h>
#include <nvgpu/hal_init.h>
#include <nvgpu/posix/io.h>
#include <nvgpu/hw/gp10b/hw_fuse_gp10b.h>

#include "../falcon_utf.h"

static struct nvgpu_falcon *pmu_flcn;
static struct nvgpu_falcon *uninit_flcn;
static u32 *rand_test_data;

#define NV_PMC_BOOT_0_ARCHITECTURE_GV110	(0x00000015 << \
						 NVGPU_GPU_ARCHITECTURE_SHIFT)
#define NV_PMC_BOOT_0_IMPLEMENTATION_B		0xB
#define MAX_MEM_TYPE				(MEM_IMEM + 1)

#define RAND_DATA_SIZE				(SZ_4K)

static void init_rand_buffer(void)
{
	u32 i;

	/*
	 * Fill the test buffer with random data. Always use the same seed to
	 * make the test deterministic.
	 */
	srand(0);
	for (i = 0; i < RAND_DATA_SIZE/sizeof(u32); i++) {
		rand_test_data[i] = (u32) rand();
	}
}

static int init_falcon_test_env(struct unit_module *m, struct gk20a *g)
{
	int err = 0;

	nvgpu_posix_io_init_reg_space(g);
	nvgpu_utf_falcon_register_io(g);

	/*
	 * Fuse register fuse_opt_priv_sec_en_r() is read during init_hal hence
	 * add it to reg space
	 */
	if (nvgpu_posix_io_add_reg_space(g,
					 fuse_opt_priv_sec_en_r(), 0x4) != 0) {
		unit_err(m, "Add reg space failed!\n");
		return -ENOMEM;
	}

	/* HAL init parameters for gv11b */
	g->params.gpu_arch = NV_PMC_BOOT_0_ARCHITECTURE_GV110;
	g->params.gpu_impl = NV_PMC_BOOT_0_IMPLEMENTATION_B;

	/* HAL init required for getting the falcon ops initialized. */
	err = nvgpu_init_hal(g);
	if (err != 0) {
		return -ENODEV;
	}

	/* Initialize utf & nvgpu falcon for test usage */
	err = nvgpu_utf_falcon_init(m, g, FALCON_ID_PMU);
	if (err) {
		return err;
	}

	/* Set falcons for test usage */
	pmu_flcn = &g->pmu_flcn;
	uninit_flcn = &g->fecs_flcn;

	/* Create a test buffer to be filled with random data */
	rand_test_data = (u32 *) nvgpu_kzalloc(g, RAND_DATA_SIZE);
	if (rand_test_data == NULL) {
		return -ENOMEM;
	}

	init_rand_buffer();
	return 0;
}

static int free_falcon_test_env(struct unit_module *m, struct gk20a *g,
				void *__args)
{
	if (pmu_flcn == NULL || !pmu_flcn->is_falcon_supported) {
		unit_return_fail(m, "test environment not initialized.");
	}

	nvgpu_kfree(g, rand_test_data);
	nvgpu_utf_falcon_free(g, FALCON_ID_PMU);
	return UNIT_SUCCESS;
}

/*
 * This function will compare rand_test_data with falcon flcn's imem/dmem
 * based on type from offset src of size. It returns 0 on match else
 * non-zero value.
 */
static int falcon_read_compare(struct unit_module *m, struct gk20a *g,
			       enum falcon_mem_type type,
			       u32 src, u32 size)
{
	u8 *dest = NULL, *destp;
	u32 total_block_read = 0;
	u32 byte_read_count = 0;
	u32 byte_cnt = size;
	int err;

	dest = (u8 *) nvgpu_kzalloc(g, byte_cnt);
	if (dest == NULL) {
		unit_err(m, "Memory allocation failed\n");
		return -ENOMEM;
	}

	destp = dest;

	total_block_read = byte_cnt >> 8;
	do {
		byte_read_count =
				total_block_read ? FALCON_BLOCK_SIZE : byte_cnt;
		if (!byte_read_count) {
			break;
		}

		if (type == MEM_IMEM) {
			err = nvgpu_falcon_copy_from_imem(pmu_flcn, src,
					destp, byte_read_count, 0);
		} else if (type == MEM_DMEM) {
			err = nvgpu_falcon_copy_from_dmem(pmu_flcn, src,
					destp, byte_read_count, 0);
		} else {
			unit_err(m, "Invalid read type\n");
			err = -EINVAL;
			goto free_dest;
		}

		if (err) {
			unit_err(m, "Failed to copy from falcon memory\n");
			goto free_dest;
		}

		destp += byte_read_count;
		src += byte_read_count;
		byte_cnt -= byte_read_count;
	} while (total_block_read--);

	if (memcmp((void *) dest, (void *) rand_test_data, size) != 0) {
		unit_err(m, "Mismatch comparing copied data\n");
		err = -EINVAL;
	}

free_dest:
	nvgpu_kfree(g, dest);
	return err;
}

/*
 * This function will check that falcon memory read/write functions with
 * specified parameters complete with return value exp_err.
 */
static int falcon_check_read_write(struct gk20a *g,
				   struct unit_module *m,
				   struct nvgpu_falcon *flcn,
				   enum falcon_mem_type type,
				   u32 dst, u32 byte_cnt, int exp_err)
{
	u8 *dest = NULL;
	int ret = -1;
	int err;

	dest = (u8 *) nvgpu_kzalloc(g, byte_cnt);
	if (dest == NULL) {
		unit_err(m, "Memory allocation failed\n");
		goto out;
	}


	if (type == MEM_IMEM) {
		err = nvgpu_falcon_copy_to_imem(flcn, dst,
						(u8 *) rand_test_data,
						byte_cnt, 0, false, 0);
		if (err != exp_err) {
			unit_err(m, "Copy to IMEM should %s\n",
				 exp_err ? "fail" : "pass");
			goto free_dest;
		}

		err = nvgpu_falcon_copy_from_imem(flcn, dst,
						  dest, byte_cnt, 0);
		if (err != exp_err) {
			unit_err(m, "Copy from IMEM should %s\n",
				 exp_err ? "fail" : "pass");
			goto free_dest;
		}
	} else if (type == MEM_DMEM) {
		err = nvgpu_falcon_copy_to_dmem(flcn, dst,
						(u8 *) rand_test_data,
						byte_cnt, 0);
		if (err != exp_err) {
			unit_err(m, "Copy to DMEM should %s\n",
				 exp_err ? "fail" : "pass");
			goto free_dest;
		}

		err = nvgpu_falcon_copy_from_dmem(flcn, dst,
						  dest, byte_cnt, 0);
		if (err != exp_err) {
			unit_err(m, "Copy from DMEM should %s\n",
				 exp_err ? "fail" : "pass");
			goto free_dest;
		}
	}

	ret = 0;

free_dest:
	nvgpu_kfree(g, dest);
out:
	return ret;
}

/*
 * Valid/Invalid: Status of read and write from Falcon
 * Valid: Read and write from initialized Falcon succeeds.
 * Invalid: Read and write for uninitialized Falcon fails
 *	    with error -EINVAL.
 */
static int test_falcon_mem_rw_init(struct unit_module *m, struct gk20a *g,
				   void *__args)
{
	u32 dst = 0;
	int err = 0, i;

	/* initialize falcons */
	if (init_falcon_test_env(m, g) != 0) {
		unit_return_fail(m, "Module init failed\n");
	}

	/* write/read to/from uninitialized falcon */
	for (i = 0; i < MAX_MEM_TYPE; i++) {
		err = falcon_check_read_write(g, m, uninit_flcn, i, dst,
					      RAND_DATA_SIZE, -EINVAL);
		if (err) {
			return UNIT_FAIL;
		}
	}

	/* write/read to/from initialized falcon */
	for (i = 0; i < MAX_MEM_TYPE; i++) {
		err = falcon_check_read_write(g, m, pmu_flcn, i, dst,
					      RAND_DATA_SIZE, 0);
		if (err) {
			return UNIT_FAIL;
		}
	}

	return UNIT_SUCCESS;
}

/*
 * Valid/Invalid: Reading and writing data in accessible range should work
 *		  and fail otherwise.
 * Valid: Data read from or written to Falcon memory in bounds is valid
 *	  operation and should return success.
 * Invalid: Reading and writing data out of Falcon memory bounds should
 *	    return error -EINVAL.
 */
static int test_falcon_mem_rw_range(struct unit_module *m, struct gk20a *g,
				    void *__args)
{
	u32 byte_cnt = RAND_DATA_SIZE;
	u32 dst = 0;
	int err = UNIT_FAIL;

	if (pmu_flcn == NULL || !pmu_flcn->is_falcon_supported) {
		unit_return_fail(m, "test environment not initialized.");
	}

	/* write data to valid range in imem */
	unit_info(m, "Writing %d bytes to imem\n", byte_cnt);
	err = nvgpu_falcon_copy_to_imem(pmu_flcn, dst, (u8 *) rand_test_data,
					byte_cnt, 0, false, 0);
	if (err) {
		unit_return_fail(m, "Failed to copy to IMEM\n");
	}

	/* verify data written to imem matches */
	unit_info(m, "Reading %d bytes from imem\n", byte_cnt);
	err = falcon_read_compare(m, g, MEM_IMEM, dst, byte_cnt);
	if (err) {
		unit_err(m, "IMEM read data does not match %d\n", err);
		return UNIT_FAIL;
	}

	/* write data to valid range in dmem */
	unit_info(m, "Writing %d bytes to dmem\n", byte_cnt);
	err = nvgpu_falcon_copy_to_dmem(pmu_flcn, dst, (u8 *) rand_test_data,
					byte_cnt, 0);
	if (err) {
		unit_return_fail(m, "Failed to copy to DMEM\n");
	}

	/* verify data written to dmem matches */
	unit_info(m, "Reading %d bytes from dmem\n", byte_cnt);
	err = falcon_read_compare(m, g, MEM_DMEM, dst, byte_cnt);
	if (err) {
		unit_err(m, "DMEM read data does not match %d\n", err);
		return UNIT_FAIL;
	}

	dst = UTF_FALCON_IMEM_DMEM_SIZE - RAND_DATA_SIZE;
	byte_cnt *= 2;

	/* write/read data to/from invalid range in imem */
	err = falcon_check_read_write(g, m, pmu_flcn, MEM_IMEM, dst,
				      byte_cnt, -EINVAL);
	if (err) {
		return UNIT_FAIL;
	}

	/* write/read data to/from invalid range in dmem */
	err = falcon_check_read_write(g, m, pmu_flcn, MEM_DMEM, dst,
				      byte_cnt, -EINVAL);
	if (err) {
		return UNIT_FAIL;
	}

	return UNIT_SUCCESS;
}

/*
 * Valid/Invalid: Reading and writing data at offset that is word (4-byte)
 *		  aligned data should work and fail otherwise.
 * Valid: Data read/written from/to Falcon memory from word (4-byte) aligned
 *	  offset is valid operation and should return success.
 * Invalid: Reading and writing data out of non-word-aligned offset in Falcon
 *	    memory should return error -EINVAL.
 */
static int test_falcon_mem_rw_aligned(struct unit_module *m, struct gk20a *g,
				      void *__args)
{
	u32 byte_cnt = RAND_DATA_SIZE;
	u32 dst = 0, i;
	int err = 0;

	if (pmu_flcn == NULL || !pmu_flcn->is_falcon_supported) {
		unit_return_fail(m, "test environment not initialized.");
	}

	for (i = 0; i < MAX_MEM_TYPE; i++) {
		/*
		 * Copy to/from offset dst = 3 that is not word aligned should
		 * fail.
		 */
		dst = 0x3;
		err = falcon_check_read_write(g, m, pmu_flcn, i, dst,
					      byte_cnt, -EINVAL);
		if (err) {
			return UNIT_FAIL;
		}

		/*
		 * Copy to/from offset dst = 4 that is word aligned should
		 * succeed.
		 */
		dst = 0x4;
		err = falcon_check_read_write(g, m, pmu_flcn, i, dst,
					      byte_cnt, 0);
		if (err) {
			return UNIT_FAIL;
		}
	}

	return UNIT_SUCCESS;
}

/*
 * Reading/writing zero bytes should return error -EINVAL.
 */
static int test_falcon_mem_rw_zero(struct unit_module *m, struct gk20a *g,
				   void *__args)
{
	u32 byte_cnt = 0;
	u32 dst = 0, i;
	int err = 0;

	if (pmu_flcn == NULL || !pmu_flcn->is_falcon_supported) {
		unit_return_fail(m, "test environment not initialized.");
	}

	for (i = 0; i < MAX_MEM_TYPE; i++) {
		/* write/read zero bytes should fail*/
		err = falcon_check_read_write(g, m, pmu_flcn, MEM_IMEM, dst,
					      byte_cnt, -EINVAL);
		if (err) {
			return UNIT_FAIL;
		}
	}

	return UNIT_SUCCESS;
}

struct unit_module_test falcon_tests[] = {
	UNIT_TEST(falcon_mem_rw_init, test_falcon_mem_rw_init, NULL, 0),
	UNIT_TEST(falcon_mem_rw_range, test_falcon_mem_rw_range, NULL, 0),
	UNIT_TEST(falcon_mem_rw_aligned, test_falcon_mem_rw_aligned, NULL, 0),
	UNIT_TEST(falcon_mem_rw_zero, test_falcon_mem_rw_zero, NULL, 0),

	/* Cleanup */
	UNIT_TEST(falcon_free_test_env, free_falcon_test_env, NULL, 0),
};

UNIT_MODULE(falcon, falcon_tests, UNIT_PRIO_NVGPU_TEST);
