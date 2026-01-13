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
#include "x86-64-ufispace-s9620-40dg-cpld.h"

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


static attr_reg_map_t attr_reg[] = {
    /******************************************************************************
    * CPLD Common                                                                    *
    ******************************************************************************/
    // @ CPLD_VERSION_REG (0x02)
    [CPLD_MINOR_VER]              = { CPLD_VERSION_REG,                MASK_0011_1111,  DATA_DEC,    REG_WP_DIS},
    [CPLD_MAJOR_VER]              = { CPLD_VERSION_REG,                MASK_1100_0000,  DATA_DEC,    REG_WP_DIS},

    // @ CPLD_ID_REG (0x03)
    [CPLD_ID]                     = { CPLD_ID_REG,                     MASK_0000_0111,  DATA_DEC,    REG_WP_DIS},

    // @ CPLD_BUILD_REG (0x04)
    [CPLD_BUILD_VER]              = { CPLD_BUILD_REG,                  MASK_ALL,        DATA_DEC,    REG_WP_DIS},

    [CPLD_VERSION_H]              = { NONE_REG,                        MASK_ALL,        DATA_DEC,    REG_WP_DIS},
    [EVENT_DETECT_CTRL]           = { EVENT_DETECT_CTRL_REG,           MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [CPLD_CHIP_TYPE]              = { CPLD_CHIP_TYPE_REG,              MASK_0000_0111,  DATA_DEC,    REG_WP_DIS},
    [MODULE_RESET]                = { MODULE_RESET_REG,                MASK_0000_0001,  DATA_DEC,    REG_WP_EN },
    [CPLD_WRITE_PROTECT_1]        = { WRITE_PROTECT_1_REG,             MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [CPLD_WRITE_PROTECT_2]        = { WRITE_PROTECT_2_REG,             MASK_ALL,        DATA_DEC,    REG_WP_DIS},


    /******************************************************************************
    * CPLD 1                                                                     *
    ******************************************************************************/

    // Board information
    [BRD_SKU_ID]                  = { BRD_SKU_ID_REG,                  MASK_ALL,        DATA_DEC,    REG_WP_DIS},

    // @ BRD_HW_BUILD_REV_REG (0x01)
    [BRD_HW_ID]                   = { BRD_HW_BUILD_REV_REG,            MASK_0000_0011,  DATA_DEC,    REG_WP_DIS},
    [BRD_DEPH_ID]                 = { BRD_HW_BUILD_REV_REG,            MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [BRD_BUILD_ID]                = { BRD_HW_BUILD_REV_REG,            MASK_0001_1000,  DATA_DEC,    REG_WP_DIS},
    [BRD_ID_TYPE]                 = { BRD_HW_BUILD_REV_REG,            MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},

    [CPLD_BOARD_EXT_ID]           = { CPLD_BOARD_EXT_ID_REG,           MASK_0000_0111,  DATA_DEC,    REG_WP_DIS},

    // CPLD information
    [GDDR6_ID]                    = { GDDR6_ID_REG,                    MASK_0000_0111,  DATA_DEC,    REG_WP_DIS},
    [GDDR6_ID_FUNC]               = { GDDR6_ID_FUNC_REG,               MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    // Interrupt Status
    [PHY_INTR]                    = { PHY_INTR_REG,                    MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [CLK_PTP_INTR]                = { CLK_PTP_INTR_REG,                MASK_ALL,        DATA_HEX,    REG_WP_DIS},

    // @ TOP_BRD_CPLD_FRU_INTR_REG (0x14)
    [TOP_BRD_CPLD_FRU_INTR]       = { TOP_BRD_CPLD_FRU_INTR_REG,       MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [PSU0_INTR]                   = { TOP_BRD_CPLD_FRU_INTR_REG,       MASK_0000_0001,  DATA_HEX,    REG_WP_DIS},
    [PSU1_INTR]                   = { TOP_BRD_CPLD_FRU_INTR_REG,       MASK_0000_0010,  DATA_HEX,    REG_WP_DIS},
    [CPLD2_INTR]                  = { TOP_BRD_CPLD_FRU_INTR_REG,       MASK_0000_0100,  DATA_HEX,    REG_WP_DIS},

    // @ MAIN_BRD_CPLD_INTR_REG (0x15)
    [MAIN_BRD_CPLD_INTR]          = { MAIN_BRD_CPLD_INTR_REG,          MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [CPLD3_INTR]                  = { MAIN_BRD_CPLD_INTR_REG,          MASK_0000_0001,  DATA_HEX,    REG_WP_DIS},
    [CPLD4_INTR]                  = { MAIN_BRD_CPLD_INTR_REG,          MASK_0000_0010,  DATA_HEX,    REG_WP_DIS},
    [MB_ETH_INTR]                 = { MAIN_BRD_CPLD_INTR_REG,          MASK_0000_0100,  DATA_HEX,    REG_WP_DIS},
    [FAN_INTR]                    = { MAIN_BRD_CPLD_INTR_REG,          MASK_0001_0000,  DATA_HEX,    REG_WP_DIS},

    [THERMAL_INTR]                = { THERMAL_INTR_REG,                MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [CPU_NMI_INTR]                = { CPU_NMI_INTR_REG,                MASK_0000_0001,  DATA_HEX,    REG_WP_DIS},
    [OUT_STATUS_INTR]             = { OUT_STATUS_INTR_REG,             MASK_ALL,        DATA_HEX,    REG_WP_DIS},

    // Interrupt Mask
    [CLK_PTP_MASK_INTR]           = { CLK_PTP_MASK_INTR_REG,           MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [PHY_MASK_INTR]               = { PHY_MASK_INTR_REG,               MASK_ALL,        DATA_HEX,    REG_WP_DIS},

    // @ TOP_BRD_CPLD_FRU_MASK_INTR_REG (0x24)
    [TOP_BRD_CPLD_FRU_MASK_INTR]  = { TOP_BRD_CPLD_FRU_MASK_INTR_REG,  MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [PSU0_MASK_INTR]              = { TOP_BRD_CPLD_FRU_MASK_INTR_REG,  MASK_0000_0001,  DATA_HEX,    REG_WP_DIS},
    [PSU1_MASK_INTR]              = { TOP_BRD_CPLD_FRU_MASK_INTR_REG,  MASK_0000_0010,  DATA_HEX,    REG_WP_DIS},
    [CPLD2_MASK_INTR]             = { TOP_BRD_CPLD_FRU_MASK_INTR_REG,  MASK_0000_0100,  DATA_HEX,    REG_WP_DIS},
    [CPLD2_IO_MASK_INTR]          = { TOP_BRD_CPLD_FRU_MASK_INTR_REG,  MASK_0000_1000,  DATA_HEX,    REG_WP_DIS},
    // @ MAIN_BRD_CPLD_MASK_INTR_REG (0x25)
    [MAIN_BRD_CPLD_MASK_INTR]     = { MAIN_BRD_CPLD_MASK_INTR_REG,     MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [CPLD3_MASK_INTR]             = { MAIN_BRD_CPLD_MASK_INTR_REG,     MASK_0000_0001,  DATA_HEX,    REG_WP_DIS},
    [CPLD4_MASK_INTR]             = { MAIN_BRD_CPLD_MASK_INTR_REG,     MASK_0000_0010,  DATA_HEX,    REG_WP_DIS},
    [MB_ETH_MASK_INTR]            = { MAIN_BRD_CPLD_MASK_INTR_REG,     MASK_0000_0100,  DATA_HEX,    REG_WP_DIS},
    [MB_PTP_MASK_INTR]            = { MAIN_BRD_CPLD_MASK_INTR_REG,     MASK_0000_1000,  DATA_HEX,    REG_WP_DIS},
    [FAN_MASK_INTR]               = { MAIN_BRD_CPLD_MASK_INTR_REG,     MASK_0001_0000,  DATA_HEX,    REG_WP_DIS},

    [THERMAL_MASK_INTR]           = { THERMAL_MASK_INTR_REG,           MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [CPU_NMI_MASK_INTR]           = { CPU_NMI_MASK_INTR_REG,           MASK_0000_0001,  DATA_HEX,    REG_WP_DIS},
    [OUT_STATUS_MASK_INTR]        = { OUT_STATUS_MASK_INTR_REG,        MASK_ALL,        DATA_HEX,    REG_WP_DIS},

    // Interrupt Event
    [CLK_PTP_EVT_INTR]            = { CLK_PTP_EVT_INTR_REG,            MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [PHY_EVT_INTR]                = { PHY_EVT_INTR_REG,                MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [TOP_BRD_CPLD_FRU_EVT_INTR]   = { TOP_BRD_CPLD_FRU_EVT_INTR_REG,   MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [MAIN_BRD_CPLD_EVT_INTR]      = { MAIN_BRD_CPLD_EVT_INTR_REG,      MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [THERMAL_EVT_INTR]            = { THERMAL_EVT_INTR_REG,            MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [CPU_NMI_EVT_INTR]            = { CPU_NMI_EVT_INTR_REG,            MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [OUT_STATUS_EVT_INTR]         = { OUT_STATUS_EVT_INTR_REG,         MASK_ALL,        DATA_HEX,    REG_WP_DIS},

    // @ BIOS_FLASH_RESET_CTRL_REG (0x41)
    [BTN_FP_RESET]                = { BIOS_FLASH_RESET_CTRL_REG,       MASK_0001_0000,  DATA_DEC,    REG_WP_EN },
    [SPI_BIOS_RESET]              = { BIOS_FLASH_RESET_CTRL_REG,       MASK_0000_0001,  DATA_DEC,    REG_WP_EN },

    // @ BMC_PHY_RESET_CTRL_REG (0x43)
    [RGB_1_RESET]                 = { BMC_PHY_RESET_CTRL_REG,          MASK_0010_0000,  DATA_DEC,    REG_WP_EN },
    [RGB_0_RESET]                 = { BMC_PHY_RESET_CTRL_REG,          MASK_0001_0000,  DATA_DEC,    REG_WP_EN },
    [BMC_LPC_RESET]               = { BMC_PHY_RESET_CTRL_REG,          MASK_0000_1000,  DATA_DEC,    REG_WP_EN },
    [BMC_PCIE_RESET]              = { BMC_PHY_RESET_CTRL_REG,          MASK_0000_0100,  DATA_DEC,    REG_WP_EN },
    [CPLD_TO_BMC_SYS_RESET]       = { BMC_PHY_RESET_CTRL_REG,          MASK_0000_0010,  DATA_DEC,    REG_WP_EN },
    [CPLD_TO_CPU_RESET]           = { BMC_PHY_RESET_CTRL_REG,          MASK_0000_0001,  DATA_DEC,    REG_WP_EN },

    // @ USB_RESET_CTRL_REG (0x44)
    [USB_PWR_EN]                  = { USB_RESET_CTRL_REG,              MASK_1000_0000,  DATA_DEC,    REG_WP_EN },
    [USB_SIE_RESET]               = { USB_RESET_CTRL_REG,              MASK_0000_0001,  DATA_DEC,    REG_WP_EN },

    // @ TOP_I2C_MUX_RESET_REG (0x46)
    [I2C_MUX_SYS_RESET]           = {TOP_I2C_MUX_RESET_REG,            MASK_0000_0001,  DATA_DEC,    REG_WP_EN },
    [I2C_MUX_SMBUS_RESET]         = {TOP_I2C_MUX_RESET_REG,            MASK_0000_0010,  DATA_DEC,    REG_WP_EN },
    [I2C_MUX_QSFP28_RESET]        = {TOP_I2C_MUX_RESET_REG,            MASK_0000_0100,  DATA_DEC,    REG_WP_EN },
    [I2C_MUX_SFP_RESET]           = {TOP_I2C_MUX_RESET_REG,            MASK_0000_1000,  DATA_DEC,    REG_WP_EN },
    [MB_I2C_RESET]                = {TOP_I2C_MUX_RESET_REG,            MASK_0001_0000,  DATA_DEC,    REG_WP_EN },

    // Misc Status Control
    // @ DAUGHTER_BRD_ABS_REG (0x50)
    [BMC_ABS]                     = { DAUGHTER_BRD_ABS_REG,            MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [SATA_SSD1_ABS]               = { DAUGHTER_BRD_ABS_REG,            MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [SATA_SSD2_ABS]               = { DAUGHTER_BRD_ABS_REG,            MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},

    // @ PSU_STATUS_REG (0x51)
    [PSU0_ABS]                    = { PSU_STATUS_REG,                  MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [PSU1_ABS]                    = { PSU_STATUS_REG,                  MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [PSU0_VIN_PG]                 = { PSU_STATUS_REG,                  MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [PSU1_VIN_PG]                 = { PSU_STATUS_REG,                  MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [PSU0_VOUT_PG]                = { PSU_STATUS_REG,                  MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [PSU1_VOUT_PG]                = { PSU_STATUS_REG,                  MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},
    [PSU_STATUS]                  = { PSU_STATUS_REG,                  MASK_ALL,        DATA_DEC,    REG_WP_DIS},

    // @ SYSTEM_PWR_STATUS_REG (0x52)
    [CPU_BOOT_DONE]               = { SYSTEM_PWR_STATUS_REG,           MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [CPU_PG]                      = { SYSTEM_PWR_STATUS_REG,           MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},

    // WD_STATUS
    [WDT_CPU_TO_CPLD]             = { NONE_REG,                        MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [BMC_WDT_1_RESET]             = { NONE_REG,                        MASK_ALL,        DATA_HEX,    REG_WP_EN },
    [BMC_WDT_2_RESET]             = { NONE_REG,                        MASK_ALL,        DATA_HEX,    REG_WP_EN },

    // @ MUX_CTRL_REG (0x5C)
    [SMBUS_PECI_DIS]              = { MUX_CTRL_REG,                    MASK_0000_0001,   DATA_DEC,   REG_WP_DIS},
    [I2C_PSU0_MUX_SEL]            = { MUX_CTRL_REG,                    MASK_0000_0010,   DATA_DEC,   REG_WP_DIS},
    [I2C_PSU1_MUX_SEL]            = { MUX_CTRL_REG,                    MASK_0000_0100,   DATA_DEC,   REG_WP_DIS},
    [I2C_CPLD_MUX_SEL]            = { MUX_CTRL_REG,                    MASK_0001_0000,   DATA_DEC,   REG_WP_DIS},
    [BMC_USB_MUX_SEL]             = { MUX_CTRL_REG,                    MASK_0010_0000,   DATA_DEC,   REG_WP_DIS},
    [UART_CPU_BMC_MUX_SEL]        = { MUX_CTRL_REG,                    MASK_0100_0000,   DATA_DEC,   REG_WP_DIS},
    [UART_MUX_SEL]                = { MUX_CTRL_REG,                    MASK_1000_0000,   DATA_DEC,   REG_WP_DIS},

    [EXT_CTRL]                    = { EXT_CTRL_REG,                    MASK_0000_0011,   DATA_DEC,   REG_WP_DIS},

    // LED Control
    // @ SYSTEM_LED_CTRL_1_REG (0x80)
    [CPLD_SYSTEM_LED_SYS]         = { SYSTEM_LED_CTRL_1_REG,           MASK_0000_1111,   DATA_DEC,   REG_WP_DIS},
    [SYSTEM_LED_STATUS]           = { SYSTEM_LED_CTRL_1_REG,           MASK_0000_0001,   DATA_DEC,   REG_WP_DIS},
    [SYSTEM_LED_SPEED]            = { SYSTEM_LED_CTRL_1_REG,           MASK_0000_0010,   DATA_DEC,   REG_WP_DIS},
    [SYSTEM_LED_BLINK]            = { SYSTEM_LED_CTRL_1_REG,           MASK_0000_0100,   DATA_DEC,   REG_WP_DIS},
    [SYSTEM_LED_ONOFF]            = { SYSTEM_LED_CTRL_1_REG,           MASK_0000_1000,   DATA_DEC,   REG_WP_DIS},
    [CPLD_SYSTEM_LED_FAN]         = { SYSTEM_LED_CTRL_1_REG,           MASK_1111_0000,   DATA_DEC,   REG_WP_DIS},
    [FAN_LED_STATUS]              = { SYSTEM_LED_CTRL_1_REG,           MASK_0001_0000,   DATA_DEC,   REG_WP_DIS},
    [FAN_LED_SPEED]               = { SYSTEM_LED_CTRL_1_REG,           MASK_0010_0000,   DATA_DEC,   REG_WP_DIS},
    [FAN_LED_BLINK]               = { SYSTEM_LED_CTRL_1_REG,           MASK_0100_0000,   DATA_DEC,   REG_WP_DIS},
    [FAN_LED_ONOFF]               = { SYSTEM_LED_CTRL_1_REG,           MASK_1000_0000,   DATA_DEC,   REG_WP_DIS},

    // @ SYSTEM_LED_CTRL_2_REG (0x81)
    [CPLD_SYSTEM_LED_PWR]         = { SYSTEM_LED_CTRL_2_REG,           MASK_0000_1111,   DATA_DEC,   REG_WP_DIS},
    [PWR_LED_STATUS]              = { SYSTEM_LED_CTRL_2_REG,           MASK_0000_0001,   DATA_DEC,   REG_WP_DIS},
    [PWR_LED_SPEED]               = { SYSTEM_LED_CTRL_2_REG,           MASK_0000_0010,   DATA_DEC,   REG_WP_DIS},
    [PWR_LED_BLINK]               = { SYSTEM_LED_CTRL_2_REG,           MASK_0000_0100,   DATA_DEC,   REG_WP_DIS},
    [PWR_LED_ONOFF]               = { SYSTEM_LED_CTRL_2_REG,           MASK_0000_1000,   DATA_DEC,   REG_WP_DIS},
    [CPLD_SYSTEM_LED_GNSS]        = { SYSTEM_LED_CTRL_2_REG,           MASK_1111_0000,   DATA_DEC,   REG_WP_DIS},
    [GNSS_LED_STATUS]             = { SYSTEM_LED_CTRL_2_REG,           MASK_0001_0000,   DATA_DEC,   REG_WP_DIS},
    [GNSS_LED_SPEED]              = { SYSTEM_LED_CTRL_2_REG,           MASK_0010_0000,   DATA_DEC,   REG_WP_DIS},
    [GNSS_LED_BLINK]              = { SYSTEM_LED_CTRL_2_REG,           MASK_0100_0000,   DATA_DEC,   REG_WP_DIS},
    [GNSS_LED_ONOFF]              = { SYSTEM_LED_CTRL_2_REG,           MASK_1000_0000,   DATA_DEC,   REG_WP_DIS},

    // @ SYSTEM_LED_CTRL_3_REG (0x82)
    [CPLD_SYSTEM_LED_SYNC]        = { SYSTEM_LED_CTRL_3_REG,           MASK_0000_1111,   DATA_DEC,   REG_WP_DIS},
    [SYNC_LED_STATUS]             = { SYSTEM_LED_CTRL_3_REG,           MASK_0000_0001,   DATA_DEC,   REG_WP_DIS},
    [SYNC_LED_SPEED]              = { SYSTEM_LED_CTRL_3_REG,           MASK_0000_0010,   DATA_DEC,   REG_WP_DIS},
    [SYNC_LED_BLINK]              = { SYSTEM_LED_CTRL_3_REG,           MASK_0000_0100,   DATA_DEC,   REG_WP_DIS},
    [SYNC_LED_ONOFF]              = { SYSTEM_LED_CTRL_3_REG,           MASK_0000_1000,   DATA_DEC,   REG_WP_DIS},

    [LED_CLEAR]                   = { LED_CLEAR_REG,                   MASK_0011_1001,   DATA_DEC,   REG_WP_DIS},

    // Internal Control
    // @ QSFP28_0_5_PWR_EN,
    [QSFP28_P0_PWR_EN]            = { QSFP28_0_5_PWR_EN_REG,           MASK_0000_0001,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P1_PWR_EN]            = { QSFP28_0_5_PWR_EN_REG,           MASK_0000_0010,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P2_PWR_EN]            = { QSFP28_0_5_PWR_EN_REG,           MASK_0000_0100,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P3_PWR_EN]            = { QSFP28_0_5_PWR_EN_REG,           MASK_0000_1000,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P4_PWR_EN]            = { QSFP28_0_5_PWR_EN_REG,           MASK_0001_0000,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P5_PWR_EN]            = { QSFP28_0_5_PWR_EN_REG,           MASK_0010_0000,  DATA_DEC,    REG_WP_EN },

    // @ QSFP28_6_11_PWR_EN,
    [QSFP28_P6_PWR_EN]            = { QSFP28_6_11_PWR_EN_REG,          MASK_0000_0001,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P7_PWR_EN]            = { QSFP28_6_11_PWR_EN_REG,          MASK_0000_0010,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P8_PWR_EN]            = { QSFP28_6_11_PWR_EN_REG,          MASK_0000_0100,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P9_PWR_EN]            = { QSFP28_6_11_PWR_EN_REG,          MASK_0000_1000,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P10_PWR_EN]           = { QSFP28_6_11_PWR_EN_REG,          MASK_0001_0000,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P11_PWR_EN]           = { QSFP28_6_11_PWR_EN_REG,          MASK_0010_0000,  DATA_DEC,    REG_WP_EN },

    // @ QSFPDD_12_15_PWR_EN,
    [QSFPDD_P12_PWR_EN]           = { QSFPDD_12_15_PWR_EN_REG,         MASK_0000_0001,  DATA_DEC,    REG_WP_EN },
    [QSFPDD_P13_PWR_EN]           = { QSFPDD_12_15_PWR_EN_REG,         MASK_0000_0010,  DATA_DEC,    REG_WP_EN },
    [QSFPDD_P14_PWR_EN]           = { QSFPDD_12_15_PWR_EN_REG,         MASK_0000_0100,  DATA_DEC,    REG_WP_EN },
    [QSFPDD_P15_PWR_EN]           = { QSFPDD_12_15_PWR_EN_REG,         MASK_0000_1000,  DATA_DEC,    REG_WP_EN },

    // @ SFP56_16_23_PWR_EN_0_REG (CPLD1 0xB3 -> CPLD4)
    [SFP56_P16_PWR_EN]            = { SFP56_16_23_PWR_EN_0_REG,        MASK_0000_0001,   DATA_DEC,   REG_WP_EN },
    [SFP56_P17_PWR_EN]            = { SFP56_16_23_PWR_EN_0_REG,        MASK_0000_0010,   DATA_DEC,   REG_WP_EN },
    [SFP56_P18_PWR_EN]            = { SFP56_16_23_PWR_EN_0_REG,        MASK_0000_0100,   DATA_DEC,   REG_WP_EN },
    [SFP56_P19_PWR_EN]            = { SFP56_16_23_PWR_EN_0_REG,        MASK_0000_1000,   DATA_DEC,   REG_WP_EN },
    [SFP56_P20_PWR_EN]            = { SFP56_16_23_PWR_EN_0_REG,        MASK_0001_0000,   DATA_DEC,   REG_WP_EN },
    [SFP56_P21_PWR_EN]            = { SFP56_16_23_PWR_EN_0_REG,        MASK_0010_0000,   DATA_DEC,   REG_WP_EN },
    [SFP56_P22_PWR_EN]            = { SFP56_16_23_PWR_EN_0_REG,        MASK_0100_0000,   DATA_DEC,   REG_WP_EN },
    [SFP56_P23_PWR_EN]            = { SFP56_16_23_PWR_EN_0_REG,        MASK_1000_0000,   DATA_DEC,   REG_WP_EN },

    // @ SFP56_24_31_PWR_EN_1_REG (CPLD1 0xB4 -> CPLD4)
    [SFP56_P24_PWR_EN]            = { SFP56_24_31_PWR_EN_1_REG,        MASK_0000_0001,   DATA_DEC,   REG_WP_EN },
    [SFP56_P25_PWR_EN]            = { SFP56_24_31_PWR_EN_1_REG,        MASK_0000_0010,   DATA_DEC,   REG_WP_EN },
    [SFP56_P26_PWR_EN]            = { SFP56_24_31_PWR_EN_1_REG,        MASK_0000_0100,   DATA_DEC,   REG_WP_EN },
    [SFP56_P27_PWR_EN]            = { SFP56_24_31_PWR_EN_1_REG,        MASK_0000_1000,   DATA_DEC,   REG_WP_EN },
    [SFP56_P28_PWR_EN]            = { SFP56_24_31_PWR_EN_1_REG,        MASK_0001_0000,   DATA_DEC,   REG_WP_EN },
    [SFP56_P29_PWR_EN]            = { SFP56_24_31_PWR_EN_1_REG,        MASK_0010_0000,   DATA_DEC,   REG_WP_EN },
    [SFP56_P30_PWR_EN]            = { SFP56_24_31_PWR_EN_1_REG,        MASK_0100_0000,   DATA_DEC,   REG_WP_EN },
    [SFP56_P31_PWR_EN]            = { SFP56_24_31_PWR_EN_1_REG,        MASK_1000_0000,   DATA_DEC,   REG_WP_EN },

    // @ SFP56_32_39_PWR_EN_2_REG (CPLD1 0xB5 -> CPLD4)
    [SFP56_P32_PWR_EN]            = { SFP56_32_39_PWR_EN_2_REG,        MASK_0000_0001,   DATA_DEC,   REG_WP_EN },
    [SFP56_P33_PWR_EN]            = { SFP56_32_39_PWR_EN_2_REG,        MASK_0000_0010,   DATA_DEC,   REG_WP_EN },
    [SFP56_P34_PWR_EN]            = { SFP56_32_39_PWR_EN_2_REG,        MASK_0000_0100,   DATA_DEC,   REG_WP_EN },
    [SFP56_P35_PWR_EN]            = { SFP56_32_39_PWR_EN_2_REG,        MASK_0000_1000,   DATA_DEC,   REG_WP_EN },
    [SFP56_P36_PWR_EN]            = { SFP56_32_39_PWR_EN_2_REG,        MASK_0001_0000,   DATA_DEC,   REG_WP_EN },
    [SFP56_P37_PWR_EN]            = { SFP56_32_39_PWR_EN_2_REG,        MASK_0010_0000,   DATA_DEC,   REG_WP_EN },
    [SFP56_P38_PWR_EN]            = { SFP56_32_39_PWR_EN_2_REG,        MASK_0100_0000,   DATA_DEC,   REG_WP_EN },
    [SFP56_P39_PWR_EN]            = { SFP56_32_39_PWR_EN_2_REG,        MASK_1000_0000,   DATA_DEC,   REG_WP_EN },


    // Timing Control
    [CLK_PTP_RESET]               = { CLK_PTP_RESET_REG,               MASK_ALL,         DATA_HEX,   REG_WP_EN },
    [CJAP_RESET]                  = { CLK_PTP_RESET_REG,               MASK_0000_0010,   DATA_DEC,   REG_WP_EN },
    [NTM_RESET]                   = { CLK_PTP_RESET_REG,               MASK_0000_0100,   DATA_DEC,   REG_WP_EN },
    [GNSS_RESET]                  = { CLK_PTP_RESET_REG,               MASK_0000_1000,   DATA_DEC,   REG_WP_EN },
    [BITS_RESET]                  = { CLK_PTP_RESET_REG,               MASK_0010_0000,   DATA_DEC,   REG_WP_EN },
    [CLK_TIMING_CTRL]             = { CLK_TIMING_CTRL_REG,             MASK_ALL,         DATA_HEX,   REG_WP_EN },
    [GNSS_STATUS]                 = { GNSS_STATUS_REG,                 MASK_0000_0111,   DATA_DEC,   REG_WP_EN },
    [TIMING_STATUS]               = { TIMING_STATUS_REG,               MASK_0000_0111,   DATA_DEC,   REG_WP_EN },

    // @ CLK_BUFFER_EN_CTRL_REG (0xC6)
    [QSFPDD_SEL]                  = { CLK_BUFFER_EN_CTRL_REG,          MASK_1111_0000,   DATA_DEC,   REG_WP_EN },
    [QSFP28_SEL]                  = { CLK_BUFFER_EN_CTRL_REG,          MASK_0000_1111,   DATA_DEC,   REG_WP_EN },

    // @ CLK_BUFFER_EN_CTRL_REG (0xC6)
    [QSFPDD_SEL]                  = { CLK_BUFFER_EN_CTRL_REG,          MASK_1111_0000,   DATA_DEC,   REG_WP_DIS}, // Bits [7:4]
    [QSFP28_SEL]                  = { CLK_BUFFER_EN_CTRL_REG,          MASK_0000_1111,   DATA_DEC,   REG_WP_DIS}, // Bits [3:0]

    // @ MAIN_I2C_MUX_RESET_REG (0xc7)
    [I2C_MUX_0X76_RESET]          = { MAIN_I2C_MUX_RESET_REG,          MASK_0000_0001,   DATA_DEC,   REG_WP_EN },
    [I2C_MUX_0X75_RESET]          = { MAIN_I2C_MUX_RESET_REG,          MASK_0000_0010,   DATA_DEC,   REG_WP_EN },
    [I2C_MUX_QSFP28_6_11_RESET]   = { MAIN_I2C_MUX_RESET_REG,          MASK_0000_0100,   DATA_DEC,   REG_WP_EN },
    [I2C_MUX_SFP56_16_23_RESET]   = { MAIN_I2C_MUX_RESET_REG,          MASK_0000_1000,   DATA_DEC,   REG_WP_EN },
    [I2C_MUX_SFP56_24_31_RESET]   = { MAIN_I2C_MUX_RESET_REG,          MASK_0001_0000,   DATA_DEC,   REG_WP_EN },
    [I2C_MUX_SFP56_32_39_RESET]   = { MAIN_I2C_MUX_RESET_REG,          MASK_0010_0000,   DATA_DEC,   REG_WP_EN },
    [I2C_MUX_0X71_RESET]          = { MAIN_I2C_MUX_RESET_REG,          MASK_0100_0000,   DATA_DEC,   REG_WP_EN },

    /******************************************************************************
    * CPLD 2                                                                     *
    ******************************************************************************/

    // Ports Interrupt Status
    // @ QSFP28_0_5_ABS,
    [QSFP28_P0_ABS]               = { QSFP28_0_5_ABS_REG,              MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P1_ABS]               = { QSFP28_0_5_ABS_REG,              MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P2_ABS]               = { QSFP28_0_5_ABS_REG,              MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P3_ABS]               = { QSFP28_0_5_ABS_REG,              MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P4_ABS]               = { QSFP28_0_5_ABS_REG,              MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P5_ABS]               = { QSFP28_0_5_ABS_REG,              MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},
    // @ QSFPDD_12_15_ABS,
    [QSFPDD_P12_ABS]              = { QSFPDD_12_15_ABS_REG,            MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [QSFPDD_P13_ABS]              = { QSFPDD_12_15_ABS_REG,            MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [QSFPDD_P14_ABS]              = { QSFPDD_12_15_ABS_REG,            MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [QSFPDD_P15_ABS]              = { QSFPDD_12_15_ABS_REG,            MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},

    // @ QSFP28_0_5_INTR,
    [QSFP28_P0_INTR]              = { QSFP28_0_5_INTR_REG,             MASK_0000_0001,  DATA_HEX,    REG_WP_DIS},
    [QSFP28_P1_INTR]              = { QSFP28_0_5_INTR_REG,             MASK_0000_0010,  DATA_HEX,    REG_WP_DIS},
    [QSFP28_P2_INTR]              = { QSFP28_0_5_INTR_REG,             MASK_0000_0100,  DATA_HEX,    REG_WP_DIS},
    [QSFP28_P3_INTR]              = { QSFP28_0_5_INTR_REG,             MASK_0000_1000,  DATA_HEX,    REG_WP_DIS},
    [QSFP28_P4_INTR]              = { QSFP28_0_5_INTR_REG,             MASK_0001_0000,  DATA_HEX,    REG_WP_DIS},
    [QSFP28_P5_INTR]              = { QSFP28_0_5_INTR_REG,             MASK_0010_0000,  DATA_HEX,    REG_WP_DIS},
    // @ QSFPDD_12_15_INTR,
    [QSFPDD_P12_INTR]             = { QSFPDD_12_15_INTR_REG,           MASK_0000_0001,  DATA_HEX,    REG_WP_DIS},
    [QSFPDD_P13_INTR]             = { QSFPDD_12_15_INTR_REG,           MASK_0000_0010,  DATA_HEX,    REG_WP_DIS},
    [QSFPDD_P14_INTR]             = { QSFPDD_12_15_INTR_REG,           MASK_0000_0100,  DATA_HEX,    REG_WP_DIS},
    [QSFPDD_P15_INTR]             = { QSFPDD_12_15_INTR_REG,           MASK_0000_1000,  DATA_HEX,    REG_WP_DIS},


    // @ QSFP28_0_5_EFUSE_PG,
    [QSFP28_P0_EFUSE_PG]          = { QSFP28_0_5_EFUSE_PG_REG,         MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P1_EFUSE_PG]          = { QSFP28_0_5_EFUSE_PG_REG,         MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P2_EFUSE_PG]          = { QSFP28_0_5_EFUSE_PG_REG,         MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P3_EFUSE_PG]          = { QSFP28_0_5_EFUSE_PG_REG,         MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P4_EFUSE_PG]          = { QSFP28_0_5_EFUSE_PG_REG,         MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P5_EFUSE_PG]          = { QSFP28_0_5_EFUSE_PG_REG,         MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},
    // @ QSFPDD_12_15_EFUSE_PG,
    [QSFPDD_P12_EFUSE_PG]         = { QSFPDD_12_15_EFUSE_PG_REG,       MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [QSFPDD_P13_EFUSE_PG]         = { QSFPDD_12_15_EFUSE_PG_REG,       MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [QSFPDD_P14_EFUSE_PG]         = { QSFPDD_12_15_EFUSE_PG_REG,       MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [QSFPDD_P15_EFUSE_PG]         = { QSFPDD_12_15_EFUSE_PG_REG,       MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},

    // Interrupt Mask
    // @ QSFP28_0_5_MASK_ABS,
    [QSFP28_P0_MASK_ABS]          = { QSFP28_0_5_MASK_ABS_REG,         MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P1_MASK_ABS]          = { QSFP28_0_5_MASK_ABS_REG,         MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P2_MASK_ABS]          = { QSFP28_0_5_MASK_ABS_REG,         MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P3_MASK_ABS]          = { QSFP28_0_5_MASK_ABS_REG,         MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P4_MASK_ABS]          = { QSFP28_0_5_MASK_ABS_REG,         MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P5_MASK_ABS]          = { QSFP28_0_5_MASK_ABS_REG,         MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},
    // @ QSFPDD_12_15_MASK_ABS,
    [QSFPDD_P12_MASK_ABS]         = { QSFPDD_12_15_MASK_ABS_REG,       MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [QSFPDD_P13_MASK_ABS]         = { QSFPDD_12_15_MASK_ABS_REG,       MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [QSFPDD_P14_MASK_ABS]         = { QSFPDD_12_15_MASK_ABS_REG,       MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [QSFPDD_P15_MASK_ABS]         = { QSFPDD_12_15_MASK_ABS_REG,       MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},

    // @ QSFP28_0_5_MASK_INTR,
    [QSFP28_P0_MASK_INTR]         = { QSFP28_0_5_MASK_INTR_REG,        MASK_0000_0001,  DATA_HEX,    REG_WP_DIS},
    [QSFP28_P1_MASK_INTR]         = { QSFP28_0_5_MASK_INTR_REG,        MASK_0000_0010,  DATA_HEX,    REG_WP_DIS},
    [QSFP28_P2_MASK_INTR]         = { QSFP28_0_5_MASK_INTR_REG,        MASK_0000_0100,  DATA_HEX,    REG_WP_DIS},
    [QSFP28_P3_MASK_INTR]         = { QSFP28_0_5_MASK_INTR_REG,        MASK_0000_1000,  DATA_HEX,    REG_WP_DIS},
    [QSFP28_P4_MASK_INTR]         = { QSFP28_0_5_MASK_INTR_REG,        MASK_0001_0000,  DATA_HEX,    REG_WP_DIS},
    [QSFP28_P5_MASK_INTR]         = { QSFP28_0_5_MASK_INTR_REG,        MASK_0010_0000,  DATA_HEX,    REG_WP_DIS},
    // @ QSFPDD_12_15_MASK_INTR,
    [QSFPDD_P12_MASK_INTR]        = { QSFPDD_12_15_MASK_INTR_REG,      MASK_0000_0001,  DATA_HEX,    REG_WP_DIS},
    [QSFPDD_P13_MASK_INTR]        = { QSFPDD_12_15_MASK_INTR_REG,      MASK_0000_0010,  DATA_HEX,    REG_WP_DIS},
    [QSFPDD_P14_MASK_INTR]        = { QSFPDD_12_15_MASK_INTR_REG,      MASK_0000_0100,  DATA_HEX,    REG_WP_DIS},
    [QSFPDD_P15_MASK_INTR]        = { QSFPDD_12_15_MASK_INTR_REG,      MASK_0000_1000,  DATA_HEX,    REG_WP_DIS},

    // @ QSFP28_0_5_MASK_EFUSE_PG,
    [QSFP28_P0_MASK_EFUSE_PG]     = { QSFP28_0_5_MASK_EFUSE_PG_REG,    MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P1_MASK_EFUSE_PG]     = { QSFP28_0_5_MASK_EFUSE_PG_REG,    MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P2_MASK_EFUSE_PG]     = { QSFP28_0_5_MASK_EFUSE_PG_REG,    MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P3_MASK_EFUSE_PG]     = { QSFP28_0_5_MASK_EFUSE_PG_REG,    MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P4_MASK_EFUSE_PG]     = { QSFP28_0_5_MASK_EFUSE_PG_REG,    MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P5_MASK_EFUSE_PG]     = { QSFP28_0_5_MASK_EFUSE_PG_REG,    MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},
    // @ QSFPDD_12_15_MASK_EFUSE_PG,
    [QSFPDD_P12_MASK_EFUSE_PG]    = { QSFPDD_12_15_MASK_EFUSE_PG_REG,  MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [QSFPDD_P13_MASK_EFUSE_PG]    = { QSFPDD_12_15_MASK_EFUSE_PG_REG,  MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [QSFPDD_P14_MASK_EFUSE_PG]    = { QSFPDD_12_15_MASK_EFUSE_PG_REG,  MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [QSFPDD_P15_MASK_EFUSE_PG]    = { QSFPDD_12_15_MASK_EFUSE_PG_REG,  MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},

    // Interrupt Event
    [QSFP28_0_5_EVT_ABS]          = { QSFP28_0_5_EVT_ABS_REG,          MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [QSFPDD_12_15_EVT_ABS]        = { QSFPDD_12_15_EVT_ABS_REG,        MASK_ALL,        DATA_HEX,    REG_WP_DIS},

    [QSFP28_0_5_EVT_INTR]         = { QSFP28_0_5_EVT_INTR_REG,         MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [QSFPDD_12_15_EVT_INTR]       = { QSFPDD_12_15_EVT_INTR_REG,       MASK_ALL,        DATA_HEX,    REG_WP_DIS},

    [QSFP28_0_5_EVT_EFUSE_PG]     = { QSFP28_0_5_EVT_EFUSE_PG_REG,     MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [QSFPDD_12_15_EVT_EFUSE_PG]   = { QSFPDD_12_15_EVT_EFUSE_PG_REG,   MASK_ALL,        DATA_HEX,    REG_WP_DIS},

    // Reset
    // @ QSFP28_0_5_RESET,
    [QSFP28_P0_RESET]             = { QSFP28_0_5_RESET_REG,            MASK_0000_0001,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P1_RESET]             = { QSFP28_0_5_RESET_REG,            MASK_0000_0010,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P2_RESET]             = { QSFP28_0_5_RESET_REG,            MASK_0000_0100,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P3_RESET]             = { QSFP28_0_5_RESET_REG,            MASK_0000_1000,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P4_RESET]             = { QSFP28_0_5_RESET_REG,            MASK_0001_0000,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P5_RESET]             = { QSFP28_0_5_RESET_REG,            MASK_0010_0000,  DATA_DEC,    REG_WP_EN },
    // @ QSFPDD_12_15_RESET,
    [QSFPDD_P12_RESET]            = { QSFPDD_12_15_RESET_REG,          MASK_0000_0001,  DATA_DEC,    REG_WP_EN },
    [QSFPDD_P13_RESET]            = { QSFPDD_12_15_RESET_REG,          MASK_0000_0010,  DATA_DEC,    REG_WP_EN },
    [QSFPDD_P14_RESET]            = { QSFPDD_12_15_RESET_REG,          MASK_0000_0100,  DATA_DEC,    REG_WP_EN },
    [QSFPDD_P15_RESET]            = { QSFPDD_12_15_RESET_REG,          MASK_0000_1000,  DATA_DEC,    REG_WP_EN },

    // @ QSFP28_0_5_LPMODE,
    [QSFP28_P0_LPMODE]             = { QSFP28_0_5_LPMODE_REG,           MASK_0000_0001,  DATA_DEC,   REG_WP_EN },
    [QSFP28_P1_LPMODE]             = { QSFP28_0_5_LPMODE_REG,           MASK_0000_0010,  DATA_DEC,   REG_WP_EN },
    [QSFP28_P2_LPMODE]             = { QSFP28_0_5_LPMODE_REG,           MASK_0000_0100,  DATA_DEC,   REG_WP_EN },
    [QSFP28_P3_LPMODE]             = { QSFP28_0_5_LPMODE_REG,           MASK_0000_1000,  DATA_DEC,   REG_WP_EN },
    [QSFP28_P4_LPMODE]             = { QSFP28_0_5_LPMODE_REG,           MASK_0001_0000,  DATA_DEC,   REG_WP_EN },
    [QSFP28_P5_LPMODE]             = { QSFP28_0_5_LPMODE_REG,           MASK_0010_0000,  DATA_DEC,   REG_WP_EN },
    // @ QSFPDD_12_15_LPMODE,
    [QSFPDD_P12_LPMODE]            = { QSFPDD_12_15_LPMODE_REG,         MASK_0000_0001,  DATA_DEC,   REG_WP_EN },
    [QSFPDD_P13_LPMODE]            = { QSFPDD_12_15_LPMODE_REG,         MASK_0000_0010,  DATA_DEC,   REG_WP_EN },
    [QSFPDD_P14_LPMODE]            = { QSFPDD_12_15_LPMODE_REG,         MASK_0000_0100,  DATA_DEC,   REG_WP_EN },
    [QSFPDD_P15_LPMODE]            = { QSFPDD_12_15_LPMODE_REG,         MASK_0000_1000,  DATA_DEC,   REG_WP_EN },

    [CLK_EN_CTRL]                 = { CLK_EN_CTRL_REG,                  MASK_0000_0011,  DATA_DEC,   REG_WP_EN },

    /******************************************************************************
    * CPLD 3                                                                     *
    ******************************************************************************/

    // CPLD information
    [GNSS_MODEL_ID]               = { GNSS_MODEL_ID_REG,               MASK_0000_0111,   DATA_DEC,   REG_WP_DIS},
    [OCXO_ID]                     = { OCXO_ID_REG,                     MASK_0000_0111,   DATA_DEC,   REG_WP_DIS},

    // Interrupt Status
    // @ QSFP28_6_11_ABS,
    [QSFP28_P6_ABS]               = { QSFP28_6_11_ABS_REG,             MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P7_ABS]               = { QSFP28_6_11_ABS_REG,             MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P8_ABS]               = { QSFP28_6_11_ABS_REG,             MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P9_ABS]               = { QSFP28_6_11_ABS_REG,             MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P10_ABS]              = { QSFP28_6_11_ABS_REG,             MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P11_ABS]              = { QSFP28_6_11_ABS_REG,             MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},

    // @ QSFP28_6_11_INTR,
    [QSFP28_P6_INTR]              = { QSFP28_6_11_INTR_REG,            MASK_0000_0001,  DATA_HEX,    REG_WP_DIS},
    [QSFP28_P7_INTR]              = { QSFP28_6_11_INTR_REG,            MASK_0000_0010,  DATA_HEX,    REG_WP_DIS},
    [QSFP28_P8_INTR]              = { QSFP28_6_11_INTR_REG,            MASK_0000_0100,  DATA_HEX,    REG_WP_DIS},
    [QSFP28_P9_INTR]              = { QSFP28_6_11_INTR_REG,            MASK_0000_1000,  DATA_HEX,    REG_WP_DIS},
    [QSFP28_P10_INTR]             = { QSFP28_6_11_INTR_REG,            MASK_0001_0000,  DATA_HEX,    REG_WP_DIS},
    [QSFP28_P11_INTR]             = { QSFP28_6_11_INTR_REG,            MASK_0010_0000,  DATA_HEX,    REG_WP_DIS},

    // @ QSFP28_6_11_EFUSE_PG,
    [QSFP28_P6_EFUSE_PG]          = { QSFP28_6_11_EFUSE_PG_REG,        MASK_0000_0001,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P7_EFUSE_PG]          = { QSFP28_6_11_EFUSE_PG_REG,        MASK_0000_0010,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P8_EFUSE_PG]          = { QSFP28_6_11_EFUSE_PG_REG,        MASK_0000_0100,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P9_EFUSE_PG]          = { QSFP28_6_11_EFUSE_PG_REG,        MASK_0000_1000,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P10_EFUSE_PG]         = { QSFP28_6_11_EFUSE_PG_REG,        MASK_0001_0000,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P11_EFUSE_PG]         = { QSFP28_6_11_EFUSE_PG_REG,        MASK_0010_0000,  DATA_DEC,    REG_WP_EN },

    [MAC_INTR]                    = { MAC_INTR_REG,                    MASK_0000_0001,  DATA_HEX,    REG_WP_DIS},
    // @ FAN_ABS_REG (0x15)
    [FAN_0_ABS]                   = { FAN_ABS_REG,                     MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [FAN_1_ABS]                   = { FAN_ABS_REG,                     MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [FAN_2_ABS]                   = { FAN_ABS_REG,                     MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [FAN_3_ABS]                   = { FAN_ABS_REG,                     MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [FAN_4_ABS]                   = { FAN_ABS_REG,                     MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},

    // Interrupt Mask
    // @ QSFP28_6_11_MASK_ABS,
    [QSFP28_P6_MASK_ABS]          = { QSFP28_6_11_MASK_ABS_REG,        MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P7_MASK_ABS]          = { QSFP28_6_11_MASK_ABS_REG,        MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P8_MASK_ABS]          = { QSFP28_6_11_MASK_ABS_REG,        MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P9_MASK_ABS]          = { QSFP28_6_11_MASK_ABS_REG,        MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P10_MASK_ABS]         = { QSFP28_6_11_MASK_ABS_REG,        MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [QSFP28_P11_MASK_ABS]         = { QSFP28_6_11_MASK_ABS_REG,        MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},

    // @ QSFP28_6_11_MASK_INTR,
    [QSFP28_P6_MASK_INTR]         = { QSFP28_6_11_MASK_INTR_REG,       MASK_0000_0001,  DATA_HEX,    REG_WP_DIS},
    [QSFP28_P7_MASK_INTR]         = { QSFP28_6_11_MASK_INTR_REG,       MASK_0000_0010,  DATA_HEX,    REG_WP_DIS},
    [QSFP28_P8_MASK_INTR]         = { QSFP28_6_11_MASK_INTR_REG,       MASK_0000_0100,  DATA_HEX,    REG_WP_DIS},
    [QSFP28_P9_MASK_INTR]         = { QSFP28_6_11_MASK_INTR_REG,       MASK_0000_1000,  DATA_HEX,    REG_WP_DIS},
    [QSFP28_P10_MASK_INTR]        = { QSFP28_6_11_MASK_INTR_REG,       MASK_0001_0000,  DATA_HEX,    REG_WP_DIS},
    [QSFP28_P11_MASK_INTR]        = { QSFP28_6_11_MASK_INTR_REG,       MASK_0010_0000,  DATA_HEX,    REG_WP_DIS},

    // @ QSFP28_6_11_MASK_EFUSE_PG,
    [QSFP28_P6_MASK_EFUSE_PG]     = { QSFP28_6_11_MASK_EFUSE_PG_REG,   MASK_0000_0001,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P7_MASK_EFUSE_PG]     = { QSFP28_6_11_MASK_EFUSE_PG_REG,   MASK_0000_0010,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P8_MASK_EFUSE_PG]     = { QSFP28_6_11_MASK_EFUSE_PG_REG,   MASK_0000_0100,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P9_MASK_EFUSE_PG]     = { QSFP28_6_11_MASK_EFUSE_PG_REG,   MASK_0000_1000,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P10_MASK_EFUSE_PG]    = { QSFP28_6_11_MASK_EFUSE_PG_REG,   MASK_0001_0000,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P11_MASK_EFUSE_PG]    = { QSFP28_6_11_MASK_EFUSE_PG_REG,   MASK_0010_0000,  DATA_DEC,    REG_WP_EN },

    [MAC_MASK_INTR]               = { MAC_MASK_INTR_REG,               MASK_0000_0001,  DATA_HEX,    REG_WP_DIS},
    // @ FAN_MASK_ABS_REG (0x25)
    [FAN_0_MASK_ABS]              = { FAN_MASK_ABS_REG,               MASK_0000_0001,   DATA_DEC,    REG_WP_DIS},
    [FAN_1_MASK_ABS]              = { FAN_MASK_ABS_REG,               MASK_0000_0010,   DATA_DEC,    REG_WP_DIS},
    [FAN_2_MASK_ABS]              = { FAN_MASK_ABS_REG,               MASK_0000_0100,   DATA_DEC,    REG_WP_DIS},
    [FAN_3_MASK_ABS]              = { FAN_MASK_ABS_REG,               MASK_0000_1000,   DATA_DEC,    REG_WP_DIS},
    [FAN_4_MASK_ABS]              = { FAN_MASK_ABS_REG,               MASK_0001_0000,   DATA_DEC,    REG_WP_DIS},

    // Interrupt Event
    [QSFP28_6_11_EVT_ABS]         = { QSFP28_6_11_EVT_ABS_REG,         MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [QSFP28_6_11_EVT_INTR]        = { QSFP28_6_11_EVT_INTR_REG,        MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [QSFP28_6_11_EVT_EFUSE_PG]    = { QSFP28_6_11_EVT_EFUSE_PG_REG,    MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [MAC_EVT_INTR]                = { MAC_EVT_INTR_REG,                MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [FAN_EVT_ABS]                 = { FAN_EVT_ABS_REG,                 MASK_ALL,        DATA_HEX,    REG_WP_DIS},

    // Reset
    // @ QSFP28_6_11_RESET,
    [QSFP28_P6_RESET]             = { QSFP28_6_11_RESET_REG,           MASK_0000_0001,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P7_RESET]             = { QSFP28_6_11_RESET_REG,           MASK_0000_0010,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P8_RESET]             = { QSFP28_6_11_RESET_REG,           MASK_0000_0100,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P9_RESET]             = { QSFP28_6_11_RESET_REG,           MASK_0000_1000,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P10_RESET]            = { QSFP28_6_11_RESET_REG,           MASK_0001_0000,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P11_RESET]            = { QSFP28_6_11_RESET_REG,           MASK_0010_0000,  DATA_DEC,    REG_WP_EN },

    // @ QSFP28_6_11_LPMODE,
    [QSFP28_P6_LPMODE]            = { QSFP28_6_11_LPMODE_REG,          MASK_0000_0001,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P7_LPMODE]            = { QSFP28_6_11_LPMODE_REG,          MASK_0000_0010,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P8_LPMODE]            = { QSFP28_6_11_LPMODE_REG,          MASK_0000_0100,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P9_LPMODE]            = { QSFP28_6_11_LPMODE_REG,          MASK_0000_1000,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P10_LPMODE]           = { QSFP28_6_11_LPMODE_REG,          MASK_0001_0000,  DATA_DEC,    REG_WP_EN },
    [QSFP28_P11_LPMODE]           = { QSFP28_6_11_LPMODE_REG,          MASK_0010_0000,  DATA_DEC,    REG_WP_EN },

    // Reset
    [MAC_RESET]                   = { MAC_RESET_REG,                   MASK_0000_0001,  DATA_DEC,    REG_WP_EN },

    [USB_QSPI_RESET]              = { USB_QSPI_RESET_REG,              MASK_0000_0001,   DATA_DEC,   REG_WP_EN },
    [MAC_ROV]                     = { ROV_STATUS_REG,                  MASK_ALL,         DATA_DEC,   REG_WP_EN },
    [I2C_ROV_MUX_SEL]             = { MISC_CONTROL_REG,                MASK_0000_0010,   DATA_DEC,   REG_WP_EN },

    // @ I2C_MUX_SELECT,
    [I2C_CPLD_MUX_EN]             = { I2C_MUX_SELECT_REG,              MASK_0000_0001,  DATA_DEC,    REG_WP_EN },
    [I2C0_PSU_MUX_SEL]            = { I2C_MUX_SELECT_REG,              MASK_0000_0010,  DATA_DEC,    REG_WP_EN },
    [I2C0_HWM_MUX_SEL]            = { I2C_MUX_SELECT_REG,              MASK_0000_0100,  DATA_DEC,    REG_WP_EN },
    [I2C_IO_MUX_SEL]              = { I2C_MUX_SELECT_REG,              MASK_0000_1000,  DATA_DEC,    REG_WP_EN },

    // FAN Control
    [FAN_SPEED_READ_MODE]         = { FAN_SPEED_READ_MODE_REG,         MASK_0000_1111,   DATA_DEC,   REG_WP_DIS},
    // 0x73~0x74: FAN RPM read value
    [FAN_RPM_LOW_BYTE]            = { FAN_RPM_READ_VALUE_0_REG,        MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [FAN_RPM_HIGH_BYTE]           = { FAN_RPM_READ_VALUE_1_REG,        MASK_ALL,        DATA_HEX,    REG_WP_DIS},

    // Status Registers
    [SYSTEM_LED_STATUS_1]         = { SYSTEM_LED_STATUS_1_REG,         MASK_ALL,        DATA_HEX,    REG_WP_DIS},

    /******************************************************************************
    * CPLD 4                                                                     *
    ******************************************************************************/

    // Interrupt Status
    // @ SFP56_16_23_ABS,
    [SFP56_P16_ABS]               = { SFP56_16_23_ABS_REG,             MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P17_ABS]               = { SFP56_16_23_ABS_REG,             MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P18_ABS]               = { SFP56_16_23_ABS_REG,             MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P19_ABS]               = { SFP56_16_23_ABS_REG,             MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P20_ABS]               = { SFP56_16_23_ABS_REG,             MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P21_ABS]               = { SFP56_16_23_ABS_REG,             MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P22_ABS]               = { SFP56_16_23_ABS_REG,             MASK_0100_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P23_ABS]               = { SFP56_16_23_ABS_REG,             MASK_1000_0000,  DATA_DEC,    REG_WP_DIS},
    // @ SFP56_24_31_ABS,
    [SFP56_P24_ABS]               = { SFP56_24_31_ABS_REG,             MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P25_ABS]               = { SFP56_24_31_ABS_REG,             MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P26_ABS]               = { SFP56_24_31_ABS_REG,             MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P27_ABS]               = { SFP56_24_31_ABS_REG,             MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P28_ABS]               = { SFP56_24_31_ABS_REG,             MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P29_ABS]               = { SFP56_24_31_ABS_REG,             MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P30_ABS]               = { SFP56_24_31_ABS_REG,             MASK_0100_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P31_ABS]               = { SFP56_24_31_ABS_REG,             MASK_1000_0000,  DATA_DEC,    REG_WP_DIS},
    // @ SFP56_32_39_ABS,
    [SFP56_P32_ABS]               = { SFP56_32_39_ABS_REG,             MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P33_ABS]               = { SFP56_32_39_ABS_REG,             MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P34_ABS]               = { SFP56_32_39_ABS_REG,             MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P35_ABS]               = { SFP56_32_39_ABS_REG,             MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P36_ABS]               = { SFP56_32_39_ABS_REG,             MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P37_ABS]               = { SFP56_32_39_ABS_REG,             MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P38_ABS]               = { SFP56_32_39_ABS_REG,             MASK_0100_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P39_ABS]               = { SFP56_32_39_ABS_REG,             MASK_1000_0000,  DATA_DEC,    REG_WP_DIS},

    // @ SFP56_16_23_RX_LOS,
    [SFP56_P16_RX_LOS]            = { SFP56_16_23_RX_LOS_REG,          MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P17_RX_LOS]            = { SFP56_16_23_RX_LOS_REG,          MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P18_RX_LOS]            = { SFP56_16_23_RX_LOS_REG,          MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P19_RX_LOS]            = { SFP56_16_23_RX_LOS_REG,          MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P20_RX_LOS]            = { SFP56_16_23_RX_LOS_REG,          MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P21_RX_LOS]            = { SFP56_16_23_RX_LOS_REG,          MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P22_RX_LOS]            = { SFP56_16_23_RX_LOS_REG,          MASK_0100_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P23_RX_LOS]            = { SFP56_16_23_RX_LOS_REG,          MASK_1000_0000,  DATA_DEC,    REG_WP_DIS},
    // @ SFP56_24_31_RX_LOS,
    [SFP56_P24_RX_LOS]            = { SFP56_24_31_RX_LOS_REG,          MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P25_RX_LOS]            = { SFP56_24_31_RX_LOS_REG,          MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P26_RX_LOS]            = { SFP56_24_31_RX_LOS_REG,          MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P27_RX_LOS]            = { SFP56_24_31_RX_LOS_REG,          MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P28_RX_LOS]            = { SFP56_24_31_RX_LOS_REG,          MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P29_RX_LOS]            = { SFP56_24_31_RX_LOS_REG,          MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P30_RX_LOS]            = { SFP56_24_31_RX_LOS_REG,          MASK_0100_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P31_RX_LOS]            = { SFP56_24_31_RX_LOS_REG,          MASK_1000_0000,  DATA_DEC,    REG_WP_DIS},
    // @ SFP56_32_39_RX_LOS,
    [SFP56_P32_RX_LOS]            = { SFP56_32_39_RX_LOS_REG,          MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P33_RX_LOS]            = { SFP56_32_39_RX_LOS_REG,          MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P34_RX_LOS]            = { SFP56_32_39_RX_LOS_REG,          MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P35_RX_LOS]            = { SFP56_32_39_RX_LOS_REG,          MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P36_RX_LOS]            = { SFP56_32_39_RX_LOS_REG,          MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P37_RX_LOS]            = { SFP56_32_39_RX_LOS_REG,          MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P38_RX_LOS]            = { SFP56_32_39_RX_LOS_REG,          MASK_0100_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P39_RX_LOS]            = { SFP56_32_39_RX_LOS_REG,          MASK_1000_0000,  DATA_DEC,    REG_WP_DIS},

    // @ SFP56_16_23_TX_FAULT,
    [SFP56_P16_TX_FAULT]          = { SFP56_16_23_TX_FAULT_REG,        MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P17_TX_FAULT]          = { SFP56_16_23_TX_FAULT_REG,        MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P18_TX_FAULT]          = { SFP56_16_23_TX_FAULT_REG,        MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P19_TX_FAULT]          = { SFP56_16_23_TX_FAULT_REG,        MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P20_TX_FAULT]          = { SFP56_16_23_TX_FAULT_REG,        MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P21_TX_FAULT]          = { SFP56_16_23_TX_FAULT_REG,        MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P22_TX_FAULT]          = { SFP56_16_23_TX_FAULT_REG,        MASK_0100_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P23_TX_FAULT]          = { SFP56_16_23_TX_FAULT_REG,        MASK_1000_0000,  DATA_DEC,    REG_WP_DIS},
    // @ SFP56_24_31_TX_FAULT,
    [SFP56_P24_TX_FAULT]          = { SFP56_24_31_TX_FAULT_REG,        MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P25_TX_FAULT]          = { SFP56_24_31_TX_FAULT_REG,        MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P26_TX_FAULT]          = { SFP56_24_31_TX_FAULT_REG,        MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P27_TX_FAULT]          = { SFP56_24_31_TX_FAULT_REG,        MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P28_TX_FAULT]          = { SFP56_24_31_TX_FAULT_REG,        MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P29_TX_FAULT]          = { SFP56_24_31_TX_FAULT_REG,        MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P30_TX_FAULT]          = { SFP56_24_31_TX_FAULT_REG,        MASK_0100_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P31_TX_FAULT]          = { SFP56_24_31_TX_FAULT_REG,        MASK_1000_0000,  DATA_DEC,    REG_WP_DIS},
    // @ SFP56_32_39_TX_FAULT,
    [SFP56_P32_TX_FAULT]          = { SFP56_32_39_TX_FAULT_REG,        MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P33_TX_FAULT]          = { SFP56_32_39_TX_FAULT_REG,        MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P34_TX_FAULT]          = { SFP56_32_39_TX_FAULT_REG,        MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P35_TX_FAULT]          = { SFP56_32_39_TX_FAULT_REG,        MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P36_TX_FAULT]          = { SFP56_32_39_TX_FAULT_REG,        MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P37_TX_FAULT]          = { SFP56_32_39_TX_FAULT_REG,        MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P38_TX_FAULT]          = { SFP56_32_39_TX_FAULT_REG,        MASK_0100_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P39_TX_FAULT]          = { SFP56_32_39_TX_FAULT_REG,        MASK_1000_0000,  DATA_DEC,    REG_WP_DIS},

    // Interrupt Mask
    // @ SFP56_16_23_MASK_ABS,
    [SFP56_P16_MASK_ABS]          = { SFP56_16_23_MASK_ABS_REG,        MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P17_MASK_ABS]          = { SFP56_16_23_MASK_ABS_REG,        MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P18_MASK_ABS]          = { SFP56_16_23_MASK_ABS_REG,        MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P19_MASK_ABS]          = { SFP56_16_23_MASK_ABS_REG,        MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P20_MASK_ABS]          = { SFP56_16_23_MASK_ABS_REG,        MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P21_MASK_ABS]          = { SFP56_16_23_MASK_ABS_REG,        MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P22_MASK_ABS]          = { SFP56_16_23_MASK_ABS_REG,        MASK_0100_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P23_MASK_ABS]          = { SFP56_16_23_MASK_ABS_REG,        MASK_1000_0000,  DATA_DEC,    REG_WP_DIS},
    // @ SFP56_24_31_MASK_ABS,
    [SFP56_P24_MASK_ABS]          = { SFP56_24_31_MASK_ABS_REG,        MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P25_MASK_ABS]          = { SFP56_24_31_MASK_ABS_REG,        MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P26_MASK_ABS]          = { SFP56_24_31_MASK_ABS_REG,        MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P27_MASK_ABS]          = { SFP56_24_31_MASK_ABS_REG,        MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P28_MASK_ABS]          = { SFP56_24_31_MASK_ABS_REG,        MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P29_MASK_ABS]          = { SFP56_24_31_MASK_ABS_REG,        MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P30_MASK_ABS]          = { SFP56_24_31_MASK_ABS_REG,        MASK_0100_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P31_MASK_ABS]          = { SFP56_24_31_MASK_ABS_REG,        MASK_1000_0000,  DATA_DEC,    REG_WP_DIS},
    // @ SFP56_32_39_MASK_ABS,
    [SFP56_P32_MASK_ABS]          = { SFP56_32_39_MASK_ABS_REG,        MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P33_MASK_ABS]          = { SFP56_32_39_MASK_ABS_REG,        MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P34_MASK_ABS]          = { SFP56_32_39_MASK_ABS_REG,        MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P35_MASK_ABS]          = { SFP56_32_39_MASK_ABS_REG,        MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P36_MASK_ABS]          = { SFP56_32_39_MASK_ABS_REG,        MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P37_MASK_ABS]          = { SFP56_32_39_MASK_ABS_REG,        MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P38_MASK_ABS]          = { SFP56_32_39_MASK_ABS_REG,        MASK_0100_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P39_MASK_ABS]          = { SFP56_32_39_MASK_ABS_REG,        MASK_1000_0000,  DATA_DEC,    REG_WP_DIS},

    // @ SFP56_16_23_MASK_RX_LOS,
    [SFP56_P16_MASK_RX_LOS]       = { SFP56_16_23_MASK_RX_LOS_REG,     MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P17_MASK_RX_LOS]       = { SFP56_16_23_MASK_RX_LOS_REG,     MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P18_MASK_RX_LOS]       = { SFP56_16_23_MASK_RX_LOS_REG,     MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P19_MASK_RX_LOS]       = { SFP56_16_23_MASK_RX_LOS_REG,     MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P20_MASK_RX_LOS]       = { SFP56_16_23_MASK_RX_LOS_REG,     MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P21_MASK_RX_LOS]       = { SFP56_16_23_MASK_RX_LOS_REG,     MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P22_MASK_RX_LOS]       = { SFP56_16_23_MASK_RX_LOS_REG,     MASK_0100_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P23_MASK_RX_LOS]       = { SFP56_16_23_MASK_RX_LOS_REG,     MASK_1000_0000,  DATA_DEC,    REG_WP_DIS},
    // @ SFP56_24_31_MASK_RX_LOS,
    [SFP56_P24_MASK_RX_LOS]       = { SFP56_24_31_MASK_RX_LOS_REG,     MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P25_MASK_RX_LOS]       = { SFP56_24_31_MASK_RX_LOS_REG,     MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P26_MASK_RX_LOS]       = { SFP56_24_31_MASK_RX_LOS_REG,     MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P27_MASK_RX_LOS]       = { SFP56_24_31_MASK_RX_LOS_REG,     MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P28_MASK_RX_LOS]       = { SFP56_24_31_MASK_RX_LOS_REG,     MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P29_MASK_RX_LOS]       = { SFP56_24_31_MASK_RX_LOS_REG,     MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P30_MASK_RX_LOS]       = { SFP56_24_31_MASK_RX_LOS_REG,     MASK_0100_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P31_MASK_RX_LOS]       = { SFP56_24_31_MASK_RX_LOS_REG,     MASK_1000_0000,  DATA_DEC,    REG_WP_DIS},
    // @ SFP56_32_39_MASK_RX_LOS,
    [SFP56_P32_MASK_RX_LOS]       = { SFP56_32_39_MASK_RX_LOS_REG,     MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P33_MASK_RX_LOS]       = { SFP56_32_39_MASK_RX_LOS_REG,     MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P34_MASK_RX_LOS]       = { SFP56_32_39_MASK_RX_LOS_REG,     MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P35_MASK_RX_LOS]       = { SFP56_32_39_MASK_RX_LOS_REG,     MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P36_MASK_RX_LOS]       = { SFP56_32_39_MASK_RX_LOS_REG,     MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P37_MASK_RX_LOS]       = { SFP56_32_39_MASK_RX_LOS_REG,     MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P38_MASK_RX_LOS]       = { SFP56_32_39_MASK_RX_LOS_REG,     MASK_0100_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P39_MASK_RX_LOS]       = { SFP56_32_39_MASK_RX_LOS_REG,     MASK_1000_0000,  DATA_DEC,    REG_WP_DIS},

    // @ SFP56_16_23_MASK_TX_FAULT,
    [SFP56_P16_MASK_TX_FAULT]     = { SFP56_16_23_MASK_TX_FAULT_REG,   MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P17_MASK_TX_FAULT]     = { SFP56_16_23_MASK_TX_FAULT_REG,   MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P18_MASK_TX_FAULT]     = { SFP56_16_23_MASK_TX_FAULT_REG,   MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P19_MASK_TX_FAULT]     = { SFP56_16_23_MASK_TX_FAULT_REG,   MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P20_MASK_TX_FAULT]     = { SFP56_16_23_MASK_TX_FAULT_REG,   MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P21_MASK_TX_FAULT]     = { SFP56_16_23_MASK_TX_FAULT_REG,   MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P22_MASK_TX_FAULT]     = { SFP56_16_23_MASK_TX_FAULT_REG,   MASK_0100_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P23_MASK_TX_FAULT]     = { SFP56_16_23_MASK_TX_FAULT_REG,   MASK_1000_0000,  DATA_DEC,    REG_WP_DIS},
    // @ SFP56_24_31_MASK_TX_FAULT,
    [SFP56_P24_MASK_TX_FAULT]     = { SFP56_24_31_MASK_TX_FAULT_REG,   MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P25_MASK_TX_FAULT]     = { SFP56_24_31_MASK_TX_FAULT_REG,   MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P26_MASK_TX_FAULT]     = { SFP56_24_31_MASK_TX_FAULT_REG,   MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P27_MASK_TX_FAULT]     = { SFP56_24_31_MASK_TX_FAULT_REG,   MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P28_MASK_TX_FAULT]     = { SFP56_24_31_MASK_TX_FAULT_REG,   MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P29_MASK_TX_FAULT]     = { SFP56_24_31_MASK_TX_FAULT_REG,   MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P30_MASK_TX_FAULT]     = { SFP56_24_31_MASK_TX_FAULT_REG,   MASK_0100_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P31_MASK_TX_FAULT]     = { SFP56_24_31_MASK_TX_FAULT_REG,   MASK_1000_0000,  DATA_DEC,    REG_WP_DIS},
    // @ SFP56_32_39_MASK_TX_FAULT,
    [SFP56_P32_MASK_TX_FAULT]     = { SFP56_32_39_MASK_TX_FAULT_REG,   MASK_0000_0001,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P33_MASK_TX_FAULT]     = { SFP56_32_39_MASK_TX_FAULT_REG,   MASK_0000_0010,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P34_MASK_TX_FAULT]     = { SFP56_32_39_MASK_TX_FAULT_REG,   MASK_0000_0100,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P35_MASK_TX_FAULT]     = { SFP56_32_39_MASK_TX_FAULT_REG,   MASK_0000_1000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P36_MASK_TX_FAULT]     = { SFP56_32_39_MASK_TX_FAULT_REG,   MASK_0001_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P37_MASK_TX_FAULT]     = { SFP56_32_39_MASK_TX_FAULT_REG,   MASK_0010_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P38_MASK_TX_FAULT]     = { SFP56_32_39_MASK_TX_FAULT_REG,   MASK_0100_0000,  DATA_DEC,    REG_WP_DIS},
    [SFP56_P39_MASK_TX_FAULT]     = { SFP56_32_39_MASK_TX_FAULT_REG,   MASK_1000_0000,  DATA_DEC,    REG_WP_DIS},

    // Interrupt Event
    [SFP56_16_23_EVT_ABS]         = { SFP56_16_23_EVT_ABS_REG,         MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [SFP56_24_31_EVT_ABS]         = { SFP56_24_31_EVT_ABS_REG,         MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [SFP56_32_39_EVT_ABS]         = { SFP56_32_39_EVT_ABS_REG,         MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [SFP56_16_23_EVT_RX_LOS]      = { SFP56_16_23_EVT_RX_LOS_REG,      MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [SFP56_24_31_EVT_RX_LOS]      = { SFP56_24_31_EVT_RX_LOS_REG,      MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [SFP56_32_39_EVT_RX_LOS]      = { SFP56_32_39_EVT_RX_LOS_REG,      MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [SFP56_16_23_EVT_TX_FAULT]    = { SFP56_16_23_EVT_TX_FAULT_REG,    MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [SFP56_24_31_EVT_TX_FAULT]    = { SFP56_24_31_EVT_TX_FAULT_REG,    MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [SFP56_32_39_EVT_TX_FAULT]    = { SFP56_32_39_EVT_TX_FAULT_REG,    MASK_ALL,        DATA_HEX,    REG_WP_DIS},

    // SFP Control
    // @ SFP56_16_23_TX_DISABLE,
    [SFP56_P16_TX_DISABLE]        = { SFP56_16_23_TX_DISABLE_REG,      MASK_0000_0001,  DATA_DEC,    REG_WP_EN },
    [SFP56_P17_TX_DISABLE]        = { SFP56_16_23_TX_DISABLE_REG,      MASK_0000_0010,  DATA_DEC,    REG_WP_EN },
    [SFP56_P18_TX_DISABLE]        = { SFP56_16_23_TX_DISABLE_REG,      MASK_0000_0100,  DATA_DEC,    REG_WP_EN },
    [SFP56_P19_TX_DISABLE]        = { SFP56_16_23_TX_DISABLE_REG,      MASK_0000_1000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P20_TX_DISABLE]        = { SFP56_16_23_TX_DISABLE_REG,      MASK_0001_0000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P21_TX_DISABLE]        = { SFP56_16_23_TX_DISABLE_REG,      MASK_0010_0000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P22_TX_DISABLE]        = { SFP56_16_23_TX_DISABLE_REG,      MASK_0100_0000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P23_TX_DISABLE]        = { SFP56_16_23_TX_DISABLE_REG,      MASK_1000_0000,  DATA_DEC,    REG_WP_EN },
    // @ SFP56_24_31_TX_DISABLE,
    [SFP56_P24_TX_DISABLE]        = { SFP56_24_31_TX_DISABLE_REG,      MASK_0000_0001,  DATA_DEC,    REG_WP_EN },
    [SFP56_P25_TX_DISABLE]        = { SFP56_24_31_TX_DISABLE_REG,      MASK_0000_0010,  DATA_DEC,    REG_WP_EN },
    [SFP56_P26_TX_DISABLE]        = { SFP56_24_31_TX_DISABLE_REG,      MASK_0000_0100,  DATA_DEC,    REG_WP_EN },
    [SFP56_P27_TX_DISABLE]        = { SFP56_24_31_TX_DISABLE_REG,      MASK_0000_1000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P28_TX_DISABLE]        = { SFP56_24_31_TX_DISABLE_REG,      MASK_0001_0000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P29_TX_DISABLE]        = { SFP56_24_31_TX_DISABLE_REG,      MASK_0010_0000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P30_TX_DISABLE]        = { SFP56_24_31_TX_DISABLE_REG,      MASK_0100_0000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P31_TX_DISABLE]        = { SFP56_24_31_TX_DISABLE_REG,      MASK_1000_0000,  DATA_DEC,    REG_WP_EN },
    // @ SFP56_32_39_TX_DISABLE,
    [SFP56_P32_TX_DISABLE]        = { SFP56_32_39_TX_DISABLE_REG,      MASK_0000_0001,  DATA_DEC,    REG_WP_EN },
    [SFP56_P33_TX_DISABLE]        = { SFP56_32_39_TX_DISABLE_REG,      MASK_0000_0010,  DATA_DEC,    REG_WP_EN },
    [SFP56_P34_TX_DISABLE]        = { SFP56_32_39_TX_DISABLE_REG,      MASK_0000_0100,  DATA_DEC,    REG_WP_EN },
    [SFP56_P35_TX_DISABLE]        = { SFP56_32_39_TX_DISABLE_REG,      MASK_0000_1000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P36_TX_DISABLE]        = { SFP56_32_39_TX_DISABLE_REG,      MASK_0001_0000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P37_TX_DISABLE]        = { SFP56_32_39_TX_DISABLE_REG,      MASK_0010_0000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P38_TX_DISABLE]        = { SFP56_32_39_TX_DISABLE_REG,      MASK_0100_0000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P39_TX_DISABLE]        = { SFP56_32_39_TX_DISABLE_REG,      MASK_1000_0000,  DATA_DEC,    REG_WP_EN },

    // @ SFP56_16_23_RATE_SEL,
    [SFP56_P16_RATE_SEL]          = { SFP56_16_23_RATE_SEL_REG,        MASK_0000_0001,  DATA_DEC,    REG_WP_EN },
    [SFP56_P17_RATE_SEL]          = { SFP56_16_23_RATE_SEL_REG,        MASK_0000_0010,  DATA_DEC,    REG_WP_EN },
    [SFP56_P18_RATE_SEL]          = { SFP56_16_23_RATE_SEL_REG,        MASK_0000_0100,  DATA_DEC,    REG_WP_EN },
    [SFP56_P19_RATE_SEL]          = { SFP56_16_23_RATE_SEL_REG,        MASK_0000_1000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P20_RATE_SEL]          = { SFP56_16_23_RATE_SEL_REG,        MASK_0001_0000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P21_RATE_SEL]          = { SFP56_16_23_RATE_SEL_REG,        MASK_0010_0000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P22_RATE_SEL]          = { SFP56_16_23_RATE_SEL_REG,        MASK_0100_0000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P23_RATE_SEL]          = { SFP56_16_23_RATE_SEL_REG,        MASK_1000_0000,  DATA_DEC,    REG_WP_EN },
    // @ SFP56_24_31_RATE_SEL,
    [SFP56_P24_RATE_SEL]          = { SFP56_24_31_RATE_SEL_REG,        MASK_0000_0001,  DATA_DEC,    REG_WP_EN },
    [SFP56_P25_RATE_SEL]          = { SFP56_24_31_RATE_SEL_REG,        MASK_0000_0010,  DATA_DEC,    REG_WP_EN },
    [SFP56_P26_RATE_SEL]          = { SFP56_24_31_RATE_SEL_REG,        MASK_0000_0100,  DATA_DEC,    REG_WP_EN },
    [SFP56_P27_RATE_SEL]          = { SFP56_24_31_RATE_SEL_REG,        MASK_0000_1000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P28_RATE_SEL]          = { SFP56_24_31_RATE_SEL_REG,        MASK_0001_0000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P29_RATE_SEL]          = { SFP56_24_31_RATE_SEL_REG,        MASK_0010_0000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P30_RATE_SEL]          = { SFP56_24_31_RATE_SEL_REG,        MASK_0100_0000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P31_RATE_SEL]          = { SFP56_24_31_RATE_SEL_REG,        MASK_1000_0000,  DATA_DEC,    REG_WP_EN },
    // @ SFP56_32_39_RATE_SEL,
    [SFP56_P32_RATE_SEL]          = { SFP56_32_39_RATE_SEL_REG,        MASK_0000_0001,  DATA_DEC,    REG_WP_EN },
    [SFP56_P33_RATE_SEL]          = { SFP56_32_39_RATE_SEL_REG,        MASK_0000_0010,  DATA_DEC,    REG_WP_EN },
    [SFP56_P34_RATE_SEL]          = { SFP56_32_39_RATE_SEL_REG,        MASK_0000_0100,  DATA_DEC,    REG_WP_EN },
    [SFP56_P35_RATE_SEL]          = { SFP56_32_39_RATE_SEL_REG,        MASK_0000_1000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P36_RATE_SEL]          = { SFP56_32_39_RATE_SEL_REG,        MASK_0001_0000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P37_RATE_SEL]          = { SFP56_32_39_RATE_SEL_REG,        MASK_0010_0000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P38_RATE_SEL]          = { SFP56_32_39_RATE_SEL_REG,        MASK_0100_0000,  DATA_DEC,    REG_WP_EN },
    [SFP56_P39_RATE_SEL]          = { SFP56_32_39_RATE_SEL_REG,        MASK_1000_0000,  DATA_DEC,    REG_WP_EN },

    /******************************************************************************
    * BSP DEBUG                                                                    *
    ******************************************************************************/
    //BSP DEBUG
    [BSP_DEBUG]                   = { NONE_REG,                        MASK_ALL,        DATA_HEX,    REG_WP_DIS},
    [BSP_WP_ACCESS_COUNT]         = { NONE_REG,                        MASK_ALL,        DATA_DEC,    REG_WP_DIS},
};

/* CPLD sysfs attributes hook functions  */
static ssize_t cpld_show(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t cpld_store(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t cpld_reg_read(struct device *dev, u8 *reg_val, u8 reg, u8 mask);
static ssize_t cpld_reg_write(struct device *dev, u8 reg_val, size_t count, u8 reg, u8 mask,bool write_protect);
static ssize_t bsp_read(char *buf, char *str);
static ssize_t bsp_write(const char *buf, char *str, size_t str_len, size_t count);
static ssize_t bsp_callback_show(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t bsp_callback_store(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t version_h_show(struct device *dev, struct device_attribute *da, char *buf);

#if 0
static ssize_t led_show(struct device *dev,
        struct device_attribute *da, char *buf);
static ssize_t led_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
#endif

static LIST_HEAD(cpld_client_list);  /* client list for cpld */
static struct mutex list_lock;  /* mutex for client list */

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

// struct cpld_data {
//     int index;                  /* CPLD index */
//     struct mutex access_lock;   /* mutex for cpld access */
//     u8 access_reg;              /* register to access */
// };

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

    // CPLD Common
    // @ CPLD_VERSION,
static SENSOR_DEVICE_ATTR_RO(cpld_minor_ver,             cpld,    CPLD_MINOR_VER);
static SENSOR_DEVICE_ATTR_RO(cpld_major_ver,             cpld,    CPLD_MAJOR_VER);

static SENSOR_DEVICE_ATTR_RO(cpld_id,                    cpld,    CPLD_ID);

    // @ CPLD_BUILD,
static SENSOR_DEVICE_ATTR_RO(cpld_build_ver,             cpld,    CPLD_BUILD_VER);

static SENSOR_DEVICE_ATTR_RO(cpld_version_h,           version_h, CPLD_VERSION_H);
static SENSOR_DEVICE_ATTR_RW(event_detect_ctrl,          cpld,    EVENT_DETECT_CTRL);
static SENSOR_DEVICE_ATTR_RO(cpld_chip_type,             cpld,    CPLD_CHIP_TYPE);
static SENSOR_DEVICE_ATTR_RW(module_reset,               cpld,    MODULE_RESET);


/******************************************************************************
 * CPLD 1                                                                     *
 ******************************************************************************/

    // Board information
static SENSOR_DEVICE_ATTR_RO(brd_sku_id,                 cpld,    BRD_SKU_ID);

    // @ BRD_HW_BUILD_REV,
static SENSOR_DEVICE_ATTR_RO(brd_hw_id,                  cpld,    BRD_HW_ID);
static SENSOR_DEVICE_ATTR_RO(brd_deph_id,                cpld,    BRD_DEPH_ID);
static SENSOR_DEVICE_ATTR_RO(brd_build_id,               cpld,    BRD_BUILD_ID);
static SENSOR_DEVICE_ATTR_RO(brd_id_type,                cpld,    BRD_ID_TYPE);

static SENSOR_DEVICE_ATTR_RO(cpld_board_ext_id,          cpld,    CPLD_BOARD_EXT_ID);

    // CPLD information
static SENSOR_DEVICE_ATTR_RO(gddr6_id,                   cpld,    GDDR6_ID);
static SENSOR_DEVICE_ATTR_RO(gddr6_id_func,              cpld,    GDDR6_ID_FUNC);

    // Interrupt Status
static SENSOR_DEVICE_ATTR_RO(clk_ptp_intr,               cpld,    CLK_PTP_INTR);
static SENSOR_DEVICE_ATTR_RO(phy_intr,                   cpld,    PHY_INTR);

    // @ TOP_BRD_CPLD_FRU_INTR,
static SENSOR_DEVICE_ATTR_RO(top_brd_cpld_fru_intr,      cpld,    TOP_BRD_CPLD_FRU_INTR);
static SENSOR_DEVICE_ATTR_RO(psu0_intr,                  cpld,    PSU0_INTR);
static SENSOR_DEVICE_ATTR_RO(psu1_intr,                  cpld,    PSU1_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld2_intr,                 cpld,    CPLD2_INTR);
// static SENSOR_DEVICE_ATTR_RO(cpld2_io_intr,              cpld,    CPLD2_IO_INTR);
    // @ MAIN_BRD_CPLD_INTR,
static SENSOR_DEVICE_ATTR_RO(main_brd_cpld_intr,         cpld,    MAIN_BRD_CPLD_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld3_intr,                 cpld,    CPLD3_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld4_intr,                 cpld,    CPLD4_INTR);
static SENSOR_DEVICE_ATTR_RO(mb_eth_intr,                cpld,    MB_ETH_INTR);
// static SENSOR_DEVICE_ATTR_RO(mb_ptp_intr,                cpld,    MB_PTP_INTR);
static SENSOR_DEVICE_ATTR_RO(fan_intr,                   cpld,    FAN_INTR);

static SENSOR_DEVICE_ATTR_RO(thermal_intr,               cpld,    THERMAL_INTR);
static SENSOR_DEVICE_ATTR_RO(cpu_nmi_intr,               cpld,    CPU_NMI_INTR);
static SENSOR_DEVICE_ATTR_RO(out_status_intr,            cpld,    OUT_STATUS_INTR);

    // Interrupt Mask
static SENSOR_DEVICE_ATTR_RW(clk_ptp_mask_intr,          cpld,    CLK_PTP_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(phy_mask_intr,              cpld,    PHY_MASK_INTR);

    // @ TOP_BRD_CPLD_FRU_MASK_INTR,
static SENSOR_DEVICE_ATTR_RW(top_brd_cpld_fru_mask_intr, cpld,    TOP_BRD_CPLD_FRU_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(psu0_mask_intr,             cpld,    PSU0_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(psu1_mask_intr,             cpld,    PSU1_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(cpld2_mask_intr,            cpld,    CPLD2_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(cpld2_io_mask_intr,         cpld,    CPLD2_IO_MASK_INTR);
    // @ MAIN_BRD_CPLD_MASK_INTR,
static SENSOR_DEVICE_ATTR_RW(main_brd_cpld_mask_intr,    cpld,    MAIN_BRD_CPLD_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(cpld3_mask_intr,            cpld,    CPLD3_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(cpld4_mask_intr,            cpld,    CPLD4_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(mb_eth_mask_intr,           cpld,    MB_ETH_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(mb_ptp_mask_intr,           cpld,    MB_PTP_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(fan_mask_intr,              cpld,    FAN_MASK_INTR);

static SENSOR_DEVICE_ATTR_RW(thermal_mask_intr,          cpld,    THERMAL_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(cpu_nmi_mask_intr,          cpld,    CPU_NMI_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(out_status_mask_intr,       cpld,    OUT_STATUS_MASK_INTR);

    // Interrupt Event
static SENSOR_DEVICE_ATTR_RO(clk_ptp_evt_intr,           cpld,    CLK_PTP_EVT_INTR);
static SENSOR_DEVICE_ATTR_RO(phy_evt_intr,               cpld,    PHY_EVT_INTR);
static SENSOR_DEVICE_ATTR_RO(top_brd_cpld_fru_evt_intr,  cpld,    TOP_BRD_CPLD_FRU_EVT_INTR);
static SENSOR_DEVICE_ATTR_RO(main_brd_cpld_evt_intr,     cpld,    MAIN_BRD_CPLD_EVT_INTR);
static SENSOR_DEVICE_ATTR_RO(thermal_evt_intr,           cpld,    THERMAL_EVT_INTR);
static SENSOR_DEVICE_ATTR_RO(cpu_nmi_evt_intr,           cpld,    CPU_NMI_EVT_INTR);
static SENSOR_DEVICE_ATTR_RO(out_status_evt_intr,        cpld,    OUT_STATUS_EVT_INTR);


    // @ BIOS_FLASH_RESET_CTRL,
static SENSOR_DEVICE_ATTR_RW(btn_fp_reset,               cpld,    BTN_FP_RESET);
static SENSOR_DEVICE_ATTR_RW(spi_bios_reset,             cpld,    SPI_BIOS_RESET);

    // @ BMC_PHY_RESET_CTRL,
static SENSOR_DEVICE_ATTR_RW(rgb_0_reset,                cpld,    RGB_0_RESET);
static SENSOR_DEVICE_ATTR_RW(rgb_1_reset,                cpld,    RGB_1_RESET);
static SENSOR_DEVICE_ATTR_RW(bmc_lpc_reset,              cpld,    BMC_LPC_RESET);
static SENSOR_DEVICE_ATTR_RW(bmc_pcie_reset,             cpld,    BMC_PCIE_RESET);
static SENSOR_DEVICE_ATTR_RW(cpld_to_bmc_sys_reset,      cpld,    CPLD_TO_BMC_SYS_RESET);
static SENSOR_DEVICE_ATTR_RW(cpld_to_cpu_reset,          cpld,    CPLD_TO_CPU_RESET);


    // @ USB_RESET_CTRL,
static SENSOR_DEVICE_ATTR_RW(usb_pwr_en,                 cpld,    USB_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(usb_sie_reset,              cpld,    USB_SIE_RESET);

    // I2C_MUX_RESET,
static SENSOR_DEVICE_ATTR_RW(i2c_mux_sys_reset,          cpld,    I2C_MUX_SYS_RESET);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_smbus_reset,        cpld,    I2C_MUX_SMBUS_RESET);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_qsfp28_reset,       cpld,    I2C_MUX_QSFP28_RESET);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_sfp_reset,          cpld,    I2C_MUX_SFP_RESET);
static SENSOR_DEVICE_ATTR_RW(mb_i2c_reset,               cpld,    MB_I2C_RESET);

    // Misc Status Control
    // @ DAUGHTER_BRD_ABS,
static SENSOR_DEVICE_ATTR_RO(bmc_abs,                    cpld,    BMC_ABS);
static SENSOR_DEVICE_ATTR_RO(sata_ssd1_abs,              cpld,    SATA_SSD1_ABS);
static SENSOR_DEVICE_ATTR_RO(sata_ssd2_abs,              cpld,    SATA_SSD2_ABS);

    // @ PSU_STATUS,
static SENSOR_DEVICE_ATTR_RO(psu0_abs,                   cpld,    PSU0_ABS);
static SENSOR_DEVICE_ATTR_RO(psu1_abs,                   cpld,    PSU1_ABS);
static SENSOR_DEVICE_ATTR_RO(psu0_vin_pg,                cpld,    PSU0_VIN_PG);
static SENSOR_DEVICE_ATTR_RO(psu1_vin_pg,                cpld,    PSU1_VIN_PG);
static SENSOR_DEVICE_ATTR_RO(psu0_vout_pg,               cpld,    PSU0_VOUT_PG);
static SENSOR_DEVICE_ATTR_RO(psu1_vout_pg,               cpld,    PSU1_VOUT_PG);
static SENSOR_DEVICE_ATTR_RO(psu_status,                 cpld,    PSU_STATUS);

    // @ SYSTEM_PWR_STATUS,
static SENSOR_DEVICE_ATTR_RO(cpu_boot_done,              cpld,    CPU_BOOT_DONE);
static SENSOR_DEVICE_ATTR_RO(cpu_pg,                     cpld,    CPU_PG);

    // WD_STATUS,
static SENSOR_DEVICE_ATTR_RO(wdt_cpu_to_cpld,            cpld,    WDT_CPU_TO_CPLD);
static SENSOR_DEVICE_ATTR_RW(bmc_wdt_1_reset,            cpld,    BMC_WDT_1_RESET);
static SENSOR_DEVICE_ATTR_RW(bmc_wdt_2_reset,            cpld,    BMC_WDT_2_RESET);

    // @ MUX_CTRL,
static SENSOR_DEVICE_ATTR_RO(smbus_peci_dis,             cpld,    SMBUS_PECI_DIS);
static SENSOR_DEVICE_ATTR_RW(i2c_psu0_mux_sel,           cpld,    I2C_PSU0_MUX_SEL);
static SENSOR_DEVICE_ATTR_RW(i2c_psu1_mux_sel,           cpld,    I2C_PSU1_MUX_SEL);
static SENSOR_DEVICE_ATTR_RW(i2c_cpld_mux_sel,           cpld,    I2C_CPLD_MUX_SEL);
static SENSOR_DEVICE_ATTR_RW(bmc_usb_mux_sel,            cpld,    BMC_USB_MUX_SEL);
static SENSOR_DEVICE_ATTR_RW(uart_cpu_bmc_mux_sel,       cpld,    UART_CPU_BMC_MUX_SEL);
static SENSOR_DEVICE_ATTR_RW(uart_mux_sel,               cpld,    UART_MUX_SEL);

static SENSOR_DEVICE_ATTR_RW(ext_ctrl,                   cpld,    EXT_CTRL);

    // LED Control
    // @ SYSTEM_LED_CTRL_1,
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_sys,        cpld,    CPLD_SYSTEM_LED_SYS);
static SENSOR_DEVICE_ATTR_RW(system_led_status,          cpld,    SYSTEM_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(system_led_speed,           cpld,    SYSTEM_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(system_led_blink,           cpld,    SYSTEM_LED_BLINK);
static SENSOR_DEVICE_ATTR_RW(system_led_onoff,           cpld,    SYSTEM_LED_ONOFF);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_fan,        cpld,    CPLD_SYSTEM_LED_FAN);
static SENSOR_DEVICE_ATTR_RW(fan_led_status,             cpld,    FAN_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(fan_led_speed,              cpld,    FAN_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(fan_led_blink,              cpld,    FAN_LED_BLINK);
static SENSOR_DEVICE_ATTR_RW(fan_led_onoff,              cpld,    FAN_LED_ONOFF);

    // @ SYSTEM_LED_CTRL_2,
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_pwr,        cpld,    CPLD_SYSTEM_LED_PWR);
static SENSOR_DEVICE_ATTR_RW(pwr_led_status,             cpld,    PWR_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(pwr_led_speed,              cpld,    PWR_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(pwr_led_blink,              cpld,    PWR_LED_BLINK);
static SENSOR_DEVICE_ATTR_RW(pwr_led_onoff,              cpld,    PWR_LED_ONOFF);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_gnss,       cpld,    CPLD_SYSTEM_LED_GNSS);
static SENSOR_DEVICE_ATTR_RW(gnss_led_status,            cpld,    GNSS_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(gnss_led_speed,             cpld,    GNSS_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(gnss_led_blink,             cpld,    GNSS_LED_BLINK);
static SENSOR_DEVICE_ATTR_RW(gnss_led_onoff,             cpld,    GNSS_LED_ONOFF);

    // @ SYSTEM_LED_CTRL_3,
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_sync,       cpld,    CPLD_SYSTEM_LED_SYNC);
static SENSOR_DEVICE_ATTR_RW(sync_led_status,            cpld,    SYNC_LED_STATUS);
static SENSOR_DEVICE_ATTR_RW(sync_led_speed,             cpld,    SYNC_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(sync_led_blink,             cpld,    SYNC_LED_BLINK);
static SENSOR_DEVICE_ATTR_RW(sync_led_onoff,             cpld,    SYNC_LED_ONOFF);

static SENSOR_DEVICE_ATTR_RW(led_clear,                  cpld,    LED_CLEAR);

    // Internal Control
    // @ QSFP28_0_5_PWR_EN,
static SENSOR_DEVICE_ATTR_RW(qsfp28_p0_pwr_en,           cpld,    QSFP28_P0_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p1_pwr_en,           cpld,    QSFP28_P1_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p2_pwr_en,           cpld,    QSFP28_P2_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p3_pwr_en,           cpld,    QSFP28_P3_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p4_pwr_en,           cpld,    QSFP28_P4_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p5_pwr_en,           cpld,    QSFP28_P5_PWR_EN);

    // @ QSFP28_6_11_PWR_EN,
static SENSOR_DEVICE_ATTR_RW(qsfp28_p6_pwr_en,           cpld,    QSFP28_P6_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p7_pwr_en,           cpld,    QSFP28_P7_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p8_pwr_en,           cpld,    QSFP28_P8_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p9_pwr_en,           cpld,    QSFP28_P9_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p10_pwr_en,          cpld,    QSFP28_P10_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p11_pwr_en,          cpld,    QSFP28_P11_PWR_EN);

    // @ QSFPDD_12_15_PWR_EN,
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p12_pwr_en,          cpld,    QSFPDD_P12_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p13_pwr_en,          cpld,    QSFPDD_P13_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p14_pwr_en,          cpld,    QSFPDD_P14_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p15_pwr_en,          cpld,    QSFPDD_P15_PWR_EN);

    // @ SFP56_16_23_PWR_EN_0,
static SENSOR_DEVICE_ATTR_RW(sfp56_p16_pwr_en,           cpld,    SFP56_P16_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(sfp56_p17_pwr_en,           cpld,    SFP56_P17_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(sfp56_p18_pwr_en,           cpld,    SFP56_P18_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(sfp56_p19_pwr_en,           cpld,    SFP56_P19_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(sfp56_p20_pwr_en,           cpld,    SFP56_P20_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(sfp56_p21_pwr_en,           cpld,    SFP56_P21_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(sfp56_p22_pwr_en,           cpld,    SFP56_P22_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(sfp56_p23_pwr_en,           cpld,    SFP56_P23_PWR_EN);

    // @ SFP56_24_31_PWR_EN_1,
static SENSOR_DEVICE_ATTR_RW(sfp56_p24_pwr_en,           cpld,    SFP56_P24_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(sfp56_p25_pwr_en,           cpld,    SFP56_P25_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(sfp56_p26_pwr_en,           cpld,    SFP56_P26_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(sfp56_p27_pwr_en,           cpld,    SFP56_P27_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(sfp56_p28_pwr_en,           cpld,    SFP56_P28_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(sfp56_p29_pwr_en,           cpld,    SFP56_P29_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(sfp56_p30_pwr_en,           cpld,    SFP56_P30_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(sfp56_p31_pwr_en,           cpld,    SFP56_P31_PWR_EN);

    // @ SFP56_32_39_PWR_EN_2,
static SENSOR_DEVICE_ATTR_RW(sfp56_p32_pwr_en,           cpld,    SFP56_P32_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(sfp56_p33_pwr_en,           cpld,    SFP56_P33_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(sfp56_p34_pwr_en,           cpld,    SFP56_P34_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(sfp56_p35_pwr_en,           cpld,    SFP56_P35_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(sfp56_p36_pwr_en,           cpld,    SFP56_P36_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(sfp56_p37_pwr_en,           cpld,    SFP56_P37_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(sfp56_p38_pwr_en,           cpld,    SFP56_P38_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(sfp56_p39_pwr_en,           cpld,    SFP56_P39_PWR_EN);

static SENSOR_DEVICE_ATTR_RW(clk_ptp_reset,              cpld,    CLK_PTP_RESET);
static SENSOR_DEVICE_ATTR_RW(cjap_reset,                 cpld,    CJAP_RESET);
static SENSOR_DEVICE_ATTR_RW(ntm_reset,                  cpld,    NTM_RESET);
static SENSOR_DEVICE_ATTR_RW(gnss_reset,                 cpld,    GNSS_RESET);
static SENSOR_DEVICE_ATTR_RW(bits_reset,                 cpld,    BITS_RESET);
static SENSOR_DEVICE_ATTR_RW(clk_timing_ctrl,            cpld,    CLK_TIMING_CTRL);
static SENSOR_DEVICE_ATTR_RO(gnss_status,                cpld,    GNSS_STATUS);
static SENSOR_DEVICE_ATTR_RO(timing_status,              cpld,    TIMING_STATUS);

    // @ CLK_BUFFER_EN_CTRL,
static SENSOR_DEVICE_ATTR_RW(qsfpdd_sel,                 cpld,    QSFPDD_SEL);
static SENSOR_DEVICE_ATTR_RW(qsfp28_sel,                 cpld,    QSFP28_SEL);
    // I2C_MUX_RESET,
    // @ I2C_MUX_RESET,
static SENSOR_DEVICE_ATTR_RW(i2c_mux_0x76_reset,         cpld,    I2C_MUX_0X76_RESET);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_0x75_reset,         cpld,    I2C_MUX_0X75_RESET);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_6_11_reset,         cpld,    I2C_MUX_QSFP28_6_11_RESET);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_sfp56_16_23_reset,  cpld,    I2C_MUX_SFP56_16_23_RESET);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_sfp56_24_31_reset,  cpld,    I2C_MUX_SFP56_24_31_RESET);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_sfp56_32_39_reset,  cpld,    I2C_MUX_SFP56_32_39_RESET);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_0x71_reset,         cpld,    I2C_MUX_0X71_RESET);

/******************************************************************************
 * CPLD 2                                                                     *
 ******************************************************************************/

    // Ports Interrupt Status
    // @ QSFP28_0_5_ABS,
static SENSOR_DEVICE_ATTR_RO(qsfp28_p0_abs,              cpld,    QSFP28_P0_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p1_abs,              cpld,    QSFP28_P1_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p2_abs,              cpld,    QSFP28_P2_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p3_abs,              cpld,    QSFP28_P3_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p4_abs,              cpld,    QSFP28_P4_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p5_abs,              cpld,    QSFP28_P5_ABS);
    // @ QSFPDD_12_15_ABS,
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p12_abs,             cpld,    QSFPDD_P12_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p13_abs,             cpld,    QSFPDD_P13_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p14_abs,             cpld,    QSFPDD_P14_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p15_abs,             cpld,    QSFPDD_P15_ABS);

    // @ QSFP28_0_5_INTR,
static SENSOR_DEVICE_ATTR_RO(qsfp28_p0_intr,             cpld,    QSFP28_P0_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p1_intr,             cpld,    QSFP28_P1_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p2_intr,             cpld,    QSFP28_P2_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p3_intr,             cpld,    QSFP28_P3_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p4_intr,             cpld,    QSFP28_P4_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p5_intr,             cpld,    QSFP28_P5_INTR);
    // @ QSFPDD_12_15_INTR,
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p12_intr,            cpld,    QSFPDD_P12_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p13_intr,            cpld,    QSFPDD_P13_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p14_intr,            cpld,    QSFPDD_P14_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p15_intr,            cpld,    QSFPDD_P15_INTR);


    // @ QSFP28_0_5_EFUSE_PG,
static SENSOR_DEVICE_ATTR_RO(qsfp28_p0_efuse_pg,         cpld,    QSFP28_P0_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p1_efuse_pg,         cpld,    QSFP28_P1_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p2_efuse_pg,         cpld,    QSFP28_P2_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p3_efuse_pg,         cpld,    QSFP28_P3_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p4_efuse_pg,         cpld,    QSFP28_P4_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p5_efuse_pg,         cpld,    QSFP28_P5_EFUSE_PG);
    // @ QSFPDD_12_15_EFUSE_PG,
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p12_efuse_pg,        cpld,    QSFPDD_P12_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p13_efuse_pg,        cpld,    QSFPDD_P13_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p14_efuse_pg,        cpld,    QSFPDD_P14_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_p15_efuse_pg,        cpld,    QSFPDD_P15_EFUSE_PG);

    // Interrupt Mask
    // @ QSFP28_0_5_MASK_ABS,
static SENSOR_DEVICE_ATTR_RW(qsfp28_p0_mask_abs,         cpld,    QSFP28_P0_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p1_mask_abs,         cpld,    QSFP28_P1_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p2_mask_abs,         cpld,    QSFP28_P2_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p3_mask_abs,         cpld,    QSFP28_P3_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p4_mask_abs,         cpld,    QSFP28_P4_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p5_mask_abs,         cpld,    QSFP28_P5_MASK_ABS);
    // @ QSFPDD_12_15_MASK_ABS,
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p12_mask_abs,        cpld,    QSFPDD_P12_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p13_mask_abs,        cpld,    QSFPDD_P13_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p14_mask_abs,        cpld,    QSFPDD_P14_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p15_mask_abs,        cpld,    QSFPDD_P15_MASK_ABS);

    // @ QSFP28_0_5_MASK_INTR,
static SENSOR_DEVICE_ATTR_RW(qsfp28_p0_mask_intr,        cpld,    QSFP28_P0_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p1_mask_intr,        cpld,    QSFP28_P1_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p2_mask_intr,        cpld,    QSFP28_P2_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p3_mask_intr,        cpld,    QSFP28_P3_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p4_mask_intr,        cpld,    QSFP28_P4_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p5_mask_intr,        cpld,    QSFP28_P5_MASK_INTR);
    // @ QSFPDD_12_15_MASK_INTR,
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p12_mask_intr,       cpld,    QSFPDD_P12_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p13_mask_intr,       cpld,    QSFPDD_P13_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p14_mask_intr,       cpld,    QSFPDD_P14_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p15_mask_intr,       cpld,    QSFPDD_P15_MASK_INTR);

    // @ QSFP28_0_5_MASK_EFUSE_PG,
static SENSOR_DEVICE_ATTR_RW(qsfp28_p0_mask_efuse_pg,    cpld,    QSFP28_P0_MASK_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p1_mask_efuse_pg,    cpld,    QSFP28_P1_MASK_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p2_mask_efuse_pg,    cpld,    QSFP28_P2_MASK_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p3_mask_efuse_pg,    cpld,    QSFP28_P3_MASK_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p4_mask_efuse_pg,    cpld,    QSFP28_P4_MASK_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p5_mask_efuse_pg,    cpld,    QSFP28_P5_MASK_EFUSE_PG);
    // @ QSFPDD_12_15_MASK_EFUSE_PG,
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p12_mask_efuse_pg,   cpld,    QSFPDD_P12_MASK_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p13_mask_efuse_pg,   cpld,    QSFPDD_P13_MASK_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p14_mask_efuse_pg,   cpld,    QSFPDD_P14_MASK_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p15_mask_efuse_pg,   cpld,    QSFPDD_P15_MASK_EFUSE_PG);

    // Interrupt Event
static SENSOR_DEVICE_ATTR_RO(qsfp28_0_5_evt_abs,         cpld,    QSFP28_0_5_EVT_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_12_15_evt_abs,       cpld,    QSFPDD_12_15_EVT_ABS);

static SENSOR_DEVICE_ATTR_RO(qsfp28_0_5_evt_intr,        cpld,    QSFP28_0_5_EVT_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_12_15_evt_intr,      cpld,    QSFPDD_12_15_EVT_INTR);

static SENSOR_DEVICE_ATTR_RO(qsfp28_0_5_evt_efuse_pg,    cpld,    QSFP28_0_5_EVT_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(qsfpdd_12_15_evt_efuse_pg,  cpld,    QSFPDD_12_15_EVT_EFUSE_PG);

    // Reset
    // @ QSFP28_0_5_RESET,
static SENSOR_DEVICE_ATTR_RW(qsfp28_p0_reset,            cpld,    QSFP28_P0_RESET);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p1_reset,            cpld,    QSFP28_P1_RESET);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p2_reset,            cpld,    QSFP28_P2_RESET);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p3_reset,            cpld,    QSFP28_P3_RESET);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p4_reset,            cpld,    QSFP28_P4_RESET);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p5_reset,            cpld,    QSFP28_P5_RESET);
    // @ QSFPDD_12_15_RESET,
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p12_reset,           cpld,    QSFPDD_P12_RESET);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p13_reset,           cpld,    QSFPDD_P13_RESET);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p14_reset,           cpld,    QSFPDD_P14_RESET);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p15_reset,           cpld,    QSFPDD_P15_RESET);

    // @ QSFP28_0_5_LPMODE,
static SENSOR_DEVICE_ATTR_RW(qsfp28_p0_lpmode,            cpld,    QSFP28_P0_LPMODE);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p1_lpmode,            cpld,    QSFP28_P1_LPMODE);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p2_lpmode,            cpld,    QSFP28_P2_LPMODE);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p3_lpmode,            cpld,    QSFP28_P3_LPMODE);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p4_lpmode,            cpld,    QSFP28_P4_LPMODE);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p5_lpmode,            cpld,    QSFP28_P5_LPMODE);
    // @ QSFPDD_12_15_LPMODE,
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p12_lpmode,           cpld,    QSFPDD_P12_LPMODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p13_lpmode,           cpld,    QSFPDD_P13_LPMODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p14_lpmode,           cpld,    QSFPDD_P14_LPMODE);
static SENSOR_DEVICE_ATTR_RW(qsfpdd_p15_lpmode,           cpld,    QSFPDD_P15_LPMODE);

static SENSOR_DEVICE_ATTR_RW(clk_en_ctrl,                cpld,    CLK_EN_CTRL);

/******************************************************************************
 * CPLD 3                                                                     *
 ******************************************************************************/

    // CPLD information
static SENSOR_DEVICE_ATTR_RO(gnss_model_id,              cpld,    GNSS_MODEL_ID);
static SENSOR_DEVICE_ATTR_RO(ocxo_id,                    cpld,    OCXO_ID);

    // Interrupt Status
    // @ QSFP28_6_11_ABS,
static SENSOR_DEVICE_ATTR_RO(qsfp28_p6_abs,              cpld,    QSFP28_P6_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p7_abs,              cpld,    QSFP28_P7_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p8_abs,              cpld,    QSFP28_P8_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p9_abs,              cpld,    QSFP28_P9_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p10_abs,             cpld,    QSFP28_P10_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p11_abs,             cpld,    QSFP28_P11_ABS);

    // @ QSFP28_6_11_INTR,
static SENSOR_DEVICE_ATTR_RO(qsfp28_p6_intr,             cpld,    QSFP28_P6_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p7_intr,             cpld,    QSFP28_P7_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p8_intr,             cpld,    QSFP28_P8_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p9_intr,             cpld,    QSFP28_P9_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p10_intr,            cpld,    QSFP28_P10_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p11_intr,            cpld,    QSFP28_P11_INTR);

    // @ QSFP28_6_11_EFUSE_PG,
static SENSOR_DEVICE_ATTR_RO(qsfp28_p6_efuse_pg,         cpld,    QSFP28_P6_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p7_efuse_pg,         cpld,    QSFP28_P7_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p8_efuse_pg,         cpld,    QSFP28_P8_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p9_efuse_pg,         cpld,    QSFP28_P9_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p10_efuse_pg,        cpld,    QSFP28_P10_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(qsfp28_p11_efuse_pg,        cpld,    QSFP28_P11_EFUSE_PG);

static SENSOR_DEVICE_ATTR_RO(mac_intr,                   cpld,    MAC_INTR);
    // @ FAN_ABS,
static SENSOR_DEVICE_ATTR_RO(fan_0_abs,                  cpld,    FAN_0_ABS);
static SENSOR_DEVICE_ATTR_RO(fan_1_abs,                  cpld,    FAN_1_ABS);
static SENSOR_DEVICE_ATTR_RO(fan_2_abs,                  cpld,    FAN_2_ABS);
static SENSOR_DEVICE_ATTR_RO(fan_3_abs,                  cpld,    FAN_3_ABS);
static SENSOR_DEVICE_ATTR_RO(fan_4_abs,                  cpld,    FAN_4_ABS);

    // Interrupt Mask
    // @ QSFP28_6_11_MASK_ABS,
static SENSOR_DEVICE_ATTR_RW(qsfp28_p6_mask_abs,         cpld,    QSFP28_P6_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p7_mask_abs,         cpld,    QSFP28_P7_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p8_mask_abs,         cpld,    QSFP28_P8_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p9_mask_abs,         cpld,    QSFP28_P9_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p10_mask_abs,        cpld,    QSFP28_P10_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p11_mask_abs,        cpld,    QSFP28_P11_MASK_ABS);

    // @ QSFP28_6_11_MASK_INTR,
static SENSOR_DEVICE_ATTR_RW(qsfp28_p6_mask_intr,        cpld,    QSFP28_P6_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p7_mask_intr,        cpld,    QSFP28_P7_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p8_mask_intr,        cpld,    QSFP28_P8_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p9_mask_intr,        cpld,    QSFP28_P9_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p10_mask_intr,       cpld,    QSFP28_P10_MASK_INTR);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p11_mask_intr,       cpld,    QSFP28_P11_MASK_INTR);

    // @ QSFP28_6_11_MASK_EFUSE_PG,
static SENSOR_DEVICE_ATTR_RW(qsfp28_p6_mask_efuse_pg,    cpld,    QSFP28_P6_MASK_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p7_mask_efuse_pg,    cpld,    QSFP28_P7_MASK_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p8_mask_efuse_pg,    cpld,    QSFP28_P8_MASK_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p9_mask_efuse_pg,    cpld,    QSFP28_P9_MASK_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p10_mask_efuse_pg,   cpld,    QSFP28_P10_MASK_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p11_mask_efuse_pg,   cpld,    QSFP28_P11_MASK_EFUSE_PG);

static SENSOR_DEVICE_ATTR_RW(mac_mask_intr,              cpld,    MAC_MASK_INTR);

static SENSOR_DEVICE_ATTR_RW(fan_0_mask_abs,                  cpld,    FAN_0_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(fan_1_mask_abs,                  cpld,    FAN_1_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(fan_2_mask_abs,                  cpld,    FAN_2_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(fan_3_mask_abs,                  cpld,    FAN_3_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(fan_4_mask_abs,                  cpld,    FAN_4_MASK_ABS);

    // Interrupt Event
static SENSOR_DEVICE_ATTR_RO(qsfp28_6_11_evt_abs,        cpld,    QSFP28_6_11_EVT_ABS);
static SENSOR_DEVICE_ATTR_RO(qsfp28_6_11_evt_intr,       cpld,    QSFP28_6_11_EVT_INTR);
static SENSOR_DEVICE_ATTR_RO(qsfp28_6_11_evt_efuse_pg,   cpld,    QSFP28_6_11_EVT_EFUSE_PG);
static SENSOR_DEVICE_ATTR_RO(mac_evt_intr,               cpld,    MAC_EVT_INTR);
static SENSOR_DEVICE_ATTR_RO(fan_evt_abs,                cpld,    FAN_EVT_ABS);

    // Reset
    // @ QSFP28_6_11_RESET,
static SENSOR_DEVICE_ATTR_RW(qsfp28_p6_reset,            cpld,    QSFP28_P6_RESET);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p7_reset,            cpld,    QSFP28_P7_RESET);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p8_reset,            cpld,    QSFP28_P8_RESET);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p9_reset,            cpld,    QSFP28_P9_RESET);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p10_reset,           cpld,    QSFP28_P10_RESET);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p11_reset,           cpld,    QSFP28_P11_RESET);

    // @ QSFP28_6_11_LPMODE,
static SENSOR_DEVICE_ATTR_RW(qsfp28_p6_lpmode,           cpld,    QSFP28_P6_LPMODE);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p7_lpmode,           cpld,    QSFP28_P7_LPMODE);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p8_lpmode,           cpld,    QSFP28_P8_LPMODE);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p9_lpmode,           cpld,    QSFP28_P9_LPMODE);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p10_lpmode,          cpld,    QSFP28_P10_LPMODE);
static SENSOR_DEVICE_ATTR_RW(qsfp28_p11_lpmode,          cpld,    QSFP28_P11_LPMODE);

static SENSOR_DEVICE_ATTR_RW(mac_reset,                  cpld,    MAC_RESET);

static SENSOR_DEVICE_ATTR_RW(usb_qspi_reset,             cpld,    USB_QSPI_RESET);
static SENSOR_DEVICE_ATTR_RO(mac_rov,                    cpld,    MAC_ROV);
static SENSOR_DEVICE_ATTR_RO(i2c_rov_mux_sel,            cpld,    I2C_ROV_MUX_SEL);



    // @ I2C_MUX_SELECT,
static SENSOR_DEVICE_ATTR_RW(i2c_cpld_mux_en,            cpld,    I2C_CPLD_MUX_EN);
static SENSOR_DEVICE_ATTR_RW(i2c0_psu_mux_sel,           cpld,    I2C0_PSU_MUX_SEL);
static SENSOR_DEVICE_ATTR_RW(i2c0_hwm_mux_sel,           cpld,    I2C0_HWM_MUX_SEL);
static SENSOR_DEVICE_ATTR_RW(i2c_io_mux_sel,             cpld,    I2C_IO_MUX_SEL);


    // FAN Control
static SENSOR_DEVICE_ATTR_RO(fan_speed_read_mode,        cpld,    FAN_SPEED_READ_MODE);
    // 0x73~0x74: FAN RPM read value
static SENSOR_DEVICE_ATTR_RO(fan_rpm_low_byte,           cpld,    FAN_RPM_LOW_BYTE);
static SENSOR_DEVICE_ATTR_RO(fan_rpm_high_byte,          cpld,    FAN_RPM_HIGH_BYTE);

    // Status Registers
static SENSOR_DEVICE_ATTR_RW(system_led_status_1,        cpld,    SYSTEM_LED_STATUS_1);




/******************************************************************************
 * CPLD 4                                                                     *
 ******************************************************************************/

    // Interrupt Status
    // @ SFP56_16_23_ABS,
static SENSOR_DEVICE_ATTR_RO(sfp56_p16_abs,              cpld,    SFP56_P16_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p17_abs,              cpld,    SFP56_P17_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p18_abs,              cpld,    SFP56_P18_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p19_abs,              cpld,    SFP56_P19_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p20_abs,              cpld,    SFP56_P20_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p21_abs,              cpld,    SFP56_P21_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p22_abs,              cpld,    SFP56_P22_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p23_abs,              cpld,    SFP56_P23_ABS);
    // @ SFP56_24_31_ABS,
static SENSOR_DEVICE_ATTR_RO(sfp56_p24_abs,              cpld,    SFP56_P24_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p25_abs,              cpld,    SFP56_P25_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p26_abs,              cpld,    SFP56_P26_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p27_abs,              cpld,    SFP56_P27_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p28_abs,              cpld,    SFP56_P28_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p29_abs,              cpld,    SFP56_P29_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p30_abs,              cpld,    SFP56_P30_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p31_abs,              cpld,    SFP56_P31_ABS);
    // @ SFP56_32_39_ABS,
static SENSOR_DEVICE_ATTR_RO(sfp56_p32_abs,              cpld,    SFP56_P32_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p33_abs,              cpld,    SFP56_P33_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p34_abs,              cpld,    SFP56_P34_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p35_abs,              cpld,    SFP56_P35_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p36_abs,              cpld,    SFP56_P36_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p37_abs,              cpld,    SFP56_P37_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p38_abs,              cpld,    SFP56_P38_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_p39_abs,              cpld,    SFP56_P39_ABS);

    // @ SFP56_16_23_RX_LOS,
static SENSOR_DEVICE_ATTR_RW(sfp56_p16_rx_los,           cpld,    SFP56_P16_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p17_rx_los,           cpld,    SFP56_P17_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p18_rx_los,           cpld,    SFP56_P18_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p19_rx_los,           cpld,    SFP56_P19_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p20_rx_los,           cpld,    SFP56_P20_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p21_rx_los,           cpld,    SFP56_P21_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p22_rx_los,           cpld,    SFP56_P22_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p23_rx_los,           cpld,    SFP56_P23_RX_LOS);
    // @ SFP56_24_31_RX_LOS,
static SENSOR_DEVICE_ATTR_RW(sfp56_p24_rx_los,           cpld,    SFP56_P24_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p25_rx_los,           cpld,    SFP56_P25_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p26_rx_los,           cpld,    SFP56_P26_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p27_rx_los,           cpld,    SFP56_P27_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p28_rx_los,           cpld,    SFP56_P28_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p29_rx_los,           cpld,    SFP56_P29_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p30_rx_los,           cpld,    SFP56_P30_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p31_rx_los,           cpld,    SFP56_P31_RX_LOS);
    // @ SFP56_32_39_RX_LOS,
static SENSOR_DEVICE_ATTR_RW(sfp56_p32_rx_los,           cpld,    SFP56_P32_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p33_rx_los,           cpld,    SFP56_P33_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p34_rx_los,           cpld,    SFP56_P34_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p35_rx_los,           cpld,    SFP56_P35_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p36_rx_los,           cpld,    SFP56_P36_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p37_rx_los,           cpld,    SFP56_P37_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p38_rx_los,           cpld,    SFP56_P38_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p39_rx_los,           cpld,    SFP56_P39_RX_LOS);

    // @ SFP56_16_23_TX_FAULT,
static SENSOR_DEVICE_ATTR_RW(sfp56_p16_tx_fault,         cpld,    SFP56_P16_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p17_tx_fault,         cpld,    SFP56_P17_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p18_tx_fault,         cpld,    SFP56_P18_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p19_tx_fault,         cpld,    SFP56_P19_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p20_tx_fault,         cpld,    SFP56_P20_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p21_tx_fault,         cpld,    SFP56_P21_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p22_tx_fault,         cpld,    SFP56_P22_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p23_tx_fault,         cpld,    SFP56_P23_TX_FAULT);
    // @ SFP56_24_31_TX_FAULT,
static SENSOR_DEVICE_ATTR_RW(sfp56_p24_tx_fault,         cpld,    SFP56_P24_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p25_tx_fault,         cpld,    SFP56_P25_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p26_tx_fault,         cpld,    SFP56_P26_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p27_tx_fault,         cpld,    SFP56_P27_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p28_tx_fault,         cpld,    SFP56_P28_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p29_tx_fault,         cpld,    SFP56_P29_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p30_tx_fault,         cpld,    SFP56_P30_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p31_tx_fault,         cpld,    SFP56_P31_TX_FAULT);
    // @ SFP56_32_39_TX_FAULT,
static SENSOR_DEVICE_ATTR_RW(sfp56_p32_tx_fault,         cpld,    SFP56_P32_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p33_tx_fault,         cpld,    SFP56_P33_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p34_tx_fault,         cpld,    SFP56_P34_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p35_tx_fault,         cpld,    SFP56_P35_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p36_tx_fault,         cpld,    SFP56_P36_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p37_tx_fault,         cpld,    SFP56_P37_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p38_tx_fault,         cpld,    SFP56_P38_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p39_tx_fault,         cpld,    SFP56_P39_TX_FAULT);

    // Interrupt Mask
    // @ SFP56_16_23_MASK_ABS,
static SENSOR_DEVICE_ATTR_RW(sfp56_p16_mask_abs,         cpld,    SFP56_P16_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p17_mask_abs,         cpld,    SFP56_P17_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p18_mask_abs,         cpld,    SFP56_P18_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p19_mask_abs,         cpld,    SFP56_P19_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p20_mask_abs,         cpld,    SFP56_P20_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p21_mask_abs,         cpld,    SFP56_P21_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p22_mask_abs,         cpld,    SFP56_P22_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p23_mask_abs,         cpld,    SFP56_P23_MASK_ABS);
    // @ SFP56_24_31_MASK_ABS,
static SENSOR_DEVICE_ATTR_RW(sfp56_p24_mask_abs,         cpld,    SFP56_P24_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p25_mask_abs,         cpld,    SFP56_P25_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p26_mask_abs,         cpld,    SFP56_P26_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p27_mask_abs,         cpld,    SFP56_P27_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p28_mask_abs,         cpld,    SFP56_P28_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p29_mask_abs,         cpld,    SFP56_P29_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p30_mask_abs,         cpld,    SFP56_P30_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p31_mask_abs,         cpld,    SFP56_P31_MASK_ABS);
    // @ SFP56_32_39_MASK_ABS,
static SENSOR_DEVICE_ATTR_RW(sfp56_p32_mask_abs,         cpld,    SFP56_P32_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p33_mask_abs,         cpld,    SFP56_P33_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p34_mask_abs,         cpld,    SFP56_P34_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p35_mask_abs,         cpld,    SFP56_P35_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p36_mask_abs,         cpld,    SFP56_P36_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p37_mask_abs,         cpld,    SFP56_P37_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p38_mask_abs,         cpld,    SFP56_P38_MASK_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p39_mask_abs,         cpld,    SFP56_P39_MASK_ABS);

    // @ SFP56_16_23_MASK_RX_LOS,
static SENSOR_DEVICE_ATTR_RW(sfp56_p16_mask_rx_los,      cpld,    SFP56_P16_MASK_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p17_mask_rx_los,      cpld,    SFP56_P17_MASK_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p18_mask_rx_los,      cpld,    SFP56_P18_MASK_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p19_mask_rx_los,      cpld,    SFP56_P19_MASK_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p20_mask_rx_los,      cpld,    SFP56_P20_MASK_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p21_mask_rx_los,      cpld,    SFP56_P21_MASK_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p22_mask_rx_los,      cpld,    SFP56_P22_MASK_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p23_mask_rx_los,      cpld,    SFP56_P23_MASK_RX_LOS);
    // @ SFP56_24_31_MASK_RX_LOS,
static SENSOR_DEVICE_ATTR_RW(sfp56_p24_mask_rx_los,      cpld,    SFP56_P24_MASK_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p25_mask_rx_los,      cpld,    SFP56_P25_MASK_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p26_mask_rx_los,      cpld,    SFP56_P26_MASK_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p27_mask_rx_los,      cpld,    SFP56_P27_MASK_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p28_mask_rx_los,      cpld,    SFP56_P28_MASK_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p29_mask_rx_los,      cpld,    SFP56_P29_MASK_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p30_mask_rx_los,      cpld,    SFP56_P30_MASK_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p31_mask_rx_los,      cpld,    SFP56_P31_MASK_RX_LOS);
    // @ SFP56_32_39_MASK_RX_LOS,
static SENSOR_DEVICE_ATTR_RW(sfp56_p32_mask_rx_los,      cpld,    SFP56_P32_MASK_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p33_mask_rx_los,      cpld,    SFP56_P33_MASK_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p34_mask_rx_los,      cpld,    SFP56_P34_MASK_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p35_mask_rx_los,      cpld,    SFP56_P35_MASK_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p36_mask_rx_los,      cpld,    SFP56_P36_MASK_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p37_mask_rx_los,      cpld,    SFP56_P37_MASK_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p38_mask_rx_los,      cpld,    SFP56_P38_MASK_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_p39_mask_rx_los,      cpld,    SFP56_P39_MASK_RX_LOS);

    // @ SFP56_16_23_MASK_TX_FAULT,
static SENSOR_DEVICE_ATTR_RW(sfp56_p16_mask_tx_fault,    cpld,    SFP56_P16_MASK_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p17_mask_tx_fault,    cpld,    SFP56_P17_MASK_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p18_mask_tx_fault,    cpld,    SFP56_P18_MASK_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p19_mask_tx_fault,    cpld,    SFP56_P19_MASK_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p20_mask_tx_fault,    cpld,    SFP56_P20_MASK_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p21_mask_tx_fault,    cpld,    SFP56_P21_MASK_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p22_mask_tx_fault,    cpld,    SFP56_P22_MASK_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p23_mask_tx_fault,    cpld,    SFP56_P23_MASK_TX_FAULT);
    // @ SFP56_24_31_MASK_TX_FAULT,
static SENSOR_DEVICE_ATTR_RW(sfp56_p24_mask_tx_fault,    cpld,    SFP56_P24_MASK_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p25_mask_tx_fault,    cpld,    SFP56_P25_MASK_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p26_mask_tx_fault,    cpld,    SFP56_P26_MASK_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p27_mask_tx_fault,    cpld,    SFP56_P27_MASK_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p28_mask_tx_fault,    cpld,    SFP56_P28_MASK_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p29_mask_tx_fault,    cpld,    SFP56_P29_MASK_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p30_mask_tx_fault,    cpld,    SFP56_P30_MASK_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p31_mask_tx_fault,    cpld,    SFP56_P31_MASK_TX_FAULT);
    // @ SFP56_32_39_MASK_TX_FAULT,
static SENSOR_DEVICE_ATTR_RW(sfp56_p32_mask_tx_fault,    cpld,    SFP56_P32_MASK_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p33_mask_tx_fault,    cpld,    SFP56_P33_MASK_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p34_mask_tx_fault,    cpld,    SFP56_P34_MASK_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p35_mask_tx_fault,    cpld,    SFP56_P35_MASK_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p36_mask_tx_fault,    cpld,    SFP56_P36_MASK_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p37_mask_tx_fault,    cpld,    SFP56_P37_MASK_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p38_mask_tx_fault,    cpld,    SFP56_P38_MASK_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_p39_mask_tx_fault,    cpld,    SFP56_P39_MASK_TX_FAULT);

    // Interrupt Event
static SENSOR_DEVICE_ATTR_RO(sfp56_16_23_evt_abs,        cpld,    SFP56_16_23_EVT_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_24_31_evt_abs,        cpld,    SFP56_24_31_EVT_ABS);
static SENSOR_DEVICE_ATTR_RO(sfp56_32_39_evt_abs,        cpld,    SFP56_32_39_EVT_ABS);
static SENSOR_DEVICE_ATTR_RW(sfp56_16_23_evt_rx_los,     cpld,    SFP56_16_23_EVT_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_24_31_evt_rx_los,     cpld,    SFP56_24_31_EVT_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_32_39_evt_rx_los,     cpld,    SFP56_32_39_EVT_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(sfp56_16_23_evt_tx_fault,   cpld,    SFP56_16_23_EVT_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_24_31_evt_tx_fault,   cpld,    SFP56_24_31_EVT_TX_FAULT);
static SENSOR_DEVICE_ATTR_RW(sfp56_32_39_evt_tx_fault,   cpld,    SFP56_32_39_EVT_TX_FAULT);

    // SFP Control
    // @ SFP56_16_23_TX_DISABLE,
static SENSOR_DEVICE_ATTR_RW(sfp56_p16_tx_disable,       cpld,    SFP56_P16_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(sfp56_p17_tx_disable,       cpld,    SFP56_P17_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(sfp56_p18_tx_disable,       cpld,    SFP56_P18_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(sfp56_p19_tx_disable,       cpld,    SFP56_P19_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(sfp56_p20_tx_disable,       cpld,    SFP56_P20_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(sfp56_p21_tx_disable,       cpld,    SFP56_P21_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(sfp56_p22_tx_disable,       cpld,    SFP56_P22_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(sfp56_p23_tx_disable,       cpld,    SFP56_P23_TX_DISABLE);
    // @ SFP56_24_31_TX_DISABLE,
static SENSOR_DEVICE_ATTR_RW(sfp56_p24_tx_disable,       cpld,    SFP56_P24_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(sfp56_p25_tx_disable,       cpld,    SFP56_P25_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(sfp56_p26_tx_disable,       cpld,    SFP56_P26_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(sfp56_p27_tx_disable,       cpld,    SFP56_P27_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(sfp56_p28_tx_disable,       cpld,    SFP56_P28_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(sfp56_p29_tx_disable,       cpld,    SFP56_P29_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(sfp56_p30_tx_disable,       cpld,    SFP56_P30_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(sfp56_p31_tx_disable,       cpld,    SFP56_P31_TX_DISABLE);
    // @ SFP56_32_39_TX_DISABLE,
static SENSOR_DEVICE_ATTR_RW(sfp56_p32_tx_disable,       cpld,    SFP56_P32_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(sfp56_p33_tx_disable,       cpld,    SFP56_P33_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(sfp56_p34_tx_disable,       cpld,    SFP56_P34_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(sfp56_p35_tx_disable,       cpld,    SFP56_P35_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(sfp56_p36_tx_disable,       cpld,    SFP56_P36_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(sfp56_p37_tx_disable,       cpld,    SFP56_P37_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(sfp56_p38_tx_disable,       cpld,    SFP56_P38_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(sfp56_p39_tx_disable,       cpld,    SFP56_P39_TX_DISABLE);

    // @ SFP56_16_23_RATE_SEL,
static SENSOR_DEVICE_ATTR_RW(sfp56_p16_rate_sel,         cpld,    SFP56_P16_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(sfp56_p17_rate_sel,         cpld,    SFP56_P17_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(sfp56_p18_rate_sel,         cpld,    SFP56_P18_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(sfp56_p19_rate_sel,         cpld,    SFP56_P19_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(sfp56_p20_rate_sel,         cpld,    SFP56_P20_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(sfp56_p21_rate_sel,         cpld,    SFP56_P21_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(sfp56_p22_rate_sel,         cpld,    SFP56_P22_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(sfp56_p23_rate_sel,         cpld,    SFP56_P23_RATE_SEL);
    // @ SFP56_24_31_RATE_SEL,
static SENSOR_DEVICE_ATTR_RW(sfp56_p24_rate_sel,         cpld,    SFP56_P24_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(sfp56_p25_rate_sel,         cpld,    SFP56_P25_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(sfp56_p26_rate_sel,         cpld,    SFP56_P26_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(sfp56_p27_rate_sel,         cpld,    SFP56_P27_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(sfp56_p28_rate_sel,         cpld,    SFP56_P28_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(sfp56_p29_rate_sel,         cpld,    SFP56_P29_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(sfp56_p30_rate_sel,         cpld,    SFP56_P30_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(sfp56_p31_rate_sel,         cpld,    SFP56_P31_RATE_SEL);
    // @ SFP56_32_39_RATE_SEL,
static SENSOR_DEVICE_ATTR_RW(sfp56_p32_rate_sel,         cpld,    SFP56_P32_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(sfp56_p33_rate_sel,         cpld,    SFP56_P33_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(sfp56_p34_rate_sel,         cpld,    SFP56_P34_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(sfp56_p35_rate_sel,         cpld,    SFP56_P35_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(sfp56_p36_rate_sel,         cpld,    SFP56_P36_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(sfp56_p37_rate_sel,         cpld,    SFP56_P37_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(sfp56_p38_rate_sel,         cpld,    SFP56_P38_RATE_SEL);
static SENSOR_DEVICE_ATTR_RW(sfp56_p39_rate_sel,         cpld,    SFP56_P39_RATE_SEL);


/******************************************************************************
 * BSP DEBUG                                                                    *
 ******************************************************************************/
//BSP DEBUG
static SENSOR_DEVICE_ATTR_RW(bsp_debug,                  bsp_callback,     BSP_DEBUG);


/* define support attributes of cpldx */
static struct attribute *cpld1_attributes[] = {
    // cpld common
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(event_detect_ctrl),
    _DEVICE_ATTR(cpld_chip_type),

    // cpld1
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
    _DEVICE_ATTR(cpu_nmi_intr),
    _DEVICE_ATTR(out_status_intr),
    _DEVICE_ATTR(clk_ptp_mask_intr),
    _DEVICE_ATTR(phy_mask_intr),
    _DEVICE_ATTR(top_brd_cpld_fru_mask_intr),
    _DEVICE_ATTR(psu0_mask_intr),
    _DEVICE_ATTR(psu1_mask_intr),
    _DEVICE_ATTR(cpld2_mask_intr),
    _DEVICE_ATTR(cpld2_io_mask_intr),
    _DEVICE_ATTR(main_brd_cpld_mask_intr),
    _DEVICE_ATTR(cpld3_mask_intr),
    _DEVICE_ATTR(cpld4_mask_intr),
    _DEVICE_ATTR(mb_eth_mask_intr),
    _DEVICE_ATTR(mb_ptp_mask_intr),
    _DEVICE_ATTR(fan_mask_intr),
    _DEVICE_ATTR(thermal_mask_intr),
    _DEVICE_ATTR(cpu_nmi_mask_intr),
    _DEVICE_ATTR(out_status_mask_intr),
    _DEVICE_ATTR(clk_ptp_evt_intr),
    _DEVICE_ATTR(phy_evt_intr),
    _DEVICE_ATTR(top_brd_cpld_fru_evt_intr),
    _DEVICE_ATTR(main_brd_cpld_evt_intr),
    _DEVICE_ATTR(thermal_evt_intr),
    _DEVICE_ATTR(cpu_nmi_evt_intr),
    _DEVICE_ATTR(out_status_evt_intr),
    _DEVICE_ATTR(btn_fp_reset),
    _DEVICE_ATTR(spi_bios_reset),
    _DEVICE_ATTR(rgb_0_reset),
    _DEVICE_ATTR(rgb_1_reset),
    _DEVICE_ATTR(bmc_lpc_reset),
    _DEVICE_ATTR(bmc_pcie_reset),
    _DEVICE_ATTR(cpld_to_bmc_sys_reset),
    _DEVICE_ATTR(cpld_to_cpu_reset),
    _DEVICE_ATTR(usb_pwr_en),
    _DEVICE_ATTR(usb_sie_reset),
    _DEVICE_ATTR(i2c_mux_sys_reset),
    _DEVICE_ATTR(i2c_mux_smbus_reset),
    _DEVICE_ATTR(i2c_mux_qsfp28_reset),
    _DEVICE_ATTR(i2c_mux_sfp_reset),
    _DEVICE_ATTR(mb_i2c_reset),
    _DEVICE_ATTR(bmc_abs),
    _DEVICE_ATTR(sata_ssd1_abs),
    _DEVICE_ATTR(sata_ssd2_abs),
    _DEVICE_ATTR(psu0_abs),
    _DEVICE_ATTR(psu1_abs),
    _DEVICE_ATTR(psu0_vin_pg),
    _DEVICE_ATTR(psu1_vin_pg),
    _DEVICE_ATTR(psu0_vout_pg),
    _DEVICE_ATTR(psu1_vout_pg),
    _DEVICE_ATTR(psu_status),
    _DEVICE_ATTR(cpu_boot_done),
    _DEVICE_ATTR(cpu_pg),
    _DEVICE_ATTR(wdt_cpu_to_cpld),
    _DEVICE_ATTR(bmc_wdt_1_reset),
    _DEVICE_ATTR(bmc_wdt_2_reset),
    _DEVICE_ATTR(smbus_peci_dis),
    _DEVICE_ATTR(i2c_psu0_mux_sel),
    _DEVICE_ATTR(i2c_psu1_mux_sel),
    _DEVICE_ATTR(i2c_cpld_mux_sel),
    _DEVICE_ATTR(bmc_usb_mux_sel),
    _DEVICE_ATTR(uart_cpu_bmc_mux_sel),
    _DEVICE_ATTR(uart_mux_sel),
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
    _DEVICE_ATTR(qsfp28_p0_pwr_en),
    _DEVICE_ATTR(qsfp28_p1_pwr_en),
    _DEVICE_ATTR(qsfp28_p2_pwr_en),
    _DEVICE_ATTR(qsfp28_p3_pwr_en),
    _DEVICE_ATTR(qsfp28_p4_pwr_en),
    _DEVICE_ATTR(qsfp28_p5_pwr_en),
    _DEVICE_ATTR(qsfp28_p6_pwr_en),
    _DEVICE_ATTR(qsfp28_p7_pwr_en),
    _DEVICE_ATTR(qsfp28_p8_pwr_en),
    _DEVICE_ATTR(qsfp28_p9_pwr_en),
    _DEVICE_ATTR(qsfp28_p10_pwr_en),
    _DEVICE_ATTR(qsfp28_p11_pwr_en),
    _DEVICE_ATTR(qsfpdd_p12_pwr_en),
    _DEVICE_ATTR(qsfpdd_p13_pwr_en),
    _DEVICE_ATTR(qsfpdd_p14_pwr_en),
    _DEVICE_ATTR(qsfpdd_p15_pwr_en),
    _DEVICE_ATTR(sfp56_p16_pwr_en),
    _DEVICE_ATTR(sfp56_p17_pwr_en),
    _DEVICE_ATTR(sfp56_p18_pwr_en),
    _DEVICE_ATTR(sfp56_p19_pwr_en),
    _DEVICE_ATTR(sfp56_p20_pwr_en),
    _DEVICE_ATTR(sfp56_p21_pwr_en),
    _DEVICE_ATTR(sfp56_p22_pwr_en),
    _DEVICE_ATTR(sfp56_p23_pwr_en),
    _DEVICE_ATTR(sfp56_p24_pwr_en),
    _DEVICE_ATTR(sfp56_p25_pwr_en),
    _DEVICE_ATTR(sfp56_p26_pwr_en),
    _DEVICE_ATTR(sfp56_p27_pwr_en),
    _DEVICE_ATTR(sfp56_p28_pwr_en),
    _DEVICE_ATTR(sfp56_p29_pwr_en),
    _DEVICE_ATTR(sfp56_p30_pwr_en),
    _DEVICE_ATTR(sfp56_p31_pwr_en),
    _DEVICE_ATTR(sfp56_p32_pwr_en),
    _DEVICE_ATTR(sfp56_p33_pwr_en),
    _DEVICE_ATTR(sfp56_p34_pwr_en),
    _DEVICE_ATTR(sfp56_p35_pwr_en),
    _DEVICE_ATTR(sfp56_p36_pwr_en),
    _DEVICE_ATTR(sfp56_p37_pwr_en),
    _DEVICE_ATTR(sfp56_p38_pwr_en),
    _DEVICE_ATTR(sfp56_p39_pwr_en),
    _DEVICE_ATTR(clk_ptp_reset),
    _DEVICE_ATTR(cjap_reset),
    _DEVICE_ATTR(ntm_reset),
    _DEVICE_ATTR(gnss_reset),
    _DEVICE_ATTR(bits_reset),
    _DEVICE_ATTR(clk_timing_ctrl),
    _DEVICE_ATTR(gnss_status),
    _DEVICE_ATTR(timing_status),
    _DEVICE_ATTR(qsfpdd_sel),
    _DEVICE_ATTR(qsfp28_sel),
    _DEVICE_ATTR(i2c_mux_0x76_reset),
    _DEVICE_ATTR(i2c_mux_0x75_reset),
    _DEVICE_ATTR(i2c_mux_6_11_reset),
    _DEVICE_ATTR(i2c_mux_sfp56_16_23_reset),
    _DEVICE_ATTR(i2c_mux_sfp56_24_31_reset),
    _DEVICE_ATTR(i2c_mux_sfp56_32_39_reset),
    _DEVICE_ATTR(i2c_mux_0x71_reset),
    NULL,
};

static struct attribute *cpld2_attributes[] = {
    // cpld common
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(event_detect_ctrl),
    _DEVICE_ATTR(cpld_chip_type),
    _DEVICE_ATTR(module_reset),
    // cpld2
    _DEVICE_ATTR(qsfp28_p0_abs),
    _DEVICE_ATTR(qsfp28_p1_abs),
    _DEVICE_ATTR(qsfp28_p2_abs),
    _DEVICE_ATTR(qsfp28_p3_abs),
    _DEVICE_ATTR(qsfp28_p4_abs),
    _DEVICE_ATTR(qsfp28_p5_abs),
    _DEVICE_ATTR(qsfpdd_p12_abs),
    _DEVICE_ATTR(qsfpdd_p13_abs),
    _DEVICE_ATTR(qsfpdd_p14_abs),
    _DEVICE_ATTR(qsfpdd_p15_abs),
    _DEVICE_ATTR(qsfp28_p0_intr),
    _DEVICE_ATTR(qsfp28_p1_intr),
    _DEVICE_ATTR(qsfp28_p2_intr),
    _DEVICE_ATTR(qsfp28_p3_intr),
    _DEVICE_ATTR(qsfp28_p4_intr),
    _DEVICE_ATTR(qsfp28_p5_intr),
    _DEVICE_ATTR(qsfpdd_p12_intr),
    _DEVICE_ATTR(qsfpdd_p13_intr),
    _DEVICE_ATTR(qsfpdd_p14_intr),
    _DEVICE_ATTR(qsfpdd_p15_intr),
    _DEVICE_ATTR(qsfp28_p0_efuse_pg),
    _DEVICE_ATTR(qsfp28_p1_efuse_pg),
    _DEVICE_ATTR(qsfp28_p2_efuse_pg),
    _DEVICE_ATTR(qsfp28_p3_efuse_pg),
    _DEVICE_ATTR(qsfp28_p4_efuse_pg),
    _DEVICE_ATTR(qsfp28_p5_efuse_pg),
    _DEVICE_ATTR(qsfpdd_p12_efuse_pg),
    _DEVICE_ATTR(qsfpdd_p13_efuse_pg),
    _DEVICE_ATTR(qsfpdd_p14_efuse_pg),
    _DEVICE_ATTR(qsfpdd_p15_efuse_pg),
    _DEVICE_ATTR(qsfp28_p0_mask_abs),
    _DEVICE_ATTR(qsfp28_p1_mask_abs),
    _DEVICE_ATTR(qsfp28_p2_mask_abs),
    _DEVICE_ATTR(qsfp28_p3_mask_abs),
    _DEVICE_ATTR(qsfp28_p4_mask_abs),
    _DEVICE_ATTR(qsfp28_p5_mask_abs),
    _DEVICE_ATTR(qsfpdd_p12_mask_abs),
    _DEVICE_ATTR(qsfpdd_p13_mask_abs),
    _DEVICE_ATTR(qsfpdd_p14_mask_abs),
    _DEVICE_ATTR(qsfpdd_p15_mask_abs),
    _DEVICE_ATTR(qsfp28_p0_mask_intr),
    _DEVICE_ATTR(qsfp28_p1_mask_intr),
    _DEVICE_ATTR(qsfp28_p2_mask_intr),
    _DEVICE_ATTR(qsfp28_p3_mask_intr),
    _DEVICE_ATTR(qsfp28_p4_mask_intr),
    _DEVICE_ATTR(qsfp28_p5_mask_intr),
    _DEVICE_ATTR(qsfpdd_p12_mask_intr),
    _DEVICE_ATTR(qsfpdd_p13_mask_intr),
    _DEVICE_ATTR(qsfpdd_p14_mask_intr),
    _DEVICE_ATTR(qsfpdd_p15_mask_intr),
    _DEVICE_ATTR(qsfp28_p0_mask_efuse_pg),
    _DEVICE_ATTR(qsfp28_p1_mask_efuse_pg),
    _DEVICE_ATTR(qsfp28_p2_mask_efuse_pg),
    _DEVICE_ATTR(qsfp28_p3_mask_efuse_pg),
    _DEVICE_ATTR(qsfp28_p4_mask_efuse_pg),
    _DEVICE_ATTR(qsfp28_p5_mask_efuse_pg),
    _DEVICE_ATTR(qsfpdd_p12_mask_efuse_pg),
    _DEVICE_ATTR(qsfpdd_p13_mask_efuse_pg),
    _DEVICE_ATTR(qsfpdd_p14_mask_efuse_pg),
    _DEVICE_ATTR(qsfpdd_p15_mask_efuse_pg),
    _DEVICE_ATTR(qsfp28_0_5_evt_abs),
    _DEVICE_ATTR(qsfpdd_12_15_evt_abs),
    _DEVICE_ATTR(qsfp28_0_5_evt_intr),
    _DEVICE_ATTR(qsfpdd_12_15_evt_intr),
    _DEVICE_ATTR(qsfp28_0_5_evt_efuse_pg),
    _DEVICE_ATTR(qsfpdd_12_15_evt_efuse_pg),
    _DEVICE_ATTR(qsfp28_p0_reset),
    _DEVICE_ATTR(qsfp28_p1_reset),
    _DEVICE_ATTR(qsfp28_p2_reset),
    _DEVICE_ATTR(qsfp28_p3_reset),
    _DEVICE_ATTR(qsfp28_p4_reset),
    _DEVICE_ATTR(qsfp28_p5_reset),
    _DEVICE_ATTR(qsfpdd_p12_reset),
    _DEVICE_ATTR(qsfpdd_p13_reset),
    _DEVICE_ATTR(qsfpdd_p14_reset),
    _DEVICE_ATTR(qsfpdd_p15_reset),
    _DEVICE_ATTR(qsfp28_p0_lpmode),
    _DEVICE_ATTR(qsfp28_p1_lpmode),
    _DEVICE_ATTR(qsfp28_p2_lpmode),
    _DEVICE_ATTR(qsfp28_p3_lpmode),
    _DEVICE_ATTR(qsfp28_p4_lpmode),
    _DEVICE_ATTR(qsfp28_p5_lpmode),
    _DEVICE_ATTR(qsfpdd_p12_lpmode),
    _DEVICE_ATTR(qsfpdd_p13_lpmode),
    _DEVICE_ATTR(qsfpdd_p14_lpmode),
    _DEVICE_ATTR(qsfpdd_p15_lpmode),
    _DEVICE_ATTR(clk_en_ctrl),
    NULL,
};

static struct attribute *cpld3_attributes[] = {
    // cpld common
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(event_detect_ctrl),
    _DEVICE_ATTR(cpld_chip_type),
    _DEVICE_ATTR(module_reset),
    // cpld3
    _DEVICE_ATTR(gnss_model_id),
    _DEVICE_ATTR(ocxo_id),
    _DEVICE_ATTR(qsfp28_p6_abs),
    _DEVICE_ATTR(qsfp28_p7_abs),
    _DEVICE_ATTR(qsfp28_p8_abs),
    _DEVICE_ATTR(qsfp28_p9_abs),
    _DEVICE_ATTR(qsfp28_p10_abs),
    _DEVICE_ATTR(qsfp28_p11_abs),
    _DEVICE_ATTR(qsfp28_p6_intr),
    _DEVICE_ATTR(qsfp28_p7_intr),
    _DEVICE_ATTR(qsfp28_p8_intr),
    _DEVICE_ATTR(qsfp28_p9_intr),
    _DEVICE_ATTR(qsfp28_p10_intr),
    _DEVICE_ATTR(qsfp28_p11_intr),
    _DEVICE_ATTR(qsfp28_p6_efuse_pg),
    _DEVICE_ATTR(qsfp28_p7_efuse_pg),
    _DEVICE_ATTR(qsfp28_p8_efuse_pg),
    _DEVICE_ATTR(qsfp28_p9_efuse_pg),
    _DEVICE_ATTR(qsfp28_p10_efuse_pg),
    _DEVICE_ATTR(qsfp28_p11_efuse_pg),
    _DEVICE_ATTR(mac_intr),
    _DEVICE_ATTR(fan_0_abs),
    _DEVICE_ATTR(fan_1_abs),
    _DEVICE_ATTR(fan_2_abs),
    _DEVICE_ATTR(fan_3_abs),
    _DEVICE_ATTR(fan_4_abs),
    _DEVICE_ATTR(thermal_intr),
    _DEVICE_ATTR(qsfp28_p6_mask_abs),
    _DEVICE_ATTR(qsfp28_p7_mask_abs),
    _DEVICE_ATTR(qsfp28_p8_mask_abs),
    _DEVICE_ATTR(qsfp28_p9_mask_abs),
    _DEVICE_ATTR(qsfp28_p10_mask_abs),
    _DEVICE_ATTR(qsfp28_p11_mask_abs),
    _DEVICE_ATTR(qsfp28_p6_mask_intr),
    _DEVICE_ATTR(qsfp28_p7_mask_intr),
    _DEVICE_ATTR(qsfp28_p8_mask_intr),
    _DEVICE_ATTR(qsfp28_p9_mask_intr),
    _DEVICE_ATTR(qsfp28_p10_mask_intr),
    _DEVICE_ATTR(qsfp28_p11_mask_intr),
    _DEVICE_ATTR(qsfp28_p6_mask_efuse_pg),
    _DEVICE_ATTR(qsfp28_p7_mask_efuse_pg),
    _DEVICE_ATTR(qsfp28_p8_mask_efuse_pg),
    _DEVICE_ATTR(qsfp28_p9_mask_efuse_pg),
    _DEVICE_ATTR(qsfp28_p10_mask_efuse_pg),
    _DEVICE_ATTR(qsfp28_p11_mask_efuse_pg),
    _DEVICE_ATTR(mac_mask_intr),
    _DEVICE_ATTR(fan_0_mask_abs),
    _DEVICE_ATTR(fan_1_mask_abs),
    _DEVICE_ATTR(fan_2_mask_abs),
    _DEVICE_ATTR(fan_3_mask_abs),
    _DEVICE_ATTR(fan_4_mask_abs),
    _DEVICE_ATTR(thermal_mask_intr),
    _DEVICE_ATTR(qsfp28_6_11_evt_abs),
    _DEVICE_ATTR(qsfp28_6_11_evt_intr),
    _DEVICE_ATTR(qsfp28_6_11_evt_efuse_pg),
    _DEVICE_ATTR(mac_evt_intr),
    _DEVICE_ATTR(fan_evt_abs),
    _DEVICE_ATTR(thermal_evt_intr),
    _DEVICE_ATTR(qsfp28_p6_reset),
    _DEVICE_ATTR(qsfp28_p7_reset),
    _DEVICE_ATTR(qsfp28_p8_reset),
    _DEVICE_ATTR(qsfp28_p9_reset),
    _DEVICE_ATTR(qsfp28_p10_reset),
    _DEVICE_ATTR(qsfp28_p11_reset),
    _DEVICE_ATTR(qsfp28_p6_lpmode),
    _DEVICE_ATTR(qsfp28_p7_lpmode),
    _DEVICE_ATTR(qsfp28_p8_lpmode),
    _DEVICE_ATTR(qsfp28_p9_lpmode),
    _DEVICE_ATTR(qsfp28_p10_lpmode),
    _DEVICE_ATTR(qsfp28_p11_lpmode),
    _DEVICE_ATTR(mac_reset),
    _DEVICE_ATTR(usb_qspi_reset),
    _DEVICE_ATTR(mac_rov),
    _DEVICE_ATTR(i2c_rov_mux_sel),
    _DEVICE_ATTR(smbus_peci_dis),
    _DEVICE_ATTR(i2c_cpld_mux_en),
    _DEVICE_ATTR(i2c0_psu_mux_sel),
    _DEVICE_ATTR(i2c0_hwm_mux_sel),
    _DEVICE_ATTR(i2c_io_mux_sel),
    _DEVICE_ATTR(fan_speed_read_mode),
    _DEVICE_ATTR(fan_rpm_low_byte),
    _DEVICE_ATTR(fan_rpm_high_byte),
    _DEVICE_ATTR(system_led_status_1),
    NULL,
};

static struct attribute *cpld4_attributes[] = {
    // cpld common
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(event_detect_ctrl),
    _DEVICE_ATTR(cpld_chip_type),
    _DEVICE_ATTR(module_reset),
    // cpld4
    _DEVICE_ATTR(sfp56_p16_abs),
    _DEVICE_ATTR(sfp56_p17_abs),
    _DEVICE_ATTR(sfp56_p18_abs),
    _DEVICE_ATTR(sfp56_p19_abs),
    _DEVICE_ATTR(sfp56_p20_abs),
    _DEVICE_ATTR(sfp56_p21_abs),
    _DEVICE_ATTR(sfp56_p22_abs),
    _DEVICE_ATTR(sfp56_p23_abs),
    _DEVICE_ATTR(sfp56_p24_abs),
    _DEVICE_ATTR(sfp56_p25_abs),
    _DEVICE_ATTR(sfp56_p26_abs),
    _DEVICE_ATTR(sfp56_p27_abs),
    _DEVICE_ATTR(sfp56_p28_abs),
    _DEVICE_ATTR(sfp56_p29_abs),
    _DEVICE_ATTR(sfp56_p30_abs),
    _DEVICE_ATTR(sfp56_p31_abs),
    _DEVICE_ATTR(sfp56_p32_abs),
    _DEVICE_ATTR(sfp56_p33_abs),
    _DEVICE_ATTR(sfp56_p34_abs),
    _DEVICE_ATTR(sfp56_p35_abs),
    _DEVICE_ATTR(sfp56_p36_abs),
    _DEVICE_ATTR(sfp56_p37_abs),
    _DEVICE_ATTR(sfp56_p38_abs),
    _DEVICE_ATTR(sfp56_p39_abs),
    _DEVICE_ATTR(sfp56_p16_rx_los),
    _DEVICE_ATTR(sfp56_p17_rx_los),
    _DEVICE_ATTR(sfp56_p18_rx_los),
    _DEVICE_ATTR(sfp56_p19_rx_los),
    _DEVICE_ATTR(sfp56_p20_rx_los),
    _DEVICE_ATTR(sfp56_p21_rx_los),
    _DEVICE_ATTR(sfp56_p22_rx_los),
    _DEVICE_ATTR(sfp56_p23_rx_los),
    _DEVICE_ATTR(sfp56_p24_rx_los),
    _DEVICE_ATTR(sfp56_p25_rx_los),
    _DEVICE_ATTR(sfp56_p26_rx_los),
    _DEVICE_ATTR(sfp56_p27_rx_los),
    _DEVICE_ATTR(sfp56_p28_rx_los),
    _DEVICE_ATTR(sfp56_p29_rx_los),
    _DEVICE_ATTR(sfp56_p30_rx_los),
    _DEVICE_ATTR(sfp56_p31_rx_los),
    _DEVICE_ATTR(sfp56_p32_rx_los),
    _DEVICE_ATTR(sfp56_p33_rx_los),
    _DEVICE_ATTR(sfp56_p34_rx_los),
    _DEVICE_ATTR(sfp56_p35_rx_los),
    _DEVICE_ATTR(sfp56_p36_rx_los),
    _DEVICE_ATTR(sfp56_p37_rx_los),
    _DEVICE_ATTR(sfp56_p38_rx_los),
    _DEVICE_ATTR(sfp56_p39_rx_los),
    _DEVICE_ATTR(sfp56_p16_tx_fault),
    _DEVICE_ATTR(sfp56_p17_tx_fault),
    _DEVICE_ATTR(sfp56_p18_tx_fault),
    _DEVICE_ATTR(sfp56_p19_tx_fault),
    _DEVICE_ATTR(sfp56_p20_tx_fault),
    _DEVICE_ATTR(sfp56_p21_tx_fault),
    _DEVICE_ATTR(sfp56_p22_tx_fault),
    _DEVICE_ATTR(sfp56_p23_tx_fault),
    _DEVICE_ATTR(sfp56_p24_tx_fault),
    _DEVICE_ATTR(sfp56_p25_tx_fault),
    _DEVICE_ATTR(sfp56_p26_tx_fault),
    _DEVICE_ATTR(sfp56_p27_tx_fault),
    _DEVICE_ATTR(sfp56_p28_tx_fault),
    _DEVICE_ATTR(sfp56_p29_tx_fault),
    _DEVICE_ATTR(sfp56_p30_tx_fault),
    _DEVICE_ATTR(sfp56_p31_tx_fault),
    _DEVICE_ATTR(sfp56_p32_tx_fault),
    _DEVICE_ATTR(sfp56_p33_tx_fault),
    _DEVICE_ATTR(sfp56_p34_tx_fault),
    _DEVICE_ATTR(sfp56_p35_tx_fault),
    _DEVICE_ATTR(sfp56_p36_tx_fault),
    _DEVICE_ATTR(sfp56_p37_tx_fault),
    _DEVICE_ATTR(sfp56_p38_tx_fault),
    _DEVICE_ATTR(sfp56_p39_tx_fault),
    _DEVICE_ATTR(sfp56_p16_mask_abs),
    _DEVICE_ATTR(sfp56_p17_mask_abs),
    _DEVICE_ATTR(sfp56_p18_mask_abs),
    _DEVICE_ATTR(sfp56_p19_mask_abs),
    _DEVICE_ATTR(sfp56_p20_mask_abs),
    _DEVICE_ATTR(sfp56_p21_mask_abs),
    _DEVICE_ATTR(sfp56_p22_mask_abs),
    _DEVICE_ATTR(sfp56_p23_mask_abs),
    _DEVICE_ATTR(sfp56_p24_mask_abs),
    _DEVICE_ATTR(sfp56_p25_mask_abs),
    _DEVICE_ATTR(sfp56_p26_mask_abs),
    _DEVICE_ATTR(sfp56_p27_mask_abs),
    _DEVICE_ATTR(sfp56_p28_mask_abs),
    _DEVICE_ATTR(sfp56_p29_mask_abs),
    _DEVICE_ATTR(sfp56_p30_mask_abs),
    _DEVICE_ATTR(sfp56_p31_mask_abs),
    _DEVICE_ATTR(sfp56_p32_mask_abs),
    _DEVICE_ATTR(sfp56_p33_mask_abs),
    _DEVICE_ATTR(sfp56_p34_mask_abs),
    _DEVICE_ATTR(sfp56_p35_mask_abs),
    _DEVICE_ATTR(sfp56_p36_mask_abs),
    _DEVICE_ATTR(sfp56_p37_mask_abs),
    _DEVICE_ATTR(sfp56_p38_mask_abs),
    _DEVICE_ATTR(sfp56_p39_mask_abs),
    _DEVICE_ATTR(sfp56_p16_mask_rx_los),
    _DEVICE_ATTR(sfp56_p17_mask_rx_los),
    _DEVICE_ATTR(sfp56_p18_mask_rx_los),
    _DEVICE_ATTR(sfp56_p19_mask_rx_los),
    _DEVICE_ATTR(sfp56_p20_mask_rx_los),
    _DEVICE_ATTR(sfp56_p21_mask_rx_los),
    _DEVICE_ATTR(sfp56_p22_mask_rx_los),
    _DEVICE_ATTR(sfp56_p23_mask_rx_los),
    _DEVICE_ATTR(sfp56_p24_mask_rx_los),
    _DEVICE_ATTR(sfp56_p25_mask_rx_los),
    _DEVICE_ATTR(sfp56_p26_mask_rx_los),
    _DEVICE_ATTR(sfp56_p27_mask_rx_los),
    _DEVICE_ATTR(sfp56_p28_mask_rx_los),
    _DEVICE_ATTR(sfp56_p29_mask_rx_los),
    _DEVICE_ATTR(sfp56_p30_mask_rx_los),
    _DEVICE_ATTR(sfp56_p31_mask_rx_los),
    _DEVICE_ATTR(sfp56_p32_mask_rx_los),
    _DEVICE_ATTR(sfp56_p33_mask_rx_los),
    _DEVICE_ATTR(sfp56_p34_mask_rx_los),
    _DEVICE_ATTR(sfp56_p35_mask_rx_los),
    _DEVICE_ATTR(sfp56_p36_mask_rx_los),
    _DEVICE_ATTR(sfp56_p37_mask_rx_los),
    _DEVICE_ATTR(sfp56_p38_mask_rx_los),
    _DEVICE_ATTR(sfp56_p39_mask_rx_los),
    _DEVICE_ATTR(sfp56_p16_mask_tx_fault),
    _DEVICE_ATTR(sfp56_p17_mask_tx_fault),
    _DEVICE_ATTR(sfp56_p18_mask_tx_fault),
    _DEVICE_ATTR(sfp56_p19_mask_tx_fault),
    _DEVICE_ATTR(sfp56_p20_mask_tx_fault),
    _DEVICE_ATTR(sfp56_p21_mask_tx_fault),
    _DEVICE_ATTR(sfp56_p22_mask_tx_fault),
    _DEVICE_ATTR(sfp56_p23_mask_tx_fault),
    _DEVICE_ATTR(sfp56_p24_mask_tx_fault),
    _DEVICE_ATTR(sfp56_p25_mask_tx_fault),
    _DEVICE_ATTR(sfp56_p26_mask_tx_fault),
    _DEVICE_ATTR(sfp56_p27_mask_tx_fault),
    _DEVICE_ATTR(sfp56_p28_mask_tx_fault),
    _DEVICE_ATTR(sfp56_p29_mask_tx_fault),
    _DEVICE_ATTR(sfp56_p30_mask_tx_fault),
    _DEVICE_ATTR(sfp56_p31_mask_tx_fault),
    _DEVICE_ATTR(sfp56_p32_mask_tx_fault),
    _DEVICE_ATTR(sfp56_p33_mask_tx_fault),
    _DEVICE_ATTR(sfp56_p34_mask_tx_fault),
    _DEVICE_ATTR(sfp56_p35_mask_tx_fault),
    _DEVICE_ATTR(sfp56_p36_mask_tx_fault),
    _DEVICE_ATTR(sfp56_p37_mask_tx_fault),
    _DEVICE_ATTR(sfp56_p38_mask_tx_fault),
    _DEVICE_ATTR(sfp56_p39_mask_tx_fault),
    _DEVICE_ATTR(sfp56_16_23_evt_abs),
    _DEVICE_ATTR(sfp56_24_31_evt_abs),
    _DEVICE_ATTR(sfp56_32_39_evt_abs),
    _DEVICE_ATTR(sfp56_16_23_evt_rx_los),
    _DEVICE_ATTR(sfp56_24_31_evt_rx_los),
    _DEVICE_ATTR(sfp56_32_39_evt_rx_los),
    _DEVICE_ATTR(sfp56_16_23_evt_tx_fault),
    _DEVICE_ATTR(sfp56_24_31_evt_tx_fault),
    _DEVICE_ATTR(sfp56_32_39_evt_tx_fault),
    _DEVICE_ATTR(sfp56_p16_tx_disable),
    _DEVICE_ATTR(sfp56_p17_tx_disable),
    _DEVICE_ATTR(sfp56_p18_tx_disable),
    _DEVICE_ATTR(sfp56_p19_tx_disable),
    _DEVICE_ATTR(sfp56_p20_tx_disable),
    _DEVICE_ATTR(sfp56_p21_tx_disable),
    _DEVICE_ATTR(sfp56_p22_tx_disable),
    _DEVICE_ATTR(sfp56_p23_tx_disable),
    _DEVICE_ATTR(sfp56_p24_tx_disable),
    _DEVICE_ATTR(sfp56_p25_tx_disable),
    _DEVICE_ATTR(sfp56_p26_tx_disable),
    _DEVICE_ATTR(sfp56_p27_tx_disable),
    _DEVICE_ATTR(sfp56_p28_tx_disable),
    _DEVICE_ATTR(sfp56_p29_tx_disable),
    _DEVICE_ATTR(sfp56_p30_tx_disable),
    _DEVICE_ATTR(sfp56_p31_tx_disable),
    _DEVICE_ATTR(sfp56_p32_tx_disable),
    _DEVICE_ATTR(sfp56_p33_tx_disable),
    _DEVICE_ATTR(sfp56_p34_tx_disable),
    _DEVICE_ATTR(sfp56_p35_tx_disable),
    _DEVICE_ATTR(sfp56_p36_tx_disable),
    _DEVICE_ATTR(sfp56_p37_tx_disable),
    _DEVICE_ATTR(sfp56_p38_tx_disable),
    _DEVICE_ATTR(sfp56_p39_tx_disable),
    _DEVICE_ATTR(sfp56_p16_rate_sel),
    _DEVICE_ATTR(sfp56_p17_rate_sel),
    _DEVICE_ATTR(sfp56_p18_rate_sel),
    _DEVICE_ATTR(sfp56_p19_rate_sel),
    _DEVICE_ATTR(sfp56_p20_rate_sel),
    _DEVICE_ATTR(sfp56_p21_rate_sel),
    _DEVICE_ATTR(sfp56_p22_rate_sel),
    _DEVICE_ATTR(sfp56_p23_rate_sel),
    _DEVICE_ATTR(sfp56_p24_rate_sel),
    _DEVICE_ATTR(sfp56_p25_rate_sel),
    _DEVICE_ATTR(sfp56_p26_rate_sel),
    _DEVICE_ATTR(sfp56_p27_rate_sel),
    _DEVICE_ATTR(sfp56_p28_rate_sel),
    _DEVICE_ATTR(sfp56_p29_rate_sel),
    _DEVICE_ATTR(sfp56_p30_rate_sel),
    _DEVICE_ATTR(sfp56_p31_rate_sel),
    _DEVICE_ATTR(sfp56_p32_rate_sel),
    _DEVICE_ATTR(sfp56_p33_rate_sel),
    _DEVICE_ATTR(sfp56_p34_rate_sel),
    _DEVICE_ATTR(sfp56_p35_rate_sel),
    _DEVICE_ATTR(sfp56_p36_rate_sel),
    _DEVICE_ATTR(sfp56_p37_rate_sel),
    _DEVICE_ATTR(sfp56_p38_rate_sel),
    _DEVICE_ATTR(sfp56_p39_rate_sel),
    _DEVICE_ATTR(bsp_debug),
    NULL,
};

/* cpld1 attributes group */
static const struct attribute_group cpld1_group = {
    .attrs = cpld1_attributes,
};

/* cpld2 attributes group */
static const struct attribute_group cpld2_group = {
    .attrs = cpld2_attributes,
};

/* cpld3 attributes group */
static const struct attribute_group cpld3_group = {
    .attrs = cpld3_attributes,
};

/* cpld4 attributes group */
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
static u8 _mask_shift(u8 val, u8 mask)
{
    int shift=0;

    shift = _shift(mask);

    return (val & mask) >> shift;
}

static ssize_t _parse_data(char *buf, unsigned int data, u8 data_type)
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
        // case CPLD_MGMT_PORT_0_LED_SPEED:
        // case CPLD_MGMT_PORT_1_LED_SPEED:
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
    u8 reg = 0;
    u8 mask = MASK_NONE;
    u8 data_type=DATA_UNK;
    u8 reg_val = 0;
    int ret = 0;

    switch (attr->index) {
    //CPLD_COMMON
    case CPLD_MINOR_VER:
    case CPLD_MAJOR_VER:
    case CPLD_ID:
    case CPLD_BUILD_VER:
    case CPLD_VERSION_H:
    case EVENT_DETECT_CTRL:
    case CPLD_CHIP_TYPE:
    case MODULE_RESET:
    //CPLD1
    case BRD_SKU_ID:
    case BRD_HW_ID:
    case BRD_DEPH_ID:
    case BRD_BUILD_ID:
    case BRD_ID_TYPE:
    case CPLD_BOARD_EXT_ID:
    case GDDR6_ID:
    case GDDR6_ID_FUNC:
    case CLK_PTP_INTR:
    case PHY_INTR:
    case TOP_BRD_CPLD_FRU_INTR:
    case PSU0_INTR:
    case PSU1_INTR:
    case CPLD2_INTR:
    case MAIN_BRD_CPLD_INTR:
    case CPLD3_INTR:
    case CPLD4_INTR:
    case MB_ETH_INTR:
    case FAN_INTR:
    case THERMAL_INTR:
    case CPU_NMI_INTR:
    case OUT_STATUS_INTR:
    case CLK_PTP_MASK_INTR:
    case PHY_MASK_INTR:
    case TOP_BRD_CPLD_FRU_MASK_INTR:
    case PSU0_MASK_INTR:
    case PSU1_MASK_INTR:
    case CPLD2_MASK_INTR:
    case CPLD2_IO_MASK_INTR:
    case MAIN_BRD_CPLD_MASK_INTR:
    case CPLD3_MASK_INTR:
    case CPLD4_MASK_INTR:
    case MB_ETH_MASK_INTR:
    case MB_PTP_MASK_INTR:
    case FAN_MASK_INTR:
    case THERMAL_MASK_INTR:
    case CPU_NMI_MASK_INTR:
    case OUT_STATUS_MASK_INTR:
    case CLK_PTP_EVT_INTR:
    case PHY_EVT_INTR:
    case TOP_BRD_CPLD_FRU_EVT_INTR:
    case MAIN_BRD_CPLD_EVT_INTR:
    case THERMAL_EVT_INTR:
    case CPU_NMI_EVT_INTR:
    case OUT_STATUS_EVT_INTR:
    case BTN_FP_RESET:
    case SPI_BIOS_RESET:
    case RGB_0_RESET:
    case RGB_1_RESET:
    case BMC_LPC_RESET:
    case BMC_PCIE_RESET:
    case CPLD_TO_BMC_SYS_RESET:
    case CPLD_TO_CPU_RESET:
    case USB_PWR_EN:
    case USB_SIE_RESET:
    case I2C_MUX_SYS_RESET:
    case I2C_MUX_SMBUS_RESET:
    case I2C_MUX_QSFP28_RESET:
    case I2C_MUX_SFP_RESET:
    case MB_I2C_RESET:
    case BMC_ABS:
    case SATA_SSD1_ABS:
    case SATA_SSD2_ABS:
    case PSU0_ABS:
    case PSU1_ABS:
    case PSU0_VIN_PG:
    case PSU1_VIN_PG:
    case PSU0_VOUT_PG:
    case PSU1_VOUT_PG:
    case PSU_STATUS:
    case CPU_BOOT_DONE:
    case CPU_PG:
    case WDT_CPU_TO_CPLD:
    case BMC_WDT_1_RESET:
    case BMC_WDT_2_RESET:
    case SMBUS_PECI_DIS:
    case I2C_PSU0_MUX_SEL:
    case I2C_PSU1_MUX_SEL:
    case I2C_CPLD_MUX_SEL:
    case BMC_USB_MUX_SEL:
    case UART_CPU_BMC_MUX_SEL:
    case UART_MUX_SEL:
    case EXT_CTRL:
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
    case CPLD_SYSTEM_LED_PWR:
    case PWR_LED_STATUS:
    case PWR_LED_SPEED:
    case PWR_LED_BLINK:
    case PWR_LED_ONOFF:
    case CPLD_SYSTEM_LED_GNSS:
    case GNSS_LED_STATUS:
    case GNSS_LED_SPEED:
    case GNSS_LED_BLINK:
    case GNSS_LED_ONOFF:
    case CPLD_SYSTEM_LED_SYNC:
    case SYNC_LED_STATUS:
    case SYNC_LED_SPEED:
    case SYNC_LED_BLINK:
    case SYNC_LED_ONOFF:
    case LED_CLEAR:
    case QSFP28_P0_PWR_EN ... QSFP28_P5_PWR_EN:
    case QSFP28_P6_PWR_EN ... QSFP28_P11_PWR_EN:
    case QSFPDD_P12_PWR_EN ... QSFPDD_P15_PWR_EN:
    case SFP56_P16_PWR_EN ... SFP56_P23_PWR_EN:
    case SFP56_P24_PWR_EN ... SFP56_P31_PWR_EN:
    case SFP56_P32_PWR_EN ... SFP56_P39_PWR_EN:
    case CLK_PTP_RESET:
    case CJAP_RESET:
    case NTM_RESET:
    case GNSS_RESET:
    case BITS_RESET:
    case CLK_TIMING_CTRL:
    case GNSS_STATUS:
    case TIMING_STATUS:
    case QSFPDD_SEL:
    case QSFP28_SEL:
    case I2C_MUX_0X76_RESET:
    case I2C_MUX_0X75_RESET:
    case I2C_MUX_QSFP28_6_11_RESET:
    case I2C_MUX_SFP56_16_23_RESET:
    case I2C_MUX_SFP56_24_31_RESET:
    case I2C_MUX_SFP56_32_39_RESET:
    case I2C_MUX_0X71_RESET:
    //CPLD2
    case QSFP28_P0_ABS ... QSFP28_P5_ABS:
    case QSFPDD_P12_ABS ... QSFPDD_P15_ABS:
    case QSFP28_P0_INTR ... QSFP28_P5_INTR:
    case QSFPDD_P12_INTR ... QSFPDD_P15_INTR:
    case QSFP28_P0_EFUSE_PG ... QSFP28_P5_EFUSE_PG:
    case QSFPDD_P12_EFUSE_PG ... QSFPDD_P15_EFUSE_PG:
    case QSFP28_P0_MASK_ABS ... QSFP28_P5_MASK_ABS:
    case QSFPDD_P12_MASK_ABS ... QSFPDD_P15_MASK_ABS:
    case QSFP28_P0_MASK_INTR ... QSFP28_P5_MASK_INTR:
    case QSFPDD_P12_MASK_INTR ... QSFPDD_P15_MASK_INTR:
    case QSFP28_P0_MASK_EFUSE_PG ... QSFP28_P5_MASK_EFUSE_PG:
    case QSFPDD_P12_MASK_EFUSE_PG ... QSFPDD_P15_MASK_EFUSE_PG:
    case QSFP28_0_5_EVT_ABS:
    case QSFPDD_12_15_EVT_ABS:
    case QSFP28_0_5_EVT_INTR:
    case QSFPDD_12_15_EVT_INTR:
    case QSFP28_0_5_EVT_EFUSE_PG:
    case QSFPDD_12_15_EVT_EFUSE_PG:
    case QSFP28_P0_RESET ... QSFP28_P5_RESET:
    case QSFPDD_P12_RESET ... QSFPDD_P15_RESET:
    case QSFP28_P0_LPMODE ... QSFP28_P5_LPMODE:
    case QSFPDD_P12_LPMODE ... QSFPDD_P15_LPMODE:
    case CLK_EN_CTRL:
    //CPLD3
    case GNSS_MODEL_ID:
    case OCXO_ID:
    case QSFP28_P6_ABS ... QSFP28_P11_ABS:
    case QSFP28_P6_INTR ... QSFP28_P11_INTR:
    case QSFP28_P6_EFUSE_PG ... QSFP28_P11_EFUSE_PG:
    case MAC_INTR:
    case FAN_0_ABS:
    case FAN_1_ABS:
    case FAN_2_ABS:
    case FAN_3_ABS:
    case FAN_4_ABS:
    case QSFP28_P6_MASK_ABS ... QSFP28_P11_MASK_ABS:
    case QSFP28_P6_MASK_INTR ... QSFP28_P11_MASK_INTR:
    case QSFP28_P6_MASK_EFUSE_PG ... QSFP28_P11_MASK_EFUSE_PG:
    case MAC_MASK_INTR:
    case FAN_0_MASK_ABS:
    case FAN_1_MASK_ABS:
    case FAN_2_MASK_ABS:
    case FAN_3_MASK_ABS:
    case FAN_4_MASK_ABS:
    case QSFP28_6_11_EVT_ABS:
    case QSFP28_6_11_EVT_INTR:
    case QSFP28_6_11_EVT_EFUSE_PG:
    case MAC_EVT_INTR:
    case FAN_EVT_ABS:
    case QSFP28_P6_RESET ... QSFP28_P11_RESET:
    case QSFP28_P6_LPMODE ... QSFP28_P11_LPMODE:
    case MAC_RESET:
    case USB_QSPI_RESET:
    case MAC_ROV:
    case I2C_ROV_MUX_SEL:
    case I2C_CPLD_MUX_EN:
    case I2C0_PSU_MUX_SEL:
    case I2C0_HWM_MUX_SEL:
    case I2C_IO_MUX_SEL:
    case FAN_SPEED_READ_MODE:
    case FAN_RPM_LOW_BYTE:
    case FAN_RPM_HIGH_BYTE:
    case SYSTEM_LED_STATUS_1:
    //CPLD4
    case SFP56_P16_ABS ... SFP56_P23_ABS:
    case SFP56_P24_ABS ... SFP56_P31_ABS:
    case SFP56_P32_ABS ... SFP56_P39_ABS:
    case SFP56_P16_RX_LOS ... SFP56_P23_RX_LOS:
    case SFP56_P24_RX_LOS ... SFP56_P31_RX_LOS:
    case SFP56_P32_RX_LOS ... SFP56_P39_RX_LOS:
    case SFP56_P16_TX_FAULT ... SFP56_P23_TX_FAULT:
    case SFP56_P24_TX_FAULT ... SFP56_P31_TX_FAULT:
    case SFP56_P32_TX_FAULT ... SFP56_P39_TX_FAULT:
    case SFP56_P16_MASK_ABS ... SFP56_P23_MASK_ABS:
    case SFP56_P24_MASK_ABS ... SFP56_P31_MASK_ABS:
    case SFP56_P32_MASK_ABS ... SFP56_P39_MASK_ABS:
    case SFP56_P16_MASK_RX_LOS ... SFP56_P23_MASK_RX_LOS:
    case SFP56_P24_MASK_RX_LOS ... SFP56_P31_MASK_RX_LOS:
    case SFP56_P32_MASK_RX_LOS ... SFP56_P39_MASK_RX_LOS:
    case SFP56_P16_MASK_TX_FAULT ... SFP56_P23_MASK_TX_FAULT:
    case SFP56_P24_MASK_TX_FAULT ... SFP56_P31_MASK_TX_FAULT:
    case SFP56_P32_MASK_TX_FAULT ... SFP56_P39_MASK_TX_FAULT:
    case SFP56_16_23_EVT_ABS:
    case SFP56_24_31_EVT_ABS:
    case SFP56_32_39_EVT_ABS:
    case SFP56_16_23_EVT_RX_LOS:
    case SFP56_24_31_EVT_RX_LOS:
    case SFP56_32_39_EVT_RX_LOS:
    case SFP56_16_23_EVT_TX_FAULT:
    case SFP56_24_31_EVT_TX_FAULT:
    case SFP56_32_39_EVT_TX_FAULT:
    case SFP56_P16_TX_DISABLE ... SFP56_P23_TX_DISABLE:
    case SFP56_P24_TX_DISABLE ... SFP56_P31_TX_DISABLE:
    case SFP56_P32_TX_DISABLE ... SFP56_P39_TX_DISABLE:
    case SFP56_P16_RATE_SEL ... SFP56_P23_RATE_SEL:
    case SFP56_P24_RATE_SEL ... SFP56_P31_RATE_SEL:
    case SFP56_P32_RATE_SEL ... SFP56_P39_RATE_SEL:
    case BSP_DEBUG:
        reg = attr_reg[attr->index].reg;
        mask= attr_reg[attr->index].mask;
        data_type = attr_reg[attr->index].data_type;
        break;
    default:
        return -EINVAL;
    }

    ret = cpld_reg_read(dev, &reg_val, reg, mask);
    if( ret < 0) {
        return ret;
    }
    return _parse_data(buf, reg_val, data_type);
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
    bool write_protect;

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
    //CPLD_COMMON
    case EVENT_DETECT_CTRL:
    case MODULE_RESET:
    //CPLD1
    case CLK_PTP_MASK_INTR:
    case PHY_MASK_INTR:
    case TOP_BRD_CPLD_FRU_MASK_INTR:
    case PSU0_MASK_INTR:
    case PSU1_MASK_INTR:
    case CPLD2_MASK_INTR:
    case CPLD2_IO_MASK_INTR:
    case MAIN_BRD_CPLD_MASK_INTR:
    case CPLD3_MASK_INTR:
    case CPLD4_MASK_INTR:
    case MB_ETH_MASK_INTR:
    case MB_PTP_MASK_INTR:
    case FAN_MASK_INTR:
    case THERMAL_MASK_INTR:
    case CPU_NMI_MASK_INTR:
    case OUT_STATUS_MASK_INTR:
    case BTN_FP_RESET:
    case SPI_BIOS_RESET:
    case RGB_0_RESET:
    case RGB_1_RESET:
    case BMC_LPC_RESET:
    case BMC_PCIE_RESET:
    case CPLD_TO_BMC_SYS_RESET:
    case CPLD_TO_CPU_RESET:
    case USB_PWR_EN:
    case USB_SIE_RESET:
    case I2C_MUX_SYS_RESET:
    case I2C_MUX_SMBUS_RESET:
    case I2C_MUX_QSFP28_RESET:
    case I2C_MUX_SFP_RESET:
    case MB_I2C_RESET:
    case BMC_WDT_1_RESET:
    case BMC_WDT_2_RESET:
    case SMBUS_PECI_DIS:
    case I2C_PSU0_MUX_SEL:
    case I2C_PSU1_MUX_SEL:
    case I2C_CPLD_MUX_SEL:
    case BMC_USB_MUX_SEL:
    case UART_CPU_BMC_MUX_SEL:
    case UART_MUX_SEL:
    case EXT_CTRL:
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
    case CPLD_SYSTEM_LED_PWR:
    case PWR_LED_STATUS:
    case PWR_LED_SPEED:
    case PWR_LED_BLINK:
    case PWR_LED_ONOFF:
    case CPLD_SYSTEM_LED_GNSS:
    case GNSS_LED_STATUS:
    case GNSS_LED_SPEED:
    case GNSS_LED_BLINK:
    case GNSS_LED_ONOFF:
    case CPLD_SYSTEM_LED_SYNC:
    case SYNC_LED_STATUS:
    case SYNC_LED_SPEED:
    case SYNC_LED_BLINK:
    case SYNC_LED_ONOFF:
    case LED_CLEAR:
    case QSFP28_P0_PWR_EN ... QSFP28_P5_PWR_EN:
    case QSFP28_P6_PWR_EN ... QSFP28_P11_PWR_EN:
    case QSFPDD_P12_PWR_EN ... QSFPDD_P15_PWR_EN:
    case SFP56_P16_PWR_EN ... SFP56_P23_PWR_EN:
    case SFP56_P24_PWR_EN ... SFP56_P31_PWR_EN:
    case SFP56_P32_PWR_EN ... SFP56_P39_PWR_EN:
    case CLK_PTP_RESET:
    case CJAP_RESET:
    case NTM_RESET:
    case GNSS_RESET:
    case BITS_RESET:
    case CLK_TIMING_CTRL:
    case QSFPDD_SEL:
    case QSFP28_SEL:
    case I2C_MUX_0X76_RESET:
    case I2C_MUX_0X75_RESET:
    case I2C_MUX_QSFP28_6_11_RESET:
    case I2C_MUX_SFP56_16_23_RESET:
    case I2C_MUX_SFP56_24_31_RESET:
    case I2C_MUX_SFP56_32_39_RESET:
    case I2C_MUX_0X71_RESET:

    //CPLD2
    case QSFP28_P0_MASK_ABS ... QSFP28_P5_MASK_ABS:
    case QSFPDD_P12_MASK_ABS ... QSFPDD_P15_MASK_ABS:
    case QSFP28_P0_MASK_INTR ... QSFP28_P5_MASK_INTR:
    case QSFPDD_P12_MASK_INTR ... QSFPDD_P15_MASK_INTR:
    case QSFP28_P0_MASK_EFUSE_PG ... QSFP28_P5_MASK_EFUSE_PG:
    case QSFPDD_P12_MASK_EFUSE_PG ... QSFPDD_P15_MASK_EFUSE_PG:
    case QSFP28_P0_RESET ... QSFP28_P5_RESET:
    case QSFPDD_P12_RESET ... QSFPDD_P15_RESET:
    case QSFP28_P0_LPMODE ... QSFP28_P5_LPMODE:
    case QSFPDD_P12_LPMODE ... QSFPDD_P15_LPMODE:
    case CLK_EN_CTRL:

    //CPLD3
    case QSFP28_P6_MASK_ABS ... QSFP28_P11_MASK_ABS:
    case QSFP28_P6_MASK_INTR ... QSFP28_P11_MASK_INTR:
    case QSFP28_P6_MASK_EFUSE_PG ... QSFP28_P11_MASK_EFUSE_PG:
    case MAC_MASK_INTR:
    case FAN_0_MASK_ABS:
    case FAN_1_MASK_ABS:
    case FAN_2_MASK_ABS:
    case FAN_3_MASK_ABS:
    case FAN_4_MASK_ABS:
    case QSFP28_P6_RESET ... QSFP28_P11_RESET:
    case QSFP28_P6_LPMODE ... QSFP28_P11_LPMODE:
    case MAC_RESET:
    case USB_QSPI_RESET:
    case I2C_ROV_MUX_SEL:
    case I2C_CPLD_MUX_EN:
    case I2C0_PSU_MUX_SEL:
    case I2C0_HWM_MUX_SEL:
    case I2C_IO_MUX_SEL:
    case SYSTEM_LED_STATUS_1:

    //CPLD4
    case SFP56_P16_RX_LOS ... SFP56_P23_RX_LOS:
    case SFP56_P24_RX_LOS ... SFP56_P31_RX_LOS:
    case SFP56_P32_RX_LOS ... SFP56_P39_RX_LOS:
    case SFP56_P16_TX_FAULT ... SFP56_P23_TX_FAULT:
    case SFP56_P24_TX_FAULT ... SFP56_P31_TX_FAULT:
    case SFP56_P32_TX_FAULT ... SFP56_P39_TX_FAULT:
    case SFP56_P16_MASK_ABS ... SFP56_P23_MASK_ABS:
    case SFP56_P24_MASK_ABS ... SFP56_P31_MASK_ABS:
    case SFP56_P32_MASK_ABS ... SFP56_P39_MASK_ABS:
    case SFP56_P16_MASK_RX_LOS ... SFP56_P23_MASK_RX_LOS:
    case SFP56_P24_MASK_RX_LOS ... SFP56_P31_MASK_RX_LOS:
    case SFP56_P32_MASK_RX_LOS ... SFP56_P39_MASK_RX_LOS:
    case SFP56_P16_MASK_TX_FAULT ... SFP56_P23_MASK_TX_FAULT:
    case SFP56_P24_MASK_TX_FAULT ... SFP56_P31_MASK_TX_FAULT:
    case SFP56_P32_MASK_TX_FAULT ... SFP56_P39_MASK_TX_FAULT:
    case SFP56_16_23_EVT_RX_LOS:
    case SFP56_24_31_EVT_RX_LOS:
    case SFP56_32_39_EVT_RX_LOS:
    case SFP56_16_23_EVT_TX_FAULT:
    case SFP56_24_31_EVT_TX_FAULT:
    case SFP56_32_39_EVT_TX_FAULT:
    case SFP56_P16_TX_DISABLE ... SFP56_P23_TX_DISABLE:
    case SFP56_P24_TX_DISABLE ... SFP56_P31_TX_DISABLE:
    case SFP56_P32_TX_DISABLE ... SFP56_P39_TX_DISABLE:
    case SFP56_P16_RATE_SEL ... SFP56_P23_RATE_SEL:
    case SFP56_P24_RATE_SEL ... SFP56_P31_RATE_SEL:
    case SFP56_P32_RATE_SEL ... SFP56_P39_RATE_SEL:
    case BSP_DEBUG:
        reg = attr_reg[attr->index].reg;
        mask= attr_reg[attr->index].mask;
        write_protect = attr_reg[attr->index].write_protect;
        break;
    default:
        return -EINVAL;
    }
    return cpld_reg_write(dev, reg_val, count, reg, mask, write_protect);
}

/* get cpld register value */
u8 _cpld_reg_read(struct device *dev, u8 reg, u8 mask)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
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
                    u8 *reg_val,
                    u8 reg,
                    u8 mask)
{
    int ret = 0;

    if(reg_val == NULL) {
        return -EINVAL;
    }

    ret = _cpld_reg_read(dev, reg, mask);
    if (unlikely(ret < 0)) {
        dev_err(dev, "cpld_reg_read() error, reg_val=%d\n", ret);
        return ret;
    }

    *reg_val = (u8)ret;
    return 0;
}

u8 _cpld_reg_write(struct device *dev,
                    u8 reg,
                    u8 reg_val)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
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
    struct cpld_data *data = i2c_get_clientdata(i2c_client);

    BSP_LOG_W("Writing protected cpld register, reg=0x%x, value=0x%x\n", reg, reg_val);

    // lock the write protect session
    mutex_lock(&data->access_lock);

    // read the write protect reg
    ret = i2c_smbus_read_byte_data(i2c_client, reg_wp);
    if (unlikely(ret < 0))
    {
        dev_err(dev, "i2c_smbus_read_byte_data() error, reg=0x%x, return=%d\n", reg_wp, ret);
        goto unlock;
    }

    current_wp = ret;

    if (!(current_wp & reg_wp_mask))
    {
        // enable reg write
        reg_wp_val = current_wp | reg_wp_mask;
        ret = i2c_smbus_write_byte_data(i2c_client, reg_wp, reg_wp_val);

        if (unlikely(ret < 0))
        {
            dev_err(dev, "i2c_smbus_write_byte_data() error, reg=0x%x, value=0x%x, return=%d\n", reg_wp, reg_wp_val, ret);
            goto unlock;
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

unlock:
    // unlock the write protect session
    mutex_unlock(&data->access_lock);
    return ret;
}

/* set cpld register value */
static ssize_t cpld_reg_write(struct device *dev,
                    u8 reg_val,
                    size_t count,
                    u8 reg,
                    u8 mask,
                    bool write_protect)
{
    u8 reg_val_now, shift;
    int ret = 0;

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

    if (write_protect)
    {
        ret = _cpld_reg_write_with_protect(dev, reg, reg_val);
    }
    else
    {
        ret = _cpld_reg_write(dev, reg, reg_val);
    }

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


#if 0
static int _get_led_node(int index, led_node_t *node)
{
    color_obj_t mgmt_port_set[COLOR_VAL_MAX] = {
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

    switch (index){
        case CPLD_MGMT_PORT_0_LED_STATUS:
        case CPLD_MGMT_PORT_1_LED_STATUS:
            node->type=TYPE_LED_2_SETS;
            node->reg = attr_reg[index].reg;
            node->mask= attr_reg[index].mask;
            node->color_mask = MASK_0000_1101;
            node->data_type = attr_reg[index].data_type;
            memcpy(node->color_obj, mgmt_port_set, sizeof(mgmt_port_set));
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static ssize_t led_show(struct device *dev,
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

static ssize_t led_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    led_node_t node = {0};
    int status = LED_COLOR_DARK;
    short int val;
    int found = 0;
    int i = 0;

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

    return cpld_reg_write(dev, (u8)val, count, node.reg, node.mask, REG_WP_DIS);
}
#endif

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
    struct cpld_data *data = NULL;
    int ret = -EPERM;

    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    /* init cpld data for client */
    i2c_set_clientdata(client, data);
    mutex_init(&data->access_lock);

    if (!i2c_check_functionality(client->adapter,
                I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_info(&client->dev,
            "i2c_check_functionality failed (0x%x)\n",
            client->addr);
        status = -EIO;
        goto exit;
    }

    /* get cpld id from device */
    ret = i2c_smbus_read_byte_data(client, CPLD_ID_REG);

    if (ret < 0) {
        dev_info(&client->dev,
            "fail to get cpld id (0x%x) at addr (0x%x)\n",
            CPLD_ID_REG, client->addr);
        status = -EIO;
        goto exit;
    }

    if (INVALID(ret, cpld1, cpld4)) {
        dev_info(&client->dev,
            "cpld id %d(device) not valid\n", ret);
        //status = -EPERM;
        //goto exit;
    }

    data->index = dev_id->driver_data;

    /* register sysfs hooks for different cpld group */
    dev_info(&client->dev, "probe cpld with index %d\n", data->index);
    switch (data->index) {
    case cpld1:
        status = sysfs_create_group(&client->dev.kobj,
                    &cpld1_group);
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
    default:
        status = -EINVAL;
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
    case cpld4:
        sysfs_remove_group(&client->dev.kobj, &cpld4_group);
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
    struct cpld_data *data = i2c_get_clientdata(client);

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
    case cpld4:
        sysfs_remove_group(&client->dev.kobj, &cpld4_group);
        break;
    default:
        break;
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
MODULE_VERSION("1.0.0");
MODULE_LICENSE("GPL");

module_init(cpld_init);
module_exit(cpld_exit);
