/*
 * Copyright (c) 2017-2020, NVIDIA CORPORATION.  All rights reserved.
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
#ifndef NVGPU_FUSE_H
#define NVGPU_FUSE_H
/**
 * @file
 *
 * Interface for fuse ops.
 */

struct gk20a;

#include <nvgpu/types.h>
#include <nvgpu/errno.h>

#ifdef CONFIG_NVGPU_TEGRA_FUSE

#ifdef CONFIG_NVGPU_NON_FUSA
int nvgpu_tegra_get_gpu_speedo_id(struct gk20a *g, int *id);
int nvgpu_tegra_fuse_read_reserved_calib(struct gk20a *g, u32 *val);
#endif /* CONFIG_NVGPU_NON_FUSA */

/**
 * @brief -  Write Fuse bypass register which controls fuse bypass.
 *
 * @param g [in] - GPU super structure.
 * @param val [in]- 0 : DISABLED, 1 : ENABLED
 *
 * - Write 0/1 to control the fuse bypass.
 *
 * @return none.
 */
void nvgpu_tegra_fuse_write_bypass(struct gk20a *g, u32 val);

/**
 * @brief - Enable software write access
 *
 * @param g [in] - GPU super structure.
 * @param val [in] - 0 : READWRITE, 1 : READONLY
 *
 * - Bit 0 of the register is the write control register. When set to 1,
 *   it disables writes to chip.
 *
 * @return none.
 */
void nvgpu_tegra_fuse_write_access_sw(struct gk20a *g, u32 val);

/**
 * @brief - Disable TPC0
 *
 * @param g [in] - GPU super structure.
 * @param val [in] - 1 : DISABLED, 0 : ENABLED
 *
 * - Write 1/0 to fuse tpc disable register to disable/enable the TPC0.
 *
 * @return none.
 */
void nvgpu_tegra_fuse_write_opt_gpu_tpc0_disable(struct gk20a *g, u32 val);

/**
 * @brief - Disable TPC1
 *
 * @param g [in] - GPU super structure.
 * @param val [in] - 1 : DISABLED, 0 : ENABLED
 *
 * - Write 1/0 to fuse tpc disable register to disable/enable the TPC1.
 *
 * @return none.
 */
void nvgpu_tegra_fuse_write_opt_gpu_tpc1_disable(struct gk20a *g, u32 val);

/**
 * @brief - Reads GCPLEX_CONFIG_FUSE configuration.
 *
 * @param g [in] - GPU super structure.
 * @param val [out] - Populated with register GCPLEX_CONFIG_FUSE value.
 *
 * - Provide information about the GPU complex configuration.
 *
 * @return 0 on success.
 *
 */
int nvgpu_tegra_fuse_read_gcplex_config_fuse(struct gk20a *g, u32 *val);

#else /* CONFIG_NVGPU_TEGRA_FUSE */

#ifdef CONFIG_NVGPU_NON_FUSA
static inline int nvgpu_tegra_get_gpu_speedo_id(struct gk20a *g, int *id)
{
	return -EINVAL;
}

static inline int nvgpu_tegra_fuse_read_reserved_calib(struct gk20a *g,
						       u32 *val)
{
	return -EINVAL;
}
#endif /* CONFIG_NVGPU_NON_FUSA */

static inline void nvgpu_tegra_fuse_write_bypass(struct gk20a *g, u32 val)
{
}

static inline void nvgpu_tegra_fuse_write_access_sw(struct gk20a *g, u32 val)
{
}

static inline void nvgpu_tegra_fuse_write_opt_gpu_tpc0_disable(struct gk20a *g,
							       u32 val)
{
}

static inline void nvgpu_tegra_fuse_write_opt_gpu_tpc1_disable(struct gk20a *g,
							       u32 val)
{
}

static inline int nvgpu_tegra_fuse_read_gcplex_config_fuse(struct gk20a *g,
							   u32 *val)
{
	/*
	 * Setting gcplex_config fuse to wpr_enabled/vpr_auto_fetch_disable
	 * by default that is expected on the production chip.
	 */
	*val = 0x4;

	return 0;
}

#endif /* CONFIG_NVGPU_TEGRA_FUSE */
#endif /* NVGPU_FUSE_H */
