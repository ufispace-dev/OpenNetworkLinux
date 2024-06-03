/* A poe api header file for the ufispace_s6301_56stp poe driver
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
 * but WITHANY WARRANTY; without even the implied warranty of
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
***      bcm_poe_api.h
***
***    DESCRIPTION :
***      APIs to verify PoE init, interrupt, detection, classification, and powering
***
***    HISTORY :
***       
***             File Creation
***
***************************************************************************/

#ifndef __BCM_POE_API_H__
#define __BCM_POE_API_H__

/*==========================================================================
 *
 *      Library Inclusion Segment
 *
 *==========================================================================
 */
#include "bcm_poe_hal.h"

/*==========================================================================
 *
 *      Constant Definition Segment
 *
 *==========================================================================
 */
#define BCM_POE_REG_TEST_PATTER              0x55
#define BCM_POE_PD_POWER_UPPER_LIMIT         60
#define BCM_POE_PD_POWER_LOWER_LIMIT         45
#define BCM_POE_SCP_MAX_RETRY                1
#define BCM_POE_PD_DEFAULT_TYPE              E_BCM_POE_PD_STANDARD   /* standard */
#define BCM_POE_PD_DEFAULT_CLASS             0   /* class 0 */
#define MAX_BCM_POE_BANK                     16
#define BCM_POE_DEFINE_TEST_MAX_CURRENT      600
#define BCM_POE_DEFINE_TEST_MIN_CURRENT      500
#define BCM_POE_DEFINE_TEST_MAX_CONSUMPTION  32400
#define BCM_POE_DEFINE_TEST_MIN_CONSUMPTION  27000
#define BCM_POH_60W_DEFINE_TEST_MAX_CURRENT      1200
#define BCM_POH_60W_DEFINE_TEST_MIN_CURRENT      1000
#define BCM_POH_60W_DEFINE_TEST_MAX_CONSUMPTION  64800
#define BCM_POH_60W_DEFINE_TEST_MIN_CONSUMPTION  54000
#define BCM_POH_90W_DEFINE_TEST_MAX_CURRENT      1730
#define BCM_POH_90W_DEFINE_TEST_MIN_CURRENT      1500
#define BCM_POH_90W_DEFINE_TEST_MAX_CONSUMPTION  93400
#define BCM_POH_90W_DEFINE_TEST_MIN_CONSUMPTION  81000

#define BCM_POE_DETECTION_STAGE              (1<<0)
#define BCM_POE_CLASSIFICATION_STAGE         (1<<1)
#define BCM_POE_POWERUP_STAGE                (1<<2)
#define BCM_POE_DISCONNECT_STAGE             (1<<3)
#define BCM_POE_ALL_STAGES                   (BCM_POE_DETECTION_STAGE|BCM_POE_CLASSIFICATION_STAGE|BCM_POE_POWERUP_STAGE|BCM_POE_DISCONNECT_STAGE)

#define BCM_POE_EVENT_POWERUP                0x1
#define BCM_POE_EVENT_DISCONNECT             0x2
#define BCM_POE_EVENT_FAULT                  0x4
#define BCM_POE_EVENT_REQ_POWER              0x8
#define BCM_POE_EVENT_OVERLOAD               0x10
#define BCM_POE_PORT_BIT_GROUP               0x6

#define BCM_POE_LED_OFF_BITS                 0x0
#define BCM_POE_LED_REQUST_BITS              0x0
#define BCM_POE_LED_ERROR_BITS               0x0
#define BCM_POE_LED_ON_BITS                  0x42
#define BCM_POE_MAP_NOT_PRESENT              0x0

#define BCM_POE_DELAY(x)                     mdelay(x)

/* POE port number */
#define POE_MAX_PORT_NUM                     47
#define POE_MIN_PORT_NUM                     0
//#define BOARD_START_PORT_NUM               0 /* 0-based */

/* CPLD reg */
#define CPLD_REG_MISC_CNTL                   0x64

