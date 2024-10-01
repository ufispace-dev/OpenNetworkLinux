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


#define MGMT_NUM              0
#define SFP_NUM               1
#define QSFP_NUM              32
#define QSFPDD_NUM            0
#define QSFPX_NUM             (QSFP_NUM+QSFPDD_NUM)
#define PORT_NUM              (SFP_NUM+QSFPX_NUM+MGMT_NUM)

#define SYSFS_EEPROM         "eeprom"


#define EEPROM_ADDR (0x50)

typedef enum port_type_e {
    TYPE_SFP = 0,
    TYPE_QSFP,
    TYPE_QSFPDD,
    TYPE_MGMT,
    TYPE_UNNKOW,
    TYPE_MAX,
} port_type_t;

typedef struct
{
    int abs;
    int lpmode;
    int reset;
    int rxlos;
    int txfault;
    int txdis;
    int eeprom_bus;
    int port_type;
    unsigned int cpld_bit;
} port_attr_t;

typedef enum cpld_attr_idx_e {
    CPLD_ABS1 = 0,
    CPLD_ABS2,
    CPLD_ABS3,
    CPLD_ABS4,
    CPLD_ABS5,
    CPLD_RXLOS1,
    CPLD_TXFLT1,
    CPLD_RESET1,
    CPLD_RESET2,
    CPLD_RESET3,
    CPLD_RESET4,
    CPLD_LPMODE1,
    CPLD_LPMODE2,
    CPLD_LPMODE3,
    CPLD_LPMODE4,
    CPLD_TXDIS1,
    CPLD_NONE,
    CPLD_DUMMY_ABS,
    CPLD_DUMMY_RXLOS,
    CPLD_DUMMY_TXFLT,
    CPLD_DUMMY_TXDIS,
} cpld_attr_idx_t;

