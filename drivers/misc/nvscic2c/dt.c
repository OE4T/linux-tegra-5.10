/*
 * Copyright (c) 2019, NVIDIA CORPORATION.	All rights reserved.
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

#include <linux/of_platform.h>
#include <linux/string.h>

#include "dt.h"

/*When module platform device not available.*/
#define TAG              "nvscic2c: dt     : "
#define C2C_ERROR(...)   pr_err(TAG __VA_ARGS__)
#define C2C_DEBUG(...)   pr_debug(TAG __VA_ARGS__)
#define C2C_INFO(...)    pr_info(TAG __VA_ARGS__)
#define C2C_ALERT(...)   pr_alert(TAG __VA_ARGS__)

/*
 * Macros for compatible strings, property names we expect to be
 * present and therefore look for in DT file.
 */
#define DT_NVSCIC2C_NODE_NAME              ("nvscic2c")
#define DT_NVSCIC2C_COMPATIBLE             ("nvidia,nvscic2c")

/* Global properties for root node.
 * applicable for each child node.
 */
#define DT_NVSCIC2C_TEGRA_HV               ("nvidia,tegra_hv")
#define DT_NVSCIC2C_CTRL_CHANNEL           ("nvidia,ctrl-channel")
#define DT_NVSCIC2C_APERTURE               ("nvidia,c2c-aperture")
#define DT_NVSCIC2C_MW_IVM                 ("nvidia,mw-ivm")
#define DT_NVSCIC2C_MW_CARVEOUT            ("nvidia,mw-carveout")

/* Child node initials.*/
#define DT_NVSCIC2C_CH_SUBNODE_NAME        ("channel_")

/* Channel specific properties.
 * Common to CPU/Bulk type channels.
 */
#define DT_NVSCIC2C_CH_ALIGN               ("align")
#define DT_NVSCIC2C_CH_CFG_NAME            ("channel-name")
#define DT_NVSCIC2C_CH_EVENT_TYPE          ("event-type")
#define DT_NVSCIC2C_CH_PCI_SHM_FRAMES      ("frames")

/* Bulk channel specific properties.*/
#define DT_NVSCIC2C_CH_BULK_XFER_PROP      ("bulk-xfer")
#define DT_NVSCIC2C_CH_EDMA_ENABLED        ("edma")

/* Strings to expect when properties is of string type.*/
#define DT_NVSCIC2C_CH_BULK_XFER_PROD_VAL  ("producer")
#define DT_NVSCIC2C_CH_BULK_XFER_CONS_VAL  ("consumer")
#define DT_NVSCIC2C_CH_EVENT_DOORBELL      ("doorbell")
#define DT_NVSCIC2C_CH_EVENT_SYNCPOINT     ("syncpoint")


/***************Internal functions for DT parsing sub-module****************/
static int read_ctrl_channel(struct dt_param *dt_param,
			struct device_node *node);

static int read_static_apt(struct dt_param *dt_param,
			struct device_node *node);

static int read_pcie_shared_mem(struct dt_param *dt_param,
			struct device_node *node);

static int read_mw_trans_ivm(struct dt_param *dt_param,
			struct device_node *node);

static int parse_channel_params(struct dt_param *dt_param,
			struct device_node *node);

static int read_channel_param(struct dt_param *dt_param,
			struct channel_dt_param *param,
			struct device_node *child);

static int channel_align_param(struct dt_param *dt_param,
			struct channel_dt_param *param,
			struct device_node *child);

static int channel_ivc_param(struct dt_param *dt_param,
			struct channel_dt_param *param,
			struct device_node *child);

static int channel_event_param(struct dt_param *dt_param,
			struct channel_dt_param *param,
			struct device_node *child);

static int channel_bulk_param(struct dt_param *dt_param,
			struct channel_dt_param *param,
			struct device_node *child);

static int channel_shm_frames(struct dt_param *dt_param,
			struct channel_dt_param *param,
			struct device_node *child);

static void print_parsed_dt(struct device *dev,
				struct dt_param *dt_param);

static void print_dt_chn_param(struct device *dev,
			struct channel_dt_param *param);

static char *get_event_type_name(enum event_type type);

static bool ispoweroftwo(uint32_t num);
/*************************************************************************/

