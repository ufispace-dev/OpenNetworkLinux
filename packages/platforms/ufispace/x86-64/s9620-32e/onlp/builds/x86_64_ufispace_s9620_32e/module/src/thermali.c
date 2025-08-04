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

#define MILLI(cel)         (cel * 1000)
#define IS_SYSFS(_node)    (_node.type == TYPE_THRM_ATTR_GENERAL_SYSFS || _node.type == TYPE_THRM_ATTR_PSU_SYSFS || TYPE_THRM_ATTR_CPU_SYSFS)
#define IS_BMC(_node)      (_node.type == TYPE_THRM_ATTR_GENERAL_BMC || _node.type == TYPE_THRM_ATTR_PSU_BMC)
#define IS_GENERAL(_node)  (_node.type == TYPE_THRM_ATTR_GENERAL_SYSFS || _node.type == TYPE_THRM_ATTR_GENERAL_BMC)
#define IS_CPU(_node)      (_node.type == TYPE_THRM_ATTR_CPU_SYSFS)
#define IS_PSU(_node)      (_node.type == TYPE_THRM_ATTR_PSU_SYSFS || _node.type == TYPE_THRM_ATTR_PSU_BMC)

/* Thermal threshold */
#define THERMAL_WARNING_DEFAULT               77
#define THERMAL_ERROR_DEFAULT                 95
#define THERMAL_SHUTDOWN_DEFAULT              105
#define THERMAL_STATE_NOT_SUPPORT             -273

typedef struct
{
    int local_id;
    int type;
    int temp_idx;
    int bmc;
    int parent;
} thrm_node_t;

typedef enum thrm_attr_type_e {
    TYPE_THRM_ATTR_UNNKOW = 0,
    TYPE_THRM_ATTR_GENERAL_SYSFS,
    TYPE_THRM_ATTR_GENERAL_BMC,
    TYPE_THRM_ATTR_CPU_SYSFS,
    TYPE_THRM_ATTR_PSU_SYSFS,
    TYPE_THRM_ATTR_PSU_BMC,
    TYPE_THRM_ATTR_MAX,
} thrm_type_t;

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
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV2),
            .description = "TEMP_ENV2",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV3),
            .description = "TEMP_ENV3",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV4),
            .description = "TEMP_ENV4",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV5),
            .description = "TEMP_ENV5",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV_EXT0),
            .description = "TEMP_ENV_EXT0",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV_FAN0),
            .description = "TEMP_ENV_FAN0",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV_FAN1),
            .description = "TEMP_ENV_FAN1",
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
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV_MAC1),
            .description = "TEMP_ENV_MAC1",
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
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC1),
            .description = "TEMP_MAC1",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU0_TEMP1),
            .description = "PSU 0 THERMAL 1",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE | ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD | ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU1_TEMP1),
            .description = "PSU 1 THERMAL 1",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE | ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD | ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PKG),
            .description = "CPU Package",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL),
        .thresholds = {
            .warning = MILLI(THERMAL_WARNING_DEFAULT),
            .error = MILLI(THERMAL_ERROR_DEFAULT),
            .shutdown = MILLI(THERMAL_SHUTDOWN_DEFAULT),
        }
    },
};

static int get_node(int id, thrm_node_t *node) {
    board_t board = {0};
    if(node == NULL) {
        return ONLP_STATUS_E_PARAM;
    }

    if(!ONLP_OID_IS_THERMAL(id)) {
        return ONLP_STATUS_E_INVALID;
    }

    ONLP_TRY(get_board_version(&board));

    node->local_id = ONLP_OID_ID_GET(id);
    switch(node->local_id) {
        case ONLP_THERMAL_CPU_PECI:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_CPU_PECI;
            break;
        case ONLP_THERMAL_ENV_CPU:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_ENV_CPU;
            break;
        case ONLP_THERMAL_ENV0:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_ENV0;
            break;
        case ONLP_THERMAL_ENV1:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_ENV1;
            break;
        case ONLP_THERMAL_ENV2:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_ENV2;
            break;
        case ONLP_THERMAL_ENV3:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_ENV3;
            break;
        case ONLP_THERMAL_ENV4:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_ENV4;
            break;
        case ONLP_THERMAL_ENV5:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_ENV5;
            break;
        case ONLP_THERMAL_ENV_EXT0:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_ENV_EXT0;
            break;
        case ONLP_THERMAL_ENV_FAN0:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_ENV_FAN0;
            break;
        case ONLP_THERMAL_ENV_FAN1:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_ENV_FAN1;
            break;
        case ONLP_THERMAL_ENV_MAC0:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_ENV_MAC0;
            break;
        case ONLP_THERMAL_ENV_MAC1:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_ENV_MAC1;
            break;
        case ONLP_THERMAL_MAC0:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_MAC0;
            break;
        case ONLP_THERMAL_MAC1:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_MAC1;
            break;
        case ONLP_THERMAL_PSU0_TEMP1:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_PSU0_TEMP1;
            break;
        case ONLP_THERMAL_PSU1_TEMP1:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_PSU1_TEMP1;
            break;
        case ONLP_THERMAL_CPU_PKG:
            node->type = TYPE_THRM_ATTR_CPU_SYSFS;
            node->temp_idx = 1;
            break;
        default:
             return ONLP_STATUS_E_INVALID;
    }

    return ONLP_STATUS_OK;
}