/*=========================================================================
 *
 *      Type and Structure Definition Segment
 *
 *==========================================================================
 */

typedef enum BCM_POE_PSE_STATUS_E
{
    E_BCM_POE_PSE_DISABLE     = 0,
    E_BCM_POE_PSE_ENABLE      = 1,
    E_BCM_POE_PSE_FORCE_POWER = 2
} E_BCM_POE_PSE_STATUS;

typedef enum BCM_POE_CLASS_ENABLE_E
{
    E_CLASSIFICATION_BYPASS = 0,
    E_CLASSIFICATION_ENABLE = 1
} E_BCM_POE_CLASS_ENABLE;

typedef enum BCM_POE_THRESHOLD_TYPE_E
{
    E_THRESHOLD_NONE         = 0,
    E_THRESHOLD_CLASS_BASE   = 1,
    E_THRESHOLD_USER_DEFINED = 2
} E_BCM_POE_THRESHOLD_TYPE;

typedef enum BCM_POE_STATUS_E
{
    E_BCM_POE_DISABLE     = 0,
    E_BCM_POE_SEARCH      = 1,
    E_BCM_POE_DELIVERING  = 2,
    E_BCM_POE_TEST        = 3,
    E_BCM_POE_FAULT       = 4,
    E_BCM_POE_OTHER_FAULT = 5,
    E_BCM_POE_REQUEST_PWR = 6
} E_BCM_POE_STATUS;

typedef enum BCM_POE_PDTYPE
{
    E_BCM_POE_NO_DETECT                = 0,
    E_BCM_POE_PD_LEGACY                = 1,
    E_BCM_POE_PD_STANDARD              = 2,
    E_BCM_POE_PD_STANDARD_THEN_LEGACY  = 3
} E_BCM_POE_PD_TYPE;

typedef enum BCM_POE_POWER_UP_MODE_E
{
    E_BCM_POE_POWERUP_AF          = 0,
    E_BCM_POE_POWERUP_HIGH_INRUSH = 1,
    E_BCM_POE_POWERUP_PRE_AT      = 2,
    E_BCM_POE_POWERUP_AT          = 3,
    E_BCM_POE_PRE_BT              = 4,
    E_BCM_POE_BT_TYPE3_MODE       = 5,
    E_BCM_POE_BT_TYPE4_MODE       = 6
} E_BCM_POE_POWER_UP_MODE;

typedef enum BCM_POE_DISCONNECT_MODE_E
{
    E_BCM_POE_DISCONNECT_NONE  = 0,
    E_BCM_POE_DISCONNECT_AC    = 1,
    E_BCM_POE_DISCONNECT_DC    = 2
} E_BCM_POE_DISCONNECT_MODE;

typedef enum BCM_POE_PRIORITY_E
{
    E_BCM_POE_PRIORITY_LOW      = 0,
    E_BCM_POE_PRIORITY_MEDIUM   = 1,
    E_BCM_POE_PRIORITY_HIGH     = 2,
    E_BCM_POE_PRIORITY_CRITICAL = 3
} E_BCM_POE_PRIORITY;

typedef enum BCM_POE_POWER_MANAGE_MODE_E
{
    E_BCM_POE_POWER_MANAGE_NONE                   = 0,
    E_BCM_POE_POWER_STATIC_WITH_PORT_PRIORITY     = 1,
    E_BCM_POE_POWER_DYNAMIC_WITH_PORT_PRIORITY    = 2,
    E_BCM_POE_POWER_STATIC_WITHOUT_PORT_PRIORITY  = 3,
    E_BCM_POE_POWER_DYNAMIC_WITHOUT_PORT_PRIORITY = 4
} E_BCM_POE_POWER_MANAGE_MODE;

