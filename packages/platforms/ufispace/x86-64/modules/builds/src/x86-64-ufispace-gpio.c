/*
 * A platform GPIO driver for the ufispace
 *
 * Copyright (C) 2026 Ufispace Technology Corporation.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "x86-64-ufispace-gpio.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 18, 0)
#include "x86-64-ufispace-overflow.h"
#endif

#define _DEVICE_ATTR(_name)     \
    &dev_attr_##_name

#define _BIN_ATTR(_name)     \
    &bin_attr_##_name

#define DEV_ATTR(_name)  (&dev_attr_##_name.attr)

#define DRIVER_NAME "x86_64_ufispace_gpio"
#define GROUP_NAME "gpio_function"

#define GPIO_NAME_MAX          31
#define VALUE_STRING_MAX       255
#define VALUE_ARRAY_MAX        31
#define GPIO_MAX_NUM           1023
#define DUMP_TABLE_MAX_SIZE (PAGE_SIZE * 60)

enum gpio_table_type {
    TABLE_TYPE_GPIO,
    TABLE_TYPE_SYSFS
};

enum gpio_table_act {
    GPIO_TABLE_ADD = 1,
    GPIO_TABLE_DELETE
};

enum gpio_attr_conf_flags {
    GPIO_ATTR_CONF_KEY           = (1 << 0),
    GPIO_ATTR_CONF_CHIP_HWNUM    = (1 << 1),
    GPIO_ATTR_CONF_CHIP_CON_ID   = (1 << 3),
    GPIO_ATTR_CONF_CHIP_IDX      = (1 << 4),
    GPIO_ATTR_CONF_CHIP_DIR      = (1 << 5),
    GPIO_ATTR_CONF_ACTIVE_LOW    = (1 << 6),
    GPIO_ATTR_CONF_VAL_MAP       = (1 << 7),
    GPIO_ATTR_CONF_CLASS         = (1 << 8),
    GPIO_ATTR_CONF_DATA_TYPE     = (1 << 9),
};

struct gpio_config_node {
    char key[GPIO_NAME_MAX+1];
    int *chip_hwnums;
    char con_id[GPIO_NAME_MAX+1];
    unsigned int* idxs;
    unsigned long dir;
    bool active_low;
    char reg_vals[VALUE_STRING_MAX+1];
    char user_vals[VALUE_STRING_MAX+1];
    unsigned int class;
    unsigned int data_type;
    unsigned long attr_flags;
};

struct gpio_node {
    struct list_head list;
    char key[GPIO_NAME_MAX+1];
    u16 chip_hwnum;
    char con_id[GPIO_NAME_MAX+1];
    unsigned int idx;
    unsigned long dir;
    bool active_low;
    char reg_vals[VALUE_STRING_MAX+1];
    char user_vals[VALUE_STRING_MAX+1];
    unsigned int class;
    unsigned int data_type;
};

struct gpio_data_s {
    struct gpio_node node_lists;
    unsigned int table_used;
    bool is_enabled;
    bool is_kernel_use;
    struct gpio_dev_attr gpio_attr_lists;
    struct mutex access_lock;
    enum gpio_table_type dump_table_type;
    struct attribute_group grp;
    struct gpiod_lookup_table *lookup_table;
};

static int update_gpio_chip_label(struct device *dev);
static int copy_lower_str(char *dstr, char *sstr, ssize_t len);
static unsigned int get_shift(u8 mask);
static unsigned int mask_and_shift(u8 val, u8 mask);
static int reg_val_to_user_val(struct device *dev, char *reg_vals,
    char *user_vals, int reg_val, int *user_val);
static int user_val_to_reg_val(struct device *dev, char *reg_vals,
    char *user_vals, int user_val, int *reg_val);
static int gpio_config_parse(struct device *dev, const char *buf,
    enum gpio_table_act act_flags);
static int gpio_table_node_add(struct device *dev,
    struct gpio_config_node *conf_node);
static int gpio_table_node_delete(struct device *dev,
    struct gpio_config_node *conf_node);
static int gpio_table_clear(struct device *dev);
static int register_gpio_lookup_table(struct device *dev);
static int unregister_gpio_lookup_table(struct device *dev);
static int apply_gpio_default_conf(struct device *dev, unsigned long dir);
static int register_gpio_sysfs(struct device *dev);
static int unregister_gpio_sysfs(struct device *dev);
static ssize_t new_device_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t delete_device_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t dump_gpio_table(struct device *dev, char *buf,
    loff_t off, size_t count);
static ssize_t dump_sysfs_table(struct device *dev, char *buf,
    loff_t off, size_t count);
static ssize_t dump_table_read(struct file *filp, struct kobject *kobj,
        struct bin_attribute *attr,
        char *buf, loff_t off, size_t count);
static ssize_t dump_table_write(struct file *filp, struct kobject *kobj,
        struct bin_attribute *attr,
        char *buf, loff_t off, size_t count);
static ssize_t class_general_show(struct device *dev,
    struct device_attribute *da, char *buf);
static ssize_t class_general_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t class_general_array_show(struct device *dev,
    struct device_attribute *da, char *buf);
static ssize_t class_general_array_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t class_val_map_show(struct device *dev,
    struct device_attribute *da, char *buf);
static ssize_t class_val_map_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t class_udf_show(struct device *dev,
    struct device_attribute *da, char *buf);
static ssize_t class_udf_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
int register_gpio(struct device * dev, struct gpio_pconf *table);
void unregister_gpio(struct device * dev);

int gpio_init(void);
void gpio_exit(void);

static DEVICE_ATTR_WO(new_device);
static DEVICE_ATTR_WO(delete_device);
static BIN_ATTR_RW(dump_table, DUMP_TABLE_MAX_SIZE);

struct udf_show_store ufi_gpio_show_store_udf1 = {.show=NULL, .store=NULL};
struct udf_show_store ufi_gpio_show_store_udf2 = {.show=NULL, .store=NULL};
struct udf_show_store ufi_gpio_show_store_udf3 = {.show=NULL, .store=NULL};
struct udf_show_store ufi_gpio_show_store_udf4 = {.show=NULL, .store=NULL};

EXPORT_SYMBOL(ufi_gpio_show_store_udf1);
EXPORT_SYMBOL(ufi_gpio_show_store_udf2);
EXPORT_SYMBOL(ufi_gpio_show_store_udf3);
EXPORT_SYMBOL(ufi_gpio_show_store_udf4);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
// Copy from kernel 5.12 lib/cmdline.c
char *next_arg(char *args, char **param, char **val)
{
    unsigned int i, equals = 0;
    int in_quote = 0, quoted = 0;

    if (*args == '"') {
        args++;
        in_quote = 1;
        quoted = 1;
    }

    for (i = 0; args[i]; i++) {
        if (isspace(args[i]) && !in_quote)
            break;
        if (equals == 0) {
            if (args[i] == '=')
                equals = i;
        }
        if (args[i] == '"')
            in_quote = !in_quote;
    }

    *param = args;
    if (!equals)
        *val = NULL;
    else {
        args[equals] = '\0';
        *val = args + equals + 1;

        /* Don't include quotes in value. */
        if (**val == '"') {
            (*val)++;
            if (args[i-1] == '"')
                args[i-1] = '\0';
        }
    }
    if (quoted && args[i-1] == '"')
        args[i-1] = '\0';

    if (args[i]) {
        args[i] = '\0';
        args += i + 1;
    } else
        args += i;

    /* Chew up trailing spaces. */
    return skip_spaces(args);
}

#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0)

/*
 * In kernels prior to 5.1, the gpio-pca953x driver assigns the GPIO chip label
 * using the I2C device match name rather than the I2C device name.
 * This may lead to duplicate GPIO chip labels and subsequent GPIO lookup
 * failures. To mitigate this issue, we update the GPIO chip label accordingly.
 */

static int gpiochip_not_match_dev_name_label(struct gpio_chip *chip,
    void *data)
{
    if(!!(chip->parent)) {
       return strcmp(chip->label, dev_name(chip->parent));
    }
    return 0;
}

static int update_gpio_chip_label(struct device *dev)
{
    struct gpio_chip* gc = NULL;
    do {
        gc = gpiochip_find(NULL, gpiochip_not_match_dev_name_label);
        if(gc) {
            dev_dbg(dev, "Update gpio chip label(%s) to device name(%s)\n",
                gc->label, dev_name(gc->parent));
            gc->label = dev_name(gc->parent);
        }
    } while(!!gc);
    return 0;
}
#else

static int update_gpio_chip_label(struct device *dev)
{
    return 0;
}
#endif

static int copy_lower_str(char *dstr, char *sstr, ssize_t len)
{
    if(!dstr || !sstr) {
        return -EINVAL;
    }
    memcpy(dstr, sstr, len);
    while (*dstr) {
        *dstr = tolower(*dstr);
        dstr++;
    }
    return 0;
}

static unsigned int get_shift(u8 mask)
{
    int i=0, mask_one=1;

    for(i=0; i<8; ++i) {
        if ((mask & mask_one) == 1)
            return i;
        else
            mask >>= 1;
    }

    return -1;
}

