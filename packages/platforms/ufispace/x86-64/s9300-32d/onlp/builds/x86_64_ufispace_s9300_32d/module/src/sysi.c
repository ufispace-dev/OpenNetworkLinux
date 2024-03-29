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

#define SYSFS_BIOS_VER  "/sys/class/dmi/id/bios_version"
#define CMD_BMC_VER_1   "expr `ipmitool mc info" IPMITOOL_REDIRECT_FIRST_ERR " | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f1` + 0"
#define CMD_BMC_VER_2   "expr `ipmitool mc info" IPMITOOL_REDIRECT_ERR " | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f2` + 0"
#define CMD_BMC_VER_3   "echo $((`ipmitool mc info" IPMITOOL_REDIRECT_ERR " | grep 'Aux Firmware Rev Info' -A 2 | sed -n '2p'` + 0))"

/* This is definitions for x86-64-ufispace-s9300-32d */
/* OID map*/
/*
 * [01] CHASSIS----[01] ONLP_THERMAL_CPU_PECI
 *            |----[02] ONLP_THERMAL_CPU_ENV
 *            |----[03] ONLP_THERMAL_CPU_ENV_2
 *            |----[04] ONLP_THERMAL_MAC_ENV
 *            |----[05] ONLP_THERMAL_CAGE
 *            |----[06] ONLP_THERMAL_PSU_CONN
 *            |----[07] ONLP_THERMAL_PSU0
 *            |----[08] ONLP_THERMAL_PSU1
 *            |----[09] ONLP_THERMAL_CPU_PKG
 *            |----[01] ONLP_LED_SYSTEM
 *            |----[02] ONLP_LED_PSU0
 *            |----[03] ONLP_LED_PSU1
 *            |----[04] ONLP_LED_FAN
 *            |----[01] ONLP_PSU_0----[13] ONLP_PSU0_FAN1
 *            |                  |----[07] ONLP_THERMAL_PSU0
 *            |
 *            |----[02] ONLP_PSU_1----[14] ONLP_PSU1_FAN1
 *            |                  |----[08] ONLP_THERMAL_PSU1
 *            |
 *            |----[01] ONLP_FAN1_F
 *            |----[02] ONLP_FAN1_R
 *            |----[03] ONLP_FAN2_F
 *            |----[04] ONLP_FAN2_R
 *            |----[05] ONLP_FAN3_F
 *            |----[06] ONLP_FAN3_R
 *            |----[07] ONLP_FAN4_F
 *            |----[08] ONLP_FAN4_R
 *            |----[09] ONLP_FAN5_F
 *            |----[10] ONLP_FAN5_R
 *            |----[11] ONLP_FAN6_F
 *            |----[12] ONLP_FAN6_R
 */

/* SYSFS */
#define LPC_CPU_CPLD_VERSION_ATTR   "cpu_cpld_version"
#define LPC_CPU_CPLD_BUILD_ATTR     "cpu_cpld_build"
#define LPC_CPU_CPLD_VER_H_ATTR     "cpu_cpld_version_h"
#define LPC_MB_SKUID_ATTR           "board_sku_id"
#define LPC_MB_HWID_ATTR            "board_hw_id"
#define LPC_MB_IDTYPE_ATTR          "board_id_type"
#define LPC_MB_BUILDID_ATTR         "board_build_id"
#define LPC_MB_DEPHID_ATTR          "board_deph_id"
#define MB_CPLD_VER_H_ATTR          "cpld_version_h"

static onlp_oid_t __onlp_oid_info[] = {
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PECI),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_ENV),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_ENV_2),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC_ENV),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CAGE),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU_CONN),
    //ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU0),
    //ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU1),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PKG),
    ONLP_LED_ID_CREATE(ONLP_LED_SYSTEM),
    ONLP_LED_ID_CREATE(ONLP_LED_PSU0),
    ONLP_LED_ID_CREATE(ONLP_LED_PSU1),
    ONLP_LED_ID_CREATE(ONLP_LED_FAN),
    ONLP_PSU_ID_CREATE(ONLP_PSU0),
    ONLP_PSU_ID_CREATE(ONLP_PSU1),
    ONLP_FAN_ID_CREATE(ONLP_FAN1_F),
    ONLP_FAN_ID_CREATE(ONLP_FAN1_R),
    ONLP_FAN_ID_CREATE(ONLP_FAN2_F),
    ONLP_FAN_ID_CREATE(ONLP_FAN2_R),
    ONLP_FAN_ID_CREATE(ONLP_FAN3_F),
    ONLP_FAN_ID_CREATE(ONLP_FAN3_R),
    ONLP_FAN_ID_CREATE(ONLP_FAN4_F),
    ONLP_FAN_ID_CREATE(ONLP_FAN4_R),
    ONLP_FAN_ID_CREATE(ONLP_FAN5_F),
    ONLP_FAN_ID_CREATE(ONLP_FAN5_R),
    ONLP_FAN_ID_CREATE(ONLP_FAN6_F),
    ONLP_FAN_ID_CREATE(ONLP_FAN6_R),
    //ONLP_FAN_ID_CREATE(ONLP_PSU0_FAN1),
    //ONLP_FAN_ID_CREATE(ONLP_PSU1_FAN1),
};

