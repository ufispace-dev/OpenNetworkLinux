/*
 * A lpc driver for the ufispace_s9510_28dc
 *
 * Copyright (C) 2017-2020 UfiSpace Technology Corporation.
 * Jason Tsai <jason.cy.tsai@ufispace.com>
 *
 *
 * Based on ad7414.c
 * Copyright 2006 Stefan Roese <sr at denx.de>, DENX Software Engineering
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/acpi.h>
#include <linux/io.h>
#include <linux/dmi.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <uapi/linux/stat.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>

#define DRIVER_NAME "x86_64_ufispace_s9510_28dc_lpc"

/* LPC registers */

#define REG_BASE_CPU                      0x600
#define REG_BASE_MB                       0x700


//MB CPLD
#define REG_MB_BRD_ID_0                   (REG_BASE_MB + 0x00)
#define REG_MB_BRD_ID_1                   (REG_BASE_MB + 0x01)
#define REG_MB_CPLD_VERSION               (REG_BASE_MB + 0x02)
#define REG_MB_CPLD_ID                    (REG_BASE_MB + 0x03)
#define REG_MB_MUX_RESET                  (REG_BASE_MB + 0x43)
#define REG_MB_MAC_ROV                    (REG_BASE_MB + 0x52)
#define REG_MB_FAN_PRESENT                (REG_BASE_MB + 0x55)
#define REG_MB_PSU_STATUS                 (REG_BASE_MB + 0x59)
#define REG_MB_BIOS_BOOT_SEL              (REG_BASE_MB + 0x5B)
#define REG_MB_UART_CTRL                  (REG_BASE_MB + 0x63)
#define REG_MB_USB_CTRL                   (REG_BASE_MB + 0x64)
#define REG_MB_MUX_CTRL                   (REG_BASE_MB + 0x65)
#define REG_MB_LED_CLR                    (REG_BASE_MB + 0x80)
#define REG_MB_LED_CTRL_1                 (REG_BASE_MB + 0x81)
#define REG_MB_LED_CTRL_2                 (REG_BASE_MB + 0x82)

#define MASK_ALL                          (0xFF)

/* LPC sysfs attributes index  */
enum lpc_sysfs_attributes {
    //MB CPLD
    ATT_MB_BRD_ID_0,
    ATT_MB_BRD_ID_1,
    ATT_MB_BRD_SKU_ID,
    ATT_MB_BRD_HW_ID,
    ATT_MB_BRD_BUILD_ID,
    ATT_MB_BRD_DEPH_ID,
    ATT_MB_CPLD_VERSION,
    ATT_MB_CPLD_VERSION_H,
    ATT_MB_CPLD_ID,
    ATT_MB_MUX_RESET,
    ATT_MB_MAC_ROV,
    ATT_MB_FAN_PRESENT_0,
    ATT_MB_FAN_PRESENT_1,
    ATT_MB_FAN_PRESENT_2,
    ATT_MB_FAN_PRESENT_3,
    ATT_MB_FAN_PRESENT_4,
    ATT_MB_PSU_STATUS,
    ATT_MB_BIOS_BOOT_SEL,
    ATT_MB_UART_CTRL,
    ATT_MB_USB_CTRL,
    ATT_MB_MUX_CTRL,
    ATT_MB_LED_CLR,
    ATT_MB_LED_CTRL_1,
    ATT_MB_LED_CTRL_2,
    //BSP
    ATT_BSP_VERSION,
    ATT_BSP_DEBUG,
    ATT_BSP_REG,
    ATT_MAX
};

struct lpc_data_s {
    struct mutex    access_lock;
};

struct lpc_data_s *lpc_data;
char bsp_version[16];
char bsp_debug[32];
char bsp_reg[8]="0x0";

/* reg shift */
static u8 _shift(u8 mask)
{
    int i=0, mask_one=1;

    for(i=0; i<8; ++i) {
        if ((mask & mask_one) == 1)
            return i;
        else
            mask >>= 1;
    }

    return -1;
}

