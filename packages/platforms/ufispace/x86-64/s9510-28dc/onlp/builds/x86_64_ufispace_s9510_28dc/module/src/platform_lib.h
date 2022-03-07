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
#include "x86_64_ufispace_s9510_28dc_log.h"

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

#define SYS_FMT                     "/sys/bus/i2c/devices/%d-%04x/%s"
#define SYS_FMT_OFFSET              "/sys/bus/i2c/devices/%d-%04x/%s_%d"
#define SYS_GPIO_FMT                "/sys/class/gpio/gpio%d/value"
#define LPC_FMT                     "/sys/devices/platform/x86_64_ufispace_s9510_28dc_lpc/mb_cpld/"
#define SYS_CPU_CORETEMP_PREFIX     "/sys/devices/platform/coretemp.0/hwmon/hwmon1/"
#define SYS_CPU_CORETEMP_PREFIX2    "/sys/devices/platform/coretemp.0/"
#define I2C_STUCK_CHECK_CMD         "i2cget -f -y 0 0x76 > /dev/null 2>&1"
#define MUX_RESET_PATH              "/sys/devices/platform/x86_64_ufispace_s9510_28dc_lpc/mb_cpld/mux_reset_all"

#define CPLD_START_ADDR                 0x700
#define CPLD_END_ADDR                   0x790
#define IS_INVALID_CPLD_ADDR(_addr)     (_addr > CPLD_END_ADDR || _addr < CPLD_START_ADDR)

/* BMC CMD */
#define BMC_SENSOR_CACHE            "/tmp/bmc_sensor_cache"
#define IPMITOOL_REDIRECT_FIRST_ERR " 2>/tmp/ipmitool_err_msg"
#define IPMITOOL_REDIRECT_ERR       " 2>>/tmp/ipmitool_err_msg"
#define FAN_CACHE_TIME          10
#define PSU_CACHE_TIME          30
#define THERMAL_CACHE_TIME      10

/*   IPMITOOL_CMD_TIMEOUT get from ipmitool test.
 *   Test Case: Run 100 times of CMD_BMC_SENSOR_CACHE command and 100 times of CMD_FRU_CACHE_SET command and get the execution times.
 *              We take 10s as The IPMITOOL_CMD_TIMEOUT value 
 *              since the CMD_BMC_SENSOR_CACHE execution times value is between 0.216s - 2.926s and
 *                    the CMD_FRU_CACHE_SET execution times value is between 0.031s - 0.076s.
 */

#define IPMITOOL_CMD_TIMEOUT        10
#define CMD_BMC_SENSOR_CACHE        "timeout %ds ipmitool sdr -c get "\
                                    "TEMP_MAC "\
                                    "TEMP_DDR4 "\
                                    "TEMP_BMC "\
                                    "TEMP_FANCARD1 "\
                                    "TEMP_FANCARD2 "\
                                    "TEMP_FPGA_R "\
                                    "TEMP_FPGA_L "\
                                    "HWM_TEMP_GDDR "\
                                    "HWM_TEMP_MAC "\
                                    "HWM_TEMP_AMB "\
                                    "HWM_TEMP_NTMCARD "\
                                    "PSU0_TEMP1 "\
                                    "PSU1_TEMP1 "\
                                    "FAN_0 "\
                                    "FAN_1 "\
                                    "FAN_2 "\
                                    "FAN_3 "\
                                    "FAN_4 "\
                                    "PSU0_FAN "\
                                    "PSU1_FAN "\
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
                                    "> "BMC_SENSOR_CACHE IPMITOOL_REDIRECT_ERR

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

enum sensor
{
    FAN_SENSOR = 0,
    PSU_SENSOR,
    THERMAL_SENSOR,
};

