/*
 * Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.
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

#include <linux/seq_file.h>
#include "pva_dma.h"
#include "pva_queue.h"
#include "pva-sys-dma.h"
#include "pva.h"
#include "pva_vpu_exe.h"
#include "nvpva_client.h"
#include "dev.h"
#include "pva-bit.h"

static int32_t patch_dma_desc_address(struct pva_submit_task *task,
				      struct nvpva_dma_descriptor *umd_dma_desc,
				      struct pva_dtd_s *dma_desc, int *is_cfg)
{
	int32_t err = 0;
	uint64_t addr_base = 0;

	switch (umd_dma_desc->srcTransferMode) {
	case DMA_DESC_SRC_XFER_L2RAM:
		/*
		 * PVA_HW_GEN1 has CVNAS RAM PVA_HW_GEN2 has L2SRAM CVNAS RAM
		 * memory is pinned and needs conversion from pin ID -> IOVA
		 * L2SRAM has memory offset which does not need conversion. The
		 * same conversion is applied for dst
		 */
		if (task->pva->version == PVA_HW_GEN1) {
			struct pva_pinned_memory *mem =
				pva_task_pin_mem(task, umd_dma_desc->srcPtr);
			if (IS_ERR(mem)) {
				err = PTR_ERR(mem);
				task_err(
					task,
					"invalid memory handle in descriptor for src L2RAM");
				goto out;
			}
			addr_base = mem->dma_addr;
		} else
			addr_base = 0;
		break;
	case DMA_DESC_SRC_XFER_VMEM:
	case DMA_DESC_SRC_XFER_MMIO: {
		/* calculate symbol address */
		u32 addr;

		err = pva_get_sym_offset(&task->client->elf_ctx, task->exe_id,
					 umd_dma_desc->srcPtr, &addr);
		if (err) {
			err = -EINVAL;
			task_err(
				task,
				"invalid symbol id in descriptor for src VMEM");
			goto out;
		}
		addr_base = addr;
	} break;
	case DMA_DESC_SRC_XFER_VPU_CONFIG: {
		u32 addr;
		/* dest must be null*/
		if (umd_dma_desc->dstPtr != 0U) {
			task_err(task, "vpu config's dstPtr must be 0");
			err = -EINVAL;
			goto out;
		}

		/* calculate symbol address */
		/* TODO: check VPUC handling in ELF segment */
		err = pva_get_sym_offset(&task->client->elf_ctx, task->exe_id,
					 umd_dma_desc->srcPtr, &addr);

		if (err) {
			task_err(task, "ERROR: Invalid offset or address");
			goto out;
		}

		addr_base = addr;
		*is_cfg = 1;
		break;
	}
	case DMA_DESC_SRC_XFER_MC: {
		struct pva_pinned_memory *mem =
			pva_task_pin_mem(task, umd_dma_desc->srcPtr);
		if (IS_ERR(mem)) {
			err = PTR_ERR(mem);
			task_err(
				task,
				"invalid memory handle in descriptor for src MC");
			goto out;
		}
		addr_base = mem->dma_addr;
		task->src_surf_base_addr = addr_base;

		/** If BL format selected, set addr bit 39 to indicate */
		/* XBAR_RAW swizzling is required */
		addr_base |= (u64)umd_dma_desc->srcFormat << 39U;

		break;
	}
	case DMA_DESC_SRC_XFER_R5TCM:
		if (!task->is_system_app) { /* check if system app */
			err = -EFAULT;
			goto out;
		} else {
			task->special_access = 1;
			addr_base = 0;
			break;
		}
	case DMA_DESC_SRC_XFER_INVAL:
	case DMA_DESC_SRC_XFER_RSVD:
	default:
		err = -EFAULT;
		goto out;
	}
	addr_base += umd_dma_desc->src_offset;
	dma_desc->src_adr0 = (uint32_t)(addr_base & 0xFFFFFFFFLL);
	dma_desc->src_adr1 = (uint8_t)((addr_base >> 32U) & 0xFF);

	addr_base = 0;
	switch (umd_dma_desc->dstTransferMode) {
	case DMA_DESC_DST_XFER_L2RAM:
		if (task->pva->version == PVA_HW_GEN1) {
			struct pva_pinned_memory *mem =
				pva_task_pin_mem(task, umd_dma_desc->dstPtr);
			if (IS_ERR(mem)) {
				err = PTR_ERR(mem);
				task_err(
					task,
					"invalid memory handle in descriptor for dst L2RAM");
				goto out;
			}
			addr_base = mem->dma_addr;
		} else
			addr_base = 0;
		break;
	case DMA_DESC_DST_XFER_VMEM: {
		/* calculate symbol address */
		u32 addr;

		err = pva_get_sym_offset(&task->client->elf_ctx, task->exe_id,
					 umd_dma_desc->dstPtr, &addr);
		if (err) {
			err = -EINVAL;
			task_err(
				task,
				"invalid symbol id in descriptor for dst VMEM");
			goto out;
		}
		addr_base = addr;
	} break;
	case DMA_DESC_DST_XFER_MMIO:
		/*
		 * Currently passing Ptr as it is. To be updated later
		 * as per update from UMD
		 */
		addr_base = umd_dma_desc->dstPtr;
		break;
	case DMA_DESC_DST_XFER_MC: {
		struct pva_pinned_memory *mem =
			pva_task_pin_mem(task, umd_dma_desc->dstPtr);
		if (IS_ERR(mem)) {
			err = PTR_ERR(mem);
			task_err(
				task,
				"invalid memory handle in descriptor for dst MC");
			goto out;
		}
		addr_base = mem->dma_addr;
		task->dst_surf_base_addr = addr_base;

		/* If BL format selected, set addr bit 39 to indicate */
		/* XBAR_RAW swizzling is required */
		addr_base |= (u64)umd_dma_desc->dstFormat << 39U;
		break;
	}
	case DMA_DESC_DST_XFER_R5TCM:
		if (!task->is_system_app) { /* check if system app */
			err = -EFAULT;
			goto out;
		} else {
			task->special_access = 1;
			addr_base = 0;
			break;
		}
	case DMA_DESC_DST_XFER_INVAL:
	case DMA_DESC_DST_XFER_RSVD1:
	case DMA_DESC_DST_XFER_RSVD2:
	default:
		err = -EFAULT;
		goto out;
	}

	addr_base += umd_dma_desc->dst_offset;
	dma_desc->dst_adr0 = (uint32_t)(addr_base & 0xFFFFFFFFLL);
	dma_desc->dst_adr1 = (uint8_t)((addr_base >> 32U) & 0xFF);
