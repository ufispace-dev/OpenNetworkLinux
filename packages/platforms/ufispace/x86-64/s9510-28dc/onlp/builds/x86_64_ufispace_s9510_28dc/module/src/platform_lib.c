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
    [WARM_RESET_ALL] = {-1,      NULL, NULL},
    [WARM_RESET_MAC] = {MAC_MAX, "mac", mac_unit_str},
    [WARM_RESET_PHY] = {-1,      NULL, NULL}, //not support
    [WARM_RESET_MUX] = {-1,      NULL, NULL}, //not support
    [WARM_RESET_OP2] = {-1,      NULL, NULL}, //not support
    [WARM_RESET_GB]  = {-1,      NULL, NULL}, //not support
};

bmc_info_t bmc_cache[] =
{
    [BMC_ATTR_ID_TEMP_MAC]        = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_TEMP_MAC        , 0},
    [BMC_ATTR_ID_TEMP_DDR4]       = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_TEMP_DDR4       , 0},
    [BMC_ATTR_ID_TEMP_BMC]        = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_TEMP_BMC        , 0},
    [BMC_ATTR_ID_TEMP_FANCARD1]   = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_TEMP_FANCARD1   , 0},
    [BMC_ATTR_ID_TEMP_FANCARD2]   = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_TEMP_FANCARD2   , 0},
    [BMC_ATTR_ID_TEMP_FPGA_R]     = {(HW_PLAT_PREMIUM_EXT)                                 , BMC_ATTR_NAME_TEMP_FPGA_R     , 0},
    [BMC_ATTR_ID_TEMP_FPGA_L]     = {(HW_PLAT_PREMIUM_EXT)                                 , BMC_ATTR_NAME_TEMP_FPGA_L     , 0},
    [BMC_ATTR_ID_HWM_TEMP_GDDR]   = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_HWM_TEMP_GDDR   , 0},
    [BMC_ATTR_ID_HWM_TEMP_MAC]    = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_HWM_TEMP_MAC    , 0},
    [BMC_ATTR_ID_HWM_TEMP_AMB]    = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_HWM_TEMP_AMB    , 0},
    [BMC_ATTR_ID_HWM_TEMP_NTMCARD]= {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_HWM_TEMP_NTMCARD, 0},
    [BMC_ATTR_ID_PSU0_TEMP]       = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_PSU0_TEMP1      , 0},
    [BMC_ATTR_ID_PSU1_TEMP]       = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_PSU1_TEMP1      , 0},
    [BMC_ATTR_ID_FAN_0]           = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_FAN_0           , 0},
    [BMC_ATTR_ID_FAN_1]           = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_FAN_1           , 0},
    [BMC_ATTR_ID_FAN_2]           = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_FAN_2           , 0},
    [BMC_ATTR_ID_FAN_3]           = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_FAN_3           , 0},
    [BMC_ATTR_ID_FAN_4]           = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_FAN_4           , 0},
    [BMC_ATTR_ID_PSU0_FAN]        = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_PSU0_FAN        , 0},
    [BMC_ATTR_ID_PSU1_FAN]        = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_PSU1_FAN        , 0},
    [BMC_ATTR_ID_PSU0_VIN]        = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_PSU0_VIN        , 0},
    [BMC_ATTR_ID_PSU0_VOUT]       = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_PSU0_VOUT       , 0},
    [BMC_ATTR_ID_PSU0_IIN]        = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_PSU0_IIN        , 0},
    [BMC_ATTR_ID_PSU0_IOUT]       = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_PSU0_IOUT       , 0},
    [BMC_ATTR_ID_PSU0_STBVOUT]    = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_PSU0_STBVOUT    , 0},
    [BMC_ATTR_ID_PSU0_STBIOUT]    = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_PSU0_STBIOUT    , 0},
    [BMC_ATTR_ID_PSU1_VIN]        = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_PSU1_VIN        , 0},
    [BMC_ATTR_ID_PSU1_VOUT]       = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_PSU1_VOUT       , 0},
    [BMC_ATTR_ID_PSU1_IIN]        = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_PSU1_IIN        , 0},
    [BMC_ATTR_ID_PSU1_IOUT]       = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_PSU1_IOUT       , 0},
    [BMC_ATTR_ID_PSU1_STBVOUT]    = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_PSU1_STBVOUT    , 0},
    [BMC_ATTR_ID_PSU1_STBIOUT]    = {(HW_PLAT_PREMIUM_EXT|HW_PLAT_STANDARD|HW_PLAT_PREMIUM), BMC_ATTR_NAME_PSU1_STBIOUT    , 0}
};

