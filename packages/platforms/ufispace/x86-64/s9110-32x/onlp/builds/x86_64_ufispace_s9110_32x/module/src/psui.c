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

#define PSU_STATUS_PWR_FAIL    0
#define PSU_STATUS_PWR_GD      1

#define PSU_INFO(id, desc, fid, tid)            \
    {                                           \
        { ONLP_PSU_ID_CREATE(id), desc, POID_0,\
            {                                   \
                ONLP_FAN_ID_CREATE(fid),        \
                ONLP_THERMAL_ID_CREATE(tid),    \
            }                                   \
        }                                       \
    }

static onlp_psu_info_t psu_info[] =
{
    { }, /* Not used */
    PSU_INFO(ONLP_PSU_0, "PSU-0", ONLP_PSU_0_FAN, ONLP_THERMAL_PSU_0),
    PSU_INFO(ONLP_PSU_1, "PSU-1", ONLP_PSU_1_FAN, ONLP_THERMAL_PSU_1),
};

static psu_support_info_t psu_support_list[] =
{
    {"FSPGROUP", "YNEE0750EM", ONLP_PSU_TYPE_AC},
    {"FSPGROUP", "YNEE0750BM", ONLP_PSU_TYPE_AC},
    {"FSPGROUP", "YNEE0750AM", ONLP_PSU_TYPE_DC48},
};
typedef struct
{
    int cpld_attr;
    int abs;
    int pwrgd;
} psu_attr_t;

typedef enum cpld_attr_idx_e {
    CPLD_PSU = 0,
} cpld_attr_idx_t;

static const psu_attr_t psu_attr[] = {
    /*              attr      abs  pwrgd */
    [ONLP_PSU_0] = {CPLD_PSU, 0   , 4},
    [ONLP_PSU_1] = {CPLD_PSU, 1   , 5},
};

static int get_psu_sysfs(cpld_attr_idx_t idx, char** str)
{
    if(str == NULL)
        return ONLP_STATUS_E_PARAM;

    switch(idx) {
        case CPLD_PSU:
           *str =  SYSFS_CPLD1 "cpld_psu_status";
           break;
        default:
            *str = "";
            return ONLP_STATUS_E_PARAM;
    }
    return ONLP_STATUS_OK;
}

/**
 * @brief Get and check psu local ID
 * @param id [in] OID
 * @param local_id [out] The psu local id
 */
static int get_psu_local_id(int id, int *local_id)
{
    int tmp_id;

    if(local_id == NULL) {
        return ONLP_STATUS_E_PARAM;
    }

    if(!ONLP_OID_IS_PSU(id)) {
        return ONLP_STATUS_E_INVALID;
    }

    tmp_id = ONLP_OID_ID_GET(id);
    switch (tmp_id) {
        case ONLP_PSU_0:
        case ONLP_PSU_1:
            *local_id = tmp_id;
            return ONLP_STATUS_OK;
        default:
            return ONLP_STATUS_E_INVALID;
    }

    return ONLP_STATUS_E_INVALID;
}

int get_psu_present_status(int local_id, int *pw_present)
{
    char *sysfs = NULL;
    int status;

    if(pw_present == NULL) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (local_id != ONLP_PSU_0 && local_id != ONLP_PSU_1) {
        return ONLP_STATUS_E_INTERNAL;
    }

    ONLP_TRY(get_psu_sysfs(psu_attr[local_id].cpld_attr, &sysfs));
    ONLP_TRY(read_file_hex(&status, sysfs));

    *pw_present = ((get_bit_value(status,psu_attr[local_id].abs))? PSU_STATUS_ABS : PSU_STATUS_PRES);

    return ONLP_STATUS_OK;
}

