/*
 * A lpc driver for the ufispace_s9511_20ct
 *
 * Copyright (C) 2024 UfiSpace Technology Corporation.
 * Melo Lin <melo.lin@ufispace.com>
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
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/hwmon-sysfs.h>
#include <linux/gpio.h>
#include <linux/version.h>
#define DRIVER_NAME "x86_64_ufispace_s9511_20ct_lpc"

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

/* LPC Registers */
#define REG_BASE_MB             0x700
#define REG_BASE_EC             0xE300
#define REG_NONE                0x00

/* LPC/CPLD 1 (MB CPLD) Registers */
#define REG_BRD_ID_0            (REG_BASE_MB + 0x00)  /* SKU ID Revision */
#define REG_BRD_ID_1            (REG_BASE_MB + 0x01)  /* HW & Build Revision Register */
#define REG_CPLD_VERSION        (REG_BASE_MB + 0x02)
#define REG_CPLD_ID             (REG_BASE_MB + 0x03)
#define REG_CPLD_BUILD          (REG_BASE_MB + 0x04)
#define REG_CPLD_CHIP           (REG_BASE_MB + 0x05)
#define REG_REV_ID              (REG_BASE_MB + 0x06)
#define REG_BRD_EXT_ID          (REG_BASE_MB + 0xD0)
#define REG_MUX_RESET           (REG_BASE_MB + 0x43)
#define REG_PSU_TYPE            (REG_BASE_MB + 0x59)
#define REG_CPLD_RESET          (REG_BASE_MB + 0xF0)
#define REG_CPLD_ENABLE_DEBUG   (REG_BASE_MB + 0xF1)
#define REG_CPLD_ROV_DEBUG      (REG_BASE_MB + 0xF2)
#define REG_CPLD_TEST           (REG_BASE_MB + 0xFF)

//EC
#define REG_BIOS_BOOT           (REG_BASE_EC + 0x0C)
#define REG_CPU_REV             (REG_BASE_EC + 0x17)

/************************ MASK Bit ************************/
#define MASK_ALL                     (0xFF)
#define MASK_NONE                    (0x00)
#define MASK_HB                      (0b11110000)
#define MASK_LB                      (0b00001111)
#define MASK_BIT0                    (0b00000001)
#define MASK_BIT1                    (0b00000010)
#define MASK_BIT2                    (0b00000100)
#define MASK_BIT3                    (0b00001000)
#define MASK_BIT4                    (0b00010000)
#define MASK_BIT5                    (0b00100000)
#define MASK_BIT6                    (0b01000000)
#define MASK_BIT7                    (0b10000000)
#define MASK_BIT1_0                  (0b00000011)
#define MASK_BIT2_1                  (0b00000110)
#define MASK_BIT3_2                  (0b00001100)
#define MASK_BIT4_3                  (0b00011000)
#define MASK_BIT5_4                  (0b00110000)
#define MASK_BIT6_5                  (0b01100000)
#define MASK_BIT7_6                  (0b11000000)
#define MASK_BIT2_0                  (0b00000111)
#define MASK_BIT3_1                  (0b00001110)
#define MASK_BIT4_2                  (0b00011100)
#define MASK_BIT5_3                  (0b00111000)
#define MASK_BIT6_4                  (0b01110000)
#define MASK_BIT7_5                  (0b11100000)
#define MASK_BIT3_0                  (MASK_LB)
#define MASK_BIT4_1                  (0b00011110)
#define MASK_BIT5_2                  (0b00111100)
#define MASK_BIT6_3                  (0b01111000)
#define MASK_BIT7_4                  (MASK_HB)
#define MASK_BIT4_0                  (0b00011111)
#define MASK_BIT5_1                  (0b00111110)
#define MASK_BIT6_2                  (0b01111100)
#define MASK_BIT7_3                  (0b11111000)
#define MASK_BIT5_0                  (0b00111111)
#define MASK_BIT6_1                  (0b01111110)
#define MASK_BIT6_0                  (0b01111111)
#define MASK_BIT7_1                  (0b11111110)
#define MASK_BIT7_0                  (MASK_ALL)
/******** Mask for better identification ********/
#define MASK_HW_REV                  (MASK_BIT1_0)
#define MASK_CPLD_MAJOR_VER          (MASK_BIT7_6)
#define MASK_CPLD_MINOR_VER          (MASK_BIT5_0)
#define MASK_CPLD_ID                 (MASK_BIT2_0)
#define MASK_HW_ID                   (0b00000011)
#define MASK_DEPH_ID                 (0b00000100)
#define MASK_BUILD_ID                (0b00011000)
#define MASK_BIOS_BOOT_ROM           (0b01000000)

