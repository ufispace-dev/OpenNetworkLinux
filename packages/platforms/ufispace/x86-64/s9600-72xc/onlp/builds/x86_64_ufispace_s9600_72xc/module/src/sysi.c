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

/* EEPROM */
#define SYS_EEPROM_PATH         SYS_DEV "0-0057/eeprom"
#define SYS_EEPROM_SIZE         512
/* CPLD */
#define CPLD_MAX                4
#define MB_CPLDX_SYSFS_PATH_FMT SYS_DEV "1-00%02x"
/* SYSFS ATTR */
#define MB_CPLD_MAJOR_VER_ATTR  "cpld_major_ver"
#define MB_CPLD_MINOR_VER_ATTR  "cpld_minor_ver"
#define MB_CPLD_BUILD_VER_ATTR  "cpld_build_ver"
#define MB_CPLD_VER_ATTR        "cpld_version_h"
#define LPC_MB_SKU_ID_ATTR      "board_sku_id"
#define LPC_MB_HW_ID_ATTR       "board_hw_id"
#define LPC_MB_ID_TYPE_ATTR     "board_id_type"
#define LPC_MB_BUILD_ID_ATTR    "board_build_id"
#define LPC_MB_DEPH_ID_ATTR     "board_deph_id"
#define LPC_CPU_CPLD_VER_ATTR   "cpu_cpld_version_h"

#define SYSFS_BIOS_VER          "/sys/class/dmi/id/bios_version"
/* CMD */
#define CMD_BMC_VER_1           "expr `ipmitool mc info" IPMITOOL_REDIRECT_FIRST_ERR " | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f1` + 0"
#define CMD_BMC_VER_2           "expr `ipmitool mc info" IPMITOOL_REDIRECT_ERR " | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f2` + 0"
#define CMD_BMC_VER_3           "echo $((`ipmitool mc info" IPMITOOL_REDIRECT_ERR " | grep 'Aux Firmware Rev Info' -A 2 | sed -n '2p'` + 0))"

const int CPLD_BASE_ADDR[] = {0x30, 0x31, 0x32, 0x33};

/* This is definitions for x86-64-ufispace-s9600-72xc */
/* OID map*/
/*
 * [01] CHASSIS
 *            |----[01] ONLP_THERMAL_CPU_PECI
 *            |----[02] ONLP_THERMAL_CPU_ENV
 *            |----[03] ONLP_THERMAL_CPU_ENV_2
 *            |----[04] ONLP_THERMAL_MAC_ENV
 *            |----[05] ONLP_THERMAL_MAC_DIE
 *            |----[06] ONLP_THERMAL_ENV_FRONT
 *            |----[07] ONLP_THERMAL_ENV_REAR
 *            |----[08] ONLP_THERMAL_PSU0
 *            |----[09] ONLP_THERMAL_PSU1
 *            |----[10] ONLP_THERMAL_CPU_PKG
 *            |----[11] ONLP_THERMAL_CPU1
 *            |----[12] ONLP_THERMAL_CPU2
 *            |----[13] ONLP_THERMAL_CPU3
 *            |----[14] ONLP_THERMAL_CPU4
 *            |----[15] ONLP_THERMAL_CPU5
 *            |----[16] ONLP_THERMAL_CPU6
 *            |----[17] ONLP_THERMAL_CPU7
 *            |----[18] ONLP_THERMAL_CPU8
 *            |
 *            |----[01] ONLP_LED_SYS_SYNC
 *            |----[02] ONLP_LED_SYS_SYS
 *            |----[03] ONLP_LED_SYS_FAN
 *            |----[04] ONLP_LED_SYS_PSU0
 *            |----[05] ONLP_LED_SYS_PSU1
 *            |
 *            |----[01] ONLP_PSU_0----[08] ONLP_THERMAL_PSU0
 *            |                  |----[05] ONLP_PSU_0_FAN
 *            |----[02] ONLP_PSU_1----[09] ONLP_THERMAL_PSU1
 *            |                  |----[06] ONLP_PSU_1_FAN
 *            |
 *            |----[01] ONLP_FAN_0
 *            |----[02] ONLP_FAN_1
 *            |----[03] ONLP_FAN_2
 *            |----[04] ONLP_FAN_3
 */

static onlp_oid_t __onlp_oid_info[] = {
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PECI),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_ENV),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_ENV_2),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC_ENV),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC_DIE),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV_FRONT),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV_REAR),
    //ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU0),
    //ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU1),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PKG),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU1),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU2),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU3),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU4),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU5),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU6),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU7),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU8),

    ONLP_LED_ID_CREATE(ONLP_LED_SYS_SYNC),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_SYS),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_FAN),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_PSU0),
    ONLP_LED_ID_CREATE(ONLP_LED_SYS_PSU1),

    ONLP_PSU_ID_CREATE(ONLP_PSU_0),
    ONLP_PSU_ID_CREATE(ONLP_PSU_1),

    ONLP_FAN_ID_CREATE(ONLP_FAN_0),
    ONLP_FAN_ID_CREATE(ONLP_FAN_1),
    ONLP_FAN_ID_CREATE(ONLP_FAN_2),
    ONLP_FAN_ID_CREATE(ONLP_FAN_3),
    //ONLP_FAN_ID_CREATE(ONLP_PSU_0_FAN),
    //ONLP_FAN_ID_CREATE(ONLP_PSU_1_FAN),
};

