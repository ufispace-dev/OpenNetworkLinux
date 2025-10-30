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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <onlp/platformi/sfpi.h>
#include <onlplib/i2c.h>
#include <onlplib/file.h>
//#include <fcntl.h> /* For O_RDWR && open */
//#include <sys/ioctl.h>
//#include <dirent.h>

#include "platform_lib.h"

#define SYSFS_DEV_CLASS       "dev_class"
#define ALL_PORTS             -1

#define QSFP_NIF_PORT_NUM    40
#define QSFPDD_FAB_PORT_NUM  13
#define QSFPX_PORT_NUM       (QSFP_NIF_PORT_NUM+QSFPDD_FAB_PORT_NUM)
#define SFP_PORT_NUM          2
#define ALL_PORT_NUM         (QSFP_NIF_PORT_NUM+QSFPDD_FAB_PORT_NUM+SFP_PORT_NUM) //55

/* port order: QSFP_NIF(0-39), QSFPDD_FAB(40-52), SFP(53-54) */
#define IS_QSFP_NIF(_port)   (_port >= 0 && _port < QSFP_NIF_PORT_NUM)
#define IS_QSFPDD_FAB(_port) (_port >= QSFP_NIF_PORT_NUM && _port < QSFPX_PORT_NUM)
#define IS_QSFPX(_port)      (_port >= 0 && _port < QSFPX_PORT_NUM)
#define IS_SFP(_port)        (_port >= QSFPX_PORT_NUM && _port < ALL_PORT_NUM)
#define IS_SFP_P0(_port)     (_port == QSFPX_PORT_NUM)
#define IS_SFP_P1(_port)     (_port == (QSFPX_PORT_NUM+1))

#define SFP0_INTERFACE_NAME "enp2s0f0"
#define SFP1_INTERFACE_NAME "enp2s0f1"


static int qsfp_nif_port_eeprom_bus_id_array[QSFP_NIF_PORT_NUM] = { 25, 26, 27, 28, 29, \
                                                                    30, 31, 32, 33, 34, \
                                                                    35, 36, 37, 38, 39, \
                                                                    40, 41, 42, 43, 44, \
                                                                    46, 45, 48, 47, 50, \
                                                                    49, 52, 51, 54, 53, \
                                                                    56, 55, 58, 57, 60, \
                                                                    59, 62, 61, 64, 63 };

static char qsfp_nif_port_status_cpld_addr_array[QSFP_NIF_PORT_NUM][7] = {"1-0030", "1-0030", "1-0030", "1-0030", "1-0030", \
                                                                          "1-0030", "1-0030", "1-0030", "1-0030", "1-0030", \
                                                                          "1-0030", "1-0030", "1-0039", "1-0039", "1-0039", \
                                                                          "1-0039", "1-0039", "1-0039", "1-0039", "1-0039", \
                                                                          "1-0039", "1-0039", "1-0039", "1-0039", "1-003a", \
                                                                          "1-0039", "1-003a", "1-003a", "1-003a", "1-003a", \
                                                                          "1-003a", "1-003a", "1-003a", "1-003a", "1-003a", \
                                                                          "1-003a", "1-003a", "1-003a", "1-003b", "1-003b" };

static char qsfp_nif_port_status_cpld_index_array[QSFP_NIF_PORT_NUM][3] = {"0" , "1" , "2" , "3" , "4" , \
                                                                           "5" , "6" , "7" , "8" , "9" , \
                                                                           "10", "11", "0" , "1" , "2" , \
                                                                           "3" , "4" , "5" , "6" , "7" , \
                                                                           "9" , "8" , "11", "10", "0" , \
                                                                           "12", "2" , "1" , "4" , "3" , \
                                                                           "6" , "5" , "8" , "7" , "10", \
                                                                           "9" , "12", "11", "1" , "0" };

static int qsfpdd_fab_port_eeprom_bus_id_array[QSFPDD_FAB_PORT_NUM] = { 65, 66, 67, 68, 69, \
                                                                        70, 71, 72, 73, 74, \
                                                                        75, 76, 77};

static char qsfpdd_fab_port_status_cpld_addr_array[QSFPDD_FAB_PORT_NUM][7] = {"1-003b", "1-003b", "1-003b", "1-003c", "1-003c", \
                                                                              "1-003c", "1-003c", "1-003c", "1-003c", "1-003c", \
                                                                              "1-003c", "1-003c", "1-003c"};

static char qsfpdd_fab_port_status_cpld_index_array[QSFPDD_FAB_PORT_NUM][3] = {"0", "1", "2", "0", "1", \
                                                                               "2", "3", "4", "5", "6", \
                                                                               "7", "8", "9"};

#define SYSFS_DEV_CLASS                       "dev_class"
#define SYSFS_EEPROM                          "eeprom"
#define EEPROM_ADDR                           (0x50)
#define MASK_1000_0000 0x80
#define MASK_0000_0010 0x02

//SFF8636 TX Disable
#define SFF8636_EEPROM_OFFSET_TXDIS    0x56
#define SFF8636_EEPROM_TX_DIS          0x0f  /* txdis valid bit(bit0-bit3), xxxx 1111 */
#define SFF8636_EEPROM_TX_EN           0x0

