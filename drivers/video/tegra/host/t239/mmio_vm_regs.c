/*
 * Copyright (c) 2021, NVIDIA Corporation.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

static const u32 t239_host1x_mmio_vm_r[] = {
	0x0d04u, /* HOST1X_THOST_COMMON_THOST_MMIO_VM */
	0x0d08u, /* HOST1X_THOST_COMMON_SE1_MMIO_VM */
	0x0d0cu, /* HOST1X_THOST_COMMON_SE2_MMIO_VM */
	0x0d10u, /* HOST1X_THOST_COMMON_SE4_MMIO_VM */
	0x0d14u, /* HOST1X_THOST_COMMON_SEU1SE1_MMIO_VM */
	0x0d18u, /* HOST1X_THOST_COMMON_SEU1SE2_MMIO_VM */
	0x0d1cu, /* HOST1X_THOST_COMMON_SEU1SE4_MMIO_VM */
	0x0d20u, /* HOST1X_THOST_COMMON_DPAUX_MMIO_VM */
	0x0d24u, /* HOST1X_THOST_COMMON_VIC_MMIO_VM */
	0x0d28u, /* HOST1X_THOST_COMMON_NVENC_MMIO_VM */
	0x0d2cu, /* HOST1X_THOST_COMMON_NVDEC_MMIO_VM */
	0x0d30u, /* HOST1X_THOST_COMMON_TSEC_MMIO_VM */
	0x0d34u, /* HOST1X_THOST_COMMON_FDE_MMIO_VM */
	0x0d38u, /* HOST1X_THOST_COMMON_OFA_MMIO_VM */
};
