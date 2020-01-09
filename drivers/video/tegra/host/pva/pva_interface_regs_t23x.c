/*
 * Copyright (c) 2016-2019, NVIDIA Corporation. All rights reserved.
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

#include <linux/export.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <soc/tegra/chip-id.h>
#include <linux/platform_device.h>

#include "nvhost_acm.h"
#include "dev.h"
#include "pva/pva.h"
#include "pva/pva_mailbox.h"
#include "pva_interface_regs_t23x.h"

static struct pva_status_interface_registers t23x_status_regs[NUM_INTERFACES_T23X] = {
	{
		{
			PVA_EMPTY_STATUS_REG,
			PVA_MBOX_STATUS4_REG,
			PVA_MBOX_STATUS5_REG,
			PVA_MBOX_STATUS6_REG,
			PVA_MBOX_STATUS7_REG
		}
	},
};

void read_status_interface_t23x(struct pva *pva,
				uint32_t interface_id, u32 isr_status,
				struct pva_mailbox_status_regs *status_output)
{
	int i;
	u32 valid_status = PVA_VALID_STATUS3;
	uint32_t *status_registers;
	status_registers = t23x_status_regs[interface_id].registers;
	if (isr_status && PVA_VALID_STATUS3) {
		status_output->status[0] = PVA_GET_ERROR_CODE(isr_status);
	}
	if (isr_status && PVA_CMD_ERROR) {
		status_output->error = PVA_GET_ERROR_CODE(isr_status);
	}
	for (i = 1; i < NUM_STATUS_REGS; i++) {
		valid_status = valid_status << 1;
		if (isr_status & valid_status) {
			status_output->status[i + PVA_CCQ_STATUS3_INDEX] =
							host1x_readl(pva->pdev,
							status_registers[i]);
		}
	}
}