/* reg mask and shift */
static u8 _mask_shift(u8 val, u8 mask)
{
    int shift=0;

    shift = _shift(mask);

    return (val & mask) >> shift;
}

static u8 _bit_operation(u8 reg_val, u8 bit, u8 bit_val)
{
    if (bit_val == 0)
        reg_val = reg_val & ~(1 << bit);
    else
        reg_val = reg_val | (1 << bit);
    return reg_val;
}

/* get lpc register value */
static u8 _read_lpc_reg(u16 reg, u8 mask)
{
    u8 reg_val;

    mutex_lock(&lpc_data->access_lock);
    reg_val=_mask_shift(inb(reg), mask);
    mutex_unlock(&lpc_data->access_lock);

    return reg_val;
}

/* get lpc register value */
static ssize_t read_lpc_reg(u16 reg, u8 mask, char *buf)
{
    u8 reg_val;
    int len=0;

    reg_val = _read_lpc_reg(reg, mask);
    len=sprintf(buf,"%d\n", reg_val);

    return len;
}

/* set lpc register value */
static ssize_t write_lpc_reg(u16 reg, u8 mask, const char *buf, size_t count)
{
    u8 reg_val, reg_val_now, shift;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    //apply SINGLE BIT operation if mask is specified, multiple bits are not supported
    if (mask != MASK_ALL) {
        reg_val_now = _read_lpc_reg(reg, MASK_ALL);
        shift = _shift(mask);
        reg_val = _bit_operation(reg_val_now, shift, reg_val);
    }

    mutex_lock(&lpc_data->access_lock);

    outb(reg_val, reg);
    mutex_unlock(&lpc_data->access_lock);

    return count;
}

/* get bsp value */
static ssize_t read_bsp(char *buf, char *str)
{
    ssize_t len=0;

    mutex_lock(&lpc_data->access_lock);
    len=sprintf(buf, "%s", str);
    mutex_unlock(&lpc_data->access_lock);

    return len;
}

/* set bsp value */
static ssize_t write_bsp(const char *buf, char *str, size_t str_len, size_t count)
{
    mutex_lock(&lpc_data->access_lock);
    snprintf(str, str_len, "%s", buf);
    mutex_unlock(&lpc_data->access_lock);

    return count;
}

/* get mb cpld version in human readable format */
static ssize_t read_mb_cpld_version_h(struct device *dev,
        struct device_attribute *da, char *buf)
{
    ssize_t len=0;
    u16 reg = REG_MB_CPLD_VERSION;
    u8 mask = MASK_ALL;
    u8 mask_major = 0b11000000;
    u8 mask_minor = 0b00111111;
    u8 reg_val;

    mutex_lock(&lpc_data->access_lock);
    reg_val=_mask_shift(inb(reg), mask);
    len=sprintf(buf, "%d.%02d\n", _mask_shift(reg_val, mask_major), _mask_shift(reg_val, mask_minor));
    mutex_unlock(&lpc_data->access_lock);

    return len;
}