static bmc_fru_t bmc_fru_cache[] =
{
    [ONLP_PSU_0] = {
        .bmc_fru_id = 1,
        .init_done = 0,
        .cache_files = "/tmp/bmc_fru_cache_1",
        .vendor   = {BMC_FRU_KEY_MANUFACTURER ,""},
        .name     = {BMC_FRU_KEY_NAME         ,""},
        .part_num = {BMC_FRU_KEY_PART_NUMBER  ,""},
        .serial   = {BMC_FRU_KEY_SERIAL       ,""},

    },
    [ONLP_PSU_1] = {
        .bmc_fru_id = 2,
        .init_done = 0,
        .cache_files = "/tmp/bmc_fru_cache_2",
        .vendor   = {BMC_FRU_KEY_MANUFACTURER ,""},
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

void lock_init()
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
int ufi_get_board_version(board_t *board)
{
    int rv = ONLP_STATUS_OK;

    if(board == NULL) {
        return ONLP_STATUS_E_INVALID;
    }

    //Get HW Version
    if(file_read_hex(&board->hw_rev, LPC_FMT "board_hw_id") != ONLP_STATUS_OK ||
       file_read_hex(&board->deph_id, LPC_FMT "board_deph_id") != ONLP_STATUS_OK ||
       file_read_hex(&board->hw_build, LPC_FMT "board_build_id") != ONLP_STATUS_OK ||
       file_read_hex(&board->rev_id, LPC_FMT "rev_id") != ONLP_STATUS_OK)
    {
        board->hw_rev = 0;
        board->deph_id = 0;
        board->hw_build = 0;
        board->rev_id = 0;
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

int bmc_sensor_read(int bmc_cache_index, int sensor_type, float *data)
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
    ONLP_TRY(ufi_get_board_version(&board));

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
            if(bmc_check_alive() != ONLP_STATUS_OK) {
                rv = ONLP_STATUS_E_INTERNAL;
                goto done;
            }

            // get bmc data
            char ipmi_cmd[1536] = {0};
            char bmc_token[1024] = {0};
            int i = 0;
            for(i = BMC_ATTR_ID_START; i <= BMC_ATTR_ID_LAST; i++) {
                int plat = 0;
                if(board.rev_id == HW_EXT_ID_PREMIUM_EXT)
                    plat = HW_PLAT_PREMIUM_EXT;
                else if(board.rev_id == HW_EXT_ID_STANDARD)
                    plat = HW_PLAT_STANDARD;
                else if(board.rev_id == HW_EXT_ID_PREMIUM)
                    plat = HW_PLAT_PREMIUM;
                else
                    plat = HW_PLAT_PREMIUM_EXT;

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
                    if(0) {
                        /* fan present, got from bmc */
                        if( strstr(line_fields[4], "Present") != NULL ) {
                            bmc_cache[i].data = 1;
                        } else {
                            bmc_cache[i].data = 0;
                        }
                    } else {
                        /* other attribut, got from bmc */
                        bmc_cache[i].data = atof(line_fields[1]);
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
 * @param local_id The psu local id
 * @param data [out] The psu fru information.
 */
int bmc_fru_read(int local_id, bmc_fru_t *data)
{
    struct timeval new_tv;
    int cache_time = PSU_CACHE_TIME;
    int bmc_cache_expired = 0;
    int bmc_cache_change = 0;
    static long file_pre_time = 0;
    long file_last_time = 0;
    int rv = ONLP_STATUS_OK;

    if((local_id != ONLP_PSU_0 && local_id != ONLP_PSU_1)  || (data == NULL)) {
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
            if(fscanf(fp ,"%[^:]:%s\n", key, val) != 2) {
                break;
            }

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
        if (strnlen(fru->vendor.val, BMC_FRU_ATTR_KEY_VALUE_LEN) == 0 ||
            strnlen(fru->name.val, BMC_FRU_ATTR_KEY_VALUE_LEN) == 0 ||
            strnlen(fru->part_num.val, BMC_FRU_ATTR_KEY_VALUE_LEN) == 0 ||
            strnlen(fru->serial.val, BMC_FRU_ATTR_KEY_VALUE_LEN) == 0) {
            AIM_LOG_ERROR("unable to read some fru info from BMC, fru id=%d, vendor=%s, product name=%s, part_num=%s, serial=%s",
                local_id, fru->vendor.val, fru->name.val, fru->part_num.val, fru->serial.val);
            rv = ONLP_STATUS_E_INTERNAL;
            goto done;
        }
    }

    //read from cache
    *data = *fru;

done:
    ONLP_UNLOCK();
    return rv;
}

int ufi_read_ioport(unsigned int addr, unsigned char *reg_val) {
    int ret;

    if(reg_val == NULL) {
        AIM_LOG_ERROR("reg_val is null");
        return ONLP_STATUS_E_PARAM;
    }

    if (addr < 0x0 || addr > 0xffff) {
        AIM_LOG_ERROR("Invalid addr, it should be 0x0 - 0xFFFF.");
        return ONLP_STATUS_E_PARAM;
    }

    /*enable io port*/
    ret = iopl(3);
    if(ret < 0) {
        AIM_LOG_ERROR("read_ioport() iopl enable error %d\n", ret);
        return ONLP_STATUS_E_INTERNAL;
    }

    *reg_val = inb(addr);

    /*disable io port*/
    ret = iopl(0);
    if(ret < 0) {
        AIM_LOG_ERROR("read_ioport() iopl disable error %d\n", ret);
        return ONLP_STATUS_E_INTERNAL;
    }
    return ONLP_STATUS_OK;
}

int ufi_write_ioport(unsigned int addr, unsigned char reg_val) {
    int ret;

    if (IS_INVALID_CPLD_ADDR(addr)) {
        AIM_LOG_ERROR("Invalid address, it should be 0x%X - 0x%X", CPLD_START_ADDR, CPLD_END_ADDR);
        return ONLP_STATUS_E_PARAM;
    }

    /*enable io port*/
    ret = iopl(3);
    if(ret < 0){
        AIM_LOG_ERROR("write_ioport() iopl enable error %d\n", ret);
        return ONLP_STATUS_E_INTERNAL;
    }

    outb(reg_val, addr);

    /*disable io port*/
    ret = iopl(0);
    if(ret < 0){
        AIM_LOG_ERROR("write_ioport(), iopl disable error %d\n", ret);
        return ONLP_STATUS_E_INTERNAL;
    }
    return ONLP_STATUS_OK;
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
    // only support beta and later
    if(get_hw_rev_id() > 1) {
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
uint8_t ufi_shift(uint8_t mask)
{
    int i=0, mask_one=1;

    for(i=0; i<8; ++i) {
        if((mask & mask_one) == 1)
            return i;
        else
            mask >>= 1;
    }

    return -1;
}

/* reg mask and shift */
uint8_t ufi_mask_shift(uint8_t val, uint8_t mask)
{
    int shift=0;

    shift = ufi_shift(mask);

    return (val & mask) >> shift;
}

uint8_t ufi_bit_operation(uint8_t reg_val, uint8_t bit, uint8_t bit_val)
{
    if(bit_val == 0)
        reg_val = reg_val & ~(1 << bit);
    else
        reg_val = reg_val | (1 << bit);
    return reg_val;
}

int get_hw_rev_id(void)
{
    int hw_rev;
    char hw_rev_cmd[128];
    char buffer[128];
    FILE *fp;

    snprintf(hw_rev_cmd, sizeof(hw_rev_cmd), "cat /sys/devices/platform/x86_64_ufispace_s9510_28dc_lpc/mb_cpld/board_hw_id");
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

int ufi_get_cpu_hw_rev_id(int *rev_id, int *dev_phase, int *build_id)
{
    if (rev_id == NULL || dev_phase == NULL || build_id == NULL) {
        AIM_LOG_ERROR("rev_id, dev_phase or build_id is NULL pointer");
        return ONLP_STATUS_E_PARAM;
    }

    ONLP_TRY(file_read_hex(rev_id, "/sys/devices/platform/x86_64_ufispace_s9510_28dc_lpc/ec/cpu_rev_hw_rev"));
    ONLP_TRY(file_read_hex(dev_phase, "/sys/devices/platform/x86_64_ufispace_s9510_28dc_lpc/ec/cpu_rev_dev_phase"));
    ONLP_TRY(file_read_hex(build_id, "/sys/devices/platform/x86_64_ufispace_s9510_28dc_lpc/ec/cpu_rev_build_id"));

    return ONLP_STATUS_OK;
}

int ufi_get_thermal_thld(int thermal_local_id,  temp_thld_t *temp_thld) {

    board_t board = {0};
    if(temp_thld == NULL) {
        return ONLP_STATUS_E_INVALID;
    }

    ONLP_TRY(ufi_get_board_version(&board));
    if(board.rev_id == HW_EXT_ID_PREMIUM_EXT) {
        switch(thermal_local_id) {
            case ONLP_THERMAL_FPGA_R:
                temp_thld->warning = THERMAL_STATE_NOT_SUPPORT;
                temp_thld->error = THERMAL_FPGA_R_ERROR;
                temp_thld->shutdown = THERMAL_FPGA_R_SHUTDOWN;
                return ONLP_STATUS_OK;
            case ONLP_THERMAL_FPGA_L:
                temp_thld->warning = THERMAL_STATE_NOT_SUPPORT;
                temp_thld->error = THERMAL_FPGA_L_ERROR;
                temp_thld->shutdown = THERMAL_FPGA_L_SHUTDOWN;
                return ONLP_STATUS_OK;
            default:
                break;
        }
    }

    if(board.rev_id == HW_EXT_ID_STANDARD) {
        switch(thermal_local_id) {
            case ONLP_THERMAL_CPU_PKG:
                temp_thld->warning = THERMAL_CPU_STD_WARNING;
                temp_thld->error = THERMAL_CPU_STD_ERROR;
                temp_thld->shutdown = THERMAL_CPU_STD_SHUTDOWN;
                return ONLP_STATUS_OK;
            default:
                break;
        }
    }

    if(board.rev_id == HW_EXT_ID_PREMIUM_EXT || board.rev_id == HW_EXT_ID_PREMIUM) {
        switch(thermal_local_id) {
            case ONLP_THERMAL_CPU_PKG:
                temp_thld->warning = THERMAL_CPU_WARNING;
                temp_thld->error = THERMAL_CPU_ERROR;
                temp_thld->shutdown = THERMAL_CPU_SHUTDOWN;
                return ONLP_STATUS_OK;
            default:
                break;
        }
    }

    switch(thermal_local_id) {
        case ONLP_THERMAL_MAC:
            temp_thld->warning = THERMAL_STATE_NOT_SUPPORT;
            temp_thld->error = THERMAL_MAC_ERROR;
            temp_thld->shutdown = THERMAL_MAC_SHUTDOWN;
            return ONLP_STATUS_OK;
        case ONLP_THERMAL_DDR4:
            temp_thld->warning = THERMAL_STATE_NOT_SUPPORT;
            temp_thld->error = THERMAL_DDR4_ERROR;
            temp_thld->shutdown = THERMAL_DDR4_SHUTDOWN;
            return ONLP_STATUS_OK;
        case ONLP_THERMAL_BMC:
            temp_thld->warning = THERMAL_STATE_NOT_SUPPORT;
            temp_thld->error = THERMAL_BMC_ERROR;
            temp_thld->shutdown = THERMAL_BMC_SHUTDOWN;
            return ONLP_STATUS_OK;
        case ONLP_THERMAL_FANCARD1:
            temp_thld->warning = THERMAL_STATE_NOT_SUPPORT;
            temp_thld->error = THERMAL_FANCARD1_ERROR;
            temp_thld->shutdown = THERMAL_FANCARD1_SHUTDOWN;
            return ONLP_STATUS_OK;
        case ONLP_THERMAL_FANCARD2:
            temp_thld->warning = THERMAL_STATE_NOT_SUPPORT;
            temp_thld->error = THERMAL_FANCARD2_ERROR;
            temp_thld->shutdown = THERMAL_FANCARD2_SHUTDOWN;
            return ONLP_STATUS_OK;
        case ONLP_THERMAL_HWM_GDDR:
            temp_thld->warning = THERMAL_STATE_NOT_SUPPORT;
            temp_thld->error = THERMAL_HWM_GDDR_ERROR;
            temp_thld->shutdown = THERMAL_HWM_GDDR_SHUTDOWN;
            return ONLP_STATUS_OK;
        case ONLP_THERMAL_HWM_MAC:
            temp_thld->warning = THERMAL_STATE_NOT_SUPPORT;
            temp_thld->error = THERMAL_HWM_MAC_ERROR;
            temp_thld->shutdown = THERMAL_HWM_MAC_SHUTDOWN;
            return ONLP_STATUS_OK;
        case ONLP_THERMAL_HWM_AMB:
            temp_thld->warning = THERMAL_STATE_NOT_SUPPORT;
            temp_thld->error = THERMAL_HWM_AMB_ERROR;
            temp_thld->shutdown = THERMAL_HWM_AMB_SHUTDOWN;
            return ONLP_STATUS_OK;
        case ONLP_THERMAL_HWM_NTMCARD:
            temp_thld->warning = THERMAL_STATE_NOT_SUPPORT;
            temp_thld->error = THERMAL_HWM_NTMCARD_ERROR;
            temp_thld->shutdown = THERMAL_HWM_NTMCARD_SHUTDOWN;
            return ONLP_STATUS_OK;
        case ONLP_THERMAL_PSU_0:
            temp_thld->warning = THERMAL_PSU_0_WARNING;
            temp_thld->error = THERMAL_PSU_0_ERROR;
            temp_thld->shutdown = THERMAL_PSU_0_SHUTDOWN;
            return ONLP_STATUS_OK;
        case ONLP_THERMAL_PSU_1:
            temp_thld->warning = THERMAL_PSU_1_WARNING;
            temp_thld->error = THERMAL_PSU_1_ERROR;
            temp_thld->shutdown = THERMAL_PSU_1_SHUTDOWN;
            return ONLP_STATUS_OK;
        default:
            break;
    }

    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Get gpio max
 * @param gpio_max [out] GPIO max
 */
int ufi_get_gpio_max(int *gpio_max)
{
    int rv = ONLP_STATUS_OK;

    if(gpio_max == NULL) {
        return ONLP_STATUS_E_INVALID;
    }

    if(file_read_hex(gpio_max, LPC_BSP_FMT "bsp_gpio_max") != ONLP_STATUS_OK)
    {
        *gpio_max = 511;
        rv = ONLP_STATUS_E_INVALID;
    }
    return rv;
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
