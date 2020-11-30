/*
 * Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef INCLUDED_MACSEC_H
#define INCLUDED_MACSEC_H

#include <osi_macsec.h>
#include <linux/random.h>
#include <net/genetlink.h>
#include <linux/crypto.h>

//#define TEST 1

/**
 * @brief Size of Macsec IRQ name.
 */
#define MACSEC_IRQ_NAME_SZ		32

//TODO - include name of driver interface as well
#define NV_MACSEC_GENL_NAME	"nv_macsec"
#define NV_MACSEC_GENL_VERSION	1

enum nv_macsec_sa_attrs {
	NV_MACSEC_SA_ATTR_UNSPEC,
	NV_MACSEC_SA_ATTR_SCI,
	NV_MACSEC_SA_ATTR_AN,
	NV_MACSEC_SA_ATTR_PN,
	NV_MACSEC_SA_ATTR_KEY,
	__NV_MACSEC_SA_ATTR_END,
	NUM_NV_MACSEC_SA_ATTR = __NV_MACSEC_SA_ATTR_END,
	NV_MACSEC_SA_ATTR_MAX = __NV_MACSEC_SA_ATTR_END - 1,
};

enum nv_macsec_attrs {
	NV_MACSEC_ATTR_UNSPEC,
	NV_MACSEC_ATTR_IFNAME,
	NV_MACSEC_ATTR_TXSC_PORT,
	NV_MACSEC_ATTR_PROT_FRAMES_EN,
	NV_MACSEC_ATTR_REPLAY_PROT_EN,
	NV_MACSEC_ATTR_REPLAY_WINDOW,
	NV_MACSEC_ATTR_CIPHER_SUITE,
	NV_MACSEC_ATTR_CTRL_PORT_EN,
	NV_MACSEC_ATTR_SA_CONFIG, /* Nested SA config */
	__NV_MACSEC_ATTR_END,
	NUM_NV_MACSEC_ATTR = __NV_MACSEC_ATTR_END,
	NV_MACSEC_ATTR_MAX = __NV_MACSEC_ATTR_END - 1,
};

static const struct nla_policy nv_macsec_sa_genl_policy[NUM_NV_MACSEC_SA_ATTR] = {
	[NV_MACSEC_SA_ATTR_SCI] = { .type = NLA_BINARY,
				    .len = 8, }, /* SCI is 64bit */
	[NV_MACSEC_SA_ATTR_AN] = { .type = NLA_U8 },
	[NV_MACSEC_SA_ATTR_PN] = { .type = NLA_U32 },
	[NV_MACSEC_SA_ATTR_KEY] = { .type = NLA_BINARY,
				    .len = KEY_LEN_128,},
};

static const struct nla_policy nv_macsec_genl_policy[NUM_NV_MACSEC_ATTR] = {
	[NV_MACSEC_ATTR_IFNAME] = { .type = NLA_STRING },
	[NV_MACSEC_ATTR_TXSC_PORT] = { .type = NLA_U16 },
	[NV_MACSEC_ATTR_REPLAY_PROT_EN] = { .type = NLA_U32 },
	[NV_MACSEC_ATTR_REPLAY_WINDOW] = { .type = NLA_U32 },
	[NV_MACSEC_ATTR_SA_CONFIG] = { .type = NLA_NESTED },
};

enum nv_macsec_nl_commands {
	NV_MACSEC_CMD_INIT,
	NV_MACSEC_CMD_GET_TX_NEXT_PN,
	NV_MACSEC_CMD_SET_PROT_FRAMES,
	NV_MACSEC_CMD_SET_REPLAY_PROT,
	NV_MACSEC_CMD_SET_CIPHER,
	NV_MACSEC_CMD_SET_CONTROLLED_PORT,
	NV_MACSEC_CMD_EN_TX_SA,
	NV_MACSEC_CMD_DIS_TX_SA,
	NV_MACSEC_CMD_EN_RX_SA,
	NV_MACSEC_CMD_DIS_RX_SA,
	NV_MACSEC_CMD_DEINIT,
};

/**
 * @brief MACsec private data structure
 */
struct macsec_priv_data {
	/** Non secure reset */
	struct reset_control *ns_rst;
	/** APB interface clk - macsec_pclk */
	struct clk *apb_pclk;
	/** macsec Tx clock */
	struct clk *tx_clk;
	/** macsec Rx clock */
	struct clk *rx_clk;
	/** Secure irq */
	int s_irq;
	/** Non secure irq */
	int ns_irq;
	/** pointer to ether private data struct */
	struct ether_priv_data *ether_pdata;
	/** macsec IRQ name strings */
	char irq_name[2][MACSEC_IRQ_NAME_SZ];
	/** loopback mode */
	unsigned int loopback_mode;
	/** MACsec protect frames variable */
	unsigned int protect_frames;
	/** MACsec enabled flags for Tx/Rx controller status */
	unsigned int enabled;
	/** MACsec controller initialized flag */
	unsigned int init_done;
};

int macsec_probe(struct ether_priv_data *pdata);
void macsec_remove(struct ether_priv_data *pdata);
int macsec_open(struct macsec_priv_data *macsec_pdata);
int macsec_close(struct macsec_priv_data *macsec_pdata);
#ifdef TEST
int macsec_genl_register(void);
void macsec_genl_unregister(void);
#endif /* TEST */

#ifdef MACSEC_DEBUG
#define PRINT_ENTRY()	(pr_info("-->%s()\n", __func__))
#define PRINT_EXIT()	(pr_info("<--%s()\n", __func__))
#else
#define PRINT_ENTRY()
#define PRINT_EXIT()
#endif /* MACSEC_DEBUG */

#endif /* INCLUDED_MACSEC_H */

