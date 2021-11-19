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
 * PSU
 *
 ***********************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <onlp/platformi/psui.h>
#include <onlplib/file.h>
#include <AIM/aim.h>

#include "platform_lib.h"


#define PSU0_PRESENT_MASK      0x01
#define PSU1_PRESENT_MASK      0x02
#define PSU0_PWGOOD_MASK       0x10
#define PSU1_PWGOOD_MASK       0x20
#define PSU_STATUS_PRESENT     1
#define PSU_STATUS_POWER_GOOD  1

#define SYSFS_PSU_STATUS        LPC_FMT "psu_status"
#define CMD_FRU_INFO_GET        "ipmitool fru print %d"IPMITOOL_REDIRECT_ERR" | grep '%s' | cut -d':' -f2 | awk '{$1=$1};1' | tr -d '\n'"

/**
 * Get all information about the given PSU oid.
 *
 * [01] CHASSIS
 *            |----[01] ONLP_PSU_0----[21] ONLP_THERMAL_PSU_0
 *            |                  |----[06] ONLP_PSU_0_FAN
 *            |----[02] ONLP_PSU_1----[22] ONLP_THERMAL_PSU_1
 *            |                  |----[07] ONLP_PSU_1_FAN
 */
static onlp_psu_info_t __onlp_psu_info[] = {
    { }, /* Not used */
    {
        .hdr = {
            .id = ONLP_PSU_ID_CREATE(ONLP_PSU_0),
            .description = "PSU-0",
            .poid = ONLP_OID_CHASSIS,
            .coids = {
                ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU_0),
                ONLP_FAN_ID_CREATE(ONLP_PSU_0_FAN),
            },
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .model = "",
        .serial = "",
        .caps = ONLP_PSU_CAPS_GET_VIN|ONLP_PSU_CAPS_GET_VOUT|ONLP_PSU_CAPS_GET_IIN|ONLP_PSU_CAPS_GET_IOUT|ONLP_PSU_CAPS_GET_PIN|ONLP_PSU_CAPS_GET_POUT,
        .type = ONLP_PSU_TYPE_AC,
    },
    {
        .hdr = {
            .id = ONLP_PSU_ID_CREATE(ONLP_PSU_1),
            .description = "PSU-1",
            .poid = ONLP_OID_CHASSIS,
            .coids = {
                ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU_1),
                ONLP_FAN_ID_CREATE(ONLP_PSU_1_FAN),
            },
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .model = "",
        .serial = "",
        .caps = ONLP_PSU_CAPS_GET_VIN|ONLP_PSU_CAPS_GET_VOUT|ONLP_PSU_CAPS_GET_IIN|ONLP_PSU_CAPS_GET_IOUT|ONLP_PSU_CAPS_GET_PIN|ONLP_PSU_CAPS_GET_POUT,
        .type = ONLP_PSU_TYPE_AC,
    },
};

/**
 * @brief get psu presnet status
 * @param[out] pw_present: psu present status(0: absence, 1: presence)
 * @param local_id: psu id
 */
