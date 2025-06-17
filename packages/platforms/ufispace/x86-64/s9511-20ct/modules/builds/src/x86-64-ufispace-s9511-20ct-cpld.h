/* header file for i2c cpld driver of ufispace_s9511_20ct
 *
 * Copyright (C) 2024 UfiSpace Technology Corporation.
 * Melo Lin <melo.lin@ufispace.com>
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

#ifndef UFISPACE_S9511_20CT_CPLD_H
#define UFISPACE_S9511_20CT_CPLD_H

/* CPLD device index value */
enum cpld_id {
    cpld1,
    cpld2
};

/* CPLD 1&2 common registers - CPLD Information */
#define VERSION_REG                          0x02
#define ID_REG                               0x03
#define BUILD_REG                            0x04
#define CHIP_REG                             0x05
#define MODULE_RESET_REG                     0xF0  /* CPLD Module Reset Register */
#define EVENT_DETECTION_REG                  0x3F  /* Event Detection Control Register */
#define NONE_REG                             0x00

/* CPLD 1 registers (MB CPLD) - Board Information */
#define BOARD_ID_1_REG                       0x00
#define BOARD_ID_2_REG                       0x01
#define REV_ID_REG                           0x06
#define BRD_EXT_ID_REG                       0xD0

/* CPLD 1 registers (MB CPLD) - Interrut Status */
#define INTR_1_REG_CLK_PTP                   0x10
#define INTR_2_REG_PSU                       0x12
#define INTR_3_REG_HWM                       0x13
#define INTR_4_REG_THERMAL                   0x14
#define INTR_5_REG_FAN                       0x16
#define INTR_6_REG_ETH                       0x17
#define INTR_7_REG_CPU_NMI                   0x19
#define INTR_8_REG_SYS                       0x1B  /* Interrupt Output Status Register */

/* CPLD 1 registers (MB CPLD) - Interrut Mask */
#define INTR_MASK_1_REG_CLK_PTP              0x20
#define INTR_MASK_2_REG_PSU                  0x22
#define INTR_MASK_3_REG_HWM                  0x23
#define INTR_MASK_4_REG_THERMAL              0x24
#define INTR_MASK_5_REG_FAN                  0x26
#define INTR_MASK_6_REG_ETH                  0x27
#define INTR_MASK_8_REG_SYS                  0x2B  /* Interrupt Output Status Mask Register */

/* CPLD 1 registers (MB CPLD) - Interrut Event */
#define INTR_EVENT_1_REG_CLK_PTP             0x30
#define INTR_EVENT_2_REG_PSU                 0x32
#define INTR_EVENT_3_REG_HWM                 0x33
#define INTR_EVENT_4_REG_THERMAL             0x34
#define INTR_EVENT_5_REG_FAN                 0x36
#define INTR_EVENT_6_REG_ETH                 0x37

/* CPLD 1 registers (MB CPLD) - Interrut Debug */
#define INTR_DEBUG_1_REG_CLK_PTP             0xE0
#define INTR_DEBUG_2_REG_PSU                 0xE2
#define INTR_DEBUG_3_REG_HWM                 0xE3
#define INTR_DEBUG_4_REG_THERMAL             0xE4
#define INTR_DEBUG_5_REG_FAN                 0xE6
#define INTR_DEBUG_6_REG_ETH                 0xE7
#define INTR_DEBUG_7_REG_CPU_NMI             0xE9
//#define INTR_DEBUG_10_REG_CLK_PTP_2          0xE1  /* Clock & PTP Interrupt 2 Debug Register */

/* CPLD 1 registers (MB CPLD) - Reset Control */
#define RESET_1_REG_CLK_PTP                  0x40
#define RESET_2_REG_SYS                      0x41
#define RESET_3_REG_ETH                      0x42
#define RESET_4_REG_I2C_MUX                  0x43

