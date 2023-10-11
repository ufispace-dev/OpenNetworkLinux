/* header file for i2c cpld driver of ufispace_s9601_104bc
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

#ifndef UFISPACE_S9601_104BC_CPLD_H
#define UFISPACE_S9601_104BC_CPLD_H

/* CPLD device index value */
enum cpld_id {
    cpld1,
    cpld2,
    cpld3,
    cpld4,
    cpld5,
};


#define CPLD_NONE_REG                       0x00

/* CPLD Common */
#define CPLD_VERSION_REG                    0x02
#define CPLD_ID_REG                         0x03
#define CPLD_SUB_VERSION_REG                0x04

/* CPLD 1 registers */
#define CPLD_SKU_ID_REG                     0x00
#define CPLD_HW_REV_REG                     0x01
#define CPLD_SKU_EXT_REG                    0x06
// Interrupt status
#define CPLD_MAC_OP2_INTR_REG               0x10
#define CPLD_10GPHY_INTR_REG                0x13
#define CPLD_CPLD_FRU_INTR_REG              0x14
#define CPLD_MGMT_SFP_PORT_STATUS_REG       0x18
#define CPLD_MISC_INTR_REG                  0x1B
#define CPLD_SYSTEM_INTR_REG                0x1D
// Interrupt mask
#define CPLD_MAC_OP2_INTR_MASK_REG          0x20
#define CPLD_10GPHY_INTR_MASK_REG           0x23
#define CPLD_CPLD_FRU_INTR_MASK_REG         0x24
#define CPLD_MGMT_SFP_PORT_STATUS_MASK_REG  0x28
#define CPLD_MISC_INTR_MASK_REG             0x2B
#define CPLD_SYSTEM_INTR_MASK_REG           0x2D
// Interrupt event
#define CPLD_MAC_OP2_INTR_EVENT_REG	        0x30
#define CPLD_10GPHY_INTR_EVENT_REG          0x33
#define CPLD_CPLD_FRU_INTR_EVENT_REG        0x34
#define CPLD_MGMT_SFP_PORT_STATUS_EVENT_REG 0x38
#define CPLD_MISC_INTR_EVENT_REG            0x3B
#define CPLD_SYSTEM_INTR_EVENT_REG          0x3D
// Reset ctrl
#define CPLD_MAC_OP2_RST_REG                0x40
#define CPLD_PHY_RST_REG                    0x42
#define CPLD_BMC_NTM_RST_REG                0x43
#define CPLD_USB_RST_REG                    0x44
#define CPLD_CPLD_RST_REG                   0x45
#define CPLD_MUX_RST_1_REG                  0x46
#define CPLD_MUX_RST_2_REG                  0x47
#define CPLD_MISC_RST_REG                   0x48
#define CPLD_PUSHBTN_REG                    0x4C
// Sys status
#define CPLD_CPU_MISC_REG                   0x50
#define CPLD_PSU_STATUS_REG                 0x51
#define CPLD_SYS_PW_STATUS_REG              0x52
#define CPLD_MAC_STATUS_1_REG               0x53
#define CPLD_MAC_STATUS_2_REG               0x54
#define CPLD_MAC_STATUS_3_REG               0x55
#define CPLD_MGMT_SFP_PORT_CONF_REG         0x56
#define CPLD_MISC_REG                       0x5B
// Mux ctrl
#define CPLD_MUX_CTRL_REG                   0x5C
// Led ctrl
#define CPLD_SYSTEM_LED_PSU_REG             0x80
#define CPLD_SYSTEM_LED_SYS_REG             0x81
#define CPLD_SYSTEM_LED_SYNC_REG            0x82
#define CPLD_SYSTEM_LED_FAN_REG             0x83
#define CPLD_SYSTEM_LED_ID_REG              0x84
// Power good status
#define CPLD_MAC_PG_REG                     0x90
#define CPLD_OP2_PG_REG                     0x91
#define CPLD_MISC_PG_REG                    0x92
#define CPLD_HBM_PW_EN_REG                  0x95
#define CPLD_EXT_BYTE_REG                   0xD0