/* get lpc register value */
static ssize_t read_lpc_callback(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u16 reg = 0;
    u8 mask = MASK_ALL;

    switch (attr->index) {
        //MB CPLD
        case ATT_MB_BRD_ID_0:
            reg = REG_MB_BRD_ID_0;
            break;
        case ATT_MB_BRD_ID_1:
            reg = REG_MB_BRD_ID_1;
            break;
        case ATT_MB_BRD_SKU_ID:
            reg = REG_MB_BRD_ID_0;
            mask = 0x0F;
            break;
        case ATT_MB_BRD_HW_ID:
            reg = REG_MB_BRD_ID_1;
            mask = 0x03;
            break;
        case ATT_MB_BRD_BUILD_ID:
            reg = REG_MB_BRD_ID_1;
            mask = 0x18;
            break;
        case ATT_MB_BRD_DEPH_ID:
            reg = REG_MB_BRD_ID_1;
            mask = 0x04;
            break;
        case ATT_MB_CPLD_VERSION:
            reg = REG_MB_CPLD_VERSION;
            break;
        case ATT_MB_CPLD_ID:
            reg = REG_MB_CPLD_ID;
            break;
        case ATT_MB_MUX_RESET:
            reg = REG_MB_MUX_RESET;
            break;
        case ATT_MB_MAC_ROV:
            reg = REG_MB_MAC_ROV;
            mask = 0x07;
            break;
        case ATT_MB_FAN_PRESENT_0:
            reg = REG_MB_FAN_PRESENT;
            mask = 0x02;
            break;
        case ATT_MB_FAN_PRESENT_1:
            reg = REG_MB_FAN_PRESENT;
            mask = 0x04;
            break;
        case ATT_MB_FAN_PRESENT_2:
            reg = REG_MB_FAN_PRESENT;
            mask = 0x08;
            break;
        case ATT_MB_FAN_PRESENT_3:
            reg = REG_MB_FAN_PRESENT;
            mask = 0x10;
            break;
        case ATT_MB_FAN_PRESENT_4:
            reg = REG_MB_FAN_PRESENT;
            mask = 0x20;
            break;
        case ATT_MB_PSU_STATUS:
            reg = REG_MB_PSU_STATUS;
            break;
        case ATT_MB_BIOS_BOOT_SEL:
            reg = REG_MB_BIOS_BOOT_SEL;
            mask = 0x03;
            break;
        case ATT_MB_UART_CTRL:
            reg = REG_MB_UART_CTRL;
            mask = 0x03;
            break;
        case ATT_MB_USB_CTRL:
            reg = REG_MB_USB_CTRL;
            mask = 0x07;
            break;
        case ATT_MB_MUX_CTRL:
            reg = REG_MB_MUX_CTRL;
            mask = 0x1F;
            break;
        case ATT_MB_LED_CLR:
            reg = REG_MB_LED_CLR;
            mask = 0x07;
            break;
        case ATT_MB_LED_CTRL_1:
            reg = REG_MB_LED_CTRL_1;
            break;
        case ATT_MB_LED_CTRL_2:
            reg = REG_MB_LED_CTRL_2;
            break;
        //BSP
        case ATT_BSP_REG:
            if (kstrtou16(bsp_reg, 0, &reg) < 0)
                return -EINVAL;
            break;
        default:
            return -EINVAL;
    }
    return read_lpc_reg(reg, mask, buf);
}

/* set lpc register value */
static ssize_t write_lpc_callback(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u16 reg = 0;
    u8 mask = MASK_ALL;

    switch (attr->index) {
        case ATT_MB_MUX_RESET:
            reg = REG_MB_MUX_RESET;
            break;
        case ATT_MB_UART_CTRL:
            reg = REG_MB_UART_CTRL;
            break;
        case ATT_MB_USB_CTRL:
            reg = REG_MB_USB_CTRL;
            break;
        case ATT_MB_LED_CLR:
            reg = REG_MB_LED_CLR;
            break;
        case ATT_MB_LED_CTRL_1:
            reg = REG_MB_LED_CTRL_1;
            break;
        case ATT_MB_LED_CTRL_2:
            reg = REG_MB_LED_CTRL_2;
            break;
        default:
            return -EINVAL;
    }
    return write_lpc_reg(reg, mask, buf, count);
}

/* get bsp parameter value */
static ssize_t read_bsp_callback(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int str_len=0;
    char *str=NULL;

    switch (attr->index) {
        case ATT_BSP_VERSION:
            str = bsp_version;
            str_len = sizeof(bsp_version);
            break;
        case ATT_BSP_DEBUG:
            str = bsp_debug;
            str_len = sizeof(bsp_debug);
            break;
        case ATT_BSP_REG:
            str = bsp_reg;
            str_len = sizeof(bsp_reg);
            break;
        default:
            return -EINVAL;
    }
    return read_bsp(buf, str);
}

