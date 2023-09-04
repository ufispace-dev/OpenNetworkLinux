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
 * ONLP System Platform Interface.
 *
 ***********************************************************/
#include <onlp/platformi/sysi.h>
#include "platform_lib.h"

/* EEPROM */
#define SYS_EEPROM_PATH         SYS_DEV "1-0056/eeprom"
#define SYS_EEPROM_SIZE         512

/* This is definitions for x86-64-ufispace-s6301-56st */
/* OID map*/
/*
 * [01] CHASSIS
 *            |----[01] ONLP_THERMAL_FAN1
 *            |----[02] ONLP_THERMAL_FAN2
 *            |----[03] ONLP_THERMAL_PSUDB
 *            |----[04] ONLP_THERMAL_MAC
 *            |----[05] ONLP_THERMAL_INLET
 *            |----[06] ONLP_THERMAL_PSU0
 *            |----[07] ONLP_THERMAL_PSU1
 *            |----[08] ONLP_THERMAL_CPU_PKG
 *            |
 *            |----[01] ONLP_LED_ID
 *            |----[02] ONLP_LED_SYS
 *            |----[03] ONLP_LED_POE
 *            |----[04] ONLP_LED_SPD
 *            |----[05] ONLP_LED_FAN
 *            |----[05] ONLP_LED_LNK
 *            |----[05] ONLP_LED_PWR1
 *            |----[05] ONLP_LED_PWR0
 *            |
 *            |----[01] ONLP_PSU_0----[06] ONLP_THERMAL_PSU0
 *            |                  |----[03] ONLP_PSU_0_FAN
 *            |----[02] ONLP_PSU_1----[09] ONLP_THERMAL_PSU1
 *            |                  |----[04] ONLP_PSU_1_FAN
 *            |
 *            |----[01] ONLP_FAN_0
 *            |----[02] ONLP_FAN_1
 *            |----[03] ONLP_PSU_0_FAN
 *            |----[04] ONLP_PSU_1_FAN
 */

static onlp_oid_t __onlp_oid_info[] = {
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_FAN1),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_FAN2),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSUDB),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_INLET),
    //ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU0),
    //ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU1),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PKG),

    ONLP_LED_ID_CREATE(ONLP_LED_ID),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS),
    ONLP_LED_ID_CREATE(ONLP_LED_POE),
    ONLP_LED_ID_CREATE(ONLP_LED_SPD),
    ONLP_LED_ID_CREATE(ONLP_LED_FAN),
    ONLP_LED_ID_CREATE(ONLP_LED_LNK),
    ONLP_LED_ID_CREATE(ONLP_LED_PWR1),
    ONLP_LED_ID_CREATE(ONLP_LED_PWR0),

    ONLP_PSU_ID_CREATE(ONLP_PSU_0),
    ONLP_PSU_ID_CREATE(ONLP_PSU_1),

    ONLP_FAN_ID_CREATE(ONLP_FAN_0),
    ONLP_FAN_ID_CREATE(ONLP_FAN_1),
    //ONLP_FAN_ID_CREATE(ONLP_PSU_0_FAN),
    //ONLP_FAN_ID_CREATE(ONLP_PSU_1_FAN),
};

static int sysi_platform_info_get(onlp_platform_info_t* pi)
{
    uint8_t mb_cpld_ver_h[16];
    uint8_t bios_ver_h[32];
    char psu0_fw_ver_h[32];
    char psu1_fw_ver_h[32];
    char ucd_fw_ver_h[32];
    int sku_id, hw_id, id_type, build_id, deph_id, ext_id;
    int data_len;
    int size;

    memset(mb_cpld_ver_h, 0, sizeof(mb_cpld_ver_h));
    memset(bios_ver_h, 0, sizeof(bios_ver_h));
    memset(psu0_fw_ver_h, 0, sizeof(psu0_fw_ver_h));
    memset(psu1_fw_ver_h, 0, sizeof(psu1_fw_ver_h));
    memset(ucd_fw_ver_h, 0, sizeof(ucd_fw_ver_h));

    //get MB CPLD version from CPLD sysfs
    ONLP_TRY(onlp_file_read(mb_cpld_ver_h, sizeof(mb_cpld_ver_h), &data_len,
            LPC_MB_CPLD_PATH LPC_MB_CPLD_VER_ATTR));
    //trim new line
    mb_cpld_ver_h[strcspn((char *)mb_cpld_ver_h, "\n" )] = '\0';

    //get psu firmware version
    ONLP_TRY(psu_fw_ver_get(psu0_fw_ver_h, sizeof(psu0_fw_ver_h), ONLP_PSU_0));
    ONLP_TRY(psu_fw_ver_get(psu1_fw_ver_h, sizeof(psu1_fw_ver_h), ONLP_PSU_1));

    //get ucd firmware version
    ONLP_TRY(ucd_fw_ver_get(ucd_fw_ver_h, sizeof(ucd_fw_ver_h)));

    pi->cpld_versions = aim_fstrdup(
        "\n"
        "[MB CPLD] %s\n"
        "[PSU0 FW VER] %s\n"
        "[PSU1 FW VER] %s\n"
        "[UCD FW VER] %s\n",
        mb_cpld_ver_h, psu0_fw_ver_h, psu1_fw_ver_h, ucd_fw_ver_h);

    //get HW Build Version
    ONLP_TRY(file_read_hex(&sku_id, LPC_MB_CPLD_PATH "/" LPC_MB_SKU_ID_ATTR));
    ONLP_TRY(file_read_hex(&hw_id, LPC_MB_CPLD_PATH "/" LPC_MB_HW_ID_ATTR));
    ONLP_TRY(file_read_hex(&id_type, LPC_MB_CPLD_PATH "/" LPC_MB_ID_TYPE_ATTR));
    ONLP_TRY(file_read_hex(&build_id, LPC_MB_CPLD_PATH "/" LPC_MB_BUILD_ID_ATTR));
    ONLP_TRY(file_read_hex(&deph_id, LPC_MB_CPLD_PATH "/" LPC_MB_DEPH_ID_ATTR));
    ONLP_TRY(file_read_hex(&ext_id, LPC_MB_CPLD_PATH "/" LPC_MB_EXT_ID_ATTR));

    //get BIOS version
    ONLP_TRY(onlp_file_read(bios_ver_h, sizeof(bios_ver_h), &size, SYSFS_BIOS_VER));
    //trim new line
    bios_ver_h[strcspn((char *)bios_ver_h, "\n" )] = '\0';

    pi->other_versions = aim_fstrdup(
        "\n"
        "[SKU ID] %d\n"
        "[HW ID] %d\n"
        "[BUILD ID] %d\n"
        "[ID TYPE] %d\n"
        "[DEPH ID] %d\n"
        "[EXTEND ID] %d\n"
        "[BIOS] %s\n",
        sku_id, hw_id, build_id, id_type, deph_id, ext_id, bios_ver_h);

    return ONLP_STATUS_OK;
}


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
const char* onlp_sysi_platform_get(void)
{
    return "x86-64-ufispace-s6301-56st-r0";
}

