/*
 * A i2c cpld driver for the ufispace_s9720_56ed
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
#include <linux/version.h>
#include "x86-64-ufispace-s9720-56ed-cpld-main.h"

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

#define I2C_READ_BYTE_DATA_NOLOCK(ret, i2c_client, reg) \
{ \
    ret = i2c_smbus_read_byte_data(i2c_client, reg); \
    BSP_LOG_R("cpld[%d], reg=0x%03x, reg_val=0x%02x", data->index, reg, ret); \
}

#define I2C_WRITE_BYTE_DATA_NOLOCK(ret, i2c_client, reg, val) \
{ \
    ret = i2c_smbus_write_byte_data(i2c_client, reg, val); \
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

    // CPLD 1
    CPLD_SKU_ID,
    CPLD_HW_REV,
    CPLD_DEPH_REV,
    CPLD_BUILD_REV,
    CPLD_BRD_ID_TYPE,
    CPLD_CHIP_TYPE,
    MAC_INTR,
    RGB_PHY_0_INTR,
    RGB_PHY_1_INTR,
    RGB_PHY_2_INTR,
    RGB_PHY_3_INTR,
    RGB_PHY_4_INTR,
    RGB_PHY_5_INTR,
    RGB_PHY_6_INTR,
    RGB_PHY_7_INTR,
    TOP_RGB_INTR,
    CPLD_25GPHY_INTR,
    CPLD_2_FRU_INTR,
    CPLD_4_FRU_INTR,

    SFP28_INTR,
    PSU_0_INTR,
    PSU_1_INTR,
    RETIMER_INTR,

    MAC_HBM_TEMP_ALERT,
    MB_LM75_TEMP_ALERT,
    EXT_LM75_TEMP_ALERT,
    PHY_LM75_CPLD5_INTR,
    USB_OCP,
    FPGA_INTR,
    LM75_BMC_INTR,
    LM75_CPLD1_INTR,
    CPU_INTR,
    LM75_CPLD245_INTR,

    MAC_INTR_MASK,
    RGB_PHY_0_INTR_MASK,
    RGB_PHY_1_INTR_MASK,
    RGB_PHY_2_INTR_MASK,
    RGB_PHY_3_INTR_MASK,
    RGB_PHY_4_INTR_MASK,
    RGB_PHY_5_INTR_MASK,
    RGB_PHY_6_INTR_MASK,
    RGB_PHY_7_INTR_MASK,
    TOP_RGB_INTR_MASK,
    MISC_INTR,
    SYSTEM_INTR,
    CPLD_25GPHY_INTR_MASK,
    CPLD_2_FRU_INTR_MASK,
    CPLD_4_FRU_INTR_MASK,
    SFP28_INTR_MASK,
    RETIMER_INTR_MASK,
    PSU_0_INTR_MASK,
    PSU_1_INTR_MASK,

    USB_OCP_INTR_MASK,
    MAC_TEMP_ALERT_MASK,
    MB_LM75_TEMP_ALERT_MASK,
    MB_TEMP_1_ALERT_MASK,
    MB_TEMP_2_ALERT_MASK,
    MB_TEMP_3_ALERT_MASK,
    MB_TEMP_4_ALERT_MASK,
    MB_TEMP_5_ALERT_MASK,
    MB_TEMP_6_ALERT_MASK,
    MB_TEMP_7_ALERT_MASK,
    TOP_CPLD_5_INTR_MASK,
    USB_OCP_MASK,
    FPGA_INTR_MASK,
    LM75_BMC_INTR_MASK,
    LM75_CPLD1_INTR_MASK,
    CPU_INTR_MASK,
    LM75_CPLD245_INTR_MASK,

    MAC_INTR_EVENT,
    MB_RGB_0_7_INTR_EVENT,
    TOP_RGB_INTR_EVENT,
    CPLD_25GPHY_INTR_EVENT,
    CPLD_FRU_INTR_EVENT,
    MAC_TEMP_ALERT_EVENT,
    TEMP_ALERT_EVENT,
    MISC_INTR_EVENT,
    EVENT_DETECT_CTRL,
    MAC_PCIE_RST,
    MAC_QSPI_RST,
    RGB_PHY_0_RST,
    RGB_PHY_1_RST,
    RGB_PHY_2_RST,
    RGB_PHY_3_RST,
    RGB_PHY_4_RST,
    RGB_PHY_5_RST,
    RGB_PHY_6_RST,
    RGB_PHY_7_RST,

    NTM_RST,
    BMC_RST,
    USB_REDRIVER_RST,
    USB_OCP_RCVRY,

    CPLD_2_RST,
    FPGA_RST,
    CPLD_4_RST,
    CPLD_5_RST,

    CPLD_IOEXP_RST,
    FAN_I2C_MUX_RST,
    CPLD_25GPHY_RST,
    I210_DEV,
    LAN_PG,
    I210_RST,
    TOP_I2C_MUX_RST,
    RC32312_1_RST,
    RC32312_2_RST,
    RC32312_3_RST,

    CPU_PRSNT,
    BMC_PRSNT,
    NTM_PRSNT,
    USB_PRSNT,
    EXT_PRSNT,
    TOP_PRSNT,

    PSU_0_PRSNT,
    PSU_1_PRSNT,
    PSU_0_ACIN,
    PSU_1_ACIN,
    PSU_0_PG,
    PSU_1_PG,

    CPU_PG,
    MB_CPU_PG,
    BMC_PG,
    TOP_PG,

    RGB_PHY_0_SERBOOT,
    RGB_PHY_1_SERBOOT,
    RGB_PHY_2_SERBOOT,
    RGB_PHY_3_SERBOOT,
    RGB_PHY_4_SERBOOT,
    RGB_PHY_5_SERBOOT,
    RGB_PHY_6_SERBOOT,
    RGB_PHY_7_SERBOOT,

    CPU_MUX_SEL,
    PSU_MUX_SEL,
    FPGA_QSPI_SEL,
    UART_MUX2_SEL,

    SYSTEM_LED_STATUS,
    FAN_LED_STATUS,
    PSU_0_LED_STATUS,
    PSU_1_LED_STATUS,
    SYNC_LED_STATUS,
    SFP_0_LED_STATUS,
    SFP_1_LED_STATUS,

    //CPLD 2
    QSFPDD_NIF_P20_INTR,
    QSFPDD_NIF_P21_INTR,
    QSFPDD_NIF_P22_INTR,
    QSFPDD_NIF_P23_INTR,
    QSFPDD_NIF_P24_INTR,
    QSFPDD_NIF_P25_INTR,
    QSFPDD_NIF_P26_INTR,
    QSFPDD_NIF_P27_INTR,
    QSFPDD_NIF_P28_INTR,
    QSFPDD_NIF_P29_INTR,
    QSFPDD_NIF_P30_INTR,
    QSFPDD_NIF_P31_INTR,
    QSFPDD_NIF_P32_INTR,
    QSFPDD_NIF_P33_INTR,
    QSFPDD_NIF_P34_INTR,
    QSFPDD_NIF_P35_INTR,
    QSFPDD_NIF_P20_ABS,
    QSFPDD_NIF_P21_ABS,
    QSFPDD_NIF_P22_ABS,
    QSFPDD_NIF_P23_ABS,
    QSFPDD_NIF_P24_ABS,
    QSFPDD_NIF_P25_ABS,
    QSFPDD_NIF_P26_ABS,
    QSFPDD_NIF_P27_ABS,
    QSFPDD_NIF_P28_ABS,
    QSFPDD_NIF_P29_ABS,
    QSFPDD_NIF_P30_ABS,
    QSFPDD_NIF_P31_ABS,
    QSFPDD_NIF_P32_ABS,
    QSFPDD_NIF_P33_ABS,
    QSFPDD_NIF_P34_ABS,
    QSFPDD_NIF_P35_ABS,
    QSFPDD_NIF_P20_FUSE_INTR,
    QSFPDD_NIF_P21_FUSE_INTR,
    QSFPDD_NIF_P22_FUSE_INTR,
    QSFPDD_NIF_P23_FUSE_INTR,
    QSFPDD_NIF_P24_FUSE_INTR,
    QSFPDD_NIF_P25_FUSE_INTR,
    QSFPDD_NIF_P26_FUSE_INTR,
    QSFPDD_NIF_P27_FUSE_INTR,
    QSFPDD_NIF_P28_FUSE_INTR,
    QSFPDD_NIF_P29_FUSE_INTR,
    QSFPDD_NIF_P30_FUSE_INTR,
    QSFPDD_NIF_P31_FUSE_INTR,
    QSFPDD_NIF_P32_FUSE_INTR,
    QSFPDD_NIF_P33_FUSE_INTR,
    QSFPDD_NIF_P34_FUSE_INTR,
    QSFPDD_NIF_P35_FUSE_INTR,

    QSFPDD_FAB_P10_INTR,
    QSFPDD_FAB_P11_INTR,
    QSFPDD_FAB_P12_INTR,
    QSFPDD_FAB_P13_INTR,
    QSFPDD_FAB_P14_INTR,
    QSFPDD_FAB_P15_INTR,
    QSFPDD_FAB_P16_INTR,
    QSFPDD_FAB_P17_INTR,
    QSFPDD_FAB_P18_INTR,
    QSFPDD_FAB_P19_INTR,
    QSFPDD_FAB_P10_ABS,
    QSFPDD_FAB_P11_ABS,
    QSFPDD_FAB_P12_ABS,
    QSFPDD_FAB_P13_ABS,
    QSFPDD_FAB_P14_ABS,
    QSFPDD_FAB_P15_ABS,
    QSFPDD_FAB_P16_ABS,
    QSFPDD_FAB_P17_ABS,
    QSFPDD_FAB_P18_ABS,
    QSFPDD_FAB_P19_ABS,
    QSFPDD_FAB_P10_FUSE_INTR,
    QSFPDD_FAB_P11_FUSE_INTR,
    QSFPDD_FAB_P12_FUSE_INTR,
    QSFPDD_FAB_P13_FUSE_INTR,
    QSFPDD_FAB_P14_FUSE_INTR,
    QSFPDD_FAB_P15_FUSE_INTR,
    QSFPDD_FAB_P16_FUSE_INTR,
    QSFPDD_FAB_P17_FUSE_INTR,
    QSFPDD_FAB_P18_FUSE_INTR,
    QSFPDD_FAB_P19_FUSE_INTR,
    QSFPDD_NIF_P20_INTR_MASK,
    QSFPDD_NIF_P21_INTR_MASK,
    QSFPDD_NIF_P22_INTR_MASK,
    QSFPDD_NIF_P23_INTR_MASK,
    QSFPDD_NIF_P24_INTR_MASK,
    QSFPDD_NIF_P25_INTR_MASK,
    QSFPDD_NIF_P26_INTR_MASK,
    QSFPDD_NIF_P27_INTR_MASK,
    QSFPDD_NIF_P28_INTR_MASK,
    QSFPDD_NIF_P29_INTR_MASK,
    QSFPDD_NIF_P30_INTR_MASK,
    QSFPDD_NIF_P31_INTR_MASK,
    QSFPDD_NIF_P32_INTR_MASK,
    QSFPDD_NIF_P33_INTR_MASK,
    QSFPDD_NIF_P34_INTR_MASK,
    QSFPDD_NIF_P35_INTR_MASK,

    QSFPDD_NIF_P20_ABS_MASK,
    QSFPDD_NIF_P21_ABS_MASK,
    QSFPDD_NIF_P22_ABS_MASK,
    QSFPDD_NIF_P23_ABS_MASK,
    QSFPDD_NIF_P24_ABS_MASK,
    QSFPDD_NIF_P25_ABS_MASK,
    QSFPDD_NIF_P26_ABS_MASK,
    QSFPDD_NIF_P27_ABS_MASK,
    QSFPDD_NIF_P28_ABS_MASK,
    QSFPDD_NIF_P29_ABS_MASK,
    QSFPDD_NIF_P30_ABS_MASK,
    QSFPDD_NIF_P31_ABS_MASK,
    QSFPDD_NIF_P32_ABS_MASK,
    QSFPDD_NIF_P33_ABS_MASK,
    QSFPDD_NIF_P34_ABS_MASK,
    QSFPDD_NIF_P35_ABS_MASK,

    QSFPDD_NIF_P20_FUSE_INTR_MASK,
    QSFPDD_NIF_P21_FUSE_INTR_MASK,
    QSFPDD_NIF_P22_FUSE_INTR_MASK,
    QSFPDD_NIF_P23_FUSE_INTR_MASK,
    QSFPDD_NIF_P24_FUSE_INTR_MASK,
    QSFPDD_NIF_P25_FUSE_INTR_MASK,
    QSFPDD_NIF_P26_FUSE_INTR_MASK,
    QSFPDD_NIF_P27_FUSE_INTR_MASK,
    QSFPDD_NIF_P28_FUSE_INTR_MASK,
    QSFPDD_NIF_P29_FUSE_INTR_MASK,
    QSFPDD_NIF_P30_FUSE_INTR_MASK,
    QSFPDD_NIF_P31_FUSE_INTR_MASK,
    QSFPDD_NIF_P32_FUSE_INTR_MASK,
    QSFPDD_NIF_P33_FUSE_INTR_MASK,
    QSFPDD_NIF_P34_FUSE_INTR_MASK,
    QSFPDD_NIF_P35_FUSE_INTR_MASK,
    QSFPDD_NIF_P20_27_ABS_EVENT,
    QSFPDD_NIF_P28_35_ABS_EVENT,
    QSFPDD_FAB_P10_17_ABS_EVENT,
    QSFPDD_FAB_P18_19_ABS_EVENT,
    QSFPDD_FAB_P10_INTR_MASK,
    QSFPDD_FAB_P11_INTR_MASK,
    QSFPDD_FAB_P12_INTR_MASK,
    QSFPDD_FAB_P13_INTR_MASK,
    QSFPDD_FAB_P14_INTR_MASK,
    QSFPDD_FAB_P15_INTR_MASK,
    QSFPDD_FAB_P16_INTR_MASK,
    QSFPDD_FAB_P17_INTR_MASK,
    QSFPDD_FAB_P18_INTR_MASK,
    QSFPDD_FAB_P19_INTR_MASK,
    QSFPDD_FAB_P10_ABS_MASK,
    QSFPDD_FAB_P11_ABS_MASK,
    QSFPDD_FAB_P12_ABS_MASK,
    QSFPDD_FAB_P13_ABS_MASK,
    QSFPDD_FAB_P14_ABS_MASK,
    QSFPDD_FAB_P15_ABS_MASK,
    QSFPDD_FAB_P16_ABS_MASK,
    QSFPDD_FAB_P17_ABS_MASK,
    QSFPDD_FAB_P18_ABS_MASK,
    QSFPDD_FAB_P19_ABS_MASK,
    QSFPDD_FAB_P10_FUSE_INTR_MASK,
    QSFPDD_FAB_P11_FUSE_INTR_MASK,
    QSFPDD_FAB_P12_FUSE_INTR_MASK,
    QSFPDD_FAB_P13_FUSE_INTR_MASK,
    QSFPDD_FAB_P14_FUSE_INTR_MASK,
    QSFPDD_FAB_P15_FUSE_INTR_MASK,
    QSFPDD_FAB_P16_FUSE_INTR_MASK,
    QSFPDD_FAB_P17_FUSE_INTR_MASK,
    QSFPDD_FAB_P18_FUSE_INTR_MASK,
    QSFPDD_FAB_P19_FUSE_INTR_MASK,

    QSFPDD_NIF_P20_RST,
    QSFPDD_NIF_P21_RST,
    QSFPDD_NIF_P22_RST,
    QSFPDD_NIF_P23_RST,
    QSFPDD_NIF_P24_RST,
    QSFPDD_NIF_P25_RST,
    QSFPDD_NIF_P26_RST,
    QSFPDD_NIF_P27_RST,
    QSFPDD_NIF_P28_RST,
    QSFPDD_NIF_P29_RST,
    QSFPDD_NIF_P30_RST,
    QSFPDD_NIF_P31_RST,
    QSFPDD_NIF_P32_RST,
    QSFPDD_NIF_P33_RST,
    QSFPDD_NIF_P34_RST,
    QSFPDD_NIF_P35_RST,
    QSFPDD_FAB_P10_RST,
    QSFPDD_FAB_P11_RST,
    QSFPDD_FAB_P12_RST,
    QSFPDD_FAB_P13_RST,
    QSFPDD_FAB_P14_RST,
    QSFPDD_FAB_P15_RST,
    QSFPDD_FAB_P16_RST,
    QSFPDD_FAB_P17_RST,
    QSFPDD_FAB_P18_RST,
    QSFPDD_FAB_P19_RST,

    QSFPDD_NIF_P20_LP_MODE,
    QSFPDD_NIF_P21_LP_MODE,
    QSFPDD_NIF_P22_LP_MODE,
    QSFPDD_NIF_P23_LP_MODE,
    QSFPDD_NIF_P24_LP_MODE,
    QSFPDD_NIF_P25_LP_MODE,
    QSFPDD_NIF_P26_LP_MODE,
    QSFPDD_NIF_P27_LP_MODE,
    QSFPDD_NIF_P28_LP_MODE,
    QSFPDD_NIF_P29_LP_MODE,
    QSFPDD_NIF_P30_LP_MODE,
    QSFPDD_NIF_P31_LP_MODE,
    QSFPDD_NIF_P32_LP_MODE,
    QSFPDD_NIF_P33_LP_MODE,
    QSFPDD_NIF_P34_LP_MODE,
    QSFPDD_NIF_P35_LP_MODE,

    QSFPDD_FAB_P10_LP_MODE,
    QSFPDD_FAB_P11_LP_MODE,
    QSFPDD_FAB_P12_LP_MODE,
    QSFPDD_FAB_P13_LP_MODE,
    QSFPDD_FAB_P14_LP_MODE,
    QSFPDD_FAB_P15_LP_MODE,
    QSFPDD_FAB_P16_LP_MODE,
    QSFPDD_FAB_P17_LP_MODE,
    QSFPDD_FAB_P18_LP_MODE,
    QSFPDD_FAB_P19_LP_MODE,

    QSFPDD_FAB_P10_LED_STATUS,    
    QSFPDD_FAB_P11_LED_STATUS,
    QSFPDD_FAB_P12_LED_STATUS,
    QSFPDD_FAB_P13_LED_STATUS,
    QSFPDD_FAB_P14_LED_STATUS,
    QSFPDD_FAB_P15_LED_STATUS,
    QSFPDD_FAB_P16_LED_STATUS,
    QSFPDD_FAB_P17_LED_STATUS,
    QSFPDD_FAB_P18_LED_STATUS,
    QSFPDD_FAB_P19_LED_STATUS,

    QSFPDD_FAB_P10_I2C_STUCK,    
    QSFPDD_FAB_P11_I2C_STUCK,
    QSFPDD_FAB_P12_I2C_STUCK,
    QSFPDD_FAB_P13_I2C_STUCK,
    QSFPDD_FAB_P14_I2C_STUCK,
    QSFPDD_FAB_P15_I2C_STUCK,
    QSFPDD_FAB_P16_I2C_STUCK,
    QSFPDD_FAB_P17_I2C_STUCK,
    QSFPDD_FAB_P18_I2C_STUCK,
    QSFPDD_FAB_P19_I2C_STUCK,
    QSFPDD_NIF_P20_I2C_STUCK,
    QSFPDD_NIF_P21_I2C_STUCK,
    QSFPDD_NIF_P22_I2C_STUCK,
    QSFPDD_NIF_P23_I2C_STUCK,
    QSFPDD_NIF_P24_I2C_STUCK,
    QSFPDD_NIF_P25_I2C_STUCK,
    QSFPDD_NIF_P26_I2C_STUCK,
    QSFPDD_NIF_P27_I2C_STUCK,
    QSFPDD_NIF_P28_I2C_STUCK,
    QSFPDD_NIF_P29_I2C_STUCK,
    QSFPDD_NIF_P30_I2C_STUCK,
    QSFPDD_NIF_P31_I2C_STUCK,
    QSFPDD_NIF_P32_I2C_STUCK,
    QSFPDD_NIF_P33_I2C_STUCK,
    QSFPDD_NIF_P34_I2C_STUCK,
    QSFPDD_NIF_P35_I2C_STUCK,


    // FPGA Channel Select _ CPLD2
    FPGA_CPLD2_CH_DIS,
    FPGA_QSFPDD_FAB_10_CH,
    FPGA_QSFPDD_FAB_11_CH,
    FPGA_QSFPDD_FAB_12_CH,
    FPGA_QSFPDD_FAB_13_CH,
    FPGA_QSFPDD_FAB_14_CH,
    FPGA_QSFPDD_FAB_15_CH,
    FPGA_QSFPDD_FAB_16_CH,
    FPGA_QSFPDD_FAB_17_CH,
    FPGA_QSFPDD_FAB_18_CH,
    FPGA_QSFPDD_FAB_19_CH,



    //CPLD 3
    //Change to FPGA
    FPGA_ID,
    FPGA_VER_1,
    FPGA_MINOR_VER,
    FPGA_MAJOR_VER,
    FPGA_BUILD_VER,
    FPGA_VERSION_H,
    FPGA_DEV_INFO,
    SFP28_P37_TS,
    SFP28_P36_TS,
    MGMT_P1_TS,
    MGMT_P0_TS,
    SFP28_P37_RS,
    SFP28_P36_RS,
    MGMT_P1_RS,
    MGMT_P0_RS,
    SFP28_P37_TX_DIS,
    SFP28_P36_TX_DIS,
    MGMT_P1_TX_DIS,
    MGMT_P0_TX_DIS,
    SFP28_P37_TX_FLT,
    SFP28_P36_TX_FLT,
    MGMT_P1_TX_FLT,
    MGMT_P0_TX_FLT,
    SFP28_P37_RX_LOS,
    SFP28_P36_RX_LOS,
    MGMT_P1_RX_LOS,
    MGMT_P0_RX_LOS,
    SFP28_P37_ABS,
    SFP28_P36_ABS,
    MGMT_P1_ABS,
    MGMT_P0_ABS,
    SFP28_P37_TX_FLT_MASK,
    SFP28_P36_TX_FLT_MASK,
    MGMT_P1_TX_FLT_MASK,
    MGMT_P0_TX_FLT_MASK,
    SFP28_P37_RX_LOS_MASK,
    SFP28_P36_RX_LOS_MASK,
    MGMT_P1_RX_LOS_MASK,
    MGMT_P0_RX_LOS_MASK,
    SFP28_P37_ABS_MASK,
    SFP28_P36_ABS_MASK,
    MGMT_P1_ABS_MASK,
    MGMT_P0_ABS_MASK,
    SFP28_P37_I2C_STUCK,
    SFP28_P36_I2C_STUCK,
    MGMT_P1_I2C_STUCK,
    MGMT_P0_I2C_STUCK,
    SFP28_TX_FLT_EVENT,
    SFP28_RX_LOS_EVENT,
    SFP28_ABS_EVENT,
    

    //CPLD 4
    QSFPDD_NIF_P0_INTR,
    QSFPDD_NIF_P1_INTR,
    QSFPDD_NIF_P2_INTR,
    QSFPDD_NIF_P3_INTR,
    QSFPDD_NIF_P4_INTR,
    QSFPDD_NIF_P5_INTR,
    QSFPDD_NIF_P6_INTR,
    QSFPDD_NIF_P7_INTR,
    QSFPDD_NIF_P8_INTR,
    QSFPDD_NIF_P9_INTR,
    QSFPDD_NIF_P10_INTR,
    QSFPDD_NIF_P11_INTR,
    QSFPDD_NIF_P12_INTR,
    QSFPDD_NIF_P13_INTR,
    QSFPDD_NIF_P14_INTR,
    QSFPDD_NIF_P15_INTR,
    QSFPDD_NIF_P16_INTR,
    QSFPDD_NIF_P17_INTR,
    QSFPDD_NIF_P18_INTR,
    QSFPDD_NIF_P19_INTR,
    QSFPDD_NIF_P0_ABS,
    QSFPDD_NIF_P1_ABS,
    QSFPDD_NIF_P2_ABS,
    QSFPDD_NIF_P3_ABS,
    QSFPDD_NIF_P4_ABS,
    QSFPDD_NIF_P5_ABS,
    QSFPDD_NIF_P6_ABS,
    QSFPDD_NIF_P7_ABS,
    QSFPDD_NIF_P8_ABS,
    QSFPDD_NIF_P9_ABS,
    QSFPDD_NIF_P10_ABS,
    QSFPDD_NIF_P11_ABS,
    QSFPDD_NIF_P12_ABS,
    QSFPDD_NIF_P13_ABS,
    QSFPDD_NIF_P14_ABS,
    QSFPDD_NIF_P15_ABS,
    QSFPDD_NIF_P16_ABS,
    QSFPDD_NIF_P17_ABS,
    QSFPDD_NIF_P18_ABS,
    QSFPDD_NIF_P19_ABS,
    QSFPDD_NIF_P0_FUSE_INTR,
    QSFPDD_NIF_P1_FUSE_INTR,
    QSFPDD_NIF_P2_FUSE_INTR,
    QSFPDD_NIF_P3_FUSE_INTR,
    QSFPDD_NIF_P4_FUSE_INTR,
    QSFPDD_NIF_P5_FUSE_INTR,
    QSFPDD_NIF_P6_FUSE_INTR,
    QSFPDD_NIF_P7_FUSE_INTR,
    QSFPDD_NIF_P8_FUSE_INTR,
    QSFPDD_NIF_P9_FUSE_INTR,
    QSFPDD_NIF_P10_FUSE_INTR,
    QSFPDD_NIF_P11_FUSE_INTR,
    QSFPDD_NIF_P12_FUSE_INTR,
    QSFPDD_NIF_P13_FUSE_INTR,
    QSFPDD_NIF_P14_FUSE_INTR,
    QSFPDD_NIF_P15_FUSE_INTR,
    QSFPDD_NIF_P16_FUSE_INTR,
    QSFPDD_NIF_P17_FUSE_INTR,
    QSFPDD_NIF_P18_FUSE_INTR,
    QSFPDD_NIF_P19_FUSE_INTR,

    QSFPDD_FAB_P0_INTR,
    QSFPDD_FAB_P1_INTR,
    QSFPDD_FAB_P2_INTR,
    QSFPDD_FAB_P3_INTR,
    QSFPDD_FAB_P4_INTR,
    QSFPDD_FAB_P5_INTR,
    QSFPDD_FAB_P6_INTR,
    QSFPDD_FAB_P7_INTR,
    QSFPDD_FAB_P8_INTR,
    QSFPDD_FAB_P9_INTR,
    QSFPDD_FAB_P0_ABS,
    QSFPDD_FAB_P1_ABS,
    QSFPDD_FAB_P2_ABS,
    QSFPDD_FAB_P3_ABS,
    QSFPDD_FAB_P4_ABS,
    QSFPDD_FAB_P5_ABS,
    QSFPDD_FAB_P6_ABS,
    QSFPDD_FAB_P7_ABS,
    QSFPDD_FAB_P8_ABS,
    QSFPDD_FAB_P9_ABS,
    QSFPDD_FAB_P0_FUSE_INTR,
    QSFPDD_FAB_P1_FUSE_INTR,
    QSFPDD_FAB_P2_FUSE_INTR,
    QSFPDD_FAB_P3_FUSE_INTR,
    QSFPDD_FAB_P4_FUSE_INTR,
    QSFPDD_FAB_P5_FUSE_INTR,
    QSFPDD_FAB_P6_FUSE_INTR,
    QSFPDD_FAB_P7_FUSE_INTR,
    QSFPDD_FAB_P8_FUSE_INTR,
    QSFPDD_FAB_P9_FUSE_INTR,

    QSFPDD_NIF_P0_7_ABS_EVENT,
    QSFPDD_NIF_P8_15_ABS_EVENT,
    QSFPDD_NIF_P16_19_ABS_EVENT,

    QSFPDD_FAB_P0_7_ABS_EVENT,
    QSFPDD_FAB_P8_9_ABS_EVENT,

    QSFPDD_NIF_P0_INTR_MASK,
    QSFPDD_NIF_P1_INTR_MASK,
    QSFPDD_NIF_P2_INTR_MASK,
    QSFPDD_NIF_P3_INTR_MASK,
    QSFPDD_NIF_P4_INTR_MASK,
    QSFPDD_NIF_P5_INTR_MASK,
    QSFPDD_NIF_P6_INTR_MASK,
    QSFPDD_NIF_P7_INTR_MASK,
    QSFPDD_NIF_P8_INTR_MASK,
    QSFPDD_NIF_P9_INTR_MASK,
    QSFPDD_NIF_P10_INTR_MASK,
    QSFPDD_NIF_P11_INTR_MASK,
    QSFPDD_NIF_P12_INTR_MASK,
    QSFPDD_NIF_P13_INTR_MASK,
    QSFPDD_NIF_P14_INTR_MASK,
    QSFPDD_NIF_P15_INTR_MASK,
    QSFPDD_NIF_P16_INTR_MASK,
    QSFPDD_NIF_P17_INTR_MASK,
    QSFPDD_NIF_P18_INTR_MASK,
    QSFPDD_NIF_P19_INTR_MASK,
    QSFPDD_NIF_P0_ABS_MASK,
    QSFPDD_NIF_P1_ABS_MASK,
    QSFPDD_NIF_P2_ABS_MASK,
    QSFPDD_NIF_P3_ABS_MASK,
    QSFPDD_NIF_P4_ABS_MASK,
    QSFPDD_NIF_P5_ABS_MASK,
    QSFPDD_NIF_P6_ABS_MASK,
    QSFPDD_NIF_P7_ABS_MASK,
    QSFPDD_NIF_P8_ABS_MASK,
    QSFPDD_NIF_P9_ABS_MASK,
    QSFPDD_NIF_P10_ABS_MASK,
    QSFPDD_NIF_P11_ABS_MASK,
    QSFPDD_NIF_P12_ABS_MASK,
    QSFPDD_NIF_P13_ABS_MASK,
    QSFPDD_NIF_P14_ABS_MASK,
    QSFPDD_NIF_P15_ABS_MASK,
    QSFPDD_NIF_P16_ABS_MASK,
    QSFPDD_NIF_P17_ABS_MASK,
    QSFPDD_NIF_P18_ABS_MASK,
    QSFPDD_NIF_P19_ABS_MASK,
    QSFPDD_NIF_P0_FUSE_INTR_MASK,
    QSFPDD_NIF_P1_FUSE_INTR_MASK,
    QSFPDD_NIF_P2_FUSE_INTR_MASK,
    QSFPDD_NIF_P3_FUSE_INTR_MASK,
    QSFPDD_NIF_P4_FUSE_INTR_MASK,
    QSFPDD_NIF_P5_FUSE_INTR_MASK,
    QSFPDD_NIF_P6_FUSE_INTR_MASK,
    QSFPDD_NIF_P7_FUSE_INTR_MASK,
    QSFPDD_NIF_P8_FUSE_INTR_MASK,
    QSFPDD_NIF_P9_FUSE_INTR_MASK,
    QSFPDD_NIF_P10_FUSE_INTR_MASK,
    QSFPDD_NIF_P11_FUSE_INTR_MASK,
    QSFPDD_NIF_P12_FUSE_INTR_MASK,
    QSFPDD_NIF_P13_FUSE_INTR_MASK,
    QSFPDD_NIF_P14_FUSE_INTR_MASK,
    QSFPDD_NIF_P15_FUSE_INTR_MASK,
    QSFPDD_NIF_P16_FUSE_INTR_MASK,
    QSFPDD_NIF_P17_FUSE_INTR_MASK,
    QSFPDD_NIF_P18_FUSE_INTR_MASK,
    QSFPDD_NIF_P19_FUSE_INTR_MASK,

    QSFPDD_FAB_P0_INTR_MASK,
    QSFPDD_FAB_P1_INTR_MASK,
    QSFPDD_FAB_P2_INTR_MASK,
    QSFPDD_FAB_P3_INTR_MASK,
    QSFPDD_FAB_P4_INTR_MASK,
    QSFPDD_FAB_P5_INTR_MASK,
    QSFPDD_FAB_P6_INTR_MASK,
    QSFPDD_FAB_P7_INTR_MASK,
    QSFPDD_FAB_P8_INTR_MASK,
    QSFPDD_FAB_P9_INTR_MASK,
    QSFPDD_FAB_P0_ABS_MASK,
    QSFPDD_FAB_P1_ABS_MASK,
    QSFPDD_FAB_P2_ABS_MASK,
    QSFPDD_FAB_P3_ABS_MASK,
    QSFPDD_FAB_P4_ABS_MASK,
    QSFPDD_FAB_P5_ABS_MASK,
    QSFPDD_FAB_P6_ABS_MASK,
    QSFPDD_FAB_P7_ABS_MASK,
    QSFPDD_FAB_P8_ABS_MASK,
    QSFPDD_FAB_P9_ABS_MASK,
    QSFPDD_FAB_P0_FUSE_INTR_MASK,
    QSFPDD_FAB_P1_FUSE_INTR_MASK,
    QSFPDD_FAB_P2_FUSE_INTR_MASK,
    QSFPDD_FAB_P3_FUSE_INTR_MASK,
    QSFPDD_FAB_P4_FUSE_INTR_MASK,
    QSFPDD_FAB_P5_FUSE_INTR_MASK,
    QSFPDD_FAB_P6_FUSE_INTR_MASK,
    QSFPDD_FAB_P7_FUSE_INTR_MASK,
    QSFPDD_FAB_P8_FUSE_INTR_MASK,
    QSFPDD_FAB_P9_FUSE_INTR_MASK,

    QSFPDD_NIF_P0_RST,
    QSFPDD_NIF_P1_RST,
    QSFPDD_NIF_P2_RST,
    QSFPDD_NIF_P3_RST,
    QSFPDD_NIF_P4_RST,
    QSFPDD_NIF_P5_RST,
    QSFPDD_NIF_P6_RST,
    QSFPDD_NIF_P7_RST,
    QSFPDD_NIF_P8_RST,
    QSFPDD_NIF_P9_RST,
    QSFPDD_NIF_P10_RST,
    QSFPDD_NIF_P11_RST,
    QSFPDD_NIF_P12_RST,
    QSFPDD_NIF_P13_RST,
    QSFPDD_NIF_P14_RST,
    QSFPDD_NIF_P15_RST,
    QSFPDD_NIF_P16_RST,
    QSFPDD_NIF_P17_RST,
    QSFPDD_NIF_P18_RST,
    QSFPDD_NIF_P19_RST,
    QSFPDD_FAB_P0_RST,
    QSFPDD_FAB_P1_RST,
    QSFPDD_FAB_P2_RST,
    QSFPDD_FAB_P3_RST,
    QSFPDD_FAB_P4_RST,
    QSFPDD_FAB_P5_RST,
    QSFPDD_FAB_P6_RST,
    QSFPDD_FAB_P7_RST,
    QSFPDD_FAB_P8_RST,
    QSFPDD_FAB_P9_RST,

    QSFPDD_NIF_P0_LP_MODE,
    QSFPDD_NIF_P1_LP_MODE,
    QSFPDD_NIF_P2_LP_MODE,
    QSFPDD_NIF_P3_LP_MODE,
    QSFPDD_NIF_P4_LP_MODE,
    QSFPDD_NIF_P5_LP_MODE,
    QSFPDD_NIF_P6_LP_MODE,
    QSFPDD_NIF_P7_LP_MODE,
    QSFPDD_NIF_P8_LP_MODE,
    QSFPDD_NIF_P9_LP_MODE,
    QSFPDD_NIF_P10_LP_MODE,
    QSFPDD_NIF_P11_LP_MODE,
    QSFPDD_NIF_P12_LP_MODE,
    QSFPDD_NIF_P13_LP_MODE,
    QSFPDD_NIF_P14_LP_MODE,
    QSFPDD_NIF_P15_LP_MODE,
    QSFPDD_NIF_P16_LP_MODE,
    QSFPDD_NIF_P17_LP_MODE,
    QSFPDD_NIF_P18_LP_MODE,
    QSFPDD_NIF_P19_LP_MODE,

    QSFPDD_FAB_P0_LP_MODE,
    QSFPDD_FAB_P1_LP_MODE,
    QSFPDD_FAB_P2_LP_MODE,
    QSFPDD_FAB_P3_LP_MODE,
    QSFPDD_FAB_P4_LP_MODE,
    QSFPDD_FAB_P5_LP_MODE,
    QSFPDD_FAB_P6_LP_MODE,
    QSFPDD_FAB_P7_LP_MODE,
    QSFPDD_FAB_P8_LP_MODE,
    QSFPDD_FAB_P9_LP_MODE,

    QSFPDD_FAB_P0_LED_STATUS,    
    QSFPDD_FAB_P1_LED_STATUS,
    QSFPDD_FAB_P2_LED_STATUS,
    QSFPDD_FAB_P3_LED_STATUS,
    QSFPDD_FAB_P4_LED_STATUS,
    QSFPDD_FAB_P5_LED_STATUS,
    QSFPDD_FAB_P6_LED_STATUS,
    QSFPDD_FAB_P7_LED_STATUS,
    QSFPDD_FAB_P8_LED_STATUS,
    QSFPDD_FAB_P9_LED_STATUS,

    QSFPDD_NIF_P0_I2C_STUCK,
    QSFPDD_NIF_P1_I2C_STUCK,
    QSFPDD_NIF_P2_I2C_STUCK,
    QSFPDD_NIF_P3_I2C_STUCK,
    QSFPDD_NIF_P4_I2C_STUCK,
    QSFPDD_NIF_P5_I2C_STUCK,
    QSFPDD_NIF_P6_I2C_STUCK,
    QSFPDD_NIF_P7_I2C_STUCK,
    QSFPDD_NIF_P8_I2C_STUCK,
    QSFPDD_NIF_P9_I2C_STUCK,
    QSFPDD_NIF_P10_I2C_STUCK,
    QSFPDD_NIF_P11_I2C_STUCK,
    QSFPDD_NIF_P12_I2C_STUCK,
    QSFPDD_NIF_P13_I2C_STUCK,
    QSFPDD_NIF_P14_I2C_STUCK,
    QSFPDD_NIF_P15_I2C_STUCK,
    QSFPDD_NIF_P16_I2C_STUCK,
    QSFPDD_NIF_P17_I2C_STUCK,
    QSFPDD_NIF_P18_I2C_STUCK,
    QSFPDD_NIF_P19_I2C_STUCK,
    QSFPDD_FAB_P0_I2C_STUCK,
    QSFPDD_FAB_P1_I2C_STUCK,
    QSFPDD_FAB_P2_I2C_STUCK,
    QSFPDD_FAB_P3_I2C_STUCK,
    QSFPDD_FAB_P4_I2C_STUCK,
    QSFPDD_FAB_P5_I2C_STUCK,
    QSFPDD_FAB_P6_I2C_STUCK,
    QSFPDD_FAB_P7_I2C_STUCK,
    QSFPDD_FAB_P8_I2C_STUCK,
    QSFPDD_FAB_P9_I2C_STUCK,

    // FPGA Channel Select _ CPLD4
    FPGA_CPLD4_CH_DIS,
    FPGA_QSFPDD_NIF_0_CH,
    FPGA_QSFPDD_NIF_1_CH,
    FPGA_QSFPDD_NIF_2_CH,
    FPGA_QSFPDD_NIF_3_CH,
    FPGA_QSFPDD_NIF_4_CH,
    FPGA_QSFPDD_NIF_5_CH,
    FPGA_QSFPDD_NIF_6_CH,
    FPGA_QSFPDD_NIF_7_CH,
    FPGA_QSFPDD_NIF_8_CH,
    FPGA_QSFPDD_NIF_9_CH,
    FPGA_QSFPDD_NIF_10_CH,
    FPGA_QSFPDD_NIF_11_CH,
    FPGA_QSFPDD_NIF_12_CH,
    FPGA_QSFPDD_NIF_13_CH,
    FPGA_QSFPDD_NIF_14_CH,
    FPGA_QSFPDD_NIF_15_CH,
    FPGA_QSFPDD_NIF_16_CH,
    FPGA_QSFPDD_NIF_17_CH,
    FPGA_QSFPDD_NIF_18_CH,
    FPGA_QSFPDD_NIF_19_CH,
    FPGA_QSFPDD_NIF_20_CH,
    FPGA_QSFPDD_NIF_21_CH,
    FPGA_QSFPDD_NIF_22_CH,
    FPGA_QSFPDD_NIF_23_CH,
    FPGA_QSFPDD_NIF_24_CH,
    FPGA_QSFPDD_NIF_25_CH,
    FPGA_QSFPDD_NIF_26_CH,
    FPGA_QSFPDD_NIF_27_CH,
    FPGA_QSFPDD_NIF_28_CH,
    FPGA_QSFPDD_NIF_29_CH,
    FPGA_QSFPDD_NIF_30_CH,
    FPGA_QSFPDD_NIF_31_CH,
    FPGA_QSFPDD_NIF_32_CH,
    FPGA_QSFPDD_NIF_33_CH,
    FPGA_QSFPDD_NIF_34_CH,
    FPGA_QSFPDD_NIF_35_CH,

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

typedef struct  {
    u16 reg;
    u8 mask;
    u8 data_type;
} attr_reg_map_t;

static attr_reg_map_t attr_reg[]= {

    // CPLD Common
    [CPLD_MINOR_VER]                       =         {CPLD_VERSION_REG                        , MASK_0011_1111, DATA_DEC},
    [CPLD_MAJOR_VER]                       =         {CPLD_VERSION_REG                        , MASK_1100_0000, DATA_DEC},
    [CPLD_ID]                              =         {CPLD_ID_REG                             , MASK_0000_0111, DATA_DEC},
    [CPLD_BUILD_VER]                       =         {CPLD_SUB_VERSION_REG                    , MASK_ALL      , DATA_DEC},
    [CPLD_VERSION_H]                       =         {NONE_REG                                , MASK_NONE     , DATA_UNK},

    //CPLD 1
    [CPLD_SKU_ID]                          =         {CPLD_SKU_ID_REG                         , MASK_0011_1111, DATA_DEC},
    [CPLD_HW_REV]                          =         {CPLD_HW_BUILD_REV_REG                   , MASK_0000_0011, DATA_DEC},
    [CPLD_DEPH_REV]                        =         {CPLD_HW_BUILD_REV_REG                   , MASK_0000_0100, DATA_DEC},
    [CPLD_BUILD_REV]                       =         {CPLD_HW_BUILD_REV_REG                   , MASK_0011_1000, DATA_DEC},
    [CPLD_BRD_ID_TYPE]                     =         {CPLD_HW_BUILD_REV_REG                   , MASK_1000_0000, DATA_DEC},
    [CPLD_CHIP_TYPE]                       =         {CPLD_CHIP_TYPE_REG                      , MASK_0000_0011, DATA_DEC},
    [MAC_INTR]                             =         {MAC_INTR_REG                            , MASK_0000_0001, DATA_DEC},
    [RGB_PHY_0_INTR]                       =         {MB_RGB_INTR_REG                         , MASK_0000_0001, DATA_HEX},
    [RGB_PHY_1_INTR]                       =         {MB_RGB_INTR_REG                         , MASK_0000_0010, DATA_HEX},
    [RGB_PHY_2_INTR]                       =         {MB_RGB_INTR_REG                         , MASK_0000_0100, DATA_HEX},
    [RGB_PHY_3_INTR]                       =         {MB_RGB_INTR_REG                         , MASK_0000_1000, DATA_HEX},
    [RGB_PHY_4_INTR]                       =         {MB_RGB_INTR_REG                         , MASK_0001_0000, DATA_HEX},
    [RGB_PHY_5_INTR]                       =         {MB_RGB_INTR_REG                         , MASK_0010_0000, DATA_HEX},
    [RGB_PHY_6_INTR]                       =         {MB_RGB_INTR_REG                         , MASK_0100_0000, DATA_HEX},
    [RGB_PHY_7_INTR]                       =         {MB_RGB_INTR_REG                         , MASK_1000_0000, DATA_HEX},
    [TOP_RGB_INTR]                         =         {TOP_RGB_INTR_REG                        , MASK_0000_0001, DATA_HEX},
    [CPLD_25GPHY_INTR]                     =         {CPLD_25GPHY_INTR_REG                    , MASK_0100_0000, DATA_HEX},
    [CPLD_2_FRU_INTR]                      =         {CPLD_FRU_INTR_REG                       , MASK_0000_0001, DATA_HEX},
    [CPLD_4_FRU_INTR]                      =         {CPLD_FRU_INTR_REG                       , MASK_0000_0100, DATA_HEX},
    [SFP28_INTR]                           =         {CPLD_FRU_INTR_REG                       , MASK_0000_1000, DATA_HEX},
    [RETIMER_INTR]                         =         {CPLD_FRU_INTR_REG                       , MASK_0001_0000, DATA_HEX},
    [PSU_0_INTR]                           =         {CPLD_FRU_INTR_REG                       , MASK_0010_0000, DATA_HEX},
    [PSU_1_INTR]                           =         {CPLD_FRU_INTR_REG                       , MASK_0100_0000, DATA_HEX},

    [MAC_HBM_TEMP_ALERT]                   =         {THERMAL_ALERT_1_REG                     , MASK_0000_0001, DATA_HEX},
    [MB_LM75_TEMP_ALERT]                   =         {THERMAL_ALERT_2_REG                     , MASK_0011_1111, DATA_HEX},
    [EXT_LM75_TEMP_ALERT]                  =         {THERMAL_ALERT_2_REG                     , MASK_0100_0000, DATA_HEX},
    [PHY_LM75_CPLD5_INTR]                  =         {THERMAL_ALERT_2_REG                     , MASK_1000_0000, DATA_HEX},
    [USB_OCP]                              =         {MISC_INTR_REG                           , MASK_0000_0010, DATA_HEX},
    [FPGA_INTR]                            =         {MISC_INTR_REG                           , MASK_0000_1000, DATA_HEX},

    [LM75_BMC_INTR]                        =         {SYSTEM_INTR_REG                         , MASK_0000_0001, DATA_HEX},
    [LM75_CPLD1_INTR]                      =         {SYSTEM_INTR_REG                         , MASK_0000_0010, DATA_HEX},
    [CPU_INTR]                             =         {SYSTEM_INTR_REG                         , MASK_0000_0100, DATA_HEX},
    [LM75_CPLD245_INTR]                    =         {SYSTEM_INTR_REG                         , MASK_0000_1000, DATA_HEX},
    [MAC_INTR_MASK]                        =         {MAC_INTR_MASK_REG                       , MASK_0000_0001, DATA_HEX},
    [RGB_PHY_0_INTR_MASK]                  =         {MB_RGB_INTR_MASK_REG                    , MASK_0000_0001, DATA_HEX},
    [RGB_PHY_1_INTR_MASK]                  =         {MB_RGB_INTR_MASK_REG                    , MASK_0000_0010, DATA_HEX},
    [RGB_PHY_2_INTR_MASK]                  =         {MB_RGB_INTR_MASK_REG                    , MASK_0000_0100, DATA_HEX},
    [RGB_PHY_3_INTR_MASK]                  =         {MB_RGB_INTR_MASK_REG                    , MASK_0000_1000, DATA_HEX},
    [RGB_PHY_4_INTR_MASK]                  =         {MB_RGB_INTR_MASK_REG                    , MASK_0001_0000, DATA_HEX},
    [RGB_PHY_5_INTR_MASK]                  =         {MB_RGB_INTR_MASK_REG                    , MASK_0010_0000, DATA_HEX},
    [RGB_PHY_6_INTR_MASK]                  =         {MB_RGB_INTR_MASK_REG                    , MASK_0100_0000, DATA_HEX},
    [RGB_PHY_7_INTR_MASK]                  =         {MB_RGB_INTR_MASK_REG                    , MASK_1000_0000, DATA_HEX},
    [TOP_RGB_INTR_MASK]                    =         {TOP_RGB_INTR_MASK_REG                   , MASK_0000_0001, DATA_HEX},

    [CPLD_25GPHY_INTR_MASK]                =         {CPLD_25GPHY_INTR_MASK_REG               , MASK_0100_0000, DATA_HEX},
    [CPLD_2_FRU_INTR_MASK]                 =         {CPLD_FRU_INTR_MASK_REG                  , MASK_0000_0001, DATA_HEX},
    [CPLD_4_FRU_INTR_MASK]                 =         {CPLD_FRU_INTR_MASK_REG                  , MASK_0000_0100, DATA_HEX},
    [SFP28_INTR_MASK]                      =         {CPLD_FRU_INTR_MASK_REG                  , MASK_0000_1000, DATA_HEX},
    [RETIMER_INTR_MASK]                    =         {CPLD_FRU_INTR_MASK_REG                  , MASK_0001_0000, DATA_HEX},
    [PSU_0_INTR_MASK]                      =         {CPLD_FRU_INTR_MASK_REG                  , MASK_0100_0000, DATA_HEX},
    [PSU_1_INTR_MASK]                      =         {CPLD_FRU_INTR_MASK_REG                  , MASK_1000_0000, DATA_HEX},

    [MAC_TEMP_ALERT_MASK]                  =         {THERMAL_ALERT_1_MASK_REG                , MASK_0000_0001, DATA_HEX},
    [MB_TEMP_1_ALERT_MASK]                 =         {THERMAL_ALERT_2_MASK_REG                , MASK_0000_0001, DATA_HEX},
    [MB_TEMP_2_ALERT_MASK]                 =         {THERMAL_ALERT_2_MASK_REG                , MASK_0000_0010, DATA_HEX},
    [MB_TEMP_3_ALERT_MASK]                 =         {THERMAL_ALERT_2_MASK_REG                , MASK_0000_0100, DATA_HEX},
    [MB_TEMP_4_ALERT_MASK]                 =         {THERMAL_ALERT_2_MASK_REG                , MASK_0000_1000, DATA_HEX},
    [MB_TEMP_5_ALERT_MASK]                 =         {THERMAL_ALERT_2_MASK_REG                , MASK_0001_0000, DATA_HEX},
    [MB_TEMP_6_ALERT_MASK]                 =         {THERMAL_ALERT_2_MASK_REG                , MASK_0010_0000, DATA_HEX},
    [MB_TEMP_7_ALERT_MASK]                 =         {THERMAL_ALERT_2_MASK_REG                , MASK_0100_0000, DATA_HEX},
    [TOP_CPLD_5_INTR_MASK]                 =         {THERMAL_ALERT_2_MASK_REG                , MASK_1000_0000, DATA_HEX},
    [USB_OCP_MASK]                         =         {MISC_INTR_MASK_REG                      , MASK_0000_0010, DATA_HEX},
    [FPGA_INTR_MASK]                       =         {MISC_INTR_MASK_REG                      , MASK_0000_1000, DATA_HEX},
    [LM75_BMC_INTR_MASK]                   =         {SYSTEM_INTR_MASK_REG                    , MASK_0000_0001, DATA_HEX},
    [LM75_CPLD1_INTR_MASK]                 =         {SYSTEM_INTR_MASK_REG                    , MASK_0000_0010, DATA_HEX},
    [CPU_INTR_MASK]                        =         {SYSTEM_INTR_MASK_REG                    , MASK_0000_0100, DATA_HEX},
    [LM75_CPLD245_INTR_MASK]               =         {SYSTEM_INTR_MASK_REG                    , MASK_0000_1000, DATA_HEX},

    [MAC_INTR_EVENT]                       =         {MAC_INTR_EVENT_REG                      , MASK_0000_0001, DATA_HEX},
    [MB_RGB_0_7_INTR_EVENT]                =         {MB_RGB_INTR_EVENT_REG                   , MASK_ALL      , DATA_HEX},
    [TOP_RGB_INTR_EVENT]                   =         {TOP_RGB_INTR_EVENT_REG                  , MASK_0000_0001, DATA_HEX},

    [CPLD_25GPHY_INTR_EVENT]               =         {CPLD_25GPHY_INTR_EVENT_REG              , MASK_0100_0000, DATA_HEX},
    [CPLD_FRU_INTR_EVENT]                  =         {CPLD_FRU_INTR_EVENT_REG                 , MASK_ALL      , DATA_HEX},
    [MAC_TEMP_ALERT_EVENT]                 =         {THERMAL_ALERT_INTR_1_REG                , MASK_0000_0001, DATA_HEX},
    [TEMP_ALERT_EVENT]                     =         {THERMAL_ALERT_INTR_2_REG                , MASK_ALL      , DATA_HEX},
    [MISC_INTR_EVENT]                      =         {MISC_INTR_EVENT_REG                     , MASK_ALL      , DATA_HEX},

    [EVENT_DETECT_CTRL]                    =         {EVENT_DETECT_CTRL_REG                   , MASK_0000_0001, DATA_HEX},
    [MAC_PCIE_RST]                         =         {MAC_RST_REG                             , MASK_0000_0001, DATA_HEX},
    [MAC_QSPI_RST]                         =         {MAC_RST_REG                             , MASK_0000_0010, DATA_HEX},
    [RGB_PHY_0_RST]                        =         {MB_RGB_RST_REG                          , MASK_0000_0001, DATA_HEX},
    [RGB_PHY_1_RST]                        =         {MB_RGB_RST_REG                          , MASK_0000_0010, DATA_HEX},
    [RGB_PHY_2_RST]                        =         {MB_RGB_RST_REG                          , MASK_0000_0100, DATA_HEX},
    [RGB_PHY_3_RST]                        =         {MB_RGB_RST_REG                          , MASK_0000_1000, DATA_HEX},
    [RGB_PHY_4_RST]                        =         {MB_RGB_RST_REG                          , MASK_0001_0000, DATA_HEX},
    [RGB_PHY_5_RST]                        =         {MB_RGB_RST_REG                          , MASK_0010_0000, DATA_HEX},
    [RGB_PHY_6_RST]                        =         {MB_RGB_RST_REG                          , MASK_0100_0000, DATA_HEX},
    [RGB_PHY_7_RST]                        =         {MB_RGB_RST_REG                          , MASK_1000_0000, DATA_HEX},

    [NTM_RST]                              =         {BMC_NTM_RST_REG                         , MASK_0000_0010, DATA_HEX},
    [BMC_RST]                              =         {BMC_NTM_RST_REG                         , MASK_0000_0001, DATA_HEX},
    [USB_REDRIVER_RST]                     =         {USB_REDRIVER_RST_REG                    , MASK_0000_0010, DATA_HEX},
    [USB_OCP_RCVRY]                        =         {USB_OCP_RCVRY_REG                       , MASK_0000_0001, DATA_HEX},

    [CPLD_2_RST]                           =         {CPLD_RST_REG                            , MASK_0000_0001, DATA_HEX},
    [FPGA_RST]                             =         {CPLD_RST_REG                            , MASK_0000_0010, DATA_HEX},
    [CPLD_4_RST]                           =         {CPLD_RST_REG                            , MASK_0000_1000, DATA_HEX},
    [CPLD_5_RST]                           =         {CPLD_RST_REG                            , MASK_0001_0000, DATA_HEX},

    [CPLD_IOEXP_RST]                       =         {MUX_RST_REG                             , MASK_0000_0001, DATA_HEX},
    [FAN_I2C_MUX_RST]                      =         {MUX_RST_REG                             , MASK_0000_0010, DATA_HEX},
    [CPLD_25GPHY_RST]                      =         {MISC_RST_REG                            , MASK_0000_0001, DATA_HEX},
    [I210_DEV]                             =         {MISC_RST_REG                            , MASK_0000_0010, DATA_HEX},
    [LAN_PG]                               =         {MISC_RST_REG                            , MASK_0000_0100, DATA_HEX},
    [I210_RST]                             =         {MISC_RST_REG                            , MASK_0000_1000, DATA_HEX},
    [TOP_I2C_MUX_RST]                      =         {MISC_RST_REG                            , MASK_0001_0000, DATA_HEX},
    [RC32312_1_RST]                        =         {MISC_RST_REG                            , MASK_0010_0000, DATA_HEX},
    [RC32312_2_RST]                        =         {MISC_RST_REG                            , MASK_0100_0000, DATA_HEX},
    [RC32312_3_RST]                        =         {MISC_RST_REG                            , MASK_1000_0000, DATA_HEX},

    [CPU_PRSNT]                            =         {CPU_MISC_REG                            , MASK_0000_0001, DATA_HEX},
    [BMC_PRSNT]                            =         {CPU_MISC_REG                            , MASK_0000_0010, DATA_HEX},
    [NTM_PRSNT]                            =         {CPU_MISC_REG                            , MASK_0000_0100, DATA_HEX},
    [USB_PRSNT]                            =         {CPU_MISC_REG                            , MASK_0000_1000, DATA_HEX},
    [EXT_PRSNT]                            =         {CPU_MISC_REG                            , MASK_0001_0000, DATA_HEX},
    [TOP_PRSNT]                            =         {CPU_MISC_REG                            , MASK_0010_0000, DATA_HEX},

    [PSU_0_PRSNT]                          =         {PSU_STATUS_REG                          , MASK_0000_0001, DATA_HEX},
    [PSU_1_PRSNT]                          =         {PSU_STATUS_REG                          , MASK_0000_0010, DATA_HEX},
    [PSU_0_ACIN]                           =         {PSU_STATUS_REG                          , MASK_0000_0100, DATA_HEX},
    [PSU_1_ACIN]                           =         {PSU_STATUS_REG                          , MASK_0000_1000, DATA_HEX},
    [PSU_0_PG]                             =         {PSU_STATUS_REG                          , MASK_0001_0000, DATA_HEX},
    [PSU_1_PG]                             =         {PSU_STATUS_REG                          , MASK_0010_0000, DATA_HEX},

    [CPU_PG]                               =         {SYS_PW_STATUS_REG                       , MASK_0000_0001, DATA_HEX},
    [MB_CPU_PG]                            =         {SYS_PW_STATUS_REG                       , MASK_0000_0010, DATA_HEX},
    [BMC_PG]                               =         {SYS_PW_STATUS_REG                       , MASK_0000_0100, DATA_HEX},
    [TOP_PG]                               =         {SYS_PW_STATUS_REG                       , MASK_0000_1000, DATA_HEX},

    [RGB_PHY_0_SERBOOT]                    =         {MB_RGB_SERBOOT_REG                      , MASK_0000_0001, DATA_HEX},
    [RGB_PHY_1_SERBOOT]                    =         {MB_RGB_SERBOOT_REG                      , MASK_0000_0010, DATA_HEX},
    [RGB_PHY_2_SERBOOT]                    =         {MB_RGB_SERBOOT_REG                      , MASK_0000_0100, DATA_HEX},
    [RGB_PHY_3_SERBOOT]                    =         {MB_RGB_SERBOOT_REG                      , MASK_0000_1000, DATA_HEX},
    [RGB_PHY_4_SERBOOT]                    =         {MB_RGB_SERBOOT_REG                      , MASK_0001_0000, DATA_HEX},
    [RGB_PHY_5_SERBOOT]                    =         {MB_RGB_SERBOOT_REG                      , MASK_0010_0000, DATA_HEX},
    [RGB_PHY_6_SERBOOT]                    =         {MB_RGB_SERBOOT_REG                      , MASK_0100_0000, DATA_HEX},
    [RGB_PHY_7_SERBOOT]                    =         {MB_RGB_SERBOOT_REG                      , MASK_1000_0000, DATA_HEX},

    [CPU_MUX_SEL]                          =         {MUX_CTRL_REG                            , MASK_0000_0001, DATA_HEX},
    [PSU_MUX_SEL]                          =         {MUX_CTRL_REG                            , MASK_0000_0011, DATA_HEX},
    [FPGA_QSPI_SEL]                        =         {MUX_CTRL_REG                            , MASK_0010_0000, DATA_HEX},
    [UART_MUX2_SEL]                        =         {MUX_CTRL_REG                            , MASK_0100_0000, DATA_HEX},

    [SYSTEM_LED_STATUS]                    =         {SYSTEM_LED_CTRL_1_REG                   , MASK_0000_1111, DATA_HEX},
    [FAN_LED_STATUS]                       =         {SYSTEM_LED_CTRL_1_REG                   , MASK_1111_0000, DATA_HEX},
    [PSU_0_LED_STATUS]                     =         {SYSTEM_LED_CTRL_2_REG                   , MASK_0000_1111, DATA_HEX},
    [PSU_1_LED_STATUS]                     =         {SYSTEM_LED_CTRL_2_REG                   , MASK_1111_0000, DATA_HEX},
    [SYNC_LED_STATUS]                      =         {SYSTEM_LED_CTRL_3_REG                   , MASK_0000_1111, DATA_HEX},
    [SFP_0_LED_STATUS]                     =         {SFP28_LED_REG                           , MASK_0000_1111, DATA_HEX},
    [SFP_1_LED_STATUS]                     =         {SFP28_LED_REG                           , MASK_1111_0000, DATA_HEX},
 
    //CPLD 2
    [QSFPDD_NIF_P20_INTR]                  =         {QSFPDD_NIF_20_27_INTR_REG               , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P21_INTR]                  =         {QSFPDD_NIF_20_27_INTR_REG               , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P22_INTR]                  =         {QSFPDD_NIF_20_27_INTR_REG               , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P23_INTR]                  =         {QSFPDD_NIF_20_27_INTR_REG               , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P24_INTR]                  =         {QSFPDD_NIF_20_27_INTR_REG               , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P25_INTR]                  =         {QSFPDD_NIF_20_27_INTR_REG               , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P26_INTR]                  =         {QSFPDD_NIF_20_27_INTR_REG               , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P27_INTR]                  =         {QSFPDD_NIF_20_27_INTR_REG               , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P28_INTR]                  =         {QSFPDD_NIF_28_35_INTR_REG               , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P29_INTR]                  =         {QSFPDD_NIF_28_35_INTR_REG               , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P30_INTR]                  =         {QSFPDD_NIF_28_35_INTR_REG               , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P31_INTR]                  =         {QSFPDD_NIF_28_35_INTR_REG               , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P32_INTR]                  =         {QSFPDD_NIF_28_35_INTR_REG               , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P33_INTR]                  =         {QSFPDD_NIF_28_35_INTR_REG               , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P34_INTR]                  =         {QSFPDD_NIF_28_35_INTR_REG               , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P35_INTR]                  =         {QSFPDD_NIF_28_35_INTR_REG               , MASK_1000_0000, DATA_DEC},

    [QSFPDD_NIF_P20_ABS]                   =         {QSFPDD_NIF_20_27_ABS_REG                , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P21_ABS]                   =         {QSFPDD_NIF_20_27_ABS_REG                , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P22_ABS]                   =         {QSFPDD_NIF_20_27_ABS_REG                , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P23_ABS]                   =         {QSFPDD_NIF_20_27_ABS_REG                , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P24_ABS]                   =         {QSFPDD_NIF_20_27_ABS_REG                , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P25_ABS]                   =         {QSFPDD_NIF_20_27_ABS_REG                , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P26_ABS]                   =         {QSFPDD_NIF_20_27_ABS_REG                , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P27_ABS]                   =         {QSFPDD_NIF_20_27_ABS_REG                , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P28_ABS]                   =         {QSFPDD_NIF_28_35_ABS_REG                , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P29_ABS]                   =         {QSFPDD_NIF_28_35_ABS_REG                , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P30_ABS]                   =         {QSFPDD_NIF_28_35_ABS_REG                , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P31_ABS]                   =         {QSFPDD_NIF_28_35_ABS_REG                , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P32_ABS]                   =         {QSFPDD_NIF_28_35_ABS_REG                , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P33_ABS]                   =         {QSFPDD_NIF_28_35_ABS_REG                , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P34_ABS]                   =         {QSFPDD_NIF_28_35_ABS_REG                , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P35_ABS]                   =         {QSFPDD_NIF_28_35_ABS_REG                , MASK_1000_0000, DATA_DEC},

    [QSFPDD_NIF_P20_FUSE_INTR]             =         {QSFPDD_NIF_20_27_FUSE_INTR_REG          , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P21_FUSE_INTR]             =         {QSFPDD_NIF_20_27_FUSE_INTR_REG          , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P22_FUSE_INTR]             =         {QSFPDD_NIF_20_27_FUSE_INTR_REG          , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P23_FUSE_INTR]             =         {QSFPDD_NIF_20_27_FUSE_INTR_REG          , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P24_FUSE_INTR]             =         {QSFPDD_NIF_20_27_FUSE_INTR_REG          , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P25_FUSE_INTR]             =         {QSFPDD_NIF_20_27_FUSE_INTR_REG          , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P26_FUSE_INTR]             =         {QSFPDD_NIF_20_27_FUSE_INTR_REG          , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P27_FUSE_INTR]             =         {QSFPDD_NIF_20_27_FUSE_INTR_REG          , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P28_FUSE_INTR]             =         {QSFPDD_NIF_28_35_FUSE_INTR_REG          , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P29_FUSE_INTR]             =         {QSFPDD_NIF_28_35_FUSE_INTR_REG          , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P30_FUSE_INTR]             =         {QSFPDD_NIF_28_35_FUSE_INTR_REG          , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P31_FUSE_INTR]             =         {QSFPDD_NIF_28_35_FUSE_INTR_REG          , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P32_FUSE_INTR]             =         {QSFPDD_NIF_28_35_FUSE_INTR_REG          , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P33_FUSE_INTR]             =         {QSFPDD_NIF_28_35_FUSE_INTR_REG          , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P34_FUSE_INTR]             =         {QSFPDD_NIF_28_35_FUSE_INTR_REG          , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P35_FUSE_INTR]             =         {QSFPDD_NIF_28_35_FUSE_INTR_REG          , MASK_1000_0000, DATA_DEC},

    [QSFPDD_FAB_P10_INTR]                  =         {QSFPDD_FAB_10_17_INTR_REG               , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P11_INTR]                  =         {QSFPDD_FAB_10_17_INTR_REG               , MASK_0000_0010, DATA_DEC},
    [QSFPDD_FAB_P12_INTR]                  =         {QSFPDD_FAB_10_17_INTR_REG               , MASK_0000_0100, DATA_DEC},
    [QSFPDD_FAB_P13_INTR]                  =         {QSFPDD_FAB_10_17_INTR_REG               , MASK_0000_1000, DATA_DEC},
    [QSFPDD_FAB_P14_INTR]                  =         {QSFPDD_FAB_10_17_INTR_REG               , MASK_0001_0000, DATA_DEC},
    [QSFPDD_FAB_P15_INTR]                  =         {QSFPDD_FAB_10_17_INTR_REG               , MASK_0010_0000, DATA_DEC},
    [QSFPDD_FAB_P16_INTR]                  =         {QSFPDD_FAB_10_17_INTR_REG               , MASK_0100_0000, DATA_DEC},
    [QSFPDD_FAB_P17_INTR]                  =         {QSFPDD_FAB_10_17_INTR_REG               , MASK_1000_0000, DATA_DEC},
    [QSFPDD_FAB_P18_INTR]                  =         {QSFPDD_FAB_18_19_INTR_REG               , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P19_INTR]                  =         {QSFPDD_FAB_18_19_INTR_REG               , MASK_0000_0010, DATA_DEC},

    [QSFPDD_FAB_P10_ABS]                   =         {QSFPDD_FAB_10_17_ABS_REG                , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P11_ABS]                   =         {QSFPDD_FAB_10_17_ABS_REG                , MASK_0000_0010, DATA_DEC},
    [QSFPDD_FAB_P12_ABS]                   =         {QSFPDD_FAB_10_17_ABS_REG                , MASK_0000_0100, DATA_DEC},
    [QSFPDD_FAB_P13_ABS]                   =         {QSFPDD_FAB_10_17_ABS_REG                , MASK_0000_1000, DATA_DEC},
    [QSFPDD_FAB_P14_ABS]                   =         {QSFPDD_FAB_10_17_ABS_REG                , MASK_0001_0000, DATA_DEC},
    [QSFPDD_FAB_P15_ABS]                   =         {QSFPDD_FAB_10_17_ABS_REG                , MASK_0010_0000, DATA_DEC},
    [QSFPDD_FAB_P16_ABS]                   =         {QSFPDD_FAB_10_17_ABS_REG                , MASK_0100_0000, DATA_DEC},
    [QSFPDD_FAB_P17_ABS]                   =         {QSFPDD_FAB_10_17_ABS_REG                , MASK_1000_0000, DATA_DEC},
    [QSFPDD_FAB_P18_ABS]                   =         {QSFPDD_FAB_18_19_ABS_REG                , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P19_ABS]                   =         {QSFPDD_FAB_18_19_ABS_REG                , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P20_27_ABS_EVENT]          =         {QSFPDD_NIF_20_27_ABS_EVENT_REG          , MASK_ALL      , DATA_HEX},
    [QSFPDD_NIF_P28_35_ABS_EVENT]          =         {QSFPDD_NIF_28_35_ABS_EVENT_REG          , MASK_ALL      , DATA_HEX},
    [QSFPDD_FAB_P10_17_ABS_EVENT]          =         {QSFPDD_FAB_10_17_ABS_EVENT_REG          , MASK_ALL      , DATA_HEX},
    [QSFPDD_FAB_P18_19_ABS_EVENT]          =         {QSFPDD_FAB_18_19_ABS_EVENT_REG          , MASK_ALL      , DATA_HEX},
    [QSFPDD_FAB_P10_FUSE_INTR]             =         {QSFPDD_FAB_10_17_FUSE_INTR_REG          , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P11_FUSE_INTR]             =         {QSFPDD_FAB_10_17_FUSE_INTR_REG          , MASK_0000_0010, DATA_DEC},
    [QSFPDD_FAB_P12_FUSE_INTR]             =         {QSFPDD_FAB_10_17_FUSE_INTR_REG          , MASK_0000_0100, DATA_DEC},
    [QSFPDD_FAB_P13_FUSE_INTR]             =         {QSFPDD_FAB_10_17_FUSE_INTR_REG          , MASK_0000_1000, DATA_DEC},
    [QSFPDD_FAB_P14_FUSE_INTR]             =         {QSFPDD_FAB_10_17_FUSE_INTR_REG          , MASK_0001_0000, DATA_DEC},
    [QSFPDD_FAB_P15_FUSE_INTR]             =         {QSFPDD_FAB_10_17_FUSE_INTR_REG          , MASK_0010_0000, DATA_DEC},
    [QSFPDD_FAB_P16_FUSE_INTR]             =         {QSFPDD_FAB_10_17_FUSE_INTR_REG          , MASK_0100_0000, DATA_DEC},
    [QSFPDD_FAB_P17_FUSE_INTR]             =         {QSFPDD_FAB_10_17_FUSE_INTR_REG          , MASK_1000_0000, DATA_DEC},
    [QSFPDD_FAB_P18_FUSE_INTR]             =         {QSFPDD_FAB_18_19_FUSE_INTR_REG          , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P19_FUSE_INTR]             =         {QSFPDD_FAB_18_19_FUSE_INTR_REG          , MASK_0000_0010, DATA_DEC},

    [QSFPDD_NIF_P20_INTR_MASK]             =         {QSFPDD_NIF_20_27_INTR_MASK_REG          , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P21_INTR_MASK]             =         {QSFPDD_NIF_20_27_INTR_MASK_REG          , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P22_INTR_MASK]             =         {QSFPDD_NIF_20_27_INTR_MASK_REG          , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P23_INTR_MASK]             =         {QSFPDD_NIF_20_27_INTR_MASK_REG          , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P24_INTR_MASK]             =         {QSFPDD_NIF_20_27_INTR_MASK_REG          , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P25_INTR_MASK]             =         {QSFPDD_NIF_20_27_INTR_MASK_REG          , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P26_INTR_MASK]             =         {QSFPDD_NIF_20_27_INTR_MASK_REG          , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P27_INTR_MASK]             =         {QSFPDD_NIF_20_27_INTR_MASK_REG          , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P28_INTR_MASK]             =         {QSFPDD_NIF_28_35_INTR_MASK_REG          , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P29_INTR_MASK]             =         {QSFPDD_NIF_28_35_INTR_MASK_REG          , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P30_INTR_MASK]             =         {QSFPDD_NIF_28_35_INTR_MASK_REG          , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P31_INTR_MASK]             =         {QSFPDD_NIF_28_35_INTR_MASK_REG          , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P32_INTR_MASK]             =         {QSFPDD_NIF_28_35_INTR_MASK_REG          , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P33_INTR_MASK]             =         {QSFPDD_NIF_28_35_INTR_MASK_REG          , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P34_INTR_MASK]             =         {QSFPDD_NIF_28_35_INTR_MASK_REG          , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P35_INTR_MASK]             =         {QSFPDD_NIF_28_35_INTR_MASK_REG          , MASK_1000_0000, DATA_DEC},

    [QSFPDD_NIF_P20_ABS_MASK]              =         {QSFPDD_NIF_20_27_ABS_MASK_REG           , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P21_ABS_MASK]              =         {QSFPDD_NIF_20_27_ABS_MASK_REG           , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P22_ABS_MASK]              =         {QSFPDD_NIF_20_27_ABS_MASK_REG           , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P23_ABS_MASK]              =         {QSFPDD_NIF_20_27_ABS_MASK_REG           , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P24_ABS_MASK]              =         {QSFPDD_NIF_20_27_ABS_MASK_REG           , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P25_ABS_MASK]              =         {QSFPDD_NIF_20_27_ABS_MASK_REG           , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P26_ABS_MASK]              =         {QSFPDD_NIF_20_27_ABS_MASK_REG           , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P27_ABS_MASK]              =         {QSFPDD_NIF_20_27_ABS_MASK_REG           , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P28_ABS_MASK]              =         {QSFPDD_NIF_28_35_ABS_MASK_REG           , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P29_ABS_MASK]              =         {QSFPDD_NIF_28_35_ABS_MASK_REG           , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P30_ABS_MASK]              =         {QSFPDD_NIF_28_35_ABS_MASK_REG           , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P31_ABS_MASK]              =         {QSFPDD_NIF_28_35_ABS_MASK_REG           , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P32_ABS_MASK]              =         {QSFPDD_NIF_28_35_ABS_MASK_REG           , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P33_ABS_MASK]              =         {QSFPDD_NIF_28_35_ABS_MASK_REG           , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P34_ABS_MASK]              =         {QSFPDD_NIF_28_35_ABS_MASK_REG           , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P35_ABS_MASK]              =         {QSFPDD_NIF_28_35_ABS_MASK_REG           , MASK_1000_0000, DATA_DEC},

    [QSFPDD_NIF_P20_FUSE_INTR_MASK]        =         {QSFPDD_NIF_20_27_FUSE_INTR_MASK_REG     , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P21_FUSE_INTR_MASK]        =         {QSFPDD_NIF_20_27_FUSE_INTR_MASK_REG     , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P22_FUSE_INTR_MASK]        =         {QSFPDD_NIF_20_27_FUSE_INTR_MASK_REG     , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P23_FUSE_INTR_MASK]        =         {QSFPDD_NIF_20_27_FUSE_INTR_MASK_REG     , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P24_FUSE_INTR_MASK]        =         {QSFPDD_NIF_20_27_FUSE_INTR_MASK_REG     , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P25_FUSE_INTR_MASK]        =         {QSFPDD_NIF_20_27_FUSE_INTR_MASK_REG     , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P26_FUSE_INTR_MASK]        =         {QSFPDD_NIF_20_27_FUSE_INTR_MASK_REG     , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P27_FUSE_INTR_MASK]        =         {QSFPDD_NIF_20_27_FUSE_INTR_MASK_REG     , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P28_FUSE_INTR_MASK]        =         {QSFPDD_NIF_28_35_FUSE_INTR_MASK_REG     , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P29_FUSE_INTR_MASK]        =         {QSFPDD_NIF_28_35_FUSE_INTR_MASK_REG     , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P30_FUSE_INTR_MASK]        =         {QSFPDD_NIF_28_35_FUSE_INTR_MASK_REG     , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P31_FUSE_INTR_MASK]        =         {QSFPDD_NIF_28_35_FUSE_INTR_MASK_REG     , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P32_FUSE_INTR_MASK]        =         {QSFPDD_NIF_28_35_FUSE_INTR_MASK_REG     , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P33_FUSE_INTR_MASK]        =         {QSFPDD_NIF_28_35_FUSE_INTR_MASK_REG     , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P34_FUSE_INTR_MASK]        =         {QSFPDD_NIF_28_35_FUSE_INTR_MASK_REG     , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P35_FUSE_INTR_MASK]        =         {QSFPDD_NIF_28_35_FUSE_INTR_MASK_REG     , MASK_1000_0000, DATA_DEC},

    [QSFPDD_FAB_P10_INTR_MASK]             =         {QSFPDD_FAB_10_17_INTR_MASK_REG          , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P11_INTR_MASK]             =         {QSFPDD_FAB_10_17_INTR_MASK_REG          , MASK_0000_0010, DATA_DEC},
    [QSFPDD_FAB_P12_INTR_MASK]             =         {QSFPDD_FAB_10_17_INTR_MASK_REG          , MASK_0000_0100, DATA_DEC},
    [QSFPDD_FAB_P13_INTR_MASK]             =         {QSFPDD_FAB_10_17_INTR_MASK_REG          , MASK_0000_1000, DATA_DEC},
    [QSFPDD_FAB_P14_INTR_MASK]             =         {QSFPDD_FAB_10_17_INTR_MASK_REG          , MASK_0001_0000, DATA_DEC},
    [QSFPDD_FAB_P15_INTR_MASK]             =         {QSFPDD_FAB_10_17_INTR_MASK_REG          , MASK_0010_0000, DATA_DEC},
    [QSFPDD_FAB_P16_INTR_MASK]             =         {QSFPDD_FAB_10_17_INTR_MASK_REG          , MASK_0100_0000, DATA_DEC},
    [QSFPDD_FAB_P17_INTR_MASK]             =         {QSFPDD_FAB_10_17_INTR_MASK_REG          , MASK_1000_0000, DATA_DEC},
    [QSFPDD_FAB_P18_INTR_MASK]             =         {QSFPDD_FAB_18_19_INTR_MASK_REG          , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P19_INTR_MASK]             =         {QSFPDD_FAB_18_19_INTR_MASK_REG          , MASK_0000_0010, DATA_DEC},

    [QSFPDD_FAB_P10_ABS_MASK]              =         {QSFPDD_FAB_10_17_ABS_MASK_REG           , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P11_ABS_MASK]              =         {QSFPDD_FAB_10_17_ABS_MASK_REG           , MASK_0000_0010, DATA_DEC},
    [QSFPDD_FAB_P12_ABS_MASK]              =         {QSFPDD_FAB_10_17_ABS_MASK_REG           , MASK_0000_0100, DATA_DEC},
    [QSFPDD_FAB_P13_ABS_MASK]              =         {QSFPDD_FAB_10_17_ABS_MASK_REG           , MASK_0000_1000, DATA_DEC},
    [QSFPDD_FAB_P14_ABS_MASK]              =         {QSFPDD_FAB_10_17_ABS_MASK_REG           , MASK_0001_0000, DATA_DEC},
    [QSFPDD_FAB_P15_ABS_MASK]              =         {QSFPDD_FAB_10_17_ABS_MASK_REG           , MASK_0010_0000, DATA_DEC},
    [QSFPDD_FAB_P16_ABS_MASK]              =         {QSFPDD_FAB_10_17_ABS_MASK_REG           , MASK_0100_0000, DATA_DEC},
    [QSFPDD_FAB_P17_ABS_MASK]              =         {QSFPDD_FAB_10_17_ABS_MASK_REG           , MASK_1000_0000, DATA_DEC},
    [QSFPDD_FAB_P18_ABS_MASK]              =         {QSFPDD_FAB_18_19_ABS_MASK_REG           , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P19_ABS_MASK]              =         {QSFPDD_FAB_18_19_ABS_MASK_REG           , MASK_0000_0010, DATA_DEC},

    [QSFPDD_FAB_P10_FUSE_INTR_MASK]        =         {QSFPDD_FAB_10_17_FUSE_INTR_MASK_REG     , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P11_FUSE_INTR_MASK]        =         {QSFPDD_FAB_10_17_FUSE_INTR_MASK_REG     , MASK_0000_0010, DATA_DEC},
    [QSFPDD_FAB_P12_FUSE_INTR_MASK]        =         {QSFPDD_FAB_10_17_FUSE_INTR_MASK_REG     , MASK_0000_0100, DATA_DEC},
    [QSFPDD_FAB_P13_FUSE_INTR_MASK]        =         {QSFPDD_FAB_10_17_FUSE_INTR_MASK_REG     , MASK_0000_1000, DATA_DEC},
    [QSFPDD_FAB_P14_FUSE_INTR_MASK]        =         {QSFPDD_FAB_10_17_FUSE_INTR_MASK_REG     , MASK_0001_0000, DATA_DEC},
    [QSFPDD_FAB_P15_FUSE_INTR_MASK]        =         {QSFPDD_FAB_10_17_FUSE_INTR_MASK_REG     , MASK_0010_0000, DATA_DEC},
    [QSFPDD_FAB_P16_FUSE_INTR_MASK]        =         {QSFPDD_FAB_10_17_FUSE_INTR_MASK_REG     , MASK_0100_0000, DATA_DEC},
    [QSFPDD_FAB_P17_FUSE_INTR_MASK]        =         {QSFPDD_FAB_10_17_FUSE_INTR_MASK_REG     , MASK_1000_0000, DATA_DEC},
    [QSFPDD_FAB_P18_FUSE_INTR_MASK]        =         {QSFPDD_FAB_18_19_FUSE_INTR_MASK_REG     , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P19_FUSE_INTR_MASK]        =         {QSFPDD_FAB_18_19_FUSE_INTR_MASK_REG     , MASK_0000_0010, DATA_DEC},

    [QSFPDD_NIF_P20_RST]                   =         {QSFPDD_NIF_20_27_RST_REG                , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P21_RST]                   =         {QSFPDD_NIF_20_27_RST_REG                , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P22_RST]                   =         {QSFPDD_NIF_20_27_RST_REG                , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P23_RST]                   =         {QSFPDD_NIF_20_27_RST_REG                , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P24_RST]                   =         {QSFPDD_NIF_20_27_RST_REG                , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P25_RST]                   =         {QSFPDD_NIF_20_27_RST_REG                , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P26_RST]                   =         {QSFPDD_NIF_20_27_RST_REG                , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P27_RST]                   =         {QSFPDD_NIF_20_27_RST_REG                , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P28_RST]                   =         {QSFPDD_NIF_28_35_RST_REG                , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P29_RST]                   =         {QSFPDD_NIF_28_35_RST_REG                , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P30_RST]                   =         {QSFPDD_NIF_28_35_RST_REG                , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P31_RST]                   =         {QSFPDD_NIF_28_35_RST_REG                , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P32_RST]                   =         {QSFPDD_NIF_28_35_RST_REG                , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P33_RST]                   =         {QSFPDD_NIF_28_35_RST_REG                , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P34_RST]                   =         {QSFPDD_NIF_28_35_RST_REG                , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P35_RST]                   =         {QSFPDD_NIF_28_35_RST_REG                , MASK_1000_0000, DATA_DEC},
    [QSFPDD_FAB_P10_RST]                   =         {QSFPDD_FAB_10_17_RST_REG                , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P11_RST]                   =         {QSFPDD_FAB_10_17_RST_REG                , MASK_0000_0010, DATA_DEC},
    [QSFPDD_FAB_P12_RST]                   =         {QSFPDD_FAB_10_17_RST_REG                , MASK_0000_0100, DATA_DEC},
    [QSFPDD_FAB_P13_RST]                   =         {QSFPDD_FAB_10_17_RST_REG                , MASK_0000_1000, DATA_DEC},
    [QSFPDD_FAB_P14_RST]                   =         {QSFPDD_FAB_10_17_RST_REG                , MASK_0001_0000, DATA_DEC},
    [QSFPDD_FAB_P15_RST]                   =         {QSFPDD_FAB_10_17_RST_REG                , MASK_0010_0000, DATA_DEC},
    [QSFPDD_FAB_P16_RST]                   =         {QSFPDD_FAB_10_17_RST_REG                , MASK_0100_0000, DATA_DEC},
    [QSFPDD_FAB_P17_RST]                   =         {QSFPDD_FAB_10_17_RST_REG                , MASK_1000_0000, DATA_DEC},
    [QSFPDD_FAB_P18_RST]                   =         {QSFPDD_FAB_18_19_RST_REG                , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P19_RST]                   =         {QSFPDD_FAB_18_19_RST_REG                , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P20_LP_MODE]               =         {QSFPDD_NIF_20_27_LP_REG                 , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P21_LP_MODE]               =         {QSFPDD_NIF_20_27_LP_REG                 , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P22_LP_MODE]               =         {QSFPDD_NIF_20_27_LP_REG                 , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P23_LP_MODE]               =         {QSFPDD_NIF_20_27_LP_REG                 , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P24_LP_MODE]               =         {QSFPDD_NIF_20_27_LP_REG                 , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P25_LP_MODE]               =         {QSFPDD_NIF_20_27_LP_REG                 , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P26_LP_MODE]               =         {QSFPDD_NIF_20_27_LP_REG                 , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P27_LP_MODE]               =         {QSFPDD_NIF_20_27_LP_REG                 , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P28_LP_MODE]               =         {QSFPDD_NIF_28_35_LP_REG                 , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P29_LP_MODE]               =         {QSFPDD_NIF_28_35_LP_REG                 , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P30_LP_MODE]               =         {QSFPDD_NIF_28_35_LP_REG                 , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P31_LP_MODE]               =         {QSFPDD_NIF_28_35_LP_REG                 , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P32_LP_MODE]               =         {QSFPDD_NIF_28_35_LP_REG                 , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P33_LP_MODE]               =         {QSFPDD_NIF_28_35_LP_REG                 , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P34_LP_MODE]               =         {QSFPDD_NIF_28_35_LP_REG                 , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P35_LP_MODE]               =         {QSFPDD_NIF_28_35_LP_REG                 , MASK_1000_0000, DATA_DEC},
    [QSFPDD_FAB_P10_LP_MODE]               =         {QSFPDD_FAB_10_17_LP_REG                 , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P11_LP_MODE]               =         {QSFPDD_FAB_10_17_LP_REG                 , MASK_0000_0010, DATA_DEC},
    [QSFPDD_FAB_P12_LP_MODE]               =         {QSFPDD_FAB_10_17_LP_REG                 , MASK_0000_0100, DATA_DEC},
    [QSFPDD_FAB_P13_LP_MODE]               =         {QSFPDD_FAB_10_17_LP_REG                 , MASK_0000_1000, DATA_DEC},
    [QSFPDD_FAB_P14_LP_MODE]               =         {QSFPDD_FAB_10_17_LP_REG                 , MASK_0001_0000, DATA_DEC},
    [QSFPDD_FAB_P15_LP_MODE]               =         {QSFPDD_FAB_10_17_LP_REG                 , MASK_0010_0000, DATA_DEC},
    [QSFPDD_FAB_P16_LP_MODE]               =         {QSFPDD_FAB_10_17_LP_REG                 , MASK_0100_0000, DATA_DEC},
    [QSFPDD_FAB_P17_LP_MODE]               =         {QSFPDD_FAB_10_17_LP_REG                 , MASK_1000_0000, DATA_DEC},
    [QSFPDD_FAB_P18_LP_MODE]               =         {QSFPDD_FAB_18_19_LP_REG                 , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P19_LP_MODE]               =         {QSFPDD_FAB_18_19_LP_REG                 , MASK_0000_0010, DATA_DEC},

    [FPGA_CPLD2_CH_DIS]                    =         {FPGA_QSFPDD_PORT_CH_SEL_1_REG           , MASK_NONE     , DATA_HEX},
    [FPGA_QSFPDD_FAB_10_CH]                =         {FPGA_QSFPDD_PORT_CH_SEL_1_REG           , MASK_NONE     , DATA_HEX},
    [FPGA_QSFPDD_FAB_11_CH]                =         {FPGA_QSFPDD_PORT_CH_SEL_1_REG           , MASK_NONE     , DATA_HEX},
    [FPGA_QSFPDD_FAB_12_CH]                =         {FPGA_QSFPDD_PORT_CH_SEL_1_REG           , MASK_NONE     , DATA_HEX},
    [FPGA_QSFPDD_FAB_13_CH]                =         {FPGA_QSFPDD_PORT_CH_SEL_1_REG           , MASK_NONE     , DATA_HEX},
    [FPGA_QSFPDD_FAB_14_CH]                =         {FPGA_QSFPDD_PORT_CH_SEL_1_REG           , MASK_NONE     , DATA_HEX},
    [FPGA_QSFPDD_FAB_15_CH]                =         {FPGA_QSFPDD_PORT_CH_SEL_1_REG           , MASK_NONE     , DATA_HEX},
    [FPGA_QSFPDD_FAB_16_CH]                =         {FPGA_QSFPDD_PORT_CH_SEL_1_REG           , MASK_NONE     , DATA_HEX},
    [FPGA_QSFPDD_FAB_17_CH]                =         {FPGA_QSFPDD_PORT_CH_SEL_1_REG           , MASK_NONE     , DATA_HEX},
    [FPGA_QSFPDD_FAB_18_CH]                =         {FPGA_QSFPDD_PORT_CH_SEL_1_REG           , MASK_NONE     , DATA_HEX},
    [FPGA_QSFPDD_FAB_19_CH]                =         {FPGA_QSFPDD_PORT_CH_SEL_1_REG           , MASK_NONE     , DATA_HEX},

    [QSFPDD_FAB_P10_LED_STATUS]            =         {QSFPDD_FAB_LED_10_11_STATUS_REG         , MASK_0000_1111, DATA_DEC},      
    [QSFPDD_FAB_P11_LED_STATUS]            =         {QSFPDD_FAB_LED_10_11_STATUS_REG         , MASK_1111_0000, DATA_DEC},  
    [QSFPDD_FAB_P12_LED_STATUS]            =         {QSFPDD_FAB_LED_12_13_STATUS_REG         , MASK_0000_1111, DATA_DEC},  
    [QSFPDD_FAB_P13_LED_STATUS]            =         {QSFPDD_FAB_LED_12_13_STATUS_REG         , MASK_1111_0000, DATA_DEC},  
    [QSFPDD_FAB_P14_LED_STATUS]            =         {QSFPDD_FAB_LED_14_15_STATUS_REG         , MASK_0000_1111, DATA_DEC},  
    [QSFPDD_FAB_P15_LED_STATUS]            =         {QSFPDD_FAB_LED_14_15_STATUS_REG         , MASK_1111_0000, DATA_DEC},  
    [QSFPDD_FAB_P16_LED_STATUS]            =         {QSFPDD_FAB_LED_16_17_STATUS_REG         , MASK_0000_1111, DATA_DEC},  
    [QSFPDD_FAB_P17_LED_STATUS]            =         {QSFPDD_FAB_LED_16_17_STATUS_REG         , MASK_1111_0000, DATA_DEC},  
    [QSFPDD_FAB_P18_LED_STATUS]            =         {QSFPDD_FAB_LED_18_19_STATUS_REG         , MASK_0000_1111, DATA_DEC},  
    [QSFPDD_FAB_P19_LED_STATUS]            =         {QSFPDD_FAB_LED_18_19_STATUS_REG         , MASK_1111_0000, DATA_DEC},  
 
    [QSFPDD_FAB_P10_I2C_STUCK]             =         {QSFPDD_FAB_10_17_STUCK_REG              , MASK_0000_0001, DATA_DEC},    
    [QSFPDD_FAB_P11_I2C_STUCK]             =         {QSFPDD_FAB_10_17_STUCK_REG              , MASK_0000_0010, DATA_DEC},
    [QSFPDD_FAB_P12_I2C_STUCK]             =         {QSFPDD_FAB_10_17_STUCK_REG              , MASK_0000_0100, DATA_DEC},
    [QSFPDD_FAB_P13_I2C_STUCK]             =         {QSFPDD_FAB_10_17_STUCK_REG              , MASK_0000_1000, DATA_DEC},
    [QSFPDD_FAB_P14_I2C_STUCK]             =         {QSFPDD_FAB_10_17_STUCK_REG              , MASK_0001_0000, DATA_DEC},
    [QSFPDD_FAB_P15_I2C_STUCK]             =         {QSFPDD_FAB_10_17_STUCK_REG              , MASK_0010_0000, DATA_DEC},
    [QSFPDD_FAB_P16_I2C_STUCK]             =         {QSFPDD_FAB_10_17_STUCK_REG              , MASK_0100_0000, DATA_DEC},
    [QSFPDD_FAB_P17_I2C_STUCK]             =         {QSFPDD_FAB_10_17_STUCK_REG              , MASK_1000_0000, DATA_DEC},
    [QSFPDD_FAB_P18_I2C_STUCK]             =         {QSFPDD_FAB_18_19_STUCK_REG              , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P19_I2C_STUCK]             =         {QSFPDD_FAB_18_19_STUCK_REG              , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P20_I2C_STUCK]             =         {QSFPDD_NIF_20_27_STUCK_REG              , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P21_I2C_STUCK]             =         {QSFPDD_NIF_20_27_STUCK_REG              , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P22_I2C_STUCK]             =         {QSFPDD_NIF_20_27_STUCK_REG              , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P23_I2C_STUCK]             =         {QSFPDD_NIF_20_27_STUCK_REG              , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P24_I2C_STUCK]             =         {QSFPDD_NIF_20_27_STUCK_REG              , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P25_I2C_STUCK]             =         {QSFPDD_NIF_20_27_STUCK_REG              , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P26_I2C_STUCK]             =         {QSFPDD_NIF_20_27_STUCK_REG              , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P27_I2C_STUCK]             =         {QSFPDD_NIF_20_27_STUCK_REG              , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P28_I2C_STUCK]             =         {QSFPDD_NIF_28_35_STUCK_REG              , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P29_I2C_STUCK]             =         {QSFPDD_NIF_28_35_STUCK_REG              , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P30_I2C_STUCK]             =         {QSFPDD_NIF_28_35_STUCK_REG              , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P31_I2C_STUCK]             =         {QSFPDD_NIF_28_35_STUCK_REG              , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P32_I2C_STUCK]             =         {QSFPDD_NIF_28_35_STUCK_REG              , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P33_I2C_STUCK]             =         {QSFPDD_NIF_28_35_STUCK_REG              , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P34_I2C_STUCK]             =         {QSFPDD_NIF_28_35_STUCK_REG              , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P35_I2C_STUCK]             =         {QSFPDD_NIF_28_35_STUCK_REG              , MASK_1000_0000, DATA_DEC},
    //FPGA
    [FPGA_ID]                              =         {FPGA_ID_REG                             , MASK_0000_0111, DATA_DEC},
    [FPGA_VER_1]                           =         {FPGA_VER_1_REG                          , MASK_ALL      , DATA_DEC},
    [FPGA_MINOR_VER]                       =         {FPGA_VER_1_REG                          , MASK_0011_1111, DATA_DEC},
    [FPGA_MAJOR_VER]                       =         {FPGA_VER_1_REG                          , MASK_1100_0000, DATA_DEC},
    [FPGA_BUILD_VER]                       =         {FPGA_VER_2_REG                          , MASK_ALL      , DATA_DEC},
    [FPGA_VERSION_H]                       =         {NONE_REG                                , MASK_NONE     , DATA_UNK},
    [FPGA_DEV_INFO]                        =         {FPGA_DEV_INFO_REG                       , MASK_0000_1111, DATA_DEC},
    [SFP28_P37_TS]                         =         {SFP28_TS_REG                            , MASK_0000_1000, DATA_DEC},
    [SFP28_P36_TS]                         =         {SFP28_TS_REG                            , MASK_0000_0100, DATA_DEC},
    [MGMT_P1_TS]                           =         {SFP28_TS_REG                            , MASK_0000_0010, DATA_DEC},
    [MGMT_P0_TS]                           =         {SFP28_TS_REG                            , MASK_0000_0001, DATA_DEC},
    [SFP28_P37_RS]                         =         {SFP28_RS_REG                            , MASK_0000_1000, DATA_DEC},
    [SFP28_P36_RS]                         =         {SFP28_RS_REG                            , MASK_0000_0100, DATA_DEC},
    [MGMT_P1_RS]                           =         {SFP28_RS_REG                            , MASK_0000_0010, DATA_DEC},
    [MGMT_P0_RS]                           =         {SFP28_RS_REG                            , MASK_0000_0001, DATA_DEC},
    [SFP28_P37_TX_DIS]                     =         {SFP28_TX_DIS_REG                        , MASK_0000_1000, DATA_DEC},
    [SFP28_P36_TX_DIS]                     =         {SFP28_TX_DIS_REG                        , MASK_0000_0100, DATA_DEC},
    [MGMT_P1_TX_DIS]                       =         {SFP28_TX_DIS_REG                        , MASK_0000_0010, DATA_DEC},
    [MGMT_P0_TX_DIS]                       =         {SFP28_TX_DIS_REG                        , MASK_0000_0001, DATA_DEC},
    [SFP28_P37_TX_FLT]                     =         {SFP28_TX_FLT_REG                        , MASK_0000_1000, DATA_DEC},
    [SFP28_P36_TX_FLT]                     =         {SFP28_TX_FLT_REG                        , MASK_0000_0100, DATA_DEC},
    [MGMT_P1_TX_FLT]                       =         {SFP28_TX_FLT_REG                        , MASK_0000_0010, DATA_DEC},
    [MGMT_P0_TX_FLT]                       =         {SFP28_TX_FLT_REG                        , MASK_0000_0001, DATA_DEC},
    [SFP28_P37_RX_LOS]                     =         {SFP28_RX_LOS_REG                        , MASK_0000_1000, DATA_DEC},
    [SFP28_P36_RX_LOS]                     =         {SFP28_RX_LOS_REG                        , MASK_0000_0100, DATA_DEC},
    [MGMT_P1_RX_LOS]                       =         {SFP28_RX_LOS_REG                        , MASK_0000_0010, DATA_DEC},
    [MGMT_P0_RX_LOS]                       =         {SFP28_RX_LOS_REG                        , MASK_0000_0001, DATA_DEC},
    [SFP28_P37_ABS]                        =         {SFP28_ABS_REG                           , MASK_0000_1000, DATA_DEC},
    [SFP28_P36_ABS]                        =         {SFP28_ABS_REG                           , MASK_0000_0100, DATA_DEC},
    [MGMT_P1_ABS]                          =         {SFP28_ABS_REG                           , MASK_0000_0010, DATA_DEC},
    [MGMT_P0_ABS]                          =         {SFP28_ABS_REG                           , MASK_0000_0001, DATA_DEC},
    [SFP28_P37_TX_FLT_MASK]                =         {SFP28_TX_FLT_MASK_REG                   , MASK_0000_1000, DATA_DEC},
    [SFP28_P36_TX_FLT_MASK]                =         {SFP28_TX_FLT_MASK_REG                   , MASK_0000_0100, DATA_DEC},
    [MGMT_P1_TX_FLT_MASK]                  =         {SFP28_TX_FLT_MASK_REG                   , MASK_0000_0010, DATA_DEC},
    [MGMT_P0_TX_FLT_MASK]                  =         {SFP28_TX_FLT_MASK_REG                   , MASK_0000_0001, DATA_DEC},
    [SFP28_P37_RX_LOS_MASK]                =         {SFP28_RX_LOS_MASK_REG                   , MASK_0000_1000, DATA_DEC},
    [SFP28_P36_RX_LOS_MASK]                =         {SFP28_RX_LOS_MASK_REG                   , MASK_0000_0100, DATA_DEC},
    [MGMT_P1_RX_LOS_MASK]                  =         {SFP28_RX_LOS_MASK_REG                   , MASK_0000_0010, DATA_DEC},
    [MGMT_P0_RX_LOS_MASK]                  =         {SFP28_RX_LOS_MASK_REG                   , MASK_0000_0001, DATA_DEC},
    [SFP28_P37_ABS_MASK]                   =         {SFP28_ABS_MASK_REG                      , MASK_0000_1000, DATA_DEC},
    [SFP28_P36_ABS_MASK]                   =         {SFP28_ABS_MASK_REG                      , MASK_0000_0100, DATA_DEC},
    [MGMT_P1_ABS_MASK]                     =         {SFP28_ABS_MASK_REG                      , MASK_0000_0010, DATA_DEC},
    [MGMT_P0_ABS_MASK]                     =         {SFP28_ABS_MASK_REG                      , MASK_0000_0001, DATA_DEC},
    [SFP28_TX_FLT_EVENT]                   =         {SFP28_TX_FLT_EVENT_REG                  , MASK_ALL      , DATA_HEX},
    [SFP28_RX_LOS_EVENT]                   =         {SFP28_RX_LOS_EVENT_REG                  , MASK_ALL      , DATA_HEX},
    [SFP28_ABS_EVENT]                      =         {SFP28_ABS_EVENT_REG                     , MASK_ALL      , DATA_HEX},
    [MGMT_P0_I2C_STUCK]                    =         {FPGA_SFP28_PORT_0_3_STUCK_REG                     , MASK_0000_0001, DATA_DEC},  
    [MGMT_P1_I2C_STUCK]                    =         {FPGA_SFP28_PORT_0_3_STUCK_REG                     , MASK_0000_0010, DATA_DEC},  
    [SFP28_P36_I2C_STUCK]                  =         {FPGA_SFP28_PORT_0_3_STUCK_REG                     , MASK_0000_0100, DATA_DEC},
    [SFP28_P37_I2C_STUCK]                  =         {FPGA_SFP28_PORT_0_3_STUCK_REG                     , MASK_0000_1000, DATA_DEC},
    //CPLD 4
    [QSFPDD_NIF_P0_INTR]                   =         {QSFPDD_NIF_0_7_INTR_REG                 , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P1_INTR]                   =         {QSFPDD_NIF_0_7_INTR_REG                 , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P2_INTR]                   =         {QSFPDD_NIF_0_7_INTR_REG                 , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P3_INTR]                   =         {QSFPDD_NIF_0_7_INTR_REG                 , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P4_INTR]                   =         {QSFPDD_NIF_0_7_INTR_REG                 , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P5_INTR]                   =         {QSFPDD_NIF_0_7_INTR_REG                 , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P6_INTR]                   =         {QSFPDD_NIF_0_7_INTR_REG                 , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P7_INTR]                   =         {QSFPDD_NIF_0_7_INTR_REG                 , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P8_INTR]                   =         {QSFPDD_NIF_8_15_INTR_REG                , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P9_INTR]                   =         {QSFPDD_NIF_8_15_INTR_REG                , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P10_INTR]                  =         {QSFPDD_NIF_8_15_INTR_REG                , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P11_INTR]                  =         {QSFPDD_NIF_8_15_INTR_REG                , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P12_INTR]                  =         {QSFPDD_NIF_8_15_INTR_REG                , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P13_INTR]                  =         {QSFPDD_NIF_8_15_INTR_REG                , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P14_INTR]                  =         {QSFPDD_NIF_8_15_INTR_REG                , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P15_INTR]                  =         {QSFPDD_NIF_8_15_INTR_REG                , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P16_INTR]                  =         {QSFPDD_NIF_16_19_INTR_REG               , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P17_INTR]                  =         {QSFPDD_NIF_16_19_INTR_REG               , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P18_INTR]                  =         {QSFPDD_NIF_16_19_INTR_REG               , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P19_INTR]                  =         {QSFPDD_NIF_16_19_INTR_REG               , MASK_0000_1000, DATA_DEC},

    [QSFPDD_NIF_P0_ABS]                    =         {QSFPDD_NIF_0_7_ABS_REG                  , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P1_ABS]                    =         {QSFPDD_NIF_0_7_ABS_REG                  , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P2_ABS]                    =         {QSFPDD_NIF_0_7_ABS_REG                  , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P3_ABS]                    =         {QSFPDD_NIF_0_7_ABS_REG                  , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P4_ABS]                    =         {QSFPDD_NIF_0_7_ABS_REG                  , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P5_ABS]                    =         {QSFPDD_NIF_0_7_ABS_REG                  , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P6_ABS]                    =         {QSFPDD_NIF_0_7_ABS_REG                  , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P7_ABS]                    =         {QSFPDD_NIF_0_7_ABS_REG                  , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P8_ABS]                    =         {QSFPDD_NIF_8_15_ABS_REG                 , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P9_ABS]                    =         {QSFPDD_NIF_8_15_ABS_REG                 , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P10_ABS]                   =         {QSFPDD_NIF_8_15_ABS_REG                 , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P11_ABS]                   =         {QSFPDD_NIF_8_15_ABS_REG                 , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P12_ABS]                   =         {QSFPDD_NIF_8_15_ABS_REG                 , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P13_ABS]                   =         {QSFPDD_NIF_8_15_ABS_REG                 , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P14_ABS]                   =         {QSFPDD_NIF_8_15_ABS_REG                 , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P15_ABS]                   =         {QSFPDD_NIF_8_15_ABS_REG                 , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P16_ABS]                   =         {QSFPDD_NIF_16_19_ABS_REG                , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P17_ABS]                   =         {QSFPDD_NIF_16_19_ABS_REG                , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P18_ABS]                   =         {QSFPDD_NIF_16_19_ABS_REG                , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P19_ABS]                   =         {QSFPDD_NIF_16_19_ABS_REG                , MASK_0000_1000, DATA_DEC},

    [QSFPDD_NIF_P0_7_ABS_EVENT]            =         {QSFPDD_NIF_0_7_ABS_EVENT_REG            , MASK_ALL      , DATA_HEX},
    [QSFPDD_NIF_P8_15_ABS_EVENT]           =         {QSFPDD_NIF_8_15_ABS_EVENT_REG           , MASK_ALL      , DATA_HEX}, 
    [QSFPDD_NIF_P16_19_ABS_EVENT]          =         {QSFPDD_NIF_16_19_ABS_EVENT_REG          , MASK_ALL      , DATA_HEX},  

    [QSFPDD_NIF_P0_FUSE_INTR]              =         {QSFPDD_NIF_0_7_FUSE_INTR_REG            , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P1_FUSE_INTR]              =         {QSFPDD_NIF_0_7_FUSE_INTR_REG            , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P2_FUSE_INTR]              =         {QSFPDD_NIF_0_7_FUSE_INTR_REG            , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P3_FUSE_INTR]              =         {QSFPDD_NIF_0_7_FUSE_INTR_REG            , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P4_FUSE_INTR]              =         {QSFPDD_NIF_0_7_FUSE_INTR_REG            , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P5_FUSE_INTR]              =         {QSFPDD_NIF_0_7_FUSE_INTR_REG            , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P6_FUSE_INTR]              =         {QSFPDD_NIF_0_7_FUSE_INTR_REG            , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P7_FUSE_INTR]              =         {QSFPDD_NIF_0_7_FUSE_INTR_REG            , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P8_FUSE_INTR]              =         {QSFPDD_NIF_8_15_FUSE_INTR_REG           , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P9_FUSE_INTR]              =         {QSFPDD_NIF_8_15_FUSE_INTR_REG           , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P10_FUSE_INTR]             =         {QSFPDD_NIF_8_15_FUSE_INTR_REG           , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P11_FUSE_INTR]             =         {QSFPDD_NIF_8_15_FUSE_INTR_REG           , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P12_FUSE_INTR]             =         {QSFPDD_NIF_8_15_FUSE_INTR_REG           , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P13_FUSE_INTR]             =         {QSFPDD_NIF_8_15_FUSE_INTR_REG           , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P14_FUSE_INTR]             =         {QSFPDD_NIF_8_15_FUSE_INTR_REG           , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P15_FUSE_INTR]             =         {QSFPDD_NIF_8_15_FUSE_INTR_REG           , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P16_FUSE_INTR]             =         {QSFPDD_NIF_16_19_FUSE_INTR_REG          , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P17_FUSE_INTR]             =         {QSFPDD_NIF_16_19_FUSE_INTR_REG          , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P18_FUSE_INTR]             =         {QSFPDD_NIF_16_19_FUSE_INTR_REG          , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P19_FUSE_INTR]             =         {QSFPDD_NIF_16_19_FUSE_INTR_REG          , MASK_0000_1000, DATA_DEC},

    [QSFPDD_FAB_P0_INTR]                   =         {QSFPDD_FAB_0_7_INTR_REG                 , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P1_INTR]                   =         {QSFPDD_FAB_0_7_INTR_REG                 , MASK_0000_0010, DATA_DEC},
    [QSFPDD_FAB_P2_INTR]                   =         {QSFPDD_FAB_0_7_INTR_REG                 , MASK_0000_0100, DATA_DEC},
    [QSFPDD_FAB_P3_INTR]                   =         {QSFPDD_FAB_0_7_INTR_REG                 , MASK_0000_1000, DATA_DEC},
    [QSFPDD_FAB_P4_INTR]                   =         {QSFPDD_FAB_0_7_INTR_REG                 , MASK_0001_0000, DATA_DEC},
    [QSFPDD_FAB_P5_INTR]                   =         {QSFPDD_FAB_0_7_INTR_REG                 , MASK_0010_0000, DATA_DEC},
    [QSFPDD_FAB_P6_INTR]                   =         {QSFPDD_FAB_0_7_INTR_REG                 , MASK_0100_0000, DATA_DEC},
    [QSFPDD_FAB_P7_INTR]                   =         {QSFPDD_FAB_0_7_INTR_REG                 , MASK_1000_0000, DATA_DEC},
    [QSFPDD_FAB_P8_INTR]                   =         {QSFPDD_FAB_8_9_INTR_REG                 , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P9_INTR]                   =         {QSFPDD_FAB_8_9_INTR_REG                 , MASK_0000_0010, DATA_DEC},

    [QSFPDD_FAB_P0_ABS]                    =         {QSFPDD_FAB_0_7_ABS_REG                  , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P1_ABS]                    =         {QSFPDD_FAB_0_7_ABS_REG                  , MASK_0000_0010, DATA_DEC},
    [QSFPDD_FAB_P2_ABS]                    =         {QSFPDD_FAB_0_7_ABS_REG                  , MASK_0000_0100, DATA_DEC},
    [QSFPDD_FAB_P3_ABS]                    =         {QSFPDD_FAB_0_7_ABS_REG                  , MASK_0000_1000, DATA_DEC},
    [QSFPDD_FAB_P4_ABS]                    =         {QSFPDD_FAB_0_7_ABS_REG                  , MASK_0001_0000, DATA_DEC},
    [QSFPDD_FAB_P5_ABS]                    =         {QSFPDD_FAB_0_7_ABS_REG                  , MASK_0010_0000, DATA_DEC},
    [QSFPDD_FAB_P6_ABS]                    =         {QSFPDD_FAB_0_7_ABS_REG                  , MASK_0100_0000, DATA_DEC},
    [QSFPDD_FAB_P7_ABS]                    =         {QSFPDD_FAB_0_7_ABS_REG                  , MASK_1000_0000, DATA_DEC},
    [QSFPDD_FAB_P8_ABS]                    =         {QSFPDD_FAB_8_9_ABS_REG                  , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P9_ABS]                    =         {QSFPDD_FAB_8_9_ABS_REG                  , MASK_0000_0010, DATA_DEC},

    [QSFPDD_FAB_P0_7_ABS_EVENT]            =         {QSFPDD_FAB_0_7_ABS_EVENT_REG            , MASK_ALL      , DATA_HEX},
    [QSFPDD_FAB_P8_9_ABS_EVENT]            =         {QSFPDD_FAB_8_9_ABS_EVENT_REG            , MASK_ALL      , DATA_HEX},

    [QSFPDD_FAB_P0_FUSE_INTR]              =         {QSFPDD_FAB_0_7_FUSE_INTR_REG            , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P1_FUSE_INTR]              =         {QSFPDD_FAB_0_7_FUSE_INTR_REG            , MASK_0000_0010, DATA_DEC},
    [QSFPDD_FAB_P2_FUSE_INTR]              =         {QSFPDD_FAB_0_7_FUSE_INTR_REG            , MASK_0000_0100, DATA_DEC},
    [QSFPDD_FAB_P3_FUSE_INTR]              =         {QSFPDD_FAB_0_7_FUSE_INTR_REG            , MASK_0000_1000, DATA_DEC},
    [QSFPDD_FAB_P4_FUSE_INTR]              =         {QSFPDD_FAB_0_7_FUSE_INTR_REG            , MASK_0001_0000, DATA_DEC},
    [QSFPDD_FAB_P5_FUSE_INTR]              =         {QSFPDD_FAB_0_7_FUSE_INTR_REG            , MASK_0010_0000, DATA_DEC},
    [QSFPDD_FAB_P6_FUSE_INTR]              =         {QSFPDD_FAB_0_7_FUSE_INTR_REG            , MASK_0100_0000, DATA_DEC},
    [QSFPDD_FAB_P7_FUSE_INTR]              =         {QSFPDD_FAB_0_7_FUSE_INTR_REG            , MASK_1000_0000, DATA_DEC},
    [QSFPDD_FAB_P8_FUSE_INTR]              =         {QSFPDD_FAB_8_9_FUSE_INTR_REG            , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P9_FUSE_INTR]              =         {QSFPDD_FAB_8_9_FUSE_INTR_REG            , MASK_0000_0010, DATA_DEC},

    [QSFPDD_NIF_P0_INTR_MASK]              =         {QSFPDD_NIF_0_7_INTR_MASK_REG            , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P1_INTR_MASK]              =         {QSFPDD_NIF_0_7_INTR_MASK_REG            , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P2_INTR_MASK]              =         {QSFPDD_NIF_0_7_INTR_MASK_REG            , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P3_INTR_MASK]              =         {QSFPDD_NIF_0_7_INTR_MASK_REG            , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P4_INTR_MASK]              =         {QSFPDD_NIF_0_7_INTR_MASK_REG            , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P5_INTR_MASK]              =         {QSFPDD_NIF_0_7_INTR_MASK_REG            , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P6_INTR_MASK]              =         {QSFPDD_NIF_0_7_INTR_MASK_REG            , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P7_INTR_MASK]              =         {QSFPDD_NIF_0_7_INTR_MASK_REG            , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P8_INTR_MASK]              =         {QSFPDD_NIF_8_15_INTR_MASK_REG           , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P9_INTR_MASK]              =         {QSFPDD_NIF_8_15_INTR_MASK_REG           , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P10_INTR_MASK]             =         {QSFPDD_NIF_8_15_INTR_MASK_REG           , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P11_INTR_MASK]             =         {QSFPDD_NIF_8_15_INTR_MASK_REG           , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P12_INTR_MASK]             =         {QSFPDD_NIF_8_15_INTR_MASK_REG           , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P13_INTR_MASK]             =         {QSFPDD_NIF_8_15_INTR_MASK_REG           , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P14_INTR_MASK]             =         {QSFPDD_NIF_8_15_INTR_MASK_REG           , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P15_INTR_MASK]             =         {QSFPDD_NIF_8_15_INTR_MASK_REG           , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P16_INTR_MASK]             =         {QSFPDD_NIF_16_19_INTR_MASK_REG          , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P17_INTR_MASK]             =         {QSFPDD_NIF_16_19_INTR_MASK_REG          , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P18_INTR_MASK]             =         {QSFPDD_NIF_16_19_INTR_MASK_REG          , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P19_INTR_MASK]             =         {QSFPDD_NIF_16_19_INTR_MASK_REG          , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P0_ABS_MASK]               =         {QSFPDD_NIF_0_7_ABS_MASK_REG             , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P1_ABS_MASK]               =         {QSFPDD_NIF_0_7_ABS_MASK_REG             , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P2_ABS_MASK]               =         {QSFPDD_NIF_0_7_ABS_MASK_REG             , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P3_ABS_MASK]               =         {QSFPDD_NIF_0_7_ABS_MASK_REG             , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P4_ABS_MASK]               =         {QSFPDD_NIF_0_7_ABS_MASK_REG             , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P5_ABS_MASK]               =         {QSFPDD_NIF_0_7_ABS_MASK_REG             , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P6_ABS_MASK]               =         {QSFPDD_NIF_0_7_ABS_MASK_REG             , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P7_ABS_MASK]               =         {QSFPDD_NIF_0_7_ABS_MASK_REG             , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P8_ABS_MASK]               =         {QSFPDD_NIF_8_15_ABS_MASK_REG            , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P9_ABS_MASK]               =         {QSFPDD_NIF_8_15_ABS_MASK_REG            , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P10_ABS_MASK]              =         {QSFPDD_NIF_8_15_ABS_MASK_REG            , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P11_ABS_MASK]              =         {QSFPDD_NIF_8_15_ABS_MASK_REG            , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P12_ABS_MASK]              =         {QSFPDD_NIF_8_15_ABS_MASK_REG            , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P13_ABS_MASK]              =         {QSFPDD_NIF_8_15_ABS_MASK_REG            , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P14_ABS_MASK]              =         {QSFPDD_NIF_8_15_ABS_MASK_REG            , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P15_ABS_MASK]              =         {QSFPDD_NIF_8_15_ABS_MASK_REG            , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P16_ABS_MASK]              =         {QSFPDD_NIF_16_19_ABS_MASK_REG           , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P17_ABS_MASK]              =         {QSFPDD_NIF_16_19_ABS_MASK_REG           , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P18_ABS_MASK]              =         {QSFPDD_NIF_16_19_ABS_MASK_REG           , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P19_ABS_MASK]              =         {QSFPDD_NIF_16_19_ABS_MASK_REG           , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P0_FUSE_INTR_MASK]         =         {QSFPDD_NIF_0_7_FUSE_INTR_MASK_REG       , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P1_FUSE_INTR_MASK]         =         {QSFPDD_NIF_0_7_FUSE_INTR_MASK_REG       , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P2_FUSE_INTR_MASK]         =         {QSFPDD_NIF_0_7_FUSE_INTR_MASK_REG       , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P3_FUSE_INTR_MASK]         =         {QSFPDD_NIF_0_7_FUSE_INTR_MASK_REG       , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P4_FUSE_INTR_MASK]         =         {QSFPDD_NIF_0_7_FUSE_INTR_MASK_REG       , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P5_FUSE_INTR_MASK]         =         {QSFPDD_NIF_0_7_FUSE_INTR_MASK_REG       , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P6_FUSE_INTR_MASK]         =         {QSFPDD_NIF_0_7_FUSE_INTR_MASK_REG       , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P7_FUSE_INTR_MASK]         =         {QSFPDD_NIF_0_7_FUSE_INTR_MASK_REG       , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P8_FUSE_INTR_MASK]         =         {QSFPDD_NIF_8_15_FUSE_INTR_MASK_REG      , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P9_FUSE_INTR_MASK]         =         {QSFPDD_NIF_8_15_FUSE_INTR_MASK_REG      , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P10_FUSE_INTR_MASK]        =         {QSFPDD_NIF_8_15_FUSE_INTR_MASK_REG      , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P11_FUSE_INTR_MASK]        =         {QSFPDD_NIF_8_15_FUSE_INTR_MASK_REG      , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P12_FUSE_INTR_MASK]        =         {QSFPDD_NIF_8_15_FUSE_INTR_MASK_REG      , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P13_FUSE_INTR_MASK]        =         {QSFPDD_NIF_8_15_FUSE_INTR_MASK_REG      , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P14_FUSE_INTR_MASK]        =         {QSFPDD_NIF_8_15_FUSE_INTR_MASK_REG      , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P15_FUSE_INTR_MASK]        =         {QSFPDD_NIF_8_15_FUSE_INTR_MASK_REG      , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P16_FUSE_INTR_MASK]        =         {QSFPDD_NIF_16_19_FUSE_INTR_MASK_REG     , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P17_FUSE_INTR_MASK]        =         {QSFPDD_NIF_16_19_FUSE_INTR_MASK_REG     , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P18_FUSE_INTR_MASK]        =         {QSFPDD_NIF_16_19_FUSE_INTR_MASK_REG     , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P19_FUSE_INTR_MASK]        =         {QSFPDD_NIF_16_19_FUSE_INTR_MASK_REG     , MASK_0000_1000, DATA_DEC},

    [QSFPDD_FAB_P0_INTR_MASK]              =         {QSFPDD_FAB_0_7_INTR_MASK_REG            , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P1_INTR_MASK]              =         {QSFPDD_FAB_0_7_INTR_MASK_REG            , MASK_0000_0010, DATA_DEC},
    [QSFPDD_FAB_P2_INTR_MASK]              =         {QSFPDD_FAB_0_7_INTR_MASK_REG            , MASK_0000_0100, DATA_DEC},
    [QSFPDD_FAB_P3_INTR_MASK]              =         {QSFPDD_FAB_0_7_INTR_MASK_REG            , MASK_0000_1000, DATA_DEC},
    [QSFPDD_FAB_P4_INTR_MASK]              =         {QSFPDD_FAB_0_7_INTR_MASK_REG            , MASK_0001_0000, DATA_DEC},
    [QSFPDD_FAB_P5_INTR_MASK]              =         {QSFPDD_FAB_0_7_INTR_MASK_REG            , MASK_0010_0000, DATA_DEC},
    [QSFPDD_FAB_P6_INTR_MASK]              =         {QSFPDD_FAB_0_7_INTR_MASK_REG            , MASK_0100_0000, DATA_DEC},
    [QSFPDD_FAB_P7_INTR_MASK]              =         {QSFPDD_FAB_0_7_INTR_MASK_REG            , MASK_1000_0000, DATA_DEC},
    [QSFPDD_FAB_P8_INTR_MASK]              =         {QSFPDD_FAB_8_9_INTR_MASK_REG            , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P9_INTR_MASK]              =         {QSFPDD_FAB_8_9_INTR_MASK_REG            , MASK_0000_0010, DATA_DEC},

    [QSFPDD_FAB_P0_ABS_MASK]               =         {QSFPDD_FAB_0_7_ABS_MASK_REG             , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P1_ABS_MASK]               =         {QSFPDD_FAB_0_7_ABS_MASK_REG             , MASK_0000_0010, DATA_DEC},
    [QSFPDD_FAB_P2_ABS_MASK]               =         {QSFPDD_FAB_0_7_ABS_MASK_REG             , MASK_0000_0100, DATA_DEC},
    [QSFPDD_FAB_P3_ABS_MASK]               =         {QSFPDD_FAB_0_7_ABS_MASK_REG             , MASK_0000_1000, DATA_DEC},
    [QSFPDD_FAB_P4_ABS_MASK]               =         {QSFPDD_FAB_0_7_ABS_MASK_REG             , MASK_0001_0000, DATA_DEC},
    [QSFPDD_FAB_P5_ABS_MASK]               =         {QSFPDD_FAB_0_7_ABS_MASK_REG             , MASK_0010_0000, DATA_DEC},
    [QSFPDD_FAB_P6_ABS_MASK]               =         {QSFPDD_FAB_0_7_ABS_MASK_REG             , MASK_0100_0000, DATA_DEC},
    [QSFPDD_FAB_P7_ABS_MASK]               =         {QSFPDD_FAB_0_7_ABS_MASK_REG             , MASK_1000_0000, DATA_DEC},
    [QSFPDD_FAB_P8_ABS_MASK]               =         {QSFPDD_FAB_8_9_ABS_MASK_REG             , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P9_ABS_MASK]               =         {QSFPDD_FAB_8_9_ABS_MASK_REG             , MASK_0000_0010, DATA_DEC},

    [QSFPDD_FAB_P0_FUSE_INTR_MASK]         =         {QSFPDD_FAB_0_7_FUSE_INTR_MASK_REG       , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P1_FUSE_INTR_MASK]         =         {QSFPDD_FAB_0_7_FUSE_INTR_MASK_REG       , MASK_0000_0010, DATA_DEC},
    [QSFPDD_FAB_P2_FUSE_INTR_MASK]         =         {QSFPDD_FAB_0_7_FUSE_INTR_MASK_REG       , MASK_0000_0100, DATA_DEC},
    [QSFPDD_FAB_P3_FUSE_INTR_MASK]         =         {QSFPDD_FAB_0_7_FUSE_INTR_MASK_REG       , MASK_0000_1000, DATA_DEC},
    [QSFPDD_FAB_P4_FUSE_INTR_MASK]         =         {QSFPDD_FAB_0_7_FUSE_INTR_MASK_REG       , MASK_0001_0000, DATA_DEC},
    [QSFPDD_FAB_P5_FUSE_INTR_MASK]         =         {QSFPDD_FAB_0_7_FUSE_INTR_MASK_REG       , MASK_0010_0000, DATA_DEC},
    [QSFPDD_FAB_P6_FUSE_INTR_MASK]         =         {QSFPDD_FAB_0_7_FUSE_INTR_MASK_REG       , MASK_0100_0000, DATA_DEC},
    [QSFPDD_FAB_P7_FUSE_INTR_MASK]         =         {QSFPDD_FAB_0_7_FUSE_INTR_MASK_REG       , MASK_1000_0000, DATA_DEC},
    [QSFPDD_FAB_P8_FUSE_INTR_MASK]         =         {QSFPDD_FAB_8_9_FUSE_INTR_MASK_REG       , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P9_FUSE_INTR_MASK]         =         {QSFPDD_FAB_8_9_FUSE_INTR_MASK_REG       , MASK_0000_0010, DATA_DEC},

    [QSFPDD_NIF_P0_RST]                    =         {QSFPDD_NIF_0_7_RST_REG                  , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P1_RST]                    =         {QSFPDD_NIF_0_7_RST_REG                  , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P2_RST]                    =         {QSFPDD_NIF_0_7_RST_REG                  , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P3_RST]                    =         {QSFPDD_NIF_0_7_RST_REG                  , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P4_RST]                    =         {QSFPDD_NIF_0_7_RST_REG                  , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P5_RST]                    =         {QSFPDD_NIF_0_7_RST_REG                  , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P6_RST]                    =         {QSFPDD_NIF_0_7_RST_REG                  , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P7_RST]                    =         {QSFPDD_NIF_0_7_RST_REG                  , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P8_RST]                    =         {QSFPDD_NIF_8_15_RST_REG                 , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P9_RST]                    =         {QSFPDD_NIF_8_15_RST_REG                 , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P10_RST]                   =         {QSFPDD_NIF_8_15_RST_REG                 , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P11_RST]                   =         {QSFPDD_NIF_8_15_RST_REG                 , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P12_RST]                   =         {QSFPDD_NIF_8_15_RST_REG                 , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P13_RST]                   =         {QSFPDD_NIF_8_15_RST_REG                 , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P14_RST]                   =         {QSFPDD_NIF_8_15_RST_REG                 , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P15_RST]                   =         {QSFPDD_NIF_8_15_RST_REG                 , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P16_RST]                   =         {QSFPDD_NIF_16_19_RST_REG                , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P17_RST]                   =         {QSFPDD_NIF_16_19_RST_REG                , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P18_RST]                   =         {QSFPDD_NIF_16_19_RST_REG                , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P19_RST]                   =         {QSFPDD_NIF_16_19_RST_REG                , MASK_0000_1000, DATA_DEC},
    [QSFPDD_FAB_P0_RST]                    =         {QSFPDD_FAB_0_7_RST_REG                  , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P1_RST]                    =         {QSFPDD_FAB_0_7_RST_REG                  , MASK_0000_0010, DATA_DEC},
    [QSFPDD_FAB_P2_RST]                    =         {QSFPDD_FAB_0_7_RST_REG                  , MASK_0000_0100, DATA_DEC},
    [QSFPDD_FAB_P3_RST]                    =         {QSFPDD_FAB_0_7_RST_REG                  , MASK_0000_1000, DATA_DEC},
    [QSFPDD_FAB_P4_RST]                    =         {QSFPDD_FAB_0_7_RST_REG                  , MASK_0001_0000, DATA_DEC},
    [QSFPDD_FAB_P5_RST]                    =         {QSFPDD_FAB_0_7_RST_REG                  , MASK_0010_0000, DATA_DEC},
    [QSFPDD_FAB_P6_RST]                    =         {QSFPDD_FAB_0_7_RST_REG                  , MASK_0100_0000, DATA_DEC},
    [QSFPDD_FAB_P7_RST]                    =         {QSFPDD_FAB_0_7_RST_REG                  , MASK_1000_0000, DATA_DEC},
    [QSFPDD_FAB_P8_RST]                    =         {QSFPDD_FAB_8_9_RST_REG                  , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P9_RST]                    =         {QSFPDD_FAB_8_9_RST_REG                  , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P0_LP_MODE]                =         {QSFPDD_NIF_0_7_LP_REG                   , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P1_LP_MODE]                =         {QSFPDD_NIF_0_7_LP_REG                   , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P2_LP_MODE]                =         {QSFPDD_NIF_0_7_LP_REG                   , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P3_LP_MODE]                =         {QSFPDD_NIF_0_7_LP_REG                   , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P4_LP_MODE]                =         {QSFPDD_NIF_0_7_LP_REG                   , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P5_LP_MODE]                =         {QSFPDD_NIF_0_7_LP_REG                   , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P6_LP_MODE]                =         {QSFPDD_NIF_0_7_LP_REG                   , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P7_LP_MODE]                =         {QSFPDD_NIF_0_7_LP_REG                   , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P8_LP_MODE]                =         {QSFPDD_NIF_8_15_LP_REG                  , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P9_LP_MODE]                =         {QSFPDD_NIF_8_15_LP_REG                  , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P10_LP_MODE]               =         {QSFPDD_NIF_8_15_LP_REG                  , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P11_LP_MODE]               =         {QSFPDD_NIF_8_15_LP_REG                  , MASK_0000_1000, DATA_DEC},
    [QSFPDD_NIF_P12_LP_MODE]               =         {QSFPDD_NIF_8_15_LP_REG                  , MASK_0001_0000, DATA_DEC},
    [QSFPDD_NIF_P13_LP_MODE]               =         {QSFPDD_NIF_8_15_LP_REG                  , MASK_0010_0000, DATA_DEC},
    [QSFPDD_NIF_P14_LP_MODE]               =         {QSFPDD_NIF_8_15_LP_REG                  , MASK_0100_0000, DATA_DEC},
    [QSFPDD_NIF_P15_LP_MODE]               =         {QSFPDD_NIF_8_15_LP_REG                  , MASK_1000_0000, DATA_DEC},
    [QSFPDD_NIF_P16_LP_MODE]               =         {QSFPDD_NIF_16_19_LP_REG                 , MASK_0000_0001, DATA_DEC},
    [QSFPDD_NIF_P17_LP_MODE]               =         {QSFPDD_NIF_16_19_LP_REG                 , MASK_0000_0010, DATA_DEC},
    [QSFPDD_NIF_P18_LP_MODE]               =         {QSFPDD_NIF_16_19_LP_REG                 , MASK_0000_0100, DATA_DEC},
    [QSFPDD_NIF_P19_LP_MODE]               =         {QSFPDD_NIF_16_19_LP_REG                 , MASK_0000_1000, DATA_DEC},
    [QSFPDD_FAB_P0_LP_MODE]                =         {QSFPDD_FAB_0_7_LP_REG                   , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P1_LP_MODE]                =         {QSFPDD_FAB_0_7_LP_REG                   , MASK_0000_0010, DATA_DEC},
    [QSFPDD_FAB_P2_LP_MODE]                =         {QSFPDD_FAB_0_7_LP_REG                   , MASK_0000_0100, DATA_DEC},
    [QSFPDD_FAB_P3_LP_MODE]                =         {QSFPDD_FAB_0_7_LP_REG                   , MASK_0000_1000, DATA_DEC},
    [QSFPDD_FAB_P4_LP_MODE]                =         {QSFPDD_FAB_0_7_LP_REG                   , MASK_0001_0000, DATA_DEC},
    [QSFPDD_FAB_P5_LP_MODE]                =         {QSFPDD_FAB_0_7_LP_REG                   , MASK_0010_0000, DATA_DEC},
    [QSFPDD_FAB_P6_LP_MODE]                =         {QSFPDD_FAB_0_7_LP_REG                   , MASK_0100_0000, DATA_DEC},
    [QSFPDD_FAB_P7_LP_MODE]                =         {QSFPDD_FAB_0_7_LP_REG                   , MASK_1000_0000, DATA_DEC},
    [QSFPDD_FAB_P8_LP_MODE]                =         {QSFPDD_FAB_8_9_LP_REG                   , MASK_0000_0001, DATA_DEC},
    [QSFPDD_FAB_P9_LP_MODE]                =         {QSFPDD_FAB_8_9_LP_REG                   , MASK_0000_0010, DATA_DEC},
    [QSFPDD_FAB_P0_LED_STATUS]             =         {QSFPDD_FAB_LED_0_1_STATUS_REG           , MASK_0000_1111, DATA_DEC},    
    [QSFPDD_FAB_P1_LED_STATUS]             =         {QSFPDD_FAB_LED_0_1_STATUS_REG           , MASK_1111_0000, DATA_DEC},
    [QSFPDD_FAB_P2_LED_STATUS]             =         {QSFPDD_FAB_LED_2_3_STATUS_REG           , MASK_0000_1111, DATA_DEC},
    [QSFPDD_FAB_P3_LED_STATUS]             =         {QSFPDD_FAB_LED_2_3_STATUS_REG           , MASK_1111_0000, DATA_DEC},
    [QSFPDD_FAB_P4_LED_STATUS]             =         {QSFPDD_FAB_LED_4_5_STATUS_REG           , MASK_0000_1111, DATA_DEC},
    [QSFPDD_FAB_P5_LED_STATUS]             =         {QSFPDD_FAB_LED_4_5_STATUS_REG           , MASK_1111_0000, DATA_DEC},
    [QSFPDD_FAB_P6_LED_STATUS]             =         {QSFPDD_FAB_LED_6_7_STATUS_REG           , MASK_0000_1111, DATA_DEC},
    [QSFPDD_FAB_P7_LED_STATUS]             =         {QSFPDD_FAB_LED_6_7_STATUS_REG           , MASK_1111_0000, DATA_DEC},
    [QSFPDD_FAB_P8_LED_STATUS]             =         {QSFPDD_FAB_LED_8_9_STATUS_REG           , MASK_0000_1111, DATA_DEC},
    [QSFPDD_FAB_P9_LED_STATUS]             =         {QSFPDD_FAB_LED_8_9_STATUS_REG           , MASK_1111_0000, DATA_DEC},

    [QSFPDD_NIF_P0_I2C_STUCK]              =         {QSFPDD_NIF_0_7_STUCK_REG                , MASK_0000_0001, DATA_DEC}, 
    [QSFPDD_NIF_P1_I2C_STUCK]              =         {QSFPDD_NIF_0_7_STUCK_REG                , MASK_0000_0010, DATA_DEC}, 
    [QSFPDD_NIF_P2_I2C_STUCK]              =         {QSFPDD_NIF_0_7_STUCK_REG                , MASK_0000_0100, DATA_DEC}, 
    [QSFPDD_NIF_P3_I2C_STUCK]              =         {QSFPDD_NIF_0_7_STUCK_REG                , MASK_0000_1000, DATA_DEC}, 
    [QSFPDD_NIF_P4_I2C_STUCK]              =         {QSFPDD_NIF_0_7_STUCK_REG                , MASK_0001_0000, DATA_DEC}, 
    [QSFPDD_NIF_P5_I2C_STUCK]              =         {QSFPDD_NIF_0_7_STUCK_REG                , MASK_0010_0000, DATA_DEC}, 
    [QSFPDD_NIF_P6_I2C_STUCK]              =         {QSFPDD_NIF_0_7_STUCK_REG                , MASK_0100_0000, DATA_DEC}, 
    [QSFPDD_NIF_P7_I2C_STUCK]              =         {QSFPDD_NIF_0_7_STUCK_REG                , MASK_1000_0000, DATA_DEC}, 
    [QSFPDD_NIF_P8_I2C_STUCK]              =         {QSFPDD_NIF_8_15_STUCK_REG               , MASK_0000_0001, DATA_DEC}, 
    [QSFPDD_NIF_P9_I2C_STUCK]              =         {QSFPDD_NIF_8_15_STUCK_REG               , MASK_0000_0010, DATA_DEC}, 
    [QSFPDD_NIF_P10_I2C_STUCK]             =         {QSFPDD_NIF_8_15_STUCK_REG               , MASK_0000_0100, DATA_DEC},  
    [QSFPDD_NIF_P11_I2C_STUCK]             =         {QSFPDD_NIF_8_15_STUCK_REG               , MASK_0000_1000, DATA_DEC},  
    [QSFPDD_NIF_P12_I2C_STUCK]             =         {QSFPDD_NIF_8_15_STUCK_REG               , MASK_0001_0000, DATA_DEC},  
    [QSFPDD_NIF_P13_I2C_STUCK]             =         {QSFPDD_NIF_8_15_STUCK_REG               , MASK_0010_0000, DATA_DEC},  
    [QSFPDD_NIF_P14_I2C_STUCK]             =         {QSFPDD_NIF_8_15_STUCK_REG               , MASK_0100_0000, DATA_DEC},  
    [QSFPDD_NIF_P15_I2C_STUCK]             =         {QSFPDD_NIF_8_15_STUCK_REG               , MASK_1000_0000, DATA_DEC},  
    [QSFPDD_NIF_P16_I2C_STUCK]             =         {QSFPDD_NIF_16_19_STUCK_REG              , MASK_0000_0001, DATA_DEC},  
    [QSFPDD_NIF_P17_I2C_STUCK]             =         {QSFPDD_NIF_16_19_STUCK_REG              , MASK_0000_0010, DATA_DEC},  
    [QSFPDD_NIF_P18_I2C_STUCK]             =         {QSFPDD_NIF_16_19_STUCK_REG              , MASK_0000_0100, DATA_DEC},  
    [QSFPDD_NIF_P19_I2C_STUCK]             =         {QSFPDD_NIF_16_19_STUCK_REG              , MASK_0000_1000, DATA_DEC},  
    [QSFPDD_FAB_P0_I2C_STUCK]              =         {QSFPDD_FAB_0_7_STUCK_REG                , MASK_0000_0001, DATA_DEC}, 
    [QSFPDD_FAB_P1_I2C_STUCK]              =         {QSFPDD_FAB_0_7_STUCK_REG                , MASK_0000_0010, DATA_DEC}, 
    [QSFPDD_FAB_P2_I2C_STUCK]              =         {QSFPDD_FAB_0_7_STUCK_REG                , MASK_0000_0100, DATA_DEC}, 
    [QSFPDD_FAB_P3_I2C_STUCK]              =         {QSFPDD_FAB_0_7_STUCK_REG                , MASK_0000_1000, DATA_DEC}, 
    [QSFPDD_FAB_P4_I2C_STUCK]              =         {QSFPDD_FAB_0_7_STUCK_REG                , MASK_0001_0000, DATA_DEC}, 
    [QSFPDD_FAB_P5_I2C_STUCK]              =         {QSFPDD_FAB_0_7_STUCK_REG                , MASK_0010_0000, DATA_DEC}, 
    [QSFPDD_FAB_P6_I2C_STUCK]              =         {QSFPDD_FAB_0_7_STUCK_REG                , MASK_0100_0000, DATA_DEC}, 
    [QSFPDD_FAB_P7_I2C_STUCK]              =         {QSFPDD_FAB_0_7_STUCK_REG                , MASK_1000_0000, DATA_DEC}, 
    [QSFPDD_FAB_P8_I2C_STUCK]              =         {QSFPDD_FAB_8_9_STUCK_REG                , MASK_0000_0001, DATA_DEC}, 
    [QSFPDD_FAB_P9_I2C_STUCK]              =         {QSFPDD_FAB_8_9_STUCK_REG                , MASK_0000_0010, DATA_DEC}, 

    // CPLD 5

    // MUX
    [IDLE_STATE]                           =         {NONE_REG                                , MASK_NONE     , DATA_UNK},

    //BSP DEBUG
    [BSP_DEBUG]                            =         {NONE_REG                                , MASK_NONE     , DATA_UNK},
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
int _cpld_reg_read(struct device *dev, u8 reg, u8 mask);
int _cpld_reg_read_nolock(struct device *dev, u8 reg, u8 mask);
int _cpld_reg_write_nolock(struct device *dev, u8 reg, u8 reg_val);
static ssize_t cpld_reg_read(struct device *dev, char *buf, u8 reg, u8 mask, u8 data_type);
static ssize_t cpld_reg_write(struct device *dev, const char *buf, size_t count, u8 reg, u8 mask);
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
    { "s9720_56ed_cpld1",  cpld1 },
    { "s9720_56ed_cpld2",  cpld2 },
    { "s9720_56ed_fpga" ,  fpga  },
    { "s9720_56ed_cpld4",  cpld4 },
    { "s9720_56ed_cpld5",  cpld5 },
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

// CPLD 1
static SENSOR_DEVICE_ATTR_RO(cpld_sku_id                   , cpld, CPLD_SKU_ID);
static SENSOR_DEVICE_ATTR_RO(cpld_hw_rev                   , cpld, CPLD_HW_REV);
static SENSOR_DEVICE_ATTR_RO(cpld_deph_rev                 , cpld, CPLD_DEPH_REV);
static SENSOR_DEVICE_ATTR_RO(cpld_build_rev                , cpld, CPLD_BUILD_REV);
static SENSOR_DEVICE_ATTR_RO(cpld_brd_id_type              , cpld, CPLD_BRD_ID_TYPE);
static SENSOR_DEVICE_ATTR_RO(cpld_chip_type                , cpld, CPLD_CHIP_TYPE);
static SENSOR_DEVICE_ATTR_RO(mac_intr                      , cpld, MAC_INTR);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_0_intr                , cpld, RGB_PHY_0_INTR);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_1_intr                , cpld, RGB_PHY_1_INTR);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_2_intr                , cpld, RGB_PHY_2_INTR);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_3_intr                , cpld, RGB_PHY_3_INTR);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_4_intr                , cpld, RGB_PHY_4_INTR);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_5_intr                , cpld, RGB_PHY_5_INTR);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_6_intr                , cpld, RGB_PHY_6_INTR);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_7_intr                , cpld, RGB_PHY_7_INTR);
static SENSOR_DEVICE_ATTR_RO(top_rgb_intr                  , cpld, TOP_RGB_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_25gphy_intr              , cpld, CPLD_25GPHY_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_2_fru_intr               , cpld, CPLD_2_FRU_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_4_fru_intr               , cpld, CPLD_4_FRU_INTR);

static SENSOR_DEVICE_ATTR_RO(sfp28_intr                    , cpld, SFP28_INTR);
static SENSOR_DEVICE_ATTR_RO(psu_0_intr                    , cpld, PSU_0_INTR);
static SENSOR_DEVICE_ATTR_RO(psu_1_intr                    , cpld, PSU_1_INTR);
static SENSOR_DEVICE_ATTR_RO(retimer_intr                  , cpld, RETIMER_INTR);

static SENSOR_DEVICE_ATTR_RO(mac_hbm_temp_alert            , cpld, MAC_HBM_TEMP_ALERT);
static SENSOR_DEVICE_ATTR_RO(mb_lm75_temp_alert            , cpld, MB_LM75_TEMP_ALERT);
static SENSOR_DEVICE_ATTR_RO(ext_lm75_temp_alert           , cpld, EXT_LM75_TEMP_ALERT);
static SENSOR_DEVICE_ATTR_RO(phy_lm75_cpld5_intr           , cpld, PHY_LM75_CPLD5_INTR);
static SENSOR_DEVICE_ATTR_RO(usb_ocp                       , cpld, USB_OCP);
static SENSOR_DEVICE_ATTR_RO(fpga_intr                     , cpld, FPGA_INTR);
static SENSOR_DEVICE_ATTR_RO(lm75_bmc_intr                 , cpld, LM75_BMC_INTR);
static SENSOR_DEVICE_ATTR_RO(lm75_cpld1_intr               , cpld, LM75_CPLD1_INTR);
static SENSOR_DEVICE_ATTR_RO(cpu_intr                      , cpld, CPU_INTR);
static SENSOR_DEVICE_ATTR_RO(lm75_cpld245_intr             , cpld, LM75_CPLD245_INTR);

static SENSOR_DEVICE_ATTR_RO(mac_intr_mask                 , cpld, MAC_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_0_intr_mask           , cpld, RGB_PHY_0_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_1_intr_mask           , cpld, RGB_PHY_1_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_2_intr_mask           , cpld, RGB_PHY_2_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_3_intr_mask           , cpld, RGB_PHY_3_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_4_intr_mask           , cpld, RGB_PHY_4_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_5_intr_mask           , cpld, RGB_PHY_5_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_6_intr_mask           , cpld, RGB_PHY_6_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_7_intr_mask           , cpld, RGB_PHY_7_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(top_rgb_intr_mask             , cpld, TOP_RGB_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(misc_intr                     , cpld, MISC_INTR);
static SENSOR_DEVICE_ATTR_RO(system_intr                   , cpld, SYSTEM_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_25gphy_intr_mask         , cpld, CPLD_25GPHY_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(cpld_2_fru_intr_mask          , cpld, CPLD_2_FRU_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(cpld_4_fru_intr_mask          , cpld, CPLD_4_FRU_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(sfp28_intr_mask               , cpld, SFP28_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(retimer_intr_mask             , cpld, RETIMER_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(psu_0_intr_mask               , cpld, PSU_0_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(psu_1_intr_mask               , cpld, PSU_1_INTR_MASK);

static SENSOR_DEVICE_ATTR_RO(usb_ocp_intr_mask             , cpld, USB_OCP_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(mac_temp_alert_mask           , cpld, MAC_TEMP_ALERT_MASK);
static SENSOR_DEVICE_ATTR_RO(mb_lm75_temp_alert_mask       , cpld, MB_LM75_TEMP_ALERT_MASK);
static SENSOR_DEVICE_ATTR_RO(mb_temp_0_alert_mask          , cpld, MB_TEMP_1_ALERT_MASK);
static SENSOR_DEVICE_ATTR_RO(mb_temp_1_alert_mask          , cpld, MB_TEMP_2_ALERT_MASK);
static SENSOR_DEVICE_ATTR_RO(mb_temp_2_alert_mask          , cpld, MB_TEMP_3_ALERT_MASK);
static SENSOR_DEVICE_ATTR_RO(mb_temp_3_alert_mask          , cpld, MB_TEMP_4_ALERT_MASK);
static SENSOR_DEVICE_ATTR_RO(mb_temp_4_alert_mask          , cpld, MB_TEMP_5_ALERT_MASK);
static SENSOR_DEVICE_ATTR_RO(mb_temp_5_alert_mask          , cpld, MB_TEMP_6_ALERT_MASK);
static SENSOR_DEVICE_ATTR_RO(mb_temp_6_alert_mask          , cpld, MB_TEMP_7_ALERT_MASK);
static SENSOR_DEVICE_ATTR_RO(top_cpld_5_intr_mask          , cpld, TOP_CPLD_5_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(usb_ocp_mask                  , cpld, USB_OCP_MASK);
static SENSOR_DEVICE_ATTR_RO(fpga_intr_mask                , cpld, FPGA_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(lm75_bmc_intr_mask            , cpld, LM75_BMC_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(lm75_cpld1_intr_mask          , cpld, LM75_CPLD1_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(cpu_intr_mask                 , cpld, CPU_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(lm75_cpld245_intr_mask        , cpld, LM75_CPLD245_INTR_MASK);

static SENSOR_DEVICE_ATTR_RO(mac_intr_event                , cpld, MAC_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(mb_rgb_0_7_intr_event         , cpld, MB_RGB_0_7_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(top_rgb_intr_event            , cpld, TOP_RGB_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_25gphy_intr_event        , cpld, CPLD_25GPHY_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_fru_intr_event           , cpld, CPLD_FRU_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(mac_temp_alert_event          , cpld, MAC_TEMP_ALERT_EVENT);
static SENSOR_DEVICE_ATTR_RO(temp_alert_event              , cpld, TEMP_ALERT_EVENT);
static SENSOR_DEVICE_ATTR_RO(misc_intr_event               , cpld, MISC_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RW(event_detect_ctrl             , cpld, EVENT_DETECT_CTRL);
static SENSOR_DEVICE_ATTR_RO(mac_pcie_rst                  , cpld, MAC_PCIE_RST);
static SENSOR_DEVICE_ATTR_RO(mac_qspi_rst                  , cpld, MAC_QSPI_RST);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_0_rst                 , cpld, RGB_PHY_0_RST);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_1_rst                 , cpld, RGB_PHY_1_RST);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_2_rst                 , cpld, RGB_PHY_2_RST);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_3_rst                 , cpld, RGB_PHY_3_RST);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_4_rst                 , cpld, RGB_PHY_4_RST);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_5_rst                 , cpld, RGB_PHY_5_RST);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_6_rst                 , cpld, RGB_PHY_6_RST);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_7_rst                 , cpld, RGB_PHY_7_RST);
static SENSOR_DEVICE_ATTR_RW(ntm_rst                       , cpld, NTM_RST);
static SENSOR_DEVICE_ATTR_RW(bmc_rst                       , cpld, BMC_RST);
static SENSOR_DEVICE_ATTR_RO(usb_redriver_rst              , cpld, USB_REDRIVER_RST);
static SENSOR_DEVICE_ATTR_RO(usb_ocp_rcvry                 , cpld, USB_OCP_RCVRY);

static SENSOR_DEVICE_ATTR_RO(cpld_2_rst                    , cpld, CPLD_2_RST);
static SENSOR_DEVICE_ATTR_RO(fpga_rst                      , cpld, FPGA_RST);
static SENSOR_DEVICE_ATTR_RO(cpld_4_rst                    , cpld, CPLD_4_RST);
static SENSOR_DEVICE_ATTR_RO(cpld_5_rst                    , cpld, CPLD_5_RST);

static SENSOR_DEVICE_ATTR_RO(cpld_ioexp_rst                , cpld, CPLD_IOEXP_RST);
static SENSOR_DEVICE_ATTR_RO(fan_i2c_mux_rst               , cpld, FAN_I2C_MUX_RST);
static SENSOR_DEVICE_ATTR_RO(cpld_25gphy_rst               , cpld, CPLD_25GPHY_RST);
static SENSOR_DEVICE_ATTR_RO(i210_dev                      , cpld, I210_DEV);
static SENSOR_DEVICE_ATTR_RO(lan_pg                        , cpld, LAN_PG);
static SENSOR_DEVICE_ATTR_RO(i210_rst                      , cpld, I210_RST);
static SENSOR_DEVICE_ATTR_RO(top_i2c_mux_rst               , cpld, TOP_I2C_MUX_RST);
static SENSOR_DEVICE_ATTR_RO(rc32312_1_rst                 , cpld, RC32312_1_RST);
static SENSOR_DEVICE_ATTR_RO(rc32312_2_rst                 , cpld, RC32312_2_RST);
static SENSOR_DEVICE_ATTR_RO(rc32312_3_rst                 , cpld, RC32312_3_RST);

static SENSOR_DEVICE_ATTR_RO(cpu_prsnt                     , cpld, CPU_PRSNT);
static SENSOR_DEVICE_ATTR_RO(bmc_prsnt                     , cpld, BMC_PRSNT);
static SENSOR_DEVICE_ATTR_RO(ntm_prsnt                     , cpld, NTM_PRSNT);
static SENSOR_DEVICE_ATTR_RO(usb_prsnt                     , cpld, USB_PRSNT);
static SENSOR_DEVICE_ATTR_RO(ext_prsnt                     , cpld, EXT_PRSNT);
static SENSOR_DEVICE_ATTR_RO(top_prsnt                     , cpld, TOP_PRSNT);

static SENSOR_DEVICE_ATTR_RO(psu_0_prsnt                   , cpld, PSU_0_PRSNT);
static SENSOR_DEVICE_ATTR_RO(psu_1_prsnt                   , cpld, PSU_1_PRSNT);
static SENSOR_DEVICE_ATTR_RO(psu_0_acin                    , cpld, PSU_0_ACIN);
static SENSOR_DEVICE_ATTR_RO(psu_1_acin                    , cpld, PSU_1_ACIN);
static SENSOR_DEVICE_ATTR_RO(psu_0_pg                      , cpld, PSU_0_PG);
static SENSOR_DEVICE_ATTR_RO(psu_1_pg                      , cpld, PSU_1_PG);

static SENSOR_DEVICE_ATTR_RO(cpu_pg                        , cpld, CPU_PG);
static SENSOR_DEVICE_ATTR_RO(mb_cpu_pg                     , cpld, MB_CPU_PG);
static SENSOR_DEVICE_ATTR_RO(bmc_pg                        , cpld, BMC_PG);
static SENSOR_DEVICE_ATTR_RO(top_pg                        , cpld, TOP_PG);

static SENSOR_DEVICE_ATTR_RO(rgb_phy_0_serboot             , cpld, RGB_PHY_0_SERBOOT);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_1_serboot             , cpld, RGB_PHY_1_SERBOOT);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_2_serboot             , cpld, RGB_PHY_2_SERBOOT);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_3_serboot             , cpld, RGB_PHY_3_SERBOOT);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_4_serboot             , cpld, RGB_PHY_4_SERBOOT);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_5_serboot             , cpld, RGB_PHY_5_SERBOOT);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_6_serboot             , cpld, RGB_PHY_6_SERBOOT);
static SENSOR_DEVICE_ATTR_RO(rgb_phy_7_serboot             , cpld, RGB_PHY_7_SERBOOT);

static SENSOR_DEVICE_ATTR_RO(cpu_mux_sel                   , cpld, CPU_MUX_SEL);
static SENSOR_DEVICE_ATTR_RO(psu_mux_sel                   , cpld, PSU_MUX_SEL);
static SENSOR_DEVICE_ATTR_RO(fpga_qspi_sel                 , cpld, FPGA_QSPI_SEL);
static SENSOR_DEVICE_ATTR_RO(uart_mux2_sel                 , cpld, UART_MUX2_SEL);

static SENSOR_DEVICE_ATTR_RW(system_led_status             , cpld, SYSTEM_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(fan_led_status                , cpld, FAN_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(psu_0_led_status              , cpld, PSU_0_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(psu_1_led_status              , cpld, PSU_1_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(sync_led_status               , cpld, SYNC_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(sfp_0_led_status              , cpld, SFP_0_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(sfp_1_led_status              , cpld, SFP_1_LED_STATUS);

// CPLD 2
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p20_intr           , cpld, QSFPDD_NIF_P20_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p21_intr           , cpld, QSFPDD_NIF_P21_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p22_intr           , cpld, QSFPDD_NIF_P22_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p23_intr           , cpld, QSFPDD_NIF_P23_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p24_intr           , cpld, QSFPDD_NIF_P24_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p25_intr           , cpld, QSFPDD_NIF_P25_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p26_intr           , cpld, QSFPDD_NIF_P26_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p27_intr           , cpld, QSFPDD_NIF_P27_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p28_intr           , cpld, QSFPDD_NIF_P28_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p29_intr           , cpld, QSFPDD_NIF_P29_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p30_intr           , cpld, QSFPDD_NIF_P30_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p31_intr           , cpld, QSFPDD_NIF_P31_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p32_intr           , cpld, QSFPDD_NIF_P32_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p33_intr           , cpld, QSFPDD_NIF_P33_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p34_intr           , cpld, QSFPDD_NIF_P34_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p35_intr           , cpld, QSFPDD_NIF_P35_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p20_abs            , cpld, QSFPDD_NIF_P20_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p21_abs            , cpld, QSFPDD_NIF_P21_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p22_abs            , cpld, QSFPDD_NIF_P22_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p23_abs            , cpld, QSFPDD_NIF_P23_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p24_abs            , cpld, QSFPDD_NIF_P24_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p25_abs            , cpld, QSFPDD_NIF_P25_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p26_abs            , cpld, QSFPDD_NIF_P26_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p27_abs            , cpld, QSFPDD_NIF_P27_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p28_abs            , cpld, QSFPDD_NIF_P28_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p29_abs            , cpld, QSFPDD_NIF_P29_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p30_abs            , cpld, QSFPDD_NIF_P30_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p31_abs            , cpld, QSFPDD_NIF_P31_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p32_abs            , cpld, QSFPDD_NIF_P32_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p33_abs            , cpld, QSFPDD_NIF_P33_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p34_abs            , cpld, QSFPDD_NIF_P34_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p35_abs            , cpld, QSFPDD_NIF_P35_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p20_27_abs_event   , cpld, QSFPDD_NIF_P20_27_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p28_35_abs_event   , cpld, QSFPDD_NIF_P28_35_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p20_fuse_intr      , cpld, QSFPDD_NIF_P20_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p21_fuse_intr      , cpld, QSFPDD_NIF_P21_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p22_fuse_intr      , cpld, QSFPDD_NIF_P22_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p23_fuse_intr      , cpld, QSFPDD_NIF_P23_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p24_fuse_intr      , cpld, QSFPDD_NIF_P24_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p25_fuse_intr      , cpld, QSFPDD_NIF_P25_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p26_fuse_intr      , cpld, QSFPDD_NIF_P26_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p27_fuse_intr      , cpld, QSFPDD_NIF_P27_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p28_fuse_intr      , cpld, QSFPDD_NIF_P28_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p29_fuse_intr      , cpld, QSFPDD_NIF_P29_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p30_fuse_intr      , cpld, QSFPDD_NIF_P30_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p31_fuse_intr      , cpld, QSFPDD_NIF_P31_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p32_fuse_intr      , cpld, QSFPDD_NIF_P32_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p33_fuse_intr      , cpld, QSFPDD_NIF_P33_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p34_fuse_intr      , cpld, QSFPDD_NIF_P34_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p35_fuse_intr      , cpld, QSFPDD_NIF_P35_FUSE_INTR);

static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p10_intr           , cpld, QSFPDD_FAB_P10_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p11_intr           , cpld, QSFPDD_FAB_P11_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p12_intr           , cpld, QSFPDD_FAB_P12_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p13_intr           , cpld, QSFPDD_FAB_P13_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p14_intr           , cpld, QSFPDD_FAB_P14_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p15_intr           , cpld, QSFPDD_FAB_P15_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p16_intr           , cpld, QSFPDD_FAB_P16_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p17_intr           , cpld, QSFPDD_FAB_P17_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p18_intr           , cpld, QSFPDD_FAB_P18_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p19_intr           , cpld, QSFPDD_FAB_P19_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p10_abs            , cpld, QSFPDD_FAB_P10_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p11_abs            , cpld, QSFPDD_FAB_P11_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p12_abs            , cpld, QSFPDD_FAB_P12_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p13_abs            , cpld, QSFPDD_FAB_P13_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p14_abs            , cpld, QSFPDD_FAB_P14_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p15_abs            , cpld, QSFPDD_FAB_P15_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p16_abs            , cpld, QSFPDD_FAB_P16_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p17_abs            , cpld, QSFPDD_FAB_P17_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p18_abs            , cpld, QSFPDD_FAB_P18_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p19_abs            , cpld, QSFPDD_FAB_P19_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p10_17_abs_event   , cpld, QSFPDD_FAB_P10_17_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p18_19_abs_event   , cpld, QSFPDD_FAB_P18_19_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p10_fuse_intr      , cpld, QSFPDD_FAB_P10_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p11_fuse_intr      , cpld, QSFPDD_FAB_P11_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p12_fuse_intr      , cpld, QSFPDD_FAB_P12_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p13_fuse_intr      , cpld, QSFPDD_FAB_P13_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p14_fuse_intr      , cpld, QSFPDD_FAB_P14_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p15_fuse_intr      , cpld, QSFPDD_FAB_P15_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p16_fuse_intr      , cpld, QSFPDD_FAB_P16_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p17_fuse_intr      , cpld, QSFPDD_FAB_P17_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p18_fuse_intr      , cpld, QSFPDD_FAB_P18_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p19_fuse_intr      , cpld, QSFPDD_FAB_P19_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p20_intr_mask      , cpld, QSFPDD_NIF_P20_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p21_intr_mask      , cpld, QSFPDD_NIF_P21_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p22_intr_mask      , cpld, QSFPDD_NIF_P22_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p23_intr_mask      , cpld, QSFPDD_NIF_P23_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p24_intr_mask      , cpld, QSFPDD_NIF_P24_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p25_intr_mask      , cpld, QSFPDD_NIF_P25_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p26_intr_mask      , cpld, QSFPDD_NIF_P26_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p27_intr_mask      , cpld, QSFPDD_NIF_P27_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p28_intr_mask      , cpld, QSFPDD_NIF_P28_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p29_intr_mask      , cpld, QSFPDD_NIF_P29_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p30_intr_mask      , cpld, QSFPDD_NIF_P30_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p31_intr_mask      , cpld, QSFPDD_NIF_P31_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p32_intr_mask      , cpld, QSFPDD_NIF_P32_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p33_intr_mask      , cpld, QSFPDD_NIF_P33_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p34_intr_mask      , cpld, QSFPDD_NIF_P34_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p35_intr_mask      , cpld, QSFPDD_NIF_P35_INTR_MASK);

static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p20_abs_mask       , cpld, QSFPDD_NIF_P20_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p21_abs_mask       , cpld, QSFPDD_NIF_P21_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p22_abs_mask       , cpld, QSFPDD_NIF_P22_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p23_abs_mask       , cpld, QSFPDD_NIF_P23_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p24_abs_mask       , cpld, QSFPDD_NIF_P24_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p25_abs_mask       , cpld, QSFPDD_NIF_P25_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p26_abs_mask       , cpld, QSFPDD_NIF_P26_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p27_abs_mask       , cpld, QSFPDD_NIF_P27_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p28_abs_mask       , cpld, QSFPDD_NIF_P28_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p29_abs_mask       , cpld, QSFPDD_NIF_P29_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p30_abs_mask       , cpld, QSFPDD_NIF_P30_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p31_abs_mask       , cpld, QSFPDD_NIF_P31_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p32_abs_mask       , cpld, QSFPDD_NIF_P32_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p33_abs_mask       , cpld, QSFPDD_NIF_P33_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p34_abs_mask       , cpld, QSFPDD_NIF_P34_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p35_abs_mask       , cpld, QSFPDD_NIF_P35_ABS_MASK);

static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p20_fuse_intr_mask , cpld, QSFPDD_NIF_P20_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p21_fuse_intr_mask , cpld, QSFPDD_NIF_P21_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p22_fuse_intr_mask , cpld, QSFPDD_NIF_P22_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p23_fuse_intr_mask , cpld, QSFPDD_NIF_P23_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p24_fuse_intr_mask , cpld, QSFPDD_NIF_P24_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p25_fuse_intr_mask , cpld, QSFPDD_NIF_P25_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p26_fuse_intr_mask , cpld, QSFPDD_NIF_P26_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p27_fuse_intr_mask , cpld, QSFPDD_NIF_P27_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p28_fuse_intr_mask , cpld, QSFPDD_NIF_P28_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p29_fuse_intr_mask , cpld, QSFPDD_NIF_P29_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p30_fuse_intr_mask , cpld, QSFPDD_NIF_P30_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p31_fuse_intr_mask , cpld, QSFPDD_NIF_P31_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p32_fuse_intr_mask , cpld, QSFPDD_NIF_P32_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p33_fuse_intr_mask , cpld, QSFPDD_NIF_P33_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p34_fuse_intr_mask , cpld, QSFPDD_NIF_P34_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p35_fuse_intr_mask , cpld, QSFPDD_NIF_P35_FUSE_INTR_MASK);

static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p10_intr_mask      , cpld, QSFPDD_FAB_P10_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p11_intr_mask      , cpld, QSFPDD_FAB_P11_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p12_intr_mask      , cpld, QSFPDD_FAB_P12_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p13_intr_mask      , cpld, QSFPDD_FAB_P13_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p14_intr_mask      , cpld, QSFPDD_FAB_P14_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p15_intr_mask      , cpld, QSFPDD_FAB_P15_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p16_intr_mask      , cpld, QSFPDD_FAB_P16_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p17_intr_mask      , cpld, QSFPDD_FAB_P17_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p18_intr_mask      , cpld, QSFPDD_FAB_P18_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p19_intr_mask      , cpld, QSFPDD_FAB_P19_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p10_abs_mask       , cpld, QSFPDD_FAB_P10_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p11_abs_mask       , cpld, QSFPDD_FAB_P11_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p12_abs_mask       , cpld, QSFPDD_FAB_P12_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p13_abs_mask       , cpld, QSFPDD_FAB_P13_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p14_abs_mask       , cpld, QSFPDD_FAB_P14_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p15_abs_mask       , cpld, QSFPDD_FAB_P15_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p16_abs_mask       , cpld, QSFPDD_FAB_P16_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p17_abs_mask       , cpld, QSFPDD_FAB_P17_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p18_abs_mask       , cpld, QSFPDD_FAB_P18_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p19_abs_mask       , cpld, QSFPDD_FAB_P19_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p10_fuse_intr_mask , cpld, QSFPDD_FAB_P10_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p11_fuse_intr_mask , cpld, QSFPDD_FAB_P11_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p12_fuse_intr_mask , cpld, QSFPDD_FAB_P12_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p13_fuse_intr_mask , cpld, QSFPDD_FAB_P13_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p14_fuse_intr_mask , cpld, QSFPDD_FAB_P14_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p15_fuse_intr_mask , cpld, QSFPDD_FAB_P15_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p16_fuse_intr_mask , cpld, QSFPDD_FAB_P16_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p17_fuse_intr_mask , cpld, QSFPDD_FAB_P17_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p18_fuse_intr_mask , cpld, QSFPDD_FAB_P18_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p19_fuse_intr_mask , cpld, QSFPDD_FAB_P19_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p20_rst            , cpld, QSFPDD_NIF_P20_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p21_rst            , cpld, QSFPDD_NIF_P21_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p22_rst            , cpld, QSFPDD_NIF_P22_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p23_rst            , cpld, QSFPDD_NIF_P23_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p24_rst            , cpld, QSFPDD_NIF_P24_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p25_rst            , cpld, QSFPDD_NIF_P25_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p26_rst            , cpld, QSFPDD_NIF_P26_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p27_rst            , cpld, QSFPDD_NIF_P27_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p28_rst            , cpld, QSFPDD_NIF_P28_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p29_rst            , cpld, QSFPDD_NIF_P29_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p30_rst            , cpld, QSFPDD_NIF_P30_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p31_rst            , cpld, QSFPDD_NIF_P31_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p32_rst            , cpld, QSFPDD_NIF_P32_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p33_rst            , cpld, QSFPDD_NIF_P33_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p34_rst            , cpld, QSFPDD_NIF_P34_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p35_rst            , cpld, QSFPDD_NIF_P35_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p10_rst            , cpld, QSFPDD_FAB_P10_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p11_rst            , cpld, QSFPDD_FAB_P11_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p12_rst            , cpld, QSFPDD_FAB_P12_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p13_rst            , cpld, QSFPDD_FAB_P13_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p14_rst            , cpld, QSFPDD_FAB_P14_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p15_rst            , cpld, QSFPDD_FAB_P15_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p16_rst            , cpld, QSFPDD_FAB_P16_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p17_rst            , cpld, QSFPDD_FAB_P17_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p18_rst            , cpld, QSFPDD_FAB_P18_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p19_rst            , cpld, QSFPDD_FAB_P19_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p20_lp_mode        , cpld, QSFPDD_NIF_P20_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p21_lp_mode        , cpld, QSFPDD_NIF_P21_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p22_lp_mode        , cpld, QSFPDD_NIF_P22_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p23_lp_mode        , cpld, QSFPDD_NIF_P23_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p24_lp_mode        , cpld, QSFPDD_NIF_P24_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p25_lp_mode        , cpld, QSFPDD_NIF_P25_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p26_lp_mode        , cpld, QSFPDD_NIF_P26_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p27_lp_mode        , cpld, QSFPDD_NIF_P27_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p28_lp_mode        , cpld, QSFPDD_NIF_P28_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p29_lp_mode        , cpld, QSFPDD_NIF_P29_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p30_lp_mode        , cpld, QSFPDD_NIF_P30_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p31_lp_mode        , cpld, QSFPDD_NIF_P31_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p32_lp_mode        , cpld, QSFPDD_NIF_P32_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p33_lp_mode        , cpld, QSFPDD_NIF_P33_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p34_lp_mode        , cpld, QSFPDD_NIF_P34_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p35_lp_mode        , cpld, QSFPDD_NIF_P35_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p10_lp_mode        , cpld, QSFPDD_FAB_P10_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p11_lp_mode        , cpld, QSFPDD_FAB_P11_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p12_lp_mode        , cpld, QSFPDD_FAB_P12_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p13_lp_mode        , cpld, QSFPDD_FAB_P13_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p14_lp_mode        , cpld, QSFPDD_FAB_P14_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p15_lp_mode        , cpld, QSFPDD_FAB_P15_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p16_lp_mode        , cpld, QSFPDD_FAB_P16_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p17_lp_mode        , cpld, QSFPDD_FAB_P17_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p18_lp_mode        , cpld, QSFPDD_FAB_P18_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p19_lp_mode        , cpld, QSFPDD_FAB_P19_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p10_led_status     , cpld, QSFPDD_FAB_P10_LED_STATUS);    
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p11_led_status     , cpld, QSFPDD_FAB_P11_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p12_led_status     , cpld, QSFPDD_FAB_P12_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p13_led_status     , cpld, QSFPDD_FAB_P13_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p14_led_status     , cpld, QSFPDD_FAB_P14_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p15_led_status     , cpld, QSFPDD_FAB_P15_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p16_led_status     , cpld, QSFPDD_FAB_P16_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p17_led_status     , cpld, QSFPDD_FAB_P17_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p18_led_status     , cpld, QSFPDD_FAB_P18_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p19_led_status     , cpld, QSFPDD_FAB_P19_LED_STATUS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p10_i2c_stuck      , cpld, QSFPDD_FAB_P10_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p11_i2c_stuck      , cpld, QSFPDD_FAB_P11_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p12_i2c_stuck      , cpld, QSFPDD_FAB_P12_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p13_i2c_stuck      , cpld, QSFPDD_FAB_P13_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p14_i2c_stuck      , cpld, QSFPDD_FAB_P14_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p15_i2c_stuck      , cpld, QSFPDD_FAB_P15_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p16_i2c_stuck      , cpld, QSFPDD_FAB_P16_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p17_i2c_stuck      , cpld, QSFPDD_FAB_P17_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p18_i2c_stuck      , cpld, QSFPDD_FAB_P18_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p19_i2c_stuck      , cpld, QSFPDD_FAB_P19_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p20_i2c_stuck      , cpld, QSFPDD_NIF_P20_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p21_i2c_stuck      , cpld, QSFPDD_NIF_P21_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p22_i2c_stuck      , cpld, QSFPDD_NIF_P22_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p23_i2c_stuck      , cpld, QSFPDD_NIF_P23_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p24_i2c_stuck      , cpld, QSFPDD_NIF_P24_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p25_i2c_stuck      , cpld, QSFPDD_NIF_P25_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p26_i2c_stuck      , cpld, QSFPDD_NIF_P26_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p27_i2c_stuck      , cpld, QSFPDD_NIF_P27_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p28_i2c_stuck      , cpld, QSFPDD_NIF_P28_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p29_i2c_stuck      , cpld, QSFPDD_NIF_P29_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p30_i2c_stuck      , cpld, QSFPDD_NIF_P30_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p31_i2c_stuck      , cpld, QSFPDD_NIF_P31_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p32_i2c_stuck      , cpld, QSFPDD_NIF_P32_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p33_i2c_stuck      , cpld, QSFPDD_NIF_P33_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p34_i2c_stuck      , cpld, QSFPDD_NIF_P34_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p35_i2c_stuck      , cpld, QSFPDD_NIF_P35_I2C_STUCK);
 // CPLD 3
 // Change to FPGA
static SENSOR_DEVICE_ATTR_RO(fpga_id                       , cpld, FPGA_ID);
static SENSOR_DEVICE_ATTR_RO(fpga_ver_1                    , cpld, FPGA_VER_1);
static SENSOR_DEVICE_ATTR_RO(fpga_minor_ver                , cpld, FPGA_MINOR_VER);
static SENSOR_DEVICE_ATTR_RO(fpga_major_ver                , cpld, FPGA_MAJOR_VER);
static SENSOR_DEVICE_ATTR_RO(fpga_build_ver                , cpld, FPGA_BUILD_VER);
static SENSOR_DEVICE_ATTR_RO(fpga_version_h                , version_h, FPGA_VERSION_H);
static SENSOR_DEVICE_ATTR_RO(fpga_dev_info                 , cpld, FPGA_DEV_INFO);
static SENSOR_DEVICE_ATTR_RW(sfp28_p37_ts                  , cpld, SFP28_P37_TS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p36_ts                  , cpld, SFP28_P36_TS);
static SENSOR_DEVICE_ATTR_RW(mgmt_p1_ts                    , cpld, MGMT_P1_TS);
static SENSOR_DEVICE_ATTR_RW(mgmt_p0_ts                    , cpld, MGMT_P0_TS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p37_rs                  , cpld, SFP28_P37_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p36_rs                  , cpld, SFP28_P36_RS);
static SENSOR_DEVICE_ATTR_RW(mgmt_p1_rs                    , cpld, MGMT_P1_RS);
static SENSOR_DEVICE_ATTR_RW(mgmt_p0_rs                    , cpld, MGMT_P0_RS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p37_tx_dis              , cpld, SFP28_P37_TX_DIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p36_tx_dis              , cpld, SFP28_P36_TX_DIS);
static SENSOR_DEVICE_ATTR_RW(mgmt_p1_tx_dis                , cpld, MGMT_P1_TX_DIS);
static SENSOR_DEVICE_ATTR_RW(mgmt_p0_tx_dis                , cpld, MGMT_P0_TX_DIS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p37_tx_flt              , cpld, SFP28_P37_TX_FLT);
static SENSOR_DEVICE_ATTR_RW(sfp28_p36_tx_flt              , cpld, SFP28_P36_TX_FLT);
static SENSOR_DEVICE_ATTR_RW(mgmt_p1_tx_flt                , cpld, MGMT_P1_TX_FLT);
static SENSOR_DEVICE_ATTR_RW(mgmt_p0_tx_flt                , cpld, MGMT_P0_TX_FLT);
static SENSOR_DEVICE_ATTR_RW(sfp28_p37_rx_los              , cpld, SFP28_P37_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p36_rx_los              , cpld, SFP28_P36_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(mgmt_p1_rx_los                , cpld, MGMT_P1_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(mgmt_p0_rx_los                , cpld, MGMT_P0_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p37_abs                 , cpld, SFP28_P37_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p36_abs                 , cpld, SFP28_P36_ABS);
static SENSOR_DEVICE_ATTR_RW(mgmt_p1_abs                   , cpld, MGMT_P1_ABS);
static SENSOR_DEVICE_ATTR_RW(mgmt_p0_abs                   , cpld, MGMT_P0_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp28_p37_tx_flt_mask         , cpld, SFP28_P37_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(sfp28_p36_tx_flt_mask         , cpld, SFP28_P36_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(mgmt_p1_tx_flt_mask           , cpld, MGMT_P1_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(mgmt_p0_tx_flt_mask           , cpld, MGMT_P0_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(sfp28_p37_rx_los_mask         , cpld, SFP28_P37_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(sfp28_p36_rx_los_mask         , cpld, SFP28_P36_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(mgmt_p1_rx_los_mask           , cpld, MGMT_P1_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(mgmt_p0_rx_los_mask           , cpld, MGMT_P0_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(sfp28_p37_abs_mask            , cpld, SFP28_P37_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(sfp28_p36_abs_mask            , cpld, SFP28_P36_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(mgmt_p1_abs_mask              , cpld, MGMT_P1_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(mgmt_p0_abs_mask              , cpld, MGMT_P0_ABS_MASK);
static SENSOR_DEVICE_ATTR_RO(sfp28_tx_flt_event            , cpld, SFP28_TX_FLT_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp28_rx_los_event            , cpld, SFP28_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp28_abs_event               , cpld, SFP28_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(mgmt_p0_i2c_stuck             , cpld, MGMT_P0_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(mgmt_p1_i2c_stuck             , cpld, MGMT_P1_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p36_i2c_stuck           , cpld, SFP28_P36_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(sfp28_p37_i2c_stuck           , cpld, SFP28_P37_I2C_STUCK);

 // CPLD 4
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p0_intr            , cpld, QSFPDD_NIF_P0_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p1_intr            , cpld, QSFPDD_NIF_P1_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p2_intr            , cpld, QSFPDD_NIF_P2_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p3_intr            , cpld, QSFPDD_NIF_P3_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p4_intr            , cpld, QSFPDD_NIF_P4_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p5_intr            , cpld, QSFPDD_NIF_P5_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p6_intr            , cpld, QSFPDD_NIF_P6_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p7_intr            , cpld, QSFPDD_NIF_P7_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p8_intr            , cpld, QSFPDD_NIF_P8_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p9_intr            , cpld, QSFPDD_NIF_P9_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p10_intr           , cpld, QSFPDD_NIF_P10_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p11_intr           , cpld, QSFPDD_NIF_P11_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p12_intr           , cpld, QSFPDD_NIF_P12_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p13_intr           , cpld, QSFPDD_NIF_P13_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p14_intr           , cpld, QSFPDD_NIF_P14_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p15_intr           , cpld, QSFPDD_NIF_P15_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p16_intr           , cpld, QSFPDD_NIF_P16_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p17_intr           , cpld, QSFPDD_NIF_P17_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p18_intr           , cpld, QSFPDD_NIF_P18_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p19_intr           , cpld, QSFPDD_NIF_P19_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p0_abs             , cpld, QSFPDD_NIF_P0_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p1_abs             , cpld, QSFPDD_NIF_P1_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p2_abs             , cpld, QSFPDD_NIF_P2_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p3_abs             , cpld, QSFPDD_NIF_P3_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p4_abs             , cpld, QSFPDD_NIF_P4_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p5_abs             , cpld, QSFPDD_NIF_P5_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p6_abs             , cpld, QSFPDD_NIF_P6_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p7_abs             , cpld, QSFPDD_NIF_P7_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p8_abs             , cpld, QSFPDD_NIF_P8_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p9_abs             , cpld, QSFPDD_NIF_P9_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p10_abs            , cpld, QSFPDD_NIF_P10_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p11_abs            , cpld, QSFPDD_NIF_P11_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p12_abs            , cpld, QSFPDD_NIF_P12_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p13_abs            , cpld, QSFPDD_NIF_P13_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p14_abs            , cpld, QSFPDD_NIF_P14_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p15_abs            , cpld, QSFPDD_NIF_P15_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p16_abs            , cpld, QSFPDD_NIF_P16_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p17_abs            , cpld, QSFPDD_NIF_P17_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p18_abs            , cpld, QSFPDD_NIF_P18_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p19_abs            , cpld, QSFPDD_NIF_P19_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p0_7_abs_event     , cpld, QSFPDD_NIF_P0_7_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p8_15_abs_event    , cpld, QSFPDD_NIF_P8_15_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p16_19_abs_event   , cpld, QSFPDD_NIF_P16_19_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p0_fuse_intr       , cpld, QSFPDD_NIF_P0_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p1_fuse_intr       , cpld, QSFPDD_NIF_P1_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p2_fuse_intr       , cpld, QSFPDD_NIF_P2_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p3_fuse_intr       , cpld, QSFPDD_NIF_P3_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p4_fuse_intr       , cpld, QSFPDD_NIF_P4_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p5_fuse_intr       , cpld, QSFPDD_NIF_P5_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p6_fuse_intr       , cpld, QSFPDD_NIF_P6_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p7_fuse_intr       , cpld, QSFPDD_NIF_P7_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p8_fuse_intr       , cpld, QSFPDD_NIF_P8_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p9_fuse_intr       , cpld, QSFPDD_NIF_P9_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p10_fuse_intr      , cpld, QSFPDD_NIF_P10_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p11_fuse_intr      , cpld, QSFPDD_NIF_P11_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p12_fuse_intr      , cpld, QSFPDD_NIF_P12_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p13_fuse_intr      , cpld, QSFPDD_NIF_P13_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p14_fuse_intr      , cpld, QSFPDD_NIF_P14_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p15_fuse_intr      , cpld, QSFPDD_NIF_P15_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p16_fuse_intr      , cpld, QSFPDD_NIF_P16_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p17_fuse_intr      , cpld, QSFPDD_NIF_P17_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p18_fuse_intr      , cpld, QSFPDD_NIF_P18_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p19_fuse_intr      , cpld, QSFPDD_NIF_P19_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p0_intr            , cpld, QSFPDD_FAB_P0_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p1_intr            , cpld, QSFPDD_FAB_P1_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p2_intr            , cpld, QSFPDD_FAB_P2_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p3_intr            , cpld, QSFPDD_FAB_P3_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p4_intr            , cpld, QSFPDD_FAB_P4_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p5_intr            , cpld, QSFPDD_FAB_P5_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p6_intr            , cpld, QSFPDD_FAB_P6_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p7_intr            , cpld, QSFPDD_FAB_P7_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p8_intr            , cpld, QSFPDD_FAB_P8_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p9_intr            , cpld, QSFPDD_FAB_P9_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p0_abs             , cpld, QSFPDD_FAB_P0_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p1_abs             , cpld, QSFPDD_FAB_P1_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p2_abs             , cpld, QSFPDD_FAB_P2_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p3_abs             , cpld, QSFPDD_FAB_P3_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p4_abs             , cpld, QSFPDD_FAB_P4_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p5_abs             , cpld, QSFPDD_FAB_P5_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p6_abs             , cpld, QSFPDD_FAB_P6_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p7_abs             , cpld, QSFPDD_FAB_P7_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p8_abs             , cpld, QSFPDD_FAB_P8_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p9_abs             , cpld, QSFPDD_FAB_P9_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p0_7_abs_event     , cpld, QSFPDD_FAB_P0_7_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p8_9_abs_event     , cpld, QSFPDD_FAB_P8_9_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p0_fuse_intr       , cpld, QSFPDD_FAB_P0_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p1_fuse_intr       , cpld, QSFPDD_FAB_P1_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p2_fuse_intr       , cpld, QSFPDD_FAB_P2_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p3_fuse_intr       , cpld, QSFPDD_FAB_P3_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p4_fuse_intr       , cpld, QSFPDD_FAB_P4_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p5_fuse_intr       , cpld, QSFPDD_FAB_P5_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p6_fuse_intr       , cpld, QSFPDD_FAB_P6_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p7_fuse_intr       , cpld, QSFPDD_FAB_P7_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p8_fuse_intr       , cpld, QSFPDD_FAB_P8_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p9_fuse_intr       , cpld, QSFPDD_FAB_P9_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p0_intr_mask       , cpld, QSFPDD_NIF_P0_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p1_intr_mask       , cpld, QSFPDD_NIF_P1_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p2_intr_mask       , cpld, QSFPDD_NIF_P2_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p3_intr_mask       , cpld, QSFPDD_NIF_P3_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p4_intr_mask       , cpld, QSFPDD_NIF_P4_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p5_intr_mask       , cpld, QSFPDD_NIF_P5_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p6_intr_mask       , cpld, QSFPDD_NIF_P6_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p7_intr_mask       , cpld, QSFPDD_NIF_P7_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p8_intr_mask       , cpld, QSFPDD_NIF_P8_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p9_intr_mask       , cpld, QSFPDD_NIF_P9_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p10_intr_mask      , cpld, QSFPDD_NIF_P10_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p11_intr_mask      , cpld, QSFPDD_NIF_P11_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p12_intr_mask      , cpld, QSFPDD_NIF_P12_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p13_intr_mask      , cpld, QSFPDD_NIF_P13_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p14_intr_mask      , cpld, QSFPDD_NIF_P14_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p15_intr_mask      , cpld, QSFPDD_NIF_P15_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p16_intr_mask      , cpld, QSFPDD_NIF_P16_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p17_intr_mask      , cpld, QSFPDD_NIF_P17_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p18_intr_mask      , cpld, QSFPDD_NIF_P18_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p19_intr_mask      , cpld, QSFPDD_NIF_P19_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p0_abs_mask        , cpld, QSFPDD_NIF_P0_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p1_abs_mask        , cpld, QSFPDD_NIF_P1_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p2_abs_mask        , cpld, QSFPDD_NIF_P2_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p3_abs_mask        , cpld, QSFPDD_NIF_P3_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p4_abs_mask        , cpld, QSFPDD_NIF_P4_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p5_abs_mask        , cpld, QSFPDD_NIF_P5_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p6_abs_mask        , cpld, QSFPDD_NIF_P6_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p7_abs_mask        , cpld, QSFPDD_NIF_P7_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p8_abs_mask        , cpld, QSFPDD_NIF_P8_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p9_abs_mask        , cpld, QSFPDD_NIF_P9_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p10_abs_mask       , cpld, QSFPDD_NIF_P10_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p11_abs_mask       , cpld, QSFPDD_NIF_P11_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p12_abs_mask       , cpld, QSFPDD_NIF_P12_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p13_abs_mask       , cpld, QSFPDD_NIF_P13_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p14_abs_mask       , cpld, QSFPDD_NIF_P14_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p15_abs_mask       , cpld, QSFPDD_NIF_P15_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p16_abs_mask       , cpld, QSFPDD_NIF_P16_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p17_abs_mask       , cpld, QSFPDD_NIF_P17_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p18_abs_mask       , cpld, QSFPDD_NIF_P18_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p19_abs_mask       , cpld, QSFPDD_NIF_P19_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p0_fuse_intr_mask  , cpld, QSFPDD_NIF_P0_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p1_fuse_intr_mask  , cpld, QSFPDD_NIF_P1_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p2_fuse_intr_mask  , cpld, QSFPDD_NIF_P2_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p3_fuse_intr_mask  , cpld, QSFPDD_NIF_P3_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p4_fuse_intr_mask  , cpld, QSFPDD_NIF_P4_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p5_fuse_intr_mask  , cpld, QSFPDD_NIF_P5_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p6_fuse_intr_mask  , cpld, QSFPDD_NIF_P6_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p7_fuse_intr_mask  , cpld, QSFPDD_NIF_P7_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p8_fuse_intr_mask  , cpld, QSFPDD_NIF_P8_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p9_fuse_intr_mask  , cpld, QSFPDD_NIF_P9_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p10_fuse_intr_mask , cpld, QSFPDD_NIF_P10_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p11_fuse_intr_mask , cpld, QSFPDD_NIF_P11_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p12_fuse_intr_mask , cpld, QSFPDD_NIF_P12_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p13_fuse_intr_mask , cpld, QSFPDD_NIF_P13_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p14_fuse_intr_mask , cpld, QSFPDD_NIF_P14_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p15_fuse_intr_mask , cpld, QSFPDD_NIF_P15_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p16_fuse_intr_mask , cpld, QSFPDD_NIF_P16_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p17_fuse_intr_mask , cpld, QSFPDD_NIF_P17_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p18_fuse_intr_mask , cpld, QSFPDD_NIF_P18_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p19_fuse_intr_mask , cpld, QSFPDD_NIF_P19_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p0_intr_mask       , cpld, QSFPDD_FAB_P0_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p1_intr_mask       , cpld, QSFPDD_FAB_P1_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p2_intr_mask       , cpld, QSFPDD_FAB_P2_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p3_intr_mask       , cpld, QSFPDD_FAB_P3_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p4_intr_mask       , cpld, QSFPDD_FAB_P4_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p5_intr_mask       , cpld, QSFPDD_FAB_P5_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p6_intr_mask       , cpld, QSFPDD_FAB_P6_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p7_intr_mask       , cpld, QSFPDD_FAB_P7_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p8_intr_mask       , cpld, QSFPDD_FAB_P8_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p9_intr_mask       , cpld, QSFPDD_FAB_P9_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p0_abs_mask        , cpld, QSFPDD_FAB_P0_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p1_abs_mask        , cpld, QSFPDD_FAB_P1_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p2_abs_mask        , cpld, QSFPDD_FAB_P2_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p3_abs_mask        , cpld, QSFPDD_FAB_P3_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p4_abs_mask        , cpld, QSFPDD_FAB_P4_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p5_abs_mask        , cpld, QSFPDD_FAB_P5_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p6_abs_mask        , cpld, QSFPDD_FAB_P6_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p7_abs_mask        , cpld, QSFPDD_FAB_P7_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p8_abs_mask        , cpld, QSFPDD_FAB_P8_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p9_abs_mask        , cpld, QSFPDD_FAB_P9_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p0_fuse_intr_mask  , cpld, QSFPDD_FAB_P0_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p1_fuse_intr_mask  , cpld, QSFPDD_FAB_P1_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p2_fuse_intr_mask  , cpld, QSFPDD_FAB_P2_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p3_fuse_intr_mask  , cpld, QSFPDD_FAB_P3_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p4_fuse_intr_mask  , cpld, QSFPDD_FAB_P4_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p5_fuse_intr_mask  , cpld, QSFPDD_FAB_P5_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p6_fuse_intr_mask  , cpld, QSFPDD_FAB_P6_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p7_fuse_intr_mask  , cpld, QSFPDD_FAB_P7_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p8_fuse_intr_mask  , cpld, QSFPDD_FAB_P8_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p9_fuse_intr_mask  , cpld, QSFPDD_FAB_P9_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p0_rst             , cpld, QSFPDD_NIF_P0_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p1_rst             , cpld, QSFPDD_NIF_P1_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p2_rst             , cpld, QSFPDD_NIF_P2_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p3_rst             , cpld, QSFPDD_NIF_P3_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p4_rst             , cpld, QSFPDD_NIF_P4_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p5_rst             , cpld, QSFPDD_NIF_P5_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p6_rst             , cpld, QSFPDD_NIF_P6_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p7_rst             , cpld, QSFPDD_NIF_P7_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p8_rst             , cpld, QSFPDD_NIF_P8_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p9_rst             , cpld, QSFPDD_NIF_P9_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p10_rst            , cpld, QSFPDD_NIF_P10_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p11_rst            , cpld, QSFPDD_NIF_P11_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p12_rst            , cpld, QSFPDD_NIF_P12_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p13_rst            , cpld, QSFPDD_NIF_P13_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p14_rst            , cpld, QSFPDD_NIF_P14_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p15_rst            , cpld, QSFPDD_NIF_P15_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p16_rst            , cpld, QSFPDD_NIF_P16_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p17_rst            , cpld, QSFPDD_NIF_P17_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p18_rst            , cpld, QSFPDD_NIF_P18_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p19_rst            , cpld, QSFPDD_NIF_P19_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p0_rst             , cpld, QSFPDD_FAB_P0_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p1_rst             , cpld, QSFPDD_FAB_P1_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p2_rst             , cpld, QSFPDD_FAB_P2_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p3_rst             , cpld, QSFPDD_FAB_P3_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p4_rst             , cpld, QSFPDD_FAB_P4_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p5_rst             , cpld, QSFPDD_FAB_P5_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p6_rst             , cpld, QSFPDD_FAB_P6_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p7_rst             , cpld, QSFPDD_FAB_P7_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p8_rst             , cpld, QSFPDD_FAB_P8_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p9_rst             , cpld, QSFPDD_FAB_P9_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p0_lp_mode         , cpld, QSFPDD_NIF_P0_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p1_lp_mode         , cpld, QSFPDD_NIF_P1_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p2_lp_mode         , cpld, QSFPDD_NIF_P2_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p3_lp_mode         , cpld, QSFPDD_NIF_P3_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p4_lp_mode         , cpld, QSFPDD_NIF_P4_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p5_lp_mode         , cpld, QSFPDD_NIF_P5_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p6_lp_mode         , cpld, QSFPDD_NIF_P6_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p7_lp_mode         , cpld, QSFPDD_NIF_P7_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p8_lp_mode         , cpld, QSFPDD_NIF_P8_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p9_lp_mode         , cpld, QSFPDD_NIF_P9_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p10_lp_mode        , cpld, QSFPDD_NIF_P10_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p11_lp_mode        , cpld, QSFPDD_NIF_P11_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p12_lp_mode        , cpld, QSFPDD_NIF_P12_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p13_lp_mode        , cpld, QSFPDD_NIF_P13_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p14_lp_mode        , cpld, QSFPDD_NIF_P14_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p15_lp_mode        , cpld, QSFPDD_NIF_P15_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p16_lp_mode        , cpld, QSFPDD_NIF_P16_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p17_lp_mode        , cpld, QSFPDD_NIF_P17_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p18_lp_mode        , cpld, QSFPDD_NIF_P18_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_nif_p19_lp_mode        , cpld, QSFPDD_NIF_P19_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p0_lp_mode         , cpld, QSFPDD_FAB_P0_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p1_lp_mode         , cpld, QSFPDD_FAB_P1_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p2_lp_mode         , cpld, QSFPDD_FAB_P2_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p3_lp_mode         , cpld, QSFPDD_FAB_P3_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p4_lp_mode         , cpld, QSFPDD_FAB_P4_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p5_lp_mode         , cpld, QSFPDD_FAB_P5_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p6_lp_mode         , cpld, QSFPDD_FAB_P6_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p7_lp_mode         , cpld, QSFPDD_FAB_P7_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p8_lp_mode         , cpld, QSFPDD_FAB_P8_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p9_lp_mode         , cpld, QSFPDD_FAB_P9_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p0_led_status      , cpld, QSFPDD_FAB_P0_LED_STATUS);    
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p1_led_status      , cpld, QSFPDD_FAB_P1_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p2_led_status      , cpld, QSFPDD_FAB_P2_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p3_led_status      , cpld, QSFPDD_FAB_P3_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p4_led_status      , cpld, QSFPDD_FAB_P4_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p5_led_status      , cpld, QSFPDD_FAB_P5_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p6_led_status      , cpld, QSFPDD_FAB_P6_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p7_led_status      , cpld, QSFPDD_FAB_P7_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p8_led_status      , cpld, QSFPDD_FAB_P8_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_fab_p9_led_status      , cpld, QSFPDD_FAB_P9_LED_STATUS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p0_i2c_stuck       , cpld, QSFPDD_NIF_P0_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p1_i2c_stuck       , cpld, QSFPDD_NIF_P1_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p2_i2c_stuck       , cpld, QSFPDD_NIF_P2_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p3_i2c_stuck       , cpld, QSFPDD_NIF_P3_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p4_i2c_stuck       , cpld, QSFPDD_NIF_P4_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p5_i2c_stuck       , cpld, QSFPDD_NIF_P5_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p6_i2c_stuck       , cpld, QSFPDD_NIF_P6_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p7_i2c_stuck       , cpld, QSFPDD_NIF_P7_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p8_i2c_stuck       , cpld, QSFPDD_NIF_P8_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p9_i2c_stuck       , cpld, QSFPDD_NIF_P9_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p10_i2c_stuck       , cpld, QSFPDD_NIF_P10_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p11_i2c_stuck       , cpld, QSFPDD_NIF_P11_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p12_i2c_stuck       , cpld, QSFPDD_NIF_P12_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p13_i2c_stuck       , cpld, QSFPDD_NIF_P13_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p14_i2c_stuck       , cpld, QSFPDD_NIF_P14_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p15_i2c_stuck       , cpld, QSFPDD_NIF_P15_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p16_i2c_stuck       , cpld, QSFPDD_NIF_P16_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p17_i2c_stuck       , cpld, QSFPDD_NIF_P17_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p18_i2c_stuck       , cpld, QSFPDD_NIF_P18_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_nif_p19_i2c_stuck       , cpld, QSFPDD_NIF_P19_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p0_i2c_stuck        , cpld, QSFPDD_FAB_P0_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p1_i2c_stuck        , cpld, QSFPDD_FAB_P1_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p2_i2c_stuck        , cpld, QSFPDD_FAB_P2_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p3_i2c_stuck        , cpld, QSFPDD_FAB_P3_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p4_i2c_stuck        , cpld, QSFPDD_FAB_P4_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p5_i2c_stuck        , cpld, QSFPDD_FAB_P5_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p6_i2c_stuck        , cpld, QSFPDD_FAB_P6_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p7_i2c_stuck        , cpld, QSFPDD_FAB_P7_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p8_i2c_stuck        , cpld, QSFPDD_FAB_P8_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_fab_p9_i2c_stuck        , cpld, QSFPDD_FAB_P9_I2C_STUCK);

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
    // CPLD 1
    _DEVICE_ATTR(cpld_sku_id),
    _DEVICE_ATTR(cpld_hw_rev),
    _DEVICE_ATTR(cpld_deph_rev),
    _DEVICE_ATTR(cpld_build_rev),
    _DEVICE_ATTR(cpld_brd_id_type),
    _DEVICE_ATTR(cpld_chip_type),
    _DEVICE_ATTR(mac_intr),
    _DEVICE_ATTR(rgb_phy_0_intr),
    _DEVICE_ATTR(rgb_phy_1_intr),
    _DEVICE_ATTR(rgb_phy_2_intr),
    _DEVICE_ATTR(rgb_phy_3_intr),
    _DEVICE_ATTR(rgb_phy_4_intr),
    _DEVICE_ATTR(rgb_phy_5_intr),
    _DEVICE_ATTR(rgb_phy_6_intr),
    _DEVICE_ATTR(rgb_phy_7_intr),
    _DEVICE_ATTR(top_rgb_intr),
    _DEVICE_ATTR(cpld_25gphy_intr),
    _DEVICE_ATTR(cpld_2_fru_intr),
    _DEVICE_ATTR(cpld_4_fru_intr),
    _DEVICE_ATTR(sfp28_intr),
    _DEVICE_ATTR(psu_0_intr),
    _DEVICE_ATTR(psu_1_intr),
    _DEVICE_ATTR(retimer_intr),
    _DEVICE_ATTR(mac_hbm_temp_alert),
    _DEVICE_ATTR(mb_lm75_temp_alert),
    _DEVICE_ATTR(ext_lm75_temp_alert),
    _DEVICE_ATTR(phy_lm75_cpld5_intr),
    _DEVICE_ATTR(usb_ocp),
    _DEVICE_ATTR(fpga_intr),
    _DEVICE_ATTR(lm75_bmc_intr),
    _DEVICE_ATTR(lm75_cpld1_intr),
    _DEVICE_ATTR(cpu_intr),
    _DEVICE_ATTR(lm75_cpld245_intr),
    _DEVICE_ATTR(mac_intr_mask),
    _DEVICE_ATTR(rgb_phy_0_intr_mask),
    _DEVICE_ATTR(rgb_phy_1_intr_mask),
    _DEVICE_ATTR(rgb_phy_2_intr_mask),
    _DEVICE_ATTR(rgb_phy_3_intr_mask),
    _DEVICE_ATTR(rgb_phy_4_intr_mask),
    _DEVICE_ATTR(rgb_phy_5_intr_mask),
    _DEVICE_ATTR(rgb_phy_6_intr_mask),
    _DEVICE_ATTR(rgb_phy_7_intr_mask),
    _DEVICE_ATTR(top_rgb_intr_mask),
    _DEVICE_ATTR(misc_intr),
    _DEVICE_ATTR(system_intr),
    _DEVICE_ATTR(cpld_25gphy_intr_mask),
    _DEVICE_ATTR(cpld_2_fru_intr_mask),
    _DEVICE_ATTR(cpld_4_fru_intr_mask),
    _DEVICE_ATTR(sfp28_intr_mask),
    _DEVICE_ATTR(retimer_intr_mask),
    _DEVICE_ATTR(psu_0_intr_mask),
    _DEVICE_ATTR(psu_1_intr_mask),
    _DEVICE_ATTR(usb_ocp_intr_mask),
    _DEVICE_ATTR(mac_temp_alert_mask),
    _DEVICE_ATTR(mb_lm75_temp_alert_mask),
    _DEVICE_ATTR(mb_temp_0_alert_mask),
    _DEVICE_ATTR(mb_temp_1_alert_mask),
    _DEVICE_ATTR(mb_temp_2_alert_mask),
    _DEVICE_ATTR(mb_temp_3_alert_mask),
    _DEVICE_ATTR(mb_temp_4_alert_mask),
    _DEVICE_ATTR(mb_temp_5_alert_mask),
    _DEVICE_ATTR(mb_temp_6_alert_mask),
    _DEVICE_ATTR(top_cpld_5_intr_mask),
    _DEVICE_ATTR(usb_ocp_mask),
    _DEVICE_ATTR(fpga_intr_mask),
    _DEVICE_ATTR(lm75_bmc_intr_mask),
    _DEVICE_ATTR(lm75_cpld1_intr_mask),
    _DEVICE_ATTR(cpu_intr_mask),
    _DEVICE_ATTR(lm75_cpld245_intr_mask),
    _DEVICE_ATTR(mac_intr_event),
    _DEVICE_ATTR(mb_rgb_0_7_intr_event),
    _DEVICE_ATTR(top_rgb_intr_event),
    _DEVICE_ATTR(cpld_25gphy_intr_event),
    _DEVICE_ATTR(cpld_fru_intr_event),
    _DEVICE_ATTR(mac_temp_alert_event),
    _DEVICE_ATTR(temp_alert_event),
    _DEVICE_ATTR(misc_intr_event),
    _DEVICE_ATTR(event_detect_ctrl),
    _DEVICE_ATTR(mac_pcie_rst),
    _DEVICE_ATTR(mac_qspi_rst),
    _DEVICE_ATTR(rgb_phy_0_rst),
    _DEVICE_ATTR(rgb_phy_1_rst),
    _DEVICE_ATTR(rgb_phy_2_rst),
    _DEVICE_ATTR(rgb_phy_3_rst),
    _DEVICE_ATTR(rgb_phy_4_rst),
    _DEVICE_ATTR(rgb_phy_5_rst),
    _DEVICE_ATTR(rgb_phy_6_rst),
    _DEVICE_ATTR(rgb_phy_7_rst),
    _DEVICE_ATTR(ntm_rst),
    _DEVICE_ATTR(bmc_rst),
    _DEVICE_ATTR(usb_redriver_rst),
    _DEVICE_ATTR(usb_ocp_rcvry),
    _DEVICE_ATTR(cpld_2_rst),
    _DEVICE_ATTR(fpga_rst),
    _DEVICE_ATTR(cpld_4_rst),
    _DEVICE_ATTR(cpld_5_rst),
    _DEVICE_ATTR(cpld_ioexp_rst),
    _DEVICE_ATTR(fan_i2c_mux_rst),
    _DEVICE_ATTR(cpld_25gphy_rst),
    _DEVICE_ATTR(i210_dev),
    _DEVICE_ATTR(lan_pg),
    _DEVICE_ATTR(i210_rst),
    _DEVICE_ATTR(top_i2c_mux_rst),
    _DEVICE_ATTR(rc32312_1_rst),
    _DEVICE_ATTR(rc32312_2_rst),
    _DEVICE_ATTR(rc32312_3_rst),
    _DEVICE_ATTR(cpu_prsnt),
    _DEVICE_ATTR(bmc_prsnt),
    _DEVICE_ATTR(ntm_prsnt),
    _DEVICE_ATTR(usb_prsnt),
    _DEVICE_ATTR(ext_prsnt),
    _DEVICE_ATTR(top_prsnt),
    _DEVICE_ATTR(psu_0_prsnt),
    _DEVICE_ATTR(psu_1_prsnt),
    _DEVICE_ATTR(psu_0_acin),
    _DEVICE_ATTR(psu_1_acin),
    _DEVICE_ATTR(psu_0_pg),
    _DEVICE_ATTR(psu_1_pg),
    _DEVICE_ATTR(cpu_pg),
    _DEVICE_ATTR(mb_cpu_pg),
    _DEVICE_ATTR(bmc_pg),
    _DEVICE_ATTR(top_pg),
    _DEVICE_ATTR(rgb_phy_0_serboot),
    _DEVICE_ATTR(rgb_phy_1_serboot),
    _DEVICE_ATTR(rgb_phy_2_serboot),
    _DEVICE_ATTR(rgb_phy_3_serboot),
    _DEVICE_ATTR(rgb_phy_4_serboot),
    _DEVICE_ATTR(rgb_phy_5_serboot),
    _DEVICE_ATTR(rgb_phy_6_serboot),
    _DEVICE_ATTR(rgb_phy_7_serboot),
    _DEVICE_ATTR(cpu_mux_sel),
    _DEVICE_ATTR(psu_mux_sel),
    _DEVICE_ATTR(fpga_qspi_sel),
    _DEVICE_ATTR(uart_mux2_sel),
    _DEVICE_ATTR(system_led_status),
    _DEVICE_ATTR(fan_led_status),
    _DEVICE_ATTR(psu_0_led_status),
    _DEVICE_ATTR(psu_1_led_status),
    _DEVICE_ATTR(sync_led_status),    
    _DEVICE_ATTR(sfp_0_led_status),
    _DEVICE_ATTR(sfp_1_led_status),
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

    // CPLD 2
    _DEVICE_ATTR(qsfpdd_nif_p20_intr),
    _DEVICE_ATTR(qsfpdd_nif_p21_intr), 
    _DEVICE_ATTR(qsfpdd_nif_p22_intr),
    _DEVICE_ATTR(qsfpdd_nif_p23_intr),
    _DEVICE_ATTR(qsfpdd_nif_p24_intr),
    _DEVICE_ATTR(qsfpdd_nif_p25_intr),
    _DEVICE_ATTR(qsfpdd_nif_p26_intr),
    _DEVICE_ATTR(qsfpdd_nif_p27_intr),
    _DEVICE_ATTR(qsfpdd_nif_p28_intr),
    _DEVICE_ATTR(qsfpdd_nif_p29_intr),
    _DEVICE_ATTR(qsfpdd_nif_p30_intr),
    _DEVICE_ATTR(qsfpdd_nif_p31_intr),
    _DEVICE_ATTR(qsfpdd_nif_p32_intr),
    _DEVICE_ATTR(qsfpdd_nif_p33_intr),
    _DEVICE_ATTR(qsfpdd_nif_p34_intr),
    _DEVICE_ATTR(qsfpdd_nif_p35_intr),
    _DEVICE_ATTR(qsfpdd_nif_p20_abs),
    _DEVICE_ATTR(qsfpdd_nif_p21_abs),
    _DEVICE_ATTR(qsfpdd_nif_p22_abs),
    _DEVICE_ATTR(qsfpdd_nif_p23_abs),
    _DEVICE_ATTR(qsfpdd_nif_p24_abs),
    _DEVICE_ATTR(qsfpdd_nif_p25_abs),
    _DEVICE_ATTR(qsfpdd_nif_p26_abs),
    _DEVICE_ATTR(qsfpdd_nif_p27_abs),
    _DEVICE_ATTR(qsfpdd_nif_p28_abs),
    _DEVICE_ATTR(qsfpdd_nif_p29_abs),
    _DEVICE_ATTR(qsfpdd_nif_p30_abs),
    _DEVICE_ATTR(qsfpdd_nif_p31_abs),
    _DEVICE_ATTR(qsfpdd_nif_p32_abs),
    _DEVICE_ATTR(qsfpdd_nif_p33_abs),
    _DEVICE_ATTR(qsfpdd_nif_p34_abs),
    _DEVICE_ATTR(qsfpdd_nif_p35_abs),
    _DEVICE_ATTR(qsfpdd_nif_p20_27_abs_event),
    _DEVICE_ATTR(qsfpdd_nif_p28_35_abs_event),

    _DEVICE_ATTR(qsfpdd_nif_p20_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p21_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p22_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p23_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p24_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p25_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p26_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p27_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p28_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p29_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p30_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p31_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p32_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p33_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p34_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p35_fuse_intr),
    _DEVICE_ATTR(qsfpdd_fab_p10_intr),
    _DEVICE_ATTR(qsfpdd_fab_p11_intr),
    _DEVICE_ATTR(qsfpdd_fab_p12_intr),
    _DEVICE_ATTR(qsfpdd_fab_p13_intr),
    _DEVICE_ATTR(qsfpdd_fab_p14_intr),
    _DEVICE_ATTR(qsfpdd_fab_p15_intr),
    _DEVICE_ATTR(qsfpdd_fab_p16_intr),
    _DEVICE_ATTR(qsfpdd_fab_p17_intr),
    _DEVICE_ATTR(qsfpdd_fab_p18_intr),
    _DEVICE_ATTR(qsfpdd_fab_p19_intr),
    _DEVICE_ATTR(qsfpdd_fab_p10_abs),
    _DEVICE_ATTR(qsfpdd_fab_p11_abs),
    _DEVICE_ATTR(qsfpdd_fab_p12_abs),
    _DEVICE_ATTR(qsfpdd_fab_p13_abs),
    _DEVICE_ATTR(qsfpdd_fab_p14_abs),
    _DEVICE_ATTR(qsfpdd_fab_p15_abs),
    _DEVICE_ATTR(qsfpdd_fab_p16_abs),
    _DEVICE_ATTR(qsfpdd_fab_p17_abs),
    _DEVICE_ATTR(qsfpdd_fab_p18_abs),
    _DEVICE_ATTR(qsfpdd_fab_p19_abs),
    _DEVICE_ATTR(qsfpdd_fab_p10_17_abs_event),
    _DEVICE_ATTR(qsfpdd_fab_p18_19_abs_event),
    _DEVICE_ATTR(qsfpdd_fab_p10_fuse_intr),
    _DEVICE_ATTR(qsfpdd_fab_p11_fuse_intr),
    _DEVICE_ATTR(qsfpdd_fab_p12_fuse_intr),
    _DEVICE_ATTR(qsfpdd_fab_p13_fuse_intr),
    _DEVICE_ATTR(qsfpdd_fab_p14_fuse_intr),
    _DEVICE_ATTR(qsfpdd_fab_p15_fuse_intr),
    _DEVICE_ATTR(qsfpdd_fab_p16_fuse_intr),
    _DEVICE_ATTR(qsfpdd_fab_p17_fuse_intr),
    _DEVICE_ATTR(qsfpdd_fab_p18_fuse_intr),
    _DEVICE_ATTR(qsfpdd_fab_p19_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p20_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p21_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p22_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p23_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p24_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p25_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p26_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p27_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p28_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p29_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p30_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p31_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p32_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p33_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p34_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p35_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p20_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p21_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p22_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p23_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p24_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p25_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p26_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p27_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p28_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p29_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p30_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p31_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p32_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p33_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p34_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p35_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p20_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p21_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p22_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p23_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p24_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p25_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p26_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p27_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p28_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p29_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p30_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p31_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p32_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p33_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p34_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p35_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p10_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p11_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p12_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p13_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p14_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p15_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p16_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p17_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p18_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p19_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p10_abs_mask),
    _DEVICE_ATTR(qsfpdd_fab_p11_abs_mask),
    _DEVICE_ATTR(qsfpdd_fab_p12_abs_mask),
    _DEVICE_ATTR(qsfpdd_fab_p13_abs_mask),
    _DEVICE_ATTR(qsfpdd_fab_p14_abs_mask),
    _DEVICE_ATTR(qsfpdd_fab_p15_abs_mask),
    _DEVICE_ATTR(qsfpdd_fab_p16_abs_mask),
    _DEVICE_ATTR(qsfpdd_fab_p17_abs_mask),
    _DEVICE_ATTR(qsfpdd_fab_p18_abs_mask),
    _DEVICE_ATTR(qsfpdd_fab_p19_abs_mask),
    _DEVICE_ATTR(qsfpdd_fab_p10_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p11_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p12_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p13_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p14_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p15_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p16_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p17_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p18_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p19_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p20_rst),
    _DEVICE_ATTR(qsfpdd_nif_p21_rst),
    _DEVICE_ATTR(qsfpdd_nif_p22_rst),
    _DEVICE_ATTR(qsfpdd_nif_p23_rst),
    _DEVICE_ATTR(qsfpdd_nif_p24_rst),
    _DEVICE_ATTR(qsfpdd_nif_p25_rst),
    _DEVICE_ATTR(qsfpdd_nif_p26_rst),
    _DEVICE_ATTR(qsfpdd_nif_p27_rst),
    _DEVICE_ATTR(qsfpdd_nif_p28_rst),
    _DEVICE_ATTR(qsfpdd_nif_p29_rst),
    _DEVICE_ATTR(qsfpdd_nif_p30_rst),
    _DEVICE_ATTR(qsfpdd_nif_p31_rst),
    _DEVICE_ATTR(qsfpdd_nif_p32_rst),
    _DEVICE_ATTR(qsfpdd_nif_p33_rst),
    _DEVICE_ATTR(qsfpdd_nif_p34_rst),
    _DEVICE_ATTR(qsfpdd_nif_p35_rst),
    _DEVICE_ATTR(qsfpdd_fab_p10_rst),
    _DEVICE_ATTR(qsfpdd_fab_p11_rst),
    _DEVICE_ATTR(qsfpdd_fab_p12_rst),
    _DEVICE_ATTR(qsfpdd_fab_p13_rst),
    _DEVICE_ATTR(qsfpdd_fab_p14_rst),
    _DEVICE_ATTR(qsfpdd_fab_p15_rst),
    _DEVICE_ATTR(qsfpdd_fab_p16_rst),
    _DEVICE_ATTR(qsfpdd_fab_p17_rst),
    _DEVICE_ATTR(qsfpdd_fab_p18_rst),
    _DEVICE_ATTR(qsfpdd_fab_p19_rst),
    _DEVICE_ATTR(qsfpdd_nif_p20_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p21_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p22_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p23_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p24_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p25_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p26_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p27_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p28_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p29_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p30_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p31_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p32_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p33_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p34_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p35_lp_mode),
    _DEVICE_ATTR(qsfpdd_fab_p10_lp_mode),
    _DEVICE_ATTR(qsfpdd_fab_p11_lp_mode),
    _DEVICE_ATTR(qsfpdd_fab_p12_lp_mode),
    _DEVICE_ATTR(qsfpdd_fab_p13_lp_mode),
    _DEVICE_ATTR(qsfpdd_fab_p14_lp_mode),
    _DEVICE_ATTR(qsfpdd_fab_p15_lp_mode),
    _DEVICE_ATTR(qsfpdd_fab_p16_lp_mode),
    _DEVICE_ATTR(qsfpdd_fab_p17_lp_mode),
    _DEVICE_ATTR(qsfpdd_fab_p18_lp_mode),
    _DEVICE_ATTR(qsfpdd_fab_p19_lp_mode),
    _DEVICE_ATTR(qsfpdd_fab_p10_led_status),
    _DEVICE_ATTR(qsfpdd_fab_p11_led_status),
    _DEVICE_ATTR(qsfpdd_fab_p12_led_status),
    _DEVICE_ATTR(qsfpdd_fab_p13_led_status),
    _DEVICE_ATTR(qsfpdd_fab_p14_led_status),
    _DEVICE_ATTR(qsfpdd_fab_p15_led_status),
    _DEVICE_ATTR(qsfpdd_fab_p16_led_status),
    _DEVICE_ATTR(qsfpdd_fab_p17_led_status),
    _DEVICE_ATTR(qsfpdd_fab_p18_led_status),
    _DEVICE_ATTR(qsfpdd_fab_p19_led_status),
    _DEVICE_ATTR(qsfpdd_fab_p10_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_fab_p11_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_fab_p12_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_fab_p13_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_fab_p14_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_fab_p15_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_fab_p16_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_fab_p17_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_fab_p18_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_fab_p19_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p20_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p21_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p22_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p23_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p24_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p25_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p26_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p27_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p28_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p29_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p30_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p31_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p32_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p33_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p34_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p35_i2c_stuck),

    NULL
};

/* cpld 3 */
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

    _DEVICE_ATTR(sfp28_p37_ts),
    _DEVICE_ATTR(sfp28_p36_ts),
    _DEVICE_ATTR(mgmt_p1_ts),
    _DEVICE_ATTR(mgmt_p0_ts),
    _DEVICE_ATTR(sfp28_p37_rs),
    _DEVICE_ATTR(sfp28_p36_rs),
    _DEVICE_ATTR(mgmt_p1_rs),
    _DEVICE_ATTR(mgmt_p0_rs),
    _DEVICE_ATTR(sfp28_p37_tx_dis),
    _DEVICE_ATTR(sfp28_p36_tx_dis),
    _DEVICE_ATTR(mgmt_p1_tx_dis),
    _DEVICE_ATTR(mgmt_p0_tx_dis),
    _DEVICE_ATTR(sfp28_p37_tx_flt),
    _DEVICE_ATTR(sfp28_p36_tx_flt),
    _DEVICE_ATTR(mgmt_p1_tx_flt),
    _DEVICE_ATTR(mgmt_p0_tx_flt),
    _DEVICE_ATTR(sfp28_p37_rx_los),
    _DEVICE_ATTR(sfp28_p36_rx_los),
    _DEVICE_ATTR(mgmt_p1_rx_los),
    _DEVICE_ATTR(mgmt_p0_rx_los),
    _DEVICE_ATTR(sfp28_p37_abs),
    _DEVICE_ATTR(sfp28_p36_abs),
    _DEVICE_ATTR(mgmt_p1_abs),
    _DEVICE_ATTR(mgmt_p0_abs),
    _DEVICE_ATTR(sfp28_p37_tx_flt_mask),
    _DEVICE_ATTR(sfp28_p36_tx_flt_mask),
    _DEVICE_ATTR(mgmt_p1_tx_flt_mask),
    _DEVICE_ATTR(mgmt_p0_tx_flt_mask),
    _DEVICE_ATTR(sfp28_p37_rx_los_mask),
    _DEVICE_ATTR(sfp28_p36_rx_los_mask),
    _DEVICE_ATTR(mgmt_p1_rx_los_mask),
    _DEVICE_ATTR(mgmt_p0_rx_los_mask),
    _DEVICE_ATTR(sfp28_p37_abs_mask),
    _DEVICE_ATTR(sfp28_p36_abs_mask),
    _DEVICE_ATTR(mgmt_p1_abs_mask),
    _DEVICE_ATTR(mgmt_p0_abs_mask),
    _DEVICE_ATTR(sfp28_tx_flt_event),
    _DEVICE_ATTR(sfp28_rx_los_event),
    _DEVICE_ATTR(sfp28_abs_event),
    _DEVICE_ATTR(mgmt_p0_i2c_stuck),
    _DEVICE_ATTR(mgmt_p1_i2c_stuck),
    _DEVICE_ATTR(sfp28_p36_i2c_stuck),
    _DEVICE_ATTR(sfp28_p37_i2c_stuck),

    NULL
};


