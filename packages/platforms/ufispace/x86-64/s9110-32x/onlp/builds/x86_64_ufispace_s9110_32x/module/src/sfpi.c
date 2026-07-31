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
#include <unistd.h>
#include <fcntl.h>
#include <onlp/platformi/sfpi.h>
#include "platform_lib.h"

#define ALL_PORTS             -1

#define QSFP_NUM              32
#define QSFPDD_NUM            0
#define SFP_NUM               1
#define MGMT_NUM              0
#define OSFP_NUM              0
#define XSFPX_NUM             (QSFP_NUM+QSFPDD_NUM+OSFP_NUM)
#define PORT_NUM              (XSFPX_NUM+SFP_NUM+MGMT_NUM)

#define SYSFS_EEPROM         "eeprom"
#define SYSFS_DEV_CLASS      "dev_class"
#define EEPROM_ADDR         (0x50)
#define EEPROM_SFP_DOM_ADDR (0x51)
#define TX_DIS_INPUT_MAX (0xff) /* for input value validation only */
#define SFF8636_EEPROM_OFFSET_TXDIS (0x56)
#define SFF8636_EEPROM_TX_DIS (0x0f) /* txdis valid bit(bit0-bit3), xxxx 1111 */
#define SFF8636_EEPROM_TX_EN (0x0)

//CMIS TX Disable
#define CMIS_PAGE_SIZE                        (128)
#define CMIS_PAGE_SUPPORTED_CTRL_ADV          (1)
#define CMIS_PAGE_TX_DIS                      (16)
#define CMIS_OFFSET_REVISION                  (1)
#define CMIS_OFFSET_MEMORY_MODEL              (2)
#define CMIS_OFFSET_TX_DIS                    (130)
#define CMIS_OFFSET_SUPPORTED_CTRL_ADV        (155)
#define CMIS_MASK_MEMORY_MODEL                (0b10000000)
#define CMIS_MASK_TX_DIS_ADV                  (0b00000010)
#define CMIS_VAL_TX_DIS                       (0xff)
#define CMIS_VAL_TX_EN                        (0x0)
#define CMIS_VAL_MEMORY_MODEL_PAGED           (0)
#define CMIS_VAL_TX_DIS_SUPPORTED             (1)
#define CMIS_VAL_VERSION_MIN                  (0x30)
#define CMIS_VAL_VERSION_MAX                  (0x5F)
#define CMIS_SEEK_TX_DIS_ADV                  (CMIS_PAGE_SIZE * CMIS_PAGE_SUPPORTED_CTRL_ADV + CMIS_OFFSET_SUPPORTED_CTRL_ADV)
#define CMIS_SEEK_TX_DIS                      (CMIS_PAGE_SIZE * CMIS_PAGE_TX_DIS + CMIS_OFFSET_TX_DIS)

typedef enum port_type_e {
    TYPE_SFP = 0,
    TYPE_QSFP,
    TYPE_QSFPDD,
    TYPE_MGMT,
    TYPE_OSFP,
    TYPE_UNNKOW,
    TYPE_MAX,
} port_type_t;

typedef enum op_type_e {
    OP_SYSFS = 0,
    OP_CMIS,
    OP_8636,
    OP_UNNKOW,
    OP_MAX,
} op_type_t;

typedef struct {
    int key;  //[module_type]
    int value;  // [dev_class]
} PortTypeDictEntry;

PortTypeDictEntry port_type_dict[] = {
    {0x03, 2},// 'SFP/SFP+/SFP28'
    {0x0B, 2},// 'DWDM-SFP/SFP+'
    {0x0C, 1},// 'QSFP'
    {0x0D, 1},// 'QSFP+'
    {0x11, 1},// 'QSFP28'
    {0x18, 3},// 'QSFP-DD Double Density 8x (INF-8628)'
    {0x19, 3},// 'OSFP 8x Pluggable Transceiver'
    {0x1E, 3},// 'QSFP+ or later with CMIS spec'
    {0x1F, 3},// 'SFP-DD Double Density 2X Pluggable Transceiver with CMIS spec'
};

#define PORT_TYPE_DICT_SIZE (sizeof(port_type_dict) / sizeof(PortTypeDictEntry))

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
#define IS_XSFPX(_port)         (IS_OSFP(_port) || IS_QSFPX(_port))
#define IS_QSFPX(_port)         (IS_QSFP(_port) || IS_QSFPDD(_port))
#define IS_QSFP(_port)          (port_attr[_port].port_type == TYPE_QSFP)
#define IS_QSFPDD(_port)        (port_attr[_port].port_type == TYPE_QSFPDD)
#define IS_OSFP(_port)          (port_attr[_port].port_type == TYPE_OSFP)

