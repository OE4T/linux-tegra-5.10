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
#ifndef UNIT_NVGPU_TSG_H
#define UNIT_NVGPU_TSG_H

#include <nvgpu/types.h>

struct unit_module;
struct gk20a;

/** @addtogroup SWUTS-fifo-tsg
 *  @{
 *
 * Software Unit Test Specification for fifo/tsg
 */

/**
 * Test specification for: test_tsg_open
 *
 * Description: Create TSG
 *
 * Test Type: Feature
 *
 * Targets: nvgpu_tsg_open, nvgpu_tsg_open_common,
 *          nvgpu_tsg_alloc_sm_error_states_mem,
 *          nvgpu_tsg_default_timeslice_us,
 *          nvgpu_tsg_get_from_id,
 *          nvgpu_tsg_check_and_get_from_id
 *
 * Input: test_fifo_init_support() run for this GPU
 *
 * Steps:
 * - Check that nvgpu_tsg_default_timeslice_us returns
 *   NVGPU_TSG_TIMESLICE_DEFAULT_US.
 * - Check that nvgpu_tsg_check_and_get_from_id return NULL for
 *   NVGPU_INVALID_TSG_ID.
 * - Check that TSG can be allocated with nvgpu_tsg_open.
 *    - Check that nvgpu_tsg_open returns a non NULL value.
 *    - Check that tsg can be retrieved from tsgid with nvgpu_tsg_get_from_id.
 *    - Check that nvgpu_tsg_check_and_get_from_id return tsg from its id.
 *    - Decrement ref_count in order to invoke nvgpu_tsg_release.
 * - Check TSG allocation failures cases:
 *   - failure to acquire unused TSG (by forcing f->num_channels to 0).
 *   - failure to allocate sm error state:
 *     - invalid number of SMs (by stubbing g->ops.gr.init.get_no_of_sm).
 *     - TSG context in use (by setting next tsg->sm_error_states to
 *       non NULL value).
 *     - failure to allocate memory (by enabling fault injection for
 *       kzalloc).
 *   In negative testing case, original state is restored after checking
 *   that nvgpu_tsg_open failed.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_tsg_open(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_tsg_bind_channel
 *
 * Description: Bind channel to TSG.
 *
 * Test Type: Feature
 *
 * Targets: nvgpu_tsg_bind_channel, nvgpu_tsg_from_ch
 *
 * Input: test_fifo_init_support() run for this GPU
 *
 * Steps:
 * - Check that channel can be bound to TSG:
 *   - Allocate TSG with nvgpu_tsg_open.
 *   - Allocate channel with nvgpu_channel_open_new.
 *   - Check that nvgpu_tsg_bind_channel returns 0.
 *   - Check that TSG's list of channel is not empty.
 *   - Unbind channel with nvgpu_tsg_unbind_channel.
 *   - Check that ch->tsgid is now invalid.
 *   - Check that tsg can be retrieved from ch using nvgpu_tsg_from_ch.
 * - Check TSG bind failure cases:
 *   - Attempt to bind an already bound channel (by binding a channel to a
 *     TSG, then attempting to bind it to another TSG).
 *   - Attempt to bind channel and TSGs with runlist_id mismatch (by forcing
 *     TSG's runlist_id to a different value).
 *   - Attempt to bind a channel that is already active (by forcing related
 *     bit in the runlist->active_channels bitmask).
 *   - Check that nvgpu_tsg_from_ch return NULL when bind failed.

 *   In negative testing case, original state is restored after checking
 *   that test_tsg_bind_channel failed.
 * - Additionally, the following cases are checked:
 *   - Case where g->ops.tsg.bind_channel is NULL.
 *   - Case where g->ops.tsg.bind_channel_eng_method_buffers is NULL.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_tsg_bind_channel(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_tsg_unbind_channel
 *
 * Description: Unbind channel from TSG.
 *
 * Test Type: Feature
 *
 * Targets: nvgpu_tsg_unbind_channel
 *
 * Input: test_fifo_init_support() run for this GPU
 *
 * Steps:
 * - Check that channel can be unbound from TSG:
 *   - Allocate TSG and channel.
 *   - Bind channel to TSG.
 *   - Unbind channel from TSG.
 *   - Check that channel has been removed from TSG's list.
 *   - Check that channel's tsgid is invalid.
 *   - Check that other channels in TSG are still bound.
 * - Check TSG unbind failure cases:
 *   - Attempt to unbind an unserviceable channel (by forcing unserviceable).
 *   - Failure to preempt TSG (by stubbing g->ops.fifo.preempt_tsg).
 *   - Channel with invalid HW state (by stubbing
 *     g->ops.tsg.unbind_channel_check_hw_state).
 *   - Failure to update runlist (by stubbing
 *     g->ops.runlist.update_for_channel).
 *   - Failure to update runlist during TSG abort (by stubbing
 *     g->ops.runlist.update_for_channel and using a counter to fail only
 *     during abort).
 *   - Attempt to bind an already bound channel (by binding a channel to a
 *     TSG, then attempting to bind it to another TSG).
 *   - Attempt to bind channel and TSGs with runlist_id mismatch (by forcing
 *     TSG's runlist_id to a different value).
 *   - Attempt to bind a channel that is already active (by forcing related
 *     bit in the runlist->active_channels bitmask).
 *   In negative testing case, original state is restored after checking
 *   that test_tsg_unbind_channel failed.
 * - Additionally, the following cases are checked:
 *   - Case where g->ops.tsg.unbind_channel is NULL.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_tsg_unbind_channel(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_tsg_release
 *
 * Description: Release TSG.
 *
 * Test Type: Feature
 *
 * Targets: nvgpu_tsg_release, nvgpu_tsg_release_common
 *
 * Input: test_fifo_init_support() run for this GPU
 *
 * Steps:
 * - Check that TSG can be released:
 *   - Allocate TSG.
 *   - Decrement ref count and check that TSG is released.
 *   - Check that in_use is false.
 * - Check de-allocation of other resources:
 *   - Case where g->ops.gr.setup.free_gr_ctx is called.
 *     It requires dummy vm, gr_ctx and gr_ctx->mem to be allocated.
 *     A stub is used to check that the HAL was actually invoked.
 *   - Other combinations of vm, gr_ctx and gr_ctx->mem allocations, to
 *     check that g->ops.gr.setup.free_gr_ctx is not called.
 *   - Unhook of event_ids (by adding 2 dummy events in event_id list, and
 *     checking that list is empty after TSG release).
 *   - Case where event_id is empty before TSG release is tested as well
 *   - Check that VM refcount is decremented (and VM deallocated in our
 *     case), when present.
 *   - Check that sm_error_states is deallocated.
 *   - Check any combination of VM, gr_ctx, gr_ctx->mem, and sm_error_state.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_tsg_release(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_tsg_unbind_channel_check_hw_state
 *
 * Description: Check HW state during TSG unbind channel.
 *
 * Test Type: Feature
 *
 * Targets: nvgpu_tsg_unbind_channel_check_hw_state
 *
 * Input: test_fifo_init_support() run for this GPU
 *
 * Steps:
 * - Check valid cases for nvgpu_tsg_unbind_channel_check_hw_state:
 *   - hw_state.next is not set (as per g->ops.channel.read_state).
 *   - Check that g->ops.tsg.unbind_channel_check_ctx_reload is called
 *     when defined (using a stub).
 *   - Check that g->ops.tsg.unbind_channel_check_eng_faulted is called
 *     when defined (using a stub).
 * - Check failure cases in nvgpu_tsg_unbind_channel_check_hw_state:
 *   - Case where hw_state.next is set (by stubbing g->ops.channel.read_state).
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_tsg_unbind_channel_check_hw_state(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_tsg_unbind_channel_check_ctx_reload
 *
 * Description: Check if channel reload is needed during TSG unbind
 *
 * Test Type: Feature
 *
 * Targets: nvgpu_tsg_unbind_channel_check_ctx_reload
 *
 * Input: test_fifo_init_support() run for this GPU
 *
 * Steps:
 * - Check valid cases for nvgpu_tsg_unbind_channel_check_ctx_reload:
 *   - hw_state.ctx_reload is not set (nothing to do).
 *   - hw_state.ctx_reload is set:
 *     - Check that what another is bound to TSG, g->ops.channel.force_ctx_reload
 *       is called for this channel. This is done by allocating another channel,
 *       binding it to the same TSG, stubbing g->ops.channel.force_ctx_reload,
 *       and checking that the stub was called for this channel.
 *     - Check that g->ops.channel.force_ctx_reload is not called when there is
 *       no other channel in the TSG.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_tsg_unbind_channel_check_ctx_reload(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_tsg_enable
 *
 * Description: Enable/disable TSG
 *
 * Test Type: Feature
 *
 * Targets: gops_tsg.enable, gops_tsg.disable,
 *          nvgpu_tsg_disable
 *
 * Input: test_fifo_init_support() run for this GPU
 *
 * Steps:
 * - Check valid cases for g->ops.tsg.enable:
 *   - Enable TSG with a bound channel.
 *     - Check that g->ops.channel.enable is called (using stub).
 *     - Check that g->ops.usermode.ring_doorbell (using stub).
 *   - Enable TSG without bound channel.
 * - Check valid cases for g->ops.tsg.disable:
 *   - Disable TSG with a bound channel.
 *     - Check that g->ops.channel.disable is called (using stub).
 *   - Disable TSG without bound channel.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_tsg_enable(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_tsg_enable_sched
 *
 * Description: Enable/disable TSG scheduling
 *
 * Test Type: Feature
 *
 * Targets: nvgpu_tsg_enable_sched, nvgpu_tsg_disable_sched
 *
 * Input: test_fifo_init_support() run for this GPU
 *
 * Steps:
 * - Create a TSG with a bound channel.
 * - Use stub for g->ops.runlist.write_state to store runlist_mask
 *   and runlist_state.
 * - Call nvgpu_tsg_enable_sched and check that runlist_mask matches
 *   TSG's runlist_id and runlist_state is RUNLIST_ENABLED.
 * - Call nvgpu_tsg_diable_sched and check that runlist_mask matches
 *   TSG's runlist_id and runlist_state is RUNLIST_DISABLED.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_tsg_enable_sched(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_tsg_check_and_get_from_id
 *
 * Description: Get TSG context from id
 *
 * Test Type: Feature
 *
 * Targets: tsg_check_and_get_from_id
 *
 * Input: test_fifo_init_support() run for this GPU
 *
 * Steps:
 * - Check that nvgpu_tsg_check_and_get_from_id returns NULL for
 *   and invalid tsgid (NVGPU_INVALID_TSG_ID).
 * - Check that nvgpu_tsg_check_and_get_from_id returns correct
 *   tsg pointer for an existing TSG.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_tsg_check_and_get_from_id(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_tsg_abort
 *
 * Description: Abort TSG
 *
 * Test Type: Feature
 *
 * Targets: nvgpu_tsg_abort
 *
 * Input: test_fifo_init_support() run for this GPU
 *
 * Steps:
 * - Check valid cases for nvgpu_tsg_abort:
 *   - Abort TSG with bound channel.
 *     - Check that g->ops.channel.abort_clean_up is called for channel
 *       (by using stub).
 *   - Abort TSG without bound channel.
 *   - Check with and without preempt set.
 *   - Check that g->ops.fifo.preempt_tsg is called when preempt is
 *     requested (by using stub).
 * - Check invalid cases for nvgpu_tsg_abort:
 *   - Attempt to abort a non-abortable TSG (by forcing tsg->abortable=false).
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_tsg_abort(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_tsg_setup_sw
 *
 * Description: SW Initialization for TSGs
 *
 * Test Type: Feature
 *
 * Targets: nvgpu_tsg_setup_sw, nvgpu_tsg_cleanup_sw
 *
 * Input: None
 *
 * Steps:
 * - Check valid case for nvgpu_tsg_setup_sw.
 * - Check valid case for nvgpu_tsg_cleanup_sw.
 * - Check invalid case for nvgpu_tsg_setup_sw.
 *   - Failure to allocate TSG context (by using fault injection for vzalloc).
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_tsg_setup_sw(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_tsg_mark_error
 *
 * Description: Mark all channels unserviceable in a TSG
 *
 * Test Type: Feature
 *
 * Targets: nvgpu_tsg_mark_error, nvgpu_tsg_set_error_notifier
 *
 * Input: None
 *
 * Steps:
 * - Check marginal cases:
 *   - Mark error for TSG with no bound channel.
 *   - Mark error for TSG with one non serviceable channel.
 * - Check likely cases:
 *   - Use one TSG with one bound channel.
 *   - Set error notifier to NVGPU_ERR_NOTIFIER_FIFO_ERROR_IDLE_TIMEOUT.
 *   - Check that nvgpu_tsg_mark_error returns true (i.e. verbose), when
 *     ch->ctxsw_timeout_debug_dump is true.
 *   - Check that nvgpu_tsg_mark_error returns false otherwise.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_tsg_mark_error(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_tsg_set_ctx_mmu_error
 *
 * Description: Set MMU fault error notifier for TSG
 *
 * Test Type: Feature
 *
 * Targets: nvgpu_tsg_set_ctx_mmu_error
 *
 * Input: None
 *
 * Steps:
 * - Setup a TSG with one bound channel.
 * - Initialize error notifier for channel.
 * - Call nvgpu_tsg_set_ctx_mmu_erro for TSG.
 * - Check that channel's error notifier has been set to
 *   NVGPU_ERR_NOTIFIER_FIFO_ERROR_MMU_ERR_FLT.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_tsg_set_ctx_mmu_error(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_tsg_reset_faulted_eng_pbdma
 *
 * Description: Reset faulted engine and/or PBDMAs for a TSG
 *
 * Test Type: Feature
 *
 * Targets: nvgpu_tsg_reset_faulted_eng_pbdma, nvgpu_channel_from_ch_entry
 *
 * Input: None
 *
 * Steps:
 * - Check valid case:
 *   - Setup a TSG with one bound channel.
 *   - Call nvgpu_tsg_reset_faulted_eng_pbdma.
 *   - Check that g->ops.channel.reset_faulted was called for channel.
 * - Check invalid cases:
 *   - Case where TSG pointer is NULL.
 *   - Case where  g->ops.channel.reset_faulted is NULL.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_tsg_reset_faulted_eng_pbdma(struct unit_module *m,
		struct gk20a *g, void *args);
/**
 * @}
 */

#endif /* UNIT_NVGPU_TSG_H */
