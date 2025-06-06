/*
 * A i2c cpld driver for the ufispace_s9620_54dc
 *
 * Copyright (C) 2023 UfiSpace Technology Corporation.
 * Alex Hsia <alex.hsia@ufispace.com>
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
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/dmi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include "x86-64-ufispace-s9620-54dc-cpld-main.h"

bool mux_en = false;
module_param(mux_en, bool, S_IWUSR|S_IRUSR);

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

#ifdef DEBUG
#define DEBUG_PRINT(fmt, args...) \
    printk(KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
            __FILE__, __func__, __LINE__, ##args)
#else
#define DEBUG_PRINT(fmt, args...)
#endif

#define BSP_LOG_R(fmt, args...) \
    _bsp_log (LOG_READ, KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
            __FILE__, __func__, __LINE__, ##args)
#define BSP_LOG_W(fmt, args...) \
    _bsp_log (LOG_WRITE, KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
            __FILE__, __func__, __LINE__, ##args)

#define I2C_READ_BYTE_DATA(ret, lock, i2c_client, reg) \
{ \
    mutex_lock(lock); \
    ret = i2c_smbus_read_byte_data(i2c_client, reg); \
    mutex_unlock(lock); \
    BSP_LOG_R("cpld[%d], reg=0x%03x, reg_val=0x%02x", data->index, reg, ret); \
}

#define I2C_WRITE_BYTE_DATA(ret, lock, i2c_client, reg, val) \
{ \
    mutex_lock(lock); \
    ret = i2c_smbus_write_byte_data(i2c_client, reg, val); \
    mutex_unlock(lock); \
    BSP_LOG_W("cpld[%d], reg=0x%03x, reg_val=0x%02x", data->index, reg, val); \
}

#define _DEVICE_ATTR(_name)     \
    &sensor_dev_attr_##_name.dev_attr.attr


/* CPLD sysfs attributes index  */
enum cpld_sysfs_attributes {

    // CPLD Common
    CPLD_MINOR_VER,
    CPLD_MAJOR_VER,
    CPLD_ID,
    CPLD_BUILD_VER,
    CPLD_VERSION_H,
    CPLD_WRITE_PROTECT,

    // CPLD 1
    CPLD_SKU_ID,
    CPLD_HW_REV,
    CPLD_DEPH_REV,
    CPLD_BUILD_REV,
    CPLD_BRD_ID_TYPE,
    CPLD_CHIP_TYPE,
    BOARD_ID_INFO,
    CPLD_2_INTR,
    CPLD_3_INTR,
    FPGA_INTR,
    EVENT_DETECT_CTRL,

    MAC_RST,
    MAC_QSPI_FLASH_RST,
    BCM85344_RST, 
    BCM81384_RST,
    SPI_BIOS_RST,
    NTM_RST,
    BMC_RST,
    BMC_PCIE_RST,
    BMC_LPC_RST,
    BMC_WDT_RST,

    CPLD_2_3_RST,
    FPGA_RST,
    I2C_MUX_1_RST,
    I2C_MUX_2_RST,
    I2C_MUX_3_RST,
    IOEXP_1_RST,
    IOEXP_2_RST,
    FAN_I2C_MUX_RST,

    PSU_0_PRSNT,
    PSU_1_PRSNT,
    PSU_0_ACIN,
    PSU_1_ACIN,
    PSU_0_PG,
    PSU_1_PG,

    CPU_PG,
    MB_CPU_PG,
    BMC_PG,
    MAC_ROV_STATUS,
    CPU_MUX_SEL,
    PSU_MUX_SEL,
    // LED STATUS
    SYSTEM_LED_STATUS,
    FAN_LED_STATUS,
    PSU_0_LED_STATUS,
    PSU_1_LED_STATUS,
    SYNC_LED_STATUS,
    ID_LED_STATUS,
    // PORT FUNCTION
    SFP28_P0_ABS,
    SFP28_P1_ABS,
    SFP28_P2_ABS,
    SFP28_P3_ABS,
    SFP28_P4_ABS,
    SFP28_P5_ABS,
    SFP28_P6_ABS,
    SFP28_P7_ABS,
    SFP28_P8_ABS,
    SFP28_P9_ABS,
    SFP28_P10_ABS,
    SFP28_P11_ABS,
    SFP28_P12_ABS,
    SFP28_P13_ABS,
    SFP28_P14_ABS,
    SFP28_P15_ABS,
    SFP28_P16_ABS,
    SFP28_P17_ABS,
    SFP28_P18_ABS,
    SFP28_P19_ABS,
    SFP28_P20_ABS,
    SFP28_P21_ABS,
    SFP28_P22_ABS,
    SFP28_P23_ABS, 
    SFP28_P24_ABS,
    SFP28_P25_ABS,
    SFP28_P26_ABS,
    SFP28_P27_ABS,
    SFP28_P28_ABS,
    SFP28_P29_ABS,
    SFP28_P30_ABS,
    SFP28_P31_ABS,
    SFP28_P32_ABS,
    SFP28_P33_ABS,
    SFP28_P34_ABS,
    SFP28_P35_ABS, 
    SFP28_P36_ABS,
    SFP28_P37_ABS,
    SFP28_P38_ABS,
    SFP28_P39_ABS,
    SFP56_P40_ABS,
    SFP56_P41_ABS,
    SFP56_P42_ABS,
    SFP56_P43_ABS,
    SFP56_P44_ABS,
    SFP56_P45_ABS, 
    SFP56_P46_ABS,
    SFP56_P47_ABS,
    QSFP28_P48_ABS,
    QSFP28_P49_ABS,
    QSFP28_P50_ABS,
    QSFP28_P51_ABS,
    QSFPDD_P52_ABS,
    QSFPDD_P53_ABS,
    SFP28_P0_I2C_STUCK,
    SFP28_P1_I2C_STUCK,
    SFP28_P2_I2C_STUCK,
    SFP28_P3_I2C_STUCK,
    SFP28_P4_I2C_STUCK,
    SFP28_P5_I2C_STUCK,
    SFP28_P6_I2C_STUCK,
    SFP28_P7_I2C_STUCK,
    SFP28_P8_I2C_STUCK,
    SFP28_P9_I2C_STUCK,
    SFP28_P10_I2C_STUCK,
    SFP28_P11_I2C_STUCK,
    SFP28_P12_I2C_STUCK,
    SFP28_P13_I2C_STUCK,
    SFP28_P14_I2C_STUCK,
    SFP28_P15_I2C_STUCK,
    SFP28_P16_I2C_STUCK,
    SFP28_P17_I2C_STUCK,
    SFP28_P18_I2C_STUCK,
    SFP28_P19_I2C_STUCK,
    SFP28_P20_I2C_STUCK,
    SFP28_P21_I2C_STUCK,
    SFP28_P22_I2C_STUCK,
    SFP28_P23_I2C_STUCK, 
    SFP28_P24_I2C_STUCK,
    SFP28_P25_I2C_STUCK,
    SFP28_P26_I2C_STUCK,
    SFP28_P27_I2C_STUCK,
    SFP28_P28_I2C_STUCK,
    SFP28_P29_I2C_STUCK,
    SFP28_P30_I2C_STUCK,
    SFP28_P31_I2C_STUCK,
    SFP28_P32_I2C_STUCK,
    SFP28_P33_I2C_STUCK,
    SFP28_P34_I2C_STUCK,
    SFP28_P35_I2C_STUCK, 
    SFP28_P36_I2C_STUCK,
    SFP28_P37_I2C_STUCK,
    SFP28_P38_I2C_STUCK,
    SFP28_P39_I2C_STUCK,
    SFP56_P40_I2C_STUCK,
    SFP56_P41_I2C_STUCK,
    SFP56_P42_I2C_STUCK,
    SFP56_P43_I2C_STUCK,
    SFP56_P44_I2C_STUCK,
    SFP56_P45_I2C_STUCK, 
    SFP56_P46_I2C_STUCK,
    SFP56_P47_I2C_STUCK,
    QSFP28_P48_I2C_STUCK,
    QSFP28_P49_I2C_STUCK,
    QSFP28_P50_I2C_STUCK,
    QSFP28_P51_I2C_STUCK,
    QSFPDD_P52_I2C_STUCK,
    QSFPDD_P53_I2C_STUCK,
    PORT_0_7_ABS_EVENT,
    PORT_8_15_ABS_EVENT,
    PORT_16_23_ABS_EVENT, 
    PORT_24_27_ABS_EVENT,
    PORT_28_35_ABS_EVENT,
    PORT_36_43_ABS_EVENT,
    PORT_44_51_ABS_EVENT,
    PORT_52_53_ABS_EVENT,
    SFP28_P0_TXDIS,
    SFP28_P1_TXDIS,
    SFP28_P2_TXDIS,
    SFP28_P3_TXDIS,
    SFP28_P4_TXDIS,
    SFP28_P5_TXDIS,
    SFP28_P6_TXDIS,
    SFP28_P7_TXDIS,
    SFP28_P8_TXDIS,
    SFP28_P9_TXDIS,
    SFP28_P10_TXDIS,
    SFP28_P11_TXDIS,
    SFP28_P12_TXDIS,
    SFP28_P13_TXDIS,
    SFP28_P14_TXDIS,
    SFP28_P15_TXDIS,
    SFP28_P16_TXDIS,
    SFP28_P17_TXDIS,
    SFP28_P18_TXDIS,
    SFP28_P19_TXDIS,
    SFP28_P20_TXDIS,
    SFP28_P21_TXDIS,
    SFP28_P22_TXDIS,
    SFP28_P23_TXDIS, 
    SFP28_P24_TXDIS,
    SFP28_P25_TXDIS,
    SFP28_P26_TXDIS,
    SFP28_P27_TXDIS,
    SFP28_P28_TXDIS,
    SFP28_P29_TXDIS,
    SFP28_P30_TXDIS,
    SFP28_P31_TXDIS,
    SFP28_P32_TXDIS,
    SFP28_P33_TXDIS,
    SFP28_P34_TXDIS,
    SFP28_P35_TXDIS, 
    SFP28_P36_TXDIS,
    SFP28_P37_TXDIS,
    SFP28_P38_TXDIS,
    SFP28_P39_TXDIS,
    SFP56_P40_TXDIS,
    SFP56_P41_TXDIS,
    SFP56_P42_TXDIS,
    SFP56_P43_TXDIS,
    SFP56_P44_TXDIS,
    SFP56_P45_TXDIS, 
    SFP56_P46_TXDIS,
    SFP56_P47_TXDIS,
    SFP28_P0_RS,
    SFP28_P1_RS,
    SFP28_P2_RS,
    SFP28_P3_RS,
    SFP28_P4_RS,
    SFP28_P5_RS,
    SFP28_P6_RS,
    SFP28_P7_RS,
    SFP28_P8_RS,
    SFP28_P9_RS,
    SFP28_P10_RS,
    SFP28_P11_RS,
    SFP28_P12_RS,
    SFP28_P13_RS,
    SFP28_P14_RS,
    SFP28_P15_RS,
    SFP28_P16_RS,
    SFP28_P17_RS,
    SFP28_P18_RS,
    SFP28_P19_RS,
    SFP28_P20_RS,
    SFP28_P21_RS,
    SFP28_P22_RS,
    SFP28_P23_RS, 
    SFP28_P24_RS,
    SFP28_P25_RS,
    SFP28_P26_RS,
    SFP28_P27_RS,
    SFP28_P28_RS,
    SFP28_P29_RS,
    SFP28_P30_RS,
    SFP28_P31_RS,
    SFP28_P32_RS,
    SFP28_P33_RS,
    SFP28_P34_RS,
    SFP28_P35_RS, 
    SFP28_P36_RS,
    SFP28_P37_RS,
    SFP28_P38_RS,
    SFP28_P39_RS,
    SFP56_P40_RS,
    SFP56_P41_RS,
    SFP56_P42_RS,
    SFP56_P43_RS,
    SFP56_P44_RS,
    SFP56_P45_RS, 
    SFP56_P46_RS,
    SFP56_P47_RS,
    SFP28_P0_RXLOS,
    SFP28_P1_RXLOS,
    SFP28_P2_RXLOS,
    SFP28_P3_RXLOS,
    SFP28_P4_RXLOS,
    SFP28_P5_RXLOS,
    SFP28_P6_RXLOS,
    SFP28_P7_RXLOS,
    SFP28_P8_RXLOS,
    SFP28_P9_RXLOS,
    SFP28_P10_RXLOS,
    SFP28_P11_RXLOS,
    SFP28_P12_RXLOS,
    SFP28_P13_RXLOS,
    SFP28_P14_RXLOS,
    SFP28_P15_RXLOS,
    SFP28_P16_RXLOS,
    SFP28_P17_RXLOS,
    SFP28_P18_RXLOS,
    SFP28_P19_RXLOS,
    SFP28_P20_RXLOS,
    SFP28_P21_RXLOS,
    SFP28_P22_RXLOS,
    SFP28_P23_RXLOS, 
    SFP28_P24_RXLOS,
    SFP28_P25_RXLOS,
    SFP28_P26_RXLOS,
    SFP28_P27_RXLOS,
    SFP28_P28_RXLOS,
    SFP28_P29_RXLOS,
    SFP28_P30_RXLOS,
    SFP28_P31_RXLOS,
    SFP28_P32_RXLOS,
    SFP28_P33_RXLOS,
    SFP28_P34_RXLOS,
    SFP28_P35_RXLOS, 
    SFP28_P36_RXLOS,
    SFP28_P37_RXLOS,
    SFP28_P38_RXLOS,
    SFP28_P39_RXLOS,
    SFP56_P40_RXLOS,
    SFP56_P41_RXLOS,
    SFP56_P42_RXLOS,
    SFP56_P43_RXLOS,
    SFP56_P44_RXLOS,
    SFP56_P45_RXLOS, 
    SFP56_P46_RXLOS,
    SFP56_P47_RXLOS,
    SFP28_P0_TXFLT,
    SFP28_P1_TXFLT,
    SFP28_P2_TXFLT,
    SFP28_P3_TXFLT,
    SFP28_P4_TXFLT,
    SFP28_P5_TXFLT,
    SFP28_P6_TXFLT,
    SFP28_P7_TXFLT,
    SFP28_P8_TXFLT,
    SFP28_P9_TXFLT,
    SFP28_P10_TXFLT,
    SFP28_P11_TXFLT,
    SFP28_P12_TXFLT,
    SFP28_P13_TXFLT,
    SFP28_P14_TXFLT,
    SFP28_P15_TXFLT,
    SFP28_P16_TXFLT,
    SFP28_P17_TXFLT,
    SFP28_P18_TXFLT,
    SFP28_P19_TXFLT,
    SFP28_P20_TXFLT,
    SFP28_P21_TXFLT,
    SFP28_P22_TXFLT,
    SFP28_P23_TXFLT, 
    SFP28_P24_TXFLT,
    SFP28_P25_TXFLT,
    SFP28_P26_TXFLT,
    SFP28_P27_TXFLT,
    SFP28_P28_TXFLT,
    SFP28_P29_TXFLT,
    SFP28_P30_TXFLT,
    SFP28_P31_TXFLT,
    SFP28_P32_TXFLT,
    SFP28_P33_TXFLT,
    SFP28_P34_TXFLT,
    SFP28_P35_TXFLT, 
    SFP28_P36_TXFLT,
    SFP28_P37_TXFLT,
    SFP28_P38_TXFLT,
    SFP28_P39_TXFLT,
    SFP56_P40_TXFLT,
    SFP56_P41_TXFLT,
    SFP56_P42_TXFLT,
    SFP56_P43_TXFLT,
    SFP56_P44_TXFLT,
    SFP56_P45_TXFLT, 
    SFP56_P46_TXFLT,
    SFP56_P47_TXFLT,
    QSFPDD_P52_EFUSE_PG,
    QSFPDD_P53_EFUSE_PG,
    QSFP28_P48_INTR,
    QSFP28_P49_INTR,
    QSFP28_P50_INTR,
    QSFP28_P51_INTR,
    QSFPDD_P52_INTR,
    QSFPDD_P53_INTR,
    QSFP28_P48_RST,
    QSFP28_P49_RST,
    QSFP28_P50_RST,
    QSFP28_P51_RST,
    QSFPDD_P52_RST,
    QSFPDD_P53_RST,
    QSFP28_P48_LP_MODE,
    QSFP28_P49_LP_MODE,
    QSFP28_P50_LP_MODE,
    QSFP28_P51_LP_MODE,
    QSFPDD_P52_LP_MODE,
    QSFPDD_P53_LP_MODE,

    //FPGA
    FPGA_ID,
    FPGA_VER_1,
    FPGA_MINOR_VER,
    FPGA_MAJOR_VER,
    FPGA_BUILD_VER,
    FPGA_VERSION_H,
    FPGA_DEV_INFO,
    
    //MUX
    IDLE_STATE,

    //BSP DEBUG
    BSP_DEBUG
};

enum data_type {
    DATA_HEX,
    DATA_DEC,
    DATA_UNK,
};

enum reg_write_protect
{
    REG_WP_DIS,
    REG_WP_EN
};

typedef struct  {
    u16 reg;
    u8 mask;
    u8 data_type;
    bool write_protect;
} attr_reg_map_t;

