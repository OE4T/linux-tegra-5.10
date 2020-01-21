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

#include <stdlib.h>
#include <stdint.h>

#include <unit/io.h>
#include <unit/unit.h>

#include <nvgpu/utils.h>

#include "posix-utils.h"

/*
 * Test to ensure the EXPECT_BUG construct works as intended by making sure it
 * behaves properly when BUG is called or not.
 * In the event that EXPECT_BUG is completely broken, the call to BUG() would
 * cause the unit to crash and report a failure correctly.
 */
int test_hamming_weight(struct unit_module *m,
				struct gk20a *g, void *args)
{
	unsigned int result;
	unsigned int i;
	uint8_t hwt_8bit;
	uint16_t hwt_16bit;
	uint32_t hwt_32bit;
	uint64_t hwt_64bit;

	for (i = 0; i < 8; i++) {
		hwt_8bit = (unsigned int) 1 << i;
		result = nvgpu_posix_hweight8(hwt_8bit);
		if (result != 1) {
			unit_return_fail(m,
				"8 bit hwt failed for %d\n", hwt_8bit);
		}
	}

	for (i = 0; i < 16; i++) {
		hwt_16bit = (unsigned int) 1 << i;
		result = nvgpu_posix_hweight16(hwt_16bit);
		if (result != 1) {
			unit_return_fail(m,
				"16 bit hwt failed for %d\n", hwt_16bit);
		}
	}

	for (i = 0; i < 32; i++) {
		hwt_32bit = (unsigned int) 1 << i;
		result = nvgpu_posix_hweight32(hwt_32bit);
		if (result != 1) {
			unit_return_fail(m,
				"32 bit hwt failed for %d\n", hwt_32bit);
		}
	}

	for (i = 0; i < 64; i++) {
		hwt_64bit = (unsigned long) 1 << i;
		result = nvgpu_posix_hweight64(hwt_64bit);
		if (result != 1) {
			unit_return_fail(m,
				"64 bit hwt failed for %lx\n", hwt_64bit);
		}
	}

	hwt_8bit = 0x0;
	result = nvgpu_posix_hweight8(hwt_8bit);
	if (result != 0) {
		unit_return_fail(m,
			"8 bit hwt failed for %d\n", hwt_8bit);
	}

	hwt_8bit = 0xff;
	result = nvgpu_posix_hweight8(hwt_8bit);
	if (result != 8) {
		unit_return_fail(m,
			"8 bit hwt failed for %d\n", hwt_8bit);
	}

	hwt_16bit = 0x0;
	result = nvgpu_posix_hweight16(hwt_16bit);
	if (result != 0) {
		unit_return_fail(m,
			"16 bit hwt failed for %d\n", hwt_16bit);
	}

	hwt_16bit = 0xffff;
	result = nvgpu_posix_hweight16(hwt_16bit);
	if (result != 16) {
		unit_return_fail(m,
			"16 bit hwt failed for %d\n", hwt_16bit);
	}

	hwt_32bit = 0x0;
	result = nvgpu_posix_hweight32(hwt_32bit);
	if (result != 0) {
		unit_return_fail(m,
			"32 bit hwt failed for %d\n", hwt_32bit);
	}

	hwt_32bit = 0xffffffff;
	result = nvgpu_posix_hweight32(hwt_32bit);
	if (result != 32) {
		unit_return_fail(m,
			"32 bit hwt failed for %d\n", hwt_32bit);
	}

	hwt_64bit = 0x0;
	result = nvgpu_posix_hweight64(hwt_64bit);
	if (result != 0) {
		unit_return_fail(m,
			"64 bit hwt failed for %ld\n", hwt_64bit);
	}

	hwt_64bit = 0xffffffffffffffff;
	result = nvgpu_posix_hweight64(hwt_64bit);
	if (result != 64) {
		unit_return_fail(m,
			"64 bit hwt failed for %ld\n", hwt_64bit);
	}

	return UNIT_SUCCESS;
}

int test_be32tocpu(struct unit_module *m,
			struct gk20a *g, void *args)
{
	uint32_t pattern;
	uint32_t result;
	uint8_t *ptr;

	pattern = 0xaabbccdd;

	ptr = (uint8_t*)&pattern;

	result = be32_to_cpu(pattern);

	if (*ptr == 0xdd) {
		if (result != 0xddccbbaa) {
			unit_return_fail(m,
				"be32tocpu failed for %x %x\n",
				pattern, result);
		}
	}

	return UNIT_SUCCESS;
}
struct unit_module_test posix_utils_tests[] = {
	UNIT_TEST(hweight_test, test_hamming_weight, NULL, 0),
	UNIT_TEST(be32tocpu_test, test_be32tocpu, NULL, 0),
};

UNIT_MODULE(posix_utils, posix_utils_tests, UNIT_PRIO_POSIX_TEST);
