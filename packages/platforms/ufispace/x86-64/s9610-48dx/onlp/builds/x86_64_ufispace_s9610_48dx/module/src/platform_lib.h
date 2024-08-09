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
#include "x86_64_ufispace_s9610_48dx_log.h"

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

#define COMM_STR_NOT_SUPPORTED "not supported"
#define COMM_STR_NOT_AVAILABLE "not available"

#define SYS_FMT                     "/sys/bus/i2c/devices/%d-%04x/%s"
#define SYSFS_DEVICES               "/sys/bus/i2c/devices/"
#define SYSFS_CPLD1                 SYSFS_DEVICES "1-0030/"
#define SYSFS_CPLD1_ID              SYSFS_CPLD1 "cpld_id"
#define SYSFS_HBM_PWR_CTRL          SYSFS_CPLD1 "cpld_hbm_pwr_ctrl"
#define SYSFS_HBM_PWR_FUNC          SYSFS_CPLD1 "cpld_hbm_pwr_func"
#define SYS_FMT_OFFSET              "/sys/bus/i2c/devices/%d-%04x/%s_%d"
#define SYS_CPU_CORETEMP_PREFIX     "/sys/devices/platform/coretemp.0/hwmon/hwmon0/"
#define SYS_CPU_CORETEMP_PREFIX2    "/sys/devices/platform/coretemp.0/"
#define LPC_BSP_FMT                 "/sys/devices/platform/x86_64_ufispace_s9610_48dx_lpc/bsp/"

#define BMC_SENSOR_CACHE            "/tmp/bmc_sensor_cache"
#define IPMITOOL_REDIRECT_FIRST_ERR " 2>/tmp/ipmitool_err_msg"
#define IPMITOOL_REDIRECT_ERR       " 2>>/tmp/ipmitool_err_msg"
#define OUTPUT_REDIRECT_ERR         " 2>>"LPC_BSP_FMT"bsp_pr_err"
#define OUTPUT_REDIRECT_INFO         " 1>>"LPC_BSP_FMT"bsp_pr_info"
//[BMC] 2.23
#define CMD_BMC_SENSOR_CACHE        "timeout %ds ipmitool sdr -c get "\
                                    "TEMP_ENV_CPU "\
                                    "TEMP_CPU_PECI "\
                                    "TEMP_ENV0 "\
                                    "TEMP_ENV1 "\
                                    "TEMP_ENV2 "\
                                    "TEMP_ENV3 "\
                                    "TEMP_ENV4 "\
                                    "TEMP_ENV5 "\
                                    "TEMP_ENV_FAN0 "\
                                    "TEMP_ENV_FAN1 "\
                                    "PSU0_TEMP1 "\
                                    "PSU1_TEMP1 "\
                                    "FAN0_RPM_FRONT "\
                                    "FAN0_RPM_REAR "\
                                    "FAN1_RPM_FRONT "\
                                    "FAN1_RPM_REAR "\
                                    "FAN2_RPM_FRONT "\
                                    "FAN2_RPM_REAR "\
                                    "FAN3_RPM_FRONT "\
                                    "FAN3_RPM_REAR "\
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
                                    "PSU0_PIN "\
                                    "PSU0_POUT "\
                                    "PSU1_VIN "\
                                    "PSU1_VOUT "\
                                    "PSU1_IIN "\
                                    "PSU1_IOUT "\
                                    "PSU1_PIN "\
                                    "PSU1_POUT "\
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

#define CMD_I2C_STUCK_CHECK     "i2cget -f -y 0 0x71 > /dev/null 2>&1"
#define MUX_RESET_PATH          "/sys/devices/platform/x86_64_ufispace_s9610_48dx_lpc/mb_cpld/mux_reset"

/* SYS */
#define CPLD_MAX      4
extern const int CPLD_BASE_ADDR[CPLD_MAX];
extern const int CPLD_I2C_BUS[CPLD_MAX];

/* BMC CMD */
#define FAN_CACHE_TIME          10
#define PSU_CACHE_TIME          30
#define THERMAL_CACHE_TIME      10
#define IPMITOOL_CMD_TIMEOUT    10

/* PSU */
#define TMP_PSU_TYPE "/tmp/psu_type_%d"
#define CMD_CREATE_PSU_TYPE "touch " TMP_PSU_TYPE

/* Warm Reset */
#define WARM_RESET_PATH          "/lib/platform-config/current/onl/warm_reset/warm_reset"
#define WARM_RESET_TIMEOUT       60
#define CMD_WARM_RESET           "timeout %ds "WARM_RESET_PATH " %s" OUTPUT_REDIRECT_ERR OUTPUT_REDIRECT_INFO
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

enum sensor {
    FAN_SENSOR = 0,
    PSU_SENSOR,
    THERMAL_SENSOR,
};

