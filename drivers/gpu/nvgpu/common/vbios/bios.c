/*
 * Copyright (c) 2015-2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/bios.h>
#include <nvgpu/io.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/string.h>

static void nvgpu_bios_parse_bit(struct gk20a *g, u32 offset);

static u16 nvgpu_bios_rdu16(struct gk20a *g, u32 offset)
{
	u16 val = (U16(g->bios.data[offset+1U]) << U16(8)) +
		U16(g->bios.data[offset]);
	return val;
}

static u32 nvgpu_bios_rdu32(struct gk20a *g, u32 offset)
{
	u32 val = (U32(g->bios.data[offset+3U]) << U32(24)) +
		  (U32(g->bios.data[offset+2U]) << U32(16)) +
		  (U32(g->bios.data[offset+1U]) << U32(8)) +
		  U32(g->bios.data[offset]);
	return val;
}

int nvgpu_bios_parse_rom(struct gk20a *g)
{
	u32 offset = 0;
	u8 last = 0;
	bool found = false;
	unsigned int i;

	while (last == 0U) {
		struct pci_exp_rom *pci_rom;
		struct pci_data_struct *pci_data;
		struct pci_ext_data_struct *pci_ext_data;

		pci_rom = (struct pci_exp_rom *)&g->bios.data[offset];
		nvgpu_log_fn(g, "pci rom sig %04x ptr %04x block %x",
				pci_rom->sig, pci_rom->pci_data_struct_ptr,
				pci_rom->size_of_block);

		if (pci_rom->sig != PCI_EXP_ROM_SIG &&
		    pci_rom->sig != PCI_EXP_ROM_SIG_NV) {
			nvgpu_err(g, "invalid VBIOS signature");
			return -EINVAL;
		}

		pci_data =
			(struct pci_data_struct *)
			&g->bios.data[offset + pci_rom->pci_data_struct_ptr];
		nvgpu_log_fn(g, "pci data sig %08x len %d image len %x type %x last %d max %08x",
				pci_data->sig, pci_data->pci_data_struct_len,
				pci_data->image_len, pci_data->code_type,
				pci_data->last_image,
				pci_data->max_runtime_image_len);

		/* Get Base ROM Size */
		if (pci_data->code_type ==
				PCI_DATA_STRUCTURE_CODE_TYPE_VBIOS_BASE) {
			g->bios.base_rom_size = (u32)pci_data->image_len *
						PCI_ROM_IMAGE_BLOCK_SIZE;
			nvgpu_log_fn(g, "Base ROM Size: %x",
						g->bios.base_rom_size);
		}

		/* Get Expansion ROM offset:
		 * In the UEFI case, the expansion ROM where the Perf tables
		 * are located is not necessarily immediately after the base
		 * VBIOS image. Some VBIOS images uses a "private image" layout,
		 * where the order of the images is the VBIOS base block,
		 * the UEFI ROM, the expansion ROM, and then the cert. So we
		 * need to add the UEFI ROM size to offsets within the
		 * expansion ROM.
		 */
		if (pci_data->code_type ==
				PCI_DATA_STRUCTURE_CODE_TYPE_VBIOS_UEFI) {
			pci_ext_data = (struct pci_ext_data_struct *)
				&g->bios.data[(offset +
					       pci_rom->pci_data_struct_ptr +
					       pci_data->pci_data_struct_len +
					       0xfU)
					      & ~0xfU];
			nvgpu_log_fn(g, "pci ext data sig %08x rev %x len %x sub_image_len %x priv_last %d flags %x",
					pci_ext_data->sig,
					pci_ext_data->nv_pci_data_ext_rev,
					pci_ext_data->nv_pci_data_ext_len,
					pci_ext_data->sub_image_len,
					pci_ext_data->priv_last_image,
					pci_ext_data->flags);

			nvgpu_log_fn(g, "expansion rom offset %x",
					pci_data->image_len *
						PCI_ROM_IMAGE_BLOCK_SIZE);
			g->bios.expansion_rom_offset =
					(u32)pci_data->image_len *
						PCI_ROM_IMAGE_BLOCK_SIZE;
			offset += (u32)pci_ext_data->sub_image_len *
						PCI_ROM_IMAGE_BLOCK_SIZE;
			last = pci_ext_data->priv_last_image;
		} else {
			offset += (u32)pci_data->image_len *
						PCI_ROM_IMAGE_BLOCK_SIZE;
			last = pci_data->last_image;
		}
	}

	nvgpu_log_info(g, "read bios");
	for (i = 0; i < g->bios.size - 6U; i++) {
		if (nvgpu_bios_rdu16(g, i) == BIT_HEADER_ID &&
		    nvgpu_bios_rdu32(g, i+2U) ==  BIT_HEADER_SIGNATURE) {
			nvgpu_bios_parse_bit(g, i);
			found = true;
		}
	}

	if (!found) {
		return -EINVAL;
	} else {
		return 0;
	}
}

