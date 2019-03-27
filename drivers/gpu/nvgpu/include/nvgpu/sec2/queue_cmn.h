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

#ifndef NVGPU_SEC2_CMN_H
#define NVGPU_SEC2_CMN_H

/*
 * Defines the logical queue IDs that must be used when submitting commands
 * to or reading messages from SEC2. The identifiers must begin with zero and
 * should increment sequentially. _CMDQ_LOG_ID__LAST must always be set to the
 * last command queue identifier. _NUM must always be set to the last
 * identifier plus one.
 */
#define SEC2_NV_CMDQ_LOG_ID			0U
#define SEC2_NV_CMDQ_LOG_ID__LAST	0U
#define SEC2_NV_MSGQ_LOG_ID			1U
#define SEC2_QUEUE_NUM				2U

#endif /* NVGPU_SEC2_CMN_H */
