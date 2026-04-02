/*
 * A i2c cpld driver for the ufispace_s9321_64e
 *
 * Copyright (C) 2017-2019 UfiSpace Technology Corporation.
 * Nonodark Huang <nonodark.huang@ufispace.com>
 *
 * Based on i2c-mux-pca954x.c
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

#include <linux/delay.h>

#include "x86-64-ufispace-s9620-32e-cpld-main.h"

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

/* Provide specs for the cpld mux types we know about */
const struct chip_desc chips[] = {
    [cpld1] = {
        .nchans = 0,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
        .id = { .manufacturer_id = I2C_DEVICE_ID_NONE },
#endif /* #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0) */
    },
    [cpld2] = {
        .nchans = 16,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
        .id = { .manufacturer_id = I2C_DEVICE_ID_NONE },
#endif /* #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0) */
    },
    [cpld3] = {
        .nchans = 16,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
        .id = { .manufacturer_id = I2C_DEVICE_ID_NONE },
#endif /* #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0) */
    },
    [cpld4] = {
        .nchans = 6,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
        .id = { .manufacturer_id = I2C_DEVICE_ID_NONE },
#endif /* #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0) */
    },
    [fpga] = {
        .nchans = 0,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
        .id = { .manufacturer_id = I2C_DEVICE_ID_NONE },
#endif /* #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0) */
    },
};


typedef u8 chan_map[CPLD_MAX_NCHANS];
static const chan_map chans_map[] = {
    [cpld1] = {0},
    [cpld2] = {
        //PORT 0      PORT 1        PORT 2        PORT 3
        (0b00000001), (0b00000010), (0b00000011), (0b00000100),
        //PORT 4      PORT 5        PORT 6        PORT 7
        (0b00000101), (0b00000110), (0b00000111), (0b00001000),
        //PORT 8      PORT 9        PORT 10       PORT 11
        (0b00001001), (0b00001010), (0b00001011), (0b00001100),
        //PORT 12     PORT 13       PORT 14       PORT 15
        (0b00001101), (0b00001110), (0b00001111), (0b00010000),
    },
    [cpld3] = {
        //PORT 16     PORT 17       PORT 18       PORT 19
        (0b00000001), (0b00000010), (0b00000011), (0b00000100),
        //PORT 20     PORT 21       PORT 22       PORT 23
        (0b00000101), (0b00000110), (0b00000111), (0b00001000),
        //PORT 24     PORT 25       PORT 26       PORT 27
        (0b00001001), (0b00001010), (0b00001011), (0b00001100),
        //PORT 28     PORT 29       PORT 30       PORT 31
        (0b00001101), (0b00001110), (0b00001111), (0b00010000),
    },
    [cpld4] = {
        //SFP 0       SFP 1         SFP 2         SFP 3
        (0b00000001), (0b00000010), (0b00000011), (0b00000100),
        //SFP 4       SFP 5
        (0b00000101), (0b00000110),
    },
    [fpga] = {0},
};

typedef u32 port_map[CPLD_MAX_NCHANS];
static const port_map ports_map[] = {
    [cpld1] = {-1},
    [cpld2] = {
        0 , 1 , 2 , 3,
        4 , 5 , 6 , 7,
        8 , 9 , 10, 11,
        12, 13, 14, 15
    },
    [cpld3] = {
        16, 17, 18, 19,
        20, 21, 22, 23,
        24, 25, 26, 27,
        28, 29, 30, 31,
    },
    [cpld4] = {
        32, 33, 34, 35,
        36, 37
    },
    [fpga] = {-1},
};

typedef struct  {
    u16 reg;
    u16 evt_reg;
    u8 mask;
} port_block_map_t;

