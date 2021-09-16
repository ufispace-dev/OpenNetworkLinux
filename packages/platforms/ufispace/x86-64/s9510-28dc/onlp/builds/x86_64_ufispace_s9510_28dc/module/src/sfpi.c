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

#define SFP_NUM               24
#define QSFP_NUM              2
#define QSFPDD_NUM            2
#define QSFPX_NUM             (QSFP_NUM+QSFPDD_NUM)
#define PORT_NUM              (SFP_NUM+QSFPX_NUM)

#define SFP_PORT(_port) (port-QSFPX_NUM)

#define IS_SFP(_port)         (_port >= QSFPX_NUM && _port < PORT_NUM)
#define IS_QSFPX(_port)       (_port >= 0 && _port < QSFPX_NUM)
#define IS_QSFP(_port)        (_port >= QSFPDD_NUM && _port < QSFPX_NUM)
#define IS_QSFPDD(_port)      (_port >= 0 && _port < QSFPDD_NUM)

#define SYSFS_EEPROM         "eeprom"

#define EEPROM_ADDR (0x50)

#define VALIDATE_PORT(p) { if ((p < 0) || (p >= PORT_NUM)) return ONLP_STATUS_E_PARAM; }

typedef struct
{
    int abs_gpin;
    int lpmode_gpin;
    int reset_gpin;
    int rxlos_gpin;
    int txfault_gpin;
    int txdis_gpin;
    int eeprom_bus;
} port_attr_t;

static const port_attr_t port_attr[] = {
/*  port  abs   lpmode reset rxlos txfault txdis eeprom */
    [0] ={487  ,491   ,495  ,-1   ,-1     ,-1   ,13},
    [1] ={486  ,490   ,494  ,-1   ,-1     ,-1   ,12},
    [2] ={485  ,489   ,493  ,-1   ,-1     ,-1   ,11},
    [3] ={484  ,488   ,492  ,-1   ,-1     ,-1   ,10},
    [4] ={375  ,-1    ,-1   ,343  ,439    ,471  ,14},
    [5] ={374  ,-1    ,-1   ,342  ,438    ,470  ,15},
    [6] ={373  ,-1    ,-1   ,341  ,437    ,469  ,16},
    [7] ={372  ,-1    ,-1   ,340  ,436    ,468  ,17},
    [8] ={371  ,-1    ,-1   ,339  ,435    ,467  ,18},
    [9] ={370  ,-1    ,-1   ,338  ,434    ,466  ,19},
    [10]={369  ,-1    ,-1   ,337  ,433    ,465  ,20},
    [11]={368  ,-1    ,-1   ,336  ,432    ,464  ,21},
    [12]={383  ,-1    ,-1   ,351  ,447    ,479  ,22},
    [13]={382  ,-1    ,-1   ,350  ,446    ,478  ,23},
    [14]={381  ,-1    ,-1   ,349  ,445    ,477  ,24},
    [15]={380  ,-1    ,-1   ,348  ,444    ,476  ,25},
    [16]={379  ,-1    ,-1   ,347  ,443    ,475  ,26},
    [17]={378  ,-1    ,-1   ,346  ,442    ,474  ,27},
    [18]={377  ,-1    ,-1   ,345  ,441    ,473  ,28},
    [19]={376  ,-1    ,-1   ,344  ,440    ,472  ,29},
    [20]={359  ,-1    ,-1   ,327  ,423    ,455  ,30},
    [21]={358  ,-1    ,-1   ,326  ,422    ,454  ,31},
    [22]={357  ,-1    ,-1   ,325  ,421    ,453  ,32},
    [23]={356  ,-1    ,-1   ,324  ,420    ,452  ,33},
    [24]={355  ,-1    ,-1   ,323  ,419    ,451  ,34},
    [25]={354  ,-1    ,-1   ,322  ,418    ,450  ,35},
    [26]={353  ,-1    ,-1   ,321  ,417    ,449  ,36},
    [27]={352  ,-1    ,-1   ,320  ,416    ,448  ,37},
};

int ufi_port_to_gpio_num(int port, onlp_sfp_control_t control)
{
    int gpio_num = -1;

     switch(control)
        {
        case ONLP_SFP_CONTROL_RESET_STATE:
            {
                gpio_num = port_attr[port].reset_gpin;
                break;
            }
        case ONLP_SFP_CONTROL_RX_LOS:
            {
                gpio_num = port_attr[port].rxlos_gpin;
                break;
            }
        case ONLP_SFP_CONTROL_TX_FAULT:
            {
                gpio_num = port_attr[port].txfault_gpin;
                break;
            }
        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                gpio_num = port_attr[port].txdis_gpin;
                break;
            }
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                gpio_num = port_attr[port].lpmode_gpin;
                break;
            }
        default:
            gpio_num=-1;
        }

    if (gpio_num<0) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    return gpio_num;
}

