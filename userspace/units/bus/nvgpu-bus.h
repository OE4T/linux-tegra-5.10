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

#ifndef UNIT_NVGPU_BUS_H
#define UNIT_NVGPU_BUS_H

struct gk20a;
struct unit_module;

/** @addtogroup SWUTS-bus
 *  @{
 *
 * Software Unit Test Specification for nvgpu.common.bus
 */

/**
 * Test specification for: test_setup
 *
 * Description: Setup prerequisites for tests.
 *
 * Test Type: Other (setup)
 *
 * Input: None
 *
 * Steps:
 * - Initialize common.bus and few other necessary HAL function pointers.
 * - Map the register space for NV_PBUS, NV_PMC and NV_PTIMER.
 * - Register read/write callback functions.
 *
 * Output:
 * - UNIT_FAIL if encounters an error creating reg space
 * - UNIT_SUCCESS otherwise
 */
int test_setup(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_free_reg_space
 *
 * Description: Free resources from test_setup()
 *
 * Test Type: Other (setup)
 *
 * Input: test_setup() has been executed.
 *
 * Steps:
 * - Free up NV_PBUS, NV_PMC and NV_PTIMER register space.
 *
 * Output:
 * - UNIT_SUCCESS
 */
int test_free_reg_space(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_init_hw
 *
 * Description: Verify the bus.init_hw and bus.configure_debug_bus HAL.
 *
 * Test Type: Feature Based
 *
 * Targets: gk20a_bus_init_hw, gv11b_bus_configure_debug_bus.
 *
 * Input: test_setup() has been executed.
 *
 * Steps:
 * - Initialize the Debug bus related registers to non-zero value.
 * - Set is_silicon flag to true to get branch coverage.
 * - Call init_hw() HAL.
 * - Read back the debug bus registers to make sure they are zeroed out.
 *      pri_ringmaster_command_r = 0x4
 *      pri_ringstation_sys_decode_config_r = 0x2
 *
 * Output:
 * - UNIT_FAIL if above HAL fails to enable interrupts.
 * - UNIT_SUCCESS otherwise.
 */
int test_init_hw(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_bar_bind
 *
 * Description: Verify the bus.bar1_bind and bus.bar2_bind HAL.
 *
 * Test Type: Feature Based
 *
 * Targets: gm20b_bus_bar1_bind, gp10b_bus_bar2_bind.
 *
 * Input: test_setup() has been executed.
 *
 * Steps:
 * - Initialize cpu_va to a known value (say 0xCE418000U).
 * - Set bus_bind_status_r to 0xF that is both bar1 and bar2 status
 *   pending and outstanding.
 * - Call bus.bar1_bind() HAL.
 * - Make sure HAL returns success as bind_status is marked as done in
 *   third polling attempt.
 * - Send error if bar1_block register is not set as expected:
 *     - Bit 27:0 - 4k aligned block pointer = bar_inst.cpu_va >> 12 = 0xCE418
 *     - Bit 29:28- Target = (11)b
 *     - Bit 30   - Debug CYA = (0)b
 *     - Bit 31   - Mode = virtual = (1)b
 * - Call bus.bar1_bind HAL again and except ret != 0 as the bind status
 *   will remain pending and outstanding during this call.
 * - The HAL should return error this time as timeout is expected to expire.
 * - Enable fault injection for the timer init call for branch coverage.
 * - Repeat the above steps for BAR2 but with different cpu_va = 0x2670C000U.
 *
 * Output:
 * - UNIT_FAIL if above HAL fails to bind BAR1/2
 * - UNIT_SUCCESS otherwise.
 */
int test_bar_bind(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_isr
 *
 * Description: Verify the bus.isr HAL.
 *
 * Test Type: Feature Based
 *
 * Targets: gk20a_bus_isr
 *
 * Input: test_setup() has been executed.
 *
 * Steps:
 * - Initialize interrupt register bus_intr_0_r() to 0x2(pri_squash)
 * - Call isr HAL.
 * - Initialize interrupt register bus_intr_0_r() to 0x4(pri_fecserr)
 * - Call isr HAL.
 * - Initialize interrupt register bus_intr_0_r() to 0x8(pri_timeout)
 * - Call isr HAL.
 * - Initialize interrupt register bus_intr_0_r() to 0x10(fb_req_timeout)
 * - Call isr HAL.
 *
 * Output:
 * - UNIT_SUCCESS.
 */
int test_isr(struct unit_module *m, struct gk20a *g, void *args);
#endif /* UNIT_NVGPU_BUS_H */
