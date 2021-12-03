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

static onlp_fan_info_t __onlp_fan_info[ONLP_FAN_COUNT] = {
    { }, /* Not used */
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN1_F),
            .description = "Chassis Fan - 1 Front",
            .poid = 0,
        },
        .status = (ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B),
        .caps = (ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN1_R),
            .description = "Chassis Fan - 1 Rear",
            .poid = 0,
        },
        .status = (ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B),
        .caps = (ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN2_F),
            .description = "Chassis Fan - 2 Front",
            .poid = 0,
        },
        .status = (ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B),
        .caps = (ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN2_R),
            .description = "Chassis Fan - 2 Rear",
            .poid = 0,
        },
        .status = (ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B),
        .caps = (ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN3_F),
            .description = "Chassis Fan - 3 Front",
            .poid = 0,
        },
        .status = (ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B),
        .caps = (ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN3_R),
            .description = "Chassis Fan - 3 Rear",
            .poid = 0,
        },
        .status = (ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B),
        .caps = (ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN4_F),
            .description = "Chassis Fan - 4 Front",
            .poid = 0,
        },
        .status = (ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B),
        .caps = (ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN4_R),
            .description = "Chassis Fan - 4 Rear",
            .poid = 0,
        },
        .status = (ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B),
        .caps = (ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN5_F),
            .description = "Chassis Fan - 5 Front",
            .poid = 0,
        },
        .status = (ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B),
        .caps = (ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN5_R),
            .description = "Chassis Fan - 5 Rear",
            .poid = 0,
        },
        .status = (ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B),
        .caps = (ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN6_F),
            .description = "Chassis Fan - 6 Front",
            .poid = 0,
        },
        .status = (ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B),
        .caps = (ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN6_R),
            .description = "Chassis Fan - 6 Rear",
            .poid = 0,
        },
        .status = (ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B),
        .caps = (ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_PSU0_FAN1),
            .description = "PSU 0 - Fan 1",
            .poid = ONLP_PSU_ID_CREATE(ONLP_PSU0),
        },
        .status = (ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B),
        .caps = (ONLP_FAN_CAPS_GET_RPM),
        .model = "",
        .serial = "",
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_PSU1_FAN1),
            .description = "PSU 1 - Fan 1",
            .poid = ONLP_PSU_ID_CREATE(ONLP_PSU1),
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
    int ret = ONLP_STATUS_OK;
    int rpm = 0, percentage = 0;
    float data = 0;
    int sys_max_fan_f_speed = 36200; //front fan max speed
    int sys_max_fan_r_speed = 32000; //rear  fan max speed
    int sys_max_fan_speed = 0;
    int psu_max_fan1_speed = 27000;
    int max_fan_speed = 0;
    int attr_id = -1;

    if((info->status & ONLP_FAN_STATUS_PRESENT) == 0) {
        //not present, do nothing
        return ONLP_STATUS_OK;
    }

    /* set bmc attr id */
    if(local_id == ONLP_FAN1_F) {
        attr_id = BMC_ATTR_ID_FAN0_RPM_F;
        sys_max_fan_speed = sys_max_fan_f_speed;

    } else if(local_id == ONLP_FAN1_R) {
        attr_id = BMC_ATTR_ID_FAN0_RPM_R;
        sys_max_fan_speed = sys_max_fan_r_speed;

    } else if(local_id == ONLP_FAN2_F) {
        attr_id = BMC_ATTR_ID_FAN1_RPM_F;
        sys_max_fan_speed = sys_max_fan_f_speed;

    } else if(local_id == ONLP_FAN2_R) {
        attr_id = BMC_ATTR_ID_FAN1_RPM_R;
        sys_max_fan_speed = sys_max_fan_r_speed;

    } else if(local_id == ONLP_FAN3_F) {
        attr_id = BMC_ATTR_ID_FAN2_RPM_F;
        sys_max_fan_speed = sys_max_fan_f_speed;

    } else if(local_id == ONLP_FAN3_R) {
        attr_id = BMC_ATTR_ID_FAN2_RPM_R;
        sys_max_fan_speed = sys_max_fan_r_speed;

    } else if(local_id == ONLP_FAN4_F) {
        attr_id = BMC_ATTR_ID_FAN3_RPM_F;
        sys_max_fan_speed = sys_max_fan_f_speed;

    } else if(local_id == ONLP_FAN4_R) {
        attr_id = BMC_ATTR_ID_FAN3_RPM_R;
        sys_max_fan_speed = sys_max_fan_r_speed;

    } else if(local_id == ONLP_FAN5_F) {
        attr_id = BMC_ATTR_ID_FAN4_RPM_F;
        sys_max_fan_speed = sys_max_fan_f_speed;

    } else if(local_id == ONLP_FAN5_R) {
        attr_id = BMC_ATTR_ID_FAN4_RPM_R;
        sys_max_fan_speed = sys_max_fan_r_speed;

    } else if(local_id == ONLP_FAN6_F) {
        attr_id = BMC_ATTR_ID_FAN5_RPM_F;
        sys_max_fan_speed = sys_max_fan_f_speed;

    } else if(local_id == ONLP_FAN6_R) {
        attr_id = BMC_ATTR_ID_FAN5_RPM_R;
        sys_max_fan_speed = sys_max_fan_r_speed;

    } else if(local_id == ONLP_PSU0_FAN1) {
        attr_id = BMC_ATTR_ID_PSU0_FAN1;
        sys_max_fan_speed = sys_max_fan_f_speed;

    } else if(local_id == ONLP_PSU1_FAN1) {
        attr_id = BMC_ATTR_ID_PSU1_FAN1;
        sys_max_fan_speed = sys_max_fan_r_speed;

    } else {
        AIM_LOG_ERROR("unknown id, func=%s, local_id=%d\n", __FUNCTION__, local_id);
        return ONLP_STATUS_E_PARAM;

    }

    /* get fan rpm from BMC */
    ret = bmc_sensor_read(attr_id, FAN_SENSOR, &data);
    if(ret != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%d\n", local_id);
        return ONLP_STATUS_E_INTERNAL;
    }

    rpm = (int) data;

    //set rpm field
    info->rpm = rpm;

    if(local_id >= ONLP_FAN1_F && local_id <= ONLP_FAN6_R) {
        percentage = (info->rpm * 100) / sys_max_fan_speed;
        percentage = (percentage > 100) ? 100 : percentage;
        info->percentage = percentage;
        info->status |= (rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;
    } else if(local_id >= ONLP_PSU0_FAN1 && local_id <= ONLP_PSU1_FAN1) {
        max_fan_speed = psu_max_fan1_speed;
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

    if(local_id >= ONLP_FAN1_F && local_id <= ONLP_PSU1_FAN1) {
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
    int attr_id_presence = -1;
#if (UFI_BMC_FAN_DIR == 1)
    int attr_id_direction = -1;
    int dir = 0;
#endif

    /* clear FAN status */
    *status = 0;

    if(local_id >= ONLP_FAN1_F && local_id <= ONLP_FAN6_R) {
        *status = 0;

        /* set bmc attr id */
        if(local_id == ONLP_FAN1_F || local_id == ONLP_FAN1_R) {
            attr_id_presence = BMC_ATTR_ID_FAN0_PSNT_L;
#if (UFI_BMC_FAN_DIR == 1)
            attr_id_direction = BMC_ATTR_ID_FAN0_DIR;
#endif

        } else if(local_id == ONLP_FAN2_F || local_id == ONLP_FAN2_R) {
            attr_id_presence = BMC_ATTR_ID_FAN1_PSNT_L;
#if (UFI_BMC_FAN_DIR == 1)
            attr_id_direction = BMC_ATTR_ID_FAN1_DIR;
#endif

        } else if(local_id == ONLP_FAN3_F || local_id == ONLP_FAN3_R) {
            attr_id_presence = BMC_ATTR_ID_FAN2_PSNT_L;
#if (UFI_BMC_FAN_DIR == 1)
            attr_id_direction = BMC_ATTR_ID_FAN2_DIR;
#endif

        } else if(local_id == ONLP_FAN4_F || local_id == ONLP_FAN4_R) {
            attr_id_presence = BMC_ATTR_ID_FAN3_PSNT_L;
#if (UFI_BMC_FAN_DIR == 1)
            attr_id_direction = BMC_ATTR_ID_FAN3_DIR;
#endif

        } else if(local_id == ONLP_FAN5_F || local_id == ONLP_FAN5_R) {
            attr_id_presence = BMC_ATTR_ID_FAN4_PSNT_L;
#if (UFI_BMC_FAN_DIR == 1)
            attr_id_direction = BMC_ATTR_ID_FAN4_DIR;
#endif

        } else if(local_id == ONLP_FAN6_F || local_id == ONLP_FAN6_R) {
            attr_id_presence = BMC_ATTR_ID_FAN5_PSNT_L;
#if (UFI_BMC_FAN_DIR == 1)
            attr_id_direction = BMC_ATTR_ID_FAN5_DIR;
#endif

        } else {
            AIM_LOG_ERROR("unknown id, func=%s, local_id=%d\n", __FUNCTION__, local_id);
            return ONLP_STATUS_E_INTERNAL;

        }

        /* get fan present status from BMC */
        ONLP_TRY(bmc_sensor_read(attr_id_presence, FAN_SENSOR, &data));

        fan_presence = (int) data;
        if( fan_presence == 1 ) {
            *status |= ONLP_FAN_STATUS_PRESENT;
            *status |= ONLP_FAN_STATUS_F2B;
        } else {
            *status &= ~ONLP_FAN_STATUS_PRESENT;
        }

#if (UFI_BMC_FAN_DIR == 1)
        /* if UFI_BMC_FAN_DIR is not defined as 1, do nothing (BMC does not support!!) */
        /* get fan direction status from BMC */
        ONLP_TRY(bmc_sensor_read(attr_id_direction, FAN_SENSOR, &data));

        dir = (int) data;
        if(dir == FAN_DIR_B2F) {
            /* B2F */
            *status |= ONLP_FAN_STATUS_B2F;
            *status &= ~ONLP_FAN_STATUS_F2B;
        } else {
            /* F2B */
            *status |= ONLP_FAN_STATUS_F2B;
            *status &= ~ONLP_FAN_STATUS_B2F;
        }
#endif

    } else if(local_id == ONLP_PSU0_FAN1) {
        /* get psu0 fan present status */
        ONLP_TRY(get_psui_present_status(ONLP_PSU0, &psu_presence));
        if(psu_presence == 0) {
            *status &= ~ONLP_FAN_STATUS_PRESENT;
        } else if(psu_presence == 1) {
            *status |= ONLP_FAN_STATUS_PRESENT;
            *status |= ONLP_FAN_STATUS_F2B;
        } else {
            return ONLP_STATUS_E_INTERNAL;
        }

#if (UFI_BMC_FAN_DIR == 1)
        /* if UFI_BMC_FAN_DIR is not defined as 1, do nothing (BMC does not support!!) */
        /* get psu0 fan direction status from BMC */
        ONLP_TRY(bmc_sensor_read(attr_id_direction, FAN_SENSOR, &data));

        dir = (int) data;
        if(dir == FAN_DIR_B2F) {
            /* B2F */
            *status |= ONLP_FAN_STATUS_B2F;
            *status &= ~ONLP_FAN_STATUS_F2B;
        } else {
            /* F2B */
            *status |= ONLP_FAN_STATUS_F2B;
            *status &= ~ONLP_FAN_STATUS_B2F;
        }
#endif

    } else if(local_id == ONLP_PSU1_FAN1) {
        /* get psu1 fan present status */
        ONLP_TRY(get_psui_present_status(ONLP_PSU1, &psu_presence));
        if(psu_presence == 0) {
            *status &= ~ONLP_FAN_STATUS_PRESENT;
        } else if(psu_presence == 1) {
            *status |= ONLP_FAN_STATUS_PRESENT;
            *status |= ONLP_FAN_STATUS_F2B;
        } else {
            return ONLP_STATUS_E_INTERNAL;
        }

#if (UFI_BMC_FAN_DIR == 1)
        /* get psu1 fan direction status from BMC */
        ONLP_TRY(bmc_sensor_read(attr_id_direction, FAN_SENSOR, &data));

        dir = (int) data;
        if(dir == FAN_DIR_B2F) {
            /* B2F */
            *status |= ONLP_FAN_STATUS_B2F;
            *status &= ~ONLP_FAN_STATUS_F2B;
        } else {
            /* F2B */
            *status |= ONLP_FAN_STATUS_F2B;
            *status &= ~ONLP_FAN_STATUS_B2F;
        }
#endif

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
