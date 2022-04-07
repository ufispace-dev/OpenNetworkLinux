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

#define PSU0_PRESENT_MASK       0b00000001
#define PSU1_PRESENT_MASK       0b00000010
#define PSU0_PWGOOD_MASK        0b00010000
#define PSU1_PWGOOD_MASK        0b00100000
#define PSU_STATUS_PRESENT      1
#define PSU_STATUS_POWER_GOOD   1
/* SYSFS */
#define MB_CPLD_PSU_STS_ATTR    "cpld_psu_status"
#define SYSFS_PSU_STATUS        MB_CPLD1_SYSFS_PATH"/"MB_CPLD_PSU_STS_ATTR
#define PSU_STATUS              ONLP_OID_STATUS_FLAG_PRESENT
#define PSU_CAPS                ONLP_PSU_CAPS_GET_VIN|ONLP_PSU_CAPS_GET_VOUT|   \
                                ONLP_PSU_CAPS_GET_IIN|ONLP_PSU_CAPS_GET_IOUT|   \
                                ONLP_PSU_CAPS_GET_PIN|ONLP_PSU_CAPS_GET_POUT
#define PSU_TYPE                ONLP_PSU_TYPE_AC
#define CHASSIS_PSU_INFO(idx, desc, fid, tid)       \
    {                                               \
        .hdr = {                                    \
            .id = ONLP_PSU_ID_CREATE(idx),          \
            .description = desc,                    \
            .poid = ONLP_OID_CHASSIS,               \
            .coids = {                              \
                ONLP_THERMAL_ID_CREATE(tid),        \
                ONLP_FAN_ID_CREATE(fid),            \
            },                                      \
            .status = PSU_STATUS,                   \
        },                                          \
        .model = "",                                \
        .serial = "",                               \
        .caps = PSU_CAPS,                           \
        .type = PSU_TYPE,                           \
    }


/**
 * Get all information about the given PSU oid.
 *
 *            |----[01] ONLP_PSU_0----[05] ONLP_PSU_0_FAN
 *            |
 *            |----[02] ONLP_PSU_1----[06] ONLP_PSU_1_FAN
 */
static onlp_psu_info_t __onlp_psu_info[] = {
    { }, /* Not used */
    CHASSIS_PSU_INFO(ONLP_PSU_0, "PSU-0", ONLP_PSU_0_FAN, ONLP_THERMAL_PSU0),
    CHASSIS_PSU_INFO(ONLP_PSU_1, "PSU-1", ONLP_PSU_1_FAN, ONLP_THERMAL_PSU1),
};

static char *vendors[] = {"DELTA", "FSPGROUP"};

