/*
 * A lpc driver for the ufispace_s9700_53dx platform.
 *
 * Copyright (C) 2017-2019 UfiSpace Technology Corporation.
 * Jason Tsai <jason.cy.tsai@ufispace.com>
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
#include <linux/gpio.h>

#define BSP_LOG_R(fmt, args...) \
    _bsp_log (LOG_READ, KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
            __FILE__, __func__, __LINE__, ##args)
#define BSP_LOG_W(fmt, args...) \
    _bsp_log (LOG_WRITE, KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
            __FILE__, __func__, __LINE__, ##args)
#define BSP_PR(level, fmt, args...) _bsp_log (LOG_SYS, level "[BSP]" fmt "\r\n", ##args)

#define DRIVER_NAME "x86_64_ufispace_s9700_53dx_lpc"

/* LPC Base Address */
#define REG_BASE_CPU                      0x600
#define REG_BASE_MB                       0x700

/* CPU CPLD Register */
#define REG_CPU_CPLD_VERSION              (REG_BASE_CPU + 0x00)
#define REG_CPU_STATUS_0                  (REG_BASE_CPU + 0x01)
#define REG_CPU_STATUS_1                  (REG_BASE_CPU + 0x02)
#define REG_CPU_CTRL_0                    (REG_BASE_CPU + 0x03)
#define REG_CPU_CTRL_1                    (REG_BASE_CPU + 0x04)
#define REG_CPU_CTRL_2                    (REG_BASE_CPU + 0x0B)

/* MB CPLD Register */
#define REG_MB_CPLD1_VERSION              (REG_BASE_MB + 0x02)

/* MAC Temp Register */
#define REG_TEMP_J2_PM0                   (REG_BASE_MB + 0x60)
#define REG_TEMP_J2_PM1                   (REG_BASE_MB + 0x61)
#define REG_TEMP_J2_PM2                   (REG_BASE_MB + 0x62)
#define REG_TEMP_J2_PM3                   (REG_BASE_MB + 0x63)
#define REG_TEMP_OP2                      (REG_BASE_MB + 0x64)

#define MASK_ALL                          (0xFF)
#define MASK_CPLD_MAJOR_VER               (0b11000000)
#define MASK_CPLD_MINOR_VER               (0b00111111)
#define MASK_BIOS_BOOT_ROM                (0b10000000)
#define MASK_CPU_MUX_RESET                (0b00000001)
#define LPC_MDELAY                        (5)

/* LPC sysfs attributes index  */
enum lpc_sysfs_attributes {
    /* CPU CPLD */
    ATT_CPU_CPLD_VERSION,
    ATT_CPU_CPLD_VERSION_H,
	ATT_CPU_BIOS_BOOT_ROM,
    ATT_CPU_MUX_RESET,
    /* MB CPLD */
    ATT_MB_CPLD1_VERSION,
    ATT_MB_CPLD1_VERSION_H,
    //BSP
    ATT_BSP_VERSION,
    ATT_BSP_DEBUG,
    ATT_BSP_PR_INFO,
    ATT_BSP_PR_ERR,
    ATT_BSP_REG,
    ATT_BSP_GPIO_MAX,
    /* MAC TEMP */
    ATT_TEMP_J2_PM0,
    ATT_TEMP_J2_PM1,
    ATT_TEMP_J2_PM2,
    ATT_TEMP_J2_PM3,
    ATT_TEMP_OP2,
    ATT_MAX
};

enum bsp_log_types {
    LOG_NONE,
    LOG_RW,
    LOG_READ,
    LOG_WRITE,
    LOG_SYS
};

enum bsp_log_ctrl {
    LOG_DISABLE,
    LOG_ENABLE
};

struct lpc_data_s {
    struct mutex    access_lock;
};

struct lpc_data_s *lpc_data;
char bsp_version[16]="";
char bsp_debug[2]="0";
char bsp_reg[8]="0x0";
u8 enable_log_read=LOG_DISABLE;
u8 enable_log_write=LOG_DISABLE;
u8 enable_log_sys=LOG_ENABLE;

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

static int _bsp_log(u8 log_type, char *fmt, ...)
{
    if ((log_type==LOG_READ  && enable_log_read) ||
        (log_type==LOG_WRITE && enable_log_write) ||
        (log_type==LOG_SYS && enable_log_sys) ) {
        va_list args;
        int r;

        va_start(args, fmt);
        r = vprintk(fmt, args);
        va_end(args);

        return r;
    } else {
        return 0;
    }
}

