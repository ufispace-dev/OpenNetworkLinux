/*
 * A lpc driver for the ufispace_s9520_28xc
 *
 * Copyright (C) 2025 UfiSpace Technology Corporation.
 * Zack Yen <zack.yen@ufispace.com>
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
#include <linux/version.h>
#include <linux/i2c.h>

#if !defined(SENSOR_DEVICE_ATTR_RO)
#define SENSOR_DEVICE_ATTR_RO(_name, _func, _index)		\
	SENSOR_DEVICE_ATTR(_name, 0444, _func##_show, NULL, _index)
#endif

#if !defined(SENSOR_DEVICE_ATTR_RW)
#define SENSOR_DEVICE_ATTR_RW(_name, _func, _index)		\
	SENSOR_DEVICE_ATTR(_name, 0644, _func##_show, _func##_store, _index)

#endif

#if !defined(SENSOR_DEVICE_ATTR_WO)
#define SENSOR_DEVICE_ATTR_WO(_name, _func, _index)		\
	SENSOR_DEVICE_ATTR(_name, 0200, NULL, _func##_store, _index)
#endif

#define BSP_LOG_R(fmt, args...) \
    _bsp_log (LOG_READ, KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
            __FILE__, __func__, __LINE__, ##args)
#define BSP_LOG_W(fmt, args...) \
    _bsp_log (LOG_WRITE, KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
            __FILE__, __func__, __LINE__, ##args)

#define _DEVICE_ATTR(_name)     \
    &sensor_dev_attr_##_name.dev_attr.attr

#define BSP_PR(level, fmt, args...) _bsp_log (LOG_SYS, level "[BSP]" fmt "\r\n", ##args)

#define DRIVER_NAME "x86_64_ufispace_s9520_28xc_lpc"

/* LPC registers */
#define REG_BASE_MB                       0x700
#define REG_BASE_EC                       0xE300

#define REG_NONE                          0xFFF

/* LPC WRITE PROTECT */
#define REG_LPC_WRITE_PROTECT             (REG_BASE_MB + 0x70)
#define MASK_LPC_WP_ENABLE                (1 << 0)

//CPLD1
#define BRD_SKU_ID_REG                    (REG_BASE_MB + 0x00)
#define BRD_HW_BUILD_REV_REG              (REG_BASE_MB + 0x01)
#define CPLD_VERSION_REG                  (REG_BASE_MB + 0x02)
#define CPLD_ID_REG                       (REG_BASE_MB + 0x03)
#define CPLD_BUILD_REG                    (REG_BASE_MB + 0x04)
#define CPLD_CHIP_TYPE_REG                (REG_BASE_MB + 0x05)
#define I2C_MUX_RESET_REG                 (REG_BASE_MB + 0x46)
#define MUX_CTRL_REG                      (REG_BASE_MB + 0x5C)

//MB EC
#define REG_MISC_CTRL                     (REG_BASE_EC + 0x0C)
#define REG_EC_CPU_SKU_ID                 (REG_BASE_EC + 0x16)
#define REG_EC_CPU_REV_ID                 (REG_BASE_EC + 0x17)
#define REG_EC_MAJOR_VER                  (REG_BASE_EC + 0x1D)
#define REG_EC_MINOR_VER                  (REG_BASE_EC + 0x1E)
#define REG_EC_BUILD_VER                  (REG_BASE_EC + 0x1F)

#define MASK_ALL                          (0xFF)
#define MASK_NONE                         (0x00)
#define MASK_0000_0001                    (0x01)
#define MASK_0000_0010                    (0x02)
#define MASK_0000_0011                    (0x03)
#define MASK_0000_0100                    (0x04)
#define MASK_0000_0110                    (0x06)
#define MASK_0000_0111                    (0x07)
#define MASK_0000_1000                    (0x08)
#define MASK_0000_1011                    (0x0B)
#define MASK_0000_1100                    (0x0C)
#define MASK_0000_1111                    (0x0F)
#define MASK_0001_0000                    (0x10)
#define MASK_0001_1111                    (0x1F)
#define MASK_0001_1000                    (0x18)
#define MASK_0010_0000                    (0x20)
#define MASK_0011_0000                    (0x30)
#define MASK_0011_1000                    (0x38)
#define MASK_0011_1001                    (0x39)
#define MASK_0011_1111                    (0x3F)
#define MASK_0100_0000                    (0x40)
#define MASK_0110_0111                    (0x67)
#define MASK_0111_0000                    (0x70)
#define MASK_0111_1111                    (0x7F)
#define MASK_1000_0000                    (0x80)
#define MASK_1000_0001                    (0x81)
#define MASK_1100_0000                    (0xC0)
#define MASK_1111_0000                    (0xF0)

#define LPC_MDELAY                        (5)
#define MDELAY_RESET_INTERVAL             (100)
#define MDELAY_RESET_FINISH               (500)

#define MUX_RESET_TRIGGER_VAL             (1)