out:
	return err;
}

/* User to FW DMA descriptor structure mapping helper */
/* TODO: Need to handle DMA descriptor like dst2ptr and dst2Offset */
static int32_t nvpva_task_dma_desc_mapping(struct pva_submit_task *task,
					   struct pva_hw_task *hw_task)
{
	struct nvpva_dma_descriptor *umd_dma_desc = NULL;
	struct pva_dtd_s *dma_desc = NULL;
	int32_t err = 0;
	unsigned int numDesc;
	uint32_t addr = 0U;
	int valid_link_did = 0;
	int is_cfg = 0;

	task->special_access = 0;

	for (numDesc = 0U; numDesc < task->num_dma_descriptors; numDesc++) {
		umd_dma_desc = &task->dma_descriptors[numDesc];
		dma_desc = &hw_task->dma_desc[numDesc];
		is_cfg = 0;

		err = patch_dma_desc_address(task, umd_dma_desc, dma_desc,
					     &is_cfg);
		if (err) {
			/* @TODO: Check if this condition related to usage of
			 * vpu_config transfer control mode can be detected
			 * before hitting this error case.
			 */
			if (valid_link_did) {
				/* @TODO: Check whether separate handling is
				 * required to support multiple linked
				 * descriptors provided by user in any order.
				 */
				err = 0;
				valid_link_did = 0;
				task_err(
					task,
					"invalid memory handle in descriptor but vpuc");
			} else
				goto out;
		}
		hw_task->dma_info.special_access = task->special_access;
		/* DMA_DESC_TRANS CNTL0 */
		dma_desc->transfer_control0 =
			umd_dma_desc->srcTransferMode |
			(umd_dma_desc->srcFormat << 3U) |
			(umd_dma_desc->dstTransferMode << 4U) |
			(umd_dma_desc->dstFormat << 7U);
		/* DMA_DESC_TRANS CNTL1 */
		dma_desc->transfer_control1 =
			umd_dma_desc->bytePerPixel |
			(umd_dma_desc->pxDirection << 2U) |
			(umd_dma_desc->pyDirection << 3U) |
			(umd_dma_desc->boundaryPixelExtension << 4U) |
			(umd_dma_desc->transTrueCompletion << 7U);
		/* DMA_DESC_TRANS CNTL2 */
		if (umd_dma_desc->prefetchEnable &&
		    (umd_dma_desc->tx == 0 || umd_dma_desc->ty == 0 ||
		     umd_dma_desc->srcTransferMode != DMA_DESC_SRC_XFER_MC ||
		     umd_dma_desc->dstTransferMode != DMA_DESC_DST_XFER_VMEM)) {
			/* also ECET must be non zero */
			task_err(task, " Invalid criteria to enable Prefetch");
			return -EINVAL;
		}
		dma_desc->transfer_control2 =
			umd_dma_desc->prefetchEnable |
			(umd_dma_desc->dstCbEnable << 1U) |
			(umd_dma_desc->srcCbEnable << 2U);

		/*!
		 * Block-linear surface offset. Only the surface in dram
		 * can be block-linear.
		 * BLBaseAddress = translate(srcPtr / dstPtr) + surfBLOffset;
		 * transfer_control2.bit[3:7] = BLBaseAddress[1].bit[1:5]
		 * GOB offset in BL mode and corresponds to surface address
		 * bits [13:9]
		 */
		if ((umd_dma_desc->srcFormat == 1U)
		   && (umd_dma_desc->srcTransferMode ==
					DMA_DESC_SRC_XFER_MC)) {
			task->src_surf_base_addr += umd_dma_desc->surfBLOffset;
			dma_desc->transfer_control2 |=
			    (u8)((task->src_surf_base_addr & 0x3E00) >> 6U);
		} else if ((umd_dma_desc->dstFormat == 1U) &&
				(umd_dma_desc->dstTransferMode ==
					DMA_DESC_DST_XFER_MC)) {
			task->dst_surf_base_addr += umd_dma_desc->surfBLOffset;
			dma_desc->transfer_control2 |=
			    (u8)((task->dst_surf_base_addr & 0x3E00) >> 6U);
		}

		/* DMA_DESC_LDID */
		if (umd_dma_desc->linkDescId < 0x40)
			dma_desc->link_did = umd_dma_desc->linkDescId;

		if ((dma_desc->link_did > 0) && (is_cfg))
			valid_link_did = 1;

		/* DMA_DESC_TX */
		dma_desc->tx = umd_dma_desc->tx;
		/* DMA_DESC_TY */
		dma_desc->ty = umd_dma_desc->ty;
		/* DMA_DESC_DLP_ADV */
		dma_desc->dlp_adv = umd_dma_desc->dstLinePitch;
		/* DMA_DESC_SLP_ADV */
		dma_desc->slp_adv = umd_dma_desc->srcLinePitch;
		/* DMA_DESC_DB_START */
		dma_desc->db_start = umd_dma_desc->dstCbStart;
		/* DMA_DESC_DB_SIZE */
		dma_desc->db_size = umd_dma_desc->dstCbSize;
		/* DMA_DESC_SB_START */
		dma_desc->sb_start = umd_dma_desc->srcCbStart;
		/* DMA_DESC_SB_SIZE */
		dma_desc->sb_size = umd_dma_desc->srcCbSize;
		/* DMA_DESC_TRIG_CH */
		/* TODO: Need to handle this parameter */
		dma_desc->trig_ch_events = 0U;
		/* DMA_DESC_HW_SW_TRIG */
		dma_desc->hw_sw_trig_events =
			umd_dma_desc->trigEventMode |
			(umd_dma_desc->trigVpuEvents << 2U) |
			(umd_dma_desc->descReloadEnable << (8U + 4U));
		/* DMA_DESC_PX */
		dma_desc->px = (uint8_t)umd_dma_desc->px;
		/* DMA_DESC_PY */
		dma_desc->py = (uint8_t)umd_dma_desc->py;
		/* DMA_DESC_FRDA */
		if (umd_dma_desc->dst2Ptr) {
			err = pva_get_sym_offset(&task->client->elf_ctx,
						 task->exe_id,
						 umd_dma_desc->dst2Ptr, &addr);
			if (err) {
				task_err(task,
					 "invalid symbol id in descriptor");
				goto out;
			}
			addr = addr + umd_dma_desc->dst2Offset;
			dma_desc->frda |= (addr & 0x3FFC0) >> 6U;
		}
		/* DMA_DESC_NDTM_CNTL0 */
		/* TODO: Need to handle this parameter */
		dma_desc->cb_ext = 0U;
		/* DMA_DESC_NS1_ADV & DMA_DESC_ST1_ADV */
		dma_desc->srcpt1_cntl =
			(((umd_dma_desc->srcRpt1 & 0xFF) << 24U) |
			 (umd_dma_desc->srcAdv1 & 0xFFFFFF));
		/* DMA_DESC_ND1_ADV & DMA_DESC_DT1_ADV */
		dma_desc->dstpt1_cntl =
			(((umd_dma_desc->dstRpt1 & 0xFF) << 24U) |
			 (umd_dma_desc->dstAdv1 & 0xFFFFFF));
		/* DMA_DESC_NS2_ADV & DMA_DESC_ST2_ADV */
		dma_desc->srcpt2_cntl =
			(((umd_dma_desc->srcRpt2 & 0xFF) << 24U) |
			 (umd_dma_desc->srcAdv2 & 0xFFFFFF));
		/* DMA_DESC_ND2_ADV & DMA_DESC_DT2_ADV */
		dma_desc->dstpt2_cntl =
			(((umd_dma_desc->dstRpt2 & 0xFF) << 24U) |
			 (umd_dma_desc->dstAdv2 & 0xFFFFFF));
		/* DMA_DESC_NS3_ADV & DMA_DESC_ST3_ADV */
		dma_desc->srcpt3_cntl =
			(((umd_dma_desc->srcRpt3 & 0xFF) << 24U) |
			 (umd_dma_desc->srcAdv3 & 0xFFFFFF));
		/* DMA_DESC_ND3_ADV & DMA_DESC_DT3_ADV */
		dma_desc->dstpt3_cntl =
			(((umd_dma_desc->dstRpt3 & 0xFF) << 24U) |
			 (umd_dma_desc->dstAdv3 & 0xFFFFFF));
	}

out:
	return err;
}

