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
    cpld4
};

/*
 *  Normally, the CPLD register range is 0x00-0xff.
 *  Therefore, we define the invalid address 0x100 as NONE_REG
 */

#define NONE_REG                                0x00
#define REG_BASE_MB                             0xE00

/* CPLD common information registers */
// & cpld_common
#define CPLD_VERSION_REG                         0x02
#define CPLD_ID_REG                              0x03
#define CPLD_BUILD_REG                           0x04
#define CPLD_CHIP_TYPE_REG                       0x05
#define EVENT_DETECT_CTRL_REG                    0x3F
#define MODULE_RESET_REG                         0xF0

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
#define CPU_NMI_INTR_REG                         0x19
#define OUT_STATUS_INTR_REG                      0x1C

/* Interrupt Mask */
#define CLK_PTP_MASK_INTR_REG                    0x20
#define PHY_MASK_INTR_REG                        0x23
#define TOP_BRD_CPLD_FRU_MASK_INTR_REG           0x24
#define MAIN_BRD_CPLD_MASK_INTR_REG              0x25
#define THERMAL_MASK_INTR_REG                    0x26
#define CPU_NMI_MASK_INTR_REG                    0x29
#define OUT_STATUS_MASK_INTR_REG                 0x2C

/* Interrupt Event */
#define CLK_PTP_EVT_INTR_REG                     0x30
#define PHY_EVT_INTR_REG                         0x33
#define TOP_BRD_CPLD_FRU_EVT_INTR_REG            0x34
#define MAIN_BRD_CPLD_EVT_INTR_REG               0x35
#define THERMAL_EVT_INTR_REG                     0x36
#define CPU_NMI_EVT_INTR_REG                     0x39
#define OUT_STATUS_EVT_INTR_REG                  0x3C

/* Reset */
#define BIOS_FLASH_RESET_CTRL_REG                0x41
#define BMC_PHY_RESET_CTRL_REG                   0x43
#define USB_RESET_CTRL_REG                       0x44
#define TOP_I2C_MUX_RESET_REG                    0x46

/* Misc Status Control */
#define DAUGHTER_BRD_ABS_REG                     0x50
#define PSU_STATUS_REG                           0x51
#define SYSTEM_PWR_STATUS_REG                    0x52
#define USB_SSD_STATUS_REG                       0x54
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

/* BIOS SPI W/R control */
/* Ext. 0xB0~0xBF: BIOS SPI Control Register(Reg. Control) */
#define BIOS_SPI_CTRL_0_REG                      0xB0
#define BIOS_SPI_CTRL_1_REG                      0xB1
#define BIOS_SPI_RD_DATA_0_REG                   0xB2
#define BIOS_SPI_RD_DATA_1_REG                   0xB3
#define BIOS_SPI_RD_DATA_2_REG                   0xB4
#define BIOS_SPI_RD_DATA_3_REG                   0xB5
#define BIOS_SPI_WR_DATA_0_REG                   0xB6
#define BIOS_SPI_WR_DATA_1_REG                   0xB7
#define BIOS_SPI_WR_DATA_2_REG                   0xB8
#define BIOS_SPI_WR_DATA_3_REG                   0xB9
#define BIOS_SPI_ADDR_0_REG                      0xBA
#define BIOS_SPI_ADDR_1_REG                      0xBB
#define BIOS_SPI_ADDR_2_REG                      0xBC
#define BIOS_SPI_ADDR_3_REG                      0xBD
#define BIOS_SPI_EN_REG                          0xBE

/* Ext. 0xE0~0xEF: BIOS SPI Control Register(RAM control) */
#define BIOS_SPI_PP_STATUS_REG                   0xE0
#define BIOS_SPI_WRITE_RAM_0_REG                 0xE1
#define BIOS_SPI_WRITE_RAM_1_REG                 0xE2
#define BIOS_SPI_DEMO_CTRL_0_REG                 0xE3
#define BIOS_SPI_DEMO_CTRL_1_REG                 0xE4
#define BIOS_SPI_DEMO_CTRL_2_REG                 0xE5
#define BIOS_SPI_DEMO_CTRL_3_REG                 0xE6
#define BIOS_SPI_DEMO_CTRL_4_REG                 0xE7
#define BIOS_SPI_DEMO_CTRL_5_REG                 0xE8
#define BIOS_SPI_DEMO_CTRL_6_REG                 0xE9
#define BIOS_SPI_DEMO_CTRL_7_REG                 0xEA
#define BIOS_SPI_DEMO_CTRL_8_REG                 0xEB
#define BIOS_SPI_DEMO_CTRL_9_REG                 0xEC
#define BIOS_SPI_DEMO_CTRL_10_REG                0xED

/* Timing Control */
#define OCXO_GNSS_ID_REG                         0xC0
#define CLK_PTP_RESET_REG                        0xC2
#define CLK_TIMING_CTRL_REG                      0xC3
#define GNSS_STATUS_REG                          0xC4
#define TIMING_STATUS_REG                        0xC5

/* Modified SPEC */
#define CLK_BUFFER_EN_CTRL_REG                   0xC6
#define MAIN_I2C_MUX_RESET_REG                   0xC7

/* Interrupt Debug */
#define DBG_CLK_PTP_INTR_REG                     0xE0
#define DBG_PHY_INTR_REG                         0xE3
#define DBG_TOP_BRD_CPLD_FRU_INTR_REG            0xE4
#define DBG_MAIN_BRD_CPLD_INTR_REG               0xE5
#define DBG_THERMAL_INTR_REG                     0xE6
#define DBG_CPU_NMI_INTR_REG                     0xE9

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

/* Interrupt Mask */
#define QSFP28_0_5_MASK_ABS_REG                  0x20
#define QSFPDD_12_15_MASK_ABS_REG                0x21
#define QSFP28_0_5_MASK_INTR_REG                 0x22
#define QSFPDD_12_15_MASK_INTR_REG               0x23
#define QSFP28_0_5_MASK_EFUSE_PG_REG             0x24
#define QSFPDD_12_15_MASK_EFUSE_PG_REG           0x25

