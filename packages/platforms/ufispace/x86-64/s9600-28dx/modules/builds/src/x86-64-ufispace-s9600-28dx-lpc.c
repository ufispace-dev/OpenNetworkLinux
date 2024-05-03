/*
 * A lpc driver for the ufispace_s9600_28dx
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

#define BSP_LOG_R(fmt, args...) \
    _bsp_log (LOG_READ, KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
            __FILE__, __func__, __LINE__, ##args)
#define BSP_LOG_W(fmt, args...) \
    _bsp_log (LOG_WRITE, KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
            __FILE__, __func__, __LINE__, ##args)
#define BSP_PR(level, fmt, args...) _bsp_log (LOG_SYS, level "[BSP]" fmt "\r\n", ##args)

#define _SENSOR_DEVICE_ATTR_RO(_name, _func, _index)     \
    SENSOR_DEVICE_ATTR(_name, S_IRUGO, read_##_func, NULL, _index)

#define _SENSOR_DEVICE_ATTR_WO(_name, _func, _index)     \
    SENSOR_DEVICE_ATTR(_name, S_IWUSR, NULL, write_##_func, _index)

#define _SENSOR_DEVICE_ATTR_RW(_name, _func, _index)     \
    SENSOR_DEVICE_ATTR(_name, S_IRUGO | S_IWUSR, read_##_func, write_##_func, _index)

#define _DEVICE_ATTR(_name)     \
    &sensor_dev_attr_##_name.dev_attr.attr

#define DRIVER_NAME "x86_64_ufispace_s9600_28dx_lpc"
#define CPU_BDE 0
#define CPU_SKY 1
#define CPU_ICE 2
#define CPU_TYPE CPU_ICE

/* LPC registers */

#define REG_BASE_CPU                      0x600

#if CPU_TYPE == CPU_SKY || (CPU_TYPE == CPU_ICE)
#define REG_BASE_MB                       0xE00
#define REG_BASE_I2C_ALERT                0x700
#else
#define REG_BASE_MB                       0x700
#define REG_BASE_I2C_ALERT                0xF000
#endif

//CPU CPLD
#define REG_CPU_CPLD_VERSION              (REG_BASE_CPU + 0x00)
#define REG_CPU_STATUS_0                  (REG_BASE_CPU + 0x01)
#define REG_CPU_STATUS_1                  (REG_BASE_CPU + 0x02)
#define REG_CPU_CTRL_0                    (REG_BASE_CPU + 0x03)
#define REG_CPU_CTRL_1                    (REG_BASE_CPU + 0x04)
#define REG_CPU_CPLD_BUILD                (REG_BASE_CPU + 0xE0)

//MB CPLD
#define REG_MB_BRD_ID_0                   (REG_BASE_MB + 0x00)
#define REG_MB_BRD_ID_1                   (REG_BASE_MB + 0x01)
#define REG_MB_CPLD_VERSION               (REG_BASE_MB + 0x02)
#define REG_MB_CPLD_BUILD                 (REG_BASE_MB + 0x04)
#define REG_MB_MUX_RESET_1                (REG_BASE_MB + 0x46)
#define REG_MB_MUX_RESET_2                (REG_BASE_MB + 0x47)
#define REG_MB_MISC_RESET                 (REG_BASE_MB + 0x48)
//FIXME
#define REG_MB_MUX_CTRL                   (REG_BASE_MB + 0x45)

//I2C Alert
#if (CPU_TYPE == CPU_SKY) || (CPU_TYPE == CPU_ICE)
#define REG_ALERT_STATUS                  (REG_BASE_I2C_ALERT + 0x80)
#else
#define REG_ALERT_STATUS                  (REG_BASE_I2C_ALERT + 0x00)
#define REG_ALERT_DISABLE                 (REG_BASE_I2C_ALERT + 0x11)
#endif

#define MASK_ALL                          (0xFF)
#define MASK_CPLD_MAJOR_VER               (0b11000000)
#define MASK_CPLD_MINOR_VER               (0b00111111)
#define MASK_MB_MUX_RESET_1               (0b11111111)
#define MASK_MB_MUX_RESET_2               (0b00110011)
#define LPC_MDELAY                        (5)

/* LPC sysfs attributes index  */
enum lpc_sysfs_attributes {
    //CPU CPLD
    ATT_CPU_CPLD_VERSION,
    ATT_CPU_CPLD_VERSION_H,
    ATT_CPU_BIOS_BOOT_ROM,
    ATT_CPU_BIOS_BOOT_CFG,
    ATT_CPU_CPLD_BUILD,

