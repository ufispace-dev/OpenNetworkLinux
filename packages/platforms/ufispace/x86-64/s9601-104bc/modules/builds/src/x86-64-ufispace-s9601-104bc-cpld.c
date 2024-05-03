/*
 * A i2c cpld driver for the ufispace_s9601_104bc
 *
 * Copyright (C) 2022 UfiSpace Technology Corporation.
 * Nonodark Huang <nonodark.huang@ufispace.com>
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
#include "x86-64-ufispace-s9601-104bc-cpld.h"

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

    // CPLD 1
    CPLD_SKU_ID,
    CPLD_HW_REV,
    CPLD_DEPH_REV,
    CPLD_BUILD_REV,
    CPLD_ID_TYPE,
    CPLD_SKU_EXT,
    CPLD_MAC_OP2_INTR,
    CPLD_10GPHY_INTR,
    CPLD_CPLD_FRU_INTR,
    CPLD_MGMT_SFP_PORT_STATUS,
    CPLD_MISC_INTR,
    CPLD_SYSTEM_INTR,
    CPLD_MAC_OP2_INTR_MASK,
    CPLD_10GPHY_INTR_MASK,
    CPLD_CPLD_FRU_INTR_MASK,
    CPLD_MGMT_SFP_PORT_STATUS_MASK,
    CPLD_MISC_INTR_MASK,
    CPLD_SYSTEM_INTR_MASK,
    CPLD_MAC_OP2_INTR_EVENT,
    CPLD_10GPHY_INTR_EVENT,
    CPLD_CPLD_FRU_INTR_EVENT,
    CPLD_MGMT_SFP_PORT_STATUS_EVENT,
    CPLD_MISC_INTR_EVENT,
    CPLD_SYSTEM_INTR_EVENT,
    CPLD_MAC_OP2_RST,
    CPLD_PHY_RST,
    CPLD_BMC_NTM_RST,
    CPLD_USB_RST,
    CPLD_CPLD_RST,
    CPLD_MUX_RST_1,
    CPLD_MUX_RST_2,
    CPLD_MISC_RST,
    CPLD_PUSHBTN,
    CPLD_CPU_MISC,
    CPLD_PSU_STATUS,
    CPLD_SYS_PW_STATUS,
    CPLD_MAC_STATUS_1,
    CPLD_MAC_STATUS_2,
    CPLD_MAC_STATUS_3,
    CPLD_MGMT_SFP_PORT_CONF,
    CPLD_MISC,
    CPLD_MUX_CTRL,
    CPLD_SYSTEM_LED_PSU,
    CPLD_SYSTEM_LED_SYS,
    CPLD_SYSTEM_LED_SYNC,
    CPLD_SYSTEM_LED_FAN,
    CPLD_SYSTEM_LED_ID,
    CPLD_MAC_PG,
    CPLD_OP2_PG,
    CPLD_MISC_PG,
    CPLD_HBM_PW_EN,
    CPLD_EXT_BYTE,
    CPLD_BRD_ID_DEF,

    // CPLD 2
    CPLD_SFP_PORT_0_7_PRES,
    CPLD_SFP_PORT_8_15_PRES,
    CPLD_SFP_PORT_48_55_PRES,
    CPLD_SFP_PORT_56_63_PRES,
    CPLD_SFP_PORT_0_7_PRES_MASK,
    CPLD_SFP_PORT_8_15_PRES_MASK,
    CPLD_SFP_PORT_48_55_PRES_MASK,
    CPLD_SFP_PORT_56_63_PRES_MASK,
    CPLD_SFP_PORT_0_7_PLUG_EVENT,
    CPLD_SFP_PORT_8_15_PLUG_EVENT,
    CPLD_SFP_PORT_48_55_PLUG_EVENT,
    CPLD_SFP_PORT_56_63_PLUG_EVENT,
    CPLD_SFP_PORT_0_7_TX_DISABLE,
    CPLD_SFP_PORT_8_15_TX_DISABLE,
    CPLD_SFP_PORT_48_55_TX_DISABLE,
    CPLD_SFP_PORT_56_63_TX_DISABLE,
    CPLD_SFP_PORT_0_7_RX_LOS,
    CPLD_SFP_PORT_8_15_RX_LOS,
    CPLD_SFP_PORT_48_55_RX_LOS,
    CPLD_SFP_PORT_56_63_RX_LOS,
    CPLD_SFP_PORT_0_7_TX_FAULT,
    CPLD_SFP_PORT_8_15_TX_FAULT,
    CPLD_SFP_PORT_48_55_TX_FAULT,
    CPLD_SFP_PORT_56_63_TX_FAULT,

    // CPLD 3
    CPLD_SFP_PORT_16_23_PRES,
    CPLD_SFP_PORT_24_31_PRES,
    CPLD_SFP_PORT_64_71_PRES,
    CPLD_SFP_PORT_72_79_PRES,
    CPLD_SFP_PORT_16_23_PRES_MASK,
    CPLD_SFP_PORT_24_31_PRES_MASK,
    CPLD_SFP_PORT_64_71_PRES_MASK,
    CPLD_SFP_PORT_72_79_PRES_MASK,
    CPLD_SFP_PORT_16_23_PLUG_EVENT,
    CPLD_SFP_PORT_24_31_PLUG_EVENT,
    CPLD_SFP_PORT_64_71_PLUG_EVENT,
    CPLD_SFP_PORT_72_79_PLUG_EVENT,
    CPLD_SFP_PORT_16_23_TX_DISABLE,
    CPLD_SFP_PORT_24_31_TX_DISABLE,
    CPLD_SFP_PORT_64_71_TX_DISABLE,
    CPLD_SFP_PORT_72_79_TX_DISABLE,
    CPLD_SFP_PORT_16_23_RX_LOS,
    CPLD_SFP_PORT_24_31_RX_LOS,
    CPLD_SFP_PORT_64_71_RX_LOS,
    CPLD_SFP_PORT_72_79_RX_LOS,
    CPLD_SFP_PORT_16_23_TX_FAULT,
    CPLD_SFP_PORT_24_31_TX_FAULT,
    CPLD_SFP_PORT_64_71_TX_FAULT,
    CPLD_SFP_PORT_72_79_TX_FAULT,

    // CPLD 4
    CPLD_SFP_PORT_32_39_PRES,
    CPLD_SFP_PORT_40_43_PRES,
    CPLD_SFP_PORT_80_87_PRES,
    CPLD_SFP_PORT_88_91_PRES,
    CPLD_SFP_PORT_32_39_PRES_MASK,
    CPLD_SFP_PORT_40_43_PRES_MASK,
    CPLD_SFP_PORT_80_87_PRES_MASK,
    CPLD_SFP_PORT_88_91_PRES_MASK,
    CPLD_SFP_PORT_32_39_PLUG_EVENT,
    CPLD_SFP_PORT_40_43_PLUG_EVENT,
    CPLD_SFP_PORT_80_87_PLUG_EVENT,
    CPLD_SFP_PORT_88_91_PLUG_EVENT,
    CPLD_SFP_PORT_32_39_TX_DISABLE,
    CPLD_SFP_PORT_40_43_TX_DISABLE,
    CPLD_SFP_PORT_80_87_TX_DISABLE,
    CPLD_SFP_PORT_88_91_TX_DISABLE,
    CPLD_SFP_PORT_32_39_RX_LOS,
    CPLD_SFP_PORT_40_43_RX_LOS,
    CPLD_SFP_PORT_80_87_RX_LOS,
    CPLD_SFP_PORT_88_91_RX_LOS,
    CPLD_SFP_PORT_32_39_TX_FAULT,
    CPLD_SFP_PORT_40_43_TX_FAULT,
    CPLD_SFP_PORT_80_87_TX_FAULT,
    CPLD_SFP_PORT_88_91_TX_FAULT,

    // CPLD 5
    CPLD_QSFP_PORT_96_103_INTR,
    CPLD_SFP_PORT_44_47_PRES,
    CPLD_SFP_PORT_92_95_PRES,
    CPLD_QSFP_PORT_96_103_PRES,
    CPLD_QSFP_PORT_96_103_INTR_MASK,
    CPLD_SFP_PORT_44_47_PRES_MASK,
    CPLD_SFP_PORT_92_95_PRES_MASK,
    CPLD_QSFP_PORT_96_103_PRES_MASK,
    CPLD_QSFP_PORT_96_103_INTR_EVENT,
    CPLD_SFP_PORT_44_47_PLUG_EVENT,
    CPLD_SFP_PORT_92_95_PLUG_EVENT,
    CPLD_QSFP_PORT_96_103_PLUG_EVENT,
    CPLD_SFP_PORT_44_47_TX_DISABLE,
    CPLD_SFP_PORT_92_95_TX_DISABLE,
    CPLD_QSFP_PORT_96_103_RST,
    CPLD_QSFP_PORT_96_103_MODSEL,
    CPLD_QSFP_PORT_96_103_LPMODE,
    CPLD_SFP_PORT_44_47_RX_LOS,
    CPLD_SFP_PORT_92_95_RX_LOS,
    CPLD_SFP_PORT_44_47_TX_FAULT,
    CPLD_SFP_PORT_92_95_TX_FAULT,

    //BSP DEBUG
    BSP_DEBUG
};

enum data_type {
    DATA_HEX,
    DATA_DEC,
    DATA_UNK,
};

typedef struct  {
    u8 reg;
    u8 mask;
    u8 data_type;
} attr_reg_map_t;

static attr_reg_map_t attr_reg[]= {

    // CPLD Common
    [CPLD_MINOR_VER] = {CPLD_VERSION_REG          , MASK_0011_1111, DATA_DEC},
    [CPLD_MAJOR_VER] = {CPLD_VERSION_REG          , MASK_1100_0000, DATA_DEC},
    [CPLD_ID]        = {CPLD_ID_REG               , MASK_ALL      , DATA_DEC},
    [CPLD_BUILD_VER] = {CPLD_SUB_VERSION_REG      , MASK_ALL      , DATA_DEC},
    [CPLD_VERSION_H] = {CPLD_NONE_REG             , MASK_NONE     , DATA_UNK},

    //CPLD 1
    [CPLD_SKU_ID]                     = {CPLD_SKU_ID_REG                    , MASK_ALL      , DATA_DEC},
    [CPLD_HW_REV]                     = {CPLD_HW_REV_REG                    , MASK_0000_0011, DATA_DEC},
    [CPLD_DEPH_REV]                   = {CPLD_HW_REV_REG                    , MASK_0000_0100, DATA_DEC},
    [CPLD_BUILD_REV]                  = {CPLD_HW_REV_REG                    , MASK_0011_1000, DATA_DEC},
    [CPLD_ID_TYPE]                    = {CPLD_HW_REV_REG                    , MASK_1000_0000, DATA_DEC},
    [CPLD_SKU_EXT]                    = {CPLD_SKU_EXT_REG                   , MASK_ALL      , DATA_HEX},
    [CPLD_MAC_OP2_INTR]               = {CPLD_MAC_OP2_INTR_REG              , MASK_ALL      , DATA_HEX},
    [CPLD_10GPHY_INTR]                = {CPLD_10GPHY_INTR_REG               , MASK_ALL      , DATA_HEX},
    [CPLD_CPLD_FRU_INTR]              = {CPLD_CPLD_FRU_INTR_REG             , MASK_ALL      , DATA_HEX},
    [CPLD_MGMT_SFP_PORT_STATUS]       = {CPLD_MGMT_SFP_PORT_STATUS_REG      , MASK_ALL      , DATA_HEX},
    [CPLD_MISC_INTR]                  = {CPLD_MISC_INTR_REG                 , MASK_ALL      , DATA_HEX},
    [CPLD_SYSTEM_INTR]                = {CPLD_SYSTEM_INTR_REG               , MASK_ALL      , DATA_HEX},
    [CPLD_MAC_OP2_INTR_MASK]          = {CPLD_MAC_OP2_INTR_MASK_REG         , MASK_ALL      , DATA_HEX},
    [CPLD_10GPHY_INTR_MASK]           = {CPLD_10GPHY_INTR_MASK_REG          , MASK_ALL      , DATA_HEX},
    [CPLD_CPLD_FRU_INTR_MASK]         = {CPLD_CPLD_FRU_INTR_MASK_REG        , MASK_ALL      , DATA_HEX},
    [CPLD_MGMT_SFP_PORT_STATUS_MASK]  = {CPLD_MGMT_SFP_PORT_STATUS_MASK_REG , MASK_ALL      , DATA_HEX},
    [CPLD_MISC_INTR_MASK]             = {CPLD_MISC_INTR_MASK_REG            , MASK_ALL      , DATA_HEX},
    [CPLD_SYSTEM_INTR_MASK]           = {CPLD_SYSTEM_INTR_MASK_REG          , MASK_ALL      , DATA_HEX},
    [CPLD_MAC_OP2_INTR_EVENT]         = {CPLD_MAC_OP2_INTR_EVENT_REG        , MASK_ALL      , DATA_HEX},
    [CPLD_10GPHY_INTR_EVENT]          = {CPLD_10GPHY_INTR_EVENT_REG         , MASK_ALL      , DATA_HEX},
    [CPLD_CPLD_FRU_INTR_EVENT]        = {CPLD_CPLD_FRU_INTR_EVENT_REG       , MASK_ALL      , DATA_HEX},
    [CPLD_MGMT_SFP_PORT_STATUS_EVENT] = {CPLD_MGMT_SFP_PORT_STATUS_EVENT_REG, MASK_ALL      , DATA_HEX},
    [CPLD_MISC_INTR_EVENT]            = {CPLD_MISC_INTR_EVENT_REG           , MASK_ALL      , DATA_HEX},
    [CPLD_SYSTEM_INTR_EVENT]          = {CPLD_SYSTEM_INTR_EVENT_REG         , MASK_ALL      , DATA_HEX},
    [CPLD_MAC_OP2_RST]                = {CPLD_MAC_OP2_RST_REG               , MASK_ALL      , DATA_HEX},
    [CPLD_PHY_RST]                    = {CPLD_PHY_RST_REG                   , MASK_ALL      , DATA_HEX},
    [CPLD_BMC_NTM_RST]                = {CPLD_BMC_NTM_RST_REG               , MASK_ALL      , DATA_HEX},
    [CPLD_USB_RST]                    = {CPLD_USB_RST_REG                   , MASK_ALL      , DATA_HEX},
    [CPLD_CPLD_RST]                   = {CPLD_CPLD_RST_REG                  , MASK_ALL      , DATA_HEX},
    [CPLD_MUX_RST_1]                  = {CPLD_MUX_RST_1_REG                 , MASK_ALL      , DATA_HEX},
    [CPLD_MUX_RST_2]                  = {CPLD_MUX_RST_2_REG                 , MASK_ALL      , DATA_HEX},
    [CPLD_MISC_RST]                   = {CPLD_MISC_RST_REG                  , MASK_ALL      , DATA_HEX},
    [CPLD_PUSHBTN]                    = {CPLD_PUSHBTN_REG                   , MASK_ALL      , DATA_HEX},
    [CPLD_CPU_MISC]                   = {CPLD_CPU_MISC_REG                  , MASK_ALL      , DATA_HEX},
    [CPLD_PSU_STATUS]                 = {CPLD_PSU_STATUS_REG                , MASK_ALL      , DATA_HEX},
    [CPLD_SYS_PW_STATUS]              = {CPLD_SYS_PW_STATUS_REG             , MASK_ALL      , DATA_HEX},
    [CPLD_MAC_STATUS_1]               = {CPLD_MAC_STATUS_1_REG              , MASK_ALL      , DATA_HEX},
    [CPLD_MAC_STATUS_2]               = {CPLD_MAC_STATUS_2_REG              , MASK_ALL      , DATA_HEX},
    [CPLD_MAC_STATUS_3]               = {CPLD_MAC_STATUS_3_REG              , MASK_ALL      , DATA_HEX},
    [CPLD_MGMT_SFP_PORT_CONF]         = {CPLD_MGMT_SFP_PORT_CONF_REG        , MASK_ALL      , DATA_HEX},
    [CPLD_MISC]                       = {CPLD_MISC_REG                      , MASK_ALL      , DATA_HEX},
    [CPLD_MUX_CTRL]                   = {CPLD_MUX_CTRL_REG                  , MASK_ALL      , DATA_HEX},
    [CPLD_SYSTEM_LED_PSU]             = {CPLD_SYSTEM_LED_PSU_REG            , MASK_ALL      , DATA_HEX},
    [CPLD_SYSTEM_LED_SYS]             = {CPLD_SYSTEM_LED_SYS_REG            , MASK_ALL      , DATA_HEX},
    [CPLD_SYSTEM_LED_SYNC]            = {CPLD_SYSTEM_LED_SYNC_REG           , MASK_ALL      , DATA_HEX},
    [CPLD_SYSTEM_LED_FAN]             = {CPLD_SYSTEM_LED_FAN_REG            , MASK_ALL      , DATA_HEX},
    [CPLD_SYSTEM_LED_ID]              = {CPLD_SYSTEM_LED_ID_REG             , MASK_ALL      , DATA_HEX},
    [CPLD_MAC_PG]                     = {CPLD_MAC_PG_REG                    , MASK_ALL      , DATA_HEX},
    [CPLD_OP2_PG]                     = {CPLD_OP2_PG_REG                    , MASK_ALL      , DATA_HEX},
    [CPLD_MISC_PG]                    = {CPLD_MISC_PG_REG                   , MASK_ALL      , DATA_HEX},
    [CPLD_HBM_PW_EN]                  = {CPLD_HBM_PW_EN_REG                 , MASK_ALL      , DATA_HEX},
    [CPLD_EXT_BYTE]                   = {CPLD_EXT_BYTE_REG                  , MASK_ALL      , DATA_HEX},
    [CPLD_BRD_ID_DEF]                 = {CPLD_EXT_BYTE_REG                  , MASK_1100_0000, DATA_DEC},

    //CPLD 2
    [CPLD_SFP_PORT_0_7_PRES]          = {CPLD_SFP_PORT_0_7_PRES_REG          , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_8_15_PRES]         = {CPLD_SFP_PORT_8_15_PRES_REG         , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_48_55_PRES]        = {CPLD_SFP_PORT_48_55_PRES_REG        , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_56_63_PRES]        = {CPLD_SFP_PORT_56_63_PRES_REG        , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_0_7_PRES_MASK]     = {CPLD_SFP_PORT_0_7_PRES_MASK_REG     , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_8_15_PRES_MASK]    = {CPLD_SFP_PORT_8_15_PRES_MASK_REG    , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_48_55_PRES_MASK]   = {CPLD_SFP_PORT_48_55_PRES_MASK_REG   , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_56_63_PRES_MASK]   = {CPLD_SFP_PORT_56_63_PRES_MASK_REG   , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_0_7_PLUG_EVENT]    = {CPLD_SFP_PORT_0_7_PLUG_EVENT_REG    , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_8_15_PLUG_EVENT]   = {CPLD_SFP_PORT_8_15_PLUG_EVENT_REG   , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_48_55_PLUG_EVENT]  = {CPLD_SFP_PORT_48_55_PLUG_EVENT_REG  , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_56_63_PLUG_EVENT]  = {CPLD_SFP_PORT_56_63_PLUG_EVENT_REG  , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_0_7_TX_DISABLE]    = {CPLD_SFP_PORT_0_7_TX_DISABLE_REG    , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_8_15_TX_DISABLE]   = {CPLD_SFP_PORT_8_15_TX_DISABLE_REG   , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_48_55_TX_DISABLE]  = {CPLD_SFP_PORT_48_55_TX_DISABLE_REG  , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_56_63_TX_DISABLE]  = {CPLD_SFP_PORT_56_63_TX_DISABLE_REG  , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_0_7_RX_LOS]        = {CPLD_SFP_PORT_0_7_RX_LOS_REG        , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_8_15_RX_LOS]       = {CPLD_SFP_PORT_8_15_RX_LOS_REG       , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_48_55_RX_LOS]      = {CPLD_SFP_PORT_48_55_RX_LOS_REG      , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_56_63_RX_LOS]      = {CPLD_SFP_PORT_56_63_RX_LOS_REG      , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_0_7_TX_FAULT]      = {CPLD_SFP_PORT_0_7_TX_FAULT_REG      , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_8_15_TX_FAULT]     = {CPLD_SFP_PORT_8_15_TX_FAULT_REG     , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_48_55_TX_FAULT]    = {CPLD_SFP_PORT_48_55_TX_FAULT_REG    , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_56_63_TX_FAULT]    = {CPLD_SFP_PORT_56_63_TX_FAULT_REG    , MASK_ALL     , DATA_HEX},

    //CPLD 3
    [CPLD_SFP_PORT_16_23_PRES]        = {CPLD_SFP_PORT_16_23_PRES_REG        , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_24_31_PRES]        = {CPLD_SFP_PORT_24_31_PRES_REG        , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_64_71_PRES]        = {CPLD_SFP_PORT_64_71_PRES_REG        , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_72_79_PRES]        = {CPLD_SFP_PORT_72_79_PRES_REG        , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_16_23_PRES_MASK]   = {CPLD_SFP_PORT_16_23_PRES_MASK_REG   , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_24_31_PRES_MASK]   = {CPLD_SFP_PORT_24_31_PRES_MASK_REG   , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_64_71_PRES_MASK]   = {CPLD_SFP_PORT_64_71_PRES_MASK_REG   , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_72_79_PRES_MASK]   = {CPLD_SFP_PORT_72_79_PRES_MASK_REG   , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_16_23_PLUG_EVENT]  = {CPLD_SFP_PORT_16_23_PLUG_EVENT_REG  , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_24_31_PLUG_EVENT]  = {CPLD_SFP_PORT_24_31_PLUG_EVENT_REG  , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_64_71_PLUG_EVENT]  = {CPLD_SFP_PORT_64_71_PLUG_EVENT_REG  , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_72_79_PLUG_EVENT]  = {CPLD_SFP_PORT_72_79_PLUG_EVENT_REG  , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_16_23_TX_DISABLE]  = {CPLD_SFP_PORT_16_23_TX_DISABLE_REG  , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_24_31_TX_DISABLE]  = {CPLD_SFP_PORT_24_31_TX_DISABLE_REG  , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_64_71_TX_DISABLE]  = {CPLD_SFP_PORT_64_71_TX_DISABLE_REG  , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_72_79_TX_DISABLE]  = {CPLD_SFP_PORT_72_79_TX_DISABLE_REG  , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_16_23_RX_LOS]      = {CPLD_SFP_PORT_16_23_RX_LOS_REG      , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_24_31_RX_LOS]      = {CPLD_SFP_PORT_24_31_RX_LOS_REG      , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_64_71_RX_LOS]      = {CPLD_SFP_PORT_64_71_RX_LOS_REG      , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_72_79_RX_LOS]      = {CPLD_SFP_PORT_72_79_RX_LOS_REG      , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_16_23_TX_FAULT]    = {CPLD_SFP_PORT_16_23_TX_FAULT_REG    , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_24_31_TX_FAULT]    = {CPLD_SFP_PORT_24_31_TX_FAULT_REG    , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_64_71_TX_FAULT]    = {CPLD_SFP_PORT_64_71_TX_FAULT_REG    , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_72_79_TX_FAULT]    = {CPLD_SFP_PORT_72_79_TX_FAULT_REG    , MASK_ALL     , DATA_HEX},

    //CPLD 4
    [CPLD_SFP_PORT_32_39_PRES]        = {CPLD_SFP_PORT_32_39_PRES_REG        , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_40_43_PRES]        = {CPLD_SFP_PORT_40_43_PRES_REG        , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_80_87_PRES]        = {CPLD_SFP_PORT_80_87_PRES_REG        , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_88_91_PRES]        = {CPLD_SFP_PORT_88_91_PRES_REG        , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_32_39_PRES_MASK]   = {CPLD_SFP_PORT_32_39_PRES_MASK_REG   , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_40_43_PRES_MASK]   = {CPLD_SFP_PORT_40_43_PRES_MASK_REG   , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_80_87_PRES_MASK]   = {CPLD_SFP_PORT_80_87_PRES_MASK_REG   , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_88_91_PRES_MASK]   = {CPLD_SFP_PORT_88_91_PRES_MASK_REG   , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_32_39_PLUG_EVENT]  = {CPLD_SFP_PORT_32_39_PLUG_EVENT_REG  , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_40_43_PLUG_EVENT]  = {CPLD_SFP_PORT_40_43_PLUG_EVENT_REG  , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_80_87_PLUG_EVENT]  = {CPLD_SFP_PORT_80_87_PLUG_EVENT_REG  , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_88_91_PLUG_EVENT]  = {CPLD_SFP_PORT_88_91_PLUG_EVENT_REG  , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_32_39_TX_DISABLE]  = {CPLD_SFP_PORT_32_39_TX_DISABLE_REG  , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_40_43_TX_DISABLE]  = {CPLD_SFP_PORT_40_43_TX_DISABLE_REG  , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_80_87_TX_DISABLE]  = {CPLD_SFP_PORT_80_87_TX_DISABLE_REG  , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_88_91_TX_DISABLE]  = {CPLD_SFP_PORT_88_91_TX_DISABLE_REG  , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_32_39_RX_LOS]      = {CPLD_SFP_PORT_32_39_RX_LOS_REG      , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_40_43_RX_LOS]      = {CPLD_SFP_PORT_40_43_RX_LOS_REG      , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_80_87_RX_LOS]      = {CPLD_SFP_PORT_80_87_RX_LOS_REG      , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_88_91_RX_LOS]      = {CPLD_SFP_PORT_88_91_RX_LOS_REG      , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_32_39_TX_FAULT]    = {CPLD_SFP_PORT_32_39_TX_FAULT_REG    , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_40_43_TX_FAULT]    = {CPLD_SFP_PORT_40_43_TX_FAULT_REG    , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_80_87_TX_FAULT]    = {CPLD_SFP_PORT_80_87_TX_FAULT_REG    , MASK_ALL     , DATA_HEX},
    [CPLD_SFP_PORT_88_91_TX_FAULT]    = {CPLD_SFP_PORT_88_91_TX_FAULT_REG    , MASK_ALL     , DATA_HEX},

    // CPLD 5
    [CPLD_QSFP_PORT_96_103_INTR]       = {CPLD_QSFP_PORT_96_103_INTR_REG        , MASK_ALL, DATA_HEX},
    [CPLD_SFP_PORT_44_47_PRES]         = {CPLD_SFP_PORT_44_47_PRES_REG          , MASK_ALL, DATA_HEX},
    [CPLD_SFP_PORT_92_95_PRES]         = {CPLD_SFP_PORT_92_95_PRES_REG          , MASK_ALL, DATA_HEX},
    [CPLD_QSFP_PORT_96_103_PRES]       = {CPLD_QSFP_PORT_96_103_PRES_REG        , MASK_ALL, DATA_HEX},
    [CPLD_QSFP_PORT_96_103_INTR_MASK]  = {CPLD_QSFP_PORT_96_103_INTR_MASK_REG   , MASK_ALL, DATA_HEX},
    [CPLD_SFP_PORT_44_47_PRES_MASK]    = {CPLD_SFP_PORT_44_47_PRES_MASK_REG     , MASK_ALL, DATA_HEX},
    [CPLD_SFP_PORT_92_95_PRES_MASK]    = {CPLD_SFP_PORT_92_95_PRES_MASK_REG     , MASK_ALL, DATA_HEX},
    [CPLD_QSFP_PORT_96_103_PRES_MASK]  = {CPLD_QSFP_PORT_96_103_PRES_MASK_REG   , MASK_ALL, DATA_HEX},
    [CPLD_QSFP_PORT_96_103_INTR_EVENT] = {CPLD_QSFP_PORT_96_103_INTR_EVENT_REG  , MASK_ALL, DATA_HEX},
    [CPLD_SFP_PORT_44_47_PLUG_EVENT]   = {CPLD_SFP_PORT_44_47_PLUG_EVENT_REG    , MASK_ALL, DATA_HEX},
    [CPLD_SFP_PORT_92_95_PLUG_EVENT]   = {CPLD_SFP_PORT_92_95_PLUG_EVENT_REG    , MASK_ALL, DATA_HEX},
    [CPLD_QSFP_PORT_96_103_PLUG_EVENT] = {CPLD_QSFP_PORT_96_103_PLUG_EVENT_REG  , MASK_ALL, DATA_HEX},
    [CPLD_SFP_PORT_44_47_TX_DISABLE]   = {CPLD_SFP_PORT_44_47_TX_DISABLE_REG    , MASK_ALL, DATA_HEX},
    [CPLD_SFP_PORT_92_95_TX_DISABLE]   = {CPLD_SFP_PORT_92_95_TX_DISABLE_REG    , MASK_ALL, DATA_HEX},
    [CPLD_QSFP_PORT_96_103_RST]        = {CPLD_QSFP_PORT_96_103_RST_REG         , MASK_ALL, DATA_HEX},
    [CPLD_QSFP_PORT_96_103_MODSEL]     = {CPLD_QSFP_PORT_96_103_MODSEL_REG      , MASK_ALL, DATA_HEX},
    [CPLD_QSFP_PORT_96_103_LPMODE]     = {CPLD_QSFP_PORT_96_103_LPMODE_REG      , MASK_ALL, DATA_HEX},
    [CPLD_SFP_PORT_44_47_RX_LOS]       = {CPLD_SFP_PORT_44_47_RX_LOS_REG        , MASK_ALL, DATA_HEX},
    [CPLD_SFP_PORT_92_95_RX_LOS]       = {CPLD_SFP_PORT_92_95_RX_LOS_REG        , MASK_ALL, DATA_HEX},
    [CPLD_SFP_PORT_44_47_TX_FAULT]     = {CPLD_SFP_PORT_44_47_TX_FAULT_REG      , MASK_ALL, DATA_HEX},
    [CPLD_SFP_PORT_92_95_TX_FAULT]     = {CPLD_SFP_PORT_92_95_TX_FAULT_REG      , MASK_ALL, DATA_HEX},
    //BSP DEBUG
    [BSP_DEBUG]                = {CPLD_NONE_REG                 , MASK_NONE, DATA_UNK},
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
static ssize_t cpld_show(struct device *dev,
        struct device_attribute *da, char *buf);
static ssize_t cpld_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static u8 _cpld_reg_read(struct device *dev, u8 reg, u8 mask);
static ssize_t cpld_reg_read(struct device *dev, char *buf, u8 reg, u8 mask, u8 data_type);
static ssize_t cpld_reg_write(struct device *dev, const char *buf, size_t count, u8 reg, u8 mask);
static ssize_t bsp_read(char *buf, char *str);
static ssize_t bsp_write(const char *buf, char *str, size_t str_len, size_t count);
static ssize_t bsp_callback_show(struct device *dev,
        struct device_attribute *da, char *buf);
static ssize_t bsp_callback_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t cpld_version_h_show(struct device *dev,
                    struct device_attribute *da,
                    char *buf);

static LIST_HEAD(cpld_client_list);  /* client list for cpld */
static struct mutex list_lock;  /* mutex for client list */

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

