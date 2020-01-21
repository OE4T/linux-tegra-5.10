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
#include <unistd.h>

#include <unit/io.h>
#include <unit/unit.h>

#include <nvgpu/types.h>
#include <nvgpu/posix/queue.h>
#include <nvgpu/posix/kmem.h>
#include <nvgpu/posix/posix-fault-injection.h>

#include "posix-queue.h"

#define QUEUE_LEN		10
#define QUEUE_LEN_POW_2		16
#define BUF_LEN			10

int test_nvgpu_queue_alloc_and_free(struct unit_module *m, struct gk20a *g,
				void *args)
{
	int ret = 0, err = UNIT_SUCCESS;
	struct nvgpu_queue q = {0};
	struct nvgpu_posix_fault_inj *kmem_fi =
				nvgpu_kmem_get_fault_injection();

	ret = nvgpu_queue_alloc(NULL, QUEUE_LEN);
	if (ret != -EINVAL) {
		err = UNIT_FAIL;
		unit_err(m, "%d. test_queue_alloc failed err=%d\n", __LINE__,
			ret);
		goto fail;
	}

	ret = nvgpu_queue_alloc(&q, 0);
	if (ret != -EINVAL) {
		err = UNIT_FAIL;
		unit_err(m, "%d. test_queue_alloc failed err=%d\n", __LINE__,
			ret);
		goto fail;
	}

	ret = nvgpu_queue_alloc(&q, (unsigned int)INT64_MAX);
	if (ret != -EINVAL) {
		err = UNIT_FAIL;
		unit_err(m, "%d. test_queue_alloc failed err=%d\n", __LINE__,
			ret);
		goto fail;
	}

	nvgpu_posix_enable_fault_injection(kmem_fi, true, 0);
	ret = nvgpu_queue_alloc(&q, QUEUE_LEN_POW_2);
	if (ret != -ENOMEM) {
		nvgpu_posix_enable_fault_injection(kmem_fi, false, 0);
		err = UNIT_FAIL;
		unit_err(m, "%d. test_queue_alloc failed err=%d\n", __LINE__,
			ret);
		goto fail;
	}
	nvgpu_posix_enable_fault_injection(kmem_fi, false, 0);

	ret = nvgpu_queue_alloc(&q, QUEUE_LEN);
	if (ret != 0) {
		err = UNIT_FAIL;
		unit_err(m, "%d. test_queue_alloc failed err=%d\n", __LINE__,
			ret);
		goto fail;
	}
	nvgpu_queue_free(&q);

	ret = nvgpu_queue_alloc(&q, QUEUE_LEN_POW_2);
	if (ret != 0) {
		err = UNIT_FAIL;
		unit_err(m, "%d. test_queue_alloc failed err=%d\n", __LINE__,
			ret);
		goto fail;
	}
	nvgpu_queue_free(&q);

fail:
	return err;
}

int test_nvgpu_queue_in(struct unit_module *m, struct gk20a *g, void *args)
{
	int ret = 0, err = UNIT_SUCCESS;
	struct nvgpu_queue q = {0};
	struct nvgpu_mutex lock;
	char buf[BUF_LEN];

	nvgpu_mutex_init(&lock);

	/* Allocate Queue of size QUEUE_LEN_POW_2 */
	ret = nvgpu_queue_alloc(&q, QUEUE_LEN_POW_2);
	if (ret != 0) {
		err = UNIT_FAIL;
		unit_err(m, "%d. queue_alloc failed err=%d\n", __LINE__,  ret);
		goto fail;
	}

	/* Enqueue message of length BUF_LEN */
	ret = nvgpu_queue_in(&q, buf, BUF_LEN);
	if (ret != BUF_LEN) {
		err = UNIT_FAIL;
		unit_err(m, "%d. queue_in failed err=%d\n", __LINE__,  ret);
		goto fail;
	}

	/*
	 * Update "in" and "out" indexes and enqueue message of length BUF_LEN
	 * such that we wrap around the Queue while enqueuing the message.
	 */
	q.in = BUF_LEN;
	q.out = BUF_LEN;
	ret = nvgpu_queue_in(&q, buf, BUF_LEN);
	if (ret != BUF_LEN) {
		err = UNIT_FAIL;
		unit_err(m, "%d. queue_in failed err=%d\n", __LINE__,  ret);
		goto fail;
	}

	/*
	 * Reset "in" and "out" indexes and enqueue message of length BUF_LEN
	 * with the lock.
	 */
	q.in = 0;
	q.out = 0;
	ret = nvgpu_queue_in_locked(&q, buf, BUF_LEN, &lock);
	if (ret != BUF_LEN) {
		err = UNIT_FAIL;
		unit_err(m, "%d. queue_in failed err=%d\n", __LINE__,  ret);
		goto fail;
	}

	/* Enqueue message of length BUF_LEN again and expect memory full */
	ret = nvgpu_queue_in_locked(&q, buf, BUF_LEN, &lock);
	if (ret != -ENOMEM) {
		err = UNIT_FAIL;
		unit_err(m, "%d. queue_in failed err=%d\n", __LINE__,  ret);
		goto fail;
	}

	/* Enqueue message of length BUF_LEN again (without lock) and expect
	 * memory full.
	 */
	ret = nvgpu_queue_in(&q, buf, BUF_LEN);
	if (ret != -ENOMEM) {
		err = UNIT_FAIL;
		unit_err(m, "%d. queue_in failed err=%d\n", __LINE__,  ret);
		goto fail;
	}

fail:
	if (q.data != NULL)
		free(q.data);
	nvgpu_mutex_destroy(&lock);

	return err;
}

