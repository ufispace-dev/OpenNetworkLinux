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
#include <unistd.h>
#include <sys/io.h>
#include <onlplib/shlocks.h>
#include <onlp/oids.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "platform_lib.h"

/*                                   ALL   UNIT1*/
static const char *mac_unit_str[] = {"",   ""};
static const warm_reset_data_t warm_reset_data[] = {
//                     unit_max | dev | unit
    [WARM_RESET_ALL] = {-1,      "mac", NULL},
    [WARM_RESET_MAC] = {MAC_MAX, "mac", mac_unit_str},
    [WARM_RESET_PHY] = {-1,      NULL, NULL}, //not support
    [WARM_RESET_MUX] = {-1,      NULL, NULL}, //not support
    [WARM_RESET_OP2] = {-1,      NULL, NULL}, //not support
    [WARM_RESET_GB]  = {-1,      NULL, NULL}, //not support
};

static onlp_shlock_t* onlp_lock = NULL;

#define ONLP_LOCK() \
    do{ \
        onlp_shlock_take(onlp_lock); \
    }while(0)

#define ONLP_UNLOCK() \
    do{ \
        onlp_shlock_give(onlp_lock); \
    }while(0)

#define LOCK_MAGIC 0xA2B4C6D8

void init_lock()
{
    static int sem_inited = 0;
    if(!sem_inited) {
        onlp_shlock_create(LOCK_MAGIC, &onlp_lock, "bmc-file-lock");
        sem_inited = 1;
        check_and_do_i2c_mux_reset(-1);
    }
}

/**
 * @brief Get board version
 * @param board [out] board data struct
 */
int get_board_version(board_t *board)
{
    int rv = ONLP_STATUS_OK;

    if(board == NULL) {
        return ONLP_STATUS_E_INVALID;
    }

    //Get HW Version
    if(read_file_hex(&board->hw_rev, LPC_FMT "hw_rev") != ONLP_STATUS_OK ||
        read_file_hex(&board->deph_id, LPC_FMT "deph_id") != ONLP_STATUS_OK ||
        read_file_hex(&board->hw_build, LPC_FMT "build_id") != ONLP_STATUS_OK ||
        read_file_hex(&board->ext_id, LPC_FMT "extend_id") != ONLP_STATUS_OK)
    {
        board->hw_rev = 0;
        board->deph_id = 0;
        board->hw_build = 0;
        board->ext_id = 0;
        rv = ONLP_STATUS_E_INVALID;
    }

    return rv;
}

int check_file_exist(char *file_path, long *file_time)
{
    struct stat file_info;

    if(stat(file_path, &file_info) == 0) {
        if(file_info.st_size == 0) {
            return 0;
        } else {
            *file_time = file_info.st_mtime;
            return 1;
        }
    } else {
        return 0;
    }
}

int exec_cmd(char *cmd, char* out, int size) {
    FILE *fp;

    /* Open the command for reading. */
    fp = popen(cmd, "r");
    if(fp == NULL) {
        AIM_LOG_ERROR("Failed to run command %s\n", cmd );
        return ONLP_STATUS_E_INTERNAL;
    }

    /* Read the output a line at a time - output it. */
    while (fgets(out, size-1, fp) != NULL) {
    }

    /* close */
    pclose(fp);

    return ONLP_STATUS_OK;
}

int read_file_hex(int* value, const char* fmt, ...)
{
    int rv;
    va_list vargs;
    va_start(vargs, fmt);
    rv = vread_file_hex(value, fmt, vargs);
    va_end(vargs);
    return rv;
}

int vread_file_hex(int* value, const char* fmt, va_list vargs)
{
    int rv;
    uint8_t data[32];
    int len;
    rv = onlp_file_vread(data, sizeof(data), &len, fmt, vargs);
    if(rv < 0) {
        return rv;
    }
    //hex to int
    *value = (int) strtol((char *)data, NULL, 0);
    return 0;
}

/*
 * This function check the I2C bus statuas by using the sysfs of cpld_id,
 * If the I2C Bus is stcuk, do the i2c mux reset.
 */
void check_and_do_i2c_mux_reset(int port)
{
    // only support beta and later
    if(get_hw_rev_id() >= BRD_BETA) {
        char cmd_buf[256] = {0};
        int ret = 0;

        snprintf(cmd_buf, sizeof(cmd_buf), I2C_STUCK_CHECK_CMD);
        ret = system(cmd_buf);
        if(ret != 0) {
            if(access(MUX_RESET_PATH, F_OK) != -1 ) {
                memset(cmd_buf, 0, sizeof(cmd_buf));
                snprintf(cmd_buf, sizeof(cmd_buf), "echo 0 > %s 2> /dev/null", MUX_RESET_PATH);
                ret = system(cmd_buf);
            }
        }
    }
}

