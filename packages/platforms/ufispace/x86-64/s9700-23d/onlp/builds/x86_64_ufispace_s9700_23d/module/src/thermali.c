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
 * Thermal Sensor Platform Implementation.
 *
 ***********************************************************/
#include <unistd.h>
#include <onlplib/file.h>
#include <onlp/platformi/thermali.h>
#include <onlplib/i2c.h>
//#include <fcntl.h>
#include "platform_lib.h"


#define UFI_ONLP_THERMAL_THRESHOLD(WARNING_DEFAULT, ERROR_DEFAULT, SHUTDOWN_DEFAULT){ \
    WARNING_DEFAULT,                                                                  \
    ERROR_DEFAULT,                                                                    \
    SHUTDOWN_DEFAULT,                                                                 \
}

/**
 * Get all information about the given Thermal oid.
 *
 * [01] CHASSIS----[01] ONLP_THERMAL_CPU_PECI
 *            |----[02] ONLP_THERMAL_OP2_ENV
 *            |----[03] ONLP_THERMAL_J2_ENV_1
 *            |----[04] ONLP_THERMAL_J2_DIE_1
 *            |----[05] ONLP_THERMAL_J2_ENV_2
 *            |----[06] ONLP_THERMAL_J2_DIE_2
 *            |----[09] ONLP_THERMAL_CPU_PKG
 *            |----[10] ONLP_THERMAL_CPU_BOARD
 *            |----[11] ONLP_THERMAL_BMC_ENV
 *            |----[12] ONLP_THERMAL_ENV
 *            |----[13] ONLP_THERMAL_ENV_FRONT
 *            |
 *            |----[01] ONLP_PSU_0----[07] ONLP_THERMAL_PSU0
 *            |
 *            |----[02] ONLP_PSU_1----[08] ONLP_THERMAL_PSU1
 */
/* Static values */
static onlp_thermal_info_t __onlp_thermal_info[] = {
    { }, /* Not used */
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PECI),
            .description = "TEMP_CPU_PECI",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(90000, 95000, 100000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_OP2_ENV),
            .description = "TEMP_OP2_ENV",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(70000, 75000, 80000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_J2_ENV_1),
            .description = "TEMP_J2_ENV_1",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(70000, 75000, 80000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_J2_DIE_1),
            .description = "TEMP_J2_DIE_1",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(100000, 105000, 110000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_J2_ENV_2),
            .description = "TEMP_J2_ENV_2",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(70000, 75000, 80000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_J2_DIE_2),
            .description = "TEMP_J2_DIE_2",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(100000, 105000, 110000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU0),
            .description = "PSU 0 THERMAL 1",
            .poid = ONLP_PSU_ID_CREATE(ONLP_PSU_0),
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(65000, 70000, 75000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU1),
            .description = "PSU 1 THERMAL 1",
            .poid = ONLP_PSU_ID_CREATE(ONLP_PSU_1),
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(65000, 70000, 75000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PKG),
            .description = "CPU Package",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(77000, 95000, 105000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_BOARD),
            .description = "CPU Board",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(77000, 95000, 105000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_BMC_ENV),
            .description = "TEMP_BMC_ENV",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(60000, 65000, 70000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV),
            .description = "TEMP_ENV",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(60000, 65000, 70000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV_FRONT),
            .description = "TEMP_ENV_FRONT",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(60000, 65000, 70000),
    }
};

/**
 * @brief Update the information structure for the CPU thermal
 * @param id The Thermal Local ID
 * @param[out] info Receives the thermal information.
 */
