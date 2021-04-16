/*
 * A i2c cpld driver for the ufispace_s9600_64x
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
#include "x86-64-ufispace-s9600-64x-cpld.h"

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
enum cpld_sysfs_attributes {
    CPLD_BOARD_TYPE,
    CPLD_EXT_BOARD_TYPE,
    CPLD_VERSION,
    CPLD_ID,
    CPLD_INTR_MASK_1,
    CPLD_INTR_MASK_2,
    CPLD_INTR_MASK_3,
    CPLD_INTR_EVENT_1,
    CPLD_INTR_EVENT_2,
    CPLD_INTR_EVENT_3,
    CPLD_INTR_STATUS_1,
    CPLD_INTR_STATUS_2,
    CPLD_INTR_STATUS_3,
    CPLDX_INTR_MASK,
    CPLD_LED_MASK,
    CPLD_SYS_THERMAL_STATUS,
    CPLD_DAUGHTER_INFO,
    CPLD_PSU_STATUS,
    CPLD_MUX_CTRL,
    CPLD_PUSH_BUTTON,
    CPLDX_PLUG_EVT_0,
    CPLDX_PLUG_EVT_1,
    CPLD_MISC_STATUS,
    CPLD_MAC_STATUS,
    CPLDX_EVT_SUMMARY,
    CPLD_MAC_ROV,
    CPLD_QSFP_PRESENT_0,
    CPLD_QSFP_PRESENT_1,
    CPLD_QSFP_INTR_0,
    CPLD_QSFP_INTR_1,
    CPLD_QSFP_RESET_0,
    CPLD_QSFP_RESET_1,
    CPLD_QSFP_LPMODE_0,
    CPLD_QSFP_LPMODE_1,
    CPLD_RESET_1,
    CPLD_RESET_2,
    CPLD_RESET_3,
    CPLD_RESET_4,
    CPLD_RESET_5,
    CPLD_SYS_THERMAL_MASK,
    CPLD_SYS_THERMAL_EVENT,
    CPLD_TIMING_CTRL,
    CPLD_GNSS_CTRL,
    CPLD_SYSTEM_LED_0,
    CPLD_SYSTEM_LED_1,
    CPLD_SYSTEM_LED_2,
    CPLD_BMC_WD,
    CPLD_SFP_STATUS_CPU,
    CPLD_SFP_CONFIG_CPU,
    CPLD_SFP_PLUG_EVT_CPU,
    CPLD_SFP_LED_CTRL,
    CPLD_QSFP_LED_CTRL_0,
    CPLD_QSFP_LED_CTRL_1,
    CPLD_QSFP_LED_CTRL_2,
    CPLD_QSFP_LED_CTRL_3,
    CPLD_QSFP_LED_CTRL_4,
    CPLD_QSFP_LED_CTRL_5,
    CPLD_QSFP_LED_CTRL_6,
    CPLD_QSFP_LED_CTRL_7,
    CPLD_QSFP_LED_CTRL_8,
    CPLD_QSFP_LED_CTRL_9,
    CPLD_QSFP_LED_CTRL_10,
    CPLD_QSFP_LED_CTRL_11,
    CPLD_QSFP_LED_CTRL_12,
    CPLD_QSFP_LED_CTRL_13,
    CPLD_QSFP_LED_CTRL_14,
    CPLD_QSFP_LED_CTRL_15,
    CPLD_SFP_STATUS_INBAND,
    CPLD_SFP_CONFIG_INBAND,
    CPLD_SFP_PLUG_EVT_INBAND,
    CPLD_MAC_PG,
    CPLD_P5V_P3V3_PG,
    CPLD_PHY_PG,
    CPLD_HBM_PWR_EN,
	CPLD_MAX
};

/* CPLD sysfs attributes hook functions  */
static ssize_t read_cpld_callback(struct device *dev,
        struct device_attribute *da, char *buf);
