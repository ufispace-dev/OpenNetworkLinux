/************************************************************
 * <bsn.cl fy=2017 v=onl>
 *
 *        Copyright 2017 Big Switch Networks, Inc.
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
 * Attribute Implementation.
 *
 ***********************************************************/
#include <onlp/platformi/attributei.h>
#include <onlp/stdattrs.h>
#include <onlplib/file.h>
#include <unistd.h>
#include "platform_lib.h"

#define ONIE_FIELD_CPY(dst, src, field)             \
    if (src.field != NULL)                          \
    {                                               \
        dst->field = malloc(strlen(src.field) + 1); \
        strcpy(dst->field, src.field);              \
    }

#define IDPROM_PATH "/sys/bus/i2c/devices/0-0057/eeprom"

/* CPLD */
#define CPLD_MAX 4
#define MB_CPLDX_SYSFS_PATH_FMT SYS_DEV "1-00%02x"
/* SYSFS ATTR */
#define MB_CPLD_MAJOR_VER_ATTR "cpld_major_ver"
#define MB_CPLD_MINOR_VER_ATTR "cpld_minor_ver"
#define MB_CPLD_BUILD_VER_ATTR "cpld_build_ver"
#define MB_CPLD_VER_ATTR "cpld_version_h"
#define LPC_MB_SKU_ID_ATTR "board_sku_id"
#define LPC_MB_HW_ID_ATTR "board_hw_id"
#define LPC_MB_ID_TYPE_ATTR "board_id_type"
#define LPC_MB_BUILD_ID_ATTR "board_build_id"
#define LPC_MB_DEPH_ID_ATTR "board_deph_id"
#define LPC_CPU_CPLD_VER_ATTR "cpu_cpld_version_h"
#define SYSFS_BIOS_VER "/sys/class/dmi/id/bios_version"
/* CMD */
#define CMD_BMC_VER_1 "expr `ipmitool mc info" IPMITOOL_REDIRECT_FIRST_ERR " | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f1` + 0"
#define CMD_BMC_VER_2 "expr `ipmitool mc info" IPMITOOL_REDIRECT_ERR " | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f2` + 0"
#define CMD_BMC_VER_3 "echo $((`ipmitool mc info" IPMITOOL_REDIRECT_ERR " | grep 'Aux Firmware Rev Info' -A 2 | sed -n '2p'` + 0))"

const int CPLD_BASE_ADDR[] = {0x30, 0x31, 0x32, 0x33};

