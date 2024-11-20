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
    int type;
    int temp_idx;
    int bmc;
    int parent;
    int warning;
    int error;
    int shutdown;
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
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PKG),
            .description = "CPU Package",
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
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PECI),
            .description = "TEMP_CPU_PECI",
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
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV_RISER),
            .description = "TEMP_ENV_RISER",
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
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU0_TEMP1),
            .description = "PSU 0 THERMAL 1",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU1_TEMP1),
            .description = "PSU 1 THERMAL 1",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL)
    }
};

static int get_node(int local_id, thrm_node_t *node) {
    if(node == NULL)
        return ONLP_STATUS_E_PARAM;

    switch(local_id) {
        case ONLP_THERMAL_CPU_PKG:
            node->type = TYPE_THRM_ATTR_CPU_SYSFS;
            node->temp_idx = 1;
            node->warning  = THERMAL_WARNING_DEFAULT;
            node->error    = THERMAL_ERROR_DEFAULT;
            node->shutdown = THERMAL_SHUTDOWN_DEFAULT;
            break;
        case ONLP_THERMAL_ENV_CPU:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_ENV_CPU;
            node->warning  = 85;
            node->error    = 95;
            node->shutdown = 100;
            break;
        case ONLP_THERMAL_CPU_PECI:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_CPU_PECI;
            node->warning  = 85;
            node->error    = 95;
            node->shutdown = 100;
            break;
        case ONLP_THERMAL_ENV0:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_ENV0;
            node->warning  = 60;
            node->error    = 65;
            node->shutdown = 70;
            break;
        case ONLP_THERMAL_ENV1:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_ENV1;
            node->warning  = 60;
            node->error    = 65;
            node->shutdown = 70;
            break;
        case ONLP_THERMAL_ENV2:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_ENV2;
            node->warning  = 60;
            node->error    = 65;
            node->shutdown = 70;
            break;
        case ONLP_THERMAL_ENV3:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_ENV3;
            node->warning  = 60;
            node->error    = 65;
            node->shutdown = 70;
            break;
        case ONLP_THERMAL_ENV4:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_ENV4;
            node->warning  = 60;
            node->error    = 65;
            node->shutdown = 70;
            break;
        case ONLP_THERMAL_ENV5:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_ENV5;
            node->warning  = 60;
            node->error    = 65;
            node->shutdown = 70;
            break;
        case ONLP_THERMAL_ENV_RISER:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_ENV_RISER;
            node->warning  = 60;
            node->error    = 65;
            node->shutdown = 70;
            break;
        case ONLP_THERMAL_ENV_FAN0:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_ENV_FAN0;
            node->warning  = 60;
            node->error    = 65;
            node->shutdown = 70;
            break;
        case ONLP_THERMAL_ENV_FAN1:
            node->type = TYPE_THRM_ATTR_GENERAL_BMC;
            node->bmc = BMC_ATTR_ID_TEMP_ENV_FAN1;
            node->warning  = 60;
            node->error    = 65;
            node->shutdown = 70;
            break;
        case ONLP_THERMAL_PSU0_TEMP1:
            node->type = TYPE_THRM_ATTR_PSU_BMC;
            node->bmc = BMC_ATTR_ID_PSU0_TEMP1;
            node->parent = ONLP_PSU_0;
            node->warning  = 65;
            node->error    = 70;
            node->shutdown = 75;
            break;
        case ONLP_THERMAL_PSU1_TEMP1:
            node->type = TYPE_THRM_ATTR_PSU_BMC;
            node->bmc = BMC_ATTR_ID_PSU1_TEMP1;
            node->parent = ONLP_PSU_1;
            node->warning  = 65;
            node->error    = 70;
            node->shutdown = 75;
            break;
        default:
             return ONLP_STATUS_E_PARAM;
    }

    return ONLP_STATUS_OK;
}

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
        case ONLP_THERMAL_ENV_CPU:
        case ONLP_THERMAL_CPU_PECI:
        case ONLP_THERMAL_ENV0:
        case ONLP_THERMAL_ENV1:
        case ONLP_THERMAL_ENV2:
        case ONLP_THERMAL_ENV3:
        case ONLP_THERMAL_ENV4:
        case ONLP_THERMAL_ENV5:
        case ONLP_THERMAL_ENV_RISER:
        case ONLP_THERMAL_ENV_FAN0:
        case ONLP_THERMAL_ENV_FAN1:
        case ONLP_THERMAL_PSU0_TEMP1:
        case ONLP_THERMAL_PSU1_TEMP1:
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
    thrm_node_t node = {0};

    *info = thermal_info[local_id];
    ONLP_TRY(get_node(local_id, &node));
    info->thresholds.warning = MILLI(node.warning);
    info->thresholds.error = MILLI(node.error);
    info->thresholds.shutdown = MILLI(node.shutdown);

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

static int get_bmc_thermal_info(int local_id, onlp_thermal_info_t* info)
{
    float data = 0;
    *info = thermal_info[local_id];
    thrm_node_t node = {0};
    ONLP_TRY(get_node(local_id, &node));
    info->thresholds.warning =  MILLI(node.warning);
    info->thresholds.error =  MILLI(node.error);
    info->thresholds.shutdown =  MILLI(node.shutdown);

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
    thrm_node_t node = {0};

    if(rv == NULL) {
        return ONLP_STATUS_E_PARAM;
    }

    ONLP_TRY(get_thermal_local_id(id, &local_id));
    ONLP_TRY(get_node(local_id, &node));

    /* update info  */
    if(IS_CPU(node))
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
    thrm_node_t node = {0};

    ONLP_TRY(get_thermal_local_id(id, &local_id));
    ONLP_TRY(get_node(local_id, &node));

    if(IS_CPU(node))
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