/*
 * A i2c cpld driver for the ufispace_s9511_20ct
 *
 * Copyright (C) 2024 UfiSpace Technology Corporation.
 * Melo Lin <melo.lin@ufispace.com>
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
#include "x86-64-ufispace-s9511-20ct-cpld.h"

#define DRIVER_NAME "x86_64_ufispace_s9511_20ct_cpld"

#if !defined(SENSOR_DEVICE_ATTR_RO)
#define SENSOR_DEVICE_ATTR_RO(_name, _func, _index)    \
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
{   \
    mutex_lock(lock);   \
    ret = i2c_smbus_read_byte_data(i2c_client, reg);    \
    mutex_unlock(lock); \
    BSP_LOG_R("cpld[%d], reg=0x%03x, reg_val=0x%02x", data->index, reg, ret);   \
}

#define I2C_WRITE_BYTE_DATA(ret, lock, i2c_client, reg, val)    \
{   \
    mutex_lock(lock);   \
    ret = i2c_smbus_write_byte_data(i2c_client, reg, val);  \
    mutex_unlock(lock); \
    BSP_LOG_W("cpld[%d], reg=0x%03x, reg_val=0x%02x", data->index, reg, val);   \
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

#define _DEVICE_ATTR(_name) \
    &sensor_dev_attr_##_name.dev_attr.attr

/* All CPLD sysfs attributes index */
enum cpld_sysfs_attributes {
    /********************************************** CPLD 1&2 common registers *********************************************/
    CPLD_MAJOR_VER,                 /* 0x02 */
    CPLD_MINOR_VER,                 /* 0x02 */
    CPLD_VERSION_H,
    CPLD_ID,                        /* 0x03 */
    CPLD_BUILD_VER,                 /* 0x04 */
    CPLD_CHIP_TYPE,                 /* 0x05 */
    CPLD_I2C_UPGRADE_MODULE_RESET,  /* 0xF0 BIT[0] */
    EVENT_CTRL,                     /* 0x3F */
    /******************************************************* CPLD 1 *******************************************************/
    SKU_ID,                         /* 0x00 */
    HW_REV,                         /* 0x01 */
    DEPH_ID,                        /* 0x01 */
    BUILD_ID,                       /* 0x01 */
    BIT_SEL_ID,                     /* 0x01 */
    EXT_ID,                         /* 0x06 */
    EXTEND_ID,                      /* 0xD0 */
    INTR_0,                         /* 0x10 */
    INTR_1,                         /* 0x10 */
    INTR_2,                         /* 0x10 */
    INTR_3,                         /* 0x10 */
    INTR_4,                         /* 0x10 */
    INTR_5,                         /* 0x10 */
    INTR_6,                         /* 0x12 */
    INTR_7,                         /* 0x12 */
    INTR_8,                         /* 0x13 */
    INTR_9,                         /* 0x14 */
    INTR_10,                        /* 0x14 */
    INTR_11,                        /* 0x14 */
    INTR_12,                        /* 0x14 */
    INTR_13,                        /* 0x14 */
    INTR_14,                        /* 0x16 */
    INTR_15,                        /* 0x16 */
    INTR_16,                        /* 0x16 */
    INTR_17,                        /* 0x16 */
    INTR_18,                        /* 0x17 */
    INTR_19,                        /* 0x17 */
    INTR_20,                        /* 0x17 */
    INTR_21,                        /* 0x17 */
    INTR_22,                        /* 0x19 */
    INTR_23,                        /* 0x1B */
    INTR_24,                        /* 0x1B */
    INTR_25,                        /* 0x1B */
    INTR_26,                        /* 0x1B */
    INTR_27,                        /* 0x1B */
    INTR_MASK_0,                    /* 0x20 */
    INTR_MASK_1,                    /* 0x20 */
    INTR_MASK_2,                    /* 0x20 */
    INTR_MASK_3,                    /* 0x20 */
    INTR_MASK_4,                    /* 0x20 */
    INTR_MASK_5,                    /* 0x20 */
    INTR_MASK_6,                    /* 0x22 */
    INTR_MASK_7,                    /* 0x22 */
    INTR_MASK_8,                    /* 0x23 */
    INTR_MASK_9,                    /* 0x24 */
    INTR_MASK_10,                   /* 0x24 */
    INTR_MASK_11,                   /* 0x24 */
    INTR_MASK_12,                   /* 0x24 */
    INTR_MASK_13,                   /* 0x24 */
    INTR_MASK_14,                   /* 0x26 */
    INTR_MASK_15,                   /* 0x26 */
    INTR_MASK_16,                   /* 0x26 */
    INTR_MASK_17,                   /* 0x26 */
    INTR_MASK_18,                   /* 0x27 */
    INTR_MASK_19,                   /* 0x27 */
    INTR_MASK_20,                   /* 0x27 */
    INTR_MASK_21,                   /* 0x27 */
    INTR_MASK_22,                   /* 0x2B */
    INTR_MASK_23,                   /* 0x2B */
    INTR_MASK_24,                   /* 0x2B */
    INTR_MASK_25,                   /* 0x2B */
    INTR_EVENT_CLK_PTP,             /* 0x30 */
    INTR_EVENT_PSU,                 /* 0x32 */
    INTR_EVENT_HWM,                 /* 0x33 */
    INTR_EVENT_THERMAL,             /* 0x34 */
    INTR_EVENT_FAN,                 /* 0x36 */
    INTR_EVENT_ETH,                 /* 0x37 */
    INTR_DEBUG_1,                   /* 0xE0 */
    INTR_DEBUG_2,                   /* 0xE0 */
    INTR_DEBUG_3,                   /* 0xE0 */
    INTR_DEBUG_4,                   /* 0xE0 */
    INTR_DEBUG_5,                   /* 0xE0 */
    INTR_DEBUG_6,                   /* 0xE0 */
    INTR_DEBUG_7,                   /* 0xE2 */
    INTR_DEBUG_8,                   /* 0xE2 */
    INTR_DEBUG_9,                   /* 0xE3 */
    INTR_DEBUG_10,                  /* 0xE4 */
    INTR_DEBUG_11,                  /* 0xE4 */
    INTR_DEBUG_12,                  /* 0xE6 */
    INTR_DEBUG_13,                  /* 0xE6 */
    INTR_DEBUG_14,                  /* 0xE6 */
    INTR_DEBUG_15,                  /* 0xE6 */
    INTR_DEBUG_16,                  /* 0xE7 */
    INTR_DEBUG_17,                  /* 0xE7 */
    INTR_DEBUG_18,                  /* 0xE7 */
    INTR_DEBUG_19,                  /* 0xE7 */
    INTR_DEBUG_20,                  /* 0xE9 */
    RESET_0 ,                        /* 0X40 */
    RESET_1 ,                        /* 0X40 */
    RESET_2 ,                        /* 0X40 */
    RESET_3 ,                        /* 0X40 */
    RESET_4 ,                        /* 0X41 */
    RESET_5 ,                        /* 0X41 */
    RESET_6 ,                        /* 0X42 */
    RESET_7 ,                        /* 0X42 */
    RESET_8 ,                        /* 0X42 */
    RESET_9 ,                       /* 0X42 */
    RESET_10,                       /* 0x43 */
    RESET_11,                       /* 0x43 */
    RESET_12,                       /* 0x43 */
    RESET_13,                       /* 0x43 */
    RESET_14,                       /* 0x43 */
    CPLD_ROV_RESET,                 /* 0xF0 BIT[1] */
    ROV_DEBUG_ENABLE,               /* 0xF1 */
    ROV_DEBUG_1,                    /* 0xF2 */
    ROV_DEBUG_2,                    /* 0xF2 */
    SYS_STATUS_1,                   /* 0x50 */
    SYS_STATUS_2,                   /* 0x50 */
    SYS_STATUS_3,                   /* 0X51 */
    SYS_STATUS_4,                   /* 0X51 */
    SYS_STATUS_5,                   /* 0X51 */
    SYS_STATUS_6,                   /* 0X51 */
    SYS_STATUS_7_MAC_ROV1,          /* 0X52 */
    SYS_STATUS_8_MAC_ROV2,          /* 0X52 */
    SYS_STATUS_9_MAC_ROV3,          /* 0X52 */
    SYS_STATUS_10,                  /* 0X53 */
    SYS_STATUS_11,                  /* 0X53 */
    SYS_STATUS_12,                  /* 0X53 */
    SYS_STATUS_13,                  /* 0X54 */
    SYS_STATUS_14,                  /* 0x56 SFP Port */
    SYS_STATUS_15,                  /* 0x56 SFP Port */
    SYS_STATUS_16,                  /* 0x56 SFP Port */
    SYS_STATUS_17,                  /* 0x56 SFP Port */
    SYS_STATUS_18,                  /* 0x56 SFP Port */
    SYS_STATUS_19,                  /* 0x57 */
    SYS_STATUS_20,                  /* 0x57 */
    SYS_STATUS_21,                  /* 0x57 */
    SYS_STATUS_22,                  /* 0x57 */
    SYS_STATUS_23,                  /* 0x57 */
    SYS_STATUS_24,                  /* 0x57 */
    SYS_STATUS_25,                  /* 0x57 */
    SYS_STATUS_26,                  /* 0x58 */
    SYS_STATUS_27,                  /* 0x58 */
    SYS_STATUS_28,                  /* 0x58 */
    SYS_STATUS_29,                  /* 0x58 */
    PSU0_PRESENT,                   /* 0x59 */
    PSU1_PRESENT,                   /* 0x59 */
    PSU0_VIN_PWOK,                  /* 0x59 */
    PSU1_VIN_PWOK,                  /* 0x59 */
    PSU0_VOUT_PWOK,                 /* 0x59 */
    PSU1_VOUT_PWOK,                 /* 0x59 */
    PSU_TYPE,                       /* 0x59 */
    BOOT_SELECT,                    /* 0x5B */
    CLK_TIMING_CTRL_1,              /* 0X5C Clock & Timing Control 1 Register */
    CLK_TIMING_CTRL_2,              /* 0x5D Clock & Timing Control 2 Register */
    PW_SYS_CTRL,                    /* 0x5E */
    GNSS_CTRL,                      /* 0x61 */
    USB_CTRL,                       /* 0x64 */
    //i2c mux select 0x65
    SYNCE_CTRL,                     /* 0x66 */
    TS_PLL_CLOCK_CTRL,              /* 0x67 */
    LED_CLEAR,                      /* 0x80 */
    SYS_LED_STATUS,                 /* 0x81 */
    SYS_LED_BLINKING,               /* 0x81 */
    SYS_LED_COLOR,                  /* 0x81 */
    GNSS_LED_STATUS,                /* 0x81 */
    GNSS_LED_BLINKING,              /* 0x81 */
    GNSS_LED_COLOR,                 /* 0x81 */
    SYNC_LED_STATUS,                /* 0x82 */
    SYNC_LED_BLINKING,              /* 0x82 */
    SYNC_LED_COLOR,                 /* 0x82 */
    PWR_LED_STATUS,                 /* 0x83 */
    PWR_LED_BLINKING,               /* 0x83 */
    PWR_LED_COLOR,                  /* 0x83 */
    FAN_LED_STATUS,                 /* 0x83 */
    FAN_LED_BLINKING,               /* 0x83 */
    FAN_LED_COLOR,                  /* 0x83 */
    SYS_LED,                        /* 0x81 */
    GNSS_LED,                       /* 0x81 */
    SYNC_LED,                       /* 0x82 */
    PWR_LED,                        /* 0x83 */
    FAN_LED,                        /* 0x83 */
    CPLD_TEST,                      /* 0xff */
    /******************************************************* CPLD 2 *******************************************************/
    PORT_0_ABS,                      /* 0x10 */
    PORT_1_ABS,                      /* 0x10 */
    PORT_2_ABS,                      /* 0x10 */
    PORT_3_ABS,                      /* 0x10 */
    PORT_4_ABS,                      /* 0x10 */
    PORT_5_ABS,                      /* 0x10 */
    PORT_6_ABS,                      /* 0x10 */
    PORT_7_ABS,                      /* 0x10 */
    PORT_8_ABS,                      /* 0x11 */
    PORT_9_ABS,                      /* 0x11 */
    PORT_10_ABS,                     /* 0x11 */
    PORT_11_ABS,                     /* 0x11 */
    PORT_12_ABS,                     /* 0x11 */
    PORT_13_ABS,                     /* 0x11 */
    PORT_14_ABS,                     /* 0x11 */
    PORT_15_ABS,                     /* 0x11 */
    PORT_16_ABS,                     /* 0x12 */
    PORT_17_ABS,                     /* 0x12 */
    PORT_18_ABS,                     /* 0x12 */
    PORT_19_ABS,                     /* 0x12 */
    PORT_0_ABS_MASK,                 /* 0x20 */
    PORT_1_ABS_MASK,                 /* 0x20 */
    PORT_2_ABS_MASK,                 /* 0x20 */
    PORT_3_ABS_MASK,                 /* 0x20 */
    PORT_4_ABS_MASK,                 /* 0x20 */
    PORT_5_ABS_MASK,                 /* 0x20 */
    PORT_6_ABS_MASK,                 /* 0x20 */
    PORT_7_ABS_MASK,                 /* 0x20 */
    PORT_8_ABS_MASK,                 /* 0x21 */
    PORT_9_ABS_MASK,                 /* 0x21 */
    PORT_10_ABS_MASK,                /* 0x21 */
    PORT_11_ABS_MASK,                /* 0x21 */
    PORT_12_ABS_MASK,                /* 0x21 */
    PORT_13_ABS_MASK,                /* 0x21 */
    PORT_14_ABS_MASK,                /* 0x21 */
    PORT_15_ABS_MASK,                /* 0x21 */
    PORT_16_ABS_MASK,                /* 0x22 */
    PORT_17_ABS_MASK,                /* 0x22 */
    PORT_18_ABS_MASK,                /* 0x22 */
    PORT_19_ABS_MASK,                /* 0x22 */
    PORT_0_7_ABS_EVENT,              /* 0x30 */
    PORT_8_15_ABS_EVENT,             /* 0x31 */
    PORT_16_19_ABS_EVENT,            /* 0x32 */
    PORT_0_ABS_DEBUG,                /* 0xE0 */
    PORT_1_ABS_DEBUG,                /* 0xE0 */
    PORT_2_ABS_DEBUG,                /* 0xE0 */
    PORT_3_ABS_DEBUG,                /* 0xE0 */
    PORT_4_ABS_DEBUG,                /* 0xE0 */
    PORT_5_ABS_DEBUG,                /* 0xE0 */
    PORT_6_ABS_DEBUG,                /* 0xE0 */
    PORT_7_ABS_DEBUG,                /* 0xE0 */
    PORT_8_ABS_DEBUG,                /* 0xE1 */
    PORT_9_ABS_DEBUG,                /* 0xE1 */
    PORT_10_ABS_DEBUG,               /* 0xE1 */
    PORT_11_ABS_DEBUG,               /* 0xE1 */
    PORT_12_ABS_DEBUG,               /* 0xE1 */
    PORT_13_ABS_DEBUG,               /* 0xE1 */
    PORT_14_ABS_DEBUG,               /* 0xE1 */
    PORT_15_ABS_DEBUG,               /* 0xE1 */
    PORT_16_ABS_DEBUG,               /* 0xE2 */
    PORT_17_ABS_DEBUG,               /* 0xE2 */
    PORT_18_ABS_DEBUG,               /* 0xE2 */
    PORT_19_ABS_DEBUG,               /* 0xE2 */
    PORT_0_RX_LOS,                   /* 0x17 */
    PORT_1_RX_LOS,                   /* 0x17 */
    PORT_2_RX_LOS,                   /* 0x17 */
    PORT_3_RX_LOS,                   /* 0x17 */
    PORT_4_RX_LOS,                   /* 0x17 */
    PORT_5_RX_LOS,                   /* 0x17 */
    PORT_6_RX_LOS,                   /* 0x17 */
    PORT_7_RX_LOS,                   /* 0x17 */
    PORT_8_RX_LOS,                   /* 0x18 */
    PORT_9_RX_LOS,                   /* 0x18 */
    PORT_10_RX_LOS,                  /* 0x18 */
    PORT_11_RX_LOS,                  /* 0x18 */
    PORT_12_RX_LOS,                  /* 0x18 */
    PORT_13_RX_LOS,                  /* 0x18 */
    PORT_14_RX_LOS,                  /* 0x18 */
    PORT_15_RX_LOS,                  /* 0x18 */
    PORT_16_RX_LOS,                  /* 0x19 */
    PORT_17_RX_LOS,                  /* 0x19 */
    PORT_18_RX_LOS,                  /* 0x19 */
    PORT_19_RX_LOS,                  /* 0x19 */
    PORT_0_RX_LOS_MASK,              /* 0x27 */
    PORT_1_RX_LOS_MASK,              /* 0x27 */
    PORT_2_RX_LOS_MASK,              /* 0x27 */
    PORT_3_RX_LOS_MASK,              /* 0x27 */
    PORT_4_RX_LOS_MASK,              /* 0x27 */
    PORT_5_RX_LOS_MASK,              /* 0x27 */
    PORT_6_RX_LOS_MASK,              /* 0x27 */
    PORT_7_RX_LOS_MASK,              /* 0x27 */
    PORT_8_RX_LOS_MASK,              /* 0x28 */
    PORT_9_RX_LOS_MASK,              /* 0x28 */
    PORT_10_RX_LOS_MASK,             /* 0x28 */
    PORT_11_RX_LOS_MASK,             /* 0x28 */
    PORT_12_RX_LOS_MASK,             /* 0x28 */
    PORT_13_RX_LOS_MASK,             /* 0x28 */
    PORT_14_RX_LOS_MASK,             /* 0x28 */
    PORT_15_RX_LOS_MASK,             /* 0x28 */
    PORT_16_RX_LOS_MASK,             /* 0x29 */
    PORT_17_RX_LOS_MASK,             /* 0x29 */
    PORT_18_RX_LOS_MASK,             /* 0x29 */
    PORT_19_RX_LOS_MASK,             /* 0x29 */
    PORT_0_7_RX_LOS_EVENT,           /* 0x37 */
    PORT_8_15_RX_LOS_EVENT,          /* 0x38 */
    PORT_16_19_RX_LOS_EVENT,         /* 0x39 */
    PORT_0_RX_LOS_DEBUG,             /* 0xE7 */
    PORT_1_RX_LOS_DEBUG,             /* 0xE7 */
    PORT_2_RX_LOS_DEBUG,             /* 0xE7 */
    PORT_3_RX_LOS_DEBUG,             /* 0xE7 */
    PORT_4_RX_LOS_DEBUG,             /* 0xE7 */
    PORT_5_RX_LOS_DEBUG,             /* 0xE7 */
    PORT_6_RX_LOS_DEBUG,             /* 0xE7 */
    PORT_7_RX_LOS_DEBUG,             /* 0xE7 */
    PORT_8_RX_LOS_DEBUG,             /* 0xE8 */
    PORT_9_RX_LOS_DEBUG,             /* 0xE8 */
    PORT_10_RX_LOS_DEBUG,            /* 0xE8 */
    PORT_11_RX_LOS_DEBUG,            /* 0xE8 */
    PORT_12_RX_LOS_DEBUG,            /* 0xE8 */
    PORT_13_RX_LOS_DEBUG,            /* 0xE8 */
    PORT_14_RX_LOS_DEBUG,            /* 0xE8 */
    PORT_15_RX_LOS_DEBUG,            /* 0xE8 */
    PORT_16_RX_LOS_DEBUG,            /* 0xE9 */
    PORT_17_RX_LOS_DEBUG,            /* 0xE9 */
    PORT_18_RX_LOS_DEBUG,            /* 0xE9 */
    PORT_19_RX_LOS_DEBUG,            /* 0xE9 */
    PORT_0_TX_FAULT,                 /* 0x1A */
    PORT_1_TX_FAULT,                 /* 0x1A */
    PORT_2_TX_FAULT,                 /* 0x1A */
    PORT_3_TX_FAULT,                 /* 0x1A */
    PORT_4_TX_FAULT,                 /* 0x1A */
    PORT_5_TX_FAULT,                 /* 0x1A */
    PORT_6_TX_FAULT,                 /* 0x1A */
    PORT_7_TX_FAULT,                 /* 0x1A */
    PORT_8_TX_FAULT,                 /* 0x1B */
    PORT_9_TX_FAULT,                 /* 0x1B */
    PORT_10_TX_FAULT,                /* 0x1B */
    PORT_11_TX_FAULT,                /* 0x1B */
    PORT_12_TX_FAULT,                /* 0x1B */
    PORT_13_TX_FAULT,                /* 0x1B */
    PORT_14_TX_FAULT,                /* 0x1B */
    PORT_15_TX_FAULT,                /* 0x1B */
    PORT_16_TX_FAULT,                /* 0x1C */
    PORT_17_TX_FAULT,                /* 0x1C */
    PORT_18_TX_FAULT,                /* 0x1C */
    PORT_19_TX_FAULT,                /* 0x1C */
    PORT_0_TX_FAULT_MASK,            /* 0x2A */
    PORT_1_TX_FAULT_MASK,            /* 0x2A */
    PORT_2_TX_FAULT_MASK,            /* 0x2A */
    PORT_3_TX_FAULT_MASK,            /* 0x2A */
    PORT_4_TX_FAULT_MASK,            /* 0x2A */
    PORT_5_TX_FAULT_MASK,            /* 0x2A */
    PORT_6_TX_FAULT_MASK,            /* 0x2A */
    PORT_7_TX_FAULT_MASK,            /* 0x2A */
    PORT_8_TX_FAULT_MASK,            /* 0x2B */
    PORT_9_TX_FAULT_MASK,            /* 0x2B */
    PORT_10_TX_FAULT_MASK,           /* 0x2B */
    PORT_11_TX_FAULT_MASK,           /* 0x2B */
    PORT_12_TX_FAULT_MASK,           /* 0x2B */
    PORT_13_TX_FAULT_MASK,           /* 0x2B */
    PORT_14_TX_FAULT_MASK,           /* 0x2B */
    PORT_15_TX_FAULT_MASK,           /* 0x2B */
    PORT_16_TX_FAULT_MASK,           /* 0x2C */
    PORT_17_TX_FAULT_MASK,           /* 0x2C */
    PORT_18_TX_FAULT_MASK,           /* 0x2C */
    PORT_19_TX_FAULT_MASK,           /* 0x2C */
    PORT_0_7_TX_FAULT_EVENT,         /* 0x3A */
    PORT_8_15_TX_FAULT_EVENT,        /* 0x3B */
    PORT_16_19_TX_FAULT_EVENT,       /* 0x3C */
    PORT_0_TX_FAULT_DEBUG,           /* 0xEA */
    PORT_1_TX_FAULT_DEBUG,           /* 0xEA */
    PORT_2_TX_FAULT_DEBUG,           /* 0xEA */
    PORT_3_TX_FAULT_DEBUG,           /* 0xEA */
    PORT_4_TX_FAULT_DEBUG,           /* 0xEA */
    PORT_5_TX_FAULT_DEBUG,           /* 0xEA */
    PORT_6_TX_FAULT_DEBUG,           /* 0xEA */
    PORT_7_TX_FAULT_DEBUG,           /* 0xEA */
    PORT_8_TX_FAULT_DEBUG,           /* 0xEB */
    PORT_9_TX_FAULT_DEBUG,           /* 0xEB */
    PORT_10_TX_FAULT_DEBUG,          /* 0xEB */
    PORT_11_TX_FAULT_DEBUG,          /* 0xEB */
    PORT_12_TX_FAULT_DEBUG,          /* 0xEB */
    PORT_13_TX_FAULT_DEBUG,          /* 0xEB */
    PORT_14_TX_FAULT_DEBUG,          /* 0xEB */
    PORT_15_TX_FAULT_DEBUG,          /* 0xEB */
    PORT_16_TX_FAULT_DEBUG,          /* 0xEC */
    PORT_17_TX_FAULT_DEBUG,          /* 0xEC */
    PORT_18_TX_FAULT_DEBUG,          /* 0xEC */
    PORT_19_TX_FAULT_DEBUG,          /* 0xEC */
    PORT_0_TX_DISABLE,               /* 0x40 */
    PORT_1_TX_DISABLE,               /* 0x40 */
    PORT_2_TX_DISABLE,               /* 0x40 */
    PORT_3_TX_DISABLE,               /* 0x40 */
    PORT_4_TX_DISABLE,               /* 0x40 */
    PORT_5_TX_DISABLE,               /* 0x40 */
    PORT_6_TX_DISABLE,               /* 0x40 */
    PORT_7_TX_DISABLE,               /* 0x40 */
    PORT_8_TX_DISABLE,               /* 0x41 */
    PORT_9_TX_DISABLE,               /* 0x41 */
    PORT_10_TX_DISABLE,              /* 0x41 */
    PORT_11_TX_DISABLE,              /* 0x41 */
    PORT_12_TX_DISABLE,              /* 0x41 */
    PORT_13_TX_DISABLE,              /* 0x41 */
    PORT_14_TX_DISABLE,              /* 0x41 */
    PORT_15_TX_DISABLE,              /* 0x41 */
    PORT_16_TX_DISABLE,              /* 0x42 */
    PORT_17_TX_DISABLE,              /* 0x42 */
    PORT_18_TX_DISABLE,              /* 0x42 */
    PORT_19_TX_DISABLE,              /* 0x42 */
    PORT_0_RATE_SEL,                 /* 0x43 */
    PORT_1_RATE_SEL,                 /* 0x43 */
    PORT_2_RATE_SEL,                 /* 0x43 */
    PORT_3_RATE_SEL,                 /* 0x43 */
    PORT_4_RATE_SEL,                 /* 0x43 */
    PORT_5_RATE_SEL,                 /* 0x43 */
    PORT_6_RATE_SEL,                 /* 0x43 */
    PORT_7_RATE_SEL,                 /* 0x43 */
    PORT_8_RATE_SEL,                 /* 0x44 */
    PORT_9_RATE_SEL,                 /* 0x44 */
    PORT_10_RATE_SEL,                /* 0x44 */
    PORT_11_RATE_SEL,                /* 0x44 */
    PORT_12_RATE_SEL,                /* 0x44 */
    PORT_13_RATE_SEL,                /* 0x44 */
    PORT_14_RATE_SEL,                /* 0x44 */
    PORT_15_RATE_SEL,                /* 0x44 */
    PORT_16_RATE_SEL,                /* 0x45 */
    PORT_17_RATE_SEL,                /* 0x45 */
    PORT_18_RATE_SEL,                /* 0x45 */
    PORT_19_RATE_SEL,                /* 0x45 */
    PORT_0_PWR_EN,                   /* 0x49 */
    PORT_1_PWR_EN,                   /* 0x49 */
    PORT_2_PWR_EN,                   /* 0x49 */
    PORT_3_PWR_EN,                   /* 0x49 */
    PORT_4_PWR_EN,                   /* 0x49 */
    PORT_5_PWR_EN,                   /* 0x49 */
    PORT_6_PWR_EN,                   /* 0x49 */
    PORT_7_PWR_EN,                   /* 0x49 */
    PORT_8_PWR_EN,                   /* 0x4A */
    PORT_9_PWR_EN,                   /* 0x4A */
    PORT_10_PWR_EN,                  /* 0x4A */
    PORT_11_PWR_EN,                  /* 0x4A */
    PORT_12_PWR_EN,                  /* 0x4A */
    PORT_13_PWR_EN,                  /* 0x4A */
    PORT_14_PWR_EN,                  /* 0x4A */
    PORT_15_PWR_EN,                  /* 0x4A */
    PORT_16_PWR_EN,                  /* 0x4B */
    PORT_17_PWR_EN,                  /* 0x4B */
    PORT_18_PWR_EN,                  /* 0x4B */
    PORT_19_PWR_EN,                  /* 0x4B */
    PSU_0_VIN_PWOK,                  /* 0x59 */
    PSU_1_VIN_PWOK,                  /* 0x59 */
    PSU_0_VOUT_PWOK,                 /* 0x59 */
    PSU_1_VOUT_PWOK,                 /* 0x59 */
    INTR_FAN_0,                      /* 0x70 */
    INTR_FAN_1,                      /* 0x70 */
    INTR_FAN_2,                      /* 0x70 */
    INTR_FAN_3,                      /* 0x70 */
    FAN_0_PWM_RPM_H,                 /* 0x71 */
    FAN_0_PWM_RPM_L,                 /* 0x72 */
    FAN_0_PWM_RPM,                   /* 0x71 + 0x72*/
    FAN_1_PWM_RPM_H,                 /* 0x73 */
    FAN_1_PWM_RPM_L,                 /* 0x74 */
    FAN_1_PWM_RPM,                   /* 0x73 + 0x74*/
    FAN_2_PWM_RPM_H,                 /* 0x75 */
    FAN_2_PWM_RPM_L,                 /* 0x76 */
    FAN_2_PWM_RPM,                   /* 0x75 + 0x76*/
    FAN_3_PWM_RPM_H,                 /* 0X77 */
    FAN_3_PWM_RPM_L,                 /* 0X78 */
    FAN_3_PWM_RPM,                   /* 0x77 + 0x78*/
    FAN_PWM_MODE_1,                  /* 0x80 */
    FAN_PWM_MODE_2,                  /* 0x80 */
    FAN_PWM_DIAG_CTRL_1,             /* 0x81 */
    FAN_PWM_DIAG_CTRL_2,             /* 0x82 */
    VOL_1_VALUE,                     /* 0xA0 */
    VOL_2_VALUE,                     /* 0xA1 */
    VOL_3_VALUE,                     /* 0xA2 */
    VOL_4_VALUE,                     /* 0xA3 */
    VOL_5_VALUE,                     /* 0xA4 */
    VOL_6_VALUE,                     /* 0xA5 */
    VOL_7_VALUE,                     /* 0xA6 */
    VOL_8_VALUE,                     /* 0xA7 */
    VOL_9_VALUE,                     /* 0xA8 */
    VOL_10_VALUE,                    /* 0xA9 */
    VOL_11_VALUE,                    /* 0xAA */
    VOL_12_VALUE,                    /* 0xAB */
    VOL_13_VALUE,                    /* 0xAC */
    VOL_14_VALUE,                    /* 0xAD */
    VOL_15_VALUE,                    /* 0xAE */
    VOL_16_VALUE,                    /* 0xAF */

    BSP_DEBUG,
};

enum data_type {
    DATA_HEX,
    DATA_DEC,
    DATA_UNK,
};

typedef struct {
    u8 reg;
    u8 mask;
    u8 data_type;
} attr_reg_map_t;

