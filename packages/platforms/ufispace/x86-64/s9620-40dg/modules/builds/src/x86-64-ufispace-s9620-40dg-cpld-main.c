/*
 * A i2c cpld driver for the ufispace_s9620_40dg
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
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/dmi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/version.h>

#include "x86-64-ufispace-s9620-40dg-cpld-main.h"

static bool mux_en = false;
module_param(mux_en, bool, S_IWUSR|S_IRUSR);

static int system_hw_id = -1;

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

static attr_reg_map_t attr_reg[] = {
    // & cpld_common
    [CPLD_MINOR_VER]                = { CPLD_VERSION_REG,                  0b00111111,  DATA_DEC,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD_MAJOR_VER]                = { CPLD_VERSION_REG,                  0b11000000,  DATA_DEC,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD_ID]                       = { CPLD_ID_REG,                       0b00000111,  DATA_DEC,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD_BUILD_VER]                = { CPLD_BUILD_REG,                    MASK_ALL,    DATA_DEC,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD_VERSION_H]                = { NONE_REG,                          MASK_ALL,    DATA_DEC,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD_CHIP_TYPE]                = { CPLD_CHIP_TYPE_REG,                0b00000111,  DATA_DEC,        REG_WP_DIS,  REG_NOT_EVENT},
    [EVENT_DETECT_CTRL]             = { EVENT_DETECT_CTRL_REG,             0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [MODULE_RESET]                  = { MODULE_RESET_REG,                  0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [CPLD_TEST]                     = { CPLD_TEST_REG,                     MASK_ALL,    DATA_DEC,        REG_WP_DIS,  REG_NOT_EVENT},

    // & cpld2_3_4
    [CPLD_I2C_CONTROL]              = { CPLD_I2C_CONTROL_REG,              MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD_I2C_RELAY]                = { CPLD_I2C_RELAY_REG,                MASK_ALL,    DATA_DEC,        REG_WP_DIS,  REG_NOT_EVENT},

    // & cpld1
    [BRD_SKU_ID]                    = { BRD_SKU_ID_REG,                    MASK_ALL,    DATA_DEC,        REG_WP_DIS,  REG_NOT_EVENT},
    [BRD_HW_ID]                     = { BRD_HW_BUILD_REV_REG,              0b00000011,  DATA_DEC,        REG_WP_DIS,  REG_NOT_EVENT},
    [BRD_DEPH_ID]                   = { BRD_HW_BUILD_REV_REG,              0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [BRD_BUILD_ID]                  = { BRD_HW_BUILD_REV_REG,              0b00011000,  DATA_DEC,        REG_WP_DIS,  REG_NOT_EVENT},
    [BRD_ID_TYPE]                   = { BRD_HW_BUILD_REV_REG,              0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD_BOARD_EXT_ID]             = { CPLD_BOARD_EXT_ID_REG,             0b00000111,  DATA_DEC,        REG_WP_DIS,  REG_NOT_EVENT},
    [GDDR6_ID]                      = { GDDR6_ID_REG,                      0b00000111,  DATA_DEC,        REG_WP_DIS,  REG_NOT_EVENT},
    [GDDR6_ID_FUNC]                 = { GDDR6_ID_FUNC_REG,                 0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [CLK_PTP_INTR]                  = { CLK_PTP_INTR_REG,                  MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [PHY_INTR]                      = { PHY_INTR_REG,                      MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [TOP_BRD_CPLD_FRU_INTR]         = { TOP_BRD_CPLD_FRU_INTR_REG,         MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [PSU0_INTR]                     = { TOP_BRD_CPLD_FRU_INTR_REG,         0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [PSU1_INTR]                     = { TOP_BRD_CPLD_FRU_INTR_REG,         0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD2_INTR]                    = { TOP_BRD_CPLD_FRU_INTR_REG,         0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [MAIN_BRD_CPLD_INTR]            = { MAIN_BRD_CPLD_INTR_REG,            MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD3_INTR]                    = { MAIN_BRD_CPLD_INTR_REG,            0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD4_INTR]                    = { MAIN_BRD_CPLD_INTR_REG,            0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [MB_ETH_INTR]                   = { MAIN_BRD_CPLD_INTR_REG,            0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [FAN_INTR]                      = { MAIN_BRD_CPLD_INTR_REG,            0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [THERMAL_INTR]                  = { THERMAL_INTR_REG,                  MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [USB_SSD_INTR]                  = { USB_SSD_INTR_REG,                  MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPU_NMI_INTR]                  = { CPU_NMI_INTR_REG,                  MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [OUT_STATUS_INTR]               = { OUT_STATUS_INTR_REG,               MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [CLK_PTP_INTR_MASK]             = { CLK_PTP_INTR_MASK_REG,             MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [PHY_INTR_MASK]                 = { PHY_INTR_MASK_REG,                 MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [TOP_BRD_CPLD_FRU_INTR_MASK]    = { TOP_BRD_CPLD_FRU_INTR_MASK_REG,    MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [PSU0_INTR_MASK]                = { TOP_BRD_CPLD_FRU_INTR_MASK_REG,    0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [PSU1_INTR_MASK]                = { TOP_BRD_CPLD_FRU_INTR_MASK_REG,    0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD2_INTR_MASK]               = { TOP_BRD_CPLD_FRU_INTR_MASK_REG,    0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD2_IO_INTR_MASK]            = { TOP_BRD_CPLD_FRU_INTR_MASK_REG,    0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [MAIN_BRD_CPLD_INTR_MASK]       = { MAIN_BRD_CPLD_INTR_MASK_REG,       MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD3_INTR_MASK]               = { MAIN_BRD_CPLD_INTR_MASK_REG,       0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD4_INTR_MASK]               = { MAIN_BRD_CPLD_INTR_MASK_REG,       0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [MB_ETH_INTR_MASK]              = { MAIN_BRD_CPLD_INTR_MASK_REG,       0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [MB_PTP_INTR_MASK]              = { MAIN_BRD_CPLD_INTR_MASK_REG,       0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [FAN_INTR_MASK]                 = { MAIN_BRD_CPLD_INTR_MASK_REG,       0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [THERMAL_INTR_MASK]             = { THERMAL_INTR_MASK_REG,             MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [USB_SSD_INTR_MASK]             = { USB_SSD_INTR_MASK_REG,             MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPU_NMI_INTR_MASK]             = { CPU_NMI_INTR_MASK_REG,             MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [OUT_STATUS_INTR_MASK]          = { OUT_STATUS_INTR_MASK_REG,          MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [CLK_PTP_INTR_EVENT]            = { CLK_PTP_INTR_EVENT_REG,            MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_IS_EVENT},
    [PHY_INTR_EVENT]                = { PHY_INTR_EVENT_REG,                MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_IS_EVENT},
    [TOP_BRD_CPLD_FRU_INTR_EVENT]   = { TOP_BRD_CPLD_FRU_INTR_EVENT_REG,   MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_IS_EVENT},
    [MAIN_BRD_CPLD_INTR_EVENT]      = { MAIN_BRD_CPLD_INTR_EVENT_REG,      MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_IS_EVENT},
    [THERMAL_INTR_EVENT]            = { THERMAL_INTR_EVENT_REG,            MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_IS_EVENT},
    [USB_SSD_INTR_EVENT]            = { USB_SSD_INTR_EVENT_REG,            MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_IS_EVENT},
    [CPU_NMI_INTR_EVENT]            = { CPU_NMI_INTR_EVENT_REG,            MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_IS_EVENT},
    [OUT_STATUS_INTR_EVENT]         = { OUT_STATUS_INTR_EVENT_REG,         MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_IS_EVENT},
    [BTN_FP_RESET]                  = { BIOS_FLASH_RESET_CTRL_REG,         0b00010000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [SPI_BIOS_RESET]                = { BIOS_FLASH_RESET_CTRL_REG,         0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [CPU_BOARD_CTRL]                = { CPU_BOARD_CTRL_REG,                MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [RGB_1_RESET]                   = { BMC_PHY_RESET_CTRL_REG,            0b00100000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [RGB_0_RESET]                   = { BMC_PHY_RESET_CTRL_REG,            0b00010000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [BMC_LPC_RESET]                 = { BMC_PHY_RESET_CTRL_REG,            0b00001000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [BMC_PCIE_RESET]                = { BMC_PHY_RESET_CTRL_REG,            0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [CPLD_TO_BMC_SYS_RESET]         = { BMC_PHY_RESET_CTRL_REG,            0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [CPLD_TO_CPU_RESET]             = { BMC_PHY_RESET_CTRL_REG,            0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [USB_PWR_EN]                    = { USB_RESET_CTRL_REG,                0b10000000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [USB_SIE_RESET]                 = { USB_RESET_CTRL_REG,                0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [I2C_MUX_SYS_RESET]             = { TOP_I2C_MUX_RESET_REG,             0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [I2C_MUX_SMBUS_RESET]           = { TOP_I2C_MUX_RESET_REG,             0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [I2C_MUX_QSFP28_RESET]          = { TOP_I2C_MUX_RESET_REG,             0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [I2C_MUX_SFP_RESET]             = { TOP_I2C_MUX_RESET_REG,             0b00001000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [MB_I2C_RESET]                  = { TOP_I2C_MUX_RESET_REG,             0b00010000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [BMC_PRESENT]                   = { DAUGHTER_BRD_ABS_REG,              0b00000001,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [SATA_SSD1_PRESENT]             = { DAUGHTER_BRD_ABS_REG,              0b00000010,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [SATA_SSD2_PRESENT]             = { DAUGHTER_BRD_ABS_REG,              0b00000100,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [PSU0_PRESENT]                  = { PSU_STATUS_REG,                    0b00000001,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [PSU1_PRESENT]                  = { PSU_STATUS_REG,                    0b00000010,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [PSU0_VIN_PG]                   = { PSU_STATUS_REG,                    0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [PSU1_VIN_PG]                   = { PSU_STATUS_REG,                    0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [PSU0_VOUT_PG]                  = { PSU_STATUS_REG,                    0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [PSU1_VOUT_PG]                  = { PSU_STATUS_REG,                    0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [PSU_STATUS]                    = { PSU_STATUS_REG,                    MASK_ALL,    DATA_DEC,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPU_BOOT_DONE]                 = { SYSTEM_PWR_STATUS_REG,             0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPU_PG]                        = { SYSTEM_PWR_STATUS_REG,             0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPU_STATUS]                    = { CPU_STATUS_REG,                    MASK_ALL,    DATA_DEC,        REG_WP_DIS,  REG_NOT_EVENT},
    [PHY_BOOT_CTRL]                 = { PHY_BOOT_CTRL_REG,                 MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [WD_STATUS]                     = { WD_STATUS_REG,                     MASK_ALL,    DATA_DEC,        REG_WP_EN,   REG_NOT_EVENT},
    [TIMING_CTRL_STATUS]            = { TIMING_CTRL_STATUS_REG,            MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [SMBUS_PECI_DIS]                = { MUX_CTRL_REG,                      0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [I2C_PSU0_MUX_SEL]              = { MUX_CTRL_REG,                      0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [I2C_PSU1_MUX_SEL]              = { MUX_CTRL_REG,                      0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [I2C_CPLD_MUX_SEL]              = { MUX_CTRL_REG,                      0b00010000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [BMC_USB_MUX_SEL]               = { MUX_CTRL_REG,                      0b00100000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [UART_CPU_BMC_MUX_SEL]          = { MUX_CTRL_REG,                      0b01000000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [UART_MUX_SEL]                  = { MUX_CTRL_REG,                      0b10000000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [PWR_SYSTEM_CTRL]               = { PWR_SYSTEM_CTRL_REG,               MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [SERBOOT_UFM_STORE]             = { SERBOOT_UFM_STORE_1_REG,           MASK_ALL,    DATA_DEC,        REG_WP_DIS,  REG_NOT_EVENT},
    [SERBOOT_UFM_WRITE]             = { SERBOOT_UFM_WRITE_1_REG,           MASK_ALL,    DATA_DEC,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD_WRITE_PROTECT_1]          = { WRITE_PROTECT_1_REG,               0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD_WRITE_PROTECT_2]          = { WRITE_PROTECT_2_REG,               MASK_ALL,    DATA_DEC,        REG_WP_DIS,  REG_NOT_EVENT},
    [EXT_CTRL]                      = { EXT_CTRL_REG,                      0b00000011,  DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD_SYSTEM_LED_SYS]           = { SYSTEM_LED_CTRL_1_REG,             0b00001111,  DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [SYSTEM_LED_STATUS]             = { SYSTEM_LED_CTRL_1_REG,             0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [SYSTEM_LED_SPEED]              = { SYSTEM_LED_CTRL_1_REG,             0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [SYSTEM_LED_BLINK]              = { SYSTEM_LED_CTRL_1_REG,             0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [SYSTEM_LED_ONOFF]              = { SYSTEM_LED_CTRL_1_REG,             0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD_SYSTEM_LED_FAN]           = { SYSTEM_LED_CTRL_1_REG,             0b11110000,  DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [FAN_LED_STATUS]                = { SYSTEM_LED_CTRL_1_REG,             0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [FAN_LED_SPEED]                 = { SYSTEM_LED_CTRL_1_REG,             0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [FAN_LED_BLINK]                 = { SYSTEM_LED_CTRL_1_REG,             0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [FAN_LED_ONOFF]                 = { SYSTEM_LED_CTRL_1_REG,             0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD_SYSTEM_LED_PWR]           = { SYSTEM_LED_CTRL_2_REG,             0b00001111,  DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [PWR_LED_STATUS]                = { SYSTEM_LED_CTRL_2_REG,             0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [PWR_LED_SPEED]                 = { SYSTEM_LED_CTRL_2_REG,             0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [PWR_LED_BLINK]                 = { SYSTEM_LED_CTRL_2_REG,             0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [PWR_LED_ONOFF]                 = { SYSTEM_LED_CTRL_2_REG,             0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD_SYSTEM_LED_GNSS]          = { SYSTEM_LED_CTRL_2_REG,             0b11110000,  DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [GNSS_LED_STATUS]               = { SYSTEM_LED_CTRL_2_REG,             0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [GNSS_LED_SPEED]                = { SYSTEM_LED_CTRL_2_REG,             0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [GNSS_LED_BLINK]                = { SYSTEM_LED_CTRL_2_REG,             0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [GNSS_LED_ONOFF]                = { SYSTEM_LED_CTRL_2_REG,             0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD_SYSTEM_LED_SYNC]          = { SYSTEM_LED_CTRL_3_REG,             0b00001111,  DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [SYNC_LED_STATUS]               = { SYSTEM_LED_CTRL_3_REG,             0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [SYNC_LED_SPEED]                = { SYSTEM_LED_CTRL_3_REG,             0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [SYNC_LED_BLINK]                = { SYSTEM_LED_CTRL_3_REG,             0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [SYNC_LED_ONOFF]                = { SYSTEM_LED_CTRL_3_REG,             0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD_SYSTEM_LED_ID]            = { SYSTEM_LED_CTRL_3_REG,             0b11110000,  DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [ID_LED_SPEED]                  = { SYSTEM_LED_CTRL_3_REG,             0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ID_LED_BLINK]                  = { SYSTEM_LED_CTRL_3_REG,             0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ID_LED_ONOFF]                  = { SYSTEM_LED_CTRL_3_REG,             0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [LED_CLEAR]                     = { LED_CLEAR_REG,                     0b00111001,  DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD1_PWR_STATUS]              = { CPLD1_PWR_STATUS_REG,              MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_0_PWR_EN]                  = { QSFP28_0_5_PWR_EN_REG,             0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_1_PWR_EN]                  = { QSFP28_0_5_PWR_EN_REG,             0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_2_PWR_EN]                  = { QSFP28_0_5_PWR_EN_REG,             0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_3_PWR_EN]                  = { QSFP28_0_5_PWR_EN_REG,             0b00001000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_4_PWR_EN]                  = { QSFP28_0_5_PWR_EN_REG,             0b00010000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_5_PWR_EN]                  = { QSFP28_0_5_PWR_EN_REG,             0b00100000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_12_PWR_EN]                 = { QSFPDD_12_15_PWR_EN_REG,           0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_13_PWR_EN]                 = { QSFPDD_12_15_PWR_EN_REG,           0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_14_PWR_EN]                 = { QSFPDD_12_15_PWR_EN_REG,           0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_15_PWR_EN]                 = { QSFPDD_12_15_PWR_EN_REG,           0b00001000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_6_PWR_EN]                  = { QSFP28_6_11_PWR_EN_REG,            0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_7_PWR_EN]                  = { QSFP28_6_11_PWR_EN_REG,            0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_8_PWR_EN]                  = { QSFP28_6_11_PWR_EN_REG,            0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_9_PWR_EN]                  = { QSFP28_6_11_PWR_EN_REG,            0b00001000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_10_PWR_EN]                 = { QSFP28_6_11_PWR_EN_REG,            0b00010000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_11_PWR_EN]                 = { QSFP28_6_11_PWR_EN_REG,            0b00100000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_16_PWR_EN]                 = { SFP56_16_23_PWR_EN_0_REG,          0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_17_PWR_EN]                 = { SFP56_16_23_PWR_EN_0_REG,          0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_18_PWR_EN]                 = { SFP56_16_23_PWR_EN_0_REG,          0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_19_PWR_EN]                 = { SFP56_16_23_PWR_EN_0_REG,          0b00001000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_20_PWR_EN]                 = { SFP56_16_23_PWR_EN_0_REG,          0b00010000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_21_PWR_EN]                 = { SFP56_16_23_PWR_EN_0_REG,          0b00100000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_22_PWR_EN]                 = { SFP56_16_23_PWR_EN_0_REG,          0b01000000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_23_PWR_EN]                 = { SFP56_16_23_PWR_EN_0_REG,          0b10000000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_24_PWR_EN]                 = { SFP56_24_31_PWR_EN_1_REG,          0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_25_PWR_EN]                 = { SFP56_24_31_PWR_EN_1_REG,          0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_26_PWR_EN]                 = { SFP56_24_31_PWR_EN_1_REG,          0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_27_PWR_EN]                 = { SFP56_24_31_PWR_EN_1_REG,          0b00001000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_28_PWR_EN]                 = { SFP56_24_31_PWR_EN_1_REG,          0b00010000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_29_PWR_EN]                 = { SFP56_24_31_PWR_EN_1_REG,          0b00100000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_30_PWR_EN]                 = { SFP56_24_31_PWR_EN_1_REG,          0b01000000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_31_PWR_EN]                 = { SFP56_24_31_PWR_EN_1_REG,          0b10000000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_32_PWR_EN]                 = { SFP56_32_39_PWR_EN_2_REG,          0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_33_PWR_EN]                 = { SFP56_32_39_PWR_EN_2_REG,          0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_34_PWR_EN]                 = { SFP56_32_39_PWR_EN_2_REG,          0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_35_PWR_EN]                 = { SFP56_32_39_PWR_EN_2_REG,          0b00001000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_36_PWR_EN]                 = { SFP56_32_39_PWR_EN_2_REG,          0b00010000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_37_PWR_EN]                 = { SFP56_32_39_PWR_EN_2_REG,          0b00100000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_38_PWR_EN]                 = { SFP56_32_39_PWR_EN_2_REG,          0b01000000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_39_PWR_EN]                 = { SFP56_32_39_PWR_EN_2_REG,          0b10000000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [OCXO_GNSS_ID]                  = { OCXO_GNSS_ID_REG,                  MASK_ALL,    DATA_DEC,        REG_WP_EN,   REG_NOT_EVENT},
    [CLK_PTP_RESET]                 = { CLK_PTP_RESET_REG,                 MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [CJAP_RESET]                    = { CLK_PTP_RESET_REG,                 0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [NTM_RESET]                     = { CLK_PTP_RESET_REG,                 0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [GNSS_RESET]                    = { CLK_PTP_RESET_REG,                 0b00001000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [BITS_RESET]                    = { CLK_PTP_RESET_REG,                 0b00100000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [CLK_TIMING_CTRL]               = { CLK_TIMING_CTRL_REG,               MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [GNSS_STATUS]                   = { GNSS_STATUS_REG,                   MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [TIMING_STATUS]                 = { TIMING_STATUS_REG,                 MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [QSFPDD_SEL]                    = { CLK_BUFFER_EN_CTRL_REG,            0b11110000,  DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_28_SEL]                    = { CLK_BUFFER_EN_CTRL_REG,            0b00001111,  DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [I2C_MUX_0X76_RESET]            = { MAIN_I2C_MUX_RESET_REG,            0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [I2C_MUX_0X75_RESET]            = { MAIN_I2C_MUX_RESET_REG,            0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [I2C_MUX_6_11_RESET]            = { MAIN_I2C_MUX_RESET_REG,            0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [I2C_MUX_SFP56_16_23_RESET]     = { MAIN_I2C_MUX_RESET_REG,            0b00001000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [I2C_MUX_SFP56_24_31_RESET]     = { MAIN_I2C_MUX_RESET_REG,            0b00010000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [I2C_MUX_SFP56_32_39_RESET]     = { MAIN_I2C_MUX_RESET_REG,            0b00100000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [I2C_MUX_0X71_RESET]            = { MAIN_I2C_MUX_RESET_REG,            0b01000000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},

    // & cpld2
    [ETH_0_PRESENT]                 = { QSFP28_0_5_ABS_REG,                0b00000001,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_1_PRESENT]                 = { QSFP28_0_5_ABS_REG,                0b00000010,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_2_PRESENT]                 = { QSFP28_0_5_ABS_REG,                0b00000100,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_3_PRESENT]                 = { QSFP28_0_5_ABS_REG,                0b00001000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_4_PRESENT]                 = { QSFP28_0_5_ABS_REG,                0b00010000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_5_PRESENT]                 = { QSFP28_0_5_ABS_REG,                0b00100000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_12_PRESENT]                = { QSFPDD_12_15_ABS_REG,              0b00000001,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_13_PRESENT]                = { QSFPDD_12_15_ABS_REG,              0b00000010,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_14_PRESENT]                = { QSFPDD_12_15_ABS_REG,              0b00000100,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_15_PRESENT]                = { QSFPDD_12_15_ABS_REG,              0b00001000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_0_INTR]                    = { QSFP28_0_5_INTR_REG,               0b00000001,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_1_INTR]                    = { QSFP28_0_5_INTR_REG,               0b00000010,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_2_INTR]                    = { QSFP28_0_5_INTR_REG,               0b00000100,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_3_INTR]                    = { QSFP28_0_5_INTR_REG,               0b00001000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_4_INTR]                    = { QSFP28_0_5_INTR_REG,               0b00010000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_5_INTR]                    = { QSFP28_0_5_INTR_REG,               0b00100000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_12_INTR]                   = { QSFPDD_12_15_INTR_REG,             0b00000001,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_13_INTR]                   = { QSFPDD_12_15_INTR_REG,             0b00000010,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_14_INTR]                   = { QSFPDD_12_15_INTR_REG,             0b00000100,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_15_INTR]                   = { QSFPDD_12_15_INTR_REG,             0b00001000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_0_EFUSE_PG]                = { QSFP28_0_5_EFUSE_PG_REG,           0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_1_EFUSE_PG]                = { QSFP28_0_5_EFUSE_PG_REG,           0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_2_EFUSE_PG]                = { QSFP28_0_5_EFUSE_PG_REG,           0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_3_EFUSE_PG]                = { QSFP28_0_5_EFUSE_PG_REG,           0b00001000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_4_EFUSE_PG]                = { QSFP28_0_5_EFUSE_PG_REG,           0b00010000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_5_EFUSE_PG]                = { QSFP28_0_5_EFUSE_PG_REG,           0b00100000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_12_EFUSE_PG]               = { QSFPDD_12_15_EFUSE_PG_REG,         0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_13_EFUSE_PG]               = { QSFPDD_12_15_EFUSE_PG_REG,         0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_14_EFUSE_PG]               = { QSFPDD_12_15_EFUSE_PG_REG,         0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_15_EFUSE_PG]               = { QSFPDD_12_15_EFUSE_PG_REG,         0b00001000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_0_I2C_STUCK]               = { QSFP28_0_5_I2C_STUCK_REG,          0b00000001,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_1_I2C_STUCK]               = { QSFP28_0_5_I2C_STUCK_REG,          0b00000010,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_2_I2C_STUCK]               = { QSFP28_0_5_I2C_STUCK_REG,          0b00000100,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_3_I2C_STUCK]               = { QSFP28_0_5_I2C_STUCK_REG,          0b00001000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_4_I2C_STUCK]               = { QSFP28_0_5_I2C_STUCK_REG,          0b00010000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_5_I2C_STUCK]               = { QSFP28_0_5_I2C_STUCK_REG,          0b00100000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_12_I2C_STUCK]              = { QSFPDD_12_15_I2C_STUCK_REG,        0b00000001,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_13_I2C_STUCK]              = { QSFPDD_12_15_I2C_STUCK_REG,        0b00000010,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_14_I2C_STUCK]              = { QSFPDD_12_15_I2C_STUCK_REG,        0b00000100,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_15_I2C_STUCK]              = { QSFPDD_12_15_I2C_STUCK_REG,        0b00001000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD2_I2C_STUCK]               = { CPLD2_I2C_STUCK_REG,               0b00000001,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD2_TO_CPLD1_INTR]           = { CPLD2_TO_CPLD1_INTR_REG,           0b00000001,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_0_PRESENT_MASK]            = { QSFP28_0_5_ABS_MASK_REG,           0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_1_PRESENT_MASK]            = { QSFP28_0_5_ABS_MASK_REG,           0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_2_PRESENT_MASK]            = { QSFP28_0_5_ABS_MASK_REG,           0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_3_PRESENT_MASK]            = { QSFP28_0_5_ABS_MASK_REG,           0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_4_PRESENT_MASK]            = { QSFP28_0_5_ABS_MASK_REG,           0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_5_PRESENT_MASK]            = { QSFP28_0_5_ABS_MASK_REG,           0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_12_PRESENT_MASK]           = { QSFPDD_12_15_ABS_MASK_REG,         0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_13_PRESENT_MASK]           = { QSFPDD_12_15_ABS_MASK_REG,         0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_14_PRESENT_MASK]           = { QSFPDD_12_15_ABS_MASK_REG,         0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_15_PRESENT_MASK]           = { QSFPDD_12_15_ABS_MASK_REG,         0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_0_INTR_MASK]               = { QSFP28_0_5_INTR_MASK_REG,          0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_1_INTR_MASK]               = { QSFP28_0_5_INTR_MASK_REG,          0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_2_INTR_MASK]               = { QSFP28_0_5_INTR_MASK_REG,          0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_3_INTR_MASK]               = { QSFP28_0_5_INTR_MASK_REG,          0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_4_INTR_MASK]               = { QSFP28_0_5_INTR_MASK_REG,          0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_5_INTR_MASK]               = { QSFP28_0_5_INTR_MASK_REG,          0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_12_INTR_MASK]              = { QSFPDD_12_15_INTR_MASK_REG,        0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_13_INTR_MASK]              = { QSFPDD_12_15_INTR_MASK_REG,        0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_14_INTR_MASK]              = { QSFPDD_12_15_INTR_MASK_REG,        0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_15_INTR_MASK]              = { QSFPDD_12_15_INTR_MASK_REG,        0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_0_EFUSE_PG_MASK]           = { QSFP28_0_5_EFUSE_PG_MASK_REG,      0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_1_EFUSE_PG_MASK]           = { QSFP28_0_5_EFUSE_PG_MASK_REG,      0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_2_EFUSE_PG_MASK]           = { QSFP28_0_5_EFUSE_PG_MASK_REG,      0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_3_EFUSE_PG_MASK]           = { QSFP28_0_5_EFUSE_PG_MASK_REG,      0b00001000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_4_EFUSE_PG_MASK]           = { QSFP28_0_5_EFUSE_PG_MASK_REG,      0b00010000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_5_EFUSE_PG_MASK]           = { QSFP28_0_5_EFUSE_PG_MASK_REG,      0b00100000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_12_EFUSE_PG_MASK]          = { QSFPDD_12_15_EFUSE_PG_MASK_REG,    0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_13_EFUSE_PG_MASK]          = { QSFPDD_12_15_EFUSE_PG_MASK_REG,    0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_14_EFUSE_PG_MASK]          = { QSFPDD_12_15_EFUSE_PG_MASK_REG,    0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_15_EFUSE_PG_MASK]          = { QSFPDD_12_15_EFUSE_PG_MASK_REG,    0b00001000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_0_I2C_STUCK_MASK]          = { QSFP28_0_5_I2C_STUCK_MASK_REG,     0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_1_I2C_STUCK_MASK]          = { QSFP28_0_5_I2C_STUCK_MASK_REG,     0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_2_I2C_STUCK_MASK]          = { QSFP28_0_5_I2C_STUCK_MASK_REG,     0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_3_I2C_STUCK_MASK]          = { QSFP28_0_5_I2C_STUCK_MASK_REG,     0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_4_I2C_STUCK_MASK]          = { QSFP28_0_5_I2C_STUCK_MASK_REG,     0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_5_I2C_STUCK_MASK]          = { QSFP28_0_5_I2C_STUCK_MASK_REG,     0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_12_I2C_STUCK_MASK]         = { QSFPDD_12_15_I2C_STUCK_MASK_REG,   0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_13_I2C_STUCK_MASK]         = { QSFPDD_12_15_I2C_STUCK_MASK_REG,   0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_14_I2C_STUCK_MASK]         = { QSFPDD_12_15_I2C_STUCK_MASK_REG,   0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_15_I2C_STUCK_MASK]         = { QSFPDD_12_15_I2C_STUCK_MASK_REG,   0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD2_I2C_STUCK_MASK]          = { CPLD2_I2C_STUCK_MASK_REG,          0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD2_TO_CPLD1_INTR_MASK]      = { CPLD2_TO_CPLD1_INTR_MASK_REG,      0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_0_PRESENT_EVENT]           = { QSFP28_0_5_ABS_EVENT_REG,          0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_1_PRESENT_EVENT]           = { QSFP28_0_5_ABS_EVENT_REG,          0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_2_PRESENT_EVENT]           = { QSFP28_0_5_ABS_EVENT_REG,          0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_3_PRESENT_EVENT]           = { QSFP28_0_5_ABS_EVENT_REG,          0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_4_PRESENT_EVENT]           = { QSFP28_0_5_ABS_EVENT_REG,          0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_5_PRESENT_EVENT]           = { QSFP28_0_5_ABS_EVENT_REG,          0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_12_PRESENT_EVENT]          = { QSFPDD_12_15_ABS_EVENT_REG,        0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_13_PRESENT_EVENT]          = { QSFPDD_12_15_ABS_EVENT_REG,        0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_14_PRESENT_EVENT]          = { QSFPDD_12_15_ABS_EVENT_REG,        0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_15_PRESENT_EVENT]          = { QSFPDD_12_15_ABS_EVENT_REG,        0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_0_INTR_EVENT]              = { QSFP28_0_5_INTR_EVENT_REG,         0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_1_INTR_EVENT]              = { QSFP28_0_5_INTR_EVENT_REG,         0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_2_INTR_EVENT]              = { QSFP28_0_5_INTR_EVENT_REG,         0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_3_INTR_EVENT]              = { QSFP28_0_5_INTR_EVENT_REG,         0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_4_INTR_EVENT]              = { QSFP28_0_5_INTR_EVENT_REG,         0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_5_INTR_EVENT]              = { QSFP28_0_5_INTR_EVENT_REG,         0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_12_INTR_EVENT]             = { QSFPDD_12_15_INTR_EVENT_REG,       0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_13_INTR_EVENT]             = { QSFPDD_12_15_INTR_EVENT_REG,       0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_14_INTR_EVENT]             = { QSFPDD_12_15_INTR_EVENT_REG,       0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_15_INTR_EVENT]             = { QSFPDD_12_15_INTR_EVENT_REG,       0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_0_EFUSE_PG_EVENT]          = { QSFP28_0_5_EFUSE_PG_EVENT_REG,     0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_1_EFUSE_PG_EVENT]          = { QSFP28_0_5_EFUSE_PG_EVENT_REG,     0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_2_EFUSE_PG_EVENT]          = { QSFP28_0_5_EFUSE_PG_EVENT_REG,     0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_3_EFUSE_PG_EVENT]          = { QSFP28_0_5_EFUSE_PG_EVENT_REG,     0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_4_EFUSE_PG_EVENT]          = { QSFP28_0_5_EFUSE_PG_EVENT_REG,     0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_5_EFUSE_PG_EVENT]          = { QSFP28_0_5_EFUSE_PG_EVENT_REG,     0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_12_EFUSE_PG_EVENT]         = { QSFPDD_12_15_EFUSE_PG_EVENT_REG,   0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_13_EFUSE_PG_EVENT]         = { QSFPDD_12_15_EFUSE_PG_EVENT_REG,   0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_14_EFUSE_PG_EVENT]         = { QSFPDD_12_15_EFUSE_PG_EVENT_REG,   0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_15_EFUSE_PG_EVENT]         = { QSFPDD_12_15_EFUSE_PG_EVENT_REG,   0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_0_I2C_STUCK_EVENT]         = { QSFP28_0_5_I2C_STUCK_EVENT_REG,    0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_1_I2C_STUCK_EVENT]         = { QSFP28_0_5_I2C_STUCK_EVENT_REG,    0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_2_I2C_STUCK_EVENT]         = { QSFP28_0_5_I2C_STUCK_EVENT_REG,    0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_3_I2C_STUCK_EVENT]         = { QSFP28_0_5_I2C_STUCK_EVENT_REG,    0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_4_I2C_STUCK_EVENT]         = { QSFP28_0_5_I2C_STUCK_EVENT_REG,    0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_5_I2C_STUCK_EVENT]         = { QSFP28_0_5_I2C_STUCK_EVENT_REG,    0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_12_I2C_STUCK_EVENT]        = { QSFPDD_12_15_I2C_STUCK_EVENT_REG,  0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_13_I2C_STUCK_EVENT]        = { QSFPDD_12_15_I2C_STUCK_EVENT_REG,  0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_14_I2C_STUCK_EVENT]        = { QSFPDD_12_15_I2C_STUCK_EVENT_REG,  0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_15_I2C_STUCK_EVENT]        = { QSFPDD_12_15_I2C_STUCK_EVENT_REG,  0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [CPLD2_I2C_STUCK_EVENT]         = { CPLD2_I2C_STUCK_EVENT_REG,         MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_IS_EVENT},
    [CPLD2_TO_CPLD1_INTR_EVENT]     = { CPLD2_TO_CPLD1_INTR_EVENT_REG,     MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_0_RESET]                   = { QSFP28_0_5_RESET_REG,              0b00000001,  DATA_0_1_INV,    REG_WP_EN,   REG_NOT_EVENT},
    [ETH_1_RESET]                   = { QSFP28_0_5_RESET_REG,              0b00000010,  DATA_0_1_INV,    REG_WP_EN,   REG_NOT_EVENT},
    [ETH_2_RESET]                   = { QSFP28_0_5_RESET_REG,              0b00000100,  DATA_0_1_INV,    REG_WP_EN,   REG_NOT_EVENT},
    [ETH_3_RESET]                   = { QSFP28_0_5_RESET_REG,              0b00001000,  DATA_0_1_INV,    REG_WP_EN,   REG_NOT_EVENT},
    [ETH_4_RESET]                   = { QSFP28_0_5_RESET_REG,              0b00010000,  DATA_0_1_INV,    REG_WP_EN,   REG_NOT_EVENT},
    [ETH_5_RESET]                   = { QSFP28_0_5_RESET_REG,              0b00100000,  DATA_0_1_INV,    REG_WP_EN,   REG_NOT_EVENT},
    [ETH_12_RESET]                  = { QSFPDD_12_15_RESET_REG,            0b00000001,  DATA_0_1_INV,    REG_WP_EN,   REG_NOT_EVENT},
    [ETH_13_RESET]                  = { QSFPDD_12_15_RESET_REG,            0b00000010,  DATA_0_1_INV,    REG_WP_EN,   REG_NOT_EVENT},
    [ETH_14_RESET]                  = { QSFPDD_12_15_RESET_REG,            0b00000100,  DATA_0_1_INV,    REG_WP_EN,   REG_NOT_EVENT},
    [ETH_15_RESET]                  = { QSFPDD_12_15_RESET_REG,            0b00001000,  DATA_0_1_INV,    REG_WP_EN,   REG_NOT_EVENT},
    [ETH_0_LPMODE]                  = { QSFP28_0_5_LPMODE_REG,             0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_1_LPMODE]                  = { QSFP28_0_5_LPMODE_REG,             0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_2_LPMODE]                  = { QSFP28_0_5_LPMODE_REG,             0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_3_LPMODE]                  = { QSFP28_0_5_LPMODE_REG,             0b00001000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_4_LPMODE]                  = { QSFP28_0_5_LPMODE_REG,             0b00010000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_5_LPMODE]                  = { QSFP28_0_5_LPMODE_REG,             0b00100000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_12_LPMODE]                 = { QSFPDD_12_15_LPMODE_REG,           0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_13_LPMODE]                 = { QSFPDD_12_15_LPMODE_REG,           0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_14_LPMODE]                 = { QSFPDD_12_15_LPMODE_REG,           0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_15_LPMODE]                 = { QSFPDD_12_15_LPMODE_REG,           0b00001000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [CLK_EN_CTRL]                   = { CLK_EN_CTRL_REG,                   MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [PSU_CTRL]                      = { PSU_CTRL_REG,                      MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_12_LED_CTRL]               = { QSFPDD_P12_P13_LED_CTRL_REG,       0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_13_LED_CTRL]               = { QSFPDD_P12_P13_LED_CTRL_REG,       0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_14_LED_CTRL]               = { QSFPDD_P14_P15_LED_CTRL_REG,       0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_15_LED_CTRL]               = { QSFPDD_P14_P15_LED_CTRL_REG,       0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD2_PWR_STATUS_0]            = { CPLD2_PWR_STATUS_0_REG,            MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD2_PWR_STATUS_1]            = { CPLD2_PWR_STATUS_1_REG,            MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [DBG_QSFP28_0_5_ABS]            = { DBG_QSFP28_0_5_ABS_REG,            MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [DBG_QSFPDD_12_15_ABS]          = { DBG_QSFPDD_12_15_ABS_REG,          MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [DBG_QSFP28_0_5_INTR]           = { DBG_QSFP28_0_5_INTR_REG,           MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [DBG_QSFPDD_12_15_INTR]         = { DBG_QSFPDD_12_15_INTR_REG,         MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [DBG_QSFP28_0_5_EFUSE_PG]       = { DBG_QSFP28_0_5_EFUSE_PG_REG,       MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [DBG_QSFPDD_12_15_EFUSE_PG]     = { DBG_QSFPDD_12_15_EFUSE_PG_REG,     MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},

    // & cpld3
    [GNSS_MODEL_ID]                 = { GNSS_MODEL_ID_REG,                 0b00000111,  DATA_DEC,        REG_WP_DIS,  REG_NOT_EVENT},
    [OCXO_ID]                       = { OCXO_ID_REG,                       0b00000111,  DATA_DEC,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_6_PRESENT]                 = { QSFP28_6_11_ABS_REG,               0b00000001,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_7_PRESENT]                 = { QSFP28_6_11_ABS_REG,               0b00000010,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_8_PRESENT]                 = { QSFP28_6_11_ABS_REG,               0b00000100,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_9_PRESENT]                 = { QSFP28_6_11_ABS_REG,               0b00001000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_10_PRESENT]                = { QSFP28_6_11_ABS_REG,               0b00010000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_11_PRESENT]                = { QSFP28_6_11_ABS_REG,               0b00100000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_6_INTR]                    = { QSFP28_6_11_INTR_REG,              0b00000001,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_7_INTR]                    = { QSFP28_6_11_INTR_REG,              0b00000010,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_8_INTR]                    = { QSFP28_6_11_INTR_REG,              0b00000100,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_9_INTR]                    = { QSFP28_6_11_INTR_REG,              0b00001000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_10_INTR]                   = { QSFP28_6_11_INTR_REG,              0b00010000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_11_INTR]                   = { QSFP28_6_11_INTR_REG,              0b00100000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_6_EFUSE_PG]                = { QSFP28_6_11_EFUSE_PG_REG,          0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_7_EFUSE_PG]                = { QSFP28_6_11_EFUSE_PG_REG,          0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_8_EFUSE_PG]                = { QSFP28_6_11_EFUSE_PG_REG,          0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_9_EFUSE_PG]                = { QSFP28_6_11_EFUSE_PG_REG,          0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_10_EFUSE_PG]               = { QSFP28_6_11_EFUSE_PG_REG,          0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_11_EFUSE_PG]               = { QSFP28_6_11_EFUSE_PG_REG,          0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [MAC_INTR]                      = { MAC_INTR_REG,                      MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [MAIN_THERMAL_INTR]             = { MAIN_THERMAL_INTR_REG,             MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [FAN_0_PRESENT]                 = { FAN_ABS_REG,                       0b00000001,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [FAN_1_PRESENT]                 = { FAN_ABS_REG,                       0b00000010,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [FAN_2_PRESENT]                 = { FAN_ABS_REG,                       0b00000100,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [FAN_3_PRESENT]                 = { FAN_ABS_REG,                       0b00001000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [FAN_4_PRESENT]                 = { FAN_ABS_REG,                       0b00010000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [IO_OC_INTR]                    = { IO_OC_INTR_REG,                    MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_6_I2C_STUCK]               = { QSFPDD_6_11_I2C_STUCK_REG,         0b00000001,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_7_I2C_STUCK]               = { QSFPDD_6_11_I2C_STUCK_REG,         0b00000010,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_8_I2C_STUCK]               = { QSFPDD_6_11_I2C_STUCK_REG,         0b00000100,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_9_I2C_STUCK]               = { QSFPDD_6_11_I2C_STUCK_REG,         0b00001000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_10_I2C_STUCK]              = { QSFPDD_6_11_I2C_STUCK_REG,         0b00010000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_11_I2C_STUCK]              = { QSFPDD_6_11_I2C_STUCK_REG,         0b00100000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD3_I2C_STUCK]               = { CPLD3_I2C_STUCK_REG,               0b00000001,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD3_TO_CPLD1_INTR]           = { CPLD3_TO_CPLD1_INTR_REG,           0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_6_PRESENT_MASK]            = { QSFP28_6_11_ABS_MASK_REG,          0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_7_PRESENT_MASK]            = { QSFP28_6_11_ABS_MASK_REG,          0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_8_PRESENT_MASK]            = { QSFP28_6_11_ABS_MASK_REG,          0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_9_PRESENT_MASK]            = { QSFP28_6_11_ABS_MASK_REG,          0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_10_PRESENT_MASK]           = { QSFP28_6_11_ABS_MASK_REG,          0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_11_PRESENT_MASK]           = { QSFP28_6_11_ABS_MASK_REG,          0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_6_INTR_MASK]               = { QSFP28_6_11_INTR_MASK_REG,         0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_7_INTR_MASK]               = { QSFP28_6_11_INTR_MASK_REG,         0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_8_INTR_MASK]               = { QSFP28_6_11_INTR_MASK_REG,         0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_9_INTR_MASK]               = { QSFP28_6_11_INTR_MASK_REG,         0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_10_INTR_MASK]              = { QSFP28_6_11_INTR_MASK_REG,         0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_11_INTR_MASK]              = { QSFP28_6_11_INTR_MASK_REG,         0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_6_EFUSE_PG_MASK]           = { QSFP28_6_11_EFUSE_PG_MASK_REG,     0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_7_EFUSE_PG_MASK]           = { QSFP28_6_11_EFUSE_PG_MASK_REG,     0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_8_EFUSE_PG_MASK]           = { QSFP28_6_11_EFUSE_PG_MASK_REG,     0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_9_EFUSE_PG_MASK]           = { QSFP28_6_11_EFUSE_PG_MASK_REG,     0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_10_EFUSE_PG_MASK]          = { QSFP28_6_11_EFUSE_PG_MASK_REG,     0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_11_EFUSE_PG_MASK]          = { QSFP28_6_11_EFUSE_PG_MASK_REG,     0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [MAC_INTR_MASK]                 = { MAC_INTR_MASK_REG,                 MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [MAIN_THERMAL_INTR_MASK]        = { MAIN_THERMAL_INTR_MASK_REG,        MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [FAN_ABS_MASK]                  = { FAN_ABS_MASK_REG,                  MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [IO_OC_INTR_MASK]               = { IO_OC_INTR_MASK_REG,               MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_6_I2C_STUCK_MASK]          = { QSFPDD_6_11_I2C_STUCK_MASK_REG,    0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_7_I2C_STUCK_MASK]          = { QSFPDD_6_11_I2C_STUCK_MASK_REG,    0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_8_I2C_STUCK_MASK]          = { QSFPDD_6_11_I2C_STUCK_MASK_REG,    0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_9_I2C_STUCK_MASK]          = { QSFPDD_6_11_I2C_STUCK_MASK_REG,    0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_10_I2C_STUCK_MASK]         = { QSFPDD_6_11_I2C_STUCK_MASK_REG,    0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_11_I2C_STUCK_MASK]         = { QSFPDD_6_11_I2C_STUCK_MASK_REG,    0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD3_I2C_STUCK_MASK]          = { CPLD3_I2C_STUCK_MASK_REG,          0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD3_TO_CPLD1_INTR_MASK]      = { CPLD3_TO_CPLD1_INTR_MASK_REG,      0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_6_PRESENT_EVENT]           = { QSFP28_6_11_ABS_EVENT_REG,         0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_7_PRESENT_EVENT]           = { QSFP28_6_11_ABS_EVENT_REG,         0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_8_PRESENT_EVENT]           = { QSFP28_6_11_ABS_EVENT_REG,         0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_9_PRESENT_EVENT]           = { QSFP28_6_11_ABS_EVENT_REG,         0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_10_PRESENT_EVENT]          = { QSFP28_6_11_ABS_EVENT_REG,         0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_11_PRESENT_EVENT]          = { QSFP28_6_11_ABS_EVENT_REG,         0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_6_INTR_EVENT]              = { QSFP28_6_11_INTR_EVENT_REG,        0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_7_INTR_EVENT]              = { QSFP28_6_11_INTR_EVENT_REG,        0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_8_INTR_EVENT]              = { QSFP28_6_11_INTR_EVENT_REG,        0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_9_INTR_EVENT]              = { QSFP28_6_11_INTR_EVENT_REG,        0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_10_INTR_EVENT]             = { QSFP28_6_11_INTR_EVENT_REG,        0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_11_INTR_EVENT]             = { QSFP28_6_11_INTR_EVENT_REG,        0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_6_EFUSE_PG_EVENT]          = { QSFP28_6_11_EFUSE_PG_EVENT_REG,    0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_7_EFUSE_PG_EVENT]          = { QSFP28_6_11_EFUSE_PG_EVENT_REG,    0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_8_EFUSE_PG_EVENT]          = { QSFP28_6_11_EFUSE_PG_EVENT_REG,    0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_9_EFUSE_PG_EVENT]          = { QSFP28_6_11_EFUSE_PG_EVENT_REG,    0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_10_EFUSE_PG_EVENT]         = { QSFP28_6_11_EFUSE_PG_EVENT_REG,    0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_11_EFUSE_PG_EVENT]         = { QSFP28_6_11_EFUSE_PG_EVENT_REG,    0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [MAC_INTR_EVENT]                = { MAC_INTR_EVENT_REG,                MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_IS_EVENT},
    [MAIN_THERMAL_INTR_EVENT]       = { MAIN_THERMAL_INTR_EVENT_REG,       MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_IS_EVENT},
    [FAN_ABS_EVENT]                 = { FAN_ABS_EVENT_REG,                 MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_IS_EVENT},
    [IO_OC_INTR_EVENT]              = { IO_OC_INTR_EVENT_REG,              MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_6_I2C_STUCK_EVENT]         = { QSFPDD_6_11_I2C_STUCK_EVENT_REG,   0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_7_I2C_STUCK_EVENT]         = { QSFPDD_6_11_I2C_STUCK_EVENT_REG,   0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_8_I2C_STUCK_EVENT]         = { QSFPDD_6_11_I2C_STUCK_EVENT_REG,   0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_9_I2C_STUCK_EVENT]         = { QSFPDD_6_11_I2C_STUCK_EVENT_REG,   0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_10_I2C_STUCK_EVENT]        = { QSFPDD_6_11_I2C_STUCK_EVENT_REG,   0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_11_I2C_STUCK_EVENT]        = { QSFPDD_6_11_I2C_STUCK_EVENT_REG,   0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [CPLD3_I2C_STUCK_EVENT]         = { CPLD3_I2C_STUCK_EVENT_REG,         MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_IS_EVENT},
    [CPLD3_TO_CPLD1_INTR_EVENT]     = { CPLD3_TO_CPLD1_INTR_EVENT_REG,     MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_6_RESET]                   = { QSFP28_6_11_RESET_REG,             0b00000001,  DATA_0_1_INV,    REG_WP_EN,   REG_NOT_EVENT},
    [ETH_7_RESET]                   = { QSFP28_6_11_RESET_REG,             0b00000010,  DATA_0_1_INV,    REG_WP_EN,   REG_NOT_EVENT},
    [ETH_8_RESET]                   = { QSFP28_6_11_RESET_REG,             0b00000100,  DATA_0_1_INV,    REG_WP_EN,   REG_NOT_EVENT},
    [ETH_9_RESET]                   = { QSFP28_6_11_RESET_REG,             0b00001000,  DATA_0_1_INV,    REG_WP_EN,   REG_NOT_EVENT},
    [ETH_10_RESET]                  = { QSFP28_6_11_RESET_REG,             0b00010000,  DATA_0_1_INV,    REG_WP_EN,   REG_NOT_EVENT},
    [ETH_11_RESET]                  = { QSFP28_6_11_RESET_REG,             0b00100000,  DATA_0_1_INV,    REG_WP_EN,   REG_NOT_EVENT},
    [ETH_6_LPMODE]                  = { QSFP28_6_11_LPMODE_REG,            0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_7_LPMODE]                  = { QSFP28_6_11_LPMODE_REG,            0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_8_LPMODE]                  = { QSFP28_6_11_LPMODE_REG,            0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_9_LPMODE]                  = { QSFP28_6_11_LPMODE_REG,            0b00001000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_10_LPMODE]                 = { QSFP28_6_11_LPMODE_REG,            0b00010000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_11_LPMODE]                 = { QSFP28_6_11_LPMODE_REG,            0b00100000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [MAC_RESET]                     = { MAC_RESET_REG,                     MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [USB_QSPI_RESET]                = { USB_QSPI_RESET_REG,                MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [CLK_TIMING_STATUS_1]           = { CLK_TIMING_STATUS_1_REG,           MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [CLK_TIMING_STATUS_2]           = { CLK_TIMING_STATUS_2_REG,           MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [ROV_STATUS]                    = { ROV_STATUS_REG,                    MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [MISC_CONTROL]                  = { MISC_CONTROL_REG,                  MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [GNSS_CTRL]                     = { GNSS_CTRL_REG,                     MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [SYNCE_CTRL]                    = { SYNCE_CTRL_REG,                    MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [FAN_SPEED_READ_MODE]           = { FAN_SPEED_READ_MODE_REG,           0b00001111,  DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD3_PWR_STATUS]              = { CPLD3_PWR_STATUS_REG,              MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [DBG_QSFP28_6_11_ABS]           = { DBG_QSFP28_6_11_ABS_REG,           MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [DBG_QSFP28_6_11_INTR]          = { DBG_QSFP28_6_11_INTR_REG,          MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [DBG_QSFP28_6_11_EFUSE_PG]      = { DBG_QSFP28_6_11_EFUSE_PG_REG,      MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [DBG_MAC_INTR]                  = { DBG_MAC_INTR_REG,                  MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [DBG_THERMAL_INTR]              = { DBG_THERMAL_INTR_REG,              MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [DBG_MISC_INTR]                 = { DBG_MISC_INTR_REG,                 MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [DBG_SFP56_16_23_TX_FAULT]      = { DBG_SFP56_16_23_TX_FAULT_REG,      MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},

    // & cpld4
    [ETH_16_PRESENT]                = { SFP56_16_23_ABS_REG,               0b00000001,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_17_PRESENT]                = { SFP56_16_23_ABS_REG,               0b00000010,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_18_PRESENT]                = { SFP56_16_23_ABS_REG,               0b00000100,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_19_PRESENT]                = { SFP56_16_23_ABS_REG,               0b00001000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_20_PRESENT]                = { SFP56_16_23_ABS_REG,               0b00010000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_21_PRESENT]                = { SFP56_16_23_ABS_REG,               0b00100000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_22_PRESENT]                = { SFP56_16_23_ABS_REG,               0b01000000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_23_PRESENT]                = { SFP56_16_23_ABS_REG,               0b10000000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_24_PRESENT]                = { SFP56_24_31_ABS_REG,               0b00000001,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_25_PRESENT]                = { SFP56_24_31_ABS_REG,               0b00000010,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_26_PRESENT]                = { SFP56_24_31_ABS_REG,               0b00000100,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_27_PRESENT]                = { SFP56_24_31_ABS_REG,               0b00001000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_28_PRESENT]                = { SFP56_24_31_ABS_REG,               0b00010000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_29_PRESENT]                = { SFP56_24_31_ABS_REG,               0b00100000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_30_PRESENT]                = { SFP56_24_31_ABS_REG,               0b01000000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_31_PRESENT]                = { SFP56_24_31_ABS_REG,               0b10000000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_32_PRESENT]                = { SFP56_32_39_ABS_REG,               0b00000001,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_33_PRESENT]                = { SFP56_32_39_ABS_REG,               0b00000010,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_34_PRESENT]                = { SFP56_32_39_ABS_REG,               0b00000100,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_35_PRESENT]                = { SFP56_32_39_ABS_REG,               0b00001000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_36_PRESENT]                = { SFP56_32_39_ABS_REG,               0b00010000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_37_PRESENT]                = { SFP56_32_39_ABS_REG,               0b00100000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_38_PRESENT]                = { SFP56_32_39_ABS_REG,               0b01000000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_39_PRESENT]                = { SFP56_32_39_ABS_REG,               0b10000000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_16_RX_LOS]                 = { SFP56_16_23_RX_LOS_REG,            0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_17_RX_LOS]                 = { SFP56_16_23_RX_LOS_REG,            0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_18_RX_LOS]                 = { SFP56_16_23_RX_LOS_REG,            0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_19_RX_LOS]                 = { SFP56_16_23_RX_LOS_REG,            0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_20_RX_LOS]                 = { SFP56_16_23_RX_LOS_REG,            0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_21_RX_LOS]                 = { SFP56_16_23_RX_LOS_REG,            0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_22_RX_LOS]                 = { SFP56_16_23_RX_LOS_REG,            0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_23_RX_LOS]                 = { SFP56_16_23_RX_LOS_REG,            0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_24_RX_LOS]                 = { SFP56_24_31_RX_LOS_REG,            0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_25_RX_LOS]                 = { SFP56_24_31_RX_LOS_REG,            0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_26_RX_LOS]                 = { SFP56_24_31_RX_LOS_REG,            0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_27_RX_LOS]                 = { SFP56_24_31_RX_LOS_REG,            0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_28_RX_LOS]                 = { SFP56_24_31_RX_LOS_REG,            0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_29_RX_LOS]                 = { SFP56_24_31_RX_LOS_REG,            0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_30_RX_LOS]                 = { SFP56_24_31_RX_LOS_REG,            0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_31_RX_LOS]                 = { SFP56_24_31_RX_LOS_REG,            0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_32_RX_LOS]                 = { SFP56_32_39_RX_LOS_REG,            0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_33_RX_LOS]                 = { SFP56_32_39_RX_LOS_REG,            0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_34_RX_LOS]                 = { SFP56_32_39_RX_LOS_REG,            0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_35_RX_LOS]                 = { SFP56_32_39_RX_LOS_REG,            0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_36_RX_LOS]                 = { SFP56_32_39_RX_LOS_REG,            0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_37_RX_LOS]                 = { SFP56_32_39_RX_LOS_REG,            0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_38_RX_LOS]                 = { SFP56_32_39_RX_LOS_REG,            0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_39_RX_LOS]                 = { SFP56_32_39_RX_LOS_REG,            0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_16_TX_FAULT]               = { SFP56_16_23_TX_FAULT_REG,          0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_17_TX_FAULT]               = { SFP56_16_23_TX_FAULT_REG,          0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_18_TX_FAULT]               = { SFP56_16_23_TX_FAULT_REG,          0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_19_TX_FAULT]               = { SFP56_16_23_TX_FAULT_REG,          0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_20_TX_FAULT]               = { SFP56_16_23_TX_FAULT_REG,          0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_21_TX_FAULT]               = { SFP56_16_23_TX_FAULT_REG,          0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_22_TX_FAULT]               = { SFP56_16_23_TX_FAULT_REG,          0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_23_TX_FAULT]               = { SFP56_16_23_TX_FAULT_REG,          0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_24_TX_FAULT]               = { SFP56_24_31_TX_FAULT_REG,          0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_25_TX_FAULT]               = { SFP56_24_31_TX_FAULT_REG,          0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_26_TX_FAULT]               = { SFP56_24_31_TX_FAULT_REG,          0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_27_TX_FAULT]               = { SFP56_24_31_TX_FAULT_REG,          0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_28_TX_FAULT]               = { SFP56_24_31_TX_FAULT_REG,          0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_29_TX_FAULT]               = { SFP56_24_31_TX_FAULT_REG,          0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_30_TX_FAULT]               = { SFP56_24_31_TX_FAULT_REG,          0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_31_TX_FAULT]               = { SFP56_24_31_TX_FAULT_REG,          0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_32_TX_FAULT]               = { SFP56_32_39_TX_FAULT_REG,          0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_33_TX_FAULT]               = { SFP56_32_39_TX_FAULT_REG,          0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_34_TX_FAULT]               = { SFP56_32_39_TX_FAULT_REG,          0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_35_TX_FAULT]               = { SFP56_32_39_TX_FAULT_REG,          0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_36_TX_FAULT]               = { SFP56_32_39_TX_FAULT_REG,          0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_37_TX_FAULT]               = { SFP56_32_39_TX_FAULT_REG,          0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_38_TX_FAULT]               = { SFP56_32_39_TX_FAULT_REG,          0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_39_TX_FAULT]               = { SFP56_32_39_TX_FAULT_REG,          0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_16_I2C_STUCK]              = { SFP56_16_23_I2C_STUCK_REG,         0b00000001,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_17_I2C_STUCK]              = { SFP56_16_23_I2C_STUCK_REG,         0b00000010,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_18_I2C_STUCK]              = { SFP56_16_23_I2C_STUCK_REG,         0b00000100,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_19_I2C_STUCK]              = { SFP56_16_23_I2C_STUCK_REG,         0b00001000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_20_I2C_STUCK]              = { SFP56_16_23_I2C_STUCK_REG,         0b00010000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_21_I2C_STUCK]              = { SFP56_16_23_I2C_STUCK_REG,         0b00100000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_22_I2C_STUCK]              = { SFP56_16_23_I2C_STUCK_REG,         0b01000000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_23_I2C_STUCK]              = { SFP56_16_23_I2C_STUCK_REG,         0b10000000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_24_I2C_STUCK]              = { SFP56_24_31_I2C_STUCK_REG,         0b00000001,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_25_I2C_STUCK]              = { SFP56_24_31_I2C_STUCK_REG,         0b00000010,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_26_I2C_STUCK]              = { SFP56_24_31_I2C_STUCK_REG,         0b00000100,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_27_I2C_STUCK]              = { SFP56_24_31_I2C_STUCK_REG,         0b00001000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_28_I2C_STUCK]              = { SFP56_24_31_I2C_STUCK_REG,         0b00010000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_29_I2C_STUCK]              = { SFP56_24_31_I2C_STUCK_REG,         0b00100000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_30_I2C_STUCK]              = { SFP56_24_31_I2C_STUCK_REG,         0b01000000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_31_I2C_STUCK]              = { SFP56_24_31_I2C_STUCK_REG,         0b10000000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_32_I2C_STUCK]              = { SFP56_32_39_I2C_STUCK_REG,         0b00000001,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_33_I2C_STUCK]              = { SFP56_32_39_I2C_STUCK_REG,         0b00000010,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_34_I2C_STUCK]              = { SFP56_32_39_I2C_STUCK_REG,         0b00000100,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_35_I2C_STUCK]              = { SFP56_32_39_I2C_STUCK_REG,         0b00001000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_36_I2C_STUCK]              = { SFP56_32_39_I2C_STUCK_REG,         0b00010000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_37_I2C_STUCK]              = { SFP56_32_39_I2C_STUCK_REG,         0b00100000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_38_I2C_STUCK]              = { SFP56_32_39_I2C_STUCK_REG,         0b01000000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_39_I2C_STUCK]              = { SFP56_32_39_I2C_STUCK_REG,         0b10000000,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD4_TO_CPLD1_INTR]           = { CPLD4_TO_CPLD1_INTR_REG,           0b00000001,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD4_I2C_STUCK]               = { CPLD4_I2C_STUCK_REG,               0b00000001,  DATA_0_1_INV,    REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_16_PRESENT_MASK]           = { SFP56_16_23_ABS_MASK_REG,          0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_17_PRESENT_MASK]           = { SFP56_16_23_ABS_MASK_REG,          0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_18_PRESENT_MASK]           = { SFP56_16_23_ABS_MASK_REG,          0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_19_PRESENT_MASK]           = { SFP56_16_23_ABS_MASK_REG,          0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_20_PRESENT_MASK]           = { SFP56_16_23_ABS_MASK_REG,          0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_21_PRESENT_MASK]           = { SFP56_16_23_ABS_MASK_REG,          0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_22_PRESENT_MASK]           = { SFP56_16_23_ABS_MASK_REG,          0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_23_PRESENT_MASK]           = { SFP56_16_23_ABS_MASK_REG,          0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_24_PRESENT_MASK]           = { SFP56_24_31_ABS_MASK_REG,          0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_25_PRESENT_MASK]           = { SFP56_24_31_ABS_MASK_REG,          0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_26_PRESENT_MASK]           = { SFP56_24_31_ABS_MASK_REG,          0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_27_PRESENT_MASK]           = { SFP56_24_31_ABS_MASK_REG,          0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_28_PRESENT_MASK]           = { SFP56_24_31_ABS_MASK_REG,          0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_29_PRESENT_MASK]           = { SFP56_24_31_ABS_MASK_REG,          0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_30_PRESENT_MASK]           = { SFP56_24_31_ABS_MASK_REG,          0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_31_PRESENT_MASK]           = { SFP56_24_31_ABS_MASK_REG,          0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_32_PRESENT_MASK]           = { SFP56_32_39_ABS_MASK_REG,          0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_33_PRESENT_MASK]           = { SFP56_32_39_ABS_MASK_REG,          0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_34_PRESENT_MASK]           = { SFP56_32_39_ABS_MASK_REG,          0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_35_PRESENT_MASK]           = { SFP56_32_39_ABS_MASK_REG,          0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_36_PRESENT_MASK]           = { SFP56_32_39_ABS_MASK_REG,          0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_37_PRESENT_MASK]           = { SFP56_32_39_ABS_MASK_REG,          0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_38_PRESENT_MASK]           = { SFP56_32_39_ABS_MASK_REG,          0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_39_PRESENT_MASK]           = { SFP56_32_39_ABS_MASK_REG,          0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_16_RX_LOS_MASK]            = { SFP56_16_23_RX_LOS_MASK_REG,       0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_17_RX_LOS_MASK]            = { SFP56_16_23_RX_LOS_MASK_REG,       0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_18_RX_LOS_MASK]            = { SFP56_16_23_RX_LOS_MASK_REG,       0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_19_RX_LOS_MASK]            = { SFP56_16_23_RX_LOS_MASK_REG,       0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_20_RX_LOS_MASK]            = { SFP56_16_23_RX_LOS_MASK_REG,       0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_21_RX_LOS_MASK]            = { SFP56_16_23_RX_LOS_MASK_REG,       0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_22_RX_LOS_MASK]            = { SFP56_16_23_RX_LOS_MASK_REG,       0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_23_RX_LOS_MASK]            = { SFP56_16_23_RX_LOS_MASK_REG,       0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_24_RX_LOS_MASK]            = { SFP56_24_31_RX_LOS_MASK_REG,       0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_25_RX_LOS_MASK]            = { SFP56_24_31_RX_LOS_MASK_REG,       0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_26_RX_LOS_MASK]            = { SFP56_24_31_RX_LOS_MASK_REG,       0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_27_RX_LOS_MASK]            = { SFP56_24_31_RX_LOS_MASK_REG,       0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_28_RX_LOS_MASK]            = { SFP56_24_31_RX_LOS_MASK_REG,       0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_29_RX_LOS_MASK]            = { SFP56_24_31_RX_LOS_MASK_REG,       0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_30_RX_LOS_MASK]            = { SFP56_24_31_RX_LOS_MASK_REG,       0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_31_RX_LOS_MASK]            = { SFP56_24_31_RX_LOS_MASK_REG,       0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_32_RX_LOS_MASK]            = { SFP56_32_39_RX_LOS_MASK_REG,       0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_33_RX_LOS_MASK]            = { SFP56_32_39_RX_LOS_MASK_REG,       0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_34_RX_LOS_MASK]            = { SFP56_32_39_RX_LOS_MASK_REG,       0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_35_RX_LOS_MASK]            = { SFP56_32_39_RX_LOS_MASK_REG,       0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_36_RX_LOS_MASK]            = { SFP56_32_39_RX_LOS_MASK_REG,       0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_37_RX_LOS_MASK]            = { SFP56_32_39_RX_LOS_MASK_REG,       0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_38_RX_LOS_MASK]            = { SFP56_32_39_RX_LOS_MASK_REG,       0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_39_RX_LOS_MASK]            = { SFP56_32_39_RX_LOS_MASK_REG,       0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_16_TX_FAULT_MASK]          = { SFP56_16_23_TX_FAULT_MASK_REG,     0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_17_TX_FAULT_MASK]          = { SFP56_16_23_TX_FAULT_MASK_REG,     0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_18_TX_FAULT_MASK]          = { SFP56_16_23_TX_FAULT_MASK_REG,     0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_19_TX_FAULT_MASK]          = { SFP56_16_23_TX_FAULT_MASK_REG,     0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_20_TX_FAULT_MASK]          = { SFP56_16_23_TX_FAULT_MASK_REG,     0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_21_TX_FAULT_MASK]          = { SFP56_16_23_TX_FAULT_MASK_REG,     0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_22_TX_FAULT_MASK]          = { SFP56_16_23_TX_FAULT_MASK_REG,     0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_23_TX_FAULT_MASK]          = { SFP56_16_23_TX_FAULT_MASK_REG,     0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_24_TX_FAULT_MASK]          = { SFP56_24_31_TX_FAULT_MASK_REG,     0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_25_TX_FAULT_MASK]          = { SFP56_24_31_TX_FAULT_MASK_REG,     0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_26_TX_FAULT_MASK]          = { SFP56_24_31_TX_FAULT_MASK_REG,     0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_27_TX_FAULT_MASK]          = { SFP56_24_31_TX_FAULT_MASK_REG,     0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_28_TX_FAULT_MASK]          = { SFP56_24_31_TX_FAULT_MASK_REG,     0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_29_TX_FAULT_MASK]          = { SFP56_24_31_TX_FAULT_MASK_REG,     0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_30_TX_FAULT_MASK]          = { SFP56_24_31_TX_FAULT_MASK_REG,     0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_31_TX_FAULT_MASK]          = { SFP56_24_31_TX_FAULT_MASK_REG,     0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_32_TX_FAULT_MASK]          = { SFP56_32_39_TX_FAULT_MASK_REG,     0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_33_TX_FAULT_MASK]          = { SFP56_32_39_TX_FAULT_MASK_REG,     0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_34_TX_FAULT_MASK]          = { SFP56_32_39_TX_FAULT_MASK_REG,     0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_35_TX_FAULT_MASK]          = { SFP56_32_39_TX_FAULT_MASK_REG,     0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_36_TX_FAULT_MASK]          = { SFP56_32_39_TX_FAULT_MASK_REG,     0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_37_TX_FAULT_MASK]          = { SFP56_32_39_TX_FAULT_MASK_REG,     0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_38_TX_FAULT_MASK]          = { SFP56_32_39_TX_FAULT_MASK_REG,     0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_39_TX_FAULT_MASK]          = { SFP56_32_39_TX_FAULT_MASK_REG,     0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_16_I2C_STUCK_MASK]         = { SFP56_16_23_I2C_STUCK_MASK_REG,    0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_17_I2C_STUCK_MASK]         = { SFP56_16_23_I2C_STUCK_MASK_REG,    0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_18_I2C_STUCK_MASK]         = { SFP56_16_23_I2C_STUCK_MASK_REG,    0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_19_I2C_STUCK_MASK]         = { SFP56_16_23_I2C_STUCK_MASK_REG,    0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_20_I2C_STUCK_MASK]         = { SFP56_16_23_I2C_STUCK_MASK_REG,    0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_21_I2C_STUCK_MASK]         = { SFP56_16_23_I2C_STUCK_MASK_REG,    0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_22_I2C_STUCK_MASK]         = { SFP56_16_23_I2C_STUCK_MASK_REG,    0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_23_I2C_STUCK_MASK]         = { SFP56_16_23_I2C_STUCK_MASK_REG,    0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_24_I2C_STUCK_MASK]         = { SFP56_24_31_I2C_STUCK_MASK_REG,    0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_25_I2C_STUCK_MASK]         = { SFP56_24_31_I2C_STUCK_MASK_REG,    0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_26_I2C_STUCK_MASK]         = { SFP56_24_31_I2C_STUCK_MASK_REG,    0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_27_I2C_STUCK_MASK]         = { SFP56_24_31_I2C_STUCK_MASK_REG,    0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_28_I2C_STUCK_MASK]         = { SFP56_24_31_I2C_STUCK_MASK_REG,    0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_29_I2C_STUCK_MASK]         = { SFP56_24_31_I2C_STUCK_MASK_REG,    0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_30_I2C_STUCK_MASK]         = { SFP56_24_31_I2C_STUCK_MASK_REG,    0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_31_I2C_STUCK_MASK]         = { SFP56_24_31_I2C_STUCK_MASK_REG,    0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_32_I2C_STUCK_MASK]         = { SFP56_32_39_I2C_STUCK_MASK_REG,    0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_33_I2C_STUCK_MASK]         = { SFP56_32_39_I2C_STUCK_MASK_REG,    0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_34_I2C_STUCK_MASK]         = { SFP56_32_39_I2C_STUCK_MASK_REG,    0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_35_I2C_STUCK_MASK]         = { SFP56_32_39_I2C_STUCK_MASK_REG,    0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_36_I2C_STUCK_MASK]         = { SFP56_32_39_I2C_STUCK_MASK_REG,    0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_37_I2C_STUCK_MASK]         = { SFP56_32_39_I2C_STUCK_MASK_REG,    0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_38_I2C_STUCK_MASK]         = { SFP56_32_39_I2C_STUCK_MASK_REG,    0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_39_I2C_STUCK_MASK]         = { SFP56_32_39_I2C_STUCK_MASK_REG,    0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD4_TO_CPLD1_INTR_MASK]      = { CPLD4_TO_CPLD1_INTR_MASK_REG,      0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD4_I2C_STUCK_MASK]          = { CPLD4_I2C_STUCK_MASK_REG,          0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_NOT_EVENT},
    [ETH_16_PRESENT_EVENT]          = { SFP56_16_23_ABS_EVENT_REG,         0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_17_PRESENT_EVENT]          = { SFP56_16_23_ABS_EVENT_REG,         0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_18_PRESENT_EVENT]          = { SFP56_16_23_ABS_EVENT_REG,         0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_19_PRESENT_EVENT]          = { SFP56_16_23_ABS_EVENT_REG,         0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_20_PRESENT_EVENT]          = { SFP56_16_23_ABS_EVENT_REG,         0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_21_PRESENT_EVENT]          = { SFP56_16_23_ABS_EVENT_REG,         0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_22_PRESENT_EVENT]          = { SFP56_16_23_ABS_EVENT_REG,         0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_23_PRESENT_EVENT]          = { SFP56_16_23_ABS_EVENT_REG,         0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_24_PRESENT_EVENT]          = { SFP56_24_31_ABS_EVENT_REG,         0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_25_PRESENT_EVENT]          = { SFP56_24_31_ABS_EVENT_REG,         0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_26_PRESENT_EVENT]          = { SFP56_24_31_ABS_EVENT_REG,         0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_27_PRESENT_EVENT]          = { SFP56_24_31_ABS_EVENT_REG,         0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_28_PRESENT_EVENT]          = { SFP56_24_31_ABS_EVENT_REG,         0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_29_PRESENT_EVENT]          = { SFP56_24_31_ABS_EVENT_REG,         0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_30_PRESENT_EVENT]          = { SFP56_24_31_ABS_EVENT_REG,         0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_31_PRESENT_EVENT]          = { SFP56_24_31_ABS_EVENT_REG,         0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_32_PRESENT_EVENT]          = { SFP56_32_39_ABS_EVENT_REG,         0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_33_PRESENT_EVENT]          = { SFP56_32_39_ABS_EVENT_REG,         0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_34_PRESENT_EVENT]          = { SFP56_32_39_ABS_EVENT_REG,         0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_35_PRESENT_EVENT]          = { SFP56_32_39_ABS_EVENT_REG,         0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_36_PRESENT_EVENT]          = { SFP56_32_39_ABS_EVENT_REG,         0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_37_PRESENT_EVENT]          = { SFP56_32_39_ABS_EVENT_REG,         0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_38_PRESENT_EVENT]          = { SFP56_32_39_ABS_EVENT_REG,         0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_39_PRESENT_EVENT]          = { SFP56_32_39_ABS_EVENT_REG,         0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_16_RX_LOS_EVENT]           = { SFP56_16_23_RX_LOS_EVENT_REG,      0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_17_RX_LOS_EVENT]           = { SFP56_16_23_RX_LOS_EVENT_REG,      0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_18_RX_LOS_EVENT]           = { SFP56_16_23_RX_LOS_EVENT_REG,      0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_19_RX_LOS_EVENT]           = { SFP56_16_23_RX_LOS_EVENT_REG,      0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_20_RX_LOS_EVENT]           = { SFP56_16_23_RX_LOS_EVENT_REG,      0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_21_RX_LOS_EVENT]           = { SFP56_16_23_RX_LOS_EVENT_REG,      0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_22_RX_LOS_EVENT]           = { SFP56_16_23_RX_LOS_EVENT_REG,      0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_23_RX_LOS_EVENT]           = { SFP56_16_23_RX_LOS_EVENT_REG,      0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_24_RX_LOS_EVENT]           = { SFP56_24_31_RX_LOS_EVENT_REG,      0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_25_RX_LOS_EVENT]           = { SFP56_24_31_RX_LOS_EVENT_REG,      0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_26_RX_LOS_EVENT]           = { SFP56_24_31_RX_LOS_EVENT_REG,      0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_27_RX_LOS_EVENT]           = { SFP56_24_31_RX_LOS_EVENT_REG,      0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_28_RX_LOS_EVENT]           = { SFP56_24_31_RX_LOS_EVENT_REG,      0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_29_RX_LOS_EVENT]           = { SFP56_24_31_RX_LOS_EVENT_REG,      0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_30_RX_LOS_EVENT]           = { SFP56_24_31_RX_LOS_EVENT_REG,      0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_31_RX_LOS_EVENT]           = { SFP56_24_31_RX_LOS_EVENT_REG,      0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_32_RX_LOS_EVENT]           = { SFP56_32_39_RX_LOS_EVENT_REG,      0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_33_RX_LOS_EVENT]           = { SFP56_32_39_RX_LOS_EVENT_REG,      0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_34_RX_LOS_EVENT]           = { SFP56_32_39_RX_LOS_EVENT_REG,      0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_35_RX_LOS_EVENT]           = { SFP56_32_39_RX_LOS_EVENT_REG,      0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_36_RX_LOS_EVENT]           = { SFP56_32_39_RX_LOS_EVENT_REG,      0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_37_RX_LOS_EVENT]           = { SFP56_32_39_RX_LOS_EVENT_REG,      0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_38_RX_LOS_EVENT]           = { SFP56_32_39_RX_LOS_EVENT_REG,      0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_39_RX_LOS_EVENT]           = { SFP56_32_39_RX_LOS_EVENT_REG,      0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_16_TX_FAULT_EVENT]         = { SFP56_16_23_TX_FAULT_EVENT_REG,    0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_17_TX_FAULT_EVENT]         = { SFP56_16_23_TX_FAULT_EVENT_REG,    0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_18_TX_FAULT_EVENT]         = { SFP56_16_23_TX_FAULT_EVENT_REG,    0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_19_TX_FAULT_EVENT]         = { SFP56_16_23_TX_FAULT_EVENT_REG,    0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_20_TX_FAULT_EVENT]         = { SFP56_16_23_TX_FAULT_EVENT_REG,    0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_21_TX_FAULT_EVENT]         = { SFP56_16_23_TX_FAULT_EVENT_REG,    0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_22_TX_FAULT_EVENT]         = { SFP56_16_23_TX_FAULT_EVENT_REG,    0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_23_TX_FAULT_EVENT]         = { SFP56_16_23_TX_FAULT_EVENT_REG,    0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_24_TX_FAULT_EVENT]         = { SFP56_24_31_TX_FAULT_EVENT_REG,    0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_25_TX_FAULT_EVENT]         = { SFP56_24_31_TX_FAULT_EVENT_REG,    0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_26_TX_FAULT_EVENT]         = { SFP56_24_31_TX_FAULT_EVENT_REG,    0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_27_TX_FAULT_EVENT]         = { SFP56_24_31_TX_FAULT_EVENT_REG,    0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_28_TX_FAULT_EVENT]         = { SFP56_24_31_TX_FAULT_EVENT_REG,    0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_29_TX_FAULT_EVENT]         = { SFP56_24_31_TX_FAULT_EVENT_REG,    0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_30_TX_FAULT_EVENT]         = { SFP56_24_31_TX_FAULT_EVENT_REG,    0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_31_TX_FAULT_EVENT]         = { SFP56_24_31_TX_FAULT_EVENT_REG,    0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_32_TX_FAULT_EVENT]         = { SFP56_32_39_TX_FAULT_EVENT_REG,    0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_33_TX_FAULT_EVENT]         = { SFP56_32_39_TX_FAULT_EVENT_REG,    0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_34_TX_FAULT_EVENT]         = { SFP56_32_39_TX_FAULT_EVENT_REG,    0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_35_TX_FAULT_EVENT]         = { SFP56_32_39_TX_FAULT_EVENT_REG,    0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_36_TX_FAULT_EVENT]         = { SFP56_32_39_TX_FAULT_EVENT_REG,    0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_37_TX_FAULT_EVENT]         = { SFP56_32_39_TX_FAULT_EVENT_REG,    0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_38_TX_FAULT_EVENT]         = { SFP56_32_39_TX_FAULT_EVENT_REG,    0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_39_TX_FAULT_EVENT]         = { SFP56_32_39_TX_FAULT_EVENT_REG,    0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_16_I2C_STUCK_EVENT]        = { SFP56_16_23_I2C_STUCK_EVENT_REG,   0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_17_I2C_STUCK_EVENT]        = { SFP56_16_23_I2C_STUCK_EVENT_REG,   0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_18_I2C_STUCK_EVENT]        = { SFP56_16_23_I2C_STUCK_EVENT_REG,   0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_19_I2C_STUCK_EVENT]        = { SFP56_16_23_I2C_STUCK_EVENT_REG,   0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_20_I2C_STUCK_EVENT]        = { SFP56_16_23_I2C_STUCK_EVENT_REG,   0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_21_I2C_STUCK_EVENT]        = { SFP56_16_23_I2C_STUCK_EVENT_REG,   0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_22_I2C_STUCK_EVENT]        = { SFP56_16_23_I2C_STUCK_EVENT_REG,   0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_23_I2C_STUCK_EVENT]        = { SFP56_16_23_I2C_STUCK_EVENT_REG,   0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_24_I2C_STUCK_EVENT]        = { SFP56_24_31_I2C_STUCK_EVENT_REG,   0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_25_I2C_STUCK_EVENT]        = { SFP56_24_31_I2C_STUCK_EVENT_REG,   0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_26_I2C_STUCK_EVENT]        = { SFP56_24_31_I2C_STUCK_EVENT_REG,   0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_27_I2C_STUCK_EVENT]        = { SFP56_24_31_I2C_STUCK_EVENT_REG,   0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_28_I2C_STUCK_EVENT]        = { SFP56_24_31_I2C_STUCK_EVENT_REG,   0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_29_I2C_STUCK_EVENT]        = { SFP56_24_31_I2C_STUCK_EVENT_REG,   0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_30_I2C_STUCK_EVENT]        = { SFP56_24_31_I2C_STUCK_EVENT_REG,   0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_31_I2C_STUCK_EVENT]        = { SFP56_24_31_I2C_STUCK_EVENT_REG,   0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_32_I2C_STUCK_EVENT]        = { SFP56_32_39_I2C_STUCK_EVENT_REG,   0b00000001,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_33_I2C_STUCK_EVENT]        = { SFP56_32_39_I2C_STUCK_EVENT_REG,   0b00000010,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_34_I2C_STUCK_EVENT]        = { SFP56_32_39_I2C_STUCK_EVENT_REG,   0b00000100,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_35_I2C_STUCK_EVENT]        = { SFP56_32_39_I2C_STUCK_EVENT_REG,   0b00001000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_36_I2C_STUCK_EVENT]        = { SFP56_32_39_I2C_STUCK_EVENT_REG,   0b00010000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_37_I2C_STUCK_EVENT]        = { SFP56_32_39_I2C_STUCK_EVENT_REG,   0b00100000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_38_I2C_STUCK_EVENT]        = { SFP56_32_39_I2C_STUCK_EVENT_REG,   0b01000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_39_I2C_STUCK_EVENT]        = { SFP56_32_39_I2C_STUCK_EVENT_REG,   0b10000000,  DATA_0_1,        REG_WP_DIS,  REG_IS_EVENT},
    [CPLD4_TO_CPLD1_INTR_EVENT]     = { CPLD4_TO_CPLD1_INTR_EVENT_REG,     MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_IS_EVENT},
    [CPLD4_I2C_STUCK_EVENT]         = { CPLD4_I2C_STUCK_EVENT_REG,         MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_IS_EVENT},
    [ETH_16_TX_DISABLE]             = { SFP56_16_23_TX_DISABLE_REG,        0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_17_TX_DISABLE]             = { SFP56_16_23_TX_DISABLE_REG,        0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_18_TX_DISABLE]             = { SFP56_16_23_TX_DISABLE_REG,        0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_19_TX_DISABLE]             = { SFP56_16_23_TX_DISABLE_REG,        0b00001000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_20_TX_DISABLE]             = { SFP56_16_23_TX_DISABLE_REG,        0b00010000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_21_TX_DISABLE]             = { SFP56_16_23_TX_DISABLE_REG,        0b00100000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_22_TX_DISABLE]             = { SFP56_16_23_TX_DISABLE_REG,        0b01000000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_23_TX_DISABLE]             = { SFP56_16_23_TX_DISABLE_REG,        0b10000000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_24_TX_DISABLE]             = { SFP56_24_31_TX_DISABLE_REG,        0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_25_TX_DISABLE]             = { SFP56_24_31_TX_DISABLE_REG,        0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_26_TX_DISABLE]             = { SFP56_24_31_TX_DISABLE_REG,        0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_27_TX_DISABLE]             = { SFP56_24_31_TX_DISABLE_REG,        0b00001000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_28_TX_DISABLE]             = { SFP56_24_31_TX_DISABLE_REG,        0b00010000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_29_TX_DISABLE]             = { SFP56_24_31_TX_DISABLE_REG,        0b00100000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_30_TX_DISABLE]             = { SFP56_24_31_TX_DISABLE_REG,        0b01000000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_31_TX_DISABLE]             = { SFP56_24_31_TX_DISABLE_REG,        0b10000000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_32_TX_DISABLE]             = { SFP56_32_39_TX_DISABLE_REG,        0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_33_TX_DISABLE]             = { SFP56_32_39_TX_DISABLE_REG,        0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_34_TX_DISABLE]             = { SFP56_32_39_TX_DISABLE_REG,        0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_35_TX_DISABLE]             = { SFP56_32_39_TX_DISABLE_REG,        0b00001000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_36_TX_DISABLE]             = { SFP56_32_39_TX_DISABLE_REG,        0b00010000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_37_TX_DISABLE]             = { SFP56_32_39_TX_DISABLE_REG,        0b00100000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_38_TX_DISABLE]             = { SFP56_32_39_TX_DISABLE_REG,        0b01000000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_39_TX_DISABLE]             = { SFP56_32_39_TX_DISABLE_REG,        0b10000000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_16_RATE_SEL]               = { SFP56_16_23_RATE_SEL_REG,          0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_17_RATE_SEL]               = { SFP56_16_23_RATE_SEL_REG,          0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_18_RATE_SEL]               = { SFP56_16_23_RATE_SEL_REG,          0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_19_RATE_SEL]               = { SFP56_16_23_RATE_SEL_REG,          0b00001000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_20_RATE_SEL]               = { SFP56_16_23_RATE_SEL_REG,          0b00010000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_21_RATE_SEL]               = { SFP56_16_23_RATE_SEL_REG,          0b00100000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_22_RATE_SEL]               = { SFP56_16_23_RATE_SEL_REG,          0b01000000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_23_RATE_SEL]               = { SFP56_16_23_RATE_SEL_REG,          0b10000000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_24_RATE_SEL]               = { SFP56_24_31_RATE_SEL_REG,          0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_25_RATE_SEL]               = { SFP56_24_31_RATE_SEL_REG,          0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_26_RATE_SEL]               = { SFP56_24_31_RATE_SEL_REG,          0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_27_RATE_SEL]               = { SFP56_24_31_RATE_SEL_REG,          0b00001000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_28_RATE_SEL]               = { SFP56_24_31_RATE_SEL_REG,          0b00010000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_29_RATE_SEL]               = { SFP56_24_31_RATE_SEL_REG,          0b00100000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_30_RATE_SEL]               = { SFP56_24_31_RATE_SEL_REG,          0b01000000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_31_RATE_SEL]               = { SFP56_24_31_RATE_SEL_REG,          0b10000000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_32_RATE_SEL]               = { SFP56_32_39_RATE_SEL_REG,          0b00000001,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_33_RATE_SEL]               = { SFP56_32_39_RATE_SEL_REG,          0b00000010,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_34_RATE_SEL]               = { SFP56_32_39_RATE_SEL_REG,          0b00000100,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_35_RATE_SEL]               = { SFP56_32_39_RATE_SEL_REG,          0b00001000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_36_RATE_SEL]               = { SFP56_32_39_RATE_SEL_REG,          0b00010000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_37_RATE_SEL]               = { SFP56_32_39_RATE_SEL_REG,          0b00100000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_38_RATE_SEL]               = { SFP56_32_39_RATE_SEL_REG,          0b01000000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [ETH_39_RATE_SEL]               = { SFP56_32_39_RATE_SEL_REG,          0b10000000,  DATA_0_1,        REG_WP_EN,   REG_NOT_EVENT},
    [CPLD4_PWR_STATUS_0]            = { CPLD4_PWR_STATUS_0_REG,            MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD4_PWR_STATUS_1]            = { CPLD4_PWR_STATUS_1_REG,            MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD4_PWR_STATUS_2]            = { CPLD4_PWR_STATUS_2_REG,            MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [CPLD4_PWR_STATUS_3]            = { CPLD4_PWR_STATUS_3_REG,            MASK_ALL,    DATA_HEX,        REG_WP_DIS,  REG_NOT_EVENT},
    [DBG_SFP56_16_23_ABS]           = { DBG_SFP56_16_23_ABS_REG,           MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [DBG_SFP56_24_31_ABS]           = { DBG_SFP56_24_31_ABS_REG,           MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [DBG_SFP56_32_39_ABS]           = { DBG_SFP56_32_39_ABS_REG,           MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [DBG_SFP56_16_23_RX_LOS]        = { DBG_SFP56_16_23_RX_LOS_REG,        MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [DBG_SFP56_24_31_RX_LOS]        = { DBG_SFP56_24_31_RX_LOS_REG,        MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [DBG_SFP56_32_39_RX_LOS]        = { DBG_SFP56_32_39_RX_LOS_REG,        MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [DBG_SFP56_16_23_TX_FAULT]      = { DBG_SFP56_16_23_TX_FAULT_REG,      MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [DBG_SFP56_24_31_TX_FAULT]      = { DBG_SFP56_24_31_TX_FAULT_REG,      MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},
    [DBG_SFP56_32_39_TX_FAULT]      = { DBG_SFP56_32_39_TX_FAULT_REG,      MASK_ALL,    DATA_HEX,        REG_WP_EN,   REG_NOT_EVENT},


    // MUX
    [IDLE_STATE]                   =  {NONE_REG,                           MASK_NONE,   DATA_UNK,        REG_WP_DIS,  REG_NOT_EVENT},
    /******************************************************************************
    * BSP DEBUG                                                                    *
    ******************************************************************************/
    //BSP DEBUG
    [BSP_DEBUG]                   = { NONE_REG,                            MASK_ALL,    DATA_DEC,        REG_WP_DIS,  REG_NOT_EVENT},
    [BSP_WP_ACCESS_COUNT]         = { NONE_REG,                            MASK_ALL,    DATA_DEC,        REG_WP_DIS,  REG_NOT_EVENT},
};
/* CPLD sysfs attributes hook functions  */
static ssize_t cpld_show(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t cpld_store(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
int _cpld_reg_read(struct device *dev, u8 reg, u8 mask);
int _cpld_reg_read_nolock(struct device *dev, u8 reg, u8 mask);
int _cpld_reg_write_nolock(struct device *dev, u8 reg, u8 reg_val);
int _cpld_event_read(struct device *dev, u8 reg, u8 mask);
static ssize_t _cpld_reg_write_with_protect(struct device *dev, u8 reg, u8 reg_val);
static ssize_t cpld_reg_read(struct device *dev, char *buf, attr_reg_map_t attr_reg);
static ssize_t cpld_reg_write(struct device *dev, const char *buf, size_t count, attr_reg_map_t attr_reg);
static ssize_t bsp_read(char *buf, char *str);
static ssize_t bsp_write(const char *buf, char *str, size_t str_len, size_t count);
static ssize_t bsp_callback_show(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t bsp_callback_store(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t version_h_show(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t led_show(struct device *dev,
        struct device_attribute *da, char *buf);
static ssize_t led_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);


static LIST_HEAD(cpld_client_list);  /* client list for cpld */
static struct mutex list_lock;  /* mutex for client list */

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

/* CPLD device id and data */
static const struct i2c_device_id cpld_id[] = {
    { "s9620_40dg_cpld1",  cpld1 },
    { "s9620_40dg_cpld2",  cpld2 },
    { "s9620_40dg_cpld3",  cpld3 },
    { "s9620_40dg_cpld4",  cpld4 },
    {}
};

static unsigned int wp_access_count = 0;

static char bsp_debug[2]="0";
static u8 enable_log_read=LOG_DISABLE;
static u8 enable_log_write=LOG_DISABLE;

/* Addresses scanned for cpld */
static const unsigned short cpld_i2c_addr[] = { 0x26, 0x27, 0x25, 0x24 , I2C_CLIENT_END };

/* define all support register access of cpld in attribute */

/* CPLD sysfs attributes index  */
static SENSOR_DEVICE_ATTR_RO(cpld_minor_ver,               cpld,           CPLD_MINOR_VER);
static SENSOR_DEVICE_ATTR_RO(cpld_major_ver,               cpld,           CPLD_MAJOR_VER);
static SENSOR_DEVICE_ATTR_RO(cpld_id,                      cpld,           CPLD_ID);
static SENSOR_DEVICE_ATTR_RO(cpld_build_ver,               cpld,           CPLD_BUILD_VER);
static SENSOR_DEVICE_ATTR_RO(cpld_chip_type,               cpld,           CPLD_CHIP_TYPE);
static SENSOR_DEVICE_ATTR_RO(cpld_version_h,               version_h,      CPLD_VERSION_H);
static SENSOR_DEVICE_ATTR_RW(event_detect_ctrl,            cpld,           EVENT_DETECT_CTRL);
static SENSOR_DEVICE_ATTR_RW(module_reset,                 cpld,           MODULE_RESET);
static SENSOR_DEVICE_ATTR_RO(cpld_test,                    cpld,           CPLD_TEST);
static SENSOR_DEVICE_ATTR_RW(cpld_i2c_control,             cpld,           CPLD_I2C_CONTROL);
static SENSOR_DEVICE_ATTR_RO(cpld_i2c_relay,               cpld,           CPLD_I2C_RELAY);
static SENSOR_DEVICE_ATTR_RO(brd_sku_id,                   cpld,           BRD_SKU_ID);
static SENSOR_DEVICE_ATTR_RO(brd_hw_id,                    cpld,           BRD_HW_ID);
static SENSOR_DEVICE_ATTR_RO(brd_deph_id,                  cpld,           BRD_DEPH_ID);
static SENSOR_DEVICE_ATTR_RO(brd_build_id,                 cpld,           BRD_BUILD_ID);
static SENSOR_DEVICE_ATTR_RO(brd_id_type,                  cpld,           BRD_ID_TYPE);
static SENSOR_DEVICE_ATTR_RO(cpld_board_ext_id,            cpld,           CPLD_BOARD_EXT_ID);
static SENSOR_DEVICE_ATTR_RO(gddr6_id,                     cpld,           GDDR6_ID);
static SENSOR_DEVICE_ATTR_RO(gddr6_id_func,                cpld,           GDDR6_ID_FUNC);
static SENSOR_DEVICE_ATTR_RO(clk_ptp_intr,                 cpld,           CLK_PTP_INTR);
static SENSOR_DEVICE_ATTR_RO(phy_intr,                     cpld,           PHY_INTR);
static SENSOR_DEVICE_ATTR_RO(top_brd_cpld_fru_intr,        cpld,           TOP_BRD_CPLD_FRU_INTR);
static SENSOR_DEVICE_ATTR_RO(psu0_intr,                    cpld,           PSU0_INTR);
static SENSOR_DEVICE_ATTR_RO(psu1_intr,                    cpld,           PSU1_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld2_intr,                   cpld,           CPLD2_INTR);
static SENSOR_DEVICE_ATTR_RO(main_brd_cpld_intr,           cpld,           MAIN_BRD_CPLD_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld3_intr,                   cpld,           CPLD3_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld4_intr,                   cpld,           CPLD4_INTR);
static SENSOR_DEVICE_ATTR_RO(mb_eth_intr,                  cpld,           MB_ETH_INTR);
static SENSOR_DEVICE_ATTR_RO(fan_intr,                     cpld,           FAN_INTR);
static SENSOR_DEVICE_ATTR_RO(thermal_intr,                 cpld,           THERMAL_INTR);
static SENSOR_DEVICE_ATTR_RO(usb_ssd_intr,                 cpld,           USB_SSD_INTR);
static SENSOR_DEVICE_ATTR_RO(cpu_nmi_intr,                 cpld,           CPU_NMI_INTR);
static SENSOR_DEVICE_ATTR_RO(out_status_intr,              cpld,           OUT_STATUS_INTR);
static SENSOR_DEVICE_ATTR_RW(clk_ptp_intr_mask,            cpld,           CLK_PTP_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(phy_intr_mask,                cpld,           PHY_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(top_brd_cpld_fru_intr_mask,   cpld,           TOP_BRD_CPLD_FRU_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(psu0_intr_mask,               cpld,           PSU0_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(psu1_intr_mask,               cpld,           PSU1_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld2_intr_mask,              cpld,           CPLD2_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld2_io_intr_mask,           cpld,           CPLD2_IO_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(main_brd_cpld_intr_mask,      cpld,           MAIN_BRD_CPLD_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld3_intr_mask,              cpld,           CPLD3_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld4_intr_mask,              cpld,           CPLD4_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(mb_eth_intr_mask,             cpld,           MB_ETH_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(mb_ptp_intr_mask,             cpld,           MB_PTP_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(fan_intr_mask,                cpld,           FAN_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(thermal_intr_mask,            cpld,           THERMAL_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(usb_ssd_intr_mask,            cpld,           USB_SSD_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(cpu_nmi_intr_mask,            cpld,           CPU_NMI_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(out_status_intr_mask,         cpld,           OUT_STATUS_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(clk_ptp_intr_event,           cpld,           CLK_PTP_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(phy_intr_event,               cpld,           PHY_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(top_brd_cpld_fru_intr_event,  cpld,           TOP_BRD_CPLD_FRU_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(main_brd_cpld_intr_event,     cpld,           MAIN_BRD_CPLD_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(thermal_intr_event,           cpld,           THERMAL_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(usb_ssd_intr_event,           cpld,           USB_SSD_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpu_nmi_intr_event,           cpld,           CPU_NMI_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(out_status_intr_event,        cpld,           OUT_STATUS_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RW(btn_fp_reset,                 cpld,           BTN_FP_RESET);
static SENSOR_DEVICE_ATTR_RW(spi_bios_reset,               cpld,           SPI_BIOS_RESET);
static SENSOR_DEVICE_ATTR_RW(cpu_board_ctrl,               cpld,           CPU_BOARD_CTRL);
static SENSOR_DEVICE_ATTR_RW(rgb_1_reset,                  cpld,           RGB_1_RESET);
static SENSOR_DEVICE_ATTR_RW(rgb_0_reset,                  cpld,           RGB_0_RESET);
static SENSOR_DEVICE_ATTR_RW(bmc_lpc_reset,                cpld,           BMC_LPC_RESET);
static SENSOR_DEVICE_ATTR_RW(bmc_pcie_reset,               cpld,           BMC_PCIE_RESET);
static SENSOR_DEVICE_ATTR_RW(cpld_to_bmc_sys_reset,        cpld,           CPLD_TO_BMC_SYS_RESET);
static SENSOR_DEVICE_ATTR_RW(cpld_to_cpu_reset,            cpld,           CPLD_TO_CPU_RESET);
static SENSOR_DEVICE_ATTR_RW(usb_pwr_en,                   cpld,           USB_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(usb_sie_reset,                cpld,           USB_SIE_RESET);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_sys_reset,            cpld,           I2C_MUX_SYS_RESET);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_smbus_reset,          cpld,           I2C_MUX_SMBUS_RESET);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_eth_28_reset,         cpld,           I2C_MUX_QSFP28_RESET);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_sfp_reset,            cpld,           I2C_MUX_SFP_RESET);
static SENSOR_DEVICE_ATTR_RW(mb_i2c_reset,                 cpld,           MB_I2C_RESET);
static SENSOR_DEVICE_ATTR_RO(bmc_present,                  cpld,           BMC_PRESENT);
static SENSOR_DEVICE_ATTR_RO(sata_ssd1_present,            cpld,           SATA_SSD1_PRESENT);
static SENSOR_DEVICE_ATTR_RO(sata_ssd2_present,            cpld,           SATA_SSD2_PRESENT);
static SENSOR_DEVICE_ATTR_RO(psu0_present,                 cpld,           PSU0_PRESENT);
static SENSOR_DEVICE_ATTR_RO(psu1_present,                 cpld,           PSU1_PRESENT);
static SENSOR_DEVICE_ATTR_RO(psu0_vin_pg,                  cpld,           PSU0_VIN_PG);
static SENSOR_DEVICE_ATTR_RO(psu1_vin_pg,                  cpld,           PSU1_VIN_PG);
static SENSOR_DEVICE_ATTR_RO(psu0_vout_pg,                 cpld,           PSU0_VOUT_PG);
static SENSOR_DEVICE_ATTR_RO(psu1_vout_pg,                 cpld,           PSU1_VOUT_PG);
static SENSOR_DEVICE_ATTR_RO(psu_status,                   cpld,           PSU_STATUS);
static SENSOR_DEVICE_ATTR_RO(cpu_boot_done,                cpld,           CPU_BOOT_DONE);
static SENSOR_DEVICE_ATTR_RO(cpu_pg,                       cpld,           CPU_PG);
static SENSOR_DEVICE_ATTR_RO(cpu_status,                   cpld,           CPU_STATUS);
static SENSOR_DEVICE_ATTR_RW(phy_boot_ctrl,                cpld,           PHY_BOOT_CTRL);
static SENSOR_DEVICE_ATTR_RW(wd_status,                    cpld,           WD_STATUS);
static SENSOR_DEVICE_ATTR_RW(timing_ctrl_status,           cpld,           TIMING_CTRL_STATUS);
static SENSOR_DEVICE_ATTR_RW(smbus_peci_dis,               cpld,           SMBUS_PECI_DIS);
static SENSOR_DEVICE_ATTR_RW(i2c_psu0_mux_sel,             cpld,           I2C_PSU0_MUX_SEL);
static SENSOR_DEVICE_ATTR_RW(i2c_psu1_mux_sel,             cpld,           I2C_PSU1_MUX_SEL);
static SENSOR_DEVICE_ATTR_RW(i2c_cpld_mux_sel,             cpld,           I2C_CPLD_MUX_SEL);
static SENSOR_DEVICE_ATTR_RW(bmc_usb_mux_sel,              cpld,           BMC_USB_MUX_SEL);
static SENSOR_DEVICE_ATTR_RW(uart_cpu_bmc_mux_sel,         cpld,           UART_CPU_BMC_MUX_SEL);
static SENSOR_DEVICE_ATTR_RW(uart_mux_sel,                 cpld,           UART_MUX_SEL);
static SENSOR_DEVICE_ATTR_RW(pwr_system_ctrl,              cpld,           PWR_SYSTEM_CTRL);
static SENSOR_DEVICE_ATTR_RO(serboot_ufm_store,            cpld,           SERBOOT_UFM_STORE);
static SENSOR_DEVICE_ATTR_RW(serboot_ufm_write,            cpld,           SERBOOT_UFM_WRITE);
static SENSOR_DEVICE_ATTR_RW(cpld_write_protect_1,         cpld,           CPLD_WRITE_PROTECT_1);
static SENSOR_DEVICE_ATTR_RW(cpld_write_protect_2,         cpld,           CPLD_WRITE_PROTECT_2);
static SENSOR_DEVICE_ATTR_RW(ext_ctrl,                     cpld,           EXT_CTRL);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_sys,          led,            CPLD_SYSTEM_LED_SYS);
static SENSOR_DEVICE_ATTR_RW(system_led_status,            led,            SYSTEM_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(system_led_speed,             led,            SYSTEM_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(system_led_blink,             led,            SYSTEM_LED_BLINK);
static SENSOR_DEVICE_ATTR_RW(system_led_onoff,             led,            SYSTEM_LED_ONOFF);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_fan,          led,            CPLD_SYSTEM_LED_FAN);
static SENSOR_DEVICE_ATTR_RW(fan_led_status,               led,            FAN_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(fan_led_speed,                led,            FAN_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(fan_led_blink,                led,            FAN_LED_BLINK);
static SENSOR_DEVICE_ATTR_RW(fan_led_onoff,                led,            FAN_LED_ONOFF);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_pwr,          led,            CPLD_SYSTEM_LED_PWR);
static SENSOR_DEVICE_ATTR_RW(pwr_led_status,               led,            PWR_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(pwr_led_speed,                led,            PWR_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(pwr_led_blink,                led,            PWR_LED_BLINK);
static SENSOR_DEVICE_ATTR_RW(pwr_led_onoff,                led,            PWR_LED_ONOFF);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_gnss,         led,            CPLD_SYSTEM_LED_GNSS);
static SENSOR_DEVICE_ATTR_RW(gnss_led_status,              led,            GNSS_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(gnss_led_speed,               led,            GNSS_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(gnss_led_blink,               led,            GNSS_LED_BLINK);
static SENSOR_DEVICE_ATTR_RW(gnss_led_onoff,               led,            GNSS_LED_ONOFF);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_sync,         led,            CPLD_SYSTEM_LED_SYNC);
static SENSOR_DEVICE_ATTR_RW(sync_led_status,              led,            SYNC_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(sync_led_speed,               led,            SYNC_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(sync_led_blink,               led,            SYNC_LED_BLINK);
static SENSOR_DEVICE_ATTR_RW(sync_led_onoff,               led,            SYNC_LED_ONOFF);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_id,           led,            CPLD_SYSTEM_LED_ID);
static SENSOR_DEVICE_ATTR_RW(id_led_speed,                 led,            ID_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(id_led_blink,                 led,            ID_LED_BLINK);
static SENSOR_DEVICE_ATTR_RW(id_led_onoff,                 led,            ID_LED_ONOFF);
static SENSOR_DEVICE_ATTR_RW(led_clear,                    led,            LED_CLEAR);
static SENSOR_DEVICE_ATTR_RO(cpld1_pwr_status,             cpld,           CPLD1_PWR_STATUS);
static SENSOR_DEVICE_ATTR_RW(eth_0_pwr_en,                 cpld,           ETH_0_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_1_pwr_en,                 cpld,           ETH_1_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_2_pwr_en,                 cpld,           ETH_2_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_3_pwr_en,                 cpld,           ETH_3_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_4_pwr_en,                 cpld,           ETH_4_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_5_pwr_en,                 cpld,           ETH_5_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_12_pwr_en,                cpld,           ETH_12_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_13_pwr_en,                cpld,           ETH_13_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_14_pwr_en,                cpld,           ETH_14_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_15_pwr_en,                cpld,           ETH_15_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_6_pwr_en,                 cpld,           ETH_6_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_7_pwr_en,                 cpld,           ETH_7_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_8_pwr_en,                 cpld,           ETH_8_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_9_pwr_en,                 cpld,           ETH_9_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_10_pwr_en,                cpld,           ETH_10_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_11_pwr_en,                cpld,           ETH_11_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_16_pwr_en,                cpld,           ETH_16_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_17_pwr_en,                cpld,           ETH_17_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_18_pwr_en,                cpld,           ETH_18_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_19_pwr_en,                cpld,           ETH_19_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_20_pwr_en,                cpld,           ETH_20_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_21_pwr_en,                cpld,           ETH_21_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_22_pwr_en,                cpld,           ETH_22_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_23_pwr_en,                cpld,           ETH_23_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_24_pwr_en,                cpld,           ETH_24_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_25_pwr_en,                cpld,           ETH_25_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_26_pwr_en,                cpld,           ETH_26_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_27_pwr_en,                cpld,           ETH_27_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_28_pwr_en,                cpld,           ETH_28_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_29_pwr_en,                cpld,           ETH_29_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_30_pwr_en,                cpld,           ETH_30_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_31_pwr_en,                cpld,           ETH_31_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_32_pwr_en,                cpld,           ETH_32_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_33_pwr_en,                cpld,           ETH_33_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_34_pwr_en,                cpld,           ETH_34_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_35_pwr_en,                cpld,           ETH_35_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_36_pwr_en,                cpld,           ETH_36_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_37_pwr_en,                cpld,           ETH_37_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_38_pwr_en,                cpld,           ETH_38_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(eth_39_pwr_en,                cpld,           ETH_39_PWR_EN);
static SENSOR_DEVICE_ATTR_RO(ocxo_gnss_id,                 cpld,           OCXO_GNSS_ID);
static SENSOR_DEVICE_ATTR_RW(clk_ptp_reset,                cpld,           CLK_PTP_RESET);
static SENSOR_DEVICE_ATTR_RW(cjap_reset,                   cpld,           CJAP_RESET);
static SENSOR_DEVICE_ATTR_RW(ntm_reset,                    cpld,           NTM_RESET);
static SENSOR_DEVICE_ATTR_RW(gnss_reset,                   cpld,           GNSS_RESET);
static SENSOR_DEVICE_ATTR_RW(bits_reset,                   cpld,           BITS_RESET);
static SENSOR_DEVICE_ATTR_RW(clk_timing_ctrl,              cpld,           CLK_TIMING_CTRL);
static SENSOR_DEVICE_ATTR_RO(gnss_status,                  cpld,           GNSS_STATUS);
static SENSOR_DEVICE_ATTR_RO(timing_status,                cpld,           TIMING_STATUS);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_sel,                   cpld,           QSFPDD_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_28_sel,                   cpld,           ETH_28_SEL);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_0x76_reset,           cpld,           I2C_MUX_0X76_RESET);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_0x75_reset,           cpld,           I2C_MUX_0X75_RESET);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_6_11_reset,           cpld,           I2C_MUX_6_11_RESET);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_eth_16_23_reset,      cpld,           I2C_MUX_SFP56_16_23_RESET);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_eth_24_31_reset,      cpld,           I2C_MUX_SFP56_24_31_RESET);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_eth_32_39_reset,      cpld,           I2C_MUX_SFP56_32_39_RESET);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_0x71_reset,           cpld,           I2C_MUX_0X71_RESET);
static SENSOR_DEVICE_ATTR_RW(eth_0_present,                cpld,           ETH_0_PRESENT);
static SENSOR_DEVICE_ATTR_RW(eth_1_present,                cpld,           ETH_1_PRESENT);
static SENSOR_DEVICE_ATTR_RW(eth_2_present,                cpld,           ETH_2_PRESENT);
static SENSOR_DEVICE_ATTR_RW(eth_3_present,                cpld,           ETH_3_PRESENT);
static SENSOR_DEVICE_ATTR_RW(eth_4_present,                cpld,           ETH_4_PRESENT);
static SENSOR_DEVICE_ATTR_RW(eth_5_present,                cpld,           ETH_5_PRESENT);
static SENSOR_DEVICE_ATTR_RW(eth_12_present,               cpld,           ETH_12_PRESENT);
static SENSOR_DEVICE_ATTR_RW(eth_13_present,               cpld,           ETH_13_PRESENT);
static SENSOR_DEVICE_ATTR_RW(eth_14_present,               cpld,           ETH_14_PRESENT);
static SENSOR_DEVICE_ATTR_RW(eth_15_present,               cpld,           ETH_15_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_0_intr,                   cpld,           ETH_0_INTR);
static SENSOR_DEVICE_ATTR_RO(eth_1_intr,                   cpld,           ETH_1_INTR);
static SENSOR_DEVICE_ATTR_RO(eth_2_intr,                   cpld,           ETH_2_INTR);
static SENSOR_DEVICE_ATTR_RO(eth_3_intr,                   cpld,           ETH_3_INTR);
static SENSOR_DEVICE_ATTR_RO(eth_4_intr,                   cpld,           ETH_4_INTR);
static SENSOR_DEVICE_ATTR_RO(eth_5_intr,                   cpld,           ETH_5_INTR);
static SENSOR_DEVICE_ATTR_RO(eth_12_intr,                  cpld,           ETH_12_INTR);
static SENSOR_DEVICE_ATTR_RO(eth_13_intr,                  cpld,           ETH_13_INTR);
static SENSOR_DEVICE_ATTR_RO(eth_14_intr,                  cpld,           ETH_14_INTR);
static SENSOR_DEVICE_ATTR_RO(eth_15_intr,                  cpld,           ETH_15_INTR);
static SENSOR_DEVICE_ATTR_RO(eth_0_efuse_pg,               cpld,           ETH_0_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(eth_1_efuse_pg,               cpld,           ETH_1_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(eth_2_efuse_pg,               cpld,           ETH_2_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(eth_3_efuse_pg,               cpld,           ETH_3_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(eth_4_efuse_pg,               cpld,           ETH_4_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(eth_5_efuse_pg,               cpld,           ETH_5_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(eth_12_efuse_pg,              cpld,           ETH_12_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(eth_13_efuse_pg,              cpld,           ETH_13_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(eth_14_efuse_pg,              cpld,           ETH_14_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(eth_15_efuse_pg,              cpld,           ETH_15_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(eth_0_i2c_stuck,              cpld,           ETH_0_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_1_i2c_stuck,              cpld,           ETH_1_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_2_i2c_stuck,              cpld,           ETH_2_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_3_i2c_stuck,              cpld,           ETH_3_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_4_i2c_stuck,              cpld,           ETH_4_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_5_i2c_stuck,              cpld,           ETH_5_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_12_i2c_stuck,             cpld,           ETH_12_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_13_i2c_stuck,             cpld,           ETH_13_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_14_i2c_stuck,             cpld,           ETH_14_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_15_i2c_stuck,             cpld,           ETH_15_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(cpld2_i2c_stuck,              cpld,           CPLD2_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(cpld2_to_cpld1_intr,          cpld,           CPLD2_TO_CPLD1_INTR);
static SENSOR_DEVICE_ATTR_RW(eth_0_present_mask,           cpld,           ETH_0_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_1_present_mask,           cpld,           ETH_1_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_2_present_mask,           cpld,           ETH_2_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_3_present_mask,           cpld,           ETH_3_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_4_present_mask,           cpld,           ETH_4_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_5_present_mask,           cpld,           ETH_5_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_12_present_mask,          cpld,           ETH_12_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_13_present_mask,          cpld,           ETH_13_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_14_present_mask,          cpld,           ETH_14_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_15_present_mask,          cpld,           ETH_15_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_0_intr_mask,              cpld,           ETH_0_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_1_intr_mask,              cpld,           ETH_1_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_2_intr_mask,              cpld,           ETH_2_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_3_intr_mask,              cpld,           ETH_3_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_4_intr_mask,              cpld,           ETH_4_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_5_intr_mask,              cpld,           ETH_5_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_12_intr_mask,             cpld,           ETH_12_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_13_intr_mask,             cpld,           ETH_13_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_14_intr_mask,             cpld,           ETH_14_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_15_intr_mask,             cpld,           ETH_15_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_0_efuse_pg_mask,          cpld,           ETH_0_EFUSE_PG_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_1_efuse_pg_mask,          cpld,           ETH_1_EFUSE_PG_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_2_efuse_pg_mask,          cpld,           ETH_2_EFUSE_PG_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_3_efuse_pg_mask,          cpld,           ETH_3_EFUSE_PG_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_4_efuse_pg_mask,          cpld,           ETH_4_EFUSE_PG_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_5_efuse_pg_mask,          cpld,           ETH_5_EFUSE_PG_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_12_efuse_pg_mask,         cpld,           ETH_12_EFUSE_PG_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_13_efuse_pg_mask,         cpld,           ETH_13_EFUSE_PG_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_14_efuse_pg_mask,         cpld,           ETH_14_EFUSE_PG_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_15_efuse_pg_mask,         cpld,           ETH_15_EFUSE_PG_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_0_i2c_stuck_mask,         cpld,           ETH_0_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_1_i2c_stuck_mask,         cpld,           ETH_1_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_2_i2c_stuck_mask,         cpld,           ETH_2_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_3_i2c_stuck_mask,         cpld,           ETH_3_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_4_i2c_stuck_mask,         cpld,           ETH_4_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_5_i2c_stuck_mask,         cpld,           ETH_5_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_12_i2c_stuck_mask,        cpld,           ETH_12_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_13_i2c_stuck_mask,        cpld,           ETH_13_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_14_i2c_stuck_mask,        cpld,           ETH_14_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_15_i2c_stuck_mask,        cpld,           ETH_15_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld2_i2c_stuck_mask,         cpld,           CPLD2_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld2_to_cpld1_intr_mask,     cpld,           CPLD2_TO_CPLD1_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(eth_0_present_event,          cpld,           ETH_0_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_1_present_event,          cpld,           ETH_1_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_2_present_event,          cpld,           ETH_2_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_3_present_event,          cpld,           ETH_3_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_4_present_event,          cpld,           ETH_4_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_5_present_event,          cpld,           ETH_5_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_12_present_event,         cpld,           ETH_12_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_13_present_event,         cpld,           ETH_13_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_14_present_event,         cpld,           ETH_14_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_15_present_event,         cpld,           ETH_15_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_0_intr_event,             cpld,           ETH_0_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_1_intr_event,             cpld,           ETH_1_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_2_intr_event,             cpld,           ETH_2_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_3_intr_event,             cpld,           ETH_3_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_4_intr_event,             cpld,           ETH_4_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_5_intr_event,             cpld,           ETH_5_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_12_intr_event,            cpld,           ETH_12_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_13_intr_event,            cpld,           ETH_13_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_14_intr_event,            cpld,           ETH_14_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_15_intr_event,            cpld,           ETH_15_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_0_efuse_pg_event,         cpld,           ETH_0_EFUSE_PG_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_1_efuse_pg_event,         cpld,           ETH_1_EFUSE_PG_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_2_efuse_pg_event,         cpld,           ETH_2_EFUSE_PG_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_3_efuse_pg_event,         cpld,           ETH_3_EFUSE_PG_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_4_efuse_pg_event,         cpld,           ETH_4_EFUSE_PG_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_5_efuse_pg_event,         cpld,           ETH_5_EFUSE_PG_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_12_efuse_pg_event,        cpld,           ETH_12_EFUSE_PG_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_13_efuse_pg_event,        cpld,           ETH_13_EFUSE_PG_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_14_efuse_pg_event,        cpld,           ETH_14_EFUSE_PG_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_15_efuse_pg_event,        cpld,           ETH_15_EFUSE_PG_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_0_i2c_stuck_event,        cpld,           ETH_0_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_1_i2c_stuck_event,        cpld,           ETH_1_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_2_i2c_stuck_event,        cpld,           ETH_2_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_3_i2c_stuck_event,        cpld,           ETH_3_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_4_i2c_stuck_event,        cpld,           ETH_4_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_5_i2c_stuck_event,        cpld,           ETH_5_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_12_i2c_stuck_event,       cpld,           ETH_12_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_13_i2c_stuck_event,       cpld,           ETH_13_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_14_i2c_stuck_event,       cpld,           ETH_14_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_15_i2c_stuck_event,       cpld,           ETH_15_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld2_i2c_stuck_event,        cpld,           CPLD2_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld2_to_cpld1_intr_event,    cpld,           CPLD2_TO_CPLD1_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RW(eth_0_reset,                  cpld,           ETH_0_RESET);
static SENSOR_DEVICE_ATTR_RW(eth_1_reset,                  cpld,           ETH_1_RESET);
static SENSOR_DEVICE_ATTR_RW(eth_2_reset,                  cpld,           ETH_2_RESET);
static SENSOR_DEVICE_ATTR_RW(eth_3_reset,                  cpld,           ETH_3_RESET);
static SENSOR_DEVICE_ATTR_RW(eth_4_reset,                  cpld,           ETH_4_RESET);
static SENSOR_DEVICE_ATTR_RW(eth_5_reset,                  cpld,           ETH_5_RESET);
static SENSOR_DEVICE_ATTR_RW(eth_12_reset,                 cpld,           ETH_12_RESET);
static SENSOR_DEVICE_ATTR_RW(eth_13_reset,                 cpld,           ETH_13_RESET);
static SENSOR_DEVICE_ATTR_RW(eth_14_reset,                 cpld,           ETH_14_RESET);
static SENSOR_DEVICE_ATTR_RW(eth_15_reset,                 cpld,           ETH_15_RESET);
static SENSOR_DEVICE_ATTR_RW(eth_0_lpmode,                 cpld,           ETH_0_LPMODE);
static SENSOR_DEVICE_ATTR_RW(eth_1_lpmode,                 cpld,           ETH_1_LPMODE);
static SENSOR_DEVICE_ATTR_RW(eth_2_lpmode,                 cpld,           ETH_2_LPMODE);
static SENSOR_DEVICE_ATTR_RW(eth_3_lpmode,                 cpld,           ETH_3_LPMODE);
static SENSOR_DEVICE_ATTR_RW(eth_4_lpmode,                 cpld,           ETH_4_LPMODE);
static SENSOR_DEVICE_ATTR_RW(eth_5_lpmode,                 cpld,           ETH_5_LPMODE);
static SENSOR_DEVICE_ATTR_RW(eth_12_lpmode,                cpld,           ETH_12_LPMODE);
static SENSOR_DEVICE_ATTR_RW(eth_13_lpmode,                cpld,           ETH_13_LPMODE);
static SENSOR_DEVICE_ATTR_RW(eth_14_lpmode,                cpld,           ETH_14_LPMODE);
static SENSOR_DEVICE_ATTR_RW(eth_15_lpmode,                cpld,           ETH_15_LPMODE);
static SENSOR_DEVICE_ATTR_RW(clk_en_ctrl,                  cpld,           CLK_EN_CTRL);
static SENSOR_DEVICE_ATTR_RW(psu_ctrl,                     cpld,           PSU_CTRL);
static SENSOR_DEVICE_ATTR_RW(eth_12_led_ctrl,              led,            ETH_12_LED_CTRL);
static SENSOR_DEVICE_ATTR_RW(eth_13_led_ctrl,              led,            ETH_13_LED_CTRL);
static SENSOR_DEVICE_ATTR_RW(eth_14_led_ctrl,              led,            ETH_14_LED_CTRL);
static SENSOR_DEVICE_ATTR_RW(eth_15_led_ctrl,              led,            ETH_15_LED_CTRL);
static SENSOR_DEVICE_ATTR_RO(cpld2_pwr_status_0,           cpld,           CPLD2_PWR_STATUS_0);
static SENSOR_DEVICE_ATTR_RO(cpld2_pwr_status_1,           cpld,           CPLD2_PWR_STATUS_1);
static SENSOR_DEVICE_ATTR_RW(dbg_eth_0_5_present,          cpld,           DBG_QSFP28_0_5_ABS);
static SENSOR_DEVICE_ATTR_RW(dbg_eth_12_15_present,        cpld,           DBG_QSFPDD_12_15_ABS);
static SENSOR_DEVICE_ATTR_RW(dbg_eth_0_5_intr,             cpld,           DBG_QSFP28_0_5_INTR);
static SENSOR_DEVICE_ATTR_RW(dbg_eth_12_15_intr,           cpld,           DBG_QSFPDD_12_15_INTR);
static SENSOR_DEVICE_ATTR_RW(dbg_eth_0_5_efuse_pg,         cpld,           DBG_QSFP28_0_5_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RW(dbg_eth_12_15_efuse_pg,       cpld,           DBG_QSFPDD_12_15_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(gnss_model_id,                cpld,           GNSS_MODEL_ID);
static SENSOR_DEVICE_ATTR_RO(ocxo_id,                      cpld,           OCXO_ID);
static SENSOR_DEVICE_ATTR_RO(eth_6_present,                cpld,           ETH_6_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_7_present,                cpld,           ETH_7_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_8_present,                cpld,           ETH_8_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_9_present,                cpld,           ETH_9_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_10_present,               cpld,           ETH_10_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_11_present,               cpld,           ETH_11_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_6_intr,                   cpld,           ETH_6_INTR);
static SENSOR_DEVICE_ATTR_RO(eth_7_intr,                   cpld,           ETH_7_INTR);
static SENSOR_DEVICE_ATTR_RO(eth_8_intr,                   cpld,           ETH_8_INTR);
static SENSOR_DEVICE_ATTR_RO(eth_9_intr,                   cpld,           ETH_9_INTR);
static SENSOR_DEVICE_ATTR_RO(eth_10_intr,                  cpld,           ETH_10_INTR);
static SENSOR_DEVICE_ATTR_RO(eth_11_intr,                  cpld,           ETH_11_INTR);
static SENSOR_DEVICE_ATTR_RO(eth_6_efuse_pg,               cpld,           ETH_6_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(eth_7_efuse_pg,               cpld,           ETH_7_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(eth_8_efuse_pg,               cpld,           ETH_8_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(eth_9_efuse_pg,               cpld,           ETH_9_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(eth_10_efuse_pg,              cpld,           ETH_10_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(eth_11_efuse_pg,              cpld,           ETH_11_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(mac_intr,                     cpld,           MAC_INTR);
static SENSOR_DEVICE_ATTR_RO(main_thermal_intr,            cpld,           MAIN_THERMAL_INTR);
static SENSOR_DEVICE_ATTR_RO(fan_0_present,                cpld,           FAN_0_PRESENT);
static SENSOR_DEVICE_ATTR_RO(fan_1_present,                cpld,           FAN_1_PRESENT);
static SENSOR_DEVICE_ATTR_RO(fan_2_present,                cpld,           FAN_2_PRESENT);
static SENSOR_DEVICE_ATTR_RO(fan_3_present,                cpld,           FAN_3_PRESENT);
static SENSOR_DEVICE_ATTR_RO(fan_4_present,                cpld,           FAN_4_PRESENT);
static SENSOR_DEVICE_ATTR_RO(io_oc_intr,                   cpld,           IO_OC_INTR);
static SENSOR_DEVICE_ATTR_RO(eth_6_i2c_stuck,              cpld,           ETH_6_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_7_i2c_stuck,              cpld,           ETH_7_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_8_i2c_stuck,              cpld,           ETH_8_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_9_i2c_stuck,              cpld,           ETH_9_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_10_i2c_stuck,             cpld,           ETH_10_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_11_i2c_stuck,             cpld,           ETH_11_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(cpld3_i2c_stuck,              cpld,           CPLD3_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(cpld3_to_cpld1_intr,          cpld,           CPLD3_TO_CPLD1_INTR);
static SENSOR_DEVICE_ATTR_RW(eth_6_present_mask,           cpld,           ETH_6_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_7_present_mask,           cpld,           ETH_7_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_8_present_mask,           cpld,           ETH_8_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_9_present_mask,           cpld,           ETH_9_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_10_present_mask,          cpld,           ETH_10_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_11_present_mask,          cpld,           ETH_11_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_6_intr_mask,              cpld,           ETH_6_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_7_intr_mask,              cpld,           ETH_7_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_8_intr_mask,              cpld,           ETH_8_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_9_intr_mask,              cpld,           ETH_9_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_10_intr_mask,             cpld,           ETH_10_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_11_intr_mask,             cpld,           ETH_11_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_6_efuse_pg_mask,          cpld,           ETH_6_EFUSE_PG_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_7_efuse_pg_mask,          cpld,           ETH_7_EFUSE_PG_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_8_efuse_pg_mask,          cpld,           ETH_8_EFUSE_PG_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_9_efuse_pg_mask,          cpld,           ETH_9_EFUSE_PG_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_10_efuse_pg_mask,         cpld,           ETH_10_EFUSE_PG_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_11_efuse_pg_mask,         cpld,           ETH_11_EFUSE_PG_MASK);
static SENSOR_DEVICE_ATTR_RW(mac_intr_mask,                cpld,           MAC_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(main_thermal_intr_mask,       cpld,           MAIN_THERMAL_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(fan_present_mask,             cpld,           FAN_ABS_MASK);
static SENSOR_DEVICE_ATTR_RW(io_oc_intr_mask,              cpld,           IO_OC_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_6_i2c_stuck_mask,         cpld,           ETH_6_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_7_i2c_stuck_mask,         cpld,           ETH_7_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_8_i2c_stuck_mask,         cpld,           ETH_8_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_9_i2c_stuck_mask,         cpld,           ETH_9_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_10_i2c_stuck_mask,        cpld,           ETH_10_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_11_i2c_stuck_mask,        cpld,           ETH_11_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld3_i2c_stuck_mask,         cpld,           CPLD3_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld3_to_cpld1_intr_mask,     cpld,           CPLD3_TO_CPLD1_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(eth_6_present_event,          cpld,           ETH_6_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_7_present_event,          cpld,           ETH_7_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_8_present_event,          cpld,           ETH_8_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_9_present_event,          cpld,           ETH_9_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_10_present_event,         cpld,           ETH_10_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_11_present_event,         cpld,           ETH_11_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_6_intr_event,             cpld,           ETH_6_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_7_intr_event,             cpld,           ETH_7_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_8_intr_event,             cpld,           ETH_8_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_9_intr_event,             cpld,           ETH_9_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_10_intr_event,            cpld,           ETH_10_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_11_intr_event,            cpld,           ETH_11_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_6_efuse_pg_event,         cpld,           ETH_6_EFUSE_PG_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_7_efuse_pg_event,         cpld,           ETH_7_EFUSE_PG_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_8_efuse_pg_event,         cpld,           ETH_8_EFUSE_PG_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_9_efuse_pg_event,         cpld,           ETH_9_EFUSE_PG_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_10_efuse_pg_event,        cpld,           ETH_10_EFUSE_PG_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_11_efuse_pg_event,        cpld,           ETH_11_EFUSE_PG_EVENT);
static SENSOR_DEVICE_ATTR_RO(mac_intr_event,               cpld,           MAC_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(main_thermal_intr_event,      cpld,           MAIN_THERMAL_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(fan_present_event,            cpld,           FAN_ABS_EVENT);
static SENSOR_DEVICE_ATTR_RO(io_oc_intr_event,             cpld,           IO_OC_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_6_i2c_stuck_event,        cpld,           ETH_6_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_7_i2c_stuck_event,        cpld,           ETH_7_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_8_i2c_stuck_event,        cpld,           ETH_8_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_9_i2c_stuck_event,        cpld,           ETH_9_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_10_i2c_stuck_event,       cpld,           ETH_10_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_11_i2c_stuck_event,       cpld,           ETH_11_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld3_i2c_stuck_event,        cpld,           CPLD3_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld3_to_cpld1_intr_event,    cpld,           CPLD3_TO_CPLD1_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RW(eth_6_reset,                  cpld,           ETH_6_RESET);
static SENSOR_DEVICE_ATTR_RW(eth_7_reset,                  cpld,           ETH_7_RESET);
static SENSOR_DEVICE_ATTR_RW(eth_8_reset,                  cpld,           ETH_8_RESET);
static SENSOR_DEVICE_ATTR_RW(eth_9_reset,                  cpld,           ETH_9_RESET);
static SENSOR_DEVICE_ATTR_RW(eth_10_reset,                 cpld,           ETH_10_RESET);
static SENSOR_DEVICE_ATTR_RW(eth_11_reset,                 cpld,           ETH_11_RESET);
static SENSOR_DEVICE_ATTR_RW(eth_6_lpmode,                 cpld,           ETH_6_LPMODE);
static SENSOR_DEVICE_ATTR_RW(eth_7_lpmode,                 cpld,           ETH_7_LPMODE);
static SENSOR_DEVICE_ATTR_RW(eth_8_lpmode,                 cpld,           ETH_8_LPMODE);
static SENSOR_DEVICE_ATTR_RW(eth_9_lpmode,                 cpld,           ETH_9_LPMODE);
static SENSOR_DEVICE_ATTR_RW(eth_10_lpmode,                cpld,           ETH_10_LPMODE);
static SENSOR_DEVICE_ATTR_RW(eth_11_lpmode,                cpld,           ETH_11_LPMODE);
static SENSOR_DEVICE_ATTR_RW(mac_reset,                    cpld,           MAC_RESET);
static SENSOR_DEVICE_ATTR_RW(usb_qspi_reset,               cpld,           USB_QSPI_RESET);
static SENSOR_DEVICE_ATTR_RO(clk_timing_status_1,          cpld,           CLK_TIMING_STATUS_1);
static SENSOR_DEVICE_ATTR_RO(clk_timing_status_2,          cpld,           CLK_TIMING_STATUS_2);
static SENSOR_DEVICE_ATTR_RO(rov_status,                   cpld,           ROV_STATUS);
static SENSOR_DEVICE_ATTR_RW(misc_control,                 cpld,           MISC_CONTROL);
static SENSOR_DEVICE_ATTR_RW(gnss_ctrl,                    cpld,           GNSS_CTRL);
static SENSOR_DEVICE_ATTR_RW(synce_ctrl,                   cpld,           SYNCE_CTRL);
static SENSOR_DEVICE_ATTR_RW(fan_speed_read_mode,          cpld,           FAN_SPEED_READ_MODE);
static SENSOR_DEVICE_ATTR_RO(cpld3_pwr_status,             cpld,           CPLD3_PWR_STATUS);
static SENSOR_DEVICE_ATTR_RW(dbg_eth_6_11_present,         cpld,           DBG_QSFP28_6_11_ABS);
static SENSOR_DEVICE_ATTR_RW(dbg_eth_6_11_intr,            cpld,           DBG_QSFP28_6_11_INTR);
static SENSOR_DEVICE_ATTR_RW(dbg_eth_6_11_efuse_pg,        cpld,           DBG_QSFP28_6_11_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RW(dbg_mac_intr,                 cpld,           DBG_MAC_INTR);
static SENSOR_DEVICE_ATTR_RW(dbg_thermal_intr,             cpld,           DBG_THERMAL_INTR);
static SENSOR_DEVICE_ATTR_RW(dbg_misc_intr,                cpld,           DBG_MISC_INTR);
static SENSOR_DEVICE_ATTR_RO(eth_16_present,               cpld,           ETH_16_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_17_present,               cpld,           ETH_17_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_18_present,               cpld,           ETH_18_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_19_present,               cpld,           ETH_19_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_20_present,               cpld,           ETH_20_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_21_present,               cpld,           ETH_21_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_22_present,               cpld,           ETH_22_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_23_present,               cpld,           ETH_23_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_24_present,               cpld,           ETH_24_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_25_present,               cpld,           ETH_25_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_26_present,               cpld,           ETH_26_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_27_present,               cpld,           ETH_27_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_28_present,               cpld,           ETH_28_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_29_present,               cpld,           ETH_29_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_30_present,               cpld,           ETH_30_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_31_present,               cpld,           ETH_31_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_32_present,               cpld,           ETH_32_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_33_present,               cpld,           ETH_33_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_34_present,               cpld,           ETH_34_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_35_present,               cpld,           ETH_35_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_36_present,               cpld,           ETH_36_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_37_present,               cpld,           ETH_37_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_38_present,               cpld,           ETH_38_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_39_present,               cpld,           ETH_39_PRESENT);
static SENSOR_DEVICE_ATTR_RW(eth_16_rx_los,                cpld,           ETH_16_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_17_rx_los,                cpld,           ETH_17_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_18_rx_los,                cpld,           ETH_18_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_19_rx_los,                cpld,           ETH_19_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_20_rx_los,                cpld,           ETH_20_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_21_rx_los,                cpld,           ETH_21_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_22_rx_los,                cpld,           ETH_22_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_23_rx_los,                cpld,           ETH_23_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_24_rx_los,                cpld,           ETH_24_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_25_rx_los,                cpld,           ETH_25_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_26_rx_los,                cpld,           ETH_26_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_27_rx_los,                cpld,           ETH_27_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_28_rx_los,                cpld,           ETH_28_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_29_rx_los,                cpld,           ETH_29_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_30_rx_los,                cpld,           ETH_30_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_31_rx_los,                cpld,           ETH_31_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_32_rx_los,                cpld,           ETH_32_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_33_rx_los,                cpld,           ETH_33_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_34_rx_los,                cpld,           ETH_34_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_35_rx_los,                cpld,           ETH_35_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_36_rx_los,                cpld,           ETH_36_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_37_rx_los,                cpld,           ETH_37_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_38_rx_los,                cpld,           ETH_38_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_39_rx_los,                cpld,           ETH_39_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_16_tx_fault,              cpld,           ETH_16_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(eth_17_tx_fault,              cpld,           ETH_17_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(eth_18_tx_fault,              cpld,           ETH_18_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(eth_19_tx_fault,              cpld,           ETH_19_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(eth_20_tx_fault,              cpld,           ETH_20_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(eth_21_tx_fault,              cpld,           ETH_21_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(eth_22_tx_fault,              cpld,           ETH_22_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(eth_23_tx_fault,              cpld,           ETH_23_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(eth_24_tx_fault,              cpld,           ETH_24_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(eth_25_tx_fault,              cpld,           ETH_25_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(eth_26_tx_fault,              cpld,           ETH_26_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(eth_27_tx_fault,              cpld,           ETH_27_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(eth_28_tx_fault,              cpld,           ETH_28_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(eth_29_tx_fault,              cpld,           ETH_29_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(eth_30_tx_fault,              cpld,           ETH_30_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(eth_31_tx_fault,              cpld,           ETH_31_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(eth_32_tx_fault,              cpld,           ETH_32_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(eth_33_tx_fault,              cpld,           ETH_33_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(eth_34_tx_fault,              cpld,           ETH_34_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(eth_35_tx_fault,              cpld,           ETH_35_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(eth_36_tx_fault,              cpld,           ETH_36_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(eth_37_tx_fault,              cpld,           ETH_37_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(eth_38_tx_fault,              cpld,           ETH_38_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(eth_39_tx_fault,              cpld,           ETH_39_TX_FAULT);
static SENSOR_DEVICE_ATTR_RO(eth_16_i2c_stuck,             cpld,           ETH_16_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_17_i2c_stuck,             cpld,           ETH_17_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_18_i2c_stuck,             cpld,           ETH_18_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_19_i2c_stuck,             cpld,           ETH_19_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_20_i2c_stuck,             cpld,           ETH_20_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_21_i2c_stuck,             cpld,           ETH_21_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_22_i2c_stuck,             cpld,           ETH_22_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_23_i2c_stuck,             cpld,           ETH_23_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_24_i2c_stuck,             cpld,           ETH_24_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_25_i2c_stuck,             cpld,           ETH_25_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_26_i2c_stuck,             cpld,           ETH_26_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_27_i2c_stuck,             cpld,           ETH_27_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_28_i2c_stuck,             cpld,           ETH_28_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_29_i2c_stuck,             cpld,           ETH_29_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_30_i2c_stuck,             cpld,           ETH_30_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_31_i2c_stuck,             cpld,           ETH_31_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_32_i2c_stuck,             cpld,           ETH_32_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_33_i2c_stuck,             cpld,           ETH_33_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_34_i2c_stuck,             cpld,           ETH_34_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_35_i2c_stuck,             cpld,           ETH_35_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_36_i2c_stuck,             cpld,           ETH_36_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_37_i2c_stuck,             cpld,           ETH_37_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_38_i2c_stuck,             cpld,           ETH_38_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_39_i2c_stuck,             cpld,           ETH_39_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(cpld4_to_cpld1_intr,          cpld,           CPLD4_TO_CPLD1_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld4_i2c_stuck,              cpld,           CPLD4_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RW(eth_16_present_mask,          cpld,           ETH_16_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_17_present_mask,          cpld,           ETH_17_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_18_present_mask,          cpld,           ETH_18_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_19_present_mask,          cpld,           ETH_19_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_20_present_mask,          cpld,           ETH_20_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_21_present_mask,          cpld,           ETH_21_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_22_present_mask,          cpld,           ETH_22_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_23_present_mask,          cpld,           ETH_23_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_24_present_mask,          cpld,           ETH_24_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_25_present_mask,          cpld,           ETH_25_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_26_present_mask,          cpld,           ETH_26_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_27_present_mask,          cpld,           ETH_27_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_28_present_mask,          cpld,           ETH_28_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_29_present_mask,          cpld,           ETH_29_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_30_present_mask,          cpld,           ETH_30_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_31_present_mask,          cpld,           ETH_31_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_32_present_mask,          cpld,           ETH_32_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_33_present_mask,          cpld,           ETH_33_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_34_present_mask,          cpld,           ETH_34_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_35_present_mask,          cpld,           ETH_35_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_36_present_mask,          cpld,           ETH_36_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_37_present_mask,          cpld,           ETH_37_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_38_present_mask,          cpld,           ETH_38_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_39_present_mask,          cpld,           ETH_39_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_16_rx_los_mask,           cpld,           ETH_16_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_17_rx_los_mask,           cpld,           ETH_17_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_18_rx_los_mask,           cpld,           ETH_18_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_19_rx_los_mask,           cpld,           ETH_19_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_20_rx_los_mask,           cpld,           ETH_20_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_21_rx_los_mask,           cpld,           ETH_21_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_22_rx_los_mask,           cpld,           ETH_22_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_23_rx_los_mask,           cpld,           ETH_23_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_24_rx_los_mask,           cpld,           ETH_24_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_25_rx_los_mask,           cpld,           ETH_25_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_26_rx_los_mask,           cpld,           ETH_26_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_27_rx_los_mask,           cpld,           ETH_27_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_28_rx_los_mask,           cpld,           ETH_28_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_29_rx_los_mask,           cpld,           ETH_29_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_30_rx_los_mask,           cpld,           ETH_30_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_31_rx_los_mask,           cpld,           ETH_31_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_32_rx_los_mask,           cpld,           ETH_32_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_33_rx_los_mask,           cpld,           ETH_33_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_34_rx_los_mask,           cpld,           ETH_34_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_35_rx_los_mask,           cpld,           ETH_35_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_36_rx_los_mask,           cpld,           ETH_36_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_37_rx_los_mask,           cpld,           ETH_37_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_38_rx_los_mask,           cpld,           ETH_38_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_39_rx_los_mask,           cpld,           ETH_39_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_16_tx_fault_mask,         cpld,           ETH_16_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_17_tx_fault_mask,         cpld,           ETH_17_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_18_tx_fault_mask,         cpld,           ETH_18_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_19_tx_fault_mask,         cpld,           ETH_19_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_20_tx_fault_mask,         cpld,           ETH_20_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_21_tx_fault_mask,         cpld,           ETH_21_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_22_tx_fault_mask,         cpld,           ETH_22_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_23_tx_fault_mask,         cpld,           ETH_23_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_24_tx_fault_mask,         cpld,           ETH_24_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_25_tx_fault_mask,         cpld,           ETH_25_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_26_tx_fault_mask,         cpld,           ETH_26_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_27_tx_fault_mask,         cpld,           ETH_27_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_28_tx_fault_mask,         cpld,           ETH_28_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_29_tx_fault_mask,         cpld,           ETH_29_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_30_tx_fault_mask,         cpld,           ETH_30_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_31_tx_fault_mask,         cpld,           ETH_31_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_32_tx_fault_mask,         cpld,           ETH_32_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_33_tx_fault_mask,         cpld,           ETH_33_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_34_tx_fault_mask,         cpld,           ETH_34_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_35_tx_fault_mask,         cpld,           ETH_35_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_36_tx_fault_mask,         cpld,           ETH_36_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_37_tx_fault_mask,         cpld,           ETH_37_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_38_tx_fault_mask,         cpld,           ETH_38_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_39_tx_fault_mask,         cpld,           ETH_39_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_16_i2c_stuck_mask,        cpld,           ETH_16_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_17_i2c_stuck_mask,        cpld,           ETH_17_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_18_i2c_stuck_mask,        cpld,           ETH_18_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_19_i2c_stuck_mask,        cpld,           ETH_19_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_20_i2c_stuck_mask,        cpld,           ETH_20_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_21_i2c_stuck_mask,        cpld,           ETH_21_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_22_i2c_stuck_mask,        cpld,           ETH_22_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_23_i2c_stuck_mask,        cpld,           ETH_23_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_24_i2c_stuck_mask,        cpld,           ETH_24_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_25_i2c_stuck_mask,        cpld,           ETH_25_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_26_i2c_stuck_mask,        cpld,           ETH_26_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_27_i2c_stuck_mask,        cpld,           ETH_27_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_28_i2c_stuck_mask,        cpld,           ETH_28_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_29_i2c_stuck_mask,        cpld,           ETH_29_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_30_i2c_stuck_mask,        cpld,           ETH_30_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_31_i2c_stuck_mask,        cpld,           ETH_31_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_32_i2c_stuck_mask,        cpld,           ETH_32_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_33_i2c_stuck_mask,        cpld,           ETH_33_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_34_i2c_stuck_mask,        cpld,           ETH_34_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_35_i2c_stuck_mask,        cpld,           ETH_35_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_36_i2c_stuck_mask,        cpld,           ETH_36_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_37_i2c_stuck_mask,        cpld,           ETH_37_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_38_i2c_stuck_mask,        cpld,           ETH_38_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_39_i2c_stuck_mask,        cpld,           ETH_39_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld4_to_cpld1_intr_mask,     cpld,           CPLD4_TO_CPLD1_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld4_i2c_stuck_mask,         cpld,           CPLD4_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RO(eth_16_present_event,         cpld,           ETH_16_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_17_present_event,         cpld,           ETH_17_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_18_present_event,         cpld,           ETH_18_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_19_present_event,         cpld,           ETH_19_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_20_present_event,         cpld,           ETH_20_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_21_present_event,         cpld,           ETH_21_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_22_present_event,         cpld,           ETH_22_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_23_present_event,         cpld,           ETH_23_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_24_present_event,         cpld,           ETH_24_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_25_present_event,         cpld,           ETH_25_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_26_present_event,         cpld,           ETH_26_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_27_present_event,         cpld,           ETH_27_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_28_present_event,         cpld,           ETH_28_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_29_present_event,         cpld,           ETH_29_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_30_present_event,         cpld,           ETH_30_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_31_present_event,         cpld,           ETH_31_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_32_present_event,         cpld,           ETH_32_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_33_present_event,         cpld,           ETH_33_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_34_present_event,         cpld,           ETH_34_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_35_present_event,         cpld,           ETH_35_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_36_present_event,         cpld,           ETH_36_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_37_present_event,         cpld,           ETH_37_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_38_present_event,         cpld,           ETH_38_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_39_present_event,         cpld,           ETH_39_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_16_rx_los_event,          cpld,           ETH_16_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_17_rx_los_event,          cpld,           ETH_17_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_18_rx_los_event,          cpld,           ETH_18_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_19_rx_los_event,          cpld,           ETH_19_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_20_rx_los_event,          cpld,           ETH_20_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_21_rx_los_event,          cpld,           ETH_21_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_22_rx_los_event,          cpld,           ETH_22_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_23_rx_los_event,          cpld,           ETH_23_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_24_rx_los_event,          cpld,           ETH_24_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_25_rx_los_event,          cpld,           ETH_25_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_26_rx_los_event,          cpld,           ETH_26_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_27_rx_los_event,          cpld,           ETH_27_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_28_rx_los_event,          cpld,           ETH_28_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_29_rx_los_event,          cpld,           ETH_29_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_30_rx_los_event,          cpld,           ETH_30_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_31_rx_los_event,          cpld,           ETH_31_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_32_rx_los_event,          cpld,           ETH_32_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_33_rx_los_event,          cpld,           ETH_33_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_34_rx_los_event,          cpld,           ETH_34_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_35_rx_los_event,          cpld,           ETH_35_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_36_rx_los_event,          cpld,           ETH_36_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_37_rx_los_event,          cpld,           ETH_37_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_38_rx_los_event,          cpld,           ETH_38_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_39_rx_los_event,          cpld,           ETH_39_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_16_tx_fault_event,        cpld,           ETH_16_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_17_tx_fault_event,        cpld,           ETH_17_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_18_tx_fault_event,        cpld,           ETH_18_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_19_tx_fault_event,        cpld,           ETH_19_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_20_tx_fault_event,        cpld,           ETH_20_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_21_tx_fault_event,        cpld,           ETH_21_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_22_tx_fault_event,        cpld,           ETH_22_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_23_tx_fault_event,        cpld,           ETH_23_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_24_tx_fault_event,        cpld,           ETH_24_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_25_tx_fault_event,        cpld,           ETH_25_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_26_tx_fault_event,        cpld,           ETH_26_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_27_tx_fault_event,        cpld,           ETH_27_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_28_tx_fault_event,        cpld,           ETH_28_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_29_tx_fault_event,        cpld,           ETH_29_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_30_tx_fault_event,        cpld,           ETH_30_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_31_tx_fault_event,        cpld,           ETH_31_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_32_tx_fault_event,        cpld,           ETH_32_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_33_tx_fault_event,        cpld,           ETH_33_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_34_tx_fault_event,        cpld,           ETH_34_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_35_tx_fault_event,        cpld,           ETH_35_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_36_tx_fault_event,        cpld,           ETH_36_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_37_tx_fault_event,        cpld,           ETH_37_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_38_tx_fault_event,        cpld,           ETH_38_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_39_tx_fault_event,        cpld,           ETH_39_TX_FAULT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_16_i2c_stuck_event,       cpld,           ETH_16_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_17_i2c_stuck_event,       cpld,           ETH_17_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_18_i2c_stuck_event,       cpld,           ETH_18_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_19_i2c_stuck_event,       cpld,           ETH_19_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_20_i2c_stuck_event,       cpld,           ETH_20_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_21_i2c_stuck_event,       cpld,           ETH_21_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_22_i2c_stuck_event,       cpld,           ETH_22_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_23_i2c_stuck_event,       cpld,           ETH_23_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_24_i2c_stuck_event,       cpld,           ETH_24_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_25_i2c_stuck_event,       cpld,           ETH_25_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_26_i2c_stuck_event,       cpld,           ETH_26_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_27_i2c_stuck_event,       cpld,           ETH_27_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_28_i2c_stuck_event,       cpld,           ETH_28_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_29_i2c_stuck_event,       cpld,           ETH_29_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_30_i2c_stuck_event,       cpld,           ETH_30_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_31_i2c_stuck_event,       cpld,           ETH_31_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_32_i2c_stuck_event,       cpld,           ETH_32_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_33_i2c_stuck_event,       cpld,           ETH_33_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_34_i2c_stuck_event,       cpld,           ETH_34_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_35_i2c_stuck_event,       cpld,           ETH_35_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_36_i2c_stuck_event,       cpld,           ETH_36_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_37_i2c_stuck_event,       cpld,           ETH_37_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_38_i2c_stuck_event,       cpld,           ETH_38_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_39_i2c_stuck_event,       cpld,           ETH_39_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld4_to_cpld1_intr_event,    cpld,           CPLD4_TO_CPLD1_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld4_i2c_stuck_event,        cpld,           CPLD4_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RW(eth_16_tx_disable,            cpld,           ETH_16_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_17_tx_disable,            cpld,           ETH_17_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_18_tx_disable,            cpld,           ETH_18_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_19_tx_disable,            cpld,           ETH_19_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_20_tx_disable,            cpld,           ETH_20_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_21_tx_disable,            cpld,           ETH_21_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_22_tx_disable,            cpld,           ETH_22_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_23_tx_disable,            cpld,           ETH_23_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_24_tx_disable,            cpld,           ETH_24_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_25_tx_disable,            cpld,           ETH_25_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_26_tx_disable,            cpld,           ETH_26_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_27_tx_disable,            cpld,           ETH_27_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_28_tx_disable,            cpld,           ETH_28_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_29_tx_disable,            cpld,           ETH_29_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_30_tx_disable,            cpld,           ETH_30_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_31_tx_disable,            cpld,           ETH_31_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_32_tx_disable,            cpld,           ETH_32_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_33_tx_disable,            cpld,           ETH_33_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_34_tx_disable,            cpld,           ETH_34_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_35_tx_disable,            cpld,           ETH_35_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_36_tx_disable,            cpld,           ETH_36_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_37_tx_disable,            cpld,           ETH_37_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_38_tx_disable,            cpld,           ETH_38_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_39_tx_disable,            cpld,           ETH_39_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_16_rate_sel,              cpld,           ETH_16_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_17_rate_sel,              cpld,           ETH_17_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_18_rate_sel,              cpld,           ETH_18_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_19_rate_sel,              cpld,           ETH_19_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_20_rate_sel,              cpld,           ETH_20_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_21_rate_sel,              cpld,           ETH_21_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_22_rate_sel,              cpld,           ETH_22_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_23_rate_sel,              cpld,           ETH_23_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_24_rate_sel,              cpld,           ETH_24_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_25_rate_sel,              cpld,           ETH_25_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_26_rate_sel,              cpld,           ETH_26_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_27_rate_sel,              cpld,           ETH_27_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_28_rate_sel,              cpld,           ETH_28_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_29_rate_sel,              cpld,           ETH_29_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_30_rate_sel,              cpld,           ETH_30_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_31_rate_sel,              cpld,           ETH_31_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_32_rate_sel,              cpld,           ETH_32_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_33_rate_sel,              cpld,           ETH_33_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_34_rate_sel,              cpld,           ETH_34_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_35_rate_sel,              cpld,           ETH_35_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_36_rate_sel,              cpld,           ETH_36_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_37_rate_sel,              cpld,           ETH_37_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_38_rate_sel,              cpld,           ETH_38_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(eth_39_rate_sel,              cpld,           ETH_39_RATE_SEL);
static SENSOR_DEVICE_ATTR_RO(cpld4_pwr_status_0,           cpld,           CPLD4_PWR_STATUS_0);
static SENSOR_DEVICE_ATTR_RO(cpld4_pwr_status_1,           cpld,           CPLD4_PWR_STATUS_1);
static SENSOR_DEVICE_ATTR_RO(cpld4_pwr_status_2,           cpld,           CPLD4_PWR_STATUS_2);
static SENSOR_DEVICE_ATTR_RO(cpld4_pwr_status_3,           cpld,           CPLD4_PWR_STATUS_3);
static SENSOR_DEVICE_ATTR_RW(dbg_eth_16_23_present,        cpld,           DBG_SFP56_16_23_ABS);
static SENSOR_DEVICE_ATTR_RW(dbg_eth_24_31_present,        cpld,           DBG_SFP56_24_31_ABS);
static SENSOR_DEVICE_ATTR_RW(dbg_eth_32_39_present,        cpld,           DBG_SFP56_32_39_ABS);
static SENSOR_DEVICE_ATTR_RW(dbg_eth_16_23_rx_los,         cpld,           DBG_SFP56_16_23_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(dbg_eth_24_31_rx_los,         cpld,           DBG_SFP56_24_31_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(dbg_eth_32_39_rx_los,         cpld,           DBG_SFP56_32_39_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(dbg_eth_16_23_tx_fault,       cpld,           DBG_SFP56_16_23_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(dbg_eth_24_31_tx_fault,       cpld,           DBG_SFP56_24_31_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(dbg_eth_32_39_tx_fault,       cpld,           DBG_SFP56_32_39_TX_FAULT);

/******************************************************************************
 * BSP DEBUG                                                                    *
 ******************************************************************************/
//BSP DEBUG
static SENSOR_DEVICE_ATTR_RW(bsp_debug,                  bsp_callback,     BSP_DEBUG);
static SENSOR_DEVICE_ATTR_RO(bsp_wp_access_count,             bsp_callback, BSP_WP_ACCESS_COUNT);

//MUX
static SENSOR_DEVICE_ATTR_RW(idle_state, idle_state, IDLE_STATE);

/* define support attributes of cpldx */
static struct attribute *cpld1_common_attributes[] = {
    /* cpld_common start */
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_chip_type),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(event_detect_ctrl),
    _DEVICE_ATTR(module_reset),
    _DEVICE_ATTR(cpld_test),
    /* cpld_common end */
    _DEVICE_ATTR(brd_sku_id),
    _DEVICE_ATTR(brd_hw_id),
    _DEVICE_ATTR(brd_deph_id),
    _DEVICE_ATTR(brd_build_id),
    _DEVICE_ATTR(brd_id_type),
    _DEVICE_ATTR(cpld_board_ext_id),
    _DEVICE_ATTR(gddr6_id),
    _DEVICE_ATTR(gddr6_id_func),
    _DEVICE_ATTR(clk_ptp_intr),
    _DEVICE_ATTR(phy_intr),
    _DEVICE_ATTR(top_brd_cpld_fru_intr),
    _DEVICE_ATTR(psu0_intr),
    _DEVICE_ATTR(psu1_intr),
    _DEVICE_ATTR(cpld2_intr),
    _DEVICE_ATTR(main_brd_cpld_intr),
    _DEVICE_ATTR(cpld3_intr),
    _DEVICE_ATTR(cpld4_intr),
    _DEVICE_ATTR(mb_eth_intr),
    _DEVICE_ATTR(fan_intr),
    _DEVICE_ATTR(thermal_intr),
    _DEVICE_ATTR(usb_ssd_intr),
    _DEVICE_ATTR(cpu_nmi_intr),
    _DEVICE_ATTR(out_status_intr),
    _DEVICE_ATTR(clk_ptp_intr_mask),
    _DEVICE_ATTR(phy_intr_mask),
    _DEVICE_ATTR(top_brd_cpld_fru_intr_mask),
    _DEVICE_ATTR(psu0_intr_mask),
    _DEVICE_ATTR(psu1_intr_mask),
    _DEVICE_ATTR(cpld2_intr_mask),
    _DEVICE_ATTR(cpld2_io_intr_mask),
    _DEVICE_ATTR(main_brd_cpld_intr_mask),
    _DEVICE_ATTR(cpld3_intr_mask),
    _DEVICE_ATTR(cpld4_intr_mask),
    _DEVICE_ATTR(mb_eth_intr_mask),
    _DEVICE_ATTR(mb_ptp_intr_mask),
    _DEVICE_ATTR(fan_intr_mask),
    _DEVICE_ATTR(thermal_intr_mask),
    _DEVICE_ATTR(usb_ssd_intr_mask),
    _DEVICE_ATTR(cpu_nmi_intr_mask),
    _DEVICE_ATTR(out_status_intr_mask),
    _DEVICE_ATTR(clk_ptp_intr_event),
    _DEVICE_ATTR(phy_intr_event),
    _DEVICE_ATTR(top_brd_cpld_fru_intr_event),
    _DEVICE_ATTR(main_brd_cpld_intr_event),
    _DEVICE_ATTR(thermal_intr_event),
    _DEVICE_ATTR(usb_ssd_intr_event),
    _DEVICE_ATTR(cpu_nmi_intr_event),
    _DEVICE_ATTR(out_status_intr_event),
    _DEVICE_ATTR(btn_fp_reset),
    _DEVICE_ATTR(spi_bios_reset),
    _DEVICE_ATTR(cpu_board_ctrl),
    _DEVICE_ATTR(rgb_1_reset),
    _DEVICE_ATTR(rgb_0_reset),
    _DEVICE_ATTR(bmc_lpc_reset),
    _DEVICE_ATTR(bmc_pcie_reset),
    _DEVICE_ATTR(cpld_to_bmc_sys_reset),
    _DEVICE_ATTR(cpld_to_cpu_reset),
    _DEVICE_ATTR(usb_pwr_en),
    _DEVICE_ATTR(usb_sie_reset),
    _DEVICE_ATTR(i2c_mux_sys_reset),
    _DEVICE_ATTR(i2c_mux_smbus_reset),
    _DEVICE_ATTR(i2c_mux_eth_28_reset),
    _DEVICE_ATTR(i2c_mux_sfp_reset),
    _DEVICE_ATTR(mb_i2c_reset),
    _DEVICE_ATTR(bmc_present),
    _DEVICE_ATTR(sata_ssd1_present),
    _DEVICE_ATTR(sata_ssd2_present),
    _DEVICE_ATTR(psu0_present),
    _DEVICE_ATTR(psu1_present),
    _DEVICE_ATTR(psu0_vin_pg),
    _DEVICE_ATTR(psu1_vin_pg),
    _DEVICE_ATTR(psu0_vout_pg),
    _DEVICE_ATTR(psu1_vout_pg),
    _DEVICE_ATTR(psu_status),
    _DEVICE_ATTR(cpu_boot_done),
    _DEVICE_ATTR(cpu_pg),
    _DEVICE_ATTR(cpu_status),
    _DEVICE_ATTR(phy_boot_ctrl),
    _DEVICE_ATTR(wd_status),
    _DEVICE_ATTR(timing_ctrl_status),
    _DEVICE_ATTR(smbus_peci_dis),
    _DEVICE_ATTR(i2c_psu0_mux_sel),
    _DEVICE_ATTR(i2c_psu1_mux_sel),
    _DEVICE_ATTR(i2c_cpld_mux_sel),
    _DEVICE_ATTR(bmc_usb_mux_sel),
    _DEVICE_ATTR(uart_cpu_bmc_mux_sel),
    _DEVICE_ATTR(uart_mux_sel),
    _DEVICE_ATTR(pwr_system_ctrl),
    _DEVICE_ATTR(serboot_ufm_store),
    _DEVICE_ATTR(serboot_ufm_write),
    _DEVICE_ATTR(cpld_write_protect_1),
    _DEVICE_ATTR(cpld_write_protect_2),
    _DEVICE_ATTR(ext_ctrl),
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
    _DEVICE_ATTR(cpld_system_led_pwr),
    _DEVICE_ATTR(pwr_led_status),
    _DEVICE_ATTR(pwr_led_speed),
    _DEVICE_ATTR(pwr_led_blink),
    _DEVICE_ATTR(pwr_led_onoff),
    _DEVICE_ATTR(cpld_system_led_gnss),
    _DEVICE_ATTR(gnss_led_status),
    _DEVICE_ATTR(gnss_led_speed),
    _DEVICE_ATTR(gnss_led_blink),
    _DEVICE_ATTR(gnss_led_onoff),
    _DEVICE_ATTR(cpld_system_led_sync),
    _DEVICE_ATTR(sync_led_status),
    _DEVICE_ATTR(sync_led_speed),
    _DEVICE_ATTR(sync_led_blink),
    _DEVICE_ATTR(sync_led_onoff),
    _DEVICE_ATTR(led_clear),
    _DEVICE_ATTR(cpld1_pwr_status),
    _DEVICE_ATTR(eth_0_pwr_en),
    _DEVICE_ATTR(eth_1_pwr_en),
    _DEVICE_ATTR(eth_2_pwr_en),
    _DEVICE_ATTR(eth_3_pwr_en),
    _DEVICE_ATTR(eth_4_pwr_en),
    _DEVICE_ATTR(eth_5_pwr_en),
    _DEVICE_ATTR(eth_12_pwr_en),
    _DEVICE_ATTR(eth_13_pwr_en),
    _DEVICE_ATTR(eth_14_pwr_en),
    _DEVICE_ATTR(eth_15_pwr_en),
    _DEVICE_ATTR(eth_6_pwr_en),
    _DEVICE_ATTR(eth_7_pwr_en),
    _DEVICE_ATTR(eth_8_pwr_en),
    _DEVICE_ATTR(eth_9_pwr_en),
    _DEVICE_ATTR(eth_10_pwr_en),
    _DEVICE_ATTR(eth_11_pwr_en),
    _DEVICE_ATTR(eth_16_pwr_en),
    _DEVICE_ATTR(eth_17_pwr_en),
    _DEVICE_ATTR(eth_18_pwr_en),
    _DEVICE_ATTR(eth_19_pwr_en),
    _DEVICE_ATTR(eth_20_pwr_en),
    _DEVICE_ATTR(eth_21_pwr_en),
    _DEVICE_ATTR(eth_22_pwr_en),
    _DEVICE_ATTR(eth_23_pwr_en),
    _DEVICE_ATTR(eth_24_pwr_en),
    _DEVICE_ATTR(eth_25_pwr_en),
    _DEVICE_ATTR(eth_26_pwr_en),
    _DEVICE_ATTR(eth_27_pwr_en),
    _DEVICE_ATTR(eth_28_pwr_en),
    _DEVICE_ATTR(eth_29_pwr_en),
    _DEVICE_ATTR(eth_30_pwr_en),
    _DEVICE_ATTR(eth_31_pwr_en),
    _DEVICE_ATTR(eth_32_pwr_en),
    _DEVICE_ATTR(eth_33_pwr_en),
    _DEVICE_ATTR(eth_34_pwr_en),
    _DEVICE_ATTR(eth_35_pwr_en),
    _DEVICE_ATTR(eth_36_pwr_en),
    _DEVICE_ATTR(eth_37_pwr_en),
    _DEVICE_ATTR(eth_38_pwr_en),
    _DEVICE_ATTR(eth_39_pwr_en),
    _DEVICE_ATTR(ocxo_gnss_id),
    _DEVICE_ATTR(clk_ptp_reset),
    _DEVICE_ATTR(cjap_reset),
    _DEVICE_ATTR(ntm_reset),
    _DEVICE_ATTR(gnss_reset),
    _DEVICE_ATTR(bits_reset),
    _DEVICE_ATTR(clk_timing_ctrl),
    _DEVICE_ATTR(gnss_status),
    _DEVICE_ATTR(timing_status),
    _DEVICE_ATTR(qsfpdd_sel),
    _DEVICE_ATTR(eth_28_sel),
    _DEVICE_ATTR(i2c_mux_0x76_reset),
    _DEVICE_ATTR(i2c_mux_0x75_reset),
    _DEVICE_ATTR(i2c_mux_6_11_reset),
    _DEVICE_ATTR(i2c_mux_eth_16_23_reset),
    _DEVICE_ATTR(i2c_mux_eth_24_31_reset),
    _DEVICE_ATTR(i2c_mux_eth_32_39_reset),
    _DEVICE_ATTR(i2c_mux_0x71_reset),
    _DEVICE_ATTR(bsp_debug),
    _DEVICE_ATTR(bsp_wp_access_count),
    NULL,
};

static struct attribute *cpld1_beta_attributes[] = {
    _DEVICE_ATTR(cpld_system_led_id),
    _DEVICE_ATTR(id_led_speed),
    _DEVICE_ATTR(id_led_blink),
    _DEVICE_ATTR(id_led_onoff),
    NULL,
};

static struct attribute *cpld2_common_attributes[] = {
    /* cpld_common start */
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_chip_type),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(event_detect_ctrl),
    _DEVICE_ATTR(module_reset),
    _DEVICE_ATTR(cpld_test),
    /* cpld_common end */
    _DEVICE_ATTR(eth_0_present),
    _DEVICE_ATTR(eth_1_present),
    _DEVICE_ATTR(eth_2_present),
    _DEVICE_ATTR(eth_3_present),
    _DEVICE_ATTR(eth_4_present),
    _DEVICE_ATTR(eth_5_present),
    _DEVICE_ATTR(eth_12_present),
    _DEVICE_ATTR(eth_13_present),
    _DEVICE_ATTR(eth_14_present),
    _DEVICE_ATTR(eth_15_present),
    _DEVICE_ATTR(eth_0_intr),
    _DEVICE_ATTR(eth_1_intr),
    _DEVICE_ATTR(eth_2_intr),
    _DEVICE_ATTR(eth_3_intr),
    _DEVICE_ATTR(eth_4_intr),
    _DEVICE_ATTR(eth_5_intr),
    _DEVICE_ATTR(eth_12_intr),
    _DEVICE_ATTR(eth_13_intr),
    _DEVICE_ATTR(eth_14_intr),
    _DEVICE_ATTR(eth_15_intr),
    _DEVICE_ATTR(eth_0_efuse_pg),
    _DEVICE_ATTR(eth_1_efuse_pg),
    _DEVICE_ATTR(eth_2_efuse_pg),
    _DEVICE_ATTR(eth_3_efuse_pg),
    _DEVICE_ATTR(eth_4_efuse_pg),
    _DEVICE_ATTR(eth_5_efuse_pg),
    _DEVICE_ATTR(eth_12_efuse_pg),
    _DEVICE_ATTR(eth_13_efuse_pg),
    _DEVICE_ATTR(eth_14_efuse_pg),
    _DEVICE_ATTR(eth_15_efuse_pg),
    _DEVICE_ATTR(eth_0_present_mask),
    _DEVICE_ATTR(eth_1_present_mask),
    _DEVICE_ATTR(eth_2_present_mask),
    _DEVICE_ATTR(eth_3_present_mask),
    _DEVICE_ATTR(eth_4_present_mask),
    _DEVICE_ATTR(eth_5_present_mask),
    _DEVICE_ATTR(eth_12_present_mask),
    _DEVICE_ATTR(eth_13_present_mask),
    _DEVICE_ATTR(eth_14_present_mask),
    _DEVICE_ATTR(eth_15_present_mask),
    _DEVICE_ATTR(eth_0_intr_mask),
    _DEVICE_ATTR(eth_1_intr_mask),
    _DEVICE_ATTR(eth_2_intr_mask),
    _DEVICE_ATTR(eth_3_intr_mask),
    _DEVICE_ATTR(eth_4_intr_mask),
    _DEVICE_ATTR(eth_5_intr_mask),
    _DEVICE_ATTR(eth_12_intr_mask),
    _DEVICE_ATTR(eth_13_intr_mask),
    _DEVICE_ATTR(eth_14_intr_mask),
    _DEVICE_ATTR(eth_15_intr_mask),
    _DEVICE_ATTR(eth_0_efuse_pg_mask),
    _DEVICE_ATTR(eth_1_efuse_pg_mask),
    _DEVICE_ATTR(eth_2_efuse_pg_mask),
    _DEVICE_ATTR(eth_3_efuse_pg_mask),
    _DEVICE_ATTR(eth_4_efuse_pg_mask),
    _DEVICE_ATTR(eth_5_efuse_pg_mask),
    _DEVICE_ATTR(eth_12_efuse_pg_mask),
    _DEVICE_ATTR(eth_13_efuse_pg_mask),
    _DEVICE_ATTR(eth_14_efuse_pg_mask),
    _DEVICE_ATTR(eth_15_efuse_pg_mask),
    _DEVICE_ATTR(eth_0_present_event),
    _DEVICE_ATTR(eth_1_present_event),
    _DEVICE_ATTR(eth_2_present_event),
    _DEVICE_ATTR(eth_3_present_event),
    _DEVICE_ATTR(eth_4_present_event),
    _DEVICE_ATTR(eth_5_present_event),
    _DEVICE_ATTR(eth_12_present_event),
    _DEVICE_ATTR(eth_13_present_event),
    _DEVICE_ATTR(eth_14_present_event),
    _DEVICE_ATTR(eth_15_present_event),
    _DEVICE_ATTR(eth_0_intr_event),
    _DEVICE_ATTR(eth_1_intr_event),
    _DEVICE_ATTR(eth_2_intr_event),
    _DEVICE_ATTR(eth_3_intr_event),
    _DEVICE_ATTR(eth_4_intr_event),
    _DEVICE_ATTR(eth_5_intr_event),
    _DEVICE_ATTR(eth_12_intr_event),
    _DEVICE_ATTR(eth_13_intr_event),
    _DEVICE_ATTR(eth_14_intr_event),
    _DEVICE_ATTR(eth_15_intr_event),
    _DEVICE_ATTR(eth_0_efuse_pg_event),
    _DEVICE_ATTR(eth_1_efuse_pg_event),
    _DEVICE_ATTR(eth_2_efuse_pg_event),
    _DEVICE_ATTR(eth_3_efuse_pg_event),
    _DEVICE_ATTR(eth_4_efuse_pg_event),
    _DEVICE_ATTR(eth_5_efuse_pg_event),
    _DEVICE_ATTR(eth_12_efuse_pg_event),
    _DEVICE_ATTR(eth_13_efuse_pg_event),
    _DEVICE_ATTR(eth_14_efuse_pg_event),
    _DEVICE_ATTR(eth_15_efuse_pg_event),
    _DEVICE_ATTR(eth_0_reset),
    _DEVICE_ATTR(eth_1_reset),
    _DEVICE_ATTR(eth_2_reset),
    _DEVICE_ATTR(eth_3_reset),
    _DEVICE_ATTR(eth_4_reset),
    _DEVICE_ATTR(eth_5_reset),
    _DEVICE_ATTR(eth_12_reset),
    _DEVICE_ATTR(eth_13_reset),
    _DEVICE_ATTR(eth_14_reset),
    _DEVICE_ATTR(eth_15_reset),
    _DEVICE_ATTR(eth_0_lpmode),
    _DEVICE_ATTR(eth_1_lpmode),
    _DEVICE_ATTR(eth_2_lpmode),
    _DEVICE_ATTR(eth_3_lpmode),
    _DEVICE_ATTR(eth_4_lpmode),
    _DEVICE_ATTR(eth_5_lpmode),
    _DEVICE_ATTR(eth_12_lpmode),
    _DEVICE_ATTR(eth_13_lpmode),
    _DEVICE_ATTR(eth_14_lpmode),
    _DEVICE_ATTR(eth_15_lpmode),
    _DEVICE_ATTR(clk_en_ctrl),
    _DEVICE_ATTR(psu_ctrl),
    _DEVICE_ATTR(eth_12_led_ctrl),
    _DEVICE_ATTR(eth_13_led_ctrl),
    _DEVICE_ATTR(eth_14_led_ctrl),
    _DEVICE_ATTR(eth_15_led_ctrl),
    _DEVICE_ATTR(cpld2_pwr_status_0),
    _DEVICE_ATTR(cpld2_pwr_status_1),
    _DEVICE_ATTR(dbg_eth_0_5_present),
    _DEVICE_ATTR(dbg_eth_12_15_present),
    _DEVICE_ATTR(dbg_eth_0_5_intr),
    _DEVICE_ATTR(dbg_eth_12_15_intr),
    _DEVICE_ATTR(dbg_eth_0_5_efuse_pg),
    _DEVICE_ATTR(dbg_eth_12_15_efuse_pg),
    _DEVICE_ATTR(bsp_debug),
    NULL,
};

static struct attribute *cpld2_beta_attributes[] = {
    _DEVICE_ATTR(cpld_i2c_control),
    _DEVICE_ATTR(cpld_i2c_relay),
    _DEVICE_ATTR(eth_0_i2c_stuck),
    _DEVICE_ATTR(eth_1_i2c_stuck),
    _DEVICE_ATTR(eth_2_i2c_stuck),
    _DEVICE_ATTR(eth_3_i2c_stuck),
    _DEVICE_ATTR(eth_4_i2c_stuck),
    _DEVICE_ATTR(eth_5_i2c_stuck),
    _DEVICE_ATTR(eth_12_i2c_stuck),
    _DEVICE_ATTR(eth_13_i2c_stuck),
    _DEVICE_ATTR(eth_14_i2c_stuck),
    _DEVICE_ATTR(eth_15_i2c_stuck),
    _DEVICE_ATTR(cpld2_i2c_stuck),
    _DEVICE_ATTR(cpld2_to_cpld1_intr),

    _DEVICE_ATTR(cpld2_i2c_stuck_mask),
    _DEVICE_ATTR(eth_0_i2c_stuck_mask),
    _DEVICE_ATTR(eth_1_i2c_stuck_mask),
    _DEVICE_ATTR(eth_2_i2c_stuck_mask),
    _DEVICE_ATTR(eth_3_i2c_stuck_mask),
    _DEVICE_ATTR(eth_4_i2c_stuck_mask),
    _DEVICE_ATTR(eth_5_i2c_stuck_mask),
    _DEVICE_ATTR(eth_12_i2c_stuck_mask),
    _DEVICE_ATTR(eth_13_i2c_stuck_mask),
    _DEVICE_ATTR(eth_14_i2c_stuck_mask),
    _DEVICE_ATTR(eth_15_i2c_stuck_mask),
    _DEVICE_ATTR(cpld2_to_cpld1_intr_mask),

    _DEVICE_ATTR(eth_0_i2c_stuck_event),
    _DEVICE_ATTR(eth_1_i2c_stuck_event),
    _DEVICE_ATTR(eth_2_i2c_stuck_event),
    _DEVICE_ATTR(eth_3_i2c_stuck_event),
    _DEVICE_ATTR(eth_4_i2c_stuck_event),
    _DEVICE_ATTR(eth_5_i2c_stuck_event),
    _DEVICE_ATTR(eth_12_i2c_stuck_event),
    _DEVICE_ATTR(eth_13_i2c_stuck_event),
    _DEVICE_ATTR(eth_14_i2c_stuck_event),
    _DEVICE_ATTR(eth_15_i2c_stuck_event),
    _DEVICE_ATTR(cpld2_i2c_stuck_event),
    _DEVICE_ATTR(cpld2_to_cpld1_intr_event),
    NULL,
};

static struct attribute *cpld3_common_attributes[] = {
    /* cpld_common start */
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_chip_type),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(event_detect_ctrl),
    _DEVICE_ATTR(module_reset),
    _DEVICE_ATTR(cpld_test),
    /* cpld_common end */
    _DEVICE_ATTR(gnss_model_id),
    _DEVICE_ATTR(ocxo_id),
    _DEVICE_ATTR(eth_6_present),
    _DEVICE_ATTR(eth_7_present),
    _DEVICE_ATTR(eth_8_present),
    _DEVICE_ATTR(eth_9_present),
    _DEVICE_ATTR(eth_10_present),
    _DEVICE_ATTR(eth_11_present),
    _DEVICE_ATTR(eth_6_intr),
    _DEVICE_ATTR(eth_7_intr),
    _DEVICE_ATTR(eth_8_intr),
    _DEVICE_ATTR(eth_9_intr),
    _DEVICE_ATTR(eth_10_intr),
    _DEVICE_ATTR(eth_11_intr),
    _DEVICE_ATTR(eth_6_efuse_pg),
    _DEVICE_ATTR(eth_7_efuse_pg),
    _DEVICE_ATTR(eth_8_efuse_pg),
    _DEVICE_ATTR(eth_9_efuse_pg),
    _DEVICE_ATTR(eth_10_efuse_pg),
    _DEVICE_ATTR(eth_11_efuse_pg),
    _DEVICE_ATTR(mac_intr),
    _DEVICE_ATTR(main_thermal_intr),
    _DEVICE_ATTR(fan_0_present),
    _DEVICE_ATTR(fan_1_present),
    _DEVICE_ATTR(fan_2_present),
    _DEVICE_ATTR(fan_3_present),
    _DEVICE_ATTR(fan_4_present),
    _DEVICE_ATTR(io_oc_intr),
    _DEVICE_ATTR(eth_6_present_mask),
    _DEVICE_ATTR(eth_7_present_mask),
    _DEVICE_ATTR(eth_8_present_mask),
    _DEVICE_ATTR(eth_9_present_mask),
    _DEVICE_ATTR(eth_10_present_mask),
    _DEVICE_ATTR(eth_11_present_mask),
    _DEVICE_ATTR(eth_6_intr_mask),
    _DEVICE_ATTR(eth_7_intr_mask),
    _DEVICE_ATTR(eth_8_intr_mask),
    _DEVICE_ATTR(eth_9_intr_mask),
    _DEVICE_ATTR(eth_10_intr_mask),
    _DEVICE_ATTR(eth_11_intr_mask),
    _DEVICE_ATTR(eth_6_efuse_pg_mask),
    _DEVICE_ATTR(eth_7_efuse_pg_mask),
    _DEVICE_ATTR(eth_8_efuse_pg_mask),
    _DEVICE_ATTR(eth_9_efuse_pg_mask),
    _DEVICE_ATTR(eth_10_efuse_pg_mask),
    _DEVICE_ATTR(eth_11_efuse_pg_mask),
    _DEVICE_ATTR(mac_intr_mask),
    _DEVICE_ATTR(main_thermal_intr_mask),
    _DEVICE_ATTR(fan_present_mask),
    _DEVICE_ATTR(io_oc_intr_mask),
    _DEVICE_ATTR(eth_6_present_event),
    _DEVICE_ATTR(eth_7_present_event),
    _DEVICE_ATTR(eth_8_present_event),
    _DEVICE_ATTR(eth_9_present_event),
    _DEVICE_ATTR(eth_10_present_event),
    _DEVICE_ATTR(eth_11_present_event),
    _DEVICE_ATTR(eth_6_intr_event),
    _DEVICE_ATTR(eth_7_intr_event),
    _DEVICE_ATTR(eth_8_intr_event),
    _DEVICE_ATTR(eth_9_intr_event),
    _DEVICE_ATTR(eth_10_intr_event),
    _DEVICE_ATTR(eth_11_intr_event),
    _DEVICE_ATTR(eth_6_efuse_pg_event),
    _DEVICE_ATTR(eth_7_efuse_pg_event),
    _DEVICE_ATTR(eth_8_efuse_pg_event),
    _DEVICE_ATTR(eth_9_efuse_pg_event),
    _DEVICE_ATTR(eth_10_efuse_pg_event),
    _DEVICE_ATTR(eth_11_efuse_pg_event),
    _DEVICE_ATTR(mac_intr_event),
    _DEVICE_ATTR(main_thermal_intr_event),
    _DEVICE_ATTR(fan_present_event),
    _DEVICE_ATTR(io_oc_intr_event),
    _DEVICE_ATTR(eth_6_reset),
    _DEVICE_ATTR(eth_7_reset),
    _DEVICE_ATTR(eth_8_reset),
    _DEVICE_ATTR(eth_9_reset),
    _DEVICE_ATTR(eth_10_reset),
    _DEVICE_ATTR(eth_11_reset),
    _DEVICE_ATTR(eth_6_lpmode),
    _DEVICE_ATTR(eth_7_lpmode),
    _DEVICE_ATTR(eth_8_lpmode),
    _DEVICE_ATTR(eth_9_lpmode),
    _DEVICE_ATTR(eth_10_lpmode),
    _DEVICE_ATTR(eth_11_lpmode),
    _DEVICE_ATTR(mac_reset),
    _DEVICE_ATTR(usb_qspi_reset),
    _DEVICE_ATTR(clk_timing_status_1),
    _DEVICE_ATTR(clk_timing_status_2),
    _DEVICE_ATTR(rov_status),
    _DEVICE_ATTR(misc_control),
    _DEVICE_ATTR(gnss_ctrl),
    _DEVICE_ATTR(synce_ctrl),
    _DEVICE_ATTR(fan_speed_read_mode),
    _DEVICE_ATTR(cpld3_pwr_status),
    _DEVICE_ATTR(dbg_eth_6_11_present),
    _DEVICE_ATTR(dbg_eth_6_11_intr),
    _DEVICE_ATTR(dbg_eth_6_11_efuse_pg),
    _DEVICE_ATTR(dbg_mac_intr),
    _DEVICE_ATTR(dbg_thermal_intr),
    _DEVICE_ATTR(dbg_misc_intr),
    _DEVICE_ATTR(dbg_eth_16_23_tx_fault),
    _DEVICE_ATTR(bsp_debug),
    NULL,
};

static struct attribute *cpld3_beta_attributes[] = {
    _DEVICE_ATTR(cpld_i2c_control),
    _DEVICE_ATTR(cpld_i2c_relay),
    _DEVICE_ATTR(eth_6_i2c_stuck),
    _DEVICE_ATTR(eth_7_i2c_stuck),
    _DEVICE_ATTR(eth_8_i2c_stuck),
    _DEVICE_ATTR(eth_9_i2c_stuck),
    _DEVICE_ATTR(eth_10_i2c_stuck),
    _DEVICE_ATTR(eth_11_i2c_stuck),
    _DEVICE_ATTR(cpld3_i2c_stuck),
    _DEVICE_ATTR(cpld3_to_cpld1_intr),
    _DEVICE_ATTR(eth_6_i2c_stuck_mask),
    _DEVICE_ATTR(eth_7_i2c_stuck_mask),
    _DEVICE_ATTR(eth_8_i2c_stuck_mask),
    _DEVICE_ATTR(eth_9_i2c_stuck_mask),
    _DEVICE_ATTR(eth_10_i2c_stuck_mask),
    _DEVICE_ATTR(eth_11_i2c_stuck_mask),
    _DEVICE_ATTR(cpld3_i2c_stuck_mask),
    _DEVICE_ATTR(cpld3_to_cpld1_intr_mask),
    _DEVICE_ATTR(eth_6_i2c_stuck_event),
    _DEVICE_ATTR(eth_7_i2c_stuck_event),
    _DEVICE_ATTR(eth_8_i2c_stuck_event),
    _DEVICE_ATTR(eth_9_i2c_stuck_event),
    _DEVICE_ATTR(eth_10_i2c_stuck_event),
    _DEVICE_ATTR(eth_11_i2c_stuck_event),
    _DEVICE_ATTR(cpld3_i2c_stuck_event),
    _DEVICE_ATTR(cpld3_to_cpld1_intr_event),
    NULL,
};

static struct attribute *cpld4_common_attributes[] = {
    /* cpld_common start */
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_chip_type),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(event_detect_ctrl),
    _DEVICE_ATTR(module_reset),
    _DEVICE_ATTR(cpld_test),
    /* cpld_common end */
    _DEVICE_ATTR(eth_16_present),
    _DEVICE_ATTR(eth_17_present),
    _DEVICE_ATTR(eth_18_present),
    _DEVICE_ATTR(eth_19_present),
    _DEVICE_ATTR(eth_20_present),
    _DEVICE_ATTR(eth_21_present),
    _DEVICE_ATTR(eth_22_present),
    _DEVICE_ATTR(eth_23_present),
    _DEVICE_ATTR(eth_24_present),
    _DEVICE_ATTR(eth_25_present),
    _DEVICE_ATTR(eth_26_present),
    _DEVICE_ATTR(eth_27_present),
    _DEVICE_ATTR(eth_28_present),
    _DEVICE_ATTR(eth_29_present),
    _DEVICE_ATTR(eth_30_present),
    _DEVICE_ATTR(eth_31_present),
    _DEVICE_ATTR(eth_32_present),
    _DEVICE_ATTR(eth_33_present),
    _DEVICE_ATTR(eth_34_present),
    _DEVICE_ATTR(eth_35_present),
    _DEVICE_ATTR(eth_36_present),
    _DEVICE_ATTR(eth_37_present),
    _DEVICE_ATTR(eth_38_present),
    _DEVICE_ATTR(eth_39_present),
    _DEVICE_ATTR(eth_16_rx_los),
    _DEVICE_ATTR(eth_17_rx_los),
    _DEVICE_ATTR(eth_18_rx_los),
    _DEVICE_ATTR(eth_19_rx_los),
    _DEVICE_ATTR(eth_20_rx_los),
    _DEVICE_ATTR(eth_21_rx_los),
    _DEVICE_ATTR(eth_22_rx_los),
    _DEVICE_ATTR(eth_23_rx_los),
    _DEVICE_ATTR(eth_24_rx_los),
    _DEVICE_ATTR(eth_25_rx_los),
    _DEVICE_ATTR(eth_26_rx_los),
    _DEVICE_ATTR(eth_27_rx_los),
    _DEVICE_ATTR(eth_28_rx_los),
    _DEVICE_ATTR(eth_29_rx_los),
    _DEVICE_ATTR(eth_30_rx_los),
    _DEVICE_ATTR(eth_31_rx_los),
    _DEVICE_ATTR(eth_32_rx_los),
    _DEVICE_ATTR(eth_33_rx_los),
    _DEVICE_ATTR(eth_34_rx_los),
    _DEVICE_ATTR(eth_35_rx_los),
    _DEVICE_ATTR(eth_36_rx_los),
    _DEVICE_ATTR(eth_37_rx_los),
    _DEVICE_ATTR(eth_38_rx_los),
    _DEVICE_ATTR(eth_39_rx_los),
    _DEVICE_ATTR(eth_16_tx_fault),
    _DEVICE_ATTR(eth_17_tx_fault),
    _DEVICE_ATTR(eth_18_tx_fault),
    _DEVICE_ATTR(eth_19_tx_fault),
    _DEVICE_ATTR(eth_20_tx_fault),
    _DEVICE_ATTR(eth_21_tx_fault),
    _DEVICE_ATTR(eth_22_tx_fault),
    _DEVICE_ATTR(eth_23_tx_fault),
    _DEVICE_ATTR(eth_24_tx_fault),
    _DEVICE_ATTR(eth_25_tx_fault),
    _DEVICE_ATTR(eth_26_tx_fault),
    _DEVICE_ATTR(eth_27_tx_fault),
    _DEVICE_ATTR(eth_28_tx_fault),
    _DEVICE_ATTR(eth_29_tx_fault),
    _DEVICE_ATTR(eth_30_tx_fault),
    _DEVICE_ATTR(eth_31_tx_fault),
    _DEVICE_ATTR(eth_32_tx_fault),
    _DEVICE_ATTR(eth_33_tx_fault),
    _DEVICE_ATTR(eth_34_tx_fault),
    _DEVICE_ATTR(eth_35_tx_fault),
    _DEVICE_ATTR(eth_36_tx_fault),
    _DEVICE_ATTR(eth_37_tx_fault),
    _DEVICE_ATTR(eth_38_tx_fault),
    _DEVICE_ATTR(eth_39_tx_fault),
    _DEVICE_ATTR(eth_16_present_mask),
    _DEVICE_ATTR(eth_17_present_mask),
    _DEVICE_ATTR(eth_18_present_mask),
    _DEVICE_ATTR(eth_19_present_mask),
    _DEVICE_ATTR(eth_20_present_mask),
    _DEVICE_ATTR(eth_21_present_mask),
    _DEVICE_ATTR(eth_22_present_mask),
    _DEVICE_ATTR(eth_23_present_mask),
    _DEVICE_ATTR(eth_24_present_mask),
    _DEVICE_ATTR(eth_25_present_mask),
    _DEVICE_ATTR(eth_26_present_mask),
    _DEVICE_ATTR(eth_27_present_mask),
    _DEVICE_ATTR(eth_28_present_mask),
    _DEVICE_ATTR(eth_29_present_mask),
    _DEVICE_ATTR(eth_30_present_mask),
    _DEVICE_ATTR(eth_31_present_mask),
    _DEVICE_ATTR(eth_32_present_mask),
    _DEVICE_ATTR(eth_33_present_mask),
    _DEVICE_ATTR(eth_34_present_mask),
    _DEVICE_ATTR(eth_35_present_mask),
    _DEVICE_ATTR(eth_36_present_mask),
    _DEVICE_ATTR(eth_37_present_mask),
    _DEVICE_ATTR(eth_38_present_mask),
    _DEVICE_ATTR(eth_39_present_mask),
    _DEVICE_ATTR(eth_16_rx_los_mask),
    _DEVICE_ATTR(eth_17_rx_los_mask),
    _DEVICE_ATTR(eth_18_rx_los_mask),
    _DEVICE_ATTR(eth_19_rx_los_mask),
    _DEVICE_ATTR(eth_20_rx_los_mask),
    _DEVICE_ATTR(eth_21_rx_los_mask),
    _DEVICE_ATTR(eth_22_rx_los_mask),
    _DEVICE_ATTR(eth_23_rx_los_mask),
    _DEVICE_ATTR(eth_24_rx_los_mask),
    _DEVICE_ATTR(eth_25_rx_los_mask),
    _DEVICE_ATTR(eth_26_rx_los_mask),
    _DEVICE_ATTR(eth_27_rx_los_mask),
    _DEVICE_ATTR(eth_28_rx_los_mask),
    _DEVICE_ATTR(eth_29_rx_los_mask),
    _DEVICE_ATTR(eth_30_rx_los_mask),
    _DEVICE_ATTR(eth_31_rx_los_mask),
    _DEVICE_ATTR(eth_32_rx_los_mask),
    _DEVICE_ATTR(eth_33_rx_los_mask),
    _DEVICE_ATTR(eth_34_rx_los_mask),
    _DEVICE_ATTR(eth_35_rx_los_mask),
    _DEVICE_ATTR(eth_36_rx_los_mask),
    _DEVICE_ATTR(eth_37_rx_los_mask),
    _DEVICE_ATTR(eth_38_rx_los_mask),
    _DEVICE_ATTR(eth_39_rx_los_mask),
    _DEVICE_ATTR(eth_16_tx_fault_mask),
    _DEVICE_ATTR(eth_17_tx_fault_mask),
    _DEVICE_ATTR(eth_18_tx_fault_mask),
    _DEVICE_ATTR(eth_19_tx_fault_mask),
    _DEVICE_ATTR(eth_20_tx_fault_mask),
    _DEVICE_ATTR(eth_21_tx_fault_mask),
    _DEVICE_ATTR(eth_22_tx_fault_mask),
    _DEVICE_ATTR(eth_23_tx_fault_mask),
    _DEVICE_ATTR(eth_24_tx_fault_mask),
    _DEVICE_ATTR(eth_25_tx_fault_mask),
    _DEVICE_ATTR(eth_26_tx_fault_mask),
    _DEVICE_ATTR(eth_27_tx_fault_mask),
    _DEVICE_ATTR(eth_28_tx_fault_mask),
    _DEVICE_ATTR(eth_29_tx_fault_mask),
    _DEVICE_ATTR(eth_30_tx_fault_mask),
    _DEVICE_ATTR(eth_31_tx_fault_mask),
    _DEVICE_ATTR(eth_32_tx_fault_mask),
    _DEVICE_ATTR(eth_33_tx_fault_mask),
    _DEVICE_ATTR(eth_34_tx_fault_mask),
    _DEVICE_ATTR(eth_35_tx_fault_mask),
    _DEVICE_ATTR(eth_36_tx_fault_mask),
    _DEVICE_ATTR(eth_37_tx_fault_mask),
    _DEVICE_ATTR(eth_38_tx_fault_mask),
    _DEVICE_ATTR(eth_39_tx_fault_mask),
    _DEVICE_ATTR(eth_16_present_event),
    _DEVICE_ATTR(eth_17_present_event),
    _DEVICE_ATTR(eth_18_present_event),
    _DEVICE_ATTR(eth_19_present_event),
    _DEVICE_ATTR(eth_20_present_event),
    _DEVICE_ATTR(eth_21_present_event),
    _DEVICE_ATTR(eth_22_present_event),
    _DEVICE_ATTR(eth_23_present_event),
    _DEVICE_ATTR(eth_24_present_event),
    _DEVICE_ATTR(eth_25_present_event),
    _DEVICE_ATTR(eth_26_present_event),
    _DEVICE_ATTR(eth_27_present_event),
    _DEVICE_ATTR(eth_28_present_event),
    _DEVICE_ATTR(eth_29_present_event),
    _DEVICE_ATTR(eth_30_present_event),
    _DEVICE_ATTR(eth_31_present_event),
    _DEVICE_ATTR(eth_32_present_event),
    _DEVICE_ATTR(eth_33_present_event),
    _DEVICE_ATTR(eth_34_present_event),
    _DEVICE_ATTR(eth_35_present_event),
    _DEVICE_ATTR(eth_36_present_event),
    _DEVICE_ATTR(eth_37_present_event),
    _DEVICE_ATTR(eth_38_present_event),
    _DEVICE_ATTR(eth_39_present_event),
    _DEVICE_ATTR(eth_16_rx_los_event),
    _DEVICE_ATTR(eth_17_rx_los_event),
    _DEVICE_ATTR(eth_18_rx_los_event),
    _DEVICE_ATTR(eth_19_rx_los_event),
    _DEVICE_ATTR(eth_20_rx_los_event),
    _DEVICE_ATTR(eth_21_rx_los_event),
    _DEVICE_ATTR(eth_22_rx_los_event),
    _DEVICE_ATTR(eth_23_rx_los_event),
    _DEVICE_ATTR(eth_24_rx_los_event),
    _DEVICE_ATTR(eth_25_rx_los_event),
    _DEVICE_ATTR(eth_26_rx_los_event),
    _DEVICE_ATTR(eth_27_rx_los_event),
    _DEVICE_ATTR(eth_28_rx_los_event),
    _DEVICE_ATTR(eth_29_rx_los_event),
    _DEVICE_ATTR(eth_30_rx_los_event),
    _DEVICE_ATTR(eth_31_rx_los_event),
    _DEVICE_ATTR(eth_32_rx_los_event),
    _DEVICE_ATTR(eth_33_rx_los_event),
    _DEVICE_ATTR(eth_34_rx_los_event),
    _DEVICE_ATTR(eth_35_rx_los_event),
    _DEVICE_ATTR(eth_36_rx_los_event),
    _DEVICE_ATTR(eth_37_rx_los_event),
    _DEVICE_ATTR(eth_38_rx_los_event),
    _DEVICE_ATTR(eth_39_rx_los_event),
    _DEVICE_ATTR(eth_16_tx_fault_event),
    _DEVICE_ATTR(eth_17_tx_fault_event),
    _DEVICE_ATTR(eth_18_tx_fault_event),
    _DEVICE_ATTR(eth_19_tx_fault_event),
    _DEVICE_ATTR(eth_20_tx_fault_event),
    _DEVICE_ATTR(eth_21_tx_fault_event),
    _DEVICE_ATTR(eth_22_tx_fault_event),
    _DEVICE_ATTR(eth_23_tx_fault_event),
    _DEVICE_ATTR(eth_24_tx_fault_event),
    _DEVICE_ATTR(eth_25_tx_fault_event),
    _DEVICE_ATTR(eth_26_tx_fault_event),
    _DEVICE_ATTR(eth_27_tx_fault_event),
    _DEVICE_ATTR(eth_28_tx_fault_event),
    _DEVICE_ATTR(eth_29_tx_fault_event),
    _DEVICE_ATTR(eth_30_tx_fault_event),
    _DEVICE_ATTR(eth_31_tx_fault_event),
    _DEVICE_ATTR(eth_32_tx_fault_event),
    _DEVICE_ATTR(eth_33_tx_fault_event),
    _DEVICE_ATTR(eth_34_tx_fault_event),
    _DEVICE_ATTR(eth_35_tx_fault_event),
    _DEVICE_ATTR(eth_36_tx_fault_event),
    _DEVICE_ATTR(eth_37_tx_fault_event),
    _DEVICE_ATTR(eth_38_tx_fault_event),
    _DEVICE_ATTR(eth_39_tx_fault_event),
    _DEVICE_ATTR(eth_16_tx_disable),
    _DEVICE_ATTR(eth_17_tx_disable),
    _DEVICE_ATTR(eth_18_tx_disable),
    _DEVICE_ATTR(eth_19_tx_disable),
    _DEVICE_ATTR(eth_20_tx_disable),
    _DEVICE_ATTR(eth_21_tx_disable),
    _DEVICE_ATTR(eth_22_tx_disable),
    _DEVICE_ATTR(eth_23_tx_disable),
    _DEVICE_ATTR(eth_24_tx_disable),
    _DEVICE_ATTR(eth_25_tx_disable),
    _DEVICE_ATTR(eth_26_tx_disable),
    _DEVICE_ATTR(eth_27_tx_disable),
    _DEVICE_ATTR(eth_28_tx_disable),
    _DEVICE_ATTR(eth_29_tx_disable),
    _DEVICE_ATTR(eth_30_tx_disable),
    _DEVICE_ATTR(eth_31_tx_disable),
    _DEVICE_ATTR(eth_32_tx_disable),
    _DEVICE_ATTR(eth_33_tx_disable),
    _DEVICE_ATTR(eth_34_tx_disable),
    _DEVICE_ATTR(eth_35_tx_disable),
    _DEVICE_ATTR(eth_36_tx_disable),
    _DEVICE_ATTR(eth_37_tx_disable),
    _DEVICE_ATTR(eth_38_tx_disable),
    _DEVICE_ATTR(eth_39_tx_disable),
    _DEVICE_ATTR(eth_16_rate_sel),
    _DEVICE_ATTR(eth_17_rate_sel),
    _DEVICE_ATTR(eth_18_rate_sel),
    _DEVICE_ATTR(eth_19_rate_sel),
    _DEVICE_ATTR(eth_20_rate_sel),
    _DEVICE_ATTR(eth_21_rate_sel),
    _DEVICE_ATTR(eth_22_rate_sel),
    _DEVICE_ATTR(eth_23_rate_sel),
    _DEVICE_ATTR(eth_24_rate_sel),
    _DEVICE_ATTR(eth_25_rate_sel),
    _DEVICE_ATTR(eth_26_rate_sel),
    _DEVICE_ATTR(eth_27_rate_sel),
    _DEVICE_ATTR(eth_28_rate_sel),
    _DEVICE_ATTR(eth_29_rate_sel),
    _DEVICE_ATTR(eth_30_rate_sel),
    _DEVICE_ATTR(eth_31_rate_sel),
    _DEVICE_ATTR(eth_32_rate_sel),
    _DEVICE_ATTR(eth_33_rate_sel),
    _DEVICE_ATTR(eth_34_rate_sel),
    _DEVICE_ATTR(eth_35_rate_sel),
    _DEVICE_ATTR(eth_36_rate_sel),
    _DEVICE_ATTR(eth_37_rate_sel),
    _DEVICE_ATTR(eth_38_rate_sel),
    _DEVICE_ATTR(eth_39_rate_sel),
    _DEVICE_ATTR(cpld4_pwr_status_0),
    _DEVICE_ATTR(cpld4_pwr_status_1),
    _DEVICE_ATTR(cpld4_pwr_status_2),
    _DEVICE_ATTR(cpld4_pwr_status_3),
    _DEVICE_ATTR(dbg_eth_16_23_present),
    _DEVICE_ATTR(dbg_eth_24_31_present),
    _DEVICE_ATTR(dbg_eth_32_39_present),
    _DEVICE_ATTR(dbg_eth_16_23_rx_los),
    _DEVICE_ATTR(dbg_eth_24_31_rx_los),
    _DEVICE_ATTR(dbg_eth_32_39_rx_los),
    _DEVICE_ATTR(dbg_eth_16_23_tx_fault),
    _DEVICE_ATTR(dbg_eth_24_31_tx_fault),
    _DEVICE_ATTR(dbg_eth_32_39_tx_fault),
    _DEVICE_ATTR(bsp_debug),
    NULL,
};

static struct attribute *cpld4_beta_attributes[] = {
    _DEVICE_ATTR(cpld_i2c_control),
    _DEVICE_ATTR(cpld_i2c_relay),
    _DEVICE_ATTR(eth_16_i2c_stuck),
    _DEVICE_ATTR(eth_17_i2c_stuck),
    _DEVICE_ATTR(eth_18_i2c_stuck),
    _DEVICE_ATTR(eth_19_i2c_stuck),
    _DEVICE_ATTR(eth_20_i2c_stuck),
    _DEVICE_ATTR(eth_21_i2c_stuck),
    _DEVICE_ATTR(eth_22_i2c_stuck),
    _DEVICE_ATTR(eth_23_i2c_stuck),
    _DEVICE_ATTR(eth_24_i2c_stuck),
    _DEVICE_ATTR(eth_25_i2c_stuck),
    _DEVICE_ATTR(eth_26_i2c_stuck),
    _DEVICE_ATTR(eth_27_i2c_stuck),
    _DEVICE_ATTR(eth_28_i2c_stuck),
    _DEVICE_ATTR(eth_29_i2c_stuck),
    _DEVICE_ATTR(eth_30_i2c_stuck),
    _DEVICE_ATTR(eth_31_i2c_stuck),
    _DEVICE_ATTR(eth_32_i2c_stuck),
    _DEVICE_ATTR(eth_33_i2c_stuck),
    _DEVICE_ATTR(eth_34_i2c_stuck),
    _DEVICE_ATTR(eth_35_i2c_stuck),
    _DEVICE_ATTR(eth_36_i2c_stuck),
    _DEVICE_ATTR(eth_37_i2c_stuck),
    _DEVICE_ATTR(eth_38_i2c_stuck),
    _DEVICE_ATTR(eth_39_i2c_stuck),
    _DEVICE_ATTR(cpld4_to_cpld1_intr),
    _DEVICE_ATTR(cpld4_i2c_stuck),
    _DEVICE_ATTR(eth_16_i2c_stuck_mask),
    _DEVICE_ATTR(eth_17_i2c_stuck_mask),
    _DEVICE_ATTR(eth_18_i2c_stuck_mask),
    _DEVICE_ATTR(eth_19_i2c_stuck_mask),
    _DEVICE_ATTR(eth_20_i2c_stuck_mask),
    _DEVICE_ATTR(eth_21_i2c_stuck_mask),
    _DEVICE_ATTR(eth_22_i2c_stuck_mask),
    _DEVICE_ATTR(eth_23_i2c_stuck_mask),
    _DEVICE_ATTR(eth_24_i2c_stuck_mask),
    _DEVICE_ATTR(eth_25_i2c_stuck_mask),
    _DEVICE_ATTR(eth_26_i2c_stuck_mask),
    _DEVICE_ATTR(eth_27_i2c_stuck_mask),
    _DEVICE_ATTR(eth_28_i2c_stuck_mask),
    _DEVICE_ATTR(eth_29_i2c_stuck_mask),
    _DEVICE_ATTR(eth_30_i2c_stuck_mask),
    _DEVICE_ATTR(eth_31_i2c_stuck_mask),
    _DEVICE_ATTR(eth_32_i2c_stuck_mask),
    _DEVICE_ATTR(eth_33_i2c_stuck_mask),
    _DEVICE_ATTR(eth_34_i2c_stuck_mask),
    _DEVICE_ATTR(eth_35_i2c_stuck_mask),
    _DEVICE_ATTR(eth_36_i2c_stuck_mask),
    _DEVICE_ATTR(eth_37_i2c_stuck_mask),
    _DEVICE_ATTR(eth_38_i2c_stuck_mask),
    _DEVICE_ATTR(eth_39_i2c_stuck_mask),
    _DEVICE_ATTR(cpld4_to_cpld1_intr_mask),
    _DEVICE_ATTR(cpld4_i2c_stuck_mask),
    _DEVICE_ATTR(eth_16_i2c_stuck_event),
    _DEVICE_ATTR(eth_17_i2c_stuck_event),
    _DEVICE_ATTR(eth_18_i2c_stuck_event),
    _DEVICE_ATTR(eth_19_i2c_stuck_event),
    _DEVICE_ATTR(eth_20_i2c_stuck_event),
    _DEVICE_ATTR(eth_21_i2c_stuck_event),
    _DEVICE_ATTR(eth_22_i2c_stuck_event),
    _DEVICE_ATTR(eth_23_i2c_stuck_event),
    _DEVICE_ATTR(eth_24_i2c_stuck_event),
    _DEVICE_ATTR(eth_25_i2c_stuck_event),
    _DEVICE_ATTR(eth_26_i2c_stuck_event),
    _DEVICE_ATTR(eth_27_i2c_stuck_event),
    _DEVICE_ATTR(eth_28_i2c_stuck_event),
    _DEVICE_ATTR(eth_29_i2c_stuck_event),
    _DEVICE_ATTR(eth_30_i2c_stuck_event),
    _DEVICE_ATTR(eth_31_i2c_stuck_event),
    _DEVICE_ATTR(eth_32_i2c_stuck_event),
    _DEVICE_ATTR(eth_33_i2c_stuck_event),
    _DEVICE_ATTR(eth_34_i2c_stuck_event),
    _DEVICE_ATTR(eth_35_i2c_stuck_event),
    _DEVICE_ATTR(eth_36_i2c_stuck_event),
    _DEVICE_ATTR(eth_37_i2c_stuck_event),
    _DEVICE_ATTR(eth_38_i2c_stuck_event),
    _DEVICE_ATTR(eth_39_i2c_stuck_event),
    _DEVICE_ATTR(cpld4_to_cpld1_intr_event),
    _DEVICE_ATTR(cpld4_i2c_stuck_event),
    NULL,
};

/* cpld1 attributes group */
static const struct attribute_group cpld1_group = {
    .attrs = cpld1_common_attributes,
};

/* cpld2 attributes group */
static const struct attribute_group cpld2_group = {
    .attrs = cpld2_common_attributes,
};

/* cpld3 attributes group */
static const struct attribute_group cpld3_group = {
    .attrs = cpld3_common_attributes,
};

/* cpld4 attributes group */
static const struct attribute_group cpld4_group = {
    .attrs = cpld4_common_attributes,
};

static const struct attribute_group cpld1_beta_group = {
    .attrs = cpld1_beta_attributes,
};

static const struct attribute_group cpld2_beta_group = {
    .attrs = cpld2_beta_attributes,
};

static const struct attribute_group cpld3_beta_group = {
    .attrs = cpld3_beta_attributes,
};

static const struct attribute_group cpld4_beta_group = {
    .attrs = cpld4_beta_attributes,
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
        return sprintf(buf, "0x%02x\n", data);
    } else if(data_type == DATA_DEC) {
        return sprintf(buf, "%u\n", data);
    } else if(data_type == DATA_0_1) {
        return sprintf(buf, "%u\n", (data & 0x1));
    } else if(data_type == DATA_0_1_INV) {
        return sprintf(buf, "%u\n", (data & 0x1)? 0:1);
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

static int _store_value_check(int index, u8 reg_val, char **range) {
    int ret = 0;
    if(range == NULL) {
        return -2;
    }

    switch (index) {
        // case MGMT_0_LED_SPEED:
        // case MGMT_1_LED_SPEED:
        //     if(reg_val != 0 && reg_val != 1) {
        //         ret = -1;
        //     }
        //     *range = "0 or 1";
        //     break;
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
    ssize_t ret = 0;

    switch (attr->index) {
        case BSP_DEBUG:
            str = bsp_debug;
            str_len = sizeof(bsp_debug);
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

    return cpld_reg_read(dev, buf, attr_reg[attr->index]);
}

/* set cpld register value */
static ssize_t cpld_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg_val = 0;

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

    return cpld_reg_write(dev, buf, count, attr_reg[attr->index]);
}

/* Event reading with soft latch and per-bit clear */
int _cpld_event_read(struct device *dev, u8 reg, u8 mask)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct cpld_data *data = i2c_mux_priv(muxc);
    int reg_val, xor_reg_val;
    int ret_val;

    mutex_lock(&data->access_lock);

    // read cpld event register (clear on read)
    reg_val = i2c_smbus_read_byte_data(client, reg);

    if (reg_val >= 0) {

        // inverse reg_val value because hardware is active low but soft_latch is active high
        xor_reg_val = reg_val ^ 0xFF;

        /*
         * Soft Latch Logic:
         * 1. Accumulate events value into soft latch (OR operation).
         * This saves events that might happen between reads or for other ports.
         * 2. Extract the value to return from the latch.
         * 3. Clear ONLY the bits requested by the current 'mask' (Per-bit clear).
         * Other bits (other ports) remain latched until their specific node is read.
         */
        data->soft_latch[reg] |= (u8) xor_reg_val;

        // Use the latched value
        ret_val = data->soft_latch[reg];

        // Per-bit Clear: Clear only the bits we are reading right now
        data->soft_latch[reg] &= ~mask;

        BSP_LOG_R("cpld[%d] Event: reg=0x%02x, reg_val=0x%02x, xor_reg_val=0x%02x, latch_before=0x%02x, latch_after=0x%02x",
                  data->index, reg, reg_val, xor_reg_val, ret_val, data->soft_latch[reg]);

        reg_val = ret_val;
    }

    mutex_unlock(&data->access_lock);

    if (unlikely(reg_val < 0)) {
        return reg_val;
    } else {
        // Shift and mask to return the specific bit value (0 or 1)
        return _mask_shift(reg_val, mask);
    }
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
                    attr_reg_map_t attr_reg)
{
    int reg_val;
    u8 reg = 0;
    u8 mask = MASK_NONE;
    u8 data_type = DATA_UNK;
    bool is_event = REG_NOT_EVENT;

    reg = attr_reg.reg;
    mask = attr_reg.mask;
    data_type = attr_reg.data_type;
    is_event = attr_reg.is_event;

    if (is_event) {
        reg_val = _cpld_event_read(dev, reg, mask);
    } else {
        reg_val = _cpld_reg_read(dev, reg, mask);
    }

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


/* set cpld register value with protect */
static ssize_t _cpld_reg_write_with_protect(struct device *dev,
                            u8 reg,
                            u8 reg_val)
{
    int ret = 0;
    u8 reg_wp = WRITE_PROTECT_1_REG;
    u8 reg_wp_val = 0;
    u8 reg_wp_mask = 0x01;
    u8 current_wp = 0;
    struct i2c_client *i2c_client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(i2c_client);
    struct cpld_data *data = i2c_mux_priv(muxc);

    BSP_LOG_W("Writing protected cpld register, reg=0x%x, value=0x%x\n", reg, reg_val);

    // read the write protect reg
    I2C_READ_BYTE_DATA_NOLOCK(ret, i2c_client, reg_wp);
    if (unlikely(ret < 0))
    {
        dev_err(dev, "i2c_smbus_read_byte_data() error, reg=0x%x, return=%d\n", reg_wp, ret);
        goto exit;
    }

    current_wp = ret;

    if (!(current_wp & reg_wp_mask))
    {
        // enable reg write
        reg_wp_val = current_wp | reg_wp_mask;
        I2C_WRITE_BYTE_DATA_NOLOCK(ret, i2c_client, reg_wp, reg_wp_val);

        if (unlikely(ret < 0))
        {
            dev_err(dev, "i2c_smbus_write_byte_data() error, reg=0x%x, value=0x%x, return=%d\n", reg_wp, reg_wp_val, ret);
            goto exit;
        }

        // increase access count for write enable
        wp_access_count++;
    }

    // write target reg
    ret = i2c_smbus_write_byte_data(i2c_client, reg, reg_val);

    if (unlikely(ret < 0))
    {
        dev_err(dev, "i2c_smbus_write_byte_data() error, reg=0x%x, value=0x%x, return=%d\n", reg, reg_val, ret);
    }

    if (!(current_wp & reg_wp_mask))
    {
        // restore write protect reg
        reg_wp_val = current_wp;
        ret = i2c_smbus_write_byte_data(i2c_client, reg_wp, reg_wp_val);

        if (unlikely(ret < 0))
        {
            dev_err(dev, "i2c_smbus_write_byte_data() error, reg=0x%x, value=0x%x, return=%d\n", reg_wp, reg_wp_val, ret);
        }
    }

exit:
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
                    attr_reg_map_t attr_reg)
{
    u8 reg_val, shift;
    int reg_val_now;
    int ret = 0;
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct cpld_data *data = i2c_mux_priv(muxc);
    u8 reg = 0;
    u8 mask = 0;
    u8 data_type = 0;
    bool write_protect = false;

    reg = attr_reg.reg;
    mask = attr_reg.mask;
    data_type = attr_reg.data_type;
    write_protect = attr_reg.write_protect;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    if(data_type == DATA_0_1) {
        if (kstrtou8(buf, 0, &reg_val) < 0) {
            return -EINVAL;
        }
        reg_val = (reg_val & 0x1);
    } else if(data_type == DATA_0_1_INV) {
        if (kstrtou8(buf, 0, &reg_val) < 0) {
            return -EINVAL;
        }
        reg_val = (reg_val & 0x1) ? 0:1;
    } else {
        if (kstrtou8(buf, 0, &reg_val) < 0) {
            if(data_type == DATA_S_DEC) {
                if (kstrtos8(buf, 0, &reg_val) < 0) {
                    return -EINVAL;
                }
            } else {
                return -EINVAL;
            }
        }
    }

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

    if (write_protect)
    {
        ret = _cpld_reg_write_with_protect(dev, reg, reg_val);
    }
    else
    {
        ret = _cpld_reg_write_nolock(dev, reg, reg_val);
    }

    // unlock mutex after register access
    mutex_unlock(&data->access_lock);

    if (unlikely(ret < 0)) {
        dev_err(dev, "cpld_reg_write() error, return=%d\n", ret);
        return ret;
    }

    return count;
}

static int _get_led_node(int index, led_node_t *node)
{
    color_obj_t general_set[COLOR_VAL_MAX] = {
        [0] = {.status = LED_COLOR_DARK           , .val = 0b00000000},
        [1] = {.status = LED_COLOR_DARK           , .val = 0b00000001},
        [2] = {.status = LED_COLOR_DARK           , .val = 0b00000100},
        [3] = {.status = LED_COLOR_DARK           , .val = 0b00000101},
        [4] = {.status = LED_COLOR_GREEN          , .val = 0b00001001},
        [5] = {.status = LED_COLOR_GREEN_BLINK    , .val = 0b00001101},
        [6] = {.status = LED_COLOR_YELLOW         , .val = 0b00001000},
        [7] = {.status = LED_COLOR_YELLOW_BLINK   , .val = 0b00001100},
        [8] = {.val = -1}
    };

    color_obj_t id_led_set[COLOR_VAL_MAX] = {
        [0] = {.status = LED_COLOR_DARK           , .val = 0b00000000},
        [1] = {.status = LED_COLOR_BLUE           , .val = 0b00000100},
        [2] = {.status = LED_COLOR_BLUE_BLINK     , .val = 0b00000110},
        [3] = {.val = -1}
    };

    switch (index){
        case CPLD_SYSTEM_LED_SYS:
        case CPLD_SYSTEM_LED_FAN:
        case CPLD_SYSTEM_LED_PWR:
        case CPLD_SYSTEM_LED_GNSS:
        case CPLD_SYSTEM_LED_SYNC:
            node->reg = attr_reg[index].reg;
            node->mask= attr_reg[index].mask;
            node->color_mask = 0b00001101;
            node->data_type = attr_reg[index].data_type;
            memcpy(node->color_obj, general_set, sizeof(general_set));
            break;
        case CPLD_SYSTEM_LED_ID:
            node->reg = attr_reg[index].reg;
            node->mask= attr_reg[index].mask;
            node->color_mask = 0b00001110;
            node->data_type = attr_reg[index].data_type;
            memcpy(node->color_obj, id_led_set, sizeof(id_led_set));
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

ssize_t led_show(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    led_node_t node = {0};
    int status = LED_COLOR_DARK;
    int led_val = 0;
    int found = 0;
    int i = 0;

    if(_get_led_node(attr->index,&node) != 0) {
        return -EINVAL;
    }

    led_val=_cpld_reg_read(dev, node.reg, node.mask);

    if(led_val < 0) {
        return led_val;
    }

    for(i= 0; i<COLOR_VAL_MAX; i++) {
        if(node.color_obj[i].val == -1)
            break;

        if((led_val & node.color_mask) == node.color_obj[i].val) {
            status = node.color_obj[i].status;
            found=1;
            break;
        }
    }

    if(found == 0) {
        pr_err("Led value not in definition!!\n");
        return -EINVAL;
    }

    return _parse_data(buf, status, node.data_type);
}

ssize_t led_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    led_node_t node = {0};
    int status = LED_COLOR_DARK;
    short int val;
    int found = 0;
    int i = 0;
    char led_str[5] = {'\0'};
    attr_reg_map_t attr_reg = {0};

    if(_get_led_node(attr->index,&node) != 0) {
        return -EINVAL;
    }

    if (kstrtoint(buf, 0, &status) < 0)
        return -EINVAL;

    for(i= 0; i<COLOR_VAL_MAX; i++) {
        if(node.color_obj[i].val == -1)
            break;

        if(status == node.color_obj[i].status) {
            found=1;
            val = node.color_obj[i].val;
            break;
        }
    }

    if(found == 0) {
        pr_err("Led value not in definition!!\n");
        return -EINVAL;
    }
    snprintf(led_str, sizeof(led_str), "0x%x", (val & 0xff));

    attr_reg.reg = node.reg;
    attr_reg.mask = node.mask;
    attr_reg.data_type = node.data_type;

    return cpld_reg_write(dev, led_str, count, attr_reg);
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

/* cpld driver probe */
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
    u16 hw_build_rev_reg = (REG_BASE_MB + BRD_HW_BUILD_REV_REG);
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

    if (system_hw_id == -1) {
        system_hw_id = inb(hw_build_rev_reg) & hw_build_rev_mask;
    }
    data->hw_id = system_hw_id;

    if (data->hw_id > 1 && mux_en) {
        status = mux_init(dev);
        if (status < 0) {
            dev_warn(dev, "Mux init failed\n");
            goto exit;
        }
    }

    switch (data->index) {
    case cpld1:
        status = sysfs_create_group(&client->dev.kobj, &cpld1_group);
        if (status) break;

        if (data->hw_id > 1) {
            status = sysfs_create_group(&client->dev.kobj, &cpld1_beta_group);
            if (status) sysfs_remove_group(&client->dev.kobj, &cpld1_group);
        }
        break;

    case cpld2:
        status = sysfs_create_group(&client->dev.kobj, &cpld2_group);
        if (status) break;

        if (data->hw_id > 1) {
            status = sysfs_create_group(&client->dev.kobj, &cpld2_beta_group);
            if (status) sysfs_remove_group(&client->dev.kobj, &cpld2_group);
        }
        break;

    case cpld3:
        status = sysfs_create_group(&client->dev.kobj, &cpld3_group);
        if (status) break;

        if (data->hw_id > 1) {
            status = sysfs_create_group(&client->dev.kobj, &cpld3_beta_group);
            if (status) sysfs_remove_group(&client->dev.kobj, &cpld3_group);
        }
        break;

    case cpld4:
        status = sysfs_create_group(&client->dev.kobj, &cpld4_group);
        if (status) break;

        if (data->hw_id > 1) {
            status = sysfs_create_group(&client->dev.kobj, &cpld4_beta_group);
            if (status) sysfs_remove_group(&client->dev.kobj, &cpld4_group);
        }
        break;
    default:
        status = -EINVAL;
    }

    if (status)
        goto exit;

    if (data->hw_id > 1 && mux_en) {
        if (data->chip->nchans > 0) {
            status = sysfs_add_file_to_group(&client->dev.kobj,
                        &sensor_dev_attr_idle_state.dev_attr.attr, NULL);
            if (status) goto sysfs_err;
        }
    }

    dev_info(&client->dev, "chip found\n");

    /* add probe chip to client list */
    cpld_add_client(client);

    return 0;

sysfs_err:
    switch (data->index) {
    case cpld1:
        sysfs_remove_group(&client->dev.kobj, &cpld1_group);
        if (data->hw_id > 1) sysfs_remove_group(&client->dev.kobj, &cpld1_beta_group);
        break;
    case cpld2:
        sysfs_remove_group(&client->dev.kobj, &cpld2_group);
        if (data->hw_id > 1) sysfs_remove_group(&client->dev.kobj, &cpld2_beta_group);
        break;
    case cpld3:
        sysfs_remove_group(&client->dev.kobj, &cpld3_group);
        if (data->hw_id > 1) sysfs_remove_group(&client->dev.kobj, &cpld3_beta_group);
        break;
    case cpld4:
        sysfs_remove_group(&client->dev.kobj, &cpld4_group);
        if (data->hw_id > 1) sysfs_remove_group(&client->dev.kobj, &cpld4_beta_group);
        break;
    }

    if (data->hw_id > 1 && mux_en) {
        if (data->chip->nchans > 0) {
            mux_cleanup(dev);
        }
    }

exit:
    return status;
}

/* cpld driver remove */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
static int cpld_remove(struct i2c_client *client)
#else
static void cpld_remove(struct i2c_client *client)
#endif
{
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct device *dev = &client->dev;
    struct cpld_data *data = i2c_mux_priv(muxc);

    if (data->hw_id > 1 && mux_en) {
        if (data->chip->nchans > 0) {
            sysfs_remove_file_from_group(&client->dev.kobj,
                &sensor_dev_attr_idle_state.dev_attr.attr, NULL);
        }
    }

    switch (data->index) {
    case cpld1:
        sysfs_remove_group(&client->dev.kobj, &cpld1_group);
        if (data->hw_id > 1) {
            sysfs_remove_group(&client->dev.kobj, &cpld1_beta_group);
        }
        break;
    case cpld2:
        sysfs_remove_group(&client->dev.kobj, &cpld2_group);
        if (data->hw_id > 1) {
            sysfs_remove_group(&client->dev.kobj, &cpld2_beta_group);
        }
        break;
    case cpld3:
        sysfs_remove_group(&client->dev.kobj, &cpld3_group);
        if (data->hw_id > 1) {
            sysfs_remove_group(&client->dev.kobj, &cpld3_beta_group);
        }
        break;
    case cpld4:
        sysfs_remove_group(&client->dev.kobj, &cpld4_group);
        if (data->hw_id > 1) {
            sysfs_remove_group(&client->dev.kobj, &cpld4_beta_group);
        }
        break;
    default:
        break;
    }

    if (data->hw_id > 1 && mux_en) {
        if (data->chip->nchans > 0) {
            mux_cleanup(dev);
        }
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
        .name = "x86_64_ufispace_s9620_40dg_cpld",
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
MODULE_AUTHOR("Zack Yen <zack.yen@ufispace.com>");
MODULE_DESCRIPTION("x86_64_ufispace_s9620_40dg_cpld driver");
MODULE_VERSION("1.0.1");
MODULE_LICENSE("GPL");

module_init(cpld_init);
module_exit(cpld_exit);
