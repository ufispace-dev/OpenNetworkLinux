/*
 * A i2c cpld driver for the ufispace_s9620_32e
 *
 * Copyright (C) 2025 UfiSpace Technology Corporation.
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

#include <linux/slab.h>
#include <linux/list.h>
#include <linux/dmi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/version.h>

#include "x86-64-ufispace-s9620-32e-cpld-main.h"

static bool mux_en = true;
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
    EVENT_DETECT_CTRL,

    // CPLD 2/3/4
    CPLD_I2C_STUCK,
    CPLD_I2C_CONTROL,
    CPLD_I2C_IDLE,
    CPLD_I2C_RELAY,

    // CPLD 1
    BRD_SKU_ID,
    BRD_HW_ID,
    BRD_DEPH_ID,
    BRD_BUILD_ID,
    BRD_ID_TYPE,
    CPLD_CHIP_TYPE,

    MAC_INTR,
    CPLD_25GPHY_INTR,
    CPLD_FRU_INTR,
    NTM_INTR,
    THERMAL_ALERT_1_INTR,
    THERMAL_ALERT_2_INTR,
    THERMAL_ALERT_3_INTR,
    MISC_INTR,
    SYSTEM_INTR,

    CPLD_2_INTR,
    CPLD_3_INTR,
    CPLD_4_INTR,
    PSU_0_INTR,
    PSU_1_INTR,

    MAC_HBM_TEMP_ALERT,
    MB_LM75_TEMP_ALERT,
    EXT_LM75_TEMP_ALERT,
    PHY_LM75_CPLD5_INTR,
    USB_OCP,
    FPGA_INTR,
    THERMAL_ALERT_INTR,
    PSU_CPLD_INTR,
    MAIN_CHIP_INTR,
    PTP_INTR,

    MAC_INTR_MASK,
    CPLD_25GPHY_INTR_MASK,
    CPLD_FRU_INTR_MASK,
    NTM_INTR_MASK,
    THERMAL_ALERT_1_INTR_MASK,
    THERMAL_ALERT_2_INTR_MASK,
    THERMAL_ALERT_3_INTR_MASK,
    MISC_INTR_MASK,
    SYSTEM_INTR_MASK,

    CPLD_2_INTR_MASK,
    CPLD_3_INTR_MASK,
    CPLD_4_INTR_MASK,
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
    THERMAL_ALERT_INTR_MASK,
    PSU_CPLD_INTR_MASK,
    MAIN_CHIP_INTR_MASK,
    PTP_INTR_MASK,

    MAC_INTR_EVENT,
    CPLD_25GPHY_INTR_EVENT,
    CPLD_FRU_INTR_EVENT,
    NTM_INTR_EVENT,
    THERMAL_ALERT_1_INTR_EVENT,
    THERMAL_ALERT_2_INTR_EVENT,
    THERMAL_ALERT_3_INTR_EVENT,
    MISC_INTR_EVENT,
    SYSTEM_INTR_EVENT,

    MAC_TEMP_ALERT_EVENT,
    TEMP_ALERT_EVENT,

    MAC_RST,
    BIOS_FLASH_RST,
    BMC_RST,
    BMC_PCIE_RST,
    BMC_LPC_RST,
    BMC_WDT_RST,
    NTM_RST,

    USB_REDRIVER_EXT_RST,
    MAC_QSPI_FLASH_RST,
    CP2130_RST,
    USB_REDRIVER_NTM_RST,
    USB_REDRIVER_MB_RST,
    USB_CURR_LIMITER_EN,

    CPLD_2_RST,
    CPLD_3_RST,
    FPGA_RST,

    I2C_MUX_RST,
    I2C_MUX_9548_1_RST,
    I2C_MUX_9548_2_RST,
    I2C_MUX_FAN_RST,

    BCM82399_RST,
    CP2102_RST,
    RC32312_RST,
    I210_RST,
    IOEXP_RST,
    FPGA_PCIE_RST,
    I2C_MUX_OOB_RST,
    I2C_MUX_TS_RST,

    PSU_RST,

    PSU_0_PRSNT,
    PSU_1_PRSNT,
    PSU_0_ACIN,
    PSU_1_ACIN,
    PSU_0_PG,
    PSU_1_PG,

    PHY_BOOT,
    MB_EEPROM_WP,

    CPU_MUX_SEL,
    PSU_MUX_SEL,
    FPGA_QSPI_SEL,
    BMC_UART_MUX_SEL,
    SYSTEM_UART_MUX_SEL,

    FPGA_CTRL,
    FPGA_NCONFIG,
    FPGA_CONFIG_DONE,

    CPLD_SYSTEM_LED_SYS,
    SYSTEM_LED_STATUS,
    SYSTEM_LED_SPEED,
    SYSTEM_LED_BLINK,
    SYSTEM_LED_ONOFF,

    CPLD_SYSTEM_LED_FAN,
    FAN_LED_STATUS,
    FAN_LED_SPEED,
    FAN_LED_BLINK,
    FAN_LED_ONOFF,

    CPLD_SYSTEM_LED_PSU_0,
    PSU_0_LED_STATUS,
    PSU_0_LED_SPEED,
    PSU_0_LED_BLINK,
    PSU_0_LED_ONOFF,

    CPLD_SYSTEM_LED_PSU_1,
    PSU_1_LED_STATUS,
    PSU_1_LED_SPEED,
    PSU_1_LED_BLINK,
    PSU_1_LED_ONOFF,

    CPLD_SYSTEM_LED_SYNC,
    SYNC_LED_STATUS,
    SYNC_LED_SPEED,
    SYNC_LED_BLINK,
    SYNC_LED_ONOFF,

    CPLD_SYSTEM_LED_ID,
    ID_LED_STATUS,
    ID_LED_SPEED,
    ID_LED_BLINK,
    ID_LED_ONOFF,

    LED_CLEAR,

    MGMT_0_LED_STATUS,
    MGMT_0_LED_SPEED,
    MGMT_0_LED_BLINK,
    MGMT_0_LED_ONOFF,
    MGMT_1_LED_STATUS,
    MGMT_1_LED_SPEED,
    MGMT_1_LED_BLINK,
    MGMT_1_LED_ONOFF,
    MGMT_P0_LED,
    MGMT_P1_LED,

    //CPLD 2
    QSFPDD_P0_INTR,
    QSFPDD_P1_INTR,
    QSFPDD_P2_INTR,
    QSFPDD_P3_INTR,
    QSFPDD_P4_INTR,
    QSFPDD_P5_INTR,
    QSFPDD_P6_INTR,
    QSFPDD_P7_INTR,
    QSFPDD_P8_INTR,
    QSFPDD_P9_INTR,
    QSFPDD_P10_INTR,
    QSFPDD_P11_INTR,
    QSFPDD_P12_INTR,
    QSFPDD_P13_INTR,
    QSFPDD_P14_INTR,
    QSFPDD_P15_INTR,

    QSFPDD_P0_ABS,
    QSFPDD_P1_ABS,
    QSFPDD_P2_ABS,
    QSFPDD_P3_ABS,
    QSFPDD_P4_ABS,
    QSFPDD_P5_ABS,
    QSFPDD_P6_ABS,
    QSFPDD_P7_ABS,
    QSFPDD_P8_ABS,
    QSFPDD_P9_ABS,
    QSFPDD_P10_ABS,
    QSFPDD_P11_ABS,
    QSFPDD_P12_ABS,
    QSFPDD_P13_ABS,
    QSFPDD_P14_ABS,
    QSFPDD_P15_ABS,

    QSFPDD_P0_FUSE_INTR,
    QSFPDD_P1_FUSE_INTR,
    QSFPDD_P2_FUSE_INTR,
    QSFPDD_P3_FUSE_INTR,
    QSFPDD_P4_FUSE_INTR,
    QSFPDD_P5_FUSE_INTR,
    QSFPDD_P6_FUSE_INTR,
    QSFPDD_P7_FUSE_INTR,
    QSFPDD_P8_FUSE_INTR,
    QSFPDD_P9_FUSE_INTR,
    QSFPDD_P10_FUSE_INTR,
    QSFPDD_P11_FUSE_INTR,
    QSFPDD_P12_FUSE_INTR,
    QSFPDD_P13_FUSE_INTR,
    QSFPDD_P14_FUSE_INTR,
    QSFPDD_P15_FUSE_INTR,

    QSFPDD_P0_INTR_MASK,
    QSFPDD_P1_INTR_MASK,
    QSFPDD_P2_INTR_MASK,
    QSFPDD_P3_INTR_MASK,
    QSFPDD_P4_INTR_MASK,
    QSFPDD_P5_INTR_MASK,
    QSFPDD_P6_INTR_MASK,
    QSFPDD_P7_INTR_MASK,
    QSFPDD_P8_INTR_MASK,
    QSFPDD_P9_INTR_MASK,
    QSFPDD_P10_INTR_MASK,
    QSFPDD_P11_INTR_MASK,
    QSFPDD_P12_INTR_MASK,
    QSFPDD_P13_INTR_MASK,
    QSFPDD_P14_INTR_MASK,
    QSFPDD_P15_INTR_MASK,

    QSFPDD_P0_ABS_MASK,
    QSFPDD_P1_ABS_MASK,
    QSFPDD_P2_ABS_MASK,
    QSFPDD_P3_ABS_MASK,
    QSFPDD_P4_ABS_MASK,
    QSFPDD_P5_ABS_MASK,
    QSFPDD_P6_ABS_MASK,
    QSFPDD_P7_ABS_MASK,
    QSFPDD_P8_ABS_MASK,
    QSFPDD_P9_ABS_MASK,
    QSFPDD_P10_ABS_MASK,
    QSFPDD_P11_ABS_MASK,
    QSFPDD_P12_ABS_MASK,
    QSFPDD_P13_ABS_MASK,
    QSFPDD_P14_ABS_MASK,
    QSFPDD_P15_ABS_MASK,

    QSFPDD_P0_FUSE_INTR_MASK,
    QSFPDD_P1_FUSE_INTR_MASK,
    QSFPDD_P2_FUSE_INTR_MASK,
    QSFPDD_P3_FUSE_INTR_MASK,
    QSFPDD_P4_FUSE_INTR_MASK,
    QSFPDD_P5_FUSE_INTR_MASK,
    QSFPDD_P6_FUSE_INTR_MASK,
    QSFPDD_P7_FUSE_INTR_MASK,
    QSFPDD_P8_FUSE_INTR_MASK,
    QSFPDD_P9_FUSE_INTR_MASK,
    QSFPDD_P10_FUSE_INTR_MASK,
    QSFPDD_P11_FUSE_INTR_MASK,
    QSFPDD_P12_FUSE_INTR_MASK,
    QSFPDD_P13_FUSE_INTR_MASK,
    QSFPDD_P14_FUSE_INTR_MASK,
    QSFPDD_P15_FUSE_INTR_MASK,

    QSFPDD_P0_RST,
    QSFPDD_P1_RST,
    QSFPDD_P2_RST,
    QSFPDD_P3_RST,
    QSFPDD_P4_RST,
    QSFPDD_P5_RST,
    QSFPDD_P6_RST,
    QSFPDD_P7_RST,
    QSFPDD_P8_RST,
    QSFPDD_P9_RST,
    QSFPDD_P10_RST,
    QSFPDD_P11_RST,
    QSFPDD_P12_RST,
    QSFPDD_P13_RST,
    QSFPDD_P14_RST,
    QSFPDD_P15_RST,

    QSFPDD_P0_LP_MODE,
    QSFPDD_P1_LP_MODE,
    QSFPDD_P2_LP_MODE,
    QSFPDD_P3_LP_MODE,
    QSFPDD_P4_LP_MODE,
    QSFPDD_P5_LP_MODE,
    QSFPDD_P6_LP_MODE,
    QSFPDD_P7_LP_MODE,
    QSFPDD_P8_LP_MODE,
    QSFPDD_P9_LP_MODE,
    QSFPDD_P10_LP_MODE,
    QSFPDD_P11_LP_MODE,
    QSFPDD_P12_LP_MODE,
    QSFPDD_P13_LP_MODE,
    QSFPDD_P14_LP_MODE,
    QSFPDD_P15_LP_MODE,

    QSFPDD_P0_STUCK,
    QSFPDD_P1_STUCK,
    QSFPDD_P2_STUCK,
    QSFPDD_P3_STUCK,
    QSFPDD_P4_STUCK,
    QSFPDD_P5_STUCK,
    QSFPDD_P6_STUCK,
    QSFPDD_P7_STUCK,
    QSFPDD_P8_STUCK,
    QSFPDD_P9_STUCK,
    QSFPDD_P10_STUCK,
    QSFPDD_P11_STUCK,
    QSFPDD_P12_STUCK,
    QSFPDD_P13_STUCK,
    QSFPDD_P14_STUCK,
    QSFPDD_P15_STUCK,

    QSFPDD_P0_7_ABS_EVENT,
    QSFPDD_P8_15_ABS_EVENT,

    QSFPDD_P0_7_INTR_EVENT,
    QSFPDD_P8_15_INTR_EVENT,

    QSFPDD_P0_7_FUSE_INTR_EVENT,
    QSFPDD_P8_15_FUSE_INTR_EVENT,

    //CPLD 3

    QSFPDD_P16_INTR,
    QSFPDD_P17_INTR,
    QSFPDD_P18_INTR,
    QSFPDD_P19_INTR,
    QSFPDD_P20_INTR,
    QSFPDD_P21_INTR,
    QSFPDD_P22_INTR,
    QSFPDD_P23_INTR,
    QSFPDD_P24_INTR,
    QSFPDD_P25_INTR,
    QSFPDD_P26_INTR,
    QSFPDD_P27_INTR,
    QSFPDD_P28_INTR,
    QSFPDD_P29_INTR,
    QSFPDD_P30_INTR,
    QSFPDD_P31_INTR,

    QSFPDD_P16_ABS,
    QSFPDD_P17_ABS,
    QSFPDD_P18_ABS,
    QSFPDD_P19_ABS,
    QSFPDD_P20_ABS,
    QSFPDD_P21_ABS,
    QSFPDD_P22_ABS,
    QSFPDD_P23_ABS,
    QSFPDD_P24_ABS,
    QSFPDD_P25_ABS,
    QSFPDD_P26_ABS,
    QSFPDD_P27_ABS,
    QSFPDD_P28_ABS,
    QSFPDD_P29_ABS,
    QSFPDD_P30_ABS,
    QSFPDD_P31_ABS,

    QSFPDD_P16_23_ABS_EVENT,
    QSFPDD_P24_31_ABS_EVENT,

    QSFPDD_P16_23_INTR_EVENT,
    QSFPDD_P24_31_INTR_EVENT,

    QSFPDD_P16_23_FUSE_INTR_EVENT,
    QSFPDD_P24_31_FUSE_INTR_EVENT,

    QSFPDD_P16_FUSE_INTR,
    QSFPDD_P17_FUSE_INTR,
    QSFPDD_P18_FUSE_INTR,
    QSFPDD_P19_FUSE_INTR,
    QSFPDD_P20_FUSE_INTR,
    QSFPDD_P21_FUSE_INTR,
    QSFPDD_P22_FUSE_INTR,
    QSFPDD_P23_FUSE_INTR,
    QSFPDD_P24_FUSE_INTR,
    QSFPDD_P25_FUSE_INTR,
    QSFPDD_P26_FUSE_INTR,
    QSFPDD_P27_FUSE_INTR,
    QSFPDD_P28_FUSE_INTR,
    QSFPDD_P29_FUSE_INTR,
    QSFPDD_P30_FUSE_INTR,
    QSFPDD_P31_FUSE_INTR,

    QSFPDD_P16_INTR_MASK,
    QSFPDD_P17_INTR_MASK,
    QSFPDD_P18_INTR_MASK,
    QSFPDD_P19_INTR_MASK,
    QSFPDD_P20_INTR_MASK,
    QSFPDD_P21_INTR_MASK,
    QSFPDD_P22_INTR_MASK,
    QSFPDD_P23_INTR_MASK,
    QSFPDD_P24_INTR_MASK,
    QSFPDD_P25_INTR_MASK,
    QSFPDD_P26_INTR_MASK,
    QSFPDD_P27_INTR_MASK,
    QSFPDD_P28_INTR_MASK,
    QSFPDD_P29_INTR_MASK,
    QSFPDD_P30_INTR_MASK,
    QSFPDD_P31_INTR_MASK,

    QSFPDD_P16_ABS_MASK,
    QSFPDD_P17_ABS_MASK,
    QSFPDD_P18_ABS_MASK,
    QSFPDD_P19_ABS_MASK,
    QSFPDD_P20_ABS_MASK,
    QSFPDD_P21_ABS_MASK,
    QSFPDD_P22_ABS_MASK,
    QSFPDD_P23_ABS_MASK,
    QSFPDD_P24_ABS_MASK,
    QSFPDD_P25_ABS_MASK,
    QSFPDD_P26_ABS_MASK,
    QSFPDD_P27_ABS_MASK,
    QSFPDD_P28_ABS_MASK,
    QSFPDD_P29_ABS_MASK,
    QSFPDD_P30_ABS_MASK,
    QSFPDD_P31_ABS_MASK,

    QSFPDD_P16_FUSE_INTR_MASK,
    QSFPDD_P17_FUSE_INTR_MASK,
    QSFPDD_P18_FUSE_INTR_MASK,
    QSFPDD_P19_FUSE_INTR_MASK,
    QSFPDD_P20_FUSE_INTR_MASK,
    QSFPDD_P21_FUSE_INTR_MASK,
    QSFPDD_P22_FUSE_INTR_MASK,
    QSFPDD_P23_FUSE_INTR_MASK,
    QSFPDD_P24_FUSE_INTR_MASK,
    QSFPDD_P25_FUSE_INTR_MASK,
    QSFPDD_P26_FUSE_INTR_MASK,
    QSFPDD_P27_FUSE_INTR_MASK,
    QSFPDD_P28_FUSE_INTR_MASK,
    QSFPDD_P29_FUSE_INTR_MASK,
    QSFPDD_P30_FUSE_INTR_MASK,
    QSFPDD_P31_FUSE_INTR_MASK,

    QSFPDD_P16_RST,
    QSFPDD_P17_RST,
    QSFPDD_P18_RST,
    QSFPDD_P19_RST,
    QSFPDD_P20_RST,
    QSFPDD_P21_RST,
    QSFPDD_P22_RST,
    QSFPDD_P23_RST,
    QSFPDD_P24_RST,
    QSFPDD_P25_RST,
    QSFPDD_P26_RST,
    QSFPDD_P27_RST,
    QSFPDD_P28_RST,
    QSFPDD_P29_RST,
    QSFPDD_P30_RST,
    QSFPDD_P31_RST,

    QSFPDD_P16_LP_MODE,
    QSFPDD_P17_LP_MODE,
    QSFPDD_P18_LP_MODE,
    QSFPDD_P19_LP_MODE,
    QSFPDD_P20_LP_MODE,
    QSFPDD_P21_LP_MODE,
    QSFPDD_P22_LP_MODE,
    QSFPDD_P23_LP_MODE,
    QSFPDD_P24_LP_MODE,
    QSFPDD_P25_LP_MODE,
    QSFPDD_P26_LP_MODE,
    QSFPDD_P27_LP_MODE,
    QSFPDD_P28_LP_MODE,
    QSFPDD_P29_LP_MODE,
    QSFPDD_P30_LP_MODE,
    QSFPDD_P31_LP_MODE,

    QSFPDD_P16_STUCK,
    QSFPDD_P17_STUCK,
    QSFPDD_P18_STUCK,
    QSFPDD_P19_STUCK,
    QSFPDD_P20_STUCK,
    QSFPDD_P21_STUCK,
    QSFPDD_P22_STUCK,
    QSFPDD_P23_STUCK,
    QSFPDD_P24_STUCK,
    QSFPDD_P25_STUCK,
    QSFPDD_P26_STUCK,
    QSFPDD_P27_STUCK,
    QSFPDD_P28_STUCK,
    QSFPDD_P29_STUCK,
    QSFPDD_P30_STUCK,
    QSFPDD_P31_STUCK,

    //FPGA
    FPGA_MAJOR_VER,
    FPGA_MINOR_VER,
    FPGA_BUILD_VER,
    FPGA_VERSION_H,
    FPGA_ID,
    FPGA_MAC_ROV_1,
    FPGA_MAC_ROV_2,

    MGMT_P0_TS,
    MGMT_P1_TS,
    SFP_P32_TS,
    SFP_P33_TS,
    SFP_P34_TS,
    SFP_P35_TS,
    MGMT_P0_RS,
    MGMT_P1_RS,
    SFP_P32_RS,
    SFP_P33_RS,
    SFP_P34_RS,
    SFP_P35_RS,
    MGMT_P0_TX_DIS,
    MGMT_P1_TX_DIS,
    SFP_P32_TX_DIS,
    SFP_P33_TX_DIS,
    SFP_P34_TX_DIS,
    SFP_P35_TX_DIS,
    MGMT_P0_TX_FLT,
    MGMT_P1_TX_FLT,
    SFP_P32_TX_FLT,
    SFP_P33_TX_FLT,
    SFP_P34_TX_FLT,
    SFP_P35_TX_FLT,
    MGMT_P0_RX_LOS,
    MGMT_P1_RX_LOS,
    SFP_P32_RX_LOS,
    SFP_P33_RX_LOS,
    SFP_P34_RX_LOS,
    SFP_P35_RX_LOS,
    MGMT_P0_ABS,
    MGMT_P1_ABS,
    SFP_P32_ABS,
    SFP_P33_ABS,
    SFP_P34_ABS,
    SFP_P35_ABS,
    MGMT_P0_TX_FLT_MASK,
    MGMT_P1_TX_FLT_MASK,
    SFP_P32_TX_FLT_MASK,
    SFP_P33_TX_FLT_MASK,
    SFP_P34_TX_FLT_MASK,
    SFP_P35_TX_FLT_MASK,
    MGMT_P0_RX_LOS_MASK,
    MGMT_P1_RX_LOS_MASK,
    SFP_P32_RX_LOS_MASK,
    SFP_P33_RX_LOS_MASK,
    SFP_P34_RX_LOS_MASK,
    SFP_P35_RX_LOS_MASK,
    MGMT_P0_ABS_MASK,
    MGMT_P1_ABS_MASK,
    SFP_P32_ABS_MASK,
    SFP_P33_ABS_MASK,
    SFP_P34_ABS_MASK,
    SFP_P35_ABS_MASK,
    MGMT_P0_TX_FLT_EVENT,
    MGMT_P1_TX_FLT_EVENT,
    SFP_P32_TX_FLT_EVENT,
    SFP_P33_TX_FLT_EVENT,
    SFP_P34_TX_FLT_EVENT,
    SFP_P35_TX_FLT_EVENT,
    MGMT_SFP_TX_FLT_EVENT,
    MGMT_P0_RX_LOS_EVENT,
    MGMT_P1_RX_LOS_EVENT,
    SFP_P32_RX_LOS_EVENT,
    SFP_P33_RX_LOS_EVENT,
    SFP_P34_RX_LOS_EVENT,
    SFP_P35_RX_LOS_EVENT,
    MGMT_SFP_RX_LOS_EVENT,
    MGMT_P0_ABS_EVENT,
    MGMT_P1_ABS_EVENT,
    SFP_P32_ABS_EVENT,
    SFP_P33_ABS_EVENT,
    SFP_P34_ABS_EVENT,
    SFP_P35_ABS_EVENT,
    MGMT_SFP_ABS_EVENT,

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
    [CPLD_MAJOR_VER]                       =         {CPLD_VERSION_REG                        , MASK_1100_0000, DATA_DEC},
    [CPLD_MINOR_VER]                       =         {CPLD_VERSION_REG                        , MASK_0011_1111, DATA_DEC},
    [CPLD_ID]                              =         {CPLD_ID_REG                             , MASK_0000_0111, DATA_DEC},
    [CPLD_BUILD_VER]                       =         {CPLD_SUB_VERSION_REG                    , MASK_ALL      , DATA_DEC},
    [CPLD_VERSION_H]                       =         {NONE_REG                                , MASK_NONE     , DATA_UNK},

    [CPLD_I2C_STUCK]                       =         {CPLD_I2C_STUCK_REG                      , MASK_0000_0011, DATA_DEC},
    [CPLD_I2C_CONTROL]                     =         {CPLD_I2C_CONTROL_REG                    , MASK_1000_0000, DATA_DEC},
    [CPLD_I2C_IDLE]                        =         {CPLD_I2C_CONTROL_REG                    , MASK_0000_0011, DATA_DEC},
    [CPLD_I2C_RELAY]                       =         {CPLD_I2C_RELAY_REG                      , MASK_0000_0011, DATA_DEC},

    //CPLD 1
    [BRD_SKU_ID]                           =         {CPLD_SKU_ID_REG                         , MASK_0011_1111, DATA_DEC},
    [BRD_HW_ID]                            =         {CPLD_HW_BUILD_REV_REG                   , MASK_0000_0011, DATA_DEC},
    [BRD_DEPH_ID]                          =         {CPLD_HW_BUILD_REV_REG                   , MASK_0000_0100, DATA_DEC},
    [BRD_BUILD_ID]                         =         {CPLD_HW_BUILD_REV_REG                   , MASK_0011_1000, DATA_DEC},
    [BRD_ID_TYPE]                          =         {CPLD_HW_BUILD_REV_REG                   , MASK_1000_0000, DATA_DEC},
    [CPLD_CHIP_TYPE]                       =         {CPLD_CHIP_TYPE_REG                      , MASK_0000_0011, DATA_DEC},

    [MAC_INTR]                             =         {MAC_INTR_REG                            , MASK_ALL,       DATA_DEC},
    [CPLD_25GPHY_INTR]                     =         {CPLD_25GPHY_INTR_REG                    , MASK_ALL,       DATA_HEX},
    [CPLD_FRU_INTR]                        =         {CPLD_FRU_INTR_REG                       , MASK_ALL,       DATA_HEX},
    [NTM_INTR]                             =         {NTM_INTR_REG                            , MASK_ALL,       DATA_HEX},
    [THERMAL_ALERT_1_INTR]                 =         {THERMAL_ALERT_1_INTR_REG                , MASK_ALL,       DATA_HEX},
    [THERMAL_ALERT_2_INTR]                 =         {THERMAL_ALERT_2_INTR_REG                , MASK_ALL,       DATA_HEX},
    [THERMAL_ALERT_3_INTR]                 =         {THERMAL_ALERT_3_INTR_REG                , MASK_ALL,       DATA_HEX},
    [MISC_INTR]                            =         {MISC_INTR_REG                           , MASK_ALL,       DATA_HEX},
    [SYSTEM_INTR]                          =         {SYSTEM_INTR_REG                         , MASK_ALL,       DATA_HEX},

    [CPLD_2_INTR]                          =         {CPLD_FRU_INTR_REG                       , MASK_0000_0001, DATA_HEX},
    [CPLD_3_INTR]                          =         {CPLD_FRU_INTR_REG                       , MASK_0000_0010, DATA_HEX},
    [CPLD_4_INTR]                          =         {CPLD_FRU_INTR_REG                       , MASK_0000_0100, DATA_HEX},
    [PSU_0_INTR]                           =         {CPLD_FRU_INTR_REG                       , MASK_0010_0000, DATA_HEX},
    [PSU_1_INTR]                           =         {CPLD_FRU_INTR_REG                       , MASK_0100_0000, DATA_HEX},

    [MAC_HBM_TEMP_ALERT]                   =         {THERMAL_ALERT_1_INTR_REG                , MASK_0000_0001, DATA_HEX},
    [MB_LM75_TEMP_ALERT]                   =         {THERMAL_ALERT_2_INTR_REG                , MASK_0011_1111, DATA_HEX},
    [EXT_LM75_TEMP_ALERT]                  =         {THERMAL_ALERT_2_INTR_REG                , MASK_0100_0000, DATA_HEX},
    [PHY_LM75_CPLD5_INTR]                  =         {THERMAL_ALERT_2_INTR_REG                , MASK_1000_0000, DATA_HEX},
    [USB_OCP]                              =         {MISC_INTR_REG                           , MASK_0000_0010, DATA_HEX},
    [FPGA_INTR]                            =         {MISC_INTR_REG                           , MASK_0000_1000, DATA_HEX},

    [THERMAL_ALERT_INTR]                   =         {SYSTEM_INTR_REG                         , MASK_0000_0001, DATA_HEX},
    [PSU_CPLD_INTR]                        =         {SYSTEM_INTR_REG                         , MASK_0000_0010, DATA_HEX},
    [MAIN_CHIP_INTR]                       =         {SYSTEM_INTR_REG                         , MASK_0000_0100, DATA_HEX},
    [PTP_INTR]                             =         {SYSTEM_INTR_REG                         , MASK_0001_0000, DATA_HEX},

    [MAC_INTR_MASK]                        =         {MAC_INTR_MASK_REG                       , MASK_ALL,       DATA_HEX},
    [CPLD_25GPHY_INTR_MASK]                =         {CPLD_25GPHY_INTR_MASK_REG               , MASK_ALL,       DATA_HEX},
    [CPLD_FRU_INTR_MASK]                   =         {CPLD_FRU_INTR_MASK_REG                  , MASK_ALL,       DATA_HEX},
    [NTM_INTR_MASK]                        =         {NTM_INTR_MASK_REG                       , MASK_ALL,       DATA_HEX},
    [THERMAL_ALERT_1_INTR_MASK]            =         {THERMAL_ALERT_1_INTR_MASK_REG           , MASK_ALL,       DATA_HEX},
    [THERMAL_ALERT_2_INTR_MASK]            =         {THERMAL_ALERT_2_INTR_MASK_REG           , MASK_ALL,       DATA_HEX},
    [THERMAL_ALERT_3_INTR_MASK]            =         {THERMAL_ALERT_3_INTR_MASK_REG           , MASK_ALL,       DATA_HEX},
    [MISC_INTR_MASK]                       =         {MISC_INTR_MASK_REG                      , MASK_ALL,       DATA_HEX},
    [SYSTEM_INTR_MASK]                     =         {SYSTEM_INTR_MASK_REG                    , MASK_ALL,       DATA_HEX},

    [CPLD_2_INTR_MASK]                     =         {CPLD_FRU_INTR_MASK_REG                  , MASK_0000_0001, DATA_HEX},
    [CPLD_3_INTR_MASK]                     =         {CPLD_FRU_INTR_MASK_REG                  , MASK_0000_0010, DATA_HEX},
    [CPLD_4_INTR_MASK]                     =         {CPLD_FRU_INTR_MASK_REG                  , MASK_0000_0100, DATA_HEX},
    [PSU_0_INTR_MASK]                      =         {CPLD_FRU_INTR_MASK_REG                  , MASK_0100_0000, DATA_HEX},
    [PSU_1_INTR_MASK]                      =         {CPLD_FRU_INTR_MASK_REG                  , MASK_1000_0000, DATA_HEX},

    [MAC_TEMP_ALERT_MASK]                  =         {THERMAL_ALERT_1_INTR_MASK_REG           , MASK_0000_0001, DATA_HEX},
    [MB_TEMP_1_ALERT_MASK]                 =         {THERMAL_ALERT_2_INTR_MASK_REG           , MASK_0000_0001, DATA_HEX},
    [MB_TEMP_2_ALERT_MASK]                 =         {THERMAL_ALERT_2_INTR_MASK_REG           , MASK_0000_0010, DATA_HEX},
    [MB_TEMP_3_ALERT_MASK]                 =         {THERMAL_ALERT_2_INTR_MASK_REG           , MASK_0000_0100, DATA_HEX},
    [MB_TEMP_4_ALERT_MASK]                 =         {THERMAL_ALERT_2_INTR_MASK_REG           , MASK_0000_1000, DATA_HEX},
    [MB_TEMP_5_ALERT_MASK]                 =         {THERMAL_ALERT_2_INTR_MASK_REG           , MASK_0001_0000, DATA_HEX},
    [MB_TEMP_6_ALERT_MASK]                 =         {THERMAL_ALERT_2_INTR_MASK_REG           , MASK_0010_0000, DATA_HEX},
    [MB_TEMP_7_ALERT_MASK]                 =         {THERMAL_ALERT_2_INTR_MASK_REG           , MASK_0100_0000, DATA_HEX},
    [TOP_CPLD_5_INTR_MASK]                 =         {THERMAL_ALERT_2_INTR_MASK_REG           , MASK_1000_0000, DATA_HEX},
    [USB_OCP_MASK]                         =         {MISC_INTR_MASK_REG                      , MASK_0000_0010, DATA_HEX},
    [FPGA_INTR_MASK]                       =         {MISC_INTR_MASK_REG                      , MASK_0000_1000, DATA_HEX},
    [THERMAL_ALERT_INTR_MASK]              =         {SYSTEM_INTR_MASK_REG                    , MASK_0000_0001, DATA_HEX},
    [PSU_CPLD_INTR_MASK]                   =         {SYSTEM_INTR_MASK_REG                    , MASK_0000_0010, DATA_HEX},
    [MAIN_CHIP_INTR_MASK]                  =         {SYSTEM_INTR_MASK_REG                    , MASK_0000_0100, DATA_HEX},
    [PTP_INTR_MASK]                        =         {SYSTEM_INTR_MASK_REG                    , MASK_0001_0000, DATA_HEX},

    [MAC_INTR_EVENT]                       =         {MAC_INTR_EVENT_REG                      , MASK_ALL,       DATA_HEX},
    [CPLD_25GPHY_INTR_EVENT]               =         {CPLD_25GPHY_INTR_EVENT_REG              , MASK_ALL,       DATA_HEX},
    [CPLD_FRU_INTR_EVENT]                  =         {CPLD_FRU_INTR_EVENT_REG                 , MASK_ALL,       DATA_HEX},
    [NTM_INTR_EVENT]                       =         {NTM_INTR_EVENT_REG                      , MASK_ALL,       DATA_HEX},
    [THERMAL_ALERT_1_INTR_EVENT]           =         {THERMAL_ALERT_1_INTR_EVENT_REG          , MASK_ALL,       DATA_HEX},
    [THERMAL_ALERT_2_INTR_EVENT]           =         {THERMAL_ALERT_2_INTR_EVENT_REG          , MASK_ALL,       DATA_HEX},
    [THERMAL_ALERT_3_INTR_EVENT]           =         {THERMAL_ALERT_3_INTR_EVENT_REG          , MASK_ALL,       DATA_HEX},
    [MISC_INTR_EVENT]                      =         {MISC_INTR_EVENT_REG                     , MASK_ALL,       DATA_HEX},

    [MAC_TEMP_ALERT_EVENT]                 =         {THERMAL_ALERT_1_INTR_REG                , MASK_ALL,       DATA_HEX},

    [TEMP_ALERT_EVENT]                     =         {THERMAL_ALERT_2_INTR_REG                , MASK_ALL      , DATA_HEX},

    [EVENT_DETECT_CTRL]                    =         {EVENT_DETECT_CTRL_REG                   , MASK_0000_0001, DATA_HEX},

    [MAC_RST]                              =         {MAC_RST_REG                             , MASK_0000_0001, DATA_HEX},
    [BIOS_FLASH_RST]                       =         {BIOS_FLASH_RST_REG                      , MASK_0000_0001, DATA_HEX},
    [BMC_RST]                              =         {BMC_NTM_RST_REG                         , MASK_0000_0001, DATA_HEX},
    [BMC_PCIE_RST]                         =         {BMC_NTM_RST_REG                         , MASK_0000_0010, DATA_HEX},
    [BMC_LPC_RST]                          =         {BMC_NTM_RST_REG                         , MASK_0000_0100, DATA_HEX},
    [BMC_WDT_RST]                          =         {BMC_NTM_RST_REG                         , MASK_0000_1000, DATA_HEX},
    [NTM_RST]                              =         {BMC_NTM_RST_REG                         , MASK_0001_0000, DATA_HEX},

    [USB_REDRIVER_EXT_RST]                 =         {USB_RST_REG                             , MASK_0000_0010, DATA_HEX},
    [MAC_QSPI_FLASH_RST]                   =         {USB_RST_REG                             , MASK_0000_0100, DATA_HEX},
    [CP2130_RST]                           =         {USB_RST_REG                             , MASK_0000_1000, DATA_HEX},
    [USB_REDRIVER_NTM_RST]                 =         {USB_RST_REG                             , MASK_0001_0000, DATA_HEX},
    [USB_REDRIVER_MB_RST]                  =         {USB_RST_REG                             , MASK_0100_0000, DATA_HEX},
    [USB_CURR_LIMITER_EN]                  =         {USB_RST_REG                             , MASK_1000_0000, DATA_HEX},

    [CPLD_2_RST]                           =         {CPLD_RST_REG                            , MASK_0000_0001, DATA_HEX},
    [CPLD_3_RST]                           =         {CPLD_RST_REG                            , MASK_0000_0010, DATA_HEX},
    [FPGA_RST]                             =         {CPLD_RST_REG                            , MASK_0001_0000, DATA_HEX},

    [I2C_MUX_RST]                          =         {I2C_MUX_RST_REG                         , MASK_0000_1011, DATA_HEX},
    [I2C_MUX_9548_1_RST]                   =         {I2C_MUX_RST_REG                         , MASK_0000_0001, DATA_HEX},
    [I2C_MUX_9548_2_RST]                   =         {I2C_MUX_RST_REG                         , MASK_0000_0010, DATA_HEX},
    [I2C_MUX_FAN_RST]                      =         {I2C_MUX_RST_REG                         , MASK_0000_1000, DATA_HEX},

    [BCM82399_RST]                         =         {MISC_RST_REG                            , MASK_0000_0001, DATA_HEX},
    [CP2102_RST]                           =         {MISC_RST_REG                            , MASK_0000_0010, DATA_HEX},
    [RC32312_RST]                          =         {MISC_RST_REG                            , MASK_0000_0100, DATA_HEX},
    [I210_RST]                             =         {MISC_RST_REG                            , MASK_0000_1000, DATA_HEX},
    [IOEXP_RST]                            =         {MISC_RST_REG                            , MASK_0001_0000, DATA_HEX},
    [FPGA_PCIE_RST]                        =         {MISC_RST_REG                            , MASK_0010_0000, DATA_HEX},
    [I2C_MUX_OOB_RST]                      =         {MISC_RST_REG                            , MASK_0100_0000, DATA_HEX},
    [I2C_MUX_TS_RST]                       =         {MISC_RST_REG                            , MASK_1000_0000, DATA_HEX},

    [PSU_RST]                              =         {PSU_RST_REG                             , MASK_0000_0001, DATA_HEX},

    [PSU_0_PRSNT]                          =         {PSU_STATUS_REG                          , MASK_0000_0001, DATA_HEX},
    [PSU_1_PRSNT]                          =         {PSU_STATUS_REG                          , MASK_0000_0010, DATA_HEX},
    [PSU_0_ACIN]                           =         {PSU_STATUS_REG                          , MASK_0000_0100, DATA_HEX},
    [PSU_1_ACIN]                           =         {PSU_STATUS_REG                          , MASK_0000_1000, DATA_HEX},
    [PSU_0_PG]                             =         {PSU_STATUS_REG                          , MASK_0001_0000, DATA_HEX},
    [PSU_1_PG]                             =         {PSU_STATUS_REG                          , MASK_0010_0000, DATA_HEX},

    [PHY_BOOT]                             =         {PHY_BOOT_REG                            , MASK_0000_0001, DATA_HEX},
    [MB_EEPROM_WP]                         =         {MB_EEPROM_WP_REG                        , MASK_0000_0001, DATA_HEX},

    [CPU_MUX_SEL]                          =         {MUX_CTRL_REG                            , MASK_0000_0001, DATA_HEX},
    [PSU_MUX_SEL]                          =         {MUX_CTRL_REG                            , MASK_0000_0110, DATA_HEX},
    [FPGA_QSPI_SEL]                        =         {MUX_CTRL_REG                            , MASK_0010_0000, DATA_HEX},
    [BMC_UART_MUX_SEL]                     =         {MUX_CTRL_REG                            , MASK_0100_0000, DATA_HEX},
    [SYSTEM_UART_MUX_SEL]                  =         {MUX_CTRL_REG                            , MASK_1000_0000, DATA_HEX},

    [FPGA_CTRL]                            =         {FPGA_CTRL_REG                           , MASK_1000_0001, DATA_HEX},
    [FPGA_NCONFIG]                         =         {FPGA_CTRL_REG                           , MASK_0000_0001, DATA_HEX},
    [FPGA_CONFIG_DONE]                     =         {FPGA_CTRL_REG                           , MASK_1000_0000, DATA_HEX},

    [CPLD_SYSTEM_LED_SYS]                  =         {SYSTEM_LED_CTRL_1_REG                   , MASK_0000_1111, DATA_HEX},
    [SYSTEM_LED_STATUS]                    =         {SYSTEM_LED_CTRL_1_REG                   , MASK_0000_0001, DATA_DEC},
    [SYSTEM_LED_SPEED]                     =         {SYSTEM_LED_CTRL_1_REG                   , MASK_0000_0010, DATA_DEC},
    [SYSTEM_LED_BLINK]                     =         {SYSTEM_LED_CTRL_1_REG                   , MASK_0000_0100, DATA_DEC},
    [SYSTEM_LED_ONOFF]                     =         {SYSTEM_LED_CTRL_1_REG                   , MASK_0000_1000, DATA_DEC},
    [CPLD_SYSTEM_LED_FAN]                  =         {SYSTEM_LED_CTRL_1_REG                   , MASK_1111_0000, DATA_HEX},
    [FAN_LED_STATUS]                       =         {SYSTEM_LED_CTRL_1_REG                   , MASK_0001_0000, DATA_DEC},
    [FAN_LED_SPEED]                        =         {SYSTEM_LED_CTRL_1_REG                   , MASK_0010_0000, DATA_DEC},
    [FAN_LED_BLINK]                        =         {SYSTEM_LED_CTRL_1_REG                   , MASK_0100_0000, DATA_DEC},
    [FAN_LED_ONOFF]                        =         {SYSTEM_LED_CTRL_1_REG                   , MASK_1000_0000, DATA_DEC},

    [CPLD_SYSTEM_LED_PSU_0]                =         {SYSTEM_LED_CTRL_2_REG                   , MASK_0000_1111, DATA_HEX},
    [PSU_0_LED_STATUS]                     =         {SYSTEM_LED_CTRL_2_REG                   , MASK_0000_0001, DATA_DEC},
    [PSU_0_LED_SPEED]                      =         {SYSTEM_LED_CTRL_2_REG                   , MASK_0000_0010, DATA_DEC},
    [PSU_0_LED_BLINK]                      =         {SYSTEM_LED_CTRL_2_REG                   , MASK_0000_0100, DATA_DEC},
    [PSU_0_LED_ONOFF]                      =         {SYSTEM_LED_CTRL_2_REG                   , MASK_0000_1000, DATA_DEC},
    [CPLD_SYSTEM_LED_PSU_1]                =         {SYSTEM_LED_CTRL_2_REG                   , MASK_1111_0000, DATA_HEX},
    [PSU_1_LED_STATUS]                     =         {SYSTEM_LED_CTRL_2_REG                   , MASK_0001_0000, DATA_DEC},
    [PSU_1_LED_SPEED]                      =         {SYSTEM_LED_CTRL_2_REG                   , MASK_0010_0000, DATA_DEC},
    [PSU_1_LED_BLINK]                      =         {SYSTEM_LED_CTRL_2_REG                   , MASK_0100_0000, DATA_DEC},
    [PSU_1_LED_ONOFF]                      =         {SYSTEM_LED_CTRL_2_REG                   , MASK_1000_0000, DATA_DEC},

    [CPLD_SYSTEM_LED_SYNC]                 =         {SYSTEM_LED_CTRL_3_REG                   , MASK_0000_1111, DATA_HEX},
    [SYNC_LED_STATUS]                      =         {SYSTEM_LED_CTRL_3_REG                   , MASK_0000_0001, DATA_DEC},
    [SYNC_LED_SPEED]                       =         {SYSTEM_LED_CTRL_3_REG                   , MASK_0000_0010, DATA_DEC},
    [SYNC_LED_BLINK]                       =         {SYSTEM_LED_CTRL_3_REG                   , MASK_0000_0100, DATA_DEC},
    [SYNC_LED_ONOFF]                       =         {SYSTEM_LED_CTRL_3_REG                   , MASK_0000_1000, DATA_DEC},
    [CPLD_SYSTEM_LED_ID]                   =         {SYSTEM_LED_CTRL_3_REG                   , MASK_1111_0000, DATA_HEX},
    [ID_LED_STATUS]                        =         {SYSTEM_LED_CTRL_3_REG                   , MASK_0001_0000, DATA_DEC},
    [ID_LED_SPEED]                         =         {SYSTEM_LED_CTRL_3_REG                   , MASK_0010_0000, DATA_DEC},
    [ID_LED_BLINK]                         =         {SYSTEM_LED_CTRL_3_REG                   , MASK_0100_0000, DATA_DEC},
    [ID_LED_ONOFF]                         =         {SYSTEM_LED_CTRL_3_REG                   , MASK_1000_0000, DATA_DEC},

    [LED_CLEAR]                            =         {LED_CLEAR_REG                           , MASK_0000_0001, DATA_DEC},

    [MGMT_0_LED_STATUS]                    =         {MGMT_LED_CTRL_REG                       , MASK_0000_1111, DATA_HEX},
    [MGMT_0_LED_SPEED]                     =         {MGMT_LED_CTRL_REG                       , MASK_0000_0010, DATA_DEC},
    [MGMT_0_LED_BLINK]                     =         {MGMT_LED_CTRL_REG                       , MASK_0000_0100, DATA_DEC},
    [MGMT_0_LED_ONOFF]                     =         {MGMT_LED_CTRL_REG                       , MASK_0000_1000, DATA_DEC},
    [MGMT_1_LED_STATUS]                    =         {MGMT_LED_CTRL_REG                       , MASK_1111_0000, DATA_HEX},
    [MGMT_1_LED_SPEED]                     =         {MGMT_LED_CTRL_REG                       , MASK_0010_0000, DATA_DEC},
    [MGMT_1_LED_BLINK]                     =         {MGMT_LED_CTRL_REG                       , MASK_0100_0000, DATA_DEC},
    [MGMT_1_LED_ONOFF]                     =         {MGMT_LED_CTRL_REG                       , MASK_1000_0000, DATA_DEC},
    [MGMT_P0_LED]                           =         {MGMT_LED_CTRL_REG                       , MASK_0000_1111, DATA_HEX},
    [MGMT_P1_LED]                           =         {MGMT_LED_CTRL_REG                       , MASK_1111_0000, DATA_HEX},

    //QSFPDD
    [QSFPDD_P0_INTR]                   =         {QSFPDD_0_7_16_23_INTR_REG                 , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P1_INTR]                   =         {QSFPDD_0_7_16_23_INTR_REG                 , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P2_INTR]                   =         {QSFPDD_0_7_16_23_INTR_REG                 , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P3_INTR]                   =         {QSFPDD_0_7_16_23_INTR_REG                 , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P4_INTR]                   =         {QSFPDD_0_7_16_23_INTR_REG                 , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P5_INTR]                   =         {QSFPDD_0_7_16_23_INTR_REG                 , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P6_INTR]                   =         {QSFPDD_0_7_16_23_INTR_REG                 , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P7_INTR]                   =         {QSFPDD_0_7_16_23_INTR_REG                 , MASK_1000_0000, DATA_DEC},
    [QSFPDD_P8_INTR]                   =         {QSFPDD_8_15_24_31_INTR_REG                , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P9_INTR]                   =         {QSFPDD_8_15_24_31_INTR_REG                , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P10_INTR]                  =         {QSFPDD_8_15_24_31_INTR_REG                , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P11_INTR]                  =         {QSFPDD_8_15_24_31_INTR_REG                , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P12_INTR]                  =         {QSFPDD_8_15_24_31_INTR_REG                , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P13_INTR]                  =         {QSFPDD_8_15_24_31_INTR_REG                , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P14_INTR]                  =         {QSFPDD_8_15_24_31_INTR_REG                , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P15_INTR]                  =         {QSFPDD_8_15_24_31_INTR_REG                , MASK_1000_0000, DATA_DEC},
    [QSFPDD_P16_INTR]                  =         {QSFPDD_0_7_16_23_INTR_REG                 , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P17_INTR]                  =         {QSFPDD_0_7_16_23_INTR_REG                 , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P18_INTR]                  =         {QSFPDD_0_7_16_23_INTR_REG                 , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P19_INTR]                  =         {QSFPDD_0_7_16_23_INTR_REG                 , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P20_INTR]                  =         {QSFPDD_0_7_16_23_INTR_REG                 , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P21_INTR]                  =         {QSFPDD_0_7_16_23_INTR_REG                 , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P22_INTR]                  =         {QSFPDD_0_7_16_23_INTR_REG                 , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P23_INTR]                  =         {QSFPDD_0_7_16_23_INTR_REG                 , MASK_1000_0000, DATA_DEC},
    [QSFPDD_P24_INTR]                  =         {QSFPDD_8_15_24_31_INTR_REG                , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P25_INTR]                  =         {QSFPDD_8_15_24_31_INTR_REG                , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P26_INTR]                  =         {QSFPDD_8_15_24_31_INTR_REG                , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P27_INTR]                  =         {QSFPDD_8_15_24_31_INTR_REG                , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P28_INTR]                  =         {QSFPDD_8_15_24_31_INTR_REG                , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P29_INTR]                  =         {QSFPDD_8_15_24_31_INTR_REG                , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P30_INTR]                  =         {QSFPDD_8_15_24_31_INTR_REG                , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P31_INTR]                  =         {QSFPDD_8_15_24_31_INTR_REG                , MASK_1000_0000, DATA_DEC},

    [QSFPDD_P0_7_INTR_EVENT]           =         {QSFPDD_0_7_16_23_INTR_EVENT_REG           , MASK_ALL      , DATA_DEC},
    [QSFPDD_P8_15_INTR_EVENT]          =         {QSFPDD_8_15_24_31_INTR_EVENT_REG          , MASK_ALL      , DATA_DEC},
    [QSFPDD_P16_23_INTR_EVENT]         =         {QSFPDD_0_7_16_23_INTR_EVENT_REG           , MASK_ALL      , DATA_DEC},
    [QSFPDD_P24_31_INTR_EVENT]         =         {QSFPDD_8_15_24_31_INTR_EVENT_REG          , MASK_ALL      , DATA_DEC},

    [QSFPDD_P0_ABS]                    =         {QSFPDD_0_7_16_23_ABS_REG                  , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P1_ABS]                    =         {QSFPDD_0_7_16_23_ABS_REG                  , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P2_ABS]                    =         {QSFPDD_0_7_16_23_ABS_REG                  , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P3_ABS]                    =         {QSFPDD_0_7_16_23_ABS_REG                  , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P4_ABS]                    =         {QSFPDD_0_7_16_23_ABS_REG                  , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P5_ABS]                    =         {QSFPDD_0_7_16_23_ABS_REG                  , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P6_ABS]                    =         {QSFPDD_0_7_16_23_ABS_REG                  , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P7_ABS]                    =         {QSFPDD_0_7_16_23_ABS_REG                  , MASK_1000_0000, DATA_DEC},
    [QSFPDD_P8_ABS]                    =         {QSFPDD_8_15_24_31_ABS_REG                 , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P9_ABS]                    =         {QSFPDD_8_15_24_31_ABS_REG                 , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P10_ABS]                   =         {QSFPDD_8_15_24_31_ABS_REG                 , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P11_ABS]                   =         {QSFPDD_8_15_24_31_ABS_REG                 , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P12_ABS]                   =         {QSFPDD_8_15_24_31_ABS_REG                 , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P13_ABS]                   =         {QSFPDD_8_15_24_31_ABS_REG                 , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P14_ABS]                   =         {QSFPDD_8_15_24_31_ABS_REG                 , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P15_ABS]                   =         {QSFPDD_8_15_24_31_ABS_REG                 , MASK_1000_0000, DATA_DEC},
    [QSFPDD_P16_ABS]                   =         {QSFPDD_0_7_16_23_ABS_REG                  , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P17_ABS]                   =         {QSFPDD_0_7_16_23_ABS_REG                  , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P18_ABS]                   =         {QSFPDD_0_7_16_23_ABS_REG                  , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P19_ABS]                   =         {QSFPDD_0_7_16_23_ABS_REG                  , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P20_ABS]                   =         {QSFPDD_0_7_16_23_ABS_REG                  , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P21_ABS]                   =         {QSFPDD_0_7_16_23_ABS_REG                  , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P22_ABS]                   =         {QSFPDD_0_7_16_23_ABS_REG                  , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P23_ABS]                   =         {QSFPDD_0_7_16_23_ABS_REG                  , MASK_1000_0000, DATA_DEC},
    [QSFPDD_P24_ABS]                   =         {QSFPDD_8_15_24_31_ABS_REG                 , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P25_ABS]                   =         {QSFPDD_8_15_24_31_ABS_REG                 , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P26_ABS]                   =         {QSFPDD_8_15_24_31_ABS_REG                 , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P27_ABS]                   =         {QSFPDD_8_15_24_31_ABS_REG                 , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P28_ABS]                   =         {QSFPDD_8_15_24_31_ABS_REG                 , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P29_ABS]                   =         {QSFPDD_8_15_24_31_ABS_REG                 , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P30_ABS]                   =         {QSFPDD_8_15_24_31_ABS_REG                 , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P31_ABS]                   =         {QSFPDD_8_15_24_31_ABS_REG                 , MASK_1000_0000, DATA_DEC},

    [QSFPDD_P0_7_ABS_EVENT]            =         {QSFPDD_0_7_16_23_ABS_EVENT_REG            , MASK_ALL      , DATA_DEC},
    [QSFPDD_P8_15_ABS_EVENT]           =         {QSFPDD_8_15_24_31_ABS_EVENT_REG           , MASK_ALL      , DATA_DEC},
    [QSFPDD_P16_23_ABS_EVENT]          =         {QSFPDD_0_7_16_23_ABS_EVENT_REG            , MASK_ALL      , DATA_DEC},
    [QSFPDD_P24_31_ABS_EVENT]          =         {QSFPDD_8_15_24_31_ABS_EVENT_REG           , MASK_ALL      , DATA_DEC},

    [QSFPDD_P0_FUSE_INTR]              =         {QSFPDD_0_7_16_23_FUSE_INTR_REG            , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P1_FUSE_INTR]              =         {QSFPDD_0_7_16_23_FUSE_INTR_REG            , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P2_FUSE_INTR]              =         {QSFPDD_0_7_16_23_FUSE_INTR_REG            , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P3_FUSE_INTR]              =         {QSFPDD_0_7_16_23_FUSE_INTR_REG            , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P4_FUSE_INTR]              =         {QSFPDD_0_7_16_23_FUSE_INTR_REG            , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P5_FUSE_INTR]              =         {QSFPDD_0_7_16_23_FUSE_INTR_REG            , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P6_FUSE_INTR]              =         {QSFPDD_0_7_16_23_FUSE_INTR_REG            , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P7_FUSE_INTR]              =         {QSFPDD_0_7_16_23_FUSE_INTR_REG            , MASK_1000_0000, DATA_DEC},
    [QSFPDD_P8_FUSE_INTR]              =         {QSFPDD_8_15_24_31_FUSE_INTR_REG           , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P9_FUSE_INTR]              =         {QSFPDD_8_15_24_31_FUSE_INTR_REG           , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P10_FUSE_INTR]             =         {QSFPDD_8_15_24_31_FUSE_INTR_REG           , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P11_FUSE_INTR]             =         {QSFPDD_8_15_24_31_FUSE_INTR_REG           , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P12_FUSE_INTR]             =         {QSFPDD_8_15_24_31_FUSE_INTR_REG           , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P13_FUSE_INTR]             =         {QSFPDD_8_15_24_31_FUSE_INTR_REG           , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P14_FUSE_INTR]             =         {QSFPDD_8_15_24_31_FUSE_INTR_REG           , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P15_FUSE_INTR]             =         {QSFPDD_8_15_24_31_FUSE_INTR_REG           , MASK_1000_0000, DATA_DEC},
    [QSFPDD_P16_FUSE_INTR]             =         {QSFPDD_0_7_16_23_FUSE_INTR_REG            , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P17_FUSE_INTR]             =         {QSFPDD_0_7_16_23_FUSE_INTR_REG            , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P18_FUSE_INTR]             =         {QSFPDD_0_7_16_23_FUSE_INTR_REG            , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P19_FUSE_INTR]             =         {QSFPDD_0_7_16_23_FUSE_INTR_REG            , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P20_FUSE_INTR]             =         {QSFPDD_0_7_16_23_FUSE_INTR_REG            , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P21_FUSE_INTR]             =         {QSFPDD_0_7_16_23_FUSE_INTR_REG            , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P22_FUSE_INTR]             =         {QSFPDD_0_7_16_23_FUSE_INTR_REG            , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P23_FUSE_INTR]             =         {QSFPDD_0_7_16_23_FUSE_INTR_REG            , MASK_1000_0000, DATA_DEC},
    [QSFPDD_P24_FUSE_INTR]             =         {QSFPDD_8_15_24_31_FUSE_INTR_REG           , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P25_FUSE_INTR]             =         {QSFPDD_8_15_24_31_FUSE_INTR_REG           , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P26_FUSE_INTR]             =         {QSFPDD_8_15_24_31_FUSE_INTR_REG           , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P27_FUSE_INTR]             =         {QSFPDD_8_15_24_31_FUSE_INTR_REG           , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P28_FUSE_INTR]             =         {QSFPDD_8_15_24_31_FUSE_INTR_REG           , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P29_FUSE_INTR]             =         {QSFPDD_8_15_24_31_FUSE_INTR_REG           , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P30_FUSE_INTR]             =         {QSFPDD_8_15_24_31_FUSE_INTR_REG           , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P31_FUSE_INTR]             =         {QSFPDD_8_15_24_31_FUSE_INTR_REG           , MASK_1000_0000, DATA_DEC},

    [QSFPDD_P0_7_FUSE_INTR_EVENT]      =         {QSFPDD_0_7_16_23_FUSE_INTR_EVENT_REG      , MASK_ALL      , DATA_DEC},
    [QSFPDD_P8_15_FUSE_INTR_EVENT]     =         {QSFPDD_8_15_24_31_FUSE_INTR_EVENT_REG     , MASK_ALL      , DATA_DEC},
    [QSFPDD_P16_23_FUSE_INTR_EVENT]    =         {QSFPDD_0_7_16_23_FUSE_INTR_EVENT_REG      , MASK_ALL      , DATA_DEC},
    [QSFPDD_P24_31_FUSE_INTR_EVENT]    =         {QSFPDD_8_15_24_31_FUSE_INTR_EVENT_REG     , MASK_ALL      , DATA_DEC},

    [QSFPDD_P0_INTR_MASK]              =         {QSFPDD_0_7_16_23_INTR_MASK_REG            , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P1_INTR_MASK]              =         {QSFPDD_0_7_16_23_INTR_MASK_REG            , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P2_INTR_MASK]              =         {QSFPDD_0_7_16_23_INTR_MASK_REG            , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P3_INTR_MASK]              =         {QSFPDD_0_7_16_23_INTR_MASK_REG            , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P4_INTR_MASK]              =         {QSFPDD_0_7_16_23_INTR_MASK_REG            , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P5_INTR_MASK]              =         {QSFPDD_0_7_16_23_INTR_MASK_REG            , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P6_INTR_MASK]              =         {QSFPDD_0_7_16_23_INTR_MASK_REG            , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P7_INTR_MASK]              =         {QSFPDD_0_7_16_23_INTR_MASK_REG            , MASK_1000_0000, DATA_DEC},
    [QSFPDD_P8_INTR_MASK]              =         {QSFPDD_8_15_24_31_INTR_MASK_REG           , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P9_INTR_MASK]              =         {QSFPDD_8_15_24_31_INTR_MASK_REG           , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P10_INTR_MASK]             =         {QSFPDD_8_15_24_31_INTR_MASK_REG           , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P11_INTR_MASK]             =         {QSFPDD_8_15_24_31_INTR_MASK_REG           , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P12_INTR_MASK]             =         {QSFPDD_8_15_24_31_INTR_MASK_REG           , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P13_INTR_MASK]             =         {QSFPDD_8_15_24_31_INTR_MASK_REG           , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P14_INTR_MASK]             =         {QSFPDD_8_15_24_31_INTR_MASK_REG           , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P15_INTR_MASK]             =         {QSFPDD_8_15_24_31_INTR_MASK_REG           , MASK_1000_0000, DATA_DEC},
    [QSFPDD_P16_INTR_MASK]             =         {QSFPDD_0_7_16_23_INTR_MASK_REG            , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P17_INTR_MASK]             =         {QSFPDD_0_7_16_23_INTR_MASK_REG            , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P18_INTR_MASK]             =         {QSFPDD_0_7_16_23_INTR_MASK_REG            , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P19_INTR_MASK]             =         {QSFPDD_0_7_16_23_INTR_MASK_REG            , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P20_INTR_MASK]             =         {QSFPDD_0_7_16_23_INTR_MASK_REG            , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P21_INTR_MASK]             =         {QSFPDD_0_7_16_23_INTR_MASK_REG            , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P22_INTR_MASK]             =         {QSFPDD_0_7_16_23_INTR_MASK_REG            , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P23_INTR_MASK]             =         {QSFPDD_0_7_16_23_INTR_MASK_REG            , MASK_1000_0000, DATA_DEC},
    [QSFPDD_P24_INTR_MASK]             =         {QSFPDD_8_15_24_31_INTR_MASK_REG           , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P25_INTR_MASK]             =         {QSFPDD_8_15_24_31_INTR_MASK_REG           , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P26_INTR_MASK]             =         {QSFPDD_8_15_24_31_INTR_MASK_REG           , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P27_INTR_MASK]             =         {QSFPDD_8_15_24_31_INTR_MASK_REG           , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P28_INTR_MASK]             =         {QSFPDD_8_15_24_31_INTR_MASK_REG           , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P29_INTR_MASK]             =         {QSFPDD_8_15_24_31_INTR_MASK_REG           , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P30_INTR_MASK]             =         {QSFPDD_8_15_24_31_INTR_MASK_REG           , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P31_INTR_MASK]             =         {QSFPDD_8_15_24_31_INTR_MASK_REG           , MASK_1000_0000, DATA_DEC},

    [QSFPDD_P0_ABS_MASK]               =         {QSFPDD_0_7_16_23_ABS_MASK_REG             , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P1_ABS_MASK]               =         {QSFPDD_0_7_16_23_ABS_MASK_REG             , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P2_ABS_MASK]               =         {QSFPDD_0_7_16_23_ABS_MASK_REG             , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P3_ABS_MASK]               =         {QSFPDD_0_7_16_23_ABS_MASK_REG             , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P4_ABS_MASK]               =         {QSFPDD_0_7_16_23_ABS_MASK_REG             , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P5_ABS_MASK]               =         {QSFPDD_0_7_16_23_ABS_MASK_REG             , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P6_ABS_MASK]               =         {QSFPDD_0_7_16_23_ABS_MASK_REG             , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P7_ABS_MASK]               =         {QSFPDD_0_7_16_23_ABS_MASK_REG             , MASK_1000_0000, DATA_DEC},
    [QSFPDD_P8_ABS_MASK]               =         {QSFPDD_8_15_24_31_ABS_MASK_REG            , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P9_ABS_MASK]               =         {QSFPDD_8_15_24_31_ABS_MASK_REG            , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P10_ABS_MASK]              =         {QSFPDD_8_15_24_31_ABS_MASK_REG            , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P11_ABS_MASK]              =         {QSFPDD_8_15_24_31_ABS_MASK_REG            , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P12_ABS_MASK]              =         {QSFPDD_8_15_24_31_ABS_MASK_REG            , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P13_ABS_MASK]              =         {QSFPDD_8_15_24_31_ABS_MASK_REG            , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P14_ABS_MASK]              =         {QSFPDD_8_15_24_31_ABS_MASK_REG            , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P15_ABS_MASK]              =         {QSFPDD_8_15_24_31_ABS_MASK_REG            , MASK_1000_0000, DATA_DEC},
    [QSFPDD_P16_ABS_MASK]              =         {QSFPDD_0_7_16_23_ABS_MASK_REG             , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P17_ABS_MASK]              =         {QSFPDD_0_7_16_23_ABS_MASK_REG             , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P18_ABS_MASK]              =         {QSFPDD_0_7_16_23_ABS_MASK_REG             , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P19_ABS_MASK]              =         {QSFPDD_0_7_16_23_ABS_MASK_REG             , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P20_ABS_MASK]              =         {QSFPDD_0_7_16_23_ABS_MASK_REG             , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P21_ABS_MASK]              =         {QSFPDD_0_7_16_23_ABS_MASK_REG             , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P22_ABS_MASK]              =         {QSFPDD_0_7_16_23_ABS_MASK_REG             , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P23_ABS_MASK]              =         {QSFPDD_0_7_16_23_ABS_MASK_REG             , MASK_1000_0000, DATA_DEC},
    [QSFPDD_P24_ABS_MASK]              =         {QSFPDD_8_15_24_31_ABS_MASK_REG            , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P25_ABS_MASK]              =         {QSFPDD_8_15_24_31_ABS_MASK_REG            , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P26_ABS_MASK]              =         {QSFPDD_8_15_24_31_ABS_MASK_REG            , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P27_ABS_MASK]              =         {QSFPDD_8_15_24_31_ABS_MASK_REG            , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P28_ABS_MASK]              =         {QSFPDD_8_15_24_31_ABS_MASK_REG            , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P29_ABS_MASK]              =         {QSFPDD_8_15_24_31_ABS_MASK_REG            , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P30_ABS_MASK]              =         {QSFPDD_8_15_24_31_ABS_MASK_REG            , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P31_ABS_MASK]              =         {QSFPDD_8_15_24_31_ABS_MASK_REG            , MASK_1000_0000, DATA_DEC},

    [QSFPDD_P0_FUSE_INTR_MASK]         =         {QSFPDD_0_7_16_23_FUSE_INTR_MASK_REG       , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P1_FUSE_INTR_MASK]         =         {QSFPDD_0_7_16_23_FUSE_INTR_MASK_REG       , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P2_FUSE_INTR_MASK]         =         {QSFPDD_0_7_16_23_FUSE_INTR_MASK_REG       , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P3_FUSE_INTR_MASK]         =         {QSFPDD_0_7_16_23_FUSE_INTR_MASK_REG       , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P4_FUSE_INTR_MASK]         =         {QSFPDD_0_7_16_23_FUSE_INTR_MASK_REG       , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P5_FUSE_INTR_MASK]         =         {QSFPDD_0_7_16_23_FUSE_INTR_MASK_REG       , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P6_FUSE_INTR_MASK]         =         {QSFPDD_0_7_16_23_FUSE_INTR_MASK_REG       , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P7_FUSE_INTR_MASK]         =         {QSFPDD_0_7_16_23_FUSE_INTR_MASK_REG       , MASK_1000_0000, DATA_DEC},
    [QSFPDD_P8_FUSE_INTR_MASK]         =         {QSFPDD_8_15_24_31_FUSE_INTR_MASK_REG      , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P9_FUSE_INTR_MASK]         =         {QSFPDD_8_15_24_31_FUSE_INTR_MASK_REG      , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P10_FUSE_INTR_MASK]        =         {QSFPDD_8_15_24_31_FUSE_INTR_MASK_REG      , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P11_FUSE_INTR_MASK]        =         {QSFPDD_8_15_24_31_FUSE_INTR_MASK_REG      , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P12_FUSE_INTR_MASK]        =         {QSFPDD_8_15_24_31_FUSE_INTR_MASK_REG      , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P13_FUSE_INTR_MASK]        =         {QSFPDD_8_15_24_31_FUSE_INTR_MASK_REG      , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P14_FUSE_INTR_MASK]        =         {QSFPDD_8_15_24_31_FUSE_INTR_MASK_REG      , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P15_FUSE_INTR_MASK]        =         {QSFPDD_8_15_24_31_FUSE_INTR_MASK_REG      , MASK_1000_0000, DATA_DEC},
    [QSFPDD_P16_FUSE_INTR_MASK]        =         {QSFPDD_0_7_16_23_FUSE_INTR_MASK_REG       , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P17_FUSE_INTR_MASK]        =         {QSFPDD_0_7_16_23_FUSE_INTR_MASK_REG       , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P18_FUSE_INTR_MASK]        =         {QSFPDD_0_7_16_23_FUSE_INTR_MASK_REG       , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P19_FUSE_INTR_MASK]        =         {QSFPDD_0_7_16_23_FUSE_INTR_MASK_REG       , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P20_FUSE_INTR_MASK]        =         {QSFPDD_0_7_16_23_FUSE_INTR_MASK_REG       , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P21_FUSE_INTR_MASK]        =         {QSFPDD_0_7_16_23_FUSE_INTR_MASK_REG       , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P22_FUSE_INTR_MASK]        =         {QSFPDD_0_7_16_23_FUSE_INTR_MASK_REG       , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P23_FUSE_INTR_MASK]        =         {QSFPDD_0_7_16_23_FUSE_INTR_MASK_REG       , MASK_1000_0000, DATA_DEC},
    [QSFPDD_P24_FUSE_INTR_MASK]        =         {QSFPDD_8_15_24_31_FUSE_INTR_MASK_REG      , MASK_0000_0001, DATA_DEC},
    [QSFPDD_P25_FUSE_INTR_MASK]        =         {QSFPDD_8_15_24_31_FUSE_INTR_MASK_REG      , MASK_0000_0010, DATA_DEC},
    [QSFPDD_P26_FUSE_INTR_MASK]        =         {QSFPDD_8_15_24_31_FUSE_INTR_MASK_REG      , MASK_0000_0100, DATA_DEC},
    [QSFPDD_P27_FUSE_INTR_MASK]        =         {QSFPDD_8_15_24_31_FUSE_INTR_MASK_REG      , MASK_0000_1000, DATA_DEC},
    [QSFPDD_P28_FUSE_INTR_MASK]        =         {QSFPDD_8_15_24_31_FUSE_INTR_MASK_REG      , MASK_0001_0000, DATA_DEC},
    [QSFPDD_P29_FUSE_INTR_MASK]        =         {QSFPDD_8_15_24_31_FUSE_INTR_MASK_REG      , MASK_0010_0000, DATA_DEC},
    [QSFPDD_P30_FUSE_INTR_MASK]        =         {QSFPDD_8_15_24_31_FUSE_INTR_MASK_REG      , MASK_0100_0000, DATA_DEC},
    [QSFPDD_P31_FUSE_INTR_MASK]        =         {QSFPDD_8_15_24_31_FUSE_INTR_MASK_REG      , MASK_1000_0000, DATA_DEC},

    [QSFPDD_P0_RST]                    =         {QSFPDD_0_7_16_23_RST_REG                  , MASK_0000_0001, DATA_HEX},
    [QSFPDD_P1_RST]                    =         {QSFPDD_0_7_16_23_RST_REG                  , MASK_0000_0010, DATA_HEX},
    [QSFPDD_P2_RST]                    =         {QSFPDD_0_7_16_23_RST_REG                  , MASK_0000_0100, DATA_HEX},
    [QSFPDD_P3_RST]                    =         {QSFPDD_0_7_16_23_RST_REG                  , MASK_0000_1000, DATA_HEX},
    [QSFPDD_P4_RST]                    =         {QSFPDD_0_7_16_23_RST_REG                  , MASK_0001_0000, DATA_HEX},
    [QSFPDD_P5_RST]                    =         {QSFPDD_0_7_16_23_RST_REG                  , MASK_0010_0000, DATA_HEX},
    [QSFPDD_P6_RST]                    =         {QSFPDD_0_7_16_23_RST_REG                  , MASK_0100_0000, DATA_HEX},
    [QSFPDD_P7_RST]                    =         {QSFPDD_0_7_16_23_RST_REG                  , MASK_1000_0000, DATA_HEX},
    [QSFPDD_P8_RST]                    =         {QSFPDD_8_15_24_31_RST_REG                 , MASK_0000_0001, DATA_HEX},
    [QSFPDD_P9_RST]                    =         {QSFPDD_8_15_24_31_RST_REG                 , MASK_0000_0010, DATA_HEX},
    [QSFPDD_P10_RST]                   =         {QSFPDD_8_15_24_31_RST_REG                 , MASK_0000_0100, DATA_HEX},
    [QSFPDD_P11_RST]                   =         {QSFPDD_8_15_24_31_RST_REG                 , MASK_0000_1000, DATA_HEX},
    [QSFPDD_P12_RST]                   =         {QSFPDD_8_15_24_31_RST_REG                 , MASK_0001_0000, DATA_HEX},
    [QSFPDD_P13_RST]                   =         {QSFPDD_8_15_24_31_RST_REG                 , MASK_0010_0000, DATA_HEX},
    [QSFPDD_P14_RST]                   =         {QSFPDD_8_15_24_31_RST_REG                 , MASK_0100_0000, DATA_HEX},
    [QSFPDD_P15_RST]                   =         {QSFPDD_8_15_24_31_RST_REG                 , MASK_1000_0000, DATA_HEX},
    [QSFPDD_P16_RST]                   =         {QSFPDD_0_7_16_23_RST_REG                  , MASK_0000_0001, DATA_HEX},
    [QSFPDD_P17_RST]                   =         {QSFPDD_0_7_16_23_RST_REG                  , MASK_0000_0010, DATA_HEX},
    [QSFPDD_P18_RST]                   =         {QSFPDD_0_7_16_23_RST_REG                  , MASK_0000_0100, DATA_HEX},
    [QSFPDD_P19_RST]                   =         {QSFPDD_0_7_16_23_RST_REG                  , MASK_0000_1000, DATA_HEX},
    [QSFPDD_P20_RST]                   =         {QSFPDD_0_7_16_23_RST_REG                  , MASK_0001_0000, DATA_HEX},
    [QSFPDD_P21_RST]                   =         {QSFPDD_0_7_16_23_RST_REG                  , MASK_0010_0000, DATA_HEX},
    [QSFPDD_P22_RST]                   =         {QSFPDD_0_7_16_23_RST_REG                  , MASK_0100_0000, DATA_HEX},
    [QSFPDD_P23_RST]                   =         {QSFPDD_0_7_16_23_RST_REG                  , MASK_1000_0000, DATA_HEX},
    [QSFPDD_P24_RST]                   =         {QSFPDD_8_15_24_31_RST_REG                 , MASK_0000_0001, DATA_HEX},
    [QSFPDD_P25_RST]                   =         {QSFPDD_8_15_24_31_RST_REG                 , MASK_0000_0010, DATA_HEX},
    [QSFPDD_P26_RST]                   =         {QSFPDD_8_15_24_31_RST_REG                 , MASK_0000_0100, DATA_HEX},
    [QSFPDD_P27_RST]                   =         {QSFPDD_8_15_24_31_RST_REG                 , MASK_0000_1000, DATA_HEX},
    [QSFPDD_P28_RST]                   =         {QSFPDD_8_15_24_31_RST_REG                 , MASK_0001_0000, DATA_HEX},
    [QSFPDD_P29_RST]                   =         {QSFPDD_8_15_24_31_RST_REG                 , MASK_0010_0000, DATA_HEX},
    [QSFPDD_P30_RST]                   =         {QSFPDD_8_15_24_31_RST_REG                 , MASK_0100_0000, DATA_HEX},
    [QSFPDD_P31_RST]                   =         {QSFPDD_8_15_24_31_RST_REG                 , MASK_1000_0000, DATA_HEX},

    [QSFPDD_P0_LP_MODE]                =         {QSFPDD_0_7_16_23_LP_REG                   , MASK_0000_0001, DATA_HEX},
    [QSFPDD_P1_LP_MODE]                =         {QSFPDD_0_7_16_23_LP_REG                   , MASK_0000_0010, DATA_HEX},
    [QSFPDD_P2_LP_MODE]                =         {QSFPDD_0_7_16_23_LP_REG                   , MASK_0000_0100, DATA_HEX},
    [QSFPDD_P3_LP_MODE]                =         {QSFPDD_0_7_16_23_LP_REG                   , MASK_0000_1000, DATA_HEX},
    [QSFPDD_P4_LP_MODE]                =         {QSFPDD_0_7_16_23_LP_REG                   , MASK_0001_0000, DATA_HEX},
    [QSFPDD_P5_LP_MODE]                =         {QSFPDD_0_7_16_23_LP_REG                   , MASK_0010_0000, DATA_HEX},
    [QSFPDD_P6_LP_MODE]                =         {QSFPDD_0_7_16_23_LP_REG                   , MASK_0100_0000, DATA_HEX},
    [QSFPDD_P7_LP_MODE]                =         {QSFPDD_0_7_16_23_LP_REG                   , MASK_1000_0000, DATA_HEX},
    [QSFPDD_P8_LP_MODE]                =         {QSFPDD_8_15_24_31_LP_REG                  , MASK_0000_0001, DATA_HEX},
    [QSFPDD_P9_LP_MODE]                =         {QSFPDD_8_15_24_31_LP_REG                  , MASK_0000_0010, DATA_HEX},
    [QSFPDD_P10_LP_MODE]               =         {QSFPDD_8_15_24_31_LP_REG                  , MASK_0000_0100, DATA_HEX},
    [QSFPDD_P11_LP_MODE]               =         {QSFPDD_8_15_24_31_LP_REG                  , MASK_0000_1000, DATA_HEX},
    [QSFPDD_P12_LP_MODE]               =         {QSFPDD_8_15_24_31_LP_REG                  , MASK_0001_0000, DATA_HEX},
    [QSFPDD_P13_LP_MODE]               =         {QSFPDD_8_15_24_31_LP_REG                  , MASK_0010_0000, DATA_HEX},
    [QSFPDD_P14_LP_MODE]               =         {QSFPDD_8_15_24_31_LP_REG                  , MASK_0100_0000, DATA_HEX},
    [QSFPDD_P15_LP_MODE]               =         {QSFPDD_8_15_24_31_LP_REG                  , MASK_1000_0000, DATA_HEX},
    [QSFPDD_P16_LP_MODE]               =         {QSFPDD_0_7_16_23_LP_REG                   , MASK_0000_0001, DATA_HEX},
    [QSFPDD_P17_LP_MODE]               =         {QSFPDD_0_7_16_23_LP_REG                   , MASK_0000_0010, DATA_HEX},
    [QSFPDD_P18_LP_MODE]               =         {QSFPDD_0_7_16_23_LP_REG                   , MASK_0000_0100, DATA_HEX},
    [QSFPDD_P19_LP_MODE]               =         {QSFPDD_0_7_16_23_LP_REG                   , MASK_0000_1000, DATA_HEX},
    [QSFPDD_P20_LP_MODE]               =         {QSFPDD_0_7_16_23_LP_REG                   , MASK_0001_0000, DATA_HEX},
    [QSFPDD_P21_LP_MODE]               =         {QSFPDD_0_7_16_23_LP_REG                   , MASK_0010_0000, DATA_HEX},
    [QSFPDD_P22_LP_MODE]               =         {QSFPDD_0_7_16_23_LP_REG                   , MASK_0100_0000, DATA_HEX},
    [QSFPDD_P23_LP_MODE]               =         {QSFPDD_0_7_16_23_LP_REG                   , MASK_1000_0000, DATA_HEX},
    [QSFPDD_P24_LP_MODE]               =         {QSFPDD_8_15_24_31_LP_REG                  , MASK_0000_0001, DATA_HEX},
    [QSFPDD_P25_LP_MODE]               =         {QSFPDD_8_15_24_31_LP_REG                  , MASK_0000_0010, DATA_HEX},
    [QSFPDD_P26_LP_MODE]               =         {QSFPDD_8_15_24_31_LP_REG                  , MASK_0000_0100, DATA_HEX},
    [QSFPDD_P27_LP_MODE]               =         {QSFPDD_8_15_24_31_LP_REG                  , MASK_0000_1000, DATA_HEX},
    [QSFPDD_P28_LP_MODE]               =         {QSFPDD_8_15_24_31_LP_REG                  , MASK_0001_0000, DATA_HEX},
    [QSFPDD_P29_LP_MODE]               =         {QSFPDD_8_15_24_31_LP_REG                  , MASK_0010_0000, DATA_HEX},
    [QSFPDD_P30_LP_MODE]               =         {QSFPDD_8_15_24_31_LP_REG                  , MASK_0100_0000, DATA_HEX},
    [QSFPDD_P31_LP_MODE]               =         {QSFPDD_8_15_24_31_LP_REG                  , MASK_1000_0000, DATA_HEX},

    [QSFPDD_P0_STUCK]                  =         {QSFPDD_0_7_16_23_STUCK_REG                , MASK_0000_0001, DATA_HEX},
    [QSFPDD_P1_STUCK]                  =         {QSFPDD_0_7_16_23_STUCK_REG                , MASK_0000_0010, DATA_HEX},
    [QSFPDD_P2_STUCK]                  =         {QSFPDD_0_7_16_23_STUCK_REG                , MASK_0000_0100, DATA_HEX},
    [QSFPDD_P3_STUCK]                  =         {QSFPDD_0_7_16_23_STUCK_REG                , MASK_0000_1000, DATA_HEX},
    [QSFPDD_P4_STUCK]                  =         {QSFPDD_0_7_16_23_STUCK_REG                , MASK_0001_0000, DATA_HEX},
    [QSFPDD_P5_STUCK]                  =         {QSFPDD_0_7_16_23_STUCK_REG                , MASK_0010_0000, DATA_HEX},
    [QSFPDD_P6_STUCK]                  =         {QSFPDD_0_7_16_23_STUCK_REG                , MASK_0100_0000, DATA_HEX},
    [QSFPDD_P7_STUCK]                  =         {QSFPDD_0_7_16_23_STUCK_REG                , MASK_1000_0000, DATA_HEX},
    [QSFPDD_P8_STUCK]                  =         {QSFPDD_8_15_24_31_STUCK_REG               , MASK_0000_0001, DATA_HEX},
    [QSFPDD_P9_STUCK]                  =         {QSFPDD_8_15_24_31_STUCK_REG               , MASK_0000_0010, DATA_HEX},
    [QSFPDD_P10_STUCK]                 =         {QSFPDD_8_15_24_31_STUCK_REG               , MASK_0000_0100, DATA_HEX},
    [QSFPDD_P11_STUCK]                 =         {QSFPDD_8_15_24_31_STUCK_REG               , MASK_0000_1000, DATA_HEX},
    [QSFPDD_P12_STUCK]                 =         {QSFPDD_8_15_24_31_STUCK_REG               , MASK_0001_0000, DATA_HEX},
    [QSFPDD_P13_STUCK]                 =         {QSFPDD_8_15_24_31_STUCK_REG               , MASK_0010_0000, DATA_HEX},
    [QSFPDD_P14_STUCK]                 =         {QSFPDD_8_15_24_31_STUCK_REG               , MASK_0100_0000, DATA_HEX},
    [QSFPDD_P15_STUCK]                 =         {QSFPDD_8_15_24_31_STUCK_REG               , MASK_1000_0000, DATA_HEX},
    [QSFPDD_P16_STUCK]                 =         {QSFPDD_0_7_16_23_STUCK_REG                , MASK_0000_0001, DATA_HEX},
    [QSFPDD_P17_STUCK]                 =         {QSFPDD_0_7_16_23_STUCK_REG                , MASK_0000_0010, DATA_HEX},
    [QSFPDD_P18_STUCK]                 =         {QSFPDD_0_7_16_23_STUCK_REG                , MASK_0000_0100, DATA_HEX},
    [QSFPDD_P19_STUCK]                 =         {QSFPDD_0_7_16_23_STUCK_REG                , MASK_0000_1000, DATA_HEX},
    [QSFPDD_P20_STUCK]                 =         {QSFPDD_0_7_16_23_STUCK_REG                , MASK_0001_0000, DATA_HEX},
    [QSFPDD_P21_STUCK]                 =         {QSFPDD_0_7_16_23_STUCK_REG                , MASK_0010_0000, DATA_HEX},
    [QSFPDD_P22_STUCK]                 =         {QSFPDD_0_7_16_23_STUCK_REG                , MASK_0100_0000, DATA_HEX},
    [QSFPDD_P23_STUCK]                 =         {QSFPDD_0_7_16_23_STUCK_REG                , MASK_1000_0000, DATA_HEX},
    [QSFPDD_P24_STUCK]                 =         {QSFPDD_8_15_24_31_STUCK_REG               , MASK_0000_0001, DATA_HEX},
    [QSFPDD_P25_STUCK]                 =         {QSFPDD_8_15_24_31_STUCK_REG               , MASK_0000_0010, DATA_HEX},
    [QSFPDD_P26_STUCK]                 =         {QSFPDD_8_15_24_31_STUCK_REG               , MASK_0000_0100, DATA_HEX},
    [QSFPDD_P27_STUCK]                 =         {QSFPDD_8_15_24_31_STUCK_REG               , MASK_0000_1000, DATA_HEX},
    [QSFPDD_P28_STUCK]                 =         {QSFPDD_8_15_24_31_STUCK_REG               , MASK_0001_0000, DATA_HEX},
    [QSFPDD_P29_STUCK]                 =         {QSFPDD_8_15_24_31_STUCK_REG               , MASK_0010_0000, DATA_HEX},
    [QSFPDD_P30_STUCK]                 =         {QSFPDD_8_15_24_31_STUCK_REG               , MASK_0100_0000, DATA_HEX},
    [QSFPDD_P31_STUCK]                 =         {QSFPDD_8_15_24_31_STUCK_REG               , MASK_1000_0000, DATA_HEX},

    //FPGA
    [FPGA_MAJOR_VER]                   =         {FPGA_VERSION_REG                          , MASK_1100_0000, DATA_DEC},
    [FPGA_MINOR_VER]                   =         {FPGA_VERSION_REG                          , MASK_0011_1111, DATA_DEC},
    [FPGA_BUILD_VER]                   =         {FPGA_SUB_VERSION_REG                      , MASK_ALL      , DATA_DEC},
    [FPGA_VERSION_H]                   =         {NONE_REG                                  , MASK_NONE     , DATA_UNK},
    [FPGA_ID]                          =         {FPGA_ID_REG                               , MASK_0000_0111, DATA_DEC},
    [FPGA_MAC_ROV_1]                   =         {FPGA_MAC_ROV_1_REG                        , MASK_ALL      , DATA_DEC},
    [FPGA_MAC_ROV_2]                   =         {FPGA_MAC_ROV_2_REG                        , MASK_ALL      , DATA_DEC},

    // CPLD 4
    [MGMT_P0_TS]                       =         {MGMT_SFP_TS_REG                           , MASK_0000_0001, DATA_HEX},
    [MGMT_P1_TS]                       =         {MGMT_SFP_TS_REG                           , MASK_0000_0010, DATA_HEX},
    [SFP_P32_TS]                       =         {MGMT_SFP_TS_REG                           , MASK_0000_0100, DATA_HEX},
    [SFP_P33_TS]                       =         {MGMT_SFP_TS_REG                           , MASK_0000_1000, DATA_HEX},
    [SFP_P34_TS]                       =         {MGMT_SFP_TS_REG                           , MASK_0001_0000, DATA_HEX},
    [SFP_P35_TS]                       =         {MGMT_SFP_TS_REG                           , MASK_0010_0000, DATA_HEX},
    [MGMT_P0_RS]                       =         {MGMT_SFP_RS_REG                           , MASK_0000_0001, DATA_HEX},
    [MGMT_P1_RS]                       =         {MGMT_SFP_RS_REG                           , MASK_0000_0010, DATA_HEX},
    [SFP_P32_RS]                       =         {MGMT_SFP_RS_REG                           , MASK_0000_0100, DATA_HEX},
    [SFP_P33_RS]                       =         {MGMT_SFP_RS_REG                           , MASK_0000_1000, DATA_HEX},
    [SFP_P34_RS]                       =         {MGMT_SFP_RS_REG                           , MASK_0001_0000, DATA_HEX},
    [SFP_P35_RS]                       =         {MGMT_SFP_RS_REG                           , MASK_0010_0000, DATA_HEX},
    [MGMT_P0_TX_DIS]                   =         {MGMT_SFP_TX_DIS_REG                       , MASK_0000_0001, DATA_HEX},
    [MGMT_P1_TX_DIS]                   =         {MGMT_SFP_TX_DIS_REG                       , MASK_0000_0010, DATA_HEX},
    [SFP_P32_TX_DIS]                   =         {MGMT_SFP_TX_DIS_REG                       , MASK_0000_0100, DATA_HEX},
    [SFP_P33_TX_DIS]                   =         {MGMT_SFP_TX_DIS_REG                       , MASK_0000_1000, DATA_HEX},
    [SFP_P34_TX_DIS]                   =         {MGMT_SFP_TX_DIS_REG                       , MASK_0001_0000, DATA_HEX},
    [SFP_P35_TX_DIS]                   =         {MGMT_SFP_TX_DIS_REG                       , MASK_0010_0000, DATA_HEX},

    [MGMT_P0_TX_FLT]                   =         {MGMT_SFP_TX_FLT_REG                       , MASK_0000_0001, DATA_HEX},
    [MGMT_P1_TX_FLT]                   =         {MGMT_SFP_TX_FLT_REG                       , MASK_0000_0010, DATA_HEX},
    [SFP_P32_TX_FLT]                   =         {MGMT_SFP_TX_FLT_REG                       , MASK_0000_0100, DATA_HEX},
    [SFP_P33_TX_FLT]                   =         {MGMT_SFP_TX_FLT_REG                       , MASK_0000_1000, DATA_HEX},
    [SFP_P34_TX_FLT]                   =         {MGMT_SFP_TX_FLT_REG                       , MASK_0001_0000, DATA_HEX},
    [SFP_P35_TX_FLT]                   =         {MGMT_SFP_TX_FLT_REG                       , MASK_0010_0000, DATA_HEX},
    [MGMT_P0_RX_LOS]                   =         {MGMT_SFP_RX_LOS_REG                       , MASK_0000_0001, DATA_HEX},
    [MGMT_P1_RX_LOS]                   =         {MGMT_SFP_RX_LOS_REG                       , MASK_0000_0010, DATA_HEX},
    [SFP_P32_RX_LOS]                   =         {MGMT_SFP_RX_LOS_REG                       , MASK_0000_0100, DATA_HEX},
    [SFP_P33_RX_LOS]                   =         {MGMT_SFP_RX_LOS_REG                       , MASK_0000_1000, DATA_HEX},
    [SFP_P34_RX_LOS]                   =         {MGMT_SFP_RX_LOS_REG                       , MASK_0001_0000, DATA_HEX},
    [SFP_P35_RX_LOS]                   =         {MGMT_SFP_RX_LOS_REG                       , MASK_0010_0000, DATA_HEX},
    [MGMT_P0_ABS]                      =         {MGMT_SFP_ABS_REG                          , MASK_0000_0001, DATA_HEX},
    [MGMT_P1_ABS]                      =         {MGMT_SFP_ABS_REG                          , MASK_0000_0010, DATA_HEX},
    [SFP_P32_ABS]                      =         {MGMT_SFP_ABS_REG                          , MASK_0000_0100, DATA_HEX},
    [SFP_P33_ABS]                      =         {MGMT_SFP_ABS_REG                          , MASK_0000_1000, DATA_HEX},
    [SFP_P34_ABS]                      =         {MGMT_SFP_ABS_REG                          , MASK_0001_0000, DATA_HEX},
    [SFP_P35_ABS]                      =         {MGMT_SFP_ABS_REG                          , MASK_0010_0000, DATA_HEX},
    [MGMT_P0_TX_FLT_MASK]              =         {MGMT_SFP_TX_FLT_MASK_REG                  , MASK_0000_0001, DATA_HEX},
    [MGMT_P1_TX_FLT_MASK]              =         {MGMT_SFP_TX_FLT_MASK_REG                  , MASK_0000_0010, DATA_HEX},
    [SFP_P32_TX_FLT_MASK]              =         {MGMT_SFP_TX_FLT_MASK_REG                  , MASK_0000_0100, DATA_HEX},
    [SFP_P33_TX_FLT_MASK]              =         {MGMT_SFP_TX_FLT_MASK_REG                  , MASK_0000_1000, DATA_HEX},
    [SFP_P34_TX_FLT_MASK]              =         {MGMT_SFP_TX_FLT_MASK_REG                  , MASK_0001_0000, DATA_HEX},
    [SFP_P35_TX_FLT_MASK]              =         {MGMT_SFP_TX_FLT_MASK_REG                  , MASK_0010_0000, DATA_HEX},
    [MGMT_P0_RX_LOS_MASK]              =         {MGMT_SFP_RX_LOS_MASK_REG                  , MASK_0000_0001, DATA_HEX},
    [MGMT_P1_RX_LOS_MASK]              =         {MGMT_SFP_RX_LOS_MASK_REG                  , MASK_0000_0010, DATA_HEX},
    [SFP_P32_RX_LOS_MASK]              =         {MGMT_SFP_RX_LOS_MASK_REG                  , MASK_0000_0100, DATA_HEX},
    [SFP_P33_RX_LOS_MASK]              =         {MGMT_SFP_RX_LOS_MASK_REG                  , MASK_0000_1000, DATA_HEX},
    [SFP_P34_RX_LOS_MASK]              =         {MGMT_SFP_RX_LOS_MASK_REG                  , MASK_0001_0000, DATA_HEX},
    [SFP_P35_RX_LOS_MASK]              =         {MGMT_SFP_RX_LOS_MASK_REG                  , MASK_0010_0000, DATA_HEX},
    [MGMT_P0_ABS_MASK]                 =         {MGMT_SFP_ABS_MASK_REG                     , MASK_0000_0001, DATA_HEX},
    [MGMT_P1_ABS_MASK]                 =         {MGMT_SFP_ABS_MASK_REG                     , MASK_0000_0010, DATA_HEX},
    [SFP_P32_ABS_MASK]                 =         {MGMT_SFP_ABS_MASK_REG                     , MASK_0000_0100, DATA_HEX},
    [SFP_P33_ABS_MASK]                 =         {MGMT_SFP_ABS_MASK_REG                     , MASK_0000_1000, DATA_HEX},
    [SFP_P34_ABS_MASK]                 =         {MGMT_SFP_ABS_MASK_REG                     , MASK_0001_0000, DATA_HEX},
    [SFP_P35_ABS_MASK]                 =         {MGMT_SFP_ABS_MASK_REG                     , MASK_0010_0000, DATA_HEX},
    [MGMT_P0_TX_FLT_EVENT]             =         {MGMT_SFP_TX_FLT_EVENT_REG                 , MASK_0000_0001, DATA_HEX},
    [MGMT_P1_TX_FLT_EVENT]             =         {MGMT_SFP_TX_FLT_EVENT_REG                 , MASK_0000_0010, DATA_HEX},
    [SFP_P32_TX_FLT_EVENT]             =         {MGMT_SFP_TX_FLT_EVENT_REG                 , MASK_0000_0100, DATA_HEX},
    [SFP_P33_TX_FLT_EVENT]             =         {MGMT_SFP_TX_FLT_EVENT_REG                 , MASK_0000_1000, DATA_HEX},
    [SFP_P34_TX_FLT_EVENT]             =         {MGMT_SFP_TX_FLT_EVENT_REG                 , MASK_0001_0000, DATA_HEX},
    [SFP_P35_TX_FLT_EVENT]             =         {MGMT_SFP_TX_FLT_EVENT_REG                 , MASK_0010_0000, DATA_HEX},
    [MGMT_P0_RX_LOS_EVENT]             =         {MGMT_SFP_RX_LOS_EVENT_REG                 , MASK_0000_0001, DATA_HEX},
    [MGMT_P1_RX_LOS_EVENT]             =         {MGMT_SFP_RX_LOS_EVENT_REG                 , MASK_0000_0010, DATA_HEX},
    [SFP_P32_RX_LOS_EVENT]             =         {MGMT_SFP_RX_LOS_EVENT_REG                 , MASK_0000_0100, DATA_HEX},
    [SFP_P33_RX_LOS_EVENT]             =         {MGMT_SFP_RX_LOS_EVENT_REG                 , MASK_0000_1000, DATA_HEX},
    [SFP_P34_RX_LOS_EVENT]             =         {MGMT_SFP_RX_LOS_EVENT_REG                 , MASK_0001_0000, DATA_HEX},
    [SFP_P35_RX_LOS_EVENT]             =         {MGMT_SFP_RX_LOS_EVENT_REG                 , MASK_0010_0000, DATA_HEX},
    [MGMT_P0_ABS_EVENT]                =         {MGMT_SFP_ABS_EVENT_REG                    , MASK_0000_0001, DATA_HEX},
    [MGMT_P1_ABS_EVENT]                =         {MGMT_SFP_ABS_EVENT_REG                    , MASK_0000_0010, DATA_HEX},
    [SFP_P32_ABS_EVENT]                =         {MGMT_SFP_ABS_EVENT_REG                    , MASK_0000_0100, DATA_HEX},
    [SFP_P33_ABS_EVENT]                =         {MGMT_SFP_ABS_EVENT_REG                    , MASK_0000_1000, DATA_HEX},
    [SFP_P34_ABS_EVENT]                =         {MGMT_SFP_ABS_EVENT_REG                    , MASK_0001_0000, DATA_HEX},
    [SFP_P35_ABS_EVENT]                =         {MGMT_SFP_ABS_EVENT_REG                    , MASK_0010_0000, DATA_HEX},
    [MGMT_SFP_TX_FLT_EVENT]            =         {MGMT_SFP_TX_FLT_EVENT_REG                 , MASK_ALL      , DATA_HEX},
    [MGMT_SFP_RX_LOS_EVENT]            =         {MGMT_SFP_RX_LOS_EVENT_REG                 , MASK_ALL      , DATA_HEX},
    [MGMT_SFP_ABS_EVENT]               =         {MGMT_SFP_ABS_EVENT_REG                    , MASK_ALL      , DATA_HEX},

    // MUX
    [IDLE_STATE]                       =         {NONE_REG                                  , MASK_NONE     , DATA_UNK},

    //BSP DEBUG
    [BSP_DEBUG]                        =         {NONE_REG                                  , MASK_NONE     , DATA_UNK},
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

/* CPLD device id and data */
static const struct i2c_device_id cpld_id[] = {
    { "s9620_32e_cpld1",  cpld1 },
    { "s9620_32e_cpld2",  cpld2 },
    { "s9620_32e_cpld3",  cpld3 },
    { "s9620_32e_cpld4",  cpld4 },
    { "s9620_32e_fpga" ,  fpga  },
    {}
};

