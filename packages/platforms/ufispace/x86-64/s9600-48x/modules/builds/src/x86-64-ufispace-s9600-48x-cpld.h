/* header file for i2c cpld driver of ufispace_s9600_48x
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

#ifndef UFISPACE_S9600_48X_CPLD_H
#define UFISPACE_S9600_48X_CPLD_H

/* CPLD device index value */
enum cpld_id {
    cpld1,
    cpld2,
    cpld3,
    cpld4
};

/* CPLD registers */
#define CPLD_BOARD_ID_0_REG               0x00
#define CPLD_BOARD_ID_1_REG               0x01
#define CPLD_VERSION_REG                  0x02
#define CPLD_ID_REG                       0x03
#define CPLD_INTR_MASK_BASE_REG           0x10
#define CPLD_QSFP_STATUS_BASE_REG         0x20
#define CPLD_GBOX_INTR_BASE_REG           0x2C
#define CPLD_SFP_STATUS_REG               0x2F
#define CPLD_QSFP_CONFIG_BASE_REG         0x30
#define CPLD_SFP_CONFIG_REG               0x3F
#define CPLD_INTR_0_REG                   0x40
#define CPLD_INTR_1_REG                   0x41
#define CPLD_PSU_STATUS_REG               0x44
#define CPLD_MUX_CTRL_REG                 0x45
#define CPLD_RESET_0_REG                  0x4A
#define CPLD_RESET_1_REG                  0x4D
#define CPLD_RESET_2_REG                  0x4E
#define CPLD_RESET_3_REG                  0x4F
#define CPLD_SYSTEM_LED_BASE_REG          0x50
#define CPLD_QSFP_PORT_EVT_REG            0x70
#define CPLD_QSFP_PLUG_EVT_BASE_REG       0x71

#define CPLD_QSFP_MUX_RESET_REG           0x4A

/* common manipulation */
#define INVALID(i, min, max)    ((i < min) || (i > max) ? 1u : 0u)

#endif