static int ufi_sysi_platform_info_get(onlp_platform_info_t* pi)
{
    uint8_t cpu_cpld_ver_h[32];
    uint8_t mb_cpld_ver_h[CPLD_MAX][16];
    uint8_t bios_ver_h[32];
    char bmc_ver[3][16];
    int sku_id, hw_id, id_type, build_id, deph_id;
    int data_len;
    int i;
    int size;

    char mu_ver[128], mu_result[128];
    char path_onie_folder[] = "/mnt/onie-boot/onie";
    char path_onie_update_log[] = "/mnt/onie-boot/onie/update/update_details.log";
    char cmd_mount_mu_dir[] = "mkdir -p /mnt/onie-boot && mount LABEL=ONIE-BOOT /mnt/onie-boot/ 2> /dev/null";
    char cmd_mu_ver[] = "cat /mnt/onie-boot/onie/update/update_details.log | grep -i 'Updater version:' | tail -1 | awk -F ' ' '{ print $3}' | tr -d '\\r\\n'";
    char cmd_mu_result_template[] = "/mnt/onie-boot/onie/tools/bin/onie-fwpkg | grep '%s' | awk -F '|' '{ print $3 }' | tail -1 | xargs | tr -d '\\r\\n'";
    char cmd_mu_result[256];

    memset(cpu_cpld_ver_h, 0, sizeof(cpu_cpld_ver_h));
    memset(mb_cpld_ver_h, 0, sizeof(mb_cpld_ver_h));
    memset(bios_ver_h, 0, sizeof(bios_ver_h));
    memset(bmc_ver, 0, sizeof(bmc_ver));
    memset(mu_ver, 0, sizeof(mu_ver));
    memset(mu_result, 0, sizeof(mu_result));
    memset(cmd_mu_result, 0, sizeof(cmd_mu_result));

    //get CPU CPLD version readable string
    ONLP_TRY(onlp_file_read(cpu_cpld_ver_h, sizeof(cpu_cpld_ver_h), &data_len,
                    LPC_CPU_CPLD_PATH "/" LPC_CPU_CPLD_VER_ATTR));
    //trim new line
    cpu_cpld_ver_h[strcspn((char *)cpu_cpld_ver_h, "\n" )] = '\0';

    //get MB CPLD version from CPLD sysfs
    for(i=0; i<CPLD_MAX; ++i) {

        ONLP_TRY(onlp_file_read(mb_cpld_ver_h[i], sizeof(mb_cpld_ver_h[i]), &data_len,
                MB_CPLDX_SYSFS_PATH_FMT "/" MB_CPLD_VER_ATTR, CPLD_BASE_ADDR[i]));
        //trim new line
        mb_cpld_ver_h[i][strcspn((char *)cpu_cpld_ver_h, "\n" )] = '\0';
    }

    pi->cpld_versions = aim_fstrdup(
        "\n"
        "[CPU CPLD] %s\n"
        "[MB CPLD1] %s\n"
        "[MB CPLD2] %s\n"
        "[MB CPLD3] %s\n"
        "[MB CPLD4] %s\n",
        cpu_cpld_ver_h,
        mb_cpld_ver_h[0],
        mb_cpld_ver_h[1],
        mb_cpld_ver_h[2],
        mb_cpld_ver_h[3]);

    //get HW Build Version
    ONLP_TRY(file_read_hex(&sku_id, LPC_MB_CPLD_PATH "/" LPC_MB_SKU_ID_ATTR));
    ONLP_TRY(file_read_hex(&hw_id, LPC_MB_CPLD_PATH "/" LPC_MB_HW_ID_ATTR));
    ONLP_TRY(file_read_hex(&id_type, LPC_MB_CPLD_PATH "/" LPC_MB_ID_TYPE_ATTR));
    ONLP_TRY(file_read_hex(&build_id, LPC_MB_CPLD_PATH "/" LPC_MB_BUILD_ID_ATTR));
    ONLP_TRY(file_read_hex(&deph_id, LPC_MB_CPLD_PATH "/" LPC_MB_DEPH_ID_ATTR));

    //get BIOS version
    ONLP_TRY(onlp_file_read(bios_ver_h, sizeof(bios_ver_h), &size, SYSFS_BIOS_VER));
    //trim new line
    bios_ver_h[strcspn((char *)bios_ver_h, "\n" )] = '\0';

    // Detect bmc status
    if(bmc_check_alive() != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Timeout, BMC did not respond.\n");
        return ONLP_STATUS_E_INTERNAL;
    }

    //get BMC version
    if(exec_cmd(CMD_BMC_VER_1, bmc_ver[0], sizeof(bmc_ver[0])) < 0 ||
        exec_cmd(CMD_BMC_VER_2, bmc_ver[1], sizeof(bmc_ver[1])) < 0 ||
        exec_cmd(CMD_BMC_VER_3, bmc_ver[2], sizeof(bmc_ver[2]))) {
            AIM_LOG_ERROR("unable to read BMC version\n");
            return ONLP_STATUS_E_INTERNAL;
    }

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
        "[SKU ID] %d\n"
        "[HW ID] %d\n"
        "[BUILD ID] %d\n"
        "[ID TYPE] %d\n"
        "[DEPH ID] %d\n"
        "[BIOS] %s\n"
        "[BMC] %d.%d.%d\n"
        "[MU] %s (%s)\n",
        sku_id, hw_id, build_id, id_type, deph_id,
        bios_ver_h,
        atoi(bmc_ver[0]), atoi(bmc_ver[1]), atoi(bmc_ver[2]),
        strnlen(mu_ver, sizeof(mu_ver)) != 0 ? mu_ver : "NA", mu_result);

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
    return "x86-64-ufispace-s9600-72xc-r0";
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
    ONLP_TRY(ufi_sysi_platform_info_get(info));

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
