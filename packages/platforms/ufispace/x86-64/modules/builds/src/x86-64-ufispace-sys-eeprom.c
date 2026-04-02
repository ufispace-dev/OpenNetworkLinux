/*
 * Copyright (C) 1998, 1999  Frodo Looijaard <frodol@dds.nl> and
 *                           Philip Edelbrock <phil@netroedge.com>
 * Copyright (C) 2003 Greg Kroah-Hartman <greg@kroah.com>
 * Copyright (C) 2003 IBM Corp.
 * Copyright (C) 2004 Jean Delvare <jdelvare@suse.de>
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
 */

/* enable dev_dbg print out */
//#define DEBUG

#define __STDC_WANT_LIB_EXT1__ 1
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/version.h>
#include <linux/types.h>

#define _memset(s, c, n) memset(s, c, n)

/* Addresses to scan */
static const unsigned short normal_i2c[] = { /*0x50, 0x51, 0x52, 0x53, 0x54,
                    0x55, 0x56, 0x57,*/ I2C_CLIENT_END };

/* Define the EEPROM size constants */
#define EEPROM_SIZE         8192
#define EEPROM_ONIE_OFFSET  0
#define EEPROM_ONIE_SIZE    1024
#define EEPROM_DDR_OFFSET   2048
#define EEPROM_DDR_SIZE     2048
#define EEPROM_DEVID_OFFSET 4096
#define EEPROM_DEVID_SIZE   4096

#define SLICE_BITS      (6)
#define SLICE_SIZE      (1 << SLICE_BITS)
#define SLICE_NUM       (EEPROM_SIZE/SLICE_SIZE)

/* Each client has this additional data */
struct eeprom_data {
    struct mutex update_lock;
    /* bitfield, bit!=0 if slice is valid */
    DECLARE_BITMAP(valid, SLICE_NUM); //unsigned long valid[BITS_TO_LONGS(SLICE_NUM)];
    unsigned long last_updated[SLICE_NUM];  /* In jiffies */
    u8 data[EEPROM_SIZE];       /* Register values */
};

static void sys_eeprom_update_client(struct i2c_client *client, u8 slice)
{
    struct eeprom_data *data = i2c_get_clientdata(client);
    int i;
    int ret;
    int addr;

    mutex_lock(&data->update_lock);

    if (!test_bit(slice, data->valid) ||
        time_after(jiffies, data->last_updated[slice] + 300 * HZ)) {
        dev_dbg(&client->dev, "Starting eeprom update, slice %u\n", slice);

        addr = slice << SLICE_BITS;

        /* select the eeprom address */
        ret = i2c_smbus_write_byte_data(client, (u8)((addr >> 8) & 0xFF), (u8)(addr & 0xFF));
        if (ret < 0) {
            dev_err(&client->dev, "address set failed\n");
            goto exit;
        }

        if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_READ_BYTE)) {
            goto exit;
        }

        /* Reading a slice of data */
        for (i = 0; i < SLICE_SIZE; i++) {
            int res;
            res = i2c_smbus_read_byte(client);
            if (res < 0) {
                dev_err(&client->dev, "Failed to read byte at offset %d\n", addr + i);
                goto exit;
            }
            data->data[addr + i] = res & 0xFF;
        }

        data->last_updated[slice] = jiffies;
        set_bit(slice, data->valid);
    }
exit:
    mutex_unlock(&data->update_lock);
}

/* Generic helper function for reading from EEPROM */
static ssize_t sys_eeprom_read_helper(struct file *filp, struct kobject *kobj,
                                      char *buf, loff_t off, size_t count,
                                      loff_t base_offset, size_t file_size)
{
    struct i2c_client *client = to_i2c_client(container_of(kobj, struct device, kobj));
    struct eeprom_data *data = i2c_get_clientdata(client);
    loff_t real_off = base_offset + off;
    u8 slice;

    if (off >= file_size)
        return 0;
    if (off + count > file_size)
        count = file_size - off;
    if (count == 0)
        return 0;

    /* Only refresh slices which contain requested bytes */
    for (slice = real_off >> SLICE_BITS; slice <= (real_off + count - 1) >> SLICE_BITS; slice++) {
        sys_eeprom_update_client(client, slice);
    }

    memcpy(buf, &data->data[real_off], count);

    return count;
}