/**
 * Internal variable used in DT sub-module.
 * points to root for nvscic2c node in DTB.
 */
static struct device_node *root;


/*
 * to free-up any memory and decrement ref_count of device nodes
 * allocated, accessed while parsing DT node of 'tegra_c2c'
 */
int dt_release(struct device *dev, struct dt_param *dt_param)
{
	int ret = 0;

	if (dt_param == NULL)
		goto exit;

	if (dt_param->hyp_dn != NULL) {
		of_node_put(dt_param->hyp_dn);
		dt_param->hyp_dn = NULL;
	}

	if (dt_param->chn_params != NULL) {
		devm_kfree(dev, dt_param->chn_params);
		dt_param->chn_params = NULL;
	}

exit:
	return ret;
}


/*
 * nvscic2c main context call this to parse nvscic2c DT node.
 *
 * It creates place-holder for all the valid c2c channel parameter
 * entries which would subsequently be used for c2c channel setup.
 *
 * All DT parsing is limited to this function.
 *
 * It doesn't populate the channels place-holder but only the channel
 * params place-holder with valid/enabled DT nodes.
 *
 * Initialises/obtains the phandles/dev nodes of platform devices that
 * we may need based on the feature requested like IVC.
 *
 * This function is split in multiple local function based on
 * DT properties.
 *
 * Use nvscic2c platform device to print parsed device node properties.
 */
