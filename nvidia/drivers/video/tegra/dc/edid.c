/*
 * edid.c: tegra edid functions.
 *
 * Copyright (C) 2010 Google, Inc.
 * Author: Erik Gilling <konkers@android.com>
 *
 * Copyright (c) 2010-2022, NVIDIA CORPORATION. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/debugfs.h>
#include <linux/fb.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>

#include "edid.h"
#include "dc_priv.h"
#include "hdmi2.0.h"

struct tegra_edid_pvt {
	struct kref			refcnt;
	struct tegra_edid_hdmi_eld	eld;
	struct tegra_dc_ext_dv_caps	dv_caps;
	bool				support_stereo;
	bool				support_underscan;
	bool				support_audio;
	bool				scdc_present;
	bool				db420_present;
	bool				support_yuv422;
	bool				support_yuv444;
	bool				rgb_quant_selectable;
	bool				yuv_quant_selectable;
	u16			color_depth_flag;
	u16			max_tmds_char_rate_hf_mhz;
	u16			max_tmds_char_rate_hllc_mhz;
	u16			colorimetry;
	u16			min_vrr_fps;
	u8			hdr_pckt_len;
	bool			hdr_eotf_smpte2084;
	u8			hdr_eotf;
	u8			hdr_static_metadata;
	u8			hdr_desired_max_luma;
	u8			hdr_desired_max_frame_avg_luma;
	u8			hdr_desired_min_luma;
	u32			quirks;
	/* Note: dc_edid must remain the last member */
	struct tegra_dc_edid		dc_edid;
};

/* 720p 60Hz EDID */
static const char default_720p_edid[256] = {
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
	0x3a, 0xc4, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x05, 0x1b, 0x01, 0x03, 0x80, 0x59, 0x32, 0x8c,
	0x0a, 0xe2, 0xbd, 0xa1, 0x5b, 0x4a, 0x98, 0x24,
	0x15, 0x47, 0x4a, 0x20, 0x00, 0x00, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1d,
	0x00, 0x72, 0x51, 0xd0, 0x1e, 0x20, 0x6e, 0x28,
	0x55, 0x00, 0x75, 0xf2, 0x31, 0x00, 0x00, 0x1e,
	0x01, 0x1d, 0x00, 0xbc, 0x52, 0xd0, 0x1e, 0x20,
	0xb8, 0x28, 0x55, 0x40, 0x75, 0xf2, 0x31, 0x00,
	0x00, 0x1e, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x32,
	0x3d, 0x0f, 0x2e, 0x08, 0x00, 0x0a, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfc,
	0x00, 0x4e, 0x56, 0x49, 0x44, 0x49, 0x41, 0x00,
	0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0xf9,
	0x02, 0x03, 0x19, 0x71, 0x46, 0x84, 0x13, 0x05,
	0x14, 0x03, 0x12, 0x23, 0x09, 0x07, 0x07, 0x83,
	0x01, 0x00, 0x00, 0x65, 0x03, 0x0c, 0x00, 0x10,
	0x00, 0x01, 0x1d, 0x80, 0x18, 0x71, 0x1c, 0x16,
	0x20, 0x58, 0x2c, 0x25, 0x00, 0x75, 0xf2, 0x31,
	0x00, 0x00, 0x9e, 0x01, 0x1d, 0x80, 0xd0, 0x72,
	0x1c, 0x16, 0x20, 0x10, 0x2c, 0x25, 0x80, 0x75,
	0xf2, 0x31, 0x00, 0x00, 0x9e, 0x8c, 0x0a, 0xd0,
	0x8a, 0x20, 0xe0, 0x2d, 0x10, 0x10, 0x3e, 0x96,
	0x00, 0x75, 0xf2, 0x31, 0x00, 0x00, 0x18, 0x8c,
	0x0a, 0xd0, 0x90, 0x20, 0x40, 0x31, 0x20, 0x0c,
	0x40, 0x55, 0x00, 0x75, 0xf2, 0x31, 0x00, 0x00,
	0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xca,
};

#ifdef DEBUG
static char tegra_edid_dump_buff[16 * 1024];

static void tegra_edid_dump(struct tegra_edid *edid)
{
	struct seq_file s;
	int i;
	char c;

	memset(&s, 0x0, sizeof(s));

	s.buf = tegra_edid_dump_buff;
	s.size = sizeof(tegra_edid_dump_buff);
	s.private = edid;

	tegra_edid_show(&s, NULL);

	i = 0;
	while (i < s.count ) {
		if ((s.count - i) > 256) {
			c = s.buf[i + 256];
			s.buf[i + 256] = 0;
			printk("%s", s.buf + i);
			s.buf[i + 256] = c;
		} else {
			printk("%s", s.buf + i);
		}
		i += 256;
	}
}
#else
static void tegra_edid_dump(struct tegra_edid *edid)
{
}
#endif


int tegra_edid_i2c_adap_change_rate(struct i2c_adapter *i2c_adap, int rate)
{
	const int MIN_RATE = 5000, MAX_RATE = 4000000;
	int err = 0, cur_rate = 0;
	if (rate < MIN_RATE || rate > MAX_RATE) {
		pr_warn("Cannot change the i2c_ddc rate, the rate:%d cannot"
"be below minimum rate:%d or above maximum rate:%d", rate, MIN_RATE, MAX_RATE);
		return -1;
	}

	if (i2c_adap) {
		cur_rate = i2c_get_adapter_bus_clk_rate(i2c_adap);
		if (cur_rate == rate)
			return 0;

		err = i2c_set_adapter_bus_clk_rate(i2c_adap, rate);
		if (err)
			pr_warn("Could not change i2c_ddc sclk rate\n");
		else
			pr_info("Switching i2c_ddc sclk rate: from %d, "
"to %d\n", cur_rate, rate);
	} else {
		pr_warn("ddc i2c adapter NULL\n");
		err = -1;
	}
	return err;
}

int tegra_edid_i2c_divide_rate(struct tegra_edid *edid)
{
	struct i2c_adapter *i2c_adap = i2c_get_adapter(edid->dc->out->ddc_bus);
	int new_rate = 0, old_rate = 0, err = 0;

	if (i2c_adap) {
		old_rate = i2c_get_adapter_bus_clk_rate(i2c_adap);
		new_rate = old_rate >> 1;
		err = tegra_edid_i2c_adap_change_rate(i2c_adap, new_rate);
	} else
		err = -1;
	return err;
}

