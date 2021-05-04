/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_NEXT_ERRATA_H
#define NVGPU_NEXT_ERRATA_H

#define ERRATA_FLAGS_NEXT						\
	/* GA100 */							\
	DEFINE_ERRATA(NVGPU_ERRATA_200601972, "GA100", "LTC TSTG"),	\
	/* GA10B */							\
	DEFINE_ERRATA(NVGPU_ERRATA_2969956, "GA10B", "FMODEL FB LTCS"),	\
	DEFINE_ERRATA(NVGPU_ERRATA_200677649, "GA10B", "UCODE"),	\
	DEFINE_ERRATA(NVGPU_ERRATA_3154076, "GA10B", "PROD VAL"),	\
	DEFINE_ERRATA(NVGPU_ERRATA_3288192, "GA10B", "L4 SCF NOT SUPPORTED"),

#endif /* NVGPU_NEXT_ERRATA_H */
