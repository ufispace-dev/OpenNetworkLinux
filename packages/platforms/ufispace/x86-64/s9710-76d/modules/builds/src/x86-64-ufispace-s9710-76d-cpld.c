/*
 * A i2c cpld driver for the ufispace_s9710_76d
 *
 * Copyright (C) 2017-2019 UfiSpace Technology Corporation.
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
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/dmi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/version.h>
#include "x86-64-ufispace-s9710-76d-cpld.h"

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

    CPLD_MAC_INTR,
    CPLD_PHY_INTR,
    CPLD_CPLDX_INTR,
    CPLD_MAC_THERMAL_INTR,
    CPLD_MISC_INTR,
    CPLD_CPU_INTR,
    CPLD_MAC_MASK,
    CPLD_PHY_MASK,
    CPLD_CPLDX_MASK,
    CPLD_MAC_THERMAL_MASK,
    CPLD_MISC_MASK,
    CPLD_CPU_MASK,
    CPLD_MAC_EVT,
    CPLD_PHY_EVT,
    CPLD_CPLDX_EVT,
    CPLD_MAC_THERMAL_EVT,
    CPLD_MISC_EVT,
    CPLD_EVT_CTRL,
    CPLD_MAC_RESET,
    CPLD_PHY_RESET,
    CPLD_BMC_RESET,
    CPLD_USB_RESET,
    CPLD_CPLDX_RESET,
    CPLD_I2C_MUX_NIF_RESET,
    CPLD_MISC_RESET,
    CPLD_BRD_PRESENT,
    CPLD_PSU_STATUS,
    CPLD_SYSTEM_PWR,
    CPLD_MAC_SYNCE,
    CPLD_MAC_ROV,
    CPLD_MUX_CTRL,
    CPLD_SYSTEM_LED_0,
    CPLD_SYSTEM_LED_1,
    CPLD_SYSTEM_LED_2,
    CPLD_LED_CLEAR,
    DBG_CPLD_MAC_INTR,
    DBG_CPLD_PHY_INTR,
    DBG_CPLD_CPLDX_INTR,
    DBG_CPLD_MAC_THERMAL_INTR,
    DBG_CPLD_MISC_INTR,

    //CPLD 2-5
    CPLD_QSFPDD_INTR_PORT_0,
    CPLD_QSFPDD_INTR_PORT_1,
    CPLD_QSFPDD_INTR_PORT_2,
    CPLD_QSFPDD_INTR_PRESENT_0,
    CPLD_QSFPDD_INTR_PRESENT_1,
    CPLD_QSFPDD_INTR_PRESENT_2,
    CPLD_QSFPDD_INTR_FUSE_0,
    CPLD_QSFPDD_INTR_FUSE_1,
    CPLD_QSFPDD_INTR_FUSE_2,
    CPLD_QSFPDD_MASK_PORT_0,
    CPLD_QSFPDD_MASK_PORT_1,
    CPLD_QSFPDD_MASK_PORT_2,
    CPLD_QSFPDD_MASK_PRESENT_0,
    CPLD_QSFPDD_MASK_PRESENT_1,
    CPLD_QSFPDD_MASK_PRESENT_2,
    CPLD_QSFPDD_MASK_FUSE_0,
    CPLD_QSFPDD_MASK_FUSE_1,
    CPLD_QSFPDD_MASK_FUSE_2,
    CPLD_QSFPDD_EVT_PORT_0,
    CPLD_QSFPDD_EVT_PORT_1,
    CPLD_QSFPDD_EVT_PORT_2,
    CPLD_QSFPDD_EVT_PRESENT_0,
    CPLD_QSFPDD_EVT_PRESENT_1,
    CPLD_QSFPDD_EVT_PRESENT_2,
    CPLD_QSFPDD_EVT_FUSE_0,
    CPLD_QSFPDD_EVT_FUSE_1,
    CPLD_QSFPDD_EVT_FUSE_2,
    CPLD_QSFPDD_RESET_0,
    CPLD_QSFPDD_RESET_1,
    CPLD_QSFPDD_RESET_2,
    CPLD_QSFPDD_LPMODE_0,
    CPLD_QSFPDD_LPMODE_1,
    CPLD_QSFPDD_LPMODE_2,
    DBG_CPLD_QSFPDD_INTR_PORT_0,
    DBG_CPLD_QSFPDD_INTR_PORT_1,
    DBG_CPLD_QSFPDD_INTR_PORT_2,
    DBG_CPLD_QSFPDD_INTR_PRESENT_0,
    DBG_CPLD_QSFPDD_INTR_PRESENT_1,
    DBG_CPLD_QSFPDD_INTR_PRESENT_2,
    DBG_CPLD_QSFPDD_INTR_FUSE_0,
    DBG_CPLD_QSFPDD_INTR_FUSE_1,
    DBG_CPLD_QSFPDD_INTR_FUSE_2,

    //CPLD 2
    CPLD_OP2_INTR,
    CPLD_OP2_MASK,
    CPLD_OP2_EVT,
    CPLD_OP2_RESET,
    CPLD_OP2_PWR,
    CPLD_MISC_PWR,
    DBG_CPLD_OP2_INTR,

    //CPLD 2/3
    CPLD_SFP_STATUS,
    CPLD_SFP_MASK,
    CPLD_SFP_EVT,
    CPLD_SFP_CONFIG,
    DBG_CPLD_SFP_STATUS,

    //CPLD 4
    CPLD_I2C_MUX_FAB_RESET,

    //CPLD 4/5
    CPLD_QSFPDD_FAB_LED_0,
    CPLD_QSFPDD_FAB_LED_1,
    CPLD_QSFPDD_FAB_LED_2,
    CPLD_QSFPDD_FAB_LED_3,
    CPLD_QSFPDD_FAB_LED_4,
    CPLD_QSFPDD_FAB_LED_5,
    CPLD_QSFPDD_FAB_LED_6,
    CPLD_QSFPDD_FAB_LED_7,
    CPLD_QSFPDD_FAB_LED_8,
    CPLD_QSFPDD_FAB_LED_9,

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
    { "s9710_76d_cpld1",  cpld1 },
    { "s9710_76d_cpld2",  cpld2 },
    { "s9710_76d_cpld3",  cpld3 },
    { "s9710_76d_cpld4",  cpld4 },
    { "s9710_76d_cpld5",  cpld5 },
    {}
};

char bsp_debug[2]="0";
u8 enable_log_read=LOG_DISABLE;
u8 enable_log_write=LOG_DISABLE;

/* Addresses scanned for cpld */
static const unsigned short cpld_i2c_addr[] = { 0x30, 0x31, 0x32, 0x33, 0x34, I2C_CLIENT_END };

/* define all support register access of cpld in attribute */

static _SENSOR_DEVICE_ATTR_RO(cpld_board_id_0,  cpld_callback, CPLD_BOARD_ID_0);
static _SENSOR_DEVICE_ATTR_RO(cpld_board_id_1,  cpld_callback, CPLD_BOARD_ID_1);
static _SENSOR_DEVICE_ATTR_RO(cpld_version,     cpld_callback, CPLD_VERSION);
static _SENSOR_DEVICE_ATTR_RO(cpld_build,       cpld_callback, CPLD_BUILD);
static _SENSOR_DEVICE_ATTR_RO(cpld_id,          cpld_callback, CPLD_ID);