static const port_block_map_t ports_block_map[] = {
    [0]=  {.reg=QSFPDD_0_7_16_23_STUCK_REG  , .evt_reg=QSFPDD_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0000_0001},
    [1]=  {.reg=QSFPDD_0_7_16_23_STUCK_REG  , .evt_reg=QSFPDD_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0000_0010},
    [2]=  {.reg=QSFPDD_0_7_16_23_STUCK_REG  , .evt_reg=QSFPDD_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0000_0100},
    [3]=  {.reg=QSFPDD_0_7_16_23_STUCK_REG  , .evt_reg=QSFPDD_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0000_1000},
    [4]=  {.reg=QSFPDD_0_7_16_23_STUCK_REG  , .evt_reg=QSFPDD_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0001_0000},
    [5]=  {.reg=QSFPDD_0_7_16_23_STUCK_REG  , .evt_reg=QSFPDD_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0010_0000},
    [6]=  {.reg=QSFPDD_0_7_16_23_STUCK_REG  , .evt_reg=QSFPDD_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0100_0000},
    [7]=  {.reg=QSFPDD_0_7_16_23_STUCK_REG  , .evt_reg=QSFPDD_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_1000_0000},
    [8]=  {.reg=QSFPDD_8_15_24_31_STUCK_REG , .evt_reg=QSFPDD_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0000_0001},
    [9]=  {.reg=QSFPDD_8_15_24_31_STUCK_REG , .evt_reg=QSFPDD_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0000_0010},
    [10]= {.reg=QSFPDD_8_15_24_31_STUCK_REG , .evt_reg=QSFPDD_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0000_0100},
    [11]= {.reg=QSFPDD_8_15_24_31_STUCK_REG , .evt_reg=QSFPDD_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0000_1000},
    [12]= {.reg=QSFPDD_8_15_24_31_STUCK_REG , .evt_reg=QSFPDD_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0001_0000},
    [13]= {.reg=QSFPDD_8_15_24_31_STUCK_REG , .evt_reg=QSFPDD_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0010_0000},
    [14]= {.reg=QSFPDD_8_15_24_31_STUCK_REG , .evt_reg=QSFPDD_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0100_0000},
    [15]= {.reg=QSFPDD_8_15_24_31_STUCK_REG , .evt_reg=QSFPDD_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_1000_0000},
    [16]= {.reg=QSFPDD_0_7_16_23_STUCK_REG  , .evt_reg=QSFPDD_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0000_0001},
    [17]= {.reg=QSFPDD_0_7_16_23_STUCK_REG  , .evt_reg=QSFPDD_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0000_0010},
    [18]= {.reg=QSFPDD_0_7_16_23_STUCK_REG  , .evt_reg=QSFPDD_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0000_0100},
    [19]= {.reg=QSFPDD_0_7_16_23_STUCK_REG  , .evt_reg=QSFPDD_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0000_1000},
    [20]= {.reg=QSFPDD_0_7_16_23_STUCK_REG  , .evt_reg=QSFPDD_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0001_0000},
    [21]= {.reg=QSFPDD_0_7_16_23_STUCK_REG  , .evt_reg=QSFPDD_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0010_0000},
    [22]= {.reg=QSFPDD_0_7_16_23_STUCK_REG  , .evt_reg=QSFPDD_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_0100_0000},
    [23]= {.reg=QSFPDD_0_7_16_23_STUCK_REG  , .evt_reg=QSFPDD_0_7_16_23_STUCK_EVENT_REG  , .mask=MASK_1000_0000},
    [24]= {.reg=QSFPDD_8_15_24_31_STUCK_REG , .evt_reg=QSFPDD_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0000_0001},
    [25]= {.reg=QSFPDD_8_15_24_31_STUCK_REG , .evt_reg=QSFPDD_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0000_0010},
    [26]= {.reg=QSFPDD_8_15_24_31_STUCK_REG , .evt_reg=QSFPDD_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0000_0100},
    [27]= {.reg=QSFPDD_8_15_24_31_STUCK_REG , .evt_reg=QSFPDD_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0000_1000},
    [28]= {.reg=QSFPDD_8_15_24_31_STUCK_REG , .evt_reg=QSFPDD_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0001_0000},
    [29]= {.reg=QSFPDD_8_15_24_31_STUCK_REG , .evt_reg=QSFPDD_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0010_0000},
    [30]= {.reg=QSFPDD_8_15_24_31_STUCK_REG , .evt_reg=QSFPDD_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_0100_0000},
    [31]= {.reg=QSFPDD_8_15_24_31_STUCK_REG , .evt_reg=QSFPDD_8_15_24_31_STUCK_EVENT_REG , .mask=MASK_1000_0000},
    [32]= {.reg=MGMT_SFP_STUCK_REG     , .evt_reg=MGMT_SFP_STUCK_EVENT_REG     , .mask=MASK_0000_0001},
    [33]= {.reg=MGMT_SFP_STUCK_REG     , .evt_reg=MGMT_SFP_STUCK_EVENT_REG     , .mask=MASK_0000_0010},
    [34]= {.reg=MGMT_SFP_STUCK_REG     , .evt_reg=MGMT_SFP_STUCK_EVENT_REG     , .mask=MASK_0000_0100},
    [35]= {.reg=MGMT_SFP_STUCK_REG     , .evt_reg=MGMT_SFP_STUCK_EVENT_REG     , .mask=MASK_0000_1000},
    [36]= {.reg=MGMT_SFP_STUCK_REG     , .evt_reg=MGMT_SFP_STUCK_EVENT_REG     , .mask=MASK_0001_0000},
    [37]= {.reg=MGMT_SFP_STUCK_REG     , .evt_reg=MGMT_SFP_STUCK_EVENT_REG     , .mask=MASK_0010_0000},
};

