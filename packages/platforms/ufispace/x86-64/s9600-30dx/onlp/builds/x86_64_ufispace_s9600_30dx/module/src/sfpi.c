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
#include <onlp/platformi/sfpi.h>
#include "platform_lib.h"

#define SFP_NUM               2
#define QSFP_NUM        16
#define QSFPDD_NUM        14
#define QSFPX_NUM             (QSFP_NUM+QSFPDD_NUM)
#define PORT_NUM              (SFP_NUM+QSFPX_NUM)

#define SFP_PORT(_port) (port-QSFPX_NUM)

#define IS_SFP(_port)         (_port >= QSFPX_NUM && _port < PORT_NUM)
#define IS_QSFPX(_port)       (_port >= 0 && _port < QSFPX_NUM)
#define IS_QSFP(_port)  (_port >= 0 && _port < QSFP_NUM)
#define IS_QSFPDD(_port)  (_port >= QSFP_NUM && _port < QSFPX_NUM)
#define IS_SFP_P0(_port)     (_port == (QSFP_NUM+QSFPDD_NUM))
#define IS_SFP_P1(_port)     (_port == (QSFP_NUM+QSFPDD_NUM+1))

//#define SFP0_INTERFACE_NAME "enp2s0f0"
//#define SFP1_INTERFACE_NAME "enp2s0f1"
#define SFP0_INTERFACE_NAME "enp182s0f0"
#define SFP1_INTERFACE_NAME "enp182s0f1"

#define MASK_SFP_PRESENT      0x01
#define MASK_SFP_TX_FAULT     0x02
#define MASK_SFP_RX_LOS       0x04
#define MASK_SFP_TX_DISABLE   0x01

#define SYSFS_SFP_CONFIG     "cpld_sfp_config"
#define SYSFS_SFP_STATUS     "cpld_sfp_status"
#define SYSFS_QSFP_RESET   "cpld_qsfp_reset"
#define SYSFS_QSFP_LPMODE  "cpld_qsfp_lpmode"
#define SYSFS_QSFP_PRESENT "cpld_qsfp_intr_present"
#define SYSFS_QSFPDD_RESET   "cpld_qsfpdd_reset"
#define SYSFS_QSFPDD_LPMODE  "cpld_qsfpdd_lpmode"
#define SYSFS_QSFPDD_PRESENT "cpld_qsfpdd_intr_present"
#define SYSFS_EEPROM         "eeprom"

#define VALIDATE_PORT(p) { if ((p < 0) || (p >= PORT_NUM)) return ONLP_STATUS_E_PARAM; }
#define VALIDATE_SFP_PORT(p) { if (!IS_SFP(p)) return ONLP_STATUS_E_PARAM; }

const char sysfs_attr_suffix[][8] = {"0_7", "8_15", "16_23", "24_29"};
const int SFP_BIT_SHIFT[SFP_NUM] = {0, 3};

#define EEPROM_ADDR (0x50)

static int ufi_port_to_cpld_addr(int port)
{
    int cpld_addr = 0;
    
    if (IS_QSFP(port)) {
        cpld_addr = CPLD_BASE_ADDR[1];
    } else if (IS_QSFPDD(port)) {
        cpld_addr = CPLD_BASE_ADDR[2];
    } else if (IS_SFP(port)) {
        cpld_addr = CPLD_BASE_ADDR[1];
    } else {
        return ONLP_STATUS_E_PARAM;
    }
    return cpld_addr;
}

static int ufi_qsfp_port_to_sysfs_attr_offset(int port)
{
    int sysfs_attr_offset = 0;

    if (IS_QSFPX(port)) {
        sysfs_attr_offset = port/8;    
    } else {
        return ONLP_STATUS_E_PARAM;
    }
    
    return sysfs_attr_offset;
}

static int ufi_qsfp_port_to_bit_offset(int port)
{
    int bit_offset = 0;

    if (IS_QSFPX(port)) {
        bit_offset = port % 8;
    } else {
        return ONLP_STATUS_E_PARAM;
    }
    
    return bit_offset;
}