struct cpld_data {
    int index;                  /* CPLD index */
    struct mutex access_lock;   /* mutex for cpld access */
    u8 access_reg;              /* register to access */
};

/* CPLD device id and data */
static const struct i2c_device_id cpld_id[] = {
    { "s9601_104bc_cpld1",  cpld1 },
    { "s9601_104bc_cpld2",  cpld2 },
    { "s9601_104bc_cpld3",  cpld3 },
    { "s9601_104bc_cpld4",  cpld4 },
    { "s9601_104bc_cpld5",  cpld5 },
    {}
};

char bsp_debug[2]="0";
u8 enable_log_read=LOG_DISABLE;
u8 enable_log_write=LOG_DISABLE;

/* Addresses scanned for cpld */
static const unsigned short cpld_i2c_addr[] = { 0x30, 0x31, 0x32, 0x33, 0x34, I2C_CLIENT_END };

/* define all support register access of cpld in attribute */

// CPLD Common

static SENSOR_DEVICE_ATTR_RO(cpld_minor_ver           , cpld, CPLD_MINOR_VER);
static SENSOR_DEVICE_ATTR_RO(cpld_major_ver           , cpld, CPLD_MAJOR_VER);
static SENSOR_DEVICE_ATTR_RO(cpld_id                  , cpld, CPLD_ID);
static SENSOR_DEVICE_ATTR_RO(cpld_build_ver           , cpld, CPLD_BUILD_VER);
static SENSOR_DEVICE_ATTR_RO(cpld_version_h           , cpld_version_h, CPLD_VERSION_H);