#define LPC_MDELAY                        (5)
#define MDELAY_RESET_INTERVAL             (100)
#define MDELAY_RESET_FINISH               (500)

enum lpc_sysfs_attributes
{
    /******************** MB CPLD (CPLD 1)  ********************/
    BRD_ID_0,                             //0x00
    SKU_ID,                               //0x00
    BRD_ID_1,                             //0x01
    HW_REV,                               //0x01
    DEPH_ID,                              //0x01
    BUILD_ID,                             //0x01
    BIT_SEL_ID,                           //0x01
    CPLD_MAJOR_VER,                       //0x02
    CPLD_MINOR_VER,                       //0x02
    CPLD_VERSION_H,
    CPLD_ID,                              //0x03
    CPLD_BUILD_VER,                       //0x04
    CPLD_CHIP_TYPE,                       //0x05
    CPLD_EXT_ID,                          //0x06
    EXTEND_ID,                            //0xD0
    I2C_MUX_RESET_0,                      //0x43(RW)
    I2C_MUX_RESET_1,                      //0x43(RW)
    I2C_MUX_RESET_2,                      //0x43(RW)
    I2C_MUX_RESET_3,                      //0x43(RW)
    I2C_MUX_RESET_4,                      //0x43(RW)
    MUX_RESET,                            //0x43(RW)
    PSU_TYPE,                             //0x59(RO)
    CPLD1_I2C_UPGRADE_MODULE_RESET,       //0XF0(RW)
    CPLD1_ROV_RESET,                      //0XF0(RW)
    ROV_DEBUG_EN,                         //0XF1(RW)
    CLEAR_FRONT_PANEL_LED,                //0XF2(RW)
    CLEAR_PORT_LED,                       //0XF2(RW)
    CPLD1_TEST,                           //0XFF(RW)
    /************************** EC  ***************************/
    CPU_HW_ID,
    CPU_DEPH_ID,
    CPU_BUILD_ID,
    BIOS_BOOT_ROM,
    /************************** BSP  ***************************/
    BSP_VERSION,
    BSP_DEBUG,
    BSP_PR_INFO,
    BSP_PR_ERR,
    BSP_REG,
    BSP_REG_VALUE,
    BSP_GPIO_MAX,
    BSP_GPIO_BASE,
};

enum data_type
{
    DATA_HEX,
    DATA_DEC,
    DATA_S_DEC,
    DATA_UNK,
};

typedef struct
{
    u16 reg;
    u8 mask;
    u8 data_type;
} attr_reg_map_t;