//CMIS TX Disable
#define CMIS_PAGE_SIZE                        (128)
#define CMIS_PAGE_SUPPORTED_CTRL_ADV          (1)
#define CMIS_PAGE_TX_DIS                      (16)
#define CMIS_OFFSET_REVISION                  (1)
#define CMIS_OFFSET_MEMORY_MODEL              (2)
#define CMIS_OFFSET_TX_DIS                    (130)
#define CMIS_OFFSET_SUPPORTED_CTRL_ADV        (155)
#define CMIS_MASK_MEMORY_MODEL                (MASK_1000_0000)
#define CMIS_MASK_TX_DIS_ADV                  (MASK_0000_0010)
#define CMIS_VAL_TX_DIS                       (0xff)
#define CMIS_VAL_TX_EN                        (0x0)
#define CMIS_VAL_MEMORY_MODEL_PAGED           (0)
#define CMIS_VAL_TX_DIS_SUPPORTED             (1)
#define CMIS_VAL_VERSION_MIN                  (0x30)
#define CMIS_VAL_VERSION_MAX                  (0x5F)
#define CMIS_SEEK_TX_DIS_ADV                  (CMIS_PAGE_SIZE * CMIS_PAGE_SUPPORTED_CTRL_ADV + CMIS_OFFSET_SUPPORTED_CTRL_ADV)
#define CMIS_SEEK_TX_DIS                      (CMIS_PAGE_SIZE * CMIS_PAGE_TX_DIS + CMIS_OFFSET_TX_DIS)

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

