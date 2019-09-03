#ifndef _DT_BINDINGS_MEMORY_TEGRA194_SWGROUP_H
#define _DT_BINDINGS_MEMORY_TEGRA194_SWGROUP_H
/*
 * Copyright (c) 2017-2018, NVIDIA CORPORATION. All rights reserved.
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

/*
 * This is the t19x specific component of the new SID dt-binding.
 */
#define TEGRA_SID_RCE		0x2aU	/* 42 */
#define TEGRA_SID_RCE_VM2	0x2bU	/* 43 */

#define TEGRA_SID_RCE_RM	0x2FU	/* 47 */
#define TEGRA_SID_VIFALC	0x30U	/* 48 */
#define TEGRA_SID_ISPFALC	0x31U	/* 49 */

#define TEGRA_SID_MIU		0x50U	/* 80 */

#define TEGRA_SID_NVDLA0	0x51U	/* 81 */
#define TEGRA_SID_NVDLA1	0x52U	/* 82 */

#define TEGRA_SID_PVA0		0x53U	/* 83 */
#define TEGRA_SID_PVA1		0x54U	/* 84 */

#define TEGRA_SID_NVENC1	0x55U	/* 85 */

#define TEGRA_SID_PCIE0		0x56U	/* 86 */
#define TEGRA_SID_PCIE1		0x57U	/* 87 */
#define TEGRA_SID_PCIE2		0x58U	/* 88 */
#define TEGRA_SID_PCIE3		0x59U	/* 89 */
#define TEGRA_SID_PCIE4		0x5AU	/* 90 */
#define TEGRA_SID_PCIE5		0x5BU	/* 91 */

#define TEGRA_SID_NVDEC1	0x5CU	/* 92 */

#define TEGRA_SID_RCE_VM3	0x61U	/* 97 */

#define TEGRA_SID_VI_VM2	0x62U	/* 98 */
#define TEGRA_SID_VI_VM3	0x63U	/* 99 */
#define TEGRA_SID_RCE_SERVER	0x64U	/* 100 */

#endif
