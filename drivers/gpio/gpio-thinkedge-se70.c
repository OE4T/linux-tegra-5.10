// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Copyright (C) 2023 Zededa Inc. All rights reserved.
 *
 * Driver for the NCT5635Y GPIO expander present in the Lenovo ThinkEdge
 * SE70 for GPIO control and Serial Port protocol selection.
 *
 * Author: Renê de Souza Pinto <rene@renesp.com.br>
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio/driver.h>
#include <linux/mutex.h>
#include <linux/i2c.h>

/* NCT5635Y registers */
#define REG_INPUT_GPIO0    0x00
#define REG_INPUT_GPIO1    0x01
#define REG_OUTPUT_GPIO0   0x02
#define REG_OUTPUT_GPIO1   0x03
#define REG_CONFIG_GPIO0   0x06
#define REG_CONFIG_GPIO1   0x07

/* GP0 port
 * GP0_0~3 = Output pins
 * GP0_4~7 = Input pins
 */
#define DIO_CONFIG 0xf0

/* GP1 port
 * GP1_0~1 = Output pins
 * GP1_4~5 = Output pins
 */
#define SER_CONFIG_MASK 0xcc

/* GP1 port
 * GP1_5 COM0_MODE0
 * GP1_4 COM0_MODE1
 * GP1_1 COM1_MODE0
 * GP1_0 COM1_MODE1
 *
 * These pins are connected to a multiprotocol transceiver (MaxLinear
 * SP339E) and set the communication protocol for the serial ports (COM0
 * and COM1):
 *
 * MODE0|MODE1|Serial mode
 *   0  |  0  | LOOP
 *   0  |  1  | RS-232
 *   1  |  0  | RS-485
 *   1  |  1  | RS-422
 */
#define COM0_REG_MODE0  BIT(5)
#define COM0_REG_MODE1  BIT(4)
#define COM1_REG_MODE0  BIT(1)
#define COM1_REG_MODE1  BIT(0)

struct _se70 {
	struct gpio_chip gpio_dio;
	struct gpio_chip gpio_ser;
	struct mutex lock;
};

/* Exported GPIO pins:
 * 0 - COM0_MODE0
 * 1 - COM0_MODE1
 * 2 - COM1_MODE0
 * 3 - COM1_MODE1
 */
const unsigned int ser_regs[] = {
	COM0_REG_MODE0,
	COM0_REG_MODE1,
	COM1_REG_MODE0,
	COM1_REG_MODE1
};

static int nct5635y_read(struct gpio_chip *gc, int reg)
{
	struct i2c_client *client = to_i2c_client(gc->parent);

	return i2c_smbus_read_byte_data(client, reg);
}

static int nct5635y_write(struct gpio_chip *gc, int reg, int value)
{
	struct i2c_client *client = to_i2c_client(gc->parent);

	return i2c_smbus_write_byte_data(client, reg, value);
}

static int setup_serial_pins(struct gpio_chip *gc)
{
	int ret, reg;

	/* Configure serial mode pins as outputs */
	reg = nct5635y_read(gc, REG_CONFIG_GPIO1);
	if (reg < 0)
		goto err;

	reg &= SER_CONFIG_MASK;
	ret  = nct5635y_write(gc, REG_CONFIG_GPIO1, reg);
	if (reg < 0)
		goto err;

	/* Set default mode to RS-232 on both serial ports */
	reg = nct5635y_read(gc, REG_OUTPUT_GPIO1);
	if (reg < 0)
		goto err;

	reg  = (reg & 0xcc) | 0x22; /* Set xx10xx10b */
	ret  = nct5635y_write(gc, REG_OUTPUT_GPIO1, reg);
err:
	return ret;
}

static int dio_get_direction(struct gpio_chip *gc, unsigned int offset)
{
	/* I/O pins 0~3 are Digital Outputs and 4~7 are Digital Inputs */
	if (offset >= 0 && offset <= 3)
		return GPIO_LINE_DIRECTION_OUT;
	else
		return GPIO_LINE_DIRECTION_IN;
}

static void dio_set(struct gpio_chip *gc, unsigned int offset, int value)
{
	int reg, val;
	struct _se70 *se70_gpio = gpiochip_get_data(gc);

	if (offset > 3)
		return;

	mutex_lock(&se70_gpio->lock);

	reg = nct5635y_read(gc, REG_INPUT_GPIO0);
	if (reg < 0)
		goto out;

	if (value > 0)
		val = (reg | BIT(offset));
	else
		val = (reg & ~BIT(offset));

	nct5635y_write(gc, REG_OUTPUT_GPIO0, val);
out:
	mutex_unlock(&se70_gpio->lock);
}

