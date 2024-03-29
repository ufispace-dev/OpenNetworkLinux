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
            PSU_OID_PSU0,
            "PSU 0",
            0,
        },
        COMM_STR_NOT_SUPPORTED,
        COMM_STR_NOT_SUPPORTED,
    },
    {
        {
            PSU_OID_PSU1,
            "PSU 1",
            0,
        },
        COMM_STR_NOT_SUPPORTED,
        COMM_STR_NOT_SUPPORTED,
    }
};

int onlp_psui_init(void)
{
    return ONLP_STATUS_OK;
}

int psu_status_info_get(int id, onlp_psu_info_t *info)
{
    int rc, pw_good;

    /* Get power present status */
    /*if ((rc = psu_present_get(&pw_present, id))
            != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (pw_present != PSU_STATUS_PRESENT) {
        info->status &= ~ONLP_PSU_STATUS_PRESENT;
        info->status |=  ONLP_PSU_STATUS_UNPLUGGED;
        return ONLP_STATUS_OK;
    }*/

    info->status |= ONLP_PSU_STATUS_PRESENT; //TODO: S9502 series use fixed PSU.

    /* Get power good status */
    if ((rc = psu_pwgood_get(&pw_good, id))
            != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (pw_good != PSU_STATUS_POWER_GOOD) {
        info->status |= ONLP_PSU_STATUS_FAILED;
    } else {
        info->status &= ~ONLP_PSU_STATUS_FAILED;
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
        case PSU_ID_PSU0:
        case PSU_ID_PSU1:
            return psu_status_info_get(pid, info);
            break;
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
            break;
    }

    return ONLP_STATUS_OK;
}