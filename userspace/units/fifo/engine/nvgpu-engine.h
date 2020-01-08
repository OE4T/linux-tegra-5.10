/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
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


#ifndef UNIT_NVGPU_ENGINE_H
#define UNIT_NVGPU_ENGINE_H

#include <nvgpu/types.h>

struct unit_module;
struct gk20a;

/** @addtogroup SWUTS-fifo-engine
 *  @{
 *
 * Software Unit Test Specification for fifo/engine
 */

/**
 * Test specification for: test_engine_setup_sw
 *
 * Description: Branch coverage for nvgpu_engine_setup/cleanup_sw.
 *
 * Test Type: Feature
 *
 * Targets: nvgpu_engine_setup_sw, nvgpu_engine_cleanup_sw
 *
 * Input: None
 *
 * Steps:
 * - Check valid case for nvgpu_engine_setup_sw.
 * - Check valid case for nvgpu_engine_cleanup_sw.
 * - Check invalid case for nvgpu_engine_setup_sw.
 *   - Failure to allocate engine contexts (w/ fault injection)
 *   - Failure to allocate active engines list (w/ fault injection)
 *   - Failure to initialize engine info (using stub for
 *     g->ops.engine.init_info)
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_engine_setup_sw(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_engine_init_info
 *
 * Description: Branch coverage for nvgpu_engine_init_info
 *
 * Test Type: Feature
 *
 * Targets: nvgpu_engine_init_info
 *
 * Input: test_fifo_init_support must have run.
 *
 * Steps:
 * - Check valid cases for nvgpu_engine_init_info using gv11b HALs.
 *   - Check that function returns 0 and that number of engines is > 0.
 * - Check invalid cases for nvgpu_engine_init_info:
 *   - g->ops.top.get_device_info is NULL
 *   - g->ops.top.get_device_info returns failure
 *   - g->ops.pbdma.find_for_runlist fails to find PBDMA servicing the engine.
 *   - Check that function returns < 0 and that number of engines is 0.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_engine_init_info(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_engine_ids
 *
 * Description: Branch coverage for engine ids
 *
 * Test Type: Feature
 *
 * Targets: nvgpu_engine_get_ids, nvgpu_engine_check_valid_id,
 *          nvgpu_engine_get_gr_id
 *
 * Input: test_fifo_init_support must have run.
 *
 * Steps:
 * - Check nvgpu_engine_check_valid_id returns false for U32_MAX
 * - Get engine ids for all engine enums in NVGPU_ENGINE_GR to
 *   NVGPU_ENGINE_INVAL
 *   - Check that all returned ids are valid  with nvgpu_engine_check_valid_id.
 *   - Check that nvgpu_engine_get_gr_id is in the returned ids for
 *     NVGPU_ENGINE_GR
 *   - Build a mask of CE engines (for other test use)
 *   - Build a mask of active engines (for other test use)
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_engine_ids(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_engine_get_active_eng_info
 *
 * Description: Branch coverage for nvgpu_engine_get_active_eng_info
  *
 * Test Type: Feature
 *
 * Targets: nvgpu_engine_get_active_eng_info, nvgpu_engine_check_valid_id
 *
 * Input: test_engine_ids must have run.
 *
 * Steps:
 * - For each H/W engine id, call nvgpu_engine_get_active_eng_info:
 *   - Check that info is non NULL for active engines.
 *   - Check that info is NULL for inactive engines.
 * - Check that nvgpu_engine_get_active_eng_info returns NULL when g == NULL.
 * - Check that nvgpu_engine_get_active_eng_info returns NULL when f->max_engines == 0.
 * - Check that nvgpu_engine_get_active_eng_info returns NULL when f->num_engines == 0.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_engine_get_active_eng_info(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_engine_enum_from_type
 *
 * Description: Branch coverage for nvgpu_engine_enum_from_type
  *
 * Test Type: Feature
 *
 * Targets: nvgpu_engine_enum_from_type
 *
 * Input: test_engine_ids must have run.
 *
 * Steps:
 * - For each HW enum type, call nvgpu_engine_enum_from_type.
 *   - Check that NVGPU_ENGINE_GR is returned for
 *     top_device_info_type_enum_graphics_v().
 *   - Check that NVGPU_ENGINE_ASYNC_CE is returned for
 *     top_device_info_type_enum_lce_v().
 *   - Check that NVGPU_ENGINE_INVAL is returned for other values.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_engine_enum_from_type(struct unit_module *m,
		struct gk20a *g, void *args);


/**
 * Test specification for: test_engine_interrupt_mask
 *
 * Description: Engine interrupt masks
 *
 * Test Type: Feature
 *
 * Targets: nvgpu_gr_engine_interrupt_mask, nvgpu_ce_engine_interrupt_mask,
 * nvgpu_engine_act_interrupt_mask, nvgpu_engine_get_all_ce_reset_mask
 *
 * Input: test_engine_ids must have run.
 *
 * Steps:
 * - Get interrupt mask for all engines using ngpu_engine_interrupt_mask.
 *   - Check that engine_intr_mask in non NULL
 * - For each active engine, get interrupt mask with
 *   nvgpu_engine_act_interrupt_mask.
 *   - Check that mask in non NULL
 *   - Check that mask is contained in engine_intr_mask.
 *   - Check that engine_intr_mask only contains active engines
 * - Get CE reset mask using nvgpu_engine_get_all_ce_reset_mask
 *   - Check that ce_reset_mask == ce_mask (from unit context)
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_engine_interrupt_mask(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * @}
 */

#endif /* UNIT_NVGPU_ENGINE_H */
