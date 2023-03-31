/*
 * A i2c cpld driver for the ufispace_s9500_54cf
 *
 * Copyright (C) 2017-2022 UfiSpace Technology Corporation.
 * Wade He  <wade.ce.he@ufispace.com>
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
#include "x86-64-ufispace-s9500-54cf-cpld.h"

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

#define _SENSOR_DEVICE_ATTR_RO(_name, _func, _index)     \
    SENSOR_DEVICE_ATTR(_name, S_IRUGO, read_##_func, NULL, _index)

#define _SENSOR_DEVICE_ATTR_WO(_name, _func, _index)     \
    SENSOR_DEVICE_ATTR(_name, S_IWUSR, NULL, write_##_func, _index)

#define _SENSOR_DEVICE_ATTR_RW(_name, _func, _index)     \
    SENSOR_DEVICE_ATTR(_name, S_IRUGO | S_IWUSR, read_##_func, write_##_func, _index)

#define _DEVICE_ATTR(_name)     \
    &sensor_dev_attr_##_name.dev_attr.attr

/* CPLD sysfs attributes index  */
enum cpld_sysfs_attributes {
    CPLD_BOARD_ID_0,
    CPLD_BOARD_ID_1,
    CPLD_VERSION,
    CPLD_BUILD,
    CPLD_ID,
    CPLD_MAJOR_VER,
    CPLD_MINOR_VER,
    CPLD_BUILD_VER,
    CPLD_VERSION_H,
    CPLD_NTM_INTR,
    CPLD_MAC_INTR,
    CPLD_PSU_INTR,
    CPLD_THERMAL_SENSOR_INTR,
    CPLD_CPLDX_INTR,
    CPLD_FAN_INTR,
    CPLD_SYSTEM_INTR,
    CPLD_NTM_MASK,
    CPLD_MAC_MASK,
    CPLD_PSU_MASK,
    CPLD_THERMAL_SENSOR_MASK,
    CPLD_CPLDX_MASK,
    CPLD_FAN_MASK,
    CPLD_SYSTEM_MASK,
    CPLD_NTM_EVT,
    CPLD_MAC_EVT,
    CPLD_PSU_EVT,
    CPLD_THERMAL_SENSOR_EVT,
    CPLD_CPLDX_EVT,
    CPLD_FAN_EVT,
    CPLD_NTM_RESET,
    CPLD_MAC_RESET,
    CPLD_BMC_RESET,
    CPLD_CPLDX_RESET,
    CPLD_I2C_MUX_RESET_1,
    CPLD_I2C_MUX_RESET_2,
    CPLD_PSU_STATUS,
    CPLD_FAN_PRESENT,
    CPLD_MAC_ROV,
    CPLD_GNSS_STATUS,
    CPLD_MUX_CTRL,
    CPLD_SYSTEM_LED_ID,
    CPLD_SYSTEM_LED_SYNC,
    CPLD_SYSTEM_LED_SYS,
    CPLD_SYSTEM_LED_GNSS,
    CPLD_SYSTEM_LED_FAN,
    CPLD_SYSTEM_LED_PSU_0,
    CPLD_SYSTEM_LED_PSU_1,

    //CPLD 2-3
    CPLD_SFP_TX_FAULT_0_7,
    CPLD_SFP_TX_FAULT_8_15,
    CPLD_SFP_TX_FAULT_16_23,
    CPLD_SFP_TX_FAULT_24_31,
    CPLD_SFP_TX_FAULT_32_39,
    CPLD_SFP_TX_FAULT_40_47,
    CPLD_SFP_TX_FAULT_48_53,
    CPLD_SFP_RX_LOS_0_7,
    CPLD_SFP_RX_LOS_8_15,
    CPLD_SFP_RX_LOS_16_23,
    CPLD_SFP_RX_LOS_24_31,
    CPLD_SFP_RX_LOS_32_39,
    CPLD_SFP_RX_LOS_40_47,
    CPLD_SFP_RX_LOS_48_53,
    CPLD_SFP_PRESENT_0_7,
    CPLD_SFP_PRESENT_8_15,
    CPLD_SFP_PRESENT_16_23,
    CPLD_SFP_PRESENT_24_31,
    CPLD_SFP_PRESENT_32_39,
    CPLD_SFP_PRESENT_40_47,
    CPLD_SFP_PRESENT_48_53,

    CPLD_SFP_MASK_TX_FAULT_0_7,
    CPLD_SFP_MASK_TX_FAULT_8_15,
    CPLD_SFP_MASK_TX_FAULT_16_23,
    CPLD_SFP_MASK_TX_FAULT_24_31,
    CPLD_SFP_MASK_TX_FAULT_32_39,
    CPLD_SFP_MASK_TX_FAULT_40_47,
    CPLD_SFP_MASK_TX_FAULT_48_53,
    CPLD_SFP_MASK_RX_LOS_0_7,
    CPLD_SFP_MASK_RX_LOS_8_15,
    CPLD_SFP_MASK_RX_LOS_16_23,
    CPLD_SFP_MASK_RX_LOS_24_31,
    CPLD_SFP_MASK_RX_LOS_32_39,
    CPLD_SFP_MASK_RX_LOS_40_47,
    CPLD_SFP_MASK_RX_LOS_48_53,
    CPLD_SFP_MASK_PRESENT_0_7,
    CPLD_SFP_MASK_PRESENT_8_15,
    CPLD_SFP_MASK_PRESENT_16_23,
    CPLD_SFP_MASK_PRESENT_24_31,
    CPLD_SFP_MASK_PRESENT_32_39,
    CPLD_SFP_MASK_PRESENT_40_47,
    CPLD_SFP_MASK_PRESENT_48_53,
    CPLD_SFP_EVT_TX_FAULT_0_7,
    CPLD_SFP_EVT_TX_FAULT_8_15,
    CPLD_SFP_EVT_TX_FAULT_16_23,
    CPLD_SFP_EVT_TX_FAULT_24_31,
    CPLD_SFP_EVT_TX_FAULT_32_39,
    CPLD_SFP_EVT_TX_FAULT_40_47,
    CPLD_SFP_EVT_TX_FAULT_48_53,
    CPLD_SFP_EVT_RX_LOS_0_7,
    CPLD_SFP_EVT_RX_LOS_8_15,
    CPLD_SFP_EVT_RX_LOS_16_23,
    CPLD_SFP_EVT_RX_LOS_24_31,
    CPLD_SFP_EVT_RX_LOS_32_39,
    CPLD_SFP_EVT_RX_LOS_40_47,
    CPLD_SFP_EVT_RX_LOS_48_53,
    CPLD_SFP_EVT_PRESENT_0_7,
    CPLD_SFP_EVT_PRESENT_8_15,
    CPLD_SFP_EVT_PRESENT_16_23,
    CPLD_SFP_EVT_PRESENT_24_31,
    CPLD_SFP_EVT_PRESENT_32_39,
    CPLD_SFP_EVT_PRESENT_40_47,
    CPLD_SFP_EVT_PRESENT_48_53,
    CPLD_SFP_TX_DISABLE_0_7,
    CPLD_SFP_TX_DISABLE_8_15,
    CPLD_SFP_TX_DISABLE_16_23,
    CPLD_SFP_TX_DISABLE_24_31,
    CPLD_SFP_TX_DISABLE_32_39,
    CPLD_SFP_TX_DISABLE_40_47,
    CPLD_SFP_TX_DISABLE_48_53,
    CPLD_SFP_LED_0_3,
    CPLD_SFP_LED_4_7,
    CPLD_SFP_LED_8_11,
    CPLD_SFP_LED_12_15,
    CPLD_SFP_LED_16_19,
    CPLD_SFP_LED_20_23,
    CPLD_SFP_LED_24_27,
    CPLD_SFP_LED_28_31,
    CPLD_SFP_LED_32_35,
    CPLD_SFP_LED_36_39,
    CPLD_SFP_LED_40_43,
    CPLD_SFP_LED_44_47,
    CPLD_SFP_LED_48_51,
    CPLD_SFP_LED_52_53,
    CPLD_SFP_LED_CLR,
    DBG_CPLD_SFP_TX_FAULT_0_7,
    DBG_CPLD_SFP_TX_FAULT_8_15,
    DBG_CPLD_SFP_TX_FAULT_16_23,
    DBG_CPLD_SFP_TX_FAULT_24_31,
    DBG_CPLD_SFP_TX_FAULT_32_39,
    DBG_CPLD_SFP_TX_FAULT_40_47,
    DBG_CPLD_SFP_TX_FAULT_48_53,
    DBG_CPLD_SFP_RX_LOS_0_7,
    DBG_CPLD_SFP_RX_LOS_8_15,
    DBG_CPLD_SFP_RX_LOS_16_23,
    DBG_CPLD_SFP_RX_LOS_24_31,
    DBG_CPLD_SFP_RX_LOS_32_39,
    DBG_CPLD_SFP_RX_LOS_40_47,
    DBG_CPLD_SFP_RX_LOS_48_53,
    DBG_CPLD_SFP_PRESENT_0_7,
    DBG_CPLD_SFP_PRESENT_8_15,
    DBG_CPLD_SFP_PRESENT_16_23,
    DBG_CPLD_SFP_PRESENT_24_31,
    DBG_CPLD_SFP_PRESENT_32_39,
    DBG_CPLD_SFP_PRESENT_40_47,
    DBG_CPLD_SFP_PRESENT_48_53,