/* User to FW mapping for DMA channel */
static int nvpva_task_dma_channel_mapping(struct nvpva_dma_channel *user_ch,
					  struct pva_dma_ch_config_s *ch,
					  int32_t hwgen)
{
	/* DMA_CHANNEL_CNTL0_CHSDID: DMA_CHANNEL_CNTL0[0] = descIndex + 1;*/
	ch->cntl0 = (((user_ch->descIndex + 1U) & 0xFFU) << 0U);
	/* DMA_CHANNEL_CNTL0_CHVMEMOREQ */
	ch->cntl0 |= ((user_ch->vdbSize & 0xFFU) << 8U);
	/* DMA_CHANNEL_CNTL0_CHBH */
	ch->cntl0 |= ((user_ch->adbSize & 0x1FFU) << 16U);
	/* DMA_CHANNEL_CNTL0_CHAXIOREQ */
	ch->cntl0 |= ((user_ch->blockHeight & 7U) << 25U);
	/* DMA_CHANNEL_CNTL0_CHPREF */
	ch->cntl0 |= ((user_ch->prefetchEnable & 1U) << 30U);
	/* Enable DMA channel */
	ch->cntl0 |= (0x1U << 31U);

	/* DMA_CHANNEL_CNTL1_CHPWT */
	ch->cntl1 = ((user_ch->reqPerGrant & 0x7U) << 2U);
	/* DMA_CHANNEL_CNTL1_CHREP */
	ch->cntl1 |= ((user_ch->chRepFactor & 0x7U) << 8U);
	/* DMA_CHANNEL_CNTL1_CHVDBSTART */
	ch->cntl1 |= ((user_ch->vdbOffset & 0x7FU) << 16U);
	/* DMA_CHANNEL_CNTL1_CHADBSTART */
	if (hwgen == PVA_HW_GEN1)
		ch->cntl1 |= ((user_ch->adbOffset & 0xFFU) << 24U);
	else if (hwgen == PVA_HW_GEN2)
		ch->cntl1 |= ((user_ch->adbOffset & 0x1FFU) << 23U);

	ch->boundary_pad = user_ch->padValue;

	/* Applicable only for T23x */
	if (hwgen == PVA_HW_GEN2) {
		/* DMA_CHANNEL_HWSEQCNTL_CHHWSEQSTART */
		ch->hwseqcntl = ((user_ch->hwseqStart & 0xFFU) << 0U);
		/* DMA_CHANNEL_HWSEQCNTL_CHHWSEQEND */
		ch->hwseqcntl |= ((user_ch->hwseqEnd & 0xFFU) << 12U);
		/* DMA_CHANNEL_HWSEQCNTL_CHHWSEQTD */
		ch->hwseqcntl |= ((user_ch->hwseqTriggerDone & 0x3U) << 24U);
		/* DMA_CHANNEL_HWSEQCNTL_CHHWSEQTS */
		ch->hwseqcntl |= ((user_ch->hwseqTxSelect & 0x1U) << 27U);
		/* DMA_CHANNEL_HWSEQCNTL_CHHWSEQTO */
		ch->hwseqcntl |= ((user_ch->hwseqTraversalOrder & 0x1U) << 30U);
		/* DMA_CHANNEL_HWSEQCNTL_CHHWSEQEN */
		ch->hwseqcntl |= ((user_ch->hwseqEnable & 0x1U) << 31U);
	}
	return 0;
}

