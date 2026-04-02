/* header file for i2c cpld driver of ufispace_s9620_54dc
 *
 * Copyright (C) 2023 UfiSpace Technology Corporation.
 * Alex Hsia <alex.hsia@ufispace.com>
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

#ifndef UFISPACE_S9620_54DC_CPLD_MAIN_H
#define UFISPACE_S9620_54DC_CPLD_MAIN_H

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
    fpga,
};


#define NONE_REG                                0x100

/* CPLD Common */
#define CPLD_HW_BUILD_REV_REG                   0x01
#define CPLD_VERSION_REG                        0x02
#define CPLD_ID_REG                             0x03
#define CPLD_SUB_VERSION_REG                    0x04
#define EVENT_DETECT_CTRL_REG                   0x3F

/* CPLD write protect function */
#define CPLD_WRITE_PROTECT_REG                  0x70


/* CPLD 1 registers */
#define CPLD_SKU_ID_REG                         0x00
#define CPLD_CHIP_TYPE_REG                      0x05
#define CPLD_SKU_EXT_REG                        0xD0
// Interrupt status
#define MAC_INTR_REG                            0x10
// #define MB_RGB_INTR_REG                      0x11
// #define TOP_RGB_INTR_REG                     0x12
#define PHY_INTR_REG                            0x13
#define CPLD_FRU_INTR_REG                       0x14
#define NTM_INTR_REG                            0x15
#define THERMAL_ALERT_REG                       0x17
#define MISC_INTR_REG                           0x1B
#define SYSTEM_INTR_REG                         0x1C
// Interrupt mask
#define MAC_INTR_MASK_REG                       0x20
#define PHY_INTR_MASK_REG                       0x23
#define CPLD_FRU_INTR_MASK_REG                  0x24
#define NTM_INTR_MASK_REG                       0x25
#define THERMAL_ALERT_MASK_REG                  0x27
#define MISC_INTR_MASK_REG                      0x2B
#define SYSTEM_INTR_MASK_REG                    0x2C
// Interrupt event
#define MAC_INTR_EVENT_REG	                    0x30
#define PHY_INTR_EVENT_REG                      0x33
#define CPLD_FRU_INTR_EVENT_REG                 0x34
#define NTM_INTR_EVENT_REG                      0x35
#define THERMAL_ALERT_EVENT_REG                 0x37
#define MISC_INTR_EVENT_REG                     0x3B
#define SYSTEM_INTR_EVENT_REG                   0x3C
// Reset ctrl
#define MAC_PHY_RST_REG                         0x40
#define BIOS_FLASH_RST_REG                      0x41
#define BMC_NTM_RST_REG                         0x43
#define USB_RST_REG                             0x44
#define MISC_RST_1_REG                          0x48
#define MISC_RST_2_REG                          0x49
#define PUSHBTN_REG                             0x4C
// Sys status
#define NTM_SSD_REG                             0x50
#define PSU_STATUS_REG                          0x51
#define SYS_PW_STATUS_REG                       0x52
#define MAC_STATUS_1_REG                        0x53
#define MAC_STATUS_2_REG                        0x54
#define EEPROM_PROTECT_REG                      0x56
#define PHY_SERBOOT_REG                         0x57
#define MISC_REG                                0x5B
// Mux ctrl
#define MUX_CTRL_REG                            0x5C
#define BIOS_FLASH_MUX_REG                      0x5D
#define TIMING_CTRL_REG                         0x5E

// Led ctrl
#define SYSTEM_LED_CTRL_1_REG                   0x80
#define SYSTEM_LED_CTRL_2_REG                   0x81
#define SYSTEM_LED_CTRL_3_REG                   0x82
#define PORT_LED_REG                            0x85
// Power good status
#define MAC_PG_REG                              0x90
#define CLK_FPGA_PG_REG                         0x91
#define MISC_PG_REG                             0x92
// Debug
#define MAC_INTR_DEBUG_REG                      0xE0
#define PHY_INTR_DEBUG_REG                      0xE3
#define CPLD_FRU_INTR_DEBUG_REG                 0xE4
#define NTM_INTR_DEBUG_REG                      0xE5
#define THERMAL_ALERT_DEBUG_REG                 0xE7
#define MISC_INTR_DEBUG_REG                     0xEB

