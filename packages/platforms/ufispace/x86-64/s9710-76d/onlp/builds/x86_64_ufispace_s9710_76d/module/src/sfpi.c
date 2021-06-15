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
 * sfpi
 *
 ***********************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <onlp/platformi/sfpi.h>
#include <onlplib/i2c.h>
#include <onlplib/file.h>
#include "platform_lib.h"

#define SFP_NUM               2
#define QSFP_NUM              0
#define QSFPDD_NIF_NUM        36
#define QSFPDD_FAB_NUM        40
#define QSFPX_NUM             (QSFP_NUM+QSFPDD_NIF_NUM+QSFPDD_FAB_NUM)
#define PORT_NUM              (SFP_NUM+QSFPX_NUM)

#define SFP_PORT(_port) (port-QSFPX_NUM)

#define IS_SFP(_port)         (_port >= QSFPX_NUM && _port < PORT_NUM)
#define IS_QSFPX(_port)       (_port >= 0 && _port < QSFPX_NUM)
#define IS_QSFPDD_NIF(_port)  (_port >= 0 && _port < QSFPDD_NIF_NUM)
#define IS_QSFPDD_FAB(_port)  (_port >= QSFPDD_NIF_NUM && _port < QSFPX_NUM)

#define MASK_SFP_PRESENT      0x01
#define MASK_SFP_TX_FAULT     0x02
#define MASK_SFP_RX_LOS       0x04
#define MASK_SFP_TX_DISABLE   0x01

#define SYSFS_SFP_CONFIG     "cpld_sfp_config"
#define SYSFS_SFP_STATUS     "cpld_sfp_status"
#define SYSFS_QSFPDD_RESET   "cpld_qsfpdd_reset"
#define SYSFS_QSFPDD_LPMODE  "cpld_qsfpdd_lpmode"
#define SYSFS_QSFPDD_PRESENT "cpld_qsfpdd_intr_present"
#define SYSFS_EEPROM         "eeprom"

#define EEPROM_ADDR (0x50)

#define VALIDATE_PORT(p) { if ((p < 0) || (p >= PORT_NUM)) return ONLP_STATUS_E_PARAM; }
#define VALIDATE_SFP_PORT(p) { if (!IS_SFP(p)) return ONLP_STATUS_E_PARAM; }

static int ufi_port_to_cpld_addr(int port)
{
    int cpld_addr = 0;
    
    if (port >= 0 && port <= 17) {
        cpld_addr = CPLD_BASE_ADDR[1];
    } else if (port >= 18 && port < QSFPDD_NIF_NUM) { 
        cpld_addr = CPLD_BASE_ADDR[2];
    } else if (port >= 36 && port <= 55) { 
        cpld_addr = CPLD_BASE_ADDR[3];
    } else if (port >= 56 && port < QSFPX_NUM) { 
        cpld_addr = CPLD_BASE_ADDR[4];    
    } else if (IS_SFP(port)) {
        cpld_addr = CPLD_BASE_ADDR[1] + SFP_PORT(port);    
    }
    return cpld_addr;
}

static int ufi_qsfp_port_to_sysfs_attr_offset(int port)
{
    int sysfs_attr_offset = 0;

    if (port >= 0 && port <= 7) {
        sysfs_attr_offset = 0;
    } else if (port >= 8 && port <= 15) { 
        sysfs_attr_offset = 1;
    } else if (port >= 16 && port <= 17) { 
        sysfs_attr_offset = 2;
    } else if (port >= 18 && port <= 25) { 
        sysfs_attr_offset = 0;
    } else if (port >= 26 && port <= 33) { 
        sysfs_attr_offset = 1;
    } else if (port >= 34 && port <= 35) { 
        sysfs_attr_offset = 2;
    } else if (port >= 36 && port <= 43) { 
        sysfs_attr_offset = 0;
    } else if (port >= 44 && port <= 51) { 
        sysfs_attr_offset = 1;
    } else if (port >= 52 && port <= 55) { 
        sysfs_attr_offset = 2;
    } else if (port >= 56 && port <= 63) { 
        sysfs_attr_offset = 0;
    } else if (port >= 64 && port <= 71) { 
        sysfs_attr_offset = 1;
    } else if (port >= 72 && port <= 75) { 
        sysfs_attr_offset = 2;
    }
    
    return sysfs_attr_offset;
}