static attr_reg_map_t attr_reg[]= {

    // CPLD Common
    [CPLD_MINOR_VER]                       =  {CPLD_VERSION_REG                        , MASK_0011_1111, DATA_DEC, REG_WP_DIS},
    [CPLD_MAJOR_VER]                       =  {CPLD_VERSION_REG                        , MASK_1100_0000, DATA_DEC, REG_WP_DIS},
    [CPLD_ID]                              =  {CPLD_ID_REG                             , MASK_0000_0111, DATA_DEC, REG_WP_DIS},
    [CPLD_BUILD_VER]                       =  {CPLD_SUB_VERSION_REG                    , MASK_ALL      , DATA_DEC, REG_WP_DIS},
    [CPLD_VERSION_H]                       =  {NONE_REG                                , MASK_NONE     , DATA_UNK, REG_WP_DIS},
    [CPLD_WRITE_PROTECT]                   =  {CPLD_WRITE_PROTECT_REG                  , MASK_0000_0001, DATA_HEX, REG_WP_DIS}, 

    //CPLD 1
    [CPLD_SKU_ID]                          =  {CPLD_SKU_ID_REG                         , MASK_0011_1111, DATA_DEC, REG_WP_DIS},
    [CPLD_HW_REV]                          =  {CPLD_HW_BUILD_REV_REG                   , MASK_0000_0011, DATA_DEC, REG_WP_DIS},
    [CPLD_DEPH_REV]                        =  {CPLD_HW_BUILD_REV_REG                   , MASK_0000_0100, DATA_DEC, REG_WP_DIS},
    [CPLD_BUILD_REV]                       =  {CPLD_HW_BUILD_REV_REG                   , MASK_0011_1000, DATA_DEC, REG_WP_DIS},
    [CPLD_BRD_ID_TYPE]                     =  {CPLD_HW_BUILD_REV_REG                   , MASK_1000_0000, DATA_DEC, REG_WP_DIS},
    [CPLD_CHIP_TYPE]                       =  {CPLD_CHIP_TYPE_REG                      , MASK_0000_0011, DATA_DEC, REG_WP_DIS},
    [FPGA_INTR]                            =  {MISC_INTR_REG                           , MASK_0000_1000, DATA_HEX, REG_WP_DIS},
    [EVENT_DETECT_CTRL]                    =  {EVENT_DETECT_CTRL_REG                   , MASK_0000_0001, DATA_HEX, REG_WP_DIS},
    [NTM_RST]                              =  {BMC_NTM_RST_REG                         , MASK_0000_0010, DATA_HEX, REG_WP_EN },
    [BMC_RST]                              =  {BMC_NTM_RST_REG                         , MASK_0000_0001, DATA_HEX, REG_WP_EN },
    [CPLD_2_3_RST]                         =  {MISC_RST_1_REG                          , MASK_0000_0001, DATA_HEX, REG_WP_EN },      
    [FPGA_RST]                             =  {MISC_RST_1_REG                          , MASK_0001_0000, DATA_HEX, REG_WP_EN },  
    [I2C_MUX_1_RST]                        =  {MISC_RST_1_REG                          , MASK_0010_0000, DATA_HEX, REG_WP_EN },       
    [I2C_MUX_2_RST]                        =  {MISC_RST_1_REG                          , MASK_0100_0000, DATA_HEX, REG_WP_EN },       
    [I2C_MUX_3_RST]                        =  {MISC_RST_2_REG                          , MASK_0000_0100, DATA_HEX, REG_WP_EN },       
    [IOEXP_1_RST]                          =  {MISC_RST_1_REG                          , MASK_0000_1000, DATA_HEX, REG_WP_EN },        
    [IOEXP_2_RST]                          =  {MISC_RST_2_REG                          , MASK_0000_0010, DATA_HEX, REG_WP_EN },
    [FAN_I2C_MUX_RST]                      =  {MISC_RST_2_REG                          , MASK_0000_0001, DATA_HEX, REG_WP_EN },         
    [PSU_0_PRSNT]                          =  {PSU_STATUS_REG                          , MASK_0000_0001, DATA_HEX, REG_WP_DIS},
    [PSU_1_PRSNT]                          =  {PSU_STATUS_REG                          , MASK_0000_0010, DATA_HEX, REG_WP_DIS},
    [PSU_0_ACIN]                           =  {PSU_STATUS_REG                          , MASK_0000_0100, DATA_HEX, REG_WP_DIS},
    [PSU_1_ACIN]                           =  {PSU_STATUS_REG                          , MASK_0000_1000, DATA_HEX, REG_WP_DIS},
    [PSU_0_PG]                             =  {PSU_STATUS_REG                          , MASK_0001_0000, DATA_HEX, REG_WP_DIS},
    [PSU_1_PG]                             =  {PSU_STATUS_REG                          , MASK_0010_0000, DATA_HEX, REG_WP_DIS},
    [CPU_PG]                               =  {SYS_PW_STATUS_REG                       , MASK_0000_0001, DATA_HEX, REG_WP_DIS},
    [MB_CPU_PG]                            =  {SYS_PW_STATUS_REG                       , MASK_0000_0010, DATA_HEX, REG_WP_DIS},
    [BMC_PG]                               =  {SYS_PW_STATUS_REG                       , MASK_0000_0100, DATA_HEX, REG_WP_DIS},
    [CPU_MUX_SEL]                          =  {MUX_CTRL_REG                            , MASK_0000_0001, DATA_HEX, REG_WP_DIS},
    [PSU_MUX_SEL]                          =  {MUX_CTRL_REG                            , MASK_0000_0011, DATA_HEX, REG_WP_DIS},
    [SYSTEM_LED_STATUS]                    =  {SYSTEM_LED_CTRL_1_REG                   , MASK_0000_1111, DATA_HEX, REG_WP_DIS},
    [FAN_LED_STATUS]                       =  {SYSTEM_LED_CTRL_1_REG                   , MASK_1111_0000, DATA_HEX, REG_WP_DIS},
    [PSU_0_LED_STATUS]                     =  {SYSTEM_LED_CTRL_2_REG                   , MASK_0000_1111, DATA_HEX, REG_WP_DIS},
    [PSU_1_LED_STATUS]                     =  {SYSTEM_LED_CTRL_2_REG                   , MASK_1111_0000, DATA_HEX, REG_WP_DIS},
    [SYNC_LED_STATUS]                      =  {SYSTEM_LED_CTRL_3_REG                   , MASK_0000_1111, DATA_HEX, REG_WP_DIS},
    [ID_LED_STATUS]                        =  {SYSTEM_LED_CTRL_3_REG                   , MASK_1110_0000, DATA_HEX, REG_WP_DIS},
    // PORT FUNCTION
    [SFP28_P0_ABS]                         =  {PORT_0_7_ABS_REG                        , MASK_0000_0001, DATA_DEC, REG_WP_DIS},    
    [SFP28_P1_ABS]                         =  {PORT_0_7_ABS_REG                        , MASK_0000_0010, DATA_DEC, REG_WP_DIS},    
    [SFP28_P2_ABS]                         =  {PORT_0_7_ABS_REG                        , MASK_0000_0100, DATA_DEC, REG_WP_DIS},    
    [SFP28_P3_ABS]                         =  {PORT_0_7_ABS_REG                        , MASK_0000_1000, DATA_DEC, REG_WP_DIS},    
    [SFP28_P4_ABS]                         =  {PORT_0_7_ABS_REG                        , MASK_0001_0000, DATA_DEC, REG_WP_DIS},    
    [SFP28_P5_ABS]                         =  {PORT_0_7_ABS_REG                        , MASK_0010_0000, DATA_DEC, REG_WP_DIS},    
    [SFP28_P6_ABS]                         =  {PORT_0_7_ABS_REG                        , MASK_0100_0000, DATA_DEC, REG_WP_DIS},    
    [SFP28_P7_ABS]                         =  {PORT_0_7_ABS_REG                        , MASK_1000_0000, DATA_DEC, REG_WP_DIS},    
    [SFP28_P8_ABS]                         =  {PORT_8_15_ABS_REG                       , MASK_0000_0001, DATA_DEC, REG_WP_DIS},    
    [SFP28_P9_ABS]                         =  {PORT_8_15_ABS_REG                       , MASK_0000_0010, DATA_DEC, REG_WP_DIS},    
    [SFP28_P10_ABS]                        =  {PORT_8_15_ABS_REG                       , MASK_0000_0100, DATA_DEC, REG_WP_DIS},     
    [SFP28_P11_ABS]                        =  {PORT_8_15_ABS_REG                       , MASK_0000_1000, DATA_DEC, REG_WP_DIS},     
    [SFP28_P12_ABS]                        =  {PORT_8_15_ABS_REG                       , MASK_0001_0000, DATA_DEC, REG_WP_DIS},     
    [SFP28_P13_ABS]                        =  {PORT_8_15_ABS_REG                       , MASK_0010_0000, DATA_DEC, REG_WP_DIS},     
    [SFP28_P14_ABS]                        =  {PORT_8_15_ABS_REG                       , MASK_0100_0000, DATA_DEC, REG_WP_DIS},     
    [SFP28_P15_ABS]                        =  {PORT_8_15_ABS_REG                       , MASK_1000_0000, DATA_DEC, REG_WP_DIS},     
    [SFP28_P16_ABS]                        =  {PORT_16_23_ABS_REG                      , MASK_0000_0001, DATA_DEC, REG_WP_DIS},     
    [SFP28_P17_ABS]                        =  {PORT_16_23_ABS_REG                      , MASK_0000_0010, DATA_DEC, REG_WP_DIS},     
    [SFP28_P18_ABS]                        =  {PORT_16_23_ABS_REG                      , MASK_0000_0100, DATA_DEC, REG_WP_DIS},     
    [SFP28_P19_ABS]                        =  {PORT_16_23_ABS_REG                      , MASK_0000_1000, DATA_DEC, REG_WP_DIS},     
    [SFP28_P20_ABS]                        =  {PORT_16_23_ABS_REG                      , MASK_0001_0000, DATA_DEC, REG_WP_DIS},     
    [SFP28_P21_ABS]                        =  {PORT_16_23_ABS_REG                      , MASK_0010_0000, DATA_DEC, REG_WP_DIS},     
    [SFP28_P22_ABS]                        =  {PORT_16_23_ABS_REG                      , MASK_0100_0000, DATA_DEC, REG_WP_DIS},     
    [SFP28_P23_ABS]                        =  {PORT_16_23_ABS_REG                      , MASK_1000_0000, DATA_DEC, REG_WP_DIS},      
    [SFP28_P24_ABS]                        =  {PORT_24_27_ABS_REG                      , MASK_0000_0001, DATA_DEC, REG_WP_DIS},     
    [SFP28_P25_ABS]                        =  {PORT_24_27_ABS_REG                      , MASK_0000_0010, DATA_DEC, REG_WP_DIS},     
    [SFP28_P26_ABS]                        =  {PORT_24_27_ABS_REG                      , MASK_0000_0100, DATA_DEC, REG_WP_DIS},     
    [SFP28_P27_ABS]                        =  {PORT_24_27_ABS_REG                      , MASK_0000_1000, DATA_DEC, REG_WP_DIS},     
    [SFP28_P28_ABS]                        =  {PORT_28_35_ABS_REG                      , MASK_0000_0001, DATA_DEC, REG_WP_DIS},     
    [SFP28_P29_ABS]                        =  {PORT_28_35_ABS_REG                      , MASK_0000_0010, DATA_DEC, REG_WP_DIS},     
    [SFP28_P30_ABS]                        =  {PORT_28_35_ABS_REG                      , MASK_0000_0100, DATA_DEC, REG_WP_DIS},     
    [SFP28_P31_ABS]                        =  {PORT_28_35_ABS_REG                      , MASK_0000_1000, DATA_DEC, REG_WP_DIS},     
    [SFP28_P32_ABS]                        =  {PORT_28_35_ABS_REG                      , MASK_0001_0000, DATA_DEC, REG_WP_DIS},     
    [SFP28_P33_ABS]                        =  {PORT_28_35_ABS_REG                      , MASK_0010_0000, DATA_DEC, REG_WP_DIS},     
    [SFP28_P34_ABS]                        =  {PORT_28_35_ABS_REG                      , MASK_0100_0000, DATA_DEC, REG_WP_DIS},     
    [SFP28_P35_ABS]                        =  {PORT_28_35_ABS_REG                      , MASK_1000_0000, DATA_DEC, REG_WP_DIS},      
    [SFP28_P36_ABS]                        =  {PORT_36_43_ABS_REG                      , MASK_0000_0001, DATA_DEC, REG_WP_DIS},     
    [SFP28_P37_ABS]                        =  {PORT_36_43_ABS_REG                      , MASK_0000_0010, DATA_DEC, REG_WP_DIS},     
    [SFP28_P38_ABS]                        =  {PORT_36_43_ABS_REG                      , MASK_0000_0100, DATA_DEC, REG_WP_DIS},     
    [SFP28_P39_ABS]                        =  {PORT_36_43_ABS_REG                      , MASK_0000_1000, DATA_DEC, REG_WP_DIS},     
    [SFP56_P40_ABS]                        =  {PORT_36_43_ABS_REG                      , MASK_0001_0000, DATA_DEC, REG_WP_DIS},     
    [SFP56_P41_ABS]                        =  {PORT_36_43_ABS_REG                      , MASK_0010_0000, DATA_DEC, REG_WP_DIS},     
    [SFP56_P42_ABS]                        =  {PORT_36_43_ABS_REG                      , MASK_0100_0000, DATA_DEC, REG_WP_DIS},     
    [SFP56_P43_ABS]                        =  {PORT_36_43_ABS_REG                      , MASK_1000_0000, DATA_DEC, REG_WP_DIS},     
    [SFP56_P44_ABS]                        =  {PORT_44_51_ABS_REG                      , MASK_0000_0001, DATA_DEC, REG_WP_DIS},     
    [SFP56_P45_ABS]                        =  {PORT_44_51_ABS_REG                      , MASK_0000_0010, DATA_DEC, REG_WP_DIS},      
    [SFP56_P46_ABS]                        =  {PORT_44_51_ABS_REG                      , MASK_0000_0100, DATA_DEC, REG_WP_DIS},     
    [SFP56_P47_ABS]                        =  {PORT_44_51_ABS_REG                      , MASK_0000_1000, DATA_DEC, REG_WP_DIS},     
    [QSFP28_P48_ABS]                       =  {PORT_44_51_ABS_REG                      , MASK_0001_0000, DATA_DEC, REG_WP_DIS},      
    [QSFP28_P49_ABS]                       =  {PORT_44_51_ABS_REG                      , MASK_0010_0000, DATA_DEC, REG_WP_DIS},      
    [QSFP28_P50_ABS]                       =  {PORT_44_51_ABS_REG                      , MASK_0100_0000, DATA_DEC, REG_WP_DIS},      
    [QSFP28_P51_ABS]                       =  {PORT_44_51_ABS_REG                      , MASK_1000_0000, DATA_DEC, REG_WP_DIS},      
    [QSFPDD_P52_ABS]                       =  {PORT_52_53_ABS_REG                      , MASK_0000_0001, DATA_DEC, REG_WP_DIS},      
    [QSFPDD_P53_ABS]                       =  {PORT_52_53_ABS_REG                      , MASK_0000_0010, DATA_DEC, REG_WP_DIS},
    [SFP28_P0_I2C_STUCK]                   =  {PORT_0_7_I2C_STUCK_REG                  , MASK_0000_0001, DATA_DEC, REG_WP_DIS},
    [SFP28_P1_I2C_STUCK]                   =  {PORT_0_7_I2C_STUCK_REG                  , MASK_0000_0010, DATA_DEC, REG_WP_DIS},
    [SFP28_P2_I2C_STUCK]                   =  {PORT_0_7_I2C_STUCK_REG                  , MASK_0000_0100, DATA_DEC, REG_WP_DIS},
    [SFP28_P3_I2C_STUCK]                   =  {PORT_0_7_I2C_STUCK_REG                  , MASK_0000_1000, DATA_DEC, REG_WP_DIS},
    [SFP28_P4_I2C_STUCK]                   =  {PORT_0_7_I2C_STUCK_REG                  , MASK_0001_0000, DATA_DEC, REG_WP_DIS},
    [SFP28_P5_I2C_STUCK]                   =  {PORT_0_7_I2C_STUCK_REG                  , MASK_0010_0000, DATA_DEC, REG_WP_DIS},
    [SFP28_P6_I2C_STUCK]                   =  {PORT_0_7_I2C_STUCK_REG                  , MASK_0100_0000, DATA_DEC, REG_WP_DIS},
    [SFP28_P7_I2C_STUCK]                   =  {PORT_0_7_I2C_STUCK_REG                  , MASK_1000_0000, DATA_DEC, REG_WP_DIS},
    [SFP28_P8_I2C_STUCK]                   =  {PORT_8_15_I2C_STUCK_REG                 , MASK_0000_0001, DATA_DEC, REG_WP_DIS},
    [SFP28_P9_I2C_STUCK]                   =  {PORT_8_15_I2C_STUCK_REG                 , MASK_0000_0010, DATA_DEC, REG_WP_DIS},
    [SFP28_P10_I2C_STUCK]                  =  {PORT_8_15_I2C_STUCK_REG                 , MASK_0000_0100, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P11_I2C_STUCK]                  =  {PORT_8_15_I2C_STUCK_REG                 , MASK_0000_1000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P12_I2C_STUCK]                  =  {PORT_8_15_I2C_STUCK_REG                 , MASK_0001_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P13_I2C_STUCK]                  =  {PORT_8_15_I2C_STUCK_REG                 , MASK_0010_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P14_I2C_STUCK]                  =  {PORT_8_15_I2C_STUCK_REG                 , MASK_0100_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P15_I2C_STUCK]                  =  {PORT_8_15_I2C_STUCK_REG                 , MASK_1000_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P16_I2C_STUCK]                  =  {PORT_16_23_I2C_STUCK_REG                , MASK_0000_0001, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P17_I2C_STUCK]                  =  {PORT_16_23_I2C_STUCK_REG                , MASK_0000_0010, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P18_I2C_STUCK]                  =  {PORT_16_23_I2C_STUCK_REG                , MASK_0000_0100, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P19_I2C_STUCK]                  =  {PORT_16_23_I2C_STUCK_REG                , MASK_0000_1000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P20_I2C_STUCK]                  =  {PORT_16_23_I2C_STUCK_REG                , MASK_0001_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P21_I2C_STUCK]                  =  {PORT_16_23_I2C_STUCK_REG                , MASK_0010_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P22_I2C_STUCK]                  =  {PORT_16_23_I2C_STUCK_REG                , MASK_0100_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P23_I2C_STUCK]                  =  {PORT_16_23_I2C_STUCK_REG                , MASK_1000_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P24_I2C_STUCK]                  =  {PORT_24_27_I2C_STUCK_REG                , MASK_0000_0001, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P25_I2C_STUCK]                  =  {PORT_24_27_I2C_STUCK_REG                , MASK_0000_0010, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P26_I2C_STUCK]                  =  {PORT_24_27_I2C_STUCK_REG                , MASK_0000_0100, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P27_I2C_STUCK]                  =  {PORT_24_27_I2C_STUCK_REG                , MASK_0000_1000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P28_I2C_STUCK]                  =  {PORT_28_35_I2C_STUCK_REG                , MASK_0000_0001, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P29_I2C_STUCK]                  =  {PORT_28_35_I2C_STUCK_REG                , MASK_0000_0010, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P30_I2C_STUCK]                  =  {PORT_28_35_I2C_STUCK_REG                , MASK_0000_0100, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P31_I2C_STUCK]                  =  {PORT_28_35_I2C_STUCK_REG                , MASK_0000_1000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P32_I2C_STUCK]                  =  {PORT_28_35_I2C_STUCK_REG                , MASK_0001_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P33_I2C_STUCK]                  =  {PORT_28_35_I2C_STUCK_REG                , MASK_0010_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P34_I2C_STUCK]                  =  {PORT_28_35_I2C_STUCK_REG                , MASK_0100_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P35_I2C_STUCK]                  =  {PORT_28_35_I2C_STUCK_REG                , MASK_1000_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P36_I2C_STUCK]                  =  {PORT_36_43_I2C_STUCK_REG                , MASK_0000_0001, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P37_I2C_STUCK]                  =  {PORT_36_43_I2C_STUCK_REG                , MASK_0000_0010, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P38_I2C_STUCK]                  =  {PORT_36_43_I2C_STUCK_REG                , MASK_0000_0100, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P39_I2C_STUCK]                  =  {PORT_36_43_I2C_STUCK_REG                , MASK_0000_1000, DATA_DEC, REG_WP_DIS}, 
    [SFP56_P40_I2C_STUCK]                  =  {PORT_36_43_I2C_STUCK_REG                , MASK_0001_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP56_P41_I2C_STUCK]                  =  {PORT_36_43_I2C_STUCK_REG                , MASK_0010_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP56_P42_I2C_STUCK]                  =  {PORT_36_43_I2C_STUCK_REG                , MASK_0100_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP56_P43_I2C_STUCK]                  =  {PORT_36_43_I2C_STUCK_REG                , MASK_1000_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP56_P44_I2C_STUCK]                  =  {PORT_44_51_I2C_STUCK_REG                , MASK_0000_0001, DATA_DEC, REG_WP_DIS}, 
    [SFP56_P45_I2C_STUCK]                  =  {PORT_44_51_I2C_STUCK_REG                , MASK_0000_0010, DATA_DEC, REG_WP_DIS}, 
    [SFP56_P46_I2C_STUCK]                  =  {PORT_44_51_I2C_STUCK_REG                , MASK_0000_0100, DATA_DEC, REG_WP_DIS}, 
    [SFP56_P47_I2C_STUCK]                  =  {PORT_44_51_I2C_STUCK_REG                , MASK_0000_1000, DATA_DEC, REG_WP_DIS}, 
    [QSFP28_P48_I2C_STUCK]                 =  {PORT_44_51_I2C_STUCK_REG                , MASK_0001_0000, DATA_DEC, REG_WP_DIS},  
    [QSFP28_P49_I2C_STUCK]                 =  {PORT_44_51_I2C_STUCK_REG                , MASK_0010_0000, DATA_DEC, REG_WP_DIS},  
    [QSFP28_P50_I2C_STUCK]                 =  {PORT_44_51_I2C_STUCK_REG                , MASK_0100_0000, DATA_DEC, REG_WP_DIS},  
    [QSFP28_P51_I2C_STUCK]                 =  {PORT_44_51_I2C_STUCK_REG                , MASK_1000_0000, DATA_DEC, REG_WP_DIS},  
    [QSFPDD_P52_I2C_STUCK]                 =  {PORT_52_53_I2C_STUCK_REG                , MASK_0000_0001, DATA_DEC, REG_WP_DIS},  
    [QSFPDD_P53_I2C_STUCK]                 =  {PORT_52_53_I2C_STUCK_REG                , MASK_0000_0010, DATA_DEC, REG_WP_DIS},        
    [PORT_0_7_ABS_EVENT]                   =  {PORT_0_7_ABS_EVENT_REG                  , MASK_ALL      , DATA_HEX, REG_WP_DIS},  
    [PORT_8_15_ABS_EVENT]                  =  {PORT_8_15_ABS_EVENT_REG                 , MASK_ALL      , DATA_HEX, REG_WP_DIS},  
    [PORT_16_23_ABS_EVENT]                 =  {PORT_16_23_ABS_EVENT_REG                , MASK_ALL      , DATA_HEX, REG_WP_DIS},   
    [PORT_24_27_ABS_EVENT]                 =  {PORT_24_27_ABS_EVENT_REG                , MASK_ALL      , DATA_HEX, REG_WP_DIS},   
    [PORT_28_35_ABS_EVENT]                 =  {PORT_28_35_ABS_EVENT_REG                , MASK_ALL      , DATA_HEX, REG_WP_DIS},   
    [PORT_36_43_ABS_EVENT]                 =  {PORT_36_43_ABS_EVENT_REG                , MASK_ALL      , DATA_HEX, REG_WP_DIS},   
    [PORT_44_51_ABS_EVENT]                 =  {PORT_44_51_ABS_EVENT_REG                , MASK_ALL      , DATA_HEX, REG_WP_DIS},   
    [PORT_52_53_ABS_EVENT]                 =  {PORT_52_53_ABS_EVENT_REG                , MASK_ALL      , DATA_HEX, REG_WP_DIS},    
    [SFP28_P0_TXDIS]                       =  {PORT_0_7_TXDIS_REG                      , MASK_0000_0001, DATA_DEC, REG_WP_EN },  
    [SFP28_P1_TXDIS]                       =  {PORT_0_7_TXDIS_REG                      , MASK_0000_0010, DATA_DEC, REG_WP_EN },  
    [SFP28_P2_TXDIS]                       =  {PORT_0_7_TXDIS_REG                      , MASK_0000_0100, DATA_DEC, REG_WP_EN },  
    [SFP28_P3_TXDIS]                       =  {PORT_0_7_TXDIS_REG                      , MASK_0000_1000, DATA_DEC, REG_WP_EN },  
    [SFP28_P4_TXDIS]                       =  {PORT_0_7_TXDIS_REG                      , MASK_0001_0000, DATA_DEC, REG_WP_EN },  
    [SFP28_P5_TXDIS]                       =  {PORT_0_7_TXDIS_REG                      , MASK_0010_0000, DATA_DEC, REG_WP_EN },  
    [SFP28_P6_TXDIS]                       =  {PORT_0_7_TXDIS_REG                      , MASK_0100_0000, DATA_DEC, REG_WP_EN },  
    [SFP28_P7_TXDIS]                       =  {PORT_0_7_TXDIS_REG                      , MASK_1000_0000, DATA_DEC, REG_WP_EN },  
    [SFP28_P8_TXDIS]                       =  {PORT_8_15_TXDIS_REG                     , MASK_0000_0001, DATA_DEC, REG_WP_EN },  
    [SFP28_P9_TXDIS]                       =  {PORT_8_15_TXDIS_REG                     , MASK_0000_0010, DATA_DEC, REG_WP_EN },  
    [SFP28_P10_TXDIS]                      =  {PORT_8_15_TXDIS_REG                     , MASK_0000_0100, DATA_DEC, REG_WP_EN },   
    [SFP28_P11_TXDIS]                      =  {PORT_8_15_TXDIS_REG                     , MASK_0000_1000, DATA_DEC, REG_WP_EN },   
    [SFP28_P12_TXDIS]                      =  {PORT_8_15_TXDIS_REG                     , MASK_0001_0000, DATA_DEC, REG_WP_EN },   
    [SFP28_P13_TXDIS]                      =  {PORT_8_15_TXDIS_REG                     , MASK_0010_0000, DATA_DEC, REG_WP_EN },   
    [SFP28_P14_TXDIS]                      =  {PORT_8_15_TXDIS_REG                     , MASK_0100_0000, DATA_DEC, REG_WP_EN },   
    [SFP28_P15_TXDIS]                      =  {PORT_8_15_TXDIS_REG                     , MASK_1000_0000, DATA_DEC, REG_WP_EN },   
    [SFP28_P16_TXDIS]                      =  {PORT_16_23_TXDIS_REG                    , MASK_0000_0001, DATA_DEC, REG_WP_EN },   
    [SFP28_P17_TXDIS]                      =  {PORT_16_23_TXDIS_REG                    , MASK_0000_0010, DATA_DEC, REG_WP_EN },   
    [SFP28_P18_TXDIS]                      =  {PORT_16_23_TXDIS_REG                    , MASK_0000_0100, DATA_DEC, REG_WP_EN },   
    [SFP28_P19_TXDIS]                      =  {PORT_16_23_TXDIS_REG                    , MASK_0000_1000, DATA_DEC, REG_WP_EN },   
    [SFP28_P20_TXDIS]                      =  {PORT_16_23_TXDIS_REG                    , MASK_0001_0000, DATA_DEC, REG_WP_EN },   
    [SFP28_P21_TXDIS]                      =  {PORT_16_23_TXDIS_REG                    , MASK_0010_0000, DATA_DEC, REG_WP_EN },   
    [SFP28_P22_TXDIS]                      =  {PORT_16_23_TXDIS_REG                    , MASK_0100_0000, DATA_DEC, REG_WP_EN },   
    [SFP28_P23_TXDIS]                      =  {PORT_16_23_TXDIS_REG                    , MASK_1000_0000, DATA_DEC, REG_WP_EN },   
    [SFP28_P24_TXDIS]                      =  {PORT_24_27_TXDIS_REG                    , MASK_0000_0001, DATA_DEC, REG_WP_EN },   
    [SFP28_P25_TXDIS]                      =  {PORT_24_27_TXDIS_REG                    , MASK_0000_0010, DATA_DEC, REG_WP_EN },   
    [SFP28_P26_TXDIS]                      =  {PORT_24_27_TXDIS_REG                    , MASK_0000_0100, DATA_DEC, REG_WP_EN },   
    [SFP28_P27_TXDIS]                      =  {PORT_24_27_TXDIS_REG                    , MASK_0000_1000, DATA_DEC, REG_WP_EN },   
    [SFP28_P28_TXDIS]                      =  {PORT_28_35_TXDIS_REG                    , MASK_0000_0001, DATA_DEC, REG_WP_EN },   
    [SFP28_P29_TXDIS]                      =  {PORT_28_35_TXDIS_REG                    , MASK_0000_0010, DATA_DEC, REG_WP_EN },   
    [SFP28_P30_TXDIS]                      =  {PORT_28_35_TXDIS_REG                    , MASK_0000_0100, DATA_DEC, REG_WP_EN },   
    [SFP28_P31_TXDIS]                      =  {PORT_28_35_TXDIS_REG                    , MASK_0000_1000, DATA_DEC, REG_WP_EN },   
    [SFP28_P32_TXDIS]                      =  {PORT_28_35_TXDIS_REG                    , MASK_0001_0000, DATA_DEC, REG_WP_EN },   
    [SFP28_P33_TXDIS]                      =  {PORT_28_35_TXDIS_REG                    , MASK_0010_0000, DATA_DEC, REG_WP_EN },   
    [SFP28_P34_TXDIS]                      =  {PORT_28_35_TXDIS_REG                    , MASK_0100_0000, DATA_DEC, REG_WP_EN },   
    [SFP28_P35_TXDIS]                      =  {PORT_28_35_TXDIS_REG                    , MASK_1000_0000, DATA_DEC, REG_WP_EN },   
    [SFP28_P36_TXDIS]                      =  {PORT_36_43_TXDIS_REG                    , MASK_0000_0001, DATA_DEC, REG_WP_EN },   
    [SFP28_P37_TXDIS]                      =  {PORT_36_43_TXDIS_REG                    , MASK_0000_0010, DATA_DEC, REG_WP_EN },   
    [SFP28_P38_TXDIS]                      =  {PORT_36_43_TXDIS_REG                    , MASK_0000_0100, DATA_DEC, REG_WP_EN },   
    [SFP28_P39_TXDIS]                      =  {PORT_36_43_TXDIS_REG                    , MASK_0000_1000, DATA_DEC, REG_WP_EN },   
    [SFP56_P40_TXDIS]                      =  {PORT_36_43_TXDIS_REG                    , MASK_0001_0000, DATA_DEC, REG_WP_EN },   
    [SFP56_P41_TXDIS]                      =  {PORT_36_43_TXDIS_REG                    , MASK_0010_0000, DATA_DEC, REG_WP_EN },   
    [SFP56_P42_TXDIS]                      =  {PORT_36_43_TXDIS_REG                    , MASK_0100_0000, DATA_DEC, REG_WP_EN },   
    [SFP56_P43_TXDIS]                      =  {PORT_36_43_TXDIS_REG                    , MASK_1000_0000, DATA_DEC, REG_WP_EN },   
    [SFP56_P44_TXDIS]                      =  {PORT_44_47_TXDIS_REG                    , MASK_0000_0001, DATA_DEC, REG_WP_EN },   
    [SFP56_P45_TXDIS]                      =  {PORT_44_47_TXDIS_REG                    , MASK_0000_0010, DATA_DEC, REG_WP_EN },   
    [SFP56_P46_TXDIS]                      =  {PORT_44_47_TXDIS_REG                    , MASK_0000_0100, DATA_DEC, REG_WP_EN },   
    [SFP56_P47_TXDIS]                      =  {PORT_44_47_TXDIS_REG                    , MASK_0000_1000, DATA_DEC, REG_WP_EN },    
    [SFP28_P0_RS]                          =  {PORT_0_7_RS_REG                         , MASK_0000_0001, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P1_RS]                          =  {PORT_0_7_RS_REG                         , MASK_0000_0010, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P2_RS]                          =  {PORT_0_7_RS_REG                         , MASK_0000_0100, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P3_RS]                          =  {PORT_0_7_RS_REG                         , MASK_0000_1000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P4_RS]                          =  {PORT_0_7_RS_REG                         , MASK_0001_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P5_RS]                          =  {PORT_0_7_RS_REG                         , MASK_0010_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P6_RS]                          =  {PORT_0_7_RS_REG                         , MASK_0100_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P7_RS]                          =  {PORT_0_7_RS_REG                         , MASK_1000_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P8_RS]                          =  {PORT_8_15_RS_REG                        , MASK_0000_0001, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P9_RS]                          =  {PORT_8_15_RS_REG                        , MASK_0000_0010, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P10_RS]                         =  {PORT_8_15_RS_REG                        , MASK_0000_0100, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P11_RS]                         =  {PORT_8_15_RS_REG                        , MASK_0000_1000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P12_RS]                         =  {PORT_8_15_RS_REG                        , MASK_0001_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P13_RS]                         =  {PORT_8_15_RS_REG                        , MASK_0010_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P14_RS]                         =  {PORT_8_15_RS_REG                        , MASK_0100_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P15_RS]                         =  {PORT_8_15_RS_REG                        , MASK_1000_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P16_RS]                         =  {PORT_16_23_RS_REG                       , MASK_0000_0001, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P17_RS]                         =  {PORT_16_23_RS_REG                       , MASK_0000_0010, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P18_RS]                         =  {PORT_16_23_RS_REG                       , MASK_0000_0100, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P19_RS]                         =  {PORT_16_23_RS_REG                       , MASK_0000_1000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P20_RS]                         =  {PORT_16_23_RS_REG                       , MASK_0001_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P21_RS]                         =  {PORT_16_23_RS_REG                       , MASK_0010_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P22_RS]                         =  {PORT_16_23_RS_REG                       , MASK_0100_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P23_RS]                         =  {PORT_16_23_RS_REG                       , MASK_1000_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P24_RS]                         =  {PORT_24_27_RS_REG                       , MASK_0000_0001, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P25_RS]                         =  {PORT_24_27_RS_REG                       , MASK_0000_0010, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P26_RS]                         =  {PORT_24_27_RS_REG                       , MASK_0000_0100, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P27_RS]                         =  {PORT_24_27_RS_REG                       , MASK_0000_1000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P28_RS]                         =  {PORT_28_35_RS_REG                       , MASK_0000_0001, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P29_RS]                         =  {PORT_28_35_RS_REG                       , MASK_0000_0010, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P30_RS]                         =  {PORT_28_35_RS_REG                       , MASK_0000_0100, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P31_RS]                         =  {PORT_28_35_RS_REG                       , MASK_0000_1000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P32_RS]                         =  {PORT_28_35_RS_REG                       , MASK_0001_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P33_RS]                         =  {PORT_28_35_RS_REG                       , MASK_0010_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P34_RS]                         =  {PORT_28_35_RS_REG                       , MASK_0100_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P35_RS]                         =  {PORT_28_35_RS_REG                       , MASK_1000_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P36_RS]                         =  {PORT_36_43_RS_REG                       , MASK_0000_0001, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P37_RS]                         =  {PORT_36_43_RS_REG                       , MASK_0000_0010, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P38_RS]                         =  {PORT_36_43_RS_REG                       , MASK_0000_0100, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P39_RS]                         =  {PORT_36_43_RS_REG                       , MASK_0000_1000, DATA_DEC, REG_WP_DIS}, 
    [SFP56_P40_RS]                         =  {PORT_36_43_RS_REG                       , MASK_0001_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP56_P41_RS]                         =  {PORT_36_43_RS_REG                       , MASK_0010_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP56_P42_RS]                         =  {PORT_36_43_RS_REG                       , MASK_0100_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP56_P43_RS]                         =  {PORT_36_43_RS_REG                       , MASK_1000_0000, DATA_DEC, REG_WP_DIS}, 
    [SFP56_P44_RS]                         =  {PORT_44_47_RS_REG                       , MASK_0000_0001, DATA_DEC, REG_WP_DIS}, 
    [SFP56_P45_RS]                         =  {PORT_44_47_RS_REG                       , MASK_0000_0010, DATA_DEC, REG_WP_DIS}, 
    [SFP56_P46_RS]                         =  {PORT_44_47_RS_REG                       , MASK_0000_0100, DATA_DEC, REG_WP_DIS}, 
    [SFP56_P47_RS]                         =  {PORT_44_47_RS_REG                       , MASK_0000_1000, DATA_DEC, REG_WP_DIS}, 
    [SFP28_P0_RXLOS]                       =  {PORT_0_7_RXLOS_REG                      , MASK_0000_0001, DATA_DEC, REG_WP_DIS},  
    [SFP28_P1_RXLOS]                       =  {PORT_0_7_RXLOS_REG                      , MASK_0000_0010, DATA_DEC, REG_WP_DIS},  
    [SFP28_P2_RXLOS]                       =  {PORT_0_7_RXLOS_REG                      , MASK_0000_0100, DATA_DEC, REG_WP_DIS},  
    [SFP28_P3_RXLOS]                       =  {PORT_0_7_RXLOS_REG                      , MASK_0000_1000, DATA_DEC, REG_WP_DIS},  
    [SFP28_P4_RXLOS]                       =  {PORT_0_7_RXLOS_REG                      , MASK_0001_0000, DATA_DEC, REG_WP_DIS},  
    [SFP28_P5_RXLOS]                       =  {PORT_0_7_RXLOS_REG                      , MASK_0010_0000, DATA_DEC, REG_WP_DIS},  
    [SFP28_P6_RXLOS]                       =  {PORT_0_7_RXLOS_REG                      , MASK_0100_0000, DATA_DEC, REG_WP_DIS},  
    [SFP28_P7_RXLOS]                       =  {PORT_0_7_RXLOS_REG                      , MASK_1000_0000, DATA_DEC, REG_WP_DIS},  
    [SFP28_P8_RXLOS]                       =  {PORT_8_15_RXLOS_REG                     , MASK_0000_0001, DATA_DEC, REG_WP_DIS},  
    [SFP28_P9_RXLOS]                       =  {PORT_8_15_RXLOS_REG                     , MASK_0000_0010, DATA_DEC, REG_WP_DIS},  
    [SFP28_P10_RXLOS]                      =  {PORT_8_15_RXLOS_REG                     , MASK_0000_0100, DATA_DEC, REG_WP_DIS},   
    [SFP28_P11_RXLOS]                      =  {PORT_8_15_RXLOS_REG                     , MASK_0000_1000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P12_RXLOS]                      =  {PORT_8_15_RXLOS_REG                     , MASK_0001_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P13_RXLOS]                      =  {PORT_8_15_RXLOS_REG                     , MASK_0010_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P14_RXLOS]                      =  {PORT_8_15_RXLOS_REG                     , MASK_0100_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P15_RXLOS]                      =  {PORT_8_15_RXLOS_REG                     , MASK_1000_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P16_RXLOS]                      =  {PORT_16_23_RXLOS_REG                    , MASK_0000_0001, DATA_DEC, REG_WP_DIS},   
    [SFP28_P17_RXLOS]                      =  {PORT_16_23_RXLOS_REG                    , MASK_0000_0010, DATA_DEC, REG_WP_DIS},   
    [SFP28_P18_RXLOS]                      =  {PORT_16_23_RXLOS_REG                    , MASK_0000_0100, DATA_DEC, REG_WP_DIS},   
    [SFP28_P19_RXLOS]                      =  {PORT_16_23_RXLOS_REG                    , MASK_0000_1000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P20_RXLOS]                      =  {PORT_16_23_RXLOS_REG                    , MASK_0001_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P21_RXLOS]                      =  {PORT_16_23_RXLOS_REG                    , MASK_0010_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P22_RXLOS]                      =  {PORT_16_23_RXLOS_REG                    , MASK_0100_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P23_RXLOS]                      =  {PORT_16_23_RXLOS_REG                    , MASK_1000_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P24_RXLOS]                      =  {PORT_24_27_RXLOS_REG                    , MASK_0000_0001, DATA_DEC, REG_WP_DIS},   
    [SFP28_P25_RXLOS]                      =  {PORT_24_27_RXLOS_REG                    , MASK_0000_0010, DATA_DEC, REG_WP_DIS},   
    [SFP28_P26_RXLOS]                      =  {PORT_24_27_RXLOS_REG                    , MASK_0000_0100, DATA_DEC, REG_WP_DIS},   
    [SFP28_P27_RXLOS]                      =  {PORT_24_27_RXLOS_REG                    , MASK_0000_1000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P28_RXLOS]                      =  {PORT_28_35_RXLOS_REG                    , MASK_0000_0001, DATA_DEC, REG_WP_DIS},   
    [SFP28_P29_RXLOS]                      =  {PORT_28_35_RXLOS_REG                    , MASK_0000_0010, DATA_DEC, REG_WP_DIS},   
    [SFP28_P30_RXLOS]                      =  {PORT_28_35_RXLOS_REG                    , MASK_0000_0100, DATA_DEC, REG_WP_DIS},   
    [SFP28_P31_RXLOS]                      =  {PORT_28_35_RXLOS_REG                    , MASK_0000_1000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P32_RXLOS]                      =  {PORT_28_35_RXLOS_REG                    , MASK_0001_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P33_RXLOS]                      =  {PORT_28_35_RXLOS_REG                    , MASK_0010_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P34_RXLOS]                      =  {PORT_28_35_RXLOS_REG                    , MASK_0100_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P35_RXLOS]                      =  {PORT_28_35_RXLOS_REG                    , MASK_1000_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P36_RXLOS]                      =  {PORT_36_43_RXLOS_REG                    , MASK_0000_0001, DATA_DEC, REG_WP_DIS},   
    [SFP28_P37_RXLOS]                      =  {PORT_36_43_RXLOS_REG                    , MASK_0000_0010, DATA_DEC, REG_WP_DIS},   
    [SFP28_P38_RXLOS]                      =  {PORT_36_43_RXLOS_REG                    , MASK_0000_0100, DATA_DEC, REG_WP_DIS},   
    [SFP28_P39_RXLOS]                      =  {PORT_36_43_RXLOS_REG                    , MASK_0000_1000, DATA_DEC, REG_WP_DIS},   
    [SFP56_P40_RXLOS]                      =  {PORT_36_43_RXLOS_REG                    , MASK_0001_0000, DATA_DEC, REG_WP_DIS},   
    [SFP56_P41_RXLOS]                      =  {PORT_36_43_RXLOS_REG                    , MASK_0010_0000, DATA_DEC, REG_WP_DIS},   
    [SFP56_P42_RXLOS]                      =  {PORT_36_43_RXLOS_REG                    , MASK_0100_0000, DATA_DEC, REG_WP_DIS},   
    [SFP56_P43_RXLOS]                      =  {PORT_36_43_RXLOS_REG                    , MASK_1000_0000, DATA_DEC, REG_WP_DIS},   
    [SFP56_P44_RXLOS]                      =  {PORT_44_47_RXLOS_REG                    , MASK_0000_0001, DATA_DEC, REG_WP_DIS},   
    [SFP56_P45_RXLOS]                      =  {PORT_44_47_RXLOS_REG                    , MASK_0000_0010, DATA_DEC, REG_WP_DIS},   
    [SFP56_P46_RXLOS]                      =  {PORT_44_47_RXLOS_REG                    , MASK_0000_0100, DATA_DEC, REG_WP_DIS},   
    [SFP56_P47_RXLOS]                      =  {PORT_44_47_RXLOS_REG                    , MASK_0000_1000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P0_TXFLT]                       =  {PORT_0_7_TXFLT_REG                      , MASK_0000_0001, DATA_DEC, REG_WP_DIS},  
    [SFP28_P1_TXFLT]                       =  {PORT_0_7_TXFLT_REG                      , MASK_0000_0010, DATA_DEC, REG_WP_DIS},  
    [SFP28_P2_TXFLT]                       =  {PORT_0_7_TXFLT_REG                      , MASK_0000_0100, DATA_DEC, REG_WP_DIS},  
    [SFP28_P3_TXFLT]                       =  {PORT_0_7_TXFLT_REG                      , MASK_0000_1000, DATA_DEC, REG_WP_DIS},  
    [SFP28_P4_TXFLT]                       =  {PORT_0_7_TXFLT_REG                      , MASK_0001_0000, DATA_DEC, REG_WP_DIS},  
    [SFP28_P5_TXFLT]                       =  {PORT_0_7_TXFLT_REG                      , MASK_0010_0000, DATA_DEC, REG_WP_DIS},  
    [SFP28_P6_TXFLT]                       =  {PORT_0_7_TXFLT_REG                      , MASK_0100_0000, DATA_DEC, REG_WP_DIS},  
    [SFP28_P7_TXFLT]                       =  {PORT_0_7_TXFLT_REG                      , MASK_1000_0000, DATA_DEC, REG_WP_DIS},  
    [SFP28_P8_TXFLT]                       =  {PORT_8_15_TXFLT_REG                     , MASK_0000_0001, DATA_DEC, REG_WP_DIS},  
    [SFP28_P9_TXFLT]                       =  {PORT_8_15_TXFLT_REG                     , MASK_0000_0010, DATA_DEC, REG_WP_DIS},  
    [SFP28_P10_TXFLT]                      =  {PORT_8_15_TXFLT_REG                     , MASK_0000_0100, DATA_DEC, REG_WP_DIS},   
    [SFP28_P11_TXFLT]                      =  {PORT_8_15_TXFLT_REG                     , MASK_0000_1000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P12_TXFLT]                      =  {PORT_8_15_TXFLT_REG                     , MASK_0001_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P13_TXFLT]                      =  {PORT_8_15_TXFLT_REG                     , MASK_0010_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P14_TXFLT]                      =  {PORT_8_15_TXFLT_REG                     , MASK_0100_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P15_TXFLT]                      =  {PORT_8_15_TXFLT_REG                     , MASK_1000_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P16_TXFLT]                      =  {PORT_16_23_TXFLT_REG                    , MASK_0000_0001, DATA_DEC, REG_WP_DIS},   
    [SFP28_P17_TXFLT]                      =  {PORT_16_23_TXFLT_REG                    , MASK_0000_0010, DATA_DEC, REG_WP_DIS},   
    [SFP28_P18_TXFLT]                      =  {PORT_16_23_TXFLT_REG                    , MASK_0000_0100, DATA_DEC, REG_WP_DIS},   
    [SFP28_P19_TXFLT]                      =  {PORT_16_23_TXFLT_REG                    , MASK_0000_1000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P20_TXFLT]                      =  {PORT_16_23_TXFLT_REG                    , MASK_0001_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P21_TXFLT]                      =  {PORT_16_23_TXFLT_REG                    , MASK_0010_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P22_TXFLT]                      =  {PORT_16_23_TXFLT_REG                    , MASK_0100_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P23_TXFLT]                      =  {PORT_16_23_TXFLT_REG                    , MASK_1000_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P24_TXFLT]                      =  {PORT_24_27_TXFLT_REG                    , MASK_0001_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P25_TXFLT]                      =  {PORT_24_27_TXFLT_REG                    , MASK_0010_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P26_TXFLT]                      =  {PORT_24_27_TXFLT_REG                    , MASK_0100_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P27_TXFLT]                      =  {PORT_24_27_TXFLT_REG                    , MASK_1000_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P28_TXFLT]                      =  {PORT_28_35_TXFLT_REG                    , MASK_0000_0001, DATA_DEC, REG_WP_DIS},   
    [SFP28_P29_TXFLT]                      =  {PORT_28_35_TXFLT_REG                    , MASK_0000_0010, DATA_DEC, REG_WP_DIS},   
    [SFP28_P30_TXFLT]                      =  {PORT_28_35_TXFLT_REG                    , MASK_0000_0100, DATA_DEC, REG_WP_DIS},   
    [SFP28_P31_TXFLT]                      =  {PORT_28_35_TXFLT_REG                    , MASK_0000_1000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P32_TXFLT]                      =  {PORT_28_35_TXFLT_REG                    , MASK_0001_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P33_TXFLT]                      =  {PORT_28_35_TXFLT_REG                    , MASK_0010_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P34_TXFLT]                      =  {PORT_28_35_TXFLT_REG                    , MASK_0100_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P35_TXFLT]                      =  {PORT_28_35_TXFLT_REG                    , MASK_1000_0000, DATA_DEC, REG_WP_DIS},   
    [SFP28_P36_TXFLT]                      =  {PORT_36_43_TXFLT_REG                    , MASK_0000_0001, DATA_DEC, REG_WP_DIS},   
    [SFP28_P37_TXFLT]                      =  {PORT_36_43_TXFLT_REG                    , MASK_0000_0010, DATA_DEC, REG_WP_DIS},   
    [SFP28_P38_TXFLT]                      =  {PORT_36_43_TXFLT_REG                    , MASK_0000_0100, DATA_DEC, REG_WP_DIS},   
    [SFP28_P39_TXFLT]                      =  {PORT_36_43_TXFLT_REG                    , MASK_0000_1000, DATA_DEC, REG_WP_DIS},   
    [SFP56_P40_TXFLT]                      =  {PORT_36_43_TXFLT_REG                    , MASK_0001_0000, DATA_DEC, REG_WP_DIS},   
    [SFP56_P41_TXFLT]                      =  {PORT_36_43_TXFLT_REG                    , MASK_0010_0000, DATA_DEC, REG_WP_DIS},   
    [SFP56_P42_TXFLT]                      =  {PORT_36_43_TXFLT_REG                    , MASK_0100_0000, DATA_DEC, REG_WP_DIS},   
    [SFP56_P43_TXFLT]                      =  {PORT_36_43_TXFLT_REG                    , MASK_1000_0000, DATA_DEC, REG_WP_DIS},   
    [SFP56_P44_TXFLT]                      =  {PORT_44_47_TXFLT_REG                    , MASK_0000_0001, DATA_DEC, REG_WP_DIS},   
    [SFP56_P45_TXFLT]                      =  {PORT_44_47_TXFLT_REG                    , MASK_0000_0010, DATA_DEC, REG_WP_DIS},   
    [SFP56_P46_TXFLT]                      =  {PORT_44_47_TXFLT_REG                    , MASK_0000_0100, DATA_DEC, REG_WP_DIS},   
    [SFP56_P47_TXFLT]                      =  {PORT_44_47_TXFLT_REG                    , MASK_0000_1000, DATA_DEC, REG_WP_DIS},   
    [QSFPDD_P52_EFUSE_PG]                  =  {PORT_52_53_EFUSE_PG_EVENT_REG           , MASK_0000_0001, DATA_DEC, REG_WP_DIS},     
    [QSFPDD_P53_EFUSE_PG]                  =  {PORT_52_53_EFUSE_PG_EVENT_REG           , MASK_0000_0010, DATA_DEC, REG_WP_DIS},     
    [QSFP28_P48_INTR]                      =  {PORT_48_53_INTR_REG                     , MASK_0000_0001, DATA_DEC, REG_WP_DIS},       
    [QSFP28_P49_INTR]                      =  {PORT_48_53_INTR_REG                     , MASK_0000_0010, DATA_DEC, REG_WP_DIS},       
    [QSFP28_P50_INTR]                      =  {PORT_48_53_INTR_REG                     , MASK_0000_0100, DATA_DEC, REG_WP_DIS},       
    [QSFP28_P51_INTR]                      =  {PORT_48_53_INTR_REG                     , MASK_0000_1000, DATA_DEC, REG_WP_DIS},       
    [QSFPDD_P52_INTR]                      =  {PORT_48_53_INTR_REG                     , MASK_0001_0000, DATA_DEC, REG_WP_DIS},       
    [QSFPDD_P53_INTR]                      =  {PORT_48_53_INTR_REG                     , MASK_0010_0000, DATA_DEC, REG_WP_DIS},       
    [QSFP28_P48_RST]                       =  {PORT_48_53_RST_REG                      , MASK_0000_0001, DATA_DEC, REG_WP_EN }, 
    [QSFP28_P49_RST]                       =  {PORT_48_53_RST_REG                      , MASK_0000_0010, DATA_DEC, REG_WP_EN }, 
    [QSFP28_P50_RST]                       =  {PORT_48_53_RST_REG                      , MASK_0000_0100, DATA_DEC, REG_WP_EN }, 
    [QSFP28_P51_RST]                       =  {PORT_48_53_RST_REG                      , MASK_0000_1000, DATA_DEC, REG_WP_EN }, 
    [QSFPDD_P52_RST]                       =  {PORT_48_53_RST_REG                      , MASK_0001_0000, DATA_DEC, REG_WP_EN }, 
    [QSFPDD_P53_RST]                       =  {PORT_48_53_RST_REG                      , MASK_0010_0000, DATA_DEC, REG_WP_EN }, 
    [QSFP28_P48_LP_MODE]                   =  {PORT_48_53_LP_REG                       , MASK_0000_0001, DATA_DEC, REG_WP_EN },    
    [QSFP28_P49_LP_MODE]                   =  {PORT_48_53_LP_REG                       , MASK_0000_0010, DATA_DEC, REG_WP_EN },    
    [QSFP28_P50_LP_MODE]                   =  {PORT_48_53_LP_REG                       , MASK_0000_0100, DATA_DEC, REG_WP_EN },    
    [QSFP28_P51_LP_MODE]                   =  {PORT_48_53_LP_REG                       , MASK_0000_1000, DATA_DEC, REG_WP_EN },    
    [QSFPDD_P52_LP_MODE]                   =  {PORT_48_53_LP_REG                       , MASK_0001_0000, DATA_DEC, REG_WP_EN },    
    [QSFPDD_P53_LP_MODE]                   =  {PORT_48_53_LP_REG                       , MASK_0010_0000, DATA_DEC, REG_WP_EN },    
    //FPGA
    [FPGA_VER_1]                           =  {FPGA_VER_1_REG                          , MASK_ALL      , DATA_DEC, REG_WP_DIS},
    [FPGA_MINOR_VER]                       =  {FPGA_VER_1_REG                          , MASK_0011_1111, DATA_DEC, REG_WP_DIS},
    [FPGA_MAJOR_VER]                       =  {FPGA_VER_1_REG                          , MASK_1100_0000, DATA_DEC, REG_WP_DIS},
    [FPGA_BUILD_VER]                       =  {FPGA_VER_2_REG                          , MASK_ALL      , DATA_DEC, REG_WP_DIS},
    [FPGA_VERSION_H]                       =  {NONE_REG                                , MASK_NONE     , DATA_UNK, REG_WP_DIS},
    [FPGA_DEV_INFO]                        =  {FPGA_DEV_INFO_REG                       , MASK_0000_1111, DATA_DEC, REG_WP_DIS},

    // MUX
    [IDLE_STATE]                           =  {NONE_REG                                , MASK_NONE     , DATA_UNK, REG_WP_DIS},
    //BSP DEBUG
    [BSP_DEBUG]                            =  {NONE_REG                                , MASK_NONE     , DATA_UNK, REG_WP_DIS},
};

