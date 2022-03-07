/*
 * Copyright (c) 2021-2022, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#ifndef TEGRA_HWPM_H
#define TEGRA_HWPM_H

#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <soc/tegra/fuse.h>

#include <uapi/linux/tegra-soc-hwpm-uapi.h>

#undef BIT
#define BIT(x) (0x1U << (u32)(x))

#undef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define TEGRA_SOC_HWPM_IP_INACTIVE	~(0U)

/* FIXME: Default timeout is 1 sec. Is this sufficient for pre-si? */
#define HWPM_TIMEOUT(timeout_check, expiry_msg) ({			\
	bool timeout_expired = false;					\
	s32 timeout_msecs = 1000;					\
	u32 sleep_msecs = 100;						\
	while (!(timeout_check)) {					\
		msleep(sleep_msecs);					\
		timeout_msecs -= sleep_msecs;				\
		if (timeout_msecs <= 0) {				\
			tegra_hwpm_err(NULL, "Timeout expired for %s!",	\
					expiry_msg);			\
			timeout_expired = true;				\
			break;						\
		}							\
	}								\
	timeout_expired;						\
})

struct hwpm_ip_register_list {
	struct tegra_soc_hwpm_ip_ops ip_ops;
	struct hwpm_ip_register_list *next;
};
extern struct hwpm_ip_register_list *ip_register_list_head;

/*
 * This structure is copy of struct tegra_soc_hwpm_ip_ops uapi structure.
 * This is not a hard requirement as tegra_hwpm_validate_ip_ops conversion
 * function.
 */
struct tegra_hwpm_ip_ops {
	/*
	 * Opaque ip device handle used for callback from
	 * SOC HWPM driver to IP drivers. This handle can be used
	 * to access IP driver functionality with the callbacks.
	 */
	void *ip_dev;
	/*
	 * hwpm_ip_pm is callback function to disable/enable
	 * IP driver power management. Before SOC HWPM doing
	 * perf measuremnts, this callback is called with
	 * "disable = true ", so that IP driver will disable IP specific
	 * power management to keep IP driver responsive. Once SOC HWPM is
	 * done with perf measurement, this callaback is called
	 * with "disable = false", so that IP driver can restore back
	 * it's orignal power management.
	 */
	int (*hwpm_ip_pm)(void *dev, bool disable);
	/*
	 * hwpm_ip_reg_op is callback function to do IP
	 * register 32 bit read or write.
	 * For read:
	 *      input : dev - IP device handle
	 *      input : reg_op - TEGRA_SOC_HWPM_IP_REG_OP_READ
	 *      input : reg_offset - register offset
	 *      output: reg_data - u32 read value
	 * For write:
	 *      input : dev - IP device handle
	 *      input : reg_op - TEGRA_SOC_HWPM_IP_REG_OP_WRITE
	 *      input : reg_offset - register offset
	 *      output: reg_data -  u32 write value
	 * Return:
	 *      reg_op success / failure
	 */
	int (*hwpm_ip_reg_op)(void *dev,
				enum tegra_soc_hwpm_ip_reg_op reg_op,
				__u64 reg_offset, __u32 *reg_data);
};

struct hwpm_ip_aperture {
	/*
	 * Indicates which domain (HWPM or IP) aperture belongs to,
	 * used for reverse mapping
	 */
	bool is_hwpm_element;

	/* HW index : This is used to update IP fs_mask */
	u32 hw_inst_mask;

	/* MMIO device tree aperture - only populated for perfmon */
	void __iomem *dt_mmio;

	/* DT tree name */
	char name[64];

	/* IP ops - only populated for perfmux */
	struct tegra_hwpm_ip_ops ip_ops;

	/* Allowlist */
	struct allowlist *alist;
	u64 alist_size;

	/* Physical aperture */
	u64 start_abs_pa;
	u64 end_abs_pa;

	/* MMIO aperture */
	u64 start_pa;
	u64 end_pa;

	/* Base address: used to calculate register offset */
	u64 base_pa;

	/* Fake registers for VDK which doesn't have a SOC HWPM fmodel */
	u32 *fake_registers;
};

typedef struct hwpm_ip_aperture hwpm_ip_perfmon;
typedef struct hwpm_ip_aperture hwpm_ip_perfmux;

struct hwpm_ip {
	/* Number of instances */
	u32 num_instances;

	/* Number of perfmons per instance */
	u32 num_perfmon_per_inst;

	/* Number of perfmuxes per instance */
	u32 num_perfmux_per_inst;

	/* IP perfmon address range */
	u64 perfmon_range_start;
	u64 perfmon_range_end;

	/* Perfmon physical address stride for each IP instance */
	u64 inst_perfmon_stride;

	/*
	 * Perfmon slots that can fit into perfmon address range.
	 * This gives number of indices in ip_perfmon
	 */
	u32 num_perfmon_slots;

	/* IP perfmon array */
	hwpm_ip_perfmon **ip_perfmon;

	/* IP perfmux address range */
	u64 perfmux_range_start;
	u64 perfmux_range_end;

	/* Perfmux physical address stride for each IP instance */
	u64 inst_perfmux_stride;

	/*
	 * Perfmux slots that can fit into perfmux address range.
	 * This gives number of indices in ip_perfmux
	 */
	u32 num_perfmux_slots;

	/* IP perfmux array */
	hwpm_ip_perfmux **ip_perfmux;

	/* Override IP config based on fuse value */
	bool override_enable;

	/*
	 * IP floorsweep info based on hw index of aperture
	 * NOTE: This mask needs to based on hw instance index because
	 * hwpm driver clients use hw instance index to find aperture
	 * info (start/end address) from hw manual.
	 */
	u32 fs_mask;