enum bmc_attr_id {
    BMC_ATTR_ID_TEMP_ENV_CPU,
    BMC_ATTR_ID_TEMP_CPU_PECI,
    BMC_ATTR_ID_TEMP_ENV0,
    BMC_ATTR_ID_TEMP_ENV1,
    BMC_ATTR_ID_TEMP_ENV2,
    BMC_ATTR_ID_TEMP_ENV3,
    BMC_ATTR_ID_TEMP_ENV4,
    BMC_ATTR_ID_TEMP_ENV5,
    BMC_ATTR_ID_TEMP_ENV_FAN0,
    BMC_ATTR_ID_TEMP_ENV_FAN1,
    BMC_ATTR_ID_PSU0_TEMP1,
    BMC_ATTR_ID_PSU1_TEMP1,
    BMC_ATTR_ID_FAN0_RPM_FRONT,
    BMC_ATTR_ID_FAN0_RPM_REAR,
    BMC_ATTR_ID_FAN1_RPM_FRONT,
    BMC_ATTR_ID_FAN1_RPM_REAR,
    BMC_ATTR_ID_FAN2_RPM_FRONT,
    BMC_ATTR_ID_FAN2_RPM_REAR,
    BMC_ATTR_ID_FAN3_RPM_FRONT,
    BMC_ATTR_ID_FAN3_RPM_REAR,
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
    BMC_ATTR_ID_PSU0_PIN,
    BMC_ATTR_ID_PSU0_POUT,
    BMC_ATTR_ID_PSU1_VIN,
    BMC_ATTR_ID_PSU1_VOUT,
    BMC_ATTR_ID_PSU1_IIN,
    BMC_ATTR_ID_PSU1_IOUT,
    BMC_ATTR_ID_PSU1_PIN,
    BMC_ATTR_ID_PSU1_POUT,
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
    ONLP_LED_RESERVED  = 0,
    ONLP_LED_SYS_SYNC,
    ONLP_LED_SYS_SYS,
    ONLP_LED_SYS_FAN,
    ONLP_LED_SYS_PSU_0,
    ONLP_LED_SYS_PSU_1,
    ONLP_LED_SYS_ID,
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
    ONLP_PSU_1_VIN  = 7,
    ONLP_PSU_1_VOUT = 8,
    ONLP_PSU_1_IIN  = 9,
    ONLP_PSU_1_IOUT = 10,
    ONLP_PSU_MAX = 11,
};

/* thermal_id */
enum onlp_thermal_id {
    ONLP_THERMAL_RESERVED = 0,
    ONLP_THERMAL_CPU_PKG = 1,
    ONLP_THERMAL_ENV_CPU = 2,
    ONLP_THERMAL_CPU_PECI = 3,
    ONLP_THERMAL_ENV0 = 4,
    ONLP_THERMAL_ENV1 = 5,
    ONLP_THERMAL_ENV2 = 6,
    ONLP_THERMAL_ENV3 = 7,
    ONLP_THERMAL_ENV4 = 8,
    ONLP_THERMAL_ENV5 = 9,
    ONLP_THERMAL_ENV_FAN0 = 10,
    ONLP_THERMAL_ENV_FAN1 = 11,
    ONLP_THERMAL_PSU0_TEMP1 = 12,
    ONLP_THERMAL_PSU1_TEMP1 = 13,
    ONLP_THERMAL_MAX = 14,
};

enum onlp_psu_type_e {
  ONLP_PSU_TYPE_AC,
  ONLP_PSU_TYPE_DC12,
  ONLP_PSU_TYPE_DC48,
  ONLP_PSU_TYPE_LAST = ONLP_PSU_TYPE_DC48,
  ONLP_PSU_TYPE_COUNT,
  ONLP_PSU_TYPE_INVALID = -1
};

enum onlp_fru_type_e {
  ONLP_FRU_PSU = 1,
  ONLP_FRU_FAN,
  ONLP_FRU_TYPE_INVALID = -1
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

typedef struct warm_reset_data_s {
    int unit_max;
    const char *warm_reset_dev_str;
    const char **unit_str;
} warm_reset_data_t;


int read_ioport(int addr, int *reg_val);

int exec_cmd(char *cmd, char* out, int size);

int file_read_hex(int* value, const char* fmt, ...);

int file_vread_hex(int* value, const char* fmt, va_list vargs);

int get_psu_type(int local_id, int *psu_type, bmc_fru_t *fru_in);
void lock_init();

int bmc_check_alive(void);
int bmc_sensor_read(int bmc_cache_index, int sensor_type, float *data);
int bmc_fru_read(int local_id, bmc_fru_t *data, int type);
void check_and_do_i2c_mux_reset(int port);

uint8_t ufi_shift(uint8_t mask);

uint8_t ufi_mask_shift(uint8_t val, uint8_t mask);

uint8_t ufi_bit_operation(uint8_t reg_val, uint8_t bit, uint8_t bit_val);

int onlp_data_path_reset(uint8_t unit_id, uint8_t reset_dev);

#endif  /* __PLATFORM_LIB_H__ */