static unsigned int mask_and_shift(u8 val, u8 mask)
{
    int shift=0;

    shift = get_shift(mask);

    return (val & mask) >> shift;
}

unsigned int ufi_gpio_print_data(char *buf, unsigned int data,
    unsigned int data_type)
{
    if(buf == NULL) {
        return -1;
    }

    if(data_type == DATA_HEX) {
        return sprintf(buf, "0x%02x\n", data);
    } else if(data_type == DATA_DEC) {
        return sprintf(buf, "%u\n", data);
    } else {
        return -1;
    }
    return 0;
}

EXPORT_SYMBOL(ufi_gpio_print_data);

static int reg_val_to_user_val(struct device *dev, char *reg_vals,
    char *user_vals, int reg_val, int *user_val)
{
    unsigned int i = 0;
    int rv = 0;
    int *reg_val_list = NULL;
    int *user_val_list = NULL;
    reg_val_list =
        devm_kzalloc(dev, sizeof(int) * (VALUE_ARRAY_MAX + 1), GFP_KERNEL);
    if (!reg_val_list) {
        dev_dbg(dev, "Failed to reallocate memory for reg_val_list.\n");
        rv = -ENOSPC;
        goto done;
    }

    get_options(reg_vals, VALUE_ARRAY_MAX, reg_val_list);

    user_val_list =
        devm_kzalloc(dev, sizeof(int) * (VALUE_ARRAY_MAX + 1), GFP_KERNEL);
    if (!user_val_list) {
        dev_dbg(dev, "Failed to reallocate memory for user_val_list.\n");
        rv = -ENOSPC;
        goto done;
    }

    get_options(user_vals, VALUE_ARRAY_MAX, user_val_list);

    if(reg_val_list[0] != user_val_list[0]) {
            dev_dbg(dev, "The number of reg_vals (%d) and user_vals "
                "(%d) do not match",
                reg_val_list[0], user_val_list[0]);
            rv = -EINVAL;
            goto done;
    }

    for(i = 1; i <= reg_val_list[0]; i++) {
        if(reg_val == reg_val_list[i]) {
            *user_val = user_val_list[i];
            goto done;
        }
    }

    rv = -ENODATA;

done:
    devm_kfree(dev, reg_val_list);
    devm_kfree(dev, user_val_list);
    return rv;
}

static int user_val_to_reg_val(struct device *dev, char *reg_vals,
    char *user_vals, int user_val, int *reg_val)
{
    unsigned int i = 0;
    int rv = 0;
    int *reg_val_list = NULL;
    int *user_val_list = NULL;
    reg_val_list =
        devm_kzalloc(dev, sizeof(int) * (VALUE_ARRAY_MAX + 1), GFP_KERNEL);
    if (!reg_val_list) {
        dev_dbg(dev, "Failed to reallocate memory for reg_val_list.\n");
        rv = -ENOSPC;
        goto done;
    }

    get_options(reg_vals, VALUE_ARRAY_MAX, reg_val_list);

    user_val_list =
        devm_kzalloc(dev, sizeof(int) * (VALUE_ARRAY_MAX + 1), GFP_KERNEL);
    if (!user_val_list) {
        dev_dbg(dev, "Failed to reallocate memory for user_val_list.\n");
        rv = -ENOSPC;
        goto done;
    }

    get_options(user_vals, VALUE_ARRAY_MAX, user_val_list);

    if(reg_val_list[0] != user_val_list[0]) {
            dev_dbg(dev, "The number of reg_vals (%d) and user_vals "
                "(%d) do not match",
                reg_val_list[0], user_val_list[0]);
            rv = -EINVAL;
            goto done;
    }

    for(i = 1; i <= user_val_list[0]; i++) {
        if(user_val == user_val_list[i]) {
            *reg_val = reg_val_list[i];
            goto done;
        }
    }

    rv = -ENODATA;

done:
    devm_kfree(dev, reg_val_list);
    devm_kfree(dev, user_val_list);
    return rv;
}

static int gpio_config_parse(struct device *dev, const char *buf,
    enum gpio_table_act act_flags)
{
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) dev_get_drvdata(dev);
    int rv = 0;
    char *args = skip_spaces(buf);
    char *key, *value;
    struct gpio_config_node conf_node = {0};

    if(!gpio_data) {
        return -EINVAL;
    }

    conf_node.chip_hwnums =
        devm_kzalloc(dev, sizeof(int) * (GPIO_MAX_NUM + 1), GFP_KERNEL);
    if (!conf_node.chip_hwnums) {
        dev_dbg(dev, "Failed to reallocate memory for chip_hwnums.\n");
        rv = -ENOSPC;
        goto done;
    }

    conf_node.idxs =
        devm_kzalloc(dev, sizeof(int) * (GPIO_MAX_NUM + 1), GFP_KERNEL);
    if (!conf_node.idxs) {
        dev_dbg(dev, "Failed to reallocate memory for idxs.\n");
        rv = -ENOSPC;
        goto done;
    }

    while (*args) {
        args = next_arg(args, &key, &value);
        if(!strcmp(key, "key") && !!(value)) {
            memcpy(conf_node.key, value, GPIO_NAME_MAX);
            conf_node.attr_flags |= GPIO_ATTR_CONF_KEY;
        } else if(!strcmp(key, "chip_hwnum") && !!(value)){
            get_options(value, GPIO_MAX_NUM, conf_node.chip_hwnums);
            if(conf_node.chip_hwnums[0] == 0) {
                dev_dbg(dev, "The chip_hwnum is invalid.\n");
                rv = -EINVAL;
                goto done;
            } else if(gpio_data->table_used + conf_node.chip_hwnums[0]
                > GPIO_MAX_NUM) {
                dev_dbg(dev, "Requested chip_hwnums resources (%d) + "
                    "used (%d) will exceed the maximum allowed (%d).\n",
                    conf_node.chip_hwnums[0],
                    gpio_data->table_used, GPIO_MAX_NUM);
                rv = -ENOSPC;
                goto done;
            }
            conf_node.attr_flags |= GPIO_ATTR_CONF_CHIP_HWNUM;
        } else if(!strcmp(key, "con_id") && !!(value)){
            strncpy(conf_node.con_id, value, GPIO_NAME_MAX);
            conf_node.attr_flags |= GPIO_ATTR_CONF_CHIP_CON_ID;
        } else if(!strcmp(key, "idx") && !!(value)){
            get_options(value, GPIO_MAX_NUM, conf_node.idxs);
            if(conf_node.idxs[0] == 0) {
                dev_dbg(dev, "The idx is invalid \n");
                rv =  -EINVAL;
                goto done;
            } else if(gpio_data->table_used + conf_node.idxs[0]
                > GPIO_MAX_NUM) {
                dev_dbg(dev, "Requested idxs resources (%d) + used (%d) will "
                    "exceed the maximum allowed (%d).\n",
                    conf_node.idxs[0], gpio_data->table_used, GPIO_MAX_NUM);
                rv = -ENOSPC;
                goto done;
            }
            conf_node.attr_flags |= GPIO_ATTR_CONF_CHIP_IDX;
        } else if(!strcmp(key, "dir") && !!(value)){
            if(!strcmp(value, "in")) {
                conf_node.dir = (GPIOD_IN);
            } else if(!strcmp(value, "low")){
                conf_node.dir = (GPIOD_OUT_LOW);
            } else if(!strcmp(value, "high")){
                conf_node.dir = (GPIOD_OUT_HIGH);
            } else {
                conf_node.dir = (GPIOD_IN);
            }
            conf_node.attr_flags |= GPIO_ATTR_CONF_CHIP_DIR;
        } else if(!strcmp(key, "active_low") && !!(value)){
            if(!strcmp(value, "1") || !strcmp(value, "true")) {
                conf_node.active_low = true;
                conf_node.attr_flags |= GPIO_ATTR_CONF_ACTIVE_LOW;
            }
        } else if(!strcmp(key, "class") && !!(value)){
            if(!strcmp(value, "valmap")) {
                conf_node.class = GPIO_CLASS_VAL_MAP;
            } else if(!strcmp(value, "udf1")) {
                conf_node.class = GPIO_CLASS_UDF1;
            } else if(!strcmp(value, "udf2")) {
                conf_node.class = GPIO_CLASS_UDF2;
            } else if(!strcmp(value, "udf3")) {
                conf_node.class = GPIO_CLASS_UDF3;
            } else if(!strcmp(value, "udf4")) {
                conf_node.class = GPIO_CLASS_UDF4;
            }else{
                conf_node.class = GPIO_CLASS_UNK;
            }
            conf_node.attr_flags |= GPIO_ATTR_CONF_CLASS;
        } else if(!strcmp(key, "reg_vals") && !!(value)){
            memcpy(conf_node.reg_vals, value, VALUE_STRING_MAX);
            conf_node.attr_flags |= GPIO_ATTR_CONF_VAL_MAP;
        } else if(!strcmp(key, "user_vals") && !!(value)){
            memcpy(conf_node.user_vals, value, VALUE_STRING_MAX);
            conf_node.attr_flags |= GPIO_ATTR_CONF_VAL_MAP;
        } else if(!strcmp(key, "data_type") && !!(value)){
            if(!strcmp(value, "dec")) {
                conf_node.data_type = DATA_DEC;
            } else if(!strcmp(value, "hex")) {
                conf_node.data_type = DATA_HEX;
            }else{
                conf_node.data_type = DATA_HEX;
            }
            conf_node.attr_flags |= GPIO_ATTR_CONF_DATA_TYPE;
        } else {
            continue;
        }
    }

    if(act_flags == GPIO_TABLE_ADD) {
        rv = gpio_table_node_add(dev, &conf_node);
    } else {
        rv = gpio_table_node_delete(dev, &conf_node);
    }
