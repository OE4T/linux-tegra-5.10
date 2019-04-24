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

#ifndef NVGPU_INCLUDE_GR_CTX_H
#define NVGPU_INCLUDE_GR_CTX_H

#include <nvgpu/types.h>
#include <nvgpu/nvgpu_mem.h>
#include <nvgpu/gr/global_ctx.h>

/*
 * allocate a minimum of 1 page (4KB) worth of patch space, this is 512 entries
 * of address and data pairs
 */
#define PATCH_CTX_SLOTS_REQUIRED_PER_ENTRY	2U
#define PATCH_CTX_SLOTS_PER_PAGE \
	(PAGE_SIZE/(PATCH_CTX_SLOTS_REQUIRED_PER_ENTRY * (u32)sizeof(u32)))
#define PATCH_CTX_ENTRIES_FROM_SIZE(size) ((size)/sizeof(u32))

#define NVGPU_PREEMPTION_MODE_GRAPHICS_WFI	BIT32(0)
#define NVGPU_PREEMPTION_MODE_GRAPHICS_GFXP	BIT32(1)

#define NVGPU_PREEMPTION_MODE_COMPUTE_WFI	BIT32(0)
#define NVGPU_PREEMPTION_MODE_COMPUTE_CTA	BIT32(1)
#define NVGPU_PREEMPTION_MODE_COMPUTE_CILP	BIT32(2)

struct gk20a;
struct vm_gk20a;
struct nvgpu_gr_ctx;
struct nvgpu_gr_global_ctx_buffer_desc;
struct nvgpu_gr_global_ctx_local_golden_image;
struct patch_desc;
struct zcull_ctx_desc;
struct pm_ctx_desc;
struct nvgpu_gr_ctx_desc;

#define NVGPU_GR_CTX_CTX		0U
#define NVGPU_GR_CTX_PM_CTX		1U
#define NVGPU_GR_CTX_PATCH_CTX		2U
#define NVGPU_GR_CTX_PREEMPT_CTXSW	3U
#define NVGPU_GR_CTX_SPILL_CTXSW	4U
#define NVGPU_GR_CTX_BETACB_CTXSW	5U
#define NVGPU_GR_CTX_PAGEPOOL_CTXSW	6U
#define NVGPU_GR_CTX_GFXP_RTVCB_CTXSW	7U
#define NVGPU_GR_CTX_COUNT		8U

/*
 * either ATTRIBUTE or ATTRIBUTE_VPR maps to NVGPU_GR_CTX_ATTRIBUTE_VA
*/
#define NVGPU_GR_CTX_CIRCULAR_VA		0U
#define NVGPU_GR_CTX_PAGEPOOL_VA		1U
#define NVGPU_GR_CTX_ATTRIBUTE_VA		2U
#define NVGPU_GR_CTX_PRIV_ACCESS_MAP_VA		3U
#define NVGPU_GR_CTX_RTV_CIRCULAR_BUFFER_VA	4U
#define NVGPU_GR_CTX_FECS_TRACE_BUFFER_VA	5U
#define NVGPU_GR_CTX_VA_COUNT			6U

 /* PM Context Switch Mode */
/*This mode says that the pms are not to be context switched. */
#define NVGPU_GR_CTX_HWPM_CTXSW_MODE_NO_CTXSW               (0x00000000U)
/* This mode says that the pms in Mode-B are to be context switched */
#define NVGPU_GR_CTX_HWPM_CTXSW_MODE_CTXSW                  (0x00000001U)
/* This mode says that the pms in Mode-E (stream out) are to be context switched. */
#define NVGPU_GR_CTX_HWPM_CTXSW_MODE_STREAM_OUT_CTXSW       (0x00000002U)

struct nvgpu_gr_ctx_desc *nvgpu_gr_ctx_desc_alloc(struct gk20a *g);
void nvgpu_gr_ctx_desc_free(struct gk20a *g,
	struct nvgpu_gr_ctx_desc *desc);

