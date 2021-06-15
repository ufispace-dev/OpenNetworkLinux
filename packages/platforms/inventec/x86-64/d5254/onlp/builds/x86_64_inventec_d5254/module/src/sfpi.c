/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2014 Accton Technology Corporation.
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *        http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 *
 * </bsn.cl>
 ************************************************************
 *
 *
 *
 ***********************************************************/
#include <onlp/platformi/sfpi.h>
#include <fcntl.h> /* For O_RDWR && open */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <onlplib/i2c.h>
#include "platform_lib.h"
#include <dirent.h>
#include <onlplib/file.h>

#define MAX_SFP_PATH 128
//static char sfp_node_path[MAX_SFP_PATH] = {0};

#define MUX_START_INDEX 10
#define QSFP_DEV_ADDR 0x50
#define NUM_OF_SFP_PORT 48
#define NUM_OF_QSFP_PORT 6
#define NUM_OF_ALL_PORT (NUM_OF_SFP_PORT+NUM_OF_QSFP_PORT)
#define FRONT_PORT_TO_MUX_INDEX(port) (port+MUX_START_INDEX)

/************************************************************
 *
 * SFPI Entry Points
 *
 ***********************************************************/

int
onlp_sfpi_port_is_valid(int port){
    if(port >= NUM_OF_ALL_PORT || port < 0)
        return 0;
    return 1;
}

static int
onlp_sfpi_port_type(int port)
{
    if(port >= 0 && port < NUM_OF_SFP_PORT) {
        return ONLP_SFP_TYPE_SFP;
    } else if(port >= NUM_OF_SFP_PORT && port < NUM_OF_ALL_PORT) {
        return ONLP_SFP_TYPE_QSFP;
    } else {
        AIM_LOG_ERROR("Invalid port(%d)\r\n", port);
        return ONLP_STATUS_E_PARAM;
    }
    return ONLP_STATUS_E_PARAM;
}

int
onlp_sfpi_get_file_byte(int port, char* attr){
    if(!onlp_sfpi_port_is_valid(port)){
        return -1;
    }
    char path[128]={0};
    int err = snprintf(path, sizeof(path), "/sys/class/swps/port%d/%s", port, attr);
    if( err < 0){
        return err;
    }
    FILE* pFile = fopen(path, "r");
    if(pFile == NULL){
        return ONLP_STATUS_E_UNSUPPORTED;
    }
    char buf[8] = {0};
    fread( buf, sizeof(buf), sizeof(buf), pFile );
    int ret = strtol (buf, NULL, 10);
    fclose(pFile);
    return ret;
}

int
onlp_sfpi_set_file_byte(int port, char* attr, int value){
    if(!onlp_sfpi_port_is_valid(port)){
        return -1;
    }
    if(value > 10 || value < 0){
        return -1;
    }
    char path[128]={0};
    int err = snprintf(path, sizeof(path), "/sys/class/swps/port%d/%s", port, attr);
    if( err < 0){
        return err;
    }
    FILE* pFile = fopen(path, "r+");
    if(pFile == NULL){
        return ONLP_STATUS_E_UNSUPPORTED;
    }
    char buf = 0;
    buf = value+'0';
    err = fwrite(&buf, sizeof(buf), sizeof(buf), pFile);
    if(err < 0){
        return err;
    }
    fclose(pFile);
    return err;
}