static int update_thermali_cpu_info(int local_id, onlp_thermal_info_t* info)
{
    int ret = ONLP_STATUS_OK;
    int temperature = 0;
    int sysfs_index = -1;

    /* set sysfs index */
    if (local_id == ONLP_THERMAL_CPU_PKG) {
        sysfs_index = 1;
    } else {
        AIM_LOG_ERROR("unsupported thermal cpu id (%d), func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    ret = onlp_file_read_int(&temperature, "/sys/devices/platform/coretemp.0/hwmon/hwmon0/temp%d_input", sysfs_index);
    if(ret != ONLP_STATUS_OK) {
        ONLP_TRY(onlp_file_read_int(&temperature, "/sys/devices/platform/coretemp.0/temp%d_input", sysfs_index));
    }

    info->mcelsius = temperature;

    return ONLP_STATUS_OK;
}

/**
 * @brief Update the information structure for the CPU Board thermal
 * @param id The Thermal Local ID
 * @param[out] info Receives the thermal information.
 */
static int update_thermali_cpu_board_info(onlp_thermal_info_t* info)
{
    int ret = ONLP_STATUS_OK;
    int temperature = 0;

    ret = onlp_file_read_int(&temperature, "/sys/bus/i2c/devices/0-004f/hwmon/hwmon1/temp1_input");
    if (ret != ONLP_STATUS_OK) {
        ONLP_TRY(onlp_file_read_int(&temperature, "/sys/bus/i2c/devices/0-004f/temp1_input"));
    }

    info->mcelsius = temperature;

    return ONLP_STATUS_OK;
}

/**
 * @brief Update the information structure for the thermal that got from BMC
 * @param id The Thermal Local ID
 * @param[out] info Receives the thermal information.
 */
static int update_thermali_from_bmc_info(int local_id, onlp_thermal_info_t* info)
{
    float data = 0;
    int attr_id = -1;

    /* set bmc attr id */
    switch (local_id) {
        case ONLP_THERMAL_CPU_PECI:
            attr_id = BMC_ATTR_ID_TEMP_CPU_PECI;
            break;
        case ONLP_THERMAL_OP2_ENV:
            attr_id = BMC_ATTR_ID_TEMP_OP2_ENV;
            break;
        case ONLP_THERMAL_J2_ENV_1:
            attr_id = BMC_ATTR_ID_TEMP_J2_ENV_1;
            break;
        case ONLP_THERMAL_J2_DIE_1:
            attr_id = BMC_ATTR_ID_TEMP_J2_DIE_1;
            break;
        case ONLP_THERMAL_J2_ENV_2:
            attr_id = BMC_ATTR_ID_TEMP_J2_ENV_2;
            break;
        case ONLP_THERMAL_J2_DIE_2:
            attr_id = BMC_ATTR_ID_TEMP_J2_DIE_2;
            break;
        case ONLP_THERMAL_PSU0:
            attr_id = BMC_ATTR_ID_PSU0_TEMP;
            break;
        case ONLP_THERMAL_PSU1:
            attr_id = BMC_ATTR_ID_PSU1_TEMP;
            break;
        case ONLP_THERMAL_BMC_ENV:
            attr_id = BMC_ATTR_ID_TEMP_BMC_ENV;
            break;
        case ONLP_THERMAL_ENV:
            attr_id = BMC_ATTR_ID_TEMP_ENV;
            break;
        case ONLP_THERMAL_ENV_FRONT:
            attr_id = BMC_ATTR_ID_TEMP_ENV_FRONT;
            break;
        default:
            return ONLP_STATUS_E_PARAM;
            break;
    }

    /* get thermal info from BMC */
    ONLP_TRY(bmc_sensor_read(attr_id, THERMAL_SENSOR, &data));

    info->mcelsius = (int) (data*1000);

    return ONLP_STATUS_OK;
}

/**
 * @brief Initialize the thermal subsystem.
 */
int onlp_thermali_init(void)
{
    lock_init();
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the information for the given thermal OID.
 * @param id The Thermal OID
 * @param info [out] Receives the thermal information.
 */
int onlp_thermali_info_get(onlp_oid_t id, onlp_thermal_info_t* info)
{
    int local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_thermal_info_t */
    memset(info, 0, sizeof(onlp_thermal_info_t));
    *info = __onlp_thermal_info[local_id];
    ONLP_TRY(onlp_thermali_hdr_get(id, &info->hdr));

    /* Update the onlp_thermal_info_t status */
    ONLP_TRY(onlp_thermali_status_get(id, &info->status));

    //info->caps |= ONLP_THERMAL_CAPS_GET_TEMPERATURE;

    switch (local_id) {
        case ONLP_THERMAL_CPU_PKG:
            ONLP_TRY(update_thermali_cpu_info(local_id, info));
            break;
        case ONLP_THERMAL_CPU_BOARD:
            ONLP_TRY(update_thermali_cpu_board_info(info));
            break;
        case ONLP_THERMAL_CPU_PECI:
        case ONLP_THERMAL_OP2_ENV:
        case ONLP_THERMAL_J2_ENV_1:
        case ONLP_THERMAL_J2_DIE_1:
        case ONLP_THERMAL_J2_ENV_2:
        case ONLP_THERMAL_J2_DIE_2:
        case ONLP_THERMAL_PSU0:
        case ONLP_THERMAL_PSU1:
        case ONLP_THERMAL_BMC_ENV:
        case ONLP_THERMAL_ENV:
        case ONLP_THERMAL_ENV_FRONT:
            ONLP_TRY(update_thermali_from_bmc_info(local_id, info));
            break;
        default:
            return ONLP_STATUS_E_PARAM;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Retrieve the thermal's operational status.
 * @param id The thermal oid.
 * @param status [out] Receives the operational status.
 */
int onlp_thermali_status_get(onlp_oid_t id, uint32_t* status)
{
    int local_id = ONLP_OID_ID_GET(id);
    int psu_presence = 0;

    /* clear status */
    *status = 0;

    /* When the PSU module is unplugged, the psu thermal does not exist. */
    if (local_id == ONLP_THERMAL_PSU0) {
        ONLP_TRY(get_psui_present_status(ONLP_PSU_0, &psu_presence));
        if (psu_presence == 0) {
            *status &= ~ONLP_THERMAL_STATUS_PRESENT;
        } else if (psu_presence == 1) {
            *status |= ONLP_THERMAL_STATUS_PRESENT;
        } else {
            return ONLP_STATUS_E_INTERNAL;
        }
    } else if (local_id == ONLP_THERMAL_PSU1) {
        ONLP_TRY(get_psui_present_status(ONLP_PSU_1, &psu_presence));
        if (psu_presence == 0) {
            *status &= ~ONLP_THERMAL_STATUS_PRESENT;
        } else if (psu_presence == 1) {
            *status |= ONLP_THERMAL_STATUS_PRESENT;
        } else {
            return ONLP_STATUS_E_INTERNAL;
        }
    } else {
        //do nothing, all thermals are present as default setting.
        *status |= ONLP_THERMAL_STATUS_PRESENT;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Retrieve the thermal's oid header.
 * @param id The thermal oid.
 * @param hdr [out] Receives the header.
 */
int onlp_thermali_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* hdr)
{
    int local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_oid_hdr_t */
    *hdr = __onlp_thermal_info[local_id].hdr;

    return ONLP_STATUS_OK;
}

/**
 * @brief Generic ioctl.
 */
int onlp_thermali_ioctl(int id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}