int get_hw_rev_id(void)
{
    int hw_rev;
    char hw_rev_cmd[128];
    char buffer[128];
    FILE *fp;

    snprintf(hw_rev_cmd, sizeof(hw_rev_cmd), "cat "LPC_FMT "hw_rev");
    fp = popen(hw_rev_cmd, "r");
    if(fp == NULL) {
        AIM_LOG_ERROR("Unable to popen cmd(%s)\r\n", hw_rev_cmd);
        return ONLP_STATUS_E_INTERNAL;
    }
    /* Read the output a line at a time - output it. */
    fgets(buffer, sizeof(buffer), fp);
    hw_rev = atoi(buffer);

    pclose(fp);

    return hw_rev;
}

int get_thermal_thld(int thermal_local_id,  temp_thld_t *temp_thld)
{
    int psu_type = ONLP_PSU_TYPE_INVALID;

    if(temp_thld == NULL) {
        return ONLP_STATUS_E_INVALID;
    }

    ONLP_TRY(get_psu_type(&psu_type));

    /*
    if(psu_type == ONLP_PSU_TYPE_AC) {
        switch(thermal_local_id) {
            case ONLP_THERMAL_CPU_PKG:
                temp_thld->warning = THERMAL_CPU_WARNING;
                temp_thld->error = THERMAL_CPU_ERROR;
                temp_thld->shutdown = THERMAL_SHUTDOWN_DEFAULT;
            case ONLP_THERMAL_MAINBOARD_MAC:
                temp_thld->warning = THERMAL_MAC_WARNING;
                temp_thld->error = THERMAL_MAC_ERROR;
                temp_thld->shutdown = THERMAL_MAC_SHUTDOWN;
            case ONLP_THERMAL_MAINBOARD_GDDR6:
                temp_thld->warning = THERMAL_GDDR6_WARNING;
                temp_thld->error = THERMAL_GDDR6_ERROR;
                temp_thld->shutdown = THERMAL_SHUTDOWN_DEFAULT;
            case ONLP_THERMAL_MAINBOARD_NTM:
                temp_thld->warning = THERMAL_NTM_WARNING;
                temp_thld->error = THERMAL_NTM_ERROR;
                temp_thld->shutdown = THERMAL_SHUTDOWN_DEFAULT;
            case ONLP_THERMAL_VMON_HWM_MAC:
                temp_thld->warning = THERMAL_HWM_MAC_WARNING;
                temp_thld->error = THERMAL_HWM_MAC_ERROR;
                temp_thld->shutdown = THERMAL_SHUTDOWN_DEFAULT;
            case ONLP_THERMAL_VMON_HWM_AMB:
                temp_thld->warning = THERMAL_HWM_AMB_WARNING;
                temp_thld->error = THERMAL_HWM_AMB_ERROR;
                temp_thld->shutdown = THERMAL_SHUTDOWN_DEFAULT;
            case ONLP_THERMAL_VMON_HWM_PHY:
                temp_thld->warning = THERMAL_HWM_PHY_WARNING;
                temp_thld->error = THERMAL_HWM_PHY_ERROR;
                temp_thld->shutdown = THERMAL_SHUTDOWN_DEFAULT;
            case ONLP_THERMAL_PSU_0:
                temp_thld->warning = THERMAL_PSU_TEMP1_WARNING_AC;
                temp_thld->error = THERMAL_PSU_TEMP1_ERROR;
                temp_thld->shutdown = THERMAL_SHUTDOWN_DEFAULT;
            case ONLP_THERMAL_PSU_1:
                temp_thld->warning = THERMAL_PSU_TEMP1_WARNING_AC;
                temp_thld->error = THERMAL_PSU_TEMP1_ERROR;
                temp_thld->shutdown = THERMAL_SHUTDOWN_DEFAULT;
            default:
                break;
        }

        return ONLP_STATUS_OK;
    }
    else if(psu_type == ONLP_PSU_TYPE_DC48) {
        switch(thermal_local_id) {
            case ONLP_THERMAL_CPU_PKG:
                temp_thld->warning = THERMAL_CPU_WARNING;
                temp_thld->error = THERMAL_CPU_ERROR;
                temp_thld->shutdown = THERMAL_CPU_SHUTDOWN;
            case ONLP_THERMAL_MAINBOARD_MAC:
                temp_thld->warning = THERMAL_MAC_WARNING;
                temp_thld->error = THERMAL_MAC_ERROR;
                temp_thld->shutdown = THERMAL_MAC_SHUTDOWN;
            case ONLP_THERMAL_MAINBOARD_GDDR6:
                temp_thld->warning = THERMAL_GDDR6_WARNING;
                temp_thld->error = THERMAL_GDDR6_ERROR;
                temp_thld->shutdown = THERMAL_SHUTDOWN_DEFAULT;
            case ONLP_THERMAL_MAINBOARD_NTM:
                temp_thld->warning = THERMAL_NTM_WARNING;
                temp_thld->error = THERMAL_NTM_ERROR;
                temp_thld->shutdown = THERMAL_SHUTDOWN_DEFAULT;
            case ONLP_THERMAL_VMON_HWM_MAC:
                temp_thld->warning = THERMAL_HWM_MAC_WARNING;
                temp_thld->error = THERMAL_HWM_MAC_ERROR;
                temp_thld->shutdown = THERMAL_SHUTDOWN_DEFAULT;
            case ONLP_THERMAL_VMON_HWM_AMB:
                temp_thld->warning = THERMAL_HWM_AMB_WARNING;
                temp_thld->error = THERMAL_HWM_AMB_ERROR;
                temp_thld->shutdown = THERMAL_SHUTDOWN_DEFAULT;
            case ONLP_THERMAL_VMON_HWM_PHY:
                temp_thld->warning = THERMAL_PSU_TEMP1_WARNING_DC;
                temp_thld->error = THERMAL_PSU_TEMP1_ERROR;
                temp_thld->shutdown = THERMAL_SHUTDOWN_DEFAULT;
            default:
                break;
        }

        return ONLP_STATUS_OK;
    }
    else {
        return ONLP_STATUS_E_INTERNAL;
    }
    */

    switch(thermal_local_id) {
        case ONLP_THERMAL_CPU_PKG:
            temp_thld->warning = THERMAL_CPU_WARNING;
            temp_thld->error = THERMAL_CPU_ERROR;
            temp_thld->shutdown = THERMAL_CPU_SHUTDOWN;
            break;
        case ONLP_THERMAL_MAINBOARD_MAC:
            temp_thld->warning = THERMAL_MAC_WARNING;
            temp_thld->error = THERMAL_MAC_ERROR;
            temp_thld->shutdown = THERMAL_MAC_SHUTDOWN;
            break;
        case ONLP_THERMAL_MAINBOARD_GDDR6:
            temp_thld->warning = THERMAL_GDDR6_WARNING;
            temp_thld->error = THERMAL_GDDR6_ERROR;
            temp_thld->shutdown = THERMAL_SHUTDOWN_DEFAULT;
            break;
        case ONLP_THERMAL_MAINBOARD_NTM:
            temp_thld->warning = THERMAL_NTM_WARNING;
            temp_thld->error = THERMAL_NTM_ERROR;
            temp_thld->shutdown = THERMAL_SHUTDOWN_DEFAULT;
            break;
        case ONLP_THERMAL_VMON_HWM_MAC:
            temp_thld->warning = THERMAL_HWM_MAC_WARNING;
            temp_thld->error = THERMAL_HWM_MAC_ERROR;
            temp_thld->shutdown = THERMAL_HWM_MAC_SHUTDOWN;
            break;
        case ONLP_THERMAL_VMON_HWM_AMB:
            temp_thld->warning = THERMAL_HWM_AMB_WARNING;
            temp_thld->error = THERMAL_HWM_AMB_ERROR;
            temp_thld->shutdown = THERMAL_SHUTDOWN_DEFAULT;
            break;
        case ONLP_THERMAL_VMON_HWM_PHY:
            temp_thld->warning = THERMAL_HWM_PHY_WARNING;
            temp_thld->error = THERMAL_HWM_PHY_ERROR;
            temp_thld->shutdown = THERMAL_HWM_PHY_SHUTDOWN;
            break;
        case ONLP_THERMAL_PSU_0:
            if(psu_type == ONLP_PSU_TYPE_AC) {
                temp_thld->warning = THERMAL_PSU_TEMP1_WARNING_AC;
                temp_thld->error = THERMAL_PSU_TEMP1_ERROR_AC;
                temp_thld->shutdown = THERMAL_PSU_SHUTDOWN_AC;
            } else if(psu_type == ONLP_PSU_TYPE_DC48) {
                temp_thld->warning = THERMAL_PSU_TEMP1_WARNING_DC;
                temp_thld->error = THERMAL_PSU_TEMP1_ERROR_DC;
                temp_thld->shutdown = THERMAL_PSU_SHUTDOWN_DC;
            } else {
                return ONLP_STATUS_E_INVALID;
            }
            break;
        case ONLP_THERMAL_PSU_1:
            if(psu_type == ONLP_PSU_TYPE_AC) {
                temp_thld->warning = THERMAL_PSU_TEMP1_WARNING_AC;
                temp_thld->error = THERMAL_PSU_TEMP1_ERROR_AC;
                temp_thld->shutdown = THERMAL_PSU_SHUTDOWN_AC;
            } else {
                return ONLP_STATUS_E_INVALID;
            }
            break;
        default:
            AIM_LOG_ERROR("%s() unknown thermal_local_id %d\n",__func__, thermal_local_id);
            break;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief warm reset for mac, phy, mux and op2
 * @param unit_id The warm reset device unit id
 * @param reset_dev The warm reset device id
 * @param ret return value.
 */
int onlp_data_path_reset(uint8_t unit_id, uint8_t reset_dev)
{
    char cmd_buf[256] = {0};
    char dev_unit_buf[32] = {0};
    const warm_reset_data_t *data = NULL;
    int ret = 0;

    if (reset_dev >= WARM_RESET_MAX) {
        AIM_LOG_ERROR("%s() dev_id(%d) out of range.", __func__, reset_dev);
        return ONLP_STATUS_E_PARAM;
    }

    if(access(WARM_RESET_PATH, F_OK) == -1) {
        AIM_LOG_ERROR("%s() file not exist, file=%s", __func__, WARM_RESET_PATH);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (warm_reset_data[reset_dev].warm_reset_dev_str == NULL) {
        AIM_LOG_ERROR("%s() reset_dev not support, reset_dev=%d", __func__, reset_dev);
        return ONLP_STATUS_E_PARAM;
    }

    data = &warm_reset_data[reset_dev];

    if (data != NULL && data->warm_reset_dev_str != NULL) {
        snprintf(dev_unit_buf, sizeof(dev_unit_buf), "%s", data->warm_reset_dev_str);
        if (data->unit_str != NULL && unit_id < data->unit_max) {  // assuming unit_max is defined
            snprintf(dev_unit_buf + strlen(dev_unit_buf), sizeof(dev_unit_buf) - strlen(dev_unit_buf),
                     " %s", data->unit_str[unit_id]);
        }
        snprintf(cmd_buf, sizeof(cmd_buf), CMD_WARM_RESET, WARM_RESET_TIMEOUT, dev_unit_buf);
        AIM_LOG_INFO("%s() info, warm reset cmd=%s", __func__, cmd_buf); //TODO
        ret = system(cmd_buf);
    } else {
        AIM_LOG_ERROR("%s() error, invalid reset_dev %d", __func__, reset_dev);
        return ONLP_STATUS_E_PARAM;
    }

    if (ret != 0) {
        AIM_LOG_ERROR("%s() error, please check dmesg error output.", __func__);
        return ONLP_STATUS_E_INTERNAL;
    }


    return ret;
}

///* two's complement to decimal */
//int _two_complement(int num, int bit)
//{
//    unsigned int extend = 0x0;
//    unsigned int data_bits = 0x0;
//
//    int i = 0;
//    for(i=0; i<bit; i++)
//        data_bits |= (1<<i);
//
//    extend = (~data_bits);
//
//    return (num & (1<<bit)) ? (num | extend) : num;
//}
//
//int pmbus_linear_data_format(unsigned int word_data)
//{
//    int bit_size_exp = 5;
//    int bit_size_mantissa = 11;
//    int exp = 0;
//    int mantissa = 0;
//    long long value = 0;
//
//    //fan_rpm = (unsigned int)tmp_fan_rpm;
//    //fan_rpm = (fan_rpm & 0x07FF) * (1 << ((fan_rpm >> 11) & 0x1F));
//
//    exp = _two_complement((word_data >> bit_size_mantissa) & 0x1F, bit_size_exp-1);
//    mantissa = _two_complement(word_data & 0x07FF, bit_size_mantissa-1);
//
//    // ref pmbus 1.1, 7.1 Linear data format(X=Y*2^N) and 8.3.1 Linear mode(Voltage=V*2^N).
//    // value unit is milli.
//    value = (exp>=0) ? (signed long long) mantissa * 1000 * (1 << exp) : (signed long long) mantissa * 1000 / (1 << (0-exp));
//
//    return value;
//}