static attr_reg_map_t attr_reg[]= {
    /********************************************** CPLD 1&2 common registers *********************************************/
    [CPLD_MAJOR_VER]                    =   {VERSION_REG                           ,MASK_CPLD_MAJOR_VER        ,DATA_DEC},
    [CPLD_MINOR_VER]                    =   {VERSION_REG                           ,MASK_CPLD_MINOR_VER        ,DATA_DEC},
    [CPLD_VERSION_H]                    =   {NONE_REG                              ,MASK_NONE                  ,DATA_UNK},
    [CPLD_ID]                           =   {ID_REG                                ,MASK_CPLD_ID               ,DATA_DEC},
    [CPLD_BUILD_VER]                    =   {BUILD_REG                             ,MASK_ALL                   ,DATA_DEC},
    [CPLD_CHIP_TYPE]                    =   {CHIP_REG                              ,MASK_BIT1_0                ,DATA_HEX},
    [CPLD_I2C_UPGRADE_MODULE_RESET]     =   {MODULE_RESET_REG                      ,MASK_BIT0                  ,DATA_HEX},
    [EVENT_CTRL]                        =   {EVENT_DETECTION_REG                   ,MASK_BIT0                  ,DATA_HEX},
    /******************************************************* CPLD 1 *******************************************************/
    [SKU_ID]                            =   {BOARD_ID_1_REG                        ,MASK_ALL                   ,DATA_HEX},
    [HW_REV]                            =   {BOARD_ID_2_REG                        ,MASK_HW_REV                ,DATA_HEX},
    [DEPH_ID]                           =   {BOARD_ID_2_REG                        ,MASK_BIT2                  ,DATA_HEX},
    [BUILD_ID]                          =   {BOARD_ID_2_REG                        ,MASK_BIT4_3                ,DATA_HEX},
    [BIT_SEL_ID]                        =   {BOARD_ID_2_REG                        ,MASK_BIT5                  ,DATA_HEX},
    [EXT_ID]                            =   {REV_ID_REG                            ,MASK_BIT2_0                ,DATA_HEX},
    [EXTEND_ID]                         =   {BRD_EXT_ID_REG                        ,MASK_BIT7_6                ,DATA_HEX},
    [INTR_0]                            =   {INTR_1_REG_CLK_PTP                    ,MASK_BIT0                  ,DATA_HEX},
    [INTR_1]                            =   {INTR_1_REG_CLK_PTP                    ,MASK_BIT1                  ,DATA_HEX},
    [INTR_2]                            =   {INTR_1_REG_CLK_PTP                    ,MASK_BIT2                  ,DATA_HEX},
    [INTR_3]                            =   {INTR_1_REG_CLK_PTP                    ,MASK_BIT3                  ,DATA_HEX},
    [INTR_4]                            =   {INTR_1_REG_CLK_PTP                    ,MASK_BIT4                  ,DATA_HEX},
    [INTR_5]                            =   {INTR_1_REG_CLK_PTP                    ,MASK_BIT5                  ,DATA_HEX},
    [INTR_6]                            =   {INTR_2_REG_PSU                        ,MASK_BIT0                  ,DATA_HEX},
    [INTR_7]                            =   {INTR_2_REG_PSU                        ,MASK_BIT1                  ,DATA_HEX},
    [INTR_8]                            =   {INTR_3_REG_HWM                        ,MASK_BIT0                  ,DATA_HEX},
    [INTR_9]                            =   {INTR_4_REG_THERMAL                    ,MASK_BIT0                  ,DATA_HEX},
    [INTR_10]                           =   {INTR_4_REG_THERMAL                    ,MASK_BIT1                  ,DATA_HEX},
    [INTR_11]                           =   {INTR_4_REG_THERMAL                    ,MASK_BIT2                  ,DATA_HEX},
    [INTR_12]                           =   {INTR_4_REG_THERMAL                    ,MASK_BIT4                  ,DATA_HEX},
    [INTR_13]                           =   {INTR_4_REG_THERMAL                    ,MASK_BIT5                  ,DATA_HEX},
    [INTR_14]                           =   {INTR_5_REG_FAN                        ,MASK_BIT1                  ,DATA_HEX},
    [INTR_15]                           =   {INTR_5_REG_FAN                        ,MASK_BIT2                  ,DATA_HEX},
    [INTR_16]                           =   {INTR_5_REG_FAN                        ,MASK_BIT3                  ,DATA_HEX},
    [INTR_17]                           =   {INTR_5_REG_FAN                        ,MASK_BIT4                  ,DATA_HEX},
    [INTR_18]                           =   {INTR_6_REG_ETH                        ,MASK_BIT0                  ,DATA_HEX},
    [INTR_19]                           =   {INTR_6_REG_ETH                        ,MASK_BIT1                  ,DATA_HEX},
    [INTR_20]                           =   {INTR_6_REG_ETH                        ,MASK_BIT2                  ,DATA_HEX},
    [INTR_21]                           =   {INTR_6_REG_ETH                        ,MASK_BIT3                  ,DATA_HEX},
    [INTR_22]                           =   {INTR_7_REG_CPU_NMI                    ,MASK_BIT0                  ,DATA_HEX},
    [INTR_23]                           =   {INTR_8_REG_SYS                        ,MASK_BIT0                  ,DATA_HEX},
    [INTR_24]                           =   {INTR_8_REG_SYS                        ,MASK_BIT1                  ,DATA_HEX},
    [INTR_25]                           =   {INTR_8_REG_SYS                        ,MASK_BIT2                  ,DATA_HEX},
    [INTR_26]                           =   {INTR_8_REG_SYS                        ,MASK_BIT3                  ,DATA_HEX},
    [INTR_27]                           =   {INTR_8_REG_SYS                        ,MASK_BIT4                  ,DATA_HEX},
    [INTR_MASK_0]                       =   {INTR_MASK_1_REG_CLK_PTP               ,MASK_BIT0                  ,DATA_HEX},
    [INTR_MASK_1]                       =   {INTR_MASK_1_REG_CLK_PTP               ,MASK_BIT1                  ,DATA_HEX},
    [INTR_MASK_2]                       =   {INTR_MASK_1_REG_CLK_PTP               ,MASK_BIT2                  ,DATA_HEX},
    [INTR_MASK_3]                       =   {INTR_MASK_1_REG_CLK_PTP               ,MASK_BIT3                  ,DATA_HEX},
    [INTR_MASK_4]                       =   {INTR_MASK_1_REG_CLK_PTP               ,MASK_BIT4                  ,DATA_HEX},
    [INTR_MASK_5]                       =   {INTR_MASK_1_REG_CLK_PTP               ,MASK_BIT5                  ,DATA_HEX},
    [INTR_MASK_6]                       =   {INTR_MASK_2_REG_PSU                   ,MASK_BIT0                  ,DATA_HEX},
    [INTR_MASK_7]                       =   {INTR_MASK_2_REG_PSU                   ,MASK_BIT1                  ,DATA_HEX},
    [INTR_MASK_8]                       =   {INTR_MASK_3_REG_HWM                   ,MASK_BIT0                  ,DATA_HEX},
    [INTR_MASK_9]                       =   {INTR_MASK_4_REG_THERMAL               ,MASK_BIT0                  ,DATA_HEX},
    [INTR_MASK_10]                      =   {INTR_MASK_4_REG_THERMAL               ,MASK_BIT1                  ,DATA_HEX},
    [INTR_MASK_11]                      =   {INTR_MASK_4_REG_THERMAL               ,MASK_BIT2                  ,DATA_HEX},
    [INTR_MASK_12]                      =   {INTR_MASK_4_REG_THERMAL               ,MASK_BIT4                  ,DATA_HEX},
    [INTR_MASK_13]                      =   {INTR_MASK_4_REG_THERMAL               ,MASK_BIT5                  ,DATA_HEX},
    [INTR_MASK_14]                      =   {INTR_MASK_5_REG_FAN                   ,MASK_BIT1                  ,DATA_HEX},
    [INTR_MASK_15]                      =   {INTR_MASK_5_REG_FAN                   ,MASK_BIT2                  ,DATA_HEX},
    [INTR_MASK_16]                      =   {INTR_MASK_5_REG_FAN                   ,MASK_BIT3                  ,DATA_HEX},
    [INTR_MASK_17]                      =   {INTR_MASK_5_REG_FAN                   ,MASK_BIT4                  ,DATA_HEX},
    [INTR_MASK_18]                      =   {INTR_MASK_6_REG_ETH                   ,MASK_BIT0                  ,DATA_HEX},
    [INTR_MASK_19]                      =   {INTR_MASK_6_REG_ETH                   ,MASK_BIT1                  ,DATA_HEX},
    [INTR_MASK_20]                      =   {INTR_MASK_6_REG_ETH                   ,MASK_BIT2                  ,DATA_HEX},
    [INTR_MASK_21]                      =   {INTR_MASK_6_REG_ETH                   ,MASK_BIT3                  ,DATA_HEX},
    [INTR_MASK_22]                      =   {INTR_MASK_8_REG_SYS                   ,MASK_BIT0                  ,DATA_HEX},
    [INTR_MASK_23]                      =   {INTR_MASK_8_REG_SYS                   ,MASK_BIT2                  ,DATA_HEX},
    [INTR_MASK_24]                      =   {INTR_MASK_8_REG_SYS                   ,MASK_BIT3                  ,DATA_HEX},
    [INTR_MASK_25]                      =   {INTR_MASK_8_REG_SYS                   ,MASK_BIT4                  ,DATA_HEX},
    [INTR_EVENT_CLK_PTP]                =   {INTR_EVENT_1_REG_CLK_PTP              ,MASK_ALL                   ,DATA_HEX},
    [INTR_EVENT_PSU]                    =   {INTR_EVENT_2_REG_PSU                  ,MASK_ALL                   ,DATA_HEX},
    [INTR_EVENT_HWM]                    =   {INTR_EVENT_3_REG_HWM                  ,MASK_ALL                   ,DATA_HEX},
    [INTR_EVENT_THERMAL]                =   {INTR_EVENT_4_REG_THERMAL              ,MASK_ALL                   ,DATA_HEX},
    [INTR_EVENT_FAN]                    =   {INTR_EVENT_5_REG_FAN                  ,MASK_ALL                   ,DATA_HEX},
    [INTR_EVENT_ETH]                    =   {INTR_EVENT_6_REG_ETH                  ,MASK_ALL                   ,DATA_HEX},
    [INTR_DEBUG_1]                      =   {INTR_DEBUG_1_REG_CLK_PTP              ,MASK_BIT0                  ,DATA_HEX},
    [INTR_DEBUG_2]                      =   {INTR_DEBUG_1_REG_CLK_PTP              ,MASK_BIT1                  ,DATA_HEX},
    [INTR_DEBUG_3]                      =   {INTR_DEBUG_1_REG_CLK_PTP              ,MASK_BIT2                  ,DATA_HEX},
    [INTR_DEBUG_4]                      =   {INTR_DEBUG_1_REG_CLK_PTP              ,MASK_BIT3                  ,DATA_HEX},
    [INTR_DEBUG_5]                      =   {INTR_DEBUG_1_REG_CLK_PTP              ,MASK_BIT4                  ,DATA_HEX},
    [INTR_DEBUG_6]                      =   {INTR_DEBUG_1_REG_CLK_PTP              ,MASK_BIT5                  ,DATA_HEX},
    [INTR_DEBUG_7]                      =   {INTR_DEBUG_2_REG_PSU                  ,MASK_BIT0                  ,DATA_HEX},
    [INTR_DEBUG_8]                      =   {INTR_DEBUG_2_REG_PSU                  ,MASK_BIT1                  ,DATA_HEX},
    [INTR_DEBUG_9]                      =   {INTR_DEBUG_3_REG_HWM                  ,MASK_BIT0                  ,DATA_HEX},
    [INTR_DEBUG_10]                     =   {INTR_DEBUG_4_REG_THERMAL              ,MASK_BIT0                  ,DATA_HEX},
    [INTR_DEBUG_11]                     =   {INTR_DEBUG_4_REG_THERMAL              ,MASK_BIT1                  ,DATA_HEX},
    [INTR_DEBUG_12]                     =   {INTR_DEBUG_5_REG_FAN                  ,MASK_BIT1                  ,DATA_HEX},
    [INTR_DEBUG_13]                     =   {INTR_DEBUG_5_REG_FAN                  ,MASK_BIT2                  ,DATA_HEX},
    [INTR_DEBUG_14]                     =   {INTR_DEBUG_5_REG_FAN                  ,MASK_BIT3                  ,DATA_HEX},
    [INTR_DEBUG_15]                     =   {INTR_DEBUG_5_REG_FAN                  ,MASK_BIT4                  ,DATA_HEX},
    [INTR_DEBUG_16]                     =   {INTR_DEBUG_6_REG_ETH                  ,MASK_BIT0                  ,DATA_HEX},
    [INTR_DEBUG_17]                     =   {INTR_DEBUG_6_REG_ETH                  ,MASK_BIT1                  ,DATA_HEX},
    [INTR_DEBUG_18]                     =   {INTR_DEBUG_6_REG_ETH                  ,MASK_BIT2                  ,DATA_HEX},
    [INTR_DEBUG_19]                     =   {INTR_DEBUG_6_REG_ETH                  ,MASK_BIT3                  ,DATA_HEX},
    [INTR_DEBUG_20]                     =   {INTR_DEBUG_7_REG_CPU_NMI              ,MASK_BIT0                  ,DATA_HEX},
    [RESET_0]                           =   {RESET_1_REG_CLK_PTP                   ,MASK_BIT1                  ,DATA_HEX},
    [RESET_1]                           =   {RESET_1_REG_CLK_PTP                   ,MASK_BIT2                  ,DATA_HEX},
    [RESET_2]                           =   {RESET_1_REG_CLK_PTP                   ,MASK_BIT3                  ,DATA_HEX},
    [RESET_3]                           =   {RESET_1_REG_CLK_PTP                   ,MASK_BIT5                  ,DATA_HEX},
    [RESET_4]                           =   {RESET_2_REG_SYS                       ,MASK_BIT0                  ,DATA_HEX},
    [RESET_5]                           =   {RESET_2_REG_SYS                       ,MASK_BIT4                  ,DATA_HEX},
    [RESET_6]                           =   {RESET_3_REG_ETH                       ,MASK_BIT0                  ,DATA_HEX},
    [RESET_7]                           =   {RESET_3_REG_ETH                       ,MASK_BIT1                  ,DATA_HEX},
    [RESET_8]                           =   {RESET_3_REG_ETH                       ,MASK_BIT3                  ,DATA_HEX},
    [RESET_9]                           =   {RESET_3_REG_ETH                       ,MASK_BIT4                  ,DATA_HEX},
    [RESET_10]                          =   {RESET_4_REG_I2C_MUX                   ,MASK_BIT0                  ,DATA_HEX},
    [RESET_11]                          =   {RESET_4_REG_I2C_MUX                   ,MASK_BIT1                  ,DATA_HEX},
    [RESET_12]                          =   {RESET_4_REG_I2C_MUX                   ,MASK_BIT2                  ,DATA_HEX},
    [RESET_13]                          =   {RESET_4_REG_I2C_MUX                   ,MASK_BIT3                  ,DATA_HEX},
    [RESET_14]                          =   {RESET_4_REG_I2C_MUX                   ,MASK_BIT4                  ,DATA_HEX},
    [CPLD_ROV_RESET]                    =   {MODULE_RESET_REG                      ,MASK_BIT1                  ,DATA_HEX},
    [ROV_DEBUG_ENABLE]                  =   {CPLD_ENABLE_DEBUG_REG                 ,MASK_BIT0                  ,DATA_HEX},
    [ROV_DEBUG_1]                       =   {CPLD_ROV_DEBUG_REG                    ,MASK_BIT0                  ,DATA_HEX},
    [ROV_DEBUG_2]                       =   {CPLD_ROV_DEBUG_REG                    ,MASK_BIT2                  ,DATA_HEX},
    [SYS_STATUS_1]                      =   {SYS_STATUS_1_REG_CLK_TIMING_1         ,MASK_BIT0                  ,DATA_HEX},
    [SYS_STATUS_2]                      =   {SYS_STATUS_1_REG_CLK_TIMING_1         ,MASK_BIT1                  ,DATA_HEX},
    [SYS_STATUS_3]                      =   {SYS_STATUS_2_REG_CLK_TIMING_2         ,MASK_BIT3                  ,DATA_HEX},
    [SYS_STATUS_4]                      =   {SYS_STATUS_2_REG_CLK_TIMING_2         ,MASK_BIT4                  ,DATA_HEX},
    [SYS_STATUS_5]                      =   {SYS_STATUS_2_REG_CLK_TIMING_2         ,MASK_BIT5                  ,DATA_HEX},
    [SYS_STATUS_6]                      =   {SYS_STATUS_2_REG_CLK_TIMING_2         ,MASK_BIT6                  ,DATA_HEX},
    [SYS_STATUS_7_MAC_ROV1]             =   {SYS_STATUS_3_REG_ROV                  ,MASK_BIT0                  ,DATA_HEX},
    [SYS_STATUS_8_MAC_ROV2]             =   {SYS_STATUS_3_REG_ROV                  ,MASK_BIT1                  ,DATA_HEX},
    [SYS_STATUS_9_MAC_ROV3]             =   {SYS_STATUS_3_REG_ROV                  ,MASK_BIT2                  ,DATA_HEX},
    [SYS_STATUS_10]                     =   {SYS_STATUS_4_REG_GNSS                 ,MASK_BIT0                  ,DATA_HEX},
    [SYS_STATUS_11]                     =   {SYS_STATUS_4_REG_GNSS                 ,MASK_BIT1                  ,DATA_HEX},
    [SYS_STATUS_12]                     =   {SYS_STATUS_4_REG_GNSS                 ,MASK_BIT2                  ,DATA_HEX},
    [SYS_STATUS_13]                     =   {SYS_STATUS_5_REG_USB                  ,MASK_BIT2                  ,DATA_HEX},
    [SYS_STATUS_14]                     =   {SYS_STATUS_6_REG_IO_OVER_CURR         ,MASK_BIT0                  ,DATA_HEX},  //SFP Port 1-4 (not) over current
    [SYS_STATUS_15]                     =   {SYS_STATUS_6_REG_IO_OVER_CURR         ,MASK_BIT1                  ,DATA_HEX},  //SFP Port 5-8 (not) over current
    [SYS_STATUS_16]                     =   {SYS_STATUS_6_REG_IO_OVER_CURR         ,MASK_BIT2                  ,DATA_HEX},  //SFP Port 9-12 (not) over current
    [SYS_STATUS_17]                     =   {SYS_STATUS_6_REG_IO_OVER_CURR         ,MASK_BIT3                  ,DATA_HEX},  //SFP Port 13-16 (not) over current
    [SYS_STATUS_18]                     =   {SYS_STATUS_6_REG_IO_OVER_CURR         ,MASK_BIT4                  ,DATA_HEX},  //SFP Port 17-20 (not) over current
    [SYS_STATUS_19]                     =   {SYS_STATUS_7_REG_SYS_1                ,MASK_BIT0                  ,DATA_HEX},
    [SYS_STATUS_20]                     =   {SYS_STATUS_7_REG_SYS_1                ,MASK_BIT1                  ,DATA_HEX},
    [SYS_STATUS_21]                     =   {SYS_STATUS_7_REG_SYS_1                ,MASK_BIT2                  ,DATA_HEX},
    [SYS_STATUS_22]                     =   {SYS_STATUS_7_REG_SYS_1                ,MASK_BIT3                  ,DATA_HEX},
    [SYS_STATUS_23]                     =   {SYS_STATUS_7_REG_SYS_1                ,MASK_BIT4                  ,DATA_HEX},
    [SYS_STATUS_24]                     =   {SYS_STATUS_7_REG_SYS_1                ,MASK_BIT6                  ,DATA_HEX},
    [SYS_STATUS_25]                     =   {SYS_STATUS_7_REG_SYS_1                ,MASK_BIT7                  ,DATA_HEX},
    [SYS_STATUS_26]                     =   {SYS_STATUS_8_REG_SYS_2                ,MASK_BIT0                  ,DATA_HEX},
    [SYS_STATUS_27]                     =   {SYS_STATUS_8_REG_SYS_2                ,MASK_BIT1                  ,DATA_HEX},
    [SYS_STATUS_28]                     =   {SYS_STATUS_8_REG_SYS_2                ,MASK_BIT3                  ,DATA_HEX},
    [SYS_STATUS_29]                     =   {SYS_STATUS_8_REG_SYS_2                ,MASK_BIT4                  ,DATA_HEX},
    [PSU0_PRESENT]                      =   {SYS_STATUS_9_REG_PSU                  ,MASK_BIT0                  ,DATA_HEX},
    [PSU1_PRESENT]                      =   {SYS_STATUS_9_REG_PSU                  ,MASK_BIT1                  ,DATA_HEX},
    [PSU0_VIN_PWOK]                     =   {SYS_STATUS_9_REG_PSU                  ,MASK_BIT2                  ,DATA_HEX},
    [PSU1_VIN_PWOK]                     =   {SYS_STATUS_9_REG_PSU                  ,MASK_BIT3                  ,DATA_HEX},
    [PSU0_VOUT_PWOK]                    =   {SYS_STATUS_9_REG_PSU                  ,MASK_BIT4                  ,DATA_HEX},
    [PSU1_VOUT_PWOK]                    =   {SYS_STATUS_9_REG_PSU                  ,MASK_BIT5                  ,DATA_HEX},
    [PSU_TYPE]                          =   {SYS_STATUS_9_REG_PSU                  ,MASK_BIT6                  ,DATA_HEX},
    [BOOT_SELECT]                       =   {SYS_CONTROL_1_REG_BOOT_SEL            ,MASK_BIT1_0                ,DATA_HEX},
    [CLK_TIMING_CTRL_1]                 =   {SYS_CONTROL_2_REG_CLK_TIMING_1        ,MASK_ALL                   ,DATA_HEX},
    [CLK_TIMING_CTRL_2]                 =   {SYS_CONTROL_3_REG_CLK_TIMINH_2        ,MASK_CLK_TIMING_2_CTRL     ,DATA_HEX},
    [PW_SYS_CTRL]                       =   {SYS_CONTROL_4_REG_PW_SYS              ,MASK_PW_SYS_CTRL           ,DATA_HEX},
    [GNSS_CTRL]                         =   {SYS_CONTROL_6_REG_GNSS                ,MASK_BIT1_0                ,DATA_HEX},
    [USB_CTRL]                          =   {SYS_CONTROL_8_REG_USB                 ,MASK_BIT2                  ,DATA_HEX},
    [SYNCE_CTRL]                        =   {SYS_CONTROL_10_REG_SYNCE              ,MASK_BIT2_0                ,DATA_HEX},
    [TS_PLL_CLOCK_CTRL]                 =   {SYS_CONTROL_11_REG_TS_PLL_CLK         ,MASK_BIT0                  ,DATA_HEX},
    [LED_CLEAR]                         =   {SYS_LED_CONTROL_1_REG_LED_CLEAR       ,MASK_LED_CLEAR             ,DATA_HEX},
    [GNSS_LED_COLOR]                    =   {SYS_LED_CONTROL_2_REG_SYS_LED_1       ,MASK_BIT0                  ,DATA_HEX},
    [GNSS_LED_BLINKING]                 =   {SYS_LED_CONTROL_2_REG_SYS_LED_1       ,MASK_BIT2                  ,DATA_HEX},
    [GNSS_LED_STATUS]                   =   {SYS_LED_CONTROL_2_REG_SYS_LED_1       ,MASK_BIT3                  ,DATA_HEX},
    [SYS_LED_COLOR]                     =   {SYS_LED_CONTROL_2_REG_SYS_LED_1       ,MASK_BIT4                  ,DATA_HEX},
    [SYS_LED_BLINKING]                  =   {SYS_LED_CONTROL_2_REG_SYS_LED_1       ,MASK_BIT6                  ,DATA_HEX},
    [SYS_LED_STATUS]                    =   {SYS_LED_CONTROL_2_REG_SYS_LED_1       ,MASK_BIT7                  ,DATA_HEX},
    [SYNC_LED_COLOR]                    =   {SYS_LED_CONTROL_3_REG_SYS_LED_2       ,MASK_BIT0                  ,DATA_HEX},
    [SYNC_LED_BLINKING]                 =   {SYS_LED_CONTROL_3_REG_SYS_LED_2       ,MASK_BIT2                  ,DATA_HEX},
    [SYNC_LED_STATUS]                   =   {SYS_LED_CONTROL_3_REG_SYS_LED_2       ,MASK_BIT3                  ,DATA_HEX},
    [FAN_LED_COLOR]                     =   {SYS_STATUS_10_REG_SYS_LED_1           ,MASK_BIT0                  ,DATA_HEX},
    [FAN_LED_BLINKING]                  =   {SYS_STATUS_10_REG_SYS_LED_1           ,MASK_BIT2                  ,DATA_HEX},
    [FAN_LED_STATUS]                    =   {SYS_STATUS_10_REG_SYS_LED_1           ,MASK_BIT3                  ,DATA_HEX},
    [PWR_LED_COLOR]                     =   {SYS_STATUS_10_REG_SYS_LED_1           ,MASK_BIT4                  ,DATA_HEX},
    [PWR_LED_BLINKING]                  =   {SYS_STATUS_10_REG_SYS_LED_1           ,MASK_BIT6                  ,DATA_HEX},
    [PWR_LED_STATUS]                    =   {SYS_STATUS_10_REG_SYS_LED_1           ,MASK_BIT7                  ,DATA_HEX},
    [GNSS_LED]                          =   {SYS_LED_CONTROL_2_REG_SYS_LED_1       ,MASK_LB                    ,DATA_HEX},
    [SYS_LED]                           =   {SYS_LED_CONTROL_2_REG_SYS_LED_1       ,MASK_HB                    ,DATA_HEX},
    [SYNC_LED]                          =   {SYS_LED_CONTROL_3_REG_SYS_LED_2       ,MASK_LB                    ,DATA_HEX},
    [FAN_LED]                           =   {SYS_STATUS_10_REG_SYS_LED_1           ,MASK_LB                    ,DATA_HEX},
    [PWR_LED]                           =   {SYS_STATUS_10_REG_SYS_LED_1           ,MASK_HB                    ,DATA_HEX},
    [CPLD_TEST]                         =   {CPLD_TEST_REG                         ,MASK_ALL                   ,DATA_HEX},
    /******************************************************* CPLD 2 *******************************************************/
    [PORT_0_ABS]                        =   {PORT_INTR_1_REG                       ,MASK_BIT0                  ,DATA_HEX},
    [PORT_1_ABS]                        =   {PORT_INTR_1_REG                       ,MASK_BIT1                  ,DATA_HEX},
    [PORT_2_ABS]                        =   {PORT_INTR_1_REG                       ,MASK_BIT2                  ,DATA_HEX},
    [PORT_3_ABS]                        =   {PORT_INTR_1_REG                       ,MASK_BIT3                  ,DATA_HEX},
    [PORT_4_ABS]                        =   {PORT_INTR_1_REG                       ,MASK_BIT4                  ,DATA_HEX},
    [PORT_5_ABS]                        =   {PORT_INTR_1_REG                       ,MASK_BIT5                  ,DATA_HEX},
    [PORT_6_ABS]                        =   {PORT_INTR_1_REG                       ,MASK_BIT6                  ,DATA_HEX},
    [PORT_7_ABS]                        =   {PORT_INTR_1_REG                       ,MASK_BIT7                  ,DATA_HEX},
    [PORT_8_ABS]                        =   {PORT_INTR_2_REG                       ,MASK_BIT0                  ,DATA_HEX},
    [PORT_9_ABS]                        =   {PORT_INTR_2_REG                       ,MASK_BIT1                  ,DATA_HEX},
    [PORT_10_ABS]                       =   {PORT_INTR_2_REG                       ,MASK_BIT2                  ,DATA_HEX},
    [PORT_11_ABS]                       =   {PORT_INTR_2_REG                       ,MASK_BIT3                  ,DATA_HEX},
    [PORT_12_ABS]                       =   {PORT_INTR_2_REG                       ,MASK_BIT4                  ,DATA_HEX},
    [PORT_13_ABS]                       =   {PORT_INTR_2_REG                       ,MASK_BIT5                  ,DATA_HEX},
    [PORT_14_ABS]                       =   {PORT_INTR_2_REG                       ,MASK_BIT6                  ,DATA_HEX},
    [PORT_15_ABS]                       =   {PORT_INTR_2_REG                       ,MASK_BIT7                  ,DATA_HEX},
    [PORT_16_ABS]                       =   {PORT_INTR_3_REG                       ,MASK_BIT0                  ,DATA_HEX},
    [PORT_17_ABS]                       =   {PORT_INTR_3_REG                       ,MASK_BIT1                  ,DATA_HEX},
    [PORT_18_ABS]                       =   {PORT_INTR_3_REG                       ,MASK_BIT2                  ,DATA_HEX},
    [PORT_19_ABS]                       =   {PORT_INTR_3_REG                       ,MASK_BIT3                  ,DATA_HEX},
    [PORT_0_ABS_MASK]                   =   {PORT_INTR_MASK_1_REG                  ,MASK_BIT0                  ,DATA_HEX},
    [PORT_1_ABS_MASK]                   =   {PORT_INTR_MASK_1_REG                  ,MASK_BIT1                  ,DATA_HEX},
    [PORT_2_ABS_MASK]                   =   {PORT_INTR_MASK_1_REG                  ,MASK_BIT2                  ,DATA_HEX},
    [PORT_3_ABS_MASK]                   =   {PORT_INTR_MASK_1_REG                  ,MASK_BIT3                  ,DATA_HEX},
    [PORT_4_ABS_MASK]                   =   {PORT_INTR_MASK_1_REG                  ,MASK_BIT4                  ,DATA_HEX},
    [PORT_5_ABS_MASK]                   =   {PORT_INTR_MASK_1_REG                  ,MASK_BIT5                  ,DATA_HEX},
    [PORT_6_ABS_MASK]                   =   {PORT_INTR_MASK_1_REG                  ,MASK_BIT6                  ,DATA_HEX},
    [PORT_7_ABS_MASK]                   =   {PORT_INTR_MASK_1_REG                  ,MASK_BIT7                  ,DATA_HEX},
    [PORT_8_ABS_MASK]                   =   {PORT_INTR_MASK_2_REG                  ,MASK_BIT0                  ,DATA_HEX},
    [PORT_9_ABS_MASK]                   =   {PORT_INTR_MASK_2_REG                  ,MASK_BIT1                  ,DATA_HEX},
    [PORT_10_ABS_MASK]                  =   {PORT_INTR_MASK_2_REG                  ,MASK_BIT2                  ,DATA_HEX},
    [PORT_11_ABS_MASK]                  =   {PORT_INTR_MASK_2_REG                  ,MASK_BIT3                  ,DATA_HEX},
    [PORT_12_ABS_MASK]                  =   {PORT_INTR_MASK_2_REG                  ,MASK_BIT4                  ,DATA_HEX},
    [PORT_13_ABS_MASK]                  =   {PORT_INTR_MASK_2_REG                  ,MASK_BIT5                  ,DATA_HEX},
    [PORT_14_ABS_MASK]                  =   {PORT_INTR_MASK_2_REG                  ,MASK_BIT6                  ,DATA_HEX},
    [PORT_15_ABS_MASK]                  =   {PORT_INTR_MASK_2_REG                  ,MASK_BIT7                  ,DATA_HEX},
    [PORT_16_ABS_MASK]                  =   {PORT_INTR_MASK_3_REG                  ,MASK_BIT0                  ,DATA_HEX},
    [PORT_17_ABS_MASK]                  =   {PORT_INTR_MASK_3_REG                  ,MASK_BIT1                  ,DATA_HEX},
    [PORT_18_ABS_MASK]                  =   {PORT_INTR_MASK_3_REG                  ,MASK_BIT2                  ,DATA_HEX},
    [PORT_19_ABS_MASK]                  =   {PORT_INTR_MASK_3_REG                  ,MASK_BIT3                  ,DATA_HEX},
    [PORT_0_7_ABS_EVENT]                =   {PORT_INTR_EVENT_1_REG                 ,MASK_ALL                   ,DATA_HEX},
    [PORT_8_15_ABS_EVENT]               =   {PORT_INTR_EVENT_2_REG                 ,MASK_ALL                   ,DATA_HEX},
    [PORT_16_19_ABS_EVENT]              =   {PORT_INTR_EVENT_3_REG                 ,MASK_ALL                   ,DATA_HEX},
    [PORT_0_ABS_DEBUG]                  =   {PORT_INTR_DEBUG_1_REG                 ,MASK_BIT0                  ,DATA_HEX},
    [PORT_1_ABS_DEBUG]                  =   {PORT_INTR_DEBUG_1_REG                 ,MASK_BIT1                  ,DATA_HEX},
    [PORT_2_ABS_DEBUG]                  =   {PORT_INTR_DEBUG_1_REG                 ,MASK_BIT2                  ,DATA_HEX},
    [PORT_3_ABS_DEBUG]                  =   {PORT_INTR_DEBUG_1_REG                 ,MASK_BIT3                  ,DATA_HEX},
    [PORT_4_ABS_DEBUG]                  =   {PORT_INTR_DEBUG_1_REG                 ,MASK_BIT4                  ,DATA_HEX},
    [PORT_5_ABS_DEBUG]                  =   {PORT_INTR_DEBUG_1_REG                 ,MASK_BIT5                  ,DATA_HEX},
    [PORT_6_ABS_DEBUG]                  =   {PORT_INTR_DEBUG_1_REG                 ,MASK_BIT6                  ,DATA_HEX},
    [PORT_7_ABS_DEBUG]                  =   {PORT_INTR_DEBUG_1_REG                 ,MASK_BIT7                  ,DATA_HEX},
    [PORT_8_ABS_DEBUG]                  =   {PORT_INTR_DEBUG_2_REG                 ,MASK_BIT0                  ,DATA_HEX},
    [PORT_9_ABS_DEBUG]                  =   {PORT_INTR_DEBUG_2_REG                 ,MASK_BIT1                  ,DATA_HEX},
    [PORT_10_ABS_DEBUG]                 =   {PORT_INTR_DEBUG_2_REG                 ,MASK_BIT2                  ,DATA_HEX},
    [PORT_11_ABS_DEBUG]                 =   {PORT_INTR_DEBUG_2_REG                 ,MASK_BIT3                  ,DATA_HEX},
    [PORT_12_ABS_DEBUG]                 =   {PORT_INTR_DEBUG_2_REG                 ,MASK_BIT4                  ,DATA_HEX},
    [PORT_13_ABS_DEBUG]                 =   {PORT_INTR_DEBUG_2_REG                 ,MASK_BIT5                  ,DATA_HEX},
    [PORT_14_ABS_DEBUG]                 =   {PORT_INTR_DEBUG_2_REG                 ,MASK_BIT6                  ,DATA_HEX},
    [PORT_15_ABS_DEBUG]                 =   {PORT_INTR_DEBUG_2_REG                 ,MASK_BIT7                  ,DATA_HEX},
    [PORT_16_ABS_DEBUG]                 =   {PORT_INTR_DEBUG_3_REG                 ,MASK_BIT0                  ,DATA_HEX},
    [PORT_17_ABS_DEBUG]                 =   {PORT_INTR_DEBUG_3_REG                 ,MASK_BIT1                  ,DATA_HEX},
    [PORT_18_ABS_DEBUG]                 =   {PORT_INTR_DEBUG_3_REG                 ,MASK_BIT2                  ,DATA_HEX},
    [PORT_19_ABS_DEBUG]                 =   {PORT_INTR_DEBUG_3_REG                 ,MASK_BIT3                  ,DATA_HEX},
    [PORT_0_RX_LOS]                     =   {PORT_INTR_4_REG                       ,MASK_BIT0                  ,DATA_HEX},
    [PORT_1_RX_LOS]                     =   {PORT_INTR_4_REG                       ,MASK_BIT1                  ,DATA_HEX},
    [PORT_2_RX_LOS]                     =   {PORT_INTR_4_REG                       ,MASK_BIT2                  ,DATA_HEX},
    [PORT_3_RX_LOS]                     =   {PORT_INTR_4_REG                       ,MASK_BIT3                  ,DATA_HEX},
    [PORT_4_RX_LOS]                     =   {PORT_INTR_4_REG                       ,MASK_BIT4                  ,DATA_HEX},
    [PORT_5_RX_LOS]                     =   {PORT_INTR_4_REG                       ,MASK_BIT5                  ,DATA_HEX},
    [PORT_6_RX_LOS]                     =   {PORT_INTR_4_REG                       ,MASK_BIT6                  ,DATA_HEX},
    [PORT_7_RX_LOS]                     =   {PORT_INTR_4_REG                       ,MASK_BIT7                  ,DATA_HEX},
    [PORT_8_RX_LOS]                     =   {PORT_INTR_5_REG                       ,MASK_BIT0                  ,DATA_HEX},
    [PORT_9_RX_LOS]                     =   {PORT_INTR_5_REG                       ,MASK_BIT1                  ,DATA_HEX},
    [PORT_10_RX_LOS]                    =   {PORT_INTR_5_REG                       ,MASK_BIT2                  ,DATA_HEX},
    [PORT_11_RX_LOS]                    =   {PORT_INTR_5_REG                       ,MASK_BIT3                  ,DATA_HEX},
    [PORT_12_RX_LOS]                    =   {PORT_INTR_5_REG                       ,MASK_BIT4                  ,DATA_HEX},
    [PORT_13_RX_LOS]                    =   {PORT_INTR_5_REG                       ,MASK_BIT5                  ,DATA_HEX},
    [PORT_14_RX_LOS]                    =   {PORT_INTR_5_REG                       ,MASK_BIT6                  ,DATA_HEX},
    [PORT_15_RX_LOS]                    =   {PORT_INTR_5_REG                       ,MASK_BIT7                  ,DATA_HEX},
    [PORT_16_RX_LOS]                    =   {PORT_INTR_6_REG                       ,MASK_BIT0                  ,DATA_HEX},
    [PORT_17_RX_LOS]                    =   {PORT_INTR_6_REG                       ,MASK_BIT1                  ,DATA_HEX},
    [PORT_18_RX_LOS]                    =   {PORT_INTR_6_REG                       ,MASK_BIT2                  ,DATA_HEX},
    [PORT_19_RX_LOS]                    =   {PORT_INTR_6_REG                       ,MASK_BIT3                  ,DATA_HEX},
    [PORT_0_RX_LOS_MASK]                =   {PORT_INTR_MASK_4_REG                  ,MASK_BIT0                  ,DATA_HEX},
    [PORT_1_RX_LOS_MASK]                =   {PORT_INTR_MASK_4_REG                  ,MASK_BIT1                  ,DATA_HEX},
    [PORT_2_RX_LOS_MASK]                =   {PORT_INTR_MASK_4_REG                  ,MASK_BIT2                  ,DATA_HEX},
    [PORT_3_RX_LOS_MASK]                =   {PORT_INTR_MASK_4_REG                  ,MASK_BIT3                  ,DATA_HEX},
    [PORT_4_RX_LOS_MASK]                =   {PORT_INTR_MASK_4_REG                  ,MASK_BIT4                  ,DATA_HEX},
    [PORT_5_RX_LOS_MASK]                =   {PORT_INTR_MASK_4_REG                  ,MASK_BIT5                  ,DATA_HEX},
    [PORT_6_RX_LOS_MASK]                =   {PORT_INTR_MASK_4_REG                  ,MASK_BIT6                  ,DATA_HEX},
    [PORT_7_RX_LOS_MASK]                =   {PORT_INTR_MASK_4_REG                  ,MASK_BIT7                  ,DATA_HEX},
    [PORT_8_RX_LOS_MASK]                =   {PORT_INTR_MASK_5_REG                  ,MASK_BIT0                  ,DATA_HEX},
    [PORT_9_RX_LOS_MASK]                =   {PORT_INTR_MASK_5_REG                  ,MASK_BIT1                  ,DATA_HEX},
    [PORT_10_RX_LOS_MASK]               =   {PORT_INTR_MASK_5_REG                  ,MASK_BIT2                  ,DATA_HEX},
    [PORT_11_RX_LOS_MASK]               =   {PORT_INTR_MASK_5_REG                  ,MASK_BIT3                  ,DATA_HEX},
    [PORT_12_RX_LOS_MASK]               =   {PORT_INTR_MASK_5_REG                  ,MASK_BIT4                  ,DATA_HEX},
    [PORT_13_RX_LOS_MASK]               =   {PORT_INTR_MASK_5_REG                  ,MASK_BIT5                  ,DATA_HEX},
    [PORT_14_RX_LOS_MASK]               =   {PORT_INTR_MASK_5_REG                  ,MASK_BIT6                  ,DATA_HEX},
    [PORT_15_RX_LOS_MASK]               =   {PORT_INTR_MASK_5_REG                  ,MASK_BIT7                  ,DATA_HEX},
    [PORT_16_RX_LOS_MASK]               =   {PORT_INTR_MASK_6_REG                  ,MASK_BIT0                  ,DATA_HEX},
    [PORT_17_RX_LOS_MASK]               =   {PORT_INTR_MASK_6_REG                  ,MASK_BIT1                  ,DATA_HEX},
    [PORT_18_RX_LOS_MASK]               =   {PORT_INTR_MASK_6_REG                  ,MASK_BIT2                  ,DATA_HEX},
    [PORT_19_RX_LOS_MASK]               =   {PORT_INTR_MASK_6_REG                  ,MASK_BIT3                  ,DATA_HEX},
    [PORT_0_7_RX_LOS_EVENT]             =   {PORT_INTR_EVENT_4_REG                 ,MASK_ALL                   ,DATA_HEX},
    [PORT_8_15_RX_LOS_EVENT]            =   {PORT_INTR_EVENT_5_REG                 ,MASK_ALL                   ,DATA_HEX},
    [PORT_16_19_RX_LOS_EVENT]           =   {PORT_INTR_EVENT_6_REG                 ,MASK_ALL                   ,DATA_HEX},
    [PORT_0_RX_LOS_DEBUG]               =   {PORT_INTR_DEBUG_4_REG                 ,MASK_BIT0                  ,DATA_HEX},
    [PORT_1_RX_LOS_DEBUG]               =   {PORT_INTR_DEBUG_4_REG                 ,MASK_BIT1                  ,DATA_HEX},
    [PORT_2_RX_LOS_DEBUG]               =   {PORT_INTR_DEBUG_4_REG                 ,MASK_BIT2                  ,DATA_HEX},
    [PORT_3_RX_LOS_DEBUG]               =   {PORT_INTR_DEBUG_4_REG                 ,MASK_BIT3                  ,DATA_HEX},
    [PORT_4_RX_LOS_DEBUG]               =   {PORT_INTR_DEBUG_4_REG                 ,MASK_BIT4                  ,DATA_HEX},
    [PORT_5_RX_LOS_DEBUG]               =   {PORT_INTR_DEBUG_4_REG                 ,MASK_BIT5                  ,DATA_HEX},
    [PORT_6_RX_LOS_DEBUG]               =   {PORT_INTR_DEBUG_4_REG                 ,MASK_BIT6                  ,DATA_HEX},
    [PORT_7_RX_LOS_DEBUG]               =   {PORT_INTR_DEBUG_4_REG                 ,MASK_BIT7                  ,DATA_HEX},
    [PORT_8_RX_LOS_DEBUG]               =   {PORT_INTR_DEBUG_5_REG                 ,MASK_BIT0                  ,DATA_HEX},
    [PORT_9_RX_LOS_DEBUG]               =   {PORT_INTR_DEBUG_5_REG                 ,MASK_BIT1                  ,DATA_HEX},
    [PORT_10_RX_LOS_DEBUG]              =   {PORT_INTR_DEBUG_5_REG                 ,MASK_BIT2                  ,DATA_HEX},
    [PORT_11_RX_LOS_DEBUG]              =   {PORT_INTR_DEBUG_5_REG                 ,MASK_BIT3                  ,DATA_HEX},
    [PORT_12_RX_LOS_DEBUG]              =   {PORT_INTR_DEBUG_5_REG                 ,MASK_BIT4                  ,DATA_HEX},
    [PORT_13_RX_LOS_DEBUG]              =   {PORT_INTR_DEBUG_5_REG                 ,MASK_BIT5                  ,DATA_HEX},
    [PORT_14_RX_LOS_DEBUG]              =   {PORT_INTR_DEBUG_5_REG                 ,MASK_BIT6                  ,DATA_HEX},
    [PORT_15_RX_LOS_DEBUG]              =   {PORT_INTR_DEBUG_5_REG                 ,MASK_BIT7                  ,DATA_HEX},
    [PORT_16_RX_LOS_DEBUG]              =   {PORT_INTR_DEBUG_6_REG                 ,MASK_BIT0                  ,DATA_HEX},
    [PORT_17_RX_LOS_DEBUG]              =   {PORT_INTR_DEBUG_6_REG                 ,MASK_BIT1                  ,DATA_HEX},
    [PORT_18_RX_LOS_DEBUG]              =   {PORT_INTR_DEBUG_6_REG                 ,MASK_BIT2                  ,DATA_HEX},
    [PORT_19_RX_LOS_DEBUG]              =   {PORT_INTR_DEBUG_6_REG                 ,MASK_BIT3                  ,DATA_HEX},
    [PORT_0_TX_FAULT]                   =   {PORT_INTR_7_REG                       ,MASK_BIT0                  ,DATA_HEX},
    [PORT_1_TX_FAULT]                   =   {PORT_INTR_7_REG                       ,MASK_BIT1                  ,DATA_HEX},
    [PORT_2_TX_FAULT]                   =   {PORT_INTR_7_REG                       ,MASK_BIT2                  ,DATA_HEX},
    [PORT_3_TX_FAULT]                   =   {PORT_INTR_7_REG                       ,MASK_BIT3                  ,DATA_HEX},
    [PORT_4_TX_FAULT]                   =   {PORT_INTR_7_REG                       ,MASK_BIT4                  ,DATA_HEX},
    [PORT_5_TX_FAULT]                   =   {PORT_INTR_7_REG                       ,MASK_BIT5                  ,DATA_HEX},
    [PORT_6_TX_FAULT]                   =   {PORT_INTR_7_REG                       ,MASK_BIT6                  ,DATA_HEX},
    [PORT_7_TX_FAULT]                   =   {PORT_INTR_7_REG                       ,MASK_BIT7                  ,DATA_HEX},
    [PORT_8_TX_FAULT]                   =   {PORT_INTR_8_REG                       ,MASK_BIT0                  ,DATA_HEX},
    [PORT_9_TX_FAULT]                   =   {PORT_INTR_8_REG                       ,MASK_BIT1                  ,DATA_HEX},
    [PORT_10_TX_FAULT]                  =   {PORT_INTR_8_REG                       ,MASK_BIT2                  ,DATA_HEX},
    [PORT_11_TX_FAULT]                  =   {PORT_INTR_8_REG                       ,MASK_BIT3                  ,DATA_HEX},
    [PORT_12_TX_FAULT]                  =   {PORT_INTR_8_REG                       ,MASK_BIT4                  ,DATA_HEX},
    [PORT_13_TX_FAULT]                  =   {PORT_INTR_8_REG                       ,MASK_BIT5                  ,DATA_HEX},
    [PORT_14_TX_FAULT]                  =   {PORT_INTR_8_REG                       ,MASK_BIT6                  ,DATA_HEX},
    [PORT_15_TX_FAULT]                  =   {PORT_INTR_8_REG                       ,MASK_BIT7                  ,DATA_HEX},
    [PORT_16_TX_FAULT]                  =   {PORT_INTR_9_REG                       ,MASK_BIT0                  ,DATA_HEX},
    [PORT_17_TX_FAULT]                  =   {PORT_INTR_9_REG                       ,MASK_BIT1                  ,DATA_HEX},
    [PORT_18_TX_FAULT]                  =   {PORT_INTR_9_REG                       ,MASK_BIT2                  ,DATA_HEX},
    [PORT_19_TX_FAULT]                  =   {PORT_INTR_9_REG                       ,MASK_BIT3                  ,DATA_HEX},
    [PORT_0_TX_FAULT_MASK]              =   {PORT_INTR_MASK_7_REG                  ,MASK_BIT0                  ,DATA_HEX},
    [PORT_1_TX_FAULT_MASK]              =   {PORT_INTR_MASK_7_REG                  ,MASK_BIT1                  ,DATA_HEX},
    [PORT_2_TX_FAULT_MASK]              =   {PORT_INTR_MASK_7_REG                  ,MASK_BIT2                  ,DATA_HEX},
    [PORT_3_TX_FAULT_MASK]              =   {PORT_INTR_MASK_7_REG                  ,MASK_BIT3                  ,DATA_HEX},
    [PORT_4_TX_FAULT_MASK]              =   {PORT_INTR_MASK_7_REG                  ,MASK_BIT4                  ,DATA_HEX},
    [PORT_5_TX_FAULT_MASK]              =   {PORT_INTR_MASK_7_REG                  ,MASK_BIT5                  ,DATA_HEX},
    [PORT_6_TX_FAULT_MASK]              =   {PORT_INTR_MASK_7_REG                  ,MASK_BIT6                  ,DATA_HEX},
    [PORT_7_TX_FAULT_MASK]              =   {PORT_INTR_MASK_7_REG                  ,MASK_BIT7                  ,DATA_HEX},
    [PORT_8_TX_FAULT_MASK]              =   {PORT_INTR_MASK_8_REG                  ,MASK_BIT0                  ,DATA_HEX},
    [PORT_9_TX_FAULT_MASK]              =   {PORT_INTR_MASK_8_REG                  ,MASK_BIT1                  ,DATA_HEX},
    [PORT_10_TX_FAULT_MASK]             =   {PORT_INTR_MASK_8_REG                  ,MASK_BIT2                  ,DATA_HEX},
    [PORT_11_TX_FAULT_MASK]             =   {PORT_INTR_MASK_8_REG                  ,MASK_BIT3                  ,DATA_HEX},
    [PORT_12_TX_FAULT_MASK]             =   {PORT_INTR_MASK_8_REG                  ,MASK_BIT4                  ,DATA_HEX},
    [PORT_13_TX_FAULT_MASK]             =   {PORT_INTR_MASK_8_REG                  ,MASK_BIT5                  ,DATA_HEX},
    [PORT_14_TX_FAULT_MASK]             =   {PORT_INTR_MASK_8_REG                  ,MASK_BIT6                  ,DATA_HEX},
    [PORT_15_TX_FAULT_MASK]             =   {PORT_INTR_MASK_8_REG                  ,MASK_BIT7                  ,DATA_HEX},
    [PORT_16_TX_FAULT_MASK]             =   {PORT_INTR_MASK_9_REG                  ,MASK_BIT0                  ,DATA_HEX},
    [PORT_17_TX_FAULT_MASK]             =   {PORT_INTR_MASK_9_REG                  ,MASK_BIT1                  ,DATA_HEX},
    [PORT_18_TX_FAULT_MASK]             =   {PORT_INTR_MASK_9_REG                  ,MASK_BIT2                  ,DATA_HEX},
    [PORT_19_TX_FAULT_MASK]             =   {PORT_INTR_MASK_9_REG                  ,MASK_BIT3                  ,DATA_HEX},
    [PORT_0_7_TX_FAULT_EVENT]           =   {PORT_INTR_EVENT_7_REG                 ,MASK_ALL                   ,DATA_HEX},
    [PORT_8_15_TX_FAULT_EVENT]          =   {PORT_INTR_EVENT_8_REG                 ,MASK_ALL                   ,DATA_HEX},
    [PORT_16_19_TX_FAULT_EVENT]         =   {PORT_INTR_EVENT_9_REG                 ,MASK_ALL                   ,DATA_HEX},
    [PORT_0_TX_FAULT_DEBUG]             =   {PORT_INTR_DEBUG_7_REG                 ,MASK_BIT0                  ,DATA_HEX},
    [PORT_1_TX_FAULT_DEBUG]             =   {PORT_INTR_DEBUG_7_REG                 ,MASK_BIT1                  ,DATA_HEX},
    [PORT_2_TX_FAULT_DEBUG]             =   {PORT_INTR_DEBUG_7_REG                 ,MASK_BIT2                  ,DATA_HEX},
    [PORT_3_TX_FAULT_DEBUG]             =   {PORT_INTR_DEBUG_7_REG                 ,MASK_BIT3                  ,DATA_HEX},
    [PORT_4_TX_FAULT_DEBUG]             =   {PORT_INTR_DEBUG_7_REG                 ,MASK_BIT4                  ,DATA_HEX},
    [PORT_5_TX_FAULT_DEBUG]             =   {PORT_INTR_DEBUG_7_REG                 ,MASK_BIT5                  ,DATA_HEX},
    [PORT_6_TX_FAULT_DEBUG]             =   {PORT_INTR_DEBUG_7_REG                 ,MASK_BIT6                  ,DATA_HEX},
    [PORT_7_TX_FAULT_DEBUG]             =   {PORT_INTR_DEBUG_7_REG                 ,MASK_BIT7                  ,DATA_HEX},
    [PORT_8_TX_FAULT_DEBUG]             =   {PORT_INTR_DEBUG_8_REG                 ,MASK_BIT0                  ,DATA_HEX},
    [PORT_9_TX_FAULT_DEBUG]             =   {PORT_INTR_DEBUG_8_REG                 ,MASK_BIT1                  ,DATA_HEX},
    [PORT_10_TX_FAULT_DEBUG]            =   {PORT_INTR_DEBUG_8_REG                 ,MASK_BIT2                  ,DATA_HEX},
    [PORT_11_TX_FAULT_DEBUG]            =   {PORT_INTR_DEBUG_8_REG                 ,MASK_BIT3                  ,DATA_HEX},
    [PORT_12_TX_FAULT_DEBUG]            =   {PORT_INTR_DEBUG_8_REG                 ,MASK_BIT4                  ,DATA_HEX},
    [PORT_13_TX_FAULT_DEBUG]            =   {PORT_INTR_DEBUG_8_REG                 ,MASK_BIT5                  ,DATA_HEX},
    [PORT_14_TX_FAULT_DEBUG]            =   {PORT_INTR_DEBUG_8_REG                 ,MASK_BIT6                  ,DATA_HEX},
    [PORT_15_TX_FAULT_DEBUG]            =   {PORT_INTR_DEBUG_8_REG                 ,MASK_BIT7                  ,DATA_HEX},
    [PORT_16_TX_FAULT_DEBUG]            =   {PORT_INTR_DEBUG_9_REG                 ,MASK_BIT0                  ,DATA_HEX},
    [PORT_17_TX_FAULT_DEBUG]            =   {PORT_INTR_DEBUG_9_REG                 ,MASK_BIT1                  ,DATA_HEX},
    [PORT_18_TX_FAULT_DEBUG]            =   {PORT_INTR_DEBUG_9_REG                 ,MASK_BIT2                  ,DATA_HEX},
    [PORT_19_TX_FAULT_DEBUG]            =   {PORT_INTR_DEBUG_9_REG                 ,MASK_BIT3                  ,DATA_HEX},
    [PORT_0_TX_DISABLE]                 =   {PORT_CONTROL_1_REG                    ,MASK_BIT0                  ,DATA_HEX},
    [PORT_1_TX_DISABLE]                 =   {PORT_CONTROL_1_REG                    ,MASK_BIT1                  ,DATA_HEX},
    [PORT_2_TX_DISABLE]                 =   {PORT_CONTROL_1_REG                    ,MASK_BIT2                  ,DATA_HEX},
    [PORT_3_TX_DISABLE]                 =   {PORT_CONTROL_1_REG                    ,MASK_BIT3                  ,DATA_HEX},
    [PORT_4_TX_DISABLE]                 =   {PORT_CONTROL_1_REG                    ,MASK_BIT4                  ,DATA_HEX},
    [PORT_5_TX_DISABLE]                 =   {PORT_CONTROL_1_REG                    ,MASK_BIT5                  ,DATA_HEX},
    [PORT_6_TX_DISABLE]                 =   {PORT_CONTROL_1_REG                    ,MASK_BIT6                  ,DATA_HEX},
    [PORT_7_TX_DISABLE]                 =   {PORT_CONTROL_1_REG                    ,MASK_BIT7                  ,DATA_HEX},
    [PORT_8_TX_DISABLE]                 =   {PORT_CONTROL_2_REG                    ,MASK_BIT0                  ,DATA_HEX},
    [PORT_9_TX_DISABLE]                 =   {PORT_CONTROL_2_REG                    ,MASK_BIT1                  ,DATA_HEX},
    [PORT_10_TX_DISABLE]                =   {PORT_CONTROL_2_REG                    ,MASK_BIT2                  ,DATA_HEX},
    [PORT_11_TX_DISABLE]                =   {PORT_CONTROL_2_REG                    ,MASK_BIT3                  ,DATA_HEX},
    [PORT_12_TX_DISABLE]                =   {PORT_CONTROL_2_REG                    ,MASK_BIT4                  ,DATA_HEX},
    [PORT_13_TX_DISABLE]                =   {PORT_CONTROL_2_REG                    ,MASK_BIT5                  ,DATA_HEX},
    [PORT_14_TX_DISABLE]                =   {PORT_CONTROL_2_REG                    ,MASK_BIT6                  ,DATA_HEX},
    [PORT_15_TX_DISABLE]                =   {PORT_CONTROL_2_REG                    ,MASK_BIT7                  ,DATA_HEX},
    [PORT_16_TX_DISABLE]                =   {PORT_CONTROL_3_REG                    ,MASK_BIT0                  ,DATA_HEX},
    [PORT_17_TX_DISABLE]                =   {PORT_CONTROL_3_REG                    ,MASK_BIT1                  ,DATA_HEX},
    [PORT_18_TX_DISABLE]                =   {PORT_CONTROL_3_REG                    ,MASK_BIT2                  ,DATA_HEX},
    [PORT_19_TX_DISABLE]                =   {PORT_CONTROL_3_REG                    ,MASK_BIT3                  ,DATA_HEX},
    [PORT_0_RATE_SEL]                   =   {PORT_CONTROL_4_REG                    ,MASK_BIT0                  ,DATA_HEX},
    [PORT_1_RATE_SEL]                   =   {PORT_CONTROL_4_REG                    ,MASK_BIT1                  ,DATA_HEX},
    [PORT_2_RATE_SEL]                   =   {PORT_CONTROL_4_REG                    ,MASK_BIT2                  ,DATA_HEX},
    [PORT_3_RATE_SEL]                   =   {PORT_CONTROL_4_REG                    ,MASK_BIT3                  ,DATA_HEX},
    [PORT_4_RATE_SEL]                   =   {PORT_CONTROL_4_REG                    ,MASK_BIT4                  ,DATA_HEX},
    [PORT_5_RATE_SEL]                   =   {PORT_CONTROL_4_REG                    ,MASK_BIT5                  ,DATA_HEX},
    [PORT_6_RATE_SEL]                   =   {PORT_CONTROL_4_REG                    ,MASK_BIT6                  ,DATA_HEX},
    [PORT_7_RATE_SEL]                   =   {PORT_CONTROL_4_REG                    ,MASK_BIT7                  ,DATA_HEX},
    [PORT_8_RATE_SEL]                   =   {PORT_CONTROL_5_REG                    ,MASK_BIT0                  ,DATA_HEX},
    [PORT_9_RATE_SEL]                   =   {PORT_CONTROL_5_REG                    ,MASK_BIT1                  ,DATA_HEX},
    [PORT_10_RATE_SEL]                  =   {PORT_CONTROL_5_REG                    ,MASK_BIT2                  ,DATA_HEX},
    [PORT_11_RATE_SEL]                  =   {PORT_CONTROL_5_REG                    ,MASK_BIT3                  ,DATA_HEX},
    [PORT_12_RATE_SEL]                  =   {PORT_CONTROL_5_REG                    ,MASK_BIT4                  ,DATA_HEX},
    [PORT_13_RATE_SEL]                  =   {PORT_CONTROL_5_REG                    ,MASK_BIT5                  ,DATA_HEX},
    [PORT_14_RATE_SEL]                  =   {PORT_CONTROL_5_REG                    ,MASK_BIT6                  ,DATA_HEX},
    [PORT_15_RATE_SEL]                  =   {PORT_CONTROL_5_REG                    ,MASK_BIT7                  ,DATA_HEX},
    [PORT_16_RATE_SEL]                  =   {PORT_CONTROL_6_REG                    ,MASK_BIT0                  ,DATA_HEX},
    [PORT_17_RATE_SEL]                  =   {PORT_CONTROL_6_REG                    ,MASK_BIT1                  ,DATA_HEX},
    [PORT_18_RATE_SEL]                  =   {PORT_CONTROL_6_REG                    ,MASK_BIT2                  ,DATA_HEX},
    [PORT_19_RATE_SEL]                  =   {PORT_CONTROL_6_REG                    ,MASK_BIT3                  ,DATA_HEX},
    [PORT_0_PWR_EN]                     =   {PORT_CONTROL_7_REG                    ,MASK_BIT0                  ,DATA_HEX},
    [PORT_1_PWR_EN]                     =   {PORT_CONTROL_7_REG                    ,MASK_BIT1                  ,DATA_HEX},
    [PORT_2_PWR_EN]                     =   {PORT_CONTROL_7_REG                    ,MASK_BIT2                  ,DATA_HEX},
    [PORT_3_PWR_EN]                     =   {PORT_CONTROL_7_REG                    ,MASK_BIT3                  ,DATA_HEX},
    [PORT_4_PWR_EN]                     =   {PORT_CONTROL_7_REG                    ,MASK_BIT4                  ,DATA_HEX},
    [PORT_5_PWR_EN]                     =   {PORT_CONTROL_7_REG                    ,MASK_BIT5                  ,DATA_HEX},
    [PORT_6_PWR_EN]                     =   {PORT_CONTROL_7_REG                    ,MASK_BIT6                  ,DATA_HEX},
    [PORT_7_PWR_EN]                     =   {PORT_CONTROL_7_REG                    ,MASK_BIT7                  ,DATA_HEX},
    [PORT_8_PWR_EN]                     =   {PORT_CONTROL_8_REG                    ,MASK_BIT0                  ,DATA_HEX},
    [PORT_9_PWR_EN]                     =   {PORT_CONTROL_8_REG                    ,MASK_BIT1                  ,DATA_HEX},
    [PORT_10_PWR_EN]                    =   {PORT_CONTROL_8_REG                    ,MASK_BIT2                  ,DATA_HEX},
    [PORT_11_PWR_EN]                    =   {PORT_CONTROL_8_REG                    ,MASK_BIT3                  ,DATA_HEX},
    [PORT_12_PWR_EN]                    =   {PORT_CONTROL_8_REG                    ,MASK_BIT4                  ,DATA_HEX},
    [PORT_13_PWR_EN]                    =   {PORT_CONTROL_8_REG                    ,MASK_BIT5                  ,DATA_HEX},
    [PORT_14_PWR_EN]                    =   {PORT_CONTROL_8_REG                    ,MASK_BIT6                  ,DATA_HEX},
    [PORT_15_PWR_EN]                    =   {PORT_CONTROL_8_REG                    ,MASK_BIT7                  ,DATA_HEX},
    [PORT_16_PWR_EN]                    =   {PORT_CONTROL_9_REG                    ,MASK_BIT0                  ,DATA_HEX},
    [PORT_17_PWR_EN]                    =   {PORT_CONTROL_9_REG                    ,MASK_BIT1                  ,DATA_HEX},
    [PORT_18_PWR_EN]                    =   {PORT_CONTROL_9_REG                    ,MASK_BIT2                  ,DATA_HEX},
    [PORT_19_PWR_EN]                    =   {PORT_CONTROL_9_REG                    ,MASK_BIT3                  ,DATA_HEX},
    //[PSU_0_VIN_PWOK]                    =   {SYS_STATUS_1_REG                     ,MASK_BIT2                  ,DATA_HEX},
    //[PSU_1_VIN_PWOK]                    =   {SYS_STATUS_1_REG                     ,MASK_BIT3                  ,DATA_HEX},
    //[PSU_0_VOUT_PWOK]                   =   {SYS_STATUS_1_REG                     ,MASK_BIT4                  ,DATA_HEX},
    //[PSU_1_VOUT_PWOK]                   =   {SYS_STATUS_1_REG                     ,MASK_BIT5                  ,DATA_HEX},
    [INTR_FAN_0]                        =   {SYS_STATUS_2_REG                      ,MASK_BIT0                  ,DATA_HEX},
    [INTR_FAN_1]                        =   {SYS_STATUS_2_REG                      ,MASK_BIT1                  ,DATA_HEX},
    [INTR_FAN_2]                        =   {SYS_STATUS_2_REG                      ,MASK_BIT2                  ,DATA_HEX},
    [INTR_FAN_3]                        =   {SYS_STATUS_2_REG                      ,MASK_BIT3                  ,DATA_HEX},
    [FAN_0_PWM_RPM_H]                   =   {SYS_STATUS_3_REG                      ,MASK_ALL                   ,DATA_HEX},
    [FAN_0_PWM_RPM_L]                   =   {SYS_STATUS_4_REG                      ,MASK_ALL                   ,DATA_HEX},
    [FAN_0_PWM_RPM]                     =   {NONE_REG                              ,MASK_NONE                  ,DATA_HEX},
    [FAN_1_PWM_RPM_H]                   =   {SYS_STATUS_5_REG                      ,MASK_ALL                   ,DATA_HEX},
    [FAN_1_PWM_RPM_L]                   =   {SYS_STATUS_6_REG                      ,MASK_ALL                   ,DATA_HEX},
    [FAN_1_PWM_RPM]                     =   {NONE_REG                              ,MASK_NONE                  ,DATA_HEX},
    [FAN_2_PWM_RPM_H]                   =   {SYS_STATUS_7_REG                      ,MASK_ALL                   ,DATA_HEX},
    [FAN_2_PWM_RPM_L]                   =   {SYS_STATUS_8_REG                      ,MASK_ALL                   ,DATA_HEX},
    [FAN_2_PWM_RPM]                     =   {NONE_REG                              ,MASK_NONE                  ,DATA_HEX},
    [FAN_3_PWM_RPM_H]                   =   {SYS_STATUS_9_REG                      ,MASK_ALL                   ,DATA_HEX},
    [FAN_3_PWM_RPM_L]                   =   {SYS_STATUS_10_REG                     ,MASK_ALL                   ,DATA_HEX},
    [FAN_3_PWM_RPM]                     =   {NONE_REG                              ,MASK_NONE                  ,DATA_HEX},
    [FAN_PWM_MODE_1]                    =   {SYS_CTRL_1_FAN_PWM_MODE               ,MASK_BIT7                  ,DATA_HEX},
    [FAN_PWM_MODE_2]                    =   {SYS_CTRL_1_FAN_PWM_MODE               ,MASK_BIT6                  ,DATA_HEX},
    [FAN_PWM_DIAG_CTRL_1]               =   {SYS_CTRL_2_FAN_PWM_DIAG_MODE          ,MASK_FAN_PWM_DIAG_MODE     ,DATA_HEX},
    [FAN_PWM_DIAG_CTRL_2]               =   {SYS_CTRL_3_FAN_PWM_DIAG_MODE          ,MASK_FAN_PWM_DIAG_MODE     ,DATA_HEX},
    [VOL_1_VALUE]                       =   {VOLTAGE_1_CH1                         ,MASK_ALL                   ,DATA_HEX},
    [VOL_2_VALUE]                       =   {VOLTAGE_2_CH2                         ,MASK_ALL                   ,DATA_HEX},
    [VOL_3_VALUE]                       =   {VOLTAGE_3_CH3                         ,MASK_ALL                   ,DATA_HEX},
    [VOL_4_VALUE]                       =   {VOLTAGE_4_CH4                         ,MASK_ALL                   ,DATA_HEX},
    [VOL_5_VALUE]                       =   {VOLTAGE_5_CH5                         ,MASK_ALL                   ,DATA_HEX},
    [VOL_6_VALUE]                       =   {VOLTAGE_6_CH6                         ,MASK_ALL                   ,DATA_HEX},
    [VOL_7_VALUE]                       =   {VOLTAGE_7_CH7                         ,MASK_ALL                   ,DATA_HEX},
    [VOL_8_VALUE]                       =   {VOLTAGE_8_CH8                         ,MASK_ALL                   ,DATA_HEX},
    [VOL_9_VALUE]                       =   {VOLTAGE_9_CH9                         ,MASK_ALL                   ,DATA_HEX},
    [VOL_10_VALUE]                      =   {VOLTAGE_10_CH10                       ,MASK_ALL                   ,DATA_HEX},
    [VOL_11_VALUE]                      =   {VOLTAGE_11_CH11                       ,MASK_ALL                   ,DATA_HEX},
    [VOL_12_VALUE]                      =   {VOLTAGE_12_CH12                       ,MASK_ALL                   ,DATA_HEX},
    [VOL_13_VALUE]                      =   {VOLTAGE_13_CH13                       ,MASK_ALL                   ,DATA_HEX},
    [VOL_14_VALUE]                      =   {VOLTAGE_14_CH14                       ,MASK_ALL                   ,DATA_HEX},
    [VOL_15_VALUE]                      =   {VOLTAGE_15_CH15                       ,MASK_ALL                   ,DATA_HEX},
    [VOL_16_VALUE]                      =   {VOLTAGE_16_CH16                       ,MASK_ALL                   ,DATA_HEX},
    [BSP_DEBUG]                         =   {NONE_REG                              ,MASK_NONE                  ,DATA_UNK},
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
static int _cpld_reg_read(struct device *dev, u8 reg, u8 mask);
int _cpld_reg_read_nolock(struct device *dev, u8 reg, u8 mask);
int _cpld_reg_write_nolock(struct device *dev, u8 reg, u8 reg_val);
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
static ssize_t fan_0_pwm_rpm_show(struct device *dev,
                    struct device_attribute *da,
                    char *buf);
static ssize_t fan_1_pwm_rpm_show(struct device *dev,
                    struct device_attribute *da,
                    char *buf);
static ssize_t fan_2_pwm_rpm_show(struct device *dev,
                    struct device_attribute *da,
                    char *buf);
static ssize_t fan_3_pwm_rpm_show(struct device *dev,
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
    int psu_type;               /* PSU Type from CPLD1 */
    struct mutex access_lock;   /* mutex for cpld access */
    u8 access_reg;              /* register to access */
};

struct lpc_data_s
{
    struct mutex    access_lock;
};

struct lpc_data_s *lpc_data;

/* CPLD device id and data */
static const struct i2c_device_id cpld_id[] = {
    { "s9511_20ct_cpld1",  cpld1 },
    { "s9511_20ct_cpld2",  cpld2 },
    {}
};

char bsp_debug[2]="0";
u8 enable_log_read=LOG_DISABLE;
u8 enable_log_write=LOG_DISABLE;

/* Addresses scanned for cpld */
static const unsigned short cpld_i2c_addr[] = { 0x33, 0x27, I2C_CLIENT_END };

/* define all support register access of cpld in attribute */
/********************************************** CPLD 1&2 common registers *********************************************/
static SENSOR_DEVICE_ATTR_RO(cpld_major_ver                     ,cpld               ,CPLD_MAJOR_VER                   );
static SENSOR_DEVICE_ATTR_RO(cpld_minor_ver                     ,cpld               ,CPLD_MINOR_VER                   );
static SENSOR_DEVICE_ATTR_RO(cpld_version_h                     ,cpld_version_h     ,CPLD_VERSION_H                   );
static SENSOR_DEVICE_ATTR_RO(cpld_id                            ,cpld               ,CPLD_ID                          );
static SENSOR_DEVICE_ATTR_RO(cpld_build_ver                     ,cpld               ,CPLD_BUILD_VER                   );
static SENSOR_DEVICE_ATTR_RO(cpld_chip_type                     ,cpld               ,CPLD_CHIP_TYPE                   );
static SENSOR_DEVICE_ATTR_RW(cpld_i2c_upgrade_module_reset      ,cpld               ,CPLD_I2C_UPGRADE_MODULE_RESET    );
static SENSOR_DEVICE_ATTR_RW(event_ctrl                         ,cpld               ,EVENT_CTRL                       );
/******************************************************* CPLD 1 *******************************************************/
static SENSOR_DEVICE_ATTR_RO(sku_id                             ,cpld               ,SKU_ID                           );
static SENSOR_DEVICE_ATTR_RO(hw_rev                             ,cpld               ,HW_REV                           );
static SENSOR_DEVICE_ATTR_RO(deph_id                            ,cpld               ,DEPH_ID                          );
static SENSOR_DEVICE_ATTR_RO(build_id                           ,cpld               ,BUILD_ID                         );
static SENSOR_DEVICE_ATTR_RO(bit_sel_id                         ,cpld               ,BIT_SEL_ID                       );
static SENSOR_DEVICE_ATTR_RO(ext_id                             ,cpld               ,EXT_ID                           );
static SENSOR_DEVICE_ATTR_RO(extend_id                          ,cpld               ,EXTEND_ID                        );
static SENSOR_DEVICE_ATTR_RO(intr_0                             ,cpld               ,INTR_0                           );
static SENSOR_DEVICE_ATTR_RO(intr_1                             ,cpld               ,INTR_1                           );
static SENSOR_DEVICE_ATTR_RO(intr_2                             ,cpld               ,INTR_2                           );
static SENSOR_DEVICE_ATTR_RO(intr_3                             ,cpld               ,INTR_3                           );
static SENSOR_DEVICE_ATTR_RO(intr_4                             ,cpld               ,INTR_4                           );
static SENSOR_DEVICE_ATTR_RO(intr_5                             ,cpld               ,INTR_5                           );
static SENSOR_DEVICE_ATTR_RO(intr_6                             ,cpld               ,INTR_6                           );
static SENSOR_DEVICE_ATTR_RO(intr_7                             ,cpld               ,INTR_7                           );
static SENSOR_DEVICE_ATTR_RO(intr_8                             ,cpld               ,INTR_8                           );
static SENSOR_DEVICE_ATTR_RO(intr_9                             ,cpld               ,INTR_9                           );
static SENSOR_DEVICE_ATTR_RO(intr_10                            ,cpld               ,INTR_10                          );
static SENSOR_DEVICE_ATTR_RO(intr_11                            ,cpld               ,INTR_11                          );
static SENSOR_DEVICE_ATTR_RO(intr_12                            ,cpld               ,INTR_12                          );
static SENSOR_DEVICE_ATTR_RO(intr_13                            ,cpld               ,INTR_13                          );
static SENSOR_DEVICE_ATTR_RO(intr_14                            ,cpld               ,INTR_14                          );
static SENSOR_DEVICE_ATTR_RO(intr_15                            ,cpld               ,INTR_15                          );
static SENSOR_DEVICE_ATTR_RO(intr_16                            ,cpld               ,INTR_16                          );
static SENSOR_DEVICE_ATTR_RO(intr_17                            ,cpld               ,INTR_17                          );
static SENSOR_DEVICE_ATTR_RO(intr_18                            ,cpld               ,INTR_18                          );
static SENSOR_DEVICE_ATTR_RO(intr_19                            ,cpld               ,INTR_19                          );
static SENSOR_DEVICE_ATTR_RO(intr_20                            ,cpld               ,INTR_20                          );
static SENSOR_DEVICE_ATTR_RO(intr_21                            ,cpld               ,INTR_21                          );
static SENSOR_DEVICE_ATTR_RO(intr_22                            ,cpld               ,INTR_22                          );
static SENSOR_DEVICE_ATTR_RO(intr_23                            ,cpld               ,INTR_23                          );
static SENSOR_DEVICE_ATTR_RO(intr_24                            ,cpld               ,INTR_24                          );
static SENSOR_DEVICE_ATTR_RO(intr_25                            ,cpld               ,INTR_25                          );
static SENSOR_DEVICE_ATTR_RO(intr_26                            ,cpld               ,INTR_26                          );
static SENSOR_DEVICE_ATTR_RO(intr_27                            ,cpld               ,INTR_27                          );
static SENSOR_DEVICE_ATTR_RW(intr_mask_0                        ,cpld               ,INTR_MASK_0                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_1                        ,cpld               ,INTR_MASK_1                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_2                        ,cpld               ,INTR_MASK_2                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_3                        ,cpld               ,INTR_MASK_3                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_4                        ,cpld               ,INTR_MASK_4                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_5                        ,cpld               ,INTR_MASK_5                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_6                        ,cpld               ,INTR_MASK_6                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_7                        ,cpld               ,INTR_MASK_7                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_8                        ,cpld               ,INTR_MASK_8                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_9                        ,cpld               ,INTR_MASK_9                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_10                       ,cpld               ,INTR_MASK_10                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_11                       ,cpld               ,INTR_MASK_11                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_12                       ,cpld               ,INTR_MASK_12                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_13                       ,cpld               ,INTR_MASK_13                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_14                       ,cpld               ,INTR_MASK_14                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_15                       ,cpld               ,INTR_MASK_15                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_16                       ,cpld               ,INTR_MASK_16                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_17                       ,cpld               ,INTR_MASK_17                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_18                       ,cpld               ,INTR_MASK_18                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_19                       ,cpld               ,INTR_MASK_19                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_20                       ,cpld               ,INTR_MASK_20                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_21                       ,cpld               ,INTR_MASK_21                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_22                       ,cpld               ,INTR_MASK_22                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_23                       ,cpld               ,INTR_MASK_23                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_24                       ,cpld               ,INTR_MASK_24                     );
static SENSOR_DEVICE_ATTR_RW(intr_mask_25                       ,cpld               ,INTR_MASK_25                     );
static SENSOR_DEVICE_ATTR_RO(intr_event_clk_ptp                 ,cpld               ,INTR_EVENT_CLK_PTP               );
static SENSOR_DEVICE_ATTR_RO(intr_event_psu                     ,cpld               ,INTR_EVENT_PSU                   );
static SENSOR_DEVICE_ATTR_RO(intr_event_hwm                     ,cpld               ,INTR_EVENT_HWM                   );
static SENSOR_DEVICE_ATTR_RO(intr_event_thermal                 ,cpld               ,INTR_EVENT_THERMAL               );
static SENSOR_DEVICE_ATTR_RO(intr_event_fan                     ,cpld               ,INTR_EVENT_FAN                   );
static SENSOR_DEVICE_ATTR_RO(intr_event_eth                     ,cpld               ,INTR_EVENT_ETH                   );
static SENSOR_DEVICE_ATTR_RW(intr_debug_1                       ,cpld               ,INTR_DEBUG_1                     );
static SENSOR_DEVICE_ATTR_RW(intr_debug_2                       ,cpld               ,INTR_DEBUG_2                     );
static SENSOR_DEVICE_ATTR_RW(intr_debug_3                       ,cpld               ,INTR_DEBUG_3                     );
static SENSOR_DEVICE_ATTR_RW(intr_debug_4                       ,cpld               ,INTR_DEBUG_4                     );
static SENSOR_DEVICE_ATTR_RW(intr_debug_5                       ,cpld               ,INTR_DEBUG_5                     );
static SENSOR_DEVICE_ATTR_RW(intr_debug_6                       ,cpld               ,INTR_DEBUG_6                     );
static SENSOR_DEVICE_ATTR_RW(intr_debug_7                       ,cpld               ,INTR_DEBUG_7                     );
static SENSOR_DEVICE_ATTR_RW(intr_debug_8                       ,cpld               ,INTR_DEBUG_8                     );
static SENSOR_DEVICE_ATTR_RW(intr_debug_9                       ,cpld               ,INTR_DEBUG_9                     );
static SENSOR_DEVICE_ATTR_RW(intr_debug_10                      ,cpld               ,INTR_DEBUG_10                    );
static SENSOR_DEVICE_ATTR_RW(intr_debug_11                      ,cpld               ,INTR_DEBUG_11                    );
static SENSOR_DEVICE_ATTR_RW(intr_debug_12                      ,cpld               ,INTR_DEBUG_12                    );
static SENSOR_DEVICE_ATTR_RW(intr_debug_13                      ,cpld               ,INTR_DEBUG_13                    );
static SENSOR_DEVICE_ATTR_RW(intr_debug_14                      ,cpld               ,INTR_DEBUG_14                    );
static SENSOR_DEVICE_ATTR_RW(intr_debug_15                      ,cpld               ,INTR_DEBUG_15                    );
static SENSOR_DEVICE_ATTR_RW(intr_debug_16                      ,cpld               ,INTR_DEBUG_16                    );
static SENSOR_DEVICE_ATTR_RW(intr_debug_17                      ,cpld               ,INTR_DEBUG_17                    );
static SENSOR_DEVICE_ATTR_RW(intr_debug_18                      ,cpld               ,INTR_DEBUG_18                    );
static SENSOR_DEVICE_ATTR_RW(intr_debug_19                      ,cpld               ,INTR_DEBUG_19                    );
static SENSOR_DEVICE_ATTR_RW(intr_debug_20                      ,cpld               ,INTR_DEBUG_20                    );
static SENSOR_DEVICE_ATTR_RW(reset_0                            ,cpld               ,RESET_0                          );
static SENSOR_DEVICE_ATTR_RW(reset_1                            ,cpld               ,RESET_1                          );
static SENSOR_DEVICE_ATTR_RW(reset_2                            ,cpld               ,RESET_2                          );
static SENSOR_DEVICE_ATTR_RW(reset_3                            ,cpld               ,RESET_3                          );
static SENSOR_DEVICE_ATTR_RW(reset_4                            ,cpld               ,RESET_4                          );
static SENSOR_DEVICE_ATTR_RW(reset_5                            ,cpld               ,RESET_5                          );
static SENSOR_DEVICE_ATTR_RW(reset_6                            ,cpld               ,RESET_6                          );
static SENSOR_DEVICE_ATTR_RW(reset_7                            ,cpld               ,RESET_7                          );
static SENSOR_DEVICE_ATTR_RW(reset_8                            ,cpld               ,RESET_8                          );
static SENSOR_DEVICE_ATTR_RW(reset_9                            ,cpld               ,RESET_9                          );
static SENSOR_DEVICE_ATTR_RW(reset_10                           ,cpld               ,RESET_10                         );
static SENSOR_DEVICE_ATTR_RW(reset_11                           ,cpld               ,RESET_11                         );
static SENSOR_DEVICE_ATTR_RW(reset_12                           ,cpld               ,RESET_12                         );
static SENSOR_DEVICE_ATTR_RW(reset_13                           ,cpld               ,RESET_13                         );
static SENSOR_DEVICE_ATTR_RW(reset_14                           ,cpld               ,RESET_14                         );
static SENSOR_DEVICE_ATTR_RW(cpld_rov_reset                     ,cpld               ,CPLD_ROV_RESET                   );
static SENSOR_DEVICE_ATTR_RW(rov_debug_enable                   ,cpld               ,ROV_DEBUG_ENABLE                 );
static SENSOR_DEVICE_ATTR_RW(rov_debug_1                        ,cpld               ,ROV_DEBUG_1                      );
static SENSOR_DEVICE_ATTR_RW(rov_debug_2                        ,cpld               ,ROV_DEBUG_2                      );
static SENSOR_DEVICE_ATTR_RO(sys_status_1                       ,cpld               ,SYS_STATUS_1                     );
static SENSOR_DEVICE_ATTR_RO(sys_status_2                       ,cpld               ,SYS_STATUS_2                     );
static SENSOR_DEVICE_ATTR_RO(sys_status_3                       ,cpld               ,SYS_STATUS_3                     );
static SENSOR_DEVICE_ATTR_RO(sys_status_4                       ,cpld               ,SYS_STATUS_4                     );
static SENSOR_DEVICE_ATTR_RO(sys_status_5                       ,cpld               ,SYS_STATUS_5                     );
static SENSOR_DEVICE_ATTR_RO(sys_status_6                       ,cpld               ,SYS_STATUS_6                     );
static SENSOR_DEVICE_ATTR_RO(sys_status_7_mac_rov1              ,cpld               ,SYS_STATUS_7_MAC_ROV1            );
static SENSOR_DEVICE_ATTR_RO(sys_status_8_mac_rov2              ,cpld               ,SYS_STATUS_8_MAC_ROV2            );
static SENSOR_DEVICE_ATTR_RO(sys_status_9_mac_rov3              ,cpld               ,SYS_STATUS_9_MAC_ROV3            );
static SENSOR_DEVICE_ATTR_RO(sys_status_10                      ,cpld               ,SYS_STATUS_10                    );
static SENSOR_DEVICE_ATTR_RO(sys_status_11                      ,cpld               ,SYS_STATUS_11                    );
static SENSOR_DEVICE_ATTR_RO(sys_status_12                      ,cpld               ,SYS_STATUS_12                    );
static SENSOR_DEVICE_ATTR_RO(sys_status_13                      ,cpld               ,SYS_STATUS_13                    );
static SENSOR_DEVICE_ATTR_RO(sys_status_14                      ,cpld               ,SYS_STATUS_14                    );
static SENSOR_DEVICE_ATTR_RO(sys_status_15                      ,cpld               ,SYS_STATUS_15                    );
static SENSOR_DEVICE_ATTR_RO(sys_status_16                      ,cpld               ,SYS_STATUS_16                    );
static SENSOR_DEVICE_ATTR_RO(sys_status_17                      ,cpld               ,SYS_STATUS_17                    );
static SENSOR_DEVICE_ATTR_RO(sys_status_18                      ,cpld               ,SYS_STATUS_18                    );
static SENSOR_DEVICE_ATTR_RO(sys_status_19                      ,cpld               ,SYS_STATUS_19                    );
static SENSOR_DEVICE_ATTR_RO(sys_status_20                      ,cpld               ,SYS_STATUS_20                    );
static SENSOR_DEVICE_ATTR_RO(sys_status_21                      ,cpld               ,SYS_STATUS_21                    );
static SENSOR_DEVICE_ATTR_RO(sys_status_22                      ,cpld               ,SYS_STATUS_22                    );
static SENSOR_DEVICE_ATTR_RO(sys_status_23                      ,cpld               ,SYS_STATUS_23                    );
static SENSOR_DEVICE_ATTR_RO(sys_status_24                      ,cpld               ,SYS_STATUS_24                    );
static SENSOR_DEVICE_ATTR_RO(sys_status_25                      ,cpld               ,SYS_STATUS_25                    );
static SENSOR_DEVICE_ATTR_RO(sys_status_26                      ,cpld               ,SYS_STATUS_26                    );
static SENSOR_DEVICE_ATTR_RO(sys_status_27                      ,cpld               ,SYS_STATUS_27                    );
static SENSOR_DEVICE_ATTR_RO(sys_status_28                      ,cpld               ,SYS_STATUS_28                    );
static SENSOR_DEVICE_ATTR_RO(sys_status_29                      ,cpld               ,SYS_STATUS_29                    );
static SENSOR_DEVICE_ATTR_RO(psu0_present                       ,cpld               ,PSU0_PRESENT                     );
static SENSOR_DEVICE_ATTR_RO(psu1_present                       ,cpld               ,PSU1_PRESENT                     );
static SENSOR_DEVICE_ATTR_RO(psu0_vin_pwok                      ,cpld               ,PSU0_VIN_PWOK                    );
static SENSOR_DEVICE_ATTR_RO(psu1_vin_pwok                      ,cpld               ,PSU1_VIN_PWOK                    );
static SENSOR_DEVICE_ATTR_RO(psu0_pwok                          ,cpld               ,PSU0_VOUT_PWOK                   );
static SENSOR_DEVICE_ATTR_RO(psu1_pwok                          ,cpld               ,PSU1_VOUT_PWOK                   );
static SENSOR_DEVICE_ATTR_RO(psu_type                           ,cpld               ,PSU_TYPE                         );
static SENSOR_DEVICE_ATTR_RW(boot_select                        ,cpld               ,BOOT_SELECT                      );
static SENSOR_DEVICE_ATTR_RW(clk_timing_ctrl_1                  ,cpld               ,CLK_TIMING_CTRL_1                );
static SENSOR_DEVICE_ATTR_RW(clk_timing_ctrl_2                  ,cpld               ,CLK_TIMING_CTRL_2                );
static SENSOR_DEVICE_ATTR_RW(pw_sys_ctrl                        ,cpld               ,PW_SYS_CTRL                      );
static SENSOR_DEVICE_ATTR_RW(gnss_ctrl                          ,cpld               ,GNSS_CTRL                        );
static SENSOR_DEVICE_ATTR_RW(usb_ctrl                           ,cpld               ,USB_CTRL                         );
static SENSOR_DEVICE_ATTR_RW(synce_ctrl                         ,cpld               ,SYNCE_CTRL                       );
static SENSOR_DEVICE_ATTR_RW(ts_pll_clock_ctrl                  ,cpld               ,TS_PLL_CLOCK_CTRL                );
static SENSOR_DEVICE_ATTR_RW(led_clear                          ,cpld               ,LED_CLEAR                        );
static SENSOR_DEVICE_ATTR_RW(sys_led_status                     ,cpld               ,SYS_LED_STATUS                   );
static SENSOR_DEVICE_ATTR_RW(sys_led_blinking                   ,cpld               ,SYS_LED_BLINKING                 );
static SENSOR_DEVICE_ATTR_RW(sys_led_color                      ,cpld               ,SYS_LED_COLOR                    );
static SENSOR_DEVICE_ATTR_RW(gnss_led_status                    ,cpld               ,GNSS_LED_STATUS                  );
static SENSOR_DEVICE_ATTR_RW(gnss_led_blinking                  ,cpld               ,GNSS_LED_BLINKING                );
static SENSOR_DEVICE_ATTR_RW(gnss_led_color                     ,cpld               ,GNSS_LED_COLOR                   );
static SENSOR_DEVICE_ATTR_RW(sync_led_status                    ,cpld               ,SYNC_LED_STATUS                  );
static SENSOR_DEVICE_ATTR_RW(sync_led_blinking                  ,cpld               ,SYNC_LED_BLINKING                );
static SENSOR_DEVICE_ATTR_RW(sync_led_color                     ,cpld               ,SYNC_LED_COLOR                   );
static SENSOR_DEVICE_ATTR_RO(pwr_led_status                     ,cpld               ,PWR_LED_STATUS                   );
static SENSOR_DEVICE_ATTR_RO(pwr_led_blinking                   ,cpld               ,PWR_LED_BLINKING                 );
static SENSOR_DEVICE_ATTR_RO(pwr_led_color                      ,cpld               ,PWR_LED_COLOR                    );
static SENSOR_DEVICE_ATTR_RO(fan_led_status                     ,cpld               ,FAN_LED_STATUS                   );
static SENSOR_DEVICE_ATTR_RO(fan_led_blinking                   ,cpld               ,FAN_LED_BLINKING                 );
static SENSOR_DEVICE_ATTR_RO(fan_led_color                      ,cpld               ,FAN_LED_COLOR                    );
static SENSOR_DEVICE_ATTR_RW(sys_led                            ,cpld               ,SYS_LED                          );
static SENSOR_DEVICE_ATTR_RW(gnss_led                           ,cpld               ,GNSS_LED                         );
static SENSOR_DEVICE_ATTR_RW(sync_led                           ,cpld               ,SYNC_LED                         );
static SENSOR_DEVICE_ATTR_RO(fan_led                            ,cpld               ,FAN_LED                          );
static SENSOR_DEVICE_ATTR_RO(pwr_led                            ,cpld               ,PWR_LED                          );
static SENSOR_DEVICE_ATTR_RW(cpld_test                          ,cpld               ,CPLD_TEST                        );
static SENSOR_DEVICE_ATTR_RW(bsp_debug                          ,bsp_callback       ,BSP_DEBUG                        );
/******************************************************* CPLD 2 *******************************************************/
static SENSOR_DEVICE_ATTR_RO(port_0_abs                         ,cpld               ,PORT_0_ABS                       );
static SENSOR_DEVICE_ATTR_RO(port_1_abs                         ,cpld               ,PORT_1_ABS                       );
static SENSOR_DEVICE_ATTR_RO(port_2_abs                         ,cpld               ,PORT_2_ABS                       );
static SENSOR_DEVICE_ATTR_RO(port_3_abs                         ,cpld               ,PORT_3_ABS                       );
static SENSOR_DEVICE_ATTR_RO(port_4_abs                         ,cpld               ,PORT_4_ABS                       );
static SENSOR_DEVICE_ATTR_RO(port_5_abs                         ,cpld               ,PORT_5_ABS                       );
static SENSOR_DEVICE_ATTR_RO(port_6_abs                         ,cpld               ,PORT_6_ABS                       );
static SENSOR_DEVICE_ATTR_RO(port_7_abs                         ,cpld               ,PORT_7_ABS                       );
static SENSOR_DEVICE_ATTR_RO(port_8_abs                         ,cpld               ,PORT_8_ABS                       );
static SENSOR_DEVICE_ATTR_RO(port_9_abs                         ,cpld               ,PORT_9_ABS                       );
static SENSOR_DEVICE_ATTR_RO(port_10_abs                        ,cpld               ,PORT_10_ABS                      );
static SENSOR_DEVICE_ATTR_RO(port_11_abs                        ,cpld               ,PORT_11_ABS                      );
static SENSOR_DEVICE_ATTR_RO(port_12_abs                        ,cpld               ,PORT_12_ABS                      );
static SENSOR_DEVICE_ATTR_RO(port_13_abs                        ,cpld               ,PORT_13_ABS                      );
static SENSOR_DEVICE_ATTR_RO(port_14_abs                        ,cpld               ,PORT_14_ABS                      );
static SENSOR_DEVICE_ATTR_RO(port_15_abs                        ,cpld               ,PORT_15_ABS                      );
static SENSOR_DEVICE_ATTR_RO(port_16_abs                        ,cpld               ,PORT_16_ABS                      );
static SENSOR_DEVICE_ATTR_RO(port_17_abs                        ,cpld               ,PORT_17_ABS                      );
static SENSOR_DEVICE_ATTR_RO(port_18_abs                        ,cpld               ,PORT_18_ABS                      );
static SENSOR_DEVICE_ATTR_RO(port_19_abs                        ,cpld               ,PORT_19_ABS                      );
static SENSOR_DEVICE_ATTR_RW(port_0_abs_mask                    ,cpld               ,PORT_0_ABS_MASK                  );
static SENSOR_DEVICE_ATTR_RW(port_1_abs_mask                    ,cpld               ,PORT_1_ABS_MASK                  );
static SENSOR_DEVICE_ATTR_RW(port_2_abs_mask                    ,cpld               ,PORT_2_ABS_MASK                  );
static SENSOR_DEVICE_ATTR_RW(port_3_abs_mask                    ,cpld               ,PORT_3_ABS_MASK                  );
static SENSOR_DEVICE_ATTR_RW(port_4_abs_mask                    ,cpld               ,PORT_4_ABS_MASK                  );
static SENSOR_DEVICE_ATTR_RW(port_5_abs_mask                    ,cpld               ,PORT_5_ABS_MASK                  );
static SENSOR_DEVICE_ATTR_RW(port_6_abs_mask                    ,cpld               ,PORT_6_ABS_MASK                  );
static SENSOR_DEVICE_ATTR_RW(port_7_abs_mask                    ,cpld               ,PORT_7_ABS_MASK                  );
static SENSOR_DEVICE_ATTR_RW(port_8_abs_mask                    ,cpld               ,PORT_8_ABS_MASK                  );
static SENSOR_DEVICE_ATTR_RW(port_9_abs_mask                    ,cpld               ,PORT_9_ABS_MASK                  );
static SENSOR_DEVICE_ATTR_RW(port_10_abs_mask                   ,cpld               ,PORT_10_ABS_MASK                 );
static SENSOR_DEVICE_ATTR_RW(port_11_abs_mask                   ,cpld               ,PORT_11_ABS_MASK                 );
static SENSOR_DEVICE_ATTR_RW(port_12_abs_mask                   ,cpld               ,PORT_12_ABS_MASK                 );
static SENSOR_DEVICE_ATTR_RW(port_13_abs_mask                   ,cpld               ,PORT_13_ABS_MASK                 );
static SENSOR_DEVICE_ATTR_RW(port_14_abs_mask                   ,cpld               ,PORT_14_ABS_MASK                 );
static SENSOR_DEVICE_ATTR_RW(port_15_abs_mask                   ,cpld               ,PORT_15_ABS_MASK                 );
static SENSOR_DEVICE_ATTR_RW(port_16_abs_mask                   ,cpld               ,PORT_16_ABS_MASK                 );
static SENSOR_DEVICE_ATTR_RW(port_17_abs_mask                   ,cpld               ,PORT_17_ABS_MASK                 );
static SENSOR_DEVICE_ATTR_RW(port_18_abs_mask                   ,cpld               ,PORT_18_ABS_MASK                 );
static SENSOR_DEVICE_ATTR_RW(port_19_abs_mask                   ,cpld               ,PORT_19_ABS_MASK                 );
static SENSOR_DEVICE_ATTR_RO(port_0_7_abs_event                 ,cpld               ,PORT_0_7_ABS_EVENT               );
static SENSOR_DEVICE_ATTR_RO(port_8_15_abs_event                ,cpld               ,PORT_8_15_ABS_EVENT              );
static SENSOR_DEVICE_ATTR_RO(port_16_19_abs_event               ,cpld               ,PORT_16_19_ABS_EVENT             );
static SENSOR_DEVICE_ATTR_RW(port_0_abs_debug                   ,cpld               ,PORT_0_ABS_DEBUG                 );
static SENSOR_DEVICE_ATTR_RW(port_1_abs_debug                   ,cpld               ,PORT_1_ABS_DEBUG                 );
static SENSOR_DEVICE_ATTR_RW(port_2_abs_debug                   ,cpld               ,PORT_2_ABS_DEBUG                 );
static SENSOR_DEVICE_ATTR_RW(port_3_abs_debug                   ,cpld               ,PORT_3_ABS_DEBUG                 );
static SENSOR_DEVICE_ATTR_RW(port_4_abs_debug                   ,cpld               ,PORT_4_ABS_DEBUG                 );
static SENSOR_DEVICE_ATTR_RW(port_5_abs_debug                   ,cpld               ,PORT_5_ABS_DEBUG                 );
static SENSOR_DEVICE_ATTR_RW(port_6_abs_debug                   ,cpld               ,PORT_6_ABS_DEBUG                 );
static SENSOR_DEVICE_ATTR_RW(port_7_abs_debug                   ,cpld               ,PORT_7_ABS_DEBUG                 );
static SENSOR_DEVICE_ATTR_RW(port_8_abs_debug                   ,cpld               ,PORT_8_ABS_DEBUG                 );
static SENSOR_DEVICE_ATTR_RW(port_9_abs_debug                   ,cpld               ,PORT_9_ABS_DEBUG                 );
static SENSOR_DEVICE_ATTR_RW(port_10_abs_debug                  ,cpld               ,PORT_10_ABS_DEBUG                );
static SENSOR_DEVICE_ATTR_RW(port_11_abs_debug                  ,cpld               ,PORT_11_ABS_DEBUG                );
static SENSOR_DEVICE_ATTR_RW(port_12_abs_debug                  ,cpld               ,PORT_12_ABS_DEBUG                );
static SENSOR_DEVICE_ATTR_RW(port_13_abs_debug                  ,cpld               ,PORT_13_ABS_DEBUG                );
static SENSOR_DEVICE_ATTR_RW(port_14_abs_debug                  ,cpld               ,PORT_14_ABS_DEBUG                );
static SENSOR_DEVICE_ATTR_RW(port_15_abs_debug                  ,cpld               ,PORT_15_ABS_DEBUG                );
static SENSOR_DEVICE_ATTR_RW(port_16_abs_debug                  ,cpld               ,PORT_16_ABS_DEBUG                );
static SENSOR_DEVICE_ATTR_RW(port_17_abs_debug                  ,cpld               ,PORT_17_ABS_DEBUG                );
static SENSOR_DEVICE_ATTR_RW(port_18_abs_debug                  ,cpld               ,PORT_18_ABS_DEBUG                );
static SENSOR_DEVICE_ATTR_RW(port_19_abs_debug                  ,cpld               ,PORT_19_ABS_DEBUG                );
static SENSOR_DEVICE_ATTR_RO(port_0_rx_los                      ,cpld               ,PORT_0_RX_LOS                    );
static SENSOR_DEVICE_ATTR_RO(port_1_rx_los                      ,cpld               ,PORT_1_RX_LOS                    );
static SENSOR_DEVICE_ATTR_RO(port_2_rx_los                      ,cpld               ,PORT_2_RX_LOS                    );
static SENSOR_DEVICE_ATTR_RO(port_3_rx_los                      ,cpld               ,PORT_3_RX_LOS                    );
static SENSOR_DEVICE_ATTR_RO(port_4_rx_los                      ,cpld               ,PORT_4_RX_LOS                    );
static SENSOR_DEVICE_ATTR_RO(port_5_rx_los                      ,cpld               ,PORT_5_RX_LOS                    );
static SENSOR_DEVICE_ATTR_RO(port_6_rx_los                      ,cpld               ,PORT_6_RX_LOS                    );
static SENSOR_DEVICE_ATTR_RO(port_7_rx_los                      ,cpld               ,PORT_7_RX_LOS                    );
static SENSOR_DEVICE_ATTR_RO(port_8_rx_los                      ,cpld               ,PORT_8_RX_LOS                    );
static SENSOR_DEVICE_ATTR_RO(port_9_rx_los                      ,cpld               ,PORT_9_RX_LOS                    );
static SENSOR_DEVICE_ATTR_RO(port_10_rx_los                     ,cpld               ,PORT_10_RX_LOS                   );
static SENSOR_DEVICE_ATTR_RO(port_11_rx_los                     ,cpld               ,PORT_11_RX_LOS                   );
static SENSOR_DEVICE_ATTR_RO(port_12_rx_los                     ,cpld               ,PORT_12_RX_LOS                   );
static SENSOR_DEVICE_ATTR_RO(port_13_rx_los                     ,cpld               ,PORT_13_RX_LOS                   );
static SENSOR_DEVICE_ATTR_RO(port_14_rx_los                     ,cpld               ,PORT_14_RX_LOS                   );
static SENSOR_DEVICE_ATTR_RO(port_15_rx_los                     ,cpld               ,PORT_15_RX_LOS                   );
static SENSOR_DEVICE_ATTR_RO(port_16_rx_los                     ,cpld               ,PORT_16_RX_LOS                   );
static SENSOR_DEVICE_ATTR_RO(port_17_rx_los                     ,cpld               ,PORT_17_RX_LOS                   );
static SENSOR_DEVICE_ATTR_RO(port_18_rx_los                     ,cpld               ,PORT_18_RX_LOS                   );
static SENSOR_DEVICE_ATTR_RO(port_19_rx_los                     ,cpld               ,PORT_19_RX_LOS                   );
static SENSOR_DEVICE_ATTR_RW(port_0_rx_los_mask                 ,cpld               ,PORT_0_RX_LOS_MASK               );
static SENSOR_DEVICE_ATTR_RW(port_1_rx_los_mask                 ,cpld               ,PORT_1_RX_LOS_MASK               );
static SENSOR_DEVICE_ATTR_RW(port_2_rx_los_mask                 ,cpld               ,PORT_2_RX_LOS_MASK               );
static SENSOR_DEVICE_ATTR_RW(port_3_rx_los_mask                 ,cpld               ,PORT_3_RX_LOS_MASK               );
static SENSOR_DEVICE_ATTR_RW(port_4_rx_los_mask                 ,cpld               ,PORT_4_RX_LOS_MASK               );
static SENSOR_DEVICE_ATTR_RW(port_5_rx_los_mask                 ,cpld               ,PORT_5_RX_LOS_MASK               );
static SENSOR_DEVICE_ATTR_RW(port_6_rx_los_mask                 ,cpld               ,PORT_6_RX_LOS_MASK               );
static SENSOR_DEVICE_ATTR_RW(port_7_rx_los_mask                 ,cpld               ,PORT_7_RX_LOS_MASK               );
static SENSOR_DEVICE_ATTR_RW(port_8_rx_los_mask                 ,cpld               ,PORT_8_RX_LOS_MASK               );
static SENSOR_DEVICE_ATTR_RW(port_9_rx_los_mask                 ,cpld               ,PORT_9_RX_LOS_MASK               );
static SENSOR_DEVICE_ATTR_RW(port_10_rx_los_mask                ,cpld               ,PORT_10_RX_LOS_MASK              );
static SENSOR_DEVICE_ATTR_RW(port_11_rx_los_mask                ,cpld               ,PORT_11_RX_LOS_MASK              );
static SENSOR_DEVICE_ATTR_RW(port_12_rx_los_mask                ,cpld               ,PORT_12_RX_LOS_MASK              );
static SENSOR_DEVICE_ATTR_RW(port_13_rx_los_mask                ,cpld               ,PORT_13_RX_LOS_MASK              );
static SENSOR_DEVICE_ATTR_RW(port_14_rx_los_mask                ,cpld               ,PORT_14_RX_LOS_MASK              );
static SENSOR_DEVICE_ATTR_RW(port_15_rx_los_mask                ,cpld               ,PORT_15_RX_LOS_MASK              );
static SENSOR_DEVICE_ATTR_RW(port_16_rx_los_mask                ,cpld               ,PORT_16_RX_LOS_MASK              );
static SENSOR_DEVICE_ATTR_RW(port_17_rx_los_mask                ,cpld               ,PORT_17_RX_LOS_MASK              );
static SENSOR_DEVICE_ATTR_RW(port_18_rx_los_mask                ,cpld               ,PORT_18_RX_LOS_MASK              );
static SENSOR_DEVICE_ATTR_RW(port_19_rx_los_mask                ,cpld               ,PORT_19_RX_LOS_MASK              );
static SENSOR_DEVICE_ATTR_RO(port_0_7_rx_los_event              ,cpld               ,PORT_0_7_RX_LOS_EVENT            );
static SENSOR_DEVICE_ATTR_RO(port_8_15_rx_los_event             ,cpld               ,PORT_8_15_RX_LOS_EVENT           );
static SENSOR_DEVICE_ATTR_RO(port_16_19_rx_los_event            ,cpld               ,PORT_16_19_RX_LOS_EVENT          );
static SENSOR_DEVICE_ATTR_RW(port_0_rx_los_debug                ,cpld               ,PORT_0_RX_LOS_DEBUG              );
static SENSOR_DEVICE_ATTR_RW(port_1_rx_los_debug                ,cpld               ,PORT_1_RX_LOS_DEBUG              );
static SENSOR_DEVICE_ATTR_RW(port_2_rx_los_debug                ,cpld               ,PORT_2_RX_LOS_DEBUG              );
static SENSOR_DEVICE_ATTR_RW(port_3_rx_los_debug                ,cpld               ,PORT_3_RX_LOS_DEBUG              );
static SENSOR_DEVICE_ATTR_RW(port_4_rx_los_debug                ,cpld               ,PORT_4_RX_LOS_DEBUG              );
static SENSOR_DEVICE_ATTR_RW(port_5_rx_los_debug                ,cpld               ,PORT_5_RX_LOS_DEBUG              );
static SENSOR_DEVICE_ATTR_RW(port_6_rx_los_debug                ,cpld               ,PORT_6_RX_LOS_DEBUG              );
static SENSOR_DEVICE_ATTR_RW(port_7_rx_los_debug                ,cpld               ,PORT_7_RX_LOS_DEBUG              );
static SENSOR_DEVICE_ATTR_RW(port_8_rx_los_debug                ,cpld               ,PORT_8_RX_LOS_DEBUG              );
static SENSOR_DEVICE_ATTR_RW(port_9_rx_los_debug                ,cpld               ,PORT_9_RX_LOS_DEBUG              );
static SENSOR_DEVICE_ATTR_RW(port_10_rx_los_debug               ,cpld               ,PORT_10_RX_LOS_DEBUG             );
static SENSOR_DEVICE_ATTR_RW(port_11_rx_los_debug               ,cpld               ,PORT_11_RX_LOS_DEBUG             );
static SENSOR_DEVICE_ATTR_RW(port_12_rx_los_debug               ,cpld               ,PORT_12_RX_LOS_DEBUG             );
static SENSOR_DEVICE_ATTR_RW(port_13_rx_los_debug               ,cpld               ,PORT_13_RX_LOS_DEBUG             );
static SENSOR_DEVICE_ATTR_RW(port_14_rx_los_debug               ,cpld               ,PORT_14_RX_LOS_DEBUG             );
static SENSOR_DEVICE_ATTR_RW(port_15_rx_los_debug               ,cpld               ,PORT_15_RX_LOS_DEBUG             );
static SENSOR_DEVICE_ATTR_RW(port_16_rx_los_debug               ,cpld               ,PORT_16_RX_LOS_DEBUG             );
static SENSOR_DEVICE_ATTR_RW(port_17_rx_los_debug               ,cpld               ,PORT_17_RX_LOS_DEBUG             );
static SENSOR_DEVICE_ATTR_RW(port_18_rx_los_debug               ,cpld               ,PORT_18_RX_LOS_DEBUG             );
static SENSOR_DEVICE_ATTR_RW(port_19_rx_los_debug               ,cpld               ,PORT_19_RX_LOS_DEBUG             );
static SENSOR_DEVICE_ATTR_RO(port_0_tx_fault                    ,cpld               ,PORT_0_TX_FAULT                  );
static SENSOR_DEVICE_ATTR_RO(port_1_tx_fault                    ,cpld               ,PORT_1_TX_FAULT                  );
static SENSOR_DEVICE_ATTR_RO(port_2_tx_fault                    ,cpld               ,PORT_2_TX_FAULT                  );
static SENSOR_DEVICE_ATTR_RO(port_3_tx_fault                    ,cpld               ,PORT_3_TX_FAULT                  );
static SENSOR_DEVICE_ATTR_RO(port_4_tx_fault                    ,cpld               ,PORT_4_TX_FAULT                  );
static SENSOR_DEVICE_ATTR_RO(port_5_tx_fault                    ,cpld               ,PORT_5_TX_FAULT                  );
static SENSOR_DEVICE_ATTR_RO(port_6_tx_fault                    ,cpld               ,PORT_6_TX_FAULT                  );
static SENSOR_DEVICE_ATTR_RO(port_7_tx_fault                    ,cpld               ,PORT_7_TX_FAULT                  );
static SENSOR_DEVICE_ATTR_RO(port_8_tx_fault                    ,cpld               ,PORT_8_TX_FAULT                  );
static SENSOR_DEVICE_ATTR_RO(port_9_tx_fault                    ,cpld               ,PORT_9_TX_FAULT                  );
static SENSOR_DEVICE_ATTR_RO(port_10_tx_fault                   ,cpld               ,PORT_10_TX_FAULT                 );
static SENSOR_DEVICE_ATTR_RO(port_11_tx_fault                   ,cpld               ,PORT_11_TX_FAULT                 );
static SENSOR_DEVICE_ATTR_RO(port_12_tx_fault                   ,cpld               ,PORT_12_TX_FAULT                 );
static SENSOR_DEVICE_ATTR_RO(port_13_tx_fault                   ,cpld               ,PORT_13_TX_FAULT                 );
static SENSOR_DEVICE_ATTR_RO(port_14_tx_fault                   ,cpld               ,PORT_14_TX_FAULT                 );
static SENSOR_DEVICE_ATTR_RO(port_15_tx_fault                   ,cpld               ,PORT_15_TX_FAULT                 );
static SENSOR_DEVICE_ATTR_RO(port_16_tx_fault                   ,cpld               ,PORT_16_TX_FAULT                 );
static SENSOR_DEVICE_ATTR_RO(port_17_tx_fault                   ,cpld               ,PORT_17_TX_FAULT                 );
static SENSOR_DEVICE_ATTR_RO(port_18_tx_fault                   ,cpld               ,PORT_18_TX_FAULT                 );
static SENSOR_DEVICE_ATTR_RO(port_19_tx_fault                   ,cpld               ,PORT_19_TX_FAULT                 );
static SENSOR_DEVICE_ATTR_RW(port_0_tx_fault_mask               ,cpld               ,PORT_0_TX_FAULT_MASK             );
static SENSOR_DEVICE_ATTR_RW(port_1_tx_fault_mask               ,cpld               ,PORT_1_TX_FAULT_MASK             );
static SENSOR_DEVICE_ATTR_RW(port_2_tx_fault_mask               ,cpld               ,PORT_2_TX_FAULT_MASK             );
static SENSOR_DEVICE_ATTR_RW(port_3_tx_fault_mask               ,cpld               ,PORT_3_TX_FAULT_MASK             );
static SENSOR_DEVICE_ATTR_RW(port_4_tx_fault_mask               ,cpld               ,PORT_4_TX_FAULT_MASK             );
static SENSOR_DEVICE_ATTR_RW(port_5_tx_fault_mask               ,cpld               ,PORT_5_TX_FAULT_MASK             );
static SENSOR_DEVICE_ATTR_RW(port_6_tx_fault_mask               ,cpld               ,PORT_6_TX_FAULT_MASK             );
static SENSOR_DEVICE_ATTR_RW(port_7_tx_fault_mask               ,cpld               ,PORT_7_TX_FAULT_MASK             );
static SENSOR_DEVICE_ATTR_RW(port_8_tx_fault_mask               ,cpld               ,PORT_8_TX_FAULT_MASK             );
static SENSOR_DEVICE_ATTR_RW(port_9_tx_fault_mask               ,cpld               ,PORT_9_TX_FAULT_MASK             );
static SENSOR_DEVICE_ATTR_RW(port_10_tx_fault_mask              ,cpld               ,PORT_10_TX_FAULT_MASK            );
static SENSOR_DEVICE_ATTR_RW(port_11_tx_fault_mask              ,cpld               ,PORT_11_TX_FAULT_MASK            );
static SENSOR_DEVICE_ATTR_RW(port_12_tx_fault_mask              ,cpld               ,PORT_12_TX_FAULT_MASK            );
static SENSOR_DEVICE_ATTR_RW(port_13_tx_fault_mask              ,cpld               ,PORT_13_TX_FAULT_MASK            );
static SENSOR_DEVICE_ATTR_RW(port_14_tx_fault_mask              ,cpld               ,PORT_14_TX_FAULT_MASK            );
static SENSOR_DEVICE_ATTR_RW(port_15_tx_fault_mask              ,cpld               ,PORT_15_TX_FAULT_MASK            );
static SENSOR_DEVICE_ATTR_RW(port_16_tx_fault_mask              ,cpld               ,PORT_16_TX_FAULT_MASK            );
static SENSOR_DEVICE_ATTR_RW(port_17_tx_fault_mask              ,cpld               ,PORT_17_TX_FAULT_MASK            );
static SENSOR_DEVICE_ATTR_RW(port_18_tx_fault_mask              ,cpld               ,PORT_18_TX_FAULT_MASK            );
static SENSOR_DEVICE_ATTR_RW(port_19_tx_fault_mask              ,cpld               ,PORT_19_TX_FAULT_MASK            );
static SENSOR_DEVICE_ATTR_RO(port_0_7_tx_fault_event            ,cpld               ,PORT_0_7_TX_FAULT_EVENT          );
static SENSOR_DEVICE_ATTR_RO(port_8_15_tx_fault_event           ,cpld               ,PORT_8_15_TX_FAULT_EVENT         );
static SENSOR_DEVICE_ATTR_RO(port_16_19_tx_fault_event          ,cpld               ,PORT_16_19_TX_FAULT_EVENT        );
static SENSOR_DEVICE_ATTR_RW(port_0_tx_fault_debug              ,cpld               ,PORT_0_TX_FAULT_DEBUG            );
static SENSOR_DEVICE_ATTR_RW(port_1_tx_fault_debug              ,cpld               ,PORT_1_TX_FAULT_DEBUG            );
static SENSOR_DEVICE_ATTR_RW(port_2_tx_fault_debug              ,cpld               ,PORT_2_TX_FAULT_DEBUG            );
static SENSOR_DEVICE_ATTR_RW(port_3_tx_fault_debug              ,cpld               ,PORT_3_TX_FAULT_DEBUG            );
static SENSOR_DEVICE_ATTR_RW(port_4_tx_fault_debug              ,cpld               ,PORT_4_TX_FAULT_DEBUG            );
static SENSOR_DEVICE_ATTR_RW(port_5_tx_fault_debug              ,cpld               ,PORT_5_TX_FAULT_DEBUG            );
static SENSOR_DEVICE_ATTR_RW(port_6_tx_fault_debug              ,cpld               ,PORT_6_TX_FAULT_DEBUG            );
static SENSOR_DEVICE_ATTR_RW(port_7_tx_fault_debug              ,cpld               ,PORT_7_TX_FAULT_DEBUG            );
static SENSOR_DEVICE_ATTR_RW(port_8_tx_fault_debug              ,cpld               ,PORT_8_TX_FAULT_DEBUG            );
static SENSOR_DEVICE_ATTR_RW(port_9_tx_fault_debug              ,cpld               ,PORT_9_TX_FAULT_DEBUG            );
static SENSOR_DEVICE_ATTR_RW(port_10_tx_fault_debug             ,cpld               ,PORT_10_TX_FAULT_DEBUG           );
static SENSOR_DEVICE_ATTR_RW(port_11_tx_fault_debug             ,cpld               ,PORT_11_TX_FAULT_DEBUG           );
static SENSOR_DEVICE_ATTR_RW(port_12_tx_fault_debug             ,cpld               ,PORT_12_TX_FAULT_DEBUG           );
static SENSOR_DEVICE_ATTR_RW(port_13_tx_fault_debug             ,cpld               ,PORT_13_TX_FAULT_DEBUG           );
static SENSOR_DEVICE_ATTR_RW(port_14_tx_fault_debug             ,cpld               ,PORT_14_TX_FAULT_DEBUG           );
static SENSOR_DEVICE_ATTR_RW(port_15_tx_fault_debug             ,cpld               ,PORT_15_TX_FAULT_DEBUG           );
static SENSOR_DEVICE_ATTR_RW(port_16_tx_fault_debug             ,cpld               ,PORT_16_TX_FAULT_DEBUG           );
static SENSOR_DEVICE_ATTR_RW(port_17_tx_fault_debug             ,cpld               ,PORT_17_TX_FAULT_DEBUG           );
static SENSOR_DEVICE_ATTR_RW(port_18_tx_fault_debug             ,cpld               ,PORT_18_TX_FAULT_DEBUG           );
static SENSOR_DEVICE_ATTR_RW(port_19_tx_fault_debug             ,cpld               ,PORT_19_TX_FAULT_DEBUG           );
static SENSOR_DEVICE_ATTR_RW(port_0_tx_disable                  ,cpld               ,PORT_0_TX_DISABLE                );
static SENSOR_DEVICE_ATTR_RW(port_1_tx_disable                  ,cpld               ,PORT_1_TX_DISABLE                );
static SENSOR_DEVICE_ATTR_RW(port_2_tx_disable                  ,cpld               ,PORT_2_TX_DISABLE                );
static SENSOR_DEVICE_ATTR_RW(port_3_tx_disable                  ,cpld               ,PORT_3_TX_DISABLE                );
static SENSOR_DEVICE_ATTR_RW(port_4_tx_disable                  ,cpld               ,PORT_4_TX_DISABLE                );
static SENSOR_DEVICE_ATTR_RW(port_5_tx_disable                  ,cpld               ,PORT_5_TX_DISABLE                );
static SENSOR_DEVICE_ATTR_RW(port_6_tx_disable                  ,cpld               ,PORT_6_TX_DISABLE                );
static SENSOR_DEVICE_ATTR_RW(port_7_tx_disable                  ,cpld               ,PORT_7_TX_DISABLE                );
static SENSOR_DEVICE_ATTR_RW(port_8_tx_disable                  ,cpld               ,PORT_8_TX_DISABLE                );
static SENSOR_DEVICE_ATTR_RW(port_9_tx_disable                  ,cpld               ,PORT_9_TX_DISABLE                );
static SENSOR_DEVICE_ATTR_RW(port_10_tx_disable                 ,cpld               ,PORT_10_TX_DISABLE               );
static SENSOR_DEVICE_ATTR_RW(port_11_tx_disable                 ,cpld               ,PORT_11_TX_DISABLE               );
static SENSOR_DEVICE_ATTR_RW(port_12_tx_disable                 ,cpld               ,PORT_12_TX_DISABLE               );
static SENSOR_DEVICE_ATTR_RW(port_13_tx_disable                 ,cpld               ,PORT_13_TX_DISABLE               );
static SENSOR_DEVICE_ATTR_RW(port_14_tx_disable                 ,cpld               ,PORT_14_TX_DISABLE               );
static SENSOR_DEVICE_ATTR_RW(port_15_tx_disable                 ,cpld               ,PORT_15_TX_DISABLE               );
static SENSOR_DEVICE_ATTR_RW(port_16_tx_disable                 ,cpld               ,PORT_16_TX_DISABLE               );
static SENSOR_DEVICE_ATTR_RW(port_17_tx_disable                 ,cpld               ,PORT_17_TX_DISABLE               );
static SENSOR_DEVICE_ATTR_RW(port_18_tx_disable                 ,cpld               ,PORT_18_TX_DISABLE               );
static SENSOR_DEVICE_ATTR_RW(port_19_tx_disable                 ,cpld               ,PORT_19_TX_DISABLE               );
static SENSOR_DEVICE_ATTR_RW(port_0_rate_sel                    ,cpld               ,PORT_0_RATE_SEL                  );
static SENSOR_DEVICE_ATTR_RW(port_1_rate_sel                    ,cpld               ,PORT_1_RATE_SEL                  );
static SENSOR_DEVICE_ATTR_RW(port_2_rate_sel                    ,cpld               ,PORT_2_RATE_SEL                  );
static SENSOR_DEVICE_ATTR_RW(port_3_rate_sel                    ,cpld               ,PORT_3_RATE_SEL                  );
static SENSOR_DEVICE_ATTR_RW(port_4_rate_sel                    ,cpld               ,PORT_4_RATE_SEL                  );
static SENSOR_DEVICE_ATTR_RW(port_5_rate_sel                    ,cpld               ,PORT_5_RATE_SEL                  );
static SENSOR_DEVICE_ATTR_RW(port_6_rate_sel                    ,cpld               ,PORT_6_RATE_SEL                  );
static SENSOR_DEVICE_ATTR_RW(port_7_rate_sel                    ,cpld               ,PORT_7_RATE_SEL                  );
static SENSOR_DEVICE_ATTR_RW(port_8_rate_sel                    ,cpld               ,PORT_8_RATE_SEL                  );
static SENSOR_DEVICE_ATTR_RW(port_9_rate_sel                    ,cpld               ,PORT_9_RATE_SEL                  );
static SENSOR_DEVICE_ATTR_RW(port_10_rate_sel                   ,cpld               ,PORT_10_RATE_SEL                 );
static SENSOR_DEVICE_ATTR_RW(port_11_rate_sel                   ,cpld               ,PORT_11_RATE_SEL                 );
static SENSOR_DEVICE_ATTR_RW(port_12_rate_sel                   ,cpld               ,PORT_12_RATE_SEL                 );
static SENSOR_DEVICE_ATTR_RW(port_13_rate_sel                   ,cpld               ,PORT_13_RATE_SEL                 );
static SENSOR_DEVICE_ATTR_RW(port_14_rate_sel                   ,cpld               ,PORT_14_RATE_SEL                 );
static SENSOR_DEVICE_ATTR_RW(port_15_rate_sel                   ,cpld               ,PORT_15_RATE_SEL                 );
static SENSOR_DEVICE_ATTR_RW(port_16_rate_sel                   ,cpld               ,PORT_16_RATE_SEL                 );
static SENSOR_DEVICE_ATTR_RW(port_17_rate_sel                   ,cpld               ,PORT_17_RATE_SEL                 );
static SENSOR_DEVICE_ATTR_RW(port_18_rate_sel                   ,cpld               ,PORT_18_RATE_SEL                 );
static SENSOR_DEVICE_ATTR_RW(port_19_rate_sel                   ,cpld               ,PORT_19_RATE_SEL                 );
static SENSOR_DEVICE_ATTR_RW(port_0_pwr_en                      ,cpld               ,PORT_0_PWR_EN                    );
static SENSOR_DEVICE_ATTR_RW(port_1_pwr_en                      ,cpld               ,PORT_1_PWR_EN                    );
static SENSOR_DEVICE_ATTR_RW(port_2_pwr_en                      ,cpld               ,PORT_2_PWR_EN                    );
static SENSOR_DEVICE_ATTR_RW(port_3_pwr_en                      ,cpld               ,PORT_3_PWR_EN                    );
static SENSOR_DEVICE_ATTR_RW(port_4_pwr_en                      ,cpld               ,PORT_4_PWR_EN                    );
static SENSOR_DEVICE_ATTR_RW(port_5_pwr_en                      ,cpld               ,PORT_5_PWR_EN                    );
static SENSOR_DEVICE_ATTR_RW(port_6_pwr_en                      ,cpld               ,PORT_6_PWR_EN                    );
static SENSOR_DEVICE_ATTR_RW(port_7_pwr_en                      ,cpld               ,PORT_7_PWR_EN                    );
static SENSOR_DEVICE_ATTR_RW(port_8_pwr_en                      ,cpld               ,PORT_8_PWR_EN                    );
static SENSOR_DEVICE_ATTR_RW(port_9_pwr_en                      ,cpld               ,PORT_9_PWR_EN                    );
static SENSOR_DEVICE_ATTR_RW(port_10_pwr_en                     ,cpld               ,PORT_10_PWR_EN                   );
static SENSOR_DEVICE_ATTR_RW(port_11_pwr_en                     ,cpld               ,PORT_11_PWR_EN                   );
static SENSOR_DEVICE_ATTR_RW(port_12_pwr_en                     ,cpld               ,PORT_12_PWR_EN                   );
static SENSOR_DEVICE_ATTR_RW(port_13_pwr_en                     ,cpld               ,PORT_13_PWR_EN                   );
static SENSOR_DEVICE_ATTR_RW(port_14_pwr_en                     ,cpld               ,PORT_14_PWR_EN                   );
static SENSOR_DEVICE_ATTR_RW(port_15_pwr_en                     ,cpld               ,PORT_15_PWR_EN                   );
static SENSOR_DEVICE_ATTR_RW(port_16_pwr_en                     ,cpld               ,PORT_16_PWR_EN                   );
static SENSOR_DEVICE_ATTR_RW(port_17_pwr_en                     ,cpld               ,PORT_17_PWR_EN                   );
static SENSOR_DEVICE_ATTR_RW(port_18_pwr_en                     ,cpld               ,PORT_18_PWR_EN                   );
static SENSOR_DEVICE_ATTR_RW(port_19_pwr_en                     ,cpld               ,PORT_19_PWR_EN                   );
//static SENSOR_DEVICE_ATTR_RO(psu_0_vin_pwok                     ,cpld               ,PSU_0_VIN_PWOK                   );
//static SENSOR_DEVICE_ATTR_RO(psu_1_vin_pwok                     ,cpld               ,PSU_1_VIN_PWOK                   );
//static SENSOR_DEVICE_ATTR_RO(psu_0_vout_pwok                    ,cpld               ,PSU_0_VOUT_PWOK                  );
//static SENSOR_DEVICE_ATTR_RO(psu_1_vout_pwok                    ,cpld               ,PSU_1_VOUT_PWOK                  );
static SENSOR_DEVICE_ATTR_RO(intr_fan_0                         ,cpld               ,INTR_FAN_0                       );
static SENSOR_DEVICE_ATTR_RO(intr_fan_1                         ,cpld               ,INTR_FAN_1                       );
static SENSOR_DEVICE_ATTR_RO(intr_fan_2                         ,cpld               ,INTR_FAN_2                       );
static SENSOR_DEVICE_ATTR_RO(intr_fan_3                         ,cpld               ,INTR_FAN_3                       );
static SENSOR_DEVICE_ATTR_RO(fan_0_pwm_rpm_h                    ,cpld               ,FAN_0_PWM_RPM_H                  );
static SENSOR_DEVICE_ATTR_RO(fan_0_pwm_rpm_l                    ,cpld               ,FAN_0_PWM_RPM_L                  );
static SENSOR_DEVICE_ATTR_RO(fan_0_pwm_rpm                      ,fan_0_pwm_rpm      ,FAN_0_PWM_RPM                    );
static SENSOR_DEVICE_ATTR_RO(fan_1_pwm_rpm_h                    ,cpld               ,FAN_1_PWM_RPM_H                  );
static SENSOR_DEVICE_ATTR_RO(fan_1_pwm_rpm_l                    ,cpld               ,FAN_1_PWM_RPM_L                  );
static SENSOR_DEVICE_ATTR_RO(fan_1_pwm_rpm                      ,fan_1_pwm_rpm      ,FAN_1_PWM_RPM                    );
static SENSOR_DEVICE_ATTR_RO(fan_2_pwm_rpm_h                    ,cpld               ,FAN_2_PWM_RPM_H                  );
static SENSOR_DEVICE_ATTR_RO(fan_2_pwm_rpm_l                    ,cpld               ,FAN_2_PWM_RPM_L                  );
static SENSOR_DEVICE_ATTR_RO(fan_2_pwm_rpm                      ,fan_2_pwm_rpm      ,FAN_2_PWM_RPM                    );
static SENSOR_DEVICE_ATTR_RO(fan_3_pwm_rpm_h                    ,cpld               ,FAN_3_PWM_RPM_H                  );
static SENSOR_DEVICE_ATTR_RO(fan_3_pwm_rpm_l                    ,cpld               ,FAN_3_PWM_RPM_L                  );
static SENSOR_DEVICE_ATTR_RO(fan_3_pwm_rpm                      ,fan_3_pwm_rpm      ,FAN_3_PWM_RPM                    );
static SENSOR_DEVICE_ATTR_RW(fan_pwm_mode_1                     ,cpld               ,FAN_PWM_MODE_1                   );
static SENSOR_DEVICE_ATTR_RW(fan_pwm_mode_2                     ,cpld               ,FAN_PWM_MODE_2                   );
static SENSOR_DEVICE_ATTR_RW(fan_pwm_diag_ctrl_1                ,cpld               ,FAN_PWM_DIAG_CTRL_1              );
static SENSOR_DEVICE_ATTR_RW(fan_pwm_diag_ctrl_2                ,cpld               ,FAN_PWM_DIAG_CTRL_2              );
static SENSOR_DEVICE_ATTR_RO(vol_1_value                        ,cpld               ,VOL_1_VALUE                      );
static SENSOR_DEVICE_ATTR_RO(vol_2_value                        ,cpld               ,VOL_2_VALUE                      );
static SENSOR_DEVICE_ATTR_RO(vol_3_value                        ,cpld               ,VOL_3_VALUE                      );
static SENSOR_DEVICE_ATTR_RO(vol_4_value                        ,cpld               ,VOL_4_VALUE                      );
static SENSOR_DEVICE_ATTR_RO(vol_5_value                        ,cpld               ,VOL_5_VALUE                      );
static SENSOR_DEVICE_ATTR_RO(vol_6_value                        ,cpld               ,VOL_6_VALUE                      );
static SENSOR_DEVICE_ATTR_RO(vol_7_value                        ,cpld               ,VOL_7_VALUE                      );
static SENSOR_DEVICE_ATTR_RO(vol_8_value                        ,cpld               ,VOL_8_VALUE                      );
static SENSOR_DEVICE_ATTR_RO(vol_9_value                        ,cpld               ,VOL_9_VALUE                      );
static SENSOR_DEVICE_ATTR_RO(vol_10_value                       ,cpld               ,VOL_10_VALUE                     );
static SENSOR_DEVICE_ATTR_RO(vol_11_value                       ,cpld               ,VOL_11_VALUE                     );
static SENSOR_DEVICE_ATTR_RO(vol_12_value                       ,cpld               ,VOL_12_VALUE                     );
static SENSOR_DEVICE_ATTR_RO(vol_13_value                       ,cpld               ,VOL_13_VALUE                     );
static SENSOR_DEVICE_ATTR_RO(vol_14_value                       ,cpld               ,VOL_14_VALUE                     );
static SENSOR_DEVICE_ATTR_RO(vol_15_value                       ,cpld               ,VOL_15_VALUE                     );
static SENSOR_DEVICE_ATTR_RO(vol_16_value                       ,cpld               ,VOL_16_VALUE                     );


/* define support attributes of cpldx */
/* cpld 1 (AC PSU)) */
static struct attribute *cpld1_attributes[] =
{
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_chip_type),
    _DEVICE_ATTR(cpld_i2c_upgrade_module_reset),
    _DEVICE_ATTR(event_ctrl),

    _DEVICE_ATTR(sku_id),
    _DEVICE_ATTR(hw_rev),
    _DEVICE_ATTR(deph_id),
    _DEVICE_ATTR(build_id),
    _DEVICE_ATTR(bit_sel_id),
    _DEVICE_ATTR(ext_id),
    _DEVICE_ATTR(extend_id),
    _DEVICE_ATTR(intr_0),
    _DEVICE_ATTR(intr_1),
    _DEVICE_ATTR(intr_2),
    _DEVICE_ATTR(intr_3),
    _DEVICE_ATTR(intr_4),
    _DEVICE_ATTR(intr_5),
    _DEVICE_ATTR(intr_6),
    _DEVICE_ATTR(intr_7),
    _DEVICE_ATTR(intr_8),
    _DEVICE_ATTR(intr_9),
    _DEVICE_ATTR(intr_10),
    _DEVICE_ATTR(intr_11),
    _DEVICE_ATTR(intr_12),
    _DEVICE_ATTR(intr_13),
    _DEVICE_ATTR(intr_14),
    _DEVICE_ATTR(intr_15),
    _DEVICE_ATTR(intr_16),
    _DEVICE_ATTR(intr_17),
    _DEVICE_ATTR(intr_18),
    _DEVICE_ATTR(intr_19),
    _DEVICE_ATTR(intr_20),
    _DEVICE_ATTR(intr_21),
    _DEVICE_ATTR(intr_22),
    _DEVICE_ATTR(intr_23),
    _DEVICE_ATTR(intr_24),
    _DEVICE_ATTR(intr_25),
    _DEVICE_ATTR(intr_26),
    _DEVICE_ATTR(intr_27),
    _DEVICE_ATTR(intr_mask_0),
    _DEVICE_ATTR(intr_mask_1),
    _DEVICE_ATTR(intr_mask_2),
    _DEVICE_ATTR(intr_mask_3),
    _DEVICE_ATTR(intr_mask_4),
    _DEVICE_ATTR(intr_mask_5),
    _DEVICE_ATTR(intr_mask_6),
    _DEVICE_ATTR(intr_mask_7),
    _DEVICE_ATTR(intr_mask_8),
    _DEVICE_ATTR(intr_mask_9),
    _DEVICE_ATTR(intr_mask_10),
    _DEVICE_ATTR(intr_mask_11),
    _DEVICE_ATTR(intr_mask_12),
    _DEVICE_ATTR(intr_mask_13),
    _DEVICE_ATTR(intr_mask_14),
    _DEVICE_ATTR(intr_mask_15),
    _DEVICE_ATTR(intr_mask_16),
    _DEVICE_ATTR(intr_mask_17),
    _DEVICE_ATTR(intr_mask_18),
    _DEVICE_ATTR(intr_mask_19),
    _DEVICE_ATTR(intr_mask_20),
    _DEVICE_ATTR(intr_mask_21),
    _DEVICE_ATTR(intr_mask_22),
    _DEVICE_ATTR(intr_mask_23),
    _DEVICE_ATTR(intr_mask_24),
    _DEVICE_ATTR(intr_mask_25),
    _DEVICE_ATTR(intr_event_clk_ptp),
    _DEVICE_ATTR(intr_event_psu),
    _DEVICE_ATTR(intr_event_hwm),
    _DEVICE_ATTR(intr_event_thermal),
    _DEVICE_ATTR(intr_event_fan),
    _DEVICE_ATTR(intr_event_eth),
    _DEVICE_ATTR(intr_debug_1),
    _DEVICE_ATTR(intr_debug_2),
    _DEVICE_ATTR(intr_debug_3),
    _DEVICE_ATTR(intr_debug_4),
    _DEVICE_ATTR(intr_debug_5),
    _DEVICE_ATTR(intr_debug_6),
    _DEVICE_ATTR(intr_debug_7),
    _DEVICE_ATTR(intr_debug_8),
    _DEVICE_ATTR(intr_debug_9),
    _DEVICE_ATTR(intr_debug_10),
    _DEVICE_ATTR(intr_debug_11),
    _DEVICE_ATTR(intr_debug_12),
    _DEVICE_ATTR(intr_debug_13),
    _DEVICE_ATTR(intr_debug_14),
    _DEVICE_ATTR(intr_debug_15),
    _DEVICE_ATTR(intr_debug_16),
    _DEVICE_ATTR(intr_debug_17),
    _DEVICE_ATTR(intr_debug_18),
    _DEVICE_ATTR(intr_debug_19),
    _DEVICE_ATTR(intr_debug_20),
    _DEVICE_ATTR(reset_0),
    _DEVICE_ATTR(reset_1),
    _DEVICE_ATTR(reset_2),
    _DEVICE_ATTR(reset_3),
    _DEVICE_ATTR(reset_4),
    _DEVICE_ATTR(reset_5),
    _DEVICE_ATTR(reset_6),
    _DEVICE_ATTR(reset_7),
    _DEVICE_ATTR(reset_8),
    _DEVICE_ATTR(reset_9),
    _DEVICE_ATTR(reset_10),
    _DEVICE_ATTR(reset_11),
    _DEVICE_ATTR(reset_12),
    _DEVICE_ATTR(reset_13),
    _DEVICE_ATTR(reset_14),
    _DEVICE_ATTR(cpld_rov_reset),
    _DEVICE_ATTR(rov_debug_enable),
    _DEVICE_ATTR(rov_debug_1),
    _DEVICE_ATTR(rov_debug_2),
    _DEVICE_ATTR(sys_status_1),
    _DEVICE_ATTR(sys_status_2),
    _DEVICE_ATTR(sys_status_3),
    _DEVICE_ATTR(sys_status_4),
    _DEVICE_ATTR(sys_status_5),
    _DEVICE_ATTR(sys_status_6),
    _DEVICE_ATTR(sys_status_7_mac_rov1),
    _DEVICE_ATTR(sys_status_8_mac_rov2),
    _DEVICE_ATTR(sys_status_9_mac_rov3),
    _DEVICE_ATTR(sys_status_10),
    _DEVICE_ATTR(sys_status_11),
    _DEVICE_ATTR(sys_status_12),
    _DEVICE_ATTR(sys_status_13),
    _DEVICE_ATTR(sys_status_14),
    _DEVICE_ATTR(sys_status_15),
    _DEVICE_ATTR(sys_status_16),
    _DEVICE_ATTR(sys_status_17),
    _DEVICE_ATTR(sys_status_18),
    _DEVICE_ATTR(sys_status_19),
    _DEVICE_ATTR(sys_status_20),
    _DEVICE_ATTR(sys_status_21),
    _DEVICE_ATTR(sys_status_22),
    _DEVICE_ATTR(sys_status_23),
    _DEVICE_ATTR(sys_status_24),
    _DEVICE_ATTR(sys_status_25),
    _DEVICE_ATTR(sys_status_26),
    _DEVICE_ATTR(sys_status_27),
    _DEVICE_ATTR(sys_status_28),
    _DEVICE_ATTR(sys_status_29),
    _DEVICE_ATTR(psu0_present),
    _DEVICE_ATTR(psu1_present),
    _DEVICE_ATTR(psu0_vin_pwok),
    _DEVICE_ATTR(psu1_vin_pwok),
    _DEVICE_ATTR(psu0_pwok),
    _DEVICE_ATTR(psu1_pwok),
    _DEVICE_ATTR(psu_type),
    _DEVICE_ATTR(boot_select),
    _DEVICE_ATTR(clk_timing_ctrl_1),
    _DEVICE_ATTR(clk_timing_ctrl_2),
    _DEVICE_ATTR(pw_sys_ctrl),
    _DEVICE_ATTR(gnss_ctrl),
    _DEVICE_ATTR(usb_ctrl),
    _DEVICE_ATTR(synce_ctrl),
    _DEVICE_ATTR(ts_pll_clock_ctrl),
    _DEVICE_ATTR(led_clear),
    _DEVICE_ATTR(sys_led_status),
    _DEVICE_ATTR(sys_led_blinking),
    _DEVICE_ATTR(sys_led_color),
    _DEVICE_ATTR(gnss_led_status),
    _DEVICE_ATTR(gnss_led_blinking),
    _DEVICE_ATTR(gnss_led_color),
    _DEVICE_ATTR(sync_led_status),
    _DEVICE_ATTR(sync_led_blinking),
    _DEVICE_ATTR(sync_led_color),
    _DEVICE_ATTR(pwr_led_status),
    _DEVICE_ATTR(pwr_led_blinking),
    _DEVICE_ATTR(pwr_led_color),
    _DEVICE_ATTR(fan_led_status),
    _DEVICE_ATTR(fan_led_blinking),
    _DEVICE_ATTR(fan_led_color),
    _DEVICE_ATTR(sys_led),
    _DEVICE_ATTR(gnss_led),
    _DEVICE_ATTR(sync_led),
    _DEVICE_ATTR(fan_led),
    _DEVICE_ATTR(pwr_led),
    _DEVICE_ATTR(cpld_test),

    _DEVICE_ATTR(bsp_debug),

    NULL
};

