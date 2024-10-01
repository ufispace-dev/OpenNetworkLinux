/* header file for i2c poe driver of ufispace_s6301_56stp
 *
 * Copyright (C) 2024 UfiSpace Technology Corporation.
 * Leo Lin <leo.yt.lin@ufispace.com>
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

#ifndef UFISPACE_S6301_56STP_POE_MODULE_H
#define UFISPACE_S6301_56STP_POE_MODUEL_H

/* POE device index value */
enum poe_id
{
    poe_pse
};

enum poe_port_sysfs_type
{
    POE_PORT_SET_DETECT_TYPE = 1,
    POE_PORT_SET_DISCONN_TYPE = 2,
    POE_PORT_GET_POWER = 3,
    POE_PORT_GET_TEMP = 4,
    POE_PORT_GET_CONFIG = 5,
    POE_PORT_GET_STATUS_INFO = 6,
    POE_PORT_STANDARD = 7,
    POE_PORT_ADMIN_ENABLE_STATE = 8,
    POE_PORT_POWER_LIMIT = 9,
    POE_PORT_POWER_PRIORITY = 10,
    POE_PORT_POWER_CONSUMPTION = 11,
    POE_PORT_STATUS = 12
};

#define POE_PORT_ATTR_INDEX(port_num, prefix)              \
    POE_PORT_##port_num##_SET_DETECT_TYPE = prefix##01,    \
    POE_PORT_##port_num##_SET_DISCONN_TYPE = prefix##02,   \
    POE_PORT_##port_num##_GET_POWER = prefix##03,          \
    POE_PORT_##port_num##_GET_TEMP = prefix##04,           \
    POE_PORT_##port_num##_GET_CONFIG = prefix##05,         \
    POE_PORT_##port_num##_GET_STATUS_INFO = prefix##06,    \
    POE_PORT_##port_num##_STANDARD = prefix##07,           \
    POE_PORT_##port_num##_ADMIN_ENABLE_STATE = prefix##08, \
    POE_PORT_##port_num##_POWER_LIMIT = prefix##09,        \
    POE_PORT_##port_num##_POWER_PRIORITY = prefix##10,     \
    POE_PORT_##port_num##_POWER_CONSUMPTION = prefix##11,  \
    POE_PORT_##port_num##_STATUS = prefix##12

/* sysfs attributes index  */
enum poe_sysfs_attributes
{
    /* sys */
    POE_SYS_INFO = 10000,
    POE_INIT = 10001,
    POE_SAVE_CFG = 10002,
    POE_DEBUG = 10003,
    /* for SAI adaption */
    POE_PSE_TOTAL_POWER = 10004,
    POE_PSE_POWER_CONSUMPTION = 10005,
    POE_PSE_SW_VERSION = 10006,
    POE_PSE_HW_VERSION = 10007,
    POE_PSE_TEMPERATURE = 10008,
    POE_PSE_STATUS = 10009,
    POE_PSE_POWER_LIMIT_MODE = 10010,
    POE_PORT_ATTR_INDEX(0, 1),
    POE_PORT_ATTR_INDEX(1, 2),
    POE_PORT_ATTR_INDEX(2, 3),
    POE_PORT_ATTR_INDEX(3, 4),
    POE_PORT_ATTR_INDEX(4, 5),
    POE_PORT_ATTR_INDEX(5, 6),
    POE_PORT_ATTR_INDEX(6, 7),
    POE_PORT_ATTR_INDEX(7, 8),
    POE_PORT_ATTR_INDEX(8, 9),
    POE_PORT_ATTR_INDEX(9, 10),
    POE_PORT_ATTR_INDEX(10, 11),
    POE_PORT_ATTR_INDEX(11, 12),
    POE_PORT_ATTR_INDEX(12, 13),
    POE_PORT_ATTR_INDEX(13, 14),
    POE_PORT_ATTR_INDEX(14, 15),
    POE_PORT_ATTR_INDEX(15, 16),
    POE_PORT_ATTR_INDEX(16, 17),
    POE_PORT_ATTR_INDEX(17, 18),
    POE_PORT_ATTR_INDEX(18, 19),
    POE_PORT_ATTR_INDEX(19, 20),
    POE_PORT_ATTR_INDEX(20, 21),
    POE_PORT_ATTR_INDEX(21, 22),
    POE_PORT_ATTR_INDEX(22, 23),
    POE_PORT_ATTR_INDEX(23, 24),
    POE_PORT_ATTR_INDEX(24, 25),
    POE_PORT_ATTR_INDEX(25, 26),
    POE_PORT_ATTR_INDEX(26, 27),
    POE_PORT_ATTR_INDEX(27, 28),
    POE_PORT_ATTR_INDEX(28, 29),
    POE_PORT_ATTR_INDEX(29, 30),
    POE_PORT_ATTR_INDEX(30, 31),
    POE_PORT_ATTR_INDEX(31, 32),
    POE_PORT_ATTR_INDEX(32, 33),
    POE_PORT_ATTR_INDEX(33, 34),
    POE_PORT_ATTR_INDEX(34, 35),
    POE_PORT_ATTR_INDEX(35, 36),
    POE_PORT_ATTR_INDEX(36, 37),
    POE_PORT_ATTR_INDEX(37, 38),
    POE_PORT_ATTR_INDEX(38, 39),
    POE_PORT_ATTR_INDEX(39, 40),
    POE_PORT_ATTR_INDEX(40, 41),
    POE_PORT_ATTR_INDEX(41, 42),
    POE_PORT_ATTR_INDEX(42, 43),
    POE_PORT_ATTR_INDEX(43, 44),
    POE_PORT_ATTR_INDEX(44, 45),
    POE_PORT_ATTR_INDEX(45, 46),
    POE_PORT_ATTR_INDEX(46, 47),
    POE_PORT_ATTR_INDEX(47, 48),
};