/* set bsp parameter value */
static ssize_t write_bsp_callback(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int str_len=0;
    char *str=NULL;
    u16 reg = 0;

    switch (attr->index) {
        case ATT_BSP_VERSION:
            str = bsp_version;
            str_len = sizeof(str);
            break;
        case ATT_BSP_DEBUG:
            str = bsp_debug;
            str_len = sizeof(str);
            break;
        case ATT_BSP_REG:
            if (kstrtou16(buf, 0, &reg) < 0)
                return -EINVAL;

            str = bsp_reg;
            str_len = sizeof(str);
            break;
        default:
            return -EINVAL;
    }
    return write_bsp(buf, str, str_len, count);
}

//SENSOR_DEVICE_ATTR - MB
static SENSOR_DEVICE_ATTR(board_id_0,        S_IRUGO, read_lpc_callback, NULL, ATT_MB_BRD_ID_0);
static SENSOR_DEVICE_ATTR(board_id_1,        S_IRUGO, read_lpc_callback, NULL, ATT_MB_BRD_ID_1);
static SENSOR_DEVICE_ATTR(board_sku_id,      S_IRUGO, read_lpc_callback, NULL, ATT_MB_BRD_SKU_ID);
static SENSOR_DEVICE_ATTR(board_hw_id,       S_IRUGO, read_lpc_callback, NULL, ATT_MB_BRD_HW_ID);
static SENSOR_DEVICE_ATTR(board_build_id,    S_IRUGO, read_lpc_callback, NULL, ATT_MB_BRD_BUILD_ID);
static SENSOR_DEVICE_ATTR(board_deph_id,     S_IRUGO, read_lpc_callback, NULL, ATT_MB_BRD_DEPH_ID);
static SENSOR_DEVICE_ATTR(mb_cpld_version,   S_IRUGO, read_lpc_callback, NULL, ATT_MB_CPLD_VERSION);
static SENSOR_DEVICE_ATTR(mb_cpld_version_h, S_IRUGO, read_mb_cpld_version_h, NULL, ATT_MB_CPLD_VERSION_H);
static SENSOR_DEVICE_ATTR(mb_cpld_id       , S_IRUGO, read_lpc_callback, NULL, ATT_MB_CPLD_ID);
static SENSOR_DEVICE_ATTR(mux_reset,       S_IRUGO | S_IWUSR, read_lpc_callback, write_lpc_callback, ATT_MB_MUX_RESET);
static SENSOR_DEVICE_ATTR(mac_rov,         S_IRUGO, read_lpc_callback, NULL, ATT_MB_MAC_ROV);
static SENSOR_DEVICE_ATTR(fan_present_0,   S_IRUGO, read_lpc_callback, NULL, ATT_MB_FAN_PRESENT_0);
static SENSOR_DEVICE_ATTR(fan_present_1,   S_IRUGO, read_lpc_callback, NULL, ATT_MB_FAN_PRESENT_1);
static SENSOR_DEVICE_ATTR(fan_present_2,   S_IRUGO, read_lpc_callback, NULL, ATT_MB_FAN_PRESENT_2);
static SENSOR_DEVICE_ATTR(fan_present_3,   S_IRUGO, read_lpc_callback, NULL, ATT_MB_FAN_PRESENT_3);
static SENSOR_DEVICE_ATTR(fan_present_4,   S_IRUGO, read_lpc_callback, NULL, ATT_MB_FAN_PRESENT_4);
static SENSOR_DEVICE_ATTR(psu_status,      S_IRUGO, read_lpc_callback, NULL, ATT_MB_PSU_STATUS);
static SENSOR_DEVICE_ATTR(bios_boot_sel,   S_IRUGO, read_lpc_callback, NULL, ATT_MB_BIOS_BOOT_SEL);
static SENSOR_DEVICE_ATTR(uart_ctrl,       S_IRUGO | S_IWUSR, read_lpc_callback, write_lpc_callback, ATT_MB_UART_CTRL);
static SENSOR_DEVICE_ATTR(usb_ctrl,        S_IRUGO | S_IWUSR, read_lpc_callback, write_lpc_callback, ATT_MB_USB_CTRL);
static SENSOR_DEVICE_ATTR(mux_ctrl,        S_IRUGO, read_lpc_callback, NULL, ATT_MB_MUX_CTRL);
static SENSOR_DEVICE_ATTR(led_clr,         S_IRUGO | S_IWUSR, read_lpc_callback, write_lpc_callback, ATT_MB_LED_CLR);
//FIXME: remove write operation
static SENSOR_DEVICE_ATTR(led_ctrl_1,   S_IRUGO | S_IWUSR, read_lpc_callback, write_lpc_callback, ATT_MB_LED_CTRL_1);
static SENSOR_DEVICE_ATTR(led_ctrl_2,   S_IRUGO | S_IWUSR, read_lpc_callback, write_lpc_callback, ATT_MB_LED_CTRL_2);

