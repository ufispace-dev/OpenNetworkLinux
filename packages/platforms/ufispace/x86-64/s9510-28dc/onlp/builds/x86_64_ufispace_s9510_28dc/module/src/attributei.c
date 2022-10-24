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

#define IDPROM_PATH        "/sys/bus/i2c/devices/1-0057/eeprom"
#define SYSFS_CPLD_VER_H   LPC_FMT "cpld_version_h"
#define SYSFS_BIOS_VER     "/sys/class/dmi/id/bios_version"

#define CMD_BMC_VER_1      "expr `ipmitool mc info"IPMITOOL_REDIRECT_FIRST_ERR" | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f1` + 0"
#define CMD_BMC_VER_2      "expr `ipmitool mc info"IPMITOOL_REDIRECT_ERR" | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f2` + 0"
#define CMD_BMC_VER_3      "echo $((`ipmitool mc info"IPMITOOL_REDIRECT_ERR" | grep 'Aux Firmware Rev Info' -A 2 | sed -n '2p'` + 0))"

static int update_attributei_asset_info(onlp_oid_t oid, onlp_asset_info_t* asset_info)
{
    board_t board = {0};
    int len = 0;
    char bios_out[ONLP_CONFIG_INFO_STR_MAX] = {'\0'};
    char bmc_out1[8] = {0}, bmc_out2[8] = {0}, bmc_out3[8] = {0};

    onlp_onie_info_t onie_info;

    //get MB CPLD version
    char mb_cpld_ver[ONLP_CONFIG_INFO_STR_MAX] = {'\0'};
    ONLP_TRY(onlp_file_read((uint8_t*)&mb_cpld_ver, ONLP_CONFIG_INFO_STR_MAX - 1, &len, SYSFS_CPLD_VER_H));

    asset_info->cpld_revision = aim_fstrdup(
        "\n"
        "    [MB CPLD] %s\n", mb_cpld_ver);

    //Get HW Version
    ONLP_TRY(ufi_get_board_version(&board));

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

    asset_info->firmware_revision = aim_fstrdup(
        "\n"
        "    [HW   ] %d\n"
        "    [BUILD] %d\n"
        "    [BIOS ] %s\n"
        "    [BMC  ] %d.%d.%d\n",
        board.hw_rev,
        board.hw_build,
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
 * @brief Determine whether the OID supports the given attributei.
 * @param oid The OID.
 * @param attribute The attribute name.
 */
int onlp_attributei_supported(onlp_oid_t oid, const char* attribute)
{
     return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Set an attribute on the given OID.
 * @param oid The OID.
 * @param attribute The attribute name.
 * @param value A pointer to the value.
 */
int onlp_attributei_set(onlp_oid_t oid, const char* attribute, void* value)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Get an attribute from the given OID.
 * @param oid The OID.
 * @param attribute The attribute to retrieve.
 * @param[out] value Receives the attributei's value.
 */
int onlp_attributei_get(onlp_oid_t oid, const char* attribute, void** value) 
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Free an attribute value returned from onlp_attributei_get().
 * @param oid The OID.
 * @param attribute The attribute.
 * @param value The value.
 */
int onlp_attributei_free(onlp_oid_t oid, const char* attribute, void* value)
{
    return ONLP_STATUS_E_UNSUPPORTED;
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
    int ret = ONLP_STATUS_OK;

    if(oid != ONLP_OID_CHASSIS) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if(asset_info == NULL) {
        return ONLP_STATUS_OK;
    }

    asset_info->oid = oid;

    ret = update_attributei_asset_info(oid, asset_info);

    return ret;
}
