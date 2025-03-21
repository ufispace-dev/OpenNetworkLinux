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

/**
 * Get all information about the given FAN oid.
 *
 * [01] CHASSIS----[01] ONLP_FAN1_F
 *            |----[02] ONLP_FAN1_R
 *            |----[03] ONLP_FAN2_F
 *            |----[04] ONLP_FAN2_R
 *            |----[05] ONLP_FAN3_F
 *            |----[06] ONLP_FAN3_R
 *            |----[07] ONLP_FAN4_F
 *            |----[08] ONLP_FAN4_R
 *            |----[09] ONLP_FAN5_F
 *            |----[10] ONLP_FAN5_R
 *            |----[11] ONLP_FAN6_F
 *            |----[12] ONLP_FAN6_R
 *            |
 *            |----[01] ONLP_PSU_0----[13] ONLP_PSU0_FAN1
 *            |
 *            |----[02] ONLP_PSU_1----[14] ONLP_PSU1_FAN1
 */

#define FAN_STATUS ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B
#define FAN_CAPS   ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE
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
        COMM_STR_NOT_SUPPORTED,                 \
        COMM_STR_NOT_SUPPORTED,                 \
    }
#define FAN_FRONT_MAX_RPM   36200
#define FAN_REAR_MAX_RPM    32000
#define PSU_FAN_MAX_RPM     27000

static onlp_fan_info_t fan_info[] =
{
    { }, /* Not used */
    CHASSIS_INFO(ONLP_FAN1_F    , "CHASSIS FAN 0 FRONT"),
    CHASSIS_INFO(ONLP_FAN1_R    , "CHASSIS FAN 0 REAR"),
    CHASSIS_INFO(ONLP_FAN2_F    , "CHASSIS FAN 1 FRONT"),
    CHASSIS_INFO(ONLP_FAN2_R    , "CHASSIS FAN 1 REAR"),
    CHASSIS_INFO(ONLP_FAN3_F    , "CHASSIS FAN 2 FRONT"),
    CHASSIS_INFO(ONLP_FAN3_R    , "CHASSIS FAN 2 REAR"),
    CHASSIS_INFO(ONLP_FAN4_F    , "CHASSIS FAN 3 FRONT"),
    CHASSIS_INFO(ONLP_FAN4_R    , "CHASSIS FAN 3 REAR"),
    CHASSIS_INFO(ONLP_FAN5_F    , "CHASSIS FAN 4 FRONT"),
    CHASSIS_INFO(ONLP_FAN5_R    , "CHASSIS FAN 4 REAR"),
    CHASSIS_INFO(ONLP_FAN6_F    , "CHASSIS FAN 5 FRONT"),
    CHASSIS_INFO(ONLP_FAN6_R    , "CHASSIS FAN 5 REAR"),
    CHASSIS_INFO(ONLP_PSU0_FAN1 , "PSU 0 FAN"),
    CHASSIS_INFO(ONLP_PSU1_FAN1 , "PSU 1 FAN"),
};

#define IS_FANTRAY(_node)  (_node.type == TYPE_FANTRAY)
#define IS_PSU(_node)      (_node.type == TYPE_PSU)

typedef struct 
{
    int present;
    int rpm;
    int dir;
    int fru_id;
    int type;
    int parent;
    //FAN or PSU AC
    int max_rpm1;
    //PSU DC
    int max_rpm2;
} fan_node_t;

typedef enum fan_type_e 
{
    TYPE_FANTRAY = 0,
    TYPE_PSU,
} fan_type_t;

