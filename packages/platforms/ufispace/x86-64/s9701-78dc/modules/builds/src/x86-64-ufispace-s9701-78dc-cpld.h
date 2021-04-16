/* header file for i2c cpld driver of ufispace_s9701
 *
 * Copyright (C) 2017 UfiSpace Technology Corporation.
 * Leo Lin <leo.yt.lin@ufispace.com>
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

#ifndef UFISPACE_S9701_I2C_CPLD_H
#define UFISPACE_S9701_I2C_CPLD_H

/* CPLD device index value */
enum cpld_id {
    cpld1,
    cpld2,
    cpld3,
    cpld4,
};

/* QSFPDD port number */
#define QSFPDD_MAX_PORT_NUM   5
#define QSFPDD_MIN_PORT_NUM   0

/* QSFP port number */
#define QSFP_MAX_PORT_NUM   71
#define QSFP_MIN_PORT_NUM   64

/* SFP+ port number */
#define SFP_MAX_PORT_NUM    63
#define SFP_MIN_PORT_NUM    0

/* CPLD registers */
/* CPLD 1 */
#define CPLD_SKU_ID_REG                     0x00
#define CPLD_HW_REV_REG                     0x01
#define CPLD_VERSION_REG                    0x02
#define CPLD_ID_REG                         0x03
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
#define CPLD_BMC_NTM_RST_REG                0x43
#define CPLD_USB_RST_REG                    0x44
#define CPLD_CPLD_RST_REG                   0x45
#define CPLD_MUX_RST_1_REG                  0x46
#define CPLD_MUX_RST_2_REG                  0x47
#define CPLD_MISC_RST_REG                   0x48
#define CPLD_PUSHBTN_REG                    0x4C
// Sys status
#define CPLD_DAU_BD_PRES_REG                0x50
#define CPLD_PSU_STATUS_REG                 0x51
#define CPLD_SYS_PW_STATUS_REG              0x52
#define CPLD_MAC_STATUS_1_REG               0x53
#define CPLD_MAC_STATUS_2_REG               0x54
#define CPLD_MAC_STATUS_3_REG               0x55
#define CPLD_MGMT_SFP_PORT_CONF_REG              0x56
#define CPLD_MISC_REG                       0x56
// Mux ctrl
#define CPLD_MUX_CTRL_REG                   0x5C
// Led ctrl
#define CPLD_SYS_LED_CTRL_1_REG             0x80
#define CPLD_SYS_LED_CTRL_2_REG             0x81
#define CPLD_OOB_LED_CTRL_REG               0x82
// Power good status
#define CPLD_MAC_PG_REG                     0x90
#define CPLD_OP2_PG_REG                     0x91
#define CPLD_MISC_PG_REG                    0x92
#define CPLD_HBM_PW_EN_REG                  0x95

/* CPLD 2 */
// Interrupt status
#define CPLD_SFP_PORT_0_7_PRES_REG          0x14
#define CPLD_SFP_PORT_8_15_PRES_REG         0x15
#define CPLD_SFP_PORT_32_39_PRES_REG        0x16
#define CPLD_SFP_PORT_40_47_PRES_REG        0x17
// Interrupt mask
#define CPLD_SFP_PORT_0_7_PRES_MASK_REG     0x24
#define CPLD_SFP_PORT_8_15_PRES_MASK_REG    0x25
#define CPLD_SFP_PORT_32_39_PRES_MASK_REG   0x26
#define CPLD_SFP_PORT_40_47_PRES_MASK_REG   0x27
// Interrupt event
#define CPLD_SFP_PORT_0_7_PLUG_EVENT_REG    0x34
#define CPLD_SFP_PORT_8_15_PLUG_EVENT_REG   0x35
#define CPLD_SFP_PORT_32_39_PLUG_EVENT_REG  0x36
#define CPLD_SFP_PORT_40_47_PLUG_EVENT_REG  0x37
// Port ctrl
#define CPLD_SFP_PORT_0_7_TX_DISABLE_REG    0x40
#define CPLD_SFP_PORT_8_15_TX_DISABLE_REG   0x41
#define CPLD_SFP_PORT_32_39_TX_DISABLE_REG  0x42
#define CPLD_SFP_PORT_40_47_TX_DISABLE_REG  0x43
// Port status
#define CPLD_SFP_PORT_0_7_RX_LOS_REG        0x48
#define CPLD_SFP_PORT_8_15_RX_LOS_REG       0x49
#define CPLD_SFP_PORT_32_39_RX_LOS_REG      0x4A
#define CPLD_SFP_PORT_40_47_RX_LOS_REG      0x4B
#define CPLD_SFP_PORT_0_7_TX_FAULT_REG      0x4C
#define CPLD_SFP_PORT_8_15_TX_FAULT_REG     0x4D
#define CPLD_SFP_PORT_32_39_TX_FAULT_REG    0x4E
#define CPLD_SFP_PORT_40_47_TX_FAULT_REG    0x4F

