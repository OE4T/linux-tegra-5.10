/*
 * Copyright (c) 2019, NVIDIA CORPORATION.	All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#ifndef __DT_H__
#define __DT_H__

#include "utils.h"

/**
 * Parse nvscic2c dt node inside this.
 * internally contains each supported C2C
 * channel along with PCIe memory details.
 */
struct dt_param {
	struct device *dev;
	int ivm;
	int ivcq_id;
	/* hyp device node for making ivc calls. */
	struct device_node *hyp_dn;
	char cfg_name[MAX_NAME_SZ];
	struct c2c_static_apt apt;

	/* total number of valid C2C channels.*/
	uint32_t nr_channels;

	 /* parameters of all the valid channels parsed from DT. */
	struct channel_dt_param *chn_params;
};

/*Parses nvscic2c dt node.*/
int
dt_parse(struct device *dev, struct dt_param *dt_param);

/*Release placeholders taken/allocated while parsing.*/
int
dt_release(struct device *dev, struct dt_param *dt_param);

#endif /*__DT_H__*/
