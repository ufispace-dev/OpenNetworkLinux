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


#define FAN_CACHE_TIME        5
#define PSU_CACHE_TIME        5
#define THERMAL_CACHE_TIME    10
#define FRU_CACHE_TIME        10
#define BMC_CMD_SDR_SIZE      48
#define BMC_SENSOR_CACHE      "/tmp/bmc_sensor_cache"
#define BMC_FRU_CACHE         "/tmp/bmc_fru_cache.PUS%d"
#define CMD_BMC_FRU_CACHE     "ipmitool fru print %d > %s 2> /dev/null"
#define CMD_CACHE_FRU_GET     "cat %s | grep '%s' | cut -d':' -f2 | awk '{$1=$1};1' | tr -d '\\n'"
#define CMD_BMC_CACHE_GET     "cat "BMC_SENSOR_CACHE" | grep %s | awk -F',' '{print $%d}'"
#define CMD_BMC_SDR_GET       "ipmitool sdr -c get %s 2> /dev/null"
#define CMD_BMC_FAN_TRAY_DIR  "ipmitool raw 0x3c 0x31 0x0 2> /dev/null | xargs | cut -d' ' -f%d"
#define CMD_BMC_PSU_FAN_DIR   "ipmitool raw 0x3c 0x30 0x0 2> /dev/null | xargs | cut -d' ' -f%d"

#define CMD_BMC_SENSOR_CACHE  "ipmitool sdr -c get "\
                              " TEMP_CPU_PECI"\
                              " TEMP_CPU_ENV"\
                              " TEMP_CPU_ENV_2"\
                              " TEMP_MAC_ENV"\
                              " TEMP_CAGE"\
                              " TEMP_PSU_CONN"\
                              " PSU0_TEMP"\
                              " PSU1_TEMP"\
                              " FAN0_RPM_F"\
                              " FAN0_RPM_R"\
                              " FAN1_RPM_F"\
                              " FAN1_RPM_R"\
                              " FAN2_RPM_F"\
                              " FAN2_RPM_R"\
                              " FAN3_RPM_F"\
                              " FAN3_RPM_R"\
                              " FAN4_RPM_F"\
                              " FAN4_RPM_R"\
                              " FAN5_RPM_F"\
                              " FAN5_RPM_R"\
                              " PSU0_FAN1"\
                              " PSU1_FAN1"\
                              " FAN0_PSNT_L"\
                              " FAN1_PSNT_L"\
                              " FAN2_PSNT_L"\
                              " FAN3_PSNT_L"\
                              " FAN4_PSNT_L"\
                              " FAN5_PSNT_L"\
                              " FAN0_DIR"\
                              " FAN1_DIR"\
                              " FAN2_DIR"\
                              " FAN3_DIR"\
                              " FAN4_DIR"\
                              " FAN5_DIR"\
                              " PSU0_FAN1_DIR"\
                              " PSU1_FAN1_DIR"\
                              " PSU0_VIN"\
                              " PSU0_VOUT"\
                              " PSU0_IIN"\
                              " PSU0_IOUT"\
                              " PSU0_STBVOUT"\
                              " PSU0_STBIOUT"\
                              " PSU1_VIN"\
                              " PSU1_VOUT"\
                              " PSU1_IIN"\
                              " PSU1_IOUT"\
                              " PSU1_STBVOUT"\
                              " PSU1_STBIOUT"\
                              " > "BMC_SENSOR_CACHE\
                              " 2> /dev/null"

