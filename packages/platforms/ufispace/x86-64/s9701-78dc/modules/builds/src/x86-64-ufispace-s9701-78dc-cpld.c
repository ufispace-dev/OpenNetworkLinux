/*
 * A i2c cpld driver for the ufispace_s9701_78dc
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
#include <linux/types.h>
#include <linux/version.h>
#include "x86-64-ufispace-s9701-78dc-cpld.h"

#ifdef DEBUG
#define DEBUG_PRINT(fmt, args...) \
    printk(KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
            __FILE__, __func__, __LINE__, ##args)
#else
#define DEBUG_PRINT(fmt, args...)
#endif

#define I2C_READ_BYTE_DATA(ret, lock, i2c_client, reg) \
{ \
    mutex_lock(lock); \
    ret = i2c_smbus_read_byte_data(i2c_client, reg); \
    mutex_unlock(lock); \
}
#define I2C_WRITE_BYTE_DATA(ret, lock, i2c_client, reg, val) \
{ \
        mutex_lock(lock); \
        ret = i2c_smbus_write_byte_data(i2c_client, reg, val); \
        mutex_unlock(lock); \
}

/* CPLD sysfs attributes index  */
enum s9701_cpld_sysfs_attributes {
    CPLD_ACCESS_REG,
    CPLD_REGISTER_VAL,
    CPLD_SKU_ID,
    CPLD_HW_REV,
    CPLD_DEPH_REV,
    CPLD_BUILD_REV,
    CPLD_ID_TYPE,
    CPLD_MAJOR_VER,
    CPLD_MINOR_VER,
    CPLD_ID,
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
    CPLD_BMC_NTM_RST,
    CPLD_USB_RST,
    CPLD_CPLD_RST,
    CPLD_MUX_RST_1,
    CPLD_MUX_RST_2,
    CPLD_MISC_RST,
    CPLD_PUSHBTN,
    CPLD_DAU_BD_PRES,
    CPLD_PSU_STATUS,
    CPLD_SYS_PW_STATUS,
    CPLD_MAC_STATUS_1,
    CPLD_MAC_STATUS_2,
    CPLD_MAC_STATUS_3,
    CPLD_MGMT_SFP_PORT_CONF,
    CPLD_MISC,
    CPLD_MUX_CTRL,
    CPLD_SYS_LED_CTRL_1,
    CPLD_SYS_LED_CTRL_2,
    CPLD_OOB_LED_CTRL,
    CPLD_MAC_PG,
    CPLD_OP2_PG,
    CPLD_MISC_PG,
    CPLD_HBM_PW_EN,
    CPLD_SFP_PORT_0_7_PRES,
    CPLD_SFP_PORT_8_15_PRES,
    CPLD_SFP_PORT_32_39_PRES,
    CPLD_SFP_PORT_40_47_PRES,
    CPLD_SFP_PORT_0_7_PRES_MASK,
    CPLD_SFP_PORT_8_15_PRES_MASK,
    CPLD_SFP_PORT_32_39_PRES_MASK,
    CPLD_SFP_PORT_40_47_PRES_MASK,
    CPLD_SFP_PORT_0_7_PLUG_EVENT,
    CPLD_SFP_PORT_8_15_PLUG_EVENT,
    CPLD_SFP_PORT_32_39_PLUG_EVENT,
    CPLD_SFP_PORT_40_47_PLUG_EVENT,
    CPLD_SFP_PORT_0_7_RX_LOS,
    CPLD_SFP_PORT_8_15_RX_LOS,
    CPLD_SFP_PORT_32_39_RX_LOS,
    CPLD_SFP_PORT_40_47_RX_LOS,
    CPLD_SFP_PORT_0_7_TX_FAULT,
    CPLD_SFP_PORT_8_15_TX_FAULT,
    CPLD_SFP_PORT_32_39_TX_FAULT,
    CPLD_SFP_PORT_40_47_TX_FAULT,
    CPLD_SFP_PORT_0_7_TX_DISABLE,
    CPLD_SFP_PORT_8_15_TX_DISABLE,
    CPLD_SFP_PORT_32_39_TX_DISABLE,
    CPLD_SFP_PORT_40_47_TX_DISABLE,
    CPLD_SFP_PORT_16_23_PRES,
    CPLD_SFP_PORT_24_31_PRES,
    CPLD_SFP_PORT_48_55_PRES,
    CPLD_SFP_PORT_56_63_PRES,
    CPLD_SFP_PORT_16_23_PRES_MASK,
    CPLD_SFP_PORT_24_31_PRES_MASK,
    CPLD_SFP_PORT_48_55_PRES_MASK,
    CPLD_SFP_PORT_56_63_PRES_MASK,
    CPLD_SFP_PORT_16_23_PLUG_EVENT,
    CPLD_SFP_PORT_24_31_PLUG_EVENT,
    CPLD_SFP_PORT_48_55_PLUG_EVENT,
    CPLD_SFP_PORT_56_63_PLUG_EVENT,
    CPLD_SFP_PORT_16_23_RX_LOS,
    CPLD_SFP_PORT_24_31_RX_LOS,
    CPLD_SFP_PORT_48_55_RX_LOS,
    CPLD_SFP_PORT_56_63_RX_LOS,
    CPLD_SFP_PORT_16_23_TX_FAULT,
    CPLD_SFP_PORT_24_31_TX_FAULT,
    CPLD_SFP_PORT_48_55_TX_FAULT,
    CPLD_SFP_PORT_56_63_TX_FAULT,
    CPLD_SFP_PORT_16_23_TX_DISABLE,
    CPLD_SFP_PORT_24_31_TX_DISABLE,
    CPLD_SFP_PORT_48_55_TX_DISABLE,
    CPLD_SFP_PORT_56_63_TX_DISABLE,
    CPLD_QSFP_PORT_64_71_INTR,
    CPLD_QSFPDD_PORT_INTR,
    CPLD_QSFP_PORT_64_71_PRES,
    CPLD_QSFPDD_PORT_0_5_PRES,
    CPLD_QSFP_PORT_64_71_INTR_MASK,
    CPLD_QSFPDD_PORT_INTR_MASK,
    CPLD_QSFP_PORT_64_71_PRES_MASK,
    CPLD_QSFPDD_PORT_0_5_PRES_MASK,
    CPLD_QSFP_PORT_64_71_INTR_EVENT,
    CPLD_QSFPDD_PORT_INTR_EVENT,
    CPLD_QSFP_PORT_64_71_PLUG_EVENT,
    CPLD_QSFPDD_PORT_0_5_PLUG_EVENT,
    CPLD_QSFP_PORT_64_71_RST,
    CPLD_QSFPDD_PORT_RST,
    CPLD_QSFP_PORT_64_71_LPMODE,
    CPLD_QSFPDD_PORT_LPMODE,
    CPLD_BEACON_LED,
    CPLD_QSFPDD_PORT_0_1_LED,
    CPLD_QSFPDD_PORT_2_3_LED,
    CPLD_QSFPDD_PORT_4_5_LED,
};

