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
#include "platform_lib.h"

#define PSU0_PRESENT_MASK      0x01
#define PSU1_PRESENT_MASK      0x02
#define PSU0_PWGOOD_MASK       0x10
#define PSU1_PWGOOD_MASK       0x20
#define PSU_STATUS_PRESENT     1
#define PSU_STATUS_POWER_GOOD  1

#define SYSFS_PSU_STATUS     SYSFS_CPLD1 "cpld_psu_status"

#define PSU_INFO(id, desc, fid, tid)            \
    {                                           \
        { ONLP_PSU_ID_CREATE(id), desc, POID_0,\
            {                                   \
                ONLP_FAN_ID_CREATE(fid),        \
                ONLP_THERMAL_ID_CREATE(tid),    \
            }                                   \
        }                                       \
    }

static onlp_psu_info_t psu_info[] =
{
    { }, /* Not used */
    PSU_INFO(ONLP_PSU_0, "PSU-0", ONLP_PSU_0_FAN, ONLP_THERMAL_PSU_0),
    PSU_INFO(ONLP_PSU_1, "PSU-1", ONLP_PSU_1_FAN, ONLP_THERMAL_PSU_1),
};

static char *vendors[] = {"DELTA", "FSPGROUP"};

/**
 * @brief Get and check psu local ID
 * @param id [in] OID
 * @param local_id [out] The psu local id
 */
static int get_psu_local_id(int id, int *local_id)
{
    int tmp_id;

    if(local_id == NULL) {
        return ONLP_STATUS_E_PARAM;
    }

    if(!ONLP_OID_IS_PSU(id)) {
        return ONLP_STATUS_E_INVALID;
    }

    tmp_id = ONLP_OID_ID_GET(id);
    switch (tmp_id) {
        case ONLP_PSU_0:
        case ONLP_PSU_1:
            *local_id = tmp_id;
            return ONLP_STATUS_OK;
        default:
            return ONLP_STATUS_E_INVALID;
    }

    return ONLP_STATUS_E_INVALID;
}