/* CPLD 2 */
// Interrupt status
#define CPLD_SFP_PORT_0_7_PRES_REG          0x14
#define CPLD_SFP_PORT_8_15_PRES_REG         0x15
#define CPLD_SFP_PORT_48_55_PRES_REG        0x16
#define CPLD_SFP_PORT_56_63_PRES_REG        0x17
// Interrupt mask
#define CPLD_SFP_PORT_0_7_PRES_MASK_REG     0x24
#define CPLD_SFP_PORT_8_15_PRES_MASK_REG    0x25
#define CPLD_SFP_PORT_48_55_PRES_MASK_REG   0x26
#define CPLD_SFP_PORT_56_63_PRES_MASK_REG   0x27
// Interrupt event
#define CPLD_SFP_PORT_0_7_PLUG_EVENT_REG    0x34
#define CPLD_SFP_PORT_8_15_PLUG_EVENT_REG   0x35
#define CPLD_SFP_PORT_48_55_PLUG_EVENT_REG  0x36
#define CPLD_SFP_PORT_56_63_PLUG_EVENT_REG  0x37
// Port ctrl
#define CPLD_SFP_PORT_0_7_TX_DISABLE_REG    0x40
#define CPLD_SFP_PORT_8_15_TX_DISABLE_REG   0x41
#define CPLD_SFP_PORT_48_55_TX_DISABLE_REG  0x42
#define CPLD_SFP_PORT_56_63_TX_DISABLE_REG  0x43
// Port status
#define CPLD_SFP_PORT_0_7_RX_LOS_REG        0x48
#define CPLD_SFP_PORT_8_15_RX_LOS_REG       0x49
#define CPLD_SFP_PORT_48_55_RX_LOS_REG      0x4A
#define CPLD_SFP_PORT_56_63_RX_LOS_REG      0x4B
#define CPLD_SFP_PORT_0_7_TX_FAULT_REG      0x4C
#define CPLD_SFP_PORT_8_15_TX_FAULT_REG     0x4D
#define CPLD_SFP_PORT_48_55_TX_FAULT_REG    0x4E
#define CPLD_SFP_PORT_56_63_TX_FAULT_REG    0x4F

/* CPLD 3 */
// Interrupt status
#define CPLD_SFP_PORT_16_23_PRES_REG        0x14
#define CPLD_SFP_PORT_24_31_PRES_REG        0x15
#define CPLD_SFP_PORT_64_71_PRES_REG        0x16
#define CPLD_SFP_PORT_72_79_PRES_REG        0x17
// Interrupt mask
#define CPLD_SFP_PORT_16_23_PRES_MASK_REG   0x24
#define CPLD_SFP_PORT_24_31_PRES_MASK_REG   0x25
#define CPLD_SFP_PORT_64_71_PRES_MASK_REG   0x26
#define CPLD_SFP_PORT_72_79_PRES_MASK_REG   0x27
// Interrupt event
#define CPLD_SFP_PORT_16_23_PLUG_EVENT_REG  0x34
#define CPLD_SFP_PORT_24_31_PLUG_EVENT_REG  0x35
#define CPLD_SFP_PORT_64_71_PLUG_EVENT_REG  0x36
#define CPLD_SFP_PORT_72_79_PLUG_EVENT_REG  0x37
// Port ctrl
#define CPLD_SFP_PORT_16_23_TX_DISABLE_REG  0x40
#define CPLD_SFP_PORT_24_31_TX_DISABLE_REG  0x41
#define CPLD_SFP_PORT_64_71_TX_DISABLE_REG  0x42
#define CPLD_SFP_PORT_72_79_TX_DISABLE_REG  0x43
// Port status
#define CPLD_SFP_PORT_16_23_RX_LOS_REG      0x48
#define CPLD_SFP_PORT_24_31_RX_LOS_REG      0x49
#define CPLD_SFP_PORT_64_71_RX_LOS_REG      0x4A
#define CPLD_SFP_PORT_72_79_RX_LOS_REG      0x4B
#define CPLD_SFP_PORT_16_23_TX_FAULT_REG    0x4C
#define CPLD_SFP_PORT_24_31_TX_FAULT_REG    0x4D
#define CPLD_SFP_PORT_64_71_TX_FAULT_REG    0x4E
#define CPLD_SFP_PORT_72_79_TX_FAULT_REG    0x4F