/* CPLD 2 */
// Board info
#define CPLD_VERSION_REG                        0x02
#define CPLD_ID_REG                             0x03
#define CPLD_SUB_VERSION_REG                    0x04
#define CPLD_CHIP_TYPE_REG                      0x05
// Interrupt
#define PORT_48_53_INTR_REG                     0x10
// Port present
#define PORT_0_7_ABS_REG                        0x14
#define PORT_8_15_ABS_REG                       0x15
#define PORT_16_23_ABS_REG                      0x16
#define PORT_24_27_ABS_REG                      0x17
// Present mask
#define PORT_0_7_ABS_MASK_REG                   0x24
#define PORT_8_15_ABS_MASK_REG                  0x25
#define PORT_16_23_ABS_MASK_REG                 0x26
#define PORT_24_27_ABS_MASK_REG                 0x27
// Plugging event
#define PORT_0_7_ABS_EVENT_REG                  0x34
#define PORT_8_15_ABS_EVENT_REG                 0x35
#define PORT_16_23_ABS_EVENT_REG                0x36
#define PORT_24_27_ABS_EVENT_REG                0x37
// SFP28 Port Tx Disable 
#define PORT_0_7_TXDIS_REG                      0x40
#define PORT_8_15_TXDIS_REG                     0x41
#define PORT_16_23_TXDIS_REG                    0x42
#define PORT_24_27_TXDIS_REG                    0x43
// SFP28 Port Rate Select
#define PORT_0_7_RS_REG                         0x44
#define PORT_8_15_RS_REG                        0x45
#define PORT_16_23_RS_REG                       0x46
#define PORT_24_27_RS_REG                       0x47
// SFP28 Port Rxloss
#define PORT_0_7_RXLOS_REG                      0x18
#define PORT_8_15_RXLOS_REG                     0x19
#define PORT_16_23_RXLOS_REG                    0x1A
#define PORT_24_27_RXLOS_REG                    0x1B
// SFP28 Port Rxloss Mask
#define PORT_0_7_RXLOS_MASK_REG                 0x28
#define PORT_8_15_RXLOS_MASK_REG                0x29
#define PORT_16_23_RXLOS_MASK_REG               0x2A
#define PORT_24_27_RXLOS_MASK_REG               0x2B
// SFP28 Port Rxloss Event
#define PORT_0_7_RXLOS_EVENT_REG                0x38
#define PORT_8_15_RXLOS_EVENT_REG               0x39
#define PORT_16_23_RXLOS_EVENT_REG              0x3A
#define PORT_24_27_RXLOS_EVENT_REG              0x3B
// SFP28 Port Tx Fault
#define PORT_0_7_TXFLT_REG                      0x1C
#define PORT_8_15_TXFLT_REG                     0x1D
#define PORT_16_23_TXFLT_REG                    0x1E
#define PORT_24_27_TXFLT_REG                    0x1F
// SFP28 Port Tx Fault Mask
#define PORT_0_7_TXFLT_MASK_REG                 0x2C
#define PORT_8_15_TXFLT_MASK_REG                0x2D
#define PORT_16_23_TXFLT_MASK_REG               0x2E
#define PORT_24_27_TXFLT_MASK_REG               0x2F
// SFP28 Port Tx Fault Event
#define PORT_0_7_TXFLT_EVENT_REG                0x3C
#define PORT_8_15_TXFLT_EVENT_REG               0x3D
#define PORT_16_23_TXFLT_EVENT_REG              0x3E
#define PORT_24_27_TXFLT_EVENT_REG              0x3F
// SFP28 Port I2C stuck status
#define PORT_0_7_I2C_STUCK_REG                  0x5A
#define PORT_8_15_I2C_STUCK_REG                 0x5B
#define PORT_16_23_I2C_STUCK_REG                0x5C
#define PORT_24_27_I2C_STUCK_REG                0x5D
// Port I2C stuck status
#define PORT_I2C_STUCK_STATUS_REG               0x5E
// SFP28 Port I2C stuck mask
#define PORT_0_7_I2C_STUCK_MASK_REG             0x6A
#define PORT_8_15_I2C_STUCK_MASK_REG            0x6B
#define PORT_16_23_I2C_STUCK_MASK_REG           0x6C
#define PORT_24_27_I2C_STUCK_MASK_REG           0x6D
// SFP28 Port I2C stuck event
#define PORT_0_7_I2C_STUCK_EVENT_REG            0x7A
#define PORT_8_15_I2C_STUCK_EVENT_REG           0x7B
#define PORT_16_23_I2C_STUCK_EVENT_REG          0x7C
#define PORT_24_27_I2C_STUCK_EVENT_REG          0x7D
// SFP28 Port Led control
#define PORT_0_3_LED_REG                        0x80
#define PORT_4_7_LED_REG                        0x81
#define PORT_8_11_LED_REG                       0x82
#define PORT_12_15_LED_REG                      0x83
#define PORT_16_19_LED_REG                      0x84
#define PORT_20_23_LED_REG                      0x85
#define PORT_24_27_LED_REG                      0x86
// Port Present diagnostic 
#define PORT_0_7_ABS_DIAG_REG                   0xE4
#define PORT_8_15_ABS_DIAG_REG                  0xE5
#define PORT_16_23_ABS_DIAG_REG                 0xE6
#define PORT_24_27_ABS_DIAG_REG                 0xE7
/*/ FPGA Channel Select
#define FPGA_QSFPDD_PORT_CH_SEL_1_REG           0xB5*/