static int get_node(int local_id, fan_node_t *node)
{
    if(node == NULL)
        return ONLP_STATUS_E_PARAM;

    switch(local_id) {
        case ONLP_FAN1_F:
            node->present = BMC_ATTR_ID_FAN0_PSNT_L;
            node->rpm = BMC_ATTR_ID_FAN0_RPM_F;
            node->dir = BMC_ATTR_ID_FAN0_DIR;
            node->fru_id = BMC_FRU_IDX_FAN_TRAY_0;
            node->type = TYPE_FANTRAY;
            node->max_rpm1 = FAN_FRONT_MAX_RPM;
           break;
        case ONLP_FAN1_R:
            node->present = BMC_ATTR_ID_FAN0_PSNT_L;
            node->rpm = BMC_ATTR_ID_FAN0_RPM_R;
            node->dir = BMC_ATTR_ID_FAN0_DIR;
            node->fru_id = BMC_FRU_IDX_FAN_TRAY_0;
            node->type = TYPE_FANTRAY;
            node->max_rpm1 = FAN_REAR_MAX_RPM;
           break;
        case ONLP_FAN2_F:
            node->present = BMC_ATTR_ID_FAN1_PSNT_L;
            node->rpm = BMC_ATTR_ID_FAN1_RPM_F;
            node->dir = BMC_ATTR_ID_FAN1_DIR;
            node->fru_id = BMC_FRU_IDX_FAN_TRAY_1;
            node->type = TYPE_FANTRAY;
            node->max_rpm1 = FAN_FRONT_MAX_RPM;
           break;
        case ONLP_FAN2_R:
            node->present = BMC_ATTR_ID_FAN1_PSNT_L;
            node->rpm = BMC_ATTR_ID_FAN1_RPM_R;
            node->dir = BMC_ATTR_ID_FAN1_DIR;
            node->fru_id = BMC_FRU_IDX_FAN_TRAY_1;
            node->type = TYPE_FANTRAY;
            node->max_rpm1 = FAN_REAR_MAX_RPM;
           break;
        case ONLP_FAN3_F:
            node->present = BMC_ATTR_ID_FAN2_PSNT_L;
            node->rpm = BMC_ATTR_ID_FAN2_RPM_F;
            node->dir = BMC_ATTR_ID_FAN2_DIR;
            node->fru_id = BMC_FRU_IDX_FAN_TRAY_2;
            node->type = TYPE_FANTRAY;
            node->max_rpm1 = FAN_FRONT_MAX_RPM;
           break;
        case ONLP_FAN3_R:
            node->present = BMC_ATTR_ID_FAN2_PSNT_L;
            node->rpm = BMC_ATTR_ID_FAN2_RPM_R;
            node->dir = BMC_ATTR_ID_FAN2_DIR;
            node->fru_id = BMC_FRU_IDX_FAN_TRAY_2;
            node->type = TYPE_FANTRAY;
            node->max_rpm1 = FAN_REAR_MAX_RPM;
           break;
        case ONLP_FAN4_F:
            node->present = BMC_ATTR_ID_FAN3_PSNT_L;
            node->rpm = BMC_ATTR_ID_FAN3_RPM_F;
            node->dir = BMC_ATTR_ID_FAN3_DIR;
            node->fru_id = BMC_FRU_IDX_FAN_TRAY_3;
            node->type = TYPE_FANTRAY;
            node->max_rpm1 = FAN_FRONT_MAX_RPM;
           break;
        case ONLP_FAN4_R:
            node->present = BMC_ATTR_ID_FAN3_PSNT_L;
            node->rpm = BMC_ATTR_ID_FAN3_RPM_R;
            node->dir = BMC_ATTR_ID_FAN3_DIR;
            node->fru_id = BMC_FRU_IDX_FAN_TRAY_3;
            node->type = TYPE_FANTRAY;
            node->max_rpm1 = FAN_REAR_MAX_RPM;
           break;
        case ONLP_FAN5_F:
            node->present = BMC_ATTR_ID_FAN4_PSNT_L;
            node->rpm = BMC_ATTR_ID_FAN4_RPM_F;
            node->dir = BMC_ATTR_ID_FAN4_DIR;
            node->fru_id = BMC_FRU_IDX_FAN_TRAY_4;
            node->type = TYPE_FANTRAY;
            node->max_rpm1 = FAN_FRONT_MAX_RPM;
           break;
        case ONLP_FAN5_R:
            node->present = BMC_ATTR_ID_FAN4_PSNT_L;
            node->rpm = BMC_ATTR_ID_FAN4_RPM_R;
            node->dir = BMC_ATTR_ID_FAN4_DIR;
            node->fru_id = BMC_FRU_IDX_FAN_TRAY_4;
            node->type = TYPE_FANTRAY;
            node->max_rpm1 = FAN_REAR_MAX_RPM;
           break;
        case ONLP_FAN6_F:
            node->present = BMC_ATTR_ID_FAN5_PSNT_L;
            node->rpm = BMC_ATTR_ID_FAN5_RPM_F;
            node->dir = BMC_ATTR_ID_FAN5_DIR;
            node->fru_id = BMC_FRU_IDX_FAN_TRAY_5;
            node->type = TYPE_FANTRAY;
            node->max_rpm1 = FAN_FRONT_MAX_RPM;
           break;
        case ONLP_FAN6_R:
            node->present = BMC_ATTR_ID_FAN5_PSNT_L;
            node->rpm = BMC_ATTR_ID_FAN5_RPM_R;
            node->dir = BMC_ATTR_ID_FAN5_DIR;
            node->fru_id = BMC_FRU_IDX_FAN_TRAY_5;
            node->type = TYPE_FANTRAY;
            node->max_rpm1 = FAN_REAR_MAX_RPM;
           break;
        case ONLP_PSU0_FAN1:
            node->present = BMC_ATTR_ID_INVALID;
            node->rpm = BMC_ATTR_ID_PSU0_FAN1;
            node->dir = BMC_ATTR_ID_PSU0_FAN1_DIR;
            node->fru_id = BMC_FRU_IDX_INVALID;
            node->type = TYPE_PSU;
            node->parent = ONLP_PSU0;
            node->max_rpm1 = PSU_FAN_MAX_RPM;
            node->max_rpm2 = PSU_FAN_MAX_RPM;
           break;
        case ONLP_PSU1_FAN1:
            node->present = BMC_ATTR_ID_INVALID;
            node->rpm = BMC_ATTR_ID_PSU1_FAN1;
            node->dir = BMC_ATTR_ID_PSU1_FAN1_DIR;
            node->fru_id = BMC_FRU_IDX_INVALID;
            node->type = TYPE_PSU;
            node->parent = ONLP_PSU1;
            node->max_rpm1 = PSU_FAN_MAX_RPM;
            node->max_rpm2 = PSU_FAN_MAX_RPM;
           break;
        default:
            return ONLP_STATUS_E_PARAM;
    }
    return ONLP_STATUS_OK;
}

