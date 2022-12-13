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

#define SFP_NUM             54
#define PORT_NUM            SFP_NUM
#define SFP_BASE_BUS        18
#define EEPROM_SYS_FMT      "/sys/bus/i2c/devices/%d-0050/eeprom"
/* SYSFS ATTR */
#define MB_CPLD_SFP_GROUP_PRES_ATTR_FMT     "cpld_sfp_present_%s"
#define MB_CPLD_SFP_GROUP_TXFLT_ATTR_FMT    "cpld_sfp_tx_fault_%s"
#define MB_CPLD_SFP_GROUP_TXDIS_ATTR_FMT    "cpld_sfp_tx_disable_%s"
#define MB_CPLD_SFP_GROUP_RXLOS_ATTR_FMT    "cpld_sfp_rx_los_%s"

#define VALIDATE_PORT(p) { if((p < 0) || (p >= PORT_NUM)) return ONLP_STATUS_E_PARAM; }

typedef enum port_type_e {
    TYPE_SFP = 0,
    TYPE_UNNKOW,
    TYPE_MAX,
} port_type_t;

typedef struct port_type_info_s
{
    port_type_t type;
    int port_index;
    int port_group;
    int eeprom_bus_index;
}port_type_info_t;

static char* port_type_str[] = {
    "sfp+",
};

static int port_eeprom_bus_base[] = {
    SFP_BASE_BUS,
};

const char * sfp_group_str[] = {
    "0_7",
    "8_15",
    "16_23",
    "24_31",
    "32_39",
    "40_47",
    "48_53",
};

port_type_info_t port_num_to_type(int port)
{
    port_type_info_t port_type_info;

    if(port < SFP_NUM) { //SFP+
        port_type_info.type = TYPE_SFP;
        port_type_info.port_index = port % 8;
        port_type_info.port_group = port / 8;
        port_type_info.eeprom_bus_index = port;
    } else { //unkonwn ports
        AIM_LOG_ERROR("port_num mapping to type fail, port=%d\n", port);
        port_type_info.type = TYPE_UNNKOW;
        port_type_info.port_index = -1;
    }
    return port_type_info;
}

int sfp_present_get(port_type_info_t port_info, int *pres_val)
{
    int group_pres;
    int port_pres;
    char sysfs[128];
    char *cpld_path;

    switch(port_info.port_group) {
        case 0:
        case 1:
        case 2:
            cpld_path = MB_CPLD2_SYSFS_PATH;
            break;
        case 3:
        case 4:
        case 5:
        case 6:
            cpld_path = MB_CPLD3_SYSFS_PATH;
            break;
        default:
            AIM_LOG_ERROR("invalid port group=%d", port_info.port_group);
    }

    snprintf(sysfs, sizeof(sysfs), "%s/"MB_CPLD_SFP_GROUP_PRES_ATTR_FMT,
                cpld_path, sfp_group_str[port_info.port_group]);

    ONLP_TRY(file_read_hex(&group_pres, sysfs));


    //val 0 for presence, pres_val set to 1
    port_pres = READ_BIT(group_pres, port_info.port_index);
    if(port_pres) {
        *pres_val = 0;
    } else {
        *pres_val = 1;
    }

    return ONLP_STATUS_OK;
}

