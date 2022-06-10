/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
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
 * Platform Library
 *
 ***********************************************************/
#ifndef __PLATFORM_LIB_H__
#define __PLATFORM_LIB_H__

#include <onlp/onlp.h>
#include <onlplib/file.h>
#include <onlplib/i2c.h>
#include "x86_64_ufispace_s9301_32db_log.h"
#include <onlp/psu.h>

/* This is definitions for x86-64-ufispace-s9301-32db */
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
 *            |----[10] ONLP_THERMAL_CPU1
 *            |----[11] ONLP_THERMAL_CPU2
 *            |----[12] ONLP_THERMAL_CPU3
 *            |----[13] ONLP_THERMAL_CPU4
 *            |----[14] ONLP_THERMAL_CPU5
 *            |----[15] ONLP_THERMAL_CPU6
 *            |----[16] ONLP_THERMAL_CPU7
 *            |----[17] ONLP_THERMAL_CPU8
 *            |----[01] ONLP_LED_SYSTEM
 *            |----[02] ONLP_LED_PSU0
 *            |----[03] ONLP_LED_PSU1
 *            |----[04] ONLP_LED_FAN
 *            |----[01] ONLP_PSU0----[13] ONLP_PSU0_FAN1
 *            |                 |----[07] ONLP_THERMAL_PSU0
 *            |
 *            |----[02] ONLP_PSU1----[14] ONLP_PSU1_FAN1
 *            |                 |----[08] ONLP_THERMAL_PSU1
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