/* Generic helper function for writing to EEPROM */
static ssize_t sys_eeprom_write_helper(struct file *filp, struct kobject *kobj,
                                       char *buf, loff_t off, size_t count,
                                       loff_t base_offset, size_t file_size)
{
    struct i2c_client *client = to_i2c_client(container_of(kobj, struct device, kobj));
    struct eeprom_data *data = i2c_get_clientdata(client);
    loff_t real_off;
    int ret;
    int i;
    u8 cmd;
    u16 value16;
    u8 slice;

    if (off >= file_size)
        return -EINVAL;
    if (off + count > file_size)
        count = file_size - off;
    if (count == 0)
        return 0;

    real_off = base_offset + off;

    mutex_lock(&data->update_lock);

    /* Invalidate the cache for the slices we are writing to */
    for (slice = real_off >> SLICE_BITS; slice <= (real_off + count - 1) >> SLICE_BITS; slice++) {
        clear_bit(slice, data->valid);
    }

    for (i = 0; i < count; i++, real_off++) {
        cmd = (real_off >> 8) & 0xff;
        value16 = real_off & 0xff;
        value16 |= buf[i] << 8;
        ret = i2c_smbus_write_word_data(client, cmd, value16);

        if (ret < 0) {
            dev_err(&client->dev, "write address failed at %d\n", (int)real_off);
            mutex_unlock(&data->update_lock);
            return ret;
        }

        data->data[real_off] = buf[i];
        /* 10ms delay is typical for EEPROM page write */
        msleep(10);
    }

    mutex_unlock(&data->update_lock);
    return count;
}

/* Read/Write functions for the onie file (0-511) */
static ssize_t sys_eeprom_read_onie(struct file *filp, struct kobject *kobj,
                                   struct bin_attribute *bin_attr,
                                   char *buf, loff_t off, size_t count)
{
    return sys_eeprom_read_helper(filp, kobj, buf, off, count, EEPROM_ONIE_OFFSET, EEPROM_ONIE_SIZE);
}

static ssize_t sys_eeprom_write_onie(struct file *filp, struct kobject *kobj,
                                    struct bin_attribute *bin_attr,
                                    char *buf, loff_t off, size_t count)
{
    return sys_eeprom_write_helper(filp, kobj, buf, off, count, EEPROM_ONIE_OFFSET, EEPROM_ONIE_SIZE);
}

/* Read/Write functions for the ddr file (2048-4095) */
static ssize_t sys_eeprom_read_ddr(struct file *filp, struct kobject *kobj,
                                      struct bin_attribute *bin_attr,
                                      char *buf, loff_t off, size_t count)
{
    return sys_eeprom_read_helper(filp, kobj, buf, off, count, EEPROM_DDR_OFFSET, EEPROM_DDR_SIZE);
}

static ssize_t sys_eeprom_write_ddr(struct file *filp, struct kobject *kobj,
                                       struct bin_attribute *bin_attr,
                                       char *buf, loff_t off, size_t count)
{
    return sys_eeprom_write_helper(filp, kobj, buf, off, count, EEPROM_DDR_OFFSET, EEPROM_DDR_SIZE);
}

/* Read/Write functions for the devid file (4096-8191) */
static ssize_t sys_eeprom_read_devid(struct file *filp, struct kobject *kobj,
                                      struct bin_attribute *bin_attr,
                                      char *buf, loff_t off, size_t count)
{
    return sys_eeprom_read_helper(filp, kobj, buf, off, count, EEPROM_DEVID_OFFSET, EEPROM_DEVID_SIZE);
}

static ssize_t sys_eeprom_write_devid(struct file *filp, struct kobject *kobj,
                                       struct bin_attribute *bin_attr,
                                       char *buf, loff_t off, size_t count)
{
    return sys_eeprom_write_helper(filp, kobj, buf, off, count, EEPROM_DEVID_OFFSET, EEPROM_DEVID_SIZE);
}


/* --- sysfs attributes definitions --- */

static struct bin_attribute sys_eeprom_attr = {
    .attr = {
        .name = "eeprom",
        .mode = S_IRUGO,
    },
    .size = EEPROM_ONIE_SIZE,
    .read = sys_eeprom_read_onie,
    .write = sys_eeprom_write_onie,
};

static struct bin_attribute sys_eeprom_attr_onie = {
    .attr = {
        .name = "eeprom_onie",
        .mode = S_IRUGO,
    },
    .size = EEPROM_ONIE_SIZE,
    .read = sys_eeprom_read_onie,
    .write = sys_eeprom_write_onie,
};

