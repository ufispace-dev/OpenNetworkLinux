/* header file for i2c cpld driver of ufispace_s9720_56ed
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

#ifndef UFISPACE_S9720_56ED_CPLD_MAIN_H
#define UFISPACE_S9720_56ED_CPLD_MAIN_H

#include <linux/module.h>
#include <linux/i2c.h>
#include <dt-bindings/mux/mux.h>
#include <linux/i2c-mux.h>
#include <linux/version.h>

/* CPLD device index value */
enum cpld_id {
    cpld1,
    cpld2,
    fpga,
    cpld4,
    cpld5,
};


#define NONE_REG                                0x100

/* CPLD Common */
#define CPLD_HW_BUILD_REV_REG                   0x01
#define CPLD_VERSION_REG                        0x02
#define CPLD_ID_REG                             0x03
#define CPLD_SUB_VERSION_REG                    0x04
#define EVENT_DETECT_CTRL_REG                   0x3F

/* CPLD 1 registers */
#define CPLD_SKU_ID_REG                         0x00
#define CPLD_CHIP_TYPE_REG                      0x05
//#define CPLD_SKU_EXT_REG                      0x06
// Interrupt status
#define MAC_INTR_REG                            0x10
#define MB_RGB_INTR_REG                         0x11
#define TOP_RGB_INTR_REG                        0x12
#define CPLD_25GPHY_INTR_REG                    0x13
#define CPLD_FRU_INTR_REG                       0x14
#define NTM_INTR_REG                            0x15
#define THERMAL_ALERT_1_REG                     0x16
#define THERMAL_ALERT_2_REG                     0x17
#define MISC_INTR_REG                           0x1B
#define SYSTEM_INTR_REG                         0x1C
// Interrupt mask
#define MAC_INTR_MASK_REG                       0x20
#define MB_RGB_INTR_MASK_REG                    0x21
#define TOP_RGB_INTR_MASK_REG                   0x22
#define CPLD_25GPHY_INTR_MASK_REG               0x23
#define CPLD_FRU_INTR_MASK_REG                  0x24
#define NTM_INTR_MASK_REG                       0x25
#define THERMAL_ALERT_1_MASK_REG                0x26
#define THERMAL_ALERT_2_MASK_REG                0x27
#define MISC_INTR_MASK_REG                      0x2B
#define SYSTEM_INTR_MASK_REG                    0x2C
// Interrupt event
#define MAC_INTR_EVENT_REG	                    0x30
#define MB_RGB_INTR_EVENT_REG	                0x31
#define TOP_RGB_INTR_EVENT_REG	                0x32
#define CPLD_25GPHY_INTR_EVENT_REG              0x33
#define CPLD_FRU_INTR_EVENT_REG                 0x34
#define NTM_INTR_EVENT_REG                      0x35
#define THERMAL_ALERT_INTR_1_REG                0x36
#define THERMAL_ALERT_INTR_2_REG                0x37
#define MISC_INTR_EVENT_REG                     0x3B

// Reset ctrl
#define MAC_RST_REG                             0x40
#define MB_RGB_RST_REG                          0x41
#define BMC_NTM_RST_REG                         0x42
#define USB_REDRIVER_RST_REG                    0x43
#define USB_OCP_RCVRY_REG                       0x44
#define CPLD_RST_REG                            0x45
#define MUX_RST_REG                             0x46
#define MISC_RST_REG                            0x47
#define PUSHBTN_REG                             0x4C
// Sys status
#define CPU_MISC_REG                            0x50
#define PSU_STATUS_REG                          0x51
#define SYS_PW_STATUS_REG                       0x52
#define MAC_STATUS_1_REG                        0x53
#define MAC_STATUS_3_REG                        0x56
#define MB_RGB_SERBOOT_REG                      0x57
#define JA_INDICATOR_REG                        0x5B
// Mux ctrl
#define MUX_CTRL_REG                            0x5C
#define TIMING_CTRL_REG                         0x5E
#define SFP_LED_SELECT_REG                      0x5D
// Thermal Status
#define MAC_THERMAL_TEMP_REG                    0x78
// Led ctrl
#define SYSTEM_LED_CTRL_1_REG                   0x80
#define SYSTEM_LED_CTRL_2_REG                   0x81
#define SYSTEM_LED_CTRL_3_REG                   0x82
#define SFP28_LED_REG                           0x83
// Power good status
#define MAC_PG_REG                              0x90
#define MAC_HBM_PG_REG                          0x91
#define MISC_PG_1_REG                           0x92
#define MISC_PG_2_REG                           0x93
// Voltage monitor
#define VOLMON_REG                              0xA0
// Debug
#define MAC_KBP_INTR_DEBUG_REG                  0xE0
#define MB_RGB_INTR_DEBUG_REG                   0xE1
#define TOP_RGB_INTR_DEBUG_REG                  0xE2
#define CPLD_25GPHY_INTR_DEBUG_REG              0xE3
#define CPLD_FRU_INTR_DEBUG_REG                 0xE4
#define NTM_INTR_DEBUG_REG                      0xE5
#define THERMAL_ALERT_DEBUG_REG                 0xE6
#define MISC_INTR_DEBUG_REG                     0xEB

