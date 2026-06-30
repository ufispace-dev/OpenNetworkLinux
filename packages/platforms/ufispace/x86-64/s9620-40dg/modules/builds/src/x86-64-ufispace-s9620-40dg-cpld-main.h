/* header file for i2c cpld driver of ufispace_s9620_40dg
 *
 * Copyright (C) 2025 UfiSpace Technology Corporation.
 * Zack Yen <zack.yen@ufispace.com>
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

#ifndef UFISPACE_S9620_40DG_CPLD_H
#define UFISPACE_S9620_40DG_CPLD_H

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

/* CPLD common information registers */
// & cpld_common
#define CPLD_VERSION_REG                         0x02
#define CPLD_ID_REG                              0x03
#define CPLD_BUILD_REG                           0x04
#define CPLD_CHIP_TYPE_REG                       0x05
#define EVENT_DETECT_CTRL_REG                    0x3F
#define MODULE_RESET_REG                         0xF0
#define CPLD_TEST_REG                            0xFF

/* CPLD 2/3/4 registers */

// I2C control
#define CPLD_I2C_CONTROL_REG                     0xB0
#define CPLD_I2C_RELAY_REG                       0xB5

/******************************************************************************
 * CPLD 1                                                                     *
 ******************************************************************************/
// & cpld1
/* Board information */
#define BRD_SKU_ID_REG                           0x00
#define BRD_HW_BUILD_REV_REG                     0x01
#define CPLD_BOARD_EXT_ID_REG                    0x06

/* CPLD information */
#define GDDR6_ID_REG                             0x07
#define GDDR6_ID_FUNC_REG                        0x08

/* Interrupt Status */
#define CLK_PTP_INTR_REG                         0x10
#define PHY_INTR_REG                             0x13
#define TOP_BRD_CPLD_FRU_INTR_REG                0x14
#define MAIN_BRD_CPLD_INTR_REG                   0x15
#define THERMAL_INTR_REG                         0x16
#define USB_SSD_INTR_REG                         0x17
#define CPU_NMI_INTR_REG                         0x19
#define OUT_STATUS_INTR_REG                      0x1C

/* Interrupt Mask */
#define CLK_PTP_INTR_MASK_REG                    0x20
#define PHY_INTR_MASK_REG                        0x23
#define TOP_BRD_CPLD_FRU_INTR_MASK_REG           0x24
#define MAIN_BRD_CPLD_INTR_MASK_REG              0x25
#define THERMAL_INTR_MASK_REG                    0x26
#define USB_SSD_INTR_MASK_REG                    0x27
#define CPU_NMI_INTR_MASK_REG                    0x29
#define OUT_STATUS_INTR_MASK_REG                 0x2C

/* Interrupt Event */
#define CLK_PTP_INTR_EVENT_REG                   0x30
#define PHY_INTR_EVENT_REG                       0x33
#define TOP_BRD_CPLD_FRU_INTR_EVENT_REG          0x34
#define MAIN_BRD_CPLD_INTR_EVENT_REG             0x35
#define THERMAL_INTR_EVENT_REG                   0x36
#define USB_SSD_INTR_EVENT_REG                   0x37
#define CPU_NMI_INTR_EVENT_REG                   0x39
#define OUT_STATUS_INTR_EVENT_REG                0x3C

/* Reset */
#define BIOS_FLASH_RESET_CTRL_REG                0x41
#define CPU_BOARD_CTRL_REG                       0x42
#define BMC_PHY_RESET_CTRL_REG                   0x43
#define USB_RESET_CTRL_REG                       0x44
#define TOP_I2C_MUX_RESET_REG                    0x46

/* Misc Status Control */
#define DAUGHTER_BRD_ABS_REG                     0x50
#define PSU_STATUS_REG                           0x51
#define SYSTEM_PWR_STATUS_REG                    0x52
#define CPU_STATUS_REG                           0x55
#define PHY_BOOT_CTRL_REG                        0x59
#define WD_STATUS_REG                            0x5A
#define TIMING_CTRL_STATUS_REG                   0x5B
#define MUX_CTRL_REG                             0x5C
#define PWR_SYSTEM_CTRL_REG                      0x5E

/* SERBOOT Control */
#define SERBOOT_UFM_STORE_1_REG                  0x61
#define SERBOOT_UFM_WRITE_1_REG                  0x62

/* Write Protect */
#define WRITE_PROTECT_1_REG                      0x70
#define WRITE_PROTECT_2_REG                      0x71
#define EXT_CTRL_REG                             0x7F

/* LED Control */
#define SYSTEM_LED_CTRL_1_REG                    0x80
#define SYSTEM_LED_CTRL_2_REG                    0x81
#define SYSTEM_LED_CTRL_3_REG                    0x82
#define LED_CLEAR_REG                            0x85

/* Power Status */
#define CPLD1_PWR_STATUS_REG                     0x90

/* Internal Control */
#define QSFP28_0_5_PWR_EN_REG                    0xB0
#define QSFPDD_12_15_PWR_EN_REG                  0xB1
#define QSFP28_6_11_PWR_EN_REG                   0xB2
#define SFP56_16_23_PWR_EN_0_REG                 0xB3
#define SFP56_24_31_PWR_EN_1_REG                 0xB4
#define SFP56_32_39_PWR_EN_2_REG                 0xB5


/* Timing Control */
#define OCXO_GNSS_ID_REG                         0xC0
#define CLK_PTP_RESET_REG                        0xC2
#define CLK_TIMING_CTRL_REG                      0xC3
#define GNSS_STATUS_REG                          0xC4
#define TIMING_STATUS_REG                        0xC5

/* Modified SPEC */
#define CLK_BUFFER_EN_CTRL_REG                   0xC6
#define MAIN_I2C_MUX_RESET_REG                   0xC7


/******************************************************************************
 * CPLD 2                                                                     *
 ******************************************************************************/
// & cpld2
 /* Ports Interrupt Status */
#define QSFP28_0_5_ABS_REG                       0x10
#define QSFPDD_12_15_ABS_REG                     0x11
#define QSFP28_0_5_INTR_REG                      0x12
#define QSFPDD_12_15_INTR_REG                    0x13
#define QSFP28_0_5_EFUSE_PG_REG                  0x14
#define QSFPDD_12_15_EFUSE_PG_REG                0x15
#define QSFP28_0_5_I2C_STUCK_REG                 0x16
#define QSFPDD_12_15_I2C_STUCK_REG               0x17
#define CPLD2_I2C_STUCK_REG                      0x18
#define CPLD2_TO_CPLD1_INTR_REG                  0x1C

/* Interrupt Mask */
#define QSFP28_0_5_ABS_MASK_REG                  0x20
#define QSFPDD_12_15_ABS_MASK_REG                0x21
#define QSFP28_0_5_INTR_MASK_REG                 0x22
#define QSFPDD_12_15_INTR_MASK_REG               0x23
#define QSFP28_0_5_EFUSE_PG_MASK_REG             0x24
#define QSFPDD_12_15_EFUSE_PG_MASK_REG           0x25
#define QSFP28_0_5_I2C_STUCK_MASK_REG            0x26
#define QSFPDD_12_15_I2C_STUCK_MASK_REG          0x27
#define CPLD2_I2C_STUCK_MASK_REG                 0x28
#define CPLD2_TO_CPLD1_INTR_MASK_REG             0x2C

