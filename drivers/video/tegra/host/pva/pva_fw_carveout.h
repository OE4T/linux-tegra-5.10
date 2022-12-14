/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * PVA carveout handling
 *
 * Copyright (c) 2022, NVIDIA Corporation.  All rights reserved.
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

#ifndef PVA_FW_CO_H
#define PVA_FW_CO_H

struct nvpva_carveout_info {
	dma_addr_t	base;
	dma_addr_t	base_pa;
	void		*base_va;
	size_t		size;
	bool		initialized;
};

struct nvpva_carveout_info *pva_fw_co_get_info(struct pva *pva);
bool pva_fw_co_initialized(struct pva *pva);

#endif
