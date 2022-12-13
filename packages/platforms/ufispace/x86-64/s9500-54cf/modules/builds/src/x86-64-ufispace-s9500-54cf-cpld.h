/* header file for i2c cpld driver of ufispace_s9500_54cf
 *
 * Copyright (C) 2022 UfiSpace Technology Corporation.
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

#ifndef UFISPACE_S9500_54CF_CPLD_H
#define UFISPACE_S9500_54CF_CPLD_H

/* CPLD device index value */
enum cpld_id {
    cpld1,
    cpld2,
    cpld3
};

/* CPLD common registers */
#define CPLD_VERSION_REG                  0x02
#define CPLD_ID_REG                       0x03
#define CPLD_BUILD_REG                    0x04
#define CPLD_CHIP_REG                     0x05

/* CPLD 1 registers */
#define CPLD_BOARD_ID_0_REG               0x00
#define CPLD_BOARD_ID_1_REG               0x01

#define CPLD_NTM_INTR_REG                 0x10
#define CPLD_MAC_INTR_REG                 0x11
#define CPLD_PSU_INTR_REG                 0x12
#define CPLD_THERMAL_SENSOR_INTR_REG      0x13
#define CPLD_CPLDX_INTR_REG               0x14
#define CPLD_FAN_INTR_REG                 0x15
#define CPLD_MAC_THERMAL_INTR_REG         0x16
#define CPLD_MISC_INTR_REG                0x1B
#define CPLD_SYSTEM_INTR_REG              0x1D

#define CPLD_NTM_MASK_REG                 0x20
#define CPLD_MAC_MASK_REG                 0x21
#define CPLD_PSU_MASK_REG                 0x22
#define CPLD_THERMAL_SENSOR_MASK_REG      0x23
#define CPLD_CPLDX_MASK_REG               0x24
#define CPLD_FAN_MASK_REG                 0x25
#define CPLD_MISC_MASK_REG                0x2B
#define CPLD_NTM_EVT_REG                  0x30
#define CPLD_MAC_EVT_REG                  0x31
#define CPLD_PSU_EVT_REG                  0x32
#define CPLD_THERMAL_SENSOR_EVT_REG       0x33
#define CPLD_CPLDX_EVT_REG                0x34
#define CPLD_FAN_EVT_REG                  0x35

#define CPLD_NTM_RESET_REG                0x40
#define CPLD_BMC_RESET_REG                0x41
#define CPLD_MAC_RESET_REG                0x42
#define CPLD_I2C_MUX_RESET_1_REG          0x43
#define CPLD_I2C_MUX_RESET_2_REG          0x44
#define CPLD_CPLDX_RESET_REG              0x45

#define CPLD_I2C_WATCH_DOG_REG            0x46
#define CPLD_SFP_I2C_MONITOR_REG          0x47
#define CPLD_MISC_RESET_REG               0x48
//#define CPLD_PUSH_BTN_RESET_REG           0x4C

#define CPLD_BRD_PRESENT_REG              0x57
#define CPLD_FAN_PRESENT_REG              0x55
#define CPLD_PSU_STATUS_REG               0x59
#define CPLD_SYSTEM_PWR_REG               0x92
#define CPLD_MAC_ROV_REG                  0x52
#define CPLD_GNSS_STATUS_REG              0x5A
#define CPLD_MUX_CTRL_REG                 0x63
#define CPLD_TIMING_CTRL_1_REG            0x5D
#define CPLD_TIMING_CTRL_2_REG            0x5E

#define CPLD_SYSTEM_LED_SYS_GNSS_REG      0x81
#define CPLD_SYSTEM_LED_ID_SYNC_REG       0x82
#define CPLD_SYSTEM_LED_FAN_REG           0x83
#define CPLD_SYSTEM_LED_PSU_REG           0x84

#define DBG_CPLD_NTM_INTR_REG             0xE0
#define DBG_CPLD_MAC_INTR_REG             0xE1
#define DBG_CPLD_PSU_INTR_REG             0xE2
#define DBG_CPLD_THERMAL_SENSOR_INTR_REG  0xE3
#define DBG_CPLD_CPLDX_INTR_REG           0xE4
#define DBG_CPLD_FAN_INTR_REG             0xE5
#define DBG_CPLD_MAC_THERMAL_INTR_REG     0xE6
#define DBG_CPLD_MISC_INTR_REG            0xEB

/* CPLD 2-3 registers */

