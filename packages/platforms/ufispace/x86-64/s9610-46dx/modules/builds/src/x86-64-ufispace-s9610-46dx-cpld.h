/* header file for i2c cpld driver of ufispace_s9610_46dx
 *
 * Copyright (C) 2017 UfiSpace Technology Corporation.
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

#ifndef UFISPACE_S9610_46DX_CPLD_H
#define UFISPACE_S9610_46DX_CPLD_H

/* CPLD device index value */
enum cpld_id {
    cpld1,
    cpld2,
    cpld3,
    cpld4,
};

/* CPLD registers */
#define CPLD_BOARD_ID_0_REG               0x00
#define CPLD_BOARD_ID_1_REG               0x01
#define CPLD_VERSION_REG                  0x02
#define CPLD_ID_REG                       0x03
#define CPLD_BUILD_REG                    0x04
#define CPLD_MAC_INTR_REG                 0x10
#define CPLD_CPLDX_INTR_REG               0x14
#define CPLD_NTM_INTR_REG                 0x15
#define CPLD_THERMAL_INTR_BASE_REG        0x16
#define CPLD_MISC_INTR_REG                0x1B
#define CPLD_CPU_INTR_REG                 0x1C
#define CPLD_MAC_MASK_REG                 0x20
#define CPLD_PHY_MASK_REG                 0x23
#define CPLD_CPLDX_MASK_REG               0x24
#define CPLD_NTM_MASK_REG                 0x25
#define CPLD_THERMAL_MASK_BASE_REG        0x26
#define CPLD_MISC_MASK_REG                0x2B
#define CPLD_CPU_MASK_REG                 0x2C
#define CPLD_MAC_EVT_REG                  0x30
#define CPLD_CPLDX_EVT_REG                0x34
#define CPLD_NTM_EVT_REG                  0x35
#define CPLD_THERMAL_EVT_BASE_REG         0x36
#define CPLD_MISC_EVT_REG                 0x3B
#define CPLD_EVT_CTRL_REG                 0x3F
#define CPLD_MAC_RESET_REG                0x40
#define CPLD_BMC_RESET_REG                0x43
#define CPLD_USB_RESET_REG                0x44
#define CPLD_CPLDX_RESET_REG              0x45
#define CPLD_I2C_MUX_QSFPDD_RESET_REG     0x46
#define CPLD_MISC_RESET_REG               0x48
#define CPLD_PUSH_BTN_RESET_REG           0x4C
#define CPLD_PSU_STATUS_REG               0x51
#define CPLD_SYSTEM_PWR_REG               0x52
#define CPLD_MAC_SYNCE_REG                0x53
#define CPLD_MAC_ROV_REG                  0x54
#define CPLD_MUX_CTRL_REG                 0x5C
#define CPLD_TIMING_CTRL_REG              0x5E
#define CPLD_QSPI_SEL_REG                 0x5F
#define CPLD_SYSTEM_LED_SYS_FAN_REG       0x80
#define CPLD_SYSTEM_LED_PSU_REG           0x81
#define CPLD_SYSTEM_LED_SYNC_ID_REG       0x82
#define CPLD_HBM_PWR_CTRL_REG             0x95
#define CPLD_HBM_PWR_FUNC_REG             0x9F

#define DBG_CPLD_MAC_INTR_REG             0xE0
#define DBG_CPLD_CPLDX_INTR_REG           0xE4
#define DBG_CPLD_THERMAL_INTR_BASE_REG    0xE6
#define DBG_CPLD_MISC_INTR_REG            0xEB

#define CPLD_BIT_REG                      0xFF

//CPLD 2-3
//interrupt status
#define CPLD_QSFP_INTR_PORT_BASE_REG      0x10
#define CPLD_QSFP_INTR_PRESENT_BASE_REG   0x14
#define CPLD_QSFP_INTR_FUSE_BASE_REG      0x18

//interrupt mask
#define CPLD_QSFP_MASK_PORT_BASE_REG      0x20
#define CPLD_QSFP_MASK_PRESENT_BASE_REG   0x24
#define CPLD_QSFP_MASK_FUSE_BASE_REG      0x28

//interrupt event
#define CPLD_QSFP_EVT_PORT_BASE_REG       0x30
#define CPLD_QSFP_EVT_PRESENT_BASE_REG    0x34
#define CPLD_QSFP_EVT_FUSE_BASE_REG       0x38

//reset
#define CPLD_QSFP_RESET_BASE_REG          0x40
#define CPLD_QSFP_LPMODE_BASE_REG         0x44
//synce select
#define CPLD_QSFP_SYNCE_SELECT_REG        0x50

//QSFP SFPDD LED
#define CPLD_QSFP_LED_BASE_REG            0x80
#define CPLD_QSFPDD_LED_BASE_REG          0x80
#define CPLD_SFPDD_LED_REG                0x85
#define CPLD_SFP28_LED_REG                0x82

//debug
#define DBG_CPLD_QSFP_INTR_PORT_BASE_REG      0xE0
#define DBG_CPLD_QSFP_INTR_PRESENT_BASE_REG   0xE4
#define DBG_CPLD_QSFP_INTR_FUSE_BASE_REG      0xE8

//CPLD 4
//interrupt status
#define CPLD_PHY_INTR_REG                   0x10
#define CPLD_GBOX_INTR_BASE_REG             0x11
#define CPLD_QSFPDD_INTR_PRESENT_REG        0x14
#define CPLD_SFPDD_INTR_PRESENT_REG         0x15
#define CPLD_SFP28_INTR_PRESENT_REG         0x16
#define CPLD_QSFPDD_INTR_PORT_REG           0x17
#define CPLD_SFPDD_28_TX_FAULT_REG          0x18
#define CPLD_SFPDD_28_RX_LOS_REG            0x19
#define CPLD_QSFPDD_INTR_FUSE_REG           0x1A

