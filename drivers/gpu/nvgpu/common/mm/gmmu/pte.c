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

#include <nvgpu/gk20a.h>
#include <nvgpu/static_analysis.h>
#include <nvgpu/string.h>
#include <nvgpu/gmmu.h>

u32 nvgpu_gmmu_default_big_page_size(void)
{
	return U32(SZ_64K);
}

/*
 * MSS NVLINK HW settings are in force_snoop mode.
 * This will force all the GPU mappings to be coherent.
 * By default the mem aperture is set to sysmem_non_coherent and will use L2
 * atomics.
 * Change target pte aperture to sysmem_coherent if mem attribute requests for
 * platform atomics to use rmw atomic capability.
 *
 */
u32 nvgpu_gmmu_aperture_mask(struct gk20a *g,
				  enum nvgpu_aperture mem_ap,
				  bool platform_atomic_attr,
				  u32 sysmem_mask,
				  u32 sysmem_coh_mask,
				  u32 vidmem_mask)
{
	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_PLATFORM_ATOMIC) &&
			     platform_atomic_attr) {
		mem_ap = APERTURE_SYSMEM_COH;
	}

	return nvgpu_aperture_mask_raw(g, mem_ap,
				sysmem_mask,
				sysmem_coh_mask,
				vidmem_mask);
}

static char *map_attrs_to_str(char *dest, struct nvgpu_gmmu_attrs *attrs)
{
	dest[0] = attrs->cacheable ? 'C' : '-';
	dest[1] = attrs->sparse    ? 'S' : '-';
	dest[2] = attrs->priv      ? 'P' : '-';
	dest[3] = attrs->valid     ? 'V' : '-';
	dest[4] = attrs->platform_atomic ? 'A' : '-';
	dest[5] = '\0';

	return dest;
}

void nvgpu_pte_dbg_print(struct gk20a *g,
		struct nvgpu_gmmu_attrs *attrs,
		const char *vm_name, u32 pd_idx, u32 mmu_level_entry_size,
		u64 virt_addr, u64 phys_addr, u32 page_size, u32 *pte_w)
{
	char attrs_str[6];
	char ctag_str[32] = "\0";
	const char *aperture_str = nvgpu_aperture_str(attrs->aperture);
	const char *perm_str = nvgpu_gmmu_perm_str(attrs->rw_flag);
#ifdef CONFIG_NVGPU_COMPRESSION
	u32 ctag = nvgpu_safe_cast_u64_to_u32(attrs->ctag /
					g->ops.fb.compression_page_size(g));
	(void)strcpy(ctag_str, "ctag=0x");
	(void)nvgpu_strnadd_u32(ctag_str, ctag, (u32)strlen(ctag_str), 10U);
#endif
	(void)map_attrs_to_str(attrs_str, attrs);
	pte_dbg(g, attrs,
		"vm=%s "
		"PTE: i=%-4u size=%-2u | "
		"GPU %#-12llx  phys %#-12llx "
		"pgsz: %3dkb perm=%-2s kind=%#02x APT=%-6s %-5s "
		"%s "
		"[0x%08x, 0x%08x]",
		vm_name,
		pd_idx, mmu_level_entry_size,
		virt_addr, phys_addr,
		page_size >> 10,
		perm_str,
		attrs->kind_v,
		aperture_str,
		attrs_str,
		ctag_str,
		pte_w[1], pte_w[0]);
}