/* LPC sysfs attributes index  */
enum lpc_sysfs_attributes {
    // CPLD Common
    CPLD_MAJOR_VER,
    CPLD_MINOR_VER,
    CPLD_ID,
    CPLD_BUILD_VER,
    CPLD_VERSION_H,

    // CPLD 1
    BRD_SKU_ID,
    BRD_HW_BUILD_ID,
    BRD_HW_ID,
    BRD_DEPH_ID,
    BRD_BUILD_ID,
    BRD_ID_TYPE,
    CPLD_CHIP_TYPE,
    MUX_RESET_ALL,
    I2C_MUX_0X75_RESET,
    I2C_MUX_0X77_RESET,
    I2C_MUX_0X76_RESET,
    MUX_CTRL,
    UART_CTRL,

    //BSP
    BSP_VERSION,
    BSP_DEBUG,
    BSP_PR_INFO,
    BSP_PR_ERR,
    BSP_REG,
    BSP_REG_VALUE,
    BSP_GPIO_MAX,
    BSP_GPIO_BASE,
    BSP_WP_ACCESS_COUNT,
    ATT_I2C_STUCK,

    //EC
    ATT_EC_BIOS_BOOT_ROM,
    ATT_EC_CPU_REV_HW_REV,
    ATT_EC_CPU_REV_DEV_PHASE,
    ATT_EC_CPU_REV_BUILD_ID,
    ATT_EC_MAJOR_VER,
    ATT_EC_MINOR_VER,
    ATT_EC_BUILD_VER,
    ATT_EC_VERSION_H,

    ATT_MAX
};

enum i2c_stuck_status
{
    I2C_STUCK_STATUS_NORMAL,
    I2C_STUCK_STATUS_ROOT_BUS,
    I2C_STUCK_STATUS_TRANSCEIVER,
    I2C_STUCK_STATUS_ROOT_BUS_NOT_READY
};

char *i2c_stuck_status_str[] = {
    "0", //Normal
    "1", //Root I2C bus stuck
    "2"  //Transceiver stuck
};

enum i2c_stuck_dev
{
    I2C_STUCK_DEV_ROOT_BUS,
    I2C_STUCK_DEV_MAX
};

enum data_type {
    DATA_HEX,
    DATA_DEC,
    DATA_S_DEC,
    DATA_0_1,
    DATA_0_1_INV,
    DATA_UNK,
};

enum reg_write_protect
{
    REG_WP_DIS = false,
    REG_WP_EN = true
};

typedef struct  {
    u16 reg;
    u8 mask;
    u8 data_type;
    bool write_protect;
} attr_reg_map_t;

typedef struct  {
    u8 bus;
    u8 addr;
    u8 reg;
} struct_i2c_dev;