typedef enum POE_BCM_FOUR_PAIR_ENABLE_E
{
    E_BCM_FOUR_PAIR_DISABLE = 0,
    E_BCM_FOUR_PAIR_ENABLE_60W,
    E_BCM_FOUR_PAIR_ENABLE_90W,
    E_BCM_FOUR_PAIR_BT_TYPE3,
    E_BCM_FOUR_PAIR_BT_TYPE4,
    E_BCM_FOUR_PAIR_UNSUPPORT
} E_POE_BCM_FOUR_PAIR_ENABLE;

typedef enum POE_FOUR_PAIR_DETECT_MODE_E
{
    E_POE_FOUR_PAIR_DETECT_PRIMARY_ONLY     = 0,
    E_POE_FOUR_PAIR_DETECT_STAGGERED_EITHER = 1,
    E_POE_FOUR_PAIR_DETECT_STAGGERED_BOTH   = 2
} E_POE_FOUR_PAIR_DETECT_MODE;

typedef enum POE_FOUR_PAIR_CLASS_4_MODE_E
{
    E_POE_FOUR_PAIR_30W_LOAD_SHARE_MODE     = 0,
    E_POE_FOUR_PAIR_60W_MODE                = 1
} E_POE_FOUR_PAIR_CLASS_4_MODE;

typedef enum POE_LOGICAL_PORT_MAPPING_CONFIG_S
{
    E_POE_LOGICAL_MAP_DISABLE     = 0,
    E_POE_LOGICAL_MAP_ENABLE      = 1,
    E_POE_START_FOUR_PAIR_MAPPING = 2,
    E_POE_END_FOUR_PAIR_MAPPING   = 3
} E_POE_LOGICAL_PORT_MAPPING_CONFIG_E;

typedef enum POE_FOUR_PAIR_POWER_UP_MODE_E
{
    E_POE_FOUR_PAIR_POWERUP_STAGGERED_MANUAL   = 0,
    E_POE_FOUR_PAIR_POWERUP_STAGGERED_AUTO     = 1,
    E_POE_FOUR_PAIR_POWERUP_SIMULTANEOUS       = 2
} E_POE_FOUR_PAIR_POWER_UP_MODE;

typedef enum BCM_POE_FOUR_BIT_POWER_BANK_E
{
    E_BCM_POE_FOUR_BIT_BANK_DISABLE  = 0,
    E_BCM_POE_FOUR_BIT_BANK_ENABLE   = 1
} E_BCM_POE_FOUR_BIT_POWER_BANK_MODE;

typedef enum BCM_POE_GPIO_DISABLE_STATUS_E
{
    E_BCM_POE_GPIO_DISABLE = 0,
    E_BCM_POE_GPIO_ENABLE   = 1
} E_BCM_POE_GPIO_DISABLE_STATUS;

typedef enum BCM_POE_POWER_MODE_E
{
    E_BCM_POE_LOW_POWER_15W     = 0,
    E_BCM_POE_HIGH_POWER_30W    = 1,
    E_BCM_POE_FOUR_PAIR_30W     = 2,
    E_BCM_POE_FOUR_PAIR_60W     = 3,
    E_BCM_POE_FOUR_PAIR_15W     = 4,
    E_BCM_POE_FOUR_PAIR_90W     = 5,
    E_BCM_POE_TWO_PAIR_45W      = 6
} E_BCM_POE_POWER_MODE_E;

typedef enum BCM_POE_LED_FUNCTIONALITY_E
{
    E_DISABLE_LED_FUNCTIONALITY = 0,
    E_ENABLE_LED_FUNCTIONALITY = 1
} E_BCM_POE_LED_FUNCTIONALITY;

typedef enum BCM_POE_LED_CTRL_INTERFACE_E
{
    E_CTRL_SPI_INTERFACE = 0,
    E_CTRL_GPIO_INTERFACE = 1
} E_BCM_POE_LED_CTRL_INTERFACE;

typedef enum BCM_POE_LED_ORDER_E
{
    E_MSB_ORDER = 0,
    E_LSB_ORDER = 1
} E_BCM_POE_LED_ORDER;

