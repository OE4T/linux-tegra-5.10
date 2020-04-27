/*
 * Copyright (c) 2020 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include <linux/slab.h>
#include <linux/firmware.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include "ldpc_fw_utils.h"

#define NV_LDPC_RISCV_PA_IMEM_START NV_PLDPC_RISCV_BROM_MEMLAYOUT_IMEM_START

static int write_tcm(struct ldpc_devdata *pdata, u32 offset, unsigned len,
		     void *buffer, bool isImem)
{
	u32 memC = 0U, memT = 0U, memD = 0U, aincw = 0U;
	u32 data;

	if (len == 0U) {
		return 0;	// no-op
	}

	if (len % 256 != 0) {
		pr_err("LDPC: Unaligned data\n");
		return -EINVAL;
	}

	if (isImem) {
		memT = NV_PLDPC_FALCON_IMEMT(0);
		memC = NV_PLDPC_FALCON_IMEMC(0);
		memD = NV_PLDPC_FALCON_IMEMD(0);
		aincw = DRF_DEF(_PLDPC_FALCON, _IMEMC, _AINCW, _TRUE);
	} else {
		memT = NV_PLDPC_FALCON_DMEMT(0);
		memC = NV_PLDPC_FALCON_DMEMC(0);
		memD = NV_PLDPC_FALCON_DMEMD(0);
		aincw = DRF_DEF(_PLDPC_FALCON, _DMEMC, _AINCW, _TRUE);
	}

	engineWrite(pdata, memT, 0);	// MEMT reset

	engineWrite(pdata, memC, aincw | offset);	// Set auto-inc offset
	while (len >= 4U) {
		(void)memcpy(&data, buffer, sizeof(data));
		engineWrite(pdata, memD, data);
		buffer = ((u8 *)buffer) + 4U;
		len -= 4U;
		offset += 4U;
	}
	msleep(100);

	return 0;
}

static int read_tcm(struct ldpc_devdata *pdata, u32 offset, unsigned len,
		    void *buffer, bool isImem)
{
	u32 memC = 0U, memT = 0U, memD = 0U, aincr = 0U;
        u32 data;

	if (len == 0U) {
		return 0;	// no-op
	}

	if (len % 256 != 0) {
		pr_err("LDPC: Unaligned data\n");
		return -EINVAL;
	}

	if (isImem) {
		memT = NV_PLDPC_FALCON_IMEMT(0);
		memC = NV_PLDPC_FALCON_IMEMC(0);
		memD = NV_PLDPC_FALCON_IMEMD(0);
		aincr = DRF_DEF(_PLDPC_FALCON, _IMEMC, _AINCR, _TRUE);
	} else {
		memT = NV_PLDPC_FALCON_DMEMT(0);
		memC = NV_PLDPC_FALCON_DMEMC(0);
		memD = NV_PLDPC_FALCON_DMEMD(0);
		aincr = DRF_DEF(_PLDPC_FALCON, _DMEMC, _AINCR, _TRUE);
	}

	engineWrite(pdata, memT, 0);	// MEMT reset

	engineWrite(pdata, memC, aincr | offset);	// Set auto-inc offset
	while (len >= 4U) {
		data = engineRead(pdata, memD);
		(void)memcpy(buffer, &data, sizeof(data));
		buffer = ((u8 *)buffer) + 4U;
		len -= 4U;
		offset += 4U;
	}
	msleep(100);

	return 0;
}

static int engine_write_imem(struct ldpc_devdata *pdata, u32 offset, unsigned len, void *buffer)
{
	int ret = 0;

	/* TODO: Add sanity check for length w.r.t. IMEM size */

	pr_debug("Writing ucode to IMEM address %08x.\n", offset);
	ret = write_tcm(pdata, offset, len, buffer, true);

	{
		u8 buf_read[len];
		u8 *buf_org = buffer;
		int i;

		pr_debug("Veryfying ...\n");
		ret = read_tcm(pdata, offset, len, buf_read, true);

		for (i=0; i<len; i++)
		{
			if (buf_read[i] != buf_org[i])
			{
				pr_err("Mismatch at %d/0x%x: %02x vs %02x\n", i, i,
						  buf_read[i], buf_org[i]);
				return -EBADF;
			}
		}
	}
	pr_debug("Writing ucode to IMEM address suc.\n");
	return ret;
}

static int engine_write_dmem(struct ldpc_devdata *pdata, u32 offset, unsigned len, void *buffer)
{
	int ret = 0;

	/* TODO: Add sanity check for length w.r.t. DMEM size */

	pr_debug("Writing data to DMEM address %08x.\n", offset);
	ret = write_tcm(pdata, offset, len, buffer, false);

	{
		u8 buf_read[len];
		u8 *buf_org = buffer;
		int i;

		pr_debug("Veryfying ...\n");
		ret = read_tcm(pdata, offset, len, buf_read, false);

		for (i = 0; i < len; i++)
		{
			if (buf_read[i] != buf_org[i])
			{
				pr_err("Mismatch at %d/0x%x: %02x vs %02x\n", i, i,
						  buf_read[i], buf_org[i]);
				return -EBADF;
			}
		}
	}
	return ret;
}

static int ldpc_read_ucode(const struct firmware *fw,
				struct ldpc_riscv_ucode *fw_ucode)
{
	int i, err = 0;
	size_t data_sz, code_sz, bin_head_sz;
	u32 *ucode_ptr = NULL;
	struct ldpc_bin_header *bin_header;

