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
#include <ctype.h>
#include "platform_lib.h"

bmc_info_t bmc_cache[] =
{
    [BMC_ATTR_ID_TEMP_ENV_CPU]   = {HW_PLAT_ALL, BMC_ATTR_NAME_TEMP_ENV_CPU   , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_TEMP_CPU_PECI]  = {HW_PLAT_ALL, BMC_ATTR_NAME_TEMP_CPU_PECI  , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_TEMP_ENV0]      = {HW_PLAT_ALL, BMC_ATTR_NAME_TEMP_ENV0      , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_TEMP_ENV1]      = {HW_PLAT_ALL, BMC_ATTR_NAME_TEMP_ENV1      , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_TEMP_ENV2]      = {HW_PLAT_ALL, BMC_ATTR_NAME_TEMP_ENV2      , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_TEMP_ENV3]      = {HW_PLAT_ALL, BMC_ATTR_NAME_TEMP_ENV3      , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_TEMP_ENV4]      = {HW_PLAT_ALL, BMC_ATTR_NAME_TEMP_ENV4      , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_TEMP_ENV5]      = {HW_PLAT_ALL, BMC_ATTR_NAME_TEMP_ENV5      , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_TEMP_ENV_RISER] = {HW_PLAT_ALL, BMC_ATTR_NAME_TEMP_ENV_RISER , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_TEMP_ENV_FAN0]  = {HW_PLAT_ALL, BMC_ATTR_NAME_TEMP_ENV_FAN0  , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_TEMP_ENV_FAN1]  = {HW_PLAT_ALL, BMC_ATTR_NAME_TEMP_ENV_FAN1  , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_PSU0_TEMP1]     = {HW_PLAT_ALL, BMC_ATTR_NAME_PSU0_TEMP1     , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_PSU1_TEMP1]     = {HW_PLAT_ALL, BMC_ATTR_NAME_PSU1_TEMP1     , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_FAN0_RPM_FRONT] = {HW_PLAT_ALL, BMC_ATTR_NAME_FAN0_RPM_FRONT , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_FAN0_RPM_REAR]  = {HW_PLAT_ALL, BMC_ATTR_NAME_FAN0_RPM_REAR  , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_FAN1_RPM_FRONT] = {HW_PLAT_ALL, BMC_ATTR_NAME_FAN1_RPM_FRONT , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_FAN1_RPM_REAR]  = {HW_PLAT_ALL, BMC_ATTR_NAME_FAN1_RPM_REAR  , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_FAN2_RPM_FRONT] = {HW_PLAT_ALL, BMC_ATTR_NAME_FAN2_RPM_FRONT , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_FAN2_RPM_REAR]  = {HW_PLAT_ALL, BMC_ATTR_NAME_FAN2_RPM_REAR  , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_FAN3_RPM_FRONT] = {HW_PLAT_ALL, BMC_ATTR_NAME_FAN3_RPM_FRONT , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_FAN3_RPM_REAR]  = {HW_PLAT_ALL, BMC_ATTR_NAME_FAN3_RPM_REAR  , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_PSU0_FAN]       = {HW_PLAT_ALL, BMC_ATTR_NAME_PSU0_FAN       , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_PSU1_FAN]       = {HW_PLAT_ALL, BMC_ATTR_NAME_PSU1_FAN       , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_FAN0_PRSNT_H]   = {HW_PLAT_ALL, BMC_ATTR_NAME_FAN0_PRSNT_H   , BMC_DATA_BOOL , 0},
    [BMC_ATTR_ID_FAN1_PRSNT_H]   = {HW_PLAT_ALL, BMC_ATTR_NAME_FAN1_PRSNT_H   , BMC_DATA_BOOL , 0},
    [BMC_ATTR_ID_FAN2_PRSNT_H]   = {HW_PLAT_ALL, BMC_ATTR_NAME_FAN2_PRSNT_H   , BMC_DATA_BOOL , 0},
    [BMC_ATTR_ID_FAN3_PRSNT_H]   = {HW_PLAT_ALL, BMC_ATTR_NAME_FAN3_PRSNT_H   , BMC_DATA_BOOL , 0},
    [BMC_ATTR_ID_PSU0_VIN]       = {HW_PLAT_ALL, BMC_ATTR_NAME_PSU0_VIN       , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_PSU0_VOUT]      = {HW_PLAT_ALL, BMC_ATTR_NAME_PSU0_VOUT      , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_PSU0_IIN]       = {HW_PLAT_ALL, BMC_ATTR_NAME_PSU0_IIN       , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_PSU0_IOUT]      = {HW_PLAT_ALL, BMC_ATTR_NAME_PSU0_IOUT      , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_PSU0_PIN]       = {HW_PLAT_ALL, BMC_ATTR_NAME_PSU0_PIN       , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_PSU0_POUT]      = {HW_PLAT_ALL, BMC_ATTR_NAME_PSU0_POUT      , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_PSU1_VIN]       = {HW_PLAT_ALL, BMC_ATTR_NAME_PSU1_VIN       , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_PSU1_VOUT]      = {HW_PLAT_ALL, BMC_ATTR_NAME_PSU1_VOUT      , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_PSU1_IIN]       = {HW_PLAT_ALL, BMC_ATTR_NAME_PSU1_IIN       , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_PSU1_IOUT]      = {HW_PLAT_ALL, BMC_ATTR_NAME_PSU1_IOUT      , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_PSU1_PIN]       = {HW_PLAT_ALL, BMC_ATTR_NAME_PSU1_PIN       , BMC_DATA_FLOAT, 0},
    [BMC_ATTR_ID_PSU1_POUT]      = {HW_PLAT_ALL, BMC_ATTR_NAME_PSU1_POUT      , BMC_DATA_FLOAT, 0},
};

