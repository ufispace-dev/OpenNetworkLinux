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
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/io.h>
#include <onlp/onlp.h>
#include <onlplib/file.h>
#include <onlplib/i2c.h>
#include <onlplib/shlocks.h>
#include <AIM/aim.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "platform_lib.h"

const int CPLD_BASE_ADDR[] = {0x30, 0x31, 0x32, 0x33, 0x34};

const char * thermal_id_str[] = {
    "",
    "TEMP_CPU_PECI",
    "TEMP_Q2CL_ENV",
    "TEMP_Q2CL_DIE",
    "TEMP_Q2CR_ENV",
    "TEMP_Q2CR_DIE",
    "PSU0_TEMP",
    "PSU1_TEMP",
    "CPU_PACKAGE",
    "CPU1",
    "CPU2",
    "CPU3",
    "CPU4",
    "CPU5",
    "CPU6",
    "CPU7",
    "CPU8",
    "CPU_BOARD",
    "TEMP_BMC_ENV",
    "TEMP_REAR_ENV_1",
    "TEMP_REAR_ENV_2",
    "TEMP_FRONT_ENV",
};

const char * fan_id_str[] = {
    "",
    "FAN0_RPM",
    "FAN1_RPM",
    "FAN2_RPM",
    "FAN3_RPM",
    "PSU0_FAN",
    "PSU1_FAN",
};

const char * fan_id_presence_str[] = {
    "",
    "FAN0_PRSNT_H",
    "FAN1_PRSNT_H",
    "FAN2_PRSNT_H",
    "FAN3_PRSNT_H",
};

const char * psu_id_str[] = {
    "",
    "PSU0",
    "PSU1",
    "PSU0_VIN",
    "PSU0_VOUT",
    "PSU0_IIN",
    "PSU0_IOUT",
    "PSU0_STBVOUT",
    "PSU0_STBIOUT",
    "PSU1_VIN",
    "PSU1_VOUT",
    "PSU1_IIN",
    "PSU1_IOUT",
    "PSU1_STBVOUT",
    "PSU1_STBIOUT",
};