done:
    devm_kfree(dev, conf_node.chip_hwnums);
    devm_kfree(dev, conf_node.idxs);
    return rv;
}

static int gpio_table_node_add(struct device *dev,
    struct gpio_config_node *conf_node)
{
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) dev_get_drvdata(dev);
    int rv = 0;
    unsigned int i;
    struct gpio_node *node = NULL;
    struct gpio_node *tmp_node = NULL;
    struct gpio_node new_node_lists;
    INIT_LIST_HEAD(&new_node_lists.list);

    if(!gpio_data) {
        rv = -EINVAL;
        goto done;
    }

    if(!(conf_node->attr_flags & GPIO_ATTR_CONF_KEY) ||
        !(conf_node->attr_flags & GPIO_ATTR_CONF_CHIP_HWNUM)) {
        dev_dbg(dev, "Both key and chip_hwnum are required.\n");
        rv = -EINVAL;
        goto done;
    }

    if(!(conf_node->attr_flags & GPIO_ATTR_CONF_CHIP_IDX)) {
        conf_node->idxs[0] = conf_node->chip_hwnums[0];
        for(i=1;i<=conf_node->idxs[0];i++) {
            conf_node->idxs[i] = i-1;
        }
    } else if(conf_node->idxs[0] != conf_node->chip_hwnums[0]) {
        dev_dbg(dev, "The number of chip_hwnum (%d) and idx (%d) do not match",
            conf_node->chip_hwnums[0], conf_node->idxs[0]);
        rv = -EINVAL;
        goto done;
    }

    for(i=1; i<=conf_node->chip_hwnums[0];i++) {
        node = devm_kzalloc(dev, sizeof(struct gpio_node), GFP_KERNEL);
        if (!node) {
            dev_dbg(dev, "Failed to reallocate memory for node.\n");
            rv = -ENOSPC;
            goto free_node;
        }

        memcpy(node->key, conf_node->key, GPIO_NAME_MAX);
        node->chip_hwnum = conf_node->chip_hwnums[i];
        copy_lower_str(node->con_id, conf_node->con_id, GPIO_NAME_MAX);
        node->idx = conf_node->idxs[i];

        if(conf_node->attr_flags & GPIO_ATTR_CONF_CHIP_DIR) {
            node->dir = conf_node->dir;
        } else {
            node->dir = (GPIOD_IN);
        }
        
        if(conf_node->attr_flags & GPIO_ATTR_CONF_ACTIVE_LOW) {
            node->active_low = conf_node->active_low;
        } else {
            node->active_low = false;
        }

        if(conf_node->attr_flags & GPIO_ATTR_CONF_CLASS) {
            node->class = conf_node->class;
        } else {
            node->class = GPIO_CLASS_GENERAL;
        }

        if(conf_node->attr_flags & GPIO_ATTR_CONF_DATA_TYPE) {
            node->data_type = conf_node->data_type;
        } else {
            node->data_type = DATA_DEC;
        }

        if(conf_node->attr_flags & GPIO_ATTR_CONF_VAL_MAP) {
            memcpy(node->reg_vals, conf_node->reg_vals, VALUE_STRING_MAX);
            memcpy(node->user_vals, conf_node->user_vals, VALUE_STRING_MAX);
        }
        list_add_tail(&node->list, &new_node_lists.list);
    }
    list_splice_tail_init(&new_node_lists.list,&gpio_data->node_lists.list);
    gpio_data->table_used += conf_node->chip_hwnums[0];
    goto done;

free_node:
    list_for_each_entry_safe(node, tmp_node, &new_node_lists.list, list) {
        list_del(&node->list);
        devm_kfree(dev, node);
    }
done:
    return rv;
}

static int gpio_table_node_delete(struct device *dev,
    struct gpio_config_node *conf_node)
{
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) dev_get_drvdata(dev);
    int rv = 0;
    unsigned int i = 0, j = 0;
    struct gpio_node *node = NULL;
    struct gpio_node *tmp_node = NULL;
    bool need_del = false;

    if(!gpio_data) {
        rv = -EINVAL;
        goto done;
    }

    if(!(conf_node->attr_flags & GPIO_ATTR_CONF_KEY) &&
        !(conf_node->attr_flags & GPIO_ATTR_CONF_CHIP_CON_ID)) {
        dev_dbg(dev, "Either key or con_id is mandatory.\n");
        rv = -EINVAL;
        goto done;
    }

    list_for_each_entry_safe(node, tmp_node,
        &gpio_data->node_lists.list, list) {
        need_del = false;
        if(conf_node->attr_flags & GPIO_ATTR_CONF_KEY) {
            if(conf_node->attr_flags & GPIO_ATTR_CONF_CHIP_HWNUM) {
                for(j=1; j<=conf_node->chip_hwnums[0];j++) {
                    if(!strcmp(node->key, conf_node->key) &&
                        (node->chip_hwnum == conf_node->chip_hwnums[j])) {
                        need_del = true;
                        break;
                    }
                }
            } else {
                if(!strcmp(node->key, conf_node->key)) {
                    need_del = true;
                }
            }
        } else if (conf_node->attr_flags & GPIO_ATTR_CONF_CHIP_CON_ID) {
            if(conf_node->attr_flags & GPIO_ATTR_CONF_CHIP_IDX) {
                for(j=1; j<=conf_node->idxs[0];j++) {
                    if(!strcmp(node->con_id, conf_node->con_id) &&
                        (node->idx == conf_node->idxs[j])) {
                        need_del = true;
                        break;
                    }
                }
            } else {
                if(!strcmp(node->con_id, conf_node->con_id) ) {
                    need_del = true;
                }
            }
        }
        if(need_del) {
            i++;
            list_del(&node->list);
            devm_kfree(dev, node);
        }
    }

    gpio_data->table_used -= i;
done:
    return rv;
}

static int gpio_table_clear(struct device *dev)
{
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) dev_get_drvdata(dev);
    int rv = 0, i = 0;
    struct gpio_node *node = NULL;
    struct gpio_node *tmp_node = NULL;

    if(!gpio_data) {
        return -EINVAL;
    }

    list_for_each_entry_safe(node, tmp_node,
        &gpio_data->node_lists.list, list) {
        i++;
        list_del(&node->list);
        devm_kfree(dev, node);
    }
    gpio_data->table_used -= i;
    return rv;
}

static int register_gpio_lookup_table(struct device *dev)
{
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) dev_get_drvdata(dev);
    struct gpio_node *node = NULL;
    struct gpio_node *tmp_node = NULL;
    unsigned int i = 0;

    if(!gpio_data) {
        return -EINVAL;
    }

    gpio_data->lookup_table =
        devm_kzalloc(dev,
            struct_size(gpio_data->lookup_table,
                table, gpio_data->table_used + 1),
            GFP_KERNEL);

    if (!gpio_data->lookup_table) {
        dev_dbg(dev, "Failed to allocate memory for lookup_table.\n");
        return -ENOMEM;
    }
    gpio_data->lookup_table->dev_id = DRIVER_NAME;
    list_for_each_entry_safe(node, tmp_node,
        &gpio_data->node_lists.list, list) {
        if(i+1 > gpio_data->table_used) {
            dev_dbg(dev, "Failed! The insert node index (%d) "
                "exceeds the table size (%d)!\n",
                i, gpio_data->table_used + 1);
            break;
        }
        dev_dbg(dev, "Add table entry key(%s) chip_hwnum(%d) con_id(%s) "
            "idx(%d) dir(%lu) active_low(%d)\n",
            node->key, node->chip_hwnum, node->con_id,
            node->idx, node->dir, node->active_low);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 12, 0)
        if(node->active_low) {
            gpio_data->lookup_table->table[i] =
                (struct gpiod_lookup) GPIO_LOOKUP_IDX(node->key,
                    node->chip_hwnum, node->con_id, node->idx,
                    (GPIO_ACTIVE_LOW | GPIO_PERSISTENT));
        } else {
            gpio_data->lookup_table->table[i] =
                (struct gpiod_lookup) GPIO_LOOKUP_IDX(node->key,
                    node->chip_hwnum, node->con_id, node->idx,
                    GPIO_LOOKUP_FLAGS_DEFAULT);
        }
