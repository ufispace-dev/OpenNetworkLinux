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
#include <onlp/platformi/ledi.h>
#include <onlp/platformi/psui.h>
#include <onlp/platformi/thermali.h>
#include <onlp/platformi/fani.h>
#include <onlplib/file.h>
#include <onlplib/crc32.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "platform_lib.h"

#define CMD_BIOS_VER  "dmidecode -s bios-version | tail -1 | tr -d '\r\n'"
#define CMD_BMC_VER_1 "expr `ipmitool mc info"IPMITOOL_REDIRECT_FIRST_ERR" | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f1` + 0"
#define CMD_BMC_VER_2 "expr `ipmitool mc info"IPMITOOL_REDIRECT_ERR" | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f2` + 0"
#define CMD_BMC_VER_3 "echo $((`ipmitool mc info"IPMITOOL_REDIRECT_ERR" | grep 'Aux Firmware Rev Info' -A 2 | sed -n '2p'` + 0))"
#define CMD_UCD_VER_T "ipmitool raw 0x3c 0x08 0x1"
#define CMD_UCD_VER_B "ipmitool raw 0x3c 0x08 0x0"

/* This is definitions for x86-64-ufispace-s9705-48d */
/* OID map*/
/*
 * [01] CHASSIS----[01] ONLP_THERMAL_CPU_PECI
 *            |----[02] ONLP_THERMAL_RAMON_ENV_T
 *            |----[03] ONLP_THERMAL_RAMON_DIE_T
 *            |----[04] ONLP_THERMAL_RAMON_ENV_B
 *            |----[05] ONLP_THERMAL_RAMON_DIE_B
 *            |----[08] ONLP_THERMAL_CPU_PKG
 *            |----[09] ONLP_THERMAL_CPU_BOARD
 *            |----[01] ONLP_LED_SYSTEM
 *            |----[02] ONLP_LED_PSU0
 *            |----[03] ONLP_LED_PSU1
 *            |----[04] ONLP_LED_FAN
 *            |----[01] ONLP_PSU_0----[05] ONLP_PSU0_FAN_1
 *            |                  |----[06] ONLP_PSU0_FAN_2
 *            |                  |----[06] ONLP_THERMAL_PSU0
 *            |
 *            |----[02] ONLP_PSU_1----[07] ONLP_PSU1_FAN_1
 *            |                  |----[08] ONLP_PSU1_FAN_2
 *            |                  |----[07] ONLP_THERMAL_PSU1
 *            |
 *            |----[01] ONLP_FAN_0
 *            |----[02] ONLP_FAN_1
 *            |----[03] ONLP_FAN_2
 *            |----[04] ONLP_FAN_3
 */
static onlp_oid_t __onlp_oid_info[] = {
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PECI),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_RAMON_ENV_T),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_RAMON_DIE_T),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_RAMON_ENV_B),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_RAMON_DIE_B),
    //ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU0),
    //ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU1),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PKG),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_BOARD),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_BMC_ENV),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV_FRONT_T),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_ENV_FRONT_B),
    ONLP_LED_ID_CREATE(ONLP_LED_SYSTEM),
    ONLP_LED_ID_CREATE(ONLP_LED_PSU0),
    ONLP_LED_ID_CREATE(ONLP_LED_PSU1),
    ONLP_LED_ID_CREATE(ONLP_LED_FAN),
    //ONLP_LED_ID_CREATE(ONLP_LED_FAN_TRAY1),
    //ONLP_LED_ID_CREATE(ONLP_LED_FAN_TRAY2),
    //ONLP_LED_ID_CREATE(ONLP_LED_FAN_TRAY3),
    //ONLP_LED_ID_CREATE(ONLP_LED_FAN_TRAY4),
    ONLP_PSU_ID_CREATE(ONLP_PSU_0),
    ONLP_PSU_ID_CREATE(ONLP_PSU_1),
    ONLP_FAN_ID_CREATE(ONLP_FAN_0),
    ONLP_FAN_ID_CREATE(ONLP_FAN_1),
    ONLP_FAN_ID_CREATE(ONLP_FAN_2),
    ONLP_FAN_ID_CREATE(ONLP_FAN_3),
    //ONLP_FAN_ID_CREATE(ONLP_PSU0_FAN_1),
    //ONLP_FAN_ID_CREATE(ONLP_PSU0_FAN_2),
    //ONLP_FAN_ID_CREATE(ONLP_PSU1_FAN_1),
    //ONLP_FAN_ID_CREATE(ONLP_PSU1_FAN_2),
};


