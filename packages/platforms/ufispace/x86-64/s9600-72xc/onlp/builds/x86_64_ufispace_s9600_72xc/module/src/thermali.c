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

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_THERMAL(_id)) {         \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

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

static onlp_thermal_info_t thermal_info[] = {
    { }, /* Not used */
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PECI),
            .description = "TEMP_CPU_PECI",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {85000, 95000, 100000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_ENV),
            .description = "TEMP_CPU_ENV",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {80000, 85000, 90000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_ENV_2),
            .description = "TEMP_CPU_ENV_2",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {65000, 70000, 80000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC_ENV),
            .description = "TEMP_MAC_ENV",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {80000, 85000, 90000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC_DIE),
            .description = "TEMP_MAC_DIE",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {90000, 100000, 110000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV_FRONT),
            .description = "TEMP_ENV_FRONT",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {60000, 65000, 70000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV_REAR),
            .description = "TEMP_ENV_REAR",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {65000, 70000, 80000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU0),
            .description = "TEMP_PSU0",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {65000, 70000, 75000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU1),
            .description = "TEMP_PSU1",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {65000, 70000, 75000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PKG),
            .description = "CPU Package",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU1),
            .description = "CPU Thermal 1",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU2),
            .description = "CPU Thermal 2",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU3),
            .description = "CPU Thermal 3",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU4),
            .description = "CPU Thermal 4",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU5),
            .description = "CPU Thermal 5",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU6),
            .description = "CPU Thermal 6",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU7),
            .description = "CPU Thermal 7",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU8),
            .description = "CPU Thermal 8",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
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

        rv = onlp_file_read_int(&info->mcelsius,
                            SYS_CPU_CORETEMP_PREFIX2 "temp%d_input",
                            (local_id - ONLP_THERMAL_CPU_PKG) + 1);

        if(rv != ONLP_STATUS_OK) {
            return rv;
        }
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
 * @param rv [out] Receives the thermal information.
 */
int onlp_thermali_info_get(onlp_oid_t id, onlp_thermal_info_t* rv)
{
    int local_id;
    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);

    *rv = thermal_info[local_id];

    /* update status  */
    ONLP_TRY(onlp_thermali_status_get(id, &rv->status));

    if((rv->status & ONLP_THERMAL_STATUS_PRESENT) == 0) {
        return ONLP_STATUS_OK;
    }

    switch(local_id) {
        case ONLP_THERMAL_CPU_PKG:
        case ONLP_THERMAL_CPU1:
        case ONLP_THERMAL_CPU2:
        case ONLP_THERMAL_CPU3:
        case ONLP_THERMAL_CPU4:
        case ONLP_THERMAL_CPU5:
        case ONLP_THERMAL_CPU6:
        case ONLP_THERMAL_CPU7:
        case ONLP_THERMAL_CPU8:
            ONLP_TRY(ufi_cpu_thermal_info_get(rv, local_id));
            break;
        case ONLP_THERMAL_CPU_PECI:
        case ONLP_THERMAL_CPU_ENV:
        case ONLP_THERMAL_CPU_ENV_2:
        case ONLP_THERMAL_MAC_ENV:
        case ONLP_THERMAL_MAC_DIE:
        case ONLP_THERMAL_ENV_FRONT:
        case ONLP_THERMAL_ENV_REAR:
        case ONLP_THERMAL_PSU0:
        case ONLP_THERMAL_PSU1:
            ONLP_TRY(ufi_bmc_thermal_info_get(rv, local_id));
            break;
        default:
            return ONLP_STATUS_E_INTERNAL;
            break;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Retrieve the thermal's operational status.
 * @param id The thermal oid.
 * @param rv [out] Receives the operational status.
 */
int onlp_thermali_status_get(onlp_oid_t id, uint32_t* rv)
{

    int local_id;
    VALIDATE(id);
    local_id = ONLP_OID_ID_GET(id);

    *rv = thermal_info[local_id].status;
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
        if(psu_present == 0) {
            *rv = ONLP_THERMAL_STATUS_FAILED;
        } else if(psu_present == 1) {
            *rv = ONLP_THERMAL_STATUS_PRESENT;
        }
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Retrieve the thermal's oid header.
 * @param id The thermal oid.
 * @param rv [out] Receives the header.
 */
int onlp_thermali_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* rv)
{
    int local_id;
    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);
    if(local_id >= ONLP_THERMAL_MAX) {
        return ONLP_STATUS_E_INVALID;
    } else {
        *rv = thermal_info[local_id].hdr;
    }
    return ONLP_STATUS_OK;
}

/**
 * @brief Generic ioctl.
 */
int onlp_thermali_ioctl(int id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