int tegra_edid_read_block(struct tegra_edid *edid, int block, u8 *data)
{
	u8 block_buf[] = {block >> 1};
	u8 cmd_buf[] = {(block % 0x2) * EDID_BYTES_PER_BLOCK};
	u8 i;
	u8 last_checksum = 0;
	u8 checksum = 0;
	size_t attempt_cnt = 0;
	struct i2c_msg msg[] = {
		{
			.addr = 0x30,
			.flags = 0,
			.len = 1,
			.buf = block_buf,
		},
		{
			.addr = 0x50,
			.flags = 0,
			.len = 1,
			.buf = cmd_buf,
		},
		{
			.addr = 0x50,
			.flags = I2C_M_RD,
			.len = EDID_BYTES_PER_BLOCK,
			.buf = data,
		}};
	struct i2c_msg *m;
	int msg_len;
	if (block > 1) {
		msg_len = 3;
		m = msg;
	} else {
		msg_len = 2;
		m = &msg[1];
	}

	do {
		int status = edid->i2c_ops.i2c_transfer(edid->dc, m, msg_len);

		checksum = 0;

		if (status < 0)
			return status;

		if (status != msg_len)
			return -EIO;

		/* fix base block header if corrupted */
		if (!block) {
			for (i = 0; i < EDID_BASE_HEADER_SIZE; i++) {
				if (data[i] != edid_base_header[i])
					data[i] = edid_base_header[i];
			}
		}

		for (i = 0; i < EDID_BYTES_PER_BLOCK; i++)
			checksum += data[i];
		if (checksum != 0) {
			/*
			 * It is completely possible that the sink that we are
			 * reading has a bad EDID checksum (specifically, some
			 * of the older TVs). These TVs have the modes, etc
			 * programmed in their EDID correctly, but just have
			 * a bad checksum. It then becomes hard to distinguish
			 * between an i2c failure vs bad EDID.
			 * To get around this, read the EDID multiple times.
			 * If the calculated checksum is the exact same
			 * multiple number of times, just print a
			 * warning and ignore.
			 */

			if (attempt_cnt == 0)
				last_checksum = checksum;

			/* On different checksum remainder, lower i2c speed */
			if (last_checksum != checksum) {
				pr_warn("%s: checksum failed and did not match consecutive reads. Previous remainder was %u. New remainder is %u. Failed at attempt %zu\n",
					__func__, last_checksum, checksum, attempt_cnt);
				if (tegra_edid_i2c_divide_rate(edid)) {
					pr_warn("Cannot halve i2c speed giving"
"up on trying to change the i2c speed for EDID read\n");
					return -EIO;
				} else {
					attempt_cnt = 0;
					continue;
				}
			}
			usleep_range(TEGRA_EDID_MIN_RETRY_DELAY_US, TEGRA_EDID_MAX_RETRY_DELAY_US);
		}
	} while (checksum != 0 && ++attempt_cnt < TEGRA_EDID_MAX_RETRY);

	/*
	 * Re-calculate the checksum since the standard EDID parser doesn't
	 * like the bad checksum
	 */
	if (checksum != 0) {
		checksum = 0;

		edid->errors |= EDID_ERRORS_CHECKSUM_CORRUPTED;

		for (i = 0; i < 127; i++)
			checksum += data[i];

		checksum = (u8)(256 - checksum);
		data[127] = checksum;

		pr_warn("%s: remainder is %u for the last %d attempts. Assuming bad sink EDID and ignoring. New checksum is %u\n",
				__func__, last_checksum, TEGRA_EDID_MAX_RETRY,
				checksum);
	}

	return 0;
}

