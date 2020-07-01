/*
 * Copyright (C) 2020 NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ARM_SMMU_DEBUG_H
#define _ARM_SMMU_DEBUG_H

#include "arm-smmu.h"

/* Maximum number of context banks per SMMU */
#define ARM_SMMU_MAX_CBS		128

struct smmu_debugfs_info {
	struct device	*dev;
	DECLARE_BITMAP(context_filter, ARM_SMMU_MAX_CBS);
	struct dentry	*debugfs_root;
	int		num_context_banks;
	int		max_cbs;
};

void arm_smmu_debugfs_setup(struct arm_smmu_device *smmu);

#endif /* _ARM_SMMU_DEBUG_H */