static void nvgpu_bios_parse_biosdata(struct gk20a *g, u32 offset)
{
	struct biosdata biosdata;

	nvgpu_memcpy((u8 *)&biosdata, &g->bios.data[offset], sizeof(biosdata));
	nvgpu_log_fn(g, "bios version %x, oem version %x",
			biosdata.version,
			biosdata.oem_version);

	g->bios.vbios_version = biosdata.version;
	g->bios.vbios_oem_version = biosdata.oem_version;
}

static void nvgpu_bios_parse_nvinit_ptrs(struct gk20a *g, u32 offset)
{
	struct nvinit_ptrs nvinit_ptrs;

	nvgpu_memcpy((u8 *)&nvinit_ptrs, &g->bios.data[offset],
		sizeof(nvinit_ptrs));
	nvgpu_log_fn(g, "devinit ptr %x size %d", nvinit_ptrs.devinit_tables_ptr,
			nvinit_ptrs.devinit_tables_size);
	nvgpu_log_fn(g, "bootscripts ptr %x size %d", nvinit_ptrs.bootscripts_ptr,
			nvinit_ptrs.bootscripts_size);

	g->bios.devinit_tables = &g->bios.data[nvinit_ptrs.devinit_tables_ptr];
	g->bios.devinit_tables_size = nvinit_ptrs.devinit_tables_size;
	g->bios.bootscripts = &g->bios.data[nvinit_ptrs.bootscripts_ptr];
	g->bios.bootscripts_size = nvinit_ptrs.bootscripts_size;
	g->bios.condition_table_ptr = nvinit_ptrs.condition_table_ptr;
	g->bios.nvlink_config_data_offset = nvinit_ptrs.nvlink_config_data_ptr;
}
static void nvgpu_bios_parse_memory_ptrs(struct gk20a *g, u16 offset, u8 version)
{
	struct memory_ptrs_v1 v1;
	struct memory_ptrs_v2 v2;

	switch (version) {
	case MEMORY_PTRS_V1:
		nvgpu_memcpy((u8 *)&v1, &g->bios.data[offset], sizeof(v1));
		g->bios.mem_strap_data_count = v1.mem_strap_data_count;
		g->bios.mem_strap_xlat_tbl_ptr = v1.mem_strap_xlat_tbl_ptr;
		break;
	case MEMORY_PTRS_V2:
		nvgpu_memcpy((u8 *)&v2, &g->bios.data[offset], sizeof(v2));
		g->bios.mem_strap_data_count = v2.mem_strap_data_count;
		g->bios.mem_strap_xlat_tbl_ptr = v2.mem_strap_xlat_tbl_ptr;
		break;
	default:
		nvgpu_err(g, "unknown vbios memory table version %x", version);
		break;
	}

	return;
}

static void nvgpu_bios_parse_devinit_appinfo(struct gk20a *g, u32 dmem_offset)
{
	struct devinit_engine_interface interface;

	nvgpu_memcpy((u8 *)&interface, &g->bios.devinit.dmem[dmem_offset],
		sizeof(interface));
	nvgpu_log_fn(g, "devinit version %x tables phys %x script phys %x size %d",
			interface.version,
			interface.tables_phys_base,
			interface.script_phys_base,
			interface.script_size);

	if (interface.version != 1U) {
		return;
	}
	g->bios.devinit_tables_phys_base = interface.tables_phys_base;
	g->bios.devinit_script_phys_base = interface.script_phys_base;
}

