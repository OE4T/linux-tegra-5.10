/*
 * Copyright (c) 2018-2022, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/vm.h>
#include <nvgpu/bug.h>

#include <nvgpu/posix/vm.h>

u64 nvgpu_os_buf_get_size(struct nvgpu_os_buffer *os_buf)
{
	return os_buf->size;
}

struct nvgpu_mapped_buf *nvgpu_vm_find_mapping(struct vm_gk20a *vm,
					       struct nvgpu_os_buffer *os_buf,
					       u64 map_addr,
					       u32 flags,
					       s16 kind)
{
	struct nvgpu_mapped_buf *mapped_buffer = NULL;

	(void)os_buf;
	(void)kind;

	mapped_buffer = nvgpu_vm_find_mapped_buf(vm, map_addr);
	if (mapped_buffer == NULL) {
		return NULL;
	}

	if (mapped_buffer->flags != flags) {
		return NULL;
	}

	return mapped_buffer;
}

void nvgpu_vm_unmap_system(struct nvgpu_mapped_buf *mapped_buffer)
{
	free(mapped_buffer->os_priv.buf);
}
