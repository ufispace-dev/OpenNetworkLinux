/* header file for i2c cpld driver of ufispace_s9520_28xc
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

#ifndef UFISPACE_S9520_28XC_CPLD_H
#define UFISPACE_S9520_28XC_CPLD_H

#include <linux/module.h>
#include <linux/i2c.h>
#include <dt-bindings/mux/mux.h>
#include <linux/i2c-mux.h>
#include <linux/version.h>

/* CPLD device index value */
enum cpld_id {
    cpld1,
    cpld2,
    fpga
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
/* Write Protect */
#define WRITE_PROTECT_1_REG                      0x70
#define WRITE_PROTECT_2_REG                      0x71


#define CPLD_I2C_CONTROL_REG                     0xA0
#define CPLD_I2C_RELAY_REG                       0xA5


/******************************************************************************
 * CPLD 1                                                                     *
 ******************************************************************************/
// & cpld1
/* Board information */
#define SKU_ID_REV_REG                           0x00
#define HW_BUILD_REV_REG                         0x01
#define CPLD_EXT_ID_REG                          0x06
/* Interrupt Status */
#define CLK_PTP_INTR_REG                         0x10
#define I2C_NIC_INTR_REG                         0x13
#define TOP_BRD_CPLD_FRU_INTR_REG                0x14
#define MAIN_BRD_CPLD_INTR_REG                   0x15
#define THERMAL_INTR_REG                         0x16
#define MAC_INTR_REG                             0x17
#define CPU_NMI_INTR_REG                         0x19
#define OUT_STATUS_INTR_REG                      0x1C

/* Interrupt Mask */
#define CLK_PTP_INTR_MASK_REG                    0x20
#define PHY_INTR_MASK_REG                        0x23
#define TOP_BRD_CPLD_FRU_INTR_MASK_REG           0x24
#define MAIN_BRD_CPLD_INTR_MASK_REG              0x25
#define THERMAL_INTR_MASK_REG                    0x26
#define MAC_INTR_MASK_REG                        0x27
#define CPU_NMI_INTR_MASK_REG                    0x29
#define OUT_STATUS_INTR_MASK_REG                 0x2C

/* Interrupt Event */
#define CLK_PTP_INTR_EVENT_REG                   0x30
#define PHY_INTR_EVENT_REG                       0x33
#define TOP_BRD_CPLD_FRU_INTR_EVENT_REG          0x34
#define MAIN_BRD_CPLD_INTR_EVENT_REG             0x35
#define THERMAL_INTR_EVENT_REG                   0x36
#define MAC_INTR_EVENT_REG                       0x37
#define CPU_NMI_INTR_EVENT_REG                   0x39
#define OUT_STATUS_INTR_EVENT_REG                0x3C

/* Reset */
#define MAC_RESET_REG                            0x40
#define BIOS_FLASH_RESET_CTRL_REG                0x41
#define CPU_BRD_CTRL_REG                         0x42
#define BMC_PHY_RESET_CTRL_REG                   0x43
#define USB_RESET_CTRL_REG                       0x44
#define JA_RESET_CTRL_REG                        0x45
#define I2C_MUX_RESET_REG                        0x46
#define NIC_CTRL_REG                             0x48
#define PSU_EEPROM_CTRL_REG                      0x49
#define SYS_PWR_RESET_REG                        0x4E

/* Misc Status Control */
#define PSU_STATUS_REG                           0x50
#define DAUGHTER_BRD_PRSNT_REG                   0x51
#define SYS_PWR_STATUS_REG                       0x52
#define CPU_STATUS_REG                           0x55
#define PHY_BOOT_CTRL_REG                        0x59
#define WD_STATUS_REG                            0x5A
#define TIMING_CTRL_STATUS_REG                   0x5B
#define MUX_CTRL_REG                             0x5C
#define BIOS_SPI_MUX_REG                         0x5D
#define PWR_SYSTEM_CTRL_REG                      0x5E

/* SERBOOT Control */
#define UFM_WRITE_EN_REG                         0x60
#define BIOS_BOOT_SEL_TGT_REG                    0x64

/* LED Clear & GNSS Control */
#define LED_CLEAR_REG                            0x65
#define GNSS_CTRL_REG                            0x66
#define TIMING_MISC_CTRL_REG                     0x68

/* Write Protect */

#define EXT_CTRL_REG                             0x7F

/* LED Control */
#define SYSTEM_LED_CTRL_1_REG                    0x80
#define SYSTEM_LED_CTRL_2_REG                    0x81
#define SYSTEM_LED_CTRL_3_REG                    0x82
#define FAN_LED_1_2_REG                          0x85
#define FAN_LED_3_4_REG                          0x86
#define FAN_LED_5_REG                            0x87

/* Power Status */
#define PWR_STATUS_1_REG                         0x90
#define PWR_STATUS_2_REG                         0x91
#define PWR_STATUS_3_REG                         0x92

/* MAC SPI (Shared Address, function depends on CPLD Logic) */
#define MAC_SPI_CTRL_0_REG                       0xB0
#define MAC_SPI_CTRL_1_REG                       0xB1
#define MAC_SPI_RD_DATA_0_REG                    0xB2
#define MAC_SPI_RD_DATA_1_REG                    0xB3
#define MAC_SPI_RD_DATA_2_REG                    0xB4
#define MAC_SPI_RD_DATA_3_REG                    0xB5
#define MAC_SPI_WR_DATA_0_REG                    0xB6
#define MAC_SPI_WR_DATA_1_REG                    0xB7
#define MAC_SPI_WR_DATA_2_REG                    0xB8
#define MAC_SPI_WR_DATA_3_REG                    0xB9
#define MAC_SPI_ADDR_0_REG                       0xBA
#define MAC_SPI_ADDR_1_REG                       0xBB
#define MAC_SPI_ADDR_2_REG                       0xBC
#define MAC_SPI_ADDR_3_REG                       0xBD
#define MAC_SPI_MODE_REG                         0xBE

/* BIOS SPI Control (Extended Range 0xB0-0xBE) - Ext1 */
#define EXT1_BIOS_SPI_CTRL_0_REG                 0xB0
#define EXT1_BIOS_SPI_CTRL_1_REG                 0xB1
#define EXT1_BIOS_SPI_RD_DATA_0_REG              0xB2
#define EXT1_BIOS_SPI_RD_DATA_1_REG              0xB3
#define EXT1_BIOS_SPI_RD_DATA_2_REG              0xB4
#define EXT1_BIOS_SPI_RD_DATA_3_REG              0xB5
#define EXT1_BIOS_SPI_WR_DATA_0_REG              0xB6
#define EXT1_BIOS_SPI_WR_DATA_1_REG              0xB7
#define EXT1_BIOS_SPI_WR_DATA_2_REG              0xB8
#define EXT1_BIOS_SPI_WR_DATA_3_REG              0xB9
#define EXT1_BIOS_SPI_ADDR_0_REG                 0xBA
#define EXT1_BIOS_SPI_ADDR_1_REG                 0xBB
#define EXT1_BIOS_SPI_ADDR_2_REG                 0xBC
#define EXT1_BIOS_SPI_ADDR_3_REG                 0xBD
#define EXT1_BIOS_SPI_EN_REG                     0xBE

/* Timing Control */
#define OCXO_GNSS_ID_REG                         0xC0
#define CLK_PTP_RESET_REG                        0xC2
#define CLK_TIMING_CTRL_REG                      0xC3
#define GNSS_STATUS_REG                          0xC4
#define TIMING_STATUS_REG                        0xC5

/* Interrupt Debug & BIOS SPI Extended */
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

/* Interrupt Debug */
#define DBG_CLK_PTP_INTR_REG                     0xE0
#define DBG_I2C_INTR_REG                         0xE3
#define DBG_TOP_BRD_CPLD_FRU_INTR_REG            0xE4
#define DBG_MAIN_BRD_CPLD_INTR_REG               0xE5
#define DBG_THERMAL_INTR_REG                     0xE6
#define DBG_MAC_INTR_REG                         0xE7
#define DBG_CPU_NMI_INTR_REG                     0xE9
#define DBG_OUT_STATUS_INTR_REG                  0xEC

/******************************************************************************
 * CPLD 2                                                                     *
 ******************************************************************************/
// & cpld2

/* Ports Interrupt Status */
#define QSFP28_0_3_ABS_REG                       0x10
#define SFP28_4_11_ABS_REG                       0x11
#define SFP28_12_19_ABS_REG                      0x12
#define SFP56_20_27_ABS_REG                      0x13
#define QSFP28_0_3_INTR_REG                      0x14
#define SFP28_4_11_RX_LOS_REG                    0x15
#define SFP28_12_19_RX_LOS_REG                   0x16
#define SFP56_20_27_RX_LOS_REG                   0x17
#define QSFP28_0_3_FUSE_REG                      0x18
#define SFP28_4_11_TX_FLT_REG                    0x1C
#define SFP28_12_19_TX_FLT_REG                   0x1D
#define SFP56_20_27_TX_FLT_REG                   0x1E

/* Interrupt Mask */
#define QSFP28_0_3_MASK_ABS_REG                  0x20
#define SFP28_4_11_MASK_ABS_REG                  0x21
#define SFP28_12_19_MASK_ABS_REG                 0x22
#define SFP56_20_27_MASK_ABS_REG                 0x23
#define QSFP28_0_3_INTR_MASK_REG                 0x24
#define SFP28_4_11_RX_LOS_MASK_REG               0x25
#define SFP28_12_19_RX_LOS_MASK_REG              0x26
#define SFP56_20_27_RX_LOS_MASK_REG              0x27
#define QSFP28_0_3_FUSE_MASK_REG                 0x28
#define SFP28_4_11_TX_FLT_MASK_REG               0x2C
#define SFP28_12_19_TX_FLT_MASK_REG              0x2D
#define SFP56_20_27_TX_FLT_MASK_REG              0x2E

/* Interrupt Event */
#define QSFP28_0_3_ABS_EVENT_REG                 0x30
#define SFP28_4_11_ABS_EVENT_REG                 0x31
#define SFP28_12_19_ABS_EVENT_REG                0x32
#define SFP56_20_27_ABS_EVENT_REG                0x33
#define QSFP28_0_3_INTR_EVENT_REG                0x34
#define SFP28_4_11_RX_LOS_EVENT_REG              0x35
#define SFP28_12_19_RX_LOS_EVENT_REG             0x36
#define SFP56_20_27_RX_LOS_EVENT_REG             0x37
#define QSFP28_0_3_FUSE_EVENT_REG                0x38
#define SFP28_4_11_TX_FLT_EVENT_REG              0x3C
#define SFP28_12_19_TX_FLT_EVENT_REG             0x3D
#define SFP56_20_27_TX_FLT_EVENT_REG             0x3E

/* Reset and Control */
#define QSFP28_0_3_RESET_REG                     0x40
#define QSFP28_0_3_LPMODE_REG                    0x42

#define SSD_CTRL_REG                             0x47

#define SFP28_4_11_TX_DISABLE_REG                0x49
#define SFP28_12_19_TX_DISABLE_REG               0x4A
#define SFP56_20_27_TX_DISABLE_REG               0x4B

#define SFP28_4_11_RATE_SELECT_REG               0x4C
#define SFP28_12_19_RATE_SELECT_REG              0x4D
#define SFP56_20_27_RATE_SELECT_REG              0x4E

/* Status */
#define USB_CTRL_REG                             0x50
#define SSD_STATUS_REG                           0x51
#define SFP28_4_19_OC_REG                        0x52
#define SFP56_20_27_OC_REG                       0x53

#define QSFP28_0_3_I2C_STUCK_STATUS_REG          0x5A
#define SFP28_4_11_I2C_STUCK_STATUS_REG          0x5B
#define SFP28_12_19_I2C_STUCK_STATUS_REG         0x5C
#define SFP56_20_27_I2C_STUCK_STATUS_REG         0x5D
#define CPU_I2C_STUCK_STATUS_REG                 0x5E


#define QSFP28_0_3_I2C_STUCK_MASK_REG            0x6A
#define SFP28_4_11_I2C_STUCK_MASK_REG            0x6B
#define SFP28_12_19_I2C_STUCK_MASK_REG           0x6C
#define SFP56_20_27_I2C_STUCK_MASK_REG           0x6D
#define CPU_I2C_STUCK_MASK_REG                   0x6E

#define QSFP28_0_3_I2C_STUCK_EVENT_REG           0x7A
#define SFP28_4_11_I2C_STUCK_EVENT_REG           0x7B
#define SFP28_12_19_I2C_STUCK_EVENT_REG          0x7C
#define SFP56_20_27_I2C_STUCK_EVENT_REG          0x7D
#define CPU_I2C_STUCK_EVENT_REG                  0x7E

/* Debug Registers CPLD2 */
#define DBG_QSFP28_0_3_ABS_REG                   0xE0
#define DBG_SFP28_4_11_ABS_REG                   0xE1
#define DBG_SFP28_12_19_ABS_REG                  0xE2
#define DBG_SFP56_20_27_ABS_REG                  0xE3
#define DBG_QSFP28_0_3_INTR_REG                  0xE4
#define DBG_SFP28_4_11_RX_LOS_REG                0xE5
#define DBG_SFP28_12_19_RX_LOS_REG               0xE6
#define DBG_SFP56_20_27_RX_LOS_REG               0xE7
#define DBG_QSFP28_0_3_FUSE_REG                  0xE8

#define DBG_SFP28_4_11_TX_FLT_REG                0xEC
#define DBG_SFP28_12_19_TX_FLT_REG               0xED
#define DBG_SFP56_20_27_TX_FLT_REG               0xEE

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
#define CPLD_MAX_NCHANS                         28
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

/* --- Enums (cpld_sysfs_attributes) --- */
enum cpld_sysfs_attributes {
    CPLD_MINOR_VER,
    CPLD_MAJOR_VER,
    CPLD_VERSION,
    CPLD_VERSION_H,
    CPLD_ID,
    CPLD_BUILD_VER,
    CPLD_CHIP_TYPE,
    EVENT_CTRL,
    EVENT_DETECT_CTRL,
    REMOTE_I2C_RESET,
    MODULE_RESET,
    CPLD_WRITE_PROTECT,
    CPLD_SKU_ID,
    BRD_HW_ID,
    DEPH_ID,
    BUILD_ID,
    BIT_SEL_ID,
    HW_BUILD_REV,
    EXT_ID,
    CPLD_EXT_ID,
    CJA_LOL_INTR,
    NTM_INTR,
    DPLL_1588_INTR,
    DPLL_SYNC_INTR,
    BITS_INTR,
    CLK_PTP_INTR,
    I2C_NIC_ALRT,
    I2C_NIC_INTR,
    PSU1_INTR,
    PSU2_INTR,
    VDD_CORE_PIN_ALRT_INTR,
    CPLD2_INTR,
    FAN_CARD_INTR,
    TOP_BRD_CPLD_FRU_INTR,
    USB_PWR_OC,
    SSD1_PWR_OC,
    SSD2_PWR_OC,
    GNSS_INTR,
    HWM_NMI_INTR,
    MAIN_BRD_CPLD_INTR,
    TSEN_NMI_INTR,
    TSEN1_NMI_INTR,
    TSEN2_NMI_INTR,
    TSEN_ALRT_INTR,
    TSEN1_ALRT_INTR,
    TSEN2_ALRT_INTR,
    CPU_THRMTRIP,
    VDD_CORE_VRHOT,
    THERMAL_INTR,
    MAC_INTR,
    CPU_NMI_INTR,
    USB_PWR_OC_CPU,
    NMI_CPLD_TO_CPU,
    PTP_TO_CPU_INTR,
    ETH_TO_CPU_INTR,
    THRM_CPLD_TO_CPU_INTR,
    NMI_SYS_TO_BMC,
    THRM_CHIP_INTR,
    CPLD_TO_CPU_INTR,
    BITS_INTR_MASK,
    DPLL_SYNC_INTR_MASK,
    DPLL_1588_INTR_MASK,
    NTM_INTR_MASK,
    CJA_LOL_INTR_MASK,
    CLK_PTP_INTR_MASK,
    I2C_NIC_ALRT_MASK,
    PHY_INTR_MASK,
    FAN_CARD_INTR_MASK,
    CPLD2_INTR_MASK,
    PSU2_INTR_MASK,
    PSU1_INTR_MASK,
    TOP_BRD_CPLD_FRU_INTR_MASK,
    HWM_NMI_INTR_MASK,
    GNSS_INTR_MASK,
    SSD2_PWR_OC_MASK,
    SSD1_PWR_OC_MASK,
    USB_PWR_OC_MASK,
    MAIN_BRD_CPLD_INTR_MASK,
    VDD_CORE_VRHOT_MASK,
    CPU_THRMTRIP_MASK,
    TSEN2_ALRT_INTR_MASK,
    TSEN1_ALRT_INTR_MASK,
    TSEN_ALRT_INTR_MASK,
    TSEN2_NMI_INTR_MASK,
    TSEN1_NMI_INTR_MASK,
    TSEN_NMI_INTR_MASK,
    THERMAL_INTR_MASK,
    MAC_INT_MASK,
    MAC_INTR_MASK,
    CPU_NMI_INTR_MASK,
    CPLD_TO_CPU_INTR_MASK,
    THRM_CHIP_INTR_MASK,
    NMI_SYS_TO_BMC_MASK,
    THRM_CPLD_TO_CPU_INTR_MASK,
    ETH_TO_CPU_INTR_MASK,
    PTP_TO_CPU_INTR_MASK,
    NMI_CPLD_TO_CPU_MASK,
    USB_PWR_OC_CPU_MASK,
    OUT_STATUS_INTR_MASK,
    CLK_PTP_INTR_EVENT,
    PHY_INTR_EVENT,
    TOP_BRD_CPLD_FRU_INTR_EVENT,
    MAIN_BRD_CPLD_INTR_EVENT,
    THERMAL_INTR_EVENT,
    MAC_INTR_EVENT,
    CPU_NMI_INTR_EVENT,
    OUT_STATUS_INTR_EVENT,
    MAC_SYS_RESET,
    MAC_QSPI_RESET,
    MAC_RESET,
    SPI_BIOS_RESET,
    BTN_FP_RESET,
    BIOS_FLASH_RESET_CTRL,
    CPLD_TO_CPU_RESET,
    CPLD_TO_BMC_SYS_RESET,
    BMC_PCIE_RESET,
    BMC_LPC_RESET,
    CPU_MON_RESET,
    BMC_PHY_RESET_CTRL,
    USB_PWR_EN,
    USB_RESET_CTRL,
    CJA_RESET,
    OE_CJA,
    JA_WP,
    JA_RESET_CTRL,
    I2C_MUX_0X75_RESET,
    I2C_MUX_0X77_RESET,
    I2C_MUX_0X76_RESET,
    I2C_MUX_RESET,
    NIC1_PCIE_RESET,
    CPLD1_TO_CPLD2_RESET,
    NIC_CTRL,
    PSU1_EEPROM_WP,
    PSU2_EEPROM_WP,
    PSU_EEPROM_CTRL,
    SYS_PWR_RESET,
    PSU1_PRESENT,
    PSU2_PRESENT,
    PSU1_VIN_PWROK,
    PSU2_VIN_PWROK,
    PSU1_VOUT_PWROK,
    PSU2_VOUT_PWROK,
    PSU_STATUS,
    DAUGHTER_BRD_PRESENT,
    CPU_PWRGD,
    CPU_BOOT_DONE,
    WAKE_CPU_PCIE,
    SYS_PWR_STATUS,
    PHY_BOOT_CTRL,
    BMC_WDT1_RESET,
    BMC_WDT2_RESET,
    WD_STATUS,
    BIOS_BOOT_SEL0,
    BIOS_BOOT_SEL1,
    TIMING_CTRL_STATUS,
    I2C_ROV_MUX_SEL,
    QSPI_MAC_MUX_SEL,
    I2C_CPLD_MUX_SEL,
    UART_MUX_SEL,
    UART_CPU_BMC_MUX_SEL,
    MUX_CTRL,
    ICX_SPI_MUX_SEL_1,
    ICX_SPI_MUX_SEL_2,
    ICX_SPI_MUX_SEL_3,
    BIOS_SPI_MUX,
    PWREN_CPLD_TO_CPU,
    PWRBTN_CPLD_TO_CPU,
    PWR_SYSTEM_CTRL,
    UFM_WRITE,
    UFM_WRITE_EN,
    BIOS_BOOT_SEL0_TGT,
    BIOS_BOOT_SEL1_TGT,
    BIOS_BOOT_SEL_TGT,
    LED_FAN_CLR,
    LED_CLEAR,
    GNSS_ANT_PWREN,
    GNSS_RESET,
    ST_GNSS_10M_OUT,
    GNSS_CTRL,
    TS_PLL_CLK_EN,
    P3V3_EN_BROADSYNC,
    TIMING_MISC_CTRL,
    EXT_REG_DEFINE,
    EXT_REG_RANGE,
    EXT_CTRL,
    PWR_LED_COLOR,
    PWR_LED_SPEED,
    PWR_LED_BLINKING,
    PWR_LED_ON_OFF,
    CPLD_SYSTEM_LED_PWR,
    FAN_LED_COLOR,
    FAN_LED_SPEED,
    FAN_LED_BLINKING,
    FAN_LED_ON_OFF,
    CPLD_SYSTEM_LED_FAN,
    SYSTEM_LED_CTRL_1,
    GNSS_LED_COLOR,
    GNSS_LED_SPEED,
    GNSS_LED_BLINKING,
    GNSS_LED_ON_OFF,
    CPLD_SYSTEM_LED_GNSS,
    SYS_LED_COLOR,
    SYS_LED_SPEED,
    SYS_LED_BLINKING,
    SYS_LED_ON_OFF,
    CPLD_SYSTEM_LED_SYS,
    SYSTEM_LED_CTRL_2,
    SYNC_LED_COLOR,
    SYNC_LED_SPEED,
    SYNC_LED_BLINKING,
    SYNC_LED_ON_OFF,
    CPLD_SYSTEM_LED_SYNC,
    ID_LED_ON_OFF,
    CPLD_SYSTEM_LED_ID,
    ID_LED_BLINKING,
    ID_LED_SPEED,
    SYSTEM_LED_CTRL_3,
    FAN_LED_1_2,
    FAN2_LED_ON_OFF,
    FAN2_LED_BLINKING,
    FAN2_LED_SPEED,
    FAN2_LED_COLOR,
    FAN1_LED_ON_OFF,
    FAN1_LED_BLINKING,
    FAN1_LED_SPEED,
    FAN1_LED_COLOR,
    FAN_LED_3_4,
    FAN4_LED_ON_OFF,
    FAN4_LED_BLINKING,
    FAN4_LED_SPEED,
    FAN4_LED_COLOR,
    FAN3_LED_ON_OFF,
    FAN3_LED_BLINKING,
    FAN3_LED_SPEED,
    FAN3_LED_COLOR,
    FAN_LED_5,
    FAN5_LED_ON_OFF,
    FAN5_LED_BLINKING,
    FAN5_LED_SPEED,
    FAN5_LED_COLOR,
    P5V_AUX_PG,
    P3V3_PG,
    P1V8_CLK_PG,
    P1V2_CPLD2_PG,
    P2V5_CPLD2_PG,
    P1V8_VDD0_PG,
    VDD_CORE_PG,
    P0V9_PVDD_PG,
    PWR_STATUS_1,
    P0V75_TRVDD100_PG,
    P0V75_TRVDD50_PG,
    P0V9_TRVDD_PG,
    P0V8_AVDD_PG,
    P1V2_TRVDD_PG,
    P1V2_PVDD_PG,
    P1V5_AVDD_PG,
    P1V5_TRXVDD_PG,
    PWR_STATUS_2,
    P0V75_DDR_VDDC_PG,
    P1V05_DDR_VDDQX_PG,
    P1V8_DDR_PG,
    P0V5_DDR_VDDQ_PG,
    PWR_STATUS_3,
    MAC_SPI_CTRL_0,
    MAC_SPI_CTRL_1,
    MAC_SPI_RD_DATA_0,
    MAC_SPI_RD_DATA_1,
    MAC_SPI_RD_DATA_2,
    MAC_SPI_RD_DATA_3,
    MAC_SPI_WR_DATA_0,
    MAC_SPI_WR_DATA_1,
    MAC_SPI_WR_DATA_2,
    MAC_SPI_WR_DATA_3,
    MAC_SPI_ADDR_0,
    MAC_SPI_ADDR_1,
    MAC_SPI_ADDR_2,
    MAC_SPI_ADDR_3,
    MAC_SPI_MODE,
    OCXO_ID,
    GNSS_MODL_ID,
    OCXO_GNSS_ID,
    NTM_RESET,
    BITS_RESET,
    CLK_PTP_RESET,
    TS_PLL_CLK_SOURCE_SEL,
    SMB_1PPS_DIR_SEL,
    CLK_10M_PTP_IN,
    SMB_10M_INPUT_EN,
    PTP_TOD_RS422_DIR_CTRL,
    PTP_1PPS_RS422_DIR_CTRL,
    SMB_10M_DIR_SEL,
    CLK_TIMING_CTRL,
    GNSS_ANT_SHORT,
    GNSS_ANT_OPEN,
    GNSS_ANT_ON,
    GNSS_STATUS,
    NTM_PRESENT,
    GNSS_PRESENT,
    GNSS_ANT_PWR_ST,
    TIMING_STATUS,
    SYNCE_CH_SET,
    I2C_MUX_RESET_MB,
    ETH_0_PRESENT,
    ETH_1_PRESENT,
    ETH_2_PRESENT,
    ETH_3_PRESENT,
    ETH_4_PRESENT,
    ETH_5_PRESENT,
    ETH_6_PRESENT,
    ETH_7_PRESENT,
    ETH_8_PRESENT,
    ETH_9_PRESENT,
    ETH_10_PRESENT,
    ETH_11_PRESENT,
    ETH_12_PRESENT,
    ETH_13_PRESENT,
    ETH_14_PRESENT,
    ETH_15_PRESENT,
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
    ETH_0_INTR,
    ETH_1_INTR,
    ETH_2_INTR,
    ETH_3_INTR,
    ETH_4_RX_LOS,
    ETH_5_RX_LOS,
    ETH_6_RX_LOS,
    ETH_7_RX_LOS,
    ETH_8_RX_LOS,
    ETH_9_RX_LOS,
    ETH_10_RX_LOS,
    ETH_11_RX_LOS,
    ETH_12_RX_LOS,
    ETH_13_RX_LOS,
    ETH_14_RX_LOS,
    ETH_15_RX_LOS,
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
    ETH_0_FUSE,
    ETH_1_FUSE,
    ETH_2_FUSE,
    ETH_3_FUSE,
    ETH_4_TX_FLT,
    ETH_5_TX_FLT,
    ETH_6_TX_FLT,
    ETH_7_TX_FLT,
    ETH_8_TX_FLT,
    ETH_9_TX_FLT,
    ETH_10_TX_FLT,
    ETH_11_TX_FLT,
    ETH_12_TX_FLT,
    ETH_13_TX_FLT,
    ETH_14_TX_FLT,
    ETH_15_TX_FLT,
    ETH_16_TX_FLT,
    ETH_17_TX_FLT,
    ETH_18_TX_FLT,
    ETH_19_TX_FLT,
    ETH_20_TX_FLT,
    ETH_21_TX_FLT,
    ETH_22_TX_FLT,
    ETH_23_TX_FLT,
    ETH_24_TX_FLT,
    ETH_25_TX_FLT,
    ETH_26_TX_FLT,
    ETH_27_TX_FLT,
    ETH_0_PRESENT_MASK,
    ETH_1_PRESENT_MASK,
    ETH_2_PRESENT_MASK,
    ETH_3_PRESENT_MASK,
    ETH_4_PRESENT_MASK,
    ETH_5_PRESENT_MASK,
    ETH_6_PRESENT_MASK,
    ETH_7_PRESENT_MASK,
    ETH_8_PRESENT_MASK,
    ETH_9_PRESENT_MASK,
    ETH_10_PRESENT_MASK,
    ETH_11_PRESENT_MASK,
    ETH_12_PRESENT_MASK,
    ETH_13_PRESENT_MASK,
    ETH_14_PRESENT_MASK,
    ETH_15_PRESENT_MASK,
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
    ETH_0_INTR_MASK,
    ETH_1_INTR_MASK,
    ETH_2_INTR_MASK,
    ETH_3_INTR_MASK,
    ETH_4_RX_LOS_MASK,
    ETH_5_RX_LOS_MASK,
    ETH_6_RX_LOS_MASK,
    ETH_7_RX_LOS_MASK,
    ETH_8_RX_LOS_MASK,
    ETH_9_RX_LOS_MASK,
    ETH_10_RX_LOS_MASK,
    ETH_11_RX_LOS_MASK,
    ETH_12_RX_LOS_MASK,
    ETH_13_RX_LOS_MASK,
    ETH_14_RX_LOS_MASK,
    ETH_15_RX_LOS_MASK,
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
    ETH_0_FUSE_MASK,
    ETH_1_FUSE_MASK,
    ETH_2_FUSE_MASK,
    ETH_3_FUSE_MASK,
    ETH_4_TX_FLT_MASK,
    ETH_5_TX_FLT_MASK,
    ETH_6_TX_FLT_MASK,
    ETH_7_TX_FLT_MASK,
    ETH_8_TX_FLT_MASK,
    ETH_9_TX_FLT_MASK,
    ETH_10_TX_FLT_MASK,
    ETH_11_TX_FLT_MASK,
    ETH_12_TX_FLT_MASK,
    ETH_13_TX_FLT_MASK,
    ETH_14_TX_FLT_MASK,
    ETH_15_TX_FLT_MASK,
    ETH_16_TX_FLT_MASK,
    ETH_17_TX_FLT_MASK,
    ETH_18_TX_FLT_MASK,
    ETH_19_TX_FLT_MASK,
    ETH_20_TX_FLT_MASK,
    ETH_21_TX_FLT_MASK,
    ETH_22_TX_FLT_MASK,
    ETH_23_TX_FLT_MASK,
    ETH_24_TX_FLT_MASK,
    ETH_25_TX_FLT_MASK,
    ETH_26_TX_FLT_MASK,
    ETH_27_TX_FLT_MASK,
    QSFP28_0_3_PRESENT_EVENT,
    SFP28_4_11_PRESENT_EVENT,
    SFP28_12_19_PRESENT_EVENT,
    SFP56_20_27_PRESENT_EVENT,
    QSFP28_0_3_INTR_EVENT,
    SFP28_4_11_RX_LOS_EVENT,
    SFP28_12_19_RX_LOS_EVENT,
    SFP56_20_27_RX_LOS_EVENT,
    QSFP28_0_3_FUSE_EVENT,
    SFP28_4_11_TX_FLT_EVENT,
    SFP28_12_19_TX_FLT_EVENT,
    SFP56_20_27_TX_FLT_EVENT,
    ETH_0_PRESENT_EVENT,
    ETH_1_PRESENT_EVENT,
    ETH_2_PRESENT_EVENT,
    ETH_3_PRESENT_EVENT,
    ETH_4_PRESENT_EVENT,
    ETH_5_PRESENT_EVENT,
    ETH_6_PRESENT_EVENT,
    ETH_7_PRESENT_EVENT,
    ETH_8_PRESENT_EVENT,
    ETH_9_PRESENT_EVENT,
    ETH_10_PRESENT_EVENT,
    ETH_11_PRESENT_EVENT,
    ETH_12_PRESENT_EVENT,
    ETH_13_PRESENT_EVENT,
    ETH_14_PRESENT_EVENT,
    ETH_15_PRESENT_EVENT,
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
    ETH_0_RESET,
    ETH_1_RESET,
    ETH_2_RESET,
    ETH_3_RESET,
    ETH_0_LPMODE,
    ETH_1_LPMODE,
    ETH_2_LPMODE,
    ETH_3_LPMODE,
    SSD1_M2_CONFIG,
    SSD2_M2_CONFIG,
    CPU_SSD1_PERST,
    CPU_SSD2_PERST,
    PWREN_P3V3_SSD1,
    PWREN_P3V3_SSD2,
    ETH_4_TX_DISABLE,
    ETH_5_TX_DISABLE,
    ETH_6_TX_DISABLE,
    ETH_7_TX_DISABLE,
    ETH_8_TX_DISABLE,
    ETH_9_TX_DISABLE,
    ETH_10_TX_DISABLE,
    ETH_11_TX_DISABLE,
    ETH_12_TX_DISABLE,
    ETH_13_TX_DISABLE,
    ETH_14_TX_DISABLE,
    ETH_15_TX_DISABLE,
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
    ETH_4_RATE_SELECT,
    ETH_5_RATE_SELECT,
    ETH_6_RATE_SELECT,
    ETH_7_RATE_SELECT,
    ETH_8_RATE_SELECT,
    ETH_9_RATE_SELECT,
    ETH_10_RATE_SELECT,
    ETH_11_RATE_SELECT,
    ETH_12_RATE_SELECT,
    ETH_13_RATE_SELECT,
    ETH_14_RATE_SELECT,
    ETH_15_RATE_SELECT,
    ETH_16_RATE_SELECT,
    ETH_17_RATE_SELECT,
    ETH_18_RATE_SELECT,
    ETH_19_RATE_SELECT,
    ETH_20_RATE_SELECT,
    ETH_21_RATE_SELECT,
    ETH_22_RATE_SELECT,
    ETH_23_RATE_SELECT,
    ETH_24_RATE_SELECT,
    ETH_25_RATE_SELECT,
    ETH_26_RATE_SELECT,
    ETH_27_RATE_SELECT,
    USB_CTRL_PRESENT,
    NVME_SSD2_PRESENT,
    NVME_SSD1_PRESENT,
    ETH_4_5_OC,
    ETH_6_7_OC,
    ETH_8_9_OC,
    ETH_10_11_OC,
    ETH_12_13_OC,
    ETH_14_15_OC,
    ETH_16_17_OC,
    ETH_18_19_OC,
    ETH_20_OC,
    ETH_21_OC,
    ETH_22_OC,
    ETH_23_OC,
    ETH_24_OC,
    ETH_25_OC,
    ETH_26_OC,
    ETH_27_OC,
    ETH_0_I2C_STUCK,
    ETH_1_I2C_STUCK,
    ETH_2_I2C_STUCK,
    ETH_3_I2C_STUCK,
    ETH_4_I2C_STUCK,
    ETH_5_I2C_STUCK,
    ETH_6_I2C_STUCK,
    ETH_7_I2C_STUCK,
    ETH_8_I2C_STUCK,
    ETH_9_I2C_STUCK,
    ETH_10_I2C_STUCK,
    ETH_11_I2C_STUCK,
    ETH_12_I2C_STUCK,
    ETH_13_I2C_STUCK,
    ETH_14_I2C_STUCK,
    ETH_15_I2C_STUCK,
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
    CPU_LEGACY_SIDE_I2C_STUCK,
    CPU_I2C_STUCK_STATUS,
    ETH_0_I2C_STUCK_MASK,
    ETH_1_I2C_STUCK_MASK,
    ETH_2_I2C_STUCK_MASK,
    ETH_3_I2C_STUCK_MASK,
    ETH_4_I2C_STUCK_MASK,
    ETH_5_I2C_STUCK_MASK,
    ETH_6_I2C_STUCK_MASK,
    ETH_7_I2C_STUCK_MASK,
    ETH_8_I2C_STUCK_MASK,
    ETH_9_I2C_STUCK_MASK,
    ETH_10_I2C_STUCK_MASK,
    ETH_11_I2C_STUCK_MASK,
    ETH_12_I2C_STUCK_MASK,
    ETH_13_I2C_STUCK_MASK,
    ETH_14_I2C_STUCK_MASK,
    ETH_15_I2C_STUCK_MASK,
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
    CPU_LEGACY_SIDE_I2C_STUCK_MASK,
    CPU_I2C_STUCK_MASK,
    QSFP28_0_3_I2C_STUCK_EVENT,
    SFP28_4_11_I2C_STUCK_EVENT,
    SFP28_12_19_I2C_STUCK_EVENT,
    SFP56_20_27_I2C_STUCK_EVENT,
    CPU_I2C_STUCK_EVENT,
    //MUX
    IDLE_STATE,

    //BSP DEBUG
    BSP_DEBUG,
    BSP_WP_ACCESS_COUNT
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
