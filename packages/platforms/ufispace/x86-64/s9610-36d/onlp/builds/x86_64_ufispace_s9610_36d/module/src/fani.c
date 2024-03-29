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
#define SYS_FAN_F_RPM_MAX 12000
#define SYS_FAN_R_RPM_MAX 11300
#define PSU_FAN_RPM_MAX_AC 25000
#define PSU_FAN_RPM_MAX_DC 32500

#define VALIDATE(_id)                      \
    do {                                   \
        if(!ONLP_OID_IS_FAN(_id)) {        \
            return ONLP_STATUS_E_INVALID;  \
        }                                  \
    } while(0)

#define CHASSIS_FAN_INFO(id, idx, fan_loc) \
    {                                      \
        { ONLP_FAN_ID_CREATE(id), "CHASSIS FAN "#idx" "#fan_loc, POID_0},\
        FAN_STATUS,                        \
        FAN_CAPS,                          \
        0,                                 \
        0,                                 \
        ONLP_FAN_MODE_INVALID,             \
        COMM_STR_NOT_SUPPORTED,            \
        COMM_STR_NOT_SUPPORTED,            \
    }

#define PSU_FAN_INFO(id, pid)              \
    {                                      \
        { ONLP_FAN_ID_CREATE(id), "PSU "#pid" FAN", POID_0},\
        FAN_STATUS,                        \
        FAN_CAPS,                          \
        0,                                 \
        0,                                 \
        ONLP_FAN_MODE_INVALID,             \
        COMM_STR_NOT_SUPPORTED,            \
        COMM_STR_NOT_SUPPORTED,            \
    }

/*
 * Get the fan information.
 */

onlp_fan_info_t fan_info[] = {
    { }, /* Not used */
    CHASSIS_FAN_INFO(ONLP_FAN_F_0, 0, FRONT),
    CHASSIS_FAN_INFO(ONLP_FAN_R_0, 0, REAR),
    CHASSIS_FAN_INFO(ONLP_FAN_F_1, 1, FRONT),
    CHASSIS_FAN_INFO(ONLP_FAN_R_1, 1, REAR),
    CHASSIS_FAN_INFO(ONLP_FAN_F_2, 2, FRONT),
    CHASSIS_FAN_INFO(ONLP_FAN_R_2, 2, REAR),
    CHASSIS_FAN_INFO(ONLP_FAN_F_3, 3, FRONT),
    CHASSIS_FAN_INFO(ONLP_FAN_R_3, 3, REAR),
    PSU_FAN_INFO(ONLP_PSU_0_FAN, 0),
    PSU_FAN_INFO(ONLP_PSU_1_FAN, 1),
};

/**
 * @brief Get the fan information from BMC
 * @param info [out] The fan information
 * @param id [in] FAN ID
 */
static int ufi_bmc_fan_info_get(onlp_fan_info_t* info, int id)
{
    int rpm = 0, percentage = 0;
    int presence = 0;
    float data = 0;
    int sys_fan_max = 0;
    int sys_fan_f_max = SYS_FAN_F_RPM_MAX;
    int sys_fan_r_max = SYS_FAN_R_RPM_MAX;
    int psu_fan_max = 0;
    int psu_type = 0;
    int bmc_attr_id = BMC_ATTR_ID_MAX;
    int fan_present_id = BMC_ATTR_ID_MAX;

    switch(id)
    {
        case ONLP_FAN_F_0:
            bmc_attr_id = BMC_ATTR_ID_FAN0_RPM_FRONT;
            fan_present_id = BMC_ATTR_ID_FAN0_PRSNT_H;
            break;
        case ONLP_FAN_R_0:
            bmc_attr_id = BMC_ATTR_ID_FAN0_RPM_REAR;
            fan_present_id = BMC_ATTR_ID_FAN0_PRSNT_H;
            break;
        case ONLP_FAN_F_1:
            bmc_attr_id = BMC_ATTR_ID_FAN1_RPM_FRONT;
            fan_present_id = BMC_ATTR_ID_FAN1_PRSNT_H;
            break;
        case ONLP_FAN_R_1:
            bmc_attr_id = BMC_ATTR_ID_FAN1_RPM_REAR;
            fan_present_id = BMC_ATTR_ID_FAN1_PRSNT_H;
            break;
        case ONLP_FAN_F_2:
            bmc_attr_id = BMC_ATTR_ID_FAN2_RPM_FRONT;
            fan_present_id = BMC_ATTR_ID_FAN2_PRSNT_H;
            break;
        case ONLP_FAN_R_2:
            bmc_attr_id = BMC_ATTR_ID_FAN2_RPM_REAR;
            fan_present_id = BMC_ATTR_ID_FAN2_PRSNT_H;
            break;
        case ONLP_FAN_F_3:
            bmc_attr_id = BMC_ATTR_ID_FAN3_RPM_FRONT;
            fan_present_id = BMC_ATTR_ID_FAN3_PRSNT_H;
            break;
        case ONLP_FAN_R_3:
            bmc_attr_id = BMC_ATTR_ID_FAN3_RPM_REAR;
            fan_present_id = BMC_ATTR_ID_FAN3_PRSNT_H;
            break;
        case ONLP_PSU_0_FAN:
            bmc_attr_id = BMC_ATTR_ID_PSU0_FAN;
            break;
        case ONLP_PSU_1_FAN:
            bmc_attr_id = BMC_ATTR_ID_PSU1_FAN;
            break;
        default:
            bmc_attr_id = BMC_ATTR_ID_MAX;
    }

    if(bmc_attr_id == BMC_ATTR_ID_MAX) {
        return ONLP_STATUS_E_PARAM;
    }

    //check presence for fantray 1-4
    if (id >= ONLP_FAN_F_0 && id <= ONLP_FAN_R_3) {
        if(fan_present_id == BMC_ATTR_ID_MAX) {
            return ONLP_STATUS_E_PARAM;
        }

        ONLP_TRY(bmc_sensor_read(fan_present_id, FAN_SENSOR, &data));
        presence = (int) data;

        if(presence == 1) {
            info->status |= ONLP_FAN_STATUS_PRESENT;
        } else {
            info->status &= ~ONLP_FAN_STATUS_PRESENT;
            return ONLP_STATUS_OK;
        }
    }

    //get fan rpm
    ONLP_TRY(bmc_sensor_read(bmc_attr_id, FAN_SENSOR, &data));

    rpm = (int) data;

    //set rpm field
    info->rpm = rpm;

    if (id >= ONLP_FAN_F_0 && id <= ONLP_FAN_R_3) {
        if (id%2 == 1) {
            sys_fan_max = sys_fan_f_max;
        } else {
            sys_fan_max = sys_fan_r_max;
        }
        percentage = (info->rpm*100)/sys_fan_max;
        if (percentage > 100)
            percentage = 100;
        info->percentage = percentage;
        info->status |= (rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;
    } else if (id >= ONLP_PSU_0_FAN && id <= ONLP_PSU_1_FAN) {
        //get psu type
        if(id == ONLP_PSU_0_FAN) {
            ONLP_TRY(get_psu_type(ONLP_PSU_0, &psu_type, NULL));
        } else if(id == ONLP_PSU_1_FAN) {
            ONLP_TRY(get_psu_type(ONLP_PSU_1, &psu_type, NULL));
        } else {
            AIM_LOG_ERROR("unknown fan id (%d), func=%s", id, __FUNCTION__);
            return ONLP_STATUS_E_PARAM;
        }

        //AC/DC has different MAX RPM
        if (psu_type==ONLP_PSU_TYPE_AC) {
            psu_fan_max = PSU_FAN_RPM_MAX_AC;
        } else if (psu_type==ONLP_PSU_TYPE_DC12 || psu_type==ONLP_PSU_TYPE_DC48) {
            psu_fan_max = PSU_FAN_RPM_MAX_DC;
        } else {
            AIM_LOG_ERROR("unknown psu_type from file %s, id=%d, psu_type=%d", TMP_PSU_TYPE, id-ONLP_PSU_0_FAN+1, psu_type);
            return ONLP_STATUS_E_INTERNAL;
        }

        percentage = (info->rpm*100)/psu_fan_max;
        if (percentage > 100)
            percentage = 100;
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
    int fan_id = 0, rc = 0;
    VALIDATE(id);

    fan_id = ONLP_OID_ID_GET(id);
    *rv = fan_info[fan_id];

    switch (fan_id) {
        case ONLP_FAN_F_0 ... ONLP_FAN_R_3:
        case ONLP_PSU_0_FAN ... ONLP_PSU_1_FAN:
            rc = ufi_bmc_fan_info_get(rv, fan_id);
            break;
        default:
            return ONLP_STATUS_E_INTERNAL;
            break;
    }

    return rc;
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
    int result = ONLP_STATUS_OK;
    onlp_fan_info_t info;

    result = onlp_fani_info_get(id, &info);
    *rv = info.status;

    return result;
}

/**
 * @brief Retrieve the fan's OID hdr.
 * @param id The fan OID.
 * @param rv [out] Receives the OID header.
 */
int onlp_fani_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* hdr)
{
    int result = ONLP_STATUS_OK;
    onlp_fan_info_t* info;
    int fan_id = 0;
    VALIDATE(id);

    fan_id = ONLP_OID_ID_GET(id);
    if(fan_id >= ONLP_FAN_MAX) {
        result = ONLP_STATUS_E_INVALID;
    } else {
        info = &fan_info[fan_id];
        *hdr = info->hdr;
    }
    return result;
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