/* cpld 2 (AC PSU) */
static struct attribute *cpld2_attributes[] =
{
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_chip_type),
    _DEVICE_ATTR(cpld_i2c_upgrade_module_reset),
    _DEVICE_ATTR(event_ctrl),

    _DEVICE_ATTR(port_0_abs),
    _DEVICE_ATTR(port_1_abs),
    _DEVICE_ATTR(port_2_abs),
    _DEVICE_ATTR(port_3_abs),
    _DEVICE_ATTR(port_4_abs),
    _DEVICE_ATTR(port_5_abs),
    _DEVICE_ATTR(port_6_abs),
    _DEVICE_ATTR(port_7_abs),
    _DEVICE_ATTR(port_8_abs),
    _DEVICE_ATTR(port_9_abs),
    _DEVICE_ATTR(port_10_abs),
    _DEVICE_ATTR(port_11_abs),
    _DEVICE_ATTR(port_12_abs),
    _DEVICE_ATTR(port_13_abs),
    _DEVICE_ATTR(port_14_abs),
    _DEVICE_ATTR(port_15_abs),
    _DEVICE_ATTR(port_16_abs),
    _DEVICE_ATTR(port_17_abs),
    _DEVICE_ATTR(port_18_abs),
    _DEVICE_ATTR(port_19_abs),
    _DEVICE_ATTR(port_0_abs_mask),
    _DEVICE_ATTR(port_1_abs_mask),
    _DEVICE_ATTR(port_2_abs_mask),
    _DEVICE_ATTR(port_3_abs_mask),
    _DEVICE_ATTR(port_4_abs_mask),
    _DEVICE_ATTR(port_5_abs_mask),
    _DEVICE_ATTR(port_6_abs_mask),
    _DEVICE_ATTR(port_7_abs_mask),
    _DEVICE_ATTR(port_8_abs_mask),
    _DEVICE_ATTR(port_9_abs_mask),
    _DEVICE_ATTR(port_10_abs_mask),
    _DEVICE_ATTR(port_11_abs_mask),
    _DEVICE_ATTR(port_12_abs_mask),
    _DEVICE_ATTR(port_13_abs_mask),
    _DEVICE_ATTR(port_14_abs_mask),
    _DEVICE_ATTR(port_15_abs_mask),
    _DEVICE_ATTR(port_16_abs_mask),
    _DEVICE_ATTR(port_17_abs_mask),
    _DEVICE_ATTR(port_18_abs_mask),
    _DEVICE_ATTR(port_19_abs_mask),
    _DEVICE_ATTR(port_0_7_abs_event),
    _DEVICE_ATTR(port_8_15_abs_event),
    _DEVICE_ATTR(port_16_19_abs_event),
    _DEVICE_ATTR(port_0_abs_debug),
    _DEVICE_ATTR(port_1_abs_debug),
    _DEVICE_ATTR(port_2_abs_debug),
    _DEVICE_ATTR(port_3_abs_debug),
    _DEVICE_ATTR(port_4_abs_debug),
    _DEVICE_ATTR(port_5_abs_debug),
    _DEVICE_ATTR(port_6_abs_debug),
    _DEVICE_ATTR(port_7_abs_debug),
    _DEVICE_ATTR(port_8_abs_debug),
    _DEVICE_ATTR(port_9_abs_debug),
    _DEVICE_ATTR(port_10_abs_debug),
    _DEVICE_ATTR(port_11_abs_debug),
    _DEVICE_ATTR(port_12_abs_debug),
    _DEVICE_ATTR(port_13_abs_debug),
    _DEVICE_ATTR(port_14_abs_debug),
    _DEVICE_ATTR(port_15_abs_debug),
    _DEVICE_ATTR(port_16_abs_debug),
    _DEVICE_ATTR(port_17_abs_debug),
    _DEVICE_ATTR(port_18_abs_debug),
    _DEVICE_ATTR(port_19_abs_debug),
    _DEVICE_ATTR(port_0_rx_los),
    _DEVICE_ATTR(port_1_rx_los),
    _DEVICE_ATTR(port_2_rx_los),
    _DEVICE_ATTR(port_3_rx_los),
    _DEVICE_ATTR(port_4_rx_los),
    _DEVICE_ATTR(port_5_rx_los),
    _DEVICE_ATTR(port_6_rx_los),
    _DEVICE_ATTR(port_7_rx_los),
    _DEVICE_ATTR(port_8_rx_los),
    _DEVICE_ATTR(port_9_rx_los),
    _DEVICE_ATTR(port_10_rx_los),
    _DEVICE_ATTR(port_11_rx_los),
    _DEVICE_ATTR(port_12_rx_los),
    _DEVICE_ATTR(port_13_rx_los),
    _DEVICE_ATTR(port_14_rx_los),
    _DEVICE_ATTR(port_15_rx_los),
    _DEVICE_ATTR(port_16_rx_los),
    _DEVICE_ATTR(port_17_rx_los),
    _DEVICE_ATTR(port_18_rx_los),
    _DEVICE_ATTR(port_19_rx_los),
    _DEVICE_ATTR(port_0_rx_los_mask),
    _DEVICE_ATTR(port_1_rx_los_mask),
    _DEVICE_ATTR(port_2_rx_los_mask),
    _DEVICE_ATTR(port_3_rx_los_mask),
    _DEVICE_ATTR(port_4_rx_los_mask),
    _DEVICE_ATTR(port_5_rx_los_mask),
    _DEVICE_ATTR(port_6_rx_los_mask),
    _DEVICE_ATTR(port_7_rx_los_mask),
    _DEVICE_ATTR(port_8_rx_los_mask),
    _DEVICE_ATTR(port_9_rx_los_mask),
    _DEVICE_ATTR(port_10_rx_los_mask),
    _DEVICE_ATTR(port_11_rx_los_mask),
    _DEVICE_ATTR(port_12_rx_los_mask),
    _DEVICE_ATTR(port_13_rx_los_mask),
    _DEVICE_ATTR(port_14_rx_los_mask),
    _DEVICE_ATTR(port_15_rx_los_mask),
    _DEVICE_ATTR(port_16_rx_los_mask),
    _DEVICE_ATTR(port_17_rx_los_mask),
    _DEVICE_ATTR(port_18_rx_los_mask),
    _DEVICE_ATTR(port_19_rx_los_mask),
    _DEVICE_ATTR(port_0_7_rx_los_event),
    _DEVICE_ATTR(port_8_15_rx_los_event),
    _DEVICE_ATTR(port_16_19_rx_los_event),
    _DEVICE_ATTR(port_0_rx_los_debug),
    _DEVICE_ATTR(port_1_rx_los_debug),
    _DEVICE_ATTR(port_2_rx_los_debug),
    _DEVICE_ATTR(port_3_rx_los_debug),
    _DEVICE_ATTR(port_4_rx_los_debug),
    _DEVICE_ATTR(port_5_rx_los_debug),
    _DEVICE_ATTR(port_6_rx_los_debug),
    _DEVICE_ATTR(port_7_rx_los_debug),
    _DEVICE_ATTR(port_8_rx_los_debug),
    _DEVICE_ATTR(port_9_rx_los_debug),
    _DEVICE_ATTR(port_10_rx_los_debug),
    _DEVICE_ATTR(port_11_rx_los_debug),
    _DEVICE_ATTR(port_12_rx_los_debug),
    _DEVICE_ATTR(port_13_rx_los_debug),
    _DEVICE_ATTR(port_14_rx_los_debug),
    _DEVICE_ATTR(port_15_rx_los_debug),
    _DEVICE_ATTR(port_16_rx_los_debug),
    _DEVICE_ATTR(port_17_rx_los_debug),
    _DEVICE_ATTR(port_18_rx_los_debug),
    _DEVICE_ATTR(port_19_rx_los_debug),
    _DEVICE_ATTR(port_0_tx_fault),
    _DEVICE_ATTR(port_1_tx_fault),
    _DEVICE_ATTR(port_2_tx_fault),
    _DEVICE_ATTR(port_3_tx_fault),
    _DEVICE_ATTR(port_4_tx_fault),
    _DEVICE_ATTR(port_5_tx_fault),
    _DEVICE_ATTR(port_6_tx_fault),
    _DEVICE_ATTR(port_7_tx_fault),
    _DEVICE_ATTR(port_8_tx_fault),
    _DEVICE_ATTR(port_9_tx_fault),
    _DEVICE_ATTR(port_10_tx_fault),
    _DEVICE_ATTR(port_11_tx_fault),
    _DEVICE_ATTR(port_12_tx_fault),
    _DEVICE_ATTR(port_13_tx_fault),
    _DEVICE_ATTR(port_14_tx_fault),
    _DEVICE_ATTR(port_15_tx_fault),
    _DEVICE_ATTR(port_16_tx_fault),
    _DEVICE_ATTR(port_17_tx_fault),
    _DEVICE_ATTR(port_18_tx_fault),
    _DEVICE_ATTR(port_19_tx_fault),
    _DEVICE_ATTR(port_0_tx_fault_mask),
    _DEVICE_ATTR(port_1_tx_fault_mask),
    _DEVICE_ATTR(port_2_tx_fault_mask),
    _DEVICE_ATTR(port_3_tx_fault_mask),
    _DEVICE_ATTR(port_4_tx_fault_mask),
    _DEVICE_ATTR(port_5_tx_fault_mask),
    _DEVICE_ATTR(port_6_tx_fault_mask),
    _DEVICE_ATTR(port_7_tx_fault_mask),
    _DEVICE_ATTR(port_8_tx_fault_mask),
    _DEVICE_ATTR(port_9_tx_fault_mask),
    _DEVICE_ATTR(port_10_tx_fault_mask),
    _DEVICE_ATTR(port_11_tx_fault_mask),
    _DEVICE_ATTR(port_12_tx_fault_mask),
    _DEVICE_ATTR(port_13_tx_fault_mask),
    _DEVICE_ATTR(port_14_tx_fault_mask),
    _DEVICE_ATTR(port_15_tx_fault_mask),
    _DEVICE_ATTR(port_16_tx_fault_mask),
    _DEVICE_ATTR(port_17_tx_fault_mask),
    _DEVICE_ATTR(port_18_tx_fault_mask),
    _DEVICE_ATTR(port_19_tx_fault_mask),
    _DEVICE_ATTR(port_0_7_tx_fault_event),
    _DEVICE_ATTR(port_8_15_tx_fault_event),
    _DEVICE_ATTR(port_16_19_tx_fault_event),
    _DEVICE_ATTR(port_0_tx_fault_debug),
    _DEVICE_ATTR(port_1_tx_fault_debug),
    _DEVICE_ATTR(port_2_tx_fault_debug),
    _DEVICE_ATTR(port_3_tx_fault_debug),
    _DEVICE_ATTR(port_4_tx_fault_debug),
    _DEVICE_ATTR(port_5_tx_fault_debug),
    _DEVICE_ATTR(port_6_tx_fault_debug),
    _DEVICE_ATTR(port_7_tx_fault_debug),
    _DEVICE_ATTR(port_8_tx_fault_debug),
    _DEVICE_ATTR(port_9_tx_fault_debug),
    _DEVICE_ATTR(port_10_tx_fault_debug),
    _DEVICE_ATTR(port_11_tx_fault_debug),
    _DEVICE_ATTR(port_12_tx_fault_debug),
    _DEVICE_ATTR(port_13_tx_fault_debug),
    _DEVICE_ATTR(port_14_tx_fault_debug),
    _DEVICE_ATTR(port_15_tx_fault_debug),
    _DEVICE_ATTR(port_16_tx_fault_debug),
    _DEVICE_ATTR(port_17_tx_fault_debug),
    _DEVICE_ATTR(port_18_tx_fault_debug),
    _DEVICE_ATTR(port_19_tx_fault_debug),
    _DEVICE_ATTR(port_0_tx_disable),
    _DEVICE_ATTR(port_1_tx_disable),
    _DEVICE_ATTR(port_2_tx_disable),
    _DEVICE_ATTR(port_3_tx_disable),
    _DEVICE_ATTR(port_4_tx_disable),
    _DEVICE_ATTR(port_5_tx_disable),
    _DEVICE_ATTR(port_6_tx_disable),
    _DEVICE_ATTR(port_7_tx_disable),
    _DEVICE_ATTR(port_8_tx_disable),
    _DEVICE_ATTR(port_9_tx_disable),
    _DEVICE_ATTR(port_10_tx_disable),
    _DEVICE_ATTR(port_11_tx_disable),
    _DEVICE_ATTR(port_12_tx_disable),
    _DEVICE_ATTR(port_13_tx_disable),
    _DEVICE_ATTR(port_14_tx_disable),
    _DEVICE_ATTR(port_15_tx_disable),
    _DEVICE_ATTR(port_16_tx_disable),
    _DEVICE_ATTR(port_17_tx_disable),
    _DEVICE_ATTR(port_18_tx_disable),
    _DEVICE_ATTR(port_19_tx_disable),
    _DEVICE_ATTR(port_0_rate_sel),
    _DEVICE_ATTR(port_1_rate_sel),
    _DEVICE_ATTR(port_2_rate_sel),
    _DEVICE_ATTR(port_3_rate_sel),
    _DEVICE_ATTR(port_4_rate_sel),
    _DEVICE_ATTR(port_5_rate_sel),
    _DEVICE_ATTR(port_6_rate_sel),
    _DEVICE_ATTR(port_7_rate_sel),
    _DEVICE_ATTR(port_8_rate_sel),
    _DEVICE_ATTR(port_9_rate_sel),
    _DEVICE_ATTR(port_10_rate_sel),
    _DEVICE_ATTR(port_11_rate_sel),
    _DEVICE_ATTR(port_12_rate_sel),
    _DEVICE_ATTR(port_13_rate_sel),
    _DEVICE_ATTR(port_14_rate_sel),
    _DEVICE_ATTR(port_15_rate_sel),
    _DEVICE_ATTR(port_16_rate_sel),
    _DEVICE_ATTR(port_17_rate_sel),
    _DEVICE_ATTR(port_18_rate_sel),
    _DEVICE_ATTR(port_19_rate_sel),
    _DEVICE_ATTR(port_0_pwr_en),
    _DEVICE_ATTR(port_1_pwr_en),
    _DEVICE_ATTR(port_2_pwr_en),
    _DEVICE_ATTR(port_3_pwr_en),
    _DEVICE_ATTR(port_4_pwr_en),
    _DEVICE_ATTR(port_5_pwr_en),
    _DEVICE_ATTR(port_6_pwr_en),
    _DEVICE_ATTR(port_7_pwr_en),
    _DEVICE_ATTR(port_8_pwr_en),
    _DEVICE_ATTR(port_9_pwr_en),
    _DEVICE_ATTR(port_10_pwr_en),
    _DEVICE_ATTR(port_11_pwr_en),
    _DEVICE_ATTR(port_12_pwr_en),
    _DEVICE_ATTR(port_13_pwr_en),
    _DEVICE_ATTR(port_14_pwr_en),
    _DEVICE_ATTR(port_15_pwr_en),
    _DEVICE_ATTR(port_16_pwr_en),
    _DEVICE_ATTR(port_17_pwr_en),
    _DEVICE_ATTR(port_18_pwr_en),
    _DEVICE_ATTR(port_19_pwr_en),
    //_DEVICE_ATTR(psu_0_vin_pwok),
    //_DEVICE_ATTR(psu_1_vin_pwok),
    //_DEVICE_ATTR(psu_0_vout_pwok),
    //_DEVICE_ATTR(psu_1_vout_pwok),
    _DEVICE_ATTR(intr_fan_0),
    _DEVICE_ATTR(intr_fan_1),
    _DEVICE_ATTR(intr_fan_2),
    //_DEVICE_ATTR(intr_fan_3),
    _DEVICE_ATTR(fan_0_pwm_rpm_h),
    _DEVICE_ATTR(fan_0_pwm_rpm_l),
    _DEVICE_ATTR(fan_0_pwm_rpm),
    _DEVICE_ATTR(fan_1_pwm_rpm_h),
    _DEVICE_ATTR(fan_1_pwm_rpm_l),
    _DEVICE_ATTR(fan_1_pwm_rpm),
    _DEVICE_ATTR(fan_2_pwm_rpm_h),
    _DEVICE_ATTR(fan_2_pwm_rpm_l),
    _DEVICE_ATTR(fan_2_pwm_rpm),
    //_DEVICE_ATTR(fan_3_pwm_rpm_h),
    //_DEVICE_ATTR(fan_3_pwm_rpm_l),
    //_DEVICE_ATTR(fan_3_pwm_rpm),
    _DEVICE_ATTR(fan_pwm_mode_1),
    _DEVICE_ATTR(fan_pwm_mode_2),
    _DEVICE_ATTR(fan_pwm_diag_ctrl_1),
    _DEVICE_ATTR(fan_pwm_diag_ctrl_2),
    _DEVICE_ATTR(vol_1_value),
    _DEVICE_ATTR(vol_2_value),
    _DEVICE_ATTR(vol_3_value),
    _DEVICE_ATTR(vol_4_value),
    _DEVICE_ATTR(vol_5_value),
    _DEVICE_ATTR(vol_6_value),
    _DEVICE_ATTR(vol_7_value),
    _DEVICE_ATTR(vol_8_value),
    _DEVICE_ATTR(vol_9_value),
    _DEVICE_ATTR(vol_10_value),
    _DEVICE_ATTR(vol_11_value),
    _DEVICE_ATTR(vol_12_value),
    _DEVICE_ATTR(vol_13_value),
    _DEVICE_ATTR(vol_14_value),
    _DEVICE_ATTR(vol_15_value),
    _DEVICE_ATTR(vol_16_value),

    NULL
};

