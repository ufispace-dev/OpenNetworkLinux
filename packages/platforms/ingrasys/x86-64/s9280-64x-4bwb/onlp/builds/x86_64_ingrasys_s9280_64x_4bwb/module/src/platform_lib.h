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
#include "x86_64_ingrasys_s9280_64x_4bwb_int.h"
#include "x86_64_ingrasys_s9280_64x_4bwb_log.h"
#include <x86_64_ingrasys_s9280_64x_4bwb/x86_64_ingrasys_s9280_64x_4bwb_config.h>


#define SYS_CPU_CORETEMP_PREFIX     "/sys/devices/platform/coretemp.0/hwmon/hwmon0/"
#define SYS_CPU_CORETEMP_PREFIX2    "/sys/devices/platform/coretemp.0/"
#define SYS_CPU_BOARD_PREFIX        "/sys/class/hwmon/hwmon1/"
#define SYS_EEPROM_PATH             "/sys/bus/i2c/devices/0-0053/eeprom"
#define SYS_EEPROM_SIZE             512
#define PSU_STATUS_PRESENT          1
#define PSU_STATUS_POWER_GOOD       1
#define QSFP_NUM                    64
#define SFP_NUM                     2
#define PSU_NUM                     2
#define PORT_NUM                    QSFP_NUM + SFP_NUM

#define THERMAL_SHUTDOWN_DEFAULT    105000

#define THERMAL_ERROR_DEFAULT       95000
#define THERMAL_ERROR_FAN_PERC      100

#define THERMAL_WARNING_DEFAULT     77000
#define THERMAL_WARNING_FAN_PERC    80

#define THERMAL_NORMAL_DEFAULT      72000
#define THERMAL_NORMAL_FAN_PERC     50

/* I2C bus */
#define I2C_BUS_0                   0
#define I2C_BUS_1                   1
#define I2C_BUS_2                   2
#define I2C_BUS_3                   3
#define I2C_BUS_4                   4
#define I2C_BUS_5                   5

#define I2C_BUS_CPLD1               I2C_BUS_1      /* CPLD 1 */
#define I2C_BUS_CPLD2               I2C_BUS_2      /* CPLD 2 */
#define I2C_BUS_CPLD3               I2C_BUS_3      /* CPLD 3 */
#define I2C_BUS_CPLD4               I2C_BUS_4      /* CPLD 4 */
#define I2C_BUS_CPLD5               I2C_BUS_5      /* CPLD 5 */

/* CPLD */
#define CPLD_MAX                    5
#define CPLD_I2C_ADDR               0x33
#define CPLD_BUS_BASE               I2C_BUS_1

/* SYSFS ATTR */
#define MB_CPLD_SYSFS_PATH              "/sys/bus/i2c/devices/%d-00%02x"
#define MB_CPLD_VER_ATTR                "cpld_version"
#define MB_CPLD_BOARD_TYPE_ATTR         "cpld_board_type"
#define MB_CPLD_EXT_BOARD_TYPE_ATTR     "cpld_ext_board_type"
#define MB_CPLD_QSFP_PORT_STATUS_ATTR   "cpld_qsfp_port_status"
#define MB_CPLD_SFP_PORT_STATUS_ATTR    "cpld_sfp_port_status"
#define MB_CPLD_SFP_PORT_CONFIF_ATTR    "cpld_sfp_port_config"

/* PORTS */
#define QSFP_EEPROM_I2C_ADDR        0x50
#define CPLD1_PORTS                 12
#define CPLDx_PORTS                 13
#define CPLD_PRES_BIT               1
#define CPLD_SFP1_PRES_BIT          1
#define CPLD_SFP2_PRES_BIT          4
#define CPLD_QSFP_REG_PATH          "/sys/bus/i2c/devices/%d-00%02x/%s_%d"
#define CPLD_SFP_REG_PATH           "/sys/bus/i2c/devices/%d-00%02x/%s"

/* bmc cache */
#define BMC_SENSOR_CACHE            "/tmp/bmc_sensor_cache"
#define BMC_FRU_CACHE               "/tmp/bmc_fru_cache.PUS%d"
#define CMD_BMC_SENSOR_CACHE        "ipmitool sdr -c get %s > "BMC_SENSOR_CACHE
#define CMD_BMC_CACHE_GET           "cat "BMC_SENSOR_CACHE" | grep %s | awk -F',' '{print $%d}'"
#define CMD_BMC_FRU_CACHE           "ipmitool fru print %d > %s"
#define CMD_CACHE_FRU_GET           "cat %s | grep '%s' | cut -d':' -f2 | awk '{$1=$1};1' | tr -d '\\n'"
#define CMD_BMC_LED_GET             "ipmitool raw 0x3c 0x20 0x0 | xargs | cut -d' ' -f%d"