static _SENSOR_DEVICE_ATTR_RO(cpld_major_ver,   cpld_callback, CPLD_MAJOR_VER);
static _SENSOR_DEVICE_ATTR_RO(cpld_minor_ver,   cpld_callback, CPLD_MINOR_VER);
static _SENSOR_DEVICE_ATTR_RO(cpld_build_ver,   cpld_callback, CPLD_BUILD_VER);
static _SENSOR_DEVICE_ATTR_RO(cpld_version_h,   cpld_version_h, CPLD_VERSION_H);

static _SENSOR_DEVICE_ATTR_RO(cpld_mac_intr,    cpld_callback, CPLD_MAC_INTR);
static _SENSOR_DEVICE_ATTR_RO(cpld_phy_intr,    cpld_callback, CPLD_PHY_INTR);
static _SENSOR_DEVICE_ATTR_RO(cpld_cpldx_intr,  cpld_callback, CPLD_CPLDX_INTR);
static _SENSOR_DEVICE_ATTR_RO(cpld_mac_thermal_intr, cpld_callback, CPLD_MAC_THERMAL_INTR);
static _SENSOR_DEVICE_ATTR_RO(cpld_misc_intr,   cpld_callback, CPLD_MISC_INTR);
static _SENSOR_DEVICE_ATTR_RO(cpld_cpu_intr,    cpld_callback, CPLD_CPU_INTR);

static _SENSOR_DEVICE_ATTR_RW(cpld_mac_mask,    cpld_callback, CPLD_MAC_MASK);
static _SENSOR_DEVICE_ATTR_RW(cpld_phy_mask,    cpld_callback, CPLD_PHY_MASK);
static _SENSOR_DEVICE_ATTR_RW(cpld_cpldx_mask,  cpld_callback, CPLD_CPLDX_MASK);
static _SENSOR_DEVICE_ATTR_RW(cpld_mac_thermal_mask, cpld_callback, CPLD_MAC_THERMAL_MASK);
static _SENSOR_DEVICE_ATTR_RW(cpld_misc_mask,   cpld_callback, CPLD_MISC_MASK);
static _SENSOR_DEVICE_ATTR_RW(cpld_cpu_mask,    cpld_callback, CPLD_CPU_MASK);

static _SENSOR_DEVICE_ATTR_RO(cpld_mac_evt,     cpld_callback, CPLD_MAC_EVT);
static _SENSOR_DEVICE_ATTR_RO(cpld_phy_evt,     cpld_callback, CPLD_PHY_EVT);
static _SENSOR_DEVICE_ATTR_RO(cpld_cpldx_evt,   cpld_callback, CPLD_CPLDX_EVT);
static _SENSOR_DEVICE_ATTR_RO(cpld_mac_thermal_evt, cpld_callback, CPLD_MAC_THERMAL_EVT);
static _SENSOR_DEVICE_ATTR_RO(cpld_misc_evt,    cpld_callback, CPLD_MISC_EVT);
static _SENSOR_DEVICE_ATTR_RW(cpld_evt_ctrl,    cpld_callback, CPLD_EVT_CTRL);

static _SENSOR_DEVICE_ATTR_RW(cpld_mac_reset,   cpld_callback, CPLD_MAC_RESET);
static _SENSOR_DEVICE_ATTR_RW(cpld_phy_reset,   cpld_callback, CPLD_PHY_RESET);
static _SENSOR_DEVICE_ATTR_RW(cpld_bmc_reset,   cpld_callback, CPLD_BMC_RESET);
static _SENSOR_DEVICE_ATTR_RW(cpld_usb_reset,   cpld_callback, CPLD_USB_RESET);
static _SENSOR_DEVICE_ATTR_RW(cpld_cpldx_reset, cpld_callback, CPLD_CPLDX_RESET);
static _SENSOR_DEVICE_ATTR_RW(cpld_i2c_mux_nif_reset, cpld_callback, CPLD_I2C_MUX_NIF_RESET);
static _SENSOR_DEVICE_ATTR_RW(cpld_misc_reset,  cpld_callback, CPLD_MISC_RESET);

static _SENSOR_DEVICE_ATTR_RO(cpld_brd_present, cpld_callback, CPLD_BRD_PRESENT);
static _SENSOR_DEVICE_ATTR_RO(cpld_psu_status,  cpld_callback, CPLD_PSU_STATUS);
static _SENSOR_DEVICE_ATTR_RO(cpld_system_pwr,  cpld_callback, CPLD_SYSTEM_PWR);

static _SENSOR_DEVICE_ATTR_RO(cpld_mac_synce,   cpld_callback, CPLD_MAC_SYNCE);
static _SENSOR_DEVICE_ATTR_RO(cpld_mac_rov,     cpld_callback, CPLD_MAC_ROV);

static _SENSOR_DEVICE_ATTR_RW(cpld_mux_ctrl,    cpld_callback, CPLD_MUX_CTRL);

static _SENSOR_DEVICE_ATTR_RW(cpld_system_led_0, cpld_callback, CPLD_SYSTEM_LED_0);
static _SENSOR_DEVICE_ATTR_RW(cpld_system_led_1, cpld_callback, CPLD_SYSTEM_LED_1);
static _SENSOR_DEVICE_ATTR_RW(cpld_system_led_2, cpld_callback, CPLD_SYSTEM_LED_2);
static _SENSOR_DEVICE_ATTR_RW(cpld_led_clear,    cpld_callback, CPLD_LED_CLEAR);

static _SENSOR_DEVICE_ATTR_RO(dbg_cpld_mac_intr,   cpld_callback, DBG_CPLD_MAC_INTR);
static _SENSOR_DEVICE_ATTR_RO(dbg_cpld_phy_intr,   cpld_callback, DBG_CPLD_PHY_INTR);
static _SENSOR_DEVICE_ATTR_RO(dbg_cpld_cpldx_intr, cpld_callback, DBG_CPLD_CPLDX_INTR);
static _SENSOR_DEVICE_ATTR_RO(dbg_cpld_mac_thermal_intr, cpld_callback, DBG_CPLD_MAC_THERMAL_INTR);
static _SENSOR_DEVICE_ATTR_RO(dbg_cpld_misc_intr, cpld_callback, DBG_CPLD_MISC_INTR);

//CPLD 2-5

static _SENSOR_DEVICE_ATTR_RO(cpld_qsfpdd_intr_port_0, cpld_callback, CPLD_QSFPDD_INTR_PORT_0);
static _SENSOR_DEVICE_ATTR_RO(cpld_qsfpdd_intr_port_1, cpld_callback, CPLD_QSFPDD_INTR_PORT_1);
static _SENSOR_DEVICE_ATTR_RO(cpld_qsfpdd_intr_port_2, cpld_callback, CPLD_QSFPDD_INTR_PORT_2);

