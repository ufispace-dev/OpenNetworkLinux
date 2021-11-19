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

/* This is definitions for x86-64-ufispace-s9510-28dc*/
/* OID map*/
/*
 * [01] CHASSIS
 *            |----[01]ONLP_THERMAL_CPU_PKG
 *            |----[02]ONLP_THERMAL_CPU_0
 *            |----[03]ONLP_THERMAL_CPU_1
 *            |----[04]ONLP_THERMAL_CPU_2
 *            |----[05]ONLP_THERMAL_CPU_3
 *            |----[06]ONLP_THERMAL_CPU_4
 *            |----[07]ONLP_THERMAL_CPU_5
 *            |----[08]ONLP_THERMAL_CPU_6
 *            |----[09]ONLP_THERMAL_CPU_7
 *            |----[10]ONLP_THERMAL_MAC
 *            |----[11]ONLP_THERMAL_DDR4
 *            |----[12]ONLP_THERMAL_BMC
 *            |----[13]ONLP_THERMAL_FANCARD1
 *            |----[14]ONLP_THERMAL_FANCARD2
 *            |----[15]ONLP_THERMAL_FPGA_R
 *            |----[16]ONLP_THERMAL_FPGA_L
 *            |----[17]ONLP_THERMAL_HWM_GDDR
 *            |----[18]ONLP_THERMAL_HWM_MAC
 *            |----[19]ONLP_THERMAL_HWM_AMB
 *            |----[20]ONLP_THERMAL_HWM_NTMCARD
 *            |
 *            |----[01] ONLP_LED_SYS_GNSS
 *            |----[02] ONLP_LED_SYS_SYNC
 *            |----[03] ONLP_LED_SYS_SYS
 *            |----[04] ONLP_LED_SYS_FAN
 *            |----[05] ONLP_LED_SYS_PWR
 *            |----[06] ONLP_LED_FLEXE_0
 *            |----[07] ONLP_LED_FLEXE_1
 *            |
 *            |----[01] ONLP_PSU_0----[21] ONLP_THERMAL_PSU_0
 *            |                  |----[06] ONLP_PSU_0_FAN
 *            |----[02] ONLP_PSU_1----[22] ONLP_THERMAL_PSU_1
 *            |                  |----[07] ONLP_PSU_1_FAN
 *            |
 *            |----[01] ONLP_FAN_0
 *            |----[02] ONLP_FAN_1
 *            |----[03] ONLP_FAN_2
 *            |----[04] ONLP_FAN_3
 *            |----[05] ONLP_FAN_4
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
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_DDR4),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_BMC),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_FANCARD1),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_FANCARD2),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_FPGA_R),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_FPGA_L),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_HWM_GDDR),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_HWM_MAC),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_HWM_AMB),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_HWM_NTMCARD),

    ONLP_LED_ID_CREATE(ONLP_LED_SYS_GNSS),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_SYNC),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_SYS),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_FAN),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_PWR),
    ONLP_LED_ID_CREATE(ONLP_LED_FLEXE_0),
    ONLP_LED_ID_CREATE(ONLP_LED_FLEXE_1),

    ONLP_PSU_ID_CREATE(ONLP_PSU_0),
    ONLP_PSU_ID_CREATE(ONLP_PSU_1),

    ONLP_FAN_ID_CREATE(ONLP_FAN_0),
    ONLP_FAN_ID_CREATE(ONLP_FAN_1),
    ONLP_FAN_ID_CREATE(ONLP_FAN_2),
    ONLP_FAN_ID_CREATE(ONLP_FAN_3),
    ONLP_FAN_ID_CREATE(ONLP_FAN_4),
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

    /** Add 2 QSFPDD, 2QSFP and 24 SFP+ OIDs after the static table */
    onlp_oid_t* e = hdr->coids + AIM_ARRAYSIZE(__onlp_oid_info);

    /* 2 QSFPDD + 40 2QSFP + 24 SFP+ */
    for(i = 1; i <= 28; i++) {
        *e++ = ONLP_SFP_ID_CREATE(i);
    }

    return ONLP_STATUS_OK;
}
