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
#include <onlp/onlp.h>
#include <onlplib/file.h>
#include <onlplib/i2c.h>
#include "platform_lib.h"

#define EEPROM_SYS_FMT "/sys/bus/i2c/devices/%d-0050/eeprom" 
#define SUPPORT_CPU_SFP_PRES 1

typedef enum port_type_e {
    TYPE_QSFPDD = 0,
    TYPE_CPU_SFP,
    TYPE_MAC_SFP,
    TYPE_UNNKOW,
    TYPE__MAX,
} port_type_t;

static char* port_type_str[] = {
    "qsfpdd",
    "cpu sfp",
    "mac sfp",
    "mgmt sfp",
};

typedef struct port_type_info_s
{
    port_type_t type;  // port module type
    int port_index;  // port index in group, 0 based
    int port_group;  // port group for cpld access
    int port_mask;  // mask for port info value
    int eeprom_bus;  // i2c bus number of eerpom
}port_type_info_t;

port_type_info_t platform_get_port_info(int port) 
{
    port_type_info_t port_type_info;
    
    if (port < QSFPDD_NUM) { //QSFPDD
        port_type_info.type = TYPE_QSFPDD;
        port_type_info.port_index = port % 8;
        port_type_info.port_group = port / 8;
        port_type_info.port_mask = 0b00000001 << port_type_info.port_index;
        port_type_info.eeprom_bus = QSFPDD_BASE_BUS + port;
    } else if ((port >= QSFPDD_NUM) && (port < (QSFPDD_NUM+SFP_CPU_NUM))) { //CPU SFP
        port_type_info.type = TYPE_CPU_SFP;
        port_type_info.port_index = port - QSFPDD_NUM;
        port_type_info.port_group = 0;
        port_type_info.port_mask = 0b00000001 << port_type_info.port_index;
        port_type_info.eeprom_bus = 0;
    } else if ((port >= (QSFPDD_NUM+SFP_CPU_NUM)) &&  (port < PORT_NUM)) { //MAC SFP
        port_type_info.type = TYPE_MAC_SFP;
        port_type_info.port_index = port - QSFPDD_NUM;
        port_type_info.port_group = 0;
        port_type_info.port_mask = 0b00000001 << port_type_info.port_index;
        port_type_info.eeprom_bus = SFP_BASE_BUS + (port_type_info.port_index -SFP_CPU_NUM);
    } else { //unkonwn ports
        AIM_LOG_ERROR("port_num mapping to type fail, port=%d", port);
        port_type_info.type = TYPE_UNNKOW;
    }
    return port_type_info;
}

int
onlp_sfpi_init(void)
{  
    lock_init();
    return ONLP_STATUS_OK;
}

int onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{
    int p;
    for(p = 0; p < PORT_NUM; p++) {
        AIM_BITMAP_SET(bmap, p);
    }
    return ONLP_STATUS_OK;
}

int qsfpdd_present_get(port_type_info_t *port_info, int *pres_val)
{     
    int rc;
    uint8_t data[8];
    int data_len;
    int group_pres;
    int port_pres;
    char sysfs[128];
   
    memset(data, 0, sizeof(data));

    snprintf(sysfs, sizeof(sysfs), MB_CPLD2_SYSFS_PATH"/"MB_CPLD_QSFPDD_PRES_ATTR,
        port_info->port_group);
    if ((rc = onlp_file_read(data, sizeof(data), &data_len, sysfs))
         != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("onlp_file_read failed, error=%d, sysfs=%s", rc, sysfs);
        return ONLP_STATUS_E_INTERNAL;
    }
    group_pres = (int) strtol((char *)data, NULL, 0);
    // val 0 for presence, pres_val set to 1
    port_pres = (group_pres & port_info->port_mask) >> port_info->port_index;
    if (port_pres) {
        *pres_val = 0;
    } else {
        *pres_val = 1;
    }
   
    return ONLP_STATUS_OK;
}

