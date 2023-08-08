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

#define THERMAL_SHUTDOWN_DEFAULT  105000
#define THERMAL_ERROR_DEFAULT         95000
#define THERMAL_WARNING_DEFAULT     77000

/* Shortcut for CPU thermal threshold value. */
#define THERMAL_THRESHOLD_INIT_DEFAULTS  \
    { THERMAL_WARNING_DEFAULT, \
      THERMAL_ERROR_DEFAULT,   \
      THERMAL_SHUTDOWN_DEFAULT }
#define THERMAL_THRESHOLD_CPU_PECI {85000, 95000, 100000}
#define THERMAL_THRESHOLD_PSU          {65000, 70000, 75000}
#define THERMAL_THRESHOLD_ENV          {60000, 65000, 70000}
#define THERMAL_THRESHOLD_MAC_PVT  {90000, 95000, 105000}
#define THERMAL_THRESHOLD_MAC_HBM {85000, 90000, 95000}
#define THERMAL_THRESHOLD_MAC_ENV  {65000, 70000, 80000}
#define THERMAL_THRESHOLD_MAC_DIE  {100000, 105000, 110000}



#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_THERMAL(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

#define THERMAL_INFO(id, desc, threshold) \
    { \
        { ONLP_THERMAL_ID_CREATE(id), desc, POID_0 },\
        ONLP_THERMAL_STATUS_PRESENT,\
        ONLP_THERMAL_CAPS_ALL,\
        0,\
        threshold\
    }\

//FIXME
static onlp_thermal_info_t thermal_info[] = {
    { }, /* Not used */
    THERMAL_INFO(ONLP_THERMAL_CPU_PKG, "CPU Package", THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_CPU_0, "CPU Thermal 0", THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_CPU_1, "CPU Thermal 1", THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_CPU_2, "CPU Thermal 2", THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_CPU_3, "CPU Thermal 3", THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_CPU_4, "CPU Thermal 4", THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_CPU_5, "CPU Thermal 5", THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_CPU_6, "CPU Thermal 6", THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_CPU_7, "CPU Thermal 7", THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_ENV_CPU, "TEMP_ENV_CPU", THERMAL_THRESHOLD_CPU_PECI),
    THERMAL_INFO(ONLP_THERMAL_CPU_PECI, "TEMP_CPU_PECI", THERMAL_THRESHOLD_CPU_PECI),
    THERMAL_INFO(ONLP_THERMAL_ENV_MAC0, "TEMP_ENV_MAC0", THERMAL_THRESHOLD_MAC_ENV),
    THERMAL_INFO(ONLP_THERMAL_MAC0, "TEMP_MAC0", THERMAL_THRESHOLD_MAC_DIE),
    THERMAL_INFO(ONLP_THERMAL_ENV_MAC1, "TEMP_ENV_MAC1", THERMAL_THRESHOLD_MAC_ENV),
    THERMAL_INFO(ONLP_THERMAL_MAC1, "TEMP_MAC1", THERMAL_THRESHOLD_MAC_DIE),
    THERMAL_INFO(ONLP_THERMAL_PSU_0, "PSU-0-Thermal", THERMAL_THRESHOLD_PSU),
    THERMAL_INFO(ONLP_THERMAL_PSU_1, "PSU-1-Thermal", THERMAL_THRESHOLD_PSU),
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
        case ONLP_THERMAL_CPU_0:
        case ONLP_THERMAL_CPU_1:
        case ONLP_THERMAL_CPU_2:
        case ONLP_THERMAL_CPU_3:
        case ONLP_THERMAL_CPU_4:
        case ONLP_THERMAL_CPU_5:
        case ONLP_THERMAL_CPU_6:
        case ONLP_THERMAL_CPU_7:
        case ONLP_THERMAL_ENV_CPU:
        case ONLP_THERMAL_CPU_PECI:
        case ONLP_THERMAL_ENV_MAC0:
        case ONLP_THERMAL_MAC0:
        case ONLP_THERMAL_ENV_MAC1:
        case ONLP_THERMAL_MAC1:
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
    int rv = 0;

    rv = onlp_file_read_int(&info->mcelsius,
                            SYS_CPU_CORETEMP_PREFIX "temp%d_input", (local_id - ONLP_THERMAL_CPU_PKG) + 1);

    if(rv < 0) {
        rv = onlp_file_read_int(&info->mcelsius,
                            SYS_CPU_CORETEMP_PREFIX2 "temp%d_input", (local_id - ONLP_THERMAL_CPU_PKG) + 1);
        if(rv < 0) {
            return rv;
        }
    }

    return ONLP_STATUS_OK;
}