attr_reg_map_t attr_reg[]= {

    [CPLD_MAJOR_VER]           =         {CPLD_VERSION_REG          , MASK_1100_0000, DATA_DEC,   REG_WP_DIS},
    [CPLD_MINOR_VER]           =         {CPLD_VERSION_REG          , MASK_0011_1111, DATA_DEC,   REG_WP_DIS},
    [CPLD_ID]                  =         {CPLD_ID_REG               , MASK_0000_0111, DATA_DEC,   REG_WP_DIS},
    [CPLD_BUILD_VER]           =         {CPLD_BUILD_REG            , MASK_ALL      , DATA_DEC,   REG_WP_DIS},
    [CPLD_VERSION_H]           =         {REG_NONE                  , MASK_NONE     , DATA_UNK,   REG_WP_DIS},
    [BRD_SKU_ID]               =         {BRD_SKU_ID_REG            , MASK_ALL      , DATA_HEX,   REG_WP_DIS},
    [BRD_HW_BUILD_ID]          =         {BRD_HW_BUILD_REV_REG      , MASK_ALL      , DATA_HEX,   REG_WP_DIS},
    [BRD_HW_ID]                =         {BRD_HW_BUILD_REV_REG      , MASK_0000_0011, DATA_DEC,   REG_WP_DIS},
    [BRD_DEPH_ID]              =         {BRD_HW_BUILD_REV_REG      , MASK_0000_0100, DATA_DEC,   REG_WP_DIS},
    [BRD_BUILD_ID]             =         {BRD_HW_BUILD_REV_REG      , MASK_0011_1000, DATA_DEC,   REG_WP_DIS},
    [BRD_ID_TYPE]              =         {BRD_HW_BUILD_REV_REG      , MASK_1000_0000, DATA_DEC,   REG_WP_DIS},
    [CPLD_CHIP_TYPE]           =         {CPLD_CHIP_TYPE_REG        , MASK_0000_0011, DATA_DEC,   REG_WP_DIS},
    [MUX_RESET_ALL]            =         {REG_NONE                  , MASK_NONE     , DATA_DEC,   REG_WP_EN },
    [I2C_MUX_0X75_RESET]       =         {I2C_MUX_RESET_REG         , MASK_0000_0001, DATA_DEC,   REG_WP_EN },
    [I2C_MUX_0X77_RESET]       =         {I2C_MUX_RESET_REG         , MASK_0000_0010, DATA_DEC,   REG_WP_EN },
    [I2C_MUX_0X76_RESET]       =         {I2C_MUX_RESET_REG         , MASK_0000_0100, DATA_DEC,   REG_WP_EN },
    [MUX_CTRL]                 =         {MUX_CTRL_REG              , MASK_ALL      , DATA_HEX,   REG_WP_EN },
    [UART_CTRL]                =         {MUX_CTRL_REG              , MASK_0100_0000, DATA_HEX,   REG_WP_EN },

    //BSP
    [BSP_VERSION]              =         {REG_NONE,                   MASK_NONE,      DATA_UNK,   REG_WP_DIS},
    [BSP_DEBUG]                =         {REG_NONE,                   MASK_NONE,      DATA_UNK,   REG_WP_DIS},
    [BSP_PR_INFO]              =         {REG_NONE,                   MASK_NONE,      DATA_UNK,   REG_WP_DIS},
    [BSP_PR_ERR]               =         {REG_NONE,                   MASK_NONE,      DATA_UNK,   REG_WP_DIS},
    [BSP_REG]                  =         {REG_NONE,                   MASK_NONE,      DATA_UNK,   REG_WP_DIS},
    [BSP_REG_VALUE]            =         {REG_NONE,                   MASK_NONE,      DATA_HEX,   REG_WP_DIS},
    [BSP_GPIO_MAX]             =         {REG_NONE,                   MASK_NONE,      DATA_DEC,   REG_WP_DIS},
    [BSP_GPIO_BASE]            =         {REG_NONE,                   MASK_NONE,      DATA_DEC,   REG_WP_DIS},
    [BSP_WP_ACCESS_COUNT]      =         {REG_NONE,                   MASK_NONE,      DATA_UNK,   REG_WP_DIS},
    [ATT_I2C_STUCK]            =         {REG_NONE,                   MASK_NONE,      DATA_UNK,   REG_WP_DIS},

    // EC
    [ATT_EC_BIOS_BOOT_ROM]     =         {REG_MISC_CTRL,              MASK_0100_0000, DATA_DEC,   REG_WP_DIS},
    [ATT_EC_CPU_REV_HW_REV]    =         {REG_EC_CPU_REV_ID,          MASK_0000_0011, DATA_DEC,   REG_WP_DIS},
    [ATT_EC_CPU_REV_DEV_PHASE] =         {REG_EC_CPU_REV_ID,          MASK_0000_0100, DATA_DEC,   REG_WP_DIS},
    [ATT_EC_CPU_REV_BUILD_ID]  =         {REG_EC_CPU_REV_ID,          MASK_0001_1000, DATA_DEC,   REG_WP_DIS},
    [ATT_EC_MAJOR_VER]         =         {REG_EC_MAJOR_VER,           MASK_ALL,       DATA_DEC,   REG_WP_DIS},
    [ATT_EC_MINOR_VER]         =         {REG_EC_MINOR_VER,           MASK_ALL,       DATA_DEC,   REG_WP_DIS},
    [ATT_EC_BUILD_VER]         =         {REG_EC_BUILD_VER,           MASK_ALL,       DATA_DEC,   REG_WP_DIS},
};

