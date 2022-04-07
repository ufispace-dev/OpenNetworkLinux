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
 * Thermal Sensor Platform Implementation.
 *
 ***********************************************************/
#include <unistd.h>
#include <onlplib/file.h>
#include <onlp/platformi/thermali.h>
#include "platform_lib.h"

/* SYSFS PATH */
#define SYS_CPU_CORETEMP_PREFIX "/sys/devices/platform/coretemp.0/hwmon/hwmon0/"
#define SYS_CPU_CORETEMP_PREFIX2 "/sys/devices/platform/coretemp.0/"
#define SYS_CORE_TEMP_PREFIX    "/sys/class/hwmon/hwmon2/"

/* Threshold */
#define THERMAL_SHUTDOWN_DEFAULT    105000
#define THERMAL_ERROR_DEFAULT       95000
#define THERMAL_WARNING_DEFAULT     77000

/* Shortcut for CPU thermal threshold value. */
#define THERMAL_THRESHOLD_INIT_DEFAULTS  \
    { THERMAL_WARNING_DEFAULT, \
      THERMAL_ERROR_DEFAULT,   \
      THERMAL_SHUTDOWN_DEFAULT }
/**
 * Get all information about the given Thermal oid.
 *
 *            |----[01] ONLP_THERMAL_CPU_PECI
 *            |----[02] ONLP_THERMAL_CPU_ENV
 *            |----[03] ONLP_THERMAL_CPU_ENV_2
 *            |----[04] ONLP_THERMAL_MAC_ENV
 *            |----[05] ONLP_THERMAL_MAC_DIE
 *            |----[06] ONLP_THERMAL_ENV_FRONT
 *            |----[07] ONLP_THERMAL_ENV_REAR
 *            |----[08] ONLP_THERMAL_PSU0
 *            |----[09] ONLP_THERMAL_PSU1
 *            |----[10] ONLP_THERMAL_CPU_PKG
 *            |----[11] ONLP_THERMAL_CPU1
 *            |----[12] ONLP_THERMAL_CPU2
 *            |----[13] ONLP_THERMAL_CPU3
 *            |----[14] ONLP_THERMAL_CPU4
 *            |----[15] ONLP_THERMAL_CPU5
 *            |----[16] ONLP_THERMAL_CPU6
 *            |----[17] ONLP_THERMAL_CPU7
 *            |----[18] ONLP_THERMAL_CPU8
 *            |
 *            |----[01] ONLP_PSU_0----[08] ONLP_THERMAL_PSU0
 *            |
 *            |----[02] ONLP_PSU_1----[09] ONLP_THERMAL_PSU1
 */
/* Static values */
static onlp_thermal_info_t __onlp_thermal_info[] = {
    { }, /* Not used */
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PECI),
            .description = "TEMP_CPU_PECI",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = {85000, 95000, 100000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_ENV),
            .description = "TEMP_CPU_ENV",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = {80000, 85000, 90000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_ENV_2),
            .description = "TEMP_CPU_ENV_2",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = {65000, 70000, 80000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC_ENV),
            .description = "TEMP_MAC_ENV",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = {80000, 85000, 90000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC_DIE),
            .description = "TEMP_MAC_DIE",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = {90000, 100000, 110000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV_FRONT),
            .description = "TEMP_ENV_FRONT",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = {60000, 65000, 70000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV_REAR),
            .description = "TEMP_ENV_REAR",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = {65000, 70000, 80000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU0),
            .description = "TEMP_PSU0",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = {65000, 70000, 75000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU1),
            .description = "TEMP_PSU1",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = {65000, 70000, 75000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PKG),
            .description = "CPU Package",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU1),
            .description = "CPU Thermal 1",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU2),
            .description = "CPU Thermal 2",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU3),
            .description = "CPU Thermal 3",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU4),
            .description = "CPU Thermal 4",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU5),
            .description = "CPU Thermal 5",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU6),
            .description = "CPU Thermal 6",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU7),
            .description = "CPU Thermal 7",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU8),
            .description = "CPU Thermal 8",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = THERMAL_THRESHOLD_INIT_DEFAULTS
    },
};

