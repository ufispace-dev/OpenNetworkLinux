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

#define MILLI(cel) (cel * 1000)

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
 *            |----[02]ONLP_THERMAL_MAC
 *            |----[03]ONLP_THERMAL_DDR4
 *            |----[04]ONLP_THERMAL_BMC
 *            |----[05]ONLP_THERMAL_FANCARD1
 *            |----[06]ONLP_THERMAL_FANCARD2
 *            |----[07]ONLP_THERMAL_FPGA_R (PREMIUM EXTENDED)
 *            |----[08]ONLP_THERMAL_FPGA_L (PREMIUM EXTENDED)
 *            |----[09]ONLP_THERMAL_HWM_GDDR
 *            |----[10]ONLP_THERMAL_HWM_MAC
 *            |----[11]ONLP_THERMAL_HWM_AMB
 *            |----[12]ONLP_THERMAL_HWM_NTMCARD
 *            |
 *            |----[01] ONLP_PSU_0----[13] ONLP_THERMAL_PSU_0
 *            |
 *            |----[02] ONLP_PSU_1----[14] ONLP_THERMAL_PSU_1
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
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD)
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
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD)
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
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD)
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
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD)
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
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD)
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
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD)
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
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD)
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
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD)
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
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD)
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
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD)
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
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD)
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
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU_0),
            .description = "PSU-0-Thermal",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = (ONLP_THERMAL_CAPS_ALL)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU_1),
            .description = "PSU-1-Thermal",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = (ONLP_THERMAL_CAPS_ALL)
    },
};

/**
 * @brief Get and check thermal local ID
 * @param id [in] OID
 * @param local_id [out] The thermal local id
 */
static int get_thermal_local_id(int id, int *local_id)
{
    int tmp_id;
    board_t board = {0};

    if(local_id == NULL) {
        return ONLP_STATUS_E_PARAM;
    }

    if((ONLP_OID_TYPE_GET(id) != 0) && !ONLP_OID_IS_THERMAL(id)) {
        return ONLP_STATUS_E_INVALID;
    }

    tmp_id = ONLP_OID_ID_GET(id);
    ONLP_TRY(ufi_get_board_version(&board));

    if(board.rev_id == HW_EXT_ID_PREMIUM_EXT) {
        switch (tmp_id) {
            case ONLP_THERMAL_CPU_PKG:
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
                *local_id = tmp_id;
                return ONLP_STATUS_OK;
            default:
                return ONLP_STATUS_E_INVALID;
        }
    } else if(board.rev_id == HW_EXT_ID_STANDARD) {
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
    } else if(board.rev_id == HW_EXT_ID_PREMIUM) {
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
    } else {
        return ONLP_STATUS_E_INVALID;
    }

    return ONLP_STATUS_E_INVALID;
}


/**
 * @brief Update the information structure for the CPU thermal
 * @param id The Thermal Local ID
 * @param[out] info Receives the thermal information.
 */
static int update_thermali_cpu_info(int local_id, onlp_thermal_info_t* info)
{
    int rv = 0;
    char *sysfs = NULL;

    switch(local_id) {
        case ONLP_THERMAL_CPU_PKG:
            sysfs = CPU_PKG_CORE_TEMP_SYS_ID;
            break;
        default:
            return ONLP_STATUS_E_PARAM;
    }

    rv = onlp_file_read_int(&info->mcelsius,
                            SYS_CPU_CORETEMP_PREFIX "temp%s_input", sysfs);

    if(rv < 0) {
        rv = onlp_file_read_int(&info->mcelsius,
                            SYS_CPU_CORETEMP_PREFIX2 "temp%s_input", sysfs);
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
    float data = 0;
    int bmc_attr = BMC_ATTR_ID_INVALID;

    switch(local_id)
    {
        case ONLP_THERMAL_MAC:
            bmc_attr = BMC_ATTR_ID_TEMP_MAC;
            break;
        case ONLP_THERMAL_DDR4:
            bmc_attr = BMC_ATTR_ID_TEMP_DDR4;
            break;
        case ONLP_THERMAL_BMC:
            bmc_attr = BMC_ATTR_ID_TEMP_BMC;
            break;
        case ONLP_THERMAL_FANCARD1:
            bmc_attr = BMC_ATTR_ID_TEMP_FANCARD1;
            break;
        case ONLP_THERMAL_FANCARD2:
            bmc_attr = BMC_ATTR_ID_TEMP_FANCARD2;
            break;
        case ONLP_THERMAL_FPGA_R:
            bmc_attr = BMC_ATTR_ID_TEMP_FPGA_R;
            break;
        case ONLP_THERMAL_FPGA_L:
            bmc_attr = BMC_ATTR_ID_TEMP_FPGA_L;
            break;
        case ONLP_THERMAL_HWM_GDDR:
            bmc_attr = BMC_ATTR_ID_HWM_TEMP_GDDR;
            break;
        case ONLP_THERMAL_HWM_MAC:
            bmc_attr = BMC_ATTR_ID_HWM_TEMP_MAC;
            break;
        case ONLP_THERMAL_HWM_AMB:
            bmc_attr = BMC_ATTR_ID_HWM_TEMP_AMB;
            break;
        case ONLP_THERMAL_HWM_NTMCARD:
            bmc_attr = BMC_ATTR_ID_HWM_TEMP_NTMCARD;
            break;
        case ONLP_THERMAL_PSU_0:
            bmc_attr = BMC_ATTR_ID_PSU0_TEMP;
            break;
        case ONLP_THERMAL_PSU_1:
            bmc_attr = BMC_ATTR_ID_PSU1_TEMP;
            break;
        default:
            return ONLP_STATUS_E_PARAM;
    }

    ONLP_TRY(bmc_sensor_read(bmc_attr, THERMAL_SENSOR, &data));
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
    int local_id = 0;
    if(get_thermal_local_id(id, &local_id) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INVALID;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Retrieve the thermal's oid header.
 * @param id The thermal oid.
 * @param[out] hdr Receives the header.
 */
int onlp_thermali_hdr_get(onlp_oid_id_t id, onlp_oid_hdr_t* hdr)
{
    int local_id = 0;
    ONLP_TRY(get_thermal_local_id(id, &local_id));

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
        ONLP_TRY(get_psu_present_status(psu_local_id, &psu_present));
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
    int local_id = 0;
    ONLP_TRY(get_thermal_local_id(id, &local_id));

    /* Set the onlp_thermal_info_t */
    memset(info, 0, sizeof(onlp_thermal_info_t));
    *info = __onlp_thermal_info[local_id];

    temp_thld_t temp_thld = {0};
    ONLP_TRY(ufi_get_thermal_thld(local_id, &temp_thld));
    info->thresholds.warning = MILLI(temp_thld.warning);
    info->thresholds.error = MILLI(temp_thld.error);
    info->thresholds.shutdown = MILLI(temp_thld.shutdown);

    /* get hdr update */
    ONLP_TRY(onlp_thermali_hdr_get(id, &info->hdr));

    if (ONLP_OID_STATUS_FLAG_GET_VALUE(&info->hdr, PRESENT) == 0) {
        return ONLP_STATUS_OK;
    }

    switch (local_id) {
        case ONLP_THERMAL_CPU_PKG:
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