static int parse_ucd_out(char *ucd_out, char *ucd_data, int start, int len)
{
    int i=0;
    char data[3];

    memset(data, 0, sizeof(data));

    for (i=0; i<len; ++i) {
        data[0] = ucd_out[(start+i)*3 + 1];
        data[1] = ucd_out[(start+i)*3 + 2];
        //hex string to int
        ucd_data[i] = (int) strtol(data, NULL, 16);
    }
    return ONLP_STATUS_OK;
}

/* update sysi platform info */
static int update_sysi_platform_info(onlp_platform_info_t* info)
{
    int cpu_cpld_addr = 0x600, cpu_cpld_ver, cpu_cpld_ver_major, cpu_cpld_ver_minor;
    int cpld_ver[CPLD_MAX], cpld_ver_major[CPLD_MAX], cpld_ver_minor[CPLD_MAX];
    int mb_cpld1_addr = 0x700, mb_cpld1_board_type_rev, mb_cpld1_hw_rev, mb_cpld1_build_rev;
    int i;
    char bios_out[32];
    char bmc_out1[8], bmc_out2[8], bmc_out3[8];
    char ucd_out_t[48], ucd_out_b[48];
    char ucd_ver_t[8], ucd_ver_b[8];
    char ucd_date_t[8], ucd_date_b[8];
    int ucd_len=0, ucd_date_len=6;

    memset(bios_out, 0, sizeof(bios_out));
    memset(bmc_out1, 0, sizeof(bmc_out1));
    memset(bmc_out2, 0, sizeof(bmc_out2));
    memset(bmc_out3, 0, sizeof(bmc_out3));
    memset(ucd_out_t, 0, sizeof(ucd_out_t));
    memset(ucd_out_b, 0, sizeof(ucd_out_b));
    memset(ucd_ver_t, 0, sizeof(ucd_ver_t));
    memset(ucd_ver_b, 0, sizeof(ucd_ver_b));
    memset(ucd_date_t, 0, sizeof(ucd_date_t));
    memset(ucd_date_b, 0, sizeof(ucd_date_b));

    //get CPU CPLD version
    ONLP_TRY(read_ioport(cpu_cpld_addr, &cpu_cpld_ver));
    cpu_cpld_ver_major = (((cpu_cpld_ver) >> 6 & 0x01));
    cpu_cpld_ver_minor = (((cpu_cpld_ver) & 0x3F));

    //get MB CPLD version
    for(i=0; i<CPLD_MAX; ++i) {
        //cpld_ver[i] = onlp_i2c_readb(I2C_BUS_1, CPLD_BASE_ADDR[i], CPLD_REG_VER, ONLP_I2C_F_FORCE);
        ONLP_TRY(file_read_hex(&cpld_ver[i], "/sys/bus/i2c/devices/2-00%02x/cpld_version", CPLD_BASE_ADDR[i]));

        if (cpld_ver[i] < 0) {
            AIM_LOG_ERROR("unable to read MB CPLD version\n");
            return ONLP_STATUS_E_INTERNAL;
        }

        cpld_ver_major[i] = (((cpld_ver[i]) >> 6 & 0x01));
        cpld_ver_minor[i] = (((cpld_ver[i]) & 0x3F));
    }

    info->cpld_versions = aim_fstrdup(
        "\n"
        "    [CPU CPLD] %d.%02d\n"
        "    [MB CPLD1] %d.%02d\n"
        "    [MB CPLD2] %d.%02d\n"
        "    [MB CPLD3] %d.%02d\n"
        "    [MB CPLD4] %d.%02d\n",
        cpu_cpld_ver_major, cpu_cpld_ver_minor,
        cpld_ver_major[0], cpld_ver_minor[0],
        cpld_ver_major[1], cpld_ver_minor[1],
        cpld_ver_major[2], cpld_ver_minor[2],
        cpld_ver_major[3], cpld_ver_minor[3]);

    //Get HW Build Version
    ONLP_TRY(read_ioport(mb_cpld1_addr, &mb_cpld1_board_type_rev));
    mb_cpld1_hw_rev = (((mb_cpld1_board_type_rev) >> 2 & 0x03));
    mb_cpld1_build_rev = (((mb_cpld1_board_type_rev) & 0x03) | ((mb_cpld1_board_type_rev) >> 5 & 0x04));

    //Get BIOS version
    ONLP_TRY(exec_cmd(CMD_BIOS_VER, bios_out, sizeof(bios_out)));

    //Detect bmc status
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

    //Get UCD version - Top Board
    ONLP_TRY(exec_cmd(CMD_UCD_VER_T, ucd_out_t, sizeof(ucd_out_t)));

    //Parse UCD version and date - Top Board
    ucd_len = get_ipmitool_len(ucd_out_t);
    if (ucd_len > ucd_date_len) {
        parse_ucd_out(ucd_out_t, ucd_ver_t, 0, ucd_len-ucd_date_len);
        parse_ucd_out(ucd_out_t, ucd_date_t, ucd_len-ucd_date_len, ucd_date_len);
    } else {
        parse_ucd_out(ucd_out_t, ucd_ver_t, 0, ucd_len);
    }

    //Get UCD version - Bottom Board
    ONLP_TRY(exec_cmd(CMD_UCD_VER_B, ucd_out_b, sizeof(ucd_out_b)));

    //Parse UCD version and date - Bottom Board
    ucd_len = get_ipmitool_len(ucd_out_b);
    if (ucd_len > ucd_date_len) {
        parse_ucd_out(ucd_out_b, ucd_ver_b, 0, ucd_len-ucd_date_len);
        parse_ucd_out(ucd_out_b, ucd_date_b, ucd_len-ucd_date_len, ucd_date_len);
    } else {
        parse_ucd_out(ucd_out_b, ucd_ver_b, 0, ucd_len);
    }

    info->other_versions = aim_fstrdup(
            "\n"
            "    [HW   ] %d\n"
            "    [BUILD] %d\n"
            "    [BIOS ] %s\n"
            "    [BMC  ] %d.%d.%d\n"
            "    [UCD-T] %s %s\n"
            "    [UCD-B] %s %s\n",
            mb_cpld1_hw_rev,
            mb_cpld1_build_rev,
            bios_out,
            atoi(bmc_out1), atoi(bmc_out2), atoi(bmc_out3),
            ucd_ver_t, ucd_date_t, ucd_ver_b, ucd_date_b);

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
    int mb_cpld1_addr = 0x700;
    int mb_cpld1_board_type_rev;
    int mb_cpld1_hw_rev, mb_cpld1_build_rev;

    if (read_ioport(mb_cpld1_addr, &mb_cpld1_board_type_rev) < 0) {
        AIM_LOG_ERROR("unable to read MB CPLD1 Board Type Revision\n");
        return "x86-64-ufispace-s9705-48d-r7";
    }
    mb_cpld1_hw_rev = (((mb_cpld1_board_type_rev) >> 2 & 0x03));
    mb_cpld1_build_rev = (((mb_cpld1_board_type_rev) & 0x03) | ((mb_cpld1_board_type_rev) >> 5 & 0x04));

    if (mb_cpld1_hw_rev == 0 && mb_cpld1_build_rev == 0) {
        return "x86-64-ufispace-s9705-48d-r0";
    } else if (mb_cpld1_hw_rev == 1 && mb_cpld1_build_rev == 0) {
        return "x86-64-ufispace-s9705-48d-r1";
    } else if (mb_cpld1_hw_rev == 1 && mb_cpld1_build_rev == 1) {
        return "x86-64-ufispace-s9705-48d-r2";
    } else if (mb_cpld1_hw_rev == 2 && mb_cpld1_build_rev == 0) {
        return "x86-64-ufispace-s9705-48d-r3";
    } else if (mb_cpld1_hw_rev == 2 && mb_cpld1_build_rev == 1) {
        return "x86-64-ufispace-s9705-48d-r4";
    } else if (mb_cpld1_hw_rev == 2 && mb_cpld1_build_rev == 2) {
        return "x86-64-ufispace-s9705-48d-r5";
    } else if (mb_cpld1_hw_rev == 3 && mb_cpld1_build_rev == 0) {
        return "x86-64-ufispace-s9705-48d-r6";
    } else if (mb_cpld1_hw_rev == 3 && mb_cpld1_build_rev == 1) {
        return "x86-64-ufispace-s9705-48d-r7";
    } else {
        return "x86-64-ufispace-s9705-48d-r7";
    }
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

    if(onlp_file_read(rdata, 512, size, "/sys/bus/i2c/devices/0-0057/eeprom") == ONLP_STATUS_OK) {
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

    memcpy(table, __onlp_oid_info, sizeof(__onlp_oid_info));

#if 0
    /** Add 32 QSFP OIDs after the static table */
    onlp_oid_t* e = table + AIM_ARRAYSIZE(__onlp_oid_info);

    /* 32 QSFP */
    for(i = 1; i <= 32; i++) {
        *e++ = ONLP_SFP_ID_CREATE(i);
    }
#endif

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
    if (info->cpld_versions) {
        aim_free(info->cpld_versions);
    }

    if (info->other_versions) {
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
