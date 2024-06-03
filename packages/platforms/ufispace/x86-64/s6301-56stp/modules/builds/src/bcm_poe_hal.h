/* A poe hal header file for the ufispace_s6301_56stp poe driver
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
***         bcm_poe_hal.h
***
***    DESCRIPTION :
***         type, structure, and constant definiton for SCP packet used in Microsem's PoE module
***
***    HISTORY :
***       
***             File Creation
***
***************************************************************************/

#ifndef __BCM_POE_HAL_H__
#define __BCM_POE_HAL_H__

/*==========================================================================
 *
 *      Library Inclusion Segment
 *                                                                          
 *==========================================================================
 */
#include "linux/types.h"
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/delay.h>

/*==========================================================================
 *
 *      Constant Definition Segment
 *
 *==========================================================================
 */
#ifdef BCM_POE_DBG_FLAG
#define BCM_POE_DBG_CHECK(x) x
#else
#define BCM_POE_DBG_CHECK(x)
#endif


#define I2C_MAX_PKT_WAIT     400  /* 400ms */ 
#define I2C_MAX_RESET_WAIT   3000 /* 3 seconds    */
#define I2C_MSG_LEN sizeof(BCM_POE_TYPE_PKTBUF_I2C_T) /* bytes */
#define BCM_POE_EN    8  //GPIO8 is MCU_ENABLE pin
#define BCM_POE_NA_FIELD_VALUE                           0xff
#define BCM_POE_ACK                                      0x0

#define BCM_POE_TYPE_COM_PORT_PSE_SWITCH                 0x0
#define BCM_POE_TYPE_COM_PORT_MAPPING_ENABLE             0x02
#define BCM_POE_TYPE_COM_PORT_RESET                      0x3
#define BCM_POE_TYPE_COM_POWER_MANAGE_EXTEND_SET         0x0B
#define BCM_POE_TYPE_COM_PORT_EVENT_SET                  0x0D
#define BCM_POE_TYPE_COM_PORT_FOUR_PAIR_MAP_SET          0x0E
#define BCM_POE_TYPE_SUBSYSTEM_RESET                     0x09
#define BCM_POE_TYPE_COM_PORT_DETECTION_SET              0x10
#define BCM_POE_TYPE_COM_PORT_CLASSIFICATION_ENABLE_SET  0x11
#define BCM_POE_TYPE_COM_PORT_DISCONNECT_SET             0x13
#define BCM_POE_TYPE_COM_PORT_VIOLATION_SET              0x15
#define BCM_POE_TYPE_COM_PORT_POWER_LIMIT_SET            0x16
#define BCM_POE_TYPE_COM_POWER_MANAGE_MODE_SET           0x17
#define BCM_POE_TYPE_COM_SYSTEM_POWER_GUARD_SET          0x18
#define BCM_POE_TYPE_COM_TOTAL_POWER_ALLOC_GET           0x23
#define BCM_POE_TYPE_COM_SYSTEM_POWER_GUARD_GET          0x27
#define BCM_POE_TYPE_COM_PORT_PAIR_SET                   0x19
#define BCM_POE_TYPE_COM_PORT_PRIORITY_SET               0x1A
#define BCM_POE_TYPE_COM_PORT_POWER_MODE_SET             0x1C

#define BCM_POE_TYPE_COM_PORT_MAPPING_SET                0x1D
#define BCM_POE_TYPE_LED_BEHAVIOR_CONFIG_SET             0x41

#define BCM_POE_TYPE_COM_SYSTEM_STATUS_GET               0x20
#define BCM_POE_TYPE_COM_PORT_STATUS_GET                 0x21
#define BCM_POE_TYPE_COM_PORT_CONFIG_GET                 0x25
#define BCM_POE_TYPE_COM_PORT_EXT_CONFIG_GET             0x26
#define BCM_POE_TYPE_COM_POWER_MANAGE_MODE_GET           0x27
#define BCM_POE_TYPE_COM_MULTIPLE_PORT_STATUS_GET        0x28
#define BCM_POE_TYPE_COM_GLOBAL_PORT_EVENT_STATUS_GET    0x2C
#define BCM_POE_TYPE_COM_PORT_MEASURE_GET                0x30
#define BCM_POE_TYPE_COM_4BIT_POWER_BANK_ENABLE          0x47
#define BCM_POE_TYPE_COM_IMAGE_UPGRADE                   0xE0

