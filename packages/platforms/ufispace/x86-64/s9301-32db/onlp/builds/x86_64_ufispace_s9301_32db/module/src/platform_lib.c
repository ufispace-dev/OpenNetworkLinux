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

const int CPLD_BASE_ADDR[] = {0x30, 0x31, 0x32};

#define FAN_CACHE_TIME          10
#define PSU_CACHE_TIME          30
#define THERMAL_CACHE_TIME      10
#define FANDIR_CACHE_TIME       60
#define IPMITOOL_CMD_TIMEOUT    "10s"

#define BMC_SENSOR_CACHE        "/tmp/bmc_sensor_cache"
#define BMC_FANDIR_CACHE        "/tmp/bmc_fandir_cache"

#define BMC_FRU_KEY_MANUF       "Product Manufacturer"
#define BMC_FRU_KEY_NAME        "Product Name"
#define BMC_FRU_KEY_PART_NUMBER "Product Part Number"
#define BMC_FRU_KEY_SERIAL      "Product Serial"

#define CMD_FRU_CACHE_SET       "timeout "IPMITOOL_CMD_TIMEOUT" ipmitool fru print %d " \
                                IPMITOOL_REDIRECT_ERR \
                                " | grep %s" \
                                " | awk -F: '/:/{gsub(/^ /,\"\", $0);gsub(/ +:/,\":\",$0);gsub(/: +/,\":\", $0);print $0}'" \
                                " > %s"
#define CMD_BMC_FAN_TRAY_DIR    "timeout "IPMITOOL_CMD_TIMEOUT" ipmitool raw 0x3c 0x31 0x0 | xargs"IPMITOOL_REDIRECT_ERR
#define CMD_BMC_PSU_FAN_DIR     "timeout "IPMITOOL_CMD_TIMEOUT" ipmitool raw 0x3c 0x30 0x0 | xargs"IPMITOOL_REDIRECT_ERR
#define CMD_BMC_FAN_DIR_CACHE   CMD_BMC_FAN_TRAY_DIR" > "BMC_FANDIR_CACHE";"CMD_BMC_PSU_FAN_DIR" >> "BMC_FANDIR_CACHE
//[BMC] 2.5.0
#define CMD_BMC_SENSOR_CACHE    "timeout "IPMITOOL_CMD_TIMEOUT" ipmitool sdr -c get "\
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
                                " > "BMC_SENSOR_CACHE IPMITOOL_REDIRECT_ERR

#define CPLD_ID_SYSFS_PATH      CPLD1_SYSFS_PATH "/cpld_id"
#define MUX_RESET_SYSFS_PATH    LPC_MB_CPLD_PATH "/mux_reset"

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