int dt_parse(struct device *dev, struct dt_param *dt_param)
{
	int ret = 0;

	if ((dev == NULL)
		|| (dt_param == NULL)) {
		C2C_ERROR("(%s): Invalid Params\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	root = dev->of_node;
	dt_param->dev = dev;

	/*Initialize default values for validation.*/
	dt_param->ivm             = -1;
	dt_param->ivcq_id         = -1;
	dt_param->hyp_dn          = NULL;
	dt_param->nr_channels     = 0;
	dt_param->apt.present     = false;

	/* Logs are already printed inside functions.
	 * No need to duplicate logs.
	 */
	ret = read_ctrl_channel(dt_param, root);
	if (ret != 0)
		goto err;

	ret = read_static_apt(dt_param, root);
	if (ret != 0)
		goto err;

	ret = read_pcie_shared_mem(dt_param, root);
	if (ret != 0)
		goto err;

	ret = parse_channel_params(dt_param, root);
	if (ret != 0)
		goto err;

	print_parsed_dt(dev, dt_param);

	return ret;
err:
	dt_release(dev, dt_param);
	return ret;
}

/**
 * There is a control channel between NvSciC2C LKM and C2C server.
 * this channel is suppose to carry SDL messages.
 * This channel is exposed as NvSciIpc channel to C2C server.
 * We have this channel entry in nvscic2c root node.
 * Please make sure that this channel name is in sync with
 * channel name mentioned in nvsciipc.cfg/nvsciipc dt node.
 */
static int read_ctrl_channel(struct dt_param *dt_param,
				struct device_node *node)
{
	int ret = 0;

	/* obtain the platform devices for tegra_hv for ivc calls. */
	dt_param->hyp_dn = of_parse_phandle(node, DT_NVSCIC2C_TEGRA_HV, 0);
	if (dt_param->hyp_dn == NULL) {
		dev_err(dt_param->dev,
			"Property 'nvidia,tegra_hv' phandle parsing failed\n");
		ret = -EINVAL;
		return ret;
	}

	ret = of_property_read_u32_index(node, DT_NVSCIC2C_CTRL_CHANNEL, 1,
					(u32 *)&(dt_param->ivcq_id));
	if (ret != 0) {
		dev_err(dt_param->dev, "(%s) is missing (%s)",
			DT_NVSCIC2C_NODE_NAME, DT_NVSCIC2C_CTRL_CHANNEL);
		ret = -EINVAL;
		return ret;
	}

	return ret;
}

/**
 * nvscic2c_daemon running in C2C server is registered as
 * NTB client.
 * PCIe aperture for this NTB device is configured statically,
 * so that guest OS can map this directly.
 * PCIe aperture address is available in nvscic2c dt node
 * along with aperture size.
 */
static int read_static_apt(struct dt_param *dt_param,
				struct device_node *node)
{
	int ret = 0;
	uint32_t value = 0;

	ret = of_property_read_u32_index(node, DT_NVSCIC2C_APERTURE, 0,
							(u32 *)&value);
	if (ret == 0) {
		dt_param->apt.present   = true;
		dt_param->apt.base      = value;
		dt_param->apt.base    <<= 32;
		ret = of_property_read_u32_index(node, DT_NVSCIC2C_APERTURE, 1,
					(u32 *)&value);
		if (ret == 0) {
			dt_param->apt.base |= value;
			ret = of_property_read_u32_index(node,
						DT_NVSCIC2C_APERTURE, 2,
						(u32 *)&value);
			if (ret == 0) {
				dt_param->apt.size = value;
				ret = 0;
			}
		}
	}

	if (ret != 0) {
		dev_err(dt_param->dev, "(%s) Invalid/Missing in (%s)\n",
			DT_NVSCIC2C_APERTURE, DT_NVSCIC2C_NODE_NAME);
		ret = -ENOENT;
		return ret;
	}

	return ret;
}

/**
 * Read PCIe shared memory property available in root node.
 * First check if IVM is available.
 * If IVM is available then this would be used for PCIe shared memory
 * in C2C server.
 * If IVM is not available then carveout should be used.
 * Please note for this properties nvscic2c and nvscic2c_daemon nodes
 * should be in sync.
 * Currently in tegra linux we don't support carveout as PCIe shared memory.
 */
static int read_pcie_shared_mem(struct dt_param *dt_param,
				struct device_node *node)
{
	int ret = 0;

	ret = read_mw_trans_ivm(dt_param, node);
	if (ret != 0) {
		dev_err(dt_param->dev,  "No IVM supplied.\n");
		return ret;
	}

	return ret;
}

/**
 * Read IVM property in root node.
 * This IVM is supposed to be used for NTB translation in C2C server.
 * Guest will map this IVM as self memory and split this into
 * different supported C2C channel.
 */
static int read_mw_trans_ivm(struct dt_param *dt_param,
				struct device_node *node)
{
	int ret = 0;

	ret = of_property_read_u32_index(node, DT_NVSCIC2C_MW_IVM, 1,
				(u32 *)&(dt_param->ivm));
	if (ret != 0) {
		dev_err(dt_param->dev, "(%s) is missing (%s)",
			DT_NVSCIC2C_NODE_NAME, DT_NVSCIC2C_MW_IVM);
		ret = -EINVAL;
		return ret;
	}

	return ret;
}

/**
 * Parse all supported C2C channels.
 * Loop through max supported channel as we dont have dt field
 * to tell how many channels are there.
 * Read each channel, fail if no channel there or if more than supported
 * channels are there.
 * We will read partial channels if some channels have invalid/wrong values.
 * Will return with valid channels only.
 * In case more than supported channels are there we return as soon as we get
 * max number of supported channels.
 */
static int parse_channel_params(struct dt_param *dt_param,
				struct device_node *node)
{
	int ret = 0;
	int i = 0;
	int j = 0;
	size_t sz = 0x0;
	struct device_node *child = NULL;
	struct channel_dt_param *param = NULL;

	/*We expect parent to call post validation of params.*/
	/* allocate place holder for c2c channels. */
	sz = (sizeof(struct channel_dt_param) * (MAX_CHANNELS));
	dt_param->chn_params = devm_kzalloc(dt_param->dev, sz, GFP_KERNEL);
	if (dt_param->chn_params == NULL) {
		dev_err(dt_param->dev, "failed to alloc c2c channel params\n");
		ret = -ENOMEM;
		goto err;
	}

	for (i = 0; i < MAX_CHANNELS; i++) {
		param = &(dt_param->chn_params[j]);
		memset(param, 0x0, sizeof(struct channel_dt_param));

		param->edma_enabled     = false;
		param->event            = INVALID_EVENT;
		param->xfer_type        = XFER_TYPE_CPU;

		snprintf(param->ch_name, (MAX_NAME_SZ - 1), "%s%d",
				DT_NVSCIC2C_CH_SUBNODE_NAME, i);
		child = of_get_child_by_name(node, param->ch_name);
		if (child == NULL)
			continue;

		ret = read_channel_param(dt_param, param, child);
		if (ret != 0) {
			dev_err(dt_param->dev, "Reading channel params failed for: (%s)",
				param->ch_name);
			continue;
		}

		j++;
	}

	if (j == 0) {
		dev_err(dt_param->dev, "No c2c channel has valid parameter. Quitting");
		ret = -ENOENT;
		return ret;
	}

	/* j can't be more than MAX_CHANNELS.
	 * Hence no need to check for j > MAX_CHANNELS here.
	 */
	dt_param->nr_channels = j;

err:
	return ret;
}

/**
 * All supported C2C channels are available in DT file under nvscic2c DT node.
 * Parse each channel.
 * Fail for channel if any mandatory entry is missing or invalid.
 */
static int read_channel_param(struct dt_param *dt_param,
				struct channel_dt_param *param,
				struct device_node *child)
{
	int ret = 0;

	/*
	 * Read each filed in channel node. Return if fails for any entry.
	 * No log as child and parent both will print.
	 */
	ret = channel_align_param(dt_param, param, child);
	if (ret != 0) {
		dev_err(dt_param->dev, "channel_align_param failed.");
		return ret;
	}

	ret = channel_ivc_param(dt_param, param, child);
	if (ret != 0) {
		dev_err(dt_param->dev, "channel_ivc_param  failed.");
		return ret;
	}

	ret = channel_event_param(dt_param, param, child);
	if (ret != 0) {
		dev_err(dt_param->dev, "channel_event_param failed.");
		return ret;
	}

	ret = channel_bulk_param(dt_param, param, child);
	if (ret != 0) {
		dev_err(dt_param->dev, "channel_bulk_param failed.");
		return ret;
	}

	ret = channel_shm_frames(dt_param, param, child);
	if (ret != 0) {
		dev_err(dt_param->dev, "channel_shm_frames failed.");
		return ret;
	}

	return ret;
}

/**
 * C2C is supported in Tegra QNX and Tegra Linux with DAV config.
 * C2C data transfer is achieved by mapping send/recv area into user space.
 * In linux there is recommendation that mapped area should be page aligned.
 * These alignment needs may chane between different architecture/OS.
 * We carry one align field in each channel DT node.
 * Channel memory (send/recv) are aligned to this.
 * Please make sure that alignment value is power of 2.
 */
static int channel_align_param(struct dt_param *dt_param,
				struct channel_dt_param *param,
				struct device_node *child)
{
	int ret = 0;

	/* look for 'align' field for channel's tx and rx memory.*/
	ret = of_property_read_u32(child, DT_NVSCIC2C_CH_ALIGN,
				(u32 *)&(param->align));
	if (ret != 0) {
		dev_err(dt_param->dev,
			"Skipping c2c sub-node: (%s), dt-prop:(%s) missing.",
			param->ch_name, DT_NVSCIC2C_CH_ALIGN);
		return ret;
	} else if (!ispoweroftwo(param->align)) {
		dev_err(dt_param->dev, "align field is not power of 2 for (%s)",
			param->ch_name);
		ret = -EINVAL;
		return ret;
	}

	return ret;
}

/**
 * All NTB related operations are performed in C2C server.
 * When guest want to perform some NTB operations,
 * guest is suppose to send message to C2C server.
 * Each C2C channel has specific NvSciIPC channel(IVC) with
 * C2C server for this purpose.
 * Read IVC channel name.
 * Please note this property is mandatory for each C2C channel.
 * Also please make sure that this property is in sync with
 * channel name mentioned in nvsciipc.cfg file.
 */
static int channel_ivc_param(struct dt_param *dt_param,
				struct channel_dt_param *param,
				struct device_node *child)
{
	int ret = 0;
	const char *str;

	ret = of_property_read_string(child, DT_NVSCIC2C_CH_CFG_NAME, &str);
	if (ret != 0) {
		dev_err(dt_param->dev, "Skipping c2c sub-node: (%s), ivcq cfg name inval",
		   param->ch_name);
		ret = -ENOENT;
	} else {
		strncpy(param->cfg_name, str, (MAX_NAME_SZ - 1));
	}

	return ret;
}

/**
 * C2C channels can use Doorbell or Syncpoint for notification purpose.
 * Notification type to be used is supplies via DT node.
 * Please note currently we are only supporting Doorbell as event type.
 */
static int channel_event_param(struct dt_param *dt_param,
				struct channel_dt_param *param,
				struct device_node *child)
{
	int ret = 0;
	const char *str;

	ret = of_property_read_string(child, DT_NVSCIC2C_CH_EVENT_TYPE, &str);
	if (ret != 0) {
		dev_err(dt_param->dev,
			"Skipping c2c sub-node: (%s), event field missing.",
			param->ch_name);
		ret = -ENOENT;
	} else {
		if (!strncasecmp(str, DT_NVSCIC2C_CH_EVENT_DOORBELL,
				strlen(DT_NVSCIC2C_CH_EVENT_DOORBELL))) {
			param->event = DOORBELL;
		} else if (!strncasecmp(str, DT_NVSCIC2C_CH_EVENT_SYNCPOINT,
				strlen(DT_NVSCIC2C_CH_EVENT_SYNCPOINT))) {
			param->event = SYNCPOINT;
		} else {
			param->event = INVALID_EVENT;
			ret = -EINVAL;
		}
	}

	return ret;
}

/**
 * C2C channels are either CPU transfer channel or
 * Bulk transfer channel.
 * CPU transfer channels are bi-directional channels.
 * But bulk transfer channels are uni-directional channels.
 * Hence at a time one bulk channel can be either Producer or Consumer.
 * This property si supplied via DT entry.
 */
static int channel_bulk_param(struct dt_param *dt_param,
				struct channel_dt_param *param,
				struct device_node *child)
{
	int ret = 0;
	const char *str;

	/* check if channel is meant for bulk transfer.*/
	ret = of_property_read_string(child,
			DT_NVSCIC2C_CH_BULK_XFER_PROP, &str);
	if (ret == 0) {
		if (!strncasecmp(str, DT_NVSCIC2C_CH_BULK_XFER_PROD_VAL,
			strlen(DT_NVSCIC2C_CH_BULK_XFER_PROD_VAL))) {
			param->xfer_type = XFER_TYPE_BULK_PRODUCER;
		} else if (!strncasecmp(str, DT_NVSCIC2C_CH_BULK_XFER_CONS_VAL,
				strlen(DT_NVSCIC2C_CH_BULK_XFER_CONS_VAL))) {
			param->xfer_type = XFER_TYPE_BULK_CONSUMER;
		} else {
			dev_err(dt_param->dev, "(%s): skipping c2c sub-node: (%s), mode invalid",
				param->ch_name, DT_NVSCIC2C_CH_BULK_XFER_PROP);
			dev_err(dt_param->dev, "should be either '%s' or '%s'",
				DT_NVSCIC2C_CH_BULK_XFER_PROD_VAL,
				DT_NVSCIC2C_CH_BULK_XFER_CONS_VAL);
			ret = -EINVAL;
			return ret;
		}
	}

	/*Check if local edma to be used.*/
	if (param->xfer_type != XFER_TYPE_CPU) {
		ret = of_property_read_string(child,
					DT_NVSCIC2C_CH_EDMA_ENABLED, &str);
		if (ret == 0) {
			if (!strncasecmp(str, "true", strlen("true")))
				param->edma_enabled = true;
		}
	}

	ret = 0;

	return ret;
}

/**
 * Each supported C2C channel is given specific area within
 * PCIe shared memory and PCIe aperture.
 * Each channel area is divided between different size of frames.
 * This information is supplied in each channel specific DT node.
 * This contains number of frames along with frame_size.
 */
static int channel_shm_frames(struct dt_param *dt_param,
				struct channel_dt_param *param,
				struct device_node *child)
{
	int ret = 0;

	/* look for chunk property that pcie shared mem would be fragmented
	 * into for this channel. Frames and frame_sz: 2 properties.
	 */
	ret = of_property_read_u32_index(child,
				DT_NVSCIC2C_CH_PCI_SHM_FRAMES,
				0, &param->nframes);
	if (ret == 0) {
		ret = of_property_read_u32_index(child,
				DT_NVSCIC2C_CH_PCI_SHM_FRAMES,
				1, &param->frame_size);
	}

	if (ret != 0) {
		dev_err(dt_param->dev,
				"(%s): skipping c2c sub-node: (%s), values invalid",
				param->ch_name, DT_NVSCIC2C_CH_PCI_SHM_FRAMES);
		ret = -EINVAL;
		return ret;
	}

	return ret;
}

static void print_parsed_dt(struct device *dev, struct dt_param *dt_param)
{
	int i = 0;

	if (dt_param == NULL) {
		dev_err(dev, "Invalid dt param context.\n");
		return;
	}

	/* print the c2c channels parsed frm DT - only for debugging. */
	dev_dbg(dev, "\n");
	dev_dbg(dev, "DT node parsing leads to::::\n");

	dev_dbg(dev, "\tC2C ctrl channel ivcq id: (%d)", dt_param->ivcq_id);

	if (dt_param->apt.present) {
		dev_dbg(dev, "\tC2C Static Apt Details:");
		dev_dbg(dev, "\t\tbase address: (0x%08llx)",
			dt_param->apt.base);
		dev_dbg(dev, "\t\tsize        : (0x%08x)", dt_param->apt.size);
	}

	if (dt_param->ivm != -1) {
		dev_dbg(dev, "\tC2C ivm_id: (%d)", dt_param->ivm);
		dev_dbg(dev, "\tIVM is supplied, we will use IVM even if CO is present.");
	}

	dev_dbg(dev, "\tTotal c2c channels=%u", dt_param->nr_channels);
	for (i = 0; i < dt_param->nr_channels; i++) {
		struct channel_dt_param *param = NULL;

		param = &(dt_param->chn_params[i]);

		print_dt_chn_param(dev, param);
	}

	dev_dbg(dev, "\n");
}

/**
 * Print Channel specific properties.
 * Print channel control NvSciIpc channel name,
 * to be used with C2C server.
 * Print event type in use (Doorbell/SyncPoint).
 * Print if local EDMA engine is going to be used,
 * in case channel is bulk transfer channel.
 */
static void print_dt_chn_param(struct device *dev,
				struct channel_dt_param *param)
{
	dev_dbg(dev, "\t\t(%s)::", param->ch_name);
	dev_dbg(dev, "\t\t\tcfg-name   = (%s)", param->cfg_name);
	dev_dbg(dev, "\t\t\tnframes    = (%u) frame-size = (%u)",
		param->nframes, param->frame_size);

	if ((param->event >= INVALID_EVENT)
		 && (param->event < MAX_EVENT)) {
		dev_dbg(dev, "\t\t\tevent type = (%s)",
		   get_event_type_name(param->event));
	}

	if (param->xfer_type != XFER_TYPE_CPU) {
		dev_dbg(dev, "\t\t\tbulk_xfer, mode=(%s)",
			(param->xfer_type == (XFER_TYPE_BULK_PRODUCER) ?
			("producer") : ("consumer")));
		if (param->xfer_type == XFER_TYPE_BULK_CONSUMER) {
			if (param->edma_enabled) {
				dev_dbg(dev, "\t\t\tedma write channel will be used.");
			} else {
				/* Adding as debug in dt parsing.
				 */
				dev_dbg(dev, "\t\t\tRemote will use edma read.");
			}
		}

		if (param->xfer_type == XFER_TYPE_BULK_PRODUCER) {
			if (param->edma_enabled) {
				dev_dbg(dev, "\t\t\tedma read channel will be used.");
			} else {
				/* Adding as debug in dt parsing.
				 */
				dev_dbg(dev, "\t\t\tRemote will use edma write.");
			}
		}
	}
}

/**
 * Fetch Event type name(string) used for notification purpose.
 */
static char *get_event_type_name(enum event_type type)
{
	char *str = "";

	switch (type) {
	case DOORBELL:
		str = "Doorbell";
		break;
	case SYNCPOINT:
		str = "Syncpoint";
		break;
	default:
		str = "Invalid";
		break;
	}

	return str;
}

/**
 * Check if number is power of two or not.
 */
static bool ispoweroftwo(uint32_t num)
{
	return (num != 0) && ((num & (num - 1)) == 0);
}