    //BSP DEBUG
    BSP_DEBUG
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
static ssize_t read_cpld_callback(struct device *dev,
        struct device_attribute *da, char *buf);
static ssize_t write_cpld_callback(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static u8 _read_cpld_reg(struct device *dev, u8 reg, u8 mask);
static ssize_t read_cpld_reg(struct device *dev, char *buf, u8 reg, u8 mask);
static ssize_t write_cpld_reg(struct device *dev, const char *buf, size_t count, u8 reg, u8 mask);
static ssize_t read_bsp(char *buf, char *str);
static ssize_t write_bsp(const char *buf, char *str, size_t str_len, size_t count);
static ssize_t read_bsp_callback(struct device *dev,
        struct device_attribute *da, char *buf);
static ssize_t write_bsp_callback(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_cpld_version_h(struct device *dev,
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
    { "s9500_54cf_cpld1",  cpld1 },
    { "s9500_54cf_cpld2",  cpld2 },
    { "s9500_54cf_cpld3",  cpld3 },
    {}
};

char bsp_debug[2]="0";
u8 enable_log_read=LOG_DISABLE;
u8 enable_log_write=LOG_DISABLE;

/* Addresses scanned for cpld */
static const unsigned short cpld_i2c_addr[] = { 0x30, 0x31, 0x32, I2C_CLIENT_END };

/* define all support register access of cpld in attribute */

// CPLD common
static _SENSOR_DEVICE_ATTR_RO(cpld_board_id_0,  cpld_callback, CPLD_BOARD_ID_0);
static _SENSOR_DEVICE_ATTR_RO(cpld_board_id_1,  cpld_callback, CPLD_BOARD_ID_1);
static _SENSOR_DEVICE_ATTR_RO(cpld_version,     cpld_callback, CPLD_VERSION);
static _SENSOR_DEVICE_ATTR_RO(cpld_build,       cpld_callback, CPLD_BUILD);
static _SENSOR_DEVICE_ATTR_RO(cpld_id,          cpld_callback, CPLD_ID);
static _SENSOR_DEVICE_ATTR_RO(cpld_major_ver,   cpld_callback, CPLD_MAJOR_VER);
static _SENSOR_DEVICE_ATTR_RO(cpld_minor_ver,   cpld_callback, CPLD_MINOR_VER);
static _SENSOR_DEVICE_ATTR_RO(cpld_build_ver,   cpld_callback, CPLD_BUILD_VER);
static _SENSOR_DEVICE_ATTR_RO(cpld_version_h,   cpld_version_h, CPLD_VERSION_H);

//CPLD 1
static _SENSOR_DEVICE_ATTR_RO(cpld_mac_intr,    cpld_callback, CPLD_MAC_INTR);
static _SENSOR_DEVICE_ATTR_RO(cpld_psu_intr,    cpld_callback, CPLD_PSU_INTR);
static _SENSOR_DEVICE_ATTR_RO(cpld_fan_intr,    cpld_callback, CPLD_FAN_INTR);
static _SENSOR_DEVICE_ATTR_RO(cpld_cpldx_intr,      cpld_callback, CPLD_CPLDX_INTR);
static _SENSOR_DEVICE_ATTR_RO(cpld_ntm_intr,      cpld_callback, CPLD_NTM_INTR);
static _SENSOR_DEVICE_ATTR_RO(cpld_thermal_sensor_intr, cpld_callback, CPLD_THERMAL_SENSOR_INTR);
static _SENSOR_DEVICE_ATTR_RO(cpld_system_intr,   cpld_callback, CPLD_SYSTEM_INTR);

static _SENSOR_DEVICE_ATTR_RW(cpld_mac_mask,    cpld_callback, CPLD_MAC_MASK);
static _SENSOR_DEVICE_ATTR_RW(cpld_psu_mask,    cpld_callback, CPLD_PSU_MASK);
static _SENSOR_DEVICE_ATTR_RW(cpld_cpldx_mask, cpld_callback, CPLD_CPLDX_MASK);
static _SENSOR_DEVICE_ATTR_RW(cpld_ntm_mask, cpld_callback, CPLD_NTM_MASK);
static _SENSOR_DEVICE_ATTR_RW(cpld_fan_mask, cpld_callback, CPLD_FAN_MASK);
static _SENSOR_DEVICE_ATTR_RW(cpld_thermal_sensor_mask, cpld_callback, CPLD_THERMAL_SENSOR_MASK);
static _SENSOR_DEVICE_ATTR_RW(cpld_system_mask,   cpld_callback, CPLD_SYSTEM_MASK);

static _SENSOR_DEVICE_ATTR_RO(cpld_mac_evt,     cpld_callback, CPLD_MAC_EVT);
static _SENSOR_DEVICE_ATTR_RO(cpld_psu_evt,     cpld_callback, CPLD_PSU_EVT);
static _SENSOR_DEVICE_ATTR_RO(cpld_cpldx_evt, cpld_callback, CPLD_CPLDX_EVT);
static _SENSOR_DEVICE_ATTR_RO(cpld_ntm_evt, cpld_callback, CPLD_NTM_EVT);
static _SENSOR_DEVICE_ATTR_RO(cpld_fan_evt, cpld_callback, CPLD_FAN_EVT);
static _SENSOR_DEVICE_ATTR_RO(cpld_thermal_sensor_evt, cpld_callback, CPLD_THERMAL_SENSOR_EVT);

static _SENSOR_DEVICE_ATTR_RW(cpld_mac_reset,     cpld_callback, CPLD_MAC_RESET);
static _SENSOR_DEVICE_ATTR_RW(cpld_ntm_reset,     cpld_callback, CPLD_NTM_RESET);
static _SENSOR_DEVICE_ATTR_RW(cpld_bmc_reset, cpld_callback, CPLD_BMC_RESET);
static _SENSOR_DEVICE_ATTR_RW(cpld_cpldx_reset,     cpld_callback, CPLD_CPLDX_RESET);
static _SENSOR_DEVICE_ATTR_RW(cpld_i2c_mux_reset_1, cpld_callback, CPLD_I2C_MUX_RESET_1);
static _SENSOR_DEVICE_ATTR_RW(cpld_i2c_mux_reset_2, cpld_callback, CPLD_I2C_MUX_RESET_2);

static _SENSOR_DEVICE_ATTR_RO(cpld_psu_status,  cpld_callback, CPLD_PSU_STATUS);
static _SENSOR_DEVICE_ATTR_RO(cpld_fan_present,  cpld_callback, CPLD_FAN_PRESENT);
static _SENSOR_DEVICE_ATTR_RO(cpld_mac_rov,   cpld_callback, CPLD_MAC_ROV);
static _SENSOR_DEVICE_ATTR_RO(cpld_gnss_status, cpld_callback, CPLD_GNSS_STATUS);

static _SENSOR_DEVICE_ATTR_RW(cpld_mux_ctrl, cpld_callback, CPLD_MUX_CTRL);

static _SENSOR_DEVICE_ATTR_RW(cpld_system_led_sync,  cpld_callback, CPLD_SYSTEM_LED_SYNC);
static _SENSOR_DEVICE_ATTR_RW(cpld_system_led_gnss,  cpld_callback, CPLD_SYSTEM_LED_GNSS);
static _SENSOR_DEVICE_ATTR_RW(cpld_system_led_id,  cpld_callback, CPLD_SYSTEM_LED_ID);
static _SENSOR_DEVICE_ATTR_RW(cpld_system_led_sys,   cpld_callback, CPLD_SYSTEM_LED_SYS);
static _SENSOR_DEVICE_ATTR_RW(cpld_system_led_fan,   cpld_callback, CPLD_SYSTEM_LED_FAN);
static _SENSOR_DEVICE_ATTR_RW(cpld_system_led_psu_0, cpld_callback, CPLD_SYSTEM_LED_PSU_0);
static _SENSOR_DEVICE_ATTR_RW(cpld_system_led_psu_1, cpld_callback, CPLD_SYSTEM_LED_PSU_1);

//CPLD 2/3/4/5
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_tx_fault_0_7,   cpld_callback, CPLD_SFP_TX_FAULT_0_7);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_tx_fault_8_15,  cpld_callback, CPLD_SFP_TX_FAULT_8_15);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_tx_fault_16_23, cpld_callback, CPLD_SFP_TX_FAULT_16_23);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_tx_fault_24_31, cpld_callback, CPLD_SFP_TX_FAULT_24_31);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_tx_fault_32_39, cpld_callback, CPLD_SFP_TX_FAULT_32_39);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_tx_fault_40_47, cpld_callback, CPLD_SFP_TX_FAULT_40_47);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_tx_fault_48_53, cpld_callback, CPLD_SFP_TX_FAULT_48_53);

static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_rx_los_0_7,   cpld_callback, CPLD_SFP_RX_LOS_0_7);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_rx_los_8_15,  cpld_callback, CPLD_SFP_RX_LOS_8_15);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_rx_los_16_23, cpld_callback, CPLD_SFP_RX_LOS_16_23);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_rx_los_24_31, cpld_callback, CPLD_SFP_RX_LOS_24_31);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_rx_los_32_39, cpld_callback, CPLD_SFP_RX_LOS_32_39);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_rx_los_40_47, cpld_callback, CPLD_SFP_RX_LOS_40_47);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_rx_los_48_53, cpld_callback, CPLD_SFP_RX_LOS_48_53);

static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_present_0_7,   cpld_callback, CPLD_SFP_PRESENT_0_7);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_present_8_15,  cpld_callback, CPLD_SFP_PRESENT_8_15);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_present_16_23, cpld_callback, CPLD_SFP_PRESENT_16_23);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_present_24_31, cpld_callback, CPLD_SFP_PRESENT_24_31);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_present_32_39, cpld_callback, CPLD_SFP_PRESENT_32_39);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_present_40_47, cpld_callback, CPLD_SFP_PRESENT_40_47);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_present_48_53, cpld_callback, CPLD_SFP_PRESENT_48_53);

