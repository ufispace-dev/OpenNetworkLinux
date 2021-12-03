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
 * Power Supply Management Implementation.
 *
 ***********************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <onlp/platformi/psui.h>
#include <onlplib/file.h>
#include <AIM/aim.h>

#include "platform_lib.h"

#define CMD_FRU_INFO_GET        "ipmitool fru print %d | grep '%s' | cut -d':' -f2 | awk '{$1=$1};1' | tr -d '\n'"


/**
 * Get all information about the given PSU oid.
 *
 *            |----[01] ONLP_PSU_0----[05] ONLP_PSU0_FAN_1
 *            |                  |----[06] ONLP_PSU0_FAN_2
 *            |                  |----[07] ONLP_THERMAL_PSU0
 *            |
 *            |----[02] ONLP_PSU_1----[07] ONLP_PSU1_FAN_1
 *            |                  |----[08] ONLP_PSU1_FAN_2
 *            |                  |----[08] ONLP_THERMAL_PSU1
 */
static onlp_psu_info_t __onlp_psu_info[ONLP_PSU_COUNT] = { 
    { }, /* Not used */
    {   
        .hdr = { 
            .id = ONLP_PSU_ID_CREATE(ONLP_PSU_0),
            .description = "PSU-0",
            .poid = 0,
            .coids = { 
                ONLP_FAN_ID_CREATE(ONLP_PSU0_FAN_1),
                ONLP_FAN_ID_CREATE(ONLP_PSU0_FAN_2),
                ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU0),
            },  
        },  
        .model = "", 
        .serial = "", 
        .status = ONLP_PSU_STATUS_PRESENT,
        .caps = ONLP_PSU_CAPS_VIN | ONLP_PSU_CAPS_VOUT | ONLP_PSU_CAPS_IIN | 
                ONLP_PSU_CAPS_IOUT | ONLP_PSU_CAPS_PIN | ONLP_PSU_CAPS_POUT,
    },  
    {   
        .hdr = { 
            .id = ONLP_PSU_ID_CREATE(ONLP_PSU_1),
            .description = "PSU-1",
            .poid = 0,
            .coids = {
                ONLP_FAN_ID_CREATE(ONLP_PSU1_FAN_1),
                ONLP_FAN_ID_CREATE(ONLP_PSU1_FAN_2),
                ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU1),
            },
        },
        .model = "",
        .serial = "",
        .status = ONLP_PSU_STATUS_PRESENT,
        .caps = ONLP_PSU_CAPS_VIN | ONLP_PSU_CAPS_VOUT | ONLP_PSU_CAPS_IIN |
                ONLP_PSU_CAPS_IOUT | ONLP_PSU_CAPS_PIN | ONLP_PSU_CAPS_POUT,
    }
};

/**
 * @brief Get PSU Power Good Status
 * @param local_id: psu id
 * @status 1 if power good
 * @status 0 if power failed
 * @returns An error condition.
 */
