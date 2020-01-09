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

#ifndef __PVA_INTERFACE_REGS_T23X_H__
#define __PVA_INTERFACE_REGS_T23X_H__

#include "pva/pva.h"

#define NUM_INTERFACES_T23X     1

#define PVA_EMPTY_STATUS_REG    0

#define PVA_MBOX_STATUS4_REG	0x178000
#define PVA_MBOX_STATUS5_REG	0x180000
#define PVA_MBOX_STATUS6_REG	0x188000
#define PVA_MBOX_STATUS7_REG	0x190000

void read_status_interface_t23x(struct pva *pva,
				uint32_t interface_id, u32 isr_status,
				struct pva_mailbox_status_regs *status_output);

#endif