struct_i2c_dev i2c_stuck_dev[]= {
    [I2C_STUCK_DEV_ROOT_BUS]          = {0, 0x75, 0x00},
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
u8 enable_log_read  = LOG_DISABLE;
u8 enable_log_write = LOG_DISABLE;
u8 enable_log_sys   = LOG_ENABLE;
u8 mailbox_inited=0;

static unsigned int wp_access_count = 0;

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

static int _parse_data(char *buf, unsigned int data, u8 data_type)
{
    if(buf == NULL) {
        return -EINVAL;
    }

    if(data_type == DATA_HEX) {
        return sprintf(buf, "0x%02x", data);
    } else if(data_type == DATA_DEC) {
        return sprintf(buf, "%u", data);
    } else if(data_type == DATA_S_DEC) {
        return sprintf(buf, "%d", (s8)data);
    } else {
        return -EINVAL;
    }
    return 0;
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

static int _bsp_log_config(u8 log_type)
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

// write enable
static u8 lpc_wp_begin(void)
{
    u8 current_wp = 0;

    mutex_lock(&lpc_data->access_lock);

    current_wp = inb(REG_LPC_WRITE_PROTECT);

    if (!(current_wp & MASK_LPC_WP_ENABLE))
    {
        _outb(current_wp | MASK_LPC_WP_ENABLE, REG_LPC_WRITE_PROTECT);
        wp_access_count++;
    }

    return current_wp;
}

// write disable
static void lpc_wp_end(u8 original_wp_state)
{
    if (!(original_wp_state & MASK_LPC_WP_ENABLE))
    {
        _outb(original_wp_state, REG_LPC_WRITE_PROTECT);
    }

    mutex_unlock(&lpc_data->access_lock);
}
#if 0
/* init bmc mailbox */
static int bmc_mailbox_init(void)
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

    //set mailbox_inited
    mailbox_inited = 1;

    return mailbox_inited;
}
#endif

/* get lpc register value */
static u8 _lpc_reg_read(u16 reg, u8 mask)
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
static ssize_t lpc_reg_read(u16 reg, u8 mask, char *buf, u8 data_type)
{
    u8 reg_val;
    int len=0;

    reg_val = _lpc_reg_read(reg, mask);

    // may need to change to hex value ?
    len=_parse_data(buf, reg_val, data_type);

    return len;
}

/* set lpc register value */
static ssize_t lpc_reg_write(u16 reg, u8 mask, const char *buf, size_t count, u8 data_type, bool write_protect)
{
    u8 reg_val, reg_val_now, shift;

    if (kstrtou8(buf, 0, &reg_val) < 0) {
        if(data_type == DATA_S_DEC) {
            if (kstrtos8(buf, 0, &reg_val) < 0) {
                return -EINVAL;
            }
        } else {
            return -EINVAL;
        }
    }

    //apply continuous bits operation if mask is specified, discontinuous bits are not supported
    if (mask != MASK_ALL) {
        reg_val_now = _lpc_reg_read(reg, MASK_ALL);
        //clear bits in reg_val_now by the mask
        reg_val_now &= ~mask;
        //get bit shift by the mask
        shift = _shift(mask);
        //calculate new reg_val
        reg_val = _bit_operation(reg_val_now, shift, reg_val);
    }


    if (write_protect)
    {
        u8 original_wp = lpc_wp_begin();
        _outb(reg_val, reg);
        lpc_wp_end(original_wp);
    }
    else
    {
        mutex_lock(&lpc_data->access_lock);
        _outb(reg_val, reg);
        mutex_unlock(&lpc_data->access_lock);
    }

    BSP_LOG_W("reg=0x%03x, reg_val=0x%02x, mask=0x%02x", reg, reg_val, mask);

    return count;
}

/* get bsp value */
static ssize_t bsp_read(char *buf, char *str)
{
    ssize_t len=0;

    mutex_lock(&lpc_data->access_lock);
    len=sprintf(buf, "%s", str);
    mutex_unlock(&lpc_data->access_lock);

    BSP_LOG_R("reg_val=%s", str);

    return len;
}

/* set bsp value */
static ssize_t bsp_write(const char *buf, char *str, size_t str_len, size_t count)
{
    mutex_lock(&lpc_data->access_lock);
    snprintf(str, str_len, "%s", buf);
    mutex_unlock(&lpc_data->access_lock);

    BSP_LOG_W("reg_val=%s", str);

    return count;
}

/* get gpio max value */
static ssize_t gpio_max_show(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    if (attr->index == BSP_GPIO_MAX) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0)
        u8 data_type=DATA_UNK;
        data_type = attr_reg[attr->index].data_type;
        return _parse_data(buf, ARCH_NR_GPIOS-1, data_type);
#else
        return sprintf(buf, "%d\n", -1);
#endif
    }
    return -1;
}

/* get gpio base value */
static ssize_t gpio_base_show(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    if (attr->index == BSP_GPIO_BASE) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0)
        return sprintf(buf, "%d\n", -1);
#else
        u8 data_type=DATA_UNK;
        data_type = attr_reg[attr->index].data_type;
        return _parse_data(buf, GPIO_DYNAMIC_BASE, data_type);
#endif
    }
    return -1;
}

/* get mb cpld version in human readable format */
static ssize_t cpld_version_h_show(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    unsigned int attr_major = 0;
    unsigned int attr_minor = 0;
    unsigned int attr_build = 0;
    char *fmt=NULL;

    switch (attr->index) {
        case CPLD_VERSION_H:
            attr_major = CPLD_MAJOR_VER;
            attr_minor = CPLD_MINOR_VER;
            attr_build = CPLD_BUILD_VER;
            fmt="%d.%02d.%03d";
            break;
        case ATT_EC_VERSION_H:
            attr_major = ATT_EC_MAJOR_VER;
            attr_minor = ATT_EC_MINOR_VER;
            attr_build = ATT_EC_BUILD_VER;
            fmt="%d.%d.%d";
            break;
        default:
            return -1;
    }

    return sprintf(buf, fmt, _lpc_reg_read(attr_reg[attr_major].reg, attr_reg[attr_major].mask),
                                        _lpc_reg_read(attr_reg[attr_minor].reg, attr_reg[attr_minor].mask),
                                        _lpc_reg_read(attr_reg[attr_build].reg, attr_reg[attr_build].mask));

}

