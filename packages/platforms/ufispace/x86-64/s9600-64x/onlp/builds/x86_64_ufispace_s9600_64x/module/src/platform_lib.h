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
 *
 *
 ***********************************************************/
#ifndef __PLATFORM_LIB_H__
#define __PLATFORM_LIB_H__

#include <onlp/fan.h>
#include <onlp/psu.h>
#include <onlp/platformi/thermali.h>
#include <onlp/platformi/ledi.h>
#include <onlp/platformi/sysi.h>
#include "x86_64_ufispace_s9600_64x_int.h"
#include "x86_64_ufispace_s9600_64x_log.h"

#include <x86_64_ufispace_s9600_64x/x86_64_ufispace_s9600_64x_config.h>
#define ONLP_TRY(_expr)                                                 \
    do {                                                                \
        int _rv = (_expr);                                              \
        if(ONLP_FAILURE(_rv)) {                                         \
            AIM_LOG_ERROR("%s returned %{onlp_status}", #_expr, _rv);   \
            return _rv;                                                 \
        }                                                               \
    } while(0)

#define COMM_STR_NOT_SUPPORTED "not supported"
#define COMM_STR_NOT_AVAILABLE "not available"

#define SYS_FMT                     "/sys/bus/i2c/devices/%d-%04x/%s"
#define SYS_FMT_OFFSET              "/sys/bus/i2c/devices/%d-%04x/%s_%d"
#define SYS_DEV                     "/sys/bus/i2c/devices/"
#define SYS_CPU_CORETEMP_PREFIX         "/sys/devices/platform/coretemp.0/hwmon/hwmon0/"
#define SYS_CPU_CORETEMP_PREFIX2      "/sys/devices/platform/coretemp.0/"
#define SYS_CORE_TEMP_PREFIX        "/sys/class/hwmon/hwmon2/"
#define SYS_CPU_BOARD_TEMP_PREFIX   "/sys/bus/i2c/devices/0-004f/hwmon/hwmon1/"
#define SYS_CPU_BOARD_TEMP_PREFIX2   "/sys/bus/i2c/devices/0-004f/"

/* LPC ATTR */
#define SYS_LPC                 "/sys/devices/platform/x86_64_ufispace_s9600_64x_lpc"
#define LPC_BSP_FMT             SYS_LPC"/bsp/"
#define LPC_MB_CPLD_PATH        SYS_LPC "/mb_cpld"
#define LPC_CPU_CPLD_PATH       SYS_LPC "/cpu_cpld"
#define LPC_CPU_CPLD_VER_ATTR   "cpu_cpld_version_h"

#define SYS_FAN_PREFIX              "/sys/class/hwmon/hwmon1/device/"
#define SYS_EEPROM_PATH             "/sys/bus/i2c/devices/0-0057/eeprom"
#define SYS_EEPROM_SIZE 512
#define PSU1_EEPROM_PATH            "/sys/bus/i2c/devices/58-0050/eeprom"
#define PSU2_EEPROM_PATH            "/sys/bus/i2c/devices/57-0050/eeprom"
#define BMC_EN_FILE_PATH            "/etc/onl/bmc_en"
#define BMC_SENSOR_CACHE            "/tmp/bmc_sensor_cache"
#define IPMITOOL_REDIRECT_FIRST_ERR " 2>/tmp/ipmitool_err_msg"
#define IPMITOOL_REDIRECT_ERR       " 2>>/tmp/ipmitool_err_msg"
#define OUTPUT_REDIRECT_ERR         " 2>>"LPC_BSP_FMT"bsp_pr_err"
#define OUTPUT_REDIRECT_INFO         " 1>>"LPC_BSP_FMT"bsp_pr_info"
#define CMD_BIOS_VER                "dmidecode -s bios-version | tail -1 | tr -d '\r\n'"
#define CMD_BMC_VER_1               "expr `ipmitool mc info"IPMITOOL_REDIRECT_FIRST_ERR" | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f1` + 0"
#define CMD_BMC_VER_2               "expr `ipmitool mc info"IPMITOOL_REDIRECT_ERR" | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f2` + 0"
#define CMD_BMC_VER_3               "echo $((`ipmitool mc info"IPMITOOL_REDIRECT_ERR" | grep 'Aux Firmware Rev Info' -A 2 | sed -n '2p'` + 0))"
//[BMC] 3.31
#define CMD_BMC_SENSOR_CACHE        "timeout %ds ipmitool sdr -c get ADC_CPU_TEMP TEMP_CPU_PECI TEMP_FRONT_ENV TEMP_OCXO TEMP_Q2CL_ENV TEMP_Q2CL_DIE TEMP_Q2CR_ENV TEMP_Q2CR_DIE TEMP_REAR_ENV_1 TEMP_REAR_ENV_2 PSU0_TEMP PSU1_TEMP FAN0_RPM FAN1_RPM FAN2_RPM FAN3_RPM PSU0_FAN PSU1_FAN FAN0_PRSNT_H FAN1_PRSNT_H FAN2_PRSNT_H FAN3_PRSNT_H PSU0_VIN PSU0_VOUT PSU0_IIN PSU0_IOUT PSU0_STBVOUT PSU0_STBIOUT PSU1_VIN PSU1_VOUT PSU1_IIN PSU1_IOUT PSU1_STBVOUT PSU1_STBIOUT > "BMC_SENSOR_CACHE IPMITOOL_REDIRECT_ERR
#define CMD_BMC_SDR_GET             "timeout %ds ipmitool sdr -c get %s"IPMITOOL_REDIRECT_ERR
#define CMD_FRU_INFO_GET            "timeout %ds ipmitool fru print %d"IPMITOOL_REDIRECT_ERR" | grep '%s' | cut -d':' -f2 | awk '{$1=$1};1' | tr -d '\n'"
#define CMD_BMC_CACHE_GET           "cat "BMC_SENSOR_CACHE" | grep %s | awk -F',' '{print $%d}'"
#define BMC_FRU_LINE_SIZE            256
#define BMC_FRU_ATTR_KEY_VALUE_SIZE  ONLP_CONFIG_INFO_STR_MAX
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

#define MB_CPLD1_ID_PATH            "/sys/bus/i2c/devices/1-0030/cpld_id"
#define MUX_RESET_PATH              "/sys/devices/platform/x86_64_ufispace_s9600_64x_lpc/mb_cpld/mux_reset"

/*   IPMITOOL_CMD_TIMEOUT get from ipmitool test.
 *   Test Case: Run 100 times of CMD_BMC_SENSOR_CACHE command and 100 times of CMD_FRU_CACHE_SET command and get the execution times.
 *              We take 10s as The IPMITOOL_CMD_TIMEOUT value
 *              since the CMD_BMC_SENSOR_CACHE execution times value is between 0.216s - 2.926s and
 *                    the CMD_FRU_CACHE_SET execution times value is between 0.031s - 0.076s.
 */

#define IPMITOOL_CMD_TIMEOUT        10

#define PSU_STATUS_PRESENT          1
#define PSU_STATUS_POWER_GOOD       1
#define FAN_PRESENT                 0
#define FAN_CTRL_SET1               1
#define FAN_CTRL_SET2               2
#define MAX_SYS_FAN_NUM             8
#define BOARD_THERMAL_NUM           6
#define SYS_FAN_NUM                 8
#define QSFP_NUM                    64
#define QSFPDD_NUM                  0
#define SFP_NUM                     4
#define PORT_NUM                    (QSFP_NUM + QSFPDD_NUM + SFP_NUM)
#define PORT_PER_CPLD               16

#define THERMAL_NUM                 21
#define LED_NUM                     4
#define FAN_NUM                     8



#define THERMAL_SHUTDOWN_DEFAULT    105000

#define THERMAL_ERROR_DEFAULT       95000
#define THERMAL_ERROR_FAN_PERC      100

#define THERMAL_WARNING_DEFAULT     77000
#define THERMAL_WARNING_FAN_PERC    80

#define THERMAL_NORMAL_DEFAULT      72000
#define THERMAL_NORMAL_FAN_PERC     50

/* I2C bus */
#define I2C_BUS_0               0
#define I2C_BUS_1               1
#define I2C_BUS_2               2
#define I2C_BUS_3               3
#define I2C_BUS_4               4
#define I2C_BUS_5               5
#define I2C_BUS_6               6
#define I2C_BUS_7               7
#define I2C_BUS_8               8
#define I2C_BUS_9               9
#define I2C_BUS_44              44      /* cpld */
#define I2C_BUS_50              50      /* SYS LED */
#define I2C_BUS_57              (57)      /* PSU2 */
#define I2C_BUS_58              (58)      /* PSU1 */
#define I2C_BUS_59              59      /* FRU  */

#define I2C_BUS_PSU1            I2C_BUS_58      /* PSU1 */
#define I2C_BUS_PSU2            I2C_BUS_57      /* PSU2 */

/* PSU */
#define PSU_MUX_MASK            0x01

#define PSU_THERMAL1_OFFSET     0x8D
#define PSU_THERMAL2_OFFSET     0x8E
#define PSU_THERMAL_REG         0x58
#define PSU_FAN_RPM_REG         0x58
#define PSU_FAN_RPM_OFFSET      0x90
#define PSU_REG                 0x58
#define PSU_VOUT_OFFSET1        0x20
#define PSU_VOUT_OFFSET2        0x8B
#define PSU_IOUT_OFFSET         0x8C
#define PSU_POUT_OFFSET         0x96
#define PSU_PIN_OFFSET          0x97

#define PSU_STATE_REG           0x15
#define PSU0_PRESENT_MASK       0x04
#define PSU1_PRESENT_MASK       0x08
#define PSU0_PWGOOD_MASK        0x01
#define PSU1_PWGOOD_MASK        0x02

/* LED */
#define LED_REG                 0x75
#define LED_OFFSET              0x02
#define LED_PWOK_OFFSET         0x03

#define LED_SYS_AND_MASK        0xFE
#define LED_SYS_GMASK           0x01
#define LED_SYS_YMASK           0x00

#define LED_FAN_AND_MASK        0xF9
#define LED_FAN_GMASK           0x02
#define LED_FAN_YMASK           0x06

#define LED_PSU2_AND_MASK       0xEF
#define LED_PSU2_GMASK          0x00
#define LED_PSU2_YMASK          0x10
#define LED_PSU2_ON_AND_MASK    0xFD
#define LED_PSU2_ON_OR_MASK     0x02
#define LED_PSU2_OFF_AND_MASK   0xFD
#define LED_PSU2_OFF_OR_MASK    0x00
#define LED_PSU1_AND_MASK       0xF7
#define LED_PSU1_GMASK          0x00
#define LED_PSU1_YMASK          0x08
#define LED_PSU1_ON_AND_MASK    0xFE
#define LED_PSU1_ON_OR_MASK     0x01
#define LED_PSU1_OFF_AND_MASK   0xFE
#define LED_PSU1_OFF_OR_MASK    0x00
#define LED_SYS_ON_MASK         0x00
#define LED_SYS_OFF_MASK        0x33

/* SYS */
#define CPLD_MAX                5
//#define CPLD_BASE_ADDR          0x30
#define CPLD_REG_VER            0x02
extern const int CPLD_BASE_ADDR[CPLD_MAX];

/* QSFP */
#define QSFP_PRES_REG1          0x20
#define QSFP_PRES_REG2          0x21
#define QSFP_PRES_OFFSET1       0x00
#define QSFP_PRES_OFFSET2       0x01

/* FAN */
#define FAN_GPIO_ADDR           0x20
#define FAN_1_2_PRESENT_MASK    0x40
#define FAN_3_4_PRESENT_MASK    0x04
#define FAN_5_6_PRESENT_MASK    0x40
#define FAN_7_8_PRESENT_MASK    0x04
#define SYS_FAN_RPM_MAX         16100
#define PSU_FAN_RPM_MAX_AC      27000
#define PSU_FAN_RPM_MAX_DC      28500

/* BMC CMD */
#define BMC_CACHE_EN            1
#define BMC_CACHE_CYCLE         30
#define BMC_CMD_SDR_SIZE        ONLP_CONFIG_INFO_STR_MAX
#define BMC_TOKEN_SIZE          20

#define FAN_CACHE_TIME          5
#define PSU_CACHE_TIME          5
#define THERMAL_CACHE_TIME      10

/* CPLD */
#define CPLD_REG_BASE           0x700
#define BRD_ID_REG              0x01

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
     MAC2_ID,
     MAC_MAX
};

