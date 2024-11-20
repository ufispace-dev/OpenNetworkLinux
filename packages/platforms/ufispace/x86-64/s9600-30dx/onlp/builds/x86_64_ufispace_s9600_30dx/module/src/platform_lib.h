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

#define ONLP_TRY(_expr)                                                 \
    do {                                                                \
        int _rv = (_expr);                                              \
        if(ONLP_FAILURE(_rv)) {                                         \
            AIM_LOG_ERROR("%s returned %{onlp_status}", #_expr, _rv);   \
            return _rv;                                                 \
        }                                                               \
    } while(0)

#define POID_0 0
#define I2C_BUS(_bus) (_bus)

#define SYSFS_PLTM                  "/sys/devices/platform/"
#define SYSFS_DEVICES               "/sys/bus/i2c/devices/"

#define SYSFS_LPC                   SYSFS_PLTM "x86_64_ufispace_s9600_30dx_lpc/"
#define SYSFS_LPC_MB_CPLD           SYSFS_LPC "mb_cpld/"
#define SYSFS_HW_ID                 SYSFS_LPC_MB_CPLD "board_hw_id"
#define SYSFS_DEPH_ID               SYSFS_LPC_MB_CPLD "board_deph_id"
#define SYSFS_BUILD_ID              SYSFS_LPC_MB_CPLD "board_build_id"
#define SYSFS_MUX_RESET             SYSFS_LPC_MB_CPLD "mux_reset"
#define SYSFS_CPLD1                 SYSFS_DEVICES "1-0030/"
#define SYSFS_CPLD1_ID              SYSFS_CPLD1 "cpld_id"
#define SYSFS_HBM_PWR_CTRL          SYSFS_CPLD1 "cpld_hbm_pwr_ctrl"
#define SYS_FMT                     SYSFS_DEVICES "%d-%04x/%s"
#define SYS_FMT_OFFSET              SYSFS_DEVICES "%d-%04x/%s_%s"
#define SYS_CPU_CORETEMP_PREFIX     SYSFS_PLTM "coretemp.0/hwmon/hwmon0/"
#define SYS_CPU_CORETEMP_PREFIX2    SYSFS_PLTM "coretemp.0/"
#define SYS_LPC_BSP                 SYSFS_LPC"bsp/"

#define BMC_SENSOR_CACHE            "/tmp/bmc_sensor_cache"
#define IPMITOOL_REDIRECT_FIRST_ERR " 2>/tmp/ipmitool_err_msg"
#define IPMITOOL_REDIRECT_ERR       " 2>>/tmp/ipmitool_err_msg"
#define BSP_PR_REDIRECT_ERR         " 2>>"SYS_LPC_BSP"bsp_pr_err"
#define BSP_PR_REDIRECT_INFO        " 1>>"SYS_LPC_BSP"bsp_pr_info"

#define BETA_TEMP_ENV_CPU "ADC_CPU_TEMP"
#define BETA_TEMP_ENV_MAC0 "Temp_J2_ENV1"
#define BETA_TEMP_ENV_MAC1 "Temp_J2_ENV2"
#define BETA_TEMP_MAC0 "Temp_J2_DIE1"
#define BETA_TEMP_MAC1 "Temp_J2_DIE2"
#define BETA_PSU0_TEMP "PSU0_TEMP"
#define BETA_PSU1_TEMP "PSU1_TEMP"

#define PVT_TEMP_ENV_CPU "TEMP_ENV_CPU"
#define PVT_TEMP_ENV_MAC0 "TEMP_ENV_MAC0"
#define PVT_TEMP_ENV_MAC1 "TEMP_ENV_MAC1"
#define PVT_TEMP_MAC0 "TEMP_MAC0"
#define PVT_TEMP_MAC1 "TEMP_MAC1"
#define PVT_PSU0_TEMP "PSU0_TEMP1"
#define PVT_PSU1_TEMP "PSU1_TEMP1"

#define BETA_CMD_BMC_SENSOR_CACHE   "timeout %ds ipmitool sdr -c get "\
                                    BETA_TEMP_ENV_CPU" "\
                                    "TEMP_CPU_PECI "\
                                    BETA_TEMP_ENV_MAC0" "\
                                    BETA_TEMP_MAC0" "\
                                    BETA_TEMP_ENV_MAC1 " "\
                                    BETA_TEMP_MAC1" "\
                                    BETA_PSU0_TEMP" "\
                                    BETA_PSU1_TEMP" "\
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

//[BMC] 2.19
#define PVT_CMD_BMC_SENSOR_CACHE    "timeout %ds ipmitool sdr -c get "\
                                    PVT_TEMP_ENV_CPU" "\
                                    "TEMP_CPU_PECI "\
                                    PVT_TEMP_ENV_MAC0" "\
                                    PVT_TEMP_MAC0" "\
                                    PVT_TEMP_ENV_MAC1 " "\
                                    PVT_TEMP_MAC1" "\
                                    PVT_PSU0_TEMP" "\
                                    PVT_PSU1_TEMP" "\
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

enum cpld_id_e {
  CPLD_1,
  CPLD_2,
  CPLD_3,
  CPLD_MAX
};

/* SYS */
extern const int CPLD_BASE_ADDR[CPLD_MAX];
extern const int CPLD_I2C_BUS;

/* BMC CMD */
#define FAN_CACHE_TIME          60
#define PSU_CACHE_TIME          60
#define THERMAL_CACHE_TIME      60
#define IPMITOOL_CMD_TIMEOUT    30

/* PSU */
#define TMP_PSU_TYPE "/tmp/psu_type_%d"
#define CMD_CREATE_PSU_TYPE "touch " TMP_PSU_TYPE

/* I2C Get/Set command*/
#define CMD_I2C_GET "i2cget -f -y %d %d %d"
#define CMD_I2C_SET "i2cset -f -y %d %d %d %d"

#define COMM_STR_NOT_SUPPORTED              "not supported"
#define COMM_STR_NOT_AVAILABLE              "not available"

/* Warm Reset */
#define WARM_RESET_PATH          "/lib/platform-config/current/onl/warm_reset/warm_reset"
#define WARM_RESET_TIMEOUT       60
#define CMD_WARM_RESET           "timeout %ds "WARM_RESET_PATH " %s" BSP_PR_REDIRECT_ERR BSP_PR_REDIRECT_INFO
enum reset_dev_type {
    WARM_RESET_ALL = 0,
    WARM_RESET_MAC,
    WARM_RESET_PHY,
    WARM_RESET_MUX,
    WARM_RESET_OP2,
    WARM_RESET_GB,
    WARM_RESET_MAX
};

enum mac_unit_id {
     MAC_ALL = 0,
     MAC1_ID,
     MAC_MAX
};
enum sensor
{
    FAN_SENSOR = 0,
    PSU_SENSOR,
    THERMAL_SENSOR,
};

enum bmc_attr_id {
    BMC_ATTR_ID_TEMP_ENV_CPU,
    BMC_ATTR_ID_TEMP_CPU_PECI,
    BMC_ATTR_ID_TEMP_ENV_MAC0,
    BMC_ATTR_ID_TEMP_MAC0,
    BMC_ATTR_ID_TEMP_ENV_MAC1,
    BMC_ATTR_ID_TEMP_MAC1,
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
    ONLP_THERMAL_ENV_CPU,
    ONLP_THERMAL_CPU_PECI,
    ONLP_THERMAL_ENV_MAC0,
    ONLP_THERMAL_MAC0,
    ONLP_THERMAL_ENV_MAC1,
    ONLP_THERMAL_MAC1,
    ONLP_THERMAL_PSU_0,
    ONLP_THERMAL_PSU_1,
    ONLP_THERMAL_MAX,
};

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

enum onlp_psu_type_e {
  ONLP_PSU_TYPE_AC,
  ONLP_PSU_TYPE_DC12,
  ONLP_PSU_TYPE_DC48,
  ONLP_PSU_TYPE_LAST = ONLP_PSU_TYPE_DC48,
  ONLP_PSU_TYPE_COUNT,
  ONLP_PSU_TYPE_INVALID = -1
};

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

typedef struct warm_reset_data_s {
    int unit_max;
    const char *warm_reset_dev_str;
    const char **unit_str;
} warm_reset_data_t;

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
int ufi_read_cpld_reg(int cpld_id, uint8_t reg, uint8_t *reg_val);
int ufi_write_cpld_reg(int cpld_id, uint8_t reg, uint8_t reg_val);
int ufi_get_cpu_rev(int *rev_sku, int *rev_hw, int *rev_build);
int ufi_get_cpu_hw_rev_id(int *rev_id, int *sku_id, int *build_id);
int onlp_data_path_reset(uint8_t unit_id, uint8_t reset_dev);
#endif  /* __PLATFORM_LIB_H__ */

