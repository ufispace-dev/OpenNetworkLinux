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
#include "x86_64_ufispace_s9600_102xc_log.h"

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
#define SYS_FMT_OFFSET              "/sys/bus/i2c/devices/%d-%04x/%s_%d"
#define SYS_GPIO_FMT                "/sys/class/gpio/gpio%d/value"
#define LPC_FMT                     "/sys/devices/platform/x86_64_ufispace_s9600_102xc_lpc/mb_cpld/"
#define LPC_BSP_FMT                 "/sys/devices/platform/x86_64_ufispace_s9600_102xc_lpc/bsp/"
#define LPC_BSP_CPU_CPLD            "/sys/devices/platform/x86_64_ufispace_s9600_102xc_lpc/cpu_cpld/"
#define SYS_CPU_CORETEMP_PREFIX     "/sys/devices/platform/coretemp.0/hwmon/hwmon0/"
#define SYS_CPU_CORETEMP_PREFIX2    "/sys/devices/platform/coretemp.0/"
#define MUX_RESET_PATH              "/sys/devices/platform/x86_64_ufispace_s9600_102xc_lpc/mb_cpld/mux_reset_all"
#define SYSFS_DEVICES               "/sys/bus/i2c/devices/"
#define SYSFS_CPLD1                 SYSFS_DEVICES "1-0030/"
#define SYSFS_CPLD2                 SYSFS_DEVICES "1-0031/"
#define SYSFS_CPLD3                 SYSFS_DEVICES "1-0032/"
#define SYSFS_CPLD4                 SYSFS_DEVICES "1-0033/"
#define SYSFS_CPLD5                 SYSFS_DEVICES "1-0034/"
#define I2C_STUCK_CHECK_SYSFS       SYSFS_CPLD1 "cpld_id"
#define BSP_PR_REDIRECT_ERR         " 2>>"LPC_BSP_FMT"bsp_pr_err"
#define BSP_PR_REDIRECT_INFO        " 1>>"LPC_BSP_FMT"bsp_pr_info"

/* Thermal threshold */
#define THERMAL_WARNING_DEFAULT               77
#define THERMAL_ERROR_DEFAULT                 95
#define THERMAL_SHUTDOWN_DEFAULT              105
#define THERMAL_TEMP_CPU_PECI_WARNING         85
#define THERMAL_TEMP_CPU_PECI_ERROR           95
#define THERMAL_TEMP_CPU_PECI_SHUTDOWN        100
#define THERMAL_TEMP_ENV_CPU_WARNING          85
#define THERMAL_TEMP_ENV_CPU_ERROR            95
#define THERMAL_TEMP_ENV_CPU_SHUTDOWN         100
#define THERMAL_TEMP_ENV0_WARNING             60
#define THERMAL_TEMP_ENV0_ERROR               65
#define THERMAL_TEMP_ENV0_SHUTDOWN            70
#define THERMAL_TEMP_ENV1_WARNING             60
#define THERMAL_TEMP_ENV1_ERROR               65
#define THERMAL_TEMP_ENV1_SHUTDOWN            70
#define THERMAL_TEMP_ENV_MAC0_WARNING         65
#define THERMAL_TEMP_ENV_MAC0_ERROR           70
#define THERMAL_TEMP_ENV_MAC0_SHUTDOWN        80
#define THERMAL_TEMP_MAC0_WARNING             100
#define THERMAL_TEMP_MAC0_ERROR               105
#define THERMAL_TEMP_MAC0_SHUTDOWN            110
#define THERMAL_TEMP_PSU0_WARNING             65
#define THERMAL_TEMP_PSU0_ERROR               70
#define THERMAL_TEMP_PSU0_SHUTDOWN            75
#define THERMAL_TEMP_PSU1_WARNING             65
#define THERMAL_TEMP_PSU1_ERROR               70
#define THERMAL_TEMP_PSU1_SHUTDOWN            75
#define THERMAL_CPU_WARNING                   THERMAL_WARNING_DEFAULT
#define THERMAL_CPU_ERROR                     THERMAL_ERROR_DEFAULT
#define THERMAL_CPU_SHUTDOWN                  THERMAL_SHUTDOWN_DEFAULT
#define THERMAL_STATE_NOT_SUPPORT             0

