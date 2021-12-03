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
#include <onlp/platformi/sfpi.h>
#include "x86_64_ufispace_s9300_32d_int.h"
#include "x86_64_ufispace_s9300_32d_log.h"
#include <x86_64_ufispace_s9300_32d/x86_64_ufispace_s9300_32d_config.h>

/* SYSFS */
#define SYS_CPU_CORETEMP_PREFIX         "/sys/devices/platform/coretemp.0/hwmon/hwmon0/"
#define SYS_CPU_CORETEMP_PREFIX2      "/sys/devices/platform/coretemp.0/"
#define SYS_CORE_TEMP_PREFIX        "/sys/class/hwmon/hwmon2/"

#define SYS_EEPROM_PATH             "/sys/bus/i2c/devices/0-0057/eeprom"
#define SYS_EEPROM_SIZE 512
#define BMC_EN_FILE_PATH            "/etc/onl/bmc_en"
#define BMC_SENSOR_CACHE            "/tmp/bmc_sensor_cache"
#define BMC_FRU_CACHE            "/tmp/bmc_fru_cache.PUS%d"
#define MB_CPLD1_SYSFS_PATH    "/sys/bus/i2c/devices/2-0030"
#define MB_CPLD2_SYSFS_PATH    "/sys/bus/i2c/devices/2-0031"
#define MB_CPLD3_SYSFS_PATH    "/sys/bus/i2c/devices/2-0032"
#define MB_CPLDX_SYSFS_PATH    "/sys/bus/i2c/devices/2-00%02x"
#define MB_CPLD_ID_ATTR                 "cpld_id"
#define MB_CPLD_MAJOR_VER_ATTR                 "cpld_major_ver"
#define MB_CPLD_MINOR_VER_ATTR                 "cpld_minor_ver"
#define MB_CPLD_BUILD_VER_ATTR                  "cpld_build_ver"
#define MB_CPLD_SFP_RXLOS_ATTR  "cpld_sfp_rxlos"
#define MB_CPLD_SFP_TXFLT_ATTR  "cpld_sfp_txfault"
#define MB_CPLD_SFP_TXDIS_ATTR  "cpld_sfp_tx_dis"
#define MB_CPLD_PSU_STS_ATTR    "cpld_psu_status"
#define MB_CPLD_LED_ATTR            "cpld_sys_led_ctrl_%d"
#define MB_CPLD_QSFPDD_PRES_ATTR    "cpld_qsfpdd_pres_g%d"
#define MB_CPLD_SFP_ABS_ATTR    "cpld_sfp_abs"
#define LPC_MB_CPLD_SYFSFS_PATH \
            "/sys/devices/platform/x86_64_ufispace_s9300_32d_lpc/mb_cpld"
#define LPC_MUX_RESET_ATTR      "mux_reset"
#define LPC_MB_CPLDX_VER_ATTR "mb_cpld_%d_version_h"
#define LPC_MB_SKU_ID_ATTR      "board_sku_id"
#define LPC_MB_HW_ID_ATTR       "board_hw_id"
#define LPC_MB_ID_TYPE_ATTR     "board_id_type"
#define LPC_MB_BUILD_ID_ATTR    "board_build_id"
#define LPC_MB_DEPH_ID_ATTR     "board_deph_id"

#define LPC_CPU_CPLD_SYSFFS_PATH \
            "/sys/devices/platform/x86_64_ufispace_s9300_32d_lpc/cpu_cpld"
#define LPC_CPU_CPLD_VER_ATTR "cpu_cpld_version"
#define LPC_CPU_CPLD_BUILD_ATTR "cpu_cpld_build"

/* BMC CMD */
#define BMC_CACHE_EN            1
#define BMC_CACHE_CYCLE         30
#define BMC_CMD_SDR_SIZE        48
#define BMC_TOKEN_SIZE          20

#define FAN_CACHE_TIME          5
#define PSU_CACHE_TIME          5
#define THERMAL_CACHE_TIME      10
#define FRU_CACHE_TIME      10