/* Interrupt Event */
#define QSFP28_0_5_ABS_EVENT_REG                 0x30
#define QSFPDD_12_15_ABS_EVENT_REG               0x31
#define QSFP28_0_5_INTR_EVENT_REG                0x32
#define QSFPDD_12_15_INTR_EVENT_REG              0x33
#define QSFP28_0_5_EFUSE_PG_EVENT_REG            0x34
#define QSFPDD_12_15_EFUSE_PG_EVENT_REG          0x35
#define QSFP28_0_5_I2C_STUCK_EVENT_REG           0x36
#define QSFPDD_12_15_I2C_STUCK_EVENT_REG         0x37
#define CPLD2_I2C_STUCK_EVENT_REG                0x38
#define CPLD2_TO_CPLD1_INTR_EVENT_REG            0x3C

/* Reset */
#define QSFP28_0_5_RESET_REG                     0x40
#define QSFPDD_12_15_RESET_REG                   0x41
#define QSFP28_0_5_LPMODE_REG                    0x42
#define QSFPDD_12_15_LPMODE_REG                  0x43
#define CLK_EN_CTRL_REG                          0x44

/* PSU Control */
#define PSU_CTRL_REG                             0x50

/* Ports LED control */
/* 0x80~0x85: QSFP28 ports LED control */
#define QSFP28_P0_LED_CTRL_REG                   0x80
#define QSFP28_P1_LED_CTRL_REG                   0x81
#define QSFP28_P2_LED_CTRL_REG                   0x82
#define QSFP28_P3_LED_CTRL_REG                   0x83
#define QSFP28_P4_LED_CTRL_REG                   0x84
#define QSFP28_P5_LED_CTRL_REG                   0x85
/* 0x86~0x87: QSFP-DD ports LED control */
#define QSFPDD_P12_P13_LED_CTRL_REG              0x86
#define QSFPDD_P14_P15_LED_CTRL_REG              0x87

/* Power Status */
#define CPLD2_PWR_STATUS_0_REG                   0x90
#define CPLD2_PWR_STATUS_1_REG                   0x91

/* Interrupt Debug */
#define DBG_QSFP28_0_5_ABS_REG                   0xE0
#define DBG_QSFPDD_12_15_ABS_REG                 0xE1
#define DBG_QSFP28_0_5_INTR_REG                  0xE2
#define DBG_QSFPDD_12_15_INTR_REG                0xE3
#define DBG_QSFP28_0_5_EFUSE_PG_REG              0xE4
#define DBG_QSFPDD_12_15_EFUSE_PG_REG            0xE5


/******************************************************************************
 * CPLD 3                                                                     *
 ******************************************************************************/
// & cpld3
/* CPLD information */
#define GNSS_MODEL_ID_REG                        0x06
#define OCXO_ID_REG                              0x07

/* Interrupt Status */
#define QSFP28_6_11_ABS_REG                      0x10
#define QSFP28_6_11_INTR_REG                     0x11
#define QSFP28_6_11_EFUSE_PG_REG                 0x12
#define MAC_INTR_REG                             0x13
#define MAIN_THERMAL_INTR_REG                    0x14
#define FAN_ABS_REG                              0x15
#define IO_OC_INTR_REG                           0x16
#define QSFPDD_6_11_I2C_STUCK_REG                0x17
#define CPLD3_I2C_STUCK_REG                      0x18
#define CPLD3_TO_CPLD1_INTR_REG                  0x1C

/* Interrupt Mask */
#define QSFP28_6_11_ABS_MASK_REG                 0x20
#define QSFP28_6_11_INTR_MASK_REG                0x21
#define QSFP28_6_11_EFUSE_PG_MASK_REG            0x22
#define MAC_INTR_MASK_REG                        0x23
#define MAIN_THERMAL_INTR_MASK_REG               0x24
#define FAN_ABS_MASK_REG                         0x25
#define IO_OC_INTR_MASK_REG                      0x26
#define QSFPDD_6_11_I2C_STUCK_MASK_REG           0x27
#define CPLD3_I2C_STUCK_MASK_REG                 0x28
#define CPLD3_TO_CPLD1_INTR_MASK_REG             0x2C

/* Interrupt Event */
#define QSFP28_6_11_ABS_EVENT_REG                0x30
#define QSFP28_6_11_INTR_EVENT_REG               0x31
#define QSFP28_6_11_EFUSE_PG_EVENT_REG           0x32
#define MAC_INTR_EVENT_REG                       0x33
#define MAIN_THERMAL_INTR_EVENT_REG              0x34
#define FAN_ABS_EVENT_REG                        0x35
#define IO_OC_INTR_EVENT_REG                     0x36
#define QSFPDD_6_11_I2C_STUCK_EVENT_REG          0x37
#define CPLD3_I2C_STUCK_EVENT_REG                0x38
#define CPLD3_TO_CPLD1_INTR_EVENT_REG            0x3C

/* Reset */
#define QSFP28_6_11_RESET_REG                    0x40
#define QSFP28_6_11_LPMODE_REG                   0x41
#define MAC_RESET_REG                            0x42
#define USB_QSPI_RESET_REG                       0x44

/* Timing Status & Misc Control */
#define CLK_TIMING_STATUS_1_REG                  0x50
#define CLK_TIMING_STATUS_2_REG                  0x51
#define ROV_STATUS_REG                           0x52
#define MISC_CONTROL_REG                         0x53

/* Control Registers */
#define GNSS_CTRL_REG                            0x61
#define SYNCE_CTRL_REG                           0x66

/* FAN Control */
#define FAN_SPEED_READ_MODE_REG                  0x72
#define FAN_RPM_READ_VALUE_0_REG                 0x73
#define FAN_RPM_READ_VALUE_1_REG                 0x74

/* FAN LED Control */
#define FAN_1_2_LED_CTRL_REG                     0x7A
#define FAN_3_4_LED_CTRL_REG                     0x7B
#define FAN_5_LED_CTRL_REG                       0x7C


/* Status Registers */
#define CPLD3_PWR_STATUS_REG                     0x90
#define FAN_PWR_STATUS_REG                       0x91

/* Interrupt Debug */
#define DBG_QSFP28_6_11_ABS_REG                  0xE0
#define DBG_QSFP28_6_11_INTR_REG                 0xE1
#define DBG_QSFP28_6_11_EFUSE_PG_REG             0xE2
#define DBG_MAC_INTR_REG                         0xE3
#define DBG_THERMAL_INTR_REG                     0xE4
#define DBG_MISC_INTR_REG                        0xE5
#define DBG_IO_OC_INTR_REG                       0xE6


/******************************************************************************
 * CPLD 4                                                                     *
 ******************************************************************************/
