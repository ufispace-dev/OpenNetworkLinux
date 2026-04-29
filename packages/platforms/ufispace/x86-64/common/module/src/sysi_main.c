/************************************************************
 * Copyright (C) 2026 Ufispace Technology Corporation.
 ************************************************************
 *
 * ONLP System Platform Interface.
 *
 ***********************************************************/
#include <unistd.h>
#include <onlp/platformi/sysi.h>
#include <ufispace_platform/platform_lib.h>
#include <ufispace_common/platform_lib_main.h>
#include <sys/stat.h>

#ifndef __PLATFORM_LIB_H__
#define PLATFORM_NAME                    "x86-64-ufispace-unknown-r0"
#define LPC_BSP_FMT                      ""
#endif

#define SYS_EEPROM_PATH    "/sys_switch/syseeprom"
#define SYS_EEPROM_SIZE    512
#define SYSFS_CPLD_VER_H   "/sys_switch/cpld/cpld1/firmware_version"
#define SYSFS_BIOS_VER     "/sys/class/dmi/id/bios_version"
#define SYSFS_BMC_VER     "/sys/devices/platform/x86_64_ufispace_bmc/bmc/firmware_version"

#define CMD_BMC_VER_1      "expr `ipmitool mc info"IPMITOOL_REDIRECT_ERR" | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f1` + 0"
#define CMD_BMC_VER_2      "expr `ipmitool mc info"IPMITOOL_REDIRECT_ERR" | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f2` + 0"
#define CMD_BMC_VER_3      "echo $((`ipmitool mc info"IPMITOOL_REDIRECT_ERR" | grep 'Aux Firmware Rev Info' -A 2 | sed -n '2p'` + 0))"