#define CMD_BIOS_VER                "dmidecode -s bios-version | tail -1 | tr -d '\\r\\n'"
#define CMD_BMC_VER_1               "expr `ipmitool mc info | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f1` + 0"
#define CMD_BMC_VER_2               "expr `ipmitool mc info | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f2` + 0"
#define CMD_BMC_VER_3               "echo $((`ipmitool mc info | grep 'Aux Firmware Rev Info' -A 2 | sed -n '2p'`))"
#define CMD_BMC_SENSOR_CACHE        "ipmitool sdr -c get %s > "BMC_SENSOR_CACHE
#define CMD_BMC_SDR_GET             "ipmitool sdr -c get %s"
#define CMD_FRU_INFO_GET            "ipmitool fru print %d | grep '%s' | cut -d':' -f2 | awk '{$1=$1};1' | tr -d '\\n'"
#define CMD_BMC_CACHE_GET           "cat "BMC_SENSOR_CACHE" | grep %s | awk -F',' '{print $%d}'"
#define CMD_BMC_FRU_CACHE        "ipmitool fru print %d > %s"
#define CMD_CACHE_FRU_GET        "cat %s | grep '%s' | cut -d':' -f2 | awk '{$1=$1};1' | tr -d '\\n'"
#define CMD_BMC_FAN_TRAY_DIR        "ipmitool raw 0x3c 0x31 0x0 | xargs | cut -d' ' -f%d"
#define CMD_BMC_PSU_FAN_DIR        "ipmitool raw 0x3c 0x30 0x0 | xargs | cut -d' ' -f%d"

/* MAC PORT */
#define QSFPDD_NUM                32
#define SFP_CPU_NUM             2
#define SFP_MAC_NUM             2
#define SFP_NUM                     SFP_CPU_NUM +  SFP_MAC_NUM
#define PORT_NUM                    QSFPDD_NUM + SFP_NUM
#define SFP_0_IFNAME                "enp182s0f0"
#define SFP_1_IFNAME                "enp182s0f1"
#define CMD_SFP_EEPROM_GET  "ethtool -m %s raw on length 256 > /tmp/.sfp.%s.eeprom"
#define CMD_SFP_PRES_GET        "ethtool -m %s raw on length 1 > /dev/null 2>&1"

/* default value */
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
#define QSFPDD_BASE_BUS 17
#define SFP_BASE_BUS        13

/* PSU */
#define PSU1_PRESENT_MASK       0b00000100
#define PSU2_PRESENT_MASK       0b00001000
#define PSU1_PWGOOD_MASK        0b00000001
#define PSU2_PWGOOD_MASK        0b00000010
#define PSU_STATUS_PRESENT          1
#define PSU_STATUS_POWER_GOOD       1

/* LED */


/* SYS */
#define CPLD_MAX                    3
#define CPLD_BASE_ADDR      0x30
#define THERMAL_NUM             18
#define LED_NUM                     4
#define FAN_NUM                     14
#define PSU_NUM                     2

/* FAN */
#define FAN_DIR_B2F               1
#define FAN_DIR_F2B               2