static void tegra_edid_parse_dv_caps(struct tegra_edid_pvt *edid,
			const u8 *ptr, u32 vsvdb_size)
{
	u32 dv_vsvdb_ver;
	struct tegra_dc_ext_dv_caps_vsvdb_v0 *v0;
	struct tegra_dc_ext_dv_caps_vsvdb_v1_15b *v1_15b;
	struct tegra_dc_ext_dv_caps_vsvdb_v1_12b *v1_12b;
	struct tegra_dc_ext_dv_caps_vsvdb_v2 *v2;

	if (edid->dv_caps.vsvdb_ver != TEGRA_DC_DV_VSVDB_NONE) {
		/* earlier parsed VSVDB marked the sink as DV capable */
		return;
	}

	dv_vsvdb_ver = ((ptr[5] & 0xe0) >> 5);

	/* Check version bits and populate dv caps accordingly.
	 *
	 * Note, that when certain AVR are connected to certain HDR10+ and Dolby
	 * Vision capable sinks, HDR10+ and Dolby Vision VSVDB, in this order,
	 * will be squashed in EDID. We try to WAR such malformed EDID, but as
	 * a precaution, we also check if the length of VSVDB matches the length
	 * according to Dolby Vision VSVDB version.
	 */
	switch (dv_vsvdb_ver) {
	case 0:
		/* version 0 */
		if (vsvdb_size != TEGRA_DC_DV_VSVDB_V0_SIZE)
			break;
		edid->dv_caps.vsvdb_ver = TEGRA_DC_DV_VSVDB_V0;
		v0 = &edid->dv_caps.v0;
		v0->dm_version = ptr[21];
		v0->supports_YUV422_12bit = (ptr[5] & 0x1);
		v0->supports_2160p60hz = ((ptr[5] & 0x2) >> 1);
		v0->supports_global_dimming = ((ptr[5] & 0x4) >> 2);
		v0->target_min_pq = ((ptr[19] << 4) | ((ptr[18] & 0xf0) >> 4));
		v0->target_max_pq = ((ptr[20] << 4) | (ptr[18] & 0x0f));
		v0->cc_red_x = ((ptr[7] << 4) | ((ptr[6] & 0xf0) >> 4));
		v0->cc_red_y = ((ptr[8] << 4) | (ptr[16] & 0x0f));
		v0->cc_green_x = ((ptr[10] << 4) | ((ptr[9] & 0xf0) >> 4));
		v0->cc_green_y = ((ptr[11] << 4) | (ptr[9] & 0x0f));
		v0->cc_blue_x = ((ptr[13] << 4) | ((ptr[12] & 0xf0) >> 4));
		v0->cc_blue_y = ((ptr[14] << 4) | (ptr[12] & 0x0f));
		v0->cc_white_x = ((ptr[16] << 4) | ((ptr[15] & 0xf0) >> 4));
		v0->cc_white_y = ((ptr[17] << 4) | (ptr[15] & 0x0f));
		break;
	case 1:
		/* version 1, check size for differentiating*/
		if (vsvdb_size == TEGRA_DC_DV_VSVDB_V1_15B_SIZE) {
			/* version 1, 15 byte */
			edid->dv_caps.vsvdb_ver = TEGRA_DC_DV_VSVDB_V1_15B;
			v1_15b = &edid->dv_caps.v1_15b;
			v1_15b->supports_YUV422_12bit = (ptr[5] & 0x1);
			v1_15b->supports_2160p60hz = ((ptr[5] & 0x2) >> 1);
			v1_15b->dm_version = ((ptr[5] & 0x1c) >> 2);
			v1_15b->supports_global_dimming = (ptr[6] & 0x1);
			v1_15b->target_max_luminance = ((ptr[6] & 0xfe) >> 1);
			v1_15b->colorimetry = (ptr[7] & 0x1);
			v1_15b->target_min_luminance = ((ptr[7] & 0xfe) >> 1);
			v1_15b->cc_red_x = ptr[9];
			v1_15b->cc_red_y = ptr[10];
			v1_15b->cc_green_x = ptr[11];
			v1_15b->cc_green_y = ptr[12];
			v1_15b->cc_blue_x = ptr[13];
			v1_15b->cc_blue_y = ptr[14];
		} else if (vsvdb_size == TEGRA_DC_DV_VSVDB_V1_12B_SIZE) {
			/* version 1, 12 byte */
			edid->dv_caps.vsvdb_ver = TEGRA_DC_DV_VSVDB_V1_12B;
			v1_12b = &edid->dv_caps.v1_12b;
			v1_12b->supports_YUV422_12bit = (ptr[5] & 0x1);
			v1_12b->supports_2160p60hz = ((ptr[5] & 0x2) >> 1);
			v1_12b->dm_version = ((ptr[5] & 0x1c) >> 2);
			v1_12b->supports_global_dimming = (ptr[6] & 0x1);
			v1_12b->target_max_luminance = ((ptr[6] & 0xfe) >> 1);
			v1_12b->colorimetry = (ptr[7] & 0x1);
			v1_12b->target_min_luminance = ((ptr[7] & 0xfe) >> 1);
			v1_12b->cc_red_x = 0xA0 | ((ptr[11] & 0xf8) >> 3);
			v1_12b->cc_red_y = 0x40 | (((ptr[11] & 0x7) << 2) |
				((ptr[10] & 0x1) << 1) | (ptr[9] & 0x1));
			v1_12b->cc_green_x = 0x00 | ((ptr[9] & 0xfe) >> 1);
			v1_12b->cc_green_y = 0x80 | ((ptr[10] & 0xfe) >> 1);
			v1_12b->cc_blue_x = 0x20 | ((ptr[8] & 0xe0) >> 5);
			v1_12b->cc_blue_y = 0x08 | ((ptr[8] & 0x1c) >> 2);
			v1_12b->low_latency = (ptr[8] & 0x3);
		}
		break;
	case 2:
		/* version 2 */
		if (vsvdb_size != TEGRA_DC_DV_VSVDB_V2_SIZE)
			break;
		edid->dv_caps.vsvdb_ver = TEGRA_DC_DV_VSVDB_V2;
		v2 = &edid->dv_caps.v2;
		v2->dm_version = ((ptr[5] & 0x1c) >> 2);
		v2->supports_backlight_control = ((ptr[5] & 0x2) >> 1);
		v2->supports_YUV422_12bit = (ptr[5] & 0x1);
		v2->supports_global_dimming = ((ptr[6] & 0x4) >> 2);
		v2->backlt_min_luma = (ptr[6] & 0x3);
		v2->target_max_pq_v2 = ((ptr[7] & 0xf8) >> 3);
		v2->target_min_pq_v2 = ((ptr[6] & 0xf8) >> 3);
		v2->interface_supported_by_sink = (ptr[7] & 0x3);
		v2->cc_red_x = 0xA0 | ((ptr[10] & 0xf8) >> 3);
		v2->cc_red_y = 0x40 | ((ptr[11] & 0xf8) >> 3);
		v2->cc_green_x = 0x00 | ((ptr[8] & 0xfe) >> 1);
		v2->cc_green_y = 0x80 | ((ptr[9] & 0xfe) >> 1);
		v2->cc_blue_x = 0x20 | (ptr[10] & 0x07);
		v2->cc_blue_y = 0x08 | (ptr[11] & 0x07);
		v2->supports_10b_12b_444 =
			((ptr[8] & 0x1 << 1) | (ptr[9] & 0x1));
		break;
	}
}

static void tegra_edid_parse_vsvdb(struct tegra_edid_pvt *edid, const u8 *ptr)
{
	u32 vsvdb_size = (ptr[0] & 0x1f);
	u32 ieee_id = ((ptr[2]) | (ptr[3] << 8) | (ptr[4] << 16));

	/* Quirk for bug 2875137: HDR10+ and DV VSVDB, in this order, may happen
	 * to be squashed in EDID of certain sinks. Therefore we attempt to
	 * recognise HDR10+ VSVDB first, and if there are bytes left in data
	 * part, we try to interpret the rest as DV VSVDB
	 */

	if (ieee_id == IEEE_CEA861_HDR10P_ID) {
		/* HDR10+ is not implemented, therefore just ignore it. And,
		 * if HDR10+ was the only content, end here. Otherwise advance
		 * to data portion of the following supposed to be VSVDB. Note,
		 * HDR10+ VSVDB has fixed length of 5 bytes, unlike e.g. Dolby
		 * Vision VSVDB.
		 */
		if (vsvdb_size == 5)
			return;
		ptr += 4;
		vsvdb_size -= 4;
		ieee_id = ((ptr[2]) | (ptr[3] << 8) | (ptr[4] << 16));
	}

	if (ieee_id == IEEE_CEA861_DV_ID) {
		tegra_edid_parse_dv_caps(edid, ptr, vsvdb_size);
	}
}

static int tegra_edid_parse_ext_block(const u8 *raw, int idx,
			       struct tegra_edid_pvt *edid)
{
	const u8 *ptr;
	u8 tmp;
	u8 code;
	int len;
	int i;
	bool basic_audio = false;

	if (!edid) {
		pr_err("%s: invalid argument\n", __func__);
		return -EINVAL;
	}
	ptr = &raw[0];

