/* header file for i2c cpld driver of ufispace_s9620_32e
 *
 * Copyright (C) 2023 UfiSpace Technology Corporation.
 * Jason Tsai <jason.cy.tsai@ufispace.com>
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

#ifndef UFISPACE_S9620_32E_CPLD_MAIN_H
#define UFISPACE_S9620_32E_CPLD_MAIN_H

#include <linux/module.h>
#include <linux/i2c.h>
#include <dt-bindings/mux/mux.h>
#include <linux/i2c-mux.h>
#include <linux/version.h>

/* CPLD device index value */
enum cpld_id {
    cpld1,
    cpld2,
    cpld3,
    cpld4,
    fpga,
};

/*
 *  Normally, the CPLD register range is 0x00-0xff.
 *  Therefore, we define the invalid address 0x100 as NONE_REG
 */

#define NONE_REG                                0x100
#define REG_BASE_MB                             0xE00

/* CPLD Common */
#define CPLD_VERSION_REG                        0x02
#define CPLD_ID_REG                             0x03
#define CPLD_SUB_VERSION_REG                    0x04
#define CPLD_CHIP_TYPE_REG                      0x05
#define EVENT_DETECT_CTRL_REG                   0x3F
#define CPLD_RST_DEBUG_REG                      0xF0

/* CPLD 2/3/4 */
// cpld i2c stuck with x86/fgpa
#define CPLD_I2C_STUCK_REG                      0x1E
#define CPLD_I2C_STUCK_MASK_REG                 0x2E
#define CPLD_I2C_STUCK_EVENT_REG                0x3E

// I2C control
#define CPLD_I2C_CONTROL_REG                    0xB0
//#define FPGA_QSFPDD_PORT_CH_SEL_REG             0xB1
// I2C relay (QSFPDD Port Channel Select)
#define CPLD_I2C_RELAY_REG                      0xB5

/* CPLD 1 registers */
#define CPLD_SKU_ID_REG                         0x00
#define CPLD_HW_BUILD_REV_REG                   0x01

// Interrupt status
#define MAC_INTR_REG                            0x10
#define CPLD_25GPHY_INTR_REG                    0x13
#define CPLD_FRU_INTR_REG                       0x14
#define NTM_INTR_REG                            0x15
#define THERMAL_ALERT_1_INTR_REG                0x16
#define THERMAL_ALERT_2_INTR_REG                0x17
#define THERMAL_ALERT_3_INTR_REG                0x18
#define MISC_INTR_REG                           0x1B
#define SYSTEM_INTR_REG                         0x1C
// Interrupt mask
#define MAC_INTR_MASK_REG                       0x20
#define CPLD_25GPHY_INTR_MASK_REG               0x23
#define CPLD_FRU_INTR_MASK_REG                  0x24
#define NTM_INTR_MASK_REG                       0x25
#define THERMAL_ALERT_1_INTR_MASK_REG           0x26
#define THERMAL_ALERT_2_INTR_MASK_REG           0x27
#define THERMAL_ALERT_3_INTR_MASK_REG           0x28
#define MISC_INTR_MASK_REG                      0x2B
#define SYSTEM_INTR_MASK_REG                    0x2C
// Interrupt event
#define MAC_INTR_EVENT_REG	                    0x30
#define CPLD_25GPHY_INTR_EVENT_REG              0x33
#define CPLD_FRU_INTR_EVENT_REG                 0x34
#define NTM_INTR_EVENT_REG                      0x35
#define THERMAL_ALERT_1_INTR_EVENT_REG          0x36
#define THERMAL_ALERT_2_INTR_EVENT_REG          0x37
#define THERMAL_ALERT_3_INTR_EVENT_REG          0x38
#define MISC_INTR_EVENT_REG                     0x3B

// Reset ctrl
#define MAC_RST_REG                             0x40
#define BIOS_FLASH_RST_REG                      0x41
#define BMC_NTM_RST_REG                         0x43
#define USB_RST_REG                             0x44
#define CPLD_RST_REG                            0x45
#define I2C_MUX_RST_REG                         0x46
#define MISC_RST_REG                            0x48
#define PUSHBTN_REG                             0x4C
#define PSU_RST_REG                             0x4E