static bmc_fru_t bmc_fru_cache[] =
{
    [BMC_FRU_IDX_ONLP_PSU_0] = {
        .bmc_fru_id = 1,
        .init_done = 0,
        .cache_files = "/tmp/bmc_fru_cache_1",
        .vendor   = {BMC_FRU_KEY_MANUFACTURER ,""},
        .name     = {BMC_FRU_KEY_NAME         ,""},
        .part_num = {BMC_FRU_KEY_PART_NUMBER  ,""},
        .serial   = {BMC_FRU_KEY_SERIAL       ,""},

    },
    [BMC_FRU_IDX_ONLP_PSU_1] = {
        .bmc_fru_id = 2,
        .init_done = 0,
        .cache_files = "/tmp/bmc_fru_cache_2",
        .vendor   = {BMC_FRU_KEY_MANUFACTURER ,""},
        .name     = {BMC_FRU_KEY_NAME         ,""},
        .part_num = {BMC_FRU_KEY_PART_NUMBER  ,""},
        .serial   = {BMC_FRU_KEY_SERIAL       ,""},
    },
    [BMC_FRU_IDX_FAN_TRAY_0] = {
        .bmc_fru_id = 3,
        .init_done = 0,
        .cache_files = "/tmp/bmc_fru_cache_3",
        .vendor   = {BMC_FRU_KEY_MANUFACTURER ,""},
        .name     = {BMC_FRU_KEY_NAME         ,""},
        .part_num = {BMC_FRU_KEY_PART_NUMBER  ,""},
        .serial   = {BMC_FRU_KEY_SERIAL       ,""},
    },
    [BMC_FRU_IDX_FAN_TRAY_1] = {
        .bmc_fru_id = 4,
        .init_done = 0,
        .cache_files = "/tmp/bmc_fru_cache_4",
        .vendor   = {BMC_FRU_KEY_MANUFACTURER ,""},
        .name     = {BMC_FRU_KEY_NAME         ,""},
        .part_num = {BMC_FRU_KEY_PART_NUMBER  ,""},
        .serial   = {BMC_FRU_KEY_SERIAL       ,""},
    },
    [BMC_FRU_IDX_FAN_TRAY_2] = {
        .bmc_fru_id = 5,
        .init_done = 0,
        .cache_files = "/tmp/bmc_fru_cache_5",
        .vendor   = {BMC_FRU_KEY_MANUFACTURER ,""},
        .name     = {BMC_FRU_KEY_NAME         ,""},
        .part_num = {BMC_FRU_KEY_PART_NUMBER  ,""},
        .serial   = {BMC_FRU_KEY_SERIAL       ,""},
    },
    [BMC_FRU_IDX_FAN_TRAY_3] = {
        .bmc_fru_id = 6,
        .init_done = 0,
        .cache_files = "/tmp/bmc_fru_cache_6",
        .vendor   = {BMC_FRU_KEY_MANUFACTURER ,""},
        .name     = {BMC_FRU_KEY_NAME         ,""},
        .part_num = {BMC_FRU_KEY_PART_NUMBER  ,""},
        .serial   = {BMC_FRU_KEY_SERIAL       ,""},
    }
};