static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_tx_fault_0_7,   cpld_callback, CPLD_SFP_MASK_TX_FAULT_0_7);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_tx_fault_8_15,  cpld_callback, CPLD_SFP_MASK_TX_FAULT_8_15);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_tx_fault_16_23, cpld_callback, CPLD_SFP_MASK_TX_FAULT_16_23);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_tx_fault_24_31, cpld_callback, CPLD_SFP_MASK_TX_FAULT_24_31);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_tx_fault_32_39, cpld_callback, CPLD_SFP_MASK_TX_FAULT_32_39);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_tx_fault_40_47, cpld_callback, CPLD_SFP_MASK_TX_FAULT_40_47);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_tx_fault_48_53, cpld_callback, CPLD_SFP_MASK_TX_FAULT_48_53);

static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_rx_los_0_7,   cpld_callback, CPLD_SFP_MASK_RX_LOS_0_7);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_rx_los_8_15,  cpld_callback, CPLD_SFP_MASK_RX_LOS_8_15);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_rx_los_16_23, cpld_callback, CPLD_SFP_MASK_RX_LOS_16_23);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_rx_los_24_31, cpld_callback, CPLD_SFP_MASK_RX_LOS_24_31);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_rx_los_32_39, cpld_callback, CPLD_SFP_MASK_RX_LOS_32_39);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_rx_los_40_47, cpld_callback, CPLD_SFP_MASK_RX_LOS_40_47);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_rx_los_48_53, cpld_callback, CPLD_SFP_MASK_RX_LOS_48_53);

static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_present_0_7,   cpld_callback, CPLD_SFP_MASK_PRESENT_0_7);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_present_8_15,  cpld_callback, CPLD_SFP_MASK_PRESENT_8_15);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_present_16_23, cpld_callback, CPLD_SFP_MASK_PRESENT_16_23);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_present_24_31, cpld_callback, CPLD_SFP_MASK_PRESENT_24_31);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_present_32_39, cpld_callback, CPLD_SFP_MASK_PRESENT_32_39);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_present_40_47, cpld_callback, CPLD_SFP_MASK_PRESENT_40_47);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask_present_48_53, cpld_callback, CPLD_SFP_MASK_PRESENT_48_53);

static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_tx_fault_0_7,   cpld_callback, CPLD_SFP_EVT_TX_FAULT_0_7);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_tx_fault_8_15,  cpld_callback, CPLD_SFP_EVT_TX_FAULT_8_15);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_tx_fault_16_23, cpld_callback, CPLD_SFP_EVT_TX_FAULT_16_23);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_tx_fault_24_31, cpld_callback, CPLD_SFP_EVT_TX_FAULT_24_31);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_tx_fault_32_39, cpld_callback, CPLD_SFP_EVT_TX_FAULT_32_39);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_tx_fault_40_47, cpld_callback, CPLD_SFP_EVT_TX_FAULT_40_47);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_tx_fault_48_53, cpld_callback, CPLD_SFP_EVT_TX_FAULT_48_53);

static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_rx_los_0_7,   cpld_callback, CPLD_SFP_EVT_RX_LOS_0_7);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_rx_los_8_15,  cpld_callback, CPLD_SFP_EVT_RX_LOS_8_15);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_rx_los_16_23, cpld_callback, CPLD_SFP_EVT_RX_LOS_16_23);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_rx_los_24_31, cpld_callback, CPLD_SFP_EVT_RX_LOS_24_31);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_rx_los_32_39, cpld_callback, CPLD_SFP_EVT_RX_LOS_32_39);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_rx_los_40_47, cpld_callback, CPLD_SFP_EVT_RX_LOS_40_47);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_rx_los_48_53, cpld_callback, CPLD_SFP_EVT_RX_LOS_48_53);

static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_present_0_7,   cpld_callback, CPLD_SFP_EVT_PRESENT_0_7);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_present_8_15,  cpld_callback, CPLD_SFP_EVT_PRESENT_8_15);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_present_16_23, cpld_callback, CPLD_SFP_EVT_PRESENT_16_23);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_present_24_31, cpld_callback, CPLD_SFP_EVT_PRESENT_24_31);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_present_32_39, cpld_callback, CPLD_SFP_EVT_PRESENT_32_39);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_present_40_47, cpld_callback, CPLD_SFP_EVT_PRESENT_40_47);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt_present_48_53, cpld_callback, CPLD_SFP_EVT_PRESENT_48_53);

static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_tx_disable_0_7,   cpld_callback, CPLD_SFP_TX_DISABLE_0_7);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_tx_disable_8_15,  cpld_callback, CPLD_SFP_TX_DISABLE_8_15);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_tx_disable_16_23, cpld_callback, CPLD_SFP_TX_DISABLE_16_23);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_tx_disable_24_31, cpld_callback, CPLD_SFP_TX_DISABLE_24_31);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_tx_disable_32_39, cpld_callback, CPLD_SFP_TX_DISABLE_32_39);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_tx_disable_40_47, cpld_callback, CPLD_SFP_TX_DISABLE_40_47);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_tx_disable_48_53, cpld_callback, CPLD_SFP_TX_DISABLE_48_53);

static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_led_0_3,   cpld_callback, CPLD_SFP_LED_0_3);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_led_4_7,   cpld_callback, CPLD_SFP_LED_4_7);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_led_8_11,  cpld_callback, CPLD_SFP_LED_8_11);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_led_12_15,  cpld_callback, CPLD_SFP_LED_12_15);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_led_16_19, cpld_callback, CPLD_SFP_LED_16_19);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_led_20_23, cpld_callback, CPLD_SFP_LED_20_23);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_led_24_27, cpld_callback, CPLD_SFP_LED_24_27);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_led_28_31, cpld_callback, CPLD_SFP_LED_28_31);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_led_32_35, cpld_callback, CPLD_SFP_LED_32_35);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_led_36_39, cpld_callback, CPLD_SFP_LED_36_39);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_led_40_43, cpld_callback, CPLD_SFP_LED_40_43);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_led_44_47, cpld_callback, CPLD_SFP_LED_44_47);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_led_48_51, cpld_callback, CPLD_SFP_LED_48_51);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_led_52_53, cpld_callback, CPLD_SFP_LED_52_53);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_led_clr, cpld_callback, CPLD_SFP_LED_CLR);

static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_sfp_tx_fault_0_7,   cpld_callback, DBG_CPLD_SFP_TX_FAULT_0_7);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_sfp_tx_fault_8_15,  cpld_callback, DBG_CPLD_SFP_TX_FAULT_8_15);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_sfp_tx_fault_16_23, cpld_callback, DBG_CPLD_SFP_TX_FAULT_16_23);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_sfp_tx_fault_24_31,   cpld_callback, DBG_CPLD_SFP_TX_FAULT_24_31);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_sfp_tx_fault_32_39,  cpld_callback, DBG_CPLD_SFP_TX_FAULT_32_39);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_sfp_tx_fault_40_47, cpld_callback, DBG_CPLD_SFP_TX_FAULT_40_47);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_sfp_tx_fault_48_53, cpld_callback, DBG_CPLD_SFP_TX_FAULT_48_53);

static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_sfp_rx_los_0_7,   cpld_callback, DBG_CPLD_SFP_RX_LOS_0_7);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_sfp_rx_los_8_15,  cpld_callback, DBG_CPLD_SFP_RX_LOS_8_15);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_sfp_rx_los_16_23, cpld_callback, DBG_CPLD_SFP_RX_LOS_16_23);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_sfp_rx_los_24_31, cpld_callback, DBG_CPLD_SFP_RX_LOS_24_31);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_sfp_rx_los_32_39, cpld_callback, DBG_CPLD_SFP_RX_LOS_32_39);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_sfp_rx_los_40_47, cpld_callback, DBG_CPLD_SFP_RX_LOS_40_47);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_sfp_rx_los_48_53, cpld_callback, DBG_CPLD_SFP_RX_LOS_48_53);

static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_sfp_present_0_7,   cpld_callback, DBG_CPLD_SFP_PRESENT_0_7);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_sfp_present_8_15,  cpld_callback, DBG_CPLD_SFP_PRESENT_8_15);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_sfp_present_16_23, cpld_callback, DBG_CPLD_SFP_PRESENT_16_23);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_sfp_present_24_31, cpld_callback, DBG_CPLD_SFP_PRESENT_24_31);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_sfp_present_32_39, cpld_callback, DBG_CPLD_SFP_PRESENT_32_39);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_sfp_present_40_47, cpld_callback, DBG_CPLD_SFP_PRESENT_40_47);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_sfp_present_48_53, cpld_callback, DBG_CPLD_SFP_PRESENT_48_53);

//BSP DEBUG
static _SENSOR_DEVICE_ATTR_RW(bsp_debug, bsp_callback, BSP_DEBUG);

/* define support attributes of cpldx */

