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
#include <onlp/platformi/sfpi.h>

#include "x86_64_ufispace_s9501_28smt_log.h"
#include "platform_lib.h"

#define IS_SFP(_port)   (_port >= SFP_START_NUM && _port < PORT_NUM)

#define SYSFS_EEPROM    "eeprom"
#define EEPROM_ADDR     (0x50)

#define VALIDATE_PORT(p) { if ((p < SFP_START_NUM) || (p >= PORT_NUM)) return ONLP_STATUS_E_PARAM; }
#define VALIDATE_SFP_PORT(p) { if (!IS_SFP(p)) return ONLP_STATUS_E_PARAM; }

static int ufi_port_to_eeprom_bus(int port)
{
    int bus = -1;

    if (IS_SFP(port)) { //SFP
        bus =  port + 6;
    } else { //unknown ports
        AIM_LOG_ERROR("unknown ports, port=%d\n", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    return bus;
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
    for(p = SFP_START_NUM; p < PORT_NUM; p++) {
        AIM_BITMAP_SET(bmap, p);
    }
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_is_present(int port)
{
    int status=1;

    //SFP and SFP+ Ports
    if (port >= SFP_START_NUM && port < PORT_NUM) {
        if (sfp_present_get(port, &status) != ONLP_STATUS_OK) {
            AIM_LOG_ERROR("onlp_sfpi_is_present() failed, port=%d\n", port);
            check_and_do_i2c_mux_reset(port);
            return ONLP_STATUS_E_INTERNAL;
        }
    } else { //unknown ports
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

    for (p = SFP_START_NUM; p < PORT_NUM; p++) {
        rc = onlp_sfpi_is_present(p);
        AIM_BITMAP_MOD(dst, p, rc);
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
    int size = 0;
    int bus = -1;

    VALIDATE_PORT(port);

    memset(data, 0, 256);

    bus = ufi_port_to_eeprom_bus(port);

    if (bus < 0) {
        AIM_LOG_ERROR("unknown bus, bus=%d\n", bus);
        return bus;
    }

    if(onlp_file_read(data, 256, &size, SYS_FMT, bus, EEPROM_ADDR, SYSFS_EEPROM) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d)\r\n", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (size != 256) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d), size is different!\r\n", port);
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
    int bus = ufi_port_to_eeprom_bus(port);

    if (bus < 0) {
        AIM_LOG_ERROR("unknown bus, bus=%d\n", bus);
        return bus;
    }

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if ((rc = onlp_i2c_readb(bus, devaddr, addr, ONLP_I2C_F_FORCE)) < 0) {
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
    int bus = ufi_port_to_eeprom_bus(port);

    if (bus < 0) {
        AIM_LOG_ERROR("unknown bus, bus=%d\n", bus);
        return bus;
    }

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if ((rc = onlp_i2c_writeb(bus, devaddr, addr, value, ONLP_I2C_F_FORCE)) < 0) {
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
    int bus = ufi_port_to_eeprom_bus(port);

    if (bus < 0) {
        AIM_LOG_ERROR("unknown bus, bus=%d\n", bus);
        return bus;
    }

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if((rc = onlp_i2c_readw(bus, devaddr, addr, ONLP_I2C_F_FORCE)) < 0) {
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
    int bus = ufi_port_to_eeprom_bus(port);

    if (bus < 0) {
        AIM_LOG_ERROR("unknown bus, bus=%d\n", bus);
        return bus;
    }

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if ((rc = onlp_i2c_writew(bus, devaddr, addr, value, ONLP_I2C_F_FORCE)) < 0) {
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
    int bus = -1;

    VALIDATE_PORT(port);

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    devaddr = EEPROM_ADDR;
    bus = ufi_port_to_eeprom_bus(port);

    if (bus < 0) {
        AIM_LOG_ERROR("unknown bus, bus=%d\n", bus);
        return bus;
    }

    if (port < PORT_NUM) {
        if (onlp_i2c_block_read(bus, devaddr, addr, size, rdata, ONLP_I2C_F_FORCE) < 0) {
            check_and_do_i2c_mux_reset(port);
            return ONLP_STATUS_E_INTERNAL;
        }
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
    VALIDATE_PORT(port);
    int rc = 0;
    int bus = ufi_port_to_eeprom_bus(port);

    if (bus < 0) {
        AIM_LOG_ERROR("unknown bus, bus=%d\n", bus);
        return bus;
    }

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if ((rc = onlp_i2c_write(bus, devaddr, addr, size, data, ONLP_I2C_F_FORCE)) < 0) {
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
    int bus = 0;

    //sfp dom is on 0x51 (2nd 256 bytes)
    //qsfp dom is on lower page 0x00
    //qsfpdd 2.0 dom is on lower page 0x00
    //qsfpdd 3.0 and later dom and above is on lower page 0x00 and higher page 0x17
    VALIDATE_SFP_PORT(port);

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    memset(data, 0, 256);
    memset(eeprom_path, 0, sizeof(eeprom_path));

    //set eeprom_path
    bus = ufi_port_to_eeprom_bus(port);
    if (bus < 0) {
        AIM_LOG_ERROR("unknown bus, bus=%d\n", bus);
        return bus;
    }
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
    *rv=0;

    switch (control) {
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

int
onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int i=0, value=0, rc=0;

    /* Populate bitmap - SFP and SFP+ ports*/
    for(i = SFP_START_NUM; i < PORT_NUM; i++) {
        if ((rc=onlp_sfpi_control_get(i, ONLP_SFP_CONTROL_RX_LOS, &value)) != ONLP_STATUS_OK) {
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
    int rc, gpio_num=0;
    char sysfs[48];

    memset(sysfs, 0, sizeof(sysfs));

    //port range check
    if (port < SFP_START_NUM || port >= PORT_NUM) {
        AIM_LOG_ERROR("onlp_sfpi_control_get() invalid port, port=%d", port);
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    switch(control)
    {
        case ONLP_SFP_CONTROL_RX_LOS:
        {
            //set gpio_num by port and control
            if (port>=4 && port<8) {
                gpio_num = 359 - port;
            } else if (port>=8 && port<16) {
                gpio_num = 367 - (port-8);
            } else if (port>=16 && port<24) {
                gpio_num = 343 - (port-16);
            } else if (port>=24 && port<28) {
                gpio_num = 351 - (port-24);
            }
            break;
        }
        case ONLP_SFP_CONTROL_TX_FAULT:
        {
            //set gpio_num by port and control
            if (port>=4 && port<8) {
                gpio_num = 455 - port;
            } else if (port>=8 && port<16) {
                gpio_num = 463 - (port-8);
            } else if (port>=16 && port<24) {
                gpio_num = 439 - (port-16);
            } else if (port>=24 && port<28) {
                gpio_num = 447 - (port-24);
            }
            break;
        }
        case ONLP_SFP_CONTROL_TX_DISABLE:
        {
            //set gpio_num by port and control
            if (port>=4 && port<8) {
                gpio_num = 487 - port;
            } else if (port>=8 && port<16) {
                gpio_num = 495 - (port-8);
            } else if (port>=16 && port<24) {
                gpio_num = 471 - (port-16);
            } else if (port>=24 && port<28) {
                gpio_num = 479 - (port-24);
            }
            break;
        }
        default:
            rc = ONLP_STATUS_E_UNSUPPORTED;
            return rc;
    }

    snprintf(sysfs, sizeof(sysfs), SYS_GPIO_VAL, gpio_num);

    if ((rc = onlp_file_read_int(value, sysfs)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Unable to read gpio, error=%s, sysfs=%s, port=%d, gpio_num=%d, control=%d", onlp_status_name(rc), sysfs, port, gpio_num, control);
        return ONLP_STATUS_E_INTERNAL;
    }
    rc = ONLP_STATUS_OK;

    return rc;
}

int
onlp_sfpi_control_set(int port, onlp_sfp_control_t control, int value)
{
    int rc, gpio_num;
    char sysfs[48];

    memset(sysfs, 0, sizeof(sysfs));

    //port range check
    if (port < SFP_START_NUM || port >= PORT_NUM) {
        AIM_LOG_ERROR("%s() invalid port, port=%d", __func__, port);
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    switch(control)
    {
        case ONLP_SFP_CONTROL_RX_LOS:
        {
            //set gpio_num by port and control
            if (port>=4 && port<8) {
                gpio_num = 359 - port;
            } else if (port>=8 && port<16) {
                gpio_num = 367 - (port-8);
            } else if (port>=16 && port<24) {
                gpio_num = 343 - (port-16);
            } else if (port>=24 && port<28) {
                gpio_num = 351 - (port-24);
            }
            break;
        }
        case ONLP_SFP_CONTROL_TX_FAULT:
        {
            //set gpio_num by port and control
            if (port>=4 && port<8) {
                gpio_num = 455 - port;
            } else if (port>=8 && port<16) {
                gpio_num = 463 - (port-8);
            } else if (port>=16 && port<24) {
                gpio_num = 439 - (port-16);
            } else if (port>=24 && port<28) {
                gpio_num = 447 - (port-24);
            }
            break;
        }
        case ONLP_SFP_CONTROL_TX_DISABLE:
        {
            //set gpio_num by port and control
            if (port>=4 && port<8) {
                gpio_num = 487 - port;
            } else if (port>=8 && port<16) {
                gpio_num = 495 - (port-8);
            } else if (port>=16 && port<24) {
                gpio_num = 471 - (port-16);
            } else if (port>=24 && port<28) {
                gpio_num = 479 - (port-24);
            }
            break;
        }
        default:
            rc = ONLP_STATUS_E_UNSUPPORTED;
            return rc;
    }

    snprintf(sysfs, sizeof(sysfs), SYS_GPIO_VAL, gpio_num);

    if ((rc = onlp_file_write_int(value, sysfs)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("%s() Unable to write gpio, error=%d, sysfs=%s", __func__, rc, sysfs);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }
    rc = ONLP_STATUS_OK;

    return rc;
}

/*
 * De-initialize the SFPI subsystem.
 */
int
onlp_sfpi_denit(void)
{
    return ONLP_STATUS_OK;
}
