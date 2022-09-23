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

/* SYSFS */
#define QSFPDD_PRES_ATTR_FMT    "cpld_qsfpdd_pres_g%d"
#define QSFPDD_RESET_ATTR_FMT   "cpld_qsfpdd_reset_ctrl_g%d"
#define QSFPDD_LPMODE_ATTR_FMT  "cpld_qsfpdd_lp_mode_g%d"
#define SFP_ABS_ATTR            "cpld_sfp_abs"
#define SFP_TXFAULT_ATTR        "cpld_sfp_txfault"
#define SFP_RXLOS_ATTR          "cpld_sfp_rxlos"
#define SFP_TXDIS_ATTR          "cpld_sfp_tx_dis"

#define QSFPDD_PORT_NUM         32
#define SFP_PORT_NUM            4
#define ALL_PORT_NUM            (QSFPDD_PORT_NUM+SFP_PORT_NUM) //36

/* port order: QSFPDD(0-31), SFP(32-35) */
#define IS_QSFPDD(_port)        (_port >= 0 && _port < QSFPDD_PORT_NUM)
#define IS_SFP(_port)           (_port >= (QSFPDD_PORT_NUM) && _port < ALL_PORT_NUM)
#define IS_SFP_P0(_port)        (_port == (QSFPDD_PORT_NUM))   //CPU
#define IS_SFP_P1(_port)        (_port == (QSFPDD_PORT_NUM+1)) //CPU
#define IS_SFP_P2(_port)        (_port == (QSFPDD_PORT_NUM+2)) //MAC
#define IS_SFP_P3(_port)        (_port == (QSFPDD_PORT_NUM+3)) //MAC

#define SFP0_INTERFACE_NAME     "enp182s0f0"
#define SFP1_INTERFACE_NAME     "enp182s0f1"

static int qsfpdd_port_eeprom_bus_id_array[QSFPDD_PORT_NUM] = { 17, 18, 19, 20, 21, \
                                                                22, 23, 24, 25, 26, \
                                                                27, 28, 29, 30, 31, \
                                                                32, 33, 34, 35, 36, \
                                                                37, 38, 39, 40, 41, \
                                                                42, 43, 44, 45, 46, \
                                                                47, 48 };