	/* If CEA 861 block get info for eld struct */
	if (ptr) {
		if (*ptr <= 3)
			edid->eld.eld_ver = 0x02;
		edid->eld.cea_edid_ver = ptr[1];

		/* check for basic audio support in CEA 861 block */
		if(raw[3] & (1<<6)) {
			/* For basic audio, set spk_alloc to Left+Right.
			 * If there is a Speaker Alloc block this will
			 * get over written with that value */
			basic_audio = true;
			edid->support_audio = 1;
		}
	}

	if (raw[3] & 0x80)
		edid->support_underscan = 1;
	else
		edid->support_underscan = 0;

	if (raw[3] & (1<<5))
		edid->support_yuv444 = 1;
	else
		edid->support_yuv444 = 0;

	if (raw[3] & (1<<4))
		edid->support_yuv422 = 1;
	else
		edid->support_yuv422 = 0;
	ptr = &raw[4];

	while (ptr < &raw[idx]) {
		tmp = *ptr;
		len = tmp & 0x1f;

		/* HDMI Specification v1.4a, section 8.3.2:
		 * see Table 8-16 for HDMI VSDB format.
		 * data blocks have tags in top 3 bits:
		 * tag code 2: video data block
		 * tag code 3: vendor specific data block
		 */
		code = (tmp >> 5) & 0x7;
		switch (code) {
		case CEA_DATA_BLOCK_AUDIO:
		{
			int sad_n = edid->eld.sad_count * 3;
			edid->eld.sad_count += len / 3;
			pr_debug("%s: incrementing eld.sad_count by %d to %d\n",
				 __func__, len / 3, edid->eld.sad_count);
			edid->eld.conn_type = 0x00;
			edid->eld.support_hdcp = 0x00;
			for (i = 0; (i < len) && (sad_n < ELD_MAX_SAD_BYTES);
			     i++, sad_n++)
				edid->eld.sad[sad_n] = ptr[i + 1];
			len++;
			ptr += len; /* adding the header */
			/* Got an audio data block so enable audio */
			if (basic_audio == true)
				edid->eld.spk_alloc = 1;
			if (edid->quirks & TEGRA_EDID_QUIRK_IGNORE_EAC3) {
				for (i = 0; i < edid->eld.sad_count; i++) {
					/* Bits 3-6 of Byte 0 will have the Audio Format */
					unsigned int format = (edid->eld.sad[i*ELD_SAD_LENGTH]
											& 0x78) >> 3;

					if (format == AUDIO_CODING_TYPE_EAC3) {
						pr_warn("%s: format is E_AC3 skip it", __func__);
						edid->eld.sad[i*ELD_SAD_LENGTH] = 0;
					}
				}
			}
			break;
		}
		/* case 2 is commented out for now */
		case CEA_DATA_BLOCK_VENDOR:
		{
			int j = 0;
			u16 temp = 0;
			/* OUI for hdmi licensing, LLC */
			if ((ptr[1] == 0x03) &&
				(ptr[2] == 0x0c) &&
				(ptr[3] == 0)) {
				edid->eld.port_id[0] = ptr[4];
				edid->eld.port_id[1] = ptr[5];
				temp = ptr[6];
				edid->color_depth_flag = (temp << 5) &
							TEGRA_DC_RGB_MASK;
				if (edid->support_yuv422 && (temp & 0x08))
					edid->color_depth_flag |= (temp >> 1) &
							TEGRA_DC_Y422_MASK;
				if (edid->support_yuv444 && (temp & 0x08))
					edid->color_depth_flag |= (temp << 2) &
							TEGRA_DC_Y444_MASK;
				if (len >= 7)
					edid->max_tmds_char_rate_hllc_mhz =
								ptr[7] * 5;
				edid->max_tmds_char_rate_hllc_mhz =
					edid->max_tmds_char_rate_hllc_mhz ? :
					165; /* for <=165MHz field may be 0 */
			}

			/* OUI for hdmi forum */
			if ((ptr[1] == 0xd8) &&
				(ptr[2] == 0x5d) &&
				(ptr[3] == 0xc4)) {
				/* Read Sink Capability Data Structure (SCDS) */
				edid->color_depth_flag |= ptr[7] &
							TEGRA_DC_Y420_MASK;
				edid->max_tmds_char_rate_hf_mhz = ptr[5] * 5;
				edid->scdc_present = (ptr[6] >> 7) & 0x1;
			}

			/* OUI for Nvidia */
			if ((ptr[1] == 0x4b) &&
				(ptr[2] == 0x04) &&
				(ptr[3] == 0)) {
				/* version 1.0 vrr capabilities */
				if (ptr[4] == 1)
					edid->min_vrr_fps = ptr[5];
			}

			if ((len >= 8) &&
				(ptr[1] == 0x03) &&
				(ptr[2] == 0x0c) &&
				(ptr[3] == 0)) {
				j = 8;
				tmp = ptr[j++];
				/* HDMI_Video_present? */
				if (tmp & 0x20) {
					/* Latency_Fields_present? */
					if (tmp & 0x80)
						j += 2;
					/* I_Latency_Fields_present? */
					if (tmp & 0x40)
						j += 2;
					/* 3D_present? */
					if (j <= len && (ptr[j] & 0x80))
						edid->support_stereo = 1;
				}
			}
			if ((len > 5) &&
				(ptr[1] == 0x03) &&
				(ptr[2] == 0x0c) &&
				(ptr[3] == 0)) {

				edid->eld.support_ai = (ptr[6] & 0x80);
			}

			if ((len > 9) &&
				(ptr[1] == 0x03) &&
				(ptr[2] == 0x0c) &&
				(ptr[3] == 0)) {

				edid->eld.aud_synch_delay = ptr[10];
			}
			len++;
			ptr += len; /* adding the header */
			break;
		}
		case CEA_DATA_BLOCK_SPEAKER_ALLOC:
		{
			edid->eld.spk_alloc = ptr[1];
			len++;
			ptr += len; /* adding the header */
			break;
		}
		case CEA_DATA_BLOCK_EXT:
		{
			u8 ext_db = ptr[1];

			switch (ext_db) {
			case CEA_DATA_BLOCK_EXT_VCDB:
				edid->rgb_quant_selectable = ptr[2] & 0x40;
				edid->yuv_quant_selectable = ptr[2] & 0x80;
				break;
			case CEA_DATA_BLOCK_EXT_Y420VDB: /* fall through */
			case CEA_DATA_BLOCK_EXT_Y420CMDB:
				edid->db420_present = true;
				break;
			case CEA_DATA_BLOCK_EXT_CDB:
				edid->colorimetry = ptr[2];
				break;
			case CEA_DATA_BLOCK_EXT_HDR:
				edid->hdr_pckt_len = ptr[0] & 0x1f;
				edid->hdr_eotf_smpte2084 = ptr[2] &
					TEGRA_DC_EXT_CEA861_3_EOTF_SMPTE_2084;
				edid->hdr_eotf = ptr[2];
				edid->hdr_static_metadata = ptr[3];
				if (edid->hdr_pckt_len > 5) {
					edid->hdr_desired_max_luma = ptr[4];
					edid->hdr_desired_max_frame_avg_luma =
									ptr[5];
					edid->hdr_desired_min_luma = ptr[6];
				} else if (edid->hdr_pckt_len > 4) {
					edid->hdr_desired_max_luma = ptr[4];
					edid->hdr_desired_max_frame_avg_luma =
									ptr[5];
				} else if (edid->hdr_pckt_len > 3) {
					edid->hdr_desired_max_luma = ptr[4];
				}
				break;
			case CEA_DATA_BLOCK_EXT_VSVDB:
				tegra_edid_parse_vsvdb(edid, ptr);
				break;
			case CEA_DATA_BLOCK_EXT_SCDB:
				/* Read Sink Capability Data Structure (SCDS) */
				edid->color_depth_flag |= ptr[7] &
							TEGRA_DC_Y420_MASK;
				edid->max_tmds_char_rate_hf_mhz = ptr[5] * 5;
				edid->scdc_present = (ptr[6] >> 7) & 0x1;
				break;
			};

			len++;
			ptr += len;
			break;
		}
		default:
			len++; /* len does not include header */
			ptr += len;
			break;
		}
	}
	/*
	 * Copying user data from raw to edid.
	 * raw is being copied from edid->dc->vedid_data in calling function.
	 * edid->dc->vedid_data is being populated through debugfs(edid_fops).
	 */
	spec_bar();
	return 0;
}