/* cpld 1 (DC PSU)) */
static struct attribute *cpld1_dc_psu_attributes[] =
{
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_chip_type),
    _DEVICE_ATTR(cpld_i2c_upgrade_module_reset),
    _DEVICE_ATTR(event_ctrl),

    _DEVICE_ATTR(sku_id),
    _DEVICE_ATTR(hw_rev),
    _DEVICE_ATTR(deph_id),
    _DEVICE_ATTR(build_id),
    _DEVICE_ATTR(bit_sel_id),
    _DEVICE_ATTR(ext_id),
    _DEVICE_ATTR(extend_id),
    _DEVICE_ATTR(intr_0),
    _DEVICE_ATTR(intr_1),
    _DEVICE_ATTR(intr_2),
    _DEVICE_ATTR(intr_3),
    _DEVICE_ATTR(intr_4),
    _DEVICE_ATTR(intr_5),
    _DEVICE_ATTR(intr_6),
    _DEVICE_ATTR(intr_7),
    _DEVICE_ATTR(intr_8),
    _DEVICE_ATTR(intr_9),
    _DEVICE_ATTR(intr_10),
    _DEVICE_ATTR(intr_11),
    _DEVICE_ATTR(intr_12),
    _DEVICE_ATTR(intr_13),
    _DEVICE_ATTR(intr_14),
    _DEVICE_ATTR(intr_15),
    _DEVICE_ATTR(intr_16),
    _DEVICE_ATTR(intr_17),
    _DEVICE_ATTR(intr_18),
    _DEVICE_ATTR(intr_19),
    _DEVICE_ATTR(intr_20),
    _DEVICE_ATTR(intr_21),
    _DEVICE_ATTR(intr_22),
    _DEVICE_ATTR(intr_23),
    _DEVICE_ATTR(intr_24),
    _DEVICE_ATTR(intr_25),
    _DEVICE_ATTR(intr_26),
    _DEVICE_ATTR(intr_27),
    _DEVICE_ATTR(intr_mask_0),
    _DEVICE_ATTR(intr_mask_1),
    _DEVICE_ATTR(intr_mask_2),
    _DEVICE_ATTR(intr_mask_3),
    _DEVICE_ATTR(intr_mask_4),
    _DEVICE_ATTR(intr_mask_5),
    _DEVICE_ATTR(intr_mask_6),
    _DEVICE_ATTR(intr_mask_7),
    _DEVICE_ATTR(intr_mask_8),
    _DEVICE_ATTR(intr_mask_9),
    _DEVICE_ATTR(intr_mask_10),
    _DEVICE_ATTR(intr_mask_11),
    _DEVICE_ATTR(intr_mask_12),
    _DEVICE_ATTR(intr_mask_13),
    _DEVICE_ATTR(intr_mask_14),
    _DEVICE_ATTR(intr_mask_15),
    _DEVICE_ATTR(intr_mask_16),
    _DEVICE_ATTR(intr_mask_17),
    _DEVICE_ATTR(intr_mask_18),
    _DEVICE_ATTR(intr_mask_19),
    _DEVICE_ATTR(intr_mask_20),
    _DEVICE_ATTR(intr_mask_21),
    _DEVICE_ATTR(intr_mask_22),
    _DEVICE_ATTR(intr_mask_23),
    _DEVICE_ATTR(intr_mask_24),
    _DEVICE_ATTR(intr_mask_25),
    _DEVICE_ATTR(intr_event_clk_ptp),
    _DEVICE_ATTR(intr_event_psu),
    _DEVICE_ATTR(intr_event_hwm),
    _DEVICE_ATTR(intr_event_thermal),
    _DEVICE_ATTR(intr_event_fan),
    _DEVICE_ATTR(intr_event_eth),
    _DEVICE_ATTR(intr_debug_1),
    _DEVICE_ATTR(intr_debug_2),
    _DEVICE_ATTR(intr_debug_3),
    _DEVICE_ATTR(intr_debug_4),
    _DEVICE_ATTR(intr_debug_5),
    _DEVICE_ATTR(intr_debug_6),
    _DEVICE_ATTR(intr_debug_7),
    _DEVICE_ATTR(intr_debug_8),
    _DEVICE_ATTR(intr_debug_9),
    _DEVICE_ATTR(intr_debug_10),
    _DEVICE_ATTR(intr_debug_11),
    _DEVICE_ATTR(intr_debug_12),
    _DEVICE_ATTR(intr_debug_13),
    _DEVICE_ATTR(intr_debug_14),
    _DEVICE_ATTR(intr_debug_15),
    _DEVICE_ATTR(intr_debug_16),
    _DEVICE_ATTR(intr_debug_17),
    _DEVICE_ATTR(intr_debug_18),
    _DEVICE_ATTR(intr_debug_19),
    _DEVICE_ATTR(intr_debug_20),
    _DEVICE_ATTR(reset_0),
    _DEVICE_ATTR(reset_1),
    _DEVICE_ATTR(reset_2),
    _DEVICE_ATTR(reset_3),
    _DEVICE_ATTR(reset_4),
    _DEVICE_ATTR(reset_5),
    _DEVICE_ATTR(reset_6),
    _DEVICE_ATTR(reset_7),
    _DEVICE_ATTR(reset_8),
    _DEVICE_ATTR(reset_9),
    _DEVICE_ATTR(reset_10),
    _DEVICE_ATTR(reset_11),
    _DEVICE_ATTR(reset_12),
    _DEVICE_ATTR(reset_13),
    _DEVICE_ATTR(reset_14),
    _DEVICE_ATTR(cpld_rov_reset),
    _DEVICE_ATTR(rov_debug_enable),
    _DEVICE_ATTR(rov_debug_1),
    _DEVICE_ATTR(rov_debug_2),
    _DEVICE_ATTR(sys_status_1),
    _DEVICE_ATTR(sys_status_2),
    _DEVICE_ATTR(sys_status_3),
    _DEVICE_ATTR(sys_status_4),
    _DEVICE_ATTR(sys_status_5),
    _DEVICE_ATTR(sys_status_6),
    _DEVICE_ATTR(sys_status_7_mac_rov1),
    _DEVICE_ATTR(sys_status_8_mac_rov2),
    _DEVICE_ATTR(sys_status_9_mac_rov3),
    _DEVICE_ATTR(sys_status_10),
    _DEVICE_ATTR(sys_status_11),
    _DEVICE_ATTR(sys_status_12),
    _DEVICE_ATTR(sys_status_13),
    _DEVICE_ATTR(sys_status_14),
    _DEVICE_ATTR(sys_status_15),
    _DEVICE_ATTR(sys_status_16),
    _DEVICE_ATTR(sys_status_17),
    _DEVICE_ATTR(sys_status_18),
    _DEVICE_ATTR(sys_status_19),
    _DEVICE_ATTR(sys_status_20),
    _DEVICE_ATTR(sys_status_21),
    _DEVICE_ATTR(sys_status_22),
    _DEVICE_ATTR(sys_status_23),
    _DEVICE_ATTR(sys_status_24),
    _DEVICE_ATTR(sys_status_25),
    _DEVICE_ATTR(sys_status_26),
    _DEVICE_ATTR(sys_status_27),
    _DEVICE_ATTR(sys_status_28),
    _DEVICE_ATTR(sys_status_29),
    _DEVICE_ATTR(psu0_present),
    _DEVICE_ATTR(psu1_present),
    _DEVICE_ATTR(psu0_vin_pwok),
    _DEVICE_ATTR(psu1_vin_pwok),
    _DEVICE_ATTR(psu0_pwok),
    _DEVICE_ATTR(psu1_pwok),
    _DEVICE_ATTR(psu_type),
    _DEVICE_ATTR(boot_select),
    _DEVICE_ATTR(clk_timing_ctrl_1),
    _DEVICE_ATTR(clk_timing_ctrl_2),
    _DEVICE_ATTR(pw_sys_ctrl),
    _DEVICE_ATTR(gnss_ctrl),
    _DEVICE_ATTR(usb_ctrl),
    _DEVICE_ATTR(synce_ctrl),
    _DEVICE_ATTR(ts_pll_clock_ctrl),
    _DEVICE_ATTR(led_clear),
    _DEVICE_ATTR(sys_led_status),
    _DEVICE_ATTR(sys_led_blinking),
    _DEVICE_ATTR(sys_led_color),
    _DEVICE_ATTR(gnss_led_status),
    _DEVICE_ATTR(gnss_led_blinking),
    _DEVICE_ATTR(gnss_led_color),
    _DEVICE_ATTR(sync_led_status),
    _DEVICE_ATTR(sync_led_blinking),
    _DEVICE_ATTR(sync_led_color),
    _DEVICE_ATTR(pwr_led_status),
    _DEVICE_ATTR(pwr_led_blinking),
    _DEVICE_ATTR(pwr_led_color),
    _DEVICE_ATTR(fan_led_status),
    _DEVICE_ATTR(fan_led_blinking),
    _DEVICE_ATTR(fan_led_color),
    _DEVICE_ATTR(sys_led),
    _DEVICE_ATTR(gnss_led),
    _DEVICE_ATTR(sync_led),
    _DEVICE_ATTR(fan_led),
    _DEVICE_ATTR(pwr_led),
    _DEVICE_ATTR(cpld_test),

    _DEVICE_ATTR(bsp_debug),

    NULL
};