static int _config_bsp_log(u8 log_type)
{
    switch(log_type) {
        case LOG_NONE:
            enable_log_read = LOG_DISABLE;
            enable_log_write = LOG_DISABLE;
            break;
        case LOG_RW:
            enable_log_read = LOG_ENABLE;
            enable_log_write = LOG_ENABLE;
            break;
        case LOG_READ:
            enable_log_read = LOG_ENABLE;
            enable_log_write = LOG_DISABLE;
            break;
        case LOG_WRITE:
            enable_log_read = LOG_DISABLE;
            enable_log_write = LOG_ENABLE;
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static void _outb(u8 data, u16 port)
{
    outb(data, port);
    mdelay(LPC_MDELAY);
}

/* get lpc register value */
static u8 _read_lpc_reg(u16 reg, u8 mask)
{
    u8 reg_val;

    mutex_lock(&lpc_data->access_lock);
    reg_val=_mask_shift(inb(reg), mask);
    mutex_unlock(&lpc_data->access_lock);

    BSP_LOG_R("reg=0x%03x, reg_val=0x%02x", reg, reg_val);

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

    //apply continuous bits operation if mask is specified, discontinuous bits are not supported
    if (mask != MASK_ALL) {
        reg_val_now = _read_lpc_reg(reg, MASK_ALL);
        //clear bits in reg_val_now by the mask
        reg_val_now &= ~mask;
        //get bit shift by the mask
        shift = _shift(mask);
        //calculate new reg_val
        reg_val = reg_val_now | (reg_val << shift);
    }

    mutex_lock(&lpc_data->access_lock);

    _outb(reg_val, reg);

    mutex_unlock(&lpc_data->access_lock);

    BSP_LOG_W("reg=0x%03x, reg_val=0x%02x", reg, reg_val);

    return count;
}

/* get bsp value */
static ssize_t read_bsp(char *buf, char *str)
{
	ssize_t len=0;

	mutex_lock(&lpc_data->access_lock);
    len=sprintf(buf, "%s", str);
    mutex_unlock(&lpc_data->access_lock);

    BSP_LOG_R("reg_val=%s", str);

    return len;
}

/* set bsp value */
static ssize_t write_bsp(const char *buf, char *str, size_t str_len, size_t count)
{
	mutex_lock(&lpc_data->access_lock);
    snprintf(str, str_len, "%s", buf);
    mutex_unlock(&lpc_data->access_lock);

    BSP_LOG_W("reg_val=%s", str);

    return count;
}

/* get gpio max value */
static ssize_t read_gpio_max(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    if (attr->index == ATT_BSP_GPIO_MAX) {
        return sprintf(buf, "%d\n", ARCH_NR_GPIOS-1);
    }
    return -1;
}

/* get cpu_cpld_version register value */
static ssize_t read_cpu_cpld_version(struct device *dev,
        struct device_attribute *da, char *buf)
{
    return sprintf(buf,"%d\n", _read_lpc_reg(REG_CPU_CPLD_VERSION, MASK_ALL));
}

/* get cpu_cpld_version_h register value (human readable) */
static ssize_t read_cpu_cpld_version_h(struct device *dev,
        struct device_attribute *da, char *buf)
{
    return sprintf(buf,"%d.%02d\n", _read_lpc_reg(REG_CPU_CPLD_VERSION, MASK_CPLD_MAJOR_VER),
                                    _read_lpc_reg(REG_CPU_CPLD_VERSION, MASK_CPLD_MINOR_VER));
}

/* get bios_boot_rom register value */
static ssize_t read_cpu_bios_boot_rom(struct device *dev,
        struct device_attribute *da, char *buf)
{
    return sprintf(buf,"%d\n", _read_lpc_reg(REG_CPU_STATUS_1, MASK_BIOS_BOOT_ROM));
}

/* get cpu_mux_reset register value */
static ssize_t read_cpu_mux_reset_callback(struct device *dev,
        struct device_attribute *da, char *buf)
{
    return sprintf(buf,"%d\n", _read_lpc_reg(REG_CPU_CTRL_2, MASK_CPU_MUX_RESET));
}

/* set cpu_mux_reset register value */
static ssize_t write_cpu_mux_reset(struct device *dev,
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
            BSP_LOG_W("i2c mux reset is triggered...");
            reg_val = inb(REG_CPU_CTRL_2);
            outb((reg_val & 0b11111110), REG_CPU_CTRL_2);
            mdelay(100);
            BSP_LOG_W("reg=0x%03x, reg_val=0x%02x", REG_CPU_CTRL_2, reg_val & 0b11111110);
            outb((reg_val | 0b00000001), REG_CPU_CTRL_2);
            mdelay(500);
            BSP_LOG_W("reg=0x%03x, reg_val=0x%02x", REG_CPU_CTRL_2, reg_val | 0b00000001);
            cpu_mux_reset_flag = 0;
            mutex_unlock(&lpc_data->access_lock);
        } else {
            return -EINVAL;
        }
    } else {
        BSP_LOG_W("i2c mux is resetting... (ignore)");
        mutex_lock(&lpc_data->access_lock);
        mutex_unlock(&lpc_data->access_lock);
    }

    return count;
}