/* get lpc register value */
static ssize_t lpc_callback_show(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u16 reg = 0;
    u8 mask = MASK_NONE;
    u8 data_type=DATA_UNK;

    switch (attr->index) {
        // CPLD
        case CPLD_MINOR_VER:
        case CPLD_MAJOR_VER:
        case CPLD_ID:
        case CPLD_BUILD_VER:
        case BRD_SKU_ID:
        case BRD_HW_BUILD_ID:
        case BRD_HW_ID:
        case BRD_DEPH_ID:
        case BRD_BUILD_ID:
        case BRD_ID_TYPE:
        case CPLD_CHIP_TYPE:
        case MUX_RESET_ALL:
        case I2C_MUX_0X75_RESET:
        case I2C_MUX_0X77_RESET:
        case I2C_MUX_0X76_RESET:
        case MUX_CTRL:
        case UART_CTRL:

        //BSP
        case BSP_GPIO_MAX:
        case BSP_GPIO_BASE:
        //EC
        case ATT_EC_BIOS_BOOT_ROM:
        case ATT_EC_CPU_REV_HW_REV:
        case ATT_EC_CPU_REV_DEV_PHASE:
        case ATT_EC_CPU_REV_BUILD_ID:
        case ATT_EC_MAJOR_VER:
        case ATT_EC_MINOR_VER:
        case ATT_EC_BUILD_VER:
            reg = attr_reg[attr->index].reg;
            mask= attr_reg[attr->index].mask;
            data_type = attr_reg[attr->index].data_type;
            break;
        //BSP special case
        case BSP_REG_VALUE:
            if (kstrtou16(bsp_reg, 0, &reg) < 0)
                return -EINVAL;

            mask = MASK_ALL;
            break;

        default:
            return -EINVAL;
    }
    return lpc_reg_read(reg, mask, buf, data_type);
}

/* set lpc register value */
static ssize_t lpc_callback_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u16 reg = 0;
    u8 mask = MASK_NONE;
    u8 data_type=DATA_UNK;
    bool write_protect;

    switch (attr->index) {
        // MB CPLD
        //case TOP_I2C_MUX_RST:
        //    reg = attr_reg[attr->index].reg;
        //    mask= attr_reg[attr->index].mask;
        //    data_type = attr_reg[attr->index].data_type;
        //    break;
        case UART_CTRL:
            reg = attr_reg[attr->index].reg;
            mask= attr_reg[attr->index].mask;
            data_type = attr_reg[attr->index].data_type;
            write_protect = attr_reg[attr->index].write_protect;
            break;
        default:
            return -EINVAL;
    }
    return lpc_reg_write(reg, mask, buf, count, data_type, write_protect);
}

/* get bsp parameter value */
static ssize_t bsp_callback_show(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    char *str=NULL;
    ssize_t ret = 0;

    switch (attr->index) {
        case BSP_VERSION:
            str = bsp_version;
            ret = bsp_read(buf, str);
            break;
        case BSP_DEBUG:
            str = bsp_debug;
            ret = bsp_read(buf, str);
            break;
        case BSP_REG:
            str = bsp_reg;
            ret = bsp_read(buf, str);
            break;
        case BSP_WP_ACCESS_COUNT:
            ret =  _parse_data(buf, wp_access_count, DATA_DEC);
            break;
        default:
            return -EINVAL;
    }
    return ret;
}

/* set bsp parameter value */
static ssize_t bsp_callback_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int str_len=0;
    char *str=NULL;
    u16 reg = 0;
    u8 bsp_debug_u8 = 0;

    switch (attr->index) {
        case BSP_VERSION:
            str = bsp_version;
            str_len = sizeof(bsp_version);
            break;
        case BSP_DEBUG:
            if (kstrtou8(buf, 0, &bsp_debug_u8) < 0) {
                return -EINVAL;
            } else if (_bsp_log_config(bsp_debug_u8) < 0) {
                return -EINVAL;
            }
            str = bsp_debug;
            str_len = sizeof(bsp_debug);
            break;
        case BSP_REG:
            if (kstrtou16(buf, 0, &reg) < 0)
                return -EINVAL;

            str = bsp_reg;
            str_len = sizeof(bsp_reg);
            break;
        default:
            return -EINVAL;
    }

    return bsp_write(buf, str, str_len, count);
}

static ssize_t bsp_pr_callback_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int str_len = strlen(buf);

    if(str_len <= 0)
        return str_len;

    switch (attr->index) {
        case BSP_PR_INFO:
            BSP_PR(KERN_INFO, "%s", buf);
            break;
        case BSP_PR_ERR:
            BSP_PR(KERN_ERR, "%s", buf);
            break;
        default:
            return -EINVAL;
    }

    return str_len;
}