int mac_sfp_present_get(port_type_info_t *port_info, int *pres_val) 
{
    uint8_t data[8];
    int data_len;
    int group_pres;
    int port_pres;
    char sysfs[128];
    int rc;

    snprintf(sysfs, sizeof(sysfs), "%s/%s", MB_CPLD2_SYSFS_PATH, 
        MB_CPLD_SFP_ABS_ATTR);
    if ((rc=onlp_file_read(data, sizeof(data), &data_len, sysfs)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("onlp_file_read failed, error=%d, %s", rc, sysfs);
        return ONLP_STATUS_E_INTERNAL;
    }
    group_pres = (int) strtol((char *)data, NULL, 0);
    // val 0 for presence, pres_val set to 1
    port_pres = (group_pres & port_info->port_mask) >> port_info->port_index;
    if (port_pres) {
        *pres_val = 0;
    } else {
        *pres_val = 1;
    }

    return ONLP_STATUS_OK;
}

int cpu_sfp_present_get(port_type_info_t *port_info, int *pres_val) 
{
    int ret;
    char cmd[128];

    #ifdef SUPPORT_CPU_SFP_PRES
    return mac_sfp_present_get(port_info, pres_val);
    #endif
 
    if (port_info->port_index == 0) {
        snprintf(cmd, sizeof(cmd), CMD_SFP_PRES_GET, SFP_0_IFNAME);
        // debug
        //AIM_LOG_INFO("[%s] cmd=%s", __FUNCTION__, cmd);
        ret = system(cmd);
        *pres_val = (ret==0) ? 1 : 0;
    } else if (port_info->port_index == 1) {
        snprintf(cmd, sizeof(cmd), CMD_SFP_PRES_GET, SFP_1_IFNAME);
        // debug
        //AIM_LOG_INFO("[%s] cmd=%s", __FUNCTION__, cmd);
        ret = system(cmd);
        *pres_val = (ret==0) ? 1 : 0;
    } else {
        AIM_LOG_ERROR("unknow cpu sfp port, port=%d", port_info->port_index);
        return ONLP_STATUS_E_INTERNAL;
    }   

    return ONLP_STATUS_OK;
}

int onlp_sfpi_is_present(int port)
{
    int status=1;
    port_type_info_t port_type_info;
    port_type_info = platform_get_port_info(port);

    switch(port_type_info.type) {
        case TYPE_QSFPDD:
            if (qsfpdd_present_get(&port_type_info, &status) != ONLP_STATUS_OK) {
                AIM_LOG_ERROR("qsfpdd_presnet_get() failed, port=%d", port);
                return ONLP_STATUS_E_INTERNAL;
            }
            break;
        case TYPE_CPU_SFP:
            if (cpu_sfp_present_get(&port_type_info, &status) != ONLP_STATUS_OK) {
                AIM_LOG_ERROR("mac_sfp_presnet_get() failed, port=%d", port);
                return ONLP_STATUS_E_INTERNAL;
            }
            break;
        case TYPE_MAC_SFP:
            if (mac_sfp_present_get(&port_type_info, &status) != ONLP_STATUS_OK) {
                AIM_LOG_ERROR("mac_sfp_presnet_get() failed, port=%d", port);
                return ONLP_STATUS_E_INTERNAL;
            }
            break;
        default:
            AIM_LOG_ERROR("invalid port type for port(%d)", port);;
            return ONLP_STATUS_E_PARAM;
    }

    return status;
}

int onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int p = 0;
    int rc = 0;

    for (p = 0; p < PORT_NUM; p++) {
        rc = onlp_sfpi_is_present(p);
        AIM_BITMAP_MOD(dst, p, rc);
    }

    return ONLP_STATUS_OK;
}

