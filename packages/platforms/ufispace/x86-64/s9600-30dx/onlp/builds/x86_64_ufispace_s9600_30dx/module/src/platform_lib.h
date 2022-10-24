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
#include "x86_64_ufispace_s9600_30dx_log.h"

#define SYSFS_PLTM                   "/sys/devices/platform/"
#define SYSFS_DEVICES        "/sys/bus/i2c/devices/" 

#define SYSFS_LPC                 SYSFS_PLTM "x86_64_ufispace_s9600_30dx_lpc/"
#define SYSFS_LPC_MB_CPLD  SYSFS_LPC "mb_cpld/"
#define SYSFS_HW_ID           SYSFS_LPC_MB_CPLD "board_hw_id"
#define SYSFS_DEPH_ID        SYSFS_LPC_MB_CPLD "board_deph_id"
#define SYSFS_BUILD_ID       SYSFS_LPC_MB_CPLD "board_build_id"
//#define SYSFS_EXT_ID           SYSFS_LPC_MB_CPLD "board_ext_id"
#define SYSFS_MUX_RESET    SYSFS_LPC_MB_CPLD "mux_reset"
#define SYSFS_CPLD1            SYSFS_DEVICES "1-0030/"
#define SYSFS_CPLD1_ID       SYSFS_CPLD1 "cpld_id"
#define SYS_FMT                     SYSFS_DEVICES "%d-%04x/%s"
#define SYS_FMT_OFFSET       SYSFS_DEVICES "%d-%04x/%s_%s"
#define SYS_CPU_CORETEMP_PREFIX     SYSFS_PLTM "coretemp.0/hwmon/hwmon0/"
#define SYS_CPU_CORETEMP_PREFIX2    SYSFS_PLTM "coretemp.0/"

#define BMC_SENSOR_CACHE            "/tmp/bmc_sensor_cache"
#define IPMITOOL_REDIRECT_FIRST_ERR " 2>/tmp/ipmitool_err_msg"
#define IPMITOOL_REDIRECT_ERR       " 2>>/tmp/ipmitool_err_msg"

#define BETA_TEMP_J2_ENV1 "Temp_J2_ENV1"
#define BETA_TEMP_J2_ENV2 "Temp_J2_ENV2"
#define BETA_TEMP_J2_DIE1 "Temp_J2_DIE1"
#define BETA_TEMP_J2_DIE2 "Temp_J2_DIE2"
#define PVT_TEMP_J2_ENV1 "TEMP_J2_ENV1"
#define PVT_TEMP_J2_ENV2 "TEMP_J2_ENV2"
#define PVT_TEMP_J2_DIE1 "TEMP_J2_DIE1"
#define PVT_TEMP_J2_DIE2 "TEMP_J2_DIE2"

#define BETA_CMD_BMC_SENSOR_CACHE        "timeout %ds ipmitool sdr -c get "\
                                    "ADC_CPU_TEMP "\
                                    "TEMP_CPU_PECI "\
                                    BETA_TEMP_J2_ENV1" "\
                                    BETA_TEMP_J2_DIE1" "\
                                    BETA_TEMP_J2_ENV2 " "\
                                    BETA_TEMP_J2_DIE2" "\
                                    "PSU0_TEMP "\
                                    "PSU1_TEMP "\
                                    "FAN0_RPM "\
                                    "FAN1_RPM "\
                                    "FAN2_RPM "\
                                    "FAN3_RPM "\
                                    "PSU0_FAN "\
                                    "PSU1_FAN "\
                                    "FAN0_PRSNT_H "\
                                    "FAN1_PRSNT_H "\
                                    "FAN2_PRSNT_H "\
                                    "FAN3_PRSNT_H "\
                                    "PSU0_VIN "\
                                    "PSU0_VOUT "\
                                    "PSU0_IIN "\
                                    "PSU0_IOUT "\
                                    "PSU0_STBVOUT "\
                                    "PSU0_STBIOUT "\
                                    "PSU1_VIN "\
                                    "PSU1_VOUT "\
                                    "PSU1_IIN "\
                                    "PSU1_IOUT "\
                                    "PSU1_STBVOUT "\
                                    "PSU1_STBIOUT "\
                                    "> " BMC_SENSOR_CACHE IPMITOOL_REDIRECT_ERR

