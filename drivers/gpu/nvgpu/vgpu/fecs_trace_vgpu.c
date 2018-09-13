/*
 * Copyright (c) 2016-2018, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/kmem.h>
#include <nvgpu/bug.h>
#include <nvgpu/enabled.h>
#include <nvgpu/ctxsw_trace.h>
#include <nvgpu/fecs_trace.h>
#include <nvgpu/dt.h>
#include <nvgpu/vgpu/vgpu_ivm.h>
#include <nvgpu/vgpu/tegra_vgpu.h>
#include <nvgpu/vgpu/vgpu.h>
#include <nvgpu/gk20a.h>

#include "gk20a/fecs_trace_gk20a.h"
#include "vgpu/fecs_trace_vgpu.h"


int vgpu_fecs_trace_init(struct gk20a *g)
{
	struct vgpu_fecs_trace *vcst;
	u32 mempool;
	int err;

	nvgpu_log_fn(g, " ");

	if (g->fecs_trace)
		return 0;

	vcst = nvgpu_kzalloc(g, sizeof(*vcst));
	if (!vcst)
		return -ENOMEM;

	err = nvgpu_dt_read_u32_index(g, "mempool-fecs-trace", 1, &mempool);
	if (err) {
		nvgpu_info(g, "does not support fecs trace");
		goto fail;
	}
	__nvgpu_set_enabled(g, NVGPU_SUPPORT_FECS_CTXSW_TRACE, true);

	vcst->cookie = vgpu_ivm_mempool_reserve(mempool);
	if (IS_ERR(vcst->cookie)) {
		nvgpu_info(g,
			"mempool  %u reserve failed", mempool);
		vcst->cookie = NULL;
		err = -EINVAL;
		goto fail;
	}

	vcst->buf = vgpu_ivm_mempool_map(vcst->cookie);
	if (!vcst->buf) {
		nvgpu_info(g, "ioremap_cache failed");
		err = -EINVAL;
		goto fail;
	}
	vcst->header = vcst->buf;
	vcst->num_entries = vcst->header->num_ents;
	if (unlikely(vcst->header->ent_size != sizeof(*vcst->entries))) {
		nvgpu_err(g, "entry size mismatch");
		goto fail;
	}
	vcst->entries = (struct nvgpu_gpu_ctxsw_trace_entry *)(
			(char *)vcst->buf + sizeof(*vcst->header));
	g->fecs_trace = (struct gk20a_fecs_trace *)vcst;

	return 0;
fail:
	if (vcst->cookie != NULL && vcst->buf != NULL) {
		vgpu_ivm_mempool_unmap(vcst->cookie, vcst->buf);
	}
	if (vcst->cookie)
		vgpu_ivm_mempool_unreserve(vcst->cookie);
	nvgpu_kfree(g, vcst);
	return err;
}

int vgpu_fecs_trace_deinit(struct gk20a *g)
{
	struct vgpu_fecs_trace *vcst = (struct vgpu_fecs_trace *)g->fecs_trace;

	vgpu_ivm_mempool_unmap(vcst->cookie, vcst->buf);
	vgpu_ivm_mempool_unreserve(vcst->cookie);
	nvgpu_kfree(g, vcst);
	return 0;
}

int vgpu_fecs_trace_enable(struct gk20a *g)
{
	struct vgpu_fecs_trace *vcst = (struct vgpu_fecs_trace *)g->fecs_trace;
	struct tegra_vgpu_cmd_msg msg = {
		.cmd = TEGRA_VGPU_CMD_FECS_TRACE_ENABLE,
		.handle = vgpu_get_handle(g),
	};
	int err;

	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
	err = err ? err : msg.ret;
	WARN_ON(err);
	vcst->enabled = !err;
	return err;
}

int vgpu_fecs_trace_disable(struct gk20a *g)
{
	struct vgpu_fecs_trace *vcst = (struct vgpu_fecs_trace *)g->fecs_trace;
	struct tegra_vgpu_cmd_msg msg = {
		.cmd = TEGRA_VGPU_CMD_FECS_TRACE_DISABLE,
		.handle = vgpu_get_handle(g),
	};
	int err;

	vcst->enabled = false;
	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
	err = err ? err : msg.ret;
	WARN_ON(err);
	return err;
}

bool vgpu_fecs_trace_is_enabled(struct gk20a *g)
{
	struct vgpu_fecs_trace *vcst = (struct vgpu_fecs_trace *)g->fecs_trace;

	return (vcst && vcst->enabled);
}

int vgpu_fecs_trace_poll(struct gk20a *g)
{
	struct tegra_vgpu_cmd_msg msg = {
		.cmd = TEGRA_VGPU_CMD_FECS_TRACE_POLL,
		.handle = vgpu_get_handle(g),
	};
	int err;

	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
	err = err ? err : msg.ret;
	WARN_ON(err);
	return err;
}

int vgpu_free_user_buffer(struct gk20a *g)
{
	return 0;
}


#ifdef CONFIG_GK20A_CTXSW_TRACE
int vgpu_fecs_trace_max_entries(struct gk20a *g,
			struct nvgpu_gpu_ctxsw_trace_filter *filter)
{
	struct vgpu_fecs_trace *vcst = (struct vgpu_fecs_trace *)g->fecs_trace;

	return vcst->header->num_ents;
}

int vgpu_fecs_trace_set_filter(struct gk20a *g,
			struct nvgpu_gpu_ctxsw_trace_filter *filter)
{
	struct tegra_vgpu_cmd_msg msg = {
		.cmd = TEGRA_VGPU_CMD_FECS_TRACE_SET_FILTER,
		.handle = vgpu_get_handle(g),
	};
	struct tegra_vgpu_fecs_trace_filter *p = &msg.params.fecs_trace_filter;
	int err;

	(void) memcpy(&p->tag_bits, &filter->tag_bits, sizeof(p->tag_bits));
	err = vgpu_comm_sendrecv(&msg, sizeof(msg), sizeof(msg));
	err = err ? err : msg.ret;
	WARN_ON(err);
	return err;
}

#endif /* CONFIG_GK20A_CTXSW_TRACE */
