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
 *
 *
 ***********************************************************/
#include <onlp/platformi/psui.h>
#include <onlplib/file.h>

#include "platform_lib.h"

static onlp_psu_info_t pinfo[] =
{
    { }, /* Not used */
    {
        {
            PSU_OID_PSU1,
            "PSU 1",
            0,
            {
                FAN_OID_PSU1_FAN1,
	         THERMAL_OID_PSU1,
            },
        }
    },
    {
        {
            PSU_OID_PSU2,
            "PSU 2",
            0,
            {
                FAN_OID_PSU2_FAN1,
                THERMAL_OID_PSU2,
            },
        }
    }
};

int onlp_psui_init(void)
{
    lock_init();
    return ONLP_STATUS_OK;
}

int psu_status_info_get(int id, onlp_psu_info_t *info)
{
    int rc;
    int stbmvout, stbmiout;
    float data;
    int offset_sts = 0, offset_status = 0;
    int fru_id = id;

    if (id == PSU_ID_PSU2) {
        offset_sts = 6;
        offset_status = 2;
    }

     /* Get PSU present status */
    if ((rc = bmc_sensor_read(ID_PSU1_PRSNT_L + offset_status, PSU_SENSOR,
        &data)) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    } else {
        if (data != PSU_STATUS_PRESENT) {
            info->status &= ~ONLP_PSU_STATUS_PRESENT;
            info->status |=  ONLP_PSU_STATUS_UNPLUGGED;
            return ONLP_STATUS_OK;
        }
    }

    info->status |= ONLP_PSU_STATUS_PRESENT;

    /* Get PSU power good status */
    if ((rc = bmc_sensor_read(ID_PSU1_PWROK_H + offset_status, PSU_SENSOR,
        &data)) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (data != PSU_STATUS_POWER_GOOD) {
        info->status |= ONLP_PSU_STATUS_FAILED;
    } else {
        info->status &= ~ONLP_PSU_STATUS_FAILED;
    }

    /* Get power vin status */
    if ((rc = bmc_sensor_read(ID_PSU1_VIN + offset_sts, PSU_SENSOR,
        &data)) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    } else {
        info->mvin = (int) (data*1000);
        info->caps |= ONLP_PSU_CAPS_VIN;
    }

    /* Get power vout status */
    if ((rc = bmc_sensor_read(ID_PSU1_VOUT + offset_sts, PSU_SENSOR,
        &data)) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    } else {
        info->mvout = (int) (data*1000);
        info->caps |= ONLP_PSU_CAPS_VOUT;
    }

    /* Get power iin status */
    if ((rc = bmc_sensor_read(ID_PSU1_IIN+ offset_sts, PSU_SENSOR,
        &data)) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    } else {
        info->miin = (int) (data*1000);
        info->caps |= ONLP_PSU_CAPS_IIN;
    }

    /* Get power iout status */
    if ((rc = bmc_sensor_read(ID_PSU1_IOUT + offset_sts, PSU_SENSOR,
        &data)) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    } else {
        info->miout = (int) (data*1000);
        info->caps |= ONLP_PSU_CAPS_IOUT;
    }

    /* Get standby power vout */
    if ((rc = bmc_sensor_read(ID_PSU1_STBVOUT+ offset_sts, PSU_SENSOR,
        &data)) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    } else {
        stbmvout = (int) (data*1000);
    }

    /* Get standby power iout */
    if ((rc = bmc_sensor_read(ID_PSU1_STBIOUT + offset_sts, PSU_SENSOR,
        &data)) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    } else {
        stbmiout = (int) (data*1000);
    }

    /* Get power in and out */
    info->mpin = info->miin * info->mvin / 1000;
    info->mpout = (info->miout * info->mvout + stbmiout * stbmvout) / 1000;
    info->caps |= ONLP_PSU_CAPS_PIN | ONLP_PSU_CAPS_POUT;

    /* Get FRU (model/serial) */
    if ((rc = bmc_fru_read(info, fru_id)) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;

}

int onlp_psui_info_get(onlp_oid_t id, onlp_psu_info_t* info)
{
    int pid;

    pid = ONLP_OID_ID_GET(id);
    memset(info, 0, sizeof(onlp_psu_info_t));

    /* Set the onlp_oid_hdr_t */
    *info = pinfo[pid];

    switch (pid) {
        case PSU_ID_PSU1:
        case PSU_ID_PSU2:
            return psu_status_info_get(pid, info);
            break;
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
            break;
    }

    return ONLP_STATUS_OK;


}
