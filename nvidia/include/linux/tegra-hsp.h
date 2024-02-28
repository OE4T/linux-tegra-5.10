/*
 * Copyright (c) 2014-2018, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef _LINUX_TEGRA_HSP_H
#define _LINUX_TEGRA_HSP_H

#include <linux/types.h>

enum tegra_hsp_master {
	HSP_FIRST_MASTER = 1,

	/* secure */
	HSP_MASTER_SECURE_CCPLEX = HSP_FIRST_MASTER,
	HSP_MASTER_SECURE_DPMU,
	HSP_MASTER_SECURE_BPMP,
	HSP_MASTER_SECURE_SPE,
	HSP_MASTER_SECURE_SCE,
	HSP_MASTER_SECURE_DMA,
	HSP_MASTER_SECURE_TSECA,
	HSP_MASTER_SECURE_TSECB,
	HSP_MASTER_SECURE_JTAGM,
	HSP_MASTER_SECURE_CSITE,
	HSP_MASTER_SECURE_APE,

	/* non-secure */
	HSP_MASTER_CCPLEX = HSP_FIRST_MASTER + 16,
	HSP_MASTER_DPMU,
	HSP_MASTER_BPMP,
	HSP_MASTER_SPE,
	HSP_MASTER_SCE,
	HSP_MASTER_DMA,
	HSP_MASTER_TSECA,
	HSP_MASTER_TSECB,
	HSP_MASTER_JTAGM,
	HSP_MASTER_CSITE,
	HSP_MASTER_APE,

	HSP_LAST_MASTER = HSP_MASTER_APE,
};

enum tegra_hsp_doorbell {
	HSP_FIRST_DB = 0,
	HSP_DB_DPMU = HSP_FIRST_DB,
	HSP_DB_CCPLEX,
	HSP_DB_CCPLEX_TZ,
	HSP_DB_BPMP,
	HSP_DB_SPE,
	HSP_DB_SCE,
	HSP_DB_APE,
	HSP_LAST_DB = HSP_DB_APE,
	HSP_NR_DBS,
};

typedef void (*db_handler_t)(void *data);

int tegra_hsp_init(void);

int tegra_hsp_db_enable_master(enum tegra_hsp_master master);

int tegra_hsp_db_disable_master(enum tegra_hsp_master master);

int tegra_hsp_db_ring(enum tegra_hsp_doorbell dbell);

int tegra_hsp_db_can_ring(enum tegra_hsp_doorbell dbell);

int tegra_hsp_db_add_handler(int master, db_handler_t handler, void *data);

int tegra_hsp_db_del_handler(int master);

#define tegra_hsp_find_master(mask, master)	((mask) & (1 << (master)))

typedef void (*tegra_hsp_sm_notify)(void *, u32);

struct tegra_hsp_sm_tx;
struct tegra_hsp_sm_rx;
struct tegra_hsp_ss;

struct tegra_hsp_sm_pair {
	struct tegra_hsp_sm_rx *rx;
	struct tegra_hsp_sm_tx *tx;
};

struct tegra_hsp_sm_pair *of_tegra_hsp_sm_pair_request(
	const struct device_node *np, u32 index,
	tegra_hsp_sm_notify full_notify,
	tegra_hsp_sm_notify empty_notify,
	void *data);
struct tegra_hsp_sm_pair *of_tegra_hsp_sm_pair_by_name(
	struct device_node *np, char const *name,
	tegra_hsp_sm_notify full_notify,
	tegra_hsp_sm_notify empty_notify,
	void *data);
void tegra_hsp_sm_pair_free(struct tegra_hsp_sm_pair *pair);
void tegra_hsp_sm_pair_write(const struct tegra_hsp_sm_pair *pair, u32 value);
bool tegra_hsp_sm_pair_is_empty(const struct tegra_hsp_sm_pair *pair);
void tegra_hsp_sm_pair_enable_empty_notify(struct tegra_hsp_sm_pair *pair);

struct tegra_hsp_sm_tx *of_tegra_hsp_sm_tx_by_name(
	struct device_node *np, char const *name,
	tegra_hsp_sm_notify notify, void *data);
void tegra_hsp_sm_tx_free(struct tegra_hsp_sm_tx *tx);
bool tegra_hsp_sm_tx_is_empty(const struct tegra_hsp_sm_tx *tx);
void tegra_hsp_sm_tx_write(const struct tegra_hsp_sm_tx *tx, u32 value);
void tegra_hsp_sm_tx_enable_notify(struct tegra_hsp_sm_tx *tx);

struct tegra_hsp_sm_rx *of_tegra_hsp_sm_rx_by_name(
	struct device_node *np, char const *name,
	tegra_hsp_sm_notify notify, void *data);
void tegra_hsp_sm_rx_free(struct tegra_hsp_sm_rx *rx);
bool tegra_hsp_sm_rx_is_empty(const struct tegra_hsp_sm_rx *rx);

struct tegra_hsp_ss *of_tegra_hsp_ss_by_name(
	struct device_node *np, char const *name);
void tegra_hsp_ss_free(struct tegra_hsp_ss *ss);
u32 tegra_hsp_ss_status(const struct tegra_hsp_ss *ss);
void tegra_hsp_ss_set(const struct tegra_hsp_ss *ss, u32 bits);
void tegra_hsp_ss_clr(const struct tegra_hsp_ss *ss, u32 bits);

#endif
