/*
 * Copyright (c) 2020 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
#include <linux/module.h>
#include <linux/init.h>

static int __init ldpc_init(void)
{
        printk("LDPC init\n");
        return 0;
}

static void __exit ldpc_exit(void)
{
        printk("LDPC exit\n");
        return;
}

module_init(ldpc_init);
module_exit(ldpc_exit);
MODULE_LICENSE("GPL v2");