typedef enum BCM_POE_LED_NUMBER_E
{
    E_ONE_LED_PER_PORT = 1,
    E_TWO_LED_PER_PORT = 2
} E_BCM_POE_LED_NUMBER;

typedef enum BCM_POE_LED_OPTION_E
{
    E_74595_LATCH = 0x03,
    E_74164_LTACH = 0x0B
} E_BCM_POE_LED_OPTION;

/* PoE port status information */
typedef struct BCM_POE_PORT_INFO_S
{       
    u8      status;                            /* PoE port status             */
    u8      errorType;                         /* PoE port error type         */
    u8      pdClassInfo;                       /* Class information of the PD */
    u8      remotePdType;                      /* Remote Power Device Type    */
    u8      turnOffOrNot;                      /* Port has turned off or not  */
    u8      portPowerMode;                     /* Port power mode             */
    u8      portPowerChannel;                  /* Which channels are powered  */
    u8      portConnectionCheck;               /* Connection check             */
}BCM_POE_PORT_INFO_T;

typedef struct BCM_POE_PORT_POWER_INFO_S
{       
    u16     vol;                           /* PoE port voltage            */
    u16     cur;                           /* PoE port current            */
    u16     temperature;                       /* PoE port temperature        */
    u16     power;                             /* PoE port power consumpt     */
}BCM_POE_PORT_POWER_INFO_T;

typedef struct BCM_POE_PORT_EVENT_INFO_S
{       
    u8      eventMask;                         /* PoE event mask status       */
    u8      eventStatus;                       /* PoE event status            */
    u8      portBitMask[BCM_POE_PORT_BIT_GROUP];   /* PoE port bit mask (0~48)    */
}BCM_POE_PORT_EVENT_INFO_T;

typedef struct BCM_POE_SYSTEM_INFO_S
{       
    u8      mcu_state;                         /* MCU mode */
    u8      mode;                              /* PoE interface               */
    u8      maxPort;                           /* max ports                   */
    u8      portMapEnableStatus;               /* PoE port map enable or not  */
    u16     deviceId;                          /* 0xE121 for BCM59121         */
    u8      swVersion;                         /* software version            */
    u8      eepromStatus;                      /* EEPROM status               */
    u8      systemStatus;                      /* system status               */
    u8      extSwVersion;                      /* extended version            */
}BCM_POE_SYSTEM_INFO_T;

typedef struct BCM_POE_POWER_MANAGE_INFO_S
{       
    u8      pm_mode;                           /* Power management mode       */
    u16     totalPowerTemp1;                   /* Temp total Power1           */
    u16     guardBand1;                        /* Temp guard band1            */
    u16     totalPowerTemp2;                   /* Temp total Power2           */
    u16     guardBand2;                        /* Temp guard band2            */
}BCM_POE_POWER_MANAGE_INFO_T;

typedef struct BCM_POE_BUDGE_S
{
    u32     powerBank;
    u32     totalPower;
    u32     guardBand;
} BCM_POE_BUDGE_T;

typedef struct BCM_POE_TOTAL_PWR_ALLOC_INFO_S
{       
    u16     total_pwr_alloc;                   /* MCU mode */
    u16     pwr_availabe;                      /* Available power             */
    u8      mpsm_status;                       /* max ports                   */
    u8      lcut;                              /* PoE port map enable or not  */
}BCM_POE_TOTAL_PWR_ALLOC_INFO_T;

typedef struct BCM_POE_TWO_PAIR_CONFIG_S
{       
    u8      logicalPortIndex;                  /* PoE logical port index      */
    u8      fourPairEnable;                    /* 4-pair enable               */
    u8      DevId;                             /* Device Id                   */
    u8      PrimaryCh;                         /* Primary channel index       */
    u8      AlternateCh;                       /* Alternate channel index     */
}BCM_POE_TWO_PAIR_CONFIG_T;

