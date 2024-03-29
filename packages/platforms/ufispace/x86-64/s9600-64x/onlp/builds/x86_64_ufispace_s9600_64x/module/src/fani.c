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
#include "x86_64_ufispace_s9600_64x_int.h"
#include <onlplib/file.h>
#include <onlplib/i2c.h>
#include "platform_lib.h"

onlp_fan_info_t fan_info[] = {
    { }, /* Not used */
    {
        { FAN_OID_FAN0, "CHASSIS FAN 0", 0 },
        ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B,
        ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE,
        0,
        0,
        ONLP_FAN_MODE_INVALID,
        COMM_STR_NOT_SUPPORTED,
        COMM_STR_NOT_SUPPORTED,
    },
    {
        { FAN_OID_FAN1, "CHASSIS FAN 1", 0 },
        ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B,
        ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE,
        0,
        0,
        ONLP_FAN_MODE_INVALID,
        COMM_STR_NOT_SUPPORTED,
        COMM_STR_NOT_SUPPORTED,
    },
    {
        { FAN_OID_FAN2, "CHASSIS FAN 2", 0 },
        ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B,
        ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE,
        0,
        0,
        ONLP_FAN_MODE_INVALID,
        COMM_STR_NOT_SUPPORTED,
        COMM_STR_NOT_SUPPORTED,
    },
    {
        { FAN_OID_FAN3, "CHASSIS FAN 3", 0 },
        ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B,
        ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE,
        0,
        0,
        ONLP_FAN_MODE_INVALID,
        COMM_STR_NOT_SUPPORTED,
        COMM_STR_NOT_SUPPORTED,
    },
    {
        { FAN_OID_PSU0_FAN, "PSU 0 FAN", 0 },
        ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B,
        ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE,
        0,
        0,
        ONLP_FAN_MODE_INVALID,
        COMM_STR_NOT_SUPPORTED,
        COMM_STR_NOT_SUPPORTED,
    },
    {
        { FAN_OID_PSU1_FAN, "PSU 1 FAN", 0 },
        ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B,
        ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE,
        0,
        0,
        ONLP_FAN_MODE_INVALID,
        COMM_STR_NOT_SUPPORTED,
        COMM_STR_NOT_SUPPORTED,
    },
};

/*
 * This function will be called prior to all of onlp_fani_* functions.
 */
int onlp_fani_init(void)
{
    lock_init();
    return ONLP_STATUS_OK;
}

int sys_fan_info_get(onlp_fan_info_t* info, int id)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int sys_fan_rpm_percent_set(int perc)
{
    return ONLP_STATUS_E_UNSUPPORTED;
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
int onlp_fani_percentage_set(onlp_oid_t id, int percentage)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Get the information structure for the given fan OID.
 * @param id The fan OID
 * @param rv [out] Receives the fan information.
 */
int onlp_fani_info_get(onlp_oid_t id, onlp_fan_info_t* rv)
{
    int fan_id ,rc;

    fan_id = ONLP_OID_ID_GET(id);
    *rv = fan_info[fan_id];

    switch (fan_id) {
        case FAN_ID_FAN0:
        case FAN_ID_FAN1:
        case FAN_ID_FAN2:
        case FAN_ID_FAN3:
        case FAN_ID_PSU0_FAN:
        case FAN_ID_PSU1_FAN:
            rc = bmc_fan_info_get(rv, fan_id);
            break;
        default:
            return ONLP_STATUS_E_INTERNAL;
            break;
    }

    return rc;
}