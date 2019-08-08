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

#ifndef NVGPU_FILE_OPS_H
#define NVGPU_FILE_OPS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <nvgpu/types.h>

#ifdef NVGPU_UNITTEST_FAULT_INJECTION_ENABLEMENT
#include <nvgpu/posix/posix-fault-injection.h>
#endif

#ifdef NVGPU_UNITTEST_FAULT_INJECTION_ENABLEMENT
struct nvgpu_posix_fault_inj *nvgpu_file_ops_get_fstat_injection(void);
struct nvgpu_posix_fault_inj *nvgpu_file_ops_get_fread_injection(void);
#endif

int nvgpu_fstat(int fd, struct stat *buf);
ssize_t nvgpu_fread(int fildes, void* buf, size_t nbytes);
void nvgpu_close(int fd);
#endif /* NVGPU_FILE_OPS_H */

