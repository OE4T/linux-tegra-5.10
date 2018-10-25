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

#ifndef __UNIT_FAULT_INJECTION_KMEM_H__
#define __UNIT_FAULT_INJECTION_KMEM_H__

int test_kmem_init(struct unit_module *m,
		   struct gk20a *g, void *__args);

int test_kmem_cache_fi_default(struct unit_module *m,
			       struct gk20a *g, void *__args);
int test_kmem_cache_fi_enabled(struct unit_module *m,
			       struct gk20a *g, void *__args);
int test_kmem_cache_fi_delayed_enable(struct unit_module *m,
			       struct gk20a *g, void *__args);
int test_kmem_cache_fi_delayed_disable(struct unit_module *m,
			       struct gk20a *g, void *__args);

int test_kmem_kmalloc_fi_default(struct unit_module *m,
			       struct gk20a *g, void *__args);
int test_kmem_kmalloc_fi_enabled(struct unit_module *m,
			       struct gk20a *g, void *__args);
int test_kmem_kmalloc_fi_delayed_enable(struct unit_module *m,
			       struct gk20a *g, void *__args);
int test_kmem_kmalloc_fi_delayed_disable(struct unit_module *m,
			       struct gk20a *g, void *__args);

#endif /* __UNIT_FAULT_INJECTION_KMEM_H__ */
