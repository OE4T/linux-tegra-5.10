/*
 * Tegra GK20A GPU Debugger Driver Register Ops
 *
 * Copyright (c) 2013-2019, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/regops.h>

#include "regops_gm20b.h"

static const struct regop_offset_range gm20b_global_whitelist_ranges[] = {
	{ 0x00001a00,   3 },
	{ 0x0000259c,   1 },
	{ 0x0000280c,   1 },
	{ 0x00009400,   1 },
	{ 0x00009410,   1 },
	{ 0x00021970,   1 },
	{ 0x00021c00,   4 },
	{ 0x00021c14,   3 },
	{ 0x00021c24,   1 },
	{ 0x00021c2c,   5 },
	{ 0x00021cb8,   2 },
	{ 0x00021d38,   2 },
	{ 0x00021d44,   1 },
	{ 0x00021d4c,   1 },
	{ 0x00021d54,   1 },
	{ 0x00021d5c,   1 },
	{ 0x00021d64,   2 },
	{ 0x00021d70,   1 },
	{ 0x00022430,   7 },
	{ 0x00100c18,   3 },
	{ 0x00100c84,   1 },
	{ 0x00100cc4,   1 },
	{ 0x00106640,   1 },
	{ 0x0010a0a8,   1 },
	{ 0x0010a4f0,   1 },
	{ 0x0010e064,   1 },
	{ 0x0010e164,   1 },
	{ 0x0010e490,   1 },
	{ 0x00140028,   1 },
	{ 0x00140350,   1 },
	{ 0x00140550,   1 },
	{ 0x00142028,   1 },
	{ 0x00142350,   1 },
	{ 0x00142550,   1 },
	{ 0x0017e028,   1 },
	{ 0x0017e350,   1 },
	{ 0x0017e550,   1 },
	{ 0x00180040,  52 },
	{ 0x00180240,  52 },
	{ 0x00180440,  52 },
	{ 0x001a0040,  52 },
	{ 0x001b0040,  52 },
	{ 0x001b0240,  52 },
	{ 0x001b0440,  52 },
	{ 0x001b0640,  52 },
	{ 0x001b4000,   3 },
	{ 0x001b4010,   3 },
	{ 0x001b4020,   3 },
	{ 0x001b4030,   3 },
	{ 0x001b4040,   3 },
	{ 0x001b4050,   3 },
	{ 0x001b4060,   4 },
	{ 0x001b4074,  11 },
	{ 0x001b40a4,   1 },
	{ 0x001b4100,   6 },
	{ 0x001b4124,   2 },
	{ 0x001b8000,   7 },
	{ 0x001bc000,   7 },
	{ 0x001be000,   7 },
	{ 0x00400500,   1 },
	{ 0x00400700,   1 },
	{ 0x0040415c,   1 },
	{ 0x00405850,   1 },
	{ 0x00405908,   1 },
	{ 0x00405b40,   1 },
	{ 0x00405b50,   1 },
	{ 0x00406024,   1 },
	{ 0x00407010,   1 },
	{ 0x00407808,   1 },
	{ 0x0040803c,   1 },
	{ 0x0040880c,   1 },
	{ 0x00408910,   1 },
	{ 0x00408984,   1 },
	{ 0x004090a8,   1 },
	{ 0x004098a0,   1 },
	{ 0x00409b00,   1 },
	{ 0x0041000c,   1 },
	{ 0x00410110,   1 },
	{ 0x00410184,   1 },
	{ 0x0041040c,   1 },
	{ 0x00410510,   1 },
	{ 0x00410584,   1 },
	{ 0x00418384,   1 },
	{ 0x004184a0,   1 },
	{ 0x00418604,   1 },
	{ 0x00418680,   1 },
	{ 0x00418714,   1 },
	{ 0x0041881c,   1 },
	{ 0x00418884,   1 },
	{ 0x004188b0,   1 },
	{ 0x004188c8,   2 },
	{ 0x00418b04,   1 },
	{ 0x00418c04,   1 },
	{ 0x00418c1c,   1 },
	{ 0x00418c88,   1 },
	{ 0x00418d00,   1 },
	{ 0x00418e08,   1 },
	{ 0x00418f08,   1 },
	{ 0x00419000,   1 },
	{ 0x0041900c,   1 },
	{ 0x00419018,   1 },
	{ 0x00419854,   1 },
	{ 0x00419ab0,   1 },
	{ 0x00419ab8,   3 },
	{ 0x00419c0c,   1 },
	{ 0x00419c90,   1 },
	{ 0x00419d08,   2 },
	{ 0x00419e00,   4 },
	{ 0x00419e24,   2 },
	{ 0x00419e44,  11 },
	{ 0x00419e74,   9 },
	{ 0x00419ea4,   1 },
	{ 0x00419eb0,   1 },
	{ 0x00419ef0,  26 },
	{ 0x0041a0a0,   1 },
	{ 0x0041a0a8,   1 },
	{ 0x0041a17c,   1 },
	{ 0x0041a890,   2 },
	{ 0x0041a8a0,   3 },
	{ 0x0041a8b0,   2 },
	{ 0x0041b014,   1 },
	{ 0x0041b0a0,   1 },
	{ 0x0041b0cc,   1 },
	{ 0x0041b0e8,   2 },
	{ 0x0041b1dc,   1 },
	{ 0x0041be14,   1 },
	{ 0x0041bea0,   1 },
	{ 0x0041becc,   1 },
	{ 0x0041bee8,   2 },
	{ 0x0041bfdc,   1 },
	{ 0x0041c054,   1 },
	{ 0x0041c2b0,   1 },
	{ 0x0041c2b8,   3 },
	{ 0x0041c40c,   1 },
	{ 0x0041c490,   1 },
	{ 0x0041c508,   2 },
	{ 0x0041c600,   4 },
	{ 0x0041c624,   2 },
	{ 0x0041c644,  11 },
	{ 0x0041c674,   9 },
	{ 0x0041c6a4,   1 },
	{ 0x0041c6b0,   1 },
	{ 0x0041c6f0,  26 },
	{ 0x0041c854,   1 },
	{ 0x0041cab0,   1 },
	{ 0x0041cab8,   3 },
	{ 0x0041cc0c,   1 },
	{ 0x0041cc90,   1 },
	{ 0x0041cd08,   2 },
	{ 0x0041ce00,   4 },
	{ 0x0041ce24,   2 },
	{ 0x0041ce44,  11 },
	{ 0x0041ce74,   9 },
	{ 0x0041cea4,   1 },
	{ 0x0041ceb0,   1 },
	{ 0x0041cef0,  26 },
	{ 0x00500384,   1 },
	{ 0x005004a0,   1 },
	{ 0x00500604,   1 },
	{ 0x00500680,   1 },
	{ 0x00500714,   1 },
	{ 0x0050081c,   1 },
	{ 0x00500884,   1 },
	{ 0x005008c8,   2 },
	{ 0x00500b04,   1 },
	{ 0x00500c04,   1 },
	{ 0x00500c88,   1 },
	{ 0x00500d00,   1 },
	{ 0x00500e08,   1 },
	{ 0x00500f08,   1 },
	{ 0x00501000,   1 },
	{ 0x0050100c,   1 },
	{ 0x00501018,   1 },
	{ 0x00501854,   1 },
	{ 0x00501ab0,   1 },
	{ 0x00501ab8,   3 },
	{ 0x00501c0c,   1 },
	{ 0x00501c90,   1 },
	{ 0x00501d08,   2 },
	{ 0x00501e00,   4 },
	{ 0x00501e24,   2 },
	{ 0x00501e44,  11 },
	{ 0x00501e74,   9 },
	{ 0x00501ea4,   1 },
	{ 0x00501eb0,   1 },
	{ 0x00501ef0,  26 },
	{ 0x005020a0,   1 },
	{ 0x005020a8,   1 },
	{ 0x0050217c,   1 },
	{ 0x00502890,   2 },
	{ 0x005028a0,   3 },
	{ 0x005028b0,   2 },
	{ 0x00503014,   1 },
	{ 0x005030a0,   1 },
	{ 0x005030cc,   1 },
	{ 0x005030e8,   2 },
	{ 0x005031dc,   1 },
	{ 0x00503e14,   1 },
	{ 0x00503ea0,   1 },
	{ 0x00503ecc,   1 },
	{ 0x00503ee8,   2 },
	{ 0x00503fdc,   1 },
	{ 0x00504054,   1 },
	{ 0x005042b0,   1 },
	{ 0x005042b8,   3 },
	{ 0x0050440c,   1 },
	{ 0x00504490,   1 },
	{ 0x00504508,   2 },
	{ 0x00504600,   4 },
	{ 0x00504614,   6 },
	{ 0x00504634,   2 },
	{ 0x00504644,  11 },
	{ 0x00504674,   9 },
	{ 0x005046a4,   1 },
	{ 0x005046b0,   1 },
	{ 0x005046f0,  28 },
	{ 0x00504854,   1 },
	{ 0x00504ab0,   1 },
	{ 0x00504ab8,   3 },
	{ 0x00504c0c,   1 },
	{ 0x00504c90,   1 },
	{ 0x00504d08,   2 },
	{ 0x00504e00,   4 },
	{ 0x00504e14,   6 },
	{ 0x00504e34,   2 },
	{ 0x00504e44,  11 },
	{ 0x00504e74,   9 },
	{ 0x00504ea4,   1 },
	{ 0x00504eb0,   1 },
	{ 0x00504ef0,  28 },
};
static const u64 gm20b_global_whitelist_ranges_count =
	ARRAY_SIZE(gm20b_global_whitelist_ranges);

/* context */

