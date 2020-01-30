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

#ifndef UNIT_NVGPU_MC_H
#define UNIT_NVGPU_MC_H

struct gk20a;
struct unit_module;

/** @addtogroup SWUTS-mc
 *  @{
 *
 * Software Unit Test Specification for MC
 */

/**
 * Test specification for: test_setup_env
 *
 * Description: Do basic setup before starting other tests.
 *
 * Test Type: Other (setup)
 *
 * Input: None
 *
 * Steps:
 * - Initialize reg spaces used by tests.
 * - Override HALs for other dependent units.
 * - Do minimal initialization for engines and ltc units.
 *
 * Output:
 * - UNIT_FAIL if encounters an error creating reg space
 * - UNIT_SUCCESS otherwise
 */
int test_setup_env(struct unit_module *m,
			  struct gk20a *g, void *args);

/**
 * Test specification for: test_free_env
 *
 * Description: Do basic setup before starting other tests.
 *
 * Test Type: Other (setup)
 *
 * Input: test_setup_env has run.
 *
 * Steps:
 * - Free reg spaces.
 * - Cleanup engine setup.
 * - Free ltc memory.
 *
 * Output: UNIT_SUCCESS always.
 */
int test_free_env(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_unit_config
 *
 * Description: Validate function of nvgpu_mc_intr_stall_unit_config and
 *              nvgpu_mc_intr_nonstall_unit_config.
 *
 * Test Type: Feature, Error guessing
 *
 * Targets: nvgpu_mc_intr_stall_unit_config, nvgpu_mc_intr_nonstall_unit_config,
 *          mc_gp10b_intr_stall_unit_config, mc_gp10b_intr_nonstall_unit_config
 *
 * Input: test_setup_env must have been run.
 *
 * Steps:
 * - Set each of the mock registers for enabling & disabling the stall &
 *   non-stall interrupts to 0.
 * - Loop through table of units:
 *   - Call nvgpu_mc_intr_stall_unit_config for the unit to enable the stall
 *     interrupt.
 *   - Verify the stall interrupt enable register has the bit set for the unit.
 *   - Call nvgpu_mc_intr_stall_unit_config for the unit to disable the interrupt.
 *   - Verify the stall interrupt disable register has the bit set for the unit.
 *   - Call nvgpu_mc_intr_nonstall_unit_config for the unit to enable the
 *     non-stall interrupt.
 *   - Verify the non-stall interrupt enable register has the bit set for the unit.
 *   - Call nvgpu_mc_intr_nonstall_unit_config for the unit to disable the interrupt.
 *   - Verify the non-stall interrupt disable register has the bit set for the unit.
 * - Clear the stall enable register.
 * - For negative testing, call nvgpu_mc_intr_stall_unit_config() with an
 *   invalid unit number, and verify no bits are set in the stall interrupt
 *   enable register.
 * - Clear the stall enable register.
 * - For negative testing, call nvgpu_mc_intr_nonstall_unit_config() with an
 *   invalid unit number, and verify no bits are set in the non-stall interrupt
 *   enable register.
 *
 * Output: Returns PASS if expected result is met, FAIL otherwise.
 */
int test_unit_config(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_pause_resume_mask
 *
 * Description: Validate function for pausing, resuming, and masking interrupts.
 *
 * Test Type: Feature
 *
 * Targets: nvgpu_mc_intr_stall_pause, nvgpu_mc_intr_stall_resume,
 *          nvgpu_mc_intr_nonstall_pause, nvgpu_mc_intr_nonstall_resume,
 *          nvgpu_mc_intr_mask, mc_gp10b_intr_stall_pause,
 *          mc_gp10b_intr_stall_resume, mc_gp10b_intr_nonstall_pause,
 *          mc_gp10b_intr_nonstall_resume, mc_gp10b_intr_mask
 *
 * Input: test_setup_env must have been run.
 *
 * Steps:
 * - Clear each of the mock registers for enabling & disabling the stall &
 *   non-stall interrupts.
 * - Clear mc state regs for active interrupts.
 * - Enable interupts so they can be paused and resumed.
 * - Pause the interrupts.
 * - Verify all the bits were written in the stall and non-stall interrupt
 *   disable registers.
 * - Resume the interrupts.
 * - Verify the correct values are in the stall and non-stall interrupt enable
 *   registers.
 * - Clear the stall and non-stall disable registers.
 * - Mask the interrupts.
 * - Verify all the bits were written in the stall and non-stall interrupt
 *   disable registers.
 * - For branch coverage, temporarily set the g->ops.mc.intr_mask HAL to NULL.
 * - Mask the interrupts.
 *
 * Output: Returns PASS if expected result is met, FAIL otherwise.
 */
int test_pause_resume_mask(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_intr_stall
 *
 * Description: Validate function mc_gp10b_intr_stall which returns the pending
 *              interrupts.
 *
 * Test Type: Feature
 *
 * Targets: gops_mc.intr_stall, mc_gp10b_intr_stall
 *
 * Input: test_setup_env must have been run.
 *
 * Steps:
 * - Loop through setting each bit individually in the stall interrupt pending
 *   register:
 *   - For iteration, call the HAL and verify the correct value is returned.
 *
 * Output: Returns PASS if expected result is met, FAIL otherwise.
 */
int test_intr_stall(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_is_stall_and_eng_intr_pending
 *
 * Description: Validate function of HAL gv11b_mc_is_stall_and_eng_intr_pending.
 *
 * Test Type: Feature
 *
 * Targets: gops_mc.is_stall_and_eng_intr_pending,
 *          gv11b_mc_is_stall_and_eng_intr_pending
 *
 * Input: test_setup_env must have been run.
 *
 * Steps:
 * - Clear the stall interrupt pending register.
 * - Call the HAL API and verify it returns false since nothing is pending.
 * - Set all interrupts pending in the stall interrupt pending register.
 * - Call the HAL API and verify it returns true and the correct pending mask.
 *
 * Output: Returns PASS if expected result is met, FAIL otherwise.
 */
int test_is_stall_and_eng_intr_pending(struct unit_module *m, struct gk20a *g,
					void *args);

/**
 * Test specification for: test_isr_stall
 *
 * Description: Validate handling of the stall interrupts by the stall interrupt
 *              service routine.
 *
 * Test Type: Feature
 *
 * Targets: gops_mc.isr_stall, mc_gp10b_isr_stall
 *
 * Input: test_setup_env must have been run.
 *
 * Steps:
 * - Clear the stall interrupt pending register.
 * - Call the stall ISR.
 * - Verify none of the mock unit ISRs (for bus, ce, fb, etc) are called.
 * - Set all interrupts pending in the stall interrupt pending register.
 * - Call the stall ISR.
 * - Verify all of the mock unit ISRs are called.
 * - For branch coverage, set the HAL pointer g->ops.mc.is_intr_hub_pending to
 *   NULL.
 * - Call the stall ISR. No exception should occur.
 * - For branch coverage, configure the mock GR ISR to return an error.
 * - Call the stall ISR. No exception should occur.
 * - For branch coverage, configure the mock CE ISR pointer to NULL.
 * - Call the stall ISR. No exception should occur.
 * - For branch coverage, configure the active CE engine to the other type.
 * - Call the stall ISR. No exception should occur.
 * - For branch coverage, enable the LTC interupt pending in main MC pending
 *   register, MC_INTR, but disable the LTC interrupt pending in the LTC-specific
 *   register, MC_INTR_LTC.
 * - Call the stall ISR.
 * - Verify the mock LTC ISR was not called.
 *
 * Output: Returns PASS if expected result is met, FAIL otherwise.
 */
int test_isr_stall(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_isr_nonstall
 *
 * Description: Validate handling of the stall interrupts by the non-stall
 * 		interrupt service routine.
 *
 * Test Type: Feature
 *
 * Targets: gops_mc.isr_nonstall, gm20b_mc_isr_nonstall, gops_mc.intr_nonstall,
 *          mc_gp10b_intr_nonstall
 *
 * Input: test_setup_env must have been run.
 *
 * Steps:
 * - Clear the non-stall interrupt pending register.
 * - Call the non-stall ISR.
 * - Verify none of the mock unit ISRs (for bus, ce, fb, etc) are called.
 * - Set all interrupts pending in the non-stall interrupt pending register.
 * - Call the non-stall ISR.
 * - Verify all of the mock unit ISRs are called and the correct ops are returned.
 * - For branch coverage, configure the mock CE ISR pointer to NULL.
 * - Call the non-stall ISR. No exception should occur.
 * - For branch coverage, configure the active CE engine to the other type.
 * - Call the non-stall ISR. No exception should occur.
 *
 * Output: Returns PASS if expected result is met, FAIL otherwise.
 */
int test_isr_nonstall(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_is_intr1_pending
 *
 * Description: Validate functionality of mc_gp10b_is_intr1_pending.
 *
 * Test Type: Feature
 *
 * Targets: gops_mc.is_intr1_pending, mc_gp10b_is_intr1_pending
 *
 * Input: test_setup_env must have been run.
 *
 * Steps:
 * - Call the HAL API, requesting if the FIFO Unit is pending, passing in a
 *   register mask that does not have that Unit pending. Verify false is
 *   returned.
 * - Call the HAL API, requesting if the FIFO Unit is pending, passing in a
 *   register mask that does have that Unit pending. Verify true is returned.
 * - Call the HAL API passing in an invalid unit number. Verify false is
 *   returned.
 *
 * Output: Returns PASS if expected result is met, FAIL otherwise.
 */
int test_is_intr1_pending(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_enable_disable_reset
 *
 * Description: Validate functionality of functions for enabling, disabling,
 *              and reseting units.
 *
 * Test Type: Feature
 *
 * Targets: gops_mc.enable, gops_mc.disable, gops_mc.reset, gm20b_mc_enable,
 *           gm20b_mc_disable, gm20b_mc_reset
 *
 * Input: test_setup_env must have been run.
 *
 * Steps:
 * - Call the enable HAL API to enable units.
 * - Read the MC_ENABLE reg to verify the units were enabled.
 * - Call the disable HAL API to disable units.
 * - Read the MC_ENABLE reg to verify the units were disabled.
 * - Call the reset HAL API to reset units.
 * - Read the MC_ENABLE reg to verify the units were re-enabled.
 *
 * Output: Returns PASS if expected result is met, FAIL otherwise.
 */
int test_enable_disable_reset(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_reset_mask
 *
 * Description: Validate functionality of HAL to get reset mask for a unit.
 *
 * Test Type: Feature, Error guessing
 *
 * Targets: gops_mc.reset_mask, gm20b_mc_reset_mask
 *
 * Input: test_setup_env must have been run.
 *
 * Steps:
 * - Call the enable HAL API for a number of units and verify the correct
 *   mask is returned.
 * - For branch coverage pass in an invalid Unit number, and verify the mask
 *   returned is 0.
 *
 * Output: Returns PASS if expected result is met, FAIL otherwise.
 */
int test_reset_mask(struct unit_module *m, struct gk20a *g, void *args);

/**
 * Test specification for: test_wait_for_deferred_interrupts
 *
 * Description: Validate functionality of waiting for deferred interrupts.
 *
 * Test Type: Feature
 *
 * Targets: nvgpu_wait_for_deferred_interrupts
 *
 * Input: test_setup_env must have been run.
 *
 * Steps:
 * - Initialize cond structures required by the API.
 * - Set the irq count states in the gk20a struct to 0 to cause immediate
 *   completion.
 * - Call the API.
 * - Enable cond fault injection to simulate a timeouts.
 * - Set the irq count states in the gk20a struct to simulate pending stall
 *   interrupts.
 * - Call the API.
 * - Set the irq count states in the gk20a struct to simulate pending non-stall
 *   interrupts.
 * - Disable cond fault injection.
 *
 * Output: Returns PASS if expected result is met, FAIL otherwise.
 */
int test_wait_for_deferred_interrupts(struct unit_module *m, struct gk20a *g,
					void *args);

/**
 * @}
 */

#endif /* UNIT_NVGPU_CE_H */
