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
#include <unistd.h>
#include <sys/io.h>
#include <onlplib/shlocks.h>
#include <onlp/oids.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "platform_lib.h"

static const warm_reset_data_t warm_reset_data[] = {
    //                     unit_max | dev | unit
    [WARM_RESET_ALL] = {MAC_MAX, "mac", NULL},
    [WARM_RESET_MAC] = {MAC_MAX, "mac", NULL},
    [WARM_RESET_PHY] = {-1, NULL, NULL}, // not support
    [WARM_RESET_MUX] = {-1, NULL, NULL}, // not support
    [WARM_RESET_OP2] = {-1, NULL, NULL}, // not support
    [WARM_RESET_GB] = {-1, NULL, NULL},  // not support
};

/* execute system command */
int exec_cmd(char *cmd, char* out, int size)
{
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

int file_read_hex(int* value, const char* fmt, ...)
{
    int rv;
    va_list vargs;
    va_start(vargs, fmt);
    rv = file_vread_hex(value, fmt, vargs);
    va_end(vargs);
    return rv;
}

int file_vread_hex(int* value, const char* fmt, va_list vargs)
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

int get_shift(int mask)
{
    int mask_one = 1;
    int i=0;

    for(i=0; i<8; ++i) {
        if((mask & mask_one) == 1) {
            return i;
        } else {
            mask >>= 1;
        }
    }

    return -1;
}

int mask_shift(int val, int mask)
{
    return (val & mask) >> get_shift(mask);
}


/*
 * This function check the I2C bus statuas by using i2cget,
 * If the I2C Bus is stcuk, do the i2c mux reset.
 */
void check_and_do_i2c_mux_reset(int port)
{

    char cmd_buf[256] = {0};
    int ret = 0;

    snprintf(cmd_buf, sizeof(cmd_buf), CMD_I2C_STUCK_CHECK);
    ret = system(cmd_buf);
    if(ret != 0) {
        if(access(MUX_RESET_SYSFS, F_OK) != -1 ) {
            snprintf(cmd_buf, sizeof(cmd_buf), "echo 0 > " MUX_RESET_SYSFS " 2> /dev/null");
            ret = system(cmd_buf);
            //AIM_LOG_SYSLOG_WARN("Do I2C mux reset!! (ret=%d)\r\n", ret);
        }
    }
}

/**
 * @brief Get hw re version
 * @param board [out] hw rev id
 */
int get_hw_rev_id()
{
    int hw_rev_id;

    //Get HW Version
    if(onlp_file_read_int(&hw_rev_id, LPC_MB_CPLD_PATH "/" LPC_MB_HW_ID_ATTR) != ONLP_STATUS_OK) {
        hw_rev_id = 0;
    }

    return hw_rev_id;
}

/**
 * @brief init gpio max value
 * @param board [out] gpio max value
 */
int get_gpio_max() {

    int gpio_max;

    //Get HW Version
    if(onlp_file_read_int(&gpio_max, GPIO_MAX_SYSFS) != ONLP_STATUS_OK) {
        gpio_max = 512;
    }

    if(gpio_max < MIN_GPIO_MAX) {
      gpio_max = 512;
      AIM_LOG_ERROR("GPIO_BASE %d is not enough for init, please check kernel config\n", gpio_max);
    }

    return gpio_max;
}

/**
 * @brief warm reset for mac
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

    if (reset_dev >= WARM_RESET_MAX)
    {
        AIM_LOG_ERROR("%s() dev_id(%d) out of range.", __func__, reset_dev);
        return ONLP_STATUS_E_PARAM;
    }

    if (access(WARM_RESET_PATH, F_OK) == -1)
    {
        AIM_LOG_ERROR("%s() file not exist, file=%s", __func__, WARM_RESET_PATH);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (warm_reset_data[reset_dev].warm_reset_dev_str == NULL)
    {
        AIM_LOG_ERROR("%s() reset_dev not support, reset_dev=%d", __func__, reset_dev);
        return ONLP_STATUS_E_PARAM;
    }

    data = &warm_reset_data[reset_dev];

    if (data != NULL && data->warm_reset_dev_str != NULL)
    {
        snprintf(dev_unit_buf, sizeof(dev_unit_buf), "%s", data->warm_reset_dev_str);
        if (data->unit_str != NULL && unit_id < data->unit_max)
        { // assuming unit_max is defined
            snprintf(dev_unit_buf + strlen(dev_unit_buf), sizeof(dev_unit_buf) - strlen(dev_unit_buf),
                     " %s", data->unit_str[unit_id]);
        }
        snprintf(cmd_buf, sizeof(cmd_buf), CMD_WARM_RESET, WARM_RESET_TIMEOUT, dev_unit_buf);
        AIM_LOG_INFO("%s() info, warm reset cmd=%s", __func__, cmd_buf); // TODO
        ret = system(cmd_buf);
    }
    else
    {
        AIM_LOG_ERROR("%s() error, invalid reset_dev %d", __func__, reset_dev);
        return ONLP_STATUS_E_PARAM;
    }

    if (ret != 0)
    {
        AIM_LOG_ERROR("%s() error, please check dmesg error output.", __func__);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ret;
}
