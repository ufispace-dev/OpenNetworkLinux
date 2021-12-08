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

#define IDPROM_PATH   "/sys/bus/i2c/devices/0-0057/eeprom"
#define CMD_BIOS_VER "cat /sys/class/dmi/id/bios_version | tr -d '\r\n'"
#define CMD_BMC_VER_1 "expr `ipmitool mc info | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f1` + 0"
#define CMD_BMC_VER_2 "expr `ipmitool mc info | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f2` + 0"
#define CMD_BMC_VER_3 "echo $((`ipmitool mc info | grep 'Aux Firmware Rev Info' -A 2 | sed -n '2p'`))"


static int update_attributei_asset_info(onlp_oid_t oid, onlp_asset_info_t* asset_info)
{
    int cpu_cpld_addr = 0x600;
    int cpu_cpld_build_addr = 0x6e0;
    int cpu_cpld_ver = 0, cpu_cpld_ver_major = 0, cpu_cpld_ver_minor = 0, cpu_cpld_ver_build = 0;
    int cpld_ver[CPLD_MAX], cpld_ver_major[CPLD_MAX], cpld_ver_minor[CPLD_MAX], cpld_build[CPLD_MAX];
    int mb_cpld1_addr = 0xE01, mb_cpld1_board_type_rev = 0, mb_cpld1_hw_rev = 0, mb_cpld1_build_rev = 0;
    int i = 0;
    char bios_out[32];
    char bmc_out1[8], bmc_out2[8], bmc_out3[8];

    onlp_onie_info_t onie_info;
    
    memset(bios_out, 0, sizeof(bios_out));
    memset(bmc_out1, 0, sizeof(bmc_out1));
    memset(bmc_out2, 0, sizeof(bmc_out2));
    memset(bmc_out3, 0, sizeof(bmc_out3));

    //get CPU CPLD version
    ONLP_TRY(read_ioport(cpu_cpld_addr, &cpu_cpld_ver));

    cpu_cpld_ver_major = (((cpu_cpld_ver) >> 6 & 0x03));
    cpu_cpld_ver_minor = (((cpu_cpld_ver) & 0x3F));

    //get CPU CPLD build version
    ONLP_TRY(read_ioport(cpu_cpld_build_addr, &cpu_cpld_ver_build));
    
    //get MB CPLD version
    for(i=0; i < CPLD_MAX; ++i) {        
        ONLP_TRY(file_read_hex(&cpld_ver[i], "/sys/bus/i2c/devices/%d-00%02x/cpld_version",
                                 CPLD_I2C_BUS[i], CPLD_BASE_ADDR[i]));
        
        if (cpld_ver[i] < 0) {            
            AIM_LOG_ERROR("unable to read MB CPLD version\n");
            return ONLP_STATUS_E_INTERNAL;             
        }
       
        cpld_ver_major[i] = (((cpld_ver[i]) >> 6 & 0x03));
        cpld_ver_minor[i] = (((cpld_ver[i]) & 0x3F));

        ONLP_TRY(file_read_hex(&cpld_build[i], "/sys/bus/i2c/devices/%d-00%02x/cpld_build",
                                 CPLD_I2C_BUS[i], CPLD_BASE_ADDR[i]));
        
        if (cpld_build[i] < 0) {            
            AIM_LOG_ERROR("unable to read MB CPLD build\n");
            return ONLP_STATUS_E_INTERNAL;             
        }        
    }    

    asset_info->cpld_revision = aim_fstrdup(
        "\n" 
        "[CPU CPLD] %d.%02d.%d\n"
        "[MB CPLD1] %d.%02d.%d\n"
        "[MB CPLD2] %d.%02d.%d\n"
        "[MB CPLD3] %d.%02d.%d\n"
        "[MB CPLD4] %d.%02d.%d\n" 
        "[MB CPLD5] %d.%02d.%d\n",
        cpu_cpld_ver_major, cpu_cpld_ver_minor, cpu_cpld_ver_build,
        cpld_ver_major[0], cpld_ver_minor[0], cpld_build[0],
        cpld_ver_major[1], cpld_ver_minor[1], cpld_build[1],
        cpld_ver_major[2], cpld_ver_minor[2], cpld_build[2],
        cpld_ver_major[3], cpld_ver_minor[3], cpld_build[3],
        cpld_ver_major[4], cpld_ver_minor[4], cpld_build[4]);

    //Get HW Build Version
    ONLP_TRY(read_ioport(mb_cpld1_addr, &mb_cpld1_board_type_rev));
    
    mb_cpld1_hw_rev = (((mb_cpld1_board_type_rev) >> 0 & 0x03));
    mb_cpld1_build_rev = ((mb_cpld1_board_type_rev) >> 3 & 0x07);

    //Get BIOS version
    ONLP_TRY(exec_cmd(CMD_BIOS_VER, bios_out, sizeof(bios_out)));

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
            mb_cpld1_hw_rev,
            mb_cpld1_build_rev,
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