/**
 * @brief Attempt to set the platform personality
 * in the event that the current platform does not match the
 * reported platform.
 * @note Optional
 */
int onlp_sysi_platform_set(const char* platform)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Initialize the system platform subsystem.
 */
int onlp_sysi_init(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Provide the physical base address for the ONIE eeprom.
 * @param param [out] physaddr Receives the physical address.
 * @notes If your platform provides a memory-mappable base
 * address for the ONIE eeprom data you can return it here.
 * The ONLP common code will then use this address and decode
 * the ONIE TLV specification data. If you cannot return a mappable
 * address due to the platform organization see onlp_sysi_onie_data_get()
 * instead.
 */
 int onlp_sysi_onie_data_phys_addr_get(void** physaddr)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Return the raw contents of the ONIE system eeprom.
 * @param data [out] Receives the data pointer to the ONIE data.
 * @param size [out] Receives the size of the data (if available).
 * @notes This function is only necessary if you cannot provide
 * the physical base address as per onlp_sysi_onie_data_phys_addr_get().
 */
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

/**
 * @brief Free the data returned by onlp_sys_onie_data_get()
 * @param data The data pointer.
 * @notes If onlp_sysi_onie_data_get() is called to retreive the
 * contents of the ONIE system eeprom then this function
 * will be called to perform any cleanup that may be necessary
 * after the data has been used.
 */
void onlp_sysi_onie_data_free(uint8_t* data)
{
    if(data) {
        aim_free(data);
    }
}

/**
 * @brief Return the ONIE system information for this platform.
 * @param onie The onie information structure.
 * @notes If all previous attempts to get the eeprom data fail
 * then this routine will be called. Used as a translation option
 * for platforms without access to an ONIE-formatted eeprom.
 */
int onlp_sysi_onie_info_get(onlp_onie_info_t* onie)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief This function returns the root oid list for the platform.
 * @param table [out] Receives the table.
 * @param max The maximum number of entries you can fill.
 */
int onlp_sysi_oids_get(onlp_oid_t* table, int max)
{
    memset(table, 0, max*sizeof(onlp_oid_t));
    memcpy(table, __onlp_oid_info, sizeof(__onlp_oid_info));

    return ONLP_STATUS_OK;
}


/**
 * @brief This function provides a generic ioctl interface.
 * @param code context dependent.
 * @param vargs The variable argument list for the ioctl call.
 * @notes This is provided as a generic expansion and
 * and custom programming mechanism for future and non-standard
 * functionality.
 * @notes Optional
 */
int onlp_sysi_ioctl(int code, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Platform management initialization.
 */
int onlp_sysi_platform_manage_init(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Perform necessary platform fan management.
 * @note This function should automatically adjust the FAN speeds
 * according to the platform conditions.
 */
int onlp_sysi_platform_manage_fans(void)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Perform necessary platform LED management.
 * @note This function should automatically adjust the LED indicators
 * according to the platform conditions.
 */
int onlp_sysi_platform_manage_leds(void)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Return custom platform information.
 */
int onlp_sysi_platform_info_get(onlp_platform_info_t* info)
{
    ONLP_TRY(sysi_platform_info_get(info));

    return ONLP_STATUS_OK;
}

/**
 * @brief Friee a custom platform information structure.
 */
void onlp_sysi_platform_info_free(onlp_platform_info_t* info)
{
    if(info && info->cpld_versions) {
        aim_free(info->cpld_versions);
    }

    if(info && info->other_versions) {
        aim_free(info->other_versions);
    }
}

/**
 * @brief Builtin platform debug tool.
 */
int onlp_sysi_debug(aim_pvs_t* pvs, int argc, char** argv)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}
