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

#include <nvgpu/bug.h>
#include <nvgpu/comptags.h>

#include <nvgpu/posix/vm.h>

void gk20a_get_comptags(struct nvgpu_os_buffer *buf,
			struct gk20a_comptags *comptags)
{
	(void)buf;
	(void)comptags;
}

int gk20a_alloc_comptags(struct gk20a *g, struct nvgpu_os_buffer *buf,
			 struct gk20a_comptag_allocator *allocator)
{
	(void)g;
	(void)buf;
	(void)allocator;
	return -ENODEV;
}

void gk20a_alloc_or_get_comptags(struct gk20a *g,
				 struct nvgpu_os_buffer *buf,
				 struct gk20a_comptag_allocator *allocator,
				 struct gk20a_comptags *comptags)
{
	(void)g;
	(void)buf;
	(void)allocator;
	(void)comptags;
}

bool gk20a_comptags_start_clear(struct nvgpu_os_buffer *buf)
{
	(void)buf;
	return false;
}

void gk20a_comptags_finish_clear(struct nvgpu_os_buffer *buf,
				 bool clear_successful)
{
	(void)buf;
	(void)clear_successful;
}