static int tegra_edid_mode_support_stereo(struct fb_videomode *mode)
{
	if (!mode)
		return 0;

	if (mode->xres == 1280 &&
		mode->yres == 720 &&
		((mode->refresh == 60) || (mode->refresh == 50)))
		return 1;

	if (mode->xres == 1920 && mode->yres == 1080 && mode->refresh == 24)
		return 1;

	return 0;
}

static void data_release(struct kref *ref)
{
	struct tegra_edid_pvt *data =
		container_of(ref, struct tegra_edid_pvt, refcnt);
	vfree(data);
}

u16 tegra_edid_get_cd_flag(struct tegra_edid *edid)
{
	if (!edid || !edid->data) {
		pr_warn("edid invalid\n");
		return -EFAULT;
	}

	return edid->data->color_depth_flag;
}

u16 tegra_edid_get_ex_hdr_cap(struct tegra_edid *edid)
{
	u16 ret = 0;
	struct tegra_dc_out *default_out;

	if (!edid || !edid->data) {
		pr_warn("edid invalid\n");
		return -EFAULT;
	}

	if (edid->dc && edid->dc->pdata && edid->dc->pdata->default_out) {
		default_out = edid->dc->pdata->default_out;
		if ((default_out->type == TEGRA_DC_OUT_HDMI) &&
			(default_out->hdmi_out->generic_infoframe_type !=
			HDMI_INFOFRAME_TYPE_HDR)) {
			pr_debug("hdmi generic infoframe is not for hdr\n");
			return ret;
		}
	}

	if (edid->data->hdr_eotf_smpte2084)
		ret |= FB_CAP_SMPTE_2084;

	return ret;
}

int tegra_edid_get_ex_hdr_cap_info(struct tegra_edid *edid,
			struct tegra_dc_ext_hdr_caps *hdr_cap_info)
{
	int ret = 0;
	struct tegra_dc_out *default_out;

	if (!edid || !edid->data) {
		pr_warn("edid invalid\n");
		return -EFAULT;
	}

	if (edid->dc && edid->dc->pdata && edid->dc->pdata->default_out) {
		default_out = edid->dc->pdata->default_out;
		if ((default_out->type == TEGRA_DC_OUT_HDMI) &&
			(default_out->hdmi_out->generic_infoframe_type !=
			HDMI_INFOFRAME_TYPE_HDR)) {
			pr_debug("hdmi generic infoframe is not for hdr\n");
			return ret;
		}
	}

	if (!edid->data->hdr_pckt_len)
		return ret;

	hdr_cap_info->nr_elements = edid->data->hdr_pckt_len;
	hdr_cap_info->eotf = edid->data->hdr_eotf;
	hdr_cap_info->static_metadata_type =
			edid->data->hdr_static_metadata;
	hdr_cap_info->desired_content_max_lum =
			edid->data->hdr_desired_max_luma;
	hdr_cap_info->desired_content_max_frame_avg_lum =
			edid->data->hdr_desired_max_frame_avg_luma;
	hdr_cap_info->desired_content_min_lum =
			edid->data->hdr_desired_min_luma;

	return ret;
}

void tegra_edid_get_ex_dv_cap_info(struct tegra_edid *edid,
			struct tegra_dc_ext_dv_caps *dv_cap_info)
{
	if (!edid || !edid->data) {
		pr_warn("%s: edid invalid\n", __func__);
		return;
	}

	if (edid->data->dv_caps.vsvdb_ver == TEGRA_DC_DV_VSVDB_NONE)
		return;

	memcpy(dv_cap_info, &edid->data->dv_caps,
		sizeof(struct tegra_dc_ext_dv_caps));

}

inline bool tegra_edid_is_rgb_quantization_selectable(struct tegra_edid *edid)
{
	if (!edid || !edid->data) {
		return false;
	}
	return edid->data->rgb_quant_selectable;
}

inline bool tegra_edid_is_yuv_quantization_selectable(struct tegra_edid *edid)
{
	if (!edid || !edid->data) {
		return false;
	}
	return edid->data->yuv_quant_selectable;
}

int tegra_edid_get_ex_quant_cap_info(struct tegra_edid *edid,
			struct tegra_dc_ext_quant_caps *quant_cap_info)
{
	if (!edid || !edid->data) {
		pr_warn("edid invalid\n");
		return -EINVAL;
	}

	quant_cap_info->rgb_quant_selectable
		= edid->data->rgb_quant_selectable;

	quant_cap_info->yuv_quant_selectable
		= edid->data->yuv_quant_selectable;