    ATT_CPU_CPLD_MAJOR_VER,
    ATT_CPU_CPLD_MINOR_VER,
    ATT_CPU_CPLD_BUILD_VER,

    //MB CPLD
    ATT_MB_BRD_ID_0,
    ATT_MB_BRD_ID_1,
    ATT_MB_CPLD_1_VERSION,
    ATT_MB_CPLD_1_VERSION_H,
    ATT_MB_CPLD_1_BUILD,
    ATT_MB_MUX_CTRL,
    ATT_MB_BRD_SKU_ID,
    ATT_MB_BRD_HW_ID,
    ATT_MB_BRD_ID_TYPE,
    ATT_MB_BRD_BUILD_ID,
    ATT_MB_BRD_DEPH_ID,
    ATT_MB_MUX_RESET,

    ATT_MB_CPLD_1_MAJOR_VER,
    ATT_MB_CPLD_1_MINOR_VER,
    ATT_MB_CPLD_1_BUILD_VER,
    //I2C Alert
    ATT_ALERT_STATUS,
#if CPU_TYPE == CPU_BDE
    ATT_ALERT_DISABLE,
#endif
    //BSP
    ATT_BSP_VERSION,
    ATT_BSP_DEBUG,
    ATT_BSP_PR_INFO,
    ATT_BSP_PR_ERR,
    ATT_BSP_REG,
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
u8 mailbox_inited=0;

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

/* init bmc mailbox */
/*
static int init_bmc_mailbox(void)
{
    if (mailbox_inited) {
        return mailbox_inited;
    }

    //Enable super io writing
    _outb(0xa5, 0x2e);
    _outb(0xa5, 0x2e);

    //Logic device number
    _outb(0x07, 0x2e);
    _outb(0x0e, 0x2f);

    //Disable mailbox
    _outb(0x30, 0x2e);
    _outb(0x00, 0x2f);

    //Set base address bit
    _outb(0x60, 0x2e);
    _outb(0x0e, 0x2f);
    _outb(0x61, 0x2e);
    _outb(0xc0, 0x2f);

    //Select bit[3:0] of SIRQ
    _outb(0x70, 0x2e);
    _outb(0x07, 0x2f);

    //Low level trigger
    _outb(0x71, 0x2e);
    _outb(0x01, 0x2f);

    //Enable mailbox
    _outb(0x30, 0x2e);
    _outb(0x01, 0x2f);

    //Disable super io writing
    _outb(0xaa, 0x2e);

    //Mailbox initial
    _outb(0x00, 0xed6);
    _outb(0x00, 0xed7);

    //set mailbox_inited
    mailbox_inited = 1;

    return mailbox_inited;
}
*/

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

/* get cpu cpld version in human readable format */
static ssize_t read_cpu_cpld_version_h(struct device *dev,
        struct device_attribute *da, char *buf)
{
    ssize_t len=0;
    len=sprintf(buf, "%d.%02d.%03d", _read_lpc_reg(REG_CPU_CPLD_VERSION, MASK_CPLD_MAJOR_VER),
                                     _read_lpc_reg(REG_CPU_CPLD_VERSION, MASK_CPLD_MINOR_VER),
                                     _read_lpc_reg(REG_CPU_CPLD_BUILD, MASK_ALL));

