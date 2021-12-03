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
#include "x86_64_ufispace_s9600_72xc_log.h"

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
#define SYS_DEV                 "/sys/bus/i2c/devices/"
/* BMC */
#define BMC_SENSOR_CACHE        "/tmp/bmc_sensor_cache"
#define BMC_FRU_CACHE           "/tmp/bmc_fru_cache.PUS%d"
#define FAN_CACHE_TIME          5
#define PSU_CACHE_TIME          5
#define THERMAL_CACHE_TIME      10
#define FRU_CACHE_TIME          10
/* SYSFS ATTR */
#define MB_CPLD1_SYSFS_PATH     SYS_DEV"1-0030"
#define MB_CPLD2_SYSFS_PATH     SYS_DEV"1-0031"
#define MB_CPLD3_SYSFS_PATH     SYS_DEV"1-0032"
#define MB_CPLD4_SYSFS_PATH     SYS_DEV"1-0033"
#define MB_CPLD_ID_ATTR         "cpld_id"
/* LPC ATTR */
#define SYS_LPC                 "/sys/devices/platform/x86_64_ufispace_s9600_72xc_lpc/"
#define LPC_MB_CPLD_SYFSFS_PATH SYS_LPC"mb_cpld"
#define LPC_CPU_CPLD_SYSFFS_PATH SYS_LPC"cpu_cpld"
#define LPC_MUX_RESET_ATTR      "mux_reset"
/* CMD */
#define CMD_BMC_SENSOR_CACHE    "ipmitool sdr -c get %s > "BMC_SENSOR_CACHE
#define CMD_BMC_CACHE_GET       "cat "BMC_SENSOR_CACHE" | grep %s | awk -F',' '{print $%d}'"
#define CMD_BMC_FRU_CACHE       "ipmitool fru print %d > %s"
#define CMD_CACHE_FRU_GET       "cat %s | grep '%s' | cut -d':' -f2 | awk '{$1=$1};1' | tr -d '\\n'"

enum sensor
{
    FAN_SENSOR = 0,
    PSU_SENSOR,
    THERMAL_SENSOR,
};

enum bmc_cache_idx {
    ID_TEMP_CPU_PECI = 0,
    ID_TEMP_CPU_ENV,
    ID_TEMP_CPU_ENV_2,
    ID_TEMP_MAC_FRONT,
    ID_TEMP_MAC_DIE,
    ID_TEMP_0x48,
    ID_TEMP_0x49,
    ID_PSU0_TEMP,
    ID_PSU1_TEMP,
    ID_FAN0_RPM,
    ID_FAN1_RPM,
    ID_FAN2_RPM,
    ID_FAN3_RPM,
    ID_PSU0_FAN1,
    ID_PSU1_FAN1,
    ID_FAN0_PSNT_L,
    ID_FAN1_PSNT_L,
    ID_FAN2_PSNT_L,
    ID_FAN3_PSNT_L,
    ID_PSU0_VIN,
    ID_PSU0_VOUT,
    ID_PSU0_IIN,
    ID_PSU0_IOUT,
    ID_PSU0_STBVOUT,
    ID_PSU0_STBIOUT,
    ID_PSU1_VIN,
    ID_PSU1_VOUT,
    ID_PSU1_IIN,
    ID_PSU1_IOUT,
    ID_PSU1_STBVOUT,
    ID_PSU1_STBIOUT,
    ID_MAX,
};

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
    ONLP_LED_SYS_SYS = 1,
    ONLP_LED_SYS_FAN,
    ONLP_LED_SYS_PSU0,
    ONLP_LED_SYS_PSU1,
    ONLP_LED_SYS_SYNC,
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
    ONLP_THERMAL_MAC_FRONT,
    ONLP_THERMAL_MAC_DIE,
    ONLP_THERMAL_0x48,
    ONLP_THERMAL_0x49,
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

typedef struct bmc_info_s
{
    char name[20];
    float data;
}bmc_info_t;

typedef struct bmc_fru_info_s
{
    char name[20];
    char data[128];
}bmc_fru_info_t;

int psu_present_get(int *pw_present, int id);

int psu_pwgood_get(int *pw_good, int id);

int exec_cmd(char *cmd, char* out, int size);

int file_read_hex(int* value, const char* fmt, ...);

int file_vread_hex(int* value, const char* fmt, va_list vargs);

void lock_init();

int bmc_fru_read(onlp_psu_info_t* info, int fru_id);

int bmc_sensor_read(int bmc_cache_index, int sensor_type, float *data);

void check_and_do_i2c_mux_reset(int port);

#endif  /* __PLATFORM_LIB_H__ */