static _SENSOR_DEVICE_ATTR_RO(cpld_qsfpdd_intr_present_0, cpld_callback, CPLD_QSFPDD_INTR_PRESENT_0);
static _SENSOR_DEVICE_ATTR_RO(cpld_qsfpdd_intr_present_1, cpld_callback, CPLD_QSFPDD_INTR_PRESENT_1);
static _SENSOR_DEVICE_ATTR_RO(cpld_qsfpdd_intr_present_2, cpld_callback, CPLD_QSFPDD_INTR_PRESENT_2);

static _SENSOR_DEVICE_ATTR_RO(cpld_qsfpdd_intr_fuse_0, cpld_callback, CPLD_QSFPDD_INTR_FUSE_0);
static _SENSOR_DEVICE_ATTR_RO(cpld_qsfpdd_intr_fuse_1, cpld_callback, CPLD_QSFPDD_INTR_FUSE_1);
static _SENSOR_DEVICE_ATTR_RO(cpld_qsfpdd_intr_fuse_2, cpld_callback, CPLD_QSFPDD_INTR_FUSE_2);

static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_mask_port_0, cpld_callback, CPLD_QSFPDD_MASK_PORT_0);
static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_mask_port_1, cpld_callback, CPLD_QSFPDD_MASK_PORT_1);
static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_mask_port_2, cpld_callback, CPLD_QSFPDD_MASK_PORT_2);

static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_mask_present_0, cpld_callback, CPLD_QSFPDD_MASK_PRESENT_0);
static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_mask_present_1, cpld_callback, CPLD_QSFPDD_MASK_PRESENT_1);
static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_mask_present_2, cpld_callback, CPLD_QSFPDD_MASK_PRESENT_2);

static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_mask_fuse_0, cpld_callback, CPLD_QSFPDD_MASK_FUSE_0);
static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_mask_fuse_1, cpld_callback, CPLD_QSFPDD_MASK_FUSE_1);
static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_mask_fuse_2, cpld_callback, CPLD_QSFPDD_MASK_FUSE_2);

static _SENSOR_DEVICE_ATTR_RO(cpld_qsfpdd_evt_port_0, cpld_callback, CPLD_QSFPDD_EVT_PORT_0);
static _SENSOR_DEVICE_ATTR_RO(cpld_qsfpdd_evt_port_1, cpld_callback, CPLD_QSFPDD_EVT_PORT_1);
static _SENSOR_DEVICE_ATTR_RO(cpld_qsfpdd_evt_port_2, cpld_callback, CPLD_QSFPDD_EVT_PORT_2);

static _SENSOR_DEVICE_ATTR_RO(cpld_qsfpdd_evt_present_0, cpld_callback, CPLD_QSFPDD_EVT_PRESENT_0);
static _SENSOR_DEVICE_ATTR_RO(cpld_qsfpdd_evt_present_1, cpld_callback, CPLD_QSFPDD_EVT_PRESENT_1);
static _SENSOR_DEVICE_ATTR_RO(cpld_qsfpdd_evt_present_2, cpld_callback, CPLD_QSFPDD_EVT_PRESENT_2);

static _SENSOR_DEVICE_ATTR_RO(cpld_qsfpdd_evt_fuse_0, cpld_callback, CPLD_QSFPDD_EVT_FUSE_0);
static _SENSOR_DEVICE_ATTR_RO(cpld_qsfpdd_evt_fuse_1, cpld_callback, CPLD_QSFPDD_EVT_FUSE_1);
static _SENSOR_DEVICE_ATTR_RO(cpld_qsfpdd_evt_fuse_2, cpld_callback, CPLD_QSFPDD_EVT_FUSE_2);


static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_reset_0, cpld_callback, CPLD_QSFPDD_RESET_0);
static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_reset_1, cpld_callback, CPLD_QSFPDD_RESET_1);
static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_reset_2, cpld_callback, CPLD_QSFPDD_RESET_2);

static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_lpmode_0, cpld_callback, CPLD_QSFPDD_LPMODE_0);
static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_lpmode_1, cpld_callback, CPLD_QSFPDD_LPMODE_1);
static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_lpmode_2, cpld_callback, CPLD_QSFPDD_LPMODE_2);

static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_qsfpdd_intr_port_0, cpld_callback, DBG_CPLD_QSFPDD_INTR_PORT_0);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_qsfpdd_intr_port_1, cpld_callback, DBG_CPLD_QSFPDD_INTR_PORT_1);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_qsfpdd_intr_port_2, cpld_callback, DBG_CPLD_QSFPDD_INTR_PORT_2);

static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_qsfpdd_intr_present_0, cpld_callback, DBG_CPLD_QSFPDD_INTR_PRESENT_0);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_qsfpdd_intr_present_1, cpld_callback, DBG_CPLD_QSFPDD_INTR_PRESENT_1);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_qsfpdd_intr_present_2, cpld_callback, DBG_CPLD_QSFPDD_INTR_PRESENT_2);

static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_qsfpdd_intr_fuse_0, cpld_callback, DBG_CPLD_QSFPDD_INTR_FUSE_0);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_qsfpdd_intr_fuse_1, cpld_callback, DBG_CPLD_QSFPDD_INTR_FUSE_1);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_qsfpdd_intr_fuse_2, cpld_callback, DBG_CPLD_QSFPDD_INTR_FUSE_2);

//CPLD 2
static _SENSOR_DEVICE_ATTR_RO(cpld_op2_intr,     cpld_callback, CPLD_OP2_INTR);
static _SENSOR_DEVICE_ATTR_RW(cpld_op2_mask,     cpld_callback, CPLD_OP2_MASK);
static _SENSOR_DEVICE_ATTR_RO(cpld_op2_evt,      cpld_callback, CPLD_OP2_EVT);
static _SENSOR_DEVICE_ATTR_RW(cpld_op2_reset,    cpld_callback, CPLD_OP2_RESET);
static _SENSOR_DEVICE_ATTR_RO(cpld_op2_pwr,      cpld_callback, CPLD_OP2_PWR);
static _SENSOR_DEVICE_ATTR_RO(cpld_misc_pwr,     cpld_callback, CPLD_MISC_PWR);
static _SENSOR_DEVICE_ATTR_RW(dbg_cpld_op2_intr, cpld_callback, DBG_CPLD_OP2_INTR);

//CPLD 2/3
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_status, cpld_callback, CPLD_SFP_STATUS);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_mask,   cpld_callback, CPLD_SFP_MASK);
static _SENSOR_DEVICE_ATTR_RO(cpld_sfp_evt,    cpld_callback, CPLD_SFP_EVT);
static _SENSOR_DEVICE_ATTR_RW(cpld_sfp_config, cpld_callback, CPLD_SFP_CONFIG);
static _SENSOR_DEVICE_ATTR_RO(dbg_cpld_sfp_status, cpld_callback, DBG_CPLD_SFP_STATUS);

//CPLD 4
static _SENSOR_DEVICE_ATTR_RW(cpld_i2c_mux_fab_reset, cpld_callback, CPLD_I2C_MUX_FAB_RESET);

