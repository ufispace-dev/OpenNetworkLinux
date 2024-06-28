/* A poe api file for the ufispace_s6301_56stp poe driver
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

/****************************************************************************
***
***    FILE NAME :
***      bcm_poe_api.c
***
***    DESCRIPTION :
***      APIs to verify PoE init, interrupt, detection, classification, and powering
***
***    HISTORY :
***    
***             File Creation
***
***************************************************************************/

/*==========================================================================
 *                                                                          
 *      Library Inclusion Segment
 *                                                                          
 *==========================================================================
 */
#include <linux/io.h>

/* User-defined Library */
#include "bcm_poe_api.h"

/*==========================================================================
 *
 *      Constant
 *
 *==========================================================================
 */
#define BUFFER_SIZE    64*1024
#define BCM_POE_INT    0x4
/* 16bit byte swap. For example 0x1122 -> 0x2211                            */
#define BYTE_SWAP_16BIT(X)      ((((X) & 0xff) << 8) | (((X) & 0xff00) >> 8))
#define CHECK_BIT(var, bit)     ((var & (1 << bit)) >> bit)

/*==========================================================================
 *
 *      Structrue segment
 *
 *==========================================================================
 */

const BCM_POE_BUDGE_T poeConfigTD3X2[] =
{
    /* Guard band = 7W, need to swap hex value due to endian issue between MCU NUC029Z */
    /* power bank,  totalPower ,  guardBand */
    {0x0,           0x0,           0x46},     /* Internal PSU = 0W */
    {0x1,           0x1CE8,        0x46},     /* Internal PSU = 740W */
    {0x2,           0x1CE8,        0x46},     /* Internal PSU = 740W */
    {0x3,           0x3CBC,        0x46},     /* Internal PSU = 1550W */
};


static u8 g_poePortMapTD3X2[] =
{
    0,   1,    2,    3,    4,    5,    6,    7,   8,   9,  10,  11, /* 1-12 */
    12,  13 ,  14 ,  15 ,  16 ,  17 ,  18,  19,  20,  21,  22,  23, /* 13-24 */
    24,  25 ,  26 ,  27 ,  28 ,  29 ,  30,  31,  32,  33,  34,  35, /* 25-36 */
    36,  37 ,  38 ,  39 ,  40 ,  41 ,  42,  43,  44,  45,  46,  47, /* 37-48 */
};

