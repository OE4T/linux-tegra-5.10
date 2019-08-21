/*
 * Copyright (c) 2017-2018, NVIDIA CORPORATION
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

#ifndef BPMP_ABI_MACH_T194_SOCTHERM_H
#define BPMP_ABI_MACH_T194_SOCTHERM_H

/**
 * @file
 * @defgroup bpmp_soctherm_ids Soctherm ID's
 * @{
 *   @defgroup bpmp_soctherm_throt_ids Throttle Identifiers
 *   @defgroup bpmp_soctherm_edp_oc_ids EDP/OC Identifiers
 *   @defgroup bpmp_soctherm_throt_modes Throttle Modes
 * @}
 */

/**
 * @addtogroup bpmp_soctherm_throt_ids
 * @{
 */
#define SOCTHERM_THROT_VEC_LITE		0U
#define SOCTHERM_THROT_VEC_HEAVY	1U
#define SOCTHERM_THROT_VEC_OC1		2U
#define SOCTHERM_THROT_VEC_OC2		3U
#define SOCTHERM_THROT_VEC_OC3		4U
#define SOCTHERM_THROT_VEC_OC4		5U
#define SOCTHERM_THROT_VEC_OC5		6U
#define SOCTHERM_THROT_VEC_OC6		7U
#define SOCTHERM_THROT_VEC_INVALID	8U
/** @} */

/**
 * @addtogroup bpmp_soctherm_edp_oc_ids
 * @{
 */
#define SOCTHERM_EDP_OC1	0U
#define SOCTHERM_EDP_OC2	1U
#define SOCTHERM_EDP_OC3	2U
#define SOCTHERM_EDP_OC4	3U
#define SOCTHERM_EDP_OC5	4U
#define SOCTHERM_EDP_OC6	5U
#define SOCTHERM_EDP_OC_INVALID	6U
/** @} */

/**
 * @addtogroup bpmp_soctherm_throt_modes
 */
#define SOCTHERM_EDP_OC_MODE_BRIEF	2U

#endif