// CPLD 1
static SENSOR_DEVICE_ATTR_RO(cpld_sku_id                , cpld, CPLD_SKU_ID);
static SENSOR_DEVICE_ATTR_RO(cpld_hw_rev                , cpld, CPLD_HW_REV);
static SENSOR_DEVICE_ATTR_RO(cpld_deph_rev              , cpld, CPLD_DEPH_REV);
static SENSOR_DEVICE_ATTR_RO(cpld_build_rev             , cpld, CPLD_BUILD_REV);
static SENSOR_DEVICE_ATTR_RO(cpld_id_type               , cpld, CPLD_ID_TYPE);
static SENSOR_DEVICE_ATTR_RO(cpld_sku_ext               , cpld, CPLD_SKU_EXT);
static SENSOR_DEVICE_ATTR_RO(cpld_mac_op2_intr          , cpld, CPLD_MAC_OP2_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_10gphy_intr           , cpld, CPLD_10GPHY_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_cpld_fru_intr         , cpld, CPLD_CPLD_FRU_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_mgmt_sfp_port_status  , cpld, CPLD_MGMT_SFP_PORT_STATUS);
static SENSOR_DEVICE_ATTR_RO(cpld_misc_intr             , cpld, CPLD_MISC_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_system_intr           , cpld, CPLD_SYSTEM_INTR);
static SENSOR_DEVICE_ATTR_RW(cpld_mac_op2_intr_mask     , cpld, CPLD_MAC_OP2_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_10gphy_intr_mask      , cpld, CPLD_10GPHY_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_cpld_fru_intr_mask    , cpld, CPLD_CPLD_FRU_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_mgmt_sfp_port_status_mask, cpld, CPLD_MGMT_SFP_PORT_STATUS_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_misc_intr_mask        , cpld, CPLD_MISC_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_system_intr_mask      , cpld, CPLD_SYSTEM_INTR_MASK);
static SENSOR_DEVICE_ATTR_RO(cpld_mac_op2_intr_event    , cpld, CPLD_MAC_OP2_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_10gphy_intr_event     , cpld, CPLD_10GPHY_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_cpld_fru_intr_event   , cpld, CPLD_CPLD_FRU_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_mgmt_sfp_port_status_event, cpld, CPLD_MGMT_SFP_PORT_STATUS_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_misc_intr_event       , cpld, CPLD_MISC_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_system_intr_event     , cpld, CPLD_SYSTEM_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RW(cpld_mac_op2_rst           , cpld, CPLD_MAC_OP2_RST);
static SENSOR_DEVICE_ATTR_RW(cpld_phy_rst               , cpld, CPLD_PHY_RST);
static SENSOR_DEVICE_ATTR_RW(cpld_bmc_ntm_rst           , cpld, CPLD_BMC_NTM_RST);
static SENSOR_DEVICE_ATTR_RW(cpld_usb_rst               , cpld, CPLD_USB_RST);
static SENSOR_DEVICE_ATTR_RW(cpld_cpld_rst              , cpld, CPLD_CPLD_RST);
static SENSOR_DEVICE_ATTR_RW(cpld_mux_rst_1             , cpld, CPLD_MUX_RST_1);
static SENSOR_DEVICE_ATTR_RW(cpld_mux_rst_2             , cpld, CPLD_MUX_RST_2);
static SENSOR_DEVICE_ATTR_RW(cpld_misc_rst              , cpld, CPLD_MISC_RST);
static SENSOR_DEVICE_ATTR_RW(cpld_pushbtn               , cpld, CPLD_PUSHBTN);
static SENSOR_DEVICE_ATTR_RO(cpld_cpu_misc              , cpld, CPLD_CPU_MISC);
static SENSOR_DEVICE_ATTR_RO(cpld_psu_status            , cpld, CPLD_PSU_STATUS);
static SENSOR_DEVICE_ATTR_RO(cpld_sys_pw_status         , cpld, CPLD_SYS_PW_STATUS);
static SENSOR_DEVICE_ATTR_RO(cpld_mac_status_1          , cpld, CPLD_MAC_STATUS_1);
static SENSOR_DEVICE_ATTR_RO(cpld_mac_status_2          , cpld, CPLD_MAC_STATUS_2);
static SENSOR_DEVICE_ATTR_RO(cpld_mac_status_3          , cpld, CPLD_MAC_STATUS_3);
static SENSOR_DEVICE_ATTR_RW(cpld_mgmt_sfp_port_conf    , cpld, CPLD_MGMT_SFP_PORT_CONF);
static SENSOR_DEVICE_ATTR_RO(cpld_misc                  , cpld, CPLD_MISC);
static SENSOR_DEVICE_ATTR_RW(cpld_mux_ctrl              , cpld, CPLD_MUX_CTRL);
static SENSOR_DEVICE_ATTR_RO(cpld_system_led_psu        , cpld, CPLD_SYSTEM_LED_PSU);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_sys        , cpld, CPLD_SYSTEM_LED_SYS);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_sync       , cpld, CPLD_SYSTEM_LED_SYNC);
static SENSOR_DEVICE_ATTR_RO(cpld_system_led_fan        , cpld, CPLD_SYSTEM_LED_FAN);
static SENSOR_DEVICE_ATTR_RW(cpld_system_led_id         , cpld, CPLD_SYSTEM_LED_ID);
static SENSOR_DEVICE_ATTR_RO(cpld_mac_pg                , cpld, CPLD_MAC_PG);
static SENSOR_DEVICE_ATTR_RO(cpld_op2_pg                , cpld, CPLD_OP2_PG);
static SENSOR_DEVICE_ATTR_RO(cpld_misc_pg               , cpld, CPLD_MISC_PG);
static SENSOR_DEVICE_ATTR_RW(cpld_hbm_pw_en             , cpld, CPLD_HBM_PW_EN);
static SENSOR_DEVICE_ATTR_RO(cpld_ext_byte              , cpld, CPLD_EXT_BYTE);
static SENSOR_DEVICE_ATTR_RO(cpld_brd_id_def            , cpld, CPLD_BRD_ID_DEF);

