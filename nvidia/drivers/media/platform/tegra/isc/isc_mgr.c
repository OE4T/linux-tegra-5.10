/*
 * isc manager.
 *
 * Copyright (c) 2015-2021, NVIDIA Corporation. All Rights Reserved.
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

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regmap.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <asm/siginfo.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
#include <linux/sched/signal.h>
#endif
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <linux/i2c.h>
#include <linux/pwm.h>
#include <linux/debugfs.h>
#include <linux/nospec.h>
#include <linux/seq_file.h>
#include <media/isc-dev.h>
#include <media/isc-mgr.h>

#include <asm/barrier.h>

#include "isc-mgr-priv.h"

#define PW_ON(flag)	((flag) ? 0 : 1)
#define PW_OFF(flag)	((flag) ? 1 : 0)

/* minor number range would be 0 to 127 */
#define ISC_DEV_MAX	128

/* ISC Dev Debugfs functions
 *
 *    - isc_mgr_debugfs_init
 *    - isc_mgr_debugfs_remove
 *    - isc_mgr_status_show
 *    - isc_mgr_attr_set
 *    - pwr_on_get
 *    - pwr_on_set
 *    - pwr_off_get
 *    - pwr_off_set
 */
static int isc_mgr_status_show(struct seq_file *s, void *data)
{
	struct isc_mgr_priv *isc_mgr = s->private;
	struct isc_mgr_client *isc_dev;

	if (isc_mgr == NULL)
		return 0;
	pr_info("%s - %s\n", __func__, isc_mgr->devname);

	if (list_empty(&isc_mgr->dev_list)) {
		seq_printf(s, "%s: No devices supported.\n", isc_mgr->devname);
		return 0;
	}

	mutex_lock(&isc_mgr->mutex);
	list_for_each_entry_reverse(isc_dev, &isc_mgr->dev_list, list) {
		seq_printf(s, "    %02d  --  @0x%02x, %02d, %d, %s\n",
			isc_dev->id,
			isc_dev->cfg.addr,
			isc_dev->cfg.reg_bits,
			isc_dev->cfg.val_bits,
			isc_dev->cfg.drv_name
			);
	}
	mutex_unlock(&isc_mgr->mutex);

	return 0;
}

static ssize_t isc_mgr_attr_set(struct file *s,
		const char __user *user_buf, size_t count, loff_t *ppos)
{
	return count;
}

static int isc_mgr_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, isc_mgr_status_show, inode->i_private);
}

static const struct file_operations isc_mgr_debugfs_fops = {
	.open = isc_mgr_debugfs_open,
	.read = seq_read,
	.write = isc_mgr_attr_set,
	.llseek = seq_lseek,
	.release = single_release,
};

static int pwr_on_get(void *data, u64 *val)
{
	struct isc_mgr_priv *isc_mgr = data;

	if (isc_mgr->pdata == NULL || !isc_mgr->pdata->num_pwr_gpios) {
		*val = 0ULL;
		return 0;
	}

	*val = (isc_mgr->pwr_state & (BIT(28) - 1)) |
		((isc_mgr->pdata->num_pwr_gpios & 0x0f) << 28);
	return 0;
}

static int pwr_on_set(void *data, u64 val)
{
	return isc_mgr_power_up((struct isc_mgr_priv *)data, val);
}

DEFINE_SIMPLE_ATTRIBUTE(pwr_on_fops, pwr_on_get, pwr_on_set, "0x%02llx\n");

static int pwr_off_get(void *data, u64 *val)
{
	struct isc_mgr_priv *isc_mgr = data;

	if (isc_mgr->pdata == NULL || !isc_mgr->pdata->num_pwr_gpios) {
		*val = 0ULL;
		return 0;
	}

	*val = (~isc_mgr->pwr_state) & (BIT(isc_mgr->pdata->num_pwr_gpios) - 1);
	*val = (*val & (BIT(28) - 1)) |
		((isc_mgr->pdata->num_pwr_gpios & 0x0f) << 28);
	return 0;
}

static int pwr_off_set(void *data, u64 val)
{
	return isc_mgr_power_down((struct isc_mgr_priv *)data, val);
}

DEFINE_SIMPLE_ATTRIBUTE(pwr_off_fops, pwr_off_get, pwr_off_set, "0x%02llx\n");

int isc_mgr_debugfs_init(struct isc_mgr_priv *isc_mgr)
{
	struct dentry *d;

	dev_dbg(isc_mgr->dev, "%s %s\n", __func__, isc_mgr->devname);
	isc_mgr->d_entry = debugfs_create_dir(
		isc_mgr->devname, NULL);
	if (isc_mgr->d_entry == NULL) {
		dev_err(isc_mgr->dev, "%s: create dir failed\n", __func__);
		return -ENOMEM;
	}

	d = debugfs_create_file("map", S_IRUGO|S_IWUSR, isc_mgr->d_entry,
		(void *)isc_mgr, &isc_mgr_debugfs_fops);
	if (!d)
		goto debugfs_init_err;

	d = debugfs_create_file("pwr-on", S_IRUGO|S_IWUSR, isc_mgr->d_entry,
		(void *)isc_mgr, &pwr_on_fops);
	if (!d)
		goto debugfs_init_err;

	d = debugfs_create_file("pwr-off", S_IRUGO|S_IWUSR, isc_mgr->d_entry,
		(void *)isc_mgr, &pwr_off_fops);
	if (!d)
		goto debugfs_init_err;

	return 0;

debugfs_init_err:
	dev_err(isc_mgr->dev, "%s: create file failed\n", __func__);
	debugfs_remove_recursive(isc_mgr->d_entry);
	isc_mgr->d_entry = NULL;
	return -ENOMEM;
}