int sfp_cpu_eeprom_path_get(int port, char *eeprom_path, int size) {
    char cmd[128];
    int ret;

    if (port == 0) {
        snprintf(cmd, sizeof(cmd), CMD_SFP_EEPROM_GET, SFP_0_IFNAME, SFP_0_IFNAME);
        ret = system(cmd);
        if (ret == 0) {
            snprintf(eeprom_path, size, "/tmp/.sfp.%s.eeprom", SFP_0_IFNAME);
        } else {
            AIM_LOG_ERROR("Unable to get eeprom path for sfp port %d", port);
            return ONLP_STATUS_E_INTERNAL;
        }
    } else {
        snprintf(cmd, sizeof(cmd), CMD_SFP_EEPROM_GET, SFP_1_IFNAME, SFP_1_IFNAME);
        ret = system(cmd);
        if (ret == 0) {
            snprintf(eeprom_path, size, "/tmp/.sfp.%s.eeprom", SFP_1_IFNAME);
        } else {
            AIM_LOG_ERROR("Unable to get eeprom path for sfp port %d", port);
            return ONLP_STATUS_E_INTERNAL;
        }
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Read the SFP EEPROM.
 * @param port The port number.
 * @param data Receives the SFP data. 
 */
int
onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{   
    int ret = ONLP_STATUS_OK;
    char eeprom_path[256];
    int size = 0;
    port_type_info_t port_type_info;
    port_type_info = platform_get_port_info(port);

    memset(data, 0, 256);
    memset(eeprom_path, 0, sizeof(eeprom_path));

    // get eeprom sysfs path
    switch(port_type_info.type) {
        case TYPE_QSFPDD:
        case TYPE_MAC_SFP:
            snprintf(eeprom_path, sizeof(eeprom_path), EEPROM_SYS_FMT, port_type_info.eeprom_bus);
            break;
        case TYPE_CPU_SFP:
            ret = sfp_cpu_eeprom_path_get(port_type_info.port_index, eeprom_path, sizeof(eeprom_path));
            break;
        default:
            AIM_LOG_ERROR("invalid port type(%d) for port(%d)", port_type_info.type, port);
            return ONLP_STATUS_E_PARAM;
    }

    if (ret != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    }

    // read eeprom data from sysfs
    if(onlp_file_read(data, 256, &size, eeprom_path) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Unable to read eeprom for %s port(%d) sysfs: %s\n", 
                                    port_type_str[port_type_info.type], port, eeprom_path);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Read a byte from an address on the given SFP port's bus.
 * @param port The port number.
 * @param devaddr The device address.
 * @param addr The address.
 */
int onlp_sfpi_dev_readb(int port, uint8_t devaddr, uint8_t addr)
{
    int rc = 0;
    port_type_info_t port_type_info;

    port_type_info = platform_get_port_info(port);

    if(port_type_info.type == TYPE_UNNKOW) {
        return ONLP_STATUS_E_PARAM;
    }

    // not support for cpu sfp port
    if(port_type_info.type == TYPE_CPU_SFP) {
        rc = ONLP_STATUS_E_UNSUPPORTED;
        return rc;
    }
    
    if ((rc=onlp_i2c_readb(port_type_info.eeprom_bus, devaddr, addr, 
        ONLP_I2C_F_FORCE))<0) {
        check_and_do_i2c_mux_reset(port);
    }
    
    return rc;    
}

/**
 * @brief Write a byte to an address on the given SFP port's bus.
 */
int onlp_sfpi_dev_writeb(int port, uint8_t devaddr, uint8_t addr, uint8_t value)
{
    int rc = 0;
    port_type_info_t port_type_info;

    port_type_info = platform_get_port_info(port);

    if(port_type_info.type == TYPE_UNNKOW) {
        return ONLP_STATUS_E_PARAM;
    }

    // not support for cpu sfp port
    if(port_type_info.type == TYPE_CPU_SFP) {
        rc = ONLP_STATUS_E_UNSUPPORTED;
        return rc;
    }

    if ((rc=onlp_i2c_writeb(port_type_info.eeprom_bus, devaddr, addr, value, 
        ONLP_I2C_F_FORCE))<0) {
        check_and_do_i2c_mux_reset(port);
    }   
    
    return rc;
}

/**
 * @brief Read a word from an address on the given SFP port's bus.
 * @param port The port number.
 * @param devaddr The device address.
 * @param addr The address.
 * @returns The word if successful, error otherwise.
 */
int onlp_sfpi_dev_readw(int port, uint8_t devaddr, uint8_t addr)
{
    int rc = 0;
    port_type_info_t port_type_info;

    port_type_info = platform_get_port_info(port);

    if(port_type_info.type == TYPE_UNNKOW) {
        return ONLP_STATUS_E_PARAM;
    }

    // not support for cpu sfp port
    if(port_type_info.type == TYPE_CPU_SFP) {
        rc = ONLP_STATUS_E_UNSUPPORTED;
        return rc;
    }

    if ((rc=onlp_i2c_readw(port_type_info.eeprom_bus, devaddr, addr, 
        ONLP_I2C_F_FORCE))<0) {
        check_and_do_i2c_mux_reset(port);
    }  
    
    return rc;    
}

/**
 * @brief Write a word to an address on the given SFP port's bus.
 */
int onlp_sfpi_dev_writew(int port, uint8_t devaddr, uint8_t addr, uint16_t value)
{
    int rc = 0;
    port_type_info_t port_type_info;

    port_type_info = platform_get_port_info(port);

    if(port_type_info.type == TYPE_UNNKOW) {
        return ONLP_STATUS_E_PARAM;
    }

    // not support for cpu sfp port
    if(port_type_info.type == TYPE_CPU_SFP) {
        rc = ONLP_STATUS_E_UNSUPPORTED;
        return rc;
    }

    if ((rc=onlp_i2c_writew(port_type_info.eeprom_bus, devaddr, addr, value, 
        ONLP_I2C_F_FORCE))<0) {
        check_and_do_i2c_mux_reset(port);
    }
    
    return rc;      
}

/**
 * @brief Read from an address on the given SFP port's bus.
 * @param port The port number.
 * @param devaddr The device address.
 * @param addr The address.
 * @returns The data if successful, error otherwise.
 */
int onlp_sfpi_dev_read(int port, uint8_t devaddr, uint8_t addr, uint8_t* rdata, int size)
{
    int rc = 0;
    port_type_info_t port_type_info;

    port_type_info = platform_get_port_info(port);

    if(port_type_info.type == TYPE_UNNKOW) {
        return ONLP_STATUS_E_PARAM;
    }

    // not support for cpu sfp port
    if(port_type_info.type == TYPE_CPU_SFP) {
        rc = ONLP_STATUS_E_UNSUPPORTED;
        return rc;
    }

    if ((rc=onlp_i2c_block_read(port_type_info.eeprom_bus, devaddr, 
        addr, size, rdata, ONLP_I2C_F_FORCE)) < 0) {
        check_and_do_i2c_mux_reset(port);
    }

    return rc;
}

/**
 * @brief Write to an address on the given SFP port's bus.
 */
int onlp_sfpi_dev_write(int port, uint8_t devaddr, uint8_t addr, uint8_t* data, int size)
{
    int rc = 0;
    port_type_info_t port_type_info;

    port_type_info = platform_get_port_info(port);

    if(port_type_info.type == TYPE_UNNKOW) {
        return ONLP_STATUS_E_PARAM;
    }

    // not support for cpu sfp port
    if(port_type_info.type == TYPE_CPU_SFP) {
        rc = ONLP_STATUS_E_UNSUPPORTED;
        return rc;
    }

    if ((rc=onlp_i2c_write(port_type_info.eeprom_bus, devaddr, 
        addr, size, data, ONLP_I2C_F_FORCE))<0) {
        check_and_do_i2c_mux_reset(port);
    }
    
    return rc;
}

/**
 * @brief Read the SFP DOM EEPROM.
 * @param port The port number.
 * @param data Receives the SFP data.
 */
int onlp_sfpi_dom_read(int port, uint8_t data[256])
{
    char eeprom_path[512];
    FILE* fp;
    port_type_info_t port_type_info;
    int ret;

    //sfp dom is on 0x51 (2nd 256 bytes)
    //qsfp dom is on lower page 0x00
    //qsfpdd 2.0 dom is on lower page 0x00
    //qsfpdd 3.0 and later dom and above is on lower page 0x00 and higher page 0x17
    port_type_info = platform_get_port_info(port);

    if(port_type_info.type == TYPE_UNNKOW) {
        return ONLP_STATUS_E_PARAM;
    }

    memset(data, 0, 256);
    memset(eeprom_path, 0, sizeof(eeprom_path));

    //set eeprom_path
    switch(port_type_info.type) {
        case TYPE_QSFPDD:
        case TYPE_MAC_SFP:
            snprintf(eeprom_path, sizeof(eeprom_path), EEPROM_SYS_FMT, 
                port_type_info.eeprom_bus);
            break;
        case TYPE_CPU_SFP:
            ret = sfp_cpu_eeprom_path_get(port, eeprom_path, sizeof(eeprom_path));
            break;
        default:
            AIM_LOG_ERROR("invalid port type for port(%d)", port);
            return ONLP_STATUS_E_PARAM;
    }

    //read eeprom
    fp = fopen(eeprom_path, "r");
    if(fp == NULL) {
        AIM_LOG_ERROR("Unable to open the eeprom device file of port(%d)", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (fseek(fp, 256, SEEK_CUR) != 0) {
        fclose(fp);
        AIM_LOG_ERROR("Unable to set the file position indicator of port(%d)", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }

    ret = fread(data, 1, 256, fp);
    fclose(fp);
    if (ret != 256) {
        AIM_LOG_ERROR("Unable to read the module_eeprom device file of port(%d)", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Return the RX_LOS bitmap for all SFP ports.
 * @param dst Receives the RX_LOS bitmap.
 */
int onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
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

int onlp_sfpi_control_get(int port, onlp_sfp_control_t control, int* value)
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

int onlp_sfpi_control_set(int port, onlp_sfp_control_t control, int value)
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
int onlp_sfpi_denit(void)
{
    return ONLP_STATUS_OK;
}
