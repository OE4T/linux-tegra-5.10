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


#ifndef UNIT_NVGPU_CHANNEL_H
#define UNIT_NVGPU_CHANNEL_H

#include <nvgpu/types.h>

struct unit_module;
struct gk20a;

/** @addtogroup SWUTS-fifo-channel
 *  @{
 *
 * Software Unit Test Specification for fifo/channel
 */

/**
 * Test specification for: test_channel_setup_sw
 *
 * Description: Branch coverage for nvgpu_channel_setup/cleanup_sw.
 *
 * Test Type: Feature based
 *
 * Input: None
 *
 * Steps:
 * - Check valid case for nvgpu_channel_setup_sw.
 * - Check valid case for nvgpu_channel_cleanup_sw.
 * - Check invalid case for nvgpu_channel_setup_sw.
 *   - Failure to allocate channel contexts (by using fault injection for
 *     vzalloc).
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_channel_setup_sw(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_channel_open
 *
 * Description: Branch coverage for nvgpu_channel_open_new.
 *
 * Test Type: Feature based
 *
 * Input: test_fifo_init_support() run for this GPU
 *
 * Steps:
 * - Check that channel can be allocated with nvgpu_channel_open_new:
 *    - Allocate channel w/ valid runlist_id.
 *    - Allocate channel w/ invalid runlist_id (nvgpu_channel_open_new
 *      should set it to GR runlist_id).
 *    - Allocate w/ or w/o is_privileged_channel set.
 *    - Check that aggresive_sync_destroy is set to true, if used channels
 *      is above threshold (by setting threshold and forcing used_channels
 *      to a greater value).
 *    - Check that nvgpu_channel_open_new returns a non NULL value,
 *      and that ch->g is initialized.
 * - Check channel allocation failures cases:
 *   - Failure to acquire unused channel (by forcibly emptying f->free_chs).
 *   - Failure to allocate channel instance (by using stub for
 *     g->ops.channel.alloc_inst).
 *   - Channel is not referenceable (by forcing ch->referenceable = false and
 *     checking that WARN occurs).
 *   - Channel is in use (by forcing ch->ref_count > 0 and checking that
 *     WARN occurs).
 *   - Allocated channel invalid (by forcing ch->g to NULL value
 *     and checking that BUG occurs).
 *   In negative testing case, original state is restored after checking
 *   that nvgpu_channel_open_new failed.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_channel_open(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_channel_close
 *
 * Description: Branch coverage for nvgpu_channel_close/kill.
 *
 * Test Type: Feature based
 *
 * Input: test_fifo_init_support() run for this GPU
 *
 * Steps:
 * - Check valid cases for nvgpu_channel_close/kill:
 *    - Closing channel w/ force = false (nvgpu_channel_close).
 *    - Closing channel w/ force = true (nvgpu_channel_kill).
 *    - Check that g->os_channel.close is called when defined (by using stub).
 *    - Closing a channel bound to TSG.
 *    - Closing a channel with bound AS (by bounding it to dummy VM, and
 *      checking that ref count is decremented).
 *    - Check that g->ops.gr.setup.free_subctx is called when defined.
 *    - Once closed, check that ch->g is NULL, channel is in list of free
 *      channels, and that it is not referenceable.
 * - Check invalid cases:
 *    - Closing a channel while driver is dying (unbind is skipped when
 *      driver is dying).
 *    - Channel already freed (by closing it twice, and checking that BUG
 *      occurs for second invokation).
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_channel_close(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_channel_setup_bind
 *
 * Description: Branch coverage for nvgpu_channel_setup_bind.
 *
 * Test Type: Feature based
 *
 * Input: test_fifo_init_support() run for this GPU
 *
 * Steps:
 * - Check valid cases for nvgpu_channel_setup_bind:
 *    - Allocate channel and TSG.
 *    - Bind channel to TSG.
 *    - Allocate dummy pdb_mem, and set dummy VM for ch->vm
 *    - Call nvgpu_channel_setup_bind.
 *    - Check that g->os_channel.alloc_usermode_buffers is called (by using
 *      stub), and that ch->usermode_submit_enabled is true.
 *    - Check that g->ops.runlist.update_for_channel is called for this
 *      channel (by using stub).
 *    - Check that channel is bound (ch->bound = true).
 * - Check invalid cases for nvgpu_channel_setup_bind:
 *    - Channel does not have address space (by setting ch->vm = NULL).
 *    - Channel already has GPFIFO set up (by allocating dummy ch->gpfifo.mem).
 *    - Usermode submit is already set for this channel (by forcing
 *      ch->usermode).
 *    - Closing a channel while driver is dying (unbind is skipped when
 *      drive is dying).
 *    - Channel already freed (by closing it twice, and checking that BUG
 *      occurs for second invokation).
 *   For invalid cases, check that error is returned, and that channel does not
 *   have valid userd or gpfifo.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_channel_setup_bind(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_channel_alloc_inst
 *
 * Description: Branch coverage for nvgpu_channel_alloc_inst.
 *
 * Test Type: Feature based
 *
 * Input: test_fifo_init_support() run for this GPU
 *
 * Steps:
 * - Check valid cases for nvgpu_channel_alloc_inst:
 *    - Open a channel with nvgpu_channel_open_new, and check that
 *      nvgpu_channel_alloc_inst returns valid DMA memory for ch->inst_block
 *      (aperture != INVALID).
 *    - Free channel instance with nvgpu_channel_free_inst and check
 *      that ch->inst_block has now an invalid aperture.
 * - Check invalid cases for nvgpu_channel_alloc_inst:
 *    - Enable fault injection for DMA allocation, check that
 *      nvgpu_channel_alloc_inst fails and that ch->inst_block.aperture
 *      is invalid.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_channel_alloc_inst(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_channel_from_inst
 *
 * Description: Branch coverage for nvgpu_channel_refch_from_inst_ptr.
 *
 * Test Type: Feature based
 *
 * Input: test_fifo_init_support() run for this GPU
 *
 * Steps:
 * - Check valid cases for nvgpu_channel_refch_from_inst_ptr:
 *   - Allocate 2 channels each with its instance block.
 *   - Check that chA is retrieved from instA.
 *   - Check that chB is retrieved from instB.
 *   - Check that refcount is incremented for channel.
 * - Check invalid cases for nvgpu_channel_refch_from_inst_ptr:
 *   - Pass invalid inst_ptr and check that no channel is found.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_channel_from_inst(struct unit_module *m,
		struct gk20a *g, void *args);

/**
 * Test specification for: test_channel_enable_disable_tsg
 *
 * Description: Branch coverage for nvgpu_channel_enable/disable_tsg.
 *
 * Test Type: Feature based
 *
 * Input: test_fifo_init_support() run for this GPU
 *
 * Steps:
 * - Allocate channel and TSG, and bind them.
 * - Check that g->ops.tsg.enable is called for TSG when
 *   nvgpu_channel_enable_tsg is called for ch (by using stub).
 * - Check that g->ops.tsg.disable is called for TSG when
 *   nvgpu_channel_disable_tsg is called for ch (by using stub).
 * - Unbind channel from TSG, and check that nvgpu_channel_enable_tsg
 *   and nvgpu_channel_disable_tsg return an error.
 *
 * Output: Returns PASS if all branches gave expected results. FAIL otherwise.
 */
int test_channel_enable_disable_tsg(struct unit_module *m,
		struct gk20a *g, void *args);
/**
 * @}
 */

#endif /* UNIT_NVGPU_CHANNEL_H */