#else
        if(node->active_low) {
            gpio_data->lookup_table->table[i] =
                GPIO_LOOKUP_IDX(node->key, node->chip_hwnum, node->con_id,
                    node->idx, (GPIO_ACTIVE_LOW | GPIO_PERSISTENT));
        } else {
            gpio_data->lookup_table->table[i] =
                GPIO_LOOKUP_IDX(node->key, node->chip_hwnum, node->con_id,
                    node->idx, GPIO_LOOKUP_FLAGS_DEFAULT);
        }
#endif
        i++;
    }
    gpiod_add_lookup_table(gpio_data->lookup_table);
    gpio_data->is_enabled = true;
    return 0;
}

static int unregister_gpio_lookup_table(struct device *dev)
{
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) dev_get_drvdata(dev);

    if(!gpio_data) {
        return -EINVAL;
    }

    if(gpio_data->lookup_table) {
        gpiod_remove_lookup_table(gpio_data->lookup_table);
        devm_kfree(dev, gpio_data->lookup_table);
        gpio_data->lookup_table = NULL;
    }
    gpio_data->is_enabled = false;
    return 0;
}

static int apply_gpio_default_conf(struct device *dev, unsigned long dir)
{
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) dev_get_drvdata(dev);
    struct gpio_node *node = NULL;
    struct gpio_node *tmp_node = NULL;
    struct gpio_desc *desc = NULL;

    if(!gpio_data) {
        return -EINVAL;
    }

    list_for_each_entry_safe(node, tmp_node,
        &gpio_data->node_lists.list, list) {
        dev_dbg(dev, "Apply default gpio config key(%s) chip_hwnum(%d) "
            "con_id(%s) idx(%d) dir(%lu) force_dir(%lu) active_low(%d)\n",
            node->key, node->chip_hwnum, node->con_id, node->idx, node->dir,
            dir, node->active_low);
        if(dir == GPIOD_ASIS) {
            desc =
                devm_gpiod_get_index(dev, node->con_id, node->idx, node->dir);
        } else {
            desc = devm_gpiod_get_index(dev, node->con_id, node->idx, dir);
        }
        if (IS_ERR(desc)) {
            dev_dbg(dev, "Failed to apply gpio default config key(%s) "
                "chip_hwnum(%d) con_id(%s) idx(%d) dir(%lu) "
                "force_dir(%lu) active_low(%d)\n",
                node->key, node->chip_hwnum, node->con_id, node->idx,
                node->dir, dir, node->active_low);
            continue;
        } else {
            devm_gpiod_put(dev, desc);
        }
    }
    return 0;
}

static int register_gpio_sysfs(struct device *dev)
{
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) dev_get_drvdata(dev);
    int rv = 0;
    struct gpio_node *node = NULL;
    struct gpio_node *tmp_node = NULL;
    struct gpio_dev_attr *gdev_attr = NULL;
    struct gpio_dev_attr *tmp_gdev_attr = NULL;
    bool found = false;
    int class = GPIO_CLASS_GENERAL;

    if(!gpio_data) {
        return -EINVAL;
    }

    list_for_each_entry_safe(node, tmp_node,
        &gpio_data->node_lists.list, list) {
        class = node->class;

        list_for_each_entry_safe(gdev_attr, tmp_gdev_attr,
            &gpio_data->gpio_attr_lists.list, list) {
            if(!strcmp(gdev_attr->con_id, node->con_id)) {
                found = true;
                break;
            }
        }

        if(!found) {
            gdev_attr =
                devm_kzalloc(dev, sizeof(struct gpio_dev_attr), GFP_KERNEL);
            if (!gdev_attr) {
                dev_dbg(dev, "Failed to allocate memory for gdev_attr.\n");
                continue;
            }

            gdev_attr->dev_attr.attr.name = node->con_id; //sysfs name

            if(node->dir & GPIOD_FLAGS_BIT_DIR_SET &&
                node->dir & GPIOD_FLAGS_BIT_DIR_OUT) {
                gdev_attr->dev_attr.attr.mode = (S_IRUGO | S_IWUSR);
            } else {
                gdev_attr->dev_attr.attr.mode = (S_IRUGO);
            }

            switch(node->class) {
                case GPIO_CLASS_GENERAL_ARRAY:
                    gdev_attr->dev_attr.show = class_general_array_show;
                    gdev_attr->dev_attr.store = class_general_array_store;
                    break;
                case GPIO_CLASS_VAL_MAP:
                    gdev_attr->dev_attr.show = class_val_map_show;
                    gdev_attr->dev_attr.store = class_val_map_store;
                    break;
                case GPIO_CLASS_UDF1:
                    gdev_attr->dev_attr.show = class_udf_show;
                    gdev_attr->dev_attr.store = class_udf_store;
                    break;
                case GPIO_CLASS_UDF2:
                    gdev_attr->dev_attr.show = class_udf_show;
                    gdev_attr->dev_attr.store = class_udf_store;
                    break;
                case GPIO_CLASS_UDF3:
                    gdev_attr->dev_attr.show = class_udf_show;
                    gdev_attr->dev_attr.store = class_udf_store;
                    break;
                case GPIO_CLASS_UDF4:
                    gdev_attr->dev_attr.show = class_udf_show;
                    gdev_attr->dev_attr.store = class_udf_store;
                    break;
                default:
                    gdev_attr->dev_attr.show = class_general_show;
                    gdev_attr->dev_attr.store = class_general_store;
                    break;
            }

            gdev_attr->con_id = node->con_id;
            gdev_attr->reg_vals = node->reg_vals;
            gdev_attr->user_vals = node->user_vals;
            gdev_attr->class = node->class;
            gdev_attr->data_type = node->data_type;

            rv = sysfs_add_file_to_group(&dev->kobj,
                    &gdev_attr->dev_attr.attr, gpio_data->grp.name);
            if(rv <0) {
                devm_kfree(dev, gdev_attr);
                dev_dbg(dev, "Create sysfs(%s) fail (%d)\n", node->con_id, rv);
            } else {
                list_add_tail(&gdev_attr->list,
                    &gpio_data->gpio_attr_lists.list);
                dev_dbg(dev, "Create sysfs attr name(%s) class(%d) "
                    "entry addr(%p)\n",
                    gdev_attr->dev_attr.attr.name, node->class,
                    &gdev_attr->dev_attr.attr);
            }
        } else {
            if(node->class == GPIO_CLASS_GENERAL) {
                gdev_attr->class = GPIO_CLASS_GENERAL_ARRAY;
                gdev_attr->data_type = DATA_HEX;
                gdev_attr->dev_attr.show = class_general_array_show;
                gdev_attr->dev_attr.store = class_general_array_store;
            }
        }
        found = false;
    }
    return rv;
}

static int unregister_gpio_sysfs(struct device *dev)
{
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) dev_get_drvdata(dev);
    struct gpio_dev_attr *gdev_attr = NULL;
    struct gpio_dev_attr *tmp_gdev_attr = NULL;

    if(!gpio_data) {
        return -EINVAL;
    }

    list_for_each_entry_safe(gdev_attr, tmp_gdev_attr, 
        &gpio_data->gpio_attr_lists.list, list) {
        dev_dbg(dev, "Delete sysfs attr name(%s) entry gdev_attr(%p) "
            "dev_attr(%p) addr(%p)\n",
            gdev_attr->dev_attr.attr.name, gdev_attr,
            &gdev_attr->dev_attr, &gdev_attr->dev_attr.attr);
        sysfs_remove_file_from_group(&dev->kobj, &gdev_attr->dev_attr.attr,
            gpio_data->grp.name);
        list_del(&gdev_attr->list);
        devm_kfree(dev, gdev_attr);
    }

    return 0;
}

static ssize_t new_device_store(struct device *dev,
    struct device_attribute *da, const char *buf, size_t count)
{
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) dev_get_drvdata(dev);
    int rv = 0;

    if(!gpio_data) {
        return -EINVAL;
    }

    mutex_lock(&gpio_data->access_lock);
    if(gpio_data->is_enabled) {
        dev_dbg(dev, "Operation failed: table is enabled.\n");
        rv = -EBUSY;
        goto done;
    }

    if(gpio_data->table_used > GPIO_MAX_NUM) {
        dev_dbg(dev, "Table resources are fully used (%d / %d).\n",
            gpio_data->table_used, GPIO_MAX_NUM);
        rv = -ENOSPC;
        goto done;
    }

    if(sysfs_streq(buf,"enable")) {
        update_gpio_chip_label(dev);
        register_gpio_lookup_table(dev);
        apply_gpio_default_conf(dev, GPIOD_ASIS);
        register_gpio_sysfs(dev);
    } else {
        rv = gpio_config_parse(dev, buf, GPIO_TABLE_ADD);
        if(rv < 0) {
            goto done;
        }
    }
    rv = count;
done:
    mutex_unlock(&gpio_data->access_lock);
    return rv;
}

