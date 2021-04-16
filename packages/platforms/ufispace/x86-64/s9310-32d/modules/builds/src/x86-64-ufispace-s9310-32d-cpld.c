/*
 * A i2c cpld driver for the ufispace_s9310
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
#include "x86-64-ufispace-s9310-32d-cpld.h"

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
enum s9310_cpld_sysfs_attributes {
    /* CPLD1 */
    CPLD_ACCESS_REG,
    CPLD_REGISTER_VAL,
    CPLD_BOARD_TYPE,
    CPLD_HW_REV,
    CPLD_VERSION,
    CPLD_ID,
    CPLD_INTERRUPT,
    CPLD_INTERRUPT_MASK,
    CPLD_INTERRUPT_EVENT,
    CPLD_RESET_CTRL1,
    CPLD_RESET_CTRL2,
    CPLD_RESET_CTRL3,
    CPLD_BMC_STATUS,
    CPLD_PSU_STATUS,
    CPLD_MISC_STATUS,
    CPLD_MISC_CTRL,
    CPLD_TIMING_CTRL,
    CPLD_MUX_CTRL,
    CPLD_BMC_WATCHDOG,
    CPLD_BEACON_LED_CTRL,
    CPLD_LED_CTRL1,
    CPLD_LED_CTRL2,
    /* CPLD2 */
    CPLD_PORT_INT_EVENT,
    CPLD_PORT_INT_MASK,
    CPLD_QSFPDD_MOD_INT_G0,
    CPLD_QSFPDD_MOD_INT_G1,
    CPLD_QSFPDD_MOD_INT_G2,
    CPLD_QSFPDD_MOD_INT_G3,
    CPLD_QSFPDD_PRES_G0,
    CPLD_QSFPDD_PRES_G1,
    CPLD_QSFPDD_PRES_G2,
    CPLD_QSFPDD_PRES_G3,
    CPLD_QSFPDD_FUSE_INT_G0,
    CPLD_QSFPDD_FUSE_INT_G1,
    CPLD_QSFPDD_FUSE_INT_G2,
    CPLD_QSFPDD_FUSE_INT_G3,
    CPLD_QSFPDD_PLUG_EVENT_G0,
    CPLD_QSFPDD_PLUG_EVENT_G1,
    CPLD_QSFPDD_PLUG_EVENT_G2,
    CPLD_QSFPDD_PLUG_EVENT_G3,
    CPLD_QSFPDD_RESET_CTRL_G0,
    CPLD_QSFPDD_RESET_CTRL_G1,
    CPLD_QSFPDD_RESET_CTRL_G2,
    CPLD_QSFPDD_RESET_CTRL_G3,
    CPLD_QSFPDD_LP_MODE_G0,
    CPLD_QSFPDD_LP_MODE_G1,
    CPLD_QSFPDD_LP_MODE_G2,
    CPLD_QSFPDD_LP_MODE_G3,
    CPLD_SFP_TXFAULT,
    CPLD_SFP_RXLOS,
    CPLD_SFP_ABS,
    CPLD_SFP_PLUG_EVENT,
    CPLD_SFP_TX_DIS,
    CPLD_SFP_RS,
    CPLD_SFP_TS,
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
static ssize_t read_cpld_callback(struct device *dev,
        struct device_attribute *da, char *buf);
