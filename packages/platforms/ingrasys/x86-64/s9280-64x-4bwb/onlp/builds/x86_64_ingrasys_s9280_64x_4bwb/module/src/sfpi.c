/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *        Copyright 2014, 2015 Big Switch Networks, Inc.
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
 ***********************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <onlplib/sfp.h>
#include <onlplib/file.h>
#include <onlp/platformi/sfpi.h>
#include <x86_64_ingrasys_s9280_64x_4bwb/x86_64_ingrasys_s9280_64x_4bwb_config.h>
#include "x86_64_ingrasys_s9280_64x_4bwb_log.h"
#include "platform_lib.h"

static int _fp2phy_port_mapping[64] = {
    0,  1,  4,  5,  8,
    9, 12, 13, 16, 17,
   20, 21, 24, 25, 28,
   29, 32, 33, 36, 37,
   40, 41, 44, 45, 48,
   49, 52, 53, 56, 57,
   60, 61,  2,  3,  6,
    7, 10, 11, 14, 15,
   18, 19, 22, 23, 26,
   27, 30, 31, 34, 35,
   38, 39, 42, 43, 46,
   47, 50, 51, 54, 55,
   58, 59, 62, 63};

static void
qsfp_to_cpld_index(int phy_port, int *cpld_id, int *cpld_port_index)
{
    if (phy_port < CPLD1_PORTS) { 
        *cpld_id = 0;
        *cpld_port_index = phy_port + 1;
    } else {
        *cpld_id = 1 + (phy_port - CPLD1_PORTS) / CPLDx_PORTS;
        *cpld_port_index = ((phy_port - CPLD1_PORTS) % CPLDx_PORTS) + 1;
    }
    return;
}