//SENSOR_DEVICE_ATTR - BSP
static SENSOR_DEVICE_ATTR(bsp_version,     S_IRUGO | S_IWUSR, read_bsp_callback, write_bsp_callback, ATT_BSP_VERSION);
static SENSOR_DEVICE_ATTR(bsp_debug,       S_IRUGO | S_IWUSR, read_bsp_callback, write_bsp_callback, ATT_BSP_DEBUG);
static SENSOR_DEVICE_ATTR(bsp_reg,         S_IRUGO | S_IWUSR, read_lpc_callback, write_bsp_callback, ATT_BSP_REG);

static struct attribute *mb_cpld_attrs[] = {
    &sensor_dev_attr_board_id_0.dev_attr.attr,
    &sensor_dev_attr_board_id_1.dev_attr.attr,
    &sensor_dev_attr_board_sku_id.dev_attr.attr,
    &sensor_dev_attr_board_hw_id.dev_attr.attr,
    &sensor_dev_attr_board_build_id.dev_attr.attr,
    &sensor_dev_attr_board_deph_id.dev_attr.attr,
    &sensor_dev_attr_mb_cpld_version.dev_attr.attr,
    &sensor_dev_attr_mb_cpld_version_h.dev_attr.attr,
    &sensor_dev_attr_mb_cpld_id.dev_attr.attr,
    &sensor_dev_attr_mux_reset.dev_attr.attr,
    &sensor_dev_attr_mac_rov.dev_attr.attr,
    &sensor_dev_attr_fan_present_0.dev_attr.attr,
    &sensor_dev_attr_fan_present_1.dev_attr.attr,
    &sensor_dev_attr_fan_present_2.dev_attr.attr,
    &sensor_dev_attr_fan_present_3.dev_attr.attr,
    &sensor_dev_attr_fan_present_4.dev_attr.attr,
    &sensor_dev_attr_psu_status.dev_attr.attr,
    &sensor_dev_attr_uart_ctrl.dev_attr.attr,
    &sensor_dev_attr_usb_ctrl.dev_attr.attr,
    &sensor_dev_attr_mux_ctrl.dev_attr.attr,
    &sensor_dev_attr_led_clr.dev_attr.attr,
    &sensor_dev_attr_led_ctrl_1.dev_attr.attr,
    &sensor_dev_attr_led_ctrl_2.dev_attr.attr,
    NULL,
};

static struct attribute *bios_attrs[] = {
    &sensor_dev_attr_bios_boot_sel.dev_attr.attr,
    NULL,
};