/* CPLD 3 */
// Port present
#define PORT_28_35_ABS_REG                      0x14
#define PORT_36_43_ABS_REG                      0x15
#define PORT_44_51_ABS_REG                      0x16
#define PORT_52_53_ABS_REG                      0x17
// Fuse status
#define PORT_52_53_EFUSE_PG_REG                 0x11
// Interrupt mask
#define PORT_48_53_INTR_MASK_REG                0x20
// Present mask
#define PORT_28_35_ABS_MASK_REG                 0x24
#define PORT_36_43_ABS_MASK_REG                 0x25
#define PORT_44_51_ABS_MASK_REG                 0x26
#define PORT_52_53_ABS_MASK_REG                 0x27
// eFuse power good mask
#define PORT_52_53_EFUSE_PG_MASK_REG            0x21
// Interrupt event
#define PORT_48_53_INTR_EVENT_REG               0x30
// Present event
#define PORT_28_35_ABS_EVENT_REG                0x34
#define PORT_36_43_ABS_EVENT_REG                0x35
#define PORT_44_51_ABS_EVENT_REG                0x36
#define PORT_52_53_ABS_EVENT_REG                0x37
// eFuse power good event
#define PORT_52_53_EFUSE_PG_EVENT_REG           0x31
// SFP28/56 Port Tx Disable 
#define PORT_28_35_TXDIS_REG                    0x40
#define PORT_36_43_TXDIS_REG                    0x41
#define PORT_44_47_TXDIS_REG                    0x42
// QSFP28/DD Port Reset 
#define PORT_48_53_RST_REG                      0x43
// QSFP28/DD Port LPMode
#define PORT_48_53_LP_REG                       0x44
// SFP28/56 Port Rate Select
#define PORT_28_35_RS_REG                       0x45
#define PORT_36_43_RS_REG                       0x46
#define PORT_44_47_RS_REG                       0x47
// SFP28/56 Port Rxloss 
#define PORT_28_35_RXLOS_REG                    0x18
#define PORT_36_43_RXLOS_REG                    0x19
#define PORT_44_47_RXLOS_REG                    0x1A
// SFP28/56 Port Rxloss Mask
#define PORT_28_35_RXLOS_MASK_REG               0x28
#define PORT_36_43_RXLOS_MASK_REG               0x29
#define PORT_44_47_RXLOS_MASK_REG               0x2A
// SFP28/56 Port Rxloss Event
#define PORT_28_35_RXLOS_EVENT_REG              0x38
#define PORT_36_43_RXLOS_EVENT_REG              0x39
#define PORT_44_47_RXLOS_EVENT_REG              0x3A
// SFP28/56 Port Tx Fault 
#define PORT_28_35_TXFLT_REG                    0x1C
#define PORT_36_43_TXFLT_REG                    0x1D
#define PORT_44_47_TXFLT_REG                    0x1E
// SFP28/56 Port Tx Fault Mask
#define PORT_28_35_TXFLT_MASK_REG               0x2C
#define PORT_36_43_TXFLT_MASK_REG               0x2D
#define PORT_44_47_TXFLT_MASK_REG               0x2E
// SFP28/56 Port Tx Event
#define PORT_28_35_TXFLT_EVENT_REG              0x3C
#define PORT_36_43_TXFLT_EVENT_REG              0x3D
#define PORT_44_47_TXFLT_EVENT_REG              0x3E
// SFP28/56 Port I2C stuck status
#define PORT_28_35_I2C_STUCK_REG                0x5A
#define PORT_36_43_I2C_STUCK_REG                0x5B
#define PORT_44_51_I2C_STUCK_REG                0x5C
#define PORT_52_53_I2C_STUCK_REG                0x5D
// Port I2C stuck status
#define PORT_I2C_STUCK_STATUS_REG               0x5E
// SFP28/56 Port I2C stuck mask
#define PORT_28_35_I2C_STUCK_MASK_REG           0x6A
#define PORT_36_43_I2C_STUCK_MASK_REG           0x6B
#define PORT_44_51_I2C_STUCK_MASK_REG           0x6C
#define PORT_52_53_I2C_STUCK_MASK_REG           0x6D
// SFP28/56 Port I2C stuck mask
#define PORT_28_35_I2C_STUCK_EVENT_REG          0x7A
#define PORT_36_43_I2C_STUCK_EVENT_REG          0x7B
#define PORT_44_51_I2C_STUCK_EVENT_REG          0x7C
#define PORT_52_53_I2C_STUCK_EVENT_REG          0x7D
// SFP28/56/QSFP28 Port Led control
#define PORT_28_31_LED_REG                      0x80
#define PORT_32_35_LED_REG                      0x81
#define PORT_36_29_LED_REG                      0x82
#define PORT_40_43_LED_REG                      0x83
#define PORT_44_47_LED_REG                      0x84
#define PORT_48_LED_REG                         0x85
#define PORT_49_LED_REG                         0x86
#define PORT_50_LED_REG                         0x87
#define PORT_51_LED_REG                         0x88
#define PORT_52_53_LED_REG                      0x89
// Port interrupt diagnostic 
#define PORT_48_53_INTR_DIAG_REG                0xE0
// Port present diagnostic
#define PORT_28_35_ABS_DIAG_REG                 0xE4
#define PORT_36_43_ABS_DIAG_REG                 0xE5
#define PORT_44_51_ABS_DIAG_REG                 0xE6
#define PORT_52_53_ABS_DIAG_REG                 0xE7
// Port eFuse PG diagnostic
#define PORT_52_53_EFUSE_PG_DIAG_REG            0xE8