/* cpld 4 */
static struct attribute *cpld4_attributes[] = {

    // CPLD Common
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(event_detect_ctrl),

    // CPLD 4
    _DEVICE_ATTR(qsfpdd_nif_p0_intr),
    _DEVICE_ATTR(qsfpdd_nif_p1_intr),
    _DEVICE_ATTR(qsfpdd_nif_p2_intr),
    _DEVICE_ATTR(qsfpdd_nif_p3_intr),
    _DEVICE_ATTR(qsfpdd_nif_p4_intr),
    _DEVICE_ATTR(qsfpdd_nif_p5_intr),
    _DEVICE_ATTR(qsfpdd_nif_p6_intr),
    _DEVICE_ATTR(qsfpdd_nif_p7_intr),
    _DEVICE_ATTR(qsfpdd_nif_p8_intr),
    _DEVICE_ATTR(qsfpdd_nif_p9_intr),
    _DEVICE_ATTR(qsfpdd_nif_p10_intr),
    _DEVICE_ATTR(qsfpdd_nif_p11_intr),
    _DEVICE_ATTR(qsfpdd_nif_p12_intr),
    _DEVICE_ATTR(qsfpdd_nif_p13_intr),
    _DEVICE_ATTR(qsfpdd_nif_p14_intr),
    _DEVICE_ATTR(qsfpdd_nif_p15_intr),
    _DEVICE_ATTR(qsfpdd_nif_p16_intr),
    _DEVICE_ATTR(qsfpdd_nif_p17_intr),
    _DEVICE_ATTR(qsfpdd_nif_p18_intr),
    _DEVICE_ATTR(qsfpdd_nif_p19_intr),
    _DEVICE_ATTR(qsfpdd_nif_p0_abs),
    _DEVICE_ATTR(qsfpdd_nif_p1_abs),
    _DEVICE_ATTR(qsfpdd_nif_p2_abs),
    _DEVICE_ATTR(qsfpdd_nif_p3_abs),
    _DEVICE_ATTR(qsfpdd_nif_p4_abs),
    _DEVICE_ATTR(qsfpdd_nif_p5_abs),
    _DEVICE_ATTR(qsfpdd_nif_p6_abs),
    _DEVICE_ATTR(qsfpdd_nif_p7_abs),
    _DEVICE_ATTR(qsfpdd_nif_p8_abs),
    _DEVICE_ATTR(qsfpdd_nif_p9_abs),
    _DEVICE_ATTR(qsfpdd_nif_p10_abs),
    _DEVICE_ATTR(qsfpdd_nif_p11_abs),
    _DEVICE_ATTR(qsfpdd_nif_p12_abs),
    _DEVICE_ATTR(qsfpdd_nif_p13_abs),
    _DEVICE_ATTR(qsfpdd_nif_p14_abs),
    _DEVICE_ATTR(qsfpdd_nif_p15_abs),
    _DEVICE_ATTR(qsfpdd_nif_p16_abs),
    _DEVICE_ATTR(qsfpdd_nif_p17_abs),
    _DEVICE_ATTR(qsfpdd_nif_p18_abs),
    _DEVICE_ATTR(qsfpdd_nif_p19_abs),
    _DEVICE_ATTR(qsfpdd_nif_p0_7_abs_event),
    _DEVICE_ATTR(qsfpdd_nif_p8_15_abs_event),
    _DEVICE_ATTR(qsfpdd_nif_p16_19_abs_event),
    _DEVICE_ATTR(qsfpdd_nif_p0_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p1_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p2_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p3_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p4_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p5_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p6_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p7_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p8_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p9_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p10_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p11_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p12_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p13_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p14_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p15_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p16_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p17_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p18_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p19_fuse_intr),

