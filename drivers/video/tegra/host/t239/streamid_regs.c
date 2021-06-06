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

struct nvhost_streamid_mapping {
	u32 host1x_offset;
	u32 client_offset;
	u32 client_limit;
};

static struct nvhost_streamid_mapping __attribute__((__unused__))
	t239_host1x_streamid_mapping[] = {

	/* host1x client streamid registers for fw ucode */

	/* HOST1X_THOST_COMMON_SE1_STRMID_0_OFFSET_BASE_0 */
	{ 0x00000e50, 0x00000090, 0x00000090 },
	/* HOST1X_THOST_COMMON_SE2_STRMID_0_OFFSET_BASE_0 */
	{ 0x00000e58, 0x00000090, 0x00000090 },
	/* HOST1X_THOST_COMMON_SE4_STRMID_0_OFFSET_BASE_0 */
	{ 0x00000e60, 0x00000090, 0x00000090 },
	/* HOST1X_THOST_COMMON_VIC_STRMID_0_OFFSET_BASE_0 */
	{ 0x00000e80, 0x00000034, 0x00000034 },
	/* HOST1X_THOST_COMMON_NVENC_STRMID_0_OFFSET_BASE_0 */
	{ 0x00000e88, 0x00000034, 0x00000034 },
	/* HOST1X_THOST_COMMON_NVDEC_STRMID_0_OFFSET_BASE_0 */
	{ 0x00000e90, 0x00000034, 0x00000034 },
	/* HOST1X_THOST_COMMON_OFA_STRMID_0_OFFSET_BASE_0 */
	{ 0x00000ea8, 0x00000034, 0x00000034 },
	/* HOST1X_THOST_COMMON_TSEC_STRMID_0_OFFSET_BASE_0 */
	{ 0x00000e98, 0x00000034, 0x00000034 },

	/* host1x_client ch streamid registers for data buffers */

	/* HOST1X_THOST_COMMON_SE1_CH_STRMID_0_OFFSET_BASE_0 */
	{ 0x00000ee0, 0x00000090, 0x00000090 },
	/* HOST1X_THOST_COMMON_SE2_CH_STRMID_0_OFFSET_BASE_0 */
	{ 0x00000ee8, 0x00000090, 0x00000090 },
	/* HOST1X_THOST_COMMON_SE4_CH_STRMID_0_OFFSET_BASE_0 */
	{ 0x00000ef0, 0x00000090, 0x00000090 },
	/* HOST1X_THOST_COMMON_VIC_CH_STRMID_0_OFFSET_BASE_0 */
	{ 0x00000f18, 0x00000030, 0x00000030 },
	/* HOST1X_THOST_COMMON_NVENC_CH_STRMID_0_OFFSET_BASE_0 */
	{ 0x00000f20, 0x00000030, 0x00000030 },
	/* HOST1X_THOST_COMMON_NVDEC_CH_STRMID_0_OFFSET_BASE_0 */
	{ 0x00000f28, 0x00000030, 0x00000030 },
	/* HOST1X_THOST_COMMON_OFA_CH_STRMID_0_OFFSET_BASE_0 */
	{ 0x00000f40, 0x00000030, 0x00000030 },
	/* HOST1X_THOST_COMMON_TSEC_CH_STRMID_0_OFFSET_BASE_0 */
	{ 0x00000f30, 0x00000030, 0x00000030 },
	{}
};