/* CPLD 1 registers (MB CPLD) - System Status */
#define SYS_STATUS_1_REG_CLK_TIMING_1        0x50
#define SYS_STATUS_2_REG_CLK_TIMING_2        0x51
#define SYS_STATUS_3_REG_ROV                 0x52
#define SYS_STATUS_4_REG_GNSS                0x53
#define SYS_STATUS_5_REG_USB                 0x54
#define SYS_STATUS_6_REG_IO_OVER_CURR        0x56
#define SYS_STATUS_7_REG_SYS_1               0x57
#define SYS_STATUS_8_REG_SYS_2               0x58
#define SYS_STATUS_9_REG_PSU                 0x59
#define SYS_STATUS_10_REG_SYS_LED_1          0x83  /* System LED Status 1 Register */

/* CPLD 1 registers (MB CPLD) - System Control */
#define SYS_CONTROL_1_REG_BOOT_SEL           0x5B
#define SYS_CONTROL_2_REG_CLK_TIMING_1       0x5C
#define SYS_CONTROL_3_REG_CLK_TIMINH_2       0x5D
#define SYS_CONTROL_4_REG_PW_SYS             0x5E
//#define SYS_CONTROL_5_REG_I210               0x60
#define SYS_CONTROL_6_REG_GNSS               0x61
#define SYS_CONTROL_7_REG_UART               0x63
#define SYS_CONTROL_8_REG_USB                0x64
//#define SYS_CONTROL_9_REG_I2C_MUX_SEL        0x65
#define SYS_CONTROL_10_REG_SYNCE             0x66
#define SYS_CONTROL_11_REG_TS_PLL_CLK        0x67

/* CPLD 1 registers (MB CPLD) - LED Control */
#define SYS_LED_CONTROL_1_REG_LED_CLEAR      0x80
#define SYS_LED_CONTROL_2_REG_SYS_LED_1      0x81
#define SYS_LED_CONTROL_3_REG_SYS_LED_2      0x82

/* CPLD 1 registers (MB CPLD) - Power Good Status */
//#define PWGOOD_REG                           0x90

/* CPLD 1 registers (MB CPLD) - CPLD Enable Debug */
#define CPLD_ENABLE_DEBUG_REG                0xF1

/* CPLD 1 registers (MB CPLD) - CPLD ROV Debug */
#define CPLD_ROV_DEBUG_REG                   0xF2

/* CPLD 1 registers (MB CPLD) - CPLD Read/ Write Test */
#define CPLD_TEST_REG                        0xFF


/* CPLD 2 registers - Interrupt (Present/ RXLOS/ TXFAULT)*/
#define PORT_INTR_1_REG                       0x10  /* SFP Port Present Register */
#define PORT_INTR_2_REG                       0x11  /* SFP Port Present Register */
#define PORT_INTR_3_REG                       0x12  /* SFP Port Present Register */
#define PORT_INTR_4_REG                       0x17  /* SFP Port RXLOS Register */
#define PORT_INTR_5_REG                       0x18  /* SFP Port RXLOS Register */
#define PORT_INTR_6_REG                       0x19  /* SFP Port RXLOS Register */
#define PORT_INTR_7_REG                       0x1A  /* SFP Port TXFAULT Register */
#define PORT_INTR_8_REG                       0x1B  /* SFP Port TXFAULT Register */
#define PORT_INTR_9_REG                       0x1C  /* SFP Port TXFAULT Register */

/* CPLD 2 registers - Interrupt Mask (Present Mask/ RXLOS Mask/ TXFAULT Mask)*/
#define PORT_INTR_MASK_1_REG                  0x20  /* SFP Port Present Mask Register */
#define PORT_INTR_MASK_2_REG                  0x21  /* SFP Port Present Mask Register */
#define PORT_INTR_MASK_3_REG                  0x22  /* SFP Port Present Mask Register */
#define PORT_INTR_MASK_4_REG                  0x27  /* SFP Port RXLOS Mask Register */
#define PORT_INTR_MASK_5_REG                  0x28  /* SFP Port RXLOS Mask Register */
#define PORT_INTR_MASK_6_REG                  0x29  /* SFP Port RXLOS Mask Register */
#define PORT_INTR_MASK_7_REG                  0x2A  /* SFP Port TXFAULT Mask Register */
#define PORT_INTR_MASK_8_REG                  0x2B  /* SFP Port TXFAULT Mask Register */
#define PORT_INTR_MASK_9_REG                  0x2C  /* SFP Port TXFAULT Mask Register */