/* cpld 2 (DC PSU)) */
static struct attribute *cpld2_dc_psu_attributes[] =
{
    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_version_h),
    _DEVICE_ATTR(cpld_id),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_chip_type),
    _DEVICE_ATTR(cpld_i2c_upgrade_module_reset),
    _DEVICE_ATTR(event_ctrl),

    _DEVICE_ATTR(port_0_abs),
    _DEVICE_ATTR(port_1_abs),
    _DEVICE_ATTR(port_2_abs),
    _DEVICE_ATTR(port_3_abs),
    _DEVICE_ATTR(port_4_abs),
    _DEVICE_ATTR(port_5_abs),
    _DEVICE_ATTR(port_6_abs),
    _DEVICE_ATTR(port_7_abs),
    _DEVICE_ATTR(port_8_abs),
    _DEVICE_ATTR(port_9_abs),
    _DEVICE_ATTR(port_10_abs),
    _DEVICE_ATTR(port_11_abs),
    _DEVICE_ATTR(port_12_abs),
    _DEVICE_ATTR(port_13_abs),
    _DEVICE_ATTR(port_14_abs),
    _DEVICE_ATTR(port_15_abs),
    _DEVICE_ATTR(port_16_abs),
    _DEVICE_ATTR(port_17_abs),
    _DEVICE_ATTR(port_18_abs),
    _DEVICE_ATTR(port_19_abs),
    _DEVICE_ATTR(port_0_abs_mask),
    _DEVICE_ATTR(port_1_abs_mask),
    _DEVICE_ATTR(port_2_abs_mask),
    _DEVICE_ATTR(port_3_abs_mask),
    _DEVICE_ATTR(port_4_abs_mask),
    _DEVICE_ATTR(port_5_abs_mask),
    _DEVICE_ATTR(port_6_abs_mask),
    _DEVICE_ATTR(port_7_abs_mask),
    _DEVICE_ATTR(port_8_abs_mask),
    _DEVICE_ATTR(port_9_abs_mask),
    _DEVICE_ATTR(port_10_abs_mask),
    _DEVICE_ATTR(port_11_abs_mask),
    _DEVICE_ATTR(port_12_abs_mask),
    _DEVICE_ATTR(port_13_abs_mask),
    _DEVICE_ATTR(port_14_abs_mask),
    _DEVICE_ATTR(port_15_abs_mask),
    _DEVICE_ATTR(port_16_abs_mask),
    _DEVICE_ATTR(port_17_abs_mask),
    _DEVICE_ATTR(port_18_abs_mask),
    _DEVICE_ATTR(port_19_abs_mask),
    _DEVICE_ATTR(port_0_7_abs_event),
    _DEVICE_ATTR(port_8_15_abs_event),
    _DEVICE_ATTR(port_16_19_abs_event),
    _DEVICE_ATTR(port_0_abs_debug),
    _DEVICE_ATTR(port_1_abs_debug),
    _DEVICE_ATTR(port_2_abs_debug),
    _DEVICE_ATTR(port_3_abs_debug),
    _DEVICE_ATTR(port_4_abs_debug),
    _DEVICE_ATTR(port_5_abs_debug),
    _DEVICE_ATTR(port_6_abs_debug),
    _DEVICE_ATTR(port_7_abs_debug),
    _DEVICE_ATTR(port_8_abs_debug),
    _DEVICE_ATTR(port_9_abs_debug),
    _DEVICE_ATTR(port_10_abs_debug),
    _DEVICE_ATTR(port_11_abs_debug),
    _DEVICE_ATTR(port_12_abs_debug),
    _DEVICE_ATTR(port_13_abs_debug),
    _DEVICE_ATTR(port_14_abs_debug),
    _DEVICE_ATTR(port_15_abs_debug),
    _DEVICE_ATTR(port_16_abs_debug),
    _DEVICE_ATTR(port_17_abs_debug),
    _DEVICE_ATTR(port_18_abs_debug),
    _DEVICE_ATTR(port_19_abs_debug),
    _DEVICE_ATTR(port_0_rx_los),
    _DEVICE_ATTR(port_1_rx_los),
    _DEVICE_ATTR(port_2_rx_los),
    _DEVICE_ATTR(port_3_rx_los),
    _DEVICE_ATTR(port_4_rx_los),
    _DEVICE_ATTR(port_5_rx_los),
    _DEVICE_ATTR(port_6_rx_los),
    _DEVICE_ATTR(port_7_rx_los),
    _DEVICE_ATTR(port_8_rx_los),
    _DEVICE_ATTR(port_9_rx_los),
    _DEVICE_ATTR(port_10_rx_los),
    _DEVICE_ATTR(port_11_rx_los),
    _DEVICE_ATTR(port_12_rx_los),
    _DEVICE_ATTR(port_13_rx_los),
    _DEVICE_ATTR(port_14_rx_los),
    _DEVICE_ATTR(port_15_rx_los),
    _DEVICE_ATTR(port_16_rx_los),
    _DEVICE_ATTR(port_17_rx_los),
    _DEVICE_ATTR(port_18_rx_los),
    _DEVICE_ATTR(port_19_rx_los),
    _DEVICE_ATTR(port_0_rx_los_mask),
    _DEVICE_ATTR(port_1_rx_los_mask),
    _DEVICE_ATTR(port_2_rx_los_mask),
    _DEVICE_ATTR(port_3_rx_los_mask),
    _DEVICE_ATTR(port_4_rx_los_mask),
    _DEVICE_ATTR(port_5_rx_los_mask),
    _DEVICE_ATTR(port_6_rx_los_mask),
    _DEVICE_ATTR(port_7_rx_los_mask),
    _DEVICE_ATTR(port_8_rx_los_mask),
    _DEVICE_ATTR(port_9_rx_los_mask),
    _DEVICE_ATTR(port_10_rx_los_mask),
    _DEVICE_ATTR(port_11_rx_los_mask),
    _DEVICE_ATTR(port_12_rx_los_mask),
    _DEVICE_ATTR(port_13_rx_los_mask),
    _DEVICE_ATTR(port_14_rx_los_mask),
    _DEVICE_ATTR(port_15_rx_los_mask),
    _DEVICE_ATTR(port_16_rx_los_mask),
    _DEVICE_ATTR(port_17_rx_los_mask),
    _DEVICE_ATTR(port_18_rx_los_mask),
    _DEVICE_ATTR(port_19_rx_los_mask),
    _DEVICE_ATTR(port_0_7_rx_los_event),
    _DEVICE_ATTR(port_8_15_rx_los_event),
    _DEVICE_ATTR(port_16_19_rx_los_event),
    _DEVICE_ATTR(port_0_rx_los_debug),
    _DEVICE_ATTR(port_1_rx_los_debug),
    _DEVICE_ATTR(port_2_rx_los_debug),
    _DEVICE_ATTR(port_3_rx_los_debug),
    _DEVICE_ATTR(port_4_rx_los_debug),
    _DEVICE_ATTR(port_5_rx_los_debug),
    _DEVICE_ATTR(port_6_rx_los_debug),
    _DEVICE_ATTR(port_7_rx_los_debug),
    _DEVICE_ATTR(port_8_rx_los_debug),
    _DEVICE_ATTR(port_9_rx_los_debug),
    _DEVICE_ATTR(port_10_rx_los_debug),
    _DEVICE_ATTR(port_11_rx_los_debug),
    _DEVICE_ATTR(port_12_rx_los_debug),
    _DEVICE_ATTR(port_13_rx_los_debug),
    _DEVICE_ATTR(port_14_rx_los_debug),
    _DEVICE_ATTR(port_15_rx_los_debug),
    _DEVICE_ATTR(port_16_rx_los_debug),
    _DEVICE_ATTR(port_17_rx_los_debug),
    _DEVICE_ATTR(port_18_rx_los_debug),
    _DEVICE_ATTR(port_19_rx_los_debug),
    _DEVICE_ATTR(port_0_tx_fault),
    _DEVICE_ATTR(port_1_tx_fault),
    _DEVICE_ATTR(port_2_tx_fault),
    _DEVICE_ATTR(port_3_tx_fault),
    _DEVICE_ATTR(port_4_tx_fault),
    _DEVICE_ATTR(port_5_tx_fault),
    _DEVICE_ATTR(port_6_tx_fault),
    _DEVICE_ATTR(port_7_tx_fault),
    _DEVICE_ATTR(port_8_tx_fault),
    _DEVICE_ATTR(port_9_tx_fault),
    _DEVICE_ATTR(port_10_tx_fault),
    _DEVICE_ATTR(port_11_tx_fault),
    _DEVICE_ATTR(port_12_tx_fault),
    _DEVICE_ATTR(port_13_tx_fault),
    _DEVICE_ATTR(port_14_tx_fault),
    _DEVICE_ATTR(port_15_tx_fault),
    _DEVICE_ATTR(port_16_tx_fault),
    _DEVICE_ATTR(port_17_tx_fault),
    _DEVICE_ATTR(port_18_tx_fault),
    _DEVICE_ATTR(port_19_tx_fault),
    _DEVICE_ATTR(port_0_tx_fault_mask),
    _DEVICE_ATTR(port_1_tx_fault_mask),
    _DEVICE_ATTR(port_2_tx_fault_mask),
    _DEVICE_ATTR(port_3_tx_fault_mask),
    _DEVICE_ATTR(port_4_tx_fault_mask),
    _DEVICE_ATTR(port_5_tx_fault_mask),
    _DEVICE_ATTR(port_6_tx_fault_mask),
    _DEVICE_ATTR(port_7_tx_fault_mask),
    _DEVICE_ATTR(port_8_tx_fault_mask),
    _DEVICE_ATTR(port_9_tx_fault_mask),
    _DEVICE_ATTR(port_10_tx_fault_mask),
    _DEVICE_ATTR(port_11_tx_fault_mask),
    _DEVICE_ATTR(port_12_tx_fault_mask),
    _DEVICE_ATTR(port_13_tx_fault_mask),
    _DEVICE_ATTR(port_14_tx_fault_mask),
    _DEVICE_ATTR(port_15_tx_fault_mask),
    _DEVICE_ATTR(port_16_tx_fault_mask),
    _DEVICE_ATTR(port_17_tx_fault_mask),
    _DEVICE_ATTR(port_18_tx_fault_mask),
    _DEVICE_ATTR(port_19_tx_fault_mask),
    _DEVICE_ATTR(port_0_7_tx_fault_event),
    _DEVICE_ATTR(port_8_15_tx_fault_event),
    _DEVICE_ATTR(port_16_19_tx_fault_event),
    _DEVICE_ATTR(port_0_tx_fault_debug),
    _DEVICE_ATTR(port_1_tx_fault_debug),
    _DEVICE_ATTR(port_2_tx_fault_debug),
    _DEVICE_ATTR(port_3_tx_fault_debug),
    _DEVICE_ATTR(port_4_tx_fault_debug),
    _DEVICE_ATTR(port_5_tx_fault_debug),
    _DEVICE_ATTR(port_6_tx_fault_debug),
    _DEVICE_ATTR(port_7_tx_fault_debug),
    _DEVICE_ATTR(port_8_tx_fault_debug),
    _DEVICE_ATTR(port_9_tx_fault_debug),
    _DEVICE_ATTR(port_10_tx_fault_debug),
    _DEVICE_ATTR(port_11_tx_fault_debug),
    _DEVICE_ATTR(port_12_tx_fault_debug),
    _DEVICE_ATTR(port_13_tx_fault_debug),
    _DEVICE_ATTR(port_14_tx_fault_debug),
    _DEVICE_ATTR(port_15_tx_fault_debug),
    _DEVICE_ATTR(port_16_tx_fault_debug),
    _DEVICE_ATTR(port_17_tx_fault_debug),
    _DEVICE_ATTR(port_18_tx_fault_debug),
    _DEVICE_ATTR(port_19_tx_fault_debug),
    _DEVICE_ATTR(port_0_tx_disable),
    _DEVICE_ATTR(port_1_tx_disable),
    _DEVICE_ATTR(port_2_tx_disable),
    _DEVICE_ATTR(port_3_tx_disable),
    _DEVICE_ATTR(port_4_tx_disable),
    _DEVICE_ATTR(port_5_tx_disable),
    _DEVICE_ATTR(port_6_tx_disable),
    _DEVICE_ATTR(port_7_tx_disable),
    _DEVICE_ATTR(port_8_tx_disable),
    _DEVICE_ATTR(port_9_tx_disable),
    _DEVICE_ATTR(port_10_tx_disable),
    _DEVICE_ATTR(port_11_tx_disable),
    _DEVICE_ATTR(port_12_tx_disable),
    _DEVICE_ATTR(port_13_tx_disable),
    _DEVICE_ATTR(port_14_tx_disable),
    _DEVICE_ATTR(port_15_tx_disable),
    _DEVICE_ATTR(port_16_tx_disable),
    _DEVICE_ATTR(port_17_tx_disable),
    _DEVICE_ATTR(port_18_tx_disable),
    _DEVICE_ATTR(port_19_tx_disable),
    _DEVICE_ATTR(port_0_rate_sel),
    _DEVICE_ATTR(port_1_rate_sel),
    _DEVICE_ATTR(port_2_rate_sel),
    _DEVICE_ATTR(port_3_rate_sel),
    _DEVICE_ATTR(port_4_rate_sel),
    _DEVICE_ATTR(port_5_rate_sel),
    _DEVICE_ATTR(port_6_rate_sel),
    _DEVICE_ATTR(port_7_rate_sel),
    _DEVICE_ATTR(port_8_rate_sel),
    _DEVICE_ATTR(port_9_rate_sel),
    _DEVICE_ATTR(port_10_rate_sel),
    _DEVICE_ATTR(port_11_rate_sel),
    _DEVICE_ATTR(port_12_rate_sel),
    _DEVICE_ATTR(port_13_rate_sel),
    _DEVICE_ATTR(port_14_rate_sel),
    _DEVICE_ATTR(port_15_rate_sel),
    _DEVICE_ATTR(port_16_rate_sel),
    _DEVICE_ATTR(port_17_rate_sel),
    _DEVICE_ATTR(port_18_rate_sel),
    _DEVICE_ATTR(port_19_rate_sel),
    _DEVICE_ATTR(port_0_pwr_en),
    _DEVICE_ATTR(port_1_pwr_en),
    _DEVICE_ATTR(port_2_pwr_en),
    _DEVICE_ATTR(port_3_pwr_en),
    _DEVICE_ATTR(port_4_pwr_en),
    _DEVICE_ATTR(port_5_pwr_en),
    _DEVICE_ATTR(port_6_pwr_en),
    _DEVICE_ATTR(port_7_pwr_en),
    _DEVICE_ATTR(port_8_pwr_en),
    _DEVICE_ATTR(port_9_pwr_en),
    _DEVICE_ATTR(port_10_pwr_en),
    _DEVICE_ATTR(port_11_pwr_en),
    _DEVICE_ATTR(port_12_pwr_en),
    _DEVICE_ATTR(port_13_pwr_en),
    _DEVICE_ATTR(port_14_pwr_en),
    _DEVICE_ATTR(port_15_pwr_en),
    _DEVICE_ATTR(port_16_pwr_en),
    _DEVICE_ATTR(port_17_pwr_en),
    _DEVICE_ATTR(port_18_pwr_en),
    _DEVICE_ATTR(port_19_pwr_en),
    //_DEVICE_ATTR(psu_0_vin_pwok),
    //_DEVICE_ATTR(psu_1_vin_pwok),
    //_DEVICE_ATTR(psu_0_vout_pwok),
    //_DEVICE_ATTR(psu_1_vout_pwok),
    _DEVICE_ATTR(intr_fan_0),
    _DEVICE_ATTR(intr_fan_1),
    _DEVICE_ATTR(intr_fan_2),
    _DEVICE_ATTR(intr_fan_3),
    _DEVICE_ATTR(fan_0_pwm_rpm_h),
    _DEVICE_ATTR(fan_0_pwm_rpm_l),
    _DEVICE_ATTR(fan_0_pwm_rpm),
    _DEVICE_ATTR(fan_1_pwm_rpm_h),
    _DEVICE_ATTR(fan_1_pwm_rpm_l),
    _DEVICE_ATTR(fan_1_pwm_rpm),
    _DEVICE_ATTR(fan_2_pwm_rpm_h),
    _DEVICE_ATTR(fan_2_pwm_rpm_l),
    _DEVICE_ATTR(fan_2_pwm_rpm),
    _DEVICE_ATTR(fan_3_pwm_rpm_h),
    _DEVICE_ATTR(fan_3_pwm_rpm_l),
    _DEVICE_ATTR(fan_3_pwm_rpm),
    _DEVICE_ATTR(fan_pwm_mode_1),
    _DEVICE_ATTR(fan_pwm_mode_2),
    _DEVICE_ATTR(fan_pwm_diag_ctrl_1),
    _DEVICE_ATTR(fan_pwm_diag_ctrl_2),
    _DEVICE_ATTR(vol_1_value),
    _DEVICE_ATTR(vol_2_value),
    _DEVICE_ATTR(vol_3_value),
    _DEVICE_ATTR(vol_4_value),
    _DEVICE_ATTR(vol_5_value),
    _DEVICE_ATTR(vol_6_value),
    _DEVICE_ATTR(vol_7_value),
    _DEVICE_ATTR(vol_8_value),
    _DEVICE_ATTR(vol_9_value),
    _DEVICE_ATTR(vol_10_value),
    _DEVICE_ATTR(vol_11_value),
    _DEVICE_ATTR(vol_12_value),
    _DEVICE_ATTR(vol_13_value),
    _DEVICE_ATTR(vol_14_value),
    _DEVICE_ATTR(vol_15_value),
    _DEVICE_ATTR(vol_16_value),

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

