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
#ifndef UNIT_NVGPU_GR_CONFIG_H
#define UNIT_NVGPU_GR_CONFIG_H

#include <nvgpu/types.h>

struct gk20a;
struct unit_module;

/** @addtogroup SWUTS-gr-config
 *  @{
 *
 * Software Unit Test Specification for common.gr.config
 */

/**
 * Test specification for: test_gr_config_init.
 *
 * Description: Setup for common.gr.config unit. This test helps
 * to read the GR engine configuration and stores the configuration
 * values in the #nvgpu_gr_config struct.
 *
 * Test Type: Feature based.
 *
 * Input: None
 *
 * Steps:
 * -  Call nvgpu_gr_config_init
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_gr_config_init(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_gr_config_deinit.
 *
 * Description: Cleanup common.gr.config unit.
 *
 * Test Type: Feature based.
 *
 * Input: test_gr_config_init must have been executed successfully.
 *
 * Steps:
 * -  Call nvgpu_gr_config_deinit
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_gr_config_deinit(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_gr_config_count.
 *
 * Description: This test helps to verify whether the configurations
 *              read from the h/w matches with locally stored informations
 *              for a particular chip.
 *
 * Test Type: Feature based, Error guessing.
 *
 * Input: test_gr_config_init must have been executed successfully.
 *
 * Steps:
 * -  Read configuration count and mask informations from the driver
 *    which got stored as part of the nvgpu_gr_config_init.
 *    Compare those values against the locally maintained table.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_gr_config_count(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_gr_config_set_get.
 *
 * Description: This test helps to verify whether the write and read back
 *              reflect the same value. This test helps to verify the
 *              configuration values can be changed as part of floorsweeping.
 *
 * Test Type: Feature based, Error guessing
 *
 * Input: nvgpu_gr_config_init must have been executed successfully.
 *
 * Steps:
 * -  Random values are set for various configuration and read back to
 *    check those values.
 *
 * Output: Returns PASS if the steps above were executed successfully. FAIL
 * otherwise.
 */
int test_gr_config_set_get(struct unit_module *m, struct gk20a *g, void *args);

#endif /* UNIT_NVGPU_GR_CONFIG_H */

/**
 * @}
 */