/* CPLD sysfs attributes hook functions  */
static ssize_t read_access_register(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_access_register(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_register_value(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_register_value(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_cpld_cb(struct device *dev,
        struct device_attribute *da, char *buf);
static ssize_t read_hw_rev_cb(struct device *dev,
        struct device_attribute *da, char *buf);
static ssize_t read_cpld_version_cb(struct device *dev,
        struct device_attribute *da, char *buf);
static ssize_t write_cpld_cb(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
// cpld access api
static ssize_t read_cpld_reg(struct device *dev, char *buf, u8 reg);
static ssize_t write_cpld_reg(struct device *dev, const char *buf, size_t count, u8 reg);
static bool read_cpld_reg_raw_byte(struct device *dev, u8 reg, u8 *val);
static bool read_cpld_reg_raw_int(struct device *dev, u8 reg, int *val);

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
static const struct i2c_device_id s9701_cpld_id[] = {
    { "s9701_78dc_cpld1",  cpld1 },
    { "s9701_78dc_cpld2",  cpld2 },
    { "s9701_78dc_cpld3",  cpld3 },
    { "s9701_78dc_cpld4",  cpld4 },    
    {}
};

/* Addresses scanned for s9701_cpld */
static const unsigned short cpld_i2c_addr[] = { 0x30, 0x31, 0x32, 0x33, I2C_CLIENT_END };

/* define all support register access of cpld in attribute */
static SENSOR_DEVICE_ATTR(cpld_access_register, S_IWUSR | S_IRUGO, \
        read_access_register, write_access_register, CPLD_ACCESS_REG);
static SENSOR_DEVICE_ATTR(cpld_register_value, S_IWUSR | S_IRUGO, \
        read_register_value, write_register_value, CPLD_REGISTER_VAL);
static SENSOR_DEVICE_ATTR(cpld_sku_id, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SKU_ID);
static SENSOR_DEVICE_ATTR(cpld_hw_rev, S_IRUGO, \
        read_hw_rev_cb, NULL, CPLD_HW_REV);
static SENSOR_DEVICE_ATTR(cpld_deph_rev, S_IRUGO, \
        read_hw_rev_cb, NULL, CPLD_DEPH_REV);
static SENSOR_DEVICE_ATTR(cpld_build_rev, S_IRUGO, \
        read_hw_rev_cb, NULL, CPLD_BUILD_REV);
static SENSOR_DEVICE_ATTR(cpld_id_type, S_IRUGO, \
        read_hw_rev_cb, NULL, CPLD_ID_TYPE);
static SENSOR_DEVICE_ATTR(cpld_major_ver, S_IRUGO, \
        read_cpld_version_cb, NULL, CPLD_MAJOR_VER);
static SENSOR_DEVICE_ATTR(cpld_minor_ver, S_IRUGO, \
        read_cpld_version_cb, NULL, CPLD_MINOR_VER);
static SENSOR_DEVICE_ATTR(cpld_id, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_ID);
static SENSOR_DEVICE_ATTR(cpld_mac_op2_intr, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_MAC_OP2_INTR);
static SENSOR_DEVICE_ATTR(cpld_10gphy_intr, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_10GPHY_INTR);
static SENSOR_DEVICE_ATTR(cpld_cpld_fru_intr, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_CPLD_FRU_INTR);
static SENSOR_DEVICE_ATTR(cpld_mgmt_sfp_port_status, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_MGMT_SFP_PORT_STATUS);
static SENSOR_DEVICE_ATTR(cpld_misc_intr, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_MISC_INTR);
static SENSOR_DEVICE_ATTR(cpld_system_intr, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SYSTEM_INTR);
static SENSOR_DEVICE_ATTR(cpld_mac_op2_intr_mask, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_MAC_OP2_INTR_MASK);
static SENSOR_DEVICE_ATTR(cpld_10gphy_intr_mask, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_10GPHY_INTR_MASK);
static SENSOR_DEVICE_ATTR(cpld_cpld_fru_intr_mask, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_CPLD_FRU_INTR_MASK);
static SENSOR_DEVICE_ATTR(cpld_mgmt_sfp_port_status_mask, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_MGMT_SFP_PORT_STATUS_MASK);
static SENSOR_DEVICE_ATTR(cpld_misc_intr_mask, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_MISC_INTR_MASK);
static SENSOR_DEVICE_ATTR(cpld_system_intr_mask, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_SYSTEM_INTR_MASK);
static SENSOR_DEVICE_ATTR(cpld_mac_op2_intr_event, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_MAC_OP2_INTR_EVENT);
static SENSOR_DEVICE_ATTR(cpld_10gphy_intr_event, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_10GPHY_INTR_EVENT);
static SENSOR_DEVICE_ATTR(cpld_cpld_fru_intr_event, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_CPLD_FRU_INTR_EVENT);
static SENSOR_DEVICE_ATTR(cpld_mgmt_sfp_port_status_event, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_MGMT_SFP_PORT_STATUS_EVENT);
static SENSOR_DEVICE_ATTR(cpld_misc_intr_event, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_MISC_INTR_EVENT);
static SENSOR_DEVICE_ATTR(cpld_system_intr_event, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SYSTEM_INTR_EVENT);
static SENSOR_DEVICE_ATTR(cpld_mac_op2_rst, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_MAC_OP2_RST);
static SENSOR_DEVICE_ATTR(cpld_bmc_ntm_rst, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_BMC_NTM_RST);
static SENSOR_DEVICE_ATTR(cpld_usb_rst, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_USB_RST);
static SENSOR_DEVICE_ATTR(cpld_cpld_rst, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_CPLD_RST);
static SENSOR_DEVICE_ATTR(cpld_mux_rst_1, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_MUX_RST_1);
static SENSOR_DEVICE_ATTR(cpld_mux_rst_2, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_MUX_RST_2);
static SENSOR_DEVICE_ATTR(cpld_misc_rst, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_MISC_RST);
static SENSOR_DEVICE_ATTR(cpld_pushbtn, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_PUSHBTN);
static SENSOR_DEVICE_ATTR(cpld_dau_bd_pres, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_DAU_BD_PRES);
static SENSOR_DEVICE_ATTR(cpld_psu_status, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_PSU_STATUS);
static SENSOR_DEVICE_ATTR(cpld_sys_pw_status, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SYS_PW_STATUS);
static SENSOR_DEVICE_ATTR(cpld_mac_status_1, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_MAC_STATUS_1);
static SENSOR_DEVICE_ATTR(cpld_mac_status_2, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_MAC_STATUS_2);
static SENSOR_DEVICE_ATTR(cpld_mac_status_3, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_MAC_STATUS_3);
static SENSOR_DEVICE_ATTR(cpld_mgmt_sfp_port_conf, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_MGMT_SFP_PORT_CONF);
static SENSOR_DEVICE_ATTR(cpld_misc, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_MISC);
static SENSOR_DEVICE_ATTR(cpld_mux_ctrl, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_MUX_CTRL);
static SENSOR_DEVICE_ATTR(cpld_sys_led_ctrl_1, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_SYS_LED_CTRL_1);
static SENSOR_DEVICE_ATTR(cpld_sys_led_ctrl_2, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_SYS_LED_CTRL_2);
static SENSOR_DEVICE_ATTR(cpld_oob_led_ctrl, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_OOB_LED_CTRL);
static SENSOR_DEVICE_ATTR(cpld_mac_pg, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_MAC_PG);
static SENSOR_DEVICE_ATTR(cpld_op2_pg, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_OP2_PG);
static SENSOR_DEVICE_ATTR(cpld_misc_pg, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_MISC_PG);
static SENSOR_DEVICE_ATTR(cpld_hbm_pw_en, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_HBM_PW_EN);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_0_7_pres, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_0_7_PRES);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_8_15_pres, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_8_15_PRES);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_32_39_pres, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_32_39_PRES);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_40_47_pres, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_40_47_PRES);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_0_7_pres_mask, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_SFP_PORT_0_7_PRES_MASK);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_8_15_pres_mask, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_SFP_PORT_8_15_PRES_MASK);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_32_39_pres_mask, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_SFP_PORT_32_39_PRES_MASK);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_40_47_pres_mask, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_SFP_PORT_40_47_PRES_MASK);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_0_7_plug_event, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_0_7_PLUG_EVENT);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_8_15_plug_event, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_8_15_PLUG_EVENT);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_32_39_plug_event, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_32_39_PLUG_EVENT);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_40_47_plug_event, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_40_47_PLUG_EVENT);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_0_7_rx_los, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_0_7_RX_LOS);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_8_15_rx_los, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_8_15_RX_LOS);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_32_39_rx_los, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_32_39_RX_LOS);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_40_47_rx_los, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_40_47_RX_LOS);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_0_7_tx_fault, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_0_7_TX_FAULT);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_8_15_tx_fault, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_8_15_TX_FAULT);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_32_39_tx_fault, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_32_39_TX_FAULT);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_40_47_tx_fault, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_40_47_TX_FAULT);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_0_7_tx_disable, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_SFP_PORT_0_7_TX_DISABLE);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_8_15_tx_disable, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_SFP_PORT_8_15_TX_DISABLE);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_32_39_tx_disable, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_SFP_PORT_32_39_TX_DISABLE);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_40_47_tx_disable, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_SFP_PORT_40_47_TX_DISABLE);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_16_23_pres, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_16_23_PRES);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_24_31_pres, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_24_31_PRES);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_48_55_pres, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_48_55_PRES);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_56_63_pres, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_56_63_PRES);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_16_23_pres_mask, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_SFP_PORT_16_23_PRES_MASK);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_24_31_pres_mask, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_SFP_PORT_24_31_PRES_MASK);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_48_55_pres_mask, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_SFP_PORT_48_55_PRES_MASK);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_56_63_pres_mask, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_SFP_PORT_56_63_PRES_MASK);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_16_23_plug_event, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_16_23_PLUG_EVENT);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_24_31_plug_event, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_24_31_PLUG_EVENT);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_48_55_plug_event, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_48_55_PLUG_EVENT);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_56_63_plug_event, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_56_63_PLUG_EVENT);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_16_23_rx_los, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_16_23_RX_LOS);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_24_31_rx_los, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_24_31_RX_LOS);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_48_55_rx_los, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_48_55_RX_LOS);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_56_63_rx_los, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_56_63_RX_LOS);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_16_23_tx_fault, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_16_23_TX_FAULT);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_24_31_tx_fault, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_24_31_TX_FAULT);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_48_55_tx_fault, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_48_55_TX_FAULT);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_56_63_tx_fault, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_SFP_PORT_56_63_TX_FAULT);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_16_23_tx_disable, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_SFP_PORT_16_23_TX_DISABLE);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_24_31_tx_disable, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_SFP_PORT_24_31_TX_DISABLE);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_48_55_tx_disable, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_SFP_PORT_48_55_TX_DISABLE);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_56_63_tx_disable, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_SFP_PORT_56_63_TX_DISABLE);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_64_71_intr, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_QSFP_PORT_64_71_INTR);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_port_intr, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_QSFPDD_PORT_INTR);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_64_71_pres, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_QSFP_PORT_64_71_PRES);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_port_0_5_pres, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_QSFPDD_PORT_0_5_PRES);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_64_71_intr_mask, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_QSFP_PORT_64_71_INTR_MASK);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_port_intr_mask, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_QSFPDD_PORT_INTR_MASK);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_64_71_pres_mask, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_QSFP_PORT_64_71_PRES_MASK);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_port_0_5_pres_mask, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_QSFPDD_PORT_0_5_PRES_MASK);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_64_71_intr_event, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_QSFP_PORT_64_71_INTR_EVENT);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_port_intr_event, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_QSFPDD_PORT_INTR_EVENT);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_64_71_plug_event, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_QSFP_PORT_64_71_PLUG_EVENT);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_port_0_5_plug_event, S_IRUGO, \
        read_cpld_cb, NULL, CPLD_QSFPDD_PORT_0_5_PLUG_EVENT);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_64_71_rst, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_QSFP_PORT_64_71_RST);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_port_rst, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_QSFPDD_PORT_RST);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_64_71_lpmode, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_QSFP_PORT_64_71_LPMODE);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_port_lpmode, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_QSFPDD_PORT_LPMODE);
