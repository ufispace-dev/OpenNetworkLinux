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
#include <x86_64_ufispace_s9310_32d/x86_64_ufispace_s9310_32d_config.h>
#include "x86_64_ufispace_s9310_32d_log.h"
#include "platform_lib.h"

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
    for(p = 0; p < PORT_NUM; p++) {
        AIM_BITMAP_SET(bmap, p);
    }
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_is_present(int port)
{
    int status=1;
    
    //QSFPDD, SFP Ports
    if (port < QSFPDD_NUM) { //QSFPDD
        if (qsfpdd_present_get(port, &status) != ONLP_STATUS_OK) {
            AIM_LOG_ERROR("qsfpdd_presnet_get() failed, port=%d\n", port);
            return ONLP_STATUS_E_INTERNAL;
        }
    } else if ((port >= QSFPDD_NUM) && (port < PORT_NUM)) { //SFP
        if (sfp_present_get(port, &status) != ONLP_STATUS_OK) {
            AIM_LOG_ERROR("sfp_presnet_get() failed, port=%d\n", port);
            return ONLP_STATUS_E_INTERNAL;
        }
    } else { //unkonwn ports
        AIM_LOG_ERROR("unknown ports, port=%d\n", port);
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    return status;
}

int
onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int p = 0;
    int rc = 0;

    for (p = 0; p < PORT_NUM; p++) {
        rc = onlp_sfpi_is_present(p);
        AIM_BITMAP_MOD(dst, p, rc);
    }

    return ONLP_STATUS_OK;
}

