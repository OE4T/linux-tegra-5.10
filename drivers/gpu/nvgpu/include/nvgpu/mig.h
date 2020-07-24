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

#ifndef NVGPU_MIG_H
#define NVGPU_MIG_H

#include <nvgpu/types.h>
#include <nvgpu/lock.h>

/** Maximum GPC group supported by HW. */
#define NVGPU_MIG_MAX_GPCGRP				2U

/** Maximum gpu instances count. */
#define NVGPU_MIG_MAX_GPU_INSTANCES			8U

/** Maximum mig config count. */
#define NVGPU_MIG_MAX_MIG_CONFIG_COUNT			16U

/** INVALID sys pipe id. */
#define NVGPU_MIG_INVALID_GR_SYSPIPE_ID			(~U32(0U))

/** Maximum engine slot count. */
#define NVGPU_MIG_MAX_ENGINES				32U

/** Maximum config name size. */
#define NVGPU_MIG_MAX_CONFIG_NAME_SIZE			256U

/**
 * @brief GR syspipe information.
 * This struct describes the number of gpc, physical_gpc_mask, veid, etc
 * associated to a particualr gr syspipe.
 */
struct nvgpu_gr_syspipe {
	/** GR sys pipe instance Id */
	u32 gr_instance_id;
	/** GR syspipe id which is used to set gr remap window */
	u32 gr_syspipe_id;
	/**
	 * The unique per-device ID that host uses to identify any given engine.
	 */
	u32 engine_id;
	/** Number of GPC assigned to this gr syspipe. */
	u32 num_gpc;
	/**
	 * Mask of Physical GPCs. A set bit indicates GPC is available,
	 * otherwise it is not available.
	 */
	u32 physical_gpc_mask;
	/**
	 * Mask of Logical GPCs. A set bit indicates GPC is available,
	 * otherwise it is not available.
	 */
	u32 logical_gpc_mask;
	/**
	 * Mask of local GPCs belongs to this syspipe. A set bit indicates
	 * GPC is available, otherwise it is not available.
	 */
	u32 gpc_mask;
	/** Maximum veid allocated to this gr syspipe. */
	u32 max_veid_count_per_tsg;
	/** VEID start offset. */
	u32 veid_start_offset;
	/** GPC group Id. */
	u32 gpcgrp_id;
};

/**
 * @brief GPU instance information.
 * This struct describes the gr_syspipe, LCEs, etc associated
 * to a particualr gpu instance.
 */
struct nvgpu_gpu_instance {
	/** GPU instance Id */
	u32 gpu_instance_id;
	/** GR syspipe information. */
	struct nvgpu_gr_syspipe gr_syspipe;
	/** Number of Logical CE engine associated to this gpu instances. */
	u32 num_lce;
	/** Memory area to store h/w CE engine ids. */
	u32 lce_engine_ids[NVGPU_MIG_MAX_ENGINES];
	/* Flag to indicate whether memory partition is supported or not. */
	bool is_memory_partition_supported;
};

/**
 * @brief GPU instance configuration information.
 * This struct describes the number of gpu instances, gr_syspipe, LCEs, etc
 * associated to a particualr mig config.
 */
struct nvgpu_gpu_instance_config {
	/** Name of the gpu instance config. */
	const char config_name[NVGPU_MIG_MAX_CONFIG_NAME_SIZE];
	/** Number of gpu instance associated to this config. */
	u32 num_gpu_instances;
	/** Array of gpu instance information associated to this config. */
	struct nvgpu_gpu_instance
		gpu_instance[NVGPU_MIG_MAX_GPU_INSTANCES];
};

/**
 * @brief MIG configuration options.
 * This struct describes the various number of mig gpu instance configuration
 * supported by a particual GPU.
 */
struct nvgpu_mig_gpu_instance_config {
	/** Number of gpu instance configurations. */
	u32 num_config_supported;
	/** GPC count associated to each GPC group. */
	u32 gpcgrp_gpc_count[NVGPU_MIG_MAX_GPCGRP];
	/** Array of gpu instance configuration information. */
	struct nvgpu_gpu_instance_config
		gpu_instance_config[NVGPU_MIG_MAX_MIG_CONFIG_COUNT];
};

/**
 * @brief Multi Instance GPU information.
 * This struct describes the mig top level information supported
 * by a particual GPU.
 */
struct nvgpu_mig {
	/** GPC count associated to each GPC group. */
	u32 gpcgrp_gpc_count[NVGPU_MIG_MAX_GPCGRP];
	/** Enabled gpu instances count. */
	u32 num_gpu_instances;
	/** Maximum gr sys pipes are supported by HW. */
	u32 max_gr_sys_pipes_supported;
	/** Total number of enabled gr syspipes count. */
	u32 num_gr_sys_pipes_enabled;
	/** GR sys pipe enabled mask. */
	u32 gr_syspipe_en_mask;
	/**
	 * Current gr syspipe id.
	 * It is valid if num_gr_sys_pipes_enabled > 1.
	 */
	u32 current_gr_syspipe_id;
	/**
	 * GR syspipe acquire lock.
	 * It is valid lock if num_gr_sys_pipes_enabled > 1.
	 */
	struct nvgpu_mutex gr_syspipe_lock;
	/** Gpu instance configuration id. */
	u32 current_gpu_instance_config_id;
	/**
	 * Flag to indicate whether nonGR(CE) engine is sharable
	 * between gr syspipes or not.
	 */
	bool is_nongr_engine_sharable;
	/** Array of enabled gpu instance information. */
	struct nvgpu_gpu_instance
		gpu_instance[NVGPU_MIG_MAX_GPU_INSTANCES];
};

#endif /* NVGPU_MIG_H */