//CPLD 4/5
static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_fab_led_0, cpld_callback, CPLD_QSFPDD_FAB_LED_0);
static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_fab_led_1, cpld_callback, CPLD_QSFPDD_FAB_LED_1);
static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_fab_led_2, cpld_callback, CPLD_QSFPDD_FAB_LED_2);
static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_fab_led_3, cpld_callback, CPLD_QSFPDD_FAB_LED_3);
static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_fab_led_4, cpld_callback, CPLD_QSFPDD_FAB_LED_4);
static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_fab_led_5, cpld_callback, CPLD_QSFPDD_FAB_LED_5);
static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_fab_led_6, cpld_callback, CPLD_QSFPDD_FAB_LED_6);
static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_fab_led_7, cpld_callback, CPLD_QSFPDD_FAB_LED_7);
static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_fab_led_8, cpld_callback, CPLD_QSFPDD_FAB_LED_8);
static _SENSOR_DEVICE_ATTR_RW(cpld_qsfpdd_fab_led_9, cpld_callback, CPLD_QSFPDD_FAB_LED_9);

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
    _DEVICE_ATTR(cpld_phy_intr),
    _DEVICE_ATTR(cpld_cpldx_intr),
    _DEVICE_ATTR(cpld_mac_thermal_intr),
    _DEVICE_ATTR(cpld_misc_intr),
    _DEVICE_ATTR(cpld_cpu_intr),

    _DEVICE_ATTR(cpld_mac_mask),
    _DEVICE_ATTR(cpld_phy_mask),
    _DEVICE_ATTR(cpld_cpldx_mask),
    _DEVICE_ATTR(cpld_mac_thermal_mask),
    _DEVICE_ATTR(cpld_misc_mask),
    _DEVICE_ATTR(cpld_cpu_mask),

    _DEVICE_ATTR(cpld_mac_evt),
    _DEVICE_ATTR(cpld_phy_evt),
    _DEVICE_ATTR(cpld_cpldx_evt),
    _DEVICE_ATTR(cpld_mac_thermal_evt),
    _DEVICE_ATTR(cpld_misc_evt),
    _DEVICE_ATTR(cpld_evt_ctrl),

    _DEVICE_ATTR(cpld_mac_reset),
    _DEVICE_ATTR(cpld_phy_reset),
    _DEVICE_ATTR(cpld_bmc_reset),
    _DEVICE_ATTR(cpld_usb_reset),
    _DEVICE_ATTR(cpld_cpldx_reset),
    _DEVICE_ATTR(cpld_i2c_mux_nif_reset),
    _DEVICE_ATTR(cpld_misc_reset),

    _DEVICE_ATTR(cpld_brd_present),
    _DEVICE_ATTR(cpld_psu_status),
    _DEVICE_ATTR(cpld_system_pwr),

    _DEVICE_ATTR(cpld_mac_synce),
    _DEVICE_ATTR(cpld_mac_rov),

    _DEVICE_ATTR(cpld_mux_ctrl),

    _DEVICE_ATTR(cpld_system_led_0),
    _DEVICE_ATTR(cpld_system_led_1),
    _DEVICE_ATTR(cpld_system_led_2),
    _DEVICE_ATTR(cpld_led_clear),
    _DEVICE_ATTR(bsp_debug),

    _DEVICE_ATTR(dbg_cpld_mac_intr),
    _DEVICE_ATTR(dbg_cpld_phy_intr),
    _DEVICE_ATTR(dbg_cpld_cpldx_intr),
    _DEVICE_ATTR(dbg_cpld_mac_thermal_intr),
    _DEVICE_ATTR(dbg_cpld_misc_intr),
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

    //CPLD 2-5

    _DEVICE_ATTR(cpld_qsfpdd_intr_port_0),
    _DEVICE_ATTR(cpld_qsfpdd_intr_port_1),
    _DEVICE_ATTR(cpld_qsfpdd_intr_port_2),

    _DEVICE_ATTR(cpld_qsfpdd_intr_present_0),
    _DEVICE_ATTR(cpld_qsfpdd_intr_present_1),
    _DEVICE_ATTR(cpld_qsfpdd_intr_present_2),

    _DEVICE_ATTR(cpld_qsfpdd_intr_fuse_0),
    _DEVICE_ATTR(cpld_qsfpdd_intr_fuse_1),
    _DEVICE_ATTR(cpld_qsfpdd_intr_fuse_2),

    _DEVICE_ATTR(cpld_qsfpdd_mask_port_0),
    _DEVICE_ATTR(cpld_qsfpdd_mask_port_1),
    _DEVICE_ATTR(cpld_qsfpdd_mask_port_2),

    _DEVICE_ATTR(cpld_qsfpdd_mask_present_0),
    _DEVICE_ATTR(cpld_qsfpdd_mask_present_1),
    _DEVICE_ATTR(cpld_qsfpdd_mask_present_2),

    _DEVICE_ATTR(cpld_qsfpdd_mask_fuse_0),
    _DEVICE_ATTR(cpld_qsfpdd_mask_fuse_1),
    _DEVICE_ATTR(cpld_qsfpdd_mask_fuse_2),

    _DEVICE_ATTR(cpld_qsfpdd_evt_port_0),
    _DEVICE_ATTR(cpld_qsfpdd_evt_port_1),
    _DEVICE_ATTR(cpld_qsfpdd_evt_port_2),

    _DEVICE_ATTR(cpld_qsfpdd_evt_present_0),
    _DEVICE_ATTR(cpld_qsfpdd_evt_present_1),
    _DEVICE_ATTR(cpld_qsfpdd_evt_present_2),

    _DEVICE_ATTR(cpld_qsfpdd_evt_fuse_0),
    _DEVICE_ATTR(cpld_qsfpdd_evt_fuse_1),
    _DEVICE_ATTR(cpld_qsfpdd_evt_fuse_2),

    _DEVICE_ATTR(cpld_evt_ctrl),

    _DEVICE_ATTR(cpld_qsfpdd_reset_0),
    _DEVICE_ATTR(cpld_qsfpdd_reset_1),
    _DEVICE_ATTR(cpld_qsfpdd_reset_2),

    _DEVICE_ATTR(cpld_qsfpdd_lpmode_0),
    _DEVICE_ATTR(cpld_qsfpdd_lpmode_1),
    _DEVICE_ATTR(cpld_qsfpdd_lpmode_2),

    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_port_0),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_port_1),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_port_2),

    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_present_0),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_present_1),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_present_2),

    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_fuse_0),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_fuse_1),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_fuse_2),

    //CPLD2 only

    _DEVICE_ATTR(cpld_op2_intr),
    _DEVICE_ATTR(cpld_op2_mask),
    _DEVICE_ATTR(cpld_op2_evt),
    _DEVICE_ATTR(cpld_op2_reset),
    _DEVICE_ATTR(cpld_op2_pwr),
    _DEVICE_ATTR(cpld_misc_pwr),
    _DEVICE_ATTR(dbg_cpld_op2_intr),

    //CPLD 2/3

    _DEVICE_ATTR(cpld_sfp_status),
    _DEVICE_ATTR(cpld_sfp_mask),
    _DEVICE_ATTR(cpld_sfp_evt),
    _DEVICE_ATTR(cpld_sfp_config),
    _DEVICE_ATTR(dbg_cpld_sfp_status),
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

    //CPLD 2-5

    _DEVICE_ATTR(cpld_qsfpdd_intr_port_0),
    _DEVICE_ATTR(cpld_qsfpdd_intr_port_1),
    _DEVICE_ATTR(cpld_qsfpdd_intr_port_2),

    _DEVICE_ATTR(cpld_qsfpdd_intr_present_0),
    _DEVICE_ATTR(cpld_qsfpdd_intr_present_1),
    _DEVICE_ATTR(cpld_qsfpdd_intr_present_2),

    _DEVICE_ATTR(cpld_qsfpdd_intr_fuse_0),
    _DEVICE_ATTR(cpld_qsfpdd_intr_fuse_1),
    _DEVICE_ATTR(cpld_qsfpdd_intr_fuse_2),

    _DEVICE_ATTR(cpld_qsfpdd_mask_port_0),
    _DEVICE_ATTR(cpld_qsfpdd_mask_port_1),
    _DEVICE_ATTR(cpld_qsfpdd_mask_port_2),

    _DEVICE_ATTR(cpld_qsfpdd_mask_present_0),
    _DEVICE_ATTR(cpld_qsfpdd_mask_present_1),
    _DEVICE_ATTR(cpld_qsfpdd_mask_present_2),

    _DEVICE_ATTR(cpld_qsfpdd_mask_fuse_0),
    _DEVICE_ATTR(cpld_qsfpdd_mask_fuse_1),
    _DEVICE_ATTR(cpld_qsfpdd_mask_fuse_2),

    _DEVICE_ATTR(cpld_qsfpdd_evt_port_0),
    _DEVICE_ATTR(cpld_qsfpdd_evt_port_1),
    _DEVICE_ATTR(cpld_qsfpdd_evt_port_2),

    _DEVICE_ATTR(cpld_qsfpdd_evt_present_0),
    _DEVICE_ATTR(cpld_qsfpdd_evt_present_1),
    _DEVICE_ATTR(cpld_qsfpdd_evt_present_2),

    _DEVICE_ATTR(cpld_qsfpdd_evt_fuse_0),
    _DEVICE_ATTR(cpld_qsfpdd_evt_fuse_1),
    _DEVICE_ATTR(cpld_qsfpdd_evt_fuse_2),

    _DEVICE_ATTR(cpld_evt_ctrl),

    _DEVICE_ATTR(cpld_qsfpdd_reset_0),
    _DEVICE_ATTR(cpld_qsfpdd_reset_1),
    _DEVICE_ATTR(cpld_qsfpdd_reset_2),

    _DEVICE_ATTR(cpld_qsfpdd_lpmode_0),
    _DEVICE_ATTR(cpld_qsfpdd_lpmode_1),
    _DEVICE_ATTR(cpld_qsfpdd_lpmode_2),

    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_port_0),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_port_1),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_port_2),

    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_present_0),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_present_1),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_present_2),

    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_fuse_0),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_fuse_1),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_fuse_2),

    //CPLD 2/3

    _DEVICE_ATTR(cpld_sfp_status),
    _DEVICE_ATTR(cpld_sfp_mask),
    _DEVICE_ATTR(cpld_sfp_evt),
    _DEVICE_ATTR(cpld_sfp_config),
    _DEVICE_ATTR(dbg_cpld_sfp_status),
    NULL
};

