/*
* Copyright (c) 2016-2018, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_CLK_VF_POINT_H
#define NVGPU_CLK_VF_POINT_H
#include <nvgpu/pmuif/ctrlclk.h>
#include <nvgpu/pmuif/ctrlboardobj.h>
#include <nvgpu/pmuif/nvgpu_gpmu_cmdif.h>
#include <nvgpu/boardobjgrp_e32.h>
#include <nvgpu/boardobjgrp_e255.h>
#include <nvgpu/boardobjgrpmask.h>

#define VMIN_PAD_UV 50000U

int clk_vf_point_cache(struct gk20a *g);
struct nvgpu_clk_arb;
struct nvgpu_clk_slave_freq{
	u16 gpc_mhz;
	u16 sys_mhz;
	u16 xbar_mhz;
	u16 host_mhz;
	u16 nvd_mhz;
};

int nvgpu_clk_set_req_fll_clk_ps35(struct gk20a *g, struct nvgpu_clk_slave_freq *vf_point);
int nvgpu_clk_arb_find_slave_points(struct nvgpu_clk_arb *arb,struct nvgpu_clk_slave_freq *vf_point);

struct clk_vf_point {
	struct boardobj super;
	u8  vfe_equ_idx;
	u8  volt_rail_idx;
	struct ctrl_clk_vf_pair pair;
};

struct clk_vf_point_volt {
	struct clk_vf_point super;
	u32 source_voltage_uv;
	struct ctrl_clk_freq_delta freq_delta;
};

struct clk_vf_point_freq {
	struct clk_vf_point super;
	int volt_delta_uv;
};

#define CLK_CLK_VF_POINT_GET(pclk, idx)                                        \
	((struct clk_vf_point *)BOARDOBJGRP_OBJ_GET_BY_IDX(                    \
		&pclk->clk_vf_pointobjs.super.super, (u8)(idx)))

#define clkvfpointpairget(pvfpoint)                                            \
	(&((pvfpoint)->pair))

#define clkvfpointfreqmhzget(pgpu, pvfpoint)                                   \
	CTRL_CLK_VF_PAIR_FREQ_MHZ_GET(clkvfpointpairget(pvfpoint))

#define clkvfpointfreqdeltamhzGet(pgpu, pvfPoint)                              \
	((BOARDOBJ_GET_TYPE(pvfpoint) == CTRL_CLK_CLK_VF_POINT_TYPE_VOLT) ?    \
	(((struct clk_vf_point_volt *)(pvfpoint))->freq_delta_khz / 1000) : 0)

#define clkvfpointfreqmhzset(pgpu, pvfpoint, _freqmhz)                         \
	CTRL_CLK_VF_PAIR_FREQ_MHZ_SET(clkvfpointpairget(pvfpoint), _freqmhz)

#define clkvfpointvoltageuvset(pgpu, pvfpoint, _voltageuv)                     \
	CTRL_CLK_VF_PAIR_VOLTAGE_UV_SET(clkvfpointpairget(pvfpoint),           \
	_voltageuv)

#define clkvfpointvoltageuvget(pgpu, pvfpoint)                          \
	CTRL_CLK_VF_PAIR_VOLTAGE_UV_GET(clkvfpointpairget(pvfpoint))	\

struct clk_vf_point *construct_clk_vf_point(struct gk20a *g, void *pargs);

#endif /* NVGPU_CLK_VF_POINT_H */
