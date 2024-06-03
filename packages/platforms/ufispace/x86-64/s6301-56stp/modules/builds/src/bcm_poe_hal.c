/* A poe hal file for the ufispace_s6301_56stp poe driver
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

/* System Library */

/* User-defined Library */
#include "bcm_poe_hal.h"
#include "x86-64-ufispace-s6301-56stp-poe-module.h"

/*==========================================================================
 *
 *      Marco Definition Segment
 *
 *==========================================================================
 */

/*==========================================================================
 *
 *      Structrue Definition segment
 *
 *==========================================================================
 */

/*==========================================================================
 *
 *      Static Variable segment
 *
 *==========================================================================
 */
static u8 seqNo = 0x00;
u8  CmdBuffer[I2C_MSG_LEN];
u8  RespBuffer[I2C_MSG_LEN];

/*==========================================================================
 *                                                                          
 *      Function Definition Segment
 *                                                                          
 *==========================================================================
 */


/*==========================================================================
 *
 *      Local Function segment
 *
 *==========================================================================
 */

/*==========================================================================
 *
 *      External Funtion segment
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
void bcm_poe_halInsertCheckSum(BCM_POE_TYPE_PKTBUF_I2C_T *pCmd)
{
    if (pCmd->command != BCM_POE_TYPE_COM_IMAGE_UPGRADE)
    {
        pCmd->seqNo = seqNo++;
    }
    pCmd->checksum = pCmd->command + pCmd->seqNo + pCmd->data1 +
                       pCmd->data2 + pCmd->data3 + pCmd->data4 +
                       pCmd->data5 + pCmd->data6 + pCmd->data7 +
                       pCmd->data8 + pCmd->data9;
}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      bcm_poe_halCheckSumCmp
 *
 *  DESCRIPTION :
 *      a local API to verify the check sum of a received frame
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

bool bcm_poe_halCheckSumCmp(BCM_POE_TYPE_PKTBUF_I2C_T *pCmd)
{
    u8 ck;
    ck= pCmd->command + pCmd->seqNo + pCmd->data1 +
                       pCmd->data2 + pCmd->data3 + pCmd->data4 +
                       pCmd->data5 + pCmd->data6 + pCmd->data7 +
                       pCmd->data8 + pCmd->data9;

    if(ck==pCmd->checksum){
        return true;
    }
    return false;
}


/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME :
 *      halSCPPktPrint
 *
 *  DESCRIPTION :
 *      a local API to print the content of a I2C packet
 *
 *
 *  INPUT :
 *      *pkt - points to a I2C packet
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
static void halI2CPktPrint(BCM_POE_TYPE_PKTBUF_I2C_T *pkt)
{
    if (pkt->command == BCM_POE_TYPE_COM_IMAGE_UPGRADE)
    {
        BSP_LOG_R("+-------+-------+------+------+------+------+------+------+------+------+---------+\n");
        BSP_LOG_R("|Command| subCmd|DATA0 |DATA1 |DATA2 |DATA3 |DATA4 |DATA5 |DATA6 |DATA7 |DATA8 |CheckSum |\n");
        BSP_LOG_R("|0x%02x | 0x%02x|0x%02x|0x%02x|0x%02x|0x%02x|0x%02x|0x%02x|0x%02x|0x%02x|0x%02x|0x%02x|\n",
        pkt->command,pkt->seqNo,pkt->data1,pkt->data2,pkt->data3,pkt->data4,pkt->data5,pkt->data6,
        pkt->data7,pkt->data8,pkt->data9,pkt->checksum);
        BSP_LOG_R("+-------+-------+------+------+------+------+------+------+------+------+---------+\n");
    }
    else
    {
        BSP_LOG_R("+-------+-------+------+------+------+------+------+------+------+------+---------+\n");
        BSP_LOG_R("|Command| Seq    |DATA0 |DATA1 |DATA2 |DATA3 |DATA4 |DATA5 |DATA6 |DATA7 |DATA8 | CheckSum |\n");
        BSP_LOG_R("|0x%-5.2x| 0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|0x%-4.2x|\n",
        pkt->command,pkt->seqNo,pkt->data1,pkt->data2,pkt->data3,pkt->data4,pkt->data5,pkt->data6,
        pkt->data7,pkt->data8,pkt->data9,pkt->checksum);
        BSP_LOG_R("+-------+-------+------+------+------+------+------+------+------+------+---------+\n");

    }
}

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
 *      max_wait - before receiving a SCP packet via i2c, how long will it hold on.
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
E_BCM_SCP_RET_CODE bcm_poe_halSCPPktReceive(struct device *dev, BCM_POE_TYPE_PKTBUF_I2C_T *pkt)
{
    int ret = E_BCM_SCP_SUCCESS;
    u8 *data;
    u8 reg, len;

    struct i2c_client *client = to_i2c_client(dev);
    struct poe_data *clientdata = i2c_get_clientdata(client);

    reg = 0;
    data = (u8 *)pkt;
    len = I2C_MSG_LEN;

    I2C_READ_BLOCK_DATA(ret, &clientdata->access_lock, client, reg, len, data)
    if(unlikely(ret < 0)) 
    {
        dev_err(dev, "bcm_poe_halSCPPktReceive() error, return=%d\n", ret);
        return E_BCM_SCP_STATUS_RECEIVE_ERROR;
    }

    if(bcm_poe_halCheckSumCmp(pkt))
    {
        return E_BCM_SCP_SUCCESS; 
    }
    else
    {
        BCM_POE_DBG_CHECK(log_printf("Checksum error: %x !\n", pkt->checksum));
        return E_BCM_SCP_CHECKSUM_ERROR;
    }
}

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
E_BCM_SCP_RET_CODE bcm_poe_halSCPPktTransmit(struct device *dev, BCM_POE_TYPE_PKTBUF_I2C_T *pkt)
{
    int ret = E_BCM_SCP_SUCCESS;
    u8 *data;
    u8 reg, len;
    u8 *upkt = (u8 *)pkt;

    struct i2c_client *client = to_i2c_client(dev);
    struct poe_data *clientdata = i2c_get_clientdata(client);

    /* get command for i2c regsiter */
    reg = upkt[0] & 0xff;
    /* remove command in data and data len */
    data = upkt + 1;
    len = I2C_MSG_LEN - 1;

    I2C_WRITE_BLOCK_DATA(ret, &clientdata->access_lock, client, reg, len, data)

    if(unlikely(ret < 0)) {
        dev_err(dev, "bcm_poe_halSCPPktTransmit() error, return=%d\n", ret);
        return E_BCM_SCP_STATUS_TRANSMIT_ERROR;
    }
    return E_BCM_SCP_SUCCESS;

}