/* cpld 4 */
static struct attribute *cpld4_attributes[] = {
    _DEVICE_ATTR(cpld_version),
    _DEVICE_ATTR(cpld_build),
    _DEVICE_ATTR(cpld_id),

    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),

    //CPLD 2-5

    _DEVICE_ATTR(cpld_qsfpdd_intr_port_0),
    _DEVICE_ATTR(cpld_qsfpdd_intr_port_1),
    _DEVICE_ATTR(cpld_qsfpdd_intr_port_2),

    _DEVICE_ATTR(cpld_qsfpdd_intr_present_0),
    _DEVICE_ATTR(cpld_qsfpdd_intr_present_1),
    _DEVICE_ATTR(cpld_qsfpdd_intr_present_2),

    _DEVICE_ATTR(cpld_qsfpdd_intr_fuse_0),
    _DEVICE_ATTR(cpld_qsfpdd_intr_fuse_1),
    _DEVICE_ATTR(cpld_qsfpdd_intr_fuse_2),

    _DEVICE_ATTR(cpld_qsfpdd_mask_port_0),
    _DEVICE_ATTR(cpld_qsfpdd_mask_port_1),
    _DEVICE_ATTR(cpld_qsfpdd_mask_port_2),

    _DEVICE_ATTR(cpld_qsfpdd_mask_present_0),
    _DEVICE_ATTR(cpld_qsfpdd_mask_present_1),
    _DEVICE_ATTR(cpld_qsfpdd_mask_present_2),

    _DEVICE_ATTR(cpld_qsfpdd_mask_fuse_0),
    _DEVICE_ATTR(cpld_qsfpdd_mask_fuse_1),
    _DEVICE_ATTR(cpld_qsfpdd_mask_fuse_2),

    _DEVICE_ATTR(cpld_qsfpdd_evt_port_0),
    _DEVICE_ATTR(cpld_qsfpdd_evt_port_1),
    _DEVICE_ATTR(cpld_qsfpdd_evt_port_2),

    _DEVICE_ATTR(cpld_qsfpdd_evt_present_0),
    _DEVICE_ATTR(cpld_qsfpdd_evt_present_1),
    _DEVICE_ATTR(cpld_qsfpdd_evt_present_2),

    _DEVICE_ATTR(cpld_qsfpdd_evt_fuse_0),
    _DEVICE_ATTR(cpld_qsfpdd_evt_fuse_1),
    _DEVICE_ATTR(cpld_qsfpdd_evt_fuse_2),

    _DEVICE_ATTR(cpld_evt_ctrl),

    _DEVICE_ATTR(cpld_qsfpdd_reset_0),
    _DEVICE_ATTR(cpld_qsfpdd_reset_1),
    _DEVICE_ATTR(cpld_qsfpdd_reset_2),

    _DEVICE_ATTR(cpld_qsfpdd_lpmode_0),
    _DEVICE_ATTR(cpld_qsfpdd_lpmode_1),
    _DEVICE_ATTR(cpld_qsfpdd_lpmode_2),

    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_port_0),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_port_1),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_port_2),

    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_present_0),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_present_1),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_present_2),

    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_fuse_0),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_fuse_1),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_fuse_2),

    //CPLD 4

    _DEVICE_ATTR(cpld_i2c_mux_fab_reset),

    //CPLD 4/5

    _DEVICE_ATTR(cpld_qsfpdd_fab_led_0),
    _DEVICE_ATTR(cpld_qsfpdd_fab_led_1),
    _DEVICE_ATTR(cpld_qsfpdd_fab_led_2),
    _DEVICE_ATTR(cpld_qsfpdd_fab_led_3),
    _DEVICE_ATTR(cpld_qsfpdd_fab_led_4),
    _DEVICE_ATTR(cpld_qsfpdd_fab_led_5),
    _DEVICE_ATTR(cpld_qsfpdd_fab_led_6),
    _DEVICE_ATTR(cpld_qsfpdd_fab_led_7),
    _DEVICE_ATTR(cpld_qsfpdd_fab_led_8),
    _DEVICE_ATTR(cpld_qsfpdd_fab_led_9),
    NULL
};