static ssize_t delete_device_store(struct device *dev,
    struct device_attribute *da, const char *buf, size_t count)
{
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) dev_get_drvdata(dev);
    int rv = count;

    if(!gpio_data) {
        return -EINVAL;
    }

    mutex_lock(&gpio_data->access_lock);
    if(sysfs_streq(buf,"disable")) {
        if(!gpio_data->is_enabled) {
            dev_dbg(dev, "Operation failed: table is disabled.\n");
            rv = -EBUSY;
            goto done;
        }

        if(gpio_data->is_kernel_use) {
            dev_dbg(dev, "Operation failed: table is used by kernel.\n");
            rv = -EBUSY;
            goto done;
        }

        unregister_gpio_sysfs(dev);
        apply_gpio_default_conf(dev, GPIOD_IN);
        unregister_gpio_lookup_table(dev);
    } else if(sysfs_streq(buf,"clear")) {
        if(gpio_data->is_enabled) {
            dev_dbg(dev, "Operation failed: table is enabled.\n");
            rv = -EBUSY;
            goto done;
        }
        gpio_table_clear(dev);
    } else {
        if(gpio_data->is_enabled) {
            dev_dbg(dev, "Operation failed: table is enabled.\n");
            rv = -EBUSY;
            goto done;
        }
        rv = gpio_config_parse(dev, buf, GPIO_TABLE_DELETE);
        if(rv < 0) {
            goto done;
        }
    }
    rv = count;
done:
    mutex_unlock(&gpio_data->access_lock);
    return rv;
}

static ssize_t dump_gpio_table(struct device *dev, char *buf,
    loff_t off, size_t count)
{
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) dev_get_drvdata(dev);
    struct gpio_node *node = NULL;
    struct gpio_node *tmp_node = NULL;
    char *dir = NULL;
    size_t ret_count = 0;
    size_t total_count = 0;
    struct gpio_desc *desc = NULL;
    struct gpio_chip *gc = NULL;
    int value = 0;
    bool can_sleep = true;
    /**
      * gpioXXXXX
      * "gpio" --> 4 character
      * "xxxxx"--> 5 character
      * "(X:X)"--> 5 character
      * '\0'   --> 1 character
      */
    char gpio[15] = {'\0'};

    if(!gpio_data) {
        return -EINVAL;
    }

    list_for_each_entry_safe(node, tmp_node,
        &gpio_data->node_lists.list, list) {
        if(node->dir & GPIOD_FLAGS_BIT_DIR_SET &&
            node->dir & GPIOD_FLAGS_BIT_DIR_OUT &&
            node->dir & GPIOD_FLAGS_BIT_DIR_VAL) {
            dir = "high";
        } else if(node->dir & GPIOD_FLAGS_BIT_DIR_SET &&
            node->dir & GPIOD_FLAGS_BIT_DIR_OUT) {
            dir = "low";
        } else if(node->dir & GPIOD_FLAGS_BIT_DIR_SET) {
            dir = "in";
        } else {
            dir = "unk";
        }

        memset(buf, 0, count);
        ret_count = snprintf(buf, count,"gpioXXXXX(U:U) "
            "name(%s) chip_hwnum(%d) con_id(%s) idx(%d) "
            "dir(0x%lx) dir_str(%s) active_low(%d) class(%d)\n",
            node->key,node->chip_hwnum,
            node->con_id, node->idx, node->dir, dir,
            node->active_low, node->class);

        total_count += min(ret_count, count);
        if(off < total_count) {
            desc =
                devm_gpiod_get_index(dev, node->con_id, node->idx, GPIOD_ASIS);
            if (IS_ERR(desc)) {
                snprintf(gpio, sizeof(gpio), "gpio%-5s(%s:%s)",
                    "N/A", "U", "U");
            } else {
                gc = gpiod_to_chip(desc);
                if(!!gc) {
                    can_sleep = gc->can_sleep;
                }
                if(!can_sleep) {
                    value = gpiod_get_value(desc);
                } else {
                    value = gpiod_get_value_cansleep(desc);
                }

                snprintf(gpio, sizeof(gpio), "gpio%-5d(%s:%s)",
                    desc_to_gpio(desc),
                    gpiod_get_direction(desc) ? "I":"O",
                    value ? "1":"0");
                devm_gpiod_put(dev, desc);
            }

            memset(buf, 0, count);
            snprintf(buf, count,"%s name(%s) chip_hwnum(%d) "
                "con_id(%s) idx(%d) dir(0x%lx) dir_str(%s) "
                "active_low(%d) class(%d)\n",
                gpio, node->key,node->chip_hwnum,
                node->con_id, node->idx, node->dir, dir,
                node->active_low, node->class);
            dev_dbg(dev, "%s name(%s) chip_hwnum(%d) "
                "con_id(%s) idx(%d) dir(0x%lx) dir_str(%s) "
                "active_low(%d) class(%d)\n",
                gpio, node->key,node->chip_hwnum,
                node->con_id, node->idx, node->dir, dir,
                node->active_low, node->class);
            return min(ret_count, count);
        }
    }

    memset(buf, 0, count);
    ret_count = snprintf(buf, count, "== used(%d/%d) enable_status(%d) ==\n",
        gpio_data->table_used, GPIO_MAX_NUM, gpio_data->is_enabled);
    total_count += min(ret_count, count);

    if(off < total_count) {
        dev_dbg(dev, "used(%d/%d) enable_status(%d)\n", 
            gpio_data->table_used, GPIO_MAX_NUM, gpio_data->is_enabled);
        return min(ret_count, count);
    } else {
        memset(buf, 0, count);
        return (off+count > DUMP_TABLE_MAX_SIZE) ?
            DUMP_TABLE_MAX_SIZE-count:count;
    }
}

static ssize_t dump_sysfs_table(struct device *dev, char *buf,
    loff_t off, size_t count)
{
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) dev_get_drvdata(dev);
    int i = 0;
    struct gpio_dev_attr *gdev_attr = NULL;
    struct gpio_dev_attr *tmp_gdev_attr = NULL;
    char *data_type = NULL;
    char *class = NULL;
    size_t ret_count = 0;
    size_t total_count = 0;

    if(!gpio_data) {
        return -EINVAL;
    }

    list_for_each_entry_safe(gdev_attr, tmp_gdev_attr,
        &gpio_data->gpio_attr_lists.list, list) {
        switch(gdev_attr->data_type) {
            case DATA_HEX:
                data_type = "HEX";
                break;
            case DATA_DEC:
                data_type = "DEC";
                break;
            case DATA_S_DEC:
                data_type = "Signed DEC";
                break;
            default:
                data_type = "UNK";
                break;
        }

        switch(gdev_attr->class) {
            case GPIO_CLASS_GENERAL:
                class = "GENERAL";
                break;
            case GPIO_CLASS_GENERAL_ARRAY:
                class = "GENERAL_ARRAY";
                break;
            case GPIO_CLASS_VAL_MAP:
                class = "VALMAP";
                break;
            case GPIO_CLASS_UDF1:
                class = "UDF1";
                break;
            case GPIO_CLASS_UDF2:
                class = "UDF2";
                break;
            case GPIO_CLASS_UDF3:
                class = "UDF3";
                break;
            case GPIO_CLASS_UDF4:
                class = "UDF4";
                break;
            default:
                class = "UNK";
                break;
        }

        memset(buf, 0, count);
        ret_count = snprintf(buf, count, "sysfs attr index(%d) name(%s) "
            "reg_vals(%s) user_vals(%s) class(%s/%d) "
            "data_type(%s/%d) entry addr(%p)\n",
            i, gdev_attr->dev_attr.attr.name,
            gdev_attr->reg_vals, gdev_attr->user_vals,class, gdev_attr->class,
            data_type, gdev_attr->data_type, &gdev_attr->dev_attr.attr);

        total_count += min(ret_count, count);

        if(off < total_count) {
            dev_dbg(dev, "sysfs attr index(%d) name(%s) "
                "reg_vals(%s) user_vals(%s) class(%s/%d) "
                "data_type(%s/%d) entry addr(%p)\n",
                i, gdev_attr->dev_attr.attr.name,
                gdev_attr->reg_vals, gdev_attr->user_vals, class, gdev_attr->class,
                data_type, gdev_attr->data_type, &gdev_attr->dev_attr.attr);
            return min(ret_count, count);
        }

        i++;
    }

    memset(buf, 0, count);
    ret_count =
        snprintf(buf, count, "== sysfs attribute used(%d) used_by(%s) ==\n",
            i, gpio_data->is_kernel_use ? "kernel": "user");
    total_count += min(ret_count, count);
    if(off < total_count) {
        dev_dbg(dev, "sysfs attribute used(%d) used_by(%s)\n",
            i, gpio_data->is_kernel_use ? "kernel": "user");
        return min(ret_count, count);
    } else {
        memset(buf, 0, count);
        return (off+count > DUMP_TABLE_MAX_SIZE) ?
            DUMP_TABLE_MAX_SIZE-count:count;
    }
}

static ssize_t dump_table_read(struct file *filp, struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf, loff_t off, size_t count)
{
    struct device *dev = kobj_to_dev(kobj);
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) dev_get_drvdata(dev);
    ssize_t rv = 0;

    if(!gpio_data) {
        return -EINVAL;
    }

    mutex_lock(&gpio_data->access_lock);

    switch(gpio_data->dump_table_type) {
        case TABLE_TYPE_GPIO:
            rv = dump_gpio_table(dev, buf, off, count);
            break;
        case TABLE_TYPE_SYSFS:
            rv = dump_sysfs_table(dev, buf, off, count);
            break;
        default:
            rv = dump_gpio_table(dev, buf, off, count);
            break;
    }

    dev_dbg(dev, "Dump info offet(%lld) count(%lu) ret_count(%lu)\n",
        off, count, rv);

    mutex_unlock(&gpio_data->access_lock);
    return rv;
}

