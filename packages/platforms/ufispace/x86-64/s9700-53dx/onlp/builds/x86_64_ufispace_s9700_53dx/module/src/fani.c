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
#include <onlplib/file.h>
#include "platform_lib.h"


/**
 * Get all information about the given FAN oid.
 *
 * [01] CHASSIS----[01] ONLP_FAN_1
 *            |----[02] ONLP_FAN_2
 *            |----[03] ONLP_FAN_3
 *            |----[04] ONLP_FAN_4
 *            |
 *            |----[01] ONLP_PSU_0----[05] ONLP_PSU0_FAN_1
 *            |                  |----[06] ONLP_PSU0_FAN_2
 *            |
 *            |----[02] ONLP_PSU_1----[07] ONLP_PSU1_FAN_1
 *            |                  |----[08] ONLP_PSU1_FAN_2
 */
static onlp_fan_info_t __onlp_fan_info[ONLP_FAN_COUNT] = { 
    { }, /* Not used */
    {   
        .hdr = { 
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN_1),
            .description = "Chassis Fan - 1",
            .poid = 0,
        },  
        .status = (ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B),
        .caps = (ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM),
        .model = "", 
        .serial = "", 
    }, 
    {   
        .hdr = { 
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN_2),
            .description = "Chassis Fan - 2",
            .poid = 0,
        },  
        .status = (ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B),
        .caps = (ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM),
        .model = "", 
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN_3),
            .description = "Chassis Fan - 3",
            .poid = 0,
        },
        .status = (ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B),
        .caps = (ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN_4),
            .description = "Chassis Fan - 4",
            .poid = 0,
        },
        .status = (ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B),
        .caps = (ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_PSU0_FAN_1),
            .description = "PSU 0 - Fan 1",
            .poid = ONLP_PSU_ID_CREATE(ONLP_PSU_0),
        },
        .status = (ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B),
        .caps = (ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_PSU0_FAN_2),
            .description = "PSU 0 - Fan 2",
            .poid = ONLP_PSU_ID_CREATE(ONLP_PSU_0),
        },
        .status = (ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B),
        .caps = (ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_PSU1_FAN_1),
            .description = "PSU 1 - Fan 1",
            .poid = ONLP_PSU_ID_CREATE(ONLP_PSU_1),
        },
        .status = (ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B),
        .caps = (ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_PSU1_FAN_2),
            .description = "PSU 1 - Fan 2",
            .poid = ONLP_PSU_ID_CREATE(ONLP_PSU_1),
        },
        .status = (ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B),
        .caps = (ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
};


/**
 * @brief Update the information structure for the given FAN
 * @param id The FAN Local ID
 * @param[out] info Receives the FAN information.
 */
