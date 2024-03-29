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

#include "x86_64_ufispace_s9705_48d_log.h"

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
 *            |----[10] ONLP_THERMAL_BMC_ENV
 *            |----[11] ONLP_THERMAL_ENV_FRONT_T
 *            |----[12] ONLP_THERMAL_ENV_FRONT_B
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

#define IPMITOOL_REDIRECT_FIRST_ERR " 2>/tmp/ipmitool_err_msg"
#define IPMITOOL_REDIRECT_ERR       " 2>>/tmp/ipmitool_err_msg"

#define ONLP_TRY(_expr)                                                 \
    do {                                                                \
        int _rv = (_expr);                                              \
        if(ONLP_FAILURE(_rv)) {                                         \
            AIM_LOG_ERROR("%s returned %{onlp_status}", #_expr, _rv);   \
            return _rv;                                                 \
        }                                                               \
    } while(0)

/* Thermal definitions*/
enum onlp_thermal_id {
    ONLP_THERMAL_RESERVED    = 0,
    ONLP_THERMAL_CPU_PECI    = 1,
    ONLP_THERMAL_RAMON_ENV_T = 2,
    ONLP_THERMAL_RAMON_DIE_T = 3,
    ONLP_THERMAL_RAMON_ENV_B = 4,
    ONLP_THERMAL_RAMON_DIE_B = 5,
    ONLP_THERMAL_PSU0        = 6,
    ONLP_THERMAL_PSU1        = 7,
    ONLP_THERMAL_CPU_PKG     = 8,
    ONLP_THERMAL_CPU_BOARD   = 9,
    ONLP_THERMAL_BMC_ENV     = 10,
    ONLP_THERMAL_ENV_FRONT_T = 11,
    ONLP_THERMAL_ENV_FRONT_B = 12,
    ONLP_THERMAL_MAX         = ONLP_THERMAL_ENV_FRONT_B+1,
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

#define ONLP_LED_COUNT ONLP_LED_MAX /*include "reserved"*/


/* PSU definitions*/
enum onlp_psu_id {
    ONLP_PSU_RESERVED  = 0,
    ONLP_PSU_0         = 1,
    ONLP_PSU_1         = 2,
    ONLP_PSU_0_VIN     = 3,
    ONLP_PSU_0_VOUT    = 4,
    ONLP_PSU_0_IIN     = 5,
    ONLP_PSU_0_IOUT    = 6,
    ONLP_PSU_0_STBVOUT = 7,
    ONLP_PSU_0_STBIOUT = 8,
    ONLP_PSU_1_VIN     = 9,
    ONLP_PSU_1_VOUT    = 10,
    ONLP_PSU_1_IIN     = 11,
    ONLP_PSU_1_IOUT    = 12,
    ONLP_PSU_1_STBVOUT = 13,
    ONLP_PSU_1_STBIOUT = 14,
    ONLP_PSU_MAX       = ONLP_PSU_1+1,
};

#define ONLP_PSU_COUNT ONLP_PSU_MAX /*include "reserved"*/


/* Fan definitions*/
enum onlp_fan_id {
    ONLP_FAN_RESERVED = 0,
    ONLP_FAN_0        = 1,
    ONLP_FAN_1        = 2,
    ONLP_FAN_2        = 3,
    ONLP_FAN_3        = 4,
    ONLP_PSU0_FAN_1   = 5,
    ONLP_PSU0_FAN_2   = 6,
    ONLP_PSU1_FAN_1   = 7,
    ONLP_PSU1_FAN_2   = 8,
    ONLP_FAN_MAX      = ONLP_PSU1_FAN_2+1,
};

#define ONLP_FAN_COUNT ONLP_FAN_MAX /*include "reserved"*/

#define CPLD_MAX 4  //Number of MB CPLD
extern const int CPLD_BASE_ADDR[CPLD_MAX];

#define COMM_STR_NOT_SUPPORTED              "not supported"
#define COMM_STR_NOT_AVAILABLE              "not available"

/* For BMC Cached Mechanism */
enum bmc_attr_id {
    BMC_ATTR_ID_TEMP_CPU_PECI     = 0,
    BMC_ATTR_ID_TEMP_RAMON_ENV_T  = 1,
    BMC_ATTR_ID_TEMP_RAMON_DIE_T  = 2,
    BMC_ATTR_ID_TEMP_RAMON_ENV_B  = 3,
    BMC_ATTR_ID_TEMP_RAMON_DIE_B  = 4,
    BMC_ATTR_ID_PSU0_TEMP         = 5,
    BMC_ATTR_ID_PSU1_TEMP         = 6,
    BMC_ATTR_ID_TEMP_BMC_ENV      = 7,
    BMC_ATTR_ID_TEMP_ENV_FRONT_T  = 8,
    BMC_ATTR_ID_TEMP_ENV_FRONT_B  = 9,
    BMC_ATTR_ID_FAN0_RPM          = 10,
    BMC_ATTR_ID_FAN1_RPM          = 11,
    BMC_ATTR_ID_FAN2_RPM          = 12,
    BMC_ATTR_ID_FAN3_RPM          = 13,
    BMC_ATTR_ID_PSU0_FAN1         = 14,
    BMC_ATTR_ID_PSU0_FAN2         = 15,
    BMC_ATTR_ID_PSU1_FAN1         = 16,
    BMC_ATTR_ID_PSU1_FAN2         = 17,
    BMC_ATTR_ID_FAN0_PRSNT_H      = 18,
    BMC_ATTR_ID_FAN1_PRSNT_H      = 19,
    BMC_ATTR_ID_FAN2_PRSNT_H      = 20,
    BMC_ATTR_ID_FAN3_PRSNT_H      = 21,
    BMC_ATTR_ID_PSU0_VIN          = 22,
    BMC_ATTR_ID_PSU0_VOUT         = 23,
    BMC_ATTR_ID_PSU0_IIN          = 24,
    BMC_ATTR_ID_PSU0_IOUT         = 25,
    BMC_ATTR_ID_PSU0_STBVOUT      = 26,
    BMC_ATTR_ID_PSU0_STBIOUT      = 27,
    BMC_ATTR_ID_PSU1_VIN          = 28,
    BMC_ATTR_ID_PSU1_VOUT         = 29,
    BMC_ATTR_ID_PSU1_IIN          = 30,
    BMC_ATTR_ID_PSU1_IOUT         = 31,
    BMC_ATTR_ID_PSU1_STBVOUT      = 32,
    BMC_ATTR_ID_PSU1_STBIOUT      = 33,
    BMC_ATTR_ID_MAX               = 34,
};

enum fru_attr_id {
    FRU_ATTR_ID_PSU0_VENDOR,
    FRU_ATTR_ID_PSU0_NAME,
    FRU_ATTR_ID_PSU0_MODEL,
    FRU_ATTR_ID_PSU0_SERIAL,
    FRU_ATTR_ID_PSU1_VENDOR,
    FRU_ATTR_ID_PSU1_NAME,
    FRU_ATTR_ID_PSU1_MODEL,
    FRU_ATTR_ID_PSU1_SERIAL,
    FRU_ATTR_ID_MAX
};

enum sensor {
    FAN_SENSOR = 0,
    PSU_SENSOR,
    THERMAL_SENSOR,
};

typedef struct bmc_info_s {
    char name[20];
    float data;
} bmc_info_t;

#define BMC_FRU_LINE_SIZE            256
#define BMC_FRU_ATTR_KEY_VALUE_SIZE  ONLP_CONFIG_INFO_STR_MAX
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


void lock_init();
int check_file_exist(char *file_path, long *file_time);
int bmc_check_alive(void);
int bmc_cache_expired_check(long last_time, long new_time, int cache_time);
int bmc_sensor_read(int bmc_cache_index, int sensor_type, float *data);
int bmc_fru_read(int local_id, bmc_fru_t *data);
int read_ioport(int addr, int *reg_val);
int exec_cmd(char *cmd, char* out, int size);
int get_ipmitool_len(char *ipmitool_out) ;
int file_read_hex(int* value, const char* fmt, ...);
int file_vread_hex(int* value, const char* fmt, va_list vargs);
int get_psui_present_status(int local_id, int *status);
void check_and_do_i2c_mux_reset(int port);


#endif  /* __PLATFORM_LIB_H__ */
