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
#ifndef __PLATFORM_LIB_MAIN_H__
#define __PLATFORM_LIB_MAIN_H__

#include <onlp/onlp.h>
#include <onlplib/file.h>
#include <onlplib/i2c.h>

#define __WEAK __attribute__((weak))

#define ONLP_TRY(_expr)                                                 \
    do {                                                                \
        int _rv = (_expr);                                              \
        if(ONLP_FAILURE(_rv)) {                                         \
            AIM_LOG_ERROR("%s returned %{onlp_status}", #_expr, _rv);   \
            return _rv;                                                 \
        }                                                               \
    } while(0)

#define BSP_PR_REDIRECT_ERR              " 2>>"LPC_BSP_FMT"bsp_pr_err"
#define BSP_PR_REDIRECT_INFO             " 1>>"LPC_BSP_FMT"bsp_pr_info"
#define COMM_STR_NOT_SUPPORTED           "not supported"
#define COMM_STR_NOT_AVAILABLE           "not available"
#define COMM_STR_NA                      "NA"
#define COMM_STR_NS                      "NS"
#define COMM_STR_PSU                     "psu"
#define COMM_STR_QSFPDD                  "QSFPDD"
#define COMM_STR_OSFP                    "OSFP"
#define COMM_STR_QSFP                    "QSFP"
#define COMM_STR_SFP                     "SFP"

#define DEFAULT_PSU_OID_BASE   (1)
#define DEFAULT_TEMP_OID_BASE  (1)
#define DEFAULT_FAN_OID_BASE   (1)
#define DEFAULT_LED_OID_BASE   (1)

/* Warm Reset */
#define WARM_RESET_PATH          "/lib/platform-config/current/onl/warm_reset/warm_reset"
#define WARM_RESET_TIMEOUT       60
#define CMD_WARM_RESET           "timeout %ds %s %s" BSP_PR_REDIRECT_ERR BSP_PR_REDIRECT_INFO
enum reset_dev_type {
    WARM_RESET_ALL = 0,
    WARM_RESET_MAC,
    WARM_RESET_PHY,
    WARM_RESET_MUX,
    WARM_RESET_OP2,
    WARM_RESET_GB,
    WARM_RESET_I210,
    WARM_RESET_RT,
    WARM_RESET_MAX
};

enum mac_unit_id {
     MAC_ALL = 0,
     MAC1_ID,
     MAC_MAX
};

enum i2c_stuck_status {
    I2C_STUCK_STATUS_NORMAL,
    I2C_STUCK_STATUS_ROOT_BUS,
    I2C_STUCK_STATUS_TRANSCEIVER
};

typedef struct board_s
{
    int hw_build;
    int deph_id;
    int hw_rev;
    int rev_id;
}board_t;

typedef struct warm_reset_data_s {
    int unit_max;
    const char *warm_reset_dev_str;
    const char **unit_str;
} warm_reset_data_t;

void lock_init();
int ufi_file_read_longlong(long long int* value, const char* fmt, ...);
int ufi_file_read_int(int* value, const char* fmt, ...);
int ufi_get_board_version(board_t *board);
int exec_cmd(char *cmd, char* out, int size);
void check_and_do_i2c_mux_reset(int port);
uint8_t ufi_shift(uint8_t mask);
uint8_t ufi_mask_shift(uint8_t val, uint8_t mask);
uint8_t ufi_bit_operation(uint8_t reg_val, uint8_t bit, uint8_t bit_val);
int onlp_data_path_reset(uint8_t unit_id, uint8_t reset_dev);
int _onlp_psu_oid_base_get(int *base);
int _onlp_temp_oid_base_get(int *base);
int _onlp_fan_oid_base_get(int *base);
int _onlp_led_oid_base_get(int *base);
#endif  /* __PLATFORM_LIB_MAIN_H__ */