/* set mux_reset register value */
static ssize_t mux_reset_all_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u16 reg = 0;
    u8 mask = MASK_NONE;
    u8 val = 0;
    u8 reg_val = 0;
    u8 original_wp;
    int i;

    static atomic_t mux_reset_flag = ATOMIC_INIT(0);

    static const int mux_list[] = {
        I2C_MUX_0X75_RESET,
        I2C_MUX_0X77_RESET,
        I2C_MUX_0X76_RESET,
    };

    const int *mux_targets = NULL;
    int num_targets = 0;

    switch (attr->index) {
        case MUX_RESET_ALL:
            mux_targets = mux_list;
            num_targets = ARRAY_SIZE(mux_list);
            break;
        case I2C_MUX_0X75_RESET:
        case I2C_MUX_0X77_RESET:
        case I2C_MUX_0X76_RESET:
            // take &attr->index as an array for 1 element
            mux_targets = &attr->index;
            num_targets = 1;
            break;
        default:
            return -EINVAL;
    }

    if (kstrtou8(buf, 0, &val) < 0)
        return -EINVAL;

    if (val != MUX_RESET_TRIGGER_VAL)
        return -EINVAL;

    if (atomic_cmpxchg(&mux_reset_flag, 0, 1) == 0) {
        BSP_LOG_W("i2c mux reset is triggered...");

        original_wp = lpc_wp_begin();
        for (i = 0; i < num_targets; i++) {
            reg  = attr_reg[mux_targets[i]].reg;
            mask = attr_reg[mux_targets[i]].mask;
            if (reg != REG_NONE) {
                reg_val = inb(reg);
                outb((reg_val & (u8)(~mask)), reg);
                BSP_LOG_W("reg=0x%03x, reg_val=0x%02x", reg, reg_val & (u8)(~mask));
            }
        }
        lpc_wp_end(original_wp);

        msleep(MDELAY_RESET_INTERVAL);

        original_wp = lpc_wp_begin();
        // Unset all MUXes
        for (i = 0; i < num_targets; i++) {
            reg  = attr_reg[mux_targets[i]].reg;
            mask = attr_reg[mux_targets[i]].mask;
            if (reg != REG_NONE) {
                reg_val = inb(reg);
                outb((reg_val | mask), reg);
                BSP_LOG_W("reg=0x%03x, reg_val=0x%02x", reg, reg_val | mask);
            }
        }
        lpc_wp_end(original_wp);

        msleep(MDELAY_RESET_FINISH);

        atomic_set(&mux_reset_flag, 0);

    } else {
        BSP_LOG_W("i2c mux is resetting... (ignore)");
    }
    return count;
}

/* read i2c device_reg by i2c_dev_id*/
static int is_i2c_adapter_ready(int bus)
{
    struct i2c_adapter *adapter;

    adapter = i2c_get_adapter(bus);
    if (!adapter) {
        return 0;
    } else {
        i2c_put_adapter(adapter);
        return 1;
    }
}

/* read i2c device */
static int i2c_dev_read(int bus, int addr, u8 dev_reg)
{
    struct i2c_adapter *adapter;
    struct i2c_client client;
    int ret;

    adapter = i2c_get_adapter(bus);
    if (!adapter) {
        BSP_PR(KERN_ERR, "i2c_get_adapter(bus %d) not loaded", bus);
        return 0;
    }

    //avoid i2c_new_dummy_device() or device_register() to skip device occupied check by creating fake client
    memset(&client, 0, sizeof(client));
    client.adapter = adapter;
    client.addr = addr;

    mutex_lock(&lpc_data->access_lock);
    ret = i2c_smbus_read_byte_data(&client, dev_reg);
    mutex_unlock(&lpc_data->access_lock);

    i2c_put_adapter(adapter);

    if (ret < 0) {
        BSP_PR(KERN_ERR, "i2c device Reader: Failed to read from device (%d-00%02x): %d\n", bus, addr, ret);
        return ret;
    }

    return ret;
}

static int i2c_dev_read_by_id(int i2c_dev_id)
{
    int bus, addr, dev_reg;

    if (i2c_dev_id < 0 || i2c_dev_id >= I2C_STUCK_DEV_MAX) {
        return -EINVAL;
    }

    bus     = i2c_stuck_dev[i2c_dev_id].bus;
    addr    = i2c_stuck_dev[i2c_dev_id].addr;
    dev_reg = i2c_stuck_dev[i2c_dev_id].reg;

    return i2c_dev_read(bus, addr, dev_reg);
}

/* get i2c stuck value */
static ssize_t i2c_stuck_callback_show(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int ret = I2C_STUCK_STATUS_NORMAL;
    int mask_normal = MASK_ALL;
    int i=0;
    int is_root_bus_ready = 0;
    int is_root_bus_stuck = 0;
    int is_transceiver_stuck = 0;
    struct_i2c_dev *i2c_stuck_devp = i2c_stuck_dev;

    switch (attr->index) {
        case ATT_I2C_STUCK:
            // Check root i2c bus ready
            if (is_i2c_adapter_ready(i2c_stuck_devp[I2C_STUCK_DEV_ROOT_BUS].bus)) {
                is_root_bus_ready = 1;
            }  else {
                is_root_bus_ready = 0;
                break;
            }

            // Check root i2c bus stuck
            if (is_root_bus_ready && (ret = i2c_dev_read_by_id(I2C_STUCK_DEV_ROOT_BUS)) < 0) {
                is_root_bus_stuck = 1;
                break;
            }

            // Check transceiver stuck
            for (i = I2C_STUCK_DEV_ROOT_BUS + 1; i < I2C_STUCK_DEV_MAX; i++) {

                // Check i2c adapter ready before accessing i2c device
                if (!is_i2c_adapter_ready(i2c_stuck_devp[i].bus)) {
                    continue;
                }

                ret = i2c_dev_read_by_id(i);
                if (ret < 0) {
                    BSP_PR(KERN_ERR, "i2c_dev_read_by_id(%d) failed, bus=%d, addr=0x%02x, reg=0x%02x, ret=%d", i,
                            i2c_stuck_devp[i].bus, i2c_stuck_devp[i].addr, i2c_stuck_devp[i].reg, ret);
                    return ret;
                } else if (ret != mask_normal) {
                    BSP_PR(KERN_ERR, "i2c_dev_read_by_id(%d) transceiver stuck detected: bus=%d, addr=0x%02x, reg=0x%02x, reg_val=0x%02x", i,
                            i2c_stuck_devp[i].bus, i2c_stuck_devp[i].addr, i2c_stuck_devp[i].reg, ret);
                    is_transceiver_stuck = 1;
                    break;
                }
            }
            break;
        default:
            return -EINVAL;
    }

    if (!is_root_bus_ready)
        ret = I2C_STUCK_STATUS_ROOT_BUS_NOT_READY;
    else if (is_root_bus_stuck)
        ret = I2C_STUCK_STATUS_ROOT_BUS;
    else if (is_transceiver_stuck)
        ret = I2C_STUCK_STATUS_TRANSCEIVER;
    else
        ret = I2C_STUCK_STATUS_NORMAL;

    return bsp_read(buf, i2c_stuck_status_str[ret]);
}