typedef struct BCM_POE_PORT_CONFIG_INFO_S
{       
    u8      pseEnable;                         /* PSE functionality           */
    u8      autoMode;                          /* Automatic Mode              */
    u8      detectType;                        /* Detection type              */
    u8      classifType;                       /* Classificatin type          */
    u8      disconnType;                       /* Disconnect type             */
    u8      pairConfig;                        /* Pair configuration          */
}BCM_POE_PORT_CONFIG_INFO_T;

typedef struct BCM_POE_PORT_EXTEND_INFO_S
{       
    u8      powerMode;                         /* Power Mode                  */
    u8      violateType;                       /* Power limit violation type  */
    u8      maxPower;                          /* User defined max power threshold (0.2W/LSB) */
    u8      priority;                          /* Port priority               */
    u8      phyPort;                           /* Physical port               */
    u8      fourPairPowerUpMode;               /* 4-pair power up mode for class 4 */
    u8      dynamicPowerLimit;                 /* User-defined power limit (0.2W/LSB) */
}BCM_POE_PORT_EXTEND_INFO_T;

typedef struct BCM_POE_FOUR_PAIR_CONFIG_S
{       
    u8      logicalPortIndex;                  /* PoE logical port index      */
    u8      fourPairEnable;                    /* 4-pair enable               */
    u8      DevId;                             /* Device Id                   */
    u8      PrimaryCh;                         /* Primary channel index       */
    u8      AlternateCh;                       /* Alternate channel index     */
    u8      fourPairPowerUpMode;               /* 4-pair power up mode        */
    u8      fourPairDetectMode;                /* 4-pair detect mode          */
    u8      fourPairMode;                      /* 4-pair powerup mode of simultaneous powerup */
}BCM_POE_FOUR_PAIR_CONFIG_T;

typedef struct BCM_POE_LOGICAL_PORT_MAPPING_S
{       
    u8      portMappingMode;                   /* PoE port mapping mode       */
    u8      totalPortNum;                      /* Total PoE ports             */
}BCM_POE_LOGICAL_PORT_MAPPING_T;

typedef struct BCM_POE_POWER_MANAGE_EXTENDED_S
{       
    u8      preAllocEnable;                    /* Pre-allocation enable       */
    u8      powerupMode;                       /* PoE power up mode           */
    u8      disconnectMode;                    /* PoE disconnect mode         */
    u8      threshold;                         /* 8 bit value with 0.1W/LSB   */
}BCM_POE_POWER_MANAGE_EXTENDED_T;

typedef struct BCM_POE_SETTING_INFO_S
{
    u8      pseEnable;                         /* PSE functionality           */
    u8      fourPairEnable;                    /* 4 pair status               */
    u8      userDefinedFlag;                   /* userDefinedFlag             */
    u32     maxPower;                          /* User defined max power threshold (0.2W/LSB) */
}BCM_POE_SETTING_INFO_T;

typedef struct BCM_POE_LED_CONFIG_S
{       
    u8      mode;                              /* LED functionality           */
    u8      interface;                         /* PoE controller interface    */
    u8      order;                             /* LED MSB or LSB              */
    u8      ledPortNum;                        /* Number of LEDs              */
    u8      off;                               /* Off state bits value        */
    u8      request;                           /* REQ_POWER state bits valus  */
    u8      error;                             /* ERROR state bits valus      */
    u8      on;                                /* ON state bits valus         */
    u8      options;                           /* LED OPTIONS                 */
}BCM_POE_LED_CONFIG_T;