	/* IP perfmon array */
	hwpm_ip_perfmon *perfmon_static_array;

	/* IP perfmux array */
	hwpm_ip_perfmux *perfmux_static_array;

	bool reserved;
};

struct tegra_soc_hwpm;

struct tegra_soc_hwpm_chip {
	/* Array of pointers to active IP structures */
	struct hwpm_ip **chip_ips;

	/* Chip HALs */
	bool (*is_ip_active)(struct tegra_soc_hwpm *hwpm,
	u32 ip_index, u32 *config_ip_index);
	bool (*is_resource_active)(struct tegra_soc_hwpm *hwpm,
	u32 res_index, u32 *config_ip_index);

	u32 (*get_pma_int_idx)(struct tegra_soc_hwpm *hwpm);
	u32 (*get_rtr_int_idx)(struct tegra_soc_hwpm *hwpm);
	u32 (*get_ip_max_idx)(struct tegra_soc_hwpm *hwpm);

	int (*init_chip_ip_structures)(struct tegra_soc_hwpm *hwpm);

	int (*extract_ip_ops)(struct tegra_soc_hwpm *hwpm,
	struct tegra_soc_hwpm_ip_ops *hwpm_ip_ops, bool available);
	int (*force_enable_ips)(struct tegra_soc_hwpm *hwpm);
	int (*get_fs_info)(struct tegra_soc_hwpm *hwpm,
	u32 ip_index, u64 *fs_mask, u8 *ip_status);

	int (*init_prod_values)(struct tegra_soc_hwpm *hwpm);
	int (*disable_slcg)(struct tegra_soc_hwpm *hwpm);
	int (*enable_slcg)(struct tegra_soc_hwpm *hwpm);

	int (*reserve_pma)(struct tegra_soc_hwpm *hwpm);
	int (*reserve_rtr)(struct tegra_soc_hwpm *hwpm);
	int (*release_pma)(struct tegra_soc_hwpm *hwpm);
	int (*release_rtr)(struct tegra_soc_hwpm *hwpm);

	int (*disable_triggers)(struct tegra_soc_hwpm *hwpm);
	int (*perfmon_enable)(struct tegra_soc_hwpm *hwpm,
	hwpm_ip_perfmon *perfmon);
	int (*perfmon_disable)(struct tegra_soc_hwpm *hwpm,
		hwpm_ip_perfmon *perfmon);
	int (*perfmux_disable)(struct tegra_soc_hwpm *hwpm,
		hwpm_ip_perfmux *perfmux);

	int (*disable_mem_mgmt)(struct tegra_soc_hwpm *hwpm);
	int (*enable_mem_mgmt)(struct tegra_soc_hwpm *hwpm,
		struct tegra_soc_hwpm_alloc_pma_stream *alloc_pma_stream);
	int (*invalidate_mem_config)(struct tegra_soc_hwpm *hwpm);
	int (*stream_mem_bytes)(struct tegra_soc_hwpm *hwpm);
	int (*disable_pma_streaming)(struct tegra_soc_hwpm *hwpm);
	int (*update_mem_bytes_get_ptr)(struct tegra_soc_hwpm *hwpm,
		u64 mem_bump);
	u64 (*get_mem_bytes_put_ptr)(struct tegra_soc_hwpm *hwpm);
	bool (*membuf_overflow_status)(struct tegra_soc_hwpm *hwpm);

	size_t (*get_alist_buf_size)(struct tegra_soc_hwpm *hwpm);
	int (*zero_alist_regs)(struct tegra_soc_hwpm *hwpm,
		struct hwpm_ip_aperture *aperture);
	int (*copy_alist)(struct tegra_soc_hwpm *hwpm,
		struct hwpm_ip_aperture *aperture,
		u64 *full_alist,
		u64 *full_alist_idx);
	bool (*check_alist)(struct tegra_soc_hwpm *hwpm,
		struct hwpm_ip_aperture *aperture, u64 phys_addr);

	int (*exec_reg_ops)(struct tegra_soc_hwpm *hwpm,
		struct tegra_soc_hwpm_reg_op *reg_op);

	void (*release_sw_setup)(struct tegra_soc_hwpm *hwpm);
};

struct allowlist;
extern struct platform_device *tegra_soc_hwpm_pdev;
extern const struct file_operations tegra_soc_hwpm_ops;

/* Driver struct */
struct tegra_soc_hwpm {
	/* Device */
	struct platform_device *pdev;
	struct device *dev;
	struct device_node *np;
	struct class class;
	dev_t dev_t;
	struct cdev cdev;

	/* Device info */
	struct tegra_soc_hwpm_device_info device_info;

	/* Active chip info */
	struct tegra_soc_hwpm_chip *active_chip;

	/* Clocks and resets */
	struct clk *la_clk;
	struct clk *la_parent_clk;
	struct reset_control *la_rst;
	struct reset_control *hwpm_rst;

	/* Memory Management */
	struct dma_buf *stream_dma_buf;
	struct dma_buf_attachment *stream_attach;
	struct sg_table *stream_sgt;
	struct dma_buf *mem_bytes_dma_buf;
	struct dma_buf_attachment *mem_bytes_attach;
	struct sg_table *mem_bytes_sgt;
	void *mem_bytes_kernel;

	/* SW State */
	bool bind_completed;
	bool device_opened;
	u64 full_alist_size;

	atomic_t hwpm_in_use;

	u32 dbg_mask;

	/* Debugging */
#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs_root;
#endif
	bool fake_registers_enabled;
};

#endif /* TEGRA_HWPM_H */
