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
#include <onlp/platformi/sysi.h>
#include <onlp/platformi/ledi.h>
#include <onlp/platformi/psui.h>
#include <onlp/platformi/thermali.h>
#include <onlp/platformi/fani.h>
#include <onlplib/file.h>
#include <onlplib/crc32.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "platform_lib.h"

const char* onlp_sysi_platform_get(void)
{
    return "x86-64-ingrasys-s9280-64x-4bwb-r0";
}

int onlp_sysi_platform_set(const char* platform)
{
    //AIM_LOG_INFO("Set ONL platform interface to '%s'\n", platform);
    //AIM_LOG_INFO("Real HW Platform: '%s'\n", onlp_sysi_platform_get());
    return ONLP_STATUS_OK;
}

int onlp_sysi_init(void)
{
    return ONLP_STATUS_OK;
}

int onlp_sysi_onie_data_get(uint8_t** data, int* size)
{
    uint8_t* rdata = aim_zmalloc(SYS_EEPROM_SIZE);
    if(onlp_file_read(rdata, SYS_EEPROM_SIZE, size, SYS_EEPROM_PATH) == ONLP_STATUS_OK) {
        if(*size == SYS_EEPROM_SIZE) {
            *data = rdata;
            return ONLP_STATUS_OK;
        }
    }

    AIM_LOG_INFO("Unable to get data from eeprom \n");
    aim_free(rdata);
    *size = 0;
    return ONLP_STATUS_E_INTERNAL;
}

void onlp_sysi_onie_data_free(uint8_t* data)
{
    if (data) {
        aim_free(data);
    }
}

int onlp_sysi_oids_get(onlp_oid_t* table, int max)
{
    onlp_oid_t* e = table;
    memset(table, 0, max*sizeof(onlp_oid_t));

    // THERMAL
    *e++ = THERMAL_OID_CPU_PECI;
    *e++ = THERMAL_OID_BMC_ENV;
    *e++ = THERMAL_OID_PCIE_CONN;
    *e++ = THERMAL_OID_CAGE_R;
    *e++ = THERMAL_OID_CAGE_L;
    *e++ = THERMAL_OID_MAC_FRONT;
    *e++ = THERMAL_OID_MAC_ENV;
    *e++ = THERMAL_OID_MAC_DIE;
    *e++ = THERMAL_OID_PSU1;
    *e++ = THERMAL_OID_PSU2;
    *e++ = THERMAL_OID_CPU_BOARD;
    *e++ = THERMAL_OID_CPU_PKG;
    *e++ = THERMAL_OID_CPU1;
    *e++ = THERMAL_OID_CPU2;
    *e++ = THERMAL_OID_CPU3;
    *e++ = THERMAL_OID_CPU4;
    *e++ = THERMAL_OID_CPU5;
    *e++ = THERMAL_OID_CPU6;
    *e++ = THERMAL_OID_CPU7;
    *e++ = THERMAL_OID_CPU8;
    // LED
    #ifdef ENABLE_SYSLED
    *e++ = LED_OID_SYSTEM;
    *e++ = LED_OID_FAN;
    *e++ = LED_OID_PSU1;
    *e++ = LED_OID_PSU2;
    #endif
    // PSU
    *e++ = PSU_OID_PSU1;
    *e++ = PSU_OID_PSU2;
    // FAN
    *e++ = FAN_OID_FAN1;
    *e++ = FAN_OID_FAN2;
    *e++ = FAN_OID_FAN3;
    *e++ = FAN_OID_FAN4;
    *e++ = FAN_OID_PSU1_FAN1;
    *e++ = FAN_OID_PSU2_FAN1;

    return ONLP_STATUS_OK;
}

int onlp_sysi_platform_manage_fans(void)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int onlp_sysi_platform_manage_leds(void)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int onlp_sysi_platform_info_get(onlp_platform_info_t* pi)
{
    int rc;
    if ((rc = sysi_platform_info_get(pi)) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

void onlp_sysi_platform_info_free(onlp_platform_info_t* pi)
{
    if (pi->cpld_versions) {
        aim_free(pi->cpld_versions);
    }

    if (pi->other_versions) {
        aim_free(pi->other_versions);
    }
}