static int get_cpu_thermal_info(thrm_node_t node, onlp_thermal_info_t* info)
{
    int rv = 0;

    if(info == NULL) {
        return ONLP_STATUS_E_PARAM;
    }

    *info = thermal_info[node.local_id];

    /* present */
    info->status |= ONLP_THERMAL_STATUS_PRESENT;

    /* contents */
    if(info->status & ONLP_THERMAL_STATUS_PRESENT) {
        rv = onlp_file_read_int(&info->mcelsius,
                                SYS_CPU_CORETEMP_PREFIX "temp%d_input", node.temp_idx);

        if(rv < 0) {
            rv = onlp_file_read_int(&info->mcelsius,
                                SYS_CPU_CORETEMP_PREFIX2 "temp%d_input", node.temp_idx);
            if(rv < 0) {
                return rv;
            }
        }
    }

    return ONLP_STATUS_OK;
}

static int get_bmc_thermal_info(thrm_node_t node, onlp_thermal_info_t* info)
{
    bmc_info_t bmc_data = {0};

    if(info == NULL) {
        return ONLP_STATUS_E_PARAM;
    }

    *info = thermal_info[node.local_id];

    /* present */
    if(IS_GENERAL(node)) {
        info->status |= ONLP_THERMAL_STATUS_PRESENT;
    } else if(IS_PSU(node)) {
        int psu_present = 0;
        ONLP_TRY(get_psu_present_status(node.parent, &psu_present));
        if (psu_present == PSU_STATUS_PRES) {
            info->status |= ONLP_THERMAL_STATUS_PRESENT;
        } else {
            info->status &= ~ONLP_THERMAL_STATUS_PRESENT;
        }
    } else {
        return ONLP_STATUS_E_PARAM;
    }

    /* contents */
    if(info->status & ONLP_THERMAL_STATUS_PRESENT) {
        int bmc_attr = node.bmc;
        ONLP_TRY(read_bmc_sensor(bmc_attr, THERMAL_SENSOR, &bmc_data));

        if(BMC_ATTR_INVALID_VAL != (int)(bmc_data.data)) {
            info->status &= ~ONLP_THERMAL_STATUS_FAILED;
            info->mcelsius = (int) (bmc_data.data*1000);
        }else{
            info->status |= ONLP_THERMAL_STATUS_FAILED;
            info->mcelsius = 0;
        }

        if(BMC_ATTR_INVALID_VAL != (int)(bmc_data.upper_non_crit)) {
            info->caps |= ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD;
            info->thresholds.warning = (int) (bmc_data.upper_non_crit*1000);
        }else{
            info->caps &= ~ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD;
            info->thresholds.warning = 0;
        }

        if(BMC_ATTR_INVALID_VAL != (int)(bmc_data.upper_crit)) {
            info->caps |= ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD;
            info->thresholds.error = (int) (bmc_data.upper_crit*1000);
        }else{
            info->caps &= ~ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD;
            info->thresholds.error = 0;
        }

        if(BMC_ATTR_INVALID_VAL != (int)(bmc_data.upper_non_recov)) {
            info->caps |= ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD;
            info->thresholds.shutdown = (int) (bmc_data.upper_non_recov*1000);
        }else{
            info->caps &= ~ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD;
            info->thresholds.shutdown = 0;
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
    thrm_node_t node = {0};

    if(rv == NULL) {
        return ONLP_STATUS_E_PARAM;
    }

    ONLP_TRY(get_node(id, &node));

    /* update info  */
    if(IS_CPU(node))
        ONLP_TRY(get_cpu_thermal_info(node, rv));
    else
        ONLP_TRY(get_bmc_thermal_info(node, rv));

    return ONLP_STATUS_OK;
}

/**
 * @brief Retrieve the thermal's operational status.
 * @param id The thermal oid.
 * @param rv [out] Receives the operational status.
 */
int onlp_thermali_status_get(onlp_oid_t id, uint32_t* rv)
{
    onlp_thermal_info_t info ={0};
    thrm_node_t node = {0};

    ONLP_TRY(get_node(id, &node));

    if(IS_CPU(node))
        ONLP_TRY(get_cpu_thermal_info(node, &info));
    else
        ONLP_TRY(get_bmc_thermal_info(node, &info));

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
    thrm_node_t node = {0};

    ONLP_TRY(get_node(id, &node));
    *rv = thermal_info[node.local_id].hdr;

    return ONLP_STATUS_OK;
}

/**
 * @brief Generic ioctl.
 */
int onlp_thermali_ioctl(int id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}