static int nvgpu_bios_parse_appinfo_table(struct gk20a *g, u32 offset)
{
	struct application_interface_table_hdr_v1 hdr;
	u32 i;

	nvgpu_memcpy((u8 *)&hdr, &g->bios.data[offset], sizeof(hdr));

	nvgpu_log_fn(g, "appInfoHdr ver %d size %d entrySize %d entryCount %d",
			hdr.version, hdr.header_size,
			hdr.entry_size, hdr.entry_count);

	if (hdr.version != 1U) {
		return 0;
	}

	offset += U32(sizeof(hdr));
	for (i = 0U; i < hdr.entry_count; i++) {
		struct application_interface_entry_v1 entry;

		nvgpu_memcpy((u8 *)&entry, &g->bios.data[offset],
			sizeof(entry));

		nvgpu_log_fn(g, "appInfo id %d dmem_offset %d",
				entry.id, entry.dmem_offset);

		if (entry.id == APPINFO_ID_DEVINIT) {
			nvgpu_bios_parse_devinit_appinfo(g, entry.dmem_offset);
		}

		offset += hdr.entry_size;
	}

	return 0;
}

static int nvgpu_bios_parse_falcon_ucode_desc(struct gk20a *g,
		struct nvgpu_bios_ucode *ucode, u32 offset)
{
	union falcon_ucode_desc udesc;
	struct falcon_ucode_desc_v2 desc;
	u8 version;
	u16 desc_size;
	int ret = 0;

	nvgpu_memcpy((u8 *)&udesc, &g->bios.data[offset], sizeof(udesc));

	if (FALCON_UCODE_IS_VERSION_AVAILABLE(udesc)) {
		version = FALCON_UCODE_GET_VERSION(udesc);
		desc_size = FALCON_UCODE_GET_DESC_SIZE(udesc);
	} else {
		size_t tmp_size = sizeof(udesc.v1);

		version = 1;
		nvgpu_assert(tmp_size <= (size_t)U16_MAX);
		desc_size = U16(tmp_size);
	}

	switch (version) {
	case 1:
		desc.stored_size = udesc.v1.hdr_size.stored_size;
		desc.uncompressed_size = udesc.v1.uncompressed_size;
		desc.virtual_entry = udesc.v1.virtual_entry;
		desc.interface_offset = udesc.v1.interface_offset;
		desc.imem_phys_base = udesc.v1.imem_phys_base;
		desc.imem_load_size = udesc.v1.imem_load_size;
		desc.imem_virt_base = udesc.v1.imem_virt_base;
		desc.imem_sec_base = udesc.v1.imem_sec_base;
		desc.imem_sec_size = udesc.v1.imem_sec_size;
		desc.dmem_offset = udesc.v1.dmem_offset;
		desc.dmem_phys_base = udesc.v1.dmem_phys_base;
		desc.dmem_load_size = udesc.v1.dmem_load_size;
		break;
	case 2:
		nvgpu_memcpy((u8 *)&desc, (u8 *)&udesc, sizeof(udesc.v2));
		break;
	default:
		nvgpu_log_info(g, "invalid version");
		ret = -EINVAL;
		break;
	}
	if (ret != 0) {
		return ret;
	}

	nvgpu_log_info(g, "falcon ucode desc version %x len %x", version, desc_size);

	nvgpu_log_info(g, "falcon ucode desc stored size %x uncompressed size %x",
			desc.stored_size, desc.uncompressed_size);
	nvgpu_log_info(g, "falcon ucode desc virtualEntry %x, interfaceOffset %x",
			desc.virtual_entry, desc.interface_offset);
	nvgpu_log_info(g, "falcon ucode IMEM phys base %x, load size %x virt base %x sec base %x sec size %x",
			desc.imem_phys_base, desc.imem_load_size,
			desc.imem_virt_base, desc.imem_sec_base,
			desc.imem_sec_size);
	nvgpu_log_info(g, "falcon ucode DMEM offset %x phys base %x, load size %x",
			desc.dmem_offset, desc.dmem_phys_base,
			desc.dmem_load_size);

	if (desc.stored_size != desc.uncompressed_size) {
		nvgpu_log_info(g, "does not match");
		return -EINVAL;
	}

	ucode->code_entry_point = desc.virtual_entry;
	ucode->bootloader = &g->bios.data[offset] + desc_size;
	ucode->bootloader_phys_base = desc.imem_phys_base;
	ucode->bootloader_size = desc.imem_load_size - desc.imem_sec_size;
	ucode->ucode = ucode->bootloader + ucode->bootloader_size;
	ucode->phys_base = ucode->bootloader_phys_base + ucode->bootloader_size;
	ucode->size = desc.imem_sec_size;
	ucode->dmem = ucode->bootloader + desc.dmem_offset;
	ucode->dmem_phys_base = desc.dmem_phys_base;
	ucode->dmem_size = desc.dmem_load_size;

	ret = nvgpu_bios_parse_appinfo_table(g,
			offset + U32(desc_size) +
			desc.dmem_offset + desc.interface_offset);