/* BMC Cached String (mapping with ipmitool sensors) */
bmc_info_t bmc_cache[] =
{
    [BMC_ATTR_ID_TEMP_CPU_PECI]  = {"TEMP_CPU_PECI"  , 0},
    [BMC_ATTR_ID_TEMP_CPU_ENV]   = {"TEMP_CPU_ENV"   , 0},
    [BMC_ATTR_ID_TEMP_CPU_ENV_2] = {"TEMP_CPU_ENV_2" , 0},
    [BMC_ATTR_ID_TEMP_MAC_ENV]   = {"TEMP_MAC_ENV"   , 0},
    [BMC_ATTR_ID_TEMP_CAGE]      = {"TEMP_CAGE"      , 0},
    [BMC_ATTR_ID_TEMP_PSU_CONN]  = {"TEMP_PSU_CONN"  , 0},
    [BMC_ATTR_ID_PSU0_TEMP]      = {"PSU0_TEMP"      , 0},
    [BMC_ATTR_ID_PSU1_TEMP]      = {"PSU1_TEMP"      , 0},
    [BMC_ATTR_ID_FAN0_RPM_F]     = {"FAN0_RPM_F"     , 0},
    [BMC_ATTR_ID_FAN0_RPM_R]     = {"FAN0_RPM_R"     , 0},
    [BMC_ATTR_ID_FAN1_RPM_F]     = {"FAN1_RPM_F"     , 0},
    [BMC_ATTR_ID_FAN1_RPM_R]     = {"FAN1_RPM_R"     , 0},
    [BMC_ATTR_ID_FAN2_RPM_F]     = {"FAN2_RPM_F"     , 0},
    [BMC_ATTR_ID_FAN2_RPM_R]     = {"FAN2_RPM_R"     , 0},
    [BMC_ATTR_ID_FAN3_RPM_F]     = {"FAN3_RPM_F"     , 0},
    [BMC_ATTR_ID_FAN3_RPM_R]     = {"FAN3_RPM_R"     , 0},
    [BMC_ATTR_ID_FAN4_RPM_F]     = {"FAN4_RPM_F"     , 0},
    [BMC_ATTR_ID_FAN4_RPM_R]     = {"FAN4_RPM_R"     , 0},
    [BMC_ATTR_ID_FAN5_RPM_F]     = {"FAN5_RPM_F"     , 0},
    [BMC_ATTR_ID_FAN5_RPM_R]     = {"FAN5_RPM_R"     , 0},
    [BMC_ATTR_ID_PSU0_FAN1]      = {"PSU0_FAN1"      , 0},
    [BMC_ATTR_ID_PSU1_FAN1]      = {"PSU1_FAN1"      , 0},
    [BMC_ATTR_ID_FAN0_PSNT_L]    = {"FAN0_PSNT_L"    , 0},
    [BMC_ATTR_ID_FAN1_PSNT_L]    = {"FAN1_PSNT_L"    , 0},
    [BMC_ATTR_ID_FAN2_PSNT_L]    = {"FAN2_PSNT_L"    , 0},
    [BMC_ATTR_ID_FAN3_PSNT_L]    = {"FAN3_PSNT_L"    , 0},
    [BMC_ATTR_ID_FAN4_PSNT_L]    = {"FAN4_PSNT_L"    , 0},
    [BMC_ATTR_ID_FAN5_PSNT_L]    = {"FAN5_PSNT_L"    , 0},
    [BMC_ATTR_ID_FAN0_DIR]       = {"FAN0_DIR"       , 0},
    [BMC_ATTR_ID_FAN1_DIR]       = {"FAN1_DIR"       , 0},
    [BMC_ATTR_ID_FAN2_DIR]       = {"FAN2_DIR"       , 0},
    [BMC_ATTR_ID_FAN3_DIR]       = {"FAN3_DIR"       , 0},
    [BMC_ATTR_ID_FAN4_DIR]       = {"FAN4_DIR"       , 0},
    [BMC_ATTR_ID_FAN5_DIR]       = {"FAN5_DIR"       , 0},
    [BMC_ATTR_ID_PSU0_FAN1_DIR]  = {"PSU0_FAN1_DIR"  , 0},
    [BMC_ATTR_ID_PSU1_FAN1_DIR]  = {"PSU1_FAN1_DIR"  , 0},
    [BMC_ATTR_ID_PSU0_VIN]       = {"PSU0_VIN"       , 0},
    [BMC_ATTR_ID_PSU0_VOUT]      = {"PSU0_VOUT"      , 0},
    [BMC_ATTR_ID_PSU0_IIN]       = {"PSU0_IIN"       , 0},
    [BMC_ATTR_ID_PSU0_IOUT]      = {"PSU0_IOUT"      , 0},
    [BMC_ATTR_ID_PSU0_STBVOUT]   = {"PSU0_STBVOUT"   , 0},
    [BMC_ATTR_ID_PSU0_STBIOUT]   = {"PSU0_STBIOUT"   , 0},
    [BMC_ATTR_ID_PSU1_VIN]       = {"PSU1_VIN"       , 0},
    [BMC_ATTR_ID_PSU1_VOUT]      = {"PSU1_VOUT"      , 0},
    [BMC_ATTR_ID_PSU1_IIN]       = {"PSU1_IIN"       , 0},
    [BMC_ATTR_ID_PSU1_IOUT]      = {"PSU1_IOUT"      , 0},
    [BMC_ATTR_ID_PSU1_STBVOUT]   = {"PSU1_STBVOUT"   , 0},
    [BMC_ATTR_ID_PSU1_STBIOUT]   = {"PSU1_STBIOUT"   , 0}
};

