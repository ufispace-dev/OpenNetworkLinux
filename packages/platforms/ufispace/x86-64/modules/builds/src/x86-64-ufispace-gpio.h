#ifndef UFISPACE_GPIO_H
#define UFISPACE_GPIO_H

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/gpio/driver.h>
#include <linux/gpio/machine.h>
#include <linux/list.h>
#include <linux/ctype.h>
#include <linux/version.h>


#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 16, 0)
#define GPIO_PERSISTENT GPIO_SLEEP_MAINTAIN_VALUE
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
#define GPIOD_FLAGS_BIT_NONEXCLUSIVE   0
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0)
#define GPIO_LOOKUP_FLAGS_DEFAULT  (GPIO_ACTIVE_HIGH | GPIO_PERSISTENT)
#endif

#define to_gpio_dev_attr(_dev_attr) \
    container_of(_dev_attr, struct gpio_dev_attr, dev_attr)

#define to_dev_attr(_attr) \
    container_of(_attr, struct device_attribute, attr);

#define GENERAL_IN(_key, _chip_hwnum, _con_id, _data_type) \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = 0, \
     .dir = GPIOD_IN, \
     .active_low = false, \
     .class = GPIO_CLASS_GENERAL, \
     .data_type = _data_type}

#define GENERAL_IN_ACTIVE_LOW(_key, _chip_hwnum, _con_id, _data_type) \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = 0, \
     .dir = GPIOD_IN, \
     .active_low = true, \
     .class = GPIO_CLASS_GENERAL, \
     .data_type = _data_type}

#define GENERAL_LOW(_key, _chip_hwnum, _con_id, _data_type) \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = 0, \
     .dir = GPIOD_OUT_LOW, \
     .active_low = false, \
     .class = GPIO_CLASS_GENERAL, \
     .data_type = _data_type}

#define GENERAL_LOW_ACTIVE_LOW(_key, _chip_hwnum, _con_id, _data_type) \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = 0, \
     .dir = GPIOD_OUT_LOW, \
     .active_low = true, \
     .class = GPIO_CLASS_GENERAL, \
     .data_type = _data_type}

#define GENERAL_HIGH(_key, _chip_hwnum, _con_id, _data_type) \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = 0, \
     .dir = GPIOD_OUT_HIGH, \
     .active_low = false, \
     .class = GPIO_CLASS_GENERAL, \
     .data_type = _data_type}

#define GENERAL_HIGH_ACTIVE_LOW(_key, _chip_hwnum, _con_id, _data_type) \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = 0, \
     .dir = GPIOD_OUT_HIGH, \
     .active_low = true, \
     .class = GPIO_CLASS_GENERAL, \
     .data_type = _data_type}

#define GENERAL_ARRAY_IN(_key, _chip_hwnum, _con_id, _idx, _data_type)  \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = _idx, \
     .dir = GPIOD_IN, \
     .active_low = false, \
     .class = GPIO_CLASS_GENERAL_ARRAY, \
     .data_type = _data_type}


#define GENERAL_ARRAY_IN_ACTIVE_LOW(_key, _chip_hwnum, _con_id, _idx, _data_type)  \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = _idx, \
     .dir = GPIOD_IN, \
     .active_low = true, \
     .class = GPIO_CLASS_GENERAL_ARRAY, \
     .data_type = _data_type}

#define GENERAL_ARRAY_LOW(_key, _chip_hwnum, _con_id, _idx, _data_type)  \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = _idx, \
     .dir = GPIOD_OUT_LOW, \
     .active_low = false, \
     .class = GPIO_CLASS_GENERAL_ARRAY, \
     .data_type = _data_type}

#define GENERAL_ARRAY_LOW_ACTIVE_LOW(_key, _chip_hwnum, _con_id, _idx, _data_type)  \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = _idx, \
     .dir = GPIOD_OUT_LOW, \
     .active_low = true, \
     .class = GPIO_CLASS_GENERAL_ARRAY, \
     .data_type = _data_type}

#define GENERAL_ARRAY_HIGH(_key, _chip_hwnum, _con_id, _idx, _data_type)  \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = _idx, \
     .dir = GPIOD_OUT_HIGH, \
     .active_low = false, \
     .class = GPIO_CLASS_GENERAL_ARRAY, \
     .data_type = _data_type}

#define GENERAL_ARRAY_HIGH_ACTIVE_LOW(_key, _chip_hwnum, _con_id, _idx, _data_type)  \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = _idx, \
     .dir = GPIOD_OUT_HIGH, \
     .active_low = true, \
     .class = GPIO_CLASS_GENERAL_ARRAY, \
     .data_type = _data_type}

#define VAL_MAP_IN(_key, _chip_hwnum, _con_id, _idx, _data_type, _user_vals, _reg_vals) \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = _idx, \
     .dir = GPIOD_IN, \
     .active_low = false, \
     .class = GPIO_CLASS_VAL_MAP, \
     .data_type = _data_type, \
     .user_vals = _user_vals, \
     .reg_vals = _reg_vals}