/**
 * @brief Check if fan FRU is supported
 */
static int is_fru_sup(void) {

    board_t board = {0};
    ONLP_TRY(get_board_version(&board));
    if(board.hw_rev >= HW_REV_PVT && board.hw_build >= HW_BUILD_1) {
        return ONLP_STATUS_OK;
    } else {
        return ONLP_STATUS_E_UNSUPPORTED;
    }
}

/**
 * @brief Update the information of Model and Serial from FAN EEPROM
 * @param local_id The FAN Local ID
 * @param[out] info Receives the FAN information (model and serial).
 */
static int update_fru_info(int local_id, onlp_fan_info_t* info)
{
    bmc_fru_t fru = {0};
    fan_node_t node = {0};

    ONLP_TRY(get_node(local_id, &node));
    if(node.fru_id == BMC_FRU_IDX_INVALID) {
        return ONLP_STATUS_OK;
    }

    if(is_fru_sup() != ONLP_STATUS_OK) {
        return ONLP_STATUS_OK;
    }

    //read fru data
    ONLP_TRY(bmc_fru_read(node.fru_id, &fru));

    //update FRU model
    memset(info->model, 0, sizeof(info->model));
    snprintf(info->model, sizeof(info->model), "%s", fru.name.val);

    //update FRU serial
    memset(info->serial, 0, sizeof(info->serial));
    snprintf(info->serial, sizeof(info->serial), "%s", fru.serial.val);

    return ONLP_STATUS_OK;
}

/**
 * @brief Update the information structure for the given FAN
 * @param id The FAN Local ID
 * @param[out] info Receives the FAN information.
 */
