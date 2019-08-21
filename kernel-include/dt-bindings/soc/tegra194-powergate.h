/*
 * Copyright (c) 2016-2018, NVIDIA CORPORATION
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef BPMP_ABI_MACH_T194_POWERGATE_T194_H
#define BPMP_ABI_MACH_T194_POWERGATE_T194_H

/**
 * @file
 * @defgroup bpmp_pdomain_ids Power Domain ID's
 * This is a list of power domain IDs provided by the firmware.
 * @note Following partitions are forcefully powered down upon entering SC7 power state.
 *  - TEGRA194_POWER_DOMAIN_PVAA
 *  - TEGRA194_POWER_DOMAIN_PVAB
 *  - TEGRA194_POWER_DOMAIN_DLAA
 *  - TEGRA194_POWER_DOMAIN_DLAB
 *  - TEGRA194_POWER_DOMAIN_CV
 *  - TEGRA194_POWER_DOMAIN_GPU
 * @{
 */
#define TEGRA194_POWER_DOMAIN_AUD	1U
#define TEGRA194_POWER_DOMAIN_DISP	2U
#define TEGRA194_POWER_DOMAIN_DISPB	3U
#define TEGRA194_POWER_DOMAIN_DISPC	4U
#define TEGRA194_POWER_DOMAIN_ISPA	5U
#define TEGRA194_POWER_DOMAIN_NVDECA	6U
#define TEGRA194_POWER_DOMAIN_NVJPG	7U
#define TEGRA194_POWER_DOMAIN_NVENCA	8U
#define TEGRA194_POWER_DOMAIN_NVENCB	9U
#define TEGRA194_POWER_DOMAIN_NVDECB	10U
#define TEGRA194_POWER_DOMAIN_SAX	11U
#define TEGRA194_POWER_DOMAIN_VE	12U
#define TEGRA194_POWER_DOMAIN_VIC	13U
#define TEGRA194_POWER_DOMAIN_XUSBA	14U
#define TEGRA194_POWER_DOMAIN_XUSBB	15U
#define TEGRA194_POWER_DOMAIN_XUSBC	16U
#define TEGRA194_POWER_DOMAIN_PCIEX8A	17U
#define TEGRA194_POWER_DOMAIN_PCIEX4A	18U
#define TEGRA194_POWER_DOMAIN_PCIEX1A	19U
#define TEGRA194_POWER_DOMAIN_NVL	20U
#define TEGRA194_POWER_DOMAIN_PCIEX8B	21U
#define TEGRA194_POWER_DOMAIN_PVAA	22U
#define TEGRA194_POWER_DOMAIN_PVAB	23U
#define TEGRA194_POWER_DOMAIN_DLAA	24U
#define TEGRA194_POWER_DOMAIN_DLAB	25U
#define TEGRA194_POWER_DOMAIN_CV	26U
#define TEGRA194_POWER_DOMAIN_GPU	27U
#define TEGRA194_POWER_DOMAIN_MAX	27U
/** @} */

#endif
