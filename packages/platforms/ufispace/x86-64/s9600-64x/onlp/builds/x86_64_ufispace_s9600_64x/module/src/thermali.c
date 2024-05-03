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
#include "x86_64_ufispace_s9600_64x_log.h"
#include "platform_lib.h"

//FIXE: check thermal threshold

static onlp_thermal_info_t thermal_info[] = {
    { }, /* Not used */
    { { THERMAL_OID_ADC_CPU, "ADC_CPU_TEMP", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, {85000, 95000, 100000}
    },
    { { THERMAL_OID_CPU_PECI, "TEMP_CPU_PECI", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, {85000, 95000, 100000}
    },
    { { THERMAL_OID_FRONT_ENV, "TEMP_FRONT_ENV", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, {60000, 65000, 70000}
    },
    { { THERMAL_OID_OCXO, "TEMP_OCXO", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, {60000, 65000, 70000}
    },
    { { THERMAL_OID_Q2CL_ENV, "TEMP_Q2CL_ENV", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, {65000, 70000, 80000}
    },
    { { THERMAL_OID_Q2CL_DIE, "TEMP_Q2CL_DIE", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, {100000, 105000, 110000}
    },
    { { THERMAL_OID_Q2CR_ENV, "TEMP_Q2CR_ENV", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, {65000, 70000, 88000}
    },
    { { THERMAL_OID_Q2CR_DIE, "TEMP_Q2CR_DIE", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, {100000, 105000, 110000}
    },
    { { THERMAL_OID_REAR_ENV_1, "TEMP_REAR_ENV_1", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, {60000, 65000, 70000}
    },
    { { THERMAL_OID_REAR_ENV_2, "TEMP_REAR_ENV_2", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, {60000, 65000, 70000}
    },
    { { THERMAL_OID_PSU0, "PSU 0 THERMAL 1", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, {65000, 70000, 75000}
    },
    { { THERMAL_OID_PSU1, "PSU 1 THERMAL 1", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, {65000, 70000, 75000}
    },
    { { THERMAL_OID_CPU_PKG, "CPU Package", 0},
                ONLP_THERMAL_STATUS_PRESENT,
                ONLP_THERMAL_CAPS_ALL, 0, THERMAL_THRESHOLD_INIT_DEFAULTS
    },
};

/**
 * @brief Initialize the thermal subsystem.
 */
int onlp_thermali_init(void)
{
    lock_init();
    return ONLP_STATUS_OK;
}

static int cpu_thermal_info_get(onlp_thermal_info_t* info, int id)
{
    int rv;

    rv = onlp_file_read_int(&info->mcelsius,
                            SYS_CPU_CORETEMP_PREFIX "temp%d_input", (id - THERMAL_ID_CPU_PKG) + 1);

    if(rv != ONLP_STATUS_OK) {

        rv = onlp_file_read_int(&info->mcelsius,
                            SYS_CPU_CORETEMP_PREFIX2 "temp%d_input", (id - THERMAL_ID_CPU_PKG) + 1);

        if(rv != ONLP_STATUS_OK) {
            return rv;
        }
    }

    if(rv == ONLP_STATUS_E_MISSING) {
        info->status &= ~1;
        return 0;
    }

    return ONLP_STATUS_OK;
}

int psu_thermal_info_get(onlp_thermal_info_t* info, int id)
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

/**
 * @brief Get the information for the given thermal OID.
 * @param id The Thermal OID
 * @param rv [out] Receives the thermal information.
 */
int onlp_thermali_info_get(onlp_oid_t id, onlp_thermal_info_t* info)
{
    int sensor_id, rc;
    sensor_id = ONLP_OID_ID_GET(id);

    *info = thermal_info[sensor_id];
    info->caps |= ONLP_THERMAL_CAPS_GET_TEMPERATURE;

    switch (sensor_id) {
        case THERMAL_ID_CPU_PKG:
            rc = cpu_thermal_info_get(info, sensor_id);
            break;
        case THERMAL_ID_ADC_CPU:
        case THERMAL_ID_CPU_PECI:
        case THERMAL_ID_FRONT_ENV:
        case THERMAL_ID_OCXO:
        case THERMAL_ID_Q2CL_ENV:
        case THERMAL_ID_Q2CL_DIE:
        case THERMAL_ID_Q2CR_ENV:
        case THERMAL_ID_Q2CR_DIE:
        case THERMAL_ID_REAR_ENV_1:
        case THERMAL_ID_REAR_ENV_2:
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