#define PVT_CMD_BMC_SENSOR_CACHE        "timeout %ds ipmitool sdr -c get "\
                                    "ADC_CPU_TEMP "\
                                    "TEMP_CPU_PECI "\
                                    PVT_TEMP_J2_ENV1" "\
                                    PVT_TEMP_J2_DIE1" "\
                                    PVT_TEMP_J2_ENV2 " "\
                                    PVT_TEMP_J2_DIE2" "\
                                    "PSU0_TEMP "\
                                    "PSU1_TEMP "\
                                    "FAN0_RPM "\
                                    "FAN1_RPM "\
                                    "FAN2_RPM "\
                                    "FAN3_RPM "\
                                    "PSU0_FAN "\
                                    "PSU1_FAN "\
                                    "FAN0_PRSNT_H "\
                                    "FAN1_PRSNT_H "\
                                    "FAN2_PRSNT_H "\
                                    "FAN3_PRSNT_H "\
                                    "PSU0_VIN "\
                                    "PSU0_VOUT "\
                                    "PSU0_IIN "\
                                    "PSU0_IOUT "\
                                    "PSU0_STBVOUT "\
                                    "PSU0_STBIOUT "\
                                    "PSU1_VIN "\
                                    "PSU1_VOUT "\
                                    "PSU1_IIN "\
                                    "PSU1_IOUT "\
                                    "PSU1_STBVOUT "\
                                    "PSU1_STBIOUT "\
                                    "> " BMC_SENSOR_CACHE IPMITOOL_REDIRECT_ERR                                    
#define BMC_FRU_LINE_SIZE           256
#define BMC_FRU_ATTR_KEY_VALUE_SIZE ONLP_CONFIG_INFO_STR_MAX
#define BMC_FRU_ATTR_KEY_VALUE_LEN  (BMC_FRU_ATTR_KEY_VALUE_SIZE - 1)
#define BMC_FRU_KEY_MANUFACTURER    "Product Manufacturer"
#define BMC_FRU_KEY_NAME            "Product Name"
#define BMC_FRU_KEY_PART_NUMBER     "Product Part Number"
#define BMC_FRU_KEY_SERIAL          "Product Serial"

#define CMD_FRU_CACHE_SET "timeout %ds ipmitool fru print %d " \
                           IPMITOOL_REDIRECT_ERR \
                          " | grep %s" \
                          " | awk -F: '/:/{gsub(/^ /,\"\", $0);gsub(/ +:/,\":\",$0);gsub(/: +/,\":\", $0);print $0}'" \
                          " > %s"


/* SYS */
#define CPLD_MAX      3  //Number of MB CPLD
extern const int CPLD_BASE_ADDR[CPLD_MAX];
extern const int CPLD_I2C_BUS;

/* BMC CMD */
#define FAN_CACHE_TIME          10
#define PSU_CACHE_TIME          30
#define THERMAL_CACHE_TIME      10
#define IPMITOOL_CMD_TIMEOUT    10

/* PSU */
#define TMP_PSU_TYPE "/tmp/psu_type_%d"
#define CMD_CREATE_PSU_TYPE "touch " TMP_PSU_TYPE

enum sensor
{
    FAN_SENSOR = 0,
    PSU_SENSOR,
    THERMAL_SENSOR,
};