/* Interrupt Event */
#define QSFP28_0_5_EVT_ABS_REG                   0x30
#define QSFPDD_12_15_EVT_ABS_REG                 0x31
#define QSFP28_0_5_EVT_INTR_REG                  0x32
#define QSFPDD_12_15_EVT_INTR_REG                0x33
#define QSFP28_0_5_EVT_EFUSE_PG_REG              0x34
#define QSFPDD_12_15_EVT_EFUSE_PG_REG            0x35

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
#define CPLD2_PWR_STATUS_2_REG                   0x92

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

/* Interrupt Mask */
#define QSFP28_6_11_MASK_ABS_REG                 0x20
#define QSFP28_6_11_MASK_INTR_REG                0x21
#define QSFP28_6_11_MASK_EFUSE_PG_REG            0x22
#define MAC_MASK_INTR_REG                        0x23
#define MAIN_THERMAL_MASK_INTR_REG               0x24
#define FAN_MASK_ABS_REG                         0x25

/* Interrupt Event */
#define QSFP28_6_11_EVT_ABS_REG                  0x30
#define QSFP28_6_11_EVT_INTR_REG                 0x31
#define QSFP28_6_11_EVT_EFUSE_PG_REG             0x32
#define MAC_EVT_INTR_REG                         0x33
#define MAIN_THERMAL_EVT_INTR_REG                0x34
#define FAN_EVT_ABS_REG                          0x35

/* Reset */
#define QSFP28_6_11_RESET_REG                    0x40
#define QSFP28_6_11_LPMODE_REG                   0x41
#define MAC_RESET_REG                            0x42
#define USB_QSPI_RESET_REG                       0x44

/* Timing Status & Misc Control */
#define CLK_TIMING_STATUS_1_REG                  0x50
#define CLK_TIMING_STATUS_2_REG                  0x51
#define ROV_STATUS_REG                           0x52
#define MISC_CONTROL_REG                         0x56
#define IO_OVER_CURRENT_REG                      0x56

/* Control Registers */
// #define LED_CLEAR_REG                            0x60
#define GNSS_CTRL_REG                            0x61
#define I2C_MUX_SELECT_REG                       0x65
#define SYNCE_CTRL_REG                           0x66

/* FAN Control */
#define FAN_SPEED_READ_MODE_REG                  0x72
/* 0x73~0x74: FAN RPM read value */
#define FAN_RPM_READ_VALUE_0_REG                 0x73
#define FAN_RPM_READ_VALUE_1_REG                 0x74

#define FAN_1_6_PWM_CTRL_REG                     0x75
#define FAN_2_7_PWM_CTRL_REG                     0x76
#define FAN_3_8_PWM_CTRL_REG                     0x77
#define FAN_4_9_PWM_CTRL_REG                     0x78
#define FAN_5_10_PWM_CTRL_REG                    0x79

/* FAN LED Control */
#define FAN_1_2_LED_CTRL_REG                     0x7A
#define FAN_3_4_LED_CTRL_REG                     0x7B
#define FAN_5_LED_CTRL_REG                       0x7C

/* Port LED Control */
/* 0x80~0x85: QSFP Port 06~11 LED control */
#define QSFP28_P6_LED_CTRL_REG                   0x80
#define QSFP28_P7_LED_CTRL_REG                   0x81
#define QSFP28_P8_LED_CTRL_REG                   0x82
#define QSFP28_P9_LED_CTRL_REG                   0x83
#define QSFP28_P10_LED_CTRL_REG                  0x84
#define QSFP28_P11_LED_CTRL_REG                  0x85

/* 0x86~0x8B: SFP56 Port 16~39 LED control */
#define SFP56_16_19_LED_CTRL_REG                 0x86
#define SFP56_20_23_LED_CTRL_REG                 0x87
#define SFP56_24_27_LED_CTRL_REG                 0x88
#define SFP56_28_31_LED_CTRL_REG                 0x89
#define SFP56_32_35_LED_CTRL_REG                 0x8A
#define SFP56_36_39_LED_CTRL_REG                 0x8B

/* Status Registers */
#define SYSTEM_LED_STATUS_1_REG                  0x90
#define CPLD3_PWR_STATUS_REG                     0x91
#define FAN_PWR_STATUS_REG                       0x92

/* Interrupt Debug */
#define DBG_QSFP28_6_11_ABS_REG                  0xE0
#define DBG_QSFP28_6_11_INTR_REG                 0xE1
#define DBG_QSFP28_6_11_EFUSE_PG_REG             0xE2


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

/* Interrupt Mask */
#define SFP56_16_23_MASK_ABS_REG                 0x20
#define SFP56_24_31_MASK_ABS_REG                 0x21
#define SFP56_32_39_MASK_ABS_REG                 0x22
#define SFP56_16_23_MASK_RX_LOS_REG              0x23
#define SFP56_24_31_MASK_RX_LOS_REG              0x24
#define SFP56_32_39_MASK_RX_LOS_REG              0x25
#define SFP56_16_23_MASK_TX_FAULT_REG            0x26
#define SFP56_24_31_MASK_TX_FAULT_REG            0x27
#define SFP56_32_39_MASK_TX_FAULT_REG            0x28

/* Interrupt Event */
#define SFP56_16_23_EVT_ABS_REG                  0x30
#define SFP56_24_31_EVT_ABS_REG                  0x31
#define SFP56_32_39_EVT_ABS_REG                  0x32
#define SFP56_16_23_EVT_RX_LOS_REG               0x33
#define SFP56_24_31_EVT_RX_LOS_REG               0x34
#define SFP56_32_39_EVT_RX_LOS_REG               0x35
#define SFP56_16_23_EVT_TX_FAULT_REG             0x36
#define SFP56_24_31_EVT_TX_FAULT_REG             0x37
#define SFP56_32_39_EVT_TX_FAULT_REG             0x38

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
#define CPLD_MAX_NCHANS                         16
#define CPLD_MUX_TIMEOUT                        1400
#define CPLD_MUX_RETRY_WAIT                     200
#define CPLD_MUX_CHN_OFF                        (0x0)
//#define FPGA_MUX_CHN_OFF                        (0x0)
#define CPLD_I2C_ENABLE_BRIDGE                  MASK_1000_0000
#define CPLD_I2C_ENABLE_CHN_SEL                 MASK_1000_0000
//#define LAN_PORT_RELAY_ENABLE                   MASK_1000_0000