int pva_task_write_dma_info(struct pva_submit_task *task,
			    struct pva_hw_task *hw_task)
{
	int err = 0;
	u8 ch_num = 0L;
	int hwgen = task->pva->version;
	bool is_hwseq_mode = false;
	u32 i;

	if (task->num_dma_descriptors == 0L || task->num_dma_channels == 0L) {
		nvhost_dbg_info("pva: DMA Decr and channel not present, NOOP mode");
		return err;
	}

	err = nvpva_task_dma_desc_mapping(task, hw_task);
	if (err) {
		task_err(task, "failed to map DMA desc info");
		return err;
	}

	if (task->hwseq_config.hwseqBuf.pin_id != 0U) {
		if (hwgen != PVA_HW_GEN2) {
			/* HW sequencer is supported only in HW_GEN2 */
			return -EINVAL;
		}

		/* Ensure that HWSeq blob size is valid and within the
		 * acceptable range, i.e. up to 1KB, as per HW Sequencer RAM
		 * size from T23x DMA IAS doc.
		 */
		if ((task->hwseq_config.hwseqBuf.size == 0U) ||
		    (task->hwseq_config.hwseqBuf.size > 1024U)) {
			return -EINVAL;
		}

		is_hwseq_mode = true;
	}

	/* write dma channel info */
	hw_task->dma_info.num_channels = task->num_dma_channels;
	hw_task->dma_info.num_descriptors = task->num_dma_descriptors;
	hw_task->dma_info.r5_channel_mask = task->system_channel_mask;
	hw_task->dma_info.r5_descriptor_mask[0] = PVA_LOW32(task->system_descriptor_mask);
	hw_task->dma_info.r5_descriptor_mask[1] = PVA_HI32(task->system_descriptor_mask);
	hw_task->dma_info.descriptor_id = 1U; /* PVA_DMA_DESC0 */