#define VAL_MAP_IN_ACTIVE_LOW(_key, _chip_hwnum, _con_id, _idx, _data_type, _user_vals, _reg_vals) \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = _idx, \
     .dir = GPIOD_IN, \
     .active_low = true, \
     .class = GPIO_CLASS_VAL_MAP, \
     .data_type = _data_type, \
     .user_vals = _user_vals, \
     .reg_vals = _reg_vals}

#define VAL_MAP_LOW(_key, _chip_hwnum, _con_id, _idx, _data_type, _user_vals, _reg_vals) \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = _idx, \
     .dir = GPIOD_OUT_LOW, \
     .active_low = false, \
     .class = GPIO_CLASS_VAL_MAP, \
     .data_type = _data_type, \
     .user_vals = _user_vals, \
     .reg_vals = _reg_vals}

#define VAL_MAP_LOW_ACTIVE_LOW(_key, _chip_hwnum, _con_id, _idx, _data_type, _user_vals, _reg_vals) \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = _idx, \
     .dir = GPIOD_OUT_LOW, \
     .active_low = true, \
     .class = GPIO_CLASS_VAL_MAP, \
     .data_type = _data_type, \
     .user_vals = _user_vals, \
     .reg_vals = _reg_vals}

#define VAL_MAP_HIGH(_key, _chip_hwnum, _con_id, _idx, _data_type, _user_vals, _reg_vals) \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = _idx, \
     .dir = GPIOD_OUT_HIGH, \
     .active_low = false, \
     .class = GPIO_CLASS_VAL_MAP, \
     .data_type = _data_type, \
     .user_vals = _user_vals, \
     .reg_vals = _reg_vals}

#define VAL_MAP_HIGH_ACTIVE_LOW(_key, _chip_hwnum, _con_id, _idx, _data_type, _user_vals, _reg_vals) \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = _idx, \
     .dir = GPIOD_OUT_HIGH, \
     .active_low = true, \
     .class = GPIO_CLASS_VAL_MAP, \
     .data_type = _data_type, \
     .user_vals = _user_vals, \
     .reg_vals = _reg_vals}


#define CLASS_IN(_key, _chip_hwnum, _con_id, _idx, _class, _data_type)  \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = _idx, \
     .dir = GPIOD_IN, \
     .active_low = false, \
     .class = _class, \
     .data_type = _data_type}

#define CLASS_IN_ACTIVE_LOW(_key, _chip_hwnum, _con_id, _idx, _class, _data_type)  \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = _idx, \
     .dir = GPIOD_IN, \
     .active_low = true, \
     .class = _class, \
     .data_type = _data_type}

#define CLASS_LOW(_key, _chip_hwnum, _con_id, _idx, _class, _data_type)  \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = _idx, \
     .dir = GPIOD_OUT_LOW, \
     .active_low = false, \
     .class = _class, \
     .data_type = _data_type}

#define CLASS_LOW_ACTIVE_LOW(_key, _chip_hwnum, _con_id, _idx, _class, _data_type)  \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = _idx, \
     .dir = GPIOD_OUT_LOW, \
     .active_low = true, \
     .class = _class, \
     .data_type = _data_type}

#define CLASS_HIGH(_key, _chip_hwnum, _con_id, _idx, _class, _data_type)  \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = _idx, \
     .dir = GPIOD_OUT_HIGH, \
     .active_low = false, \
     .class = _class, \
     .data_type = _data_type}

#define CLASS_HIGH_ACTIVE_LOW(_key, _chip_hwnum, _con_id, _idx, _class, _data_type)  \
    {.key = _key,  \
     .chip_hwnum = _chip_hwnum, \
     .con_id = _con_id, \
     .idx = _idx, \
     .dir = GPIOD_OUT_HIGH, \
     .active_low = true, \
     .class = _class, \
     .data_type = _data_type}

enum data_type {
    DATA_HEX,
    DATA_DEC,
    DATA_S_DEC,
    DATA_UNK,
};

enum gpio_class {
    GPIO_CLASS_GENERAL,
    GPIO_CLASS_GENERAL_ARRAY,
    GPIO_CLASS_VAL_MAP,
    GPIO_CLASS_UDF1,
    GPIO_CLASS_UDF2,
    GPIO_CLASS_UDF3,
    GPIO_CLASS_UDF4,
    GPIO_CLASS_UNK,
};

struct udf_show_store {
    ssize_t (*show)(struct device *dev,
        struct device_attribute *da, char *buf);
    ssize_t (*store)(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
};

struct gpio_dev_attr{
    struct list_head list;
    struct device_attribute dev_attr;
    unsigned long long index;
    char *con_id;
    char *reg_vals;
    char *user_vals;
    unsigned int class;
    unsigned int  data_type;
};

struct gpio_pconf {
    char *key;
    u16 chip_hwnum;
    char *con_id;
    unsigned int idx;
    unsigned long dir;
    bool active_low;
    char *reg_vals;
    char *user_vals;
    unsigned int class;
    unsigned int data_type;
};

unsigned int ufi_gpio_print_data(char *buf, unsigned int data,
    unsigned int data_type);
#endif