static SENSOR_DEVICE_ATTR(cpld_beacon_led, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_BEACON_LED);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_port_0_1_led, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_QSFPDD_PORT_0_1_LED);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_port_2_3_led, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_QSFPDD_PORT_2_3_LED);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_port_4_5_led, S_IWUSR | S_IRUGO, \
        read_cpld_cb, write_cpld_cb, CPLD_QSFPDD_PORT_4_5_LED);

/* define support attributes of cpldx , total 3 */
/* cpld 1 */
static struct attribute *s9701_cpld1_attributes[] = {
    &sensor_dev_attr_cpld_access_register.dev_attr.attr,
    &sensor_dev_attr_cpld_register_value.dev_attr.attr,
    &sensor_dev_attr_cpld_sku_id.dev_attr.attr,
    &sensor_dev_attr_cpld_hw_rev.dev_attr.attr,
    &sensor_dev_attr_cpld_deph_rev.dev_attr.attr,
    &sensor_dev_attr_cpld_build_rev.dev_attr.attr,
    &sensor_dev_attr_cpld_id_type.dev_attr.attr,
    &sensor_dev_attr_cpld_major_ver.dev_attr.attr,
    &sensor_dev_attr_cpld_minor_ver.dev_attr.attr,
    &sensor_dev_attr_cpld_id.dev_attr.attr,
    &sensor_dev_attr_cpld_mac_op2_intr.dev_attr.attr,
    &sensor_dev_attr_cpld_10gphy_intr.dev_attr.attr,
    &sensor_dev_attr_cpld_cpld_fru_intr.dev_attr.attr,
    &sensor_dev_attr_cpld_mgmt_sfp_port_status.dev_attr.attr,
    &sensor_dev_attr_cpld_misc_intr.dev_attr.attr,
    &sensor_dev_attr_cpld_system_intr.dev_attr.attr,
    &sensor_dev_attr_cpld_mac_op2_intr_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_10gphy_intr_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_cpld_fru_intr_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_mgmt_sfp_port_status_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_misc_intr_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_system_intr_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_mac_op2_intr_event.dev_attr.attr,
    &sensor_dev_attr_cpld_10gphy_intr_event.dev_attr.attr,
    &sensor_dev_attr_cpld_cpld_fru_intr_event.dev_attr.attr,
    &sensor_dev_attr_cpld_mgmt_sfp_port_status_event.dev_attr.attr,
    &sensor_dev_attr_cpld_misc_intr_event.dev_attr.attr,
    &sensor_dev_attr_cpld_system_intr_event.dev_attr.attr,
    &sensor_dev_attr_cpld_mac_op2_rst.dev_attr.attr,
    &sensor_dev_attr_cpld_bmc_ntm_rst.dev_attr.attr,
    &sensor_dev_attr_cpld_usb_rst.dev_attr.attr,
    &sensor_dev_attr_cpld_cpld_rst.dev_attr.attr,
    &sensor_dev_attr_cpld_mux_rst_1.dev_attr.attr,
    &sensor_dev_attr_cpld_mux_rst_2.dev_attr.attr,
    &sensor_dev_attr_cpld_misc_rst.dev_attr.attr,
    &sensor_dev_attr_cpld_pushbtn.dev_attr.attr,
    &sensor_dev_attr_cpld_dau_bd_pres.dev_attr.attr,
    &sensor_dev_attr_cpld_psu_status.dev_attr.attr,
    &sensor_dev_attr_cpld_sys_pw_status.dev_attr.attr,
    &sensor_dev_attr_cpld_mac_status_1.dev_attr.attr,
    &sensor_dev_attr_cpld_mac_status_2.dev_attr.attr,
    &sensor_dev_attr_cpld_mac_status_3.dev_attr.attr,
    &sensor_dev_attr_cpld_mgmt_sfp_port_conf.dev_attr.attr,
    &sensor_dev_attr_cpld_misc.dev_attr.attr,
    &sensor_dev_attr_cpld_mux_ctrl.dev_attr.attr,
    &sensor_dev_attr_cpld_sys_led_ctrl_1.dev_attr.attr,
    &sensor_dev_attr_cpld_sys_led_ctrl_2.dev_attr.attr,
    &sensor_dev_attr_cpld_oob_led_ctrl.dev_attr.attr,
    &sensor_dev_attr_cpld_mac_pg.dev_attr.attr,
    &sensor_dev_attr_cpld_op2_pg.dev_attr.attr,
    &sensor_dev_attr_cpld_misc_pg.dev_attr.attr,
    &sensor_dev_attr_cpld_hbm_pw_en.dev_attr.attr,
    NULL
};

