/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef SUPER_SURFACE_PRIV_H
#define SUPER_SURFACE_PRIV_H

#include <nvgpu/pmu/pmuif/nvgpu_cmdif.h>
#include <nvgpu/pmu/pmuif/boardobj.h>
#include <nvgpu/pmu/pmuif/volt.h>
#include <nvgpu/pmu/pmuif/clk.h>
#include <nvgpu/pmu/pmuif/cmn.h>
#include <nvgpu/flcnif_cmn.h>

struct nvgpu_mem;

/* PMU super surface */
/* 1MB Bytes for SUPER_SURFACE_SIZE */
#define SUPER_SURFACE_SIZE     (1024U * 1024U)
/* 64K Bytes for command queues */
#define FBQ_CMD_QUEUES_SIZE    (64U * 1024U)
/* 1K Bytes for message queue */
#define FBQ_MSG_QUEUE_SIZE     (1024U)
/* 512 Bytes for SUPER_SURFACE_MEMBER_DESCRIPTOR */
#define SSMD_SIZE              (512U)
/* 16 bytes for SUPER_SURFACE_HDR */
#define SS_HDR_SIZE            (16U)
#define SS_UNMAPPED_MEMBERS_SIZE (SUPER_SURFACE_SIZE - \
	(FBQ_CMD_QUEUES_SIZE + FBQ_MSG_QUEUE_SIZE + SSMD_SIZE + SS_HDR_SIZE))

/* SSMD */
#define NV_PMU_SUPER_SURFACE_MEMBER_DESCRIPTOR_COUNT    32U

/*
 * Defines the structure of the @ nv_pmu_super_surface_member_descriptor::id
 */
#define NV_RM_PMU_SUPER_SURFACE_MEMBER_ID_GROUP              0x0000U
#define NV_RM_PMU_SUPER_SURFACE_MEMBER_ID_GROUP_INVALID      0xFFFFU
#define NV_RM_PMU_SUPER_SURFACE_MEMBER_ID_TYPE_SET           BIT(16)
#define NV_RM_PMU_SUPER_SURFACE_MEMBER_ID_TYPE_GET_STATUS    BIT(17)
#define NV_RM_PMU_SUPER_SURFACE_MEMBER_ID_RSVD               (0x00UL << 20U)

struct nv_pmu_super_surface_member_descriptor {
	/* The member ID (@see NV_PMU_SUPER_SURFACE_MEMBER_ID_<xyz>). */
	u32   id;

	/* The sub-structure's byte offset within the super-surface. */
	u32   offset;

	/* The sub-structure's byte size (must always be properly aligned). */
	u32   size;

	/* Reserved (and preserving required size/alignment). */
	u32   rsvd;
};

/* PMU super surface */
struct nv_pmu_super_surface_hdr {
	struct falc_u64 address;
	u32 member_mask;
	u16 dmem_buffer_size_max;
};

NV_PMU_MAKE_ALIGNED_STRUCT(nv_pmu_super_surface_hdr,
		sizeof(struct nv_pmu_super_surface_hdr));

/*
 * Global Super Surface structure for combined INIT data required by PMU.
 * NOTE: Any new substructures or entries must be aligned.
 */
struct nv_pmu_super_surface {
	struct nv_pmu_super_surface_member_descriptor
		ssmd[NV_PMU_SUPER_SURFACE_MEMBER_DESCRIPTOR_COUNT];

	struct {
		struct nv_pmu_fbq_cmd_queues cmd_queues;
		struct nv_pmu_fbq_msg_queue msg_queue;
	} fbq;

	union nv_pmu_super_surface_hdr_aligned hdr;

	union {
		u8 ss_unmapped_members_rsvd[SS_UNMAPPED_MEMBERS_SIZE];

		/*
		 * Below members are only for reference to know
		 * supported boardobjs from nvgpu, should not be
		 * accessed any boardobj member from below list
		 * in nvgpu using these members, instead use ssmd
		 * member present above to know the offset of
		 * required boardobj from super surface in nvgpu
		 */
		struct {
			struct nv_pmu_volt_volt_device_boardobj_grp_set
				volt_device_grp_set;
			struct nv_pmu_volt_volt_policy_boardobj_grp_set
				volt_policy_grp_set;
			struct nv_pmu_volt_volt_rail_boardobj_grp_set
				volt_rail_grp_set;

			struct nv_pmu_volt_volt_rail_boardobj_grp_get_status
				volt_rail_grp_get_status;
		} volt;
		struct  {
			struct nv_pmu_clk_clk_vin_device_boardobj_grp_set
				clk_vin_device_grp_set;
			struct nv_pmu_clk_clk_domain_boardobj_grp_set
				clk_domain_grp_set;
			struct nv_pmu_clk_clk_fll_device_boardobj_grp_set
				clk_fll_device_grp_set;
			struct nv_pmu_clk_clk_prog_boardobj_grp_set
				clk_prog_grp_set;
			struct nv_pmu_clk_clk_vf_point_boardobj_grp_set
				clk_vf_point_grp_set;
			struct nv_pmu_clk_clk_vin_device_boardobj_grp_get_status
				clk_vin_device_grp_get_status;
			struct nv_pmu_clk_clk_fll_device_boardobj_grp_get_status
				clk_fll_device_grp_get_status;
			struct nv_pmu_clk_clk_vf_point_boardobj_grp_get_status
				clk_vf_point_grp_get_status;
		} clk;
		struct {
			struct nv_pmu_therm_therm_channel_boardobj_grp_set
				therm_channel_grp_set;
			struct nv_pmu_therm_therm_device_boardobj_grp_set
				therm_device_grp_set;
			struct nv_pmu_therm_therm_channel_boardobj_grp_get_status
				therm_channel_grp_get_status;
		} therm;
		struct {
			struct nv_pmu_clk_clk_vf_point_boardobj_grp_set
				clk_vf_point_grp_set;
			struct nv_pmu_clk_clk_vf_point_boardobj_grp_get_status
				clk_vf_point_grp_get_status;
		} clk_35;
	};
};

/* nvgpu super surface */
struct pmu_super_surface {
	/* super surface members */
	struct nvgpu_mem super_surface_buf;

	struct nv_pmu_super_surface_member_descriptor
		ssmd_set[NV_PMU_SUPER_SURFACE_MEMBER_DESCRIPTOR_COUNT];

	struct nv_pmu_super_surface_member_descriptor
		ssmd_get_status[NV_PMU_SUPER_SURFACE_MEMBER_DESCRIPTOR_COUNT];
};

#endif /* SUPER_SURFACE_PRIV_H */
