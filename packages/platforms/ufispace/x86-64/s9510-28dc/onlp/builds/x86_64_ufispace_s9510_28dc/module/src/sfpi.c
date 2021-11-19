/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <onlp/platformi/sfpi.h>
#include <onlplib/i2c.h>
#include <onlplib/file.h>
#include "platform_lib.h"

#define SFP_NUM               24
#define QSFP_NUM              2
#define QSFPDD_NUM            2
#define QSFPX_NUM             (QSFP_NUM+QSFPDD_NUM)
#define PORT_NUM              (SFP_NUM+QSFPX_NUM)

#define SYSFS_EEPROM         "eeprom"
#define EEPROM_ADDR (0x50)
#define VALIDATE_PORT(p) { if ((p < 0) || (p >= PORT_NUM)) return ONLP_STATUS_E_PARAM; }

typedef enum port_type_e {
    TYPE_SFP = 0,
    TYPE_QSFP,
    TYPE_QSFPDD,
    TYPE_MGMT_SFP,
    TYPE_UNNKOW,
    TYPE__MAX,
} port_type_t;

typedef struct
{
    int abs_gpin;
    int lpmode_gpin;
    int reset_gpin;
    int rxlos_gpin;
    int txfault_gpin;
    int txdis_gpin;
    int eeprom_bus;
    int port_type;
} port_attr_t;

static const port_attr_t port_attr[] = {
/*  port  abs   lpmode reset rxlos txfault txdis eeprom type */
    [0] ={487  ,491   ,495  ,-1   ,-1     ,-1   ,13    ,TYPE_QSFPDD},
    [1] ={486  ,490   ,494  ,-1   ,-1     ,-1   ,12    ,TYPE_QSFPDD},
    [2] ={485  ,489   ,493  ,-1   ,-1     ,-1   ,11    ,TYPE_QSFP  },
    [3] ={484  ,488   ,492  ,-1   ,-1     ,-1   ,10    ,TYPE_QSFP  },
    [4] ={375  ,-1    ,-1   ,343  ,439    ,471  ,14    ,TYPE_SFP   },
    [5] ={374  ,-1    ,-1   ,342  ,438    ,470  ,15    ,TYPE_SFP   },
    [6] ={373  ,-1    ,-1   ,341  ,437    ,469  ,16    ,TYPE_SFP   },
    [7] ={372  ,-1    ,-1   ,340  ,436    ,468  ,17    ,TYPE_SFP   },
    [8] ={371  ,-1    ,-1   ,339  ,435    ,467  ,18    ,TYPE_SFP   },
    [9] ={370  ,-1    ,-1   ,338  ,434    ,466  ,19    ,TYPE_SFP   },
    [10]={369  ,-1    ,-1   ,337  ,433    ,465  ,20    ,TYPE_SFP   },
    [11]={368  ,-1    ,-1   ,336  ,432    ,464  ,21    ,TYPE_SFP   },
    [12]={383  ,-1    ,-1   ,351  ,447    ,479  ,22    ,TYPE_SFP   },
    [13]={382  ,-1    ,-1   ,350  ,446    ,478  ,23    ,TYPE_SFP   },
    [14]={381  ,-1    ,-1   ,349  ,445    ,477  ,24    ,TYPE_SFP   },
    [15]={380  ,-1    ,-1   ,348  ,444    ,476  ,25    ,TYPE_SFP   },
    [16]={379  ,-1    ,-1   ,347  ,443    ,475  ,26    ,TYPE_SFP   },
    [17]={378  ,-1    ,-1   ,346  ,442    ,474  ,27    ,TYPE_SFP   },
    [18]={377  ,-1    ,-1   ,345  ,441    ,473  ,28    ,TYPE_SFP   },
    [19]={376  ,-1    ,-1   ,344  ,440    ,472  ,29    ,TYPE_SFP   },
    [20]={359  ,-1    ,-1   ,327  ,423    ,455  ,30    ,TYPE_SFP   },
    [21]={358  ,-1    ,-1   ,326  ,422    ,454  ,31    ,TYPE_SFP   },
    [22]={357  ,-1    ,-1   ,325  ,421    ,453  ,32    ,TYPE_SFP   },
    [23]={356  ,-1    ,-1   ,324  ,420    ,452  ,33    ,TYPE_SFP   },
    [24]={355  ,-1    ,-1   ,323  ,419    ,451  ,34    ,TYPE_SFP   },
    [25]={354  ,-1    ,-1   ,322  ,418    ,450  ,35    ,TYPE_SFP   },
    [26]={353  ,-1    ,-1   ,321  ,417    ,449  ,36    ,TYPE_SFP   },
    [27]={352  ,-1    ,-1   ,320  ,416    ,448  ,37    ,TYPE_SFP   },
};