attr_reg_map_t attr_reg[]=
{
    /*********************************************** MB CPLD (CPLD 1) ************************************************/
    [BRD_ID_0]                              =   {REG_BRD_ID_0                ,MASK_ALL                 ,DATA_HEX     },
    [SKU_ID]                                =   {REG_BRD_ID_0                ,MASK_ALL                 ,DATA_HEX     },
    [BRD_ID_1]                              =   {REG_BRD_ID_1                ,MASK_ALL                 ,DATA_HEX     },
    [HW_REV]                                =   {REG_BRD_ID_1                ,MASK_HW_REV              ,DATA_HEX     },
    [DEPH_ID]                               =   {REG_BRD_ID_1                ,MASK_BIT2                ,DATA_HEX     },
    [BUILD_ID]                              =   {REG_BRD_ID_1                ,MASK_BIT4_3              ,DATA_HEX     },
    [BIT_SEL_ID]                            =   {REG_BRD_ID_1                ,MASK_BIT5                ,DATA_HEX     },
    [CPLD_MAJOR_VER]                        =   {REG_CPLD_VERSION            ,MASK_CPLD_MAJOR_VER      ,DATA_DEC     },
    [CPLD_MINOR_VER]                        =   {REG_CPLD_VERSION            ,MASK_CPLD_MINOR_VER      ,DATA_DEC     },
    [CPLD_VERSION_H]                        =   {REG_NONE                    ,MASK_NONE                ,DATA_UNK     },
    [CPLD_ID]                               =   {REG_CPLD_ID                 ,MASK_CPLD_ID             ,DATA_DEC     },
    [CPLD_BUILD_VER]                        =   {REG_CPLD_BUILD              ,MASK_ALL                 ,DATA_DEC     },
    [CPLD_CHIP_TYPE]                        =   {REG_CPLD_CHIP               ,MASK_BIT1_0              ,DATA_HEX     },
    [CPLD_EXT_ID]                           =   {REG_REV_ID                  ,MASK_BIT2_0              ,DATA_HEX     },
    [EXTEND_ID]                             =   {REG_BRD_EXT_ID              ,MASK_BIT7_6              ,DATA_HEX     },
    [I2C_MUX_RESET_0]                       =   {REG_MUX_RESET               ,MASK_BIT0                ,DATA_HEX     },
    [I2C_MUX_RESET_1]                       =   {REG_MUX_RESET               ,MASK_BIT1                ,DATA_HEX     },
    [I2C_MUX_RESET_2]                       =   {REG_MUX_RESET               ,MASK_BIT2                ,DATA_HEX     },
    [I2C_MUX_RESET_3]                       =   {REG_MUX_RESET               ,MASK_BIT3                ,DATA_HEX     },
    [I2C_MUX_RESET_4]                       =   {REG_MUX_RESET               ,MASK_BIT4                ,DATA_HEX     },
    [MUX_RESET]                             =   {REG_MUX_RESET               ,MASK_BIT4_0              ,DATA_HEX     },
    [PSU_TYPE]                              =   {REG_PSU_TYPE                ,MASK_BIT6                ,DATA_HEX     },
    [CPLD1_I2C_UPGRADE_MODULE_RESET]        =   {REG_CPLD_RESET              ,MASK_BIT0                ,DATA_HEX     },
    [CPLD1_ROV_RESET]                       =   {REG_CPLD_RESET              ,MASK_BIT1                ,DATA_HEX     },
    [ROV_DEBUG_EN]                          =   {REG_CPLD_ENABLE_DEBUG       ,MASK_BIT0                ,DATA_HEX     },
    [CLEAR_FRONT_PANEL_LED]                 =   {REG_CPLD_ROV_DEBUG          ,MASK_BIT0                ,DATA_HEX     },
    [CLEAR_PORT_LED]                        =   {REG_CPLD_ROV_DEBUG          ,MASK_BIT2                ,DATA_HEX     },
    [CPLD1_TEST]                            =   {REG_CPLD_TEST               ,MASK_ALL                 ,DATA_HEX     },
    //EC
    [CPU_HW_ID]                             =   {REG_CPU_REV                 ,MASK_HW_ID               ,DATA_DEC     },
    [CPU_DEPH_ID]                           =   {REG_CPU_REV                 ,MASK_DEPH_ID             ,DATA_DEC     },
    [CPU_BUILD_ID]                          =   {REG_CPU_REV                 ,MASK_BUILD_ID            ,DATA_DEC     },
    [BIOS_BOOT_ROM]                         =   {REG_BIOS_BOOT               ,MASK_BIOS_BOOT_ROM       ,DATA_DEC     },
    /****************************************************** BSP *****************************************************/
    [BSP_VERSION]                           =   {REG_NONE                    ,MASK_NONE                ,DATA_UNK     },
    [BSP_DEBUG]                             =   {REG_NONE                    ,MASK_NONE                ,DATA_UNK     },
    [BSP_PR_INFO]                           =   {REG_NONE                    ,MASK_NONE                ,DATA_UNK     },
    [BSP_PR_ERR]                            =   {REG_NONE                    ,MASK_NONE                ,DATA_UNK     },
    [BSP_REG]                               =   {REG_NONE                    ,MASK_NONE                ,DATA_UNK     },
    [BSP_REG_VALUE]                         =   {REG_NONE                    ,MASK_NONE                ,DATA_HEX     },
    [BSP_GPIO_MAX]                          =   {REG_NONE                    ,MASK_NONE                ,DATA_DEC     },
    [BSP_GPIO_BASE]                         =   {REG_NONE                    ,MASK_NONE                ,DATA_DEC     },
};

