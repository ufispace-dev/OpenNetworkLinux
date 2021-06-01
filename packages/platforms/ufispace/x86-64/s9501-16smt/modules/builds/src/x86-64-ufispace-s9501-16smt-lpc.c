/*
 * A lpc driver for the ufispace_s9501_16smt
 *
 * Copyright (C) 2017-2020 UfiSpace Technology Corporation.
 * Jason Tsai <jason.cy.tsai@ufispace.com>
 * Wade He <wade.ce.he@ufispace.com>
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
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/hwmon-sysfs.h>

#define DRIVER_NAME "x86_64_ufispace_s9501_16smt_lpc"

/* LPC registers */

#define REG_BASE_CPU                      0x600
#define REG_BASE_MB                       0x700


//MB CPLD
#define REG_MB_BRD_ID_0                   (REG_BASE_MB + 0x00)
#define REG_MB_BRD_ID_1                   (REG_BASE_MB + 0x01)
#define REG_MB_CPLD_VERSION               (REG_BASE_MB + 0x02)
#define REG_MB_BIOS_BOOT                  (REG_BASE_MB + 0x04)
#define REG_MB_MUX_CTRL                   (REG_BASE_MB + 0x13)
#define REG_MB_MUX_RST                    (REG_BASE_MB + 0x1B)


#define MASK_ALL                          (0xFF)

/* LPC sysfs attributes index  */
enum lpc_sysfs_attributes {
    //MB CPLD
    ATT_MB_BRD_ID_0,
    ATT_MB_BRD_ID_1,
    ATT_MB_CPLD_1_VERSION,
    ATT_MB_CPLD_1_VERSION_H,
    ATT_MB_BIOS_BOOT_ROM,
    ATT_MB_BIOS_BOOT_CFG,
    ATT_MB_MUX_CTRL,
    ATT_MB_BRD_SKU_ID,
    ATT_MB_BRD_HW_ID,
    ATT_MB_BRD_ID_TYPE,
    ATT_MB_BRD_BUILD_ID,
    ATT_MB_BRD_DEPH_ID,
    ATT_MB_BRD_EXT_ID,
    ATT_MB_MUX_RESET,
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
static ssize_t read_mb_cpld_1_version_h(struct device *dev,
        struct device_attribute *da, char *buf)
{
    ssize_t len=0;
    u16 reg = REG_MB_CPLD_VERSION;
    u8 mask = MASK_ALL;
    u8 reg_val;

	mutex_lock(&lpc_data->access_lock);
	reg_val=_mask_shift(inb(reg), mask);
    len=sprintf(buf, "v%02d\n", reg_val);
    mutex_unlock(&lpc_data->access_lock);

    return len;
}

/* get cpu_mux_reset register value */
static ssize_t read_mb_mux_reset_callback(struct device *dev,
        struct device_attribute *da, char *buf)
{
    int len = 0, i=0;
    u8 reg_val = 0;
    u8 cpu_mux_reset = 0;

    mutex_lock(&lpc_data->access_lock);
    reg_val = inb(REG_MB_MUX_RST);
    cpu_mux_reset = reg_val;
    for(i=0; i < 5; i++){
        cpu_mux_reset |= (cpu_mux_reset >> i);
    }
    cpu_mux_reset = (cpu_mux_reset & 0b00011111 ) >> 4;

    len = sprintf(buf,"%d\n", cpu_mux_reset);
    mutex_unlock(&lpc_data->access_lock);