static const struct regop_offset_range gm20b_context_whitelist_ranges[] = {
	{ 0x0000259c,   1 },
	{ 0x0000280c,   1 },
	{ 0x00140028,   1 },
	{ 0x00180040,  52 },
	{ 0x001a0040,  52 },
	{ 0x001b0040,  52 },
	{ 0x00400500,   1 },
	{ 0x0040415c,   1 },
	{ 0x00405b40,   1 },
	{ 0x00418e00,   1 },
	{ 0x00418e34,   1 },
	{ 0x00418e40,   2 },
	{ 0x00418e58,   2 },
	{ 0x00419000,   1 },
	{ 0x00419864,   1 },
	{ 0x00419c90,   1 },
	{ 0x00419d08,   2 },
	{ 0x00419e04,   3 },
	{ 0x00419e24,   2 },
	{ 0x00419e44,  11 },
	{ 0x00419e74,  10 },
	{ 0x00419ea4,   1 },
	{ 0x00419eac,   2 },
	{ 0x00419ee8,   1 },
	{ 0x00419ef0,  26 },
	{ 0x0041b0e8,   2 },
	{ 0x0041bee8,   2 },
	{ 0x0041c490,   1 },
	{ 0x0041c508,   2 },
	{ 0x0041c604,   3 },
	{ 0x0041c624,   2 },
	{ 0x0041c644,  11 },
	{ 0x0041c674,  10 },
	{ 0x0041c6a4,   1 },
	{ 0x0041c6ac,   2 },
	{ 0x0041c6e8,   1 },
	{ 0x0041c6f0,  26 },
	{ 0x0041cc90,   1 },
	{ 0x0041cd08,   2 },
	{ 0x0041ce04,   3 },
	{ 0x0041ce24,   2 },
	{ 0x0041ce44,  11 },
	{ 0x0041ce74,  10 },
	{ 0x0041cea4,   1 },
	{ 0x0041ceac,   2 },
	{ 0x0041cee8,   1 },
	{ 0x0041cef0,  26 },
	{ 0x00501000,   1 },
	{ 0x00501c90,   1 },
	{ 0x00501d08,   2 },
	{ 0x00501e04,   3 },
	{ 0x00501e24,   2 },
	{ 0x00501e44,  11 },
	{ 0x00501e74,  10 },
	{ 0x00501ea4,   1 },
	{ 0x00501eac,   2 },
	{ 0x00501ee8,   1 },
	{ 0x00501ef0,  26 },
	{ 0x005030e8,   2 },
	{ 0x00503ee8,   2 },
	{ 0x00504490,   1 },
	{ 0x00504508,   2 },
	{ 0x00504600,   4 },
	{ 0x00504604,   3 },
	{ 0x00504614,   6 },
	{ 0x00504634,   2 },
	{ 0x00504644,  11 },
	{ 0x00504674,  10 },
	{ 0x005046a4,   1 },
	{ 0x005046ac,   2 },
	{ 0x005046e8,   1 },
	{ 0x005046f0,  28 },
	{ 0x00504c90,   1 },
	{ 0x00504d08,   2 },
	{ 0x00504e04,   3 },
	{ 0x00504e14,   6 },
	{ 0x00504e34,   2 },
	{ 0x00504e44,  11 },
	{ 0x00504e74,  10 },
	{ 0x00504ea4,   1 },
	{ 0x00504eac,   2 },
	{ 0x00504ee8,   1 },
	{ 0x00504ef0,  28 },
};
static const u64 gm20b_context_whitelist_ranges_count =
	ARRAY_SIZE(gm20b_context_whitelist_ranges);

/* runcontrol */
static const u32 gm20b_runcontrol_whitelist[] = {
	0x00419e10,
	0x0041c610,
	0x0041ce10,
	0x00501e10,
	0x00504610,
	0x00504e10,
};
static const u64 gm20b_runcontrol_whitelist_count =
	ARRAY_SIZE(gm20b_runcontrol_whitelist);

const struct regop_offset_range *gm20b_get_global_whitelist_ranges(void)
{
	return gm20b_global_whitelist_ranges;
}

u64 gm20b_get_global_whitelist_ranges_count(void)
{
	return gm20b_global_whitelist_ranges_count;
}

const struct regop_offset_range *gm20b_get_context_whitelist_ranges(void)
{
	return gm20b_context_whitelist_ranges;
}

u64 gm20b_get_context_whitelist_ranges_count(void)
{
	return gm20b_context_whitelist_ranges_count;
}

const u32 *gm20b_get_runcontrol_whitelist(void)
{
	return gm20b_runcontrol_whitelist;
}

u64 gm20b_get_runcontrol_whitelist_count(void)
{
	return gm20b_runcontrol_whitelist_count;
}
