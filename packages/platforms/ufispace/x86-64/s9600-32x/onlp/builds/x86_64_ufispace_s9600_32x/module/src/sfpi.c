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
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <onlplib/sfp.h>
#include <onlplib/file.h>
#include <onlp/platformi/sfpi.h>
#include <x86_64_ufispace_s9600_32x/x86_64_ufispace_s9600_32x_config.h>
#include "x86_64_ufispace_s9600_32x_log.h"
#include "platform_lib.h"

/**
 * @brief Initialize the SFPI subsystem.
 */
int
onlp_sfpi_init(void)
{
    lock_init();
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the bitmap of SFP-capable port numbers.
 * @param bmap [out] Receives the bitmap.
 */
int
onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
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
int
onlp_sfpi_is_present(int port)
{
    int status=1;

    //QSFP, QSFPDD, SFP Ports
    if (port < (QSFP_NUM + QSFPDD_NUM)) { //QSFP, QSFPDD
        if (qsfp_present_get(port, &status) != ONLP_STATUS_OK) {
            AIM_LOG_ERROR("qsfp_presnet_get() failed, port=%d\n", port);
            return ONLP_STATUS_E_INTERNAL;
        }
    } else if (port < PORT_NUM) { //SFP
        if (sfp_present_get(port, &status) != ONLP_STATUS_OK) {
            AIM_LOG_ERROR("sfp_presnet_get() failed, port=%d\n", port);
            return ONLP_STATUS_E_INTERNAL;
        }
    } else { //unkonwn ports
        AIM_LOG_ERROR("unknown ports, port=%d\n", port);
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    return status;
}

/**
 * @brief Return the presence bitmap for all SFP ports.
 * @param dst Receives the presence bitmap.
 */
int
onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int p = 0;
    int rc = 0;

    for (p = 0; p < PORT_NUM; p++) {
        rc = onlp_sfpi_is_present(p);
        AIM_BITMAP_MOD(dst, p, rc);
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Read the SFP EEPROM.
 * @param port The port number.
 * @param data Receives the SFP data.
 */
int
onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{
    char eeprom_path[512];
    char ethtool_cmd[96];
    int size = 0;
    int real_port = 0;

    memset(eeprom_path, 0, sizeof(eeprom_path));
    memset(data, 0, 256);
    memset(ethtool_cmd, 0, sizeof(ethtool_cmd));

    //QSFP, QSFPDD, SFP real ports
    if (port >= 0 && port < (QSFP_NUM + QSFPDD_NUM)) { //QSFPDD
        real_port = port;
        snprintf(eeprom_path, sizeof(eeprom_path), "/sys/bus/i2c/devices/%d-0050/eeprom", real_port+29);
    } else if (port < PORT_NUM) { //SFP
        real_port = port - QSFP_NUM - QSFPDD_NUM;
        snprintf(eeprom_path, sizeof(eeprom_path), "/sys/bus/i2c/devices/%d-0050/eeprom", real_port + 25);
    }


    if(onlp_file_read(data, 256, &size, eeprom_path) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d)\r\n", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (size != 256) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d), size is different!\r\n", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Return the RX_LOS bitmap for all SFP ports.
 * @param dst Receives the RX_LOS bitmap.
 */
int
onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int i=0, value=0, rc=0;
    int qsfpx_num = (QSFP_NUM + QSFPDD_NUM);

    /* Populate bitmap - QSFP and QSFPDD ports*/
    for(i = 0; i < qsfpx_num; i++) {
        AIM_BITMAP_MOD(dst, i, 0);
    }

    /* Populate bitmap - SFP+ ports*/
    for(i = qsfpx_num; i < PORT_NUM; i++) {
        if ((rc=onlp_sfpi_control_get(i, ONLP_SFP_CONTROL_RX_LOS, &value)) != ONLP_STATUS_OK) {
            return ONLP_STATUS_E_INTERNAL;
        } else {
            AIM_BITMAP_MOD(dst, i, value);
        }
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get an SFP control.
 * @param port The port.
 * @param control The control
 * @param [out] value Receives the current value.
 */
int
onlp_sfpi_control_get(int port, onlp_sfp_control_t control, int* value)
{
    int rc;
    uint8_t data[8];
    int data_len = 0, data_value = 0, data_mask = 0;
    char sysfs_path[128];
    int cpld_addr = 0x0;
    int qsfpx_num = (QSFP_NUM + QSFPDD_NUM);
    int real_port = 0;

    memset(data, 0, sizeof(data));
    memset(sysfs_path, 0, sizeof(sysfs_path));

    //QSFPDD ports are not supported
    if (port < qsfpx_num || port >= PORT_NUM) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    real_port = port - QSFP_NUM - QSFPDD_NUM;

    cpld_addr = 0x31;

    switch(real_port) {
        case 0:
        case 1:
            //INBAND sysfs path
            if (control == ONLP_SFP_CONTROL_RX_LOS || control == ONLP_SFP_CONTROL_TX_FAULT) {
                snprintf(sysfs_path, sizeof(sysfs_path), SYS_DEV "%d-%04x/cpld_sfp_status_p0_p1_inband", I2C_BUS_1, cpld_addr);
            } else if (control == ONLP_SFP_CONTROL_TX_DISABLE) {
                snprintf(sysfs_path, sizeof(sysfs_path), SYS_DEV "%d-%04x/cpld_sfp_config_p0_p1_inband", I2C_BUS_1, cpld_addr);
            } else {
                return ONLP_STATUS_E_UNSUPPORTED;
            }
            break;
        case 2:
        case 3:
            if (control == ONLP_SFP_CONTROL_RX_LOS || control == ONLP_SFP_CONTROL_TX_FAULT) {
                snprintf(sysfs_path, sizeof(sysfs_path), SYS_DEV "%d-%04x/cpld_sfp_status_p2_p3_inband", I2C_BUS_1, cpld_addr);
            } else if (control == ONLP_SFP_CONTROL_TX_DISABLE) {
                snprintf(sysfs_path, sizeof(sysfs_path), SYS_DEV "%d-%04x/cpld_sfp_config_p2_p3_inband", I2C_BUS_1, cpld_addr);
            } else {
            return ONLP_STATUS_E_UNSUPPORTED;
            }
            break;
        default:
            break;
    }

    if ((rc = onlp_file_read(data, sizeof(data), &data_len, sysfs_path)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("onlp_file_read failed, error=%d, sysfs=%s", rc, sysfs_path);
        return ONLP_STATUS_E_INTERNAL;
    }

    //hex to int
    data_value = (int) strtol((char *)data, NULL, 0);

    switch(control) {
        case ONLP_SFP_CONTROL_RX_LOS:
            {
                switch(real_port){
                    case 0:
                        data_mask = 0x04;
                        break;
                    case 1:
                        data_mask = 0x40;
                        break;
                    case 2:
                        data_mask = 0x04;
                        break;
                    case 3:
                        data_mask = 0x40;
                        break;
                    default:
                        break;

                }
                *value = data_value & data_mask;
                rc = ONLP_STATUS_OK;
                break;
            }

        case ONLP_SFP_CONTROL_TX_FAULT:
            {
                switch(real_port){
                    case 0:
                        data_mask = 0x02;
                        break;
                    case 1:
                        data_mask = 0x20;
                        break;
                    case 2:
                        data_mask = 0x02;
                        break;
                    case 3:
                        data_mask = 0x20;
                        break;
                    default:
                        break;
                }
                *value = data_value & data_mask;
                rc = ONLP_STATUS_OK;
                break;
            }

        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                switch(real_port){
                    case 0:
                        data_mask = 0x01;
                        break;
                    case 1:
                        data_mask = 0x10;
                        break;
                    case 2:
                        data_mask = 0x01;
                        break;
                    case 3:
                        data_mask = 0x10;
                        break;
                    default:
                        break;
                }
                *value = data_value & data_mask;
                rc = ONLP_STATUS_OK;
                break;
            }

        default:
            rc = ONLP_STATUS_E_UNSUPPORTED;
    }

    return rc;
}

/**
 * @brief Set an SFP control.
 * @param port The port.
 * @param control The control.
 * @param value The value.
 */
int
onlp_sfpi_control_set(int port, onlp_sfp_control_t control, int value)
{
    int rc;
    uint8_t data[8];
    int data_len = 0, data_value = 0, data_mask = 0;
    char sysfs_path[128];
    int cpld_addr = 0x0;
    int qsfpx_num = (QSFP_NUM + QSFPDD_NUM);
    int reg_val = 0;
    int real_port = 0;

    memset(data, 0, sizeof(data));
    memset(sysfs_path, 0, sizeof(sysfs_path));

    //QSFPDD ports are not supported
    if (port < qsfpx_num || port >= PORT_NUM) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    real_port = port - QSFP_NUM - QSFPDD_NUM;
    switch(real_port) {
        case 0:
        case 1:
            cpld_addr = 0x30;
            //CPU sysfs path
            if (control == ONLP_SFP_CONTROL_TX_DISABLE) {
                snprintf(sysfs_path, sizeof(sysfs_path), SYS_DEV "%d-%04x/cpld_sfp_config_cpu", I2C_BUS_1, cpld_addr);
            } else {
                return ONLP_STATUS_E_UNSUPPORTED;
            }
            break;
        case 2:
        case 3:
            cpld_addr = 0x31;
            if (control == ONLP_SFP_CONTROL_TX_DISABLE) {
                snprintf(sysfs_path, sizeof(sysfs_path), SYS_DEV "%d-%04x/cpld_sfp_config_inband", I2C_BUS_1, cpld_addr);
            } else {
            return ONLP_STATUS_E_UNSUPPORTED;
            }
            break;
        default:
            break;
    }

    if ((rc = onlp_file_read(data, sizeof(data), &data_len, sysfs_path)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("onlp_file_read failed, error=%d, sysfs=%s", rc, sysfs_path);
        return ONLP_STATUS_E_INTERNAL;
    }

    //hex to int
    data_value = (int) strtol((char *)data, NULL, 0);

    switch(control) {
        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                //SFP+ Port 0
                switch(real_port){
                    case 0:
                        data_mask = 0x01;
                        break;
                    case 1:
                        data_mask = 0x10;
                        break;
                    case 2:
                        data_mask = 0x01;
                        break;
                    case 3:
                        data_mask = 0x10;
                        break;
                    default:
                        break;

                }

                if (value == 1) {
                    reg_val = data_value | data_mask;
                } else {
                    reg_val = data_value & ~data_mask;
                }
                if ((rc = onlp_file_write_int(reg_val, sysfs_path)) != ONLP_STATUS_OK) {
                    AIM_LOG_ERROR("Unable to write cpld_sfp_config, error=%d, sysfs=%s, reg_val=%x", rc, sysfs_path, reg_val);
                    return ONLP_STATUS_E_INTERNAL;
                }
                rc = ONLP_STATUS_OK;
                break;
            }

        default:
            rc = ONLP_STATUS_E_UNSUPPORTED;
        }

    return rc;
}

/**
 * @brief Deinitialize the SFP driver.
 */
int
onlp_sfpi_denit(void)
{
    return ONLP_STATUS_OK;
}