static int ufi_bmc_thermal_info_get(int local_id, onlp_thermal_info_t* info)
{
    int rc = 0;
    float data = 0;
    int bmc_attr_id = BMC_ATTR_ID_MAX;

    switch(local_id)
    {
        case ONLP_THERMAL_ENV_CPU:
            bmc_attr_id = BMC_ATTR_ID_TEMP_ENV_CPU;
            break;
        case ONLP_THERMAL_CPU_PECI:
            bmc_attr_id = BMC_ATTR_ID_TEMP_CPU_PECI;
            break;
        case ONLP_THERMAL_ENV_MAC0:
            bmc_attr_id = BMC_ATTR_ID_TEMP_ENV_MAC0;
            break;
        case ONLP_THERMAL_MAC0:
            bmc_attr_id = BMC_ATTR_ID_TEMP_MAC0;
            break;
        case ONLP_THERMAL_ENV_MAC1:
            bmc_attr_id = BMC_ATTR_ID_TEMP_ENV_MAC1;
            break;
        case ONLP_THERMAL_MAC1:
            bmc_attr_id = BMC_ATTR_ID_TEMP_MAC1;
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

    if (local_id >= ONLP_THERMAL_PSU_0 && local_id <= ONLP_THERMAL_PSU_1) {
        //check presence for psu
        int pw_present, psu_id;

        switch (local_id) {
            case ONLP_THERMAL_PSU_0:
                psu_id = ONLP_PSU_0;
                break;
            case ONLP_THERMAL_PSU_1:
                psu_id = ONLP_PSU_1;
                break;
            default:
                return ONLP_STATUS_E_INVALID;
        }
        ONLP_TRY(get_psu_present_status(psu_id, &pw_present));

        //update psu thermal presence by psu presence status
        if(pw_present == 1) {
            info->status |= ONLP_THERMAL_STATUS_PRESENT;
        } else {
            info->status &= ~ONLP_THERMAL_STATUS_PRESENT ;
            return ONLP_STATUS_OK;
        }
    }

    ONLP_TRY(bmc_sensor_read(bmc_attr_id, THERMAL_SENSOR, &data));
    info->mcelsius = (int) (data*1000);

    return rc;
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
    int rc = 0;
    int local_id;

    if(rv == NULL) {
        return ONLP_STATUS_E_PARAM;
    }

    ONLP_TRY(get_thermal_local_id(id, &local_id));
    *rv = thermal_info[local_id];

    switch (local_id) {
        case ONLP_THERMAL_CPU_PKG ... ONLP_THERMAL_CPU_7:
            rc = ufi_cpu_thermal_info_get(local_id, rv);
            break;
        case ONLP_THERMAL_ENV_CPU ... ONLP_THERMAL_PSU_1:
            rc = ufi_bmc_thermal_info_get(local_id, rv);
            break;
        default:
            return ONLP_STATUS_E_INTERNAL;
            break;
    }

    return rc;
}

/**
 * @brief Retrieve the thermal's operational status.
 * @param id The thermal oid.
 * @param rv [out] Receives the operational status.
 */
int onlp_thermali_status_get(onlp_oid_t id, uint32_t* rv)
{
    int result = ONLP_STATUS_OK;
    onlp_thermal_info_t info;

    result = onlp_thermali_info_get(id, &info);
    *rv = info.status;

    return result;
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