static const port_attr_t port_attr[] = {
/*
 *  TYPE_MGMT cpld_bit is bit stream
 *  Def: txdis txflt rxlos abs
 *   0b  xxx   xxx   xxx   xxx
 *
 *  Ex:  0x50 = 0b 000 001 010 000
 *       abs  : bit 0
 *       rxlos: bit 2
 *       txflt: bit 1
 *       txdis: bit 0
 */


//  port  abs        lpmode         reset      , rxlos        txfault       txdis        eeprom  type       bit
    [0] ={CPLD_ABS1, CPLD_LPMODE1 , CPLD_RESET1, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 18    , TYPE_QSFP, 0 },
    [1] ={CPLD_ABS1, CPLD_LPMODE1 , CPLD_RESET1, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 19    , TYPE_QSFP, 1 },
    [2] ={CPLD_ABS1, CPLD_LPMODE1 , CPLD_RESET1, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 20    , TYPE_QSFP, 2 },
    [3] ={CPLD_ABS1, CPLD_LPMODE1 , CPLD_RESET1, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 21    , TYPE_QSFP, 3 },
    [4] ={CPLD_ABS1, CPLD_LPMODE1 , CPLD_RESET1, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 22    , TYPE_QSFP, 4 },
    [5] ={CPLD_ABS1, CPLD_LPMODE1 , CPLD_RESET1, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 23    , TYPE_QSFP, 5 },
    [6] ={CPLD_ABS1, CPLD_LPMODE1 , CPLD_RESET1, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 24    , TYPE_QSFP, 6 },
    [7] ={CPLD_ABS1, CPLD_LPMODE1 , CPLD_RESET1, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 25    , TYPE_QSFP, 7 },
    [8] ={CPLD_ABS2, CPLD_LPMODE2 , CPLD_RESET2, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 26    , TYPE_QSFP, 0 },
    [9] ={CPLD_ABS2, CPLD_LPMODE2 , CPLD_RESET2, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 27    , TYPE_QSFP, 1 },
    [10]={CPLD_ABS2, CPLD_LPMODE2 , CPLD_RESET2, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 28    , TYPE_QSFP, 2 },
    [11]={CPLD_ABS2, CPLD_LPMODE2 , CPLD_RESET2, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 29    , TYPE_QSFP, 3 },
    [12]={CPLD_ABS2, CPLD_LPMODE2 , CPLD_RESET2, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 30    , TYPE_QSFP, 4 },
    [13]={CPLD_ABS2, CPLD_LPMODE2 , CPLD_RESET2, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 31    , TYPE_QSFP, 5 },
    [14]={CPLD_ABS2, CPLD_LPMODE2 , CPLD_RESET2, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 32    , TYPE_QSFP, 6 },
    [15]={CPLD_ABS2, CPLD_LPMODE2 , CPLD_RESET2, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 33    , TYPE_QSFP, 7 },
    [16]={CPLD_ABS3, CPLD_LPMODE3 , CPLD_RESET3, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 34    , TYPE_QSFP, 0 },
    [17]={CPLD_ABS3, CPLD_LPMODE3 , CPLD_RESET3, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 35    , TYPE_QSFP, 1 },
    [18]={CPLD_ABS3, CPLD_LPMODE3 , CPLD_RESET3, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 36    , TYPE_QSFP, 2 },
    [19]={CPLD_ABS3, CPLD_LPMODE3 , CPLD_RESET3, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 37    , TYPE_QSFP, 3 },
    [20]={CPLD_ABS3, CPLD_LPMODE3 , CPLD_RESET3, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 38    , TYPE_QSFP, 4 },
    [21]={CPLD_ABS3, CPLD_LPMODE3 , CPLD_RESET3, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 39    , TYPE_QSFP, 5 },
    [22]={CPLD_ABS3, CPLD_LPMODE3 , CPLD_RESET3, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 40    , TYPE_QSFP, 6 },
    [23]={CPLD_ABS3, CPLD_LPMODE3 , CPLD_RESET3, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 41    , TYPE_QSFP, 7 },
    [24]={CPLD_ABS4, CPLD_LPMODE4 , CPLD_RESET4, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 42    , TYPE_QSFP, 0 },
    [25]={CPLD_ABS4, CPLD_LPMODE4 , CPLD_RESET4, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 43    , TYPE_QSFP, 1 },
    [26]={CPLD_ABS4, CPLD_LPMODE4 , CPLD_RESET4, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 44    , TYPE_QSFP, 2 },
    [27]={CPLD_ABS4, CPLD_LPMODE4 , CPLD_RESET4, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 45    , TYPE_QSFP, 3 },
    [28]={CPLD_ABS4, CPLD_LPMODE4 , CPLD_RESET4, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 46    , TYPE_QSFP, 4 },
    [29]={CPLD_ABS4, CPLD_LPMODE4 , CPLD_RESET4, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 47    , TYPE_QSFP, 5 },
    [30]={CPLD_ABS4, CPLD_LPMODE4 , CPLD_RESET4, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 48    , TYPE_QSFP, 6 },
    [31]={CPLD_ABS4, CPLD_LPMODE4 , CPLD_RESET4, CPLD_NONE  , CPLD_NONE   , CPLD_NONE  , 49    , TYPE_QSFP, 7 },
    [32]={CPLD_ABS5, CPLD_NONE    , CPLD_NONE  , CPLD_RXLOS1, CPLD_TXFLT1 , CPLD_TXDIS1, 51    , TYPE_SFP , 1 },
};

#define IS_PORT_INVALID(_port)  (_port < 0) || (_port >= PORT_NUM)
#define IS_SFP(_port)           (port_attr[_port].port_type == TYPE_SFP || port_attr[_port].port_type == TYPE_MGMT)
#define IS_QSFPX(_port)         (port_attr[_port].port_type == TYPE_QSFPDD || port_attr[_port].port_type == TYPE_QSFP)
#define IS_QSFP(_port)          (port_attr[_port].port_type == TYPE_QSFP)
#define IS_QSFPDD(_port)        (port_attr[_port].port_type == TYPE_QSFPDD)

#define VALIDATE_PORT(p) { if (IS_PORT_INVALID(p)) return ONLP_STATUS_E_PARAM; }
#define VALIDATE_SFP_PORT(p) { if (IS_PORT_INVALID(p) || !IS_SFP(p)) return ONLP_STATUS_E_PARAM; }