static int get_psui_pwgood_status(int local_id, int *status)
{
    int psu_reg_value = 0;
    int psu_pwgood = 0;

    if (local_id == ONLP_PSU_0) {
        ONLP_TRY(file_read_hex(&psu_reg_value, "/sys/bus/i2c/devices/2-0030/cpld_psu_status_0"));
        psu_pwgood = (psu_reg_value & 0b00010000) ? 1 : 0;
    } else if (local_id == ONLP_PSU_1) {
        ONLP_TRY(file_read_hex(&psu_reg_value, "/sys/bus/i2c/devices/2-0030/cpld_psu_status_1"));
        psu_pwgood = (psu_reg_value & 0b00100000) ? 1 : 0;
    } else {
        AIM_LOG_ERROR("unknown psu id (%d), func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    *status = psu_pwgood;

    return ONLP_STATUS_OK;
}

/**
 * @brief Update the information of Model and Serial from PSU EEPROM
 * @param id The PSU Local ID
 * @param[out] info Receives the PSU information (model and serial).
 */
static int update_psui_fru_info(int local_id, onlp_psu_info_t* info)
{
    char cmd[100];
    char cmd_out[64];
    char fru_model[] = "Product Name";  //only Product Name can identify AC/DC
    char fru_serial[] = "Product Serial";

    //FRU (model)

    memset(cmd, 0, sizeof(cmd));
    memset(cmd_out, 0, sizeof(cmd_out));
    memset(info->model, 0, sizeof(info->model));

    snprintf(cmd, sizeof(cmd), CMD_FRU_INFO_GET, local_id, fru_model);

    //Get psu fru info (model) from BMC 
    if (exec_cmd(cmd, cmd_out, sizeof(cmd_out)) < 0) {
        AIM_LOG_ERROR("unable to read fru info from BMC, fru id=%d, cmd=%s, out=%s\n", local_id, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    //Check output is correct    
    if (strnlen(cmd_out, sizeof(cmd_out)) == 0){
        AIM_LOG_ERROR("unable to read fru info from BMC, cmd_out is empty, fru id=%d, cmd=%s, out=%s\n", local_id, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    snprintf(info->model, sizeof(info->model), "%s", cmd_out);

    //FRU (serial) 
    memset(cmd, 0, sizeof(cmd));
    memset(cmd_out, 0, sizeof(cmd_out));
    memset(info->serial, 0, sizeof(info->serial));

    snprintf(cmd, sizeof(cmd), CMD_FRU_INFO_GET, local_id, fru_serial);

    //Get psu fru info (model) from BMC
    if (exec_cmd(cmd, cmd_out, sizeof(cmd_out)) < 0) {
        AIM_LOG_ERROR("unable to read fru info from BMC, cmd_out is empty, fru id=%d, cmd=%s, out=%s\n", local_id, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    //Check output is correct
    if (strnlen(cmd_out, sizeof(cmd_out)) == 0) {
        AIM_LOG_ERROR("unable to read fru info from BMC, fru id=%d, cmd=%s, out=%s\n", local_id, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    snprintf(info->serial, sizeof(info->serial), "%s", cmd_out);

    return ONLP_STATUS_OK;
}

#if 0
/**
 * @brief Update the information of PSU Type from PSU Model (AC/DC)
 * @param id The PSU Local ID
 * @param[out] info Receives the PSU information (psu type).
 */
static int update_psui_psu_type(int local_id, onlp_psu_info_t* info)
{
    if (strstr(info->model, "AM") > 0) {
        info->type = ONLP_PSU_TYPE_AC;
    } else if (strstr(info->model, "EM") > 0) {
        info->type = ONLP_PSU_TYPE_DC48;
    } else {
        AIM_LOG_ERROR("unknown PSU type, model=%s, func=%s\n", info->model, __FUNCTION__);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}
#endif

/**
 * @brief Update the information structure for the given PSU
 * @param id The PSU Local ID
 * @param[out] info Receives the PSU information.
 */
static int update_psui_info(int local_id, onlp_psu_info_t* info)
{
    int stbmvout, stbmiout;
    float data;
    int attr_id_vin = -1, attr_id_vout = -1;
    int attr_id_iin = -1, attr_id_iout = -1;
    int attr_id_svout = -1, attr_id_siout = -1;
    //int pw_present;

    if ((info->status & ONLP_PSU_STATUS_PRESENT) == 0) {
        //not present, do nothing
        return ONLP_STATUS_OK;
    }

    if (local_id == ONLP_PSU_0) {
        attr_id_vin   = BMC_ATTR_ID_PSU0_VIN;
        attr_id_vout  = BMC_ATTR_ID_PSU0_VOUT;
        attr_id_iin   = BMC_ATTR_ID_PSU0_IIN;
        attr_id_iout  = BMC_ATTR_ID_PSU0_IOUT;
        attr_id_svout = BMC_ATTR_ID_PSU0_STBVOUT;
        attr_id_siout = BMC_ATTR_ID_PSU0_STBIOUT;
    } else if (local_id == ONLP_PSU_1) {
        attr_id_vin   = BMC_ATTR_ID_PSU1_VIN;
        attr_id_vout  = BMC_ATTR_ID_PSU1_VOUT;
        attr_id_iin   = BMC_ATTR_ID_PSU1_IIN;
        attr_id_iout  = BMC_ATTR_ID_PSU1_IOUT;
        attr_id_svout = BMC_ATTR_ID_PSU1_STBVOUT;
        attr_id_siout = BMC_ATTR_ID_PSU1_STBIOUT;
    } else {
        AIM_LOG_ERROR("unknown PSU_ID (%d), func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_INTERNAL;
    }

    /* Get power vin status */
    ONLP_TRY(bmc_sensor_read(attr_id_vin, PSU_SENSOR, &data));
    info->mvin = (int) (data*1000);
    //info->caps |= ONLP_PSU_CAPS_GET_VIN;

    /* Get power vout status */
    ONLP_TRY(bmc_sensor_read(attr_id_vout, PSU_SENSOR, &data));
    info->mvout = (int) (data*1000);
    //info->caps |= ONLP_PSU_CAPS_GET_VOUT;

    /* Get power iin status */
    ONLP_TRY(bmc_sensor_read(attr_id_iin, PSU_SENSOR, &data));
    info->miin = (int) (data*1000);
    //info->caps |= ONLP_PSU_CAPS_GET_IIN;

    /* Get power iout status */
    ONLP_TRY(bmc_sensor_read(attr_id_iout, PSU_SENSOR, &data));
    info->miout = (int) (data*1000);
    //info->caps |= ONLP_PSU_CAPS_GET_IOUT;

    /* Get standby power vout */
    ONLP_TRY(bmc_sensor_read(attr_id_svout, PSU_SENSOR, &data));
    stbmvout = (int) (data*1000);

    /* Get standby power iout */
    ONLP_TRY(bmc_sensor_read(attr_id_siout, PSU_SENSOR, &data));
    stbmiout = (int) (data*1000);

    /* Get power in and out */
    info->mpin = info->miin * info->mvin / 1000;
    info->mpout = (info->miout * info->mvout + stbmiout * stbmvout) / 1000;
    //info->caps |= ONLP_PSU_CAPS_GET_PIN | ONLP_PSU_CAPS_GET_POUT;

    /* Update FRU (model/serial) */
    ONLP_TRY(update_psui_fru_info(local_id, info));

#if 0
    /* Update PSU Type AC/DC */
    ONLP_TRY(update_psui_psu_type(local_id, info));
#endif

    return ONLP_STATUS_OK;
}

/**
 * @brief Initialize the PSU subsystem.
 */
int onlp_psui_init(void)
{
    lock_init();
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the information structure for the given PSU
 * @param id The PSU OID
 * @param info [out] Receives the PSU information.
 */
int onlp_psui_info_get(onlp_oid_t id, onlp_psu_info_t* info)
{
    int local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_psu_info_t */
    memset(info, 0, sizeof(onlp_psu_info_t));
    *info = __onlp_psu_info[local_id];
    ONLP_TRY(onlp_psui_hdr_get(id, &info->hdr));

    /* Update onlp_psu_info_t status */
    ONLP_TRY(onlp_psui_status_get(id, &info->status));

    if (local_id == ONLP_PSU_0 || ONLP_PSU_1) {
        ONLP_TRY(update_psui_info(local_id, info));
    } else {
        return ONLP_STATUS_E_PARAM;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the PSU's operational status.
 * @param id The PSU OID.
 * @param rv [out] Receives the operational status.
 */
int onlp_psui_status_get(onlp_oid_t id, uint32_t* status)
{
    int local_id = ONLP_OID_ID_GET(id);
    int psu_presence = 0, psu_pwgood = 0;

    /* clear psui status */
    *status = 0;

    /* get psu present status */
    ONLP_TRY(get_psui_present_status(local_id, &psu_presence));
    if (psu_presence == 0) {
        *status |= ONLP_PSU_STATUS_UNPLUGGED;
    } else if (psu_presence == 1) {
        *status |= ONLP_PSU_STATUS_PRESENT;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    /* get psu power good status */
    ONLP_TRY(get_psui_pwgood_status(local_id, &psu_pwgood));
    if (psu_pwgood == 0) {
        *status |= ONLP_PSU_STATUS_FAILED;
    } else if (psu_pwgood == 1) {
        *status &= ~ONLP_PSU_STATUS_FAILED;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the PSU's oid header.
 * @param id The PSU OID.
 * @param hdr [out] Receives the header.
 */
int onlp_psui_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* hdr)
{
    int local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_psu_info_t */
    *hdr = __onlp_psu_info[local_id].hdr;

    return ONLP_STATUS_OK;
}

/**
 * @brief Generic PSU ioctl
 * @param id The PSU OID
 * @param vargs The variable argument list for the ioctl call.
 */
int onlp_psui_ioctl(onlp_oid_t pid, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}