#define PSU_STATUS_ABS              0
#define PSU_STATUS_PRES             1


/* CPU core-temp sysfs ID */
#define CPU_PKG_CORE_TEMP_SYS_ID  "1"

/* BMC attr */
//[BMC] 2.25
#define BMC_ATTR_NAME_TEMP_CPU_PECI         "TEMP_CPU_PECI"
#define BMC_ATTR_NAME_TEMP_ENV_CPU          "TEMP_ENV_CPU"
#define BMC_ATTR_NAME_TEMP_ENV0             "TEMP_ENV0"
#define BMC_ATTR_NAME_TEMP_ENV1             "TEMP_ENV1"
#define BMC_ATTR_NAME_TEMP_ENV_MAC0         "TEMP_ENV_MAC0"
#define BMC_ATTR_NAME_TEMP_MAC0             "TEMP_MAC0"
#define BMC_ATTR_NAME_PSU0_TEMP             "PSU0_TEMP1"
#define BMC_ATTR_NAME_PSU1_TEMP             "PSU1_TEMP1"
#define BMC_ATTR_NAME_FAN0_RPM              "FAN0_RPM"
#define BMC_ATTR_NAME_FAN1_RPM              "FAN1_RPM"
#define BMC_ATTR_NAME_FAN2_RPM              "FAN2_RPM"
#define BMC_ATTR_NAME_FAN3_RPM              "FAN3_RPM"
#define BMC_ATTR_NAME_PSU0_FAN1             "PSU0_FAN"
#define BMC_ATTR_NAME_PSU1_FAN1             "PSU1_FAN"
#define BMC_ATTR_NAME_FAN0_PSNT_L           "FAN0_PRSNT_H"
#define BMC_ATTR_NAME_FAN1_PSNT_L           "FAN1_PRSNT_H"
#define BMC_ATTR_NAME_FAN2_PSNT_L           "FAN2_PRSNT_H"
#define BMC_ATTR_NAME_FAN3_PSNT_L           "FAN3_PRSNT_H"
#define BMC_ATTR_NAME_PSU0_VIN              "PSU0_VIN"
#define BMC_ATTR_NAME_PSU0_VOUT             "PSU0_VOUT"
#define BMC_ATTR_NAME_PSU0_IIN              "PSU0_IIN"
#define BMC_ATTR_NAME_PSU0_IOUT             "PSU0_IOUT"
#define BMC_ATTR_NAME_PSU0_PIN              "PSU0_PIN"
#define BMC_ATTR_NAME_PSU0_POUT             "PSU0_POUT"
#define BMC_ATTR_NAME_PSU1_VIN              "PSU1_VIN"
#define BMC_ATTR_NAME_PSU1_VOUT             "PSU1_VOUT"
#define BMC_ATTR_NAME_PSU1_IIN              "PSU1_IIN"
#define BMC_ATTR_NAME_PSU1_IOUT             "PSU1_IOUT"
#define BMC_ATTR_NAME_PSU1_PIN              "PSU1_PIN"
#define BMC_ATTR_NAME_PSU1_POUT             "PSU1_POUT"

/* BMC cmd */
#define BMC_SENSOR_CACHE            "/tmp/bmc_sensor_cache"
#define BMC_OEM_CACHE               "/tmp/bmc_oem_cache"
#define IPMITOOL_REDIRECT_ERR       " 2>>"LPC_BSP_FMT"bsp_pr_err"
#define FAN_CACHE_TIME          10
#define PSU_CACHE_TIME          30
#define THERMAL_CACHE_TIME      10
#define BMC_FIELDS_MAX          20
#define FANDIR_CACHE_TIME       60