static int get_port_sysfs(cpld_attr_idx_t idx, char** str)
{
    if(str == NULL)
        return ONLP_STATUS_E_PARAM;

    switch(idx) {
        case CPLD_ABS1:
           *str =  SYSFS_CPLD2 "cpld_qsfp_abs_0_7";
           break;
        case CPLD_ABS2:
            *str = SYSFS_CPLD2 "cpld_qsfp_abs_8_15";
            break;
        case CPLD_ABS3:
            *str = SYSFS_CPLD2 "cpld_qsfp_abs_16_23";
            break;
        case CPLD_ABS4:
            *str = SYSFS_CPLD2 "cpld_qsfp_abs_24_31";
            break;
        case CPLD_ABS5:
            *str = SYSFS_CPLD2 "cpld_sfp_abs_0_1";
            break;
        case CPLD_RXLOS1:
            *str = SYSFS_CPLD2 "cpld_sfp_rxlos_0_1";
            break;
        case CPLD_TXFLT1:
            *str = SYSFS_CPLD2 "cpld_sfp_txflt_0_1";
            break;
        case CPLD_RESET1:
            *str = SYSFS_CPLD2 "cpld_qsfp_reset_0_7";
            break;
        case CPLD_RESET2:
            *str = SYSFS_CPLD2 "cpld_qsfp_reset_8_15";
            break;
        case CPLD_RESET3:
            *str = SYSFS_CPLD2 "cpld_qsfp_reset_16_23";
            break;
        case CPLD_RESET4:
            *str = SYSFS_CPLD2 "cpld_qsfp_reset_24_31";
            break;
        case CPLD_LPMODE1:
            *str = SYSFS_CPLD2 "cpld_qsfp_lpmode_0_7";
            break;
        case CPLD_LPMODE2:
            *str = SYSFS_CPLD2 "cpld_qsfp_lpmode_8_15";
            break;
        case CPLD_LPMODE3:
            *str = SYSFS_CPLD2 "cpld_qsfp_lpmode_16_23";
            break;
        case CPLD_LPMODE4:
            *str = SYSFS_CPLD2 "cpld_qsfp_lpmode_24_31";
            break;
        case CPLD_TXDIS1:
            *str = SYSFS_CPLD2 "cpld_sfp_txdis_0_1";
            break;
        default:
            *str = "";
            return ONLP_STATUS_E_PARAM;
    }
    return ONLP_STATUS_OK;
}

static int get_bit(int attr, unsigned int bit_stream, uint8_t *bit)
{
    int rv  = ONLP_STATUS_OK;
    int tmp_value = 0;

    if(bit == NULL)
        return ONLP_STATUS_E_PARAM;

    switch(attr) {

        case CPLD_DUMMY_ABS:
            tmp_value = bit_stream >> 0;
            break;
        case CPLD_DUMMY_RXLOS:
            tmp_value = bit_stream >> 3;
            break;
        case CPLD_DUMMY_TXFLT:
            tmp_value = bit_stream >> 6;
            break;
        case CPLD_DUMMY_TXDIS:
            tmp_value = bit_stream >> 9;
            break;
        default:
            if(bit_stream > 7) {
                return ONLP_STATUS_E_PARAM;
            } else {
                tmp_value = bit_stream;
                break;
            }
    }
     *bit = (tmp_value & 0x7);
    return rv;
}

static int xfr_ctrl_to_sysfs(int port, onlp_sfp_control_t control , char **sysfs, int *attr)
{
    int rv  = ONLP_STATUS_OK;

    if(sysfs == NULL || attr == NULL)
        return ONLP_STATUS_E_PARAM;

    switch(control)
    {
        case ONLP_SFP_CONTROL_RESET:
        case ONLP_SFP_CONTROL_RESET_STATE:
            {
                rv = get_port_sysfs(port_attr[port].reset, sysfs);
                *attr = port_attr[port].reset;
                break;
            }
        case ONLP_SFP_CONTROL_RX_LOS:
            {
                rv = get_port_sysfs(port_attr[port].rxlos, sysfs);
                *attr = port_attr[port].rxlos;
                break;
            }
        case ONLP_SFP_CONTROL_TX_FAULT:
            {
                rv = get_port_sysfs(port_attr[port].txfault, sysfs);
                *attr = port_attr[port].txfault;
                break;
            }
        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                rv = get_port_sysfs(port_attr[port].txdis, sysfs);
                *attr = port_attr[port].txdis;
                break;
            }
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                rv = get_port_sysfs(port_attr[port].lpmode, sysfs);
                *attr = port_attr[port].lpmode;
                break;
            }
        default:
            rv = ONLP_STATUS_E_UNSUPPORTED;
            *sysfs = "";
            *attr = CPLD_NONE;
    }

    if (rv != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    return ONLP_STATUS_OK;
}

