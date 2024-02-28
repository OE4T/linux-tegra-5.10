/*
 * Copyright (c) 2017-2022, NVIDIA CORPORATION.  All rights reserved.
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
#include <nvgpu/log.h>
#include <nvgpu/bitops.h>
#include <nvgpu/nvgpu_mem.h>
#include <nvgpu/dma.h>
#include <nvgpu/hw_sim_pci.h>
#include <nvgpu/sim.h>
#include <nvgpu/io.h>
#include <nvgpu/utils.h>
#include <nvgpu/bug.h>
#include <nvgpu/string.h>

static inline u32 pci_sim_msg_header_size(void)
{
	return 32U;
}

static inline u32 *pci_sim_msg_param(struct gk20a *g, u32 byte_offset)
{
	/* starts after msg header/cmn */
	return sim_msg_bfr(g, byte_offset + pci_sim_msg_header_size());
}

static inline void pci_sim_write_hdr(struct gk20a *g, u32 func, u32 size)
{
	*sim_msg_hdr(g, sim_msg_header_version_r()) =
		sim_msg_header_version_major_tot_v() |
		sim_msg_header_version_minor_tot_v();
	*sim_msg_hdr(g, sim_msg_signature_r()) = sim_msg_signature_valid_v();
	*sim_msg_hdr(g, sim_msg_result_r())    = sim_msg_result_rpc_pending_v();
	*sim_msg_hdr(g, sim_msg_spare_r())     = sim_msg_spare__init_v();
	*sim_msg_hdr(g, sim_msg_function_r())  = func;
	*sim_msg_hdr(g, sim_msg_length_r())    =
		size + pci_sim_msg_header_size();
}

static u32 *sim_send_ring_bfr(struct gk20a *g, u32 byte_offset)
{
	u8 *cpu_va;

	cpu_va = (u8 *)g->sim->send_bfr.cpu_va;

	return (u32 *)(cpu_va + byte_offset);
}

static int rpc_send_message(struct gk20a *g)
{
	/* calculations done in units of u32s */
	u32 send_base = sim_send_put_pointer_v(g->sim->send_ring_put) * 2;
	u32 dma_offset = send_base + sim_dma_r()/(u32)sizeof(u32);
	u32 dma_hi_offset = send_base + sim_dma_hi_r()/4U;

	*sim_send_ring_bfr(g, dma_offset*4U) =
		sim_dma_target_phys_pci_coherent_f() |
		sim_dma_status_valid_f() |
		sim_dma_size_4kb_f() |
		sim_dma_addr_lo_f((u32)(nvgpu_mem_get_phys_addr(g, &g->sim->msg_bfr)
				>> sim_dma_addr_lo_b()));

	*sim_send_ring_bfr(g, dma_hi_offset*4U) =
		u64_hi32(nvgpu_mem_get_phys_addr(g, &g->sim->msg_bfr));

	*sim_msg_hdr(g, sim_msg_sequence_r()) = g->sim->sequence_base++;

	g->sim->send_ring_put = (g->sim->send_ring_put + 2 * 4U) %
		SIM_BFR_SIZE;

	/* Update the put pointer. This will trap into the host. */
	sim_writel(g->sim, sim_send_put_r(), g->sim->send_ring_put);

	return 0;
}

static inline u32 *sim_recv_ring_bfr(struct gk20a *g, u32 byte_offset)
{
	u8 *cpu_va;

	cpu_va = (u8 *)g->sim->recv_bfr.cpu_va;

	return (u32 *)(cpu_va + byte_offset);
}

static int rpc_recv_poll(struct gk20a *g)
{
	u64 recv_phys_addr;

	/* Poll the recv ring get pointer in an infinite loop */
	do {
		g->sim->recv_ring_put = sim_readl(g->sim, sim_recv_put_r());
	} while (g->sim->recv_ring_put == g->sim->recv_ring_get);

	/* process all replies */
	while (g->sim->recv_ring_put != g->sim->recv_ring_get) {
		/* these are in u32 offsets */
		u32 dma_lo_offset =
			sim_recv_put_pointer_v(g->sim->recv_ring_get)*2 + 0;
		u32 dma_hi_offset = dma_lo_offset + 1;
		u32 recv_phys_addr_lo = sim_dma_addr_lo_v(
				*sim_recv_ring_bfr(g, dma_lo_offset*4));
		u32 recv_phys_addr_hi = sim_dma_hi_addr_v(
				*sim_recv_ring_bfr(g, dma_hi_offset*4));

		recv_phys_addr = (u64)recv_phys_addr_hi << 32 |
				 (u64)recv_phys_addr_lo << sim_dma_addr_lo_b();

		if (recv_phys_addr !=
				nvgpu_mem_get_phys_addr(g, &g->sim->msg_bfr)) {
			nvgpu_err(g, "Error in RPC reply");
			return -EINVAL;
		}

		/* Update GET pointer */
		g->sim->recv_ring_get = (g->sim->recv_ring_get + 2*4U)
			% SIM_BFR_SIZE;

		sim_writel(g->sim, sim_recv_get_r(), g->sim->recv_ring_get);

		g->sim->recv_ring_put = sim_readl(g->sim, sim_recv_put_r());
	}

	return 0;
}

