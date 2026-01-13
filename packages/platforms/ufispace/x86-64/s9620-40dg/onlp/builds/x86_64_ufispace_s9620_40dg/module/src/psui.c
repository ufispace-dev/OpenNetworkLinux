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
 * Power Supply Management Implementation.
 *
 ***********************************************************/
#include <onlp/platformi/psui.h>
#include <onlp/platformi/fani.h>
#include "platform_lib.h"

#define PSU_STATUS_PWR_FAIL    0
#define PSU_STATUS_PWR_GD      1

typedef struct
{
    int local_id;
    char *sysfs;
    int abs_bit;
    int pwrgd_bit;
    char *sysfs_abs;
    char *sysfs_pwrgd;
    int fru_id;
    int attr_vin;
    int attr_vout;
    int attr_iin;
    int attr_iout;
    int attr_pin;
    int attr_pout;
    int attr_stbvout;
    int attr_stbiout;
} psu_node_t;

static onlp_psu_info_t psu_info[] =
{
    { }, /* Not used */
    {
        .hdr = {
            .id = ONLP_PSU_ID_CREATE(ONLP_PSU_0),
            .description = "PSU 0",
            .poid = POID_0,
            .coids = {
                ONLP_FAN_ID_CREATE(ONLP_PSU_0_FAN),
                ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU0_TEMP1)
            }
        },
        .model = COMM_STR_NOT_SUPPORTED,
        .serial = COMM_STR_NOT_SUPPORTED,
    },
    {
        .hdr = {
            .id = ONLP_PSU_ID_CREATE(ONLP_PSU_1),
            .description = "PSU 1",
            .poid = POID_0,
            .coids = {
                ONLP_FAN_ID_CREATE(ONLP_PSU_1_FAN),
                ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU1_TEMP1)
            }
        },
        .model = COMM_STR_NOT_SUPPORTED,
        .serial = COMM_STR_NOT_SUPPORTED,
    },
};

static psu_support_info_t psu_support_list[] =
{
    {"FSPGROUP", "YNEM1000AM-2R01T10", ONLP_PSU_TYPE_AC  , ONLP_FAN_STATUS_F2B},
    {"FSPGROUP", "YNEM1000DM-2R01N01", ONLP_PSU_TYPE_DC48, ONLP_FAN_STATUS_F2B},
};

static int get_node(int id, psu_node_t* node)
{
    board_t board = {0};
    if(node == NULL) {
        return ONLP_STATUS_E_PARAM;
    }

    if(!ONLP_OID_IS_PSU(id)) {
        return ONLP_STATUS_E_INVALID;
    }

    ONLP_TRY(get_board_version(&board));

    node->local_id = ONLP_OID_ID_GET(id);
    switch(node->local_id) {
        case ONLP_PSU_0:
            node->sysfs_abs   = SYSFS_CPLD1 "psu0_abs";
            node->sysfs_pwrgd = SYSFS_CPLD1 "psu0_vout_pg";
            node->fru_id = BMC_FRU_IDX_ONLP_PSU_0;
            node->attr_vin = BMC_ATTR_ID_PSU0_VIN;
            node->attr_vout = BMC_ATTR_ID_PSU0_VOUT;
            node->attr_iin = BMC_ATTR_ID_PSU0_IIN;
            node->attr_iout = BMC_ATTR_ID_PSU0_IOUT;
            node->attr_pin = BMC_ATTR_ID_PSU0_PIN;
            node->attr_pout = BMC_ATTR_ID_PSU0_POUT;
            node->attr_stbvout = BMC_ATTR_ID_INVALID;
            node->attr_stbiout = BMC_ATTR_ID_INVALID;
            break;
        case ONLP_PSU_1:
            node->sysfs_abs   = SYSFS_CPLD1 "psu1_abs";
            node->sysfs_pwrgd = SYSFS_CPLD1 "psu1_vout_pg";
            node->fru_id = BMC_FRU_IDX_ONLP_PSU_1;
            node->attr_vin = BMC_ATTR_ID_PSU1_VIN;
            node->attr_vout = BMC_ATTR_ID_PSU1_VOUT;
            node->attr_iin = BMC_ATTR_ID_PSU1_IIN;
            node->attr_iout = BMC_ATTR_ID_PSU1_IOUT;
            node->attr_pin = BMC_ATTR_ID_PSU1_PIN;
            node->attr_pout = BMC_ATTR_ID_PSU1_POUT;
            node->attr_stbvout = BMC_ATTR_ID_INVALID;
            node->attr_stbiout = BMC_ATTR_ID_INVALID;
            break;
        default:
            return ONLP_STATUS_E_INVALID;
    }
    return ONLP_STATUS_OK;
}

int get_psu_present_status(int id, int *pw_present)
{
    int status;
    psu_node_t node = {0};

    if(pw_present == NULL) {
        return ONLP_STATUS_E_INTERNAL;
    }

    ONLP_TRY(get_node(id, &node));
    ONLP_TRY(read_file_hex(&status, node.sysfs_abs));

    *pw_present = status ? PSU_STATUS_ABS : PSU_STATUS_PRES;

    return ONLP_STATUS_OK;
}