/* cpld 1 (dc psu) attributes group */
static const struct attribute_group cpld1_dc_psu_group = {
    .attrs = cpld1_dc_psu_attributes,
};

/* cpld 2 (dc psu) attributes group */
static const struct attribute_group cpld2_dc_psu_group = {
    .attrs = cpld2_dc_psu_attributes,
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

static int _parse_data(char *buf, unsigned int data, u8 data_type)
{
    if (buf == NULL) {
        return -EINVAL;
    }

    if (data_type == DATA_HEX)
        return sprintf(buf, "0x%02x", data);
    else if (data_type == DATA_DEC)
        return sprintf(buf, "%u", data);
    else
        return -EINVAL;
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

            if (kstrtou8(buf, 0, &bsp_debug_u8) < 0)
                return -EINVAL;
            else if (_config_bsp_log(bsp_debug_u8) < 0)
                return -EINVAL;
            return ret;
        default:
            return -EINVAL;
    }
    return 0;
}

/* (read) get cpld register value */
static ssize_t cpld_show(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg = 0;
    u8 mask = MASK_NONE;
    u8 data_type=DATA_UNK;

    switch(attr->index){
        /********************************************** CPLD 1&2 common registers *********************************************/
        case CPLD_MAJOR_VER:
        case CPLD_MINOR_VER:
        case CPLD_ID:
        case CPLD_BUILD_VER:
        case CPLD_CHIP_TYPE:
        case CPLD_I2C_UPGRADE_MODULE_RESET:
        case EVENT_CTRL:

        /******************************************************* CPLD 1 *******************************************************/
        case SKU_ID:
        case HW_REV:
        case DEPH_ID:
        case BUILD_ID:
        case BIT_SEL_ID:
        case EXT_ID:
        case EXTEND_ID:
        case INTR_0:
        case INTR_1:
        case INTR_2:
        case INTR_3:
        case INTR_4:
        case INTR_5:
        case INTR_6:
        case INTR_7:
        case INTR_8:
        case INTR_9:
        case INTR_10:
        case INTR_11:
        case INTR_12:
        case INTR_13:
        case INTR_14:
        case INTR_15:
        case INTR_16:
        case INTR_17:
        case INTR_18:
        case INTR_19:
        case INTR_20:
        case INTR_21:
        case INTR_22:
        case INTR_23:
        case INTR_24:
        case INTR_25:
        case INTR_26:
        case INTR_27:
        case INTR_MASK_0:
        case INTR_MASK_1:
        case INTR_MASK_2:
        case INTR_MASK_3:
        case INTR_MASK_4:
        case INTR_MASK_5:
        case INTR_MASK_6:
        case INTR_MASK_7:
        case INTR_MASK_8:
        case INTR_MASK_9:
        case INTR_MASK_10:
        case INTR_MASK_11:
        case INTR_MASK_12:
        case INTR_MASK_13:
        case INTR_MASK_14:
        case INTR_MASK_15:
        case INTR_MASK_16:
        case INTR_MASK_17:
        case INTR_MASK_18:
        case INTR_MASK_19:
        case INTR_MASK_20:
        case INTR_MASK_21:
        case INTR_MASK_22:
        case INTR_MASK_23:
        case INTR_MASK_24:
        case INTR_MASK_25:
        case INTR_EVENT_CLK_PTP:
        case INTR_EVENT_PSU:
        case INTR_EVENT_HWM:
        case INTR_EVENT_THERMAL:
        case INTR_EVENT_FAN:
        case INTR_EVENT_ETH:
        case INTR_DEBUG_1:
        case INTR_DEBUG_2:
        case INTR_DEBUG_3:
        case INTR_DEBUG_4:
        case INTR_DEBUG_5:
        case INTR_DEBUG_6:
        case INTR_DEBUG_7:
        case INTR_DEBUG_8:
        case INTR_DEBUG_9:
        case INTR_DEBUG_10:
        case INTR_DEBUG_11:
        case INTR_DEBUG_12:
        case INTR_DEBUG_13:
        case INTR_DEBUG_14:
        case INTR_DEBUG_15:
        case INTR_DEBUG_16:
        case INTR_DEBUG_17:
        case INTR_DEBUG_18:
        case INTR_DEBUG_19:
        case INTR_DEBUG_20:
        case RESET_0:
        case RESET_1:
        case RESET_2:
        case RESET_3:
        case RESET_4:
        case RESET_5:
        case RESET_6:
        case RESET_7:
        case RESET_8:
        case RESET_9:
        case RESET_10:
        case RESET_11:
        case RESET_12:
        case RESET_13:
        case RESET_14:
        case CPLD_ROV_RESET:
        case ROV_DEBUG_ENABLE:
        case ROV_DEBUG_1:
        case ROV_DEBUG_2:
        case SYS_STATUS_1:
        case SYS_STATUS_2:
        case SYS_STATUS_3:
        case SYS_STATUS_4:
        case SYS_STATUS_5:
        case SYS_STATUS_6:
        case SYS_STATUS_7_MAC_ROV1:
        case SYS_STATUS_8_MAC_ROV2:
        case SYS_STATUS_9_MAC_ROV3:
        case SYS_STATUS_10:
        case SYS_STATUS_11:
        case SYS_STATUS_12:
        case SYS_STATUS_13:
        case SYS_STATUS_14:
        case SYS_STATUS_15:
        case SYS_STATUS_16:
        case SYS_STATUS_17:
        case SYS_STATUS_18:
        case SYS_STATUS_19:
        case SYS_STATUS_20:
        case SYS_STATUS_21:
        case SYS_STATUS_22:
        case SYS_STATUS_23:
        case SYS_STATUS_24:
        case SYS_STATUS_25:
        case SYS_STATUS_26:
        case SYS_STATUS_27:
        case SYS_STATUS_28:
        case SYS_STATUS_29:
        case PSU0_PRESENT:
        case PSU1_PRESENT:
        case PSU0_VIN_PWOK:
        case PSU1_VIN_PWOK:
        case PSU0_VOUT_PWOK:
        case PSU1_VOUT_PWOK:
        case PSU_TYPE:
        case BOOT_SELECT:
        case CLK_TIMING_CTRL_1:
        case CLK_TIMING_CTRL_2:
        case PW_SYS_CTRL:
        case GNSS_CTRL:
        case USB_CTRL:
        case SYNCE_CTRL:
        case TS_PLL_CLOCK_CTRL:
        case LED_CLEAR:
        case SYS_LED_STATUS:
        case SYS_LED_BLINKING:
        case SYS_LED_COLOR:
        case GNSS_LED_STATUS:
        case GNSS_LED_BLINKING:
        case GNSS_LED_COLOR:
        case SYNC_LED_STATUS:
        case SYNC_LED_BLINKING:
        case SYNC_LED_COLOR:
        case PWR_LED_STATUS:
        case PWR_LED_BLINKING:
        case PWR_LED_COLOR:
        case FAN_LED_STATUS:
        case FAN_LED_BLINKING:
        case FAN_LED_COLOR:
        case SYS_LED:
        case GNSS_LED:
        case SYNC_LED:
        case FAN_LED:
        case PWR_LED:
        case CPLD_TEST:
        /******************************************************* CPLD 2 *******************************************************/
        case PORT_0_ABS:
        case PORT_1_ABS:
        case PORT_2_ABS:
        case PORT_3_ABS:
        case PORT_4_ABS:
        case PORT_5_ABS:
        case PORT_6_ABS:
        case PORT_7_ABS:
        case PORT_8_ABS:
        case PORT_9_ABS:
        case PORT_10_ABS:
        case PORT_11_ABS:
        case PORT_12_ABS:
        case PORT_13_ABS:
        case PORT_14_ABS:
        case PORT_15_ABS:
        case PORT_16_ABS:
        case PORT_17_ABS:
        case PORT_18_ABS:
        case PORT_19_ABS:
        case PORT_0_ABS_MASK:
        case PORT_1_ABS_MASK:
        case PORT_2_ABS_MASK:
        case PORT_3_ABS_MASK:
        case PORT_4_ABS_MASK:
        case PORT_5_ABS_MASK:
        case PORT_6_ABS_MASK:
        case PORT_7_ABS_MASK:
        case PORT_8_ABS_MASK:
        case PORT_9_ABS_MASK:
        case PORT_10_ABS_MASK:
        case PORT_11_ABS_MASK:
        case PORT_12_ABS_MASK:
        case PORT_13_ABS_MASK:
        case PORT_14_ABS_MASK:
        case PORT_15_ABS_MASK:
        case PORT_16_ABS_MASK:
        case PORT_17_ABS_MASK:
        case PORT_18_ABS_MASK:
        case PORT_19_ABS_MASK:
        case PORT_0_7_ABS_EVENT:
        case PORT_8_15_ABS_EVENT:
        case PORT_16_19_ABS_EVENT:
        case PORT_0_ABS_DEBUG:
        case PORT_1_ABS_DEBUG:
        case PORT_2_ABS_DEBUG:
        case PORT_3_ABS_DEBUG:
        case PORT_4_ABS_DEBUG:
        case PORT_5_ABS_DEBUG:
        case PORT_6_ABS_DEBUG:
        case PORT_7_ABS_DEBUG:
        case PORT_8_ABS_DEBUG:
        case PORT_9_ABS_DEBUG:
        case PORT_10_ABS_DEBUG:
        case PORT_11_ABS_DEBUG:
        case PORT_12_ABS_DEBUG:
        case PORT_13_ABS_DEBUG:
        case PORT_14_ABS_DEBUG:
        case PORT_15_ABS_DEBUG:
        case PORT_16_ABS_DEBUG:
        case PORT_17_ABS_DEBUG:
        case PORT_18_ABS_DEBUG:
        case PORT_19_ABS_DEBUG:
        case PORT_0_RX_LOS:
        case PORT_1_RX_LOS:
        case PORT_2_RX_LOS:
        case PORT_3_RX_LOS:
        case PORT_4_RX_LOS:
        case PORT_5_RX_LOS:
        case PORT_6_RX_LOS:
        case PORT_7_RX_LOS:
        case PORT_8_RX_LOS:
        case PORT_9_RX_LOS:
        case PORT_10_RX_LOS:
        case PORT_11_RX_LOS:
        case PORT_12_RX_LOS:
        case PORT_13_RX_LOS:
        case PORT_14_RX_LOS:
        case PORT_15_RX_LOS:
        case PORT_16_RX_LOS:
        case PORT_17_RX_LOS:
        case PORT_18_RX_LOS:
        case PORT_19_RX_LOS:
        case PORT_0_RX_LOS_MASK:
        case PORT_1_RX_LOS_MASK:
        case PORT_2_RX_LOS_MASK:
        case PORT_3_RX_LOS_MASK:
        case PORT_4_RX_LOS_MASK:
        case PORT_5_RX_LOS_MASK:
        case PORT_6_RX_LOS_MASK:
        case PORT_7_RX_LOS_MASK:
        case PORT_8_RX_LOS_MASK:
        case PORT_9_RX_LOS_MASK:
        case PORT_10_RX_LOS_MASK:
        case PORT_11_RX_LOS_MASK:
        case PORT_12_RX_LOS_MASK:
        case PORT_13_RX_LOS_MASK:
        case PORT_14_RX_LOS_MASK:
        case PORT_15_RX_LOS_MASK:
        case PORT_16_RX_LOS_MASK:
        case PORT_17_RX_LOS_MASK:
        case PORT_18_RX_LOS_MASK:
        case PORT_19_RX_LOS_MASK:
        case PORT_0_7_RX_LOS_EVENT:
        case PORT_8_15_RX_LOS_EVENT:
        case PORT_16_19_RX_LOS_EVENT:
        case PORT_0_RX_LOS_DEBUG:
        case PORT_1_RX_LOS_DEBUG:
        case PORT_2_RX_LOS_DEBUG:
        case PORT_3_RX_LOS_DEBUG:
        case PORT_4_RX_LOS_DEBUG:
        case PORT_5_RX_LOS_DEBUG:
        case PORT_6_RX_LOS_DEBUG:
        case PORT_7_RX_LOS_DEBUG:
        case PORT_8_RX_LOS_DEBUG:
        case PORT_9_RX_LOS_DEBUG:
        case PORT_10_RX_LOS_DEBUG:
        case PORT_11_RX_LOS_DEBUG:
        case PORT_12_RX_LOS_DEBUG:
        case PORT_13_RX_LOS_DEBUG:
        case PORT_14_RX_LOS_DEBUG:
        case PORT_15_RX_LOS_DEBUG:
        case PORT_16_RX_LOS_DEBUG:
        case PORT_17_RX_LOS_DEBUG:
        case PORT_18_RX_LOS_DEBUG:
        case PORT_19_RX_LOS_DEBUG:
        case PORT_0_TX_FAULT:
        case PORT_1_TX_FAULT:
        case PORT_2_TX_FAULT:
        case PORT_3_TX_FAULT:
        case PORT_4_TX_FAULT:
        case PORT_5_TX_FAULT:
        case PORT_6_TX_FAULT:
        case PORT_7_TX_FAULT:
        case PORT_8_TX_FAULT:
        case PORT_9_TX_FAULT:
        case PORT_10_TX_FAULT:
        case PORT_11_TX_FAULT:
        case PORT_12_TX_FAULT:
        case PORT_13_TX_FAULT:
        case PORT_14_TX_FAULT:
        case PORT_15_TX_FAULT:
        case PORT_16_TX_FAULT:
        case PORT_17_TX_FAULT:
        case PORT_18_TX_FAULT:
        case PORT_19_TX_FAULT:
        case PORT_0_TX_FAULT_MASK:
        case PORT_1_TX_FAULT_MASK:
        case PORT_2_TX_FAULT_MASK:
        case PORT_3_TX_FAULT_MASK:
        case PORT_4_TX_FAULT_MASK:
        case PORT_5_TX_FAULT_MASK:
        case PORT_6_TX_FAULT_MASK:
        case PORT_7_TX_FAULT_MASK:
        case PORT_8_TX_FAULT_MASK:
        case PORT_9_TX_FAULT_MASK:
        case PORT_10_TX_FAULT_MASK:
        case PORT_11_TX_FAULT_MASK:
        case PORT_12_TX_FAULT_MASK:
        case PORT_13_TX_FAULT_MASK:
        case PORT_14_TX_FAULT_MASK:
        case PORT_15_TX_FAULT_MASK:
        case PORT_16_TX_FAULT_MASK:
        case PORT_17_TX_FAULT_MASK:
        case PORT_18_TX_FAULT_MASK:
        case PORT_19_TX_FAULT_MASK:
        case PORT_0_7_TX_FAULT_EVENT:
        case PORT_8_15_TX_FAULT_EVENT:
        case PORT_16_19_TX_FAULT_EVENT:
        case PORT_0_TX_FAULT_DEBUG:
        case PORT_1_TX_FAULT_DEBUG:
        case PORT_2_TX_FAULT_DEBUG:
        case PORT_3_TX_FAULT_DEBUG:
        case PORT_4_TX_FAULT_DEBUG:
        case PORT_5_TX_FAULT_DEBUG:
        case PORT_6_TX_FAULT_DEBUG:
        case PORT_7_TX_FAULT_DEBUG:
        case PORT_8_TX_FAULT_DEBUG:
        case PORT_9_TX_FAULT_DEBUG:
        case PORT_10_TX_FAULT_DEBUG:
        case PORT_11_TX_FAULT_DEBUG:
        case PORT_12_TX_FAULT_DEBUG:
        case PORT_13_TX_FAULT_DEBUG:
        case PORT_14_TX_FAULT_DEBUG:
        case PORT_15_TX_FAULT_DEBUG:
        case PORT_16_TX_FAULT_DEBUG:
        case PORT_17_TX_FAULT_DEBUG:
        case PORT_18_TX_FAULT_DEBUG:
        case PORT_19_TX_FAULT_DEBUG:
        case PORT_0_TX_DISABLE:
        case PORT_1_TX_DISABLE:
        case PORT_2_TX_DISABLE:
        case PORT_3_TX_DISABLE:
        case PORT_4_TX_DISABLE:
        case PORT_5_TX_DISABLE:
        case PORT_6_TX_DISABLE:
        case PORT_7_TX_DISABLE:
        case PORT_8_TX_DISABLE:
        case PORT_9_TX_DISABLE:
        case PORT_10_TX_DISABLE:
        case PORT_11_TX_DISABLE:
        case PORT_12_TX_DISABLE:
        case PORT_13_TX_DISABLE:
        case PORT_14_TX_DISABLE:
        case PORT_15_TX_DISABLE:
        case PORT_16_TX_DISABLE:
        case PORT_17_TX_DISABLE:
        case PORT_18_TX_DISABLE:
        case PORT_19_TX_DISABLE:
        case PORT_0_RATE_SEL:
        case PORT_1_RATE_SEL:
        case PORT_2_RATE_SEL:
        case PORT_3_RATE_SEL:
        case PORT_4_RATE_SEL:
        case PORT_5_RATE_SEL:
        case PORT_6_RATE_SEL:
        case PORT_7_RATE_SEL:
        case PORT_8_RATE_SEL:
        case PORT_9_RATE_SEL:
        case PORT_10_RATE_SEL:
        case PORT_11_RATE_SEL:
        case PORT_12_RATE_SEL:
        case PORT_13_RATE_SEL:
        case PORT_14_RATE_SEL:
        case PORT_15_RATE_SEL:
        case PORT_16_RATE_SEL:
        case PORT_17_RATE_SEL:
        case PORT_18_RATE_SEL:
        case PORT_19_RATE_SEL:
        case PORT_0_PWR_EN:
        case PORT_1_PWR_EN:
        case PORT_2_PWR_EN:
        case PORT_3_PWR_EN:
        case PORT_4_PWR_EN:
        case PORT_5_PWR_EN:
        case PORT_6_PWR_EN:
        case PORT_7_PWR_EN:
        case PORT_8_PWR_EN:
        case PORT_9_PWR_EN:
        case PORT_10_PWR_EN:
        case PORT_11_PWR_EN:
        case PORT_12_PWR_EN:
        case PORT_13_PWR_EN:
        case PORT_14_PWR_EN:
        case PORT_15_PWR_EN:
        case PORT_16_PWR_EN:
        case PORT_17_PWR_EN:
        case PORT_18_PWR_EN:
        case PORT_19_PWR_EN:
        case PSU_0_VIN_PWOK:
        case PSU_1_VIN_PWOK:
        case PSU_0_VOUT_PWOK:
        case PSU_1_VOUT_PWOK:
        case INTR_FAN_0:
        case INTR_FAN_1:
        case INTR_FAN_2:
        case INTR_FAN_3:
        case FAN_0_PWM_RPM_H:
        case FAN_0_PWM_RPM_L:
        case FAN_1_PWM_RPM_H:
        case FAN_1_PWM_RPM_L:
        case FAN_2_PWM_RPM_H:
        case FAN_2_PWM_RPM_L:
        case FAN_3_PWM_RPM_H:
        case FAN_3_PWM_RPM_L:
        case FAN_PWM_MODE_1:
        case FAN_PWM_MODE_2:
        case FAN_PWM_DIAG_CTRL_1:
        case FAN_PWM_DIAG_CTRL_2:
        case VOL_1_VALUE:
        case VOL_2_VALUE:
        case VOL_3_VALUE:
        case VOL_4_VALUE:
        case VOL_5_VALUE:
        case VOL_6_VALUE:
        case VOL_7_VALUE:
        case VOL_8_VALUE:
        case VOL_9_VALUE:
        case VOL_10_VALUE:
        case VOL_11_VALUE:
        case VOL_12_VALUE:
        case VOL_13_VALUE:
        case VOL_14_VALUE:
        case VOL_15_VALUE:
        case VOL_16_VALUE:
            reg = attr_reg[attr->index].reg;
            mask= attr_reg[attr->index].mask;
            data_type = attr_reg[attr->index].data_type;
            break;
        default:
            return -EINVAL;
    }
    return cpld_reg_read(dev, buf, reg, mask, data_type);
}