int port_chan_get_from_reg(u8 val, int index, int *chan, int *port)
{
    u32 i = 0;
    u32 cmp = 0;

    cmp = val & ~(CPLD_I2C_ENABLE_CHN_SEL);

    if(cmp != 0) {
        for(i = 0;i < NELEMS(chans_map[index]);i++) {
            if(chans_map[index][i] == cmp) {
                *chan = i;
                *port = ports_map[index][i];
                return 0;
            }
        }
    }

    *chan = -1;
    *port = -1;

    return EINVAL;
}

int mux_reg_get(struct i2c_adapter *adap, struct i2c_client *client)
{
    int ret;
	union i2c_smbus_data i2c_data;
	struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct cpld_data *data = i2c_mux_priv(muxc);
    int i2c_relay_reg = CPLD_I2C_RELAY_REG;
    unsigned long stop_time;
    u32 try_times = 0;

    i2c_relay_reg = CPLD_I2C_RELAY_REG;

	/* Start a round of trying to claim the bus */
	stop_time = jiffies + msecs_to_jiffies(CPLD_MUX_TIMEOUT);
    do {
        try_times += 1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)

        ret = __i2c_smbus_xfer(adap, client->addr, client->flags,
                    I2C_SMBUS_READ, i2c_relay_reg,
                    I2C_SMBUS_BYTE_DATA, &i2c_data);
#else
        ret = adap->algo->smbus_xfer(adap, client->addr, client->flags,
                            I2C_SMBUS_READ, i2c_relay_reg,
                            I2C_SMBUS_BYTE_DATA, &i2c_data);

#endif /* #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0) */
        if(ret != 0) {
            mdelay(CPLD_MUX_RETRY_WAIT);
        } else {
            break;
        }
    } while (time_before(jiffies, stop_time));

    if(ret != 0) {
        pr_info("Fail to get cpld mux. dev_index(%d) reg(0x%x) retry(%d)\n",
             data->index, i2c_relay_reg, try_times -1);
    }

	return (ret < 0) ? ret : i2c_data.byte;
}

static int _port_block_status_get(struct i2c_adapter *adap, struct i2c_client *client, int port, int is_evt)
{
    int ret;
	union i2c_smbus_data i2c_data;
    int reg = 0;
    u8 mask = MASK_ALL;
    unsigned long stop_time;
    u32 try_times = 0;

    if(port >= 0 && port < NELEMS(ports_block_map)) {
        if(is_evt == 1) {
            reg = ports_block_map[port].evt_reg;
        } else {
            reg = ports_block_map[port].reg;
        }
        mask = ports_block_map[port].mask;
    } else {
        return 0;
    }

	/* Start a round of trying to claim the bus */
	stop_time = jiffies + msecs_to_jiffies(CPLD_MUX_TIMEOUT);
    do {
        try_times += 1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)

        ret = __i2c_smbus_xfer(adap, client->addr, client->flags,
                    I2C_SMBUS_READ, reg,
                    I2C_SMBUS_BYTE_DATA, &i2c_data);
#else
        ret = adap->algo->smbus_xfer(adap, client->addr, client->flags,
                            I2C_SMBUS_READ, reg,
                            I2C_SMBUS_BYTE_DATA, &i2c_data);

#endif /* #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0) */
        if(ret != 0) {
            mdelay(CPLD_MUX_RETRY_WAIT);
        } else {
            break;
        }
    } while (time_before(jiffies, stop_time));

	return (ret < 0) ? 0 : (_mask_shift(i2c_data.byte, mask) == 0);
}

