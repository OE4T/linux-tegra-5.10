/*
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NVGPU_LINUX_LOG_H
#define NVGPU_LINUX_LOG_H

#include <nvgpu/types.h>
#include <nvgpu/bitops.h>
#include <nvgpu/log_common.h>

struct gk20a;

__attribute__((format (printf, 5, 6)))
void nvgpu_log_msg_impl(struct gk20a *g, const char *func_name, int line,
			enum nvgpu_log_type type, const char *fmt, ...);

__attribute__((format (printf, 5, 6)))
void nvgpu_log_dbg_impl(struct gk20a *g, u64 log_mask,
			const char *func_name, int line,
			const char *fmt, ...);

/**
 * nvgpu_log_impl - Print a debug message
 */
#define nvgpu_log_impl(g, log_mask, fmt, arg...)			\
	nvgpu_log_dbg_impl(g, log_mask, __func__, __LINE__, fmt, ##arg)

/**
 * nvgpu_err_impl - Print an error
 */
#define nvgpu_err_impl(g, fmt, arg...)					\
	nvgpu_log_msg_impl(g, __func__, __LINE__, NVGPU_ERROR, fmt, ##arg)

/**
 * nvgpu_warn_impl - Print a warning
 */
#define nvgpu_warn_impl(g, fmt, arg...)					\
	nvgpu_log_msg_impl(g, __func__, __LINE__, NVGPU_WARNING, fmt, ##arg)

/**
 * nvgpu_info_impl - Print an info message
 */
#define nvgpu_info_impl(g, fmt, arg...)					\
	nvgpu_log_msg_impl(g, __func__, __LINE__, NVGPU_INFO, fmt, ##arg)

/*
 * Deprecated API. Do not use!!
 */

#define gk20a_dbg_impl(log_mask, fmt, arg...)				\
	do {								\
		if (((log_mask) & NVGPU_DEFAULT_DBG_MASK) != 0)	{	\
			nvgpu_log_msg_impl(NULL, __func__, __LINE__,	\
					NVGPU_DEBUG, fmt "\n", ##arg);	\
		}							\
	} while (false)

#endif