	return 0;
}

/* hdmi spec mandates sink to specify correct max_tmds_clk only for >165MHz */
u16 tegra_edid_get_max_clk_rate(struct tegra_edid *edid)
{
	u16 tmds_hf, tmds_llc;

	if (!edid || !edid->data) {
		pr_warn("edid invalid\n");
		return -EFAULT;
	}

	tmds_hf = edid->data->max_tmds_char_rate_hf_mhz;
	tmds_llc = edid->data->max_tmds_char_rate_hllc_mhz;

	if (tmds_hf || tmds_llc)
		return tmds_hf ? : tmds_llc;

	return 0;
}

bool tegra_edid_require_dv_vsif(struct tegra_edid *edid)
{
	if (!edid || !edid->data) {
		pr_warn("edid invalid\n");
		return false;
	}

	/* Dolby Vision VSVDB version 1, 12-byte with low-latency
	 * support and VSVDB version 2 require Dolby VSIF.
	 */
	if (((edid->data->dv_caps.vsvdb_ver == TEGRA_DC_DV_VSVDB_V1_12B) &&
	     (edid->data->dv_caps.v1_12b.low_latency == 0x1)) ||
	    (edid->data->dv_caps.vsvdb_ver == TEGRA_DC_DV_VSVDB_V2))
		return true;
	else
		return false;
}

bool tegra_edid_support_dv_std_422(struct tegra_edid *edid)
{
	if (!edid || !edid->data) {
		pr_warn("edid invalid\n");
		return false;
	}

	if (((edid->data->dv_caps.vsvdb_ver == TEGRA_DC_DV_VSVDB_V0) &&
	     (edid->data->dv_caps.v0.supports_YUV422_12bit)) ||
	    ((edid->data->dv_caps.vsvdb_ver == TEGRA_DC_DV_VSVDB_V1_15B) &&
	     (edid->data->dv_caps.v1_15b.supports_YUV422_12bit)) ||
	    ((edid->data->dv_caps.vsvdb_ver == TEGRA_DC_DV_VSVDB_V1_12B) &&
	     (edid->data->dv_caps.v1_12b.supports_YUV422_12bit)) ||
	    ((edid->data->dv_caps.vsvdb_ver == TEGRA_DC_DV_VSVDB_V2) &&
	     (edid->data->dv_caps.v2.supports_YUV422_12bit)))
		return true;
	else
		return false;
}

bool tegra_edid_is_scdc_present(struct tegra_edid *edid)
{
	if (tegra_platform_is_vdk())
		return false;

	if (!edid || !edid->data) {
		pr_warn("edid invalid\n");
		return false;
	}

	return edid->data->scdc_present;
}

bool tegra_edid_is_420db_present(struct tegra_edid *edid)
{
	if (!edid || !edid->data) {
		pr_warn("edid invalid\n");
		return false;
	}

	return edid->data->db420_present;
}

u32 tegra_edid_get_quirks(struct tegra_edid *edid)
{
	if (!edid || !edid->data) {
		pr_warn("edid invalid\n");
		return 0;
	}

	return edid->data->quirks;
}

u16 tegra_edid_get_ex_colorimetry(struct tegra_edid *edid)
{
	if (!edid || !edid->data) {
		pr_warn("edid invalid\n");
		return 0;
	}

	return edid->data->colorimetry;
}

bool tegra_edid_support_yuv422(struct tegra_edid *edid)
{
	if (!edid || !edid->data) {
		pr_warn("edid invalid\n");
		return 0;
	}

	return edid->data->support_yuv422;
}

bool tegra_edid_support_yuv444(struct tegra_edid *edid)
{
	if (!edid || !edid->data) {
		pr_warn("edid invalid\n");
		return 0;
	}

	return edid->data->support_yuv444;
}

/* Add VIC modes with id 96 and 97 */
void tegra_edid_quirk_lg_sbar(struct tegra_edid_pvt *new_data,
			      struct fb_monspecs *specs)
{
	struct fb_videomode *m;

	/* Additional checks that we got the specific EDID */
	if (new_data->max_tmds_char_rate_hf_mhz != 450 ||
	    new_data->dv_caps.vsvdb_ver != TEGRA_DC_DV_VSVDB_V1_12B ||
	    new_data->dv_caps.v1_12b.supports_2160p60hz == 0) {
		return;
	}

	m = kzalloc((specs->modedb_len + 2) *
		    sizeof(struct fb_videomode), GFP_KERNEL);
	if (m) {
		memcpy(m, specs->modedb,
		       specs->modedb_len * sizeof(struct fb_videomode));

		memcpy(m + specs->modedb_len, &cea_modes[96],
		       sizeof(struct fb_videomode));
		m[specs->modedb_len].vmode |= FB_VMODE_IS_CEA;
		specs->modedb_len++;
		memcpy(m + specs->modedb_len, &cea_modes[97],
		       sizeof(struct fb_videomode));
		m[specs->modedb_len].vmode |= FB_VMODE_IS_CEA;
		specs->modedb_len++;

		kfree(specs->modedb);
		specs->modedb = m;
	}
}

/* Fix length of VSVDB */
void tegra_edid_quirk_vsvdb_len(u8 *data)
{
	/* Fix only a specific length of Dolby Vision VSVDB */
	if (data[EDID_BYTES_PER_BLOCK-23] != 0xef ||
	    data[EDID_BYTES_PER_BLOCK-21] != ((IEEE_CEA861_DV_ID >> 0) & 0xff) ||
	    data[EDID_BYTES_PER_BLOCK-20] != ((IEEE_CEA861_DV_ID >> 8) & 0xff) ||
	    data[EDID_BYTES_PER_BLOCK-19] != ((IEEE_CEA861_DV_ID >> 16) & 0xff))
		return;

	/* Additional check of checksum that we got the specific EDID */
	if (data[EDID_BYTES_PER_BLOCK-1] != 0x9c)
		return;

	/* Fix the length and adjust checksum */
	data[EDID_BYTES_PER_BLOCK-23] -= 4;
	data[EDID_BYTES_PER_BLOCK-1] += 4;
}