int isc_mgr_debugfs_remove(struct isc_mgr_priv *isc_mgr)
{
	if (isc_mgr->d_entry == NULL)
		return 0;
	debugfs_remove_recursive(isc_mgr->d_entry);
	isc_mgr->d_entry = NULL;
	return 0;
}

static irqreturn_t isc_mgr_isr(int irq, void *data)
{
	struct isc_mgr_priv *isc_mgr;
	int ret;
	unsigned long flags;

	if (data) {
		isc_mgr = (struct isc_mgr_priv *)data;
		isc_mgr->err_irq_recvd = true;
		wake_up_interruptible(&isc_mgr->err_queue);
		spin_lock_irqsave(&isc_mgr->spinlock, flags);
		if (isc_mgr->sinfo.si_signo && isc_mgr->t) {
			/* send the signal to user space */
			ret = send_sig_info(isc_mgr->sinfo.si_signo,
					&isc_mgr->sinfo,
					isc_mgr->t);
			if (ret < 0) {
				pr_err("error sending signal\n");
				spin_unlock_irqrestore(&isc_mgr->spinlock,
							flags);
				return IRQ_HANDLED;
			}
		}
		spin_unlock_irqrestore(&isc_mgr->spinlock, flags);
	}

	return IRQ_HANDLED;
}

int isc_delete_lst(struct device *dev, struct i2c_client *client)
{
	struct isc_mgr_priv *isc_mgr;
	struct isc_mgr_client *isc_dev;

	if (dev == NULL)
		return -EFAULT;

	isc_mgr = (struct isc_mgr_priv *)dev_get_drvdata(dev);

	mutex_lock(&isc_mgr->mutex);
	list_for_each_entry(isc_dev, &isc_mgr->dev_list, list) {
		if (isc_dev->client == client) {
			list_del(&isc_dev->list);
			break;
		}
	}
	mutex_unlock(&isc_mgr->mutex);

	return 0;
}
EXPORT_SYMBOL_GPL(isc_delete_lst);

static int isc_remove_dev(struct isc_mgr_priv *isc_mgr, unsigned long arg)
{
	struct isc_mgr_client *isc_dev;

	dev_dbg(isc_mgr->dev, "%s %ld\n", __func__, arg);
	mutex_lock(&isc_mgr->mutex);
	list_for_each_entry(isc_dev, &isc_mgr->dev_list, list) {
		if (isc_dev->id == arg) {
			list_del(&isc_dev->list);
			break;
		}
	}
	mutex_unlock(&isc_mgr->mutex);

	if (&isc_dev->list != &isc_mgr->dev_list)
		i2c_unregister_device(isc_dev->client);
	else
		dev_err(isc_mgr->dev, "%s: list %lx un-exist\n", __func__, arg);

	return 0;
}

static int __isc_create_dev(
	struct isc_mgr_priv *isc_mgr, struct isc_mgr_new_dev *new_dev)
{
	struct isc_mgr_client *isc_dev;
	struct i2c_board_info brd;
	int err = 0;

	if (new_dev->addr >= 0x80 || new_dev->drv_name[0] == '\0' ||
		(new_dev->val_bits != 8 && new_dev->val_bits != 16) ||
		(new_dev->reg_bits != 0 && new_dev->reg_bits != 8 &&
		new_dev->reg_bits != 16)) {
		dev_err(isc_mgr->dev,
			"%s: invalid isc dev params: %s %x %d %d\n",
			__func__, new_dev->drv_name, new_dev->addr,
			new_dev->reg_bits, new_dev->val_bits);
		return -EINVAL;
	}

	isc_dev = devm_kzalloc(isc_mgr->dev, sizeof(*isc_dev), GFP_KERNEL);
	if (!isc_dev) {
		dev_err(isc_mgr->dev, "Unable to allocate memory!\n");
		return -ENOMEM;
	}

	memcpy(&isc_dev->cfg, new_dev, sizeof(isc_dev->cfg));
	dev_dbg(isc_mgr->pdev, "%s - %s @ %x, %d %d\n", __func__,
		isc_dev->cfg.drv_name, isc_dev->cfg.addr,
		isc_dev->cfg.reg_bits, isc_dev->cfg.val_bits);

	snprintf(isc_dev->pdata.drv_name, sizeof(isc_dev->pdata.drv_name),
			"%s.%u.%02x", isc_dev->cfg.drv_name,
			isc_mgr->adap->nr, isc_dev->cfg.addr);
	isc_dev->pdata.reg_bits = isc_dev->cfg.reg_bits;
	isc_dev->pdata.val_bits = isc_dev->cfg.val_bits;
	isc_dev->pdata.pdev = isc_mgr->dev;

	mutex_init(&isc_dev->mutex);
	INIT_LIST_HEAD(&isc_dev->list);

	memset(&brd, 0, sizeof(brd));
	strncpy(brd.type, "isc-dev", sizeof(brd.type));
	brd.addr = isc_dev->cfg.addr;
	brd.platform_data = &isc_dev->pdata;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
	isc_dev->client = i2c_new_device(isc_mgr->adap, &brd);
#else
	isc_dev->client = i2c_new_client_device(isc_mgr->adap, &brd);
#endif
	if (!isc_dev->client) {
		dev_err(isc_mgr->dev,
			"%s cannot allocate client: %s bus %d, %x\n", __func__,
			isc_dev->pdata.drv_name, isc_mgr->adap->nr, brd.addr);
		err = -EINVAL;
		goto dev_create_err;
	}