int
onlp_sfpi_init(void)
{
    lock_init();
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{
    int p;
    for(p = 1; p <= PORT_NUM; p++) {
        AIM_BITMAP_SET(bmap, p);
    }
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_is_present(int port)
{
    int status, phy_port;
    int i2c_id, cpld_id, cpld_port_index;
    char sysfs[128];
    int value, mask;
    int rc;

    if (port >= 1 && port <=64) {
        phy_port = _fp2phy_port_mapping[port-1];
        qsfp_to_cpld_index(phy_port, &cpld_id, &cpld_port_index);

        i2c_id = CPLD_BUS_BASE + cpld_id;
        mask = 1 << CPLD_PRES_BIT;

        snprintf(sysfs, 128, CPLD_QSFP_REG_PATH, i2c_id, CPLD_I2C_ADDR, 
            MB_CPLD_QSFP_PORT_STATUS_ATTR, cpld_port_index);
    } else if (port>= 65 && port <= 66) {
        cpld_port_index = 0;
        i2c_id = CPLD_BUS_BASE;

        if (port == 65) {
            mask = 1 << CPLD_SFP1_PRES_BIT;
        } else {
            mask = 1 << CPLD_SFP2_PRES_BIT;
        }

        snprintf(sysfs, 128, CPLD_SFP_REG_PATH, i2c_id, CPLD_I2C_ADDR, 
            MB_CPLD_SFP_PORT_STATUS_ATTR);
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    if ((rc = file_read_hex(&value, sysfs)) == ONLP_STATUS_OK) {
        if ( (value & mask) == 0) {
            status = 1;
        } else {
            status = 0;
        }
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    return status;
}


int
onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int p = 1;
    int rc = 0;

    for (p = 1; p <= PORT_NUM; p++) {
        rc = onlp_sfpi_is_present(p);
        AIM_BITMAP_MOD(dst, p, (1 == rc) ? 1 : 0);
    }

    return ONLP_STATUS_OK;
}

/*
 * This function reads the SFPs idrom and returns in
 * in the data buffer provided.
 */
int
onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{   
    int eeprombus=0, eeprombusbase=0, phy_port=0, port_group=0, eeprombusshift=0;
    char eeprom_path[512], eeprom_addr[32];
    memset(eeprom_path, 0, sizeof(eeprom_path));    
    memset(eeprom_addr, 0, sizeof(eeprom_addr));    
    aim_strlcpy(eeprom_addr, "0050", sizeof(eeprom_addr));    
    
    memset(data, 0, 256);
    
    if (port >=1 && port <= 64) {
        phy_port = _fp2phy_port_mapping[port-1] + 1;
        port_group = (phy_port-1)/8;
        eeprombusbase = 29 + (port_group * 8);
        eeprombusshift = (phy_port-1)%8;
        eeprombus = eeprombusbase + eeprombusshift;
    } else if (port == 65 ){
        eeprombus = 17;
    } else if (port == 66 ){
        eeprombus = 18;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }
    
    snprintf(eeprom_path, sizeof(eeprom_path), 
             "/sys/bus/i2c/devices/%d-%s/eeprom", eeprombus, eeprom_addr);

    if (onlplib_sfp_eeprom_read_file(eeprom_path, data) != 0) {
        return ONLP_STATUS_E_INTERNAL;
    } 

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int i=0, value=0, rc=0;

    /* Populate bitmap - QSFP ports*/
    for(i = 1; i <= QSFP_NUM; i++) {
        AIM_BITMAP_MOD(dst, i, 0);
    }

    /* Populate bitmap - SFP+ ports*/
    for(i = QSFP_NUM + 1; i <= PORT_NUM; i++) {
        if ((rc=onlp_sfpi_control_get(i, ONLP_SFP_CONTROL_RX_LOS, &value)) 
                != ONLP_STATUS_OK) {
            return ONLP_STATUS_E_INTERNAL;
        } else {
            AIM_BITMAP_MOD(dst, i, value);
        }
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_control_get(int port, onlp_sfp_control_t control, int* value)
{
    int data_val, mask, shift, port_index;
    char sysfs[128];
    int rc;
    
    //QSFP ports are not supported
    if (port <= QSFP_NUM || port > PORT_NUM) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    // sfp port number
    port_index = port - QSFP_NUM - 1;
 
    //sysfs path
    if (control == ONLP_SFP_CONTROL_RX_LOS) {
        snprintf(sysfs, sizeof(sysfs), MB_CPLD_SYSFS_PATH"/"MB_CPLD_SFP_PORT_STATUS_ATTR, 
            I2C_BUS_CPLD1, CPLD_I2C_ADDR);
        mask = 0b00000100 << (port_index*4);
        shift = 2 + (port_index*4);
        
    } else if (control == ONLP_SFP_CONTROL_TX_FAULT) {
        snprintf(sysfs, sizeof(sysfs), MB_CPLD_SYSFS_PATH"/"MB_CPLD_SFP_PORT_STATUS_ATTR, 
            I2C_BUS_CPLD1, CPLD_I2C_ADDR);
        mask = 0b00000010 << (port_index*4);
        shift = 1 + (port_index*4);;

    } else if (control == ONLP_SFP_CONTROL_TX_DISABLE) {
        snprintf(sysfs, sizeof(sysfs), MB_CPLD_SYSFS_PATH"/"MB_CPLD_SFP_PORT_CONFIF_ATTR, 
            I2C_BUS_CPLD1, CPLD_I2C_ADDR);
        mask = 0b00000001 << (port_index*4);
        shift = 0 + (port_index*4);
    } else {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if ((rc = file_read_hex(&data_val, sysfs)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("file_read_hex failed, error=%d, %s", rc, sysfs);
        return ONLP_STATUS_E_INTERNAL;
    }

    *value = (data_val & mask) >> shift;
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_control_set(int port, onlp_sfp_control_t control, int value)
{

    int data_val, mask, port_index;
    char sysfs[128];
    int reg_val;
    int rc;

    
    //QSFP ports are not supported
    if (port <= QSFP_NUM || port > PORT_NUM) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    // sfp port number
    port_index = port - QSFP_NUM - 1;

    if (control == ONLP_SFP_CONTROL_TX_DISABLE) {
        snprintf(sysfs, sizeof(sysfs), MB_CPLD_SYSFS_PATH"/"MB_CPLD_SFP_PORT_CONFIF_ATTR, 
            I2C_BUS_CPLD1, CPLD_I2C_ADDR);
        mask = 0b00000001 << (port_index*4);
    } else {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if ((rc = file_read_hex(&data_val, sysfs)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("file_read_hex failed, error=%d, %s", rc, sysfs);
        return ONLP_STATUS_E_INTERNAL;
    }

    // update reg val
    if (value == 1) {
        reg_val = data_val | mask;
    } else {
        reg_val = data_val & ~mask;
    }

    // write reg val back
    if ((rc = onlp_file_write_int(reg_val, sysfs)) 
            != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Unable to write tx disable value, error=%d, \
                                    sysfs=%s, reg_val=%x", rc, sysfs, reg_val);
        return ONLP_STATUS_E_INTERNAL;
    }
    
    return ONLP_STATUS_OK;
}


/*
 * De-initialize the SFPI subsystem.
 */
int
onlp_sfpi_denit(void)
{
    return ONLP_STATUS_OK;
}