int
qsfpdd_eeprom_read(int port, uint8_t data[256]) {
    char eeprom_path[128];
    int size = 0;

    memset(eeprom_path, 0, sizeof(eeprom_path));
    memset(data, 0, 256);

    snprintf(eeprom_path, sizeof(eeprom_path), 
                "/sys/bus/i2c/devices/%d-0050/eeprom", 
                QSFPDD_BASE_BUS+port);

    if(onlp_file_read(data, 256, &size, eeprom_path) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Unable to read eeprom from QSFPDD port(%d)\r\n", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }
    
    return ONLP_STATUS_OK;
}

int
sfp_cpu_eeprom_path_get(int port, char *eeprom_path, int size) {
    char cmd[128];
    int ret;

    if (port == 0) {
        snprintf(cmd, sizeof(cmd), CMD_SFP_EEPROM_GET, SFP_0_IFNAME, SFP_0_IFNAME);
        ret = system(cmd);
        if (ret == 0) {
            snprintf(eeprom_path, size, "/tmp/.sfp.%s.eeprom", SFP_0_IFNAME);
        } else {
            AIM_LOG_ERROR("Unable to get eeprom path for sfp port %d\n", port);
            return ONLP_STATUS_E_INTERNAL;
        }
    } else {
        snprintf(cmd, sizeof(cmd), CMD_SFP_EEPROM_GET, SFP_1_IFNAME, SFP_1_IFNAME);
        ret = system(cmd);
        if (ret == 0) {
            snprintf(eeprom_path, size, "/tmp/.sfp.%s.eeprom", SFP_1_IFNAME);
        } else {
            AIM_LOG_ERROR("Unable to get eeprom path for sfp port %d\n", port);
            return ONLP_STATUS_E_INTERNAL;
        }
    }

    return ONLP_STATUS_OK;
}

int
sfp_mac_eeprom_path_get(int port, char *eeprom_path, int size) {

    snprintf(eeprom_path, size, 
                "/sys/bus/i2c/devices/%d-0050/eeprom", 
                SFP_BASE_BUS+port);

    return ONLP_STATUS_OK;
}


int 
sfp_eeprom_read(int port, uint8_t data[256]) {
    int ret;
    char eeprom_path[256];
    int size = 0;

    memset(eeprom_path, 0, sizeof(eeprom_path));
    if(port < SFP_CPU_NUM) { // cpu sfp
        ret = sfp_cpu_eeprom_path_get(port, eeprom_path, sizeof(eeprom_path));
    } else { // mac sfp
        ret = sfp_mac_eeprom_path_get(port - SFP_CPU_NUM, eeprom_path, 
                sizeof(eeprom_path));
    }

    if (ret != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    } else {
        // read eeprom data
        // debug
        // AIM_LOG_INFO("[%s] eeprom_path=%s", __FUNCTION__, eeprom_path);
        if(onlp_file_read(data, 256, &size, eeprom_path) != ONLP_STATUS_OK) {
            AIM_LOG_ERROR("Unable to read eeprom from port %d\n", port);
            check_and_do_i2c_mux_reset(port);
            return ONLP_STATUS_E_INTERNAL;
        }
    }

    return ret;
}


/*
 * This function reads the SFPs idrom and returns in
 * in the data buffer provided.
 */
int
onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{   
    int ret;
     
    memset(data, 0, 256);
    //QSFP, QSFPDD, SFP real ports
    if (port >= 0 && port < QSFPDD_NUM) { //QSFPDD
        ret = qsfpdd_eeprom_read(port, data);
    } else if (port >= QSFPDD_NUM && port < PORT_NUM) { //sfp
        ret = sfp_eeprom_read(port - QSFPDD_NUM, data);
    } else {
        AIM_LOG_ERROR("unknown ports, port=%d\n", port);
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if(ret != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d)\r\n", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int i=0, value=0, rc=0;

    /* Populate bitmap - QSFPDD ports*/
    for(i = 0; i < QSFPDD_NUM; i++) {
        AIM_BITMAP_MOD(dst, i, 0);
    }

    /* Populate bitmap - SFP+ ports*/
    for(i = QSFPDD_NUM; i < PORT_NUM; i++) {
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
    uint8_t data[8];
    int data_len = 0, data_value = 0;
    char sysfs[128];
    int port_mask, port_index;
    int rc;
    
    //QSFPDD ports are not supported
    if (port < QSFPDD_NUM || port >= PORT_NUM) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    // sfp port number
    port_index = port - QSFPDD_NUM;
 
    //sysfs path
    if (control == ONLP_SFP_CONTROL_RX_LOS) {
        snprintf(sysfs, sizeof(sysfs), "%s/%s", MB_CPLD2_SYSFS_PATH, 
                    MB_CPLD_SFP_RXLOS_ATTR);
    } else if (control == ONLP_SFP_CONTROL_TX_FAULT) {
        snprintf(sysfs, sizeof(sysfs), "%s/%s", MB_CPLD2_SYSFS_PATH, 
                    MB_CPLD_SFP_TXFLT_ATTR);
    } else if (control == ONLP_SFP_CONTROL_TX_DISABLE) {
        snprintf(sysfs, sizeof(sysfs), "%s/%s", MB_CPLD2_SYSFS_PATH, 
                    MB_CPLD_SFP_TXDIS_ATTR);
    } else {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if ((rc = onlp_file_read(data, sizeof(data), &data_len, sysfs)) 
            != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("onlp_file_read failed, error=%d, %s", rc, sysfs);
        return ONLP_STATUS_E_INTERNAL;
    }

    //hex to int
    data_value = (int) strtol((char *)data, NULL, 0);

    // mask value
    port_mask = 0b00000001 << port_index;


    *value = (data_value & port_mask) >> port_index;
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_control_set(int port, onlp_sfp_control_t control, int value)
{

    uint8_t data[8];
    int data_len = 0, data_value = 0;
    char sysfs[128];
    int port_mask, port_index;
    int reg_val = 0;
    int rc;

    
    //QSFPDD ports are not supported
    if (port < QSFPDD_NUM || port >= PORT_NUM) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    // sfp port number
    port_index = port - QSFPDD_NUM;
 
    //sysfs path
    if (control == ONLP_SFP_CONTROL_TX_DISABLE) {
        snprintf(sysfs, sizeof(sysfs), "%s/%s", MB_CPLD2_SYSFS_PATH,
                    MB_CPLD_SFP_TXDIS_ATTR);
    } else {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if ((rc = onlp_file_read(data, sizeof(data), &data_len, sysfs)) 
            != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("onlp_file_read failed, error=%d, %s", rc, sysfs);
        return ONLP_STATUS_E_INTERNAL;
    }

    //hex to int
    data_value = (int) strtol((char *)data, NULL, 0);

    // mask value
    port_mask = 0b00000001 << port_index;

    // update reg val
    if (value == 1) {
        reg_val = data_value | port_mask;
    } else {
        reg_val = data_value & ~port_mask;
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