//CPLD 2
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_0_7_pres        , cpld, CPLD_SFP_PORT_0_7_PRES);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_8_15_pres       , cpld, CPLD_SFP_PORT_8_15_PRES);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_48_55_pres      , cpld, CPLD_SFP_PORT_48_55_PRES);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_56_63_pres      , cpld, CPLD_SFP_PORT_56_63_PRES);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_0_7_pres_mask   , cpld, CPLD_SFP_PORT_0_7_PRES_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_8_15_pres_mask  , cpld, CPLD_SFP_PORT_8_15_PRES_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_48_55_pres_mask , cpld, CPLD_SFP_PORT_48_55_PRES_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_56_63_pres_mask , cpld, CPLD_SFP_PORT_56_63_PRES_MASK);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_0_7_plug_event  , cpld, CPLD_SFP_PORT_0_7_PLUG_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_8_15_plug_event , cpld, CPLD_SFP_PORT_8_15_PLUG_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_48_55_plug_event, cpld, CPLD_SFP_PORT_48_55_PLUG_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_56_63_plug_event, cpld, CPLD_SFP_PORT_56_63_PLUG_EVENT);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_0_7_tx_disable  , cpld, CPLD_SFP_PORT_0_7_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_8_15_tx_disable , cpld, CPLD_SFP_PORT_8_15_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_48_55_tx_disable, cpld, CPLD_SFP_PORT_48_55_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_56_63_tx_disable, cpld, CPLD_SFP_PORT_56_63_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_0_7_rx_los      , cpld, CPLD_SFP_PORT_0_7_RX_LOS);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_8_15_rx_los     , cpld, CPLD_SFP_PORT_8_15_RX_LOS);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_48_55_rx_los    , cpld, CPLD_SFP_PORT_48_55_RX_LOS);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_56_63_rx_los    , cpld, CPLD_SFP_PORT_56_63_RX_LOS);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_0_7_tx_fault    , cpld, CPLD_SFP_PORT_0_7_TX_FAULT);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_8_15_tx_fault   , cpld, CPLD_SFP_PORT_8_15_TX_FAULT);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_48_55_tx_fault  , cpld, CPLD_SFP_PORT_48_55_TX_FAULT);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_56_63_tx_fault  , cpld, CPLD_SFP_PORT_56_63_TX_FAULT);

