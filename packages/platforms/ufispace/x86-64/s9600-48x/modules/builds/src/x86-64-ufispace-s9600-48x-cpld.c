/*
 * A i2c cpld driver for the ufispace_s9600_48x
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
#include "x86-64-ufispace-s9600-48x-cpld.h"

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
    CPLD_BOARD_ID_0,
    CPLD_BOARD_ID_1,
    CPLD_VERSION,
    CPLD_ID,
    CPLD_INTR_MASK_0,
    CPLD_INTR_MASK_1,
    CPLD_INTR_MASK_2,
	CPLD_INTR_MASK_3,
    CPLD_QSFP_STATUS_0,
    CPLD_QSFP_STATUS_1,
    CPLD_QSFP_STATUS_2,
    CPLD_QSFP_STATUS_3,
    CPLD_QSFP_STATUS_4,
    CPLD_QSFP_STATUS_5,
    CPLD_QSFP_STATUS_6,
    CPLD_QSFP_STATUS_7,
    CPLD_QSFP_STATUS_8,
    CPLD_QSFP_STATUS_9,
    CPLD_QSFP_STATUS_10,
    CPLD_QSFP_STATUS_11,
    CPLD_QSFP_STATUS_12,
    CPLD_GBOX_INTR_0,
    CPLD_GBOX_INTR_1,
    CPLD_SFP_STATUS,
    CPLD_QSFP_CONFIG_0,
    CPLD_QSFP_CONFIG_1,
    CPLD_QSFP_CONFIG_2,
    CPLD_QSFP_CONFIG_3,
    CPLD_QSFP_CONFIG_4,
    CPLD_QSFP_CONFIG_5,
    CPLD_QSFP_CONFIG_6,
    CPLD_QSFP_CONFIG_7,
    CPLD_QSFP_CONFIG_8,
    CPLD_QSFP_CONFIG_9,
    CPLD_QSFP_CONFIG_10,
    CPLD_QSFP_CONFIG_11,
    CPLD_QSFP_CONFIG_12,
    CPLD_SFP_CONFIG,
    CPLD_INTR_0,
    CPLD_INTR_1,
    CPLD_PSU_STATUS,
    CPLD_MUX_CTRL,
    CPLD_RESET_0,
    CPLD_RESET_1,
    CPLD_RESET_2,
    CPLD_RESET_3,
    CPLD_SYSTEM_LED_0,
    CPLD_SYSTEM_LED_1,
    CPLD_QSFP_PORT_EVT,
    CPLD_QSFP_PLUG_EVT_0,
    CPLD_QSFP_PLUG_EVT_1,
    //CPLD 2
    CPLD_QSFP_MUX_RESET,
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
    { "s9600_48x_cpld1",  cpld1 },
    { "s9600_48x_cpld2",  cpld2 },
    { "s9600_48x_cpld3",  cpld3 },
    { "s9600_48x_cpld4",  cpld4 },
    {}
};

/* Addresses scanned for cpld */
static const unsigned short cpld_i2c_addr[] = { 0x30, 0x31, 0x32, 0x33, I2C_CLIENT_END };

/* define all support register access of cpld in attribute */

