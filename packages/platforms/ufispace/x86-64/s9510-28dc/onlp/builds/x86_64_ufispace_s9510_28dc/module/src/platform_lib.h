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
#include "x86_64_ufispace_s9510_28dc_log.h"

#define POID_0 0
#define I2C_BUS(_bus) (_bus)

#define SYS_FMT                     "/sys/bus/i2c/devices/%d-%04x/%s"
#define SYS_FMT_OFFSET              "/sys/bus/i2c/devices/%d-%04x/%s_%d"
#define LPC_FMT                     "/sys/devices/platform/x86_64_ufispace_s9510_28dc_lpc/mb_cpld/"
#define SYS_CPU_CORETEMP_PREFIX     "/sys/devices/platform/coretemp.0/hwmon/hwmon0/"
#define SYS_CPU_CORETEMP_PREFIX2    "/sys/devices/platform/coretemp.0/"

#define BMC_SENSOR_CACHE            "/tmp/bmc_sensor_cache"
#define IPMITOOL_REDIRECT_FIRST_ERR " 2>/tmp/ipmitool_err_msg"
#define IPMITOOL_REDIRECT_ERR       " 2>>/tmp/ipmitool_err_msg"

#define CMD_BMC_SENSOR_CACHE        "ipmitool sdr -c get TEMP_MAC PSU0_TEMP PSU1_TEMP FAN0_RPM FAN1_RPM FAN2_RPM FAN3_RPM FAN4_RPM PSU0_FAN PSU1_FAN FAN0_PRSNT_H FAN1_PRSNT_H FAN2_PRSNT_H FAN3_PRSNT_H FAN4_PRSNT_H PSU0_VIN PSU0_VOUT PSU0_IIN PSU0_IOUT PSU0_STBVOUT PSU0_STBIOUT PSU1_VIN PSU1_VOUT PSU1_IIN PSU1_IOUT PSU1_STBVOUT PSU1_STBIOUT > "BMC_SENSOR_CACHE IPMITOOL_REDIRECT_ERR
#define CMD_BMC_CACHE_GET           "cat "BMC_SENSOR_CACHE" | grep %s | awk -F',' '{print $%d}'"

#define CACHE_OFFSET_THERMAL     (-10)
#define CACHE_OFFSET_FAN_RPM     (2)
#define CACHE_OFFSET_PSU         (14)

#define MB_CPLD1_ID_PATH            "/sys/bus/i2c/devices/1-0030/cpld_id"
#define CPU_MUX_RESET_PATH          "/sys/devices/platform/x86_64_ufispace_s9510_28dc_lpc/cpu_cpld/mux_reset"

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

/* fan_id */
enum onlp_fan_id {
    ONLP_FAN_0 = 1,
    ONLP_FAN_1,
    ONLP_FAN_2,
    ONLP_FAN_3,
    ONLP_FAN_4,
    ONLP_PSU_0_FAN,
    ONLP_PSU_1_FAN,
    ONLP_FAN_MAX,
};

/* led_id */
enum onlp_led_id {
    ONLP_LED_SYS_GNSS = 1,    
    ONLP_LED_SYS_SYNC,    
    ONLP_LED_SYS_SYS,    
    ONLP_LED_SYS_FAN,
    ONLP_LED_SYS_PWR,
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
    ONLP_THERMAL_MAC    = 10,
    ONLP_THERMAL_PSU_0 = 11, 
    ONLP_THERMAL_PSU_1 = 12,    
    ONLP_THERMAL_MAX = 13,
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