/*==========================================================================
 *
 *      External Funtion Segment
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
s32 bcm_poe_GetPortPowerInfo(struct device *dev, u32 lPort, BCM_POE_PORT_POWER_INFO_T *portPowerInfo);

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
s32 bcm_poe_GetTotalPowerInfo(struct device *dev, BCM_POE_TOTAL_PWR_ALLOC_INFO_T *allocPowerInfo);

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
s32 bcm_poe_PowerShow(struct device *dev, u32 lPort, u8 *buf);

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
s32 bcm_poe_DynamicPowerShow(struct device *dev, u32 lPort, u8 *buf);

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
s32 bcm_poe_TemperatureRead(struct device *dev, u32 lPort, u16 *tempValue);

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
s32 bcm_poe_TemperatureShow(struct device *dev, u32 lPort, u8 *buf);

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
s32 bcm_poe_SetPortPowerMode(struct device *dev, u32 lPort,u8 powerUpMode);

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
s32 bcm_poe_SetPortEnableStatus(struct device *dev, u32 lPort, u8 status);

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
s32 bcm_poe_PortmappingSet(u8 *portMap, u32 tableSize);

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
s32 bcm_poe_GetPoeSystemInfo(struct device *dev, BCM_POE_SYSTEM_INFO_T *sysInfo);

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
s32 bcm_poe_SetPortDetectType(struct device *dev, u32  lPort, E_BCM_POE_PD_TYPE pdType);

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
s32 bcm_poe_SetPortViolationType(struct device *dev, u32 lPort, u8 threshold_type);

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
s32 bcm_poe_SetPortClassificationType(struct device *dev, u32 lPort, u8 class_enable);

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
s32 bcm_poe_SetPortPriority(struct device *dev, u32  lPort, E_BCM_POE_PRIORITY priority);

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
s32 bcm_poe_GetPortPoEStatus(struct device *dev, u8 lPort, BCM_POE_PORT_INFO_T *poePortInfo);

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
s32 bcm_poe_SetPortDisconnectType(struct device *dev, u32  lPort, E_BCM_POE_DISCONNECT_MODE disconnectType);

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
s32 bcm_poe_SetPowerSourceConfiguration(struct device *dev, BCM_POE_BUDGE_T powerSourceInfo);

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
s32 bcm_poe_SetPowerManagementMode(struct device *dev, u8 powerManageMode);

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
s32 bcm_poe_SetPortEventMask(struct device *dev, u8 eventMask);

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
s32 bcm_poe_GetPortEventStatus(struct device *dev, u8 clearIntFlag, BCM_POE_PORT_EVENT_INFO_T *globalEventStatus);

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
s32 bcm_poe_SaveRunningConfig(struct device *dev);

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
s32 bcm_poe_GetPortExtendedStatus(struct device *dev, u8 lPort, BCM_POE_PORT_EXTEND_INFO_T *poePortExtInfo);

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
s32 bcm_poe_PortConfigShow(struct device *dev, u32 lPort, u8 *buf);

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
s32 bcm_poe_FourPairSetting(struct device *dev, u32 lPort, u32 status);

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
s32 bcm_poe_Set4PairMapping(struct device *dev, BCM_POE_FOUR_PAIR_CONFIG_T pairMapInfo);

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
s32 bcm_poe_TwoPairSetting(struct device *dev, u32 lPort);

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
s32 bcm_poe_Set2PairMapping(struct device *dev, BCM_POE_TWO_PAIR_CONFIG_T pairMapInfo);

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
s32 bcm_poe_SetPortMaxPowerThreshold(struct device *dev, u32 lPort, u32 maxPower);

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
s32 bcm_poe_SetLogicalPortMapping(struct device *dev, BCM_POE_LOGICAL_PORT_MAPPING_T portMapInfo);

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
s32 bcm_poe_SetPowerManagementExtendedMode(struct device *dev, BCM_POE_POWER_MANAGE_EXTENDED_T powerManageExtMode);

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
                    u8 f7, u8 f8, u8 f9, u8 f10);

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
s32 bcm_poe_SystemStatus(struct device *dev, char *buf);

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
s32 bcm_poe_ResetPort(struct device *dev, u32 lPort, u8 reset);

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
s32 bcm_poe_PortStatus(struct device *dev, u8 lport, char *buf);

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
s32 bcm_poe_SetMCULedConfig(struct device *dev, BCM_POE_LED_CONFIG_T ledConfig);

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
s32 bcm_poe_init(struct device *dev);

#endif /* __BCM_POE_API_H__ */