/* cpld 5 */
static struct attribute *cpld5_attributes[] = {
    _DEVICE_ATTR(cpld_version),
    _DEVICE_ATTR(cpld_build),
    _DEVICE_ATTR(cpld_id),

    _DEVICE_ATTR(cpld_major_ver),
    _DEVICE_ATTR(cpld_minor_ver),
    _DEVICE_ATTR(cpld_build_ver),
    _DEVICE_ATTR(cpld_version_h),
    //CPLD 2-5

    _DEVICE_ATTR(cpld_qsfpdd_intr_port_0),
    _DEVICE_ATTR(cpld_qsfpdd_intr_port_1),
    _DEVICE_ATTR(cpld_qsfpdd_intr_port_2),

    _DEVICE_ATTR(cpld_qsfpdd_intr_present_0),
    _DEVICE_ATTR(cpld_qsfpdd_intr_present_1),
    _DEVICE_ATTR(cpld_qsfpdd_intr_present_2),

    _DEVICE_ATTR(cpld_qsfpdd_intr_fuse_0),
    _DEVICE_ATTR(cpld_qsfpdd_intr_fuse_1),
    _DEVICE_ATTR(cpld_qsfpdd_intr_fuse_2),

    _DEVICE_ATTR(cpld_qsfpdd_mask_port_0),
    _DEVICE_ATTR(cpld_qsfpdd_mask_port_1),
    _DEVICE_ATTR(cpld_qsfpdd_mask_port_2),

    _DEVICE_ATTR(cpld_qsfpdd_mask_present_0),
    _DEVICE_ATTR(cpld_qsfpdd_mask_present_1),
    _DEVICE_ATTR(cpld_qsfpdd_mask_present_2),

    _DEVICE_ATTR(cpld_qsfpdd_mask_fuse_0),
    _DEVICE_ATTR(cpld_qsfpdd_mask_fuse_1),
    _DEVICE_ATTR(cpld_qsfpdd_mask_fuse_2),

    _DEVICE_ATTR(cpld_qsfpdd_evt_port_0),
    _DEVICE_ATTR(cpld_qsfpdd_evt_port_1),
    _DEVICE_ATTR(cpld_qsfpdd_evt_port_2),

    _DEVICE_ATTR(cpld_qsfpdd_evt_present_0),
    _DEVICE_ATTR(cpld_qsfpdd_evt_present_1),
    _DEVICE_ATTR(cpld_qsfpdd_evt_present_2),

    _DEVICE_ATTR(cpld_qsfpdd_evt_fuse_0),
    _DEVICE_ATTR(cpld_qsfpdd_evt_fuse_1),
    _DEVICE_ATTR(cpld_qsfpdd_evt_fuse_2),

    _DEVICE_ATTR(cpld_evt_ctrl),

    _DEVICE_ATTR(cpld_qsfpdd_reset_0),
    _DEVICE_ATTR(cpld_qsfpdd_reset_1),
    _DEVICE_ATTR(cpld_qsfpdd_reset_2),