enum bsp_log_types
{
    LOG_NONE,
    LOG_RW,
    LOG_READ,
    LOG_WRITE,
    LOG_SYS
};

enum bsp_log_ctrl
{
    LOG_DISABLE,
    LOG_ENABLE
};

struct lpc_data_s
{
    struct mutex    access_lock;
};

struct lpc_data_s *lpc_data;
char bsp_version[16]="";
char bsp_debug[2]="0";
char bsp_reg[8]="0x0";
u8 enable_log_read  = LOG_DISABLE;
u8 enable_log_write = LOG_DISABLE;
u8 enable_log_sys   = LOG_ENABLE;

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

static u8 _parse_data(char *buf, unsigned int data, u8 data_type)
{
    if (buf == NULL) {
        return -1;
    }

    if (data_type == DATA_HEX)
        return sprintf(buf, "0x%02x", data);
    else if (data_type == DATA_DEC)
        return sprintf(buf, "%u", data);
    else
        return -1;
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
static ssize_t lpc_reg_write(u16 reg, u8 mask, const char *buf, size_t count, u8 data_type)
{
    u8 reg_val, reg_val_now, shift;

    if (kstrtou8(buf, 0, &reg_val) < 0) {
        if (data_type == DATA_S_DEC) {
            if (kstrtos8(buf, 0, &reg_val) < 0) {
                return -EINVAL;
            }
        } else {
            return -EINVAL;
        }
    }

    /* apply continuous bits operation if mask is specified, discontinuous bits are not supported */
    if (mask != MASK_ALL) {
        reg_val_now = _lpc_reg_read(reg, MASK_ALL);
        /* clear bits in reg_val_now by the mask */
        reg_val_now &= ~mask;
        /* get bit shift by the mask */
        shift = _shift(mask);
        /* calculate new reg_val */
        reg_val = _bit_operation(reg_val_now, shift, reg_val);
    }

    mutex_lock(&lpc_data->access_lock);

    _outb(reg_val, reg);

    mutex_unlock(&lpc_data->access_lock);

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
static ssize_t mb_cpld_version_h_show(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    unsigned int attr_major = 0;
    unsigned int attr_minor = 0;
    unsigned int attr_build = 0;

    switch (attr->index) {
        case CPLD_VERSION_H:
            attr_major = CPLD_MAJOR_VER;
            attr_minor = CPLD_MINOR_VER;
            attr_build = CPLD_BUILD_VER;
            break;
        default:
            return -1;
    }

    return sprintf(buf, "%d.%02d.%03d", _lpc_reg_read(attr_reg[attr_major].reg, attr_reg[attr_major].mask),
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
        /* MB CPLD (CPLD 1) */
        case BRD_ID_0:
        case SKU_ID:
        case BRD_ID_1:
        case HW_REV:
        case DEPH_ID:
        case BUILD_ID:
        case BIT_SEL_ID:
        case CPLD_MAJOR_VER:
        case CPLD_MINOR_VER:
        case CPLD_VERSION_H:
        case CPLD_ID:
        case CPLD_BUILD_VER:
        case CPLD_CHIP_TYPE:
        case CPLD_EXT_ID:
        case EXTEND_ID:
        //case I2C_MUX_RESET_0:
        //case I2C_MUX_RESET_1:
        //case I2C_MUX_RESET_2:
        //case I2C_MUX_RESET_3:
        //case I2C_MUX_RESET_4:
        //case MUX_RESET:
        case PSU_TYPE:
        case CPLD1_I2C_UPGRADE_MODULE_RESET:
        case CPLD1_ROV_RESET:
        case ROV_DEBUG_EN:
        case CLEAR_FRONT_PANEL_LED:
        case CLEAR_PORT_LED:
        case CPLD1_TEST:
        /* EC */
        case CPU_HW_ID:
        case CPU_DEPH_ID:
        case CPU_BUILD_ID:
        case BIOS_BOOT_ROM:
        /* BSP */
        case BSP_GPIO_MAX:
        case BSP_GPIO_BASE:
            reg = attr_reg[attr->index].reg;
            mask= attr_reg[attr->index].mask;
            data_type = attr_reg[attr->index].data_type;
            break;
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

    switch (attr->index) {
        //MB CPLD
        case CPLD1_I2C_UPGRADE_MODULE_RESET:
        case CPLD1_ROV_RESET:
        case ROV_DEBUG_EN:
        case CLEAR_FRONT_PANEL_LED:
        case CLEAR_PORT_LED:
        case CPLD1_TEST:
            reg = attr_reg[attr->index].reg;
            mask= attr_reg[attr->index].mask;
            data_type = attr_reg[attr->index].data_type;
            break;
        default:
            return -EINVAL;
    }

    return lpc_reg_write(reg, mask, buf, count, data_type);
}

/* get bsp parameter value */
static ssize_t bsp_callback_show(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    char *str=NULL;

    switch (attr->index) {
        case BSP_VERSION:
            str = bsp_version;
            break;
        case BSP_DEBUG:
            str = bsp_debug;
            break;
        case BSP_REG:
            str = bsp_reg;
            break;
        default:
            return -EINVAL;
    }

    return bsp_read(buf, str);
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
            if (kstrtou8(buf, 0, &bsp_debug_u8) < 0)
                return -EINVAL;
            else if (_bsp_log_config(bsp_debug_u8) < 0)
                return -EINVAL;

            str = bsp_debug;
            str_len = sizeof(bsp_debug);
            break;
        case BSP_REG:
            if (kstrtou16(buf, 0, &reg) < 0) {
                return -EINVAL;
            }
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
    u8 mux_reset_reg_val = 0;
    static int mux_reset_flag = 0;

    switch (attr->index) {
        case I2C_MUX_RESET_0:
        case I2C_MUX_RESET_1:
        case I2C_MUX_RESET_2:
        case I2C_MUX_RESET_3:
        case I2C_MUX_RESET_4:
        case MUX_RESET:
            reg = attr_reg[attr->index].reg;
            mask= attr_reg[attr->index].mask;
            break;
        default:
            return -EINVAL;
    }

    if (kstrtou8(buf, 0, &val) < 0)
        return -EINVAL;

    if (mux_reset_flag == 0) {
        if (val == 0) {
            mutex_lock(&lpc_data->access_lock);
            mux_reset_flag = 1;
            BSP_LOG_W("i2c mux reset is triggered...");

            //reset mux on ports
            mux_reset_reg_val = inb(reg);
            outb((mux_reset_reg_val & (u8)(~mask)), reg);
            BSP_LOG_W("reg=0x%03x, reg_val=0x%02x", reg, mux_reset_reg_val & (u8)(~mask));

            mdelay(MDELAY_RESET_INTERVAL);

            //unset mux on ports
            outb((mux_reset_reg_val | mask), reg);
            BSP_LOG_W("reg=0x%03x, reg_val=0x%02x", reg, mux_reset_reg_val | mask);
            mdelay(MDELAY_RESET_FINISH);
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

/* SENSOR_DEVICE_ATTR - MB CPLD (CPLD 1) */

static SENSOR_DEVICE_ATTR_RO(board_id_0                         ,lpc_callback       ,BRD_ID_0                         );
static SENSOR_DEVICE_ATTR_RO(board_id_1                         ,lpc_callback       ,BRD_ID_1                         );
static SENSOR_DEVICE_ATTR_RO(sku_id                             ,lpc_callback       ,SKU_ID                           );
static SENSOR_DEVICE_ATTR_RO(hw_rev                             ,lpc_callback       ,HW_REV                           );
static SENSOR_DEVICE_ATTR_RO(deph_id                            ,lpc_callback       ,DEPH_ID                          );
static SENSOR_DEVICE_ATTR_RO(build_id                           ,lpc_callback       ,BUILD_ID                         );
static SENSOR_DEVICE_ATTR_RO(bit_sel_id                         ,lpc_callback       ,BIT_SEL_ID                       );
static SENSOR_DEVICE_ATTR_RO(cpld_major_ver                     ,lpc_callback       ,CPLD_MAJOR_VER                   );
static SENSOR_DEVICE_ATTR_RO(cpld_minor_ver                     ,lpc_callback       ,CPLD_MINOR_VER                   );
static SENSOR_DEVICE_ATTR_RO(cpld_version_h                     ,mb_cpld_version_h  ,CPLD_VERSION_H                   );
static SENSOR_DEVICE_ATTR_RO(cpld_id                            ,lpc_callback       ,CPLD_ID                          );
static SENSOR_DEVICE_ATTR_RO(cpld_build_ver                     ,lpc_callback       ,CPLD_BUILD_VER                   );
static SENSOR_DEVICE_ATTR_RO(cpld_chip_type                     ,lpc_callback       ,CPLD_CHIP_TYPE                   );
static SENSOR_DEVICE_ATTR_RO(cpld_ext_id                        ,lpc_callback       ,CPLD_EXT_ID                      );
static SENSOR_DEVICE_ATTR_RO(extend_id                          ,lpc_callback       ,EXTEND_ID                        );
static SENSOR_DEVICE_ATTR_WO(i2c_mux_reset_0                    ,mux_reset_all      ,I2C_MUX_RESET_0                  );
static SENSOR_DEVICE_ATTR_WO(i2c_mux_reset_1                    ,mux_reset_all      ,I2C_MUX_RESET_1                  );
static SENSOR_DEVICE_ATTR_WO(i2c_mux_reset_2                    ,mux_reset_all      ,I2C_MUX_RESET_2                  );
static SENSOR_DEVICE_ATTR_WO(i2c_mux_reset_3                    ,mux_reset_all      ,I2C_MUX_RESET_3                  );
static SENSOR_DEVICE_ATTR_WO(i2c_mux_reset_4                    ,mux_reset_all      ,I2C_MUX_RESET_4                  );
static SENSOR_DEVICE_ATTR_WO(mux_reset                          ,mux_reset_all      ,MUX_RESET                        );
static SENSOR_DEVICE_ATTR_RO(psu_type                           ,lpc_callback       ,PSU_TYPE                         );
static SENSOR_DEVICE_ATTR_RW(cpld1_i2c_upgrade_module_reset     ,lpc_callback       ,CPLD1_I2C_UPGRADE_MODULE_RESET   );
static SENSOR_DEVICE_ATTR_RW(cpld1_rev_reset                    ,lpc_callback       ,CPLD1_ROV_RESET                  );
static SENSOR_DEVICE_ATTR_RW(rov_debug_en                       ,lpc_callback       ,ROV_DEBUG_EN                     );
static SENSOR_DEVICE_ATTR_RW(clear_front_panel_led              ,lpc_callback       ,CLEAR_FRONT_PANEL_LED            );
static SENSOR_DEVICE_ATTR_RW(clear_port_led                     ,lpc_callback       ,CLEAR_PORT_LED                   );
static SENSOR_DEVICE_ATTR_RW(cpld1_test                         ,lpc_callback       ,CPLD1_TEST                       );
//SENSOR_DEVICE_ATTR - EC
static SENSOR_DEVICE_ATTR_RO(cpu_hw_id                          ,lpc_callback       ,CPU_HW_ID                        );
static SENSOR_DEVICE_ATTR_RO(cpu_deph_id                        ,lpc_callback       ,CPU_DEPH_ID                      );
static SENSOR_DEVICE_ATTR_RO(cpu_build_id                       ,lpc_callback       ,CPU_BUILD_ID                     );
static SENSOR_DEVICE_ATTR_RO(bios_boot_rom                      ,lpc_callback       ,BIOS_BOOT_ROM                    );
/* SENSOR_DEVICE_ATTR - BSP */
static SENSOR_DEVICE_ATTR_RW(bsp_version                        ,bsp_callback       ,BSP_VERSION                      );
static SENSOR_DEVICE_ATTR_RW(bsp_debug                          ,bsp_callback       ,BSP_DEBUG                        );
static SENSOR_DEVICE_ATTR_WO(bsp_pr_info                        ,bsp_pr_callback    ,BSP_PR_INFO                      );
static SENSOR_DEVICE_ATTR_WO(bsp_pr_err                         ,bsp_pr_callback    ,BSP_PR_ERR                       );
static SENSOR_DEVICE_ATTR_RW(bsp_reg                            ,bsp_callback       ,BSP_REG                          );
static SENSOR_DEVICE_ATTR_RO(bsp_reg_value                      ,lpc_callback       ,BSP_REG_VALUE                    );
static SENSOR_DEVICE_ATTR_RO(bsp_gpio_max                       ,gpio_max           ,BSP_GPIO_MAX                     );
static SENSOR_DEVICE_ATTR_RO(bsp_gpio_base                      ,gpio_base          ,BSP_GPIO_BASE                    );

static struct attribute *mb_cpld_attrs[] = {
    _DEVICE_ATTR(board_id_0),
    _DEVICE_ATTR(board_id_1),
    _DEVICE_ATTR(sku_id),
    _DEVICE_ATTR(hw_rev),
    _DEVICE_ATTR(deph_id),
    _DEVICE_ATTR(build_id),
    _DEVICE_ATTR(bit_sel_id),
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_chip_type),
    _DEVICE_ATTR(cpld_ext_id),
    _DEVICE_ATTR(extend_id),
    _DEVICE_ATTR(i2c_mux_reset_0),
    _DEVICE_ATTR(i2c_mux_reset_1),
    _DEVICE_ATTR(i2c_mux_reset_2),
    _DEVICE_ATTR(i2c_mux_reset_3),
    _DEVICE_ATTR(i2c_mux_reset_4),
    _DEVICE_ATTR(mux_reset),
    _DEVICE_ATTR(psu_type),
    _DEVICE_ATTR(cpld1_i2c_upgrade_module_reset),
    _DEVICE_ATTR(cpld1_rev_reset),
    _DEVICE_ATTR(rov_debug_en),
    _DEVICE_ATTR(clear_front_panel_led),
    _DEVICE_ATTR(clear_port_led),
    _DEVICE_ATTR(cpld1_test),

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

    NULL,
};

static struct attribute *ec_attrs[] = {
    _DEVICE_ATTR(cpu_hw_id),
    _DEVICE_ATTR(cpu_deph_id),
    _DEVICE_ATTR(cpu_build_id),
    _DEVICE_ATTR(bios_boot_rom),
    NULL,
};

static struct attribute_group mb_cpld_attr_grp = {
    .name = "mb_cpld",
    .attrs = mb_cpld_attrs,
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
    int i = 0, grp_num = 3;
    int err[3] = {0};
    struct attribute_group *grp = NULL;

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
                grp = &bsp_attr_grp;
                break;
            case 2:
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
                grp = &bsp_attr_grp;
                break;
            case 2:
                grp = &ec_attr_grp;
                break;
            default:
                break;
        }

        sysfs_remove_group(&pdev->dev.kobj, grp);
        if (!err[i]) {
            /* remove previous successful cases */
            continue;
        } else {
            /* remove first failed case, then return */
            return err[i];
        }
    }

    return 0;
}

static int lpc_drv_remove(struct platform_device *pdev)
{
    sysfs_remove_group(&pdev->dev.kobj, &mb_cpld_attr_grp);
    sysfs_remove_group(&pdev->dev.kobj, &bsp_attr_grp);
    sysfs_remove_group(&pdev->dev.kobj, &ec_attr_grp);
    return 0;
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

module_init(lpc_init);
module_exit(lpc_exit);

MODULE_AUTHOR("Jason Tsai <jason.cy.tsai@ufispace.com>");
MODULE_DESCRIPTION(DRIVER_NAME" driver");
MODULE_VERSION("1.0.0");
MODULE_LICENSE("GPL");