    return len;
}

/* set cpu_mux_reset register value */
static ssize_t write_mb_mux_reset(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    u8 val = 0;
    u8 reg_val = 0;
    static int cpu_mux_reset_flag = 0;

    if (kstrtou8(buf, 0, &val) < 0)
        return -EINVAL;

    if (cpu_mux_reset_flag == 0) {
        if (val == 0) {
            mutex_lock(&lpc_data->access_lock);
            cpu_mux_reset_flag = 1;
            printk(KERN_INFO "i2c mux reset is triggered...\n");
            reg_val = inb(REG_MB_MUX_RST);
            outb((reg_val & 0b11100000), REG_MB_MUX_RST);
            mdelay(100);
            outb((reg_val | 0b00011111), REG_MB_MUX_RST);
            mdelay(500);
            cpu_mux_reset_flag = 0;
            mutex_unlock(&lpc_data->access_lock);
        } else {
            return -EINVAL;
        }
    } else {
        printk(KERN_INFO "i2c mux is resetting... (ignore)\n");
        mutex_lock(&lpc_data->access_lock);
        mutex_unlock(&lpc_data->access_lock);
    }

    return count;
}

/* get lpc register value */
static ssize_t read_lpc_callback(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u16 reg = 0;
    u8 mask = MASK_ALL;

    switch (attr->index) {
        //CPU CPLD
        case ATT_MB_BIOS_BOOT_ROM:
            reg = REG_MB_BIOS_BOOT;
            mask = 0x80;
            break;
        case ATT_MB_BIOS_BOOT_CFG:
            reg = REG_MB_BIOS_BOOT;
            mask = 0x80;
            break;
        //MB CPLD
        case ATT_MB_BRD_ID_0:
            reg = REG_MB_BRD_ID_0;
            break;
        case ATT_MB_BRD_ID_1:
            reg = REG_MB_BRD_ID_1;
            break;
        case ATT_MB_CPLD_1_VERSION:
            reg = REG_MB_CPLD_VERSION;
            break;
        case ATT_MB_BRD_SKU_ID:
            reg = REG_MB_BRD_ID_0;
            mask = 0x0F;
            break;
        case ATT_MB_BRD_HW_ID:
            reg = REG_MB_BRD_ID_0;
            mask = 0x30;
            break;
        case ATT_MB_BRD_ID_TYPE:
            reg = REG_MB_BRD_ID_0;
            mask = 0x80;
            break;
        case ATT_MB_BRD_BUILD_ID:
            reg = REG_MB_BRD_ID_0;
            mask = 0xc0;
            break;
        case ATT_MB_BRD_DEPH_ID:
            reg = REG_MB_BRD_ID_1;
            mask = 0x10;
            break;
        case ATT_MB_BRD_EXT_ID:
            reg = REG_MB_BRD_ID_1;
            mask = 0x07;
            break;
        case ATT_MB_MUX_CTRL:
            reg = REG_MB_MUX_CTRL;
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
        case ATT_MB_MUX_CTRL:
            reg = REG_MB_MUX_CTRL;
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
static SENSOR_DEVICE_ATTR(mb_cpld_1_version,   S_IRUGO, read_lpc_callback, NULL, ATT_MB_CPLD_1_VERSION);
static SENSOR_DEVICE_ATTR(mb_cpld_1_version_h, S_IRUGO, read_mb_cpld_1_version_h, NULL, ATT_MB_CPLD_1_VERSION_H);
static SENSOR_DEVICE_ATTR(boot_rom,           S_IRUGO, read_lpc_callback, NULL, ATT_MB_BIOS_BOOT_ROM);
static SENSOR_DEVICE_ATTR(boot_cfg,           S_IRUGO, read_lpc_callback, NULL, ATT_MB_BIOS_BOOT_CFG);
static SENSOR_DEVICE_ATTR(mux_ctrl,          S_IRUGO | S_IWUSR, read_lpc_callback, write_lpc_callback, ATT_MB_MUX_CTRL);
static SENSOR_DEVICE_ATTR(board_sku_id,      S_IRUGO, read_lpc_callback, NULL, ATT_MB_BRD_SKU_ID);
static SENSOR_DEVICE_ATTR(board_hw_id,       S_IRUGO, read_lpc_callback, NULL, ATT_MB_BRD_HW_ID);
static SENSOR_DEVICE_ATTR(board_id_type,     S_IRUGO, read_lpc_callback, NULL, ATT_MB_BRD_ID_TYPE);
static SENSOR_DEVICE_ATTR(board_build_id,    S_IRUGO, read_lpc_callback, NULL, ATT_MB_BRD_BUILD_ID);
static SENSOR_DEVICE_ATTR(board_deph_id,     S_IRUGO, read_lpc_callback, NULL, ATT_MB_BRD_DEPH_ID);
static SENSOR_DEVICE_ATTR(board_ext_id,     S_IRUGO, read_lpc_callback, NULL, ATT_MB_BRD_EXT_ID);
static SENSOR_DEVICE_ATTR(mux_reset,        S_IRUGO | S_IWUSR, read_mb_mux_reset_callback, write_mb_mux_reset, ATT_MB_MUX_RESET);
//SENSOR_DEVICE_ATTR - BSP
static SENSOR_DEVICE_ATTR(bsp_version,     S_IRUGO | S_IWUSR, read_bsp_callback, write_bsp_callback, ATT_BSP_VERSION);
static SENSOR_DEVICE_ATTR(bsp_debug,       S_IRUGO | S_IWUSR, read_bsp_callback, write_bsp_callback, ATT_BSP_DEBUG);
static SENSOR_DEVICE_ATTR(bsp_reg,         S_IRUGO | S_IWUSR, read_lpc_callback, write_bsp_callback, ATT_BSP_REG);

static struct attribute *mb_cpld_attrs[] = {
    &sensor_dev_attr_board_id_0.dev_attr.attr,
    &sensor_dev_attr_board_id_1.dev_attr.attr,
    &sensor_dev_attr_mb_cpld_1_version.dev_attr.attr,
	&sensor_dev_attr_mb_cpld_1_version_h.dev_attr.attr,
	&sensor_dev_attr_board_sku_id.dev_attr.attr,
	&sensor_dev_attr_board_hw_id.dev_attr.attr,
	&sensor_dev_attr_board_id_type.dev_attr.attr,
	&sensor_dev_attr_board_build_id.dev_attr.attr,
	&sensor_dev_attr_board_deph_id.dev_attr.attr,
	&sensor_dev_attr_board_ext_id.dev_attr.attr,
	&sensor_dev_attr_mux_ctrl.dev_attr.attr,
	&sensor_dev_attr_mux_reset.dev_attr.attr,
    NULL,
};

static struct attribute *bios_attrs[] = {
    &sensor_dev_attr_boot_rom.dev_attr.attr,
    &sensor_dev_attr_boot_cfg.dev_attr.attr,
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

MODULE_AUTHOR("Wade He <wade.ce.he@ufispace.com>");
MODULE_DESCRIPTION("x86_64_ufispace_s9501_16smt_lpc driver");
MODULE_LICENSE("GPL");

module_init(lpc_init);
module_exit(lpc_exit);