/* (write) set cpld register value */
static ssize_t cpld_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg = 0;
    u8 mask = MASK_NONE;

    switch(attr->index) {
        /********************************************** CPLD 1&2 common registers *********************************************/
        case CPLD_I2C_UPGRADE_MODULE_RESET:
        case EVENT_CTRL:
        /******************************************************* CPLD 1 *******************************************************/
        case INTR_MASK_0:
        case INTR_MASK_1:
        case INTR_MASK_2:
        case INTR_MASK_3:
        case INTR_MASK_4:
        case INTR_MASK_5:
        case INTR_MASK_6:
        case INTR_MASK_7:
        case INTR_MASK_8:
        case INTR_MASK_9:
        case INTR_MASK_10:
        case INTR_MASK_11:
        case INTR_MASK_12:
        case INTR_MASK_13:
        case INTR_MASK_14:
        case INTR_MASK_15:
        case INTR_MASK_16:
        case INTR_MASK_17:
        case INTR_MASK_18:
        case INTR_MASK_19:
        case INTR_MASK_20:
        case INTR_MASK_21:
        case INTR_MASK_22:
        case INTR_MASK_23:
        case INTR_MASK_24:
        case INTR_MASK_25:
        case INTR_DEBUG_1:
        case INTR_DEBUG_2:
        case INTR_DEBUG_3:
        case INTR_DEBUG_4:
        case INTR_DEBUG_5:
        case INTR_DEBUG_6:
        case INTR_DEBUG_7:
        case INTR_DEBUG_8:
        case INTR_DEBUG_9:
        case INTR_DEBUG_10:
        case INTR_DEBUG_11:
        case INTR_DEBUG_12:
        case INTR_DEBUG_13:
        case INTR_DEBUG_14:
        case INTR_DEBUG_15:
        case INTR_DEBUG_16:
        case INTR_DEBUG_17:
        case INTR_DEBUG_18:
        case INTR_DEBUG_19:
        case INTR_DEBUG_20:
        case RESET_0:
        case RESET_1:
        case RESET_2:
        case RESET_3:
        case RESET_4:
        case RESET_5:
        case RESET_6:
        case RESET_7:
        case RESET_8:
        case RESET_9:
        case RESET_10:
        case RESET_11:
        case RESET_12:
        case RESET_13:
        case RESET_14:
        case CPLD_ROV_RESET:
        case ROV_DEBUG_ENABLE:
        case ROV_DEBUG_1:
        case ROV_DEBUG_2:
        case BOOT_SELECT:
        case CLK_TIMING_CTRL_1:
        case CLK_TIMING_CTRL_2:
        case PW_SYS_CTRL:
        case GNSS_CTRL:
        case USB_CTRL:
        case SYNCE_CTRL:
        case TS_PLL_CLOCK_CTRL:
        case LED_CLEAR:
        case SYS_LED_STATUS:
        case SYS_LED_BLINKING:
        case SYS_LED_COLOR:
        case GNSS_LED_STATUS:
        case GNSS_LED_BLINKING:
        case GNSS_LED_COLOR:
        case SYNC_LED_STATUS:
        case SYNC_LED_BLINKING:
        case SYNC_LED_COLOR:
        case SYS_LED:
        case GNSS_LED:
        case SYNC_LED:
        case CPLD_TEST:
        /******************************************************* CPLD 2 *******************************************************/
        case PORT_0_ABS_MASK:
        case PORT_1_ABS_MASK:
        case PORT_2_ABS_MASK:
        case PORT_3_ABS_MASK:
        case PORT_4_ABS_MASK:
        case PORT_5_ABS_MASK:
        case PORT_6_ABS_MASK:
        case PORT_7_ABS_MASK:
        case PORT_8_ABS_MASK:
        case PORT_9_ABS_MASK:
        case PORT_10_ABS_MASK:
        case PORT_11_ABS_MASK:
        case PORT_12_ABS_MASK:
        case PORT_13_ABS_MASK:
        case PORT_14_ABS_MASK:
        case PORT_15_ABS_MASK:
        case PORT_16_ABS_MASK:
        case PORT_17_ABS_MASK:
        case PORT_18_ABS_MASK:
        case PORT_19_ABS_MASK:
        case PORT_0_ABS_DEBUG:
        case PORT_1_ABS_DEBUG:
        case PORT_2_ABS_DEBUG:
        case PORT_3_ABS_DEBUG:
        case PORT_4_ABS_DEBUG:
        case PORT_5_ABS_DEBUG:
        case PORT_6_ABS_DEBUG:
        case PORT_7_ABS_DEBUG:
        case PORT_8_ABS_DEBUG:
        case PORT_9_ABS_DEBUG:
        case PORT_10_ABS_DEBUG:
        case PORT_11_ABS_DEBUG:
        case PORT_12_ABS_DEBUG:
        case PORT_13_ABS_DEBUG:
        case PORT_14_ABS_DEBUG:
        case PORT_15_ABS_DEBUG:
        case PORT_16_ABS_DEBUG:
        case PORT_17_ABS_DEBUG:
        case PORT_18_ABS_DEBUG:
        case PORT_19_ABS_DEBUG:
        case PORT_0_RX_LOS_MASK:
        case PORT_1_RX_LOS_MASK:
        case PORT_2_RX_LOS_MASK:
        case PORT_3_RX_LOS_MASK:
        case PORT_4_RX_LOS_MASK:
        case PORT_5_RX_LOS_MASK:
        case PORT_6_RX_LOS_MASK:
        case PORT_7_RX_LOS_MASK:
        case PORT_8_RX_LOS_MASK:
        case PORT_9_RX_LOS_MASK:
        case PORT_10_RX_LOS_MASK:
        case PORT_11_RX_LOS_MASK:
        case PORT_12_RX_LOS_MASK:
        case PORT_13_RX_LOS_MASK:
        case PORT_14_RX_LOS_MASK:
        case PORT_15_RX_LOS_MASK:
        case PORT_16_RX_LOS_MASK:
        case PORT_17_RX_LOS_MASK:
        case PORT_18_RX_LOS_MASK:
        case PORT_19_RX_LOS_MASK:
        case PORT_0_RX_LOS_DEBUG:
        case PORT_1_RX_LOS_DEBUG:
        case PORT_2_RX_LOS_DEBUG:
        case PORT_3_RX_LOS_DEBUG:
        case PORT_4_RX_LOS_DEBUG:
        case PORT_5_RX_LOS_DEBUG:
        case PORT_6_RX_LOS_DEBUG:
        case PORT_7_RX_LOS_DEBUG:
        case PORT_8_RX_LOS_DEBUG:
        case PORT_9_RX_LOS_DEBUG:
        case PORT_10_RX_LOS_DEBUG:
        case PORT_11_RX_LOS_DEBUG:
        case PORT_12_RX_LOS_DEBUG:
        case PORT_13_RX_LOS_DEBUG:
        case PORT_14_RX_LOS_DEBUG:
        case PORT_15_RX_LOS_DEBUG:
        case PORT_16_RX_LOS_DEBUG:
        case PORT_17_RX_LOS_DEBUG:
        case PORT_18_RX_LOS_DEBUG:
        case PORT_19_RX_LOS_DEBUG:
        case PORT_0_TX_FAULT_MASK:
        case PORT_1_TX_FAULT_MASK:
        case PORT_2_TX_FAULT_MASK:
        case PORT_3_TX_FAULT_MASK:
        case PORT_4_TX_FAULT_MASK:
        case PORT_5_TX_FAULT_MASK:
        case PORT_6_TX_FAULT_MASK:
        case PORT_7_TX_FAULT_MASK:
        case PORT_8_TX_FAULT_MASK:
        case PORT_9_TX_FAULT_MASK:
        case PORT_10_TX_FAULT_MASK:
        case PORT_11_TX_FAULT_MASK:
        case PORT_12_TX_FAULT_MASK:
        case PORT_13_TX_FAULT_MASK:
        case PORT_14_TX_FAULT_MASK:
        case PORT_15_TX_FAULT_MASK:
        case PORT_16_TX_FAULT_MASK:
        case PORT_17_TX_FAULT_MASK:
        case PORT_18_TX_FAULT_MASK:
        case PORT_19_TX_FAULT_MASK:
        case PORT_0_TX_FAULT_DEBUG:
        case PORT_1_TX_FAULT_DEBUG:
        case PORT_2_TX_FAULT_DEBUG:
        case PORT_3_TX_FAULT_DEBUG:
        case PORT_4_TX_FAULT_DEBUG:
        case PORT_5_TX_FAULT_DEBUG:
        case PORT_6_TX_FAULT_DEBUG:
        case PORT_7_TX_FAULT_DEBUG:
        case PORT_8_TX_FAULT_DEBUG:
        case PORT_9_TX_FAULT_DEBUG:
        case PORT_10_TX_FAULT_DEBUG:
        case PORT_11_TX_FAULT_DEBUG:
        case PORT_12_TX_FAULT_DEBUG:
        case PORT_13_TX_FAULT_DEBUG:
        case PORT_14_TX_FAULT_DEBUG:
        case PORT_15_TX_FAULT_DEBUG:
        case PORT_16_TX_FAULT_DEBUG:
        case PORT_17_TX_FAULT_DEBUG:
        case PORT_18_TX_FAULT_DEBUG:
        case PORT_19_TX_FAULT_DEBUG:
        case PORT_0_TX_DISABLE:
        case PORT_1_TX_DISABLE:
        case PORT_2_TX_DISABLE:
        case PORT_3_TX_DISABLE:
        case PORT_4_TX_DISABLE:
        case PORT_5_TX_DISABLE:
        case PORT_6_TX_DISABLE:
        case PORT_7_TX_DISABLE:
        case PORT_8_TX_DISABLE:
        case PORT_9_TX_DISABLE:
        case PORT_10_TX_DISABLE:
        case PORT_11_TX_DISABLE:
        case PORT_12_TX_DISABLE:
        case PORT_13_TX_DISABLE:
        case PORT_14_TX_DISABLE:
        case PORT_15_TX_DISABLE:
        case PORT_16_TX_DISABLE:
        case PORT_17_TX_DISABLE:
        case PORT_18_TX_DISABLE:
        case PORT_19_TX_DISABLE:
        case PORT_0_RATE_SEL:
        case PORT_1_RATE_SEL:
        case PORT_2_RATE_SEL:
        case PORT_3_RATE_SEL:
        case PORT_4_RATE_SEL:
        case PORT_5_RATE_SEL:
        case PORT_6_RATE_SEL:
        case PORT_7_RATE_SEL:
        case PORT_8_RATE_SEL:
        case PORT_9_RATE_SEL:
        case PORT_10_RATE_SEL:
        case PORT_11_RATE_SEL:
        case PORT_12_RATE_SEL:
        case PORT_13_RATE_SEL:
        case PORT_14_RATE_SEL:
        case PORT_15_RATE_SEL:
        case PORT_16_RATE_SEL:
        case PORT_17_RATE_SEL:
        case PORT_18_RATE_SEL:
        case PORT_19_RATE_SEL:
        case PORT_0_PWR_EN:
        case PORT_1_PWR_EN:
        case PORT_2_PWR_EN:
        case PORT_3_PWR_EN:
        case PORT_4_PWR_EN:
        case PORT_5_PWR_EN:
        case PORT_6_PWR_EN:
        case PORT_7_PWR_EN:
        case PORT_8_PWR_EN:
        case PORT_9_PWR_EN:
        case PORT_10_PWR_EN:
        case PORT_11_PWR_EN:
        case PORT_12_PWR_EN:
        case PORT_13_PWR_EN:
        case PORT_14_PWR_EN:
        case PORT_15_PWR_EN:
        case PORT_16_PWR_EN:
        case PORT_17_PWR_EN:
        case PORT_18_PWR_EN:
        case PORT_19_PWR_EN:
        case FAN_PWM_MODE_1:
        case FAN_PWM_MODE_2:
        case FAN_PWM_DIAG_CTRL_1:
        case FAN_PWM_DIAG_CTRL_2:
            reg = attr_reg[attr->index].reg;
            mask= attr_reg[attr->index].mask;
            break;
        default:
            return -EINVAL;
    }
    return cpld_reg_write(dev, buf, count, reg, mask);
}

