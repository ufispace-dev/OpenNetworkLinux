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
 * SFP Platform Implementation Interface.
 *
 ***********************************************************/
#include <onlp/onlp.h>
#include <onlplib/i2c.h>
#include <onlplib/file.h>
#include <onlp/platformi/sfpi.h>
#include "platform_lib.h"

#define SFP_NUM                     2
#define QSFP_NUM                    40
#define QSFPDD_NUM                  13
#define QSFPX_NUM             (QSFP_NUM+QSFPDD_NUM)
#define PORT_NUM                    (SFP_NUM+QSFPX_NUM)

#define IS_SFP(_port)         (_port >= QSFPX_NUM && _port < PORT_NUM)
#define IS_QSFP(_port)       (_port >= 0 && _port < QSFP_NUM)
#define IS_QSFPDD(_port)  (_port >= QSFP_NUM && _port < QSFPX_NUM)
#define IS_QSFPX(_port)       (_port >= 0 && _port < QSFPX_NUM)

#define SYSFS_EEPROM         "eeprom"

#define VALIDATE_PORT(p) { if ((p < 0) || (p >= PORT_NUM)) return ONLP_STATUS_E_PARAM; }
#define VALIDATE_SFP_PORT(p) { if (!IS_SFP(p)) return ONLP_STATUS_E_PARAM; }

#define EEPROM_ADDR (0x50)

static int qsfp_port_to_cpld_addr(int port)
{
    int cpld_addr = 0;

    if (port >=0 && port <=11) {
        cpld_addr = 0x30;
    } else if (port >=12 && port <=24) { 
        cpld_addr = 0x39;    
    } else if (port >= 25 && port <=37) { 
        cpld_addr = 0x3a;    
    } else if (port >=38 && port <=42) { 
        cpld_addr = 0x3b;    
    } else if (port >=43 && port <=52) { 
        cpld_addr = 0x3c;    
    }    
    return cpld_addr;
}

static int qsfp_port_to_sysfs_attr_offset(int port)
{
    int sysfs_attr_offset = 0;

    if (port >=0 && port <=11) {
        sysfs_attr_offset = port-0;    
    } else if (port >=12 && port <=24) { 
        sysfs_attr_offset = port-12;    
    } else if (port >= 25 && port <=37) { 
        sysfs_attr_offset = port-25;    
    } else if (port >=38 && port <=39) { 
        sysfs_attr_offset = port-38;    
    } else if (port >=40 && port <=42) { 
        sysfs_attr_offset = port-40;    
    } else if (port >=43 && port <=52) { 
        sysfs_attr_offset = port-43;    
    }    
    return sysfs_attr_offset;
}

static int ufi_port_to_eeprom_bus(int port)
{
    int bus = -1;

    if (IS_QSFPX(port)) { //QSFP/QSFPDD
        if (port >= 20 && port < QSFP_NUM) { //swap some QSFP port
            port = (port % 2) == 0 ? port + 1 : port - 1;
        }
        bus =  port + 25;
    } else { //unknown ports
        AIM_LOG_ERROR("unknown ports, port=%d\n", port);
        return ONLP_STATUS_E_UNSUPPORTED;
    }
    
    return bus;
}