static int ufi_port_to_eeprom_bus(int port)
{
    int bus = -1;

    bus=port_attr[port].eeprom_bus;
    return bus;
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
    int status=ONLP_STATUS_OK, gpio_num;
    int abs = 0, present = 0;

    VALIDATE_PORT(port);

    //set gpio_num by port
    gpio_num=port_attr[port].abs_gpin;

    if ((status = file_read_hex(&abs, SYS_GPIO_FMT, gpio_num)) < 0) {
        AIM_LOG_ERROR("onlp_sfpi_is_present() failed, error=%d, sysfs=%s, gpio_num=%d",
            status, SYS_GPIO_FMT, gpio_num);
        check_and_do_i2c_mux_reset(port);
        return status;
    }

    present = (abs==0) ? 1:0;

    return present;
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
        if ((rc = onlp_sfpi_is_present(p)) < 0) {
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

    /* Populate bitmap - QSFPDD and QSFP ports */
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
 * @brief Read the SFP EEPROM.
 * @param port The port number.
 * @param data Receives the SFP data.
 */
int onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{
    int size = 0, bus = 0, rc = 0;

    VALIDATE_PORT(port);

    memset(data, 0, 256);
    bus = ufi_port_to_eeprom_bus(port);

    if((rc = onlp_file_read(data, 256, &size, SYS_FMT, bus, EEPROM_ADDR, SYSFS_EEPROM)) < 0) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d)", port);
        AIM_LOG_ERROR(SYS_FMT, bus, EEPROM_ADDR, SYSFS_EEPROM);

        check_and_do_i2c_mux_reset(port);
        return rc;
    }

    if (size != 256) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d), size is different!", port);
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

    if (onlp_sfpi_is_present(port) !=  1) {
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

    if (onlp_sfpi_is_present(port) !=  1) {
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

    if(onlp_sfpi_is_present(port) !=  1) {
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

    if(onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }
    
    if((rc = onlp_i2c_writew(bus, devaddr, addr, value, ONLP_I2C_F_FORCE)) < 0) {
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
    int bus = ufi_port_to_eeprom_bus(port);

    if (onlp_sfpi_is_present(port) !=  1) {
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

    VALIDATE_PORT(port);

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
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
    int gpio_num = 0;

    VALIDATE_PORT(port);

    //check control is valid for this port
    switch(control)
        {
        case ONLP_SFP_CONTROL_RESET:
            {
                if (IS_QSFPX(port)) {
                    //reverse value
                    value = (value == 0) ? 1:0;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                if (IS_SFP(port)) {
                    break;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
            }
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                if (IS_QSFPX(port)) {
                    break;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
            }
        default:
            rc = ONLP_STATUS_E_UNSUPPORTED;
        }

    //get gpio_num
    if ((rc = ufi_port_to_gpio_num(port, control)) < 0) {
        AIM_LOG_ERROR("ufi_port_to_gpio_num() failed, error=%d, port=%d, control=%d", rc, port, control);
        return rc;
    } else {
        gpio_num = rc;
    }

    //write gpio value
    if ((rc = onlp_file_write_int(value, SYS_GPIO_FMT, gpio_num)) < 0) {
        AIM_LOG_ERROR("onlp_sfpi_control_set() failed, error=%d, sysfs=%s, gpio_num=%d", rc, SYS_GPIO_FMT, gpio_num);
        check_and_do_i2c_mux_reset(port);
        return rc;
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
    int rc;
    int gpio_num = 0;

    VALIDATE_PORT(port);

    //get gpio_num
    if ((rc = ufi_port_to_gpio_num(port, control)) < 0) {
        AIM_LOG_ERROR("ufi_port_to_gpio_num() failed, error=%d, port=%d, control=%d", rc, port, control);
        return rc;
    } else {
        gpio_num = rc;
    }

    //read gpio value
    if ((rc = file_read_hex(value, SYS_GPIO_FMT, gpio_num)) < 0) {
        AIM_LOG_ERROR("onlp_sfpi_control_get() failed, error=%d, sysfs=%s, gpio_num=%d", rc, SYS_GPIO_FMT, gpio_num);
        check_and_do_i2c_mux_reset(port);
        return rc;
    }

    //reverse bit
    if (control == ONLP_SFP_CONTROL_RESET_STATE) {
        *value = !(*value);
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

