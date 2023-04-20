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
#define SYS_FAN_FRONT_MAX_RPM   36200
#define SYS_FAN_REAR_MAX_RPM    32000
#define PSU_FAN_RPM_MAX_AC      26500
#define PSU_FAN_RPM_MAX_DC      29000

#define CHASSIS_INFO(id, des)                               \
    {                                                           \
        { ONLP_FAN_ID_CREATE(id), des, POID_0},\
        FAN_STATUS,                                             \
        FAN_CAPS,                                               \
        0,                                                      \
        0,                                                      \
        ONLP_FAN_MODE_INVALID,                                  \
    }

/*
 * Get the fan information.
 */

onlp_fan_info_t fan_info[] = {
    { }, /* Not used */
    CHASSIS_INFO(ONLP_FAN_0_F  , "Chassis Fan - 0 Front"),
    CHASSIS_INFO(ONLP_FAN_0_R  , "Chassis Fan - 0 Rear"),
    CHASSIS_INFO(ONLP_FAN_1_F  , "Chassis Fan - 1 Front"),
    CHASSIS_INFO(ONLP_FAN_1_R  , "Chassis Fan - 1 Rear"),
    CHASSIS_INFO(ONLP_FAN_2_F  , "Chassis Fan - 2 Front"),
    CHASSIS_INFO(ONLP_FAN_2_R  , "Chassis Fan - 2 Rear"),
    CHASSIS_INFO(ONLP_FAN_3_F  , "Chassis Fan - 3 Front"),
    CHASSIS_INFO(ONLP_FAN_3_R  , "Chassis Fan - 3 Rear"),
    CHASSIS_INFO(ONLP_PSU_0_FAN, "PSU-0-Fan"),
    CHASSIS_INFO(ONLP_PSU_1_FAN, "PSU-1-Fan"),
};

typedef struct
{
    int present;
    int rpm;
    int dir;
} fan_attr_t;

static const fan_attr_t fan_attr[] = {
    /*                   present                   rpm                        dir  */
    [ONLP_FAN_0_F]   = { BMC_ATTR_ID_FAN0_PRSNT_L, BMC_ATTR_ID_FAN0_RPM_F   , BMC_OEM_IDX_FAN_0_F_DIR},
    [ONLP_FAN_0_R]   = { BMC_ATTR_ID_FAN0_PRSNT_L, BMC_ATTR_ID_FAN0_RPM_R   , BMC_OEM_IDX_FAN_0_R_DIR},
    [ONLP_FAN_1_F]   = { BMC_ATTR_ID_FAN1_PRSNT_L, BMC_ATTR_ID_FAN1_RPM_F   , BMC_OEM_IDX_FAN_1_F_DIR},
    [ONLP_FAN_1_R]   = { BMC_ATTR_ID_FAN1_PRSNT_L, BMC_ATTR_ID_FAN1_RPM_R   , BMC_OEM_IDX_FAN_1_R_DIR},
    [ONLP_FAN_2_F]   = { BMC_ATTR_ID_FAN2_PRSNT_L, BMC_ATTR_ID_FAN2_RPM_F   , BMC_OEM_IDX_FAN_2_F_DIR},
    [ONLP_FAN_2_R]   = { BMC_ATTR_ID_FAN2_PRSNT_L, BMC_ATTR_ID_FAN2_RPM_R   , BMC_OEM_IDX_FAN_2_R_DIR},
    [ONLP_FAN_3_F]   = { BMC_ATTR_ID_FAN3_PRSNT_L, BMC_ATTR_ID_FAN3_RPM_F   , BMC_OEM_IDX_FAN_3_F_DIR},
    [ONLP_FAN_3_R]   = { BMC_ATTR_ID_FAN3_PRSNT_L, BMC_ATTR_ID_FAN3_RPM_R   , BMC_OEM_IDX_FAN_3_R_DIR},
    [ONLP_PSU_0_FAN] = { BMC_ATTR_ID_INVALID     , BMC_ATTR_ID_PSU0_FAN     , BMC_OEM_IDX_PSU_0_FAN_DIR},
    [ONLP_PSU_1_FAN] = { BMC_ATTR_ID_INVALID     , BMC_ATTR_ID_PSU1_FAN     , BMC_OEM_IDX_PSU_1_FAN_DIR},
};