static SENSOR_DEVICE_ATTR(cpld_board_id_0,  S_IRUGO, read_cpld_callback, NULL, CPLD_BOARD_ID_0);
static SENSOR_DEVICE_ATTR(cpld_board_id_1,  S_IRUGO, read_cpld_callback, NULL, CPLD_BOARD_ID_1);
static SENSOR_DEVICE_ATTR(cpld_version,     S_IRUGO, read_cpld_callback, NULL, CPLD_VERSION);
static SENSOR_DEVICE_ATTR(cpld_id,          S_IRUGO, read_cpld_callback, NULL, CPLD_ID);
static SENSOR_DEVICE_ATTR(cpld_intr_mask_0, S_IWUSR | S_IRUGO, read_cpld_callback, write_cpld_callback, CPLD_INTR_MASK_0);
static SENSOR_DEVICE_ATTR(cpld_intr_mask_1, S_IWUSR | S_IRUGO, read_cpld_callback, write_cpld_callback, CPLD_INTR_MASK_1);
static SENSOR_DEVICE_ATTR(cpld_intr_mask_2, S_IWUSR | S_IRUGO, read_cpld_callback, write_cpld_callback, CPLD_INTR_MASK_2);
static SENSOR_DEVICE_ATTR(cpld_intr_mask_3, S_IWUSR | S_IRUGO, read_cpld_callback, write_cpld_callback, CPLD_INTR_MASK_3);
static SENSOR_DEVICE_ATTR(cpld_qsfp_status_0, S_IRUGO, read_cpld_callback, NULL, CPLD_QSFP_STATUS_0);
static SENSOR_DEVICE_ATTR(cpld_qsfp_status_1, S_IRUGO, read_cpld_callback, NULL, CPLD_QSFP_STATUS_1);
static SENSOR_DEVICE_ATTR(cpld_qsfp_status_2, S_IRUGO, read_cpld_callback, NULL, CPLD_QSFP_STATUS_2);
static SENSOR_DEVICE_ATTR(cpld_qsfp_status_3, S_IRUGO, read_cpld_callback, NULL, CPLD_QSFP_STATUS_3);
static SENSOR_DEVICE_ATTR(cpld_qsfp_status_4, S_IRUGO, read_cpld_callback, NULL, CPLD_QSFP_STATUS_4);
static SENSOR_DEVICE_ATTR(cpld_qsfp_status_5, S_IRUGO, read_cpld_callback, NULL, CPLD_QSFP_STATUS_5);
static SENSOR_DEVICE_ATTR(cpld_qsfp_status_6, S_IRUGO, read_cpld_callback, NULL, CPLD_QSFP_STATUS_6);
static SENSOR_DEVICE_ATTR(cpld_qsfp_status_7, S_IRUGO, read_cpld_callback, NULL, CPLD_QSFP_STATUS_7);
static SENSOR_DEVICE_ATTR(cpld_qsfp_status_8, S_IRUGO, read_cpld_callback, NULL, CPLD_QSFP_STATUS_8);
static SENSOR_DEVICE_ATTR(cpld_qsfp_status_9, S_IRUGO, read_cpld_callback, NULL, CPLD_QSFP_STATUS_9);
static SENSOR_DEVICE_ATTR(cpld_qsfp_status_10, S_IRUGO,read_cpld_callback, NULL, CPLD_QSFP_STATUS_10);
static SENSOR_DEVICE_ATTR(cpld_qsfp_status_11, S_IRUGO,read_cpld_callback, NULL, CPLD_QSFP_STATUS_11);
static SENSOR_DEVICE_ATTR(cpld_qsfp_status_12, S_IRUGO,read_cpld_callback, NULL, CPLD_QSFP_STATUS_12);
static SENSOR_DEVICE_ATTR(cpld_gbox_intr_0, S_IRUGO, read_cpld_callback, NULL, CPLD_GBOX_INTR_0);
static SENSOR_DEVICE_ATTR(cpld_gbox_intr_1, S_IRUGO, read_cpld_callback, NULL, CPLD_GBOX_INTR_1);
static SENSOR_DEVICE_ATTR(cpld_sfp_status, S_IRUGO, read_cpld_callback, NULL, CPLD_SFP_STATUS);
static SENSOR_DEVICE_ATTR(cpld_qsfp_config_0, S_IWUSR | S_IRUGO, read_cpld_callback, write_cpld_callback, CPLD_QSFP_CONFIG_0);
static SENSOR_DEVICE_ATTR(cpld_qsfp_config_1, S_IWUSR | S_IRUGO, read_cpld_callback, write_cpld_callback, CPLD_QSFP_CONFIG_1);
static SENSOR_DEVICE_ATTR(cpld_qsfp_config_2, S_IWUSR | S_IRUGO, read_cpld_callback, write_cpld_callback, CPLD_QSFP_CONFIG_2);
static SENSOR_DEVICE_ATTR(cpld_qsfp_config_3, S_IWUSR | S_IRUGO, read_cpld_callback, write_cpld_callback, CPLD_QSFP_CONFIG_3);
static SENSOR_DEVICE_ATTR(cpld_qsfp_config_4, S_IWUSR | S_IRUGO, read_cpld_callback, write_cpld_callback, CPLD_QSFP_CONFIG_4);
static SENSOR_DEVICE_ATTR(cpld_qsfp_config_5, S_IWUSR | S_IRUGO, read_cpld_callback, write_cpld_callback, CPLD_QSFP_CONFIG_5);
static SENSOR_DEVICE_ATTR(cpld_qsfp_config_6, S_IWUSR | S_IRUGO, read_cpld_callback, write_cpld_callback, CPLD_QSFP_CONFIG_6);
static SENSOR_DEVICE_ATTR(cpld_qsfp_config_7, S_IWUSR | S_IRUGO, read_cpld_callback, write_cpld_callback, CPLD_QSFP_CONFIG_7);
static SENSOR_DEVICE_ATTR(cpld_qsfp_config_8, S_IWUSR | S_IRUGO, read_cpld_callback, write_cpld_callback, CPLD_QSFP_CONFIG_8);
static SENSOR_DEVICE_ATTR(cpld_qsfp_config_9, S_IWUSR | S_IRUGO, read_cpld_callback, write_cpld_callback, CPLD_QSFP_CONFIG_9);
static SENSOR_DEVICE_ATTR(cpld_qsfp_config_10, S_IWUSR | S_IRUGO,read_cpld_callback, write_cpld_callback, CPLD_QSFP_CONFIG_10);
static SENSOR_DEVICE_ATTR(cpld_qsfp_config_11, S_IWUSR | S_IRUGO,read_cpld_callback, write_cpld_callback, CPLD_QSFP_CONFIG_11);
static SENSOR_DEVICE_ATTR(cpld_qsfp_config_12, S_IWUSR | S_IRUGO,read_cpld_callback, write_cpld_callback, CPLD_QSFP_CONFIG_12);
static SENSOR_DEVICE_ATTR(cpld_sfp_config, S_IWUSR | S_IRUGO, read_cpld_callback, write_cpld_callback, CPLD_SFP_CONFIG);
static SENSOR_DEVICE_ATTR(cpld_intr_0, S_IWUSR | S_IRUGO, read_cpld_callback, NULL, CPLD_INTR_0);
static SENSOR_DEVICE_ATTR(cpld_intr_1, S_IWUSR | S_IRUGO, read_cpld_callback, NULL, CPLD_INTR_1);
static SENSOR_DEVICE_ATTR(cpld_psu_status, S_IRUGO, read_cpld_callback, NULL, CPLD_PSU_STATUS);
static SENSOR_DEVICE_ATTR(cpld_mux_ctrl, S_IWUSR | S_IRUGO, read_cpld_callback, write_cpld_callback, CPLD_MUX_CTRL);
static SENSOR_DEVICE_ATTR(cpld_reset_0, S_IWUSR | S_IRUGO, read_cpld_callback, write_cpld_callback, CPLD_RESET_0);
static SENSOR_DEVICE_ATTR(cpld_reset_1, S_IWUSR | S_IRUGO, read_cpld_callback, write_cpld_callback, CPLD_RESET_1);
static SENSOR_DEVICE_ATTR(cpld_reset_2, S_IWUSR | S_IRUGO, read_cpld_callback, write_cpld_callback, CPLD_RESET_2);
static SENSOR_DEVICE_ATTR(cpld_reset_3, S_IWUSR | S_IRUGO, read_cpld_callback, write_cpld_callback, CPLD_RESET_3);
static SENSOR_DEVICE_ATTR(cpld_system_led_0, S_IWUSR | S_IRUGO, read_cpld_callback, write_cpld_callback, CPLD_SYSTEM_LED_0);
static SENSOR_DEVICE_ATTR(cpld_system_led_1, S_IWUSR | S_IRUGO, read_cpld_callback, write_cpld_callback, CPLD_SYSTEM_LED_1);
static SENSOR_DEVICE_ATTR(cpld_qsfp_port_evt, S_IRUGO, read_cpld_callback, NULL, CPLD_QSFP_PORT_EVT);
static SENSOR_DEVICE_ATTR(cpld_qsfp_plug_evt_0, S_IRUGO, read_cpld_callback, NULL, CPLD_QSFP_PLUG_EVT_0);
static SENSOR_DEVICE_ATTR(cpld_qsfp_plug_evt_1, S_IRUGO, read_cpld_callback, NULL, CPLD_QSFP_PLUG_EVT_1);

