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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <onlp/onlp.h>
#include <onlplib/file.h>
#include <onlplib/i2c.h>
#include "x86_64_ufispace_s9511_20ct_log.h"

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
#define LPC_FMT                     "/sys/devices/platform/x86_64_ufispace_s9511_20ct_lpc/mb_cpld/"
#define LPC_BSP_FMT                 "/sys/devices/platform/x86_64_ufispace_s9511_20ct_lpc/bsp/"

#define SYS_CPU_CORETEMP_PREFIX     "/sys/devices/platform/coretemp.0/hwmon/hwmon1/"
#define SYS_CPU_CORETEMP_PREFIX2    "/sys/devices/platform/coretemp.0/"
#define SYS_HWM_PREFIX              "/sys/class/hwmon/hwmon%d/"
#define HWM_TMP75_INDEX             (2)
#define HWM_TMP451_INDEX            (3)

#define I2C_STUCK_CHECK_CMD         "i2cget -f -y 0 0x76 > /dev/null 2>&1"
#define MUX_RESET_PATH              "/sys/devices/platform/x86_64_ufispace_s9511_20ct_lpc/mb_cpld/mux_reset_all"
#define BSP_PR_REDIRECT_ERR         " 2>>"LPC_BSP_FMT"bsp_pr_err"
#define BSP_PR_REDIRECT_INFO        " 1>>"LPC_BSP_FMT"bsp_pr_info"
#define SYSFS_DEVICES               "/sys/bus/i2c/devices/"
#define SYSFS_CPLD1                 SYSFS_DEVICES "10-0033/"
#define SYSFS_CPLD2                 SYSFS_DEVICES "6-0027/"
#define PSU0_EEPROM_PATH            "/sys/bus/i2c/devices/13-0051/eeprom"
#define PSU1_EEPROM_PATH            "/sys/bus/i2c/devices/13-0053/eeprom"

#define PSU_TYPE_FILE_PATH          "/tmp/psu_type"

/* I2C Bus */
#define I2C_BUS_13                       (13)
#define I2C_BUS_PSU0                     (I2C_BUS_13)      /* PSU0 */
#define I2C_BUS_PSU1                     (I2C_BUS_13)      /* PSU1 */


/* Thermal threshold */
#define THERMAL_WARNING_DEFAULT          (77)
#define THERMAL_ERROR_DEFAULT            (95)

#define THERMAL_MAC_WARNING              (85)
#define THERMAL_MAC_ERROR                (100)
#define THERMAL_MAC_SHUTDOWN             (110)

#define THERMAL_SHUTDOWN_DEFAULT         (THERMAL_MAC_SHUTDOWN)

#define THERMAL_CPU_WARNING              (THERMAL_WARNING_DEFAULT)
#define THERMAL_CPU_ERROR                (THERMAL_ERROR_DEFAULT)
#define THERMAL_CPU_SHUTDOWN             (THERMAL_SHUTDOWN_DEFAULT)

#define THERMAL_MAC_HWM_WARNING          (95)
#define THERMAL_MAC_HWM_ERROR            (105)
#define THERMAL_MAC_HWM_SHUTDOWN         (110)
#define THERMAL_GDDR6_WARNING            (95)
#define THERMAL_GDDR6_ERROR              (105)
#define THERMAL_NTM_WARNING              (95)
#define THERMAL_NTM_ERROR                (105)
#define THERMAL_HWM_MAC_WARNING          (100)
#define THERMAL_HWM_MAC_ERROR            (110)
#define THERMAL_HWM_MAC_SHUTDOWN         (0)
#define THERMAL_HWM_AMB_WARNING          (90)
#define THERMAL_HWM_AMB_ERROR            (100)
#define THERMAL_HWM_PHY_WARNING          (100)
#define THERMAL_HWM_PHY_ERROR            (110)
#define THERMAL_HWM_PHY_SHUTDOWN         (0)
#define THERMAL_PSU_TEMP1_WARNING_AC     (0)
#define THERMAL_PSU_TEMP1_WARNING_DC     (0)
#define THERMAL_PSU_TEMP1_ERROR_AC       (78)
#define THERMAL_PSU_TEMP1_ERROR_DC       (75)
#define THERMAL_PSU_SHUTDOWN_AC          (83)
#define THERMAL_PSU_SHUTDOWN_DC          (78)
#define THERMAL_STATE_NOT_SUPPORT        (0)

/* PSU */
#define PSU_STATUS_ABS                   (0)
#define PSU_STATUS_PRES                  (1)
#define PSU0_ADDRESS                     (0x59)
#define PSU1_ADDRESS                     (0x5B)