//SENSOR_DEVICE_ATTR - CPLD
static SENSOR_DEVICE_ATTR_RO(cpld_minor_ver             , lpc_callback     , CPLD_MINOR_VER);
static SENSOR_DEVICE_ATTR_RO(cpld_major_ver             , lpc_callback     , CPLD_MAJOR_VER);
static SENSOR_DEVICE_ATTR_RO(cpld_id                    , lpc_callback     , CPLD_ID);
static SENSOR_DEVICE_ATTR_RO(cpld_build_ver             , lpc_callback     , CPLD_BUILD_VER);
static SENSOR_DEVICE_ATTR_RO(cpld_version_h             , cpld_version_h   , CPLD_VERSION_H);
static SENSOR_DEVICE_ATTR_RO(board_sku_id               , lpc_callback     , BRD_SKU_ID);
static SENSOR_DEVICE_ATTR_RO(board_hw_build_id          , lpc_callback     , BRD_HW_BUILD_ID);
static SENSOR_DEVICE_ATTR_RO(board_hw_id                , lpc_callback     , BRD_HW_ID);
static SENSOR_DEVICE_ATTR_RO(board_deph_id              , lpc_callback     , BRD_DEPH_ID);
static SENSOR_DEVICE_ATTR_RO(board_build_id             , lpc_callback     , BRD_BUILD_ID);
static SENSOR_DEVICE_ATTR_RO(board_id_type              , lpc_callback     , BRD_ID_TYPE);
static SENSOR_DEVICE_ATTR_RO(cpld_chip_type             , lpc_callback     , CPLD_CHIP_TYPE);
static SENSOR_DEVICE_ATTR_WO(mux_reset_all              , mux_reset_all    , MUX_RESET_ALL);
static SENSOR_DEVICE_ATTR_WO(i2c_mux_0x75_reset         , mux_reset_all    , I2C_MUX_0X75_RESET);
static SENSOR_DEVICE_ATTR_WO(i2c_mux_0x77_reset         , mux_reset_all    , I2C_MUX_0X77_RESET);
static SENSOR_DEVICE_ATTR_WO(i2c_mux_0x76_reset         , mux_reset_all    , I2C_MUX_0X76_RESET);
static SENSOR_DEVICE_ATTR_RO(mux_ctrl                   , lpc_callback     , MUX_CTRL);
static SENSOR_DEVICE_ATTR_RW(uart_ctrl                  , lpc_callback     , UART_CTRL);

//SENSOR_DEVICE_ATTR - BSP
static SENSOR_DEVICE_ATTR_RW(bsp_version                , bsp_callback     , BSP_VERSION);
static SENSOR_DEVICE_ATTR_RW(bsp_debug                  , bsp_callback     , BSP_DEBUG);
static SENSOR_DEVICE_ATTR_WO(bsp_pr_info                , bsp_pr_callback  , BSP_PR_INFO);
static SENSOR_DEVICE_ATTR_WO(bsp_pr_err                 , bsp_pr_callback  , BSP_PR_ERR);
static SENSOR_DEVICE_ATTR_RW(bsp_reg                    , bsp_callback     , BSP_REG);
static SENSOR_DEVICE_ATTR_RO(bsp_reg_value              , lpc_callback     , BSP_REG_VALUE);
static SENSOR_DEVICE_ATTR_RO(bsp_gpio_max               , gpio_max         , BSP_GPIO_MAX);
static SENSOR_DEVICE_ATTR_RO(bsp_gpio_base              , gpio_base        , BSP_GPIO_BASE);
static SENSOR_DEVICE_ATTR_RO(bsp_wp_access_count        , bsp_callback     , BSP_WP_ACCESS_COUNT);
static SENSOR_DEVICE_ATTR_RO(i2c_stuck           , i2c_stuck_callback, ATT_I2C_STUCK);

