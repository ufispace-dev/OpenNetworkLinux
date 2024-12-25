/*
 * Copyright 2024 UfiSpace
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 */

/*
 * IRQ handler driver.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/slab.h>

/**********************  DEFINES  ******************************************/
/* Module Information */
#define VERSION         "0.0.1"
#define MOD_NAME        "irq_handler"
#define IRQ_DEBUG       0

/*****************   DATA STRUCTURES   ****************************************/
/* IRQ control info */
typedef struct irq_info_s {
    uint32_t  irq_num;
} irq_info_t;

/*********************   STATIC GLOBAL VARIABLES   *************************/
static irq_info_t *irqInfo = NULL;
static int irq_num = -1;

module_param(irq_num, int, 0);

/*********************************************************************
*
* @purpose Interrupts handler
*
* @param   param [in] irq_num Interrupt vector from kernel.
*          param [in] data Interrupt control information
*
* @returns IRQ_HANDLED Interrupt recognized and handled.
*
* @notes   None
* @end
*
*********************************************************************/
static irqreturn_t isr(int irq_num, void *data)
{
#if IRQ_DEBUG
    irq_info_t *irqInfo = (irq_info_t *)data;

    printk(KERN_INFO "%s: Process interrupt num %d\n", MOD_NAME, irqInfo->irq_num);
#endif

    return IRQ_HANDLED;
}

/*********************************************************************
*
* @purpose Initializes the module.
*          It's called on driver initialization.
*
* @param   None
*
* @returns 0 Success, -1 otherwise.
*
* @notes   None
* @end
*
*********************************************************************/
static int __init irq_module_init(void)
{
    int ret = 0;

    if (irq_num == -1) {
        printk(KERN_ERR "%s: Invalid irq_num %d. Please specify irq_num when loading irq_handler driver\n", MOD_NAME, irq_num);
        return -EINVAL;
    }

    if ((irqInfo = kzalloc(sizeof(irq_info_t), GFP_KERNEL)) == NULL) {
        return -ENOMEM;
    }
    memset(irqInfo, 0, sizeof(irq_info_t));

    // set irq_num from module_param
    irqInfo->irq_num = irq_num;

    /* Specific module Initialization */
    if ( (ret=request_irq(irqInfo->irq_num, isr, IRQF_SHARED, MOD_NAME, irqInfo)) < 0) {
        printk(KERN_WARNING "%s: Could not request irq %d, ret=%d\n", MOD_NAME, irqInfo->irq_num, ret);
        kfree(irqInfo);
        irqInfo = NULL;

        return -EINVAL;
    } else {
        printk(KERN_INFO "%s: Request irq %d is successful\n", MOD_NAME, irqInfo->irq_num);
    }

    return 0; /* succeed */
}

/*********************************************************************
*
* @purpose Terminates the module.
*          It's called on driver exit.
*
* @param   None
*
* @returns None
*
* @notes   None
* @end
*
*********************************************************************/
static void __exit irq_module_cleanup(void)
{
    free_irq(irqInfo->irq_num, irqInfo);
    kfree(irqInfo);
    irqInfo = NULL;
}

MODULE_AUTHOR("UfiSpace Corporation");
MODULE_DESCRIPTION("IRQ Handler Driver");
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");

module_init(irq_module_init);
module_exit(irq_module_cleanup);