/* cpld 2 */
static struct attribute *s9701_cpld2_attributes[] = {
    &sensor_dev_attr_cpld_access_register.dev_attr.attr,
    &sensor_dev_attr_cpld_register_value.dev_attr.attr,
    &sensor_dev_attr_cpld_major_ver.dev_attr.attr,
    &sensor_dev_attr_cpld_minor_ver.dev_attr.attr,
    &sensor_dev_attr_cpld_id.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_0_7_pres.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_8_15_pres.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_32_39_pres.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_40_47_pres.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_0_7_pres_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_8_15_pres_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_32_39_pres_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_40_47_pres_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_0_7_plug_event.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_8_15_plug_event.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_32_39_plug_event.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_40_47_plug_event.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_0_7_rx_los.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_8_15_rx_los.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_32_39_rx_los.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_40_47_rx_los.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_0_7_tx_fault.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_8_15_tx_fault.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_32_39_tx_fault.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_40_47_tx_fault.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_0_7_tx_disable.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_8_15_tx_disable.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_32_39_tx_disable.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_40_47_tx_disable.dev_attr.attr,
    NULL
};

/* cpld 3 */
static struct attribute *s9701_cpld3_attributes[] = {
    &sensor_dev_attr_cpld_access_register.dev_attr.attr,
    &sensor_dev_attr_cpld_register_value.dev_attr.attr,
    &sensor_dev_attr_cpld_major_ver.dev_attr.attr,
    &sensor_dev_attr_cpld_minor_ver.dev_attr.attr,
    &sensor_dev_attr_cpld_id.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_16_23_pres.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_24_31_pres.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_48_55_pres.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_56_63_pres.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_16_23_pres_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_24_31_pres_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_48_55_pres_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_56_63_pres_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_16_23_plug_event.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_24_31_plug_event.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_48_55_plug_event.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_56_63_plug_event.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_16_23_rx_los.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_24_31_rx_los.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_48_55_rx_los.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_56_63_rx_los.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_16_23_tx_fault.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_24_31_tx_fault.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_48_55_tx_fault.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_56_63_tx_fault.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_16_23_tx_disable.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_24_31_tx_disable.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_48_55_tx_disable.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_56_63_tx_disable.dev_attr.attr,
    NULL
};

/* cpld 4 */
static struct attribute *s9701_cpld4_attributes[] = {
    &sensor_dev_attr_cpld_access_register.dev_attr.attr,
    &sensor_dev_attr_cpld_register_value.dev_attr.attr,
    &sensor_dev_attr_cpld_major_ver.dev_attr.attr,
    &sensor_dev_attr_cpld_minor_ver.dev_attr.attr,
    &sensor_dev_attr_cpld_id.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_64_71_intr.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_port_intr.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_64_71_pres.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_port_0_5_pres.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_64_71_intr_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_port_intr_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_64_71_pres_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_port_0_5_pres_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_64_71_intr_event.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_port_intr_event.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_64_71_plug_event.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_port_0_5_plug_event.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_64_71_rst.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_port_rst.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_port_64_71_lpmode.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_port_lpmode.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_port_0_1_led.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_port_2_3_led.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_port_4_5_led.dev_attr.attr,
    &sensor_dev_attr_cpld_beacon_led.dev_attr.attr,
    NULL
};

/* cpld 1 attributes group */
static const struct attribute_group s9701_cpld1_group = {
    .attrs = s9701_cpld1_attributes,
};
/* cpld 2 attributes group */
static const struct attribute_group s9701_cpld2_group = {
    .attrs = s9701_cpld2_attributes,
};
/* cpld 3 attributes group */
static const struct attribute_group s9701_cpld3_group = {
    .attrs = s9701_cpld3_attributes,
};
/* cpld 4 attributes group */
static const struct attribute_group s9701_cpld4_group = {
    .attrs = s9701_cpld4_attributes,
};

/* read access register from cpld data */
static ssize_t read_access_register(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg = data->access_reg;

    return sprintf(buf, "0x%x\n", reg);
}

/* write access register to cpld data */
static ssize_t write_access_register(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;

    if (kstrtou8(buf, 0, &reg) < 0)
        return -EINVAL;

    data->access_reg = reg;
    return count;
}

/* read the value of access register in cpld data */
static ssize_t read_register_value(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg = data->access_reg;
    int reg_val;

    I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);

    if (reg_val < 0)
        return -1;

    return sprintf(buf, "0x%x\n", reg_val);
}

/* wrtie the value to access register in cpld data */
static ssize_t write_register_value(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    int ret = -EIO;
    u8 reg = data->access_reg;
    u8 reg_val;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    I2C_WRITE_BYTE_DATA(ret, &data->access_lock, client, reg, reg_val);

    if (unlikely(ret < 0)) {
        dev_err(dev, "I2C_WRITE_BYTE_DATA error, return=%d\n", ret);
        return ret;
    }

    return count;
}

/* get cpld register value */
static ssize_t read_cpld_reg(struct device *dev,
                    char *buf,
                    u8 reg)
{
    int reg_val;

    if (!read_cpld_reg_raw_int(dev, reg, &reg_val))
        return -1;
    else
        return sprintf(buf, "0x%02x\n", reg_val);
}

static bool read_cpld_reg_raw_int(struct device *dev, u8 reg, int *val)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    int reg_val;
    I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
    if (reg_val < 0)
        return false;
    else {
        *val = reg_val;
        return true;
    }
}

static bool read_cpld_reg_raw_byte(struct device *dev, u8 reg, u8 *val)
{
    int reg_val;

    if (!read_cpld_reg_raw_int(dev, reg, &reg_val))
        return false;
    else {
        *val = (u8)reg_val;
        return true;
    }
}