enum bmc_attr_id {
    BMC_ATTR_ID_TEMP_MAC,
    BMC_ATTR_ID_TEMP_DDR4,
    BMC_ATTR_ID_TEMP_BMC,
    BMC_ATTR_ID_TEMP_FANCARD1,
    BMC_ATTR_ID_TEMP_FANCARD2,
    BMC_ATTR_ID_TEMP_FPGA_R,
    BMC_ATTR_ID_TEMP_FPGA_L,
    BMC_ATTR_ID_HWM_TEMP_GDDR,
    BMC_ATTR_ID_HWM_TEMP_MAC,
    BMC_ATTR_ID_HWM_TEMP_AMB,
    BMC_ATTR_ID_HWM_TEMP_NTMCARD,
    BMC_ATTR_ID_PSU0_TEMP,
    BMC_ATTR_ID_PSU1_TEMP,
    BMC_ATTR_ID_FAN_0,
    BMC_ATTR_ID_FAN_1,
    BMC_ATTR_ID_FAN_2,
    BMC_ATTR_ID_FAN_3,
    BMC_ATTR_ID_FAN_4,
    BMC_ATTR_ID_PSU0_FAN,
    BMC_ATTR_ID_PSU1_FAN,
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

/* fan_id */
enum onlp_fan_id {
    ONLP_FAN_0 = 1,
    ONLP_FAN_1,
    ONLP_FAN_2,
    ONLP_FAN_3,
    ONLP_FAN_4,
    ONLP_PSU_0_FAN,
    ONLP_PSU_1_FAN,
    ONLP_FAN_MAX,
};

/* led_id */
enum onlp_led_id {
    ONLP_LED_SYS_GNSS = 1,
    ONLP_LED_SYS_SYNC,
    ONLP_LED_SYS_SYS,
    ONLP_LED_SYS_FAN,
    ONLP_LED_SYS_PWR,
    ONLP_LED_FLEXE_0,
    ONLP_LED_FLEXE_1,
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
    ONLP_THERMAL_CPU_PKG = 1,
    ONLP_THERMAL_CPU_0,
    ONLP_THERMAL_CPU_1,
    ONLP_THERMAL_CPU_2,
    ONLP_THERMAL_CPU_3,
    ONLP_THERMAL_CPU_4,
    ONLP_THERMAL_CPU_5,
    ONLP_THERMAL_CPU_6,
    ONLP_THERMAL_CPU_7,
    ONLP_THERMAL_MAC,
    ONLP_THERMAL_DDR4,
    ONLP_THERMAL_BMC,
    ONLP_THERMAL_FANCARD1,
    ONLP_THERMAL_FANCARD2,
    ONLP_THERMAL_FPGA_R,
    ONLP_THERMAL_FPGA_L,
    ONLP_THERMAL_HWM_GDDR,
    ONLP_THERMAL_HWM_MAC,
    ONLP_THERMAL_HWM_AMB,
    ONLP_THERMAL_HWM_NTMCARD,
    ONLP_THERMAL_PSU_0,
    ONLP_THERMAL_PSU_1,
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

typedef struct bmc_info_s
{
    char name[20];
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

int ufi_read_ioport(unsigned int addr, unsigned char *reg_val);
int ufi_write_ioport(unsigned int addr, unsigned char reg_val);

int exec_cmd(char *cmd, char* out, int size);

int file_read_hex(int* value, const char* fmt, ...);

int file_vread_hex(int* value, const char* fmt, va_list vargs);

void lock_init();

int bmc_check_alive(void);
int bmc_sensor_read(int bmc_cache_index, int sensor_type, float *data);
int bmc_fru_read(int local_id, bmc_fru_t *data);

void check_and_do_i2c_mux_reset(int port);

uint8_t ufi_shift(uint8_t mask);

uint8_t ufi_mask_shift(uint8_t val, uint8_t mask);

uint8_t ufi_bit_operation(uint8_t reg_val, uint8_t bit, uint8_t bit_val);

int get_hw_rev_id(void);

int get_psu_present_status(int local_id, int *pw_present);
int get_psu_type(int local_id, int *psu_type, bmc_fru_t *fru_in);
#endif  /* __PLATFORM_LIB_H__ */