/* CPLD 2 */
// Board info
#define CPLD_REVISION_REG                       0x02
#define CPLD_ID_REG                             0x03
#define CPLD_BUILD_ID_REG                       0x04
#define CPLD_CHIP_TYPE_REG                      0x05
// Interrupt
#define QSFPDD_NIF_20_27_INTR_REG               0x10
#define QSFPDD_NIF_28_35_INTR_REG               0x11
// Port present
#define QSFPDD_NIF_20_27_ABS_REG                0x13
#define QSFPDD_NIF_28_35_ABS_REG                0x14
// Fuse interrupt
#define QSFPDD_NIF_20_27_FUSE_INTR_REG          0x16
#define QSFPDD_NIF_28_35_FUSE_INTR_REG          0x17
// Interrupt
#define QSFPDD_FAB_10_17_INTR_REG               0x19
#define QSFPDD_FAB_18_19_INTR_REG               0x1A
// Port present
#define QSFPDD_FAB_10_17_ABS_REG                0x1B
#define QSFPDD_FAB_18_19_ABS_REG                0x1C
// Fuse interrupt
#define QSFPDD_FAB_10_17_FUSE_INTR_REG          0x1D
#define QSFPDD_FAB_18_19_FUSE_INTR_REG          0x1E
// Interrupt mask
#define QSFPDD_NIF_20_27_INTR_MASK_REG          0x20
#define QSFPDD_NIF_28_35_INTR_MASK_REG          0x21
// Port present mask
#define QSFPDD_NIF_20_27_ABS_MASK_REG           0x23
#define QSFPDD_NIF_28_35_ABS_MASK_REG           0x24
// Fuse interrupt mask
#define QSFPDD_NIF_20_27_FUSE_INTR_MASK_REG     0x26
#define QSFPDD_NIF_28_35_FUSE_INTR_MASK_REG     0x27
// Interrupt mask
#define QSFPDD_FAB_10_17_INTR_MASK_REG          0x29
#define QSFPDD_FAB_18_19_INTR_MASK_REG          0x2A
// Port present mask
#define QSFPDD_FAB_10_17_ABS_MASK_REG           0x2B
#define QSFPDD_FAB_18_19_ABS_MASK_REG           0x2C
// Fuse interrupt mask
#define QSFPDD_FAB_10_17_FUSE_INTR_MASK_REG     0x2D
#define QSFPDD_FAB_18_19_FUSE_INTR_MASK_REG     0x2E
// Interrupt event
#define QSFPDD_NIF_20_27_INTR_EVENT_REG         0x30
#define QSFPDD_NIF_28_35_INTR_EVENT_REG         0x31
// Present event
#define QSFPDD_NIF_20_27_ABS_EVENT_REG          0x33
#define QSFPDD_NIF_28_35_ABS_EVENT_REG          0x34
// Fuse interrupt event
#define QSFPDD_NIF_20_27_FUSE_INTR_EVENT_REG    0x36
#define QSFPDD_NIF_28_35_FUSE_INTR_EVENT_REG    0x37
// Interrupt event
#define QSFPDD_FAB_10_17_INTR_EVENT_REG         0x39
#define QSFPDD_FAB_18_19_INTR_EVENT_REG         0x3A
// Present event
#define QSFPDD_FAB_10_17_ABS_EVENT_REG          0x3B
#define QSFPDD_FAB_18_19_ABS_EVENT_REG          0x3C
// Fuse interrupt event
#define QSFPDD_FAB_10_17_FUSE_INTR_EVENT_REG    0x3D
#define QSFPDD_FAB_18_19_FUSE_INTR_EVENT_REG    0x3E