bmc_fru_info_t bmc_fru_cache[] =
{
    [BMC_FRU_ATTR_ID_PSU0_MODEL]  = {"PSU0_MODEL",  ""},
    [BMC_FRU_ATTR_ID_PSU0_SERIAL] = {"PSU0_SERIAL", ""},
    [BMC_FRU_ATTR_ID_PSU1_MODEL]  = {"PSU1_MODEL",  ""},
    [BMC_FRU_ATTR_ID_PSU1_SERIAL] = {"PSU1_SERIAL", ""}
};

const int CPLD_BASE_ADDR[] = {0x30, 0x31, 0x32};

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
    } else {
         if(new_time > last_time) {
             if((new_time - last_time) > cache_time) {
                bmc_cache_expired = 1;
             } else {
                bmc_cache_expired = 0;
             }
         } else if(new_time == last_time) {
             bmc_cache_expired = 0;
         } else {
             bmc_cache_expired = 1;
         }
    }

    return bmc_cache_expired;
}

/* bmc sesnor read with cache mechanism */
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
            snprintf(ipmi_cmd, sizeof(ipmi_cmd), CMD_BMC_SENSOR_CACHE);
            for (retry = 0; retry < retry_max; ++retry) {
                if((rv=system(ipmi_cmd)) != ONLP_STATUS_OK) {
                    if(retry == retry_max-1) {
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

            if( dev_num >= BMC_ATTR_ID_FAN0_PSNT_L && dev_num <= BMC_ATTR_ID_FAN5_PSNT_L ) {
                snprintf(get_data_cmd, sizeof(get_data_cmd), CMD_BMC_CACHE_GET, bmc_cache[dev_num].name, 5);
                fp = popen(get_data_cmd, "r");
                if(fp != NULL) {
                    if(fgets(buf, sizeof(buf), fp) != NULL) {
                        if( strstr(buf, presence_str) != NULL ) {
                            f_rv = 1;
                        } else {
                            f_rv = 0;
                        }
                        bmc_cache[dev_num].data = f_rv;
                    }
                }
                pclose(fp);
            } else if( dev_num >= BMC_ATTR_ID_FAN0_DIR && dev_num <= BMC_ATTR_ID_FAN5_DIR ) {

#if (UFI_BMC_FAN_DIR == 1)
                /* if UFI_BMC_FAN_DIR is not defined as 1, do nothing (BMC does not support!!) */
                snprintf(get_data_cmd, sizeof(get_data_cmd), CMD_BMC_FAN_TRAY_DIR, dev_num-BMC_ATTR_ID_FAN0_DIR+1);
                fp = popen(get_data_cmd, "r");
                if(fp != NULL) {
                    if(fgets(buf, sizeof(buf), fp) != NULL) {
                        if(atoi(buf) == FAN_DIR_B2F) {
                            f_rv = FAN_DIR_B2F;
                        } else {
                            f_rv = FAN_DIR_F2B;
                        }
                        bmc_cache[dev_num].data = f_rv;
                    }
                }
                pclose(fp);
#endif

            } else if( dev_num >= BMC_ATTR_ID_PSU0_FAN1_DIR && dev_num <= BMC_ATTR_ID_PSU1_FAN1_DIR ) {

#if (UFI_BMC_FAN_DIR == 1)
                /* if UFI_BMC_FAN_DIR is not defined as 1, do nothing (BMC does not support!!) */
                snprintf(get_data_cmd, sizeof(get_data_cmd), CMD_BMC_PSU_FAN_DIR, dev_num-BMC_ATTR_ID_PSU0_FAN1_DIR+1);
                fp = popen(get_data_cmd, "r");
                if(fp != NULL) {
                    if(fgets(buf, sizeof(buf), fp) != NULL) {
                        if(atoi(buf) == FAN_DIR_B2F) {
                            f_rv = FAN_DIR_B2F;
                        } else {
                            f_rv = FAN_DIR_F2B;
                        }
                        bmc_cache[dev_num].data = f_rv;
                    }
                }
                pclose(fp);
#endif

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

    return rv;
}

/* bmc fru read with cache mechanism */
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
    static long bmc_fru_cache_time[ONLP_PSU_MAX] = {0,0,0};  //index 0 for dummy
    char fru_model[] = "Product Name";  //only Product Name can identify AC/DC
    char fru_serial[] = "Product Serial";
    int cache_idx;

    //check fru id
    if(fru_id >= ONLP_PSU_MAX) {
        AIM_LOG_ERROR("invalid fru_id %d\n", fru_id);
        return ONLP_STATUS_E_INTERNAL;
    }

    cache_time = FRU_CACHE_TIME;
    snprintf(cache_file, sizeof(cache_file), BMC_FRU_CACHE, fru_id);

    if(check_file_exist(cache_file, &file_last_time)) {
        gettimeofday(&new_tv, NULL);
        if(bmc_cache_expired_check(file_last_time, new_tv.tv_sec, cache_time)) {
            bmc_cache_expired = 1;
        } else {
            bmc_cache_expired = 0;
        }
    } else {
        bmc_cache_expired = 1;
    }

    if(bmc_fru_cache_time[fru_id] == 0 && check_file_exist(cache_file, &file_last_time)) {
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

        /* handle psu model */
        //Get psu fru from cache file
        snprintf(get_data_cmd, sizeof(get_data_cmd), CMD_CACHE_FRU_GET, cache_file, fru_model);
        if(exec_cmd(get_data_cmd, cmd_out, sizeof(cmd_out)) < 0) {
            AIM_LOG_ERROR("unable to read psu model from BMC, fru id=%d, cmd=%s, out=%s\n",
                fru_id, get_data_cmd, cmd_out);
            goto exit;
        }

        //Check output is correct
        if(strnlen(cmd_out, sizeof(cmd_out))==0){
            AIM_LOG_ERROR("unable to read psu model from BMC, fru id=%d, cmd=%s, out=%s\n",
                fru_id, get_data_cmd, cmd_out);
            goto exit;
        }

        //save to cache
        cache_idx = (fru_id - 1)*2;
        snprintf(bmc_fru_cache[cache_idx].data, sizeof(bmc_fru_cache[cache_idx].data), "%s", cmd_out);

        /* handle psu serial */
        //Get psu fru from cache file
        snprintf(get_data_cmd, sizeof(get_data_cmd), CMD_CACHE_FRU_GET, cache_file, fru_serial);
        if(exec_cmd(get_data_cmd, cmd_out, sizeof(cmd_out)) < 0) {
            AIM_LOG_ERROR("unable to read psu serial from BMC, fru id=%d, cmd=%s, out=%s\n",
                fru_id, get_data_cmd, cmd_out);
            goto exit;
        }

        //Check output is correct
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

/* read io port */
int read_ioport(int addr, int *reg_val)
{
    int ret;

    /* set r/w permission of all 65536 ports */
    ret = iopl(3);
    if(ret < 0) {
        AIM_LOG_ERROR("unable to read cpu cpld version, iopl enable error %d\n", ret);
        return ONLP_STATUS_E_INTERNAL;
    }
    *reg_val = inb(addr);

    /* set r/w permission of  all 65536 ports */
    ret = iopl(0);
    if(ret < 0) {
        AIM_LOG_ERROR("unable to read cpu cpld version, iopl disable error %d\n", ret);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

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

/*
 * This function check the I2C bus statuas by using the sysfs of cpld_id,
 * If the I2C Bus is stcuk, do the i2c mux reset.
 */
void check_and_do_i2c_mux_reset(int port)
{
    char cmd_buf[256] = {0};
    int ret = 0;

    if(access("/sys/bus/i2c/devices/2-0030/cpld_id", F_OK) != -1 ) {

        snprintf(cmd_buf, sizeof(cmd_buf), "cat /sys/bus/i2c/devices/2-0030/cpld_id > /dev/null 2>&1");
        ret = system(cmd_buf);

        if(ret != 0) {
            if(access("/sys/devices/platform/x86_64_ufispace_s9300_32db_lpc/mb_cpld/mux_reset", F_OK) != -1 ) {
                //AIM_LOG_SYSLOG_WARN("I2C bus is stuck!! (port=%d)\r\n", port);
                snprintf(cmd_buf, sizeof(cmd_buf), "echo 0 > /sys/devices/platform/x86_64_ufispace_s9300_32db_lpc/mb_cpld/mux_reset 2> /dev/null");
                ret = system(cmd_buf);
                //AIM_LOG_SYSLOG_WARN("Do I2C mux reset!! (ret=%d)\r\n", ret);
            }
        }
    }
}