static int dio_get(struct gpio_chip *gc, unsigned int offset)
{
	int reg;

	if (offset >= 8)
		return -EINVAL;

	reg = nct5635y_read(gc, REG_INPUT_GPIO0);
	if (reg < 0)
		return reg;

	if (!(reg & BIT(offset)))
		return 0;
	else
		return 1;
}

static int ser_get_direction(struct gpio_chip *gc, unsigned int offset)
{
	/* Serial mode pins are always output pins */
	return GPIO_LINE_DIRECTION_OUT;
}

static void ser_set(struct gpio_chip *gc, unsigned int offset, int value)
{
	int reg, val;
	struct _se70 *se70_gpio = gpiochip_get_data(gc);

	if (offset >= 4)
		return;

	mutex_lock(&se70_gpio->lock);

	reg = nct5635y_read(gc, REG_OUTPUT_GPIO1);
	if (reg < 0)
		goto out;

	if (value > 0)
		val = (reg | ser_regs[offset]);
	else
		val = (reg & ~ser_regs[offset]);

	nct5635y_write(gc, REG_OUTPUT_GPIO1, val);
out:
	mutex_unlock(&se70_gpio->lock);
}

static int ser_get(struct gpio_chip *gc, unsigned int offset)
{
	int reg;

	if (offset >= 4)
		return -EINVAL;

	reg = nct5635y_read(gc, REG_INPUT_GPIO1);
	if (reg < 0)
		return reg;

	if (!(reg & ser_regs[offset]))
		return 0;
	else
		return 1;
}

static int thinkedge_se70_probe(struct i2c_client *client)
{
	struct _se70 *se70_gpio;
	int ret = 0;

	se70_gpio = devm_kzalloc(&client->dev, sizeof(*se70_gpio), GFP_KERNEL);
	if (!se70_gpio)
		return -ENOMEM;

	mutex_init(&se70_gpio->lock);

	/* Digital Input/Output pins */
	se70_gpio->gpio_dio.label         = "dio";
	se70_gpio->gpio_dio.parent        = &client->dev;
	se70_gpio->gpio_dio.owner         = THIS_MODULE;
	se70_gpio->gpio_dio.get_direction = dio_get_direction;
	se70_gpio->gpio_dio.get           = dio_get;
	se70_gpio->gpio_dio.set           = dio_set;
	se70_gpio->gpio_dio.base          = -1;
	se70_gpio->gpio_dio.ngpio         = 8;
	se70_gpio->gpio_dio.can_sleep     = true;

	ret = devm_gpiochip_add_data(&client->dev, &se70_gpio->gpio_dio, se70_gpio);
	if (ret)
		goto err_io;

	/* GPIO pins that control Serial Port protocol */
	se70_gpio->gpio_ser.label         = "serialmode";
	se70_gpio->gpio_ser.parent        = &client->dev;
	se70_gpio->gpio_ser.owner         = THIS_MODULE;
	se70_gpio->gpio_ser.get_direction = ser_get_direction;
	se70_gpio->gpio_ser.get           = ser_get;
	se70_gpio->gpio_ser.set           = ser_set;
	se70_gpio->gpio_ser.base          = -1;
	se70_gpio->gpio_ser.ngpio         = 4;
	se70_gpio->gpio_ser.can_sleep     = true;

	ret = devm_gpiochip_add_data(&client->dev, &se70_gpio->gpio_ser, se70_gpio);
	if (ret)
		goto err_ser;

	/* Setup Digital I/O pins */
	ret = nct5635y_write(&se70_gpio->gpio_dio, REG_CONFIG_GPIO0, DIO_CONFIG);
	if (ret)
		goto err_setup;

	/* Setup serial port mode pins */
	ret = setup_serial_pins(&se70_gpio->gpio_ser);
	if (ret)
		goto err_setup;

	pr_info("ThinkEdge SE70 GPIO expander initialized\n");
	return ret;
err_setup:
	gpiochip_remove(&se70_gpio->gpio_ser);
err_ser:
	gpiochip_remove(&se70_gpio->gpio_dio);
err_io:
	pr_err("Failed to initialize ThinkEdge SE70 GPIO expander\n");
	kfree(se70_gpio);
	return ret;
}

static const struct of_device_id i2c_se70_of_match[] = {
	{ .compatible = "lenovo,thinkedge-se70" },
	{},
};
MODULE_DEVICE_TABLE(of, i2c_se70_of_match);

static struct i2c_driver i2c_se70_driver = {
	.driver = {
		.name = "ThinkEdge-SE70",
		.of_match_table = of_match_ptr(i2c_se70_of_match),
	},
	.probe_new = thinkedge_se70_probe,
};
module_i2c_driver(i2c_se70_driver);

MODULE_AUTHOR("Renê de Souza Pinto <rene@renesp.com.br>");
MODULE_DESCRIPTION("GPIO control and Serial Port protocol selection for Lenovo ThinkEdge SE70");
MODULE_LICENSE("GPL");