//SFP status (TX Fault, RX Loss and Present)
#define CPLD_SFP_TX_FAULT_0_7_REG       0x14
#define CPLD_SFP_TX_FAULT_8_15_REG      0x15
#define CPLD_SFP_TX_FAULT_16_23_REG     0x16
#define CPLD_SFP_TX_FAULT_24_31_REG     0x14
#define CPLD_SFP_TX_FAULT_32_39_REG     0x15
#define CPLD_SFP_TX_FAULT_40_47_REG     0x16
#define CPLD_SFP_TX_FAULT_48_53_REG     0x17
#define CPLD_SFP_RX_LOS_0_7_REG         0x18
#define CPLD_SFP_RX_LOS_8_15_REG        0x19
#define CPLD_SFP_RX_LOS_16_23_REG       0x1A
#define CPLD_SFP_RX_LOS_24_31_REG       0x18
#define CPLD_SFP_RX_LOS_32_39_REG       0x19
#define CPLD_SFP_RX_LOS_40_47_REG       0x1A
#define CPLD_SFP_RX_LOS_48_53_REG       0x1B
#define CPLD_SFP_PRESENT_0_7_REG        0x10
#define CPLD_SFP_PRESENT_8_15_REG       0x11
#define CPLD_SFP_PRESENT_16_23_REG      0x12
#define CPLD_SFP_PRESENT_24_31_REG      0x10
#define CPLD_SFP_PRESENT_32_39_REG      0x11
#define CPLD_SFP_PRESENT_40_47_REG      0x12
#define CPLD_SFP_PRESENT_48_53_REG      0x13

//SFP Status Mask
#define CPLD_SFP_MASK_TX_FAULT_0_7_REG       0x24
#define CPLD_SFP_MASK_TX_FAULT_8_15_REG      0x25
#define CPLD_SFP_MASK_TX_FAULT_16_23_REG     0x26
#define CPLD_SFP_MASK_TX_FAULT_24_31_REG     0x24
#define CPLD_SFP_MASK_TX_FAULT_32_39_REG     0x25
#define CPLD_SFP_MASK_TX_FAULT_40_47_REG     0x26
#define CPLD_SFP_MASK_TX_FAULT_48_53_REG     0x27
#define CPLD_SFP_MASK_RX_LOS_0_7_REG         0x28
#define CPLD_SFP_MASK_RX_LOS_8_15_REG        0x29
#define CPLD_SFP_MASK_RX_LOS_16_23_REG       0x2A
#define CPLD_SFP_MASK_RX_LOS_24_31_REG       0x28
#define CPLD_SFP_MASK_RX_LOS_32_39_REG       0x29
#define CPLD_SFP_MASK_RX_LOS_40_47_REG       0x2A
#define CPLD_SFP_MASK_RX_LOS_48_53_REG       0x2B
#define CPLD_SFP_MASK_PRESENT_0_7_REG        0x20
#define CPLD_SFP_MASK_PRESENT_8_15_REG       0x21
#define CPLD_SFP_MASK_PRESENT_16_23_REG      0x22
#define CPLD_SFP_MASK_PRESENT_24_31_REG      0x20
#define CPLD_SFP_MASK_PRESENT_32_39_REG      0x21
#define CPLD_SFP_MASK_PRESENT_40_47_REG      0x22
#define CPLD_SFP_MASK_PRESENT_48_53_REG      0x23

//SFP status event
#define CPLD_SFP_EVT_TX_FAULT_0_7_REG        0x34
#define CPLD_SFP_EVT_TX_FAULT_8_15_REG       0x35
#define CPLD_SFP_EVT_TX_FAULT_16_23_REG      0x36
#define CPLD_SFP_EVT_TX_FAULT_24_31_REG      0x34
#define CPLD_SFP_EVT_TX_FAULT_32_39_REG      0x35
#define CPLD_SFP_EVT_TX_FAULT_40_47_REG      0x36
#define CPLD_SFP_EVT_TX_FAULT_48_53_REG      0x37
#define CPLD_SFP_EVT_RX_LOS_0_7_REG          0x38
#define CPLD_SFP_EVT_RX_LOS_8_15_REG         0x39
#define CPLD_SFP_EVT_RX_LOS_16_23_REG        0x3A
#define CPLD_SFP_EVT_RX_LOS_24_31_REG        0x38
#define CPLD_SFP_EVT_RX_LOS_32_39_REG        0x39
#define CPLD_SFP_EVT_RX_LOS_40_47_REG        0x3A
#define CPLD_SFP_EVT_RX_LOS_48_53_REG        0x3B
#define CPLD_SFP_EVT_PRESENT_0_7_REG         0x30
#define CPLD_SFP_EVT_PRESENT_8_15_REG        0x31
#define CPLD_SFP_EVT_PRESENT_16_23_REG       0x32
#define CPLD_SFP_EVT_PRESENT_24_31_REG       0x30
#define CPLD_SFP_EVT_PRESENT_32_39_REG       0x31
#define CPLD_SFP_EVT_PRESENT_40_47_REG       0x32
#define CPLD_SFP_EVT_PRESENT_48_53_REG       0x33