int
onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{
    /*
     * Ports {0, 53}
     */
    int p;
    AIM_BITMAP_CLR_ALL(bmap);

    for(p = 0; p < NUM_OF_ALL_PORT; p++) {
        AIM_BITMAP_SET(bmap, p);
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_is_present(onlp_oid_id_t port)
{
    /*
     * Return 1 if present.
     * Return 0 if not present.
     * Return < 0 if error.
     */
    int present = -999;
    present = onlp_sfpi_get_file_byte(port, "present");
    if(present >= 0){
        return (-present)+1;
    }
    return present;
}

int
onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    AIM_BITMAP_CLR_ALL(dst);
    int port=MUX_START_INDEX;
    for(port=MUX_START_INDEX;port<=NUM_OF_ALL_PORT;port++){
        if(onlp_sfpi_is_present(port))
            AIM_BITMAP_SET(dst, port);
    }
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_is_rx_los(int port)
{
    if(port <= NUM_OF_SFP_PORT){
        if (onlp_sfpi_is_present(port) == 1){
            int rxlos = onlp_sfpi_get_file_byte(port, "rxlos");
            if(rxlos < 0){
                AIM_LOG_ERROR("Unable to read rxlos from port(%d)\r\n", port);
                return ONLP_STATUS_E_INTERNAL;
            }
            return rxlos;
        }
        return 0;
    }
    else if(port > NUM_OF_SFP_PORT){
        return 0;
    }
    AIM_LOG_ERROR("Read rxlos from port(%d) out of range.\r\n", port);
    return ONLP_STATUS_E_INTERNAL;
}

int
onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    AIM_BITMAP_CLR_ALL(dst);
    int port;
    int isrxlos;
    for(port = 0; port < NUM_OF_ALL_PORT; port++) {
        if(onlp_sfpi_is_present(port) == true) {
            isrxlos = onlp_sfpi_is_rx_los(port);
            if(isrxlos == true) {
                AIM_BITMAP_MOD(dst, port, 1);
            } else if(isrxlos == false) {
                AIM_BITMAP_MOD(dst, port, 0);
            } else {
                return ONLP_STATUS_E_INTERNAL;
            }
        }
    }
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{
    /*
     * Read the SFP eeprom into data[]
     *
     * Return MISSING if SFP is missing.
     * Return OK if eeprom is read
     */
    memset(data, 0, 256);

    if(onlp_sfpi_port_type(port) < 0) { return ONLP_STATUS_E_INVALID; }
    int sts;
    int bus = FRONT_PORT_TO_MUX_INDEX(port);
    sts = onlp_i2c_read(bus, QSFP_DEV_ADDR, 0, 256, data, ONLP_I2C_F_FORCE);
    if(sts < 0) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d)\r\n", port);
        return ONLP_STATUS_E_MISSING;
    }
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_dev_read(onlp_oid_id_t port, int devaddr, int addr,
                   uint8_t* dst, int len)
{
    int bus = FRONT_PORT_TO_MUX_INDEX(port);
    /* Can this be block_read? */
    return onlp_i2c_read(bus, devaddr, addr, len, dst, ONLP_I2C_F_FORCE);
}

int
onlp_sfpi_control_supported(onlp_oid_id_t port, onlp_sfp_control_t control, int* rv)
{
    switch (control) {
        case ONLP_SFP_CONTROL_RESET_STATE:
            if(port >= NUM_OF_SFP_PORT && port < (NUM_OF_SFP_PORT + NUM_OF_QSFP_PORT)){
                *rv = 1;
            }
            else{
                *rv = 0;
            }
            break;
        case ONLP_SFP_CONTROL_RX_LOS:
            *rv = 0;
            break;
        case ONLP_SFP_CONTROL_TX_DISABLE:
            if(port <= NUM_OF_SFP_PORT){
                *rv = 1;
            }
            else if(port >= NUM_OF_SFP_PORT && port < (NUM_OF_SFP_PORT + NUM_OF_QSFP_PORT)){
                *rv = 0;
            }
            break;
        case ONLP_SFP_CONTROL_LP_MODE:
        if(port >= NUM_OF_SFP_PORT && port < (NUM_OF_SFP_PORT + NUM_OF_QSFP_PORT)){
                *rv = 1;
            }
            else{
                *rv = 0;
            }
            break;
        default:
            break;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_control_set(onlp_oid_id_t port, onlp_sfp_control_t control, int value)
{
    int ret_val = 0;
    int err = 0;
    switch (control) {
        case ONLP_SFP_CONTROL_RESET_STATE:
            err = onlp_sfpi_set_file_byte(port, "reset", value);
            if(err == ONLP_STATUS_E_UNSUPPORTED){
                ret_val = ONLP_STATUS_E_UNSUPPORTED;
                break;
            }
            ret_val = ONLP_STATUS_OK;
            break;
        case ONLP_SFP_CONTROL_RX_LOS:
            ret_val = ONLP_STATUS_E_UNSUPPORTED;
            break;
        case ONLP_SFP_CONTROL_TX_DISABLE:
            err = onlp_sfpi_set_file_byte(port, "tx_disable", value);
            if(err == ONLP_STATUS_E_UNSUPPORTED){
                ret_val = ONLP_STATUS_E_UNSUPPORTED;
                break;
            }
            ret_val = ONLP_STATUS_OK;
            break;
        case ONLP_SFP_CONTROL_LP_MODE:
            err = onlp_sfpi_set_file_byte(port, "lpmod", value);
            if(err == ONLP_STATUS_E_UNSUPPORTED){
                ret_val = ONLP_STATUS_E_UNSUPPORTED;
                break;
            }
            ret_val = ONLP_STATUS_OK;
            break;
        default:
            ret_val = ONLP_STATUS_E_UNSUPPORTED;
            break;
    }
    return ret_val;
}

int
onlp_sfpi_control_get(onlp_oid_id_t port, onlp_sfp_control_t control, int* value)
{
    int ret_val = 0;
    int err = 0;

    switch (control) {
        case ONLP_SFP_CONTROL_RESET_STATE:
            err = onlp_sfpi_get_file_byte(port, "reset");
            if(err == ONLP_STATUS_E_UNSUPPORTED){
                ret_val = ONLP_STATUS_E_UNSUPPORTED;
                break;
            }
            *value = err;
            ret_val = ONLP_STATUS_OK;
            break;
        case ONLP_SFP_CONTROL_RX_LOS:
            err = onlp_sfpi_get_file_byte(port, "rxlos");
            if(err == ONLP_STATUS_E_UNSUPPORTED){
                ret_val = ONLP_STATUS_E_UNSUPPORTED;
                break;
            }
            *value = err;
            ret_val = ONLP_STATUS_OK;
            break;
        case ONLP_SFP_CONTROL_TX_DISABLE:
            err = onlp_sfpi_get_file_byte(port, "tx_disable");
            if(err == ONLP_STATUS_E_UNSUPPORTED){
                ret_val = ONLP_STATUS_E_UNSUPPORTED;
                break;
            }
            *value = err;
            ret_val = ONLP_STATUS_OK;
            break;
        case ONLP_SFP_CONTROL_LP_MODE:
            err = onlp_sfpi_get_file_byte(port, "lpmod");
            if(err == ONLP_STATUS_E_UNSUPPORTED){
                ret_val = ONLP_STATUS_E_UNSUPPORTED;
                break;
            }
            *value = err;
            ret_val = ONLP_STATUS_OK;
            break;
        case ONLP_SFP_CONTROL_TX_FAULT:
            err = onlp_sfpi_get_file_byte(port, "tx_fault");
            if(err == ONLP_STATUS_E_UNSUPPORTED){
                ret_val = ONLP_STATUS_E_UNSUPPORTED;
                break;
            }
            *value = err;
            ret_val = ONLP_STATUS_OK;
            break;
        default:
            ret_val = ONLP_STATUS_E_UNSUPPORTED;
            break;
    }
    return ret_val;
}