//CPLD 3
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_16_23_pres       , cpld, CPLD_SFP_PORT_16_23_PRES);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_24_31_pres       , cpld, CPLD_SFP_PORT_24_31_PRES);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_64_71_pres       , cpld, CPLD_SFP_PORT_64_71_PRES);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_72_79_pres       , cpld, CPLD_SFP_PORT_72_79_PRES);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_16_23_pres_mask  , cpld, CPLD_SFP_PORT_16_23_PRES_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_24_31_pres_mask  , cpld, CPLD_SFP_PORT_24_31_PRES_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_64_71_pres_mask  , cpld, CPLD_SFP_PORT_64_71_PRES_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_72_79_pres_mask  , cpld, CPLD_SFP_PORT_72_79_PRES_MASK);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_16_23_plug_event , cpld, CPLD_SFP_PORT_16_23_PLUG_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_24_31_plug_event , cpld, CPLD_SFP_PORT_24_31_PLUG_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_64_71_plug_event , cpld, CPLD_SFP_PORT_64_71_PLUG_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_72_79_plug_event , cpld, CPLD_SFP_PORT_72_79_PLUG_EVENT);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_16_23_tx_disable , cpld, CPLD_SFP_PORT_16_23_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_24_31_tx_disable , cpld, CPLD_SFP_PORT_24_31_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_64_71_tx_disable , cpld, CPLD_SFP_PORT_64_71_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_72_79_tx_disable , cpld, CPLD_SFP_PORT_72_79_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_16_23_rx_los     , cpld, CPLD_SFP_PORT_16_23_RX_LOS);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_24_31_rx_los     , cpld, CPLD_SFP_PORT_24_31_RX_LOS);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_64_71_rx_los     , cpld, CPLD_SFP_PORT_64_71_RX_LOS);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_72_79_rx_los     , cpld, CPLD_SFP_PORT_72_79_RX_LOS);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_16_23_tx_fault   , cpld, CPLD_SFP_PORT_16_23_TX_FAULT);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_24_31_tx_fault   , cpld, CPLD_SFP_PORT_24_31_TX_FAULT);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_64_71_tx_fault   , cpld, CPLD_SFP_PORT_64_71_TX_FAULT);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_72_79_tx_fault   , cpld, CPLD_SFP_PORT_72_79_TX_FAULT);

//CPLD 4
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_32_39_pres       , cpld, CPLD_SFP_PORT_32_39_PRES);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_40_43_pres       , cpld, CPLD_SFP_PORT_40_43_PRES);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_80_87_pres       , cpld, CPLD_SFP_PORT_80_87_PRES);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_88_91_pres       , cpld, CPLD_SFP_PORT_88_91_PRES);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_32_39_pres_mask  , cpld, CPLD_SFP_PORT_32_39_PRES_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_40_43_pres_mask  , cpld, CPLD_SFP_PORT_40_43_PRES_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_80_87_pres_mask  , cpld, CPLD_SFP_PORT_80_87_PRES_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_88_91_pres_mask  , cpld, CPLD_SFP_PORT_88_91_PRES_MASK);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_32_39_plug_event , cpld, CPLD_SFP_PORT_32_39_PLUG_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_40_43_plug_event , cpld, CPLD_SFP_PORT_40_43_PLUG_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_80_87_plug_event , cpld, CPLD_SFP_PORT_80_87_PLUG_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_88_91_plug_event , cpld, CPLD_SFP_PORT_88_91_PLUG_EVENT);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_32_39_tx_disable , cpld, CPLD_SFP_PORT_32_39_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_40_43_tx_disable , cpld, CPLD_SFP_PORT_40_43_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_80_87_tx_disable , cpld, CPLD_SFP_PORT_80_87_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_88_91_tx_disable , cpld, CPLD_SFP_PORT_88_91_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_32_39_rx_los     , cpld, CPLD_SFP_PORT_32_39_RX_LOS);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_40_43_rx_los     , cpld, CPLD_SFP_PORT_40_43_RX_LOS);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_80_87_rx_los     , cpld, CPLD_SFP_PORT_80_87_RX_LOS);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_88_91_rx_los     , cpld, CPLD_SFP_PORT_88_91_RX_LOS);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_32_39_tx_fault   , cpld, CPLD_SFP_PORT_32_39_TX_FAULT);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_40_43_tx_fault   , cpld, CPLD_SFP_PORT_40_43_TX_FAULT);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_80_87_tx_fault   , cpld, CPLD_SFP_PORT_80_87_TX_FAULT);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_88_91_tx_fault   , cpld, CPLD_SFP_PORT_88_91_TX_FAULT);

