/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
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
#ifndef NVGPU_GOPS_DEBUGGER_H
#define NVGPU_GOPS_DEBUGGER_H

#ifdef CONFIG_NVGPU_DEBUGGER
struct gops_regops {
	int (*exec_regops)(struct gk20a *g,
				struct nvgpu_tsg *tsg,
				struct nvgpu_dbg_reg_op *ops,
				u32 num_ops,
				u32 *flags);
	const struct regop_offset_range* (
			*get_global_whitelist_ranges)(void);
	u64 (*get_global_whitelist_ranges_count)(void);
	const struct regop_offset_range* (
			*get_context_whitelist_ranges)(void);
	u64 (*get_context_whitelist_ranges_count)(void);
	const u32* (*get_runcontrol_whitelist)(void);
	u64 (*get_runcontrol_whitelist_count)(void);
};
struct gops_debugger {
	void (*post_events)(struct nvgpu_channel *ch);
	int (*dbg_set_powergate)(struct dbg_session_gk20a *dbg_s,
				bool disable_powergate);
};
struct gops_perf {
	void (*enable_membuf)(struct gk20a *g, u32 size, u64 buf_addr);
	void (*disable_membuf)(struct gk20a *g);
	void (*init_inst_block)(struct gk20a *g,
		struct nvgpu_mem *inst_block);
	void (*deinit_inst_block)(struct gk20a *g);
	void (*membuf_reset_streaming)(struct gk20a *g);
	u32 (*get_membuf_pending_bytes)(struct gk20a *g);
	void (*set_membuf_handled_bytes)(struct gk20a *g,
		u32 entries, u32 entry_size);
	bool (*get_membuf_overflow_status)(struct gk20a *g);
	u32 (*get_pmmsys_per_chiplet_offset)(void);
	u32 (*get_pmmgpc_per_chiplet_offset)(void);
	u32 (*get_pmmfbp_per_chiplet_offset)(void);
};
struct gops_perfbuf {
	int (*perfbuf_enable)(struct gk20a *g, u64 offset, u32 size);
	int (*perfbuf_disable)(struct gk20a *g);
	int (*init_inst_block)(struct gk20a *g);
	void (*deinit_inst_block)(struct gk20a *g);
};
#endif

#endif /* NVGPU_GOPS_DEBUGGER_H */