    _DEVICE_ATTR(qsfpdd_fab_p0_intr),
    _DEVICE_ATTR(qsfpdd_fab_p1_intr),
    _DEVICE_ATTR(qsfpdd_fab_p2_intr),
    _DEVICE_ATTR(qsfpdd_fab_p3_intr),
    _DEVICE_ATTR(qsfpdd_fab_p4_intr),
    _DEVICE_ATTR(qsfpdd_fab_p5_intr),
    _DEVICE_ATTR(qsfpdd_fab_p6_intr),
    _DEVICE_ATTR(qsfpdd_fab_p7_intr),
    _DEVICE_ATTR(qsfpdd_fab_p8_intr),
    _DEVICE_ATTR(qsfpdd_fab_p9_intr),
    _DEVICE_ATTR(qsfpdd_fab_p0_abs),
    _DEVICE_ATTR(qsfpdd_fab_p1_abs),
    _DEVICE_ATTR(qsfpdd_fab_p2_abs),
    _DEVICE_ATTR(qsfpdd_fab_p3_abs),
    _DEVICE_ATTR(qsfpdd_fab_p4_abs),
    _DEVICE_ATTR(qsfpdd_fab_p5_abs),
    _DEVICE_ATTR(qsfpdd_fab_p6_abs),
    _DEVICE_ATTR(qsfpdd_fab_p7_abs),
    _DEVICE_ATTR(qsfpdd_fab_p8_abs),
    _DEVICE_ATTR(qsfpdd_fab_p9_abs),
    _DEVICE_ATTR(qsfpdd_fab_p0_7_abs_event),
    _DEVICE_ATTR(qsfpdd_fab_p8_9_abs_event),
    _DEVICE_ATTR(qsfpdd_fab_p0_fuse_intr),
    _DEVICE_ATTR(qsfpdd_fab_p1_fuse_intr),
    _DEVICE_ATTR(qsfpdd_fab_p2_fuse_intr),
    _DEVICE_ATTR(qsfpdd_fab_p3_fuse_intr),
    _DEVICE_ATTR(qsfpdd_fab_p4_fuse_intr),
    _DEVICE_ATTR(qsfpdd_fab_p5_fuse_intr),
    _DEVICE_ATTR(qsfpdd_fab_p6_fuse_intr),
    _DEVICE_ATTR(qsfpdd_fab_p7_fuse_intr),
    _DEVICE_ATTR(qsfpdd_fab_p8_fuse_intr),
    _DEVICE_ATTR(qsfpdd_fab_p9_fuse_intr),
    _DEVICE_ATTR(qsfpdd_nif_p0_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p1_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p2_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p3_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p4_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p5_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p6_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p7_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p8_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p9_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p10_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p11_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p12_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p13_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p14_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p15_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p16_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p17_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p18_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p19_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p0_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p1_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p2_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p3_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p4_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p5_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p6_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p7_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p8_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p9_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p10_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p11_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p12_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p13_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p14_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p15_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p16_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p17_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p18_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p19_abs_mask),
    _DEVICE_ATTR(qsfpdd_nif_p0_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p1_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p2_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p3_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p4_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p5_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p6_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p7_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p8_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p9_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p10_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p11_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p12_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p13_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p14_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p15_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p16_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p17_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p18_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_nif_p19_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p0_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p1_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p2_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p3_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p4_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p5_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p6_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p7_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p8_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p9_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p0_abs_mask),
    _DEVICE_ATTR(qsfpdd_fab_p1_abs_mask),
    _DEVICE_ATTR(qsfpdd_fab_p2_abs_mask),
    _DEVICE_ATTR(qsfpdd_fab_p3_abs_mask),
    _DEVICE_ATTR(qsfpdd_fab_p4_abs_mask),
    _DEVICE_ATTR(qsfpdd_fab_p5_abs_mask),
    _DEVICE_ATTR(qsfpdd_fab_p6_abs_mask),
    _DEVICE_ATTR(qsfpdd_fab_p7_abs_mask),
    _DEVICE_ATTR(qsfpdd_fab_p8_abs_mask),
    _DEVICE_ATTR(qsfpdd_fab_p9_abs_mask),
    _DEVICE_ATTR(qsfpdd_fab_p0_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p1_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p2_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p3_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p4_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p5_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p6_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p7_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p8_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_fab_p9_fuse_intr_mask),