int get_psu_present_status(int local_id, int *pw_present)
{
    int status;
    int mask;

    if(pw_present == NULL) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (local_id == ONLP_PSU_0) {
        mask = PSU0_PRESENT_MASK;
    } else if (local_id == ONLP_PSU_1) {
        mask = PSU1_PRESENT_MASK;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    ONLP_TRY(file_read_hex(&status, SYSFS_PSU_STATUS));

    *pw_present = ((status & mask)? 0 : 1);

    return ONLP_STATUS_OK;
}

static int get_psu_pwgood_status(int local_id, int *pw_good)
{
    int status;
    int mask;

    if(pw_good == NULL) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (local_id == ONLP_PSU_0) {
        mask = PSU0_PWGOOD_MASK;
    } else if (local_id == ONLP_PSU_1) {
        mask = PSU1_PWGOOD_MASK;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    ONLP_TRY(file_read_hex(&status, SYSFS_PSU_STATUS));

    *pw_good = ((status & mask)? 1 : 0);

    return ONLP_STATUS_OK;
}

/**
 * @brief get psu type
 * @param local_id: psu id
 * @param[out] psu_type: psu type(ONLP_PSU_TYPE_AC, ONLP_PSU_TYPE_DC48)
 * @param fru_in: input fru node. we will use the input node informations to get psu type
 */
int get_psu_type(int local_id, int *psu_type, bmc_fru_t *fru_in)
{
    bmc_fru_t *fru = NULL;
    bmc_fru_t fru_tmp = {0};

    if(local_id != ONLP_PSU_0 && local_id !=  ONLP_PSU_1) {
        return ONLP_STATUS_E_INVALID;
    }

    if(psu_type == NULL) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if(fru_in == NULL) {
        fru = &fru_tmp;
        ONLP_TRY(bmc_fru_read(local_id, fru));
    } else {
        fru = fru_in;
    }
    
    if (strncmp(fru->vendor.val, vendors[0], BMC_FRU_ATTR_KEY_VALUE_SIZE)==0) { //Delta
        //read from part_num
        if (fru->part_num.val[7] == 'A') {
            *psu_type = ONLP_PSU_TYPE_AC;
        } else if (fru->part_num.val[7] == 'D') {
            *psu_type = ONLP_PSU_TYPE_DC48;
        } else {
            AIM_LOG_ERROR("unknown PSU type, vendor=%d, model=%s, func=%s\n", fru->vendor.val, fru->part_num.val, __FUNCTION__);
            return ONLP_STATUS_E_INTERNAL; 
        }
    } else if (strncmp(fru->vendor.val, vendors[1], BMC_FRU_ATTR_KEY_VALUE_SIZE)==0) { //FSP
        //read from name        
        if (strstr(fru->name.val, "AM") > 0) {
            *psu_type = ONLP_PSU_TYPE_AC;
        } else if (strstr(fru->name.val, "EM") > 0) {
            *psu_type = ONLP_PSU_TYPE_DC48;
        } else {
            AIM_LOG_ERROR("unknown PSU type, vendor=%d, name=%s, func=%s\n", fru->vendor.val, fru->name.val, __FUNCTION__);
            return ONLP_STATUS_E_INTERNAL; 
        }
    } else {
        *psu_type = ONLP_PSU_TYPE_INVALID;
        AIM_LOG_ERROR("unknown PSU type, vendor=%s, model=%s, func=%s", fru->vendor.val, fru->name.val, __FUNCTION__);
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Update the information of Model and Serial from PSU EEPROM
 * @param local_id The PSU Local ID
 * @param[out] info Receives the PSU information (model and serial).
 */
static int update_psui_fru_info(int local_id, onlp_psu_info_t* info)
{
    bmc_fru_t fru = {0};
    int psu_type = ONLP_PSU_TYPE_AC;

    //read fru data
    ONLP_TRY(bmc_fru_read(local_id, &fru));

    //update FRU model
    memset(info->model, 0, sizeof(info->model));
    if (strncmp(fru.vendor.val, vendors[1], BMC_FRU_ATTR_KEY_VALUE_SIZE)==0) {
        //read product name for FSP
        snprintf(info->model, sizeof(info->model), "%s", fru.name.val);
    } else {
        //read part num for others
        snprintf(info->model, sizeof(info->model), "%s", fru.part_num.val);
    }

    //update FRU serial
    memset(info->serial, 0, sizeof(info->serial));
    snprintf(info->serial, sizeof(info->serial), "%s", fru.serial.val);

    //update FRU type
    ONLP_TRY(get_psu_type(local_id, &psu_type, &fru));
    if(psu_type == ONLP_PSU_TYPE_AC) {
        info->caps |= ONLP_PSU_CAPS_AC;
        info->caps &= ~ONLP_PSU_CAPS_DC48;
    } else if(psu_type == ONLP_PSU_TYPE_DC48){
        info->caps &= ~ONLP_PSU_CAPS_AC;
        info->caps |= ONLP_PSU_CAPS_DC48;
    }

    return ONLP_STATUS_OK;
}

static int update_psui_info(int local_id, onlp_psu_info_t *info)
{
    int stbmvout, stbmiout;
    float data;
    int attr_vin, attr_vout, attr_iin, attr_iout, attr_stbvout, attr_stbiout;

    if (local_id == ONLP_PSU_0) {
        attr_vin = BMC_ATTR_ID_PSU0_VIN;
        attr_vout = BMC_ATTR_ID_PSU0_VOUT;
        attr_iin = BMC_ATTR_ID_PSU0_IIN;
        attr_iout = BMC_ATTR_ID_PSU0_IOUT;
        attr_stbvout = BMC_ATTR_ID_PSU0_STBVOUT;
        attr_stbiout = BMC_ATTR_ID_PSU0_STBIOUT;
    } else {
        attr_vin = BMC_ATTR_ID_PSU1_VIN;
        attr_vout = BMC_ATTR_ID_PSU1_VOUT;
        attr_iin = BMC_ATTR_ID_PSU1_IIN;
        attr_iout = BMC_ATTR_ID_PSU1_IOUT;
        attr_stbvout = BMC_ATTR_ID_PSU1_STBVOUT;
        attr_stbiout = BMC_ATTR_ID_PSU1_STBIOUT;
    }

    /* Get power vin status */
    ONLP_TRY(bmc_sensor_read(attr_vin, PSU_SENSOR, &data));
    info->mvin = (int) (data*1000);
    info->caps |= ONLP_PSU_CAPS_VIN;

    /* Get power vout status */
    ONLP_TRY(bmc_sensor_read(attr_vout, PSU_SENSOR, &data));
    info->mvout = (int) (data*1000);
    info->caps |= ONLP_PSU_CAPS_VOUT;

    /* Get power iin status */
    ONLP_TRY(bmc_sensor_read(attr_iin, PSU_SENSOR, &data));
    info->miin = (int) (data*1000);
    info->caps |= ONLP_PSU_CAPS_IIN;

    /* Get power iout status */
    ONLP_TRY(bmc_sensor_read(attr_iout, PSU_SENSOR, &data));
    info->miout = (int) (data*1000);
    info->caps |= ONLP_PSU_CAPS_IOUT;

    /* Get standby power vout */
    ONLP_TRY(bmc_sensor_read(attr_stbvout, PSU_SENSOR, &data));
    stbmvout = (int) (data*1000);

    /* Get standby power iout */
    ONLP_TRY(bmc_sensor_read(attr_stbiout, PSU_SENSOR, &data));
    stbmiout = (int) (data*1000);

    /* Get power in and out */
    info->mpin = info->miin * info->mvin / 1000;
    info->mpout = (info->miout * info->mvout + stbmiout * stbmvout) / 1000;
    info->caps |= ONLP_PSU_CAPS_PIN | ONLP_PSU_CAPS_POUT;

    /* Get FRU (model/serial) */
    ONLP_TRY(update_psui_fru_info(local_id, info));

    return ONLP_STATUS_OK;
}

/**
 * @brief Initialize the PSU subsystem.
 */
int onlp_psui_init(void)
{
    lock_init();
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the information structure for the given PSU
 * @param id The PSU OID
 * @param rv [out] Receives the PSU information.
 */
int onlp_psui_info_get(onlp_oid_t id, onlp_psu_info_t* rv)
{
    int local_id;

    ONLP_TRY(get_psu_local_id(id, &local_id));
    memset(rv, 0, sizeof(onlp_psu_info_t));

    *rv = psu_info[local_id];
    /* update status */
    ONLP_TRY(onlp_psui_status_get(id, &rv->status));

    if((rv->status & ONLP_PSU_STATUS_PRESENT) == 0) {
        return ONLP_STATUS_OK;
    }

    ONLP_TRY(update_psui_info(local_id, rv));

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the PSU's operational status.
 * @param id The PSU OID.
 * @param rv [out] Receives the operational status.
 */
int onlp_psui_status_get(onlp_oid_t id, uint32_t* rv)
{
    int local_id;
    int pw_present, pw_good;

    ONLP_TRY(get_psu_local_id(id, &local_id));
    ONLP_TRY(get_psu_present_status(local_id, &pw_present));

    if (pw_present != PSU_STATUS_PRESENT) {
        *rv &= ~ONLP_PSU_STATUS_PRESENT;
        *rv |=  ONLP_PSU_STATUS_UNPLUGGED;
    } else {
        *rv |= ONLP_PSU_STATUS_PRESENT;
    }

    /* Get power good status */
    ONLP_TRY(get_psu_pwgood_status(local_id, &pw_good));

    if (pw_good != PSU_STATUS_POWER_GOOD) {
        *rv |= ONLP_PSU_STATUS_FAILED;
    } else {
        *rv &= ~ONLP_PSU_STATUS_FAILED;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the PSU's oid header.
 * @param id The PSU OID.
 * @param rv [out] Receives the header.
 */
int onlp_psui_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* rv)
{
    int local_id;

    ONLP_TRY(get_psu_local_id(id, &local_id));
    *rv = psu_info[local_id].hdr;

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