//CPLD 5
static SENSOR_DEVICE_ATTR_RO(cpld_qsfp_port_96_103_intr       , cpld, CPLD_QSFP_PORT_96_103_INTR);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_44_47_pres         , cpld, CPLD_SFP_PORT_44_47_PRES);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_92_95_pres         , cpld, CPLD_SFP_PORT_92_95_PRES);
static SENSOR_DEVICE_ATTR_RO(cpld_qsfp_port_96_103_pres       , cpld, CPLD_QSFP_PORT_96_103_PRES);
static SENSOR_DEVICE_ATTR_RW(cpld_qsfp_port_96_103_intr_mask  , cpld, CPLD_QSFP_PORT_96_103_INTR_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_44_47_pres_mask    , cpld, CPLD_SFP_PORT_44_47_PRES_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_92_95_pres_mask    , cpld, CPLD_SFP_PORT_92_95_PRES_MASK);
static SENSOR_DEVICE_ATTR_RW(cpld_qsfp_port_96_103_pres_mask  , cpld, CPLD_QSFP_PORT_96_103_PRES_MASK);
static SENSOR_DEVICE_ATTR_RO(cpld_qsfp_port_96_103_intr_event , cpld, CPLD_QSFP_PORT_96_103_INTR_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_44_47_plug_event   , cpld, CPLD_SFP_PORT_44_47_PLUG_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_92_95_plug_event   , cpld, CPLD_SFP_PORT_92_95_PLUG_EVENT);
static SENSOR_DEVICE_ATTR_RO(cpld_qsfp_port_96_103_plug_event , cpld, CPLD_QSFP_PORT_96_103_PLUG_EVENT);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_44_47_tx_disable   , cpld, CPLD_SFP_PORT_44_47_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(cpld_sfp_port_92_95_tx_disable   , cpld, CPLD_SFP_PORT_92_95_TX_DISABLE);
static SENSOR_DEVICE_ATTR_RW(cpld_qsfp_port_96_103_rst        , cpld, CPLD_QSFP_PORT_96_103_RST);
static SENSOR_DEVICE_ATTR_RW(cpld_qsfp_port_96_103_modsel     , cpld, CPLD_QSFP_PORT_96_103_MODSEL);
static SENSOR_DEVICE_ATTR_RW(cpld_qsfp_port_96_103_lpmode     , cpld, CPLD_QSFP_PORT_96_103_LPMODE);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_44_47_rx_los       , cpld, CPLD_SFP_PORT_44_47_RX_LOS);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_92_95_rx_los       , cpld, CPLD_SFP_PORT_92_95_RX_LOS);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_44_47_tx_fault     , cpld, CPLD_SFP_PORT_44_47_TX_FAULT);
static SENSOR_DEVICE_ATTR_RO(cpld_sfp_port_92_95_tx_fault     , cpld, CPLD_SFP_PORT_92_95_TX_FAULT);