static ssize_t write_cpld_callback(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_cpld_reg(struct device *dev, char *buf, u8 reg);
static ssize_t write_cpld_reg(struct device *dev, const char *buf, \
                              size_t count, u8 reg);

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
static const struct i2c_device_id s9310_cpld_id[] = {
    { "s9310_32d_cpld1",  cpld1 },
    { "s9310_32d_cpld2",  cpld2 },
    { "s9310_32d_cpld3",  cpld3 },
    {}
};

/* Addresses scanned for s9310_cpld */
static const unsigned short cpld_i2c_addr[] = { 0x30, 0x31, 0x32, I2C_CLIENT_END };

/* define all support register access of cpld in attribute */
/* CPLD1 */
static SENSOR_DEVICE_ATTR(cpld_access_register, S_IWUSR | S_IRUGO, \
        read_access_register, write_access_register, CPLD_ACCESS_REG);
static SENSOR_DEVICE_ATTR(cpld_register_value, S_IWUSR | S_IRUGO, \
        read_register_value, write_register_value, CPLD_REGISTER_VAL);
static SENSOR_DEVICE_ATTR(cpld_board_type, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_BOARD_TYPE);
static SENSOR_DEVICE_ATTR(cpld_hw_rev, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_HW_REV);
static SENSOR_DEVICE_ATTR(cpld_version, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_VERSION);
static SENSOR_DEVICE_ATTR(cpld_id, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_ID);
static SENSOR_DEVICE_ATTR(cpld_interrupt, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_INTERRUPT);
static SENSOR_DEVICE_ATTR(cpld_interrupt_mask, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_INTERRUPT_MASK);
static SENSOR_DEVICE_ATTR(cpld_interrupt_event, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_INTERRUPT_EVENT);
static SENSOR_DEVICE_ATTR(cpld_reset_ctrl1, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_RESET_CTRL1);
static SENSOR_DEVICE_ATTR(cpld_reset_ctrl2, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_RESET_CTRL2);
static SENSOR_DEVICE_ATTR(cpld_reset_ctrl3, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_RESET_CTRL3);
static SENSOR_DEVICE_ATTR(cpld_bmc_status, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_BMC_STATUS);
static SENSOR_DEVICE_ATTR(cpld_psu_status, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_PSU_STATUS);
static SENSOR_DEVICE_ATTR(cpld_misc_status, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_MISC_STATUS);
static SENSOR_DEVICE_ATTR(cpld_misc_ctrl, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_MISC_CTRL);
static SENSOR_DEVICE_ATTR(cpld_timing_ctrl, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_TIMING_CTRL);
static SENSOR_DEVICE_ATTR(cpld_mux_ctrl, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_MUX_CTRL);
static SENSOR_DEVICE_ATTR(cpld_bmc_watchdog, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_BMC_WATCHDOG);
static SENSOR_DEVICE_ATTR(cpld_beacon_led_ctrl, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_BEACON_LED_CTRL);
static SENSOR_DEVICE_ATTR(cpld_led_ctrl1, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_LED_CTRL1);
static SENSOR_DEVICE_ATTR(cpld_led_ctrl2, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_LED_CTRL2);
/* CPLD2 */
static SENSOR_DEVICE_ATTR(cpld_port_int_event, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_PORT_INT_EVENT);
static SENSOR_DEVICE_ATTR(cpld_port_int_mask, S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_PORT_INT_MASK);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_mod_int_g0, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_MOD_INT_G0);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_mod_int_g1, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_MOD_INT_G1);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_mod_int_g2, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_MOD_INT_G2);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_mod_int_g3, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_MOD_INT_G3);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_pres_g0, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_PRES_G0);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_pres_g1, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_PRES_G1);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_pres_g2, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_PRES_G2);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_pres_g3, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_PRES_G3);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fuse_int_g0, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_FUSE_INT_G0);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fuse_int_g1, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_FUSE_INT_G1);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fuse_int_g2, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_FUSE_INT_G2);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fuse_int_g3, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_FUSE_INT_G3);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_plug_event_g0, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_PLUG_EVENT_G0);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_plug_event_g1, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_PLUG_EVENT_G1);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_plug_event_g2, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_PLUG_EVENT_G2);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_plug_event_g3, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_QSFPDD_PLUG_EVENT_G3);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_reset_ctrl_g0, \
	 S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, \
        CPLD_QSFPDD_RESET_CTRL_G0);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_reset_ctrl_g1, \
	 S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, \
        CPLD_QSFPDD_RESET_CTRL_G1);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_reset_ctrl_g2, \
	 S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, \
        CPLD_QSFPDD_RESET_CTRL_G2);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_reset_ctrl_g3, \
	 S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, \
        CPLD_QSFPDD_RESET_CTRL_G3);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_lp_mode_g0, \
	 S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, \
        CPLD_QSFPDD_LP_MODE_G0);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_lp_mode_g1, \
	 S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, \
        CPLD_QSFPDD_LP_MODE_G1);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_lp_mode_g2, \
	 S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, \
        CPLD_QSFPDD_LP_MODE_G2);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_lp_mode_g3, \
	 S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, \
        CPLD_QSFPDD_LP_MODE_G3);