/**
 * @brief Get and check fan local ID
 * @param id [in] OID
 * @param local_id [out] The fan local id
 */
static int get_fan_local_id(int id, int *local_id)
{
    int tmp_id;
    board_t board = {0};

    if(local_id == NULL) {
        return ONLP_STATUS_E_PARAM;
    }

    if(!ONLP_OID_IS_FAN(id)) {
        return ONLP_STATUS_E_INVALID;
    }

    tmp_id = ONLP_OID_ID_GET(id);
    ONLP_TRY(get_board_version(&board));

    if(board.hw_rev <= BRD_BETA) {
        switch (tmp_id) {
            case ONLP_FAN_0_F:
            case ONLP_FAN_0_R:
            case ONLP_FAN_1_F:
            case ONLP_FAN_1_R:
            case ONLP_FAN_2_F:
            case ONLP_FAN_2_R:
            case ONLP_FAN_3_F:
            case ONLP_FAN_3_R:
            case ONLP_PSU_0_FAN:
            case ONLP_PSU_1_FAN:
                *local_id = tmp_id;
                return ONLP_STATUS_OK;
            default:
                return ONLP_STATUS_E_INVALID;
        }
    } else {
        switch (tmp_id) {
            case ONLP_FAN_0_F:
            case ONLP_FAN_0_R:
            case ONLP_FAN_1_F:
            case ONLP_FAN_1_R:
            case ONLP_FAN_2_F:
            case ONLP_FAN_2_R:
            case ONLP_PSU_0_FAN:
            case ONLP_PSU_1_FAN:
                *local_id = tmp_id;
                return ONLP_STATUS_OK;
            default:
                return ONLP_STATUS_E_INVALID;
        }
    }
    return ONLP_STATUS_E_INVALID;
}

/**
 * @brief Get the fan information from BMC
 * @param id [in] FAN ID
 * @param info [out] The fan information
 */