// Sys status
#define DAUGHTER_BRD_PRESENT_REG                0x50
#define PSU_STATUS_REG                          0x51
#define SYS_PW_STATUS_REG                       0x52
#define MAC_STATUS_1_REG                        0x53
#define MAC_STATUS_2_REG                        0x54
#define CPU_STATUS_REG                          0x55
#define BIOS_FLASH_MUX_REG                      0x57
#define PHY_BOOT_REG                            0x59
#define MB_EEPROM_WP_REG                        0x5A
#define JA_INDICATOR_REG                        0x5B

// Mux ctrl
#define MUX_CTRL_REG                            0x5C
#define FPGA_CTRL_REG                           0x5D
#define TIMING_CTRL_REG                         0x5E

// Led ctrl
#define SYSTEM_LED_CTRL_1_REG                   0x80
#define SYSTEM_LED_CTRL_2_REG                   0x81
#define SYSTEM_LED_CTRL_3_REG                   0x82
#define LED_CLEAR_REG                           0x85
#define MGMT_LED_CTRL_REG                       0x8A

// Power good status
#define MAC_PG_1_REG                            0x90
#define MAC_PG_2_REG                            0x91
#define MISC_PG_REG                             0x92
#define MAC_PW_EN_REG                           0x93
#define HBM_PW_EN_REG                           0x94
#define MISC_PW_EN_REG                          0x95

// Debug
#define MAC_INTR_DEBUG_REG                      0xE0
#define CPLD_25GPHY_INTR_DEBUG_REG              0xE3
#define CPLD_FRU_INTR_DEBUG_REG                 0xE4
#define NTM_INTR_DEBUG_REG                      0xE5
#define THERMAL_ALERT_DEBUG_1_REG               0xE6
#define THERMAL_ALERT_DEBUG_2_REG               0xE7
#define MISC_INTR_DEBUG_REG                     0xEB

/* CPLD 2/3 */

// Interrupt
#define QSFPDD_0_7_16_23_INTR_REG               0x10
#define QSFPDD_8_15_24_31_INTR_REG              0x11
// Port present
#define QSFPDD_0_7_16_23_ABS_REG                0x14
#define QSFPDD_8_15_24_31_ABS_REG               0x15
// Fuse interrupt
#define QSFPDD_0_7_16_23_FUSE_INTR_REG          0x18
#define QSFPDD_8_15_24_31_FUSE_INTR_REG         0x19
// Port i2c stuck
#define QSFPDD_0_7_16_23_STUCK_REG              0x1A
#define QSFPDD_8_15_24_31_STUCK_REG             0x1B

// Interrupt mask
#define QSFPDD_0_7_16_23_INTR_MASK_REG          0x20
#define QSFPDD_8_15_24_31_INTR_MASK_REG         0x21
// Port present mask
#define QSFPDD_0_7_16_23_ABS_MASK_REG           0x24
#define QSFPDD_8_15_24_31_ABS_MASK_REG          0x25
// Fuse interrupt mask
#define QSFPDD_0_7_16_23_FUSE_INTR_MASK_REG     0x28
#define QSFPDD_8_15_24_31_FUSE_INTR_MASK_REG    0x29
// Port i2c stuck mask
#define QSFPDD_0_7_16_23_STUCK_MASK_REG         0x2A
#define QSFPDD_8_15_24_31_STUCK_MASK_REG        0x2B

// Interrupt event
#define QSFPDD_0_7_16_23_INTR_EVENT_REG         0x30
#define QSFPDD_8_15_24_31_INTR_EVENT_REG        0x31
// Port present event
#define QSFPDD_0_7_16_23_ABS_EVENT_REG          0x34
#define QSFPDD_8_15_24_31_ABS_EVENT_REG         0x35
// Fuse interrupt event
#define QSFPDD_0_7_16_23_FUSE_INTR_EVENT_REG    0x38
#define QSFPDD_8_15_24_31_FUSE_INTR_EVENT_REG   0x39
// Port i2c stuck event
#define QSFPDD_0_7_16_23_STUCK_EVENT_REG        0x3A
#define QSFPDD_8_15_24_31_STUCK_EVENT_REG       0x3B