enum bsp_log_types
{
    LOG_NONE,
    LOG_RW,
    LOG_READ,
    LOG_WRITE,
    LOG_SYS
};

enum bsp_log_ctrl
{
    LOG_DISABLE,
    LOG_ENABLE
};

struct poe_client_node
{
    struct i2c_client *client;
    struct list_head list;
};

struct poe_data
{
    struct mutex access_lock; /* mutex for poe access */
    struct mutex cmd_lock;    /* mutext for cmd session */
};

#ifdef DEBUG
#define DEBUG_PRINT(fmt, args...)              \
    printk(KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
           __FILE__, __func__, __LINE__, ##args)
#else
#define DEBUG_PRINT(fmt, args...)
#endif

#define POE_LOG_R(fmt, args...)                           \
    poe_log(LOG_READ, KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
            __FILE__, __func__, __LINE__, ##args)
#define POE_LOG_W(fmt, args...)                            \
    poe_log(LOG_WRITE, KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
            __FILE__, __func__, __LINE__, ##args)
#define POE_LOG_SYS(fmt, args...)                        \
    poe_log(LOG_SYS, KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
            __FILE__, __func__, __LINE__, ##args)

#define I2C_READ_BYTE_DATA(ret, lock, i2c_client, reg)   \
    {                                                    \
        mutex_lock(lock);                                \
        ret = i2c_smbus_read_byte_data(i2c_client, reg); \
        mutex_unlock(lock);                              \
    }
#define I2C_WRITE_BYTE_DATA(ret, lock, i2c_client, reg, val)   \
    {                                                          \
        mutex_lock(lock);                                      \
        ret = i2c_smbus_write_byte_data(i2c_client, reg, val); \
        mutex_unlock(lock);                                    \
    }

#define I2C_READ_BLOCK_DATA(ret, lock, i2c_client, reg, len, val)       \
    {                                                                   \
        mutex_lock(lock);                                               \
        ret = i2c_smbus_read_i2c_block_data(i2c_client, reg, len, val); \
        mutex_unlock(lock);                                             \
    }

#define I2C_WRITE_BLOCK_DATA(ret, lock, i2c_client, reg, len, val)       \
    {                                                                    \
        mutex_lock(lock);                                                \
        ret = i2c_smbus_write_i2c_block_data(i2c_client, reg, len, val); \
        mutex_unlock(lock);                                              \
    }

/* common manipulation */
#define INVALID(i, min, max) ((i < min) || (i > max) ? 1u : 0u)
#define READ_BIT(val, bit) ((0u == (val & (1 << bit))) ? 0u : 1u)
#define SET_BIT(val, bit) (val |= (1 << bit))
#define CLEAR_BIT(val, bit) (val &= ~(1 << bit))
#define TOGGLE_BIT(val, bit) (val ^= (1 << bit))
#define _BIT(n) (1 << (n))
#define _BIT_MASK(len) (BIT(len) - 1)

/* functions */
int poe_log(u8 log_type, char *fmt, ...);

#endif
