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

/**
 * @file
 * @page NVGPU-SWUTS
 *
 * NVGPU Software Unit Test Specifications
 * =======================================
 *
 * The following pages cover the various unit test specifications needed
 * to test the NVGPU driver.
 *
 *   - @ref SWUTS-enabled
 *   - @ref SWUTS-interface-bsearch
 *   - @ref SWUTS-interface-lock
 *   - @ref SWUTS-interface-rbtree
 *   - @ref SWUTS-falcon
 *   - @ref SWUTS-netlist
 *   - @ref SWUTS-fifo
 *   - @ref SWUTS-fifo-channel
 *   - @ref SWUTS-fifo-runlist
 *   - @ref SWUTS-fifo-tsg
 *   - @ref SWUTS-fifo-tsg-gv11b
 *   - @ref SWUTS-init
 *   - @ref SWUTS-interface-atomic
 *   - @ref SWUTS-ltc
 *   - @ref SWUTS-mm-allocators-bitmap-allocator
 *   - @ref SWUTS-mm-allocators-buddy-allocator
 *   - @ref SWUTS-mm-as
 *   - @ref SWUTS-mm-nvgpu-mem
 *   - @ref SWUTS-mm-nvgpu-sgt
 *   - @ref SWUTS-mm-mm
 *   - @ref SWUTS-mm-vm
 *   - @ref SWUTS-fuse
 *   - @ref SWUTS-posix-bitops
 *   - @ref SWUTS-posix-cond
 *   - @ref SWUTS-posix-fault-injection
 *   - @ref SWUTS-posix-sizes
 *   - @ref SWUTS-posix-thread
 *   - @ref SWUTS-posix-timers
 *   - @ref SWUTS-ptimer
 *   - @ref SWUTS-sdl
 *   - @ref SWUTS-acr
 *   - @ref SWUTS-cg
 *   - @ref SWUTS-init_test
 *   - @ref SWUTS-channel_os
 *
 */

