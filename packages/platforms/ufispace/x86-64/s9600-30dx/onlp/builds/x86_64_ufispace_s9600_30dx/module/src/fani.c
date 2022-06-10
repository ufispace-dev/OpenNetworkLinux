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
#define PSU_FAN_RPM_MAX_AC 27000
#define PSU_FAN_RPM_MAX_DC 28500

/**
 * Get all information about the given FAN oid.
 *
 * [01] CHASSIS
 *            |----[01] ONLP_FAN_0
 *            |----[02] ONLP_FAN_1
 *            |----[03] ONLP_FAN_2
 *            |----[04] ONLP_FAN_3
 *            |----[01] ONLP_PSU_0----[05] ONLP_PSU_0_FAN
 *            |
 *            |----[02] ONLP_PSU_1----[06] ONLP_PSU_1_FAN
 */
static onlp_fan_info_t __onlp_fan_info[ONLP_FAN_COUNT] = { 
    { }, /* Not used */
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN_0),
            .description = "Chassis Fan - 0",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .dir = ONLP_FAN_DIR_F2B,
        .caps = (ONLP_FAN_CAPS_GET_DIR | ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN_1),
            .description = "Chassis Fan - 1",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .dir = ONLP_FAN_DIR_F2B,
        .caps = (ONLP_FAN_CAPS_GET_DIR | ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN_2),
            .description = "Chassis Fan - 2",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .dir = ONLP_FAN_DIR_F2B,
        .caps = (ONLP_FAN_CAPS_GET_DIR | ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN_3),
            .description = "Chassis Fan - 3",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .dir = ONLP_FAN_DIR_F2B,
        .caps = (ONLP_FAN_CAPS_GET_DIR | ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_PSU_0_FAN),
            .description = "PSU 0 - Fan",
            .poid = ONLP_PSU_ID_CREATE(ONLP_PSU_0),
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .dir = ONLP_FAN_DIR_F2B,
        .caps = (ONLP_FAN_CAPS_GET_DIR | ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_PSU_1_FAN),
            .description = "PSU 1 - Fan",
            .poid = ONLP_PSU_ID_CREATE(ONLP_PSU_1),
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .dir = ONLP_FAN_DIR_F2B,
        .caps = (ONLP_FAN_CAPS_GET_DIR | ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
};

/**
 * @brief Update the status of FAN's oid header.
 * @param id The FAN ID.
 * @param[out] hdr Receives the header.
 */