/**
 * @brief get psu presnet status
 * @param local_id: psu id
 * @param[out] psu_present: psu present status(0: absence, 1: presence)
 */
 int psu_present_get(int *psu_present, int local_id)
{
    int status;
    int mask;

    if(psu_present == NULL) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if(local_id == ONLP_PSU_0) {
        mask = PSU0_PRESENT_MASK;
    } else if(local_id == ONLP_PSU_1) {
        mask = PSU1_PRESENT_MASK;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    ONLP_TRY(file_read_hex(&status, SYSFS_PSU_STATUS));

    *psu_present = ((status & mask)? 0 : 1);

    return ONLP_STATUS_OK;
}

/**
 * @brief get psu pwgood status
 * @param local_id: psu id
 * @param[out] psu_pwgood: psu power good status (0: not good, 1: good)
 */
int psu_pwgood_get(int *psu_pwgood, int local_id)
{
    int status;
    int mask;

    if(psu_pwgood == NULL) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if(local_id == ONLP_PSU_0) {
        mask = PSU0_PWGOOD_MASK;
    } else if(local_id == ONLP_PSU_1) {
        mask = PSU1_PWGOOD_MASK;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    ONLP_TRY(file_read_hex(&status, SYSFS_PSU_STATUS));

    *psu_pwgood = ((status & mask)? 1 : 0);

    return ONLP_STATUS_OK;
}

/**
 * @brief get psu type
 * @param local_id: psu id
 * @param[out] psu_type: psu type(ONLP_PSU_TYPE_AC, ONLP_PSU_TYPE_DC48)
 * @param fru_in: input fru node. we will use the input node informations to get psu type
 */
int get_psu_type(int local_id, int *psu_type, bmc_fru_t *fru_in)
{
    bmc_fru_t *fru = NULL;
    bmc_fru_t fru_tmp = {0};

    if(psu_type == NULL) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if(fru_in == NULL) {
        fru = &fru_tmp;
        ONLP_TRY(bmc_fru_read(local_id, fru));
    } else {
        fru = fru_in;
    }

    if (strncmp(fru->vendor.val, vendors[0], sizeof(BMC_FRU_ATTR_KEY_VALUE_SIZE))==0) { //Delta
        //read from part_num
        if (fru->part_num.val[7] == 'A') {
            *psu_type = ONLP_PSU_TYPE_AC;
        } else if (fru->part_num.val[7] == 'D') {
            *psu_type = ONLP_PSU_TYPE_DC48;
        } else {
            AIM_LOG_ERROR("unknown PSU type, vendor=%d, model=%s, func=%s\n", fru->vendor.val, fru->part_num.val, __FUNCTION__);
            return ONLP_STATUS_E_INTERNAL;
        }
    } else if (strncmp(fru->vendor.val, vendors[1], sizeof(BMC_FRU_ATTR_KEY_VALUE_SIZE))==0) { //FSP FIXME: check rule
        //read from name
        if (strstr(fru->name.val, "AM") > 0) {
            *psu_type = ONLP_PSU_TYPE_AC;
        } else if (strstr(fru->name.val, "EM") > 0) {
            *psu_type = ONLP_PSU_TYPE_DC48;
        } else {
            AIM_LOG_ERROR("unknown PSU type, vendor=%d, name=%s, func=%s\n", fru->vendor.val, fru->name.val, __FUNCTION__);
            return ONLP_STATUS_E_INTERNAL;
        }
    } else {
        *psu_type = ONLP_PSU_TYPE_INVALID;
        AIM_LOG_ERROR("unknown PSU type, vendor=%s, model=%s, func=%s", fru->vendor.val, fru->name.val, __FUNCTION__);
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
    bmc_fru_t fru = {0};

    //read fru data
    ONLP_TRY(bmc_fru_read(local_id, &fru));

    //update FRU model
    memset(info->model, 0, sizeof(info->model));
    if (strncmp(fru.vendor.val, vendors[1], sizeof(BMC_FRU_ATTR_KEY_VALUE_SIZE))==0) {
        //read product name for FSP
        snprintf(info->model, sizeof(info->model), "%s", fru.name.val);
    } else {
        //read part num for others
        snprintf(info->model, sizeof(info->model), "%s", fru.part_num.val);
    }

    //update FRU serial
    memset(info->serial, 0, sizeof(info->serial));
    snprintf(info->serial, sizeof(info->serial), "%s", fru.serial.val);

    //update FRU type
    ONLP_TRY(get_psu_type(local_id, &info->type, &fru));

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
    ONLP_TRY(psu_present_get(&psu_presence, local_id));

    if (psu_presence != PSU_STATUS_PRESENT) {
        ONLP_OID_STATUS_FLAG_SET(hdr, UNPLUGGED);
    } else {
        ONLP_OID_STATUS_FLAG_SET(hdr, PRESENT);
    }

    /* get psu power good status */
    ONLP_TRY(psu_pwgood_get(&psu_pwgood, local_id));

    if (psu_pwgood != PSU_STATUS_POWER_GOOD) {
        ONLP_OID_STATUS_FLAG_SET(hdr, FAILED);
    } else {
        ONLP_OID_STATUS_FLAG_SET(hdr, OPERATIONAL);
    }

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
    int index_offset = 0;

    if(local_id == ONLP_PSU_1) {
        index_offset = 6;
    }

    /* Get power vin status */
    ONLP_TRY(bmc_sensor_read(BMC_ATTR_ID_PSU0_VIN + index_offset, PSU_SENSOR, &data));
    info->mvin = (int) (data*1000);
    info->caps |= ONLP_PSU_CAPS_GET_VIN;

    /* Get power vout status */
    ONLP_TRY(bmc_sensor_read(BMC_ATTR_ID_PSU0_VOUT + index_offset, PSU_SENSOR, &data));
    info->mvout = (int) (data*1000);
    info->caps |= ONLP_PSU_CAPS_GET_VOUT;

    /* Get power iin status */
    ONLP_TRY(bmc_sensor_read(BMC_ATTR_ID_PSU0_IIN + index_offset, PSU_SENSOR, &data));
    info->miin = (int) (data*1000);
    info->caps |= ONLP_PSU_CAPS_GET_IIN;

    /* Get power iout status */
    ONLP_TRY(bmc_sensor_read(BMC_ATTR_ID_PSU0_IOUT + index_offset, PSU_SENSOR, &data));
    info->miout = (int) (data*1000);
    info->caps |= ONLP_PSU_CAPS_GET_IOUT;

    /* Get standby power vout */
    ONLP_TRY(bmc_sensor_read(BMC_ATTR_ID_PSU0_STBVOUT + index_offset, PSU_SENSOR, &data));
    stbmvout = (int) (data*1000);

    /* Get standby power iout */
    ONLP_TRY(bmc_sensor_read(BMC_ATTR_ID_PSU0_STBIOUT + index_offset, PSU_SENSOR, &data));
    stbmiout = (int) (data*1000);

    /* Get power in and out */
    info->mpin = info->miin * info->mvin / 1000;
    info->mpout = (info->miout * info->mvout + stbmiout * stbmvout) / 1000;
    info->caps |= ONLP_PSU_CAPS_GET_PIN | ONLP_PSU_CAPS_GET_POUT;

    /* Update FRU (model/serial) */
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
int onlp_psui_hdr_get(onlp_oid_id_t id, onlp_oid_hdr_t* hdr)
{
    int ret = ONLP_STATUS_OK;
    int local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_psu_info_t */
    *hdr = __onlp_psu_info[local_id].hdr;

    /* Update onlp_oid_hdr_t */
    ret = update_psui_status(local_id, hdr);

    return ret;}

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