bmc_info_t bmc_cache[] =
{
    {"TEMP_CPU_PECI", 0},
    {"TEMP_Q2CL_ENV", 0},
    {"TEMP_Q2CL_DIE", 0},
    {"TEMP_Q2CR_ENV", 0},
    {"TEMP_Q2CR_DIE", 0},
    {"TEMP_REAR_ENV_1", 0},
    {"TEMP_REAR_ENV_2", 0},
    {"PSU0_TEMP", 0},
    {"PSU1_TEMP", 0},
    {"FAN0_RPM", 0},
    {"FAN1_RPM", 0},
    {"FAN2_RPM", 0},
    {"FAN3_RPM", 0},
    {"PSU0_FAN", 0},
    {"PSU1_FAN", 0},
    {"FAN0_PRSNT_H",0},
    {"FAN1_PRSNT_H",0},
    {"FAN2_PRSNT_H", 0},
    {"FAN3_PRSNT_H", 0},
    {"PSU0_VIN", 0},
    {"PSU0_VOUT", 0},
    {"PSU0_IIN",0},
    {"PSU0_IOUT",0},
    {"PSU0_STBVOUT", 0},
    {"PSU0_STBIOUT", 0},
    {"PSU1_VIN", 0},
    {"PSU1_VOUT", 0},
    {"PSU1_IIN", 0},
    {"PSU1_IOUT", 0},
    {"PSU1_STBVOUT", 0},
    {"PSU1_STBIOUT", 0}
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

void lock_init()
{
    static int sem_inited = 0;
    if(!sem_inited)
    {
        onlp_shlock_create(LOCK_MAGIC, &onlp_lock, "bmc-file-lock");
        sem_inited = 1;
        check_and_do_i2c_mux_reset(-1);
    }
}

int get_shift(int mask) {
    int mask_one = 1;
    int i=0;

    for( i=0; i<8; ++i) {
        if ((mask & mask_one) == 1) {
            return i;
        } else {
            mask >>= 1;
        }
    }

    return -1;
}

int mask_shift(int val, int mask) {
    return (val & mask) >> get_shift(mask);
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

int get_board_id(void)
{
    int board_id, hw_rev, deph_id, hw_build_id;
    char board_id_cmd[128];
    char buffer[128];
    FILE *fp;

    snprintf(board_id_cmd, sizeof(board_id_cmd), "cat /sys/devices/platform/x86_64_ufispace_s9600_64x_lpc/mb_cpld/board_id_1");
    fp = popen(board_id_cmd, "r");
    if (fp == NULL) {
        AIM_LOG_ERROR("Unable to popen cmd(%s)\r\n", board_id_cmd);
        return ONLP_STATUS_E_INTERNAL;
    }
    /* Read the output a line at a time - output it. */
    fgets(buffer, sizeof(buffer), fp);
    board_id = atoi(buffer);

    hw_rev = (board_id & 0b00000011);
    deph_id = (board_id & 0b00000100) >> 2;
    hw_build_id = (deph_id << 2) + hw_rev;

    pclose(fp);

    return hw_build_id;
}

int bmc_cache_expired_check(long last_time, long new_time, int cache_time)
{
    int bmc_cache_expired = 0;

    if(last_time == 0) {
        bmc_cache_expired = 1;
    }
    else {
         if(new_time > last_time) {
             if((new_time - last_time) > cache_time) {
                bmc_cache_expired = 1;
             }
             else {
                bmc_cache_expired = 0;
             }
         }
         else if(new_time == last_time) {
             bmc_cache_expired = 0;
         }
         else {
             bmc_cache_expired = 1;
         }
    }

    return bmc_cache_expired;
}

int bmc_sensor_read(int bmc_cache_index, int sensor_type, float *data)
{
    struct timeval new_tv;
    FILE *fp = NULL;
    char ipmi_cmd[1024] = {0};
    char get_data_cmd[120] = {0};
    char buf[20];
    int rv = ONLP_STATUS_OK;
    int dev_num = 0;
    int dev_size = sizeof(bmc_cache)/sizeof(bmc_cache[0]);
    int cache_time = 0;
    int bmc_cache_expired = 0;
    float f_rv = 0;
    long file_last_time = 0;
    static long bmc_cache_time = 0;
    char* presence_str = "Present";
    int retry = 0, retry_max = 3;

    switch(sensor_type) {
        case FAN_SENSOR:
            cache_time = FAN_CACHE_TIME;
            break;
        case PSU_SENSOR:
            cache_time = PSU_CACHE_TIME;
            break;
        case THERMAL_SENSOR:
            cache_time = THERMAL_CACHE_TIME;
            break;
    }

    if(check_file_exist(BMC_SENSOR_CACHE, &file_last_time))
    {
        gettimeofday(&new_tv, NULL);
        if(bmc_cache_expired_check(file_last_time, new_tv.tv_sec, cache_time)) {
            bmc_cache_expired = 1;
        } else {
            bmc_cache_expired = 0;
        }
    } else {
        bmc_cache_expired = 1;
    }

    if(bmc_cache_time == 0 && check_file_exist(BMC_SENSOR_CACHE, &file_last_time)) {
        bmc_cache_expired = 1;
        gettimeofday(&new_tv,NULL);
        bmc_cache_time = new_tv.tv_sec;
    }

    //update cache
    if(bmc_cache_expired == 1)
    {
        ONLP_LOCK();
        if(bmc_cache_expired_check(file_last_time, bmc_cache_time, cache_time)) {
            snprintf(ipmi_cmd, sizeof(ipmi_cmd), CMD_BMC_SENSOR_CACHE);
            for (retry = 0; retry < retry_max; ++retry) {
                if ((rv=system(ipmi_cmd)) != ONLP_STATUS_OK) {
                    if (retry == retry_max-1) {
                        AIM_LOG_ERROR("%s() write bmc sensor cache failed, retry=%d, cmd=%s, ret=%d",
                            __func__, retry, ipmi_cmd, rv);
                        return ONLP_STATUS_E_INTERNAL;
                    } else {
                        continue;
                    }
                } else {
                    break;
                }
            }
        }

        for(dev_num = 0; dev_num < dev_size; dev_num++)
        {
            memset(buf, 0, sizeof(buf));

            if( dev_num >= 15 && dev_num <=18 ) {
                snprintf(get_data_cmd, sizeof(get_data_cmd), CMD_BMC_CACHE_GET, bmc_cache[dev_num].name, 5);
                fp = popen(get_data_cmd, "r");
                if(fp != NULL)
                {
                    if(fgets(buf, sizeof(buf), fp) != NULL)
                    {
                        if( strstr(buf, presence_str) != NULL ) {
                            f_rv = 1;
                        } else {
                            f_rv = 0;
                        }
                        bmc_cache[dev_num].data = f_rv;
                    }
                }
                pclose(fp);
            } else {
                snprintf(get_data_cmd, sizeof(get_data_cmd), CMD_BMC_CACHE_GET, bmc_cache[dev_num].name, 2);

                fp = popen(get_data_cmd, "r");
                if(fp != NULL)
                {
                    if(fgets(buf, sizeof(buf), fp) != NULL) {
                        f_rv = atof(buf);
                        bmc_cache[dev_num].data = f_rv;
                    }
                }
                pclose(fp);
            }

        }
        gettimeofday(&new_tv,NULL);
        bmc_cache_time = new_tv.tv_sec;
        ONLP_UNLOCK();
    }

    //read from cache
    *data = bmc_cache[bmc_cache_index].data;

    return rv;
}

int
psu_thermal_get(onlp_thermal_info_t* info, int thermal_id)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
psu_fan_info_get(onlp_fan_info_t* info, int id)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
psu_vin_get(onlp_psu_info_t* info, int id)
{
    int i=0, token_idx=1, token_val=0;

    char cmd[48];
    char cmd_out[150];
    char* tokens[20];
    char delimiter[]=",";
    const char* sensor_str;

    memset(cmd, 0, sizeof(cmd));
    memset(cmd_out, 0, sizeof(cmd_out));

    sensor_str = (id == PSU_ID_PSU0 ? psu_id_str[PSU_ID_PSU0_VIN] : psu_id_str[PSU_ID_PSU1_VIN]);
    snprintf(cmd, sizeof(cmd), CMD_BMC_SDR_GET, sensor_str);

    //Get sensor info from BMC
    if (exec_cmd(cmd, cmd_out, sizeof(cmd_out)) < 0) {
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%s\n", sensor_str);
        return ONLP_STATUS_E_INTERNAL;
    }

    //Check output is correct
    if (strnlen(cmd_out, sizeof(cmd_out))==0 ||
        strchr(cmd_out, ',')==NULL ||
        strstr(cmd_out, sensor_str)==NULL ){
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%s, cmd=%s, out=%s\n", sensor_str, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    //parse cmd_out to tokens
    tokens[i++] = strtok(cmd_out, delimiter);
    while (tokens[i-1] != NULL) {
        tokens[i++] = strtok(NULL, delimiter);
    }

    //read token_idx field
    if (i>=token_idx) {
        token_val = (int) (atof(tokens[token_idx])*1000);
        info->mvin = token_val;
        info->caps |= ONLP_PSU_CAPS_VIN;
    } else {
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%s, cmd=%s, out=%s\n", sensor_str, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int
psu_vout_get(onlp_psu_info_t* info, int id)
{
    int i=0, token_idx=1, token_val=0;

    char cmd[48];
    char cmd_out[150];
    char* tokens[20];
    char delimiter[]=",";
    const char* sensor_str;

    memset(cmd, 0, sizeof(cmd));
    memset(cmd_out, 0, sizeof(cmd_out));

    sensor_str = (id == PSU_ID_PSU0 ? psu_id_str[PSU_ID_PSU0_VOUT] : psu_id_str[PSU_ID_PSU1_VOUT]);
    snprintf(cmd, sizeof(cmd), CMD_BMC_SDR_GET, sensor_str);

    //Get sensor info from BMC
    if (exec_cmd(cmd, cmd_out, sizeof(cmd_out)) < 0) {
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%s\n", sensor_str);
        return ONLP_STATUS_E_INTERNAL;
    }

    //Check output is correct
    if (strnlen(cmd_out, sizeof(cmd_out))==0 ||
        strchr(cmd_out, ',')==NULL ||
        strstr(cmd_out, sensor_str)==NULL ){
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%s, cmd=%s, out=%s\n", sensor_str, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    //parse cmd_out to tokens
    tokens[i++] = strtok(cmd_out, delimiter);
    while (tokens[i-1] != NULL) {
        tokens[i++] = strtok(NULL, delimiter);
    }

    //read token_idx field
    if (i>=token_idx) {
        token_val = (int) (atof(tokens[token_idx])*1000);
        info->mvout = token_val;
        info->caps |= ONLP_PSU_CAPS_VOUT;
    } else {
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%s, cmd=%s, out=%s\n", sensor_str, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int
psu_stbvout_get(int* stbmvout, int id)
{
    int i=0, token_idx=1, token_val=0;

    char cmd[48];
    char cmd_out[150];
    char* tokens[20];
    char delimiter[]=",";
    const char* sensor_str;

    memset(cmd, 0, sizeof(cmd));
    memset(cmd_out, 0, sizeof(cmd_out));

    sensor_str = (id == PSU_ID_PSU0 ? psu_id_str[PSU_ID_PSU0_STBVOUT] : psu_id_str[PSU_ID_PSU1_STBVOUT]);
    snprintf(cmd, sizeof(cmd), CMD_BMC_SDR_GET, sensor_str);

    //Get sensor info from BMC
    if (exec_cmd(cmd, cmd_out, sizeof(cmd_out)) < 0) {
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%s\n", sensor_str);
        return ONLP_STATUS_E_INTERNAL;
    }

    //Check output is correct
    if (strnlen(cmd_out, sizeof(cmd_out))==0 ||
        strchr(cmd_out, ',')==NULL ||
        strstr(cmd_out, sensor_str)==NULL ){
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%s, cmd=%s, out=%s\n", sensor_str, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    //parse cmd_out to tokens
    tokens[i++] = strtok(cmd_out, delimiter);
    while (tokens[i-1] != NULL) {
        tokens[i++] = strtok(NULL, delimiter);
    }

    //read token_idx field
    if (i>=token_idx) {
        token_val = (int) (atof(tokens[token_idx])*1000);
        *stbmvout = token_val;
    } else {
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%s, cmd=%s, out=%s\n", sensor_str, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int
psu_iin_get(onlp_psu_info_t* info, int id)
{
    int i=0, token_idx=1, token_val=0;

    char cmd[48];
    char cmd_out[150];
    char* tokens[20];
    char delimiter[]=",";
    const char* sensor_str;

    memset(cmd, 0, sizeof(cmd));
    memset(cmd_out, 0, sizeof(cmd_out));

    sensor_str = (id == PSU_ID_PSU0 ? psu_id_str[PSU_ID_PSU0_IIN] : psu_id_str[PSU_ID_PSU1_IIN]);
    snprintf(cmd, sizeof(cmd), CMD_BMC_SDR_GET, sensor_str);

    //Get sensor info from BMC
    if (exec_cmd(cmd, cmd_out, sizeof(cmd_out)) < 0) {
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%s\n", sensor_str);
        return ONLP_STATUS_E_INTERNAL;
    }

    //Check output is correct
    if (strnlen(cmd_out, sizeof(cmd_out))==0 ||
        strchr(cmd_out, ',')==NULL ||
        strstr(cmd_out, sensor_str)==NULL ){
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%s, cmd=%s, out=%s\n", sensor_str, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    //parse cmd_out to tokens
    tokens[i++] = strtok(cmd_out, delimiter);
    while (tokens[i-1] != NULL) {
        tokens[i++] = strtok(NULL, delimiter);
    }

    //read token_idx field
    if (i>=token_idx) {
        token_val = (int) (atof(tokens[token_idx])*1000);
        info->miin = token_val;
        info->caps |= ONLP_PSU_CAPS_IIN;
    } else {
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%s, cmd=%s, out=%s\n", sensor_str, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int
psu_iout_get(onlp_psu_info_t* info, int id)
{
    int i=0, token_idx=1, token_val=0;

    char cmd[48];
    char cmd_out[150];
    char* tokens[20];
    char delimiter[]=",";
    const char* sensor_str;

    memset(cmd, 0, sizeof(cmd));
    memset(cmd_out, 0, sizeof(cmd_out));

    sensor_str = (id == PSU_ID_PSU0 ? psu_id_str[PSU_ID_PSU0_IOUT] : psu_id_str[PSU_ID_PSU1_IOUT]);
    snprintf(cmd, sizeof(cmd), CMD_BMC_SDR_GET, sensor_str);

    //Get sensor info from BMC
    if (exec_cmd(cmd, cmd_out, sizeof(cmd_out)) < 0) {
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%s\n", sensor_str);
        return ONLP_STATUS_E_INTERNAL;
    }

    //Check output is correct
    if (strnlen(cmd_out, sizeof(cmd_out))==0 ||
        strchr(cmd_out, ',')==NULL ||
        strstr(cmd_out, sensor_str)==NULL ){
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%s, cmd=%s, out=%s\n", sensor_str, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    //parse cmd_out to tokens
    tokens[i++] = strtok(cmd_out, delimiter);
    while (tokens[i-1] != NULL) {
        tokens[i++] = strtok(NULL, delimiter);
    }

    //read token_idx field
    if (i>=token_idx) {
        token_val = (int) (atof(tokens[token_idx])*1000);
        info->miout = token_val;
        info->caps |= ONLP_PSU_CAPS_IOUT;
    } else {
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%s, cmd=%s, out=%s\n", sensor_str, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int
psu_stbiout_get(int* stbmiout, int id)
{
    int i=0, token_idx=1, token_val=0;

    char cmd[48];
    char cmd_out[150];
    char* tokens[20];
    char delimiter[]=",";
    const char* sensor_str;

    memset(cmd, 0, sizeof(cmd));
    memset(cmd_out, 0, sizeof(cmd_out));

    sensor_str = (id == PSU_ID_PSU0 ? psu_id_str[PSU_ID_PSU0_STBIOUT] : psu_id_str[PSU_ID_PSU1_STBIOUT]);
    snprintf(cmd, sizeof(cmd), CMD_BMC_SDR_GET, sensor_str);

    //Get sensor info from BMC
    if (exec_cmd(cmd, cmd_out, sizeof(cmd_out)) < 0) {
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%s\n", sensor_str);
        return ONLP_STATUS_E_INTERNAL;
    }

    //Check output is correct
    if (strnlen(cmd_out, sizeof(cmd_out))==0 ||
        strchr(cmd_out, ',')==NULL ||
        strstr(cmd_out, sensor_str)==NULL ){
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%s, cmd=%s, out=%s\n", sensor_str, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    //parse cmd_out to tokens
    tokens[i++] = strtok(cmd_out, delimiter);
    while (tokens[i-1] != NULL) {
        tokens[i++] = strtok(NULL, delimiter);
    }

    //read token_idx field
    if (i>=token_idx) {
        token_val = (int) (atof(tokens[token_idx])*1000);
        *stbmiout = token_val;
    } else {
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%s, cmd=%s, out=%s\n", sensor_str, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int
psu_pout_get(onlp_psu_info_t* info, int i2c_bus)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
psu_pin_get(onlp_psu_info_t* info, int i2c_bus)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
psu_eeprom_get(onlp_psu_info_t* info, int id)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
psu_fru_get(onlp_psu_info_t* info, int id)
{
    char cmd[256];
    char cmd_out[64];
    char fru_model[] = "Product Name";  //only Product Name can identify AC/DC
    char fru_serial[] = "Product Serial";

    //FRU (model)

    memset(cmd, 0, sizeof(cmd));
    memset(cmd_out, 0, sizeof(cmd_out));
    memset(info->model, 0, sizeof(info->model));

    snprintf(cmd, sizeof(cmd), CMD_FRU_INFO_GET, id, fru_model);

    //Get psu fru info (model) from BMC
    if (exec_cmd(cmd, cmd_out, sizeof(cmd_out)) < 0) {
        AIM_LOG_ERROR("unable to read fru info from BMC, fru id=%d, cmd=%s, out=%s\n", id, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    //Check output is correct
    if (strnlen(cmd_out, sizeof(cmd_out))==0){
        AIM_LOG_ERROR("unable to read fru info from BMC, cmd_out is empty, fru id=%d, cmd=%s, out=%s\n", id, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    snprintf(info->model, sizeof(info->model), "%s", cmd_out);

    //FRU (serial)

    memset(cmd, 0, sizeof(cmd));
    memset(cmd_out, 0, sizeof(cmd_out));
    memset(info->serial, 0, sizeof(info->serial));

    snprintf(cmd, sizeof(cmd), CMD_FRU_INFO_GET, id, fru_serial);

    //Get psu fru info (model) from BMC
    if (exec_cmd(cmd, cmd_out, sizeof(cmd_out)) < 0) {
        AIM_LOG_ERROR("unable to read fru info from BMC, fru id=%d, cmd=%s, out=%s\n", id, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    //Check output is correct
    if (strnlen(cmd_out, sizeof(cmd_out))==0){
        AIM_LOG_ERROR("unable to read fru info from BMC, cmd_out is empty, fru id=%d, cmd=%s, out=%s\n", id, cmd, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    snprintf(info->serial, sizeof(info->serial), "%s", cmd_out);

    return ONLP_STATUS_OK;
}

int
psu_present_get(int *pw_present, int id)
{
    int reg_val, rc;
    int mask;

    if (id == PSU_ID_PSU0) {
        mask = PSU0_PRESENT_MASK;
    } else if (id == PSU_ID_PSU1) {
        mask = PSU1_PRESENT_MASK;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    if ((rc = file_read_hex(&reg_val, "/sys/bus/i2c/devices/1-0030/cpld_psu_status"
                                 )) != ONLP_STATUS_OK) {
        return rc;
    }

    *pw_present = ((mask_shift(reg_val, mask))? 0 : 1);

    return ONLP_STATUS_OK;
}

int
psu_pwgood_get(int *pw_good, int id)
{
    int reg_val, rc;
    int mask;

    if (id == PSU_ID_PSU0) {
        mask = PSU0_PWGOOD_MASK;
    } else if (id == PSU_ID_PSU1) {
        mask = PSU1_PWGOOD_MASK;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    if ((rc = file_read_hex(&reg_val, "/sys/bus/i2c/devices/1-0030/cpld_psu_status"
                                 )) != ONLP_STATUS_OK) {
        return rc;
    }

    *pw_good = ((mask_shift(reg_val, mask))? 1 : 0);

    return ONLP_STATUS_OK;
}

int
qsfp_present_get(int port, int *pres_val)
{
    int status, rc;
    int cpld_addr, sysfs_attr_offset;
    uint8_t data[8];
    int data_len;

    memset(data, 0, sizeof(data));

    //get cpld addr
    cpld_addr = qsfp_port_to_cpld_addr(port);

    //get sysfs_attr_offset
    sysfs_attr_offset = qsfp_port_to_sysfs_attr_offset(port);


    if ((rc = onlp_file_read(data, sizeof(data), &data_len, "/sys/bus/i2c/devices/%d-%04x/cpld_qsfp_present_%d",
                                 I2C_BUS_1, cpld_addr, sysfs_attr_offset)) != ONLP_STATUS_OK) {
	    AIM_LOG_ERROR("onlp_file_read failed, error=%d, /sys/bus/i2c/devices/%d-%04x/cpld_qsfp_present_%d",
                        rc, I2C_BUS_1, cpld_addr, sysfs_attr_offset);
        return ONLP_STATUS_E_INTERNAL;
    }
    status = (int) strtol((char *)data, NULL, 0);

    *pres_val = !((status >> (port % 8))  & 0x1);
    //*pres_val = !(status >> (0x1 << port % 8));

    return ONLP_STATUS_OK;
}

int sfp_present_get(int port, int *pres_val)
{
    int rc;
    uint8_t data[8];
    int data_len = 0, data_value = 0, data_mask = 0;
    char sysfs_path[128];
    int real_port = 0;
    int cpld_addr = 0x0;

    memset(data, 0, sizeof(data));
    memset(sysfs_path, 0, sizeof(sysfs_path));

    real_port = port - QSFP_NUM - QSFPDD_NUM;
    if (get_board_id() == 1) { //alpha
        switch(real_port) {
            case 0:
                cpld_addr = 0x30;
                data_mask = 0x01;
                snprintf(sysfs_path, sizeof(sysfs_path), SYS_DEV "%d-%04x/cpld_sfp_status_cpu", I2C_BUS_1, cpld_addr);
                break;
            case 1:
                cpld_addr = 0x30;
                data_mask = 0x10;
                snprintf(sysfs_path, sizeof(sysfs_path), SYS_DEV "%d-%04x/cpld_sfp_status_cpu", I2C_BUS_1, cpld_addr);
                break;
            case 2:
                cpld_addr = 0x31;
                data_mask = 0x01;
                snprintf(sysfs_path, sizeof(sysfs_path), SYS_DEV "%d-%04x/cpld_sfp_status_inband", I2C_BUS_1, cpld_addr);
                break;

            case 3:
                cpld_addr = 0x31;
                data_mask = 0x10;
                snprintf(sysfs_path, sizeof(sysfs_path), SYS_DEV "%d-%04x/cpld_sfp_status_inband", I2C_BUS_1, cpld_addr);
                break;
            default:
                AIM_LOG_ERROR("unknown sfp port, port=%d\n", port);
                return ONLP_STATUS_E_INTERNAL;
        }
    }
    else {
        switch(real_port) {
            case 0:
                cpld_addr = 0x31;
                data_mask = 0x01;
                snprintf(sysfs_path, sizeof(sysfs_path), SYS_DEV "%d-%04x/cpld_sfp_status_p0_p1_inband", I2C_BUS_1, cpld_addr);
                break;
            case 1:
                cpld_addr = 0x31;
                data_mask = 0x10;
                snprintf(sysfs_path, sizeof(sysfs_path), SYS_DEV "%d-%04x/cpld_sfp_status_p0_p1_inband", I2C_BUS_1, cpld_addr);
                break;
            case 2:
                cpld_addr = 0x31;
                data_mask = 0x01;
                snprintf(sysfs_path, sizeof(sysfs_path), SYS_DEV "%d-%04x/cpld_sfp_status_p2_p3_inband", I2C_BUS_1, cpld_addr);
                break;

            case 3:
                cpld_addr = 0x31;
                data_mask = 0x10;
                snprintf(sysfs_path, sizeof(sysfs_path), SYS_DEV "%d-%04x/cpld_sfp_status_p2_p3_inband", I2C_BUS_1, cpld_addr);
                break;
            default:
                AIM_LOG_ERROR("unknown sfp port, port=%d\n", port);
                return ONLP_STATUS_E_INTERNAL;
        }
    }


    if ((rc = onlp_file_read(data, sizeof(data), &data_len, sysfs_path)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("onlp_file_read failed, error=%d, sysfs=%s", rc, sysfs_path);
        return ONLP_STATUS_E_INTERNAL;
    }

    //hex to int
    data_value = (int) strtol((char *)data, NULL, 0);

    *pres_val = !(data_value & data_mask);

    return ONLP_STATUS_OK;
}

int
system_led_set(onlp_led_mode_t mode)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
fan_led_set(onlp_led_mode_t mode)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
psu1_led_set(onlp_led_mode_t mode)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
psu2_led_set(onlp_led_mode_t mode)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
fan_tray_led_set(onlp_oid_t id, onlp_led_mode_t mode)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int read_ioport(int addr, int *reg_val) {
    int ret;

    /*set r/w permission of all 65536 ports*/
    ret = iopl(3);
    if(ret < 0){
        AIM_LOG_ERROR("unable to read cpu cpld version, iopl enable error %d\n", ret);
        return ONLP_STATUS_E_INTERNAL;
    }
    *reg_val = inb(addr);

    /*set r/w permission of  all 65536 ports*/
    ret = iopl(0);
    if(ret < 0){
        AIM_LOG_ERROR("unable to read cpu cpld version, iopl disable error %d\n", ret);
        return ONLP_STATUS_E_INTERNAL;
    }
    return ONLP_STATUS_OK;
}

int
exec_cmd(char *cmd, char* out, int size) {
    FILE *fp;

    /* Open the command for reading. */
    fp = popen(cmd, "r");
    if (fp == NULL) {
        AIM_LOG_ERROR("Failed to run command %s\n", cmd);
        return ONLP_STATUS_E_INTERNAL;
    }

    /* Read the output a line at a time - output it. */
    while (fgets(out, size-1, fp) != NULL) {
    }

    /* close */
    pclose(fp);

    return ONLP_STATUS_OK;
}

int
get_ipmitool_len(char *ipmitool_out){
    size_t str_len=0, ipmitool_len=0;
    str_len = strlen(ipmitool_out);
    if (str_len>0) {
        ipmitool_len = str_len/3;
    }
    return ipmitool_len;
}

int
sysi_platform_info_get(onlp_platform_info_t* pi)
{
    int cpu_cpld_addr = 0x600, cpu_cpld_ver, cpu_cpld_ver_major, cpu_cpld_ver_minor;
    int cpld_ver[CPLD_MAX], cpld_ver_major[CPLD_MAX], cpld_ver_minor[CPLD_MAX];
    int mb_cpld1_addr = CPLD_REG_BASE + BRD_ID_REG, mb_cpld1_board_type_rev, mb_cpld1_hw_rev, mb_cpld1_build_rev;
    int i;
    char bios_out[32];
    char bmc_out1[8], bmc_out2[8], bmc_out3[8];
    int rc=0;

    memset(bios_out, 0, sizeof(bios_out));
    memset(bmc_out1, 0, sizeof(bmc_out1));
    memset(bmc_out2, 0, sizeof(bmc_out2));
    memset(bmc_out3, 0, sizeof(bmc_out3));

    //get CPU CPLD version
    if (read_ioport(cpu_cpld_addr, &cpu_cpld_ver) < 0) {
        AIM_LOG_ERROR("unable to read CPU CPLD version\n");
        return ONLP_STATUS_E_INTERNAL;
    }
    cpu_cpld_ver_major = (((cpu_cpld_ver) >> 6 & 0x01));
    cpu_cpld_ver_minor = (((cpu_cpld_ver) & 0x3F));

    //get MB CPLD version
    for(i=0; i<CPLD_MAX; ++i) {
        if ((rc = file_read_hex(&cpld_ver[i], "/sys/bus/i2c/devices/1-00%02x/cpld_version",
                                 CPLD_BASE_ADDR[i])) != ONLP_STATUS_OK) {
            return ONLP_STATUS_E_INTERNAL;
        }
        if (cpld_ver[i] < 0) {
            AIM_LOG_ERROR("unable to read MB CPLD version\n");
            return ONLP_STATUS_E_INTERNAL;
        }

        cpld_ver_major[i] = (((cpld_ver[i]) >> 6 & 0x11));
        cpld_ver_minor[i] = (((cpld_ver[i]) & 0x3F));
    }

    pi->cpld_versions = aim_fstrdup(
        "\n"
        "[CPU CPLD] %d.%02d\n"
        "[MB CPLD1] %d.%02d\n"
        "[MB CPLD2] %d.%02d\n"
        "[MB CPLD3] %d.%02d\n"
        "[MB CPLD4] %d.%02d\n"
        "[MB CPLD5] %d.%02d\n",
        cpu_cpld_ver_major, cpu_cpld_ver_minor,
        cpld_ver_major[0], cpld_ver_minor[0],
        cpld_ver_major[1], cpld_ver_minor[1],
        cpld_ver_major[2], cpld_ver_minor[2],
        cpld_ver_major[3], cpld_ver_minor[3],
        cpld_ver_major[4], cpld_ver_minor[4]);

    //Get HW Build Version
    if (read_ioport(mb_cpld1_addr, &mb_cpld1_board_type_rev) < 0) {
        AIM_LOG_ERROR("unable to read MB CPLD1 Board Type Revision\n");
        return ONLP_STATUS_E_INTERNAL;
    }
    mb_cpld1_hw_rev = ((mb_cpld1_board_type_rev) & 0x03);
    //FIXME: check build_rev bits
    //mb_cpld1_build_rev = (((mb_cpld1_board_type_rev) & 0x03) | ((mb_cpld1_board_type_rev) >> 5 & 0x04));
    mb_cpld1_build_rev = ((mb_cpld1_board_type_rev) >> 3 & 0x07);

    //Get BIOS version
    if (exec_cmd(CMD_BIOS_VER, bios_out, sizeof(bios_out)) < 0) {
        AIM_LOG_ERROR("unable to read BIOS version\n");
        return ONLP_STATUS_E_INTERNAL;
    }

    //Get BMC version
    if (exec_cmd(CMD_BMC_VER_1, bmc_out1, sizeof(bmc_out1)) < 0 ||
        exec_cmd(CMD_BMC_VER_2, bmc_out2, sizeof(bmc_out2)) < 0 ||
        exec_cmd(CMD_BMC_VER_3, bmc_out3, sizeof(bmc_out3))) {
            AIM_LOG_ERROR("unable to read BMC version\n");
            return ONLP_STATUS_E_INTERNAL;
    }

    pi->other_versions = aim_fstrdup(
        "\n"
        "[HW   ] %d\n"
        "[BUILD] %d\n"
        "[BIOS ] %s\n"
        "[BMC  ] %d.%d.%d\n",
        mb_cpld1_hw_rev,
        mb_cpld1_build_rev,
        bios_out,
        atoi(bmc_out1), atoi(bmc_out2), atoi(bmc_out3));

    return ONLP_STATUS_OK;
}

bool
onlp_sysi_bmc_en_get(void)
{
   return true;
}

int
qsfp_port_to_cpld_addr(int port)
{
    int cpld_addr = 0;
    int cpld_num = ((port%32) / 8) + 1;

    cpld_addr = CPLD_BASE_ADDR[cpld_num];

    return cpld_addr;
}

int
qsfp_port_to_sysfs_attr_offset(int port)
{
    int sysfs_attr_offset = 0;

    sysfs_attr_offset = port / 32;

    return sysfs_attr_offset;
}

int
parse_bmc_sdr_cmd(char *cmd_out, int cmd_out_size,
                  char *tokens[], int token_size,
                  const char *sensor_id_str, int *idx)
{
    char cmd[BMC_CMD_SDR_SIZE];
    char delimiter[]=",";
    char delimiter_c = ',';

    *idx=0;
    memset(cmd, 0, sizeof(cmd));
    memset(cmd_out, 0, cmd_out_size);

    snprintf(cmd, sizeof(cmd), CMD_BMC_SDR_GET, sensor_id_str);

    if (exec_cmd(cmd, cmd_out, cmd_out_size) < 0) {
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%s\n", sensor_id_str);
        return ONLP_STATUS_E_INTERNAL;
    }

    //Check output is correct
    if (strnlen(cmd_out, cmd_out_size)==0 ||
        strchr(cmd_out, delimiter_c)==NULL ||
        strstr(cmd_out, sensor_id_str)==NULL ){
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%s, out=%s\n", sensor_id_str, cmd_out);
        return ONLP_STATUS_E_INTERNAL;
    }

    //parse cmd_out to tokens
    tokens[(*idx)++] = strtok(cmd_out, delimiter);
    while (tokens[(*idx)-1] != NULL) {
        tokens[(*idx)++] = strtok(NULL, delimiter);
    }

    return ONLP_STATUS_OK;
}

int
bmc_thermal_info_get(onlp_thermal_info_t* info, int id)
{
    int rc=0;
    float data=0;

    rc = bmc_sensor_read(id - 1, THERMAL_SENSOR, &data);
    if ( rc != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%d\n", id);
        return rc;
    }
    info->mcelsius = (int) (data*1000);

    return rc;
}

int
bmc_fan_info_get(onlp_fan_info_t* info, int id)
{
    int rv=0, rpm=0, percentage=0;
    int presence=0;
    float data=0;
    //FIXME
    int sys_max_fan_speed = 16100;
    int psu_max_fan_speed = 26200;
    int max_fan_speed = 0;

    //check presence for fantray 1-4
    if (id >= FAN_ID_FAN0 && id <= FAN_ID_FAN3) {
        rv = bmc_sensor_read(id - FAN_ID_FAN0 + 15, FAN_SENSOR, &data);
        if ( rv != ONLP_STATUS_OK) {
            AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%d\n", id);
            return rv;
        }
        presence = (int) data;

        if( presence == 1 ) {
            info->status |= ONLP_FAN_STATUS_PRESENT;
        } else {
            info->status &= ~ONLP_FAN_STATUS_PRESENT;
            return ONLP_STATUS_OK;
        }
    }

    //get fan rpm

    rv = bmc_sensor_read(id - FAN_ID_FAN0 + 9, FAN_SENSOR, &data);
    if ( rv != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%d\n", id);
        return rv;
    }
    rpm = (int) data;

    //set rpm field
    info->rpm = rpm;

    if (id >= FAN_ID_FAN0 && id <= FAN_ID_FAN3) {
        percentage = (info->rpm*100)/sys_max_fan_speed;
        info->percentage = percentage;
        info->status |= (rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;
    } else if (id >= FAN_ID_PSU0_FAN && id <= FAN_ID_PSU1_FAN) {
        if (id == FAN_ID_PSU0_FAN || id == FAN_ID_PSU1_FAN ) {
            max_fan_speed = psu_max_fan_speed;
        } else {
            max_fan_speed = psu_max_fan_speed;
        }
        percentage = (info->rpm*100)/max_fan_speed;
        info->percentage = percentage;
        info->status |= (rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;
    }

    return ONLP_STATUS_OK;
}

int
file_read_hex(int* value, const char* fmt, ...)
{
    int rv;
    va_list vargs;
    va_start(vargs, fmt);
    rv = file_vread_hex(value, fmt, vargs);
    va_end(vargs);
    return rv;
}

int
file_vread_hex(int* value, const char* fmt, va_list vargs)
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
    char cmd_buf[256] = {0};
    int ret = 0;

    if(access(MB_CPLD1_ID_PATH, F_OK) != -1 ) {

        snprintf(cmd_buf, sizeof(cmd_buf), "cat %s > /dev/null 2>&1", MB_CPLD1_ID_PATH);
        ret = system(cmd_buf);

        if (ret != 0) {
            if(access(MUX_RESET_PATH, F_OK) != -1 ) {
                //AIM_LOG_ERROR("I2C bus is stuck!! (port=%d)", port);
                snprintf(cmd_buf, sizeof(cmd_buf), "echo 0 > %s 2> /dev/null", MUX_RESET_PATH);
                ret = system(cmd_buf);
                //AIM_LOG_ERROR("Do I2C mux reset!! (ret=%d)", ret);
            }
        }
    }
}

int
sys_led_info_get(onlp_led_info_t* info, int id)
{
    int value, rc;
    int sysfs_index;
    int shift, led_val,led_val_color, led_val_blink, led_val_onoff;

    if (id == LED_ID_SYS_SYS) {
        sysfs_index=0;
        shift = 4;
    } else if (id == LED_ID_SYS_PSU0) {
        sysfs_index=1;
        shift = 0;
    } else if (id == LED_ID_SYS_PSU1) {
        sysfs_index=1;
        shift = 4;
    } else if (id == LED_ID_SYS_FAN) {
        sysfs_index=0;
        shift = 0;
    } else if (id == LED_ID_SYS_SYNC) {
        sysfs_index=2;
        shift = 4;
    } else if (id == LED_ID_SYS_ID) {
        sysfs_index=2;
        shift = 0;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    if ((rc = file_read_hex(&value, "/sys/bus/i2c/devices/1-0030/cpld_system_led_%d",
                                 sysfs_index)) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    }

    led_val = (value >> shift);
    led_val_color = (led_val >> 0)& 1;
    led_val_blink = (led_val >> 2)& 1;
    led_val_onoff = (led_val >> 3)& 1;

    //onoff
    if (led_val_onoff == 0) {
        info->mode = ONLP_LED_MODE_OFF;
    } else {
        //color
        if (id == LED_ID_SYS_ID) {
            info->mode = ONLP_LED_MODE_BLUE;
        } else if (led_val_color == 0) {
            info->mode = ONLP_LED_MODE_YELLOW;
        } else {
            info->mode = ONLP_LED_MODE_GREEN;
        }
        //blinking
        if (led_val_blink == 1) {
            info->mode = info->mode + 1;
        }
    }

    return ONLP_STATUS_OK;
}