/* update sysi platform info */
static int update_sysi_platform_info(onlp_platform_info_t* info)
{
    uint8_t cpu_cpld_ver_h[32];
    uint8_t mb_cpld_ver_h[CPLD_MAX][16];
    uint8_t bios_ver_h[32];
    char bmc_ver[3][16];
    int sku_id, hw_id, id_type, build_id, deph_id;
    int i;
    int size;

    memset(cpu_cpld_ver_h, 0, sizeof(cpu_cpld_ver_h));
    memset(mb_cpld_ver_h, 0, sizeof(mb_cpld_ver_h));
    memset(bios_ver_h, 0, sizeof(bios_ver_h));
    memset(bmc_ver, 0, sizeof(bmc_ver));

    //get CPU CPLD version readable string
    ONLP_TRY(onlp_file_read(cpu_cpld_ver_h, sizeof(cpu_cpld_ver_h), &size,
                LPC_CPU_CPLD_PATH "/" LPC_CPU_CPLD_VER_H_ATTR));
    //trim new line
    cpu_cpld_ver_h[strcspn((char *)cpu_cpld_ver_h, "\n" )] = '\0';

    //get MB CPLD version from CPLD sysfs
    for(i=0; i<CPLD_MAX; ++i) {

        ONLP_TRY(onlp_file_read(mb_cpld_ver_h[i], sizeof(mb_cpld_ver_h[i]), &size,
                SYS_DEV "/2-00%02x/" MB_CPLD_VER_H_ATTR, CPLD_BASE_ADDR[i]));
        //trim new line
        mb_cpld_ver_h[i][strcspn((char *)cpu_cpld_ver_h, "\n" )] = '\0';
    }

    info->cpld_versions = aim_fstrdup(
        "\n"
        "[CPU CPLD] %s\n"
        "[MB CPLD1] %s\n"
        "[MB CPLD2] %s\n"
        "[MB CPLD3] %s\n",
        cpu_cpld_ver_h,
        mb_cpld_ver_h[0],
        mb_cpld_ver_h[1],
        mb_cpld_ver_h[2]);

    //Get HW Build Version
    ONLP_TRY(file_read_hex(&sku_id, LPC_MB_CPLD_PATH "/" LPC_MB_SKUID_ATTR));
    ONLP_TRY(file_read_hex(&hw_id, LPC_MB_CPLD_PATH "/" LPC_MB_HWID_ATTR));
    ONLP_TRY(file_read_hex(&id_type, LPC_MB_CPLD_PATH "/" LPC_MB_IDTYPE_ATTR));
    ONLP_TRY(file_read_hex(&build_id, LPC_MB_CPLD_PATH "/" LPC_MB_BUILDID_ATTR));
    ONLP_TRY(file_read_hex(&deph_id, LPC_MB_CPLD_PATH "/" LPC_MB_DEPHID_ATTR));

    //Get BIOS version
    ONLP_TRY(onlp_file_read(bios_ver_h, sizeof(bios_ver_h), &size, SYSFS_BIOS_VER));
    //trim new line
    bios_ver_h[strcspn((char *)bios_ver_h, "\n" )] = '\0';

    // Detect bmc status
    if(bmc_check_alive() != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Timeout, BMC did not respond.\n");
        return ONLP_STATUS_E_INTERNAL;
    }

    //Get BMC version
    if(exec_cmd(CMD_BMC_VER_1, bmc_ver[0], sizeof(bmc_ver[0])) < 0 ||
        exec_cmd(CMD_BMC_VER_2, bmc_ver[1], sizeof(bmc_ver[1])) < 0 ||
        exec_cmd(CMD_BMC_VER_3, bmc_ver[2], sizeof(bmc_ver[2]))) {
            AIM_LOG_ERROR("unable to read BMC version\n");
            return ONLP_STATUS_E_INTERNAL;
    }

    info->other_versions = aim_fstrdup(
        "\n"
        "[SKU ID] %d\n"
        "[HW ID] %d\n"
        "[BUILD ID] %d\n"
        "[ID TYPE] %d\n"
        "[DEPH ID] %d\n"
        "[BIOS] %s\n"
        "[BMC] %d.%d.%d\n",
        sku_id, hw_id, build_id, id_type, deph_id,
        bios_ver_h,
        atoi(bmc_ver[0]), atoi(bmc_ver[1]), atoi(bmc_ver[2]));

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
    return "x86-64-ufispace-s9300-32d-r0";
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
    lock_init();
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
    uint8_t* rdata = aim_zmalloc(512);

    if(onlp_file_read(rdata, 512, size, SYS_DEV "/0-0057/eeprom") == ONLP_STATUS_OK) {
        if(*size == 512) {
            *data = rdata;
            return ONLP_STATUS_OK;
        }
    }

    AIM_LOG_INFO("Unable to data get data from eeprom \n");
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
    /* If onlp_sysi_onie_data_get() allocated, it has be freed here. */
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
    ONLP_TRY(update_sysi_platform_info(info));

    return ONLP_STATUS_OK;
}

/**
 * @brief Friee a custom platform information structure.
 */
void onlp_sysi_platform_info_free(onlp_platform_info_t* info)
{
    if(info->cpld_versions) {
        aim_free(info->cpld_versions);
    }

    if(info->other_versions) {
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
