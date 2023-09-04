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

extern int ufi_psu_present_get(int id, int *psu_present);

#define FAN_STATUS ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B
#define FAN_CAPS   ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE
#define SYS_FAN_RPM_MAX    (25000)
#define PSU_FAN_RPM_MAX_AC (20000)
#define PSU_FAN_RPM_MAX_DC (18000)

#define CHASSIS_FAN_INFO(id, idx)                               \
    {                                                           \
        { ONLP_FAN_ID_CREATE(id), "Chassis Fan - "#idx, POID_0},\
        FAN_STATUS,                                             \
        FAN_CAPS,                                               \
        0,                                                      \
        0,                                                      \
        ONLP_FAN_MODE_INVALID,                                  \
    }

#define PSU_FAN_INFO(id, pid, poid)                         \
    {                                                       \
        { ONLP_FAN_ID_CREATE(id), "PSU "#pid" - Fan", POID_0},\
        FAN_STATUS,                                         \
        FAN_CAPS,                                           \
        0,                                                  \
        0,                                                  \
        ONLP_FAN_MODE_INVALID,                              \
    }

/*
 * Get the fan information.
 */
static onlp_fan_info_t fan_info[] = {
    { }, /* Not used */
    CHASSIS_FAN_INFO(ONLP_FAN_0, 0),
    CHASSIS_FAN_INFO(ONLP_FAN_1, 1),
    CHASSIS_FAN_INFO(ONLP_FAN_2, 2),
    CHASSIS_FAN_INFO(ONLP_FAN_3, 3),
    CHASSIS_FAN_INFO(ONLP_FAN_4, 4),
    PSU_FAN_INFO(ONLP_PSU_0_FAN, 0, ONLP_PSU_0),
    PSU_FAN_INFO(ONLP_PSU_1_FAN, 1, ONLP_PSU_1),
};

/**
 * @brief Get and check fan local ID
 * @param id [in] OID
 * @param local_id [out] The fan local id
 */
static int get_fan_local_id(int id, int *local_id)
{
    int tmp_id;

    if(local_id == NULL) {
        return ONLP_STATUS_E_PARAM;
    }

    if(!ONLP_OID_IS_FAN(id)) {
        return ONLP_STATUS_E_INVALID;
    }

    tmp_id = ONLP_OID_ID_GET(id);
    switch (tmp_id) {
        case ONLP_FAN_0:
        case ONLP_FAN_1:
        case ONLP_FAN_2:
        case ONLP_FAN_3:
        case ONLP_FAN_4:
        case ONLP_PSU_0_FAN:
        case ONLP_PSU_1_FAN:
            *local_id = tmp_id;
            return ONLP_STATUS_OK;
        default:
            return ONLP_STATUS_E_INVALID;
    }

    return ONLP_STATUS_E_INVALID;
}

/**
 * @brief Get the fan information from BMC
 * @param id [in] FAN ID
 * @param info [out] The fan information
 */
static int ufi_bmc_fan_info_get(int local_id, onlp_fan_info_t* info)
{
    int rpm = 0, percentage = 0;
    int presence = 0;
    float data = 0;
    int sys_fan_max = SYS_FAN_RPM_MAX;
    int psu_fan_max = 0;
    int psu_type = 0;
    int psu_id = 0;
    psu_support_info_t psu_support_info = {0};
    int bmc_attr_id = BMC_ATTR_ID_MAX;
    int fan_present_id = BMC_ATTR_ID_MAX;
    int fan_dir_id = -1;
    int dir = 0;

    switch(local_id)
    {
        case ONLP_FAN_0:
            bmc_attr_id = BMC_ATTR_ID_FAN0_RPM;
            fan_present_id = BMC_ATTR_ID_FAN0_PRSNT_L;
            fan_dir_id = BMC_ATTR_ID_FAN0_DIR;
            break;
        case ONLP_FAN_1:
            bmc_attr_id = BMC_ATTR_ID_FAN1_RPM;
            fan_present_id = BMC_ATTR_ID_FAN1_PRSNT_L;
            fan_dir_id = BMC_ATTR_ID_FAN1_DIR;
            break;
        case ONLP_FAN_2:
            bmc_attr_id = BMC_ATTR_ID_FAN2_RPM;
            fan_present_id = BMC_ATTR_ID_FAN2_PRSNT_L;
            fan_dir_id = BMC_ATTR_ID_FAN2_DIR;
            break;
        case ONLP_FAN_3:
            bmc_attr_id = BMC_ATTR_ID_FAN3_RPM;
            fan_present_id = BMC_ATTR_ID_FAN3_PRSNT_L;
            fan_dir_id = BMC_ATTR_ID_FAN3_DIR;
            break;
        case ONLP_FAN_4:
            bmc_attr_id = BMC_ATTR_ID_FAN4_RPM;
            fan_present_id = BMC_ATTR_ID_FAN4_PRSNT_L;
            fan_dir_id = BMC_ATTR_ID_FAN4_DIR;
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

    //check presence for fantray 0-4
    if (local_id >= ONLP_FAN_0 && local_id <= ONLP_SYS_FAN_MAX) {
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
    } else if (local_id >= ONLP_PSU_0_FAN && local_id <= ONLP_PSU_1_FAN) {
        //check presence for psu
        int pw_present, psu_id;

        switch (local_id) {
            case ONLP_PSU_0_FAN:
                psu_id = ONLP_PSU_0;
                break;
            case ONLP_PSU_1_FAN:
                psu_id = ONLP_PSU_1;
                break;
            default:
                return ONLP_STATUS_E_INVALID;
        }
        ONLP_TRY(ufi_psu_present_get(psu_id, &pw_present));

        //update psu fan presence by psu presence status
        if(pw_present == 1) {
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

    if (local_id >= ONLP_FAN_0 && local_id <= ONLP_SYS_FAN_MAX) {
        percentage = (info->rpm*100)/sys_fan_max;
        if (percentage > 100)
            percentage = 100;
        info->percentage = percentage;
        info->status |= (rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;

        /* get fan direction status from BMC */
        ONLP_TRY(bmc_fan_dir_read(fan_dir_id, &data));

        dir = (int)data;
        if(dir == FAN_DIR_B2F) {
            /* B2F */
            info->status |= ONLP_FAN_STATUS_B2F;
            info->status &= ~ONLP_FAN_STATUS_F2B;
        } else {
            /* F2B */
            info->status |= ONLP_FAN_STATUS_F2B;
            info->status &= ~ONLP_FAN_STATUS_B2F;
        }
    } else if (local_id >= ONLP_PSU_0_FAN && local_id <= ONLP_PSU_1_FAN) {
        //get psu support info
        if(local_id == ONLP_PSU_0_FAN) {
            psu_id = ONLP_PSU_0;
        } else if(local_id == ONLP_PSU_1_FAN) {
            psu_id = ONLP_PSU_1;
        } else {
            AIM_LOG_ERROR("unknown fan id (%d), func=%s", local_id, __FUNCTION__);
            return ONLP_STATUS_E_PARAM;
        }
        ONLP_TRY(get_psu_support_info(psu_id, &psu_support_info, NULL));
        psu_type = psu_support_info.psu_type;

        //AC/DC has different MAX RPM
        if (psu_type==ONLP_PSU_TYPE_AC) {
            psu_fan_max = PSU_FAN_RPM_MAX_AC;
        } else if (psu_type==ONLP_PSU_TYPE_DC12 || psu_type==ONLP_PSU_TYPE_DC48) {
            psu_fan_max = PSU_FAN_RPM_MAX_DC;
        } else {
            AIM_LOG_ERROR("unknown psu_type from file %s, local_id=%d, psu_type=%d", TMP_PSU_TYPE, local_id-ONLP_PSU_0_FAN+1, psu_type);
            return ONLP_STATUS_E_INTERNAL;
        }

        percentage = (info->rpm*100)/psu_fan_max;
        if (percentage > 100)
            percentage = 100;
        info->percentage = percentage;
        info->status |= (rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;

        //clear and set fan direction
        info->status &= ~ONLP_FAN_STATUS_F2B;
        info->status &= ~ONLP_FAN_STATUS_B2F;
        info->status |= psu_support_info.fan_dir;
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
    int local_id, rc;

    ONLP_TRY(get_fan_local_id(id, &local_id));
    *rv = fan_info[local_id];

    switch (local_id) {
        case ONLP_FAN_0 ... ONLP_PSU_1_FAN:
            rc = ufi_bmc_fan_info_get(local_id, rv);
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
    int local_id;

    ONLP_TRY(get_fan_local_id(id, &local_id));
    *hdr = fan_info[local_id].hdr;

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
