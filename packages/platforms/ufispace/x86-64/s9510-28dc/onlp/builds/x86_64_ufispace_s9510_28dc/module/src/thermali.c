/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
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
 *            |----[01]ONLP_THERMAL_CPU_PKG
 *            |----[02]ONLP_THERMAL_CPU_0
 *            |----[03]ONLP_THERMAL_CPU_1
 *            |----[04]ONLP_THERMAL_CPU_2
 *            |----[05]ONLP_THERMAL_CPU_3
 *            |----[06]ONLP_THERMAL_CPU_4
 *            |----[07]ONLP_THERMAL_CPU_5
 *            |----[08]ONLP_THERMAL_CPU_6
 *            |----[09]ONLP_THERMAL_CPU_7
 *            |----[10]ONLP_THERMAL_MAC
 *            |----[11]ONLP_THERMAL_DDR4
 *            |----[12]ONLP_THERMAL_BMC
 *            |----[13]ONLP_THERMAL_FANCARD1
 *            |----[14]ONLP_THERMAL_FANCARD2
 *            |----[15]ONLP_THERMAL_FPGA_R
 *            |----[16]ONLP_THERMAL_FPGA_L
 *            |----[17]ONLP_THERMAL_HWM_GDDR
 *            |----[18]ONLP_THERMAL_HWM_MAC
 *            |----[19]ONLP_THERMAL_HWM_AMB
 *            |----[20]ONLP_THERMAL_HWM_NTMCARD
 *            |
 *            |----[01] ONLP_PSU_0----[21] ONLP_THERMAL_PSU_0
 *            |
 *            |----[02] ONLP_PSU_1----[22] ONLP_THERMAL_PSU_1
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
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(0, 102000, 105000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_0),
            .description = "CPU Thermal 0",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(0, 102000, 105000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_1),
            .description = "CPU Thermal 1",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(0, 102000, 105000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_2),
            .description = "CPU Thermal 2",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(0, 102000, 105000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_3),
            .description = "CPU Thermal 3",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(0, 102000, 105000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_4),
            .description = "CPU Thermal 4",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(0, 102000, 105000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_5),
            .description = "CPU Thermal 5",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(0, 102000, 105000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_6),
            .description = "CPU Thermal 6",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(0, 102000, 105000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_7),
            .description = "CPU Thermal 7",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(0, 102000, 105000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC),
            .description = "MAC Thermal",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(0, 100000, 110000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_DDR4),
            .description = "DDR4 Thermal",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(0, 101000, 106000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_BMC),
            .description = "BMC Thermal",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(0, 90000, 100000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_FANCARD1),
            .description = "FANCARD1 Thermal",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(0, 85000, 89000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_FANCARD2),
            .description = "FANCARD2 Thermal",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(0, 85000, 89000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_FPGA_R),
            .description = "FPGA_R Thermal",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(0, 100000, 110000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_FPGA_L),
            .description = "FPGA_L Thermal",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(0, 101000, 106000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_HWM_GDDR),
            .description = "HWM GDDR Thermal",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(0, 100000, 110000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_HWM_MAC),
            .description = "HWM MAC Thermal",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(0, 78000, 80000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_HWM_AMB),
            .description = "HWM AMB Thermal",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(0, 100000, 110000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_HWM_NTMCARD),
            .description = "NTMCARD Thermal",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(0, 100000, 110000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU_0),
            .description = "PSU-0-Thermal",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = (ONLP_THERMAL_CAPS_ALL),
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(80000, 85000, 90000),
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU_1),
            .description = "PSU-1-Thermal",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = (ONLP_THERMAL_CAPS_ALL),
        .mcelsius = 0,
        .thresholds = UFI_ONLP_THERMAL_THRESHOLD(80000, 85000, 90000),
    },
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

/**
 * @brief Update the information structure for the CPU thermal
 * @param id The Thermal Local ID
 * @param[out] info Receives the thermal information.
 */
static int update_thermali_cpu_info(int local_id, onlp_thermal_info_t* info)
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

/**
 * @brief Update the information structure for the thermal that got from BMC
 * @param id The Thermal Local ID
 * @param[out] info Receives the thermal information.
 */
static int update_thermali_from_bmc_info(int local_id, onlp_thermal_info_t* info)
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

    ONLP_TRY(bmc_sensor_read(bmc_attr_id, THERMAL_SENSOR, &data));
    info->mcelsius = (int) (data*1000);

    return ONLP_STATUS_OK;
}

/**
 * @brief Software initialization of the Thermal module.
 */
int onlp_thermali_sw_init(void)
{
    lock_init();
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
    int local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_oid_hdr_t */
    *hdr = __onlp_thermal_info[local_id].hdr;

    /* When the PSU module is unplugged, the psu thermal does not exist. */
    if(local_id == ONLP_THERMAL_PSU_0 || local_id == ONLP_THERMAL_PSU_1) {
        int psu_local_id = ONLP_PSU_MAX;

        if(local_id == ONLP_THERMAL_PSU_0) {
             psu_local_id = ONLP_PSU_0;
        } else {
             psu_local_id = ONLP_PSU_1;
        }

        int psu_present = 0;
        ONLP_TRY(get_psu_present_status(&psu_present, psu_local_id));
        if (psu_present == 0) {
            hdr->status = ONLP_OID_STATUS_FLAG_UNPLUGGED;
        } else if (psu_present == 1) {
            hdr->status = ONLP_OID_STATUS_FLAG_PRESENT;
        }
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

/**
 * @brief Get the information for the given thermal OID.
 * @param id The Thermal OID
 * @param[out] info Receives the thermal information.
 */
int onlp_thermali_info_get(onlp_oid_id_t id, onlp_thermal_info_t* info)
{
    int local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_thermal_info_t */
    memset(info, 0, sizeof(onlp_thermal_info_t));
    *info = __onlp_thermal_info[local_id];

    /* get hdr update */
    ONLP_TRY(onlp_thermali_hdr_get(id, &info->hdr));

    if (ONLP_OID_STATUS_FLAG_GET_VALUE(&info->hdr, PRESENT) == 0) {
        return ONLP_STATUS_OK;
    }

    switch (local_id) {
        case ONLP_THERMAL_CPU_PKG:
        case ONLP_THERMAL_CPU_0:
        case ONLP_THERMAL_CPU_1:
        case ONLP_THERMAL_CPU_2:
        case ONLP_THERMAL_CPU_3:
        case ONLP_THERMAL_CPU_4:
        case ONLP_THERMAL_CPU_5:
        case ONLP_THERMAL_CPU_6:
        case ONLP_THERMAL_CPU_7:
            ONLP_TRY(update_thermali_cpu_info(local_id, info));
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
            ONLP_TRY(update_thermali_from_bmc_info(local_id, info));
            break;
        default:
            return ONLP_STATUS_E_PARAM;
            break;
    }

    return ONLP_STATUS_OK;
}