static int get_psu_pwgood_status(int local_id, int *pw_good)
{
    char *sysfs = NULL;
    int status;

    if(pw_good == NULL) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (local_id != ONLP_PSU_0 && local_id != ONLP_PSU_1) {
        return ONLP_STATUS_E_INTERNAL;
    }

    ONLP_TRY(get_psu_sysfs(psu_attr[local_id].cpld_attr, &sysfs));
    ONLP_TRY(read_file_hex(&status, sysfs));

    *pw_good = ((get_bit_value(status,psu_attr[local_id].pwrgd))? PSU_STATUS_PWR_GD : PSU_STATUS_PWR_FAIL);

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
        ONLP_TRY(read_bmc_fru(local_id, fru));
    } else {
        fru = fru_in;
    }

    *psu_type = ONLP_PSU_TYPE_INVALID;
    max = sizeof(psu_support_list)/sizeof(*psu_support_list);
    for (i=0; i < max; ++i) {
        if ((strncmp(fru->vendor.val, psu_support_list[i].vendor, BMC_FRU_ATTR_KEY_VALUE_SIZE)==0) &&
            (strncmp(fru->name.val, psu_support_list[i].name, BMC_FRU_ATTR_KEY_VALUE_SIZE)==0)) {
            *psu_type = psu_support_list[i].type;
            break;
        }
    }

    if (*psu_type == ONLP_PSU_TYPE_INVALID) {
        AIM_LOG_ERROR("[%s]unknown PSU type, vendor=%s, part_num=%s", __FUNCTION__, fru->vendor.val, fru->name.val);
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Update the information of Model and Serial from PSU EEPROM
 * @param local_id The PSU Local ID
 * @param[out] info Receives the PSU information (model and serial).
 */
static int update_psui_fru_info(int local_id, onlp_psu_info_t* info)
{
    bmc_fru_t fru = {0};
    int psu_type = ONLP_PSU_TYPE_AC;

    //read fru data
    ONLP_TRY(read_bmc_fru(local_id, &fru));

    //update FRU model
    memset(info->model, 0, sizeof(info->model));
    snprintf(info->model, sizeof(info->model), "%s", fru.name.val);

    //update FRU serial
    memset(info->serial, 0, sizeof(info->serial));
    snprintf(info->serial, sizeof(info->serial), "%s", fru.serial.val);

    //update FRU type
    ONLP_TRY(get_psu_type(local_id, &psu_type, &fru));

    if(psu_type == ONLP_PSU_TYPE_AC) {
        info->caps |= ONLP_PSU_CAPS_AC;
        info->caps &= ~ONLP_PSU_CAPS_DC48;
    } else if(psu_type == ONLP_PSU_TYPE_DC48){
        info->caps &= ~ONLP_PSU_CAPS_AC;
        info->caps |= ONLP_PSU_CAPS_DC48;
    }
    return ONLP_STATUS_OK;
}

static int update_psui_info(int local_id, onlp_psu_info_t *info)
{
    int pw_present, pw_good;
    int stbmvout = 0, stbmiout = 0;
    float data;
    int attr_vin, attr_vout, attr_iin, attr_iout, attr_stbvout, attr_stbiout;

    *info = psu_info[local_id];
    ONLP_TRY(get_psu_present_status(local_id, &pw_present));

    if (pw_present != PSU_STATUS_PRES) {
        info->status &= ~ONLP_PSU_STATUS_PRESENT;
        info->status |=  ONLP_PSU_STATUS_UNPLUGGED;
    } else {
        info->status |= ONLP_PSU_STATUS_PRESENT;
    }

    if(info->status & ONLP_PSU_STATUS_PRESENT) {
        /* Get power good status */
        ONLP_TRY(get_psu_pwgood_status(local_id, &pw_good));

        if (pw_good != PSU_STATUS_PWR_GD) {
            info->status |= ONLP_PSU_STATUS_FAILED;
        } else {
            info->status &= ~ONLP_PSU_STATUS_FAILED;
        }

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
        ONLP_TRY(read_bmc_sensor(attr_vin , PSU_SENSOR, &data));
        if(BMC_ATTR_INVALID_VAL != (int)(data)) {
            info->mvin = (int) (data*1000);
            info->caps |= ONLP_PSU_CAPS_VIN;
        }

        /* Get power vout status */
        ONLP_TRY(read_bmc_sensor(attr_vout, PSU_SENSOR, &data));
        if(BMC_ATTR_INVALID_VAL != (int)(data)) {
            info->mvout = (int) (data*1000);
            info->caps |= ONLP_PSU_CAPS_VOUT;
        }

        /* Get power iin status */
        ONLP_TRY(read_bmc_sensor(attr_iin, PSU_SENSOR, &data));
        if(BMC_ATTR_INVALID_VAL != (int)(data)) {
            info->miin = (int) (data*1000);
            info->caps |= ONLP_PSU_CAPS_IIN;
        }

        /* Get power iout status */
        ONLP_TRY(read_bmc_sensor(attr_iout, PSU_SENSOR, &data));
        if(BMC_ATTR_INVALID_VAL != (int)(data)) {
            info->miout = (int) (data*1000);
            info->caps |= ONLP_PSU_CAPS_IOUT;
        }

        /* Get standby power vout */
        ONLP_TRY(read_bmc_sensor(attr_stbvout, PSU_SENSOR, &data));
        if(BMC_ATTR_INVALID_VAL != (int)(data)) {
            stbmvout = (int) (data*1000);
        }

        /* Get standby power iout */
        ONLP_TRY(read_bmc_sensor(attr_stbiout, PSU_SENSOR, &data));
        if(BMC_ATTR_INVALID_VAL != (int)(data)) {
            stbmiout = (int) (data*1000);
        }

        /* Get power in and out */
        info->mpin = info->miin * info->mvin / 1000;
        info->mpout = (info->miout * info->mvout + stbmiout * stbmvout) / 1000;
        info->caps |= ONLP_PSU_CAPS_PIN | ONLP_PSU_CAPS_POUT;

        /* Get FRU (model/serial) */
        ONLP_TRY(update_psui_fru_info(local_id, info));
    }
    return ONLP_STATUS_OK;
}

/**
 * @brief Initialize the PSU subsystem.
 */
int onlp_psui_init(void)
{
    init_lock();
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the information structure for the given PSU
 * @param id The PSU OID
 * @param rv [out] Receives the PSU information.
 */
int onlp_psui_info_get(onlp_oid_t id, onlp_psu_info_t* rv)
{
    int local_id;

    ONLP_TRY(get_psu_local_id(id, &local_id));
    ONLP_TRY(update_psui_info(local_id, rv));

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the PSU's operational status.
 * @param id The PSU OID.
 * @param rv [out] Receives the operational status.
 */
int onlp_psui_status_get(onlp_oid_t id, uint32_t* rv)
{
    int local_id;
    onlp_psu_info_t info ={0};

    ONLP_TRY(get_psu_local_id(id, &local_id));
    ONLP_TRY(update_psui_info(local_id, &info));
    *rv = info.status;

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the PSU's oid header.
 * @param id The PSU OID.
 * @param rv [out] Receives the header.
 */
int onlp_psui_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* rv)
{
    int local_id;

    ONLP_TRY(get_psu_local_id(id, &local_id));
    *rv = psu_info[local_id].hdr;

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