static ssize_t write_cpld_callback(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_cpld_reg(struct device *dev, char *buf, u8 reg);
static ssize_t write_cpld_reg(struct device *dev, const char *buf, size_t count, u8 reg);

static LIST_HEAD(cpld_client_list);  /* client list for cpld */
static struct mutex list_lock;  /* mutex for client list */

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

struct cpld_data {
    int index;                  /* CPLD index */
    struct mutex access_lock;       /* mutex for cpld access */
    u8 access_reg;              /* register to access */
};

/* CPLD device id and data */
static const struct i2c_device_id cpld_id[] = {
    { "s9600_64x_cpld1",  cpld1 },
    { "s9600_64x_cpld2",  cpld2 },
    { "s9600_64x_cpld3",  cpld3 },
    { "s9600_64x_cpld4",  cpld4 },
    { "s9600_64x_cpld5",  cpld5 },
    {}
};

/* Addresses scanned for cpld */
static const unsigned short cpld_i2c_addr[] = { 0x38, 0x39, 0x3A, 0x3B, 0x3C, I2C_CLIENT_END };

/* define all support register access of cpld in attribute */

static SENSOR_DEVICE_ATTR(cpld_board_type, S_IRUGO,
        read_cpld_callback, NULL, CPLD_BOARD_TYPE);
static SENSOR_DEVICE_ATTR(cpld_ext_board_type, S_IRUGO,
        read_cpld_callback, NULL, CPLD_EXT_BOARD_TYPE);
static SENSOR_DEVICE_ATTR(cpld_version, S_IRUGO,
        read_cpld_callback, NULL, CPLD_VERSION);
static SENSOR_DEVICE_ATTR(cpld_id, S_IRUGO, read_cpld_callback, NULL, CPLD_ID);
static SENSOR_DEVICE_ATTR(cpld_intr_mask_1, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_INTR_MASK_1);
static SENSOR_DEVICE_ATTR(cpld_intr_mask_2, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_INTR_MASK_2);
static SENSOR_DEVICE_ATTR(cpld_intr_mask_3, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_INTR_MASK_3);
static SENSOR_DEVICE_ATTR(cpld_intr_event_1, S_IRUGO,
        read_cpld_callback, NULL, CPLD_INTR_EVENT_1);
static SENSOR_DEVICE_ATTR(cpld_intr_event_2, S_IRUGO,
        read_cpld_callback, NULL, CPLD_INTR_EVENT_2);
static SENSOR_DEVICE_ATTR(cpld_intr_event_3, S_IRUGO,
        read_cpld_callback, NULL, CPLD_INTR_EVENT_3);
static SENSOR_DEVICE_ATTR(cpld_intr_status_1, S_IRUGO,
        read_cpld_callback, NULL, CPLD_INTR_STATUS_1);
static SENSOR_DEVICE_ATTR(cpld_intr_status_2, S_IRUGO,
        read_cpld_callback, NULL, CPLD_INTR_STATUS_2);
static SENSOR_DEVICE_ATTR(cpld_intr_status_3, S_IRUGO,
        read_cpld_callback, NULL, CPLD_INTR_STATUS_3);
static SENSOR_DEVICE_ATTR(cpld_led_mask, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_LED_MASK);
static SENSOR_DEVICE_ATTR(cpldx_intr_mask, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLDX_INTR_MASK);
static SENSOR_DEVICE_ATTR(cpld_sys_thermal_status, S_IRUGO,
        read_cpld_callback, NULL, CPLD_SYS_THERMAL_STATUS);
static SENSOR_DEVICE_ATTR(cpld_daughter_info, S_IRUGO,
        read_cpld_callback, NULL, CPLD_DAUGHTER_INFO);
static SENSOR_DEVICE_ATTR(cpld_psu_status, S_IRUGO,
        read_cpld_callback, NULL, CPLD_PSU_STATUS);
static SENSOR_DEVICE_ATTR(cpld_mux_ctrl, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_MUX_CTRL);
static SENSOR_DEVICE_ATTR(cpld_push_button, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_PUSH_BUTTON);
static SENSOR_DEVICE_ATTR(cpldx_plug_evt_0, S_IWUSR | S_IRUGO,
        read_cpld_callback, NULL, CPLDX_PLUG_EVT_0);
static SENSOR_DEVICE_ATTR(cpldx_plug_evt_1, S_IWUSR | S_IRUGO,
        read_cpld_callback, NULL, CPLDX_PLUG_EVT_1);
static SENSOR_DEVICE_ATTR(cpld_misc_status, S_IRUGO,
        read_cpld_callback, NULL, CPLD_MISC_STATUS);
static SENSOR_DEVICE_ATTR(cpld_mac_status, S_IRUGO,
        read_cpld_callback, NULL, CPLD_MAC_STATUS);
static SENSOR_DEVICE_ATTR(cpldx_evt_summary, S_IRUGO,
        read_cpld_callback, NULL, CPLDX_EVT_SUMMARY);
static SENSOR_DEVICE_ATTR(cpld_mac_rov, S_IRUGO,
        read_cpld_callback, NULL, CPLD_MAC_ROV);
static SENSOR_DEVICE_ATTR(cpld_qsfp_present_0, S_IRUGO,
        read_cpld_callback, NULL, CPLD_QSFP_PRESENT_0);
static SENSOR_DEVICE_ATTR(cpld_qsfp_present_1, S_IRUGO,
        read_cpld_callback, NULL, CPLD_QSFP_PRESENT_1);
static SENSOR_DEVICE_ATTR(cpld_qsfp_intr_0, S_IRUGO,
        read_cpld_callback, NULL, CPLD_QSFP_INTR_0);
static SENSOR_DEVICE_ATTR(cpld_qsfp_intr_1, S_IRUGO,
        read_cpld_callback, NULL, CPLD_QSFP_INTR_1);
static SENSOR_DEVICE_ATTR(cpld_qsfp_reset_0, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback,
                CPLD_QSFP_RESET_0);
static SENSOR_DEVICE_ATTR(cpld_qsfp_reset_1, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback,
                CPLD_QSFP_RESET_1);
static SENSOR_DEVICE_ATTR(cpld_qsfp_lpmode_0, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback,
                CPLD_QSFP_LPMODE_0);
static SENSOR_DEVICE_ATTR(cpld_qsfp_lpmode_1, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback,
                CPLD_QSFP_LPMODE_1);
static SENSOR_DEVICE_ATTR(cpld_reset_1, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_RESET_1);
static SENSOR_DEVICE_ATTR(cpld_reset_2, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_RESET_2);
static SENSOR_DEVICE_ATTR(cpld_reset_3, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_RESET_3);
static SENSOR_DEVICE_ATTR(cpld_reset_4, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_RESET_4);
static SENSOR_DEVICE_ATTR(cpld_reset_5, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_RESET_5);
static SENSOR_DEVICE_ATTR(cpld_sys_thermal_mask, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_SYS_THERMAL_MASK);
static SENSOR_DEVICE_ATTR(cpld_sys_thermal_event, S_IRUGO,
        read_cpld_callback, NULL, CPLD_SYS_THERMAL_EVENT);
static SENSOR_DEVICE_ATTR(cpld_timing_ctrl, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_TIMING_CTRL);
static SENSOR_DEVICE_ATTR(cpld_gnss_ctrl, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_GNSS_CTRL);
static SENSOR_DEVICE_ATTR(cpld_system_led_0, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_SYSTEM_LED_0);
static SENSOR_DEVICE_ATTR(cpld_system_led_1, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_SYSTEM_LED_1);
static SENSOR_DEVICE_ATTR(cpld_system_led_2, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_SYSTEM_LED_2);
static SENSOR_DEVICE_ATTR(cpld_bmc_wd, S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_BMC_WD);
static SENSOR_DEVICE_ATTR(cpld_sfp_status_cpu, S_IRUGO,
        read_cpld_callback, NULL, CPLD_SFP_STATUS_CPU);
static SENSOR_DEVICE_ATTR(cpld_sfp_config_cpu, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_SFP_CONFIG_CPU);
static SENSOR_DEVICE_ATTR(cpld_sfp_plug_evt_cpu, S_IRUGO,
        read_cpld_callback, NULL, CPLD_SFP_PLUG_EVT_CPU);
static SENSOR_DEVICE_ATTR(cpld_sfp_led_ctrl, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_SFP_LED_CTRL);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_ctrl_0, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_QSFP_LED_CTRL_0);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_ctrl_1, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_QSFP_LED_CTRL_1);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_ctrl_2, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_QSFP_LED_CTRL_2);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_ctrl_3, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_QSFP_LED_CTRL_3);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_ctrl_4, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_QSFP_LED_CTRL_4);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_ctrl_5, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_QSFP_LED_CTRL_5);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_ctrl_6, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_QSFP_LED_CTRL_6);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_ctrl_7, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_QSFP_LED_CTRL_7);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_ctrl_8, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_QSFP_LED_CTRL_8);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_ctrl_9, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_QSFP_LED_CTRL_9);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_ctrl_10, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_QSFP_LED_CTRL_10);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_ctrl_11, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_QSFP_LED_CTRL_11);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_ctrl_12, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_QSFP_LED_CTRL_12);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_ctrl_13, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_QSFP_LED_CTRL_13);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_ctrl_14, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_QSFP_LED_CTRL_14);
static SENSOR_DEVICE_ATTR(cpld_qsfp_led_ctrl_15, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_QSFP_LED_CTRL_15);
static SENSOR_DEVICE_ATTR(cpld_sfp_status_inband, S_IRUGO,
        read_cpld_callback, NULL, CPLD_SFP_STATUS_INBAND);