/* PMBUS Page Number for DC PSU */
#define DC_PSU_MAIN_PW                   (0x00)
#define DC_PSU_SECOND_PW                 (0x20)

/* PMBUS Command Code */
#define PMBUS_PAGE_COMMAND               (0x00)
#define PMBUS_VOUT_MODE_BYTE             (0x20)
#define PMBUS_READ_VOUT_WORD             (0x8B)
#define PMBUS_READ_IOUT                  (0x8C)
#define PMBUS_READ_POUT                  (0x96)
#define PMBUS_READ_PIN                   (0x97)
#define PMBUS_READ_VIN                   (0x88)
#define PMBUS_READ_IIN                   (0x89)
#define PMBUS_READ_THERMAL1              (0x8D)
#define PMBUS_PSU_FAN_RPM                (0x90)

/* CPLD Fan PWM RPM */
#define FAN_RPM_INVALID_VAL              (999999)

/* CPU core-temp sysfs ID */
#define CPU_PKG_CORE_TEMP_SYS_ID  "1"

enum sensor
{
    FAN_SENSOR = 0,
    PSU_SENSOR,
    THERMAL_SENSOR,
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
    WARM_RESET_MAX
};

enum mac_unit_id {
     MAC_ALL = 0,
     MAC1_ID,
     MAC_MAX
};

/* fan_id */
enum onlp_fan_id {
    ONLP_FAN_0 = 1,
    ONLP_FAN_1,
    ONLP_FAN_2,
    ONLP_FAN_3, /* After Beta */
    ONLP_PSU_0_FAN,
    ONLP_PSU_1_FAN,
    ONLP_FAN_MAX,
};

/* led_id */
enum onlp_led_id {
    ONLP_LED_SYS_GNSS = 1,
    ONLP_LED_SYS_SYNC,
    ONLP_LED_SYS_STAT,
    ONLP_LED_SYS_FAN,
    ONLP_LED_SYS_PWR,
    ONLP_LED_MAX
};

/* psu_id */
enum onlp_ac_psu_id {
    ONLP_PSU_0 = 1,
    ONLP_PSU_1,

    ONLP_AC_PSU_MAX,
};

enum onlp_dc_psu_id {
    ONLP_DUAL_PSU_0_PLUG_0 = 1,
    ONLP_DUAL_PSU_0_PLUG_1,
    ONLP_DC_PSU_MAX,
};

/* thermal_id */
enum onlp_thermal_id {
    ONLP_THERMAL_CPU_PKG = 1,
    ONLP_THERMAL_MAINBOARD_MAC,
    ONLP_THERMAL_MAINBOARD_GDDR6,
    ONLP_THERMAL_MAINBOARD_NTM,
    ONLP_THERMAL_VMON_HWM_MAC,  //CPLD 0xAD
    ONLP_THERMAL_VMON_HWM_AMB,  //CPLD 0xA2
    ONLP_THERMAL_VMON_HWM_PHY,  //CPLD 0xA3
    ONLP_THERMAL_PSU_0,
    ONLP_THERMAL_PSU_1,
    ONLP_THERMAL_MAX,
};

/** onlp_psu_type */
typedef enum onlp_psu_type_e {
    ONLP_PSU_TYPE_DC48,
    ONLP_PSU_TYPE_AC,
    ONLP_PSU_TYPE_DC12,
    ONLP_PSU_TYPE_LAST = ONLP_PSU_TYPE_DC12,
    ONLP_PSU_TYPE_COUNT,
    ONLP_PSU_TYPE_INVALID = -1,
} onlp_psu_type_t;

typedef enum brd_rev_id_e {
    BRD_PROTO,
    BRD_ALPHA,
    BRD_BETA,
    BRD_PVT,
} brd_rev_id_t;

enum hw_plat
{
    HW_PLAT_PROTO     = 0x1,
    HW_PLAT_ALPHA     = 0x2,
    HW_PLAT_BETA      = 0x4,
    HW_PLAT_PVT       = 0x8,
    HW_PLAT_ALL       = 0xf,
};

typedef struct board_s
{
    int hw_build;
    int deph_id;
    int hw_rev;
    int ext_id;
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
void check_and_do_i2c_mux_reset(int port);
int get_hw_rev_id(void);
int get_psu_type(int *psu_type);
int get_psu_present_status(int psu_type, int local_id, int *pw_present);
int get_psu_pwgood_status(int psu_type, int local_id, int *pw_good);
int get_board_version(board_t *board);
int get_thermal_thld(int thermal_local_id, temp_thld_t *temp_thld);
int onlp_data_path_reset(uint8_t unit_id, uint8_t reset_dev);
//signed int _two_complement(signed int num, unsigned int bit);

#endif  /* __PLATFORM_LIB_H__ */
