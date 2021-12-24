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
#include <onlp/platformi/psui.h>
#include "x86_64_ufispace_s9701_82dc_log.h"

#define ONLP_TRY(_expr)                                                 \
    do {                                                                \
        int _rv = (_expr);                                              \
        if(ONLP_FAILURE(_rv)) {                                         \
            AIM_LOG_ERROR("%s returned %{onlp_status}", #_expr, _rv);   \
            return _rv;                                                 \
        }                                                               \
    } while(0)

#define READ_BIT(val, bit)      ((0 == (val & (1<<bit))) ? 0 : 1)
#define SET_BIT(val, bit)       (val |= (1 << bit))
#define CLEAR_BIT(val, bit)     (val &= ~(1 << bit))

#define POID_0                  0
#define PSU_NUM                 2
#define SYS_DEV                     "/sys/bus/i2c/devices/"
/* SYSFS ATTR */
#define MB_CPLD1_SYSFS_PATH     SYS_DEV "1-0030"
#define MB_CPLD2_SYSFS_PATH     SYS_DEV "1-0031"
#define MB_CPLD3_SYSFS_PATH     SYS_DEV "1-0032"
#define MB_CPLD4_SYSFS_PATH     SYS_DEV "1-0033"
/* LPC ATTR*/
#define SYS_LPC                 "/sys/devices/platform/x86_64_ufispace_s9701_82dc_lpc"
#define LPC_MB_CPLD_PATH        SYS_LPC "/mb_cpld"
#define LPC_CPU_CPLD_PATH       SYS_LPC "/cpu_cpld"
/* LENGTH */
#define BMC_FRU_ATTR_KEY_VALUE_SIZE 256
#define BMC_FRU_ATTR_KEY_VALUE_LEN  (BMC_FRU_ATTR_KEY_VALUE_SIZE - 1)
/* error redirect */
#define IPMITOOL_REDIRECT_FIRST_ERR " 2>/tmp/ipmitool_err_msg"
#define IPMITOOL_REDIRECT_ERR       " 2>>/tmp/ipmitool_err_msg"

/* fan_id */
enum onlp_fan_id {
    ONLP_FAN_0 = 1,
    ONLP_FAN_1,
    ONLP_FAN_2,
    ONLP_FAN_3,
    ONLP_PSU_0_FAN,
    ONLP_PSU_1_FAN,
    ONLP_FAN_MAX,
};

/* led_id */
enum onlp_led_id {
    ONLP_LED_SYS_SYNC = 1,
    ONLP_LED_SYS_SYS,
    ONLP_LED_SYS_FAN,
    ONLP_LED_SYS_PSU0,
    ONLP_LED_SYS_PSU1,
    ONLP_LED_MAX
};

/* psu_id */
enum onlp_psu_id {
    ONLP_PSU_0 = 1,
    ONLP_PSU_1,
    ONLP_PSU_MAX,
};

/* thermal_id */
enum onlp_thermal_id {
    ONLP_THERMAL_CPU_PECI = 1,
    ONLP_THERMAL_CPU_ENV,
    ONLP_THERMAL_CPU_ENV_2,
    ONLP_THERMAL_MAC_ENV,
    ONLP_THERMAL_MAC_DIE,
    ONLP_THERMAL_ENV_FRONT,
    ONLP_THERMAL_ENV_REAR,
    ONLP_THERMAL_PSU0,
    ONLP_THERMAL_PSU1,
    ONLP_THERMAL_CPU_PKG,
    ONLP_THERMAL_CPU1,
    ONLP_THERMAL_CPU2,
    ONLP_THERMAL_CPU3,
    ONLP_THERMAL_CPU4,
    ONLP_THERMAL_CPU5,
    ONLP_THERMAL_CPU6,
    ONLP_THERMAL_CPU7,
    ONLP_THERMAL_CPU8,
    ONLP_THERMAL_MAX,
};

enum bmc_attr_id {
    BMC_ATTR_ID_TEMP_CPU_PECI = 0,
    BMC_ATTR_ID_TEMP_CPU_ENV,
    BMC_ATTR_ID_TEMP_CPU_ENV_2,
    BMC_ATTR_ID_TEMP_MAC_ENV,
    BMC_ATTR_ID_TEMP_MAC_DIE,
    BMC_ATTR_ID_TEMP_ENV_FRONT,
    BMC_ATTR_ID_TEMP_ENV_REAR,
    BMC_ATTR_ID_PSU0_TEMP,
    BMC_ATTR_ID_PSU1_TEMP,
    BMC_ATTR_ID_FAN0_RPM,
    BMC_ATTR_ID_FAN1_RPM,
    BMC_ATTR_ID_FAN2_RPM,
    BMC_ATTR_ID_FAN3_RPM,
    BMC_ATTR_ID_PSU0_FAN1,
    BMC_ATTR_ID_PSU1_FAN1,
    BMC_ATTR_ID_FAN0_PSNT_L,
    BMC_ATTR_ID_FAN1_PSNT_L,
    BMC_ATTR_ID_FAN2_PSNT_L,
    BMC_ATTR_ID_FAN3_PSNT_L,
    BMC_ATTR_ID_PSU0_VIN,
    BMC_ATTR_ID_PSU0_VOUT,
    BMC_ATTR_ID_PSU0_IIN,
    BMC_ATTR_ID_PSU0_IOUT,
    BMC_ATTR_ID_PSU0_STBVOUT,
    BMC_ATTR_ID_PSU0_STBIOUT,
    BMC_ATTR_ID_PSU1_VIN,
    BMC_ATTR_ID_PSU1_VOUT,
    BMC_ATTR_ID_PSU1_IIN,
    BMC_ATTR_ID_PSU1_IOUT,
    BMC_ATTR_ID_PSU1_STBVOUT,
    BMC_ATTR_ID_PSU1_STBIOUT,
    BMC_ATTR_ID_MAX,
};

enum sensor
{
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
}bmc_info_t;

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

int psu_present_get(int *pw_present, int id);

int psu_pwgood_get(int *pw_good, int id);

int exec_cmd(char *cmd, char* out, int size);

int file_read_hex(int* value, const char* fmt, ...);

int file_vread_hex(int* value, const char* fmt, va_list vargs);

void lock_init();

int bmc_fru_read(int local_id, bmc_fru_t *data);

int bmc_sensor_read(int bmc_cache_index, int sensor_type, float *data);

void check_and_do_i2c_mux_reset(int port);

int bmc_check_alive(void);

#endif  /* __PLATFORM_LIB_H__ */