int tegra_edid_get_monspecs(struct tegra_edid *edid, struct fb_monspecs *specs)
{
	int i;
	int j;
	int ret;
	int extension_blocks;
	struct tegra_edid_pvt *new_data, *old_data;
	u8 checksum = 0;
	u8 *data;
	bool use_fallback = false;

	new_data = vzalloc(SZ_32K + sizeof(struct tegra_edid_pvt));
	if (!new_data)
		return -ENOMEM;

	kref_init(&new_data->refcnt);

	if (edid->errors & EDID_ERRORS_READ_FAILED)
		use_fallback = true;

	edid->errors = 0;

	data = new_data->dc_edid.buf;

	if (edid->dc->vedid) {
		memcpy(data, edid->dc->vedid_data, EDID_BYTES_PER_BLOCK);
		/* checksum new edid */
		for (i = 0; i < EDID_BYTES_PER_BLOCK; i++)
			checksum += data[i];
		if (checksum != 0) {
			pr_err("%s: checksum failed\n", __func__);
			ret = -EINVAL;
			goto fail;
		}
	} else if (use_fallback) {
		memcpy(data, default_720p_edid, EDID_BYTES_PER_BLOCK);
		/* no checksum test needed */
	} else {
		ret = tegra_edid_read_block(edid, 0, data);
		if (ret)
			goto fail;
	}

	memset(specs, 0x0, sizeof(struct fb_monspecs));
	memset(&new_data->eld, 0x0, sizeof(new_data->eld));
	fb_edid_to_monspecs(data, specs);
	if (specs->modedb == NULL)
		pr_info("%s: no modes in EDID base block\n", __func__);

	memcpy(new_data->eld.monitor_name, specs->monitor, sizeof(specs->monitor));
	new_data->eld.mnl = strlen(new_data->eld.monitor_name) + 1;
	new_data->eld.product_id[0] = data[0x8];
	new_data->eld.product_id[1] = data[0x9];
	new_data->eld.manufacture_id[0] = data[0xA];
	new_data->eld.manufacture_id[1] = data[0xB];
	new_data->quirks = tegra_edid_lookup_quirks(specs->manufacturer,
		specs->model, specs->monitor);

	extension_blocks = data[0x7e];

	/* verify only one extension block (to stay in bounds) */
	if (use_fallback && extension_blocks != 1) {
		pr_err("%s: fallback edid parsing failed\n", __func__);
		ret = -EINVAL;
		goto fail;
	}

	for (i = 1; i <= extension_blocks; i++) {
		if (edid->dc->vedid) {
			memcpy(data + i * EDID_BYTES_PER_BLOCK,
				edid->dc->vedid_data + i * EDID_BYTES_PER_BLOCK,
				EDID_BYTES_PER_BLOCK);
			for (j = 0; j < EDID_BYTES_PER_BLOCK; j++)
				checksum += data[i * EDID_BYTES_PER_BLOCK + j];
			if (checksum != 0) {
				pr_err("%s: checksum failed\n", __func__);
				ret = -EINVAL;
				goto fail;
			}
		} else if (use_fallback) {
			/* only one extension block, verified above */
			memcpy(data + i * EDID_BYTES_PER_BLOCK,
				default_720p_edid + i * EDID_BYTES_PER_BLOCK,
				EDID_BYTES_PER_BLOCK);
		} else {
			ret = tegra_edid_read_block(edid, i,
				data + i * EDID_BYTES_PER_BLOCK);
			if (ret < 0)
				goto fail;
		}

		if (new_data->quirks == TEGRA_EDID_QUIRK_VSVDB_LEN) {
			tegra_edid_quirk_vsvdb_len(data + i * EDID_BYTES_PER_BLOCK);
		}

		if (data[i * EDID_BYTES_PER_BLOCK] == 0x2) {
			fb_edid_add_monspecs(
				data + i * EDID_BYTES_PER_BLOCK,
				specs);
			tegra_edid_parse_ext_block(
				data + i * EDID_BYTES_PER_BLOCK,
				data[i * EDID_BYTES_PER_BLOCK + 2],
				new_data);

			if ((new_data->quirks & TEGRA_EDID_QUIRK_LG_SBAR) != 0)
				tegra_edid_quirk_lg_sbar(new_data, specs);

			if (new_data->support_stereo) {
				for (j = 0; j < specs->modedb_len; j++) {
					if (tegra_edid_mode_support_stereo(
						&specs->modedb[j]))
						specs->modedb[j].vmode |=
						FB_VMODE_STEREO_FRAME_PACK;
				}
			}
		} else if (data[i * EDID_BYTES_PER_BLOCK] == 0x70) {
			tegra_edid_disp_id_ext_block_parse(
				data + i * EDID_BYTES_PER_BLOCK, specs,
				new_data);
		}
	}

	if (specs->modedb == NULL) {
		pr_err("%s: EDID has no valid modes\n", __func__);
		ret = -EINVAL;
		goto fail;
	}

	/* T210 and T186 supports fractional divider and hence can support the * 1000 / 1001 modes.
	   For now, only enable support for 24, 30 and 60 Hz modes. */
	{
		const int max_modes = 50;
		struct fb_videomode *frac_modes, *m;
		int frac_n = 0;
		frac_modes = kzalloc(sizeof(struct fb_videomode) * max_modes,
				     GFP_KERNEL);

		if (frac_modes) {
			for (j = 0; j < specs->modedb_len; ++j) {
				int rate = tegra_dc_calc_fb_refresh(&specs->modedb[j]);
#if defined(CONFIG_FB_MODE_PIXCLOCK_HZ)
				u64 pixclock_hz = 0;
				u64 frac_pixclock_hz = 0;
#endif
				/*
				 * 1000/1001 modes are only supported on CEA
				 * SVDs or on HDMI EXT
				 * */
				bool supported = ((specs->modedb[j].vmode &
						   FB_VMODE_IS_CEA) &&
						  !(specs->modedb[j].vmode &
						    FB_VMODE_IS_DETAILED)) ||
						 specs->modedb[j].vmode &
						 FB_VMODE_IS_HDMI_EXT;
				if (supported && (rate == 24000 ||
				     rate == 30000 ||
				    (rate > (60000 - 20) && rate < (60000 + 20))) &&
				    frac_n < max_modes) {
					memcpy(&frac_modes[frac_n], &specs->modedb[j], sizeof(struct fb_videomode));
					frac_modes[frac_n].pixclock = frac_modes[frac_n].pixclock * 1001 / 1000;
					frac_modes[frac_n].vmode |= FB_VMODE_1000DIV1001;
#if defined(CONFIG_FB_MODE_PIXCLOCK_HZ)
					/* u64 to avoid overflow in pclk hz */
					pixclock_hz =
					    frac_modes[frac_n].pixclock_hz;
					frac_pixclock_hz =
					    pixclock_hz * 1000 / 1001;
					frac_modes[frac_n].pixclock_hz =
					    (u32)frac_pixclock_hz;
#endif
					frac_n++;
				}
			}

			if (frac_n == max_modes)
				pr_warn("Hit fractional mode limit %d!\n", frac_n);

			m = kzalloc((specs->modedb_len + frac_n) * sizeof(struct fb_videomode), GFP_KERNEL);
			if (m) {
				memcpy(m, specs->modedb, specs->modedb_len * sizeof(struct fb_videomode));
				memcpy(&m[specs->modedb_len], frac_modes, frac_n * sizeof(struct fb_videomode));
				kfree(specs->modedb);
				specs->modedb = m;
				specs->modedb_len += frac_n;
			}

			kfree(frac_modes);
		}
	}

	for (j = 0; j < specs->modedb_len; j++) {
		if (!new_data->rgb_quant_selectable &&
		    !(specs->modedb[j].vmode & FB_VMODE_SET_YUV_MASK))
			/*
			 * Follow HDMI 2.0 specification (section 7.3) to
			 * select color range.
			 */
			if (specs->modedb[j].vmode & FB_VMODE_IS_CEA &&
				!(specs->modedb[j].xres == 640 &&
				specs->modedb[j].yres == 480))
				specs->modedb[j].vmode |= FB_VMODE_LIMITED_RANGE;
		/* TODO: add color range selection for YUV mode. */
		if (!new_data->yuv_quant_selectable &&
		    (specs->modedb[j].vmode & FB_VMODE_SET_YUV_MASK))
			specs->modedb[j].vmode |= FB_VMODE_LIMITED_RANGE;
	}

	if (use_fallback)
		edid->errors |= EDID_ERRORS_USING_FALLBACK;

	new_data->dc_edid.len = i * EDID_BYTES_PER_BLOCK;

	mutex_lock(&edid->lock);
	old_data = edid->data;
	edid->data = new_data;
	mutex_unlock(&edid->lock);

	if (old_data)
		kref_put(&old_data->refcnt, data_release);

	tegra_edid_dump(edid);
	return 0;

fail:
	vfree(new_data);
	return ret;
}

