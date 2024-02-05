/*
 * A fpga driver for the ufispace
 *
 * Copyright (C) 2017-2023 UfiSpace Technology Corporation.
 * Wade He <wade.ce.he@ufispace.com>
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

/*#define DEBUG*/

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/hwmon-sysfs.h>

#define DRVNAME "ufispace_fpga"
#define DRIVER_NAME "ufispace_fpga"

/* fpga registers */

#define REG_BASE      0xf9000000
#define REG_SPAN      0x00200000
#define REG_MASK      (REG_SPAN - 1)


#define REG_READ_NOTIFY       0x0
#define REG_ADDR              0x4
#define REG_WRITE             0x8

#define VIR_MEM_MAP(vir_base, REG) (void *)((unsigned long)vir_base + ((unsigned long)(0xf9000000 + 0x4000 + REG) & (unsigned long)(REG_MASK)))

#define REG_OFFSET_1                   0x01
#define REG_OFFSET_2                   0x02
#define REG_OFFSET_3                   0x03
#define REG_OFFSET_4                   0x04
#define REG_OFFSET_5                   0x05
#define REG_OFFSET_48763               0x48763

#define MASK_ALL                       (0xFF) // 2#11111111


#define NOTIFY_OFF                     (0x0) // 2#00000000
#define NOTIFY_WRITE                   (0x1) // 2#00000001
#define NOTIFY_READ                    (0x2) // 2#00000010


enum fpga_sysfs_attributes {
    ATTR_OFFSET_1,
    ATTR_OFFSET_2,
    ATTR_OFFSET_3,
    ATTR_OFFSET_4,
    ATTR_OFFSET_5,
    ATTR_OFFSET_48763,
};

struct fpga_data_s {
    struct mutex    access_lock;
    u32 __iomem *vir_base;
};

struct fpga_data_s *fpga_data;

/* reg shift */
static u8 _shift(u8 mask)
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

/* reg mask and shift */
static u8 _mask_shift(u8 val, u8 mask)
{
    int shift=0;

    shift = _shift(mask);

    return (val & mask) >> shift;
}

static u8 _bit_operation(u8 reg_val, u8 bit, u8 bit_val)
{
    if (bit_val == 0)
        reg_val = reg_val & ~(1 << bit);
    else
        reg_val = reg_val | (1 << bit);
    return reg_val;
}

static ssize_t _read_fpga_reg(u32 reg, u8 mask)
{

    u8 reg_val=0x0, reg_mk_shf_val = 0x0;

    mutex_lock(&fpga_data->access_lock);

    //set read address
    iowrite32(reg, VIR_MEM_MAP(fpga_data->vir_base, REG_ADDR));
    // pr_err("reg(0x%x) vir_base(0x%lx) REG_ADDR(0x%lx) vir(0x%lx)  %s %d\n", reg, fpga_data->vir_base, ((unsigned long)( 0xf9000000 + 0x4000 + REG_ADDR ) & (unsigned long)(REG_MASK)), VIR_MEM_MAP(fpga_data->vir_base, REG_ADDR),__FUNCTION__, __LINE__);

    //read reg data
    reg_val = ioread32(VIR_MEM_MAP(fpga_data->vir_base, REG_READ_NOTIFY));
    // pr_err("reg_val(0x%x) vir_base(0x%lx) REG_READ_NOTIFY(0x%lx)  vir(0x%lx)  %s %d\n", reg_val, fpga_data->vir_base, ((unsigned long)( 0xf9000000 + 0x4000 + REG_READ_NOTIFY ) & (unsigned long)(REG_MASK)),VIR_MEM_MAP(fpga_data->vir_base, REG_READ_NOTIFY),__FUNCTION__, __LINE__);

    // notify read data done
    iowrite32(NOTIFY_READ, VIR_MEM_MAP(fpga_data->vir_base, REG_READ_NOTIFY));
     // Todo, need delay ?
    iowrite32(NOTIFY_OFF, VIR_MEM_MAP(fpga_data->vir_base, REG_READ_NOTIFY));

    mutex_unlock(&fpga_data->access_lock);

    reg_mk_shf_val = _mask_shift(reg_val, mask);

    return reg_mk_shf_val;
}