static SENSOR_DEVICE_ATTR(cpld_sfp_config_inband, S_IWUSR | S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_SFP_CONFIG_INBAND);
static SENSOR_DEVICE_ATTR(cpld_sfp_plug_evt_inband, S_IRUGO,
        read_cpld_callback, NULL, CPLD_SFP_PLUG_EVT_INBAND);
static SENSOR_DEVICE_ATTR(cpld_mac_pg, S_IRUGO,
        read_cpld_callback, NULL, CPLD_MAC_PG);
static SENSOR_DEVICE_ATTR(cpld_p5v_p3v3_pg, S_IRUGO,
        read_cpld_callback, NULL, CPLD_P5V_P3V3_PG);
static SENSOR_DEVICE_ATTR(cpld_phy_pg, S_IRUGO,
        read_cpld_callback, NULL, CPLD_PHY_PG);
static SENSOR_DEVICE_ATTR(cpld_hbm_pwr_en, S_IRUGO,
        read_cpld_callback, write_cpld_callback, CPLD_HBM_PWR_EN);
/* define support attributes of cpldx */

/* cpld 1 */
static struct attribute *cpld1_attributes[] = {
    &sensor_dev_attr_cpld_board_type.dev_attr.attr,
    &sensor_dev_attr_cpld_ext_board_type.dev_attr.attr,
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_cpld_id.dev_attr.attr,
    &sensor_dev_attr_cpld_intr_mask_1.dev_attr.attr,
    &sensor_dev_attr_cpld_intr_mask_2.dev_attr.attr,
    &sensor_dev_attr_cpld_intr_mask_3.dev_attr.attr,
    &sensor_dev_attr_cpld_intr_event_1.dev_attr.attr,
    &sensor_dev_attr_cpld_intr_event_2.dev_attr.attr,
    &sensor_dev_attr_cpld_intr_event_3.dev_attr.attr,
    &sensor_dev_attr_cpld_intr_status_1.dev_attr.attr,
    &sensor_dev_attr_cpld_intr_status_2.dev_attr.attr,
    &sensor_dev_attr_cpld_intr_status_3.dev_attr.attr,
    &sensor_dev_attr_cpld_sys_thermal_status.dev_attr.attr,
    &sensor_dev_attr_cpld_daughter_info.dev_attr.attr,
    &sensor_dev_attr_cpld_psu_status.dev_attr.attr,
    &sensor_dev_attr_cpld_mux_ctrl.dev_attr.attr,
    &sensor_dev_attr_cpld_push_button.dev_attr.attr,
    &sensor_dev_attr_cpld_misc_status.dev_attr.attr,
    &sensor_dev_attr_cpld_mac_status.dev_attr.attr,
    &sensor_dev_attr_cpld_mac_rov.dev_attr.attr,
    &sensor_dev_attr_cpld_reset_1.dev_attr.attr,
    &sensor_dev_attr_cpld_reset_2.dev_attr.attr,
    &sensor_dev_attr_cpld_reset_3.dev_attr.attr,
    &sensor_dev_attr_cpld_reset_4.dev_attr.attr,
    &sensor_dev_attr_cpld_reset_5.dev_attr.attr,
    &sensor_dev_attr_cpld_sys_thermal_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_sys_thermal_event.dev_attr.attr,
    &sensor_dev_attr_cpld_timing_ctrl.dev_attr.attr,
    &sensor_dev_attr_cpld_gnss_ctrl.dev_attr.attr,
    &sensor_dev_attr_cpld_system_led_0.dev_attr.attr,
    &sensor_dev_attr_cpld_system_led_1.dev_attr.attr,
    &sensor_dev_attr_cpld_system_led_2.dev_attr.attr,
    &sensor_dev_attr_cpld_bmc_wd.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_status_cpu.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_config_cpu.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_plug_evt_cpu.dev_attr.attr,
    &sensor_dev_attr_cpld_mac_pg.dev_attr.attr,
    &sensor_dev_attr_cpld_p5v_p3v3_pg.dev_attr.attr,
    &sensor_dev_attr_cpld_phy_pg.dev_attr.attr,
    &sensor_dev_attr_cpld_hbm_pwr_en.dev_attr.attr,
    NULL
};

