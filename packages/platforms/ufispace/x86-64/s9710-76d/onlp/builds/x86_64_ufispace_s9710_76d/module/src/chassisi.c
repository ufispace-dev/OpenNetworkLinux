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


/* This is definitions for x86-64-ufispace-s9710-76d*/
/* OID map*/
/*
 * [01] CHASSIS
 *            |----[01] ONLP_THERMAL_CPU_PKG
 *            |----[02] ONLP_THERMAL_CPU_0
 *            |----[03] ONLP_THERMAL_CPU_1
 *            |----[04] ONLP_THERMAL_CPU_2
 *            |----[05] ONLP_THERMAL_CPU_3
 *            |----[06] ONLP_THERMAL_CPU_4
 *            |----[07] ONLP_THERMAL_CPU_5
 *            |----[08] ONLP_THERMAL_CPU_6
 *            |----[09] ONLP_THERMAL_CPU_7
 *            |----[10] ONLP_THERMAL_ADC_CPU
 *            |----[11] ONLP_THERMAL_CPU_PECI
 *            |----[12] ONLP_THERMAL_MAC_ENV_1
 *            |----[13] ONLP_THERMAL_MAC_ENV_2
 *            |----[14] ONLP_THERMAL_FRONT_ENV_1
 *            |----[15] ONLP_THERMAL_FRONT_ENV_2
 *            |----[16] ONLP_THERMAL_ENV_1
 *            |----[17] ONLP_THERMAL_ENV_2
 *            |----[18] ONLP_THERMAL_EXT_ENV_1
 *            |----[19] ONLP_THERMAL_EXT_ENV_2 
 *            |----[01] ONLP_LED_SYS_SYS
 *            |----[02] ONLP_LED_SYS_FAN
 *            |----[03] ONLP_LED_SYS_PSU_0
 *            |----[04] ONLP_LED_SYS_PSU_1
 *            |----[01] ONLP_PSU_0----[20] ONLP_THERMAL_PSU_0
 *            |                  |----[05] ONLP_PSU_0_FAN
 *            |----[02] ONLP_PSU_1----[21] ONLP_THERMAL_PSU_1
 *            |                  |----[06] ONLP_PSU_1_FAN 
 *            |----[01] ONLP_FAN_F_0
 *            |----[02] ONLP_FAN_R_0
 *            |----[03] ONLP_FAN_F_1
 *            |----[04] ONLP_FAN_R_1
 *            |----[05] ONLP_FAN_F_2
 *            |----[06] ONLP_FAN_R_2
 *            |----[07] ONLP_FAN_F_3
 *            |----[08] ONLP_FAN_R_3
 */
 
static onlp_oid_t __onlp_oid_info[] = {
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PKG),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_0),        
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_1),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_2),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_3),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_4),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_5),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_6),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_7),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ADC_CPU),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PECI),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC_ENV_1),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC_ENV_2),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_FRONT_ENV_1),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_FRONT_ENV_2),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV_1),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV_2),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_EXT_ENV_1),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_EXT_ENV_2),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_SYS),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_FAN),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_PSU_0),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_PSU_1),
    ONLP_PSU_ID_CREATE(ONLP_PSU_0),
    ONLP_PSU_ID_CREATE(ONLP_PSU_1),
    ONLP_FAN_ID_CREATE(ONLP_FAN_F_0),
    ONLP_FAN_ID_CREATE(ONLP_FAN_R_0),
    ONLP_FAN_ID_CREATE(ONLP_FAN_F_1),
    ONLP_FAN_ID_CREATE(ONLP_FAN_R_1),
    ONLP_FAN_ID_CREATE(ONLP_FAN_F_2),
    ONLP_FAN_ID_CREATE(ONLP_FAN_R_2),
    ONLP_FAN_ID_CREATE(ONLP_FAN_F_3),
    ONLP_FAN_ID_CREATE(ONLP_FAN_R_3),
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
int onlp_chassisi_hdr_get(onlp_oid_id_t id, onlp_oid_hdr_t* hdr)
{
    int i;
    ONLP_OID_STATUS_FLAG_SET(hdr, PRESENT);
    ONLP_OID_STATUS_FLAG_SET(hdr, OPERATIONAL);

    memcpy(hdr->coids, __onlp_oid_info, sizeof(__onlp_oid_info));

    /** Add 2 SFP+ and 76 QSFPDD OIDs after the static table */
    onlp_oid_t* e = hdr->coids + AIM_ARRAYSIZE(__onlp_oid_info);

    /* 36 QSFPDD_NIF + 40 QSFPDD_NIF + 2 SFP+ */
    for(i = 1; i <= 78; i++) {
        *e++ = ONLP_SFP_ID_CREATE(i);
    }

    return ONLP_STATUS_OK;
}