/* cpld 1 */
static struct attribute *cpld1_attributes[] = {
    _DEVICE_ATTR(cpld_board_id_0),
    _DEVICE_ATTR(cpld_board_id_1),
    _DEVICE_ATTR(cpld_version),
    _DEVICE_ATTR(cpld_build),
    _DEVICE_ATTR(cpld_id),

    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),

    _DEVICE_ATTR(cpld_mac_intr),
    _DEVICE_ATTR(cpld_psu_intr),
    _DEVICE_ATTR(cpld_fan_intr),
    _DEVICE_ATTR(cpld_cpldx_intr),
    _DEVICE_ATTR(cpld_ntm_intr),
	_DEVICE_ATTR(cpld_thermal_sensor_intr),
    _DEVICE_ATTR(cpld_system_intr),

    _DEVICE_ATTR(cpld_mac_mask),
    _DEVICE_ATTR(cpld_psu_mask),
    _DEVICE_ATTR(cpld_cpldx_mask),
    _DEVICE_ATTR(cpld_ntm_mask),
	_DEVICE_ATTR(cpld_fan_mask),
	_DEVICE_ATTR(cpld_thermal_sensor_mask),
    _DEVICE_ATTR(cpld_system_mask),

    _DEVICE_ATTR(cpld_mac_evt),
    _DEVICE_ATTR(cpld_psu_evt),
    _DEVICE_ATTR(cpld_cpldx_evt),
    _DEVICE_ATTR(cpld_ntm_evt),
	_DEVICE_ATTR(cpld_fan_evt),
	_DEVICE_ATTR(cpld_thermal_sensor_evt),

    _DEVICE_ATTR(cpld_ntm_reset),
    _DEVICE_ATTR(cpld_mac_reset),
    _DEVICE_ATTR(cpld_bmc_reset),
    _DEVICE_ATTR(cpld_cpldx_reset),
    _DEVICE_ATTR(cpld_i2c_mux_reset_1),
    _DEVICE_ATTR(cpld_i2c_mux_reset_2),

    _DEVICE_ATTR(cpld_psu_status),
    _DEVICE_ATTR(cpld_fan_present),

    _DEVICE_ATTR(cpld_mac_rov),
    _DEVICE_ATTR(cpld_gnss_status),

    _DEVICE_ATTR(cpld_mux_ctrl),

    _DEVICE_ATTR(cpld_system_led_sync),
    _DEVICE_ATTR(cpld_system_led_gnss),
    _DEVICE_ATTR(cpld_system_led_id),
    _DEVICE_ATTR(cpld_system_led_sys),
    _DEVICE_ATTR(cpld_system_led_fan),
    _DEVICE_ATTR(cpld_system_led_psu_0),
    _DEVICE_ATTR(cpld_system_led_psu_1),

    _DEVICE_ATTR(bsp_debug),

    NULL
};

/* cpld 2 */
static struct attribute *cpld2_attributes[] = {
    _DEVICE_ATTR(cpld_version),
    _DEVICE_ATTR(cpld_build),
    _DEVICE_ATTR(cpld_id),

    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),

    _DEVICE_ATTR(cpld_sfp_tx_fault_0_7),
    _DEVICE_ATTR(cpld_sfp_tx_fault_8_15),
    _DEVICE_ATTR(cpld_sfp_tx_fault_16_23),
    _DEVICE_ATTR(cpld_sfp_rx_los_0_7),
    _DEVICE_ATTR(cpld_sfp_rx_los_8_15),
    _DEVICE_ATTR(cpld_sfp_rx_los_16_23),
    _DEVICE_ATTR(cpld_sfp_present_0_7),
    _DEVICE_ATTR(cpld_sfp_present_8_15),
    _DEVICE_ATTR(cpld_sfp_present_16_23),

    _DEVICE_ATTR(cpld_sfp_mask_tx_fault_0_7),
    _DEVICE_ATTR(cpld_sfp_mask_tx_fault_8_15),
    _DEVICE_ATTR(cpld_sfp_mask_tx_fault_16_23),
    _DEVICE_ATTR(cpld_sfp_mask_rx_los_0_7),
    _DEVICE_ATTR(cpld_sfp_mask_rx_los_8_15),
    _DEVICE_ATTR(cpld_sfp_mask_rx_los_16_23),
    _DEVICE_ATTR(cpld_sfp_mask_present_0_7),
    _DEVICE_ATTR(cpld_sfp_mask_present_8_15),
    _DEVICE_ATTR(cpld_sfp_mask_present_16_23),

    _DEVICE_ATTR(cpld_sfp_evt_tx_fault_0_7),
    _DEVICE_ATTR(cpld_sfp_evt_tx_fault_8_15),
    _DEVICE_ATTR(cpld_sfp_evt_tx_fault_16_23),
    _DEVICE_ATTR(cpld_sfp_evt_rx_los_0_7),
    _DEVICE_ATTR(cpld_sfp_evt_rx_los_8_15),
    _DEVICE_ATTR(cpld_sfp_evt_rx_los_16_23),
    _DEVICE_ATTR(cpld_sfp_evt_present_0_7),
    _DEVICE_ATTR(cpld_sfp_evt_present_8_15),
    _DEVICE_ATTR(cpld_sfp_evt_present_16_23),

    _DEVICE_ATTR(cpld_sfp_tx_disable_0_7),
    _DEVICE_ATTR(cpld_sfp_tx_disable_8_15),
    _DEVICE_ATTR(cpld_sfp_tx_disable_16_23),
    _DEVICE_ATTR(cpld_sfp_led_0_3),
    _DEVICE_ATTR(cpld_sfp_led_4_7),
    _DEVICE_ATTR(cpld_sfp_led_8_11),
    _DEVICE_ATTR(cpld_sfp_led_12_15),
    _DEVICE_ATTR(cpld_sfp_led_16_19),
    _DEVICE_ATTR(cpld_sfp_led_20_23),
    _DEVICE_ATTR(cpld_sfp_led_clr),

    _DEVICE_ATTR(dbg_cpld_sfp_tx_fault_0_7),
    _DEVICE_ATTR(dbg_cpld_sfp_tx_fault_8_15),
    _DEVICE_ATTR(dbg_cpld_sfp_tx_fault_16_23),
    _DEVICE_ATTR(dbg_cpld_sfp_rx_los_0_7),
    _DEVICE_ATTR(dbg_cpld_sfp_rx_los_8_15),
    _DEVICE_ATTR(dbg_cpld_sfp_rx_los_16_23),
    _DEVICE_ATTR(dbg_cpld_sfp_present_0_7),
    _DEVICE_ATTR(dbg_cpld_sfp_present_8_15),
    _DEVICE_ATTR(dbg_cpld_sfp_present_16_23),
    NULL
};

/* cpld 3 */
static struct attribute *cpld3_attributes[] = {
    _DEVICE_ATTR(cpld_version),
    _DEVICE_ATTR(cpld_build),
    _DEVICE_ATTR(cpld_id),

    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),

    _DEVICE_ATTR(cpld_sfp_tx_fault_24_31),
    _DEVICE_ATTR(cpld_sfp_tx_fault_32_39),
    _DEVICE_ATTR(cpld_sfp_tx_fault_40_47),
    _DEVICE_ATTR(cpld_sfp_tx_fault_48_53),
    _DEVICE_ATTR(cpld_sfp_rx_los_24_31),
    _DEVICE_ATTR(cpld_sfp_rx_los_32_39),
    _DEVICE_ATTR(cpld_sfp_rx_los_40_47),
    _DEVICE_ATTR(cpld_sfp_rx_los_48_53),
    _DEVICE_ATTR(cpld_sfp_present_24_31),
    _DEVICE_ATTR(cpld_sfp_present_32_39),
    _DEVICE_ATTR(cpld_sfp_present_40_47),
    _DEVICE_ATTR(cpld_sfp_present_48_53),

    _DEVICE_ATTR(cpld_sfp_mask_tx_fault_24_31),
    _DEVICE_ATTR(cpld_sfp_mask_tx_fault_32_39),
    _DEVICE_ATTR(cpld_sfp_mask_tx_fault_40_47),
    _DEVICE_ATTR(cpld_sfp_mask_tx_fault_48_53),
    _DEVICE_ATTR(cpld_sfp_mask_rx_los_24_31),
    _DEVICE_ATTR(cpld_sfp_mask_rx_los_32_39),
    _DEVICE_ATTR(cpld_sfp_mask_rx_los_40_47),
    _DEVICE_ATTR(cpld_sfp_mask_rx_los_48_53),
    _DEVICE_ATTR(cpld_sfp_mask_present_24_31),
    _DEVICE_ATTR(cpld_sfp_mask_present_32_39),
    _DEVICE_ATTR(cpld_sfp_mask_present_40_47),
    _DEVICE_ATTR(cpld_sfp_mask_present_48_53),

    _DEVICE_ATTR(cpld_sfp_evt_tx_fault_24_31),
    _DEVICE_ATTR(cpld_sfp_evt_tx_fault_32_39),
    _DEVICE_ATTR(cpld_sfp_evt_tx_fault_40_47),
    _DEVICE_ATTR(cpld_sfp_evt_tx_fault_48_53),
    _DEVICE_ATTR(cpld_sfp_evt_rx_los_24_31),
    _DEVICE_ATTR(cpld_sfp_evt_rx_los_32_39),
    _DEVICE_ATTR(cpld_sfp_evt_rx_los_40_47),
    _DEVICE_ATTR(cpld_sfp_evt_rx_los_48_53),
    _DEVICE_ATTR(cpld_sfp_evt_present_24_31),
    _DEVICE_ATTR(cpld_sfp_evt_present_32_39),
    _DEVICE_ATTR(cpld_sfp_evt_present_40_47),
    _DEVICE_ATTR(cpld_sfp_evt_present_48_53),

    _DEVICE_ATTR(cpld_sfp_tx_disable_24_31),
    _DEVICE_ATTR(cpld_sfp_tx_disable_32_39),
    _DEVICE_ATTR(cpld_sfp_tx_disable_40_47),
    _DEVICE_ATTR(cpld_sfp_tx_disable_48_53),

    _DEVICE_ATTR(cpld_sfp_led_24_27),
    _DEVICE_ATTR(cpld_sfp_led_28_31),
    _DEVICE_ATTR(cpld_sfp_led_32_35),
    _DEVICE_ATTR(cpld_sfp_led_36_39),
    _DEVICE_ATTR(cpld_sfp_led_40_43),
    _DEVICE_ATTR(cpld_sfp_led_44_47),
    _DEVICE_ATTR(cpld_sfp_led_48_51),
    _DEVICE_ATTR(cpld_sfp_led_52_53),
    _DEVICE_ATTR(cpld_sfp_led_clr),

    _DEVICE_ATTR(dbg_cpld_sfp_tx_fault_24_31),
    _DEVICE_ATTR(dbg_cpld_sfp_tx_fault_32_39),
    _DEVICE_ATTR(dbg_cpld_sfp_tx_fault_40_47),
    _DEVICE_ATTR(dbg_cpld_sfp_tx_fault_48_53),
    _DEVICE_ATTR(dbg_cpld_sfp_rx_los_24_31),
    _DEVICE_ATTR(dbg_cpld_sfp_rx_los_32_39),
    _DEVICE_ATTR(dbg_cpld_sfp_rx_los_40_47),
    _DEVICE_ATTR(dbg_cpld_sfp_rx_los_48_53),
    _DEVICE_ATTR(dbg_cpld_sfp_present_24_31),
    _DEVICE_ATTR(dbg_cpld_sfp_present_32_39),
    _DEVICE_ATTR(dbg_cpld_sfp_present_40_47),
    _DEVICE_ATTR(dbg_cpld_sfp_present_48_53),


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
static ssize_t read_bsp(char *buf, char *str)
{
    ssize_t len=0;

    len=sprintf(buf, "%s", str);
    BSP_LOG_R("reg_val=%s", str);

    return len;
}

