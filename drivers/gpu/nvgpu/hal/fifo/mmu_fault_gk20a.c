/*
 * Copyright (c) 2011-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <trace/events/gk20a.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/timers.h>
#include <nvgpu/log.h>
#include <nvgpu/io.h>
#include <nvgpu/fifo.h>
#include <nvgpu/engines.h>

#include <hal/fifo/mmu_fault_gk20a.h>

#include <nvgpu/hw/gk20a/hw_fifo_gk20a.h>

/* fault info/descriptions */

static const char * const gk20a_fault_type_descs[] = {
	 "pde", /*fifo_intr_mmu_fault_info_type_pde_v() == 0 */
	 "pde size",
	 "pte",
	 "va limit viol",
	 "unbound inst",
	 "priv viol",
	 "ro viol",
	 "wo viol",
	 "pitch mask",
	 "work creation",
	 "bad aperture",
	 "compression failure",
	 "bad kind",
	 "region viol",
	 "dual ptes",
	 "poisoned",
};

/* engine descriptions */
static const char * const engine_subid_descs[] = {
	"gpc",
	"hub",
};

static const char * const gk20a_hub_client_descs[] = {
	"vip", "ce0", "ce1", "dniso", "fe", "fecs", "host", "host cpu",
	"host cpu nb", "iso", "mmu", "mspdec", "msppp", "msvld",
	"niso", "p2p", "pd", "perf", "pmu", "raster twod", "scc",
	"scc nb", "sec", "ssync", "gr copy", "xv", "mmu nb",
	"msenc", "d falcon", "sked", "a falcon", "n/a",
};

static const char * const gk20a_gpc_client_descs[] = {
	"l1 0", "t1 0", "pe 0",
	"l1 1", "t1 1", "pe 1",
	"l1 2", "t1 2", "pe 2",
	"l1 3", "t1 3", "pe 3",
	"rast", "gcc", "gpccs",
	"prop 0", "prop 1", "prop 2", "prop 3",
	"l1 4", "t1 4", "pe 4",
	"l1 5", "t1 5", "pe 5",
	"l1 6", "t1 6", "pe 6",
	"l1 7", "t1 7", "pe 7",
};

static const char * const does_not_exist[] = {
	"does not exist"
};

/* fill in mmu fault desc */
void gk20a_fifo_get_mmu_fault_desc(struct mmu_fault_info *mmufault)
{
	if (mmufault->fault_type >= ARRAY_SIZE(gk20a_fault_type_descs)) {
		WARN_ON(mmufault->fault_type >=
				ARRAY_SIZE(gk20a_fault_type_descs));
	} else {
		mmufault->fault_type_desc =
			 gk20a_fault_type_descs[mmufault->fault_type];
	}
}

/* fill in mmu fault client description */
void gk20a_fifo_get_mmu_fault_client_desc(struct mmu_fault_info *mmufault)
{
	if (mmufault->client_id >= ARRAY_SIZE(gk20a_hub_client_descs)) {
		WARN_ON(mmufault->client_id >=
				ARRAY_SIZE(gk20a_hub_client_descs));
	} else {
		mmufault->client_id_desc =
			 gk20a_hub_client_descs[mmufault->client_id];
	}
}

/* fill in mmu fault gpc description */
void gk20a_fifo_get_mmu_fault_gpc_desc(struct mmu_fault_info *mmufault)
{
	if (mmufault->client_id >= ARRAY_SIZE(gk20a_gpc_client_descs)) {
		WARN_ON(mmufault->client_id >=
				ARRAY_SIZE(gk20a_gpc_client_descs));
	} else {
		mmufault->client_id_desc =
			 gk20a_gpc_client_descs[mmufault->client_id];
	}
}

static void gk20a_fifo_parse_mmu_fault_info(struct gk20a *g, u32 mmu_fault_id,
	struct mmu_fault_info *mmufault)
{
	g->ops.fifo.get_mmu_fault_info(g, mmu_fault_id, mmufault);

	/* parse info */
	mmufault->fault_type_desc =  does_not_exist[0];
	if (g->ops.fifo.get_mmu_fault_desc != NULL) {
		g->ops.fifo.get_mmu_fault_desc(mmufault);
	}