/*/ FPGA Channel Select
#define FPGA_QSFPDD_PORT_CH_SEL_2_REG           0xB5*/

/* FPGA */
#define FPGA_VER_1_REG                          0x02
#define FPGA_ID_REG                             0x03
#define FPGA_VER_2_REG                          0x04
#define FPGA_DEV_INFO_REG                       0x05
#define FPGA_LAN_PORT_RELAY_REG                 0x40
//I2C RELAY
#define CPLD_I2C_CONTROL_REG                    0xA0
//#define FPGA_PORT_CH_SEL_REG                  0xA1
#define CPLD_I2C_RELAY_REG                      0xA5

//MASK
#define MASK_ALL             (0xFF)
#define MASK_NONE            (0x00)
#define MASK_0000_0001       (0x01)
#define MASK_0000_0010       (0x02)
#define MASK_0000_0011       (0x03)
#define MASK_0000_0100       (0x04)
#define MASK_0000_0111       (0x07)
#define MASK_0000_1000       (0x08)
#define MASK_0000_1111       (0x0F)
#define MASK_0001_0000       (0x10)
#define MASK_0001_1111       (0x1F)
#define MASK_0001_1000       (0x18)
#define MASK_0010_0000       (0x20)
#define MASK_0011_1000       (0x38)
#define MASK_0011_1111       (0x3F)
#define MASK_0100_0000       (0x40)
#define MASK_0111_0000       (0x70)
#define MASK_1000_0000       (0x80)
#define MASK_1100_0000       (0xC0)
#define MASK_1110_0000       (0xE0)
#define MASK_1111_0000       (0xF0)

// CPLD write protect
#define WP_DISABLE_VALUE                   0xFF

// MUX
#define CPLD_MAX_NCHANS 32
#define CPLD_MUX_TIMEOUT                   1400
#define CPLD_MUX_RETRY_WAIT                200
#define CPLD_MUX_CHN_OFF                   (0x0)
#define FPGA_MUX_CHN_OFF                   (0x0)
#define CPLD_I2C_ENABLE_BRIDGE             MASK_1000_0000
#define CPLD_I2C_ENABLE_CHN_SEL            MASK_1000_0000
#define FPGA_LAN_PORT_RELAY_ENABLE         MASK_1000_0000

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
};

typedef enum {
    PORT_NONE_BLOCK = 0,
    PORT_BLOCK      = 1,
} port_block_status_e;

struct chip_desc {
    u8 nchans;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
    struct i2c_device_identity id;
#endif /* #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0) */
};

u8 _mask_shift(u8 val, u8 mask);
int _cpld_reg_write(struct device *dev, u8 reg, u8 reg_val);
int _cpld_reg_read(struct device *dev, u8 reg, u8 mask);
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