/* get mb_cpld1_version register value */
static ssize_t read_mb_cpld1_version(struct device *dev,
        struct device_attribute *da, char *buf)
{
    return sprintf(buf,"%d\n", _read_lpc_reg(REG_MB_CPLD1_VERSION, MASK_ALL));
}

/* get mb_cpld1_version_h register value (human readable) */
static ssize_t read_mb_cpld1_version_h(struct device *dev,
        struct device_attribute *da, char *buf)
{
    return sprintf(buf,"%d.%02d\n", _read_lpc_reg(REG_MB_CPLD1_VERSION, MASK_CPLD_MAJOR_VER),
                                    _read_lpc_reg(REG_MB_CPLD1_VERSION, MASK_CPLD_MINOR_VER));
}

/* get lpc register value */
static ssize_t read_lpc_callback(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u16 reg = 0;
    u8 mask = MASK_ALL;

    switch (attr->index) {
        //MAC Temp
        case ATT_TEMP_J2_PM0:
            reg = REG_TEMP_J2_PM0;
            break;
        case ATT_TEMP_J2_PM1:
            reg = REG_TEMP_J2_PM1;
            break;
        case ATT_TEMP_J2_PM2:
            reg = REG_TEMP_J2_PM2;
            break;
        case ATT_TEMP_J2_PM3:
            reg = REG_TEMP_J2_PM3;
            break;
        case ATT_TEMP_OP2:
            reg = REG_TEMP_OP2;
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
        //MAC Temp
        case ATT_TEMP_J2_PM0:
            reg = REG_TEMP_J2_PM0;
            break;
        case ATT_TEMP_J2_PM1:
            reg = REG_TEMP_J2_PM1;
            break;
        case ATT_TEMP_J2_PM2:
            reg = REG_TEMP_J2_PM2;
            break;
        case ATT_TEMP_J2_PM3:
            reg = REG_TEMP_J2_PM3;
            break;
        case ATT_TEMP_OP2:
            reg = REG_TEMP_OP2;
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
    char *str=NULL;

    switch (attr->index) {
        case ATT_BSP_VERSION:
            str = bsp_version;
            break;
        case ATT_BSP_DEBUG:
            str = bsp_debug;
            break;
        case ATT_BSP_REG:
            str = bsp_reg;
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
    u8 bsp_debug_u8 = 0;

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
            if (kstrtou16(buf, 0, &reg) < 0)
                return -EINVAL;

            str = bsp_reg;
            str_len = sizeof(bsp_reg);
            break;
        default:
            return -EINVAL;
    }

    if (attr->index == ATT_BSP_DEBUG) {
        if (kstrtou8(buf, 0, &bsp_debug_u8) < 0) {
            return -EINVAL;
        } else if (_config_bsp_log(bsp_debug_u8) < 0) {
            return -EINVAL;
        }
    }

    return write_bsp(buf, str, str_len, count);
}

static ssize_t write_bsp_pr_callback(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int str_len = strlen(buf);

    if(str_len <= 0)
        return str_len;

    switch (attr->index) {
        case ATT_BSP_PR_INFO:
            BSP_PR(KERN_INFO, "%s", buf);
            break;
        case ATT_BSP_PR_ERR:
            BSP_PR(KERN_ERR, "%s", buf);
            break;
        default:
            return -EINVAL;
    }

    return str_len;
}

/* SENSOR_DEVICE_ATTR - CPU */
static SENSOR_DEVICE_ATTR(cpu_cpld_version, S_IRUGO, read_cpu_cpld_version, NULL, ATT_CPU_CPLD_VERSION);
static SENSOR_DEVICE_ATTR(cpu_cpld_version_h, S_IRUGO, read_cpu_cpld_version_h, NULL, ATT_CPU_CPLD_VERSION_H);
static SENSOR_DEVICE_ATTR(boot_rom,         S_IRUGO, read_cpu_bios_boot_rom, NULL, ATT_CPU_BIOS_BOOT_ROM);
static SENSOR_DEVICE_ATTR(mux_reset,        S_IRUGO | S_IWUSR, read_cpu_mux_reset_callback, write_cpu_mux_reset, ATT_CPU_MUX_RESET);

/* SENSOR_DEVICE_ATTR - MB */
static SENSOR_DEVICE_ATTR(mb_cpld1_version, S_IRUGO, read_mb_cpld1_version, NULL, ATT_MB_CPLD1_VERSION);
static SENSOR_DEVICE_ATTR(mb_cpld1_version_h, S_IRUGO, read_mb_cpld1_version_h, NULL, ATT_MB_CPLD1_VERSION_H);

//SENSOR_DEVICE_ATTR - BSP
static SENSOR_DEVICE_ATTR(bsp_version, S_IRUGO | S_IWUSR,  read_bsp_callback, write_bsp_callback, ATT_BSP_VERSION);
static SENSOR_DEVICE_ATTR(bsp_debug,   S_IRUGO | S_IWUSR,  read_bsp_callback, write_bsp_callback, ATT_BSP_DEBUG);
static SENSOR_DEVICE_ATTR(bsp_pr_info, S_IWUSR, NULL, write_bsp_pr_callback, ATT_BSP_PR_INFO);
static SENSOR_DEVICE_ATTR(bsp_pr_err , S_IWUSR, NULL, write_bsp_pr_callback, ATT_BSP_PR_ERR);
static SENSOR_DEVICE_ATTR(bsp_reg,     S_IRUGO | S_IWUSR, read_lpc_callback, write_bsp_callback, ATT_BSP_REG);
static SENSOR_DEVICE_ATTR(bsp_gpio_max,    S_IRUGO, read_gpio_max, NULL, ATT_BSP_GPIO_MAX);

/* SENSOR_DEVICE_ATTR - MAC Temp */
static SENSOR_DEVICE_ATTR(temp_j2_pm0, S_IRUGO | S_IWUSR, read_lpc_callback, write_lpc_callback, ATT_TEMP_J2_PM0);
static SENSOR_DEVICE_ATTR(temp_j2_pm1, S_IRUGO | S_IWUSR, read_lpc_callback, write_lpc_callback, ATT_TEMP_J2_PM1);
static SENSOR_DEVICE_ATTR(temp_j2_pm2, S_IRUGO | S_IWUSR, read_lpc_callback, write_lpc_callback, ATT_TEMP_J2_PM2);
static SENSOR_DEVICE_ATTR(temp_j2_pm3, S_IRUGO | S_IWUSR, read_lpc_callback, write_lpc_callback, ATT_TEMP_J2_PM3);
static SENSOR_DEVICE_ATTR(temp_op2,    S_IRUGO | S_IWUSR, read_lpc_callback, write_lpc_callback, ATT_TEMP_OP2);


static struct attribute *cpu_cpld_attrs[] = {
    &sensor_dev_attr_cpu_cpld_version.dev_attr.attr,
    &sensor_dev_attr_cpu_cpld_version_h.dev_attr.attr,
    &sensor_dev_attr_mux_reset.dev_attr.attr,
    NULL,
};

static struct attribute *bios_attrs[] = {
    &sensor_dev_attr_boot_rom.dev_attr.attr,
    NULL,
};

static struct attribute *mb_cpld_attrs[] = {
    &sensor_dev_attr_mb_cpld1_version.dev_attr.attr,
    &sensor_dev_attr_mb_cpld1_version_h.dev_attr.attr,
    NULL,
};

static struct attribute *bsp_attrs[] = {
    &sensor_dev_attr_bsp_version.dev_attr.attr,
    &sensor_dev_attr_bsp_debug.dev_attr.attr,
    &sensor_dev_attr_bsp_pr_info.dev_attr.attr,
    &sensor_dev_attr_bsp_pr_err.dev_attr.attr,
    &sensor_dev_attr_bsp_reg.dev_attr.attr,
    &sensor_dev_attr_bsp_gpio_max.dev_attr.attr,
    NULL,
};

static struct attribute *mac_temp_attrs[] = {
    &sensor_dev_attr_temp_j2_pm0.dev_attr.attr,
    &sensor_dev_attr_temp_j2_pm1.dev_attr.attr,
    &sensor_dev_attr_temp_j2_pm2.dev_attr.attr,
    &sensor_dev_attr_temp_j2_pm3.dev_attr.attr,
    &sensor_dev_attr_temp_op2.dev_attr.attr,
    NULL,
};

static struct attribute_group cpu_cpld_attr_grp = {
	.name = "cpu_cpld",
    .attrs = cpu_cpld_attrs,
};

static struct attribute_group bios_attr_grp = {
	.name = "bios",
    .attrs = bios_attrs,
};

static struct attribute_group mb_cpld_attr_grp = {
	.name = "mb_cpld",
    .attrs = mb_cpld_attrs,
};

static struct attribute_group bsp_attr_grp = {
    .name = "bsp",
    .attrs = bsp_attrs,
};

static struct attribute_group mac_temp_attr_grp = {
    .name = "mac_temp",
    .attrs = mac_temp_attrs,
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
    int ret = 0;

    lpc_data = devm_kzalloc(&pdev->dev, sizeof(struct lpc_data_s), GFP_KERNEL);
    if (!lpc_data)
        return -ENOMEM;

    mutex_init(&lpc_data->access_lock);

    ret = sysfs_create_group(&pdev->dev.kobj, &cpu_cpld_attr_grp);
    if (ret) {
        printk(KERN_ERR "Cannot create sysfs for group %s\n", cpu_cpld_attr_grp.name);
        return ret;
    }

    ret = sysfs_create_group(&pdev->dev.kobj, &bios_attr_grp);
    if (ret) {
        printk(KERN_ERR "Cannot create sysfs for group %s\n", bios_attr_grp.name);
        sysfs_remove_group(&pdev->dev.kobj, &cpu_cpld_attr_grp);
        return ret;
    }

    ret = sysfs_create_group(&pdev->dev.kobj, &mb_cpld_attr_grp);
    if (ret) {
        printk(KERN_ERR "Cannot create sysfs for group %s\n", mb_cpld_attr_grp.name);
        sysfs_remove_group(&pdev->dev.kobj, &cpu_cpld_attr_grp);
        sysfs_remove_group(&pdev->dev.kobj, &bios_attr_grp);
        return ret;
    }

    ret = sysfs_create_group(&pdev->dev.kobj, &bsp_attr_grp);
    if (ret) {
        printk(KERN_ERR "Cannot create sysfs for group %s\n", bsp_attr_grp.name);
        sysfs_remove_group(&pdev->dev.kobj, &cpu_cpld_attr_grp);
        sysfs_remove_group(&pdev->dev.kobj, &bios_attr_grp);
        sysfs_remove_group(&pdev->dev.kobj, &mb_cpld_attr_grp);
        return ret;
    }

    ret = sysfs_create_group(&pdev->dev.kobj, &mac_temp_attr_grp);
    if (ret) {
        printk(KERN_ERR "Cannot create sysfs for group %s\n", mac_temp_attr_grp.name);
        sysfs_remove_group(&pdev->dev.kobj, &cpu_cpld_attr_grp);
        sysfs_remove_group(&pdev->dev.kobj, &bios_attr_grp);
        sysfs_remove_group(&pdev->dev.kobj, &mb_cpld_attr_grp);
        sysfs_remove_group(&pdev->dev.kobj, &bsp_attr_grp);
        return ret;
    }

    return ret;
}

static int lpc_drv_remove(struct platform_device *pdev)
{
    sysfs_remove_group(&pdev->dev.kobj, &cpu_cpld_attr_grp);
    sysfs_remove_group(&pdev->dev.kobj, &bios_attr_grp);
    sysfs_remove_group(&pdev->dev.kobj, &mb_cpld_attr_grp);
    sysfs_remove_group(&pdev->dev.kobj, &bsp_attr_grp);
    sysfs_remove_group(&pdev->dev.kobj, &mac_temp_attr_grp);

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
    int ret = 0;

    ret = platform_driver_register(&lpc_drv);
    if (ret) {
        printk(KERN_ERR "Platform driver register failed (ret=%d)\n", ret);
        return ret;
    }

    ret = platform_device_register(&lpc_dev);
    if (ret) {
        printk(KERN_ERR "Platform device register failed (ret=%d)\n", ret);
        platform_driver_unregister(&lpc_drv);
        return ret;
    }

    return ret;
}

void lpc_exit(void)
{
    platform_driver_unregister(&lpc_drv);
    platform_device_unregister(&lpc_dev);
}

module_init(lpc_init);
module_exit(lpc_exit);

MODULE_AUTHOR("Feng Lee <feng.cf.lee@ufispace.com>");
MODULE_DESCRIPTION("x86_64_ufispace_s9700_53dx_lpc driver");
MODULE_LICENSE("GPL");