static int ufi_sysi_platform_info_get(onlp_platform_info_t* pi)
{
    int len = 0;
    char bios_out[ONLP_CONFIG_INFO_STR_MAX] = {'\0'};
    char *bmc_out = NULL;
    int len1 = 0, len2 = 0;
    struct stat st = {0};

    /*
     * The cpld_versions buffer size is 128 bytes.
     * We must reserve:
     * 1 byte for the leading newline (\n)
     * 1 byte for the trailing newline (\n)
     * So, the remaining buffer space is 126 bytes.
     */
    char final_string[126];
    int cpld_count = 0;
    int fpga_count = 0;
    int rv = 0;
    int i = 0;
    int current_offset = 0;

    rv = ufi_file_read_int(&cpld_count, "/sys_switch/cpld/number");
    if(rv == ONLP_STATUS_OK) {
        for(i =1; i<=cpld_count;i++) {
           /*
            * Maybe the max element is 6
            * [XX CPLDX] <space> ==> 11 bytes
            * X.XX.XXX\n  ==> 9 bytes
            * total 126/20 = 6
            */
            char *description = NULL;
            char *version = NULL;
            size_t remaining_space = sizeof(final_string) - current_offset;
            const char* format_string = "[%s] %s\n";
            len1 = onlp_file_read_str(&description, "/sys_switch/cpld/cpld%d/description", i);
            len2 = onlp_file_read_str(&version, "/sys_switch/cpld/cpld%d/firmware_version", i);
            if(!description || !version || !len1 || !len2) {
                aim_free(description);
                aim_free(version);
                continue;
            }

            int chars_written = snprintf(
                final_string + current_offset,
                remaining_space,
                format_string,
                description,
                version
            );

            aim_free(description);
            aim_free(version);

            if (chars_written < 0 || chars_written >= remaining_space) {
                break;
            }
            current_offset += chars_written;
        }
    }

    // Read fpga version and append to final_string
    rv = ufi_file_read_int(&fpga_count, "/sys_switch/fpga/number");
    if(rv == ONLP_STATUS_OK) {
        for(i =1; i<=fpga_count;i++) {
            char *description = NULL;
            char *version = NULL;
            size_t remaining_space = sizeof(final_string) - current_offset;
            const char* format_string = "[%s] %s\n";
            len1 = onlp_file_read_str(&description, "/sys_switch/fpga/fpga%d/description", i);
            len2 = onlp_file_read_str(&version, "/sys_switch/fpga/fpga%d/firmware_version", i);
            if(!description || !version || !len1 || !len2) {
                aim_free(description);
                aim_free(version);
                continue;
            }

            int chars_written = snprintf(
                final_string + current_offset,
                remaining_space,
                format_string,
                description,
                version
            );

            aim_free(description);
            aim_free(version);

            if (chars_written < 0 || chars_written >= remaining_space) {
                break;
            }
            current_offset += chars_written;
        }
    }

    pi->cpld_versions = aim_fstrdup("\n%s", final_string);

    //Get BIOS version
    ONLP_TRY(onlp_file_read((uint8_t*)&bios_out, ONLP_CONFIG_INFO_STR_MAX - 1, &len, SYSFS_BIOS_VER));

    // Remove '\n'
    bios_out[strcspn(bios_out, "\n")] = 0;
    len1 = onlp_file_read_str(&bmc_out, "/sys/devices/platform/x86_64_ufispace_bmc/bmc/firmware_version");

    //Read extra component firmware version (Ex: EC)
    int num_extra_components = 0;
    char extra_string[ONLP_CONFIG_INFO_STR_MAX] = {'\0'};
    current_offset = 0;

    ONLP_TRY(ufi_file_read_int(&num_extra_components, "/sys_switch/slot/slot1/num_extra_components"));
    for (i = 1; i <= num_extra_components; i++) {
        char *extra_desc = NULL;
        char *extra_ver = NULL;
        len1 = onlp_file_read_str(&extra_desc, "/sys_switch/slot/slot1/extra_component%d/description", i);
        len2 = onlp_file_read_str(&extra_ver, "/sys_switch/slot/slot1/extra_component%d/firmware_version", i);
        if (!extra_desc || !extra_ver || !len1 || !len2) {
            aim_free(extra_desc);
            aim_free(extra_ver);
            continue;
        }
        snprintf(extra_string + current_offset, sizeof(extra_string) - current_offset, "[%s] %s\n", extra_desc, extra_ver);
        current_offset += strlen(extra_string + current_offset);
        aim_free(extra_desc);
        aim_free(extra_ver);
    }

    char mu_ver[128] = {'\0'}, mu_result[128] = {'\0'};
    char path_onie_folder[] = "/mnt/onie-boot/onie";
    char path_onie_update_log[] = "/mnt/onie-boot/onie/update/update_details.log";
    char cmd_mount_mu_dir[] = "mkdir -p /mnt/onie-boot && mount LABEL=ONIE-BOOT /mnt/onie-boot/ 2> /dev/null";
    char cmd_mu_ver[] = "cat /mnt/onie-boot/onie/update/update_details.log | grep -i 'Updater version:' | tail -1 | awk -F ' ' '{ print $3}' | tr -d '\\r\\n'";
    char cmd_mu_result_template[] = "/mnt/onie-boot/onie/tools/bin/onie-fwpkg | grep '%s' | awk -F '|' '{ print $3 }' | tail -1 | xargs | tr -d '\\r\\n'";
    char cmd_mu_result[256] = {'\0'};

    //Mount MU Folder
    if (stat(path_onie_folder, &st) != 0)
        system(cmd_mount_mu_dir);

    //Get MU Version
    if (stat(path_onie_update_log, &st) == 0) {
        exec_cmd(cmd_mu_ver, mu_ver, sizeof(mu_ver));

        if (strnlen(mu_ver, sizeof(mu_ver)) != 0) {
            snprintf(cmd_mu_result, sizeof(cmd_mu_result), cmd_mu_result_template, mu_ver);
            exec_cmd(cmd_mu_result, mu_result, sizeof(mu_result));
        }
    }

    pi->other_versions = aim_fstrdup(
        "\n"
        "[BIOS] %s\n"
        "[BMC] %s\n"
        "%s"
        "[MU] %s (%s)\n",
        bios_out,
        !bmc_out? "NA":bmc_out,
        extra_string,
        strnlen(mu_ver, sizeof(mu_ver)) != 0 ? mu_ver : "NA", mu_result);

    aim_free(bmc_out);
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
const char* __WEAK onlp_sysi_platform_get(void)
{
    return PLATFORM_NAME;
}

/**
 * @brief Attempt to set the platform personality
 * in the event that the current platform does not match the
 * reported platform.
 * @note Optional
 */
int __WEAK onlp_sysi_platform_set(const char* platform)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Initialize the system platform subsystem.
 */
int __WEAK onlp_sysi_init(void)
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
 int __WEAK onlp_sysi_onie_data_phys_addr_get(void** physaddr)
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
int __WEAK onlp_sysi_onie_data_get(uint8_t** data, int* size)
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
void __WEAK onlp_sysi_onie_data_free(uint8_t* data)
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
int __WEAK onlp_sysi_onie_info_get(onlp_onie_info_t* onie)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief This function returns the root oid list for the platform.
 * @param table [out] Receives the table.
 * @param max The maximum number of entries you can fill.
 */
int __WEAK onlp_sysi_oids_get(onlp_oid_t* table, int max)
{
    int count = 0;
    int psu_oid_base = 0;
    int temp_oid_base = 0;
    int fan_oid_base = 0;
    int led_oid_base = 0;
    int rv = ONLP_STATUS_OK;
    int tmp_max = max;
    int i = 1;
    int index = 0;
    int id = 1;

    memset(table, 0, max*sizeof(onlp_oid_t));

    ONLP_TRY(_onlp_psu_oid_base_get(&psu_oid_base));
    ONLP_TRY(_onlp_temp_oid_base_get(&temp_oid_base));
    ONLP_TRY(_onlp_fan_oid_base_get(&fan_oid_base));
    ONLP_TRY(_onlp_led_oid_base_get(&led_oid_base));

    rv = ufi_file_read_int(&count, "/sys_switch/temp_sensor/number");
    if(rv == ONLP_STATUS_OK) {
        for(i=1; tmp_max > 0 && i <= count; i++) {
            char *description = NULL;
            char *alias = NULL;
            int len1 = onlp_file_read_str(&description, "/sys_switch/temp_sensor/temp%d/description", i);
            int len2 = onlp_file_read_str(&alias, "/sys_switch/temp_sensor/temp%d/alias", i);
            id = temp_oid_base + (i-1);
            if(!description || !alias || !len1 || !len2) {
                aim_free(description);
                aim_free(alias);
                continue;
            }

            if(!strncmp(description, COMM_STR_NA, sizeof(COMM_STR_NA))) {
                aim_free(description);
                aim_free(alias);
                continue;
            }

            aim_free(description);
            aim_free(alias);

            table[index] = ONLP_THERMAL_ID_CREATE(id);
            index++;
            tmp_max--;
        }
    }

    rv = ufi_file_read_int(&count, "/sys_switch/sysled/number");
    if(rv == ONLP_STATUS_OK) {
        for(i=1; tmp_max > 0 && i <= count; i++) {
            char *description = NULL;
            int len1 = onlp_file_read_str(&description, "/sys_switch/sysled/led%d/description", i);
            id = led_oid_base + (i-1);
            if(!description || !len1) {
                aim_free(description);
                continue;
            }

            if(!strncmp(description, COMM_STR_NA, sizeof(COMM_STR_NA))) {
                aim_free(description);
                continue;
            }

            aim_free(description);

            table[index] = ONLP_LED_ID_CREATE(id);
            index++;
            tmp_max--;
        }
    }

    rv = ufi_file_read_int(&count, "/sys_switch/psu/number");
    if(rv == ONLP_STATUS_OK) {
        for(i=1; tmp_max > 0 && i <= count; i++) {
            char *description = NULL;
            int len1 = onlp_file_read_str(&description, "/sys_switch/psu/psu%d/description", i);
            id = psu_oid_base + (i-1);
            if(!description || !len1) {
                aim_free(description);
                continue;
            }

            if(!strncmp(description, COMM_STR_NA, sizeof(COMM_STR_NA))) {
                aim_free(description);
                continue;
            }

            aim_free(description);

            table[index] = ONLP_PSU_ID_CREATE(id);
            index++;
            tmp_max--;
        }
    }

    rv = ufi_file_read_int(&count, "/sys_switch/fan/number");
    if(rv == ONLP_STATUS_OK) {
        for(i=1; tmp_max > 0 && i <= count; i++) {
            int motor_number = 0;
            int rv2 = ufi_file_read_int(&motor_number, "/sys_switch/fan/fan%d/motor_number", i);
            if(rv2 == ONLP_STATUS_OK) {
                int j = 1;
                for(j=1; tmp_max >= 0 && j <= motor_number; j++) {
                    char *description = NULL;
                    int len1 = onlp_file_read_str(&description, "/sys_switch/fan/fan%d/motor%d/description", i, j);
                    id = fan_oid_base + (i*j-1);
                    if(!description || !len1) {
                        aim_free(description);
                        continue;
                    }

                    if(!strncmp(description, COMM_STR_NA, sizeof(COMM_STR_NA))) {
                        aim_free(description);
                        continue;
                    }

                    aim_free(description);

                    table[index] = ONLP_FAN_ID_CREATE(id);
                    index++;
                    tmp_max--;
                }
            }
        }
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
int __WEAK onlp_sysi_ioctl(int code, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}


/**
 * @brief Platform management initialization.
 */
int __WEAK onlp_sysi_platform_manage_init(void)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Perform necessary platform fan management.
 * @note This function should automatically adjust the FAN speeds
 * according to the platform conditions.
 */
int __WEAK onlp_sysi_platform_manage_fans(void)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Perform necessary platform LED management.
 * @note This function should automatically adjust the LED indicators
 * according to the platform conditions.
 */
int __WEAK onlp_sysi_platform_manage_leds(void)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Return custom platform information.
 */
int __WEAK onlp_sysi_platform_info_get(onlp_platform_info_t* info)
{
    memset(info, 0, sizeof(onlp_platform_info_t));
    if (ufi_sysi_platform_info_get(info) < 0) {
        onlp_sysi_platform_info_free(info);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Friee a custom platform information structure.
 */
void __WEAK onlp_sysi_platform_info_free(onlp_platform_info_t* info)
{
    if (info && info->cpld_versions) {
        aim_free(info->cpld_versions);
        info->cpld_versions = NULL;
    }

    if (info && info->other_versions) {
        aim_free(info->other_versions);
        info->other_versions = NULL;
    }
}

/**
 * @brief Builtin platform debug tool.
 */
int __WEAK onlp_sysi_debug(aim_pvs_t* pvs, int argc, char** argv)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

