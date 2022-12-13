/* header file for i2c cpld driver of ufispace_s9110_32x
 *
 * Copyright (C) 2022 UfiSpace Technology Corporation.
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

#ifndef UFISPACE_S9110_32X_CPLD_H
#define UFISPACE_S9110_32X_CPLD_H

/* CPLD device index value */
enum cpld_id {
    cpld1,
    cpld2
};

/* CPLD common registers */
#define CPLD_VERSION_REG                  0x02
#define CPLD_ID_REG                       0x03
#define CPLD_BUILD_REG                    0x04
#define CPLD_CHIP_REG                     0x05

#define CPLD_EVT_CTRL_REG                 0x3F

/* CPLD 1 registers */
#define CPLD_BOARD_ID_0_REG               0x00
#define CPLD_BOARD_ID_1_REG               0x01
#define CPLD_SKU_EXT_REG                  0x06

#define CPLD_MAC_INTR_REG                 0x10
#define CPLD_HWM_INTR_REG                 0x13
#define CPLD_CPLD2_INTR_REG               0x14
#define CPLD_NTM_INTR_REG                 0x15
#define CPLD_FAN_PSU_INTR_REG             0x16
#define CPLD_IOEXP_SFP_RS_INTR_REG        0x18
#define CPLD_CPU_NMI_INTR_REG             0x19
#define CPLD_PTP_INTR_REG                 0x1B
#define CPLD_SYSTEM_INTR_REG              0x1C

#define CPLD_MAC_MASK_REG                 0x20
#define CPLD_HWM_MASK_REG                 0x23
#define CPLD_CPLD2_MASK_REG               0x24
#define CPLD_NTM_MASK_REG                 0x25
#define CPLD_FAN_PSU_MASK_REG             0x26
#define CPLD_QSFP_IOEXP_MASK_REG          0x28
#define CPLD_CPU_NMI_MASK_REG             0x29
#define CPLD_PTP_MASK_REG                 0x2B
#define CPLD_SYSTEM_MASK_REG              0x2C

#define CPLD_MAC_EVT_REG                  0x30
#define CPLD_HWM_EVT_REG                  0x33
#define CPLD_CPLD2_EVT_REG                0x34
#define CPLD_NTM_EVT_REG                  0x35
#define CPLD_FAN_PSU_EVT_REG              0x36
#define CPLD_QSFP_IOEXP_EVT_REG           0x38
#define CPLD_CPU_NMI_EVT_REG              0x39
#define CPLD_PTP_EVT_REG                  0x3B
#define CPLD_SYSTEM_EVT_REG               0x3C

#define CPLD_MAC_RESET_REG                0x40
#define CPLD_SYSTEM_RESET_REG             0x41
#define CPLD_BMC_NTM_RESET_REG            0x43
#define CPLD_USB_RESET_REG                0x44
#define CPLD_I2C_MUX_RESET_REG            0x46
#define CPLD_I2C_MUX_RESET_2_REG          0x47
#define CPLD_MISC_RESET_REG               0x48

#define CPLD_BRD_PRESENT_REG              0x50
#define CPLD_PSU_STATUS_REG               0x51
#define CPLD_SYSTEM_PWR_REG               0x52
#define CPLD_MAC_SYNCE_REG                0x53
#define CPLD_MAC_AVS_REG                  0x54
#define CPLD_SYSTEM_STATUS_REG            0x55
#define CPLD_FAN_PRESENT_REG              0x56
#define CPLD_WATCHDOG_REG                 0x5A
#define CPLD_BOOT_SELECT_REG              0x5B
#define CPLD_MUX_CTRL_REG                 0x5C
#define CPLD_MISC_CTRL_1_REG              0x5D
#define CPLD_MISC_CTRL_2_REG              0x5E
#define CPLD_TIMING_CTRL_REG              0x5F

#define CPLD_MAC_TEMP_REG                 0x61

#define CPLD_SYSTEM_LED_PSU_REG           0x80
#define CPLD_SYSTEM_LED_SYS_REG           0x81
#define CPLD_SYSTEM_LED_SYNC_REG          0x82
#define CPLD_SYSTEM_LED_FAN_REG           0x83
#define CPLD_SYSTEM_LED_ID_REG            0x84

#define CPLD_MAC_PG_REG                   0x90
#define CPLD_MISC_PG_REG                  0x92
#define CPLD_MAC_PG_EN_REG                0x93
#define CPLD_MISC_PG_EN_REG               0x95

#define DBG_CPLD_MAC_INTR_REG             0xE0
#define DBG_CPLD_HWM_INTR_REG             0xE3
#define DBG_CPLD_CPLD2_INTR_REG           0xE4
#define DBG_CPLD_NTM_INTR_REG             0xE5
#define DBG_CPLD_FAN_PSU_INTR_REG         0xE6
#define DBG_CPLD_IOEXP_SFP_RS_INTR_REG    0xE8
#define DBG_CPLD_PTP_INTR_REG             0xEB