static int xfr_port_to_eeprom_bus(int port)
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
    init_lock();
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
    int status=ONLP_STATUS_OK;
    int abs = 0, present = 0;
    char *sysfs = NULL;
    uint8_t bit = 0;

    VALIDATE_PORT(port);

    ONLP_TRY(get_port_sysfs(port_attr[port].abs, &sysfs));

    if ((status = read_file_hex(&abs, sysfs)) < 0) {
        AIM_LOG_ERROR("onlp_sfpi_is_present() failed, error=%d, sysfs=%s",
                          status, sysfs);
        check_and_do_i2c_mux_reset(port);
        return status;
    }

    ONLP_TRY(get_bit(port_attr[port].abs, port_attr[port].cpld_bit, &bit));
    present = (get_bit_value(abs, bit) == 0) ? 1:0;

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

    for(i = 0; i < PORT_NUM; i++) {
        if(IS_SFP(i)) {
            if(onlp_sfpi_control_get(i, ONLP_SFP_CONTROL_RX_LOS, &value) != ONLP_STATUS_OK) {
                AIM_BITMAP_MOD(dst, i, 0);  //set default value for port which has no cap
            } else {
                AIM_BITMAP_MOD(dst, i, value);
            }
        } else {
            AIM_BITMAP_MOD(dst, i, 0);  //set default value for port which has no cap
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
    bus = xfr_port_to_eeprom_bus(port);

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
    int bus = xfr_port_to_eeprom_bus(port);

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
    int bus = xfr_port_to_eeprom_bus(port);

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
    int bus = xfr_port_to_eeprom_bus(port);

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
    int bus = xfr_port_to_eeprom_bus(port);

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
    VALIDATE_PORT(port);
    int bus = xfr_port_to_eeprom_bus(port);

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

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
    int bus = xfr_port_to_eeprom_bus(port);

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

    //sfp dom is on 0x51 (2nd 256 bytes)
    //qsfp dom is on lower page 0x00
    //qsfpdd 2.0 dom is on lower page 0x00
    //qsfpdd 3.0 and later dom and above is on lower page 0x00 and higher page 0x17
    VALIDATE_SFP_PORT(port);

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    memset(data, 0, 256);
    memset(eeprom_path, 0, sizeof(eeprom_path));

    //set eeprom_path
    bus = xfr_port_to_eeprom_bus(port);
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
    int reg_val = 0;
    char *sysfs = NULL;
    uint8_t bit = 0;
    int attr = 0;

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

    //get sysfs
    ONLP_TRY(xfr_ctrl_to_sysfs(port, control, &sysfs, &attr));

    //read reg_val
    if (read_file_hex(&reg_val, sysfs) < 0) {
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }

    //update reg_val
    //0 is normal, 1 is reset, reverse value to fit our platform
    ONLP_TRY(get_bit(attr, port_attr[port].cpld_bit, &bit));
    reg_val = operate_bit(reg_val, bit, value);

    //write reg_val
    if ((rc=onlp_file_write_int(reg_val, sysfs)) < 0) {
        AIM_LOG_ERROR("Unable to write %s, error=%d, reg_val=%x", sysfs,  rc, reg_val);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }
    rc = ONLP_STATUS_OK;

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
    int reg_val = 0;
    char *sysfs = NULL;
    uint8_t bit = 0;
    int attr = 0;

    VALIDATE_PORT(port);

    //get sysfs
    ONLP_TRY(xfr_ctrl_to_sysfs(port, control, &sysfs, &attr));


    //read gpio value
    if ((rc = read_file_hex(&reg_val, sysfs)) < 0) {
        AIM_LOG_ERROR("onlp_sfpi_control_get() failed, error=%d, sysfs=%s", rc, sysfs);
        check_and_do_i2c_mux_reset(port);
        return rc;
    }

    ONLP_TRY(get_bit(attr, port_attr[port].cpld_bit, &bit));
    *value = get_bit_value(reg_val, bit);

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