static SENSOR_DEVICE_ATTR(cpld_sfp_txfault, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_SFP_TXFAULT);
static SENSOR_DEVICE_ATTR(cpld_sfp_rxlos, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_SFP_RXLOS);
static SENSOR_DEVICE_ATTR(cpld_sfp_abs, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_SFP_ABS);
static SENSOR_DEVICE_ATTR(cpld_sfp_plug_event, S_IRUGO, \
        read_cpld_callback, NULL, CPLD_SFP_PLUG_EVENT);
static SENSOR_DEVICE_ATTR(cpld_sfp_tx_dis, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_SFP_TX_DIS);
static SENSOR_DEVICE_ATTR(cpld_sfp_rs, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_SFP_RS);
static SENSOR_DEVICE_ATTR(cpld_sfp_ts, S_IWUSR | S_IRUGO, \
        read_cpld_callback, write_cpld_callback, CPLD_SFP_TS);

/* define support attributes of cpldx , total 3 */
/* cpld 1 */
static struct attribute *s9310_cpld1_attributes[] = {
    &sensor_dev_attr_cpld_access_register.dev_attr.attr,
    &sensor_dev_attr_cpld_register_value.dev_attr.attr,
    &sensor_dev_attr_cpld_board_type.dev_attr.attr,
    &sensor_dev_attr_cpld_hw_rev.dev_attr.attr,
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_cpld_id.dev_attr.attr,
    &sensor_dev_attr_cpld_interrupt.dev_attr.attr,
    &sensor_dev_attr_cpld_interrupt_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_interrupt_event.dev_attr.attr,
    &sensor_dev_attr_cpld_reset_ctrl1.dev_attr.attr,
    &sensor_dev_attr_cpld_reset_ctrl2.dev_attr.attr,
    &sensor_dev_attr_cpld_reset_ctrl3.dev_attr.attr,
    &sensor_dev_attr_cpld_bmc_status.dev_attr.attr,
    &sensor_dev_attr_cpld_psu_status.dev_attr.attr,
    &sensor_dev_attr_cpld_misc_status.dev_attr.attr,
    &sensor_dev_attr_cpld_misc_ctrl.dev_attr.attr,
    &sensor_dev_attr_cpld_timing_ctrl.dev_attr.attr,
    &sensor_dev_attr_cpld_mux_ctrl.dev_attr.attr,
    &sensor_dev_attr_cpld_bmc_watchdog.dev_attr.attr,
    &sensor_dev_attr_cpld_beacon_led_ctrl.dev_attr.attr,
    &sensor_dev_attr_cpld_led_ctrl1.dev_attr.attr,
    &sensor_dev_attr_cpld_led_ctrl2.dev_attr.attr,
    NULL
};

/* cpld 2 */
static struct attribute *s9310_cpld2_attributes[] = {
    &sensor_dev_attr_cpld_access_register.dev_attr.attr,
    &sensor_dev_attr_cpld_register_value.dev_attr.attr,
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_cpld_id.dev_attr.attr,
    &sensor_dev_attr_cpld_port_int_event.dev_attr.attr,
    &sensor_dev_attr_cpld_port_int_mask.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_mod_int_g0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_mod_int_g1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_mod_int_g2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_mod_int_g3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_pres_g0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_pres_g1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_pres_g2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_pres_g3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fuse_int_g0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fuse_int_g1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fuse_int_g2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fuse_int_g3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_plug_event_g0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_plug_event_g1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_plug_event_g2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_plug_event_g3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_reset_ctrl_g0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_reset_ctrl_g1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_reset_ctrl_g2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_reset_ctrl_g3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_lp_mode_g0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_lp_mode_g1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_lp_mode_g2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_lp_mode_g3.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_txfault.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_rxlos.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_abs.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_plug_event.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_tx_dis.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_rs.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_ts.dev_attr.attr,
    NULL
};

/* cpld 3 */
static struct attribute *s9310_cpld3_attributes[] = {
    &sensor_dev_attr_cpld_access_register.dev_attr.attr,
    &sensor_dev_attr_cpld_register_value.dev_attr.attr,
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_cpld_id.dev_attr.attr,
    NULL
};