static int ufi_port_to_eeprom_bus(int port)
{
    int bus = -1;
    int deph_id = 0;
    int hw_id = 0;
	
    if (IS_QSFPX(port)) {
        bus =  port + 25;

        if (port == 26 || port == 27) {
            //i2c bus for port 26 and port 27 are reversed for beta build or older			
            ONLP_TRY(file_read_hex(&deph_id, SYSFS_DEPH_ID));
            ONLP_TRY(file_read_hex(&hw_id, SYSFS_HW_ID));
            
            if (deph_id == 0 && hw_id <= 2) {			
                if (port == 26) {
	             bus +=1;
                } else if (port == 27) {
                    bus -=1;
                }
            }
        }
    } else { //unknown ports
        AIM_LOG_ERROR("unknown ports, port=%d\n", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_PARAM;
    }
    
    return bus;
}

static int ufi_port_to_cpld_bus(int port)
{
    int bus = -1;
    
    if (port < PORT_NUM) {
        bus =  CPLD_I2C_BUS;
    } else { //unknown ports
        AIM_LOG_ERROR("unknown ports, port=%d\n", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_PARAM;
    }
    
    return bus;
}

static int ufi_qsfp_present_get(int port, int *pres_val)
{     
    int reg_val = 0, rc = 0;
    int cpld_bus = 0, cpld_addr = 0, attr_offset = 0;
    char *sysfs_qsfpx_present = NULL;
       
    //get cpld bus, cpld addr and sysfs_attr_offset
    cpld_bus = ufi_port_to_cpld_bus(port);
    cpld_addr = ufi_port_to_cpld_addr(port);
    attr_offset = ufi_qsfp_port_to_sysfs_attr_offset(port);

    if (IS_QSFP(port)) {
        sysfs_qsfpx_present = SYSFS_QSFP_PRESENT;
    } else {
        sysfs_qsfpx_present = SYSFS_QSFPDD_PRESENT;
    }
    
    //read register
    if ((rc = file_read_hex(&reg_val, SYS_FMT_OFFSET, cpld_bus, cpld_addr, sysfs_qsfpx_present, sysfs_attr_suffix[attr_offset])) < 0) {
        AIM_LOG_ERROR("Unable to read sysfs %s", sysfs_qsfpx_present);
        AIM_LOG_ERROR(SYS_FMT_OFFSET, cpld_bus, cpld_addr, sysfs_qsfpx_present, sysfs_attr_suffix[attr_offset]);
        check_and_do_i2c_mux_reset(port);
        return rc;
    }
   
    *pres_val = !((reg_val >> ufi_qsfp_port_to_bit_offset(port)) & 0x1);
    
    return ONLP_STATUS_OK;
}

static int ufi_sfp_present_get(int port, int *pres_val)
{
    int reg_val = 0, rc = 0;
    int cpld_bus = 0, cpld_addr = 0; 

    //get cpld bus and cpld addr
    cpld_bus = ufi_port_to_cpld_bus(port);
    cpld_addr = ufi_port_to_cpld_addr(port);

    //read register
    if ((rc = file_read_hex(&reg_val, SYS_FMT, cpld_bus, cpld_addr, SYSFS_SFP_STATUS)) < 0) {
        AIM_LOG_ERROR("Unable to read sysfs %s", SYSFS_SFP_STATUS);
        AIM_LOG_ERROR(SYS_FMT, cpld_bus, cpld_addr, SYSFS_SFP_STATUS);
        check_and_do_i2c_mux_reset(port);
        return rc;
    }

    //three bits shift for port 1
    reg_val >>= SFP_BIT_SHIFT[SFP_PORT(port)];
    
    *pres_val = !(reg_val & MASK_SFP_PRESENT);
    
    return ONLP_STATUS_OK;
}


/**
 * @brief Initialize the SFPI subsystem.
 */
int onlp_sfpi_init(void)
{  
    lock_init();
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the bitmap of SFP-capable port numbers.
 * @param bmap [out] Receives the bitmap.
 */
int onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{
    int p = 0;

    AIM_BITMAP_CLR_ALL(bmap);
    for(p = 0; p < PORT_NUM; p++) {
        AIM_BITMAP_SET(bmap, p);
    }
    
    return ONLP_STATUS_OK;
}

/**
 * @brief Determine if an SFP is present.
 * @param port The port number.
 * @returns 1 if present
 * @returns 0 if absent
 * @returns An error condition.
 */
int onlp_sfpi_is_present(int port)
{
    int status = ONLP_STATUS_OK;

    VALIDATE_PORT(port);
    
    //QSFPDD Ports
    if (IS_QSFPX(port)) {
        ONLP_TRY(ufi_qsfp_present_get(port, &status));
    } else if (IS_SFP(port)) { //SFP
        ONLP_TRY(ufi_sfp_present_get(port, &status));
    } else {
        return ONLP_STATUS_E_PARAM;
    }

    return status;
}

/**
 * @brief Return the presence bitmap for all SFP ports.
 * @param dst Receives the presence bitmap.
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
 * @param dst Receives the RX_LOS bitmap.
 */
int onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int i = 0, value = 0;

    /* Populate bitmap - QSFP and QSFPDD ports */
    for(i = 0; i < (QSFPX_NUM); i++) {
        AIM_BITMAP_MOD(dst, i, 0);
    }

    /* Populate bitmap - SFP+ ports */
    for(i = QSFPX_NUM; i < PORT_NUM; i++) {
        ONLP_TRY(onlp_sfpi_control_get(i, ONLP_SFP_CONTROL_RX_LOS, &value));
        AIM_BITMAP_MOD(dst, i, value);
    }

    return ONLP_STATUS_OK;
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
    int bus = -1;
    char eeprom_path[128];
    char command[256] = "";
    int ret = ONLP_STATUS_OK;
    int ret_size = 0;
    VALIDATE_PORT(port);
    
    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent. \n", port);
        return ONLP_STATUS_OK;
    }

    if (IS_QSFPX(port)) {
        bus = ufi_port_to_eeprom_bus(port);
        if (onlp_i2c_block_read(bus, devaddr, addr, size, rdata, ONLP_I2C_F_FORCE) < 0) {
            check_and_do_i2c_mux_reset(port);
            return ONLP_STATUS_E_INTERNAL;
        }
    } else if (IS_SFP(port)) {
        /* SFP */        
        if (IS_SFP_P0(port)) {
            /* SFP Port0 */
            snprintf(command, sizeof(command), "ethtool -m %s raw on length %d > /tmp/.sfp.%s.eeprom", SFP0_INTERFACE_NAME, size, SFP0_INTERFACE_NAME);
            snprintf(eeprom_path, sizeof(eeprom_path), "/tmp/.sfp.%s.eeprom", SFP0_INTERFACE_NAME);
        } else if (IS_SFP_P1(port)) {
            /* SFP Port1 */
            snprintf(command, sizeof(command), "ethtool -m %s raw on length %d > /tmp/.sfp.%s.eeprom", SFP1_INTERFACE_NAME, size, SFP1_INTERFACE_NAME);
            snprintf(eeprom_path, sizeof(eeprom_path), "/tmp/.sfp.%s.eeprom", SFP1_INTERFACE_NAME);
        } else {
            AIM_LOG_ERROR("unknown SFP ports, port=%d\n", port);
            return ONLP_STATUS_E_PARAM;
        }

        ret = system(command);
        if (ret != 0) {
            AIM_LOG_ERROR("Unable to read sfp eeprom (port_id=%d), func=%s\n", port, __FUNCTION__);
            return ONLP_STATUS_E_INTERNAL;
        }

        ONLP_TRY(onlp_file_read(rdata, size, &ret_size, eeprom_path));
    } else {
        return ONLP_STATUS_E_PARAM;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Write to an address on the given SFP port's bus.
 */
int onlp_sfpi_dev_write(int port, uint8_t devaddr, uint8_t addr, uint8_t* data, int size)
{    
    int rc = 0;
    int bus = -1;

    VALIDATE_PORT(port);
    
    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }    
        
    if (IS_QSFPX(port)) {
        bus = ufi_port_to_eeprom_bus(port);
        if ((rc=onlp_i2c_write(bus, devaddr, addr, size, data, ONLP_I2C_F_FORCE)) < 0) {
            check_and_do_i2c_mux_reset(port);
        }
    } else if (IS_SFP(port)) {
        return ONLP_STATUS_E_UNSUPPORTED;
    } else {
        return ONLP_STATUS_E_PARAM;
    }
    
    return rc;
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
    int bus = -1;

    VALIDATE_PORT(port);
    
    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if (IS_QSFPX(port)) {
        bus = ufi_port_to_eeprom_bus(port);
        if ((rc=onlp_i2c_readb(bus, devaddr, addr, ONLP_I2C_F_FORCE)) < 0) {
            check_and_do_i2c_mux_reset(port);
        }
    } else if (IS_SFP(port)) {
        return ONLP_STATUS_E_UNSUPPORTED;
    } else {
        return ONLP_STATUS_E_PARAM;
    }
    
    return rc;
}

/**
 * @brief Write a byte to an address on the given SFP port's bus.
 */
int onlp_sfpi_dev_writeb(int port, uint8_t devaddr, uint8_t addr, uint8_t value)
{    
    int rc = 0;
    int bus = -1;

    VALIDATE_PORT(port);
    
    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if (IS_QSFPX(port)) {
        bus = ufi_port_to_eeprom_bus(port);
        if ((rc=onlp_i2c_writeb(bus, devaddr, addr, value, ONLP_I2C_F_FORCE)) < 0) {
            check_and_do_i2c_mux_reset(port);
        }   
    } else if (IS_SFP(port)) {
        return ONLP_STATUS_E_UNSUPPORTED;
    } else {
        return ONLP_STATUS_E_PARAM;
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
    int bus = -1;

    VALIDATE_PORT(port);

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if (IS_QSFPX(port)) {
        bus = ufi_port_to_eeprom_bus(port);
        if ((rc=onlp_i2c_readw(bus, devaddr, addr, ONLP_I2C_F_FORCE)) < 0) {
            check_and_do_i2c_mux_reset(port);
        }   
    } else if (IS_SFP(port)) {
        return ONLP_STATUS_E_UNSUPPORTED;
    } else {
        return ONLP_STATUS_E_PARAM;
    }
    
    return rc;    
}

/**
 * @brief Write a word to an address on the given SFP port's bus.
 */
int onlp_sfpi_dev_writew(int port, uint8_t devaddr, uint8_t addr, uint16_t value)
{
    
    int rc = 0;
    int bus = -1;

    VALIDATE_PORT(port);

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if (IS_QSFPX(port)) {
        bus = ufi_port_to_eeprom_bus(port);
        if ((rc=onlp_i2c_writew(bus, devaddr, addr, value, ONLP_I2C_F_FORCE)) < 0) {
            check_and_do_i2c_mux_reset(port);
        }   
    } else if (IS_SFP(port)) {
        return ONLP_STATUS_E_UNSUPPORTED;
    } else {
        return ONLP_STATUS_E_PARAM;
    }
    
    return rc;
}

/**
 * @brief Perform any actions required after an SFP is inserted.
 * @param port The port number.
 * @param info The SFF Module information structure.
 * @notes Optional
 */
int onlp_sfpi_post_insert(int port, sff_info_t* info)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Returns whether or not the given control is suppport on the given port.
 * @param port The port number.
 * @param control The control.
 * @param rv [out] Receives 1 if supported, 0 if not supported.
 * @note This provided for convenience and is optional.
 * If you implement this function your control_set and control_get APIs
 * will not be called on unsupported ports.
 */
int onlp_sfpi_control_supported(int port, onlp_sfp_control_t control, int* rv)
{
    VALIDATE_PORT(port);
    
    //set unsupported as default value
    *rv = 0;
    
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
 * @param port The port.
 * @param control The control.
 * @param value The value.
 */
int onlp_sfpi_control_set(int port, onlp_sfp_control_t control, int value)
{
    int rc = 0;
    int reg_val = 0;
    int reg_mask = 0;
    int bus = 0;
    int cpld_addr = 0;    
    int attr_offset = 0, bit_offset = 0;
    char *sysfs_attr = NULL;

    VALIDATE_PORT(port);
    
    bus = ufi_port_to_cpld_bus(port);
    cpld_addr = ufi_port_to_cpld_addr(port);

    switch(control)
        {
        case ONLP_SFP_CONTROL_RESET:
            {
                if (IS_QSFPX(port)) {
                    //config sysfs_attr
                    sysfs_attr = IS_QSFP(port) ? SYSFS_QSFP_RESET : SYSFS_QSFPDD_RESET;
                                        
                    //read reg_val
                    attr_offset = ufi_qsfp_port_to_sysfs_attr_offset(port);                    
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, bus, cpld_addr, sysfs_attr, sysfs_attr_suffix[attr_offset]) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //update reg_val
                    //0 is normal, 1 is reset, reverse value to fit our platform
                    bit_offset = ufi_qsfp_port_to_bit_offset(port);
                    reg_val = ufi_bit_operation(reg_val, bit_offset, !value);

                    //write reg_val
                    if ((rc=onlp_file_write_int(reg_val, SYS_FMT_OFFSET, bus, cpld_addr, sysfs_attr, sysfs_attr_suffix[attr_offset])) < 0) {
                        AIM_LOG_ERROR("Unable to write %s, error=%d, reg_val=%x", sysfs_attr,  rc, reg_val);
                        AIM_LOG_ERROR(SYS_FMT_OFFSET, bus, cpld_addr, sysfs_attr, sysfs_attr_suffix[attr_offset]);
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
                    reg_mask = MASK_SFP_TX_DISABLE << SFP_BIT_SHIFT[SFP_PORT(port)];
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
                    //config sysfs_attr
                    sysfs_attr = IS_QSFP(port) ? SYSFS_QSFP_LPMODE : SYSFS_QSFPDD_LPMODE;
                    
                    //read reg_val
                    attr_offset = ufi_qsfp_port_to_sysfs_attr_offset(port);                    
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, bus, cpld_addr, sysfs_attr, sysfs_attr_suffix[attr_offset]) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //update reg_val
                    //0 is normal, 1 is low power
                    bit_offset = ufi_qsfp_port_to_bit_offset(port);
                    reg_val = ufi_bit_operation(reg_val, bit_offset, value);

                    //write reg_val
                    if (onlp_file_write_int(reg_val, SYS_FMT_OFFSET, bus, cpld_addr, sysfs_attr, sysfs_attr_suffix[attr_offset]) < 0) {
                        AIM_LOG_ERROR("Unable to write %s, error=%d, reg_val=%x", sysfs_attr,  rc, reg_val);
                        AIM_LOG_ERROR(SYS_FMT_OFFSET, bus, cpld_addr, sysfs_attr, sysfs_attr_suffix[attr_offset]);
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
 * @param port The port.
 * @param control The control
 * @param [out] value Receives the current value.
 */
int onlp_sfpi_control_get(int port, onlp_sfp_control_t control, int* value)
{
    int rc = 0;
    int reg_val = 0, reg_mask = 0;
    int bus = 0;
    int cpld_addr = 0;    
    int attr_offset = 0, bit_offset = 0;
    char *sysfs_attr = NULL;

    VALIDATE_PORT(port);
    
    bus = ufi_port_to_cpld_bus(port);
    cpld_addr = ufi_port_to_cpld_addr(port);

    switch(control)
        {
        case ONLP_SFP_CONTROL_RESET_STATE:
            {
                if (IS_QSFPX(port)) {
                    //config sysfs_attr
                    sysfs_attr = IS_QSFP(port) ? SYSFS_QSFP_RESET : SYSFS_QSFPDD_RESET;
                    
                    //read reg_val
                    attr_offset = ufi_qsfp_port_to_sysfs_attr_offset(port);                    
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, bus, cpld_addr, sysfs_attr, sysfs_attr_suffix[attr_offset]) < 0) {
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
                    rc = ONLP_STATUS_E_PARAM;
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
                    reg_mask = MASK_SFP_RX_LOS << SFP_BIT_SHIFT[SFP_PORT(port)];
                    *value = ufi_mask_shift(reg_val, reg_mask);

                    rc = ONLP_STATUS_OK;
                } else {
                    rc = ONLP_STATUS_E_PARAM;
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
                    reg_mask = MASK_SFP_TX_FAULT << SFP_BIT_SHIFT[SFP_PORT(port)];
                    *value = ufi_mask_shift(reg_val, reg_mask);

                    rc = ONLP_STATUS_OK;
                } else {
                    rc = ONLP_STATUS_E_PARAM;
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
                    reg_mask = MASK_SFP_TX_DISABLE << SFP_BIT_SHIFT[SFP_PORT(port)];
                    *value = ufi_mask_shift(reg_val, reg_mask);

                    rc = ONLP_STATUS_OK;
                } else {
                    rc = ONLP_STATUS_E_PARAM;
                }
                break;
            }
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                if (IS_QSFPX(port)) {
                    //config sysfs_attr
                    sysfs_attr = IS_QSFP(port) ? SYSFS_QSFP_LPMODE : SYSFS_QSFPDD_LPMODE;
                    
                    //read reg_val
                    attr_offset = ufi_qsfp_port_to_sysfs_attr_offset(port);                    
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, bus, cpld_addr, sysfs_attr, sysfs_attr_suffix[attr_offset]) < 0) {
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
                    rc = ONLP_STATUS_E_PARAM;
                }
                break;
            }                
        default:
            rc = ONLP_STATUS_E_PARAM;
        }

    return rc;
}

/**
 * @brief Remap SFP user SFP port numbers before calling the SFPI interface.
 * @param port The user SFP port number.
 * @param [out] rport Receives the new port.
 * @note This function will be called to remap the user SFP port number
 * to the number returned in rport before the SFPI functions are called.
 * This is an optional convenience for platforms with dynamic or
 * variant physical SFP numbering.
 */
int onlp_sfpi_port_map(int port, int* rport)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Deinitialize the SFP driver.
 */
int onlp_sfpi_denit(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Generic debug status information.
 * @param port The port number.
 * @param pvs The output pvs.
 * @notes The purpose of this vector is to allow reporting of internal debug
 * status and information from the platform driver that might be used to debug
 * SFP runtime issues.
 * For example, internal equalizer settings, tuning status information, status
 * of additional signals useful for system debug but not exposed in this interface.
 *
 * @notes This is function is optional.
 */
void onlp_sfpi_debug(int port, aim_pvs_t* pvs)
{

}

/**
 * @brief Generic ioctl
 * @param port The port number
 * @param The variable argument list of parameters.
 *
 * @notes This generic ioctl interface can be used
 * for platform-specific or driver specific features
 * that cannot or have not yet been defined in this
 * interface. It is intended as a future feature expansion
 * support mechanism.
 *
 * @notes Optional
 */
int onlp_sfpi_ioctl(int port, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Read the SFP EEPROM.
 * @param port The port number.
 * @param data Receives the SFP data.
 */
int onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{   
    int size = 0, expect_size = 256, bus = 0, rc = 0;
    char eeprom_path[128];
    char command[256] = "";
    int ret = ONLP_STATUS_OK;
    int ret_size = 0;
    VALIDATE_PORT(port);
    
    memset(data, 0, expect_size);

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent. \n", port);
        return ONLP_STATUS_OK;
    }

    if (IS_QSFPX(port)) {
        bus = ufi_port_to_eeprom_bus(port);
        if((rc = onlp_file_read(data, expect_size, &size, SYS_FMT, bus, EEPROM_ADDR, SYSFS_EEPROM)) < 0) {
            AIM_LOG_ERROR("Unable to read eeprom from port(%d)", port);
            AIM_LOG_ERROR(SYS_FMT, bus, EEPROM_ADDR, SYSFS_EEPROM);
        
            check_and_do_i2c_mux_reset(port);
            return rc;
        }

        if (size != expect_size) {
            AIM_LOG_ERROR("Unable to read eeprom from port(%d), size is different!", port);
            return ONLP_STATUS_E_INTERNAL;
        }
    } else if (IS_SFP(port)) {
        /* SFP */        
        if (IS_SFP_P0(port)) {
            /* SFP Port0 */
            snprintf(command, sizeof(command), "ethtool -m %s raw on length %d > /tmp/.sfp.%s.eeprom", SFP0_INTERFACE_NAME, expect_size, SFP0_INTERFACE_NAME);
            snprintf(eeprom_path, sizeof(eeprom_path), "/tmp/.sfp.%s.eeprom", SFP0_INTERFACE_NAME);
        } else if (IS_SFP_P1(port)) {
            /* SFP Port1 */
            snprintf(command, sizeof(command), "ethtool -m %s raw on length %d > /tmp/.sfp.%s.eeprom", SFP1_INTERFACE_NAME, expect_size, SFP1_INTERFACE_NAME);
            snprintf(eeprom_path, sizeof(eeprom_path), "/tmp/.sfp.%s.eeprom", SFP1_INTERFACE_NAME);
        } else {
            AIM_LOG_ERROR("unknown SFP ports, port=%d\n", port);
            return ONLP_STATUS_E_PARAM;
        }

        ret = system(command);
        if (ret != 0) {
            AIM_LOG_ERROR("Unable to read sfp eeprom (port_id=%d), func=%s\n", port, __FUNCTION__);
            return ONLP_STATUS_E_INTERNAL;
        }
        
        ONLP_TRY(onlp_file_read(data, expect_size, &ret_size, eeprom_path));
    } else {
        return ONLP_STATUS_E_PARAM;
    }
    
    return ONLP_STATUS_OK;
}

/**
 * @brief Read the SFP DOM EEPROM.
 * @param port The port number.
 * @param data Receives the SFP data.
 */
int onlp_sfpi_dom_read(int port, uint8_t data[256])
{
    char eeprom_path[512];
    FILE* fp = NULL;
    int bus = 0;

    //sfp dom is on 0x51 (2nd 256 bytes)
    //qsfp dom is on lower page 0x00
    //qsfpdd 2.0 dom is on lower page 0x00
    //qsfpdd 3.0 and later dom and above is on lower page 0x00 and higher page 0x17
    VALIDATE_SFP_PORT(port);
    
    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    } else if (IS_SFP(port)) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }
    
    memset(data, 0, 256);
    memset(eeprom_path, 0, sizeof(eeprom_path));

    //set eeprom_path
    bus = ufi_port_to_eeprom_bus(port);
    snprintf(eeprom_path, sizeof(eeprom_path), SYS_FMT, bus, EEPROM_ADDR, SYSFS_EEPROM);

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

    int ret = fread(data, 1, 256, fp);
    fclose(fp);
    if (ret != 256) {
        AIM_LOG_ERROR("Unable to read the module_eeprom device file of port(%d)", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}