static int qsfp_present_get(int port, int *pres_val)
{     
    int status, rc;
    int cpld_addr, sysfs_attr_offset;
    uint8_t data[8];
    int data_len;
    char *is_dd = NULL;
   
    memset(data, 0, sizeof(data));

    //for qsfp port 20-39
    if (port >= 20 && port < QSFP_NUM) {
        port = (port%2) == 0 ? port +1 : port -1;
    }
    //get cpld addr
    cpld_addr = qsfp_port_to_cpld_addr(port);
    //get sysfs_attr_offset
    sysfs_attr_offset = qsfp_port_to_sysfs_attr_offset(port);
    //is_dd
    is_dd = IS_QSFP(port) ? "" : "dd";

    if ((rc = onlp_file_read(data, sizeof(data), &data_len, "/sys/bus/i2c/devices/%d-%04x/cpld_qsfp%s_port_status_%d",
                                 I2C_BUS_1, cpld_addr, is_dd, sysfs_attr_offset)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("onlp_file_read failed, error=%d, /sys/bus/i2c/devices/%d-%04x/cpld_qsfp%s_port_status_%d", 
            rc, I2C_BUS_1, cpld_addr, is_dd, sysfs_attr_offset);
        return ONLP_STATUS_E_INTERNAL;
    }    
    status = (int) strtol((char *)data, NULL, 0);
   
    *pres_val = !((status & 0x2) >> 1);
    
    return ONLP_STATUS_OK;
}

static int sfp_present_get(int port, int *pres_val)
{
    int ret;

    port = port - QSFPX_NUM;
    if (port == 0) {
        ret = system("ethtool -m eth1 raw on length 1 > /dev/null 2>&1");
        *pres_val = (ret==0) ? 1 : 0;
    } else if (port == 1) {
        ret = system("ethtool -m eth2 raw on length 1 > /dev/null 2>&1");
        *pres_val = (ret==0) ? 1 : 0;
    } else {
        AIM_LOG_ERROR("unknown sfp port, port=%d\n", port);
        return ONLP_STATUS_E_INTERNAL;
    }   
    
    return ONLP_STATUS_OK;
}

int onlp_sfpi_init(void)
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

int onlp_sfpi_is_present(int port)
{
    int status=1;
    
    //QSFP, QSFPDD, SFP Ports
    if (IS_QSFPX(port)) { //QSFP, QSFPDD
        if (qsfp_present_get(port, &status) != ONLP_STATUS_OK) {
            AIM_LOG_ERROR("qsfp_presnet_get() failed, port=%d\n", port);
            return ONLP_STATUS_E_INTERNAL;
        }
    } else if (IS_SFP(port)) { //SFP
        if (sfp_present_get(port, &status) != ONLP_STATUS_OK) {
            AIM_LOG_ERROR("sfp_presnet_get() failed, port=%d\n", port);
            return ONLP_STATUS_E_INTERNAL;
        }
    } else { //unknown ports
        AIM_LOG_ERROR("unknown ports, port=%d\n", port);
        return ONLP_STATUS_E_UNSUPPORTED;
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

/*
 * This function reads the SFPs idrom and returns in
 * in the data buffer provided.
 */
int onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{   
    char eeprom_path[512];
    int size = 0;
    int bus = 0;
    int sfp_port_num = 0;
    int ret;
    
    memset(eeprom_path, 0, sizeof(eeprom_path));    
    memset(data, 0, 256);

    //QSFP, QSFPDD, SFP ports
    if (IS_QSFPX(port)) { //QSFP and QSFPDD
        bus = ufi_port_to_eeprom_bus(port);        
        snprintf(eeprom_path, sizeof(eeprom_path), SYS_FMT, bus, EEPROM_ADDR, SYSFS_EEPROM);
    } else if (IS_SFP(port)) { //SFP
        sfp_port_num = port - QSFPX_NUM;
        if (sfp_port_num == 0) {
            ret = system("ethtool -m eth1 raw on length 256 > /tmp/.sfp.eth1.eeprom");
            if (ret == 0) {
                snprintf(eeprom_path, sizeof(eeprom_path), "/tmp/.sfp.eth1.eeprom");
            } else {
                AIM_LOG_ERROR("Unable to read eeprom from port(%d)\r\n", port);
                return ONLP_STATUS_E_INTERNAL;
            }
        } else {
            ret = system("ethtool -m eth2 raw on length 256 > /tmp/.sfp.eth2.eeprom");
            if (ret == 0) {
                snprintf(eeprom_path, sizeof(eeprom_path), "/tmp/.sfp.eth2.eeprom");
            } else {
                AIM_LOG_ERROR("Unable to read eeprom from port(%d)\r\n", port);
                return ONLP_STATUS_E_INTERNAL;
            }
        }
    } else { //unknown ports
        AIM_LOG_ERROR("unknown ports, port=%d\n", port);
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if(onlp_file_read(data, 256, &size, eeprom_path) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d)\r\n", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (size != 256) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d), size is different!\r\n", port);
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
    VALIDATE_PORT(port);
    int rc = 0;
    int bus = -1;

    if (IS_SFP(port)) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }
    
    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    bus = ufi_port_to_eeprom_bus(port);
    if ((rc=onlp_i2c_readb(bus, devaddr, addr, ONLP_I2C_F_FORCE))<0) {
        check_and_do_i2c_mux_reset(port);
    }
    
    return rc;    
}

/**
 * @brief Write a byte to an address on the given SFP port's bus.
 */
int onlp_sfpi_dev_writeb(int port, uint8_t devaddr, uint8_t addr, uint8_t value)
{
    VALIDATE_PORT(port);
    int rc = 0;
    int bus = -1;    

    if (IS_SFP(port)) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }
    
    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    bus = ufi_port_to_eeprom_bus(port);    
    if ((rc=onlp_i2c_writeb(bus, devaddr, addr, value, ONLP_I2C_F_FORCE))<0) {
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
    VALIDATE_PORT(port);
    int rc = 0;
    int bus = -1;    

    if (IS_SFP(port)) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }
    
    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    bus = ufi_port_to_eeprom_bus(port);
    if ((rc=onlp_i2c_readw(bus, devaddr, addr, ONLP_I2C_F_FORCE))<0) {
        check_and_do_i2c_mux_reset(port);
    }  
    
    return rc;    
}

/**
 * @brief Write a word to an address on the given SFP port's bus.
 */
int onlp_sfpi_dev_writew(int port, uint8_t devaddr, uint8_t addr, uint16_t value)
{
    VALIDATE_PORT(port);
    int rc = 0;
    int bus = -1;    

    if (IS_SFP(port)) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }
    
    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    bus = ufi_port_to_eeprom_bus(port);
    if ((rc=onlp_i2c_writew(bus, devaddr, addr, value, ONLP_I2C_F_FORCE))<0) {
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
    VALIDATE_PORT(port);
    int bus = -1;    

    if (IS_SFP(port)) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }
    
    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    bus = ufi_port_to_eeprom_bus(port);
    if (onlp_i2c_block_read(bus, devaddr, addr, size, rdata, ONLP_I2C_F_FORCE) < 0) {
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Write to an address on the given SFP port's bus.
 */
int onlp_sfpi_dev_write(int port, uint8_t devaddr, uint8_t addr, uint8_t* data, int size)
{
    VALIDATE_PORT(port);
    int rc = 0;
    int bus = -1;    

    if (IS_SFP(port)) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }
    
    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    bus = ufi_port_to_eeprom_bus(port);
    if ((rc=onlp_i2c_write(bus, devaddr, addr, size, data, ONLP_I2C_F_FORCE))<0) {
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
    return ONLP_STATUS_E_UNSUPPORTED;
}

int onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int i=0, value=0, rc=0;

    /* Populate bitmap - QSFP and QSFPDD ports */
    for(i = 0; i < QSFPX_NUM; i++) {
        AIM_BITMAP_MOD(dst, i, 0);
    }

    /* Populate bitmap - SFP+ ports */
    for(i = QSFPX_NUM; i < PORT_NUM; i++) {
        if ((rc=onlp_sfpi_control_get(i, ONLP_SFP_CONTROL_RX_LOS, &value)) != ONLP_STATUS_OK) {
            return ONLP_STATUS_E_INTERNAL;
        } else {
            AIM_BITMAP_MOD(dst, i, value);
        }
    }

    return ONLP_STATUS_OK;
}

int onlp_sfpi_control_get(int port, onlp_sfp_control_t control, int* value)
{
    int rc;
    uint8_t data[8];
    int data_len = 0, data_value = 0, data_mask = 0;
    char sysfs_path[128];
    int cpld_addr = 0x30;
    
    memset(data, 0, sizeof(data));
    memset(sysfs_path, 0, sizeof(sysfs_path));

    //QSFPDD ports are not supported
    if (!IS_SFP(port)) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    //sysfs path
    if (control == ONLP_SFP_CONTROL_RX_LOS || control == ONLP_SFP_CONTROL_TX_FAULT) {
        snprintf(sysfs_path, sizeof(sysfs_path), SYS_DEV "%d-%04x/cpld_sfp_port_status", I2C_BUS_1, cpld_addr);
    } else if (control == ONLP_SFP_CONTROL_TX_DISABLE) {
        snprintf(sysfs_path, sizeof(sysfs_path), SYS_DEV "%d-%04x/cpld_sfp_port_config", I2C_BUS_1, cpld_addr);
    } else {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if ((rc = onlp_file_read(data, sizeof(data), &data_len, sysfs_path)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("onlp_file_read failed, error=%d, %s", rc, sysfs_path);
        return ONLP_STATUS_E_INTERNAL;
    }

    //hex to int
    data_value = (int) strtol((char *)data, NULL, 0);

    switch(control)
        {
        case ONLP_SFP_CONTROL_RX_LOS:
            {
                //SFP+ Port 0
                if ( port == QSFPX_NUM ) {
                    data_mask = 0x04;
                } else { //SFP+ Port 1
                    data_mask = 0x40;
                }
                *value = data_value & data_mask;
                rc = ONLP_STATUS_OK;
                break;
            }

        case ONLP_SFP_CONTROL_TX_FAULT:
            {
                //SFP+ Port 0
                if ( port == QSFPX_NUM ) {
                    data_mask = 0x02;
                } else { //SFP+ Port 1
                    data_mask = 0x20;
                }
                *value = data_value & data_mask;
                rc = ONLP_STATUS_OK;
                break;
            }

        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                //SFP+ Port 0
                if ( port == QSFPX_NUM ) {
                    data_mask = 0x01;
                } else { //SFP+ Port 1
                    data_mask = 0x10;
                }
                *value = data_value & data_mask;
                rc = ONLP_STATUS_OK;
                break;
            }

        default:
            rc = ONLP_STATUS_E_UNSUPPORTED;
        }

    return rc;
}

int onlp_sfpi_control_set(int port, onlp_sfp_control_t control, int value)
{
    int rc;
    uint8_t data[8];
    int data_len = 0, data_value = 0, data_mask = 0;
    char sysfs_path[128];
    int cpld_addr = 0x30;
    int reg_val = 0;
    
    memset(data, 0, sizeof(data));
    memset(sysfs_path, 0, sizeof(sysfs_path));

    //QSFPDD ports are not supported
    if (!IS_SFP(port)) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    //sysfs path
    if (control == ONLP_SFP_CONTROL_TX_DISABLE) {
        snprintf(sysfs_path, sizeof(sysfs_path), SYS_DEV "%d-%04x/cpld_sfp_port_config", I2C_BUS_1, cpld_addr);
    } else {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if ((rc = onlp_file_read(data, sizeof(data), &data_len, sysfs_path)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("onlp_file_read failed, error=%d, %s", rc, sysfs_path);
        return ONLP_STATUS_E_INTERNAL;
    }

    //hex to int
    data_value = (int) strtol((char *)data, NULL, 0);

    switch(control)
        {

        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                //SFP+ Port 0
                if (port == QSFPX_NUM) {
                    data_mask = 0x01;
                } else { //SFP+ Port 1
                    data_mask = 0x10;
                }

                if (value == 1) {
                    reg_val = data_value | data_mask;
                } else {
                    reg_val = data_value & ~data_mask;
                }
                if ((rc = onlp_file_write_int(reg_val, sysfs_path)) != ONLP_STATUS_OK) {
                    AIM_LOG_ERROR("Unable to write cpld_sfp_port_config, error=%d, sysfs=%s, reg_val=%x", rc, sysfs_path, reg_val);
                    return ONLP_STATUS_E_INTERNAL;
                }
                rc = ONLP_STATUS_OK;
                break;
            }

        default:
            rc = ONLP_STATUS_E_UNSUPPORTED;
        }

    return rc;
}

/*
 * De-initialize the SFPI subsystem.
 */
int onlp_sfpi_denit(void)
{
    return ONLP_STATUS_OK;
}