/* cpld 2 */
static struct attribute *cpld2_attributes[] = {
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_cpld_id.dev_attr.attr,
    &sensor_dev_attr_cpld_led_mask.dev_attr.attr,
    &sensor_dev_attr_cpldx_intr_mask.dev_attr.attr,
	&sensor_dev_attr_cpldx_plug_evt_0.dev_attr.attr,
	&sensor_dev_attr_cpldx_plug_evt_1.dev_attr.attr,
    &sensor_dev_attr_cpldx_evt_summary.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfp_present_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_present_1.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfp_intr_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_intr_1.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfp_reset_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_reset_1.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfp_lpmode_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_lpmode_1.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_led_ctrl.dev_attr.attr, //cpld2 only
    &sensor_dev_attr_cpld_qsfp_led_ctrl_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_4.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_5.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_6.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_7.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_8.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_9.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_10.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_11.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_12.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_13.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_14.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_15.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_status_inband.dev_attr.attr, //cpld2 only
    &sensor_dev_attr_cpld_sfp_config_inband.dev_attr.attr, //cpld2 only
	&sensor_dev_attr_cpld_sfp_plug_evt_inband.dev_attr.attr, //cpld2 only
    NULL
};

/* cpld 3-5 */
static struct attribute *cpld345_attributes[] = {
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_cpld_id.dev_attr.attr,
    &sensor_dev_attr_cpld_led_mask.dev_attr.attr,
    &sensor_dev_attr_cpldx_intr_mask.dev_attr.attr,
	&sensor_dev_attr_cpldx_plug_evt_0.dev_attr.attr,
	&sensor_dev_attr_cpldx_plug_evt_1.dev_attr.attr,
    &sensor_dev_attr_cpldx_evt_summary.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfp_present_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_present_1.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfp_intr_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_intr_1.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfp_reset_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_reset_1.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfp_lpmode_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_lpmode_1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_4.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_5.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_6.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_7.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_8.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_9.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_10.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_11.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_12.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_13.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_14.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_led_ctrl_15.dev_attr.attr,
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

/* cpld 3/4/5 attributes group */
static const struct attribute_group cpld345_group = {
    .attrs = cpld345_attributes,
};

/* get cpld register value */
static ssize_t read_cpld_callback(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg = 0;

    switch (attr->index) {
        case CPLD_BOARD_TYPE:
            reg = CPLD_BOARD_TYPE_REG;
            break;
        case CPLD_EXT_BOARD_TYPE:
            reg = CPLD_BOARD_TYPE_REG;
            break;
        case CPLD_VERSION:
            reg = CPLD_VERSION_REG;
            break;
        case CPLD_ID:
            reg = CPLD_ID_REG;
            break;
        case CPLD_INTR_MASK_1:
        case CPLD_INTR_MASK_2:
        case CPLD_INTR_MASK_3:
            reg = CPLD_INTR_MASK_BASE_REG + (attr->index - CPLD_INTR_MASK_1);
            break;
        case CPLD_INTR_EVENT_1:
        case CPLD_INTR_EVENT_2:
        case CPLD_INTR_EVENT_3:
            reg = CPLD_INTR_EVENT_BASE_REG + (attr->index - CPLD_INTR_EVENT_1);
            break;
        case CPLD_INTR_STATUS_1:
        case CPLD_INTR_STATUS_2:
        case CPLD_INTR_STATUS_3:
            reg = CPLD_INTR_STATUS_BASE_REG + (attr->index - CPLD_INTR_STATUS_1);
            break;
        case CPLD_LED_MASK:
            reg = CPLD_LED_MASK_REG;
            break;
        case CPLDX_INTR_MASK:
            reg = CPLDX_INTR_MASK_REG;
            break;
        case CPLD_SYS_THERMAL_STATUS:
            reg = CPLD_SYS_THERMAL_STATUS_REG;
            break;
        case CPLD_DAUGHTER_INFO:
            reg = CPLD_DAUGHTER_INFO_REG;
            break;
        case CPLD_PSU_STATUS:
            reg = CPLD_PSU_STATUS_REG;
            break;
        case CPLD_MUX_CTRL:
            reg = CPLD_MUX_CTRL_REG;
            break;
        case CPLD_PUSH_BUTTON:
            reg = CPLD_PUSH_BUTTON_REG;
            break;
        case CPLDX_PLUG_EVT_0:
        case CPLDX_PLUG_EVT_1:
            reg = CPLDX_PLUG_EVT_BASE_REG + (attr->index - CPLDX_PLUG_EVT_0);
            break;
        case CPLD_MISC_STATUS:
            reg = CPLD_MISC_STATUS_REG;
            break;
        case CPLD_MAC_STATUS:
            reg = CPLD_MAC_STATUS_REG;
            break;
        case CPLDX_EVT_SUMMARY:
            reg = CPLDX_EVT_SUMMARY_REG;
            break;
        case CPLD_MAC_ROV:
            reg = CPLD_MAC_ROV_REG;
            break;
        case CPLD_QSFP_PRESENT_0:
        case CPLD_QSFP_PRESENT_1:
            reg = CPLD_QSFP_PRESENT_BASE_REG +
                  (attr->index - CPLD_QSFP_PRESENT_0);
            break;
        case CPLD_QSFP_INTR_0:
        case CPLD_QSFP_INTR_1:
            reg = CPLD_QSFP_INTR_BASE_REG +
                 (attr->index - CPLD_QSFP_INTR_0);
            break;
        case CPLD_QSFP_RESET_0:
        case CPLD_QSFP_RESET_1:
            reg = CPLD_QSFP_RESET_BASE_REG +
                 (attr->index - CPLD_QSFP_RESET_0);
            break;
        case CPLD_QSFP_LPMODE_0:
        case CPLD_QSFP_LPMODE_1:
            reg = CPLD_QSFP_LPMODE_BASE_REG +
                 (attr->index - CPLD_QSFP_LPMODE_0);
            break;
        case CPLD_RESET_1:
        case CPLD_RESET_2:
        case CPLD_RESET_3:
        case CPLD_RESET_4:
        case CPLD_RESET_5:
            reg = CPLD_RESET_BASE_REG +
                 (attr->index - CPLD_RESET_1);
            break;
        case CPLD_SYS_THERMAL_MASK:
        	reg = CPLD_SYS_THERMAL_MASK_REG;
            break;
        case CPLD_SYS_THERMAL_EVENT:
            reg = CPLD_SYS_THERMAL_EVENT_REG;
            break;
         case CPLD_TIMING_CTRL:
            reg = CPLD_TIMING_CTRL_REG;
            break;
        case CPLD_GNSS_CTRL:
            reg = CPLD_GNSS_CTRL_REG;
            break;
        case CPLD_SYSTEM_LED_0:
        case CPLD_SYSTEM_LED_1:
        case CPLD_SYSTEM_LED_2:
            reg = CPLD_SYSTEM_LED_BASE_REG +
                  (attr->index - CPLD_SYSTEM_LED_0);
            break;
        case CPLD_BMC_WD:
            reg = CPLD_BMC_WD_REG;
            break;
        case CPLD_SFP_STATUS_CPU:
            reg = CPLD_SFP_STATUS_CPU_REG;
            break;
        case CPLD_SFP_CONFIG_CPU:
            reg = CPLD_SFP_CONFIG_CPU_REG;
            break;
		case CPLD_SFP_PLUG_EVT_CPU:
            reg = CPLD_SFP_PLUG_EVT_CPU_REG;
            break;
        case CPLD_SFP_LED_CTRL:
            reg = CPLD_SFP_LED_CTRL_REG;
            break;
        case CPLD_QSFP_LED_CTRL_0:
        case CPLD_QSFP_LED_CTRL_1:
        case CPLD_QSFP_LED_CTRL_2:
        case CPLD_QSFP_LED_CTRL_3:
        case CPLD_QSFP_LED_CTRL_4:
        case CPLD_QSFP_LED_CTRL_5:
        case CPLD_QSFP_LED_CTRL_6:
        case CPLD_QSFP_LED_CTRL_7:
        case CPLD_QSFP_LED_CTRL_8:
        case CPLD_QSFP_LED_CTRL_9:
        case CPLD_QSFP_LED_CTRL_10:
        case CPLD_QSFP_LED_CTRL_11:
        case CPLD_QSFP_LED_CTRL_12:
        case CPLD_QSFP_LED_CTRL_13:
        case CPLD_QSFP_LED_CTRL_14:
        case CPLD_QSFP_LED_CTRL_15:
            reg = CPLD_QSFP_LED_CTRL_BASE_REG +
            (attr->index - CPLD_QSFP_LED_CTRL_0);
            break;
        case CPLD_SFP_STATUS_INBAND:
            reg = CPLD_SFP_STATUS_INBAND_REG;
            break;
        case CPLD_SFP_CONFIG_INBAND:
            reg = CPLD_SFP_CONFIG_INBAND_REG;
            break;
		case CPLD_SFP_PLUG_EVT_INBAND:
            reg = CPLD_SFP_PLUG_EVT_INBAND_REG;
            break;
        case CPLD_MAC_PG:
            reg = CPLD_MAC_PG_REG;
            break;
        case CPLD_P5V_P3V3_PG:
            reg = CPLD_P5V_P3V3_PG_REG;
            break;
        case CPLD_PHY_PG:
            reg = CPLD_PHY_PG_REG;
            break;
        case CPLD_HBM_PWR_EN:
            reg = CPLD_HBM_PWR_EN_REG;
            break;
        default:
            return -EINVAL;
    }
    return read_cpld_reg(dev, buf, reg);
}

/* set cpld register value */
static ssize_t write_cpld_callback(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg = 0;

    switch (attr->index) {
        case CPLD_INTR_MASK_1:
        case CPLD_INTR_MASK_2:
        case CPLD_INTR_MASK_3:
            reg = CPLD_INTR_MASK_BASE_REG +
                 (attr->index - CPLD_INTR_MASK_1);
            break;
        case CPLD_LED_MASK:
            reg = CPLD_LED_MASK_REG;
            break;
        case CPLDX_INTR_MASK:
            reg = CPLDX_INTR_MASK_REG;
            break;
        case CPLD_MUX_CTRL:
            reg = CPLD_MUX_CTRL_REG;
            break;
        case CPLD_PUSH_BUTTON:
            reg = CPLD_PUSH_BUTTON_REG;
            break;
        case CPLD_QSFP_RESET_0:
        case CPLD_QSFP_RESET_1:
            reg = CPLD_QSFP_RESET_BASE_REG +
                 (attr->index - CPLD_QSFP_RESET_0);
            break;
        case CPLD_QSFP_LPMODE_0:
        case CPLD_QSFP_LPMODE_1:
            reg = CPLD_QSFP_LPMODE_BASE_REG +
                 (attr->index - CPLD_QSFP_LPMODE_0);
            break;
        case CPLD_RESET_1:
        case CPLD_RESET_2:
        case CPLD_RESET_3:
        case CPLD_RESET_4:
        case CPLD_RESET_5:
            reg = CPLD_RESET_BASE_REG +
                 (attr->index - CPLD_RESET_1);
            break;
        case CPLD_SYS_THERMAL_MASK:
            reg = CPLD_SYS_THERMAL_MASK_REG;
            break;
        case CPLD_TIMING_CTRL:
            reg = CPLD_TIMING_CTRL_REG;
            break;
        case CPLD_GNSS_CTRL:
            reg = CPLD_GNSS_CTRL_REG;
            break;
        case CPLD_SYSTEM_LED_0:
        case CPLD_SYSTEM_LED_1:
        case CPLD_SYSTEM_LED_2:
            reg = CPLD_SYSTEM_LED_BASE_REG +
                  (attr->index - CPLD_SYSTEM_LED_0);
            break;
        case CPLD_BMC_WD:
            reg = CPLD_BMC_WD_REG;
            break;
        case CPLD_SFP_CONFIG_CPU:
            reg = CPLD_SFP_CONFIG_CPU_REG;
            break;
        case CPLD_SFP_LED_CTRL:
            reg = CPLD_SFP_LED_CTRL_REG;
            break;
        case CPLD_QSFP_LED_CTRL_0:
        case CPLD_QSFP_LED_CTRL_1:
        case CPLD_QSFP_LED_CTRL_2:
        case CPLD_QSFP_LED_CTRL_3:
        case CPLD_QSFP_LED_CTRL_4:
        case CPLD_QSFP_LED_CTRL_5:
        case CPLD_QSFP_LED_CTRL_6:
        case CPLD_QSFP_LED_CTRL_7:
        case CPLD_QSFP_LED_CTRL_8:
        case CPLD_QSFP_LED_CTRL_9:
        case CPLD_QSFP_LED_CTRL_10:
        case CPLD_QSFP_LED_CTRL_11:
        case CPLD_QSFP_LED_CTRL_12:
        case CPLD_QSFP_LED_CTRL_13:
        case CPLD_QSFP_LED_CTRL_14:
        case CPLD_QSFP_LED_CTRL_15:
            reg = CPLD_QSFP_LED_CTRL_BASE_REG +
            (attr->index - CPLD_QSFP_LED_CTRL_0);
            break;
        case CPLD_SFP_CONFIG_INBAND:
            reg = CPLD_SFP_CONFIG_INBAND_REG;
            break;
        case CPLD_HBM_PWR_EN:
            reg = CPLD_HBM_PWR_EN_REG;
            break;
        default:
            return -EINVAL;
    }
    return write_cpld_reg(dev, buf, count, reg);
}

/* get cpld register value */
static ssize_t read_cpld_reg(struct device *dev,
                    char *buf,
                    u8 reg)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    int reg_val;

    I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
    if (reg_val < 0)
        return -1;
    else
        return sprintf(buf, "0x%02x\n", reg_val);
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

    return count;
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

    CPLD_ID_ID_GET(ret, idx);

    if (INVALID(idx, cpld1, cpld5)) {
        dev_info(&client->dev,
            "cpld id %d(device) not valid\n", idx);
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
    case cpld4:
    case cpld5:
          status = sysfs_create_group(&client->dev.kobj,
                    &cpld345_group);
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
    case cpld4:
    case cpld5:
          sysfs_remove_group(&client->dev.kobj, &cpld345_group);
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
    case cpld4:
    case cpld5:
          sysfs_remove_group(&client->dev.kobj, &cpld345_group);
        break;
    }

    cpld_remove_client(client);
    return 0;
}

MODULE_DEVICE_TABLE(i2c, cpld_id);

static struct i2c_driver cpld_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name = "x86_64_ufispace_s9600_64x_cpld",
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
MODULE_DESCRIPTION("x86_64_ufispace_s9600_64x_cpld driver");
MODULE_LICENSE("GPL");

module_init(cpld_init);
module_exit(cpld_exit);

