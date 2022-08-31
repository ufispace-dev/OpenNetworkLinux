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
#include <onlp/platformi/psui.h>
#include "platform_lib.h"

/**
 * Get all information about the given PSU oid.
 *
 *            |----[01] ONLP_PSU0----[13] ONLP_PSU0_FAN1
 *            |                 |----[07] ONLP_THERMAL_PSU0
 *            |
 *            |----[02] ONLP_PSU1----[14] ONLP_PSU1_FAN1
 *            |                 |----[08] ONLP_THERMAL_PSU1
 */

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_PSU(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)
#define PSU_INFO(id, desc, fid, tid)            \
    {                                           \
        { ONLP_PSU_ID_CREATE(id), desc, POID_0, \
            {                                   \
                ONLP_FAN_ID_CREATE(fid),        \
                ONLP_THERMAL_ID_CREATE(tid),    \
            }                                   \
        }                                       \
    }
/* SYSFS */
#define PSU_STATUS_ATTR     "cpld_psu_status"
#define PSU0_PRESENT_MASK   0b00000100
#define PSU1_PRESENT_MASK   0b00001000
#define PSU0_PWGOOD_MASK    0b00000001
#define PSU1_PWGOOD_MASK    0b00000010

static onlp_psu_info_t psu_info[] =
{
    { }, /* Not used */
    PSU_INFO(ONLP_PSU0, "PSU-0", ONLP_PSU0_FAN1, ONLP_THERMAL_PSU0),
    PSU_INFO(ONLP_PSU1, "PSU-1", ONLP_PSU1_FAN1, ONLP_THERMAL_PSU1),
};

static psu_support_info_t psu_support_list[] =
{
    {"FSPGROUP", "YESM1300AM-2A02P10", ONLP_PSU_TYPE_AC},
    //{"FSPGROUP", "YESM1300AM-2R04P10", ONLP_PSU_TYPE_AC},
    {"ACBEL", "FSH082-HAAG", ONLP_PSU_TYPE_AC},
    {"ACBEL", "R1BA2132B-PHAA", ONLP_PSU_TYPE_AC},
    {"FSPGROUP", "YESM1300CM-2A01N01", ONLP_PSU_TYPE_DC48},
    {"FSPGROUP", "YESM1300CM-2R01N01", ONLP_PSU_TYPE_DC48},
    {"ACBEL", "FSJ033-HAGG", ONLP_PSU_TYPE_DC48},
    {"ACBEL", "R1BD2132B-OHAA", ONLP_PSU_TYPE_DC48},
};

/**
 * @brief Get PSU Present Status
 * @param local_id: psu id
 * @status 1 if presence
 * @status 0 if absence
 * @returns An error condition.
 */
