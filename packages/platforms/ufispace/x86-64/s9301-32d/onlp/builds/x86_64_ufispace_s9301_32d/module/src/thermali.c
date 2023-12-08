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
#include <onlp/platformi/thermali.h>
#include "platform_lib.h"

/* This is definitions for x86-64-ufispace-s9301-32d */
/* OID map*/
/*
 * [01] CHASSIS----[01] ONLP_THERMAL_CPU_PECI
 *            |----[02] ONLP_THERMAL_CPU_ENV
 *            |----[03] ONLP_THERMAL_CPU_ENV_2
 *            |----[04] ONLP_THERMAL_MAC_ENV
 *            |----[05] ONLP_THERMAL_CAGE
 *            |----[06] ONLP_THERMAL_PSU_CONN
 *            |----[07] ONLP_THERMAL_PSU0
 *            |----[08] ONLP_THERMAL_PSU1
 *            |----[09] ONLP_THERMAL_CPU_PKG
 *            |----[10] ONLP_THERMAL_CPU1
 *            |----[11] ONLP_THERMAL_CPU2
 *            |----[12] ONLP_THERMAL_CPU3
 *            |----[13] ONLP_THERMAL_CPU4
 *            |----[14] ONLP_THERMAL_CPU5
 *            |----[15] ONLP_THERMAL_CPU6
 *            |----[16] ONLP_THERMAL_CPU7
 *            |----[17] ONLP_THERMAL_CPU8
 *            |----[01] ONLP_LED_SYSTEM
 *            |----[02] ONLP_LED_PSU0
 *            |----[03] ONLP_LED_PSU1
 *            |----[04] ONLP_LED_FAN
 *            |
 *            |----[01] ONLP_PSU0----[07] ONLP_THERMAL_PSU0
 *            |
 *            |----[02] ONLP_PSU1----[08] ONLP_THERMAL_PSU1
 */

#define UFI_ONLP_THERMAL_THRESHOLD(WARNING_DEFAULT, ERROR_DEFAULT, SHUTDOWN_DEFAULT){ \
    WARNING_DEFAULT,                                                                  \
    ERROR_DEFAULT,                                                                    \
    SHUTDOWN_DEFAULT,                                                                 \
}
/* Threshold */
#define THERMAL_SHUTDOWN_DEFAULT    105000
#define THERMAL_ERROR_DEFAULT       95000
#define THERMAL_WARNING_DEFAULT     77000

/* Shortcut for CPU thermal threshold value. */
#define CPU_THERMAL_THRESHOLD_INIT_DEFAULTS  \
    { THERMAL_WARNING_DEFAULT, \
      THERMAL_ERROR_DEFAULT,   \
      THERMAL_SHUTDOWN_DEFAULT }