//BSP DEBUG
static SENSOR_DEVICE_ATTR_RW(bsp_debug                , bsp_callback, BSP_DEBUG);

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
    _DEVICE_ATTR(cpld_id_type),
    _DEVICE_ATTR(cpld_sku_ext),
    _DEVICE_ATTR(cpld_mac_op2_intr),
    _DEVICE_ATTR(cpld_10gphy_intr),
    _DEVICE_ATTR(cpld_cpld_fru_intr),
    _DEVICE_ATTR(cpld_mgmt_sfp_port_status),
    _DEVICE_ATTR(cpld_misc_intr),
    _DEVICE_ATTR(cpld_system_intr),
    _DEVICE_ATTR(cpld_mac_op2_intr_mask),
    _DEVICE_ATTR(cpld_10gphy_intr_mask),
    _DEVICE_ATTR(cpld_cpld_fru_intr_mask),
    _DEVICE_ATTR(cpld_mgmt_sfp_port_status_mask),
    _DEVICE_ATTR(cpld_misc_intr_mask),
    _DEVICE_ATTR(cpld_system_intr_mask),
    _DEVICE_ATTR(cpld_mac_op2_intr_event),
    _DEVICE_ATTR(cpld_10gphy_intr_event),
    _DEVICE_ATTR(cpld_cpld_fru_intr_event),
    _DEVICE_ATTR(cpld_mgmt_sfp_port_status_event),
    _DEVICE_ATTR(cpld_misc_intr_event),
    _DEVICE_ATTR(cpld_system_intr_event),
    _DEVICE_ATTR(cpld_mac_op2_rst),
    _DEVICE_ATTR(cpld_phy_rst),
    _DEVICE_ATTR(cpld_bmc_ntm_rst),
    _DEVICE_ATTR(cpld_usb_rst),
    _DEVICE_ATTR(cpld_cpld_rst),
    _DEVICE_ATTR(cpld_mux_rst_1),
    _DEVICE_ATTR(cpld_mux_rst_2),
    _DEVICE_ATTR(cpld_misc_rst),
    _DEVICE_ATTR(cpld_pushbtn),
    _DEVICE_ATTR(cpld_cpu_misc),
    _DEVICE_ATTR(cpld_psu_status),
    _DEVICE_ATTR(cpld_sys_pw_status),
    _DEVICE_ATTR(cpld_mac_status_1),
    _DEVICE_ATTR(cpld_mac_status_2),
    _DEVICE_ATTR(cpld_mac_status_3),
    _DEVICE_ATTR(cpld_mgmt_sfp_port_conf),
    _DEVICE_ATTR(cpld_misc),
    _DEVICE_ATTR(cpld_mux_ctrl),
    _DEVICE_ATTR(cpld_system_led_psu),
    _DEVICE_ATTR(cpld_system_led_sys),
    _DEVICE_ATTR(cpld_system_led_sync),
    _DEVICE_ATTR(cpld_system_led_fan),
    _DEVICE_ATTR(cpld_system_led_id),
    _DEVICE_ATTR(cpld_mac_pg),
    _DEVICE_ATTR(cpld_op2_pg),
    _DEVICE_ATTR(cpld_misc_pg),
    _DEVICE_ATTR(cpld_hbm_pw_en),
    _DEVICE_ATTR(cpld_ext_byte),
    _DEVICE_ATTR(cpld_brd_id_def),
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

    // CPLD 2
    _DEVICE_ATTR(cpld_sfp_port_0_7_pres),
    _DEVICE_ATTR(cpld_sfp_port_8_15_pres),
    _DEVICE_ATTR(cpld_sfp_port_48_55_pres),
    _DEVICE_ATTR(cpld_sfp_port_56_63_pres),
    _DEVICE_ATTR(cpld_sfp_port_0_7_pres_mask),
    _DEVICE_ATTR(cpld_sfp_port_8_15_pres_mask),
    _DEVICE_ATTR(cpld_sfp_port_48_55_pres_mask),
    _DEVICE_ATTR(cpld_sfp_port_56_63_pres_mask),
    _DEVICE_ATTR(cpld_sfp_port_0_7_plug_event),
    _DEVICE_ATTR(cpld_sfp_port_8_15_plug_event),
    _DEVICE_ATTR(cpld_sfp_port_48_55_plug_event),
    _DEVICE_ATTR(cpld_sfp_port_56_63_plug_event),
    _DEVICE_ATTR(cpld_sfp_port_0_7_tx_disable),
    _DEVICE_ATTR(cpld_sfp_port_8_15_tx_disable),
    _DEVICE_ATTR(cpld_sfp_port_48_55_tx_disable),
    _DEVICE_ATTR(cpld_sfp_port_56_63_tx_disable),
    _DEVICE_ATTR(cpld_sfp_port_0_7_rx_los),
    _DEVICE_ATTR(cpld_sfp_port_8_15_rx_los),
    _DEVICE_ATTR(cpld_sfp_port_48_55_rx_los),
    _DEVICE_ATTR(cpld_sfp_port_56_63_rx_los),
    _DEVICE_ATTR(cpld_sfp_port_0_7_tx_fault),
    _DEVICE_ATTR(cpld_sfp_port_8_15_tx_fault),
    _DEVICE_ATTR(cpld_sfp_port_48_55_tx_fault),
    _DEVICE_ATTR(cpld_sfp_port_56_63_tx_fault),

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

    // CPLD 3
    _DEVICE_ATTR(cpld_sfp_port_16_23_pres),
    _DEVICE_ATTR(cpld_sfp_port_24_31_pres),
    _DEVICE_ATTR(cpld_sfp_port_64_71_pres),
    _DEVICE_ATTR(cpld_sfp_port_72_79_pres),
    _DEVICE_ATTR(cpld_sfp_port_16_23_pres_mask),
    _DEVICE_ATTR(cpld_sfp_port_24_31_pres_mask),
    _DEVICE_ATTR(cpld_sfp_port_64_71_pres_mask),
    _DEVICE_ATTR(cpld_sfp_port_72_79_pres_mask),
    _DEVICE_ATTR(cpld_sfp_port_16_23_plug_event),
    _DEVICE_ATTR(cpld_sfp_port_24_31_plug_event),
    _DEVICE_ATTR(cpld_sfp_port_64_71_plug_event),
    _DEVICE_ATTR(cpld_sfp_port_72_79_plug_event),
    _DEVICE_ATTR(cpld_sfp_port_16_23_tx_disable),
    _DEVICE_ATTR(cpld_sfp_port_24_31_tx_disable),
    _DEVICE_ATTR(cpld_sfp_port_64_71_tx_disable),
    _DEVICE_ATTR(cpld_sfp_port_72_79_tx_disable),
    _DEVICE_ATTR(cpld_sfp_port_16_23_rx_los),
    _DEVICE_ATTR(cpld_sfp_port_24_31_rx_los),
    _DEVICE_ATTR(cpld_sfp_port_64_71_rx_los),
    _DEVICE_ATTR(cpld_sfp_port_72_79_rx_los),
    _DEVICE_ATTR(cpld_sfp_port_16_23_tx_fault),
    _DEVICE_ATTR(cpld_sfp_port_24_31_tx_fault),
    _DEVICE_ATTR(cpld_sfp_port_64_71_tx_fault),
    _DEVICE_ATTR(cpld_sfp_port_72_79_tx_fault),

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

    // CPLD 4
    _DEVICE_ATTR(cpld_sfp_port_32_39_pres),
    _DEVICE_ATTR(cpld_sfp_port_40_43_pres),
    _DEVICE_ATTR(cpld_sfp_port_80_87_pres),
    _DEVICE_ATTR(cpld_sfp_port_88_91_pres),
    _DEVICE_ATTR(cpld_sfp_port_32_39_pres_mask),
    _DEVICE_ATTR(cpld_sfp_port_40_43_pres_mask),
    _DEVICE_ATTR(cpld_sfp_port_80_87_pres_mask),
    _DEVICE_ATTR(cpld_sfp_port_88_91_pres_mask),
    _DEVICE_ATTR(cpld_sfp_port_32_39_plug_event),
    _DEVICE_ATTR(cpld_sfp_port_40_43_plug_event),
    _DEVICE_ATTR(cpld_sfp_port_80_87_plug_event),
    _DEVICE_ATTR(cpld_sfp_port_88_91_plug_event),
    _DEVICE_ATTR(cpld_sfp_port_32_39_tx_disable),
    _DEVICE_ATTR(cpld_sfp_port_40_43_tx_disable),
    _DEVICE_ATTR(cpld_sfp_port_80_87_tx_disable),
    _DEVICE_ATTR(cpld_sfp_port_88_91_tx_disable),
    _DEVICE_ATTR(cpld_sfp_port_32_39_rx_los),
    _DEVICE_ATTR(cpld_sfp_port_40_43_rx_los),
    _DEVICE_ATTR(cpld_sfp_port_80_87_rx_los),
    _DEVICE_ATTR(cpld_sfp_port_88_91_rx_los),
    _DEVICE_ATTR(cpld_sfp_port_32_39_tx_fault),
    _DEVICE_ATTR(cpld_sfp_port_40_43_tx_fault),
    _DEVICE_ATTR(cpld_sfp_port_80_87_tx_fault),
    _DEVICE_ATTR(cpld_sfp_port_88_91_tx_fault),

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

    // CPLD 5
    _DEVICE_ATTR(cpld_qsfp_port_96_103_intr),
    _DEVICE_ATTR(cpld_sfp_port_44_47_pres),
    _DEVICE_ATTR(cpld_sfp_port_92_95_pres),
    _DEVICE_ATTR(cpld_qsfp_port_96_103_pres),
    _DEVICE_ATTR(cpld_qsfp_port_96_103_intr_mask),
    _DEVICE_ATTR(cpld_sfp_port_44_47_pres_mask),
    _DEVICE_ATTR(cpld_sfp_port_92_95_pres_mask),
    _DEVICE_ATTR(cpld_qsfp_port_96_103_pres_mask),
    _DEVICE_ATTR(cpld_qsfp_port_96_103_intr_event),
    _DEVICE_ATTR(cpld_sfp_port_44_47_plug_event),
    _DEVICE_ATTR(cpld_sfp_port_92_95_plug_event),
    _DEVICE_ATTR(cpld_qsfp_port_96_103_plug_event),
    _DEVICE_ATTR(cpld_sfp_port_44_47_tx_disable),
    _DEVICE_ATTR(cpld_sfp_port_92_95_tx_disable),
    _DEVICE_ATTR(cpld_qsfp_port_96_103_rst),
    _DEVICE_ATTR(cpld_qsfp_port_96_103_modsel),
    _DEVICE_ATTR(cpld_qsfp_port_96_103_lpmode),
    _DEVICE_ATTR(cpld_sfp_port_44_47_rx_los),
    _DEVICE_ATTR(cpld_sfp_port_92_95_rx_los),
    _DEVICE_ATTR(cpld_sfp_port_44_47_tx_fault),
    _DEVICE_ATTR(cpld_sfp_port_92_95_tx_fault),

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
static u8 _mask_shift(u8 val, u8 mask)
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

        //CPLD 1
        case CPLD_SKU_ID:
        case CPLD_HW_REV:
        case CPLD_DEPH_REV:
        case CPLD_BUILD_REV:
        case CPLD_ID_TYPE:
        case CPLD_SKU_EXT:
        case CPLD_MAC_OP2_INTR:
        case CPLD_10GPHY_INTR:
        case CPLD_CPLD_FRU_INTR:
        case CPLD_MGMT_SFP_PORT_STATUS:
        case CPLD_MISC_INTR:
        case CPLD_SYSTEM_INTR:
        case CPLD_MAC_OP2_INTR_MASK:
        case CPLD_10GPHY_INTR_MASK:
        case CPLD_CPLD_FRU_INTR_MASK:
        case CPLD_MGMT_SFP_PORT_STATUS_MASK:
        case CPLD_MISC_INTR_MASK:
        case CPLD_SYSTEM_INTR_MASK:
        case CPLD_MAC_OP2_INTR_EVENT:
        case CPLD_10GPHY_INTR_EVENT:
        case CPLD_CPLD_FRU_INTR_EVENT:
        case CPLD_MGMT_SFP_PORT_STATUS_EVENT:
        case CPLD_MISC_INTR_EVENT:
        case CPLD_SYSTEM_INTR_EVENT:
        case CPLD_MAC_OP2_RST:
        case CPLD_PHY_RST:
        case CPLD_BMC_NTM_RST:
        case CPLD_USB_RST:
        case CPLD_CPLD_RST:
        case CPLD_MUX_RST_1:
        case CPLD_MUX_RST_2:
        case CPLD_MISC_RST:
        case CPLD_PUSHBTN:
        case CPLD_CPU_MISC:
        case CPLD_PSU_STATUS:
        case CPLD_SYS_PW_STATUS:
        case CPLD_MAC_STATUS_1:
        case CPLD_MAC_STATUS_2:
        case CPLD_MAC_STATUS_3:
        case CPLD_MGMT_SFP_PORT_CONF:
        case CPLD_MISC:
        case CPLD_MUX_CTRL:
        case CPLD_SYSTEM_LED_PSU:
        case CPLD_SYSTEM_LED_SYS:
        case CPLD_SYSTEM_LED_SYNC:
        case CPLD_SYSTEM_LED_FAN:
        case CPLD_SYSTEM_LED_ID:
        case CPLD_MAC_PG:
        case CPLD_OP2_PG:
        case CPLD_MISC_PG:
        case CPLD_HBM_PW_EN:
        case CPLD_EXT_BYTE:
        case CPLD_BRD_ID_DEF:

        //CPLD 2
        case CPLD_SFP_PORT_0_7_PRES:
        case CPLD_SFP_PORT_8_15_PRES:
        case CPLD_SFP_PORT_48_55_PRES:
        case CPLD_SFP_PORT_56_63_PRES:
        case CPLD_SFP_PORT_0_7_PRES_MASK:
        case CPLD_SFP_PORT_8_15_PRES_MASK:
        case CPLD_SFP_PORT_48_55_PRES_MASK:
        case CPLD_SFP_PORT_56_63_PRES_MASK:
        case CPLD_SFP_PORT_0_7_PLUG_EVENT:
        case CPLD_SFP_PORT_8_15_PLUG_EVENT:
        case CPLD_SFP_PORT_48_55_PLUG_EVENT:
        case CPLD_SFP_PORT_56_63_PLUG_EVENT:
        case CPLD_SFP_PORT_0_7_TX_DISABLE:
        case CPLD_SFP_PORT_8_15_TX_DISABLE:
        case CPLD_SFP_PORT_48_55_TX_DISABLE:
        case CPLD_SFP_PORT_56_63_TX_DISABLE:
        case CPLD_SFP_PORT_0_7_RX_LOS:
        case CPLD_SFP_PORT_8_15_RX_LOS:
        case CPLD_SFP_PORT_48_55_RX_LOS:
        case CPLD_SFP_PORT_56_63_RX_LOS:
        case CPLD_SFP_PORT_0_7_TX_FAULT:
        case CPLD_SFP_PORT_8_15_TX_FAULT:
        case CPLD_SFP_PORT_48_55_TX_FAULT:
        case CPLD_SFP_PORT_56_63_TX_FAULT:

        //CPLD 3
        case CPLD_SFP_PORT_16_23_PRES:
        case CPLD_SFP_PORT_24_31_PRES:
        case CPLD_SFP_PORT_64_71_PRES:
        case CPLD_SFP_PORT_72_79_PRES:
        case CPLD_SFP_PORT_16_23_PRES_MASK:
        case CPLD_SFP_PORT_24_31_PRES_MASK:
        case CPLD_SFP_PORT_64_71_PRES_MASK:
        case CPLD_SFP_PORT_72_79_PRES_MASK:
        case CPLD_SFP_PORT_16_23_PLUG_EVENT:
        case CPLD_SFP_PORT_24_31_PLUG_EVENT:
        case CPLD_SFP_PORT_64_71_PLUG_EVENT:
        case CPLD_SFP_PORT_72_79_PLUG_EVENT:
        case CPLD_SFP_PORT_16_23_TX_DISABLE:
        case CPLD_SFP_PORT_24_31_TX_DISABLE:
        case CPLD_SFP_PORT_64_71_TX_DISABLE:
        case CPLD_SFP_PORT_72_79_TX_DISABLE:
        case CPLD_SFP_PORT_16_23_RX_LOS:
        case CPLD_SFP_PORT_24_31_RX_LOS:
        case CPLD_SFP_PORT_64_71_RX_LOS:
        case CPLD_SFP_PORT_72_79_RX_LOS:
        case CPLD_SFP_PORT_16_23_TX_FAULT:
        case CPLD_SFP_PORT_24_31_TX_FAULT:
        case CPLD_SFP_PORT_64_71_TX_FAULT:
        case CPLD_SFP_PORT_72_79_TX_FAULT:

        //CPLD 4
        case CPLD_SFP_PORT_32_39_PRES:
        case CPLD_SFP_PORT_40_43_PRES:
        case CPLD_SFP_PORT_80_87_PRES:
        case CPLD_SFP_PORT_88_91_PRES:
        case CPLD_SFP_PORT_32_39_PRES_MASK:
        case CPLD_SFP_PORT_40_43_PRES_MASK:
        case CPLD_SFP_PORT_80_87_PRES_MASK:
        case CPLD_SFP_PORT_88_91_PRES_MASK:
        case CPLD_SFP_PORT_32_39_PLUG_EVENT:
        case CPLD_SFP_PORT_40_43_PLUG_EVENT:
        case CPLD_SFP_PORT_80_87_PLUG_EVENT:
        case CPLD_SFP_PORT_88_91_PLUG_EVENT:
        case CPLD_SFP_PORT_32_39_TX_DISABLE:
        case CPLD_SFP_PORT_40_43_TX_DISABLE:
        case CPLD_SFP_PORT_80_87_TX_DISABLE:
        case CPLD_SFP_PORT_88_91_TX_DISABLE:
        case CPLD_SFP_PORT_32_39_RX_LOS:
        case CPLD_SFP_PORT_40_43_RX_LOS:
        case CPLD_SFP_PORT_80_87_RX_LOS:
        case CPLD_SFP_PORT_88_91_RX_LOS:
        case CPLD_SFP_PORT_32_39_TX_FAULT:
        case CPLD_SFP_PORT_40_43_TX_FAULT:
        case CPLD_SFP_PORT_80_87_TX_FAULT:
        case CPLD_SFP_PORT_88_91_TX_FAULT:

        //CPLD 5
        case CPLD_QSFP_PORT_96_103_INTR:
        case CPLD_SFP_PORT_44_47_PRES:
        case CPLD_SFP_PORT_92_95_PRES:
        case CPLD_QSFP_PORT_96_103_PRES:
        case CPLD_QSFP_PORT_96_103_INTR_MASK:
        case CPLD_SFP_PORT_44_47_PRES_MASK:
        case CPLD_SFP_PORT_92_95_PRES_MASK:
        case CPLD_QSFP_PORT_96_103_PRES_MASK:
        case CPLD_QSFP_PORT_96_103_INTR_EVENT:
        case CPLD_SFP_PORT_44_47_PLUG_EVENT:
        case CPLD_SFP_PORT_92_95_PLUG_EVENT:
        case CPLD_QSFP_PORT_96_103_PLUG_EVENT:
        case CPLD_SFP_PORT_44_47_TX_DISABLE:
        case CPLD_SFP_PORT_92_95_TX_DISABLE:
        case CPLD_QSFP_PORT_96_103_RST:
        case CPLD_QSFP_PORT_96_103_MODSEL:
        case CPLD_QSFP_PORT_96_103_LPMODE:
        case CPLD_SFP_PORT_44_47_RX_LOS:
        case CPLD_SFP_PORT_92_95_RX_LOS:
        case CPLD_SFP_PORT_44_47_TX_FAULT:
        case CPLD_SFP_PORT_92_95_TX_FAULT:
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
        case CPLD_MAC_OP2_INTR_MASK:
        case CPLD_10GPHY_INTR_MASK:
        case CPLD_CPLD_FRU_INTR_MASK:
        case CPLD_MGMT_SFP_PORT_STATUS_MASK:
        case CPLD_MISC_INTR_MASK:
        case CPLD_SYSTEM_INTR_MASK:
        case CPLD_MAC_OP2_RST:
        case CPLD_PHY_RST:
        case CPLD_BMC_NTM_RST:
        case CPLD_USB_RST:
        case CPLD_CPLD_RST:
        case CPLD_MUX_RST_1:
        case CPLD_MUX_RST_2:
        case CPLD_MISC_RST:
        case CPLD_PUSHBTN:
        case CPLD_MGMT_SFP_PORT_CONF:
        case CPLD_MUX_CTRL:
        case CPLD_SYSTEM_LED_PSU:
        case CPLD_SYSTEM_LED_SYS:
        case CPLD_SYSTEM_LED_SYNC:
        case CPLD_SYSTEM_LED_FAN:
        case CPLD_SYSTEM_LED_ID:
        case CPLD_HBM_PW_EN:

        //CPLD 2
        case CPLD_SFP_PORT_0_7_PRES_MASK:
        case CPLD_SFP_PORT_8_15_PRES_MASK:
        case CPLD_SFP_PORT_48_55_PRES_MASK:
        case CPLD_SFP_PORT_56_63_PRES_MASK:
        case CPLD_SFP_PORT_0_7_TX_DISABLE:
        case CPLD_SFP_PORT_8_15_TX_DISABLE:
        case CPLD_SFP_PORT_48_55_TX_DISABLE:
        case CPLD_SFP_PORT_56_63_TX_DISABLE:

        //CPLD 3
        case CPLD_SFP_PORT_16_23_PRES_MASK:
        case CPLD_SFP_PORT_24_31_PRES_MASK:
        case CPLD_SFP_PORT_64_71_PRES_MASK:
        case CPLD_SFP_PORT_72_79_PRES_MASK:
        case CPLD_SFP_PORT_16_23_TX_DISABLE:
        case CPLD_SFP_PORT_24_31_TX_DISABLE:
        case CPLD_SFP_PORT_64_71_TX_DISABLE:
        case CPLD_SFP_PORT_72_79_TX_DISABLE:

        //CPLD 4
        case CPLD_SFP_PORT_32_39_PRES_MASK:
        case CPLD_SFP_PORT_40_43_PRES_MASK:
        case CPLD_SFP_PORT_80_87_PRES_MASK:
        case CPLD_SFP_PORT_88_91_PRES_MASK:
        case CPLD_SFP_PORT_32_39_TX_DISABLE:
        case CPLD_SFP_PORT_40_43_TX_DISABLE:
        case CPLD_SFP_PORT_80_87_TX_DISABLE:
        case CPLD_SFP_PORT_88_91_TX_DISABLE:

        //CPLD 5
        case CPLD_QSFP_PORT_96_103_INTR_MASK:
        case CPLD_SFP_PORT_44_47_PRES_MASK:
        case CPLD_SFP_PORT_92_95_PRES_MASK:
        case CPLD_QSFP_PORT_96_103_PRES_MASK:
        case CPLD_SFP_PORT_44_47_TX_DISABLE:
        case CPLD_SFP_PORT_92_95_TX_DISABLE:
        case CPLD_QSFP_PORT_96_103_RST:
        case CPLD_QSFP_PORT_96_103_MODSEL:
        case CPLD_QSFP_PORT_96_103_LPMODE:
            reg = attr_reg[attr->index].reg;
            mask= attr_reg[attr->index].mask;
            break;
        default:
            return -EINVAL;
    }
    return cpld_reg_write(dev, buf, count, reg, mask);
}