//SFP TX Disable
#define CPLD_SFP_TX_DISABLE_0_7_REG          0x40
#define CPLD_SFP_TX_DISABLE_8_15_REG         0x41
#define CPLD_SFP_TX_DISABLE_16_23_REG        0x42
#define CPLD_SFP_TX_DISABLE_24_31_REG        0x40
#define CPLD_SFP_TX_DISABLE_32_39_REG        0x41
#define CPLD_SFP_TX_DISABLE_40_47_REG        0x42
#define CPLD_SFP_TX_DISABLE_48_53_REG        0x43

//SFP Port LED
#define CPLD_SFP_LED_0_3_REG          0x50
#define CPLD_SFP_LED_4_7_REG          0x51
#define CPLD_SFP_LED_8_11_REG         0x52
#define CPLD_SFP_LED_12_15_REG        0x53
#define CPLD_SFP_LED_16_19_REG        0x54
#define CPLD_SFP_LED_20_23_REG        0x55
#define CPLD_SFP_LED_24_27_REG        0x50
#define CPLD_SFP_LED_28_31_REG        0x51
#define CPLD_SFP_LED_32_35_REG        0x52
#define CPLD_SFP_LED_36_39_REG        0x53
#define CPLD_SFP_LED_40_43_REG        0x54
#define CPLD_SFP_LED_44_47_REG        0x55
#define CPLD_SFP_LED_48_51_REG        0x56
#define CPLD_SFP_LED_52_53_REG        0x57
#define CPLD_SFP_LED_CLR_REG          0x5E

//debug
#define DBG_CPLD_SFP_TX_FAULT_0_7_REG       0xE4
#define DBG_CPLD_SFP_TX_FAULT_8_15_REG      0xE5
#define DBG_CPLD_SFP_TX_FAULT_16_23_REG     0xE6
#define DBG_CPLD_SFP_TX_FAULT_24_31_REG     0xE4
#define DBG_CPLD_SFP_TX_FAULT_32_39_REG     0xE5
#define DBG_CPLD_SFP_TX_FAULT_40_47_REG     0xE6
#define DBG_CPLD_SFP_TX_FAULT_48_53_REG     0xE7
#define DBG_CPLD_SFP_RX_LOS_0_7_REG         0xE8
#define DBG_CPLD_SFP_RX_LOS_8_15_REG        0xE9
#define DBG_CPLD_SFP_RX_LOS_16_23_REG       0xEA
#define DBG_CPLD_SFP_RX_LOS_24_31_REG       0xE8
#define DBG_CPLD_SFP_RX_LOS_32_39_REG       0xE9
#define DBG_CPLD_SFP_RX_LOS_40_47_REG       0xEA
#define DBG_CPLD_SFP_RX_LOS_48_53_REG       0xEB
#define DBG_CPLD_SFP_PRESENT_0_7_REG        0xE0
#define DBG_CPLD_SFP_PRESENT_8_15_REG       0xE1
#define DBG_CPLD_SFP_PRESENT_16_23_REG      0xE2
#define DBG_CPLD_SFP_PRESENT_24_31_REG      0xE0
#define DBG_CPLD_SFP_PRESENT_32_39_REG      0xE1
#define DBG_CPLD_SFP_PRESENT_40_47_REG      0xE2
#define DBG_CPLD_SFP_PRESENT_48_53_REG      0xE3


/* CPLD 1 registers */
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
#define CPLD_SYSTEM_LED_ID_MASK            MASK_HB
#define CPLD_SYSTEM_LED_SYNC_MASK          MASK_LB
#define CPLD_SYSTEM_LED_GNSS_MASK          MASK_LB
#define CPLD_SFP_LED_CLR_MASK                  (0b00000001)

/* common manipulation */
#define INVALID(i, min, max)    ((i < min) || (i > max) ? 1u : 0u)

#endif