void nvgpu_gr_ctx_set_size(struct nvgpu_gr_ctx_desc *gr_ctx_desc,
	u32 index, u32 size);

int nvgpu_gr_ctx_alloc(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_ctx_desc *gr_ctx_desc,
	struct vm_gk20a *vm);
void nvgpu_gr_ctx_free(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_global_ctx_buffer_desc *global_ctx_buffer,
	struct vm_gk20a *vm);

int nvgpu_gr_ctx_alloc_pm_ctx(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_ctx_desc *gr_ctx_desc,
	struct vm_gk20a *vm,
	u64 gpu_va);
void nvgpu_gr_ctx_free_pm_ctx(struct gk20a *g, struct vm_gk20a *vm,
	struct nvgpu_gr_ctx *gr_ctx);

int nvgpu_gr_ctx_alloc_patch_ctx(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_ctx_desc *gr_ctx_desc,
	struct vm_gk20a *vm);
void nvgpu_gr_ctx_free_patch_ctx(struct gk20a *g, struct vm_gk20a *vm,
	struct nvgpu_gr_ctx *gr_ctx);

void nvgpu_gr_ctx_set_zcull_ctx(struct gk20a *g, struct nvgpu_gr_ctx *gr_ctx,
	u32 mode, u64 gpu_va);

int nvgpu_gr_ctx_alloc_ctxsw_buffers(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_ctx_desc *gr_ctx_desc,
	struct vm_gk20a *vm);

int nvgpu_gr_ctx_map_global_ctx_buffers(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_global_ctx_buffer_desc *global_ctx_buffer,
	struct vm_gk20a *vm, bool vpr);

u64 nvgpu_gr_ctx_get_global_ctx_va(struct nvgpu_gr_ctx *gr_ctx,
	u32 index);

struct nvgpu_mem *nvgpu_gr_ctx_get_spill_ctxsw_buffer(
	struct nvgpu_gr_ctx *gr_ctx);

struct nvgpu_mem *nvgpu_gr_ctx_get_betacb_ctxsw_buffer(
	struct nvgpu_gr_ctx *gr_ctx);

struct nvgpu_mem *nvgpu_gr_ctx_get_pagepool_ctxsw_buffer(
	struct nvgpu_gr_ctx *gr_ctx);

struct nvgpu_mem *nvgpu_gr_ctx_get_preempt_ctxsw_buffer(
	struct nvgpu_gr_ctx *gr_ctx);

struct nvgpu_mem *nvgpu_gr_ctx_get_gfxp_rtvcb_ctxsw_buffer(
	struct nvgpu_gr_ctx *gr_ctx);

struct nvgpu_mem *nvgpu_gr_ctx_get_patch_ctx_mem(struct nvgpu_gr_ctx *gr_ctx);

void nvgpu_gr_ctx_set_patch_ctx_data_count(struct nvgpu_gr_ctx *gr_ctx,
	u32 data_count);

struct nvgpu_mem *nvgpu_gr_ctx_get_pm_ctx_mem(struct nvgpu_gr_ctx *gr_ctx);

u64 nvgpu_gr_ctx_get_zcull_ctx_va(struct nvgpu_gr_ctx *gr_ctx);

struct nvgpu_mem *nvgpu_gr_ctx_get_ctx_mem(struct nvgpu_gr_ctx *gr_ctx);

int nvgpu_gr_ctx_load_golden_ctx_image(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	struct nvgpu_gr_global_ctx_local_golden_image *local_golden_image,
	bool cde);

int nvgpu_gr_ctx_patch_write_begin(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	bool update_patch_count);
void nvgpu_gr_ctx_patch_write_end(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	bool update_patch_count);
void nvgpu_gr_ctx_patch_write(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx,
	u32 addr, u32 data, bool patch);

void nvgpu_gr_ctx_reset_patch_count(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx);
void nvgpu_gr_ctx_set_patch_ctx(struct gk20a *g, struct nvgpu_gr_ctx *gr_ctx,
	bool set_patch_addr);

