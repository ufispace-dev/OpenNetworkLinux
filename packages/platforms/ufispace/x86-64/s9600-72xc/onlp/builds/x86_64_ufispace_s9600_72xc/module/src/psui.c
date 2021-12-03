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

#define PSU0_PRESENT_MASK       0b00000001
#define PSU1_PRESENT_MASK       0b00000010
#define PSU0_PWGOOD_MASK        0b00010000
#define PSU1_PWGOOD_MASK        0b00100000
#define PSU_STATUS_PRESENT      1
#define PSU_STATUS_POWER_GOOD   1
/* SYSFS */
#define MB_CPLD_PSU_STS_ATTR    "cpld_psu_status"
#define SYSFS_PSU_STATUS        MB_CPLD1_SYSFS_PATH"/"MB_CPLD_PSU_STS_ATTR

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

static onlp_psu_info_t psu_info[] =
{
    { }, /* Not used */
    PSU_INFO(ONLP_PSU_0, "PSU-0", ONLP_PSU_0_FAN, ONLP_THERMAL_PSU0),
    PSU_INFO(ONLP_PSU_1, "PSU-1", ONLP_PSU_1_FAN, ONLP_THERMAL_PSU1),
};

int psu_present_get(int *pw_present, int local_id)
{
    int status;
    int mask;

    if(pw_present == NULL) {
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

    *pw_present = ((status & mask)? 0 : 1);

    return ONLP_STATUS_OK;
}

int psu_pwgood_get(int *pw_good, int local_id)
{
    int status;
    int mask;

    if(pw_good == NULL) {
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

    *pw_good = ((status & mask)? 1 : 0);

    return ONLP_STATUS_OK;
}

static int ufi_psu_status_info_get(int local_id, onlp_psu_info_t *info)
{
    int stbmvout, stbmiout;
    float data;
    int index_offset = 0;

    if(local_id == ONLP_PSU_1) {
        index_offset = 6;
    }

    /* Get power vin status */
    ONLP_TRY(bmc_sensor_read(ID_PSU0_VIN + index_offset, PSU_SENSOR, &data));
    info->mvin = (int) (data*1000);
    info->caps |= ONLP_PSU_CAPS_VIN;

    /* Get power vout status */
    ONLP_TRY(bmc_sensor_read(ID_PSU0_VOUT + index_offset, PSU_SENSOR, &data));
    info->mvout = (int) (data*1000);
    info->caps |= ONLP_PSU_CAPS_VOUT;

    /* Get power iin status */
    ONLP_TRY(bmc_sensor_read(ID_PSU0_IIN + index_offset, PSU_SENSOR, &data));
    info->miin = (int) (data*1000);
    info->caps |= ONLP_PSU_CAPS_IIN;

    /* Get power iout status */
    ONLP_TRY(bmc_sensor_read(ID_PSU0_IOUT + index_offset, PSU_SENSOR, &data));
    info->miout = (int) (data*1000);
    info->caps |= ONLP_PSU_CAPS_IOUT;

    /* Get standby power vout */
    ONLP_TRY(bmc_sensor_read(ID_PSU0_STBVOUT + index_offset, PSU_SENSOR, &data));
    stbmvout = (int) (data*1000);

    /* Get standby power iout */
    ONLP_TRY(bmc_sensor_read(ID_PSU0_STBIOUT + index_offset, PSU_SENSOR, &data));
    stbmiout = (int) (data*1000);

    /* Get power in and out */
    info->mpin = info->miin * info->mvin / 1000;
    info->mpout = (info->miout * info->mvout + stbmiout * stbmvout) / 1000;
    info->caps |= ONLP_PSU_CAPS_PIN | ONLP_PSU_CAPS_POUT;

    /* Get FRU (model/serial) */
    ONLP_TRY(bmc_fru_read(info, local_id));

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
 * @param rv [out] Receives the PSU information.
 */
int onlp_psui_info_get(onlp_oid_t id, onlp_psu_info_t* rv)
{
    int local_id;
    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);
    memset(rv, 0, sizeof(onlp_psu_info_t));

    *rv = psu_info[local_id];
    /* update status */
    ONLP_TRY(onlp_psui_status_get(id, &rv->status));

    if((rv->status & ONLP_PSU_STATUS_PRESENT) == 0) {
        return ONLP_STATUS_OK;
    }

    switch(local_id) {
        case ONLP_PSU_0:
        case ONLP_PSU_1:
            return ufi_psu_status_info_get(local_id, rv);
            break;
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
            break;
    }

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
    int pw_present, pw_good;
    VALIDATE(id);

     /* Get power present status */
    local_id = ONLP_OID_ID_GET(id);
    ONLP_TRY(psu_present_get(&pw_present, local_id));

    if(pw_present != PSU_STATUS_PRESENT) {
        *rv &= ~ONLP_PSU_STATUS_PRESENT;
        *rv |=  ONLP_PSU_STATUS_UNPLUGGED;
    } else {
        *rv |= ONLP_PSU_STATUS_PRESENT;
    }

    /* Get power good status */
    ONLP_TRY(psu_pwgood_get(&pw_good, local_id));

    if(pw_good != PSU_STATUS_POWER_GOOD) {
        *rv |= ONLP_PSU_STATUS_FAILED;
    } else {
        *rv &= ~ONLP_PSU_STATUS_FAILED;
    }

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
    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);
    if(local_id > ONLP_PSU_1) {
        return ONLP_STATUS_E_INVALID;
    } else {
        *rv = psu_info[local_id].hdr;
    }
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