static int ufi_cpu_thermal_info_get(onlp_thermal_info_t* info, int local_id)
{
    int rv;

    rv = onlp_file_read_int(&info->mcelsius,
                            SYS_CPU_CORETEMP_PREFIX "temp%d_input",
                            (local_id - ONLP_THERMAL_CPU_PKG) + 1);

    if(rv != ONLP_STATUS_OK) {

        ONLP_TRY(onlp_file_read_int(&info->mcelsius,
                            SYS_CPU_CORETEMP_PREFIX2 "temp%d_input",
                            (local_id - ONLP_THERMAL_CPU_PKG) + 1));
    }

    return ONLP_STATUS_OK;
}

int ufi_bmc_thermal_info_get(onlp_thermal_info_t* info, int local_id)
{
    float data=0;
    int index;
    index = local_id - ONLP_THERMAL_CPU_PECI;

    ONLP_TRY(bmc_sensor_read(index, THERMAL_SENSOR, &data));
    info->mcelsius = (int) (data*1000);

    return ONLP_STATUS_OK;
}

/**
 * @brief Software initialization of the Thermal module.
 */
int onlp_thermali_sw_init(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Hardware initialization of the Thermal module.
 * @param flags The hardware initialization flags.
 */
int onlp_thermali_hw_init(uint32_t flags)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Deinitialize the thermal software module.
 * @note The primary purpose of this API is to properly
 * deallocate any resources used by the module in order
 * faciliate detection of real resouce leaks.
 */
int onlp_thermali_sw_denit(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Validate a thermal oid.
 * @param id The thermal id.
 */
int onlp_thermali_id_validate(onlp_oid_id_t id)
{
    return ONLP_OID_ID_VALIDATE_RANGE(id, 1, ONLP_THERMAL_MAX-1);
}

/**
 * @brief Retrieve the thermal's oid header.
 * @param id The thermal oid.
 * @param[out] hdr Receives the header.
 */
int onlp_thermali_hdr_get(onlp_oid_id_t id, onlp_oid_hdr_t* hdr)
{
    int local_id;
    local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_oid_hdr_t */
    *hdr = __onlp_thermal_info[local_id].hdr;

    /* When the PSU module is unplugged, the psu thermal does not exist. */
    if(local_id == ONLP_THERMAL_PSU0 || local_id == ONLP_THERMAL_PSU1) {
        int psu_local_id = ONLP_PSU_MAX;

        if(local_id == ONLP_THERMAL_PSU0) {
             psu_local_id = ONLP_PSU_0;
        } else {
             psu_local_id = ONLP_PSU_1;
        }

        int psu_present = 0;
        ONLP_TRY(psu_present_get(&psu_present, psu_local_id));
        hdr->status = psu_present ? ONLP_OID_STATUS_FLAG_PRESENT : ONLP_OID_STATUS_FLAG_UNPLUGGED;
    }

    return ONLP_STATUS_OK;
}

/*
 * Retrieve the information structure for the given thermal OID.
 *
 * If the OID is invalid, return ONLP_E_STATUS_INVALID.
 * If an unexpected error occurs, return ONLP_E_STATUS_INTERNAL.
 * Otherwise, return ONLP_STATUS_OK with the OID's information.
 *
 * Note -- it is expected that you fill out the information
 * structure even if the sensor described by the OID is not present.
 */

/**
 * @brief Get the information for the given thermal OID.
 * @param id The Thermal OID
 * @param[out] info Receives the thermal information.
 */
int onlp_thermali_info_get(onlp_oid_id_t id, onlp_thermal_info_t* info)
{
    int local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_thermal_info_t */
    memset(info, 0, sizeof(onlp_thermal_info_t));
    *info = __onlp_thermal_info[local_id];
    ONLP_TRY(onlp_thermali_hdr_get(id, &info->hdr));

    switch(local_id) {
        case ONLP_THERMAL_CPU_PKG ... ONLP_THERMAL_CPU8:
            ONLP_TRY(ufi_cpu_thermal_info_get(info, local_id));
            break;
        case ONLP_THERMAL_CPU_PECI ... ONLP_THERMAL_PSU1:
            ONLP_TRY(ufi_bmc_thermal_info_get(info, local_id));
            break;
        default:
            return ONLP_STATUS_E_PARAM;
            break;
    }

    return ONLP_STATUS_OK;
}
