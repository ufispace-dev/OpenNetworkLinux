/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2014 Accton Technology Corporation.
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

#define UFI_ONLP_THERMAL_THRESHOLD(WARNING_DEFAULT, ERROR_DEFAULT, SHUTDOWN_DEFAULT){ \
    WARNING_DEFAULT,                                                                  \
    ERROR_DEFAULT,                                                                    \
    SHUTDOWN_DEFAULT,                                                                 \
}

/**
 * Get all information about the given Thermal oid.
 *
 * [01] CHASSIS
 *            |----[01] ONLP_THERMAL_CPU_PKG
 *            |----[02] ONLP_THERMAL_CPU_0
 *            |----[03] ONLP_THERMAL_CPU_1
 *            |----[04] ONLP_THERMAL_CPU_2
 *            |----[05] ONLP_THERMAL_CPU_3
 *            |----[06] ONLP_THERMAL_CPU_4
 *            |----[07] ONLP_THERMAL_CPU_5
 *            |----[08] ONLP_THERMAL_CPU_6
 *            |----[09] ONLP_THERMAL_CPU_7
 *            |----[10] ONLP_THERMAL_ADC_CPU
 *            |----[11] ONLP_THERMAL_CPU_PECI 
 *            |----[12] ONLP_THERMAL_MAC_ENV_1
 *            |----[13] ONLP_THERMAL_MAC_ENV_2
 *            |----[14] ONLP_THERMAL_FRONT_ENV_1
 *            |----[15] ONLP_THERMAL_FRONT_ENV_2
 *            |----[16] ONLP_THERMAL_ENV_1
 *            |----[17] ONLP_THERMAL_ENV_2
 *            |----[18] ONLP_THERMAL_EXT_ENV_1
 *            |----[19] ONLP_THERMAL_EXT_ENV_2
 *            |
 *            |----[01] ONLP_PSU_0----[20] ONLP_THERMAL_PSU_0
 *            |
 *            |----[02] ONLP_PSU_1----[21] ONLP_THERMAL_PSU_1
 */
/* Static values */
static onlp_thermal_info_t __onlp_thermal_info[] = { 
    { }, /* Not used */    

    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PKG),
            .description = "CPU Package",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(77000, 95000, 105000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_0),
            .description = "CPU Thermal 0",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(77000, 95000, 105000),
    },    
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_1),
            .description = "CPU Thermal 1",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(77000, 95000, 105000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_2),
            .description = "CPU Thermal 2",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(77000, 95000, 105000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_3),
            .description = "CPU Thermal 3",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(77000, 95000, 105000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_4),
            .description = "CPU Thermal 4",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(77000, 95000, 105000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_5),
            .description = "CPU Thermal 5",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(77000, 95000, 105000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_6),
            .description = "CPU Thermal 6",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(77000, 95000, 105000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_7),
            .description = "CPU Thermal 7",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(77000, 95000, 105000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ADC_CPU),
            .description = "ADC_CPU_TEMP",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(85000, 95000, 100000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PECI),
            .description = "TEMP_CPU_PECI",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(85000, 95000, 100000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC_ENV_1),
            .description = "TEMP_MAC_ENV_1",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(40000, 48000, 46000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC_ENV_2),
            .description = "TEMP_MAC_ENV_2",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(40000, 48000, 46000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_FRONT_ENV_1),
            .description = "TEMP_FRONT_ENV_1",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(40000, 48000, 46000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_FRONT_ENV_2),
            .description = "TEMP_FRONT_ENV_2",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(40000, 48000, 46000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV_1),
            .description = "TEMP_ENV_1",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(40000, 48000, 46000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV_2),
            .description = "TEMP_ENV_2",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(40000, 48000, 46000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_EXT_ENV_1),
            .description = "TEMP_EXT_ENV_1",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(40000, 48000, 46000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_EXT_ENV_2),
            .description = "TEMP_EXT_ENV_2",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(40000, 48000, 46000),
    },    
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU_0),
            .description = "PSU-0-Thermal",
            .poid = ONLP_PSU_ID_CREATE(ONLP_PSU_0),
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(65000, 70000, 75000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU_1),
            .description = "PSU-1-Thermal",
            .poid = ONLP_PSU_ID_CREATE(ONLP_PSU_1),
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_THERMAL_CAPS_ALL,
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(65000, 70000, 75000),
    },            
};

