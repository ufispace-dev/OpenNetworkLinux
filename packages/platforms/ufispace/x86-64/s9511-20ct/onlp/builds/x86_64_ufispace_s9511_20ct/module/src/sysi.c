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
#include <unistd.h>
#include <onlp/platformi/sysi.h>
#include "platform_lib.h"

/* This is definitions for x86-64-ufispace-s9511-20ct*/
/* OID map*/
/*
 * [01] CHASSIS - AC PSU
 *            |----[01] ONLP_THERMAL_CPU_PKG
 *            |----[02] ONLP_THERMAL_MAC
 *            |----[03] ONLP_THERMAL_GDDR6
 *            |----[04] ONLP_THERMAL_NTM
 *            |----[05] ONLP_THERMAL_TEMP_MAC
 *            |----[06] ONLP_THERMAL_TEMP_AMB
 *            |----[07] ONLP_THERMAL_TEMP_PHY
 *            |
 *            |----[01] ONLP_LED_SYS_GNSS
 *            |----[02] ONLP_LED_SYS_SYNC
 *            |----[03] ONLP_LED_SYS_STAT
 *            |----[04] ONLP_LED_SYS_FAN
 *            |----[05] ONLP_LED_SYS_PWR
 *            |
 *            |----[01] ONLP_PSU_0----[08] ONLP_THERMAL_PSU_0
 *            |                  |----[04] ONLP_PSU_0_FAN
 *            |----[02] ONLP_PSU_1----[09] ONLP_THERMAL_PSU_1
 *                               |----[05] ONLP_PSU_1_FAN
 *            |
 *            |----[01] ONLP_FAN_0
 *            |----[02] ONLP_FAN_1
 *            |----[03] ONLP_FAN_2
 * ************************************************************************************
 * [01] CHASSIS - DC PSU
 *            |----[01] ONLP_THERMAL_CPU_PKG
 *            |----[02] ONLP_THERMAL_MAC
 *            |----[03] ONLP_THERMAL_GDDR6
 *            |----[04] ONLP_THERMAL_NTM
 *            |----[05] ONLP_THERMAL_TEMP_MAC
 *            |----[06] ONLP_THERMAL_TEMP_AMB
 *            |----[07] ONLP_THERMAL_TEMP_PHY
 *            |
 *            |----[01] ONLP_LED_SYS_GNSS
 *            |----[02] ONLP_LED_SYS_SYNC
 *            |----[03] ONLP_LED_SYS_STAT
 *            |----[04] ONLP_LED_SYS_FAN
 *            |----[05] ONLP_LED_SYS_PWR
 *            |
 *            |----[01] ONLP_DUAL_PSU_0_PLUG_0----[08] ONLP_THERMAL_PSU_0
 *            |----[02] ONLP_DUAL_PSU_0_PLUG_1----[08] ONLP_THERMAL_PSU_0
 *            |
 *            |----[01] ONLP_FAN_0
 *            |----[02] ONLP_FAN_1
 *            |----[03] ONLP_FAN_2
 *            |----[04] ONLP_FAN_3
 */

static onlp_oid_t __onlp_oid_info_ac[] = {
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PKG),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAINBOARD_MAC),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAINBOARD_GDDR6),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAINBOARD_NTM),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_VMON_HWM_MAC),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_VMON_HWM_AMB),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_VMON_HWM_PHY),

    ONLP_LED_ID_CREATE(ONLP_LED_SYS_GNSS),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_SYNC),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_STAT),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_FAN),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_PWR),

    ONLP_PSU_ID_CREATE(ONLP_PSU_0),
    ONLP_PSU_ID_CREATE(ONLP_PSU_1),

    ONLP_FAN_ID_CREATE(ONLP_FAN_0),
    ONLP_FAN_ID_CREATE(ONLP_FAN_1),
    ONLP_FAN_ID_CREATE(ONLP_FAN_2),
};

static onlp_oid_t __onlp_oid_info_dc[] = {
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PKG),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAINBOARD_MAC),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAINBOARD_GDDR6),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAINBOARD_NTM),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_VMON_HWM_MAC),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_VMON_HWM_AMB),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_VMON_HWM_PHY),

    ONLP_LED_ID_CREATE(ONLP_LED_SYS_GNSS),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_SYNC),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_STAT),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_FAN),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_PWR),

    ONLP_PSU_ID_CREATE(ONLP_DUAL_PSU_0_PLUG_0),
    ONLP_PSU_ID_CREATE(ONLP_DUAL_PSU_0_PLUG_1),

    ONLP_FAN_ID_CREATE(ONLP_FAN_0),
    ONLP_FAN_ID_CREATE(ONLP_FAN_1),
    ONLP_FAN_ID_CREATE(ONLP_FAN_2),
    ONLP_FAN_ID_CREATE(ONLP_FAN_3),
};