/* CPLD 2 registers - Interrupt Mask (Present Event/ RXLOS Event/ TXFAULT Event)*/
#define PORT_INTR_EVENT_1_REG                 0x30  /* SFP Port Present Event Register */
#define PORT_INTR_EVENT_2_REG                 0x31  /* SFP Port Present Event Register */
#define PORT_INTR_EVENT_3_REG                 0x32  /* SFP Port Present Event Register */
#define PORT_INTR_EVENT_4_REG                 0x37  /* SFP Port RXLOS Event Register */
#define PORT_INTR_EVENT_5_REG                 0x38  /* SFP Port RXLOS Event Register */
#define PORT_INTR_EVENT_6_REG                 0x39  /* SFP Port RXLOS Event Register */
#define PORT_INTR_EVENT_7_REG                 0x3A  /* SFP Port TXFAULT Event Register */
#define PORT_INTR_EVENT_8_REG                 0x3B  /* SFP Port TXFAULT Event Register */
#define PORT_INTR_EVENT_9_REG                 0x3C  /* SFP Port TXFAULT Event Register */

/* CPLD 2 registers - Interrupt Debug (Present Debug/ RXLOS Debug/ TXFAULT Debug)*/
#define PORT_INTR_DEBUG_1_REG                 0xE0  /* SFP Port Present Debug Register */
#define PORT_INTR_DEBUG_2_REG                 0xE1  /* SFP Port Present Debug Register */
#define PORT_INTR_DEBUG_3_REG                 0xE2  /* SFP Port Present Debug Register */
#define PORT_INTR_DEBUG_4_REG                 0xE7  /* SFP Port RXLOS Debug Register */
#define PORT_INTR_DEBUG_5_REG                 0xE8  /* SFP Port RXLOS Debug Register */
#define PORT_INTR_DEBUG_6_REG                 0xE9  /* SFP Port RXLOS Debug Register */
#define PORT_INTR_DEBUG_7_REG                 0xEA  /* SFP Port TXFAULT Debug Register */
#define PORT_INTR_DEBUG_8_REG                 0xEB  /* SFP Port TXFAULT Debug Register */
#define PORT_INTR_DEBUG_9_REG                 0xEC  /* SFP Port TXFAULT Debug Register */

/* CPLD 2 registers - SFP Control (TX Disable/ RATE SEL/ PWR EN)*/
#define PORT_CONTROL_1_REG                    0x40  /* SFP Port TX disable Register */
#define PORT_CONTROL_2_REG                    0x41  /* SFP Port TX disable Register */
#define PORT_CONTROL_3_REG                    0x42  /* SFP Port TX disable Register */
#define PORT_CONTROL_4_REG                    0x43  /* SFP Port RATE SEL Register */
#define PORT_CONTROL_5_REG                    0x44  /* SFP Port RATE SEL Register */
#define PORT_CONTROL_6_REG                    0x45  /* SFP Port RATE SEL Register */
#define PORT_CONTROL_7_REG                    0x49  /* SFP Port PWR EN Register */
#define PORT_CONTROL_8_REG                    0x4A  /* SFP Port PWR EN Register */
#define PORT_CONTROL_9_REG                    0x4B  /* SFP Port PWR EN Register */

/* CPLD 2 registers - System Status */
//#define SYS_STATUS_1_REG                     0x59  /* PSU Status Register */
#define SYS_STATUS_2_REG                     0x70  /* FAN Status Register */
#define SYS_STATUS_3_REG                     0x71  /* FAN0 PWM RPM Hbyte Register */
#define SYS_STATUS_4_REG                     0x72  /* FAN0 PWM RPM Lbyte Register */
#define SYS_STATUS_5_REG                     0x73  /* FAN1 PWM RPM Hbyte Register */
#define SYS_STATUS_6_REG                     0x74  /* FAN1 PWM RPM Lbyte Register */
#define SYS_STATUS_7_REG                     0x75  /* FAN2 PWM RPM Hbyte Register */
#define SYS_STATUS_8_REG                     0x76  /* FAN2 PWM RPM Lbyte Register */
#define SYS_STATUS_9_REG                     0x77  /* FAN3 PWM RPM Hbyte Register */
#define SYS_STATUS_10_REG                    0x78  /* FAN3 PWM RPM Lbyte Register */