#define VALIDATE_PORT(p) { if (IS_PORT_INVALID(p)) return ONLP_STATUS_E_PARAM; }
#define VALIDATE_SFP_PORT(p) { if (IS_PORT_INVALID(p) || !IS_SFP(p)) return ONLP_STATUS_E_PARAM; }

static int base_num_g = -1;

static int update_port_base(void) {
    int rv  = ONLP_STATUS_OK;
    if(base_num_g == -1) {
        rv = ufi_port_base_get(&base_num_g);
    }
    return rv;
}

static int xfr_label_logical_port(int label_port, int *logical_port) {
    *logical_port = label_port - base_num_g;
    return ONLP_STATUS_OK;
}

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

static int xfr_ctrl_to_sysfs(int logical_port, onlp_sfp_control_t control , char **sysfs, int *attr)
{
    int rv  = ONLP_STATUS_OK;

    if(sysfs == NULL || attr == NULL)
        return ONLP_STATUS_E_PARAM;

    switch(control)
    {
        case ONLP_SFP_CONTROL_RESET:
        case ONLP_SFP_CONTROL_RESET_STATE:
            {
                rv = get_port_sysfs(port_attr[logical_port].reset, sysfs);
                *attr = port_attr[logical_port].reset;
                break;
            }
        case ONLP_SFP_CONTROL_RX_LOS:
            {
                rv = get_port_sysfs(port_attr[logical_port].rxlos, sysfs);
                *attr = port_attr[logical_port].rxlos;
                break;
            }
        case ONLP_SFP_CONTROL_TX_FAULT:
            {
                rv = get_port_sysfs(port_attr[logical_port].txfault, sysfs);
                *attr = port_attr[logical_port].txfault;
                break;
            }
        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                rv = get_port_sysfs(port_attr[logical_port].txdis, sysfs);
                *attr = port_attr[logical_port].txdis;
                break;
            }
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                rv = get_port_sysfs(port_attr[logical_port].lpmode, sysfs);
                *attr = port_attr[logical_port].lpmode;
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

static int xfr_port_to_eeprom_bus(int logical_port)
{
    int bus = -1;

    bus=port_attr[logical_port].eeprom_bus;
    return bus;
}

/**
 * @brief Get device class for a port
 *
 * This function get the device class for a given port.
 *
 * @param port The port number
 * @return An error condition or ONLP_STATUS_OK.
 */
int onlp_sfpi_dev_class_get(int port, int *dev_class)
{
    ONLP_TRY(update_port_base());
    int rv, bus;
    int logical_port = 0;

    ONLP_TRY(xfr_label_logical_port(port, &logical_port));
    VALIDATE_PORT(logical_port);
    bus = xfr_port_to_eeprom_bus(logical_port);

    //read dev_class
    rv = onlp_file_read_int(dev_class, SYS_FMT, bus, EEPROM_ADDR, SYSFS_DEV_CLASS);
    if(rv < 0) {
        AIM_LOG_ERROR("Unable to read "SYS_FMT", error=%d", bus, EEPROM_ADDR, SYSFS_DEV_CLASS, rv);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Set device class for QSFP ports
 *
 * This function set the device class for a given QSFP port.
 *
 * @param port The port number
 * @param dev_class The device class to set
 * @return An error condition or ONLP_STATUS_OK.
 */
int onlp_sfpi_dev_class_set(int port, int dev_class)
{
    ONLP_TRY(update_port_base());
    int bus=0;
    int logical_port = 0;

    ONLP_TRY(xfr_label_logical_port(port, &logical_port));
    VALIDATE_PORT(logical_port);
    bus = xfr_port_to_eeprom_bus(logical_port);

    // set dev_class
    ONLP_TRY(onlp_file_write_int(dev_class, SYS_FMT, bus, EEPROM_ADDR, SYSFS_DEV_CLASS));

    return ONLP_STATUS_OK;
}

/**
 * @brief Update device class for QSFP-Related ports
 *
 * This function updates the device class for a given QSFP-Related port.
 * It reads the current device class and module type, then checks against a dev type list
 * to determine the correct device class.
 * If the device class needs to be updated, it writes the new value to dev_class.
 *
 * @param port The port number
 * @return An error condition or current port dev_class.
 */
int onlp_sfpi_dev_class_update_port(int port)
{
    ONLP_TRY(update_port_base());
    int dev_class, type, i;
    int logical_port = 0;

    ONLP_TRY(xfr_label_logical_port(port, &logical_port));
    VALIDATE_PORT(logical_port);
    if (!IS_QSFPX(logical_port) || !onlp_sfpi_is_present(port)) {
        return ONLP_STATUS_OK;
    }

    //read dev_class
    ONLP_TRY(onlp_sfpi_dev_class_get(port, &dev_class));

    //read module type
    type = onlp_sfpi_dev_readb(port, EEPROM_ADDR, 0);
    if (type < 0) {
        AIM_LOG_ERROR("Port[%d] Addr(0x%02x): invalid module type=%d.\n", port, EEPROM_ADDR, type);
        return ONLP_STATUS_E_INTERNAL;
    }

    for(i = 0; i < PORT_TYPE_DICT_SIZE ; ++i) {
        if (type != port_type_dict[i].key) {
            continue;
        }

        if (port_type_dict[i].value != dev_class) {
            ONLP_TRY(onlp_sfpi_dev_class_set(port, port_type_dict[i].value));
            AIM_LOG_INFO("Port[%d] Type(0x%02x): %d to %d.\n", port, type, dev_class, port_type_dict[i].value);
            break;
        } else { //dev_class is the same.
            break;
        }
    }

    if (i == PORT_TYPE_DICT_SIZE) {
        AIM_LOG_ERROR("Port[%d] Type: %x is Unknown.\n", port, type);
        return ONLP_STATUS_E_INTERNAL;
    }

    return port_type_dict[i].value;
}

/**
 * @brief Update device class for QSFP-Related ports
 *
 * This function updates the device class for a given QSFP-Related port.
 * It reads the current device class and module type, then checks against a dev type list
 * to determine the correct device class.
 * If the device class needs to be updated, it writes the new value to dev_class.
 *
 * @param port The port number. -1 for all ports.
 * @return An error condition or current port dev_class.
 */
int onlp_sfpi_dev_class_update(int port)
{
    ONLP_TRY(update_port_base());
    int rv = ONLP_STATUS_OK;

    // single port update
    if (port != ALL_PORTS) {
        return onlp_sfpi_dev_class_update_port(port);
    }

    // update all QSFPX ports
    int start_port = 0 + base_num_g;
    int end_port = start_port + PORT_NUM;

    for(int i = start_port; i < end_port; ++i) {
        int logical_port = 0;

        if(xfr_label_logical_port(i, &logical_port) != ONLP_STATUS_OK) {
            continue;
        } else if(!IS_QSFPX(logical_port)) {
            continue;
        }

        if (onlp_sfpi_dev_class_update_port(i) < 0) {
            rv = ONLP_STATUS_E_INTERNAL;
        }
    }

    return rv;
}

static int ufi_file_seek_writeb(const char *file, long offset, uint8_t value)
{
    int fd = -1;

    fd = open(file, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) {
        AIM_LOG_ERROR("[%s] Failed to open sysfs file %s", __FUNCTION__, file);
        return ONLP_STATUS_E_INTERNAL;
    }

    // Check for valid offset
    if (offset < 0) {
        AIM_LOG_ERROR("[%s] Invalid offset %d", __FUNCTION__,offset);
        close(fd);
        return ONLP_STATUS_E_INTERNAL;
    }

    // Write value
    if (pwrite(fd, &value, sizeof(uint8_t), offset) != sizeof(uint8_t)) {
        AIM_LOG_ERROR("[%s] Failed to write to sysfs file, offset=%d, value=%d, file=%s", __FUNCTION__, offset, value, file);
        close(fd);
        return ONLP_STATUS_E_INTERNAL;
    }

    close(fd);

    return ONLP_STATUS_OK;
}

static int ufi_file_seek_readb(const char *file, long offset, uint8_t *value)
{
    int fd = -1;

    fd = open(file, O_RDONLY);
    if (fd == -1) {
        AIM_LOG_ERROR("[%s] Failed to open sysfs file %s", __FUNCTION__, file);
        return ONLP_STATUS_E_INTERNAL;
    }

    // Check for valid offset
    if (offset < 0) {
        AIM_LOG_ERROR("[%s] Invalid offset %d", __FUNCTION__,offset);
        close(fd);
        return ONLP_STATUS_E_INTERNAL;
    }

    // Read value
    if (pread(fd, value, sizeof(uint8_t), offset) != sizeof(uint8_t)) {
        AIM_LOG_ERROR("[%s] Failed to read sysfs file, offset=%d, file=%s", __FUNCTION__, offset, file);
        close(fd);
        return ONLP_STATUS_E_INTERNAL;
    }

    close(fd);

    return ONLP_STATUS_OK;
}

/**
 * @brief Read 256th (0-based) byte offset to force page select to 0 to avoid eeprom checksum failure caused by page mis-match
 * @param sysfs_path: The sysfs path to the EEPROM.
 * @returns An error condition.
 */
static int ufi_reset_page_select(char *sysfs_path)
{
    int fd = -1;
    off_t offset_256 = 256;
    uint8_t value = 0;

    fd = open(sysfs_path, O_RDONLY);
    if (fd == -1) {
        return ONLP_STATUS_E_INTERNAL;
    }

    // Read value
    if (pread(fd, &value, sizeof(uint8_t), offset_256) != sizeof(uint8_t)) {
        close(fd);
        return ONLP_STATUS_E_INTERNAL;
    }

    close(fd);
    return ONLP_STATUS_OK;
}

/**
 * @brief Get SFF-8636 Port TX Disable Status by EEPROM
 * @param port: The port number.
 * @param status: 1 if tx disable (turn on)
 * @param status: 0 if normal (turn off)
 * @returns An error condition.
 */
static int ufi_sff8636_txdisable_status_get(int port, int* status, onlp_sfp_control_t control)
{
    uint8_t value = 0;

    if (control != ONLP_SFP_CONTROL_TX_DISABLE &&
        control != ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL) {
        AIM_LOG_ERROR("[%s] invalid control, port=%d, control=%d\n", __FUNCTION__, port, control);
        return ONLP_STATUS_E_PARAM;
    }

    if (onlp_sfpi_is_present(port) != 1) {
        return ONLP_STATUS_OK;
    }

    ONLP_TRY(value = onlp_sfpi_dev_readb(port, EEPROM_ADDR, SFF8636_EEPROM_OFFSET_TXDIS));

    if (control == ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL) {
        *status = value;
    } else { //ONLP_SFP_CONTROL_TX_DISABLE
        // Check each bit of the 'value' has all bits set to 1 meets TX Disable condition (all channels disabled).
        if (value == SFF8636_EEPROM_TX_DIS) {
            *status = 1;
        } else {
            *status = 0;
        }
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Set SFF-8636 Port TX Disable Status by EEPROM
 * @param port: The port number.
 * @param status: 1 if tx disable (turn on)
 * @param status: 0 if normal (turn off)
 * @returns An error condition.
 */
static int ufi_sff8636_txdisable_status_set(int port, int status, onlp_sfp_control_t control)
{
    uint8_t value = 0, readback = 0;

    if (control != ONLP_SFP_CONTROL_TX_DISABLE &&
        control != ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL) {
        AIM_LOG_ERROR("[%s] invalid control, port=%d, control=%d\n", __FUNCTION__, port, control);
        return ONLP_STATUS_E_PARAM;
    }

    if (onlp_sfpi_is_present(port) != 1) {
        return ONLP_STATUS_OK;
    }

    if (control == ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL) {
        // check status range
        if (status < 0 || status > TX_DIS_INPUT_MAX) {
            AIM_LOG_ERROR("[%s] invalid status, port=%d, status=%d\n", __FUNCTION__, port, status);
            return ONLP_STATUS_E_PARAM;
        } else {
            value = (uint8_t)(status & SFF8636_EEPROM_TX_DIS);
        }
    } else { //ONLP_SFP_CONTROL_TX_DISABLE
        if (status == 0) {
            value = SFF8636_EEPROM_TX_EN;
        } else if (status == 1) {
            value = SFF8636_EEPROM_TX_DIS;
        } else {
            AIM_LOG_ERROR("[%s] invalid status, port=%d, status=%d\n", __FUNCTION__, port, status);
            return ONLP_STATUS_E_PARAM;
        }
    }

    ONLP_TRY(onlp_sfpi_dev_writeb(port, EEPROM_ADDR, SFF8636_EEPROM_OFFSET_TXDIS, value));
    ONLP_TRY(readback = onlp_sfpi_dev_readb(port, EEPROM_ADDR, SFF8636_EEPROM_OFFSET_TXDIS));
    if (value != readback) {
        AIM_LOG_ERROR("[%s] compare failed, write value=%d, readback=%d\n", __FUNCTION__, value, readback);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

static int ufi_cmis_txdisable_supported(int port)
{
    ONLP_TRY(update_port_base());
    uint8_t value = 0;
    char sysfs_path[256] = {0};
    int cmis_ver = 0;
    int mem_model = 0;
    int bus = 0;
    int seek = 0;
    int length = 0;
    int tx_dis_adv = 0;
    int logical_port = 0;

    //Check CMIS version on lower page 0x01
    cmis_ver = onlp_sfpi_dev_readb(port, EEPROM_ADDR, CMIS_OFFSET_REVISION);
    if (cmis_ver < CMIS_VAL_VERSION_MIN || cmis_ver > CMIS_VAL_VERSION_MAX) {
        AIM_LOG_INFO("Port[%d] CMIS version %x.%x is not supported (certified range is %x.x-%x.x)\n",
            port, cmis_ver/16, cmis_ver%16, CMIS_VAL_VERSION_MIN/16, CMIS_VAL_VERSION_MAX/16);
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    //Check CMIS memory model on lower page 0x02 bit[7]
    mem_model = shift_bit_mask(onlp_sfpi_dev_readb(port, EEPROM_ADDR, CMIS_OFFSET_MEMORY_MODEL), CMIS_MASK_MEMORY_MODEL);
    if (mem_model != CMIS_VAL_MEMORY_MODEL_PAGED) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    //Check CMIS Tx disable advertisement on page 0x01 offset[155] bit[1]
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));
    VALIDATE_PORT(logical_port);
    bus = xfr_port_to_eeprom_bus(logical_port);
    seek = CMIS_SEEK_TX_DIS_ADV;

    // create and check sysfs_path
    length = snprintf(sysfs_path, sizeof(sysfs_path), SYS_FMT, bus, EEPROM_ADDR, SYSFS_EEPROM);
    if (length < 0 || length >= sizeof(sysfs_path)) {
        AIM_LOG_ERROR("[%s] Error generating sysfs path\n", __FUNCTION__);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (ufi_file_seek_readb(sysfs_path, seek, &value) < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    tx_dis_adv = shift_bit_mask(value, CMIS_MASK_TX_DIS_ADV);

    if (tx_dis_adv != CMIS_VAL_TX_DIS_SUPPORTED) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get CMIS Port TX Disable Status
 * @param port: The port number.
 * @param status: 1 if tx disable (turn on)
 * @param status: 0 if normal (turn off)
 * @returns An error condition.
 */
static int ufi_cmis_txdisable_status_get(int port, int* status, onlp_sfp_control_t control)
{
    ONLP_TRY(update_port_base());
    int ret = 0;
    uint8_t value = 0;
    char sysfs_path[256] = {0};
    int bus = 0;
    int length = 0;
    int logical_port = 0;

    if (control != ONLP_SFP_CONTROL_TX_DISABLE &&
        control != ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL) {
        AIM_LOG_ERROR("[%s] invalid control, port=%d, control=%d\n", __FUNCTION__, port, control);
        return ONLP_STATUS_E_PARAM;
    }

    // Check module present
    if (onlp_sfpi_is_present(port) != 1) {
        return ONLP_STATUS_OK;
    }

    // tx disable support check
    if ((ret=ufi_cmis_txdisable_supported(port)) != ONLP_STATUS_OK) {
        return ret;
    }

    ONLP_TRY(xfr_label_logical_port(port, &logical_port));
    VALIDATE_PORT(logical_port);
    bus = xfr_port_to_eeprom_bus(logical_port);
    length = snprintf(sysfs_path, sizeof(sysfs_path), SYS_FMT, bus, EEPROM_ADDR, SYSFS_EEPROM);
    // check snprintf
    if (length < 0 || length >= sizeof(sysfs_path)) {
        AIM_LOG_ERROR("[%s] Error generating sysfs path\n", __FUNCTION__);
        return ONLP_STATUS_E_INTERNAL;
    }

    // get tx disable
    if (ufi_file_seek_readb(sysfs_path, CMIS_SEEK_TX_DIS, &value) < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (control == ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL) {
        *status = value;
    } else { //ONLP_SFP_CONTROL_TX_DISABLE
        // Check each bit of the 'value' has all bits set to 1 meets TX Disable condition (all channels disabled).
        if (value == CMIS_VAL_TX_DIS) {
            *status = 1;
        } else {
            *status = 0;
        }
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Set CMIS Port TX Disable Status
 * @param port: The port number.
 * @param status: 1 if tx disable (turn on)
 * @param status: 0 if normal (turn off)
 * @returns An error condition.
 */
static int ufi_cmis_txdisable_status_set(int port, int status, onlp_sfp_control_t control)
{
    ONLP_TRY(update_port_base());
    uint8_t value = 0, readback = 0;
    char sysfs_path[256] = {0};
    int bus = 0;
    int seek = CMIS_SEEK_TX_DIS;
    int logical_port = 0;

    if (control != ONLP_SFP_CONTROL_TX_DISABLE &&
        control != ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL) {
        AIM_LOG_ERROR("[%s] invalid control, port=%d, control=%d\n", __FUNCTION__, port, control);
        return ONLP_STATUS_E_PARAM;
    }

    // Check module present
    if (onlp_sfpi_is_present(port) != 1) {
        return ONLP_STATUS_OK;
    }

    // tx disable support check
    if (ufi_cmis_txdisable_supported(port) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if (control == ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL) {
        if (status < 0 || status > TX_DIS_INPUT_MAX) {
            AIM_LOG_ERROR("[%s] unaccepted status, port=%d, status=%d\n", __FUNCTION__, port, status);
            return ONLP_STATUS_E_PARAM;
        } else {
            value = (uint8_t)(status);
        }
    } else {
        // set value
        if (status == 0) {
            value = CMIS_VAL_TX_EN;
        } else if (status == 1) {
            value = CMIS_VAL_TX_DIS;
        } else {
            AIM_LOG_ERROR("[%s] unaccepted status, port=%d, status=%d\n", __FUNCTION__, port, status);
            return ONLP_STATUS_E_PARAM;
        }
    }

    // set sysfs_path
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));
    VALIDATE_PORT(logical_port);
    bus = xfr_port_to_eeprom_bus(logical_port);
    // check snprintf
    int length = snprintf(sysfs_path, sizeof(sysfs_path), SYS_FMT, bus, EEPROM_ADDR, SYSFS_EEPROM);
    if (length < 0 || length >= sizeof(sysfs_path)) {
        AIM_LOG_ERROR("[%s] Error generating sysfs path\n", __FUNCTION__);
        return ONLP_STATUS_E_INTERNAL;
    }

    // write tx disable
    if (ufi_file_seek_writeb(sysfs_path, seek, value) < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    // readback tx disable
    if (ufi_file_seek_readb(sysfs_path, seek, &readback) < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    // check tx disable readback
    if (value != readback) {
        AIM_LOG_ERROR("[%s] port[%d] tx disable readback failed, write value=%d, readback=%d\n", __FUNCTION__, port, value, readback);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Initialize the SFPI subsystem.
 */
int onlp_sfpi_init(void)
{
    init_lock();
    update_port_base();
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the bitmap of SFP-capable port numbers.
 * @param bmap [out] Receives the bitmap.
 */
int onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{
    ONLP_TRY(update_port_base());
    int p;
    int start_port = 0 + base_num_g;
    int end_port = start_port + PORT_NUM;

    for(p = start_port; p < end_port; p++) {
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
    ONLP_TRY(update_port_base());
    int status=ONLP_STATUS_OK;
    int abs = 0, present = 0;
    char *sysfs = NULL;
    uint8_t bit = 0;

    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));

    VALIDATE_PORT(logical_port);
    ONLP_TRY(get_port_sysfs(port_attr[logical_port].abs, &sysfs));

    if ((status = read_file_hex(&abs, sysfs)) < 0) {
        AIM_LOG_ERROR("onlp_sfpi_is_present() failed, error=%d, sysfs=%s",
                          status, sysfs);
        check_and_do_i2c_mux_reset(port);
        return status;
    }

    ONLP_TRY(get_bit(port_attr[logical_port].abs, port_attr[logical_port].cpld_bit, &bit));
    present = (get_bit_value(abs, bit) == 0) ? 1:0;

    return present;
}

/**
 * @brief Return the presence bitmap for all SFP ports.
 * @param dst Receives the presence bitmap.
 */
int onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    ONLP_TRY(update_port_base());
    int p = 0;
    int rc = 0;

    int start_port = 0 + base_num_g;
    int end_port = start_port + PORT_NUM;

    for (p = start_port; p < end_port; p++) {
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
    ONLP_TRY(update_port_base());
    int i=0, value=0;

    int start_port = 0 + base_num_g;
    int end_port = start_port + PORT_NUM;

    for(i = start_port; i < end_port; i++) {
        int logical_port = 0;
        if(xfr_label_logical_port(i, &logical_port) != ONLP_STATUS_OK) {
            continue;
        }

        if(IS_SFP(logical_port)) {
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
    ONLP_TRY(update_port_base());
    int size = 0, bus = 0, rc = 0;
    char sysfs_path[256] = {0};

    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));
    VALIDATE_PORT(logical_port);

    memset(data, 0, 256);
    bus = xfr_port_to_eeprom_bus(logical_port);

    // create and check sysfs_path
    size = snprintf(sysfs_path, sizeof(sysfs_path),
        SYS_FMT, bus, EEPROM_ADDR, SYSFS_EEPROM);

    if (size < 0 || (size_t)size >= sizeof(sysfs_path)) {
        return ONLP_STATUS_E_INTERNAL;
    }

    // reset page select to 0
    ufi_reset_page_select(sysfs_path);

    if((rc = onlp_file_read(data, 256, &size, sysfs_path)) < 0) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d), sysfs_path=%s", port, sysfs_path);
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

    ONLP_TRY(update_port_base());
    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));

    VALIDATE_PORT(logical_port);
    int rc = 0;
    int bus = xfr_port_to_eeprom_bus(logical_port);

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
    ONLP_TRY(update_port_base());
    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));

    VALIDATE_PORT(logical_port);
    int rc = 0;
    int bus = xfr_port_to_eeprom_bus(logical_port);

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
    ONLP_TRY(update_port_base());
    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));

    VALIDATE_PORT(logical_port);
    int rc = 0;
    int bus = xfr_port_to_eeprom_bus(logical_port);

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
    ONLP_TRY(update_port_base());
    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));

    VALIDATE_PORT(logical_port);
    int rc = 0;
    int bus = xfr_port_to_eeprom_bus(logical_port);

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
    ONLP_TRY(update_port_base());
    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));

    VALIDATE_PORT(logical_port);
    int bus = xfr_port_to_eeprom_bus(logical_port);

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
    ONLP_TRY(update_port_base());
    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));

    VALIDATE_PORT(logical_port);
    int rc = 0;
    int bus = xfr_port_to_eeprom_bus(logical_port);

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
    ONLP_TRY(update_port_base());
    char eeprom_path[512];
    FILE* fp;
    int bus = 0;

    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));

    //sfp dom is on 0x51 (2nd 256 bytes)
    //qsfp dom is on lower page 0x00
    //qsfpdd 2.0 dom is on lower page 0x00
    //qsfpdd 3.0 and later dom and above is on lower page 0x00 and higher page 0x17
    VALIDATE_SFP_PORT(logical_port);

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    memset(data, 0, 256);
    memset(eeprom_path, 0, sizeof(eeprom_path));

    //set eeprom_path
    bus = xfr_port_to_eeprom_bus(logical_port);
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
    ONLP_TRY(update_port_base());
    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));
    VALIDATE_PORT(logical_port);

    //set unsupported as default value
    *rv = 0;

    switch (control) {
        case ONLP_SFP_CONTROL_RESET:
        case ONLP_SFP_CONTROL_RESET_STATE:
        case ONLP_SFP_CONTROL_LP_MODE:
            if (IS_XSFPX(logical_port)) {
                *rv = 1;
            }
            break;
        case ONLP_SFP_CONTROL_RX_LOS:
        case ONLP_SFP_CONTROL_TX_FAULT:
            if (IS_SFP(logical_port)) {
                *rv = 1;
            }
            break;
        case ONLP_SFP_CONTROL_TX_DISABLE:
        case ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL:
            *rv = 1;
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
    ONLP_TRY(update_port_base());
    int rc = 0;
    int reg_val = 0;
    char *sysfs = NULL;
    uint8_t bit = 0;
    int attr = 0;
    int op_type = OP_SYSFS;
    int logical_port = 0;

    ONLP_TRY(xfr_label_logical_port(port, &logical_port));

    VALIDATE_PORT(logical_port);

    //check control is valid for this port
    switch(control)
    {
        case ONLP_SFP_CONTROL_RESET:
            {
                if (IS_XSFPX(logical_port)) {
                    //reverse value
                    value = (value == 0) ? 1:0;
                    op_type = OP_SYSFS;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_TX_DISABLE:
        case ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL:
            {
                if (IS_SFP(logical_port)) {
                    op_type = OP_SYSFS;
                } else if (IS_QSFPX(logical_port)) {
                    int dev_class = 0;
                    ONLP_TRY(dev_class = onlp_sfpi_dev_class_update(port));

                    if (dev_class == 1) { //SFF8636 module
                        op_type = OP_8636;
                    } else if (dev_class == 3) { //CMIS module
                        op_type = OP_CMIS;
                    } else if (dev_class < 0) {
                        AIM_LOG_ERROR("Port[%d] dev_class %d is not supported for tx disable control.\n", port, dev_class);
                        return ONLP_STATUS_E_UNSUPPORTED;
                    } else { // module absent or other case
                        return ONLP_STATUS_OK;
                    }
                } else if (IS_OSFP(logical_port)) {
                    op_type = OP_CMIS;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                if (IS_XSFPX(logical_port)) {
                    op_type = OP_SYSFS;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
    }

    if(op_type == OP_SYSFS) {
        //get sysfs
        ONLP_TRY(xfr_ctrl_to_sysfs(logical_port, control, &sysfs, &attr));

        //read reg_val
        if (read_file_hex(&reg_val, sysfs) < 0) {
            check_and_do_i2c_mux_reset(port);
            return ONLP_STATUS_E_INTERNAL;
        }

        //update reg_val
        //0 is normal, 1 is reset, reverse value to fit our platform
        ONLP_TRY(get_bit(attr, port_attr[logical_port].cpld_bit, &bit));
        reg_val = operate_bit(reg_val, bit, value);

        //write reg_val
        if ((rc=onlp_file_write_int(reg_val, sysfs)) < 0) {
            AIM_LOG_ERROR("Unable to write %s, error=%d, reg_val=%x", sysfs,  rc, reg_val);
            check_and_do_i2c_mux_reset(port);
            return ONLP_STATUS_E_INTERNAL;
        }
    } else if (op_type == OP_CMIS) {
        ONLP_TRY(ufi_cmis_txdisable_status_set(port, value, control));
    } else if (op_type == OP_8636) {
        ONLP_TRY(ufi_sff8636_txdisable_status_set(port, value, control));
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get an SFP control.
 * @param port The port.
 * @param control The control
 * @param [out] value Receives the current value.
 */
int onlp_sfpi_control_get(int port, onlp_sfp_control_t control, int* value)
{
    ONLP_TRY(update_port_base());
    int rc;
    int reg_val = 0;
    char *sysfs = NULL;
    uint8_t bit = 0;
    int attr = 0;
    int op_type = OP_SYSFS;
    int logical_port = 0;

    ONLP_TRY(xfr_label_logical_port(port, &logical_port));

    VALIDATE_PORT(logical_port);

    //check control is valid for this port
    switch(control)
    {
        case ONLP_SFP_CONTROL_RESET_STATE:
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                if (IS_XSFPX(logical_port)) {
                    op_type = OP_SYSFS;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_RX_LOS:
        case ONLP_SFP_CONTROL_TX_FAULT:
            {
                if (IS_SFP(logical_port)) {
                    op_type = OP_SYSFS;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_TX_DISABLE:
        case ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL:
            {
                if (IS_SFP(logical_port)) {
                    op_type = OP_SYSFS;
                } else if (IS_QSFPX(logical_port)) {
                    int dev_class = 0;
                    ONLP_TRY(dev_class = onlp_sfpi_dev_class_update(port));

                    if (dev_class == 1) { //SFF8636 module
                        op_type = OP_8636;
                    } else if (dev_class == 3) { //CMIS module
                        op_type = OP_CMIS;
                    } else if (dev_class < 0) {
                        AIM_LOG_ERROR("Port[%d] dev_class %d is not supported for tx disable control.\n", port, dev_class);
                        return ONLP_STATUS_E_UNSUPPORTED;
                    } else { // module absent or other case
                        return ONLP_STATUS_OK;
                    }
                } else if (IS_OSFP(logical_port)) {
                    op_type = OP_CMIS;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
    }

    if(op_type == OP_SYSFS) {
        //get sysfs
        ONLP_TRY(xfr_ctrl_to_sysfs(logical_port, control, &sysfs, &attr));

        //read value
        if ((rc = read_file_hex(&reg_val, sysfs)) < 0) {
            AIM_LOG_ERROR("onlp_sfpi_control_get() failed, error=%d, sysfs=%s", rc, sysfs);
            check_and_do_i2c_mux_reset(port);
            return rc;
        }

        ONLP_TRY(get_bit(attr, port_attr[logical_port].cpld_bit, &bit));
        *value = get_bit_value(reg_val, bit);

        //reverse bit
        if (control == ONLP_SFP_CONTROL_RESET_STATE) {
            *value = !(*value);
        }
    } else if (op_type == OP_CMIS) {
        return ufi_cmis_txdisable_status_get(port, value, control);
    } else if (op_type == OP_8636) {
        return ufi_sff8636_txdisable_status_get(port, value, control);
    }

    return ONLP_STATUS_OK;
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