#define SYS_EEPROM_PATH     "/sys/bus/i2c/devices/1-0057/eeprom"
#define SYS_EEPROM_SIZE     512
#define SYSFS_CPLD1_VER_H   SYSFS_CPLD1 "cpld_version_h"
#define SYSFS_CPLD2_VER_H   SYSFS_CPLD2 "cpld_version_h"
#define SYSFS_HW_ID         LPC_FMT "hw_rev"
#define SYSFS_BUILD_ID      LPC_FMT "build_id"
#define SYSFS_BIOS_VER      "/sys/class/dmi/id/bios_version"

/******************************************************************************************************************
**                                                                                                               **
**                                           Upispace Specific Defined APIs                                      **
**                                                                                                               **
*******************************************************************************************************************/
static int get_platform_info(onlp_platform_info_t* pi)
{
    int len = 0;
    char bios_out[ONLP_CONFIG_INFO_STR_MAX] = {'\0'};

    /* get MB CPLD version */
    char mb_cpld1_ver[ONLP_CONFIG_INFO_STR_MAX] = {'\0'};
    ONLP_TRY(onlp_file_read((uint8_t*)&mb_cpld1_ver, ONLP_CONFIG_INFO_STR_MAX -1, &len, SYSFS_CPLD1_VER_H));

    char mb_cpld2_ver[ONLP_CONFIG_INFO_STR_MAX] = {'\0'};
    ONLP_TRY(onlp_file_read((uint8_t*)&mb_cpld2_ver, ONLP_CONFIG_INFO_STR_MAX -1, &len, SYSFS_CPLD2_VER_H));

    pi->cpld_versions = aim_fstrdup(
        "\n"
        "[MB CPLD1] %s\n"
        "[MB CPLD2] %s\n",
        mb_cpld1_ver,
        mb_cpld2_ver);


    /* Get BIOS version */
    char tmp_str[ONLP_CONFIG_INFO_STR_MAX] = {'\0'};
    ONLP_TRY(onlp_file_read((uint8_t*)&tmp_str, ONLP_CONFIG_INFO_STR_MAX - 1, &len, SYSFS_BIOS_VER));

    /* Remove '\n' */
    sscanf (tmp_str, "%[^\n]", bios_out);

    char mu_ver[128] = {'\0'}, mu_result[128] = {'\0'};
    char path_onie_folder[] = "/mnt/onie-boot/onie";
    char path_onie_update_log[] = "/mnt/onie-boot/onie/update/update_details.log";
    char cmd_mount_mu_dir[] = "mkdir -p /mnt/onie-boot && mount LABEL=ONIE-BOOT /mnt/onie-boot/ 2> /dev/null";
    char cmd_mu_ver[] = "cat /mnt/onie-boot/onie/update/update_details.log | grep -i 'Updater version:' | tail -1 | awk -F ' ' '{ print $3}' | tr -d '\\r\\n'";
    char cmd_mu_result_template[] = "/mnt/onie-boot/onie/tools/bin/onie-fwpkg | grep '%s' | awk -F '|' '{ print $3 }' | tail -1 | xargs | tr -d '\\r\\n'";
    char cmd_mu_result[256] = {'\0'};

    //Mount MU Folder
    if(access(path_onie_folder, F_OK) == -1 )
        system(cmd_mount_mu_dir);

    //Get MU Version
    if(access(path_onie_update_log, F_OK) != -1 ) {
        exec_cmd(cmd_mu_ver, mu_ver, sizeof(mu_ver));

        if (strnlen(mu_ver, sizeof(mu_ver)) != 0) {
            snprintf(cmd_mu_result, sizeof(cmd_mu_result), cmd_mu_result_template, mu_ver);
            exec_cmd(cmd_mu_result, mu_result, sizeof(mu_result));
        }
    }

    pi->other_versions = aim_fstrdup(
        "\n"
        "[BIOS] %s\n"
        "[MU] %s (%s)\n",
        bios_out,
        strnlen(mu_ver, sizeof(mu_ver)) != 0 ? mu_ver : "NA", mu_result);

    return ONLP_STATUS_OK;
}

/******************************************************************************************************************
**                                                                                                               **
**                                                ONLP Standard APIs                                             **
**                                                                                                               **
*******************************************************************************************************************/
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
    return "x86-64-ufispace-s9511-20ct-r0";
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

    /* Check PSU Type */
    int psu_type = ONLP_PSU_TYPE_INVALID;
    ONLP_TRY(get_psu_type(&psu_type));

    /* Check Board Version */
    board_t board = {0};
    ONLP_TRY(get_board_version(&board));

    if(psu_type == ONLP_PSU_TYPE_AC) {
        memset(table, 0, max*sizeof(onlp_oid_t));
        memcpy(table, __onlp_oid_info_ac, sizeof(__onlp_oid_info_ac));
    } else if(psu_type == ONLP_PSU_TYPE_DC48){
        memset(table, 0, max*sizeof(onlp_oid_t));
        memcpy(table, __onlp_oid_info_dc, sizeof(__onlp_oid_info_dc));
    } else {
        AIM_LOG_ERROR("Unknown psu_type (%d)\n", psu_type);
        return ONLP_STATUS_E_INTERNAL;
    }

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