	return ret;
}

static int nvgpu_bios_parse_falcon_ucode_table(struct gk20a *g, u32 offset)
{
	struct falcon_ucode_table_hdr_v1 hdr;
	u32 i;

	nvgpu_memcpy((u8 *)&hdr, &g->bios.data[offset], sizeof(hdr));
	nvgpu_log_fn(g, "falcon ucode table ver %d size %d entrySize %d entryCount %d descVer %d descSize %d",
			hdr.version, hdr.header_size,
			hdr.entry_size, hdr.entry_count,
			hdr.desc_version, hdr.desc_size);

	if (hdr.version != 1U) {
		return -EINVAL;
	}

	offset += hdr.header_size;

	for (i = 0U; i < hdr.entry_count; i++) {
		struct falcon_ucode_table_entry_v1 entry;

		nvgpu_memcpy((u8 *)&entry, &g->bios.data[offset],
			sizeof(entry));

		nvgpu_log_fn(g, "falcon ucode table entry appid %x targetId %x descPtr %x",
				entry.application_id, entry.target_id,
				entry.desc_ptr);

		if (entry.target_id == TARGET_ID_PMU &&
		    entry.application_id == APPLICATION_ID_DEVINIT) {
			int err;

			err = nvgpu_bios_parse_falcon_ucode_desc(g,
					&g->bios.devinit, entry.desc_ptr);
			if (err != 0) {
				err = nvgpu_bios_parse_falcon_ucode_desc(g,
					&g->bios.devinit,
					entry.desc_ptr +
					 g->bios.expansion_rom_offset);
			}

			if (err != 0) {
				nvgpu_err(g,
					  "could not parse devinit ucode desc");
			}
		} else if (entry.target_id == TARGET_ID_PMU &&
		    entry.application_id == APPLICATION_ID_PRE_OS) {
			int err;

			err = nvgpu_bios_parse_falcon_ucode_desc(g,
					&g->bios.preos, entry.desc_ptr);
			if (err != 0) {
				err = nvgpu_bios_parse_falcon_ucode_desc(g,
					&g->bios.preos,
					entry.desc_ptr +
					 g->bios.expansion_rom_offset);
			}

			if (err != 0) {
				nvgpu_err(g,
					  "could not parse preos ucode desc");
			}
		} else {
			nvgpu_log_info(g, "App_id: %u and target_id: %u"
					" combination not supported.",
					entry.application_id,
					entry.target_id);
		}

		offset += hdr.entry_size;
	}

	return 0;
}

static void nvgpu_bios_parse_falcon_data_v2(struct gk20a *g, u32 offset)
{
	struct falcon_data_v2 falcon_data;
	int err;

	nvgpu_memcpy((u8 *)&falcon_data, &g->bios.data[offset],
		sizeof(falcon_data));
	nvgpu_log_fn(g, "falcon ucode table ptr %x",
			falcon_data.falcon_ucode_table_ptr);
	err = nvgpu_bios_parse_falcon_ucode_table(g,
			falcon_data.falcon_ucode_table_ptr);
	if (err != 0) {
		err = nvgpu_bios_parse_falcon_ucode_table(g,
				falcon_data.falcon_ucode_table_ptr +
			g->bios.expansion_rom_offset);
	}

	if (err != 0) {
		nvgpu_err(g, "could not parse falcon ucode table");
	}
}

void *nvgpu_bios_get_perf_table_ptrs(struct gk20a *g,
		struct bit_token *ptoken, u8 table_id)
{
	u32 perf_table_id_offset = 0;
	u8 *perf_table_ptr = NULL;
	u8 data_size = 4;

	if (ptoken != NULL) {

		if (ptoken->token_id == TOKEN_ID_VIRT_PTRS) {
			perf_table_id_offset = *((u16 *)&g->bios.data[
				ptoken->data_ptr +
				(U16(table_id) * U16(PERF_PTRS_WIDTH_16))]);
			data_size = PERF_PTRS_WIDTH_16;
		} else {
			perf_table_id_offset = *((u32 *)&g->bios.data[
				ptoken->data_ptr +
				(U16(table_id) * U16(PERF_PTRS_WIDTH))]);
			data_size = PERF_PTRS_WIDTH;
		}
	} else {
		return (void *)perf_table_ptr;
	}

