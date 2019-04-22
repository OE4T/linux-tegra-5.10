/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#ifndef __TEGRA_CBBERR_H
#define __TEGRA_CBBERR_H

#define CBB_BIT(_bit_) (1ULL << (_bit_))
#define CBB_MASK(_msb_, _lsb_) \
	((CBB_BIT(_msb_+1) - 1) & ~(CBB_BIT(_lsb_) - 1))
#define CBB_EXTRACT(_x_, _msb_, _lsb_)  \
	((_x_ & CBB_MASK(_msb_, _lsb_)) >> _lsb_)

#define get_cbb_err_subfield(_x_, _msb_, _lsb_) \
	CBB_EXTRACT(_x_, _msb_, _lsb_)

#define DMAAPB_X_RAW_INTERRUPT_STATUS   0x2ec


extern unsigned int tegra_axi2apb_errstatus(void __iomem *addr);


extern void print_cbb_err(struct seq_file *file, const char *fmt, ...);
extern void print_cache(struct seq_file *file, u32 cache);
extern void print_prot(struct seq_file *file, u32 prot);

extern void tegra_axi2apb_err(struct seq_file *file,
				int bridge, u32 bus_status);

extern int tegra_cbb_err_getirq(struct platform_device *pdev,
				int *nonsecure_irq,
				int *secure_irq, int *num_intr);


struct tegra_noc_errors {
	char *errcode;
	char *src;
	char *type;
};

struct tegra_noc_packet_header {
	bool lock;   // [0]
	u8   opc;    // [4:1]
	u8   errcode;// [10:8]= RD, RDW, RDL, RDX, WR, WRW, WRC, PRE, URG
	u16  len1;   // [27:16]
	bool format; // [31]  = 1 -> FlexNoC versions 2.7 & above
};

struct tegra_lookup_noc_aperture {
	u8  initflow;
	u8  targflow;
	u8  targ_subrange;
	u8  init_mapping;
	u32 init_localaddress;
	u8  targ_mapping;
	u32 targ_localaddress;
	u16 seqid;
};

struct tegra_noc_userbits {
	u8  axcache;
	u8  non_mod;
	u8  axprot;
	u8  falconsec;
	u8  grpsec;
	u8  vqc;
	u8  mstr_id;
	u8  axi_id;
};

struct tegra_cbb_noc_data {
	char *name;
	int  max_error;
	char **tegra_cbb_master_id;
	bool is_ax2apb_bridge_connected;
	void __iomem    **axi2abp_bases;
	int  apb_bridge_cnt;
	bool is_clk_rst;
	int  (*is_cluster_probed)(void);
	int  (*is_clk_enabled)(void);
	int  (*tegra_noc_en_clk_rpm)(void);
	int  (*tegra_noc_dis_clk_rpm)(void);
	int  (*tegra_noc_en_clk_no_rpm)(void);
	int  (*tegra_noc_dis_clk_no_rpm)(void);
};

struct tegra_cbb_init_data {
	struct resource *res_base;
	int secure_irq;
	int nonsecure_irq;
	void __iomem *vaddr;
	int num;
};

struct tegra_cbberr_ops {
	/*
	 * Show details of failed transaction. This is called from a debugfs
	 * context - that means you can sleep and do general kernel stuff here.
	 */
	int (*cbb_err_debugfs_show)(struct seq_file *s, void *v);
	void (*cbb_error_enable)(void __iomem *vaddr);
	int (*cbb_enable_interrupt)(struct platform_device *pdev,
                                    int noc_secure_irq, int noc_nonsecure_irq);

	unsigned int (*errvld)(void __iomem *addr);
	void (*errclr)(void __iomem *addr);
	void (*faulten)(void __iomem *addr);
	void (*stallen)(void __iomem *addr);
};

struct tegra_cbb_errlog_record {
	struct list_head node;
	struct serr_hook *callback;
	char *name;
	phys_addr_t start;
	void __iomem *vaddr;
	int num_intr;
	int noc_secure_irq;
	int noc_nonsecure_irq;
	u32 errlog0;
	u32 errlog1;
	u32 errlog2;
	u32 errlog3;
	u32 errlog4;
	u32 errlog5;
	u32 errlog6;        //RESERVED
	u32 errlog7;        //RESERVED
	u32 errlog8;        //RESERVED
	void (*tegra_noc_parse_routeid)
	             (struct tegra_lookup_noc_aperture *, u64);
	void (*tegra_noc_parse_userbits) (struct tegra_noc_userbits *, u64);
	struct tegra_lookup_noc_aperture *noc_aperture;
	int  max_noc_aperture;
	char **tegra_noc_routeid_initflow;
	char **tegra_noc_routeid_targflow;
	char **tegra_cbb_master_id;
	bool is_ax2apb_bridge_connected;
	void __iomem **axi2abp_bases;
	int  apb_bridge_cnt;
	bool is_clk_rst;
	int (*is_cluster_probed)(void);
	int (*is_clk_enabled)(void);
	int (*tegra_noc_en_clk_rpm)(void);
	int (*tegra_noc_dis_clk_rpm)(void);
	int (*tegra_noc_en_clk_no_rpm)(void);
	int (*tegra_noc_dis_clk_no_rpm)(void);
};

extern int nvcvnas_busy(void);
extern int nvcvnas_idle(void);
extern int is_nvcvnas_probed(void);
extern int nvcvnas_busy_no_rpm(void);
extern int nvcvnas_idle_no_rpm(void);
extern int is_nvcvnas_clk_enabled(void);

void tegra_cbb_stallen(void __iomem *addr);
void tegra_cbb_faulten(void __iomem *addr);
void tegra_cbb_errclr(void __iomem *addr);
unsigned int tegra_cbb_errvld(void __iomem *addr);

void tegra_cbberr_set_ops(struct tegra_cbberr_ops *tegra_cbb_err_ops);

int tegra_cbberr_register_hook_en(struct platform_device *pdev,
		const struct tegra_cbb_noc_data *bdata,
		struct serr_hook *callback,
		struct tegra_cbb_init_data cbb_init_data);

int tegra_cbb_core_probed(void);

#endif /* __TEGRA_CBBERR_H */