static int update_attributei_asset_info(onlp_oid_t oid, onlp_asset_info_t *asset_info)
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

    onlp_onie_info_t onie_info;

    memset(cpu_cpld_ver_h, 0, sizeof(cpu_cpld_ver_h));
    memset(mb_cpld_ver_h, 0, sizeof(mb_cpld_ver_h));
    memset(bios_ver_h, 0, sizeof(bios_ver_h));
    memset(bmc_ver, 0, sizeof(bmc_ver));
    memset(mu_ver, 0, sizeof(mu_ver));
    memset(mu_result, 0, sizeof(mu_result));
    memset(cmd_mu_result, 0, sizeof(cmd_mu_result));

    // get CPU CPLD version readable string
    ONLP_TRY(onlp_file_read(cpu_cpld_ver_h, sizeof(cpu_cpld_ver_h), &data_len,
                            LPC_CPU_CPLD_PATH "/" LPC_CPU_CPLD_VER_ATTR));
    // trim new line
    cpu_cpld_ver_h[strcspn((char *)cpu_cpld_ver_h, "\n")] = '\0';

    // get MB CPLD version from CPLD sysfs
    for (i = 0; i < CPLD_MAX; ++i)
    {

        ONLP_TRY(onlp_file_read(mb_cpld_ver_h[i], sizeof(mb_cpld_ver_h[i]), &data_len,
                                MB_CPLDX_SYSFS_PATH_FMT "/" MB_CPLD_VER_ATTR, CPLD_BASE_ADDR[i]));
        // trim new line
        mb_cpld_ver_h[i][strcspn((char *)cpu_cpld_ver_h, "\n")] = '\0';
    }

    asset_info->cpld_revision = aim_fstrdup(
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

    // get HW Build Version
    ONLP_TRY(file_read_hex(&sku_id, LPC_MB_CPLD_PATH "/" LPC_MB_SKU_ID_ATTR));
    ONLP_TRY(file_read_hex(&hw_id, LPC_MB_CPLD_PATH "/" LPC_MB_HW_ID_ATTR));
    ONLP_TRY(file_read_hex(&id_type, LPC_MB_CPLD_PATH "/" LPC_MB_ID_TYPE_ATTR));
    ONLP_TRY(file_read_hex(&build_id, LPC_MB_CPLD_PATH "/" LPC_MB_BUILD_ID_ATTR));
    ONLP_TRY(file_read_hex(&deph_id, LPC_MB_CPLD_PATH "/" LPC_MB_DEPH_ID_ATTR));

    // get BIOS version
    ONLP_TRY(onlp_file_read(bios_ver_h, sizeof(bios_ver_h), &size, SYSFS_BIOS_VER));
    // trim new line
    bios_ver_h[strcspn((char *)bios_ver_h, "\n")] = '\0';

    // Detect bmc status
    if (bmc_check_alive() != ONLP_STATUS_OK)
    {
        AIM_LOG_ERROR("Timeout, BMC did not respond.\n");
        return ONLP_STATUS_E_INTERNAL;
    }

    // get BMC version
    if (exec_cmd(CMD_BMC_VER_1, bmc_ver[0], sizeof(bmc_ver[0])) < 0 ||
        exec_cmd(CMD_BMC_VER_2, bmc_ver[1], sizeof(bmc_ver[1])) < 0 ||
        exec_cmd(CMD_BMC_VER_3, bmc_ver[2], sizeof(bmc_ver[2])))
    {
        AIM_LOG_ERROR("unable to read BMC version\n");
        return ONLP_STATUS_E_INTERNAL;
    }

    // Mount MU Folder
    if (access(path_onie_folder, F_OK) == -1)
        system(cmd_mount_mu_dir);

    // Get MU Version
    if (access(path_onie_update_log, F_OK) != -1)
    {
        exec_cmd(cmd_mu_ver, mu_ver, sizeof(mu_ver));

        if (strnlen(mu_ver, sizeof(mu_ver)) != 0)
        {
            snprintf(cmd_mu_result, sizeof(cmd_mu_result), cmd_mu_result_template, mu_ver);
            exec_cmd(cmd_mu_result, mu_result, sizeof(mu_result));
        }
    }

    asset_info->firmware_revision = aim_fstrdup(
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

    /* get platform info from onie syseeprom */
    onlp_attributei_onie_info_get(oid, &onie_info);

    asset_info->oid = oid;
    ONIE_FIELD_CPY(asset_info, onie_info, manufacturer)
    ONIE_FIELD_CPY(asset_info, onie_info, part_number)
    ONIE_FIELD_CPY(asset_info, onie_info, serial_number)
    ONIE_FIELD_CPY(asset_info, onie_info, manufacture_date)

    return ONLP_STATUS_OK;
}

/**
 * @brief Initialize the attribute subsystem.
 */
int onlp_attributei_sw_init(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Initialize the attribute subsystem.
 */
int onlp_attributei_hw_init(uint32_t flags)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Deinitialize the attribute subsystem.
 */
int onlp_attributei_sw_denit(void)
{
    return ONLP_STATUS_OK;
}

/**
 * Access to standard attributes.
 */

/**
 * @brief Get an OID's ONIE attribute.
 * @param oid The target OID
 * @param onie_info [out] Receives the ONIE information if supported.
 * @note if onie_info is NULL you should only return whether the attribute is supported.
 */
int onlp_attributei_onie_info_get(onlp_oid_t oid, onlp_onie_info_t *onie_info)
{
    if (oid != ONLP_OID_CHASSIS)
    {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if (onie_info == NULL)
    {
        return ONLP_STATUS_OK;
    }

    return onlp_onie_decode_file(onie_info, IDPROM_PATH);
}

/**
 * @brief Get an OID's Asset attribute.
 * @param oid The target OID.
 * @param asset_info [out] Receives the Asset information if supported.
 * @note if asset_info is NULL you should only return whether the attribute is supported.
 */
int onlp_attributei_asset_info_get(onlp_oid_t oid, onlp_asset_info_t *asset_info)
{
    int ret = ONLP_STATUS_OK;

    if (oid != ONLP_OID_CHASSIS)
    {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if (asset_info == NULL)
    {
        return ONLP_STATUS_OK;
    }

    asset_info->oid = oid;

    ret = update_attributei_asset_info(oid, asset_info);

    return ret;
}
