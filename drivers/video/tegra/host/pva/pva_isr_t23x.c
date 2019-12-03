/*
 * PVA ISR code for T23X
 *
 * Copyright (c) 2019, NVIDIA Corporation.  All rights reserved.
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

#include "pva_isr_t23x.h"

#include <linux/irq.h>
#include <linux/nvhost.h>

#include "pva_regs.h"
#include "dev.h"
#include "pva.h"

irqreturn_t pva_ccq_isr(int irq, void *dev_id)
{
	/* This ISR is a placeholder which just clears any pending interrupts */
	uint32_t clear_int = (0x1U << 20) | (0x1U << 24) | (0x1U << 28);
	uint32_t int_status;
	int i;
	struct pva *pva = dev_id;
	struct platform_device *pdev = pva->pdev;

	nvhost_dbg_info("Received ISR from CCQ block, IRQ: %d", irq);
	for (i = 0; i < MAX_PVA_QUEUE_COUNT; i++) {
		int_status = host1x_readl(pdev,
					  cfg_ccq_status_r(pva->version, i, 2))
					  & ~0xff;
		if (int_status != 0x0) {
			host1x_writel(pdev,
				      cfg_ccq_status_r(pva->version, i, 2),
				      clear_int);
		}
	}
	return IRQ_HANDLED;
}