/* common manipulation */
#define INVALID(i, min, max)    ((i < min) || (i > max) ? 1u : 0u)

/* CPLD sysfs attributes index  */
enum cpld_sysfs_attributes {
    // CPLD Common
    // @ CPLD_VERSION,
    CPLD_MINOR_VER,
    CPLD_MAJOR_VER,

    CPLD_ID,

    // @ CPLD_BUILD,
    CPLD_BUILD_VER,

    CPLD_VERSION_H,
    EVENT_DETECT_CTRL,
    CPLD_CHIP_TYPE,
    MODULE_RESET,
    CPLD_WRITE_PROTECT_1,
    CPLD_WRITE_PROTECT_2,


    /******************************************************************************
    * CPLD 1                                                                     *
    ******************************************************************************/

    // Board information
    BRD_SKU_ID,

    // @ BRD_HW_BUILD_REV,
    BRD_HW_ID,
    BRD_DEPH_ID,
    BRD_BUILD_ID,
    BRD_ID_TYPE,

    CPLD_BOARD_EXT_ID,

    // CPLD information
    GDDR6_ID,
    GDDR6_ID_FUNC,
    // Interrupt Status
    PHY_INTR,
    CLK_PTP_INTR,

    // @ TOP_BRD_CPLD_FRU_INTR,
    TOP_BRD_CPLD_FRU_INTR,
    PSU0_INTR,
    PSU1_INTR,
    CPLD2_INTR,
    CPLD2_IO_INTR,
    // @ MAIN_BRD_CPLD_INTR
    MAIN_BRD_CPLD_INTR,
    CPLD3_INTR,
    CPLD4_INTR,
    MB_ETH_INTR,
    MB_PTP_INTR,
    FAN_INTR,

    THERMAL_INTR,
    CPU_NMI_INTR,
    OUT_STATUS_INTR,

    // Interrupt Mask
    CLK_PTP_MASK_INTR,
    PHY_MASK_INTR,

    // @ TOP_BRD_CPLD_FRU_MASK_INTR
    TOP_BRD_CPLD_FRU_MASK_INTR,
    PSU0_MASK_INTR,
    PSU1_MASK_INTR,
    CPLD2_MASK_INTR,
    CPLD2_IO_MASK_INTR,
    // @ MAIN_BRD_CPLD_MASK_INTR
    MAIN_BRD_CPLD_MASK_INTR,
    CPLD3_MASK_INTR,
    CPLD4_MASK_INTR,
    MB_ETH_MASK_INTR,
    MB_PTP_MASK_INTR,
    FAN_MASK_INTR,

    THERMAL_MASK_INTR,
    CPU_NMI_MASK_INTR,
    OUT_STATUS_MASK_INTR,

    // Interrupt Event
    CLK_PTP_EVT_INTR,
    PHY_EVT_INTR,
    TOP_BRD_CPLD_FRU_EVT_INTR,
    MAIN_BRD_CPLD_EVT_INTR,
    THERMAL_EVT_INTR,
    CPU_NMI_EVT_INTR,
    OUT_STATUS_EVT_INTR,


    // @ BIOS_FLASH_RESET_CTRL,
    BTN_FP_RESET,
    SPI_BIOS_RESET,

    // @ BMC_PHY_RESET_CTRL,
    RGB_0_RESET,
    RGB_1_RESET,
    BMC_LPC_RESET,
    BMC_PCIE_RESET,
    CPLD_TO_BMC_SYS_RESET,
    CPLD_TO_CPU_RESET,


    // @ USB_RESET_CTRL,
    USB_PWR_EN,
    USB_SIE_RESET,

    // I2C_MUX_RESET,
    I2C_MUX_SYS_RESET,
    I2C_MUX_SMBUS_RESET,
    I2C_MUX_QSFP28_RESET,
    I2C_MUX_SFP_RESET,
    MB_I2C_RESET,

    // Misc Status Control
    // @ DAUGHTER_BRD_ABS,
    BMC_ABS,
    SATA_SSD1_ABS,
    SATA_SSD2_ABS,

    // @ PSU_STATUS,
    PSU0_ABS,
    PSU1_ABS,
    PSU0_VIN_PG,
    PSU1_VIN_PG,
    PSU0_VOUT_PG,
    PSU1_VOUT_PG,
    PSU_STATUS,

    // @ SYSTEM_PWR_STATUS,
    CPU_BOOT_DONE,
    CPU_PG,

    // // @ USB_SSD_STATUS,
    // SSD2_PWR_OC,
    // SSD1_PWR_OC,
    // USB_PWR_OC,

    // // @ CPU_STATUS,
    // CPU_SUS_LOW_POWER,
    // CPU_SUS_S5,

    // // @ PHY_BOOT_CTRL,
    // RGB_0_SERBOOT,
    // RGB_1_SERBOOT,
    // Gearbox_1_SERBOOT,

    // WD_STATUS,
    WDT_CPU_TO_CPLD,
    BMC_WDT_1_RESET,
    BMC_WDT_2_RESET,

    // // @ TIMING_CTRL_STATUS,
    // 81384_CLK_SEL,
    // 85361_SYNCE_SEL0,
    // 85361_SYNCE_SEL1,

    // @ MUX_CTRL,
    SMBUS_PECI_DIS,
    I2C_PSU0_MUX_SEL,
    I2C_PSU1_MUX_SEL,
    I2C_MANF_MUX_SEL,
    I2C_CPLD_MUX_SEL,
    BMC_USB_MUX_SEL,
    UART_CPU_BMC_MUX_SEL,
    UART_MUX_SEL,


    // // PWR_SYSTEM_CTRL,
    // PWREN_SYS_P12V_CPU,
    // PWREN_SYS_DC_CPU,
    // PWREN_CPLD_TO_CPU,
    // PWRBTN_CPLD_TO_CPU,


    // // SERBOOT Control
    // SERBOOT_UFM_STORE_1,
    // SERBOOT_UFM_WRITE_1,

    EXT_CTRL,

    // LED Control
    // @ SYSTEM_LED_CTRL_1,
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

    // @ SYSTEM_LED_CTRL_2,
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

    // @ SYSTEM_LED_CTRL_3,
    CPLD_SYSTEM_LED_SYNC,
    SYNC_LED_STATUS,
    SYNC_LED_SPEED,
    SYNC_LED_BLINK,
    SYNC_LED_ONOFF,