enum sensor
{
    FAN_SENSOR = 0,
    PSU_SENSOR,
    THERMAL_SENSOR,
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


/** led_oid */
typedef enum led_oid_e {
    LED_OID_SYSTEM = ONLP_LED_ID_CREATE(1),    
    LED_OID_PSU0 = ONLP_LED_ID_CREATE(2),
    LED_OID_PSU1 = ONLP_LED_ID_CREATE(3),
    LED_OID_FAN = ONLP_LED_ID_CREATE(4),
} led_oid_t;

/** led_id */
typedef enum led_id_e {
    LED_ID_SYS_SYS = 1,    
    LED_ID_SYS_PSU0 = 2,
    LED_ID_SYS_PSU1 = 3,
    LED_ID_SYS_FAN = 4,
} led_id_t;

/** Thermal_oid */
typedef enum thermal_oid_e {
    THERMAL_OID_CPU_PECI = ONLP_THERMAL_ID_CREATE(1),
    THERMAL_OID_CPU_ENV = ONLP_THERMAL_ID_CREATE(2),    
    THERMAL_OID_CPU_ENV_2 = ONLP_THERMAL_ID_CREATE(3),
    THERMAL_OID_MAC_ENV = ONLP_THERMAL_ID_CREATE(4),
    THERMAL_OID_CAGE = ONLP_THERMAL_ID_CREATE(5),
    THERMAL_OID_PSU_CONN = ONLP_THERMAL_ID_CREATE(6),
    THERMAL_OID_PSU0 = ONLP_THERMAL_ID_CREATE(7),
    THERMAL_OID_PSU1 = ONLP_THERMAL_ID_CREATE(8),    
    THERMAL_OID_CPU_PKG = ONLP_THERMAL_ID_CREATE(9),
    THERMAL_OID_CPU1 = ONLP_THERMAL_ID_CREATE(10),
    THERMAL_OID_CPU2 = ONLP_THERMAL_ID_CREATE(11),
    THERMAL_OID_CPU3 = ONLP_THERMAL_ID_CREATE(12),
    THERMAL_OID_CPU4 = ONLP_THERMAL_ID_CREATE(13), 
    THERMAL_OID_CPU5 = ONLP_THERMAL_ID_CREATE(14), 
    THERMAL_OID_CPU6 = ONLP_THERMAL_ID_CREATE(15), 
    THERMAL_OID_CPU7 = ONLP_THERMAL_ID_CREATE(16), 
    THERMAL_OID_CPU8 = ONLP_THERMAL_ID_CREATE(17),
} thermal_oid_t;

/** thermal_id */
typedef enum thermal_id_e {
    THERMAL_ID_CPU_PECI = 1,    
    THERMAL_ID_CPU_ENV = 2,
    THERMAL_ID_CPU_ENV_2 = 3, 
    THERMAL_ID_MAC_ENV = 4,      
    THERMAL_ID_CAGE = 5,
    THERMAL_ID_PSU_CONN = 6,
    THERMAL_ID_PSU0 = 7, 
    THERMAL_ID_PSU1 = 8,
    THERMAL_ID_CPU_PKG = 9,
    THERMAL_ID_CPU1 = 10,
    THERMAL_ID_CPU2 = 11,
    THERMAL_ID_CPU3 = 12,
    THERMAL_ID_CPU4 = 13,
    THERMAL_ID_CPU5 = 14,
    THERMAL_ID_CPU6 = 15,
    THERMAL_ID_CPU7 = 16,
    THERMAL_ID_CPU8 = 17,
    THERMAL_ID_MAX = 18,
} thermal_id_t;

typedef enum bmc_cache_idx_e {
    ID_TEMP_CPU_PECI = 0,
    ID_TEMP_CPU_ENV,
    ID_TEMP_CPU_ENV_2,
    ID_TEMP_MAC_ENV,
    ID_TEMP_CAGE,
    ID_TEMP_PSU_CONN,
    ID_PSU0_TEMP,
    ID_PSU1_TEMP,
    ID_FAN0_RPM_F,
    ID_FAN0_RPM_R,
    ID_FAN1_RPM_F,
    ID_FAN1_RPM_R,
    ID_FAN2_RPM_F,
    ID_FAN2_RPM_R,
    ID_FAN3_RPM_F,
    ID_FAN3_RPM_R,
    ID_FAN4_RPM_F,
    ID_FAN4_RPM_R,
    ID_FAN5_RPM_F,
    ID_FAN5_RPM_R,
    ID_PSU0_FAN1,
    ID_PSU1_FAN1,
    ID_FAN0_PSNT_L,
    ID_FAN1_PSNT_L,
    ID_FAN2_PSNT_L,
    ID_FAN3_PSNT_L,
    ID_FAN4_PSNT_L,
    ID_FAN5_PSNT_L,
    ID_FAN0_DIR,
    ID_FAN1_DIR,
    ID_FAN2_DIR,
    ID_FAN3_DIR,
    ID_FAN4_DIR,
    ID_FAN5_DIR,
    ID_PSU0_FAN1_DIR,
    ID_PSU1_FAN1_DIR,
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

} bmc_cache_idx_t;

/* Shortcut for CPU thermal threshold value. */
#define THERMAL_THRESHOLD_INIT_DEFAULTS  \
    { THERMAL_WARNING_DEFAULT, \
      THERMAL_ERROR_DEFAULT,   \
      THERMAL_SHUTDOWN_DEFAULT }
      
/** Fan_oid */
typedef enum fan_oid_e {
    FAN_OID_FAN1_F = ONLP_FAN_ID_CREATE(1),
    FAN_OID_FAN1_R = ONLP_FAN_ID_CREATE(2),
    FAN_OID_FAN2_F = ONLP_FAN_ID_CREATE(3),
    FAN_OID_FAN2_R = ONLP_FAN_ID_CREATE(4),
    FAN_OID_FAN3_F = ONLP_FAN_ID_CREATE(5),
    FAN_OID_FAN3_R = ONLP_FAN_ID_CREATE(6),
    FAN_OID_FAN4_F = ONLP_FAN_ID_CREATE(7),
    FAN_OID_FAN4_R = ONLP_FAN_ID_CREATE(8),
    FAN_OID_FAN5_F = ONLP_FAN_ID_CREATE(9),
    FAN_OID_FAN5_R = ONLP_FAN_ID_CREATE(10),
    FAN_OID_FAN6_F = ONLP_FAN_ID_CREATE(11),  
    FAN_OID_FAN6_R = ONLP_FAN_ID_CREATE(12),  
    FAN_OID_PSU0_FAN1 = ONLP_FAN_ID_CREATE(13),
    FAN_OID_PSU1_FAN1 = ONLP_FAN_ID_CREATE(14),
} fan_oid_t;

/** fan_id */
typedef enum fan_id_e {
    FAN_ID_FAN1_F = 1,
    FAN_ID_FAN1_R = 2,
    FAN_ID_FAN2_F = 3,
    FAN_ID_FAN2_R = 4,
    FAN_ID_FAN3_F = 5,
    FAN_ID_FAN3_R = 6,
    FAN_ID_FAN4_F = 7,
    FAN_ID_FAN4_R = 8,
    FAN_ID_FAN5_F = 9,
    FAN_ID_FAN5_R = 10,
    FAN_ID_FAN6_F = 11,
    FAN_ID_FAN6_R = 12,
    FAN_ID_PSU0_FAN1 = 13,
    FAN_ID_PSU1_FAN1 = 14,
    FAN_ID_MAX = 15,
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

int psu_present_get(int *pw_present, int id);

int psu_pwgood_get(int *pw_good, int id);

int sysi_platform_info_get(onlp_platform_info_t* pi);

bool onlp_sysi_bmc_en_get(void);

int bmc_thermal_info_get(onlp_thermal_info_t* info, int id);

int bmc_fan_info_get(onlp_fan_info_t* info, int id);

int exec_cmd(char *cmd, char* out, int size);

int file_read_hex(int* value, const char* fmt, ...);

int file_vread_hex(int* value, const char* fmt, va_list vargs);

int parse_bmc_sdr_cmd(char *cmd_out, int cmd_out_size,
                  char *tokens[], int token_size, 
                  const char *sensor_id_str, int *idx);

int sys_led_info_get(onlp_led_info_t* info, int id);

int bmc_fru_read(onlp_psu_info_t* info, int fru_id);

void lock_init();

int bmc_sensor_read(int bmc_cache_index, int sensor_type, float *data);

void check_and_do_i2c_mux_reset(int port);

extern bool bmc_enable;
#endif  /* __PLATFORM_LIB_H__ */
