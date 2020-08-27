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

#ifndef NVGPU_GR_INSTANCES_H
#define NVGPU_GR_INSTANCES_H

#include <nvgpu/types.h>
#include <nvgpu/grmgr.h>
#include <nvgpu/gr/gr.h>

#ifdef CONFIG_NVGPU_MIG
#define nvgpu_gr_exec_for_each_instance(g, func) \
	({ \
		if (nvgpu_is_enabled(g, NVGPU_SUPPORT_MIG)) { \
			u32 gr_instance_id = 0U; \
			while (gr_instance_id < g->num_gr_instances) { \
				u32 gr_syspipe_id = nvgpu_gr_get_syspipe_id(g, gr_instance_id); \
				nvgpu_grmgr_config_gr_remap_window(g, gr_syspipe_id, true); \
				g->cur_gr_instance = gr_instance_id; \
				(func); \
				nvgpu_grmgr_config_gr_remap_window(g, gr_syspipe_id, false); \
			} \
		} else { \
			(func); \
		} \
	})
#else
#define nvgpu_gr_exec_for_each_instance(g, func)	(func)
#endif

#ifdef CONFIG_NVGPU_MIG
#define nvgpu_gr_exec_with_ret_for_each_instance(g, func) \
	({ \
		int err = 0; \
		if (nvgpu_is_enabled(g, NVGPU_SUPPORT_MIG)) { \
			u32 gr_instance_id = 0U; \
			while (gr_instance_id < g->num_gr_instances) { \
				u32 gr_syspipe_id = nvgpu_gr_get_syspipe_id(g, gr_instance_id); \
				nvgpu_grmgr_config_gr_remap_window(g, gr_syspipe_id, true); \
				g->cur_gr_instance = gr_instance_id; \
				err = (func); \
				if (err != 0) { \
					break; \
				} \
				nvgpu_grmgr_config_gr_remap_window(g, gr_syspipe_id, false); \
			} \
		} else { \
			err = (func); \
		} \
		err; \
	})
#else
#define nvgpu_gr_exec_with_ret_for_each_instance(g, func)	(func)
#endif

#ifdef CONFIG_NVGPU_MIG
#define nvgpu_gr_exec_for_all_instances(g, func) \
	({ \
		if (nvgpu_is_enabled(g, NVGPU_SUPPORT_MIG)) { \
			nvgpu_grmgr_config_gr_remap_window(g, NVGPU_MIG_INVALID_GR_SYSPIPE_ID, false); \
			g->cur_gr_instance = 0; \
			(func); \
			nvgpu_grmgr_config_gr_remap_window(g, NVGPU_MIG_INVALID_GR_SYSPIPE_ID, true); \
		} else { \
			(func); \
		} \
	})
#else
#define nvgpu_gr_exec_for_all_instances(g, func)	(func)
#endif

#ifdef CONFIG_NVGPU_MIG
#define nvgpu_gr_exec_for_instance(g, gr_instance_id, func) \
	({ \
		if (nvgpu_is_enabled(g, NVGPU_SUPPORT_MIG)) { \
			u32 gr_syspipe_id = nvgpu_gr_get_syspipe_id(g, gr_instance_id); \
			nvgpu_grmgr_config_gr_remap_window(g, gr_syspipe_id, true); \
			g->cur_gr_instance = gr_instance_id; \
			(func); \
			nvgpu_grmgr_config_gr_remap_window(g, gr_syspipe_id, false); \
		} else { \
			(func); \
		} \
	})
#else
#define nvgpu_gr_exec_for_instance(g, gr_instance_id, func)	(func)
#endif

#ifdef CONFIG_NVGPU_MIG
#define nvgpu_gr_exec_with_ret_for_instance(g, gr_instance_id, func) \
	({ \
		int err = 0; \
		if (nvgpu_is_enabled(g, NVGPU_SUPPORT_MIG)) { \
			u32 gr_syspipe_id = nvgpu_gr_get_syspipe_id(g, gr_instance_id); \
			nvgpu_grmgr_config_gr_remap_window(g, gr_syspipe_id, true); \
			g->cur_gr_instance = gr_instance_id; \
			err = (func); \
			nvgpu_grmgr_config_gr_remap_window(g, gr_syspipe_id, false); \
		} else { \
			err = (func); \
		} \
		err; \
	})
#else
#define nvgpu_gr_exec_with_ret_for_instance(g, gr_instance_id, func)	(func)
#endif

#endif /* NVGPU_GR_INSTANCES_H */