// Port reset
#define QSFPDD_NIF_20_27_RST_REG                0x40
#define QSFPDD_NIF_28_35_RST_REG                0x41
#define QSFPDD_FAB_10_17_RST_REG                0x44
#define QSFPDD_FAB_18_19_RST_REG                0x45
// Port LP mode
#define QSFPDD_FAB_10_17_LP_REG                 0x4B
#define QSFPDD_FAB_18_19_LP_REG                 0x4C
#define QSFPDD_NIF_20_27_LP_REG                 0x48
#define QSFPDD_NIF_28_35_LP_REG                 0x49
// FAB port led control
#define QSFPDD_FAB_LED_10_11_STATUS_REG         0x80
#define QSFPDD_FAB_LED_12_13_STATUS_REG         0x81
#define QSFPDD_FAB_LED_14_15_STATUS_REG         0x82
#define QSFPDD_FAB_LED_16_17_STATUS_REG         0x83
#define QSFPDD_FAB_LED_18_19_STATUS_REG         0x84
// FPGA Channel Select
#define FPGA_QSFPDD_PORT_CH_SEL_1_REG           0xB5

/* CPLD 3 */
// change to FPGA
#define FPGA_VER_1_REG                          0x02
#define FPGA_ID_REG                             0x03
#define FPGA_VER_2_REG                          0x04
#define FPGA_DEV_INFO_REG                       0x05
#define SFP28_TS_REG                            0x0A
#define SFP28_RS_REG                            0x0B
#define SFP28_TX_DIS_REG                        0x0C
#define SFP28_TX_FLT_REG                        0x10
#define SFP28_RX_LOS_REG                        0x11
#define SFP28_ABS_REG                           0x12
#define SFP28_TX_FLT_MASK_REG                   0x20
#define SFP28_RX_LOS_MASK_REG                   0x21
#define SFP28_ABS_MASK_REG                      0x22
#define SFP28_TX_FLT_EVENT_REG                  0x30
#define SFP28_RX_LOS_EVENT_REG                  0x31
#define SFP28_ABS_EVENT_REG                     0x32

