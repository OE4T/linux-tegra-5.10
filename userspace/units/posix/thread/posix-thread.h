/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
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
 * @addtogroup SWUTS-posix-thread
 * @{
 *
 * Software Unit Test Specification for posix-thread
 */
#ifndef __UNIT_POSIX_THREAD_H__
#define __UNIT_POSIX_THREAD_H__

#include <nvgpu/thread.h>

#define UNIT_TEST_THREAD_PRIORITY 5

struct test_thread_args {
        bool use_priority;
        bool check_stop;
        bool stop_graceful;
        bool use_name;
        bool stop_repeat;
};

static struct test_thread_args create_normal = {
        .use_priority = false,
        .check_stop = false,
        .stop_graceful = false,
	.use_name = true,
	.stop_repeat = false
};

static struct test_thread_args create_normal_noname = {
        .use_priority = false,
        .check_stop = false,
        .stop_graceful = false,
	.use_name = false,
	.stop_repeat = false
};

static struct test_thread_args create_priority = {
        .use_priority = true,
        .check_stop = false,
        .stop_graceful = false,
	.use_name = true,
	.stop_repeat = false
};

static struct test_thread_args create_priority_noname = {
        .use_priority = true,
        .check_stop = false,
        .stop_graceful = false,
	.use_name = false,
	.stop_repeat = false
};

static struct test_thread_args check_stop = {
        .use_priority = false,
        .check_stop = true,
        .stop_graceful = false,
	.use_name = true,
	.stop_repeat = false
};

static struct test_thread_args stop_graceful = {
        .use_priority = false,
        .check_stop = true,
        .stop_graceful = true,
	.use_name = true,
	.stop_repeat = false
};

static struct test_thread_args stop_graceful_repeat = {
        .use_priority = false,
        .check_stop = true,
        .stop_graceful = true,
	.use_name = true,
	.stop_repeat = true
};

struct unit_test_thread_data {
        int thread_created;
        int check_priority;
        int thread_priority;
        int check_stop;
        int callback_invoked;
};

struct nvgpu_thread test_thread;
struct unit_test_thread_data test_data;

/**
 * Test specification for test_thread_cycle
 *
 * Description: Test the various functionalities provided by Threads unit.
 * Main functionalities that has to be tested in Threads unit are as follows,
 * 1) Thread creation
 * 2) Thread creation with a priority value
 * 3) Thread stop
 * 4) Stop thread gracefully
 * test_thread_cycle function tests all the above mentioned functionalities
 * based on the input arguments.
 *
 * Test Type: Feature
 *
 * Targets: nvgpu_thread_create, nvgpu_thread_create_priority,
 *          nvgpu_thread_is_running, nvgpu_thread_stop,
 *          nvgpu_thread_stop_graceful, nvgpu_thread_should_stop,
 *          nvgpu_thread_join
 * Inputs:
 * 1) Pointer to test_thread_args as function parameter
 * 2) Global instance of struct nvgpu_thread
 * 3) Global instance of struct unit_test_thread_data
 *
 * Steps:
 * Thread creation
 * 1) Reset all global and shared variables to 0.
 * 2) Create thread using nvgpu_thread_create.
 * 3) Check the return value from nvgpu_thread_create for error.
 * 4) Wait for the thread to be created by polling for a shared variable.
 * 5) Return Success once the thread function is called and the shared
 * variable is set which indicates a succesful thread creation.
 *
 * Thread creation with a priority value
 * 1) Reset all global and shared variables to 0.
 * 2) Create thread using nvgpu_thread_create_priority
 * 3) Check the return value from nvgpu_thread_create_priority for error.
 * 4) Wait for the thread to be created by polling for a shared variable.
 * 5) Upon succesful creation of the thread, confirm the priority of the
 * thread to be same as requested priority.
 * 6) In some host machines, permission is not granted to create threads
 * with priority.  In that case skip the test by returning PASS.
 * 7) Return PASS if the thread is created with requested priority.
 *
 * Thread stop
 * 1) Follow steps 1 - 4 of Thread creation scenario.
 * 2) The created thread does not exit unconditionally in this case.
 * 3) It polls for the stop flag to be set.
 * 4) The main thread checks the status of the created thread and confirms
 * it to be running.
 * 5) Request the thread to stop by calling nvgpu_thread_stop.
 * 6) Created thread detects this inside the poll loop and exits.
 * 7) Main thread continues once the created thread exits and returns PASS.
 *
 * Stop thread gracefully
 * 1) Follow steps 1 -  4 of Thread stop scenario.
 * 2) Call the api nvgpu_thread_stop_graceful and pass the function to be
 * called for graceful exit.
 * 3) Created thread detects the stop request and exits.
 * 4) Main thread continues after the created thread exits and
 * confirms if the call back function was called by checking a shared variable.
 * 5) Main thread returns PASS is step 4 passes, else returns FAIL.
 *
 * Output:
 * The output for each test scenario is as follows,
 * 1) Thread creation
 * Return PASS if thread creation is succesful else return FAIL.
 *
 * 2) Thread creation with a priority value
 * Return PASS if thread creation with priority is succesful
 * else return FAIL.  ALso return PASS if permission is denied for creating
 * a thread with priority.
 *
 * 3) Thread stop
 * Return PASS if the created thread is stopped based on the request from
 * main thread.  Else return FAIL.
 *
 * 4) Stop thread gracefully
 * Return PASS if the callback function is called and the created thread is
 * stopped based on the request from main thread.
 *
 */
int test_thread_cycle(struct unit_module *m,
		struct gk20a *g, void *args);

#endif /* __UNIT_POSIX_THREAD_H__ */