	ucode_ptr = (u32*)kzalloc(fw->size, GFP_KERNEL);
	if (ucode_ptr == NULL) {
		pr_err("ldpc: Failed to allocated memory: %s: %d\n", __func__, __LINE__);
		err = -ENOMEM;
		goto out;
	}

	for (i = 0; i < fw->size/sizeof(u32); i++) {
		ucode_ptr[i] = le32_to_cpu(((__le32 *)fw->data)[i]);
	}

	bin_header = (struct ldpc_bin_header *)ucode_ptr;

	if (bin_header->bin_magic != 0x10ae) {
		pr_err("failed to get firmware magic\n");
		err = -EINVAL;
		goto out;
	}

	bin_head_sz = bin_header->data_offset;
	data_sz = bin_header->code_offset - bin_header->data_offset;
	code_sz = bin_header->os_bin_size - data_sz;

	fw_ucode->bin_header = (struct ldpc_bin_header *)kzalloc(bin_head_sz, GFP_KERNEL);
	if (fw_ucode->bin_header == NULL) {
		pr_err("ldpc: Failed to allocated memory: %s: %d\n", __func__, __LINE__);
		err = -ENOMEM;
		goto free_fw;
	}

	fw_ucode->data_buf = (u32 *)kzalloc(data_sz, GFP_KERNEL);
	if (fw_ucode->data_buf == NULL) {
		pr_err("ldpc: Failed to allocated memory: %s: %d\n", __func__, __LINE__);
		err = -ENOMEM;
		goto free_bin_head;
	}

	fw_ucode->code_buf = (u32 *)kzalloc(code_sz, GFP_KERNEL);
	if (fw_ucode->code_buf == NULL) {
		pr_err("ldpc: Failed to allocated memory: %s: %d\n", __func__, __LINE__);
		err = -ENOMEM;
		goto free_data_buf;
	}

	memcpy(fw_ucode->bin_header, ((void *)ucode_ptr), bin_head_sz);
	memcpy(fw_ucode->data_buf, (((void *)ucode_ptr) + bin_header->data_offset), data_sz);
	memcpy(fw_ucode->code_buf, (((void *)ucode_ptr) + bin_header->data_offset), code_sz);

	goto free_fw;

free_data_buf:
	kfree(fw_ucode->data_buf);
free_bin_head:
	kfree(fw_ucode->bin_header);
free_fw:
	kfree(ucode_ptr);
out:
	return err;
}

static int ldpc_setup_ucode(struct ldpc_devdata *pdata,
			    struct ldpc_riscv_ucode *ucode)
{
	const struct firmware *fw;
	int err = 0;

	if (!current->fs) {
		pr_err("ldpc: Not on current file system: %d\n", err);
		WARN_ON(1);
		err = -ENODEV;
		goto out;
	}

	err = request_firmware(&fw, pdata->fw_name, pdata->dev);
	if (err != 0) {
		pr_err("ldpc: request_firmware failed: %d\n", err);
		err = -ENODEV;
		goto out;
	}

	err = ldpc_read_ucode(fw, ucode);
	if(err != 0) {
		pr_err("ldpc: Failed to read ucode: %d\n", err);
		err = -EBADF;
	}

	release_firmware(fw);
out:
	return err;
}

int load_fw_binaries(struct ldpc_devdata *pdata)
{
	size_t imgSize;
	int err = 0;
	struct ldpc_riscv_ucode ucode;

	err = ldpc_setup_ucode(pdata, &ucode);
	if (err != 0) {
		goto out;
	}

	imgSize = ucode.bin_header->code_offset - ucode.bin_header->data_offset;
	err = engine_write_dmem(pdata, 0, imgSize, ((void *)ucode.data_buf));
	if (err != 0) {
		goto free_buf;
	}

	imgSize = ucode.bin_header->os_bin_size - imgSize;
	err = engine_write_imem(pdata, 0, imgSize, ((void *)ucode.code_buf));
	if (err != 0) {
		goto free_buf;
	}

	pr_info("Images programmed. Booting...\n");

free_buf:
	kfree(ucode.code_buf);
	kfree(ucode.data_buf);
	kfree(ucode.bin_header);
out:
	return err;
}

static void ldpc_fw_core_select(struct ldpc_devdata *pdata)
{
	u32 bcr = 0;

	bcr = DRF_DEF(_PLDPC_RISCV, _BCR_CTRL, _CORE_SELECT, _RISCV)   |
	      DRF_DEF(_PLDPC_RISCV, _BCR_CTRL, _VALID,_TRUE);
	engineWrite(pdata, NV_PLDPC_RISCV_BCR_CTRL, bcr);
}

static void riscvBoot(struct ldpc_devdata *pdata)
{
	unsigned long long boot_vector = NV_LDPC_RISCV_PA_IMEM_START;

	engineWrite(pdata, NV_PLDPC_RISCV_BOOT_VECTOR_HI, (boot_vector >> 32) & 0xFFFFFFFF);
	engineWrite(pdata, NV_PLDPC_RISCV_BOOT_VECTOR_LO, boot_vector & 0xFFFFFFFF);
	engineWrite(pdata, NV_PLDPC_RISCV_CPUCTL, DRF_DEF(_PLDPC_RISCV, _CPUCTL, _STARTCPU, _TRUE));
}

int ldpc_load_firmware(struct ldpc_devdata *pdata)
{
	int err = 0;

	ldpc_fw_core_select(pdata);

	err = load_fw_binaries(pdata);
	if (err != 0) {
		pr_err("ldpc: load fw binaries failed: %s: %d\n", __func__, __LINE__);
		goto out;
	}

	riscvBoot(pdata);

out:
	return err;
}
