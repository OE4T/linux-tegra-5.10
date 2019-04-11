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

#ifndef NVGPU_CLASS_H
#define NVGPU_CLASS_H

#define	FERMI_TWOD_A			0x902DU
#define	KEPLER_INLINE_TO_MEMORY_A	0xA040U
#define	KEPLER_DMA_COPY_A		0xA0B5U

#define	MAXWELL_B			0xB197U
#define	MAXWELL_COMPUTE_B		0xB1C0U
#define	KEPLER_INLINE_TO_MEMORY_B	0xA140U
#define	MAXWELL_DMA_COPY_A		0xB0B5U
#define	MAXWELL_CHANNEL_GPFIFO_A	0xB06FU

#define	PASCAL_CHANNEL_GPFIFO_A		0xC06FU
#define	PASCAL_A			0xC097U
#define	PASCAL_COMPUTE_A		0xC0C0U
#define	PASCAL_DMA_COPY_A		0xC0B5U
#define	PASCAL_DMA_COPY_B		0xC1B5U

#define PASCAL_B		0xC197U
#define	PASCAL_COMPUTE_B	0xC1C0U

#define	VOLTA_CHANNEL_GPFIFO_A	0xC36FU
#define	VOLTA_A			0xC397U
#define	VOLTA_COMPUTE_A		0xC3C0U
#define	VOLTA_DMA_COPY_A	0xC3B5U

#define	TURING_CHANNEL_GPFIFO_A	0xC46FU
#define	TURING_A		0xC597U
#define	TURING_COMPUTE_A	0xC5C0U
#define	TURING_DMA_COPY_A	0xC5B5U

#endif /* NVGPU_CLASS_H */