static ssize_t dump_table_write(struct file *filp, struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf, loff_t off, size_t count)
{
    struct device *dev = kobj_to_dev(kobj);
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) dev_get_drvdata(dev);
    ssize_t rv = count;

    if(!gpio_data) {
        return -EINVAL;
    }

    mutex_lock(&gpio_data->access_lock);
    if(sysfs_streq(buf,"gpio")) {
        gpio_data->dump_table_type = TABLE_TYPE_GPIO;
    } else if(sysfs_streq(buf,"sysfs")) {
        gpio_data->dump_table_type = TABLE_TYPE_SYSFS;
    } else {
        rv = -EINVAL;
        goto done;
    }
done:
    mutex_unlock(&gpio_data->access_lock);
    return rv;
}

static ssize_t class_general_show(struct device *dev,
    struct device_attribute *da, char *buf)
{
    struct gpio_dev_attr *gdev_attr = to_gpio_dev_attr(da);
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) dev_get_drvdata(dev);
    char *reg = NULL;
    u8 data_type=DATA_UNK;
    struct gpio_desc *desc = NULL;
    struct gpio_chip *gc = NULL;
    int rv = 0;
    int value = -1;
    bool can_sleep = true;

    reg = gdev_attr->con_id;
    data_type = gdev_attr->data_type;

    if(!gpio_data) {
        return -EINVAL;
    }

    mutex_lock(&gpio_data->access_lock);
    desc = devm_gpiod_get_index(dev, reg, 0, GPIOD_ASIS);
    if (IS_ERR(desc)) {
        dev_dbg(dev, "Failed to get desc(%s): error code (%ld).\n",
            reg, PTR_ERR(desc));
        rv = PTR_ERR(desc);
        goto fail;
    }

    gc = gpiod_to_chip(desc);
    if(!!gc) {
        can_sleep = gc->can_sleep;
    }

    if(!can_sleep) {
        value = gpiod_get_value(desc);
    } else {
        value = gpiod_get_value_cansleep(desc);
    }

    devm_gpiod_put(dev, desc);
    mutex_unlock(&gpio_data->access_lock);
    return ufi_gpio_print_data(buf, value, data_type);
fail:
    mutex_unlock(&gpio_data->access_lock);
    return rv;
}

static ssize_t class_general_store(struct device *dev,
    struct device_attribute *da, const char *buf, size_t count)
{
    struct gpio_dev_attr *gdev_attr = to_gpio_dev_attr(da);
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) dev_get_drvdata(dev);
    int rv = 0;
    char *reg = NULL;
    u8 data_type=DATA_UNK;
    struct gpio_desc *desc = NULL;
    struct gpio_chip *gc = NULL;
    int value = -1;
    bool can_sleep = true;

    reg = gdev_attr->con_id;
    data_type = gdev_attr->data_type;

    if(!gpio_data) {
        return -EINVAL;
    }

    mutex_lock(&gpio_data->access_lock);

    if (kstrtoint(buf, 0, &value) < 0) {
        rv = -EINVAL;
        goto fail;
    }

    desc = devm_gpiod_get_index(dev, reg, 0, GPIOD_ASIS);
    if (IS_ERR(desc)) {
        dev_dbg(dev, "Failed to get desc(%s): error code (%ld).\n",
            reg, PTR_ERR(desc));
        rv = PTR_ERR(desc);
        goto fail;
    }

    if(value<0 || value>1) {
        dev_dbg(dev, "Input is out of range(%d-%d)\n", 0, 1);
        rv = -EINVAL;
        devm_gpiod_put(dev, desc);
        goto fail;
    }

    gc = gpiod_to_chip(desc);
    if(!!gc) {
        can_sleep = gc->can_sleep;
    }

    if(!can_sleep) {
        gpiod_set_value(desc, value);
    } else {
        gpiod_set_value_cansleep(desc, value);
    }

    devm_gpiod_put(dev, desc);
    mutex_unlock(&gpio_data->access_lock);
    return count;
fail:
    mutex_unlock(&gpio_data->access_lock);
    return rv;

}

static ssize_t class_general_array_show(struct device *dev,
    struct device_attribute *da, char *buf)
{
    struct gpio_dev_attr *gdev_attr = to_gpio_dev_attr(da);
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) dev_get_drvdata(dev);
    char *reg = NULL;
    u8 data_type=DATA_UNK;
    struct gpio_descs *descs = NULL;
    unsigned int ndescs = 0;
    struct gpio_desc *desc = NULL;
    struct gpio_chip *gc = NULL;
    int rv = 0;
    int value = 0;
    int bit_val = -1;
    bool can_sleep = true;

    reg = gdev_attr->con_id;
    data_type = gdev_attr->data_type;

    if(!gpio_data) {
        return -EINVAL;
    }

    mutex_lock(&gpio_data->access_lock);
    descs = devm_gpiod_get_array(dev, reg, GPIOD_ASIS);
    if (IS_ERR(descs)) {
        dev_dbg(dev, "Failed to get descs(%s): error code (%ld).\n",
            reg, PTR_ERR(descs));
        rv = PTR_ERR(descs);
        goto fail;
    }

    for(ndescs=0; ndescs< descs->ndescs;ndescs++) {
        desc = descs->desc[ndescs];
        gc = gpiod_to_chip(desc);
        if(!!gc) {
            can_sleep = gc->can_sleep;
        }
        if(!can_sleep) {
            bit_val = gpiod_get_value(desc);
        } else {
            bit_val = gpiod_get_value_cansleep(desc);
        }
        value |=(bit_val << ndescs);
    }
    devm_gpiod_put_array(dev, descs);
    mutex_unlock(&gpio_data->access_lock);
    return ufi_gpio_print_data(buf, value, data_type);
fail:
    mutex_unlock(&gpio_data->access_lock);
    return rv;
}

static ssize_t class_general_array_store(struct device *dev,
    struct device_attribute *da, const char *buf, size_t count)
{
    struct gpio_dev_attr *gdev_attr = to_gpio_dev_attr(da);
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) dev_get_drvdata(dev);
    int rv = 0;
    char *reg = NULL;
    u8 data_type=DATA_UNK;
    struct gpio_descs *descs = NULL;
    unsigned int ndescs = 0;
    struct gpio_desc *desc = NULL;
    struct gpio_chip *gc = NULL;
    int value = -1;
    int bit_val = 0;
    bool can_sleep = true;

    reg = gdev_attr->con_id;
    data_type = gdev_attr->data_type;

    if(!gpio_data) {
        return -EINVAL;
    }

    mutex_lock(&gpio_data->access_lock);

    if (kstrtoint(buf, 0, &value) < 0) {
        rv = -EINVAL;
        goto fail;
    }

    descs = devm_gpiod_get_array(dev, reg, GPIOD_ASIS);
    if (IS_ERR(descs)) {
        dev_dbg(dev, "Failed to get descs(%s): error code (%ld).\n",
            reg, PTR_ERR(descs));
        rv = PTR_ERR(descs);
        goto fail;
    }

    if(value<0 || value>(1<<descs->ndescs)-1) {
        dev_dbg(dev, "Input is out of range(%d-%d)\n", 0, (1<<descs->ndescs)-1);
        rv = -EINVAL;
        devm_gpiod_put_array(dev, descs);
        goto fail;
    }

    for(ndescs=0; ndescs< descs->ndescs;ndescs++) {
        bit_val = mask_and_shift(value, (1<<ndescs));
        desc = descs->desc[ndescs];
        gc = gpiod_to_chip(desc);
        if(!!gc) {
            can_sleep = gc->can_sleep;
        }
        if(!can_sleep) {
            gpiod_set_value(desc, bit_val);
        } else {
            gpiod_set_value_cansleep(desc, bit_val);
        }
    }

    devm_gpiod_put_array(dev, descs);
    mutex_unlock(&gpio_data->access_lock);
    return count;
fail:
    mutex_unlock(&gpio_data->access_lock);
    return rv;
}

