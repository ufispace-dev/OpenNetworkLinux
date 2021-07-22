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


// FIXME look up the value
#define THERMAL_SHUTDOWN_DEFAULT  105000
#define THERMAL_ERROR_DEFAULT     95000
#define THERMAL_WARNING_DEFAULT   77000

/* Shortcut for CPU thermal threshold value. */
#define THERMAL_THRESHOLD_INIT_DEFAULTS  \
    { THERMAL_WARNING_DEFAULT, \
      THERMAL_ERROR_DEFAULT,   \
      THERMAL_SHUTDOWN_DEFAULT }
#define THERMAL_THRESHOLD_PSU          {65000, 70000, 75000}

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_THERMAL(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

#define THERMAL_INFO(id, desc, threshold) \
    { \
        { ONLP_THERMAL_ID_CREATE(id), #desc, POID_0 },\
        ONLP_THERMAL_STATUS_PRESENT,\
        ONLP_THERMAL_CAPS_ALL,\
        0,\
        threshold\
    }\

 //FIXME threshold
static onlp_thermal_info_t thermal_info[] = {
    { }, /* Not used */
    THERMAL_INFO(ONLP_THERMAL_CPU_PKG    , "CPU Package"      , THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_CPU_0      , "CPU Thermal 0"    , THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_CPU_1      , "CPU Thermal 1"    , THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_CPU_2      , "CPU Thermal 2"    , THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_CPU_3      , "CPU Thermal 3"    , THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_CPU_4      , "CPU Thermal 4"    , THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_CPU_5      , "CPU Thermal 5"    , THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_CPU_6      , "CPU Thermal 6"    , THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_CPU_7      , "CPU Thermal 7"    , THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_MAC        , "MAC Thermal"      , THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_DDR4       , "DDR4 Thermal"     , THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_BMC        , "BMC Thermal"      , THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_FANCARD1   , "FANCARD1 Thermal" , THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_FANCARD2   , "FANCARD2 Thermal" , THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_FPGA_R     , "FPGA_R Thermal"   , THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_FPGA_L     , "FPGA_L Thermal"   , THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_HWM_GDDR   , "HWM GDDR Thermal" , THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_HWM_MAC    , "HWM MAC Thermal"  , THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_HWM_AMB    , "HWM AMB Thermal"  , THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_HWM_NTMCARD, "NTMCARD Thermal"  , THERMAL_THRESHOLD_INIT_DEFAULTS),
    THERMAL_INFO(ONLP_THERMAL_PSU_0      , "PSU-0-Thermal"    , THERMAL_THRESHOLD_PSU),
    THERMAL_INFO(ONLP_THERMAL_PSU_1      , "PSU-1-Thermal"    , THERMAL_THRESHOLD_PSU),
};

int cpu_thermal_sysfs_id [] =
{
    [ONLP_THERMAL_CPU_PKG] = 1,
    [ONLP_THERMAL_CPU_0]   = 2,
    [ONLP_THERMAL_CPU_1]   = 4,
    [ONLP_THERMAL_CPU_2]   = 6,
    [ONLP_THERMAL_CPU_3]   = 8,
    [ONLP_THERMAL_CPU_4]   = 10,
    [ONLP_THERMAL_CPU_5]   = 12,
    [ONLP_THERMAL_CPU_6]   = 14,
    [ONLP_THERMAL_CPU_7]   = 16
};

static int ufi_cpu_thermal_info_get(onlp_thermal_info_t* info, int id)
{
    int rv;
    rv = onlp_file_read_int(&info->mcelsius,
                            SYS_CPU_CORETEMP_PREFIX "temp%d_input", cpu_thermal_sysfs_id[id]);

    if(rv < 0) {
        rv = onlp_file_read_int(&info->mcelsius,
                            SYS_CPU_CORETEMP_PREFIX2 "temp%d_input", cpu_thermal_sysfs_id[id]);
        if(rv < 0) {
            return rv;
        }
    }

    return ONLP_STATUS_OK;
}

int ufi_bmc_thermal_info_get(onlp_thermal_info_t* info, int id)
{
    int rc=0;
    float data=0;
    int bmc_attr_id = BMC_ATTR_ID_MAX;

    switch(id)
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
        case ONLP_THERMAL_FPGA_R:
            bmc_attr_id = BMC_ATTR_ID_TEMP_FPGA_R;
            break;
        case ONLP_THERMAL_FPGA_L:
            bmc_attr_id = BMC_ATTR_ID_TEMP_FPGA_L;
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

    rc = bmc_sensor_read(bmc_attr_id, THERMAL_SENSOR, &data);
    if ( rc < 0) {
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%d\n", id);
        return rc;
    }
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
    int sensor_id, rc;
    VALIDATE(id);

    sensor_id = ONLP_OID_ID_GET(id);

    *rv = thermal_info[sensor_id];

    switch (sensor_id) {
        case ONLP_THERMAL_CPU_PKG:
        case ONLP_THERMAL_CPU_0:
        case ONLP_THERMAL_CPU_1:
        case ONLP_THERMAL_CPU_2:
        case ONLP_THERMAL_CPU_3:
        case ONLP_THERMAL_CPU_4:
        case ONLP_THERMAL_CPU_5:
        case ONLP_THERMAL_CPU_6:
        case ONLP_THERMAL_CPU_7:
            rc = ufi_cpu_thermal_info_get(rv, sensor_id);
            break;
        case ONLP_THERMAL_MAC:
        case ONLP_THERMAL_DDR4:
        case ONLP_THERMAL_BMC:
        case ONLP_THERMAL_FANCARD1:
        case ONLP_THERMAL_FANCARD2:
        case ONLP_THERMAL_FPGA_R:
        case ONLP_THERMAL_FPGA_L:
        case ONLP_THERMAL_HWM_GDDR:
        case ONLP_THERMAL_HWM_MAC:
        case ONLP_THERMAL_HWM_AMB:
        case ONLP_THERMAL_HWM_NTMCARD:
        case ONLP_THERMAL_PSU_0:
        case ONLP_THERMAL_PSU_1:
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
    onlp_thermal_info_t* info;
    int thermal_id;
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