#define BCM_POE_TYPE_SUBCOM_CHECK_CRC                    0x40
#define BCM_POE_TYPE_SUBCOM_DOWNLOAD_IMAGE               0x80
#define BCM_POE_TYPE_SUBCOM_CLEAR_IMAGE                  0xC0
#define BCM_POE_TYPE_SUBCOM_SAVE_IMAGE                   0xD0
#define BCM_POE_TYPE_SUBCOM_CLEAR_CONFIG                 0xE0
#define BCM_POE_TYPE_SUBCOM_SAVE_CONFIG                  0xF0

#define BCM_POE_TYPE_DATA_PORT_BYTE2POWER                0.1 /* 0.1W/LSB */
#define BCM_POE_TYPE_DATA_PORT_POWER2BYTE                5
#define BCM_POE_TYPE_DATA_POWERSUPPLY_NUM                0x0 /*A1 is 0x1*/
#define BCM_POE_TYPE_DATA_SYSTEM_POWER_LIMIT             70
#define BCM_POE_TYPE_DATA_SYSTEM_POWER_THRESHOLD         70
#define BCM_POE_TYPE_DATA_SYSTEM_GUARDBAND               0x00 /*0w*/
#define BCM_POE_TYPE_DATA_ACK                            0x0
#define BCM_POE_TYPE_DATA_SYSTEM_POWER2BYTE              10
#define BCM_POE_TYPE_DATA_PORT_RESET                     0x1
#define BCM_POE_TYPE_DATA_PORT_BYTE2VOLT                 64
#define BCM_POE_TYPE_DATA_PORT_BYTE2CURRENT_HIGH         0.634
#define BCM_POE_TYPE_DATA_PORT_BYTE2CURRENT_NORMAL       0.388
#define BCM_POE_TYPE_DATA_UPGRADE_OFFSET                 0x0

#define BCM_POE_TYPE_COM_PORT_CONFIG_GET                 0x25


#define BCM_POE_TYPE_SUBCOM_CLEAR_CONFIG                 0xE0
#define BCM_POE_TYPE_DATA_PORT_DELIVERING_POWER          2
#define BCM_POE_TYPE_DATA_PORT_FAULT                     4
#define BCM_POE_TYPE_DATA_PORT_OTHER_FAULT               5
#define BCM_POE_TYPE_DATA_PORT_REQUESTING_POWER          6

#define BCM_POE_MCU_BOOT_ROM_MODE                        0xaf
#define BCM_POE_MCU_APP_MODE                             0x20

/* LPC base offset */
#define REG_BASE_MB                                      0x700

/*==========================================================================
 *
 *      Type and Structure Definition Segment
 *
 *==========================================================================
 */
/* poe frame structure define*/
typedef struct BCM_POE_TYPE_PKTBUF_I2C_S
{        
    u8       command;                           /* operation code              */
    u8       seqNo;                              /* sequence number             */
    u8       data1;                              /* DATA1 field                 */
    u8       data2;                              /* DATA2 field                 */
    u8       data3;                              /* DATA3 field                 */
    u8       data4;                              /* DATA4 field                 */
    u8       data5;                              /* DATA5 field                 */
    u8       data6;                              /* DATA6 field                 */
    u8       data7;                              /* DATA7 field                 */
    u8       data8;                              /* DATA8 field                 */
    u8       data9;                              /* DATA9 field                 */
    u8       checksum;                          /* checksum                     */
}BCM_POE_TYPE_PKTBUF_I2C_T;

typedef struct POEDRV_TYPE_PKTBUF_I2C_UPGRADE_S   
{        
    u8       command;                           /* operation code              */
    u8       subCommand;                          /* sub operation code          */
    u8       data1;                              /* DATA1 field                 */
    u8       data2;                              /* DATA2 field                 */
    u8       data3;                              /* DATA3 field                 */
    u8       data4;                              /* DATA4 field                 */
    u8       data5;                              /* DATA5 field                 */
    u8       data6;                              /* DATA6 field                 */
    u8       data7;                              /* DATA7 field                 */
    u8       data8;                              /* DATA8 field                 */
    u8       data9;                              /* DATA9 field                 */
    u8       checksum;                          /* checksum                     */
}BCM_POE_TYPE_PKTBUF_I2C_UPGRADE_T;

typedef struct {
    u8 key;
    u8 echo;
    u8 subject;
    u8 subject1;
    u8 subject2;
    u8 data[8];
    u8 csumh;
    u8 csuml;
} S_BCM_SCP_PKT;