#define ONLP_TRY(_expr)                                                 \
    do {                                                                \
        int _rv = (_expr);                                              \
        if(ONLP_FAILURE(_rv)) {                                         \
            AIM_LOG_ERROR("%s returned %{onlp_status}", #_expr, _rv);   \
            return _rv;                                                 \
        }                                                               \
    } while(0)
#define POID_0                  0
/* SYSFS */
#define SYS_DEV                 "/sys/bus/i2c/devices/"
#define CPLD1_SYSFS_PATH        SYS_DEV "2-0030"
#define CPLD2_SYSFS_PATH        SYS_DEV "2-0031"
#define CPLD3_SYSFS_PATH        SYS_DEV "2-0032"
#define LPC_PATH                "/sys/devices/platform/x86_64_ufispace_s9301_32db_lpc"
#define LPC_CPU_CPLD_PATH       LPC_PATH "/cpu_cpld"
#define LPC_MB_CPLD_PATH        LPC_PATH "/mb_cpld"
/* FAN DIR */
#define FAN_DIR_B2F             1
#define FAN_DIR_F2B             2
/* LENGTH */
#define BMC_FRU_LINE_SIZE           256
#define BMC_FRU_ATTR_KEY_VALUE_SIZE ONLP_CONFIG_INFO_STR_MAX
#define BMC_FRU_ATTR_KEY_VALUE_LEN  (BMC_FRU_ATTR_KEY_VALUE_SIZE - 1)
/* error redirect */
#define IPMITOOL_REDIRECT_FIRST_ERR " 2>/tmp/ipmitool_err_msg"
#define IPMITOOL_REDIRECT_ERR   " 2>>/tmp/ipmitool_err_msg"

/* Thermal definitions*/
enum onlp_thermal_id {
    ONLP_THERMAL_RESERVED  = 0,
    ONLP_THERMAL_CPU_PECI  = 1,
    ONLP_THERMAL_CPU_ENV   = 2,
    ONLP_THERMAL_CPU_ENV_2 = 3,
    ONLP_THERMAL_MAC_ENV   = 4,
    ONLP_THERMAL_CAGE      = 5,
    ONLP_THERMAL_PSU_CONN  = 6,
    ONLP_THERMAL_PSU0      = 7,
    ONLP_THERMAL_PSU1      = 8,
    ONLP_THERMAL_CPU_PKG   = 9,
    ONLP_THERMAL_CPU1      = 10,
    ONLP_THERMAL_CPU2      = 11,
    ONLP_THERMAL_CPU3      = 12,
    ONLP_THERMAL_CPU4      = 13,
    ONLP_THERMAL_CPU5      = 14,
    ONLP_THERMAL_CPU6      = 15,
    ONLP_THERMAL_CPU7      = 16,
    ONLP_THERMAL_CPU8      = 17,
    ONLP_THERMAL_MAX       = ONLP_THERMAL_CPU8+1,
};

#define ONLP_THERMAL_COUNT ONLP_THERMAL_MAX /*include "reserved"*/


/* LED definitions*/
enum onlp_led_id {
    ONLP_LED_RESERVED  = 0,
    ONLP_LED_SYSTEM    = 1,
    ONLP_LED_PSU0      = 2,
    ONLP_LED_PSU1      = 3,
    ONLP_LED_FAN       = 4,
    ONLP_LED_MAX       = ONLP_LED_FAN+1,
};

/* PSU definitions*/
enum onlp_psu_id {
    ONLP_PSU_RESERVED  = 0,
    ONLP_PSU0          = 1,
    ONLP_PSU1          = 2,
    ONLP_PSU0_VIN      = 3,
    ONLP_PSU0_VOUT     = 4,
    ONLP_PSU0_IIN      = 5,
    ONLP_PSU0_IOUT     = 6,
    ONLP_PSU0_STBVOUT  = 7,
    ONLP_PSU0_STBIOUT  = 8,
    ONLP_PSU1_VIN      = 9,
    ONLP_PSU1_VOUT     = 10,
    ONLP_PSU1_IIN      = 11,
    ONLP_PSU1_IOUT     = 12,
    ONLP_PSU1_STBVOUT  = 13,
    ONLP_PSU1_STBIOUT  = 14,
    ONLP_PSU_MAX       = ONLP_PSU1+1,
};

#define ONLP_PSU_COUNT ONLP_PSU_MAX /*include "reserved"*/


/* Fan definitions*/
enum onlp_fan_id {
    ONLP_FAN_RESERVED = 0,
    ONLP_FAN1_F       = 1,
    ONLP_FAN1_R       = 2,
    ONLP_FAN2_F       = 3,
    ONLP_FAN2_R       = 4,
    ONLP_FAN3_F       = 5,
    ONLP_FAN3_R       = 6,
    ONLP_FAN4_F       = 7,
    ONLP_FAN4_R       = 8,
    ONLP_FAN5_F       = 9,
    ONLP_FAN5_R       = 10,
    ONLP_FAN6_F       = 11,
    ONLP_FAN6_R       = 12,
    ONLP_PSU0_FAN1    = 13,
    ONLP_PSU1_FAN1    = 14,
    ONLP_FAN_MAX      = ONLP_PSU1_FAN1+1,
};

#define ONLP_FAN_COUNT ONLP_FAN_MAX /*include "reserved"*/


#define CPLD_MAX 3  //Number of MB CPLD
extern const int CPLD_BASE_ADDR[CPLD_MAX];

enum bmc_attr_id {
    BMC_ATTR_ID_TEMP_CPU_PECI  = 0,
    BMC_ATTR_ID_TEMP_CPU_ENV   = 1,
    BMC_ATTR_ID_TEMP_CPU_ENV_2 = 2,
    BMC_ATTR_ID_TEMP_MAC_ENV   = 3,
    BMC_ATTR_ID_TEMP_CAGE      = 4,
    BMC_ATTR_ID_TEMP_PSU_CONN  = 5,
    BMC_ATTR_ID_PSU0_TEMP      = 6,
    BMC_ATTR_ID_PSU1_TEMP      = 7,
    BMC_ATTR_ID_FAN0_RPM_F     = 8,
    BMC_ATTR_ID_FAN0_RPM_R     = 9,
    BMC_ATTR_ID_FAN1_RPM_F     = 10,
    BMC_ATTR_ID_FAN1_RPM_R     = 11,
    BMC_ATTR_ID_FAN2_RPM_F     = 12,
    BMC_ATTR_ID_FAN2_RPM_R     = 13,
    BMC_ATTR_ID_FAN3_RPM_F     = 14,
    BMC_ATTR_ID_FAN3_RPM_R     = 15,
    BMC_ATTR_ID_FAN4_RPM_F     = 16,
    BMC_ATTR_ID_FAN4_RPM_R     = 17,
    BMC_ATTR_ID_FAN5_RPM_F     = 18,
    BMC_ATTR_ID_FAN5_RPM_R     = 19,
    BMC_ATTR_ID_PSU0_FAN1      = 20,
    BMC_ATTR_ID_PSU1_FAN1      = 21,
    BMC_ATTR_ID_FAN0_PSNT_L    = 22,
    BMC_ATTR_ID_FAN1_PSNT_L    = 23,
    BMC_ATTR_ID_FAN2_PSNT_L    = 24,
    BMC_ATTR_ID_FAN3_PSNT_L    = 25,
    BMC_ATTR_ID_FAN4_PSNT_L    = 26,
    BMC_ATTR_ID_FAN5_PSNT_L    = 27,
    BMC_ATTR_ID_FAN0_DIR       = 28,
    BMC_ATTR_ID_FAN1_DIR       = 29,
    BMC_ATTR_ID_FAN2_DIR       = 30,
    BMC_ATTR_ID_FAN3_DIR       = 31,
    BMC_ATTR_ID_FAN4_DIR       = 32,
    BMC_ATTR_ID_FAN5_DIR       = 33,
    BMC_ATTR_ID_PSU0_FAN1_DIR  = 34,
    BMC_ATTR_ID_PSU1_FAN1_DIR  = 35,
    BMC_ATTR_ID_PSU0_VIN       = 36,
    BMC_ATTR_ID_PSU0_VOUT      = 37,
    BMC_ATTR_ID_PSU0_IIN       = 38,
    BMC_ATTR_ID_PSU0_IOUT      = 39,
    BMC_ATTR_ID_PSU0_STBVOUT   = 40,
    BMC_ATTR_ID_PSU0_STBIOUT   = 41,
    BMC_ATTR_ID_PSU1_VIN       = 42,
    BMC_ATTR_ID_PSU1_VOUT      = 43,
    BMC_ATTR_ID_PSU1_IIN       = 44,
    BMC_ATTR_ID_PSU1_IOUT      = 45,
    BMC_ATTR_ID_PSU1_STBVOUT   = 46,
    BMC_ATTR_ID_PSU1_STBIOUT   = 47,
    BMC_ATTR_ID_MAX            = 48,
};

enum sensor {
    FAN_SENSOR = 0,
    PSU_SENSOR,
    THERMAL_SENSOR,
};

enum onlp_psu_type_e {
  ONLP_PSU_TYPE_AC,
  ONLP_PSU_TYPE_DC12,
  ONLP_PSU_TYPE_DC48,
  ONLP_PSU_TYPE_LAST = ONLP_PSU_TYPE_DC48,
  ONLP_PSU_TYPE_COUNT,
  ONLP_PSU_TYPE_INVALID = -1
};

typedef struct bmc_info_s {
    char name[20];
    float data;
} bmc_info_t;

typedef struct bmc_fru_attr_s {
    char key[BMC_FRU_ATTR_KEY_VALUE_SIZE];
    char val[BMC_FRU_ATTR_KEY_VALUE_SIZE];
} bmc_fru_attr_t;

typedef struct bmc_fru_s {
    int bmc_fru_id;
    char init_done;
    char cache_files[BMC_FRU_ATTR_KEY_VALUE_SIZE];
    bmc_fru_attr_t vendor;
    bmc_fru_attr_t name;
    bmc_fru_attr_t part_num;
    bmc_fru_attr_t serial;
} bmc_fru_t;

typedef struct psu_support_info_s {
    char vendor[BMC_FRU_ATTR_KEY_VALUE_SIZE];
    char part_number[BMC_FRU_ATTR_KEY_VALUE_SIZE];
    int type;
} psu_support_info_t;

void lock_init();
int check_file_exist(char *file_path, long *file_time);
int bmc_check_alive(void);
int bmc_cache_expired_check(long last_time, long new_time, int cache_time);
int bmc_fan_dir_read(int bmc_cache_index, float *data);
int bmc_sensor_read(int bmc_cache_index, int sensor_type, float *data);
int bmc_fru_read(int local_id, bmc_fru_t *data);
int read_ioport(int addr, int *reg_val);
int exec_cmd(char *cmd, char* out, int size);
int file_read_hex(int* value, const char* fmt, ...);
int file_vread_hex(int* value, const char* fmt, va_list vargs);
int get_psui_present_status(int local_id, int *status);
void check_and_do_i2c_mux_reset(int port);
int bmc_check_alive(void);

#endif  /* __PLATFORM_LIB_H__ */