static int update_fani_status(int local_id, onlp_oid_hdr_t* hdr)
{
    int fan_presence = 0;
    int psu_presence = 0;
    float data = 0;
    int bmc_attr_id = BMC_ATTR_ID_MAX;

    /* clear FAN status */
    hdr->status = 0;
    
    switch(local_id)
    {
        case ONLP_FAN_0:
            bmc_attr_id = BMC_ATTR_ID_FAN0_PRSNT_H;
            break;
        case ONLP_FAN_1:
            bmc_attr_id = BMC_ATTR_ID_FAN1_PRSNT_H;
            break;
        case ONLP_FAN_2:
            bmc_attr_id = BMC_ATTR_ID_FAN2_PRSNT_H;
            break;
        case ONLP_FAN_3:
            bmc_attr_id = BMC_ATTR_ID_FAN3_PRSNT_H;
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

    if (local_id >= ONLP_FAN_0 && local_id <= ONLP_FAN_3) {        
        hdr->status = 0;
        ONLP_TRY(bmc_sensor_read(bmc_attr_id, FAN_SENSOR, &data));

        fan_presence = (int) data;

        if(fan_presence == 1) {
            ONLP_OID_STATUS_FLAG_SET(hdr, PRESENT);
        } else {
            ONLP_OID_STATUS_FLAG_SET(hdr, UNPLUGGED);
        }
    } else if (local_id == ONLP_PSU_0_FAN) {
        ONLP_TRY(get_psu_present_status(ONLP_PSU_0, &psu_presence));
        if (psu_presence == 0) {
            ONLP_OID_STATUS_FLAG_SET(hdr, UNPLUGGED);
        } else if (psu_presence == 1) {
            ONLP_OID_STATUS_FLAG_SET(hdr, PRESENT);
        } else {
            return ONLP_STATUS_E_INTERNAL;
        }
    } else if (local_id == ONLP_PSU_1_FAN) {
        ONLP_TRY(get_psu_present_status(ONLP_PSU_1, &psu_presence));
        if (psu_presence == 0) {
            ONLP_OID_STATUS_FLAG_SET(hdr, UNPLUGGED);
        } else if (psu_presence == 1) {
            ONLP_OID_STATUS_FLAG_SET(hdr, PRESENT);
        } else {
            return ONLP_STATUS_E_INTERNAL;
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
    int sys_fan_max = SYS_FAN_RPM_MAX;
    int psu_fan_max = 0;
    int psu_type = 0;
    int bmc_attr_id = BMC_ATTR_ID_MAX;

    switch(local_id)
    {
        case ONLP_FAN_0:
            bmc_attr_id = BMC_ATTR_ID_FAN0_RPM;
            break;
        case ONLP_FAN_1:
            bmc_attr_id = BMC_ATTR_ID_FAN1_RPM;
            break;
        case ONLP_FAN_2:
            bmc_attr_id = BMC_ATTR_ID_FAN2_RPM;
            break;
        case ONLP_FAN_3:
            bmc_attr_id = BMC_ATTR_ID_FAN3_RPM;
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
    
    if ((info->hdr.status & ONLP_OID_STATUS_FLAG_PRESENT) == 0) {
        //not present, do nothing
        return ONLP_STATUS_OK;
    }

    //get fan rpm
    ONLP_TRY(bmc_sensor_read(bmc_attr_id, FAN_SENSOR, &data));
    
    rpm = (int) data;
    
    //set rpm field
    info->rpm = rpm;

    if (local_id >= ONLP_FAN_0 && local_id <= ONLP_FAN_3) {
        percentage = (info->rpm * 100) / sys_fan_max;
        if (percentage > 100)
            percentage = 100;
        info->percentage = percentage;
        info->hdr.status |= (rpm == 0) ? ONLP_OID_STATUS_FLAG_FAILED : ONLP_OID_STATUS_FLAG_OPERATIONAL;
    } else if (local_id >= ONLP_PSU_0_FAN && local_id <= ONLP_PSU_1_FAN) {
        //get psu type
        if(local_id == ONLP_PSU_0_FAN) {
            ONLP_TRY(get_psu_type(ONLP_PSU_0, &psu_type, NULL));
        } else if(local_id == ONLP_PSU_1_FAN) {
            ONLP_TRY(get_psu_type(ONLP_PSU_1, &psu_type, NULL));
        } else {
            AIM_LOG_ERROR("unknown fan id (%d), func=%s", local_id, __FUNCTION__);
            return ONLP_STATUS_E_PARAM;
        }
        
        //AC/DC has different MAX RPM
        if (psu_type==ONLP_PSU_TYPE_AC) {
            psu_fan_max = PSU_FAN_RPM_MAX_AC;
        } else if (psu_type==ONLP_PSU_TYPE_DC12 || psu_type==ONLP_PSU_TYPE_DC48) {
            psu_fan_max = PSU_FAN_RPM_MAX_DC;
        } else {
            AIM_LOG_ERROR("unknown psu_type from file %s, id=%d, psu_type=%d", TMP_PSU_TYPE, local_id-ONLP_PSU_0_FAN+1, psu_type);
            return ONLP_STATUS_E_INTERNAL;
        }
        percentage = (info->rpm * 100) / psu_fan_max; 
        if (percentage > 100)
            percentage = 100;
        info->percentage = percentage;
        info->hdr.status |= (rpm == 0) ? ONLP_OID_STATUS_FLAG_FAILED : ONLP_OID_STATUS_FLAG_OPERATIONAL;
    }
    
    return ONLP_STATUS_OK;
}

/**
 * @brief Software initialization of the Fan module.
 */
int onlp_fani_sw_init(void)
{
    lock_init();
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
    int local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_fan_info_t */
    *hdr = __onlp_fan_info[local_id].hdr;

    /* Update onlp_oid_hdr_t */
    ONLP_TRY(update_fani_status(local_id, hdr));

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the information structure for the given fan OID.
 * @param id The fan id
 * @param[out] info Receives the fan information.
 */
int onlp_fani_info_get(onlp_oid_id_t id, onlp_fan_info_t* info)
{
    int local_id = ONLP_OID_ID_GET(id);
    
    /* Set the onlp_fan_info_t */
    memset(info, 0, sizeof(onlp_fan_info_t));
    *info = __onlp_fan_info[local_id];
    ONLP_TRY(onlp_fani_hdr_get(id, &info->hdr));

    if (local_id > ONLP_FAN_RESERVED && local_id < ONLP_FAN_MAX) {
        ONLP_TRY(update_fani_info(local_id, info));
    } else {
        AIM_LOG_ERROR("unknown FAN id (%d), func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    return ONLP_STATUS_OK;
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