u32 nvgpu_gr_ctx_get_ctx_id(struct gk20a *g, struct nvgpu_gr_ctx *gr_ctx);

int nvgpu_gr_ctx_init_zcull(struct gk20a *g, struct nvgpu_gr_ctx *gr_ctx);
int nvgpu_gr_ctx_zcull_setup(struct gk20a *g, struct nvgpu_gr_ctx *gr_ctx,
	bool set_zcull_ptr);

int nvgpu_gr_ctx_set_smpc_mode(struct gk20a *g, struct nvgpu_gr_ctx *gr_ctx,
	bool enable);

int nvgpu_gr_ctx_prepare_hwpm_mode(struct gk20a *g, struct nvgpu_gr_ctx *gr_ctx,
	u32 mode, bool *skip_update);
int nvgpu_gr_ctx_set_hwpm_mode(struct gk20a *g, struct nvgpu_gr_ctx *gr_ctx,
	bool set_pm_ptr);

void nvgpu_gr_ctx_init_compute_preemption_mode(struct nvgpu_gr_ctx *gr_ctx,
	u32 compute_preempt_mode);
u32 nvgpu_gr_ctx_get_compute_preemption_mode(struct nvgpu_gr_ctx *gr_ctx);

void nvgpu_gr_ctx_init_graphics_preemption_mode(struct nvgpu_gr_ctx *gr_ctx,
	u32 graphics_preempt_mode);
u32 nvgpu_gr_ctx_get_graphics_preemption_mode(struct nvgpu_gr_ctx *gr_ctx);

bool nvgpu_gr_ctx_check_valid_preemption_mode(struct nvgpu_gr_ctx *gr_ctx,
	u32 graphics_preempt_mode, u32 compute_preempt_mode);
void nvgpu_gr_ctx_set_preemption_modes(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx);
void nvgpu_gr_ctx_set_preemption_buffer_va(struct gk20a *g,
	struct nvgpu_gr_ctx *gr_ctx);

struct nvgpu_gr_ctx *nvgpu_alloc_gr_ctx_struct(struct gk20a *g);

void nvgpu_free_gr_ctx_struct(struct gk20a *g, struct nvgpu_gr_ctx *gr_ctx);

void nvgpu_gr_ctx_set_tsgid(struct nvgpu_gr_ctx *gr_ctx, u32 tsgid);

u32 nvgpu_gr_ctx_get_tsgid(struct nvgpu_gr_ctx *gr_ctx);

void nvgpu_gr_ctx_set_pm_ctx_pm_mode(struct nvgpu_gr_ctx *gr_ctx, u32 pm_mode);

u32 nvgpu_gr_ctx_get_pm_ctx_pm_mode(struct nvgpu_gr_ctx *gr_ctx);

bool nvgpu_gr_ctx_get_cilp_preempt_pending(struct nvgpu_gr_ctx *gr_ctx);

void nvgpu_gr_ctx_set_cilp_preempt_pending(struct nvgpu_gr_ctx *gr_ctx,
	bool cilp_preempt_pending);

u32 nvgpu_gr_ctx_read_ctx_id(struct nvgpu_gr_ctx *gr_ctx);

void nvgpu_gr_ctx_set_boosted_ctx(struct nvgpu_gr_ctx *gr_ctx, bool boost);

bool nvgpu_gr_ctx_get_boosted_ctx(struct nvgpu_gr_ctx *gr_ctx);

bool nvgpu_gr_ctx_desc_force_preemption_gfxp(
		struct nvgpu_gr_ctx_desc *gr_ctx_desc);
bool nvgpu_gr_ctx_desc_force_preemption_cilp(
		struct nvgpu_gr_ctx_desc *gr_ctx_desc);

bool nvgpu_gr_ctx_desc_dump_ctxsw_stats_on_channel_close(
		struct nvgpu_gr_ctx_desc *gr_ctx_desc);

#endif /* NVGPU_INCLUDE_GR_CTX_H */
