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

/* This is definitions for x86-64-ufispace-s9610-46dx*/
/* OID map*/
/*
 * [01] CHASSIS
 *            |----[01] ONLP_THERMAL_CPU_PKG
 *            |----[02] ONLP_THERMAL_ENV_CPU
 *            |----[03] ONLP_THERMAL_CPU_PECI
 *            |----[04] ONLP_THERMAL_ENV0
 *            |----[05] ONLP_THERMAL_ENV1
 *            |----[06] ONLP_THERMAL_ENV2
 *            |----[07] ONLP_THERMAL_ENV3
 *            |----[08] ONLP_THERMAL_ENV4
 *            |----[09] ONLP_THERMAL_ENV5
 *            |----[10] ONLP_THERMAL_ENV_FAN0
 *            |----[11] ONLP_THERMAL_ENV_FAN1
 *            |----[12] ONLP_THERMAL_PSU0_TEMP1
 *            |----[13] ONLP_THERMAL_PSU1_TEMP1
 *            |----[01] ONLP_LED_SYS_SYNC
 *            |----[02] ONLP_LED_SYS_SYS
 *            |----[03] ONLP_LED_SYS_FAN
 *            |----[04] ONLP_LED_SYS_PSU_0
 *            |----[05] ONLP_LED_SYS_PSU_1
 *            |----[01] ONLP_PSU_0----[20] ONLP_THERMAL_PSU0_TEMP1
 *            |                  |----[09] ONLP_PSU_0_FAN
 *            |----[02] ONLP_PSU_1----[21] ONLP_THERMAL_PSU1_TEMP1
 *            |                  |----[10] ONLP_PSU_1_FAN
 *            |----[01] ONLP_FAN_F_0
 *            |----[02] ONLP_FAN_R_0
 *            |----[03] ONLP_FAN_F_1
 *            |----[04] ONLP_FAN_R_1
 *            |----[05] ONLP_FAN_F_2
 *            |----[06] ONLP_FAN_R_2
 *            |----[07] ONLP_FAN_F_3
 *            |----[08] ONLP_FAN_R_3
 */

#define SYS_EEPROM_PATH    "/sys/bus/i2c/devices/0-0057/eeprom"
#define SYS_EEPROM_SIZE    512

#define CMD_BIOS_VER       "cat /sys/class/dmi/id/bios_version | tr -d '\\r\\n'"
#define SYSFS_BIOS_VER "/sys/class/dmi/id/bios_version"

#define SYSFS_CPU_CPLD_VER "/sys/devices/platform/x86_64_ufispace_s9610_46dx_lpc/cpu_cpld/cpu_cpld_version_h"
#define SYSFS_MB_CPLD_VER "/sys/bus/i2c/devices/%d-%04x/cpld_version_h"

#define CMD_BMC_VER_1      "expr `ipmitool mc info"IPMITOOL_REDIRECT_FIRST_ERR" | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f1` + 0"
#define CMD_BMC_VER_2      "expr `ipmitool mc info"IPMITOOL_REDIRECT_ERR" | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f2` + 0"
#define CMD_BMC_VER_3      "echo $((`ipmitool mc info"IPMITOOL_REDIRECT_ERR" | grep 'Aux Firmware Rev Info' -A 2 | sed -n '2p'` + 0))"

static int ufi_sysi_platform_info_get(onlp_platform_info_t* pi)
{
    char cpu_cpld_ver_out[ONLP_CONFIG_INFO_STR_MAX];
    char mb_cpld_ver_out[CPLD_MAX][ONLP_CONFIG_INFO_STR_MAX];
    int mb_cpld1_addr = 0xE01, mb_cpld1_board_type_rev = 0, mb_cpld1_hw_rev = 0, mb_cpld1_build_rev = 0;
    int i = 0, len = 0;
    char bios_out[ONLP_CONFIG_INFO_STR_MAX] = "";
    char bmc_out1[8], bmc_out2[8], bmc_out3[8];

    memset(bios_out, 0, sizeof(bios_out));
    memset(bmc_out1, 0, sizeof(bmc_out1));
    memset(bmc_out2, 0, sizeof(bmc_out2));
    memset(bmc_out3, 0, sizeof(bmc_out3));

    //get CPU CPLD version
    ONLP_TRY(onlp_file_read((uint8_t*)&cpu_cpld_ver_out, ONLP_CONFIG_INFO_STR_MAX, &len, SYSFS_CPU_CPLD_VER));

    //get MB CPLD version
    for(i=0; i<CPLD_MAX; ++i) {
        ONLP_TRY(onlp_file_read((uint8_t*)&mb_cpld_ver_out[i], ONLP_CONFIG_INFO_STR_MAX, &len, SYSFS_MB_CPLD_VER,
                                             CPLD_I2C_BUS[i], CPLD_BASE_ADDR[i]));
    }

    pi->cpld_versions = aim_fstrdup(
        "\n"
        "[CPU CPLD] %s\n"
        "[MB CPLD1] %s\n"
        "[MB CPLD2] %s\n"
        "[MB CPLD3] %s\n",
        cpu_cpld_ver_out,
        mb_cpld_ver_out[0],
        mb_cpld_ver_out[1],
        mb_cpld_ver_out[2]);

    //Get HW Build Version
    ONLP_TRY(read_ioport(mb_cpld1_addr, &mb_cpld1_board_type_rev));

    mb_cpld1_hw_rev = (((mb_cpld1_board_type_rev) >> 0 & 0x03));
    mb_cpld1_build_rev = ((mb_cpld1_board_type_rev) >> 3 & 0x07);

    //Get BIOS version
    ONLP_TRY(onlp_file_read((uint8_t*)&bios_out, ONLP_CONFIG_INFO_STR_MAX, &len, SYSFS_BIOS_VER));

    //replace tailing new line in bios_out
    if (len > 0 && bios_out[len-1] == '\n') {
        bios_out[len-1] = '\0';
    }

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
        mb_cpld1_hw_rev,
        mb_cpld1_build_rev,
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
    return "x86-64-ufispace-s9610-46dx-r0";
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
    int i = 0;
    onlp_oid_t* e = table;
    memset(table, 0, max*sizeof(onlp_oid_t));

    /* Thermal */
    for (i = ONLP_THERMAL_CPU_PKG; i <= ONLP_THERMAL_ENV_FAN1; i++) {
        *e++ = ONLP_THERMAL_ID_CREATE(i);
    }

    /* LED */
    for (i = ONLP_LED_SYS_SYNC; i < ONLP_LED_MAX; i++) {
        *e++ = ONLP_LED_ID_CREATE(i);
    }

    /* PSU */
    for (i = ONLP_PSU_0; i <= ONLP_PSU_1; i++) {
        *e++ = ONLP_PSU_ID_CREATE(i);
    }

    /* FAN */
    for (i = ONLP_FAN_F_0; i <= ONLP_FAN_R_3; i++) {
        *e++ = ONLP_FAN_ID_CREATE(i);
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
    ONLP_TRY(ufi_sysi_platform_info_get(info));

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