	if (mmufault->client_type >= ARRAY_SIZE(engine_subid_descs)) {
		WARN_ON(mmufault->client_type >=
				ARRAY_SIZE(engine_subid_descs));
		mmufault->client_type_desc = does_not_exist[0];
	} else {
		mmufault->client_type_desc =
				 engine_subid_descs[mmufault->client_type];
	}

	mmufault->client_id_desc = does_not_exist[0];
	if ((mmufault->client_type ==
		fifo_intr_mmu_fault_info_engine_subid_hub_v())
		&& (g->ops.fifo.get_mmu_fault_client_desc != NULL)) {
		g->ops.fifo.get_mmu_fault_client_desc(mmufault);
	} else if ((mmufault->client_type ==
			fifo_intr_mmu_fault_info_engine_subid_gpc_v())
			&& (g->ops.fifo.get_mmu_fault_gpc_desc != NULL)) {
		g->ops.fifo.get_mmu_fault_gpc_desc(mmufault);
	}
}

/* reads info from hardware and fills in mmu fault info record */
void gk20a_fifo_get_mmu_fault_info(struct gk20a *g, u32 mmu_fault_id,
	struct mmu_fault_info *mmufault)
{
	u32 fault_info;
	u32 addr_lo, addr_hi;

	nvgpu_log_fn(g, "mmu_fault_id %d", mmu_fault_id);

	(void) memset(mmufault, 0, sizeof(*mmufault));

	fault_info = nvgpu_readl(g,
		fifo_intr_mmu_fault_info_r(mmu_fault_id));
	mmufault->fault_type =
		fifo_intr_mmu_fault_info_type_v(fault_info);
	mmufault->access_type =
		fifo_intr_mmu_fault_info_write_v(fault_info);
	mmufault->client_type =
		fifo_intr_mmu_fault_info_engine_subid_v(fault_info);
	mmufault->client_id =
		fifo_intr_mmu_fault_info_client_v(fault_info);

	addr_lo = nvgpu_readl(g, fifo_intr_mmu_fault_lo_r(mmu_fault_id));
	addr_hi = nvgpu_readl(g, fifo_intr_mmu_fault_hi_r(mmu_fault_id));
	mmufault->fault_addr = hi32_lo32_to_u64(addr_hi, addr_lo);
	/* note:ignoring aperture on gk20a... */
	mmufault->inst_ptr = fifo_intr_mmu_fault_inst_ptr_v(
		 nvgpu_readl(g, fifo_intr_mmu_fault_inst_r(mmu_fault_id)));
	/* note: inst_ptr is a 40b phys addr.  */
	mmufault->inst_ptr <<= fifo_intr_mmu_fault_inst_ptr_align_shift_v();
}

void gk20a_fifo_mmu_fault_info_dump(struct gk20a *g, u32 engine_id,
	u32 mmu_fault_id, bool fake_fault, struct mmu_fault_info *mmufault)
{
	gk20a_fifo_parse_mmu_fault_info(g, mmu_fault_id, mmufault);

	trace_gk20a_mmu_fault(mmufault->fault_addr,
			      mmufault->fault_type,
			      mmufault->access_type,
			      mmufault->inst_ptr,
			      engine_id,
			      mmufault->client_type_desc,
			      mmufault->client_id_desc,
			      mmufault->fault_type_desc);
	nvgpu_err(g, "MMU fault @ address: 0x%llx %s",
		  mmufault->fault_addr,
		  fake_fault ? "[FAKE]" : "");
	nvgpu_err(g, "  Engine: %d  subid: %d (%s)",
		  (int)engine_id,
		  mmufault->client_type, mmufault->client_type_desc);
	nvgpu_err(g, "  Cient %d (%s), ",
		  mmufault->client_id, mmufault->client_id_desc);
	nvgpu_err(g, "  Tpe %d (%s); access_type 0x%08x; inst_ptr 0x%llx",
		  mmufault->fault_type,
		  mmufault->fault_type_desc,
		  mmufault->access_type, mmufault->inst_ptr);
}

void gk20a_fifo_handle_dropped_mmu_fault(struct gk20a *g)
{
	u32 fault_id = gk20a_readl(g, fifo_intr_mmu_fault_id_r());

	nvgpu_err(g, "dropped mmu fault (0x%08x)", fault_id);
}