    LED_CLEAR,

    // // Power Status
    // CPLD1_PWR_STATUS,

    // Internal Control
    // @ QSFP28_0_5_PWR_EN,
    QSFP28_P0_PWR_EN,
    QSFP28_P1_PWR_EN,
    QSFP28_P2_PWR_EN,
    QSFP28_P3_PWR_EN,
    QSFP28_P4_PWR_EN,
    QSFP28_P5_PWR_EN,

    // @ QSFP28_6_11_PWR_EN,
    QSFP28_P6_PWR_EN,
    QSFP28_P7_PWR_EN,
    QSFP28_P8_PWR_EN,
    QSFP28_P9_PWR_EN,
    QSFP28_P10_PWR_EN,
    QSFP28_P11_PWR_EN,

    // @ QSFPDD_12_15_PWR_EN,
    QSFPDD_P12_PWR_EN,
    QSFPDD_P13_PWR_EN,
    QSFPDD_P14_PWR_EN,
    QSFPDD_P15_PWR_EN,

    // @ SFP56_16_23_PWR_EN_0,
    SFP56_P16_PWR_EN,
    SFP56_P17_PWR_EN,
    SFP56_P18_PWR_EN,
    SFP56_P19_PWR_EN,
    SFP56_P20_PWR_EN,
    SFP56_P21_PWR_EN,
    SFP56_P22_PWR_EN,
    SFP56_P23_PWR_EN,

    // @ SFP56_24_31_PWR_EN_1,
    SFP56_P24_PWR_EN,
    SFP56_P25_PWR_EN,
    SFP56_P26_PWR_EN,
    SFP56_P27_PWR_EN,
    SFP56_P28_PWR_EN,
    SFP56_P29_PWR_EN,
    SFP56_P30_PWR_EN,
    SFP56_P31_PWR_EN,

    // @ SFP56_32_39_PWR_EN_2,
    SFP56_P32_PWR_EN,
    SFP56_P33_PWR_EN,
    SFP56_P34_PWR_EN,
    SFP56_P35_PWR_EN,
    SFP56_P36_PWR_EN,
    SFP56_P37_PWR_EN,
    SFP56_P38_PWR_EN,
    SFP56_P39_PWR_EN,

    // // BIOS SPI W/R control
    // // Ext. 0xB0~0xBF: BIOS SPI Control Register(Reg. Control)
    // BIOS_SPI_CTRL_0,
    // BIOS_SPI_CTRL_1,
    // BIOS_SPI_RD_DATA_0,
    // BIOS_SPI_RD_DATA_1,
    // BIOS_SPI_RD_DATA_2,
    // BIOS_SPI_RD_DATA_3,
    // BIOS_SPI_WR_DATA_0,
    // BIOS_SPI_WR_DATA_1,
    // BIOS_SPI_WR_DATA_2,
    // BIOS_SPI_WR_DATA_3,
    // BIOS_SPI_ADDR_0,
    // BIOS_SPI_ADDR_1,
    // BIOS_SPI_ADDR_2,
    // BIOS_SPI_ADDR_3,
    // BIOS_SPI_EN,

    // // Ext. 0xE0~0xEF: BIOS SPI Control Register(RAM control)
    // BIOS_SPI_PP_STATUS,
    // BIOS_SPI_WRITE_RAM_0,
    // BIOS_SPI_WRITE_RAM_1,
    // BIOS_SPI_DEMO_CTRL_0,
    // BIOS_SPI_DEMO_CTRL_1,
    // BIOS_SPI_DEMO_CTRL_2,
    // BIOS_SPI_DEMO_CTRL_3,
    // BIOS_SPI_DEMO_CTRL_4,
    // BIOS_SPI_DEMO_CTRL_5,
    // BIOS_SPI_DEMO_CTRL_6,
    // BIOS_SPI_DEMO_CTRL_7,
    // BIOS_SPI_DEMO_CTRL_8,
    // BIOS_SPI_DEMO_CTRL_9,
    // BIOS_SPI_DEMO_CTRL_10,

    // Timing Control
    // // @ OCXO_GNSS_ID,
    // OCXO_ID,
    // GNSS_ID,

    CLK_PTP_RESET,
    CJAP_RESET,
    NTM_RESET,
    GNSS_RESET,
    BITS_RESET,
    CLK_TIMING_CTRL,
    GNSS_STATUS,
    TIMING_STATUS,


    // @ CLK_BUFFER_EN_CTRL,
    QSFPDD_SEL,
    QSFP28_SEL,

    // I2C_MUX_RESET,
    // @ TOP_I2C_MUX_RESET,
    I2C_MUX_0X76_RESET,
    I2C_MUX_0X75_RESET,
    I2C_MUX_QSFP28_6_11_RESET,
    I2C_MUX_SFP56_16_23_RESET,
    I2C_MUX_SFP56_24_31_RESET,
    I2C_MUX_SFP56_32_39_RESET,
    I2C_MUX_0X71_RESET,

    // // Interrupt Debug
    // DBG_CLK_PTP_INTR,
    // DBG_PHY_INTR,
    // DBG_TOP_BRD_CPLD_FRU_INTR,
    // DBG_MAIN_BRD_CPLD_INTR,
    // DBG_THERMAL_INTR,
    // DBG_CPU_NMI_INTR,

    /******************************************************************************
    * CPLD 2                                                                     *
    ******************************************************************************/

    // Ports Interrupt Status
    // @ QSFP28_0_5_ABS,
    QSFP28_P0_ABS,
    QSFP28_P1_ABS,
    QSFP28_P2_ABS,
    QSFP28_P3_ABS,
    QSFP28_P4_ABS,
    QSFP28_P5_ABS,
    // @ QSFPDD_12_15_ABS,
    QSFPDD_P12_ABS,
    QSFPDD_P13_ABS,
    QSFPDD_P14_ABS,
    QSFPDD_P15_ABS,

    // @ QSFP28_0_5_INTR,
    QSFP28_P0_INTR,
    QSFP28_P1_INTR,
    QSFP28_P2_INTR,
    QSFP28_P3_INTR,
    QSFP28_P4_INTR,
    QSFP28_P5_INTR,
    // @ QSFPDD_12_15_INTR,
    QSFPDD_P12_INTR,
    QSFPDD_P13_INTR,
    QSFPDD_P14_INTR,
    QSFPDD_P15_INTR,


