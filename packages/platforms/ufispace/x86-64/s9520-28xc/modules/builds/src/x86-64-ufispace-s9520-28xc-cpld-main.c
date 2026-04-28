/*
 * A i2c cpld driver for the ufispace_s9520_28xc
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

#include "x86-64-ufispace-s9520-28xc-cpld-main.h"

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
    /* CPLD common information registers */
    // & cpld_common
    [CPLD_VERSION]                     = { CPLD_VERSION_REG,                  MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [CPLD_MINOR_VER]                   = { CPLD_VERSION_REG,                  0b00111111,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [CPLD_MAJOR_VER]                   = { CPLD_VERSION_REG,                  0b11000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [CPLD_ID]                          = { CPLD_ID_REG,                       MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [CPLD_BUILD_VER]                   = { CPLD_BUILD_REG,                    MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [CPLD_VERSION_H]                   = { NONE_REG,                          MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [CPLD_CHIP_TYPE]                   = { CPLD_CHIP_TYPE_REG,                0b00000111,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [EVENT_CTRL]                       = { EVENT_DETECT_CTRL_REG,             0b00000001,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [EVENT_DETECT_CTRL]                = { EVENT_DETECT_CTRL_REG,             MASK_ALL,          DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [REMOTE_I2C_RESET]                 = { MODULE_RESET_REG,                  0b00000001,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [MODULE_RESET]                     = { MODULE_RESET_REG,                  MASK_ALL,          DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    /* Write Protect */
    [CPLD_WRITE_PROTECT]               = { WRITE_PROTECT_1_REG,               MASK_ALL,          DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    // & cpld1
    /* Board information */
    [CPLD_SKU_ID]                      = { SKU_ID_REV_REG,                    MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [BRD_HW_ID]                        = { HW_BUILD_REV_REG,                  0b00000011,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [DEPH_ID]                          = { HW_BUILD_REV_REG,                  0b00000100,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [BUILD_ID]                         = { HW_BUILD_REV_REG,                  0b00011000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [BIT_SEL_ID]                       = { HW_BUILD_REV_REG,                  0b00100000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [HW_BUILD_REV]                     = { HW_BUILD_REV_REG,                  MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [EXT_ID]                           = { CPLD_EXT_ID_REG,                   0b00000111,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [CPLD_EXT_ID]                      = { CPLD_EXT_ID_REG,                   MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    /* Interrupt Status */

    [CJA_LOL_INTR]                     = { CLK_PTP_INTR_REG,                  0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [NTM_INTR]                         = { CLK_PTP_INTR_REG,                  0b00010000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [DPLL_1588_INTR]                   = { CLK_PTP_INTR_REG,                  0b00100000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [DPLL_SYNC_INTR]                   = { CLK_PTP_INTR_REG,                  0b01000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [BITS_INTR]                        = { CLK_PTP_INTR_REG,                  0b10000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [I2C_NIC_ALRT]                     = { I2C_NIC_INTR_REG,                  0b10000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [PSU1_INTR]                        = { TOP_BRD_CPLD_FRU_INTR_REG,         0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [PSU2_INTR]                        = { TOP_BRD_CPLD_FRU_INTR_REG,         0b00000010,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [VDD_CORE_PIN_ALRT_INTR]           = { TOP_BRD_CPLD_FRU_INTR_REG,         0b00000100,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [CPLD2_INTR]                       = { TOP_BRD_CPLD_FRU_INTR_REG,         0b00001000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN_CARD_INTR]                    = { TOP_BRD_CPLD_FRU_INTR_REG,         0b00010000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [USB_PWR_OC]                       = { MAIN_BRD_CPLD_INTR_REG,            0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [SSD1_PWR_OC]                      = { MAIN_BRD_CPLD_INTR_REG,            0b00000010,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [SSD2_PWR_OC]                      = { MAIN_BRD_CPLD_INTR_REG,            0b00000100,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [GNSS_INTR]                        = { MAIN_BRD_CPLD_INTR_REG,            0b00001000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [HWM_NMI_INTR]                     = { MAIN_BRD_CPLD_INTR_REG,            0b00100000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [TSEN_NMI_INTR]                    = { THERMAL_INTR_REG,                  0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [TSEN1_NMI_INTR]                   = { THERMAL_INTR_REG,                  0b00000010,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [TSEN2_NMI_INTR]                   = { THERMAL_INTR_REG,                  0b00000100,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [TSEN_ALRT_INTR]                   = { THERMAL_INTR_REG,                  0b00001000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [TSEN1_ALRT_INTR]                  = { THERMAL_INTR_REG,                  0b00010000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [TSEN2_ALRT_INTR]                  = { THERMAL_INTR_REG,                  0b00100000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [CPU_THRMTRIP]                     = { THERMAL_INTR_REG,                  0b01000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [VDD_CORE_VRHOT]                   = { THERMAL_INTR_REG,                  0b10000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [THERMAL_INTR]                     = { THERMAL_INTR_REG,                  MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [MAC_INTR]                         = { MAC_INTR_REG,                      0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [CPU_NMI_INTR]                    = { CPU_NMI_INTR_REG,                  0b00000001,         DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [USB_PWR_OC_CPU]                   = { OUT_STATUS_INTR_REG,               0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [NMI_CPLD_TO_CPU]                  = { OUT_STATUS_INTR_REG,               0b00000010,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [PTP_TO_CPU_INTR]                  = { OUT_STATUS_INTR_REG,               0b00000100,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_TO_CPU_INTR]                  = { OUT_STATUS_INTR_REG,               0b00001000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [THRM_CPLD_TO_CPU_INTR]            = { OUT_STATUS_INTR_REG,               0b00010000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [NMI_SYS_TO_BMC]                   = { OUT_STATUS_INTR_REG,               0b00100000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [THRM_CHIP_INTR]                   = { OUT_STATUS_INTR_REG,               0b01000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [CPLD_TO_CPU_INTR]                 = { OUT_STATUS_INTR_REG,               0b10000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    /* Interrupt Mask */

    [BITS_INTR_MASK]                   = { CLK_PTP_INTR_MASK_REG,             0b10000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [DPLL_SYNC_INTR_MASK]              = { CLK_PTP_INTR_MASK_REG,             0b01000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [DPLL_1588_INTR_MASK]              = { CLK_PTP_INTR_MASK_REG,             0b00100000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [NTM_INTR_MASK]                    = { CLK_PTP_INTR_MASK_REG,             0b00010000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [CJA_LOL_INTR_MASK]                = { CLK_PTP_INTR_MASK_REG,             0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [I2C_NIC_ALRT_MASK]                = { PHY_INTR_MASK_REG,                 0b10000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [FAN_CARD_INTR_MASK]               = { TOP_BRD_CPLD_FRU_INTR_MASK_REG,    0b00001000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [CPLD2_INTR_MASK]                  = { TOP_BRD_CPLD_FRU_INTR_MASK_REG,    0b00000100,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [PSU2_INTR_MASK]                   = { TOP_BRD_CPLD_FRU_INTR_MASK_REG,    0b00000010,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [PSU1_INTR_MASK]                   = { TOP_BRD_CPLD_FRU_INTR_MASK_REG,    0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [HWM_NMI_INTR_MASK]                = { MAIN_BRD_CPLD_INTR_MASK_REG,       0b00100000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [GNSS_INTR_MASK]                   = { MAIN_BRD_CPLD_INTR_MASK_REG,       0b00001000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [SSD2_PWR_OC_MASK]                 = { MAIN_BRD_CPLD_INTR_MASK_REG,       0b00000100,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [SSD1_PWR_OC_MASK]                 = { MAIN_BRD_CPLD_INTR_MASK_REG,       0b00000010,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [USB_PWR_OC_MASK]                  = { MAIN_BRD_CPLD_INTR_MASK_REG,       0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [VDD_CORE_VRHOT_MASK]              = { THERMAL_INTR_MASK_REG,             0b10000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [CPU_THRMTRIP_MASK]                = { THERMAL_INTR_MASK_REG,             0b01000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [TSEN2_ALRT_INTR_MASK]             = { THERMAL_INTR_MASK_REG,             0b00100000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [TSEN1_ALRT_INTR_MASK]             = { THERMAL_INTR_MASK_REG,             0b00010000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [TSEN_ALRT_INTR_MASK]              = { THERMAL_INTR_MASK_REG,             0b00001000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [TSEN2_NMI_INTR_MASK]              = { THERMAL_INTR_MASK_REG,             0b00000100,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [TSEN1_NMI_INTR_MASK]              = { THERMAL_INTR_MASK_REG,             0b00000010,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [TSEN_NMI_INTR_MASK]               = { THERMAL_INTR_MASK_REG,             0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [MAC_INTR_MASK]                    = { MAC_INTR_MASK_REG,                 0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [CPU_NMI_INTR_MASK]                = { CPU_NMI_INTR_MASK_REG,             MASK_ALL,          DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},

    [CPLD_TO_CPU_INTR_MASK]            = { OUT_STATUS_INTR_MASK_REG,          0b10000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [THRM_CHIP_INTR_MASK]              = { OUT_STATUS_INTR_MASK_REG,          0b01000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [NMI_SYS_TO_BMC_MASK]              = { OUT_STATUS_INTR_MASK_REG,          0b00100000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [THRM_CPLD_TO_CPU_INTR_MASK]       = { OUT_STATUS_INTR_MASK_REG,          0b00010000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_TO_CPU_INTR_MASK]             = { OUT_STATUS_INTR_MASK_REG,          0b00001000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [PTP_TO_CPU_INTR_MASK]             = { OUT_STATUS_INTR_MASK_REG,          0b00000100,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [NMI_CPLD_TO_CPU_MASK]             = { OUT_STATUS_INTR_MASK_REG,          0b00000010,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [USB_PWR_OC_CPU_MASK]              = { OUT_STATUS_INTR_MASK_REG,          0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    /* Interrupt Event */
    [CLK_PTP_INTR_EVENT]               = { CLK_PTP_INTR_EVENT_REG,            MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},
    [PHY_INTR_EVENT]                   = { PHY_INTR_EVENT_REG,                MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},
    [TOP_BRD_CPLD_FRU_INTR_EVENT]      = { TOP_BRD_CPLD_FRU_INTR_EVENT_REG,   MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},
    [MAIN_BRD_CPLD_INTR_EVENT]         = { MAIN_BRD_CPLD_INTR_EVENT_REG,      MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},
    [THERMAL_INTR_EVENT]               = { THERMAL_INTR_EVENT_REG,            MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},
    [MAC_INTR_EVENT]                   = { MAC_INTR_EVENT_REG,                MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},
    [CPU_NMI_INTR_EVENT]               = { CPU_NMI_INTR_EVENT_REG,            MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},
    [OUT_STATUS_INTR_EVENT]            = { OUT_STATUS_INTR_EVENT_REG,         MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},
    /* Reset */
    [MAC_SYS_RESET]                    = { MAC_RESET_REG,                     0b00000001,        DATA_DEC,      REG_WP_EN , REG_NOT_EVENT},
    [MAC_QSPI_RESET]                   = { MAC_RESET_REG,                     0b00000010,        DATA_DEC,      REG_WP_EN , REG_NOT_EVENT},
    [MAC_RESET]                        = { MAC_RESET_REG,                     MASK_ALL,          DATA_HEX,      REG_WP_EN , REG_NOT_EVENT},
    [SPI_BIOS_RESET]                   = { BIOS_FLASH_RESET_CTRL_REG,         0b00000001,        DATA_DEC,      REG_WP_EN , REG_NOT_EVENT},
    [BTN_FP_RESET]                     = { BIOS_FLASH_RESET_CTRL_REG,         0b00010000,        DATA_DEC,      REG_WP_EN , REG_NOT_EVENT},
    [BIOS_FLASH_RESET_CTRL]            = { BIOS_FLASH_RESET_CTRL_REG,         MASK_ALL,          DATA_HEX,      REG_WP_EN , REG_NOT_EVENT},
    [CPLD_TO_CPU_RESET]                = { BMC_PHY_RESET_CTRL_REG,            0b00000001,        DATA_DEC,      REG_WP_EN , REG_NOT_EVENT},
    [CPLD_TO_BMC_SYS_RESET]            = { BMC_PHY_RESET_CTRL_REG,            0b00000010,        DATA_DEC,      REG_WP_EN , REG_NOT_EVENT},
    [BMC_PCIE_RESET]                   = { BMC_PHY_RESET_CTRL_REG,            0b00000100,        DATA_DEC,      REG_WP_EN , REG_NOT_EVENT},
    [BMC_LPC_RESET]                    = { BMC_PHY_RESET_CTRL_REG,            0b00001000,        DATA_DEC,      REG_WP_EN , REG_NOT_EVENT},
    [CPU_MON_RESET]                    = { BMC_PHY_RESET_CTRL_REG,            0b10000000,        DATA_DEC,      REG_WP_EN , REG_NOT_EVENT},
    [BMC_PHY_RESET_CTRL]               = { BMC_PHY_RESET_CTRL_REG,            MASK_ALL,          DATA_HEX,      REG_WP_EN , REG_NOT_EVENT},

    [USB_PWR_EN]                       = { USB_RESET_CTRL_REG,                0b10000000,        DATA_DEC,      REG_WP_EN , REG_NOT_EVENT},
    [USB_RESET_CTRL]                   = { USB_RESET_CTRL_REG,                MASK_ALL,          DATA_HEX,      REG_WP_EN , REG_NOT_EVENT},

    [CJA_RESET]                        = { JA_RESET_CTRL_REG,                 0b00000001,        DATA_DEC,      REG_WP_EN , REG_NOT_EVENT},
    [OE_CJA]                           = { JA_RESET_CTRL_REG,                 0b00000010,        DATA_DEC,      REG_WP_EN , REG_NOT_EVENT},
    [JA_WP]                            = { JA_RESET_CTRL_REG,                 0b00000100,        DATA_DEC,      REG_WP_EN , REG_NOT_EVENT},
    [JA_RESET_CTRL]                    = { JA_RESET_CTRL_REG,                 MASK_ALL,          DATA_HEX,      REG_WP_EN , REG_NOT_EVENT},

    [I2C_MUX_0X75_RESET]               = { I2C_MUX_RESET_REG,                 0b00000001,        DATA_DEC,      REG_WP_EN , REG_NOT_EVENT},
    [I2C_MUX_0X77_RESET]               = { I2C_MUX_RESET_REG,                 0b00000010,        DATA_DEC,      REG_WP_EN , REG_NOT_EVENT},
    [I2C_MUX_0X76_RESET]               = { I2C_MUX_RESET_REG,                 0b00000100,        DATA_DEC,      REG_WP_EN , REG_NOT_EVENT},
    [I2C_MUX_RESET]                    = { I2C_MUX_RESET_REG,                 MASK_ALL,          DATA_DEC,      REG_WP_EN , REG_NOT_EVENT},

    [NIC1_PCIE_RESET]                  = { NIC_CTRL_REG,                      0b00000010,        DATA_DEC,      REG_WP_EN , REG_NOT_EVENT},
    [CPLD1_TO_CPLD2_RESET]             = { NIC_CTRL_REG,                      0b00000100,        DATA_DEC,      REG_WP_EN , REG_NOT_EVENT},
    [NIC_CTRL]                         = { NIC_CTRL_REG,                      MASK_ALL,          DATA_HEX,      REG_WP_EN , REG_NOT_EVENT},

    [PSU1_EEPROM_WP]                   = { PSU_EEPROM_CTRL_REG,               0b00000001,        DATA_DEC,      REG_WP_EN , REG_NOT_EVENT},
    [PSU2_EEPROM_WP]                   = { PSU_EEPROM_CTRL_REG,               0b00000010,        DATA_DEC,      REG_WP_EN , REG_NOT_EVENT},
    [PSU_EEPROM_CTRL]                  = { PSU_EEPROM_CTRL_REG,               MASK_ALL,          DATA_HEX,      REG_WP_EN , REG_NOT_EVENT},

    [SYS_PWR_RESET]                    = { SYS_PWR_RESET_REG,                 0b00000001,        DATA_DEC,      REG_WP_EN , REG_NOT_EVENT},
    /* Misc Status Control */

    [PSU1_PRESENT]                     = { PSU_STATUS_REG,                    0b00000001,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [PSU2_PRESENT]                     = { PSU_STATUS_REG,                    0b00000010,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [PSU1_VIN_PWROK]                   = { PSU_STATUS_REG,                    0b00000100,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [PSU2_VIN_PWROK]                   = { PSU_STATUS_REG,                    0b00001000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [PSU1_VOUT_PWROK]                  = { PSU_STATUS_REG,                    0b00010000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [PSU2_VOUT_PWROK]                  = { PSU_STATUS_REG,                    0b00100000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [PSU_STATUS]                       = { PSU_STATUS_REG,                    MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [DAUGHTER_BRD_PRESENT]             = { DAUGHTER_BRD_PRSNT_REG,            MASK_ALL,          DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},

    [CPU_PWRGD]                        = { SYS_PWR_STATUS_REG,                0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [CPU_BOOT_DONE]                    = { SYS_PWR_STATUS_REG,                0b00000010,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [WAKE_CPU_PCIE]                    = { SYS_PWR_STATUS_REG,                0b10000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [SYS_PWR_STATUS]                   = { SYS_PWR_STATUS_REG,                MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [PHY_BOOT_CTRL]                    = { PHY_BOOT_CTRL_REG,                 MASK_ALL,          DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},

    [BMC_WDT1_RESET]                   = { WD_STATUS_REG,                     0b00000010,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [BMC_WDT2_RESET]                   = { WD_STATUS_REG,                     0b00000100,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [WD_STATUS]                        = { WD_STATUS_REG,                     MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [BIOS_BOOT_SEL0]                   = { TIMING_CTRL_STATUS_REG,            0b00000011,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [BIOS_BOOT_SEL1]                   = { TIMING_CTRL_STATUS_REG,            0b00000010,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [TIMING_CTRL_STATUS]               = { TIMING_CTRL_STATUS_REG,            MASK_ALL,          DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},

    [I2C_ROV_MUX_SEL]                  = { MUX_CTRL_REG,                      0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [QSPI_MAC_MUX_SEL]                 = { MUX_CTRL_REG,                      0b00001000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [I2C_CPLD_MUX_SEL]                 = { MUX_CTRL_REG,                      0b00100000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [UART_MUX_SEL]                     = { MUX_CTRL_REG,                      0b01000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [UART_CPU_BMC_MUX_SEL]             = { MUX_CTRL_REG,                      0b10000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [MUX_CTRL]                         = { MUX_CTRL_REG,                      MASK_ALL,          DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},

    [ICX_SPI_MUX_SEL_1]                = { BIOS_SPI_MUX_REG,                  0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [ICX_SPI_MUX_SEL_2]                = { BIOS_SPI_MUX_REG,                  0b00000010,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [ICX_SPI_MUX_SEL_3]                = { BIOS_SPI_MUX_REG,                  0b00000100,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [BIOS_SPI_MUX]                     = { BIOS_SPI_MUX_REG,                  MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [PWREN_CPLD_TO_CPU]                = { PWR_SYSTEM_CTRL_REG,               0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [PWRBTN_CPLD_TO_CPU]               = { PWR_SYSTEM_CTRL_REG,               0b00000010,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [PWR_SYSTEM_CTRL]                  = { PWR_SYSTEM_CTRL_REG,               MASK_ALL,          DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    /* SERBOOT Control */

    [UFM_WRITE]                        = { UFM_WRITE_EN_REG,                  0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [UFM_WRITE_EN]                     = { UFM_WRITE_EN_REG,                  MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},


    [BIOS_BOOT_SEL0_TGT]               = { BIOS_BOOT_SEL_TGT_REG,             0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [BIOS_BOOT_SEL1_TGT]               = { BIOS_BOOT_SEL_TGT_REG,             0b00000010,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [BIOS_BOOT_SEL_TGT]                = { BIOS_BOOT_SEL_TGT_REG,             MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    /* LED Clear & GNSS Control */

    [LED_FAN_CLR]                      = { LED_CLEAR_REG,                     0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [LED_CLEAR]                        = { LED_CLEAR_REG,                     MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [GNSS_ANT_PWREN]                   = { GNSS_CTRL_REG,                     0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [GNSS_RESET]                       = { GNSS_CTRL_REG,                     0b00010000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [ST_GNSS_10M_OUT]                  = { GNSS_CTRL_REG,                     0b10000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [GNSS_CTRL]                        = { GNSS_CTRL_REG,                     MASK_ALL,          DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},

    [TS_PLL_CLK_EN]                    = { TIMING_MISC_CTRL_REG,              0b00000100,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [P3V3_EN_BROADSYNC]                = { TIMING_MISC_CTRL_REG,              0b00001000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [TIMING_MISC_CTRL]                 = { TIMING_MISC_CTRL_REG,              MASK_ALL,          DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    /* Write Protect */

    [EXT_REG_DEFINE]                   = { EXT_CTRL_REG,                      0b00000011,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [EXT_REG_RANGE]                    = { EXT_CTRL_REG,                      0b00011000,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [EXT_CTRL]                         = { EXT_CTRL_REG,                      MASK_ALL,          DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},

    /* LED Control */
    [PWR_LED_COLOR]                    = { SYSTEM_LED_CTRL_1_REG,             0b00000001,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [PWR_LED_SPEED]                    = { SYSTEM_LED_CTRL_1_REG,             0b00000010,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [PWR_LED_BLINKING]                 = { SYSTEM_LED_CTRL_1_REG,             0b00000100,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [PWR_LED_ON_OFF]                   = { SYSTEM_LED_CTRL_1_REG,             0b00001000,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [CPLD_SYSTEM_LED_PWR]              = { SYSTEM_LED_CTRL_1_REG,             0b00001101,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN_LED_COLOR]                    = { SYSTEM_LED_CTRL_1_REG,             0b00010000,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN_LED_SPEED]                    = { SYSTEM_LED_CTRL_1_REG,             0b00100000,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN_LED_BLINKING]                 = { SYSTEM_LED_CTRL_1_REG,             0b01000000,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN_LED_ON_OFF]                   = { SYSTEM_LED_CTRL_1_REG,             0b10000000,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [CPLD_SYSTEM_LED_FAN]              = { SYSTEM_LED_CTRL_1_REG,             0b11010000,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [SYSTEM_LED_CTRL_1]                = { SYSTEM_LED_CTRL_1_REG,             MASK_ALL,          DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},

    [GNSS_LED_COLOR]                   = { SYSTEM_LED_CTRL_2_REG,             0b00000001,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [GNSS_LED_SPEED]                   = { SYSTEM_LED_CTRL_2_REG,             0b00000010,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [GNSS_LED_BLINKING]                = { SYSTEM_LED_CTRL_2_REG,             0b00000100,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [GNSS_LED_ON_OFF]                  = { SYSTEM_LED_CTRL_2_REG,             0b00001000,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [CPLD_SYSTEM_LED_GNSS]             = { SYSTEM_LED_CTRL_2_REG,             0b00001101,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [SYS_LED_COLOR]                    = { SYSTEM_LED_CTRL_2_REG,             0b00010000,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [SYS_LED_SPEED]                    = { SYSTEM_LED_CTRL_2_REG,             0b00100000,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [SYS_LED_BLINKING]                 = { SYSTEM_LED_CTRL_2_REG,             0b01000000,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [SYS_LED_ON_OFF]                   = { SYSTEM_LED_CTRL_2_REG,             0b10000000,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [CPLD_SYSTEM_LED_SYS]              = { SYSTEM_LED_CTRL_2_REG,             0b11010000,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [SYSTEM_LED_CTRL_2]                = { SYSTEM_LED_CTRL_2_REG,             MASK_ALL,          DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},

    [SYNC_LED_COLOR]                   = { SYSTEM_LED_CTRL_3_REG,             0b00000001,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [SYNC_LED_SPEED]                   = { SYSTEM_LED_CTRL_3_REG,             0b00000010,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [SYNC_LED_BLINKING]                = { SYSTEM_LED_CTRL_3_REG,             0b00000100,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [SYNC_LED_ON_OFF]                  = { SYSTEM_LED_CTRL_3_REG,             0b00001000,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [CPLD_SYSTEM_LED_SYNC]             = { SYSTEM_LED_CTRL_3_REG,             0b00001101,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [ID_LED_SPEED]                     = { SYSTEM_LED_CTRL_3_REG,             0b00100000,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [ID_LED_BLINKING]                  = { SYSTEM_LED_CTRL_3_REG,             0b01000000,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [ID_LED_ON_OFF]                    = { SYSTEM_LED_CTRL_3_REG,             0b10000000,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [CPLD_SYSTEM_LED_ID]               = { SYSTEM_LED_CTRL_3_REG,             0b11100000,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [SYSTEM_LED_CTRL_3]                = { SYSTEM_LED_CTRL_3_REG,             MASK_ALL,          DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},

    [FAN_LED_1_2]                      = { FAN_LED_1_2_REG,                   MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN2_LED_ON_OFF]                  = { FAN_LED_1_2_REG,                   0b10000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN2_LED_BLINKING]                = { FAN_LED_1_2_REG,                   0b01000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN2_LED_SPEED]                   = { FAN_LED_1_2_REG,                   0b00100000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN2_LED_COLOR]                   = { FAN_LED_1_2_REG,                   0b00010000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN1_LED_ON_OFF]                  = { FAN_LED_1_2_REG,                   0b00001000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN1_LED_BLINKING]                = { FAN_LED_1_2_REG,                   0b00000100,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN1_LED_SPEED]                   = { FAN_LED_1_2_REG,                   0b00000010,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN1_LED_COLOR]                   = { FAN_LED_1_2_REG,                   0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [FAN_LED_3_4]                      = { FAN_LED_3_4_REG,                   MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN4_LED_ON_OFF]                  = { FAN_LED_3_4_REG,                   0b10000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN4_LED_BLINKING]                = { FAN_LED_3_4_REG,                   0b01000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN4_LED_SPEED]                   = { FAN_LED_3_4_REG,                   0b00100000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN4_LED_COLOR]                   = { FAN_LED_3_4_REG,                   0b00010000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN3_LED_ON_OFF]                  = { FAN_LED_3_4_REG,                   0b00001000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN3_LED_BLINKING]                = { FAN_LED_3_4_REG,                   0b00000100,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN3_LED_SPEED]                   = { FAN_LED_3_4_REG,                   0b00000010,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN3_LED_COLOR]                   = { FAN_LED_3_4_REG,                   0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [FAN_LED_5]                        = { FAN_LED_5_REG,                     MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN5_LED_ON_OFF]                  = { FAN_LED_5_REG,                     0b00001000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN5_LED_BLINKING]                = { FAN_LED_5_REG,                     0b00000100,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN5_LED_SPEED]                   = { FAN_LED_5_REG,                     0b00000010,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [FAN5_LED_COLOR]                   = { FAN_LED_5_REG,                     0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    /* Power Status */
    [P5V_AUX_PG]                       = { PWR_STATUS_1_REG,                  0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [P3V3_PG]                          = { PWR_STATUS_1_REG,                  0b00000010,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [P1V8_CLK_PG]                      = { PWR_STATUS_1_REG,                  0b00000100,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [P1V2_CPLD2_PG]                    = { PWR_STATUS_1_REG,                  0b00001000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [P2V5_CPLD2_PG]                    = { PWR_STATUS_1_REG,                  0b00010000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [P1V8_VDD0_PG]                     = { PWR_STATUS_1_REG,                  0b00100000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [VDD_CORE_PG]                      = { PWR_STATUS_1_REG,                  0b01000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [P0V9_PVDD_PG]                     = { PWR_STATUS_1_REG,                  0b10000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [PWR_STATUS_1]                     = { PWR_STATUS_1_REG,                  MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [P0V75_TRVDD100_PG]                = { PWR_STATUS_2_REG,                  0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [P0V75_TRVDD50_PG]                 = { PWR_STATUS_2_REG,                  0b00000010,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [P0V9_TRVDD_PG]                    = { PWR_STATUS_2_REG,                  0b00000100,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [P0V8_AVDD_PG]                     = { PWR_STATUS_2_REG,                  0b00001000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [P1V2_TRVDD_PG]                    = { PWR_STATUS_2_REG,                  0b00010000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [P1V2_PVDD_PG]                     = { PWR_STATUS_2_REG,                  0b00100000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [P1V5_AVDD_PG]                     = { PWR_STATUS_2_REG,                  0b01000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [P1V5_TRXVDD_PG]                   = { PWR_STATUS_2_REG,                  0b10000000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [PWR_STATUS_2]                     = { PWR_STATUS_2_REG,                  MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [P0V75_DDR_VDDC_PG]                = { PWR_STATUS_3_REG,                  0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [P1V05_DDR_VDDQX_PG]               = { PWR_STATUS_3_REG,                  0b00000010,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [P1V8_DDR_PG]                      = { PWR_STATUS_3_REG,                  0b00000100,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [P0V5_DDR_VDDQ_PG]                 = { PWR_STATUS_3_REG,                  0b00001000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [PWR_STATUS_3]                     = { PWR_STATUS_3_REG,                  MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    /* Timing Control */
    [OCXO_ID]                          = { OCXO_GNSS_ID_REG,                  0b00000111,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [GNSS_MODL_ID]                     = { OCXO_GNSS_ID_REG,                  0b01110000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [OCXO_GNSS_ID]                     = { OCXO_GNSS_ID_REG,                  MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [NTM_RESET]                        = { CLK_PTP_RESET_REG,                 0b00000100,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [BITS_RESET]                       = { CLK_PTP_RESET_REG,                 0b00100000,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [CLK_PTP_RESET]                    = { CLK_PTP_RESET_REG,                 MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [TS_PLL_CLK_SOURCE_SEL]            = { CLK_TIMING_CTRL_REG,               0b00000001,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [SMB_1PPS_DIR_SEL]                 = { CLK_TIMING_CTRL_REG,               0b00000100,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [CLK_10M_PTP_IN]                   = { CLK_TIMING_CTRL_REG,               0b00001000,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [SMB_10M_INPUT_EN]                 = { CLK_TIMING_CTRL_REG,               0b00010000,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [PTP_TOD_RS422_DIR_CTRL]           = { CLK_TIMING_CTRL_REG,               0b00100000,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [PTP_1PPS_RS422_DIR_CTRL]          = { CLK_TIMING_CTRL_REG,               0b01000000,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [SMB_10M_DIR_SEL]                  = { CLK_TIMING_CTRL_REG,               0b10000000,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [CLK_TIMING_CTRL]                  = { CLK_TIMING_CTRL_REG,               MASK_ALL,          DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},

    [GNSS_ANT_SHORT]                   = { GNSS_STATUS_REG,                   0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [GNSS_ANT_OPEN]                    = { GNSS_STATUS_REG,                   0b00000010,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [GNSS_ANT_ON]                      = { GNSS_STATUS_REG,                   0b00000100,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [GNSS_STATUS]                      = { GNSS_STATUS_REG,                   MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [NTM_PRESENT]                      = { TIMING_STATUS_REG,                 0b00000001,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [GNSS_PRESENT]                     = { TIMING_STATUS_REG,                 0b00000010,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [GNSS_ANT_PWR_ST]                  = { TIMING_STATUS_REG,                 0b00000100,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [TIMING_STATUS]                    = { TIMING_STATUS_REG,                 MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    // & cpld2
    /* Ports Interrupt Status */
    [ETH_0_PRESENT]                    = { QSFP28_0_3_ABS_REG,                0b00000001,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_1_PRESENT]                    = { QSFP28_0_3_ABS_REG,                0b00000010,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_2_PRESENT]                    = { QSFP28_0_3_ABS_REG,                0b00000100,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_3_PRESENT]                    = { QSFP28_0_3_ABS_REG,                0b00001000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},

    [ETH_4_PRESENT]                    = { SFP28_4_11_ABS_REG,                0b00000001,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_5_PRESENT]                    = { SFP28_4_11_ABS_REG,                0b00000010,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_6_PRESENT]                    = { SFP28_4_11_ABS_REG,                0b00000100,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_7_PRESENT]                    = { SFP28_4_11_ABS_REG,                0b00001000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_8_PRESENT]                    = { SFP28_4_11_ABS_REG,                0b00010000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_9_PRESENT]                    = { SFP28_4_11_ABS_REG,                0b00100000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_10_PRESENT]                   = { SFP28_4_11_ABS_REG,                0b01000000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_11_PRESENT]                   = { SFP28_4_11_ABS_REG,                0b10000000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},

    [ETH_12_PRESENT]                   = { SFP28_12_19_ABS_REG,               0b00000001,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_13_PRESENT]                   = { SFP28_12_19_ABS_REG,               0b00000010,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_14_PRESENT]                   = { SFP28_12_19_ABS_REG,               0b00000100,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_15_PRESENT]                   = { SFP28_12_19_ABS_REG,               0b00001000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_16_PRESENT]                   = { SFP28_12_19_ABS_REG,               0b00010000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_17_PRESENT]                   = { SFP28_12_19_ABS_REG,               0b00100000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_18_PRESENT]                   = { SFP28_12_19_ABS_REG,               0b01000000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_19_PRESENT]                   = { SFP28_12_19_ABS_REG,               0b10000000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},

    [ETH_20_PRESENT]                   = { SFP56_20_27_ABS_REG,               0b00000001,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_21_PRESENT]                   = { SFP56_20_27_ABS_REG,               0b00000010,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_22_PRESENT]                   = { SFP56_20_27_ABS_REG,               0b00000100,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_23_PRESENT]                   = { SFP56_20_27_ABS_REG,               0b00001000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_24_PRESENT]                   = { SFP56_20_27_ABS_REG,               0b00010000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_25_PRESENT]                   = { SFP56_20_27_ABS_REG,               0b00100000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_26_PRESENT]                   = { SFP56_20_27_ABS_REG,               0b01000000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_27_PRESENT]                   = { SFP56_20_27_ABS_REG,               0b10000000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},

    [ETH_0_INTR]                       = { QSFP28_0_3_INTR_REG,               0b00000001,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_1_INTR]                       = { QSFP28_0_3_INTR_REG,               0b00000010,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_2_INTR]                       = { QSFP28_0_3_INTR_REG,               0b00000100,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_3_INTR]                       = { QSFP28_0_3_INTR_REG,               0b00001000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},

    [ETH_4_RX_LOS]                     = { SFP28_4_11_RX_LOS_REG,             0b00000001,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_5_RX_LOS]                     = { SFP28_4_11_RX_LOS_REG,             0b00000010,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_6_RX_LOS]                     = { SFP28_4_11_RX_LOS_REG,             0b00000100,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_7_RX_LOS]                     = { SFP28_4_11_RX_LOS_REG,             0b00001000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_8_RX_LOS]                     = { SFP28_4_11_RX_LOS_REG,             0b00010000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_9_RX_LOS]                     = { SFP28_4_11_RX_LOS_REG,             0b00100000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_10_RX_LOS]                    = { SFP28_4_11_RX_LOS_REG,             0b01000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_11_RX_LOS]                    = { SFP28_4_11_RX_LOS_REG,             0b10000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},

    [ETH_12_RX_LOS]                    = { SFP28_12_19_RX_LOS_REG,            0b00000001,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_13_RX_LOS]                    = { SFP28_12_19_RX_LOS_REG,            0b00000010,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_14_RX_LOS]                    = { SFP28_12_19_RX_LOS_REG,            0b00000100,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_15_RX_LOS]                    = { SFP28_12_19_RX_LOS_REG,            0b00001000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_16_RX_LOS]                    = { SFP28_12_19_RX_LOS_REG,            0b00010000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_17_RX_LOS]                    = { SFP28_12_19_RX_LOS_REG,            0b00100000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_18_RX_LOS]                    = { SFP28_12_19_RX_LOS_REG,            0b01000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_19_RX_LOS]                    = { SFP28_12_19_RX_LOS_REG,            0b10000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},

    [ETH_20_RX_LOS]                    = { SFP56_20_27_RX_LOS_REG,            0b00000001,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_21_RX_LOS]                    = { SFP56_20_27_RX_LOS_REG,            0b00000010,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_22_RX_LOS]                    = { SFP56_20_27_RX_LOS_REG,            0b00000100,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_23_RX_LOS]                    = { SFP56_20_27_RX_LOS_REG,            0b00001000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_24_RX_LOS]                    = { SFP56_20_27_RX_LOS_REG,            0b00010000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_25_RX_LOS]                    = { SFP56_20_27_RX_LOS_REG,            0b00100000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_26_RX_LOS]                    = { SFP56_20_27_RX_LOS_REG,            0b01000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_27_RX_LOS]                    = { SFP56_20_27_RX_LOS_REG,            0b10000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},

    [ETH_0_FUSE]                       = { QSFP28_0_3_FUSE_REG,               0b00000001,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_1_FUSE]                       = { QSFP28_0_3_FUSE_REG,               0b00000010,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_2_FUSE]                       = { QSFP28_0_3_FUSE_REG,               0b00000100,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_3_FUSE]                       = { QSFP28_0_3_FUSE_REG,               0b00001000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},

    [ETH_4_TX_FLT]                     = { SFP28_4_11_TX_FLT_REG,             0b00000001,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_5_TX_FLT]                     = { SFP28_4_11_TX_FLT_REG,             0b00000010,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_6_TX_FLT]                     = { SFP28_4_11_TX_FLT_REG,             0b00000100,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_7_TX_FLT]                     = { SFP28_4_11_TX_FLT_REG,             0b00001000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_8_TX_FLT]                     = { SFP28_4_11_TX_FLT_REG,             0b00010000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_9_TX_FLT]                     = { SFP28_4_11_TX_FLT_REG,             0b00100000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_10_TX_FLT]                    = { SFP28_4_11_TX_FLT_REG,             0b01000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_11_TX_FLT]                    = { SFP28_4_11_TX_FLT_REG,             0b10000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},

    [ETH_12_TX_FLT]                    = { SFP28_12_19_TX_FLT_REG,            0b00000001,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_13_TX_FLT]                    = { SFP28_12_19_TX_FLT_REG,            0b00000010,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_14_TX_FLT]                    = { SFP28_12_19_TX_FLT_REG,            0b00000100,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_15_TX_FLT]                    = { SFP28_12_19_TX_FLT_REG,            0b00001000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_16_TX_FLT]                    = { SFP28_12_19_TX_FLT_REG,            0b00010000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_17_TX_FLT]                    = { SFP28_12_19_TX_FLT_REG,            0b00100000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_18_TX_FLT]                    = { SFP28_12_19_TX_FLT_REG,            0b01000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_19_TX_FLT]                    = { SFP28_12_19_TX_FLT_REG,            0b10000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},

    [ETH_20_TX_FLT]                    = { SFP56_20_27_TX_FLT_REG,            0b00000001,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_21_TX_FLT]                    = { SFP56_20_27_TX_FLT_REG,            0b00000010,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_22_TX_FLT]                    = { SFP56_20_27_TX_FLT_REG,            0b00000100,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_23_TX_FLT]                    = { SFP56_20_27_TX_FLT_REG,            0b00001000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_24_TX_FLT]                    = { SFP56_20_27_TX_FLT_REG,            0b00010000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_25_TX_FLT]                    = { SFP56_20_27_TX_FLT_REG,            0b00100000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_26_TX_FLT]                    = { SFP56_20_27_TX_FLT_REG,            0b01000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_27_TX_FLT]                    = { SFP56_20_27_TX_FLT_REG,            0b10000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    /* Interrupt Mask */

    [ETH_0_PRESENT_MASK]               = { QSFP28_0_3_MASK_ABS_REG,           0b00000001,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_1_PRESENT_MASK]               = { QSFP28_0_3_MASK_ABS_REG,           0b00000010,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_2_PRESENT_MASK]               = { QSFP28_0_3_MASK_ABS_REG,           0b00000100,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_3_PRESENT_MASK]               = { QSFP28_0_3_MASK_ABS_REG,           0b00001000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},

    [ETH_4_PRESENT_MASK]               = { SFP28_4_11_MASK_ABS_REG,           0b00000001,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_5_PRESENT_MASK]               = { SFP28_4_11_MASK_ABS_REG,           0b00000010,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_6_PRESENT_MASK]               = { SFP28_4_11_MASK_ABS_REG,           0b00000100,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_7_PRESENT_MASK]               = { SFP28_4_11_MASK_ABS_REG,           0b00001000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_8_PRESENT_MASK]               = { SFP28_4_11_MASK_ABS_REG,           0b00010000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_9_PRESENT_MASK]               = { SFP28_4_11_MASK_ABS_REG,           0b00100000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_10_PRESENT_MASK]              = { SFP28_4_11_MASK_ABS_REG,           0b01000000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_11_PRESENT_MASK]              = { SFP28_4_11_MASK_ABS_REG,           0b10000000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},

    [ETH_12_PRESENT_MASK]              = { SFP28_12_19_MASK_ABS_REG,          0b00000001,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_13_PRESENT_MASK]              = { SFP28_12_19_MASK_ABS_REG,          0b00000010,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_14_PRESENT_MASK]              = { SFP28_12_19_MASK_ABS_REG,          0b00000100,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_15_PRESENT_MASK]              = { SFP28_12_19_MASK_ABS_REG,          0b00001000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_16_PRESENT_MASK]              = { SFP28_12_19_MASK_ABS_REG,          0b00010000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_17_PRESENT_MASK]              = { SFP28_12_19_MASK_ABS_REG,          0b00100000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_18_PRESENT_MASK]              = { SFP28_12_19_MASK_ABS_REG,          0b01000000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_19_PRESENT_MASK]              = { SFP28_12_19_MASK_ABS_REG,          0b10000000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},

    [ETH_20_PRESENT_MASK]              = { SFP56_20_27_MASK_ABS_REG,          0b00000001,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_21_PRESENT_MASK]              = { SFP56_20_27_MASK_ABS_REG,          0b00000010,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_22_PRESENT_MASK]              = { SFP56_20_27_MASK_ABS_REG,          0b00000100,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_23_PRESENT_MASK]              = { SFP56_20_27_MASK_ABS_REG,          0b00001000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_24_PRESENT_MASK]              = { SFP56_20_27_MASK_ABS_REG,          0b00010000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_25_PRESENT_MASK]              = { SFP56_20_27_MASK_ABS_REG,          0b00100000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_26_PRESENT_MASK]              = { SFP56_20_27_MASK_ABS_REG,          0b01000000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_27_PRESENT_MASK]              = { SFP56_20_27_MASK_ABS_REG,          0b10000000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},

    [ETH_0_INTR_MASK]                  = { QSFP28_0_3_INTR_MASK_REG,          0b00000001,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_1_INTR_MASK]                  = { QSFP28_0_3_INTR_MASK_REG,          0b00000010,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_2_INTR_MASK]                  = { QSFP28_0_3_INTR_MASK_REG,          0b00000100,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_3_INTR_MASK]                  = { QSFP28_0_3_INTR_MASK_REG,          0b00001000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},

    [ETH_4_RX_LOS_MASK]                = { SFP28_4_11_RX_LOS_MASK_REG,        0b00000001,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_5_RX_LOS_MASK]                = { SFP28_4_11_RX_LOS_MASK_REG,        0b00000010,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_6_RX_LOS_MASK]                = { SFP28_4_11_RX_LOS_MASK_REG,        0b00000100,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_7_RX_LOS_MASK]                = { SFP28_4_11_RX_LOS_MASK_REG,        0b00001000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_8_RX_LOS_MASK]                = { SFP28_4_11_RX_LOS_MASK_REG,        0b00010000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_9_RX_LOS_MASK]                = { SFP28_4_11_RX_LOS_MASK_REG,        0b00100000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_10_RX_LOS_MASK]               = { SFP28_4_11_RX_LOS_MASK_REG,        0b01000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_11_RX_LOS_MASK]               = { SFP28_4_11_RX_LOS_MASK_REG,        0b10000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},

    [ETH_12_RX_LOS_MASK]               = { SFP28_12_19_RX_LOS_MASK_REG,       0b00000001,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_13_RX_LOS_MASK]               = { SFP28_12_19_RX_LOS_MASK_REG,       0b00000010,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_14_RX_LOS_MASK]               = { SFP28_12_19_RX_LOS_MASK_REG,       0b00000100,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_15_RX_LOS_MASK]               = { SFP28_12_19_RX_LOS_MASK_REG,       0b00001000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_16_RX_LOS_MASK]               = { SFP28_12_19_RX_LOS_MASK_REG,       0b00010000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_17_RX_LOS_MASK]               = { SFP28_12_19_RX_LOS_MASK_REG,       0b00100000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_18_RX_LOS_MASK]               = { SFP28_12_19_RX_LOS_MASK_REG,       0b01000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_19_RX_LOS_MASK]               = { SFP28_12_19_RX_LOS_MASK_REG,       0b10000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},

    [ETH_20_RX_LOS_MASK]               = { SFP56_20_27_RX_LOS_MASK_REG,       0b00000001,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_21_RX_LOS_MASK]               = { SFP56_20_27_RX_LOS_MASK_REG,       0b00000010,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_22_RX_LOS_MASK]               = { SFP56_20_27_RX_LOS_MASK_REG,       0b00000100,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_23_RX_LOS_MASK]               = { SFP56_20_27_RX_LOS_MASK_REG,       0b00001000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_24_RX_LOS_MASK]               = { SFP56_20_27_RX_LOS_MASK_REG,       0b00010000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_25_RX_LOS_MASK]               = { SFP56_20_27_RX_LOS_MASK_REG,       0b00100000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_26_RX_LOS_MASK]               = { SFP56_20_27_RX_LOS_MASK_REG,       0b01000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_27_RX_LOS_MASK]               = { SFP56_20_27_RX_LOS_MASK_REG,       0b10000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},

    [ETH_0_FUSE_MASK]                  = { QSFP28_0_3_FUSE_MASK_REG,          0b00000001,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_1_FUSE_MASK]                  = { QSFP28_0_3_FUSE_MASK_REG,          0b00000010,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_2_FUSE_MASK]                  = { QSFP28_0_3_FUSE_MASK_REG,          0b00000100,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_3_FUSE_MASK]                  = { QSFP28_0_3_FUSE_MASK_REG,          0b00001000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},

    [ETH_4_TX_FLT_MASK]                = { SFP28_4_11_TX_FLT_MASK_REG,        0b00000001,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_5_TX_FLT_MASK]                = { SFP28_4_11_TX_FLT_MASK_REG,        0b00000010,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_6_TX_FLT_MASK]                = { SFP28_4_11_TX_FLT_MASK_REG,        0b00000100,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_7_TX_FLT_MASK]                = { SFP28_4_11_TX_FLT_MASK_REG,        0b00001000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_8_TX_FLT_MASK]                = { SFP28_4_11_TX_FLT_MASK_REG,        0b00010000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_9_TX_FLT_MASK]                = { SFP28_4_11_TX_FLT_MASK_REG,        0b00100000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_10_TX_FLT_MASK]               = { SFP28_4_11_TX_FLT_MASK_REG,        0b01000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_11_TX_FLT_MASK]               = { SFP28_4_11_TX_FLT_MASK_REG,        0b10000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},

    [ETH_12_TX_FLT_MASK]               = { SFP28_12_19_TX_FLT_MASK_REG,       0b00000001,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_13_TX_FLT_MASK]               = { SFP28_12_19_TX_FLT_MASK_REG,       0b00000010,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_14_TX_FLT_MASK]               = { SFP28_12_19_TX_FLT_MASK_REG,       0b00000100,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_15_TX_FLT_MASK]               = { SFP28_12_19_TX_FLT_MASK_REG,       0b00001000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_16_TX_FLT_MASK]               = { SFP28_12_19_TX_FLT_MASK_REG,       0b00010000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_17_TX_FLT_MASK]               = { SFP28_12_19_TX_FLT_MASK_REG,       0b00100000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_18_TX_FLT_MASK]               = { SFP28_12_19_TX_FLT_MASK_REG,       0b01000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_19_TX_FLT_MASK]               = { SFP28_12_19_TX_FLT_MASK_REG,       0b10000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},

    [ETH_20_TX_FLT_MASK]               = { SFP56_20_27_TX_FLT_MASK_REG,       0b00000001,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_21_TX_FLT_MASK]               = { SFP56_20_27_TX_FLT_MASK_REG,       0b00000010,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_22_TX_FLT_MASK]               = { SFP56_20_27_TX_FLT_MASK_REG,       0b00000100,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_23_TX_FLT_MASK]               = { SFP56_20_27_TX_FLT_MASK_REG,       0b00001000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_24_TX_FLT_MASK]               = { SFP56_20_27_TX_FLT_MASK_REG,       0b00010000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_25_TX_FLT_MASK]               = { SFP56_20_27_TX_FLT_MASK_REG,       0b00100000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_26_TX_FLT_MASK]               = { SFP56_20_27_TX_FLT_MASK_REG,       0b01000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_27_TX_FLT_MASK]               = { SFP56_20_27_TX_FLT_MASK_REG,       0b10000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},

    /* Interrupt Event */
    [QSFP28_0_3_PRESENT_EVENT]         = { QSFP28_0_3_ABS_EVENT_REG,          MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},
    [SFP28_4_11_PRESENT_EVENT]         = { SFP28_4_11_ABS_EVENT_REG,          MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},
    [SFP28_12_19_PRESENT_EVENT]        = { SFP28_12_19_ABS_EVENT_REG,         MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},
    [SFP56_20_27_PRESENT_EVENT]        = { SFP56_20_27_ABS_EVENT_REG,         MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},
    [QSFP28_0_3_INTR_EVENT]            = { QSFP28_0_3_INTR_EVENT_REG,         MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},
    [SFP28_4_11_RX_LOS_EVENT]          = { SFP28_4_11_RX_LOS_EVENT_REG,       MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},
    [SFP28_12_19_RX_LOS_EVENT]         = { SFP28_12_19_RX_LOS_EVENT_REG,      MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},
    [SFP56_20_27_RX_LOS_EVENT]         = { SFP56_20_27_RX_LOS_EVENT_REG,      MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},
    [QSFP28_0_3_FUSE_EVENT]            = { QSFP28_0_3_FUSE_EVENT_REG,         MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},
    [SFP28_4_11_TX_FLT_EVENT]          = { SFP28_4_11_TX_FLT_EVENT_REG,       MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},
    [SFP28_12_19_TX_FLT_EVENT]         = { SFP28_12_19_TX_FLT_EVENT_REG,      MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},
    [SFP56_20_27_TX_FLT_EVENT]         = { SFP56_20_27_TX_FLT_EVENT_REG,      MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},

    [ETH_0_PRESENT_EVENT]              = {QSFP28_0_3_ABS_EVENT_REG  ,         0b00000001,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_1_PRESENT_EVENT]              = {QSFP28_0_3_ABS_EVENT_REG  ,         0b00000010,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_2_PRESENT_EVENT]              = {QSFP28_0_3_ABS_EVENT_REG  ,         0b00000100,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_3_PRESENT_EVENT]              = {QSFP28_0_3_ABS_EVENT_REG  ,         0b00001000,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_4_PRESENT_EVENT]              = {SFP28_4_11_ABS_EVENT_REG  ,         0b00000001,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_5_PRESENT_EVENT]              = {SFP28_4_11_ABS_EVENT_REG  ,         0b00000010,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_6_PRESENT_EVENT]              = {SFP28_4_11_ABS_EVENT_REG  ,         0b00000100,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_7_PRESENT_EVENT]              = {SFP28_4_11_ABS_EVENT_REG  ,         0b00001000,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_8_PRESENT_EVENT]              = {SFP28_4_11_ABS_EVENT_REG  ,         0b00010000,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_9_PRESENT_EVENT]              = {SFP28_4_11_ABS_EVENT_REG  ,         0b00100000,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_10_PRESENT_EVENT]             = {SFP28_4_11_ABS_EVENT_REG  ,         0b01000000,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_11_PRESENT_EVENT]             = {SFP28_4_11_ABS_EVENT_REG  ,         0b10000000,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_12_PRESENT_EVENT]             = {SFP28_12_19_ABS_EVENT_REG ,         0b00000001,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_13_PRESENT_EVENT]             = {SFP28_12_19_ABS_EVENT_REG ,         0b00000010,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_14_PRESENT_EVENT]             = {SFP28_12_19_ABS_EVENT_REG ,         0b00000100,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_15_PRESENT_EVENT]             = {SFP28_12_19_ABS_EVENT_REG ,         0b00001000,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_16_PRESENT_EVENT]             = {SFP28_12_19_ABS_EVENT_REG ,         0b00010000,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_17_PRESENT_EVENT]             = {SFP28_12_19_ABS_EVENT_REG ,         0b00100000,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_18_PRESENT_EVENT]             = {SFP28_12_19_ABS_EVENT_REG ,         0b01000000,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_19_PRESENT_EVENT]             = {SFP28_12_19_ABS_EVENT_REG ,         0b10000000,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_20_PRESENT_EVENT]             = {SFP56_20_27_ABS_EVENT_REG ,         0b00000001,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_21_PRESENT_EVENT]             = {SFP56_20_27_ABS_EVENT_REG ,         0b00000010,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_22_PRESENT_EVENT]             = {SFP56_20_27_ABS_EVENT_REG ,         0b00000100,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_23_PRESENT_EVENT]             = {SFP56_20_27_ABS_EVENT_REG ,         0b00001000,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_24_PRESENT_EVENT]             = {SFP56_20_27_ABS_EVENT_REG ,         0b00010000,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_25_PRESENT_EVENT]             = {SFP56_20_27_ABS_EVENT_REG ,         0b00100000,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_26_PRESENT_EVENT]             = {SFP56_20_27_ABS_EVENT_REG ,         0b01000000,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},
    [ETH_27_PRESENT_EVENT]             = {SFP56_20_27_ABS_EVENT_REG ,         0b10000000,        DATA_0_1,      REG_WP_DIS, REG_IS_EVENT},

    /* Reset and Control */
    [ETH_0_RESET]                      = { QSFP28_0_3_RESET_REG,              0b00000001,        DATA_0_1_INV,  REG_WP_EN , REG_NOT_EVENT},
    [ETH_1_RESET]                      = { QSFP28_0_3_RESET_REG,              0b00000010,        DATA_0_1_INV,  REG_WP_EN , REG_NOT_EVENT},
    [ETH_2_RESET]                      = { QSFP28_0_3_RESET_REG,              0b00000100,        DATA_0_1_INV,  REG_WP_EN , REG_NOT_EVENT},
    [ETH_3_RESET]                      = { QSFP28_0_3_RESET_REG,              0b00001000,        DATA_0_1_INV,  REG_WP_EN , REG_NOT_EVENT},

    [ETH_0_LPMODE]                     = { QSFP28_0_3_LPMODE_REG,             0b00000001,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_1_LPMODE]                     = { QSFP28_0_3_LPMODE_REG,             0b00000010,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_2_LPMODE]                     = { QSFP28_0_3_LPMODE_REG,             0b00000100,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_3_LPMODE]                     = { QSFP28_0_3_LPMODE_REG,             0b00001000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},

    [SSD1_M2_CONFIG]                   = { SSD_CTRL_REG,                      0b00000100,        DATA_HEX,      REG_WP_EN , REG_NOT_EVENT},
    [SSD2_M2_CONFIG]                   = { SSD_CTRL_REG,                      0b00001000,        DATA_HEX,      REG_WP_EN , REG_NOT_EVENT},
    [CPU_SSD1_PERST]                   = { SSD_CTRL_REG,                      0b00010000,        DATA_HEX,      REG_WP_EN , REG_NOT_EVENT},
    [CPU_SSD2_PERST]                   = { SSD_CTRL_REG,                      0b00100000,        DATA_HEX,      REG_WP_EN , REG_NOT_EVENT},
    [PWREN_P3V3_SSD1]                  = { SSD_CTRL_REG,                      0b01000000,        DATA_HEX,      REG_WP_EN , REG_NOT_EVENT},
    [PWREN_P3V3_SSD2]                  = { SSD_CTRL_REG,                      0b10000000,        DATA_HEX,      REG_WP_EN , REG_NOT_EVENT},

    [ETH_4_TX_DISABLE]                 = { SFP28_4_11_TX_DISABLE_REG,         0b00000001,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_5_TX_DISABLE]                 = { SFP28_4_11_TX_DISABLE_REG,         0b00000010,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_6_TX_DISABLE]                 = { SFP28_4_11_TX_DISABLE_REG,         0b00000100,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_7_TX_DISABLE]                 = { SFP28_4_11_TX_DISABLE_REG,         0b00001000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_8_TX_DISABLE]                 = { SFP28_4_11_TX_DISABLE_REG,         0b00010000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_9_TX_DISABLE]                 = { SFP28_4_11_TX_DISABLE_REG,         0b00100000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_10_TX_DISABLE]                = { SFP28_4_11_TX_DISABLE_REG,         0b01000000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_11_TX_DISABLE]                = { SFP28_4_11_TX_DISABLE_REG,         0b10000000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},

    [ETH_12_TX_DISABLE]                = { SFP28_12_19_TX_DISABLE_REG,        0b00000001,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_13_TX_DISABLE]                = { SFP28_12_19_TX_DISABLE_REG,        0b00000010,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_14_TX_DISABLE]                = { SFP28_12_19_TX_DISABLE_REG,        0b00000100,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_15_TX_DISABLE]                = { SFP28_12_19_TX_DISABLE_REG,        0b00001000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_16_TX_DISABLE]                = { SFP28_12_19_TX_DISABLE_REG,        0b00010000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_17_TX_DISABLE]                = { SFP28_12_19_TX_DISABLE_REG,        0b00100000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_18_TX_DISABLE]                = { SFP28_12_19_TX_DISABLE_REG,        0b01000000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_19_TX_DISABLE]                = { SFP28_12_19_TX_DISABLE_REG,        0b10000000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},

    [ETH_20_TX_DISABLE]                = { SFP56_20_27_TX_DISABLE_REG,        0b00000001,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_21_TX_DISABLE]                = { SFP56_20_27_TX_DISABLE_REG,        0b00000010,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_22_TX_DISABLE]                = { SFP56_20_27_TX_DISABLE_REG,        0b00000100,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_23_TX_DISABLE]                = { SFP56_20_27_TX_DISABLE_REG,        0b00001000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_24_TX_DISABLE]                = { SFP56_20_27_TX_DISABLE_REG,        0b00010000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_25_TX_DISABLE]                = { SFP56_20_27_TX_DISABLE_REG,        0b00100000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_26_TX_DISABLE]                = { SFP56_20_27_TX_DISABLE_REG,        0b01000000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_27_TX_DISABLE]                = { SFP56_20_27_TX_DISABLE_REG,        0b10000000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},

    [ETH_4_RATE_SELECT]                = { SFP28_4_11_RATE_SELECT_REG,        0b00000001,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_5_RATE_SELECT]                = { SFP28_4_11_RATE_SELECT_REG,        0b00000010,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_6_RATE_SELECT]                = { SFP28_4_11_RATE_SELECT_REG,        0b00000100,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_7_RATE_SELECT]                = { SFP28_4_11_RATE_SELECT_REG,        0b00001000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_8_RATE_SELECT]                = { SFP28_4_11_RATE_SELECT_REG,        0b00010000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_9_RATE_SELECT]                = { SFP28_4_11_RATE_SELECT_REG,        0b00100000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_10_RATE_SELECT]               = { SFP28_4_11_RATE_SELECT_REG,        0b01000000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_11_RATE_SELECT]               = { SFP28_4_11_RATE_SELECT_REG,        0b10000000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},

    [ETH_12_RATE_SELECT]               = { SFP28_12_19_RATE_SELECT_REG,       0b00000001,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_13_RATE_SELECT]               = { SFP28_12_19_RATE_SELECT_REG,       0b00000010,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_14_RATE_SELECT]               = { SFP28_12_19_RATE_SELECT_REG,       0b00000100,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_15_RATE_SELECT]               = { SFP28_12_19_RATE_SELECT_REG,       0b00001000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_16_RATE_SELECT]               = { SFP28_12_19_RATE_SELECT_REG,       0b00010000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_17_RATE_SELECT]               = { SFP28_12_19_RATE_SELECT_REG,       0b00100000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_18_RATE_SELECT]               = { SFP28_12_19_RATE_SELECT_REG,       0b01000000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_19_RATE_SELECT]               = { SFP28_12_19_RATE_SELECT_REG,       0b10000000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},

    [ETH_20_RATE_SELECT]               = { SFP56_20_27_RATE_SELECT_REG,       0b00000001,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_21_RATE_SELECT]               = { SFP56_20_27_RATE_SELECT_REG,       0b00000010,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_22_RATE_SELECT]               = { SFP56_20_27_RATE_SELECT_REG,       0b00000100,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_23_RATE_SELECT]               = { SFP56_20_27_RATE_SELECT_REG,       0b00001000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_24_RATE_SELECT]               = { SFP56_20_27_RATE_SELECT_REG,       0b00010000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_25_RATE_SELECT]               = { SFP56_20_27_RATE_SELECT_REG,       0b00100000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_26_RATE_SELECT]               = { SFP56_20_27_RATE_SELECT_REG,       0b01000000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    [ETH_27_RATE_SELECT]               = { SFP56_20_27_RATE_SELECT_REG,       0b10000000,        DATA_0_1,      REG_WP_EN , REG_NOT_EVENT},
    /* Status */
    [USB_CTRL_PRESENT]                 = { USB_CTRL_REG,                      0b00000001,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [NVME_SSD2_PRESENT]                = { SSD_STATUS_REG,                    0b00000010,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [NVME_SSD1_PRESENT]                = { SSD_STATUS_REG,                    0b00000001,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},

    [ETH_4_5_OC]                       = { SFP28_4_19_OC_REG,                 0b00000001,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_6_7_OC]                       = { SFP28_4_19_OC_REG,                 0b00000010,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_8_9_OC]                       = { SFP28_4_19_OC_REG,                 0b00000100,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_10_11_OC]                     = { SFP28_4_19_OC_REG,                 0b00001000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_12_13_OC]                     = { SFP28_4_19_OC_REG,                 0b00010000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_14_15_OC]                     = { SFP28_4_19_OC_REG,                 0b00100000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_16_17_OC]                     = { SFP28_4_19_OC_REG,                 0b01000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_18_19_OC]                     = { SFP28_4_19_OC_REG,                 0b10000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},

    [ETH_20_OC]                        = { SFP56_20_27_OC_REG,                0b00000001,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_21_OC]                        = { SFP56_20_27_OC_REG,                0b00000010,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_22_OC]                        = { SFP56_20_27_OC_REG,                0b00000100,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_23_OC]                        = { SFP56_20_27_OC_REG,                0b00001000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_24_OC]                        = { SFP56_20_27_OC_REG,                0b00010000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_25_OC]                        = { SFP56_20_27_OC_REG,                0b00100000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_26_OC]                        = { SFP56_20_27_OC_REG,                0b01000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_27_OC]                        = { SFP56_20_27_OC_REG,                0b10000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},

    [ETH_0_I2C_STUCK]                  = { QSFP28_0_3_I2C_STUCK_STATUS_REG,   0b00000001,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_1_I2C_STUCK]                  = { QSFP28_0_3_I2C_STUCK_STATUS_REG,   0b00000010,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_2_I2C_STUCK]                  = { QSFP28_0_3_I2C_STUCK_STATUS_REG,   0b00000100,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_3_I2C_STUCK]                  = { QSFP28_0_3_I2C_STUCK_STATUS_REG,   0b00001000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},

    [ETH_4_I2C_STUCK]                  = { SFP28_4_11_I2C_STUCK_STATUS_REG,   0b00000001,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_5_I2C_STUCK]                  = { SFP28_4_11_I2C_STUCK_STATUS_REG,   0b00000010,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_6_I2C_STUCK]                  = { SFP28_4_11_I2C_STUCK_STATUS_REG,   0b00000100,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_7_I2C_STUCK]                  = { SFP28_4_11_I2C_STUCK_STATUS_REG,   0b00001000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_8_I2C_STUCK]                  = { SFP28_4_11_I2C_STUCK_STATUS_REG,   0b00010000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_9_I2C_STUCK]                  = { SFP28_4_11_I2C_STUCK_STATUS_REG,   0b00100000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_10_I2C_STUCK]                 = { SFP28_4_11_I2C_STUCK_STATUS_REG,   0b01000000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_11_I2C_STUCK]                 = { SFP28_4_11_I2C_STUCK_STATUS_REG,   0b10000000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},

    [ETH_12_I2C_STUCK]                 = { SFP28_12_19_I2C_STUCK_STATUS_REG,  0b00000001,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_13_I2C_STUCK]                 = { SFP28_12_19_I2C_STUCK_STATUS_REG,  0b00000010,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_14_I2C_STUCK]                 = { SFP28_12_19_I2C_STUCK_STATUS_REG,  0b00000100,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_15_I2C_STUCK]                 = { SFP28_12_19_I2C_STUCK_STATUS_REG,  0b00001000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_16_I2C_STUCK]                 = { SFP28_12_19_I2C_STUCK_STATUS_REG,  0b00010000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_17_I2C_STUCK]                 = { SFP28_12_19_I2C_STUCK_STATUS_REG,  0b00100000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_18_I2C_STUCK]                 = { SFP28_12_19_I2C_STUCK_STATUS_REG,  0b01000000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_19_I2C_STUCK]                 = { SFP28_12_19_I2C_STUCK_STATUS_REG,  0b10000000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},

    [ETH_20_I2C_STUCK]                 = { SFP56_20_27_I2C_STUCK_STATUS_REG,  0b00000001,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_21_I2C_STUCK]                 = { SFP56_20_27_I2C_STUCK_STATUS_REG,  0b00000010,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_22_I2C_STUCK]                 = { SFP56_20_27_I2C_STUCK_STATUS_REG,  0b00000100,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_23_I2C_STUCK]                 = { SFP56_20_27_I2C_STUCK_STATUS_REG,  0b00001000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_24_I2C_STUCK]                 = { SFP56_20_27_I2C_STUCK_STATUS_REG,  0b00010000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_25_I2C_STUCK]                 = { SFP56_20_27_I2C_STUCK_STATUS_REG,  0b00100000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_26_I2C_STUCK]                 = { SFP56_20_27_I2C_STUCK_STATUS_REG,  0b01000000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},
    [ETH_27_I2C_STUCK]                 = { SFP56_20_27_I2C_STUCK_STATUS_REG,  0b10000000,        DATA_0_1_INV,  REG_WP_DIS, REG_NOT_EVENT},

    [CPU_LEGACY_SIDE_I2C_STUCK]        = { CPU_I2C_STUCK_STATUS_REG,          0b00000001,        DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},
    [CPU_I2C_STUCK_STATUS]             = { CPU_I2C_STUCK_STATUS_REG,          MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_NOT_EVENT},

    [ETH_0_I2C_STUCK_MASK]             = { QSFP28_0_3_I2C_STUCK_MASK_REG,     0b00000001,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_1_I2C_STUCK_MASK]             = { QSFP28_0_3_I2C_STUCK_MASK_REG,     0b00000010,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_2_I2C_STUCK_MASK]             = { QSFP28_0_3_I2C_STUCK_MASK_REG,     0b00000100,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_3_I2C_STUCK_MASK]             = { QSFP28_0_3_I2C_STUCK_MASK_REG,     0b00001000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},

    [ETH_4_I2C_STUCK_MASK]             = { SFP28_4_11_I2C_STUCK_MASK_REG,     0b00000001,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_5_I2C_STUCK_MASK]             = { SFP28_4_11_I2C_STUCK_MASK_REG,     0b00000010,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_6_I2C_STUCK_MASK]             = { SFP28_4_11_I2C_STUCK_MASK_REG,     0b00000100,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_7_I2C_STUCK_MASK]             = { SFP28_4_11_I2C_STUCK_MASK_REG,     0b00001000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_8_I2C_STUCK_MASK]             = { SFP28_4_11_I2C_STUCK_MASK_REG,     0b00010000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_9_I2C_STUCK_MASK]             = { SFP28_4_11_I2C_STUCK_MASK_REG,     0b00100000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_10_I2C_STUCK_MASK]            = { SFP28_4_11_I2C_STUCK_MASK_REG,     0b01000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_11_I2C_STUCK_MASK]            = { SFP28_4_11_I2C_STUCK_MASK_REG,     0b10000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},

    [ETH_12_I2C_STUCK_MASK]            = { SFP28_12_19_I2C_STUCK_MASK_REG,    0b00000001,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_13_I2C_STUCK_MASK]            = { SFP28_12_19_I2C_STUCK_MASK_REG,    0b00000010,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_14_I2C_STUCK_MASK]            = { SFP28_12_19_I2C_STUCK_MASK_REG,    0b00000100,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_15_I2C_STUCK_MASK]            = { SFP28_12_19_I2C_STUCK_MASK_REG,    0b00001000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_16_I2C_STUCK_MASK]            = { SFP28_12_19_I2C_STUCK_MASK_REG,    0b00010000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_17_I2C_STUCK_MASK]            = { SFP28_12_19_I2C_STUCK_MASK_REG,    0b00100000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_18_I2C_STUCK_MASK]            = { SFP28_12_19_I2C_STUCK_MASK_REG,    0b01000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_19_I2C_STUCK_MASK]            = { SFP28_12_19_I2C_STUCK_MASK_REG,    0b10000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},

    [ETH_20_I2C_STUCK_MASK]            = { SFP56_20_27_I2C_STUCK_MASK_REG,    0b00000001,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_21_I2C_STUCK_MASK]            = { SFP56_20_27_I2C_STUCK_MASK_REG,    0b00000010,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_22_I2C_STUCK_MASK]            = { SFP56_20_27_I2C_STUCK_MASK_REG,    0b00000100,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_23_I2C_STUCK_MASK]            = { SFP56_20_27_I2C_STUCK_MASK_REG,    0b00001000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_24_I2C_STUCK_MASK]            = { SFP56_20_27_I2C_STUCK_MASK_REG,    0b00010000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_25_I2C_STUCK_MASK]            = { SFP56_20_27_I2C_STUCK_MASK_REG,    0b00100000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_26_I2C_STUCK_MASK]            = { SFP56_20_27_I2C_STUCK_MASK_REG,    0b01000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},
    [ETH_27_I2C_STUCK_MASK]            = { SFP56_20_27_I2C_STUCK_MASK_REG,    0b10000000,        DATA_0_1,      REG_WP_DIS, REG_NOT_EVENT},

    [CPU_LEGACY_SIDE_I2C_STUCK_MASK]   = { CPU_I2C_STUCK_MASK_REG,            0b00000001,        DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [CPU_I2C_STUCK_MASK]               = { CPU_I2C_STUCK_MASK_REG,            MASK_ALL,          DATA_HEX,      REG_WP_DIS, REG_NOT_EVENT},
    [QSFP28_0_3_I2C_STUCK_EVENT]       = { QSFP28_0_3_I2C_STUCK_EVENT_REG,    MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},
    [SFP28_4_11_I2C_STUCK_EVENT]       = { SFP28_4_11_I2C_STUCK_EVENT_REG,    MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},
    [SFP28_12_19_I2C_STUCK_EVENT]      = { SFP28_12_19_I2C_STUCK_EVENT_REG,   MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},
    [SFP56_20_27_I2C_STUCK_EVENT]      = { SFP56_20_27_I2C_STUCK_EVENT_REG,   MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},
    [CPU_I2C_STUCK_EVENT]              = { CPU_I2C_STUCK_EVENT_REG,           MASK_ALL,          DATA_DEC,      REG_WP_DIS, REG_IS_EVENT},


    // MUX
    [IDLE_STATE]                       =  {NONE_REG,                          MASK_NONE,         DATA_UNK,      REG_WP_DIS, REG_NOT_EVENT},
    /******************************************************************************
    * BSP DEBUG                                                                    *
    ******************************************************************************/
    //BSP DEBUG
    [BSP_DEBUG]                        = { NONE_REG,                          MASK_NONE,         DATA_UNK,      REG_WP_DIS, REG_NOT_EVENT},
    [BSP_WP_ACCESS_COUNT]              = { NONE_REG,                          MASK_NONE,         DATA_UNK,      REG_WP_DIS, REG_NOT_EVENT},
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
    { "s9520_28xc_cpld1",  cpld1 },
    { "s9520_28xc_cpld2",  cpld2 },
    {}
};

static unsigned int wp_access_count = 0;

char bsp_debug[2]="0";
u8 enable_log_read=LOG_DISABLE;
u8 enable_log_write=LOG_DISABLE;

/* Addresses scanned for cpld */
static const unsigned short cpld_i2c_addr[] = { 0x32, 0x33, I2C_CLIENT_END };

/* define all support register access of cpld in attribute */

/* --- Sensor Attributes --- */
/* --- Sensor Device Attributes (RO/RW) --- */
/* CPLD common information registers */
// & cpld_common

static SENSOR_DEVICE_ATTR_RO(cpld_version,                    cpld,      CPLD_VERSION);
static SENSOR_DEVICE_ATTR_RO(cpld_minor_ver,                  cpld,      CPLD_MINOR_VER);
static SENSOR_DEVICE_ATTR_RO(cpld_major_ver,                  cpld,      CPLD_MAJOR_VER);

static SENSOR_DEVICE_ATTR_RO(cpld_id,                         cpld,      CPLD_ID);

static SENSOR_DEVICE_ATTR_RO(cpld_build_ver,                  cpld,      CPLD_BUILD_VER);
static SENSOR_DEVICE_ATTR_RO(cpld_version_h,                  version_h, CPLD_VERSION_H);

static SENSOR_DEVICE_ATTR_RO(cpld_chip_type,                  cpld,      CPLD_CHIP_TYPE);

static SENSOR_DEVICE_ATTR_RW(event_ctrl,                      cpld,      EVENT_CTRL);
static SENSOR_DEVICE_ATTR_RW(event_detect_ctrl,               cpld,      EVENT_DETECT_CTRL);

static SENSOR_DEVICE_ATTR_RW(remote_i2c_reset,                cpld,      REMOTE_I2C_RESET);
static SENSOR_DEVICE_ATTR_RW(module_reset,                    cpld,      MODULE_RESET);

// & cpld1
/* Board information */

static SENSOR_DEVICE_ATTR_RO(cpld_sku_id,                     cpld,      CPLD_SKU_ID);

static SENSOR_DEVICE_ATTR_RO(brd_hw_id,                       cpld,      BRD_HW_ID);
static SENSOR_DEVICE_ATTR_RO(deph_id,                         cpld,      DEPH_ID);
static SENSOR_DEVICE_ATTR_RO(build_id,                        cpld,      BUILD_ID);
static SENSOR_DEVICE_ATTR_RO(bit_sel_id,                      cpld,      BIT_SEL_ID);
static SENSOR_DEVICE_ATTR_RO(hw_build_rev,                    cpld,      HW_BUILD_REV);

static SENSOR_DEVICE_ATTR_RO(ext_id,                          cpld,      EXT_ID);
static SENSOR_DEVICE_ATTR_RO(cpld_ext_id,                     cpld,      CPLD_EXT_ID);
/* Interrupt Status */

static SENSOR_DEVICE_ATTR_RO(cja_lol_intr,                    cpld,      CJA_LOL_INTR);
static SENSOR_DEVICE_ATTR_RO(ntm_intr,                        cpld,      NTM_INTR);
static SENSOR_DEVICE_ATTR_RO(dpll_1588_intr,                  cpld,      DPLL_1588_INTR);
static SENSOR_DEVICE_ATTR_RO(dpll_sync_intr,                  cpld,      DPLL_SYNC_INTR);
static SENSOR_DEVICE_ATTR_RO(bits_intr,                       cpld,      BITS_INTR);

static SENSOR_DEVICE_ATTR_RO(i2c_nic_alrt,                    cpld,      I2C_NIC_ALRT);

static SENSOR_DEVICE_ATTR_RO(psu1_intr,                       cpld,      PSU1_INTR);
static SENSOR_DEVICE_ATTR_RO(psu2_intr,                       cpld,      PSU2_INTR);
static SENSOR_DEVICE_ATTR_RO(vdd_core_pin_alrt_intr,          cpld,      VDD_CORE_PIN_ALRT_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld2_intr,                      cpld,      CPLD2_INTR);
static SENSOR_DEVICE_ATTR_RO(fan_card_intr,                   cpld,      FAN_CARD_INTR);

static SENSOR_DEVICE_ATTR_RO(usb_pwr_oc,                      cpld,      USB_PWR_OC);
static SENSOR_DEVICE_ATTR_RO(ssd1_pwr_oc,                     cpld,      SSD1_PWR_OC);
static SENSOR_DEVICE_ATTR_RO(ssd2_pwr_oc,                     cpld,      SSD2_PWR_OC);
static SENSOR_DEVICE_ATTR_RO(gnss_intr,                       cpld,      GNSS_INTR);
static SENSOR_DEVICE_ATTR_RO(hwm_nmi_intr,                    cpld,      HWM_NMI_INTR);

static SENSOR_DEVICE_ATTR_RO(tsen_nmi_intr,                   cpld,      TSEN_NMI_INTR);
static SENSOR_DEVICE_ATTR_RO(tsen1_nmi_intr,                  cpld,      TSEN1_NMI_INTR);
static SENSOR_DEVICE_ATTR_RO(tsen2_nmi_intr,                  cpld,      TSEN2_NMI_INTR);
static SENSOR_DEVICE_ATTR_RO(tsen_alrt_intr,                  cpld,      TSEN_ALRT_INTR);
static SENSOR_DEVICE_ATTR_RO(tsen1_alrt_intr,                 cpld,      TSEN1_ALRT_INTR);
static SENSOR_DEVICE_ATTR_RO(tsen2_alrt_intr,                 cpld,      TSEN2_ALRT_INTR);
static SENSOR_DEVICE_ATTR_RO(cpu_thrmtrip,                    cpld,      CPU_THRMTRIP);
static SENSOR_DEVICE_ATTR_RO(vdd_core_vrhot,                  cpld,      VDD_CORE_VRHOT);
static SENSOR_DEVICE_ATTR_RO(thermal_intr,                    cpld,      THERMAL_INTR);

static SENSOR_DEVICE_ATTR_RO(mac_intr,                        cpld,      MAC_INTR);

static SENSOR_DEVICE_ATTR_RO(cpu_nmi_intr,                    cpld,      CPU_NMI_INTR);

static SENSOR_DEVICE_ATTR_RO(usb_pwr_oc_cpu,                  cpld,      USB_PWR_OC_CPU);
static SENSOR_DEVICE_ATTR_RO(nmi_cpld_to_cpu,                 cpld,      NMI_CPLD_TO_CPU);
static SENSOR_DEVICE_ATTR_RO(ptp_to_cpu_intr,                 cpld,      PTP_TO_CPU_INTR);
static SENSOR_DEVICE_ATTR_RO(eth_to_cpu_intr,                 cpld,      ETH_TO_CPU_INTR);
static SENSOR_DEVICE_ATTR_RO(thrm_cpld_to_cpu_intr,           cpld,      THRM_CPLD_TO_CPU_INTR);
static SENSOR_DEVICE_ATTR_RO(nmi_sys_to_bmc,                  cpld,      NMI_SYS_TO_BMC);
static SENSOR_DEVICE_ATTR_RO(thrm_chip_intr,                  cpld,      THRM_CHIP_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_to_cpu_intr,                cpld,      CPLD_TO_CPU_INTR);
/* Interrupt Mask */

static SENSOR_DEVICE_ATTR_RW(bits_intr_mask,                  cpld,      BITS_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(dpll_sync_intr_mask,             cpld,      DPLL_SYNC_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(dpll_1588_intr_mask,             cpld,      DPLL_1588_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(ntm_intr_mask,                   cpld,      NTM_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(cja_lol_intr_mask,               cpld,      CJA_LOL_INTR_MASK);

static SENSOR_DEVICE_ATTR_RW(i2c_nic_alrt_mask,               cpld,      I2C_NIC_ALRT_MASK);

static SENSOR_DEVICE_ATTR_RW(fan_card_intr_mask,              cpld,      FAN_CARD_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld2_intr_mask,                 cpld,      CPLD2_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(psu2_intr_mask,                  cpld,      PSU2_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(psu1_intr_mask,                  cpld,      PSU1_INTR_MASK);

static SENSOR_DEVICE_ATTR_RW(hwm_nmi_intr_mask,               cpld,      HWM_NMI_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(gnss_intr_mask,                  cpld,      GNSS_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(ssd2_pwr_oc_mask,                cpld,      SSD2_PWR_OC_MASK);
static SENSOR_DEVICE_ATTR_RW(ssd1_pwr_oc_mask,                cpld,      SSD1_PWR_OC_MASK);
static SENSOR_DEVICE_ATTR_RW(usb_pwr_oc_mask,                 cpld,      USB_PWR_OC_MASK);

static SENSOR_DEVICE_ATTR_RW(vdd_core_vrhot_mask,             cpld,      VDD_CORE_VRHOT_MASK);
static SENSOR_DEVICE_ATTR_RW(cpu_thrmtrip_mask,               cpld,      CPU_THRMTRIP_MASK);
static SENSOR_DEVICE_ATTR_RW(tsen2_alrt_intr_mask,            cpld,      TSEN2_ALRT_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(tsen1_alrt_intr_mask,            cpld,      TSEN1_ALRT_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(tsen_alrt_intr_mask,             cpld,      TSEN_ALRT_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(tsen2_nmi_intr_mask,             cpld,      TSEN2_NMI_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(tsen1_nmi_intr_mask,             cpld,      TSEN1_NMI_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(tsen_nmi_intr_mask,              cpld,      TSEN_NMI_INTR_MASK);

static SENSOR_DEVICE_ATTR_RW(mac_intr_mask,                   cpld,      MAC_INTR_MASK);

static SENSOR_DEVICE_ATTR_RW(cpu_nmi_intr_mask,               cpld,      CPU_NMI_INTR_MASK);

static SENSOR_DEVICE_ATTR_RW(cpld_to_cpu_intr_mask,           cpld,      CPLD_TO_CPU_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(thrm_chip_intr_mask,             cpld,      THRM_CHIP_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(nmi_sys_to_bmc_mask,             cpld,      NMI_SYS_TO_BMC_MASK);
static SENSOR_DEVICE_ATTR_RW(thrm_cpld_to_cpu_intr_mask,      cpld,      THRM_CPLD_TO_CPU_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_to_cpu_intr_mask,            cpld,      ETH_TO_CPU_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(ptp_to_cpu_intr_mask,            cpld,      PTP_TO_CPU_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(nmi_cpld_to_cpu_mask,            cpld,      NMI_CPLD_TO_CPU_MASK);
static SENSOR_DEVICE_ATTR_RW(usb_pwr_oc_cpu_mask,             cpld,      USB_PWR_OC_CPU_MASK);

/* Interrupt Event */
static SENSOR_DEVICE_ATTR_RO(clk_ptp_intr_event,              cpld,      CLK_PTP_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(phy_intr_event,                  cpld,      PHY_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(top_brd_cpld_fru_intr_event,     cpld,      TOP_BRD_CPLD_FRU_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(main_brd_cpld_intr_event,        cpld,      MAIN_BRD_CPLD_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(thermal_intr_event,              cpld,      THERMAL_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(mac_intr_event,                  cpld,      MAC_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpu_nmi_intr_event,              cpld,      CPU_NMI_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(out_status_intr_event,           cpld,      OUT_STATUS_INTR_EVENT);

/* Reset */
static SENSOR_DEVICE_ATTR_RW(mac_sys_reset,                   cpld,      MAC_SYS_RESET);
static SENSOR_DEVICE_ATTR_RW(mac_qspi_reset,                  cpld,      MAC_QSPI_RESET);
static SENSOR_DEVICE_ATTR_RW(mac_reset,                       cpld,      MAC_RESET);

static SENSOR_DEVICE_ATTR_RW(spi_bios_reset,                  cpld,      SPI_BIOS_RESET);
static SENSOR_DEVICE_ATTR_RW(btn_fp_reset,                    cpld,      BTN_FP_RESET);
static SENSOR_DEVICE_ATTR_RW(bios_flash_reset_ctrl,           cpld,      BIOS_FLASH_RESET_CTRL);

static SENSOR_DEVICE_ATTR_RW(cpld_to_cpu_reset,               cpld,      CPLD_TO_CPU_RESET);
static SENSOR_DEVICE_ATTR_RW(cpld_to_bmc_sys_reset,           cpld,      CPLD_TO_BMC_SYS_RESET);
static SENSOR_DEVICE_ATTR_RW(bmc_pcie_reset,                  cpld,      BMC_PCIE_RESET);
static SENSOR_DEVICE_ATTR_RW(bmc_lpc_reset,                   cpld,      BMC_LPC_RESET);
static SENSOR_DEVICE_ATTR_RW(cpu_mon_reset,                   cpld,      CPU_MON_RESET);
static SENSOR_DEVICE_ATTR_RW(bmc_phy_reset_ctrl,              cpld,      BMC_PHY_RESET_CTRL);

static SENSOR_DEVICE_ATTR_RW(usb_pwr_en,                      cpld,      USB_PWR_EN);
static SENSOR_DEVICE_ATTR_RW(usb_reset_ctrl,                  cpld,      USB_RESET_CTRL);

static SENSOR_DEVICE_ATTR_RW(cja_reset,                       cpld,      CJA_RESET);
static SENSOR_DEVICE_ATTR_RW(oe_cja,                          cpld,      OE_CJA);
static SENSOR_DEVICE_ATTR_RW(ja_wp,                           cpld,      JA_WP);
static SENSOR_DEVICE_ATTR_RW(ja_reset_ctrl,                   cpld,      JA_RESET_CTRL);

static SENSOR_DEVICE_ATTR_RW(i2c_mux_0x75_reset,              cpld,      I2C_MUX_0X75_RESET);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_0x77_reset,              cpld,      I2C_MUX_0X77_RESET);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_0x76_reset,              cpld,      I2C_MUX_0X76_RESET);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_reset,                   cpld,      I2C_MUX_RESET);

static SENSOR_DEVICE_ATTR_RW(nic1_pcie_reset,                 cpld,      NIC1_PCIE_RESET);
static SENSOR_DEVICE_ATTR_RW(cpld1_to_cpld2_reset,            cpld,      CPLD1_TO_CPLD2_RESET);
static SENSOR_DEVICE_ATTR_RW(nic_ctrl,                        cpld,      NIC_CTRL);

static SENSOR_DEVICE_ATTR_RW(psu1_eeprom_wp,                  cpld,      PSU1_EEPROM_WP);
static SENSOR_DEVICE_ATTR_RW(psu2_eeprom_wp,                  cpld,      PSU2_EEPROM_WP);
static SENSOR_DEVICE_ATTR_RW(psu_eeprom_ctrl,                 cpld,      PSU_EEPROM_CTRL);

static SENSOR_DEVICE_ATTR_RW(sys_pwr_reset,                   cpld,      SYS_PWR_RESET);

/* Misc Status Control */
static SENSOR_DEVICE_ATTR_RO(psu1_present,                    cpld,      PSU1_PRESENT);
static SENSOR_DEVICE_ATTR_RO(psu2_present,                    cpld,      PSU2_PRESENT);
static SENSOR_DEVICE_ATTR_RO(psu1_vin_pwrok,                  cpld,      PSU1_VIN_PWROK);
static SENSOR_DEVICE_ATTR_RO(psu2_vin_pwrok,                  cpld,      PSU2_VIN_PWROK);
static SENSOR_DEVICE_ATTR_RO(psu1_vout_pwrok,                 cpld,      PSU1_VOUT_PWROK);
static SENSOR_DEVICE_ATTR_RO(psu2_vout_pwrok,                 cpld,      PSU2_VOUT_PWROK);
static SENSOR_DEVICE_ATTR_RO(psu_status,                      cpld,      PSU_STATUS);

static SENSOR_DEVICE_ATTR_RO(daughter_brd_present,            cpld,      DAUGHTER_BRD_PRESENT);

static SENSOR_DEVICE_ATTR_RO(cpu_pwrgd,                       cpld,      CPU_PWRGD);
static SENSOR_DEVICE_ATTR_RO(cpu_boot_done,                   cpld,      CPU_BOOT_DONE);
static SENSOR_DEVICE_ATTR_RO(wake_cpu_pcie,                   cpld,      WAKE_CPU_PCIE);
static SENSOR_DEVICE_ATTR_RO(sys_pwr_status,                  cpld,      SYS_PWR_STATUS);

static SENSOR_DEVICE_ATTR_RW(phy_boot_ctrl,                   cpld,      PHY_BOOT_CTRL);

static SENSOR_DEVICE_ATTR_RW(bmc_wdt1_reset,                  cpld,      BMC_WDT1_RESET);
static SENSOR_DEVICE_ATTR_RW(bmc_wdt2_reset,                  cpld,      BMC_WDT2_RESET);
static SENSOR_DEVICE_ATTR_RW(wd_status,                       cpld,      WD_STATUS);

static SENSOR_DEVICE_ATTR_RW(bios_boot_sel0,                  cpld,      BIOS_BOOT_SEL0);
static SENSOR_DEVICE_ATTR_RW(bios_boot_sel1,                  cpld,      BIOS_BOOT_SEL1);
static SENSOR_DEVICE_ATTR_RW(timing_ctrl_status,              cpld,      TIMING_CTRL_STATUS);

static SENSOR_DEVICE_ATTR_RW(i2c_rov_mux_sel,                 cpld,      I2C_ROV_MUX_SEL);
static SENSOR_DEVICE_ATTR_RW(qspi_mac_mux_sel,                cpld,      QSPI_MAC_MUX_SEL);
static SENSOR_DEVICE_ATTR_RW(i2c_cpld_mux_sel,                cpld,      I2C_CPLD_MUX_SEL);
static SENSOR_DEVICE_ATTR_RW(uart_mux_sel,                    cpld,      UART_MUX_SEL);
static SENSOR_DEVICE_ATTR_RW(uart_cpu_bmc_mux_sel,            cpld,      UART_CPU_BMC_MUX_SEL);
static SENSOR_DEVICE_ATTR_RW(mux_ctrl,                        cpld,      MUX_CTRL);

static SENSOR_DEVICE_ATTR_RO(icx_spi_mux_sel_1,               cpld,      ICX_SPI_MUX_SEL_1);
static SENSOR_DEVICE_ATTR_RO(icx_spi_mux_sel_2,               cpld,      ICX_SPI_MUX_SEL_2);
static SENSOR_DEVICE_ATTR_RO(icx_spi_mux_sel_3,               cpld,      ICX_SPI_MUX_SEL_3);
static SENSOR_DEVICE_ATTR_RO(bios_spi_mux,                    cpld,      BIOS_SPI_MUX);

static SENSOR_DEVICE_ATTR_RW(pwren_cpld_to_cpu,               cpld,      PWREN_CPLD_TO_CPU);
static SENSOR_DEVICE_ATTR_RW(pwrbtn_cpld_to_cpu,              cpld,      PWRBTN_CPLD_TO_CPU);
static SENSOR_DEVICE_ATTR_RW(pwr_system_ctrl,                 cpld,      PWR_SYSTEM_CTRL);
/* SERBOOT Control */

static SENSOR_DEVICE_ATTR_RW(ufm_write,                       cpld,      UFM_WRITE);
static SENSOR_DEVICE_ATTR_RW(ufm_write_en,                    cpld,      UFM_WRITE_EN);

static SENSOR_DEVICE_ATTR_RW(bios_boot_sel0_tgt,              cpld,      BIOS_BOOT_SEL0_TGT);
static SENSOR_DEVICE_ATTR_RW(bios_boot_sel1_tgt,              cpld,      BIOS_BOOT_SEL1_TGT);
static SENSOR_DEVICE_ATTR_RW(bios_boot_sel_tgt,               cpld,      BIOS_BOOT_SEL_TGT);
/* LED Clear & GNSS Control */

static SENSOR_DEVICE_ATTR_RW(led_fan_clr,                     cpld,      LED_FAN_CLR);
static SENSOR_DEVICE_ATTR_RW(led_clear,                       cpld,      LED_CLEAR);

static SENSOR_DEVICE_ATTR_RW(gnss_ant_pwren,                  cpld,      GNSS_ANT_PWREN);
static SENSOR_DEVICE_ATTR_RW(gnss_reset,                      cpld,      GNSS_RESET);
static SENSOR_DEVICE_ATTR_RO(st_gnss_10m_out,                 cpld,      ST_GNSS_10M_OUT);
static SENSOR_DEVICE_ATTR_RW(gnss_ctrl,                       cpld,      GNSS_CTRL);

static SENSOR_DEVICE_ATTR_RW(ts_pll_clk_en,                   cpld,      TS_PLL_CLK_EN);
static SENSOR_DEVICE_ATTR_RW(p3v3_en_broadsync,               cpld,      P3V3_EN_BROADSYNC);
static SENSOR_DEVICE_ATTR_RW(timing_misc_ctrl,                cpld,      TIMING_MISC_CTRL);
/* Write Protect */

static SENSOR_DEVICE_ATTR_RW(ext_reg_define,                  cpld,      EXT_REG_DEFINE);
static SENSOR_DEVICE_ATTR_RW(ext_reg_range,                   cpld,      EXT_REG_RANGE);
static SENSOR_DEVICE_ATTR_RW(ext_ctrl,                        cpld,      EXT_CTRL);
/* LED Control */

static SENSOR_DEVICE_ATTR_RO(pwr_led_color,                   cpld,      PWR_LED_COLOR);
static SENSOR_DEVICE_ATTR_RO(pwr_led_speed,                   cpld,      PWR_LED_SPEED);
static SENSOR_DEVICE_ATTR_RO(pwr_led_blinking,                cpld,      PWR_LED_BLINKING);
static SENSOR_DEVICE_ATTR_RO(pwr_led_on_off,                  cpld,      PWR_LED_ON_OFF);
static SENSOR_DEVICE_ATTR_RO(cpld_system_led_pwr,             led,       CPLD_SYSTEM_LED_PWR);
static SENSOR_DEVICE_ATTR_RO(fan_led_color,                   cpld,      FAN_LED_COLOR);
static SENSOR_DEVICE_ATTR_RO(fan_led_speed,                   cpld,      FAN_LED_SPEED);
static SENSOR_DEVICE_ATTR_RO(fan_led_blinking,                cpld,      FAN_LED_BLINKING);
static SENSOR_DEVICE_ATTR_RO(fan_led_on_off,                  cpld,      FAN_LED_ON_OFF);
static SENSOR_DEVICE_ATTR_RO(cpld_system_led_fan,             led,       CPLD_SYSTEM_LED_FAN);
static SENSOR_DEVICE_ATTR_RO(system_led_ctrl_1,               cpld,      SYSTEM_LED_CTRL_1);

static SENSOR_DEVICE_ATTR_RW(gnss_led_color,                  cpld,      GNSS_LED_COLOR);
static SENSOR_DEVICE_ATTR_RW(gnss_led_speed,                  cpld,      GNSS_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(gnss_led_blinking,               cpld,      GNSS_LED_BLINKING);
static SENSOR_DEVICE_ATTR_RW(gnss_led_on_off,                 cpld,      GNSS_LED_ON_OFF);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_gnss,            led,       CPLD_SYSTEM_LED_GNSS);
static SENSOR_DEVICE_ATTR_RW(sys_led_color,                   cpld,      SYS_LED_COLOR);
static SENSOR_DEVICE_ATTR_RW(sys_led_speed,                   cpld,      SYS_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(sys_led_blinking,                cpld,      SYS_LED_BLINKING);
static SENSOR_DEVICE_ATTR_RW(sys_led_on_off,                  cpld,      SYS_LED_ON_OFF);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_sys,             led,       CPLD_SYSTEM_LED_SYS);
static SENSOR_DEVICE_ATTR_RW(system_led_ctrl_2,               cpld,      SYSTEM_LED_CTRL_2);

static SENSOR_DEVICE_ATTR_RW(sync_led_color,                  cpld,      SYNC_LED_COLOR);
static SENSOR_DEVICE_ATTR_RW(sync_led_speed,                  cpld,      SYNC_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(sync_led_blinking,               cpld,      SYNC_LED_BLINKING);
static SENSOR_DEVICE_ATTR_RW(sync_led_on_off,                 cpld,      SYNC_LED_ON_OFF);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_sync,            led,       CPLD_SYSTEM_LED_SYNC);
static SENSOR_DEVICE_ATTR_RW(id_led_on_off,                   cpld,      ID_LED_ON_OFF);
static SENSOR_DEVICE_ATTR_RW(id_led_blinking,                 cpld,      ID_LED_BLINKING);
static SENSOR_DEVICE_ATTR_RW(id_led_speed,                    cpld,      ID_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_id,              led,       CPLD_SYSTEM_LED_ID);
static SENSOR_DEVICE_ATTR_RW(system_led_ctrl_3,               cpld,      SYSTEM_LED_CTRL_3);

static SENSOR_DEVICE_ATTR_RW(fan_led_1_2,                     cpld,      FAN_LED_1_2);
static SENSOR_DEVICE_ATTR_RW(fan2_led_on_off,                 cpld,      FAN2_LED_ON_OFF);
static SENSOR_DEVICE_ATTR_RW(fan2_led_blinking,               cpld,      FAN2_LED_BLINKING);
static SENSOR_DEVICE_ATTR_RW(fan2_led_speed,                  cpld,      FAN2_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(fan2_led_color,                  cpld,      FAN2_LED_COLOR);
static SENSOR_DEVICE_ATTR_RW(fan1_led_on_off,                 cpld,      FAN1_LED_ON_OFF);
static SENSOR_DEVICE_ATTR_RW(fan1_led_blinking,               cpld,      FAN1_LED_BLINKING);
static SENSOR_DEVICE_ATTR_RW(fan1_led_speed,                  cpld,      FAN1_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(fan1_led_color,                  cpld,      FAN1_LED_COLOR);

static SENSOR_DEVICE_ATTR_RW(fan_led_3_4,                     cpld,      FAN_LED_3_4);
static SENSOR_DEVICE_ATTR_RW(fan4_led_on_off,                 cpld,      FAN4_LED_ON_OFF);
static SENSOR_DEVICE_ATTR_RW(fan4_led_blinking,               cpld,      FAN4_LED_BLINKING);
static SENSOR_DEVICE_ATTR_RW(fan4_led_speed,                  cpld,      FAN4_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(fan4_led_color,                  cpld,      FAN4_LED_COLOR);
static SENSOR_DEVICE_ATTR_RW(fan3_led_on_off,                 cpld,      FAN3_LED_ON_OFF);
static SENSOR_DEVICE_ATTR_RW(fan3_led_blinking,               cpld,      FAN3_LED_BLINKING);
static SENSOR_DEVICE_ATTR_RW(fan3_led_speed,                  cpld,      FAN3_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(fan3_led_color,                  cpld,      FAN3_LED_COLOR);

static SENSOR_DEVICE_ATTR_RW(fan_led_5,                       cpld,      FAN_LED_5);
static SENSOR_DEVICE_ATTR_RW(fan5_led_on_off,                 cpld,      FAN5_LED_ON_OFF);
static SENSOR_DEVICE_ATTR_RW(fan5_led_blinking,               cpld,      FAN5_LED_BLINKING);
static SENSOR_DEVICE_ATTR_RW(fan5_led_speed,                  cpld,      FAN5_LED_SPEED);
static SENSOR_DEVICE_ATTR_RW(fan5_led_color,                  cpld,      FAN5_LED_COLOR);
/* Power Status */

static SENSOR_DEVICE_ATTR_RO(p5v_aux_pg,                      cpld,      P5V_AUX_PG);
static SENSOR_DEVICE_ATTR_RO(p3v3_pg,                         cpld,      P3V3_PG);
static SENSOR_DEVICE_ATTR_RO(p1v8_clk_pg,                     cpld,      P1V8_CLK_PG);
static SENSOR_DEVICE_ATTR_RO(p1v2_cpld2_pg,                   cpld,      P1V2_CPLD2_PG);
static SENSOR_DEVICE_ATTR_RO(p2v5_cpld2_pg,                   cpld,      P2V5_CPLD2_PG);
static SENSOR_DEVICE_ATTR_RO(p1v8_vdd0_pg,                    cpld,      P1V8_VDD0_PG);
static SENSOR_DEVICE_ATTR_RO(vdd_core_pg,                     cpld,      VDD_CORE_PG);
static SENSOR_DEVICE_ATTR_RO(p0v9_pvdd_pg,                    cpld,      P0V9_PVDD_PG);
static SENSOR_DEVICE_ATTR_RO(pwr_status_1,                    cpld,      PWR_STATUS_1);

static SENSOR_DEVICE_ATTR_RO(p0v75_trvdd100_pg,               cpld,      P0V75_TRVDD100_PG);
static SENSOR_DEVICE_ATTR_RO(p0v75_trvdd50_pg,                cpld,      P0V75_TRVDD50_PG);
static SENSOR_DEVICE_ATTR_RO(p0v9_trvdd_pg,                   cpld,      P0V9_TRVDD_PG);
static SENSOR_DEVICE_ATTR_RO(p0v8_avdd_pg,                    cpld,      P0V8_AVDD_PG);
static SENSOR_DEVICE_ATTR_RO(p1v2_trvdd_pg,                   cpld,      P1V2_TRVDD_PG);
static SENSOR_DEVICE_ATTR_RO(p1v2_pvdd_pg,                    cpld,      P1V2_PVDD_PG);
static SENSOR_DEVICE_ATTR_RO(p1v5_avdd_pg,                    cpld,      P1V5_AVDD_PG);
static SENSOR_DEVICE_ATTR_RO(p1v5_trxvdd_pg,                  cpld,      P1V5_TRXVDD_PG);
static SENSOR_DEVICE_ATTR_RO(pwr_status_2,                    cpld,      PWR_STATUS_2);

static SENSOR_DEVICE_ATTR_RO(p0v75_ddr_vddc_pg,               cpld,      P0V75_DDR_VDDC_PG);
static SENSOR_DEVICE_ATTR_RO(p1v05_ddr_vddqx_pg,              cpld,      P1V05_DDR_VDDQX_PG);
static SENSOR_DEVICE_ATTR_RO(p1v8_ddr_pg,                     cpld,      P1V8_DDR_PG);
static SENSOR_DEVICE_ATTR_RO(p0v5_ddr_vddq_pg,                cpld,      P0V5_DDR_VDDQ_PG);
static SENSOR_DEVICE_ATTR_RO(pwr_status_3,                    cpld,      PWR_STATUS_3);

/* Timing Control */
static SENSOR_DEVICE_ATTR_RO(ocxo_id,                         cpld,      OCXO_ID);
static SENSOR_DEVICE_ATTR_RO(gnss_modl_id,                    cpld,      GNSS_MODL_ID);
static SENSOR_DEVICE_ATTR_RO(ocxo_gnss_id,                    cpld,      OCXO_GNSS_ID);

static SENSOR_DEVICE_ATTR_RW(ntm_reset,                       cpld,      NTM_RESET);
static SENSOR_DEVICE_ATTR_RW(bits_reset,                      cpld,      BITS_RESET);
static SENSOR_DEVICE_ATTR_RW(clk_ptp_reset,                   cpld,      CLK_PTP_RESET);

static SENSOR_DEVICE_ATTR_RW(ts_pll_clk_source_sel,           cpld,      TS_PLL_CLK_SOURCE_SEL);
static SENSOR_DEVICE_ATTR_RW(smb_1pps_dir_sel,                cpld,      SMB_1PPS_DIR_SEL);
static SENSOR_DEVICE_ATTR_RW(clk_10m_ptp_in,                  cpld,      CLK_10M_PTP_IN);
static SENSOR_DEVICE_ATTR_RW(smb_10m_input_en,                cpld,      SMB_10M_INPUT_EN);
static SENSOR_DEVICE_ATTR_RW(ptp_tod_rs422_dir_ctrl,          cpld,      PTP_TOD_RS422_DIR_CTRL);
static SENSOR_DEVICE_ATTR_RW(ptp_1pps_rs422_dir_ctrl,         cpld,      PTP_1PPS_RS422_DIR_CTRL);
static SENSOR_DEVICE_ATTR_RW(smb_10m_dir_sel,                 cpld,      SMB_10M_DIR_SEL);
static SENSOR_DEVICE_ATTR_RW(clk_timing_ctrl,                 cpld,      CLK_TIMING_CTRL);

static SENSOR_DEVICE_ATTR_RO(gnss_ant_short,                  cpld,      GNSS_ANT_SHORT);
static SENSOR_DEVICE_ATTR_RO(gnss_ant_open,                   cpld,      GNSS_ANT_OPEN);
static SENSOR_DEVICE_ATTR_RO(gnss_ant_on,                     cpld,      GNSS_ANT_ON);
static SENSOR_DEVICE_ATTR_RO(gnss_status,                     cpld,      GNSS_STATUS);

static SENSOR_DEVICE_ATTR_RO(ntm_present,                     cpld,      NTM_PRESENT);
static SENSOR_DEVICE_ATTR_RO(gnss_present,                    cpld,      GNSS_PRESENT);
static SENSOR_DEVICE_ATTR_RO(gnss_ant_pwr_st,                 cpld,      GNSS_ANT_PWR_ST);
static SENSOR_DEVICE_ATTR_RO(timing_status,                   cpld,      TIMING_STATUS);

static SENSOR_DEVICE_ATTR_RW(synce_ch_set,                    cpld,      SYNCE_CH_SET);
static SENSOR_DEVICE_ATTR_RW(i2c_mux_reset_mb,                cpld,      I2C_MUX_RESET_MB);

// & cpld2
/* Ports Interrupt Status */

static SENSOR_DEVICE_ATTR_RO(eth_0_present,                   cpld,      ETH_0_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_1_present,                   cpld,      ETH_1_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_2_present,                   cpld,      ETH_2_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_3_present,                   cpld,      ETH_3_PRESENT);

static SENSOR_DEVICE_ATTR_RO(eth_4_present,                   cpld,      ETH_4_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_5_present,                   cpld,      ETH_5_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_6_present,                   cpld,      ETH_6_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_7_present,                   cpld,      ETH_7_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_8_present,                   cpld,      ETH_8_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_9_present,                   cpld,      ETH_9_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_10_present,                  cpld,      ETH_10_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_11_present,                  cpld,      ETH_11_PRESENT);

static SENSOR_DEVICE_ATTR_RO(eth_12_present,                  cpld,      ETH_12_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_13_present,                  cpld,      ETH_13_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_14_present,                  cpld,      ETH_14_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_15_present,                  cpld,      ETH_15_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_16_present,                  cpld,      ETH_16_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_17_present,                  cpld,      ETH_17_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_18_present,                  cpld,      ETH_18_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_19_present,                  cpld,      ETH_19_PRESENT);

static SENSOR_DEVICE_ATTR_RO(eth_20_present,                  cpld,      ETH_20_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_21_present,                  cpld,      ETH_21_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_22_present,                  cpld,      ETH_22_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_23_present,                  cpld,      ETH_23_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_24_present,                  cpld,      ETH_24_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_25_present,                  cpld,      ETH_25_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_26_present,                  cpld,      ETH_26_PRESENT);
static SENSOR_DEVICE_ATTR_RO(eth_27_present,                  cpld,      ETH_27_PRESENT);

static SENSOR_DEVICE_ATTR_RO(eth_0_intr,                      cpld,      ETH_0_INTR);
static SENSOR_DEVICE_ATTR_RO(eth_1_intr,                      cpld,      ETH_1_INTR);
static SENSOR_DEVICE_ATTR_RO(eth_2_intr,                      cpld,      ETH_2_INTR);
static SENSOR_DEVICE_ATTR_RO(eth_3_intr,                      cpld,      ETH_3_INTR);

static SENSOR_DEVICE_ATTR_RW(eth_4_rx_los,                    cpld,      ETH_4_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_5_rx_los,                    cpld,      ETH_5_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_6_rx_los,                    cpld,      ETH_6_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_7_rx_los,                    cpld,      ETH_7_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_8_rx_los,                    cpld,      ETH_8_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_9_rx_los,                    cpld,      ETH_9_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_10_rx_los,                   cpld,      ETH_10_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_11_rx_los,                   cpld,      ETH_11_RX_LOS);

static SENSOR_DEVICE_ATTR_RW(eth_12_rx_los,                   cpld,      ETH_12_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_13_rx_los,                   cpld,      ETH_13_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_14_rx_los,                   cpld,      ETH_14_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_15_rx_los,                   cpld,      ETH_15_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_16_rx_los,                   cpld,      ETH_16_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_17_rx_los,                   cpld,      ETH_17_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_18_rx_los,                   cpld,      ETH_18_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_19_rx_los,                   cpld,      ETH_19_RX_LOS);

static SENSOR_DEVICE_ATTR_RW(eth_20_rx_los,                   cpld,      ETH_20_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_21_rx_los,                   cpld,      ETH_21_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_22_rx_los,                   cpld,      ETH_22_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_23_rx_los,                   cpld,      ETH_23_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_24_rx_los,                   cpld,      ETH_24_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_25_rx_los,                   cpld,      ETH_25_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_26_rx_los,                   cpld,      ETH_26_RX_LOS);
static SENSOR_DEVICE_ATTR_RW(eth_27_rx_los,                   cpld,      ETH_27_RX_LOS);

static SENSOR_DEVICE_ATTR_RO(eth_0_fuse,                      cpld,      ETH_0_FUSE);
static SENSOR_DEVICE_ATTR_RO(eth_1_fuse,                      cpld,      ETH_1_FUSE);
static SENSOR_DEVICE_ATTR_RO(eth_2_fuse,                      cpld,      ETH_2_FUSE);
static SENSOR_DEVICE_ATTR_RO(eth_3_fuse,                      cpld,      ETH_3_FUSE);

static SENSOR_DEVICE_ATTR_RO(eth_4_tx_flt,                    cpld,      ETH_4_TX_FLT);
static SENSOR_DEVICE_ATTR_RO(eth_5_tx_flt,                    cpld,      ETH_5_TX_FLT);
static SENSOR_DEVICE_ATTR_RO(eth_6_tx_flt,                    cpld,      ETH_6_TX_FLT);
static SENSOR_DEVICE_ATTR_RO(eth_7_tx_flt,                    cpld,      ETH_7_TX_FLT);
static SENSOR_DEVICE_ATTR_RO(eth_8_tx_flt,                    cpld,      ETH_8_TX_FLT);
static SENSOR_DEVICE_ATTR_RO(eth_9_tx_flt,                    cpld,      ETH_9_TX_FLT);
static SENSOR_DEVICE_ATTR_RO(eth_10_tx_flt,                   cpld,      ETH_10_TX_FLT);
static SENSOR_DEVICE_ATTR_RO(eth_11_tx_flt,                   cpld,      ETH_11_TX_FLT);

static SENSOR_DEVICE_ATTR_RO(eth_12_tx_flt,                   cpld,      ETH_12_TX_FLT);
static SENSOR_DEVICE_ATTR_RO(eth_13_tx_flt,                   cpld,      ETH_13_TX_FLT);
static SENSOR_DEVICE_ATTR_RO(eth_14_tx_flt,                   cpld,      ETH_14_TX_FLT);
static SENSOR_DEVICE_ATTR_RO(eth_15_tx_flt,                   cpld,      ETH_15_TX_FLT);
static SENSOR_DEVICE_ATTR_RO(eth_16_tx_flt,                   cpld,      ETH_16_TX_FLT);
static SENSOR_DEVICE_ATTR_RO(eth_17_tx_flt,                   cpld,      ETH_17_TX_FLT);
static SENSOR_DEVICE_ATTR_RO(eth_18_tx_flt,                   cpld,      ETH_18_TX_FLT);
static SENSOR_DEVICE_ATTR_RO(eth_19_tx_flt,                   cpld,      ETH_19_TX_FLT);

static SENSOR_DEVICE_ATTR_RO(eth_20_tx_flt,                   cpld,      ETH_20_TX_FLT);
static SENSOR_DEVICE_ATTR_RO(eth_21_tx_flt,                   cpld,      ETH_21_TX_FLT);
static SENSOR_DEVICE_ATTR_RO(eth_22_tx_flt,                   cpld,      ETH_22_TX_FLT);
static SENSOR_DEVICE_ATTR_RO(eth_23_tx_flt,                   cpld,      ETH_23_TX_FLT);
static SENSOR_DEVICE_ATTR_RO(eth_24_tx_flt,                   cpld,      ETH_24_TX_FLT);
static SENSOR_DEVICE_ATTR_RO(eth_25_tx_flt,                   cpld,      ETH_25_TX_FLT);
static SENSOR_DEVICE_ATTR_RO(eth_26_tx_flt,                   cpld,      ETH_26_TX_FLT);
static SENSOR_DEVICE_ATTR_RO(eth_27_tx_flt,                   cpld,      ETH_27_TX_FLT);
/* Interrupt Mask */

static SENSOR_DEVICE_ATTR_RW(eth_0_present_mask,              cpld,      ETH_0_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_1_present_mask,              cpld,      ETH_1_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_2_present_mask,              cpld,      ETH_2_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_3_present_mask,              cpld,      ETH_3_PRESENT_MASK);

static SENSOR_DEVICE_ATTR_RW(eth_4_present_mask,              cpld,      ETH_4_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_5_present_mask,              cpld,      ETH_5_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_6_present_mask,              cpld,      ETH_6_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_7_present_mask,              cpld,      ETH_7_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_8_present_mask,              cpld,      ETH_8_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_9_present_mask,              cpld,      ETH_9_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_10_present_mask,             cpld,      ETH_10_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_11_present_mask,             cpld,      ETH_11_PRESENT_MASK);

static SENSOR_DEVICE_ATTR_RW(eth_12_present_mask,             cpld,      ETH_12_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_13_present_mask,             cpld,      ETH_13_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_14_present_mask,             cpld,      ETH_14_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_15_present_mask,             cpld,      ETH_15_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_16_present_mask,             cpld,      ETH_16_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_17_present_mask,             cpld,      ETH_17_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_18_present_mask,             cpld,      ETH_18_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_19_present_mask,             cpld,      ETH_19_PRESENT_MASK);

static SENSOR_DEVICE_ATTR_RW(eth_20_present_mask,             cpld,      ETH_20_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_21_present_mask,             cpld,      ETH_21_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_22_present_mask,             cpld,      ETH_22_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_23_present_mask,             cpld,      ETH_23_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_24_present_mask,             cpld,      ETH_24_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_25_present_mask,             cpld,      ETH_25_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_26_present_mask,             cpld,      ETH_26_PRESENT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_27_present_mask,             cpld,      ETH_27_PRESENT_MASK);

static SENSOR_DEVICE_ATTR_RW(eth_0_intr_mask,                 cpld,      ETH_0_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_1_intr_mask,                 cpld,      ETH_1_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_2_intr_mask,                 cpld,      ETH_2_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_3_intr_mask,                 cpld,      ETH_3_INTR_MASK);

static SENSOR_DEVICE_ATTR_RW(eth_4_rx_los_mask,               cpld,      ETH_4_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_5_rx_los_mask,               cpld,      ETH_5_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_6_rx_los_mask,               cpld,      ETH_6_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_7_rx_los_mask,               cpld,      ETH_7_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_8_rx_los_mask,               cpld,      ETH_8_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_9_rx_los_mask,               cpld,      ETH_9_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_10_rx_los_mask,              cpld,      ETH_10_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_11_rx_los_mask,              cpld,      ETH_11_RX_LOS_MASK);

static SENSOR_DEVICE_ATTR_RW(eth_12_rx_los_mask,              cpld,      ETH_12_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_13_rx_los_mask,              cpld,      ETH_13_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_14_rx_los_mask,              cpld,      ETH_14_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_15_rx_los_mask,              cpld,      ETH_15_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_16_rx_los_mask,              cpld,      ETH_16_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_17_rx_los_mask,              cpld,      ETH_17_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_18_rx_los_mask,              cpld,      ETH_18_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_19_rx_los_mask,              cpld,      ETH_19_RX_LOS_MASK);

static SENSOR_DEVICE_ATTR_RW(eth_20_rx_los_mask,              cpld,      ETH_20_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_21_rx_los_mask,              cpld,      ETH_21_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_22_rx_los_mask,              cpld,      ETH_22_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_23_rx_los_mask,              cpld,      ETH_23_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_24_rx_los_mask,              cpld,      ETH_24_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_25_rx_los_mask,              cpld,      ETH_25_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_26_rx_los_mask,              cpld,      ETH_26_RX_LOS_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_27_rx_los_mask,              cpld,      ETH_27_RX_LOS_MASK);

static SENSOR_DEVICE_ATTR_RW(eth_0_fuse_mask,                 cpld,      ETH_0_FUSE_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_1_fuse_mask,                 cpld,      ETH_1_FUSE_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_2_fuse_mask,                 cpld,      ETH_2_FUSE_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_3_fuse_mask,                 cpld,      ETH_3_FUSE_MASK);

static SENSOR_DEVICE_ATTR_RW(eth_4_tx_flt_mask,               cpld,      ETH_4_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_5_tx_flt_mask,               cpld,      ETH_5_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_6_tx_flt_mask,               cpld,      ETH_6_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_7_tx_flt_mask,               cpld,      ETH_7_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_8_tx_flt_mask,               cpld,      ETH_8_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_9_tx_flt_mask,               cpld,      ETH_9_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_10_tx_flt_mask,              cpld,      ETH_10_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_11_tx_flt_mask,              cpld,      ETH_11_TX_FLT_MASK);

static SENSOR_DEVICE_ATTR_RW(eth_12_tx_flt_mask,              cpld,      ETH_12_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_13_tx_flt_mask,              cpld,      ETH_13_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_14_tx_flt_mask,              cpld,      ETH_14_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_15_tx_flt_mask,              cpld,      ETH_15_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_16_tx_flt_mask,              cpld,      ETH_16_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_17_tx_flt_mask,              cpld,      ETH_17_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_18_tx_flt_mask,              cpld,      ETH_18_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_19_tx_flt_mask,              cpld,      ETH_19_TX_FLT_MASK);

static SENSOR_DEVICE_ATTR_RW(eth_20_tx_flt_mask,              cpld,      ETH_20_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_21_tx_flt_mask,              cpld,      ETH_21_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_22_tx_flt_mask,              cpld,      ETH_22_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_23_tx_flt_mask,              cpld,      ETH_23_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_24_tx_flt_mask,              cpld,      ETH_24_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_25_tx_flt_mask,              cpld,      ETH_25_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_26_tx_flt_mask,              cpld,      ETH_26_TX_FLT_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_27_tx_flt_mask,              cpld,      ETH_27_TX_FLT_MASK);
/* Interrupt Event */

static SENSOR_DEVICE_ATTR_RO(qsfp28_0_3_present_event,        cpld,      QSFP28_0_3_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp28_4_11_present_event,        cpld,      SFP28_4_11_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp28_12_19_present_event,       cpld,      SFP28_12_19_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp56_20_27_present_event,       cpld,      SFP56_20_27_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(qsfp28_0_3_intr_event,           cpld,      QSFP28_0_3_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp28_4_11_rx_los_event,         cpld,      SFP28_4_11_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp28_12_19_rx_los_event,        cpld,      SFP28_12_19_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp56_20_27_rx_los_event,        cpld,      SFP56_20_27_RX_LOS_EVENT);
static SENSOR_DEVICE_ATTR_RO(qsfp28_0_3_fuse_event,           cpld,      QSFP28_0_3_FUSE_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp28_4_11_tx_flt_event,         cpld,      SFP28_4_11_TX_FLT_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp28_12_19_tx_flt_event,        cpld,      SFP28_12_19_TX_FLT_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp56_20_27_tx_flt_event,        cpld,      SFP56_20_27_TX_FLT_EVENT);

static SENSOR_DEVICE_ATTR_RO(eth_0_present_event,             cpld,      ETH_0_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_1_present_event,             cpld,      ETH_1_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_2_present_event,             cpld,      ETH_2_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_3_present_event,             cpld,      ETH_3_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_4_present_event,             cpld,      ETH_4_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_5_present_event,             cpld,      ETH_5_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_6_present_event,             cpld,      ETH_6_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_7_present_event,             cpld,      ETH_7_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_8_present_event,             cpld,      ETH_8_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_9_present_event,             cpld,      ETH_9_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_10_present_event,            cpld,      ETH_10_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_11_present_event,            cpld,      ETH_11_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_12_present_event,            cpld,      ETH_12_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_13_present_event,            cpld,      ETH_13_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_14_present_event,            cpld,      ETH_14_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_15_present_event,            cpld,      ETH_15_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_16_present_event,            cpld,      ETH_16_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_17_present_event,            cpld,      ETH_17_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_18_present_event,            cpld,      ETH_18_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_19_present_event,            cpld,      ETH_19_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_20_present_event,            cpld,      ETH_20_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_21_present_event,            cpld,      ETH_21_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_22_present_event,            cpld,      ETH_22_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_23_present_event,            cpld,      ETH_23_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_24_present_event,            cpld,      ETH_24_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_25_present_event,            cpld,      ETH_25_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_26_present_event,            cpld,      ETH_26_PRESENT_EVENT);
static SENSOR_DEVICE_ATTR_RO(eth_27_present_event,            cpld,      ETH_27_PRESENT_EVENT);

/* Reset and Control */

static SENSOR_DEVICE_ATTR_RW(eth_0_reset,                     cpld,      ETH_0_RESET);
static SENSOR_DEVICE_ATTR_RW(eth_1_reset,                     cpld,      ETH_1_RESET);
static SENSOR_DEVICE_ATTR_RW(eth_2_reset,                     cpld,      ETH_2_RESET);
static SENSOR_DEVICE_ATTR_RW(eth_3_reset,                     cpld,      ETH_3_RESET);

static SENSOR_DEVICE_ATTR_RW(eth_0_lpmode,                    cpld,      ETH_0_LPMODE);
static SENSOR_DEVICE_ATTR_RW(eth_1_lpmode,                    cpld,      ETH_1_LPMODE);
static SENSOR_DEVICE_ATTR_RW(eth_2_lpmode,                    cpld,      ETH_2_LPMODE);
static SENSOR_DEVICE_ATTR_RW(eth_3_lpmode,                    cpld,      ETH_3_LPMODE);

static SENSOR_DEVICE_ATTR_RW(ssd1_m2_config,                  cpld,      SSD1_M2_CONFIG);
static SENSOR_DEVICE_ATTR_RW(ssd2_m2_config,                  cpld,      SSD2_M2_CONFIG);
static SENSOR_DEVICE_ATTR_RW(cpu_ssd1_perst,                  cpld,      CPU_SSD1_PERST);
static SENSOR_DEVICE_ATTR_RW(cpu_ssd2_perst,                  cpld,      CPU_SSD2_PERST);
static SENSOR_DEVICE_ATTR_RW(pwren_p3v3_ssd1,                 cpld,      PWREN_P3V3_SSD1);
static SENSOR_DEVICE_ATTR_RW(pwren_p3v3_ssd2,                 cpld,      PWREN_P3V3_SSD2);

static SENSOR_DEVICE_ATTR_RW(eth_4_tx_disable,                cpld,      ETH_4_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_5_tx_disable,                cpld,      ETH_5_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_6_tx_disable,                cpld,      ETH_6_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_7_tx_disable,                cpld,      ETH_7_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_8_tx_disable,                cpld,      ETH_8_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_9_tx_disable,                cpld,      ETH_9_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_10_tx_disable,               cpld,      ETH_10_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_11_tx_disable,               cpld,      ETH_11_TX_DISABLE);

static SENSOR_DEVICE_ATTR_RW(eth_12_tx_disable,               cpld,      ETH_12_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_13_tx_disable,               cpld,      ETH_13_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_14_tx_disable,               cpld,      ETH_14_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_15_tx_disable,               cpld,      ETH_15_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_16_tx_disable,               cpld,      ETH_16_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_17_tx_disable,               cpld,      ETH_17_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_18_tx_disable,               cpld,      ETH_18_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_19_tx_disable,               cpld,      ETH_19_TX_DISABLE);

static SENSOR_DEVICE_ATTR_RW(eth_20_tx_disable,               cpld,      ETH_20_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_21_tx_disable,               cpld,      ETH_21_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_22_tx_disable,               cpld,      ETH_22_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_23_tx_disable,               cpld,      ETH_23_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_24_tx_disable,               cpld,      ETH_24_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_25_tx_disable,               cpld,      ETH_25_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_26_tx_disable,               cpld,      ETH_26_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(eth_27_tx_disable,               cpld,      ETH_27_TX_DISABLE);

static SENSOR_DEVICE_ATTR_RW(eth_4_rate_select,               cpld,      ETH_4_RATE_SELECT);
static SENSOR_DEVICE_ATTR_RW(eth_5_rate_select,               cpld,      ETH_5_RATE_SELECT);
static SENSOR_DEVICE_ATTR_RW(eth_6_rate_select,               cpld,      ETH_6_RATE_SELECT);
static SENSOR_DEVICE_ATTR_RW(eth_7_rate_select,               cpld,      ETH_7_RATE_SELECT);
static SENSOR_DEVICE_ATTR_RW(eth_8_rate_select,               cpld,      ETH_8_RATE_SELECT);
static SENSOR_DEVICE_ATTR_RW(eth_9_rate_select,               cpld,      ETH_9_RATE_SELECT);
static SENSOR_DEVICE_ATTR_RW(eth_10_rate_select,              cpld,      ETH_10_RATE_SELECT);
static SENSOR_DEVICE_ATTR_RW(eth_11_rate_select,              cpld,      ETH_11_RATE_SELECT);

static SENSOR_DEVICE_ATTR_RW(eth_12_rate_select,              cpld,      ETH_12_RATE_SELECT);
static SENSOR_DEVICE_ATTR_RW(eth_13_rate_select,              cpld,      ETH_13_RATE_SELECT);
static SENSOR_DEVICE_ATTR_RW(eth_14_rate_select,              cpld,      ETH_14_RATE_SELECT);
static SENSOR_DEVICE_ATTR_RW(eth_15_rate_select,              cpld,      ETH_15_RATE_SELECT);
static SENSOR_DEVICE_ATTR_RW(eth_16_rate_select,              cpld,      ETH_16_RATE_SELECT);
static SENSOR_DEVICE_ATTR_RW(eth_17_rate_select,              cpld,      ETH_17_RATE_SELECT);
static SENSOR_DEVICE_ATTR_RW(eth_18_rate_select,              cpld,      ETH_18_RATE_SELECT);
static SENSOR_DEVICE_ATTR_RW(eth_19_rate_select,              cpld,      ETH_19_RATE_SELECT);

static SENSOR_DEVICE_ATTR_RW(eth_20_rate_select,              cpld,      ETH_20_RATE_SELECT);
static SENSOR_DEVICE_ATTR_RW(eth_21_rate_select,              cpld,      ETH_21_RATE_SELECT);
static SENSOR_DEVICE_ATTR_RW(eth_22_rate_select,              cpld,      ETH_22_RATE_SELECT);
static SENSOR_DEVICE_ATTR_RW(eth_23_rate_select,              cpld,      ETH_23_RATE_SELECT);
static SENSOR_DEVICE_ATTR_RW(eth_24_rate_select,              cpld,      ETH_24_RATE_SELECT);
static SENSOR_DEVICE_ATTR_RW(eth_25_rate_select,              cpld,      ETH_25_RATE_SELECT);
static SENSOR_DEVICE_ATTR_RW(eth_26_rate_select,              cpld,      ETH_26_RATE_SELECT);
static SENSOR_DEVICE_ATTR_RW(eth_27_rate_select,              cpld,      ETH_27_RATE_SELECT);
/* Status */

static SENSOR_DEVICE_ATTR_RO(usb_ctrl_present,                cpld,      USB_CTRL_PRESENT);

static SENSOR_DEVICE_ATTR_RO(nvme_ssd2_present,               cpld,      NVME_SSD2_PRESENT);
static SENSOR_DEVICE_ATTR_RO(nvme_ssd1_present,               cpld,      NVME_SSD1_PRESENT);

static SENSOR_DEVICE_ATTR_RO(eth_4_5_oc,                      cpld,      ETH_4_5_OC);
static SENSOR_DEVICE_ATTR_RO(eth_6_7_oc,                      cpld,      ETH_6_7_OC);
static SENSOR_DEVICE_ATTR_RO(eth_8_9_oc,                      cpld,      ETH_8_9_OC);
static SENSOR_DEVICE_ATTR_RO(eth_10_11_oc,                    cpld,      ETH_10_11_OC);
static SENSOR_DEVICE_ATTR_RO(eth_12_13_oc,                    cpld,      ETH_12_13_OC);
static SENSOR_DEVICE_ATTR_RO(eth_14_15_oc,                    cpld,      ETH_14_15_OC);
static SENSOR_DEVICE_ATTR_RO(eth_16_17_oc,                    cpld,      ETH_16_17_OC);
static SENSOR_DEVICE_ATTR_RO(eth_18_19_oc,                    cpld,      ETH_18_19_OC);
static SENSOR_DEVICE_ATTR_RO(eth_20_oc,                       cpld,      ETH_20_OC);
static SENSOR_DEVICE_ATTR_RO(eth_21_oc,                       cpld,      ETH_21_OC);
static SENSOR_DEVICE_ATTR_RO(eth_22_oc,                       cpld,      ETH_22_OC);
static SENSOR_DEVICE_ATTR_RO(eth_23_oc,                       cpld,      ETH_23_OC);
static SENSOR_DEVICE_ATTR_RO(eth_24_oc,                       cpld,      ETH_24_OC);
static SENSOR_DEVICE_ATTR_RO(eth_25_oc,                       cpld,      ETH_25_OC);
static SENSOR_DEVICE_ATTR_RO(eth_26_oc,                       cpld,      ETH_26_OC);
static SENSOR_DEVICE_ATTR_RO(eth_27_oc,                       cpld,      ETH_27_OC);

static SENSOR_DEVICE_ATTR_RO(eth_0_i2c_stuck,                 cpld,      ETH_0_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_1_i2c_stuck,                 cpld,      ETH_1_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_2_i2c_stuck,                 cpld,      ETH_2_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_3_i2c_stuck,                 cpld,      ETH_3_I2C_STUCK);

static SENSOR_DEVICE_ATTR_RO(eth_4_i2c_stuck,                 cpld,      ETH_4_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_5_i2c_stuck,                 cpld,      ETH_5_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_6_i2c_stuck,                 cpld,      ETH_6_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_7_i2c_stuck,                 cpld,      ETH_7_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_8_i2c_stuck,                 cpld,      ETH_8_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_9_i2c_stuck,                 cpld,      ETH_9_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_10_i2c_stuck,                cpld,      ETH_10_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_11_i2c_stuck,                cpld,      ETH_11_I2C_STUCK);

static SENSOR_DEVICE_ATTR_RO(eth_12_i2c_stuck,                cpld,      ETH_12_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_13_i2c_stuck,                cpld,      ETH_13_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_14_i2c_stuck,                cpld,      ETH_14_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_15_i2c_stuck,                cpld,      ETH_15_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_16_i2c_stuck,                cpld,      ETH_16_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_17_i2c_stuck,                cpld,      ETH_17_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_18_i2c_stuck,                cpld,      ETH_18_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_19_i2c_stuck,                cpld,      ETH_19_I2C_STUCK);

static SENSOR_DEVICE_ATTR_RO(eth_20_i2c_stuck,                cpld,      ETH_20_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_21_i2c_stuck,                cpld,      ETH_21_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_22_i2c_stuck,                cpld,      ETH_22_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_23_i2c_stuck,                cpld,      ETH_23_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_24_i2c_stuck,                cpld,      ETH_24_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_25_i2c_stuck,                cpld,      ETH_25_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_26_i2c_stuck,                cpld,      ETH_26_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(eth_27_i2c_stuck,                cpld,      ETH_27_I2C_STUCK);

static SENSOR_DEVICE_ATTR_RO(cpu_legacy_side_i2c_stuck,       cpld,      CPU_LEGACY_SIDE_I2C_STUCK);
static SENSOR_DEVICE_ATTR_RO(cpu_i2c_stuck_status,            cpld,      CPU_I2C_STUCK_STATUS);

static SENSOR_DEVICE_ATTR_RW(eth_0_i2c_stuck_mask,            cpld,      ETH_0_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_1_i2c_stuck_mask,            cpld,      ETH_1_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_2_i2c_stuck_mask,            cpld,      ETH_2_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_3_i2c_stuck_mask,            cpld,      ETH_3_I2C_STUCK_MASK);

static SENSOR_DEVICE_ATTR_RW(eth_4_i2c_stuck_mask,            cpld,      ETH_4_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_5_i2c_stuck_mask,            cpld,      ETH_5_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_6_i2c_stuck_mask,            cpld,      ETH_6_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_7_i2c_stuck_mask,            cpld,      ETH_7_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_8_i2c_stuck_mask,            cpld,      ETH_8_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_9_i2c_stuck_mask,            cpld,      ETH_9_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_10_i2c_stuck_mask,           cpld,      ETH_10_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_11_i2c_stuck_mask,           cpld,      ETH_11_I2C_STUCK_MASK);

static SENSOR_DEVICE_ATTR_RW(eth_12_i2c_stuck_mask,           cpld,      ETH_12_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_13_i2c_stuck_mask,           cpld,      ETH_13_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_14_i2c_stuck_mask,           cpld,      ETH_14_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_15_i2c_stuck_mask,           cpld,      ETH_15_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_16_i2c_stuck_mask,           cpld,      ETH_16_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_17_i2c_stuck_mask,           cpld,      ETH_17_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_18_i2c_stuck_mask,           cpld,      ETH_18_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_19_i2c_stuck_mask,           cpld,      ETH_19_I2C_STUCK_MASK);

static SENSOR_DEVICE_ATTR_RW(eth_20_i2c_stuck_mask,           cpld,      ETH_20_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_21_i2c_stuck_mask,           cpld,      ETH_21_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_22_i2c_stuck_mask,           cpld,      ETH_22_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_23_i2c_stuck_mask,           cpld,      ETH_23_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_24_i2c_stuck_mask,           cpld,      ETH_24_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_25_i2c_stuck_mask,           cpld,      ETH_25_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_26_i2c_stuck_mask,           cpld,      ETH_26_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(eth_27_i2c_stuck_mask,           cpld,      ETH_27_I2C_STUCK_MASK);

static SENSOR_DEVICE_ATTR_RW(cpu_legacy_side_i2c_stuck_mask,  cpld,      CPU_LEGACY_SIDE_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RW(cpu_i2c_stuck_mask,              cpld,      CPU_I2C_STUCK_MASK);
static SENSOR_DEVICE_ATTR_RO(qsfp28_0_3_i2c_stuck_event,      cpld,      QSFP28_0_3_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp28_4_11_i2c_stuck_event,      cpld,      SFP28_4_11_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp28_12_19_i2c_stuck_event,     cpld,      SFP28_12_19_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(sfp56_20_27_i2c_stuck_event,     cpld,      SFP56_20_27_I2C_STUCK_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpu_i2c_stuck_event,             cpld,      CPU_I2C_STUCK_EVENT);

//BSP DEBUG
static SENSOR_DEVICE_ATTR_RW(bsp_debug,                       bsp_callback, BSP_DEBUG);
static SENSOR_DEVICE_ATTR_RO(bsp_wp_access_count,             bsp_callback, BSP_WP_ACCESS_COUNT);

//MUX
static SENSOR_DEVICE_ATTR_RW(idle_state,                      idle_state, IDLE_STATE);

/* define support attributes of cpldx */


/* --- Attribute Groups --- */
static struct attribute *cpld1_attributes[] = {
    // cpld common
    _DEVICE_ATTR(cpld_version),
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(cpld_chip_type),
    _DEVICE_ATTR(event_ctrl),
    _DEVICE_ATTR(event_detect_ctrl),
    _DEVICE_ATTR(remote_i2c_reset),
    _DEVICE_ATTR(module_reset),

    // cpld1
    _DEVICE_ATTR(cpld_sku_id),
    _DEVICE_ATTR(brd_hw_id),
    _DEVICE_ATTR(deph_id),
    _DEVICE_ATTR(build_id),
    _DEVICE_ATTR(bit_sel_id),
    _DEVICE_ATTR(hw_build_rev),
    _DEVICE_ATTR(ext_id),
    _DEVICE_ATTR(cpld_ext_id),
    _DEVICE_ATTR(cja_lol_intr),
    _DEVICE_ATTR(ntm_intr),
    _DEVICE_ATTR(dpll_1588_intr),
    _DEVICE_ATTR(dpll_sync_intr),
    _DEVICE_ATTR(bits_intr),
    _DEVICE_ATTR(i2c_nic_alrt),
    _DEVICE_ATTR(psu1_intr),
    _DEVICE_ATTR(psu2_intr),
    _DEVICE_ATTR(vdd_core_pin_alrt_intr),
    _DEVICE_ATTR(cpld2_intr),
    _DEVICE_ATTR(fan_card_intr),
    _DEVICE_ATTR(usb_pwr_oc),
    _DEVICE_ATTR(ssd1_pwr_oc),
    _DEVICE_ATTR(ssd2_pwr_oc),
    _DEVICE_ATTR(gnss_intr),
    _DEVICE_ATTR(hwm_nmi_intr),
    _DEVICE_ATTR(tsen_nmi_intr),
    _DEVICE_ATTR(tsen1_nmi_intr),
    _DEVICE_ATTR(tsen2_nmi_intr),
    _DEVICE_ATTR(tsen_alrt_intr),
    _DEVICE_ATTR(tsen1_alrt_intr),
    _DEVICE_ATTR(tsen2_alrt_intr),
    _DEVICE_ATTR(cpu_thrmtrip),
    _DEVICE_ATTR(vdd_core_vrhot),
    _DEVICE_ATTR(thermal_intr),
    _DEVICE_ATTR(mac_intr),
    _DEVICE_ATTR(cpu_nmi_intr),
    _DEVICE_ATTR(usb_pwr_oc_cpu),
    _DEVICE_ATTR(nmi_cpld_to_cpu),
    _DEVICE_ATTR(ptp_to_cpu_intr),
    _DEVICE_ATTR(eth_to_cpu_intr),
    _DEVICE_ATTR(thrm_cpld_to_cpu_intr),
    _DEVICE_ATTR(nmi_sys_to_bmc),
    _DEVICE_ATTR(thrm_chip_intr),
    _DEVICE_ATTR(cpld_to_cpu_intr),
    _DEVICE_ATTR(bits_intr_mask),
    _DEVICE_ATTR(dpll_sync_intr_mask),
    _DEVICE_ATTR(dpll_1588_intr_mask),
    _DEVICE_ATTR(ntm_intr_mask),
    _DEVICE_ATTR(cja_lol_intr_mask),
    _DEVICE_ATTR(i2c_nic_alrt_mask),
    _DEVICE_ATTR(fan_card_intr_mask),
    _DEVICE_ATTR(cpld2_intr_mask),
    _DEVICE_ATTR(psu2_intr_mask),
    _DEVICE_ATTR(psu1_intr_mask),
    _DEVICE_ATTR(hwm_nmi_intr_mask),
    _DEVICE_ATTR(gnss_intr_mask),
    _DEVICE_ATTR(ssd2_pwr_oc_mask),
    _DEVICE_ATTR(ssd1_pwr_oc_mask),
    _DEVICE_ATTR(usb_pwr_oc_mask),
    _DEVICE_ATTR(vdd_core_vrhot_mask),
    _DEVICE_ATTR(cpu_thrmtrip_mask),
    _DEVICE_ATTR(tsen2_alrt_intr_mask),
    _DEVICE_ATTR(tsen1_alrt_intr_mask),
    _DEVICE_ATTR(tsen_alrt_intr_mask),
    _DEVICE_ATTR(tsen2_nmi_intr_mask),
    _DEVICE_ATTR(tsen1_nmi_intr_mask),
    _DEVICE_ATTR(tsen_nmi_intr_mask),
    _DEVICE_ATTR(mac_intr_mask),
    _DEVICE_ATTR(cpu_nmi_intr_mask),
    _DEVICE_ATTR(cpld_to_cpu_intr_mask),
    _DEVICE_ATTR(thrm_chip_intr_mask),
    _DEVICE_ATTR(nmi_sys_to_bmc_mask),
    _DEVICE_ATTR(thrm_cpld_to_cpu_intr_mask),
    _DEVICE_ATTR(eth_to_cpu_intr_mask),
    _DEVICE_ATTR(ptp_to_cpu_intr_mask),
    _DEVICE_ATTR(nmi_cpld_to_cpu_mask),
    _DEVICE_ATTR(usb_pwr_oc_cpu_mask),
    _DEVICE_ATTR(clk_ptp_intr_event),
    _DEVICE_ATTR(phy_intr_event),
    _DEVICE_ATTR(top_brd_cpld_fru_intr_event),
    _DEVICE_ATTR(main_brd_cpld_intr_event),
    _DEVICE_ATTR(thermal_intr_event),
    _DEVICE_ATTR(mac_intr_event),
    _DEVICE_ATTR(cpu_nmi_intr_event),
    _DEVICE_ATTR(out_status_intr_event),
    _DEVICE_ATTR(mac_sys_reset),
    _DEVICE_ATTR(mac_qspi_reset),
    _DEVICE_ATTR(mac_reset),
    _DEVICE_ATTR(spi_bios_reset),
    _DEVICE_ATTR(btn_fp_reset),
    _DEVICE_ATTR(bios_flash_reset_ctrl),
    _DEVICE_ATTR(cpld_to_cpu_reset),
    _DEVICE_ATTR(cpld_to_bmc_sys_reset),
    _DEVICE_ATTR(bmc_pcie_reset),
    _DEVICE_ATTR(bmc_lpc_reset),
    _DEVICE_ATTR(cpu_mon_reset),
    _DEVICE_ATTR(bmc_phy_reset_ctrl),
    _DEVICE_ATTR(usb_pwr_en),
    _DEVICE_ATTR(usb_reset_ctrl),
    _DEVICE_ATTR(cja_reset),
    _DEVICE_ATTR(oe_cja),
    _DEVICE_ATTR(ja_wp),
    _DEVICE_ATTR(ja_reset_ctrl),
    _DEVICE_ATTR(i2c_mux_0x75_reset),
    _DEVICE_ATTR(i2c_mux_0x77_reset),
    _DEVICE_ATTR(i2c_mux_0x76_reset),
    _DEVICE_ATTR(i2c_mux_reset),
    _DEVICE_ATTR(nic1_pcie_reset),
    _DEVICE_ATTR(cpld1_to_cpld2_reset),
    _DEVICE_ATTR(nic_ctrl),
    _DEVICE_ATTR(psu1_eeprom_wp),
    _DEVICE_ATTR(psu2_eeprom_wp),
    _DEVICE_ATTR(psu_eeprom_ctrl),
    _DEVICE_ATTR(sys_pwr_reset),
    _DEVICE_ATTR(psu1_present),
    _DEVICE_ATTR(psu2_present),
    _DEVICE_ATTR(psu1_vin_pwrok),
    _DEVICE_ATTR(psu2_vin_pwrok),
    _DEVICE_ATTR(psu1_vout_pwrok),
    _DEVICE_ATTR(psu2_vout_pwrok),
    _DEVICE_ATTR(psu_status),
    _DEVICE_ATTR(daughter_brd_present),
    _DEVICE_ATTR(cpu_pwrgd),
    _DEVICE_ATTR(cpu_boot_done),
    _DEVICE_ATTR(wake_cpu_pcie),
    _DEVICE_ATTR(sys_pwr_status),
    _DEVICE_ATTR(phy_boot_ctrl),
    _DEVICE_ATTR(bmc_wdt1_reset),
    _DEVICE_ATTR(bmc_wdt2_reset),
    _DEVICE_ATTR(wd_status),
    _DEVICE_ATTR(bios_boot_sel0),
    _DEVICE_ATTR(bios_boot_sel1),
    _DEVICE_ATTR(timing_ctrl_status),
    _DEVICE_ATTR(i2c_rov_mux_sel),
    _DEVICE_ATTR(qspi_mac_mux_sel),
    _DEVICE_ATTR(i2c_cpld_mux_sel),
    _DEVICE_ATTR(uart_mux_sel),
    _DEVICE_ATTR(uart_cpu_bmc_mux_sel),
    _DEVICE_ATTR(mux_ctrl),
    _DEVICE_ATTR(icx_spi_mux_sel_1),
    _DEVICE_ATTR(icx_spi_mux_sel_2),
    _DEVICE_ATTR(icx_spi_mux_sel_3),
    _DEVICE_ATTR(bios_spi_mux),
    _DEVICE_ATTR(pwren_cpld_to_cpu),
    _DEVICE_ATTR(pwrbtn_cpld_to_cpu),
    _DEVICE_ATTR(pwr_system_ctrl),
    _DEVICE_ATTR(ufm_write),
    _DEVICE_ATTR(ufm_write_en),
    _DEVICE_ATTR(bios_boot_sel0_tgt),
    _DEVICE_ATTR(bios_boot_sel1_tgt),
    _DEVICE_ATTR(bios_boot_sel_tgt),
    _DEVICE_ATTR(led_fan_clr),
    _DEVICE_ATTR(led_clear),
    _DEVICE_ATTR(gnss_ant_pwren),
    _DEVICE_ATTR(gnss_reset),
    _DEVICE_ATTR(st_gnss_10m_out),
    _DEVICE_ATTR(gnss_ctrl),
    _DEVICE_ATTR(ts_pll_clk_en),
    _DEVICE_ATTR(p3v3_en_broadsync),
    _DEVICE_ATTR(timing_misc_ctrl),
    _DEVICE_ATTR(ext_reg_define),
    _DEVICE_ATTR(ext_reg_range),
    _DEVICE_ATTR(ext_ctrl),
    _DEVICE_ATTR(pwr_led_color),
    _DEVICE_ATTR(pwr_led_speed),
    _DEVICE_ATTR(pwr_led_blinking),
    _DEVICE_ATTR(pwr_led_on_off),
    _DEVICE_ATTR(cpld_system_led_pwr),
    _DEVICE_ATTR(fan_led_color),
    _DEVICE_ATTR(fan_led_speed),
    _DEVICE_ATTR(fan_led_blinking),
    _DEVICE_ATTR(fan_led_on_off),
    _DEVICE_ATTR(cpld_system_led_fan),
    _DEVICE_ATTR(system_led_ctrl_1),
    _DEVICE_ATTR(gnss_led_color),
    _DEVICE_ATTR(gnss_led_speed),
    _DEVICE_ATTR(gnss_led_blinking),
    _DEVICE_ATTR(gnss_led_on_off),
    _DEVICE_ATTR(cpld_system_led_gnss),
    _DEVICE_ATTR(sys_led_color),
    _DEVICE_ATTR(sys_led_speed),
    _DEVICE_ATTR(sys_led_blinking),
    _DEVICE_ATTR(sys_led_on_off),
    _DEVICE_ATTR(cpld_system_led_sys),
    _DEVICE_ATTR(system_led_ctrl_2),
    _DEVICE_ATTR(sync_led_color),
    _DEVICE_ATTR(sync_led_speed),
    _DEVICE_ATTR(sync_led_blinking),
    _DEVICE_ATTR(sync_led_on_off),
    _DEVICE_ATTR(cpld_system_led_sync),
    _DEVICE_ATTR(id_led_on_off),
    _DEVICE_ATTR(id_led_blinking),
    _DEVICE_ATTR(id_led_speed),
    _DEVICE_ATTR(cpld_system_led_id),
    _DEVICE_ATTR(system_led_ctrl_3),
    _DEVICE_ATTR(fan_led_1_2),
    _DEVICE_ATTR(fan2_led_on_off),
    _DEVICE_ATTR(fan2_led_blinking),
    _DEVICE_ATTR(fan2_led_speed),
    _DEVICE_ATTR(fan2_led_color),
    _DEVICE_ATTR(fan1_led_on_off),
    _DEVICE_ATTR(fan1_led_blinking),
    _DEVICE_ATTR(fan1_led_speed),
    _DEVICE_ATTR(fan1_led_color),
    _DEVICE_ATTR(fan_led_3_4),
    _DEVICE_ATTR(fan4_led_on_off),
    _DEVICE_ATTR(fan4_led_blinking),
    _DEVICE_ATTR(fan4_led_speed),
    _DEVICE_ATTR(fan4_led_color),
    _DEVICE_ATTR(fan3_led_on_off),
    _DEVICE_ATTR(fan3_led_blinking),
    _DEVICE_ATTR(fan3_led_speed),
    _DEVICE_ATTR(fan3_led_color),
    _DEVICE_ATTR(fan_led_5),
    _DEVICE_ATTR(fan5_led_on_off),
    _DEVICE_ATTR(fan5_led_blinking),
    _DEVICE_ATTR(fan5_led_speed),
    _DEVICE_ATTR(fan5_led_color),
    _DEVICE_ATTR(p5v_aux_pg),
    _DEVICE_ATTR(p3v3_pg),
    _DEVICE_ATTR(p1v8_clk_pg),
    _DEVICE_ATTR(p1v2_cpld2_pg),
    _DEVICE_ATTR(p2v5_cpld2_pg),
    _DEVICE_ATTR(p1v8_vdd0_pg),
    _DEVICE_ATTR(vdd_core_pg),
    _DEVICE_ATTR(p0v9_pvdd_pg),
    _DEVICE_ATTR(pwr_status_1),
    _DEVICE_ATTR(p0v75_trvdd100_pg),
    _DEVICE_ATTR(p0v75_trvdd50_pg),
    _DEVICE_ATTR(p0v9_trvdd_pg),
    _DEVICE_ATTR(p0v8_avdd_pg),
    _DEVICE_ATTR(p1v2_trvdd_pg),
    _DEVICE_ATTR(p1v2_pvdd_pg),
    _DEVICE_ATTR(p1v5_avdd_pg),
    _DEVICE_ATTR(p1v5_trxvdd_pg),
    _DEVICE_ATTR(pwr_status_2),
    _DEVICE_ATTR(p0v75_ddr_vddc_pg),
    _DEVICE_ATTR(p1v05_ddr_vddqx_pg),
    _DEVICE_ATTR(p1v8_ddr_pg),
    _DEVICE_ATTR(p0v5_ddr_vddq_pg),
    _DEVICE_ATTR(pwr_status_3),
    _DEVICE_ATTR(ocxo_id),
    _DEVICE_ATTR(gnss_modl_id),
    _DEVICE_ATTR(ocxo_gnss_id),
    _DEVICE_ATTR(ntm_reset),
    _DEVICE_ATTR(bits_reset),
    _DEVICE_ATTR(clk_ptp_reset),
    _DEVICE_ATTR(ts_pll_clk_source_sel),
    _DEVICE_ATTR(smb_1pps_dir_sel),
    _DEVICE_ATTR(clk_10m_ptp_in),
    _DEVICE_ATTR(smb_10m_input_en),
    _DEVICE_ATTR(ptp_tod_rs422_dir_ctrl),
    _DEVICE_ATTR(ptp_1pps_rs422_dir_ctrl),
    _DEVICE_ATTR(smb_10m_dir_sel),
    _DEVICE_ATTR(clk_timing_ctrl),
    _DEVICE_ATTR(gnss_ant_short),
    _DEVICE_ATTR(gnss_ant_open),
    _DEVICE_ATTR(gnss_ant_on),
    _DEVICE_ATTR(gnss_status),
    _DEVICE_ATTR(ntm_present),
    _DEVICE_ATTR(gnss_present),
    _DEVICE_ATTR(gnss_ant_pwr_st),
    _DEVICE_ATTR(timing_status),
    _DEVICE_ATTR(synce_ch_set),
    _DEVICE_ATTR(i2c_mux_reset_mb),
    _DEVICE_ATTR(bsp_debug),
    _DEVICE_ATTR(bsp_wp_access_count),
    NULL,
};

/* cpld1 attributes group */
static const struct attribute_group cpld1_group = {
    .attrs = cpld1_attributes,
};

static struct attribute *cpld2_attributes[] = {
    // cpld common
    _DEVICE_ATTR(cpld_version),
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(cpld_chip_type),
    _DEVICE_ATTR(event_ctrl),
    _DEVICE_ATTR(event_detect_ctrl),
    _DEVICE_ATTR(remote_i2c_reset),
    _DEVICE_ATTR(module_reset),

    // cpld2
    _DEVICE_ATTR(eth_0_present),
    _DEVICE_ATTR(eth_1_present),
    _DEVICE_ATTR(eth_2_present),
    _DEVICE_ATTR(eth_3_present),
    _DEVICE_ATTR(eth_4_present),
    _DEVICE_ATTR(eth_5_present),
    _DEVICE_ATTR(eth_6_present),
    _DEVICE_ATTR(eth_7_present),
    _DEVICE_ATTR(eth_8_present),
    _DEVICE_ATTR(eth_9_present),
    _DEVICE_ATTR(eth_10_present),
    _DEVICE_ATTR(eth_11_present),
    _DEVICE_ATTR(eth_12_present),
    _DEVICE_ATTR(eth_13_present),
    _DEVICE_ATTR(eth_14_present),
    _DEVICE_ATTR(eth_15_present),
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
    _DEVICE_ATTR(eth_0_intr),
    _DEVICE_ATTR(eth_1_intr),
    _DEVICE_ATTR(eth_2_intr),
    _DEVICE_ATTR(eth_3_intr),
    _DEVICE_ATTR(eth_4_rx_los),
    _DEVICE_ATTR(eth_5_rx_los),
    _DEVICE_ATTR(eth_6_rx_los),
    _DEVICE_ATTR(eth_7_rx_los),
    _DEVICE_ATTR(eth_8_rx_los),
    _DEVICE_ATTR(eth_9_rx_los),
    _DEVICE_ATTR(eth_10_rx_los),
    _DEVICE_ATTR(eth_11_rx_los),
    _DEVICE_ATTR(eth_12_rx_los),
    _DEVICE_ATTR(eth_13_rx_los),
    _DEVICE_ATTR(eth_14_rx_los),
    _DEVICE_ATTR(eth_15_rx_los),
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
    _DEVICE_ATTR(eth_0_fuse),
    _DEVICE_ATTR(eth_1_fuse),
    _DEVICE_ATTR(eth_2_fuse),
    _DEVICE_ATTR(eth_3_fuse),
    _DEVICE_ATTR(eth_4_tx_flt),
    _DEVICE_ATTR(eth_5_tx_flt),
    _DEVICE_ATTR(eth_6_tx_flt),
    _DEVICE_ATTR(eth_7_tx_flt),
    _DEVICE_ATTR(eth_8_tx_flt),
    _DEVICE_ATTR(eth_9_tx_flt),
    _DEVICE_ATTR(eth_10_tx_flt),
    _DEVICE_ATTR(eth_11_tx_flt),
    _DEVICE_ATTR(eth_12_tx_flt),
    _DEVICE_ATTR(eth_13_tx_flt),
    _DEVICE_ATTR(eth_14_tx_flt),
    _DEVICE_ATTR(eth_15_tx_flt),
    _DEVICE_ATTR(eth_16_tx_flt),
    _DEVICE_ATTR(eth_17_tx_flt),
    _DEVICE_ATTR(eth_18_tx_flt),
    _DEVICE_ATTR(eth_19_tx_flt),
    _DEVICE_ATTR(eth_20_tx_flt),
    _DEVICE_ATTR(eth_21_tx_flt),
    _DEVICE_ATTR(eth_22_tx_flt),
    _DEVICE_ATTR(eth_23_tx_flt),
    _DEVICE_ATTR(eth_24_tx_flt),
    _DEVICE_ATTR(eth_25_tx_flt),
    _DEVICE_ATTR(eth_26_tx_flt),
    _DEVICE_ATTR(eth_27_tx_flt),
    _DEVICE_ATTR(eth_0_present_mask),
    _DEVICE_ATTR(eth_1_present_mask),
    _DEVICE_ATTR(eth_2_present_mask),
    _DEVICE_ATTR(eth_3_present_mask),
    _DEVICE_ATTR(eth_4_present_mask),
    _DEVICE_ATTR(eth_5_present_mask),
    _DEVICE_ATTR(eth_6_present_mask),
    _DEVICE_ATTR(eth_7_present_mask),
    _DEVICE_ATTR(eth_8_present_mask),
    _DEVICE_ATTR(eth_9_present_mask),
    _DEVICE_ATTR(eth_10_present_mask),
    _DEVICE_ATTR(eth_11_present_mask),
    _DEVICE_ATTR(eth_12_present_mask),
    _DEVICE_ATTR(eth_13_present_mask),
    _DEVICE_ATTR(eth_14_present_mask),
    _DEVICE_ATTR(eth_15_present_mask),
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
    _DEVICE_ATTR(eth_0_intr_mask),
    _DEVICE_ATTR(eth_1_intr_mask),
    _DEVICE_ATTR(eth_2_intr_mask),
    _DEVICE_ATTR(eth_3_intr_mask),
    _DEVICE_ATTR(eth_4_rx_los_mask),
    _DEVICE_ATTR(eth_5_rx_los_mask),
    _DEVICE_ATTR(eth_6_rx_los_mask),
    _DEVICE_ATTR(eth_7_rx_los_mask),
    _DEVICE_ATTR(eth_8_rx_los_mask),
    _DEVICE_ATTR(eth_9_rx_los_mask),
    _DEVICE_ATTR(eth_10_rx_los_mask),
    _DEVICE_ATTR(eth_11_rx_los_mask),
    _DEVICE_ATTR(eth_12_rx_los_mask),
    _DEVICE_ATTR(eth_13_rx_los_mask),
    _DEVICE_ATTR(eth_14_rx_los_mask),
    _DEVICE_ATTR(eth_15_rx_los_mask),
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
    _DEVICE_ATTR(eth_0_fuse_mask),
    _DEVICE_ATTR(eth_1_fuse_mask),
    _DEVICE_ATTR(eth_2_fuse_mask),
    _DEVICE_ATTR(eth_3_fuse_mask),
    _DEVICE_ATTR(eth_4_tx_flt_mask),
    _DEVICE_ATTR(eth_5_tx_flt_mask),
    _DEVICE_ATTR(eth_6_tx_flt_mask),
    _DEVICE_ATTR(eth_7_tx_flt_mask),
    _DEVICE_ATTR(eth_8_tx_flt_mask),
    _DEVICE_ATTR(eth_9_tx_flt_mask),
    _DEVICE_ATTR(eth_10_tx_flt_mask),
    _DEVICE_ATTR(eth_11_tx_flt_mask),
    _DEVICE_ATTR(eth_12_tx_flt_mask),
    _DEVICE_ATTR(eth_13_tx_flt_mask),
    _DEVICE_ATTR(eth_14_tx_flt_mask),
    _DEVICE_ATTR(eth_15_tx_flt_mask),
    _DEVICE_ATTR(eth_16_tx_flt_mask),
    _DEVICE_ATTR(eth_17_tx_flt_mask),
    _DEVICE_ATTR(eth_18_tx_flt_mask),
    _DEVICE_ATTR(eth_19_tx_flt_mask),
    _DEVICE_ATTR(eth_20_tx_flt_mask),
    _DEVICE_ATTR(eth_21_tx_flt_mask),
    _DEVICE_ATTR(eth_22_tx_flt_mask),
    _DEVICE_ATTR(eth_23_tx_flt_mask),
    _DEVICE_ATTR(eth_24_tx_flt_mask),
    _DEVICE_ATTR(eth_25_tx_flt_mask),
    _DEVICE_ATTR(eth_26_tx_flt_mask),
    _DEVICE_ATTR(eth_27_tx_flt_mask),
    _DEVICE_ATTR(qsfp28_0_3_present_event),
    _DEVICE_ATTR(sfp28_4_11_present_event),
    _DEVICE_ATTR(sfp28_12_19_present_event),
    _DEVICE_ATTR(sfp56_20_27_present_event),
    _DEVICE_ATTR(qsfp28_0_3_intr_event),
    _DEVICE_ATTR(sfp28_4_11_rx_los_event),
    _DEVICE_ATTR(sfp28_12_19_rx_los_event),
    _DEVICE_ATTR(sfp56_20_27_rx_los_event),
    _DEVICE_ATTR(qsfp28_0_3_fuse_event),
    _DEVICE_ATTR(sfp28_4_11_tx_flt_event),
    _DEVICE_ATTR(sfp28_12_19_tx_flt_event),
    _DEVICE_ATTR(sfp56_20_27_tx_flt_event),
    _DEVICE_ATTR(eth_0_present_event),
    _DEVICE_ATTR(eth_1_present_event),
    _DEVICE_ATTR(eth_2_present_event),
    _DEVICE_ATTR(eth_3_present_event),
    _DEVICE_ATTR(eth_4_present_event),
    _DEVICE_ATTR(eth_5_present_event),
    _DEVICE_ATTR(eth_6_present_event),
    _DEVICE_ATTR(eth_7_present_event),
    _DEVICE_ATTR(eth_8_present_event),
    _DEVICE_ATTR(eth_9_present_event),
    _DEVICE_ATTR(eth_10_present_event),
    _DEVICE_ATTR(eth_11_present_event),
    _DEVICE_ATTR(eth_12_present_event),
    _DEVICE_ATTR(eth_13_present_event),
    _DEVICE_ATTR(eth_14_present_event),
    _DEVICE_ATTR(eth_15_present_event),
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
    _DEVICE_ATTR(eth_0_reset),
    _DEVICE_ATTR(eth_1_reset),
    _DEVICE_ATTR(eth_2_reset),
    _DEVICE_ATTR(eth_3_reset),
    _DEVICE_ATTR(eth_0_lpmode),
    _DEVICE_ATTR(eth_1_lpmode),
    _DEVICE_ATTR(eth_2_lpmode),
    _DEVICE_ATTR(eth_3_lpmode),
    _DEVICE_ATTR(ssd1_m2_config),
    _DEVICE_ATTR(ssd2_m2_config),
    _DEVICE_ATTR(cpu_ssd1_perst),
    _DEVICE_ATTR(cpu_ssd2_perst),
    _DEVICE_ATTR(pwren_p3v3_ssd1),
    _DEVICE_ATTR(pwren_p3v3_ssd2),
    _DEVICE_ATTR(eth_4_tx_disable),
    _DEVICE_ATTR(eth_5_tx_disable),
    _DEVICE_ATTR(eth_6_tx_disable),
    _DEVICE_ATTR(eth_7_tx_disable),
    _DEVICE_ATTR(eth_8_tx_disable),
    _DEVICE_ATTR(eth_9_tx_disable),
    _DEVICE_ATTR(eth_10_tx_disable),
    _DEVICE_ATTR(eth_11_tx_disable),
    _DEVICE_ATTR(eth_12_tx_disable),
    _DEVICE_ATTR(eth_13_tx_disable),
    _DEVICE_ATTR(eth_14_tx_disable),
    _DEVICE_ATTR(eth_15_tx_disable),
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
    _DEVICE_ATTR(eth_4_rate_select),
    _DEVICE_ATTR(eth_5_rate_select),
    _DEVICE_ATTR(eth_6_rate_select),
    _DEVICE_ATTR(eth_7_rate_select),
    _DEVICE_ATTR(eth_8_rate_select),
    _DEVICE_ATTR(eth_9_rate_select),
    _DEVICE_ATTR(eth_10_rate_select),
    _DEVICE_ATTR(eth_11_rate_select),
    _DEVICE_ATTR(eth_12_rate_select),
    _DEVICE_ATTR(eth_13_rate_select),
    _DEVICE_ATTR(eth_14_rate_select),
    _DEVICE_ATTR(eth_15_rate_select),
    _DEVICE_ATTR(eth_16_rate_select),
    _DEVICE_ATTR(eth_17_rate_select),
    _DEVICE_ATTR(eth_18_rate_select),
    _DEVICE_ATTR(eth_19_rate_select),
    _DEVICE_ATTR(eth_20_rate_select),
    _DEVICE_ATTR(eth_21_rate_select),
    _DEVICE_ATTR(eth_22_rate_select),
    _DEVICE_ATTR(eth_23_rate_select),
    _DEVICE_ATTR(eth_24_rate_select),
    _DEVICE_ATTR(eth_25_rate_select),
    _DEVICE_ATTR(eth_26_rate_select),
    _DEVICE_ATTR(eth_27_rate_select),
    _DEVICE_ATTR(usb_ctrl_present),
    _DEVICE_ATTR(nvme_ssd2_present),
    _DEVICE_ATTR(nvme_ssd1_present),
    _DEVICE_ATTR(eth_4_5_oc),
    _DEVICE_ATTR(eth_6_7_oc),
    _DEVICE_ATTR(eth_8_9_oc),
    _DEVICE_ATTR(eth_10_11_oc),
    _DEVICE_ATTR(eth_12_13_oc),
    _DEVICE_ATTR(eth_14_15_oc),
    _DEVICE_ATTR(eth_16_17_oc),
    _DEVICE_ATTR(eth_18_19_oc),
    _DEVICE_ATTR(eth_20_oc),
    _DEVICE_ATTR(eth_21_oc),
    _DEVICE_ATTR(eth_22_oc),
    _DEVICE_ATTR(eth_23_oc),
    _DEVICE_ATTR(eth_24_oc),
    _DEVICE_ATTR(eth_25_oc),
    _DEVICE_ATTR(eth_26_oc),
    _DEVICE_ATTR(eth_27_oc),
    _DEVICE_ATTR(eth_0_i2c_stuck),
    _DEVICE_ATTR(eth_1_i2c_stuck),
    _DEVICE_ATTR(eth_2_i2c_stuck),
    _DEVICE_ATTR(eth_3_i2c_stuck),
    _DEVICE_ATTR(eth_4_i2c_stuck),
    _DEVICE_ATTR(eth_5_i2c_stuck),
    _DEVICE_ATTR(eth_6_i2c_stuck),
    _DEVICE_ATTR(eth_7_i2c_stuck),
    _DEVICE_ATTR(eth_8_i2c_stuck),
    _DEVICE_ATTR(eth_9_i2c_stuck),
    _DEVICE_ATTR(eth_10_i2c_stuck),
    _DEVICE_ATTR(eth_11_i2c_stuck),
    _DEVICE_ATTR(eth_12_i2c_stuck),
    _DEVICE_ATTR(eth_13_i2c_stuck),
    _DEVICE_ATTR(eth_14_i2c_stuck),
    _DEVICE_ATTR(eth_15_i2c_stuck),
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
    _DEVICE_ATTR(cpu_legacy_side_i2c_stuck),
    _DEVICE_ATTR(cpu_i2c_stuck_status),
    _DEVICE_ATTR(eth_0_i2c_stuck_mask),
    _DEVICE_ATTR(eth_1_i2c_stuck_mask),
    _DEVICE_ATTR(eth_2_i2c_stuck_mask),
    _DEVICE_ATTR(eth_3_i2c_stuck_mask),
    _DEVICE_ATTR(eth_4_i2c_stuck_mask),
    _DEVICE_ATTR(eth_5_i2c_stuck_mask),
    _DEVICE_ATTR(eth_6_i2c_stuck_mask),
    _DEVICE_ATTR(eth_7_i2c_stuck_mask),
    _DEVICE_ATTR(eth_8_i2c_stuck_mask),
    _DEVICE_ATTR(eth_9_i2c_stuck_mask),
    _DEVICE_ATTR(eth_10_i2c_stuck_mask),
    _DEVICE_ATTR(eth_11_i2c_stuck_mask),
    _DEVICE_ATTR(eth_12_i2c_stuck_mask),
    _DEVICE_ATTR(eth_13_i2c_stuck_mask),
    _DEVICE_ATTR(eth_14_i2c_stuck_mask),
    _DEVICE_ATTR(eth_15_i2c_stuck_mask),
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
    _DEVICE_ATTR(cpu_legacy_side_i2c_stuck_mask),
    _DEVICE_ATTR(cpu_i2c_stuck_mask),
    _DEVICE_ATTR(qsfp28_0_3_i2c_stuck_event),
    _DEVICE_ATTR(sfp28_4_11_i2c_stuck_event),
    _DEVICE_ATTR(sfp28_12_19_i2c_stuck_event),
    _DEVICE_ATTR(sfp56_20_27_i2c_stuck_event),
    _DEVICE_ATTR(cpu_i2c_stuck_event),
    _DEVICE_ATTR(bsp_debug),
    NULL,
};

/* cpld2 attributes group */
static const struct attribute_group cpld2_group = {
    .attrs = cpld2_attributes,
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
        .name = "x86_64_ufispace_s9520_28xc_cpld",
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
MODULE_DESCRIPTION("x86_64_ufispace_s9520_28xc_cpld driver");
MODULE_VERSION("1.0.0");
MODULE_LICENSE("GPL");

module_init(cpld_init);
module_exit(cpld_exit);
