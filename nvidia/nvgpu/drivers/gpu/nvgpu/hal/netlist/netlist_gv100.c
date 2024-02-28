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
#include <nvgpu/gk20a.h>

#include "netlist_gv100.h"

int gv100_netlist_get_name(struct gk20a *g, int index, char *name)
{
	u32 ver = g->params.gpu_arch + g->params.gpu_impl;
	int valid = 0;

	switch (ver) {
	case NVGPU_GPUID_GV100:
		(void) strcpy(name, "gv100/");
		(void) strcat(name, GV100_NETLIST_IMAGE_FW_NAME);
		break;
	default:
		nvgpu_err(g, "no support for GPUID %x", ver);
		valid = -1;
		break;
	}

	return valid;
}

bool gv100_netlist_is_firmware_defined(void)
{
	return true;
}