//CPLD 2
static SENSOR_DEVICE_ATTR(cpld_qsfp_mux_reset, S_IWUSR | S_IRUGO, read_cpld_callback, write_cpld_callback, CPLD_QSFP_MUX_RESET);

/* define support attributes of cpldx */

/* cpld 1 */
static struct attribute *cpld1_attributes[] = {
    &sensor_dev_attr_cpld_board_id_0.dev_attr.attr,
    &sensor_dev_attr_cpld_board_id_1.dev_attr.attr,
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_cpld_id.dev_attr.attr,
    &sensor_dev_attr_cpld_intr_mask_0.dev_attr.attr,
    &sensor_dev_attr_cpld_intr_mask_1.dev_attr.attr,
    &sensor_dev_attr_cpld_intr_mask_2.dev_attr.attr,
	&sensor_dev_attr_cpld_intr_mask_3.dev_attr.attr,
    //&sensor_dev_attr_cpld_intr_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_4.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_5.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_6.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_7.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_8.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_9.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_10.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_11.dev_attr.attr,
    &sensor_dev_attr_cpld_gbox_intr_0.dev_attr.attr,
    &sensor_dev_attr_cpld_gbox_intr_1.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_status.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_4.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_5.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_6.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_7.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_8.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_9.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_10.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_11.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_config.dev_attr.attr,
	&sensor_dev_attr_cpld_intr_0.dev_attr.attr,
    &sensor_dev_attr_cpld_intr_1.dev_attr.attr,
	&sensor_dev_attr_cpld_psu_status.dev_attr.attr,
	&sensor_dev_attr_cpld_mux_ctrl.dev_attr.attr,
    &sensor_dev_attr_cpld_reset_0.dev_attr.attr,
    &sensor_dev_attr_cpld_reset_1.dev_attr.attr,
    &sensor_dev_attr_cpld_reset_2.dev_attr.attr,
    &sensor_dev_attr_cpld_reset_3.dev_attr.attr,
    &sensor_dev_attr_cpld_system_led_0.dev_attr.attr,
    &sensor_dev_attr_cpld_system_led_1.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfp_port_evt.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfp_plug_evt_0.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfp_plug_evt_1.dev_attr.attr,
    NULL
};