/* handle read for attributes */
static ssize_t read_cpld_cb(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg = 0;

    switch (attr->index) {
        case CPLD_SKU_ID:
             reg = CPLD_SKU_ID_REG;
             break;
        case CPLD_ID:
             reg = CPLD_ID_REG;
             break;
        case CPLD_MAC_OP2_INTR:
             reg = CPLD_MAC_OP2_INTR_REG;
             break;
        case CPLD_10GPHY_INTR:
             reg = CPLD_10GPHY_INTR_REG;
             break;
        case CPLD_CPLD_FRU_INTR:
             reg = CPLD_CPLD_FRU_INTR_REG;
             break;
        case CPLD_MGMT_SFP_PORT_STATUS:
             reg = CPLD_MGMT_SFP_PORT_STATUS_REG;
             break;
        case CPLD_MISC_INTR:
             reg = CPLD_MISC_INTR_REG;
             break;
        case CPLD_SYSTEM_INTR:
             reg = CPLD_SYSTEM_INTR_REG;
             break;
        case CPLD_MAC_OP2_INTR_MASK:
             reg = CPLD_MAC_OP2_INTR_MASK_REG;
             break;
        case CPLD_10GPHY_INTR_MASK:
             reg = CPLD_10GPHY_INTR_MASK_REG;
             break;
        case CPLD_CPLD_FRU_INTR_MASK:
             reg = CPLD_CPLD_FRU_INTR_MASK_REG;
             break;
        case CPLD_MGMT_SFP_PORT_STATUS_MASK:
             reg = CPLD_MGMT_SFP_PORT_STATUS_MASK_REG;
             break;
        case CPLD_MISC_INTR_MASK:
             reg = CPLD_MISC_INTR_MASK_REG;
             break;
        case CPLD_SYSTEM_INTR_MASK:
             reg = CPLD_SYSTEM_INTR_MASK_REG;
             break;
        case CPLD_MAC_OP2_INTR_EVENT:
             reg = CPLD_MAC_OP2_INTR_EVENT_REG;
             break;
        case CPLD_10GPHY_INTR_EVENT:
             reg = CPLD_10GPHY_INTR_EVENT_REG;
             break;
        case CPLD_CPLD_FRU_INTR_EVENT:
             reg = CPLD_CPLD_FRU_INTR_EVENT_REG;
             break;
        case CPLD_MGMT_SFP_PORT_STATUS_EVENT:
             reg = CPLD_MGMT_SFP_PORT_STATUS_EVENT_REG;
             break;
        case CPLD_MISC_INTR_EVENT:
             reg = CPLD_MISC_INTR_EVENT_REG;
             break;
        case CPLD_SYSTEM_INTR_EVENT:
             reg = CPLD_SYSTEM_INTR_EVENT_REG;
             break;
        case CPLD_MAC_OP2_RST:
             reg = CPLD_MAC_OP2_RST_REG;
             break;
        case CPLD_BMC_NTM_RST:
             reg = CPLD_BMC_NTM_RST_REG;
             break;
        case CPLD_USB_RST:
             reg = CPLD_USB_RST_REG;
             break;
        case CPLD_CPLD_RST:
             reg = CPLD_CPLD_RST_REG;
             break;
        case CPLD_MUX_RST_1:
             reg = CPLD_MUX_RST_1_REG;
             break;
        case CPLD_MUX_RST_2:
             reg = CPLD_MUX_RST_2_REG;
             break;
        case CPLD_MISC_RST:
             reg = CPLD_MISC_RST_REG;
             break;
        case CPLD_PUSHBTN:
             reg = CPLD_PUSHBTN_REG;
             break;
        case CPLD_DAU_BD_PRES:
             reg = CPLD_DAU_BD_PRES_REG;
             break;
        case CPLD_PSU_STATUS:
             reg = CPLD_PSU_STATUS_REG;
             break;
        case CPLD_SYS_PW_STATUS:
             reg = CPLD_SYS_PW_STATUS_REG;
             break;
        case CPLD_MAC_STATUS_1:
             reg = CPLD_MAC_STATUS_1_REG;
             break;
        case CPLD_MAC_STATUS_2:
             reg = CPLD_MAC_STATUS_2_REG;
             break;
        case CPLD_MAC_STATUS_3:
             reg = CPLD_MAC_STATUS_3_REG;
             break;
        case CPLD_MGMT_SFP_PORT_CONF:
             reg = CPLD_MGMT_SFP_PORT_CONF_REG;
             break;
        case CPLD_MISC:
             reg = CPLD_MISC_REG;
             break;
        case CPLD_MUX_CTRL:
             reg = CPLD_MUX_CTRL_REG;
             break;
        case CPLD_SYS_LED_CTRL_1:
             reg = CPLD_SYS_LED_CTRL_1_REG;
             break;
        case CPLD_SYS_LED_CTRL_2:
             reg = CPLD_SYS_LED_CTRL_2_REG;
             break;
        case CPLD_OOB_LED_CTRL:
             reg = CPLD_OOB_LED_CTRL_REG;
             break;
        case CPLD_MAC_PG:
             reg = CPLD_MAC_PG_REG;
             break;
        case CPLD_OP2_PG:
             reg = CPLD_OP2_PG_REG;
             break;
        case CPLD_MISC_PG:
             reg = CPLD_MISC_PG_REG;
             break;
        case CPLD_HBM_PW_EN:
             reg = CPLD_HBM_PW_EN_REG;
             break;
        case CPLD_SFP_PORT_0_7_PRES:
             reg = CPLD_SFP_PORT_0_7_PRES_REG;
             break;
        case CPLD_SFP_PORT_8_15_PRES:
             reg = CPLD_SFP_PORT_8_15_PRES_REG;
             break;
        case CPLD_SFP_PORT_32_39_PRES:
             reg = CPLD_SFP_PORT_32_39_PRES_REG;
             break;
        case CPLD_SFP_PORT_40_47_PRES:
             reg = CPLD_SFP_PORT_40_47_PRES_REG;
             break;
        case CPLD_SFP_PORT_0_7_PRES_MASK:
             reg = CPLD_SFP_PORT_0_7_PRES_MASK_REG;
             break;
        case CPLD_SFP_PORT_8_15_PRES_MASK:
             reg = CPLD_SFP_PORT_8_15_PRES_MASK_REG;
             break;
        case CPLD_SFP_PORT_32_39_PRES_MASK:
             reg = CPLD_SFP_PORT_32_39_PRES_MASK_REG;
             break;
        case CPLD_SFP_PORT_40_47_PRES_MASK:
             reg = CPLD_SFP_PORT_40_47_PRES_MASK_REG;
             break;
        case CPLD_SFP_PORT_0_7_PLUG_EVENT:
             reg = CPLD_SFP_PORT_0_7_PLUG_EVENT_REG;
             break;
        case CPLD_SFP_PORT_8_15_PLUG_EVENT:
             reg = CPLD_SFP_PORT_8_15_PLUG_EVENT_REG;
             break;
        case CPLD_SFP_PORT_32_39_PLUG_EVENT:
             reg = CPLD_SFP_PORT_32_39_PLUG_EVENT_REG;
             break;
        case CPLD_SFP_PORT_40_47_PLUG_EVENT:
             reg = CPLD_SFP_PORT_40_47_PLUG_EVENT_REG;
             break;
        case CPLD_SFP_PORT_0_7_RX_LOS:
             reg = CPLD_SFP_PORT_0_7_RX_LOS_REG;
             break;
        case CPLD_SFP_PORT_8_15_RX_LOS:
             reg = CPLD_SFP_PORT_8_15_RX_LOS_REG;
             break;
        case CPLD_SFP_PORT_32_39_RX_LOS:
             reg = CPLD_SFP_PORT_32_39_RX_LOS_REG;
             break;
        case CPLD_SFP_PORT_40_47_RX_LOS:
             reg = CPLD_SFP_PORT_40_47_RX_LOS_REG;
             break;
        case CPLD_SFP_PORT_0_7_TX_FAULT:
             reg = CPLD_SFP_PORT_0_7_TX_FAULT_REG;
             break;
        case CPLD_SFP_PORT_8_15_TX_FAULT:
             reg = CPLD_SFP_PORT_8_15_TX_FAULT_REG;
             break;
        case CPLD_SFP_PORT_32_39_TX_FAULT:
             reg = CPLD_SFP_PORT_32_39_TX_FAULT_REG;
             break;
        case CPLD_SFP_PORT_40_47_TX_FAULT:
             reg = CPLD_SFP_PORT_40_47_TX_FAULT_REG;
             break;
        case CPLD_SFP_PORT_0_7_TX_DISABLE:
             reg = CPLD_SFP_PORT_0_7_TX_DISABLE_REG;
             break;
        case CPLD_SFP_PORT_8_15_TX_DISABLE:
             reg = CPLD_SFP_PORT_8_15_TX_DISABLE_REG;
             break;
        case CPLD_SFP_PORT_32_39_TX_DISABLE:
             reg = CPLD_SFP_PORT_32_39_TX_DISABLE_REG;
             break;
        case CPLD_SFP_PORT_40_47_TX_DISABLE:
             reg = CPLD_SFP_PORT_40_47_TX_DISABLE_REG;
             break;
        case CPLD_SFP_PORT_16_23_PRES:
             reg = CPLD_SFP_PORT_16_23_PRES_REG;
             break;
        case CPLD_SFP_PORT_24_31_PRES:
             reg = CPLD_SFP_PORT_24_31_PRES_REG;
             break;
        case CPLD_SFP_PORT_48_55_PRES:
             reg = CPLD_SFP_PORT_48_55_PRES_REG;
             break;
        case CPLD_SFP_PORT_56_63_PRES:
             reg = CPLD_SFP_PORT_56_63_PRES_REG;
             break;
        case CPLD_SFP_PORT_16_23_PRES_MASK:
             reg = CPLD_SFP_PORT_16_23_PRES_MASK_REG;
             break;
        case CPLD_SFP_PORT_24_31_PRES_MASK:
             reg = CPLD_SFP_PORT_24_31_PRES_MASK_REG;
             break;
        case CPLD_SFP_PORT_48_55_PRES_MASK:
             reg = CPLD_SFP_PORT_48_55_PRES_MASK_REG;
             break;
        case CPLD_SFP_PORT_56_63_PRES_MASK:
             reg = CPLD_SFP_PORT_56_63_PRES_MASK_REG;
             break;
        case CPLD_SFP_PORT_16_23_PLUG_EVENT:
             reg = CPLD_SFP_PORT_16_23_PLUG_EVENT_REG;
             break;
        case CPLD_SFP_PORT_24_31_PLUG_EVENT:
             reg = CPLD_SFP_PORT_24_31_PLUG_EVENT_REG;
             break;
        case CPLD_SFP_PORT_48_55_PLUG_EVENT:
             reg = CPLD_SFP_PORT_48_55_PLUG_EVENT_REG;
             break;
        case CPLD_SFP_PORT_56_63_PLUG_EVENT:
             reg = CPLD_SFP_PORT_56_63_PLUG_EVENT_REG;
             break;
        case CPLD_SFP_PORT_16_23_RX_LOS:
             reg = CPLD_SFP_PORT_16_23_RX_LOS_REG;
             break;
        case CPLD_SFP_PORT_24_31_RX_LOS:
             reg = CPLD_SFP_PORT_24_31_RX_LOS_REG;
             break;
        case CPLD_SFP_PORT_48_55_RX_LOS:
             reg = CPLD_SFP_PORT_48_55_RX_LOS_REG;
             break;
        case CPLD_SFP_PORT_56_63_RX_LOS:
             reg = CPLD_SFP_PORT_56_63_RX_LOS_REG;
             break;
        case CPLD_SFP_PORT_16_23_TX_FAULT:
             reg = CPLD_SFP_PORT_16_23_TX_FAULT_REG;
             break;
        case CPLD_SFP_PORT_24_31_TX_FAULT:
             reg = CPLD_SFP_PORT_24_31_TX_FAULT_REG;
             break;
        case CPLD_SFP_PORT_48_55_TX_FAULT:
             reg = CPLD_SFP_PORT_48_55_TX_FAULT_REG;
             break;
        case CPLD_SFP_PORT_56_63_TX_FAULT:
             reg = CPLD_SFP_PORT_56_63_TX_FAULT_REG;
             break;
         case CPLD_SFP_PORT_16_23_TX_DISABLE:
             reg = CPLD_SFP_PORT_16_23_TX_DISABLE_REG;
             break;
        case CPLD_SFP_PORT_24_31_TX_DISABLE:
             reg = CPLD_SFP_PORT_24_31_TX_DISABLE_REG;
             break;
        case CPLD_SFP_PORT_48_55_TX_DISABLE:
             reg = CPLD_SFP_PORT_48_55_TX_DISABLE_REG;
             break;
        case CPLD_SFP_PORT_56_63_TX_DISABLE:
             reg = CPLD_SFP_PORT_56_63_TX_DISABLE_REG;
             break;
        case CPLD_QSFP_PORT_64_71_INTR:
             reg = CPLD_QSFP_PORT_64_71_INTR_REG;
             break;
        case CPLD_QSFPDD_PORT_INTR:
             reg = CPLD_QSFPDD_PORT_INTR_REG;
             break;
        case CPLD_QSFP_PORT_64_71_PRES:
             reg = CPLD_QSFP_PORT_64_71_PRES_REG;
             break;
        case CPLD_QSFPDD_PORT_0_5_PRES:
             reg = CPLD_QSFPDD_PORT_0_5_PRES_REG;
             break;
        case CPLD_QSFP_PORT_64_71_INTR_MASK:
             reg = CPLD_QSFP_PORT_64_71_INTR_MASK_REG;
             break;
        case CPLD_QSFPDD_PORT_INTR_MASK:
             reg = CPLD_QSFPDD_PORT_INTR_MASK_REG;
             break;
        case CPLD_QSFP_PORT_64_71_PRES_MASK:
             reg = CPLD_QSFP_PORT_64_71_PRES_MASK_REG;
             break;
        case CPLD_QSFPDD_PORT_0_5_PRES_MASK:
             reg = CPLD_QSFPDD_PORT_0_5_PRES_MASK_REG;
             break;
        case CPLD_QSFP_PORT_64_71_INTR_EVENT:
             reg = CPLD_QSFP_PORT_64_71_INTR_EVENT_REG;
             break;
        case CPLD_QSFPDD_PORT_INTR_EVENT:
             reg = CPLD_QSFPDD_PORT_INTR_EVENT_REG;
             break;
        case CPLD_QSFP_PORT_64_71_PLUG_EVENT:
             reg = CPLD_QSFP_PORT_64_71_PLUG_EVENT_REG;
             break;
        case CPLD_QSFPDD_PORT_0_5_PLUG_EVENT:
             reg = CPLD_QSFPDD_PORT_0_5_PLUG_EVENT_REG;
             break;
        case CPLD_QSFP_PORT_64_71_RST:
             reg = CPLD_QSFP_PORT_64_71_RST_REG;
             break;
        case CPLD_QSFPDD_PORT_RST:
             reg = CPLD_QSFPDD_PORT_RST_REG;
             break;
        case CPLD_QSFP_PORT_64_71_LPMODE:
             reg = CPLD_QSFP_PORT_64_71_LPMODE_REG;
             break;
        case CPLD_QSFPDD_PORT_LPMODE:
             reg = CPLD_QSFPDD_PORT_LPMODE_REG;
             break;
        case CPLD_BEACON_LED:
             reg = CPLD_BEACON_LED_REG;
             break;
        case CPLD_QSFPDD_PORT_0_1_LED:
             reg = CPLD_QSFPDD_PORT_0_1_LED_REG;
             break;
        case CPLD_QSFPDD_PORT_2_3_LED:
             reg = CPLD_QSFPDD_PORT_2_3_LED_REG;
             break;
        case CPLD_QSFPDD_PORT_4_5_LED:
             reg = CPLD_QSFPDD_PORT_4_5_LED_REG;
             break;
        default:
            return -EINVAL;
    }
    return read_cpld_reg(dev, buf, reg);
}