static int get_psu_pwgood_status(int id, int *pw_good)
{
    int status;
    psu_node_t node = {0};

    if(pw_good == NULL) {
        return ONLP_STATUS_E_INTERNAL;
    }

    ONLP_TRY(get_node(id, &node));
    ONLP_TRY(read_file_hex(&status, node.sysfs_pwrgd));

    *pw_good = status ? PSU_STATUS_PWR_GD : PSU_STATUS_PWR_FAIL;

    return ONLP_STATUS_OK;
}

/**
 * @brief get psu support info
 * @param id: psu OID
 * @param[out] psu_support_info: psu support info
 * @param fru_in: input fru node. we will use the input node informations to get psu type
 */
int get_psu_support_info(int id, psu_support_info_t *psu_support_info, bmc_fru_t *fru_in)
{
    bmc_fru_t *fru = NULL;
    bmc_fru_t fru_tmp = {0};
    psu_node_t node = {0};
    int i, max, is_psu_supported = 0;

    if(psu_support_info == NULL) {
        return ONLP_STATUS_E_INTERNAL;
    }

    ONLP_TRY(get_node(id, &node));

    if(fru_in == NULL) {
        fru = &fru_tmp;
        ONLP_TRY(read_bmc_fru(node.fru_id, fru));
    } else {
        fru = fru_in;
    }

    max = sizeof(psu_support_list)/sizeof(*psu_support_list);
    for (i=0; i < max; ++i) {
        if ((strncmp(fru->vendor.val, psu_support_list[i].vendor, BMC_FRU_ATTR_KEY_VALUE_SIZE)==0) &&
            (strncmp(fru->part_num.val, psu_support_list[i].part_number, BMC_FRU_ATTR_KEY_VALUE_SIZE)==0)) {
            *psu_support_info = psu_support_list[i];
            is_psu_supported = 1;
            break;
        }
    }

    if (!is_psu_supported) {
        AIM_LOG_ERROR("[%s]unknown PSU type, vendor=%s, part_num=%s", __FUNCTION__, fru->vendor.val, fru->part_num.val);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Update the information of Model and Serial from PSU EEPROM
 * @param id The psu OID
 * @param[out] info Receives the PSU information (model and serial).
 */
static int update_psui_fru_info(int id, onlp_psu_info_t* info)
{
    bmc_fru_t fru = {0};
    psu_support_info_t psu_support_info = {0};

    psu_node_t node = {0};
    ONLP_TRY(get_node(id, &node));

    //read fru data
    ONLP_TRY(read_bmc_fru(node.fru_id, &fru));

    //update FRU model
    memset(info->model, 0, sizeof(info->model));
    snprintf(info->model, sizeof(info->model), "%s", fru.part_num.val);

    //update FRU serial
    memset(info->serial, 0, sizeof(info->serial));
    snprintf(info->serial, sizeof(info->serial), "%s", fru.serial.val);

    //update FRU type
    ONLP_TRY(get_psu_support_info(id, &psu_support_info, &fru));

    if(psu_support_info.type == ONLP_PSU_TYPE_AC) {
        info->caps |= ONLP_PSU_CAPS_AC;
        info->caps &= ~ONLP_PSU_CAPS_DC48;
    } else if(psu_support_info.type == ONLP_PSU_TYPE_DC48){
        info->caps &= ~ONLP_PSU_CAPS_AC;
        info->caps |= ONLP_PSU_CAPS_DC48;
    }
    return ONLP_STATUS_OK;
}

static int update_info(int id, onlp_psu_info_t *info)
{
    int pw_present, pw_good;
    int stbmvout = 0, stbmiout = 0;
    bmc_info_t bmc_data = {0};
    psu_node_t node = {0};
    ONLP_TRY(get_node(id, &node));
    *info = psu_info[node.local_id];
    ONLP_TRY(get_psu_present_status(id, &pw_present));

    if (pw_present != PSU_STATUS_PRES) {
        info->status &= ~ONLP_PSU_STATUS_PRESENT;
        info->status |=  ONLP_PSU_STATUS_UNPLUGGED;
    } else {
        info->status |= ONLP_PSU_STATUS_PRESENT;
    }

    if(info->status & ONLP_PSU_STATUS_PRESENT) {
        /* Get power good status */
        ONLP_TRY(get_psu_pwgood_status(id, &pw_good));

        if (pw_good != PSU_STATUS_PWR_GD) {
            info->status |= ONLP_PSU_STATUS_FAILED;
        } else {
            info->status &= ~ONLP_PSU_STATUS_FAILED;
        }

        /* Get power vin status */
        if(node.attr_vin != BMC_ATTR_ID_INVALID) {
            ONLP_TRY(read_bmc_sensor(node.attr_vin , PSU_SENSOR, &bmc_data));
            if(BMC_ATTR_INVALID_VAL != (int)(bmc_data.data)) {
                info->mvin = (int) (bmc_data.data*1000);
                info->caps |= ONLP_PSU_CAPS_VIN;
            }
        }

        /* Get power vout status */
        if(node.attr_vout != BMC_ATTR_ID_INVALID) {
            ONLP_TRY(read_bmc_sensor(node.attr_vout, PSU_SENSOR, &bmc_data));
            if(BMC_ATTR_INVALID_VAL != (int)(bmc_data.data)) {
                info->mvout = (int) (bmc_data.data*1000);
                info->caps |= ONLP_PSU_CAPS_VOUT;
            }
        }

        /* Get power iin status */
        if(node.attr_iin != BMC_ATTR_ID_INVALID) {
            ONLP_TRY(read_bmc_sensor(node.attr_iin, PSU_SENSOR, &bmc_data));
            if(BMC_ATTR_INVALID_VAL != (int)(bmc_data.data)) {
                info->miin = (int) (bmc_data.data*1000);
                info->caps |= ONLP_PSU_CAPS_IIN;
            }
        }

        /* Get power iout status */
        if(node.attr_iout != BMC_ATTR_ID_INVALID) {
            ONLP_TRY(read_bmc_sensor(node.attr_iout, PSU_SENSOR, &bmc_data));
            if(BMC_ATTR_INVALID_VAL != (int)(bmc_data.data)) {
                info->miout = (int) (bmc_data.data*1000);
                info->caps |= ONLP_PSU_CAPS_IOUT;
            }
        }

        /* Get power in */
        if(node.attr_pin != BMC_ATTR_ID_INVALID) {
            ONLP_TRY(read_bmc_sensor(node.attr_pin, PSU_SENSOR, &bmc_data));
            if(BMC_ATTR_INVALID_VAL != (int)(bmc_data.data)) {
                info->mpin = (int) (bmc_data.data*1000);
                info->caps |= ONLP_PSU_CAPS_PIN;
            }
        }

        /* Get power out */
        if(node.attr_pout != BMC_ATTR_ID_INVALID) {
            ONLP_TRY(read_bmc_sensor(node.attr_pout, PSU_SENSOR, &bmc_data));
            if(BMC_ATTR_INVALID_VAL != (int)(bmc_data.data)) {
                info->mpout = (int) (bmc_data.data*1000);
                info->caps |= ONLP_PSU_CAPS_POUT;
            }
        }

        /* Get standby power vout */
        if(node.attr_stbvout != BMC_ATTR_ID_INVALID) {
            ONLP_TRY(read_bmc_sensor(node.attr_stbvout, PSU_SENSOR, &bmc_data));
            if(BMC_ATTR_INVALID_VAL != (int)(bmc_data.data)) {
                /*
                 *  This feature is reserved for future use and serves no purpose here.
                 *  To avoid compiler warnings about unused variables, I added this workaround.
                 */
                (void) stbmvout;
                stbmvout = (int) (bmc_data.data*1000);
            }
        }

        /* Get standby power iout */
        if(node.attr_stbiout != BMC_ATTR_ID_INVALID) {
            ONLP_TRY(read_bmc_sensor(node.attr_stbiout, PSU_SENSOR, &bmc_data));
            if(BMC_ATTR_INVALID_VAL != (int)(bmc_data.data)) {
                /*
                 *  This feature is reserved for future use and serves no purpose here.
                 *  To avoid compiler warnings about unused variables, I added this workaround.
                 */
                (void) stbmiout;
                stbmiout = (int) (bmc_data.data*1000);
            }
        }

        /* Get FRU (model/serial) */
        ONLP_TRY(update_psui_fru_info(id, info));
    }
    return ONLP_STATUS_OK;
}

/**
 * @brief Initialize the PSU subsystem.
 */
int onlp_psui_init(void)
{
    init_lock();
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the information structure for the given PSU
 * @param id The PSU OID
 * @param rv [out] Receives the PSU information.
 */
int onlp_psui_info_get(onlp_oid_t id, onlp_psu_info_t* rv)
{
    ONLP_TRY(update_info(id, rv));
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the PSU's operational status.
 * @param id The PSU OID.
 * @param rv [out] Receives the operational status.
 */
int onlp_psui_status_get(onlp_oid_t id, uint32_t* rv)
{
    onlp_psu_info_t info ={0};
    ONLP_TRY(update_info(id, &info));
    *rv = info.status;

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the PSU's oid header.
 * @param id The PSU OID.
 * @param rv [out] Receives the header.
 */
int onlp_psui_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* rv)
{
    psu_node_t node = {0};
    ONLP_TRY(get_node(id, &node));
    *rv = psu_info[node.local_id].hdr;

    return ONLP_STATUS_OK;
}

/**
 * @brief Generic PSU ioctl
 * @param id The PSU OID
 * @param vargs The variable argument list for the ioctl call.
 */
int onlp_psui_ioctl(onlp_oid_t pid, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}