    _DEVICE_ATTR(qsfpdd_nif_p0_rst),
    _DEVICE_ATTR(qsfpdd_nif_p1_rst),
    _DEVICE_ATTR(qsfpdd_nif_p2_rst),
    _DEVICE_ATTR(qsfpdd_nif_p3_rst),
    _DEVICE_ATTR(qsfpdd_nif_p4_rst),
    _DEVICE_ATTR(qsfpdd_nif_p5_rst),
    _DEVICE_ATTR(qsfpdd_nif_p6_rst),
    _DEVICE_ATTR(qsfpdd_nif_p7_rst),
    _DEVICE_ATTR(qsfpdd_nif_p8_rst),
    _DEVICE_ATTR(qsfpdd_nif_p9_rst),
    _DEVICE_ATTR(qsfpdd_nif_p10_rst),
    _DEVICE_ATTR(qsfpdd_nif_p11_rst),
    _DEVICE_ATTR(qsfpdd_nif_p12_rst),
    _DEVICE_ATTR(qsfpdd_nif_p13_rst),
    _DEVICE_ATTR(qsfpdd_nif_p14_rst),
    _DEVICE_ATTR(qsfpdd_nif_p15_rst),
    _DEVICE_ATTR(qsfpdd_nif_p16_rst),
    _DEVICE_ATTR(qsfpdd_nif_p17_rst),
    _DEVICE_ATTR(qsfpdd_nif_p18_rst),
    _DEVICE_ATTR(qsfpdd_nif_p19_rst),
    _DEVICE_ATTR(qsfpdd_fab_p0_rst),
    _DEVICE_ATTR(qsfpdd_fab_p1_rst),
    _DEVICE_ATTR(qsfpdd_fab_p2_rst),
    _DEVICE_ATTR(qsfpdd_fab_p3_rst),
    _DEVICE_ATTR(qsfpdd_fab_p4_rst),
    _DEVICE_ATTR(qsfpdd_fab_p5_rst),
    _DEVICE_ATTR(qsfpdd_fab_p6_rst),
    _DEVICE_ATTR(qsfpdd_fab_p7_rst),
    _DEVICE_ATTR(qsfpdd_fab_p8_rst),
    _DEVICE_ATTR(qsfpdd_fab_p9_rst),
    _DEVICE_ATTR(qsfpdd_nif_p0_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p1_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p2_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p3_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p4_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p5_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p6_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p7_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p8_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p9_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p10_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p11_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p12_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p13_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p14_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p15_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p16_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p17_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p18_lp_mode),
    _DEVICE_ATTR(qsfpdd_nif_p19_lp_mode),
    _DEVICE_ATTR(qsfpdd_fab_p0_lp_mode),
    _DEVICE_ATTR(qsfpdd_fab_p1_lp_mode),
    _DEVICE_ATTR(qsfpdd_fab_p2_lp_mode),
    _DEVICE_ATTR(qsfpdd_fab_p3_lp_mode),
    _DEVICE_ATTR(qsfpdd_fab_p4_lp_mode),
    _DEVICE_ATTR(qsfpdd_fab_p5_lp_mode),
    _DEVICE_ATTR(qsfpdd_fab_p6_lp_mode),
    _DEVICE_ATTR(qsfpdd_fab_p7_lp_mode),
    _DEVICE_ATTR(qsfpdd_fab_p8_lp_mode),
    _DEVICE_ATTR(qsfpdd_fab_p9_lp_mode),


