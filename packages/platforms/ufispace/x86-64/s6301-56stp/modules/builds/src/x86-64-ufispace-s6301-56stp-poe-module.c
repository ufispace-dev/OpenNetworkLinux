/*
 * A i2c poe driver for the ufispace_s6301_56stp
 *
 * Copyright (C) 2017-2024 UfiSpace Technology Corporation.
 * Leo Lin <leo.yt.lin@ufispace.com>
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
#include <linux/version.h>
#include "x86-64-ufispace-s6301-56stp-poe-api.h"
#include "x86-64-ufispace-s6301-56stp-saipoe.h"
#include "x86-64-ufispace-s6301-56stp-poe-module.h"

static LIST_HEAD(poe_client_list); /* client list for poe */
static struct mutex list_lock;     /* mutex for client list */

/* poe device id and data */
static const struct i2c_device_id s6301_56stp_poe_id[] = {
    {"s6301_56stp_poe", poe_pse},
    {}};

/* poe sysfs attributes hook functions */
static ssize_t read_poe_callback(struct device *dev,
                                 struct device_attribute *da, char *buf);
static ssize_t write_poe_callback(struct device *dev,
                                  struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_poe_port_callback(struct device *dev,
                                      struct device_attribute *da, char *buf);
static ssize_t write_poe_port_callback(struct device *dev,
                                       struct device_attribute *da, const char *buf, size_t count);

u8 enable_log_read = LOG_DISABLE;
u8 enable_log_write = LOG_DISABLE;
u8 enable_log_sys = LOG_ENABLE;
u8 poe_debug = LOG_NONE;

/* Addresses scanned for s6301_56stp_poe */
static const unsigned short poe_i2c_addr[] = {0x20, I2C_CLIENT_END};

/* define all support register access of POE in attribute */
/* POE SYS */
static SENSOR_DEVICE_ATTR(poe_sys_info, S_IRUGO,
                          read_poe_callback, NULL, POE_SYS_INFO);
static SENSOR_DEVICE_ATTR(poe_init, S_IWUSR,
                          NULL, write_poe_callback, POE_INIT);
static SENSOR_DEVICE_ATTR(poe_save_cfg, S_IWUSR,
                          NULL, write_poe_callback, POE_SAVE_CFG);
static SENSOR_DEVICE_ATTR(poe_pse_total_power, S_IRUGO,
                          read_poe_callback, NULL, POE_PSE_TOTAL_POWER);
static SENSOR_DEVICE_ATTR(poe_pse_power_consumption, S_IRUGO,
                          read_poe_callback, NULL, POE_PSE_POWER_CONSUMPTION);
static SENSOR_DEVICE_ATTR(poe_pse_sw_version, S_IRUGO,
                          read_poe_callback, NULL, POE_PSE_SW_VERSION);
static SENSOR_DEVICE_ATTR(poe_pse_hw_version, S_IRUGO,
                          read_poe_callback, NULL, POE_PSE_HW_VERSION);
static SENSOR_DEVICE_ATTR(poe_pse_temperature, S_IRUGO,
                          read_poe_callback, NULL, POE_PSE_TEMPERATURE);
static SENSOR_DEVICE_ATTR(poe_pse_status, S_IRUGO,
                          read_poe_callback, NULL, POE_PSE_STATUS);
static SENSOR_DEVICE_ATTR(poe_pse_power_limit_mode, S_IRUGO | S_IWUSR,
                          read_poe_callback, write_poe_callback, POE_PSE_POWER_LIMIT_MODE);
static SENSOR_DEVICE_ATTR(poe_debug, S_IRUGO | S_IWUSR,
                          read_poe_callback, write_poe_callback, POE_DEBUG);

#define POE_PORT_SENSOR_DEVICE_ATTR(port_num)                                                                                                                                          \
    static SENSOR_DEVICE_ATTR(poe_port_##port_num##_set_detect_type, S_IWUSR, NULL, write_poe_port_callback, POE_PORT_##port_num##_SET_DETECT_TYPE);                                   \
    static SENSOR_DEVICE_ATTR(poe_port_##port_num##_set_disconn_type, S_IWUSR, NULL, write_poe_port_callback, POE_PORT_##port_num##_SET_DISCONN_TYPE);                                 \
    static SENSOR_DEVICE_ATTR(poe_port_##port_num##_get_power, S_IRUGO, read_poe_port_callback, NULL, POE_PORT_##port_num##_GET_POWER);                                                \
    static SENSOR_DEVICE_ATTR(poe_port_##port_num##_get_temp, S_IRUGO, read_poe_port_callback, NULL, POE_PORT_##port_num##_GET_TEMP);                                                  \
    static SENSOR_DEVICE_ATTR(poe_port_##port_num##_get_config, S_IRUGO, read_poe_port_callback, NULL, POE_PORT_##port_num##_GET_CONFIG);                                              \
    static SENSOR_DEVICE_ATTR(poe_port_##port_num##_get_status_info, S_IRUGO, read_poe_port_callback, NULL, POE_PORT_##port_num##_GET_STATUS_INFO);                                    \
    static SENSOR_DEVICE_ATTR(poe_port_##port_num##_standard, S_IRUGO | S_IWUSR, read_poe_port_callback, write_poe_port_callback, POE_PORT_##port_num##_STANDARD);                     \
    static SENSOR_DEVICE_ATTR(poe_port_##port_num##_admin_enable_state, S_IRUGO | S_IWUSR, read_poe_port_callback, write_poe_port_callback, POE_PORT_##port_num##_ADMIN_ENABLE_STATE); \
    static SENSOR_DEVICE_ATTR(poe_port_##port_num##_power_limit, S_IRUGO | S_IWUSR, read_poe_port_callback, write_poe_port_callback, POE_PORT_##port_num##_POWER_LIMIT);               \
    static SENSOR_DEVICE_ATTR(poe_port_##port_num##_power_priority, S_IRUGO | S_IWUSR, read_poe_port_callback, write_poe_port_callback, POE_PORT_##port_num##_POWER_PRIORITY);         \
    static SENSOR_DEVICE_ATTR(poe_port_##port_num##_power_consumption, S_IRUGO, read_poe_port_callback, NULL, POE_PORT_##port_num##_POWER_CONSUMPTION);                                \
    static SENSOR_DEVICE_ATTR(poe_port_##port_num##_status, S_IRUGO, read_poe_port_callback, NULL, POE_PORT_##port_num##_STATUS);

/* POE PORT */
POE_PORT_SENSOR_DEVICE_ATTR(0)
POE_PORT_SENSOR_DEVICE_ATTR(1)
POE_PORT_SENSOR_DEVICE_ATTR(2)
POE_PORT_SENSOR_DEVICE_ATTR(3)
POE_PORT_SENSOR_DEVICE_ATTR(4)
POE_PORT_SENSOR_DEVICE_ATTR(5)
POE_PORT_SENSOR_DEVICE_ATTR(6)
POE_PORT_SENSOR_DEVICE_ATTR(7)
POE_PORT_SENSOR_DEVICE_ATTR(8)
POE_PORT_SENSOR_DEVICE_ATTR(9)
POE_PORT_SENSOR_DEVICE_ATTR(10)
POE_PORT_SENSOR_DEVICE_ATTR(11)
POE_PORT_SENSOR_DEVICE_ATTR(12)
POE_PORT_SENSOR_DEVICE_ATTR(13)
POE_PORT_SENSOR_DEVICE_ATTR(14)
POE_PORT_SENSOR_DEVICE_ATTR(15)
POE_PORT_SENSOR_DEVICE_ATTR(16)
POE_PORT_SENSOR_DEVICE_ATTR(17)
POE_PORT_SENSOR_DEVICE_ATTR(18)
POE_PORT_SENSOR_DEVICE_ATTR(19)
POE_PORT_SENSOR_DEVICE_ATTR(20)
POE_PORT_SENSOR_DEVICE_ATTR(21)
POE_PORT_SENSOR_DEVICE_ATTR(22)
POE_PORT_SENSOR_DEVICE_ATTR(23)
POE_PORT_SENSOR_DEVICE_ATTR(24)
POE_PORT_SENSOR_DEVICE_ATTR(25)
POE_PORT_SENSOR_DEVICE_ATTR(26)
POE_PORT_SENSOR_DEVICE_ATTR(27)
POE_PORT_SENSOR_DEVICE_ATTR(28)
POE_PORT_SENSOR_DEVICE_ATTR(29)
POE_PORT_SENSOR_DEVICE_ATTR(30)
POE_PORT_SENSOR_DEVICE_ATTR(31)
POE_PORT_SENSOR_DEVICE_ATTR(32)
POE_PORT_SENSOR_DEVICE_ATTR(33)
POE_PORT_SENSOR_DEVICE_ATTR(34)
POE_PORT_SENSOR_DEVICE_ATTR(35)
POE_PORT_SENSOR_DEVICE_ATTR(36)
POE_PORT_SENSOR_DEVICE_ATTR(37)
POE_PORT_SENSOR_DEVICE_ATTR(38)
POE_PORT_SENSOR_DEVICE_ATTR(39)
POE_PORT_SENSOR_DEVICE_ATTR(40)
POE_PORT_SENSOR_DEVICE_ATTR(41)
POE_PORT_SENSOR_DEVICE_ATTR(42)
POE_PORT_SENSOR_DEVICE_ATTR(43)
POE_PORT_SENSOR_DEVICE_ATTR(44)
POE_PORT_SENSOR_DEVICE_ATTR(45)
POE_PORT_SENSOR_DEVICE_ATTR(46)
POE_PORT_SENSOR_DEVICE_ATTR(47)

/* define support attributes of POE */
/* POE SYS */
static struct attribute *s6301_56stp_poe_sys_attributes[] = {
    &sensor_dev_attr_poe_sys_info.dev_attr.attr,
    &sensor_dev_attr_poe_init.dev_attr.attr,
    &sensor_dev_attr_poe_save_cfg.dev_attr.attr,
    &sensor_dev_attr_poe_pse_total_power.dev_attr.attr,
    &sensor_dev_attr_poe_pse_power_consumption.dev_attr.attr,
    &sensor_dev_attr_poe_pse_sw_version.dev_attr.attr,
    &sensor_dev_attr_poe_pse_hw_version.dev_attr.attr,
    &sensor_dev_attr_poe_pse_temperature.dev_attr.attr,
    &sensor_dev_attr_poe_pse_status.dev_attr.attr,
    &sensor_dev_attr_poe_pse_power_limit_mode.dev_attr.attr,
    &sensor_dev_attr_poe_debug.dev_attr.attr,
    NULL};

/* POE PORT */
#define POE_PORT_SENSOR_DEVICE_ATTRS(port_num)                                   \
    static struct attribute *s6301_56stp_poe_port_##port_num##_attributes[] = {  \
        &sensor_dev_attr_poe_port_##port_num##_set_detect_type.dev_attr.attr,    \
        &sensor_dev_attr_poe_port_##port_num##_set_disconn_type.dev_attr.attr,   \
        &sensor_dev_attr_poe_port_##port_num##_get_power.dev_attr.attr,          \
        &sensor_dev_attr_poe_port_##port_num##_get_temp.dev_attr.attr,           \
        &sensor_dev_attr_poe_port_##port_num##_get_config.dev_attr.attr,         \
        &sensor_dev_attr_poe_port_##port_num##_get_status_info.dev_attr.attr,    \
        &sensor_dev_attr_poe_port_##port_num##_standard.dev_attr.attr,           \
        &sensor_dev_attr_poe_port_##port_num##_admin_enable_state.dev_attr.attr, \
        &sensor_dev_attr_poe_port_##port_num##_power_limit.dev_attr.attr,        \
        &sensor_dev_attr_poe_port_##port_num##_power_priority.dev_attr.attr,     \
        &sensor_dev_attr_poe_port_##port_num##_power_consumption.dev_attr.attr,  \
        &sensor_dev_attr_poe_port_##port_num##_status.dev_attr.attr,             \
        NULL};

POE_PORT_SENSOR_DEVICE_ATTRS(0)
POE_PORT_SENSOR_DEVICE_ATTRS(1)
POE_PORT_SENSOR_DEVICE_ATTRS(2)
POE_PORT_SENSOR_DEVICE_ATTRS(3)
POE_PORT_SENSOR_DEVICE_ATTRS(4)
POE_PORT_SENSOR_DEVICE_ATTRS(5)
POE_PORT_SENSOR_DEVICE_ATTRS(6)
POE_PORT_SENSOR_DEVICE_ATTRS(7)
POE_PORT_SENSOR_DEVICE_ATTRS(8)
POE_PORT_SENSOR_DEVICE_ATTRS(9)
POE_PORT_SENSOR_DEVICE_ATTRS(10)
POE_PORT_SENSOR_DEVICE_ATTRS(11)
POE_PORT_SENSOR_DEVICE_ATTRS(12)
POE_PORT_SENSOR_DEVICE_ATTRS(13)
POE_PORT_SENSOR_DEVICE_ATTRS(14)
POE_PORT_SENSOR_DEVICE_ATTRS(15)
POE_PORT_SENSOR_DEVICE_ATTRS(16)
POE_PORT_SENSOR_DEVICE_ATTRS(17)
POE_PORT_SENSOR_DEVICE_ATTRS(18)
POE_PORT_SENSOR_DEVICE_ATTRS(19)
POE_PORT_SENSOR_DEVICE_ATTRS(20)
POE_PORT_SENSOR_DEVICE_ATTRS(21)
POE_PORT_SENSOR_DEVICE_ATTRS(22)
POE_PORT_SENSOR_DEVICE_ATTRS(23)
POE_PORT_SENSOR_DEVICE_ATTRS(24)
POE_PORT_SENSOR_DEVICE_ATTRS(25)
POE_PORT_SENSOR_DEVICE_ATTRS(26)
POE_PORT_SENSOR_DEVICE_ATTRS(27)
POE_PORT_SENSOR_DEVICE_ATTRS(28)
POE_PORT_SENSOR_DEVICE_ATTRS(29)
POE_PORT_SENSOR_DEVICE_ATTRS(30)
POE_PORT_SENSOR_DEVICE_ATTRS(31)
POE_PORT_SENSOR_DEVICE_ATTRS(32)
POE_PORT_SENSOR_DEVICE_ATTRS(33)
POE_PORT_SENSOR_DEVICE_ATTRS(34)
POE_PORT_SENSOR_DEVICE_ATTRS(35)
POE_PORT_SENSOR_DEVICE_ATTRS(36)
POE_PORT_SENSOR_DEVICE_ATTRS(37)
POE_PORT_SENSOR_DEVICE_ATTRS(38)
POE_PORT_SENSOR_DEVICE_ATTRS(39)
POE_PORT_SENSOR_DEVICE_ATTRS(40)
POE_PORT_SENSOR_DEVICE_ATTRS(41)
POE_PORT_SENSOR_DEVICE_ATTRS(42)
POE_PORT_SENSOR_DEVICE_ATTRS(43)
POE_PORT_SENSOR_DEVICE_ATTRS(44)
POE_PORT_SENSOR_DEVICE_ATTRS(45)
POE_PORT_SENSOR_DEVICE_ATTRS(46)
POE_PORT_SENSOR_DEVICE_ATTRS(47)

/* poe sys attributes group */
static const struct attribute_group s6301_56stp_poe_sys_group = {
    .attrs = s6301_56stp_poe_sys_attributes,
    .name = "sys"};

/* poe port attributes groups */
#define POE_PORT_ATTR_GROUP(port_num) \
    {.attrs = s6301_56stp_poe_port_##port_num##_attributes, .name = "port-" #port_num}

static const struct attribute_group s6301_56stp_poe_port_groups[POE_MAX_PORT_NUM + 1] = {
    POE_PORT_ATTR_GROUP(0),
    POE_PORT_ATTR_GROUP(1),
    POE_PORT_ATTR_GROUP(2),
    POE_PORT_ATTR_GROUP(3),
    POE_PORT_ATTR_GROUP(4),
    POE_PORT_ATTR_GROUP(5),
    POE_PORT_ATTR_GROUP(6),
    POE_PORT_ATTR_GROUP(7),
    POE_PORT_ATTR_GROUP(8),
    POE_PORT_ATTR_GROUP(9),
    POE_PORT_ATTR_GROUP(10),
    POE_PORT_ATTR_GROUP(11),
    POE_PORT_ATTR_GROUP(12),
    POE_PORT_ATTR_GROUP(13),
    POE_PORT_ATTR_GROUP(14),
    POE_PORT_ATTR_GROUP(15),
    POE_PORT_ATTR_GROUP(16),
    POE_PORT_ATTR_GROUP(17),
    POE_PORT_ATTR_GROUP(18),
    POE_PORT_ATTR_GROUP(19),
    POE_PORT_ATTR_GROUP(20),
    POE_PORT_ATTR_GROUP(21),
    POE_PORT_ATTR_GROUP(22),
    POE_PORT_ATTR_GROUP(23),
    POE_PORT_ATTR_GROUP(24),
    POE_PORT_ATTR_GROUP(25),
    POE_PORT_ATTR_GROUP(26),
    POE_PORT_ATTR_GROUP(27),
    POE_PORT_ATTR_GROUP(28),
    POE_PORT_ATTR_GROUP(29),
    POE_PORT_ATTR_GROUP(30),
    POE_PORT_ATTR_GROUP(31),
    POE_PORT_ATTR_GROUP(32),
    POE_PORT_ATTR_GROUP(33),
    POE_PORT_ATTR_GROUP(34),
    POE_PORT_ATTR_GROUP(35),
    POE_PORT_ATTR_GROUP(36),
    POE_PORT_ATTR_GROUP(37),
    POE_PORT_ATTR_GROUP(38),
    POE_PORT_ATTR_GROUP(39),
    POE_PORT_ATTR_GROUP(40),
    POE_PORT_ATTR_GROUP(41),
    POE_PORT_ATTR_GROUP(42),
    POE_PORT_ATTR_GROUP(43),
    POE_PORT_ATTR_GROUP(44),
    POE_PORT_ATTR_GROUP(45),
    POE_PORT_ATTR_GROUP(46),
    POE_PORT_ATTR_GROUP(47)};

int poe_log(u8 log_type, char *fmt, ...)
{
    if ((log_type == LOG_READ && enable_log_read) ||
        (log_type == LOG_WRITE && enable_log_write) ||
        (log_type == LOG_SYS && enable_log_sys))
    {
        va_list args;
        int r;

        va_start(args, fmt);
        r = vprintk(fmt, args);
        va_end(args);

        return r;
    }
    else
    {
        return 0;
    }
}

static int _config_poe_log(u8 log_type)
{
    switch (log_type)
    {
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

/* handle read for attributes */
static ssize_t read_poe_callback(struct device *dev,
                                 struct device_attribute *da, char *buf)
{
    char out[512];
    u32 u32_out;
    ssize_t ret = E_TYPE_SUCCESS;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct poe_data *clientdata = i2c_get_clientdata(client);
    memset(out, 0, sizeof(out));

    /* add mutex lock to protect cmd data and cmd session */
    mutex_lock(&clientdata->cmd_lock);
    switch (attr->index)
    {
    case POE_SYS_INFO:
        if (ufi_poe_SystemStatus(dev, out) != E_TYPE_SUCCESS)
        {
            ret = -EIO;
        }
        break;
    case POE_PSE_TOTAL_POWER:
        if (ufi_poe_GetPseTotalPower(dev, &u32_out) != E_TYPE_SUCCESS)
        {
            ret = -EIO;
        }
        else
        {
            ret = sprintf(out, "%d\n", u32_out);
        }
        break;
    case POE_PSE_POWER_CONSUMPTION:
        if (ufi_poe_GetPsePowerConsumption(dev, &u32_out) != E_TYPE_SUCCESS)
        {
            ret = -EIO;
        }
        else
        {
            ret = sprintf(out, "%d\n", u32_out);
        }
        break;
    case POE_PSE_SW_VERSION:
        if (ufi_poe_GetPseSWVersion(dev, out) != E_TYPE_SUCCESS)
        {
            ret = -EIO;
        }
        break;
    case POE_PSE_HW_VERSION:
        if (ufi_poe_GetPseHWVersion(dev, out) != E_TYPE_SUCCESS)
        {
            ret = -EIO;
        }
        break;
    case POE_PSE_TEMPERATURE:
        if (ufi_poe_GetPseTemperature(dev, &u32_out) != E_TYPE_SUCCESS)
        {
            ret = -EIO;
        }
        else
        {
            ret = sprintf(out, "%d\n", u32_out);
        }
        break;
    case POE_PSE_STATUS:
        if (ufi_poe_GetPseStatus(dev, &u32_out) != E_TYPE_SUCCESS)
        {
            ret = -EIO;
        }
        else
        {
            ret = sprintf(out, "%d\n", u32_out);
        }
        break;
    case POE_PSE_POWER_LIMIT_MODE:
        if (ufi_poe_GetPsePowerLimitMode(dev, &u32_out) != E_TYPE_SUCCESS)
        {
            ret = -EIO;
        }
        else
        {
            ret = sprintf(out, "%d\n", u32_out);
        }
        break;
    case POE_DEBUG:
        ret = sprintf(out, "%d\n", poe_debug);
        break;
    default:
        ret = -EINVAL;
    }
    mutex_unlock(&clientdata->cmd_lock);

    if (ret < 0)
    {
        return ret;
    }

    return sprintf(buf, out);
}

static ssize_t read_poe_port_callback(struct device *dev,
                                      struct device_attribute *da, char *buf)
{
    char out[512];
    u32 u32_out;
    int port_num, attr_type;
    ssize_t ret = E_TYPE_SUCCESS;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct poe_data *clientdata = i2c_get_clientdata(client);

    memset(out, 0, sizeof(out));

    port_num = attr->index / 100 - 1;
    attr_type = attr->index % 100;
    POE_LOG_R("read poe port %d for attr type %d\n", port_num, attr_type);

    /* add mutex lock to protect cmd data and cmd session */
    mutex_lock(&clientdata->cmd_lock);
    switch (attr_type)
    {
    case POE_PORT_GET_POWER:
        if (ufi_poe_PowerShow(dev, port_num, out) != E_TYPE_SUCCESS)
        {
            ret = -EIO;
        }
        break;
    case POE_PORT_GET_TEMP:
        if (ufi_poe_TemperatureShow(dev, port_num, out) != E_TYPE_SUCCESS)
        {
            ret = -EIO;
        }
        break;
    case POE_PORT_GET_CONFIG:
        if (ufi_poe_PortConfigShow(dev, port_num, out) != E_TYPE_SUCCESS)
        {
            ret = -EIO;
        }
        break;
    case POE_PORT_GET_STATUS_INFO:
        if (ufi_poe_PortStatus(dev, port_num, out) != E_TYPE_SUCCESS)
        {
            ret = -EIO;
        }
        break;
    case POE_PORT_STANDARD:
        if (ufi_poe_GetPortStandard(dev, port_num, &u32_out) != E_TYPE_SUCCESS)
        {
            ret = -EIO;
        }
        else
        {
            ret = sprintf(out, "%d\n", u32_out);
        }
        break;
    case POE_PORT_ADMIN_ENABLE_STATE:
        if (ufi_poe_GetPortAdminEnableState(dev, port_num, &u32_out) != E_TYPE_SUCCESS)
        {
            ret = -EIO;
        }
        else
        {
            ret = sprintf(out, "%d\n", u32_out);
        }
        break;
    case POE_PORT_POWER_LIMIT:
        if (ufi_poe_GetPortPowerLimit(dev, port_num, &u32_out) != E_TYPE_SUCCESS)
        {
            ret = -EIO;
        }
        else
        {
            ret = sprintf(out, "%d\n", u32_out);
        }
        break;
    case POE_PORT_POWER_PRIORITY:
        if (ufi_poe_GetPortPowerPriority(dev, port_num, &u32_out) != E_TYPE_SUCCESS)
        {
            ret = -EIO;
        }
        else
        {
            ret = sprintf(out, "%d\n", u32_out);
        }
        break;
    case POE_PORT_POWER_CONSUMPTION:
        if (ufi_poe_GetPortPowerConsumption(dev, port_num, &u32_out) != E_TYPE_SUCCESS)
        {
            ret = -EIO;
        }
        else
        {
            ret = sprintf(out, "%d\n", u32_out);
        }
        break;
    case POE_PORT_STATUS:
        if (ufi_poe_GetPortStatus(dev, port_num, &u32_out) != E_TYPE_SUCCESS)
        {
            ret = -EIO;
        }
        else
        {
            ret = sprintf(out, "%d\n", u32_out);
        }
        break;
    default:
        ret = -EINVAL;
    }
    mutex_unlock(&clientdata->cmd_lock);

    if (ret < 0)
    {
        return ret;
    }

    return sprintf(buf, out);
}

/* handle write for attributes */
static ssize_t write_poe_callback(struct device *dev,
                                  struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 val;
    ssize_t ret = count;
    struct i2c_client *client = to_i2c_client(dev);
    struct poe_data *clientdata = i2c_get_clientdata(client);

    if (kstrtou8(buf, 0, &val) < 0)
        return -EINVAL;

    /* add mutex lock to protect cmd data and cmd session */
    mutex_lock(&clientdata->cmd_lock);
    switch (attr->index)
    {
    case POE_INIT:
        if (val == 1)
        {
            if (ufi_poe_init(dev) != E_TYPE_SUCCESS)
            {
                dev_err(dev, "bcm poe init fail");
                ret = -EIO;
            }
        }
        break;
    case POE_SAVE_CFG:
        if (val == 1)
        {
            if (ufi_poe_SaveRunningConfig(dev) != E_TYPE_SUCCESS)
            {
                dev_err(dev, "bcm poe save runnning config fail");
                ret = -EIO;
            }
        }
        break;
    case POE_DEBUG:
        if (_config_poe_log(val) < 0)
        {
            ret = -EINVAL;
        }
        else
        {
            poe_debug = val;
        }
        dev_info(dev, "config poe log mode to %d done\n", val);
        break;
    case POE_PSE_POWER_LIMIT_MODE:
        if (val <= SAI_POE_DEVICE_LIMIT_MODE_CLASS && val >= SAI_POE_DEVICE_LIMIT_MODE_PORT)
        {
            if (ufi_poe_SetPsePowerLimitMode(dev, val) != E_TYPE_SUCCESS)
            {
                dev_err(dev, "Set PSE power limit mode %d fail\n", val);
                ret = -EIO;
            }
        }
        else
        {
            ret = -EINVAL;
        }
        break;
    default:
        ret = -EINVAL;
    }
    mutex_unlock(&clientdata->cmd_lock);

    if (ret < 0)
    {
        return ret;
    }

    return ret;
}

static ssize_t write_poe_port_callback(struct device *dev,
                                       struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 val;
    int port_num, attr_type;
    ssize_t ret = count;
    struct i2c_client *client = to_i2c_client(dev);
    struct poe_data *clientdata = i2c_get_clientdata(client);

    if (kstrtou8(buf, 0, &val) < 0)
        return -EINVAL;

    port_num = attr->index / 100 - 1;
    attr_type = attr->index % 100;
    POE_LOG_W("write poe port %d for attr type %d value %d\n", port_num, attr_type, val);

    /* add mutex lock to protect cmd data and cmd session */
    mutex_lock(&clientdata->cmd_lock);
    switch (attr_type)
    {
    case POE_PORT_SET_DETECT_TYPE:
        if (val <= E_BCM_POE_PD_STANDARD_THEN_LEGACY && val >= E_BCM_POE_NO_DETECT)
        {
            if (ufi_poe_SetPortDetectType(dev, port_num, val) != E_TYPE_SUCCESS)
            {
                dev_err(dev, "Set Port %d Detect Type %d fail\n", port_num, val);
                ret = -EIO;
            }
        }
        else
        {
            ret = -EINVAL;
        }
        break;
    case POE_PORT_SET_DISCONN_TYPE:
        if (val <= E_BCM_POE_DISCONNECT_DC && val >= E_BCM_POE_DISCONNECT_NONE)
        {
            if (ufi_poe_SetPortDisconnectType(dev, port_num, val) != E_TYPE_SUCCESS)
            {
                dev_err(dev, "Set Port %d Disconnect Type %d fail\n", port_num, val);
                ret = -EIO;
            }
        }
        else
        {
            ret = -EINVAL;
        }
        break;
    case POE_PORT_STANDARD:
        switch (val)
        {
        case SAI_POE_PORT_STANDARD_TYPE_AF:
        case SAI_POE_PORT_STANDARD_TYPE_AT:
            if (ufi_poe_SetPortStandard(dev, port_num, val) != E_TYPE_SUCCESS)
            {
                dev_err(dev, "Set Port %d Standard %d fail\n", port_num, val);
                ret = -EIO;
            }
            break;
        default:
            ret = -EINVAL;
        }
        break;
    case POE_PORT_ADMIN_ENABLE_STATE:
        if (val <= 1 && val >= 0)
        {
            if (ufi_poe_SetPortAdminEnableState(dev, port_num, val) != E_TYPE_SUCCESS)
            {
                dev_err(dev, "Set Port %d Admin Enable State %d fail\n", port_num, val);
                ret = -EIO;
            }
        }
        else
        {
            ret = -EINVAL;
        }
        break;
    case POE_PORT_POWER_LIMIT:
        if (val <= 0x33 && val >= 1)
        {
            if (ufi_poe_SetPortPowerLimit(dev, port_num, val) != E_TYPE_SUCCESS)
            {
                dev_err(dev, "Set Port %d Power Limit %d fail\n", port_num, val);
                ret = -EIO;
            }
        }
        else
        {
            ret = -EINVAL;
        }
        break;
    case POE_PORT_POWER_PRIORITY:
        if (val <= SAI_POE_PORT_POWER_PRIORITY_TYPE_CRITICAL && val >= SAI_POE_PORT_POWER_PRIORITY_TYPE_LOW)
        {
            if (ufi_poe_SetPortPowerPriority(dev, port_num, val) != E_TYPE_SUCCESS)
            {
                dev_err(dev, "Set Port %d Power Priority %d fail\n", port_num, val);
                ret = -EIO;
            }
        }
        else
        {
            ret = -EINVAL;
        }
        break;
    default:
        ret = -EINVAL;
    }
    mutex_unlock(&clientdata->cmd_lock);

    if (ret < 0)
    {
        return ret;
    }

    return ret;
}

/* add valid poe client to list */
static void s6301_56stp_poe_add_client(struct i2c_client *client)
{
    struct poe_client_node *node = NULL;

    node = kzalloc(sizeof(struct poe_client_node), GFP_KERNEL);
    if (!node)
    {
        dev_info(&client->dev,
                 "Can't allocate poe_client_node for index %d\n",
                 client->addr);
        return;
    }

    node->client = client;

    mutex_lock(&list_lock);
    list_add(&node->list, &poe_client_list);
    mutex_unlock(&list_lock);
}

/* remove exist poe client in list */
static void s6301_56stp_poe_remove_client(struct i2c_client *client)
{
    struct list_head *list_node = NULL;
    struct poe_client_node *poe_node = NULL;
    int found = 0;

    mutex_lock(&list_lock);
    list_for_each(list_node, &poe_client_list)
    {
        poe_node = list_entry(list_node,
                              struct poe_client_node, list);

        if (poe_node->client == client)
        {
            found = 1;
            break;
        }
    }

    if (found)
    {
        list_del(list_node);
        kfree(poe_node);
    }
    mutex_unlock(&list_lock);
}

/* poe drvier probe */
static int s6301_56stp_poe_probe(struct i2c_client *client,
                                 const struct i2c_device_id *dev_id)
{
    int status;
    struct poe_data *data = NULL;
    int i;

    printk("s6301_56stp_poe_probe\n");

    data = kzalloc(sizeof(struct poe_data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    /* init poe data for client */
    i2c_set_clientdata(client, data);
    mutex_init(&data->access_lock);
    mutex_init(&data->cmd_lock);

    if (!i2c_check_functionality(client->adapter,
                                 I2C_FUNC_SMBUS_I2C_BLOCK))
    {
        dev_info(&client->dev,
                 "i2c_check_functionality failed (0x%x)\n",
                 client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "check i2c functionality done\n");

    /* register sysfs hooks */
    /* poe sys */
    status = sysfs_create_group(&client->dev.kobj, &s6301_56stp_poe_sys_group);
    dev_info(&client->dev, "create poe sys sysfs status=%d\n", status);

    /* poe port */
    for (i = 0; i <= POE_MAX_PORT_NUM; i++)
    {
        status = sysfs_create_group(&client->dev.kobj, &s6301_56stp_poe_port_groups[i]);
        dev_info(&client->dev, "create poe port %d sysfs status=%d\n", i, status);
    }

    if (status)
        goto exit;

    dev_info(&client->dev, "chip found\n");

    /* add probe chip to client list */
    s6301_56stp_poe_add_client(client);

    return 0;
exit:
    sysfs_remove_group(&client->dev.kobj, &s6301_56stp_poe_sys_group);

    for (i = 0; i <= POE_MAX_PORT_NUM; i++)
    {
        sysfs_remove_group(&client->dev.kobj, &s6301_56stp_poe_port_groups[i]);
    }
    return status;
}

/* poe drvier remove */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
static int
#else
static void
#endif
s6301_56stp_poe_remove(struct i2c_client *client)
{
    int i;

    sysfs_remove_group(&client->dev.kobj, &s6301_56stp_poe_sys_group);

    for (i = 0; i <= POE_MAX_PORT_NUM; i++)
    {
        sysfs_remove_group(&client->dev.kobj, &s6301_56stp_poe_port_groups[i]);
    }

    s6301_56stp_poe_remove_client(client);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
    return 0;
#endif
}

MODULE_DEVICE_TABLE(i2c, s6301_56stp_poe_id);

static struct i2c_driver s6301_56stp_poe_driver = {
    .class = I2C_CLASS_HWMON,
    .driver = {
        .name = "x86-64-ufispace-s6301-56stp-poe-module",
    },
    .probe = s6301_56stp_poe_probe,
    .remove = s6301_56stp_poe_remove,
    .id_table = s6301_56stp_poe_id,
    .address_list = poe_i2c_addr,
};

static int __init s6301_56stp_poe_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&s6301_56stp_poe_driver);
}

static void __exit s6301_56stp_poe_exit(void)
{
    i2c_del_driver(&s6301_56stp_poe_driver);
}

MODULE_AUTHOR("Leo Lin <leo.yt.lin@ufispace.com>");
MODULE_DESCRIPTION("x86_64_ufispace_s6301__56stp_poe driver");
MODULE_LICENSE("GPL");

module_init(s6301_56stp_poe_init);
module_exit(s6301_56stp_poe_exit);