//SENSOR_DEVICE_ATTR - EC
static SENSOR_DEVICE_ATTR_RO(bios_boot_rom              , lpc_callback     , ATT_EC_BIOS_BOOT_ROM);
static SENSOR_DEVICE_ATTR_RO(ec_cpu_rev_hw_rev          , lpc_callback     , ATT_EC_CPU_REV_HW_REV);
static SENSOR_DEVICE_ATTR_RO(ec_cpu_rev_dev_phase       , lpc_callback     , ATT_EC_CPU_REV_DEV_PHASE);
static SENSOR_DEVICE_ATTR_RO(ec_cpu_rev_build_id        , lpc_callback     , ATT_EC_CPU_REV_BUILD_ID);
static SENSOR_DEVICE_ATTR_RO(ec_major_ver               , lpc_callback     , ATT_EC_MAJOR_VER);
static SENSOR_DEVICE_ATTR_RO(ec_minor_ver               , lpc_callback     , ATT_EC_MINOR_VER);
static SENSOR_DEVICE_ATTR_RO(ec_build_ver               , lpc_callback     , ATT_EC_BUILD_VER);
static SENSOR_DEVICE_ATTR_RO(ec_version_h               , cpld_version_h   , ATT_EC_VERSION_H);

static struct attribute *mb_cpld_attrs[] = {
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(board_sku_id),
    _DEVICE_ATTR(board_hw_build_id),
    _DEVICE_ATTR(board_hw_id),
    _DEVICE_ATTR(board_deph_id),
    _DEVICE_ATTR(board_build_id),
    _DEVICE_ATTR(board_id_type),
    _DEVICE_ATTR(cpld_chip_type),
    _DEVICE_ATTR(mux_reset_all),
    _DEVICE_ATTR(i2c_mux_0x75_reset),
    _DEVICE_ATTR(i2c_mux_0x77_reset),
    _DEVICE_ATTR(i2c_mux_0x76_reset),
    _DEVICE_ATTR(mux_ctrl),
    _DEVICE_ATTR(uart_ctrl),
    NULL,
};

static struct attribute *bios_attrs[] = {
    // _DEVICE_ATTR(cpu_bios_boot_rom),
    // _DEVICE_ATTR(boot_cfg),
    _DEVICE_ATTR(bios_boot_rom),
    NULL,
};

static struct attribute *bsp_attrs[] = {
    _DEVICE_ATTR(bsp_version),
    _DEVICE_ATTR(bsp_debug),
    _DEVICE_ATTR(bsp_pr_info),
    _DEVICE_ATTR(bsp_pr_err),
    _DEVICE_ATTR(bsp_reg),
    _DEVICE_ATTR(bsp_reg_value),
    _DEVICE_ATTR(bsp_gpio_max),
    _DEVICE_ATTR(bsp_gpio_base),
    _DEVICE_ATTR(bsp_wp_access_count),
    _DEVICE_ATTR(i2c_stuck),
    NULL,
};

static struct attribute *ec_attrs[] = {
    _DEVICE_ATTR(bios_boot_rom),
    _DEVICE_ATTR(ec_cpu_rev_hw_rev),
    _DEVICE_ATTR(ec_cpu_rev_dev_phase),
    _DEVICE_ATTR(ec_cpu_rev_build_id),
    _DEVICE_ATTR(ec_major_ver),
    _DEVICE_ATTR(ec_minor_ver),
    _DEVICE_ATTR(ec_build_ver),
    _DEVICE_ATTR(ec_version_h),
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

static struct attribute_group ec_attr_grp = {
    .name = "ec",
    .attrs = ec_attrs,
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
    int err[4] = {0};
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

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
static int lpc_drv_remove(struct platform_device *pdev)
#else
static void lpc_drv_remove(struct platform_device *pdev)
#endif
{
    sysfs_remove_group(&pdev->dev.kobj, &mb_cpld_attr_grp);
    sysfs_remove_group(&pdev->dev.kobj, &bios_attr_grp);
    sysfs_remove_group(&pdev->dev.kobj, &bsp_attr_grp);
    sysfs_remove_group(&pdev->dev.kobj, &ec_attr_grp);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
    return 0;
#endif
}

static struct platform_driver lpc_drv = {
    .probe  = lpc_drv_probe,
    .remove = __exit_p(lpc_drv_remove),
    .driver = {
    .name   = DRIVER_NAME,
    },
};

static int __init lpc_init(void)
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

static void __exit lpc_exit(void)
{
    platform_driver_unregister(&lpc_drv);
    platform_device_unregister(&lpc_dev);
}

MODULE_AUTHOR("Zack Yen <zack.yen@ufispace.com>");
MODULE_DESCRIPTION("x86_64_ufispace_s9520_28xc_lpc driver");
MODULE_VERSION("1.0.0");
MODULE_LICENSE("GPL");

module_init(lpc_init);
module_exit(lpc_exit);