// Port reset
#define QSFPDD_0_7_16_23_RST_REG                0x40
#define QSFPDD_8_15_24_31_RST_REG               0x41
// Port LP mode
#define QSFPDD_0_7_16_23_LP_REG                 0x44
#define QSFPDD_8_15_24_31_LP_REG                0x45

// Interrupt debug
#define QSFPDD_0_7_16_23_INTR_DEBUG_REG         0xE0
#define QSFPDD_8_15_24_31_INTR_DEBUG_REG        0xE1
// Port present debug
#define QSFPDD_0_7_16_23_ABS_DEBUG_REG          0xE4
#define QSFPDD_8_15_24_31_ABS_DEBUG_REG         0xE5
// Fuse interrupt debug
#define QSFPDD_0_7_16_23_FUSE_INTR_DEBUG_REG    0xE8
#define QSFPDD_8_15_24_31_FUSE_INTR_DEBUG_REG   0xE9

/* CPLD 4 */
#define MGMT_SFP_ABS_REG                        0x10
#define MGMT_SFP_RX_LOS_REG                     0x11
#define MGMT_SFP_TX_FLT_REG                     0x12
#define RETIMER_INTR_REG                        0x13
#define MGMT_SFP_STUCK_REG                      0x1A

#define MGMT_SFP_ABS_MASK_REG                   0x20
#define MGMT_SFP_RX_LOS_MASK_REG                0x21
#define MGMT_SFP_TX_FLT_MASK_REG                0x22
#define RETIMER_INTR_MASK_REG                   0x23
#define MGMT_SFP_STUCK_MASK_REG                 0x2A

#define MGMT_SFP_ABS_EVENT_REG                  0x30
#define MGMT_SFP_RX_LOS_EVENT_REG               0x31
#define MGMT_SFP_TX_FLT_EVENT_REG               0x32
#define RETIMER_INTR_EVENT_REG                  0x33
#define MGMT_SFP_STUCK_EVENT_REG                0x3A

#define BCM81381_RST_REG                        0x40

#define MGMT_SFP_TX_DIS_REG                     0x53
#define MGMT_SFP_RS_REG                         0x54
#define MGMT_SFP_TS_REG                         0x55

#define MGMT_SFP_ABS_DEBUG_REG                  0xE0
#define MGMT_SFP_RX_LOS_DEBUG_REG               0xE1
#define MGMT_SFP_TX_FLT_DEBUG_REG               0xE2
#define RETIMER_INTR_DEBUG_REG                  0xE3

/* FPGA */
#define FPGA_VERSION_REG                        0x02
#define FPGA_ID_REG                             0x03
#define FPGA_SUB_VERSION_REG                    0x04
#define FPGA_CHIP_REG                           0x05
#define FPGA_I2C_STUCK_REG                      0x13
#define FPGA_I2C_STUCK_MASK_REG                 0x23
#define FPGA_I2C_STUCK_EVENT_REG                0x33
#define FPGA_I2C_STUCK_DEBUG_REG                0xE3
#define FPGA_MAC_ROV_1_REG                      0x54
#define FPGA_MAC_ROV_2_REG                      0x55

//MASK
#define MASK_ALL                                (0xFF)
#define MASK_NONE                               (0x00)
#define MASK_0000_0001                          (0x01)
#define MASK_0000_0010                          (0x02)
#define MASK_0000_0011                          (0x03)
#define MASK_0000_0100                          (0x04)
#define MASK_0000_0110                          (0x06)
#define MASK_0000_0111                          (0x07)
#define MASK_0000_1000                          (0x08)
#define MASK_0000_1011                          (0x0B)
#define MASK_0000_1111                          (0x0F)
#define MASK_0001_0000                          (0x10)
#define MASK_0001_1111                          (0x1F)
#define MASK_0001_1000                          (0x18)
#define MASK_0010_0000                          (0x20)
#define MASK_0011_1000                          (0x38)
#define MASK_0011_1111                          (0x3F)
#define MASK_0100_0000                          (0x40)
#define MASK_0110_0111                          (0x67)
#define MASK_0111_0000                          (0x70)
#define MASK_0111_1111                          (0x7F)
#define MASK_1000_0000                          (0x80)
#define MASK_1000_0001                          (0x81)
#define MASK_1100_0000                          (0xC0)
#define MASK_1111_0000                          (0xF0)


