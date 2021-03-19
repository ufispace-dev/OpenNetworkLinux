/* header file for i2c cpld driver of ufispace_s9710_76d
 *
 * Copyright (C) 2017 UfiSpace Technology Corporation.
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

#ifndef UFISPACE_S9710_76D_CPLD_H
#define UFISPACE_S9710_76D_CPLD_H

/* CPLD device index value */
enum cpld_id {
    cpld1,
    cpld2,
    cpld3,
    cpld4,
    cpld5,
};

/* CPLD registers */
#define CPLD_BOARD_ID_0_REG               0x00
#define CPLD_BOARD_ID_1_REG               0x01
#define CPLD_VERSION_REG                  0x02
#define CPLD_ID_REG                       0x03
#define CPLD_MAC_INTR_REG                 0x10
#define CPLD_PHY_INTR_REG                 0x13
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
#define CPLD_MAC_EVT_REG                  0x30
#define CPLD_PHY_EVT_REG                  0x33
#define CPLD_CPLDX_EVT_REG                0x34
#define CPLD_NTM_EVT_REG                  0x35
#define CPLD_THERMAL_EVT_BASE_REG         0x36
#define CPLD_MISC_EVT_REG                 0x3B
#define CPLD_MAC_RESET_REG                0x40
#define CPLD_PHY_RESET_REG                0x42
#define CPLD_BMC_RESET_REG                0x43
#define CPLD_USB_RESET_REG                0x44
#define CPLD_CPLDX_RESET_REG              0x45
#define CPLD_I2C_MUX_NIF_RESET_REG        0x46
#define CPLD_MISC_RESET_REG               0x48
#define CPLD_PUSH_BTN_RESET_REG           0x4C
#define CPLD_BRD_PRESENT_REG              0x50
#define CPLD_PSU_STATUS_REG               0x51
#define CPLD_SYSTEM_PWR_REG               0x52
#define CPLD_MAC_SYNCE_REG                0x53
#define CPLD_MAC_ROV_REG                  0x54
#define CPLD_MUX_CTRL_REG                 0x5C
#define CPLD_TIMING_CTRL_REG              0x5E
#define CPLD_QSPI_SEL_REG                 0x5F
#define CPLD_SYSTEM_LED_BASE_REG          0x80
#define CPLD_LED_CLEAR_REG                0x83

//CPLD 2-5

//interrupt status
#define CPLD_QSFPDD_INTR_PORT_BASE_REG      0x10
#define CPLD_QSFPDD_INTR_PRESENT_BASE_REG   0x14
#define CPLD_QSFPDD_INTR_FUSE_BASE_REG      0x18

//interrupt mask
#define CPLD_QSFPDD_MASK_PORT_BASE_REG      0x20
#define CPLD_QSFPDD_MASK_PRESENT_BASE_REG   0x24
#define CPLD_QSFPDD_MASK_FUSE_BASE_REG      0x28

//interrupt event
#define CPLD_QSFPDD_EVT_PORT_BASE_REG       0x30
#define CPLD_QSFPDD_EVT_PRESENT_BASE_REG    0x34
#define CPLD_QSFPDD_EVT_FUSE_BASE_REG       0x38

//reset
#define CPLD_QSFPDD_RESET_BASE_REG          0x40
#define CPLD_QSFPDD_LPMODE_BASE_REG         0x44

//CPLD2 only
#define CPLD_OP2_INTR_REG                   0x1C
#define CPLD_OP2_MASK_REG                   0x2C
#define CPLD_OP2_EVT_REG                    0x3C
#define CPLD_OP2_PWR_REG                    0x90
#define CPLD_MISC_PWR_REG                   0x91
//CPLD2/3
#define CPLD_SFP_STATUS_REG                 0x1D
#define CPLD_SFP_MASK_REG                   0x2D
#define CPLD_SFP_EVT_REG                    0x3D
#define CPLD_OP2_RESET_REG                  0x48
#define CPLD_SFP_CONFIG_REG                 0x55
//CPLD4 only
#define CPLD_I2C_MUX_FAB_RESET_REG          0x49
//CPLD4/5
#define CPLD_QSFPDD_FAB_LED_BASE_REG        0x80

/* common manipulation */
#define INVALID(i, min, max)    ((i < min) || (i > max) ? 1u : 0u)

#endif