static struct attribute *bsp_attrs[] = {
    &sensor_dev_attr_bsp_version.dev_attr.attr,
    &sensor_dev_attr_bsp_debug.dev_attr.attr,
    &sensor_dev_attr_bsp_reg.dev_attr.attr,
    NULL,
};

static struct attribute_group mb_cpld_attr_grp = {
    .name = "mb_cpld",
    .attrs = mb_cpld_attrs,
};

static struct attribute_group bios_attr_grp = {
    .name = "bios",
    .attrs = bios_attrs,
};

static struct attribute_group bsp_attr_grp = {
    .name = "bsp",
    .attrs = bsp_attrs,
};

static void lpc_dev_release( struct device * dev)
{
    return;
}

static struct platform_device lpc_dev = {
    .name           = DRIVER_NAME,
    .id             = -1,
    .dev = {
                    .release = lpc_dev_release,
    }
};

static int lpc_drv_probe(struct platform_device *pdev)
{
    int i = 0, grp_num = 3;
    int err[5] = {0};
    struct attribute_group *grp;

    lpc_data = devm_kzalloc(&pdev->dev, sizeof(struct lpc_data_s),
                    GFP_KERNEL);
    if (!lpc_data)
        return -ENOMEM;

    mutex_init(&lpc_data->access_lock);

    for (i=0; i<grp_num; ++i) {
        switch (i) {
            case 0:
                grp = &mb_cpld_attr_grp;
                break;
            case 1:
                grp = &bios_attr_grp;
                break;
            case 2:
                grp = &bsp_attr_grp;
                break;
            default:
                break;
        }

        err[i] = sysfs_create_group(&pdev->dev.kobj, grp);
        if (err[i]) {
            printk(KERN_ERR "Cannot create sysfs for group %s\n", grp->name);
            goto exit;
        } else {
            continue;
        }
    }

    return 0;

exit:
    for (i=0; i<grp_num; ++i) {
        switch (i) {
            case 0:
                grp = &mb_cpld_attr_grp;
                break;
            case 1:
                grp = &bios_attr_grp;
                break;
            case 2:
                grp = &bsp_attr_grp;
                break;
            default:
                break;
        }

        sysfs_remove_group(&pdev->dev.kobj, grp);
        if (!err[i]) {
            //remove previous successful cases
            continue;
        } else {
            //remove first failed case, then return
            return err[i];
        }
    }
    return 0;
}

static int lpc_drv_remove(struct platform_device *pdev)
{
    sysfs_remove_group(&pdev->dev.kobj, &mb_cpld_attr_grp);
    sysfs_remove_group(&pdev->dev.kobj, &bios_attr_grp);
    sysfs_remove_group(&pdev->dev.kobj, &bsp_attr_grp);

    return 0;
}

static struct platform_driver lpc_drv = {
    .probe  = lpc_drv_probe,
    .remove = __exit_p(lpc_drv_remove),
    .driver = {
    .name   = DRIVER_NAME,
    },
};

int lpc_init(void)
{
    int err = 0;
    err = platform_device_register(&lpc_dev);
    if (err) {
        printk(KERN_ERR "%s(#%d): platform_device_register failed(%d)\n",
                __func__, __LINE__, err);
        return err;
    }
    err = platform_driver_register(&lpc_drv);
    if (err) {
        printk(KERN_ERR "%s(#%d): platform_driver_register failed(%d)\n",
                __func__, __LINE__, err);
        platform_device_unregister(&lpc_dev);
        return err;
    }

    return err;
}

void lpc_exit(void)
{
    platform_driver_unregister(&lpc_drv);
    platform_device_unregister(&lpc_dev);
}

MODULE_AUTHOR("Jason Tsai <jason.cy.tsai@ufispace.com>");
MODULE_DESCRIPTION("x86_64_ufispace_s9510_28dc_lpc driver");
MODULE_LICENSE("GPL");

module_init(lpc_init);
module_exit(lpc_exit);