/*   IPMITOOL_CMD_TIMEOUT get from ipmitool test.
 *   Test Case: Run 100 times of CMD_BMC_SENSOR_CACHE command and 100 times of CMD_FRU_CACHE_SET command and get the execution times.
 *              We take 10s as The IPMITOOL_CMD_TIMEOUT value
 *              since the CMD_BMC_SENSOR_CACHE execution times value is between 0.216s - 2.926s and
 *                    the CMD_FRU_CACHE_SET execution times value is between 0.031s - 0.076s.
 */

#define IPMITOOL_CMD_TIMEOUT        10
#define CMD_BMC_SENSOR_CACHE        "timeout %ds ipmitool sdr -c get %s"\
                                    "> "BMC_SENSOR_CACHE IPMITOOL_REDIRECT_ERR

#define CMD_BMC_FAN_TRAY_DIR        "timeout %ds ipmitool raw 0x3c 0x31 0x0 | xargs"IPMITOOL_REDIRECT_ERR
#define CMD_BMC_PSU_FAN_DIR         "timeout %ds ipmitool raw 0x3c 0x30 0x0 | xargs"IPMITOOL_REDIRECT_ERR
#define CMD_BMC_OEM_CACHE           CMD_BMC_FAN_TRAY_DIR" > "BMC_OEM_CACHE";"CMD_BMC_PSU_FAN_DIR" >> "BMC_OEM_CACHE
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

#define BMC_FAN_DIR_UNK   0
#define BMC_FAN_DIR_B2F   1
#define BMC_FAN_DIR_F2B   2
#define BMC_ATTR_STATUS_ABS         0
#define BMC_ATTR_STATUS_PRES        1
#define BMC_ATTR_INVALID_VAL        999999

enum sensor
{
    FAN_SENSOR = 0,
    PSU_SENSOR,
    THERMAL_SENSOR,
};

enum bmc_attr_id {
    BMC_ATTR_ID_START = 0,
    BMC_ATTR_ID_TEMP_CPU_PECI = BMC_ATTR_ID_START,
    BMC_ATTR_ID_TEMP_ENV_CPU,
    BMC_ATTR_ID_TEMP_ENV0,
    BMC_ATTR_ID_TEMP_ENV1,
    BMC_ATTR_ID_TEMP_ENV_MAC0,
    BMC_ATTR_ID_TEMP_MAC0,
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
    BMC_ATTR_ID_PSU0_PIN,
    BMC_ATTR_ID_PSU0_POUT,
    BMC_ATTR_ID_PSU1_VIN,
    BMC_ATTR_ID_PSU1_VOUT,
    BMC_ATTR_ID_PSU1_IIN,
    BMC_ATTR_ID_PSU1_IOUT,
    BMC_ATTR_ID_PSU1_PIN,
    BMC_ATTR_ID_PSU1_POUT,
    BMC_ATTR_ID_LAST = BMC_ATTR_ID_PSU1_POUT,
    BMC_ATTR_ID_INVALID,
};

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
    WARM_RESET_I210,
    WARM_RESET_MAX
};

enum mac_unit_id {
     MAC_ALL = 0,
     MAC1_ID,
     MAC_MAX
};

/* oem_id */
enum bmc_oem_id {
    BMC_OEM_IDX_INVALID,
};

/* fru_id */
enum bmc_fru_id {
    BMC_FRU_IDX_ONLP_PSU_0,
    BMC_FRU_IDX_ONLP_PSU_1,