static ssize_t class_val_map_show(struct device *dev,
    struct device_attribute *da, char *buf)
{
    struct gpio_dev_attr *gdev_attr = to_gpio_dev_attr(da);
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) dev_get_drvdata(dev);
    char *reg = NULL;
    u8 data_type=DATA_UNK;
    struct gpio_descs *descs = NULL;
    unsigned int ndescs = 0;
    struct gpio_desc *desc = NULL;
    struct gpio_chip *gc = NULL;
    int rv = 0;
    int value = 0;
    int bit_val = -1;
    bool can_sleep = true;
    int user_val = 0;

    reg = gdev_attr->con_id;
    data_type = gdev_attr->data_type;

    if(!gpio_data) {
        return -EINVAL;
    }

    mutex_lock(&gpio_data->access_lock);
    descs = devm_gpiod_get_array(dev, reg, GPIOD_ASIS);
    if (IS_ERR(descs)) {
        dev_dbg(dev, "Failed to get descs(%s): error code (%ld).\n",
            reg, PTR_ERR(descs));
        rv = PTR_ERR(descs);
        goto fail;
    }

    for(ndescs=0; ndescs< descs->ndescs;ndescs++) {
        desc = descs->desc[ndescs];
        gc = gpiod_to_chip(desc);
        if(!!gc) {
            can_sleep = gc->can_sleep;
        }
        if(!can_sleep) {
            bit_val = gpiod_get_value(desc);
        } else {
            bit_val = gpiod_get_value_cansleep(desc);
        }
        value |=(bit_val << ndescs);
    }
    devm_gpiod_put_array(dev, descs);
    rv = reg_val_to_user_val(dev, gdev_attr->reg_vals, gdev_attr->user_vals,
            value, &user_val);
    if(rv < 0) {
        dev_dbg(dev, "Reg value (%d) is not supported.\n", value);
        goto ret_na;
    }

    mutex_unlock(&gpio_data->access_lock);
    return ufi_gpio_print_data(buf, user_val, data_type);
ret_na:
    mutex_unlock(&gpio_data->access_lock);
    return sprintf(buf, "N/A\n");
fail:
    mutex_unlock(&gpio_data->access_lock);
    return rv;
}


static ssize_t class_val_map_store(struct device *dev,
    struct device_attribute *da, const char *buf, size_t count)
{
    struct gpio_dev_attr *gdev_attr = to_gpio_dev_attr(da);
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) dev_get_drvdata(dev);
    int rv = 0;
    char *reg = NULL;
    u8 data_type=DATA_UNK;
    struct gpio_descs *descs = NULL;
    unsigned int ndescs = 0;
    struct gpio_desc *desc = NULL;
    struct gpio_chip *gc = NULL;
    int value = -1;
    int bit_val = 0;
    bool can_sleep = true;
    int reg_val = 0;
    reg = gdev_attr->con_id;
    data_type = gdev_attr->data_type;

    if(!gpio_data) {
        return -EINVAL;
    }

    mutex_lock(&gpio_data->access_lock);

    if (kstrtoint(buf, 0, &value) < 0) {
        rv = -EINVAL;
        goto fail;
    }

    descs = devm_gpiod_get_array(dev, reg, GPIOD_ASIS);
    if (IS_ERR(descs)) {
        dev_dbg(dev, "Failed to get descs(%s): error code (%ld).\n",
            reg, PTR_ERR(descs));
        rv = PTR_ERR(descs);
        goto fail;
    }

    rv = user_val_to_reg_val(dev, gdev_attr->reg_vals, gdev_attr->user_vals,
            value, &reg_val);
    if(rv == -ENODATA) {
        dev_dbg(dev, "Input value(%d) is not support\n", value);
        rv = -EINVAL;
        devm_gpiod_put_array(dev, descs);
        goto fail;
    } else if(rv < 0){
        devm_gpiod_put_array(dev, descs);
        goto fail;
    }

    for(ndescs=0; ndescs< descs->ndescs;ndescs++) {
        bit_val = mask_and_shift(reg_val, (1<<ndescs));
        desc = descs->desc[ndescs];
        gc = gpiod_to_chip(desc);
        if(!!gc) {
            can_sleep = gc->can_sleep;
        }
        if(!can_sleep) {
            gpiod_set_value(desc, bit_val);
        } else {
            gpiod_set_value_cansleep(desc, bit_val);
        }
    }

    devm_gpiod_put_array(dev, descs);
    mutex_unlock(&gpio_data->access_lock);
    return count;
fail:
    mutex_unlock(&gpio_data->access_lock);
    return rv;

}

static ssize_t class_udf_show(struct device *dev,
    struct device_attribute *da, char *buf)
{
    int rv = 0;
    struct gpio_dev_attr *gdev_attr = to_gpio_dev_attr(da);
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) dev_get_drvdata(dev);
    ssize_t (*show)(struct device *dev,
        struct device_attribute *attr, char *buf) = NULL;

    if(!gpio_data) {
        return -EINVAL;
    }

    mutex_lock(&gpio_data->access_lock);
    switch(gdev_attr->class) {
        case GPIO_CLASS_UDF1:
        {
            if(!!ufi_gpio_show_store_udf1.show) {
                show = ufi_gpio_show_store_udf1.show;
            }
            break;
        }
        case GPIO_CLASS_UDF2:
        {
            if(!!ufi_gpio_show_store_udf2.show) {
                show = ufi_gpio_show_store_udf2.show;
            }
            break;
        }
        case GPIO_CLASS_UDF3:
        {
            if(!!ufi_gpio_show_store_udf3.show) {
                show = ufi_gpio_show_store_udf3.show;
            }
            break;
        }
        case GPIO_CLASS_UDF4:
        {
            if(!!ufi_gpio_show_store_udf4.show) {
                show = ufi_gpio_show_store_udf4.show;
            }
            break;
        }
        default:
            break;
    }
    if(!!show) {
        rv = show(dev, da, buf);
    } else {
        rv = sprintf(buf, "N/A\n");
    }

    mutex_unlock(&gpio_data->access_lock);
    return rv;
}


static ssize_t class_udf_store(struct device *dev,
    struct device_attribute *da, const char *buf, size_t count)
{
    int rv = 0;
    struct gpio_dev_attr *gdev_attr = to_gpio_dev_attr(da);
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) dev_get_drvdata(dev);
    ssize_t (*store)(struct device *dev,
        struct device_attribute *attr,
                        const char *buf, size_t count) = NULL;

    if(!gpio_data) {
        return -EINVAL;
    }

    mutex_lock(&gpio_data->access_lock);
    switch(gdev_attr->class) {
        case GPIO_CLASS_UDF1:
        {
            if(!!ufi_gpio_show_store_udf1.store) {
                store = ufi_gpio_show_store_udf1.store;
            }
            break;
        }
        case GPIO_CLASS_UDF2:
        {
            if(!!ufi_gpio_show_store_udf2.store) {
                store = ufi_gpio_show_store_udf2.store;
            }
            break;
        }
        case GPIO_CLASS_UDF3:
        {
            if(!!ufi_gpio_show_store_udf3.store) {
                store = ufi_gpio_show_store_udf3.store;
            }
            break;
        }
        case GPIO_CLASS_UDF4:
        {
            if(!!ufi_gpio_show_store_udf4.store) {
                store = ufi_gpio_show_store_udf4.store;
            }
            break;
        }
        default:
            break;
    }

    if(!!store) {
        rv = store(dev, da, buf, count);
    } else {
        rv = count;
    }

    mutex_unlock(&gpio_data->access_lock);
    return rv;
}

