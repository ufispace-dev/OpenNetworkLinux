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
 * Fan Platform Implementation.
 *
 ***********************************************************/
#include <onlp/platformi/fani.h>
#include "platform_lib.h"

#define FAN_STATUS ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B
#define FAN_CAPS   ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE
#define SYS_FAN_RPM_MAX 16000
#define PSU_FAN_RPM_MAX 28500
#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_FAN(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

#define CHASSIS_INFO(id, des)                   \
    {                                           \
        { ONLP_FAN_ID_CREATE(id), des, POID_0}, \
        FAN_STATUS,                             \
        FAN_CAPS,                               \
        0,                                      \
        0,                                      \
        ONLP_FAN_MODE_INVALID,                  \
    }

/*
 * Get the fan information.
 */

onlp_fan_info_t fan_info[] = {
    { }, /* Not used */
    CHASSIS_INFO(ONLP_FAN_0    , "CHASSIS FAN 0"),
    CHASSIS_INFO(ONLP_FAN_1    , "CHASSIS FAN 1"),
    CHASSIS_INFO(ONLP_FAN_2    , "CHASSIS FAN 2"),
    CHASSIS_INFO(ONLP_FAN_3    , "CHASSIS FAN 3"),
    CHASSIS_INFO(ONLP_PSU_0_FAN, "PSU 0 FAN"),
    CHASSIS_INFO(ONLP_PSU_1_FAN, "PSU 1 FAN"),
};

static bool fan_fru_supported = false;

static int ufi_fan_fru_update(int local_id, onlp_fan_info_t* info)
{
    int result = ONLP_STATUS_OK;

    if(fan_fru_supported) {
        /* Get fan fru info */
        if(result != ONLP_STATUS_OK) {
            snprintf(info->model, sizeof(info->model), "%s", "not available");
            snprintf(info->serial, sizeof(info->serial), "%s", "not available");
        }
    } else {
        snprintf(info->model, sizeof(info->model), "%s", "not supported");
        snprintf(info->serial, sizeof(info->serial), "%s", "not supported");
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the fan information from BMC
 * @param info [out] The fan information
 * @param id [in] FAN ID
 */
static int ufi_bmc_fan_info_get(onlp_fan_info_t* info, int local_id)
{
    int rpm = 0, percentage = 0;
    float data = 0;
    int max_fan_speed = 0;

    //get fan rpm
    ONLP_TRY(bmc_sensor_read(local_id + BMC_ATTR_ID_FAN0_RPM - 1, FAN_SENSOR, &data));
    rpm = (int) data;

    //set rpm field
    info->rpm = rpm;

    if(local_id >= ONLP_FAN_0 && local_id <= ONLP_FAN_3) {
        max_fan_speed = SYS_FAN_RPM_MAX;
        percentage = (info->rpm*100)/max_fan_speed;
        info->percentage = percentage;
        info->status |= (rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;
    } else if(local_id >= ONLP_PSU_0_FAN && local_id <= ONLP_PSU_1_FAN) {
        max_fan_speed = PSU_FAN_RPM_MAX;
        percentage = (info->rpm*100)/max_fan_speed;
        info->percentage = percentage;
        info->status |= (rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;
    }

    return ONLP_STATUS_OK;
}

/**
  * @brief Initialize the fan platform subsystem.
  */
int onlp_fani_init(void)
{
    lock_init();
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the information structure for the given fan OID.
 * @param id The fan OID
 * @param rv [out] Receives the fan information.
 */
int onlp_fani_info_get(onlp_oid_t id, onlp_fan_info_t* rv)
{
    int local_id;
    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);
    *rv = fan_info[local_id];
    ONLP_TRY(onlp_fani_status_get(id, &rv->status));

    if((rv->status & ONLP_FAN_STATUS_PRESENT) == 0) {
        return ONLP_STATUS_OK;
    }

    switch(local_id) {
        case ONLP_FAN_0:
        case ONLP_FAN_1:
        case ONLP_FAN_2:
        case ONLP_FAN_3:
        case ONLP_PSU_0_FAN:
        case ONLP_PSU_1_FAN:
            ONLP_TRY(ufi_bmc_fan_info_get(rv, local_id));
            break;
        default:
            return ONLP_STATUS_E_INTERNAL;
    }

    //update fan fru info
    ONLP_TRY(ufi_fan_fru_update(local_id, rv));
    return ONLP_STATUS_OK;
}

/**
 * @brief Retrieve the fan's operational status.
 * @param id The fan OID.
 * @param rv [out] Receives the fan's operations status flags.
 * @notes Only operational state needs to be returned -
 *        PRESENT/FAILED
 */
int onlp_fani_status_get(onlp_oid_t id, uint32_t* rv)
{
    int local_id;
    int presence;
    float data;
    local_id = ONLP_OID_ID_GET(id);

    //check presence for fantray 1-4
    if(local_id >= ONLP_FAN_0 && local_id <= ONLP_FAN_3) {
        ONLP_TRY(bmc_sensor_read(local_id + BMC_ATTR_ID_FAN0_PSNT_L - 1, FAN_SENSOR, &data));
        presence = (int)data;
        if(presence == 1) {
            *rv |= ONLP_FAN_STATUS_PRESENT;
        } else {
            *rv &= ~ONLP_FAN_STATUS_PRESENT;
        }
    } else if(local_id == ONLP_PSU_0_FAN){
        ONLP_TRY(psu_present_get(&presence, ONLP_PSU_0));
        if(presence == 1) {
            *rv |= ONLP_FAN_STATUS_PRESENT;
        } else {
            *rv &= ~ONLP_FAN_STATUS_PRESENT;
        }
    } else if(local_id == ONLP_PSU_1_FAN){
        ONLP_TRY(psu_present_get(&presence, ONLP_PSU_1));
        if(presence == 1) {
            *rv |= ONLP_FAN_STATUS_PRESENT;
        } else {
            *rv &= ~ONLP_FAN_STATUS_PRESENT;
        }
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Retrieve the fan's OID hdr.
 * @param id The fan OID.
 * @param rv [out] Receives the OID header.
 */
int onlp_fani_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* hdr)
{
    int local_id;
    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);
    if(local_id >= ONLP_FAN_MAX) {
        return ONLP_STATUS_E_INVALID;
    } else {
        *hdr = fan_info[local_id].hdr;
    }
    return ONLP_STATUS_OK;
}

/**
 * @brief Set the fan speed in RPM.
 * @param id The fan OID
 * @param rpm The new RPM
 * @note This is only relevant if the RPM capability is set.
 */
int onlp_fani_rpm_set(onlp_oid_t id, int rpm)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Set the fan speed in percentage.
 * @param id The fan OID.
 * @param p The new fan speed percentage.
 * @note This is only relevant if the PERCENTAGE capability is set.
 */
int onlp_fani_percentage_set(onlp_oid_t id, int p)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Set the fan mode.
 * @param id The fan OID.
 * @param mode The new fan mode.
 */
int onlp_fani_mode_set(onlp_oid_t id, onlp_fan_mode_t mode)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Set the fan direction (if supported).
 * @param id The fan OID
 * @param dir The direction.
 */
int onlp_fani_dir_set(onlp_oid_t id, onlp_fan_dir_t dir)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Generic fan ioctl
 * @param id The fan OID
 * @param vargs The variable argument list for the ioctl call.
 * @param Optional
 */
int onlp_fani_ioctl(onlp_oid_t id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