// & cpld4
/* Interrupt Status */
#define SFP56_16_23_ABS_REG                      0x10
#define SFP56_24_31_ABS_REG                      0x11
#define SFP56_32_39_ABS_REG                      0x12
#define SFP56_16_23_RX_LOS_REG                   0x13
#define SFP56_24_31_RX_LOS_REG                   0x14
#define SFP56_32_39_RX_LOS_REG                   0x15
#define SFP56_16_23_TX_FAULT_REG                 0x16
#define SFP56_24_31_TX_FAULT_REG                 0x17
#define SFP56_32_39_TX_FAULT_REG                 0x18
#define SFP56_16_23_I2C_STUCK_REG                0x19
#define SFP56_24_31_I2C_STUCK_REG                0x1A
#define SFP56_32_39_I2C_STUCK_REG                0x1B
#define CPLD4_TO_CPLD1_INTR_REG                  0x1C
#define CPLD4_I2C_STUCK_REG                      0x1D

/* Interrupt Mask */
#define SFP56_16_23_ABS_MASK_REG                 0x20
#define SFP56_24_31_ABS_MASK_REG                 0x21
#define SFP56_32_39_ABS_MASK_REG                 0x22
#define SFP56_16_23_RX_LOS_MASK_REG              0x23
#define SFP56_24_31_RX_LOS_MASK_REG              0x24
#define SFP56_32_39_RX_LOS_MASK_REG              0x25
#define SFP56_16_23_TX_FAULT_MASK_REG            0x26
#define SFP56_24_31_TX_FAULT_MASK_REG            0x27
#define SFP56_32_39_TX_FAULT_MASK_REG            0x28
#define SFP56_16_23_I2C_STUCK_MASK_REG           0x29
#define SFP56_24_31_I2C_STUCK_MASK_REG           0x2A
#define SFP56_32_39_I2C_STUCK_MASK_REG           0x2B
#define CPLD4_TO_CPLD1_INTR_MASK_REG             0x2C
#define CPLD4_I2C_STUCK_MASK_REG                 0x2D

/* Interrupt Event */
#define SFP56_16_23_ABS_EVENT_REG                0x30
#define SFP56_24_31_ABS_EVENT_REG                0x31
#define SFP56_32_39_ABS_EVENT_REG                0x32
#define SFP56_16_23_RX_LOS_EVENT_REG             0x33
#define SFP56_24_31_RX_LOS_EVENT_REG             0x34
#define SFP56_32_39_RX_LOS_EVENT_REG             0x35
#define SFP56_16_23_TX_FAULT_EVENT_REG           0x36
#define SFP56_24_31_TX_FAULT_EVENT_REG           0x37
#define SFP56_32_39_TX_FAULT_EVENT_REG           0x38
#define SFP56_16_23_I2C_STUCK_EVENT_REG          0x39
#define SFP56_24_31_I2C_STUCK_EVENT_REG          0x3A
#define SFP56_32_39_I2C_STUCK_EVENT_REG          0x3B
#define CPLD4_TO_CPLD1_INTR_EVENT_REG            0x3C
#define CPLD4_I2C_STUCK_EVENT_REG                0x3D

/* SFP Control */
#define SFP56_16_23_TX_DISABLE_REG               0x50
#define SFP56_24_31_TX_DISABLE_REG               0x51
#define SFP56_32_39_TX_DISABLE_REG               0x52
#define SFP56_16_23_RATE_SEL_REG                 0x53
#define SFP56_24_31_RATE_SEL_REG                 0x54
#define SFP56_32_39_RATE_SEL_REG                 0x55

/* Power Status */
#define CPLD4_PWR_STATUS_0_REG                   0x90
#define CPLD4_PWR_STATUS_1_REG                   0x91
#define CPLD4_PWR_STATUS_2_REG                   0x92
#define CPLD4_PWR_STATUS_3_REG                   0x93

/* Interrupt Debug */
#define DBG_SFP56_16_23_ABS_REG                  0xE0
#define DBG_SFP56_24_31_ABS_REG                  0xE1
#define DBG_SFP56_32_39_ABS_REG                  0xE2
#define DBG_SFP56_16_23_RX_LOS_REG               0xE3
#define DBG_SFP56_24_31_RX_LOS_REG               0xE4
#define DBG_SFP56_32_39_RX_LOS_REG               0xE5
#define DBG_SFP56_16_23_TX_FAULT_REG             0xE6
#define DBG_SFP56_24_31_TX_FAULT_REG             0xE7
#define DBG_SFP56_32_39_TX_FAULT_REG             0xE8


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
#define MASK_0000_1100                          (0x0C)
#define MASK_0000_1111                          (0x0F)
#define MASK_0001_0000                          (0x10)
#define MASK_0001_1111                          (0x1F)
#define MASK_0001_1000                          (0x18)
#define MASK_0010_0000                          (0x20)
#define MASK_0011_0000                          (0x30)
#define MASK_0011_1000                          (0x38)
#define MASK_0011_1001                          (0x39)
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
#define CPLD_MAX_NCHANS                         24
#define CPLD_MUX_TIMEOUT                        1400
#define CPLD_MUX_RETRY_WAIT                     200
#define CPLD_MUX_CHN_OFF                        (0x0)
//#define FPGA_MUX_CHN_OFF                        (0x0)
#define CPLD_I2C_ENABLE_BRIDGE                  MASK_1000_0000
#define CPLD_I2C_ENABLE_CHN_SEL                 MASK_1000_0000
//#define LAN_PORT_RELAY_ENABLE                   MASK_1000_0000

/* common manipulation */
#define INVALID(i, min, max)    ((i < min) || (i > max) ? 1u : 0u)

#define SOFT_LATCH_SIZE                         (0x100)