    _DEVICE_ATTR(qsfpdd_fab_p0_led_status),
    _DEVICE_ATTR(qsfpdd_fab_p1_led_status),
    _DEVICE_ATTR(qsfpdd_fab_p2_led_status),
    _DEVICE_ATTR(qsfpdd_fab_p3_led_status),
    _DEVICE_ATTR(qsfpdd_fab_p4_led_status),
    _DEVICE_ATTR(qsfpdd_fab_p5_led_status),
    _DEVICE_ATTR(qsfpdd_fab_p6_led_status),
    _DEVICE_ATTR(qsfpdd_fab_p7_led_status),
    _DEVICE_ATTR(qsfpdd_fab_p8_led_status),
    _DEVICE_ATTR(qsfpdd_fab_p9_led_status),
    _DEVICE_ATTR(qsfpdd_nif_p0_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p1_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p2_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p3_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p4_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p5_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p6_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p7_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p8_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p9_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p10_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p11_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p12_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p13_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p14_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p15_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p16_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p17_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p18_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_nif_p19_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_fab_p0_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_fab_p1_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_fab_p2_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_fab_p3_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_fab_p4_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_fab_p5_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_fab_p6_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_fab_p7_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_fab_p8_i2c_stuck),
    _DEVICE_ATTR(qsfpdd_fab_p9_i2c_stuck),