    _DEVICE_ATTR(cpld_qsfpdd_lpmode_0),
    _DEVICE_ATTR(cpld_qsfpdd_lpmode_1),
    _DEVICE_ATTR(cpld_qsfpdd_lpmode_2),

    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_port_0),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_port_1),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_port_2),

    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_present_0),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_present_1),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_present_2),

    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_fuse_0),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_fuse_1),
    _DEVICE_ATTR(dbg_cpld_qsfpdd_intr_fuse_2),

    //CPLD 4/5

    _DEVICE_ATTR(cpld_qsfpdd_fab_led_0),
    _DEVICE_ATTR(cpld_qsfpdd_fab_led_1),
    _DEVICE_ATTR(cpld_qsfpdd_fab_led_2),
    _DEVICE_ATTR(cpld_qsfpdd_fab_led_3),
    _DEVICE_ATTR(cpld_qsfpdd_fab_led_4),
    _DEVICE_ATTR(cpld_qsfpdd_fab_led_5),
    _DEVICE_ATTR(cpld_qsfpdd_fab_led_6),
    _DEVICE_ATTR(cpld_qsfpdd_fab_led_7),
    _DEVICE_ATTR(cpld_qsfpdd_fab_led_8),
    _DEVICE_ATTR(cpld_qsfpdd_fab_led_9),
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
            str_len = sizeof(bsp_debug);
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
        case CPLD_PHY_INTR:
            reg = CPLD_PHY_INTR_REG;
            break;
        case CPLD_CPLDX_INTR:
            reg = CPLD_CPLDX_INTR_REG;
            break;
        case CPLD_MAC_THERMAL_INTR:
            reg = CPLD_THERMAL_INTR_BASE_REG;
            break;
        case CPLD_MISC_INTR:
            reg = CPLD_MISC_INTR_REG;
            break;
        case CPLD_CPU_INTR:
            reg = CPLD_CPU_INTR_REG;
            break;
        case CPLD_MAC_MASK:
            reg = CPLD_MAC_MASK_REG;
            break;
        case CPLD_PHY_MASK:
            reg = CPLD_PHY_MASK_REG;
            break;
        case CPLD_CPLDX_MASK:
            reg = CPLD_CPLDX_MASK_REG;
            break;
        case CPLD_MAC_THERMAL_MASK:
            reg = CPLD_THERMAL_MASK_BASE_REG;
            break;
        case CPLD_MISC_MASK:
            reg = CPLD_MISC_MASK_REG;
            break;
        case CPLD_CPU_MASK:
            reg = CPLD_CPU_MASK_REG;
            break;
        case CPLD_MAC_EVT:
            reg = CPLD_MAC_EVT_REG;
            break;
        case CPLD_PHY_EVT:
            reg = CPLD_PHY_EVT_REG;
            break;
        case CPLD_CPLDX_EVT:
            reg = CPLD_CPLDX_EVT_REG;
            break;
        case CPLD_MAC_THERMAL_EVT:
            reg = CPLD_THERMAL_EVT_BASE_REG;
            break;
        case CPLD_MISC_EVT:
            reg = CPLD_MISC_EVT_REG;
            break;
        case CPLD_EVT_CTRL:
            reg = CPLD_EVT_CTRL_REG;
            break;
        case CPLD_MAC_RESET:
            reg = CPLD_MAC_RESET_REG;
            break;
        case CPLD_PHY_RESET:
            reg = CPLD_PHY_RESET_REG;
            break;
        case CPLD_BMC_RESET:
            reg = CPLD_BMC_RESET_REG;
            break;
        case CPLD_USB_RESET:
            reg = CPLD_USB_RESET_REG;
            break;
        case CPLD_CPLDX_RESET:
            reg = CPLD_CPLDX_RESET_REG;
            break;
        case CPLD_I2C_MUX_NIF_RESET:
            reg = CPLD_I2C_MUX_NIF_RESET_REG;
            break;
        case CPLD_MISC_RESET:
            reg = CPLD_MISC_RESET_REG;
            break;
        case CPLD_BRD_PRESENT:
            reg = CPLD_BRD_PRESENT_REG;
            break;
        case CPLD_PSU_STATUS:
            reg = CPLD_PSU_STATUS_REG;
            break;
        case CPLD_SYSTEM_PWR:
            reg = CPLD_SYSTEM_PWR_REG;
            break;
        case CPLD_MAC_SYNCE:
            reg = CPLD_MAC_SYNCE_REG;
            break;
        case CPLD_MAC_ROV:
            reg = CPLD_MAC_ROV_REG;
            break;
        case CPLD_MUX_CTRL:
            reg = CPLD_MUX_CTRL_REG;
            break;
        case CPLD_SYSTEM_LED_0 ... CPLD_SYSTEM_LED_2:
            reg = CPLD_SYSTEM_LED_BASE_REG +
                 (attr->index - CPLD_SYSTEM_LED_0);
            break;
        case CPLD_LED_CLEAR:
            reg = CPLD_LED_CLEAR_REG;
            break;
        case CPLD_QSFPDD_INTR_PORT_0 ... CPLD_QSFPDD_INTR_PORT_2:
            reg = CPLD_QSFPDD_INTR_PORT_BASE_REG +
                 (attr->index - CPLD_QSFPDD_INTR_PORT_0);
            break;
        case CPLD_QSFPDD_INTR_PRESENT_0 ... CPLD_QSFPDD_INTR_PRESENT_2:
            reg = CPLD_QSFPDD_INTR_PRESENT_BASE_REG +
                 (attr->index - CPLD_QSFPDD_INTR_PRESENT_0);
            break;
        case CPLD_QSFPDD_INTR_FUSE_0 ... CPLD_QSFPDD_INTR_FUSE_2:
            reg = CPLD_QSFPDD_INTR_FUSE_BASE_REG +
                 (attr->index - CPLD_QSFPDD_INTR_FUSE_0);
            break;
        case CPLD_QSFPDD_MASK_PORT_0 ... CPLD_QSFPDD_MASK_PORT_2:
            reg = CPLD_QSFPDD_MASK_PORT_BASE_REG +
                 (attr->index - CPLD_QSFPDD_MASK_PORT_0);
            break;
        case CPLD_QSFPDD_MASK_PRESENT_0 ... CPLD_QSFPDD_MASK_PRESENT_2:
            reg = CPLD_QSFPDD_MASK_PRESENT_BASE_REG +
                 (attr->index - CPLD_QSFPDD_MASK_PRESENT_0);
            break;
        case CPLD_QSFPDD_MASK_FUSE_0 ... CPLD_QSFPDD_MASK_FUSE_2:
            reg = CPLD_QSFPDD_MASK_FUSE_BASE_REG +
                 (attr->index - CPLD_QSFPDD_MASK_FUSE_0);
            break;
        case CPLD_QSFPDD_EVT_PORT_0 ... CPLD_QSFPDD_EVT_PORT_2:
            reg = CPLD_QSFPDD_EVT_PORT_BASE_REG +
                 (attr->index - CPLD_QSFPDD_EVT_PORT_0);
            break;
        case CPLD_QSFPDD_EVT_PRESENT_0 ... CPLD_QSFPDD_EVT_PRESENT_2:
            reg = CPLD_QSFPDD_EVT_PRESENT_BASE_REG +
                 (attr->index - CPLD_QSFPDD_EVT_PRESENT_0);
            break;
        case CPLD_QSFPDD_EVT_FUSE_0 ... CPLD_QSFPDD_EVT_FUSE_2:
            reg = CPLD_QSFPDD_EVT_FUSE_BASE_REG +
                 (attr->index - CPLD_QSFPDD_EVT_FUSE_0);
            break;
        case CPLD_QSFPDD_RESET_0 ... CPLD_QSFPDD_RESET_2:
            reg = CPLD_QSFPDD_RESET_BASE_REG +
                 (attr->index - CPLD_QSFPDD_RESET_0);
            break;
        case CPLD_QSFPDD_LPMODE_0 ... CPLD_QSFPDD_LPMODE_2:
            reg = CPLD_QSFPDD_LPMODE_BASE_REG +
                 (attr->index - CPLD_QSFPDD_LPMODE_0);
            break;
        case DBG_CPLD_MAC_INTR:
            reg = DBG_CPLD_MAC_INTR_REG;
            break;
        case DBG_CPLD_PHY_INTR:
            reg = DBG_CPLD_PHY_INTR_REG;
            break;
        case DBG_CPLD_CPLDX_INTR:
            reg = DBG_CPLD_CPLDX_INTR_REG;
            break;
        case DBG_CPLD_MAC_THERMAL_INTR:
            reg = DBG_CPLD_THERMAL_INTR_BASE_REG;
            break;
        case DBG_CPLD_MISC_INTR:
            reg = DBG_CPLD_MISC_INTR_REG;
            break;
        case DBG_CPLD_QSFPDD_INTR_PORT_0 ... DBG_CPLD_QSFPDD_INTR_PORT_2:
            reg = DBG_CPLD_QSFPDD_INTR_PORT_BASE_REG +
                 (attr->index - DBG_CPLD_QSFPDD_INTR_PORT_0);
            break;
        case DBG_CPLD_QSFPDD_INTR_PRESENT_0 ... DBG_CPLD_QSFPDD_INTR_PRESENT_2:
            reg = DBG_CPLD_QSFPDD_INTR_PRESENT_BASE_REG +
                 (attr->index - DBG_CPLD_QSFPDD_INTR_PRESENT_0);
            break;
        case DBG_CPLD_QSFPDD_INTR_FUSE_0 ... DBG_CPLD_QSFPDD_INTR_FUSE_2:
            reg = DBG_CPLD_QSFPDD_INTR_FUSE_BASE_REG +
                 (attr->index - DBG_CPLD_QSFPDD_INTR_FUSE_0);
            break;
        //CPLD 2
        case CPLD_OP2_INTR:
            reg = CPLD_OP2_INTR_REG;
            break;
        case CPLD_OP2_MASK:
            reg = CPLD_OP2_MASK_REG;
            break;
        case CPLD_OP2_EVT:
            reg = CPLD_OP2_EVT_REG;
            break;
        case CPLD_OP2_RESET:
            reg = CPLD_OP2_RESET_REG;
            break;
        case CPLD_OP2_PWR:
            reg = CPLD_OP2_PWR_REG;
            break;
        case CPLD_MISC_PWR:
            reg = CPLD_MISC_PWR_REG;
            break;
        case DBG_CPLD_OP2_INTR:
            reg = DBG_CPLD_OP2_INTR_REG;
            break;
        //CPLD 2/3
        case CPLD_SFP_STATUS:
            reg = CPLD_SFP_STATUS_REG;
            break;
        case CPLD_SFP_MASK:
            reg = CPLD_SFP_MASK_REG;
            break;
        case CPLD_SFP_EVT:
            reg = CPLD_SFP_EVT_REG;
            break;
        case CPLD_SFP_CONFIG:
            reg = CPLD_SFP_CONFIG_REG;
            break;
        case DBG_CPLD_SFP_STATUS:
            reg = DBG_CPLD_SFP_STATUS_REG;
            break;
        //CPLD 4
        case CPLD_I2C_MUX_FAB_RESET:
            reg = CPLD_I2C_MUX_FAB_RESET_REG;
            break;
        //CPLD 4/5
        case CPLD_QSFPDD_FAB_LED_0 ... CPLD_QSFPDD_FAB_LED_9:
            reg = CPLD_QSFPDD_FAB_LED_BASE_REG +
                 (attr->index - CPLD_QSFPDD_FAB_LED_0);
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
        case CPLD_PHY_MASK:
            reg = CPLD_PHY_MASK_REG;
            break;
        case CPLD_CPLDX_MASK:
            reg = CPLD_CPLDX_MASK_REG;
            break;
        case CPLD_MAC_THERMAL_MASK:
            reg = CPLD_THERMAL_MASK_BASE_REG;
            break;
        case CPLD_MISC_MASK:
            reg = CPLD_MISC_MASK_REG;
            break;
        case CPLD_CPU_MASK:
            reg = CPLD_CPU_MASK_REG;
            break;
        case CPLD_EVT_CTRL:
            reg = CPLD_EVT_CTRL_REG;
            break;
        case CPLD_MAC_RESET:
            reg = CPLD_MAC_RESET_REG;
            break;
        case CPLD_PHY_RESET:
            reg = CPLD_PHY_RESET_REG;
            break;
        case CPLD_BMC_RESET:
            reg = CPLD_BMC_RESET_REG;
            break;
        case CPLD_USB_RESET:
            reg = CPLD_USB_RESET_REG;
            break;
        case CPLD_CPLDX_RESET:
            reg = CPLD_CPLDX_RESET_REG;
            break;
        case CPLD_I2C_MUX_NIF_RESET:
            reg = CPLD_I2C_MUX_NIF_RESET_REG;
            break;
        case CPLD_MISC_RESET:
            reg = CPLD_MISC_RESET_REG;
            break;
        case CPLD_MUX_CTRL:
            reg = CPLD_MUX_CTRL_REG;
            break;
        case CPLD_SYSTEM_LED_0 ... CPLD_SYSTEM_LED_2:
            reg = CPLD_SYSTEM_LED_BASE_REG +
                 (attr->index - CPLD_SYSTEM_LED_0);
            break;
        case CPLD_LED_CLEAR:
            reg = CPLD_LED_CLEAR_REG;
            break;
        case CPLD_QSFPDD_MASK_PORT_0 ... CPLD_QSFPDD_MASK_PORT_2:
            reg = CPLD_QSFPDD_MASK_PORT_BASE_REG +
                 (attr->index - CPLD_QSFPDD_MASK_PORT_0);
            break;
        case CPLD_QSFPDD_MASK_PRESENT_0 ... CPLD_QSFPDD_MASK_PRESENT_2:
            reg = CPLD_QSFPDD_MASK_PRESENT_BASE_REG +
                 (attr->index - CPLD_QSFPDD_MASK_PRESENT_0);
            break;
        case CPLD_QSFPDD_MASK_FUSE_0 ... CPLD_QSFPDD_MASK_FUSE_2:
            reg = CPLD_QSFPDD_MASK_FUSE_BASE_REG +
                 (attr->index - CPLD_QSFPDD_MASK_FUSE_0);
            break;
        case CPLD_QSFPDD_RESET_0 ... CPLD_QSFPDD_RESET_2:
            reg = CPLD_QSFPDD_RESET_BASE_REG +
                 (attr->index - CPLD_QSFPDD_RESET_0);
            break;
        case CPLD_QSFPDD_LPMODE_0 ... CPLD_QSFPDD_LPMODE_2:
            reg = CPLD_QSFPDD_LPMODE_BASE_REG +
                 (attr->index - CPLD_QSFPDD_LPMODE_0);
            break;
        case DBG_CPLD_MAC_INTR:
            reg = DBG_CPLD_MAC_INTR_REG;
            break;
        case DBG_CPLD_PHY_INTR:
            reg = DBG_CPLD_PHY_INTR_REG;
            break;
        case DBG_CPLD_CPLDX_INTR:
            reg = DBG_CPLD_CPLDX_INTR_REG;
            break;
        case DBG_CPLD_MAC_THERMAL_INTR:
            reg = DBG_CPLD_THERMAL_INTR_BASE_REG;
            break;
        case DBG_CPLD_MISC_INTR:
            reg = DBG_CPLD_MISC_INTR_REG;
            break;
        case DBG_CPLD_QSFPDD_INTR_PORT_0 ... DBG_CPLD_QSFPDD_INTR_PORT_2:
            reg = DBG_CPLD_QSFPDD_INTR_PORT_BASE_REG +
                 (attr->index - DBG_CPLD_QSFPDD_INTR_PORT_0);
            break;
        case DBG_CPLD_QSFPDD_INTR_PRESENT_0 ... DBG_CPLD_QSFPDD_INTR_PRESENT_2:
            reg = DBG_CPLD_QSFPDD_INTR_PRESENT_BASE_REG +
                 (attr->index - DBG_CPLD_QSFPDD_INTR_PRESENT_0);
            break;
        case DBG_CPLD_QSFPDD_INTR_FUSE_0 ... DBG_CPLD_QSFPDD_INTR_FUSE_2:
            reg = DBG_CPLD_QSFPDD_INTR_FUSE_BASE_REG +
                 (attr->index - DBG_CPLD_QSFPDD_INTR_FUSE_0);
            break;
        //CPLD 2
        case CPLD_OP2_MASK:
            reg = CPLD_OP2_MASK_REG;
            break;
        case CPLD_OP2_RESET:
            reg = CPLD_OP2_RESET_REG;
            break;
        case DBG_CPLD_OP2_INTR:
            reg = DBG_CPLD_OP2_INTR_REG;
            break;
        //CPLD 2/3
        case CPLD_SFP_MASK:
            reg = CPLD_SFP_MASK_REG;
            break;
        case CPLD_SFP_CONFIG:
            reg = CPLD_SFP_CONFIG_REG;
            break;
        case DBG_CPLD_SFP_STATUS:
            reg = DBG_CPLD_SFP_STATUS_REG;
            break;
        //CPLD 4
        case CPLD_I2C_MUX_FAB_RESET:
            reg = CPLD_I2C_MUX_FAB_RESET_REG;
            break;
        //CPLD 4/5
        case CPLD_QSFPDD_FAB_LED_0 ... CPLD_QSFPDD_FAB_LED_9:
            reg = CPLD_QSFPDD_FAB_LED_BASE_REG +
                 (attr->index - CPLD_QSFPDD_FAB_LED_0);
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

    if (INVALID(ret, cpld1, cpld5)) {
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
    case cpld5:
        sysfs_remove_group(&client->dev.kobj, &cpld5_group);
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
        .name = "x86_64_ufispace_s9710_76d_cpld",
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
MODULE_DESCRIPTION("x86_64_ufispace_s9710_76d_cpld driver");
MODULE_LICENSE("GPL");

module_init(cpld_init);
module_exit(cpld_exit);
