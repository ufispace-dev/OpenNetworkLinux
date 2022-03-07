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

#define SYS_EEPROM_PATH    "/sys/bus/i2c/devices/1-0057/eeprom"
#define SYS_EEPROM_SIZE    512
#define SYSFS_CPLD_VER_H   LPC_FMT "cpld_version_h"
#define SYSFS_HW_ID        LPC_FMT "board_hw_id"
#define SYSFS_BUILD_ID     LPC_FMT "board_build_id"
#define SYSFS_BIOS_VER     "/sys/class/dmi/id/bios_version"

#define CMD_BMC_VER_1      "expr `ipmitool mc info"IPMITOOL_REDIRECT_FIRST_ERR" | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f1` + 0"
#define CMD_BMC_VER_2      "expr `ipmitool mc info"IPMITOOL_REDIRECT_ERR" | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f2` + 0"
#define CMD_BMC_VER_3      "echo $((`ipmitool mc info"IPMITOOL_REDIRECT_ERR" | grep 'Aux Firmware Rev Info' -A 2 | sed -n '2p'` + 0))"

static int ufi_sysi_platform_info_get(onlp_platform_info_t* pi)
{
    int mb_cpld_hw_rev = 0, mb_cpld_build_rev = 0;
    int len = 0;
    char bios_out[ONLP_CONFIG_INFO_STR_MAX] = {'\0'};
    char bmc_out1[8] = {0}, bmc_out2[8] = {0}, bmc_out3[8] = {0};

    //get MB CPLD version
    char mb_cpld_ver[ONLP_CONFIG_INFO_STR_MAX] = {'\0'};
    ONLP_TRY(onlp_file_read((uint8_t*)&mb_cpld_ver, ONLP_CONFIG_INFO_STR_MAX - 1, &len, SYSFS_CPLD_VER_H));

    pi->cpld_versions = aim_fstrdup(
        "\n"
        "[MB CPLD] %s\n", mb_cpld_ver);

    //Get HW Version
    ONLP_TRY(file_read_hex(&mb_cpld_hw_rev, SYSFS_HW_ID));

    //Get Build Version
    ONLP_TRY(file_read_hex(&mb_cpld_build_rev, SYSFS_BUILD_ID));

    //Get BIOS version
    char tmp_str[ONLP_CONFIG_INFO_STR_MAX] = {'\0'};
    ONLP_TRY(onlp_file_read((uint8_t*)&tmp_str, ONLP_CONFIG_INFO_STR_MAX - 1, &len, SYSFS_BIOS_VER));

    // Remove '\n'
    sscanf (tmp_str, "%[^\n]", bios_out);

    // Detect bmc status
    if(bmc_check_alive() != ONLP_STATUS_OK) {
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
        "[HW   ] %d\n"
        "[BUILD] %d\n"
        "[BIOS ] %s\n"
        "[BMC  ] %d.%d.%d\n",
        mb_cpld_hw_rev,
        mb_cpld_build_rev,
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
    return "x86-64-ufispace-s9510-28dc-r0";
}

/**
 * @brief Attempt to set the platform personality
 * in the event that the current platform does not match the
 * reported platform.
 * @note Optional
 */
int onlp_sysi_platform_set(const char* platform)
{
    return ONLP_STATUS_E_UNSUPPORTED;
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
    if (ufi_sysi_platform_info_get(info) < 0) {
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