/**
 * @brief Update the information structure for the CPU thermal
 * @param id The Thermal Local ID
 * @param[out] info Receives the thermal information.
 */
static int update_thermali_cpu_info(int local_id, onlp_thermal_info_t* info)
{
    int rv;
    
    rv = onlp_file_read_int(&info->mcelsius,
                            SYS_CPU_CORETEMP_PREFIX "temp%d_input", (local_id - ONLP_THERMAL_CPU_PKG) + 1);    
    
    if(rv < 0) {
        ONLP_TRY(onlp_file_read_int(&info->mcelsius,
                            SYS_CPU_CORETEMP_PREFIX2 "temp%d_input", (local_id - ONLP_THERMAL_CPU_PKG) + 1)); 
    }
    
    return ONLP_STATUS_OK;
}

/**
 * @brief Update the information structure for the thermal that got from BMC
 * @param id The Thermal Local ID
 * @param[out] info Receives the thermal information.
 */
static int update_thermali_from_bmc_info(int local_id, onlp_thermal_info_t* info)
{
    int rc=0;
    float data=0;
    
    rc = bmc_sensor_read(local_id + CACHE_OFFSET_THERMAL, THERMAL_SENSOR, &data);
    if ( rc < 0) {
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%d\n", local_id);
        return rc;
    }        
    info->mcelsius = (int) (data*1000);
        
    return rc;
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
int onlp_thermali_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* hdr)
{
    int ret = ONLP_STATUS_OK;
    int local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_oid_hdr_t */
    *hdr = __onlp_thermal_info[local_id].hdr;

    /* When the PSU module is unplugged, the psu thermal does not exist. */
    if (local_id == ONLP_THERMAL_PSU_0) {
        ret = get_psu_present_status(ONLP_PSU_0);
        if (ret == 0) {
            hdr->status = ONLP_OID_STATUS_FLAG_UNPLUGGED;
        } else if (ret == 1) {
            hdr->status = ONLP_OID_STATUS_FLAG_PRESENT;
        } else {
            return ONLP_STATUS_E_INTERNAL; 
        }
    } else if (local_id == ONLP_THERMAL_PSU_1) {
        ret = get_psu_present_status(ONLP_PSU_1);
        if (ret == 0) {
            hdr->status = ONLP_OID_STATUS_FLAG_UNPLUGGED;
        } else if (ret == 1) {
            hdr->status = ONLP_OID_STATUS_FLAG_PRESENT;
        } else {
            return ONLP_STATUS_E_INTERNAL; 
        }
    } else {
        //do nothing, all thermals are present as default setting.
    }

    return ret;
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
    int ret = ONLP_STATUS_OK;
    int local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_thermal_info_t */
    memset(info, 0, sizeof(onlp_thermal_info_t));
    *info = __onlp_thermal_info[local_id];
    ONLP_TRY(onlp_thermali_hdr_get(id, &info->hdr));

    switch (local_id) {
        case ONLP_THERMAL_CPU_PKG:
        case ONLP_THERMAL_CPU_0 ... ONLP_THERMAL_CPU_7:
            ret = update_thermali_cpu_info(local_id, info);
            break;
        case ONLP_THERMAL_ADC_CPU:
        case ONLP_THERMAL_CPU_PECI:
        case ONLP_THERMAL_MAC_ENV_1 ... ONLP_THERMAL_EXT_ENV_2:
        case ONLP_THERMAL_PSU_0 ... ONLP_THERMAL_PSU_1:
            ret = update_thermali_from_bmc_info(local_id, info);
            break;    
        default:    
            return ONLP_STATUS_E_PARAM;
            break;
    }   

    return ret;
}
