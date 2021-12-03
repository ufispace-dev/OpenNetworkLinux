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

bmc_info_t bmc_cache[] =
{
    {"TEMP_CPU_PECI", 0},
    {"TEMP_CPU_ENV", 0},
    {"TEMP_CPU_ENV_2", 0},
    {"TEMP_MAC_FRONT", 0},
    {"TEMP_MAC_DIE", 0},
    {"TEMP_0x48", 0},
    {"TEMP_0x49", 0},
    {"PSU0_TEMP", 0},
    {"PSU1_TEMP", 0},
    {"FAN0_RPM", 0},
    {"FAN1_RPM", 0},
    {"FAN2_RPM", 0},
    {"FAN3_RPM", 0},
    {"PSU0_FAN1", 0},
    {"PSU1_FAN1", 0},
    {"FAN0_PSNT_L",0},
    {"FAN1_PSNT_L",0},
    {"FAN2_PSNT_L", 0},
    {"FAN3_PSNT_L", 0},
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

bmc_fru_info_t bmc_fru_cache[] =
{
    {"PSU0_MODEL", ""},
    {"PSU0_SERIAL", ""},
    {"PSU1_MODEL", ""},
    {"PSU1_SERIAL", ""},
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
    if(!sem_inited) {
        onlp_shlock_create(LOCK_MAGIC, &onlp_lock, "bmc-file-lock");
        sem_inited = 1;
        check_and_do_i2c_mux_reset(-1);
    }
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

void get_bmc_cache_dev_names(int dev_size, char *devs)
{
    int dev_num;
    devs[0] = '\0';
    for(dev_num = 0; dev_num < dev_size; dev_num++) {
        strcat(devs, bmc_cache[dev_num].name);
        strcat(devs, " ");
    }
}

int bmc_sensor_read(int bmc_cache_index, int sensor_type, float *data)
{
    struct timeval new_tv;
    FILE *fp = NULL;
    char dev_names[512] = {0};
    char ipmi_cmd[1024] = {0};
    char get_data_cmd[256] = {0};
    char buf[20];
    float f_rv = 0;
    int dev_num = 0;
    int dev_size = sizeof(bmc_cache)/sizeof(bmc_cache[0]);
    int cache_time = 0;
    int bmc_cache_expired = 0;
    long file_last_time = 0;
    static long bmc_cache_time = 0;
    char* presence_str = "Present";

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

    if(check_file_exist(BMC_SENSOR_CACHE, &file_last_time)) {
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
    if(bmc_cache_expired == 1) {
        ONLP_LOCK();
        if(bmc_cache_expired_check(file_last_time, bmc_cache_time, cache_time)) {
            get_bmc_cache_dev_names(dev_size, dev_names);
            snprintf(ipmi_cmd, sizeof(ipmi_cmd), CMD_BMC_SENSOR_CACHE, dev_names);
            system(ipmi_cmd);
        }

        for(dev_num = 0; dev_num < dev_size; dev_num++)
        {
            memset(buf, 0, sizeof(buf));

            if(dev_num >= ID_FAN0_PSNT_L && dev_num <=ID_FAN3_PSNT_L) {
                snprintf(get_data_cmd, sizeof(get_data_cmd), CMD_BMC_CACHE_GET, bmc_cache[dev_num].name, 5);
                fp = popen(get_data_cmd, "r");
                if(fp != NULL) {
                    if(fgets(buf, sizeof(buf), fp) != NULL) {
                        if(strstr(buf, presence_str) != NULL) {
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
                if(fp != NULL) {
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

    return ONLP_STATUS_OK;
}

int bmc_fru_read(onlp_psu_info_t* info, int fru_id)
{
    struct timeval new_tv;
    char cache_cmd[512] = {0};
    char get_data_cmd[256] = {0};
    char cmd_out[128] = {0};
    char cache_file[128] = {0};
    int rv = ONLP_STATUS_OK;
    int cache_time = 0;
    int bmc_cache_expired = 0;
    long file_last_time = 0;
    static long bmc_fru_cache_time[PSU_NUM+1] = {0,0,0};  //index 0 for dummy
    char fru_model[] = "Product Name";  //only Product Name can identify AC/DC
    char fru_serial[] = "Product Serial";
    int cache_idx;

    //check fru id
    if(fru_id > PSU_NUM) {
        AIM_LOG_ERROR("invalid fru_id %d\n", fru_id);
        return ONLP_STATUS_E_INTERNAL;
    }

    cache_time = FRU_CACHE_TIME;
    snprintf(cache_file, sizeof(cache_file), BMC_FRU_CACHE, fru_id);

    if(check_file_exist(cache_file, &file_last_time)) {
        gettimeofday(&new_tv, NULL);
        if(bmc_cache_expired_check(file_last_time, new_tv.tv_sec, cache_time)) {
            bmc_cache_expired = 1;
        }
        else {
            bmc_cache_expired = 0;
        }
    }
    else {
        bmc_cache_expired = 1;
    }

    if(bmc_fru_cache_time[fru_id] == 0 &&
        check_file_exist(cache_file, &file_last_time)) {
        bmc_cache_expired = 1;
        gettimeofday(&new_tv,NULL);
        bmc_fru_cache_time[fru_id] = new_tv.tv_sec;
    }

    //update cache
    if(bmc_cache_expired == 1) {
        ONLP_LOCK();
        if(bmc_cache_expired_check(file_last_time, bmc_fru_cache_time[fru_id], cache_time)) {
            //cache expired, update cache file
            snprintf(cache_cmd, sizeof(cache_cmd), CMD_BMC_FRU_CACHE, fru_id, cache_file);
            system(cache_cmd);
        }

        //handle psu model
        //get psu fru from cache file
        snprintf(get_data_cmd, sizeof(get_data_cmd), CMD_CACHE_FRU_GET, cache_file, fru_model);
        if(exec_cmd(get_data_cmd, cmd_out, sizeof(cmd_out)) < 0) {
            AIM_LOG_ERROR("unable to read psu model from BMC, fru id=%d, cmd=%s, out=%s\n",
                fru_id, get_data_cmd, cmd_out);
            goto exit;
        }

        //check output is correct
        if(strnlen(cmd_out, sizeof(cmd_out))==0){
            AIM_LOG_ERROR("unable to read psu model from BMC, fru id=%d, cmd=%s, out=%s\n",
                fru_id, get_data_cmd, cmd_out);
            goto exit;
        }

        //save to cache
        cache_idx = (fru_id - 1)*2;
        snprintf(bmc_fru_cache[cache_idx].data, sizeof(bmc_fru_cache[cache_idx].data), "%s", cmd_out);

        //handle psu serial
        //get psu fru from cache file
        snprintf(get_data_cmd, sizeof(get_data_cmd), CMD_CACHE_FRU_GET, cache_file, fru_serial);
        if(exec_cmd(get_data_cmd, cmd_out, sizeof(cmd_out)) < 0) {
            AIM_LOG_ERROR("unable to read psu serial from BMC, fru id=%d, cmd=%s, out=%s\n",
                fru_id, get_data_cmd, cmd_out);
            goto exit;
        }

        //check output is correct
        if(strnlen(cmd_out, sizeof(cmd_out))==0){
            AIM_LOG_ERROR("unable to read psu serial from BMC, fru id=%d, cmd=%s, out=%s\n",
                fru_id, get_data_cmd, cmd_out);
            goto exit;
        }

        //save to cache
        cache_idx = (fru_id - 1)*2 + 1;
        snprintf(bmc_fru_cache[cache_idx].data, sizeof(bmc_fru_cache[cache_idx].data), "%s", cmd_out);


        gettimeofday(&new_tv,NULL);
        bmc_fru_cache_time[fru_id] = new_tv.tv_sec;
        ONLP_UNLOCK();
    }

    //read from cache
    cache_idx = (fru_id - 1)*2;
    snprintf(info->model, sizeof(info->model), "%s", bmc_fru_cache[cache_idx].data) < 0 ? abort() : (void)0;
    cache_idx = (fru_id - 1)*2 + 1;
    snprintf(info->serial, sizeof(info->serial), "%s", bmc_fru_cache[cache_idx].data) < 0 ? abort() : (void)0;

    return rv;

exit:
    ONLP_UNLOCK();
    return ONLP_STATUS_E_INTERNAL;
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

/*
 * This function check the I2C bus statuas by using the sysfs of cpld_id,
 * If the I2C Bus is stcuk, do the i2c mux reset.
 */
void check_and_do_i2c_mux_reset(int port)
{
    char cmd_buf[256] = {0};
    char sysfs[128];
    int ret = 0;

    snprintf(sysfs, sizeof(sysfs), "%s/%s",
                    MB_CPLD1_SYSFS_PATH, MB_CPLD_ID_ATTR);
    if(access(sysfs, F_OK) != -1 ) {

        snprintf(cmd_buf, sizeof(cmd_buf), "cat %s > /dev/null 2>&1", sysfs);
        ret = system(cmd_buf);

        if(ret != 0) {
            snprintf(sysfs, sizeof(sysfs), "%s/%s",
                        LPC_MB_CPLD_SYFSFS_PATH, LPC_MUX_RESET_ATTR);
            if(access(sysfs, F_OK) != -1 ) {
                snprintf(cmd_buf, sizeof(cmd_buf), "echo 0 > %s 2> /dev/null", sysfs);
                ret = system(cmd_buf);
                //AIM_LOG_SYSLOG_WARN("Do I2C mux reset!! (ret=%d)\r\n", ret);
            }
        }
    }
}
