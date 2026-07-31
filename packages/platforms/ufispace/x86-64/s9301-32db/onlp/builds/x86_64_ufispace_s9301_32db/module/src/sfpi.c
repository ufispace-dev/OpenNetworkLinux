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
#include <fcntl.h>
#include <unistd.h>
#include <onlp/platformi/sfpi.h>
#include "platform_lib.h"

#define SYSFS_EEPROM "eeprom"
#define EEPROM_ADDR (0x50)
#define SYSFS_DEV_CLASS "dev_class"
#define TX_DIS_INPUT_MAX (0xff) /* for input value validation only */
#define SFF8636_EEPROM_OFFSET_TXDIS (0x56)
#define SFF8636_EEPROM_TX_DIS (0x0f) /* txdis valid bit(bit0-bit3), xxxx 1111 */
#define SFF8636_EEPROM_TX_EN (0x0)
#define CMIS_PAGE_SIZE (128)
#define CMIS_PAGE_SUPPORTED_CTRL_ADV (1)
#define CMIS_PAGE_TX_DIS (16)
#define CMIS_OFFSET_REVISION (1)
#define CMIS_OFFSET_MEMORY_MODEL (2)
#define CMIS_OFFSET_TX_DIS (130)
#define CMIS_OFFSET_SUPPORTED_CTRL_ADV (155)
#define CMIS_MASK_MEMORY_MODEL (0b10000000)
#define CMIS_MASK_TX_DIS_ADV (0b00000010)
#define CMIS_VAL_TX_DIS (0xff)
#define CMIS_VAL_TX_EN (0x0)
#define CMIS_VAL_MEMORY_MODEL_PAGED (0)
#define CMIS_VAL_TX_DIS_SUPPORTED (1)
#define CMIS_VAL_VERSION_MIN (0x30)
#define CMIS_VAL_VERSION_MAX (0x5F)
#define CMIS_SEEK_TX_DIS_ADV (CMIS_PAGE_SIZE * CMIS_PAGE_SUPPORTED_CTRL_ADV + CMIS_OFFSET_SUPPORTED_CTRL_ADV)
#define CMIS_SEEK_TX_DIS (CMIS_PAGE_SIZE * CMIS_PAGE_TX_DIS + CMIS_OFFSET_TX_DIS)

/* SYSFS */
#define QSFP56_PRES_ATTR_FMT    "cpld_qsfp56_pres_g%d"
#define QSFPDD_PRES_ATTR        "cpld_qsfpdd_pres"
#define QSFP56_RESET_ATTR_FMT   "cpld_qsfp56_reset_ctrl_g%d"
#define QSFPDD_RESET_ATTR       "cpld_qsfpdd_reset_ctrl"
#define QSFP56_LPMODE_ATTR_FMT  "cpld_qsfp56_lp_mode_g%d"
#define QSFPDD_LPMODE_ATTR      "cpld_qsfpdd_lp_mode"
#define SFP_ABS_ATTR            "cpld_sfp_abs"
#define SFP_TXFAULT_ATTR        "cpld_sfp_txfault"
#define SFP_RXLOS_ATTR          "cpld_sfp_rxlos"
#define SFP_TXDIS_ATTR          "cpld_sfp_tx_dis"

#define QSFP56_PORT_NUM         24
#define QSFPDD_PORT_NUM         8
#define QSFPX_PORT_NUM          (QSFP56_PORT_NUM+QSFPDD_PORT_NUM)
#define SFP_PORT_NUM            2
#define ALL_PORT_NUM            (QSFPX_PORT_NUM+SFP_PORT_NUM) //34
#define ALL_PORTS               -1

/* port order: QSFP56(0-23), QSFPDD(24-31), SFP(32-35) */
#define IS_QSFP56(_port)        (_port >= 0 && _port < QSFP56_PORT_NUM)
#define IS_QSFPDD(_port)        (_port >= QSFP56_PORT_NUM && _port < QSFPX_PORT_NUM)
#define IS_QSFPX(_port)         (_port >= 0 && _port < QSFPX_PORT_NUM)
#define IS_QSFP(_port)          IS_QSFP56(_port)
#define IS_SFP(_port)           (_port >= (QSFPX_PORT_NUM) && _port < ALL_PORT_NUM)
#define IS_SFP_P0(_port)        (_port == (QSFPX_PORT_NUM))   //CPU
#define IS_SFP_P1(_port)        (_port == (QSFPX_PORT_NUM+1)) //CPU


#define SFP0_INTERFACE_NAME     "enp182s0f0"
#define SFP1_INTERFACE_NAME     "enp182s0f1"

static int qsfp56_port_eeprom_bus_id_array[QSFP56_PORT_NUM] = { 17, 18, 19, 20, 21, \
                                                                22, 23, 24, 25, 26, \
                                                                27, 28, 29, 30, 31, \
                                                                32, 33, 34, 35, 36, \
                                                                37, 38, 39, 40 };
static int qsfpdd_port_eeprom_bus_id_array[QSFPDD_PORT_NUM] = { 41, 42, 43, 44, 45,
                                                                46, 47, 48 };

typedef struct
{
    int key;   //[module_type]
    int value; // [dev_class]
} PortTypeDictEntry;

PortTypeDictEntry port_type_dict[] =
    {
        {0x03, 2}, // 'SFP/SFP+/SFP28'
        {0x0B, 2}, // 'DWDM-SFP/SFP+'
        {0x0C, 1}, // 'QSFP'
        {0x0D, 1}, // 'QSFP+'
        {0x11, 1}, // 'QSFP28'
        {0x18, 3}, // 'QSFP-DD Double Density 8x (INF-8628)'
        {0x19, 3}, // 'OSFP 8x Pluggable Transceiver'
        {0x1E, 3}, // 'QSFP+ or later with CMIS spec'
        {0x1F, 3}, // 'SFP-DD Double Density 2X Pluggable Transceiver with CMIS spec'
};

#define PORT_TYPE_DICT_SIZE (sizeof(port_type_dict) / sizeof(PortTypeDictEntry))

/**
 * @brief Get QSFP56/QSFPDD/SFP Port Status
 * @param local_id: The port number.
 * @status 1 if present
 * @status 0 if absent
 * @returns An error condition.
 */