enum sensor
{
    FAN_SENSOR = 0,
    PSU_SENSOR,
    THERMAL_SENSOR,
};

enum bmc_attr_id {
    BMC_ATTR_ID_ADC_CPU_TEMP,
    BMC_ATTR_ID_TEMP_CPU_PECI,
    BMC_ATTR_ID_TEMP_FRONT_ENV,
    BMC_ATTR_ID_TEMP_OCXO,
    BMC_ATTR_ID_TEMP_Q2CL_ENV,
    BMC_ATTR_ID_TEMP_Q2CL_DIE,
    BMC_ATTR_ID_TEMP_Q2CR_ENV,
    BMC_ATTR_ID_TEMP_Q2CR_DIE,
    BMC_ATTR_ID_TEMP_REAR_ENV_1,
    BMC_ATTR_ID_TEMP_REAR_ENV_2,
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
typedef struct bmc_info_s
{
    char name[20];
    float data;
}bmc_info_t;

/** led_oid */
typedef enum led_oid_e {
    LED_OID_SYNC = ONLP_LED_ID_CREATE(1),
    LED_OID_SYS = ONLP_LED_ID_CREATE(2),
    LED_OID_FAN = ONLP_LED_ID_CREATE(3),
    LED_OID_PSU0 = ONLP_LED_ID_CREATE(4),
    LED_OID_PSU1 = ONLP_LED_ID_CREATE(5),
    LED_OID_ID = ONLP_LED_ID_CREATE(6),
    LED_OID_FAN_TRAY1 = ONLP_LED_ID_CREATE(7),
    LED_OID_FAN_TRAY2 = ONLP_LED_ID_CREATE(8),
    LED_OID_FAN_TRAY3 = ONLP_LED_ID_CREATE(9),
    LED_OID_FAN_TRAY4 = ONLP_LED_ID_CREATE(10),
} led_oid_t;

/** led_id */
typedef enum led_id_e {
    LED_ID_SYS_SYNC = 1,
    LED_ID_SYS_SYS = 2,
    LED_ID_SYS_FAN = 3,
    LED_ID_SYS_PSU0 = 4,
    LED_ID_SYS_PSU1 = 5,
    LED_ID_SYS_ID = 6,
    LED_ID_FAN_TRAY1 = 7,
    LED_ID_FAN_TRAY2 = 8,
    LED_ID_FAN_TRAY3 = 9,
    LED_ID_FAN_TRAY4 = 10,
} led_id_t;

/** Thermal_oid */
typedef enum thermal_oid_e {
    THERMAL_OID_ADC_CPU = ONLP_THERMAL_ID_CREATE(1),
    THERMAL_OID_CPU_PECI = ONLP_THERMAL_ID_CREATE(2),
    THERMAL_OID_FRONT_ENV = ONLP_THERMAL_ID_CREATE(3),
    THERMAL_OID_OCXO = ONLP_THERMAL_ID_CREATE(4),
    THERMAL_OID_Q2CL_ENV = ONLP_THERMAL_ID_CREATE(5),//FIXME: order
    THERMAL_OID_Q2CL_DIE = ONLP_THERMAL_ID_CREATE(6),
	THERMAL_OID_Q2CR_ENV = ONLP_THERMAL_ID_CREATE(7),
    THERMAL_OID_Q2CR_DIE = ONLP_THERMAL_ID_CREATE(8),
    THERMAL_OID_REAR_ENV_1 = ONLP_THERMAL_ID_CREATE(9),
    THERMAL_OID_REAR_ENV_2 = ONLP_THERMAL_ID_CREATE(10),
    THERMAL_OID_PSU0 = ONLP_THERMAL_ID_CREATE(11),
    THERMAL_OID_PSU1 = ONLP_THERMAL_ID_CREATE(12),
    THERMAL_OID_CPU_PKG = ONLP_THERMAL_ID_CREATE(13),
} thermal_oid_t;

/* psu_id */
enum onlp_psu_id {
    ONLP_PSU_0 = 1,
    ONLP_PSU_1,
    ONLP_PSU_MAX,
};

/** thermal_id */
typedef enum thermal_id_e {
    THERMAL_ID_ADC_CPU = 1,
    THERMAL_ID_CPU_PECI = 2,
    THERMAL_ID_FRONT_ENV = 3,
    THERMAL_ID_OCXO = 4,
    THERMAL_ID_Q2CL_ENV = 5, //FIXME: order
    THERMAL_ID_Q2CL_DIE = 6,
	THERMAL_ID_Q2CR_ENV = 7,
    THERMAL_ID_Q2CR_DIE = 8,
    THERMAL_ID_REAR_ENV_1 = 9,
    THERMAL_ID_REAR_ENV_2 = 10,
    THERMAL_ID_PSU0 = 11,
    THERMAL_ID_PSU1 = 12,
    THERMAL_ID_CPU_PKG = 13,
    THERMAL_ID_MAX = 14,
} thermal_id_t;

/** onlp_psu_type */
typedef enum onlp_psu_type_e {
    ONLP_PSU_TYPE_AC,
    ONLP_PSU_TYPE_DC12,
    ONLP_PSU_TYPE_DC48,
    ONLP_PSU_TYPE_LAST = ONLP_PSU_TYPE_DC48,
    ONLP_PSU_TYPE_COUNT,
    ONLP_PSU_TYPE_INVALID = -1,
} onlp_psu_type_t;

/* Shortcut for CPU thermal threshold value. */
#define THERMAL_THRESHOLD_INIT_DEFAULTS  \
    { THERMAL_WARNING_DEFAULT, \
      THERMAL_ERROR_DEFAULT,   \
      THERMAL_SHUTDOWN_DEFAULT }

/** Fan_oid */
typedef enum fan_oid_e {
    FAN_OID_FAN0 = ONLP_FAN_ID_CREATE(1),
    FAN_OID_FAN1 = ONLP_FAN_ID_CREATE(2),
    FAN_OID_FAN2 = ONLP_FAN_ID_CREATE(3),
    FAN_OID_FAN3 = ONLP_FAN_ID_CREATE(4),
    FAN_OID_PSU0_FAN = ONLP_FAN_ID_CREATE(5),
    FAN_OID_PSU1_FAN = ONLP_FAN_ID_CREATE(6),
} fan_oid_t;

/** fan_id */
typedef enum fan_id_e {
    FAN_ID_FAN0 = 1,
    FAN_ID_FAN1 = 2,
    FAN_ID_FAN2 = 3,
    FAN_ID_FAN3 = 4,
    FAN_ID_PSU0_FAN = 5,
    FAN_ID_PSU1_FAN = 6,
    FAN_ID_MAX = 7,
} fan_id_t;

/** led_oid */
typedef enum psu_oid_e {
    PSU_OID_PSU0 = ONLP_PSU_ID_CREATE(1),
    PSU_OID_PSU1 = ONLP_PSU_ID_CREATE(2)
} psu_oid_t;

/** psu_id */
typedef enum psu_id_e {
    PSU_ID_PSU0 = 1,
    PSU_ID_PSU1 = 2,
    PSU_ID_PSU0_VIN = 3,
    PSU_ID_PSU0_VOUT = 4,
    PSU_ID_PSU0_IIN = 5,
    PSU_ID_PSU0_IOUT = 6,
    PSU_ID_PSU0_STBVOUT = 7,
    PSU_ID_PSU0_STBIOUT = 8,
    PSU_ID_PSU1_VIN = 9,
    PSU_ID_PSU1_VOUT = 10,
    PSU_ID_PSU1_IIN = 11,
    PSU_ID_PSU1_IOUT = 12,
    PSU_ID_PSU1_STBVOUT = 13,
    PSU_ID_PSU1_STBIOUT = 14,
    PSU_ID_MAX = 15,
} psu_id_t;

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


int psu_thermal_get(onlp_thermal_info_t* info, int id);

int psu_present_get(int *pw_present, int id);

int psu_pwgood_get(int *pw_good, int id);

int get_psu_type(int local_id, int *psu_type, bmc_fru_t *fru_in);

int sysi_platform_info_get(onlp_platform_info_t* pi);

int get_board_id(void);

int qsfp_present_get(int port, int *pres_val);

int sfp_present_get(int port, int *pres_val);

bool onlp_sysi_bmc_en_get(void);

int qsfp_port_to_cpld_addr(int port);

int qsfp_port_to_sysfs_attr_offset(int port);

int read_ioport(int addr, int *reg_val);

int bmc_thermal_info_get(onlp_thermal_info_t* info, int id);

int bmc_fan_info_get(onlp_fan_info_t* info, int id);

int exec_cmd(char *cmd, char* out, int size);

int file_read_hex(int* value, const char* fmt, ...);

int file_vread_hex(int* value, const char* fmt, va_list vargs);

int parse_bmc_sdr_cmd(char *cmd_out, int cmd_out_size,
                  char *tokens[], int token_size,
                  const char *sensor_id_str, int *idx);

int sys_led_info_get(onlp_led_info_t* info, int id);

void lock_init();

int bmc_check_alive(void);

int bmc_sensor_read(int bmc_cache_index, int sensor_type, float *data);

int bmc_fru_read(int local_id, bmc_fru_t *data);

void check_and_do_i2c_mux_reset(int port);

uint8_t ufi_shift(uint8_t mask);

uint8_t ufi_mask_shift(uint8_t val, uint8_t mask);

uint8_t ufi_bit_operation(uint8_t reg_val, uint8_t bit, uint8_t bit_val);

extern bool bmc_enable;
int onlp_data_path_reset(uint8_t unit_id, uint8_t reset_dev);

#endif  /* __PLATFORM_LIB_H__ */