    // @ QSFP28_0_5_EFUSE_PG,
    QSFP28_P0_EFUSE_PG,
    QSFP28_P1_EFUSE_PG,
    QSFP28_P2_EFUSE_PG,
    QSFP28_P3_EFUSE_PG,
    QSFP28_P4_EFUSE_PG,
    QSFP28_P5_EFUSE_PG,
    // @ QSFPDD_12_15_EFUSE_PG,
    QSFPDD_P12_EFUSE_PG,
    QSFPDD_P13_EFUSE_PG,
    QSFPDD_P14_EFUSE_PG,
    QSFPDD_P15_EFUSE_PG,

    // Interrupt Mask
    // @ QSFP28_0_5_MASK_ABS,
    QSFP28_P0_MASK_ABS,
    QSFP28_P1_MASK_ABS,
    QSFP28_P2_MASK_ABS,
    QSFP28_P3_MASK_ABS,
    QSFP28_P4_MASK_ABS,
    QSFP28_P5_MASK_ABS,
    // @ QSFPDD_12_15_MASK_ABS,
    QSFPDD_P12_MASK_ABS,
    QSFPDD_P13_MASK_ABS,
    QSFPDD_P14_MASK_ABS,
    QSFPDD_P15_MASK_ABS,

    // @ QSFP28_0_5_MASK_INTR,
    QSFP28_P0_MASK_INTR,
    QSFP28_P1_MASK_INTR,
    QSFP28_P2_MASK_INTR,
    QSFP28_P3_MASK_INTR,
    QSFP28_P4_MASK_INTR,
    QSFP28_P5_MASK_INTR,
    // @ QSFPDD_12_15_MASK_INTR,
    QSFPDD_P12_MASK_INTR,
    QSFPDD_P13_MASK_INTR,
    QSFPDD_P14_MASK_INTR,
    QSFPDD_P15_MASK_INTR,

    // @ QSFP28_0_5_MASK_EFUSE_PG,
    QSFP28_P0_MASK_EFUSE_PG,
    QSFP28_P1_MASK_EFUSE_PG,
    QSFP28_P2_MASK_EFUSE_PG,
    QSFP28_P3_MASK_EFUSE_PG,
    QSFP28_P4_MASK_EFUSE_PG,
    QSFP28_P5_MASK_EFUSE_PG,
    // @ QSFPDD_12_15_MASK_EFUSE_PG,
    QSFPDD_P12_MASK_EFUSE_PG,
    QSFPDD_P13_MASK_EFUSE_PG,
    QSFPDD_P14_MASK_EFUSE_PG,
    QSFPDD_P15_MASK_EFUSE_PG,

    // Interrupt Event
    QSFP28_0_5_EVT_ABS,
    QSFPDD_12_15_EVT_ABS,

    QSFP28_0_5_EVT_INTR,
    QSFPDD_12_15_EVT_INTR,

    QSFP28_0_5_EVT_EFUSE_PG,
    QSFPDD_12_15_EVT_EFUSE_PG,

    // Reset
    // @ QSFP28_0_5_RESET,
    QSFP28_P0_RESET,
    QSFP28_P1_RESET,
    QSFP28_P2_RESET,
    QSFP28_P3_RESET,
    QSFP28_P4_RESET,
    QSFP28_P5_RESET,
    // @ QSFPDD_12_15_RESET,
    QSFPDD_P12_RESET,
    QSFPDD_P13_RESET,
    QSFPDD_P14_RESET,
    QSFPDD_P15_RESET,

    // @ QSFP28_0_5_LPMODE,
    QSFP28_P0_LPMODE,
    QSFP28_P1_LPMODE,
    QSFP28_P2_LPMODE,
    QSFP28_P3_LPMODE,
    QSFP28_P4_LPMODE,
    QSFP28_P5_LPMODE,
    // @ QSFPDD_12_15_LPMODE,
    QSFPDD_P12_LPMODE,
    QSFPDD_P13_LPMODE,
    QSFPDD_P14_LPMODE,
    QSFPDD_P15_LPMODE,

    CLK_EN_CTRL,

    // // PSU Control
    // PSU_CTRL,

    // Ports LED control
    // 0x80~0x85: QSFP28 ports LED control
    QSFP28_P0_LED_CTRL,
    QSFP28_P1_LED_CTRL,
    QSFP28_P2_LED_CTRL,
    QSFP28_P3_LED_CTRL,
    QSFP28_P4_LED_CTRL,
    QSFP28_P5_LED_CTRL,
    // 0x86~0x87: QSFP-DD ports LED control
    // @ QSFPDD_P12_P13_LED_CTRL,
    QSFPDD_P12_LED_CTRL,
    QSFPDD_P13_LED_CTRL,
    // @ QSFPDD_P14_P15_LED_CTRL,
    QSFPDD_P14_LED_CTRL,
    QSFPDD_P15_LED_CTRL,

    // // Power Status
    // CPLD2_PWR_STATUS_0,
    // CPLD2_PWR_STATUS_1,
    // CPLD2_PWR_STATUS_2,

    // // Interrupt Debug
    // DBG_QSFP28_0_5_ABS,
    // DBG_QSFPDD_12_15_ABS,
    // DBG_QSFP28_0_5_INTR,
    // DBG_QSFPDD_12_15_INTR,
    // DBG_QSFP28_0_5_EFUSE_PG,
    // DBG_QSFPDD_12_15_EFUSE_PG,


    /******************************************************************************
    * CPLD 3                                                                     *
    ******************************************************************************/

    // CPLD information
    GNSS_MODEL_ID,
    OCXO_ID,

    // Interrupt Status
    // @ QSFP28_6_11_ABS,
    QSFP28_P6_ABS,
    QSFP28_P7_ABS,
    QSFP28_P8_ABS,
    QSFP28_P9_ABS,
    QSFP28_P10_ABS,
    QSFP28_P11_ABS,

    // @ QSFP28_6_11_INTR,
    QSFP28_P6_INTR,
    QSFP28_P7_INTR,
    QSFP28_P8_INTR,
    QSFP28_P9_INTR,
    QSFP28_P10_INTR,
    QSFP28_P11_INTR,