/*--------------------------------------------------------------------------
 *
 *  FUNCTION NAME:
 *      bcm_poe_halBcmI2cCmd
 *
 *  DESCRIPTION :
 *      a API to issue command to PoE MCU via I2c interface
 *
 *  INPUT :
 *      dev - i2c device
 *
 *  OUTPUT :
 *      pCmd   - Command request
 *      pResp  - Response
 *
 *  RETURN :
 *      E_BCM_SCP_RET_CODE - error code
 *
 *  COMMENT :
 *      none
 *
 *--------------------------------------------------------------------------
 */
u8 bcm_poe_halBcmI2cCmd(struct device *dev, const u8 *pReq, const u8 *pRsp)
{
    E_BCM_SCP_RET_CODE result;

    if ((result = bcm_poe_halSCPPktTransmit(dev, (BCM_POE_TYPE_PKTBUF_I2C_T *)pReq)) == E_BCM_SCP_SUCCESS)
    {
        /*minimum delay between request frame and the response poll for PoE subsystem is 20 ms */
        mdelay(20);
        if ((result = bcm_poe_halSCPPktReceive(dev, (BCM_POE_TYPE_PKTBUF_I2C_T *)pRsp)) == E_BCM_SCP_SUCCESS)
        {
            halI2CPktPrint((BCM_POE_TYPE_PKTBUF_I2C_T *)pReq);
            halI2CPktPrint((BCM_POE_TYPE_PKTBUF_I2C_T *)pRsp);
        }
        if (result == E_BCM_SCP_TIMEOUT_ERROR)/* Host assumes packet loss */
        {
            dev_info(dev, "No echo packet from POE module, Re-transmit again.\n");

            /* Host transmitted Command or Request 2nd try */
            if ((result = bcm_poe_halSCPPktTransmit(dev, (BCM_POE_TYPE_PKTBUF_I2C_T *)pReq)) == E_BCM_SCP_SUCCESS)
            {
                /*minimum delay between request frame and the response poll for PoE subsystem is 20 ms */
                mdelay(20);
                if ((result = bcm_poe_halSCPPktReceive(dev, (BCM_POE_TYPE_PKTBUF_I2C_T *)pRsp)) == E_BCM_SCP_SUCCESS)
                {
                    halI2CPktPrint((BCM_POE_TYPE_PKTBUF_I2C_T *)pReq);
                    halI2CPktPrint((BCM_POE_TYPE_PKTBUF_I2C_T *)pRsp);    
                }        
            }
        }      
    }

    return result;
}

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
u8 read_lpc_reg(struct device *dev, u16 offset)
{
    u8 val;
    u16 reg;

    struct i2c_client *client = to_i2c_client(dev);
    struct poe_data *clientdata = i2c_get_clientdata(client);

    reg = offset + REG_BASE_MB;
    
    mutex_lock(&clientdata->access_lock);
    val = inb(reg);
    mutex_unlock(&clientdata->access_lock);
    return val;
}

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
u8 write_lpc_reg(struct device *dev, u16 offset, u8 val)
{
    u16 reg;

    struct i2c_client *client = to_i2c_client(dev);
    struct poe_data *clientdata = i2c_get_clientdata(client);

    reg = offset + REG_BASE_MB;

    mutex_lock(&clientdata->access_lock);
    outb(val, reg);
    mutex_unlock(&clientdata->access_lock);
    mdelay(5);

    return val;
}