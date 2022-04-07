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
 * Fan Platform Implementation Defaults.
 *
 ***********************************************************/
#include <onlp/platformi/fani.h>
#include <onlp/platformi/psui.h>
#include <onlplib/file.h>
#include "platform_lib.h"

#define SYS_FAN_RPM_MAX 16000
#define PSU_FAN_RPM_MAX 28500

#define FAN_STATUS  ONLP_OID_STATUS_FLAG_PRESENT
#define FAN_DIR     ONLP_FAN_DIR_F2B
#define FAN_CAPS    (ONLP_FAN_CAPS_GET_DIR | ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM)

#define CHASSIS_FAN_INFO(idx, des)              \
    {                                           \
        .hdr = {                                \
            .id = ONLP_FAN_ID_CREATE(idx),      \
            .description = des,                 \
            .poid = ONLP_OID_CHASSIS,           \
            .status = FAN_STATUS,               \
        },                                      \
        .dir = FAN_DIR,                         \
        .caps = FAN_CAPS,                       \
        .model = "",                            \
        .serial = "",                           \
    }

/**
 * Get all information about the given FAN oid.
 *
 * [01] CHASSIS
 *            |----[01] ONLP_FAN_0
 *            |----[02] ONLP_FAN_1
 *            |----[03] ONLP_FAN_2
 *            |----[04] ONLP_FAN_3
 *            |
 *            |----[01] ONLP_PSU_0----[05] ONLP_PSU_0_FAN
 *            |
 *            |----[02] ONLP_PSU_1----[06] ONLP_PSU_1_FAN
 */
static onlp_fan_info_t __onlp_fan_info[ONLP_FAN_COUNT] = {
    { }, /* Not used */
    CHASSIS_FAN_INFO(ONLP_FAN_0    , "Chassis Fan - 0"),
    CHASSIS_FAN_INFO(ONLP_FAN_1    , "Chassis Fan - 1"),
    CHASSIS_FAN_INFO(ONLP_FAN_2    , "Chassis Fan - 2"),
    CHASSIS_FAN_INFO(ONLP_FAN_3    , "Chassis Fan - 3"),
    CHASSIS_FAN_INFO(ONLP_PSU_0_FAN, "PSU-0-Fan"),
    CHASSIS_FAN_INFO(ONLP_PSU_1_FAN, "PSU-1-Fan"),
};

/**
 * @brief Update the status of FAN's oid header.
 * @param id The FAN ID.
 * @param[out] hdr Receives the header.
 */