/* CPLD 3 */
// Interrupt status
#define CPLD_SFP_PORT_16_23_PRES_REG        0x14
#define CPLD_SFP_PORT_24_31_PRES_REG        0x15
#define CPLD_SFP_PORT_48_55_PRES_REG        0x16
#define CPLD_SFP_PORT_56_63_PRES_REG        0x17
// Interrupt mask
#define CPLD_SFP_PORT_16_23_PRES_MASK_REG   0x24
#define CPLD_SFP_PORT_24_31_PRES_MASK_REG   0x25
#define CPLD_SFP_PORT_48_55_PRES_MASK_REG   0x26
#define CPLD_SFP_PORT_56_63_PRES_MASK_REG   0x27
// Interrupt event
#define CPLD_SFP_PORT_16_23_PLUG_EVENT_REG  0x34
#define CPLD_SFP_PORT_24_31_PLUG_EVENT_REG  0x35
#define CPLD_SFP_PORT_48_55_PLUG_EVENT_REG  0x36
#define CPLD_SFP_PORT_56_63_PLUG_EVENT_REG  0x37
// Port ctrl
#define CPLD_SFP_PORT_16_23_TX_DISABLE_REG  0x40
#define CPLD_SFP_PORT_24_31_TX_DISABLE_REG  0x41
#define CPLD_SFP_PORT_48_55_TX_DISABLE_REG  0x42
#define CPLD_SFP_PORT_56_63_TX_DISABLE_REG  0x43
// Port status
#define CPLD_SFP_PORT_16_23_RX_LOS_REG      0x48
#define CPLD_SFP_PORT_24_31_RX_LOS_REG      0x49
#define CPLD_SFP_PORT_48_55_RX_LOS_REG      0x4A
#define CPLD_SFP_PORT_56_63_RX_LOS_REG      0x4B
#define CPLD_SFP_PORT_16_23_TX_FAULT_REG    0x4C
#define CPLD_SFP_PORT_24_31_TX_FAULT_REG    0x4D
#define CPLD_SFP_PORT_48_55_TX_FAULT_REG    0x4E
#define CPLD_SFP_PORT_56_63_TX_FAULT_REG    0x4F

/* CPLD 4 */
// Interrupt status
#define CPLD_QSFP_PORT_64_71_INTR_REG       0x10
#define CPLD_QSFPDD_PORT_INTR_REG           0x12
#define CPLD_QSFP_PORT_64_71_PRES_REG       0x14
#define CPLD_QSFPDD_PORT_0_5_PRES_REG       0x16
// Interrupt mask
#define CPLD_QSFP_PORT_64_71_INTR_MASK_REG  0x20
#define CPLD_QSFPDD_PORT_INTR_MASK_REG      0x22
#define CPLD_QSFP_PORT_64_71_PRES_MASK_REG  0x24
#define CPLD_QSFPDD_PORT_0_5_PRES_MASK_REG  0x26
// Interrupt event
#define CPLD_QSFP_PORT_64_71_INTR_EVENT_REG 0x30
#define CPLD_QSFPDD_PORT_INTR_EVENT_REG     0x32
#define CPLD_QSFP_PORT_64_71_PLUG_EVENT_REG 0x34
#define CPLD_QSFPDD_PORT_0_5_PLUG_EVENT_REG 0x36
// Port ctrl
#define CPLD_QSFP_PORT_64_71_RST_REG        0x40
#define CPLD_QSFPDD_PORT_RST_REG            0x42
#define CPLD_QSFP_PORT_64_71_LPMODE_REG     0x46
#define CPLD_QSFPDD_PORT_LPMODE_REG         0x48
// Port led ctrl
#define CPLD_QSFPDD_PORT_0_1_LED_REG        0x8C
#define CPLD_QSFPDD_PORT_2_3_LED_REG        0x8D
#define CPLD_QSFPDD_PORT_4_5_LED_REG        0x8E
#define CPLD_BEACON_LED_REG                 0x8F

/* bit field structure for register value */
struct cpld_reg_hw_rev_t {
    u8 hw_rev:2;
    u8 deph_rev:1;
    u8 build_rev:3;
    u8 reserved:1;
    u8 id_type:1;
};

struct cpld_reg_version_t {
    u8 minor:6;
    u8 major:2;
};

struct cpld_reg_id_t {
    u8 id:3;
    u8 reserved:5;
};

/* common manipulation */
#define INVALID(i, min, max)    ((i < min) || (i > max) ? 1u : 0u)
#define READ_BIT(val, bit)      ((0u == (val & (1<<bit))) ? 0u : 1u)
#define SET_BIT(val, bit)       (val |= (1 << bit))
#define CLEAR_BIT(val, bit)     (val &= ~(1 << bit))
#define TOGGLE_BIT(val, bit)    (val ^= (1 << bit))
#define _BIT(n)                 (1<<(n))
#define _BIT_MASK(len)          (BIT(len)-1)

/* bitfield of register manipulation */
#define READ_BF(bf_struct, val, bf_name, bf_value) \
    (bf_value = ((struct bf_struct *)&val)->bf_name)
#define READ_BF_1(bf_struct, val, bf_name, bf_value) \
    bf_struct bf; \
    bf.data = val; \
    bf_value = bf.bf_name
#define HW_REV_GET(val, res) \
    READ_BF(cpld_reg_hw_rev_t, val, hw_rev, res)
#define DEPH_REV_GET(val, res) \
    READ_BF(cpld_reg_hw_rev_t, val, deph_rev, res)
#define BUILD_REV_GET(val, res) \
    READ_BF(cpld_reg_hw_rev_t, val, build_rev, res)
#define ID_TYPE_GET(val, res) \
    READ_BF(cpld_reg_hw_rev_t, val, id_type, res)
#define CPLD_MAJOR_VERSION_GET(val, res) \
    READ_BF(cpld_reg_version_t, val, major, res)
#define CPLD_MINOR_VERSION_GET(val, res) \
    READ_BF(cpld_reg_version_t, val, minor, res)
#define CPLD_ID_ID_GET(val, res) \
    READ_BF(cpld_reg_id_t, val, id, res)

/* CPLD access functions */
extern int s9701_cpld_read(u8 cpld_idx, u8 reg);
extern int s9701_cpld_write(u8 cpld_idx, u8 reg, u8 value);

#endif

