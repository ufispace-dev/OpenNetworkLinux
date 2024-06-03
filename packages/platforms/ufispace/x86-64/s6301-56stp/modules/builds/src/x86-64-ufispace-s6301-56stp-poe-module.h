/* header file for i2c poe driver of ufispace_s6301_56stp
 *
 * Copyright (C) 2023 UfiSpace Technology Corporation.
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
enum poe_id {
    poe_pd
};

enum poe_port_sysfs_type {
    POE_PORT_SET_EN = 0,
    POE_PORT_SET_POWER_LIMIT = 1,
    POE_PORT_SET_POWER_MODE = 2,
    POE_PORT_SET_DETECT_TYPE = 3,
    POE_PORT_SET_DISCONN_TYPE = 4,
    POE_PORT_GET_POWER = 5,
    POE_PORT_GET_TEMP = 6,
    POE_PORT_GET_CONFIG = 7,
    POE_PORT_STATUS = 8,
    POE_PORT_SET_PRIORITY = 9
};

#define POE_PORT_ATTR_INDEX_BASE(port_num, prefix) \
    POE_PORT_##port_num##_SET_EN = prefix##0, \
    POE_PORT_##port_num##_SET_POWER_LIMIT = prefix##1, \
    POE_PORT_##port_num##_SET_POWER_MODE = prefix##2, \
    POE_PORT_##port_num##_SET_DETECT_TYPE = prefix##3, \
    POE_PORT_##port_num##_SET_DISCONN_TYPE = prefix##4, \
    POE_PORT_##port_num##_GET_POWER = prefix##5, \
    POE_PORT_##port_num##_GET_TEMP = prefix##6, \
    POE_PORT_##port_num##_GET_CONFIG = prefix##7, \
    POE_PORT_##port_num##_STATUS = prefix##8, \
    POE_PORT_##port_num##_SET_PRIORITY = prefix##9

#define POE_PORT_ATTR_INDEX(port_num) \
    POE_PORT_ATTR_INDEX_BASE(port_num, port_num)

/* sysfs attributes index  */
enum poe_sysfs_attributes {
    /* sys */
    POE_SYS_INFO = 1000,
    POE_INIT = 1001,
    POE_SAVE_CFG = 1002,
    POE_DEBUG = 1003,
    POE_PORT_ATTR_INDEX_BASE(0,),
    POE_PORT_ATTR_INDEX(1),  
    POE_PORT_ATTR_INDEX(2),
    POE_PORT_ATTR_INDEX(3),
    POE_PORT_ATTR_INDEX(4),
    POE_PORT_ATTR_INDEX(5),
    POE_PORT_ATTR_INDEX(6),
    POE_PORT_ATTR_INDEX(7),
    POE_PORT_ATTR_INDEX(8),
    POE_PORT_ATTR_INDEX(9),
    POE_PORT_ATTR_INDEX(10),
    POE_PORT_ATTR_INDEX(11),
    POE_PORT_ATTR_INDEX(12),
    POE_PORT_ATTR_INDEX(13),
    POE_PORT_ATTR_INDEX(14),
    POE_PORT_ATTR_INDEX(15),
    POE_PORT_ATTR_INDEX(16),
    POE_PORT_ATTR_INDEX(17),
    POE_PORT_ATTR_INDEX(18),
    POE_PORT_ATTR_INDEX(19),
    POE_PORT_ATTR_INDEX(20),
    POE_PORT_ATTR_INDEX(21),
    POE_PORT_ATTR_INDEX(22),
    POE_PORT_ATTR_INDEX(23),
    POE_PORT_ATTR_INDEX(24),
    POE_PORT_ATTR_INDEX(25),
    POE_PORT_ATTR_INDEX(26),
    POE_PORT_ATTR_INDEX(27),
    POE_PORT_ATTR_INDEX(28),
    POE_PORT_ATTR_INDEX(29),
    POE_PORT_ATTR_INDEX(30),
    POE_PORT_ATTR_INDEX(31),
    POE_PORT_ATTR_INDEX(32),
    POE_PORT_ATTR_INDEX(33),
    POE_PORT_ATTR_INDEX(34),
    POE_PORT_ATTR_INDEX(35),
    POE_PORT_ATTR_INDEX(36),
    POE_PORT_ATTR_INDEX(37),
    POE_PORT_ATTR_INDEX(38),
    POE_PORT_ATTR_INDEX(39),
    POE_PORT_ATTR_INDEX(40),
    POE_PORT_ATTR_INDEX(41),
    POE_PORT_ATTR_INDEX(42),
    POE_PORT_ATTR_INDEX(43),
    POE_PORT_ATTR_INDEX(44),
    POE_PORT_ATTR_INDEX(45),
    POE_PORT_ATTR_INDEX(46),
    POE_PORT_ATTR_INDEX(47),
};

enum bsp_log_types {
    LOG_NONE,
    LOG_RW,
    LOG_READ,
    LOG_WRITE,
    LOG_SYS
};

enum bsp_log_ctrl {
    LOG_DISABLE,
    LOG_ENABLE
};


struct poe_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

struct poe_data {
    struct mutex access_lock;   /* mutex for poe access */
};

#ifdef DEBUG
#define DEBUG_PRINT(fmt, args...) \
    printk(KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
            __FILE__, __func__, __LINE__, ##args)
#else
#define DEBUG_PRINT(fmt, args...)
#endif

#define BSP_LOG_R(fmt, args...) \
    bsp_log (LOG_READ, KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
            __FILE__, __func__, __LINE__, ##args)
#define BSP_LOG_W(fmt, args...) \
    bsp_log (LOG_WRITE, KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
            __FILE__, __func__, __LINE__, ##args)
#define BSP_LOG_SYS(fmt, args...) \
    bsp_log (LOG_SYS, KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
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

#define I2C_READ_BLOCK_DATA(ret, lock, i2c_client, reg, len, val) \
{ \
    mutex_lock(lock); \
    ret = i2c_smbus_read_i2c_block_data(i2c_client, reg, len, val); \
    mutex_unlock(lock); \
}

#define I2C_WRITE_BLOCK_DATA(ret, lock, i2c_client, reg, len, val) \
{ \
    mutex_lock(lock); \
    ret = i2c_smbus_write_i2c_block_data(i2c_client, reg, len, val); \
    mutex_unlock(lock); \
}

/* common manipulation */
#define INVALID(i, min, max)    ((i < min) || (i > max) ? 1u : 0u)
#define READ_BIT(val, bit)      ((0u == (val & (1<<bit))) ? 0u : 1u)
#define SET_BIT(val, bit)       (val |= (1 << bit))
#define CLEAR_BIT(val, bit)     (val &= ~(1 << bit))
#define TOGGLE_BIT(val, bit)    (val ^= (1 << bit))
#define _BIT(n)                 (1<<(n))
#define _BIT_MASK(len)          (BIT(len)-1)

/* functions */
int bsp_log(u8 log_type, char *fmt, ...);

#endif