enum bsp_log_types {
    LOG_NONE,
    LOG_RW,
    LOG_READ,
    LOG_WRITE
};

enum bsp_log_ctrl {
    LOG_DISABLE,
    LOG_ENABLE
};

/* CPLD sysfs attributes hook functions  */
static ssize_t cpld_show(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t cpld_store(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
u8 _cpld_reg_read(struct device *dev, u8 reg, u8 mask);
static ssize_t cpld_reg_read(struct device *dev, char *buf, u8 reg, u8 mask, u8 data_type);
static ssize_t cpld_reg_write(struct device *dev, const char *buf, size_t count, u8 reg, u8 mask, bool write_protect);
static ssize_t _cpld_reg_write_with_protect(struct device *dev, u8 reg, u8 reg_val);
static ssize_t bsp_read(char *buf, char *str);
static ssize_t bsp_write(const char *buf, char *str, size_t str_len, size_t count);
static ssize_t bsp_callback_show(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t bsp_callback_store(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t version_h_show(struct device *dev, struct device_attribute *da, char *buf);



static LIST_HEAD(cpld_client_list);  /* client list for cpld */
static struct mutex list_lock;  /* mutex for client list */

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

//struct cpld_data {
//    int index;                  /* CPLD index */
//    struct mutex access_lock;   /* mutex for cpld access */
//    u8 access_reg;              /* register to access */
//};

/* CPLD device id and data */
static const struct i2c_device_id cpld_id[] = {
    { "s9620_54dc_cpld1",  cpld1 },
    { "s9620_54dc_cpld2",  cpld2 },
    { "s9620_54dc_cpld3",  cpld3 },
    { "s9620_54dc_fpga" ,  fpga  },
    {}
};

char bsp_debug[2]="0";
u8 enable_log_read=LOG_DISABLE;
u8 enable_log_write=LOG_DISABLE;

/* Addresses scanned for cpld */
static const unsigned short cpld_i2c_addr[] = { 0x30, 0x31, 0x37, 0x32, 0x33, I2C_CLIENT_END };

/* define all support register access of cpld in attribute */

// CPLD Common

static SENSOR_DEVICE_ATTR_RO(cpld_minor_ver                , cpld, CPLD_MINOR_VER);
static SENSOR_DEVICE_ATTR_RO(cpld_major_ver                , cpld, CPLD_MAJOR_VER);
static SENSOR_DEVICE_ATTR_RO(cpld_id                       , cpld, CPLD_ID);
static SENSOR_DEVICE_ATTR_RO(cpld_build_ver                , cpld, CPLD_BUILD_VER);
static SENSOR_DEVICE_ATTR_RO(cpld_version_h                , version_h, CPLD_VERSION_H);
static SENSOR_DEVICE_ATTR_RW(event_detect_ctrl             , cpld, EVENT_DETECT_CTRL);
static SENSOR_DEVICE_ATTR_RW(cpld_write_protect            , cpld, CPLD_WRITE_PROTECT);
// CPLD 1
static SENSOR_DEVICE_ATTR_RO(cpld_sku_id                   , cpld, CPLD_SKU_ID);
static SENSOR_DEVICE_ATTR_RO(cpld_hw_rev                   , cpld, CPLD_HW_REV);
static SENSOR_DEVICE_ATTR_RO(cpld_deph_rev                 , cpld, CPLD_DEPH_REV);
static SENSOR_DEVICE_ATTR_RO(cpld_build_rev                , cpld, CPLD_BUILD_REV);
static SENSOR_DEVICE_ATTR_RO(cpld_brd_id_type              , cpld, CPLD_BRD_ID_TYPE);
static SENSOR_DEVICE_ATTR_RO(cpld_chip_type                , cpld, CPLD_CHIP_TYPE);
static SENSOR_DEVICE_ATTR_RW(ntm_rst                       , cpld, NTM_RST);
static SENSOR_DEVICE_ATTR_RW(bmc_rst                       , cpld, BMC_RST);
static SENSOR_DEVICE_ATTR_RW(cpld_2_3_rst                  , cpld, CPLD_2_3_RST);
static SENSOR_DEVICE_ATTR_RW(fpga_rst                      , cpld, FPGA_RST);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_1_rst                 , cpld, I2C_MUX_1_RST);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_2_rst                 , cpld, I2C_MUX_2_RST);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_3_rst                 , cpld, I2C_MUX_3_RST);
static SENSOR_DEVICE_ATTR_RW(ioexp_1_rst                   , cpld, IOEXP_1_RST);
static SENSOR_DEVICE_ATTR_RW(ioexp_2_rst                   , cpld, IOEXP_2_RST);
static SENSOR_DEVICE_ATTR_RW(fan_i2c_mux_rst               , cpld, FAN_I2C_MUX_RST);
static SENSOR_DEVICE_ATTR_RO(psu_0_prsnt                   , cpld, PSU_0_PRSNT);
static SENSOR_DEVICE_ATTR_RO(psu_1_prsnt                   , cpld, PSU_1_PRSNT);
static SENSOR_DEVICE_ATTR_RO(psu_0_acin                    , cpld, PSU_0_ACIN);
static SENSOR_DEVICE_ATTR_RO(psu_1_acin                    , cpld, PSU_1_ACIN);
static SENSOR_DEVICE_ATTR_RO(psu_0_pg                      , cpld, PSU_0_PG);
static SENSOR_DEVICE_ATTR_RO(psu_1_pg                      , cpld, PSU_1_PG);
static SENSOR_DEVICE_ATTR_RO(cpu_pg                        , cpld, CPU_PG);
static SENSOR_DEVICE_ATTR_RO(mb_cpu_pg                     , cpld, MB_CPU_PG);
static SENSOR_DEVICE_ATTR_RO(bmc_pg                        , cpld, BMC_PG);
static SENSOR_DEVICE_ATTR_RO(cpu_mux_sel                   , cpld, CPU_MUX_SEL);
static SENSOR_DEVICE_ATTR_RO(psu_mux_sel                   , cpld, PSU_MUX_SEL);
static SENSOR_DEVICE_ATTR_RW(system_led_status             , cpld, SYSTEM_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(fan_led_status                , cpld, FAN_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(psu_0_led_status              , cpld, PSU_0_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(psu_1_led_status              , cpld, PSU_1_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(sync_led_status               , cpld, SYNC_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(id_led_status                 , cpld, ID_LED_STATUS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p0_abs                   , cpld, SFP28_P0_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p1_abs                   , cpld, SFP28_P1_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p2_abs                   , cpld, SFP28_P2_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p3_abs                   , cpld, SFP28_P3_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p4_abs                   , cpld, SFP28_P4_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p5_abs                   , cpld, SFP28_P5_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p6_abs                   , cpld, SFP28_P6_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p7_abs                   , cpld, SFP28_P7_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p8_abs                   , cpld, SFP28_P8_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p9_abs                   , cpld, SFP28_P9_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p10_abs                  , cpld, SFP28_P10_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p11_abs                  , cpld, SFP28_P11_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p12_abs                  , cpld, SFP28_P12_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p13_abs                  , cpld, SFP28_P13_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p14_abs                  , cpld, SFP28_P14_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p15_abs                  , cpld, SFP28_P15_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p16_abs                  , cpld, SFP28_P16_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p17_abs                  , cpld, SFP28_P17_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p18_abs                  , cpld, SFP28_P18_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p19_abs                  , cpld, SFP28_P19_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p20_abs                  , cpld, SFP28_P20_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p21_abs                  , cpld, SFP28_P21_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p22_abs                  , cpld, SFP28_P22_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p23_abs                  , cpld, SFP28_P23_ABS); 
static SENSOR_DEVICE_ATTR_RO(sfp28_p24_abs                  , cpld, SFP28_P24_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p25_abs                  , cpld, SFP28_P25_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p26_abs                  , cpld, SFP28_P26_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p27_abs                  , cpld, SFP28_P27_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p28_abs                  , cpld, SFP28_P28_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p29_abs                  , cpld, SFP28_P29_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p30_abs                  , cpld, SFP28_P30_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p31_abs                  , cpld, SFP28_P31_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p32_abs                  , cpld, SFP28_P32_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p33_abs                  , cpld, SFP28_P33_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p34_abs                  , cpld, SFP28_P34_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p35_abs                  , cpld, SFP28_P35_ABS); 
static SENSOR_DEVICE_ATTR_RO(sfp28_p36_abs                  , cpld, SFP28_P36_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p37_abs                  , cpld, SFP28_P37_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p38_abs                  , cpld, SFP28_P38_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p39_abs                  , cpld, SFP28_P39_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p40_abs                  , cpld, SFP56_P40_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p41_abs                  , cpld, SFP56_P41_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p42_abs                  , cpld, SFP56_P42_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p43_abs                  , cpld, SFP56_P43_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p44_abs                  , cpld, SFP56_P44_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p45_abs                  , cpld, SFP56_P45_ABS); 
static SENSOR_DEVICE_ATTR_RO(sfp56_p46_abs                  , cpld, SFP56_P46_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p47_abs                  , cpld, SFP56_P47_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p48_abs                 , cpld, QSFP28_P48_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p49_abs                 , cpld, QSFP28_P49_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p50_abs                 , cpld, QSFP28_P50_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p51_abs                 , cpld, QSFP28_P51_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p52_abs                 , cpld, QSFPDD_P52_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p53_abs                 , cpld, QSFPDD_P53_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p0_i2c_stuck            , cpld, SFP28_P0_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p1_i2c_stuck            , cpld, SFP28_P1_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p2_i2c_stuck            , cpld, SFP28_P2_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p3_i2c_stuck            , cpld, SFP28_P3_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p4_i2c_stuck            , cpld, SFP28_P4_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p5_i2c_stuck            , cpld, SFP28_P5_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p6_i2c_stuck            , cpld, SFP28_P6_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p7_i2c_stuck            , cpld, SFP28_P7_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p8_i2c_stuck            , cpld, SFP28_P8_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p9_i2c_stuck            , cpld, SFP28_P9_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p10_i2c_stuck           , cpld, SFP28_P10_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p11_i2c_stuck           , cpld, SFP28_P11_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p12_i2c_stuck           , cpld, SFP28_P12_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p13_i2c_stuck           , cpld, SFP28_P13_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p14_i2c_stuck           , cpld, SFP28_P14_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p15_i2c_stuck           , cpld, SFP28_P15_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p16_i2c_stuck           , cpld, SFP28_P16_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p17_i2c_stuck           , cpld, SFP28_P17_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p18_i2c_stuck           , cpld, SFP28_P18_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p19_i2c_stuck           , cpld, SFP28_P19_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p20_i2c_stuck           , cpld, SFP28_P20_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p21_i2c_stuck           , cpld, SFP28_P21_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p22_i2c_stuck           , cpld, SFP28_P22_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p23_i2c_stuck           , cpld, SFP28_P23_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p24_i2c_stuck           , cpld, SFP28_P24_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p25_i2c_stuck           , cpld, SFP28_P25_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p26_i2c_stuck           , cpld, SFP28_P26_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p27_i2c_stuck           , cpld, SFP28_P27_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p28_i2c_stuck           , cpld, SFP28_P28_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p29_i2c_stuck           , cpld, SFP28_P29_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p30_i2c_stuck           , cpld, SFP28_P30_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p31_i2c_stuck           , cpld, SFP28_P31_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p32_i2c_stuck           , cpld, SFP28_P32_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p33_i2c_stuck           , cpld, SFP28_P33_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p34_i2c_stuck           , cpld, SFP28_P34_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p35_i2c_stuck           , cpld, SFP28_P35_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p36_i2c_stuck           , cpld, SFP28_P36_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p37_i2c_stuck           , cpld, SFP28_P37_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p38_i2c_stuck           , cpld, SFP28_P38_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p39_i2c_stuck           , cpld, SFP28_P39_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp56_p40_i2c_stuck           , cpld, SFP56_P40_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp56_p41_i2c_stuck           , cpld, SFP56_P41_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp56_p42_i2c_stuck           , cpld, SFP56_P42_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp56_p43_i2c_stuck           , cpld, SFP56_P43_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp56_p44_i2c_stuck           , cpld, SFP56_P44_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp56_p45_i2c_stuck           , cpld, SFP56_P45_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp56_p46_i2c_stuck           , cpld, SFP56_P46_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp56_p47_i2c_stuck           , cpld, SFP56_P47_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p48_i2c_stuck          , cpld, QSFP28_P48_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p49_i2c_stuck          , cpld, QSFP28_P49_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p50_i2c_stuck          , cpld, QSFP28_P50_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p51_i2c_stuck          , cpld, QSFP28_P51_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p52_i2c_stuck          , cpld, QSFPDD_P52_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p53_i2c_stuck          , cpld, QSFPDD_P53_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(port_0_7_abs_event            , cpld, PORT_0_7_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(port_8_15_abs_event           , cpld, PORT_8_15_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(port_16_23_abs_event          , cpld, PORT_16_23_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(port_24_27_abs_event          , cpld, PORT_24_27_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(port_28_35_abs_event          , cpld, PORT_28_35_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(port_36_43_abs_event          , cpld, PORT_36_43_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(port_44_51_abs_event          , cpld, PORT_44_51_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(port_52_53_abs_event          , cpld, PORT_52_53_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RW(sfp28_p0_tx_dis                 , cpld, SFP28_P0_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p1_tx_dis                 , cpld, SFP28_P1_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p2_tx_dis                 , cpld, SFP28_P2_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p3_tx_dis                 , cpld, SFP28_P3_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p4_tx_dis                 , cpld, SFP28_P4_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p5_tx_dis                 , cpld, SFP28_P5_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p6_tx_dis                 , cpld, SFP28_P6_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p7_tx_dis                 , cpld, SFP28_P7_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p8_tx_dis                 , cpld, SFP28_P8_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p9_tx_dis                 , cpld, SFP28_P9_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p10_tx_dis                , cpld, SFP28_P10_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p11_tx_dis                , cpld, SFP28_P11_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p12_tx_dis                , cpld, SFP28_P12_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p13_tx_dis                , cpld, SFP28_P13_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p14_tx_dis                , cpld, SFP28_P14_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p15_tx_dis                , cpld, SFP28_P15_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p16_tx_dis                , cpld, SFP28_P16_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p17_tx_dis                , cpld, SFP28_P17_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p18_tx_dis                , cpld, SFP28_P18_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p19_tx_dis                , cpld, SFP28_P19_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p20_tx_dis                , cpld, SFP28_P20_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p21_tx_dis                , cpld, SFP28_P21_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p22_tx_dis                , cpld, SFP28_P22_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p23_tx_dis                , cpld, SFP28_P23_TXDIS); 
static SENSOR_DEVICE_ATTR_RW(sfp28_p24_tx_dis                , cpld, SFP28_P24_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p25_tx_dis                , cpld, SFP28_P25_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p26_tx_dis                , cpld, SFP28_P26_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p27_tx_dis                , cpld, SFP28_P27_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p28_tx_dis                , cpld, SFP28_P28_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p29_tx_dis                , cpld, SFP28_P29_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p30_tx_dis                , cpld, SFP28_P30_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p31_tx_dis                , cpld, SFP28_P31_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p32_tx_dis                , cpld, SFP28_P32_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p33_tx_dis                , cpld, SFP28_P33_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p34_tx_dis                , cpld, SFP28_P34_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p35_tx_dis                , cpld, SFP28_P35_TXDIS); 
static SENSOR_DEVICE_ATTR_RW(sfp28_p36_tx_dis                , cpld, SFP28_P36_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p37_tx_dis                , cpld, SFP28_P37_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p38_tx_dis                , cpld, SFP28_P38_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p39_tx_dis                , cpld, SFP28_P39_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p40_tx_dis                , cpld, SFP56_P40_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p41_tx_dis                , cpld, SFP56_P41_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p42_tx_dis                , cpld, SFP56_P42_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p43_tx_dis                , cpld, SFP56_P43_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p44_tx_dis                , cpld, SFP56_P44_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p45_tx_dis                , cpld, SFP56_P45_TXDIS); 
static SENSOR_DEVICE_ATTR_RW(sfp56_p46_tx_dis                , cpld, SFP56_P46_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p47_tx_dis                , cpld, SFP56_P47_TXDIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p0_rs                    , cpld, SFP28_P0_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p1_rs                    , cpld, SFP28_P1_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p2_rs                    , cpld, SFP28_P2_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p3_rs                    , cpld, SFP28_P3_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p4_rs                    , cpld, SFP28_P4_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p5_rs                    , cpld, SFP28_P5_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p6_rs                    , cpld, SFP28_P6_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p7_rs                    , cpld, SFP28_P7_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p8_rs                    , cpld, SFP28_P8_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p9_rs                    , cpld, SFP28_P9_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p10_rs                   , cpld, SFP28_P10_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p11_rs                   , cpld, SFP28_P11_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p12_rs                   , cpld, SFP28_P12_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p13_rs                   , cpld, SFP28_P13_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p14_rs                   , cpld, SFP28_P14_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p15_rs                   , cpld, SFP28_P15_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p16_rs                   , cpld, SFP28_P16_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p17_rs                   , cpld, SFP28_P17_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p18_rs                   , cpld, SFP28_P18_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p19_rs                   , cpld, SFP28_P19_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p20_rs                   , cpld, SFP28_P20_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p21_rs                   , cpld, SFP28_P21_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p22_rs                   , cpld, SFP28_P22_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p23_rs                   , cpld, SFP28_P23_RS); 
static SENSOR_DEVICE_ATTR_RW(sfp28_p24_rs                   , cpld, SFP28_P24_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p25_rs                   , cpld, SFP28_P25_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p26_rs                   , cpld, SFP28_P26_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p27_rs                   , cpld, SFP28_P27_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p28_rs                   , cpld, SFP28_P28_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p29_rs                   , cpld, SFP28_P29_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p30_rs                   , cpld, SFP28_P30_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p31_rs                   , cpld, SFP28_P31_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p32_rs                   , cpld, SFP28_P32_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p33_rs                   , cpld, SFP28_P33_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p34_rs                   , cpld, SFP28_P34_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p35_rs                   , cpld, SFP28_P35_RS); 
static SENSOR_DEVICE_ATTR_RW(sfp28_p36_rs                   , cpld, SFP28_P36_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p37_rs                   , cpld, SFP28_P37_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p38_rs                   , cpld, SFP28_P38_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p39_rs                   , cpld, SFP28_P39_RS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p40_rs                   , cpld, SFP56_P40_RS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p41_rs                   , cpld, SFP56_P41_RS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p42_rs                   , cpld, SFP56_P42_RS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p43_rs                   , cpld, SFP56_P43_RS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p44_rs                   , cpld, SFP56_P44_RS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p45_rs                   , cpld, SFP56_P45_RS); 
static SENSOR_DEVICE_ATTR_RW(sfp56_p46_rs                   , cpld, SFP56_P46_RS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p47_rs                   , cpld, SFP56_P47_RS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p0_rx_los                 , cpld, SFP28_P0_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p1_rx_los                 , cpld, SFP28_P1_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p2_rx_los                 , cpld, SFP28_P2_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p3_rx_los                 , cpld, SFP28_P3_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p4_rx_los                 , cpld, SFP28_P4_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p5_rx_los                 , cpld, SFP28_P5_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p6_rx_los                 , cpld, SFP28_P6_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p7_rx_los                 , cpld, SFP28_P7_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p8_rx_los                 , cpld, SFP28_P8_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p9_rx_los                 , cpld, SFP28_P9_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p10_rx_los                , cpld, SFP28_P10_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p11_rx_los                , cpld, SFP28_P11_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p12_rx_los                , cpld, SFP28_P12_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p13_rx_los                , cpld, SFP28_P13_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p14_rx_los                , cpld, SFP28_P14_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p15_rx_los                , cpld, SFP28_P15_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p16_rx_los                , cpld, SFP28_P16_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p17_rx_los                , cpld, SFP28_P17_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p18_rx_los                , cpld, SFP28_P18_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p19_rx_los                , cpld, SFP28_P19_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p20_rx_los                , cpld, SFP28_P20_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p21_rx_los                , cpld, SFP28_P21_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p22_rx_los                , cpld, SFP28_P22_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p23_rx_los                , cpld, SFP28_P23_RXLOS); 
static SENSOR_DEVICE_ATTR_RO(sfp28_p24_rx_los                , cpld, SFP28_P24_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p25_rx_los                , cpld, SFP28_P25_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p26_rx_los                , cpld, SFP28_P26_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p27_rx_los                , cpld, SFP28_P27_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p28_rx_los                , cpld, SFP28_P28_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p29_rx_los                , cpld, SFP28_P29_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p30_rx_los                , cpld, SFP28_P30_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p31_rx_los                , cpld, SFP28_P31_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p32_rx_los                , cpld, SFP28_P32_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p33_rx_los                , cpld, SFP28_P33_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p34_rx_los                , cpld, SFP28_P34_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p35_rx_los                , cpld, SFP28_P35_RXLOS); 
static SENSOR_DEVICE_ATTR_RO(sfp28_p36_rx_los                , cpld, SFP28_P36_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p37_rx_los                , cpld, SFP28_P37_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p38_rx_los                , cpld, SFP28_P38_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p39_rx_los                , cpld, SFP28_P39_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p40_rx_los                , cpld, SFP56_P40_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p41_rx_los                , cpld, SFP56_P41_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p42_rx_los                , cpld, SFP56_P42_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p43_rx_los                , cpld, SFP56_P43_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p44_rx_los                , cpld, SFP56_P44_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p45_rx_los                , cpld, SFP56_P45_RXLOS); 
static SENSOR_DEVICE_ATTR_RO(sfp56_p46_rx_los                , cpld, SFP56_P46_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p47_rx_los                , cpld, SFP56_P47_RXLOS);
static SENSOR_DEVICE_ATTR_RO(sfp28_p0_tx_flt                 , cpld, SFP28_P0_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p1_tx_flt                 , cpld, SFP28_P1_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p2_tx_flt                 , cpld, SFP28_P2_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p3_tx_flt                 , cpld, SFP28_P3_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p4_tx_flt                 , cpld, SFP28_P4_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p5_tx_flt                 , cpld, SFP28_P5_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p6_tx_flt                 , cpld, SFP28_P6_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p7_tx_flt                 , cpld, SFP28_P7_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p8_tx_flt                 , cpld, SFP28_P8_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p9_tx_flt                 , cpld, SFP28_P9_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p10_tx_flt                , cpld, SFP28_P10_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p11_tx_flt                , cpld, SFP28_P11_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p12_tx_flt                , cpld, SFP28_P12_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p13_tx_flt                , cpld, SFP28_P13_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p14_tx_flt                , cpld, SFP28_P14_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p15_tx_flt                , cpld, SFP28_P15_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p16_tx_flt                , cpld, SFP28_P16_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p17_tx_flt                , cpld, SFP28_P17_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p18_tx_flt                , cpld, SFP28_P18_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p19_tx_flt                , cpld, SFP28_P19_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p20_tx_flt                , cpld, SFP28_P20_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p21_tx_flt                , cpld, SFP28_P21_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p22_tx_flt                , cpld, SFP28_P22_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p23_tx_flt                , cpld, SFP28_P23_TXFLT); 
static SENSOR_DEVICE_ATTR_RO(sfp28_p24_tx_flt                , cpld, SFP28_P24_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p25_tx_flt                , cpld, SFP28_P25_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p26_tx_flt                , cpld, SFP28_P26_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p27_tx_flt                , cpld, SFP28_P27_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p28_tx_flt                , cpld, SFP28_P28_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p29_tx_flt                , cpld, SFP28_P29_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p30_tx_flt                , cpld, SFP28_P30_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p31_tx_flt                , cpld, SFP28_P31_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p32_tx_flt                , cpld, SFP28_P32_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p33_tx_flt                , cpld, SFP28_P33_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p34_tx_flt                , cpld, SFP28_P34_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p35_tx_flt                , cpld, SFP28_P35_TXFLT); 
static SENSOR_DEVICE_ATTR_RO(sfp28_p36_tx_flt                , cpld, SFP28_P36_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p37_tx_flt                , cpld, SFP28_P37_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p38_tx_flt                , cpld, SFP28_P38_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp28_p39_tx_flt                , cpld, SFP28_P39_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp56_p40_tx_flt                , cpld, SFP56_P40_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp56_p41_tx_flt                , cpld, SFP56_P41_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp56_p42_tx_flt                , cpld, SFP56_P42_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp56_p43_tx_flt                , cpld, SFP56_P43_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp56_p44_tx_flt                , cpld, SFP56_P44_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp56_p45_tx_flt                , cpld, SFP56_P45_TXFLT); 
static SENSOR_DEVICE_ATTR_RO(sfp56_p46_tx_flt                , cpld, SFP56_P46_TXFLT);
static SENSOR_DEVICE_ATTR_RO(sfp56_p47_tx_flt                , cpld, SFP56_P47_TXFLT);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p52_efuse_pg            , cpld, QSFPDD_P52_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p53_efuse_pg            , cpld, QSFPDD_P53_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p48_intr                , cpld, QSFP28_P48_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p49_intr                , cpld, QSFP28_P49_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p50_intr                , cpld, QSFP28_P50_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p51_intr                , cpld, QSFP28_P51_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p52_intr                , cpld, QSFPDD_P52_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p53_intr                , cpld, QSFPDD_P53_INTR);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p48_rst                 , cpld, QSFP28_P48_RST);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p49_rst                 , cpld, QSFP28_P49_RST);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p50_rst                 , cpld, QSFP28_P50_RST);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p51_rst                 , cpld, QSFP28_P51_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p52_rst                 , cpld, QSFPDD_P52_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p53_rst                 , cpld, QSFPDD_P53_RST);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p48_lp_mode             , cpld, QSFP28_P48_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p49_lp_mode             , cpld, QSFP28_P49_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p50_lp_mode             , cpld, QSFP28_P50_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p51_lp_mode             , cpld, QSFP28_P51_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p52_lp_mode             , cpld, QSFPDD_P52_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p53_lp_mode             , cpld, QSFPDD_P53_LP_MODE);
static SENSOR_DEVICE_ATTR_RO(fpga_id                       , cpld, FPGA_ID);
static SENSOR_DEVICE_ATTR_RO(fpga_ver_1                    , cpld, FPGA_VER_1);
static SENSOR_DEVICE_ATTR_RO(fpga_minor_ver                , cpld, FPGA_MINOR_VER);
static SENSOR_DEVICE_ATTR_RO(fpga_major_ver                , cpld, FPGA_MAJOR_VER);
static SENSOR_DEVICE_ATTR_RO(fpga_build_ver                , cpld, FPGA_BUILD_VER);
static SENSOR_DEVICE_ATTR_RO(fpga_version_h                , version_h, FPGA_VERSION_H);
static SENSOR_DEVICE_ATTR_RO(fpga_dev_info                 , cpld, FPGA_DEV_INFO);


//BSP DEBUG
static SENSOR_DEVICE_ATTR_RW(bsp_debug     , bsp_callback, BSP_DEBUG);

//MUX
static SENSOR_DEVICE_ATTR_RW(idle_state, idle_state, IDLE_STATE);

/* define support attributes of cpldx */

/* cpld 1 */
static struct attribute *cpld1_attributes[] = {

    // CPLD Common
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(event_detect_ctrl),
    _DEVICE_ATTR(cpld_write_protect),
    // CPLD 1
    _DEVICE_ATTR(cpld_sku_id),
    _DEVICE_ATTR(cpld_hw_rev),
    _DEVICE_ATTR(cpld_deph_rev),
    _DEVICE_ATTR(cpld_build_rev),
    _DEVICE_ATTR(cpld_brd_id_type),
    _DEVICE_ATTR(cpld_chip_type),
    _DEVICE_ATTR(ntm_rst),
    _DEVICE_ATTR(bmc_rst),
    _DEVICE_ATTR(cpld_2_3_rst),
    _DEVICE_ATTR(fpga_rst),
    _DEVICE_ATTR(i2c_mux_1_rst),
    _DEVICE_ATTR(i2c_mux_2_rst),
    _DEVICE_ATTR(i2c_mux_3_rst),
    _DEVICE_ATTR(ioexp_1_rst),
    _DEVICE_ATTR(ioexp_2_rst),
    _DEVICE_ATTR(fan_i2c_mux_rst),
    _DEVICE_ATTR(psu_0_prsnt),
    _DEVICE_ATTR(psu_1_prsnt),
    _DEVICE_ATTR(psu_0_acin),
    _DEVICE_ATTR(psu_1_acin),
    _DEVICE_ATTR(psu_0_pg),
    _DEVICE_ATTR(psu_1_pg),
    _DEVICE_ATTR(cpu_pg),
    _DEVICE_ATTR(mb_cpu_pg),
    _DEVICE_ATTR(bmc_pg),
    _DEVICE_ATTR(cpu_mux_sel),
    _DEVICE_ATTR(psu_mux_sel),
    _DEVICE_ATTR(system_led_status),
    _DEVICE_ATTR(fan_led_status),
    _DEVICE_ATTR(psu_0_led_status),
    _DEVICE_ATTR(psu_1_led_status),
    _DEVICE_ATTR(sync_led_status),
    _DEVICE_ATTR(id_led_status),
    
    _DEVICE_ATTR(bsp_debug),

    NULL
};

/* cpld 2 */
static struct attribute *cpld2_attributes[] = {

    // CPLD Common
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(event_detect_ctrl),
    _DEVICE_ATTR(cpld_write_protect),
    // PORT FUNC
    _DEVICE_ATTR(sfp28_p0_abs),
    _DEVICE_ATTR(sfp28_p1_abs),
    _DEVICE_ATTR(sfp28_p2_abs),
    _DEVICE_ATTR(sfp28_p3_abs),
    _DEVICE_ATTR(sfp28_p4_abs),
    _DEVICE_ATTR(sfp28_p5_abs),
    _DEVICE_ATTR(sfp28_p6_abs),
    _DEVICE_ATTR(sfp28_p7_abs),
    _DEVICE_ATTR(sfp28_p8_abs),
    _DEVICE_ATTR(sfp28_p9_abs),
    _DEVICE_ATTR(sfp28_p10_abs),
    _DEVICE_ATTR(sfp28_p11_abs),
    _DEVICE_ATTR(sfp28_p12_abs),
    _DEVICE_ATTR(sfp28_p13_abs),
    _DEVICE_ATTR(sfp28_p14_abs),
    _DEVICE_ATTR(sfp28_p15_abs),
    _DEVICE_ATTR(sfp28_p16_abs),
    _DEVICE_ATTR(sfp28_p17_abs),
    _DEVICE_ATTR(sfp28_p18_abs),
    _DEVICE_ATTR(sfp28_p19_abs),
    _DEVICE_ATTR(sfp28_p20_abs),
    _DEVICE_ATTR(sfp28_p21_abs),
    _DEVICE_ATTR(sfp28_p22_abs),
    _DEVICE_ATTR(sfp28_p23_abs),
    _DEVICE_ATTR(sfp28_p24_abs),
    _DEVICE_ATTR(sfp28_p25_abs),
    _DEVICE_ATTR(sfp28_p26_abs),
    _DEVICE_ATTR(sfp28_p27_abs),
    _DEVICE_ATTR(sfp28_p0_i2c_stuck),
    _DEVICE_ATTR(sfp28_p1_i2c_stuck),
    _DEVICE_ATTR(sfp28_p2_i2c_stuck),
    _DEVICE_ATTR(sfp28_p3_i2c_stuck),
    _DEVICE_ATTR(sfp28_p4_i2c_stuck),
    _DEVICE_ATTR(sfp28_p5_i2c_stuck),
    _DEVICE_ATTR(sfp28_p6_i2c_stuck),
    _DEVICE_ATTR(sfp28_p7_i2c_stuck),
    _DEVICE_ATTR(sfp28_p8_i2c_stuck),
    _DEVICE_ATTR(sfp28_p9_i2c_stuck),
    _DEVICE_ATTR(sfp28_p10_i2c_stuck),
    _DEVICE_ATTR(sfp28_p11_i2c_stuck),
    _DEVICE_ATTR(sfp28_p12_i2c_stuck),
    _DEVICE_ATTR(sfp28_p13_i2c_stuck),
    _DEVICE_ATTR(sfp28_p14_i2c_stuck),
    _DEVICE_ATTR(sfp28_p15_i2c_stuck),
    _DEVICE_ATTR(sfp28_p16_i2c_stuck),
    _DEVICE_ATTR(sfp28_p17_i2c_stuck),
    _DEVICE_ATTR(sfp28_p18_i2c_stuck),
    _DEVICE_ATTR(sfp28_p19_i2c_stuck),
    _DEVICE_ATTR(sfp28_p20_i2c_stuck),
    _DEVICE_ATTR(sfp28_p21_i2c_stuck),
    _DEVICE_ATTR(sfp28_p22_i2c_stuck),
    _DEVICE_ATTR(sfp28_p23_i2c_stuck),
    _DEVICE_ATTR(sfp28_p24_i2c_stuck),
    _DEVICE_ATTR(sfp28_p25_i2c_stuck),
    _DEVICE_ATTR(sfp28_p26_i2c_stuck),
    _DEVICE_ATTR(sfp28_p27_i2c_stuck),
    _DEVICE_ATTR(port_0_7_abs_event),
    _DEVICE_ATTR(port_8_15_abs_event),
    _DEVICE_ATTR(port_16_23_abs_event),
    _DEVICE_ATTR(port_24_27_abs_event),
    _DEVICE_ATTR(sfp28_p0_tx_dis),
    _DEVICE_ATTR(sfp28_p1_tx_dis),
    _DEVICE_ATTR(sfp28_p2_tx_dis),
    _DEVICE_ATTR(sfp28_p3_tx_dis),
    _DEVICE_ATTR(sfp28_p4_tx_dis),
    _DEVICE_ATTR(sfp28_p5_tx_dis),
    _DEVICE_ATTR(sfp28_p6_tx_dis),
    _DEVICE_ATTR(sfp28_p7_tx_dis),
    _DEVICE_ATTR(sfp28_p8_tx_dis),
    _DEVICE_ATTR(sfp28_p9_tx_dis),
    _DEVICE_ATTR(sfp28_p10_tx_dis),
    _DEVICE_ATTR(sfp28_p11_tx_dis),
    _DEVICE_ATTR(sfp28_p12_tx_dis),
    _DEVICE_ATTR(sfp28_p13_tx_dis),
    _DEVICE_ATTR(sfp28_p14_tx_dis),
    _DEVICE_ATTR(sfp28_p15_tx_dis),
    _DEVICE_ATTR(sfp28_p16_tx_dis),
    _DEVICE_ATTR(sfp28_p17_tx_dis),
    _DEVICE_ATTR(sfp28_p18_tx_dis),
    _DEVICE_ATTR(sfp28_p19_tx_dis),
    _DEVICE_ATTR(sfp28_p20_tx_dis),
    _DEVICE_ATTR(sfp28_p21_tx_dis),
    _DEVICE_ATTR(sfp28_p22_tx_dis),
    _DEVICE_ATTR(sfp28_p23_tx_dis),
    _DEVICE_ATTR(sfp28_p24_tx_dis),
    _DEVICE_ATTR(sfp28_p25_tx_dis),
    _DEVICE_ATTR(sfp28_p26_tx_dis),
    _DEVICE_ATTR(sfp28_p27_tx_dis),
    _DEVICE_ATTR(sfp28_p0_rs),
    _DEVICE_ATTR(sfp28_p1_rs),
    _DEVICE_ATTR(sfp28_p2_rs),
    _DEVICE_ATTR(sfp28_p3_rs),
    _DEVICE_ATTR(sfp28_p4_rs),
    _DEVICE_ATTR(sfp28_p5_rs),
    _DEVICE_ATTR(sfp28_p6_rs),
    _DEVICE_ATTR(sfp28_p7_rs),
    _DEVICE_ATTR(sfp28_p8_rs),
    _DEVICE_ATTR(sfp28_p9_rs),
    _DEVICE_ATTR(sfp28_p10_rs),
    _DEVICE_ATTR(sfp28_p11_rs),
    _DEVICE_ATTR(sfp28_p12_rs),
    _DEVICE_ATTR(sfp28_p13_rs),
    _DEVICE_ATTR(sfp28_p14_rs),
    _DEVICE_ATTR(sfp28_p15_rs),
    _DEVICE_ATTR(sfp28_p16_rs),
    _DEVICE_ATTR(sfp28_p17_rs),
    _DEVICE_ATTR(sfp28_p18_rs),
    _DEVICE_ATTR(sfp28_p19_rs),
    _DEVICE_ATTR(sfp28_p20_rs),
    _DEVICE_ATTR(sfp28_p21_rs),
    _DEVICE_ATTR(sfp28_p22_rs),
    _DEVICE_ATTR(sfp28_p23_rs),
    _DEVICE_ATTR(sfp28_p24_rs),
    _DEVICE_ATTR(sfp28_p25_rs),
    _DEVICE_ATTR(sfp28_p26_rs),
    _DEVICE_ATTR(sfp28_p27_rs),
    _DEVICE_ATTR(sfp28_p0_rx_los),
    _DEVICE_ATTR(sfp28_p1_rx_los),
    _DEVICE_ATTR(sfp28_p2_rx_los),
    _DEVICE_ATTR(sfp28_p3_rx_los),
    _DEVICE_ATTR(sfp28_p4_rx_los),
    _DEVICE_ATTR(sfp28_p5_rx_los),
    _DEVICE_ATTR(sfp28_p6_rx_los),
    _DEVICE_ATTR(sfp28_p7_rx_los),
    _DEVICE_ATTR(sfp28_p8_rx_los),
    _DEVICE_ATTR(sfp28_p9_rx_los),
    _DEVICE_ATTR(sfp28_p10_rx_los),
    _DEVICE_ATTR(sfp28_p11_rx_los),
    _DEVICE_ATTR(sfp28_p12_rx_los),
    _DEVICE_ATTR(sfp28_p13_rx_los),
    _DEVICE_ATTR(sfp28_p14_rx_los),
    _DEVICE_ATTR(sfp28_p15_rx_los),
    _DEVICE_ATTR(sfp28_p16_rx_los),
    _DEVICE_ATTR(sfp28_p17_rx_los),
    _DEVICE_ATTR(sfp28_p18_rx_los),
    _DEVICE_ATTR(sfp28_p19_rx_los),
    _DEVICE_ATTR(sfp28_p20_rx_los),
    _DEVICE_ATTR(sfp28_p21_rx_los),
    _DEVICE_ATTR(sfp28_p22_rx_los),
    _DEVICE_ATTR(sfp28_p23_rx_los),
    _DEVICE_ATTR(sfp28_p24_rx_los),
    _DEVICE_ATTR(sfp28_p25_rx_los),
    _DEVICE_ATTR(sfp28_p26_rx_los),
    _DEVICE_ATTR(sfp28_p27_rx_los),
    _DEVICE_ATTR(sfp28_p0_tx_flt),
    _DEVICE_ATTR(sfp28_p1_tx_flt),
    _DEVICE_ATTR(sfp28_p2_tx_flt),
    _DEVICE_ATTR(sfp28_p3_tx_flt),
    _DEVICE_ATTR(sfp28_p4_tx_flt),
    _DEVICE_ATTR(sfp28_p5_tx_flt),
    _DEVICE_ATTR(sfp28_p6_tx_flt),
    _DEVICE_ATTR(sfp28_p7_tx_flt),
    _DEVICE_ATTR(sfp28_p8_tx_flt),
    _DEVICE_ATTR(sfp28_p9_tx_flt),
    _DEVICE_ATTR(sfp28_p10_tx_flt),
    _DEVICE_ATTR(sfp28_p11_tx_flt),
    _DEVICE_ATTR(sfp28_p12_tx_flt),
    _DEVICE_ATTR(sfp28_p13_tx_flt),
    _DEVICE_ATTR(sfp28_p14_tx_flt),
    _DEVICE_ATTR(sfp28_p15_tx_flt),
    _DEVICE_ATTR(sfp28_p16_tx_flt),
    _DEVICE_ATTR(sfp28_p17_tx_flt),
    _DEVICE_ATTR(sfp28_p18_tx_flt),
    _DEVICE_ATTR(sfp28_p19_tx_flt),
    _DEVICE_ATTR(sfp28_p20_tx_flt),
    _DEVICE_ATTR(sfp28_p21_tx_flt),
    _DEVICE_ATTR(sfp28_p22_tx_flt),
    _DEVICE_ATTR(sfp28_p23_tx_flt),
    _DEVICE_ATTR(sfp28_p24_tx_flt),
    _DEVICE_ATTR(sfp28_p25_tx_flt),
    _DEVICE_ATTR(sfp28_p26_tx_flt),
    _DEVICE_ATTR(sfp28_p27_tx_flt),
    NULL
};

/* cpld 3 */
static struct attribute *cpld3_attributes[] = {

    // CPLD Common
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(event_detect_ctrl),
    _DEVICE_ATTR(cpld_write_protect),

    _DEVICE_ATTR(sfp28_p28_abs),
    _DEVICE_ATTR(sfp28_p29_abs),
    _DEVICE_ATTR(sfp28_p30_abs),
    _DEVICE_ATTR(sfp28_p31_abs),
    _DEVICE_ATTR(sfp28_p32_abs),
    _DEVICE_ATTR(sfp28_p33_abs),
    _DEVICE_ATTR(sfp28_p34_abs),
    _DEVICE_ATTR(sfp28_p35_abs),
    _DEVICE_ATTR(sfp28_p36_abs),
    _DEVICE_ATTR(sfp28_p37_abs),
    _DEVICE_ATTR(sfp28_p38_abs),
    _DEVICE_ATTR(sfp28_p39_abs),
    _DEVICE_ATTR(sfp28_p28_tx_dis),
    _DEVICE_ATTR(sfp28_p29_tx_dis),
    _DEVICE_ATTR(sfp28_p30_tx_dis),
    _DEVICE_ATTR(sfp28_p31_tx_dis),
    _DEVICE_ATTR(sfp28_p32_tx_dis),
    _DEVICE_ATTR(sfp28_p33_tx_dis),
    _DEVICE_ATTR(sfp28_p34_tx_dis),
    _DEVICE_ATTR(sfp28_p35_tx_dis),
    _DEVICE_ATTR(sfp28_p36_tx_dis),
    _DEVICE_ATTR(sfp28_p37_tx_dis),
    _DEVICE_ATTR(sfp28_p38_tx_dis),
    _DEVICE_ATTR(sfp28_p39_tx_dis),
    _DEVICE_ATTR(sfp28_p28_rs),
    _DEVICE_ATTR(sfp28_p29_rs),
    _DEVICE_ATTR(sfp28_p30_rs),
    _DEVICE_ATTR(sfp28_p31_rs),
    _DEVICE_ATTR(sfp28_p32_rs),
    _DEVICE_ATTR(sfp28_p33_rs),
    _DEVICE_ATTR(sfp28_p34_rs),
    _DEVICE_ATTR(sfp28_p35_rs),
    _DEVICE_ATTR(sfp28_p36_rs),
    _DEVICE_ATTR(sfp28_p37_rs),
    _DEVICE_ATTR(sfp28_p38_rs),
    _DEVICE_ATTR(sfp28_p39_rs),
    _DEVICE_ATTR(sfp28_p28_rx_los),
    _DEVICE_ATTR(sfp28_p29_rx_los),
    _DEVICE_ATTR(sfp28_p30_rx_los),
    _DEVICE_ATTR(sfp28_p31_rx_los),
    _DEVICE_ATTR(sfp28_p32_rx_los),
    _DEVICE_ATTR(sfp28_p33_rx_los),
    _DEVICE_ATTR(sfp28_p34_rx_los),
    _DEVICE_ATTR(sfp28_p35_rx_los),
    _DEVICE_ATTR(sfp28_p36_rx_los),
    _DEVICE_ATTR(sfp28_p37_rx_los),
    _DEVICE_ATTR(sfp28_p38_rx_los),
    _DEVICE_ATTR(sfp28_p39_rx_los),
    _DEVICE_ATTR(sfp28_p28_tx_flt),
    _DEVICE_ATTR(sfp28_p29_tx_flt),
    _DEVICE_ATTR(sfp28_p30_tx_flt),
    _DEVICE_ATTR(sfp28_p31_tx_flt),
    _DEVICE_ATTR(sfp28_p32_tx_flt),
    _DEVICE_ATTR(sfp28_p33_tx_flt),
    _DEVICE_ATTR(sfp28_p34_tx_flt),
    _DEVICE_ATTR(sfp28_p35_tx_flt),
    _DEVICE_ATTR(sfp28_p36_tx_flt),
    _DEVICE_ATTR(sfp28_p37_tx_flt),
    _DEVICE_ATTR(sfp28_p38_tx_flt),
    _DEVICE_ATTR(sfp28_p39_tx_flt),
    _DEVICE_ATTR(sfp56_p40_abs),
    _DEVICE_ATTR(sfp56_p41_abs),
    _DEVICE_ATTR(sfp56_p42_abs),
    _DEVICE_ATTR(sfp56_p43_abs),
    _DEVICE_ATTR(sfp56_p44_abs),
    _DEVICE_ATTR(sfp56_p45_abs),
    _DEVICE_ATTR(sfp56_p46_abs),
    _DEVICE_ATTR(sfp56_p47_abs),
    _DEVICE_ATTR(sfp56_p40_tx_dis),
    _DEVICE_ATTR(sfp56_p41_tx_dis),
    _DEVICE_ATTR(sfp56_p42_tx_dis),
    _DEVICE_ATTR(sfp56_p43_tx_dis),
    _DEVICE_ATTR(sfp56_p44_tx_dis),
    _DEVICE_ATTR(sfp56_p45_tx_dis),
    _DEVICE_ATTR(sfp56_p46_tx_dis),
    _DEVICE_ATTR(sfp56_p47_tx_dis),
    _DEVICE_ATTR(sfp56_p40_rs),
    _DEVICE_ATTR(sfp56_p41_rs),
    _DEVICE_ATTR(sfp56_p42_rs),
    _DEVICE_ATTR(sfp56_p43_rs),
    _DEVICE_ATTR(sfp56_p44_rs),
    _DEVICE_ATTR(sfp56_p45_rs),
    _DEVICE_ATTR(sfp56_p46_rs),
    _DEVICE_ATTR(sfp56_p47_rs),
    _DEVICE_ATTR(sfp56_p40_rx_los),
    _DEVICE_ATTR(sfp56_p41_rx_los),
    _DEVICE_ATTR(sfp56_p42_rx_los),
    _DEVICE_ATTR(sfp56_p43_rx_los),
    _DEVICE_ATTR(sfp56_p44_rx_los),
    _DEVICE_ATTR(sfp56_p45_rx_los),
    _DEVICE_ATTR(sfp56_p46_rx_los),
    _DEVICE_ATTR(sfp56_p47_rx_los),
    _DEVICE_ATTR(sfp56_p40_tx_flt),
    _DEVICE_ATTR(sfp56_p41_tx_flt),
    _DEVICE_ATTR(sfp56_p42_tx_flt),
    _DEVICE_ATTR(sfp56_p43_tx_flt),
    _DEVICE_ATTR(sfp56_p44_tx_flt),
    _DEVICE_ATTR(sfp56_p45_tx_flt),
    _DEVICE_ATTR(sfp56_p46_tx_flt),
    _DEVICE_ATTR(sfp56_p47_tx_flt),
    _DEVICE_ATTR(qsfp28_p48_abs),
    _DEVICE_ATTR(qsfp28_p49_abs),
    _DEVICE_ATTR(qsfp28_p50_abs),
    _DEVICE_ATTR(qsfp28_p51_abs),
    _DEVICE_ATTR(qsfpdd_p52_abs),
    _DEVICE_ATTR(qsfpdd_p53_abs),
    _DEVICE_ATTR(qsfp28_p48_rst),
    _DEVICE_ATTR(qsfp28_p49_rst),
    _DEVICE_ATTR(qsfp28_p50_rst),
    _DEVICE_ATTR(qsfp28_p51_rst),
    _DEVICE_ATTR(qsfpdd_p52_rst),
    _DEVICE_ATTR(qsfpdd_p53_rst),
    _DEVICE_ATTR(qsfp28_p48_lp_mode),
    _DEVICE_ATTR(qsfp28_p49_lp_mode),
    _DEVICE_ATTR(qsfp28_p50_lp_mode),
    _DEVICE_ATTR(qsfp28_p51_lp_mode),
    _DEVICE_ATTR(qsfpdd_p52_lp_mode),
    _DEVICE_ATTR(qsfpdd_p53_lp_mode),
    _DEVICE_ATTR(qsfp28_p48_intr),
    _DEVICE_ATTR(qsfp28_p49_intr),
    _DEVICE_ATTR(qsfp28_p50_intr),
    _DEVICE_ATTR(qsfp28_p51_intr),
    _DEVICE_ATTR(qsfpdd_p52_intr),
    _DEVICE_ATTR(qsfpdd_p53_intr),
    _DEVICE_ATTR(qsfpdd_p52_efuse_pg),
    _DEVICE_ATTR(qsfpdd_p53_efuse_pg),
    _DEVICE_ATTR(port_28_35_abs_event),
    _DEVICE_ATTR(port_36_43_abs_event),
    _DEVICE_ATTR(port_44_51_abs_event),
    _DEVICE_ATTR(port_52_53_abs_event),
    _DEVICE_ATTR(sfp28_p28_i2c_stuck),
    _DEVICE_ATTR(sfp28_p29_i2c_stuck),
    _DEVICE_ATTR(sfp28_p30_i2c_stuck),
    _DEVICE_ATTR(sfp28_p31_i2c_stuck),
    _DEVICE_ATTR(sfp28_p32_i2c_stuck),
    _DEVICE_ATTR(sfp28_p33_i2c_stuck),
    _DEVICE_ATTR(sfp28_p34_i2c_stuck),
    _DEVICE_ATTR(sfp28_p35_i2c_stuck),
    _DEVICE_ATTR(sfp28_p36_i2c_stuck),
    _DEVICE_ATTR(sfp28_p37_i2c_stuck),
    _DEVICE_ATTR(sfp28_p38_i2c_stuck),
    _DEVICE_ATTR(sfp28_p39_i2c_stuck),
    _DEVICE_ATTR(sfp56_p40_i2c_stuck),
    _DEVICE_ATTR(sfp56_p41_i2c_stuck),
    _DEVICE_ATTR(sfp56_p42_i2c_stuck),
    _DEVICE_ATTR(sfp56_p43_i2c_stuck),
    _DEVICE_ATTR(sfp56_p44_i2c_stuck),
    _DEVICE_ATTR(sfp56_p45_i2c_stuck),
    _DEVICE_ATTR(sfp56_p46_i2c_stuck),
    _DEVICE_ATTR(sfp56_p47_i2c_stuck),
    _DEVICE_ATTR(qsfp28_p48_i2c_stuck),
    _DEVICE_ATTR(qsfp28_p49_i2c_stuck),
    _DEVICE_ATTR(qsfp28_p50_i2c_stuck),
    _DEVICE_ATTR(qsfp28_p51_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_p52_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_p53_i2c_stuck),
    NULL
};

/* fpga */
// change to fpga
static struct attribute *fpga_attributes[] = {
    _DEVICE_ATTR(fpga_id),
    _DEVICE_ATTR(fpga_ver_1),
    _DEVICE_ATTR(fpga_minor_ver),
    _DEVICE_ATTR(fpga_major_ver),
    _DEVICE_ATTR(fpga_build_ver),
    _DEVICE_ATTR(fpga_version_h),
    _DEVICE_ATTR(fpga_dev_info),
    _DEVICE_ATTR(event_detect_ctrl),

    NULL
};



/* cpld 1 attributes group */
static const struct attribute_group cpld1_group = {
    .attrs = cpld1_attributes,
};

/* cpld 2 attributes group */
static const struct attribute_group cpld2_group = {
    .attrs = cpld2_attributes,
};

/* cpld 3 attributes group */
static const struct attribute_group cpld3_group = {
    .attrs = cpld3_attributes,
};

/* fpga attributes group */
static const struct attribute_group fpga_group = {
    .attrs = fpga_attributes,
};

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
u8 _mask_shift(u8 val, u8 mask)
{
    int shift=0;

    shift = _shift(mask);

    return (val & mask) >> shift;
}

static u8 _parse_data(char *buf, unsigned int data, u8 data_type)
{
    if(buf == NULL) {
        return -1;
    }

    if(data_type == DATA_HEX) {
        return sprintf(buf, "0x%02x", data);
    } else if(data_type == DATA_DEC) {
        return sprintf(buf, "%u", data);
    } else {
        return -1;
    }
    return 0;
}

static int _bsp_log(u8 log_type, char *fmt, ...)
{
    if ((log_type==LOG_READ  && enable_log_read) ||
        (log_type==LOG_WRITE && enable_log_write)) {
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

/* get bsp value */
static ssize_t bsp_read(char *buf, char *str)
{
    ssize_t len=0;

    len=sprintf(buf, "%s", str);
    BSP_LOG_R("reg_val=%s", str);

    return len;
}

/* set bsp value */
static ssize_t bsp_write(const char *buf, char *str, size_t str_len, size_t count)
{
    snprintf(str, str_len, "%s", buf);
    BSP_LOG_W("reg_val=%s", str);

    return count;
}

/* get bsp parameter value */
static ssize_t bsp_callback_show(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int str_len=0;
    char *str=NULL;

    switch (attr->index) {
        case BSP_DEBUG:
            str = bsp_debug;
            str_len = sizeof(bsp_debug);
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
    ssize_t ret = 0;
    u8 bsp_debug_u8 = 0;

    switch (attr->index) {
        case BSP_DEBUG:
            str = bsp_debug;
            str_len = sizeof(bsp_debug);
            ret = bsp_write(buf, str, str_len, count);

            if (kstrtou8(buf, 0, &bsp_debug_u8) < 0) {
                return -EINVAL;
            } else if (_config_bsp_log(bsp_debug_u8) < 0) {
                return -EINVAL;
            }
            return ret;
        default:
            return -EINVAL;
    }
    return 0;
}

/* get cpld register value */
static ssize_t cpld_show(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg = 0;
    u8 mask = MASK_NONE;
    u8 data_type=DATA_UNK;

    switch (attr->index) {
        //CPLD Common
        case CPLD_MINOR_VER:
        case CPLD_MAJOR_VER:
        case CPLD_ID:
        case CPLD_BUILD_VER:
        case EVENT_DETECT_CTRL:
        case CPLD_WRITE_PROTECT:    

        //CPLD 1
        case CPLD_SKU_ID:
        case CPLD_HW_REV:
        case CPLD_DEPH_REV:
        case CPLD_BUILD_REV:
        case CPLD_BRD_ID_TYPE:
        case CPLD_CHIP_TYPE:
        case FPGA_INTR:
        case NTM_RST:
        case BMC_RST:
        case CPLD_2_3_RST:
        case FPGA_RST:
        case I2C_MUX_1_RST:
        case I2C_MUX_2_RST:
        case I2C_MUX_3_RST:
        case IOEXP_1_RST:
        case IOEXP_2_RST:
        case FAN_I2C_MUX_RST:
        case PSU_0_PRSNT:
        case PSU_1_PRSNT:
        case PSU_0_ACIN:
        case PSU_1_ACIN:
        case PSU_0_PG:
        case PSU_1_PG:
        case CPU_PG:
        case MB_CPU_PG:
        case BMC_PG:
        case CPU_MUX_SEL:
        case PSU_MUX_SEL:
        case SYSTEM_LED_STATUS:
        case FAN_LED_STATUS:
        case PSU_0_LED_STATUS:
        case PSU_1_LED_STATUS:
        case SYNC_LED_STATUS:
        case ID_LED_STATUS:
        // PORT FUNC
        case SFP28_P0_ABS ... QSFPDD_P53_ABS:
        case SFP28_P0_I2C_STUCK ... QSFPDD_P53_I2C_STUCK:
        case PORT_0_7_ABS_EVENT ... PORT_52_53_ABS_EVENT:
        case SFP28_P0_TXDIS ... SFP56_P47_TXDIS:
        case SFP28_P0_RS ... SFP56_P47_RS:
        case SFP28_P0_RXLOS ... SFP56_P47_RXLOS:
        case SFP28_P0_TXFLT ... SFP56_P47_TXFLT:
        case QSFP28_P48_INTR ... QSFPDD_P53_INTR:
        case QSFP28_P48_RST ... QSFPDD_P53_RST:
        case QSFP28_P48_LP_MODE ... QSFPDD_P53_LP_MODE:
        case QSFPDD_P52_EFUSE_PG ... QSFPDD_P53_EFUSE_PG:
        
            reg = attr_reg[attr->index].reg;
            mask= attr_reg[attr->index].mask;
            data_type = attr_reg[attr->index].data_type;
            break;
        default:
            return -EINVAL;
    }
    return cpld_reg_read(dev, buf, reg, mask, data_type);
}

/* set cpld register value */
static ssize_t cpld_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg = 0;
    u8 mask = MASK_NONE;
    bool write_protect;

    switch (attr->index) {
        case NTM_RST:
        case BMC_RST:
        case CPLD_2_3_RST:
        case FPGA_RST:
        case I2C_MUX_1_RST:
        case I2C_MUX_2_RST:
        case I2C_MUX_3_RST:
        case IOEXP_1_RST:
        case IOEXP_2_RST:
        case FAN_I2C_MUX_RST:
        case SFP28_P0_TXDIS ... SFP56_P47_TXDIS:
        case QSFP28_P48_RST ... QSFPDD_P53_RST:
        case QSFP28_P48_LP_MODE ... QSFPDD_P53_LP_MODE:
        case EVENT_DETECT_CTRL:
        case CPLD_WRITE_PROTECT:
        case SYSTEM_LED_STATUS:
        case FAN_LED_STATUS:
        case PSU_0_LED_STATUS:
        case PSU_1_LED_STATUS:
        case SYNC_LED_STATUS:
        case ID_LED_STATUS:
        case SFP28_P0_RS ... SFP56_P47_RS:
            reg = attr_reg[attr->index].reg;
            mask= attr_reg[attr->index].mask;
            write_protect = attr_reg[attr->index].write_protect;
            break;
        default:
            return -EINVAL;
    }
    return cpld_reg_write(dev, buf, count, reg, mask, write_protect);
}

/* get cpld register value */
u8 _cpld_reg_read(struct device *dev,
                    u8 reg,
                    u8 mask)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct cpld_data *data = i2c_mux_priv(muxc);
    int reg_val;

    I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);

    if (unlikely(reg_val < 0)) {
        return reg_val;
    } else {
        reg_val=_mask_shift(reg_val, mask);
        return reg_val;
    }
}

/* get cpld register value */
static ssize_t cpld_reg_read(struct device *dev,
                    char *buf,
                    u8 reg,
                    u8 mask,
                    u8 data_type)
{
    int reg_val;

    reg_val = _cpld_reg_read(dev, reg, mask);
    if (unlikely(reg_val < 0)) {
        dev_err(dev, "cpld_reg_read() error, reg_val=%d\n", reg_val);
        return reg_val;
    } else {
        return _parse_data(buf, reg_val, data_type);
    }
}

u8 _cpld_reg_write(struct device *dev,
                    u8 reg,
                    u8 reg_val)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct cpld_data *data = i2c_mux_priv(muxc);
    int ret = 0;

    I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
               client, reg, reg_val);

    return ret;
}

/* set cpld register value */
static ssize_t cpld_reg_write(struct device *dev,
                    const char *buf,
                    size_t count,
                    u8 reg,
                    u8 mask,
                    bool write_protect)
{
    u8 reg_val, reg_val_now, shift;
    int ret = 0;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    //apply continuous bits operation if mask is specified, discontinuous bits are not supported
    if (mask != MASK_ALL) {
        reg_val_now = _cpld_reg_read(dev, reg, MASK_ALL);
        if (unlikely(reg_val_now < 0)) {
            dev_err(dev, "cpld_reg_write() error, reg_val_now=%d\n", reg_val_now);
            return reg_val_now;
        } else {
            //clear bits in reg_val_now by the mask
            reg_val_now &= ~mask;
            //get bit shift by the mask
            shift = _shift(mask);
            //calculate new reg_val
            reg_val = reg_val_now | (reg_val << shift);
        }
    }

    if (write_protect) {
        ret = _cpld_reg_write_with_protect(dev, reg, reg_val);
    }
    else {
        ret = _cpld_reg_write(dev, reg, reg_val);
    }

    if (unlikely(ret < 0)) {
        dev_err(dev, "cpld_reg_write() error, return=%d\n", ret);
        return ret;
    }

    return count;
}

/* set cpld register value with protect */
static ssize_t _cpld_reg_write_with_protect(struct device *dev,
                                            u8 reg,
                                            u8 reg_val)
{
    int ret = 0;
    u8 reg_wp = CPLD_WRITE_PROTECT_REG;
    u8 reg_wp_val = 0;
    u8 reg_wp_mask = 0x01;
    struct i2c_client *i2c_client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(i2c_client);
    struct cpld_data *data = i2c_mux_priv(muxc);

    BSP_LOG_W("Writing protected cpld register, reg=0x%x, value=0x%x\n", reg, reg_val);

    // lock the write protect session
    mutex_lock(&data->access_lock);

    // read the write protect reg
    ret = i2c_smbus_read_byte_data(i2c_client, reg_wp);
    if (unlikely(ret < 0)) {
        dev_err(dev, "i2c_smbus_read_byte_data() error, reg=0x%x, return=%d\n", reg_wp, ret);
        goto error;
    }

    reg_wp_val = ret;

    // check if write enable
    if ((reg_wp_val & reg_wp_mask)) {
        // write target reg
        ret = i2c_smbus_write_byte_data(i2c_client, reg, reg_val);

        if (unlikely(ret < 0)){
            dev_err(dev, "i2c_smbus_write_byte_data() error, reg=0x%x, value=0x%x, return=%d\n", reg, reg_val, ret);
            goto error;
        }
    }
    else {
        // enable reg write
        reg_wp_val |= reg_wp_mask;
        ret = i2c_smbus_write_byte_data(i2c_client, reg_wp, reg_wp_val);

        if (unlikely(ret < 0)) {
            dev_err(dev, "i2c_smbus_write_byte_data() error, reg=0x%x, value=0x%x, return=%d\n", reg_wp, reg_wp_val, ret);
            goto error;
        }

        // write target reg
        ret = i2c_smbus_write_byte_data(i2c_client, reg, reg_val);

        if (unlikely(ret < 0)) {
            dev_err(dev, "i2c_smbus_write_byte_data() error, reg=0x%x, value=0x%x, return=%d\n", reg, reg_val, ret);

            // restore write protect reg
            reg_wp_val &= ~reg_wp_mask;
            ret = i2c_smbus_write_byte_data(i2c_client, reg_wp, reg_wp_val);
            if (unlikely(ret < 0)) {
                dev_err(dev, "i2c_smbus_write_byte_data() error, reg=0x%x, value=0x%x, return=%d\n", reg_wp, reg_wp_val, ret);
            }
            goto error;
        }

        // disable reg write
        reg_wp_val &= ~reg_wp_mask;
        ret = i2c_smbus_write_byte_data(i2c_client, reg_wp, reg_wp_val);

        if (unlikely(ret < 0)) {
            dev_err(dev, "i2c_smbus_write_byte_data() error, reg=0x%x, value=0x%x, return=%d\n", reg_wp, reg_wp_val, ret);
            goto error;
        }
    }

error:
    // unlock the write protect session
    mutex_unlock(&data->access_lock);
    return ret;
}



/* get cpld/fpga verison human read */
static ssize_t version_h_show(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int major =-1;
    int minor =-1;
    int build =-1;

    switch(attr->index) {
        case CPLD_VERSION_H:
            major = CPLD_MAJOR_VER;
            minor = CPLD_MINOR_VER;
            build = CPLD_BUILD_VER;
            break;
        case FPGA_VERSION_H:
            major = FPGA_MAJOR_VER;
            minor = FPGA_MINOR_VER;
            build = FPGA_BUILD_VER;
            break;
        default:
            major=-1;
            minor=-1;
            build=-1;
            break;
    }

    if (major >= 0 && minor >= 0 && build >= 0) {
        return sprintf(buf, "%d.%02d.%03d",
                _cpld_reg_read(dev, attr_reg[major].reg, attr_reg[major].mask),
                _cpld_reg_read(dev, attr_reg[minor].reg, attr_reg[minor].mask),
                _cpld_reg_read(dev, attr_reg[build].reg, attr_reg[build].mask));
    }
    return -1;
}

/* add valid cpld client to list */
static void cpld_add_client(struct i2c_client *client)
{
    struct cpld_client_node *node = NULL;

    node = kzalloc(sizeof(struct cpld_client_node), GFP_KERNEL);
    if (!node) {
        dev_info(&client->dev,
            "Can't allocate cpld_client_node for index %d\n",
            client->addr);
        return;
    }

    node->client = client;

    mutex_lock(&list_lock);
    list_add(&node->list, &cpld_client_list);
    mutex_unlock(&list_lock);
}

/* remove exist cpld client in list */
static void cpld_remove_client(struct i2c_client *client)
{
    struct list_head    *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int found = 0;

    mutex_lock(&list_lock);
    list_for_each(list_node, &cpld_client_list) {
        cpld_node = list_entry(list_node,
                struct cpld_client_node, list);

        if (cpld_node->client == client) {
            found = 1;
            break;
        }
    }

    if (found) {
        list_del(list_node);
        kfree(cpld_node);
    }
    mutex_unlock(&list_lock);
}

/* cpld drvier probe */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0)
static int cpld_probe(struct i2c_client *client,
                    const struct i2c_device_id *dev_id)
{
#else
static int cpld_probe(struct i2c_client *client)
{
    const struct i2c_device_id *dev_id = i2c_client_get_device_id(client);
#endif

    int status;
    struct i2c_adapter *adap = client->adapter;
    struct device *dev = &client->dev;
    struct cpld_data *data = NULL;
    struct i2c_mux_core *muxc;
    //int ret = -EPERM;

    muxc = i2c_mux_alloc(adap, dev, CPLD_MAX_NCHANS, sizeof(*data), 0,
                mux_select_chan, mux_deselect_mux);

    data = i2c_mux_priv(muxc);
    if (!data)
        return -ENOMEM;

    /* init cpld data for client */
    i2c_set_clientdata(client, muxc);

    data->client = client;
    mutex_init(&data->access_lock);

    if (!i2c_check_functionality(client->adapter,
                I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_info(&client->dev,
            "i2c_check_functionality failed (0x%x)\n",
            client->addr);
        status = -EIO;
        goto exit;
    }

    // FIXME check if fpga has ID register
    /* get cpld id from device */
    //ret = i2c_smbus_read_byte_data(client, CPLD_ID_REG);
    //
    //if (ret < 0) {
    //    dev_info(&client->dev,
    //        "fail to get cpld id (0x%x) at addr (0x%x)\n",
    //        CPLD_ID_REG, client->addr);
    //    status = -EIO;
    //    goto exit;
    //}
    //
    //if (INVALID(ret, cpld1, cpld5)) {
    //    dev_info(&client->dev,
    //        "cpld id %d(device) not valid\n", ret);
    //}

    data->index = dev_id->driver_data;

    /* register sysfs hooks for different cpld group */
    dev_info(&client->dev, "probe cpld with index %d\n", data->index);

    if(mux_en) {
        status = mux_init(dev);
        if (status < 0) {
            dev_warn(dev, "Mux init failed\n");
            goto exit;
        }
    }

    switch (data->index) {
    case cpld1:
        status = sysfs_create_group(&client->dev.kobj,
                    &cpld1_group);
        break;
    case cpld2:
        status = sysfs_create_group(&client->dev.kobj,
                    &cpld2_group);
        break;
    case fpga:
        status = sysfs_create_group(&client->dev.kobj,
                    &fpga_group);
        break;
    case cpld3:
        status = sysfs_create_group(&client->dev.kobj,
                    &cpld3_group);
        break;
    default:
        status = -EINVAL;
    }

    if(mux_en) {
        if(data->chip->nchans > 0){
            status = sysfs_add_file_to_group(&client->dev.kobj,
                        &sensor_dev_attr_idle_state.dev_attr.attr, NULL);
        }
    }

    if (status)
        goto exit;

    dev_info(&client->dev, "chip found\n");

    /* add probe chip to client list */
    cpld_add_client(client);

    return 0;
exit:
    switch (data->index) {
    case cpld1:
        sysfs_remove_group(&client->dev.kobj, &cpld1_group);
        break;
    case cpld2:
        sysfs_remove_group(&client->dev.kobj, &cpld2_group);
        break;
    case cpld3:
        sysfs_remove_group(&client->dev.kobj, &cpld3_group);
        break;
    case fpga:
        sysfs_remove_group(&client->dev.kobj, &fpga_group);
        break;
    default:
        break;
    }
    return status;
}

/* cpld drvier remove */

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
static int cpld_remove(struct i2c_client *client)
#else
static void cpld_remove(struct i2c_client *client)
#endif
{
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct device *dev = &client->dev;
    struct cpld_data *data = i2c_mux_priv(muxc);

    if(mux_en) {
        if(data->chip->nchans > 0){
            sysfs_remove_file_from_group(&client->dev.kobj,
                &sensor_dev_attr_idle_state.dev_attr.attr, NULL);
        }
    }

    switch (data->index) {
    case cpld1:
        sysfs_remove_group(&client->dev.kobj, &cpld1_group);
        break;
    case cpld2:
        sysfs_remove_group(&client->dev.kobj, &cpld2_group);
        break;
    case cpld3:
        sysfs_remove_group(&client->dev.kobj, &cpld3_group);
        break;
    case fpga:
        sysfs_remove_group(&client->dev.kobj, &fpga_group);
        break;
    }

    if(mux_en) {
        mux_cleanup(dev);
    }

    cpld_remove_client(client);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
    return 0;
#endif
}

MODULE_DEVICE_TABLE(i2c, cpld_id);

static struct i2c_driver cpld_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name = "x86_64_ufispace_s9620_54dc_cpld",
    },
    .probe = cpld_probe,
    .remove = cpld_remove,
    .id_table = cpld_id,
    .address_list = cpld_i2c_addr,
};

static int __init cpld_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&cpld_driver);
}

static void __exit cpld_exit(void)
{
    i2c_del_driver(&cpld_driver);
}

MODULE_AUTHOR("Alex Hsia <alex.hsia@ufispace.com>");
MODULE_DESCRIPTION("x86_64_ufispace_s9620_54dc_cpld driver");
MODULE_VERSION("0.0.1");
MODULE_LICENSE("GPL");

module_init(cpld_init);
module_exit(cpld_exit);