/* CPLD 2 registers - System Control */
#define SYS_CTRL_1_FAN_PWM_MODE              0x80
#define SYS_CTRL_2_FAN_PWM_DIAG_MODE         0x81
#define SYS_CTRL_3_FAN_PWM_DIAG_MODE         0x82

/* CPLD 2 registers - VMON HWM */
#define VOLTAGE_1_CH1                       0xA0
#define VOLTAGE_2_CH2                       0xA1
#define VOLTAGE_3_CH3                       0xA2
#define VOLTAGE_4_CH4                       0xA3
#define VOLTAGE_5_CH5                       0xA4
#define VOLTAGE_6_CH6                       0xA5
#define VOLTAGE_7_CH7                       0xA6
#define VOLTAGE_8_CH8                       0xA7
#define VOLTAGE_9_CH9                       0xA8
#define VOLTAGE_10_CH10                     0xA9
#define VOLTAGE_11_CH11                     0xAA
#define VOLTAGE_12_CH12                     0xAB
#define VOLTAGE_13_CH13                     0xAC
#define VOLTAGE_14_CH14                     0xAD
#define VOLTAGE_15_CH15                     0xAE
#define VOLTAGE_16_CH16                     0xAF


/* MASK Bit */
#define MASK_ALL                     (0xFF)
#define MASK_NONE                    (0x00)
#define MASK_HB                      (0b11110000)
#define MASK_LB                      (0b00001111)
#define MASK_BIT0                    (0b00000001)
#define MASK_BIT1                    (0b00000010)
#define MASK_BIT2                    (0b00000100)
#define MASK_BIT3                    (0b00001000)
#define MASK_BIT4                    (0b00010000)
#define MASK_BIT5                    (0b00100000)
#define MASK_BIT6                    (0b01000000)
#define MASK_BIT7                    (0b10000000)
#define MASK_BIT1_0                  (0b00000011)
#define MASK_BIT2_1                  (0b00000110)
#define MASK_BIT3_2                  (0b00001100)
#define MASK_BIT4_3                  (0b00011000)
#define MASK_BIT5_4                  (0b00110000)
#define MASK_BIT6_5                  (0b01100000)
#define MASK_BIT7_6                  (0b11000000)
#define MASK_BIT2_0                  (0b00000111)
#define MASK_BIT3_1                  (0b00001110)
#define MASK_BIT4_2                  (0b00011100)
#define MASK_BIT5_3                  (0b00111000)
#define MASK_BIT6_4                  (0b01110000)
#define MASK_BIT7_5                  (0b11100000)
#define MASK_BIT3_0                  (MASK_LB)
#define MASK_BIT4_1                  (0b00011110)
#define MASK_BIT5_2                  (0b00111100)
#define MASK_BIT6_3                  (0b01111000)
#define MASK_BIT7_4                  (MASK_HB)
#define MASK_BIT4_0                  (0b00011111)
#define MASK_BIT5_1                  (0b00111110)
#define MASK_BIT6_2                  (0b01111100)
#define MASK_BIT7_3                  (0b11111000)
#define MASK_BIT5_0                  (0b00111111)
#define MASK_BIT6_1                  (0b01111110)
#define MASK_BIT6_0                  (0b01111111)
#define MASK_BIT7_1                  (0b11111110)
#define MASK_BIT7_0                  (MASK_ALL)

/* Mask for better identification */
#define MASK_CPLD_MAJOR_VER          (MASK_BIT7_6)
#define MASK_CPLD_MINOR_VER          (MASK_BIT5_0)
#define MASK_CPLD_ID                 (MASK_BIT2_0)
#define MASK_HW_REV                  (MASK_BIT1_0)
#define MASK_CLK_TIMING_2_CTRL       (0b00011010)
#define MASK_PW_SYS_CTRL             (0b00001001)
#define MASK_LED_CLEAR               (0b00000101)
#define MASK_FAN_PWM_DIAG_MODE       (MASK_BIT4_0)

/* common manipulation */
#define INVALID(i, min, max)    ((i < min) || (i > max) ? 1u : 0u)

#endif