static int gpio_drv_probe(struct platform_device *pdev)
{
    int rv = 0;
    struct gpio_data_s *gpio_data =
        devm_kzalloc(&pdev->dev, sizeof(struct gpio_data_s), GFP_KERNEL);
    if (!gpio_data) {
        dev_dbg(&pdev->dev, "Failed to allocate memory for gpio_data.\n");
        rv = -ENOMEM;
        goto fail;
    }

    gpio_data->grp.name = GROUP_NAME;
    gpio_data->grp.attrs =
        devm_kzalloc(&pdev->dev, sizeof(struct attribute *), GFP_KERNEL);
    if (!gpio_data->grp.attrs) {
        dev_dbg(&pdev->dev, "Failed to allocate memory for "
            "gpio_data->grp.attrs.\n");
        rv = -ENOMEM;
        goto fail;
    }

    mutex_init(&gpio_data->access_lock);
    INIT_LIST_HEAD(&gpio_data->node_lists.list);
    INIT_LIST_HEAD(&gpio_data->gpio_attr_lists.list);
    gpio_data->is_enabled = false;
    gpio_data->is_kernel_use = false;
    gpio_data->dump_table_type = TABLE_TYPE_GPIO;
    rv = device_create_file(&pdev->dev, _DEVICE_ATTR(new_device));
    if(rv <0) {
        dev_err(&pdev->dev, "Failed to create sysfs(new_device).\n");
        goto fail;
    }

    rv = device_create_file(&pdev->dev, _DEVICE_ATTR(delete_device));
    if(rv <0) {
        dev_err(&pdev->dev, "Failed to create sysfs(delete_device).\n");
        goto free_new_device;
    }

    rv = sysfs_create_bin_file(&pdev->dev.kobj, _BIN_ATTR(dump_table));
    if(rv <0) {
        dev_err(&pdev->dev, "Failed to create sysfs(dump_table).\n");
        goto free_delete_device;
    }

    rv = sysfs_create_group(&pdev->dev.kobj, &gpio_data->grp);

    if(rv !=0) {
        dev_err(&pdev->dev, "Failed to create group.\n");
        goto free_dump_table;
    }

    platform_set_drvdata(pdev, gpio_data);

    return 0;

free_dump_table:
    sysfs_remove_bin_file(&pdev->dev.kobj, _BIN_ATTR(dump_table));
free_delete_device:
    device_remove_file(&pdev->dev, _DEVICE_ATTR(delete_device));
free_new_device:
    device_remove_file(&pdev->dev, _DEVICE_ATTR(new_device));
fail:
    platform_set_drvdata(pdev, NULL);
    return rv;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
static int gpio_drv_remove(struct platform_device *pdev)
#else
static void gpio_drv_remove(struct platform_device *pdev)
#endif
{
    struct gpio_data_s *gpio_data =
        (struct gpio_data_s *) platform_get_drvdata(pdev);
    struct gpio_dev_attr *pdev_attr = NULL;
    struct gpio_dev_attr *tmp_pdev_attr = NULL;
    struct gpio_node *node = NULL;
    struct gpio_node *tmp_node = NULL;


    if(!gpio_data){
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
        return -ENOMEM;
#else
        return;
#endif
    }

    sysfs_remove_bin_file(&pdev->dev.kobj, _BIN_ATTR(dump_table));
    device_remove_file(&pdev->dev, _DEVICE_ATTR(delete_device));
    device_remove_file(&pdev->dev, _DEVICE_ATTR(new_device));

    // release sysfs node
    list_for_each_entry_safe(pdev_attr, tmp_pdev_attr,
        &gpio_data->gpio_attr_lists.list, list) {
        sysfs_remove_file_from_group(&pdev->dev.kobj,
            &pdev_attr->dev_attr.attr, gpio_data->grp.name);
        list_del(&pdev_attr->list);
        devm_kfree(&pdev->dev, pdev_attr);
    }

    // release gpio lookup table
    if(gpio_data->lookup_table) {
        gpiod_remove_lookup_table(gpio_data->lookup_table);
        devm_kfree(&pdev->dev, gpio_data->lookup_table);
        gpio_data->lookup_table = NULL;
    }

    // release gpio table
    list_for_each_entry_safe(node, tmp_node,
        &gpio_data->node_lists.list, list) {
        list_del(&node->list);
        devm_kfree(&pdev->dev, node);
    }

    devm_kfree(&pdev->dev, gpio_data->grp.attrs);
    sysfs_remove_group(&pdev->dev.kobj, &gpio_data->grp);

    devm_kfree(&pdev->dev, gpio_data);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
    return 0;
#endif
}

static struct platform_driver ufi_gpio_drv = {
    .probe = gpio_drv_probe,
    .remove = __exit_p(gpio_drv_remove),
    .driver = {
        .name = DRIVER_NAME,
    },
};

static void ufi_gpio_dev_release( struct device * dev)
{
    return;
}

struct platform_device ufi_gpio_dev = {
    .name = DRIVER_NAME,
    .id = -1,
    .dev = {
        .release = ufi_gpio_dev_release,
    }
};
EXPORT_SYMBOL(ufi_gpio_dev);

int register_gpio(struct device * dev, struct gpio_pconf *table) {
    int rv = 0;
    int i = 0;
    struct gpio_pconf *table_node = NULL;
    struct gpio_node *node = NULL;
    struct gpio_node *tmp_node = NULL;
    struct gpio_node new_node_lists;
    struct gpio_data_s *gpio_data = NULL;;
    INIT_LIST_HEAD(&new_node_lists.list);

    if(!dev || !table) {
        return -EINVAL;
    }

    gpio_data = (struct gpio_data_s *) dev_get_drvdata(dev);

    if(!gpio_data) {
        return -EINVAL;
    }

    mutex_lock(&gpio_data->access_lock);

    if(gpio_data->is_enabled) {
        dev_dbg(dev, "Operation failed: table is enabled.\n");
        rv = -EBUSY;
        goto done;
    }

    if(gpio_data->table_used > 0) {
        dev_dbg(dev, "Operation failed: table is not empty.\n");
        rv = -EBUSY;
        goto done;
    }
    for(table_node=&table[0], i = 0; !!table_node->key; table_node++, i++) {
        node = devm_kzalloc(dev, sizeof(struct gpio_node), GFP_KERNEL);
        if (!node) {
            dev_dbg(dev, "Failed to reallocate memory for node.\n");
            rv = -ENOSPC;
            goto free_node;
        }
        if(!table_node->key) {
            dev_dbg(dev, "key is mandatory.\n");
            devm_kfree(dev, node);
            rv = -EINVAL;
            goto free_node;
        } else {
            snprintf(node->key, GPIO_NAME_MAX+1, "%s", table_node->key);
        }
        node->chip_hwnum = table_node->chip_hwnum;
        if(!table_node->con_id) {
            dev_dbg(dev, "con_id is mandatory.\n");
            devm_kfree(dev, node);
            rv = -EINVAL;
            goto free_node;
        } else {
            snprintf(node->con_id, GPIO_NAME_MAX+1, "%s", table_node->con_id);
        }
        node->idx = table_node->idx;
        node->dir = table_node->dir;
        node->active_low = table_node->active_low;
        node->class = table_node->class;
        node->data_type = table_node->data_type;
        if(node->class == GPIO_CLASS_VAL_MAP) {
            if(!table_node->user_vals) {
                dev_dbg(dev,
                    "The user_vals is mandatory when class is val_map.\n");
                devm_kfree(dev, node);
                rv = -EINVAL;
                goto free_node;
            } else {
                snprintf(node->user_vals, VALUE_STRING_MAX+1, "%s",
                    table_node->user_vals);
            }
        }
        if(node->class == GPIO_CLASS_VAL_MAP) {
            if(!table_node->reg_vals) {
                dev_dbg(dev,
                    "The reg_vals is mandatory when class is val_map.\n");
                devm_kfree(dev, node);
                rv = -EINVAL;
                goto free_node;
            } else {
                snprintf(node->reg_vals, VALUE_STRING_MAX+1, "%s",
                    table_node->reg_vals);
            }
        }
        if(i > (GPIO_MAX_NUM-1)) {
            dev_err(dev, "Exceeding the maximum (%d) GPIO will be ignored.\n",
                GPIO_MAX_NUM);
            devm_kfree(dev, node);
            break;
        }
        list_add_tail(&node->list, &new_node_lists.list);
    }
    list_splice_tail_init(&new_node_lists.list,&gpio_data->node_lists.list);
    gpio_data->table_used += i;
    update_gpio_chip_label(dev);;
    register_gpio_lookup_table(dev);
    apply_gpio_default_conf(dev, GPIOD_ASIS);
    register_gpio_sysfs(dev);
    gpio_data->is_kernel_use = true;
done:
    mutex_unlock(&gpio_data->access_lock);
    return rv;
free_node:
    list_for_each_entry_safe(node, tmp_node, &new_node_lists.list, list) {
        list_del(&node->list);
        devm_kfree(dev, node);
    }
    mutex_unlock(&gpio_data->access_lock);
    return rv;
}

EXPORT_SYMBOL(register_gpio);

void unregister_gpio(struct device * dev) {
    int i = 0;
    struct gpio_node *node = NULL;
    struct gpio_node *tmp_node = NULL;
    struct gpio_data_s *gpio_data = NULL;;

    if(!dev) {
        return;
    }

    gpio_data = (struct gpio_data_s *) dev_get_drvdata(dev);

    if(!gpio_data) {
        return;
    }

    mutex_lock(&gpio_data->access_lock);

    if(!gpio_data->is_enabled) {
        dev_dbg(dev, "Operation failed: table is disabled.\n");
        goto done;
    }

    if(!gpio_data->is_kernel_use) {
        dev_dbg(dev, "Operation failed: table is used by user.\n");
        goto done;
    }

    unregister_gpio_sysfs(dev);
    apply_gpio_default_conf(dev, GPIOD_IN);
    unregister_gpio_lookup_table(dev);

    list_for_each_entry_safe(node, tmp_node,
        &gpio_data->node_lists.list, list) {
        i++;
        list_del(&node->list);
        devm_kfree(dev, node);
    }
    gpio_data->table_used -= i;
    gpio_data->is_kernel_use = false;

done:
    mutex_unlock(&gpio_data->access_lock);
}

EXPORT_SYMBOL(unregister_gpio);

int gpio_init(void)
{
    int err = 0;

    err = platform_driver_register(&ufi_gpio_drv);
    if (err) {
        pr_err("%s(#%d): platform_driver_register failed(%d)\n",
            __func__, __LINE__, err);

        return err;
    }

    err = platform_device_register(&ufi_gpio_dev);
    if (err) {
        pr_err("%s(#%d): platform_device_register failed(%d)\n",
            __func__, __LINE__, err);
        platform_driver_unregister(&ufi_gpio_drv);
        return err;
    }

    return err;
}

void gpio_exit(void)
{
    platform_driver_unregister(&ufi_gpio_drv);
    platform_device_unregister(&ufi_gpio_dev);
}

MODULE_AUTHOR("Nonodark Huang <nonodark.huang@ufispace.com>");
MODULE_DESCRIPTION("x86_64_ufispace_gpio driver");
MODULE_VERSION("0.0.2");
MODULE_LICENSE("GPL");

module_init(gpio_init);
module_exit(gpio_exit);
