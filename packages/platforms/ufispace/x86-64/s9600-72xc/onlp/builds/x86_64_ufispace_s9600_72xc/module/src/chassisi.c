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
 * Chassis Implementation.
 *
 ***********************************************************/
#include <onlp/platformi/chassisi.h>
#include "platform_lib.h"

/* This is definitions for x86-64-ufispace-s9600-72xc */
/* OID map*/
/*
 * [01] CHASSIS
 *            |----[01] ONLP_THERMAL_CPU_PECI
 *            |----[02] ONLP_THERMAL_CPU_ENV
 *            |----[03] ONLP_THERMAL_CPU_ENV_2
 *            |----[04] ONLP_THERMAL_MAC_ENV
 *            |----[05] ONLP_THERMAL_MAC_DIE
 *            |----[06] ONLP_THERMAL_ENV_FRONT
 *            |----[07] ONLP_THERMAL_ENV_REAR
 *            |----[08] ONLP_THERMAL_PSU0
 *            |----[09] ONLP_THERMAL_PSU1
 *            |----[10] ONLP_THERMAL_CPU_PKG
 *            |
 *            |----[01] ONLP_LED_SYS_SYNC
 *            |----[02] ONLP_LED_SYS_SYS
 *            |----[03] ONLP_LED_SYS_FAN
 *            |----[04] ONLP_LED_SYS_PSU0
 *            |----[05] ONLP_LED_SYS_PSU1
 *            |
 *            |----[01] ONLP_PSU_0----[08] ONLP_THERMAL_PSU0
 *            |                  |----[05] ONLP_PSU_0_FAN
 *            |----[02] ONLP_PSU_1----[09] ONLP_THERMAL_PSU1
 *            |                  |----[06] ONLP_PSU_1_FAN
 *            |
 *            |----[01] ONLP_FAN_0
 *            |----[02] ONLP_FAN_1
 *            |----[03] ONLP_FAN_2
 *            |----[04] ONLP_FAN_3
 */

static onlp_oid_t __onlp_oid_info[] = {
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PECI),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_ENV),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_ENV_2),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC_ENV),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC_DIE),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV_FRONT),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV_REAR),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PKG),

    ONLP_LED_ID_CREATE(ONLP_LED_SYS_SYNC),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_SYS),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_FAN),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_PSU0),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_PSU1),

    ONLP_PSU_ID_CREATE(ONLP_PSU_0),
    ONLP_PSU_ID_CREATE(ONLP_PSU_1),

    ONLP_FAN_ID_CREATE(ONLP_FAN_0),
    ONLP_FAN_ID_CREATE(ONLP_FAN_1),
    ONLP_FAN_ID_CREATE(ONLP_FAN_2),
    ONLP_FAN_ID_CREATE(ONLP_FAN_3),
};

/**
 * @brief Software initializaiton of the Chassis module.
 */
int onlp_chassisi_sw_init(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Hardware initializaiton of the Chassis module.
 * @param flags The hardware initialization flags.
 */
int onlp_chassisi_hw_init(uint32_t flags)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Deinitialize the chassis software module.
 * @note The primary purpose of this API is to properly
 * deallocate any resources used by the module in order
 * faciliate detection of real resouce leaks.
 */
int onlp_chassisi_sw_denit(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the chassis hdr structure.
 * @param id The Chassis OID.
 * @param[out] hdr Receives the header.
 */
int onlp_chassisi_hdr_get(onlp_oid_id_t id, onlp_oid_hdr_t *hdr)
{
    int i = 0;
    ONLP_OID_STATUS_FLAG_SET(hdr, PRESENT);
    ONLP_OID_STATUS_FLAG_SET(hdr, OPERATIONAL);

    memcpy(hdr->coids, __onlp_oid_info, sizeof(__onlp_oid_info));

    /** Add port OIDs after the static table */
    onlp_oid_id_t *e = hdr->coids + AIM_ARRAYSIZE(__onlp_oid_info);
    /* 0 base port numer */
    for (i = 1; i <= PORT_NUM; i++)
    {
        *e++ = ONLP_SFP_ID_CREATE(i);
    }

    return ONLP_STATUS_OK;
}