/* CPLD 4 */
// Interrupt status
#define CPLD_SFP_PORT_32_39_PRES_REG        0x14
#define CPLD_SFP_PORT_40_43_PRES_REG        0x15
#define CPLD_SFP_PORT_80_87_PRES_REG        0x16
#define CPLD_SFP_PORT_88_91_PRES_REG        0x17
// Interrupt mask
#define CPLD_SFP_PORT_32_39_PRES_MASK_REG   0x24
#define CPLD_SFP_PORT_40_43_PRES_MASK_REG   0x25
#define CPLD_SFP_PORT_80_87_PRES_MASK_REG   0x26
#define CPLD_SFP_PORT_88_91_PRES_MASK_REG   0x27
// Interrupt event
#define CPLD_SFP_PORT_32_39_PLUG_EVENT_REG  0x34
#define CPLD_SFP_PORT_40_43_PLUG_EVENT_REG  0x35
#define CPLD_SFP_PORT_80_87_PLUG_EVENT_REG  0x36
#define CPLD_SFP_PORT_88_91_PLUG_EVENT_REG  0x37
// Port ctrl
#define CPLD_SFP_PORT_32_39_TX_DISABLE_REG  0x40
#define CPLD_SFP_PORT_40_43_TX_DISABLE_REG  0x41
#define CPLD_SFP_PORT_80_87_TX_DISABLE_REG  0x42
#define CPLD_SFP_PORT_88_91_TX_DISABLE_REG  0x43
// Port status
#define CPLD_SFP_PORT_32_39_RX_LOS_REG      0x48
#define CPLD_SFP_PORT_40_43_RX_LOS_REG      0x49
#define CPLD_SFP_PORT_80_87_RX_LOS_REG      0x4A
#define CPLD_SFP_PORT_88_91_RX_LOS_REG      0x4B
#define CPLD_SFP_PORT_32_39_TX_FAULT_REG    0x4C
#define CPLD_SFP_PORT_40_43_TX_FAULT_REG    0x4D
#define CPLD_SFP_PORT_80_87_TX_FAULT_REG    0x4E
#define CPLD_SFP_PORT_88_91_TX_FAULT_REG    0x4F

/* CPLD 5 */
// Interrupt status
#define CPLD_QSFP_PORT_96_103_INTR_REG       0x10
#define CPLD_SFP_PORT_44_47_PRES_REG         0x14
#define CPLD_SFP_PORT_92_95_PRES_REG         0x15
#define CPLD_QSFP_PORT_96_103_PRES_REG       0x16
// Interrupt mask
#define CPLD_QSFP_PORT_96_103_INTR_MASK_REG  0x20
#define CPLD_SFP_PORT_44_47_PRES_MASK_REG    0x24
#define CPLD_SFP_PORT_92_95_PRES_MASK_REG    0x25
#define CPLD_QSFP_PORT_96_103_PRES_MASK_REG  0x26
// Interrupt event
#define CPLD_QSFP_PORT_96_103_INTR_EVENT_REG 0x30
#define CPLD_SFP_PORT_44_47_PLUG_EVENT_REG   0x34
#define CPLD_SFP_PORT_92_95_PLUG_EVENT_REG   0x35
#define CPLD_QSFP_PORT_96_103_PLUG_EVENT_REG 0x36
// Port ctrl
#define CPLD_SFP_PORT_44_47_TX_DISABLE_REG   0x40
#define CPLD_SFP_PORT_92_95_TX_DISABLE_REG   0x41
#define CPLD_QSFP_PORT_96_103_RST_REG        0x42
#define CPLD_QSFP_PORT_96_103_MODSEL_REG     0x43
#define CPLD_QSFP_PORT_96_103_LPMODE_REG     0x46
// Port status
#define CPLD_SFP_PORT_44_47_RX_LOS_REG       0x48
#define CPLD_SFP_PORT_92_95_RX_LOS_REG       0x49
#define CPLD_SFP_PORT_44_47_TX_FAULT_REG     0x4C
#define CPLD_SFP_PORT_92_95_TX_FAULT_REG     0x4D

//MASK
#define MASK_ALL             (0xFF)
#define MASK_NONE            (0x00)
#define MASK_0000_0011       (0x03)
#define MASK_0000_0100       (0x04)
#define MASK_0000_0111       (0x07)
#define MASK_0001_1000       (0x18)
#define MASK_0011_1000       (0x38)
#define MASK_0011_1111       (0x3F)
#define MASK_1000_0000       (0x80)
#define MASK_1100_0000       (0xC0)

/* common manipulation */
#define INVALID(i, min, max)    ((i < min) || (i > max) ? 1u : 0u)

#endif
