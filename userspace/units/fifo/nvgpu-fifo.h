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
#ifndef UNIT_NVGPU_FIFO_H
#define UNIT_NVGPU_FIFO_H

#include <nvgpu/types.h>

#ifdef UNIT_FIFO_DEBUG
#define unit_verbose	unit_info
#else
#define unit_verbose(unit, msg, ...) \
	do { \
		if (0) \
			{ unit_info(unit, msg, ##__VA_ARGS__); \
		} \
	} while (0)
#endif

int test_fifo_init_support(struct unit_module *m,
		struct gk20a *g, void *args);
int test_fifo_remove_support(struct unit_module *m,
		struct gk20a *g, void *args);

#endif /* UNIT_NVGPU_FIFO_H */
