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
        if(!ONLP_OID_IS_THERMAL(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

#define MILLI(cel) (cel * 1000)

 //FIXME threshold
static onlp_thermal_info_t thermal_info[] = {
    { }, /* Not used */
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PKG),
            .description = "CPU Package",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {MILLI(0), MILLI(75), MILLI(95)}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_0),
            .description = "CPU Thermal 0",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {MILLI(0), MILLI(75), MILLI(95)}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_1),
            .description = "CPU Thermal 1",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {MILLI(0), MILLI(75), MILLI(95)}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_2),
            .description = "CPU Thermal 2",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {MILLI(0), MILLI(75), MILLI(95)}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_3),
            .description = "CPU Thermal 3",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {MILLI(0), MILLI(75), MILLI(95)}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC0),
            .description = "MAC0 Thermal",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {MILLI(0), MILLI(100), MILLI(110)}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC1),
            .description = "MAC1 Thermal",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {MILLI(0), MILLI(100), MILLI(110)}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_DDR_L),
            .description = "DDR_L Thermal",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {MILLI(0), MILLI(101), MILLI(106)}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_DDR_R),
            .description = "DDR_R Thermal",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {MILLI(0), MILLI(101), MILLI(106)}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_BMC),
            .description = "BMC Thermal",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {MILLI(0), MILLI(90), MILLI(100)}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_FRONT),
            .description = "Front Thermal",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {MILLI(0), MILLI(90), MILLI(100)}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU_0),
            .description = "PSU-0-Thermal",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL),
        .mcelsius = 0,
        .thresholds = {MILLI(80), MILLI(85), MILLI(90)}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU_1),
            .description = "PSU-1-Thermal",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL),
        .mcelsius = 0,
        .thresholds = {MILLI(80), MILLI(85), MILLI(90)}
    }
};

// FIX Check cpu thermal sysfs
int cpu_thermal_sysfs_id [] =
{
    [ONLP_THERMAL_CPU_PKG] = 1,
    [ONLP_THERMAL_CPU_0]   = 4,
    [ONLP_THERMAL_CPU_1]   = 8,
    [ONLP_THERMAL_CPU_2]   = 10,
    [ONLP_THERMAL_CPU_3]   = 14,
};

static int ufi_cpu_thermal_info_get(int local_id, onlp_thermal_info_t* info)
{
    int rv;
    rv = onlp_file_read_int(&info->mcelsius,
                            SYS_CPU_CORETEMP_PREFIX "temp%d_input", cpu_thermal_sysfs_id[local_id]);

    if(rv < 0) {
        rv = onlp_file_read_int(&info->mcelsius,
                            SYS_CPU_CORETEMP_PREFIX2 "temp%d_input", cpu_thermal_sysfs_id[local_id]);
        if(rv < 0) {
            return rv;
        }
    }

    return ONLP_STATUS_OK;
}

int ufi_bmc_thermal_info_get(int local_id, onlp_thermal_info_t* info)
{
    float data=0;
    int bmc_attr_id = BMC_ATTR_ID_MAX;

    switch(local_id)
    {
        case ONLP_THERMAL_MAC0:
            bmc_attr_id = BMC_ATTR_ID_TEMP_MAC0;
            break;
        case ONLP_THERMAL_MAC1:
            bmc_attr_id = BMC_ATTR_ID_TEMP_MAC1;
            break;
        case ONLP_THERMAL_DDR_L:
            bmc_attr_id = BMC_ATTR_ID_TEMP_DDR_L;
            break;
        case ONLP_THERMAL_DDR_R:
            bmc_attr_id = BMC_ATTR_ID_TEMP_DDR_R;
            break;
        case ONLP_THERMAL_BMC:
            bmc_attr_id = BMC_ATTR_ID_TEMP_BMC;
            break;
        case ONLP_THERMAL_FRONT:
            bmc_attr_id = BMC_ATTR_ID_TEMP_FRONT;
            break;
        case ONLP_THERMAL_PSU_0:
            bmc_attr_id = BMC_ATTR_ID_PSU0_TEMP;
            break;
        case ONLP_THERMAL_PSU_1:
            bmc_attr_id = BMC_ATTR_ID_PSU1_TEMP;
            break;
        default:
            bmc_attr_id = BMC_ATTR_ID_MAX;
    }

    if(bmc_attr_id == BMC_ATTR_ID_MAX) {
        return ONLP_STATUS_E_PARAM;
    }

    ONLP_TRY(bmc_sensor_read(bmc_attr_id, THERMAL_SENSOR, &data));

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

    switch (local_id) {
        case ONLP_THERMAL_CPU_PKG:
        case ONLP_THERMAL_CPU_0:
        case ONLP_THERMAL_CPU_1:
        case ONLP_THERMAL_CPU_2:
        case ONLP_THERMAL_CPU_3:
            ONLP_TRY(ufi_cpu_thermal_info_get(local_id, rv));
            break;
        case ONLP_THERMAL_MAC0:
        case ONLP_THERMAL_MAC1:
        case ONLP_THERMAL_DDR_L:
        case ONLP_THERMAL_DDR_R:
        case ONLP_THERMAL_BMC:
        case ONLP_THERMAL_FRONT:
        case ONLP_THERMAL_PSU_0:
        case ONLP_THERMAL_PSU_1:
            ONLP_TRY(ufi_bmc_thermal_info_get(local_id, rv));
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
    if(local_id == ONLP_THERMAL_PSU_0 || local_id == ONLP_THERMAL_PSU_1) {
        int psu_local_id = ONLP_PSU_MAX;

        if(local_id == ONLP_THERMAL_PSU_0) {
             psu_local_id = ONLP_PSU_0;
        } else {
             psu_local_id = ONLP_PSU_1;
        }

        int psu_present = 0;
        ONLP_TRY(get_psu_present_status(psu_local_id, &psu_present));
        if (psu_present == 0) {
            *rv = ONLP_THERMAL_STATUS_FAILED;
        } else if (psu_present == 1) {
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