	mutex_lock(&isc_mgr->mutex);
	if (!list_empty(&isc_mgr->dev_list))
		isc_dev->id = list_entry(isc_mgr->dev_list.next,
			struct isc_mgr_client, list)->id + 1;
	list_add(&isc_dev->list, &isc_mgr->dev_list);
	mutex_unlock(&isc_mgr->mutex);

dev_create_err:
	if (err) {
		devm_kfree(isc_mgr->dev, isc_dev);
		return err;
	} else
		return isc_dev->id;
}

static int isc_create_dev(struct isc_mgr_priv *isc_mgr, const void __user *arg)
{
	struct isc_mgr_new_dev d_cfg;

	if (copy_from_user(&d_cfg, arg, sizeof(d_cfg))) {
		dev_err(isc_mgr->pdev,
			"%s: failed to copy from user\n", __func__);
		return -EFAULT;
	}

	return __isc_create_dev(isc_mgr, &d_cfg);
}

static int isc_mgr_write_pid(struct file *file, const void __user *arg)
{
	struct isc_mgr_priv *isc_mgr = file->private_data;
	struct isc_mgr_sinfo sinfo;
	unsigned long flags;

	if (copy_from_user(&sinfo, arg, sizeof(sinfo))) {
		dev_err(isc_mgr->pdev,
			"%s: failed to copy from user\n", __func__);
		return -EFAULT;
	}

	if (isc_mgr->sinfo.si_int) {
		dev_err(isc_mgr->pdev, "exist signal info\n");
		return -EINVAL;
	}

	if ((sinfo.sig_no < SIGRTMIN) || (sinfo.sig_no > SIGRTMAX)) {
		dev_err(isc_mgr->pdev, "Invalid signal number\n");
		return -EINVAL;
	}

	if (!sinfo.pid) {
		dev_err(isc_mgr->pdev, "Invalid PID\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&isc_mgr->spinlock, flags);
	isc_mgr->sinfo.si_signo = isc_mgr->sig_no = sinfo.sig_no;
	isc_mgr->sinfo.si_code = SI_QUEUE;
	isc_mgr->sinfo.si_ptr = (void __user *)((unsigned long)sinfo.context);
	spin_unlock_irqrestore(&isc_mgr->spinlock, flags);

	rcu_read_lock();
	isc_mgr->t = pid_task(find_pid_ns(sinfo.pid, &init_pid_ns),
				PIDTYPE_PID);
	if (isc_mgr->t == NULL) {
		dev_err(isc_mgr->pdev, "no such pid\n");
		rcu_read_unlock();
		return -ENODEV;
	}
	rcu_read_unlock();

	return 0;
}

static int isc_mgr_get_pwr_info(struct isc_mgr_priv *isc_mgr,
				void __user *arg)
{
	struct isc_mgr_platform_data *pd = isc_mgr->pdata;
	struct isc_mgr_pwr_info pinfo;
	int err;

	if (copy_from_user(&pinfo, arg, sizeof(pinfo))) {
		dev_err(isc_mgr->pdev,
			"%s: failed to copy from user\n", __func__);
		return -EFAULT;
	}

	if (!pd->num_pwr_gpios) {
		dev_err(isc_mgr->pdev,
			"%s: no power gpios\n", __func__);
		pinfo.pwr_status = -1;
		err = -ENODEV;
		goto pwr_info_end;
	}

	if (pinfo.pwr_gpio >= pd->num_pwr_gpios || pinfo.pwr_gpio < 0) {
		dev_err(isc_mgr->pdev,
			"%s: invalid power gpio provided\n", __func__);
		pinfo.pwr_status = -1;
		err = -EINVAL;
		goto pwr_info_end;
	}

	pinfo.pwr_gpio = array_index_nospec(pinfo.pwr_gpio, pd->num_pwr_gpios);

	pinfo.pwr_status  = gpio_get_value(pd->pwr_gpios[pinfo.pwr_gpio]);
	err = 0;

pwr_info_end:
	if (copy_to_user(arg, &pinfo, sizeof(pinfo))) {
		dev_err(isc_mgr->pdev,
			"%s: failed to copy to user\n", __func__);
		return -EFAULT;
	}
	return err;
}

int isc_mgr_power_up(struct isc_mgr_priv *isc_mgr, unsigned long arg)
{
	struct isc_mgr_platform_data *pd = isc_mgr->pdata;
	int i;
	u32 pwr_gpio;

	dev_dbg(isc_mgr->pdev, "%s - %lu\n", __func__, arg);

	if (!pd->num_pwr_gpios)
		goto pwr_up_end;

	if (arg >= MAX_ISC_GPIOS)
		arg = MAX_ISC_GPIOS - 1;

	arg = array_index_nospec(arg, MAX_ISC_GPIOS);
	pwr_gpio = pd->pwr_mapping[arg];

	if (pwr_gpio < pd->num_pwr_gpios) {
		pwr_gpio = array_index_nospec(pwr_gpio, pd->num_pwr_gpios);
		gpio_set_value(pd->pwr_gpios[pwr_gpio],
			PW_ON(pd->pwr_flags[pwr_gpio]));
		isc_mgr->pwr_state |= BIT(pwr_gpio);
		return 0;
	}

	for (i = 0; i < pd->num_pwr_gpios; i++) {
		dev_dbg(isc_mgr->pdev, "  - %d, %d\n",
			pd->pwr_gpios[i], PW_ON(pd->pwr_flags[i]));
		gpio_set_value(pd->pwr_gpios[i], PW_ON(pd->pwr_flags[i]));
		isc_mgr->pwr_state |= BIT(i);
	}

pwr_up_end:
	return 0;
}

int isc_mgr_power_down(struct isc_mgr_priv *isc_mgr, unsigned long arg)
{
	struct isc_mgr_platform_data *pd = isc_mgr->pdata;
	int i;
	u32 pwr_gpio;

	dev_dbg(isc_mgr->pdev, "%s - %lx\n", __func__, arg);

	if (!pd->num_pwr_gpios)
		goto pwr_dn_end;

	if (arg >= MAX_ISC_GPIOS)
		arg = MAX_ISC_GPIOS - 1;

	arg = array_index_nospec(arg, MAX_ISC_GPIOS);

	pwr_gpio = pd->pwr_mapping[arg];

	if (pwr_gpio < pd->num_pwr_gpios) {
		pwr_gpio = array_index_nospec(pwr_gpio, pd->num_pwr_gpios);
		gpio_set_value(pd->pwr_gpios[pwr_gpio],
				PW_OFF(pd->pwr_flags[pwr_gpio]));
		isc_mgr->pwr_state &= ~BIT(pwr_gpio);
		return 0;
	}

	for (i = 0; i < pd->num_pwr_gpios; i++) {
		dev_dbg(isc_mgr->pdev, "  - %d, %d\n",
			pd->pwr_gpios[i], PW_OFF(pd->pwr_flags[i]));
		gpio_set_value(pd->pwr_gpios[i], PW_OFF(pd->pwr_flags[i]));
		isc_mgr->pwr_state &= ~BIT(i);
	}
	mdelay(7);

pwr_dn_end:
	return 0;
}

static int isc_mgr_misc_ctrl(struct isc_mgr_priv *isc_mgr, bool misc_on)
{
	struct isc_mgr_platform_data *pd = isc_mgr->pdata;
	int err, i;

	dev_dbg(isc_mgr->pdev, "%s - %s\n", __func__, misc_on ? "ON" : "OFF");

	if (!pd->num_misc_gpios)
		return 0;

	for (i = 0; i < pd->num_misc_gpios; i++) {
		if (misc_on) {
			if (devm_gpio_request(isc_mgr->pdev,
					      pd->misc_gpios[i],
					      "misc-gpio")) {
				dev_err(isc_mgr->pdev, "failed req GPIO: %d\n",
					pd->misc_gpios[i]);
				goto misc_ctrl_err;
			}

			err = gpio_direction_output(
				pd->misc_gpios[i], PW_ON(pd->misc_flags[i]));
		} else {
			err = gpio_direction_output(
				pd->misc_gpios[i], PW_OFF(pd->misc_flags[i]));
			devm_gpio_free(isc_mgr->pdev, pd->misc_gpios[i]);
		}
	}
	return 0;

misc_ctrl_err:
	for (; i >= 0; i--)
		devm_gpio_free(isc_mgr->pdev, pd->misc_gpios[i]);
	return -EBUSY;
}

static int isc_mgr_pwm_enable(
	struct isc_mgr_priv *isc_mgr, unsigned long arg)
{
	int err = 0;

	if (!isc_mgr || !isc_mgr->pwm)
		return -EINVAL;

	switch (arg) {
	case ISC_MGR_PWM_ENABLE:
		err = pwm_enable(isc_mgr->pwm);
		break;
	case ISC_MGR_PWM_DISABLE:
		pwm_disable(isc_mgr->pwm);
		break;
	default:
		dev_err(isc_mgr->pdev, "%s unrecognized command: %lx\n",
			__func__, arg);
	}

	return err;
}

static int isc_mgr_pwm_config(
	struct isc_mgr_priv *isc_mgr, const void __user *arg)
{
	struct isc_mgr_pwm_info pwm_cfg;
	int err = 0;

	if (!isc_mgr || !isc_mgr->pwm)
		return -EINVAL;

	if (copy_from_user(&pwm_cfg, arg, sizeof(pwm_cfg))) {
		dev_err(isc_mgr->pdev,
			"%s: failed to copy from user\n", __func__);
		return -EFAULT;
	}

	err = pwm_config(isc_mgr->pwm, pwm_cfg.duty_ns, pwm_cfg.period_ns);

	return err;
}

static long isc_mgr_ioctl(
	struct file *file, unsigned int cmd, unsigned long arg)
{
	struct isc_mgr_priv *isc_mgr = file->private_data;
	struct isc_mgr_platform_data *pd = isc_mgr->pdata;
	int err = 0;
	unsigned long flags;

	/* command distributor */
	switch (cmd) {
	case ISC_MGR_IOCTL_DEV_ADD:
		err = isc_create_dev(isc_mgr, (const void __user *)arg);
		break;
	case ISC_MGR_IOCTL_DEV_DEL:
		isc_remove_dev(isc_mgr, arg);
		break;
	case ISC_MGR_IOCTL_PWR_DN:
		err = isc_mgr_power_down(isc_mgr, arg);
		break;
	case ISC_MGR_IOCTL_PWR_UP:
		err = isc_mgr_power_up(isc_mgr, arg);
		break;
	case ISC_MGR_IOCTL_SET_PID:
		/* first enable irq to clear pending interrupt
		 * and then register PID
		 */
		if (isc_mgr->err_irq && !atomic_xchg(&isc_mgr->irq_in_use, 1))
			enable_irq(isc_mgr->err_irq);

		err = isc_mgr_write_pid(file, (const void __user *)arg);
		break;
	case ISC_MGR_IOCTL_SIGNAL:
		switch (arg) {
		case ISC_MGR_SIGNAL_RESUME:
			if (!isc_mgr->sig_no) {
				dev_err(isc_mgr->pdev,
					"invalid sig_no, setup pid first\n");
				return -EINVAL;
			}
			spin_lock_irqsave(&isc_mgr->spinlock, flags);
			isc_mgr->sinfo.si_signo = isc_mgr->sig_no;
			spin_unlock_irqrestore(&isc_mgr->spinlock, flags);
			break;
		case ISC_MGR_SIGNAL_SUSPEND:
			spin_lock_irqsave(&isc_mgr->spinlock, flags);
			isc_mgr->sinfo.si_signo = 0;
			spin_unlock_irqrestore(&isc_mgr->spinlock, flags);
			break;
		default:
			dev_err(isc_mgr->pdev, "%s unrecognized signal: %lx\n",
				__func__, arg);
		}
		break;
	case ISC_MGR_IOCTL_PWR_INFO:
		err = isc_mgr_get_pwr_info(isc_mgr, (void __user *)arg);
		break;
	case ISC_MGR_IOCTL_PWM_ENABLE:
		err = isc_mgr_pwm_enable(isc_mgr, arg);
		break;
	case ISC_MGR_IOCTL_PWM_CONFIG:
		err = isc_mgr_pwm_config(isc_mgr, (const void __user *)arg);
		break;
	case ISC_MGR_IOCTL_WAIT_ERR:
		if (isc_mgr->err_irq && !atomic_xchg(&isc_mgr->irq_in_use, 1)) {
			enable_irq(isc_mgr->err_irq);
			isc_mgr->err_irq_recvd = false;
		}

		err = wait_event_interruptible(isc_mgr->err_queue,
			isc_mgr->err_irq_recvd);
		isc_mgr->err_irq_recvd = false;
		break;
	case ISC_MGR_IOCTL_ABORT_WAIT_ERR:
		isc_mgr->err_irq_recvd = true;
		wake_up_interruptible(&isc_mgr->err_queue);
		break;
	case ISC_MGR_IOCTL_GET_EXT_PWR_CTRL:
		if (copy_to_user((void __user *)arg,
				&pd->ext_pwr_ctrl,
				sizeof(u8))) {
			dev_err(isc_mgr->pdev, "%s: failed to copy to user\n",
				__func__);
			return -EFAULT;
		}
		break;
	default:
		dev_err(isc_mgr->pdev, "%s unsupported ioctl: %x\n",
			__func__, cmd);
		err = -EINVAL;
	}

	if (err)
		dev_dbg(isc_mgr->pdev, "err = %d\n", err);

	return err;
}

static int isc_mgr_open(struct inode *inode, struct file *file)
{
	struct isc_mgr_priv *isc_mgr = container_of(inode->i_cdev,
					struct isc_mgr_priv, cdev);

	/* only one application can open one isc_mgr device */
	if (atomic_xchg(&isc_mgr->in_use, 1))
		return -EBUSY;

	dev_dbg(isc_mgr->pdev, "%s\n", __func__);
	file->private_data = isc_mgr;

	/* if runtime_pwrctrl_off is not true, power on all here */
	if (!isc_mgr->pdata->runtime_pwrctrl_off)
		isc_mgr_power_up(isc_mgr, 0xffffffff);

	isc_mgr_misc_ctrl(isc_mgr, true);
	return 0;
}

static int isc_mgr_release(struct inode *inode, struct file *file)
{
	struct isc_mgr_priv *isc_mgr = file->private_data;

	if (isc_mgr->pwm)
		if (pwm_is_enabled(isc_mgr->pwm))
			pwm_disable(isc_mgr->pwm);

	isc_mgr_misc_ctrl(isc_mgr, false);

	/* disable irq if irq is in use, when device is closed */
	if (atomic_xchg(&isc_mgr->irq_in_use, 0)) {
		disable_irq(isc_mgr->err_irq);
		isc_mgr->err_irq_recvd = true;
		wake_up_interruptible(&isc_mgr->err_queue);
	}

	/* if runtime_pwrctrl_off is not true, power off all here */
	if (!isc_mgr->pdata->runtime_pwrctrl_off)
		isc_mgr_power_down(isc_mgr, 0xffffffff);

	/* clear sinfo to prevent report error after handler is closed */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
	memset(&isc_mgr->sinfo, 0, sizeof(struct siginfo));
#else
	memset(&isc_mgr->sinfo, 0, sizeof(struct kernel_siginfo));
#endif
	isc_mgr->t = NULL;
	WARN_ON(!atomic_xchg(&isc_mgr->in_use, 0));

	return 0;
}

static const struct file_operations isc_mgr_fileops = {
	.owner = THIS_MODULE,
	.open = isc_mgr_open,
	.unlocked_ioctl = isc_mgr_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = isc_mgr_ioctl,
#endif
	.release = isc_mgr_release,
};

static void isc_mgr_del(struct isc_mgr_priv *isc_mgr)
{
	struct isc_mgr_platform_data *pd = isc_mgr->pdata;
	struct isc_mgr_client *isc_dev = NULL;
	int i;

	mutex_lock(&isc_mgr->mutex);
	list_for_each_entry(isc_dev, &isc_mgr->dev_list, list) {
		/* remove i2c_clients that isc-mgr created */
		if (isc_dev->client != NULL) {
			i2c_unregister_device(isc_dev->client);
			isc_dev->client = NULL;
		}
	}
	mutex_unlock(&isc_mgr->mutex);

	for (i = 0; i < pd->num_pwr_gpios; i++)
		if (pd->pwr_gpios[i])
			gpio_direction_output(
				pd->pwr_gpios[i], PW_OFF(pd->pwr_flags[i]));

	i2c_put_adapter(isc_mgr->adap);
}

static void isc_mgr_dev_ins(struct work_struct *work)
{
	struct isc_mgr_priv *isc_mgr =
		container_of(work, struct isc_mgr_priv, ins_work);
	struct device_node *np = isc_mgr->pdev->of_node;
	struct device_node *subdev;
	struct isc_mgr_new_dev d_cfg = {.drv_name = "isc-dev"};
	const char *sname;
	u32 val;
	int err = 0;

	if (np == NULL)
		return;

	dev_dbg(isc_mgr->dev, "%s - %s\n", __func__, np->full_name);
	sname = of_get_property(np, "isc-dev", NULL);
	if (sname)
		strncpy(d_cfg.drv_name, sname, sizeof(d_cfg.drv_name) - 8);

	for_each_child_of_node(np, subdev) {
		err = of_property_read_u32(subdev, "addr", &val);
		if (err || !val) {
			dev_err(isc_mgr->dev, "%s: ERROR %d addr = %d\n",
				__func__, err, val);
			continue;
		}
		d_cfg.addr = val;
		err = of_property_read_u32(subdev, "reg_len", &val);
		if (err || !val) {
			dev_err(isc_mgr->dev, "%s: ERROR %d reg_len = %d\n",
				__func__, err, val);
			continue;
		}
		d_cfg.reg_bits = val;
		err = of_property_read_u32(subdev, "dat_len", &val);
		if (err || !val) {
			dev_err(isc_mgr->dev, "%s: ERROR %d dat_len = %d\n",
				__func__, err, val);
			continue;
		}
		d_cfg.val_bits = val;

		__isc_create_dev(isc_mgr, &d_cfg);
	}
}

static int isc_mgr_of_get_grp_gpio(
	struct device *dev, struct device_node *np,
	const char *name, int size, u32 *grp, u32 *flags)
{
	int num, i;

	num = of_gpio_named_count(np, name);
	dev_dbg(dev, "    num gpios of %s: %d\n", name, num);
	if (num < 0)
		return 0;

	for (i = 0; (i < num) && (i < size); i++) {
		grp[i] = of_get_named_gpio_flags(np, name, i, &flags[i]);
		if ((int)grp[i] < 0) {
			dev_err(dev, "%s: gpio[%d] invalid\n", __func__, i);
			return -EINVAL;
		}
		dev_dbg(dev, "        [%d] - %d %x\n", i, grp[i], flags[i]);
	}

	return num;
}

static int isc_mgr_get_pwr_map(
	struct device *dev, struct device_node *np,
	struct isc_mgr_platform_data *pd)
{
	int num_map_items = 0;
	u32 pwr_map_val;
	unsigned int i;

	for (i = 0; i < MAX_ISC_GPIOS; i++)
		pd->pwr_mapping[i] = i;

	if (!of_get_property(np, "pwr-items", NULL))
		return 0;

	num_map_items = of_property_count_elems_of_size(np,
				"pwr-items", sizeof(u32));
	if (num_map_items < 0) {
		dev_err(dev, "%s: error processing pwr items\n",
			__func__);
		return -1;
	}

	if (num_map_items < pd->num_pwr_gpios) {
		dev_err(dev, "%s: invalid number of pwr items\n",
			__func__);
		return -1;
	}

	for (i = 0; i < num_map_items; i++) {
		if (of_property_read_u32_index(np, "pwr-items",
			i, &pwr_map_val)) {
			dev_err(dev, "%s: failed to get pwr item\n",
				__func__);
			goto pwr_map_err;
		}

		if (pwr_map_val >= pd->num_pwr_gpios) {
			dev_err(dev, "%s: invalid power item index provided\n",
				__func__);
			goto pwr_map_err;
		}
		pd->pwr_mapping[i] = pwr_map_val;
	}

	pd->num_pwr_map = num_map_items;
	return 0;

pwr_map_err:
	for (i = 0; i < MAX_ISC_GPIOS; i++)
		pd->pwr_mapping[i] = i;

	pd->num_pwr_map = pd->num_pwr_gpios;

	return -1;
}

static struct isc_mgr_platform_data *of_isc_mgr_pdata(struct platform_device
	*pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct isc_mgr_platform_data *pd = NULL;
	int err;
	bool ext_pwr_ctrl_des = false, ext_pwr_ctrl_sensor = false;

	dev_dbg(&pdev->dev, "%s\n", __func__);
	pd = devm_kzalloc(&pdev->dev, sizeof(*pd), GFP_KERNEL);
	if (!pd) {
		dev_err(&pdev->dev, "%s: allocate memory error\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

	pd->drv_name = (void *)of_get_property(np, "drv_name", NULL);
	if (pd->drv_name)
		dev_dbg(&pdev->dev, "    drvname: %s\n", pd->drv_name);

	err = of_property_read_u32(np, "i2c-bus", &pd->bus);
	if (err) {
		dev_err(&pdev->dev, "%s: missing i2c bus # DT %s\n",
			__func__, np->full_name);
		return ERR_PTR(-EEXIST);
	}
	dev_dbg(&pdev->dev, "    i2c-bus: %d\n", pd->bus);

	err = of_property_read_u32(np, "csi-port", &pd->csi_port);
	if (err) {
		dev_err(&pdev->dev, "%s: missing csi port # DT %s\n",
			__func__, np->full_name);
		return ERR_PTR(-EEXIST);
	}
	dev_dbg(&pdev->dev, "    csiport: %d\n", pd->csi_port);

	pd->num_pwr_gpios = isc_mgr_of_get_grp_gpio(
		&pdev->dev, np, "pwdn-gpios",
		ARRAY_SIZE(pd->pwr_gpios), pd->pwr_gpios, pd->pwr_flags);
	if (pd->num_pwr_gpios < 0)
		return ERR_PTR(pd->num_pwr_gpios);

	pd->num_misc_gpios = isc_mgr_of_get_grp_gpio(
		&pdev->dev, np, "misc-gpios",
		ARRAY_SIZE(pd->misc_gpios), pd->misc_gpios, pd->misc_flags);
	if (pd->num_misc_gpios < 0)
		return ERR_PTR(pd->num_misc_gpios);

	pd->default_pwr_on = of_property_read_bool(np, "default-power-on");
	pd->runtime_pwrctrl_off =
		of_property_read_bool(np, "runtime-pwrctrl-off");

	pd->ext_pwr_ctrl = 0;
	ext_pwr_ctrl_des =
		of_property_read_bool(np, "ext-pwr-ctrl-deserializer");
	if (ext_pwr_ctrl_des == true)
		pd->ext_pwr_ctrl |= 1 << 0;
	ext_pwr_ctrl_sensor = of_property_read_bool(np, "ext-pwr-ctrl-sensor");
	if (ext_pwr_ctrl_sensor == true)
		pd->ext_pwr_ctrl |= 1 << 1;

	err = isc_mgr_get_pwr_map(&pdev->dev, np, pd);
	if (err)
		dev_err(&pdev->dev,
			"%s: failed to map pwr items. Using default values\n",
			__func__);

	return pd;
}

static char *isc_mgr_devnode(struct device *dev, umode_t *mode)
{
	if (!mode)
		return NULL;

	/* set alway user to access this device */
	*mode = 0666;

	return NULL;
}

static int isc_mgr_suspend(struct device *dev)
{
	/* Nothing required for isc-mgr suspend*/
	return 0;
}

static int isc_mgr_resume(struct device *dev)
{
	struct pwm_device *pwm;
	/* Reconfigure PWM as done during boot time */
	if (of_property_read_bool(dev->of_node, "pwms")) {
		pwm = devm_pwm_get(dev, NULL);
		if (!IS_ERR(pwm))
			dev_info(dev, "%s Resume successful\n", __func__);
	}
	return 0;
}

static const struct dev_pm_ops isc_mgr_pm_ops = {
	.suspend = isc_mgr_suspend,
	.resume = isc_mgr_resume,
	.runtime_suspend = isc_mgr_suspend,
	.runtime_resume = isc_mgr_resume,
};

static int isc_mgr_probe(struct platform_device *pdev)
{
	int err = 0;
	struct isc_mgr_priv *isc_mgr;
	struct isc_mgr_platform_data *pd;
	unsigned int i;

	dev_info(&pdev->dev, "%sing...\n", __func__);

	isc_mgr = devm_kzalloc(&pdev->dev,
			sizeof(struct isc_mgr_priv),
			GFP_KERNEL);
	if (!isc_mgr) {
		dev_err(&pdev->dev, "Unable to allocate memory!\n");
		return -ENOMEM;
	}

	spin_lock_init(&isc_mgr->spinlock);
	atomic_set(&isc_mgr->in_use, 0);
	INIT_LIST_HEAD(&isc_mgr->dev_list);
	mutex_init(&isc_mgr->mutex);
	init_waitqueue_head(&isc_mgr->err_queue);
	isc_mgr->err_irq_recvd = false;
	isc_mgr->pwm = NULL;

	if (pdev->dev.of_node) {
		pd = of_isc_mgr_pdata(pdev);
		if (IS_ERR(pd))
			return PTR_ERR(pd);
		isc_mgr->pdata = pd;
	} else if (pdev->dev.platform_data) {
		isc_mgr->pdata = pdev->dev.platform_data;
		pd = isc_mgr->pdata;
	} else {
		dev_err(&pdev->dev, "%s No platform data.\n", __func__);
		return -EFAULT;
	}

	if (of_property_read_bool(pdev->dev.of_node, "pwms")) {
		isc_mgr->pwm = devm_pwm_get(&pdev->dev, NULL);
		if (!IS_ERR(isc_mgr->pwm)) {
			dev_info(&pdev->dev,
				"%s: success to get PWM\n", __func__);
			pwm_disable(isc_mgr->pwm);
		} else {
			err = PTR_ERR(isc_mgr->pwm);
			if (err != -EPROBE_DEFER)
				dev_err(&pdev->dev,
					"%s: fail to get PWM\n", __func__);
			return err;
		}
	}

	isc_mgr->adap = i2c_get_adapter(pd->bus);
	if (!isc_mgr->adap) {
		dev_err(&pdev->dev, "%s no such i2c bus %d\n",
			__func__, pd->bus);
		return -ENODEV;
	}

	if (pd->num_pwr_gpios > 0) {
		for (i = 0; i < pd->num_pwr_gpios; i++) {
			if (!gpio_is_valid(pd->pwr_gpios[i]))
				goto err_probe;

			if (devm_gpio_request(
				&pdev->dev, pd->pwr_gpios[i], "pwdn-gpios")) {
				dev_err(&pdev->dev, "failed to req GPIO: %d\n",
					pd->pwr_gpios[i]);
				goto err_probe;
			}

			err = gpio_direction_output(pd->pwr_gpios[i],
				pd->default_pwr_on ?
				PW_ON(pd->pwr_flags[i]) :
				PW_OFF(pd->pwr_flags[i]));
			if (err < 0) {
				dev_err(&pdev->dev, "failed to setup GPIO: %d\n",
					pd->pwr_gpios[i]);
				i++;
				goto err_probe;
			}
			if (pd->default_pwr_on)
				isc_mgr->pwr_state |= BIT(i);
		}
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
	isc_mgr->err_irq = platform_get_irq(pdev, 0);
#else
	isc_mgr->err_irq = platform_get_irq_optional(pdev, 0);
#endif
	if (isc_mgr->err_irq > 0) {
		err = devm_request_irq(&pdev->dev,
				isc_mgr->err_irq,
				isc_mgr_isr, 0, pdev->name, isc_mgr);
		if (err) {
			dev_err(&pdev->dev,
				"request_irq failed with err %d\n", err);
			isc_mgr->err_irq = 0;
			goto err_probe;
		}
		disable_irq(isc_mgr->err_irq);
		atomic_set(&isc_mgr->irq_in_use, 0);
	}

	isc_mgr->pdev = &pdev->dev;
	dev_set_drvdata(&pdev->dev, isc_mgr);

	if (pd->drv_name)
		snprintf(isc_mgr->devname, sizeof(isc_mgr->devname),
			"%s.%x.%c", pd->drv_name, pd->bus, 'a' + pd->csi_port);
	else
		snprintf(isc_mgr->devname, sizeof(isc_mgr->devname),
			"isc-mgr.%x.%c", pd->bus, 'a' + pd->csi_port);

	/* Request dynamic allocation of a device major number */
	err = alloc_chrdev_region(&isc_mgr->devt,
				0, ISC_DEV_MAX, isc_mgr->devname);
	if (err < 0) {
		dev_err(&pdev->dev, "failed to allocate char dev region %d\n",
			err);
		goto err_probe;
	}

	/* poluate sysfs entries */
	isc_mgr->isc_class = class_create(THIS_MODULE, isc_mgr->devname);
	if (IS_ERR(isc_mgr->isc_class)) {
		err = PTR_ERR(isc_mgr->isc_class);
		isc_mgr->isc_class = NULL;
		dev_err(&pdev->dev, "failed to create class %d\n",
			err);
		goto err_probe;
	}

	isc_mgr->isc_class->devnode = isc_mgr_devnode;

	/* connect the file operations with the cdev */
	cdev_init(&isc_mgr->cdev, &isc_mgr_fileops);
	isc_mgr->cdev.owner = THIS_MODULE;

	/* connect the major/minor number to this dev */
	err = cdev_add(&isc_mgr->cdev, MKDEV(MAJOR(isc_mgr->devt), 0), 1);
	if (err) {
		dev_err(&pdev->dev, "Unable to add cdev %d\n", err);
		goto err_probe;
	}

	/* send uevents to udev, it will create /dev node for isc-mgr */
	isc_mgr->dev = device_create(isc_mgr->isc_class, &pdev->dev,
				     isc_mgr->cdev.dev,
				     isc_mgr,
				     isc_mgr->devname);
	if (IS_ERR(isc_mgr->dev)) {
		err = PTR_ERR(isc_mgr->dev);
		isc_mgr->dev = NULL;
		dev_err(&pdev->dev, "failed to create device %d\n", err);
		goto err_probe;
	}

	isc_mgr_debugfs_init(isc_mgr);
	INIT_WORK(&isc_mgr->ins_work, isc_mgr_dev_ins);
	schedule_work(&isc_mgr->ins_work);
	return 0;

err_probe:
	isc_mgr_del(isc_mgr);
	return err;
}

static int isc_mgr_remove(struct platform_device *pdev)
{
	struct isc_mgr_priv *isc_mgr = dev_get_drvdata(&pdev->dev);

	if (isc_mgr) {
		isc_mgr_debugfs_remove(isc_mgr);
		isc_mgr_del(isc_mgr);

		if (isc_mgr->dev)
			device_destroy(isc_mgr->isc_class,
				       isc_mgr->cdev.dev);
		if (isc_mgr->cdev.dev)
			cdev_del(&isc_mgr->cdev);

		if (isc_mgr->isc_class)
			class_destroy(isc_mgr->isc_class);

		if (isc_mgr->devt)
			unregister_chrdev_region(isc_mgr->devt, ISC_DEV_MAX);
	}

	return 0;
}

static const struct of_device_id isc_mgr_of_match[] = {
	{ .compatible = "nvidia,isc-mgr", },
	{ }
};
MODULE_DEVICE_TABLE(of, isc_mgr_of_match);

static struct platform_driver isc_mgr_driver = {
	.driver = {
		.name = "isc-mgr",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(isc_mgr_of_match),
		.pm = &isc_mgr_pm_ops,
	},
	.probe = isc_mgr_probe,
	.remove = isc_mgr_remove,
};

module_platform_driver(isc_mgr_driver);

MODULE_DESCRIPTION("tegra auto isc manager driver");
MODULE_AUTHOR("Songhee Baek <sbeak@nvidia.com>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:isc_mgr");
MODULE_SOFTDEP("pre: isc_pwm");
