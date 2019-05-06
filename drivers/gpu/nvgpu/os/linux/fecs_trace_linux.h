/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_FECS_TRACE_LINUX_H
#define NVGPU_FECS_TRACE_LINUX_H

#include <nvgpu/types.h>

#define GK20A_CTXSW_TRACE_NUM_DEVS			1
#define GK20A_CTXSW_TRACE_MAX_VM_RING_SIZE	(128*PAGE_SIZE)

struct file;
struct inode;
struct gk20a;
struct nvgpu_tsg;
struct nvgpu_channel;
struct vm_area_struct;
struct poll_table_struct;

int gk20a_ctxsw_trace_init(struct gk20a *g);
void gk20a_ctxsw_trace_cleanup(struct gk20a *g);

int gk20a_ctxsw_dev_mmap(struct file *filp, struct vm_area_struct *vma);

int gk20a_ctxsw_dev_release(struct inode *inode, struct file *filp);
int gk20a_ctxsw_dev_open(struct inode *inode, struct file *filp);
long gk20a_ctxsw_dev_ioctl(struct file *filp,
			 unsigned int cmd, unsigned long arg);
ssize_t gk20a_ctxsw_dev_read(struct file *filp, char __user *buf,
			     size_t size, loff_t *offs);
unsigned int gk20a_ctxsw_dev_poll(struct file *filp,
				  struct poll_table_struct *pts);

#endif /*NVGPU_FECS_TRACE_LINUX_H */