/* handle read for hw_rev attributes */
static ssize_t read_hw_rev_cb(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg = CPLD_HW_REV_REG;
    u8 reg_val;
    u8 res;

    if (!read_cpld_reg_raw_byte(dev, reg, &reg_val))
        return -1;

    switch (attr->index) {
        case CPLD_HW_REV:
             HW_REV_GET(reg_val, res);
             break;
        case CPLD_DEPH_REV:
             DEPH_REV_GET(reg_val, res);
             break;
        case CPLD_BUILD_REV:
             BUILD_REV_GET(reg_val, res);
             break;
        case CPLD_ID_TYPE:
             ID_TYPE_GET(reg_val, res);
             break;
        default:
            return -EINVAL;
    }
    return sprintf(buf, "0x%02x\n", res);
}

/* handle read for cpld_version attributes */
static ssize_t read_cpld_version_cb(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg = CPLD_VERSION_REG;
    u8 reg_val;
    u8 res;

    if (!read_cpld_reg_raw_byte(dev, reg, &reg_val))
        return -1;

    switch (attr->index) {
        case CPLD_MAJOR_VER:
             CPLD_MAJOR_VERSION_GET(reg_val, res);
             break;
        case CPLD_MINOR_VER:
             CPLD_MINOR_VERSION_GET(reg_val, res);
             break;
        default:
            return -EINVAL;
    }
    return sprintf(buf, "0x%02x\n", res);
}

