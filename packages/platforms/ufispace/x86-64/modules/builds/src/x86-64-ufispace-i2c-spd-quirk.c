/*
 * Copyright (C) 2026 Ufispace Technology Corporation.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 * A I2C SPD quirks kernel driver for the Ufispace platform.
 *
 * Quirk: Prevent i2c-i801 from monopolizing SPD addresses.
 *
 * Upon loading, the i2c-i801 driver calls the i2c_register_spd API (within 
 * i2c-smbus), which forcibly reserves the 0x50 - 0x5F address range as 
 * "spd" devices. This behavior interferes with our platform's 
 * initialization process.
 *
 * This driver detects if any child device under the i2c-i801 adapter within 
 * the 0x50 - 0x5F range is named "ee1004/spd/spd5118". If found, it 
 * forcibly unregisters the device to free up the address space for our 
 * platform's drivers, regardless of the actual hardware presence.
 */

#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#define DRVNAME  "x86_64_ufispace_i2c_spd_quirk"

static struct mutex i2c_spd_quirk_lock;

static int _check_i801(struct device *dev, void *data)
{
    struct i2c_adapter **found_adap = data;
    struct i2c_adapter *adapter = i2c_verify_adapter(dev);

    if (adapter && strstr(adapter->name, "801")) {
        *found_adap = adapter;
        return 1;
    }
    return 0;
}

static int _unregister_spd(struct device *dev, void *data)
{
    struct i2c_adapter *adap = NULL;
    struct i2c_client *user_client, *next;
    struct i2c_client *client = i2c_verify_client(dev);
    bool is_userspace = false;

    if (!client)
        return 0;

    adap = client->adapter;
    if(client->addr >= 0x50 && client->addr <= 0x5F) {
        if (strcmp(client->name, "ee1004") == 0 ||
            strcmp(client->name, "spd") == 0 ||
            strcmp(client->name, "spd5118") == 0) {

            mutex_lock_nested(&adap->userspace_clients_lock,
                i2c_adapter_depth(adap));

            list_for_each_entry_safe(user_client, next, &adap->userspace_clients,
                detected) {
                if (user_client->addr == client->addr) {
                    is_userspace = true;
                    break;
                }
            }

            if (is_userspace) {
                mutex_unlock(&adap->userspace_clients_lock);
                return 0;
            }

            pr_info("%s %s(#%d): Found unwanted %s at 0x%02x on adapter %s, removing...\n",
                DRVNAME, __func__, __LINE__, client->name, client->addr, client->adapter->name);

            i2c_unregister_device(client);
            mutex_unlock(&adap->userspace_clients_lock);

            return 1;
        }
    }
    return 0;
}

static void i2c_unregister_spd(void)
{
    int res = 0;
    struct i2c_adapter *adapter = NULL;
    res = i2c_for_each_dev(&adapter, _check_i801);

    if (!res) {
        pr_info("%s %s(#%d): Could not find i2c-i801 adapter\n",
            DRVNAME, __func__, __LINE__);
        return;
    }

    pr_info("%s %s(#%d): Found i801 adapter: %s, checking for forced spd...\n",
        DRVNAME, __func__, __LINE__, adapter->name);
    while (device_for_each_child(&adapter->dev, NULL, _unregister_spd)){
        ;
    }
}

static ssize_t do_clean_store(struct device_driver *driver,
    const char *buf, size_t count)
{
    int rv = 0;
    int value = 0;
    mutex_lock(&i2c_spd_quirk_lock);

    if (kstrtoint(buf, 0, &value) < 0 || value != 1) {

        rv = -EINVAL;
        pr_warn("%s %s(#%d): Invalid value(only support 1)\n",
            DRVNAME, __func__, __LINE__);
        goto done;
    }

    i2c_unregister_spd();

    mutex_unlock(&i2c_spd_quirk_lock);
    return count;
done:
    mutex_unlock(&i2c_spd_quirk_lock);
    return rv;
}

static DRIVER_ATTR_WO(do_clean);
static struct attribute *i2c_spd_quirk_attrs[] = {
    &driver_attr_do_clean.attr,
    NULL
};

ATTRIBUTE_GROUPS(i2c_spd_quirk);

static struct platform_driver i2c_spd_quirk_drv = {
    .driver     = {
        .name   = DRVNAME,
        .groups = i2c_spd_quirk_groups,
    },
};

static int __init i2c_spd_quirk_init(void)
{
    int err = 0;
    mutex_init(&i2c_spd_quirk_lock);
    i2c_unregister_spd();
    err = platform_driver_register(&i2c_spd_quirk_drv);
    if (err) {
        pr_info("%s %s(#%d): Platform_driver_register failed(%d)\n",
            DRVNAME, __func__, __LINE__, err);
        mutex_destroy(&i2c_spd_quirk_lock);
    }
    return err;
}

static void __exit i2c_spd_quirk_exit(void)
{
    platform_driver_unregister(&i2c_spd_quirk_drv);
    mutex_destroy(&i2c_spd_quirk_lock);
}

MODULE_AUTHOR("Nonodark Huang <nonodark.huang@ufispace.com>");
MODULE_DESCRIPTION("x86_64_ufispace_i2c_spd_quirk driver");
MODULE_VERSION("0.0.1");
MODULE_LICENSE("GPL");
module_init(i2c_spd_quirk_init);
module_exit(i2c_spd_quirk_exit);
