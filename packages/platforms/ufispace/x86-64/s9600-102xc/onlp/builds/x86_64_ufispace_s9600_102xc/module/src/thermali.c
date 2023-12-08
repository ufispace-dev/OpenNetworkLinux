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

 //FIXME threshold
static onlp_thermal_info_t thermal_info[] = {
    { }, /* Not used */
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PECI),
            .description = "TEMP_CPU_PECI",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV_CPU),
            .description = "TEMP_ENV_CPU",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV0),
            .description = "TEMP_ENV0",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV1),
            .description = "TEMP_ENV1",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV_MAC0),
            .description = "TEMP_ENV_MAC0",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC0),
            .description = "TEMP_MAC0",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU0),
            .description = "PSU 0 THERMAL 1",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU1),
            .description = "PSU 1 THERMAL 1",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PKG),
            .description = "CPU Package",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL)
    }
};

typedef enum thrm_attr_type_e {
    TYPE_THRM_ATTR_UNNKOW = 0,
    TYPE_THRM_ATTR_SYSFS,
    TYPE_THRM_ATTR_BMC,
    TYPE_THRM_ATTR_MAX,
} thrm_type_t;
typedef struct
{
    int type;
    int attr;
} thrm_attr_t;

static const thrm_attr_t thrm_attr[] = {
    /*                           thermal */
    [ONLP_THERMAL_CPU_PECI]     ={TYPE_THRM_ATTR_BMC  , BMC_ATTR_ID_TEMP_CPU_PECI},
    [ONLP_THERMAL_ENV_CPU]      ={TYPE_THRM_ATTR_BMC  , BMC_ATTR_ID_TEMP_ENV_CPU},
    [ONLP_THERMAL_ENV0]         ={TYPE_THRM_ATTR_BMC  , BMC_ATTR_ID_TEMP_ENV0},
    [ONLP_THERMAL_ENV1]         ={TYPE_THRM_ATTR_BMC  , BMC_ATTR_ID_TEMP_ENV1},
    [ONLP_THERMAL_ENV_MAC0]     ={TYPE_THRM_ATTR_BMC  , BMC_ATTR_ID_TEMP_ENV_MAC0},
    [ONLP_THERMAL_MAC0]         ={TYPE_THRM_ATTR_BMC  , BMC_ATTR_ID_TEMP_MAC0},
    [ONLP_THERMAL_PSU0]         ={TYPE_THRM_ATTR_BMC  , BMC_ATTR_ID_PSU0_TEMP},
    [ONLP_THERMAL_PSU1]         ={TYPE_THRM_ATTR_BMC  , BMC_ATTR_ID_PSU1_TEMP},
    [ONLP_THERMAL_CPU_PKG]      ={TYPE_THRM_ATTR_SYSFS, 1},

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
        case ONLP_THERMAL_CPU_PECI:
        case ONLP_THERMAL_ENV_CPU:
        case ONLP_THERMAL_ENV0:
        case ONLP_THERMAL_ENV1:
        case ONLP_THERMAL_ENV_MAC0:
        case ONLP_THERMAL_MAC0:
        case ONLP_THERMAL_PSU0:
        case ONLP_THERMAL_PSU1:
        case ONLP_THERMAL_CPU_PKG:
            *local_id = tmp_id;
            return ONLP_STATUS_OK;
        default:
            return ONLP_STATUS_E_INVALID;
    }
    return ONLP_STATUS_E_INVALID;
}

static int get_cpu_thermal_info(int local_id, onlp_thermal_info_t* info)
{
    int rv = 0;
    int attr = 0;

    *info = thermal_info[local_id];
    temp_thld_t temp_thld = {0};
    ONLP_TRY(get_thermal_thld(local_id, &temp_thld));
    info->thresholds.warning = MILLI(temp_thld.warning);
    info->thresholds.error = MILLI(temp_thld.error);
    info->thresholds.shutdown = MILLI(temp_thld.shutdown);

    /* present */
    info->status |= ONLP_THERMAL_STATUS_PRESENT;

    /* contents */
    if(info->status & ONLP_THERMAL_STATUS_PRESENT) {
        attr = thrm_attr[local_id].attr;
        rv = onlp_file_read_int(&info->mcelsius,
                                SYS_CPU_CORETEMP_PREFIX "temp%d_input", attr);

        if(rv < 0) {
            rv = onlp_file_read_int(&info->mcelsius,
                                SYS_CPU_CORETEMP_PREFIX2 "temp%d_input", attr);
            if(rv < 0) {
                return rv;
            }
        }
    }

    return ONLP_STATUS_OK;
}

static int get_bmc_thermal_info(int local_id, onlp_thermal_info_t* info)
{
    float data = 0;

    *info = thermal_info[local_id];
    temp_thld_t temp_thld = {0};
    ONLP_TRY(get_thermal_thld(local_id, &temp_thld));
    info->thresholds.warning = MILLI(temp_thld.warning);
    info->thresholds.error = MILLI(temp_thld.error);
    info->thresholds.shutdown = MILLI(temp_thld.shutdown);

    /* present */
    if(local_id == ONLP_THERMAL_PSU0 || local_id == ONLP_THERMAL_PSU1) {
        /* When the PSU module is unplugged, the psu thermal does not exist. */
        int psu_local_id = ONLP_PSU_MAX;

        if(local_id == ONLP_THERMAL_PSU0) {
             psu_local_id = ONLP_PSU_0;
        } else {
             psu_local_id = ONLP_PSU_1;
        }

        int psu_present = 0;
        ONLP_TRY(get_psu_present_status(psu_local_id, &psu_present));
        if (psu_present == PSU_STATUS_PRES) {
            info->status |= ONLP_THERMAL_STATUS_PRESENT;
        } else {
            info->status &= ~ONLP_THERMAL_STATUS_PRESENT;
        }
    }else{
        info->status |= ONLP_THERMAL_STATUS_PRESENT;
    }

    /* contents */
    if(info->status & ONLP_THERMAL_STATUS_PRESENT) {
        int bmc_attr = thrm_attr[local_id].attr;
        ONLP_TRY(read_bmc_sensor(bmc_attr, THERMAL_SENSOR, &data));

        if(BMC_ATTR_INVALID_VAL != (int)(data)) {
            info->status &= ~ONLP_THERMAL_STATUS_FAILED;
            info->mcelsius = (int) (data*1000);
        }else{
            info->status |= ONLP_THERMAL_STATUS_FAILED;
            info->mcelsius = 0;
        }
    }
    return ONLP_STATUS_OK;
}

/**
 * @brief Initialize the thermal subsystem.
 */
int onlp_thermali_init(void)
{
    init_lock();
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

    if(rv == NULL) {
        return ONLP_STATUS_E_PARAM;
    }

    ONLP_TRY(get_thermal_local_id(id, &local_id));

    /* update info  */
    if(thrm_attr[local_id].type == TYPE_THRM_ATTR_SYSFS)
        ONLP_TRY(get_cpu_thermal_info(local_id, rv));
    else
        ONLP_TRY(get_bmc_thermal_info(local_id, rv));

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
    onlp_thermal_info_t info ={0};

    ONLP_TRY(get_thermal_local_id(id, &local_id));

    if(thrm_attr[local_id].type == TYPE_THRM_ATTR_SYSFS)
        ONLP_TRY(get_cpu_thermal_info(local_id, &info));
    else
        ONLP_TRY(get_bmc_thermal_info(local_id, &info));

    *rv = info.status;

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