char bsp_debug[2]="0";
u8 enable_log_read=LOG_DISABLE;
u8 enable_log_write=LOG_DISABLE;

/* Addresses scanned for cpld */
static const unsigned short cpld_i2c_addr[] = { 0x30, 0x31, 0x32, 0x33, 0x37, I2C_CLIENT_END };

/* define all support register access of cpld in attribute */

// CPLD Common

static SENSOR_DEVICE_ATTR_RO(cpld_major_ver                , cpld, CPLD_MAJOR_VER);
static SENSOR_DEVICE_ATTR_RO(cpld_minor_ver                , cpld, CPLD_MINOR_VER);
static SENSOR_DEVICE_ATTR_RO(cpld_id                       , cpld, CPLD_ID);
static SENSOR_DEVICE_ATTR_RO(cpld_build_ver                , cpld, CPLD_BUILD_VER);
static SENSOR_DEVICE_ATTR_RO(cpld_version_h                , version_h, CPLD_VERSION_H);
static SENSOR_DEVICE_ATTR_RO(cpld_i2c_stuck                , cpld, CPLD_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(cpld_i2c_control              , cpld, CPLD_I2C_CONTROL);
static SENSOR_DEVICE_ATTR_RO(cpld_i2c_idle                 , cpld, CPLD_I2C_IDLE);
static SENSOR_DEVICE_ATTR_RO(cpld_i2c_relay                , cpld, CPLD_I2C_RELAY);

// CPLD 1
static SENSOR_DEVICE_ATTR_RO(board_sku_id                  , cpld, BRD_SKU_ID);
static SENSOR_DEVICE_ATTR_RO(board_hw_id                   , cpld, BRD_HW_ID);
static SENSOR_DEVICE_ATTR_RO(board_deph_id                 , cpld, BRD_DEPH_ID);
static SENSOR_DEVICE_ATTR_RO(board_build_id                , cpld, BRD_BUILD_ID);
static SENSOR_DEVICE_ATTR_RO(board_id_type                 , cpld, BRD_ID_TYPE);
static SENSOR_DEVICE_ATTR_RO(cpld_chip_type                , cpld, CPLD_CHIP_TYPE);
static SENSOR_DEVICE_ATTR_RO(mac_intr                      , cpld, MAC_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_25gphy_intr              , cpld, CPLD_25GPHY_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_fru_intr                 , cpld, CPLD_FRU_INTR);
static SENSOR_DEVICE_ATTR_RO(ntm_intr                      , cpld, NTM_INTR);
static SENSOR_DEVICE_ATTR_RO(thermal_alert_1_intr          , cpld, THERMAL_ALERT_1_INTR);
static SENSOR_DEVICE_ATTR_RO(thermal_alert_2_intr          , cpld, THERMAL_ALERT_2_INTR);
static SENSOR_DEVICE_ATTR_RO(thermal_alert_3_intr          , cpld, THERMAL_ALERT_3_INTR);
static SENSOR_DEVICE_ATTR_RO(misc_intr                     , cpld, MISC_INTR);
static SENSOR_DEVICE_ATTR_RO(system_intr                   , cpld, SYSTEM_INTR);

static SENSOR_DEVICE_ATTR_RO(cpld_2_intr                   , cpld, CPLD_2_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_3_intr                   , cpld, CPLD_3_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_4_intr                   , cpld, CPLD_4_INTR);
static SENSOR_DEVICE_ATTR_RO(psu_0_intr                    , cpld, PSU_0_INTR);
static SENSOR_DEVICE_ATTR_RO(psu_1_intr                    , cpld, PSU_1_INTR);

static SENSOR_DEVICE_ATTR_RO(mac_hbm_temp_alert            , cpld, MAC_HBM_TEMP_ALERT);
static SENSOR_DEVICE_ATTR_RO(mb_lm75_temp_alert            , cpld, MB_LM75_TEMP_ALERT);
static SENSOR_DEVICE_ATTR_RO(ext_lm75_temp_alert           , cpld, EXT_LM75_TEMP_ALERT);
static SENSOR_DEVICE_ATTR_RO(phy_lm75_cpld5_intr           , cpld, PHY_LM75_CPLD5_INTR);
static SENSOR_DEVICE_ATTR_RO(usb_ocp                       , cpld, USB_OCP);
static SENSOR_DEVICE_ATTR_RO(fpga_intr                     , cpld, FPGA_INTR);
static SENSOR_DEVICE_ATTR_RO(thermal_alert_intr            , cpld, THERMAL_ALERT_INTR);
static SENSOR_DEVICE_ATTR_RO(psu_cpld_intr                 , cpld, PSU_CPLD_INTR);
static SENSOR_DEVICE_ATTR_RO(main_chip_intr                , cpld, MAIN_CHIP_INTR);
static SENSOR_DEVICE_ATTR_RO(ptp_intr                      , cpld, PTP_INTR);

static SENSOR_DEVICE_ATTR_RW(mac_intr_mask                 , cpld, MAC_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_25gphy_intr_mask         , cpld, CPLD_25GPHY_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_fru_intr_mask            , cpld, CPLD_FRU_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(ntm_intr_mask                 , cpld, NTM_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(thermal_alert_1_intr_mask     , cpld, THERMAL_ALERT_1_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(thermal_alert_2_intr_mask     , cpld, THERMAL_ALERT_2_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(thermal_alert_3_intr_mask     , cpld, THERMAL_ALERT_3_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(misc_intr_mask                , cpld, MISC_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(system_intr_mask              , cpld, SYSTEM_INTR_MASK);

static SENSOR_DEVICE_ATTR_RW(cpld_2_intr_mask              , cpld, CPLD_2_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_3_intr_mask              , cpld, CPLD_3_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_4_intr_mask              , cpld, CPLD_4_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(psu_0_intr_mask               , cpld, PSU_0_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(psu_1_intr_mask               , cpld, PSU_1_INTR_MASK);

static SENSOR_DEVICE_ATTR_RW(usb_ocp_intr_mask             , cpld, USB_OCP_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(mac_temp_alert_mask           , cpld, MAC_TEMP_ALERT_MASK);
static SENSOR_DEVICE_ATTR_RW(mb_lm75_temp_alert_mask       , cpld, MB_LM75_TEMP_ALERT_MASK);
static SENSOR_DEVICE_ATTR_RW(mb_temp_0_alert_mask          , cpld, MB_TEMP_1_ALERT_MASK);
static SENSOR_DEVICE_ATTR_RW(mb_temp_1_alert_mask          , cpld, MB_TEMP_2_ALERT_MASK);
static SENSOR_DEVICE_ATTR_RW(mb_temp_2_alert_mask          , cpld, MB_TEMP_3_ALERT_MASK);
static SENSOR_DEVICE_ATTR_RW(mb_temp_3_alert_mask          , cpld, MB_TEMP_4_ALERT_MASK);
static SENSOR_DEVICE_ATTR_RW(mb_temp_4_alert_mask          , cpld, MB_TEMP_5_ALERT_MASK);
static SENSOR_DEVICE_ATTR_RW(mb_temp_5_alert_mask          , cpld, MB_TEMP_6_ALERT_MASK);
static SENSOR_DEVICE_ATTR_RW(mb_temp_6_alert_mask          , cpld, MB_TEMP_7_ALERT_MASK);
static SENSOR_DEVICE_ATTR_RW(top_cpld_5_intr_mask          , cpld, TOP_CPLD_5_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(usb_ocp_mask                  , cpld, USB_OCP_MASK);
static SENSOR_DEVICE_ATTR_RW(fpga_intr_mask                , cpld, FPGA_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(thermal_alert_intr_mask       , cpld, THERMAL_ALERT_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(psu_cpld_intr_mask            , cpld, PSU_CPLD_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(main_chip_intr_mask           , cpld, MAIN_CHIP_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(ptp_intr_mask                 , cpld, PTP_INTR_MASK);

static SENSOR_DEVICE_ATTR_RO(mac_intr_event                , cpld, MAC_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_fru_intr_event           , cpld, CPLD_FRU_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_25gphy_intr_event        , cpld, CPLD_25GPHY_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(ntm_intr_event                , cpld, NTM_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(thermal_alert_1_intr_event    , cpld, THERMAL_ALERT_1_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(thermal_alert_2_intr_event    , cpld, THERMAL_ALERT_2_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(thermal_alert_3_intr_event    , cpld, THERMAL_ALERT_3_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(misc_intr_event               , cpld, MISC_INTR_EVENT);

static SENSOR_DEVICE_ATTR_RO(mac_temp_alert_event          , cpld, MAC_TEMP_ALERT_EVENT);
static SENSOR_DEVICE_ATTR_RO(temp_alert_event              , cpld, TEMP_ALERT_EVENT);
static SENSOR_DEVICE_ATTR_RW(event_detect_ctrl             , cpld, EVENT_DETECT_CTRL);

static SENSOR_DEVICE_ATTR_RW(mac_rst                       , cpld, MAC_RST);
static SENSOR_DEVICE_ATTR_RW(bios_flash_rst                , cpld, BIOS_FLASH_RST);
static SENSOR_DEVICE_ATTR_RW(bmc_rst                       , cpld, BMC_RST);
static SENSOR_DEVICE_ATTR_RW(bmc_pcie_rst                  , cpld, BMC_PCIE_RST);
static SENSOR_DEVICE_ATTR_RW(bmc_lpc_rst                   , cpld, BMC_LPC_RST);
static SENSOR_DEVICE_ATTR_RW(bmc_wdt_rst                   , cpld, BMC_WDT_RST);
static SENSOR_DEVICE_ATTR_RW(ntm_rst                       , cpld, NTM_RST);
static SENSOR_DEVICE_ATTR_RW(usb_redriver_ext_rst          , cpld, USB_REDRIVER_EXT_RST);
static SENSOR_DEVICE_ATTR_RW(mac_qspi_flash_rst            , cpld, MAC_QSPI_FLASH_RST);
static SENSOR_DEVICE_ATTR_RW(cp2130_rst                    , cpld, CP2130_RST);
static SENSOR_DEVICE_ATTR_RW(usb_redriver_ntm_rst          , cpld, USB_REDRIVER_NTM_RST);
static SENSOR_DEVICE_ATTR_RW(usb_redriver_mb_rst           , cpld, USB_REDRIVER_MB_RST);
static SENSOR_DEVICE_ATTR_RW(usb_curr_limiter_en           , cpld, USB_CURR_LIMITER_EN);

static SENSOR_DEVICE_ATTR_RW(cpld_2_rst                    , cpld, CPLD_2_RST);
static SENSOR_DEVICE_ATTR_RW(cpld_3_rst                    , cpld, CPLD_3_RST);
static SENSOR_DEVICE_ATTR_RW(fpga_rst                      , cpld, FPGA_RST);

static SENSOR_DEVICE_ATTR_RW(i2c_mux_rst                   , cpld, I2C_MUX_RST);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_9548_1_rst            , cpld, I2C_MUX_9548_1_RST);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_9548_2_rst            , cpld, I2C_MUX_9548_2_RST);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_fan_rst               , cpld, I2C_MUX_FAN_RST);

static SENSOR_DEVICE_ATTR_RW(bcm82399_rst                  , cpld, BCM82399_RST);
static SENSOR_DEVICE_ATTR_RW(cp2102_rst                    , cpld, CP2102_RST);
static SENSOR_DEVICE_ATTR_RW(rc32312_rst                   , cpld, RC32312_RST);
static SENSOR_DEVICE_ATTR_RW(i210_rst                      , cpld, I210_RST);
static SENSOR_DEVICE_ATTR_RW(ioexp_rst                     , cpld, IOEXP_RST);
static SENSOR_DEVICE_ATTR_RW(fpga_pcie_rst                 , cpld, FPGA_PCIE_RST);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_oob_rst               , cpld, I2C_MUX_OOB_RST);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_ts_rst                , cpld, I2C_MUX_TS_RST);

//static SENSOR_DEVICE_ATTR_RW(psu_rst                       , cpld, PSU_RST);

static SENSOR_DEVICE_ATTR_RO(psu_0_prsnt                   , cpld, PSU_0_PRSNT);
static SENSOR_DEVICE_ATTR_RO(psu_1_prsnt                   , cpld, PSU_1_PRSNT);
static SENSOR_DEVICE_ATTR_RO(psu_0_acin                    , cpld, PSU_0_ACIN);
static SENSOR_DEVICE_ATTR_RO(psu_1_acin                    , cpld, PSU_1_ACIN);
static SENSOR_DEVICE_ATTR_RO(psu_0_pg                      , cpld, PSU_0_PG);
static SENSOR_DEVICE_ATTR_RO(psu_1_pg                      , cpld, PSU_1_PG);

static SENSOR_DEVICE_ATTR_RW(cpu_mux_sel                   , cpld, CPU_MUX_SEL);
static SENSOR_DEVICE_ATTR_RW(psu_mux_sel                   , cpld, PSU_MUX_SEL);
static SENSOR_DEVICE_ATTR_RW(fpga_qspi_sel                 , cpld, FPGA_QSPI_SEL);
static SENSOR_DEVICE_ATTR_RW(bmc_uart_mux_sel              , cpld, BMC_UART_MUX_SEL);
static SENSOR_DEVICE_ATTR_RW(system_uart_mux_sel           , cpld, SYSTEM_UART_MUX_SEL);

static SENSOR_DEVICE_ATTR_RW(fpga_ctrl                     , cpld, FPGA_CTRL);
static SENSOR_DEVICE_ATTR_RW(fpga_nconfig                  , cpld, FPGA_NCONFIG);
static SENSOR_DEVICE_ATTR_RW(fpga_config_done              , cpld, FPGA_CONFIG_DONE);

static SENSOR_DEVICE_ATTR_RW(cpld_system_led_sys           , cpld, CPLD_SYSTEM_LED_SYS);
static SENSOR_DEVICE_ATTR_RW(system_led_status             , cpld, SYSTEM_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(system_led_speed              , cpld, SYSTEM_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(system_led_blink              , cpld, SYSTEM_LED_BLINK);
static SENSOR_DEVICE_ATTR_RW(system_led_onoff              , cpld, SYSTEM_LED_ONOFF);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_fan           , cpld, CPLD_SYSTEM_LED_FAN);
static SENSOR_DEVICE_ATTR_RW(fan_led_status                , cpld, FAN_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(fan_led_speed                 , cpld, FAN_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(fan_led_blink                 , cpld, FAN_LED_BLINK);
static SENSOR_DEVICE_ATTR_RW(fan_led_onoff                 , cpld, FAN_LED_ONOFF);

static SENSOR_DEVICE_ATTR_RW(cpld_system_led_psu_0         , cpld, CPLD_SYSTEM_LED_PSU_0);
static SENSOR_DEVICE_ATTR_RW(psu_0_led_status              , cpld, PSU_0_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(psu_0_led_speed               , cpld, PSU_0_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(psu_0_led_blink               , cpld, PSU_0_LED_BLINK);
static SENSOR_DEVICE_ATTR_RW(psu_0_led_onoff               , cpld, PSU_0_LED_ONOFF);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_psu_1         , cpld, CPLD_SYSTEM_LED_PSU_1);
static SENSOR_DEVICE_ATTR_RW(psu_1_led_status              , cpld, PSU_1_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(psu_1_led_speed               , cpld, PSU_1_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(psu_1_led_blink               , cpld, PSU_1_LED_BLINK);
static SENSOR_DEVICE_ATTR_RW(psu_1_led_onoff               , cpld, PSU_1_LED_ONOFF);

static SENSOR_DEVICE_ATTR_RW(cpld_system_led_sync          , cpld, CPLD_SYSTEM_LED_SYNC);
static SENSOR_DEVICE_ATTR_RW(sync_led_status               , cpld, SYNC_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(sync_led_speed                , cpld, SYNC_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(sync_led_blink                , cpld, SYNC_LED_BLINK);
static SENSOR_DEVICE_ATTR_RW(sync_led_onoff                , cpld, SYNC_LED_ONOFF);

static SENSOR_DEVICE_ATTR_RW(cpld_system_led_id            , cpld, CPLD_SYSTEM_LED_ID);
static SENSOR_DEVICE_ATTR_RW(id_led_status                 , cpld, ID_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(id_led_speed                  , cpld, ID_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(id_led_blink                  , cpld, ID_LED_BLINK);
static SENSOR_DEVICE_ATTR_RW(id_led_onoff                  , cpld, ID_LED_ONOFF);

static SENSOR_DEVICE_ATTR_RW(led_clear                     , cpld, LED_CLEAR);

// TBD not used, remove
static SENSOR_DEVICE_ATTR_RW(mgmt_0_led_status             , cpld, MGMT_0_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(mgmt_0_led_speed              , cpld, MGMT_0_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(mgmt_0_led_blink              , cpld, MGMT_0_LED_BLINK);
static SENSOR_DEVICE_ATTR_RW(mgmt_0_led_onoff              , cpld, MGMT_0_LED_ONOFF);
static SENSOR_DEVICE_ATTR_RW(mgmt_1_led_status             , cpld, MGMT_1_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(mgmt_1_led_speed              , cpld, MGMT_1_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(mgmt_1_led_blink              , cpld, MGMT_1_LED_BLINK);
static SENSOR_DEVICE_ATTR_RW(mgmt_1_led_onoff              , cpld, MGMT_1_LED_ONOFF);

static SENSOR_DEVICE_ATTR_RW(mgmt_p0_led                    , cpld, MGMT_P0_LED);
static SENSOR_DEVICE_ATTR_RW(mgmt_p1_led                    , cpld, MGMT_P1_LED);

// CPLD 2
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p0_intr                , cpld, QSFPDD_P0_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p1_intr                , cpld, QSFPDD_P1_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p2_intr                , cpld, QSFPDD_P2_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p3_intr                , cpld, QSFPDD_P3_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p4_intr                , cpld, QSFPDD_P4_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p5_intr                , cpld, QSFPDD_P5_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p6_intr                , cpld, QSFPDD_P6_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p7_intr                , cpld, QSFPDD_P7_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p8_intr                , cpld, QSFPDD_P8_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p9_intr                , cpld, QSFPDD_P9_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p10_intr               , cpld, QSFPDD_P10_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p11_intr               , cpld, QSFPDD_P11_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p12_intr               , cpld, QSFPDD_P12_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p13_intr               , cpld, QSFPDD_P13_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p14_intr               , cpld, QSFPDD_P14_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p15_intr               , cpld, QSFPDD_P15_INTR);

static SENSOR_DEVICE_ATTR_RO(qsfpdd_p0_abs                 , cpld, QSFPDD_P0_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p1_abs                 , cpld, QSFPDD_P1_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p2_abs                 , cpld, QSFPDD_P2_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p3_abs                 , cpld, QSFPDD_P3_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p4_abs                 , cpld, QSFPDD_P4_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p5_abs                 , cpld, QSFPDD_P5_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p6_abs                 , cpld, QSFPDD_P6_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p7_abs                 , cpld, QSFPDD_P7_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p8_abs                 , cpld, QSFPDD_P8_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p9_abs                 , cpld, QSFPDD_P9_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p10_abs                , cpld, QSFPDD_P10_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p11_abs                , cpld, QSFPDD_P11_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p12_abs                , cpld, QSFPDD_P12_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p13_abs                , cpld, QSFPDD_P13_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p14_abs                , cpld, QSFPDD_P14_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p15_abs                , cpld, QSFPDD_P15_ABS);

static SENSOR_DEVICE_ATTR_RO(qsfpdd_p0_7_abs_event         , cpld, QSFPDD_P0_7_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p8_15_abs_event        , cpld, QSFPDD_P8_15_ABS_EVENT);

static SENSOR_DEVICE_ATTR_RO(qsfpdd_p0_7_intr_event        , cpld, QSFPDD_P0_7_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p8_15_intr_event       , cpld, QSFPDD_P8_15_INTR_EVENT);

static SENSOR_DEVICE_ATTR_RO(qsfpdd_p0_7_fuse_intr_event   , cpld, QSFPDD_P0_7_FUSE_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p8_15_fuse_intr_event  , cpld, QSFPDD_P8_15_FUSE_INTR_EVENT);

static SENSOR_DEVICE_ATTR_RO(qsfpdd_p0_fuse_intr           , cpld, QSFPDD_P0_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p1_fuse_intr           , cpld, QSFPDD_P1_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p2_fuse_intr           , cpld, QSFPDD_P2_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p3_fuse_intr           , cpld, QSFPDD_P3_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p4_fuse_intr           , cpld, QSFPDD_P4_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p5_fuse_intr           , cpld, QSFPDD_P5_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p6_fuse_intr           , cpld, QSFPDD_P6_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p7_fuse_intr           , cpld, QSFPDD_P7_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p8_fuse_intr           , cpld, QSFPDD_P8_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p9_fuse_intr           , cpld, QSFPDD_P9_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p10_fuse_intr          , cpld, QSFPDD_P10_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p11_fuse_intr          , cpld, QSFPDD_P11_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p12_fuse_intr          , cpld, QSFPDD_P12_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p13_fuse_intr          , cpld, QSFPDD_P13_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p14_fuse_intr          , cpld, QSFPDD_P14_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p15_fuse_intr          , cpld, QSFPDD_P15_FUSE_INTR);

static SENSOR_DEVICE_ATTR_RW(qsfpdd_p0_intr_mask           , cpld, QSFPDD_P0_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p1_intr_mask           , cpld, QSFPDD_P1_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p2_intr_mask           , cpld, QSFPDD_P2_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p3_intr_mask           , cpld, QSFPDD_P3_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p4_intr_mask           , cpld, QSFPDD_P4_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p5_intr_mask           , cpld, QSFPDD_P5_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p6_intr_mask           , cpld, QSFPDD_P6_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p7_intr_mask           , cpld, QSFPDD_P7_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p8_intr_mask           , cpld, QSFPDD_P8_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p9_intr_mask           , cpld, QSFPDD_P9_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p10_intr_mask          , cpld, QSFPDD_P10_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p11_intr_mask          , cpld, QSFPDD_P11_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p12_intr_mask          , cpld, QSFPDD_P12_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p13_intr_mask          , cpld, QSFPDD_P13_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p14_intr_mask          , cpld, QSFPDD_P14_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p15_intr_mask          , cpld, QSFPDD_P15_INTR_MASK);

static SENSOR_DEVICE_ATTR_RW(qsfpdd_p0_abs_mask            , cpld, QSFPDD_P0_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p1_abs_mask            , cpld, QSFPDD_P1_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p2_abs_mask            , cpld, QSFPDD_P2_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p3_abs_mask            , cpld, QSFPDD_P3_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p4_abs_mask            , cpld, QSFPDD_P4_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p5_abs_mask            , cpld, QSFPDD_P5_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p6_abs_mask            , cpld, QSFPDD_P6_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p7_abs_mask            , cpld, QSFPDD_P7_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p8_abs_mask            , cpld, QSFPDD_P8_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p9_abs_mask            , cpld, QSFPDD_P9_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p10_abs_mask           , cpld, QSFPDD_P10_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p11_abs_mask           , cpld, QSFPDD_P11_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p12_abs_mask           , cpld, QSFPDD_P12_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p13_abs_mask           , cpld, QSFPDD_P13_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p14_abs_mask           , cpld, QSFPDD_P14_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p15_abs_mask           , cpld, QSFPDD_P15_ABS_MASK);

static SENSOR_DEVICE_ATTR_RW(qsfpdd_p0_fuse_intr_mask      , cpld, QSFPDD_P0_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p1_fuse_intr_mask      , cpld, QSFPDD_P1_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p2_fuse_intr_mask      , cpld, QSFPDD_P2_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p3_fuse_intr_mask      , cpld, QSFPDD_P3_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p4_fuse_intr_mask      , cpld, QSFPDD_P4_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p5_fuse_intr_mask      , cpld, QSFPDD_P5_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p6_fuse_intr_mask      , cpld, QSFPDD_P6_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p7_fuse_intr_mask      , cpld, QSFPDD_P7_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p8_fuse_intr_mask      , cpld, QSFPDD_P8_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p9_fuse_intr_mask      , cpld, QSFPDD_P9_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p10_fuse_intr_mask     , cpld, QSFPDD_P10_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p11_fuse_intr_mask     , cpld, QSFPDD_P11_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p12_fuse_intr_mask     , cpld, QSFPDD_P12_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p13_fuse_intr_mask     , cpld, QSFPDD_P13_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p14_fuse_intr_mask     , cpld, QSFPDD_P14_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p15_fuse_intr_mask     , cpld, QSFPDD_P15_FUSE_INTR_MASK);

static SENSOR_DEVICE_ATTR_RW(qsfpdd_p0_rst                 , cpld, QSFPDD_P0_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p1_rst                 , cpld, QSFPDD_P1_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p2_rst                 , cpld, QSFPDD_P2_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p3_rst                 , cpld, QSFPDD_P3_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p4_rst                 , cpld, QSFPDD_P4_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p5_rst                 , cpld, QSFPDD_P5_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p6_rst                 , cpld, QSFPDD_P6_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p7_rst                 , cpld, QSFPDD_P7_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p8_rst                 , cpld, QSFPDD_P8_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p9_rst                 , cpld, QSFPDD_P9_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p10_rst                , cpld, QSFPDD_P10_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p11_rst                , cpld, QSFPDD_P11_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p12_rst                , cpld, QSFPDD_P12_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p13_rst                , cpld, QSFPDD_P13_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p14_rst                , cpld, QSFPDD_P14_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p15_rst                , cpld, QSFPDD_P15_RST);

static SENSOR_DEVICE_ATTR_RW(qsfpdd_p0_lp_mode             , cpld, QSFPDD_P0_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p1_lp_mode             , cpld, QSFPDD_P1_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p2_lp_mode             , cpld, QSFPDD_P2_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p3_lp_mode             , cpld, QSFPDD_P3_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p4_lp_mode             , cpld, QSFPDD_P4_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p5_lp_mode             , cpld, QSFPDD_P5_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p6_lp_mode             , cpld, QSFPDD_P6_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p7_lp_mode             , cpld, QSFPDD_P7_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p8_lp_mode             , cpld, QSFPDD_P8_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p9_lp_mode             , cpld, QSFPDD_P9_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p10_lp_mode            , cpld, QSFPDD_P10_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p11_lp_mode            , cpld, QSFPDD_P11_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p12_lp_mode            , cpld, QSFPDD_P12_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p13_lp_mode            , cpld, QSFPDD_P13_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p14_lp_mode            , cpld, QSFPDD_P14_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p15_lp_mode            , cpld, QSFPDD_P15_LP_MODE);

static SENSOR_DEVICE_ATTR_RO(qsfpdd_p0_stuck               , cpld, QSFPDD_P0_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p1_stuck               , cpld, QSFPDD_P1_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p2_stuck               , cpld, QSFPDD_P2_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p3_stuck               , cpld, QSFPDD_P3_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p4_stuck               , cpld, QSFPDD_P4_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p5_stuck               , cpld, QSFPDD_P5_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p6_stuck               , cpld, QSFPDD_P6_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p7_stuck               , cpld, QSFPDD_P7_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p8_stuck               , cpld, QSFPDD_P8_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p9_stuck               , cpld, QSFPDD_P9_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p10_stuck              , cpld, QSFPDD_P10_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p11_stuck              , cpld, QSFPDD_P11_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p12_stuck              , cpld, QSFPDD_P12_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p13_stuck              , cpld, QSFPDD_P13_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p14_stuck              , cpld, QSFPDD_P14_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p15_stuck              , cpld, QSFPDD_P15_STUCK);

// FPGA
static SENSOR_DEVICE_ATTR_RO(fpga_major_ver                , cpld, FPGA_MAJOR_VER);
static SENSOR_DEVICE_ATTR_RO(fpga_minor_ver                , cpld, FPGA_MINOR_VER);
static SENSOR_DEVICE_ATTR_RO(fpga_build_ver                , cpld, FPGA_BUILD_VER);
static SENSOR_DEVICE_ATTR_RO(fpga_version_h                , version_h, FPGA_VERSION_H);
static SENSOR_DEVICE_ATTR_RO(fpga_id                       , cpld, FPGA_ID);
static SENSOR_DEVICE_ATTR_RO(fpga_mac_rov_1                , cpld, FPGA_MAC_ROV_1);
static SENSOR_DEVICE_ATTR_RO(fpga_mac_rov_2                , cpld, FPGA_MAC_ROV_2);

 // CPLD 3

static SENSOR_DEVICE_ATTR_RO(qsfpdd_p16_intr               , cpld, QSFPDD_P16_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p17_intr               , cpld, QSFPDD_P17_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p18_intr               , cpld, QSFPDD_P18_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p19_intr               , cpld, QSFPDD_P19_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p20_intr               , cpld, QSFPDD_P20_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p21_intr               , cpld, QSFPDD_P21_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p22_intr               , cpld, QSFPDD_P22_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p23_intr               , cpld, QSFPDD_P23_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p24_intr               , cpld, QSFPDD_P24_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p25_intr               , cpld, QSFPDD_P25_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p26_intr               , cpld, QSFPDD_P26_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p27_intr               , cpld, QSFPDD_P27_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p28_intr               , cpld, QSFPDD_P28_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p29_intr               , cpld, QSFPDD_P29_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p30_intr               , cpld, QSFPDD_P30_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p31_intr               , cpld, QSFPDD_P31_INTR);

static SENSOR_DEVICE_ATTR_RO(qsfpdd_p16_abs                , cpld, QSFPDD_P16_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p17_abs                , cpld, QSFPDD_P17_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p18_abs                , cpld, QSFPDD_P18_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p19_abs                , cpld, QSFPDD_P19_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p20_abs                , cpld, QSFPDD_P20_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p21_abs                , cpld, QSFPDD_P21_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p22_abs                , cpld, QSFPDD_P22_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p23_abs                , cpld, QSFPDD_P23_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p24_abs                , cpld, QSFPDD_P24_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p25_abs                , cpld, QSFPDD_P25_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p26_abs                , cpld, QSFPDD_P26_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p27_abs                , cpld, QSFPDD_P27_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p28_abs                , cpld, QSFPDD_P28_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p29_abs                , cpld, QSFPDD_P29_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p30_abs                , cpld, QSFPDD_P30_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p31_abs                , cpld, QSFPDD_P31_ABS);

static SENSOR_DEVICE_ATTR_RO(qsfpdd_p16_23_abs_event       , cpld, QSFPDD_P16_23_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p24_31_abs_event       , cpld, QSFPDD_P24_31_ABS_EVENT);

static SENSOR_DEVICE_ATTR_RO(qsfpdd_p16_23_intr_event      , cpld, QSFPDD_P16_23_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p24_31_intr_event      , cpld, QSFPDD_P24_31_INTR_EVENT);

static SENSOR_DEVICE_ATTR_RO(qsfpdd_p16_23_fuse_intr_event   , cpld, QSFPDD_P16_23_FUSE_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p24_31_fuse_intr_event  , cpld, QSFPDD_P24_31_FUSE_INTR_EVENT);

static SENSOR_DEVICE_ATTR_RO(qsfpdd_p16_fuse_intr          , cpld, QSFPDD_P16_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p17_fuse_intr          , cpld, QSFPDD_P17_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p18_fuse_intr          , cpld, QSFPDD_P18_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p19_fuse_intr          , cpld, QSFPDD_P19_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p20_fuse_intr          , cpld, QSFPDD_P20_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p21_fuse_intr          , cpld, QSFPDD_P21_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p22_fuse_intr          , cpld, QSFPDD_P22_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p23_fuse_intr          , cpld, QSFPDD_P23_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p24_fuse_intr          , cpld, QSFPDD_P24_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p25_fuse_intr          , cpld, QSFPDD_P25_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p26_fuse_intr          , cpld, QSFPDD_P26_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p27_fuse_intr          , cpld, QSFPDD_P27_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p28_fuse_intr          , cpld, QSFPDD_P28_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p29_fuse_intr          , cpld, QSFPDD_P29_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p30_fuse_intr          , cpld, QSFPDD_P30_FUSE_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p31_fuse_intr          , cpld, QSFPDD_P31_FUSE_INTR);

static SENSOR_DEVICE_ATTR_RW(qsfpdd_p16_intr_mask          , cpld, QSFPDD_P16_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p17_intr_mask          , cpld, QSFPDD_P17_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p18_intr_mask          , cpld, QSFPDD_P18_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p19_intr_mask          , cpld, QSFPDD_P19_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p20_intr_mask          , cpld, QSFPDD_P20_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p21_intr_mask          , cpld, QSFPDD_P21_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p22_intr_mask          , cpld, QSFPDD_P22_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p23_intr_mask          , cpld, QSFPDD_P23_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p24_intr_mask          , cpld, QSFPDD_P24_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p25_intr_mask          , cpld, QSFPDD_P25_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p26_intr_mask          , cpld, QSFPDD_P26_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p27_intr_mask          , cpld, QSFPDD_P27_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p28_intr_mask          , cpld, QSFPDD_P28_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p29_intr_mask          , cpld, QSFPDD_P29_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p30_intr_mask          , cpld, QSFPDD_P30_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p31_intr_mask          , cpld, QSFPDD_P31_INTR_MASK);

static SENSOR_DEVICE_ATTR_RW(qsfpdd_p16_abs_mask           , cpld, QSFPDD_P16_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p17_abs_mask           , cpld, QSFPDD_P17_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p18_abs_mask           , cpld, QSFPDD_P18_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p19_abs_mask           , cpld, QSFPDD_P19_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p20_abs_mask           , cpld, QSFPDD_P20_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p21_abs_mask           , cpld, QSFPDD_P21_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p22_abs_mask           , cpld, QSFPDD_P22_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p23_abs_mask           , cpld, QSFPDD_P23_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p24_abs_mask           , cpld, QSFPDD_P24_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p25_abs_mask           , cpld, QSFPDD_P25_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p26_abs_mask           , cpld, QSFPDD_P26_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p27_abs_mask           , cpld, QSFPDD_P27_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p28_abs_mask           , cpld, QSFPDD_P28_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p29_abs_mask           , cpld, QSFPDD_P29_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p30_abs_mask           , cpld, QSFPDD_P30_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p31_abs_mask           , cpld, QSFPDD_P31_ABS_MASK);

static SENSOR_DEVICE_ATTR_RW(qsfpdd_p16_fuse_intr_mask     , cpld, QSFPDD_P16_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p17_fuse_intr_mask     , cpld, QSFPDD_P17_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p18_fuse_intr_mask     , cpld, QSFPDD_P18_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p19_fuse_intr_mask     , cpld, QSFPDD_P19_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p20_fuse_intr_mask     , cpld, QSFPDD_P20_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p21_fuse_intr_mask     , cpld, QSFPDD_P21_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p22_fuse_intr_mask     , cpld, QSFPDD_P22_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p23_fuse_intr_mask     , cpld, QSFPDD_P23_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p24_fuse_intr_mask     , cpld, QSFPDD_P24_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p25_fuse_intr_mask     , cpld, QSFPDD_P25_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p26_fuse_intr_mask     , cpld, QSFPDD_P26_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p27_fuse_intr_mask     , cpld, QSFPDD_P27_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p28_fuse_intr_mask     , cpld, QSFPDD_P28_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p29_fuse_intr_mask     , cpld, QSFPDD_P29_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p30_fuse_intr_mask     , cpld, QSFPDD_P30_FUSE_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p31_fuse_intr_mask     , cpld, QSFPDD_P31_FUSE_INTR_MASK);

static SENSOR_DEVICE_ATTR_RW(qsfpdd_p16_rst                , cpld, QSFPDD_P16_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p17_rst                , cpld, QSFPDD_P17_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p18_rst                , cpld, QSFPDD_P18_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p19_rst                , cpld, QSFPDD_P19_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p20_rst                , cpld, QSFPDD_P20_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p21_rst                , cpld, QSFPDD_P21_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p22_rst                , cpld, QSFPDD_P22_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p23_rst                , cpld, QSFPDD_P23_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p24_rst                , cpld, QSFPDD_P24_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p25_rst                , cpld, QSFPDD_P25_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p26_rst                , cpld, QSFPDD_P26_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p27_rst                , cpld, QSFPDD_P27_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p28_rst                , cpld, QSFPDD_P28_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p29_rst                , cpld, QSFPDD_P29_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p30_rst                , cpld, QSFPDD_P30_RST);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p31_rst                , cpld, QSFPDD_P31_RST);

static SENSOR_DEVICE_ATTR_RW(qsfpdd_p16_lp_mode            , cpld, QSFPDD_P16_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p17_lp_mode            , cpld, QSFPDD_P17_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p18_lp_mode            , cpld, QSFPDD_P18_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p19_lp_mode            , cpld, QSFPDD_P19_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p20_lp_mode            , cpld, QSFPDD_P20_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p21_lp_mode            , cpld, QSFPDD_P21_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p22_lp_mode            , cpld, QSFPDD_P22_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p23_lp_mode            , cpld, QSFPDD_P23_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p24_lp_mode            , cpld, QSFPDD_P24_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p25_lp_mode            , cpld, QSFPDD_P25_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p26_lp_mode            , cpld, QSFPDD_P26_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p27_lp_mode            , cpld, QSFPDD_P27_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p28_lp_mode            , cpld, QSFPDD_P28_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p29_lp_mode            , cpld, QSFPDD_P29_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p30_lp_mode            , cpld, QSFPDD_P30_LP_MODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p31_lp_mode            , cpld, QSFPDD_P31_LP_MODE);

static SENSOR_DEVICE_ATTR_RO(qsfpdd_p16_stuck              , cpld, QSFPDD_P16_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p17_stuck              , cpld, QSFPDD_P17_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p18_stuck              , cpld, QSFPDD_P18_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p19_stuck              , cpld, QSFPDD_P19_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p20_stuck              , cpld, QSFPDD_P20_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p21_stuck              , cpld, QSFPDD_P21_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p22_stuck              , cpld, QSFPDD_P22_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p23_stuck              , cpld, QSFPDD_P23_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p24_stuck              , cpld, QSFPDD_P24_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p25_stuck              , cpld, QSFPDD_P25_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p26_stuck              , cpld, QSFPDD_P26_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p27_stuck              , cpld, QSFPDD_P27_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p28_stuck              , cpld, QSFPDD_P28_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p29_stuck              , cpld, QSFPDD_P29_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p30_stuck              , cpld, QSFPDD_P30_STUCK);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p31_stuck              , cpld, QSFPDD_P31_STUCK);

 // CPLD 4
static SENSOR_DEVICE_ATTR_RW(mgmt_p0_ts                    , cpld, MGMT_P0_TS);
static SENSOR_DEVICE_ATTR_RW(mgmt_p1_ts                    , cpld, MGMT_P1_TS);
static SENSOR_DEVICE_ATTR_RW(sfp_p32_ts                    , cpld, SFP_P32_TS);
static SENSOR_DEVICE_ATTR_RW(sfp_p33_ts                    , cpld, SFP_P33_TS);
static SENSOR_DEVICE_ATTR_RW(sfp_p34_ts                    , cpld, SFP_P34_TS);
static SENSOR_DEVICE_ATTR_RW(sfp_p35_ts                    , cpld, SFP_P35_TS);
static SENSOR_DEVICE_ATTR_RW(mgmt_p0_rs                    , cpld, MGMT_P0_RS);
static SENSOR_DEVICE_ATTR_RW(mgmt_p1_rs                    , cpld, MGMT_P1_RS);
static SENSOR_DEVICE_ATTR_RW(sfp_p32_rs                    , cpld, SFP_P32_RS);
static SENSOR_DEVICE_ATTR_RW(sfp_p33_rs                    , cpld, SFP_P33_RS);
static SENSOR_DEVICE_ATTR_RW(sfp_p34_rs                    , cpld, SFP_P34_RS);
static SENSOR_DEVICE_ATTR_RW(sfp_p35_rs                    , cpld, SFP_P35_RS);
static SENSOR_DEVICE_ATTR_RW(mgmt_p0_tx_dis                , cpld, MGMT_P0_TX_DIS);
static SENSOR_DEVICE_ATTR_RW(mgmt_p1_tx_dis                , cpld, MGMT_P1_TX_DIS);
static SENSOR_DEVICE_ATTR_RW(sfp_p32_tx_dis                , cpld, SFP_P32_TX_DIS);
static SENSOR_DEVICE_ATTR_RW(sfp_p33_tx_dis                , cpld, SFP_P33_TX_DIS);
static SENSOR_DEVICE_ATTR_RW(sfp_p34_tx_dis                , cpld, SFP_P34_TX_DIS);
static SENSOR_DEVICE_ATTR_RW(sfp_p35_tx_dis                , cpld, SFP_P35_TX_DIS);
static SENSOR_DEVICE_ATTR_RW(mgmt_p0_tx_flt                , cpld, MGMT_P0_TX_FLT);
static SENSOR_DEVICE_ATTR_RW(mgmt_p1_tx_flt                , cpld, MGMT_P1_TX_FLT);
static SENSOR_DEVICE_ATTR_RW(sfp_p32_tx_flt                , cpld, SFP_P32_TX_FLT);
static SENSOR_DEVICE_ATTR_RW(sfp_p33_tx_flt                , cpld, SFP_P33_TX_FLT);
static SENSOR_DEVICE_ATTR_RW(sfp_p34_tx_flt                , cpld, SFP_P34_TX_FLT);
static SENSOR_DEVICE_ATTR_RW(sfp_p35_tx_flt                , cpld, SFP_P35_TX_FLT);
static SENSOR_DEVICE_ATTR_RW(mgmt_p0_rx_los                , cpld, MGMT_P0_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(mgmt_p1_rx_los                , cpld, MGMT_P1_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp_p32_rx_los                , cpld, SFP_P32_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp_p33_rx_los                , cpld, SFP_P33_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp_p34_rx_los                , cpld, SFP_P34_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp_p35_rx_los                , cpld, SFP_P35_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(mgmt_p0_abs                   , cpld, MGMT_P0_ABS);
static SENSOR_DEVICE_ATTR_RW(mgmt_p1_abs                   , cpld, MGMT_P1_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp_p32_abs                   , cpld, SFP_P32_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp_p33_abs                   , cpld, SFP_P33_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp_p34_abs                   , cpld, SFP_P34_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp_p35_abs                   , cpld, SFP_P35_ABS);
static SENSOR_DEVICE_ATTR_RW(mgmt_p0_tx_flt_mask           , cpld, MGMT_P0_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(mgmt_p1_tx_flt_mask           , cpld, MGMT_P1_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(sfp_p32_tx_flt_mask           , cpld, SFP_P32_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(sfp_p33_tx_flt_mask           , cpld, SFP_P33_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(sfp_p34_tx_flt_mask           , cpld, SFP_P34_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(sfp_p35_tx_flt_mask           , cpld, SFP_P35_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(mgmt_p0_rx_los_mask           , cpld, MGMT_P0_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(mgmt_p1_rx_los_mask           , cpld, MGMT_P1_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(sfp_p32_rx_los_mask           , cpld, SFP_P32_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(sfp_p33_rx_los_mask           , cpld, SFP_P33_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(sfp_p34_rx_los_mask           , cpld, SFP_P34_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(sfp_p35_rx_los_mask           , cpld, SFP_P35_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(mgmt_p0_abs_mask              , cpld, MGMT_P0_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(mgmt_p1_abs_mask              , cpld, MGMT_P1_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(sfp_p32_abs_mask              , cpld, SFP_P32_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(sfp_p33_abs_mask              , cpld, SFP_P33_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(sfp_p34_abs_mask              , cpld, SFP_P34_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(sfp_p35_abs_mask              , cpld, SFP_P35_ABS_MASK);
static SENSOR_DEVICE_ATTR_RO(mgmt_p0_tx_flt_event          , cpld, MGMT_P0_TX_FLT_EVENT);
static SENSOR_DEVICE_ATTR_RO(mgmt_p1_tx_flt_event          , cpld, MGMT_P1_TX_FLT_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp_p32_tx_flt_event          , cpld, SFP_P32_TX_FLT_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp_p33_tx_flt_event          , cpld, SFP_P33_TX_FLT_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp_p34_tx_flt_event          , cpld, SFP_P34_TX_FLT_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp_p35_tx_flt_event          , cpld, SFP_P35_TX_FLT_EVENT);
static SENSOR_DEVICE_ATTR_RO(mgmt_p0_rx_los_event          , cpld, MGMT_P0_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(mgmt_p1_rx_los_event          , cpld, MGMT_P1_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp_p32_rx_los_event          , cpld, SFP_P32_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp_p33_rx_los_event          , cpld, SFP_P33_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp_p34_rx_los_event          , cpld, SFP_P34_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp_p35_rx_los_event          , cpld, SFP_P35_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(mgmt_p0_abs_event             , cpld, MGMT_P0_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(mgmt_p1_abs_event             , cpld, MGMT_P1_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp_p32_abs_event             , cpld, SFP_P32_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp_p33_abs_event             , cpld, SFP_P33_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp_p34_abs_event             , cpld, SFP_P34_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp_p35_abs_event             , cpld, SFP_P35_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(mgmt_sfp_tx_flt_event         , cpld, MGMT_SFP_TX_FLT_EVENT);
static SENSOR_DEVICE_ATTR_RO(mgmt_sfp_rx_los_event         , cpld, MGMT_SFP_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(mgmt_sfp_abs_event            , cpld, MGMT_SFP_ABS_EVENT);

//BSP DEBUG
static SENSOR_DEVICE_ATTR_RW(bsp_debug     , bsp_callback, BSP_DEBUG);

//MUX
static SENSOR_DEVICE_ATTR_RW(idle_state, idle_state, IDLE_STATE);

/* define support attributes of cpldx */

/* cpld 1 */
static struct attribute *cpld1_attributes_beta[] = {
    // CPLD Common
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(event_detect_ctrl),

    // CPLD 1
    _DEVICE_ATTR(board_sku_id),
    _DEVICE_ATTR(board_hw_id),
    _DEVICE_ATTR(board_deph_id),
    _DEVICE_ATTR(board_build_id),
    _DEVICE_ATTR(board_id_type),
    _DEVICE_ATTR(cpld_chip_type),
    _DEVICE_ATTR(mac_intr),
    _DEVICE_ATTR(cpld_25gphy_intr),
    _DEVICE_ATTR(cpld_fru_intr),
    _DEVICE_ATTR(ntm_intr),
    _DEVICE_ATTR(thermal_alert_1_intr),
    _DEVICE_ATTR(thermal_alert_2_intr),
    _DEVICE_ATTR(thermal_alert_3_intr),
    _DEVICE_ATTR(misc_intr),
    _DEVICE_ATTR(system_intr),

    _DEVICE_ATTR(cpld_2_intr),
    _DEVICE_ATTR(cpld_3_intr),
    _DEVICE_ATTR(cpld_4_intr),
    _DEVICE_ATTR(psu_0_intr),
    _DEVICE_ATTR(psu_1_intr),
    _DEVICE_ATTR(mac_hbm_temp_alert),
    _DEVICE_ATTR(mb_lm75_temp_alert),
    _DEVICE_ATTR(ext_lm75_temp_alert),
    _DEVICE_ATTR(phy_lm75_cpld5_intr),
    _DEVICE_ATTR(usb_ocp),
    _DEVICE_ATTR(fpga_intr),
    _DEVICE_ATTR(thermal_alert_intr),
    _DEVICE_ATTR(psu_cpld_intr),
    _DEVICE_ATTR(main_chip_intr),
    _DEVICE_ATTR(ptp_intr),
    _DEVICE_ATTR(mac_intr_mask),
    _DEVICE_ATTR(cpld_25gphy_intr_mask),
    _DEVICE_ATTR(cpld_fru_intr_mask),
    _DEVICE_ATTR(ntm_intr_mask),
    _DEVICE_ATTR(thermal_alert_1_intr_mask),
    _DEVICE_ATTR(thermal_alert_2_intr_mask),
    _DEVICE_ATTR(thermal_alert_3_intr_mask),
    _DEVICE_ATTR(misc_intr_mask),
    _DEVICE_ATTR(system_intr_mask),

    _DEVICE_ATTR(cpld_2_intr_mask),
    _DEVICE_ATTR(cpld_3_intr_mask),
    _DEVICE_ATTR(cpld_4_intr_mask),
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
    _DEVICE_ATTR(thermal_alert_intr_mask),
    _DEVICE_ATTR(psu_cpld_intr_mask),
    _DEVICE_ATTR(main_chip_intr_mask),
    _DEVICE_ATTR(ptp_intr_mask),

    _DEVICE_ATTR(mac_intr_event),
    _DEVICE_ATTR(cpld_25gphy_intr_event),
    _DEVICE_ATTR(cpld_fru_intr_event),
    _DEVICE_ATTR(ntm_intr_event),
    _DEVICE_ATTR(thermal_alert_1_intr_event),
    _DEVICE_ATTR(thermal_alert_2_intr_event),
    _DEVICE_ATTR(thermal_alert_3_intr_event),
    _DEVICE_ATTR(misc_intr_event),

    _DEVICE_ATTR(mac_temp_alert_event),
    _DEVICE_ATTR(temp_alert_event),

    _DEVICE_ATTR(mac_rst),
    _DEVICE_ATTR(bios_flash_rst),
    _DEVICE_ATTR(bmc_rst),
    _DEVICE_ATTR(bmc_pcie_rst),
    _DEVICE_ATTR(bmc_lpc_rst),
    _DEVICE_ATTR(bmc_wdt_rst),
    _DEVICE_ATTR(ntm_rst),
    _DEVICE_ATTR(usb_redriver_ext_rst),
    _DEVICE_ATTR(mac_qspi_flash_rst),
    _DEVICE_ATTR(cp2130_rst),
    _DEVICE_ATTR(usb_redriver_ntm_rst),
    _DEVICE_ATTR(usb_redriver_mb_rst),
    _DEVICE_ATTR(usb_curr_limiter_en),

    _DEVICE_ATTR(cpld_2_rst),
    _DEVICE_ATTR(cpld_3_rst),
    _DEVICE_ATTR(fpga_rst),

    _DEVICE_ATTR(i2c_mux_rst),
    _DEVICE_ATTR(i2c_mux_9548_1_rst),
    _DEVICE_ATTR(i2c_mux_9548_2_rst),
    _DEVICE_ATTR(i2c_mux_fan_rst),

    _DEVICE_ATTR(bcm82399_rst),
    _DEVICE_ATTR(cp2102_rst),
    _DEVICE_ATTR(rc32312_rst),
    _DEVICE_ATTR(i210_rst),
    _DEVICE_ATTR(ioexp_rst),
    _DEVICE_ATTR(fpga_pcie_rst),
    _DEVICE_ATTR(i2c_mux_oob_rst),
    _DEVICE_ATTR(i2c_mux_ts_rst),

    //_DEVICE_ATTR(psu_rst),

    _DEVICE_ATTR(psu_0_prsnt),
    _DEVICE_ATTR(psu_1_prsnt),
    _DEVICE_ATTR(psu_0_acin),
    _DEVICE_ATTR(psu_1_acin),
    _DEVICE_ATTR(psu_0_pg),
    _DEVICE_ATTR(psu_1_pg),

    _DEVICE_ATTR(cpu_mux_sel),
    _DEVICE_ATTR(psu_mux_sel),
    _DEVICE_ATTR(fpga_qspi_sel),
    _DEVICE_ATTR(bmc_uart_mux_sel),
    _DEVICE_ATTR(system_uart_mux_sel),
    _DEVICE_ATTR(fpga_ctrl),
    _DEVICE_ATTR(fpga_nconfig),
    _DEVICE_ATTR(fpga_config_done),
    _DEVICE_ATTR(cpld_system_led_sys),
    _DEVICE_ATTR(system_led_status),
    _DEVICE_ATTR(system_led_speed),
    _DEVICE_ATTR(system_led_blink),
    _DEVICE_ATTR(system_led_onoff),
    _DEVICE_ATTR(cpld_system_led_fan),
    _DEVICE_ATTR(fan_led_status),
    _DEVICE_ATTR(fan_led_speed),
    _DEVICE_ATTR(fan_led_blink),
    _DEVICE_ATTR(fan_led_onoff),
    _DEVICE_ATTR(cpld_system_led_psu_0),
    _DEVICE_ATTR(psu_0_led_status),
    _DEVICE_ATTR(psu_0_led_speed),
    _DEVICE_ATTR(psu_0_led_blink),
    _DEVICE_ATTR(psu_0_led_onoff),
    _DEVICE_ATTR(cpld_system_led_psu_1),
    _DEVICE_ATTR(psu_1_led_status),
    _DEVICE_ATTR(psu_1_led_speed),
    _DEVICE_ATTR(psu_1_led_blink),
    _DEVICE_ATTR(psu_1_led_onoff),
    _DEVICE_ATTR(cpld_system_led_sync),
    _DEVICE_ATTR(sync_led_status),
    _DEVICE_ATTR(sync_led_speed),
    _DEVICE_ATTR(sync_led_blink),
    _DEVICE_ATTR(sync_led_onoff),
    _DEVICE_ATTR(led_clear),
    _DEVICE_ATTR(mgmt_0_led_status),
    _DEVICE_ATTR(mgmt_0_led_speed),
    _DEVICE_ATTR(mgmt_0_led_blink),
    _DEVICE_ATTR(mgmt_0_led_onoff),
    _DEVICE_ATTR(mgmt_1_led_status),
    _DEVICE_ATTR(mgmt_1_led_speed),
    _DEVICE_ATTR(mgmt_1_led_blink),
    _DEVICE_ATTR(mgmt_1_led_onoff),
    _DEVICE_ATTR(mgmt_p0_led),
    _DEVICE_ATTR(mgmt_p1_led),
    _DEVICE_ATTR(bsp_debug),
    NULL
};

static struct attribute *cpld1_attributes_pvt[] = {
    // CPLD Common
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(event_detect_ctrl),

    // CPLD 1
    _DEVICE_ATTR(board_sku_id),
    _DEVICE_ATTR(board_hw_id),
    _DEVICE_ATTR(board_deph_id),
    _DEVICE_ATTR(board_build_id),
    _DEVICE_ATTR(board_id_type),
    _DEVICE_ATTR(cpld_chip_type),
    _DEVICE_ATTR(mac_intr),
    _DEVICE_ATTR(cpld_25gphy_intr),
    _DEVICE_ATTR(cpld_fru_intr),
    _DEVICE_ATTR(ntm_intr),
    _DEVICE_ATTR(thermal_alert_1_intr),
    _DEVICE_ATTR(thermal_alert_2_intr),
    _DEVICE_ATTR(thermal_alert_3_intr),
    _DEVICE_ATTR(misc_intr),
    _DEVICE_ATTR(system_intr),

    _DEVICE_ATTR(cpld_2_intr),
    _DEVICE_ATTR(cpld_3_intr),
    _DEVICE_ATTR(cpld_4_intr),
    _DEVICE_ATTR(psu_0_intr),
    _DEVICE_ATTR(psu_1_intr),
    _DEVICE_ATTR(mac_hbm_temp_alert),
    _DEVICE_ATTR(mb_lm75_temp_alert),
    _DEVICE_ATTR(ext_lm75_temp_alert),
    _DEVICE_ATTR(phy_lm75_cpld5_intr),
    _DEVICE_ATTR(usb_ocp),
    _DEVICE_ATTR(fpga_intr),
    _DEVICE_ATTR(thermal_alert_intr),
    _DEVICE_ATTR(psu_cpld_intr),
    _DEVICE_ATTR(main_chip_intr),
    _DEVICE_ATTR(ptp_intr),
    _DEVICE_ATTR(mac_intr_mask),
    _DEVICE_ATTR(cpld_25gphy_intr_mask),
    _DEVICE_ATTR(cpld_fru_intr_mask),
    _DEVICE_ATTR(ntm_intr_mask),
    _DEVICE_ATTR(thermal_alert_1_intr_mask),
    _DEVICE_ATTR(thermal_alert_2_intr_mask),
    _DEVICE_ATTR(thermal_alert_3_intr_mask),
    _DEVICE_ATTR(misc_intr_mask),
    _DEVICE_ATTR(system_intr_mask),

    _DEVICE_ATTR(cpld_2_intr_mask),
    _DEVICE_ATTR(cpld_3_intr_mask),
    _DEVICE_ATTR(cpld_4_intr_mask),
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
    _DEVICE_ATTR(thermal_alert_intr_mask),
    _DEVICE_ATTR(psu_cpld_intr_mask),
    _DEVICE_ATTR(main_chip_intr_mask),
    _DEVICE_ATTR(ptp_intr_mask),

    _DEVICE_ATTR(mac_intr_event),
    _DEVICE_ATTR(cpld_25gphy_intr_event),
    _DEVICE_ATTR(cpld_fru_intr_event),
    _DEVICE_ATTR(ntm_intr_event),
    _DEVICE_ATTR(thermal_alert_1_intr_event),
    _DEVICE_ATTR(thermal_alert_2_intr_event),
    _DEVICE_ATTR(thermal_alert_3_intr_event),
    _DEVICE_ATTR(misc_intr_event),

    _DEVICE_ATTR(mac_temp_alert_event),
    _DEVICE_ATTR(temp_alert_event),

    _DEVICE_ATTR(mac_rst),
    _DEVICE_ATTR(bios_flash_rst),
    _DEVICE_ATTR(bmc_rst),
    _DEVICE_ATTR(bmc_pcie_rst),
    _DEVICE_ATTR(bmc_lpc_rst),
    _DEVICE_ATTR(bmc_wdt_rst),
    _DEVICE_ATTR(ntm_rst),
    _DEVICE_ATTR(usb_redriver_ext_rst),
    _DEVICE_ATTR(mac_qspi_flash_rst),
    _DEVICE_ATTR(cp2130_rst),
    _DEVICE_ATTR(usb_redriver_ntm_rst),
    _DEVICE_ATTR(usb_redriver_mb_rst),
    _DEVICE_ATTR(usb_curr_limiter_en),

    _DEVICE_ATTR(cpld_2_rst),
    _DEVICE_ATTR(cpld_3_rst),
    _DEVICE_ATTR(fpga_rst),

    _DEVICE_ATTR(i2c_mux_rst),
    _DEVICE_ATTR(i2c_mux_9548_1_rst),
    _DEVICE_ATTR(i2c_mux_9548_2_rst),
    _DEVICE_ATTR(i2c_mux_fan_rst),

    _DEVICE_ATTR(bcm82399_rst),
    _DEVICE_ATTR(cp2102_rst),
    _DEVICE_ATTR(rc32312_rst),
    _DEVICE_ATTR(i210_rst),
    _DEVICE_ATTR(ioexp_rst),
    _DEVICE_ATTR(fpga_pcie_rst),
    _DEVICE_ATTR(i2c_mux_oob_rst),
    _DEVICE_ATTR(i2c_mux_ts_rst),

    //_DEVICE_ATTR(psu_rst),

    _DEVICE_ATTR(psu_0_prsnt),
    _DEVICE_ATTR(psu_1_prsnt),
    _DEVICE_ATTR(psu_0_acin),
    _DEVICE_ATTR(psu_1_acin),
    _DEVICE_ATTR(psu_0_pg),
    _DEVICE_ATTR(psu_1_pg),

    _DEVICE_ATTR(cpu_mux_sel),
    _DEVICE_ATTR(psu_mux_sel),
    _DEVICE_ATTR(fpga_qspi_sel),
    _DEVICE_ATTR(bmc_uart_mux_sel),
    _DEVICE_ATTR(system_uart_mux_sel),
    _DEVICE_ATTR(fpga_ctrl),
    _DEVICE_ATTR(fpga_nconfig),
    _DEVICE_ATTR(fpga_config_done),
    _DEVICE_ATTR(cpld_system_led_sys),
    _DEVICE_ATTR(system_led_status),
    _DEVICE_ATTR(system_led_speed),
    _DEVICE_ATTR(system_led_blink),
    _DEVICE_ATTR(system_led_onoff),
    _DEVICE_ATTR(cpld_system_led_fan),
    _DEVICE_ATTR(fan_led_status),
    _DEVICE_ATTR(fan_led_speed),
    _DEVICE_ATTR(fan_led_blink),
    _DEVICE_ATTR(fan_led_onoff),
    _DEVICE_ATTR(cpld_system_led_psu_0),
    _DEVICE_ATTR(psu_0_led_status),
    _DEVICE_ATTR(psu_0_led_speed),
    _DEVICE_ATTR(psu_0_led_blink),
    _DEVICE_ATTR(psu_0_led_onoff),
    _DEVICE_ATTR(cpld_system_led_psu_1),
    _DEVICE_ATTR(psu_1_led_status),
    _DEVICE_ATTR(psu_1_led_speed),
    _DEVICE_ATTR(psu_1_led_blink),
    _DEVICE_ATTR(psu_1_led_onoff),
    _DEVICE_ATTR(cpld_system_led_sync),
    _DEVICE_ATTR(sync_led_status),
    _DEVICE_ATTR(sync_led_speed),
    _DEVICE_ATTR(sync_led_blink),
    _DEVICE_ATTR(sync_led_onoff),
    _DEVICE_ATTR(cpld_system_led_id),
    _DEVICE_ATTR(id_led_status),
    _DEVICE_ATTR(id_led_speed),
    _DEVICE_ATTR(id_led_blink),
    _DEVICE_ATTR(id_led_onoff),
    _DEVICE_ATTR(led_clear),
    _DEVICE_ATTR(mgmt_0_led_status),
    _DEVICE_ATTR(mgmt_0_led_speed),
    _DEVICE_ATTR(mgmt_0_led_blink),
    _DEVICE_ATTR(mgmt_0_led_onoff),
    _DEVICE_ATTR(mgmt_1_led_status),
    _DEVICE_ATTR(mgmt_1_led_speed),
    _DEVICE_ATTR(mgmt_1_led_blink),
    _DEVICE_ATTR(mgmt_1_led_onoff),
    _DEVICE_ATTR(mgmt_p0_led),
    _DEVICE_ATTR(mgmt_p1_led),
    _DEVICE_ATTR(bsp_debug),
    NULL
};

/* cpld 2 */
static struct attribute *cpld2_attributes[] = {
    // CPLD Common
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(event_detect_ctrl),

    _DEVICE_ATTR(cpld_i2c_stuck),
    _DEVICE_ATTR(cpld_i2c_control),
    _DEVICE_ATTR(cpld_i2c_idle),
    _DEVICE_ATTR(cpld_i2c_relay),

    // CPLD 2
    _DEVICE_ATTR(qsfpdd_p0_intr),
    _DEVICE_ATTR(qsfpdd_p1_intr),
    _DEVICE_ATTR(qsfpdd_p2_intr),
    _DEVICE_ATTR(qsfpdd_p3_intr),
    _DEVICE_ATTR(qsfpdd_p4_intr),
    _DEVICE_ATTR(qsfpdd_p5_intr),
    _DEVICE_ATTR(qsfpdd_p6_intr),
    _DEVICE_ATTR(qsfpdd_p7_intr),
    _DEVICE_ATTR(qsfpdd_p8_intr),
    _DEVICE_ATTR(qsfpdd_p9_intr),
    _DEVICE_ATTR(qsfpdd_p10_intr),
    _DEVICE_ATTR(qsfpdd_p11_intr),
    _DEVICE_ATTR(qsfpdd_p12_intr),
    _DEVICE_ATTR(qsfpdd_p13_intr),
    _DEVICE_ATTR(qsfpdd_p14_intr),
    _DEVICE_ATTR(qsfpdd_p15_intr),

    _DEVICE_ATTR(qsfpdd_p0_abs),
    _DEVICE_ATTR(qsfpdd_p1_abs),
    _DEVICE_ATTR(qsfpdd_p2_abs),
    _DEVICE_ATTR(qsfpdd_p3_abs),
    _DEVICE_ATTR(qsfpdd_p4_abs),
    _DEVICE_ATTR(qsfpdd_p5_abs),
    _DEVICE_ATTR(qsfpdd_p6_abs),
    _DEVICE_ATTR(qsfpdd_p7_abs),
    _DEVICE_ATTR(qsfpdd_p8_abs),
    _DEVICE_ATTR(qsfpdd_p9_abs),
    _DEVICE_ATTR(qsfpdd_p10_abs),
    _DEVICE_ATTR(qsfpdd_p11_abs),
    _DEVICE_ATTR(qsfpdd_p12_abs),
    _DEVICE_ATTR(qsfpdd_p13_abs),
    _DEVICE_ATTR(qsfpdd_p14_abs),
    _DEVICE_ATTR(qsfpdd_p15_abs),

    _DEVICE_ATTR(qsfpdd_p0_7_abs_event),
    _DEVICE_ATTR(qsfpdd_p8_15_abs_event),

    _DEVICE_ATTR(qsfpdd_p0_7_intr_event),
    _DEVICE_ATTR(qsfpdd_p8_15_intr_event),

    _DEVICE_ATTR(qsfpdd_p0_7_fuse_intr_event),
    _DEVICE_ATTR(qsfpdd_p8_15_fuse_intr_event),

    _DEVICE_ATTR(qsfpdd_p0_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p1_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p2_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p3_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p4_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p5_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p6_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p7_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p8_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p9_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p10_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p11_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p12_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p13_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p14_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p15_fuse_intr),

    _DEVICE_ATTR(qsfpdd_p0_intr_mask),
    _DEVICE_ATTR(qsfpdd_p1_intr_mask),
    _DEVICE_ATTR(qsfpdd_p2_intr_mask),
    _DEVICE_ATTR(qsfpdd_p3_intr_mask),
    _DEVICE_ATTR(qsfpdd_p4_intr_mask),
    _DEVICE_ATTR(qsfpdd_p5_intr_mask),
    _DEVICE_ATTR(qsfpdd_p6_intr_mask),
    _DEVICE_ATTR(qsfpdd_p7_intr_mask),
    _DEVICE_ATTR(qsfpdd_p8_intr_mask),
    _DEVICE_ATTR(qsfpdd_p9_intr_mask),
    _DEVICE_ATTR(qsfpdd_p10_intr_mask),
    _DEVICE_ATTR(qsfpdd_p11_intr_mask),
    _DEVICE_ATTR(qsfpdd_p12_intr_mask),
    _DEVICE_ATTR(qsfpdd_p13_intr_mask),
    _DEVICE_ATTR(qsfpdd_p14_intr_mask),
    _DEVICE_ATTR(qsfpdd_p15_intr_mask),

    _DEVICE_ATTR(qsfpdd_p0_abs_mask),
    _DEVICE_ATTR(qsfpdd_p1_abs_mask),
    _DEVICE_ATTR(qsfpdd_p2_abs_mask),
    _DEVICE_ATTR(qsfpdd_p3_abs_mask),
    _DEVICE_ATTR(qsfpdd_p4_abs_mask),
    _DEVICE_ATTR(qsfpdd_p5_abs_mask),
    _DEVICE_ATTR(qsfpdd_p6_abs_mask),
    _DEVICE_ATTR(qsfpdd_p7_abs_mask),
    _DEVICE_ATTR(qsfpdd_p8_abs_mask),
    _DEVICE_ATTR(qsfpdd_p9_abs_mask),
    _DEVICE_ATTR(qsfpdd_p10_abs_mask),
    _DEVICE_ATTR(qsfpdd_p11_abs_mask),
    _DEVICE_ATTR(qsfpdd_p12_abs_mask),
    _DEVICE_ATTR(qsfpdd_p13_abs_mask),
    _DEVICE_ATTR(qsfpdd_p14_abs_mask),
    _DEVICE_ATTR(qsfpdd_p15_abs_mask),

    _DEVICE_ATTR(qsfpdd_p0_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p1_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p2_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p3_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p4_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p5_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p6_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p7_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p8_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p9_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p10_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p11_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p12_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p13_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p14_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p15_fuse_intr_mask),

    _DEVICE_ATTR(qsfpdd_p0_rst),
    _DEVICE_ATTR(qsfpdd_p1_rst),
    _DEVICE_ATTR(qsfpdd_p2_rst),
    _DEVICE_ATTR(qsfpdd_p3_rst),
    _DEVICE_ATTR(qsfpdd_p4_rst),
    _DEVICE_ATTR(qsfpdd_p5_rst),
    _DEVICE_ATTR(qsfpdd_p6_rst),
    _DEVICE_ATTR(qsfpdd_p7_rst),
    _DEVICE_ATTR(qsfpdd_p8_rst),
    _DEVICE_ATTR(qsfpdd_p9_rst),
    _DEVICE_ATTR(qsfpdd_p10_rst),
    _DEVICE_ATTR(qsfpdd_p11_rst),
    _DEVICE_ATTR(qsfpdd_p12_rst),
    _DEVICE_ATTR(qsfpdd_p13_rst),
    _DEVICE_ATTR(qsfpdd_p14_rst),
    _DEVICE_ATTR(qsfpdd_p15_rst),

    _DEVICE_ATTR(qsfpdd_p0_lp_mode),
    _DEVICE_ATTR(qsfpdd_p1_lp_mode),
    _DEVICE_ATTR(qsfpdd_p2_lp_mode),
    _DEVICE_ATTR(qsfpdd_p3_lp_mode),
    _DEVICE_ATTR(qsfpdd_p4_lp_mode),
    _DEVICE_ATTR(qsfpdd_p5_lp_mode),
    _DEVICE_ATTR(qsfpdd_p6_lp_mode),
    _DEVICE_ATTR(qsfpdd_p7_lp_mode),
    _DEVICE_ATTR(qsfpdd_p8_lp_mode),
    _DEVICE_ATTR(qsfpdd_p9_lp_mode),
    _DEVICE_ATTR(qsfpdd_p10_lp_mode),
    _DEVICE_ATTR(qsfpdd_p11_lp_mode),
    _DEVICE_ATTR(qsfpdd_p12_lp_mode),
    _DEVICE_ATTR(qsfpdd_p13_lp_mode),
    _DEVICE_ATTR(qsfpdd_p14_lp_mode),
    _DEVICE_ATTR(qsfpdd_p15_lp_mode),

    _DEVICE_ATTR(qsfpdd_p0_stuck),
    _DEVICE_ATTR(qsfpdd_p1_stuck),
    _DEVICE_ATTR(qsfpdd_p2_stuck),
    _DEVICE_ATTR(qsfpdd_p3_stuck),
    _DEVICE_ATTR(qsfpdd_p4_stuck),
    _DEVICE_ATTR(qsfpdd_p5_stuck),
    _DEVICE_ATTR(qsfpdd_p6_stuck),
    _DEVICE_ATTR(qsfpdd_p7_stuck),
    _DEVICE_ATTR(qsfpdd_p8_stuck),
    _DEVICE_ATTR(qsfpdd_p9_stuck),
    _DEVICE_ATTR(qsfpdd_p10_stuck),
    _DEVICE_ATTR(qsfpdd_p11_stuck),
    _DEVICE_ATTR(qsfpdd_p12_stuck),
    _DEVICE_ATTR(qsfpdd_p13_stuck),
    _DEVICE_ATTR(qsfpdd_p14_stuck),
    _DEVICE_ATTR(qsfpdd_p15_stuck),
    NULL
};

/* cpld 3 */
static struct attribute *cpld3_attributes[] = {
    // CPLD Common
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(event_detect_ctrl),

    _DEVICE_ATTR(cpld_i2c_stuck),
    _DEVICE_ATTR(cpld_i2c_control),
    _DEVICE_ATTR(cpld_i2c_idle),
    _DEVICE_ATTR(cpld_i2c_relay),

    // CPLD 3
    _DEVICE_ATTR(qsfpdd_p16_intr),
    _DEVICE_ATTR(qsfpdd_p17_intr),
    _DEVICE_ATTR(qsfpdd_p18_intr),
    _DEVICE_ATTR(qsfpdd_p19_intr),
    _DEVICE_ATTR(qsfpdd_p20_intr),
    _DEVICE_ATTR(qsfpdd_p21_intr),
    _DEVICE_ATTR(qsfpdd_p22_intr),
    _DEVICE_ATTR(qsfpdd_p23_intr),
    _DEVICE_ATTR(qsfpdd_p24_intr),
    _DEVICE_ATTR(qsfpdd_p25_intr),
    _DEVICE_ATTR(qsfpdd_p26_intr),
    _DEVICE_ATTR(qsfpdd_p27_intr),
    _DEVICE_ATTR(qsfpdd_p28_intr),
    _DEVICE_ATTR(qsfpdd_p29_intr),
    _DEVICE_ATTR(qsfpdd_p30_intr),
    _DEVICE_ATTR(qsfpdd_p31_intr),

    _DEVICE_ATTR(qsfpdd_p16_abs),
    _DEVICE_ATTR(qsfpdd_p17_abs),
    _DEVICE_ATTR(qsfpdd_p18_abs),
    _DEVICE_ATTR(qsfpdd_p19_abs),
    _DEVICE_ATTR(qsfpdd_p20_abs),
    _DEVICE_ATTR(qsfpdd_p21_abs),
    _DEVICE_ATTR(qsfpdd_p22_abs),
    _DEVICE_ATTR(qsfpdd_p23_abs),
    _DEVICE_ATTR(qsfpdd_p24_abs),
    _DEVICE_ATTR(qsfpdd_p25_abs),
    _DEVICE_ATTR(qsfpdd_p26_abs),
    _DEVICE_ATTR(qsfpdd_p27_abs),
    _DEVICE_ATTR(qsfpdd_p28_abs),
    _DEVICE_ATTR(qsfpdd_p29_abs),
    _DEVICE_ATTR(qsfpdd_p30_abs),
    _DEVICE_ATTR(qsfpdd_p31_abs),

    _DEVICE_ATTR(qsfpdd_p16_23_abs_event),
    _DEVICE_ATTR(qsfpdd_p24_31_abs_event),

    _DEVICE_ATTR(qsfpdd_p16_23_intr_event),
    _DEVICE_ATTR(qsfpdd_p24_31_intr_event),

    _DEVICE_ATTR(qsfpdd_p16_23_fuse_intr_event),
    _DEVICE_ATTR(qsfpdd_p24_31_fuse_intr_event),

    _DEVICE_ATTR(qsfpdd_p16_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p17_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p18_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p19_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p20_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p21_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p22_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p23_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p24_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p25_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p26_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p27_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p28_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p29_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p30_fuse_intr),
    _DEVICE_ATTR(qsfpdd_p31_fuse_intr),

    _DEVICE_ATTR(qsfpdd_p16_intr_mask),
    _DEVICE_ATTR(qsfpdd_p17_intr_mask),
    _DEVICE_ATTR(qsfpdd_p18_intr_mask),
    _DEVICE_ATTR(qsfpdd_p19_intr_mask),
    _DEVICE_ATTR(qsfpdd_p20_intr_mask),
    _DEVICE_ATTR(qsfpdd_p21_intr_mask),
    _DEVICE_ATTR(qsfpdd_p22_intr_mask),
    _DEVICE_ATTR(qsfpdd_p23_intr_mask),
    _DEVICE_ATTR(qsfpdd_p24_intr_mask),
    _DEVICE_ATTR(qsfpdd_p25_intr_mask),
    _DEVICE_ATTR(qsfpdd_p26_intr_mask),
    _DEVICE_ATTR(qsfpdd_p27_intr_mask),
    _DEVICE_ATTR(qsfpdd_p28_intr_mask),
    _DEVICE_ATTR(qsfpdd_p29_intr_mask),
    _DEVICE_ATTR(qsfpdd_p30_intr_mask),
    _DEVICE_ATTR(qsfpdd_p31_intr_mask),

    _DEVICE_ATTR(qsfpdd_p16_abs_mask),
    _DEVICE_ATTR(qsfpdd_p17_abs_mask),
    _DEVICE_ATTR(qsfpdd_p18_abs_mask),
    _DEVICE_ATTR(qsfpdd_p19_abs_mask),
    _DEVICE_ATTR(qsfpdd_p20_abs_mask),
    _DEVICE_ATTR(qsfpdd_p21_abs_mask),
    _DEVICE_ATTR(qsfpdd_p22_abs_mask),
    _DEVICE_ATTR(qsfpdd_p23_abs_mask),
    _DEVICE_ATTR(qsfpdd_p24_abs_mask),
    _DEVICE_ATTR(qsfpdd_p25_abs_mask),
    _DEVICE_ATTR(qsfpdd_p26_abs_mask),
    _DEVICE_ATTR(qsfpdd_p27_abs_mask),
    _DEVICE_ATTR(qsfpdd_p28_abs_mask),
    _DEVICE_ATTR(qsfpdd_p29_abs_mask),
    _DEVICE_ATTR(qsfpdd_p30_abs_mask),
    _DEVICE_ATTR(qsfpdd_p31_abs_mask),

    _DEVICE_ATTR(qsfpdd_p16_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p17_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p18_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p19_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p20_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p21_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p22_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p23_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p24_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p25_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p26_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p27_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p28_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p29_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p30_fuse_intr_mask),
    _DEVICE_ATTR(qsfpdd_p31_fuse_intr_mask),

    _DEVICE_ATTR(qsfpdd_p16_rst),
    _DEVICE_ATTR(qsfpdd_p17_rst),
    _DEVICE_ATTR(qsfpdd_p18_rst),
    _DEVICE_ATTR(qsfpdd_p19_rst),
    _DEVICE_ATTR(qsfpdd_p20_rst),
    _DEVICE_ATTR(qsfpdd_p21_rst),
    _DEVICE_ATTR(qsfpdd_p22_rst),
    _DEVICE_ATTR(qsfpdd_p23_rst),
    _DEVICE_ATTR(qsfpdd_p24_rst),
    _DEVICE_ATTR(qsfpdd_p25_rst),
    _DEVICE_ATTR(qsfpdd_p26_rst),
    _DEVICE_ATTR(qsfpdd_p27_rst),
    _DEVICE_ATTR(qsfpdd_p28_rst),
    _DEVICE_ATTR(qsfpdd_p29_rst),
    _DEVICE_ATTR(qsfpdd_p30_rst),
    _DEVICE_ATTR(qsfpdd_p31_rst),

    _DEVICE_ATTR(qsfpdd_p16_lp_mode),
    _DEVICE_ATTR(qsfpdd_p17_lp_mode),
    _DEVICE_ATTR(qsfpdd_p18_lp_mode),
    _DEVICE_ATTR(qsfpdd_p19_lp_mode),
    _DEVICE_ATTR(qsfpdd_p20_lp_mode),
    _DEVICE_ATTR(qsfpdd_p21_lp_mode),
    _DEVICE_ATTR(qsfpdd_p22_lp_mode),
    _DEVICE_ATTR(qsfpdd_p23_lp_mode),
    _DEVICE_ATTR(qsfpdd_p24_lp_mode),
    _DEVICE_ATTR(qsfpdd_p25_lp_mode),
    _DEVICE_ATTR(qsfpdd_p26_lp_mode),
    _DEVICE_ATTR(qsfpdd_p27_lp_mode),
    _DEVICE_ATTR(qsfpdd_p28_lp_mode),
    _DEVICE_ATTR(qsfpdd_p29_lp_mode),
    _DEVICE_ATTR(qsfpdd_p30_lp_mode),
    _DEVICE_ATTR(qsfpdd_p31_lp_mode),

    _DEVICE_ATTR(qsfpdd_p16_stuck),
    _DEVICE_ATTR(qsfpdd_p17_stuck),
    _DEVICE_ATTR(qsfpdd_p18_stuck),
    _DEVICE_ATTR(qsfpdd_p19_stuck),
    _DEVICE_ATTR(qsfpdd_p20_stuck),
    _DEVICE_ATTR(qsfpdd_p21_stuck),
    _DEVICE_ATTR(qsfpdd_p22_stuck),
    _DEVICE_ATTR(qsfpdd_p23_stuck),
    _DEVICE_ATTR(qsfpdd_p24_stuck),
    _DEVICE_ATTR(qsfpdd_p25_stuck),
    _DEVICE_ATTR(qsfpdd_p26_stuck),
    _DEVICE_ATTR(qsfpdd_p27_stuck),
    _DEVICE_ATTR(qsfpdd_p28_stuck),
    _DEVICE_ATTR(qsfpdd_p29_stuck),
    _DEVICE_ATTR(qsfpdd_p30_stuck),
    _DEVICE_ATTR(qsfpdd_p31_stuck),

    NULL
};

/* cpld 4 */
static struct attribute *cpld4_attributes[] = {
    // CPLD Common
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(event_detect_ctrl),

    _DEVICE_ATTR(cpld_i2c_stuck),
    _DEVICE_ATTR(cpld_i2c_control),
    _DEVICE_ATTR(cpld_i2c_idle),
    _DEVICE_ATTR(cpld_i2c_relay),

    _DEVICE_ATTR(mgmt_p0_ts),
    _DEVICE_ATTR(mgmt_p1_ts),
    _DEVICE_ATTR(sfp_p32_ts),
    _DEVICE_ATTR(sfp_p33_ts),
    _DEVICE_ATTR(sfp_p34_ts),
    _DEVICE_ATTR(sfp_p35_ts),
    _DEVICE_ATTR(mgmt_p0_rs),
    _DEVICE_ATTR(mgmt_p1_rs),
    _DEVICE_ATTR(sfp_p32_rs),
    _DEVICE_ATTR(sfp_p33_rs),
    _DEVICE_ATTR(sfp_p34_rs),
    _DEVICE_ATTR(sfp_p35_rs),
    _DEVICE_ATTR(mgmt_p0_tx_dis),
    _DEVICE_ATTR(mgmt_p1_tx_dis),
    _DEVICE_ATTR(sfp_p32_tx_dis),
    _DEVICE_ATTR(sfp_p33_tx_dis),
    _DEVICE_ATTR(sfp_p34_tx_dis),
    _DEVICE_ATTR(sfp_p35_tx_dis),
    _DEVICE_ATTR(mgmt_p0_tx_flt),
    _DEVICE_ATTR(mgmt_p1_tx_flt),
    _DEVICE_ATTR(sfp_p32_tx_flt),
    _DEVICE_ATTR(sfp_p33_tx_flt),
    _DEVICE_ATTR(sfp_p34_tx_flt),
    _DEVICE_ATTR(sfp_p35_tx_flt),
    _DEVICE_ATTR(mgmt_p0_rx_los),
    _DEVICE_ATTR(mgmt_p1_rx_los),
    _DEVICE_ATTR(sfp_p32_rx_los),
    _DEVICE_ATTR(sfp_p33_rx_los),
    _DEVICE_ATTR(sfp_p34_rx_los),
    _DEVICE_ATTR(sfp_p35_rx_los),
    _DEVICE_ATTR(mgmt_p0_abs),
    _DEVICE_ATTR(mgmt_p1_abs),
    _DEVICE_ATTR(sfp_p32_abs),
    _DEVICE_ATTR(sfp_p33_abs),
    _DEVICE_ATTR(sfp_p34_abs),
    _DEVICE_ATTR(sfp_p35_abs),
    _DEVICE_ATTR(mgmt_p0_tx_flt_mask),
    _DEVICE_ATTR(mgmt_p1_tx_flt_mask),
    _DEVICE_ATTR(sfp_p32_tx_flt_mask),
    _DEVICE_ATTR(sfp_p33_tx_flt_mask),
    _DEVICE_ATTR(sfp_p34_tx_flt_mask),
    _DEVICE_ATTR(sfp_p35_tx_flt_mask),
    _DEVICE_ATTR(mgmt_p0_rx_los_mask),
    _DEVICE_ATTR(mgmt_p1_rx_los_mask),
    _DEVICE_ATTR(sfp_p32_rx_los_mask),
    _DEVICE_ATTR(sfp_p33_rx_los_mask),
    _DEVICE_ATTR(sfp_p34_rx_los_mask),
    _DEVICE_ATTR(sfp_p35_rx_los_mask),
    _DEVICE_ATTR(mgmt_p0_abs_mask),
    _DEVICE_ATTR(mgmt_p1_abs_mask),
    _DEVICE_ATTR(sfp_p32_abs_mask),
    _DEVICE_ATTR(sfp_p33_abs_mask),
    _DEVICE_ATTR(sfp_p34_abs_mask),
    _DEVICE_ATTR(sfp_p35_abs_mask),
    _DEVICE_ATTR(mgmt_p0_tx_flt_event),
    _DEVICE_ATTR(mgmt_p1_tx_flt_event),
    _DEVICE_ATTR(sfp_p32_tx_flt_event),
    _DEVICE_ATTR(sfp_p33_tx_flt_event),
    _DEVICE_ATTR(sfp_p34_tx_flt_event),
    _DEVICE_ATTR(sfp_p35_tx_flt_event),
    _DEVICE_ATTR(mgmt_sfp_tx_flt_event),
    _DEVICE_ATTR(mgmt_p0_rx_los_event),
    _DEVICE_ATTR(mgmt_p1_rx_los_event),
    _DEVICE_ATTR(sfp_p32_rx_los_event),
    _DEVICE_ATTR(sfp_p33_rx_los_event),
    _DEVICE_ATTR(sfp_p34_rx_los_event),
    _DEVICE_ATTR(sfp_p35_rx_los_event),
    _DEVICE_ATTR(mgmt_sfp_rx_los_event),
    _DEVICE_ATTR(mgmt_p0_abs_event),
    _DEVICE_ATTR(mgmt_p1_abs_event),
    _DEVICE_ATTR(sfp_p32_abs_event),
    _DEVICE_ATTR(sfp_p33_abs_event),
    _DEVICE_ATTR(sfp_p34_abs_event),
    _DEVICE_ATTR(sfp_p35_abs_event),
    _DEVICE_ATTR(mgmt_sfp_abs_event),
    NULL
};

// FPGA
static struct attribute *fpga_attributes[] = {
    _DEVICE_ATTR(fpga_major_ver),
    _DEVICE_ATTR(fpga_minor_ver),
    _DEVICE_ATTR(fpga_build_ver),
    _DEVICE_ATTR(fpga_version_h),
    _DEVICE_ATTR(fpga_id),
    _DEVICE_ATTR(fpga_mac_rov_1),
    _DEVICE_ATTR(fpga_mac_rov_2),
    _DEVICE_ATTR(event_detect_ctrl),
    NULL
};

/* cpld 1 attributes group */
static const struct attribute_group cpld1_group_beta = {
    .attrs = cpld1_attributes_beta,
};

static const struct attribute_group cpld1_group_pvt = {
    .attrs = cpld1_attributes_pvt,
};

/* cpld 2 attributes group */
static const struct attribute_group cpld2_group = {
    .attrs = cpld2_attributes,
};

/* fpga attributes group */
static const struct attribute_group fpga_group = {
    .attrs = fpga_attributes,
};

/* cpld 3 attributes group */
static const struct attribute_group cpld3_group = {
    .attrs = cpld3_attributes,
};

/* cpld 4 attributes group */
static const struct attribute_group cpld4_group = {
    .attrs = cpld4_attributes,
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

static int _store_value_check(int index, u8 reg_val, char **range) {
    int ret = 0;
    if(range == NULL) {
        return -2;
    }

    switch (index) {
        case MGMT_0_LED_SPEED:
        case MGMT_1_LED_SPEED:
            if(reg_val != 0 && reg_val != 1) {
                ret = -1;
            }
            *range = "0 or 1";
            break;
        default:
            break;
    }

    return ret;
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
        case CPLD_MAJOR_VER:
        case CPLD_MINOR_VER:
        case CPLD_ID:
        case CPLD_BUILD_VER:
        case EVENT_DETECT_CTRL:

        //CPLD 2/3/4
        case CPLD_I2C_STUCK:
        case CPLD_I2C_CONTROL:
        case CPLD_I2C_IDLE:
        case CPLD_I2C_RELAY:

        //CPLD 1
        case BRD_SKU_ID:
        case BRD_HW_ID:
        case BRD_DEPH_ID:
        case BRD_BUILD_ID:
        case BRD_ID_TYPE:
        case CPLD_CHIP_TYPE:

        case MAC_INTR:
        case CPLD_25GPHY_INTR:
        case CPLD_FRU_INTR:
        case NTM_INTR:
        case THERMAL_ALERT_1_INTR:
        case THERMAL_ALERT_2_INTR:
        case THERMAL_ALERT_3_INTR:
        case MISC_INTR:
        case SYSTEM_INTR:

        case CPLD_2_INTR:
        case CPLD_3_INTR:
        case CPLD_4_INTR:
        case PSU_0_INTR:
        case PSU_1_INTR:

        case MAC_HBM_TEMP_ALERT:
        case MB_LM75_TEMP_ALERT:
        case EXT_LM75_TEMP_ALERT:
        case PHY_LM75_CPLD5_INTR:
        case USB_OCP:
        case FPGA_INTR:
        case THERMAL_ALERT_INTR:
        case PSU_CPLD_INTR:
        case MAIN_CHIP_INTR:
        case PTP_INTR:

        case MAC_INTR_MASK:
        case CPLD_25GPHY_INTR_MASK:
        case CPLD_FRU_INTR_MASK:
        case NTM_INTR_MASK:
        case THERMAL_ALERT_1_INTR_MASK:
        case THERMAL_ALERT_2_INTR_MASK:
        case THERMAL_ALERT_3_INTR_MASK:
        case MISC_INTR_MASK:
        case SYSTEM_INTR_MASK:

        case CPLD_2_INTR_MASK:
        case CPLD_3_INTR_MASK:
        case CPLD_4_INTR_MASK:
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
        case THERMAL_ALERT_INTR_MASK:
        case PSU_CPLD_INTR_MASK:
        case MAIN_CHIP_INTR_MASK:
        case PTP_INTR_MASK:

        case MAC_INTR_EVENT:
        case CPLD_25GPHY_INTR_EVENT:
        case CPLD_FRU_INTR_EVENT:
        case NTM_INTR_EVENT:
        case THERMAL_ALERT_1_INTR_EVENT:
        case THERMAL_ALERT_2_INTR_EVENT:
        case THERMAL_ALERT_3_INTR_EVENT:
        case MISC_INTR_EVENT:

        case MAC_TEMP_ALERT_EVENT:
        case TEMP_ALERT_EVENT:

        case MAC_RST:
        case BIOS_FLASH_RST:
        case BMC_RST:
        case BMC_PCIE_RST:
        case BMC_LPC_RST:
        case BMC_WDT_RST:
        case NTM_RST:

        case USB_REDRIVER_EXT_RST:
        case MAC_QSPI_FLASH_RST:
        case CP2130_RST:
        case USB_REDRIVER_NTM_RST:
        case USB_REDRIVER_MB_RST:
        case USB_CURR_LIMITER_EN:

        case CPLD_2_RST:
        case CPLD_3_RST:
        case FPGA_RST:

        case I2C_MUX_RST:
        case I2C_MUX_9548_1_RST:
        case I2C_MUX_9548_2_RST:
        case I2C_MUX_FAN_RST:

        case BCM82399_RST:
        case CP2102_RST:
        case RC32312_RST:
        case I210_RST:
        case IOEXP_RST:
        case FPGA_PCIE_RST:
        case I2C_MUX_OOB_RST:
        case I2C_MUX_TS_RST:

        case PSU_RST:

        case PSU_0_PRSNT:
        case PSU_1_PRSNT:
        case PSU_0_ACIN:
        case PSU_1_ACIN:
        case PSU_0_PG:
        case PSU_1_PG:

        case CPU_MUX_SEL:
        case PSU_MUX_SEL:
        case FPGA_QSPI_SEL:
        case BMC_UART_MUX_SEL:
        case SYSTEM_UART_MUX_SEL:
        case FPGA_CTRL:
        case FPGA_NCONFIG:
        case FPGA_CONFIG_DONE:

        case CPLD_SYSTEM_LED_SYS:
        case SYSTEM_LED_STATUS:
        case SYSTEM_LED_SPEED:
        case SYSTEM_LED_BLINK:
        case SYSTEM_LED_ONOFF:
        case CPLD_SYSTEM_LED_FAN:
        case FAN_LED_STATUS:
        case FAN_LED_SPEED:
        case FAN_LED_BLINK:
        case FAN_LED_ONOFF:

        case CPLD_SYSTEM_LED_PSU_0:
        case PSU_0_LED_STATUS:
        case PSU_0_LED_SPEED:
        case PSU_0_LED_BLINK:
        case PSU_0_LED_ONOFF:
        case CPLD_SYSTEM_LED_PSU_1:
        case PSU_1_LED_STATUS:
        case PSU_1_LED_SPEED:
        case PSU_1_LED_BLINK:
        case PSU_1_LED_ONOFF:

        case CPLD_SYSTEM_LED_SYNC:
        case SYNC_LED_STATUS:
        case SYNC_LED_SPEED:
        case SYNC_LED_BLINK:
        case SYNC_LED_ONOFF:
        case CPLD_SYSTEM_LED_ID:
        case ID_LED_STATUS:
        case ID_LED_SPEED:
        case ID_LED_BLINK:
        case ID_LED_ONOFF:

        case MGMT_0_LED_STATUS:
        case MGMT_0_LED_SPEED:
        case MGMT_0_LED_BLINK:
        case MGMT_0_LED_ONOFF:
        case MGMT_1_LED_STATUS:
        case MGMT_1_LED_SPEED:
        case MGMT_1_LED_BLINK:
        case MGMT_1_LED_ONOFF:

        case MGMT_P0_LED:
        case MGMT_P1_LED:

        case LED_CLEAR:

        //CPLD 2
        case QSFPDD_P0_INTR ... QSFPDD_P15_INTR:
        case QSFPDD_P0_7_INTR_EVENT ... QSFPDD_P8_15_INTR_EVENT:
        case QSFPDD_P0_ABS ... QSFPDD_P15_ABS:
        case QSFPDD_P0_7_ABS_EVENT ... QSFPDD_P8_15_ABS_EVENT:
        case QSFPDD_P0_FUSE_INTR ... QSFPDD_P15_FUSE_INTR:
        case QSFPDD_P0_7_FUSE_INTR_EVENT ... QSFPDD_P8_15_FUSE_INTR_EVENT:
        case QSFPDD_P0_INTR_MASK ... QSFPDD_P15_INTR_MASK:
        case QSFPDD_P0_ABS_MASK ... QSFPDD_P15_ABS_MASK:
        case QSFPDD_P0_FUSE_INTR_MASK ... QSFPDD_P15_FUSE_INTR_MASK:
        case QSFPDD_P0_RST ... QSFPDD_P15_RST:
        case QSFPDD_P0_LP_MODE ... QSFPDD_P15_LP_MODE:

        // FPGA
        case FPGA_MAJOR_VER:
        case FPGA_MINOR_VER:
        case FPGA_BUILD_VER:
        case FPGA_VERSION_H:
        case FPGA_ID:
        case FPGA_MAC_ROV_1:
        case FPGA_MAC_ROV_2:

        //CPLD 3

        case QSFPDD_P16_INTR ... QSFPDD_P31_INTR:
        case QSFPDD_P16_23_INTR_EVENT ... QSFPDD_P24_31_INTR_EVENT:
        case QSFPDD_P16_ABS ... QSFPDD_P31_ABS:
        case QSFPDD_P16_23_ABS_EVENT ... QSFPDD_P24_31_ABS_EVENT:
        case QSFPDD_P16_FUSE_INTR ... QSFPDD_P31_FUSE_INTR:
        case QSFPDD_P16_23_FUSE_INTR_EVENT ... QSFPDD_P24_31_FUSE_INTR_EVENT:
        case QSFPDD_P16_INTR_MASK ... QSFPDD_P31_INTR_MASK:
        case QSFPDD_P16_ABS_MASK ... QSFPDD_P31_ABS_MASK:
        case QSFPDD_P16_FUSE_INTR_MASK ... QSFPDD_P31_FUSE_INTR_MASK:
        case QSFPDD_P16_RST ... QSFPDD_P31_RST:
        case QSFPDD_P16_LP_MODE ... QSFPDD_P31_LP_MODE:

        //CPLD 4
        case MGMT_P0_TS ... SFP_P35_TS:
        case MGMT_P0_RS ... SFP_P35_RS:
        case MGMT_P0_TX_DIS ... SFP_P35_TX_DIS:
        case MGMT_P0_TX_FLT ... SFP_P35_TX_FLT:
        case MGMT_P0_RX_LOS ... SFP_P35_RX_LOS:
        case MGMT_P0_ABS ... SFP_P35_ABS:
        case MGMT_P0_TX_FLT_MASK ... SFP_P35_TX_FLT_MASK:
        case MGMT_P0_RX_LOS_MASK ... SFP_P35_RX_LOS_MASK:
        case MGMT_P0_ABS_MASK ... SFP_P35_ABS_MASK:
        case MGMT_P0_TX_FLT_EVENT ... SFP_P35_TX_FLT_EVENT:
        case MGMT_P0_RX_LOS_EVENT ... SFP_P35_RX_LOS_EVENT:
        case MGMT_P0_ABS_EVENT ... SFP_P35_ABS_EVENT:
        case MGMT_SFP_TX_FLT_EVENT:
        case MGMT_SFP_RX_LOS_EVENT:
        case MGMT_SFP_ABS_EVENT:

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
    u8 reg_val = 0;
    u8 reg = 0;
    u8 mask = MASK_NONE;
    char *range = NULL;
    int ret = 0;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    ret = _store_value_check(attr->index, reg_val, &range);
    if (ret < 0) {
        if(ret == -2) {
            return -EINVAL;
        } else {
            pr_err("Input is out of range(%s)\n", range);
            return -EINVAL;
        }
    }

    switch (attr->index) {
        //CPLD Common
        case EVENT_DETECT_CTRL:

        //CPLD 1
        case MAC_INTR_MASK:
        case CPLD_25GPHY_INTR_MASK:
        case CPLD_FRU_INTR_MASK:
        case NTM_INTR_MASK:
        case THERMAL_ALERT_1_INTR_MASK:
        case THERMAL_ALERT_2_INTR_MASK:
        case THERMAL_ALERT_3_INTR_MASK:
        case MISC_INTR_MASK:
        case SYSTEM_INTR_MASK:

        case CPLD_2_INTR_MASK:
        case CPLD_3_INTR_MASK:
        case CPLD_4_INTR_MASK:
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
        case THERMAL_ALERT_INTR_MASK:
        case PSU_CPLD_INTR_MASK:
        case MAIN_CHIP_INTR_MASK:
        case PTP_INTR_MASK:

        case MAC_RST:
        case BIOS_FLASH_RST:
        case BMC_RST:
        case BMC_PCIE_RST:
        case BMC_LPC_RST:
        case BMC_WDT_RST:
        case NTM_RST:

        case USB_REDRIVER_EXT_RST:
        case MAC_QSPI_FLASH_RST:
        case CP2130_RST:
        case USB_REDRIVER_NTM_RST:
        case USB_REDRIVER_MB_RST:
        case USB_CURR_LIMITER_EN:

        case CPLD_2_RST:
        case CPLD_3_RST:
        case FPGA_RST:

        case I2C_MUX_RST:
        case I2C_MUX_9548_1_RST:
        case I2C_MUX_9548_2_RST:
        case I2C_MUX_FAN_RST:

        case BCM82399_RST:
        case CP2102_RST:
        case RC32312_RST:
        case I210_RST:
        case IOEXP_RST:
        case FPGA_PCIE_RST:
        case I2C_MUX_OOB_RST:
        case I2C_MUX_TS_RST:

        case PSU_RST:

        case CPU_MUX_SEL:
        case PSU_MUX_SEL:
        case FPGA_QSPI_SEL:
        case BMC_UART_MUX_SEL:
        case SYSTEM_UART_MUX_SEL:
        case FPGA_CTRL:
        case FPGA_NCONFIG:
        case FPGA_CONFIG_DONE:

        case CPLD_SYSTEM_LED_SYS:
        case SYSTEM_LED_STATUS:
        case SYSTEM_LED_SPEED:
        case SYSTEM_LED_BLINK:
        case SYSTEM_LED_ONOFF:
        case CPLD_SYSTEM_LED_FAN:
        case FAN_LED_STATUS:
        case FAN_LED_SPEED:
        case FAN_LED_BLINK:
        case FAN_LED_ONOFF:

        case CPLD_SYSTEM_LED_PSU_0:
        case PSU_0_LED_STATUS:
        case PSU_0_LED_SPEED:
        case PSU_0_LED_BLINK:
        case PSU_0_LED_ONOFF:
        case CPLD_SYSTEM_LED_PSU_1:
        case PSU_1_LED_STATUS:
        case PSU_1_LED_SPEED:
        case PSU_1_LED_BLINK:
        case PSU_1_LED_ONOFF:

        case CPLD_SYSTEM_LED_SYNC:
        case SYNC_LED_STATUS:
        case SYNC_LED_SPEED:
        case SYNC_LED_BLINK:
        case SYNC_LED_ONOFF:

        case CPLD_SYSTEM_LED_ID:
        case ID_LED_STATUS:
        case ID_LED_SPEED:
        case ID_LED_BLINK:
        case ID_LED_ONOFF:

        case LED_CLEAR:

        case MGMT_0_LED_STATUS:
        case MGMT_0_LED_SPEED:
        case MGMT_0_LED_BLINK:
        case MGMT_0_LED_ONOFF:
        case MGMT_1_LED_STATUS:
        case MGMT_1_LED_SPEED:
        case MGMT_1_LED_BLINK:
        case MGMT_1_LED_ONOFF:

        case MGMT_P0_LED:
        case MGMT_P1_LED:

        //CPLD 2
        case QSFPDD_P0_INTR_MASK ... QSFPDD_P15_INTR_MASK:
        case QSFPDD_P0_ABS_MASK ... QSFPDD_P15_ABS_MASK:
        case QSFPDD_P0_FUSE_INTR_MASK ... QSFPDD_P15_FUSE_INTR_MASK:
        case QSFPDD_P0_RST ... QSFPDD_P15_RST:
        case QSFPDD_P0_LP_MODE ... QSFPDD_P15_LP_MODE:

        //FPGA

        //CPLD 3
        case QSFPDD_P16_INTR_MASK ... QSFPDD_P31_INTR_MASK:
        case QSFPDD_P16_ABS_MASK ... QSFPDD_P31_ABS_MASK:
        case QSFPDD_P16_FUSE_INTR_MASK ... QSFPDD_P31_FUSE_INTR_MASK:
        case QSFPDD_P16_RST ... QSFPDD_P31_RST:
        case QSFPDD_P16_LP_MODE ... QSFPDD_P31_LP_MODE:
        //CPLD 4
        case MGMT_P0_TS ... SFP_P35_TS:
        case MGMT_P0_RS ... SFP_P35_RS:
        case MGMT_P0_TX_DIS ... SFP_P35_TX_DIS:
        case MGMT_P0_TX_FLT_MASK ... SFP_P35_TX_FLT_MASK:
        case MGMT_P0_RX_LOS_MASK ... SFP_P35_RX_LOS_MASK:
        case MGMT_P0_ABS_MASK ... SFP_P35_ABS_MASK:
            reg = attr_reg[attr->index].reg;
            mask= attr_reg[attr->index].mask;
            break;
        default:
            return -EINVAL;
    }
    return cpld_reg_write(dev, buf, count, reg, mask);
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
                    u8 mask)
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

    ret = _cpld_reg_write(dev, reg, reg_val);

    if (unlikely(ret < 0)) {
        dev_err(dev, "cpld_reg_write() error, return=%d\n", ret);
        return ret;
    }

    return count;
}

/* get cpld and fpga version register value */
static ssize_t version_h_show(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int major =-1;
    int minor =-1;
    int build =-1;
    int major_val = -1;
    int minor_val = -1;
    int build_val = -1;

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
        major_val = _cpld_reg_read(dev, attr_reg[major].reg, attr_reg[major].mask);
        minor_val = _cpld_reg_read(dev, attr_reg[minor].reg, attr_reg[minor].mask);
        build_val = _cpld_reg_read(dev, attr_reg[build].reg, attr_reg[build].mask);

        if(major_val < 0 || minor_val < 0 || build_val < 0)
            return -EIO ;

        return sprintf(buf, "%d.%02d.%03d", major_val, minor_val, build_val);
    }
    return -EINVAL;
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
    u16 hw_build_rev_reg = (REG_BASE_MB + CPLD_HW_BUILD_REV_REG);
    u8 hw_build_rev_mask = attr_reg[BRD_HW_ID].mask;

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
        data->hw_id = inb(hw_build_rev_reg) & hw_build_rev_mask;
        if (data->hw_id <= 0x2) {
            status = sysfs_create_group(&client->dev.kobj,
                    &cpld1_group_beta);
        } else {
            status = sysfs_create_group(&client->dev.kobj,
                    &cpld1_group_pvt);
        }
        break;
    case cpld2:
        status = sysfs_create_group(&client->dev.kobj,
                    &cpld2_group);
        break;
    case cpld3:
        status = sysfs_create_group(&client->dev.kobj,
                    &cpld3_group);
        break;
    case cpld4:
        status = sysfs_create_group(&client->dev.kobj,
                    &cpld4_group);
        break;
    case fpga:
        status = sysfs_create_group(&client->dev.kobj,
                    &fpga_group);
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
        if (data->hw_id <= 0x2) {
            sysfs_remove_group(&client->dev.kobj, &cpld1_group_beta);
        } else {
            sysfs_remove_group(&client->dev.kobj, &cpld1_group_pvt);
        }
        break;
    case cpld2:
        sysfs_remove_group(&client->dev.kobj, &cpld2_group);
        break;
    case cpld3:
        sysfs_remove_group(&client->dev.kobj, &cpld3_group);
        break;
    case cpld4:
        sysfs_remove_group(&client->dev.kobj, &cpld4_group);
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
        if (data->hw_id <= 0x2) {
            sysfs_remove_group(&client->dev.kobj, &cpld1_group_beta);
        } else {
            sysfs_remove_group(&client->dev.kobj, &cpld1_group_pvt);
        }
        break;
    case cpld2:
        sysfs_remove_group(&client->dev.kobj, &cpld2_group);
        break;
    case cpld3:
        sysfs_remove_group(&client->dev.kobj, &cpld3_group);
        break;
    case cpld4:
        sysfs_remove_group(&client->dev.kobj, &cpld4_group);
        break;
    case fpga:
        sysfs_remove_group(&client->dev.kobj, &fpga_group);
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
        .name = "x86_64_ufispace_s9620_32e_cpld",
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

MODULE_AUTHOR("Jason Tsai <jason.cy.tsai@ufispace.com>");
MODULE_DESCRIPTION("x86_64_ufispace_s9620_32e_cpld driver");
MODULE_VERSION("1.0.0");
MODULE_LICENSE("GPL");

module_init(cpld_init);
module_exit(cpld_exit);