/* CPLD sysfs attributes index  */
enum cpld_sysfs_attributes {
    CPLD_MINOR_VER,
    CPLD_MAJOR_VER,
    CPLD_ID,
    CPLD_BUILD_VER,
    CPLD_CHIP_TYPE,
    CPLD_VERSION_H,
    EVENT_DETECT_CTRL,
    MODULE_RESET,
    CPLD_TEST,
    CPLD_I2C_CONTROL,
    CPLD_I2C_RELAY,
    BRD_SKU_ID,
    BRD_HW_ID,
    BRD_DEPH_ID,
    BRD_BUILD_ID,
    BRD_ID_TYPE,
    CPLD_BOARD_EXT_ID,
    GDDR6_ID,
    GDDR6_ID_FUNC,
    CLK_PTP_INTR,
    PHY_INTR,
    TOP_BRD_CPLD_FRU_INTR,
    PSU0_INTR,
    PSU1_INTR,
    CPLD2_INTR,
    MAIN_BRD_CPLD_INTR,
    CPLD3_INTR,
    CPLD4_INTR,
    MB_ETH_INTR,
    FAN_INTR,
    THERMAL_INTR,
    USB_SSD_INTR,
    CPU_NMI_INTR,
    OUT_STATUS_INTR,
    CLK_PTP_INTR_MASK,
    PHY_INTR_MASK,
    TOP_BRD_CPLD_FRU_INTR_MASK,
    PSU0_INTR_MASK,
    PSU1_INTR_MASK,
    CPLD2_INTR_MASK,
    CPLD2_IO_INTR_MASK,
    MAIN_BRD_CPLD_INTR_MASK,
    CPLD3_INTR_MASK,
    CPLD4_INTR_MASK,
    MB_ETH_INTR_MASK,
    MB_PTP_INTR_MASK,
    FAN_INTR_MASK,
    THERMAL_INTR_MASK,
    USB_SSD_INTR_MASK,
    CPU_NMI_INTR_MASK,
    OUT_STATUS_INTR_MASK,
    CLK_PTP_INTR_EVENT,
    PHY_INTR_EVENT,
    TOP_BRD_CPLD_FRU_INTR_EVENT,
    MAIN_BRD_CPLD_INTR_EVENT,
    THERMAL_INTR_EVENT,
    USB_SSD_INTR_EVENT,
    CPU_NMI_INTR_EVENT,
    OUT_STATUS_INTR_EVENT,
    BTN_FP_RESET,
    SPI_BIOS_RESET,
    CPU_BOARD_CTRL,
    RGB_1_RESET,
    RGB_0_RESET,
    BMC_LPC_RESET,
    BMC_PCIE_RESET,
    CPLD_TO_BMC_SYS_RESET,
    CPLD_TO_CPU_RESET,
    USB_PWR_EN,
    USB_SIE_RESET,
    I2C_MUX_SYS_RESET,
    I2C_MUX_SMBUS_RESET,
    I2C_MUX_QSFP28_RESET,
    I2C_MUX_SFP_RESET,
    MB_I2C_RESET,
    BMC_PRESENT,
    SATA_SSD1_PRESENT,
    SATA_SSD2_PRESENT,
    PSU0_PRESENT,
    PSU1_PRESENT,
    PSU0_VIN_PG,
    PSU1_VIN_PG,
    PSU0_VOUT_PG,
    PSU1_VOUT_PG,
    PSU_STATUS,
    CPU_BOOT_DONE,
    CPU_PG,
    CPU_STATUS,
    PHY_BOOT_CTRL,
    WD_STATUS,
    TIMING_CTRL_STATUS,
    SMBUS_PECI_DIS,
    I2C_PSU0_MUX_SEL,
    I2C_PSU1_MUX_SEL,
    I2C_CPLD_MUX_SEL,
    BMC_USB_MUX_SEL,
    UART_CPU_BMC_MUX_SEL,
    UART_MUX_SEL,
    PWR_SYSTEM_CTRL,
    SERBOOT_UFM_STORE,
    SERBOOT_UFM_WRITE,
    CPLD_WRITE_PROTECT_1,
    CPLD_WRITE_PROTECT_2,
    EXT_CTRL,
    CPLD_SYSTEM_LED_SYS,
    SYSTEM_LED_STATUS,
    SYSTEM_LED_SPEED,
    SYSTEM_LED_BLINK,
    SYSTEM_LED_ONOFF,
    CPLD_SYSTEM_LED_FAN,
    FAN_LED_STATUS,
    FAN_LED_SPEED,
    FAN_LED_BLINK,
    FAN_LED_ONOFF,
    CPLD_SYSTEM_LED_PWR,
    PWR_LED_STATUS,
    PWR_LED_SPEED,
    PWR_LED_BLINK,
    PWR_LED_ONOFF,
    CPLD_SYSTEM_LED_GNSS,
    GNSS_LED_STATUS,
    GNSS_LED_SPEED,
    GNSS_LED_BLINK,
    GNSS_LED_ONOFF,
    CPLD_SYSTEM_LED_SYNC,
    SYNC_LED_STATUS,
    SYNC_LED_SPEED,
    SYNC_LED_BLINK,
    SYNC_LED_ONOFF,
    CPLD_SYSTEM_LED_ID,
    ID_LED_STATUS,
    ID_LED_SPEED,
    ID_LED_BLINK,
    ID_LED_ONOFF,
    LED_CLEAR,
    CPLD1_PWR_STATUS,
    ETH_0_PWR_EN,
    ETH_1_PWR_EN,
    ETH_2_PWR_EN,
    ETH_3_PWR_EN,
    ETH_4_PWR_EN,
    ETH_5_PWR_EN,
    ETH_12_PWR_EN,
    ETH_13_PWR_EN,
    ETH_14_PWR_EN,
    ETH_15_PWR_EN,
    ETH_6_PWR_EN,
    ETH_7_PWR_EN,
    ETH_8_PWR_EN,
    ETH_9_PWR_EN,
    ETH_10_PWR_EN,
    ETH_11_PWR_EN,
    ETH_16_PWR_EN,
    ETH_17_PWR_EN,
    ETH_18_PWR_EN,
    ETH_19_PWR_EN,
    ETH_20_PWR_EN,
    ETH_21_PWR_EN,
    ETH_22_PWR_EN,
    ETH_23_PWR_EN,
    ETH_24_PWR_EN,
    ETH_25_PWR_EN,
    ETH_26_PWR_EN,
    ETH_27_PWR_EN,
    ETH_28_PWR_EN,
    ETH_29_PWR_EN,
    ETH_30_PWR_EN,
    ETH_31_PWR_EN,
    ETH_32_PWR_EN,
    ETH_33_PWR_EN,
    ETH_34_PWR_EN,
    ETH_35_PWR_EN,
    ETH_36_PWR_EN,
    ETH_37_PWR_EN,
    ETH_38_PWR_EN,
    ETH_39_PWR_EN,
    OCXO_GNSS_ID,
    CLK_PTP_RESET,
    CJAP_RESET,
    NTM_RESET,
    GNSS_RESET,
    BITS_RESET,
    CLK_TIMING_CTRL,
    GNSS_STATUS,
    TIMING_STATUS,
    QSFPDD_SEL,
    ETH_28_SEL,
    I2C_MUX_0X76_RESET,
    I2C_MUX_0X75_RESET,
    I2C_MUX_6_11_RESET,
    I2C_MUX_SFP56_16_23_RESET,
    I2C_MUX_SFP56_24_31_RESET,
    I2C_MUX_SFP56_32_39_RESET,
    I2C_MUX_0X71_RESET,
    ETH_0_PRESENT,
    ETH_1_PRESENT,
    ETH_2_PRESENT,
    ETH_3_PRESENT,
    ETH_4_PRESENT,
    ETH_5_PRESENT,
    ETH_12_PRESENT,
    ETH_13_PRESENT,
    ETH_14_PRESENT,
    ETH_15_PRESENT,
    ETH_0_INTR,
    ETH_1_INTR,
    ETH_2_INTR,
    ETH_3_INTR,
    ETH_4_INTR,
    ETH_5_INTR,
    ETH_12_INTR,
    ETH_13_INTR,
    ETH_14_INTR,
    ETH_15_INTR,
    ETH_0_EFUSE_PG,
    ETH_1_EFUSE_PG,
    ETH_2_EFUSE_PG,
    ETH_3_EFUSE_PG,
    ETH_4_EFUSE_PG,
    ETH_5_EFUSE_PG,
    ETH_12_EFUSE_PG,
    ETH_13_EFUSE_PG,
    ETH_14_EFUSE_PG,
    ETH_15_EFUSE_PG,
    ETH_0_I2C_STUCK,
    ETH_1_I2C_STUCK,
    ETH_2_I2C_STUCK,
    ETH_3_I2C_STUCK,
    ETH_4_I2C_STUCK,
    ETH_5_I2C_STUCK,
    ETH_12_I2C_STUCK,
    ETH_13_I2C_STUCK,
    ETH_14_I2C_STUCK,
    ETH_15_I2C_STUCK,
    CPLD2_I2C_STUCK,
    CPLD2_TO_CPLD1_INTR,
    ETH_0_PRESENT_MASK,
    ETH_1_PRESENT_MASK,
    ETH_2_PRESENT_MASK,
    ETH_3_PRESENT_MASK,
    ETH_4_PRESENT_MASK,
    ETH_5_PRESENT_MASK,
    ETH_12_PRESENT_MASK,
    ETH_13_PRESENT_MASK,
    ETH_14_PRESENT_MASK,
    ETH_15_PRESENT_MASK,
    ETH_0_INTR_MASK,
    ETH_1_INTR_MASK,
    ETH_2_INTR_MASK,
    ETH_3_INTR_MASK,
    ETH_4_INTR_MASK,
    ETH_5_INTR_MASK,
    ETH_12_INTR_MASK,
    ETH_13_INTR_MASK,
    ETH_14_INTR_MASK,
    ETH_15_INTR_MASK,
    ETH_0_EFUSE_PG_MASK,
    ETH_1_EFUSE_PG_MASK,
    ETH_2_EFUSE_PG_MASK,
    ETH_3_EFUSE_PG_MASK,
    ETH_4_EFUSE_PG_MASK,
    ETH_5_EFUSE_PG_MASK,
    ETH_12_EFUSE_PG_MASK,
    ETH_13_EFUSE_PG_MASK,
    ETH_14_EFUSE_PG_MASK,
    ETH_15_EFUSE_PG_MASK,
    ETH_0_I2C_STUCK_MASK,
    ETH_1_I2C_STUCK_MASK,
    ETH_2_I2C_STUCK_MASK,
    ETH_3_I2C_STUCK_MASK,
    ETH_4_I2C_STUCK_MASK,
    ETH_5_I2C_STUCK_MASK,
    ETH_12_I2C_STUCK_MASK,
    ETH_13_I2C_STUCK_MASK,
    ETH_14_I2C_STUCK_MASK,
    ETH_15_I2C_STUCK_MASK,
    CPLD2_I2C_STUCK_MASK,
    CPLD2_TO_CPLD1_INTR_MASK,
    ETH_0_PRESENT_EVENT,
    ETH_1_PRESENT_EVENT,
    ETH_2_PRESENT_EVENT,
    ETH_3_PRESENT_EVENT,
    ETH_4_PRESENT_EVENT,
    ETH_5_PRESENT_EVENT,
    ETH_12_PRESENT_EVENT,
    ETH_13_PRESENT_EVENT,
    ETH_14_PRESENT_EVENT,
    ETH_15_PRESENT_EVENT,
    ETH_0_INTR_EVENT,
    ETH_1_INTR_EVENT,
    ETH_2_INTR_EVENT,
    ETH_3_INTR_EVENT,
    ETH_4_INTR_EVENT,
    ETH_5_INTR_EVENT,
    ETH_12_INTR_EVENT,
    ETH_13_INTR_EVENT,
    ETH_14_INTR_EVENT,
    ETH_15_INTR_EVENT,
    ETH_0_EFUSE_PG_EVENT,
    ETH_1_EFUSE_PG_EVENT,
    ETH_2_EFUSE_PG_EVENT,
    ETH_3_EFUSE_PG_EVENT,
    ETH_4_EFUSE_PG_EVENT,
    ETH_5_EFUSE_PG_EVENT,
    ETH_12_EFUSE_PG_EVENT,
    ETH_13_EFUSE_PG_EVENT,
    ETH_14_EFUSE_PG_EVENT,
    ETH_15_EFUSE_PG_EVENT,
    ETH_0_I2C_STUCK_EVENT,
    ETH_1_I2C_STUCK_EVENT,
    ETH_2_I2C_STUCK_EVENT,
    ETH_3_I2C_STUCK_EVENT,
    ETH_4_I2C_STUCK_EVENT,
    ETH_5_I2C_STUCK_EVENT,
    ETH_12_I2C_STUCK_EVENT,
    ETH_13_I2C_STUCK_EVENT,
    ETH_14_I2C_STUCK_EVENT,
    ETH_15_I2C_STUCK_EVENT,
    CPLD2_I2C_STUCK_EVENT,
    CPLD2_TO_CPLD1_INTR_EVENT,
    ETH_0_RESET,
    ETH_1_RESET,
    ETH_2_RESET,
    ETH_3_RESET,
    ETH_4_RESET,
    ETH_5_RESET,
    ETH_12_RESET,
    ETH_13_RESET,
    ETH_14_RESET,
    ETH_15_RESET,
    ETH_0_LPMODE,
    ETH_1_LPMODE,
    ETH_2_LPMODE,
    ETH_3_LPMODE,
    ETH_4_LPMODE,
    ETH_5_LPMODE,
    ETH_12_LPMODE,
    ETH_13_LPMODE,
    ETH_14_LPMODE,
    ETH_15_LPMODE,
    CLK_EN_CTRL,
    PSU_CTRL,
    ETH_12_LED_CTRL,
    ETH_13_LED_CTRL,
    ETH_14_LED_CTRL,
    ETH_15_LED_CTRL,
    CPLD2_PWR_STATUS_0,
    CPLD2_PWR_STATUS_1,
    DBG_QSFP28_0_5_ABS,
    DBG_QSFPDD_12_15_ABS,
    DBG_QSFP28_0_5_INTR,
    DBG_QSFPDD_12_15_INTR,
    DBG_QSFP28_0_5_EFUSE_PG,
    DBG_QSFPDD_12_15_EFUSE_PG,
    GNSS_MODEL_ID,
    OCXO_ID,
    ETH_6_PRESENT,
    ETH_7_PRESENT,
    ETH_8_PRESENT,
    ETH_9_PRESENT,
    ETH_10_PRESENT,
    ETH_11_PRESENT,
    ETH_6_INTR,
    ETH_7_INTR,
    ETH_8_INTR,
    ETH_9_INTR,
    ETH_10_INTR,
    ETH_11_INTR,
    ETH_6_EFUSE_PG,
    ETH_7_EFUSE_PG,
    ETH_8_EFUSE_PG,
    ETH_9_EFUSE_PG,
    ETH_10_EFUSE_PG,
    ETH_11_EFUSE_PG,
    MAC_INTR,
    MAIN_THERMAL_INTR,
    FAN_0_PRESENT,
    FAN_1_PRESENT,
    FAN_2_PRESENT,
    FAN_3_PRESENT,
    FAN_4_PRESENT,
    IO_OC_INTR,
    ETH_6_I2C_STUCK,
    ETH_7_I2C_STUCK,
    ETH_8_I2C_STUCK,
    ETH_9_I2C_STUCK,
    ETH_10_I2C_STUCK,
    ETH_11_I2C_STUCK,
    CPLD3_I2C_STUCK,
    CPLD3_TO_CPLD1_INTR,
    ETH_6_PRESENT_MASK,
    ETH_7_PRESENT_MASK,
    ETH_8_PRESENT_MASK,
    ETH_9_PRESENT_MASK,
    ETH_10_PRESENT_MASK,
    ETH_11_PRESENT_MASK,
    ETH_6_INTR_MASK,
    ETH_7_INTR_MASK,
    ETH_8_INTR_MASK,
    ETH_9_INTR_MASK,
    ETH_10_INTR_MASK,
    ETH_11_INTR_MASK,
    ETH_6_EFUSE_PG_MASK,
    ETH_7_EFUSE_PG_MASK,
    ETH_8_EFUSE_PG_MASK,
    ETH_9_EFUSE_PG_MASK,
    ETH_10_EFUSE_PG_MASK,
    ETH_11_EFUSE_PG_MASK,
    MAC_INTR_MASK,
    MAIN_THERMAL_INTR_MASK,
    FAN_ABS_MASK,
    IO_OC_INTR_MASK,
    ETH_6_I2C_STUCK_MASK,
    ETH_7_I2C_STUCK_MASK,
    ETH_8_I2C_STUCK_MASK,
    ETH_9_I2C_STUCK_MASK,
    ETH_10_I2C_STUCK_MASK,
    ETH_11_I2C_STUCK_MASK,
    CPLD3_I2C_STUCK_MASK,
    CPLD3_TO_CPLD1_INTR_MASK,
    ETH_6_PRESENT_EVENT,
    ETH_7_PRESENT_EVENT,
    ETH_8_PRESENT_EVENT,
    ETH_9_PRESENT_EVENT,
    ETH_10_PRESENT_EVENT,
    ETH_11_PRESENT_EVENT,
    ETH_6_INTR_EVENT,
    ETH_7_INTR_EVENT,
    ETH_8_INTR_EVENT,
    ETH_9_INTR_EVENT,
    ETH_10_INTR_EVENT,
    ETH_11_INTR_EVENT,
    ETH_6_EFUSE_PG_EVENT,
    ETH_7_EFUSE_PG_EVENT,
    ETH_8_EFUSE_PG_EVENT,
    ETH_9_EFUSE_PG_EVENT,
    ETH_10_EFUSE_PG_EVENT,
    ETH_11_EFUSE_PG_EVENT,
    MAC_INTR_EVENT,
    MAIN_THERMAL_INTR_EVENT,
    FAN_ABS_EVENT,
    IO_OC_INTR_EVENT,
    ETH_6_I2C_STUCK_EVENT,
    ETH_7_I2C_STUCK_EVENT,
    ETH_8_I2C_STUCK_EVENT,
    ETH_9_I2C_STUCK_EVENT,
    ETH_10_I2C_STUCK_EVENT,
    ETH_11_I2C_STUCK_EVENT,
    CPLD3_I2C_STUCK_EVENT,
    CPLD3_TO_CPLD1_INTR_EVENT,
    ETH_6_RESET,
    ETH_7_RESET,
    ETH_8_RESET,
    ETH_9_RESET,
    ETH_10_RESET,
    ETH_11_RESET,
    ETH_6_LPMODE,
    ETH_7_LPMODE,
    ETH_8_LPMODE,
    ETH_9_LPMODE,
    ETH_10_LPMODE,
    ETH_11_LPMODE,
    MAC_RESET,
    USB_QSPI_RESET,
    CLK_TIMING_STATUS_1,
    CLK_TIMING_STATUS_2,
    ROV_STATUS,
    MISC_CONTROL,
    GNSS_CTRL,
    SYNCE_CTRL,
    FAN_SPEED_READ_MODE,
    CPLD3_PWR_STATUS,
    DBG_QSFP28_6_11_ABS,
    DBG_QSFP28_6_11_INTR,
    DBG_QSFP28_6_11_EFUSE_PG,
    DBG_MAC_INTR,
    DBG_THERMAL_INTR,
    DBG_MISC_INTR,
    DBG_IO_OC_INTR,
    ETH_16_PRESENT,
    ETH_17_PRESENT,
    ETH_18_PRESENT,
    ETH_19_PRESENT,
    ETH_20_PRESENT,
    ETH_21_PRESENT,
    ETH_22_PRESENT,
    ETH_23_PRESENT,
    ETH_24_PRESENT,
    ETH_25_PRESENT,
    ETH_26_PRESENT,
    ETH_27_PRESENT,
    ETH_28_PRESENT,
    ETH_29_PRESENT,
    ETH_30_PRESENT,
    ETH_31_PRESENT,
    ETH_32_PRESENT,
    ETH_33_PRESENT,
    ETH_34_PRESENT,
    ETH_35_PRESENT,
    ETH_36_PRESENT,
    ETH_37_PRESENT,
    ETH_38_PRESENT,
    ETH_39_PRESENT,
    ETH_16_RX_LOS,
    ETH_17_RX_LOS,
    ETH_18_RX_LOS,
    ETH_19_RX_LOS,
    ETH_20_RX_LOS,
    ETH_21_RX_LOS,
    ETH_22_RX_LOS,
    ETH_23_RX_LOS,
    ETH_24_RX_LOS,
    ETH_25_RX_LOS,
    ETH_26_RX_LOS,
    ETH_27_RX_LOS,
    ETH_28_RX_LOS,
    ETH_29_RX_LOS,
    ETH_30_RX_LOS,
    ETH_31_RX_LOS,
    ETH_32_RX_LOS,
    ETH_33_RX_LOS,
    ETH_34_RX_LOS,
    ETH_35_RX_LOS,
    ETH_36_RX_LOS,
    ETH_37_RX_LOS,
    ETH_38_RX_LOS,
    ETH_39_RX_LOS,
    ETH_16_TX_FAULT,
    ETH_17_TX_FAULT,
    ETH_18_TX_FAULT,
    ETH_19_TX_FAULT,
    ETH_20_TX_FAULT,
    ETH_21_TX_FAULT,
    ETH_22_TX_FAULT,
    ETH_23_TX_FAULT,
    ETH_24_TX_FAULT,
    ETH_25_TX_FAULT,
    ETH_26_TX_FAULT,
    ETH_27_TX_FAULT,
    ETH_28_TX_FAULT,
    ETH_29_TX_FAULT,
    ETH_30_TX_FAULT,
    ETH_31_TX_FAULT,
    ETH_32_TX_FAULT,
    ETH_33_TX_FAULT,
    ETH_34_TX_FAULT,
    ETH_35_TX_FAULT,
    ETH_36_TX_FAULT,
    ETH_37_TX_FAULT,
    ETH_38_TX_FAULT,
    ETH_39_TX_FAULT,
    ETH_16_I2C_STUCK,
    ETH_17_I2C_STUCK,
    ETH_18_I2C_STUCK,
    ETH_19_I2C_STUCK,
    ETH_20_I2C_STUCK,
    ETH_21_I2C_STUCK,
    ETH_22_I2C_STUCK,
    ETH_23_I2C_STUCK,
    ETH_24_I2C_STUCK,
    ETH_25_I2C_STUCK,
    ETH_26_I2C_STUCK,
    ETH_27_I2C_STUCK,
    ETH_28_I2C_STUCK,
    ETH_29_I2C_STUCK,
    ETH_30_I2C_STUCK,
    ETH_31_I2C_STUCK,
    ETH_32_I2C_STUCK,
    ETH_33_I2C_STUCK,
    ETH_34_I2C_STUCK,
    ETH_35_I2C_STUCK,
    ETH_36_I2C_STUCK,
    ETH_37_I2C_STUCK,
    ETH_38_I2C_STUCK,
    ETH_39_I2C_STUCK,
    CPLD4_TO_CPLD1_INTR,
    CPLD4_I2C_STUCK,
    ETH_16_PRESENT_MASK,
    ETH_17_PRESENT_MASK,
    ETH_18_PRESENT_MASK,
    ETH_19_PRESENT_MASK,
    ETH_20_PRESENT_MASK,
    ETH_21_PRESENT_MASK,
    ETH_22_PRESENT_MASK,
    ETH_23_PRESENT_MASK,
    ETH_24_PRESENT_MASK,
    ETH_25_PRESENT_MASK,
    ETH_26_PRESENT_MASK,
    ETH_27_PRESENT_MASK,
    ETH_28_PRESENT_MASK,
    ETH_29_PRESENT_MASK,
    ETH_30_PRESENT_MASK,
    ETH_31_PRESENT_MASK,
    ETH_32_PRESENT_MASK,
    ETH_33_PRESENT_MASK,
    ETH_34_PRESENT_MASK,
    ETH_35_PRESENT_MASK,
    ETH_36_PRESENT_MASK,
    ETH_37_PRESENT_MASK,
    ETH_38_PRESENT_MASK,
    ETH_39_PRESENT_MASK,
    ETH_16_RX_LOS_MASK,
    ETH_17_RX_LOS_MASK,
    ETH_18_RX_LOS_MASK,
    ETH_19_RX_LOS_MASK,
    ETH_20_RX_LOS_MASK,
    ETH_21_RX_LOS_MASK,
    ETH_22_RX_LOS_MASK,
    ETH_23_RX_LOS_MASK,
    ETH_24_RX_LOS_MASK,
    ETH_25_RX_LOS_MASK,
    ETH_26_RX_LOS_MASK,
    ETH_27_RX_LOS_MASK,
    ETH_28_RX_LOS_MASK,
    ETH_29_RX_LOS_MASK,
    ETH_30_RX_LOS_MASK,
    ETH_31_RX_LOS_MASK,
    ETH_32_RX_LOS_MASK,
    ETH_33_RX_LOS_MASK,
    ETH_34_RX_LOS_MASK,
    ETH_35_RX_LOS_MASK,
    ETH_36_RX_LOS_MASK,
    ETH_37_RX_LOS_MASK,
    ETH_38_RX_LOS_MASK,
    ETH_39_RX_LOS_MASK,
    ETH_16_TX_FAULT_MASK,
    ETH_17_TX_FAULT_MASK,
    ETH_18_TX_FAULT_MASK,
    ETH_19_TX_FAULT_MASK,
    ETH_20_TX_FAULT_MASK,
    ETH_21_TX_FAULT_MASK,
    ETH_22_TX_FAULT_MASK,
    ETH_23_TX_FAULT_MASK,
    ETH_24_TX_FAULT_MASK,
    ETH_25_TX_FAULT_MASK,
    ETH_26_TX_FAULT_MASK,
    ETH_27_TX_FAULT_MASK,
    ETH_28_TX_FAULT_MASK,
    ETH_29_TX_FAULT_MASK,
    ETH_30_TX_FAULT_MASK,
    ETH_31_TX_FAULT_MASK,
    ETH_32_TX_FAULT_MASK,
    ETH_33_TX_FAULT_MASK,
    ETH_34_TX_FAULT_MASK,
    ETH_35_TX_FAULT_MASK,
    ETH_36_TX_FAULT_MASK,
    ETH_37_TX_FAULT_MASK,
    ETH_38_TX_FAULT_MASK,
    ETH_39_TX_FAULT_MASK,
    ETH_16_I2C_STUCK_MASK,
    ETH_17_I2C_STUCK_MASK,
    ETH_18_I2C_STUCK_MASK,
    ETH_19_I2C_STUCK_MASK,
    ETH_20_I2C_STUCK_MASK,
    ETH_21_I2C_STUCK_MASK,
    ETH_22_I2C_STUCK_MASK,
    ETH_23_I2C_STUCK_MASK,
    ETH_24_I2C_STUCK_MASK,
    ETH_25_I2C_STUCK_MASK,
    ETH_26_I2C_STUCK_MASK,
    ETH_27_I2C_STUCK_MASK,
    ETH_28_I2C_STUCK_MASK,
    ETH_29_I2C_STUCK_MASK,
    ETH_30_I2C_STUCK_MASK,
    ETH_31_I2C_STUCK_MASK,
    ETH_32_I2C_STUCK_MASK,
    ETH_33_I2C_STUCK_MASK,
    ETH_34_I2C_STUCK_MASK,
    ETH_35_I2C_STUCK_MASK,
    ETH_36_I2C_STUCK_MASK,
    ETH_37_I2C_STUCK_MASK,
    ETH_38_I2C_STUCK_MASK,
    ETH_39_I2C_STUCK_MASK,
    CPLD4_TO_CPLD1_INTR_MASK,
    CPLD4_I2C_STUCK_MASK,
    ETH_16_PRESENT_EVENT,
    ETH_17_PRESENT_EVENT,
    ETH_18_PRESENT_EVENT,
    ETH_19_PRESENT_EVENT,
    ETH_20_PRESENT_EVENT,
    ETH_21_PRESENT_EVENT,
    ETH_22_PRESENT_EVENT,
    ETH_23_PRESENT_EVENT,
    ETH_24_PRESENT_EVENT,
    ETH_25_PRESENT_EVENT,
    ETH_26_PRESENT_EVENT,
    ETH_27_PRESENT_EVENT,
    ETH_28_PRESENT_EVENT,
    ETH_29_PRESENT_EVENT,
    ETH_30_PRESENT_EVENT,
    ETH_31_PRESENT_EVENT,
    ETH_32_PRESENT_EVENT,
    ETH_33_PRESENT_EVENT,
    ETH_34_PRESENT_EVENT,
    ETH_35_PRESENT_EVENT,
    ETH_36_PRESENT_EVENT,
    ETH_37_PRESENT_EVENT,
    ETH_38_PRESENT_EVENT,
    ETH_39_PRESENT_EVENT,
    ETH_16_RX_LOS_EVENT,
    ETH_17_RX_LOS_EVENT,
    ETH_18_RX_LOS_EVENT,
    ETH_19_RX_LOS_EVENT,
    ETH_20_RX_LOS_EVENT,
    ETH_21_RX_LOS_EVENT,
    ETH_22_RX_LOS_EVENT,
    ETH_23_RX_LOS_EVENT,
    ETH_24_RX_LOS_EVENT,
    ETH_25_RX_LOS_EVENT,
    ETH_26_RX_LOS_EVENT,
    ETH_27_RX_LOS_EVENT,
    ETH_28_RX_LOS_EVENT,
    ETH_29_RX_LOS_EVENT,
    ETH_30_RX_LOS_EVENT,
    ETH_31_RX_LOS_EVENT,
    ETH_32_RX_LOS_EVENT,
    ETH_33_RX_LOS_EVENT,
    ETH_34_RX_LOS_EVENT,
    ETH_35_RX_LOS_EVENT,
    ETH_36_RX_LOS_EVENT,
    ETH_37_RX_LOS_EVENT,
    ETH_38_RX_LOS_EVENT,
    ETH_39_RX_LOS_EVENT,
    ETH_16_TX_FAULT_EVENT,
    ETH_17_TX_FAULT_EVENT,
    ETH_18_TX_FAULT_EVENT,
    ETH_19_TX_FAULT_EVENT,
    ETH_20_TX_FAULT_EVENT,
    ETH_21_TX_FAULT_EVENT,
    ETH_22_TX_FAULT_EVENT,
    ETH_23_TX_FAULT_EVENT,
    ETH_24_TX_FAULT_EVENT,
    ETH_25_TX_FAULT_EVENT,
    ETH_26_TX_FAULT_EVENT,
    ETH_27_TX_FAULT_EVENT,
    ETH_28_TX_FAULT_EVENT,
    ETH_29_TX_FAULT_EVENT,
    ETH_30_TX_FAULT_EVENT,
    ETH_31_TX_FAULT_EVENT,
    ETH_32_TX_FAULT_EVENT,
    ETH_33_TX_FAULT_EVENT,
    ETH_34_TX_FAULT_EVENT,
    ETH_35_TX_FAULT_EVENT,
    ETH_36_TX_FAULT_EVENT,
    ETH_37_TX_FAULT_EVENT,
    ETH_38_TX_FAULT_EVENT,
    ETH_39_TX_FAULT_EVENT,
    ETH_16_I2C_STUCK_EVENT,
    ETH_17_I2C_STUCK_EVENT,
    ETH_18_I2C_STUCK_EVENT,
    ETH_19_I2C_STUCK_EVENT,
    ETH_20_I2C_STUCK_EVENT,
    ETH_21_I2C_STUCK_EVENT,
    ETH_22_I2C_STUCK_EVENT,
    ETH_23_I2C_STUCK_EVENT,
    ETH_24_I2C_STUCK_EVENT,
    ETH_25_I2C_STUCK_EVENT,
    ETH_26_I2C_STUCK_EVENT,
    ETH_27_I2C_STUCK_EVENT,
    ETH_28_I2C_STUCK_EVENT,
    ETH_29_I2C_STUCK_EVENT,
    ETH_30_I2C_STUCK_EVENT,
    ETH_31_I2C_STUCK_EVENT,
    ETH_32_I2C_STUCK_EVENT,
    ETH_33_I2C_STUCK_EVENT,
    ETH_34_I2C_STUCK_EVENT,
    ETH_35_I2C_STUCK_EVENT,
    ETH_36_I2C_STUCK_EVENT,
    ETH_37_I2C_STUCK_EVENT,
    ETH_38_I2C_STUCK_EVENT,
    ETH_39_I2C_STUCK_EVENT,
    CPLD4_TO_CPLD1_INTR_EVENT,
    CPLD4_I2C_STUCK_EVENT,
    ETH_16_TX_DISABLE,
    ETH_17_TX_DISABLE,
    ETH_18_TX_DISABLE,
    ETH_19_TX_DISABLE,
    ETH_20_TX_DISABLE,
    ETH_21_TX_DISABLE,
    ETH_22_TX_DISABLE,
    ETH_23_TX_DISABLE,
    ETH_24_TX_DISABLE,
    ETH_25_TX_DISABLE,
    ETH_26_TX_DISABLE,
    ETH_27_TX_DISABLE,
    ETH_28_TX_DISABLE,
    ETH_29_TX_DISABLE,
    ETH_30_TX_DISABLE,
    ETH_31_TX_DISABLE,
    ETH_32_TX_DISABLE,
    ETH_33_TX_DISABLE,
    ETH_34_TX_DISABLE,
    ETH_35_TX_DISABLE,
    ETH_36_TX_DISABLE,
    ETH_37_TX_DISABLE,
    ETH_38_TX_DISABLE,
    ETH_39_TX_DISABLE,
    ETH_16_RATE_SEL,
    ETH_17_RATE_SEL,
    ETH_18_RATE_SEL,
    ETH_19_RATE_SEL,
    ETH_20_RATE_SEL,
    ETH_21_RATE_SEL,
    ETH_22_RATE_SEL,
    ETH_23_RATE_SEL,
    ETH_24_RATE_SEL,
    ETH_25_RATE_SEL,
    ETH_26_RATE_SEL,
    ETH_27_RATE_SEL,
    ETH_28_RATE_SEL,
    ETH_29_RATE_SEL,
    ETH_30_RATE_SEL,
    ETH_31_RATE_SEL,
    ETH_32_RATE_SEL,
    ETH_33_RATE_SEL,
    ETH_34_RATE_SEL,
    ETH_35_RATE_SEL,
    ETH_36_RATE_SEL,
    ETH_37_RATE_SEL,
    ETH_38_RATE_SEL,
    ETH_39_RATE_SEL,
    CPLD4_PWR_STATUS_0,
    CPLD4_PWR_STATUS_1,
    CPLD4_PWR_STATUS_2,
    CPLD4_PWR_STATUS_3,
    DBG_SFP56_16_23_ABS,
    DBG_SFP56_24_31_ABS,
    DBG_SFP56_32_39_ABS,
    DBG_SFP56_16_23_RX_LOS,
    DBG_SFP56_24_31_RX_LOS,
    DBG_SFP56_32_39_RX_LOS,
    DBG_SFP56_16_23_TX_FAULT,
    DBG_SFP56_24_31_TX_FAULT,
    DBG_SFP56_32_39_TX_FAULT,
    IDLE_STATE,
    BSP_DEBUG,
    BSP_WP_ACCESS_COUNT,
};
enum data_type {
    DATA_HEX,
    DATA_DEC,
    DATA_S_DEC,
    DATA_0_1,
    DATA_0_1_INV,
    DATA_UNK,
};

enum reg_write_protect
{
    REG_WP_DIS = false,
    REG_WP_EN = true
};

enum reg_is_event
{
    REG_NOT_EVENT = false,
    REG_IS_EVENT = true
};

enum bsp_log_types {
    LOG_NONE,
    LOG_RW,
    LOG_READ,
    LOG_WRITE
};

enum bsp_log_ctrl {
    LOG_DISABLE,
    LOG_ENABLE
};

typedef struct  {
    u16 reg;
    u8 mask;
    u8 data_type;
    bool write_protect;
    bool is_event;
} attr_reg_map_t;

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

    u8 soft_latch[SOFT_LATCH_SIZE];
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