/* CPLD 4 */
// Interrupt status
#define QSFPDD_NIF_0_7_INTR_REG                 0x10
#define QSFPDD_NIF_8_15_INTR_REG                0x11
#define QSFPDD_NIF_16_19_INTR_REG               0x12
// Port present
#define QSFPDD_NIF_0_7_ABS_REG                  0x13
#define QSFPDD_NIF_8_15_ABS_REG                 0x14
#define QSFPDD_NIF_16_19_ABS_REG                0x15
// Fuse interrupt
#define QSFPDD_NIF_0_7_FUSE_INTR_REG            0x16
#define QSFPDD_NIF_8_15_FUSE_INTR_REG           0x17
#define QSFPDD_NIF_16_19_FUSE_INTR_REG          0x18
// Interrupt status
#define QSFPDD_FAB_0_7_INTR_REG                 0x19
#define QSFPDD_FAB_8_9_INTR_REG                 0x1A
// Port present
#define QSFPDD_FAB_0_7_ABS_REG                  0x1B
#define QSFPDD_FAB_8_9_ABS_REG                  0x1C
// Fuse interrupt
#define QSFPDD_FAB_0_7_FUSE_INTR_REG            0x1D
#define QSFPDD_FAB_8_9_FUSE_INTR_REG            0x1E
// Interrupt mask
#define QSFPDD_NIF_0_7_INTR_MASK_REG            0x20
#define QSFPDD_NIF_8_15_INTR_MASK_REG           0x21
#define QSFPDD_NIF_16_19_INTR_MASK_REG          0x22
// Present mask
#define QSFPDD_NIF_0_7_ABS_MASK_REG             0x23
#define QSFPDD_NIF_8_15_ABS_MASK_REG            0x24
#define QSFPDD_NIF_16_19_ABS_MASK_REG           0x25
// Fuse interrupt mask
#define QSFPDD_NIF_0_7_FUSE_INTR_MASK_REG       0x26
#define QSFPDD_NIF_8_15_FUSE_INTR_MASK_REG      0x27
#define QSFPDD_NIF_16_19_FUSE_INTR_MASK_REG     0x28
// Interrupt mask
#define QSFPDD_FAB_0_7_INTR_MASK_REG            0x29
#define QSFPDD_FAB_8_9_INTR_MASK_REG            0x2A
// Present mask
#define QSFPDD_FAB_0_7_ABS_MASK_REG             0x2B
#define QSFPDD_FAB_8_9_ABS_MASK_REG             0x2C
// Fuse interrupt mask
#define QSFPDD_FAB_0_7_FUSE_INTR_MASK_REG       0x2D
#define QSFPDD_FAB_8_9_FUSE_INTR_MASK_REG       0x2E
// Interrupt event
#define QSFPDD_NIF_0_7_INTR_EVENT_REG           0x30
#define QSFPDD_NIF_8_15_INTR_EVENT_REG          0x31
#define QSFPDD_NIF_16_19_INTR_EVENT_REG         0x32
// Present event
#define QSFPDD_NIF_0_7_ABS_EVENT_REG            0x33
#define QSFPDD_NIF_8_15_ABS_EVENT_REG           0x34
#define QSFPDD_NIF_16_19_ABS_EVENT_REG          0x35
// Fuse interrupt event
#define QSFPDD_NIF_0_7_FUSE_INTR_EVENT_REG      0x36
#define QSFPDD_NIF_8_15_FUSE_INTR_EVENT_REG     0x37
#define QSFPDD_NIF_16_19_FUSE_INTR_EVENT_REG    0x38
// Interrupt event
#define QSFPDD_FAB_0_7_INTR_EVENT_REG           0x39
#define QSFPDD_FAB_8_9_INTR_EVENT_REG           0x3A
// Present event
#define QSFPDD_FAB_0_7_ABS_EVENT_REG            0x3B
#define QSFPDD_FAB_8_9_ABS_EVENT_REG            0x3C
// Fuse interrupt event
#define QSFPDD_FAB_0_7_FUSE_INTR_EVENT_REG      0x3D
#define QSFPDD_FAB_8_9_FUSE_INTR_EVENT_REG      0x3E
// Port reset
#define QSFPDD_NIF_0_7_RST_REG                  0x40
#define QSFPDD_NIF_8_15_RST_REG                 0x41
#define QSFPDD_NIF_16_19_RST_REG                0x42
#define QSFPDD_FAB_0_7_RST_REG                  0x44
#define QSFPDD_FAB_8_9_RST_REG                  0x45
// Port LP mode
#define QSFPDD_NIF_0_7_LP_REG                   0x48
#define QSFPDD_NIF_8_15_LP_REG                  0x49
#define QSFPDD_NIF_16_19_LP_REG                 0x4A
#define QSFPDD_FAB_0_7_LP_REG                   0x4B
#define QSFPDD_FAB_8_9_LP_REG                   0x4C

// FAB port led control
#define QSFPDD_FAB_LED_0_1_STATUS_REG           0x80
#define QSFPDD_FAB_LED_2_3_STATUS_REG           0x81
#define QSFPDD_FAB_LED_4_5_STATUS_REG           0x82
#define QSFPDD_FAB_LED_6_7_STATUS_REG           0x83
#define QSFPDD_FAB_LED_8_9_STATUS_REG           0x84
// FPGA Channel Select
#define FPGA_QSFPDD_PORT_CH_SEL_2_REG           0xB5


/* CPLD 5 */
#define TOP_RGB_INTR_1_REG                      0x11
#define TOP_RGB_INTR_2_REG                      0x12
#define THERMAL_ALERT_REG                       0x16
#define SYSTEM_INTR_REG                         0x1C
#define MB_RGB_INTR_MASK_1_REG                  0x21
#define MB_RGB_INTR_MASK_2_REG                  0x22
#define THERMAL_ALERT_MASK_REG                  0x26
#define SYSTEM_INTR_MASK_REG                    0x2C
#define TOP_RGB_INTR_EVENT_1_REG                0x31
#define TOP_RGB_INTR_EVENT_2_REG                0x32
#define THERMAL_ALERT_EVENT_REG                 0x36
#define TOP_RGB_RST_1_REG                       0x41
#define TOP_RGB_RST_2_REG                       0x42
#define TOP_RGB_SERBOOT_1_REG                   0x57
#define TOP_RGB_SERBOOT_2_REG                   0x58