// MUX
#define CPLD_MAX_NCHANS 16
#define CPLD_MUX_TIMEOUT                        1400
#define CPLD_MUX_RETRY_WAIT                     200
#define CPLD_MUX_CHN_OFF                        (0x0)
//#define FPGA_MUX_CHN_OFF                        (0x0)
#define CPLD_I2C_ENABLE_BRIDGE                  MASK_1000_0000
#define CPLD_I2C_ENABLE_CHN_SEL                 MASK_1000_0000
//#define LAN_PORT_RELAY_ENABLE                   MASK_1000_0000

/* common manipulation */
#define INVALID(i, min, max)    ((i < min) || (i > max) ? 1u : 0u)

struct cpld_data {
    int index;                  /* CPLD index */
    struct mutex access_lock;   /* mutex for cpld access */
    u8 access_reg;              /* register to access */

    const struct chip_desc *chip;
    u32 last_chan;       /* last register value */
    /* MUX_IDLE_AS_IS, MUX_IDLE_DISCONNECT or >= 0 for channel */
    s32 idle_state;

    struct i2c_client *client;
    raw_spinlock_t lock;
    u8 hw_id;
};

struct chip_desc {
    u8 nchans;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
    struct i2c_device_identity id;
#endif /* #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0) */
};

/*
 *  Generally, the color bit for CPLD is 4 bits, and there are 16 color sets available.
 *  The color bit for GPIO is 2 bits (representing two GPIO pins), and there are 4 color sets.
 *  Therefore, we use the 16 color sets available for our application.
 */
#define COLOR_VAL_MAX           16

typedef enum {
    LED_COLOR_DARK,
    LED_COLOR_GREEN,
    LED_COLOR_YELLOW,
    LED_COLOR_RED,
    LED_COLOR_BLUE,
    LED_COLOR_GREEN_BLINK,
    LED_COLOR_YELLOW_BLINK,
    LED_COLOR_RED_BLINK,
    LED_COLOR_BLUE_BLINK,
    LED_COLOR_CYAN=100,
    LED_COLOR_MAGENTA,
    LED_COLOR_WHITE,
    LED_COLOR_CYAN_BLINK,
    LED_COLOR_MAGENTA_BLINK,
    LED_COLOR_WHITE_BLINK,
} s3ip_led_status_e;

typedef enum {
    TYPE_LED_UNNKOW = 0,
    // Blue
    TYPE_LED_1_SETS,

    // Green, Yellow
    TYPE_LED_2_SETS,

    // Red, Green, Blue, Yellow, Cyan, Magenta, white
    TYPE_LED_7_SETS,
    TYPE_LED_SETS_MAX,
} led_type_e;

typedef enum {
    PORT_NONE_BLOCK = 0,
    PORT_BLOCK      = 1,
} port_block_status_e;

typedef struct
{
    short int val;
    int status;
} color_obj_t;

typedef struct  {
    int type;
    u8 reg;
    u8 mask;
    u8 color_mask;
    u8 data_type;
    color_obj_t color_obj[COLOR_VAL_MAX];
} led_node_t;


u8 _mask_shift(u8 val, u8 mask);
u8 _cpld_reg_write(struct device *dev, u8 reg, u8 reg_val);
u8 _cpld_reg_read(struct device *dev, u8 reg, u8 mask);
int mux_select_chan(struct i2c_mux_core *muxc, u32 chan);
int mux_deselect_mux(struct i2c_mux_core *muxc, u32 chan);
ssize_t idle_state_show(struct device *dev,
            struct device_attribute *attr,
            char *buf);

ssize_t idle_state_store(struct device *dev,
            struct device_attribute *attr,
            const char *buf, size_t count);
int mux_init(struct device *dev);
void mux_cleanup(struct device *dev);
int port_chan_get_from_reg(u8 val, int index, int *chan, int *port);
int mux_reg_get(struct i2c_adapter *adap, struct i2c_client *client);

#endif
