/*
 * Copyright (c) 2016-2019, NVIDIA Corporation.  All rights reserved.
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

#include "pva_mailbox_t19x.h"
#include "pva_interface_regs_t19x.h"
#include "pva_version_config_t19x.h"

struct pva_version_config pva_t19x_config = {
	.read_mailbox = pva_read_mailbox_t19x,
	.write_mailbox = pva_write_mailbox_t19x,
	.read_status_interface = read_status_interface_t19x,
	.irq_count = 1,
};