static int update_fani_info(int local_id, onlp_fan_info_t* info)
{
    int rpm = 0, percentage = 0;
    float data = 0;
    int sys_max_fan_speed = 16000;
    int psu_max_fan1_speed = 28500;
    int psu_max_fan2_speed = 26000;
    int max_fan_speed = 0;
    int attr_id = -1;

    if ((info->status & ONLP_FAN_STATUS_PRESENT) == 0) {
        //not present, do nothing
        return ONLP_STATUS_OK;
    }

    /* set bmc attr id */
    if (local_id == ONLP_FAN_1) {
        attr_id = BMC_ATTR_ID_FAN0_RPM;
    } else if (local_id == ONLP_FAN_2) {
        attr_id = BMC_ATTR_ID_FAN1_RPM;
    } else if (local_id == ONLP_FAN_3) {
        attr_id = BMC_ATTR_ID_FAN2_RPM;
    } else if (local_id == ONLP_FAN_4) {
        attr_id = BMC_ATTR_ID_FAN3_RPM;
    } else if (local_id == ONLP_PSU0_FAN_1) {
        attr_id = BMC_ATTR_ID_PSU0_FAN1;
    } else if (local_id == ONLP_PSU0_FAN_2) {
        attr_id = BMC_ATTR_ID_PSU0_FAN2;
    } else if (local_id == ONLP_PSU1_FAN_1) {
        attr_id = BMC_ATTR_ID_PSU1_FAN1;
    } else if (local_id == ONLP_PSU1_FAN_2) {
        attr_id = BMC_ATTR_ID_PSU1_FAN2;
    } else {
        return ONLP_STATUS_E_PARAM;
    }

    /* get fan rpm from BMC */
    ONLP_TRY(bmc_sensor_read(attr_id, FAN_SENSOR, &data));

    rpm = (int) data;

    //set rpm field
    info->rpm = rpm;

    if (local_id >= ONLP_FAN_1 && local_id <= ONLP_FAN_4) {
        percentage = (info->rpm * 100) / sys_max_fan_speed;
        percentage = (percentage > 100) ? 100 : percentage;
        info->percentage = percentage;
        info->status |= (rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;
    } else if (local_id >= ONLP_PSU0_FAN_1 && local_id <= ONLP_PSU1_FAN_2) {
        if (local_id == ONLP_PSU0_FAN_1 || local_id == ONLP_PSU1_FAN_1 ) {
            max_fan_speed = psu_max_fan1_speed;
        } else {
            max_fan_speed = psu_max_fan2_speed;
        }
        percentage = (info->rpm * 100) / max_fan_speed;
        percentage = (percentage > 100) ? 100 : percentage;
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
 * @param info [out] Receives the fan information.
 */
int onlp_fani_info_get(onlp_oid_t id, onlp_fan_info_t* info)
{
    int local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_fan_info_t */
    memset(info, 0, sizeof(onlp_fan_info_t));
    *info = __onlp_fan_info[local_id];
    ONLP_TRY(onlp_fani_hdr_get(id, &info->hdr));

    /* Update onlp fani status */
    ONLP_TRY(onlp_fani_status_get(id, &info->status));

    if (local_id >= ONLP_FAN_1 && local_id <= ONLP_PSU1_FAN_2) {
        ONLP_TRY(update_fani_info(local_id, info));
    } else {
        AIM_LOG_ERROR("unknown FAN id (%d), func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

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
    int attr_id = -1;
    
    /* clear FAN status */
    *status = 0;
    
    if (local_id >= ONLP_FAN_1 && local_id <= ONLP_FAN_4) {
        *status = 0;
        
        /* set bmc attr id */
        if (local_id == ONLP_FAN_1) {
            attr_id = BMC_ATTR_ID_FAN0_PRSNT_H;
        } else if (local_id == ONLP_FAN_2) {
            attr_id = BMC_ATTR_ID_FAN1_PRSNT_H;
        } else if (local_id == ONLP_FAN_3) {
            attr_id = BMC_ATTR_ID_FAN2_PRSNT_H;
        } else {
            attr_id = BMC_ATTR_ID_FAN3_PRSNT_H;
        }
        
        /* get fan present status from BMC */
        ONLP_TRY(bmc_sensor_read(attr_id, FAN_SENSOR, &data));
        
        fan_presence = (int) data;
        
        if( fan_presence == 1 ) {
            *status |= ONLP_FAN_STATUS_PRESENT;
            *status |= ONLP_FAN_STATUS_F2B;
        } else {
            *status &= ~ONLP_FAN_STATUS_PRESENT;
        }
    } else if (local_id == ONLP_PSU0_FAN_1 || local_id <= ONLP_PSU0_FAN_2) {
        ONLP_TRY(get_psui_present_status(ONLP_PSU_0, &psu_presence));
        if (psu_presence == 0) {
            *status &= ~ONLP_FAN_STATUS_PRESENT;
        } else if (psu_presence == 1) {
            *status |= ONLP_FAN_STATUS_PRESENT;
            *status |= ONLP_FAN_STATUS_F2B;
        } else {
            return ONLP_STATUS_E_INTERNAL;
        }
    } else if (local_id == ONLP_PSU1_FAN_1 || local_id <= ONLP_PSU1_FAN_2) {
        ONLP_TRY(get_psui_present_status(ONLP_PSU_1, &psu_presence));
        if (psu_presence == 0) {
            *status &= ~ONLP_FAN_STATUS_PRESENT;
        } else if (psu_presence == 1) {
            *status |= ONLP_FAN_STATUS_PRESENT;
            *status |= ONLP_FAN_STATUS_F2B;
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
 * @brief Retrieve the fan's OID hdr.
 * @param id The fan OID.
 * @param rv [out] Receives the OID header.
 */
int onlp_fani_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* hdr)
{
    int local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_fan_info_t */
    *hdr = __onlp_fan_info[local_id].hdr;

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
