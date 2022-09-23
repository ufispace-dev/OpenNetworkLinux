/*
 * A lpc driver for the ufispace_s9510_30xc
 *
 * Copyright (C) 2017-2020 UfiSpace Technology Corporation.
 * Nonodark Huang <nonodark.huang@ufispace.com>
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

#define DRIVER_NAME "x86_64_ufispace_s9510_30xc_lpc"

/* LPC registers */
#define REG_BASE_MB                       0x700
#define REG_BASE_EC                       0xe300

//MB CPLD
#define REG_BRD_ID_0                   (REG_BASE_MB + 0x00)
#define REG_BRD_ID_1                   (REG_BASE_MB + 0x01)
#define REG_CPLD_VERSION               (REG_BASE_MB + 0x02)
#define REG_CPLD_ID                    (REG_BASE_MB + 0x03)
#define REG_CPLD_BUILD                 (REG_BASE_MB + 0x04)
#define REG_PSU_INTR                   (REG_BASE_MB + 0x12)
#define REG_FAN_INTR                   (REG_BASE_MB + 0x16)
#define REG_PORT_INTR                  (REG_BASE_MB + 0x18)
#define REG_PSU_MASK                   (REG_BASE_MB + 0x22)
#define REG_FAN_MASK                   (REG_BASE_MB + 0x26)
#define REG_PORT_MASK                  (REG_BASE_MB + 0x28)
#define REG_MUX_RESET                  (REG_BASE_MB + 0x43)
#define REG_MAC_ROV                    (REG_BASE_MB + 0x52)
#define REG_FAN_PRESENT                (REG_BASE_MB + 0x55)
#define REG_PSU_STATUS                 (REG_BASE_MB + 0x59)
#define REG_BIOS_BOOT_SEL              (REG_BASE_MB + 0x5B)
#define REG_UART_CTRL                  (REG_BASE_MB + 0x63)
#define REG_USB_CTRL                   (REG_BASE_MB + 0x64)
#define REG_MUX_CTRL                   (REG_BASE_MB + 0x65)
#define REG_LED_CLR                    (REG_BASE_MB + 0x80)
#define REG_LED_CTRL_1                 (REG_BASE_MB + 0x81)
#define REG_LED_CTRL_2                 (REG_BASE_MB + 0x82)
#define REG_LED_STATUS_1               (REG_BASE_MB + 0x83)

//MB EC
#define REG_MISC_CTRL                  (REG_BASE_EC + 0x0C)
#define REG_CPU_REV                    (REG_BASE_EC + 0x17)

#define MASK_ALL                          (0xFF) // 2#11111111
#define MASK_CPLD_MAJOR                   (0xC0) // 2#11000000
#define MASK_CPLD_MINOR                   (0x3F) // 2#00111111
#define MASK_MUX_RESET_ALL                (0x3F) // 2#00111111

#define MDELAY_LPC                        (5)
#define MDELAY_RESET_INTERVAL             (100)
#define MDELAY_RESET_FINISH               (500)


/* LPC sysfs attributes index  */
enum lpc_sysfs_attributes {
    //MB CPLD
    ATT_BRD_ID_0,
    ATT_BRD_ID_1,
    ATT_BRD_SKU_ID,
    ATT_BRD_HW_ID,
    ATT_BRD_BUILD_ID,
    ATT_BRD_DEPH_ID,
    ATT_CPLD_VERSION,
    ATT_CPLD_VERSION_MAJOR,
    ATT_CPLD_VERSION_MINOR,
    ATT_CPLD_VERSION_H,
    ATT_CPLD_ID,
    ATT_CPLD_BUILD,
    ATT_PSU_INTR,
    ATT_FAN_INTR,
    ATT_PORT_INTR,
    ATT_PSU_MASK,
    ATT_FAN_MASK,
    ATT_PORT_MASK,
    ATT_MUX_RESET,
    ATT_MUX_RESET_ALL,
    ATT_MAC_ROV,
    ATT_FAN_PRESENT,
    ATT_FAN_PRESENT_0,
    ATT_FAN_PRESENT_1,
    ATT_FAN_PRESENT_2,
    ATT_FAN_PRESENT_3,
    ATT_FAN_PRESENT_4,
    ATT_PSU_STATUS,
    ATT_BIOS_BOOT_SEL,
    ATT_UART_CTRL,
    ATT_USB_CTRL,
    ATT_MUX_CTRL,
    ATT_LED_CLR,
    ATT_LED_CTRL_1,
    ATT_LED_CTRL_2,
    ATT_LED_STATUS_1,