#define IS_SFP(_port)         (port_attr[_port].port_type == TYPE_SFP)
#define IS_QSFPX(_port)       (port_attr[_port].port_type == TYPE_QSFPDD || port_attr[_port].port_type == TYPE_QSFP)
#define IS_QSFP(_port)        (port_attr[_port].port_type == TYPE_QSFP)
#define IS_QSFPDD(_port)      (port_attr[_port].port_type == TYPE_QSFPDD)

int ufi_port_to_gpio_num(int port, onlp_sfp_control_t control)
{
    int gpio_num = -1;

    switch(control)
    {
        case ONLP_SFP_CONTROL_RESET:
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

    //QSFPDD, QSFP and SFP Ports
    if (IS_QSFPX(local_id)) { //QSFPDD and QSFP
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
 * @brief Read bytes from the target device on the given SFP port.
 * @param id The SFP Port ID.
 * @param devaddr The device address.
 * @param addr Read offset.
 * @param[out] dst Receives the data.
 * @param len Read length.
 * @returns The number of bytes read or ONLP_STATUS_E_* no error.
 */
int onlp_sfpi_dev_read(onlp_oid_id_t id, int devaddr, int addr,
                       uint8_t* dst, int len)
{
    int port = ONLP_OID_ID_GET(id);
    int bus = -1;

    VALIDATE_PORT(port);

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    devaddr = EEPROM_ADDR;
    bus = ufi_port_to_eeprom_bus(port);

    if (onlp_i2c_block_read(bus, devaddr, addr, len, dst, ONLP_I2C_F_FORCE) < 0) {
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
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
int onlp_sfpi_dev_write(onlp_oid_id_t id, int devaddr, int addr,
                        uint8_t* src, int len)
{
    int port = ONLP_OID_ID_GET(id);
    int rc = 0;
    int bus = -1;

    VALIDATE_PORT(port);

    bus = ufi_port_to_eeprom_bus(port);

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if ((rc = onlp_i2c_write(bus, devaddr, addr, len, src, ONLP_I2C_F_FORCE)) < 0) {
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
    int rc = 0;
    int bus = -1;

    VALIDATE_PORT(port);

    bus = ufi_port_to_eeprom_bus(port);

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
 * @brief Write a byte to the target device on the given SFP port.
 * @param id The SFP Port ID.
 * @param devaddr The device address.
 * @param addr The write address.
 * @param value The write value.
 */
int onlp_sfpi_dev_writeb(onlp_oid_id_t id, int devaddr, int addr,
                         uint8_t value)
{
    int port = ONLP_OID_ID_GET(id);
    int rc = 0;
    int bus = -1;

    VALIDATE_PORT(port);

    bus = ufi_port_to_eeprom_bus(port);

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
 * @brief Read a word from the target device on the given SFP port.
 * @param id The SFP Port ID.
 * @param devaddr The device address.
 * @param addr The read address.
 * @returns The word if successful, ONLP_STATUS_E* on error.
 */
int onlp_sfpi_dev_readw(onlp_oid_id_t id, int devaddr, int addr)
{

    int port = ONLP_OID_ID_GET(id);
    int rc = 0;
    int bus = -1;

    VALIDATE_PORT(port);

    bus = ufi_port_to_eeprom_bus(port);

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
 * @brief Write a word to the target device on the given SFP port.
 * @param id The SFP Port ID.
 * @param devaddr The device address.
 * @param addr The write address.
 * @param value The write value.
 */
int onlp_sfpi_dev_writew(onlp_oid_id_t id, int devaddr, int addr,
                         uint16_t value)
{

    int port = ONLP_OID_ID_GET(id);
    int rc = 0;
    int bus = -1;

    VALIDATE_PORT(port);
    bus = ufi_port_to_eeprom_bus(port);

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
int onlp_sfpi_control_supported(onlp_oid_id_t id,
                                onlp_sfp_control_t control, int* rv)
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
int onlp_sfpi_control_set(onlp_oid_id_t id, onlp_sfp_control_t control,
                          int value)
{
    int port = ONLP_OID_ID_GET(id);
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
            return ONLP_STATUS_E_UNSUPPORTED;
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
 * @param id The SFP Port ID.
 * @param control The control
 * @param[out] value Receives the current value.
 */
int onlp_sfpi_control_get(onlp_oid_id_t id, onlp_sfp_control_t control,
                          int* value)
{
    int port = ONLP_OID_ID_GET(id);
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