/* get cpld register value */
static u8 _cpld_reg_read(struct device *dev,
                    u8 reg,
                    u8 mask)
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

/* set cpld register value */
static ssize_t cpld_reg_write(struct device *dev,
                    const char *buf,
                    size_t count,
                    u8 reg,
                    u8 mask)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
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

    I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
               client, reg, reg_val);

    if (unlikely(ret < 0)) {
        dev_err(dev, "cpld_reg_write() error, return=%d\n", ret);
        return ret;
    }

    return count;
}

/* get qsfp port config register value */
static ssize_t cpld_version_h_show(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    if (attr->index == CPLD_VERSION_H) {
        return sprintf(buf, "%d.%02d.%03d",
                _cpld_reg_read(dev, attr_reg[CPLD_MAJOR_VER].reg, attr_reg[CPLD_MAJOR_VER].mask),
                _cpld_reg_read(dev, attr_reg[CPLD_MINOR_VER].reg, attr_reg[CPLD_MINOR_VER].mask),
                _cpld_reg_read(dev, attr_reg[CPLD_BUILD_VER].reg, attr_reg[CPLD_BUILD_VER].mask));
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
static int cpld_probe(struct i2c_client *client,
                    const struct i2c_device_id *dev_id)
{
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

    if (INVALID(ret, cpld1, cpld5)) {
        dev_info(&client->dev,
            "cpld id %d(device) not valid\n", ret);
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
    case cpld5:
        status = sysfs_create_group(&client->dev.kobj,
                    &cpld5_group);
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
    case cpld5:
        sysfs_remove_group(&client->dev.kobj, &cpld5_group);
        break;
    default:
        break;
    }
    return status;
}

/* cpld drvier remove */
static int cpld_remove(struct i2c_client *client)
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
    case cpld5:
        sysfs_remove_group(&client->dev.kobj, &cpld5_group);
        break;
    }

    cpld_remove_client(client);
    return 0;
}

MODULE_DEVICE_TABLE(i2c, cpld_id);

static struct i2c_driver cpld_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name = "x86_64_ufispace_s9601_104bc_cpld",
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

MODULE_AUTHOR("Nonodark Huang <nonodark.huang@ufispace.com>");
MODULE_DESCRIPTION("x86_64_ufispace_s9601_104bc_cpld driver");
MODULE_LICENSE("GPL");

module_init(cpld_init);
module_exit(cpld_exit);
