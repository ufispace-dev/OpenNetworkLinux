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

/* This is definitions for x86-64-ufispace-s9601-104bc */
/* OID map*/
/*
 * [01] CHASSIS
 *            |----[01] ONLP_THERMAL_PSU_ENV
 *            |----[02] ONLP_THERMAL_CPU_ENV
 *            |----[03] ONLP_THERMAL_OP2_ENV
 *            |----[04] ONLP_THERMAL_MAC_ENV
 *            |----[05] ONLP_THERMAL_MAC_DIE
 *            |----[08] ONLP_THERMAL_CPU_PKG
 *            |
 *            |----[01] ONLP_LED_SYS_SYNC
 *            |----[02] ONLP_LED_SYS_SYS
 *            |----[03] ONLP_LED_SYS_FAN
 *            |----[04] ONLP_LED_SYS_PSU0
 *            |----[05] ONLP_LED_SYS_PSU1
 *            |----[06] ONLP_LED_SYS_ID
 *            |
 *            |----[01] ONLP_PSU_0----[06] ONLP_THERMAL_PSU0
 *            |                  |----[05] ONLP_PSU_0_FAN
 *            |----[02] ONLP_PSU_1----[07] ONLP_THERMAL_PSU1
 *            |                  |----[06] ONLP_PSU_1_FAN
 *            |
 *            |----[01] ONLP_FAN_0
 *            |----[02] ONLP_FAN_1
 *            |----[03] ONLP_FAN_2
 *            |----[04] ONLP_FAN_3
 */

static onlp_oid_t __onlp_oid_info[] = {
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU_ENV),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_ENV),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_OP2_ENV),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC_ENV),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC_DIE),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PKG),

    ONLP_LED_ID_CREATE(ONLP_LED_SYS_SYNC),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_SYS),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_FAN),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_PSU0),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_PSU1),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_ID),

    ONLP_PSU_ID_CREATE(ONLP_PSU_0),
    ONLP_PSU_ID_CREATE(ONLP_PSU_1),

    ONLP_FAN_ID_CREATE(ONLP_FAN_0),
    ONLP_FAN_ID_CREATE(ONLP_FAN_1),
    ONLP_FAN_ID_CREATE(ONLP_FAN_2),
    ONLP_FAN_ID_CREATE(ONLP_FAN_3),
};

#define SYS_EEPROM_PATH    "/sys/bus/i2c/devices/4-0057/eeprom"
#define SYS_EEPROM_SIZE    512
#define SYSFS_CPLD1_VER_H   SYSFS_CPLD1 "cpld_version_h"
#define SYSFS_CPLD2_VER_H   SYSFS_CPLD2 "cpld_version_h"
#define SYSFS_CPLD3_VER_H   SYSFS_CPLD3 "cpld_version_h"
#define SYSFS_CPLD4_VER_H   SYSFS_CPLD4 "cpld_version_h"
#define SYSFS_CPLD5_VER_H   SYSFS_CPLD5 "cpld_version_h"
#define SYSFS_BIOS_VER     "/sys/class/dmi/id/bios_version"

#define CMD_BMC_VER_1      "expr `ipmitool mc info"IPMITOOL_REDIRECT_ERR" | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f1` + 0"
#define CMD_BMC_VER_2      "expr `ipmitool mc info"IPMITOOL_REDIRECT_ERR" | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f2` + 0"
#define CMD_BMC_VER_3      "echo $((`ipmitool mc info"IPMITOOL_REDIRECT_ERR" | grep 'Aux Firmware Rev Info' -A 2 | sed -n '2p'` + 0))"