#define FAN_CACHE_TIME              5
#define PSU_CACHE_TIME              5
#define THERMAL_CACHE_TIME          10
#define FRU_CACHE_TIME              10

/* VERSION */
#define CMD_BIOS_VER                "dmidecode -s bios-version | tail -1 | tr -d '\\r\\n'"
#define CMD_BMC_VER_1               "expr `ipmitool mc info | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f1` + 0"
#define CMD_BMC_VER_2               "expr `ipmitool mc info | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f2` + 0"
#define CMD_BMC_VER_3               "echo $((`ipmitool mc info | grep 'Aux Firmware Rev Info' -A 2 | sed -n '2p'`))"

/* FUCNTION ENABLE */
#define ENABLE_SYSLED               1

enum sensor
{
    FAN_SENSOR = 0,
    PSU_SENSOR,
    THERMAL_SENSOR,
    LED_SENSOR,
};

enum bmc_led_status
{
    BMC_LED_OFF = 0,
    BMC_LED_YELLOW,
    BMC_LED_GREEN,
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
    LED_OID_FAN = ONLP_LED_ID_CREATE(2),
    LED_OID_PSU1 = ONLP_LED_ID_CREATE(3),
    LED_OID_PSU2 = ONLP_LED_ID_CREATE(4),
} led_oid_t;

/** led_id */
typedef enum led_id_e {
    LED_ID_SYS = 1,
    LED_ID_FAN = 2,
    LED_ID_PSU1 = 3,
    LED_ID_PSU2 = 4,
} led_id_t;

/** Thermal_oid */
typedef enum thermal_oid_e {
    THERMAL_OID_CPU_PECI = ONLP_THERMAL_ID_CREATE(1),
    THERMAL_OID_BMC_ENV = ONLP_THERMAL_ID_CREATE(2),
    THERMAL_OID_PCIE_CONN = ONLP_THERMAL_ID_CREATE(3),
    THERMAL_OID_CAGE_R = ONLP_THERMAL_ID_CREATE(4),
    THERMAL_OID_CAGE_L = ONLP_THERMAL_ID_CREATE(5),
    THERMAL_OID_MAC_FRONT = ONLP_THERMAL_ID_CREATE(6),
    THERMAL_OID_MAC_ENV = ONLP_THERMAL_ID_CREATE(7),
    THERMAL_OID_MAC_DIE = ONLP_THERMAL_ID_CREATE(8),
    THERMAL_OID_PSU1 = ONLP_THERMAL_ID_CREATE(9),
    THERMAL_OID_PSU2 = ONLP_THERMAL_ID_CREATE(10),
    THERMAL_OID_CPU_BOARD = ONLP_THERMAL_ID_CREATE(11),
    THERMAL_OID_CPU_PKG = ONLP_THERMAL_ID_CREATE(12),
    THERMAL_OID_CPU1 = ONLP_THERMAL_ID_CREATE(13),
    THERMAL_OID_CPU2 = ONLP_THERMAL_ID_CREATE(14),
    THERMAL_OID_CPU3 = ONLP_THERMAL_ID_CREATE(15),
    THERMAL_OID_CPU4 = ONLP_THERMAL_ID_CREATE(16),
    THERMAL_OID_CPU5 = ONLP_THERMAL_ID_CREATE(17),
    THERMAL_OID_CPU6 = ONLP_THERMAL_ID_CREATE(18),
    THERMAL_OID_CPU7 = ONLP_THERMAL_ID_CREATE(19),
    THERMAL_OID_CPU8 = ONLP_THERMAL_ID_CREATE(20),
} thermal_oid_t;

/** thermal_id */
typedef enum thermal_id_e {
    THERMAL_ID_CPU_PECI = 1,
    THERMAL_ID_BMC_ENV = 2,
    THERMAL_ID_PCIE_CONN = 3,
    THERMAL_ID_CAGE_R = 4,
    THERMAL_ID_CAGE_L = 5,
    THERMAL_ID_MAC_FRONT = 6,
    THERMAL_ID_MAC_ENV = 7,
    THERMAL_ID_MAC_DIE = 8,
    THERMAL_ID_PSU1 = 9,
    THERMAL_ID_PSU2 = 10,
    THERMAL_ID_CPU_BOARD = 11,
    THERMAL_ID_CPU_PKG = 12,
    THERMAL_ID_CPU1 = 13,
    THERMAL_ID_CPU2 = 14,
    THERMAL_ID_CPU3 = 15,
    THERMAL_ID_CPU4 = 16,
    THERMAL_ID_CPU5 = 17,
    THERMAL_ID_CPU6 = 18,
    THERMAL_ID_CPU7 = 19,
    THERMAL_ID_CPU8 = 20,
} thermal_id_t;

