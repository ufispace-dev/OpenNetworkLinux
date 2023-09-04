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

#define MILLI(cel) (cel * 1000)

#define THERMAL_SHUTDOWN_DEFAULT    105
#define THERMAL_ERROR_DEFAULT       95
#define THERMAL_WARNING_DEFAULT     77


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
        .caps = (ONLP_THERMAL_CAPS_ALL),
        .mcelsius = 0,
        .thresholds = {MILLI(THERMAL_WARNING_DEFAULT), MILLI(THERMAL_ERROR_DEFAULT), MILLI(THERMAL_SHUTDOWN_DEFAULT)}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC),
            .description = "MAC Thermal",
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
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_DDR4),
            .description = "DDR4 Thermal",
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
        .thresholds = {MILLI(0), MILLI(85), MILLI(95)}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_FANCARD1),
            .description = "FANCARD1 Thermal",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {MILLI(0), MILLI(85), MILLI(89)}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_FANCARD2),
            .description = "FANCARD2 Thermal",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {MILLI(0), MILLI(85), MILLI(89)}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_HWM_GDDR),
            .description = "HWM GDDR Thermal",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {MILLI(0), MILLI(91), MILLI(100)}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_HWM_MAC),
            .description = "HWM MAC Thermal",
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
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_HWM_AMB),
            .description = "HWM AMB Thermal",
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
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_HWM_NTMCARD),
            .description = "NTMCARD Thermal",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {MILLI(0), MILLI(91), MILLI(100)}
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

int cpu_thermal_sysfs_id [] =
{
    [ONLP_THERMAL_CPU_PKG] = 1,
};

/**
 * @brief Get and check thermal local ID
 * @param id [in] OID
 * @param local_id [out] The thermal local id
 */
static int get_thermal_local_id(int id, int *local_id)
{
    int tmp_id;

    if(local_id == NULL) {
        return ONLP_STATUS_E_PARAM;
    }

    if(!ONLP_OID_IS_THERMAL(id)) {
        return ONLP_STATUS_E_INVALID;
    }

    tmp_id = ONLP_OID_ID_GET(id);
    switch (tmp_id) {
        case ONLP_THERMAL_CPU_PKG:
        case ONLP_THERMAL_MAC:
        case ONLP_THERMAL_DDR4:
        case ONLP_THERMAL_BMC:
        case ONLP_THERMAL_FANCARD1:
        case ONLP_THERMAL_FANCARD2:
        case ONLP_THERMAL_HWM_GDDR:
        case ONLP_THERMAL_HWM_MAC:
        case ONLP_THERMAL_HWM_AMB:
        case ONLP_THERMAL_HWM_NTMCARD:
        case ONLP_THERMAL_PSU_0:
        case ONLP_THERMAL_PSU_1:
            *local_id = tmp_id;
            return ONLP_STATUS_OK;
        default:
            return ONLP_STATUS_E_INVALID;
    }

    return ONLP_STATUS_E_INVALID;
}

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

static int ufi_bmc_thermal_info_get(int local_id, onlp_thermal_info_t* info)
{
    float data=0;
    int bmc_attr_id = BMC_ATTR_ID_MAX;

    switch(local_id)
    {
        case ONLP_THERMAL_MAC:
            bmc_attr_id = BMC_ATTR_ID_TEMP_MAC;
            break;
        case ONLP_THERMAL_DDR4:
            bmc_attr_id = BMC_ATTR_ID_TEMP_DDR4;
            break;
        case ONLP_THERMAL_BMC:
            bmc_attr_id = BMC_ATTR_ID_TEMP_BMC;
            break;
        case ONLP_THERMAL_FANCARD1:
            bmc_attr_id = BMC_ATTR_ID_TEMP_FANCARD1;
            break;
        case ONLP_THERMAL_FANCARD2:
            bmc_attr_id = BMC_ATTR_ID_TEMP_FANCARD2;
            break;
        case ONLP_THERMAL_HWM_GDDR:
            bmc_attr_id = BMC_ATTR_ID_HWM_TEMP_GDDR;
            break;
        case ONLP_THERMAL_HWM_MAC:
            bmc_attr_id = BMC_ATTR_ID_HWM_TEMP_MAC;
            break;
        case ONLP_THERMAL_HWM_AMB:
            bmc_attr_id = BMC_ATTR_ID_HWM_TEMP_AMB;
            break;
        case ONLP_THERMAL_HWM_NTMCARD:
            bmc_attr_id = BMC_ATTR_ID_HWM_TEMP_NTMCARD;
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

    ONLP_TRY(get_thermal_local_id(id, &local_id));
    *rv = thermal_info[local_id];

    /* update status  */
    ONLP_TRY(onlp_thermali_status_get(id, &rv->status));

    if((rv->status & ONLP_THERMAL_STATUS_PRESENT) == 0) {
        return ONLP_STATUS_OK;
    }

    switch (local_id) {
        case ONLP_THERMAL_CPU_PKG:
            ONLP_TRY(ufi_cpu_thermal_info_get(local_id, rv));
            break;
        case ONLP_THERMAL_MAC:
        case ONLP_THERMAL_DDR4:
        case ONLP_THERMAL_BMC:
        case ONLP_THERMAL_FANCARD1:
        case ONLP_THERMAL_FANCARD2:
        case ONLP_THERMAL_HWM_GDDR:
        case ONLP_THERMAL_HWM_MAC:
        case ONLP_THERMAL_HWM_AMB:
        case ONLP_THERMAL_HWM_NTMCARD:
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

    ONLP_TRY(get_thermal_local_id(id, &local_id));
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

    ONLP_TRY(get_thermal_local_id(id, &local_id));
    *rv = thermal_info[local_id].hdr;

    return ONLP_STATUS_OK;
}

/**
 * @brief Generic ioctl.
 */
int onlp_thermali_ioctl(int id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