static int get_platform_info(onlp_platform_info_t* pi)
{
    board_t board = {0};
    int len = 0;
    char bios_out[ONLP_CONFIG_INFO_STR_MAX] = {'\0'};
    char bmc_out1[8] = {0}, bmc_out2[8] = {0}, bmc_out3[8] = {0};

    //get MB CPLD version
    char mb_cpld1_ver[ONLP_CONFIG_INFO_STR_MAX] = {'\0'};
    ONLP_TRY(onlp_file_read((uint8_t*)&mb_cpld1_ver, ONLP_CONFIG_INFO_STR_MAX -1, &len, SYSFS_CPLD1_VER_H));

    char mb_cpld2_ver[ONLP_CONFIG_INFO_STR_MAX] = {'\0'};
    ONLP_TRY(onlp_file_read((uint8_t*)&mb_cpld2_ver, ONLP_CONFIG_INFO_STR_MAX -1, &len, SYSFS_CPLD2_VER_H));

    char mb_cpld3_ver[ONLP_CONFIG_INFO_STR_MAX] = {'\0'};
    ONLP_TRY(onlp_file_read((uint8_t*)&mb_cpld3_ver, ONLP_CONFIG_INFO_STR_MAX -1, &len, SYSFS_CPLD3_VER_H));

    char mb_cpld4_ver[ONLP_CONFIG_INFO_STR_MAX] = {'\0'};
    ONLP_TRY(onlp_file_read((uint8_t*)&mb_cpld4_ver, ONLP_CONFIG_INFO_STR_MAX -1, &len, SYSFS_CPLD4_VER_H));

    char mb_cpld5_ver[ONLP_CONFIG_INFO_STR_MAX] = {'\0'};
    ONLP_TRY(onlp_file_read((uint8_t*)&mb_cpld5_ver, ONLP_CONFIG_INFO_STR_MAX -1, &len, SYSFS_CPLD5_VER_H));

    pi->cpld_versions = aim_fstrdup(
        "\n"
        "[MB CPLD1] %s\n"
        "[MB CPLD2] %s\n"
        "[MB CPLD3] %s\n"
        "[MB CPLD4] %s\n"
        "[MB CPLD5] %s\n",
        mb_cpld1_ver,
        mb_cpld2_ver,
        mb_cpld3_ver,
        mb_cpld4_ver,
        mb_cpld5_ver);

    //Get HW Version
    ONLP_TRY(get_board_version(&board));

    //Get BIOS version
    char tmp_str[ONLP_CONFIG_INFO_STR_MAX] = {'\0'};
    ONLP_TRY(onlp_file_read((uint8_t*)&tmp_str, ONLP_CONFIG_INFO_STR_MAX - 1, &len, SYSFS_BIOS_VER));

    // Remove '\n'
    sscanf (tmp_str, "%[^\n]", bios_out);

    // Detect bmc status
    if(check_bmc_alive() != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Timeout, BMC did not respond.\n");
        return ONLP_STATUS_E_INTERNAL;
    }

    //Get BMC version
    if (exec_cmd(CMD_BMC_VER_1, bmc_out1, sizeof(bmc_out1)) < 0 ||
        exec_cmd(CMD_BMC_VER_2, bmc_out2, sizeof(bmc_out2)) < 0 ||
        exec_cmd(CMD_BMC_VER_3, bmc_out3, sizeof(bmc_out3))) {
            AIM_LOG_ERROR("unable to read BMC version\n");
            return ONLP_STATUS_E_INTERNAL;
    }

    pi->other_versions = aim_fstrdup(
        "\n"
        "[SKU ID] %d\n"
        "[HW ID] %d\n"
        "[BUILD ID] %d\n"
        "[ID TYPE] %d\n"
        "[DEPH ID] %d\n"
        "[BIOS ] %s\n"
        "[BMC  ] %d.%d.%d\n",
        board.sku_id,
        board.hw_rev,
        board.hw_build,
        board.id_type,
        board.deph_id,
        bios_out,
        atoi(bmc_out1), atoi(bmc_out2), atoi(bmc_out3));

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
    return "x86-64-ufispace-s9601-104bc-r0";
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
    if (data) {
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
    return ONLP_STATUS_E_UNSUPPORTED;
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
    if (get_platform_info(info) < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Friee a custom platform information structure.
 */
void onlp_sysi_platform_info_free(onlp_platform_info_t* info)
{
    if (info && info->cpld_versions) {
        aim_free(info->cpld_versions);
    }

    if (info && info->other_versions) {
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