static int mux_reg_write(struct i2c_adapter *adap, struct i2c_client *client, u8 val, u32 *try_times)
{
    int ret = 0;
    union i2c_smbus_data i2c_data;
	//struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    //struct cpld_data *data = i2c_mux_priv(muxc);
    int i2c_relay_reg = CPLD_I2C_RELAY_REG;
    unsigned long stop_time;
    u32 tries = 0;

    i2c_data.byte = val;
    //i2c_relay_reg = CPLD_I2C_RELAY_REG;

	/* Start a round of trying to claim the bus */
	stop_time = jiffies + msecs_to_jiffies(CPLD_MUX_TIMEOUT);
    do {

        /*
        *  Write to mux register. Don't use i2c_transfer()/i2c_smbus_xfer()
        *  for this as they will try to lock adapter a second time
        */

        tries += 1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
        ret = __i2c_smbus_xfer(adap, client->addr, client->flags,
                    I2C_SMBUS_WRITE, i2c_relay_reg,
                    I2C_SMBUS_BYTE_DATA, &i2c_data);
#else
        ret = adap->algo->smbus_xfer(adap, client->addr, client->flags,
                            I2C_SMBUS_WRITE, i2c_relay_reg,
                            I2C_SMBUS_BYTE_DATA, &i2c_data);
#endif /* #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0) */

        if(ret != 0) {
            mdelay(CPLD_MUX_RETRY_WAIT);
        } else {
            break;
        }
    } while (time_before(jiffies, stop_time));

    * try_times = tries;
    return ret;
}

int mux_select_chan(struct i2c_mux_core *muxc, u32 chan)
{
    struct cpld_data *data = i2c_mux_priv(muxc);
    struct i2c_client *client = data->client;
    struct device *dev = &client->dev;

    u8 set_val;
    int ret = 0;
    int chan_val = 0;

    switch (data->index)
    {
        case cpld2:
        case cpld3:
        case cpld4:
            set_val = CPLD_I2C_ENABLE_CHN_SEL;
            break;
        default:
            dev_err(dev, "Invalid device index\n");
            ret=EINVAL;
            goto exit;
    }

    if(chan >= data->chip->nchans) {
        dev_err(dev, "Invalid channel (%d)>=(%d)\n",chan, data->chip->nchans);
            ret=EINVAL;
            goto exit;
    }

    chan_val = chans_map[data->index][chan];
    set_val |= chan_val;

    /* Only select the channel if its different from the last channel */
    if (data->last_chan != set_val) {
        u32 try_times = 0;
        int port = ports_map[data->index][chan];
        ret = mux_reg_write(muxc->parent, client, set_val, &try_times);
        if(ret != 0) {
            pr_info("Fail to set cpld mux. port(%d) chan(%d) reg_val(0x%x) retry(%d)\n",
                port, chan, set_val, try_times -1);
        } else {
            if(try_times -1 > 0){
                pr_info("Success to set cpld mux. port(%d) chan(%d) reg_val(0x%x) retry(%d)\n",
                    port, chan, set_val, try_times -1);
            }

            if(_port_block_status_get(muxc->parent, client, port, 0)) {
                pr_info("port(%d) addr(0x%02x) is disabled by CPLD/FPGA\n", port, client->addr);
            }
        }
        data->last_chan = set_val;
    }

exit:
    return ret;
}

int mux_deselect_mux(struct i2c_mux_core *muxc, u32 chan)
{
    struct cpld_data *data = i2c_mux_priv(muxc);
    struct i2c_client *client = data->client;
    struct device *dev = &client->dev;
    s32 idle_state;
    u32 ret = 0;

    idle_state = READ_ONCE(data->idle_state);
    if (idle_state >= 0) {
        /* Set the mux back to a predetermined channel */
        ret = mux_select_chan(muxc, idle_state);
    } else if (idle_state == MUX_IDLE_DISCONNECT) {
        u32 try_times = 0;
        int port = ports_map[data->index][chan];

        /* Deselect active channel */
        if(data->index == fpga) {
            dev_err(dev, "Invalid device index\n");
            return EINVAL;
        } else {
            data->last_chan = CPLD_MUX_CHN_OFF;
        }
        ret = mux_reg_write(muxc->parent, client, data->last_chan, &try_times);
        if(ret != 0) {
            pr_info("Fail to close cpld mux. port(%d) chan(%d) retry(%d)\n",
                port, chan, try_times -1);

        } else {
            if(try_times -1 > 0){
                pr_info("Success to close cpld mux. port(%d) chan(%d) retry(%d)\n",
                    port, chan, try_times -1);
            }
        }
    }

    return ret;
}

