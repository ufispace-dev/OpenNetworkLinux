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

#define SYSFS_PSU_STATUS     LPC_FMT "psu_status"
#define CMD_FRU_INFO_GET            "ipmitool fru print %d"IPMITOOL_REDIRECT_ERR" | grep '%s' | cut -d':' -f2 | awk '{$1=$1};1' | tr -d '\n'"

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_PSU(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

#define PSU_INFO(id, desc, fid, tid)            \
    {                                           \
        { ONLP_PSU_ID_CREATE(id), #desc, POID_0,\
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

static int ufi_psu_fru_get(onlp_psu_info_t* info, int id)
{
    char cmd[256];
    char cmd_out[64];
    char fru_model[] = "Product Name";  //only Product Name can identify AC/DC  //FIXME
    char fru_serial[] = "Product Serial";

    //FRU (model)

    memset(cmd, 0, sizeof(cmd));
    memset(cmd_out, 0, sizeof(cmd_out));
    memset(info->model, 0, sizeof(info->model));

    snprintf(cmd, sizeof(cmd), CMD_FRU_INFO_GET, id, fru_model);

    //Get psu fru info (model) from BMC
    if (exec_cmd(cmd, cmd_out, sizeof(cmd_out)) < 0) {
        AIM_LOG_ERROR("unable to read fru info from BMC, fru id=%d, cmd=%s, out=%s\n", id, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    //Check output is correct
    if (strnlen(cmd_out, sizeof(cmd_out))==0){
        AIM_LOG_ERROR("unable to read fru info from BMC, fru id=%d, cmd=%s, out=%s\n", id, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    snprintf(info->model, sizeof(info->model), "%s", cmd_out);

    //FRU (serial)

    memset(cmd, 0, sizeof(cmd));
    memset(cmd_out, 0, sizeof(cmd_out));
    memset(info->serial, 0, sizeof(info->serial));

    snprintf(cmd, sizeof(cmd), CMD_FRU_INFO_GET, id, fru_serial);

    //Get psu fru info (model) from BMC
    if (exec_cmd(cmd, cmd_out, sizeof(cmd_out)) < 0) {
        AIM_LOG_ERROR("unable to read fru info from BMC, fru id=%d, cmd=%s, out=%s\n", id, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    //Check output is correct
    if (strnlen(cmd_out, sizeof(cmd_out))==0){
        AIM_LOG_ERROR("unable to read fru info from BMC, fru id=%d, cmd=%s, out=%s\n", id, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    snprintf(info->serial, sizeof(info->serial), "%s", cmd_out);

    return ONLP_STATUS_OK;
}

static int ufi_psu_present_get(int *pw_present, int id)
{
    int status;
    int mask;

    if (id == ONLP_PSU_0) {
        mask = PSU0_PRESENT_MASK;
    } else if (id == ONLP_PSU_1) {
        mask = PSU1_PRESENT_MASK;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (file_read_hex(&status, SYSFS_PSU_STATUS) < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    *pw_present = ((status & mask)? 0 : 1);

    return ONLP_STATUS_OK;
}

static int ufi_psu_pwgood_get(int *pw_good, int id)
{
    int status;
    int mask;

    if (id == ONLP_PSU_0) {
        mask = PSU0_PWGOOD_MASK;
    } else if (id == ONLP_PSU_1) {
        mask = PSU1_PWGOOD_MASK;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (file_read_hex(&status, SYSFS_PSU_STATUS)) {
        return ONLP_STATUS_E_INTERNAL;
    }

    *pw_good = ((status & mask)? 1 : 0);

    return ONLP_STATUS_OK;
}

static int ufi_psu_status_info_get(int id, onlp_psu_info_t *info)
{
    int rc, pw_present, pw_good;
    int stbmvout, stbmiout;
    float data;
    int bmc_attr_id = BMC_ATTR_ID_MAX;
    int attr_vin, attr_vout, attr_iin, attr_iout, attr_stbvout, attr_stbiout;

    if (id == ONLP_PSU_0) {
        attr_vin = PSU_ATTR_VIN_0;
        attr_vout = PSU_ATTR_VOUT_0;
        attr_iin = PSU_ATTR_IIN_0;
        attr_iout = PSU_ATTR_IOUT_0;
        attr_stbvout = PSU_ATTR_STBVOUT_0;
        attr_stbiout = PSU_ATTR_STBIOUT_0;
    } else {
        attr_vin = PSU_ATTR_VIN_1;
        attr_vout = PSU_ATTR_VOUT_1;
        attr_iin = PSU_ATTR_IIN_1;
        attr_iout = PSU_ATTR_IOUT_1;
        attr_stbvout = PSU_ATTR_STBVOUT_1;
        attr_stbiout = PSU_ATTR_STBIOUT_1;
    }

     /* Get power present status */
    if ((rc = ufi_psu_present_get(&pw_present, id)) < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (pw_present != PSU_STATUS_PRESENT) {
        info->status &= ~ONLP_PSU_STATUS_PRESENT;
        info->status |=  ONLP_PSU_STATUS_UNPLUGGED;
        return ONLP_STATUS_OK;
    }

    info->status |= ONLP_PSU_STATUS_PRESENT;

    /* Get power good status */
    if ((rc = ufi_psu_pwgood_get(&pw_good, id)) < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (pw_good != PSU_STATUS_POWER_GOOD) {
        info->status |= ONLP_PSU_STATUS_FAILED;
    } else {
        info->status &= ~ONLP_PSU_STATUS_FAILED;
    }

    /* Get power vin status */
    bmc_attr_id = ufi_oid_to_bmc_attr_id(ONLP_OID_TYPE_PSU, id, attr_vin);
    if (bmc_sensor_read(bmc_attr_id , PSU_SENSOR, &data) < 0) {
        return ONLP_STATUS_E_INTERNAL;
    } else {
        info->mvin = (int) (data*1000);
        info->caps |= ONLP_PSU_CAPS_VIN;
    }

    /* Get power vout status */
    bmc_attr_id = ufi_oid_to_bmc_attr_id(ONLP_OID_TYPE_PSU, id, attr_vout);
    if (bmc_sensor_read(bmc_attr_id, PSU_SENSOR, &data) < 0) {
        return ONLP_STATUS_E_INTERNAL;
    } else {
        info->mvout = (int) (data*1000);
        info->caps |= ONLP_PSU_CAPS_VOUT;
    }

    /* Get power iin status */
    bmc_attr_id = ufi_oid_to_bmc_attr_id(ONLP_OID_TYPE_PSU, id, attr_iin);
    if (bmc_sensor_read(bmc_attr_id, PSU_SENSOR, &data) < 0) {
        return ONLP_STATUS_E_INTERNAL;
    } else {
        info->miin = (int) (data*1000);
        info->caps |= ONLP_PSU_CAPS_IIN;
    }

    /* Get power iout status */
    bmc_attr_id = ufi_oid_to_bmc_attr_id(ONLP_OID_TYPE_PSU, id, attr_iout);
    if (bmc_sensor_read(bmc_attr_id, PSU_SENSOR, &data) < 0) {
        return ONLP_STATUS_E_INTERNAL;
    } else {
        info->miout = (int) (data*1000);
        info->caps |= ONLP_PSU_CAPS_IOUT;
    }

    /* Get standby power vout */
    bmc_attr_id = ufi_oid_to_bmc_attr_id(ONLP_OID_TYPE_PSU, id, attr_stbvout);
    if (bmc_sensor_read(bmc_attr_id, PSU_SENSOR, &data) < 0) {
        return ONLP_STATUS_E_INTERNAL;
    } else {
        stbmvout = (int) (data*1000);
    }

    /* Get standby power iout */
    bmc_attr_id = ufi_oid_to_bmc_attr_id(ONLP_OID_TYPE_PSU, id, attr_stbiout);
    if (bmc_sensor_read(bmc_attr_id, PSU_SENSOR, &data) < 0) {
        return ONLP_STATUS_E_INTERNAL;
    } else {
        stbmiout = (int) (data*1000);
    }

    /* Get power in and out */
    info->mpin = info->miin * info->mvin / 1000;
    info->mpout = (info->miout * info->mvout + stbmiout * stbmvout) / 1000;
    info->caps |= ONLP_PSU_CAPS_PIN | ONLP_PSU_CAPS_POUT;

    /* Get FRU (model/serial) */
    if (ufi_psu_fru_get(info, id) < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

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
    int pid;
    VALIDATE(id);

    pid = ONLP_OID_ID_GET(id);
    memset(rv, 0, sizeof(onlp_psu_info_t));

    /* Set the onlp_oid_hdr_t */
    *rv = psu_info[pid];

    switch (pid) {
        case ONLP_PSU_0:
        case ONLP_PSU_1:
            return ufi_psu_status_info_get(pid, rv);
            break;
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
            break;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the PSU's operational status.
 * @param id The PSU OID.
 * @param rv [out] Receives the operational status.
 */
int onlp_psui_status_get(onlp_oid_t id, uint32_t* rv)
{
    int result = ONLP_STATUS_OK;
    onlp_psu_info_t info;
    VALIDATE(id);

    result = onlp_psui_info_get(id, &info);
    *rv = info.status;

    return result;
}

/**
 * @brief Get the PSU's oid header.
 * @param id The PSU OID.
 * @param rv [out] Receives the header.
 */
int onlp_psui_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* rv)
{
    int result = ONLP_STATUS_OK;
    onlp_psu_info_t* info;
    int psu_id;
    VALIDATE(id);

    psu_id = ONLP_OID_ID_GET(id);
    if(psu_id > ONLP_PSU_1) {
        result = ONLP_STATUS_E_INVALID;
    } else {
        info = &psu_info[psu_id];
        *rv = info->hdr;
    }
    return result;
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