/* handle write for attributes */
static ssize_t write_cpld_cb(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg = 0;

    switch (attr->index) {
        case CPLD_MAC_OP2_INTR_MASK:
             reg = CPLD_MAC_OP2_INTR_MASK_REG;
             break;
        case CPLD_10GPHY_INTR_MASK:
             reg = CPLD_10GPHY_INTR_MASK_REG;
             break;
        case CPLD_CPLD_FRU_INTR_MASK:
             reg = CPLD_CPLD_FRU_INTR_MASK_REG;
             break;
        case CPLD_MGMT_SFP_PORT_STATUS_MASK:
             reg = CPLD_MGMT_SFP_PORT_STATUS_MASK_REG;
             break;
        case CPLD_MISC_INTR_MASK:
             reg = CPLD_MISC_INTR_MASK_REG;
             break;
        case CPLD_SYSTEM_INTR_MASK:
             reg = CPLD_SYSTEM_INTR_MASK_REG;
             break;
        case CPLD_MAC_OP2_RST:
             reg = CPLD_MAC_OP2_RST_REG;
             break;
        case CPLD_BMC_NTM_RST:
             reg = CPLD_BMC_NTM_RST_REG;
             break;
        case CPLD_USB_RST:
             reg = CPLD_USB_RST_REG;
             break;
        case CPLD_CPLD_RST:
             reg = CPLD_CPLD_RST_REG;
             break;
        case CPLD_MUX_RST_1:
             reg = CPLD_MUX_RST_1_REG;
             break;
        case CPLD_MUX_RST_2:
             reg = CPLD_MUX_RST_2_REG;
             break;
        case CPLD_MISC_RST:
             reg = CPLD_MISC_RST_REG;
             break;
        case CPLD_PUSHBTN:
             reg = CPLD_PUSHBTN_REG;
             break;
        case CPLD_MGMT_SFP_PORT_CONF:
             reg = CPLD_MGMT_SFP_PORT_CONF_REG;
             break;
        case CPLD_MISC:
             reg = CPLD_MISC_REG;
             break;
        case CPLD_MUX_CTRL:
             reg = CPLD_MUX_CTRL_REG;
             break;
        case CPLD_SYS_LED_CTRL_1:
             reg = CPLD_SYS_LED_CTRL_1_REG;
             break;
        case CPLD_SYS_LED_CTRL_2:
             reg = CPLD_SYS_LED_CTRL_2_REG;
             break;
        case CPLD_OOB_LED_CTRL:
             reg = CPLD_OOB_LED_CTRL_REG;
             break;
        case CPLD_HBM_PW_EN:
             reg = CPLD_HBM_PW_EN_REG;
             break;
        case CPLD_SFP_PORT_0_7_PRES_MASK:
             reg = CPLD_SFP_PORT_0_7_PRES_MASK_REG;
             break;
        case CPLD_SFP_PORT_8_15_PRES_MASK:
             reg = CPLD_SFP_PORT_8_15_PRES_MASK_REG;
             break;
        case CPLD_SFP_PORT_32_39_PRES_MASK:
             reg = CPLD_SFP_PORT_32_39_PRES_MASK_REG;
             break;
        case CPLD_SFP_PORT_40_47_PRES_MASK:
             reg = CPLD_SFP_PORT_40_47_PRES_MASK_REG;
             break;
        case CPLD_SFP_PORT_16_23_PRES_MASK:
             reg = CPLD_SFP_PORT_16_23_PRES_MASK_REG;
             break;
        case CPLD_SFP_PORT_24_31_PRES_MASK:
             reg = CPLD_SFP_PORT_24_31_PRES_MASK_REG;
             break;
        case CPLD_SFP_PORT_48_55_PRES_MASK:
             reg = CPLD_SFP_PORT_48_55_PRES_MASK_REG;
             break;
        case CPLD_SFP_PORT_56_63_PRES_MASK:
             reg = CPLD_SFP_PORT_56_63_PRES_MASK_REG;
             break;
        case CPLD_QSFP_PORT_64_71_INTR_MASK:
             reg = CPLD_QSFP_PORT_64_71_INTR_MASK_REG;
             break;
        case CPLD_QSFPDD_PORT_INTR_MASK:
             reg = CPLD_QSFPDD_PORT_INTR_MASK_REG;
             break;
        case CPLD_QSFP_PORT_64_71_PRES_MASK:
             reg = CPLD_QSFP_PORT_64_71_PRES_MASK_REG;
             break;
        case CPLD_QSFPDD_PORT_0_5_PRES_MASK:
             reg = CPLD_QSFPDD_PORT_0_5_PRES_MASK_REG;
             break;
        case CPLD_QSFP_PORT_64_71_RST:
             reg = CPLD_QSFP_PORT_64_71_RST_REG;
             break;
        case CPLD_QSFPDD_PORT_RST:
             reg = CPLD_QSFPDD_PORT_RST_REG;
             break;
        case CPLD_QSFP_PORT_64_71_LPMODE:
             reg = CPLD_QSFP_PORT_64_71_LPMODE_REG;
             break;
        case CPLD_QSFPDD_PORT_LPMODE:
             reg = CPLD_QSFPDD_PORT_LPMODE_REG;
             break;
        case CPLD_BEACON_LED:
             reg = CPLD_BEACON_LED_REG;
             break;
        case CPLD_QSFPDD_PORT_0_1_LED:
             reg = CPLD_QSFPDD_PORT_0_1_LED_REG;
             break;
        case CPLD_QSFPDD_PORT_2_3_LED:
             reg = CPLD_QSFPDD_PORT_2_3_LED_REG;
             break;
        case CPLD_QSFPDD_PORT_4_5_LED:
             reg = CPLD_QSFPDD_PORT_4_5_LED_REG;
             break;
        case CPLD_SFP_PORT_0_7_TX_DISABLE:
             reg = CPLD_SFP_PORT_0_7_TX_DISABLE_REG;
             break;
        case CPLD_SFP_PORT_8_15_TX_DISABLE:
             reg = CPLD_SFP_PORT_8_15_TX_DISABLE_REG;
             break;
        case CPLD_SFP_PORT_32_39_TX_DISABLE:
             reg = CPLD_SFP_PORT_32_39_TX_DISABLE_REG;
             break;
        case CPLD_SFP_PORT_40_47_TX_DISABLE:
             reg = CPLD_SFP_PORT_40_47_TX_DISABLE_REG;
             break;
        case CPLD_SFP_PORT_16_23_TX_DISABLE:
             reg = CPLD_SFP_PORT_16_23_TX_DISABLE_REG;
             break;
        case CPLD_SFP_PORT_24_31_TX_DISABLE:
             reg = CPLD_SFP_PORT_24_31_TX_DISABLE_REG;
             break;
        case CPLD_SFP_PORT_48_55_TX_DISABLE:
             reg = CPLD_SFP_PORT_48_55_TX_DISABLE_REG;
             break;
        case CPLD_SFP_PORT_56_63_TX_DISABLE:
             reg = CPLD_SFP_PORT_56_63_TX_DISABLE_REG;
             break;
        default:
            return -EINVAL;
    }
    return write_cpld_reg(dev, buf, count, reg);
}

