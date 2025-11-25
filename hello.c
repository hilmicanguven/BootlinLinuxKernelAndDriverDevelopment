// SPDX-License-Identifier: GPL-2.0

#include <linux/init.h>
#include <linux/module.h>

static int __init hello_init(void)
{
    printk("init hello world module \n");
}

static int __exit hello_exit(void)
{
    printk("exit hello world module");
}