    NULL
};

/* cpld 5 */
static struct attribute *cpld5_attributes[] = {
    // CPLD Common
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),
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
static const struct attribute_group fpga_group = {
    .attrs = fpga_attributes,
};

/* cpld 4 attributes group */
static const struct attribute_group cpld4_group = {
    .attrs = cpld4_attributes,
};

/* cpld 5 attributes group */
static const struct attribute_group cpld5_group = {
    .attrs = cpld5_attributes,
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

static int _parse_data(char *buf, unsigned int data, u8 data_type)
{
    if(buf == NULL) {
        return -EINVAL;
    }

    if(data_type == DATA_HEX) {
        return sprintf(buf, "0x%02x", data);
    } else if(data_type == DATA_DEC) {
        return sprintf(buf, "%u", data);
    } else {
        return -EINVAL;
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

        //CPLD 1
        case CPLD_SKU_ID:
        case CPLD_HW_REV:
        case CPLD_DEPH_REV:
        case CPLD_BUILD_REV:
        case CPLD_BRD_ID_TYPE:
        case CPLD_CHIP_TYPE:
        case MAC_INTR:
        case RGB_PHY_0_INTR:
        case RGB_PHY_1_INTR:
        case RGB_PHY_2_INTR:
        case RGB_PHY_3_INTR:
        case RGB_PHY_4_INTR:
        case RGB_PHY_5_INTR:
        case RGB_PHY_6_INTR:
        case RGB_PHY_7_INTR:
        case TOP_RGB_INTR:
        case CPLD_25GPHY_INTR:
        case CPLD_2_FRU_INTR:
        case CPLD_4_FRU_INTR:

        case SFP28_INTR:
        case PSU_0_INTR:
        case PSU_1_INTR:
        case RETIMER_INTR:

        case MAC_HBM_TEMP_ALERT:
        case MB_LM75_TEMP_ALERT:
        case EXT_LM75_TEMP_ALERT:
        case PHY_LM75_CPLD5_INTR:
        case USB_OCP:
        case FPGA_INTR:
        case LM75_BMC_INTR:
        case LM75_CPLD1_INTR:
        case CPU_INTR:
        case LM75_CPLD245_INTR:

        case MAC_INTR_MASK:
        case RGB_PHY_0_INTR_MASK:
        case RGB_PHY_1_INTR_MASK:
        case RGB_PHY_2_INTR_MASK:
        case RGB_PHY_3_INTR_MASK:
        case RGB_PHY_4_INTR_MASK:
        case RGB_PHY_5_INTR_MASK:
        case RGB_PHY_6_INTR_MASK:
        case RGB_PHY_7_INTR_MASK:
        case TOP_RGB_INTR_MASK:
        case MISC_INTR:
        case SYSTEM_INTR:
        case CPLD_25GPHY_INTR_MASK:
        case CPLD_2_FRU_INTR_MASK:
        case CPLD_4_FRU_INTR_MASK:
        case SFP28_INTR_MASK:
        case RETIMER_INTR_MASK:
        case PSU_0_INTR_MASK:
        case PSU_1_INTR_MASK:

        case USB_OCP_INTR_MASK:
        case MAC_TEMP_ALERT_MASK:
        case MB_LM75_TEMP_ALERT_MASK:
        case MB_TEMP_1_ALERT_MASK:
        case MB_TEMP_2_ALERT_MASK:
        case MB_TEMP_3_ALERT_MASK:
        case MB_TEMP_4_ALERT_MASK:
        case MB_TEMP_5_ALERT_MASK:
        case MB_TEMP_6_ALERT_MASK:
        case MB_TEMP_7_ALERT_MASK:
        case TOP_CPLD_5_INTR_MASK:
        case USB_OCP_MASK:
        case FPGA_INTR_MASK:
        case LM75_BMC_INTR_MASK:
        case LM75_CPLD1_INTR_MASK:
        case CPU_INTR_MASK:
        case LM75_CPLD245_INTR_MASK:

        case MAC_INTR_EVENT:
        case MB_RGB_0_7_INTR_EVENT:
        case TOP_RGB_INTR_EVENT:
        case CPLD_25GPHY_INTR_EVENT:
        case CPLD_FRU_INTR_EVENT:
        case MAC_TEMP_ALERT_EVENT:
        case TEMP_ALERT_EVENT:
        case MISC_INTR_EVENT:
        case EVENT_DETECT_CTRL:
        case MAC_PCIE_RST:
        case MAC_QSPI_RST:
        case RGB_PHY_0_RST:
        case RGB_PHY_1_RST:
        case RGB_PHY_2_RST:
        case RGB_PHY_3_RST:
        case RGB_PHY_4_RST:
        case RGB_PHY_5_RST:
        case RGB_PHY_6_RST:
        case RGB_PHY_7_RST:

        case NTM_RST:
        case BMC_RST:
        case USB_REDRIVER_RST:
        case USB_OCP_RCVRY:

        case CPLD_2_RST:
        case FPGA_RST:
        case CPLD_4_RST:
        case CPLD_5_RST:

        case CPLD_IOEXP_RST:
        case FAN_I2C_MUX_RST:
        case CPLD_25GPHY_RST:
        case I210_DEV:
        case LAN_PG:
        case I210_RST:
        case TOP_I2C_MUX_RST:
        case RC32312_1_RST:
        case RC32312_2_RST:
        case RC32312_3_RST:

        case CPU_PRSNT:
        case BMC_PRSNT:
        case NTM_PRSNT:
        case USB_PRSNT:
        case EXT_PRSNT:
        case TOP_PRSNT:

        case PSU_0_PRSNT:
        case PSU_1_PRSNT:
        case PSU_0_ACIN:
        case PSU_1_ACIN:
        case PSU_0_PG:
        case PSU_1_PG:

        case CPU_PG:
        case MB_CPU_PG:
        case BMC_PG:
        case TOP_PG:

        case RGB_PHY_0_SERBOOT:
        case RGB_PHY_1_SERBOOT:
        case RGB_PHY_2_SERBOOT:
        case RGB_PHY_3_SERBOOT:
        case RGB_PHY_4_SERBOOT:
        case RGB_PHY_5_SERBOOT:
        case RGB_PHY_6_SERBOOT:
        case RGB_PHY_7_SERBOOT:

        case CPU_MUX_SEL:
        case PSU_MUX_SEL:
        case FPGA_QSPI_SEL:
        case UART_MUX2_SEL:

        case SYSTEM_LED_STATUS:
        case FAN_LED_STATUS:
        case PSU_0_LED_STATUS:
        case PSU_1_LED_STATUS:
        case SYNC_LED_STATUS:
        case SFP_0_LED_STATUS:
        case SFP_1_LED_STATUS:

        //CPLD 2
        case QSFPDD_NIF_P20_INTR:
        case QSFPDD_NIF_P21_INTR:
        case QSFPDD_NIF_P22_INTR:
        case QSFPDD_NIF_P23_INTR:
        case QSFPDD_NIF_P24_INTR:
        case QSFPDD_NIF_P25_INTR:
        case QSFPDD_NIF_P26_INTR:
        case QSFPDD_NIF_P27_INTR:
        case QSFPDD_NIF_P28_INTR:
        case QSFPDD_NIF_P29_INTR:
        case QSFPDD_NIF_P30_INTR:
        case QSFPDD_NIF_P31_INTR:
        case QSFPDD_NIF_P32_INTR:
        case QSFPDD_NIF_P33_INTR:
        case QSFPDD_NIF_P34_INTR:
        case QSFPDD_NIF_P35_INTR:
        case QSFPDD_NIF_P20_ABS:
        case QSFPDD_NIF_P21_ABS:
        case QSFPDD_NIF_P22_ABS:
        case QSFPDD_NIF_P23_ABS:
        case QSFPDD_NIF_P24_ABS:
        case QSFPDD_NIF_P25_ABS:
        case QSFPDD_NIF_P26_ABS:
        case QSFPDD_NIF_P27_ABS:
        case QSFPDD_NIF_P28_ABS:
        case QSFPDD_NIF_P29_ABS:
        case QSFPDD_NIF_P30_ABS:
        case QSFPDD_NIF_P31_ABS:
        case QSFPDD_NIF_P32_ABS:
        case QSFPDD_NIF_P33_ABS:
        case QSFPDD_NIF_P34_ABS:
        case QSFPDD_NIF_P35_ABS:
        case QSFPDD_NIF_P20_27_ABS_EVENT:
        case QSFPDD_NIF_P28_35_ABS_EVENT:
        case QSFPDD_NIF_P20_FUSE_INTR:
        case QSFPDD_NIF_P21_FUSE_INTR:
        case QSFPDD_NIF_P22_FUSE_INTR:
        case QSFPDD_NIF_P23_FUSE_INTR:
        case QSFPDD_NIF_P24_FUSE_INTR:
        case QSFPDD_NIF_P25_FUSE_INTR:
        case QSFPDD_NIF_P26_FUSE_INTR:
        case QSFPDD_NIF_P27_FUSE_INTR:
        case QSFPDD_NIF_P28_FUSE_INTR:
        case QSFPDD_NIF_P29_FUSE_INTR:
        case QSFPDD_NIF_P30_FUSE_INTR:
        case QSFPDD_NIF_P31_FUSE_INTR:
        case QSFPDD_NIF_P32_FUSE_INTR:
        case QSFPDD_NIF_P33_FUSE_INTR:
        case QSFPDD_NIF_P34_FUSE_INTR:
        case QSFPDD_NIF_P35_FUSE_INTR:

        case QSFPDD_FAB_P10_INTR:
        case QSFPDD_FAB_P11_INTR:
        case QSFPDD_FAB_P12_INTR:
        case QSFPDD_FAB_P13_INTR:
        case QSFPDD_FAB_P14_INTR:
        case QSFPDD_FAB_P15_INTR:
        case QSFPDD_FAB_P16_INTR:
        case QSFPDD_FAB_P17_INTR:
        case QSFPDD_FAB_P18_INTR:
        case QSFPDD_FAB_P19_INTR:
        case QSFPDD_FAB_P10_ABS:
        case QSFPDD_FAB_P11_ABS:
        case QSFPDD_FAB_P12_ABS:
        case QSFPDD_FAB_P13_ABS:
        case QSFPDD_FAB_P14_ABS:
        case QSFPDD_FAB_P15_ABS:
        case QSFPDD_FAB_P16_ABS:
        case QSFPDD_FAB_P17_ABS:
        case QSFPDD_FAB_P18_ABS:
        case QSFPDD_FAB_P19_ABS:
        case QSFPDD_FAB_P10_17_ABS_EVENT:
        case QSFPDD_FAB_P18_19_ABS_EVENT:
        case QSFPDD_FAB_P10_FUSE_INTR:
        case QSFPDD_FAB_P11_FUSE_INTR:
        case QSFPDD_FAB_P12_FUSE_INTR:
        case QSFPDD_FAB_P13_FUSE_INTR:
        case QSFPDD_FAB_P14_FUSE_INTR:
        case QSFPDD_FAB_P15_FUSE_INTR:
        case QSFPDD_FAB_P16_FUSE_INTR:
        case QSFPDD_FAB_P17_FUSE_INTR:
        case QSFPDD_FAB_P18_FUSE_INTR:
        case QSFPDD_FAB_P19_FUSE_INTR:
        case QSFPDD_NIF_P20_INTR_MASK:
        case QSFPDD_NIF_P21_INTR_MASK:
        case QSFPDD_NIF_P22_INTR_MASK:
        case QSFPDD_NIF_P23_INTR_MASK:
        case QSFPDD_NIF_P24_INTR_MASK:
        case QSFPDD_NIF_P25_INTR_MASK:
        case QSFPDD_NIF_P26_INTR_MASK:
        case QSFPDD_NIF_P27_INTR_MASK:
        case QSFPDD_NIF_P28_INTR_MASK:
        case QSFPDD_NIF_P29_INTR_MASK:
        case QSFPDD_NIF_P30_INTR_MASK:
        case QSFPDD_NIF_P31_INTR_MASK:
        case QSFPDD_NIF_P32_INTR_MASK:
        case QSFPDD_NIF_P33_INTR_MASK:
        case QSFPDD_NIF_P34_INTR_MASK:
        case QSFPDD_NIF_P35_INTR_MASK:

        case QSFPDD_NIF_P20_ABS_MASK:
        case QSFPDD_NIF_P21_ABS_MASK:
        case QSFPDD_NIF_P22_ABS_MASK:
        case QSFPDD_NIF_P23_ABS_MASK:
        case QSFPDD_NIF_P24_ABS_MASK:
        case QSFPDD_NIF_P25_ABS_MASK:
        case QSFPDD_NIF_P26_ABS_MASK:
        case QSFPDD_NIF_P27_ABS_MASK:
        case QSFPDD_NIF_P28_ABS_MASK:
        case QSFPDD_NIF_P29_ABS_MASK:
        case QSFPDD_NIF_P30_ABS_MASK:
        case QSFPDD_NIF_P31_ABS_MASK:
        case QSFPDD_NIF_P32_ABS_MASK:
        case QSFPDD_NIF_P33_ABS_MASK:
        case QSFPDD_NIF_P34_ABS_MASK:
        case QSFPDD_NIF_P35_ABS_MASK:

        case QSFPDD_NIF_P20_FUSE_INTR_MASK:
        case QSFPDD_NIF_P21_FUSE_INTR_MASK:
        case QSFPDD_NIF_P22_FUSE_INTR_MASK:
        case QSFPDD_NIF_P23_FUSE_INTR_MASK:
        case QSFPDD_NIF_P24_FUSE_INTR_MASK:
        case QSFPDD_NIF_P25_FUSE_INTR_MASK:
        case QSFPDD_NIF_P26_FUSE_INTR_MASK:
        case QSFPDD_NIF_P27_FUSE_INTR_MASK:
        case QSFPDD_NIF_P28_FUSE_INTR_MASK:
        case QSFPDD_NIF_P29_FUSE_INTR_MASK:
        case QSFPDD_NIF_P30_FUSE_INTR_MASK:
        case QSFPDD_NIF_P31_FUSE_INTR_MASK:
        case QSFPDD_NIF_P32_FUSE_INTR_MASK:
        case QSFPDD_NIF_P33_FUSE_INTR_MASK:
        case QSFPDD_NIF_P34_FUSE_INTR_MASK:
        case QSFPDD_NIF_P35_FUSE_INTR_MASK:

        case QSFPDD_FAB_P10_INTR_MASK:
        case QSFPDD_FAB_P11_INTR_MASK:
        case QSFPDD_FAB_P12_INTR_MASK:
        case QSFPDD_FAB_P13_INTR_MASK:
        case QSFPDD_FAB_P14_INTR_MASK:
        case QSFPDD_FAB_P15_INTR_MASK:
        case QSFPDD_FAB_P16_INTR_MASK:
        case QSFPDD_FAB_P17_INTR_MASK:
        case QSFPDD_FAB_P18_INTR_MASK:
        case QSFPDD_FAB_P19_INTR_MASK:
        case QSFPDD_FAB_P10_ABS_MASK:
        case QSFPDD_FAB_P11_ABS_MASK:
        case QSFPDD_FAB_P12_ABS_MASK:
        case QSFPDD_FAB_P13_ABS_MASK:
        case QSFPDD_FAB_P14_ABS_MASK:
        case QSFPDD_FAB_P15_ABS_MASK:
        case QSFPDD_FAB_P16_ABS_MASK:
        case QSFPDD_FAB_P17_ABS_MASK:
        case QSFPDD_FAB_P18_ABS_MASK:
        case QSFPDD_FAB_P19_ABS_MASK:
        case QSFPDD_FAB_P10_FUSE_INTR_MASK:
        case QSFPDD_FAB_P11_FUSE_INTR_MASK:
        case QSFPDD_FAB_P12_FUSE_INTR_MASK:
        case QSFPDD_FAB_P13_FUSE_INTR_MASK:
        case QSFPDD_FAB_P14_FUSE_INTR_MASK:
        case QSFPDD_FAB_P15_FUSE_INTR_MASK:
        case QSFPDD_FAB_P16_FUSE_INTR_MASK:
        case QSFPDD_FAB_P17_FUSE_INTR_MASK:
        case QSFPDD_FAB_P18_FUSE_INTR_MASK:
        case QSFPDD_FAB_P19_FUSE_INTR_MASK:

        case QSFPDD_NIF_P20_RST:
        case QSFPDD_NIF_P21_RST:
        case QSFPDD_NIF_P22_RST:
        case QSFPDD_NIF_P23_RST:
        case QSFPDD_NIF_P24_RST:
        case QSFPDD_NIF_P25_RST:
        case QSFPDD_NIF_P26_RST:
        case QSFPDD_NIF_P27_RST:
        case QSFPDD_NIF_P28_RST:
        case QSFPDD_NIF_P29_RST:
        case QSFPDD_NIF_P30_RST:
        case QSFPDD_NIF_P31_RST:
        case QSFPDD_NIF_P32_RST:
        case QSFPDD_NIF_P33_RST:
        case QSFPDD_NIF_P34_RST:
        case QSFPDD_NIF_P35_RST:
        case QSFPDD_FAB_P10_RST:
        case QSFPDD_FAB_P11_RST:
        case QSFPDD_FAB_P12_RST:
        case QSFPDD_FAB_P13_RST:
        case QSFPDD_FAB_P14_RST:
        case QSFPDD_FAB_P15_RST:
        case QSFPDD_FAB_P16_RST:
        case QSFPDD_FAB_P17_RST:
        case QSFPDD_FAB_P18_RST:
        case QSFPDD_FAB_P19_RST:
        case QSFPDD_NIF_P20_LP_MODE:
        case QSFPDD_NIF_P21_LP_MODE:
        case QSFPDD_NIF_P22_LP_MODE:
        case QSFPDD_NIF_P23_LP_MODE:
        case QSFPDD_NIF_P24_LP_MODE:
        case QSFPDD_NIF_P25_LP_MODE:
        case QSFPDD_NIF_P26_LP_MODE:
        case QSFPDD_NIF_P27_LP_MODE:
        case QSFPDD_NIF_P28_LP_MODE:
        case QSFPDD_NIF_P29_LP_MODE:
        case QSFPDD_NIF_P30_LP_MODE:
        case QSFPDD_NIF_P31_LP_MODE:
        case QSFPDD_NIF_P32_LP_MODE:
        case QSFPDD_NIF_P33_LP_MODE:
        case QSFPDD_NIF_P34_LP_MODE:
        case QSFPDD_NIF_P35_LP_MODE:
        case QSFPDD_FAB_P10_LP_MODE:
        case QSFPDD_FAB_P11_LP_MODE:
        case QSFPDD_FAB_P12_LP_MODE:
        case QSFPDD_FAB_P13_LP_MODE:
        case QSFPDD_FAB_P14_LP_MODE:
        case QSFPDD_FAB_P15_LP_MODE:
        case QSFPDD_FAB_P16_LP_MODE:
        case QSFPDD_FAB_P17_LP_MODE:
        case QSFPDD_FAB_P18_LP_MODE:
        case QSFPDD_FAB_P19_LP_MODE:

        case QSFPDD_FAB_P10_LED_STATUS:
        case QSFPDD_FAB_P11_LED_STATUS:
        case QSFPDD_FAB_P12_LED_STATUS:
        case QSFPDD_FAB_P13_LED_STATUS:
        case QSFPDD_FAB_P14_LED_STATUS:
        case QSFPDD_FAB_P15_LED_STATUS:
        case QSFPDD_FAB_P16_LED_STATUS:
        case QSFPDD_FAB_P17_LED_STATUS:
        case QSFPDD_FAB_P18_LED_STATUS:
        case QSFPDD_FAB_P19_LED_STATUS:

        case QSFPDD_FAB_P10_I2C_STUCK:
        case QSFPDD_FAB_P11_I2C_STUCK:
        case QSFPDD_FAB_P12_I2C_STUCK:
        case QSFPDD_FAB_P13_I2C_STUCK:
        case QSFPDD_FAB_P14_I2C_STUCK:
        case QSFPDD_FAB_P15_I2C_STUCK:
        case QSFPDD_FAB_P16_I2C_STUCK:
        case QSFPDD_FAB_P17_I2C_STUCK:
        case QSFPDD_FAB_P18_I2C_STUCK:
        case QSFPDD_FAB_P19_I2C_STUCK:
        case QSFPDD_NIF_P20_I2C_STUCK:
        case QSFPDD_NIF_P21_I2C_STUCK:
        case QSFPDD_NIF_P22_I2C_STUCK:
        case QSFPDD_NIF_P23_I2C_STUCK:
        case QSFPDD_NIF_P24_I2C_STUCK:
        case QSFPDD_NIF_P25_I2C_STUCK:
        case QSFPDD_NIF_P26_I2C_STUCK:
        case QSFPDD_NIF_P27_I2C_STUCK:
        case QSFPDD_NIF_P28_I2C_STUCK:
        case QSFPDD_NIF_P29_I2C_STUCK:
        case QSFPDD_NIF_P30_I2C_STUCK:
        case QSFPDD_NIF_P31_I2C_STUCK:
        case QSFPDD_NIF_P32_I2C_STUCK:
        case QSFPDD_NIF_P33_I2C_STUCK:
        case QSFPDD_NIF_P34_I2C_STUCK:
        case QSFPDD_NIF_P35_I2C_STUCK:

        //CPLD 3
        // change to fpga
        case FPGA_ID:
        case FPGA_VER_1:
        case FPGA_MINOR_VER:
        case FPGA_MAJOR_VER:
        case FPGA_BUILD_VER:
        case FPGA_VERSION_H:
        case FPGA_DEV_INFO:
        case SFP28_P37_TS:
        case SFP28_P36_TS:
        case MGMT_P1_TS:
        case MGMT_P0_TS:
        case SFP28_P37_RS:
        case SFP28_P36_RS:
        case MGMT_P1_RS:
        case MGMT_P0_RS:
        case SFP28_P37_TX_DIS:
        case SFP28_P36_TX_DIS:
        case MGMT_P1_TX_DIS:
        case MGMT_P0_TX_DIS:
        case SFP28_P37_TX_FLT:
        case SFP28_P36_TX_FLT:
        case MGMT_P1_TX_FLT:
        case MGMT_P0_TX_FLT:
        case SFP28_P37_RX_LOS:
        case SFP28_P36_RX_LOS:
        case MGMT_P1_RX_LOS:
        case MGMT_P0_RX_LOS:
        case SFP28_P37_ABS:
        case SFP28_P36_ABS:
        case MGMT_P1_ABS:
        case MGMT_P0_ABS:
        case SFP28_P37_TX_FLT_MASK:
        case SFP28_P36_TX_FLT_MASK:
        case MGMT_P1_TX_FLT_MASK:
        case MGMT_P0_TX_FLT_MASK:
        case SFP28_P37_RX_LOS_MASK:
        case SFP28_P36_RX_LOS_MASK:
        case MGMT_P1_RX_LOS_MASK:
        case MGMT_P0_RX_LOS_MASK:
        case SFP28_P37_ABS_MASK:
        case SFP28_P36_ABS_MASK:
        case MGMT_P1_ABS_MASK:
        case MGMT_P0_ABS_MASK:
        case SFP28_TX_FLT_EVENT:
        case SFP28_RX_LOS_EVENT:
        case SFP28_ABS_EVENT:
        case MGMT_P0_I2C_STUCK:
        case MGMT_P1_I2C_STUCK:
        case SFP28_P36_I2C_STUCK:
        case SFP28_P37_I2C_STUCK:

        //CPLD 4
        case QSFPDD_NIF_P0_INTR:
        case QSFPDD_NIF_P1_INTR:
        case QSFPDD_NIF_P2_INTR:
        case QSFPDD_NIF_P3_INTR:
        case QSFPDD_NIF_P4_INTR:
        case QSFPDD_NIF_P5_INTR:
        case QSFPDD_NIF_P6_INTR:
        case QSFPDD_NIF_P7_INTR:
        case QSFPDD_NIF_P8_INTR:
        case QSFPDD_NIF_P9_INTR:
        case QSFPDD_NIF_P10_INTR:
        case QSFPDD_NIF_P11_INTR:
        case QSFPDD_NIF_P12_INTR:
        case QSFPDD_NIF_P13_INTR:
        case QSFPDD_NIF_P14_INTR:
        case QSFPDD_NIF_P15_INTR:
        case QSFPDD_NIF_P16_INTR:
        case QSFPDD_NIF_P17_INTR:
        case QSFPDD_NIF_P18_INTR:
        case QSFPDD_NIF_P19_INTR:
        case QSFPDD_NIF_P0_ABS:
        case QSFPDD_NIF_P1_ABS:
        case QSFPDD_NIF_P2_ABS:
        case QSFPDD_NIF_P3_ABS:
        case QSFPDD_NIF_P4_ABS:
        case QSFPDD_NIF_P5_ABS:
        case QSFPDD_NIF_P6_ABS:
        case QSFPDD_NIF_P7_ABS:
        case QSFPDD_NIF_P8_ABS:
        case QSFPDD_NIF_P9_ABS:
        case QSFPDD_NIF_P10_ABS:
        case QSFPDD_NIF_P11_ABS:
        case QSFPDD_NIF_P12_ABS:
        case QSFPDD_NIF_P13_ABS:
        case QSFPDD_NIF_P14_ABS:
        case QSFPDD_NIF_P15_ABS:
        case QSFPDD_NIF_P16_ABS:
        case QSFPDD_NIF_P17_ABS:
        case QSFPDD_NIF_P18_ABS:
        case QSFPDD_NIF_P19_ABS:
        case QSFPDD_NIF_P0_7_ABS_EVENT:
        case QSFPDD_NIF_P8_15_ABS_EVENT:
        case QSFPDD_NIF_P16_19_ABS_EVENT:
        case QSFPDD_NIF_P0_FUSE_INTR:
        case QSFPDD_NIF_P1_FUSE_INTR:
        case QSFPDD_NIF_P2_FUSE_INTR:
        case QSFPDD_NIF_P3_FUSE_INTR:
        case QSFPDD_NIF_P4_FUSE_INTR:
        case QSFPDD_NIF_P5_FUSE_INTR:
        case QSFPDD_NIF_P6_FUSE_INTR:
        case QSFPDD_NIF_P7_FUSE_INTR:
        case QSFPDD_NIF_P8_FUSE_INTR:
        case QSFPDD_NIF_P9_FUSE_INTR:
        case QSFPDD_NIF_P10_FUSE_INTR:
        case QSFPDD_NIF_P11_FUSE_INTR:
        case QSFPDD_NIF_P12_FUSE_INTR:
        case QSFPDD_NIF_P13_FUSE_INTR:
        case QSFPDD_NIF_P14_FUSE_INTR:
        case QSFPDD_NIF_P15_FUSE_INTR:
        case QSFPDD_NIF_P16_FUSE_INTR:
        case QSFPDD_NIF_P17_FUSE_INTR:
        case QSFPDD_NIF_P18_FUSE_INTR:
        case QSFPDD_NIF_P19_FUSE_INTR:

        case QSFPDD_FAB_P0_INTR:
        case QSFPDD_FAB_P1_INTR:
        case QSFPDD_FAB_P2_INTR:
        case QSFPDD_FAB_P3_INTR:
        case QSFPDD_FAB_P4_INTR:
        case QSFPDD_FAB_P5_INTR:
        case QSFPDD_FAB_P6_INTR:
        case QSFPDD_FAB_P7_INTR:
        case QSFPDD_FAB_P8_INTR:
        case QSFPDD_FAB_P9_INTR:
        case QSFPDD_FAB_P0_ABS:
        case QSFPDD_FAB_P1_ABS:
        case QSFPDD_FAB_P2_ABS:
        case QSFPDD_FAB_P3_ABS:
        case QSFPDD_FAB_P4_ABS:
        case QSFPDD_FAB_P5_ABS:
        case QSFPDD_FAB_P6_ABS:
        case QSFPDD_FAB_P7_ABS:
        case QSFPDD_FAB_P8_ABS:
        case QSFPDD_FAB_P9_ABS:
        case QSFPDD_FAB_P0_7_ABS_EVENT:
        case QSFPDD_FAB_P8_9_ABS_EVENT:
        case QSFPDD_FAB_P0_FUSE_INTR:
        case QSFPDD_FAB_P1_FUSE_INTR:
        case QSFPDD_FAB_P2_FUSE_INTR:
        case QSFPDD_FAB_P3_FUSE_INTR:
        case QSFPDD_FAB_P4_FUSE_INTR:
        case QSFPDD_FAB_P5_FUSE_INTR:
        case QSFPDD_FAB_P6_FUSE_INTR:
        case QSFPDD_FAB_P7_FUSE_INTR:
        case QSFPDD_FAB_P8_FUSE_INTR:
        case QSFPDD_FAB_P9_FUSE_INTR:
        case QSFPDD_NIF_P0_INTR_MASK:
        case QSFPDD_NIF_P1_INTR_MASK:
        case QSFPDD_NIF_P2_INTR_MASK:
        case QSFPDD_NIF_P3_INTR_MASK:
        case QSFPDD_NIF_P4_INTR_MASK:
        case QSFPDD_NIF_P5_INTR_MASK:
        case QSFPDD_NIF_P6_INTR_MASK:
        case QSFPDD_NIF_P7_INTR_MASK:
        case QSFPDD_NIF_P8_INTR_MASK:
        case QSFPDD_NIF_P9_INTR_MASK:
        case QSFPDD_NIF_P10_INTR_MASK:
        case QSFPDD_NIF_P11_INTR_MASK:
        case QSFPDD_NIF_P12_INTR_MASK:
        case QSFPDD_NIF_P13_INTR_MASK:
        case QSFPDD_NIF_P14_INTR_MASK:
        case QSFPDD_NIF_P15_INTR_MASK:
        case QSFPDD_NIF_P16_INTR_MASK:
        case QSFPDD_NIF_P17_INTR_MASK:
        case QSFPDD_NIF_P18_INTR_MASK:
        case QSFPDD_NIF_P19_INTR_MASK:
        case QSFPDD_NIF_P0_ABS_MASK:
        case QSFPDD_NIF_P1_ABS_MASK:
        case QSFPDD_NIF_P2_ABS_MASK:
        case QSFPDD_NIF_P3_ABS_MASK:
        case QSFPDD_NIF_P4_ABS_MASK:
        case QSFPDD_NIF_P5_ABS_MASK:
        case QSFPDD_NIF_P6_ABS_MASK:
        case QSFPDD_NIF_P7_ABS_MASK:
        case QSFPDD_NIF_P8_ABS_MASK:
        case QSFPDD_NIF_P9_ABS_MASK:
        case QSFPDD_NIF_P10_ABS_MASK:
        case QSFPDD_NIF_P11_ABS_MASK:
        case QSFPDD_NIF_P12_ABS_MASK:
        case QSFPDD_NIF_P13_ABS_MASK:
        case QSFPDD_NIF_P14_ABS_MASK:
        case QSFPDD_NIF_P15_ABS_MASK:
        case QSFPDD_NIF_P16_ABS_MASK:
        case QSFPDD_NIF_P17_ABS_MASK:
        case QSFPDD_NIF_P18_ABS_MASK:
        case QSFPDD_NIF_P19_ABS_MASK:
        case QSFPDD_NIF_P0_FUSE_INTR_MASK:
        case QSFPDD_NIF_P1_FUSE_INTR_MASK:
        case QSFPDD_NIF_P2_FUSE_INTR_MASK:
        case QSFPDD_NIF_P3_FUSE_INTR_MASK:
        case QSFPDD_NIF_P4_FUSE_INTR_MASK:
        case QSFPDD_NIF_P5_FUSE_INTR_MASK:
        case QSFPDD_NIF_P6_FUSE_INTR_MASK:
        case QSFPDD_NIF_P7_FUSE_INTR_MASK:
        case QSFPDD_NIF_P8_FUSE_INTR_MASK:
        case QSFPDD_NIF_P9_FUSE_INTR_MASK:
        case QSFPDD_NIF_P10_FUSE_INTR_MASK:
        case QSFPDD_NIF_P11_FUSE_INTR_MASK:
        case QSFPDD_NIF_P12_FUSE_INTR_MASK:
        case QSFPDD_NIF_P13_FUSE_INTR_MASK:
        case QSFPDD_NIF_P14_FUSE_INTR_MASK:
        case QSFPDD_NIF_P15_FUSE_INTR_MASK:
        case QSFPDD_NIF_P16_FUSE_INTR_MASK:
        case QSFPDD_NIF_P17_FUSE_INTR_MASK:
        case QSFPDD_NIF_P18_FUSE_INTR_MASK:
        case QSFPDD_NIF_P19_FUSE_INTR_MASK:

        case QSFPDD_FAB_P0_INTR_MASK:
        case QSFPDD_FAB_P1_INTR_MASK:
        case QSFPDD_FAB_P2_INTR_MASK:
        case QSFPDD_FAB_P3_INTR_MASK:
        case QSFPDD_FAB_P4_INTR_MASK:
        case QSFPDD_FAB_P5_INTR_MASK:
        case QSFPDD_FAB_P6_INTR_MASK:
        case QSFPDD_FAB_P7_INTR_MASK:
        case QSFPDD_FAB_P8_INTR_MASK:
        case QSFPDD_FAB_P9_INTR_MASK:
        case QSFPDD_FAB_P0_ABS_MASK:
        case QSFPDD_FAB_P1_ABS_MASK:
        case QSFPDD_FAB_P2_ABS_MASK:
        case QSFPDD_FAB_P3_ABS_MASK:
        case QSFPDD_FAB_P4_ABS_MASK:
        case QSFPDD_FAB_P5_ABS_MASK:
        case QSFPDD_FAB_P6_ABS_MASK:
        case QSFPDD_FAB_P7_ABS_MASK:
        case QSFPDD_FAB_P8_ABS_MASK:
        case QSFPDD_FAB_P9_ABS_MASK:
        case QSFPDD_FAB_P0_FUSE_INTR_MASK:
        case QSFPDD_FAB_P1_FUSE_INTR_MASK:
        case QSFPDD_FAB_P2_FUSE_INTR_MASK:
        case QSFPDD_FAB_P3_FUSE_INTR_MASK:
        case QSFPDD_FAB_P4_FUSE_INTR_MASK:
        case QSFPDD_FAB_P5_FUSE_INTR_MASK:
        case QSFPDD_FAB_P6_FUSE_INTR_MASK:
        case QSFPDD_FAB_P7_FUSE_INTR_MASK:
        case QSFPDD_FAB_P8_FUSE_INTR_MASK:
        case QSFPDD_FAB_P9_FUSE_INTR_MASK:

        case QSFPDD_NIF_P0_RST:
        case QSFPDD_NIF_P1_RST:
        case QSFPDD_NIF_P2_RST:
        case QSFPDD_NIF_P3_RST:
        case QSFPDD_NIF_P4_RST:
        case QSFPDD_NIF_P5_RST:
        case QSFPDD_NIF_P6_RST:
        case QSFPDD_NIF_P7_RST:
        case QSFPDD_NIF_P8_RST:
        case QSFPDD_NIF_P9_RST:
        case QSFPDD_NIF_P10_RST:
        case QSFPDD_NIF_P11_RST:
        case QSFPDD_NIF_P12_RST:
        case QSFPDD_NIF_P13_RST:
        case QSFPDD_NIF_P14_RST:
        case QSFPDD_NIF_P15_RST:
        case QSFPDD_NIF_P16_RST:
        case QSFPDD_NIF_P17_RST:
        case QSFPDD_NIF_P18_RST:
        case QSFPDD_NIF_P19_RST:
        case QSFPDD_FAB_P0_RST:
        case QSFPDD_FAB_P1_RST:
        case QSFPDD_FAB_P2_RST:
        case QSFPDD_FAB_P3_RST:
        case QSFPDD_FAB_P4_RST:
        case QSFPDD_FAB_P5_RST:
        case QSFPDD_FAB_P6_RST:
        case QSFPDD_FAB_P7_RST:
        case QSFPDD_FAB_P8_RST:
        case QSFPDD_FAB_P9_RST:
        case QSFPDD_NIF_P0_LP_MODE:
        case QSFPDD_NIF_P1_LP_MODE:
        case QSFPDD_NIF_P2_LP_MODE:
        case QSFPDD_NIF_P3_LP_MODE:
        case QSFPDD_NIF_P4_LP_MODE:
        case QSFPDD_NIF_P5_LP_MODE:
        case QSFPDD_NIF_P6_LP_MODE:
        case QSFPDD_NIF_P7_LP_MODE:
        case QSFPDD_NIF_P8_LP_MODE:
        case QSFPDD_NIF_P9_LP_MODE:
        case QSFPDD_NIF_P10_LP_MODE:
        case QSFPDD_NIF_P11_LP_MODE:
        case QSFPDD_NIF_P12_LP_MODE:
        case QSFPDD_NIF_P13_LP_MODE:
        case QSFPDD_NIF_P14_LP_MODE:
        case QSFPDD_NIF_P15_LP_MODE:
        case QSFPDD_NIF_P16_LP_MODE:
        case QSFPDD_NIF_P17_LP_MODE:
        case QSFPDD_NIF_P18_LP_MODE:
        case QSFPDD_NIF_P19_LP_MODE:
        case QSFPDD_FAB_P0_LP_MODE:
        case QSFPDD_FAB_P1_LP_MODE:
        case QSFPDD_FAB_P2_LP_MODE:
        case QSFPDD_FAB_P3_LP_MODE:
        case QSFPDD_FAB_P4_LP_MODE:
        case QSFPDD_FAB_P5_LP_MODE:
        case QSFPDD_FAB_P6_LP_MODE:
        case QSFPDD_FAB_P7_LP_MODE:
        case QSFPDD_FAB_P8_LP_MODE:
        case QSFPDD_FAB_P9_LP_MODE:

        case QSFPDD_FAB_P0_LED_STATUS:
        case QSFPDD_FAB_P1_LED_STATUS:
        case QSFPDD_FAB_P2_LED_STATUS:
        case QSFPDD_FAB_P3_LED_STATUS:
        case QSFPDD_FAB_P4_LED_STATUS:
        case QSFPDD_FAB_P5_LED_STATUS:
        case QSFPDD_FAB_P6_LED_STATUS:
        case QSFPDD_FAB_P7_LED_STATUS:
        case QSFPDD_FAB_P8_LED_STATUS:
        case QSFPDD_FAB_P9_LED_STATUS:

        case QSFPDD_NIF_P0_I2C_STUCK:
        case QSFPDD_NIF_P1_I2C_STUCK:
        case QSFPDD_NIF_P2_I2C_STUCK:
        case QSFPDD_NIF_P3_I2C_STUCK:
        case QSFPDD_NIF_P4_I2C_STUCK:
        case QSFPDD_NIF_P5_I2C_STUCK:
        case QSFPDD_NIF_P6_I2C_STUCK:
        case QSFPDD_NIF_P7_I2C_STUCK:
        case QSFPDD_NIF_P8_I2C_STUCK:
        case QSFPDD_NIF_P9_I2C_STUCK:
        case QSFPDD_NIF_P10_I2C_STUCK:
        case QSFPDD_NIF_P11_I2C_STUCK:
        case QSFPDD_NIF_P12_I2C_STUCK:
        case QSFPDD_NIF_P13_I2C_STUCK:
        case QSFPDD_NIF_P14_I2C_STUCK:
        case QSFPDD_NIF_P15_I2C_STUCK:
        case QSFPDD_NIF_P16_I2C_STUCK:
        case QSFPDD_NIF_P17_I2C_STUCK:
        case QSFPDD_NIF_P18_I2C_STUCK:
        case QSFPDD_NIF_P19_I2C_STUCK:
        case QSFPDD_FAB_P0_I2C_STUCK:
        case QSFPDD_FAB_P1_I2C_STUCK:
        case QSFPDD_FAB_P2_I2C_STUCK:
        case QSFPDD_FAB_P3_I2C_STUCK:
        case QSFPDD_FAB_P4_I2C_STUCK:
        case QSFPDD_FAB_P5_I2C_STUCK:
        case QSFPDD_FAB_P6_I2C_STUCK:
        case QSFPDD_FAB_P7_I2C_STUCK:
        case QSFPDD_FAB_P8_I2C_STUCK:
        case QSFPDD_FAB_P9_I2C_STUCK:
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

    switch (attr->index) {

        //CPLD 1
        case SYSTEM_LED_STATUS:
        case FAN_LED_STATUS:
        case PSU_0_LED_STATUS:
        case PSU_1_LED_STATUS:
        case SYNC_LED_STATUS:
        case SFP_0_LED_STATUS:
        case SFP_1_LED_STATUS:
        case EVENT_DETECT_CTRL:
        //CPLD 2
        case QSFPDD_NIF_P20_INTR_MASK:
        case QSFPDD_NIF_P21_INTR_MASK:
        case QSFPDD_NIF_P22_INTR_MASK:
        case QSFPDD_NIF_P23_INTR_MASK:
        case QSFPDD_NIF_P24_INTR_MASK:
        case QSFPDD_NIF_P25_INTR_MASK:
        case QSFPDD_NIF_P26_INTR_MASK:
        case QSFPDD_NIF_P27_INTR_MASK:
        case QSFPDD_NIF_P28_INTR_MASK:
        case QSFPDD_NIF_P29_INTR_MASK:
        case QSFPDD_NIF_P30_INTR_MASK:
        case QSFPDD_NIF_P31_INTR_MASK:
        case QSFPDD_NIF_P32_INTR_MASK:
        case QSFPDD_NIF_P33_INTR_MASK:
        case QSFPDD_NIF_P34_INTR_MASK:
        case QSFPDD_NIF_P35_INTR_MASK:

        case QSFPDD_NIF_P20_ABS_MASK:
        case QSFPDD_NIF_P21_ABS_MASK:
        case QSFPDD_NIF_P22_ABS_MASK:
        case QSFPDD_NIF_P23_ABS_MASK:
        case QSFPDD_NIF_P24_ABS_MASK:
        case QSFPDD_NIF_P25_ABS_MASK:
        case QSFPDD_NIF_P26_ABS_MASK:
        case QSFPDD_NIF_P27_ABS_MASK:
        case QSFPDD_NIF_P28_ABS_MASK:
        case QSFPDD_NIF_P29_ABS_MASK:
        case QSFPDD_NIF_P30_ABS_MASK:
        case QSFPDD_NIF_P31_ABS_MASK:
        case QSFPDD_NIF_P32_ABS_MASK:
        case QSFPDD_NIF_P33_ABS_MASK:
        case QSFPDD_NIF_P34_ABS_MASK:
        case QSFPDD_NIF_P35_ABS_MASK:

        case QSFPDD_NIF_P20_FUSE_INTR_MASK:
        case QSFPDD_NIF_P21_FUSE_INTR_MASK:
        case QSFPDD_NIF_P22_FUSE_INTR_MASK:
        case QSFPDD_NIF_P23_FUSE_INTR_MASK:
        case QSFPDD_NIF_P24_FUSE_INTR_MASK:
        case QSFPDD_NIF_P25_FUSE_INTR_MASK:
        case QSFPDD_NIF_P26_FUSE_INTR_MASK:
        case QSFPDD_NIF_P27_FUSE_INTR_MASK:
        case QSFPDD_NIF_P28_FUSE_INTR_MASK:
        case QSFPDD_NIF_P29_FUSE_INTR_MASK:
        case QSFPDD_NIF_P30_FUSE_INTR_MASK:
        case QSFPDD_NIF_P31_FUSE_INTR_MASK:
        case QSFPDD_NIF_P32_FUSE_INTR_MASK:
        case QSFPDD_NIF_P33_FUSE_INTR_MASK:
        case QSFPDD_NIF_P34_FUSE_INTR_MASK:
        case QSFPDD_NIF_P35_FUSE_INTR_MASK:

        case QSFPDD_FAB_P10_INTR_MASK:
        case QSFPDD_FAB_P11_INTR_MASK:
        case QSFPDD_FAB_P12_INTR_MASK:
        case QSFPDD_FAB_P13_INTR_MASK:
        case QSFPDD_FAB_P14_INTR_MASK:
        case QSFPDD_FAB_P15_INTR_MASK:
        case QSFPDD_FAB_P16_INTR_MASK:
        case QSFPDD_FAB_P17_INTR_MASK:
        case QSFPDD_FAB_P18_INTR_MASK:
        case QSFPDD_FAB_P19_INTR_MASK:
        case QSFPDD_FAB_P10_ABS_MASK:
        case QSFPDD_FAB_P11_ABS_MASK:
        case QSFPDD_FAB_P12_ABS_MASK:
        case QSFPDD_FAB_P13_ABS_MASK:
        case QSFPDD_FAB_P14_ABS_MASK:
        case QSFPDD_FAB_P15_ABS_MASK:
        case QSFPDD_FAB_P16_ABS_MASK:
        case QSFPDD_FAB_P17_ABS_MASK:
        case QSFPDD_FAB_P18_ABS_MASK:
        case QSFPDD_FAB_P19_ABS_MASK:
        case QSFPDD_FAB_P10_FUSE_INTR_MASK:
        case QSFPDD_FAB_P11_FUSE_INTR_MASK:
        case QSFPDD_FAB_P12_FUSE_INTR_MASK:
        case QSFPDD_FAB_P13_FUSE_INTR_MASK:
        case QSFPDD_FAB_P14_FUSE_INTR_MASK:
        case QSFPDD_FAB_P15_FUSE_INTR_MASK:
        case QSFPDD_FAB_P16_FUSE_INTR_MASK:
        case QSFPDD_FAB_P17_FUSE_INTR_MASK:
        case QSFPDD_FAB_P18_FUSE_INTR_MASK:
        case QSFPDD_FAB_P19_FUSE_INTR_MASK:

        case QSFPDD_NIF_P20_RST:
        case QSFPDD_NIF_P21_RST:
        case QSFPDD_NIF_P22_RST:
        case QSFPDD_NIF_P23_RST:
        case QSFPDD_NIF_P24_RST:
        case QSFPDD_NIF_P25_RST:
        case QSFPDD_NIF_P26_RST:
        case QSFPDD_NIF_P27_RST:
        case QSFPDD_NIF_P28_RST:
        case QSFPDD_NIF_P29_RST:
        case QSFPDD_NIF_P30_RST:
        case QSFPDD_NIF_P31_RST:
        case QSFPDD_NIF_P32_RST:
        case QSFPDD_NIF_P33_RST:
        case QSFPDD_NIF_P34_RST:
        case QSFPDD_NIF_P35_RST:
        case QSFPDD_FAB_P10_RST:
        case QSFPDD_FAB_P11_RST:
        case QSFPDD_FAB_P12_RST:
        case QSFPDD_FAB_P13_RST:
        case QSFPDD_FAB_P14_RST:
        case QSFPDD_FAB_P15_RST:
        case QSFPDD_FAB_P16_RST:
        case QSFPDD_FAB_P17_RST:
        case QSFPDD_FAB_P18_RST:
        case QSFPDD_FAB_P19_RST:
        case QSFPDD_NIF_P20_LP_MODE:
        case QSFPDD_NIF_P21_LP_MODE:
        case QSFPDD_NIF_P22_LP_MODE:
        case QSFPDD_NIF_P23_LP_MODE:
        case QSFPDD_NIF_P24_LP_MODE:
        case QSFPDD_NIF_P25_LP_MODE:
        case QSFPDD_NIF_P26_LP_MODE:
        case QSFPDD_NIF_P27_LP_MODE:
        case QSFPDD_NIF_P28_LP_MODE:
        case QSFPDD_NIF_P29_LP_MODE:
        case QSFPDD_NIF_P30_LP_MODE:
        case QSFPDD_NIF_P31_LP_MODE:
        case QSFPDD_NIF_P32_LP_MODE:
        case QSFPDD_NIF_P33_LP_MODE:
        case QSFPDD_NIF_P34_LP_MODE:
        case QSFPDD_NIF_P35_LP_MODE:
        case QSFPDD_FAB_P10_LP_MODE:
        case QSFPDD_FAB_P11_LP_MODE:
        case QSFPDD_FAB_P12_LP_MODE:
        case QSFPDD_FAB_P13_LP_MODE:
        case QSFPDD_FAB_P14_LP_MODE:
        case QSFPDD_FAB_P15_LP_MODE:
        case QSFPDD_FAB_P16_LP_MODE:
        case QSFPDD_FAB_P17_LP_MODE:
        case QSFPDD_FAB_P18_LP_MODE:
        case QSFPDD_FAB_P19_LP_MODE:

        case QSFPDD_FAB_P10_LED_STATUS:
        case QSFPDD_FAB_P11_LED_STATUS:
        case QSFPDD_FAB_P12_LED_STATUS:
        case QSFPDD_FAB_P13_LED_STATUS:
        case QSFPDD_FAB_P14_LED_STATUS:
        case QSFPDD_FAB_P15_LED_STATUS:
        case QSFPDD_FAB_P16_LED_STATUS:
        case QSFPDD_FAB_P17_LED_STATUS:
        case QSFPDD_FAB_P18_LED_STATUS:
        case QSFPDD_FAB_P19_LED_STATUS:

        //CPLD 3
        // change to fpga
        case SFP28_P37_TS:
        case SFP28_P36_TS:
        case MGMT_P1_TS:
        case MGMT_P0_TS:
        case SFP28_P37_RS:
        case SFP28_P36_RS:
        case MGMT_P1_RS:
        case MGMT_P0_RS:
        case SFP28_P37_TX_DIS:
        case SFP28_P36_TX_DIS:
        case MGMT_P1_TX_DIS:
        case MGMT_P0_TX_DIS:
        case SFP28_P37_TX_FLT:
        case SFP28_P36_TX_FLT:
        case MGMT_P1_TX_FLT:
        case MGMT_P0_TX_FLT:
        case SFP28_P37_RX_LOS:
        case SFP28_P36_RX_LOS:
        case MGMT_P1_RX_LOS:
        case MGMT_P0_RX_LOS:
        case SFP28_P37_ABS:
        case SFP28_P36_ABS:
        case MGMT_P1_ABS:
        case MGMT_P0_ABS:
        case SFP28_P37_TX_FLT_MASK:
        case SFP28_P36_TX_FLT_MASK:
        case MGMT_P1_TX_FLT_MASK:
        case MGMT_P0_TX_FLT_MASK:
        case SFP28_P37_RX_LOS_MASK:
        case SFP28_P36_RX_LOS_MASK:
        case MGMT_P1_RX_LOS_MASK:
        case MGMT_P0_RX_LOS_MASK:
        case SFP28_P37_ABS_MASK:
        case SFP28_P36_ABS_MASK:
        case MGMT_P1_ABS_MASK:
        case MGMT_P0_ABS_MASK:

        //CPLD 4
        case QSFPDD_NIF_P0_INTR_MASK:
        case QSFPDD_NIF_P1_INTR_MASK:
        case QSFPDD_NIF_P2_INTR_MASK:
        case QSFPDD_NIF_P3_INTR_MASK:
        case QSFPDD_NIF_P4_INTR_MASK:
        case QSFPDD_NIF_P5_INTR_MASK:
        case QSFPDD_NIF_P6_INTR_MASK:
        case QSFPDD_NIF_P7_INTR_MASK:
        case QSFPDD_NIF_P8_INTR_MASK:
        case QSFPDD_NIF_P9_INTR_MASK:
        case QSFPDD_NIF_P10_INTR_MASK:
        case QSFPDD_NIF_P11_INTR_MASK:
        case QSFPDD_NIF_P12_INTR_MASK:
        case QSFPDD_NIF_P13_INTR_MASK:
        case QSFPDD_NIF_P14_INTR_MASK:
        case QSFPDD_NIF_P15_INTR_MASK:
        case QSFPDD_NIF_P16_INTR_MASK:
        case QSFPDD_NIF_P17_INTR_MASK:
        case QSFPDD_NIF_P18_INTR_MASK:
        case QSFPDD_NIF_P19_INTR_MASK:
        case QSFPDD_NIF_P0_ABS_MASK:
        case QSFPDD_NIF_P1_ABS_MASK:
        case QSFPDD_NIF_P2_ABS_MASK:
        case QSFPDD_NIF_P3_ABS_MASK:
        case QSFPDD_NIF_P4_ABS_MASK:
        case QSFPDD_NIF_P5_ABS_MASK:
        case QSFPDD_NIF_P6_ABS_MASK:
        case QSFPDD_NIF_P7_ABS_MASK:
        case QSFPDD_NIF_P8_ABS_MASK:
        case QSFPDD_NIF_P9_ABS_MASK:
        case QSFPDD_NIF_P10_ABS_MASK:
        case QSFPDD_NIF_P11_ABS_MASK:
        case QSFPDD_NIF_P12_ABS_MASK:
        case QSFPDD_NIF_P13_ABS_MASK:
        case QSFPDD_NIF_P14_ABS_MASK:
        case QSFPDD_NIF_P15_ABS_MASK:
        case QSFPDD_NIF_P16_ABS_MASK:
        case QSFPDD_NIF_P17_ABS_MASK:
        case QSFPDD_NIF_P18_ABS_MASK:
        case QSFPDD_NIF_P19_ABS_MASK:
        case QSFPDD_NIF_P0_FUSE_INTR_MASK:
        case QSFPDD_NIF_P1_FUSE_INTR_MASK:
        case QSFPDD_NIF_P2_FUSE_INTR_MASK:
        case QSFPDD_NIF_P3_FUSE_INTR_MASK:
        case QSFPDD_NIF_P4_FUSE_INTR_MASK:
        case QSFPDD_NIF_P5_FUSE_INTR_MASK:
        case QSFPDD_NIF_P6_FUSE_INTR_MASK:
        case QSFPDD_NIF_P7_FUSE_INTR_MASK:
        case QSFPDD_NIF_P8_FUSE_INTR_MASK:
        case QSFPDD_NIF_P9_FUSE_INTR_MASK:
        case QSFPDD_NIF_P10_FUSE_INTR_MASK:
        case QSFPDD_NIF_P11_FUSE_INTR_MASK:
        case QSFPDD_NIF_P12_FUSE_INTR_MASK:
        case QSFPDD_NIF_P13_FUSE_INTR_MASK:
        case QSFPDD_NIF_P14_FUSE_INTR_MASK:
        case QSFPDD_NIF_P15_FUSE_INTR_MASK:
        case QSFPDD_NIF_P16_FUSE_INTR_MASK:
        case QSFPDD_NIF_P17_FUSE_INTR_MASK:
        case QSFPDD_NIF_P18_FUSE_INTR_MASK:
        case QSFPDD_NIF_P19_FUSE_INTR_MASK:

        case QSFPDD_FAB_P0_INTR_MASK:
        case QSFPDD_FAB_P1_INTR_MASK:
        case QSFPDD_FAB_P2_INTR_MASK:
        case QSFPDD_FAB_P3_INTR_MASK:
        case QSFPDD_FAB_P4_INTR_MASK:
        case QSFPDD_FAB_P5_INTR_MASK:
        case QSFPDD_FAB_P6_INTR_MASK:
        case QSFPDD_FAB_P7_INTR_MASK:
        case QSFPDD_FAB_P8_INTR_MASK:
        case QSFPDD_FAB_P9_INTR_MASK:
        case QSFPDD_FAB_P0_ABS_MASK:
        case QSFPDD_FAB_P1_ABS_MASK:
        case QSFPDD_FAB_P2_ABS_MASK:
        case QSFPDD_FAB_P3_ABS_MASK:
        case QSFPDD_FAB_P4_ABS_MASK:
        case QSFPDD_FAB_P5_ABS_MASK:
        case QSFPDD_FAB_P6_ABS_MASK:
        case QSFPDD_FAB_P7_ABS_MASK:
        case QSFPDD_FAB_P8_ABS_MASK:
        case QSFPDD_FAB_P9_ABS_MASK:
        case QSFPDD_FAB_P0_FUSE_INTR_MASK:
        case QSFPDD_FAB_P1_FUSE_INTR_MASK:
        case QSFPDD_FAB_P2_FUSE_INTR_MASK:
        case QSFPDD_FAB_P3_FUSE_INTR_MASK:
        case QSFPDD_FAB_P4_FUSE_INTR_MASK:
        case QSFPDD_FAB_P5_FUSE_INTR_MASK:
        case QSFPDD_FAB_P6_FUSE_INTR_MASK:
        case QSFPDD_FAB_P7_FUSE_INTR_MASK:
        case QSFPDD_FAB_P8_FUSE_INTR_MASK:
        case QSFPDD_FAB_P9_FUSE_INTR_MASK:

        case QSFPDD_NIF_P0_RST:
        case QSFPDD_NIF_P1_RST:
        case QSFPDD_NIF_P2_RST:
        case QSFPDD_NIF_P3_RST:
        case QSFPDD_NIF_P4_RST:
        case QSFPDD_NIF_P5_RST:
        case QSFPDD_NIF_P6_RST:
        case QSFPDD_NIF_P7_RST:
        case QSFPDD_NIF_P8_RST:
        case QSFPDD_NIF_P9_RST:
        case QSFPDD_NIF_P10_RST:
        case QSFPDD_NIF_P11_RST:
        case QSFPDD_NIF_P12_RST:
        case QSFPDD_NIF_P13_RST:
        case QSFPDD_NIF_P14_RST:
        case QSFPDD_NIF_P15_RST:
        case QSFPDD_NIF_P16_RST:
        case QSFPDD_NIF_P17_RST:
        case QSFPDD_NIF_P18_RST:
        case QSFPDD_NIF_P19_RST:
        case QSFPDD_FAB_P0_RST:
        case QSFPDD_FAB_P1_RST:
        case QSFPDD_FAB_P2_RST:
        case QSFPDD_FAB_P3_RST:
        case QSFPDD_FAB_P4_RST:
        case QSFPDD_FAB_P5_RST:
        case QSFPDD_FAB_P6_RST:
        case QSFPDD_FAB_P7_RST:
        case QSFPDD_FAB_P8_RST:
        case QSFPDD_FAB_P9_RST:
        case QSFPDD_NIF_P0_LP_MODE:
        case QSFPDD_NIF_P1_LP_MODE:
        case QSFPDD_NIF_P2_LP_MODE:
        case QSFPDD_NIF_P3_LP_MODE:
        case QSFPDD_NIF_P4_LP_MODE:
        case QSFPDD_NIF_P5_LP_MODE:
        case QSFPDD_NIF_P6_LP_MODE:
        case QSFPDD_NIF_P7_LP_MODE:
        case QSFPDD_NIF_P8_LP_MODE:
        case QSFPDD_NIF_P9_LP_MODE:
        case QSFPDD_NIF_P10_LP_MODE:
        case QSFPDD_NIF_P11_LP_MODE:
        case QSFPDD_NIF_P12_LP_MODE:
        case QSFPDD_NIF_P13_LP_MODE:
        case QSFPDD_NIF_P14_LP_MODE:
        case QSFPDD_NIF_P15_LP_MODE:
        case QSFPDD_NIF_P16_LP_MODE:
        case QSFPDD_NIF_P17_LP_MODE:
        case QSFPDD_NIF_P18_LP_MODE:
        case QSFPDD_NIF_P19_LP_MODE:
        case QSFPDD_FAB_P0_LP_MODE:
        case QSFPDD_FAB_P1_LP_MODE:
        case QSFPDD_FAB_P2_LP_MODE:
        case QSFPDD_FAB_P3_LP_MODE:
        case QSFPDD_FAB_P4_LP_MODE:
        case QSFPDD_FAB_P5_LP_MODE:
        case QSFPDD_FAB_P6_LP_MODE:
        case QSFPDD_FAB_P7_LP_MODE:
        case QSFPDD_FAB_P8_LP_MODE:
        case QSFPDD_FAB_P9_LP_MODE:

        case QSFPDD_FAB_P0_LED_STATUS:
        case QSFPDD_FAB_P1_LED_STATUS:
        case QSFPDD_FAB_P2_LED_STATUS:
        case QSFPDD_FAB_P3_LED_STATUS:
        case QSFPDD_FAB_P4_LED_STATUS:
        case QSFPDD_FAB_P5_LED_STATUS:
        case QSFPDD_FAB_P6_LED_STATUS:
        case QSFPDD_FAB_P7_LED_STATUS:
        case QSFPDD_FAB_P8_LED_STATUS:
        case QSFPDD_FAB_P9_LED_STATUS:

            reg = attr_reg[attr->index].reg;
            mask= attr_reg[attr->index].mask;
            break;
        default:
            return -EINVAL;
    }
    return cpld_reg_write(dev, buf, count, reg, mask);
}

/* get cpld register value */
int _cpld_reg_read(struct device *dev,
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

/* get cpld register value without lock */
int _cpld_reg_read_nolock(struct device *dev,
                    u8 reg,
                    u8 mask)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct cpld_data *data = i2c_mux_priv(muxc);
    int reg_val;

    I2C_READ_BYTE_DATA_NOLOCK(reg_val, client, reg);

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

int _cpld_reg_write(struct device *dev,
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

int _cpld_reg_write_nolock(struct device *dev,
                    u8 reg,
                    u8 reg_val)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct cpld_data *data = i2c_mux_priv(muxc);
    int ret = 0;

    I2C_WRITE_BYTE_DATA_NOLOCK(ret, client, reg, reg_val);

    return ret;
}

/* set cpld register value */
static ssize_t cpld_reg_write(struct device *dev,
                    const char *buf,
                    size_t count,
                    u8 reg,
                    u8 mask)
{
    u8 reg_val, shift;
    int reg_val_now;
    int ret = 0;

    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct cpld_data *data = i2c_mux_priv(muxc);

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    // lock mutex during register access
    mutex_lock(&data->access_lock);

    //apply continuous bits operation if mask is specified, discontinuous bits are not supported
    if (mask != MASK_ALL) {
        reg_val_now = _cpld_reg_read_nolock(dev, reg, MASK_ALL);
        if (unlikely(reg_val_now < 0)) {
            mutex_unlock(&data->access_lock);
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

    ret = _cpld_reg_write_nolock(dev, reg, reg_val);

    // unlock mutex after register access
    mutex_unlock(&data->access_lock);

    if (unlikely(ret < 0)) {
        dev_err(dev, "cpld_reg_write() error, return=%d\n", ret);
        return ret;
    }

    return count;
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
    return EINVAL;
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
    case cpld4:
        status = sysfs_create_group(&client->dev.kobj,
                    &cpld4_group);
        break;
    case cpld5:
        status = sysfs_create_group(&client->dev.kobj,
                    &cpld5_group);
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
    case fpga:
        sysfs_remove_group(&client->dev.kobj, &fpga_group);
        break;
    case cpld4:
        sysfs_remove_group(&client->dev.kobj, &cpld4_group);
        break;
    case cpld5:
        sysfs_remove_group(&client->dev.kobj, &cpld5_group);
        break;
    }
    return status;
}

/* cpld drvier remove */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
static int
#else
static void
#endif
cpld_remove(struct i2c_client *client)
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
    case fpga:
        sysfs_remove_group(&client->dev.kobj, &fpga_group);
        break;
    case cpld4:
        sysfs_remove_group(&client->dev.kobj, &cpld4_group);
        break;
    case cpld5:
        sysfs_remove_group(&client->dev.kobj, &cpld5_group);
        break;
    default:
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
        .name = "x86_64_ufispace_s9720_56ed_cpld",
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
MODULE_DESCRIPTION("x86_64_ufispace_s9720_56ed_cpld driver");
MODULE_VERSION("0.0.1");
MODULE_LICENSE("GPL");

module_init(cpld_init);
module_exit(cpld_exit);
