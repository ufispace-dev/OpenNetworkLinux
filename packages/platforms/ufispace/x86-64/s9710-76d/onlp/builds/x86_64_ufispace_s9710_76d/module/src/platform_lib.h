/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2013 Accton Technology Corporation.
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
#include "x86_64_ufispace_s9710_76d_log.h"

#define POID_0 0
#define I2C_BUS(_bus) (_bus)

#define SYS_FMT                     "/sys/bus/i2c/devices/%d-%04x/%s"
#define SYS_FMT_OFFSET              "/sys/bus/i2c/devices/%d-%04x/%s_%d"
#define SYS_CPU_CORETEMP_PREFIX     "/sys/devices/platform/coretemp.0/hwmon/hwmon0/"
#define SYS_CPU_CORETEMP_PREFIX2    "/sys/devices/platform/coretemp.0/"

#define BMC_SENSOR_CACHE            "/tmp/bmc_sensor_cache"
#define IPMITOOL_REDIRECT_FIRST_ERR " 2>/tmp/ipmitool_err_msg"
#define IPMITOOL_REDIRECT_ERR       " 2>>/tmp/ipmitool_err_msg"

#define CMD_BMC_SENSOR_CACHE        "ipmitool sdr -c get ADC_CPU_TEMP TEMP_CPU_PECI TEMP_MAC_ENV_1 TEMP_MAC_ENV_2 TEMP_FRONT_ENV_1 TEMP_FRONT_ENV_2 TEMP_ENV_1 TEMP_ENV_2 TEMP_EXT_ENV_1 TEMP_EXT_ENV_2 PSU0_TEMP PSU1_TEMP FAN0_FRONT_RPM FAN0_REAR_RPM FAN1_FRONT_RPM FAN1_REAR_RPM FAN2_FRONT_RPM FAN2_REAR_RPM  FAN3_FRONT_RPM FAN3_REAR_RPM PSU0_FAN PSU1_FAN FAN0_PRSNT_H FAN1_PRSNT_H FAN2_PRSNT_H FAN3_PRSNT_H PSU0_VIN PSU0_VOUT PSU0_IIN PSU0_IOUT PSU0_STBVOUT PSU0_STBIOUT PSU1_VIN PSU1_VOUT PSU1_IIN PSU1_IOUT PSU1_STBVOUT PSU1_STBIOUT > "BMC_SENSOR_CACHE IPMITOOL_REDIRECT_ERR
#define CMD_FRU_INFO_GET            "ipmitool fru print %d"IPMITOOL_REDIRECT_ERR" | grep '%s' | cut -d':' -f2 | awk '{$1=$1};1' | tr -d '\n'"
#define CMD_BMC_CACHE_GET           "cat "BMC_SENSOR_CACHE" | grep %s | awk -F',' '{print $%d}'"

#define MB_CPLD1_ID_PATH            "/sys/bus/i2c/devices/1-0030/cpld_id"
#define MUX_RESET_PATH          "/sys/devices/platform/x86_64_ufispace_s9710_76d_lpc/mb_cpld/mux_reset"

/* SYS */
#define CPLD_MAX      5
extern const int CPLD_BASE_ADDR[CPLD_MAX];
extern const int CPLD_I2C_BUS[CPLD_MAX];

/* BMC CMD */
#define FAN_CACHE_TIME          5
#define PSU_CACHE_TIME          5
#define THERMAL_CACHE_TIME  10

enum sensor
{
    FAN_SENSOR = 0,
    PSU_SENSOR,
    THERMAL_SENSOR,
};

enum bmc_attr_id {
    BMC_ATTR_ID_ADC_CPU_TEMP,
    BMC_ATTR_ID_TEMP_CPU_PECI,
    BMC_ATTR_ID_TEMP_MAC_ENV_1,
    BMC_ATTR_ID_TEMP_MAC_ENV_2,
    BMC_ATTR_ID_TEMP_FRONT_ENV_1,
    BMC_ATTR_ID_TEMP_FRONT_ENV_2,
    BMC_ATTR_ID_TEMP_ENV_1,
    BMC_ATTR_ID_TEMP_ENV_2,
    BMC_ATTR_ID_TEMP_EXT_ENV_1,
    BMC_ATTR_ID_TEMP_EXT_ENV_2,
    BMC_ATTR_ID_PSU0_TEMP,
    BMC_ATTR_ID_PSU1_TEMP,
    BMC_ATTR_ID_FAN0_FRONT_RPM,
    BMC_ATTR_ID_FAN0_REAR_RPM,
    BMC_ATTR_ID_FAN1_FRONT_RPM,
    BMC_ATTR_ID_FAN1_REAR_RPM,
    BMC_ATTR_ID_FAN2_FRONT_RPM,
    BMC_ATTR_ID_FAN2_REAR_RPM,
    BMC_ATTR_ID_FAN3_FRONT_RPM,
    BMC_ATTR_ID_FAN3_REAR_RPM,
    BMC_ATTR_ID_PSU0_FAN,
    BMC_ATTR_ID_PSU1_FAN,
    BMC_ATTR_ID_FAN0_PRSNT_H,
    BMC_ATTR_ID_FAN1_PRSNT_H,
    BMC_ATTR_ID_FAN2_PRSNT_H,
    BMC_ATTR_ID_FAN3_PRSNT_H,
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
    BMC_ATTR_ID_MAX
};