typedef enum {
    E_BCM_SCP_SUCCESS          = 0,
    E_BCM_SCP_TIMEOUT_ERROR    = 1,
    E_BCM_SCP_ECHO_ERROR       = 2,
    E_BCM_SCP_CHECKSUM_ERROR   = 3,
    E_BCM_SCP_RE_SYNC_ERROR    = 4,
    E_BCM_SCP_STATUS_RECEIVE_ERROR = 5,
    E_BCM_SCP_STATUS_TRANSMIT_ERROR = 6,
} E_BCM_SCP_RET_CODE;

typedef enum ERROR_TYPE_E
{
    E_TYPE_SUCCESS            =    0,
    E_TYPE_FAILED             =   -1
} E_ERROR_TYPE;

extern u8 CmdBuffer[I2C_MSG_LEN];
extern u8 RespBuffer[I2C_MSG_LEN];

/*==========================================================================
 *
 *      External Funtion Segment
 *
 *==========================================================================
 */
/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_halInsertCheckSum
 *
 *  DESCRIPTION :
 *      a local API to generate the check sum of a frame to be transmitted
 *
 *  INPUT :
 *      none
 *
 *  OUTPUT :
 *      *pCmd - Command frame structure
 *
 *  RETURN :
 *      none
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
void bcm_poe_halInsertCheckSum(BCM_POE_TYPE_PKTBUF_I2C_T *pCmd);

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_halSCPPktPrint
 *
 *  DESCRIPTION :
 *      a global API to print the content of a SCP packet, solely for debugging
 *
 *
 *  INPUT :
 *      *pkt - points to a SCP packet
 *
 *  OUTPUT :
 *      none
 *
 *  RETURN :
 *      none
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
void bcm_poe_halSCPPktPrint(BCM_POE_TYPE_PKTBUF_I2C_T *pkt);

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_halSCPPktReceive
 *
 *  DESCRIPTION :
 *      receive a SCP packet
 *
 *
 *  INPUT :
 *      *pkt - points to a SCP packet
 *
 *  OUTPUT :
 *      *pkt - points to the received packet
 *
 *  RETURN :
 *      E_BCM_SCP_RET_CODE - error code 
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
E_BCM_SCP_RET_CODE bcm_poe_halSCPPktReceive(struct device *dev, BCM_POE_TYPE_PKTBUF_I2C_T *pkt);

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_halSCPPktTransmit
 *
 *  DESCRIPTION :
 *      transmit a SCP packet
 *
 *
 *  INPUT :
 *      *pkt - points to a SCP packet
 *
 *  OUTPUT :
 *      none
 *
 *  RETURN :
 *      E_BCM_SCP_RET_CODE - error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
E_BCM_SCP_RET_CODE bcm_poe_halSCPPktTransmit(struct device *dev, BCM_POE_TYPE_PKTBUF_I2C_T *pkt);

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME:
 *      bcm_poe_halPoECtrlStateGet
 *
 *  DESCRIPTION :
 *      a API to get the state of PoE module ctrl in FPGA
 *
 *  INPUT :
 *      none
 *
 *  OUTPUT :
 *      state - disable/enable
 *
 *  RETURN :
 *      error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
s32 bcm_poe_halPoECtrlStateGet(struct device *dev, bool *state);

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME:
 *      bcm_poe_halPoECtrlStateSet
 *
 *  DESCRIPTION :
 *      a API to set the state of PoE module ctrl in FPGA
 *
 *  INPUT :
 *      state - disable/enable
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
s32 bcm_poe_halPoECtrlStateSet(struct device *dev, bool state);

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME:
 *      bcm_poe_halBcmI2cCmd
 *
 *  DESCRIPTION :
 *      a API to issue command to PoE MCU via I2c interface
 *
 *  INPUT :
 *      none
 *
 *  OUTPUT :
 *      pCmd   - Command request
 *      pResp  - Response
 *
 *  RETURN :
 *      bytes
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
u8 bcm_poe_halBcmI2cCmd(struct device *dev, const u8 *pReq, const u8 *pRsp);

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME:
 *      read_lpc_reg
 *
 *  DESCRIPTION :
 *      a API to get lpc register value
 *
 *  INPUT :
 *      dev - i2c device
 *      offset - register offset
 *
 *  OUTPUT :
 *
 *  RETURN :
 *      register value
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
u8 read_lpc_reg(struct device *dev, u16 offset);

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME:
 *      write_lpc_reg
 *
 *  DESCRIPTION :
 *      a API to set lpc register value
 *
 *  INPUT :
 *      dev - i2c device
 *      offset - register offset
 *      val - register value
 *
 *  OUTPUT :
 *
 *  RETURN :
 *      register value
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
u8 write_lpc_reg(struct device *dev, u16 offset, u8 val);

#endif /* __BCM_POE_HAL_H__ */