int get_psui_present_status(int local_id, int *status)
{
    int psu_reg_value = 0;
    int psu_presence = 0;

    if(local_id == ONLP_PSU0) {
        ONLP_TRY(file_read_hex(&psu_reg_value, CPLD1_SYSFS_PATH"/"PSU_STATUS_ATTR));
        psu_presence = (psu_reg_value & PSU0_PRESENT_MASK) ? 0 : 1;
    } else if(local_id == ONLP_PSU1) {
        ONLP_TRY(file_read_hex(&psu_reg_value, CPLD1_SYSFS_PATH"/"PSU_STATUS_ATTR));
        psu_presence = (psu_reg_value & PSU1_PRESENT_MASK) ? 0 : 1;
    } else {
        AIM_LOG_ERROR("unknown psu id (%d), func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    *status = psu_presence;

    return ONLP_STATUS_OK;
}

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

    if(local_id == ONLP_PSU0) {
        ONLP_TRY(file_read_hex(&psu_reg_value, CPLD1_SYSFS_PATH"/"PSU_STATUS_ATTR));
        psu_pwgood = (psu_reg_value & PSU0_PWGOOD_MASK) ? 1 : 0;
    } else if(local_id == ONLP_PSU1) {
        ONLP_TRY(file_read_hex(&psu_reg_value, CPLD1_SYSFS_PATH"/"PSU_STATUS_ATTR));
        psu_pwgood = (psu_reg_value & PSU1_PWGOOD_MASK) ? 1 : 0;
    } else {
        AIM_LOG_ERROR("unknown psu id (%d), func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    *status = psu_pwgood;

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
    int i, max;

    if(psu_type == NULL) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if(fru_in == NULL) {
        fru = &fru_tmp;
        ONLP_TRY(bmc_fru_read(local_id, fru));
    } else {
        fru = fru_in;
    }

    *psu_type = ONLP_PSU_TYPE_INVALID;
    max = sizeof(psu_support_list)/sizeof(*psu_support_list);
    for (i = 0; i < max; ++i) {
        if ((strncmp(fru->vendor.val, psu_support_list[i].vendor, BMC_FRU_ATTR_KEY_VALUE_SIZE)==0) &&
            (strncmp(fru->part_num.val, psu_support_list[i].part_number, BMC_FRU_ATTR_KEY_VALUE_SIZE)==0)) {
            *psu_type = psu_support_list[i].type;
            break;
        }
    }

    if (*psu_type == ONLP_PSU_TYPE_INVALID) {
        AIM_LOG_ERROR("[%s]unknown PSU type, vendor=%s, part_num=%s", __FUNCTION__, fru->vendor.val, fru->part_num.val);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Update the information of Model and Serial from PSU EEPROM
 * @param id The PSU Local ID
 * @param[out] info Receives the PSU information (model and serial).
 */
static int update_psui_fru_info(int id, onlp_psu_info_t* info)
{
    bmc_fru_t fru = {0};
    int psu_type = ONLP_PSU_TYPE_AC;

    //read fru data
    ONLP_TRY(bmc_fru_read(id, &fru));

    //update FRU model
    memset(info->model, 0, sizeof(info->model));
    if (strncmp(fru.vendor.val, "FSPGROUP", BMC_FRU_ATTR_KEY_VALUE_SIZE)==0) {
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
    ONLP_TRY(get_psu_type(id, &psu_type, &fru));
    if(psu_type == ONLP_PSU_TYPE_AC) {
        info->caps |= ONLP_PSU_CAPS_AC;
        info->caps &= ~ONLP_PSU_CAPS_DC48;
    } else if(psu_type == ONLP_PSU_TYPE_DC48){
        info->caps &= ~ONLP_PSU_CAPS_AC;
        info->caps |= ONLP_PSU_CAPS_DC48;
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
    int attr_id_vin = -1, attr_id_vout = -1;
    int attr_id_iin = -1, attr_id_iout = -1;
    int attr_id_svout = -1, attr_id_siout = -1;
    //int pw_present;

    if((info->status & ONLP_PSU_STATUS_PRESENT) == 0) {
        //not present, do nothing
        return ONLP_STATUS_OK;
    }

    if(local_id == ONLP_PSU0) {
        attr_id_vin   = BMC_ATTR_ID_PSU0_VIN;
        attr_id_vout  = BMC_ATTR_ID_PSU0_VOUT;
        attr_id_iin   = BMC_ATTR_ID_PSU0_IIN;
        attr_id_iout  = BMC_ATTR_ID_PSU0_IOUT;
        attr_id_svout = BMC_ATTR_ID_PSU0_STBVOUT;
        attr_id_siout = BMC_ATTR_ID_PSU0_STBIOUT;
    } else if(local_id == ONLP_PSU1) {
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
    info->caps |= ONLP_PSU_CAPS_VIN;

    /* Get power vout status */
    ONLP_TRY(bmc_sensor_read(attr_id_vout, PSU_SENSOR, &data));
    info->mvout = (int) (data*1000);
    info->caps |= ONLP_PSU_CAPS_VOUT;

    /* Get power iin status */
    ONLP_TRY(bmc_sensor_read(attr_id_iin, PSU_SENSOR, &data));
    info->miin = (int) (data*1000);
    info->caps |= ONLP_PSU_CAPS_IIN;

    /* Get power iout status */
    ONLP_TRY(bmc_sensor_read(attr_id_iout, PSU_SENSOR, &data));
    info->miout = (int) (data*1000);
    info->caps |= ONLP_PSU_CAPS_IOUT;

    /* Get standby power vout */
    ONLP_TRY(bmc_sensor_read(attr_id_svout, PSU_SENSOR, &data));
    stbmvout = (int) (data*1000);

    /* Get standby power iout */
    ONLP_TRY(bmc_sensor_read(attr_id_siout, PSU_SENSOR, &data));
    stbmiout = (int) (data*1000);

    /* Get power in and out */
    info->mpin = info->miin * info->mvin / 1000;
    info->mpout = (info->miout * info->mvout + stbmiout * stbmvout) / 1000;
    info->caps |= ONLP_PSU_CAPS_PIN | ONLP_PSU_CAPS_POUT;

    /* Update FRU (model/serial) */
    ONLP_TRY(update_psui_fru_info(local_id, info));


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
    *info = psu_info[local_id];
    ONLP_TRY(onlp_psui_hdr_get(id, &info->hdr));

    /* Update onlp_psu_info_t status */
    ONLP_TRY(onlp_psui_status_get(id, &info->status));

    if(local_id == ONLP_PSU0 || ONLP_PSU1) {
        ONLP_TRY(update_psui_info(local_id, info));
    } else {
        AIM_LOG_ERROR("unknown psu id (%d), func=%s\n", local_id, __FUNCTION__);
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
    if(psu_presence == 0) {
        *status |= ONLP_PSU_STATUS_UNPLUGGED;
    } else if(psu_presence == 1) {
        *status |= ONLP_PSU_STATUS_PRESENT;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    /* get psu power good status */
    ONLP_TRY(get_psui_pwgood_status(local_id, &psu_pwgood));
    if(psu_pwgood == 0) {
        *status |= ONLP_PSU_STATUS_FAILED;
    } else if(psu_pwgood == 1) {
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
    *hdr = psu_info[local_id].hdr;

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

