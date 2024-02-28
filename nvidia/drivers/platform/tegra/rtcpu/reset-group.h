/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2017-2022, NVIDIA CORPORATION.  All rights reserved.
 */

#ifndef INCLUDE_RESET_GROUP_H
#define INCLUDE_RESET_GROUP_H

struct device;
struct camrtc_reset_group;

struct camrtc_reset_group *camrtc_reset_group_get(
	struct device *dev,
	const char *group_name);

void camrtc_reset_group_assert(const struct camrtc_reset_group *grp);
int camrtc_reset_group_deassert(const struct camrtc_reset_group *grp);

#endif /* INCLUDE_RESET_GROUP_H */