    //EC
    ATT_EC_BIOS_BOOT_ROM,
    ATT_EC_CPU_REV_HW_REV,
    ATT_EC_CPU_REV_DEV_PHASE,
    ATT_EC_CPU_REV_BUILD_ID,

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

/* get lpc register value */
static u8 _read_lpc_reg(u16 reg, u8 mask)
{
    u8 reg_val=0x0, reg_mk_shf_val = 0x0;

    mutex_lock(&lpc_data->access_lock);
    reg_val = inb(reg);
    mutex_unlock(&lpc_data->access_lock);

    reg_mk_shf_val = _mask_shift(reg_val, mask);

    BSP_LOG_R("reg=0x%03x, reg_val=0x%02x, mask=0x%02x, reg_mk_shf_val=0x%02x", reg, reg_val, mask, reg_mk_shf_val);

    return reg_mk_shf_val;
}

/* get lpc register value */
static ssize_t read_lpc_reg(u16 reg, u8 mask, char *buf)
{
    u8 reg_val;
    int len=0;

    reg_val = _read_lpc_reg(reg, mask);
    len=sprintf(buf,"0x%x\n", reg_val);

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
    mdelay(MDELAY_LPC);

    mutex_unlock(&lpc_data->access_lock);

    BSP_LOG_W("reg=0x%03x, reg_val=0x%02x, mask=0x%02x", reg, reg_val, mask);

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

/* get mb cpld version in human readable format */
static ssize_t read_mb_cpld_version_h(struct device *dev,
        struct device_attribute *da, char *buf)
{
    ssize_t len=0;
    u8 major = 0, minor = 0, build = 0;
    major = _read_lpc_reg( REG_CPLD_VERSION, MASK_CPLD_MAJOR);
    minor = _read_lpc_reg( REG_CPLD_VERSION, MASK_CPLD_MINOR);
    build = _read_lpc_reg( REG_CPLD_BUILD, MASK_ALL);
    len=sprintf(buf, "%u.%02u.%03u", major, minor, build);

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
        case ATT_BRD_ID_0:
            reg = REG_BRD_ID_0;
            break;
        case ATT_BRD_ID_1:
            reg = REG_BRD_ID_1;
            break;
        case ATT_BRD_SKU_ID:
            reg = REG_BRD_ID_0;
            mask = 0x0F;
            break;
        case ATT_BRD_HW_ID:
            reg = REG_BRD_ID_1;
            mask = 0x03;
            break;
        case ATT_BRD_BUILD_ID:
            reg = REG_BRD_ID_1;
            mask = 0x18;
            break;
        case ATT_BRD_DEPH_ID:
            reg = REG_BRD_ID_1;
            mask = 0x04;
            break;
        case ATT_CPLD_VERSION:
            reg = REG_CPLD_VERSION;
            break;
        case ATT_CPLD_VERSION_MAJOR:
            reg = REG_CPLD_VERSION;
            mask = MASK_CPLD_MAJOR;
            break;
        case ATT_CPLD_VERSION_MINOR:
            reg = REG_CPLD_VERSION;
            mask = MASK_CPLD_MINOR;
            break;
        case ATT_CPLD_ID:
            reg = REG_CPLD_ID;
            break;
        case ATT_CPLD_BUILD:
            reg = REG_CPLD_BUILD;
            break;
        case ATT_PSU_INTR:
            reg = REG_PSU_INTR;
            break;
        case ATT_FAN_INTR:
            reg = REG_FAN_INTR;
            break;
        case ATT_PORT_INTR:
            reg = REG_PORT_INTR;
            break;
        case ATT_PSU_MASK:
            reg = REG_PSU_MASK;
            break;
        case ATT_FAN_MASK:
            reg = REG_FAN_MASK;
            break;
        case ATT_PORT_MASK:
            reg = REG_PORT_MASK;
            break;
        case ATT_MUX_RESET:
            reg = REG_MUX_RESET;
            break;
        case ATT_MAC_ROV:
            reg = REG_MAC_ROV;
            mask = 0x07;
            break;
        case ATT_FAN_PRESENT:
            reg = REG_FAN_PRESENT;
            break;
        case ATT_FAN_PRESENT_0:
            reg = REG_FAN_PRESENT;
            mask = 0x20;
            break;
        case ATT_FAN_PRESENT_1:
            reg = REG_FAN_PRESENT;
            mask = 0x10;
            break;
        case ATT_FAN_PRESENT_2:
            reg = REG_FAN_PRESENT;
            mask = 0x08;
            break;
        case ATT_FAN_PRESENT_3:
            reg = REG_FAN_PRESENT;
            mask = 0x04;
            break;
        case ATT_FAN_PRESENT_4:
            reg = REG_FAN_PRESENT;
            mask = 0x02;
            break;
        case ATT_PSU_STATUS:
            reg = REG_PSU_STATUS;
            break;
        case ATT_BIOS_BOOT_SEL:
            reg = REG_BIOS_BOOT_SEL;
            mask = 0x03;
            break;
        case ATT_UART_CTRL:
            reg = REG_UART_CTRL;
            mask = 0x03;
            break;
        case ATT_USB_CTRL:
            reg = REG_USB_CTRL;
            mask = 0x07;
            break;
        case ATT_MUX_CTRL:
            reg = REG_MUX_CTRL;
            mask = 0x1F;
            break;
        case ATT_LED_CLR:
            reg = REG_LED_CLR;
            mask = 0x07;
            break;
        case ATT_LED_CTRL_1:
            reg = REG_LED_CTRL_1;
            break;
        case ATT_LED_CTRL_2:
            reg = REG_LED_CTRL_2;
            break;
        case ATT_LED_STATUS_1:
            reg = REG_LED_STATUS_1;
            break;

        //EC
        case ATT_EC_BIOS_BOOT_ROM:
            reg = REG_MISC_CTRL;
            mask = 0x40; // 2#01000000
            break;
        case ATT_EC_CPU_REV_HW_REV:
            reg = REG_CPU_REV;
            mask = 0x03; // 2#00000011
            break;
        case ATT_EC_CPU_REV_DEV_PHASE:
            reg = REG_CPU_REV;
            mask = 0x04; // 2#00000100
            break;
        case ATT_EC_CPU_REV_BUILD_ID:
            reg = REG_CPU_REV;
            mask = 0x18; // 2#00011000
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
        case ATT_PSU_MASK:
            reg = REG_PSU_MASK;
            break;
        case ATT_FAN_MASK:
            reg = REG_FAN_MASK;
            break;
        case ATT_PORT_MASK:
            reg = REG_PORT_MASK;
            break;
        case ATT_MUX_RESET:
            reg = REG_MUX_RESET;
            break;
        case ATT_UART_CTRL:
            reg = REG_UART_CTRL;
            break;
        case ATT_USB_CTRL:
            reg = REG_USB_CTRL;
            break;
        case ATT_LED_CLR:
            reg = REG_LED_CLR;
            break;
        case ATT_LED_CTRL_1:
            reg = REG_LED_CTRL_1;
            break;
        case ATT_LED_CTRL_2:
            reg = REG_LED_CTRL_2;
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

/* get mux reset all register value */
static ssize_t read_mux_rest_all_callback(struct device *dev,
        struct device_attribute *da, char *buf)
{
    u8 len = 0;
    u8 reg_val = 0;

    reg_val = _read_lpc_reg(REG_MUX_RESET, MASK_ALL);

    if(reg_val > 0){
        len = scnprintf(buf,3,"%u\n", 1);
    }else{
        len = scnprintf(buf,3,"%u\n", 0);
    }
    return len;
}

/* set  mux reset all register value */
static ssize_t write_mux_reset_all_callback(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    static int mux_reset_flag = 0;
    u8 reg_val = 0;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    if(mux_reset_flag == 0)
    {
        if (reg_val == 0) {
            mutex_lock(&lpc_data->access_lock);
            mux_reset_flag = 1;

            BSP_LOG_W("i2c mux reset is triggered...");
            reg_val = inb(REG_MUX_RESET);
            outb((reg_val & (u8)(~MASK_MUX_RESET_ALL)), REG_MUX_RESET);
            BSP_LOG_W("reg=0x%03x, reg_val=0x%02x", REG_MUX_RESET, reg_val & (u8)(~MASK_MUX_RESET_ALL));

            mdelay(MDELAY_RESET_INTERVAL);

            outb((reg_val | MASK_MUX_RESET_ALL), REG_MUX_RESET);
            BSP_LOG_W("reg=0x%03x, reg_val=0x%02x", REG_MUX_RESET, (reg_val | MASK_MUX_RESET_ALL));
            mdelay(MDELAY_RESET_FINISH);
            mux_reset_flag = 0;
            mutex_unlock(&lpc_data->access_lock);
        } else {
            return -EINVAL;
        }
    }else {
        BSP_LOG_W("i2c mux is resetting... (ignore)");
        mutex_lock(&lpc_data->access_lock);
        mutex_unlock(&lpc_data->access_lock);
    }
    return count;
}

//SENSOR_DEVICE_ATTR - MB
static SENSOR_DEVICE_ATTR(board_id_0          , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_BRD_ID_0);
static SENSOR_DEVICE_ATTR(board_id_1          , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_BRD_ID_1);
static SENSOR_DEVICE_ATTR(board_sku_id        , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_BRD_SKU_ID);
static SENSOR_DEVICE_ATTR(board_hw_id         , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_BRD_HW_ID);
static SENSOR_DEVICE_ATTR(board_build_id      , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_BRD_BUILD_ID);
static SENSOR_DEVICE_ATTR(board_deph_id       , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_BRD_DEPH_ID);
static SENSOR_DEVICE_ATTR(cpld_version        , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_CPLD_VERSION);
static SENSOR_DEVICE_ATTR(cpld_version_major  , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_CPLD_VERSION_MAJOR);
static SENSOR_DEVICE_ATTR(cpld_version_minor  , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_CPLD_VERSION_MINOR);
static SENSOR_DEVICE_ATTR(cpld_version_h      , S_IRUGO          , read_mb_cpld_version_h    , NULL                        , ATT_CPLD_VERSION_H);
static SENSOR_DEVICE_ATTR(cpld_id             , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_CPLD_ID);
static SENSOR_DEVICE_ATTR(cpld_build          , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_CPLD_BUILD);
static SENSOR_DEVICE_ATTR(psu_intr            , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_PSU_INTR);
static SENSOR_DEVICE_ATTR(fan_intr            , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_FAN_INTR);
static SENSOR_DEVICE_ATTR(port_intr           , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_PORT_INTR);
static SENSOR_DEVICE_ATTR(psu_mask            , S_IRUGO | S_IWUSR, read_lpc_callback         , write_lpc_callback          , ATT_PSU_MASK);
static SENSOR_DEVICE_ATTR(fan_mask            , S_IRUGO | S_IWUSR, read_lpc_callback         , write_lpc_callback          , ATT_FAN_MASK);
static SENSOR_DEVICE_ATTR(port_mask           , S_IRUGO | S_IWUSR, read_lpc_callback         , write_lpc_callback          , ATT_PORT_MASK);
static SENSOR_DEVICE_ATTR(mux_reset           , S_IRUGO | S_IWUSR, read_lpc_callback         , write_lpc_callback          , ATT_MUX_RESET);
static SENSOR_DEVICE_ATTR(mux_reset_all       , S_IRUGO | S_IWUSR, read_mux_rest_all_callback, write_mux_reset_all_callback, ATT_MUX_RESET_ALL);
static SENSOR_DEVICE_ATTR(mac_rov             , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_MAC_ROV);
static SENSOR_DEVICE_ATTR(fan_present         , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_FAN_PRESENT);
static SENSOR_DEVICE_ATTR(fan_present_0       , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_FAN_PRESENT_0);
static SENSOR_DEVICE_ATTR(fan_present_1       , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_FAN_PRESENT_1);
static SENSOR_DEVICE_ATTR(fan_present_2       , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_FAN_PRESENT_2);
static SENSOR_DEVICE_ATTR(fan_present_3       , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_FAN_PRESENT_3);
static SENSOR_DEVICE_ATTR(fan_present_4       , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_FAN_PRESENT_4);
static SENSOR_DEVICE_ATTR(psu_status          , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_PSU_STATUS);
static SENSOR_DEVICE_ATTR(uart_ctrl           , S_IRUGO | S_IWUSR, read_lpc_callback         , write_lpc_callback          , ATT_UART_CTRL);
static SENSOR_DEVICE_ATTR(usb_ctrl            , S_IRUGO | S_IWUSR, read_lpc_callback         , write_lpc_callback          , ATT_USB_CTRL);
static SENSOR_DEVICE_ATTR(mux_ctrl            , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_MUX_CTRL);
static SENSOR_DEVICE_ATTR(led_clr             , S_IRUGO | S_IWUSR, read_lpc_callback         , write_lpc_callback          , ATT_LED_CLR);
static SENSOR_DEVICE_ATTR(led_ctrl_1          , S_IRUGO | S_IWUSR, read_lpc_callback         , write_lpc_callback          , ATT_LED_CTRL_1);
static SENSOR_DEVICE_ATTR(led_ctrl_2          , S_IRUGO | S_IWUSR, read_lpc_callback         , write_lpc_callback          , ATT_LED_CTRL_2);
static SENSOR_DEVICE_ATTR(led_status_1        , S_IRUGO          , read_lpc_callback         , NULL                        , ATT_LED_STATUS_1);

//SENSOR_DEVICE_ATTR - EC
static SENSOR_DEVICE_ATTR(bios_boot_sel       , S_IRUGO          , read_lpc_callback         , NULL                         , ATT_EC_BIOS_BOOT_ROM);
static SENSOR_DEVICE_ATTR(cpu_rev_hw_rev      , S_IRUGO          , read_lpc_callback         , NULL                         , ATT_EC_CPU_REV_HW_REV);
static SENSOR_DEVICE_ATTR(cpu_rev_dev_phase   , S_IRUGO          , read_lpc_callback         , NULL                         , ATT_EC_CPU_REV_DEV_PHASE);
static SENSOR_DEVICE_ATTR(cpu_rev_build_id    , S_IRUGO          , read_lpc_callback         , NULL                         , ATT_EC_CPU_REV_BUILD_ID);

//SENSOR_DEVICE_ATTR - BSP
static SENSOR_DEVICE_ATTR(bsp_version , S_IRUGO | S_IWUSR, read_bsp_callback, write_bsp_callback             , ATT_BSP_VERSION);
static SENSOR_DEVICE_ATTR(bsp_debug   , S_IRUGO | S_IWUSR, read_bsp_callback, write_bsp_callback             , ATT_BSP_DEBUG);
static SENSOR_DEVICE_ATTR(bsp_pr_info , S_IWUSR          , NULL             , write_bsp_pr_callback          , ATT_BSP_PR_INFO);
static SENSOR_DEVICE_ATTR(bsp_pr_err  , S_IWUSR          , NULL             , write_bsp_pr_callback          , ATT_BSP_PR_ERR);
static SENSOR_DEVICE_ATTR(bsp_reg     , S_IRUGO | S_IWUSR, read_lpc_callback, write_bsp_callback             , ATT_BSP_REG);

static struct attribute *mb_cpld_attrs[] = {
    &sensor_dev_attr_board_id_0.dev_attr.attr,
    &sensor_dev_attr_board_id_1.dev_attr.attr,
    &sensor_dev_attr_board_sku_id.dev_attr.attr,
    &sensor_dev_attr_board_hw_id.dev_attr.attr,
    &sensor_dev_attr_board_build_id.dev_attr.attr,
    &sensor_dev_attr_board_deph_id.dev_attr.attr,
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_cpld_version_major.dev_attr.attr,
    &sensor_dev_attr_cpld_version_minor.dev_attr.attr,
    &sensor_dev_attr_cpld_version_h.dev_attr.attr,
    &sensor_dev_attr_cpld_id.dev_attr.attr,
    &sensor_dev_attr_cpld_build.dev_attr.attr,
    &sensor_dev_attr_psu_intr.dev_attr.attr,
    &sensor_dev_attr_fan_intr.dev_attr.attr,
    &sensor_dev_attr_port_intr.dev_attr.attr,
    &sensor_dev_attr_psu_mask.dev_attr.attr,
    &sensor_dev_attr_fan_mask.dev_attr.attr,
    &sensor_dev_attr_port_mask.dev_attr.attr,
    &sensor_dev_attr_mux_reset.dev_attr.attr,
    &sensor_dev_attr_mux_reset_all.dev_attr.attr,
    &sensor_dev_attr_mac_rov.dev_attr.attr,
    &sensor_dev_attr_fan_present.dev_attr.attr,
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
    &sensor_dev_attr_led_status_1.dev_attr.attr,
    NULL,
};

static struct attribute *bios_attrs[] = {
    &sensor_dev_attr_bios_boot_sel.dev_attr.attr,
    NULL,
};

static struct attribute *ec_attrs[] = {
    &sensor_dev_attr_cpu_rev_hw_rev.dev_attr.attr,
    &sensor_dev_attr_cpu_rev_dev_phase.dev_attr.attr,
    &sensor_dev_attr_cpu_rev_build_id.dev_attr.attr,
    NULL,
};

static struct attribute *bsp_attrs[] = {
    &sensor_dev_attr_bsp_version.dev_attr.attr,
    &sensor_dev_attr_bsp_debug.dev_attr.attr,
    &sensor_dev_attr_bsp_pr_info.dev_attr.attr,
    &sensor_dev_attr_bsp_pr_err.dev_attr.attr,
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

static struct attribute_group ec_attr_grp = {
    .name = "ec",
    .attrs = ec_attrs,
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
    int i = 0, grp_num = 4;
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
            case 3:
                grp = &ec_attr_grp;
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
            case 3:
                grp = &ec_attr_grp;
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
    sysfs_remove_group(&pdev->dev.kobj, &ec_attr_grp);
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

MODULE_AUTHOR("Nonodark Huang <nonodark.huang@ufispace.com>");
MODULE_DESCRIPTION("x86_64_ufispace_s9510_30xc_lpc driver");
MODULE_LICENSE("GPL");

module_init(lpc_init);
module_exit(lpc_exit);