    // @ QSFP28_6_11_EFUSE_PG,
    QSFP28_P6_EFUSE_PG,
    QSFP28_P7_EFUSE_PG,
    QSFP28_P8_EFUSE_PG,
    QSFP28_P9_EFUSE_PG,
    QSFP28_P10_EFUSE_PG,
    QSFP28_P11_EFUSE_PG,

    MAC_INTR,

    // @ FAN_ABS,
    FAN_0_ABS,
    FAN_1_ABS,
    FAN_2_ABS,
    FAN_3_ABS,
    FAN_4_ABS,

    // Interrupt Mask
    // @ QSFP28_6_11_MASK_ABS,
    QSFP28_P6_MASK_ABS,
    QSFP28_P7_MASK_ABS,
    QSFP28_P8_MASK_ABS,
    QSFP28_P9_MASK_ABS,
    QSFP28_P10_MASK_ABS,
    QSFP28_P11_MASK_ABS,

    // @ QSFP28_6_11_MASK_INTR,
    QSFP28_P6_MASK_INTR,
    QSFP28_P7_MASK_INTR,
    QSFP28_P8_MASK_INTR,
    QSFP28_P9_MASK_INTR,
    QSFP28_P10_MASK_INTR,
    QSFP28_P11_MASK_INTR,

    // @ QSFP28_6_11_MASK_EFUSE_PG,
    QSFP28_P6_MASK_EFUSE_PG,
    QSFP28_P7_MASK_EFUSE_PG,
    QSFP28_P8_MASK_EFUSE_PG,
    QSFP28_P9_MASK_EFUSE_PG,
    QSFP28_P10_MASK_EFUSE_PG,
    QSFP28_P11_MASK_EFUSE_PG,

    MAC_MASK_INTR,
    // @ FAN_ABS,
    FAN_0_MASK_ABS,
    FAN_1_MASK_ABS,
    FAN_2_MASK_ABS,
    FAN_3_MASK_ABS,
    FAN_4_MASK_ABS,

    // Interrupt Event
    QSFP28_6_11_EVT_ABS,
    QSFP28_6_11_EVT_INTR,
    QSFP28_6_11_EVT_EFUSE_PG,
    MAC_EVT_INTR,
    FAN_EVT_ABS,

    // Reset
    // @ QSFP28_6_11_RESET,
    QSFP28_P6_RESET,
    QSFP28_P7_RESET,
    QSFP28_P8_RESET,
    QSFP28_P9_RESET,
    QSFP28_P10_RESET,
    QSFP28_P11_RESET,

    // @ QSFP28_6_11_LPMODE,
    QSFP28_P6_LPMODE,
    QSFP28_P7_LPMODE,
    QSFP28_P8_LPMODE,
    QSFP28_P9_LPMODE,
    QSFP28_P10_LPMODE,
    QSFP28_P11_LPMODE,

    // Reset
    MAC_RESET,

    USB_QSPI_RESET,

    // // Timing Status & Misc Control
    // CLK_TIMING_STATUS_1,
    // CLK_TIMING_STATUS_2,
    MAC_ROV,
    I2C_ROV_MUX_SEL,

    // IO_OVER_CURRENT,

    // Control Registers
    // LED_CLEAR,
    // GNSS_CTRL,

    // @ I2C_MUX_SELECT,
    I2C_CPLD_MUX_EN,
    I2C0_PSU_MUX_SEL,
    I2C0_HWM_MUX_SEL,
    I2C_IO_MUX_SEL,


    // SYNCE_CTRL,

    // FAN Control
    FAN_SPEED_READ_MODE,
    // 0x73~0x74: FAN RPM read value
    FAN_RPM_LOW_BYTE,
    FAN_RPM_HIGH_BYTE,

    // FAN_1_6_PWM_CTRL,
    // FAN_2_7_PWM_CTRL,
    // FAN_3_8_PWM_CTRL,
    // FAN_4_9_PWM_CTRL,
    // FAN_5_10_PWM_CTRL,
    // FAN_1_LED_CTRL,
    // FAN_2_LED_CTRL,
    // FAN_3_LED_CTRL,
    // FAN_4_LED_CTRL,
    // FAN_5_LED_CTRL,

    // // Port LED Control
    // // 0x80~0x85: QSFP Port 06~11 LED control
    // QSFP28_P6_LED_CTRL,
    // QSFP28_P7_LED_CTRL,
    // QSFP28_P8_LED_CTRL,
    // QSFP28_P9_LED_CTRL,
    // QSFP28_P10_LED_CTRL,
    // QSFP28_P11_LED_CTRL,

    // // 0x86~0x8B: SFP56 Port 16~39 LED control
    // SFP56_P16_LED_CTRL,
    // SFP56_P17_LED_CTRL,
    // SFP56_P18_LED_CTRL,
    // SFP56_P19_LED_CTRL,
    // // @ SFP56_20_23_LED_CTRL
    // SFP56_P20_LED_CTRL,
    // SFP56_P21_LED_CTRL,
    // SFP56_P22_LED_CTRL,
    // SFP56_P23_LED_CTRL,
    // // @ SFP56_24_27_LED_CTRL
    // SFP56_P24_LED_CTRL,
    // SFP56_P25_LED_CTRL,
    // SFP56_P26_LED_CTRL,
    // SFP56_P27_LED_CTRL,
    // // @ SFP56_28_31_LED_CTRL
    // SFP56_P28_LED_CTRL,
    // SFP56_P29_LED_CTRL,
    // SFP56_P30_LED_CTRL,
    // SFP56_P31_LED_CTRL,
    // // @ SFP56_32_35_LED_CTRL
    // SFP56_P32_LED_CTRL,
    // SFP56_P33_LED_CTRL,
    // SFP56_P34_LED_CTRL,
    // SFP56_P35_LED_CTRL,
    // // @ SFP56_36_39_LED_CTRL
    // SFP56_P36_LED_CTRL,
    // SFP56_P37_LED_CTRL,
    // SFP56_P38_LED_CTRL,
    // SFP56_P39_LED_CTRL,

    // Status Registers
    SYSTEM_LED_STATUS_1,
    // CPLD3_PWR_STATUS,
    // FAN_PWR_STATUS,