/* SYSFS */
#define SYS_CPU_CORETEMP_PREFIX     "/sys/devices/platform/coretemp.0/hwmon/hwmon0/"
#define SYS_CPU_CORETEMP_PREFIX2    "/sys/devices/platform/coretemp.0/"


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
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(85000, 95000, 100000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_ENV),
            .description = "TEMP_CPU_ENV",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(80000, 85000, 90000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_ENV_2),
            .description = "TEMP_CPU_ENV_2",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(65000, 70000, 85000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC_ENV),
            .description = "TEMP_MAC_ENV",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(65000, 70000, 80000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CAGE),
            .description = "TEMP_CAGE",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(80000, 85000, 90000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU_CONN),
            .description = "TEMP_PSU_CONN",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(60000, 65000, 70000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU0),
            .description = "PSU 0 THERMAL 1",
            .poid = ONLP_PSU_ID_CREATE(ONLP_PSU0),
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
            .poid = ONLP_PSU_ID_CREATE(ONLP_PSU1),
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
        .thresholds = CPU_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU1),
            .description = "CPU Thermal 1",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = CPU_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU2),
            .description = "CPU Thermal 2",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = CPU_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU3),
            .description = "CPU Thermal 3",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = CPU_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU4),
            .description = "CPU Thermal 4",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = CPU_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU5),
            .description = "CPU Thermal 5",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = CPU_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU6),
            .description = "CPU Thermal 6",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = CPU_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU7),
            .description = "CPU Thermal 7",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = CPU_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU8),
            .description = "CPU Thermal 8",
            .poid = 0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = CPU_THERMAL_THRESHOLD_INIT_DEFAULTS
    }
};

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
    switch(local_id) {
        case ONLP_THERMAL_CPU_PECI:
            attr_id = BMC_ATTR_ID_TEMP_CPU_PECI;
            break;
        case ONLP_THERMAL_CPU_ENV:
            attr_id = BMC_ATTR_ID_TEMP_CPU_ENV;
            break;
        case ONLP_THERMAL_CPU_ENV_2:
            attr_id = BMC_ATTR_ID_TEMP_CPU_ENV_2;
            break;
        case ONLP_THERMAL_MAC_ENV:
            attr_id = BMC_ATTR_ID_TEMP_MAC_ENV;
            break;
        case ONLP_THERMAL_CAGE:
            attr_id = BMC_ATTR_ID_TEMP_CAGE;
            break;
        case ONLP_THERMAL_PSU_CONN:
            attr_id = BMC_ATTR_ID_TEMP_PSU_CONN;
            break;
        case ONLP_THERMAL_PSU0:
            attr_id = BMC_ATTR_ID_PSU0_TEMP;
            break;
        case ONLP_THERMAL_PSU1:
            attr_id = BMC_ATTR_ID_PSU1_TEMP;
            break;
        default:
            return ONLP_STATUS_E_PARAM;
    }

    /* get thermal info from BMC */
    ONLP_TRY(bmc_sensor_read(attr_id, THERMAL_SENSOR, &data));

    info->mcelsius = (int) (data*1000);

    return ONLP_STATUS_OK;
}

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
    if(local_id == ONLP_THERMAL_CPU_PKG) {
        sysfs_index = 1;
    } else if(local_id == ONLP_THERMAL_CPU1) {
        sysfs_index = 2;
    } else if(local_id == ONLP_THERMAL_CPU2) {
        sysfs_index = 3;
    } else if(local_id == ONLP_THERMAL_CPU3) {
        sysfs_index = 4;
    } else if(local_id == ONLP_THERMAL_CPU4) {
        sysfs_index = 5;
    } else if(local_id == ONLP_THERMAL_CPU5) {
        sysfs_index = 6;
    } else if(local_id == ONLP_THERMAL_CPU6) {
        sysfs_index = 7;
    } else if(local_id == ONLP_THERMAL_CPU7) {
        sysfs_index = 8;
    } else if(local_id == ONLP_THERMAL_CPU8) {
        sysfs_index = 9;
    } else {
        AIM_LOG_ERROR("unsupported thermal cpu id (%d), func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    ret = onlp_file_read_int(&temperature, SYS_CPU_CORETEMP_PREFIX "/temp%d_input", sysfs_index);
    if(ret != ONLP_STATUS_OK) {
        ONLP_TRY(onlp_file_read_int(&temperature, SYS_CPU_CORETEMP_PREFIX2 "/temp%d_input", sysfs_index));
    }

    info->mcelsius = temperature;

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

    switch(local_id) {
        case ONLP_THERMAL_CPU_PECI:
        case ONLP_THERMAL_CPU_ENV:
        case ONLP_THERMAL_CPU_ENV_2:
        case ONLP_THERMAL_MAC_ENV:
        case ONLP_THERMAL_CAGE:
        case ONLP_THERMAL_PSU_CONN:
        case ONLP_THERMAL_PSU0:
        case ONLP_THERMAL_PSU1:
            ONLP_TRY(update_thermali_from_bmc_info(local_id, info));
            break;
        case ONLP_THERMAL_CPU_PKG:
        case ONLP_THERMAL_CPU1:
        case ONLP_THERMAL_CPU2:
        case ONLP_THERMAL_CPU3:
        case ONLP_THERMAL_CPU4:
        case ONLP_THERMAL_CPU5:
        case ONLP_THERMAL_CPU6:
        case ONLP_THERMAL_CPU7:
        case ONLP_THERMAL_CPU8:
            ONLP_TRY(update_thermali_cpu_info(local_id, info));
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
    if(local_id == ONLP_THERMAL_PSU0) {
        ONLP_TRY(get_psui_present_status(ONLP_PSU0, &psu_presence));
        if(psu_presence == 0) {
            *status &= ~ONLP_THERMAL_STATUS_PRESENT;
        } else if(psu_presence == 1) {
            *status |= ONLP_THERMAL_STATUS_PRESENT;
        } else {
            return ONLP_STATUS_E_INTERNAL;
        }
    } else if(local_id == ONLP_THERMAL_PSU1) {
        ONLP_TRY(get_psui_present_status(ONLP_PSU1, &psu_presence));
        if(psu_presence == 0) {
            *status &= ~ONLP_THERMAL_STATUS_PRESENT;
        } else if(psu_presence == 1) {
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
