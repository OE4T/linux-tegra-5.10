/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2018-2020 NVIDIA Corporation. All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef _OF_TEGRA_SMMU_H_
#define _OF_TEGRA_SMMU_H_

/*
 * These are functions that are weakly defined in this
 * directory and then overridden elsewhere
 */
struct iommu_group *tegra_smmu_of_get_group(struct device *dev);
void tegra_smmu_remove_iommu_groups(void);

#endif
