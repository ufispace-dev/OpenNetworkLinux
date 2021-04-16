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

bool bmc_enable = false;

/**
 * @brief Return the name of the the platform implementation.
 * @notes This will be called PRIOR to any other calls into the
 * platform driver, including the sysi_init() function below.
 *
 * The platform implementation name should match the current
 * ONLP platform name.
 *
 * IF the platform implementation name equals the current platform name,
 * initialization will continue.
 *
 * If the platform implementation name does not match, the following will be
 * attempted:
 *
 *    onlp_sysi_platform_set(current_platform_name);
 * If this call is successful, initialization will continue.
 * If this call fails, platform initialization will abort().
 *
 * The onlp_sysi_platform_set() function is optional.
 * The onlp_sysi_platform_get() is not optional.
 */
const char*
onlp_sysi_platform_get(void)
{
    return "x86-64-ufispace-s9600-32x-r0";
}

/**
 * @brief Attempt to set the platform personality
 * in the event that the current platform does not match the
 * reported platform.
 * @note Optional
 */
int
onlp_sysi_platform_set(const char* platform)
{
    //AIM_LOG_INFO("Set ONL platform interface to '%s'\n", platform);
    //AIM_LOG_INFO("Real HW Platform: '%s'\n", onlp_sysi_platform_get());
    return ONLP_STATUS_OK;
}

/**
 * @brief Initialize the system platform subsystem.
 */
int
onlp_sysi_init(void)
{
    /* check if the platform is bmc enabled */
    if ( onlp_sysi_bmc_en_get() ) {
        bmc_enable = true;
    } else {
        bmc_enable = false;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Return the raw contents of the ONIE system eeprom.
 * @param data [out] Receives the data pointer to the ONIE data.
 * @param size [out] Receives the size of the data (if available).
 * @notes This function is only necessary if you cannot provide
 * the physical base address as per onlp_sysi_onie_data_phys_addr_get().
 */
int
onlp_sysi_onie_data_get(uint8_t** data, int* size)
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

/**
 * @brief Free the data returned by onlp_sys_onie_data_get()
 * @param data The data pointer.
 * @notes If onlp_sysi_onie_data_get() is called to retreive the
 * contents of the ONIE system eeprom then this function
 * will be called to perform any cleanup that may be necessary
 * after the data has been used.
 */
void
onlp_sysi_onie_data_free(uint8_t* data)
{
    if (data) {
        aim_free(data);
    }
}

/**
 * @brief This function returns the root oid list for the platform.
 * @param table [out] Receives the table.
 * @param max The maximum number of entries you can fill.
 */
int
onlp_sysi_oids_get(onlp_oid_t* table, int max)
{
    onlp_oid_t* e = table;
    memset(table, 0, max*sizeof(onlp_oid_t));
    int i;

    if ( !bmc_enable ) {
        /* 2 PSUs */
        *e++ = ONLP_PSU_ID_CREATE(1);
        *e++ = ONLP_PSU_ID_CREATE(2);

        /* LEDs Item */
        for (i=1; i<=LED_NUM; i++) {
            *e++ = ONLP_LED_ID_CREATE(i);
        }

        /* Fans Item */
        for (i=1; i<=FAN_NUM; i++) {
            *e++ = ONLP_FAN_ID_CREATE(i);
        }
    }

    /* THERMALs Item */
    if ( !bmc_enable ) {
        for (i=1; i<=THERMAL_NUM; i++) {
            *e++ = ONLP_THERMAL_ID_CREATE(i);
        }
    } else {
        *e++ = THERMAL_OID_CPU_PECI;
        //*e++ = THERMAL_OID_ENV;
        //*e++ = THERMAL_OID_FRONT_ENV;
        *e++ = THERMAL_OID_Q2CL_ENV;
        *e++ = THERMAL_OID_Q2CL_DIE;
        *e++ = THERMAL_OID_REAR_ENV_1;
        *e++ = THERMAL_OID_REAR_ENV_2;
        //*e++ = THERMAL_OID_CPU_PKG;
        //*e++ = THERMAL_OID_CPU1;
        //*e++ = THERMAL_OID_CPU2;
        //*e++ = THERMAL_OID_CPU3;
        //*e++ = THERMAL_OID_CPU4;
        //*e++ = THERMAL_OID_CPU5;
        //*e++ = THERMAL_OID_CPU6;
        //*e++ = THERMAL_OID_CPU7;
        //*e++ = THERMAL_OID_CPU8;
        //*e++ = THERMAL_OID_CPU_BOARD;

        *e++ = LED_OID_SYNC;
        *e++ = LED_OID_SYS;
        *e++ = LED_OID_FAN;
        *e++ = LED_OID_PSU0;
        *e++ = LED_OID_PSU1;
        *e++ = LED_OID_ID;


        *e++ = PSU_OID_PSU0;
        *e++ = PSU_OID_PSU1;

        *e++ = FAN_OID_FAN0;
        *e++ = FAN_OID_FAN1;
        *e++ = FAN_OID_FAN2;
        *e++ = FAN_OID_FAN3;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Perform necessary platform fan management.
 * @note This function should automatically adjust the FAN speeds
 * according to the platform conditions.
 */
int
onlp_sysi_platform_manage_fans(void)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Perform necessary platform LED management.
 * @note This function should automatically adjust the LED indicators
 * according to the platform conditions.
 */
int
onlp_sysi_platform_manage_leds(void)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Return custom platform information.
 */
int
onlp_sysi_platform_info_get(onlp_platform_info_t* pi)
{
    int rc;
    if ((rc = sysi_platform_info_get(pi)) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Friee a custom platform information structure.
 */
void
onlp_sysi_platform_info_free(onlp_platform_info_t* pi)
{
    if (pi->cpld_versions) {
        aim_free(pi->cpld_versions);
    }

    if (pi->other_versions) {
        aim_free(pi->other_versions);
    }
}

