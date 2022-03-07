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
 * Fan Platform Implementation Defaults.
 *
 ***********************************************************/
#include <onlp/platformi/fani.h>
#include "x86_64_ufispace_s9502_12sm_int.h"
#include <onlplib/file.h>
#include <onlplib/i2c.h>
#include "platform_lib.h"

onlp_fan_info_t fan_info[] = {
    { }, /* Not used */
    {
        { FAN_OID_FAN0, "Chassis Fan - 0", 0 },
        ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B,
        ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE,
        0,
        0,
        ONLP_FAN_MODE_INVALID,
    },
    {
        { FAN_OID_FAN1, "Chassis Fan - 1", 0 },
        ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B,
        ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE,
        0,
        0,
        ONLP_FAN_MODE_INVALID,
    },
    {
        { FAN_OID_FAN2, "Chassis Fan - 2", 0 },
        ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B,
        ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE,
        0,
        0,
        ONLP_FAN_MODE_INVALID,
    },
};

/*
 * This function will be called prior to all of onlp_fani_* functions.
 */
int onlp_fani_init(void)
{
    return ONLP_STATUS_OK;
}

int sys_fan_info_get(onlp_fan_info_t* info, int id)
{
    int rv=0, percentage=0;
    int hwmon_index = 2;
    int fan_index = 0;
    int fan_status = 0;
    int sys_max_fan_speed = 18000;

    if (id == FAN_ID_FAN0) {
        fan_index = 1;
    } else if (id == FAN_ID_FAN1) {
        fan_index = 2;
    } else if (id == FAN_ID_FAN2) {
        fan_index = 3;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    //s9502 series fixed fan
    info->status |= ONLP_FAN_STATUS_PRESENT;

    //get fan alarm for status
    rv = onlp_file_read_int(&fan_status, SYS_HWMON_PREFIX "fan%d_alarm", hwmon_index, fan_index);
    if (rv < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    /* fan status > 1, means failure */
    if (fan_status > 0) {
        info->status |= ONLP_FAN_STATUS_FAILED;
        return ONLP_STATUS_OK;
    }

    //get fan rpm
    rv = onlp_file_read_int(&info->rpm,
                      SYS_HWMON_PREFIX "fan%d_input", hwmon_index, fan_index);

    if (rv == ONLP_STATUS_E_MISSING) {
        info->status &= ~1;
        return 0;
    }

    percentage = (info->rpm*100)/sys_max_fan_speed;
    info->percentage = percentage;
    info->status |= (info->rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;

    return ONLP_STATUS_OK;
}

int sys_fan_rpm_percent_set(int perc)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int onlp_fani_rpm_set(onlp_oid_t id, int rpm)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * This function sets the fan speed of the given OID as a percentage.
 *
 * This will only be called if the OID has the PERCENTAGE_SET
 * capability.
 *
 * It is optional if you have no fans at all with this feature.
 */
int onlp_fani_percentage_set(onlp_oid_t id, int percentage)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int onlp_fani_info_get(onlp_oid_t id, onlp_fan_info_t* rv)
{
    int fan_id ,rc;

    fan_id = ONLP_OID_ID_GET(id);
    *rv = fan_info[fan_id];

    switch (fan_id) {
        case FAN_ID_FAN0:
        case FAN_ID_FAN1:
        case FAN_ID_FAN2:
        rc = sys_fan_info_get(rv, fan_id);
            break;
        default:
            return ONLP_STATUS_E_INTERNAL;
            break;
    }

    return rc;
}
