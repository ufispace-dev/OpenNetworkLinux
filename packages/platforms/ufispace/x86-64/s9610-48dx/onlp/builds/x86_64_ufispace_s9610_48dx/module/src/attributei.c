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
#include "platform_lib.h"


#define ONIE_FIELD_CPY(dst, src, field) \
  if (src.field != NULL) { \
    dst->field = malloc(strlen(src.field) + 1); \
    strcpy(dst->field, src.field); \
  }

#define IDPROM_PATH   SYSFS_DEVICES "0-0057/eeprom"
#define SYSFS_BIOS_VER "/sys/class/dmi/id/bios_version"

#define SYSFS_CPU_CPLD_VER SYSFS_LPC "cpu_cpld/cpu_cpld_version_h"
#define SYSFS_MB_CPLD_VER SYSFS_DEVICES "%d-%04x/cpld_version_h"

#define CMD_BMC_VER_1      "expr `ipmitool mc info"IPMITOOL_REDIRECT_FIRST_ERR" | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f1` + 0"
#define CMD_BMC_VER_2      "expr `ipmitool mc info"IPMITOOL_REDIRECT_ERR" | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f2` + 0"
#define CMD_BMC_VER_3      "echo $((`ipmitool mc info"IPMITOOL_REDIRECT_ERR" | grep 'Aux Firmware Rev Info' -A 2 | sed -n '2p'` + 0))"

static int update_attributei_asset_info(onlp_oid_t oid, onlp_asset_info_t* asset_info)
{
    char cpu_cpld_ver_out[ONLP_CONFIG_INFO_STR_MAX];
    char mb_cpld_ver_out[CPLD_MAX][ONLP_CONFIG_INFO_STR_MAX];
    int i = 0, len = 0;
    board_t board = {0};
    char bios_out[ONLP_CONFIG_INFO_STR_MAX] = "";
    char bmc_out1[8], bmc_out2[8], bmc_out3[8];

    onlp_onie_info_t onie_info;

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

    asset_info->cpld_revision = aim_fstrdup(
        "\n"
        "    [CPU CPLD] %s\n"
        "    [MB CPLD1] %s\n"
        "    [MB CPLD2] %s\n"
        "    [MB CPLD3] %s\n"
        "    [MB CPLD4] %s\n",
        cpu_cpld_ver_out,
        mb_cpld_ver_out[0],
        mb_cpld_ver_out[1],
        mb_cpld_ver_out[2],
        mb_cpld_ver_out[3]);

    //Get HW Version
    ONLP_TRY(ufi_get_board_version(&board));

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

    asset_info->firmware_revision = aim_fstrdup(
        "\n"
        "    [HW   ] %d\n"
        "    [BUILD] %d\n"
        "    [BIOS ] %s\n"
        "    [BMC  ] %d.%d.%d\n",
        board.hw_id,
        board.build_id,
        bios_out,
        atoi(bmc_out1), atoi(bmc_out2), atoi(bmc_out3));

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
int onlp_attributei_onie_info_get(onlp_oid_t oid, onlp_onie_info_t* onie_info)
{
    if(oid != ONLP_OID_CHASSIS) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if(onie_info == NULL) {
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
int onlp_attributei_asset_info_get(onlp_oid_t oid, onlp_asset_info_t* asset_info)
{
    if(oid != ONLP_OID_CHASSIS) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if(asset_info == NULL) {
        return ONLP_STATUS_OK;
    }

    asset_info->oid = oid;

    ONLP_TRY(update_attributei_asset_info(oid, asset_info));

    return ONLP_STATUS_OK;
}