//interrupt mask
#define CPLD_PHY_MASK_INTR_REG              0x20
#define CPLD_GBOX_MASK_INTR_BASE_REG        0x21
#define CPLD_QSFPDD_MASK_PRESENT_REG        0x24
#define CPLD_SFPDD_MASK_PRESENT_REG         0x25
#define CPLD_SFP28_MASK_PRESENT_REG         0x26
#define CPLD_QSFPDD_MASK_PORT_REG           0x27
#define CPLD_SFPDD_28_MASK_TX_FAULT_REG     0x28
#define CPLD_SFPDD_28_MASK_RX_LOS_REG       0x29
#define CPLD_QSFPDD_MASK_FUSE_REG           0x2A

//interrupt event
#define CPLD_PHY_EVT_REG                    0x30
#define CPLD_GBOX_EVT_BASE_REG              0x31
#define CPLD_QSFPDD_EVT_PRESENT_REG         0x34
#define CPLD_SFPDD_EVT_PRESENT_REG          0x35
#define CPLD_SFP28_EVT_PRESENT_REG          0x36
#define CPLD_QSFPDD_EVT_PORT_REG            0x37
#define CPLD_SFPDD_28_EVT_TX_FAULT_REG      0x38
#define CPLD_SFPDD_28_EVT_RX_LOS_REG        0x39
#define CPLD_QSFPDD_EVT_FUSE_REG            0x3A

//reset
#define CPLD_PHY_RESET_REG                  0x40
#define CPLD_GBOX_RESET_BASE_REG            0x41
#define CPLD_QSFPDD_RESET_REG               0x43
#define CPLD_UFM_WRITE_REG                  0x44
#define CPLD_QSFPDD_LPMODE_REG              0x56

//system status
#define CPLD_BRD_PRESENT_REG                 0x50
#define CPLD_GBOX_SERBOOT_BASE_REG           0x51
#define CPLD_SFPDD_28_TX_DIS_REG             0x53
#define CPLD_SFPDD_RATE_CAP_REG              0x54
#define CPLD_SFP28_RATE_CAP_REG              0x55
#define CPLD_GBOX_SERBOOT_UFM_STORE_BASE_REG 0x61
#define CPLD_GBOX_SERBOOT_UFM_WRITE_BASE_REG 0x63

//debug
#define DBG_CPLD_PHY_INTR_REG               0xE0
#define DBG_CPLD_GBOX_INTR_BASE_REG         0xE1
#define DBG_CPLD_QSFPDD_INTR_PRESENT_REG    0xE4
#define DBG_CPLD_SFPDD_INTR_PRESENT_REG     0xE5
#define DBG_CPLD_SFP28_INTR_PRESENT_REG     0xE6
#define DBG_CPLD_QSFPDD_INTR_PORT_REG       0xE7
#define DBG_CPLD_SFPDD_28_EVT_TX_FAULT_REG  0xE8
#define DBG_CPLD_SFPDD_28_EVT_RX_LOS_REG    0xE9
#define DBG_CPLD_QSFPDD_INTR_FUSE_REG       0xEA

//CPLD2 only
#define CPLD_OP2_INTR_REG                   0x1C
#define CPLD_OP2_MASK_REG                   0x2C
#define CPLD_OP2_EVT_REG                    0x3C
#define CPLD_OP2_PWR_REG                    0x90
#define CPLD_MISC_PWR_REG                   0x91
#define DBG_CPLD_OP2_INTR_REG               0x1C
//CPLD2/3
#define CPLD_SFP_STATUS_REG                 0x1D
#define CPLD_SFP_MASK_REG                   0x2D
#define CPLD_SFP_EVT_REG                    0x3D
#define CPLD_OP2_RESET_REG                  0x48
#define CPLD_SFP_CONFIG_REG                 0x55
#define DBG_CPLD_SFP_STATUS_REG             0xED

//MASK
#define MASK_ALL                           (0xFF)
#define MASK_HB                            (0b11110000)
#define MASK_LB                            (0b00001111)
#define MASK_CPLD_MAJOR_VER                (0b11000000)
#define MASK_CPLD_MINOR_VER                (0b00111111)
#define CPLD_SYSTEM_LED_SYS_MASK           MASK_LB
#define CPLD_SYSTEM_LED_FAN_MASK           MASK_HB
#define CPLD_SYSTEM_LED_PSU_0_MASK         MASK_LB
#define CPLD_SYSTEM_LED_PSU_1_MASK         MASK_HB
#define CPLD_SYSTEM_LED_SYNC_MASK          MASK_LB
#define CPLD_SYSTEM_LED_ID_MASK            MASK_HB
#define CPLD_SFP28_LED_0_MASK              (0b00000011)
#define CPLD_SFP28_LED_1_MASK              (0b00001100)
#define CPLD_SFP28_LED_2_MASK              (0b00110000)
#define CPLD_SFP28_LED_3_MASK              (0b11000000)
#define CPLD_HBM_PWR_CTRL_MASK             (0b00000001)
#define CPLD_HBM_PWR_FUNC_MASK             (0b00000001)


/* common manipulation */
#define INVALID(i, min, max)    ((i < min) || (i > max) ? 1u : 0u)

#endif