int get_psu_present_status(int *pw_present, int local_id)
{
    int psu_reg_value = 0;
    int mask;

    if(pw_present == NULL) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (local_id == ONLP_PSU_0) {
        mask = PSU0_PRESENT_MASK;
    } else if (local_id == ONLP_PSU_1) {
        mask = PSU1_PRESENT_MASK;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    ONLP_TRY(file_read_hex(&psu_reg_value, SYSFS_PSU_STATUS));
    *pw_present = (psu_reg_value & mask) ? 0 : 1;

    return ONLP_STATUS_OK;
}

/**
 * @brief get psu pwgood status
 * @param[out] psu_pwgood: psu power good status (0: not good, 1: good)
 * @param local_id: psu id
 */
static int get_psu_pwgood_status(int *psu_pwgood, int local_id)
{
    int psu_reg_value = 0;
    int mask;

    if(psu_pwgood == NULL) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (local_id == ONLP_PSU_0) {
        mask = PSU0_PWGOOD_MASK;
    } else if (local_id == ONLP_PSU_1) {
        mask = PSU1_PWGOOD_MASK;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    ONLP_TRY(file_read_hex(&psu_reg_value, SYSFS_PSU_STATUS));
    *psu_pwgood = (psu_reg_value & mask) ? 1 : 0;

    return ONLP_STATUS_OK;
}

/**
 * @brief Update the status of PSU's oid header.
 * @param id The PSU ID.
 * @param[out] hdr Receives the header.
 */
static int update_psui_status(int local_id, onlp_oid_hdr_t* hdr) {
    int psu_presence = 0, psu_pwgood = 0;

    /* clear psui status */
    hdr->status = 0;

    /* get psu present status */
    ONLP_TRY(get_psu_present_status(&psu_presence, local_id));
    if (psu_presence == 0) {
        ONLP_OID_STATUS_FLAG_SET(hdr, UNPLUGGED);
    } else if (psu_presence == 1) {
        ONLP_OID_STATUS_FLAG_SET(hdr, PRESENT);
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    /* get psu power good status */
    ONLP_TRY(get_psu_pwgood_status(&psu_pwgood, local_id));
    if (psu_pwgood == 0) {
        ONLP_OID_STATUS_FLAG_SET(hdr, FAILED);
    } else if (psu_pwgood == 1) {
        ONLP_OID_STATUS_FLAG_SET(hdr, OPERATIONAL);
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Update the information of Model and Serial from PSU EEPROM
 * @param id The PSU Local ID
 * @param[out] info Receives the PSU information (model and serial).
 */
static int update_psui_fru_info(int local_id, onlp_psu_info_t* info)
{
    char cmd[256];
    char cmd_out[64];
    char fru_model[] = "Product Name";  //only Product Name can identify AC/DC  //FIXME
    char fru_serial[] = "Product Serial";


    //FRU (model)
    memset(cmd, 0, sizeof(cmd));
    memset(cmd_out, 0, sizeof(cmd_out));
    memset(info->model, 0, sizeof(info->model));
    // FIXME correct fru id
    snprintf(cmd, sizeof(cmd), CMD_FRU_INFO_GET, local_id, fru_model);


    //Get psu fru info (model) from BMC
    if (exec_cmd(cmd, cmd_out, sizeof(cmd_out)) < 0) {
        AIM_LOG_ERROR("unable to read fru info from BMC, fru id=%d, cmd=%s, out=%s\n", local_id, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    //Check output is correct
    if (strnlen(cmd_out, sizeof(cmd_out))==0){
        AIM_LOG_ERROR("unable to read fru info from BMC, fru id=%d, cmd=%s, out=%s\n", local_id, cmd, cmd_out);
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
        AIM_LOG_ERROR("unable to read fru info from BMC, fru id=%d, cmd=%s, out=%s\n", local_id, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    //Check output is correct
    if (strnlen(cmd_out, sizeof(cmd_out))==0){
        AIM_LOG_ERROR("unable to read fru info from BMC, fru id=%d, cmd=%s, out=%s\n", local_id, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    snprintf(info->serial, sizeof(info->serial), "%s", cmd_out);

    return ONLP_STATUS_OK;
}

/**
 * @brief Update the information structure for the given PSU
 * @param id The PSU Local ID
 * @param[out] info Receives the PSU information.
 */
static int update_psui_info(int local_id, onlp_psu_info_t* info)
{
    int stbmvout, stbmiout;
    float data;
    int attr_vin, attr_vout, attr_iin, attr_iout, attr_stbvout, attr_stbiout;

    if (local_id == ONLP_PSU_0) {
        attr_vin = BMC_ATTR_ID_PSU0_VIN;
        attr_vout = BMC_ATTR_ID_PSU0_VOUT;
        attr_iin = BMC_ATTR_ID_PSU0_IIN;
        attr_iout = BMC_ATTR_ID_PSU0_IOUT;
        attr_stbvout = BMC_ATTR_ID_PSU0_STBVOUT;
        attr_stbiout = BMC_ATTR_ID_PSU0_STBIOUT;
    } else {
        attr_vin = BMC_ATTR_ID_PSU1_VIN;
        attr_vout = BMC_ATTR_ID_PSU1_VOUT;
        attr_iin = BMC_ATTR_ID_PSU1_IIN;
        attr_iout = BMC_ATTR_ID_PSU1_IOUT;
        attr_stbvout = BMC_ATTR_ID_PSU1_STBVOUT;
        attr_stbiout = BMC_ATTR_ID_PSU1_STBIOUT;
    }

    /* Get power vin status */
    ONLP_TRY(bmc_sensor_read(attr_vin , PSU_SENSOR, &data));
    info->mvin = (int) (data*1000);

    /* Get power vout status */
    ONLP_TRY(bmc_sensor_read(attr_vout, PSU_SENSOR, &data));
    info->mvout = (int) (data*1000);

    /* Get power iin status */
    ONLP_TRY(bmc_sensor_read(attr_iin, PSU_SENSOR, &data));
    info->miin = (int) (data*1000);

    /* Get power iout status */
    ONLP_TRY(bmc_sensor_read(attr_iout, PSU_SENSOR, &data));
    info->miout = (int) (data*1000);

    /* Get standby power vout */
    ONLP_TRY(bmc_sensor_read(attr_stbvout, PSU_SENSOR, &data));
    stbmvout = (int) (data*1000);

    /* Get standby power iout */
    ONLP_TRY(bmc_sensor_read(attr_stbiout, PSU_SENSOR, &data));
    stbmiout = (int) (data*1000);

    /* Get power in and out */
    info->mpin = info->miin * info->mvin / 1000;
    info->mpout = (info->miout * info->mvout + stbmiout * stbmvout) / 1000;

    /* Get FRU (model/serial) */
    ONLP_TRY(update_psui_fru_info(local_id, info));

    return ONLP_STATUS_OK;
}

/**
 * @brief Software initialization of the PSU module.
 */
int onlp_psui_sw_init(void)
{
    lock_init();
    return ONLP_STATUS_OK;
}

/**
 * @brief Hardware initialization of the PSU module.
 * @param flags The hardware initialization flags.
 */
int onlp_psui_hw_init(uint32_t flags)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Deinitialize the psu software module.
 * @note The primary purpose of this API is to properly
 * deallocate any resources used by the module in order
 * faciliate detection of real resouce leaks.
 */
int onlp_psui_sw_denit(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Validate a PSU OID.
 * @param id The id.
 */
int onlp_psui_id_validate(onlp_oid_id_t id)
{
    return ONLP_OID_ID_VALIDATE_RANGE(id, 1, ONLP_PSU_MAX-1);
}

/**
 * @brief Get the PSU's oid header.
 * @param id The PSU OID.
 * @param[out] rv Receives the header.
 */
int onlp_psui_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* hdr)
{
    int ret = ONLP_STATUS_OK;
    int local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_psu_info_t */
    *hdr = __onlp_psu_info[local_id].hdr;

    /* Update onlp_oid_hdr_t */
    ret = update_psui_status(local_id, hdr);

    return ret;
}

/**
 * @brief Get the information structure for the given PSU
 * @param id The PSU OID
 * @param[out] info Receives the PSU information.
 */
int onlp_psui_info_get(onlp_oid_id_t id, onlp_psu_info_t* info)
{
    int local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_psu_info_t */
    memset(info, 0, sizeof(onlp_psu_info_t));
    *info = __onlp_psu_info[local_id];
    ONLP_TRY(onlp_psui_hdr_get(id, &info->hdr));

    if (ONLP_OID_STATUS_FLAG_GET_VALUE(&info->hdr, PRESENT) == 0) {
        return ONLP_STATUS_OK;
    }

    ONLP_TRY(update_psui_info(local_id, info));

    return ONLP_STATUS_OK;
}