static int ufi_port_to_eeprom_bus(int port)
{
    int bus = -1;

    if (IS_QSFP_NIF(port)) {
        bus =  qsfp_nif_port_eeprom_bus_id_array[port];
    } else if (IS_QSFPDD_FAB(port)) {
        bus =  qsfpdd_fab_port_eeprom_bus_id_array[port-QSFP_NIF_PORT_NUM];
    } else { //unknown ports
        AIM_LOG_ERROR("unknown ports, port=%d\n", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_PARAM;
    }

    return bus;
}

/**
 * @brief Get QSFP NIF/QSFPDD FAB/SFP Port Status
 * @param local_id: The port number.
 * @status 1 if present
 * @status 0 if absent
 * @returns An error condition.
 */
static int get_sfpi_port_present_status(int local_id, int *status)
{
    int ret = ONLP_STATUS_OK;
    int port_status_reg = 0;
    char port_status_sysfs[255] = "";
    int port_id = -1;
    char buf[ONLP_CONFIG_INFO_STR_MAX] = "";
    int len = 0;
    char command[256] = "";

    /* For QSFP NIF, QSFPDD Fabric, SFP Ports */
    if (IS_QSFP_NIF(local_id)) {
        port_id = local_id;

        snprintf(port_status_sysfs, sizeof(port_status_sysfs), "/sys/bus/i2c/devices/%s/cpld_qsfp_port_status_%s",
                qsfp_nif_port_status_cpld_addr_array[port_id], qsfp_nif_port_status_cpld_index_array[port_id]);

        ONLP_TRY(onlp_file_read((uint8_t*)&buf, ONLP_CONFIG_INFO_STR_MAX, &len, port_status_sysfs));
        //val 0 for presence, status set to 1
        port_status_reg = (int) strtol((char *)buf, NULL, 0);
        *status = !((port_status_reg & 0b00000010) >> 1);

    } else if (IS_QSFPDD_FAB(local_id)) {
        port_id = local_id - QSFP_NIF_PORT_NUM;

        snprintf(port_status_sysfs, sizeof(port_status_sysfs), "/sys/bus/i2c/devices/%s/cpld_qsfpdd_port_status_%s",
                qsfpdd_fab_port_status_cpld_addr_array[port_id], qsfpdd_fab_port_status_cpld_index_array[port_id]);

        ONLP_TRY(onlp_file_read((uint8_t*)&buf, ONLP_CONFIG_INFO_STR_MAX, &len, port_status_sysfs));
        //val 0 for presence, status set to 1
        port_status_reg = (int) strtol((char *)buf, NULL, 0);
        *status = !((port_status_reg & 0b00000010) >> 1);

    } else if (IS_SFP_P0(local_id)) {
        /* SFP Port0 - CPU */
        snprintf(command, sizeof(command), "ethtool -m %s raw on length 1 > /dev/null 2>&1", SFP0_INTERFACE_NAME);
        ret = system(command);
        *status = (ret==0) ? 1 : 0;

    } else if (IS_SFP_P1(local_id)) {
        /* SFP Port1 - CPU */
        snprintf(command, sizeof(command), "ethtool -m %s raw on length 1 > /dev/null 2>&1", SFP1_INTERFACE_NAME);
        ret = system(command);
        *status = (ret==0) ? 1 : 0;

    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_UNSUPPORTED;

    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get QSFP NIF/QSFPDD FAB Port Reset Status
 * @param local_id: The port number.
 * @status 1 if in reset state
 * @status 0 if normal (not reset)
 * @returns An error condition.
 */
static int get_sfpi_port_reset_status(int local_id, int *status)
{
    int port_reset_reg = 0;
    char port_reset_sysfs[255] = "";
    int port_id = -1;
    char buf[ONLP_CONFIG_INFO_STR_MAX] = "";
    int len = 0;

    /* For QSFP NIF, QSFPDD Fabric */
    if (IS_QSFP_NIF(local_id)) {
        port_id = local_id;

        snprintf(port_reset_sysfs, sizeof(port_reset_sysfs), "/sys/bus/i2c/devices/%s/cpld_qsfp_port_config_%s",
                qsfp_nif_port_status_cpld_addr_array[port_id], qsfp_nif_port_status_cpld_index_array[port_id]);

        ONLP_TRY(onlp_file_read((uint8_t*)&buf, ONLP_CONFIG_INFO_STR_MAX, &len, port_reset_sysfs));
        //register value 0 for reset, status set to 1
        port_reset_reg = (int) strtol((char *)buf, NULL, 0);
        *status = !((port_reset_reg & 0b00000001) >> 0);

    } else if (IS_QSFPDD_FAB(local_id)) {
        port_id = local_id - QSFP_NIF_PORT_NUM;

        snprintf(port_reset_sysfs, sizeof(port_reset_sysfs), "/sys/bus/i2c/devices/%s/cpld_qsfpdd_port_config_%s",
                qsfpdd_fab_port_status_cpld_addr_array[port_id], qsfpdd_fab_port_status_cpld_index_array[port_id]);

        ONLP_TRY(onlp_file_read((uint8_t*)&buf, ONLP_CONFIG_INFO_STR_MAX, &len, port_reset_sysfs));
        //register value 0 for reset, status set to 1
        port_reset_reg = (int) strtol((char *)buf, NULL, 0);
        *status = !((port_reset_reg & 0b00000001) >> 0);

    } else if (IS_SFP(local_id)) {
        AIM_LOG_ERROR("unsupported ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_UNSUPPORTED;

    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_UNSUPPORTED;

    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Set QSFP NIF/QSFPDD FAB Port Reset Status
 * @param local_id: The port number.
 * @param status: 1 if in reset state
 * @param status: 0 if normal (not reset)
 * @returns An error condition.
 */
static int set_sfpi_port_reset_status(int local_id, int status)
{
    int port_reset_reg = 0;
    char port_reset_sysfs[255] = "";
    int port_id = -1;
    char buf[ONLP_CONFIG_INFO_STR_MAX] = "";
    int len = 0;
    int value = 0;

    /* For QSFP NIF, QSFPDD Fabric */
    if (IS_QSFP_NIF(local_id)) {
        port_id = local_id;

        snprintf(port_reset_sysfs, sizeof(port_reset_sysfs), "/sys/bus/i2c/devices/%s/cpld_qsfp_port_config_%s",
                qsfp_nif_port_status_cpld_addr_array[port_id], qsfp_nif_port_status_cpld_index_array[port_id]);

        ONLP_TRY(onlp_file_read((uint8_t*)&buf, ONLP_CONFIG_INFO_STR_MAX, &len, port_reset_sysfs));
        //register value 0 for reset
        port_reset_reg = (int) strtol((char *)buf, NULL, 0);

        if (status == 0) {
            //status: 0 for normal, register value set to 1
            value = port_reset_reg | 0b00000001;
        } else if (status == 1) {
            //status: 1 for reset, register value set to 0
            value = port_reset_reg & 0b11111110;
        }

        ONLP_TRY(onlp_file_write_int(value, port_reset_sysfs));

    } else if (IS_QSFPDD_FAB(local_id)) {
        port_id = local_id - QSFP_NIF_PORT_NUM;

        snprintf(port_reset_sysfs, sizeof(port_reset_sysfs), "/sys/bus/i2c/devices/%s/cpld_qsfpdd_port_config_%s",
                qsfpdd_fab_port_status_cpld_addr_array[port_id], qsfpdd_fab_port_status_cpld_index_array[port_id]);

        ONLP_TRY(onlp_file_read((uint8_t*)&buf, ONLP_CONFIG_INFO_STR_MAX, &len, port_reset_sysfs));
        //register value 0 for reset
        port_reset_reg = (int) strtol((char *)buf, NULL, 0);

        if (status == 0) {
            //status: 0 for normal, register value set to 1
            value = port_reset_reg | 0b00000001;
        } else if (status == 1) {
            //status: 1 for reset, register value set to 0
            value = port_reset_reg & 0b11111110;
        }

        ONLP_TRY(onlp_file_write_int(value, port_reset_sysfs));

    } else if (IS_SFP(local_id)) {
        AIM_LOG_ERROR("unsupported ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_UNSUPPORTED;

    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;

    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get QSFP NIF/QSFPDD FAB Port Low Power Mode Status
 * @param local_id: The port number.
 * @status 1 if in low power mode state
 * @status 0 if normal mode
 * @returns An error condition.
 */
static int get_sfpi_port_lpmode_status(int local_id, int *status)
{
    int port_lpmode_reg = 0;
    char port_lpmode_sysfs[255] = "";
    int port_id = -1;
    char buf[ONLP_CONFIG_INFO_STR_MAX] = "";
    int len = 0;

    /* For QSFP NIF, QSFPDD Fabric */
    if (IS_QSFP_NIF(local_id)) {
        port_id = local_id;

        snprintf(port_lpmode_sysfs, sizeof(port_lpmode_sysfs), "/sys/bus/i2c/devices/%s/cpld_qsfp_port_config_%s",
                qsfp_nif_port_status_cpld_addr_array[port_id], qsfp_nif_port_status_cpld_index_array[port_id]);

        ONLP_TRY(onlp_file_read((uint8_t*)&buf, ONLP_CONFIG_INFO_STR_MAX, &len, port_lpmode_sysfs));
        //register value 1 for lpmode, status set to 1
        port_lpmode_reg = (int) strtol((char *)buf, NULL, 0);
        *status = (port_lpmode_reg & 0b00000100) >> 2;

    } else if (IS_QSFPDD_FAB(local_id)) {
        port_id = local_id - QSFP_NIF_PORT_NUM;

        snprintf(port_lpmode_sysfs, sizeof(port_lpmode_sysfs), "/sys/bus/i2c/devices/%s/cpld_qsfpdd_port_config_%s",
                qsfpdd_fab_port_status_cpld_addr_array[port_id], qsfpdd_fab_port_status_cpld_index_array[port_id]);

        ONLP_TRY(onlp_file_read((uint8_t*)&buf, ONLP_CONFIG_INFO_STR_MAX, &len, port_lpmode_sysfs));
        //register value 1 for lpmode, status set to 1
        port_lpmode_reg = (int) strtol((char *)buf, NULL, 0);
        *status = (port_lpmode_reg & 0b00000100) >> 2;

    } else if (IS_SFP(local_id)) {
        AIM_LOG_ERROR("unsupported ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_UNSUPPORTED;

    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_UNSUPPORTED;

    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Set QSFPDD NIF Port Low Power Mode Status
 * @param local_id: The port number.
 * @param status: 1 if in low power mode state
 * @param status: 0 if normal mode
 * @returns An error condition.
 */
static int set_sfpi_port_lpmode_status(int local_id, int status)
{
    int port_lpmode_reg = 0;
    char port_lpmode_sysfs[255] = "";
    int port_id = -1;
    char buf[ONLP_CONFIG_INFO_STR_MAX] = "";
    int len = 0;
    int value = 0;

    /* For QSFP NIF, QSFPDD Fabric */
    if (IS_QSFP_NIF(local_id)) {
        port_id = local_id;

        snprintf(port_lpmode_sysfs, sizeof(port_lpmode_sysfs), "/sys/bus/i2c/devices/%s/cpld_qsfp_port_config_%s",
                qsfp_nif_port_status_cpld_addr_array[port_id], qsfp_nif_port_status_cpld_index_array[port_id]);

        ONLP_TRY(onlp_file_read((uint8_t*)&buf, ONLP_CONFIG_INFO_STR_MAX, &len, port_lpmode_sysfs));
        //register value 1 for lpmode
        port_lpmode_reg = (int) strtol((char *)buf, NULL, 0);

        if (status == 0) {
            //status: 0 for normal, register value set to 0
            value = port_lpmode_reg & 0b11111011;
        } else if (status == 1) {
            //status: 1 for lpmode, register value set to 1
            value = port_lpmode_reg | 0b00000100;
        }

        ONLP_TRY(onlp_file_write_int(value, port_lpmode_sysfs));

    } else if (IS_QSFPDD_FAB(local_id)) {
        port_id = local_id - QSFP_NIF_PORT_NUM;

        snprintf(port_lpmode_sysfs, sizeof(port_lpmode_sysfs), "/sys/bus/i2c/devices/%s/cpld_qsfpdd_port_config_%s",
                qsfpdd_fab_port_status_cpld_addr_array[port_id], qsfpdd_fab_port_status_cpld_index_array[port_id]);

        ONLP_TRY(onlp_file_read((uint8_t*)&buf, ONLP_CONFIG_INFO_STR_MAX, &len, port_lpmode_sysfs));
        //register value 1 for lpmode
        port_lpmode_reg = (int) strtol((char *)buf, NULL, 0);

        if (status == 0) {
            //status: 0 for normal, register value set to 0
            value = port_lpmode_reg & 0b11111011;
        } else if (status == 1) {
            //status: 1 for reset, register value set to 1
            value = port_lpmode_reg | 0b00000100;
        }

        ONLP_TRY(onlp_file_write_int(value, port_lpmode_sysfs));

    } else if (IS_SFP(local_id)) {
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
    int port_txfault_reg = 0;
    char buf[ONLP_CONFIG_INFO_STR_MAX] = "";
    int len = 0;

    /* For SFP Only */
    if (IS_QSFP_NIF(local_id) || IS_QSFPDD_FAB(local_id)) {
        AIM_LOG_ERROR("unsupported ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_UNSUPPORTED;

    } else if (IS_SFP(local_id)) {
        ONLP_TRY(onlp_file_read((uint8_t*)&buf, ONLP_CONFIG_INFO_STR_MAX, &len, "/sys/bus/i2c/devices/1-0030/cpld_sfp_port_status"));
        port_txfault_reg = (int) strtol((char *)buf, NULL, 0);

        if (IS_SFP_P0(local_id)) {
            *status = (port_txfault_reg & 0b00000010) >> 1;
        } else if IS_SFP_P1(local_id) {
            *status = (port_txfault_reg & 0b00100000) >> 5;
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
    int port_rxlos_reg = 0;
    char buf[ONLP_CONFIG_INFO_STR_MAX] = "";
    int len = 0;

    /* For SFP Only */
    if (IS_QSFP_NIF(local_id) || IS_QSFPDD_FAB(local_id)) {
        AIM_LOG_ERROR("unsupported ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_UNSUPPORTED;

    } else if (IS_SFP(local_id)) {
        ONLP_TRY(onlp_file_read((uint8_t*)&buf, ONLP_CONFIG_INFO_STR_MAX, &len, "/sys/bus/i2c/devices/1-0030/cpld_sfp_port_status"));
        port_rxlos_reg = (int) strtol((char *)buf, NULL, 0);

        if (IS_SFP_P0(local_id)) {
            *status = (port_rxlos_reg & 0b00000100) >> 2;
        } else if IS_SFP_P1(local_id) {
            *status = (port_rxlos_reg & 0b01000000) >> 6;
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
    int port_txdisable_reg = 0;
    char buf[ONLP_CONFIG_INFO_STR_MAX] = "";
    int len = 0;

    /* For SFP Only */
    if (IS_QSFP_NIF(local_id) || IS_QSFPDD_FAB(local_id)) {
        AIM_LOG_ERROR("unsupported ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_UNSUPPORTED;

    } else if (IS_SFP(local_id)) {
        ONLP_TRY(onlp_file_read((uint8_t*)&buf, ONLP_CONFIG_INFO_STR_MAX, &len, "/sys/bus/i2c/devices/1-0030/cpld_sfp_port_config"));
        port_txdisable_reg = (int) strtol((char *)buf, NULL, 0);

        if (IS_SFP_P0(local_id)) {
            *status = (port_txdisable_reg & 0b00000001) >> 0;
        } else if IS_SFP_P1(local_id) {
            *status = (port_txdisable_reg & 0b00010000) >> 4;
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
    int port_txdisable_reg = 0;
    int port_mask = 0;
    char buf[ONLP_CONFIG_INFO_STR_MAX] = "";
    int len = 0;
    int value = 0;

    /* For SFP Only */
    if (IS_QSFP_NIF(local_id) || IS_QSFPDD_FAB(local_id)) {
        AIM_LOG_ERROR("unsupported ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_UNSUPPORTED;

    } else if (IS_SFP(local_id)) {
        if (IS_SFP_P0(local_id)) {
            port_mask = 0b00000001;
        } else {
            port_mask = 0b00010000;
        }

        ONLP_TRY(onlp_file_read((uint8_t*)&buf, ONLP_CONFIG_INFO_STR_MAX, &len, "/sys/bus/i2c/devices/1-0030/cpld_sfp_port_config"));
        port_txdisable_reg = (int) strtol((char *)buf, NULL, 0);

        if (status == 0) {
            /* Normal */
            value = port_txdisable_reg & ~port_mask;
        } else if (status == 1) {
            /* Tx Disable */
            value = port_txdisable_reg | port_mask;
        } else {
            return ONLP_STATUS_E_PARAM;
            AIM_LOG_ERROR("unaccepted status, local_id=%d, status=%d, func=%s\n", local_id, status, __FUNCTION__);
        }

        ONLP_TRY(onlp_file_write_int(value, "/sys/bus/i2c/devices/1-0030/cpld_sfp_port_config"));

    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;

    }

    return ONLP_STATUS_OK;
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

    bus = ufi_port_to_eeprom_bus(port);

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
    int bus=0;

    bus = ufi_port_to_eeprom_bus(port);

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

    if (!IS_QSFPX(port) || !onlp_sfpi_is_present(port)) { // not QSFPX or module absent
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

    for (i = 0; i < PORT_TYPE_DICT_SIZE ; ++i) {
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
    if (port != ALL_PORTS) {
        return onlp_sfpi_dev_class_update_port(port);
    }

    // Check all ports and only update all QSFPX ports
    for (int i = 0; i < ALL_PORT_NUM; ++i) {
        if(IS_QSFPX(i)) {
            if (onlp_sfpi_dev_class_update_port(i) < 0) {
                rv = ONLP_STATUS_E_INTERNAL;
            }
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
 * @brief Get SFF-8636 Port TX Disable Status by EEPROM
 * @param port: The port number.
 * @param status: 1 if tx disable (turn on)
 * @param status: 0 if normal (turn off)
 * @returns An error condition.
 */
static int ufi_sff8636_txdisable_status_get(int port, int* status)
{
    uint8_t value = 0;

    if (onlp_sfpi_is_present(port) != 1) {
        return ONLP_STATUS_OK;
    }

    ONLP_TRY(value = onlp_sfpi_dev_readb(port, EEPROM_ADDR, SFF8636_EEPROM_OFFSET_TXDIS));
    // Check each bit of the 'value' has all bits set to 1 meets TX Disable condition (all channels disabled).
    if (value == SFF8636_EEPROM_TX_DIS) {
        *status = 1;
    } else {
        *status = 0;
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
static int ufi_sff8636_txdisable_status_set(int port, int status)
{
    uint8_t value = 0, readback = 0;

    if (status == 0) {
        value = SFF8636_EEPROM_TX_EN;
    } else if (status == 1) {
        value = SFF8636_EEPROM_TX_DIS;
    } else {
        AIM_LOG_ERROR("[%s] invalid status, port=%d, status=%d\n", __FUNCTION__, port, status);
        return ONLP_STATUS_E_PARAM;
    }

    if (onlp_sfpi_is_present(port) != 1) {
        return ONLP_STATUS_OK;
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
    uint8_t value = 0;
    char sysfs_path[256] = {0};
    int cmis_ver = 0;
    int mem_model = 0;
    int bus = 0;
    int seek = 0;
    int length = 0;
    int tx_dis_adv = 0;

    //Check CMIS version on lower page 0x01
    cmis_ver = onlp_sfpi_dev_readb(port, EEPROM_ADDR, CMIS_OFFSET_REVISION);
    if (cmis_ver < CMIS_VAL_VERSION_MIN || cmis_ver > CMIS_VAL_VERSION_MAX) {
        AIM_LOG_INFO("Port[%d] CMIS version %x.%x is not supported (certified range is %x.x-%x.x)\n",
            port, cmis_ver/16, cmis_ver%16, CMIS_VAL_VERSION_MIN/16, CMIS_VAL_VERSION_MAX/16);
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    //Check CMIS memory model on lower page 0x02 bit[7]
    mem_model = ufi_mask_shift(onlp_sfpi_dev_readb(port, EEPROM_ADDR, CMIS_OFFSET_MEMORY_MODEL), CMIS_MASK_MEMORY_MODEL);
    if (mem_model != CMIS_VAL_MEMORY_MODEL_PAGED) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    //Check CMIS Tx disable advertisement on page 0x01 offset[155] bit[1]

    bus = ufi_port_to_eeprom_bus(port);
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

    tx_dis_adv = ufi_mask_shift(value, CMIS_MASK_TX_DIS_ADV);

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
static int ufi_cmis_txdisable_status_get(int port, int* status)
{
    int ret = 0;
    uint8_t value = 0;
    char sysfs_path[256] = {0};
    int bus = 0;
    int length = 0;

    // Check module present
    if (onlp_sfpi_is_present(port) != 1) {
        return ONLP_STATUS_OK;
    }

    // tx disable support check
    if ((ret=ufi_cmis_txdisable_supported(port)) != ONLP_STATUS_OK) {
        return ret;
    }

    bus = ufi_port_to_eeprom_bus(port);
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

    // Check each bit of the 'value' has all bits set to 1 meets TX Disable condition (all channels disabled).
    if (value == CMIS_VAL_TX_DIS) {
        *status = 1;
    } else {
        *status = 0;
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
static int ufi_cmis_txdisable_status_set(int port, int status)
{
    uint8_t value = 0, readback = 0;
    char sysfs_path[256] = {0};
    int bus = 0;
    int seek = CMIS_SEEK_TX_DIS;

    // Check module present
    if (onlp_sfpi_is_present(port) != 1) {
        return ONLP_STATUS_OK;
    }

    // tx disable support check
    if (ufi_cmis_txdisable_supported(port) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    // set value
    if (status == 0) {
        value = CMIS_VAL_TX_EN;
    } else if (status == 1) {
        value = CMIS_VAL_TX_DIS;
    } else {
        AIM_LOG_ERROR("[%s] unaccepted status, port=%d, status=%d\n", __FUNCTION__, port, status);
        return ONLP_STATUS_E_PARAM;
    }

    // set sysfs_path
    bus = ufi_port_to_eeprom_bus(port);
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
    lock_init();
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

    if (local_id < ALL_PORT_NUM) {
        ONLP_TRY(get_sfpi_port_present_status(local_id, &status));
    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
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
 * @param[out] dst Receives the RX_LOS bitmap.
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

    if (onlp_sfpi_is_present(local_id) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", local_id);
        return ONLP_STATUS_OK;
    }

    if (IS_QSFP_NIF(local_id)) {
        /* QSFP */
        port_id = local_id;
        bus_id = qsfp_nif_port_eeprom_bus_id_array[port_id];
        snprintf(eeprom_path, sizeof(eeprom_path), "/sys/bus/i2c/devices/%d-0050/eeprom", bus_id);
        ret = onlp_file_read(data, 256, &size, eeprom_path);

    } else if (IS_QSFPDD_FAB(local_id)) {
        /* QSFPDD */
        port_id = local_id - QSFP_NIF_PORT_NUM;
        bus_id = qsfpdd_fab_port_eeprom_bus_id_array[port_id];
        snprintf(eeprom_path, sizeof(eeprom_path), "/sys/bus/i2c/devices/%d-0050/eeprom", bus_id);
        ret = onlp_file_read(data, 256, &size, eeprom_path);

    } else if (IS_SFP(local_id)) {
        /* SFP */
        port_id = local_id - (QSFP_NIF_PORT_NUM + QSFPDD_FAB_PORT_NUM);

        if (IS_SFP_P0(local_id)) {
            /* SFP Port0 */
            snprintf(command, sizeof(command), "ethtool -m %s raw on length 256 > /tmp/.sfp.%s.eeprom", SFP0_INTERFACE_NAME, SFP0_INTERFACE_NAME);
            snprintf(eeprom_path, sizeof(eeprom_path), "/tmp/.sfp.%s.eeprom", SFP0_INTERFACE_NAME);

        } else if (IS_SFP_P1(local_id)) {
            /* SFP Port0 */
            snprintf(command, sizeof(command), "ethtool -m %s raw on length 256 > /tmp/.sfp.%s.eeprom", SFP1_INTERFACE_NAME, SFP1_INTERFACE_NAME);
            snprintf(eeprom_path, sizeof(eeprom_path), "/tmp/.sfp.%s.eeprom", SFP1_INTERFACE_NAME);

        } else {
            AIM_LOG_ERROR("unknown SFP ports, port=%d\n", port_id);
            return ONLP_STATUS_E_UNSUPPORTED;
        }

        ret = system(command);
        if (ret != 0) {
            AIM_LOG_ERROR("Unable to read sfp eeprom (port_id=%d), func=%s\n", port_id, __FUNCTION__);
            return ONLP_STATUS_E_INTERNAL;
        }

        ret = onlp_file_read(data, 256, &size, eeprom_path);

    } else {
        AIM_LOG_ERROR("unknown ports, local_id=%d, func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    if (ret < 0) {
        check_and_do_i2c_mux_reset(local_id);
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
    int local_id = port;
    int ret = ONLP_STATUS_OK;
    int bus_id = -1;
    int port_id = -1;

    if (onlp_sfpi_is_present(local_id) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", local_id);
        return ONLP_STATUS_OK;
    }

    if (IS_QSFP_NIF(local_id)) {
        /* QSFP */
        port_id = local_id;
        bus_id = qsfp_nif_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_readb(bus_id, devaddr, addr, ONLP_I2C_F_FORCE);

    } else if (IS_QSFPDD_FAB(local_id)) {
        /* QSFPDD */
        port_id = local_id - QSFP_NIF_PORT_NUM;
        bus_id = qsfpdd_fab_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_readb(bus_id, devaddr, addr, ONLP_I2C_F_FORCE);

    } else if (IS_SFP(local_id)) {
        /* SFP */
        return ONLP_STATUS_E_UNSUPPORTED;

    } else {
        return ONLP_STATUS_E_PARAM;

    }

    if (ret < 0) {
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

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", local_id);
        return ONLP_STATUS_OK;
    }

    if (IS_QSFP_NIF(local_id)) {
        /* QSFP */
        port_id = local_id;
        bus_id = qsfp_nif_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_writeb(bus_id, devaddr, addr, value, ONLP_I2C_F_FORCE);

    } else if (IS_QSFPDD_FAB(local_id)) {
        /* QSFPDD */
        port_id = local_id - QSFP_NIF_PORT_NUM;
        bus_id = qsfpdd_fab_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_writeb(bus_id, devaddr, addr, value, ONLP_I2C_F_FORCE);

    } else if (IS_SFP(local_id)) {
        /* SFP */
        return ONLP_STATUS_E_UNSUPPORTED;

    } else {
        return ONLP_STATUS_E_PARAM;

    }

    if (ret < 0) {
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

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", local_id);
        return ONLP_STATUS_OK;
    }

    if (IS_QSFP_NIF(local_id)) {
        /* QSFP */
        port_id = local_id;
        bus_id = qsfp_nif_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_readw(bus_id, devaddr, addr, ONLP_I2C_F_FORCE);

    } else if (IS_QSFPDD_FAB(local_id)) {
        /* QSFPDD */
        port_id = local_id - QSFP_NIF_PORT_NUM;
        bus_id = qsfpdd_fab_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_readw(bus_id, devaddr, addr, ONLP_I2C_F_FORCE);

    } else if (IS_SFP(local_id)) {
        /* SFP */
        return ONLP_STATUS_E_UNSUPPORTED;

    } else {
        return ONLP_STATUS_E_PARAM;

    }

    if (ret < 0) {
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

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", local_id);
        return ONLP_STATUS_OK;
    }

    if (IS_QSFP_NIF(local_id)) {
        /* QSFP */
        port_id = local_id;
        bus_id = qsfp_nif_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_writew(bus_id, devaddr, addr, value, ONLP_I2C_F_FORCE);

    } else if (IS_QSFPDD_FAB(local_id)) {
        /* QSFPDD */
        port_id = local_id - QSFP_NIF_PORT_NUM;
        bus_id = qsfpdd_fab_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_writew(bus_id, devaddr, addr, value, ONLP_I2C_F_FORCE);

    } else if (IS_SFP(local_id)) {
        /* SFP */
        return ONLP_STATUS_E_UNSUPPORTED;

    } else {
        return ONLP_STATUS_E_PARAM;
    }

    if (ret < 0) {
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
    char eeprom_path[128];
    char command[256] = "";
    int ret_size = 0;

    memset(eeprom_path, 0, sizeof(eeprom_path));
    memset(command, 0, sizeof(command));

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", local_id);
        return ONLP_STATUS_OK;
    }

    if (IS_QSFP_NIF(local_id)) {
        /* QSFP */
        port_id = local_id;
        bus_id = qsfp_nif_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_block_read(bus_id, devaddr, addr, size, rdata, ONLP_I2C_F_FORCE);

    } else if (IS_QSFPDD_FAB(local_id)) {
        /* QSFPDD */
        port_id = local_id - QSFP_NIF_PORT_NUM;
        bus_id = qsfpdd_fab_port_eeprom_bus_id_array[port_id];
        ret = onlp_i2c_block_read(bus_id, devaddr, addr, size, rdata, ONLP_I2C_F_FORCE);

    } else if (IS_SFP(local_id)) {
        /* SFP */
        port_id = local_id - (QSFP_NIF_PORT_NUM + QSFPDD_FAB_PORT_NUM);

        if (IS_SFP_P0(local_id)) {
            /* SFP Port0 */
            snprintf(command, sizeof(command), "ethtool -m %s raw on length %d > /tmp/.sfp.%s.eeprom", SFP0_INTERFACE_NAME, size, SFP0_INTERFACE_NAME);
            snprintf(eeprom_path, sizeof(eeprom_path), "/tmp/.sfp.%s.eeprom", SFP0_INTERFACE_NAME);

        } else if (IS_SFP_P1(local_id)) {
            /* SFP Port0 */
            snprintf(command, sizeof(command), "ethtool -m %s raw on length %d > /tmp/.sfp.%s.eeprom", SFP1_INTERFACE_NAME, size, SFP1_INTERFACE_NAME);
            snprintf(eeprom_path, sizeof(eeprom_path), "/tmp/.sfp.%s.eeprom", SFP1_INTERFACE_NAME);

        } else {
            AIM_LOG_ERROR("unknown SFP ports, port=%d\n", port_id);
            return ONLP_STATUS_E_UNSUPPORTED;
        }

        ret = system(command);
        if (ret != 0) {
            AIM_LOG_ERROR("Unable to read sfp eeprom (port_id=%d), func=%s\n", port_id, __FUNCTION__);
            return ONLP_STATUS_E_INTERNAL;
        }

        ONLP_TRY(onlp_file_read(rdata, size, &ret_size, eeprom_path));

    } else {
        return ONLP_STATUS_E_PARAM;

    }

    if (ret < 0) {
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
    switch (control) {
        case ONLP_SFP_CONTROL_RESET:
            if (IS_QSFP_NIF(local_id) || IS_QSFPDD_FAB(local_id)) {
                *rv = 1;
            }
            break;

        case ONLP_SFP_CONTROL_RESET_STATE:
            if (IS_QSFP_NIF(local_id) || IS_QSFPDD_FAB(local_id)) {
                *rv = 1;
            }
            break;

        case ONLP_SFP_CONTROL_RX_LOS:
            if (IS_SFP(local_id)) {
                *rv = 1;
            }
            break;

        case ONLP_SFP_CONTROL_TX_FAULT:
            if (IS_SFP(local_id)) {
                *rv = 1;
            }
            break;

        case ONLP_SFP_CONTROL_TX_DISABLE:
        case ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL:
            if (IS_SFP(local_id) || IS_QSFPX(local_id)) {
                *rv = 1;
            }
            break;

        case ONLP_SFP_CONTROL_LP_MODE:
            if (IS_QSFP_NIF(local_id) || IS_QSFPDD_FAB(local_id)) {
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
    int rc = 0;
    int dev_class = 0;

    switch (control) {
        case ONLP_SFP_CONTROL_RESET:
            if (IS_QSFP_NIF(local_id) || IS_QSFPDD_FAB(local_id)) {
                ONLP_TRY(set_sfpi_port_reset_status(local_id, value));
            } else {
                //do nothing
                return ONLP_STATUS_OK;
            }
            break;

        case ONLP_SFP_CONTROL_TX_DISABLE:
        case ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL:
            if (IS_SFP(local_id)) {
                ONLP_TRY(set_sfpi_port_txdisable_status(local_id, value));
            } else if (IS_QSFPX(port)) {
                ONLP_TRY(dev_class = onlp_sfpi_dev_class_update(port));

                if (dev_class == 1) { //SFF8636 module
                    ONLP_TRY(rc = ufi_sff8636_txdisable_status_set(port, value));
                } else if (dev_class == 3) { //CMIS module
                    ONLP_TRY(rc = ufi_cmis_txdisable_status_set(port, value));
                } else if (dev_class < 0) {
                    AIM_LOG_ERROR("Port[%d] dev_class %d is not supported for tx disable control.\n", port, dev_class);
                    return ONLP_STATUS_E_UNSUPPORTED;
                } else { // module absent or other case
                    return ONLP_STATUS_OK;
				}
            } else {
                rc = ONLP_STATUS_E_UNSUPPORTED;
            }
            break;

        case ONLP_SFP_CONTROL_LP_MODE:
            if (IS_QSFP_NIF(local_id) || IS_QSFPDD_FAB(local_id)) {
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
    int rc = 0;
    int dev_class = 0;

    switch (control) {
        case ONLP_SFP_CONTROL_RESET_STATE:
            if (IS_QSFP_NIF(local_id) || IS_QSFPDD_FAB(local_id)) {
                ONLP_TRY(get_sfpi_port_reset_status(local_id, &status));
                *value = status;
            } else {
                //do nothing
                return ONLP_STATUS_OK;
            }
            break;

        case ONLP_SFP_CONTROL_RX_LOS:
            if (IS_SFP(local_id)) {
                ONLP_TRY(get_sfpi_port_rxlos_status(local_id, &status));
                *value = status;
            } else {
                //do nothing
                return ONLP_STATUS_OK;
            }
            break;

        case ONLP_SFP_CONTROL_TX_FAULT:
            if (IS_SFP(local_id)) {
                ONLP_TRY(get_sfpi_port_txfault_status(local_id, &status));
                *value = status;
            } else {
                //do nothing
                return ONLP_STATUS_OK;
            }
            break;

        case ONLP_SFP_CONTROL_TX_DISABLE:
        case ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL:
            if (IS_SFP(local_id)) {
                ONLP_TRY(get_sfpi_port_txdisable_status(local_id, &status));
                *value = status;
            } else if (IS_QSFPX(port)) {
                ONLP_TRY(dev_class = onlp_sfpi_dev_class_update(port));

                if (dev_class == 1) { //SFF8636 module
                    ONLP_TRY(rc = ufi_sff8636_txdisable_status_get(port, value));
                } else if (dev_class == 3) { //CMIS module
                    rc = ufi_cmis_txdisable_status_get(port, value);
                } else if (dev_class < 0) {
                    AIM_LOG_ERROR("Port[%d] dev_class %d is not supported for tx disable control.\n", port, dev_class);
                    return ONLP_STATUS_E_UNSUPPORTED;
                } else { // module absent or other case
                    return ONLP_STATUS_OK;
				}
            } else {
                rc = ONLP_STATUS_E_UNSUPPORTED;
            }
            break;

        case ONLP_SFP_CONTROL_LP_MODE:
            if (IS_QSFP_NIF(local_id) || IS_QSFPDD_FAB(local_id)) {
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
