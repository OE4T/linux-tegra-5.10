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

#include <stdlib.h>
#include <stdbool.h>
#include <nvgpu/posix/posix-fault-injection.h>

void nvgpu_posix_init_fault_injection(struct nvgpu_posix_fault_inj *fi)
{
	fi->enabled = false;
	fi->counter = 0U;
}

void nvgpu_posix_enable_fault_injection(struct nvgpu_posix_fault_inj *fi,
					bool enable, unsigned int number)
{
	if (number == 0U) {
		fi->enabled = enable;
		fi->counter = 0U;
	} else {
		fi->enabled = !enable;
		fi->counter = number;
	}
}

bool nvgpu_posix_is_fault_injection_triggered(struct nvgpu_posix_fault_inj *fi)
{
	return fi->enabled;
}

/*
 * Return status of fault injection.
 * Decrement fault injection count for each call.
 */
bool nvgpu_posix_fault_injection_handle_call(struct nvgpu_posix_fault_inj *fi)
{
	bool current_state = fi->enabled;

	if (fi->counter > 0U) {
		fi->counter--;
		if (fi->counter == 0U) {
			/* for next time */
			fi->enabled = !fi->enabled;
		}
	}

	return current_state;
}
