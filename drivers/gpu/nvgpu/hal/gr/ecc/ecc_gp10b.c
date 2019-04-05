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

#include <nvgpu/io.h>
#include <nvgpu/ecc.h>
#include <nvgpu/gk20a.h>

#include <nvgpu/hw/gp10b/hw_gr_gp10b.h>

#include "ecc_gp10b.h"

void gp10b_ecc_detect_enabled_units(struct gk20a *g)
{
	bool opt_ecc_en = g->ops.fuse.is_opt_ecc_enable(g);
	bool opt_feature_fuses_override_disable =
			g->ops.fuse.is_opt_feature_override_disable(g);
	u32 fecs_feature_override_ecc =
				nvgpu_readl(g,
					gr_fecs_feature_override_ecc_r());

	if (opt_feature_fuses_override_disable) {
		if (opt_ecc_en) {
			nvgpu_set_enabled(g, NVGPU_ECC_ENABLED_SM_LRF, true);
			nvgpu_set_enabled(g, NVGPU_ECC_ENABLED_SM_SHM, true);
			nvgpu_set_enabled(g, NVGPU_ECC_ENABLED_TEX, true);
			nvgpu_set_enabled(g, NVGPU_ECC_ENABLED_LTC, true);
		}
	} else {
		/* SM LRF */
		if (gr_fecs_feature_override_ecc_sm_lrf_override_v(
					fecs_feature_override_ecc) == 1U) {
			if (gr_fecs_feature_override_ecc_sm_lrf_v(
					fecs_feature_override_ecc) == 1U) {
				nvgpu_set_enabled(g,
						NVGPU_ECC_ENABLED_SM_LRF, true);
			}
		} else {
			if (opt_ecc_en) {
				nvgpu_set_enabled(g,
						NVGPU_ECC_ENABLED_SM_LRF, true);
			}
		}

		/* SM SHM */
		if (gr_fecs_feature_override_ecc_sm_shm_override_v(
					fecs_feature_override_ecc) == 1U) {
			if (gr_fecs_feature_override_ecc_sm_shm_v(
					fecs_feature_override_ecc) == 1U) {
				nvgpu_set_enabled(g,
						NVGPU_ECC_ENABLED_SM_SHM, true);
			}
		} else {
			if (opt_ecc_en) {
				nvgpu_set_enabled(g,
						NVGPU_ECC_ENABLED_SM_SHM, true);
			}
		}

		/* TEX */
		if (gr_fecs_feature_override_ecc_tex_override_v(
					fecs_feature_override_ecc) == 1U) {
			if (gr_fecs_feature_override_ecc_tex_v(
					fecs_feature_override_ecc) == 1U) {
				nvgpu_set_enabled(g,
						NVGPU_ECC_ENABLED_TEX, true);
			}
		} else {
			if (opt_ecc_en) {
				nvgpu_set_enabled(g,
						NVGPU_ECC_ENABLED_TEX, true);
			}
		}

		/* LTC */
		if (gr_fecs_feature_override_ecc_ltc_override_v(
					fecs_feature_override_ecc) == 1U) {
			if (gr_fecs_feature_override_ecc_ltc_v(
					fecs_feature_override_ecc) == 1U) {
				nvgpu_set_enabled(g,
						NVGPU_ECC_ENABLED_LTC, true);
			}
		} else {
			if (opt_ecc_en) {
				nvgpu_set_enabled(g,
						NVGPU_ECC_ENABLED_LTC, true);
			}
		}
	}
}

int gp10b_ecc_init(struct gk20a *g)
{
	int err = 0;

	err = NVGPU_ECC_COUNTER_INIT_PER_TPC(sm_lrf_ecc_single_err_count);
	if (err != 0) {
		goto done;
	}
	err = NVGPU_ECC_COUNTER_INIT_PER_TPC(sm_lrf_ecc_double_err_count);
	if (err != 0) {
		goto done;
	}

	err = NVGPU_ECC_COUNTER_INIT_PER_TPC(sm_shm_ecc_sec_count);
	if (err != 0) {
		goto done;
	}
	err = NVGPU_ECC_COUNTER_INIT_PER_TPC(sm_shm_ecc_sed_count);
	if (err != 0) {
		goto done;
	}
	err = NVGPU_ECC_COUNTER_INIT_PER_TPC(sm_shm_ecc_ded_count);
	if (err != 0) {
		goto done;
	}

	err = NVGPU_ECC_COUNTER_INIT_PER_TPC(tex_ecc_total_sec_pipe0_count);
	if (err != 0) {
		goto done;
	}
	err = NVGPU_ECC_COUNTER_INIT_PER_TPC(tex_ecc_total_ded_pipe0_count);
	if (err != 0) {
		goto done;
	}

	err = NVGPU_ECC_COUNTER_INIT_PER_TPC(tex_unique_ecc_sec_pipe0_count);
	if (err != 0) {
		goto done;
	}
	err = NVGPU_ECC_COUNTER_INIT_PER_TPC(tex_unique_ecc_ded_pipe0_count);
	if (err != 0) {
		goto done;
	}

	err = NVGPU_ECC_COUNTER_INIT_PER_TPC(tex_ecc_total_sec_pipe1_count);
	if (err != 0) {
		goto done;
	}
	err = NVGPU_ECC_COUNTER_INIT_PER_TPC(tex_ecc_total_ded_pipe1_count);
	if (err != 0) {
		goto done;
	}

	err = NVGPU_ECC_COUNTER_INIT_PER_TPC(tex_unique_ecc_sec_pipe1_count);
	if (err != 0) {
		goto done;
	}
	err = NVGPU_ECC_COUNTER_INIT_PER_TPC(tex_unique_ecc_ded_pipe1_count);
	if (err != 0) {
		goto done;
	}

	err = NVGPU_ECC_COUNTER_INIT_PER_LTS(ecc_sec_count);
	if (err != 0) {
		goto done;
	}
	err = NVGPU_ECC_COUNTER_INIT_PER_LTS(ecc_ded_count);
	if (err != 0) {
		goto done;
	}

done:
	if (err != 0) {
		nvgpu_err(g, "ecc counter allocate failed, err=%d", err);
		nvgpu_ecc_free(g);
	}

	return err;
}