/* get fpga register value */
static ssize_t read_fpga_reg(u32 reg, u8 mask, char *buf)
{
    u8 reg_val;
    int len=0;

    reg_val = _read_fpga_reg(reg, mask);

    // may need to change to hex value ?
    len=sprintf(buf,"0x%x\n", reg_val);

    return len;
}

static ssize_t write_fpga_reg(u32 reg, u8 mask, const char *buf, size_t count)
{

    u8 reg_val, reg_val_now, shift;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    //apply SINGLE BIT operation if mask is specified, multiple bits are not supported
    if (mask != MASK_ALL) {
        reg_val_now = _read_fpga_reg(reg, MASK_ALL);
        shift = _shift(mask);
        reg_val = _bit_operation(reg_val_now, shift, reg_val);
    }

    mutex_lock(&fpga_data->access_lock);

    //set write address
    iowrite32(reg, VIR_MEM_MAP(fpga_data->vir_base, REG_ADDR));

    //set write data
    iowrite32(reg_val, VIR_MEM_MAP(fpga_data->vir_base, REG_WRITE));

    //send write enable // Todo, modify comment message
    iowrite32(NOTIFY_WRITE, VIR_MEM_MAP(fpga_data->vir_base, REG_READ_NOTIFY));
    // Todo, need delay ?
    iowrite32(NOTIFY_OFF, VIR_MEM_MAP(fpga_data->vir_base, REG_READ_NOTIFY));

    mutex_unlock(&fpga_data->access_lock);

    return count;
}


/* get fpga register value */
static ssize_t read_fpga_callback(struct device *dev,
        struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u32 reg = 0;
    u8 mask = MASK_ALL;

    switch (attr->index) {
        case ATTR_OFFSET_1:
            reg = REG_OFFSET_1;
            break;
        case ATTR_OFFSET_2:
            reg = REG_OFFSET_2;
            break;
        case ATTR_OFFSET_3:
            reg = REG_OFFSET_3;
            break;
        case ATTR_OFFSET_4:
            reg = REG_OFFSET_4;
            mask = 0x6; //2#00000110
            break;
        case ATTR_OFFSET_5:
            reg = REG_OFFSET_5;
            break;
        case ATTR_OFFSET_48763:
            reg = REG_OFFSET_48763;
            break;
        default:
            return -EINVAL;
    }
    return read_fpga_reg(reg, mask, buf);
}


/* set lpc register value */
static ssize_t write_fpga_callback(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u32 reg = 0;
    u8 mask = MASK_ALL;

    switch (attr->index) {
        case ATTR_OFFSET_1:
            reg = REG_OFFSET_1;
            break;
        case ATTR_OFFSET_2:
            reg = REG_OFFSET_2;
            break;
        case ATTR_OFFSET_3:
            reg = REG_OFFSET_3;
            break;
        case ATTR_OFFSET_4:
            reg = REG_OFFSET_4;
            break;
        case ATTR_OFFSET_5:
            reg = REG_OFFSET_5;
            break;
        case ATTR_OFFSET_48763:
            reg = REG_OFFSET_48763;
            break;
        default:
            return -EINVAL;
    }
    return write_fpga_reg(reg, mask, buf, count);
}

static SENSOR_DEVICE_ATTR(offset_1    , S_IRUGO | S_IWUSR, read_fpga_callback, write_fpga_callback, ATTR_OFFSET_1);
static SENSOR_DEVICE_ATTR(offset_2    , S_IRUGO | S_IWUSR, read_fpga_callback, write_fpga_callback, ATTR_OFFSET_2);
static SENSOR_DEVICE_ATTR(offset_3    , S_IRUGO | S_IWUSR, read_fpga_callback, write_fpga_callback, ATTR_OFFSET_3);
static SENSOR_DEVICE_ATTR(offset_4    , S_IRUGO | S_IWUSR, read_fpga_callback, write_fpga_callback, ATTR_OFFSET_4);
static SENSOR_DEVICE_ATTR(offset_5    , S_IRUGO | S_IWUSR, read_fpga_callback, write_fpga_callback, ATTR_OFFSET_5);
static SENSOR_DEVICE_ATTR(offset_48763, S_IRUGO | S_IWUSR, read_fpga_callback, write_fpga_callback, ATTR_OFFSET_48763);