static int update_fani_status(int local_id, onlp_oid_hdr_t* hdr)
{
    int presence;
    float data = 0;
    int bmc_attr_id = BMC_ATTR_ID_MAX;

    /* clear FAN status */
    hdr->status = 0;

    //check presence for fantray 1-4
    if(local_id >= ONLP_FAN_0 && local_id <= ONLP_FAN_3) {
        bmc_attr_id = local_id + BMC_ATTR_ID_FAN0_PSNT_L - 1;
        ONLP_TRY(bmc_sensor_read(bmc_attr_id, FAN_SENSOR, &data));
        presence = (int)data;
        if(presence == 1) {
            ONLP_OID_STATUS_FLAG_SET(hdr, PRESENT);
        } else {
            ONLP_OID_STATUS_FLAG_SET(hdr, UNPLUGGED);
        }
    } else if(local_id == ONLP_PSU_0_FAN){
        ONLP_TRY(psu_present_get(&presence, ONLP_PSU_0));
        if(presence == 1) {
            ONLP_OID_STATUS_FLAG_SET(hdr, PRESENT);
        } else {
            ONLP_OID_STATUS_FLAG_SET(hdr, UNPLUGGED);
        }
    } else if(local_id == ONLP_PSU_1_FAN){
        ONLP_TRY(psu_present_get(&presence, ONLP_PSU_1));
        if(presence == 1) {
            ONLP_OID_STATUS_FLAG_SET(hdr, PRESENT);
        } else {
            ONLP_OID_STATUS_FLAG_SET(hdr, UNPLUGGED);
        }
    } else {
        AIM_LOG_ERROR("unknown fan id (%d), func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Update the information structure for the given FAN
 * @param id The FAN Local ID
 * @param[out] info Receives the FAN information.
 */
static int update_fani_info(int local_id, onlp_fan_info_t* info)
{
    int rpm = 0, percentage = 0;
    float data = 0;
    int max_fan_speed = 0;

    if ((info->hdr.status & ONLP_OID_STATUS_FLAG_PRESENT) == 0) {
        //not present, do nothing
        return ONLP_STATUS_OK;
    }

    //get fan rpm
    ONLP_TRY(bmc_sensor_read(local_id + BMC_ATTR_ID_FAN0_RPM - 1, FAN_SENSOR, &data));
    rpm = (int) data;

    //set rpm field
    info->rpm = rpm;

    if(local_id >= ONLP_FAN_0 && local_id <= ONLP_FAN_3) {
        max_fan_speed = SYS_FAN_RPM_MAX;
    } else if(local_id >= ONLP_PSU_0_FAN && local_id <= ONLP_PSU_1_FAN) {
        max_fan_speed = PSU_FAN_RPM_MAX;
    }

    percentage = (info->rpm*100)/max_fan_speed;
    info->percentage = percentage;
    info->hdr.status |= (rpm == 0) ? ONLP_OID_STATUS_FLAG_FAILED : ONLP_OID_STATUS_FLAG_OPERATIONAL;

    return ONLP_STATUS_OK;
}

/**
 * @brief Software initialization of the Fan module.
 */
int onlp_fani_sw_init(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Hardware initialization of the Fan module.
 * @param flags The hardware initialization flags.
 */
int onlp_fani_hw_init(uint32_t flags)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Deinitialize the fan software module.
 * @note The primary purpose of this API is to properly
 * deallocate any resources used by the module in order
 * faciliate detection of real resouce leaks.
 */
int onlp_fani_sw_denit(void)
{
    return ONLP_STATUS_OK;
}


/**
 * @brief Validate a fan id.
 * @param id The fan id.
 */
int onlp_fani_id_validate(onlp_oid_id_t id)
{
    return ONLP_OID_ID_VALIDATE_RANGE(id, 1, ONLP_FAN_MAX-1);
}

/**
 * @brief Retrieve the fan's OID hdr.
 * @param id The fan id.
 * @param[out] hdr Receives the OID header.
 */
int onlp_fani_hdr_get(onlp_oid_id_t id, onlp_oid_hdr_t* hdr)
{
    int ret = ONLP_STATUS_OK;
    int local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_fan_info_t */
    *hdr = __onlp_fan_info[local_id].hdr;

    /* Update onlp_oid_hdr_t */
    ret = update_fani_status(local_id, hdr);

    return ret;
}

/**
 * @brief Get the information structure for the given fan OID.
 * @param id The fan id
 * @param[out] info Receives the fan information.
 */
int onlp_fani_info_get(onlp_oid_id_t id, onlp_fan_info_t* info)
{
    int ret = ONLP_STATUS_OK;
    int local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_fan_info_t */
    memset(info, 0, sizeof(onlp_fan_info_t));
    *info = __onlp_fan_info[local_id];
    ONLP_TRY(onlp_fani_hdr_get(id, &info->hdr));

    if (local_id > ONLP_FAN_RESERVED && local_id < ONLP_FAN_MAX) {
        ret = update_fani_info(local_id, info);
    } else {
        AIM_LOG_ERROR("unknown FAN id (%d), func=%s\n", local_id, __FUNCTION__);
        ret = ONLP_STATUS_E_PARAM;
    }

    return ret;
}

/**
 * @brief Get the fan capabilities.
 * @param id The fan id.
 * @param[out] rv The fan capabilities
 */
int onlp_fani_caps_get(onlp_oid_id_t id, uint32_t* rv)
{
    int local_id = ONLP_OID_ID_GET(id);

    *rv = __onlp_fan_info[local_id].caps;

    return ONLP_STATUS_OK;
}

/**
 * @brief Set the fan speed in RPM.
 * @param id The fan OID
 * @param rpm The new RPM
 * @note This is only relevant if the RPM capability is set.
 */
int onlp_fani_rpm_set(onlp_oid_id_t id, int rpm)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Set the fan speed in percentage.
 * @param id The fan OID.
 * @param p The new fan speed percentage.
 * @note This is only relevant if the PERCENTAGE capability is set.
 */
int onlp_fani_percentage_set(onlp_oid_id_t id, int p)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Set the fan direction (if supported).
 * @param id The fan OID
 * @param dir The direction.
 */
int onlp_fani_dir_set(onlp_oid_id_t id, onlp_fan_dir_t dir)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}