/* cpld 1 attributes group */
static const struct attribute_group s9310_cpld1_group = {
    .attrs = s9310_cpld1_attributes,
};
/* cpld 2 attributes group */
static const struct attribute_group s9310_cpld2_group = {
    .attrs = s9310_cpld2_attributes,
};
/* cpld 3 attributes group */
static const struct attribute_group s9310_cpld3_group = {
    .attrs = s9310_cpld3_attributes,
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

/* handle read for attributes */
static ssize_t read_cpld_callback(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg = 0;

    switch (attr->index) {
        case CPLD_BOARD_TYPE:
             reg = CPLD_BOARD_TYPE_REG;
             break;
        case CPLD_HW_REV:
             reg = CPLD_HW_REV_REG;
             break;
        case CPLD_VERSION:
             reg = CPLD_VERSION_REG;
             break;
        case CPLD_ID:
             reg = CPLD_ID_REG;
             break;
        case CPLD_INTERRUPT:
             reg = CPLD_INTERRUPT_REG;
             break;
        case CPLD_INTERRUPT_MASK:
             reg = CPLD_INTERRUPT_MASK_REG;
             break;
        case CPLD_INTERRUPT_EVENT:
             reg = CPLD_INTERRUPT_EVENT_REG;
             break;
        case CPLD_RESET_CTRL1:
             reg = CPLD_RESET_CTRL1_REG;
             break;
        case CPLD_RESET_CTRL2:
             reg = CPLD_RESET_CTRL2_REG;
             break;
        case CPLD_RESET_CTRL3:
             reg = CPLD_RESET_CTRL3_REG;
             break;
        case CPLD_BMC_STATUS:
             reg = CPLD_BMC_STATUS_REG;
             break;
        case CPLD_PSU_STATUS:
             reg = CPLD_PSU_STATUS_REG;
             break;
        case CPLD_MISC_STATUS:
             reg = CPLD_MISC_STATUS_REG;
             break;
        case CPLD_MISC_CTRL:
             reg = CPLD_MISC_CTRL_REG;
             break;
        case CPLD_TIMING_CTRL:
             reg = CPLD_TIMING_CTRL_REG;
             break;
        case CPLD_MUX_CTRL:
             reg = CPLD_MUX_CTRL_REG;
             break;
        case CPLD_BMC_WATCHDOG:
             reg = CPLD_BMC_WATCHDOG_REG;
             break;
        case CPLD_BEACON_LED_CTRL:
             reg = CPLD_BEACON_LED_CTRL_REG;
             break;
        case CPLD_LED_CTRL1:
             reg = CPLD_LED_CTRL1_REG;
             break;
        case CPLD_LED_CTRL2:
             reg = CPLD_LED_CTRL2_REG;
             break;
        case CPLD_PORT_INT_EVENT:
             reg = CPLD_PORT_INT_EVENT_REG;
             break;
        case CPLD_PORT_INT_MASK:
             reg = CPLD_PORT_INT_MASK_REG;
             break;
        case CPLD_QSFPDD_MOD_INT_G0:
             reg = CPLD_QSFPDD_MOD_INT_G0_REG;
             break;
        case CPLD_QSFPDD_MOD_INT_G1:
             reg = CPLD_QSFPDD_MOD_INT_G1_REG;
             break;
        case CPLD_QSFPDD_MOD_INT_G2:
             reg = CPLD_QSFPDD_MOD_INT_G2_REG;
             break;
        case CPLD_QSFPDD_MOD_INT_G3:
             reg = CPLD_QSFPDD_MOD_INT_G3_REG;
             break;
        case CPLD_QSFPDD_PRES_G0:
             reg = CPLD_QSFPDD_PRES_G0_REG;
             break;
        case CPLD_QSFPDD_PRES_G1:
             reg = CPLD_QSFPDD_PRES_G1_REG;
             break;
        case CPLD_QSFPDD_PRES_G2:
             reg = CPLD_QSFPDD_PRES_G2_REG;
             break;
        case CPLD_QSFPDD_PRES_G3:
             reg = CPLD_QSFPDD_PRES_G3_REG;
             break;
        case CPLD_QSFPDD_FUSE_INT_G0:
             reg = CPLD_QSFPDD_FUSE_INT_G0_REG;
             break;
        case CPLD_QSFPDD_FUSE_INT_G1:
             reg = CPLD_QSFPDD_FUSE_INT_G1_REG;
             break;
        case CPLD_QSFPDD_FUSE_INT_G2:
             reg = CPLD_QSFPDD_FUSE_INT_G2_REG;
             break;
        case CPLD_QSFPDD_FUSE_INT_G3:
             reg = CPLD_QSFPDD_FUSE_INT_G3_REG;
             break;
        case CPLD_QSFPDD_PLUG_EVENT_G0:
             reg = CPLD_QSFPDD_PLUG_EVENT_G0_REG;
             break;
        case CPLD_QSFPDD_PLUG_EVENT_G1:
             reg = CPLD_QSFPDD_PLUG_EVENT_G1_REG;
             break;
        case CPLD_QSFPDD_PLUG_EVENT_G2:
             reg = CPLD_QSFPDD_PLUG_EVENT_G2_REG;
             break;
        case CPLD_QSFPDD_PLUG_EVENT_G3:
             reg = CPLD_QSFPDD_PLUG_EVENT_G3_REG;
             break;
        case CPLD_QSFPDD_RESET_CTRL_G0:
             reg = CPLD_QSFPDD_RESET_CTRL_G0_REG;
             break;
        case CPLD_QSFPDD_RESET_CTRL_G1:
             reg = CPLD_QSFPDD_RESET_CTRL_G1_REG;
             break;
        case CPLD_QSFPDD_RESET_CTRL_G2:
             reg = CPLD_QSFPDD_RESET_CTRL_G2_REG;
             break;
        case CPLD_QSFPDD_RESET_CTRL_G3:
             reg = CPLD_QSFPDD_RESET_CTRL_G3_REG;
             break;
        case CPLD_QSFPDD_LP_MODE_G0:
             reg = CPLD_QSFPDD_LP_MODE_G0_REG;
             break;
        case CPLD_QSFPDD_LP_MODE_G1:
             reg = CPLD_QSFPDD_LP_MODE_G1_REG;
             break;
        case CPLD_QSFPDD_LP_MODE_G2:
             reg = CPLD_QSFPDD_LP_MODE_G2_REG;
             break;
        case CPLD_QSFPDD_LP_MODE_G3:
             reg = CPLD_QSFPDD_LP_MODE_G3_REG;
             break;
        case CPLD_SFP_TXFAULT:
             reg = CPLD_SFP_TXFAULT_REG;
             break;
        case CPLD_SFP_RXLOS:
             reg = CPLD_SFP_RXLOS_REG;
             break;
        case CPLD_SFP_ABS:
             reg = CPLD_SFP_ABS_REG;
             break;
        case CPLD_SFP_PLUG_EVENT:
             reg = CPLD_SFP_PLUG_EVENT_REG;
             break;
        case CPLD_SFP_TX_DIS:
             reg = CPLD_SFP_TX_DIS_REG;
             break;
        case CPLD_SFP_RS:
             reg = CPLD_SFP_RS_REG;
             break;
        case CPLD_SFP_TS:
             reg = CPLD_SFP_TS_REG;
             break;
        default:
            return -EINVAL;
    }
    return read_cpld_reg(dev, buf, reg);
}

/* handle write for attributes */
static ssize_t write_cpld_callback(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 reg = 0;

    switch (attr->index) {
        case CPLD_INTERRUPT_MASK:
             reg = CPLD_INTERRUPT_MASK_REG;
             break;
        case CPLD_RESET_CTRL1:
             reg = CPLD_RESET_CTRL1_REG;
             break;
        case CPLD_RESET_CTRL2:
             reg = CPLD_RESET_CTRL2_REG;
             break;
        case CPLD_RESET_CTRL3:
             reg = CPLD_RESET_CTRL3_REG;
             break;
        case CPLD_MISC_CTRL:
             reg = CPLD_MISC_CTRL_REG;
             break;
        case CPLD_TIMING_CTRL:
             reg = CPLD_TIMING_CTRL_REG;
             break;
        case CPLD_MUX_CTRL:
             reg = CPLD_MUX_CTRL_REG;
             break;
        case CPLD_BMC_WATCHDOG:
             reg = CPLD_BMC_WATCHDOG_REG;
             break;
        case CPLD_BEACON_LED_CTRL:
             reg = CPLD_BEACON_LED_CTRL_REG;
             break;
        case CPLD_PORT_INT_MASK:
             reg = CPLD_PORT_INT_MASK_REG;
             break;
        case CPLD_QSFPDD_RESET_CTRL_G0:
             reg = CPLD_QSFPDD_RESET_CTRL_G0_REG;
             break;
        case CPLD_QSFPDD_RESET_CTRL_G1:
             reg = CPLD_QSFPDD_RESET_CTRL_G1_REG;
             break;
        case CPLD_QSFPDD_RESET_CTRL_G2:
             reg = CPLD_QSFPDD_RESET_CTRL_G2_REG;
             break;
        case CPLD_QSFPDD_RESET_CTRL_G3:
             reg = CPLD_QSFPDD_RESET_CTRL_G3_REG;
             break;
        case CPLD_QSFPDD_LP_MODE_G0:
             reg = CPLD_QSFPDD_LP_MODE_G0_REG;
             break;
        case CPLD_QSFPDD_LP_MODE_G1:
             reg = CPLD_QSFPDD_LP_MODE_G1_REG;
             break;
        case CPLD_QSFPDD_LP_MODE_G2:
             reg = CPLD_QSFPDD_LP_MODE_G2_REG;
             break;
        case CPLD_QSFPDD_LP_MODE_G3:
             reg = CPLD_QSFPDD_LP_MODE_G3_REG;
             break;
        case CPLD_SFP_TX_DIS:
             reg = CPLD_SFP_TX_DIS_REG;
             break;
        case CPLD_SFP_RS:
             reg = CPLD_SFP_RS_REG;
             break;
        case CPLD_SFP_TS:
             reg = CPLD_SFP_TS_REG;
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
        dev_err(dev, "I2C_WRITE_BYTE_DATA error, return=%d\n", ret);
        return ret;
    }

    return count;
}

/* add valid cpld client to list */
static void s9310_cpld_add_client(struct i2c_client *client)
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
static void s9310_cpld_remove_client(struct i2c_client *client)
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
static int s9310_cpld_probe(struct i2c_client *client,
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

    if (INVALID(idx, cpld1, cpld3)) {
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
                    &s9310_cpld1_group);
        break;
    case cpld2:    	  
        status = sysfs_create_group(&client->dev.kobj,
                    &s9310_cpld2_group);
        break;
    case cpld3:
        status = sysfs_create_group(&client->dev.kobj,
                    &s9310_cpld3_group);
        break;
    default:
        status = -EINVAL;
    }

    if (status)
        goto exit;

    dev_info(&client->dev, "chip found\n");

    /* add probe chip to client list */
    s9310_cpld_add_client(client);

    return 0;
exit:
    switch (data->index) {
    case cpld1:
        sysfs_remove_group(&client->dev.kobj, &s9310_cpld1_group);
        break;
    case cpld2:    	  
    	  sysfs_remove_group(&client->dev.kobj, &s9310_cpld2_group);
        break;
    case cpld3:
    	  sysfs_remove_group(&client->dev.kobj, &s9310_cpld3_group);
        break;
    default:
        break;
    }
    return status;
}

