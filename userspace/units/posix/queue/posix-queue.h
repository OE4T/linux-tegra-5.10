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

/**
 * @addtogroup SWUTS-posix-queue
 * @{
 *
 * Software Unit Test Specification for posix-queue
 */

#ifndef __UNIT_POSIX_QUEUE_H__
#define __UNIT_POSIX_QUEUE_H__

/**
 * Test specification for: test_nvgpu_queue_alloc_and_free
 *
 * Description: Functionalities of posix queue such as allocating and freeing
 * of the message queue are tested.
 *
 * Test Type: Feature, Error guessing, Boundary values
 *
 * Targets: nvgpu_queue_alloc, nvgpu_queue_free
 *
 * Input: None
 *
 * Steps:
 * - Pass NULL nvgpu_queue pointer as argument to nvgpu_queue_alloc() API and
 *   check that the API returns "-EINVAL" error.
 * - Pass zero size queue length as argument to nvgpu_queue_alloc() API and
 *   check that the API returns "-EINVAL" error.
 * - Pass "INT64_MAX" size queue length as argument to nvgpu_queue_alloc() API
 *   and check that the API returns "-EINVAL" error.
 * - Inject fault so that immediate call to nvgpu_kzalloc() API would fail.
 * - Check that when the nvgpu_queue_alloc() API is called with valid arguments,
 *   it would fail by returning "-ENOMEM" error.
 * - Remove the injected fault in nvgpu_kzalloc() API.
 * - Pass below valid arguments to nvgpu_queue_alloc() API and check that the
 *   API returns success.
 *   - Valid pointer to "struct nvgpu_queue"
 *   - Queue size which is not power of 2
 * - Free the allocated queue by caling nvgpu_queue_free() API.
 * - Pass below valid arguments to nvgpu_queue_alloc() API and check that the
 *   API returns success.
 *   - Valid pointer to "struct nvgpu_queue"
 *   - Queue size which is power of 2
 * - Free the allocated queue by caling nvgpu_queue_free() API.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_nvgpu_queue_alloc_and_free(struct unit_module *m, struct gk20a *g,
				void *args);

/**
 * Test specification for: test_nvgpu_queue_in
 *
 * Description: Functionalities of posix queue such as allocating queue and
 * enqueueing messages into the queue are tested.
 *
 * Test Type: Feature, Error guessing, Boundary values
 *
 * Targets: nvgpu_queue_alloc, nvgpu_queue_in, nvgpu_queue_in_locked,
 * nvgpu_queue_unused, nvgpu_queue_available
 *
 * Input: None
 *
 * Steps:
 * - Pass below valid arguments to nvgpu_queue_alloc() API and check that the
 *   API returns success.
 *   - Valid pointer to "struct nvgpu_queue"
 *   - Queue size which is power of 2
 * - Enqueue message of length "BUF_LEN" calling nvgpu_queue_in() API and check
 *   that the API returns "BUF_LEN".
 * - Update "in" and "out" indexes and enqueue message of length BUF_LEN such
 *   that we wrap around the Queue while enqueuing the message using
 *   nvgpu_queue_in() API. Check that the API returns "BUF_LEN".
 * - Reset "in" and "out" indexes and enqueue message of length "BUF_LEN" with
 *   the lock using nvgpu_queue_in_locked() API. Check that the API returns
 *   "BUF_LEN".
 * - Enqueue message of length "BUF_LEN" again using nvgpu_queue_in_locked()
 *   API. Check that the API returns error "-ENOMEM".
 * - Enqueue message of length "BUF_LEN" again using nvgpu_queue_in() API. Check
 *   that the API returns error "-ENOMEM".
 * - Uninitialize the allocated resources.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_nvgpu_queue_in(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_nvgpu_queue_out
 *
 * Description: Functionalities of posix queue such as allocating queue and
 * dequeuing messages from the queue are tested.
 *
 * Test Type: Feature, Error guessing, Boundary values
 *
 * Targets: nvgpu_queue_alloc, nvgpu_queue_out, nvgpu_queue_out_locked
 *
 * Input: None
 *
 * Steps:
 * - Pass below valid arguments to nvgpu_queue_alloc() API and check that the
 *   API returns success.
 *   - Valid pointer to "struct nvgpu_queue"
 *   - Queue size which is power of 2
 * - Dequeue message of length "BUF_LEN" from the empty queue calling
 *   nvgpu_queue_out() API and check that the API returns "-ENOMEM" error.
 * - Dequeue message of length "BUF_LEN" from the empty queue calling
 *   nvgpu_queue_out_locked() API and check that the API returns "-ENOMEM"
 *   error.
 * - Advance "in" index by "BUF_LEN" and dequeue message of length BUF_LEN by
 *   calling nvgpu_queue_out() API and check that the API returns "BUF_LEN".
 * - Advance "in" index by "BUF_LEN" and dequeue message of length BUF_LEN by
 *   calling nvgpu_queue_out_locked() API and check that the API returns
 *   "BUF_LEN".
 * - Update "in" and "out" indexes and dequeue message of length BUF_LEN such
 *   that we wrap around the Queue while dequeuing the message using
 *   nvgpu_queue_out() API. Check that the API returns "BUF_LEN".
 * - Do fault injection so that immediate call to nvgpu_queue_out_locked() API
 *   would return error.
 * - Invoke nvgpu_queue_out_locked() API and check that API returns -1 error.
 * - Remove the injected fault.
 * - Uninitialize the allocated resources.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_nvgpu_queue_out(struct unit_module *m, struct gk20a *g, void *args);

#endif /* __UNIT_POSIX_QUEUE_H__ */
/*
 * @}
 */
