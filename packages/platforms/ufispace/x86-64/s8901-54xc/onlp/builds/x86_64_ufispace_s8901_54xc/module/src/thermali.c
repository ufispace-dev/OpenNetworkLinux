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
#define THERMAL_ERROR_DEFAULT     95000
#define THERMAL_WARNING_DEFAULT   77000

/* Shortcut for CPU thermal threshold value. */
#define THERMAL_THRESHOLD_INIT_DEFAULTS  \
    { THERMAL_WARNING_DEFAULT, \
      THERMAL_ERROR_DEFAULT,   \
      THERMAL_SHUTDOWN_DEFAULT }
#define THERMAL_THRESHOLD_MAC          {95000, 100000, 105000}
#define THERMAL_THRESHOLD_MAC_HWM      {95000, 100000, 105000}
#define THERMAL_THRESHOLD_ENV_MACCASE  {80000, 85000, 90000}
#define THERMAL_THRESHOLD_ENV_PSUCASE  {75000, 80000, 85000}
#define THERMAL_THRESHOLD_ENV_FANCONN  {75000, 80000, 85000}
#define THERMAL_THRESHOLD_ENV_FANCARD  {75000, 80000, 85000}
#define THERMAL_THRESHOLD_ENV_BMC      {80000, 85000, 90000}
#define THERMAL_THRESHOLD_PSU_TEMP     {65000, 70000, 75000}



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
    THERMAL_INFO(ONLP_THERMAL_MAC, "TEMP_MAC", THERMAL_THRESHOLD_MAC),
    THERMAL_INFO(ONLP_THERMAL_MAC_HWM, "TEMP_MAC_HWM", THERMAL_THRESHOLD_MAC_HWM),
    THERMAL_INFO(ONLP_THERMAL_ENV_MACCASE, "TEMP_ENV_MACCASE", THERMAL_THRESHOLD_ENV_MACCASE),
    THERMAL_INFO(ONLP_THERMAL_ENV_PSUCASE, "TEMP_ENV_PSUCASE", THERMAL_THRESHOLD_ENV_PSUCASE),
    THERMAL_INFO(ONLP_THERMAL_ENV_FANCONN, "TEMP_ENV_FANCONN", THERMAL_THRESHOLD_ENV_FANCARD),
    THERMAL_INFO(ONLP_THERMAL_ENV_FANCARD, "TEMP_ENV_FANCARD", THERMAL_THRESHOLD_ENV_FANCARD),
    THERMAL_INFO(ONLP_THERMAL_ENV_BMC, "TEMP_ENV_BMC", THERMAL_THRESHOLD_ENV_BMC),
    THERMAL_INFO(ONLP_THERMAL_PSU_0, "PSU-0-Thermal", THERMAL_THRESHOLD_PSU_TEMP),
    THERMAL_INFO(ONLP_THERMAL_PSU_1, "PSU-1-Thermal", THERMAL_THRESHOLD_PSU_TEMP),
};

static int ufi_cpu_thermal_info_get(int id, onlp_thermal_info_t* info)
{
    int rv = 0;

    rv = onlp_file_read_int(&info->mcelsius,
                            SYS_CPU_CORETEMP_PREFIX "temp%d_input", (id - ONLP_THERMAL_CPU_PKG) + 1);

    if(rv < 0) {
        rv = onlp_file_read_int(&info->mcelsius,
                            SYS_CPU_CORETEMP_PREFIX2 "temp%d_input", (id - ONLP_THERMAL_CPU_PKG) + 1);
        if(rv < 0) {
            return rv;
        }
    }

    return ONLP_STATUS_OK;
}

int ufi_bmc_thermal_info_get(onlp_thermal_info_t* info, int id)
{
    int rc = 0;
    float data = 0;
    int bmc_attr_id = BMC_ATTR_ID_MAX;

    switch(id)
    {
        case ONLP_THERMAL_MAC:
            bmc_attr_id = BMC_ATTR_ID_TEMP_MAC;
            break;
        case ONLP_THERMAL_MAC_HWM:
            bmc_attr_id = BMC_ATTR_ID_TEMP_MAC_HWM;
            break;
        case ONLP_THERMAL_ENV_MACCASE:
            bmc_attr_id = BMC_ATTR_ID_TEMP_ENV_MACCASE;
            break;
        case ONLP_THERMAL_ENV_PSUCASE:
            bmc_attr_id = BMC_ATTR_ID_TEMP_ENV_PSUCASE;
            break;
        case ONLP_THERMAL_ENV_FANCONN:
            bmc_attr_id = BMC_ATTR_ID_TEMP_ENV_FANCONN;
            break;
        case ONLP_THERMAL_ENV_FANCARD:
            bmc_attr_id = BMC_ATTR_ID_TEMP_ENV_FANCARD;
            break;
        case ONLP_THERMAL_ENV_BMC:
            bmc_attr_id = BMC_ATTR_ID_TEMP_ENV_BMC;
            break;
        case ONLP_THERMAL_PSU_0:
            bmc_attr_id = BMC_ATTR_ID_PSU0_TEMP1;
            break;
        case ONLP_THERMAL_PSU_1:
            bmc_attr_id = BMC_ATTR_ID_PSU1_TEMP1;
            break;
        default:
            bmc_attr_id = BMC_ATTR_ID_MAX;
    }

    if(bmc_attr_id == BMC_ATTR_ID_MAX) {
        return ONLP_STATUS_E_PARAM;
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
    int sensor_id = 0, rc = 0;
    VALIDATE(id);

    sensor_id = ONLP_OID_ID_GET(id);

    *rv = thermal_info[sensor_id];

    switch (sensor_id) {
        case ONLP_THERMAL_CPU_PKG:
            rc = ufi_cpu_thermal_info_get(sensor_id, rv);
            break;
        case ONLP_THERMAL_MAC ... ONLP_THERMAL_PSU_1:
            rc = ufi_bmc_thermal_info_get(rv, sensor_id);
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
    VALIDATE(id);

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
    int result = ONLP_STATUS_OK;
    onlp_thermal_info_t* info = NULL;
    int thermal_id = 0;
    VALIDATE(id);

    thermal_id = ONLP_OID_ID_GET(id);
    if(thermal_id >= ONLP_THERMAL_MAX) {
        result = ONLP_STATUS_E_INVALID;
    } else {
        info = &thermal_info[thermal_id];
        *rv = info->hdr;
    }
    return result;
}

/**
 * @brief Generic ioctl.
 */
int onlp_thermali_ioctl(int id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}