    BMC_FRU_IDX_INVALID = -1,
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
    ONLP_LED_SYS_SYNC = 1,
    ONLP_LED_SYS_SYS,
    ONLP_LED_SYS_FAN,
    ONLP_LED_SYS_PSU0,
    ONLP_LED_SYS_PSU1,
    ONLP_LED_SYS_ID,
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
    ONLP_THERMAL_ENV_CPU,
    ONLP_THERMAL_ENV0,
    ONLP_THERMAL_ENV1,
    ONLP_THERMAL_ENV_MAC0,
    ONLP_THERMAL_MAC0,
    ONLP_THERMAL_PSU0,
    ONLP_THERMAL_PSU1,
    ONLP_THERMAL_CPU_PKG,
    ONLP_THERMAL_MAX,
};

/** onlp_psu_type */
typedef enum onlp_psu_type_e {
    ONLP_PSU_TYPE_AC,
    ONLP_PSU_TYPE_DC12,
    ONLP_PSU_TYPE_DC48,
    ONLP_PSU_TYPE_LAST = ONLP_PSU_TYPE_DC48,
    ONLP_PSU_TYPE_COUNT,
    ONLP_PSU_TYPE_INVALID = -1,
} onlp_psu_type_t;

typedef enum bmc_data_type_e {
    BMC_DATA_BOOL,
    BMC_DATA_FLOAT,
} bmc_data_type_t;

typedef enum brd_rev_id_e {
    BRD_PROTO,
    BRD_ALPHA,
    BRD_BETA,
    BRD_PVT,
} brd_rev_id_t;

enum hw_plat
{
    HW_PLAT_NONE      = 0x0,
    HW_PLAT_PROTO     = 0x1,
    HW_PLAT_ALPHA     = 0x2,
    HW_PLAT_BETA      = 0x4,
    HW_PLAT_PVT       = 0x8,
    HW_PLAT_ALL       = 0xf,
};

typedef struct bmc_info_s
{
    int plat;
    char name[20];
    int data_type;
    float data;
} bmc_info_t;

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

typedef struct bmc_oem_data_s {
    char init_done;
    int raw_idx;
    int col_idx;
    int data;
} bmc_oem_data_t;

typedef struct psu_sup_info_s {
    char vendor[BMC_FRU_ATTR_KEY_VALUE_SIZE];
    char part_number[BMC_FRU_ATTR_KEY_VALUE_SIZE];
    int type;
} psu_support_info_t;

typedef struct board_s
{
    int sku_id;
    int hw_build;
    int deph_id;
    int hw_rev;
    int ext_id;
    int id_type;
}board_t;

typedef struct temp_thld_s
{
    int warning;
    int error;
    int shutdown;
}temp_thld_t;

typedef struct warm_reset_data_s {
    int unit_max;
    const char *warm_reset_dev_str;
    const char **unit_str;
} warm_reset_data_t;

int exec_cmd(char *cmd, char* out, int size);

int read_file_hex(int* value, const char* fmt, ...);

int vread_file_hex(int* value, const char* fmt, va_list vargs);

void init_lock();

int check_bmc_alive(void);
int read_bmc_sensor(int bmc_cache_index, int sensor_type, float *data);
int read_bmc_fru(int fru_id, bmc_fru_t *data);
int read_bmc_oem(int bmc_oem_id, int *data);

void check_and_do_i2c_mux_reset(int port);

uint8_t shift_bit(uint8_t mask);

uint8_t shift_bit_mask(uint8_t val, uint8_t mask);

uint8_t operate_bit(uint8_t reg_val, uint8_t bit, uint8_t bit_val);

uint8_t get_bit_value(uint8_t reg_val, uint8_t bit);

int get_hw_rev_id(void);

int get_psu_present_status(int local_id, int *pw_present);
int get_psu_type(int local_id, int *psu_type, bmc_fru_t *fru_in);
int get_cpu_hw_rev_id(int *rev_id, int *dev_phase, int *build_id);
int get_board_version(board_t *board);
int get_thermal_thld(int thermal_local_id, temp_thld_t *temp_thld);
int get_gpio_max(int *gpio_max);
int onlp_data_path_reset(uint8_t unit_id, uint8_t reset_dev);
#endif  /* __PLATFORM_LIB_H__ */