int test_nvgpu_queue_out(struct unit_module *m, struct gk20a *g, void *args)
{
	int ret = 0, err = UNIT_SUCCESS;
	struct nvgpu_queue q = {0};
	struct nvgpu_mutex lock;
	char buf[BUF_LEN];

	nvgpu_mutex_init(&lock);

	/* Allocate Queue of size QUEUE_LEN_POW_2 */
	ret = nvgpu_queue_alloc(&q, QUEUE_LEN_POW_2);
	if (ret != 0) {
		err = UNIT_FAIL;
		unit_err(m, "%d. queue_alloc failed err=%d\n", __LINE__,  ret);
		goto fail;
	}

	/* Queue is empty. Dequeue message should return "-ENOMEM" */
	ret = nvgpu_queue_out(&q, buf, BUF_LEN);
	if (ret != -ENOMEM) {
		err = UNIT_FAIL;
		unit_err(m, "%d. queue_out failed err=%d\n", __LINE__,  ret);
		goto fail;
	}

	/* Queue is empty. Dequeue message with lock should return "-ENOMEM" */
	ret = nvgpu_queue_out_locked(&q, buf, BUF_LEN, &lock);
	if (ret != -ENOMEM) {
		err = UNIT_FAIL;
		unit_err(m, "%d. queue_out failed err=%d\n", __LINE__,  ret);
		goto fail;
	}

	/*
	 * Advance "in" index by "BUF_LEN" and dequeue message of length BUF_LEN
	 */
	q.in = BUF_LEN;
	q.out = 0;
	ret = nvgpu_queue_out(&q, buf, BUF_LEN);
	if (ret != BUF_LEN) {
		err = UNIT_FAIL;
		unit_err(m, "%d. queue_out failed err=%d\n", __LINE__,  ret);
		goto fail;
	}

	/*
	 * Advance "in" index by "BUF_LEN" and dequeue message of length BUF_LEN
	 * with the lock.
	 */
	q.in = BUF_LEN;
	q.out = 0;
	ret = nvgpu_queue_out_locked(&q, buf, BUF_LEN, &lock);
	if (ret != BUF_LEN) {
		err = UNIT_FAIL;
		unit_err(m, "%d. queue_out failed err=%d\n", __LINE__,  ret);
		goto fail;
	}

	/*
	 * Update "in" and "out" indexes and dequeue message of length BUF_LEN
	 * such that we wrap around the Queue while dequeuing the message.
	 */
	q.in = 1;
	q.out = QUEUE_LEN_POW_2 -(BUF_LEN -1);
	ret = nvgpu_queue_out(&q, buf, BUF_LEN);
	if (ret != BUF_LEN) {
		err = UNIT_FAIL;
		unit_err(m, "%d. queue_out failed err=%d\n", __LINE__,  ret);
		goto fail;
	}

fail:
	if (q.data != NULL)
		free(q.data);
	nvgpu_mutex_destroy(&lock);

	return err;
}

struct unit_module_test posix_queue_tests[] = {
	UNIT_TEST(nvgpu_queue_alloc_free, test_nvgpu_queue_alloc_and_free,
		NULL, 0),
	UNIT_TEST(nvgpu_queue_in, test_nvgpu_queue_in, NULL, 0),
	UNIT_TEST(nvgpu_queue_out, test_nvgpu_queue_out, NULL, 0),
};

UNIT_MODULE(posix_queue, posix_queue_tests, UNIT_PRIO_POSIX_TEST);