static int pci_issue_rpc_and_wait(struct gk20a *g)
{
	int err;

	err = rpc_send_message(g);
	if (err != 0) {
		nvgpu_err(g, "failed rpc_send_message");
		return err;
	}

	err = rpc_recv_poll(g);
	if (err != 0) {
		nvgpu_err(g, "failed rpc_recv_poll");
		return err;
	}

	/* Now check if RPC really succeeded */
	if (*sim_msg_hdr(g, sim_msg_result_r()) != sim_msg_result_success_v()) {
		nvgpu_err(g, "received failed status!");
		return -EINVAL;
	}
	return 0;
}

static void nvgpu_sim_esc_readl(struct gk20a *g,
		const char *path, u32 index, u32 *data)
{
	int err;
	size_t pathlen = strlen(path);
	u32 data_offset;

	pci_sim_write_hdr(g, sim_msg_function_sim_escape_read_v(),
		      sim_escape_read_hdr_size());
	*pci_sim_msg_param(g, 0) = index;
	*pci_sim_msg_param(g, 4) = 4U;
	data_offset = (u32)(round_up(pathlen + 1, 4U));
	*pci_sim_msg_param(g, 8) = data_offset;
	strcpy((char *)pci_sim_msg_param(g, sim_escape_read_hdr_size()), path);

	err = pci_issue_rpc_and_wait(g);

	if (err == 0) {
		nvgpu_memcpy((u8 *)data,
			(u8 *)pci_sim_msg_param(g,
				nvgpu_safe_add_u32(data_offset,
					sim_escape_read_hdr_size())),
			4U);
	} else {
		*data = 0xffffffff;
		WARN(1, "pci_issue_rpc_and_wait failed err=%d", err);
	}
}

static int nvgpu_sim_init_late(struct gk20a *g)
{
	u64 phys;
	int err = -ENOMEM;

	nvgpu_info(g, "sim init late pci");

	if (!g->sim) {
		return 0;
	}

	/* allocate sim event/msg buffers */
	err = nvgpu_alloc_sim_buffer(g, &g->sim->send_bfr);
	err = err || nvgpu_alloc_sim_buffer(g, &g->sim->recv_bfr);
	err = err || nvgpu_alloc_sim_buffer(g, &g->sim->msg_bfr);

	if (err != 0) {
		goto fail;
	}

	/* mark send ring invalid */
	sim_writel(g->sim, sim_send_ring_r(), sim_send_ring_status_invalid_f());

	/* read get pointer and make equal to put */
	g->sim->send_ring_put = sim_readl(g->sim, sim_send_get_r());
	sim_writel(g->sim, sim_send_put_r(), g->sim->send_ring_put);

	/* write send ring address and make it valid */
	phys = nvgpu_mem_get_phys_addr(g, &g->sim->send_bfr);
	sim_writel(g->sim, sim_send_ring_hi_r(),
		   sim_send_ring_hi_addr_f(u64_hi32(phys)));
	sim_writel(g->sim, sim_send_ring_r(),
		   sim_send_ring_status_valid_f() |
		   sim_send_ring_target_phys_pci_coherent_f() |
		   sim_send_ring_size_4kb_f() |
		   sim_send_ring_addr_lo_f((u32)(phys >> sim_send_ring_addr_lo_b())));

	/* repeat for recv ring (but swap put,get as roles are opposite) */
	sim_writel(g->sim, sim_recv_ring_r(), sim_recv_ring_status_invalid_f());

	/* read put pointer and make equal to get */
	g->sim->recv_ring_get = sim_readl(g->sim, sim_recv_put_r());
	sim_writel(g->sim, sim_recv_get_r(), g->sim->recv_ring_get);

	/* write send ring address and make it valid */
	phys = nvgpu_mem_get_phys_addr(g, &g->sim->recv_bfr);
	sim_writel(g->sim, sim_recv_ring_hi_r(),
		   sim_recv_ring_hi_addr_f(u64_hi32(phys)));
	sim_writel(g->sim, sim_recv_ring_r(),
		   sim_recv_ring_status_valid_f() |
		   sim_recv_ring_target_phys_pci_coherent_f() |
		   sim_recv_ring_size_4kb_f() |
		   sim_recv_ring_addr_lo_f((u32)(phys >> sim_recv_ring_addr_lo_b())));

	return 0;
 fail:
	nvgpu_free_sim_support(g);
	return err;
}

int nvgpu_init_sim_support_pci(struct gk20a *g)
{

	if(!g->sim) {
		return 0;
	}

	g->sim->sim_init_late = nvgpu_sim_init_late;
	g->sim->remove_support = nvgpu_remove_sim_support;
	g->sim->esc_readl = nvgpu_sim_esc_readl;
	return 0;

}