static bmc_fru_t bmc_fru_cache[] =
{
    [ONLP_PSU0] = {
        .bmc_fru_id = 1,
        .init_done = 0,
        .cache_files = "/tmp/bmc_fru_cache_1",
        .vendor   = {BMC_FRU_KEY_MANUF        ,""},
        .name     = {BMC_FRU_KEY_NAME         ,""},
        .part_num = {BMC_FRU_KEY_PART_NUMBER  ,""},
        .serial   = {BMC_FRU_KEY_SERIAL       ,""},

    },
    [ONLP_PSU1] = {
        .bmc_fru_id = 2,
        .init_done = 0,
        .cache_files = "/tmp/bmc_fru_cache_2",
        .vendor   = {BMC_FRU_KEY_MANUF        ,""},
        .name     = {BMC_FRU_KEY_NAME         ,""},
        .part_num = {BMC_FRU_KEY_PART_NUMBER  ,""},
        .serial   = {BMC_FRU_KEY_SERIAL       ,""},
    },
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

void lock_init() {
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

/**
 * @brief check bmc still alive
 * @returns ONLP_STATUS_OK         : bmc still alive
 *          ONLP_STATUS_E_INTERNAL : bmc not respond
 */
int bmc_check_alive(void)
{
    /**
     *   BMC detect timeout get from "ipmitool mc info" test.
     *   Test Case: Run 100 times of "ipmitool mc info" command and get the execution times.
     *              We take 3s as The detect timeout value,
     *              since the execution times value is between 0.015s - 0.062s.
     */
    char* bmc_dect = "timeout 3s ipmitool mc info > /dev/null 2>&1";

    int retry = 0, retry_max = 2;
    for (retry = 0; retry < retry_max; ++retry) {
        int ret = 0;
        if((ret=system(bmc_dect)) != 0) {
            if (retry == retry_max-1) {
                AIM_LOG_ERROR("%s() bmc detecting fail, retry=%d, ret=%d",
                    __func__, retry, ret);
                return ONLP_STATUS_E_INTERNAL;
            } else {
                continue;
            }
        } else {
            break;
        }
    }

    return ONLP_STATUS_OK;
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

int bmc_fan_dir_read(int bmc_cache_index, float *data)
{
    struct timeval new_tv;
    FILE *fp = NULL;
    char ipmi_cmd[1024] = {0};
    int rv = ONLP_STATUS_OK;
    int bmc_cache_expired = 0;
    int bmc_cache_change = 0;
    static long file_pre_time = 0;
    long file_last_time = 0;
    static int init_cache = 1;
    int retry = 0, retry_max = 2;
    char line[BMC_FRU_LINE_SIZE] = {'\0'};
    char *line_ptr = NULL;
    char line_fields[20][BMC_FRU_ATTR_KEY_VALUE_SIZE];
    char seps[] = " ";
    char *token;
    int i = 0;

    ONLP_LOCK();

    if(check_file_exist(BMC_FANDIR_CACHE, &file_last_time)) {
        gettimeofday(&new_tv, NULL);
        if(bmc_cache_expired_check(file_last_time, new_tv.tv_sec, FANDIR_CACHE_TIME)) {
            bmc_cache_expired = 1;
        } else {
            if(file_pre_time != file_last_time) {
                file_pre_time = file_last_time;
                bmc_cache_change = 1;
            }
            bmc_cache_expired = 0;
        }
    } else {
        bmc_cache_expired = 1;
    }

    //update cache
    if(bmc_cache_expired == 1 || init_cache == 1 || bmc_cache_change == 1) {
        if (bmc_cache_expired == 1) {
            // detect bmc status
            if(bmc_check_alive() != ONLP_STATUS_OK) {
                rv = ONLP_STATUS_E_INTERNAL;
                goto exit;
            }
            // get data from bmc
            snprintf(ipmi_cmd, sizeof(ipmi_cmd), CMD_BMC_FAN_DIR_CACHE);
            for (retry = 0; retry < retry_max; ++retry) {
                if ((rv=system(ipmi_cmd)) != 0) {
                    if (retry == retry_max-1) {
                        AIM_LOG_ERROR("%s() write bmc fan direction cache failed, retry=%d, cmd=%s, ret=%d",
                            __func__, retry, ipmi_cmd, rv);
                        rv = ONLP_STATUS_E_INTERNAL;
                        goto exit;
                    } else {
                        continue;
                    }
                } else {
                    break;
                }
            }
        }

        //read sensor from cache file and save to bmc_cache
        fp = fopen(BMC_FANDIR_CACHE, "r");
        if(fp == NULL) {
            AIM_LOG_ERROR("%s() open file failed, file=%s",
                            __func__, BMC_SENSOR_CACHE);
            rv = ONLP_STATUS_E_INTERNAL;
            goto exit;
        }

        //read file line for fan tray dir parsing
        if(fgets(line,BMC_FRU_LINE_SIZE,fp) != NULL) {
            line_ptr = line;
            token = NULL;
            i = 0;
            //parse line into fields
            while((token = strsep (&line_ptr, seps)) != NULL) {
                sscanf(token, "%[^\n]", line_fields[i++]);
            }

            for(i=BMC_ATTR_ID_FAN0_DIR; i<=BMC_ATTR_ID_FAN5_DIR; i++) {
                bmc_cache[i].data = atof(line_fields[i-BMC_ATTR_ID_FAN0_DIR]);
            }

        }

        //read file line for psu fan dir parsing
        if(fgets(line,BMC_FRU_LINE_SIZE,fp) != NULL) {
            line_ptr = line;
            token = NULL;
            i = 0;
            //parse line into fields
            memset(line_fields, 0, sizeof(line_fields));
            while((token = strsep (&line_ptr, seps)) != NULL) {
                sscanf(token, "%[^\n]", line_fields[i++]);
            }

            for(i=BMC_ATTR_ID_PSU0_FAN1_DIR; i<=BMC_ATTR_ID_PSU1_FAN1_DIR; i++) {
                bmc_cache[i].data = atof(line_fields[i-BMC_ATTR_ID_PSU0_FAN1_DIR]);
            }

        }

        fclose(fp);
        init_cache = 0;

    }

    //read from cache
    *data = bmc_cache[bmc_cache_index].data;

exit:
    ONLP_UNLOCK();
    return rv;
}

int bmc_sensor_read(int bmc_cache_index, int sensor_type, float *data)
{
    struct timeval new_tv;
    FILE *fp = NULL;
    char ipmi_cmd[1024] = {0};
    int rv = ONLP_STATUS_OK;
    int dev_num = 0;
    int cache_time = 0;
    int bmc_cache_expired = 0;
    int bmc_cache_change = 0;
    static long file_pre_time = 0;
    float f_rv = 0;
    long file_last_time = 0;
    static int init_cache = 1;
    char* presence_str = "Present";
    int retry = 0, retry_max = 2;
    char line[BMC_FRU_LINE_SIZE] = {'\0'};
    char *line_ptr = NULL;
    char line_fields[20][BMC_FRU_ATTR_KEY_VALUE_SIZE];
    char seps[] = ",";
    char *token;
    int i = 0;

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

    ONLP_LOCK();

    if(check_file_exist(BMC_SENSOR_CACHE, &file_last_time)) {
        gettimeofday(&new_tv, NULL);
        if(bmc_cache_expired_check(file_last_time, new_tv.tv_sec, cache_time)) {
            bmc_cache_expired = 1;
        } else {
            if(file_pre_time != file_last_time) {
                file_pre_time = file_last_time;
                bmc_cache_change = 1;
            }

            bmc_cache_expired = 0;
        }
    } else {
        bmc_cache_expired = 1;
    }

    //update cache
    if(bmc_cache_expired == 1 || init_cache == 1 || bmc_cache_change == 1) {
        if (bmc_cache_expired == 1) {
            // detect bmc status
            if(bmc_check_alive() != ONLP_STATUS_OK) {
                rv = ONLP_STATUS_E_INTERNAL;
                goto exit;
            }
            // get data from bmc
            snprintf(ipmi_cmd, sizeof(ipmi_cmd), CMD_BMC_SENSOR_CACHE);
            for (retry = 0; retry < retry_max; ++retry) {
                if ((rv=system(ipmi_cmd)) != 0) {
                    if (retry == retry_max-1) {
                        AIM_LOG_ERROR("%s() write bmc sensor cache failed, retry=%d, cmd=%s, ret=%d",
                            __func__, retry, ipmi_cmd, rv);
                        rv = ONLP_STATUS_E_INTERNAL;
                        goto exit;
                    } else {
                        continue;
                    }
                } else {
                    break;
                }
            }
        }

        //read sensor from cache file and save to bmc_cache
        fp = fopen(BMC_SENSOR_CACHE, "r");
        if(fp == NULL) {
            AIM_LOG_ERROR("%s() open file failed, file=%s",
                            __func__, BMC_SENSOR_CACHE);
            rv = ONLP_STATUS_E_INTERNAL;
            goto exit;
        }

        //read file line by line
        while(fgets(line,BMC_FRU_LINE_SIZE,fp) != NULL) {
            i=0;
            line_ptr = line;
            token = NULL;

            //parse line into fields
            while((token = strsep (&line_ptr, seps)) != NULL) {
                sscanf(token, "%[^\n]", line_fields[i++]);
            }

            //save bmc_cache from fields
            for(i=0; i<BMC_ATTR_ID_MAX; ++i) {
                if(strcmp(line_fields[0], bmc_cache[i].name) == 0) {
                    dev_num = i;
                    if(dev_num >= BMC_ATTR_ID_FAN0_PSNT_L && dev_num <= BMC_ATTR_ID_FAN5_PSNT_L) {
                        if( strstr(line_fields[4], presence_str) != NULL ) {
                            f_rv = 1;
                        } else {
                            f_rv = 0;
                        }
                        bmc_cache[dev_num].data = f_rv;
                    } else {
                        f_rv = atof(line_fields[1]);
                        bmc_cache[dev_num].data = f_rv;
                     }
                     break;
                }
            }

            //reset field for next loop
            memset(line_fields[0], 0, sizeof(line_fields[0])); //sensor name
            memset(line_fields[1], 0, sizeof(line_fields[1])); //sensor value
            memset(line_fields[4], 0, sizeof(line_fields[4])); //sensor presence
        }
        fclose(fp);
        init_cache = 0;

    }

    //read from cache
    *data = bmc_cache[bmc_cache_index].data;


exit:
    ONLP_UNLOCK();
    return rv;
}

/**
 * @brief bmc fru read
 * @param local_id The psu local id
 * @param[out] data The psu fru information.
 */
int bmc_fru_read(int local_id, bmc_fru_t *data)
{
    struct timeval new_tv;
    char ipmi_cmd[1024] = {0};
    int cache_time = PSU_CACHE_TIME;
    int bmc_cache_expired = 0;
    int bmc_cache_change = 0;
    static long file_pre_time = 0;
    long file_last_time = 0;
    int rv = ONLP_STATUS_OK;

    if((local_id != ONLP_PSU0 && local_id != ONLP_PSU1)  || (data == NULL)) {
        return ONLP_STATUS_E_INTERNAL;
    }

    bmc_fru_t *fru = &bmc_fru_cache[local_id];

    ONLP_LOCK();

    if(check_file_exist(fru->cache_files, &file_last_time)) {
        gettimeofday(&new_tv, NULL);
        if(bmc_cache_expired_check(file_last_time, new_tv.tv_sec, cache_time)) {
            bmc_cache_expired = 1;
        } else {
            if(file_pre_time != file_last_time) {
                file_pre_time = file_last_time;
                bmc_cache_change = 1;
            }
            bmc_cache_expired = 0;
        }
    } else {
        bmc_cache_expired = 1;
    }

    //update cache
    if(bmc_cache_expired == 1 || fru->init_done == 0 || bmc_cache_change == 1) {
        //get fru from ipmitool and save to cache file
        if(bmc_cache_expired == 1) {
            // detect bmc status
            if(bmc_check_alive() != ONLP_STATUS_OK) {
                rv = ONLP_STATUS_E_INTERNAL;
                goto exit;
            }
            // get data from bmc
            char fields[256]="";
            snprintf(fields, sizeof(fields), "-e '%s' -e '%s' -e '%s' -e '%s'",
                        BMC_FRU_KEY_MANUF, BMC_FRU_KEY_NAME ,BMC_FRU_KEY_PART_NUMBER, BMC_FRU_KEY_SERIAL);
            snprintf(ipmi_cmd, sizeof(ipmi_cmd), CMD_FRU_CACHE_SET, fru->bmc_fru_id, fields, fru->cache_files);
            int retry = 0, retry_max = 2;
            for (retry = 0; retry < retry_max; ++retry) {
                int rv = 0;
                if ((rv=system(ipmi_cmd)) != 0) {
                    if (retry == retry_max-1) {
                        AIM_LOG_ERROR("%s() write bmc fru cache failed, retry=%d, cmd=%s, ret=%d",
                            __func__, retry, ipmi_cmd, rv);
                        rv = ONLP_STATUS_E_INTERNAL;
                        goto exit;
                    } else {
                        continue;
                    }
                } else {
                    break;
                }
            }
        }

        //read fru from cache file and save to bmc_fru_cache
        FILE *fp = NULL;
        fp = fopen (fru->cache_files, "r");
        while(1) {
            char key[BMC_FRU_ATTR_KEY_VALUE_SIZE] = {'\0'};
            char val[BMC_FRU_ATTR_KEY_VALUE_SIZE] = {'\0'};
            if(fscanf(fp ,"%[^:]:%s\n", key, val) != 2) {
                break;
            }

            if(strcmp(key, BMC_FRU_KEY_MANUF) == 0) {
                memset(fru->vendor.val, '\0', sizeof(fru->vendor.val));
                strncpy(fru->vendor.val, val, strnlen(val, BMC_FRU_ATTR_KEY_VALUE_LEN));
            }

            if(strcmp(key, BMC_FRU_KEY_NAME) == 0) {
                memset(fru->name.val, '\0', sizeof(fru->name.val));
                strncpy(fru->name.val, val, strnlen(val, BMC_FRU_ATTR_KEY_VALUE_LEN));
            }

            if(strcmp(key, BMC_FRU_KEY_PART_NUMBER) == 0) {
                memset(fru->part_num.val, '\0', sizeof(fru->part_num.val));
                strncpy(fru->part_num.val, val, strnlen(val, BMC_FRU_ATTR_KEY_VALUE_LEN));
            }

            if(strcmp(key, BMC_FRU_KEY_SERIAL) == 0) {
                memset(fru->serial.val, '\0', sizeof(fru->serial.val));
                strncpy(fru->serial.val, val, strnlen(val, BMC_FRU_ATTR_KEY_VALUE_LEN));
            }

        }

        fclose(fp);
        fru->init_done = 1;

        //Check output is correct
        if (strnlen(fru->vendor.val, BMC_FRU_ATTR_KEY_VALUE_LEN) == 0 ||
            strnlen(fru->name.val, BMC_FRU_ATTR_KEY_VALUE_LEN) == 0 ||
            strnlen(fru->part_num.val, BMC_FRU_ATTR_KEY_VALUE_LEN) == 0 ||
            strnlen(fru->serial.val, BMC_FRU_ATTR_KEY_VALUE_LEN) == 0) {
            AIM_LOG_ERROR("unable to read some fru info from BMC, fru id=%d, vendor=%s, product name=%s, part_num=%s, serial=%s",
                local_id, fru->vendor.val, fru->name.val, fru->part_num.val, fru->serial.val);
            rv = ONLP_STATUS_E_INTERNAL;
            goto exit;
        }
    }

    //read from cache
    *data = *fru;

exit:
    ONLP_UNLOCK();
    return rv;
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

    if(access(CPLD_ID_SYSFS_PATH, F_OK) != -1 ) {

        snprintf(cmd_buf, sizeof(cmd_buf), "cat " CPLD_ID_SYSFS_PATH " > /dev/null 2>&1");
        ret = system(cmd_buf);

        if(ret != 0) {
            if(access(MUX_RESET_SYSFS_PATH, F_OK) != -1 ) {
                //AIM_LOG_SYSLOG_WARN("I2C bus is stuck!! (port=%d)\r\n", port);
                snprintf(cmd_buf, sizeof(cmd_buf), "echo 0 > " MUX_RESET_SYSFS_PATH " 2> /dev/null");
                ret = system(cmd_buf);
                //AIM_LOG_SYSLOG_WARN("Do I2C mux reset!! (ret=%d)\r\n", ret);
            }
        }
    }
}
