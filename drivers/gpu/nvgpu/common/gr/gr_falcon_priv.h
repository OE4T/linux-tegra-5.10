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

#ifndef GR_FALCON_PRIV_H
#define GR_FALCON_PRIV_H

#include <nvgpu/types.h>
#include <nvgpu/nvgpu_mem.h>

struct nvgpu_ctxsw_ucode_segments;

struct nvgpu_fecs_method_op {
	struct {
		u32 addr;
		u32 data;
	} method;

	struct {
		u32 id;
		u32 data;
		u32 clr;
		u32 *ret;
		u32 ok;
		u32 fail;
	} mailbox;

	struct {
		u32 ok;
		u32 fail;
	} cond;

};

struct nvgpu_ctxsw_bootloader_desc {
	u32 start_offset;
	u32 size;
	u32 imem_offset;
	u32 entry_point;
};

struct nvgpu_ctxsw_ucode_info {
	u64 *p_va;
	struct nvgpu_mem inst_blk_desc;
	struct nvgpu_mem surface_desc;
	struct nvgpu_ctxsw_ucode_segments fecs;
	struct nvgpu_ctxsw_ucode_segments gpccs;
};

struct nvgpu_gr_falcon_query_sizes {
	u32 golden_image_size;
	u32 pm_ctxsw_image_size;
	u32 preempt_image_size;
	u32 zcull_image_size;
};

struct nvgpu_gr_falcon {
	struct nvgpu_ctxsw_ucode_info ctxsw_ucode_info;
	struct nvgpu_mutex fecs_mutex; /* protect fecs method */
	bool skip_ucode_init;

	struct nvgpu_gr_falcon_query_sizes sizes;
};

enum wait_ucode_status {
	WAIT_UCODE_LOOP,
	WAIT_UCODE_TIMEOUT,
	WAIT_UCODE_ERROR,
	WAIT_UCODE_OK
};

enum {
	GR_IS_UCODE_OP_EQUAL,
	GR_IS_UCODE_OP_NOT_EQUAL,
	GR_IS_UCODE_OP_AND,
	GR_IS_UCODE_OP_LESSER,
	GR_IS_UCODE_OP_LESSER_EQUAL,
	GR_IS_UCODE_OP_SKIP
};

enum {
	eUcodeHandshakeInitComplete = 1,
	eUcodeHandshakeMethodFinished
};

/* sums over the ucode files as sequences of u32, computed to the
 * boot_signature field in the structure above */

/* T18X FECS remains same as T21X,
 * so FALCON_UCODE_SIG_T21X_FECS_WITH_RESERVED used
 * for T18X*/
#define FALCON_UCODE_SIG_T18X_GPCCS_WITH_RESERVED	0x68edab34U
#define FALCON_UCODE_SIG_T21X_FECS_WITH_DMEM_SIZE	0x9121ab5cU
#define FALCON_UCODE_SIG_T21X_FECS_WITH_RESERVED	0x9125ab5cU
#define FALCON_UCODE_SIG_T12X_FECS_WITH_RESERVED	0x8a621f78U
#define FALCON_UCODE_SIG_T12X_FECS_WITHOUT_RESERVED	0x67e5344bU
#define FALCON_UCODE_SIG_T12X_FECS_OLDER		0x56da09fU

#define FALCON_UCODE_SIG_T21X_GPCCS_WITH_RESERVED	0x3d3d65e2U
#define FALCON_UCODE_SIG_T12X_GPCCS_WITH_RESERVED	0x303465d5U
#define FALCON_UCODE_SIG_T12X_GPCCS_WITHOUT_RESERVED	0x3fdd33d3U
#define FALCON_UCODE_SIG_T12X_GPCCS_OLDER		0x53d7877U

#define FALCON_UCODE_SIG_T21X_FECS_WITHOUT_RESERVED	0x93671b7dU
#define FALCON_UCODE_SIG_T21X_FECS_WITHOUT_RESERVED2	0x4d6cbc10U

#define FALCON_UCODE_SIG_T21X_GPCCS_WITHOUT_RESERVED	0x393161daU


#endif /* GR_FALCON_PRIV_H */