/**
 * @brief Get QSFPDD/SFP Port Status
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
    int port_group = -1; //group0: port[0-7], group1: port[8-15], group2: port[16-23], group3: port[24-31]
    int port_mask = 0;

    //QSFPDD, SFP Ports
    if(IS_QSFPDD(local_id)) {
        /* QSFPDD */
        port_id = local_id;
        port_index = port_id % 8;
        port_group = port_id / 8;
        port_mask = 0b00000001 << port_index;
        ONLP_TRY(file_read_hex(&cpld_port_present_reg,
            CPLD2_SYSFS_PATH"/"QSFPDD_PRES_ATTR_FMT, port_group));
        //val 0 for presence, status set to 1
        *status = !((cpld_port_present_reg & port_mask) >> port_index);
    } else if(IS_SFP(local_id)) {
        /* SFP Ports */
        port_id = local_id - QSFPDD_PORT_NUM;
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
 * @brief Get QSFPDD Port Reset Status
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

    //QSFPDD
    if(IS_QSFPDD(local_id)) {
        /* QSFPDD */
        port_id = local_id;
        port_index = port_id % 8;
        port_group = port_id / 8;
        port_mask = 0b00000001 << port_index;
        ONLP_TRY(file_read_hex(&cpld_port_reset_reg, CPLD2_SYSFS_PATH"/"QSFPDD_RESET_ATTR_FMT, port_group));
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
 * @brief Set QSFPDD Port Reset Status
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

    //QSFPDD
    if(IS_QSFPDD(local_id)) {
        /* QSFPDD */
        port_id = local_id;
        port_index = port_id % 8;
        port_group = port_id / 8;
        port_mask = 0b00000001 << port_index;
        ONLP_TRY(file_read_hex(&cpld_port_reset_reg, CPLD2_SYSFS_PATH"/"QSFPDD_RESET_ATTR_FMT, port_group));
        //register value 0 for reset
        if(status == 0) {
            /* Noraml */
            value = cpld_port_reset_reg | port_mask;
        } else if(status == 1) {
            /* Reset */
            value = cpld_port_reset_reg & ~port_mask;
        } else {
            return ONLP_STATUS_E_PARAM;
            AIM_LOG_ERROR("unaccepted status, local_id=%d, status=%d, func=%s\n", local_id, status, __FUNCTION__);
        }

        ONLP_TRY(onlp_file_write_int(value, CPLD2_SYSFS_PATH"/"QSFPDD_RESET_ATTR_FMT, port_group));
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
 * @brief Get QSFPDD Port Low Power Mode Status
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
    int port_group = -1; //group0: port[0-7], group1: port[8-15], group2: port[16-23], group3: port[24-31]
    int port_mask = 0;

    //QSFPDD
    if(IS_QSFPDD(local_id)) {
        /* QSFPDD */
        port_id = local_id;
        port_index = port_id % 8;
        port_group = port_id / 8;
        port_mask = 0b00000001 << port_index;
        ONLP_TRY(file_read_hex(&cpld_port_lpmode_reg, CPLD2_SYSFS_PATH"/"QSFPDD_LPMODE_ATTR_FMT, port_group));
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
 * @brief Set QSFPDD Port Low Power Mode Status
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
    int port_group = -1; //group0: port[0-7], group1: port[8-15], group2: port[16-23], group3: port[24-31]
    int port_mask = 0;
    int value = 0;

    //QSFPDD
    if(IS_QSFPDD(local_id)) {
        /* QSFPDD */
        port_id = local_id;
        port_index = port_id % 8;
        port_group = port_id / 8;
        port_mask = 0b00000001 << port_index;
        ONLP_TRY(file_read_hex(&cpld_port_lpmode_reg, CPLD2_SYSFS_PATH"/"QSFPDD_LPMODE_ATTR_FMT, port_group));
        if(status == 0) {
            /* Normal */
            value = cpld_port_lpmode_reg & ~port_mask;
        } else if(status == 1) {
            /* LP Mode */
            value = cpld_port_lpmode_reg | port_mask;
        } else {
            return ONLP_STATUS_E_PARAM;
            AIM_LOG_ERROR("unaccepted status, local_id=%d, status=%d, func=%s\n", local_id, status, __FUNCTION__);
        }

        ONLP_TRY(onlp_file_write_int(value, CPLD2_SYSFS_PATH"/"QSFPDD_LPMODE_ATTR_FMT, port_group));
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

    //QSFPDD
    if(IS_QSFPDD(local_id)) {
        AIM_LOG_ERROR("unsupported ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_UNSUPPORTED;
    } else if(IS_SFP(local_id)) {
        ONLP_TRY(file_read_hex(&cpld_port_txfault_reg, CPLD2_SYSFS_PATH"/"SFP_TXFAULT_ATTR));
        if(IS_SFP_P0(local_id)) {
            *status = (cpld_port_txfault_reg & 0b00000001) >> 0;
        } else if IS_SFP_P1(local_id) {
            *status = (cpld_port_txfault_reg & 0b00000010) >> 1;
        } else if IS_SFP_P2(local_id) {
            *status = (cpld_port_txfault_reg & 0b00000100) >> 2;
        } else if IS_SFP_P3(local_id) {
            *status = (cpld_port_txfault_reg & 0b00001000) >> 3;
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

    //QSFPDD
    if(IS_QSFPDD(local_id)) {
        AIM_LOG_ERROR("unsupported ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_UNSUPPORTED;
    } else if(IS_SFP(local_id)) {
        ONLP_TRY(file_read_hex(&cpld_port_rxlos_reg, CPLD2_SYSFS_PATH"/"SFP_RXLOS_ATTR));
        if(IS_SFP_P0(local_id)) {
            *status = (cpld_port_rxlos_reg & 0b00000001) >> 0;
        } else if IS_SFP_P1(local_id) {
            *status = (cpld_port_rxlos_reg & 0b00000010) >> 1;
        } else if IS_SFP_P2(local_id) {
            *status = (cpld_port_rxlos_reg & 0b00000100) >> 2;
        } else if IS_SFP_P3(local_id) {
            *status = (cpld_port_rxlos_reg & 0b00001000) >> 3;
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

    //QSFPDD
    if(IS_QSFPDD(local_id)) {
        AIM_LOG_ERROR("unsupported ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_UNSUPPORTED;
    } else if(IS_SFP(local_id)) {
        ONLP_TRY(file_read_hex(&cpld_port_txdisable_reg, CPLD2_SYSFS_PATH"/"SFP_TXDIS_ATTR));
        if(IS_SFP_P0(local_id)) {
            *status = (cpld_port_txdisable_reg & 0b00000001) >> 0;
        } else if IS_SFP_P1(local_id) {
            *status = (cpld_port_txdisable_reg & 0b00000010) >> 1;
        } else if IS_SFP_P2(local_id) {
            *status = (cpld_port_txdisable_reg & 0b00000100) >> 2;
        } else if IS_SFP_P3(local_id) {
            *status = (cpld_port_txdisable_reg & 0b00001000) >> 3;
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

    //QSFPDD
    if(IS_QSFPDD(local_id)) {
        /* QSFPDD */
        AIM_LOG_ERROR("unsupported ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_UNSUPPORTED;
    } else if(IS_SFP(local_id)) {
        port_id = local_id - QSFPDD_PORT_NUM;
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
            return ONLP_STATUS_E_PARAM;
            AIM_LOG_ERROR("unaccepted status, local_id=%d, status=%d, func=%s\n", local_id, status, __FUNCTION__);
        }

        ONLP_TRY(onlp_file_write_int(value, CPLD2_SYSFS_PATH"/"SFP_TXDIS_ATTR));
    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
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

    if(IS_QSFPDD(local_id)) {
        /* QSFPDD */
        port_id = local_id;
        bus_id = qsfpdd_port_eeprom_bus_id_array[port_id];
        snprintf(eeprom_path, sizeof(eeprom_path), "/sys/bus/i2c/devices/%d-0050/eeprom", bus_id);
        ret = onlp_file_read(data, 256, &size, eeprom_path);
    } else if(IS_SFP_P0(local_id) || IS_SFP_P1(local_id)) {
        /* SFP */
        port_id = local_id - QSFPDD_PORT_NUM;

        if(IS_SFP_P0(local_id)) {
            /* SFP Port0 */
            snprintf(command, sizeof(command), "ethtool -m %s raw on length 256 > /tmp/.sfp.%s.eeprom", SFP0_INTERFACE_NAME, SFP0_INTERFACE_NAME);
            snprintf(eeprom_path, sizeof(eeprom_path), "/tmp/.sfp.%s.eeprom", SFP0_INTERFACE_NAME);
        } else if(IS_SFP_P1(local_id)) {
            /* SFP Port0 */
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
    } else if(IS_SFP_P2(local_id)) {
        ret = onlp_file_read(data, 256, &size, "/sys/bus/i2c/devices/13-0050/eeprom");
    } else if(IS_SFP_P3(local_id)) {
        ret = onlp_file_read(data, 256, &size, "/sys/bus/i2c/devices/14-0050/eeprom");
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

    if(IS_QSFPDD(local_id)) {
        /* QSFPDD */
        port_id = local_id;
        bus_id = qsfpdd_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_readb(bus_id, devaddr, addr, ONLP_I2C_F_FORCE);
    } else if(IS_SFP_P0(local_id) || IS_SFP_P1(local_id)) {
        /* SFP Port0 or SFP Port1 */
        return ONLP_STATUS_E_UNSUPPORTED;

    } else if(IS_SFP_P2(local_id)) {
        /* SFP Port2 */
        bus_id = 13;
        ret = onlp_i2c_readb(bus_id, devaddr, addr, ONLP_I2C_F_FORCE);

    } else if(IS_SFP_P3(local_id)) {
        /* SFP Port3 */
        bus_id = 14;
        ret = onlp_i2c_readb(bus_id, devaddr, addr, ONLP_I2C_F_FORCE);

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

    if(IS_QSFPDD(local_id)) {
        /* QSFPDD */
        port_id = local_id;
        bus_id = qsfpdd_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_writeb(bus_id, devaddr, addr, value, ONLP_I2C_F_FORCE);
    } else if(IS_SFP_P0(local_id) || IS_SFP_P1(local_id)) {
        /* SFP Port0 or SFP Port1 */
        return ONLP_STATUS_E_UNSUPPORTED;
    } else if(IS_SFP_P2(local_id)) {
        /* SFP Port2 */
        bus_id = 13;
        ret = onlp_i2c_writeb(bus_id, devaddr, addr, value, ONLP_I2C_F_FORCE);
    } else if(IS_SFP_P3(local_id)) {
        /* SFP Port3 */
        bus_id = 14;
        ret = onlp_i2c_writeb(bus_id, devaddr, addr, value, ONLP_I2C_F_FORCE);
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

    if(IS_QSFPDD(local_id)) {
        /* QSFPDD */
        port_id = local_id;
        bus_id = qsfpdd_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_readw(bus_id, devaddr, addr, ONLP_I2C_F_FORCE);
    } else if(IS_SFP_P0(local_id) || IS_SFP_P1(local_id)) {
        /* SFP Port0 or SFP Port1 */
        return ONLP_STATUS_E_UNSUPPORTED;
    } else if(IS_SFP_P2(local_id)) {
        /* SFP Port2 */
        bus_id = 13;
        ret = onlp_i2c_readw(bus_id, devaddr, addr, ONLP_I2C_F_FORCE);
    } else if(IS_SFP_P3(local_id)) {
        /* SFP Port3 */
        bus_id = 14;
        ret = onlp_i2c_readw(bus_id, devaddr, addr, ONLP_I2C_F_FORCE);
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

    if(IS_QSFPDD(local_id)) {
        /* QSFPDD */
        port_id = local_id;
        bus_id = qsfpdd_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_writew(bus_id, devaddr, addr, value, ONLP_I2C_F_FORCE);
    } else if(IS_QSFPDD(local_id)) {
        /* SFP Port0 or SFP Port1 */
        return ONLP_STATUS_E_UNSUPPORTED;
    } else if(IS_SFP_P2(local_id)) {
        /* SFP Port2 */
        bus_id = 13;
        ret = onlp_i2c_writew(bus_id, devaddr, addr, value, ONLP_I2C_F_FORCE);
    } else if(IS_SFP_P3(local_id)) {
        /* SFP Port3 */
        bus_id = 14;
        ret = onlp_i2c_writew(bus_id, devaddr, addr, value, ONLP_I2C_F_FORCE);
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

    if(IS_QSFPDD(local_id)) {
        /* QSFPDD */
        port_id = local_id;
        bus_id = qsfpdd_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_block_read(bus_id, devaddr, addr, size, rdata, ONLP_I2C_F_FORCE);
    } else if(IS_SFP_P0(local_id) || IS_SFP_P1(local_id)) {
        /* SFP Port0 or SFP Port1 */
        return ONLP_STATUS_E_UNSUPPORTED;

    } else if(IS_SFP_P2(local_id)) {
        /* SFP Port2 */
        bus_id = 13;
        ret = onlp_i2c_block_read(bus_id, devaddr, addr, size, rdata, ONLP_I2C_F_FORCE);

    } else if(IS_SFP_P3(local_id)) {
        /* SFP Port3 */
        bus_id = 14;
        ret = onlp_i2c_block_read(bus_id, devaddr, addr, size, rdata, ONLP_I2C_F_FORCE);

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
            if(IS_QSFPDD(local_id)) {
                *rv = 1;
            }
            break;

        case ONLP_SFP_CONTROL_RESET_STATE:
            if(IS_QSFPDD(local_id)) {
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
            if(IS_SFP(local_id)) {
                *rv = 1;
            }
            break;

        case ONLP_SFP_CONTROL_LP_MODE:
            if(IS_QSFPDD(local_id)) {
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
            if(IS_QSFPDD(local_id)) {
                ONLP_TRY(set_sfpi_port_reset_status(local_id, value));
            } else {
                //do nothing
                return ONLP_STATUS_OK;
            }
            break;

        case ONLP_SFP_CONTROL_TX_DISABLE:
            if(IS_SFP(local_id)) {
                ONLP_TRY(set_sfpi_port_txdisable_status(local_id, value));
            } else {
                //do nothing
                return ONLP_STATUS_OK;
            }
            break;

        case ONLP_SFP_CONTROL_LP_MODE:
            if(IS_QSFPDD(local_id)) {
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
            if(IS_QSFPDD(local_id)) {
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
            if(IS_SFP(local_id)) {
                ONLP_TRY(get_sfpi_port_txdisable_status(local_id, &status));
                *value = status;
            } else {
                //do nothing
                return ONLP_STATUS_OK;
            }
            break;

        case ONLP_SFP_CONTROL_LP_MODE:
            if(IS_QSFPDD(local_id)) {
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
