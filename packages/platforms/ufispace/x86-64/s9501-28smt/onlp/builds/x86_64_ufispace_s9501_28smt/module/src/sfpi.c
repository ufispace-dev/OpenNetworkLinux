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
#include <x86_64_ufispace_s9501_28smt/x86_64_ufispace_s9501_28smt_config.h>
#include "x86_64_ufispace_s9501_28smt_log.h"
#include "platform_lib.h"

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
    char eeprom_path[512];
    int size = 0;
    int i2c_bus;

    memset(data, 0, 256);
    memset(eeprom_path, 0, sizeof(eeprom_path));

    //get i2c_bus from port
    i2c_bus = port + 6;

    //SFP and SFP+ Ports
    if (port >= SFP_START_NUM && port < PORT_NUM) {
        snprintf(eeprom_path, sizeof(eeprom_path), "/sys/bus/i2c/devices/%d-0050/eeprom", i2c_bus);
    } else { //unknown ports
        AIM_LOG_ERROR("unknown ports, port=%d\n", port);
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if(onlp_file_read(data, 256, &size, eeprom_path) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d), eeprom_path=%s\r\n", port, eeprom_path);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (size != 256) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d), size is different! eeprom_path=%s\r\n", port, eeprom_path);
        return ONLP_STATUS_E_INTERNAL;
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
