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
#include <onlplib/file.h>
#include "x86_64_ufispace_s9501_18smt_log.h"
#include "platform_lib.h"

static onlp_thermal_info_t thermal_info[] = {
    { }, /* Not used */
    { { THERMAL_OID_CPU_PKG, "CPU Package", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    { { THERMAL_OID_CPU0, "CPU Thermal 0", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    { { THERMAL_OID_CPU1, "CPU Thermal 1", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    { { THERMAL_OID_DRAM0, "DRAM Thermal 0", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    { { THERMAL_OID_DRAM1, "DRAM Thermal 1", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    { { THERMAL_OID_MAC, "TEMP_MAC", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, {96000, 101000, 106000}
    },
    { { THERMAL_OID_PSU0, "PSU 0 - Thermal Sensor 1", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, {80000, 85000, 90000}
    },
    { { THERMAL_OID_PSU1, "PSU 1 - Thermal Sensor 1", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, {80000, 85000, 90000}
    },

};

/*
 * This will be called to intiialize the thermali subsystem.
 */
int
onlp_thermali_init(void)
{
    lock_init();
    return ONLP_STATUS_OK;
}

static int
cpu_thermal_info_get(onlp_thermal_info_t* info, int id)
{
    int rv;
    int hwmon_index = 1;
    int temp_index;

    if (id == THERMAL_ID_CPU_PKG) {
        temp_index = 1;
    } else if (id == THERMAL_ID_CPU0) {
        temp_index = 8;
    } else if (id == THERMAL_ID_CPU1) {
        temp_index = 14;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    rv = onlp_file_read_int(&info->mcelsius,
                            SYS_HWMON_PREFIX "temp%d_input", hwmon_index, temp_index);

    if(rv == ONLP_STATUS_E_MISSING) {
        info->status &= ~1;
        return 0;
    }

    return ONLP_STATUS_OK;
}

static int
dram_thermal_info_get(onlp_thermal_info_t* info, int id)
{
    int rv;
    int hwmon_index = 0;
    int temp_index=1;

    if (id == THERMAL_ID_DRAM0) {
        hwmon_index = 2;

    } else if (id == THERMAL_ID_DRAM1) {
        hwmon_index = 3;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    rv = onlp_file_read_int(&info->mcelsius,
                      SYS_HWMON_PREFIX "temp%d_input", hwmon_index, temp_index);

    if (rv == ONLP_STATUS_E_MISSING) {
        info->status &= ~1;
        return 0;
    }

    return ONLP_STATUS_OK;
}

int
psu_thermal_info_get(onlp_thermal_info_t* info, int id)
{
    int rv;

    if ( bmc_enable ) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    rv = psu_thermal_get(info, id);
    if(rv == ONLP_STATUS_E_INTERNAL) {
        return rv;
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
int
onlp_thermali_info_get(onlp_oid_t id, onlp_thermal_info_t* info)
{
    int sensor_id, rc;
    sensor_id = ONLP_OID_ID_GET(id);

    *info = thermal_info[sensor_id];
    info->caps |= ONLP_THERMAL_CAPS_GET_TEMPERATURE;

    switch (sensor_id) {
        case THERMAL_ID_CPU_PKG:
        case THERMAL_ID_CPU0:
        case THERMAL_ID_CPU1:
            rc = cpu_thermal_info_get(info, sensor_id);
            break;
        case THERMAL_ID_DRAM0:
        case THERMAL_ID_DRAM1:
            rc = dram_thermal_info_get(info, sensor_id);
            break;
        case THERMAL_ID_MAC:
        case THERMAL_ID_PSU0:
        case THERMAL_ID_PSU1:
            rc = bmc_thermal_info_get(info, sensor_id);
            break;
        default:
            return ONLP_STATUS_E_INTERNAL;
            break;
    }

    return rc;
}