/* Shortcut for CPU thermal threshold value. */
#define THERMAL_THRESHOLD_INIT_DEFAULTS  \
    { THERMAL_WARNING_DEFAULT, \
      THERMAL_ERROR_DEFAULT,   \
      THERMAL_SHUTDOWN_DEFAULT }

/** Fan_oid */
typedef enum fan_oid_e {
    FAN_OID_FAN1 = ONLP_FAN_ID_CREATE(1),
    FAN_OID_FAN2 = ONLP_FAN_ID_CREATE(2),
    FAN_OID_FAN3 = ONLP_FAN_ID_CREATE(3),
    FAN_OID_FAN4 = ONLP_FAN_ID_CREATE(4),
    FAN_OID_PSU1_FAN1 = ONLP_FAN_ID_CREATE(5),
    FAN_OID_PSU2_FAN1 = ONLP_FAN_ID_CREATE(6)
} fan_oid_t;

/** fan_id */
typedef enum fan_id_e {
    FAN_ID_FAN1 = 1,
    FAN_ID_FAN2 = 2,
    FAN_ID_FAN3 = 3,
    FAN_ID_FAN4 = 4,
    FAN_ID_PSU1_FAN1 = 5,
    FAN_ID_PSU2_FAN1 = 6
} fan_id_t;

/** led_oid */
typedef enum psu_oid_e {
    PSU_OID_PSU1 = ONLP_PSU_ID_CREATE(1),
    PSU_OID_PSU2 = ONLP_PSU_ID_CREATE(2)
} psu_oid_t;

/** fan_id */
typedef enum psu_id_e {
    PSU_ID_PSU1 = 1,
    PSU_ID_PSU2 = 2,
    PSU_ID_PSU1_VIN = 3,
    PSU_ID_PSU1_VOUT = 4,
    PSU_ID_PSU1_IIN = 5,
    PSU_ID_PSU1_IOUT = 6,
    PSU_ID_PSU1_STBVOUT = 7,
    PSU_ID_PSU1_STBIOUT = 8,
    PSU_ID_PSU2_VIN = 9,
    PSU_ID_PSU2_VOUT = 10,
    PSU_ID_PSU2_IIN = 11,
    PSU_ID_PSU2_IOUT = 12,
    PSU_ID_PSU2_STBVOUT = 13,
    PSU_ID_PSU2_STBIOUT = 14,
    PSU_ID_MAX = 15,
} psu_id_t;

typedef enum bmc_cache_idx_e {
    ID_TEMP_CPU_PECI = 0,
    ID_TEMP_BMC_ENV,
    ID_TEMP_PCIE_CONN,
    ID_TEMP_CAGE_R,
    ID_TEMP_CAGE_L,
    ID_TEMP_MAC_FRONT,
    ID_TEMP_MAC_ENV,
    ID_TEMP_MAC_DIE,
    ID_PSU1_TEMP,
    ID_PSU2_TEMP,
    ID_FAN1_RPM,
    ID_FAN2_RPM,
    ID_FAN3_RPM,
    ID_FAN4_RPM,
    ID_PSU1_FAN1,
    ID_PSU2_FAN1,
    ID_FAN1_PSNT_L,
    ID_FAN2_PSNT_L,
    ID_FAN3_PSNT_L,
    ID_FAN4_PSNT_L,
    ID_PSU1_PRSNT_L,
    ID_PSU1_PWROK_H,
    ID_PSU2_PRSNT_L,
    ID_PSU2_PWROK_H,
    ID_PSU1_VIN,
    ID_PSU1_VOUT,
    ID_PSU1_IIN,
    ID_PSU1_IOUT,
    ID_PSU1_STBVOUT,
    ID_PSU1_STBIOUT,
    ID_PSU2_VIN,
    ID_PSU2_VOUT,
    ID_PSU2_IIN,
    ID_PSU2_IOUT,
    ID_PSU2_STBVOUT,
    ID_PSU2_STBIOUT,
    ID_LED_SYS,
    ID_LED_FAN,
    ID_LED_PSU1,
    ID_LED_PSU2,
    ID_MAX,
} bmc_cache_idx_t;

void lock_init();

int file_vread_hex(int* value, const char* fmt, va_list vargs);

int bmc_sensor_read(int bmc_cache_index, int sensor_type, float *data);

int bmc_thermal_info_get(onlp_thermal_info_t* info, int id);

int bmc_fan_info_get(onlp_fan_info_t* info, int id);

int bmc_fru_read(onlp_psu_info_t* info, int fru_id);

int sysi_platform_info_get(onlp_platform_info_t* pi);

int sys_led_info_get(onlp_led_info_t* info, int id);

int file_read_hex(int* value, const char* fmt, ...);
#endif  /* __PLATFORM_LIB_H__ */