//I2C RELAY
#define CPLD_I2C_CONTROL_REG                            0xB0
#define CPLD_I2C_RELAY_REG                              0xB5

//FPGA  **SFP28_P 0-1 : mgmt port / P2-3 : Data port 36-37
#define FPGA_VERSION_REG                                0x02
#define FPGA_BUILD_REG                                  0x04
#define FPGA_CHIP_REG                                   0x05
#define FPGA_SFP28_PORT_0_3_RATE_SEL_REG                0x0B
#define FPGA_SFP28_PORT_0_3_TX_DIS_REG                  0x0C
#define FPGA_SFP28_PORT_0_3_TX_FAULT_REG                0x10
#define FPGA_SFP28_PORT_0_3_RX_LOS_REG                  0x11
#define FPGA_SFP28_PORT_0_3_PRES_REG                    0x12
#define FPGA_SFP28_PORT_0_3_STUCK_REG                   0x14
#define FPGA_SFP28_PORT_0_3_TX_FAULT_MASK_REG           0x20
#define FPGA_SFP28_PORT_0_3_RX_LOS_MASK_REG             0x21
#define FPGA_SFP28_PORT_0_3_PRES_MASK_REG               0x22
#define FPGA_SFP28_PORT_0_3_STUCK_MASK_REG              0x23
#define FPGA_SFP28_PORT_0_3_TX_FAULT_EVENT_REG          0x30
#define FPGA_SFP28_PORT_0_3_RX_LOS_EVENT_REG            0x31
#define FPGA_SFP28_PORT_0_3_PRES_EVENT_REG              0x32
#define FPGA_SFP28_PORT_0_3_STUCK_EVENT_REG             0x34
#define FPGA_LAN_PORT_RELAY_REG                         0x40

//I2C STUCK
#define QSFPDD_NIF_0_7_STUCK_REG                        0x58
#define QSFPDD_NIF_8_15_STUCK_REG                       0x59
#define QSFPDD_NIF_16_19_STUCK_REG                      0x5A
#define QSFPDD_NIF_20_27_STUCK_REG                      0x58
#define QSFPDD_NIF_28_35_STUCK_REG                      0x59
#define QSFPDD_FAB_0_7_STUCK_REG                        0x56
#define QSFPDD_FAB_8_9_STUCK_REG                        0x57
#define QSFPDD_FAB_10_17_STUCK_REG                      0x56
#define QSFPDD_FAB_18_19_STUCK_REG                      0x57

#define QSFPDD_NIF_0_7_STUCK_MASK_REG                   0x68
#define QSFPDD_NIF_8_15_STUCK_MASK_REG                  0x69
#define QSFPDD_NIF_16_19_STUCK_MASK_REG                 0x6A
#define QSFPDD_NIF_20_27_STUCK_MASK_REG                 0x68
#define QSFPDD_NIF_28_35_STUCK_MASK_REG                 0x69
#define QSFPDD_FAB_0_7_STUCK_MASK_REG                   0x66
#define QSFPDD_FAB_8_9_STUCK_MASK_REG                   0x67
#define QSFPDD_FAB_10_17_STUCK_MASK_REG                 0x66
#define QSFPDD_FAB_18_19_STUCK_MASK_REG                 0x67

#define QSFPDD_NIF_0_7_STUCK_EVENT_REG                  0x78
#define QSFPDD_NIF_8_15_STUCK_EVENT_REG                 0x79
#define QSFPDD_NIF_16_19_STUCK_EVENT_REG                0x7A
#define QSFPDD_NIF_20_27_STUCK_EVENT_REG                0x78
#define QSFPDD_NIF_28_35_STUCK_EVENT_REG                0x79
#define QSFPDD_FAB_0_7_STUCK_EVENT_REG                  0x76
#define QSFPDD_FAB_8_9_STUCK_EVENT_REG                  0x77
#define QSFPDD_FAB_10_17_STUCK_EVENT_REG                0x76
#define QSFPDD_FAB_18_19_STUCK_EVENT_REG                0x77

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
#define MASK_1111_0000       (0xF0)


// MUX
#define CPLD_MAX_NCHANS 30
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
#endif
