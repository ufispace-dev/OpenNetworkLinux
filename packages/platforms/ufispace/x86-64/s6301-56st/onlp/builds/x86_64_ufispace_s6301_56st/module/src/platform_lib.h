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

#include <onlp/onlp.h>
#include <onlplib/file.h>
#include <onlplib/i2c.h>
#include <onlp/platformi/psui.h>
#include <onlp/platformi/fani.h>
#include "x86_64_ufispace_s6301_56st_log.h"


#define ONLP_TRY(_expr)                                                 \
    do {                                                                \
        int _rv = (_expr);                                              \
        if(ONLP_FAILURE(_rv)) {                                         \
            AIM_LOG_ERROR("%s returned %{onlp_status}", #_expr, _rv);   \
            return _rv;                                                 \
        }                                                               \
    } while(0)

#define POID_0                      0

#define SYS_DEV                     "/sys/bus/i2c/devices/"
#define SYS_FMT                     SYS_DEV "%d-%04x/%s"
#define SYS_HWMON_FMT               "/sys/class/hwmon/hwmon%d/"
#define SYS_DEV_BUS_ADDR_PREFIX     "/sys/bus/i2c/devices/%d-%04x/"
#define SYS_I2C_HWMON_PREFIX        SYS_DEV_BUS_ADDR_PREFIX "hwmon/hwmon%d/"
#define SYS_GPIO_FMT                "/sys/class/gpio/gpio%d/value"
#define SYS_EEPROM                  "eeprom"

#define CMD_BIOS_VER                "dmidecode -s bios-version | tail -1 | tr -d '\r\n'"
#define CMD_I2C_STUCK_CHECK         "i2cget -f -y 0 0x71 > /dev/null 2>&1"

#define CORE_TEMP_HWM_ID            1
#define TMP_FAN1_HWM_ID             2
#define TMP_FAN2_HWM_ID             3
#define UCD_HWM_ID                  4

#define SYS_LPC                     "/sys/devices/platform/x86_64_ufispace_s6301_56st_lpc/"
#define LPC_MB_CPLD_PATH            SYS_LPC "mb_cpld/"
#define LPC_BSP_PATH                SYS_LPC "bsp/"
#define GPIO_MAX_SYSFS              LPC_BSP_PATH "bsp_gpio_max"
#define MUX_RESET_SYSFS             LPC_MB_CPLD_PATH "mux_reset"


#define RJ45_NUM                    48
#define SFP_PLUS_NUM                8
#define PORT_NUM                    (RJ45_NUM + SFP_PLUS_NUM)
#define SFP_0BASE_START_NUM         48
#define SFP_1BASE_START_NUM         49
#define THERMAL_NUM                 8
#define LED_NUM                     7
#define FAN_NUM                     2

/* I2C bus */
#define I2C_BUS_I801                0
#define I2C_BUS_ISMT                1

/* PSU */
#define PSU_BUS_ID                  2
#define PSU_PMBUS_FAN_RPM           0x90
#define PSU_PMBUS_VOUT1             0x20
#define PSU_PMBUS_VOUT2             0x8B
#define PSU_PMBUS_IOUT              0x8C
#define PSU_PMBUS_VIN               0x88
#define PSU_PMBUS_IIN               0x89
#define PSU_PMBUS_POUT              0x96
#define PSU_PMBUS_PIN               0x97
#define PSU_PMBUS_THERMAL1          0x8D
#define PSU_PMBUS_ADDR_0            0x58
#define PSU_PMBUS_ADDR_1            0x59

/* FAN DIR */
#define FAN0_DIR_GPIO_OFF           51
#define FAN1_DIR_GPIO_OFF           52
#define FAN_DIR_B2F                 0
#define FAN_DIR_F2B                 1

/* SYSFS ATTR */
#define LPC_MB_SKU_ID_ATTR          "board_sku_id"
#define LPC_MB_HW_ID_ATTR           "board_hw_id"
#define LPC_MB_ID_TYPE_ATTR         "board_id_type"
#define LPC_MB_BUILD_ID_ATTR        "board_build_id"
#define LPC_MB_DEPH_ID_ATTR         "board_deph_id"
#define LPC_MB_EXT_ID_ATTR          "board_ext_id"
#define LPC_MB_CPLD_VER_ATTR        "mb_cpld_1_version_h"
#define SYSFS_BIOS_VER              "/sys/class/dmi/id/bios_version"

/* VALUES */
#define MIN_GPIO_MAX                128
#define HW_REV_ALPHA                1

/* fan_id */
enum onlp_fan_id {
    ONLP_FAN_0 = 1,
    ONLP_FAN_1,
    ONLP_PSU_0_FAN,
    ONLP_PSU_1_FAN,
    ONLP_FAN_MAX,
};

/* led_id */
enum onlp_led_id {
    ONLP_LED_ID = 1,
    ONLP_LED_SYS,
    ONLP_LED_POE,
    ONLP_LED_SPD,
    ONLP_LED_FAN,
    ONLP_LED_LNK,
    ONLP_LED_PWR1,
    ONLP_LED_PWR0,
    ONLP_LED_MAX,
};

/* psu_id */
enum onlp_psu_id {
    ONLP_PSU_0 = 1,
    ONLP_PSU_1,
    ONLP_PSU_MAX,
};

/* thermal_id */
enum onlp_thermal_id {
    ONLP_THERMAL_FAN1 = 1,
    ONLP_THERMAL_FAN2,
    ONLP_THERMAL_PSUDB,
    ONLP_THERMAL_MAC,
    ONLP_THERMAL_INLET,
    ONLP_THERMAL_PSU0,
    ONLP_THERMAL_PSU1,
    ONLP_THERMAL_CPU_PKG,
    ONLP_THERMAL_MAX,
};

/* thermal_id */
enum sku_subtype_e {
    SKU_POE = 0,
    SKU_NPOE_0BASE,
    SKU_NPOE_1BASE
};

enum onlp_psu_type_e {
  ONLP_PSU_TYPE_AC,
  ONLP_PSU_TYPE_DC12,
  ONLP_PSU_TYPE_DC48,
  ONLP_PSU_TYPE_LAST = ONLP_PSU_TYPE_DC48,
  ONLP_PSU_TYPE_COUNT,
  ONLP_PSU_TYPE_INVALID = -1
};

typedef struct psu_support_info_s {
    char vendor[ONLP_CONFIG_INFO_STR_MAX];
    char part_num[ONLP_CONFIG_INFO_STR_MAX];
    int type;
    int fan_dir;
} psu_support_info_t;

typedef struct psu_fru_s {
    bool ready;
    char vendor[ONLP_CONFIG_INFO_STR_MAX];
    char model[ONLP_CONFIG_INFO_STR_MAX];
    char part_num[ONLP_CONFIG_INFO_STR_MAX];
    char serial[ONLP_CONFIG_INFO_STR_MAX];
} psu_fru_t;

int psu_present_get(int *present, int local_id);

int psu_pwgood_get(int *pw_good, int local_id);

int psu_fan_info_get(onlp_fan_info_t* info, int local_id);

int exec_cmd(char *cmd, char* out, int size);

int file_read_hex(int* value, const char* fmt, ...);

int file_vread_hex(int* value, const char* fmt, va_list vargs);

int mask_shift(int val, int mask);

void check_and_do_i2c_mux_reset(int port);

int get_hw_rev_id();

int get_gpio_max();

#endif  /* __PLATFORM_LIB_H__ */