enum bmc_attr_id {
    BMC_ATTR_ID_ADC_CPU_TEMP,
    BMC_ATTR_ID_TEMP_CPU_PECI,
    BMC_ATTR_ID_TEMP_J2_ENV_1,
    BMC_ATTR_ID_TEMP_J2_DIE_1,
    BMC_ATTR_ID_TEMP_J2_ENV_2,
    BMC_ATTR_ID_TEMP_J2_DIE_2,
    BMC_ATTR_ID_PSU0_TEMP,
    BMC_ATTR_ID_PSU1_TEMP,
    BMC_ATTR_ID_FAN0_RPM,
    BMC_ATTR_ID_FAN1_RPM,
    BMC_ATTR_ID_FAN2_RPM,
    BMC_ATTR_ID_FAN3_RPM,
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

/* Thermal definitions*/
enum onlp_thermal_id {
    ONLP_THERMAL_RESERVED = 0,
    ONLP_THERMAL_CPU_PKG,
    ONLP_THERMAL_CPU_0,
    ONLP_THERMAL_CPU_1,
    ONLP_THERMAL_CPU_2,
    ONLP_THERMAL_CPU_3,
    ONLP_THERMAL_CPU_4,
    ONLP_THERMAL_CPU_5,
    ONLP_THERMAL_CPU_6,
    ONLP_THERMAL_CPU_7,
    ONLP_THERMAL_ADC_CPU_TEMP,
    ONLP_THERMAL_CPU_PECI,
    ONLP_THERMAL_J2_ENV_1,
    ONLP_THERMAL_J2_DIE_1,
    ONLP_THERMAL_J2_ENV_2,
    ONLP_THERMAL_J2_DIE_2,
    ONLP_THERMAL_PSU_0,
    ONLP_THERMAL_PSU_1,
    ONLP_THERMAL_MAX,
};

#define ONLP_THERMAL_COUNT ONLP_THERMAL_MAX /*include "reserved"*/


/* Fan definitions*/
enum onlp_fan_id {
    ONLP_FAN_RESERVED = 0,
    ONLP_FAN_0,
    ONLP_FAN_1,
    ONLP_FAN_2,
    ONLP_FAN_3,
    ONLP_SYS_FAN_MAX = ONLP_FAN_3,
    ONLP_PSU_0_FAN,
    ONLP_PSU_1_FAN,
    ONLP_FAN_MAX,
};

#define ONLP_FAN_COUNT ONLP_FAN_MAX /*include "reserved"*/


/* PSU definitions*/
enum onlp_psu_id {
    ONLP_PSU_RESERVED  = 0,
    ONLP_PSU_0,
    ONLP_PSU_1,
    ONLP_PSU_0_VIN,
    ONLP_PSU_0_VOUT,
    ONLP_PSU_0_IIN,
    ONLP_PSU_0_IOUT,
    ONLP_PSU_0_STBVOUT,
    ONLP_PSU_0_STBIOUT,
    ONLP_PSU_1_VIN,
    ONLP_PSU_1_VOUT,
    ONLP_PSU_1_IIN,
    ONLP_PSU_1_IOUT,
    ONLP_PSU_1_STBVOUT,
    ONLP_PSU_1_STBIOUT,
    ONLP_PSU_MAX,
};


/* LED definitions*/
enum onlp_led_id {
    ONLP_LED_RESERVED  = 0,
    ONLP_LED_SYS_SYNC,
    ONLP_LED_SYS_SYS,
    ONLP_LED_SYS_FAN,
    ONLP_LED_SYS_PSU_0,
    ONLP_LED_SYS_PSU_1,
    ONLP_LED_SYS_ID,
    ONLP_LED_MAX
};

#define ONLP_LED_COUNT ONLP_LED_MAX /*include "reserved"*/

typedef struct bmc_info_s
{
    char name[20];
    float data;
}bmc_info_t;

typedef struct bmc_fru_attr_s
{
    char key[BMC_FRU_ATTR_KEY_VALUE_SIZE];
    char val[BMC_FRU_ATTR_KEY_VALUE_SIZE];
}bmc_fru_attr_t;

typedef struct bmc_fru_s
{
    int bmc_fru_id;
    char init_done;
    char cache_files[BMC_FRU_ATTR_KEY_VALUE_SIZE];
    bmc_fru_attr_t vendor;
    bmc_fru_attr_t name;
    bmc_fru_attr_t part_num;
    bmc_fru_attr_t serial;
}bmc_fru_t;

typedef struct board_s
{
    int hw_id;
    int deph_id;
    int build_id;
}board_t;

int check_file_exist(char *file_path, long *file_time);
int bmc_cache_expired_check(long last_time, long new_time, int cache_time);
int bmc_sensor_read(int bmc_cache_index, int sensor_type, float *data);
int read_ioport(int addr, int *reg_val);
int exec_cmd(char *cmd, char* out, int size);
int parse_bmc_sdr_cmd(char *cmd_out, int cmd_out_size, char *tokens[], int token_size, const char *sensor_id_str, int *idx);
int file_read_hex(int* value, const char* fmt, ...);
int file_vread_hex(int* value, const char* fmt, va_list vargs);
int get_psu_present_status(int local_id, int *psu_present);
int get_psu_type(int local_id, int *psu_type, bmc_fru_t *fru_in);

void lock_init();

int bmc_check_alive(void);
int bmc_sensor_read(int bmc_cache_index, int sensor_type, float *data);
int bmc_fru_read(int local_id, bmc_fru_t *data);

void check_and_do_i2c_mux_reset(int port);

uint8_t ufi_shift(uint8_t mask);

uint8_t ufi_mask_shift(uint8_t val, uint8_t mask);

uint8_t ufi_bit_operation(uint8_t reg_val, uint8_t bit, uint8_t bit_val);

int ufi_get_board_version(board_t *board);

#endif  /* __PLATFORM_LIB_H__ */