/* set cpld register value */
static ssize_t write_cpld_reg(struct device *dev,
                    const char *buf,
                    size_t count,
                    u8 reg)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg_val;
    int ret;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
               client, reg, reg_val);

    if (unlikely(ret < 0)) {
        dev_err(dev, "I2C_WRITE_BYTE_DATA error, return=%d\n", ret);
        return ret;
    }

    return count;
}

/* add valid cpld client to list */
static void s9701_cpld_add_client(struct i2c_client *client)
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
static void s9701_cpld_remove_client(struct i2c_client *client)
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
static int s9701_cpld_probe(struct i2c_client *client,
                    const struct i2c_device_id *dev_id)
{
    int status;
    struct cpld_data *data = NULL;
    int ret = -EPERM;
    int idx;

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

    CPLD_ID_ID_GET(ret,  idx);

    if (INVALID(idx, cpld1, cpld4)) {
        dev_info(&client->dev,
            "cpld id %d(device) not valid\n", idx);
        //status = -EPERM;
        //goto exit;
    }

    data->index = dev_id->driver_data;

    /* register sysfs hooks for different cpld group */
    dev_info(&client->dev, "probe cpld with index %d\n", data->index);
    switch (data->index) {
    case cpld1:
        status = sysfs_create_group(&client->dev.kobj,
                    &s9701_cpld1_group);
        break;
    case cpld2:    	  
        status = sysfs_create_group(&client->dev.kobj,
                    &s9701_cpld2_group);
        break;
    case cpld3:
        status = sysfs_create_group(&client->dev.kobj,
                    &s9701_cpld3_group);
        break;
    case cpld4:
        status = sysfs_create_group(&client->dev.kobj,
                    &s9701_cpld4_group);
        break;        
    default:
        status = -EINVAL;
    }

    if (status)
        goto exit;

    dev_info(&client->dev, "chip found\n");

    /* add probe chip to client list */
    s9701_cpld_add_client(client);

    return 0;
exit:
    switch (data->index) {
    case cpld1:
        sysfs_remove_group(&client->dev.kobj, &s9701_cpld1_group);
        break;
    case cpld2:    	  
    	  sysfs_remove_group(&client->dev.kobj, &s9701_cpld2_group);
        break;
    case cpld3:
    	  sysfs_remove_group(&client->dev.kobj, &s9701_cpld3_group);
        break;
    case cpld4:
    	  sysfs_remove_group(&client->dev.kobj, &s9701_cpld4_group);
        break;

    default:
        break;
    }
    return status;
}

/* cpld drvier remove */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
static int s9701_cpld_remove(struct i2c_client *client)
#else
static void s9701_cpld_remove(struct i2c_client *client)
#endif
{
    struct cpld_data *data = i2c_get_clientdata(client);

    switch (data->index) {
    case cpld1:
        sysfs_remove_group(&client->dev.kobj, &s9701_cpld1_group);
        break;
    case cpld2:
        sysfs_remove_group(&client->dev.kobj, &s9701_cpld2_group);
        break;
    case cpld3:
    	sysfs_remove_group(&client->dev.kobj, &s9701_cpld3_group);
        break;
    case cpld4:
    	sysfs_remove_group(&client->dev.kobj, &s9701_cpld4_group);
        break;
    }

    s9701_cpld_remove_client(client);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
    return 0;
#endif
}

MODULE_DEVICE_TABLE(i2c, s9701_cpld_id);

static struct i2c_driver s9701_cpld_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name = "x86_64_ufispace_s9701_78dc_cpld",
    },
    .probe = s9701_cpld_probe,
    .remove = s9701_cpld_remove,
    .id_table = s9701_cpld_id,
    .address_list = cpld_i2c_addr,
};

/* provide cpld register read */
/* cpld_idx indicate the index of cpld device */
int s9701_cpld_read(u8 cpld_idx,
                u8 reg)
{
    struct list_head *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EPERM;
    struct cpld_data *data;

    list_for_each(list_node, &cpld_client_list) {
        cpld_node = list_entry(list_node,
                    struct cpld_client_node, list);
        data = i2c_get_clientdata(cpld_node->client);
        if (data->index == cpld_idx) {
            DEBUG_PRINT("cpld_idx=%d, read reg 0x%02x",
                    cpld_idx, reg);
            I2C_READ_BYTE_DATA(ret, &data->access_lock,
                    cpld_node->client, reg);
            DEBUG_PRINT("cpld_idx=%d, read reg 0x%02x = 0x%02x",
                    cpld_idx, reg, ret);
        break;
        }
    }

    return ret;
}
EXPORT_SYMBOL(s9701_cpld_read);

/* provide cpld register write */
/* cpld_idx indicate the index of cpld device */
int s9701_cpld_write(u8 cpld_idx,
                u8 reg,
                u8 value)
{
    struct list_head *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EIO;
    struct cpld_data *data;

    list_for_each(list_node, &cpld_client_list) {
        cpld_node = list_entry(list_node,
                    struct cpld_client_node, list);
        data = i2c_get_clientdata(cpld_node->client);

        if (data->index == cpld_idx) {
                        I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
                        cpld_node->client,
                        reg, value);
            DEBUG_PRINT("cpld_idx=%d, write reg 0x%02x val 0x%02x, ret=%d",
                            cpld_idx, reg, value, ret);
            break;
        }
    }

    return ret;
}
EXPORT_SYMBOL(s9701_cpld_write);

static int __init s9701_cpld_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&s9701_cpld_driver);
}

static void __exit s9701_cpld_exit(void)
{
    i2c_del_driver(&s9701_cpld_driver);
}

MODULE_AUTHOR("Leo Lin <leo.yt.lin@ufispace.com>");
MODULE_DESCRIPTION("s9701_cpld driver");
MODULE_LICENSE("GPL");

module_init(s9701_cpld_init);
module_exit(s9701_cpld_exit);