static struct bin_attribute sys_eeprom_attr_ddr = {
    .attr = {
        .name = "eeprom_ddr",
        .mode = S_IRUGO,
    },
    .size = EEPROM_DDR_SIZE,
    .read = sys_eeprom_read_ddr,
    .write = sys_eeprom_write_ddr,
};

static struct bin_attribute sys_eeprom_attr_devid = {
    .attr = {
        .name = "eeprom_devid",
        .mode = S_IRUGO,
    },
    .size = EEPROM_DEVID_SIZE,
    .read = sys_eeprom_read_devid,
    .write = sys_eeprom_write_devid,
};

/* Array of all binary attributes to be created */
static struct bin_attribute *sys_eeprom_bin_attrs[] = {
    &sys_eeprom_attr,
    &sys_eeprom_attr_onie,
    &sys_eeprom_attr_ddr,
    &sys_eeprom_attr_devid,
    NULL
};


/* Return 0 if detection is successful, -ENODEV otherwise */
static int sys_eeprom_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    struct i2c_adapter *adapter = client->adapter;

    /* EDID EEPROMs are often 24C00 EEPROMs, which answer to all
       addresses 0x50-0x57, but we only care about 0x51 and 0x55. So decline
       attaching to addresses >= 0x56 on DDC buses */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 10, 0)
    if (!(adapter->class & I2C_CLASS_SPD) && client->addr >= 0x56) {
        return -ENODEV;
    }
#else
    if (client->addr >= 0x56) {
        return -ENODEV;
    }
#endif

    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_READ_BYTE)
     && !i2c_check_functionality(adapter, I2C_FUNC_SMBUS_WRITE_BYTE_DATA)) {
        return -ENODEV;
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 8, 0)
    strlcpy(info->type, "eeprom", I2C_NAME_SIZE);
#else
    strscpy(info->type, "eeprom", I2C_NAME_SIZE);
#endif

    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0)
static int sys_eeprom_probe(struct i2c_client *client,
            const struct i2c_device_id *id)
#else
static int sys_eeprom_probe(struct i2c_client *client)
#endif
{
    struct eeprom_data *data;
    int err;
    int i;

    data = kzalloc(sizeof(struct eeprom_data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

#ifdef __STDC_LIB_EXT1__
    memset_s(data->data, EEPROM_SIZE, 0xff, EEPROM_SIZE);
#else
    _memset(data->data, 0xff, EEPROM_SIZE);
#endif
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    /* Create all the sysfs eeprom files */
    for (i = 0; sys_eeprom_bin_attrs[i]; i++) {
        err = sysfs_create_bin_file(&client->dev.kobj, sys_eeprom_bin_attrs[i]);
        if (err) {
            dev_err(&client->dev, "Failed to create bin file %s\n",
                    sys_eeprom_bin_attrs[i]->attr.name);
            /* Clean up already created files */
            for (i--; i >= 0; i--)
                sysfs_remove_bin_file(&client->dev.kobj, sys_eeprom_bin_attrs[i]);
            kfree(data);
            return err;
        }
    }

    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
static int sys_eeprom_remove(struct i2c_client *client)
#else
static void sys_eeprom_remove(struct i2c_client *client)
#endif
{
    int i;
    /* Remove all sysfs files */
    for (i = 0; sys_eeprom_bin_attrs[i]; i++) {
        sysfs_remove_bin_file(&client->dev.kobj, sys_eeprom_bin_attrs[i]);
    }
    kfree(i2c_get_clientdata(client));

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
    return 0;
#endif
}

static const struct i2c_device_id sys_eeprom_id[] = {
    { "sys_eeprom", 0 },
    { }
};

static struct i2c_driver sys_eeprom_driver = {
    .driver = {
        .name   = "sys_eeprom",
    },
    .probe      = sys_eeprom_probe,
    .remove     = sys_eeprom_remove,
    .id_table   = sys_eeprom_id,

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 8, 0)
    .class      = I2C_CLASS_DDC | I2C_CLASS_SPD,
#elif LINUX_VERSION_CODE < KERNEL_VERSION(6, 10, 0)
    .class      = I2C_CLASS_SPD,
#else
    .class      = 0,
#endif
    .detect     = sys_eeprom_detect,
    .address_list   = normal_i2c,
};

module_i2c_driver(sys_eeprom_driver);

MODULE_AUTHOR("Jason Tsai <jason.cy.tsai@ufispace.com>");
MODULE_DESCRIPTION("UfiSpace System EEPROM driver");
MODULE_LICENSE("GPL");