	if (table_id < (ptoken->data_size/data_size)) {

		nvgpu_log_info(g, "Perf_Tbl_ID-offset 0x%x Tbl_ID_Ptr-offset- 0x%x",
					(ptoken->data_ptr +
					(U16(table_id) * U16(data_size))),
					perf_table_id_offset);

		if (perf_table_id_offset != 0U) {
			/* check if perf_table_id_offset is beyond base rom */
			if (perf_table_id_offset > g->bios.base_rom_size) {
				perf_table_ptr =
					&g->bios.data[g->bios.expansion_rom_offset +
						perf_table_id_offset];
			} else {
				perf_table_ptr =
					&g->bios.data[perf_table_id_offset];
			}
		} else {
			nvgpu_warn(g, "PERF TABLE ID %d is NULL",
					table_id);
		}
	} else {
		nvgpu_warn(g, "INVALID PERF TABLE ID - %d ", table_id);
	}

	return (void *)perf_table_ptr;
}

static void nvgpu_bios_parse_bit(struct gk20a *g, u32 offset)
{
	struct bios_bit bit;
	struct bit_token bit_token;
	u32 i;

	nvgpu_log_fn(g, " ");
	nvgpu_memcpy((u8 *)&bit, &g->bios.data[offset], sizeof(bit));

	nvgpu_log_info(g, "BIT header: %04x %08x", bit.id, bit.signature);
	nvgpu_log_info(g, "tokens: %d entries * %d bytes",
			bit.token_entries, bit.token_size);

	offset += bit.header_size;
	for (i = 0U; i < bit.token_entries; i++) {
		nvgpu_memcpy((u8 *)&bit_token, &g->bios.data[offset],
			sizeof(bit_token));

		nvgpu_log_info(g, "BIT token id %d ptr %d size %d ver %d",
				bit_token.token_id, bit_token.data_ptr,
				bit_token.data_size, bit_token.data_version);

		switch (bit_token.token_id) {
		case TOKEN_ID_BIOSDATA:
			nvgpu_bios_parse_biosdata(g, bit_token.data_ptr);
			break;
		case TOKEN_ID_NVINIT_PTRS:
			nvgpu_bios_parse_nvinit_ptrs(g, bit_token.data_ptr);
			break;
		case TOKEN_ID_FALCON_DATA:
			if (bit_token.data_version == 2U) {
				nvgpu_bios_parse_falcon_data_v2(g,
						bit_token.data_ptr);
			}
			break;
		case TOKEN_ID_PERF_PTRS:
			g->bios.perf_token =
				(struct bit_token *)&g->bios.data[offset];
			break;
		case TOKEN_ID_CLOCK_PTRS:
			g->bios.clock_token =
				(struct bit_token *)&g->bios.data[offset];
			break;
		case TOKEN_ID_VIRT_PTRS:
			g->bios.virt_token =
				(struct bit_token *)&g->bios.data[offset];
			break;
		case TOKEN_ID_MEMORY_PTRS:
			nvgpu_bios_parse_memory_ptrs(g, bit_token.data_ptr,
				bit_token.data_version);
			break;
		default:
			nvgpu_log_info(g, "Token id %d not supported",
							bit_token.token_id);
			break;
		}

		offset += bit.token_size;
	}
	nvgpu_log_fn(g, "done");
}

static u32 nvgpu_bios_readbyte_impl(struct gk20a *g, u32 offset)
{
	return g->bios.data[offset];
}

u8 nvgpu_bios_read_u8(struct gk20a *g, u32 offset)
{
	return (u8)nvgpu_bios_readbyte_impl(g, offset);
}

s8 nvgpu_bios_read_s8(struct gk20a *g, u32 offset)
{
	u32 val;
	val = nvgpu_bios_readbyte_impl(g, offset);
	val = ((val & 0x80U) != 0U) ? (val | ~0xffU) : val;

	return (s8) val;
}

u16 nvgpu_bios_read_u16(struct gk20a *g, u32 offset)
{
	u16 val;

	val = U16(nvgpu_bios_readbyte_impl(g, offset) |
		(nvgpu_bios_readbyte_impl(g, offset+1U) << 8U));

	return val;
}

u32 nvgpu_bios_read_u32(struct gk20a *g, u32 offset)
{
	u32 val;

	val = U32(nvgpu_bios_readbyte_impl(g, offset) |
		(nvgpu_bios_readbyte_impl(g, offset+1U) << 8U) |
		(nvgpu_bios_readbyte_impl(g, offset+2U) << 16U) |
		(nvgpu_bios_readbyte_impl(g, offset+3U) << 24U));

	return val;
}