int tegra_edid_audio_supported(struct tegra_edid *edid)
{
	if ((!edid) || (!edid->data))
		return 0;

	return edid->data->support_audio;
}

int tegra_edid_underscan_supported(struct tegra_edid *edid)
{
	if ((!edid) || (!edid->data))
		return 0;

	return edid->data->support_underscan;
}

int tegra_edid_get_eld(struct tegra_edid *edid, struct tegra_edid_hdmi_eld *elddata)
{
	if (!elddata || !edid || !edid->data)
		return -EFAULT;

	memcpy(elddata,&edid->data->eld,sizeof(struct tegra_edid_hdmi_eld));

	return 0;
}

int tegra_edid_get_source_physical_address(struct tegra_edid *edid, u8 *phy_address)
{
	if ((!phy_address) || (!edid)  || (!edid->data))
		return -EFAULT;

	phy_address[0] = edid->data->eld.port_id[0];
	phy_address[1] = edid->data->eld.port_id[1];

	return 0;
}

struct tegra_edid *tegra_edid_create(struct tegra_dc *dc,
	i2c_transfer_func_t i2c_func)
{
	struct tegra_edid *edid;

	edid = kzalloc(sizeof(struct tegra_edid), GFP_KERNEL);
	if (!edid)
		return ERR_PTR(-ENOMEM);

	mutex_init(&edid->lock);
	edid->i2c_ops.i2c_transfer = i2c_func;
	edid->dc = dc;

	return edid;
}

void tegra_edid_destroy(struct tegra_edid *edid)
{
	if (edid->data)
		kref_put(&edid->data->refcnt, data_release);
	kfree(edid);
}

struct tegra_dc_edid *tegra_edid_get_data(struct tegra_edid *edid)
{
	struct tegra_edid_pvt *data;

	mutex_lock(&edid->lock);
	data = edid->data;
	if (data)
		kref_get(&data->refcnt);
	mutex_unlock(&edid->lock);

	return data ? &data->dc_edid : NULL;
}

void tegra_edid_put_data(struct tegra_dc_edid *data)
{
	struct tegra_edid_pvt *pvt;

	if (!data)
		return;

	pvt = container_of(data, struct tegra_edid_pvt, dc_edid);

	kref_put(&pvt->refcnt, data_release);
}

int tegra_dc_edid_blob(struct tegra_dc *dc, struct i2c_msg *msgs, int num)
{
	struct i2c_msg *pmsg;
	int i;
	int status = 0;
	u32 len = 0;
	struct device_node *np_panel = NULL;

	np_panel = tegra_dc_get_panel_np(dc);
	if (!np_panel || !of_device_is_available(np_panel))
		return -ENOENT;

	for (i = 0; i < num; ++i) {
		pmsg = &msgs[i];

		if (pmsg->flags & I2C_M_RD) { /* Read */
			len = pmsg->len;
			status = of_property_read_u8_array(np_panel,
				"nvidia,edid", pmsg->buf, len);

			if (status) {
				dev_err(&dc->ndev->dev,
					"Failed to read EDID blob from DT"
					" addr:%d, size:%d\n",
					pmsg->addr, len);
				return status;
			}
		}
	}
	return i;
}

struct tegra_dc_edid *tegra_dc_get_edid(struct tegra_dc *dc)
{
	if (!dc || !dc->edid)
		return ERR_PTR(-ENODEV);

	return tegra_edid_get_data(dc->edid);
}
EXPORT_SYMBOL(tegra_dc_get_edid);

void tegra_dc_put_edid(struct tegra_dc_edid *edid)
{
	tegra_edid_put_data(edid);
}
EXPORT_SYMBOL(tegra_dc_put_edid);

static const struct i2c_device_id tegra_edid_id[] = {
        { "tegra_edid", 0 },
        { }
};

MODULE_DEVICE_TABLE(i2c, tegra_edid_id);

static struct i2c_driver tegra_edid_driver = {
        .id_table = tegra_edid_id,
        .driver = {
                .name = "tegra_edid",
        },
};

static int __init tegra_edid_init(void)
{
        return i2c_add_driver(&tegra_edid_driver);
}

static void __exit tegra_edid_exit(void)
{
        i2c_del_driver(&tegra_edid_driver);
}

module_init(tegra_edid_init);
module_exit(tegra_edid_exit);