static int get_bmc_fan_info(int local_id, onlp_fan_info_t* info)
{
    int rpm=0, percentage=0;
    float data=0;
    int sys_max_fan_speed = SYS_FAN_FRONT_MAX_RPM;
    int psu_max_fan_speed = PSU_FAN_RPM_MAX_AC;
    int presence;
    int dir =BMC_FAN_DIR_UNK;

    *info = fan_info[local_id];
    /* present */
    if (local_id >= ONLP_FAN_0_F && local_id <= ONLP_FAN_3_R) {
        int bmc_attr = fan_attr[local_id].present;
        ONLP_TRY(read_bmc_sensor(bmc_attr, FAN_SENSOR, &data));
        presence = (int) data;

        if(presence == BMC_ATTR_STATUS_PRES) {
            info->status |= ONLP_FAN_STATUS_PRESENT;
        } else {
            info->status &= ~ONLP_FAN_STATUS_PRESENT;
        }
    } else if(local_id == ONLP_PSU_0_FAN){
        ONLP_TRY(get_psu_present_status(ONLP_PSU_0, &presence));
        if( presence == PSU_STATUS_PRES ) {
            info->status |= ONLP_FAN_STATUS_PRESENT;
        } else {
            info->status &= ~ONLP_FAN_STATUS_PRESENT;
        }
    } else if(local_id == ONLP_PSU_1_FAN){
        ONLP_TRY(get_psu_present_status(ONLP_PSU_1, &presence));
        if( presence == PSU_STATUS_PRES ) {
            info->status |= ONLP_FAN_STATUS_PRESENT;
        } else {
            info->status &= ~ONLP_FAN_STATUS_PRESENT;
        }
    }

    /* direction */
    if(info->status & ONLP_FAN_STATUS_PRESENT) {
        int bmc_oem_attr = fan_attr[local_id].dir;
        // get fan dir
        ONLP_TRY(read_bmc_oem(bmc_oem_attr, &dir));
        if(dir == BMC_FAN_DIR_B2F) {
            info->status |= ONLP_FAN_STATUS_B2F;
            info->status &= ~ONLP_FAN_STATUS_F2B;
        } else if(dir == BMC_FAN_DIR_F2B){
            info->status |= ONLP_FAN_STATUS_F2B;
            info->status &= ~ONLP_FAN_STATUS_B2F;
        } else {
            info->status &= ~ONLP_FAN_STATUS_F2B;
            info->status &= ~ONLP_FAN_STATUS_B2F;
            AIM_LOG_ERROR("unknown dir fan id (%d), func=%s", local_id, __FUNCTION__);
        }
    } else {
        info->status &= ~ONLP_FAN_STATUS_F2B;
        info->status &= ~ONLP_FAN_STATUS_B2F;
    }

    /* contents */
    if(info->status & ONLP_FAN_STATUS_PRESENT) {
        //get fan rpm
        int bmc_attr = fan_attr[local_id].rpm;
        ONLP_TRY(read_bmc_sensor(bmc_attr, FAN_SENSOR, &data));
        rpm = (int) data;

        if(rpm == BMC_ATTR_INVALID_VAL) {
            info->status |= ONLP_FAN_STATUS_FAILED;
            return ONLP_STATUS_OK;
        }

        //set rpm field
        info->rpm = rpm;

        switch(local_id) {
            case ONLP_FAN_0_F:
            case ONLP_FAN_1_F:
            case ONLP_FAN_2_F:
            case ONLP_FAN_3_F:
            {
                sys_max_fan_speed = SYS_FAN_FRONT_MAX_RPM;
                percentage = (info->rpm*100)/sys_max_fan_speed;
                info->percentage = (percentage >= 100) ? 100:percentage;
                info->status |= (rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;
                break;
            }
            case ONLP_FAN_0_R:
            case ONLP_FAN_1_R:
            case ONLP_FAN_2_R:
            case ONLP_FAN_3_R:
            {
                sys_max_fan_speed = SYS_FAN_REAR_MAX_RPM;
                percentage = (info->rpm*100)/sys_max_fan_speed;
                info->percentage = (percentage >= 100) ? 100:percentage;
                info->status |= (rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;
                break;
            }
            case ONLP_PSU_0_FAN:
            case ONLP_PSU_1_FAN:
            {
                int psu_type = ONLP_PSU_TYPE_AC;
                if(local_id == ONLP_PSU_0_FAN) {
                    ONLP_TRY(get_psu_type(ONLP_PSU_0, &psu_type, NULL));
                } else if(local_id == ONLP_PSU_1_FAN) {
                    ONLP_TRY(get_psu_type(ONLP_PSU_1, &psu_type, NULL));
                } else {
                    AIM_LOG_ERROR("unknown fan id (%d), func=%s", local_id, __FUNCTION__);
                    return ONLP_STATUS_E_PARAM;
                }

                if(psu_type == ONLP_PSU_TYPE_AC) {
                    psu_max_fan_speed = PSU_FAN_RPM_MAX_AC;
                }else if(psu_type == ONLP_PSU_TYPE_DC48) {
                    psu_max_fan_speed = PSU_FAN_RPM_MAX_DC;
                }else {
                    psu_max_fan_speed = PSU_FAN_RPM_MAX_AC;
                }

                percentage = (info->rpm*100)/psu_max_fan_speed;
                info->percentage = (percentage >= 100) ? 100:percentage;
                info->status |= (rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;
                break;
            }
            default:
                return ONLP_STATUS_E_INVALID;
        }
    }
    return ONLP_STATUS_OK;
}

/**
  * @brief Initialize the fan platform subsystem.
  */
int onlp_fani_init(void)
{
    init_lock();
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

    ONLP_TRY(get_fan_local_id(id, &local_id));
    ONLP_TRY(get_bmc_fan_info(local_id, rv));

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
    onlp_fan_info_t info ={0};

    ONLP_TRY(get_fan_local_id(id, &local_id));
    ONLP_TRY(get_bmc_fan_info(local_id, &info));
    *rv = info.status;

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