    // // Interrupt Debug
    // DBG_QSFP28_6_11_ABS,
    // DBG_QSFP28_6_11_INTR,
    // DBG_QSFP28_6_11_EFUSE_PG,



    /******************************************************************************
    * CPLD 4                                                                     *
    ******************************************************************************/

    // Interrupt Status
    // @ SFP56_16_23_ABS,
    SFP56_P16_ABS,
    SFP56_P17_ABS,
    SFP56_P18_ABS,
    SFP56_P19_ABS,
    SFP56_P20_ABS,
    SFP56_P21_ABS,
    SFP56_P22_ABS,
    SFP56_P23_ABS,
    // @ SFP56_24_31_ABS,
    SFP56_P24_ABS,
    SFP56_P25_ABS,
    SFP56_P26_ABS,
    SFP56_P27_ABS,
    SFP56_P28_ABS,
    SFP56_P29_ABS,
    SFP56_P30_ABS,
    SFP56_P31_ABS,
    // @ SFP56_32_39_ABS,
    SFP56_P32_ABS,
    SFP56_P33_ABS,
    SFP56_P34_ABS,
    SFP56_P35_ABS,
    SFP56_P36_ABS,
    SFP56_P37_ABS,
    SFP56_P38_ABS,
    SFP56_P39_ABS,

    // @ SFP56_16_23_RX_LOS,
    SFP56_P16_RX_LOS,
    SFP56_P17_RX_LOS,
    SFP56_P18_RX_LOS,
    SFP56_P19_RX_LOS,
    SFP56_P20_RX_LOS,
    SFP56_P21_RX_LOS,
    SFP56_P22_RX_LOS,
    SFP56_P23_RX_LOS,
    // @ SFP56_24_31_RX_LOS,
    SFP56_P24_RX_LOS,
    SFP56_P25_RX_LOS,
    SFP56_P26_RX_LOS,
    SFP56_P27_RX_LOS,
    SFP56_P28_RX_LOS,
    SFP56_P29_RX_LOS,
    SFP56_P30_RX_LOS,
    SFP56_P31_RX_LOS,
    // @ SFP56_32_39_RX_LOS,
    SFP56_P32_RX_LOS,
    SFP56_P33_RX_LOS,
    SFP56_P34_RX_LOS,
    SFP56_P35_RX_LOS,
    SFP56_P36_RX_LOS,
    SFP56_P37_RX_LOS,
    SFP56_P38_RX_LOS,
    SFP56_P39_RX_LOS,

    // @ SFP56_16_23_TX_FAULT,
    SFP56_P16_TX_FAULT,
    SFP56_P17_TX_FAULT,
    SFP56_P18_TX_FAULT,
    SFP56_P19_TX_FAULT,
    SFP56_P20_TX_FAULT,
    SFP56_P21_TX_FAULT,
    SFP56_P22_TX_FAULT,
    SFP56_P23_TX_FAULT,
    // @ SFP56_24_31_TX_FAULT,
    SFP56_P24_TX_FAULT,
    SFP56_P25_TX_FAULT,
    SFP56_P26_TX_FAULT,
    SFP56_P27_TX_FAULT,
    SFP56_P28_TX_FAULT,
    SFP56_P29_TX_FAULT,
    SFP56_P30_TX_FAULT,
    SFP56_P31_TX_FAULT,
    // @ SFP56_32_39_TX_FAULT,
    SFP56_P32_TX_FAULT,
    SFP56_P33_TX_FAULT,
    SFP56_P34_TX_FAULT,
    SFP56_P35_TX_FAULT,
    SFP56_P36_TX_FAULT,
    SFP56_P37_TX_FAULT,
    SFP56_P38_TX_FAULT,
    SFP56_P39_TX_FAULT,

    // Interrupt Mask
    // @ SFP56_16_23_MASK_ABS,
    SFP56_P16_MASK_ABS,
    SFP56_P17_MASK_ABS,
    SFP56_P18_MASK_ABS,
    SFP56_P19_MASK_ABS,
    SFP56_P20_MASK_ABS,
    SFP56_P21_MASK_ABS,
    SFP56_P22_MASK_ABS,
    SFP56_P23_MASK_ABS,
    // @ SFP56_24_31_MASK_ABS,
    SFP56_P24_MASK_ABS,
    SFP56_P25_MASK_ABS,
    SFP56_P26_MASK_ABS,
    SFP56_P27_MASK_ABS,
    SFP56_P28_MASK_ABS,
    SFP56_P29_MASK_ABS,
    SFP56_P30_MASK_ABS,
    SFP56_P31_MASK_ABS,
    // @ SFP56_32_39_MASK_ABS,
    SFP56_P32_MASK_ABS,
    SFP56_P33_MASK_ABS,
    SFP56_P34_MASK_ABS,
    SFP56_P35_MASK_ABS,
    SFP56_P36_MASK_ABS,
    SFP56_P37_MASK_ABS,
    SFP56_P38_MASK_ABS,
    SFP56_P39_MASK_ABS,

    // @ SFP56_16_23_MASK_RX_LOS,
    SFP56_P16_MASK_RX_LOS,
    SFP56_P17_MASK_RX_LOS,
    SFP56_P18_MASK_RX_LOS,
    SFP56_P19_MASK_RX_LOS,
    SFP56_P20_MASK_RX_LOS,
    SFP56_P21_MASK_RX_LOS,
    SFP56_P22_MASK_RX_LOS,
    SFP56_P23_MASK_RX_LOS,
    // @ SFP56_24_31_MASK_RX_LOS,
    SFP56_P24_MASK_RX_LOS,
    SFP56_P25_MASK_RX_LOS,
    SFP56_P26_MASK_RX_LOS,
    SFP56_P27_MASK_RX_LOS,
    SFP56_P28_MASK_RX_LOS,
    SFP56_P29_MASK_RX_LOS,
    SFP56_P30_MASK_RX_LOS,
    SFP56_P31_MASK_RX_LOS,
    // @ SFP56_32_39_MASK_RX_LOS,
    SFP56_P32_MASK_RX_LOS,
    SFP56_P33_MASK_RX_LOS,
    SFP56_P34_MASK_RX_LOS,
    SFP56_P35_MASK_RX_LOS,
    SFP56_P36_MASK_RX_LOS,
    SFP56_P37_MASK_RX_LOS,
    SFP56_P38_MASK_RX_LOS,
    SFP56_P39_MASK_RX_LOS,