ssize_t idle_state_show(struct device *dev,
                    struct device_attribute *attr,
                    char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct cpld_data *data = i2c_mux_priv(muxc);
    int rv = 0;

    rv = sprintf(buf, "%d\n", READ_ONCE(data->idle_state));
    return rv;
}

ssize_t idle_state_store(struct device *dev,
                struct device_attribute *attr,
                const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct cpld_data *data = i2c_mux_priv(muxc);
    int val;
    int ret;

    ret = kstrtoint(buf, 0, &val);
    if (ret < 0)
        return ret;

    if (val != MUX_IDLE_AS_IS && val != MUX_IDLE_DISCONNECT &&
        (val < 0 || val >= data->chip->nchans))
        return -EINVAL;

    i2c_lock_bus(muxc->parent, I2C_LOCK_SEGMENT);

    WRITE_ONCE(data->idle_state, val);

    /*
     * Set the mux into a state consistent with the new
     * idle_state.
     */
    if (data->last_chan || val != MUX_IDLE_DISCONNECT)
        ret = mux_deselect_mux(muxc, 0);

    i2c_unlock_bus(muxc->parent, I2C_LOCK_SEGMENT);

    return ret < 0 ? ret : count;
}

int mux_init(struct device *dev)
{
    int ret = 0;
    int reg_val = 0;
    int num = 0;
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct cpld_data *data = i2c_mux_priv(muxc);
    u32 chan_off = CPLD_MUX_CHN_OFF;

    data->chip = &chips[data->index];

    data->idle_state = MUX_IDLE_DISCONNECT;
    if (device_property_read_u32(dev, "idle-state", &data->idle_state)) {
        if (device_property_read_bool(dev, "i2c-mux-idle-disconnect"))
            data->idle_state = MUX_IDLE_DISCONNECT;
    }

    /*
    * Write the mux register at addr to verify
    * that the mux is in fact present. This also
    * initializes the mux to a channel
    * or disconnected state.
    */

    if(data->chip->nchans > 0){
        // close multiplexer channel
        chan_off = CPLD_MUX_CHN_OFF;
        // enable mux functionality for the legacy I2C interface instead of using FPGA.

        if ((reg_val = _cpld_reg_read(dev, CPLD_I2C_CONTROL_REG, MASK_ALL)) < 0) {
            dev_err(dev, "Fail to read CPLD I2C control register\n");
            ret = reg_val;
            goto exit;
        }

        reg_val |= (CPLD_I2C_ENABLE_BRIDGE);
        ret = _cpld_reg_write(dev, CPLD_I2C_CONTROL_REG, reg_val);
        if (ret < 0) {
            dev_err(dev, "Fail to enable mux functionality for the legacy I2C interface\n");
            goto exit;
        }

        if (data->idle_state >= 0) {
            /* Set the mux back to a predetermined channel */
            ret = mux_select_chan(muxc, data->idle_state);
        } else {
            u32 try_times = 0;
            data->last_chan = chan_off;
            ret = mux_reg_write(muxc->parent, client, data->last_chan, &try_times);
        }

        if (ret < 0) {
            goto exit;
        }
    }

    /* Now create an adapter for each channel */
    for (num = 0; num < data->chip->nchans; num++) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,10,0)
        ret = i2c_mux_add_adapter(muxc, 0, num, 0);
#else
        ret = i2c_mux_add_adapter(muxc, 0, num);
#endif
        if (ret)
            goto exit;
    }

    return 0;

exit:
    mux_cleanup(dev);
    return ret;
}

void mux_cleanup(struct device *dev)
{
    int reg_val = 0;
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);

    if (_cpld_reg_write(dev, CPLD_I2C_RELAY_REG, CPLD_MUX_CHN_OFF) < 0) {
        dev_err(dev, "Fail to write CPLD I2C relay register\n");
        goto exit;
    }

    if ((reg_val = _cpld_reg_read(dev, CPLD_I2C_CONTROL_REG, MASK_ALL)) < 0) {
        dev_err(dev, "Fail to read CPLD I2C control register\n");
        goto exit;
    }

    reg_val &= ~(CPLD_I2C_ENABLE_BRIDGE);
    if (_cpld_reg_write(dev, CPLD_I2C_CONTROL_REG, reg_val) < 0) {
        dev_err(dev, "Fail to write CPLD I2C control register\n");
        goto exit;
    }

exit:
    i2c_mux_del_adapters(muxc);
}
