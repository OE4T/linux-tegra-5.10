/*
 * Copyright (c) 2016-2019, NVIDIA CORPORATION.  All rights reserved.
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
#ifndef NVGPU_PMU_PERF_PERF_H
#define NVGPU_PMU_PERF_PERF_H

#define CTRL_PERF_VFE_VAR_TYPE_INVALID                               0x00U
#define CTRL_PERF_VFE_VAR_TYPE_DERIVED                               0x01U
#define CTRL_PERF_VFE_VAR_TYPE_DERIVED_PRODUCT                       0x02U
#define CTRL_PERF_VFE_VAR_TYPE_DERIVED_SUM                           0x03U
#define CTRL_PERF_VFE_VAR_TYPE_SINGLE                                0x04U
#define CTRL_PERF_VFE_VAR_TYPE_SINGLE_FREQUENCY                      0x05U
#define CTRL_PERF_VFE_VAR_TYPE_SINGLE_SENSED                         0x06U
#define CTRL_PERF_VFE_VAR_TYPE_SINGLE_SENSED_FUSE                    0x07U
#define CTRL_PERF_VFE_VAR_TYPE_SINGLE_SENSED_TEMP                    0x08U
#define CTRL_PERF_VFE_VAR_TYPE_SINGLE_VOLTAGE                        0x09U
#define CTRL_PERF_VFE_VAR_TYPE_SINGLE_CALLER_SPECIFIED               0x0AU

#define CTRL_PERF_VFE_VAR_SINGLE_OVERRIDE_TYPE_NONE                  0x00U
#define CTRL_PERF_VFE_VAR_SINGLE_OVERRIDE_TYPE_VALUE                 0x01U
#define CTRL_PERF_VFE_VAR_SINGLE_OVERRIDE_TYPE_OFFSET                0x02U
#define CTRL_PERF_VFE_VAR_SINGLE_OVERRIDE_TYPE_SCALE                 0x03U

#define CTRL_PERF_VFE_EQU_TYPE_INVALID                               0x00U
#define CTRL_PERF_VFE_EQU_TYPE_COMPARE                               0x01U
#define CTRL_PERF_VFE_EQU_TYPE_MINMAX                                0x02U
#define CTRL_PERF_VFE_EQU_TYPE_QUADRATIC                             0x03U
#define CTRL_PERF_VFE_EQU_TYPE_SCALAR                                0x04U

#define CTRL_PERF_VFE_EQU_OUTPUT_TYPE_UNITLESS                       0x00U
#define CTRL_PERF_VFE_EQU_OUTPUT_TYPE_FREQ_MHZ                       0x01U
#define CTRL_PERF_VFE_EQU_OUTPUT_TYPE_VOLT_UV                        0x02U
#define CTRL_PERF_VFE_EQU_OUTPUT_TYPE_VF_GAIN                        0x03U
#define CTRL_PERF_VFE_EQU_OUTPUT_TYPE_VOLT_DELTA_UV                  0x04U
#define CTRL_PERF_VFE_EQU_OUTPUT_TYPE_WORK_TYPE                      0x06U
#define CTRL_PERF_VFE_EQU_OUTPUT_TYPE_UTIL_RATIO                     0x07U
#define CTRL_PERF_VFE_EQU_OUTPUT_TYPE_WORK_FB_NORM                   0x08U
#define CTRL_PERF_VFE_EQU_OUTPUT_TYPE_POWER_MW                       0x09U
#define CTRL_PERF_VFE_EQU_OUTPUT_TYPE_PWR_OVER_UTIL_SLOPE            0x0AU
#define CTRL_PERF_VFE_EQU_OUTPUT_TYPE_VIN_CODE                       0x0BU

#define CTRL_PERF_VFE_EQU_QUADRATIC_COEFF_COUNT                      0x03U

#define CTRL_PERF_VFE_EQU_COMPARE_FUNCTION_EQUAL                     0x00U
#define CTRL_PERF_VFE_EQU_COMPARE_FUNCTION_GREATER_EQ                0x01U
#define CTRL_PERF_VFE_EQU_COMPARE_FUNCTION_GREATER                   0x02U

#endif /* NVGPU_PMU_PERF_PERF_H */