#define CPLD_UPG_RESET_REG                0xF0

/* CPLD 2*/

//interrupt status
#define CPLD_QSFP_INTR_PRESENT_0_7_REG     0x10
#define CPLD_QSFP_INTR_PRESENT_8_15_REG    0x11
#define CPLD_QSFP_INTR_PRESENT_16_23_REG   0x12
#define CPLD_QSFP_INTR_PRESENT_24_31_REG   0x13

#define CPLD_QSFP_INTR_PORT_0_7_REG        0xD0
#define CPLD_QSFP_INTR_PORT_8_15_REG       0xD0
#define CPLD_QSFP_INTR_PORT_16_23_REG      0xD0
#define CPLD_QSFP_INTR_PORT_24_31_REG      0xD0

//interrupt mask
#define CPLD_QSFP_MASK_PRESENT_0_7_REG     0x20
#define CPLD_QSFP_MASK_PRESENT_8_15_REG    0x21
#define CPLD_QSFP_MASK_PRESENT_16_23_REG   0x22
#define CPLD_QSFP_MASK_PRESENT_24_31_REG   0x23

#define CPLD_QSFP_MASK_PORT_0_7_REG             0x20
#define CPLD_QSFP_MASK_PORT_8_15_REG            0x21
#define CPLD_QSFP_MASK_PORT_16_23_REG           0x22
#define CPLD_QSFP_MASK_PORT_24_31_REG           0x23

//interrupt event
#define CPLD_QSFP_EVT_PRESENT_0_7_REG      0x30
#define CPLD_QSFP_EVT_PRESENT_8_15_REG     0x31
#define CPLD_QSFP_EVT_PRESENT_16_23_REG    0x32
#define CPLD_QSFP_EVT_PRESENT_24_31_REG    0x33

#define CPLD_QSFP_EVT_PORT_0_7_REG         0x30
#define CPLD_QSFP_EVT_PORT_8_15_REG        0x31
#define CPLD_QSFP_EVT_PORT_16_23_REG       0x32
#define CPLD_QSFP_EVT_PORT_24_31_REG       0x33

#define CPLD_QSFP_RESET_0_7_REG            0x76
#define CPLD_QSFP_RESET_8_15_REG           0x76
#define CPLD_QSFP_RESET_16_23_REG          0x76
#define CPLD_QSFP_RESET_24_31_REG          0x76

#define CPLD_QSFP_LPMODE_0_7_REG           0x76
#define CPLD_QSFP_LPMODE_8_15_REG          0x76
#define CPLD_QSFP_LPMODE_16_23_REG         0x76
#define CPLD_QSFP_LPMODE_24_31_REG         0x76

//debug interrupt status
#define DBG_CPLD_QSFP_INTR_PRESENT_BASE_REG   0xD0
#define DBG_CPLD_QSFP_INTR_PRESENT_0_7_REG    0xD0
#define DBG_CPLD_QSFP_INTR_PRESENT_8_15_REG   0xD0
#define DBG_CPLD_QSFP_INTR_PRESENT_16_23_REG  0xD0
#define DBG_CPLD_QSFP_INTR_PRESENT_24_31_REG  0xD0

#define DBG_CPLD_QSFP_INTR_PORT_0_7_REG       0xD0
#define DBG_CPLD_QSFP_INTR_PORT_8_15_REG      0xD0
#define DBG_CPLD_QSFP_INTR_PORT_16_23_REG     0xD0
#define DBG_CPLD_QSFP_INTR_PORT_24_31_REG     0xD0

//MASK
#define MASK_ALL                           (0xFF)
#define MASK_HB                            (0b11110000)
#define MASK_LB                            (0b00001111)
#define MASK_CPLD_MAJOR_VER                (0b11000000)
#define MASK_CPLD_MINOR_VER                (0b00111111)
#define CPLD_SYSTEM_LED_SYS_MASK           MASK_HB
#define CPLD_SYSTEM_LED_FAN_MASK           MASK_LB
#define CPLD_SYSTEM_LED_PSU_0_MASK         MASK_LB
#define CPLD_SYSTEM_LED_PSU_1_MASK         MASK_HB
#define CPLD_SYSTEM_LED_SYNC_MASK          MASK_LB
#define CPLD_SYSTEM_LED_ID_MASK            MASK_LB
#define CPLD_SFP_LED_MASK_0                (0b00000011)
#define CPLD_SFP_LED_MASK_1                (0b00001100)
#define PERM_R                             (0b00000001)
#define PERM_W                             (0b00000010)
#define PERM_RW                            (PERM_R | PERM_W)
#define IS_PERM_R(perm)                    (perm & PERM_R ? 1u : 0u)
#define IS_PERM_W(perm)                    (perm & PERM_W ? 1u : 0u)

/* common manipulation */
#define INVALID(i, min, max)    ((i < min) || (i > max) ? 1u : 0u)

#endif