static struct attribute *fpga_attributes[] = {
    &sensor_dev_attr_offset_1.dev_attr.attr,
    &sensor_dev_attr_offset_2.dev_attr.attr,
    &sensor_dev_attr_offset_3.dev_attr.attr,
    &sensor_dev_attr_offset_4.dev_attr.attr,
    &sensor_dev_attr_offset_5.dev_attr.attr,
    &sensor_dev_attr_offset_48763.dev_attr.attr,
    NULL
};

static struct attribute_group fpga_group = {
	.attrs = fpga_attributes,
};

static void fpga_dev_release( struct device * dev)
{
    return;
}

static struct platform_device fpga_dev = {
    .name           = DRIVER_NAME,
    .id             = -1,
    .dev = {
                    .release = fpga_dev_release,
    }
};

static int fpga_drv_probe(struct platform_device *pdev)
{
    int i = 0, grp_num = 1;
    int err[5] = {0};
    struct attribute_group *grp;

    fpga_data = devm_kzalloc(&pdev->dev, sizeof(struct fpga_data_s),
                    GFP_KERNEL);
    if (!fpga_data)
        return -ENOMEM;

    fpga_data->vir_base=ioremap(REG_BASE, REG_SPAN);


    mutex_init(&fpga_data->access_lock);

    for (i=0; i<grp_num; ++i) {
        switch (i) {
            case 0:
                grp = &fpga_group;
                break;
            default:
                break;
        }

        err[i] = sysfs_create_group(&pdev->dev.kobj, grp);
        if (err[i]) {
            printk(KERN_ERR "Cannot create sysfs for group %s\n", grp->name);
            goto exit;
        } else {
            continue;
        }
    }

    return 0;

exit:
    for (i=0; i<grp_num; ++i) {
        switch (i) {
            case 0:
                grp = &fpga_group;
                break;
            default:
                break;
        }

        sysfs_remove_group(&pdev->dev.kobj, grp);
        if (!err[i]) {
            //remove previous successful cases
            continue;
        } else {
            //remove first failed case, then return
            return err[i];
        }
    }
    return 0;
}

static int fpga_drv_remove(struct platform_device *pdev)
{
    sysfs_remove_group(&pdev->dev.kobj, &fpga_group);
    return 0;
}

static struct platform_driver fpga_drv = {
    .probe	  = fpga_drv_probe,
    .remove	 =  __exit_p(fpga_drv_remove),
    .driver	 = {
        .name   = DRIVER_NAME,
    },
};


static int __init fpga_init(void)
{
    int err = 0;

    err = platform_driver_register(&fpga_drv);
    if (err) {
    	printk(KERN_ERR "%s(#%d): platform_driver_register failed(%d)\n",
                __func__, __LINE__, err);

    	return err;
    }

    err = platform_device_register(&fpga_dev);
    if (err) {
    	printk(KERN_ERR "%s(#%d): platform_device_register failed(%d)\n",
                __func__, __LINE__, err);
    	platform_driver_unregister(&fpga_drv);
    	return err;
    }

    return err;
}

void fpga_exit(void)
{
    platform_driver_unregister(&fpga_drv);
    platform_device_unregister(&fpga_dev);
}

MODULE_AUTHOR("Wade He <wade.ce.he@ufispace.com>");
MODULE_DESCRIPTION("ufispace fpga driver");
MODULE_LICENSE("GPL");

module_init(fpga_init);
module_exit(fpga_exit);