static u8 g_poePortMap[] =
{
    1,   0,    3,    2,    5,    4,    7,    6,   9,   8,  11,  10, /* 1-12 */
    13,  12 ,  15 ,  14 ,  17 ,  16 ,  19,  18,  21,  20,  23,  22, /* 13-24 */
    25,  24 ,  27 ,  26 ,  29 ,  28 ,  31,  30,  33,  32,  35,  34, /* 25-36 */
    37,  36 ,  39 ,  38 ,  41 ,  40 ,  43,  42,  45,  44,  47,  46, /* 37-48 */
    
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

static BCM_POE_LOGICAL_PORT_MAPPING_T g_portMapInfoTD3X2 = { E_POE_LOGICAL_MAP_DISABLE, 48 };
static BCM_POE_LED_CONFIG_T g_ledConfigTD3X2 = {
    E_ENABLE_LED_FUNCTIONALITY,
    E_CTRL_SPI_INTERFACE,
    E_MSB_ORDER,
    E_TWO_LED_PER_PORT,
    BCM_POE_LED_OFF_BITS,
    BCM_POE_LED_REQUST_BITS,
    BCM_POE_LED_ERROR_BITS,
    BCM_POE_LED_ON_BITS,
    E_74164_LTACH
};

const BCM_POE_TWO_PAIR_CONFIG_T poeTwoPairConfig[] = 
{
    /* logicalPortIndex,  fourPairEnable,  DevId, PriCh, AltCh  */
    {0,                   0x0,             0x0,   0x0,   0x0}, 
    {1,                   0x0,             0x0,   0x1,   0x1}, 
    {2,                   0x0,             0x0,   0x2,   0x2}, 
    {3,                   0x0,             0x0,   0x3,   0x3}, 
    {4,                   0x0,             0x0,   0x4,   0x4}, 
    {5,                   0x0,             0x0,   0x5,   0x5}, 
    {6,                   0x0,             0x0,   0x6,   0x6}, 
    {7,                   0x0,             0x0,   0x7,   0x7},

    {8,                   0x0,             0x1,   0x0,   0x0}, 
    {9,                   0x0,             0x1,   0x1,   0x1}, 
    {10,                  0x0,             0x1,   0x2,   0x2}, 
    {11,                  0x0,             0x1,   0x3,   0x3}, 
    {12,                  0x0,             0x1,   0x4,   0x4}, 
    {13,                  0x0,             0x1,   0x5,   0x5}, 
    {14,                  0x0,             0x1,   0x6,   0x6}, 
    {15,                  0x0,             0x1,   0x7,   0x7},

    {16,                  0x0,             0x2,   0x0,   0x0}, 
    {17,                  0x0,             0x2,   0x1,   0x1}, 
    {18,                  0x0,             0x2,   0x2,   0x2}, 
    {19,                  0x0,             0x2,   0x3,   0x3}, 
    {20,                  0x0,             0x2,   0x4,   0x4}, 
    {21,                  0x0,             0x2,   0x5,   0x5}, 
    {22,                  0x0,             0x2,   0x6,   0x6}, 
    {23,                  0x0,             0x2,   0x7,   0x7},

    {24,                  0x0,             0x3,   0x0,   0x0}, 
    {25,                  0x0,             0x3,   0x1,   0x1}, 
    {26,                  0x0,             0x3,   0x2,   0x2}, 
    {27,                  0x0,             0x3,   0x3,   0x3}, 
    {28,                  0x0,             0x3,   0x4,   0x4}, 
    {29,                  0x0,             0x3,   0x5,   0x5}, 
    {30,                  0x0,             0x3,   0x6,   0x6}, 
    {31,                  0x0,             0x3,   0x7,   0x7},

    {32,                  0x0,             0x4,   0x0,   0x0}, 
    {33,                  0x0,             0x4,   0x1,   0x1}, 
    {34,                  0x0,             0x4,   0x2,   0x2}, 
    {35,                  0x0,             0x4,   0x3,   0x3}, 
    {36,                  0x0,             0x4,   0x4,   0x4}, 
    {37,                  0x0,             0x4,   0x5,   0x5}, 
    {38,                  0x0,             0x4,   0x6,   0x6}, 
    {39,                  0x0,             0x4,   0x7,   0x7},

    {40,                  0x0,             0x5,   0x0,   0x0}, 
    {41,                  0x0,             0x5,   0x1,   0x1}, 
    {42,                  0x0,             0x5,   0x2,   0x2}, 
    {43,                  0x0,             0x5,   0x3,   0x3}, 
    {44,                  0x0,             0x5,   0x4,   0x4}, 
    {45,                  0x0,             0x5,   0x5,   0x5}, 
    {46,                  0x0,             0x5,   0x6,   0x6}, 
    {47,                  0x0,             0x5,   0x7,   0x7}
};

const BCM_POE_FOUR_PAIR_CONFIG_T poeFourPairConfig[] = 
{
    /* logicalPortIndex,  fourPairEnable,  DevId, PriCh, AltCh, fourPairPowerUpMode,                      fourPairDetectMode,                  fourPairMode */
    {0,                   0x1,             0x0,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {1,                   0x1,             0x0,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {2,                   0x1,             0x0,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {3,                   0x1,             0x0,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {4,                   0x1,             0x1,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {5,                   0x1,             0x1,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {6,                   0x1,             0x1,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {7,                   0x1,             0x1,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE},

    {8,                   0x1,             0x2,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {9,                   0x1,             0x2,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {10,                  0x1,             0x2,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {11,                  0x1,             0x2,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {12,                  0x1,             0x3,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {13,                  0x1,             0x3,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {14,                  0x1,             0x3,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {15,                  0x1,             0x3,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE},

    {16,                  0x1,             0x4,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {17,                  0x1,             0x4,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {18,                  0x1,             0x4,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {19,                  0x1,             0x4,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {20,                  0x1,             0x5,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {21,                  0x1,             0x5,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {22,                  0x1,             0x5,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {23,                  0x1,             0x5,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE},

    {24,                  0x1,             0x6,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {25,                  0x1,             0x6,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {26,                  0x1,             0x6,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {27,                  0x1,             0x6,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {28,                  0x1,             0x7,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {29,                  0x1,             0x7,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {30,                  0x1,             0x7,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {31,                  0x1,             0x7,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE},

    {32,                  0x1,             0x8,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {33,                  0x1,             0x8,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {34,                  0x1,             0x8,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {35,                  0x1,             0x8,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {36,                  0x1,             0x9,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {37,                  0x1,             0x9,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {38,                  0x1,             0x9,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {39,                  0x1,             0x9,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE},

    {40,                  0x1,             0xa,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {41,                  0x1,             0xa,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {42,                  0x1,             0xa,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {43,                  0x1,             0xa,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {44,                  0x1,             0xb,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {45,                  0x1,             0xb,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {46,                  0x1,             0xb,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}, 
    {47,                  0x1,             0xb,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL, E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY, E_POE_FOUR_PAIR_60W_MODE}

};

const BCM_POE_FOUR_PAIR_CONFIG_T poeFourPair90WConfig[] = 
{
    /* logicalPortIndex,  fourPairEnable,  DevId, PriCh, AltCh, fourPairPowerUpMode,                      fourPairDetectMode,                  fourPairMode */
    {0,                   0x1,             0x0,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {1,                   0x1,             0x0,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {2,                   0x1,             0x0,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {3,                   0x1,             0x0,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {4,                   0x1,             0x1,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {5,                   0x1,             0x1,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {6,                   0x1,             0x1,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {7,                   0x1,             0x1,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE},

    {8,                   0x1,             0x2,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {9,                   0x1,             0x2,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {10,                  0x1,             0x2,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {11,                  0x1,             0x2,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {12,                  0x1,             0x3,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {13,                  0x1,             0x3,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {14,                  0x1,             0x3,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {15,                  0x1,             0x3,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE},

    {16,                  0x1,             0x4,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {17,                  0x1,             0x4,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {18,                  0x1,             0x4,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {19,                  0x1,             0x4,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {20,                  0x1,             0x5,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {21,                  0x1,             0x5,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {22,                  0x1,             0x5,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {23,                  0x1,             0x5,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE},

    {24,                  0x1,             0x6,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {25,                  0x1,             0x6,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {26,                  0x1,             0x6,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {27,                  0x1,             0x6,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {28,                  0x1,             0x7,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {29,                  0x1,             0x7,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {30,                  0x1,             0x7,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {31,                  0x1,             0x7,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE},

    {32,                  0x1,             0x8,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {33,                  0x1,             0x8,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {34,                  0x1,             0x8,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {35,                  0x1,             0x8,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {36,                  0x1,             0x9,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {37,                  0x1,             0x9,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {38,                  0x1,             0x9,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {39,                  0x1,             0x9,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE},

    {40,                  0x1,             0xa,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {41,                  0x1,             0xa,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {42,                  0x1,             0xa,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {43,                  0x1,             0xa,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {44,                  0x1,             0xb,   0x0,   0x1,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {45,                  0x1,             0xb,   0x2,   0x3,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {46,                  0x1,             0xb,   0x4,   0x5,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}, 
    {47,                  0x1,             0xb,   0x6,   0x7,   E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS, E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER, E_POE_FOUR_PAIR_60W_MODE}
};

u8 g_poefwversion[64];
static BCM_POE_SETTING_INFO_T gPoeSettingInfo[48];

static u8 g_power_mode[][32] = 
{
    {"IEEE 802.3af"},
    {"High inrush"},
    {"Pre-IEEE 802.3at"},
    {"IEEE 802.3at"}
};

static u8 g_violation_type[][32] = 
{
    {"None"},
    {"Class based"},
    {"User defined"}
};

/*==========================================================================
 *
 *      Static Variable segment
 *
 *==========================================================================
 */

/*==========================================================================
 *
 *      Function Definition Segment
 *
 *==========================================================================
 */
/*==========================================================================
 *
 *      Local Function Body segment
 *
 *==========================================================================
 */

/*==========================================================================
 *
 *    External Funtion Body segment
 *
 *==========================================================================
 */

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_GetPortPowerInfo
 *
 *  DESCRIPTION :
 *      Get voltage/ current and power info of a PoE port
 *
 *
 *  INPUT :
 *      dev - i2c device
 *      lPort - the specify port
 *
 *  OUTPUT :
 *      
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_GetPortPowerInfo(struct device *dev, u32 lPort, BCM_POE_PORT_POWER_INFO_T *portPowerInfo)
{
    u8 bytes = 0;
    E_ERROR_TYPE  ret = E_TYPE_FAILED;
    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_COM_PORT_MEASURE_GET;
    pCmd->data1 = g_poePortMap[lPort];
    bcm_poe_halInsertCheckSum(pCmd);
    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp)) == E_BCM_SCP_SUCCESS)
    {
        ret = E_TYPE_SUCCESS;
        memcpy(&portPowerInfo->vol, &pResp->data2, sizeof(portPowerInfo->vol));
        memcpy(&portPowerInfo->cur, &pResp->data4, sizeof(portPowerInfo->cur)); /* current */
        memcpy(&portPowerInfo->temperature, &pResp->data6, sizeof(portPowerInfo->temperature)); /* current */
        memcpy(&portPowerInfo->power, &pResp->data8, sizeof(portPowerInfo->power)); /* power */
    }

    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_GetTotalPowerInfo
 *
 *  DESCRIPTION :
 *      Get total power info from M051
 *
 *
 *  INPUT :
 *      none
 *
 *  OUTPUT :
 *      dev - i2c device
 *      allocPowerInfo - power info structure
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_GetTotalPowerInfo(struct device *dev, BCM_POE_TOTAL_PWR_ALLOC_INFO_T *allocPowerInfo)
{
    u8 bytes=0;
    E_ERROR_TYPE ret = E_TYPE_FAILED;

    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_COM_TOTAL_POWER_ALLOC_GET;

    bcm_poe_halInsertCheckSum(pCmd);

    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp)) == E_BCM_SCP_SUCCESS)
    {
        ret = E_TYPE_SUCCESS;
        memcpy(&allocPowerInfo->total_pwr_alloc, &pResp->data1, sizeof(allocPowerInfo->total_pwr_alloc));
        memcpy(&allocPowerInfo->pwr_availabe, &pResp->data3, sizeof(allocPowerInfo->pwr_availabe));
        allocPowerInfo->mpsm_status = pResp->data5;
        allocPowerInfo->lcut = pResp->data6;
    }
    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_PowerShow
 *
 *  DESCRIPTION :
 *      Show the voltage/ current and power of a PoE port
 *
 *
 *  INPUT :
 *      dev - i2c device
 *      lPort - the specify port to show
 * 
 *  OUTPUT :
 *      buf - show output buffer
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_PowerShow(struct device *dev, u32 lPort, u8 *buf)
{

    s32 retry;
    E_ERROR_TYPE ret = E_TYPE_SUCCESS;
    BCM_POE_PORT_POWER_INFO_T portPowerInfo;
    memset(&portPowerInfo, 0, sizeof(BCM_POE_PORT_POWER_INFO_T));

    for (retry = 0 ; retry < 5 ; retry++)
    {
        bcm_poe_GetPortPowerInfo(dev, lPort, &portPowerInfo);
        portPowerInfo.vol = (BYTE_SWAP_16BIT(portPowerInfo.vol)) * BCM_POE_TYPE_DATA_PORT_BYTE2VOLT; /* milli volt */
        portPowerInfo.power = BYTE_SWAP_16BIT(portPowerInfo.power); /* walt */
        portPowerInfo.cur = BYTE_SWAP_16BIT(portPowerInfo.cur);

        if ((portPowerInfo.power == 0xFFFF) || (portPowerInfo.cur == 0xFFFF)\
            ||(portPowerInfo.power == 0x0) || (portPowerInfo.cur == 0x0))
            {
                BCM_POE_DELAY(200);
                continue;
            }
        break;
    }

    sprintf(buf, "Port %-2d \nVoltage = %-2d.%-3d V\nCurrent = %-2d.%-3d A\nPower = %d.%d W\n",
                lPort, portPowerInfo.vol/1000, portPowerInfo.vol%1000, 
                portPowerInfo.cur/1000, portPowerInfo.cur%1000, 
                portPowerInfo.power/10, portPowerInfo.power%10);
    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_DynamicPowerShow
 *
 *  DESCRIPTION :
 *      Show the voltage/ current and power of a PoE port
 *
 *
 *  INPUT :
 *      dev - i2c device
 *      lPort - the specify port to show
 *
 *  OUTPUT :
 *      buf - show output buffer
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_DynamicPowerShow(struct device *dev, u32 lPort, u8 *buf)
{
    E_ERROR_TYPE  ret = E_TYPE_FAILED;
    BCM_POE_PORT_POWER_INFO_T portInfo;

    bcm_poe_GetPortPowerInfo(dev, lPort, &portInfo);

    portInfo.vol = (BYTE_SWAP_16BIT(portInfo.vol)) * BCM_POE_TYPE_DATA_PORT_BYTE2VOLT; /* milli volt */
    portInfo.power = BYTE_SWAP_16BIT(portInfo.power); /* walt */
    portInfo.cur = BYTE_SWAP_16BIT(portInfo.cur);

    sprintf(buf, "Port %d : %d.%dV %d.%03dA %d.%dW\r\n", lPort, portInfo.vol/1000, portInfo.vol%1000, 
                portInfo.cur/1000, portInfo.cur%1000, portInfo.power/10, portInfo.power%10);
    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_TemperatureRead
 *
 *  DESCRIPTION :
 *      Read the temperature of a PoE port
 *
 *
 *  INPUT :
 *      dev - i2c device
 *      lPort - the specify port to show
 *
 *  OUTPUT :
 *      tempValue - raw temperature value
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_TemperatureRead(struct device *dev, u32 lPort, u16 *tempValue)
{
    BCM_POE_PORT_POWER_INFO_T portInfo;
    memset(&portInfo, 0, sizeof(BCM_POE_PORT_POWER_INFO_T));

    bcm_poe_GetPortPowerInfo(dev, lPort, &portInfo);
    *tempValue = BYTE_SWAP_16BIT(portInfo.temperature);
    return E_TYPE_SUCCESS;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_TemperatureShow
 *
 *  DESCRIPTION :
 *      Show the temperature of a PoE port
 *
 *
 *  INPUT :
 *      dev - i2c device
 *      lPort - the specify port to show
 *
 *  OUTPUT :
 *      buf - show output buffer
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_TemperatureShow(struct device *dev, u32 lPort, u8 *buf)
{
    u16  tempValue;
    
    bcm_poe_TemperatureRead(dev, lPort,&tempValue);

    tempValue = ((tempValue - 120) * (-125) + 12500)/100;
    sprintf(buf, "Port %2d : Temperature %d oC \n", lPort,tempValue);
    return E_TYPE_SUCCESS;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_SetPortPowerMode
 *
 *  DESCRIPTION :
 *      Set PoE port power up mode.
 *
 *  INPUT :
 *       dev - i2c device
 *       lPort - specific port index
 *       mode - 0:AF  1:High inrush  2:Pre-AT  3:AT
 *
 *  OUTPUT :
 *       none
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_SetPortPowerMode(struct device *dev, u32 lPort,u8 powerUpMode)
{
    u8 bytes=0;
    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;
    E_ERROR_TYPE ret = E_TYPE_FAILED;

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_COM_PORT_POWER_MODE_SET;
    pCmd->data1 = g_poePortMap[lPort];
    pCmd->data2 = powerUpMode;

    bcm_poe_halInsertCheckSum(pCmd);

    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp)) == E_BCM_SCP_SUCCESS)
    {
        if(pResp->data2 == BCM_POE_ACK)
        {
            ret = E_TYPE_SUCCESS;
        }
    }
    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_SetPortEnableStatus
 *
 *  DESCRIPTION :
 *      Set PoE port enable status.
 *      
 *
 *  INPUT :
 *      dev - i2c device
 *      lPort  - front port index
 *      status - PSE status (0: Disable  1: Enable  2: ForcePower)
 *
 *  OUTPUT :
 *      none
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_SetPortEnableStatus(struct device *dev, u32 lPort, u8 status)
{
    u8 bytes=0;
    E_ERROR_TYPE  ret = E_TYPE_FAILED;
    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;

    if ( (status < E_BCM_POE_PSE_DISABLE) || (status > E_BCM_POE_PSE_FORCE_POWER) )
    {
        dev_err(dev, "Unable to set PoE port %d enable status\n", lPort);
        return ret;
    }

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_COM_PORT_PSE_SWITCH;
    pCmd->data1 = g_poePortMap[lPort];
    pCmd->data2 = status;

    bcm_poe_halInsertCheckSum(pCmd);

    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp)) == E_BCM_SCP_SUCCESS)
    {
        if( pResp->data2 == BCM_POE_ACK)
        {
            gPoeSettingInfo[lPort].pseEnable = status;
            ret = E_TYPE_SUCCESS;
        }
    }
    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_PortmappingSet
 *
 *  DESCRIPTION :
 *      Init poe port mapping table.
 *      
 *
 *  INPUT :
 *      portMap - port mapping table of poe
 *      portNum - the size of port mapping table
 *
 *  OUTPUT :
 *       none
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_PortmappingSet(u8 *portMap, u32 tableSize)
{
    if( tableSize > sizeof(g_poePortMap) )
    {
        return E_TYPE_FAILED;
    }

    memset(g_poePortMap, 0xFF, sizeof(g_poePortMap));
    memcpy(g_poePortMap, portMap, tableSize);

    return E_TYPE_SUCCESS;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_GetPoeSystemInfo
 *
 *  DESCRIPTION :
 *      Get PoE system information
 *      
 *
 *  INPUT :
 *      dev - i2c device
 *
 *  OUTPUT :
 *       *sysInfo
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_GetPoeSystemInfo(struct device *dev, BCM_POE_SYSTEM_INFO_T *sysInfo)
{
    u8 bytes=0;
    E_ERROR_TYPE ret = E_TYPE_FAILED;

    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_COM_SYSTEM_STATUS_GET;
    bcm_poe_halInsertCheckSum(pCmd);
    bytes=bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp);
    if(bytes == E_BCM_SCP_SUCCESS)
    {
        sysInfo->mcu_state = pResp->command;
        sysInfo->mode = pResp->data1;
        sysInfo->maxPort = pResp->data2;
        sysInfo->portMapEnableStatus = pResp->data3;
        memcpy(&sysInfo->deviceId, &pResp->data4, sizeof(sysInfo->deviceId));
        sysInfo->swVersion = pResp->data6;
        sysInfo->eepromStatus = pResp->data7;
        sysInfo->systemStatus = pResp->data8;
        sysInfo->extSwVersion = pResp->data9;
        ret = E_TYPE_SUCCESS;
    }
    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_SetPortDetectType
 *
 *  DESCRIPTION :
 *      a API to set port detect type of PoE
 *
 *
 *  INPUT :
 *      dev - i2c device
 *      port - front port index
 *      pdType - which type the PD under test belongs to
 *
 *  OUTPUT :
 *      none
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_SetPortDetectType(struct device *dev, u32  lPort, E_BCM_POE_PD_TYPE pdType)
{
    u8 bytes = 0;
    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;
    E_ERROR_TYPE ret = E_TYPE_FAILED;

    if(pdType != E_BCM_POE_PD_LEGACY && pdType != E_BCM_POE_PD_STANDARD)
    {
        pdType = BCM_POE_PD_DEFAULT_TYPE;
    }

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_COM_PORT_DETECTION_SET;
    pCmd->data1 = g_poePortMap[lPort];
    pCmd->data2 = pdType;

    bcm_poe_halInsertCheckSum(pCmd);

    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp)) == E_BCM_SCP_SUCCESS)
    {
        if(pResp->data2 == BCM_POE_ACK)
        {
            ret = E_TYPE_SUCCESS;
        }
    }

    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_SetPortViolationType
 *
 *  DESCRIPTION :
 *      a API to set port violation type of PoE (Power threshold type)
 *
 *
 *  INPUT :
 *      dev - i2c device
 *      port - front port index
 *      threshold_type - power threshold type
 *
 *  OUTPUT :
 *      none
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_SetPortViolationType(struct device *dev, u32 lPort, u8 threshold_type)
{
    u8 bytes=0;
    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;
    E_ERROR_TYPE ret = E_TYPE_FAILED;

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_COM_PORT_VIOLATION_SET;
    pCmd->data1 = g_poePortMap[lPort];
    pCmd->data2 = threshold_type;

    bcm_poe_halInsertCheckSum(pCmd);

    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp)) == E_BCM_SCP_SUCCESS)
    {
        if(pResp->data2 == BCM_POE_ACK)
        {
            ret = E_TYPE_SUCCESS;
        }
    }

    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_SetPortClassificationType
 *
 *  DESCRIPTION :
 *      a API to set port classification type of PoE
 *
 *
 *  INPUT :
 *      dev - i2c device
 *      port - front port index
 *      class_enable - enable classification of the PD under test belongs to
 *
 *  OUTPUT :
 *      none
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_SetPortClassificationType(struct device *dev, u32 lPort, u8 class_enable)
{
    u8 bytes=0;
    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;
    E_ERROR_TYPE ret = E_TYPE_FAILED;

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_COM_PORT_CLASSIFICATION_ENABLE_SET;
    pCmd->data1 = g_poePortMap[lPort];
    pCmd->data2 = class_enable;

    bcm_poe_halInsertCheckSum(pCmd);

    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp)) == E_BCM_SCP_SUCCESS)
    {
        if(pResp->data2 == BCM_POE_ACK)
        {
            ret = E_TYPE_SUCCESS;
        }
    }

    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_SetPortPriority
 *
 *  DESCRIPTION :
 *      a API to set port priority of PoE
 *
 *
 *  INPUT :
 *      dev - i2c device
 *      lPort - front port index
 *      priority - priority type value
 *
 *  OUTPUT :
 *      none
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_SetPortPriority(struct device *dev, u32  lPort, E_BCM_POE_PRIORITY priority)
{
    u8 bytes=0;
    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;
    E_ERROR_TYPE ret = E_TYPE_FAILED;

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_COM_PORT_PRIORITY_SET;
    pCmd->data1 = g_poePortMap[lPort];
    pCmd->data2 = priority;

    bcm_poe_halInsertCheckSum(pCmd);

    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp)) == E_BCM_SCP_SUCCESS)
    {
        if(pResp->data2 == BCM_POE_ACK)
        {
            ret = E_TYPE_SUCCESS;
        }
    }
    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_GetPortPoEStatus
 *
 *  DESCRIPTION :
 *      To get port status
 *
 *  INPUT :
 *      dev - i2c device
 *      port - front port index
 *
 *  OUTPUT :
 *      poePortInfo
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_GetPortPoEStatus(struct device *dev, u8 lPort, BCM_POE_PORT_INFO_T *poePortInfo)
{
    u8 bytes=0;
    E_ERROR_TYPE ret = E_TYPE_FAILED;
    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_COM_PORT_STATUS_GET;
    pCmd->data1 = g_poePortMap[lPort];

    bcm_poe_halInsertCheckSum(pCmd);

    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp)) == E_BCM_SCP_SUCCESS)
    {
        poePortInfo->status        = pResp->data2;
        poePortInfo->errorType     = pResp->data3;
        poePortInfo->pdClassInfo   = pResp->data4;
        poePortInfo->remotePdType  = pResp->data5;
        poePortInfo->turnOffOrNot  = pResp->data6;
        poePortInfo->portPowerMode = pResp->data7;
        poePortInfo->portPowerChannel = pResp->data8;
        poePortInfo->portConnectionCheck = pResp->data9;
        ret = E_TYPE_SUCCESS;
    }
    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_SetPortDisconnectType
 *
 *  DESCRIPTION :
 *      a API to set port disconnect type of PoE
 *
 *
 *  INPUT :
 *      dev - i2c device
 *      lPort - front port index
 *      pdType - which type the PD under test belongs to
 *
 *  OUTPUT :
 *      none
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_SetPortDisconnectType(struct device *dev, u32  lPort, E_BCM_POE_DISCONNECT_MODE disconnectType)
{
    u8 bytes=0;
    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;
    E_ERROR_TYPE ret = E_TYPE_FAILED;

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_COM_PORT_DISCONNECT_SET;
    pCmd->data1 = g_poePortMap[lPort];
    pCmd->data2 = disconnectType;

    bcm_poe_halInsertCheckSum(pCmd);

    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp)) == E_BCM_SCP_SUCCESS)
    {
        if(pResp->data2 == BCM_POE_ACK)
        {
            ret = E_TYPE_SUCCESS;
        }
    }
    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_SetPowerSourceConfiguration
 *
 *  DESCRIPTION :
 *      a API to set power bank configruation
 *
 *  INPUT :
 *      dev - i2c device
 *      powerSourceInfo - Power bank, total power, guard band information.
 *
 *  OUTPUT :
 *      none
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_SetPowerSourceConfiguration(struct device *dev, BCM_POE_BUDGE_T powerSourceInfo)
{
    u8 bytes=0;
    u16 totalPower=0, guardBand=0;
    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;
    E_ERROR_TYPE ret = E_TYPE_FAILED;

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_COM_SYSTEM_POWER_GUARD_SET;
    pCmd->data1 = powerSourceInfo.powerBank;

    if(powerSourceInfo.totalPower > 0xff)
    {
        totalPower = BYTE_SWAP_16BIT(powerSourceInfo.totalPower);
        memcpy(&pCmd->data2, &totalPower, sizeof(totalPower));  /* main Power Maximum Power Budget 0.1W/LSB*/
    }
    else
    {
        pCmd->data2 = 0;
        pCmd->data3 = powerSourceInfo.totalPower;
    }

    if(powerSourceInfo.guardBand > 0xff)
    {
        guardBand = BYTE_SWAP_16BIT(powerSourceInfo.guardBand);
        memcpy(&pCmd->data4, &guardBand, sizeof(guardBand));    /* Guard Band 0.1W LSB*/
    }
    else
    {
        pCmd->data4 = 0;
        pCmd->data5 = powerSourceInfo.guardBand;    /* Guard Band 0.1W LSB*/
    }

    bcm_poe_halInsertCheckSum(pCmd);

    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp))== E_BCM_SCP_SUCCESS)
    {
        if(pResp->data2 == BCM_POE_ACK)
        {
            ret = E_TYPE_SUCCESS;
        }
    }
    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_SetPowerManagementMode
 *
 *  DESCRIPTION :
 *      a API to set power management mode of PoE
 *
 *
 *  INPUT :
 *      dev - i2c device
 *      powerManageMode - power management mode
 *
 *  OUTPUT :
 *      none
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_SetPowerManagementMode(struct device *dev, u8 powerManageMode)
{
    u8 bytes=0;
    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;
    E_ERROR_TYPE ret = E_TYPE_FAILED;

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_COM_POWER_MANAGE_MODE_SET;
    pCmd->data1 = powerManageMode;

    bcm_poe_halInsertCheckSum(pCmd);

    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp)) == E_BCM_SCP_SUCCESS)
    {
        if(pResp->data1 == BCM_POE_TYPE_DATA_ACK)
        {
            ret = E_TYPE_SUCCESS;
        }
    }
    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_SetPortEventMask
 *
 *  DESCRIPTION :
 *      a API to set port event mask
 *
 *
 *  INPUT :
 *      dev - i2c device
 *      eventMask - port event mask
 *
 *  OUTPUT :
 *      none
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_SetPortEventMask(struct device *dev, u8 eventMask)
{
    u8 bytes=0;
    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;
    E_ERROR_TYPE ret = E_TYPE_FAILED;

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_COM_PORT_EVENT_SET;
    pCmd->data1 = eventMask;

    bcm_poe_halInsertCheckSum(pCmd);

    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp)) == E_BCM_SCP_SUCCESS)
    {
        if(pResp->data1 == BCM_POE_TYPE_DATA_ACK)
        {
            ret = E_TYPE_SUCCESS;
        }
    }
    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_GetPortEventStatus
 *
 *  DESCRIPTION :
 *      a API to get port event status and clear interrupt
 *
 *
 *  INPUT :
 *      none
 *
 *  OUTPUT :
 *      dev - i2c device
 *      globalEventStatus - event status
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_GetPortEventStatus(struct device *dev, u8 clearIntFlag, BCM_POE_PORT_EVENT_INFO_T *globalEventStatus)
{
    u8 bytes=0;
    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;
    E_ERROR_TYPE ret = E_TYPE_FAILED;

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_COM_GLOBAL_PORT_EVENT_STATUS_GET;
    pCmd->data1 = clearIntFlag;

    bcm_poe_halInsertCheckSum(pCmd);

    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp)) == E_BCM_SCP_SUCCESS)
    {
        ret = E_TYPE_SUCCESS;
        globalEventStatus->eventMask    = pResp->data1;
        globalEventStatus->eventStatus  = pResp->data2;
        globalEventStatus->portBitMask[0] = pResp->data3;
        globalEventStatus->portBitMask[1] = pResp->data4;
        globalEventStatus->portBitMask[2] = pResp->data5;
        globalEventStatus->portBitMask[3] = pResp->data6;
        globalEventStatus->portBitMask[4] = pResp->data7;
        globalEventStatus->portBitMask[5] = pResp->data8;
    }
    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_SaveRunningConfig
 *
 *  DESCRIPTION :
 *      a API to save running config as default configuration
 *
 *  INPUT :
 *      dev - i2c device
 *
 *  OUTPUT :
 *      none
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_SaveRunningConfig(struct device *dev)
{
    u8 bytes=0;
    BCM_POE_TYPE_PKTBUF_I2C_UPGRADE_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_UPGRADE_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_UPGRADE_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_UPGRADE_T *) RespBuffer;
    E_ERROR_TYPE ret = E_TYPE_FAILED;

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_COM_IMAGE_UPGRADE;
    pCmd->subCommand = BCM_POE_TYPE_SUBCOM_SAVE_CONFIG;

    bcm_poe_halInsertCheckSum((BCM_POE_TYPE_PKTBUF_I2C_T *)pCmd);

    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp)) == E_BCM_SCP_SUCCESS)
    {
        if(pResp->data1 == BCM_POE_ACK)
        {
            ret = E_TYPE_SUCCESS;
        }
    }
    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_GetPortExtendedStatus
 *
 *  DESCRIPTION :
 *      To get port extended status
 *
 *  INPUT :
 *      dev - i2c device
 *      port - front port index
 *
 *  OUTPUT :
 *      poePortInfo
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_GetPortExtendedStatus(struct device *dev, u8 lPort, BCM_POE_PORT_EXTEND_INFO_T *poePortExtInfo)
{
    E_ERROR_TYPE  ret = E_TYPE_FAILED;
    u8 bytes=0;
    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_COM_PORT_EXT_CONFIG_GET;
    pCmd->seqNo = 0x01;
    pCmd->data1 = g_poePortMap[lPort];

    bcm_poe_halInsertCheckSum(pCmd);

    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp)) == E_BCM_SCP_SUCCESS)
    {
        poePortExtInfo->powerMode   = pResp->data2;
        poePortExtInfo->violateType = pResp->data3;
        poePortExtInfo->maxPower    = pResp->data4;
        poePortExtInfo->priority    = pResp->data5;
        poePortExtInfo->phyPort     = pResp->data6;
        poePortExtInfo->fourPairPowerUpMode = pResp->data7;
        poePortExtInfo->dynamicPowerLimit = pResp->data8;
        ret = E_TYPE_SUCCESS;
    }
    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_PortConfigShow
 *
 *  DESCRIPTION :
 *      Show the configuration of a PoE port
 *
 *
 *  INPUT :
 *      dev - i2c device
 *      lPort - the specify port to show
 *
 *  OUTPUT :
 *      buf - show output buffer
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_PortConfigShow(struct device *dev, u32 lPort, u8 *buf)
{
    E_ERROR_TYPE  ret = E_TYPE_FAILED;
    BCM_POE_PORT_EXTEND_INFO_T poePortExtInfo;
    u8 tmp[128];

    if(bcm_poe_GetPortExtendedStatus(dev, lPort, &poePortExtInfo) == E_TYPE_SUCCESS)
    {
        sprintf(buf, "powerMode: %s\n", g_power_mode[poePortExtInfo.powerMode]);
        sprintf(tmp, "violateType: %s\n", g_violation_type[poePortExtInfo.violateType]);
        strcat(buf, tmp);
        sprintf(tmp, "maxPower: 0x%x\n", poePortExtInfo.maxPower);
        strcat(buf, tmp);
        sprintf(tmp, "priority: %d\n", poePortExtInfo.priority);
        strcat(buf, tmp);
        sprintf(tmp, "phyPort: %x\n", poePortExtInfo.phyPort);
        strcat(buf, tmp);
        sprintf(tmp, "fourPairPowerUpMode: %x\n", poePortExtInfo.fourPairPowerUpMode);
        strcat(buf, tmp);
        sprintf(tmp, "dynamicPowerLimit: %x\n", poePortExtInfo.dynamicPowerLimit);
        strcat(buf, tmp);
        ret = E_TYPE_SUCCESS;
    }
    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_FourPairSetting
 *
 *  DESCRIPTION :
 *      Set four pair state of a PoE port
 *
 *
 *  INPUT :
 *      dev - i2c device
 *      lPort - the specify port to enable 4-pair
 *      status - enable / disable
 *
 *  OUTPUT :
 *      none
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_FourPairSetting(struct device *dev, u32 lPort, u32 status)
{
    if( (status==E_BCM_FOUR_PAIR_ENABLE_60W) || (status==E_BCM_FOUR_PAIR_ENABLE_90W) || (status==E_BCM_FOUR_PAIR_BT_TYPE3) || (status==E_BCM_FOUR_PAIR_BT_TYPE4) )
    {
        if(bcm_poe_Set4PairMapping(dev, poeFourPair90WConfig[lPort]) != E_TYPE_SUCCESS)
        {
            dev_err(dev, "Set 4 pair mapping configuration %d failed\n", lPort);
        }
    }
    else
    {
        if(bcm_poe_Set4PairMapping(dev, poeFourPairConfig[lPort]) != E_TYPE_SUCCESS)
        {
            dev_err(dev, "Set 4 pair mapping configuration %d failed\n", lPort);
        }
    }
    return E_TYPE_SUCCESS;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_Set4PairMapping
 *
 *  DESCRIPTION :
 *      a API to set 4-pair port mapping
 *
 *
 *  INPUT :
 *      dev - i2c device
 *      pairMapInfo - port 4-pair mapping info
 *
 *  OUTPUT :
 *      none
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_Set4PairMapping(struct device *dev, BCM_POE_FOUR_PAIR_CONFIG_T pairMapInfo)
{
    E_ERROR_TYPE  ret = E_TYPE_FAILED;
    u8 bytes = 0;
    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_COM_PORT_FOUR_PAIR_MAP_SET;
    pCmd->seqNo = 0x01;
    pCmd->data1 = pairMapInfo.logicalPortIndex;
    pCmd->data2 = pairMapInfo.fourPairEnable;
    pCmd->data3 = pairMapInfo.DevId;
    pCmd->data4 = pairMapInfo.PrimaryCh;
    pCmd->data5 = pairMapInfo.AlternateCh;
    pCmd->data6 = pairMapInfo.fourPairPowerUpMode;
    pCmd->data7 = pairMapInfo.fourPairDetectMode;
    pCmd->data9 = pairMapInfo.fourPairMode;
    bcm_poe_halInsertCheckSum(pCmd);

    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp)) == E_BCM_SCP_SUCCESS)
    {
        if(pResp->data1 == BCM_POE_ACK)
        {
            ret = E_TYPE_SUCCESS;
        }
    }
    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_TwoPairSetting
 *
 *  DESCRIPTION :
 *      Set two pair state of a PoE port
 *
 *
 *  INPUT :
 *      dev - i2c device
 *      lPort - the specify port to enable 2-pair
 *
 *  OUTPUT :
 *      none
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_TwoPairSetting(struct device *dev, u32 lPort)
{
    if ( bcm_poe_Set2PairMapping(dev, poeTwoPairConfig[lPort]) != E_TYPE_SUCCESS )
    {
        dev_err(dev, "Set 2 pair mapping configuration %d failed\n", lPort);
    }

    return E_TYPE_SUCCESS;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_Set2PairMapping
 *
 *  DESCRIPTION :
 *      a API to set 2-pair port mapping
 *
 *
 *  INPUT :
 *      dev - i2c device
 *      pairMapInfo - port 2-pair mapping info
 *
 *  OUTPUT :
 *      none
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_Set2PairMapping(struct device *dev, BCM_POE_TWO_PAIR_CONFIG_T  pairMapInfo)
{
    E_ERROR_TYPE  ret = E_TYPE_FAILED;
    u8 bytes=0;
    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_COM_PORT_FOUR_PAIR_MAP_SET;
    pCmd->seqNo = 0x01;
    pCmd->data1 = pairMapInfo.logicalPortIndex;
    pCmd->data2 = pairMapInfo.fourPairEnable;
    pCmd->data3 = pairMapInfo.DevId;
    pCmd->data4 = pairMapInfo.PrimaryCh;
    pCmd->data5 = pairMapInfo.AlternateCh;
    pCmd->data6 = 0x0;
    pCmd->data7 = 0x0;
    pCmd->data8 = 0x0;
    bcm_poe_halInsertCheckSum(pCmd);

    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp)) == E_BCM_SCP_SUCCESS)
    {
        if(pResp->data1 == BCM_POE_ACK)
        {
            ret = E_TYPE_SUCCESS;
        }
    }
    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_SetPortMaxPowerThreshold
 *
 *  DESCRIPTION :
 *      a API to set port max power threshold
 *
 *
 *  INPUT :
 *      dev - i2c device
 *      port - front port index
 *      maxPower - 8-bits value with 0.2W/LSB in 2-pair, 0.4W/LSB in 4-pair
 *
 *  OUTPUT :
 *      none
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_SetPortMaxPowerThreshold(struct device *dev, u32 lPort, u32 maxPower)
{
    E_ERROR_TYPE  ret = E_TYPE_FAILED;
    u8 bytes=0;
    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;

    if(maxPower < 0)
    {
        dev_err(dev, "Unable to set PoE max port threshold %d for port %d\n", maxPower, lPort);
        return E_TYPE_FAILED;
    }

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_COM_PORT_POWER_LIMIT_SET;
    pCmd->seqNo = 0x01;
    pCmd->data1 = g_poePortMap[lPort];;

    if(gPoeSettingInfo[lPort].userDefinedFlag == 0x1)
    {
        if( gPoeSettingInfo[lPort].fourPairEnable == E_BCM_FOUR_PAIR_ENABLE_60W )
        {
            /* 0.4W/LSB */
            pCmd->data2 = maxPower;
        }
        else
        {
            pCmd->data2 = maxPower;
        }
    }
    else
    {
        pCmd->data2 = maxPower;
    }

    bcm_poe_halInsertCheckSum(pCmd);

    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp)) == E_BCM_SCP_SUCCESS)
    {
        if(pResp->data2 == BCM_POE_ACK)
        {
            gPoeSettingInfo[lPort].maxPower = maxPower;
            ret = E_TYPE_SUCCESS;
        }
    }

    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_SetLogicalPortMapping
 *
 *  DESCRIPTION :
 *      a API to set logical port mapping
 *
 *
 *  INPUT :
 *      dev - i2c device
 *      portMapInfo - logical port mapping info
 *
 *  OUTPUT :
 *      none
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_SetLogicalPortMapping(struct device *dev, BCM_POE_LOGICAL_PORT_MAPPING_T portMapInfo)
{
    E_ERROR_TYPE  ret = E_TYPE_FAILED;
    u8 bytes=0;
    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_COM_PORT_MAPPING_ENABLE;
    pCmd->seqNo = 0x00;
    pCmd->data1 = portMapInfo.portMappingMode;
    pCmd->data2 = portMapInfo.totalPortNum; 

    bcm_poe_halInsertCheckSum(pCmd);

    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp)) == E_BCM_SCP_SUCCESS)
    {
        if(pResp->data1 == BCM_POE_ACK)
        {
            ret = E_TYPE_SUCCESS;
        }
    }
    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_SetPowerManagementExtendedMode
 *
 *  DESCRIPTION :
 *      a API to set power management extended mode of PoE
 *
 *
 *  INPUT :
 *      dev - i2c device
 *      powerManageExtMode - power management extended mode
 *
 *  OUTPUT :
 *      none
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_SetPowerManagementExtendedMode(struct device *dev, BCM_POE_POWER_MANAGE_EXTENDED_T powerManageExtMode)
{
    E_ERROR_TYPE  ret = E_TYPE_FAILED;
    u8 bytes=0;
    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_COM_POWER_MANAGE_EXTEND_SET;
    pCmd->seqNo = 0x01;
    pCmd->data1 = powerManageExtMode.preAllocEnable;
    pCmd->data2 = powerManageExtMode.powerupMode;
    pCmd->data3 = powerManageExtMode.disconnectMode;
    pCmd->data9 = powerManageExtMode.threshold;

    bcm_poe_halInsertCheckSum(pCmd);

    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp)) == E_BCM_SCP_SUCCESS)
    {
        if(pResp->data1 == BCM_POE_ACK)
        {
            ret = E_TYPE_SUCCESS;
        }
    }
    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_PoeCmd
 *
 *  DESCRIPTION :
 *      Issue PoE command for debug
 *
 *  INPUT :
 *      dev - i2c device
 *      cmd, f1~f11
 *
 *  OUTPUT :
 *      poeResponse
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_PoeCmd(struct device *dev, u8 cmd, u8 f1, u8 f2, u8 f3, u8 f4, u8 f5, u8 f6, 
                    u8 f7, u8 f8, u8 f9, u8 f10)
{
    E_ERROR_TYPE  ret = E_TYPE_SUCCESS;
    u8 bytes=0;
    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = cmd;
    pCmd->seqNo = f1;
    pCmd->data1 = f2;
    pCmd->data2 = f3;
    pCmd->data3 = f4;
    pCmd->data4 = f5;
    pCmd->data5 = f6;
    pCmd->data6 = f7;
    pCmd->data7 = f8;
    pCmd->data8 = f9;
    pCmd->data9 = f10;

    bcm_poe_halInsertCheckSum(pCmd);

    dev_info(dev, "+-------+-------+------+------+------+------+------+------+------+------+---------+\n");
    dev_info(dev, "|Command| Seq    |DATA0 |DATA1 |DATA2 |DATA3 |DATA4 |DATA5 |DATA6 |DATA7 |DATA8 | CheckSum |\n");
    dev_info(dev, "|0x%-5.2x| 0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|\n",
    pCmd->command,pCmd->seqNo,pCmd->data1,pCmd->data2,pCmd->data3,pCmd->data4,pCmd->data5,pCmd->data6,
    pCmd->data7,pCmd->data8,pCmd->data9,pCmd->checksum);
    dev_info(dev, "+-------+-------+------+------+------+------+------+------+------+------+---------+\n");

    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp)) == E_BCM_SCP_SUCCESS)
    {
        dev_info(dev, "+-------+-------+------+------+------+------+------+------+------+------+---------+\n");
        dev_info(dev, "|Respond| Seq    |DATA0 |DATA1 |DATA2 |DATA3 |DATA4 |DATA5 |DATA6 |DATA7 |DATA8 | CheckSum |\n");
        dev_info(dev, "|0x%-5.2x| 0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|\n",
        pResp->command,pResp->seqNo,pResp->data1,pResp->data2,pResp->data3,pResp->data4,pResp->data5,pResp->data6,
        pResp->data7,pResp->data8,pResp->data9,pResp->checksum);
        dev_info(dev, "+-------+-------+------+------+------+------+------+------+------+------+---------+\n");
        ret = E_TYPE_SUCCESS;
    }
    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_SystemStatus
 *
 *  DESCRIPTION :
 *      Display MCU system status
 *      
 *
 *  INPUT :
 *      dev - i2c device
 * 
 *  OUTPUT :
 *      buf - output buffer
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_SystemStatus(struct device *dev, char *buf)
{   
    E_ERROR_TYPE  ret = E_TYPE_SUCCESS;
    BCM_POE_SYSTEM_INFO_T sysInfo;
    u8 temp[64];

    if (bcm_poe_GetPoeSystemInfo(dev, &sysInfo) != E_TYPE_SUCCESS )
    {
        dev_err(dev, "Get system status fail\n");
        return E_TYPE_FAILED;
    }

    sprintf(buf, "Mode: ");
    switch(sysInfo.mode)
    {   
        case 2:
            strcat(buf, "Automatic\n");
            break;
        case 0:
            strcat(buf, "Semiautomatic\n");
            break;
        default:
            strcat(buf, "Unknown\n");
            break;
    }

    snprintf(temp, sizeof(temp), "Maximun Ports: %d\n", sysInfo.maxPort);
    strcat(buf, temp);

    strcat(buf, "Devide ID: ");
    switch(BYTE_SWAP_16BIT(sysInfo.deviceId))
    {   
        case 0xE011:
            strcat(buf, "BCM59011\n");
            break;
        case 0xE121:
            strcat(buf, "BCM59121\n");
            break;
        case 0xE131:
            strcat(buf, "BCM59131\n");
            break;
        case 0xE1FF:
            strcat(buf, "Generic\n");
            break;
        default:
            strcat(buf, "Unknown\n");
            break;
    }

    snprintf(temp, sizeof(temp), "SW Version: %x.%x.%x.%x\n", ((sysInfo.swVersion) >> 4), 
            ((sysInfo.swVersion) & 0x0F), ((sysInfo.extSwVersion) >> 4), ((sysInfo.extSwVersion) & 0x0F));
    strcat(buf, temp);

    strcat(buf, "MCU Type: ");
    switch(sysInfo.eepromStatus)
    {   
        case 0:
            strcat(buf, "ST Micro ST32F100\n");
            break;
        case 1:
            strcat(buf, "Nuvoton M0516\n");
            break;
        case 5:
            strcat(buf, "Nuvoton M0518\n");
            break;
        case 6:
            strcat(buf, "Nuvoton NUC029ZPoE\n");
            break;
        default:
            strcat(buf, "Unknown\n");
            break;
    } 

    strcat(buf, "System status:\n");

    switch(CHECK_BIT(sysInfo.systemStatus, 0))
    {
        case 0:
            strcat(buf, "    - Configuration is saved\n");
            break;
        case 1:
            strcat(buf, "    - Configuration is dirty\n");
            break;
        default:
            
            break;
    }
    
    switch(CHECK_BIT(sysInfo.systemStatus, 1))
    {
        case 0:
            strcat(buf, "    - No system reset before\n");
            break;
        case 1:
            strcat(buf, "    - System reset happened\n");
            break;
        default:  
            break;
    }

    switch(CHECK_BIT(sysInfo.systemStatus, 2))
    {
        case 0:
            strcat(buf, "    - Global Disable pin is deasserted\n");
            break;
        case 1:
            strcat(buf, "    - Global Disable pin is asserted\n");
            break;
        default:  
            break;
    }

    switch(CHECK_BIT(sysInfo.systemStatus,3))
    {
        case 0:
            strcat(buf, "    - Port map is not present\n");
            break;
        case 1:
            strcat(buf, "    - Port map is present\n");
            break;
        default:  
            break;
    }

    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_PortStatus
 *
 *  DESCRIPTION :
 *      Display MCU Port status
 *      
 *
 *  INPUT :
 *      dev - i2c device
 *      port - front port index
 * 
 *  OUTPUT :
 *      buf - out buffer
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_PortStatus(struct device *dev, u8 lport, char *buf)
{   
    E_ERROR_TYPE ret = E_TYPE_SUCCESS;
    BCM_POE_PORT_INFO_T poePortInfo;
    u8 temp[64];

    if (bcm_poe_GetPortPoEStatus(dev, lport, &poePortInfo) != E_TYPE_SUCCESS )
    {
        dev_err(dev, "Get port status fail\n");
        return -1;
    }
    sprintf(buf, "Port: %d\n", lport);

    strcat(buf, "    - Status: ");

    switch(poePortInfo.status)
    {   
        case 0:
            strcat(buf, "Disabled\n");
            break;
        case 1:
            strcat(buf, "Searching\n");
            break;
        case 2:
            strcat(buf, "Delivering power\n");
            break;
        case 3:
            strcat(buf, "Test mode\n");
            break;
        case 4:
            strcat(buf, "Fault\n");
            break;
        case 5:
            strcat(buf, "Other fault\n");
            break;
        case 6:
            strcat(buf, "Requesting power\n");
            break;
        default:
            strcat(buf, "Reserved\n");
            break;
    }
    switch (poePortInfo.status)
    {
        case E_BCM_POE_SEARCH:
            strcat(buf, "    - Error type: ");
            switch (poePortInfo.errorType)
            {
                case 0:
                    strcat(buf, "Unknown\n");
                    break;
                case 1:
                    strcat(buf, "Short circuit\n");
                    break;
                case 2:
                    strcat(buf, "High cappacitor\n");
                    break;
                case 3:
                    strcat(buf, "Rlow\n");
                    break;
                case 4:
                    strcat(buf, "Reserved\n");
                    break;
                case 5:
                    strcat(buf, "Rhigh\n");
                    break;
                case 6:
                    strcat(buf, "Open circuit\n");
                    break;
                case 7:
                    strcat(buf, "FET failture\n");
                    break;
                default:
                    strcat(buf, "Reserved\n");
                    break;
            }
            break;
        case E_BCM_POE_FAULT:
        case E_BCM_POE_OTHER_FAULT:
            strcat(buf, "    - Error type: ");
            switch (poePortInfo.errorType)
            {
                case 0:
                    strcat(buf, "None\n");
                    break;
                case 1:
                    strcat(buf, "MPS absent\n");
                    break;
                case 2:
                    strcat(buf, "Short\n");
                    break;
                case 3:
                    strcat(buf, "Overload\n");
                    break;
                case 4:
                    strcat(buf, "Power denied\n");
                    break;
                case 5:
                    strcat(buf, "Thermal shutdown\n");
                    break;
                case 6:
                    strcat(buf, "Startup failture\n");
                    break;
                case 7:
                    strcat(buf, "UVLO\n");
                    break;
                case 8:
                    strcat(buf, "OVLO\n");
                    break;
                default:
                    strcat(buf, "Reserved\n");
                    break;
            }
            break;
        default:
            break;
    }
    
    strcat(buf, "    - PD class: ");
    switch(poePortInfo.pdClassInfo)
    {   
       
        case 0:
            strcat(buf, "0\n");
            break;
        case 1:
            strcat(buf, "1\n");
            break;
        case 2:
            strcat(buf, "2\n");
            break;
        case 3:
            strcat(buf, "3\n");
            break;
        case 4:
            strcat(buf, "4\n");
            break;
        case 5:
            strcat(buf, "5\n");
            break;
        case 6:
            strcat(buf, "6\n");
            break;
        case 7:
            strcat(buf, "7\n");
            break;
        case 8:
            strcat(buf, "8\n");
            break;
        case 0xE:
            strcat(buf, "Mismatch\n");
            break;
        case 0xF:
            strcat(buf, "Over current\n");
            break;
        default:
            strcat(buf, "Reserved\n");
            break;
    }

    
    strcat(buf, "    - Remote power device type: ");
    switch(poePortInfo.remotePdType)
    {   
        case 0:
            strcat(buf, "PD none\n");
            break;
        case 1:
            strcat(buf, "IEEE PD\n");
            break;
        case 2:
            strcat(buf, "Pre-standard PD\n");
            break;
        case 3:
            strcat(buf, "Extended detection range PD\n");
            break;
        default:
            strcat(buf, "Reserved\n");
            break;
    } 

    sprintf(temp, "    - MPSS status: %x\n", poePortInfo.turnOffOrNot);
    strcat(buf, temp);

    strcat(buf, "    - Port powered mode: ");
    switch(poePortInfo.portPowerMode)
    {   
        case 0:
            strcat(buf, "Low power(15w)\n");
            break;
        case 1:
            strcat(buf, "High power(30w)\n");
            break;
        case 2:
            strcat(buf, "Four-pair(30w)\n");
            break;
        case 3:
            strcat(buf, "Four-pair(60w)\n");
            break;
        case 4:
            strcat(buf, "Four-pair(15w)\n");
            break;
        case 5:
            strcat(buf, "Four-pair(90w)\n");
            break;
        case 6:
            strcat(buf, "Two-pair(45w)\n");
            break;
        default:
            strcat(buf, "Reserved\n");
            break;
    }

    strcat(buf, "    - Powered channels: ");
    switch(poePortInfo.remotePdType)
    {   
        case 0:
            strcat(buf, "Both are not powered\n");
            break;
        case 1:
            strcat(buf, "Primary channel is powered\n");
            break;
        case 2:
            strcat(buf, "Alternative channel is powered\n");
            break;
        case 3:
            strcat(buf, "Both channels are powered\n");
            break;
        default:
            strcat(buf, "Reserved\n");
            break;
    } 

    strcat(buf, "    - Connection check: ");
    switch(poePortInfo.remotePdType)
    {   
        case 0:
            strcat(buf, "None\n");
            break;
        case 1:
            strcat(buf, "Shared PD interface with the primary\n");
            break;
        case 2:
            strcat(buf, "Separate PD interface\n");
            break;
        case 3:
            strcat(buf, "Unknown\n");
            break;
        default:
            strcat(buf, "Reserved\n");
            break;
    } 

    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_ResetPort
 *
 *  DESCRIPTION :
 *      Reset PoE port.
 *
 *  INPUT :
 *       dev - i2c device
 *       lPort - specific port index
 *       mode - 0: Not reset, 1: reset
 *
 *  OUTPUT :
 *       none
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_ResetPort(struct device *dev, u32 lPort, u8 reset)
{
    u8 bytes=0;
    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;
    E_ERROR_TYPE ret = E_TYPE_FAILED;

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_COM_PORT_RESET;
    pCmd->data1 = g_poePortMap[lPort];
    pCmd->data2 = reset;

    bcm_poe_halInsertCheckSum(pCmd);

    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp)) == E_BCM_SCP_SUCCESS)
    {
        if(pResp->data2 == BCM_POE_ACK)
        {
            ret = E_TYPE_SUCCESS;
        }
    }
    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_SetMCULedConfig
 *
 *  DESCRIPTION :
 *      a API to set MCU LED configuration
 *
 *
 *  INPUT :
 *      dev - i2c device
 *      ledConfig - LED configuration
 *
 *  OUTPUT :
 *      none
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_SetMCULedConfig(struct device *dev, BCM_POE_LED_CONFIG_T ledConfig)
{
    E_ERROR_TYPE  ret = E_TYPE_FAILED;
    u8 bytes=0;
    BCM_POE_TYPE_PKTBUF_I2C_T *pCmd = (BCM_POE_TYPE_PKTBUF_I2C_T *) CmdBuffer;
    BCM_POE_TYPE_PKTBUF_I2C_T *pResp = (BCM_POE_TYPE_PKTBUF_I2C_T *) RespBuffer;

    /* fill poe message buffer */
    memset(pCmd, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);
    memset(pResp, BCM_POE_NA_FIELD_VALUE, I2C_MSG_LEN);

    pCmd->command = BCM_POE_TYPE_LED_BEHAVIOR_CONFIG_SET;
    pCmd->seqNo = 0x01;
    pCmd->data1 = ledConfig.mode;
    pCmd->data2 = ledConfig.interface;
    pCmd->data3 = ledConfig.order;
    pCmd->data4 = ledConfig.ledPortNum;
    pCmd->data5 = ledConfig.off;
    pCmd->data6 = ledConfig.request;
    pCmd->data7 = ledConfig.error;
    pCmd->data8 = ledConfig.on;
    pCmd->data9 = ledConfig.options;    
    bcm_poe_halInsertCheckSum(pCmd);

    if( (bytes = bcm_poe_halBcmI2cCmd(dev, (u8 *)pCmd, (u8 *)pResp)) == E_BCM_SCP_SUCCESS)
    {
        if(pResp->data1 == BCM_POE_ACK)
        {
            ret = E_TYPE_SUCCESS;
        }
    }
    return ret;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_init
 *
 *  DESCRIPTION :
 *      Init poe module to read a system status SCP packet on
 *      power on, hw reset, or sw reset.
 *      
 *
 *  INPUT :
 *      dev - i2c device
 *
 *  OUTPUT :
 *       none
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_init(struct device *dev)
{
    int ret = E_TYPE_SUCCESS;
    int initNum = 0, i = 0, lPort;
    BCM_POE_BUDGE_T poeConfig[MAX_BCM_POE_BANK];
    BCM_POE_PORT_EVENT_INFO_T poeEventStatus;
    BCM_POE_SYSTEM_INFO_T sysInfo;
    BCM_POE_POWER_MANAGE_EXTENDED_T powerManageExtMode;
    u8 clear_flag = 1;
    u8 reg_val, reg_val_now;

    dev_info(dev, "PoE Init\n");

    /* Disable PoE Power from CPLD */
    dev_info(dev, "Disable POE power...\n");
    reg_val = read_lpc_reg(dev, (u16)CPLD_REG_MISC_CNTL);
    reg_val_now = reg_val & 0xfb;
    write_lpc_reg(dev, (u16)CPLD_REG_MISC_CNTL, reg_val_now);

    /* Delay 3 second to wait for PoE function ready */
    dev_info(dev, "Delay 3 second to wait for PoE function ready...\n");
    BCM_POE_DELAY(3000);


    /* set port mapping */
    ret = bcm_poe_PortmappingSet(g_poePortMapTD3X2, sizeof(g_poePortMapTD3X2));
    if(ret != E_TYPE_SUCCESS) 
    {
        dev_err(dev, "Over size for the poe's port mapping table\n");
    }

    /* read system status according 59136 datasheet */
    dev_info(dev, "\nGet PoE system status...\n");
    memset(&sysInfo, 0, sizeof(sysInfo));
    if(bcm_poe_GetPoeSystemInfo(dev, &sysInfo) != E_TYPE_SUCCESS )
    {
        dev_err(dev, "Get system status fail\n");
        goto __FUN_RET;
    }

    /* show PoE software version */
    sprintf((u8 *)g_poefwversion, "%x.%x.%x.%x", ((sysInfo.swVersion) >> 4), ((sysInfo.swVersion) & 0x0F),\
             ((sysInfo.extSwVersion) >> 4), ((sysInfo.extSwVersion) & 0x0F)); 
    dev_info(dev, "PoE software version %s, device id 0x%x\n", g_poefwversion, BYTE_SWAP_16BIT(sysInfo.deviceId));
;
    BCM_POE_DELAY(100); /*delay 100ms */

    if(CHECK_BIT(sysInfo.systemStatus, 3) == BCM_POE_MAP_NOT_PRESENT)
    {
        /* Setup port mapping */
        g_portMapInfoTD3X2.portMappingMode = E_POE_START_FOUR_PAIR_MAPPING;
        if (bcm_poe_SetLogicalPortMapping(dev, g_portMapInfoTD3X2) != E_TYPE_SUCCESS)
            dev_err(dev, "Start port mapping setup failed\n");

        BCM_POE_DELAY(300); /*delay 300ms */

        for (lPort = POE_MIN_PORT_NUM; lPort <= POE_MAX_PORT_NUM; lPort++)
        {
            BCM_POE_DELAY(30); /*delay 30ms */

            dev_info(dev, "Enable PoE port%d mapping...", lPort);
            if (bcm_poe_TwoPairSetting(dev, lPort) != E_TYPE_SUCCESS)
                dev_err(dev, "Enable PoE port%d mapping...fail\n", lPort);
            else
                dev_info(dev, "Enable PoE port%d mapping...ok\n", lPort);
        }
        BCM_POE_DELAY(1000); /*delay 1000ms */

        g_portMapInfoTD3X2.portMappingMode = E_POE_END_FOUR_PAIR_MAPPING;
        if (bcm_poe_SetLogicalPortMapping(dev, g_portMapInfoTD3X2) != E_TYPE_SUCCESS) {
            dev_err(dev, "Stop port mapping setup failed\n");
        } 
        BCM_POE_DELAY(4000); /*delay 4s */    
    }

    /* Config port event mask */
    dev_info(dev, "Set Port event mask...\n");
    if(bcm_poe_SetPortEventMask(dev, BCM_POE_EVENT_POWERUP | BCM_POE_EVENT_REQ_POWER | BCM_POE_EVENT_DISCONNECT 
                                | BCM_POE_EVENT_FAULT | BCM_POE_EVENT_OVERLOAD) != E_TYPE_SUCCESS)
    {
        dev_err(dev, "Set PoE interrupt event failed\n");
        goto __FUN_RET;
    }

    /* Config power management extended mode, need to disable pre-allocation for guardband behavior */
    memset(&powerManageExtMode, 0x0, sizeof(powerManageExtMode));

    /* Try to enable pre-allocation for PSU redundant test */
    powerManageExtMode.preAllocEnable = 0x0;
    powerManageExtMode.powerupMode = 0x0; /* Simultaneous power up */

    BCM_POE_DELAY(20);

    dev_info(dev, "Set power management extended mode %d...\n", powerManageExtMode.powerupMode);
    if(bcm_poe_SetPowerManagementExtendedMode(dev, powerManageExtMode) != E_TYPE_SUCCESS)
    {
        dev_err(dev, "Set PoE power management extended mode failed\n");
        goto __FUN_RET;
    }

    BCM_POE_DELAY(20);

    dev_info(dev, "Set power management mode %d...\n", E_BCM_POE_POWER_DYNAMIC_WITH_PORT_PRIORITY);
    if(bcm_poe_SetPowerManagementMode(dev, E_BCM_POE_POWER_DYNAMIC_WITH_PORT_PRIORITY) != E_TYPE_SUCCESS)
    {
        dev_err(dev, "Set PoE power management mode dynamic with port priority failed\n");
        goto __FUN_RET;
    }

    initNum = sizeof(poeConfigTD3X2) / sizeof(poeConfigTD3X2[0]);
    memcpy(poeConfig, poeConfigTD3X2, initNum*(sizeof(BCM_POE_BUDGE_T)));

    for(i = 0; i < initNum; i++)
    {
        BCM_POE_DELAY(20);
        dev_info(dev, "Config power bank%d...\n", i);
        if (bcm_poe_SetPowerSourceConfiguration(dev, poeConfig[i]) != E_TYPE_SUCCESS)
        {
            dev_err(dev, "Set Power bank %d failed\n", i);
            goto __FUN_RET;
        }
    }

    /*
     * Not necessary, DC disconnect is default setting in chip.
     * AC disconnect is not support.
     * Disable capacitor detection (legacy mode)
     */
    for(lPort = POE_MIN_PORT_NUM; lPort <= POE_MAX_PORT_NUM; lPort++)
    {
        BCM_POE_DELAY(20);
        dev_info(dev, "Set Port %d detect type %d...\n", lPort, E_BCM_POE_PD_STANDARD_THEN_LEGACY);
        if (bcm_poe_SetPortDetectType(dev, lPort, E_BCM_POE_PD_STANDARD_THEN_LEGACY) != E_TYPE_SUCCESS)
        {
            dev_err(dev, "Set Port %d detect type %d fail\n", lPort, E_BCM_POE_PD_STANDARD_THEN_LEGACY);
            goto __FUN_RET;
        }
        BCM_POE_DELAY(20);
        dev_info(dev, "Set Port %d classification type %d...\n", lPort, E_CLASSIFICATION_ENABLE);
        if (bcm_poe_SetPortClassificationType(dev, lPort, E_CLASSIFICATION_ENABLE) != E_TYPE_SUCCESS)
        {
            dev_err(dev, "Set Port %d classification type %d fail\n", lPort, E_CLASSIFICATION_ENABLE);
            goto __FUN_RET;            
        }
        BCM_POE_DELAY(20);
        dev_info(dev, "Set Port %d violation type %d...\n", lPort, E_THRESHOLD_CLASS_BASE);
        if (bcm_poe_SetPortViolationType(dev, lPort, E_THRESHOLD_CLASS_BASE) != E_TYPE_SUCCESS)
        {
            dev_err(dev, "Set Port %d violation type %d fail\n", lPort, E_THRESHOLD_CLASS_BASE);
            goto __FUN_RET;            
        }        
        BCM_POE_DELAY(20);
        dev_info(dev, "Set Port %d power mode %d...\n", lPort, E_BCM_POE_POWERUP_AT);
        if (bcm_poe_SetPortPowerMode(dev, lPort, E_BCM_POE_POWERUP_AT) != E_TYPE_SUCCESS)
        {
            dev_err(dev, "Set Port %d power mode %d fail\n", lPort, E_BCM_POE_POWERUP_AT);
            goto __FUN_RET;            
        }         
        BCM_POE_DELAY(20);
        dev_info(dev, "Set Port %d disconnect type %d...\n", lPort, E_BCM_POE_DISCONNECT_DC);
        if (bcm_poe_SetPortDisconnectType(dev, lPort, E_BCM_POE_DISCONNECT_DC) != E_TYPE_SUCCESS)
        {
            dev_err(dev, "Set Port %d power mode %d fail\n", lPort, E_BCM_POE_DISCONNECT_DC);
            goto __FUN_RET;            
        }           
    }

    for(lPort = POE_MIN_PORT_NUM; lPort <= POE_MAX_PORT_NUM; lPort++)
    {
        BCM_POE_DELAY(20);
        dev_info(dev, "Enable PoE port%d...\n", lPort);
        if (bcm_poe_SetPortEnableStatus(dev, lPort, E_BCM_POE_PSE_ENABLE) != E_TYPE_SUCCESS)
        {
            dev_err(dev, "Enable PoE port%d fail\n", lPort);
            goto __FUN_RET;            
        }      
    }

    if (bcm_poe_SetMCULedConfig(dev, g_ledConfigTD3X2) != E_TYPE_SUCCESS)
        dev_info(dev, "MCU LED config failed\n");
    else
        dev_info(dev, "MCU LED config OK\n");

    if (bcm_poe_GetPortEventStatus(dev, clear_flag, &poeEventStatus) != E_TYPE_SUCCESS)
        dev_err(dev, "Clear MCU interrupt failed\n");
    else
        dev_info(dev, "Clear MCU interrupt OK\n");

    BCM_POE_DELAY(20);
    dev_info(dev, "Save running config...\n");
    if (bcm_poe_SaveRunningConfig(dev) != E_TYPE_SUCCESS)
    {
        dev_err(dev, "Save running config fail\n");
        goto __FUN_RET;            
    }

    /* Enable PoE Power from CPLD */
    dev_info(dev, "Enable POE power...\n");
    reg_val = read_lpc_reg(dev, (u16)CPLD_REG_MISC_CNTL);
    reg_val_now = (reg_val & 0xfb) | 0x4;
    write_lpc_reg(dev, (u16)CPLD_REG_MISC_CNTL, reg_val_now);

__FUN_RET:
    if(ret != E_TYPE_SUCCESS)
    {
        dev_err(dev, "Fail\n");
    }
    else
    {
        dev_info(dev, "Done\n");
    }
    return ret;
}