int sfp_control_set(port_type_info_t port_info, onlp_sfp_control_t control, int value)
{
    char sysfs[128];
    char *cpld_sysfs_path;
    char attr[64];
    int reg_val;
    int rc;

    switch(port_info.port_group) {
        case 0:
        case 1:
        case 2:
            cpld_sysfs_path = MB_CPLD2_SYSFS_PATH;
            break;
        case 3:
        case 4:
        case 5:
        case 6:
            cpld_sysfs_path = MB_CPLD3_SYSFS_PATH;
            break;
        default:
            AIM_LOG_ERROR("unknown ports, port=%d\n", port_info.port_index);
            return ONLP_STATUS_E_UNSUPPORTED;
    }

    //sysfs path
    if(control == ONLP_SFP_CONTROL_TX_DISABLE) {
        snprintf(attr, sizeof(attr), MB_CPLD_SFP_GROUP_TXDIS_ATTR_FMT,
                    sfp_group_str[port_info.port_group]);
        snprintf(sysfs, sizeof(sysfs), "%s/%s", cpld_sysfs_path, attr);
    } else {
        return ONLP_STATUS_E_UNSUPPORTED;
    }


    ONLP_TRY(file_read_hex(&reg_val, sysfs));

    //update reg val
    if(value) {
        SET_BIT(reg_val, port_info.port_index);
    } else {
        CLEAR_BIT(reg_val, port_info.port_index);
    }

    //write reg val back
    if((rc = onlp_file_write_int(reg_val, sysfs))
            != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Unable to write sysfs value, error=%d, \
            sysfs=%s, reg_val=%x", rc, sysfs, reg_val);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int sfp_control_get(port_type_info_t port_info, onlp_sfp_control_t control, int* value)
{
    int reg_val = 0;
    char sysfs[128];
    char *cpld_sysfs_path;
    char attr[64];

    switch(port_info.port_group) {
        case 0:
        case 1:
        case 2:
            cpld_sysfs_path = MB_CPLD2_SYSFS_PATH;
            break;
        case 3:
        case 4:
        case 5:
        case 6:
            cpld_sysfs_path = MB_CPLD3_SYSFS_PATH;
            break;
        default:
            AIM_LOG_ERROR("unknown port group=%d\n", port_info.port_group);
            return ONLP_STATUS_E_UNSUPPORTED;
    }

    //sysfs path
    if(control == ONLP_SFP_CONTROL_RX_LOS) {
        snprintf(attr, sizeof(attr), MB_CPLD_SFP_GROUP_RXLOS_ATTR_FMT,
                    sfp_group_str[port_info.port_group]);
        snprintf(sysfs, sizeof(sysfs), "%s/%s", cpld_sysfs_path, attr);
    } else if(control == ONLP_SFP_CONTROL_TX_FAULT) {
        snprintf(attr, sizeof(attr), MB_CPLD_SFP_GROUP_TXFLT_ATTR_FMT,
                    sfp_group_str[port_info.port_group]);
        snprintf(sysfs, sizeof(sysfs), "%s/%s", cpld_sysfs_path, attr);
    } else if(control == ONLP_SFP_CONTROL_TX_DISABLE) {
        snprintf(attr, sizeof(attr), MB_CPLD_SFP_GROUP_TXDIS_ATTR_FMT,
                    sfp_group_str[port_info.port_group]);
        snprintf(sysfs, sizeof(sysfs), "%s/%s", cpld_sysfs_path, attr);
    } else {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    ONLP_TRY(file_read_hex(&reg_val, sysfs));

    *value = READ_BIT(reg_val, port_info.port_index);
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
    int p;
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
    int status = 1;
    port_type_info_t port_type_info;

    VALIDATE_PORT(port);

    port_type_info = port_num_to_type(port);

    switch(port_type_info.type) {
        case TYPE_SFP:
            if(sfp_present_get(port_type_info, &status) != ONLP_STATUS_OK) {
                AIM_LOG_ERROR("sfp_presnet_get() failed, port=%d\n", port);
                return ONLP_STATUS_E_INTERNAL;
            }
            break;
         default:
            AIM_LOG_ERROR("unknown ports, port=%d\n", port);
            return ONLP_STATUS_E_UNSUPPORTED;
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
    int rc = 0;

    for (p = 0; p < PORT_NUM; p++) {
        if((rc = onlp_sfpi_is_present(p)) < 0) {
            return rc;
        }
        AIM_BITMAP_MOD(dst, p, rc);
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Return the RX_LOS bitmap for all SFP ports.
 * @param dst Receives the RX_LOS bitmap.
 */
int onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int i=0, value=0;

    for(i = 0; i < PORT_NUM; i++) {
        if(onlp_sfpi_control_get(i, ONLP_SFP_CONTROL_RX_LOS, &value) != ONLP_STATUS_OK) {
            AIM_BITMAP_MOD(dst, i, 0);  //set default value for port which has no cap
        } else {
            AIM_BITMAP_MOD(dst, i, value);
        }
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Read the SFP EEPROM.
 * @param port The port number.
 * @param data Receives the SFP data.
 */
int onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{
    char eeprom_path[128];
    int size = 0;
    port_type_info_t port_type_info;

    memset(eeprom_path, 0, sizeof(eeprom_path));
    memset(data, 0, 256);


    port_type_info = port_num_to_type(port);

    if( port_type_info.type == TYPE_UNNKOW) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    snprintf(eeprom_path, sizeof(eeprom_path), EEPROM_SYS_FMT,
                port_eeprom_bus_base[port_type_info.type]+port_type_info.eeprom_bus_index);

    if(onlp_file_read(data, 256, &size, eeprom_path) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Unable to read eeprom for %s port(%d) sysfs: %s\r\n",
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
    int bus;

    port_type_info = port_num_to_type(port);

    if( port_type_info.type == TYPE_UNNKOW) {
        return ONLP_STATUS_E_PARAM;
    }

    bus = port_eeprom_bus_base[port_type_info.type] + port_type_info.eeprom_bus_index;

    if((rc=onlp_i2c_readb(bus, devaddr, addr, ONLP_I2C_F_FORCE))<0) {
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
    port_type_info_t port_type_info;
    int bus;

    port_type_info = port_num_to_type(port);

    if( port_type_info.type == TYPE_UNNKOW) {
        return ONLP_STATUS_E_PARAM;
    }

    bus = port_eeprom_bus_base[port_type_info.type] + port_type_info.eeprom_bus_index;

    if((rc=onlp_i2c_writeb(bus, devaddr, addr, value, ONLP_I2C_F_FORCE))<0) {
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
    int bus;

    port_type_info = port_num_to_type(port);

    if( port_type_info.type == TYPE_UNNKOW) {
        return ONLP_STATUS_E_PARAM;
    }

    bus = port_eeprom_bus_base[port_type_info.type] + port_type_info.eeprom_bus_index;

    if((rc=onlp_i2c_readw(bus, devaddr, addr, ONLP_I2C_F_FORCE))<0) {
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
    int bus;

    port_type_info = port_num_to_type(port);

    if( port_type_info.type == TYPE_UNNKOW) {
        return ONLP_STATUS_E_PARAM;
    }

    bus = port_eeprom_bus_base[port_type_info.type] + port_type_info.eeprom_bus_index;

    if((rc=onlp_i2c_writew(bus, devaddr, addr, value, ONLP_I2C_F_FORCE))<0) {
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
    int bus;

    port_type_info = port_num_to_type(port);

    if( port_type_info.type == TYPE_UNNKOW) {
        return ONLP_STATUS_E_PARAM;
    }

    bus = port_eeprom_bus_base[port_type_info.type] + port_type_info.eeprom_bus_index;

    if((rc=onlp_i2c_block_read(bus, devaddr, addr, size, rdata, ONLP_I2C_F_FORCE)) < 0) {
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
    int bus;

    port_type_info = port_num_to_type(port);

    if( port_type_info.type == TYPE_UNNKOW) {
        return ONLP_STATUS_E_PARAM;
    }

    bus = port_eeprom_bus_base[port_type_info.type] + port_type_info.eeprom_bus_index;

    if((rc=onlp_i2c_write(bus, devaddr, addr, size, data, ONLP_I2C_F_FORCE))<0) {
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
    int bus = 0;

    VALIDATE_PORT(port);

    //sfp dom is on 0x51 (2nd 256 bytes)
    //qsfp dom is on lower page 0x00
    //qsfpdd 2.0 dom is on lower page 0x00
    //qsfpdd 3.0 and later dom and above is on lower page 0x00 and higher page 0x17
    port_type_info = port_num_to_type(port);

    if(port_type_info.type == TYPE_UNNKOW) {
        return ONLP_STATUS_E_PARAM;
    }

    memset(data, 0, 256);
    memset(eeprom_path, 0, sizeof(eeprom_path));

    //set eeprom_path
    bus = port_eeprom_bus_base[port_type_info.type] + port_type_info.eeprom_bus_index;
    snprintf(eeprom_path, sizeof(eeprom_path), EEPROM_SYS_FMT, bus);

    //read eeprom
    fp = fopen(eeprom_path, "r");
    if(fp == NULL) {
        AIM_LOG_ERROR("Unable to open the eeprom device file of port(%d)", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }

    if(fseek(fp, 256, SEEK_CUR) != 0) {
        fclose(fp);
        AIM_LOG_ERROR("Unable to set the file position indicator of port(%d)", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }

    int ret = fread(data, 1, 256, fp);
    fclose(fp);
    if(ret != 256) {
        AIM_LOG_ERROR("Unable to read the module_eeprom device file of port(%d)", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
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
    port_type_info_t port_type_info;

    VALIDATE_PORT(port);

    port_type_info = port_num_to_type(port);

    //set unsupported as default value
    *rv=0;

    switch(control) {
        case ONLP_SFP_CONTROL_RESET:
        case ONLP_SFP_CONTROL_RESET_STATE:
        case ONLP_SFP_CONTROL_LP_MODE:
            *rv = 0;
            break;
        case ONLP_SFP_CONTROL_RX_LOS:
        case ONLP_SFP_CONTROL_TX_FAULT:
        case ONLP_SFP_CONTROL_TX_DISABLE:
            if(port_type_info.type == TYPE_SFP) {
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
    port_type_info_t port_type_info;
    int rc;

    VALIDATE_PORT(port);

    port_type_info = port_num_to_type(port);

    switch(port_type_info.type) {
        case TYPE_SFP:
            rc = sfp_control_set(port_type_info, control, value);
            break;
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
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
    port_type_info_t port_type_info;
    int rc;

    VALIDATE_PORT(port);

    port_type_info = port_num_to_type(port);

    switch(port_type_info.type) {
        case TYPE_SFP:
            rc = sfp_control_get(port_type_info, control, value);
            break;
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
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
    return;
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