    // @ SFP56_16_23_MASK_TX_FAULT,
    SFP56_P16_MASK_TX_FAULT,
    SFP56_P17_MASK_TX_FAULT,
    SFP56_P18_MASK_TX_FAULT,
    SFP56_P19_MASK_TX_FAULT,
    SFP56_P20_MASK_TX_FAULT,
    SFP56_P21_MASK_TX_FAULT,
    SFP56_P22_MASK_TX_FAULT,
    SFP56_P23_MASK_TX_FAULT,
    // @ SFP56_24_31_MASK_TX_FAULT,
    SFP56_P24_MASK_TX_FAULT,
    SFP56_P25_MASK_TX_FAULT,
    SFP56_P26_MASK_TX_FAULT,
    SFP56_P27_MASK_TX_FAULT,
    SFP56_P28_MASK_TX_FAULT,
    SFP56_P29_MASK_TX_FAULT,
    SFP56_P30_MASK_TX_FAULT,
    SFP56_P31_MASK_TX_FAULT,
    // @ SFP56_32_39_MASK_TX_FAULT,
    SFP56_P32_MASK_TX_FAULT,
    SFP56_P33_MASK_TX_FAULT,
    SFP56_P34_MASK_TX_FAULT,
    SFP56_P35_MASK_TX_FAULT,
    SFP56_P36_MASK_TX_FAULT,
    SFP56_P37_MASK_TX_FAULT,
    SFP56_P38_MASK_TX_FAULT,
    SFP56_P39_MASK_TX_FAULT,

    // Interrupt Event
    SFP56_16_23_EVT_ABS,
    SFP56_24_31_EVT_ABS,
    SFP56_32_39_EVT_ABS,
    SFP56_16_23_EVT_RX_LOS,
    SFP56_24_31_EVT_RX_LOS,
    SFP56_32_39_EVT_RX_LOS,
    SFP56_16_23_EVT_TX_FAULT,
    SFP56_24_31_EVT_TX_FAULT,
    SFP56_32_39_EVT_TX_FAULT,

    // SFP Control
    // @ SFP56_16_23_TX_DISABLE,
    SFP56_P16_TX_DISABLE,
    SFP56_P17_TX_DISABLE,
    SFP56_P18_TX_DISABLE,
    SFP56_P19_TX_DISABLE,
    SFP56_P20_TX_DISABLE,
    SFP56_P21_TX_DISABLE,
    SFP56_P22_TX_DISABLE,
    SFP56_P23_TX_DISABLE,
    // @ SFP56_24_31_TX_DISABLE,
    SFP56_P24_TX_DISABLE,
    SFP56_P25_TX_DISABLE,
    SFP56_P26_TX_DISABLE,
    SFP56_P27_TX_DISABLE,
    SFP56_P28_TX_DISABLE,
    SFP56_P29_TX_DISABLE,
    SFP56_P30_TX_DISABLE,
    SFP56_P31_TX_DISABLE,
    // @ SFP56_32_39_TX_DISABLE,
    SFP56_P32_TX_DISABLE,
    SFP56_P33_TX_DISABLE,
    SFP56_P34_TX_DISABLE,
    SFP56_P35_TX_DISABLE,
    SFP56_P36_TX_DISABLE,
    SFP56_P37_TX_DISABLE,
    SFP56_P38_TX_DISABLE,
    SFP56_P39_TX_DISABLE,

    // @ SFP56_16_23_RATE_SEL,
    SFP56_P16_RATE_SEL,
    SFP56_P17_RATE_SEL,
    SFP56_P18_RATE_SEL,
    SFP56_P19_RATE_SEL,
    SFP56_P20_RATE_SEL,
    SFP56_P21_RATE_SEL,
    SFP56_P22_RATE_SEL,
    SFP56_P23_RATE_SEL,
    // @ SFP56_24_31_RATE_SEL,
    SFP56_P24_RATE_SEL,
    SFP56_P25_RATE_SEL,
    SFP56_P26_RATE_SEL,
    SFP56_P27_RATE_SEL,
    SFP56_P28_RATE_SEL,
    SFP56_P29_RATE_SEL,
    SFP56_P30_RATE_SEL,
    SFP56_P31_RATE_SEL,
    // @ SFP56_32_39_RATE_SEL,
    SFP56_P32_RATE_SEL,
    SFP56_P33_RATE_SEL,
    SFP56_P34_RATE_SEL,
    SFP56_P35_RATE_SEL,
    SFP56_P36_RATE_SEL,
    SFP56_P37_RATE_SEL,
    SFP56_P38_RATE_SEL,
    SFP56_P39_RATE_SEL,

    // // Power Status
    // CPLD4_PWR_STATUS_0,
    // CPLD4_PWR_STATUS_1,
    // CPLD4_PWR_STATUS_2,
    // CPLD4_PWR_STATUS_3,

    // // Interrupt Debug
    // DBG_SFP56_16_23_ABS,
    // DBG_SFP56_24_31_ABS,
    // DBG_SFP56_32_39_ABS,
    // DBG_SFP56_16_23_RX_LOS,
    // DBG_SFP56_24_31_RX_LOS,
    // DBG_SFP56_32_39_RX_LOS,
    // DBG_SFP56_16_23_TX_FAULT,
    // DBG_SFP56_24_31_TX_FAULT,
    // DBG_SFP56_32_39_TX_FAULT,

    /******************************************************************************
    * BSP DEBUG                                                                    *
    ******************************************************************************/
    //BSP DEBUG
    BSP_DEBUG,
    BSP_WP_ACCESS_COUNT
};

enum data_type {
    DATA_HEX,
    DATA_DEC,
    DATA_UNK,
};

enum reg_write_protect
{
    REG_WP_DIS = false,
    REG_WP_EN = true
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
    u8 reg;
    u8 mask;
    u8 data_type;
    bool write_protect;
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

u8 _cpld_reg_write(struct device *dev, u8 reg, u8 reg_val);
u8 _cpld_reg_read(struct device *dev, u8 reg, u8 mask);

#endif