	for (i = 0; i < task->num_dma_channels; i++) {
		ch_num = i + 1; /* Channel 0 can't use */
		err = nvpva_task_dma_channel_mapping(
			&task->dma_channels[i],
			&hw_task->dma_info.dma_channels[i], hwgen);
		if (err) {
			task_err(task, "failed to map DMA channel info");
			return err;
		}
		/* Ensure that HWSEQCNTRL is zero for all dma channels in SW
		 * mode
		 */
		if (!is_hwseq_mode &&
		    (hw_task->dma_info.dma_channels[i].hwseqcntl != 0U)) {
			task_err(task, "invalid HWSeq config in SW mode");
			return -EINVAL;
		}
		hw_task->dma_info.dma_channels[i].ch_number = ch_num;
		switch (task->dma_channels[i].outputEnableMask) {
		case PVA_DMA_READ0:
			hw_task->dma_info.dma_triggers[0] |= 0x1 << ch_num;
			break;
		case PVA_DMA_STORE0:
			hw_task->dma_info.dma_triggers[0] |= 0x2
							     << (ch_num + 15);
			break;
		case PVA_DMA_READ1:
			hw_task->dma_info.dma_triggers[1] |= 0x1 << ch_num;
			break;
		case PVA_DMA_STORE1:
			hw_task->dma_info.dma_triggers[1] |= 0x2
							     << (ch_num + 15);
			break;
		case PVA_DMA_READ2:
			hw_task->dma_info.dma_triggers[2] |= 0x1 << ch_num;
			break;
		case PVA_DMA_STORE2:
			hw_task->dma_info.dma_triggers[2] |= 0x2
							     << (ch_num + 15);
			break;
		case PVA_DMA_READ3:
			hw_task->dma_info.dma_triggers[3] |= 0x1 << ch_num;
			break;
		case PVA_DMA_STORE3:
			hw_task->dma_info.dma_triggers[3] |= 0x2
							     << (ch_num + 15);
			break;
		case PVA_DMA_READ4:
			hw_task->dma_info.dma_triggers[4] |= 0x1 << ch_num;
			break;
		case PVA_DMA_STORE4:
			hw_task->dma_info.dma_triggers[4] |= 0x2
							     << (ch_num + 15);
			break;
		case PVA_DMA_READ5:
			hw_task->dma_info.dma_triggers[5] |= 0x1 << ch_num;
			break;
		case PVA_DMA_STORE5:
			hw_task->dma_info.dma_triggers[5] |= 0x2
							     << (ch_num + 15);
			break;
		case PVA_DMA_READ6:
			hw_task->dma_info.dma_triggers[6] |= 0x1 << ch_num;
			break;
		case PVA_DMA_STORE6:
			hw_task->dma_info.dma_triggers[6] |= 0x2
							     << (ch_num + 15);
			break;
		case PVA_VPUCONFIG:
			hw_task->dma_info.dma_triggers[7] |= 0x1 << ch_num;
			break;
		/**
		 * Last dma_triggers register is applicable for HWSeq vpu
		 * read/write start trigger for T23x, ignored for T19x.
		 */
		case PVA_HWSEQ_VPUREAD_START:
			if (hwgen == PVA_HW_GEN2) {
				hw_task->dma_info.dma_triggers[8] |= 0x1
								     << ch_num;
			}
			break;
		case PVA_HWSEQ_VPUWRITE_START:
			if (hwgen == PVA_HW_GEN2) {
				hw_task->dma_info.dma_triggers[8] |=
					0x2 << (ch_num + 15);
			}
			break;
		default:
			if (!task->client->elf_ctx.elf_images->elf_img[task->exe_id].is_system_app)
				task_err(task, "trigger value is not set");
			break;
		}
	}

	hw_task->task.dma_info =
		task->dma_addr + offsetof(struct pva_hw_task, dma_info);
	hw_task->dma_info.dma_descriptor_base =
		task->dma_addr + offsetof(struct pva_hw_task, dma_desc);

	if (is_hwseq_mode) {
		struct pva_pinned_memory *mem;

		/* Configure HWSeq trigger mode selection in DMA Configuration
		 * Register
		 */
		hw_task->dma_info.dma_common_config |=
			(task->hwseq_config.hwseqTrigMode & 0x1U) << 12U;

		mem = pva_task_pin_mem(task,
				       task->hwseq_config.hwseqBuf.pin_id);
		if (IS_ERR(mem)) {
			err = PTR_ERR(mem);
			task_err(task, "failed to pin hwseq buffer");
			return err;
		}
		hw_task->dma_info.dma_hwseq_base =
			mem->dma_addr + task->hwseq_config.hwseqBuf.offset;
		hw_task->dma_info.num_hwseq = task->hwseq_config.hwseqBuf.size;
	}

	hw_task->dma_info.dma_info_version = PVA_DMA_INFO_VERSION_ID;
	hw_task->dma_info.dma_info_size = sizeof(struct pva_dma_info_s);

	return err;
}