    return len;
}

/* get mb cpld version in human readable format */
static ssize_t read_mb_cpld_1_version_h(struct device *dev,
        struct device_attribute *da, char *buf)
{
    ssize_t len=0;
    len=sprintf(buf, "%d.%02d.%03d", _read_lpc_reg(REG_MB_CPLD_VERSION, MASK_CPLD_MAJOR_VER),
                                     _read_lpc_reg(REG_MB_CPLD_VERSION, MASK_CPLD_MINOR_VER),
                                     _read_lpc_reg(REG_MB_CPLD_BUILD, MASK_ALL));

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
        //CPU CPLD
        case ATT_CPU_CPLD_VERSION:
            reg = REG_CPU_CPLD_VERSION;
            break;
        case ATT_CPU_BIOS_BOOT_ROM:
            reg = REG_CPU_STATUS_1;
            mask = 0x80;
            break;
        case ATT_CPU_BIOS_BOOT_CFG:
            reg = REG_CPU_CTRL_1;
            mask = 0x80;
            break;
        case ATT_CPU_CPLD_BUILD:
            reg = REG_CPU_CPLD_BUILD;
            break;
        case ATT_CPU_CPLD_MAJOR_VER:
            reg = REG_CPU_CPLD_VERSION;
            mask = MASK_CPLD_MAJOR_VER;
            break;
        case ATT_CPU_CPLD_MINOR_VER:
            reg = REG_CPU_CPLD_VERSION;
            mask = MASK_CPLD_MINOR_VER;
            break;
        case ATT_CPU_CPLD_BUILD_VER:
            reg = REG_CPU_CPLD_BUILD;
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
        case ATT_MB_CPLD_1_BUILD:
            reg = REG_MB_CPLD_BUILD;
            break;
        case ATT_MB_BRD_SKU_ID:
            reg = REG_MB_BRD_ID_0;
            mask = 0xFF;
            break;
        case ATT_MB_BRD_HW_ID:
            reg = REG_MB_BRD_ID_1;
            mask = 0x03;
            break;
        case ATT_MB_BRD_ID_TYPE:
            reg = REG_MB_BRD_ID_1;
            mask = 0x80;
            break;
        case ATT_MB_BRD_BUILD_ID:
            reg = REG_MB_BRD_ID_1;
            mask = 0x38;
            break;
        case ATT_MB_BRD_DEPH_ID:
            reg = REG_MB_BRD_ID_1;
            mask = 0x04;
            break;
        case ATT_MB_MUX_CTRL:
            reg = REG_MB_MUX_CTRL;
            break;
        case ATT_MB_CPLD_1_MAJOR_VER:
            reg = REG_MB_CPLD_VERSION;
            mask = MASK_CPLD_MAJOR_VER;
            break;
        case ATT_MB_CPLD_1_MINOR_VER:
            reg = REG_MB_CPLD_VERSION;
            mask = MASK_CPLD_MINOR_VER;
            break;
        case ATT_MB_CPLD_1_BUILD_VER:
            reg = REG_MB_CPLD_BUILD;
            break;
        //I2C Alert
        case ATT_ALERT_STATUS:
            reg = REG_ALERT_STATUS;
            mask = 0x20;
            break;
#if CPU_TYPE == CPU_BDE
        case ATT_ALERT_DISABLE:
            reg = REG_ALERT_DISABLE;
            mask = 0x04;
            break;
#endif
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

/* set mux_reset register value */
static ssize_t write_mux_reset(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    u8 val = 0;
    u8 mux_reset_reg_val = 0;
    u8 misc_reset_reg_val = 0;
    static int mux_reset_flag = 0;

    if (kstrtou8(buf, 0, &val) < 0)
        return -EINVAL;

    if (mux_reset_flag == 0) {
        if (val == 0) {
            mutex_lock(&lpc_data->access_lock);
            mux_reset_flag = 1;
            BSP_LOG_W("i2c mux reset is triggered...");

            //reset mux on QSFP/QSFPDD ports
            mux_reset_reg_val = inb(REG_MB_MUX_RESET_1);
            _outb((u8)(mux_reset_reg_val & ~MASK_MB_MUX_RESET_1), REG_MB_MUX_RESET_1);
            BSP_LOG_W("reg=0x%03x, reg_val=0x%02x", REG_MB_MUX_RESET_1, mux_reset_reg_val & ~MASK_MB_MUX_RESET_1);

            mux_reset_reg_val = inb(REG_MB_MUX_RESET_2);
            _outb((mux_reset_reg_val & ~MASK_MB_MUX_RESET_2), REG_MB_MUX_RESET_2);
            BSP_LOG_W("reg=0x%03x, reg_val=0x%02x", REG_MB_MUX_RESET_2, mux_reset_reg_val & ~MASK_MB_MUX_RESET_2);

            //unset mux on QSFP/QSFPDD ports
            _outb((mux_reset_reg_val | MASK_MB_MUX_RESET_1), REG_MB_MUX_RESET_1);
            BSP_LOG_W("reg=0x%03x, reg_val=0x%02x", REG_MB_MUX_RESET_1, misc_reset_reg_val | MASK_MB_MUX_RESET_1);

            _outb((mux_reset_reg_val | MASK_MB_MUX_RESET_2), REG_MB_MUX_RESET_2);
            BSP_LOG_W("reg=0x%03x, reg_val=0x%02x", REG_MB_MUX_RESET_2, misc_reset_reg_val | MASK_MB_MUX_RESET_2);

            mux_reset_flag = 0;
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

//SENSOR_DEVICE_ATTR - CPU
static _SENSOR_DEVICE_ATTR_RO(cpu_cpld_version,   lpc_callback, ATT_CPU_CPLD_VERSION);
static _SENSOR_DEVICE_ATTR_RO(cpu_cpld_version_h, cpu_cpld_version_h, ATT_CPU_CPLD_VERSION_H);
static _SENSOR_DEVICE_ATTR_RO(boot_rom,           lpc_callback, ATT_CPU_BIOS_BOOT_ROM);
static _SENSOR_DEVICE_ATTR_RO(boot_cfg,           lpc_callback, ATT_CPU_BIOS_BOOT_CFG);
static _SENSOR_DEVICE_ATTR_RO(cpu_cpld_build,     lpc_callback, ATT_CPU_CPLD_BUILD);

static _SENSOR_DEVICE_ATTR_RO(cpu_cpld_major_ver, lpc_callback, ATT_CPU_CPLD_MAJOR_VER);
static _SENSOR_DEVICE_ATTR_RO(cpu_cpld_minor_ver, lpc_callback, ATT_CPU_CPLD_MINOR_VER);
static _SENSOR_DEVICE_ATTR_RO(cpu_cpld_build_ver, lpc_callback, ATT_CPU_CPLD_BUILD_VER);

//SENSOR_DEVICE_ATTR - MB
static _SENSOR_DEVICE_ATTR_RO(board_id_0,          lpc_callback, ATT_MB_BRD_ID_0);
static _SENSOR_DEVICE_ATTR_RO(board_id_1,          lpc_callback, ATT_MB_BRD_ID_1);
static _SENSOR_DEVICE_ATTR_RO(mb_cpld_1_version,   lpc_callback, ATT_MB_CPLD_1_VERSION);
static _SENSOR_DEVICE_ATTR_RO(mb_cpld_1_version_h, mb_cpld_1_version_h, ATT_MB_CPLD_1_VERSION_H);
static _SENSOR_DEVICE_ATTR_RO(mb_cpld_1_build,     lpc_callback, ATT_MB_CPLD_1_BUILD);
static _SENSOR_DEVICE_ATTR_RW(mux_ctrl,            lpc_callback, ATT_MB_MUX_CTRL);
static _SENSOR_DEVICE_ATTR_RO(board_sku_id,        lpc_callback, ATT_MB_BRD_SKU_ID);
static _SENSOR_DEVICE_ATTR_RO(board_hw_id,         lpc_callback, ATT_MB_BRD_HW_ID);
static _SENSOR_DEVICE_ATTR_RO(board_id_type,       lpc_callback, ATT_MB_BRD_ID_TYPE);
static _SENSOR_DEVICE_ATTR_RO(board_build_id,      lpc_callback, ATT_MB_BRD_BUILD_ID);
static _SENSOR_DEVICE_ATTR_RO(board_deph_id,     lpc_callback, ATT_MB_BRD_DEPH_ID);
static _SENSOR_DEVICE_ATTR_WO(mux_reset,           mux_reset, ATT_MB_MUX_RESET);

static _SENSOR_DEVICE_ATTR_RO(mb_cpld_1_major_ver, lpc_callback, ATT_MB_CPLD_1_MAJOR_VER);
static _SENSOR_DEVICE_ATTR_RO(mb_cpld_1_minor_ver, lpc_callback, ATT_MB_CPLD_1_MINOR_VER);
static _SENSOR_DEVICE_ATTR_RO(mb_cpld_1_build_ver, lpc_callback, ATT_MB_CPLD_1_BUILD_VER);

//SENSOR_DEVICE_ATTR - I2C Alert
static _SENSOR_DEVICE_ATTR_RO(alert_status,  lpc_callback, ATT_ALERT_STATUS);
#if CPU_TYPE == CPU_BDE
static _SENSOR_DEVICE_ATTR_RO(alert_disable, lpc_callback, ATT_ALERT_DISABLE);
#endif
//SENSOR_DEVICE_ATTR - BSP
static _SENSOR_DEVICE_ATTR_RW(bsp_version, bsp_callback, ATT_BSP_VERSION);
static _SENSOR_DEVICE_ATTR_RW(bsp_debug,   bsp_callback, ATT_BSP_DEBUG);
static _SENSOR_DEVICE_ATTR_WO(bsp_pr_info, bsp_pr_callback, ATT_BSP_PR_INFO);
static _SENSOR_DEVICE_ATTR_WO(bsp_pr_err , bsp_pr_callback, ATT_BSP_PR_ERR);
static SENSOR_DEVICE_ATTR(bsp_reg,         S_IRUGO | S_IWUSR, read_lpc_callback, write_bsp_callback, ATT_BSP_REG);

static struct attribute *cpu_cpld_attrs[] = {
    _DEVICE_ATTR(cpu_cpld_version),
    _DEVICE_ATTR(cpu_cpld_version_h),
    _DEVICE_ATTR(cpu_cpld_build),
    _DEVICE_ATTR(cpu_cpld_major_ver),
    _DEVICE_ATTR(cpu_cpld_minor_ver),
    _DEVICE_ATTR(cpu_cpld_build_ver),
    NULL,
};

static struct attribute *mb_cpld_attrs[] = {
    _DEVICE_ATTR(mb_cpld_1_version),
    _DEVICE_ATTR(mb_cpld_1_version_h),
    _DEVICE_ATTR(mb_cpld_1_build),
    _DEVICE_ATTR(mb_cpld_1_major_ver),
    _DEVICE_ATTR(mb_cpld_1_minor_ver),
    _DEVICE_ATTR(mb_cpld_1_build_ver),
    _DEVICE_ATTR(board_id_0),
    _DEVICE_ATTR(board_id_1),
    _DEVICE_ATTR(board_sku_id),
    _DEVICE_ATTR(board_hw_id),
    _DEVICE_ATTR(board_id_type),
    _DEVICE_ATTR(board_build_id),
    _DEVICE_ATTR(board_deph_id),
    _DEVICE_ATTR(mux_ctrl),
    _DEVICE_ATTR(mux_reset),
    NULL,
};

static struct attribute *bios_attrs[] = {
    _DEVICE_ATTR(boot_rom),
    _DEVICE_ATTR(boot_cfg),
    NULL,
};

static struct attribute *i2c_alert_attrs[] = {
    _DEVICE_ATTR(alert_status),
#if CPU_TYPE == CPU_BDE
    _DEVICE_ATTR(alert_disable),
#endif
    NULL,
};

static struct attribute *bsp_attrs[] = {
    _DEVICE_ATTR(bsp_version),
    _DEVICE_ATTR(bsp_debug),
    _DEVICE_ATTR(bsp_pr_info),
    _DEVICE_ATTR(bsp_pr_err),
    _DEVICE_ATTR(bsp_reg),
    NULL,
};

static struct attribute_group cpu_cpld_attr_grp = {
    .name = "cpu_cpld",
    .attrs = cpu_cpld_attrs,
};

static struct attribute_group mb_cpld_attr_grp = {
    .name = "mb_cpld",
    .attrs = mb_cpld_attrs,
};

static struct attribute_group bios_attr_grp = {
    .name = "bios",
    .attrs = bios_attrs,
};

static struct attribute_group i2c_alert_attr_grp = {
    .name = "i2c_alert",
    .attrs = i2c_alert_attrs,
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
    int i = 0, grp_num = 5;
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
                grp = &cpu_cpld_attr_grp;
                break;
            case 1:
                grp = &mb_cpld_attr_grp;
                break;
            case 2:
                grp = &bios_attr_grp;
            	break;
            case 3:
            	grp = &i2c_alert_attr_grp;
            	break;
            case 4:
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
                grp = &cpu_cpld_attr_grp;
                break;
            case 1:
            	grp = &mb_cpld_attr_grp;
                break;
            case 2:
            	grp = &bios_attr_grp;
            	break;
            case 3:
            	grp = &i2c_alert_attr_grp;
            	break;
            case 4:
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
    sysfs_remove_group(&pdev->dev.kobj, &cpu_cpld_attr_grp);
    sysfs_remove_group(&pdev->dev.kobj, &mb_cpld_attr_grp);
    sysfs_remove_group(&pdev->dev.kobj, &bios_attr_grp);
    sysfs_remove_group(&pdev->dev.kobj, &i2c_alert_attr_grp);
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

    err = platform_driver_register(&lpc_drv);
    if (err) {
    	printk(KERN_ERR "%s(#%d): platform_driver_register failed(%d)\n",
                __func__, __LINE__, err);

    	return err;
    }

    err = platform_device_register(&lpc_dev);
    if (err) {
    	printk(KERN_ERR "%s(#%d): platform_device_register failed(%d)\n",
                __func__, __LINE__, err);
    	platform_driver_unregister(&lpc_drv);
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
MODULE_DESCRIPTION("x86_64_ufispace_s9600_28dx_lpc driver");
MODULE_LICENSE("GPL");

module_init(lpc_init);
module_exit(lpc_exit);