static int update_fani_info(int local_id, onlp_fan_info_t* info)
{
    int ret = ONLP_STATUS_OK;
    int rpm = 0, percentage = 0;
    float data = 0;
    int max_fan_speed = 0;
    int rpm_attr_id = -1;
    fan_node_t node = {0};

    if((info->status & ONLP_FAN_STATUS_PRESENT) == 0) {
        //not present, do nothing
        return ONLP_STATUS_OK;
    }  

    ONLP_TRY(get_node(local_id, &node));
    /* set bmc attr id */
    rpm_attr_id = node.rpm;
    max_fan_speed = node.max_rpm1;

    /* get fan rpm from BMC */
    ret = bmc_sensor_read(rpm_attr_id, FAN_SENSOR, &data);
    if(ret != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%d\n", local_id);
        return ONLP_STATUS_E_INTERNAL;
    }

    rpm = (int) data;

    //set rpm field
    info->rpm = rpm;
    percentage = (info->rpm * 100) / max_fan_speed;
    percentage = (percentage > 100) ? 100 : percentage;
    info->percentage = percentage;
    info->status |= (rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;

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
 * @param info [out] Receives the fan information.
 */
int onlp_fani_info_get(onlp_oid_t id, onlp_fan_info_t* info)
{
    int local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_fan_info_t */
    memset(info, 0, sizeof(onlp_fan_info_t));
    *info = fan_info[local_id];
    ONLP_TRY(onlp_fani_hdr_get(id, &info->hdr));

    /* Update onlp fani status */
    ONLP_TRY(onlp_fani_status_get(id, &info->status));

    if(local_id >= ONLP_FAN1_F && local_id <= ONLP_PSU1_FAN1) {
        ONLP_TRY(update_fani_info(local_id, info));
    } else {
        AIM_LOG_ERROR("unknown FAN id (%d), func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    //update fan fru info
    ONLP_TRY(update_fru_info(local_id, info));
    return ONLP_STATUS_OK;
}

/**
 * @brief Retrieve the fan's operational status.
 * @param id The fan OID.
 * @param status [out] Receives the fan's operations status flags.
 * @notes Only operational state needs to be returned -
 *        PRESENT/FAILED
 */
int onlp_fani_status_get(onlp_oid_t id, uint32_t* status)
{
    int local_id = ONLP_OID_ID_GET(id);
    int fan_presence = 0;
    int psu_presence = 0;
    float data = 0;
    int dir = 0;
    fan_node_t node = {0};

    /* clear FAN status */
    *status = 0;

    ONLP_TRY(get_node(local_id, &node));

    if(IS_FANTRAY(node)) {
        /* get fan present status from BMC */
        ONLP_TRY(bmc_sensor_read(node.present, FAN_SENSOR, &data));

        fan_presence = (int)data;
        if(fan_presence == 1) {
            *status |= ONLP_FAN_STATUS_PRESENT;
            *status |= ONLP_FAN_STATUS_F2B;
        } else {
            *status &= ~ONLP_FAN_STATUS_PRESENT;
        }

    } else if(IS_PSU(node)) {
        /* get psu fan present status */
        ONLP_TRY(get_psui_present_status(node.parent, &psu_presence));
        if(psu_presence == 0) {
            *status &= ~ONLP_FAN_STATUS_PRESENT;
        } else if(psu_presence == 1) {
            *status |= ONLP_FAN_STATUS_PRESENT;
        } else {
            return ONLP_STATUS_E_INTERNAL;
        }
    } else {
        AIM_LOG_ERROR("unknown fan id (%d), func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    /* get fan direction status from BMC */
    ONLP_TRY(bmc_fan_dir_read(node.dir, &data));

    dir = (int)data;
    if(dir == FAN_DIR_B2F) {
        /* B2F */
        *status |= ONLP_FAN_STATUS_B2F;
        *status &= ~ONLP_FAN_STATUS_F2B;
    } else {
        /* F2B */
        *status |= ONLP_FAN_STATUS_F2B;
        *status &= ~ONLP_FAN_STATUS_B2F;
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
    int local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_fan_info_t */
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
int onlp_fani_ioctl(onlp_oid_t fid, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}