/* cpld 2 */
static struct attribute *cpld2_attributes[] = {
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_cpld_id.dev_attr.attr,
    &sensor_dev_attr_cpld_intr_mask_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_4.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_5.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_6.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_7.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_8.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_9.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_10.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_11.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfp_status_12.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_4.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_5.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_6.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_7.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_8.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_9.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_10.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_11.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfp_config_12.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfp_mux_reset.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfp_port_evt.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfp_plug_evt_0.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfp_plug_evt_1.dev_attr.attr,
    NULL
};

/* cpld 3/4 */
static struct attribute *cpld34_attributes[] = {
&sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_cpld_id.dev_attr.attr,
    &sensor_dev_attr_cpld_intr_mask_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_4.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_5.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_6.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_7.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_8.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_9.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_10.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_status_11.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfp_status_12.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_4.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_5.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_6.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_7.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_8.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_9.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_10.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfp_config_11.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfp_config_12.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfp_port_evt.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfp_plug_evt_0.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfp_plug_evt_1.dev_attr.attr,
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

/* cpld 3/4 attributes group */
static const struct attribute_group cpld34_group = {
    .attrs = cpld34_attributes,
};

/* get cpld register value */
static ssize_t read_cpld_callback(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg = 0;

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
        case CPLD_INTR_MASK_0:
        case CPLD_INTR_MASK_1:
        case CPLD_INTR_MASK_2:
        case CPLD_INTR_MASK_3:
            reg = CPLD_INTR_MASK_BASE_REG +
                 (attr->index - CPLD_INTR_MASK_0);
            break;
        case CPLD_QSFP_STATUS_0:
        case CPLD_QSFP_STATUS_1:
        case CPLD_QSFP_STATUS_2:
        case CPLD_QSFP_STATUS_3:
        case CPLD_QSFP_STATUS_4:
        case CPLD_QSFP_STATUS_5:
        case CPLD_QSFP_STATUS_6:
        case CPLD_QSFP_STATUS_7:
        case CPLD_QSFP_STATUS_8:
        case CPLD_QSFP_STATUS_9:
        case CPLD_QSFP_STATUS_10:
        case CPLD_QSFP_STATUS_11:
        case CPLD_QSFP_STATUS_12:
            reg = CPLD_QSFP_STATUS_BASE_REG +
                 (attr->index - CPLD_QSFP_STATUS_0);
            break;
        case CPLD_GBOX_INTR_0:
        case CPLD_GBOX_INTR_1:
            reg = CPLD_GBOX_INTR_BASE_REG +
                  (attr->index - CPLD_GBOX_INTR_0);
            break;
        case CPLD_SFP_STATUS:
            reg = CPLD_SFP_STATUS_REG;
            break;
        case CPLD_QSFP_CONFIG_0:
        case CPLD_QSFP_CONFIG_1:
        case CPLD_QSFP_CONFIG_2:
        case CPLD_QSFP_CONFIG_3:
        case CPLD_QSFP_CONFIG_4:
        case CPLD_QSFP_CONFIG_5:
        case CPLD_QSFP_CONFIG_6:
        case CPLD_QSFP_CONFIG_7:
        case CPLD_QSFP_CONFIG_8:
        case CPLD_QSFP_CONFIG_9:
        case CPLD_QSFP_CONFIG_10:
        case CPLD_QSFP_CONFIG_11:
        case CPLD_QSFP_CONFIG_12:
            reg = CPLD_QSFP_CONFIG_BASE_REG +
                 (attr->index - CPLD_QSFP_CONFIG_0);
            break;
        case CPLD_SFP_CONFIG:
            reg = CPLD_SFP_CONFIG_REG;
            break;
        case CPLD_INTR_0:
            reg = CPLD_INTR_0_REG;
            break;
        case CPLD_INTR_1:
            reg = CPLD_INTR_1_REG;
            break;
        case CPLD_PSU_STATUS:
            reg = CPLD_PSU_STATUS_REG;
            break;
        case CPLD_MUX_CTRL:
            reg = CPLD_MUX_CTRL_REG;
            break;
        case CPLD_RESET_0:
            reg = CPLD_RESET_0_REG;
            break;
        case CPLD_RESET_1:
            reg = CPLD_RESET_1_REG;
            break;
        case CPLD_RESET_2:
            reg = CPLD_RESET_2_REG;
            break;
        case CPLD_RESET_3:
            reg = CPLD_RESET_3_REG;
            break;
        case CPLD_SYSTEM_LED_0:
        case CPLD_SYSTEM_LED_1:
            reg = CPLD_SYSTEM_LED_BASE_REG +
                  (attr->index - CPLD_SYSTEM_LED_0);
            break;
        case CPLD_QSFP_PORT_EVT:
            reg = CPLD_QSFP_PORT_EVT_REG;
            break;
        case CPLD_QSFP_PLUG_EVT_0:
        case CPLD_QSFP_PLUG_EVT_1:
            reg = CPLD_QSFP_PLUG_EVT_BASE_REG +
                  (attr->index - CPLD_QSFP_PLUG_EVT_0);
            break;
        case CPLD_QSFP_MUX_RESET:
            reg = CPLD_QSFP_MUX_RESET_REG;
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
        case CPLD_INTR_MASK_0:
        case CPLD_INTR_MASK_1:
        case CPLD_INTR_MASK_2:
        case CPLD_INTR_MASK_3:
            reg = CPLD_INTR_MASK_BASE_REG +
                 (attr->index - CPLD_INTR_MASK_0);
            break;
        case CPLD_QSFP_CONFIG_0:
        case CPLD_QSFP_CONFIG_1:
        case CPLD_QSFP_CONFIG_2:
        case CPLD_QSFP_CONFIG_3:
        case CPLD_QSFP_CONFIG_4:
        case CPLD_QSFP_CONFIG_5:
        case CPLD_QSFP_CONFIG_6:
        case CPLD_QSFP_CONFIG_7:
        case CPLD_QSFP_CONFIG_8:
        case CPLD_QSFP_CONFIG_9:
        case CPLD_QSFP_CONFIG_10:
        case CPLD_QSFP_CONFIG_11:
        case CPLD_QSFP_CONFIG_12:
            reg = CPLD_QSFP_CONFIG_BASE_REG +
                 (attr->index - CPLD_QSFP_CONFIG_0);
            break;
        case CPLD_SFP_CONFIG:
            reg = CPLD_SFP_CONFIG_REG;
            break;
        case CPLD_MUX_CTRL:
            reg = CPLD_MUX_CTRL_REG;
            break;
        case CPLD_RESET_0:
            reg = CPLD_RESET_0_REG;
            break;
        case CPLD_RESET_1:
            reg = CPLD_RESET_1_REG;
            break;
        case CPLD_RESET_2:
            reg = CPLD_RESET_2_REG;
            break;
        case CPLD_RESET_3:
            reg = CPLD_RESET_3_REG;
            break;
        case CPLD_SYSTEM_LED_0:
        case CPLD_SYSTEM_LED_1:
            reg = CPLD_SYSTEM_LED_BASE_REG +
                  (attr->index - CPLD_SYSTEM_LED_0);
            break;
        case CPLD_QSFP_MUX_RESET:
            reg = CPLD_QSFP_MUX_RESET_REG;
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

    if (unlikely(ret < 0)) {
        dev_err(dev, "write_cpld_reg() error, return=%d\n", ret);
        return ret;
    }

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
          status = sysfs_create_group(&client->dev.kobj,
                    &cpld34_group);
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
          sysfs_remove_group(&client->dev.kobj, &cpld34_group);
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
    case cpld4:
          sysfs_remove_group(&client->dev.kobj, &cpld34_group);
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
        .name = "x86_64_ufispace_s9600_48x_cpld",
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
MODULE_DESCRIPTION("x86_64_ufispace_s9600_48x_cpld driver");
MODULE_LICENSE("GPL");

module_init(cpld_init);
module_exit(cpld_exit);