/* fan_id */
enum onlp_fan_id {
    ONLP_FAN_F_0 = 1,
    ONLP_FAN_R_0,
    ONLP_FAN_F_1,
    ONLP_FAN_R_1,
    ONLP_FAN_F_2,
    ONLP_FAN_R_2,
    ONLP_FAN_F_3,
    ONLP_FAN_R_3,
    ONLP_PSU_0_FAN,
    ONLP_PSU_1_FAN,
    ONLP_FAN_MAX,
};

/* led_id */
enum onlp_led_id {
    ONLP_LED_SYS_SYS = 1,    
    ONLP_LED_SYS_FAN,
    ONLP_LED_SYS_PSU_0,
    ONLP_LED_SYS_PSU_1,    
    ONLP_LED_MAX
};

/* psu_id */
enum onlp_psu_id {
    ONLP_PSU_0      = 1,
    ONLP_PSU_1      = 2,
    ONLP_PSU_0_VIN  = 3,
    ONLP_PSU_0_VOUT = 4,
    ONLP_PSU_0_IIN  = 5,
    ONLP_PSU_0_IOUT = 6,
    ONLP_PSU_0_STBVOUT = 7,
    ONLP_PSU_0_STBIOUT = 8,
    ONLP_PSU_1_VIN  = 9,
    ONLP_PSU_1_VOUT = 10,
    ONLP_PSU_1_IIN  = 11,
    ONLP_PSU_1_IOUT = 12,
    ONLP_PSU_1_STBVOUT = 13,
    ONLP_PSU_1_STBIOUT = 14,
    ONLP_PSU_MAX = 15,
};

/* thermal_id */
enum onlp_thermal_id {
    ONLP_THERMAL_CPU_PKG = 1,
    ONLP_THERMAL_CPU_0 = 2,
    ONLP_THERMAL_CPU_1 = 3,
    ONLP_THERMAL_CPU_2 = 4,
    ONLP_THERMAL_CPU_3 = 5,
    ONLP_THERMAL_CPU_4 = 6,
    ONLP_THERMAL_CPU_5 = 7,
    ONLP_THERMAL_CPU_6 = 8,
    ONLP_THERMAL_CPU_7 = 9,
    ONLP_THERMAL_ADC_CPU   = 10,
    ONLP_THERMAL_CPU_PECI  = 11,
    ONLP_THERMAL_MAC_ENV_1 = 12,
    ONLP_THERMAL_MAC_ENV_2 = 13,
    ONLP_THERMAL_FRONT_ENV_1 = 14,
    ONLP_THERMAL_FRONT_ENV_2 = 15,
    ONLP_THERMAL_ENV_1 = 16,
    ONLP_THERMAL_ENV_2 = 17,
    ONLP_THERMAL_EXT_ENV_1 = 18,
    ONLP_THERMAL_EXT_ENV_2 = 19,    
    ONLP_THERMAL_PSU_0 = 20, 
    ONLP_THERMAL_PSU_1 = 21,    
    ONLP_THERMAL_MAX = 22,
};

typedef struct bmc_info_s
{
    char name[20];
    float data;
} bmc_info_t;

int read_ioport(int addr, int *reg_val);

int exec_cmd(char *cmd, char* out, int size);

int file_read_hex(int* value, const char* fmt, ...);

int file_vread_hex(int* value, const char* fmt, va_list vargs);

void lock_init();

int bmc_sensor_read(int bmc_cache_index, int sensor_type, float *data);

void check_and_do_i2c_mux_reset(int port);

uint8_t ufi_shift(uint8_t mask);

uint8_t ufi_mask_shift(uint8_t val, uint8_t mask);

uint8_t ufi_bit_operation(uint8_t reg_val, uint8_t bit, uint8_t bit_val);

#endif  /* __PLATFORM_LIB_H__ */