/*
 *   FAN0 FAN1 FAN2 FANN
 *   PSU0 PSU1 PSUN
 */
static bmc_oem_data_t bmc_oem_table[] = {
    [BMC_OEM_IDX_INVALID] = {0},
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
    if(read_file_hex(&board->hw_rev, LPC_FMT "board_hw_id") != ONLP_STATUS_OK ||
       read_file_hex(&board->deph_id, LPC_FMT "board_deph_id") != ONLP_STATUS_OK ||
       read_file_hex(&board->hw_build, LPC_FMT "board_build_id") != ONLP_STATUS_OK ||
       read_file_hex(&board->sku_id, LPC_FMT "board_sku_id") != ONLP_STATUS_OK)
    {
        board->hw_rev = 0;
        board->deph_id = 0;
        board->hw_build = 0;
        board->sku_id = 0;
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

/**
 * @brief check bmc still alive
 * @returns ONLP_STATUS_OK         : bmc still alive
 *          ONLP_STATUS_E_INTERNAL : bmc not respond
 */
int check_bmc_alive(void)
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
            }else{
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

int read_bmc_sensor(int bmc_cache_index, int sensor_type, float *data)
{
    int cache_time = 0;
    int bmc_cache_expired = 0;
    int bmc_cache_change = 0;
    static long file_pre_time = 0;
    long file_last_time = 0;
    static int init_cache = 1;
    int rv = ONLP_STATUS_OK;

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

    board_t board = {0};
    ONLP_TRY(get_board_version(&board));

    ONLP_LOCK();

    if(check_file_exist(BMC_SENSOR_CACHE, &file_last_time)) {
        struct timeval new_tv = {0};
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
        if(bmc_cache_expired == 1) {
            // detect bmc status
            if(check_bmc_alive() != ONLP_STATUS_OK) {
                rv = ONLP_STATUS_E_INTERNAL;
                goto done;
            }

            // get bmc data
            char ipmi_cmd[1536] = {0};
            char bmc_token[1024] = {0};
            int i = 0;
            for(i = BMC_ATTR_ID_START; i <= BMC_ATTR_ID_LAST; i++) {
                int plat = 0;
                if(board.hw_rev == BRD_PROTO)
                    plat = HW_PLAT_PROTO;
                else if(board.hw_rev == BRD_ALPHA)
                    plat = HW_PLAT_ALPHA;
                else if(board.hw_rev == BRD_BETA)
                    plat = HW_PLAT_BETA;
                else if(board.hw_rev == BRD_PVT)
                    plat = HW_PLAT_PVT;
                else
                    plat = HW_PLAT_PVT;

                if(bmc_cache[i].plat & plat) {
                    char tmp_str[1024] = {0};
                    int copy_size = (sizeof(bmc_token) - strlen(bmc_token) - 1) >= 0? (sizeof(bmc_token) - strlen(bmc_token) - 1):0;
                    snprintf(tmp_str, sizeof(tmp_str), " %s", bmc_cache[i].name);
                    strncat(bmc_token, tmp_str, copy_size);
                }
            }

            snprintf(ipmi_cmd, sizeof(ipmi_cmd), CMD_BMC_SENSOR_CACHE, IPMITOOL_CMD_TIMEOUT, bmc_token);
            int retry = 0, retry_max = 2;
            for (retry = 0; retry < retry_max; ++retry) {
                int ret = 0;
                if((ret=system(ipmi_cmd)) != 0) {
                    if (retry == retry_max-1) {
                        AIM_LOG_ERROR("%s() write bmc sensor cache failed, retry=%d, cmd=%s, ret=%d",
                            __func__, retry, ipmi_cmd, ret);
                        rv = ONLP_STATUS_E_INTERNAL;
                        goto done;
                    } else {
                        continue;
                    }
                } else {
                    break;
                }
            }
        }

        //read sensor from cache file and save to bmc_cache
        FILE *fp = NULL;
        fp = fopen (BMC_SENSOR_CACHE, "r");
        if(fp == NULL) {
            AIM_LOG_ERROR("%s() open file failed, file=%s", __func__, BMC_SENSOR_CACHE);
            rv = ONLP_STATUS_E_INTERNAL;
            goto done;
        }

        //read file line by line
        char line[BMC_FRU_LINE_SIZE] = {'\0'};
        while(fgets(line,BMC_FRU_LINE_SIZE, fp) != NULL) {
            int i = 0;
            char *line_ptr = line;
            char *token = NULL;

            //parse line into fields. fields[0]: fields name, fields[1]: fields value
            char line_fields[BMC_FIELDS_MAX][BMC_FRU_ATTR_KEY_VALUE_SIZE] = {{0}};
            while ((token = strsep (&line_ptr, ",")) != NULL) {
                sscanf (token, "%[^\n]", line_fields[i++]);
            }

            //save bmc_cache from fields
            for(i=BMC_ATTR_ID_START; i<=BMC_ATTR_ID_LAST; ++i) {
                if(strcmp(line_fields[0], bmc_cache[i].name) == 0) {
                    if(bmc_cache[i].data_type == BMC_DATA_BOOL) {
                        /* fan present, got from bmc */
                        if( strstr(line_fields[4], "Present") != NULL ) {
                            bmc_cache[i].data = BMC_ATTR_STATUS_PRES;
                        } else {
                            bmc_cache[i].data = BMC_ATTR_STATUS_ABS;
                        }
                    } else {
                        /* other attribute, got from bmc */
                        if(strcmp(line_fields[1], "") == 0) {
                            bmc_cache[i].data = BMC_ATTR_INVALID_VAL;
                        } else {
                            bmc_cache[i].data = atof(line_fields[1]);
                        }
                    }
                    break;
                }
            }
        }
        fclose(fp);
        init_cache = 0;
    }

    //read from cache
    *data = bmc_cache[bmc_cache_index].data;

done:
    ONLP_UNLOCK();
    return rv;
}

/**
 * @brief bmc fru read
 * @param fru_id The fru id
 * @param data [out] The fru information.
 */
int read_bmc_fru(int fru_id, bmc_fru_t *data)
{
    struct timeval new_tv;
    int cache_time = PSU_CACHE_TIME;
    int bmc_cache_expired = 0;
    int bmc_cache_change = 0;
    static long file_pre_time = 0;
    long file_last_time = 0;
    int rv = ONLP_STATUS_OK;

    if((data == NULL)) {
        return ONLP_STATUS_E_INTERNAL;
    }

    switch(fru_id){
        case BMC_FRU_IDX_ONLP_PSU_0:
        case BMC_FRU_IDX_ONLP_PSU_1:
        case BMC_FRU_IDX_FAN_TRAY_0:
        case BMC_FRU_IDX_FAN_TRAY_1:
        case BMC_FRU_IDX_FAN_TRAY_2:
        case BMC_FRU_IDX_FAN_TRAY_3:
            break;
        default:
            return ONLP_STATUS_E_INTERNAL;
    }

    bmc_fru_t *fru = &bmc_fru_cache[fru_id];

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

            memset(fru->vendor.val, '\0', sizeof(fru->vendor.val));
            memset(fru->name.val, '\0', sizeof(fru->name.val));
            memset(fru->part_num.val, '\0', sizeof(fru->part_num.val));
            memset(fru->serial.val, '\0', sizeof(fru->serial.val));

            // detect bmc status
            if(check_bmc_alive() != ONLP_STATUS_OK) {
                rv = ONLP_STATUS_E_INTERNAL;
                goto done;
            }

            // get bmc data
            char ipmi_cmd[1536] = {0};
            char fields[256]="";
            snprintf(fields, sizeof(fields), "-e '%s' -e '%s' -e '%s' -e '%s'",
                        BMC_FRU_KEY_MANUFACTURER, BMC_FRU_KEY_NAME ,BMC_FRU_KEY_PART_NUMBER, BMC_FRU_KEY_SERIAL);

            snprintf(ipmi_cmd, sizeof(ipmi_cmd), CMD_FRU_CACHE_SET, IPMITOOL_CMD_TIMEOUT, fru->bmc_fru_id, fields, fru->cache_files);
            int retry = 0, retry_max = 2;
            for (retry = 0; retry < retry_max; ++retry) {
                int ret = 0;
                if ((ret = system(ipmi_cmd)) != 0) {
                    if (retry == retry_max-1) {
                        AIM_LOG_ERROR("%s() write bmc fru cache failed, retry=%d, cmd=%s, ret=%d",
                            __func__, retry, ipmi_cmd, ret);
                        rv = ONLP_STATUS_E_INTERNAL;
                        goto done;
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
            if(fscanf(fp ,"%[^:]:%[^\n]\n", key, val) != 2) {
                break;
            }

            trim_whitespace(key);
            trim_whitespace(val);

            if(strcmp(key, BMC_FRU_KEY_MANUFACTURER) == 0) {
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
        if (strnlen(fru->vendor.val, BMC_FRU_ATTR_KEY_VALUE_LEN) == 0 ) {
                strncpy(fru->vendor.val, COMM_STR_NOT_AVAILABLE, strnlen(COMM_STR_NOT_AVAILABLE, BMC_FRU_ATTR_KEY_VALUE_LEN));
        }


        if (strnlen(fru->name.val, BMC_FRU_ATTR_KEY_VALUE_LEN) == 0) {
                strncpy(fru->name.val, COMM_STR_NOT_AVAILABLE, strnlen(COMM_STR_NOT_AVAILABLE, BMC_FRU_ATTR_KEY_VALUE_LEN));
        }


        if (strnlen(fru->part_num.val, BMC_FRU_ATTR_KEY_VALUE_LEN) == 0) {
                strncpy(fru->part_num.val, COMM_STR_NOT_AVAILABLE, strnlen(COMM_STR_NOT_AVAILABLE, BMC_FRU_ATTR_KEY_VALUE_LEN));
        }


        if (strnlen(fru->serial.val, BMC_FRU_ATTR_KEY_VALUE_LEN) == 0) {
                strncpy(fru->serial.val, COMM_STR_NOT_AVAILABLE, strnlen(COMM_STR_NOT_AVAILABLE, BMC_FRU_ATTR_KEY_VALUE_LEN));
        }
    }

    //read from cache
    *data = *fru;

done:
    ONLP_UNLOCK();
    return rv;
}

int read_bmc_oem(int bmc_oem_id, int *data)
{
    struct timeval new_tv;
    FILE *fp = NULL;
    char ipmi_cmd[1024] = {0};
    int rv = ONLP_STATUS_OK;
    int bmc_cache_expired = 0;
    long file_last_time = 0;
    int retry = 0, retry_max = 2;

    if(data == NULL) {
        AIM_LOG_ERROR("Input param is NULL pointer\n");
        return ONLP_STATUS_E_INVALID;
    }

    switch (bmc_oem_id) {
        case BMC_OEM_IDX_INVALID:
        default:
            AIM_LOG_ERROR("Unsupport OEM ID(%d)\n", bmc_oem_id);
            return ONLP_STATUS_E_INVALID;
    }

    bmc_oem_data_t *oem_data = &bmc_oem_table[bmc_oem_id];

    ONLP_LOCK();

    if(check_file_exist(BMC_OEM_CACHE, &file_last_time)) {
        gettimeofday(&new_tv, NULL);
        if(bmc_cache_expired_check(file_last_time, new_tv.tv_sec, FANDIR_CACHE_TIME)) {
            bmc_cache_expired = 1;
        } else {
            bmc_cache_expired = 0;
        }
    } else {
        bmc_cache_expired = 1;
    }

    //update cache
    if(bmc_cache_expired == 1 || oem_data->init_done == 0) {
        if (bmc_cache_expired == 1) {
            // detect bmc status
            if(check_bmc_alive() != ONLP_STATUS_OK) {
                rv = ONLP_STATUS_E_INTERNAL;
                goto exit;
            }
            // get data from bmc
            snprintf(ipmi_cmd, sizeof(ipmi_cmd), CMD_BMC_OEM_CACHE, IPMITOOL_CMD_TIMEOUT, IPMITOOL_CMD_TIMEOUT);
            for (retry = 0; retry < retry_max; ++retry) {
                if ((rv=system(ipmi_cmd)) != 0) {
                    if (retry == retry_max-1) {
                        AIM_LOG_ERROR("%s() write bmc oem cache failed, retry=%d, cmd=%s, ret=%d",
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
        fp = fopen(BMC_OEM_CACHE, "r");
        if(fp == NULL) {
            AIM_LOG_ERROR("%s() open file failed, file=%s",
                            __func__, BMC_SENSOR_CACHE);
            rv = ONLP_STATUS_E_INTERNAL;
            goto exit;
        }

        //read file line by line
        char line[BMC_FRU_LINE_SIZE] = {'\0'};
        int raw;
        char *str_rv=NULL;
        int is_found=0;
        for(str_rv = fgets(line,BMC_FRU_LINE_SIZE, fp),raw=0;
            str_rv != NULL;
            str_rv = fgets(line,BMC_FRU_LINE_SIZE, fp),raw++) {
            char *line_ptr = line;
            char *token = NULL;

            if(raw != oem_data->raw_idx)
                continue;

            //parse line into fields.
            int col;
            char line_field[BMC_FRU_ATTR_KEY_VALUE_SIZE] = {0};
            for(token = strsep (&line_ptr, " "),col=0;
                token!=NULL;
                token = strsep (&line_ptr, " "),col++) {
                if(col != oem_data->col_idx) {
                    continue;
                }
                sscanf (token, "%[^\n]", line_field);
            }

            if(strlen(line_field) !=0) {
                oem_data->data = atoi(line_field);
                is_found = 1;
            }
        }
        fclose(fp);

        if(is_found == 1) {
            oem_data->init_done = 0;
        } else {
            rv = ONLP_STATUS_E_INTERNAL;
            AIM_LOG_ERROR("Failed to find OEM data(%d) raw(%d) col(%d)\n",
                             bmc_oem_id, bmc_oem_table[bmc_oem_id].raw_idx, bmc_oem_table[bmc_oem_id].col_idx);
        }
    }

    //read from cache
    *data = oem_data->data;
exit:
    ONLP_UNLOCK();
    return rv;
}

int read_ioport(int addr, int *reg_val) {

    /*set r/w permission of all 65536 ports*/
    ONLP_TRY(iopl(3));
    *reg_val = inb(addr);

    /*set r/w permission of  all 65536 ports*/
    ONLP_TRY(iopl(0));

    return ONLP_STATUS_OK;
}

int exec_cmd(char *cmd, char* out, int size) {
    FILE *fp = NULL;

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
    int rv = 0;
    va_list vargs;
    va_start(vargs, fmt);
    rv = vread_file_hex(value, fmt, vargs);
    va_end(vargs);
    return rv;
}

int vread_file_hex(int* value, const char* fmt, va_list vargs)
{
    uint8_t data[32];
    int len = 0;
    ONLP_TRY(onlp_file_vread(data, sizeof(data), &len, fmt, vargs));

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
    // Not support
    if(0) {
        char cmd_buf[256] = {0};
        int ret = 0;

        snprintf(cmd_buf, sizeof(cmd_buf), I2C_STUCK_CHECK_CMD);
        ret = system(cmd_buf);
        if(ret != 0) {
            if(access(MUX_RESET_PATH, F_OK) != -1 ) {
                //AIM_LOG_SYSLOG_WARN("I2C bus is stuck!! (port=%d)\r\n", port);
                memset(cmd_buf, 0, sizeof(cmd_buf));
                snprintf(cmd_buf, sizeof(cmd_buf), "echo 0 > %s 2> /dev/null", MUX_RESET_PATH);
                ret = system(cmd_buf);
                //AIM_LOG_SYSLOG_WARN("Do I2C mux reset!! (ret=%d)\r\n", ret);
            }
        }
    }
}

/* reg shift */
uint8_t shift_bit(uint8_t mask)
{
    int i = 0, mask_one = 1;

    for(i=0; i<8; ++i) {
        if((mask & mask_one) == 1)
            return i;
        else
            mask >>= 1;
    }

    return -1;
}

/* reg mask and shift */
uint8_t shift_bit_mask(uint8_t val, uint8_t mask)
{
    int shift = 0;

    shift = shift_bit(mask);

    return (val & mask) >> shift;
}

uint8_t unset_bit_mask(uint8_t val, uint8_t mask)
{
    return (val & ~mask);
}

uint8_t operate_bit(uint8_t reg_val, uint8_t bit, uint8_t bit_val)
{
    if(bit_val == 0)
        reg_val = reg_val & ~(1 << bit);
    else
        reg_val = reg_val | (1 << bit);
    return reg_val;
}

uint8_t get_bit_value(uint8_t reg_val, uint8_t bit)
{
    return (reg_val >> bit) & 0x1;
}

int get_hw_rev_id(void)
{
    int hw_rev;
    char hw_rev_cmd[128];
    char buffer[128];
    FILE *fp;

    snprintf(hw_rev_cmd, sizeof(hw_rev_cmd), "cat "LPC_FMT "board_hw_id");
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

int get_cpu_hw_rev_id(int *rev_id, int *dev_phase, int *build_id)
{
    if (rev_id == NULL || dev_phase == NULL || build_id == NULL) {
        AIM_LOG_ERROR("rev_id, dev_phase or build_id is NULL pointer");
        return ONLP_STATUS_E_PARAM;
    }

    // ONLP_TRY(read_file_hex(rev_id, "/sys/devices/platform/x86_64_ufispace_s9321_64e_lpc/ec/cpu_rev_hw_rev"));
    // ONLP_TRY(read_file_hex(dev_phase, "/sys/devices/platform/x86_64_ufispace_s9321_64e_lpc/ec/cpu_rev_dev_phase"));
    // ONLP_TRY(read_file_hex(build_id, "/sys/devices/platform/x86_64_ufispace_s9321_64e_lpc/ec/cpu_rev_build_id"));

    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Get gpio max
 * @param gpio_max [out] GPIO max
 */
int get_gpio_max(int *gpio_max)
{
    int rv = ONLP_STATUS_OK;

    if(gpio_max == NULL) {
        return ONLP_STATUS_E_INVALID;
    }

    if(read_file_hex(gpio_max, LPC_BSP_FMT "bsp_gpio_max") != ONLP_STATUS_OK)
    {
        *gpio_max = 511;
        rv = ONLP_STATUS_E_INVALID;
    }
    return rv;
}

/**
 * @brief Trim trailing whitespace
 * @param str [out] string without trailing whitespace
 */
int trim_whitespace(char *str)
{
    char *end;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator character
    end[1] = '\0';

    return ONLP_STATUS_OK;
}
