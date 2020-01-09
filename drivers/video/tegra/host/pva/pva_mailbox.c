/*
 * PVA mailbox code
 *
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

#include <linux/export.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <soc/tegra/chip-id.h>
#include <linux/platform_device.h>

#include "nvhost_acm.h"
#include "dev.h"
#include "pva.h"
#include "pva_mailbox.h"

static u32 pva_get_mb_reg_id(u32 i)
{
	u32 mb_reg_id[VALID_MB_INPUT_REGS] = {
		0,
		1,
		2,
		3
	};

	return mb_reg_id[i];
}

static int pva_mailbox_send_cmd(struct pva *pva, struct pva_cmd *cmd,
				u32 nregs)
{
	struct platform_device *pdev = pva->pdev;
	u32 reg, status;
	int i;

	if (nregs > VALID_MB_INPUT_REGS) {
		pr_err("%s nregs %d more than expected\n", __func__, nregs);
		return -EINVAL;
	}

	/* Make sure the state is what we expect it to be. */
	status = pva->version_config->read_mailbox(pdev, PVA_MBOX_ISR);

	WARN_ON((status & PVA_INT_PENDING));
	WARN_ON((status & PVA_READY) == 0);
	WARN_ON((status & PVA_BUSY));

	/* Write all of the other command mailbox
	 * registers before writing mailbox 0.
	 */
	for (i = (nregs - 1); i >= 0; i--) {
		reg = pva_get_mb_reg_id(i);
		pva->version_config->write_mailbox(pdev, reg, cmd->mbox[i]);
	}

	return 0;
}

int pva_mailbox_wait_event(struct pva *pva, int wait_time)
{
	int timeout = 1;
	int err;
	/* Wait for the event being triggered in ISR */
	if (pva->timeout_enabled == true)
		timeout = wait_event_timeout(pva->mailbox_waitqueue,
			pva->mailbox_status == PVA_MBOX_STATUS_DONE ||
			pva->mailbox_status == PVA_MBOX_STATUS_ABORTED,
			msecs_to_jiffies(wait_time));
	else
		wait_event(pva->mailbox_waitqueue,
			pva->mailbox_status == PVA_MBOX_STATUS_DONE ||
			pva->mailbox_status == PVA_MBOX_STATUS_ABORTED);

	if (timeout <= 0) {
		err = -ETIMEDOUT;
		pva_abort(pva);
	} else if  (pva->mailbox_status == PVA_MBOX_STATUS_ABORTED)
		err = -EIO;
	else
		err = 0;

	return err;
}

void pva_mailbox_isr(struct pva *pva)
{
	struct platform_device *pdev = pva->pdev;
	u32 int_status = pva->version_config->read_mailbox(pdev, PVA_MBOX_ISR);

	if (pva->mailbox_status != PVA_MBOX_STATUS_WFI) {
		nvhost_warn(&pdev->dev, "Unexpected PVA ISR (%x)", int_status);
		return;
	}

	/* Save the current command and subcommand for later processing */
	pva->mailbox_status_regs.cmd =
		pva->version_config->read_mailbox(pdev, PVA_MBOX_COMMAND);
	pva->version_config->read_status_interface(pva, PVA_MBOX_INTERFACE,
					int_status, &pva->mailbox_status_regs);
	/* Clear the mailbox interrupt status */
	int_status = int_status & PVA_READY;
	pva->version_config->write_mailbox(pdev, PVA_MBOX_ISR, int_status);

	/* Wake up the waiters */
	pva->mailbox_status = PVA_MBOX_STATUS_DONE;
	wake_up(&pva->mailbox_waitqueue);
}

int pva_mailbox_send_cmd_sync_locked(struct pva *pva,
			struct pva_cmd *cmd, u32 nregs,
			struct pva_mailbox_status_regs *mailbox_status_regs)
{
	int err = 0;

	if (mailbox_status_regs == NULL) {
		err = -EINVAL;
		goto err_invalid_parameter;
	}

	/* Ensure that mailbox state is sane */
	if (WARN_ON(pva->mailbox_status != PVA_MBOX_STATUS_INVALID)) {
		err = -EIO;
		goto err_check_status;
	}

	/* Mark that we are waiting for an interrupt */
	pva->mailbox_status = PVA_MBOX_STATUS_WFI;
	memset(&pva->mailbox_status_regs, 0, sizeof(pva->mailbox_status_regs));

	/* Submit command to PVA */
	err = pva_mailbox_send_cmd(pva, cmd, nregs);
	if (err < 0)
		goto err_send_command;

	err = pva_mailbox_wait_event(pva, 100);
	if (err < 0)
		goto err_wait_response;

	/* Return interrupt status back to caller */
	memcpy(mailbox_status_regs, &pva->mailbox_status_regs,
				sizeof(struct pva_mailbox_status_regs));

	pva->mailbox_status = PVA_MBOX_STATUS_INVALID;
	return err;

err_wait_response:
err_send_command:
	pva->mailbox_status = PVA_MBOX_STATUS_INVALID;
err_check_status:
err_invalid_parameter:
	return err;
}

int pva_mailbox_send_cmd_sync(struct pva *pva,
			struct pva_cmd *cmd, u32 nregs,
			struct pva_mailbox_status_regs *mailbox_status_regs)
{
	int err = 0;

	if (mailbox_status_regs == NULL) {
		err = -EINVAL;
		goto err_invalid_parameter;
	}

	mutex_lock(&pva->mailbox_mutex);
	err = pva_mailbox_send_cmd_sync_locked(pva,
					       cmd,
					       nregs,
					       mailbox_status_regs);
	mutex_unlock(&pva->mailbox_mutex);

	return err;

err_invalid_parameter:
	return err;
}
EXPORT_SYMBOL(pva_mailbox_send_cmd_sync);
