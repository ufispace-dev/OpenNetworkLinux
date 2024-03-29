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

#include "x86_64_ufispace_s9502_12sm_int.h"
#include "x86_64_ufispace_s9502_12sm_log.h"
#include <x86_64_ufispace_s9502_12sm/x86_64_ufispace_s9502_12sm_config.h>

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
#define SYS_HWMON_PREFIX            "/sys/class/hwmon/hwmon%d/"
#define SYS_DEV_BUS_ADDR_PREFIX     "/sys/bus/i2c/devices/%d-%04x/"
#define SYS_I2C_HWMON_PREFIX        SYS_DEV_BUS_ADDR_PREFIX"hwmon/hwmon%d/"
#define SYS_GPIO_VAL                "/sys/class/gpio/gpio%d/value"
#define SYS_EEPROM_PATH             "/sys/bus/i2c/devices/1-0057/eeprom"
#define SYS_EEPROM_SIZE 512
#define UCD_TMP_BUS                 9
#define UCD90124A_ADDR              (0x41)
#define TMP451_ADDR                 (0x4e)

#define BMC_EN_FILE_PATH            "/etc/onl/bmc_en"
#define BMC_SENSOR_CACHE            "/tmp/bmc_sensor_cache"

#define CMD_BIOS_VER                "dmidecode -s bios-version | tail -1 | tr -d '\r\n'"

#define PSU_STATUS_PRESENT          1
#define PSU_STATUS_POWER_GOOD       1

#define RJ45_NUM                    0
#define SFP_NUM                     8
#define SFP_PLUS_NUM                4
#define PORT_NUM                    (RJ45_NUM + SFP_NUM + SFP_PLUS_NUM)
#define SFP_START_NUM               0

//FIXME
#define THERMAL_NUM                 15
#define LED_NUM                     4
#define FAN_NUM                     8

/* THERMAL THRESHOLD */
#define THERMAL_SHUTDOWN_DEFAULT    105000
#define THERMAL_ERROR_DEFAULT       95000
#define THERMAL_ERROR_FAN_PERC      100
#define THERMAL_WARNING_DEFAULT     77000
#define THERMAL_WARNING_FAN_PERC    80
#define THERMAL_NORMAL_DEFAULT      72000
#define THERMAL_NORMAL_FAN_PERC     50

/* I2C bus */
#define I2C_BUS_I801               0
#define I2C_BUS_ISMT               1

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

#define PSU_STATE_REG           0x25
#define PSU0_PRESENT_MASK       0x01
#define PSU1_PRESENT_MASK       0x02
#define PSU0_PWGOOD_MASK        0x10
#define PSU1_PWGOOD_MASK        0x20

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
#define CPLD_MAX                1
#define CPLD_BASE_ADDR          0x30

/* QSFP */
#define QSFP_PRES_REG1          0x20
#define QSFP_PRES_REG2          0x21
#define QSFP_PRES_OFFSET1       0x00
#define QSFP_PRES_OFFSET2       0x01

/* FAN */
#define FAN0_PRESENT_MASK       0x08
#define FAN1_PRESENT_MASK       0x04
#define FAN2_PRESENT_MASK       0x02
#define FANS_PRESENT_MASK       0x01

#define FAN_CACHE_TIME          5
#define PSU_CACHE_TIME          5
#define THERMAL_CACHE_TIME      10

/* CPLD */
#define CPLD_REG_BASE           0x700
#define CPLD_REG_VER            0x02
#define CPLD_REG_FAN_PRNT       0x14
#define CPLD_REG_PSU_STATUS     0x17
#define CPLD_REG_LED_CLEAR      0x35
#define CPLD_REG_LED_CTRL       0x36
#define CPLD_REG_LED_BLNK       0x37

enum sensor
{
    FAN_SENSOR = 0,
    PSU_SENSOR,
    THERMAL_SENSOR,
};

/** led_oid */
typedef enum led_oid_e {
    LED_OID_SYNC = ONLP_LED_ID_CREATE(1),
    LED_OID_SYS = ONLP_LED_ID_CREATE(2),
    LED_OID_PSU = ONLP_LED_ID_CREATE(3),
    LED_OID_PWR = ONLP_LED_ID_CREATE(4),
} led_oid_t;

/** led_id */
typedef enum led_id_e {
    LED_ID_SYS_SYNC = 1,
    LED_ID_SYS_SYS = 2,
    LED_ID_SYS_PSU = 3,
    LED_ID_SYS_PWR = 4,
} led_id_t;

/** Thermal_oid */
typedef enum thermal_oid_e {
    THERMAL_OID_CPU_PKG = ONLP_THERMAL_ID_CREATE(1),
    THERMAL_OID_CPU0 = ONLP_THERMAL_ID_CREATE(2),
    THERMAL_OID_CPU1 = ONLP_THERMAL_ID_CREATE(3),
    THERMAL_OID_DRAM0 = ONLP_THERMAL_ID_CREATE(4),
    THERMAL_OID_DRAM1 = ONLP_THERMAL_ID_CREATE(5),
    THERMAL_OID_MAC = ONLP_THERMAL_ID_CREATE(6),
} thermal_oid_t;

/** thermal_id */
typedef enum thermal_id_e {
    THERMAL_ID_CPU_PKG = 1,
    THERMAL_ID_CPU0 = 2,
    THERMAL_ID_CPU1 = 3,
    THERMAL_ID_DRAM0 = 4,
    THERMAL_ID_DRAM1 = 5,
    THERMAL_ID_MAC = 6,
    THERMAL_ID_MAX = 7,//TODO
} thermal_id_t;

/* Shortcut for CPU thermal threshold value. */
#define THERMAL_THRESHOLD_INIT_DEFAULTS  \
    { THERMAL_WARNING_DEFAULT, \
      THERMAL_ERROR_DEFAULT,   \
      THERMAL_SHUTDOWN_DEFAULT }

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

int psu_thermal_get(onlp_thermal_info_t* info, int id);

int psu_fan_info_get(onlp_fan_info_t* info, int id);

int psu_vin_get(onlp_psu_info_t* info, int id);

int psu_vout_get(onlp_psu_info_t* info, int id);

int psu_iin_get(onlp_psu_info_t* info, int id);

int psu_iout_get(onlp_psu_info_t* info, int id);

int psu_pout_get(onlp_psu_info_t* info, int id);

int psu_pin_get(onlp_psu_info_t* info, int id);

int psu_eeprom_get(onlp_psu_info_t* info, int id);

int psu_present_get(int *pw_present, int id);

int psu_pwgood_get(int *pw_good, int id);

int psu2_led_set(onlp_led_mode_t mode);

int psu1_led_set(onlp_led_mode_t mode);

int fan_led_set(onlp_led_mode_t mode);

int system_led_set(onlp_led_mode_t mode);

int sysi_platform_info_get(onlp_platform_info_t* pi);

int sfp_present_get(int port, int *pres_val);

bool onlp_sysi_bmc_en_get(void);

int read_ioport(int addr, int *reg_val);

int write_ioport(int addr, int reg_val);

int exec_cmd(char *cmd, char* out, int size);

int file_read_hex(int* value, const char* fmt, ...);

int file_vread_hex(int* value, const char* fmt, va_list vargs);

int sys_led_info_get(onlp_led_info_t* info, int id);

int psu_fru_get(onlp_psu_info_t* info, int id);

int psu_stbiout_get(int* stbiout, int id);

int psu_stbvout_get(int* stbvout, int id);

extern bool bmc_enable;
#endif  /* __PLATFORM_LIB_H__ */