static int get_sfpi_port_present_status(int local_id, int *status)
{
    int cpld_port_present_reg = 0;
    int port_id = -1;
    int port_index = -1; //index(0-7) in each port group
    int port_group = -1; //group0: port[0-7], group1: port[8-15], group2: port[16-23]
    int port_mask = 0;

    //QSFP56, QSFPDD, SFP Ports
    if(IS_QSFP56(local_id)) {
        /* QSFP56 */
        port_id = local_id;
        port_index = port_id % 8;
        port_group = port_id / 8;
        port_mask = 0b00000001 << port_index;
        ONLP_TRY(file_read_hex(&cpld_port_present_reg,
            CPLD2_SYSFS_PATH"/"QSFP56_PRES_ATTR_FMT, port_group));
        //val 0 for presence, status set to 1
        *status = !((cpld_port_present_reg & port_mask) >> port_index);
    } else if(IS_QSFPDD(local_id)) {
        /* QSFPDD */
        port_id = local_id;
        port_index = port_id % 8;
        port_mask = 0b00000001 << port_index;
        ONLP_TRY(file_read_hex(&cpld_port_present_reg, CPLD2_SYSFS_PATH"/"QSFPDD_PRES_ATTR));
        //val 0 for presence, status set to 1
        *status = !((cpld_port_present_reg & port_mask) >> port_index);
    } else if(IS_SFP(local_id)) {
        /* SFP Port0 and Port1 */
        port_id = local_id - QSFPX_PORT_NUM;
        port_index = port_id;
        port_mask = 0b00000001 << port_index;
        ONLP_TRY(file_read_hex(&cpld_port_present_reg, CPLD2_SYSFS_PATH"/"SFP_ABS_ATTR));
        //register value 0 for presence, status set to 1
        *status = !((cpld_port_present_reg & port_mask) >> port_index);
    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get QSFP56/QSFPDD Port Reset Status
 * @param local_id: The port number.
 * @status 1 if in reset state
 * @status 0 if normal (not reset)
 * @returns An error condition.
 */
static int get_sfpi_port_reset_status(int local_id, int *status)
{
    int cpld_port_reset_reg = 0;
    int port_id = -1;
    int port_index = -1; //index(0-7) in each port group
    int port_group = -1; //group0: port[0-7], group1: port[8-15], group2: port[16-23]
    int port_mask = 0;

    //QSFP56/QSFPDD
    if(IS_QSFP56(local_id)) {
        /* QSFP56 */
        port_id = local_id;
        port_index = port_id % 8;
        port_group = port_id / 8;
        port_mask = 0b00000001 << port_index;
        ONLP_TRY(file_read_hex(&cpld_port_reset_reg, CPLD2_SYSFS_PATH"/"QSFP56_RESET_ATTR_FMT, port_group));
        //register value 0 for reset, status set to 1
        *status = !((cpld_port_reset_reg & port_mask) >> port_index);
    } else if(IS_QSFPDD(local_id)) {
        /* QSFPDD */
        port_id = local_id;
        port_index = port_id % 8;
        port_mask = 0b00000001 << port_index;
        ONLP_TRY(file_read_hex(&cpld_port_reset_reg, CPLD2_SYSFS_PATH"/"QSFPDD_RESET_ATTR));
        //register value 0 for reset, status set to 1
        *status = !((cpld_port_reset_reg & port_mask) >> port_index);
    } else if(IS_SFP(local_id)) {
        AIM_LOG_ERROR("unsupported ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_UNSUPPORTED;
    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Set QSFP56/QSFPDD Port Reset Status
 * @param local_id: The port number.
 * @param status: 1 if in reset state
 * @param status: 0 if normal (not reset)
 * @returns An error condition.
 */
static int set_sfpi_port_reset_status(int local_id, int status)
{
    int cpld_port_reset_reg = 0;
    int port_id = -1;
    int port_index = -1; //index(0-7) in each port group
    int port_group = -1; //group0: port[0-7], group1: port[8-15], group2: port[16-23]
    int port_mask = 0;
    int value = 0;

    //QSFP56/QSFPDD
    if(IS_QSFP56(local_id)) {
        /* QSFP56 */
        port_id = local_id;
        port_index = port_id % 8;
        port_group = port_id / 8;
        port_mask = 0b00000001 << port_index;
        ONLP_TRY(file_read_hex(&cpld_port_reset_reg, CPLD2_SYSFS_PATH"/"QSFP56_RESET_ATTR_FMT, port_group));
        //register value 0 for reset
        if(status == 0) {
            /* Noraml */
            value = cpld_port_reset_reg | port_mask;
        } else if(status == 1) {
            /* Reset */
            value = cpld_port_reset_reg & ~port_mask;
        } else {
            AIM_LOG_ERROR("unaccepted status, local_id=%d, status=%d, func=%s\n", local_id, status, __FUNCTION__);
            return ONLP_STATUS_E_PARAM;
        }

        ONLP_TRY(onlp_file_write_int(value, CPLD2_SYSFS_PATH"/"QSFP56_RESET_ATTR_FMT, port_group));
    } else if(IS_QSFPDD(local_id)) {
        /* QSFPDD */
        port_id = local_id;
        port_index = port_id % 8;
        port_mask = 0b00000001 << port_index;
        ONLP_TRY(file_read_hex(&cpld_port_reset_reg, CPLD2_SYSFS_PATH"/"QSFPDD_RESET_ATTR));
        //register value 0 for reset
        if(status == 0) {
            /* Noraml */
            value = cpld_port_reset_reg | port_mask;
        } else if(status == 1) {
            /* Reset */
            value = cpld_port_reset_reg & ~port_mask;
        } else {
            AIM_LOG_ERROR("unaccepted status, local_id=%d, status=%d, func=%s\n", local_id, status, __FUNCTION__);
            return ONLP_STATUS_E_PARAM;
        }

        ONLP_TRY(onlp_file_write_int(value, CPLD2_SYSFS_PATH"/"QSFPDD_RESET_ATTR));
    } else if(IS_SFP(local_id)) {
        AIM_LOG_ERROR("unsupported ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_UNSUPPORTED;
    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get QSFP56/QSFPDD Port Low Power Mode Status
 * @param local_id: The port number.
 * @status 1 if in low power mode state
 * @status 0 if normal mode
 * @returns An error condition.
 */
static int get_sfpi_port_lpmode_status(int local_id, int *status)
{
    int cpld_port_lpmode_reg = 0;
    int port_id = -1;
    int port_index = -1; //index(0-7) in each port group
    int port_group = -1; //group0: port[0-7], group1: port[8-15], group2: port[16-23]
    int port_mask = 0;

    //QSFP56/QSFPDD
    if(IS_QSFP56(local_id)) {
        /* QSFP56 */
        port_id = local_id;
        port_index = port_id % 8;
        port_group = port_id / 8;
        port_mask = 0b00000001 << port_index;
        ONLP_TRY(file_read_hex(&cpld_port_lpmode_reg, CPLD2_SYSFS_PATH"/"QSFP56_LPMODE_ATTR_FMT, port_group));
        //register value 1 for low power mode, status set to 1
        *status = (cpld_port_lpmode_reg & port_mask) >> port_index;
    }else if(IS_QSFPDD(local_id)) {
        /* QSFPDD */
        port_id = local_id;
        port_index = port_id % 8;
        port_mask = 0b00000001 << port_index;
        ONLP_TRY(file_read_hex(&cpld_port_lpmode_reg, CPLD2_SYSFS_PATH"/"QSFPDD_LPMODE_ATTR));
        //register value 1 for low power mode, status set to 1
        *status = (cpld_port_lpmode_reg & port_mask) >> port_index;
    } else if(IS_SFP(local_id)) {
        AIM_LOG_ERROR("unsupported ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_UNSUPPORTED;

    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;

    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Set QSFP56/QSFPDD Port Low Power Mode Status
 * @param local_id: The port number.
 * @param status: 1 if in low power mode state
 * @param status: 0 if normal mode
 * @returns An error condition.
 */
static int set_sfpi_port_lpmode_status(int local_id, int status)
{
    int cpld_port_lpmode_reg = 0;
    int port_id = -1;
    int port_index = -1; //index(0-7) in each port group
    int port_group = -1; //group0: port[0-7], group1: port[8-15], group2: port[16-23]
    int port_mask = 0;
    int value = 0;

    //QSFP56/QSFPDD
    if(IS_QSFP56(local_id)) {
        /* QSFP56 */
        port_id = local_id;
        port_index = port_id % 8;
        port_group = port_id / 8;
        port_mask = 0b00000001 << port_index;
        ONLP_TRY(file_read_hex(&cpld_port_lpmode_reg, CPLD2_SYSFS_PATH"/"QSFP56_LPMODE_ATTR_FMT, port_group));
        if(status == 0) {
            /* Normal */
            value = cpld_port_lpmode_reg & ~port_mask;
        } else if(status == 1) {
            /* LP Mode */
            value = cpld_port_lpmode_reg | port_mask;
        } else {
            AIM_LOG_ERROR("unaccepted status, local_id=%d, status=%d, func=%s\n", local_id, status, __FUNCTION__);
            return ONLP_STATUS_E_PARAM;
        }

        ONLP_TRY(onlp_file_write_int(value, CPLD2_SYSFS_PATH"/"QSFP56_LPMODE_ATTR_FMT, port_group));
    } else if(IS_QSFPDD(local_id)) {
        /* QSFPDD */
        port_id = local_id;
        port_index = port_id % 8;
        port_mask = 0b00000001 << port_index;
        ONLP_TRY(file_read_hex(&cpld_port_lpmode_reg,CPLD2_SYSFS_PATH"/"QSFPDD_LPMODE_ATTR));
        if(status == 0) {
            /* Normal */
            value = cpld_port_lpmode_reg & ~port_mask;
        } else if(status == 1) {
            /* LP Mode */
            value = cpld_port_lpmode_reg | port_mask;
        } else {
            AIM_LOG_ERROR("unaccepted status, local_id=%d, status=%d, func=%s\n", local_id, status, __FUNCTION__);
            return ONLP_STATUS_E_PARAM;
        }

        ONLP_TRY(onlp_file_write_int(value, CPLD2_SYSFS_PATH"/"QSFPDD_LPMODE_ATTR));
    } else if(IS_SFP(local_id)) {
        AIM_LOG_ERROR("unsupported ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_UNSUPPORTED;

    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;

    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get SFP Port TX Fault Status
 * @param local_id: The port number.
 * @status 1 if TX Fault (detected)
 * @status 0 if normal (undetected)
 * @returns An error condition.
 */
static int get_sfpi_port_txfault_status(int local_id, int *status)
{
    int cpld_port_txfault_reg = 0;

    //QSFP56/QSFPDD
    if(IS_QSFPX(local_id)) {
        AIM_LOG_ERROR("unsupported ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_UNSUPPORTED;
    } else if(IS_SFP(local_id)) {
        ONLP_TRY(file_read_hex(&cpld_port_txfault_reg, CPLD2_SYSFS_PATH"/"SFP_TXFAULT_ATTR));
        if(IS_SFP_P0(local_id)) {
            *status = (cpld_port_txfault_reg & 0b00000001) >> 0;
        } else if(IS_SFP_P1(local_id)) {
            *status = (cpld_port_txfault_reg & 0b00000010) >> 1;
        }
    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get SFP Port RX LOS Status
 * @param local_id: The port number.
 * @status 1 if RX LOS (detected)
 * @status 0 if normal (undetected)
 * @returns An error condition.
 */
static int get_sfpi_port_rxlos_status(int local_id, int *status)
{
    int cpld_port_rxlos_reg = 0;

    //QSFP56/QSFPDD
    if(IS_QSFPX(local_id)) {
        AIM_LOG_ERROR("unsupported ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_UNSUPPORTED;
    } else if(IS_SFP(local_id)) {
        ONLP_TRY(file_read_hex(&cpld_port_rxlos_reg, CPLD2_SYSFS_PATH"/"SFP_RXLOS_ATTR));
        if(IS_SFP_P0(local_id)) {
            *status = (cpld_port_rxlos_reg & 0b00000001) >> 0;
        } else if(IS_SFP_P1(local_id)) {
            *status = (cpld_port_rxlos_reg & 0b00000010) >> 1;
        }
    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get SFP Port TX Disable Status
 * @param local_id: The port number.
 * @status 1 if TX Disable (turn on)
 * @status 0 if normal (turn off)
 * @returns An error condition.
 */
static int get_sfpi_port_txdisable_status(int local_id, int *status)
{
    int cpld_port_txdisable_reg = 0;

    //QSFP56/QSFPDD
    if(IS_QSFPX(local_id)) {
        AIM_LOG_ERROR("unsupported ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_UNSUPPORTED;
    } else if(IS_SFP(local_id)) {
        ONLP_TRY(file_read_hex(&cpld_port_txdisable_reg, CPLD2_SYSFS_PATH"/"SFP_TXDIS_ATTR));
        if(IS_SFP_P0(local_id)) {
            *status = (cpld_port_txdisable_reg & 0b00000001) >> 0;
        } else if(IS_SFP_P1(local_id)) {
            *status = (cpld_port_txdisable_reg & 0b00000010) >> 1;
        }
    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    return ONLP_STATUS_OK;
}
/**
 * @brief Set SFP Port TX Disable Status
 * @param local_id: The port number.
 * @param status: 1 if tx disable (turn on)
 * @param status: 0 if normal (turn off)
 * @returns An error condition.
 */
static int set_sfpi_port_txdisable_status(int local_id, int status)
{
    int cpld_port_txdisable_reg = 0;
    int port_id = -1;
    int port_index = -1;
    int port_mask = 0;
    int value = 0;

    //QSFP56/QSFPDD
    if(IS_QSFPX(local_id)) {
        /* QSFP56/QSFPDD */
        AIM_LOG_ERROR("unsupported ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_UNSUPPORTED;
    } else if(IS_SFP(local_id)) {
        port_id = local_id - QSFPX_PORT_NUM;
        port_index = port_id;
        port_mask = 0b00000001 << port_index;

        ONLP_TRY(file_read_hex(&cpld_port_txdisable_reg, CPLD2_SYSFS_PATH"/"SFP_TXDIS_ATTR));
        if(status == 0) {
            /* Normal */
            value = cpld_port_txdisable_reg & ~port_mask;
        } else if(status == 1) {
            /* Tx Disable */
            value = cpld_port_txdisable_reg | port_mask;
        } else {
            AIM_LOG_ERROR("unaccepted status, local_id=%d, status=%d, func=%s\n", local_id, status, __FUNCTION__);
            return ONLP_STATUS_E_PARAM;
        }

        ONLP_TRY(onlp_file_write_int(value, CPLD2_SYSFS_PATH"/"SFP_TXDIS_ATTR));
    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    return ONLP_STATUS_OK;
}

/* reg shift */
int shift_bit(uint8_t mask)
{
    int i = 0, mask_one = 1;

    for (i = 0; i < 8; ++i)
    {
        if ((mask >> i) & mask_one) 
        {
            return i;
        }
    }

    return -1;
}

/* reg mask and shift */
uint8_t shift_bit_mask(uint8_t val, uint8_t mask)
{
    int shift = 0;

    shift = shift_bit(mask);

    if (shift < 0) {
        return 0;
    }

    return (val & mask) >> shift;
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
    int rv, bus;

    if (IS_QSFPDD(port)) {
        bus = qsfpdd_port_eeprom_bus_id_array[port - QSFP56_PORT_NUM];
    } else if (IS_QSFP56(port)) {
        bus = qsfp56_port_eeprom_bus_id_array[port];
    } else {
        AIM_LOG_ERROR("[%s] invalid port number %d\n", __FUNCTION__, port);
        return ONLP_STATUS_E_INTERNAL;
    }

    // read dev_class
    rv = onlp_file_read_int(dev_class, SYS_FMT, bus, EEPROM_ADDR, SYSFS_DEV_CLASS);
    if (rv < 0) {
        AIM_LOG_ERROR("Unable to read " SYS_FMT ", error=%d", bus, EEPROM_ADDR, SYSFS_DEV_CLASS, rv);
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
    int bus = 0;

    if (IS_QSFPDD(port)) {
        bus = qsfpdd_port_eeprom_bus_id_array[port - QSFP56_PORT_NUM];
    } else if (IS_QSFP56(port)) {
        bus = qsfp56_port_eeprom_bus_id_array[port];
    } else {
        AIM_LOG_ERROR("[%s] invalid port number %d\n", __FUNCTION__, port);
        return ONLP_STATUS_E_INTERNAL;
    }

    // set dev_class
    ONLP_TRY(onlp_file_write_int(dev_class, SYS_FMT, bus, EEPROM_ADDR, SYSFS_DEV_CLASS));

    return ONLP_STATUS_OK;
}

/**
 * @brief Update device class for QSFPDD ports
 *
 * This function updates the device class for a given QSFPDD port.
 * It reads the current device class and module type, then checks against a dev type list
 * to determine the correct device class.
 * If the device class needs to be updated, it writes the new value to dev_class.
 *
 * @param port The port number
 * @return An error condition or current port dev_class.
 */
int onlp_sfpi_dev_class_update_port(int port)
{
    int dev_class, type, i;

    if (!IS_QSFPX(port)) { // For QSFPX only, skip other ports
        return ONLP_STATUS_E_UNSUPPORTED;
    } else if (!onlp_sfpi_is_present(port)) {
        return ONLP_STATUS_E_MISSING;
    }

    // read dev_class
    ONLP_TRY(onlp_sfpi_dev_class_get(port, &dev_class));

    // read module type
    type = onlp_sfpi_dev_readb(port, EEPROM_ADDR, 0);
    if (type < 0) {
        AIM_LOG_ERROR("Port[%d] Addr(0x%02x): invalid module type=%d.\n", port, EEPROM_ADDR, type);
        return ONLP_STATUS_E_INTERNAL;
    }

    for (i = 0; i < PORT_TYPE_DICT_SIZE; ++i)
    {
        if (type != port_type_dict[i].key)
        {
            continue;
        }
        if (port_type_dict[i].value != dev_class) {
            ONLP_TRY(onlp_sfpi_dev_class_set(port, port_type_dict[i].value));
            AIM_LOG_INFO("Port[%d] Type(0x%02x): %d to %d.\n", port, type, dev_class, port_type_dict[i].value);
            break;
        } else { // dev_class is the same.
            break;
        }
    }

    if (i == PORT_TYPE_DICT_SIZE)
    {
        AIM_LOG_ERROR("Port[%d] Type: %x is Unknown.\n", port, type);
        return ONLP_STATUS_E_INTERNAL;
    }

    return port_type_dict[i].value;
}

/**
 * @brief Update device class for QSFPDD ports
 *
 * This function updates the device class for a given QSFPDD port.
 * It reads the current device class and module type, then checks against a dev type list
 * to determine the correct device class.
 * If the device class needs to be updated, it writes the new value to dev_class.
 *
 * @param port The port number. -1 for all ports.
 * @return An error condition or current port dev_class.
 */
int onlp_sfpi_dev_class_update(int port)
{
    int rv = ONLP_STATUS_OK;

    // single port update
    if (port != ALL_PORTS)
    {
        return onlp_sfpi_dev_class_update_port(port);
    }

    // update all QSFPX ports
    for (int i = 0; i < QSFPX_PORT_NUM; ++i)
    {
        if (onlp_sfpi_dev_class_update_port(i) < 0)
        {
            rv = ONLP_STATUS_E_INTERNAL;
        }
    }

    return rv;
}

static int ufi_file_seek_writeb(const char *file, long offset, uint8_t value)
{
    int fd = -1;

    fd = open(file, O_WRONLY | O_CREAT, 0644);
    if (fd == -1)
    {
        AIM_LOG_ERROR("[%s] Failed to open sysfs file %s", __FUNCTION__, file);
        return ONLP_STATUS_E_INTERNAL;
    }

    // Check for valid offset
    if (offset < 0)
    {
        AIM_LOG_ERROR("[%s] Invalid offset %ld", __FUNCTION__, offset);
        close(fd);
        return ONLP_STATUS_E_INTERNAL;
    }

    // In the CMIS 3.0 memory map, the size of one page is 128 , TX Disable function is located on page 16, at offset 130
    // Write value
    if (pwrite(fd, &value, sizeof(uint8_t), offset) != sizeof(uint8_t))
    {
        AIM_LOG_ERROR("[%s] Failed to write to sysfs file, offset=%ld, value=%d, file=%s", __FUNCTION__, offset, value, file);
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
    if (fd == -1)
    {
        AIM_LOG_ERROR("[%s] Failed to open sysfs file %s", __FUNCTION__, file);
        return ONLP_STATUS_E_INTERNAL;
    }

    // Check for valid offset
    if (offset < 0)
    {
        AIM_LOG_ERROR("[%s] Invalid offset %ld", __FUNCTION__, offset);
        close(fd);
        return ONLP_STATUS_E_INTERNAL;
    }

    // In the CMIS 3.0 memory map, the size of one page is 128 , TX Disable function is located on page 16, at offset 130
    // Read value
    if (pread(fd, value, sizeof(uint8_t), offset) != sizeof(uint8_t))
    {
        AIM_LOG_ERROR("[%s] Failed to read sysfs file, offset=%ld, file=%s", __FUNCTION__, offset, file);
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
 * @param control: control id.
 * @returns An error condition.
 */
static int ufi_sff8636_txdisable_status_get(int port, int *status, onlp_sfp_control_t control)
{
    uint8_t value = 0;

    if (control != ONLP_SFP_CONTROL_TX_DISABLE &&
        control != ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL) {
        AIM_LOG_ERROR("[%s] invalid control, port=%d, control=%d\n", __FUNCTION__, port, control);
        return ONLP_STATUS_E_PARAM;
    }

    if (onlp_sfpi_is_present(port) != 1)
    {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
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
 * @param control: control id.
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

    if (onlp_sfpi_is_present(port) != 1)
    {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
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
    if (value != readback)
    {
        AIM_LOG_ERROR("[%s] compare failed, write value=%d, readback=%d\n", __FUNCTION__, value, readback);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

static int ufi_cmis_txdisable_supported(int port)
{
    uint8_t value = 0;
    char sysfs_path[256] = {0};
    int cmis_ver = 0;
    int mem_model = 0;
    int seek = 0;
    int length = 0;
    int tx_dis_adv = 0;
    int bus_id = 0;

    if (IS_QSFPDD(port)) {
        bus_id = qsfpdd_port_eeprom_bus_id_array[port - QSFP56_PORT_NUM];
    } else if (IS_QSFP56(port)) {
        bus_id = qsfp56_port_eeprom_bus_id_array[port];
    } else {
        AIM_LOG_ERROR("[%s] invalid port number %d\n", __FUNCTION__, port);
        return ONLP_STATUS_E_INTERNAL;
    }

    // Check CMIS version on lower page 0x01
    cmis_ver = onlp_sfpi_dev_readb(port, EEPROM_ADDR, CMIS_OFFSET_REVISION);
    if (cmis_ver < CMIS_VAL_VERSION_MIN || cmis_ver > CMIS_VAL_VERSION_MAX)
    {
        AIM_LOG_INFO("Port[%d] CMIS version %x.%x is not supported (certified range is %x.x-%x.x)\n",
                     port, cmis_ver / 16, cmis_ver % 16, CMIS_VAL_VERSION_MIN / 16, CMIS_VAL_VERSION_MAX / 16);
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    // Check CMIS memory model on lower page 0x02 bit[7]
    mem_model = shift_bit_mask(onlp_sfpi_dev_readb(port, EEPROM_ADDR, CMIS_OFFSET_MEMORY_MODEL), CMIS_MASK_MEMORY_MODEL);
    if (mem_model != CMIS_VAL_MEMORY_MODEL_PAGED)
    {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    // Check CMIS Tx disable advertisement on page 0x01 offset[155] bit[1]
    seek = CMIS_SEEK_TX_DIS_ADV;

    // create and check sysfs_path
    length = snprintf(sysfs_path, sizeof(sysfs_path), SYS_FMT, bus_id, EEPROM_ADDR, SYSFS_EEPROM);
    if (length < 0 || length >= sizeof(sysfs_path))
    {
        AIM_LOG_ERROR("[%s] Error generating sysfs path\n", __FUNCTION__);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (ufi_file_seek_readb(sysfs_path, seek, &value) < 0)
    {
        return ONLP_STATUS_E_INTERNAL;
    }

    tx_dis_adv = shift_bit_mask(value, CMIS_MASK_TX_DIS_ADV);

    if (tx_dis_adv != CMIS_VAL_TX_DIS_SUPPORTED)
    {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get CMIS Port TX Disable Status
 * @param port: The port number.
 * @param status: 1 if tx disable (turn on)
 * @param status: 0 if normal (turn off)
 * @param control: control id.
 * @returns An error condition.
 */
static int ufi_cmis_txdisable_status_get(int port, int *status, onlp_sfp_control_t control)
{
    int ret = 0;
    uint8_t value = 0;
    char sysfs_path[256] = {0};
    int length = 0;
    int bus_id = 0;

    if (IS_QSFPDD(port)) {
        bus_id = qsfpdd_port_eeprom_bus_id_array[port - QSFP56_PORT_NUM];
    } else if (IS_QSFP56(port)) {
        bus_id = qsfp56_port_eeprom_bus_id_array[port];
    } else {
        AIM_LOG_ERROR("[%s] invalid port number %d\n", __FUNCTION__, port);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (control != ONLP_SFP_CONTROL_TX_DISABLE &&
        control != ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL) {
        AIM_LOG_ERROR("[%s] invalid control, port=%d, control=%d\n", __FUNCTION__, port, control);
        return ONLP_STATUS_E_PARAM;
    }

    //Check module present
    if (onlp_sfpi_is_present(port) !=  1)
	{
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    // tx disable support check
    if ((ret = ufi_cmis_txdisable_supported(port)) != ONLP_STATUS_OK)
    {
        return ret;
    }

    length = snprintf(sysfs_path, sizeof(sysfs_path), SYS_FMT, bus_id, EEPROM_ADDR, SYSFS_EEPROM);
    // check snprintf
    if (length < 0 || length >= sizeof(sysfs_path))
    {
        AIM_LOG_ERROR("[%s] Error generating sysfs path\n", __FUNCTION__);
        return ONLP_STATUS_E_INTERNAL;
    }

    // get tx disable
    if (ufi_file_seek_readb(sysfs_path, CMIS_SEEK_TX_DIS, &value) < 0)
    {
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
 * @param control: control id.
 * @returns An error condition.
 */
static int ufi_cmis_txdisable_status_set(int port, int status, onlp_sfp_control_t control)
{
    uint8_t value = 0, readback = 0;
    char sysfs_path[256] = {0};
    int seek = CMIS_SEEK_TX_DIS;
    int bus_id = 0;

    if (IS_QSFPDD(port)) {
        bus_id = qsfpdd_port_eeprom_bus_id_array[port - QSFP56_PORT_NUM];
    } else if (IS_QSFP56(port)) {
        bus_id = qsfp56_port_eeprom_bus_id_array[port];
    } else {
        AIM_LOG_ERROR("[%s] invalid port number %d\n", __FUNCTION__, port);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (control != ONLP_SFP_CONTROL_TX_DISABLE &&
        control != ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL) {
        AIM_LOG_ERROR("[%s] invalid control, port=%d, control=%d\n", __FUNCTION__, port, control);
        return ONLP_STATUS_E_PARAM;
    }

    //Check module present
    if (onlp_sfpi_is_present(port) !=  1)
	{
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    // tx disable support check
    if (ufi_cmis_txdisable_supported(port) != ONLP_STATUS_OK)
    {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    // set value
    if (control == ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL) {
        if (status < 0 || status > TX_DIS_INPUT_MAX) {
            AIM_LOG_ERROR("[%s] unaccepted status, port=%d, status=%d\n", __FUNCTION__, port, status);
            return ONLP_STATUS_E_PARAM;
        } else {
            value = (uint8_t)(status);
        }
    } else {
        if (status == 0) {
            value = CMIS_VAL_TX_EN;
        } else if (status == 1) {
            value = CMIS_VAL_TX_DIS;
        } else {
            AIM_LOG_ERROR("[%s] unaccepted status, port=%d, status=%d\n", __FUNCTION__, port, status);
            return ONLP_STATUS_E_PARAM;
        }
    }

    // check snprintf
    int length = snprintf(sysfs_path, sizeof(sysfs_path), SYS_FMT, bus_id, EEPROM_ADDR, SYSFS_EEPROM);
    if (length < 0 || length >= sizeof(sysfs_path))
    {
        AIM_LOG_ERROR("[%s] Error generating sysfs path\n", __FUNCTION__);
        return ONLP_STATUS_E_INTERNAL;
    }

    // write tx disable
    if (ufi_file_seek_writeb(sysfs_path, seek, value) < 0)
    {
        return ONLP_STATUS_E_INTERNAL;
    }

    // readback tx disable
    if (ufi_file_seek_readb(sysfs_path, seek, &readback) < 0)
    {
        return ONLP_STATUS_E_INTERNAL;
    }

    // check tx disable readback
    if (value != readback)
    {
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

    AIM_BITMAP_CLR_ALL(bmap);
    for(p = 0; p < ALL_PORT_NUM; p++) {
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
    int local_id = port;
    int status = 0;

    if(local_id < ALL_PORT_NUM) {
        ONLP_TRY(get_sfpi_port_present_status(local_id, &status));
    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
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
    int status = 0;

    AIM_BITMAP_CLR_ALL(dst);
    for (p = 0; p < ALL_PORT_NUM; p++) {
        status = onlp_sfpi_is_present(p);
        AIM_BITMAP_MOD(dst, p, status);
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

    AIM_BITMAP_CLR_ALL(dst);
    for (i = 0; i < ALL_PORT_NUM; i++) {
        ONLP_TRY(onlp_sfpi_control_get(i, ONLP_SFP_CONTROL_RX_LOS, &value));
        AIM_BITMAP_MOD(dst, i, value);
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
    int local_id = port;
    int ret = ONLP_STATUS_OK;
    int bus_id = -1;
    int port_id = -1;
    char eeprom_path[128];
    char command[256] = "";
    int size = 0;

    memset(data, 0, 256);
    memset(eeprom_path, 0, sizeof(eeprom_path));

    if(onlp_sfpi_is_present(local_id) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", local_id);
        return ONLP_STATUS_OK;
    }

    if(IS_QSFP56(local_id)) {
        /* QSFP56 */
        port_id = local_id;
        bus_id = qsfp56_port_eeprom_bus_id_array[port_id];
        snprintf(eeprom_path, sizeof(eeprom_path), "/sys/bus/i2c/devices/%d-0050/eeprom", bus_id);
        // reset page select to 0
        ufi_reset_page_select(eeprom_path);
        ret = onlp_file_read(data, 256, &size, eeprom_path);
    } else if(IS_QSFPDD(local_id)) {
        /* QSFPDD */
        port_id = local_id - QSFP56_PORT_NUM;
        bus_id = qsfpdd_port_eeprom_bus_id_array[port_id];
        snprintf(eeprom_path, sizeof(eeprom_path), "/sys/bus/i2c/devices/%d-0050/eeprom", bus_id);
        // reset page select to 0
        ufi_reset_page_select(eeprom_path);
        ret = onlp_file_read(data, 256, &size, eeprom_path);
    } else if(IS_SFP(local_id)) {
        /* SFP */
        port_id = local_id - QSFPX_PORT_NUM;

        if(IS_SFP_P0(local_id)) {
            /* SFP Port0 */
            snprintf(command, sizeof(command), "ethtool -m %s raw on length 256 > /tmp/.sfp.%s.eeprom", SFP0_INTERFACE_NAME, SFP0_INTERFACE_NAME);
            snprintf(eeprom_path, sizeof(eeprom_path), "/tmp/.sfp.%s.eeprom", SFP0_INTERFACE_NAME);
        } else if(IS_SFP_P1(local_id)) {
            /* SFP Port1 */
            snprintf(command, sizeof(command), "ethtool -m %s raw on length 256 > /tmp/.sfp.%s.eeprom", SFP1_INTERFACE_NAME, SFP1_INTERFACE_NAME);
            snprintf(eeprom_path, sizeof(eeprom_path), "/tmp/.sfp.%s.eeprom", SFP1_INTERFACE_NAME);
        } else {
            AIM_LOG_ERROR("unknown SFP ports, port=%d\n", port_id);
            return ONLP_STATUS_E_UNSUPPORTED;
        }

        if((ret = system(command)) != 0) {
            AIM_LOG_ERROR("Unable to read sfp eeprom (port_id=%d), func=%s\n", port_id, __FUNCTION__);
            return ONLP_STATUS_E_INTERNAL;
        }

        ret = onlp_file_read(data, 256, &size, eeprom_path);
    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    if(ret < 0) {
        check_and_do_i2c_mux_reset(local_id);
    }

    if(size != 256) {
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
    int local_id = port;
    int ret = ONLP_STATUS_OK;
    int bus_id = -1;
    int port_id = -1;

    if(onlp_sfpi_is_present(local_id) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", local_id);
        return ONLP_STATUS_OK;
    }

    if(IS_QSFP56(local_id)) {
        /* QSFP56 */
        port_id = local_id;
        bus_id = qsfp56_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_readb(bus_id, devaddr, addr, ONLP_I2C_F_FORCE);
    } else if(IS_QSFPDD(local_id)) {
        /* QSFPDD */
        port_id = local_id - QSFP56_PORT_NUM;
        bus_id = qsfpdd_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_readb(bus_id, devaddr, addr, ONLP_I2C_F_FORCE);
    } else if(IS_SFP_P0(local_id) || IS_SFP_P1(local_id)) {
        /* SFP Port0 or SFP Port1 */
        return ONLP_STATUS_E_UNSUPPORTED;
    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;

    }

    if(ret < 0) {
        check_and_do_i2c_mux_reset(local_id);
    }

    return ret;
}

/**
 * @brief Write a byte to an address on the given SFP port's bus.
 */
int onlp_sfpi_dev_writeb(int port, uint8_t devaddr, uint8_t addr, uint8_t value)
{
    int local_id = port;
    int ret = ONLP_STATUS_OK;
    int bus_id = -1;
    int port_id = -1;

    if(onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", local_id);
        return ONLP_STATUS_OK;
    }

    if(IS_QSFP56(local_id)) {
        /* QSFP56 */
        port_id = local_id;
        bus_id = qsfp56_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_writeb(bus_id, devaddr, addr, value, ONLP_I2C_F_FORCE);
    } else if(IS_QSFPDD(local_id)) {
        /* QSFPDD */
        port_id = local_id - QSFP56_PORT_NUM;
        bus_id = qsfpdd_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_writeb(bus_id, devaddr, addr, value, ONLP_I2C_F_FORCE);
    } else if(IS_SFP_P0(local_id) || IS_SFP_P1(local_id)) {
        /* SFP Port0 or SFP Port1 */
        return ONLP_STATUS_E_UNSUPPORTED;
    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    if(ret < 0) {
        check_and_do_i2c_mux_reset(local_id);
    }

    return ret;
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
    int local_id = port;
    int ret = ONLP_STATUS_OK;
    int bus_id = -1;
    int port_id = -1;

    if(onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", local_id);
        return ONLP_STATUS_OK;
    }

    if(IS_QSFP56(local_id)) {
        /* QSFP56 */
        port_id = local_id;
        bus_id = qsfp56_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_readw(bus_id, devaddr, addr, ONLP_I2C_F_FORCE);
    } else if(IS_QSFPDD(local_id)) {
        /* QSFPDD */
        port_id = local_id - QSFP56_PORT_NUM;
        bus_id = qsfpdd_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_readw(bus_id, devaddr, addr, ONLP_I2C_F_FORCE);
    } else if(IS_SFP_P0(local_id) || IS_SFP_P1(local_id)) {
        /* SFP Port0 or SFP Port1 */
        return ONLP_STATUS_E_UNSUPPORTED;
    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    if(ret < 0) {
        check_and_do_i2c_mux_reset(local_id);
    }

    return ret;
}

/**
 * @brief Write a word to an address on the given SFP port's bus.
 */
int onlp_sfpi_dev_writew(int port, uint8_t devaddr, uint8_t addr, uint16_t value)
{
    int local_id = port;
    int ret = ONLP_STATUS_OK;
    int bus_id = -1;
    int port_id = -1;

    if(onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", local_id);
        return ONLP_STATUS_OK;
    }

    if(IS_QSFP56(local_id)) {
        /* QSFP56 */
        port_id = local_id;
        bus_id = qsfp56_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_writew(bus_id, devaddr, addr, value, ONLP_I2C_F_FORCE);
    } else if(IS_QSFPDD(local_id)) {
        /* QSFPDD */
        port_id = local_id - QSFP56_PORT_NUM;
        bus_id = qsfpdd_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_writew(bus_id, devaddr, addr, value, ONLP_I2C_F_FORCE);
    } else if(IS_SFP_P0(local_id) || IS_SFP_P1(local_id)) {
        /* SFP Port0 or SFP Port1 */
        return ONLP_STATUS_E_UNSUPPORTED;
    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    if(ret < 0) {
        check_and_do_i2c_mux_reset(local_id);
    }

    return ret;
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
    int local_id = port;
    int ret = ONLP_STATUS_OK;
    int bus_id = -1;
    int port_id = -1;

    if(onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", local_id);
        return ONLP_STATUS_OK;
    }

    if(IS_QSFP56(local_id)) {
        /* QSFP56 */
        port_id = local_id;
        bus_id = qsfp56_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_block_read(bus_id, devaddr, addr, size, rdata, ONLP_I2C_F_FORCE);
    } else if(IS_QSFPDD(local_id)) {
        /* QSFPDD */
        port_id = local_id - QSFP56_PORT_NUM;
        bus_id = qsfpdd_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_block_read(bus_id, devaddr, addr, size, rdata, ONLP_I2C_F_FORCE);
    } else if(IS_SFP_P0(local_id) || IS_SFP_P1(local_id)) {
        /* SFP Port0 or SFP Port1 */
        return ONLP_STATUS_E_UNSUPPORTED;
    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;

    }

    if(ret < 0) {
        check_and_do_i2c_mux_reset(local_id);
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Write to an address on the given SFP port's bus.
 */
int onlp_sfpi_dev_write(int port, uint8_t devaddr, uint8_t addr, uint8_t* data, int size)
{
    return ONLP_STATUS_E_UNSUPPORTED;
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
    int local_id = port;

    *rv = 0;
    switch(control) {
        case ONLP_SFP_CONTROL_RESET:
            if(IS_QSFPX(local_id)) {
                *rv = 1;
            }
            break;

        case ONLP_SFP_CONTROL_RESET_STATE:
            if(IS_QSFPX(local_id)) {
                *rv = 1;
            }
            break;

        case ONLP_SFP_CONTROL_RX_LOS:
            if(IS_SFP(local_id)) {
                *rv = 1;
            }
            break;

        case ONLP_SFP_CONTROL_TX_FAULT:
            if(IS_SFP(local_id)) {
                *rv = 1;
            }
            break;

        case ONLP_SFP_CONTROL_TX_DISABLE:
        case ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL:
            *rv = 1;
            break;

        case ONLP_SFP_CONTROL_LP_MODE:
            if(IS_QSFPX(local_id)) {
                *rv = 1;
            }
            break;

        default:
            return ONLP_STATUS_E_UNSUPPORTED;
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
    int local_id = port;

    switch(control) {
        case ONLP_SFP_CONTROL_RESET:
            if(IS_QSFPX(local_id)) {
                ONLP_TRY(set_sfpi_port_reset_status(local_id, value));
            } else {
                //do nothing
                return ONLP_STATUS_OK;
            }
            break;

        case ONLP_SFP_CONTROL_TX_DISABLE:
        case ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL:
            if(IS_SFP(local_id)) {
                ONLP_TRY(set_sfpi_port_txdisable_status(local_id, value));
            } else if (IS_QSFPX(port)) {
                int dev_class = 0;
                ONLP_TRY(dev_class = onlp_sfpi_dev_class_update(local_id));
                if (dev_class == 1)
                { // SFF8636 module
                    ONLP_TRY(ufi_sff8636_txdisable_status_set(local_id, value, control));
                }
                else if (dev_class == 3)
                { // CMIS module
                    ONLP_TRY(ufi_cmis_txdisable_status_set(local_id, value, control));
                }
                else
                {
                    AIM_LOG_ERROR("Port[%d] dev_class %d is not supported for tx disable control.\n", local_id, dev_class);
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
            } else {
                //do nothing
                return ONLP_STATUS_OK;
            }
            break;

        case ONLP_SFP_CONTROL_LP_MODE:
            if(IS_QSFPX(local_id)) {
                ONLP_TRY(set_sfpi_port_lpmode_status(local_id, value));
            } else {
                //do nothing
                return ONLP_STATUS_OK;
            }
            break;

        default:
            //do nothing
            return ONLP_STATUS_OK;
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
    int local_id = port;
    int status = 0;

    *value = 0;

    switch(control) {
        case ONLP_SFP_CONTROL_RESET_STATE:
            if(IS_QSFPX(local_id)) {
                ONLP_TRY(get_sfpi_port_reset_status(local_id, &status));
                *value = status;
            } else {
                //do nothing
                return ONLP_STATUS_OK;
            }
            break;

        case ONLP_SFP_CONTROL_RX_LOS:
            if(IS_SFP(local_id)) {
                ONLP_TRY(get_sfpi_port_rxlos_status(local_id, &status));
                *value = status;
            } else {
                //do nothing
                return ONLP_STATUS_OK;
            }
            break;

        case ONLP_SFP_CONTROL_TX_FAULT:
            if(IS_SFP(local_id)) {
                ONLP_TRY(get_sfpi_port_txfault_status(local_id, &status));
                *value = status;
            } else {
                //do nothing
                return ONLP_STATUS_OK;
            }
            break;

        case ONLP_SFP_CONTROL_TX_DISABLE:
        case ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL:
            if(IS_SFP(local_id)) {
                return get_sfpi_port_txdisable_status(local_id, value);
            } else if (IS_QSFPX(local_id)) {
                int dev_class = 0;
                ONLP_TRY(dev_class = onlp_sfpi_dev_class_update(local_id));

                if (dev_class == 1)
                { // SFF8636 module
                    ONLP_TRY(ufi_sff8636_txdisable_status_get(local_id, value, control));
                }
                else if (dev_class == 3)
                { // CMIS module
                    int rc;
                    rc = ufi_cmis_txdisable_status_get(local_id, value, control);
                    // tx dis 0 for unsupport module 
                    if (rc != ONLP_STATUS_OK)
                    {
                        *value = 0;
                        return rc;
                    } 
                }
                else
                {
                    AIM_LOG_ERROR("Port[%d] dev_class %d is not supported for tx disable control.\n", local_id, dev_class);
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
            } else {
                //do nothing
                return ONLP_STATUS_OK;
            }
            break;

        case ONLP_SFP_CONTROL_LP_MODE:
            if(IS_QSFPX(local_id)) {
                ONLP_TRY(get_sfpi_port_lpmode_status(local_id, &status));
                *value = status;
            } else {
                //do nothing
                return ONLP_STATUS_OK;
            }
            break;

        default:
            //do nothing
            return ONLP_STATUS_OK;
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