/* cpld drvier remove */
static int s9310_cpld_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);

    switch (data->index) {
    case cpld1:
        sysfs_remove_group(&client->dev.kobj, &s9310_cpld1_group);
        break;
    case cpld2:
    	  sysfs_remove_group(&client->dev.kobj, &s9310_cpld2_group);
        break;
    case cpld3:
    	  sysfs_remove_group(&client->dev.kobj, &s9310_cpld3_group);
        break;
    }

    s9310_cpld_remove_client(client);
    return 0;
}

MODULE_DEVICE_TABLE(i2c, s9310_cpld_id);

static struct i2c_driver s9310_cpld_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name = "x86_64_ufispace_s9310_32d_cpld",
    },
    .probe = s9310_cpld_probe,
    .remove = s9310_cpld_remove,
    .id_table = s9310_cpld_id,
    .address_list = cpld_i2c_addr,
};

/* provide cpld register read */
/* cpld_idx indicate the index of cpld device */
int s9310_cpld_read(u8 cpld_idx,
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
EXPORT_SYMBOL(s9310_cpld_read);

/* provide cpld register write */
/* cpld_idx indicate the index of cpld device */
int s9310_cpld_write(u8 cpld_idx,
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
EXPORT_SYMBOL(s9310_cpld_write);

static int __init s9310_cpld_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&s9310_cpld_driver);
}

static void __exit s9310_cpld_exit(void)
{
    i2c_del_driver(&s9310_cpld_driver);
}

MODULE_AUTHOR("Leo Lin <leo.yt.lin@ufispace.com>");
MODULE_DESCRIPTION("x86_64_ufispace_s9310_cpld driver");
MODULE_LICENSE("GPL");

module_init(s9310_cpld_init);
module_exit(s9310_cpld_exit);
