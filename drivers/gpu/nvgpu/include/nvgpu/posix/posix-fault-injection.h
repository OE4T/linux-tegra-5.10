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

#ifndef NVGPU_POSIX_FAULT_INJECTION_H
#define NVGPU_POSIX_FAULT_INJECTION_H

struct nvgpu_posix_fault_inj {
	bool enabled;
	unsigned int counter;
};

/**
 * nvgpu_posix_init_fault_injection - Initialize the fault injection object
 * to be used by a module that will report errors.
 *
 * @fi - pointer to the fault_inj object to initialize
 */
void nvgpu_posix_init_fault_injection(struct nvgpu_posix_fault_inj *fi);


/**
 * nvgpu_posix_enable_fault_injection - Enable/Disable fault injection for the
 *                                      object after @number calls to the
 *                                      module. This depends on
 *                                      nvgpu_posix_fault_injection_handle_call
 *                                      being called by each function in the
 *                                      module that can fault. Only routines
 *                                      that can fault will decrement the
 *                                      delay count.
 *
 * @fi - pointer to the fault_inj object.
 * @enable - true to enable.  false to disable.
 * @number - After <number> of calls to kmem alloc or cache routines, enable or
 *           disable fault injection. Use 0 to enable/disable immediately.
 */
void nvgpu_posix_enable_fault_injection(struct nvgpu_posix_fault_inj *fi,
					bool enable, unsigned int number);


/**
 * nvgpu_posix_is_fault_injection_triggered - Query if fault injection is
 *                                            currently enabled for the @fi
 *                                            object.
 *
 * @fi - pointer to the fault_inj object
 *
 * Returns true if fault injections are enabled.
 */
bool nvgpu_posix_is_fault_injection_triggered(struct nvgpu_posix_fault_inj *fi);

/**
 * nvgpu_posix_fault_injection_handle_call - Called by module functions to
 *                                           track enabling or disabling fault
 *                                           injections. Returns true if the
 *                                           module should return an error.
 *
 * @fi - pointer to the fault_inj object
 *
 * Returns true if the module should return an error.
 */
bool nvgpu_posix_fault_injection_handle_call(struct nvgpu_posix_fault_inj *fi);

#endif /* NVGPU_POSIX_FAULT_INJECTION_H */
