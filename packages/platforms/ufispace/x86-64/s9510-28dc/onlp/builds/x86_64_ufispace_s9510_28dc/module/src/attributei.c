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
#define SYSFS_CPLD_VER     LPC_FMT "cpld_version"
#define SYSFS_CPLD_BUILD   LPC_FMT "cpld_build"
#define SYSFS_HW_ID        LPC_FMT "board_hw_id"
#define SYSFS_BUILD_ID     LPC_FMT "board_build_id"
#define CMD_BIOS_VER       "dmidecode -s bios-version | tail -1 | tr -d '\r\n'"
#define CMD_BMC_VER_1      "expr `ipmitool mc info"IPMITOOL_REDIRECT_FIRST_ERR" | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f1` + 0"
#define CMD_BMC_VER_2      "expr `ipmitool mc info"IPMITOOL_REDIRECT_ERR" | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f2` + 0"
#define CMD_BMC_VER_3      "echo $((`ipmitool mc info"IPMITOOL_REDIRECT_ERR" | grep 'Aux Firmware Rev Info' -A 2 | sed -n '2p'` + 0))"

static int update_attributei_asset_info(onlp_oid_t oid, onlp_asset_info_t* asset_info)
{
    int mb_cpld_ver = 0, cpld_ver_major = 0, cpld_ver_minor = 0, cpld_build = 0;
    int mb_cpld_hw_rev = 0, mb_cpld_build_rev = 0;
    char bios_out[32] = {0};
    char bmc_out1[8] = {0}, bmc_out2[8] = {0}, bmc_out3[8] = {0};

    onlp_onie_info_t onie_info;

    //get MB CPLD version
    ONLP_TRY(onlp_file_read_int(&mb_cpld_ver, SYSFS_CPLD_VER));

    // Major: bit 6-7, Minor: bit 0-5
    cpld_ver_major = mb_cpld_ver >> 6 & 0x03;
    cpld_ver_minor = mb_cpld_ver & 0x3F;

    //get MB CPLD Build
    ONLP_TRY(onlp_file_read_int(&cpld_build, SYSFS_CPLD_BUILD));

    asset_info->cpld_revision = aim_fstrdup(
        "\n"
        "    [MB CPLD] %d.%02d.%d\n", cpld_ver_major, cpld_ver_minor, cpld_build);

    //Get HW Version
    ONLP_TRY(file_read_hex(&mb_cpld_hw_rev, SYSFS_HW_ID));

    //Get Build Version
    ONLP_TRY(file_read_hex(&mb_cpld_build_rev, SYSFS_BUILD_ID));

    //Get BIOS version
    if (exec_cmd(CMD_BIOS_VER, bios_out, sizeof(bios_out)) < 0) {
        AIM_LOG_ERROR("unable to read BIOS version\n");
        return ONLP_STATUS_E_INTERNAL;
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
        mb_cpld_hw_rev,
        mb_cpld_build_rev,
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