/* set bsp value */
static ssize_t write_bsp(const char *buf, char *str, size_t str_len, size_t count)
{
    snprintf(str, str_len, "%s", buf);
    BSP_LOG_W("reg_val=%s", str);

    return count;
}

/* get bsp parameter value */
static ssize_t read_bsp_callback(struct device *dev,
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
    return read_bsp(buf, str);
}

/* set bsp parameter value */
static ssize_t write_bsp_callback(struct device *dev,
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
            str_len = sizeof(str);
            ret = write_bsp(buf, str, str_len, count);

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
static ssize_t read_cpld_callback(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg = 0;
    u8 mask = MASK_ALL;

    switch (attr->index) {
        case CPLD_BOARD_ID_0:
            reg = CPLD_BOARD_ID_0_REG;
            break;
        case CPLD_BOARD_ID_1:
            reg = CPLD_BOARD_ID_1_REG;
            break;
        case CPLD_VERSION:
            reg = CPLD_VERSION_REG;
            break;
        case CPLD_ID:
            reg = CPLD_ID_REG;
            break;
        case CPLD_BUILD:
        case CPLD_BUILD_VER:
            reg = CPLD_BUILD_REG;
            break;
        case CPLD_MAJOR_VER:
            reg = CPLD_VERSION_REG;
            mask = MASK_CPLD_MAJOR_VER;
            break;
        case CPLD_MINOR_VER:
            reg = CPLD_VERSION_REG;
            mask = MASK_CPLD_MINOR_VER;
            break;
        case CPLD_MAC_INTR:
            reg = CPLD_MAC_INTR_REG;
            break;
        case CPLD_PSU_INTR:
            reg = CPLD_PSU_INTR_REG;
            break;
        case CPLD_FAN_INTR:
            reg = CPLD_FAN_INTR_REG;
            break;
        case CPLD_CPLDX_INTR:
            reg = CPLD_CPLDX_INTR_REG;
            break;
        case CPLD_NTM_INTR:
            reg = CPLD_NTM_INTR_REG;
            break;
        case CPLD_THERMAL_SENSOR_INTR:
            reg = CPLD_THERMAL_SENSOR_INTR_REG;
            break;
        case CPLD_SYSTEM_INTR:
            reg = CPLD_SYSTEM_INTR_REG;
            break;
        case CPLD_MAC_MASK:
            reg = CPLD_MAC_MASK_REG;
            break;
        case CPLD_PSU_MASK:
            reg = CPLD_PSU_MASK_REG;
            break;
        case CPLD_CPLDX_MASK:
            reg = CPLD_CPLDX_MASK_REG;
            break;
        case CPLD_NTM_MASK:
            reg = CPLD_NTM_MASK_REG;
            break;
        case CPLD_FAN_MASK:
            reg = CPLD_FAN_MASK_REG;
            break;
        case CPLD_THERMAL_SENSOR_MASK:
            reg = CPLD_THERMAL_SENSOR_MASK_REG;
            break;
        case CPLD_SYSTEM_MASK:
            reg = CPLD_SYSTEM_MASK_REG;
            break;
        case CPLD_MAC_EVT:
            reg = CPLD_MAC_EVT_REG;
            break;
        case CPLD_PSU_EVT:
            reg = CPLD_PSU_EVT_REG;
            break;
        case CPLD_CPLDX_EVT:
            reg = CPLD_CPLDX_EVT_REG;
            break;
        case CPLD_NTM_EVT:
            reg = CPLD_NTM_EVT_REG;
            break;
        case CPLD_FAN_EVT:
            reg = CPLD_FAN_EVT_REG;
            break;
        case CPLD_THERMAL_SENSOR_EVT:
            reg = CPLD_THERMAL_SENSOR_EVT_REG;
            break;
        case CPLD_MAC_RESET:
            reg = CPLD_MAC_RESET_REG;
            break;
        case CPLD_NTM_RESET:
            reg = CPLD_NTM_RESET_REG;
            break;
        case CPLD_BMC_RESET:
            reg = CPLD_BMC_RESET_REG;
            break;
        case CPLD_CPLDX_RESET:
            reg = CPLD_CPLDX_RESET_REG;
            break;
        case CPLD_I2C_MUX_RESET_1:
            reg = CPLD_I2C_MUX_RESET_1_REG;
            break;
        case CPLD_I2C_MUX_RESET_2:
            reg = CPLD_I2C_MUX_RESET_2_REG;
            break;
        case CPLD_PSU_STATUS:
            reg = CPLD_PSU_STATUS_REG;
            break;
        case CPLD_FAN_PRESENT:
            reg = CPLD_FAN_PRESENT_REG;
            break;
        case CPLD_MAC_ROV:
            reg = CPLD_MAC_ROV_REG;
            break;
        case CPLD_GNSS_STATUS:
            reg = CPLD_GNSS_STATUS_REG;
            break;
        case CPLD_MUX_CTRL:
            reg = CPLD_MUX_CTRL_REG;
            break;
        case CPLD_SYSTEM_LED_SYNC:
            reg = CPLD_SYSTEM_LED_ID_SYNC_REG;
            mask = CPLD_SYSTEM_LED_SYNC_MASK;
            break;
        case CPLD_SYSTEM_LED_GNSS:
            reg = CPLD_SYSTEM_LED_SYS_GNSS_REG;
            mask = CPLD_SYSTEM_LED_GNSS_MASK;
            break;
        case CPLD_SYSTEM_LED_ID:
            reg = CPLD_SYSTEM_LED_ID_SYNC_REG;
            mask = CPLD_SYSTEM_LED_ID_MASK;
            break;
        case CPLD_SYSTEM_LED_SYS:
            reg = CPLD_SYSTEM_LED_SYS_GNSS_REG;
            mask = CPLD_SYSTEM_LED_SYS_MASK;
            break;
        case CPLD_SYSTEM_LED_FAN:
            reg = CPLD_SYSTEM_LED_FAN_REG;
            mask = CPLD_SYSTEM_LED_FAN_MASK;
            break;
        case CPLD_SYSTEM_LED_PSU_0:
            reg = CPLD_SYSTEM_LED_PSU_REG;
            mask = CPLD_SYSTEM_LED_PSU_0_MASK;
            break;
        case CPLD_SYSTEM_LED_PSU_1:
            reg = CPLD_SYSTEM_LED_PSU_REG;
            mask = CPLD_SYSTEM_LED_PSU_1_MASK;
            break;
        case CPLD_SFP_TX_FAULT_0_7:
            reg = CPLD_SFP_TX_FAULT_0_7_REG;
            break;
        case CPLD_SFP_TX_FAULT_8_15:
            reg = CPLD_SFP_TX_FAULT_8_15_REG;
            break;
        case CPLD_SFP_TX_FAULT_16_23:
            reg = CPLD_SFP_TX_FAULT_16_23_REG;
            break;
        case CPLD_SFP_TX_FAULT_24_31:
            reg = CPLD_SFP_TX_FAULT_24_31_REG;
            break;
        case CPLD_SFP_TX_FAULT_32_39:
            reg = CPLD_SFP_TX_FAULT_32_39_REG;
            break;
        case CPLD_SFP_TX_FAULT_40_47:
            reg = CPLD_SFP_TX_FAULT_40_47_REG;
            break;
        case CPLD_SFP_TX_FAULT_48_53:
            reg = CPLD_SFP_TX_FAULT_48_53_REG;
            break;
        case CPLD_SFP_RX_LOS_0_7:
            reg = CPLD_SFP_RX_LOS_0_7_REG;
            break;
        case CPLD_SFP_RX_LOS_8_15:
            reg = CPLD_SFP_RX_LOS_8_15_REG;
            break;
        case CPLD_SFP_RX_LOS_16_23:
            reg = CPLD_SFP_RX_LOS_16_23_REG;
            break;
        case CPLD_SFP_RX_LOS_24_31:
            reg = CPLD_SFP_RX_LOS_24_31_REG;
            break;
        case CPLD_SFP_RX_LOS_32_39:
            reg = CPLD_SFP_RX_LOS_32_39_REG;
            break;
        case CPLD_SFP_RX_LOS_40_47:
            reg = CPLD_SFP_RX_LOS_40_47_REG;
            break;
        case CPLD_SFP_RX_LOS_48_53:
            reg = CPLD_SFP_RX_LOS_48_53_REG;
            break;
        case CPLD_SFP_PRESENT_0_7:
            reg = CPLD_SFP_PRESENT_0_7_REG;
            break;
        case CPLD_SFP_PRESENT_8_15:
            reg = CPLD_SFP_PRESENT_8_15_REG;
            break;
        case CPLD_SFP_PRESENT_16_23:
            reg = CPLD_SFP_PRESENT_16_23_REG;
            break;
        case CPLD_SFP_PRESENT_24_31:
            reg = CPLD_SFP_PRESENT_24_31_REG;
            break;
        case CPLD_SFP_PRESENT_32_39:
            reg = CPLD_SFP_PRESENT_32_39_REG;
            break;
        case CPLD_SFP_PRESENT_40_47:
            reg = CPLD_SFP_PRESENT_40_47_REG;
            break;
        case CPLD_SFP_PRESENT_48_53:
            reg = CPLD_SFP_PRESENT_48_53_REG;
            break;
        case CPLD_SFP_MASK_TX_FAULT_0_7:
            reg = CPLD_SFP_MASK_TX_FAULT_0_7_REG;
            break;
        case CPLD_SFP_MASK_TX_FAULT_8_15:
            reg = CPLD_SFP_MASK_TX_FAULT_8_15_REG;
            break;
        case CPLD_SFP_MASK_TX_FAULT_16_23:
            reg = CPLD_SFP_MASK_TX_FAULT_16_23_REG;
            break;
        case CPLD_SFP_MASK_TX_FAULT_24_31:
            reg = CPLD_SFP_MASK_TX_FAULT_24_31_REG;
            break;
        case CPLD_SFP_MASK_TX_FAULT_32_39:
            reg = CPLD_SFP_MASK_TX_FAULT_32_39_REG;
            break;
        case CPLD_SFP_MASK_TX_FAULT_40_47:
            reg = CPLD_SFP_MASK_TX_FAULT_40_47_REG;
            break;
        case CPLD_SFP_MASK_TX_FAULT_48_53:
            reg = CPLD_SFP_MASK_TX_FAULT_48_53_REG;
            break;
        case CPLD_SFP_MASK_RX_LOS_0_7:
            reg = CPLD_SFP_MASK_RX_LOS_0_7_REG;
            break;
        case CPLD_SFP_MASK_RX_LOS_8_15:
            reg = CPLD_SFP_MASK_RX_LOS_8_15_REG;
            break;
        case CPLD_SFP_MASK_RX_LOS_16_23:
            reg = CPLD_SFP_MASK_RX_LOS_16_23_REG;
            break;
        case CPLD_SFP_MASK_RX_LOS_24_31:
            reg = CPLD_SFP_MASK_RX_LOS_24_31_REG;
            break;
        case CPLD_SFP_MASK_RX_LOS_32_39:
            reg = CPLD_SFP_MASK_RX_LOS_32_39_REG;
            break;
        case CPLD_SFP_MASK_RX_LOS_40_47:
            reg = CPLD_SFP_MASK_RX_LOS_40_47_REG;
            break;
        case CPLD_SFP_MASK_RX_LOS_48_53:
            reg = CPLD_SFP_MASK_RX_LOS_48_53_REG;
            break;
        case CPLD_SFP_MASK_PRESENT_0_7:
            reg = CPLD_SFP_MASK_PRESENT_0_7_REG;
            break;
        case CPLD_SFP_MASK_PRESENT_8_15:
            reg = CPLD_SFP_MASK_PRESENT_8_15_REG;
            break;
        case CPLD_SFP_MASK_PRESENT_16_23:
            reg = CPLD_SFP_MASK_PRESENT_16_23_REG;
            break;
        case CPLD_SFP_MASK_PRESENT_24_31:
            reg = CPLD_SFP_MASK_PRESENT_24_31_REG;
            break;
        case CPLD_SFP_MASK_PRESENT_32_39:
            reg = CPLD_SFP_MASK_PRESENT_32_39_REG;
            break;
        case CPLD_SFP_MASK_PRESENT_40_47:
            reg = CPLD_SFP_MASK_PRESENT_40_47_REG;
            break;
        case CPLD_SFP_MASK_PRESENT_48_53:
            reg = CPLD_SFP_MASK_PRESENT_48_53_REG;
            break;
        case CPLD_SFP_EVT_TX_FAULT_0_7:
            reg = CPLD_SFP_EVT_TX_FAULT_0_7_REG;
            break;
        case CPLD_SFP_EVT_TX_FAULT_8_15:
            reg = CPLD_SFP_EVT_TX_FAULT_8_15_REG;
            break;
        case CPLD_SFP_EVT_TX_FAULT_16_23:
            reg = CPLD_SFP_EVT_TX_FAULT_16_23_REG;
            break;
        case CPLD_SFP_EVT_TX_FAULT_24_31:
            reg = CPLD_SFP_EVT_TX_FAULT_24_31_REG;
            break;
        case CPLD_SFP_EVT_TX_FAULT_32_39:
            reg = CPLD_SFP_EVT_TX_FAULT_32_39_REG;
            break;
        case CPLD_SFP_EVT_TX_FAULT_40_47:
            reg = CPLD_SFP_EVT_TX_FAULT_40_47_REG;
            break;
        case CPLD_SFP_EVT_TX_FAULT_48_53:
            reg = CPLD_SFP_EVT_TX_FAULT_48_53_REG;
            break;
        case CPLD_SFP_EVT_RX_LOS_0_7:
            reg = CPLD_SFP_EVT_RX_LOS_0_7_REG;
            break;
        case CPLD_SFP_EVT_RX_LOS_8_15:
            reg = CPLD_SFP_EVT_RX_LOS_8_15_REG;
            break;
        case CPLD_SFP_EVT_RX_LOS_16_23:
            reg = CPLD_SFP_EVT_RX_LOS_16_23_REG;
            break;
        case CPLD_SFP_EVT_RX_LOS_24_31:
            reg = CPLD_SFP_EVT_RX_LOS_24_31_REG;
            break;
        case CPLD_SFP_EVT_RX_LOS_32_39:
            reg = CPLD_SFP_EVT_RX_LOS_32_39_REG;
            break;
        case CPLD_SFP_EVT_RX_LOS_40_47:
            reg = CPLD_SFP_EVT_RX_LOS_40_47_REG;
            break;
        case CPLD_SFP_EVT_RX_LOS_48_53:
            reg = CPLD_SFP_EVT_RX_LOS_48_53_REG;
            break;
        case CPLD_SFP_EVT_PRESENT_0_7:
            reg = CPLD_SFP_EVT_PRESENT_0_7_REG;
            break;
        case CPLD_SFP_EVT_PRESENT_8_15:
            reg = CPLD_SFP_EVT_PRESENT_8_15_REG;
            break;
        case CPLD_SFP_EVT_PRESENT_16_23:
            reg = CPLD_SFP_EVT_PRESENT_16_23_REG;
            break;
        case CPLD_SFP_EVT_PRESENT_24_31:
            reg = CPLD_SFP_EVT_PRESENT_24_31_REG;
            break;
        case CPLD_SFP_EVT_PRESENT_32_39:
            reg = CPLD_SFP_EVT_PRESENT_32_39_REG;
            break;
        case CPLD_SFP_EVT_PRESENT_40_47:
            reg = CPLD_SFP_EVT_PRESENT_40_47_REG;
            break;
        case CPLD_SFP_EVT_PRESENT_48_53:
            reg = CPLD_SFP_EVT_PRESENT_48_53_REG;
            break;
        case CPLD_SFP_TX_DISABLE_0_7:
            reg = CPLD_SFP_TX_DISABLE_0_7_REG;
            break;
        case CPLD_SFP_TX_DISABLE_8_15:
            reg = CPLD_SFP_TX_DISABLE_8_15_REG;
            break;
        case CPLD_SFP_TX_DISABLE_16_23:
            reg = CPLD_SFP_TX_DISABLE_16_23_REG;
            break;
        case CPLD_SFP_TX_DISABLE_24_31:
            reg = CPLD_SFP_TX_DISABLE_24_31_REG;
            break;
        case CPLD_SFP_TX_DISABLE_32_39:
            reg = CPLD_SFP_TX_DISABLE_32_39_REG;
            break;
        case CPLD_SFP_TX_DISABLE_40_47:
            reg = CPLD_SFP_TX_DISABLE_40_47_REG;
            break;
        case CPLD_SFP_TX_DISABLE_48_53:
            reg = CPLD_SFP_TX_DISABLE_48_53_REG;
            break;
        case CPLD_SFP_LED_0_3:
            reg = CPLD_SFP_LED_0_3_REG;
            break;
        case CPLD_SFP_LED_4_7:
            reg = CPLD_SFP_LED_4_7_REG;
            break;
        case CPLD_SFP_LED_8_11:
            reg = CPLD_SFP_LED_8_11_REG;
            break;
        case CPLD_SFP_LED_12_15:
            reg = CPLD_SFP_LED_12_15_REG;
            break;
        case CPLD_SFP_LED_16_19:
            reg = CPLD_SFP_LED_16_19_REG;
            break;
        case CPLD_SFP_LED_20_23:
            reg = CPLD_SFP_LED_20_23_REG;
            break;
        case CPLD_SFP_LED_24_27:
            reg = CPLD_SFP_LED_24_27_REG;
            break;
        case CPLD_SFP_LED_28_31:
            reg = CPLD_SFP_LED_28_31_REG;
            break;
        case CPLD_SFP_LED_32_35:
            reg = CPLD_SFP_LED_32_35_REG;
            break;
        case CPLD_SFP_LED_36_39:
            reg = CPLD_SFP_LED_36_39_REG;
            break;
        case CPLD_SFP_LED_40_43:
            reg = CPLD_SFP_LED_40_43_REG;
            break;
        case CPLD_SFP_LED_44_47:
            reg = CPLD_SFP_LED_44_47_REG;
            break;
        case CPLD_SFP_LED_48_51:
            reg = CPLD_SFP_LED_48_51_REG;
            break;
        case CPLD_SFP_LED_52_53:
            reg = CPLD_SFP_LED_52_53_REG;
            break;
        case CPLD_SFP_LED_CLR:
            reg = CPLD_SFP_LED_CLR_REG;
            mask = CPLD_SFP_LED_CLR_MASK;
            break;

        //CPLD 2
        case DBG_CPLD_SFP_TX_FAULT_0_7:
            reg = DBG_CPLD_SFP_TX_FAULT_0_7_REG;
            break;
        case DBG_CPLD_SFP_TX_FAULT_8_15:
            reg = DBG_CPLD_SFP_TX_FAULT_8_15_REG;
            break;
        case DBG_CPLD_SFP_TX_FAULT_16_23:
            reg = DBG_CPLD_SFP_TX_FAULT_16_23_REG;
            break;
        case DBG_CPLD_SFP_TX_FAULT_24_31:
            reg = DBG_CPLD_SFP_TX_FAULT_24_31_REG;
            break;
        case DBG_CPLD_SFP_TX_FAULT_32_39:
            reg = DBG_CPLD_SFP_TX_FAULT_32_39_REG;
            break;
        case DBG_CPLD_SFP_TX_FAULT_40_47:
            reg = DBG_CPLD_SFP_TX_FAULT_40_47_REG;
            break;
        case DBG_CPLD_SFP_TX_FAULT_48_53:
            reg = DBG_CPLD_SFP_TX_FAULT_48_53_REG;
            break;
        case DBG_CPLD_SFP_RX_LOS_0_7:
            reg = DBG_CPLD_SFP_RX_LOS_0_7_REG;
            break;
        case DBG_CPLD_SFP_RX_LOS_8_15:
            reg = DBG_CPLD_SFP_RX_LOS_8_15_REG;
            break;
        case DBG_CPLD_SFP_RX_LOS_16_23:
            reg = DBG_CPLD_SFP_RX_LOS_16_23_REG;
            break;
        case DBG_CPLD_SFP_RX_LOS_24_31:
            reg = DBG_CPLD_SFP_RX_LOS_24_31_REG;
            break;
        case DBG_CPLD_SFP_RX_LOS_32_39:
            reg = DBG_CPLD_SFP_RX_LOS_32_39_REG;
            break;
        case DBG_CPLD_SFP_RX_LOS_40_47:
            reg = DBG_CPLD_SFP_RX_LOS_40_47_REG;
            break;
        case DBG_CPLD_SFP_RX_LOS_48_53:
            reg = DBG_CPLD_SFP_RX_LOS_48_53_REG;
            break;
        case DBG_CPLD_SFP_PRESENT_0_7:
            reg = DBG_CPLD_SFP_PRESENT_0_7_REG;
            break;
        case DBG_CPLD_SFP_PRESENT_8_15:
            reg = DBG_CPLD_SFP_PRESENT_8_15_REG;
            break;
        case DBG_CPLD_SFP_PRESENT_16_23:
            reg = DBG_CPLD_SFP_PRESENT_16_23_REG;
            break;
        case DBG_CPLD_SFP_PRESENT_24_31:
            reg = DBG_CPLD_SFP_PRESENT_24_31_REG;
            break;
        case DBG_CPLD_SFP_PRESENT_32_39:
            reg = DBG_CPLD_SFP_PRESENT_32_39_REG;
            break;
        case DBG_CPLD_SFP_PRESENT_40_47:
            reg = DBG_CPLD_SFP_PRESENT_40_47_REG;
            break;
        case DBG_CPLD_SFP_PRESENT_48_53:
            reg = DBG_CPLD_SFP_PRESENT_48_53_REG;
            break;

        default:
            return -EINVAL;
    }
    return read_cpld_reg(dev, buf, reg, mask);
}

/* set cpld register value */
static ssize_t write_cpld_callback(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg = 0;
    u8 mask = MASK_ALL;

    switch (attr->index) {
        case CPLD_MAC_MASK:
            reg = CPLD_MAC_MASK_REG;
            break;
        case CPLD_PSU_MASK:
            reg = CPLD_PSU_MASK_REG;
            break;
        case CPLD_THERMAL_SENSOR_MASK:
            reg = CPLD_THERMAL_SENSOR_MASK_REG;
            break;
        case CPLD_CPLDX_MASK:
            reg = CPLD_CPLDX_MASK_REG;
            break;
        case CPLD_NTM_MASK:
            reg = CPLD_NTM_MASK_REG;
            break;
        case CPLD_FAN_MASK:
            reg = CPLD_FAN_MASK_REG;
            break;
        case CPLD_SYSTEM_MASK:
            reg = CPLD_SYSTEM_MASK_REG;
            break;
        case CPLD_MAC_RESET:
            reg = CPLD_MAC_RESET_REG;
            break;
        case CPLD_BMC_RESET:
            reg = CPLD_BMC_RESET_REG;
            break;
        case CPLD_CPLDX_RESET:
            reg = CPLD_CPLDX_RESET_REG;
            break;
        case CPLD_I2C_MUX_RESET_1:
            reg = CPLD_I2C_MUX_RESET_1_REG;
            break;
        case CPLD_I2C_MUX_RESET_2:
            reg = CPLD_I2C_MUX_RESET_2_REG;
            break;
        case CPLD_MUX_CTRL:
            reg = CPLD_MUX_CTRL_REG;
            break;
        case CPLD_SYSTEM_LED_SYNC:
            reg = CPLD_SYSTEM_LED_ID_SYNC_REG;
            mask = CPLD_SYSTEM_LED_SYNC_MASK;
            break;
        case CPLD_SYSTEM_LED_GNSS:
            reg = CPLD_SYSTEM_LED_SYS_GNSS_REG;
            mask = CPLD_SYSTEM_LED_GNSS_MASK;
            break;
        case CPLD_SYSTEM_LED_ID:
            reg = CPLD_SYSTEM_LED_ID_SYNC_REG;
            mask = CPLD_SYSTEM_LED_ID_MASK;
            break;
        case CPLD_SYSTEM_LED_SYS:
            reg = CPLD_SYSTEM_LED_SYS_GNSS_REG;
            mask = CPLD_SYSTEM_LED_SYS_MASK;
            break;
        case CPLD_SFP_TX_DISABLE_0_7:
            reg = CPLD_SFP_TX_DISABLE_0_7_REG;
            break;
        case CPLD_SFP_TX_DISABLE_8_15:
            reg = CPLD_SFP_TX_DISABLE_8_15_REG;
            break;
        case CPLD_SFP_TX_DISABLE_16_23:
            reg = CPLD_SFP_TX_DISABLE_16_23_REG;
            break;
        case CPLD_SFP_TX_DISABLE_24_31:
            reg = CPLD_SFP_TX_DISABLE_24_31_REG;
            break;
        case CPLD_SFP_TX_DISABLE_32_39:
            reg = CPLD_SFP_TX_DISABLE_32_39_REG;
            break;
        case CPLD_SFP_TX_DISABLE_40_47:
            reg = CPLD_SFP_TX_DISABLE_40_47_REG;
            break;
        case CPLD_SFP_TX_DISABLE_48_53:
            reg = CPLD_SFP_TX_DISABLE_48_53_REG;
            break;
        case CPLD_SFP_LED_0_3:
            reg = CPLD_SFP_LED_0_3_REG;
            break;
        case CPLD_SFP_LED_4_7:
            reg = CPLD_SFP_LED_4_7_REG;
            break;
        case CPLD_SFP_LED_8_11:
            reg = CPLD_SFP_LED_8_11_REG;
            break;
        case CPLD_SFP_LED_12_15:
            reg = CPLD_SFP_LED_12_15_REG;
            break;
        case CPLD_SFP_LED_16_19:
            reg = CPLD_SFP_LED_16_19_REG;
            break;
        case CPLD_SFP_LED_20_23:
            reg = CPLD_SFP_LED_20_23_REG;
            break;
        case CPLD_SFP_LED_24_27:
            reg = CPLD_SFP_LED_24_27_REG;
            break;
        case CPLD_SFP_LED_28_31:
            reg = CPLD_SFP_LED_28_31_REG;
            break;
        case CPLD_SFP_LED_32_35:
            reg = CPLD_SFP_LED_32_35_REG;
            break;
        case CPLD_SFP_LED_36_39:
            reg = CPLD_SFP_LED_36_39_REG;
            break;
        case CPLD_SFP_LED_40_43:
            reg = CPLD_SFP_LED_40_43_REG;
            break;
        case CPLD_SFP_LED_44_47:
            reg = CPLD_SFP_LED_44_47_REG;
            break;
        case CPLD_SFP_LED_48_51:
            reg = CPLD_SFP_LED_48_51_REG;
            break;
        case CPLD_SFP_LED_52_53:
            reg = CPLD_SFP_LED_52_53_REG;
            break;
        case CPLD_SFP_LED_CLR:
            reg = CPLD_SFP_LED_CLR_REG;
            mask = CPLD_SFP_LED_CLR_MASK;
            break;
        case CPLD_SFP_MASK_TX_FAULT_0_7:
            reg = CPLD_SFP_MASK_TX_FAULT_0_7_REG;
            break;
        case CPLD_SFP_MASK_TX_FAULT_8_15:
            reg = CPLD_SFP_MASK_TX_FAULT_8_15_REG;
            break;
        case CPLD_SFP_MASK_TX_FAULT_16_23:
            reg = CPLD_SFP_MASK_TX_FAULT_16_23_REG;
            break;
        case CPLD_SFP_MASK_TX_FAULT_24_31:
            reg = CPLD_SFP_MASK_TX_FAULT_24_31_REG;
            break;
        case CPLD_SFP_MASK_TX_FAULT_32_39:
            reg = CPLD_SFP_MASK_TX_FAULT_32_39_REG;
            break;
        case CPLD_SFP_MASK_TX_FAULT_40_47:
            reg = CPLD_SFP_MASK_TX_FAULT_40_47_REG;
            break;
        case CPLD_SFP_MASK_TX_FAULT_48_53:
            reg = CPLD_SFP_MASK_TX_FAULT_48_53_REG;
            break;
        case CPLD_SFP_MASK_RX_LOS_0_7:
            reg = CPLD_SFP_MASK_RX_LOS_0_7_REG;
            break;
        case CPLD_SFP_MASK_RX_LOS_8_15:
            reg = CPLD_SFP_MASK_RX_LOS_8_15_REG;
            break;
        case CPLD_SFP_MASK_RX_LOS_16_23:
            reg = CPLD_SFP_MASK_RX_LOS_16_23_REG;
            break;
        case CPLD_SFP_MASK_RX_LOS_24_31:
            reg = CPLD_SFP_MASK_RX_LOS_24_31_REG;
            break;
        case CPLD_SFP_MASK_RX_LOS_32_39:
            reg = CPLD_SFP_MASK_RX_LOS_32_39_REG;
            break;
        case CPLD_SFP_MASK_RX_LOS_40_47:
            reg = CPLD_SFP_MASK_RX_LOS_40_47_REG;
            break;
        case CPLD_SFP_MASK_RX_LOS_48_53:
            reg = CPLD_SFP_MASK_RX_LOS_48_53_REG;
            break;
        case CPLD_SFP_MASK_PRESENT_0_7:
            reg = CPLD_SFP_MASK_PRESENT_0_7_REG;
            break;
        case CPLD_SFP_MASK_PRESENT_8_15:
            reg = CPLD_SFP_MASK_PRESENT_8_15_REG;
            break;
        case CPLD_SFP_MASK_PRESENT_16_23:
            reg = CPLD_SFP_MASK_PRESENT_16_23_REG;
            break;
        case CPLD_SFP_MASK_PRESENT_24_31:
            reg = CPLD_SFP_MASK_PRESENT_24_31_REG;
            break;
        case CPLD_SFP_MASK_PRESENT_32_39:
            reg = CPLD_SFP_MASK_PRESENT_32_39_REG;
            break;
        case CPLD_SFP_MASK_PRESENT_40_47:
            reg = CPLD_SFP_MASK_PRESENT_40_47_REG;
            break;
        case CPLD_SFP_MASK_PRESENT_48_53:
            reg = CPLD_SFP_MASK_PRESENT_48_53_REG;
            break;
        //CPLD 2/3
        case DBG_CPLD_SFP_TX_FAULT_0_7:
            reg = DBG_CPLD_SFP_TX_FAULT_0_7_REG;
            break;
        case DBG_CPLD_SFP_TX_FAULT_8_15:
            reg = DBG_CPLD_SFP_TX_FAULT_8_15_REG;
            break;
        case DBG_CPLD_SFP_TX_FAULT_16_23:
            reg = DBG_CPLD_SFP_TX_FAULT_16_23_REG;
            break;
        case DBG_CPLD_SFP_TX_FAULT_24_31:
            reg = DBG_CPLD_SFP_TX_FAULT_24_31_REG;
            break;
        case DBG_CPLD_SFP_TX_FAULT_32_39:
            reg = DBG_CPLD_SFP_TX_FAULT_32_39_REG;
            break;
        case DBG_CPLD_SFP_TX_FAULT_40_47:
            reg = DBG_CPLD_SFP_TX_FAULT_40_47_REG;
            break;
        case DBG_CPLD_SFP_TX_FAULT_48_53:
            reg = DBG_CPLD_SFP_TX_FAULT_48_53_REG;
            break;
        case DBG_CPLD_SFP_RX_LOS_0_7:
            reg = DBG_CPLD_SFP_RX_LOS_0_7_REG;
            break;
        case DBG_CPLD_SFP_RX_LOS_8_15:
            reg = DBG_CPLD_SFP_RX_LOS_8_15_REG;
            break;
        case DBG_CPLD_SFP_RX_LOS_16_23:
            reg = DBG_CPLD_SFP_RX_LOS_16_23_REG;
            break;
        case DBG_CPLD_SFP_RX_LOS_24_31:
            reg = DBG_CPLD_SFP_RX_LOS_24_31_REG;
            break;
        case DBG_CPLD_SFP_RX_LOS_32_39:
            reg = DBG_CPLD_SFP_RX_LOS_32_39_REG;
            break;
        case DBG_CPLD_SFP_RX_LOS_40_47:
            reg = DBG_CPLD_SFP_RX_LOS_40_47_REG;
            break;
        case DBG_CPLD_SFP_RX_LOS_48_53:
            reg = DBG_CPLD_SFP_RX_LOS_48_53_REG;
            break;
        case DBG_CPLD_SFP_PRESENT_0_7:
            reg = DBG_CPLD_SFP_PRESENT_0_7_REG;
            break;
        case DBG_CPLD_SFP_PRESENT_8_15:
            reg = DBG_CPLD_SFP_PRESENT_8_15_REG;
            break;
        case DBG_CPLD_SFP_PRESENT_16_23:
            reg = DBG_CPLD_SFP_PRESENT_16_23_REG;
            break;
        case DBG_CPLD_SFP_PRESENT_24_31:
            reg = DBG_CPLD_SFP_PRESENT_24_31_REG;
            break;
        case DBG_CPLD_SFP_PRESENT_32_39:
            reg = DBG_CPLD_SFP_PRESENT_32_39_REG;
            break;
        case DBG_CPLD_SFP_PRESENT_40_47:
            reg = DBG_CPLD_SFP_PRESENT_40_47_REG;
            break;
        case DBG_CPLD_SFP_PRESENT_48_53:
            reg = DBG_CPLD_SFP_PRESENT_48_53_REG;
            break;

        default:
            return -EINVAL;
    }
    return write_cpld_reg(dev, buf, count, reg, mask);
}

/* get cpld register value */
static u8 _read_cpld_reg(struct device *dev,
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
static ssize_t read_cpld_reg(struct device *dev,
                    char *buf,
                    u8 reg,
                    u8 mask)
{
    int reg_val;

    reg_val = _read_cpld_reg(dev, reg, mask);
    if (unlikely(reg_val < 0)) {
        dev_err(dev, "read_cpld_reg() error, reg_val=%d\n", reg_val);
        return reg_val;
    } else {
        return sprintf(buf, "0x%02x\n", reg_val);
    }
}

/* set cpld register value */
static ssize_t write_cpld_reg(struct device *dev,
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
        reg_val_now = _read_cpld_reg(dev, reg, MASK_ALL);
        if (unlikely(reg_val_now < 0)) {
            dev_err(dev, "write_cpld_reg() error, reg_val_now=%d\n", reg_val_now);
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
        dev_err(dev, "write_cpld_reg() error, return=%d\n", ret);
        return ret;
    }

    return count;
}

/* get qsfp port config register value */
static ssize_t read_cpld_version_h(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    if (attr->index >= CPLD_VERSION_H) {
        return sprintf(buf, "%d.%02d.%03d",
                _read_cpld_reg(dev, CPLD_VERSION_REG, MASK_CPLD_MAJOR_VER),
                _read_cpld_reg(dev, CPLD_VERSION_REG, MASK_CPLD_MINOR_VER),
                _read_cpld_reg(dev, CPLD_BUILD_REG, MASK_ALL));
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

    if (INVALID(ret, cpld1, cpld3)) {
        dev_info(&client->dev,
            "cpld id %d(device) not valid\n", ret);
        //status = -EPERM;
        //goto exit;
    }

#if 0
    /* change client name for each cpld with index */
    snprintf(client->name, sizeof(client->name), "%s_%d", client->name,
            data->index);
#endif

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
    }

    cpld_remove_client(client);
    return 0;
}

MODULE_DEVICE_TABLE(i2c, cpld_id);

static struct i2c_driver cpld_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name = "x86_64_ufispace_s9500_54cf_cpld",
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

MODULE_AUTHOR("Wade He <wade.ce.he@ufispace.com>");
MODULE_DESCRIPTION("x86_64_ufispace_s9500_54cf_cpld driver");
MODULE_LICENSE("GPL");

module_init(cpld_init);
module_exit(cpld_exit);