static int ufi_qsfp_port_to_bit_offset(int port)
{
    int bit_offset = 0;

    if (port >= 0 && port <= 7) {
        bit_offset = port - 0;
    } else if (port >= 8 && port <= 15) { 
        bit_offset = port - 8;
    } else if (port >= 16 && port <= 17) { 
        bit_offset = port - 16;
    } else if (port >= 18 && port <= 25) { 
        bit_offset = port - 18;
    } else if (port >= 26 && port <= 33) { 
        bit_offset = port - 26;
    } else if (port >= 34 && port <= 35) { 
        bit_offset = port - 34;
    } else if (port >= 36 && port <= 43) { 
        bit_offset = port - 36;
    } else if (port >= 44 && port <= 51) { 
        bit_offset = port - 44;
    } else if (port >= 52 && port <= 55) { 
        bit_offset = port - 52;
    } else if (port >= 56 && port <= 63) { 
        bit_offset = port - 56;
    } else if (port >= 64 && port <= 71) { 
        bit_offset = port - 64;
    } else if (port >= 72 && port <= 75) { 
        bit_offset = port - 72;
    }
    
    return bit_offset;
}

static int ufi_port_to_eeprom_bus(int port)
{
    int bus = -1;
    
    if (IS_QSFPDD_NIF(port)) { //QSFPDD_NIF
        bus =  port + 73;
    } else if (IS_QSFPDD_FAB(port)) { //QSFPDD_FAB
        bus =  port - 3;
    } else if (IS_SFP(port)) { //SFP
        bus =  port + 33;
    } else { //unknown ports
        AIM_LOG_ERROR("unknown ports, port=%d\n", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_UNSUPPORTED;
    }
    
    return bus;
}

static int ufi_port_to_cpld_bus(int port)
{
    int bus = -1;
    
    if (IS_QSFPDD_NIF(port)) { //QSFPDD_NIF
        bus =  CPLD_I2C_BUS[1];
    } else if (IS_QSFPDD_FAB(port)) { //QSFPDD_FAB
        bus =  CPLD_I2C_BUS[3];
    } else if (IS_SFP(port)) { //SFP
        bus =  CPLD_I2C_BUS[1];
    } else { //unknown ports
        AIM_LOG_ERROR("unknown ports, port=%d\n", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_UNSUPPORTED;
    }
    
    return bus;
}

static int ufi_qsfp_present_get(int port, int *pres_val)
{     
    int reg_val, rc;
    int cpld_bus, cpld_addr, attr_offset;
       
    //get cpld bus, cpld addr and sysfs_attr_offset
    cpld_bus = ufi_port_to_cpld_bus(port);
    cpld_addr = ufi_port_to_cpld_addr(port);
    attr_offset = ufi_qsfp_port_to_sysfs_attr_offset(port);

    //read register
    if ((rc = file_read_hex(&reg_val, SYS_FMT_OFFSET, cpld_bus, cpld_addr, SYSFS_QSFPDD_PRESENT, attr_offset)) < 0) {
        AIM_LOG_ERROR("Unable to read %s", SYSFS_QSFPDD_PRESENT);
        AIM_LOG_ERROR(SYS_FMT_OFFSET, cpld_bus, cpld_addr, SYSFS_QSFPDD_PRESENT, attr_offset);
        check_and_do_i2c_mux_reset(port);
        return rc;
    }
   
    *pres_val = !((reg_val >> ufi_qsfp_port_to_bit_offset(port)) & 0x1);
    
    return ONLP_STATUS_OK;
}

static int ufi_sfp_present_get(int port, int *pres_val)
{
    int reg_val, rc;
    int cpld_bus, cpld_addr; 

    //get cpld bus and cpld addr
    cpld_bus = ufi_port_to_cpld_bus(port);
    cpld_addr = ufi_port_to_cpld_addr(port);

    //read register
    if ((rc = file_read_hex(&reg_val, SYS_FMT, cpld_bus, cpld_addr, SYSFS_SFP_STATUS)) < 0) {
        AIM_LOG_ERROR("Unable to read %s", SYSFS_SFP_STATUS);
        AIM_LOG_ERROR(SYS_FMT, cpld_bus, cpld_addr, SYSFS_SFP_STATUS);
        check_and_do_i2c_mux_reset(port);
        return rc;
    }
   
    *pres_val = !(reg_val & 0x1);
    
    return ONLP_STATUS_OK;
}

/**
 * @brief Software initialization of the SFP module.
 */
int onlp_sfpi_sw_init(void)
{
    lock_init();
    return ONLP_STATUS_OK;
}

/**
 * @brief Hardware initialization of the SFP module.
 * @param flags The hardware initialization flags.
 */
int onlp_sfpi_hw_init(uint32_t flags)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Deinitialize the chassis software module.
 * @note The primary purpose of this API is to properly
 * deallocate any resources used by the module in order
 * faciliate detection of real resouce leaks.
 */
int onlp_sfpi_sw_denit(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the bitmap of SFP-capable port numbers.
 * @param[out] bmap Receives the bitmap.
 */
int onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{
    int p;

    AIM_BITMAP_CLR_ALL(bmap);
    for(p = 0; p < PORT_NUM; p++) {
        AIM_BITMAP_SET(bmap, p);
    }
    
    return ONLP_STATUS_OK;
}

/**
 * @brief Determine the SFP connector type.
 * @param id The SFP Port ID.
 * @param[out] rtype Receives the connector type.
 */
int onlp_sfpi_type_get(onlp_oid_id_t id, onlp_sfp_type_t* rtype)
{
    int local_id = ONLP_OID_ID_GET(id);

    //QSFP, QSFPDD, SFP Ports
    if (IS_QSFPX(local_id)) { //QSFPDD
        *rtype = ONLP_SFP_TYPE_QSFP28;
    } else if (IS_SFP(local_id)) { //SFP
        *rtype = ONLP_SFP_TYPE_SFP;
    } else { //unkonwn ports
        AIM_LOG_ERROR("unknown ports, port=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Determine if an SFP is present.
 * @param id The SFP Port ID.
 * @returns 1 if present
 * @returns 0 if absent
 * @returns An error condition.
 */
int onlp_sfpi_is_present(onlp_oid_id_t id)
{
    int port = ONLP_OID_ID_GET(id);
    int status=ONLP_STATUS_OK;

    VALIDATE_PORT(port);
    
    //QSFPDD Ports
    if (IS_QSFPX(port)) {
        if (ufi_qsfp_present_get(port, &status) < 0) {
            AIM_LOG_ERROR("qsfp_presnet_get() failed, port=%d\n", port);
            return ONLP_STATUS_E_INTERNAL;
        }
    } else if (IS_SFP(port)) { //SFP
        if (ufi_sfp_present_get(port, &status) < 0) {
            AIM_LOG_ERROR("sfp_presnet_get() failed, port=%d\n", port);
            return ONLP_STATUS_E_INTERNAL;
        }
    } else {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    return status;
}

/**
 * @brief Return the presence bitmap for all SFP ports.
 * @param[out] dst Receives the presence bitmap.
 */
int onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int p = 0;
    int ret = 0;

    AIM_BITMAP_CLR_ALL(dst);
    for (p = 0; p < PORT_NUM; p++) {
        ret = onlp_sfpi_is_present(p);
        AIM_BITMAP_MOD(dst, p, ret);
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Return the RX_LOS bitmap for all SFP ports.
 * @param[out] dst Receives the RX_LOS bitmap.
 */
int onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int i=0, value=0;

    /* Populate bitmap - QSFPDD_NIF and QSFPDD_FAB ports */
    for(i = 0; i < (QSFPX_NUM); i++) {
        AIM_BITMAP_MOD(dst, i, 0);
    }

    /* Populate bitmap - SFP+ ports */
    for(i = QSFPX_NUM; i < PORT_NUM; i++) {
	if (onlp_sfpi_control_get(i, ONLP_SFP_CONTROL_RX_LOS, &value)< 0) {
            return ONLP_STATUS_E_INTERNAL;
        } else {
            AIM_BITMAP_MOD(dst, i, value);
        }
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Read bytes from the target device on the given SFP port.
 * @param id The SFP Port ID.
 * @param devaddr The device address.
 * @param addr Read offset.
 * @param[out] dst Receives the data.
 * @param len Read length.
 * @returns The number of bytes read or ONLP_STATUS_E_* no error.
 */
int onlp_sfpi_dev_read(onlp_oid_id_t id, int devaddr, int addr, uint8_t* dst, int len)
{
    int port = ONLP_OID_ID_GET(id);
    int bus = -1;

    VALIDATE_PORT(port);
    
    if (onlp_sfpi_is_present(port) < 0) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    devaddr = EEPROM_ADDR;
    bus = ufi_port_to_eeprom_bus(port);
    
    if (port < PORT_NUM) {        
        if (onlp_i2c_block_read(bus, devaddr, addr, len, dst, ONLP_I2C_F_FORCE) < 0) {
            check_and_do_i2c_mux_reset(port);
            return ONLP_STATUS_E_INTERNAL;
        }
    } else {
        return ONLP_STATUS_E_PARAM;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Write bytes to the target device on the given SFP port.
 * @param id The SFP Port ID.
 * @param devaddr The device address.
 * @param addr Write offset.
 * @param src The bytes to write.
 * @param len Write length.
 */
int onlp_sfpi_dev_write(onlp_oid_id_t id, int devaddr, int addr, uint8_t* src, int len)
{
    int port = ONLP_OID_ID_GET(id);
    VALIDATE_PORT(port);
    int rc = 0;
    int bus = ufi_port_to_eeprom_bus(port);

    if ((rc=onlp_i2c_write(bus, devaddr, addr, len, src, ONLP_I2C_F_FORCE))<0) {
        check_and_do_i2c_mux_reset(port);
    }
    
    return rc;
}

/**
 * @brief Read a byte from the target device on the given SFP port.
 * @param id The SFP Port ID.
 * @param devaddr The device address.
 * @param addr The read address.
 * @returns The byte on success or ONLP_STATUS_E* on error.
 */
int onlp_sfpi_dev_readb(onlp_oid_id_t id, int devaddr, int addr)
{
    int port = ONLP_OID_ID_GET(id);
    VALIDATE_PORT(port);
    int rc = 0;
    int bus = ufi_port_to_eeprom_bus(port);

    if (onlp_sfpi_is_present(id) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }
    
    if ((rc=onlp_i2c_readb(bus, devaddr, addr, ONLP_I2C_F_FORCE))<0) {
        check_and_do_i2c_mux_reset(port);
    }
    
    return rc;    
}

/**
 * @brief Write a byte to the target device on the given SFP port.
 * @param id The SFP Port ID.
 * @param devaddr The device address.
 * @param addr The write address.
 * @param value The write value.
 */
int onlp_sfpi_dev_writeb(onlp_oid_id_t id, int devaddr, int addr, uint8_t value)
{
    int port = ONLP_OID_ID_GET(id);
    VALIDATE_PORT(port);
    int rc = 0;
    int bus = ufi_port_to_eeprom_bus(port);    

    if (onlp_sfpi_is_present(id) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }
    
    if ((rc=onlp_i2c_writeb(bus, devaddr, addr, value, ONLP_I2C_F_FORCE))<0) {
        check_and_do_i2c_mux_reset(port);
    }   
    
    return rc;
}

/**
 * @brief Read a word from the target device on the given SFP port.
 * @param id The SFP Port ID.
 * @param devaddr The device address.
 * @param addr The read address.
 * @returns The word if successful, ONLP_STATUS_E* on error.
 */
int onlp_sfpi_dev_readw(onlp_oid_id_t id, int devaddr, int addr)
{
    int port = ONLP_OID_ID_GET(id);
    VALIDATE_PORT(port);
    int rc = 0;
    int bus = ufi_port_to_eeprom_bus(port);

    if (onlp_sfpi_is_present(id) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }
    
    if ((rc=onlp_i2c_readw(bus, devaddr, addr, ONLP_I2C_F_FORCE))<0) {
        check_and_do_i2c_mux_reset(port);
    }
    
    return rc;    
}

/**
 * @brief Write a word to the target device on the given SFP port.
 * @param id The SFP Port ID.
 * @param devaddr The device address.
 * @param addr The write address.
 * @param value The write value.
 */
int onlp_sfpi_dev_writew(onlp_oid_id_t id, int devaddr, int addr, uint16_t value)
{
    int port = ONLP_OID_ID_GET(id);
    VALIDATE_PORT(port);
    int rc = 0;
    int bus = ufi_port_to_eeprom_bus(port);    

    if (onlp_sfpi_is_present(id) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }
    
    if ((rc=onlp_i2c_writew(bus, devaddr, addr, value, ONLP_I2C_F_FORCE))<0) {
        check_and_do_i2c_mux_reset(port);
    }   
    
    return rc;
}

/**
 * @brief Perform any actions required after an SFP is inserted.
 * @param id The SFP Port ID.
 * @param info The SFF Module information structure.
 * @note This function is optional. If your platform must
 * adjust equalizer or preemphasis settings internally then
 * this function should be implemented as the trigger.
 */
int onlp_sfpi_post_insert(onlp_oid_id_t id, sff_info_t* info)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Returns whether or not the given control is supported on the given port.
 * @param id The SFP Port ID.
 * @param control The control.
 * @param[out] rv Receives 1 if supported, 0 if not supported.
 * @note This provided for convenience and is optional.
 * If you implement this function your control_set and control_get APIs
 * will not be called on unsupported ports.
 */
int onlp_sfpi_control_supported(onlp_oid_id_t id, onlp_sfp_control_t control, int* rv)
{
    int port = ONLP_OID_ID_GET(id);
    VALIDATE_PORT(port);
    
    //set unsupported as default value
    *rv=0;
    
    switch (control) {
        case ONLP_SFP_CONTROL_RESET:
        case ONLP_SFP_CONTROL_RESET_STATE:
        case ONLP_SFP_CONTROL_LP_MODE:            
            if (IS_QSFPX(port)) {
                *rv = 1;
            }
            break;        
        case ONLP_SFP_CONTROL_RX_LOS:
        case ONLP_SFP_CONTROL_TX_FAULT:    
        case ONLP_SFP_CONTROL_TX_DISABLE:        
            if (IS_SFP(port)) {
                *rv = 1;
            }
            break;        
        default:
            *rv = 0;
            break;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Set an SFP control.
 * @param id The SFP Port ID.
 * @param control The control.
 * @param value The value.
 */
int onlp_sfpi_control_set(onlp_oid_id_t id, onlp_sfp_control_t control, int value)
{
    int port = ONLP_OID_ID_GET(id);
    int rc = 0;
    int reg_val = 0;
    int reg_mask = 0;
    int bus = 0;
    int cpld_addr = 0;    
    int attr_offset = 0, bit_offset = 0;

    VALIDATE_PORT(port);
    
    bus = ufi_port_to_cpld_bus(port);
    cpld_addr = ufi_port_to_cpld_addr(port);

    switch(control)
        {
        case ONLP_SFP_CONTROL_RESET:
            {
                if (IS_QSFPX(port)) {
                    //read reg_val
                    attr_offset = ufi_qsfp_port_to_sysfs_attr_offset(port);                    
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFPDD_RESET, attr_offset) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //update reg_val
                    //0 is normal, 1 is reset, reverse value to fit our platform
                    bit_offset = ufi_qsfp_port_to_bit_offset(port);
                    reg_val = ufi_bit_operation(reg_val, bit_offset, !value);

                    //write reg_val
                    if ((rc=onlp_file_write_int(reg_val, SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFPDD_RESET, attr_offset)) < 0) {
                        AIM_LOG_ERROR("Unable to write %s, error=%d, reg_val=%x", SYSFS_QSFPDD_RESET,  rc, reg_val);
                        AIM_LOG_ERROR(SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFPDD_RESET);
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }
                    rc = ONLP_STATUS_OK;
                } else {
                    rc = ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }        
        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                if (IS_SFP(port)) {
                    //read reg_val
                    if (file_read_hex(&reg_val, SYS_FMT, bus, cpld_addr, SYSFS_SFP_CONFIG) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //update reg_val
                    //0 is normal, 1 is tx_disable
                    reg_mask = MASK_SFP_TX_DISABLE;
                    if (value == 1) {
                        reg_val = reg_val | reg_mask;
                    } else {
                        reg_val = reg_val & ~reg_mask;
                    }

                    //write reg_val
                    if (onlp_file_write_int(reg_val, SYS_FMT, bus, cpld_addr, SYSFS_SFP_CONFIG) < 0) {
                        AIM_LOG_ERROR("Unable to write %s, error=%d, reg_val=%x", SYSFS_SFP_CONFIG, rc, reg_val);
                        AIM_LOG_ERROR(SYS_FMT, bus, cpld_addr, SYSFS_SFP_CONFIG);
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }
                    rc = ONLP_STATUS_OK;
                } else {
                    rc = ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                if (IS_QSFPX(port)) {
                    //read reg_val
                    attr_offset = ufi_qsfp_port_to_sysfs_attr_offset(port);                    
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFPDD_LPMODE, attr_offset) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //update reg_val
                    //0 is normal, 1 is low power
                    bit_offset = ufi_qsfp_port_to_bit_offset(port);
                    reg_val = ufi_bit_operation(reg_val, bit_offset, value);

                    //write reg_val
                    if (onlp_file_write_int(reg_val, SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFPDD_LPMODE, attr_offset) < 0) {
                        AIM_LOG_ERROR("Unable to write %s, error=%d, reg_val=%x", SYSFS_QSFPDD_LPMODE,  rc, reg_val);
                        AIM_LOG_ERROR(SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFPDD_LPMODE);
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }
                    rc = ONLP_STATUS_OK;
                } else {
                    rc = ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }                
        default:
            rc = ONLP_STATUS_E_UNSUPPORTED;
        }

    return rc;
}

/**
 * @brief Get an SFP control.
 * @param id The SFP Port ID.
 * @param control The control
 * @param[out] value Receives the current value.
 */
int onlp_sfpi_control_get(onlp_oid_id_t id, onlp_sfp_control_t control, int* value)
{
    int port = ONLP_OID_ID_GET(id);
    int rc;
    int reg_val = 0, reg_mask = 0;
    int bus = 0;
    int cpld_addr = 0;    
    int attr_offset = 0, bit_offset = 0;

    VALIDATE_PORT(port);
    
    bus = ufi_port_to_cpld_bus(port);
    cpld_addr = ufi_port_to_cpld_addr(port);

    switch(control)
        {
        case ONLP_SFP_CONTROL_RESET_STATE:
            {
                if (IS_QSFPX(port)) {
                    //read reg_val
                    attr_offset = ufi_qsfp_port_to_sysfs_attr_offset(port);                    
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFPDD_RESET, attr_offset) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //read bit value
                    //0 is normal, 1 is reset, reverse value to fit our platform
                    bit_offset = ufi_qsfp_port_to_bit_offset(port);
                    reg_mask = 1 << bit_offset;
                    *value = !ufi_mask_shift(reg_val, reg_mask);

                    rc = ONLP_STATUS_OK;                    
                } else {
                    rc = ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_RX_LOS:
            {
                if (IS_SFP(port)) {
                    //read reg_val
                    if (file_read_hex(&reg_val, SYS_FMT, bus, cpld_addr, SYSFS_SFP_STATUS) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //read bit value
                    //0 is normal, 1 is rx_los
                    reg_mask = MASK_SFP_RX_LOS;
                    *value = ufi_mask_shift(reg_val, reg_mask);

                    rc = ONLP_STATUS_OK;
                } else {
                    rc = ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_TX_FAULT:
            {
                if (IS_SFP(port)) {
                    //read reg_val
                    if (file_read_hex(&reg_val, SYS_FMT, bus, cpld_addr, SYSFS_SFP_STATUS) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //read bit value
                    //0 is normal, 1 is tx_fault
                    reg_mask = MASK_SFP_TX_FAULT;
                    *value = ufi_mask_shift(reg_val, reg_mask);

                    rc = ONLP_STATUS_OK;
                } else {
                    rc = ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }        
        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                if (IS_SFP(port)) {
                    //read reg_val
                    if (file_read_hex(&reg_val, SYS_FMT, bus, cpld_addr, SYSFS_SFP_CONFIG) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //read bit value
                    //0 is normal, 1 is tx_disable
                    reg_mask = MASK_SFP_TX_DISABLE;
                    *value = ufi_mask_shift(reg_val, reg_mask);

                    rc = ONLP_STATUS_OK;
                } else {
                    rc = ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                if (IS_QSFPX(port)) {
                    //read reg_val
                    attr_offset = ufi_qsfp_port_to_sysfs_attr_offset(port);                    
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFPDD_LPMODE, attr_offset) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //read bit value
                    //0 is normal, 1 is low power
                    bit_offset = ufi_qsfp_port_to_bit_offset(port);
                    reg_mask = 1 << bit_offset;
                    *value = ufi_mask_shift(reg_val, reg_mask);

                    rc = ONLP_STATUS_OK;                    
                } else {
                    rc = ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }                
        default:
            rc = ONLP_STATUS_E_UNSUPPORTED;
        }

    return rc;
}

/**
 * @brief Remap SFP user SFP port numbers before calling the SFPI interface.
 * @param id The SFP Port ID.
 * @param[out] rport Receives the new port.
 * @note This function will be called to remap the user SFP port number
 * to the number returned in rport before the SFPI functions are called.
 * This is an optional convenience for platforms with dynamic or
 * variant physical SFP numbering.
 */
int onlp_sfpi_port_map(onlp_oid_id_t id, int* rport)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Get the SFP's OID header.
 * @param id The SFP oid.
 * @param [out] hdr Receives the header.
 */
int onlp_sfpi_hdr_get(onlp_oid_id_t id, onlp_oid_hdr_t* hdr)
{
    int local_id = ONLP_OID_ID_GET(id);
    int is_present = onlp_sfpi_is_present(id);

    hdr->id = ONLP_SFP_ID_CREATE(local_id);
    hdr->poid = ONLP_OID_CHASSIS;
    hdr->status = is_present ? ONLP_OID_STATUS_FLAG_PRESENT : ONLP_OID_STATUS_FLAG_UNPLUGGED;

    return ONLP_STATUS_OK;
}

