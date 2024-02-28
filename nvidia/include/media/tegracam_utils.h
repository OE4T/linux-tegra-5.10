/**
 * tegracam_utils.h - tegra camera framework core utilities
 *
 * Copyright (c) 2018, NVIDIA Corporation.  All rights reserved.
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

#ifndef __TEGRACAM_UTILS_H__
#define __TEGRACAM_UTILS_H__

#include <media/camera_common.h>

enum sensor_opcode {
	SENSOR_OPCODE_DONE = 0,
	SENSOR_OPCODE_READ = 1,
	SENSOR_OPCODE_WRITE = 2,
	SENSOR_OPCODE_SLEEP = 3,
};

int convert_table_to_blob(struct sensor_blob *pkt,
			const struct reg_8 table[],
			u16 wait_ms_addr, u16 end_addr);
int write_sensor_blob(struct regmap *regmap, struct sensor_blob *blob);
int tegracam_write_blobs(struct tegracam_ctrl_handler *hdl);

bool is_tvcf_supported(u32 version);
int format_tvcf_version(u32 version, char *buff, size_t size);

void conv_u32_u8arr(u32 val, u8 *buf);
void conv_u16_u8arr(u16 val, u8 *buf);

int prepare_write_cmd(struct sensor_blob *pkt,
			u32 size, u32 addr, u8 *buf);
int prepare_read_cmd(struct sensor_blob *pkt,
			u32 size, u32 addr);
int prepare_sleep_cmd(struct sensor_blob *pkt, u32 time_in_us);
int prepare_done_cmd(struct sensor_blob *pkt);

#endif