/* get cpld register value */
static int _cpld_reg_read(struct device *dev,
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

/* get cpld register value without lock */
int _cpld_reg_read_nolock(struct device *dev,
                    u8 reg,
                    u8 mask)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
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

int _cpld_reg_write_nolock(struct device *dev,
                    u8 reg,
                    u8 reg_val)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
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
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg_val, shift;
    int reg_val_now;
    int ret = 0;

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

/* get qsfp port config register value */
static ssize_t cpld_version_h_show(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int major_val = -1;
    int minor_val = -1;
    int build_val = -1;

    if (attr->index == CPLD_VERSION_H) {
        if ((major_val = _cpld_reg_read(dev, attr_reg[CPLD_MAJOR_VER].reg, attr_reg[CPLD_MAJOR_VER].mask)) < 0)
            return major_val;
        if ((minor_val = _cpld_reg_read(dev, attr_reg[CPLD_MINOR_VER].reg, attr_reg[CPLD_MINOR_VER].mask)) < 0)
            return minor_val;
        if ((build_val = _cpld_reg_read(dev, attr_reg[CPLD_BUILD_VER].reg, attr_reg[CPLD_BUILD_VER].mask)) < 0)
            return build_val;

        return sprintf(buf, "%d.%02d.%03d", major_val, minor_val, build_val);
    }
    return -EINVAL;
}

/* combine fan_0_pwm_rpm_h and fan_0_pwm_rpm_l register value */
static ssize_t fan_0_pwm_rpm_show(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    if (attr->index == FAN_0_PWM_RPM) {
        int high_byte = _cpld_reg_read(dev, attr_reg[FAN_0_PWM_RPM_H].reg, attr_reg[FAN_0_PWM_RPM_H].mask);
        int low_byte = _cpld_reg_read(dev, attr_reg[FAN_0_PWM_RPM_L].reg, attr_reg[FAN_0_PWM_RPM_L].mask);

        int rpm = (high_byte << 8) | low_byte;
        return sprintf(buf, "%d\n", rpm);
    }
    return -1;
}

/* combine fan_1_pwm_rpm_h and fan_1_pwm_rpm_l register value */
static ssize_t fan_1_pwm_rpm_show(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    if (attr->index == FAN_1_PWM_RPM) {
        int high_byte = _cpld_reg_read(dev, attr_reg[FAN_1_PWM_RPM_H].reg, attr_reg[FAN_1_PWM_RPM_H].mask);
        int low_byte = _cpld_reg_read(dev, attr_reg[FAN_1_PWM_RPM_L].reg, attr_reg[FAN_1_PWM_RPM_L].mask);

        int rpm = (high_byte << 8) | low_byte;
        return sprintf(buf, "%d\n", rpm);
    }
    return -1;
}

/* combine fan_2_pwm_rpm_h and fan_2_pwm_rpm_l register value */
static ssize_t fan_2_pwm_rpm_show(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    if (attr->index == FAN_2_PWM_RPM) {
        int high_byte = _cpld_reg_read(dev, attr_reg[FAN_2_PWM_RPM_H].reg, attr_reg[FAN_2_PWM_RPM_H].mask);
        int low_byte = _cpld_reg_read(dev, attr_reg[FAN_2_PWM_RPM_L].reg, attr_reg[FAN_2_PWM_RPM_L].mask);

        int rpm = (high_byte << 8) | low_byte;
        return sprintf(buf, "%d\n", rpm);
    }
    return -1;
}

/* combine fan_3_pwm_rpm_h and fan_3_pwm_rpm_l register value */
static ssize_t fan_3_pwm_rpm_show(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    if (attr->index == FAN_3_PWM_RPM) {
        int high_byte = _cpld_reg_read(dev, attr_reg[FAN_3_PWM_RPM_H].reg, attr_reg[FAN_3_PWM_RPM_H].mask);
        int low_byte = _cpld_reg_read(dev, attr_reg[FAN_3_PWM_RPM_L].reg, attr_reg[FAN_3_PWM_RPM_L].mask);

        int rpm = (high_byte << 8) | low_byte;
        return sprintf(buf, "%d\n", rpm);
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
    struct cpld_data *data = NULL;
    int ret = -EPERM;
    u16 psu_status_reg = (0x700 + 0x59);
    u8 psu_type_mask = 0x40;
    u8 psu_status_reg_value = 0x0;

    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    /* init cpld data for client */
    i2c_set_clientdata(client, data);
    mutex_init(&data->access_lock);

    if (!i2c_check_functionality(client->adapter,
                I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev,
            "i2c_check_functionality failed (0x%x)\n",
            client->addr);
        status = -EIO;
        goto exit_free;
    }

    /* get cpld id from device */
    ret = i2c_smbus_read_byte_data(client, ID_REG);

    if (ret < 0) {
        dev_err(&client->dev,
            "fail to get cpld id (0x%x) at addr (0x%x)\n",
            ID_REG, client->addr);
        status = -EIO;
        goto exit_free;
    }

    /* check whether cpld id is valid */
    //if (INVALID(ret, cpld1, cpld2)) {
    //    dev_err(&client->dev,
    //        "cpld id %d(device) not valid\n", ret);
    //}
    psu_status_reg_value = inb(psu_status_reg);

    data->psu_type = (int)_mask_shift(psu_status_reg_value, psu_type_mask);
    //BSP_LOG_R("reg=0x%03x, reg_val=0x%02x, mask=0x%02x, psu_type=%d", psu_status_reg, psu_status_reg_value, psu_type_mask, data->psu_type);
    dev_info(&client->dev, "Get psu_type = %d from LPC\n", data->psu_type);

    data->index = dev_id->driver_data;

    /* register sysfs hooks for different cpld group */
    dev_info(&client->dev, "probe cpld with index %d\n", data->index);

    if (data->psu_type == 0 /* DC PSU */) {
        switch (data->index) {
            case cpld1:
                status = sysfs_create_group(&client->dev.kobj,
                        &cpld1_dc_psu_group);
                break;
            case cpld2:
                status = sysfs_create_group(&client->dev.kobj,
                        &cpld2_dc_psu_group);
                break;
            default:
                status = -EINVAL;
        }
    } else if (data->psu_type == 1 /* AC PSU */) {
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
    } else {
        dev_err(&client->dev, "Unknown psu_type = %d, while cpld_probe()\n", data->psu_type);
        status = -EPERM;
        goto exit_free;
    }

    if (status)
        goto exit_remove_sysfs;

    dev_info(&client->dev, "chip found\n");

    /* add probe chip to client list */
    cpld_add_client(client);

    return 0;

exit_remove_sysfs:
    if (data->psu_type == 0 /* DC PSU */) {
        switch (data->index) {
            case cpld1:
                sysfs_remove_group(&client->dev.kobj, &cpld1_dc_psu_group);
                break;
            case cpld2:
                sysfs_remove_group(&client->dev.kobj, &cpld2_dc_psu_group);
                break;
            default:
                break;
        }
    } else if (data->psu_type == 1 /* AC PSU */) {
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
    } else {
        dev_err(&client->dev, "Unknown psu_type = %d, while cpld_probe() exit\n", data->psu_type);
    }

exit_free:
    kfree(data);
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

    if (data->psu_type == 0/* DC PSU */) {
        switch (data->index) {
            case cpld1:
                sysfs_remove_group(&client->dev.kobj, &cpld1_dc_psu_group);
                break;
            case cpld2:
                sysfs_remove_group(&client->dev.kobj, &cpld2_dc_psu_group);
                break;
            default:
                break;
        }
    } else if (data->psu_type == 1/* AC PSU */) {
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
    } else {
        dev_err(&client->dev, "Unknown psu_type = %d, while cpld_remove()\n", data->psu_type);
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
        .name = DRIVER_NAME,
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


/* Meta Information */
MODULE_AUTHOR("Melo Lin <melo.lin@ufispace.com>");
MODULE_DESCRIPTION(DRIVER_NAME" driver");
MODULE_VERSION("0.0.7");
MODULE_LICENSE("GPL");

module_init(cpld_init);
module_exit(cpld_exit);
