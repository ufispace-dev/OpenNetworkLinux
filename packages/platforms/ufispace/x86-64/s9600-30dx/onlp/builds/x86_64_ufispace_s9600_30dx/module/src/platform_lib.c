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
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "platform_lib.h"

/* SYS */
const int CPLD_BASE_ADDR[CPLD_MAX] = {0x30, 0x31, 0x32};
const int CPLD_I2C_BUS = 1;

bmc_info_t beta_bmc_cache[] =
{
    [BMC_ATTR_ID_TEMP_ENV_CPU] = {BETA_TEMP_ENV_CPU, 0},
    [BMC_ATTR_ID_TEMP_CPU_PECI] = {"TEMP_CPU_PECI", 0},
    [BMC_ATTR_ID_TEMP_ENV_MAC0] = {BETA_TEMP_ENV_MAC0, 0},
    [BMC_ATTR_ID_TEMP_MAC0] = {BETA_TEMP_MAC0, 0},
    [BMC_ATTR_ID_TEMP_ENV_MAC1] = {BETA_TEMP_ENV_MAC1, 0},
    [BMC_ATTR_ID_TEMP_MAC1] = {BETA_TEMP_MAC1, 0},
    [BMC_ATTR_ID_PSU0_TEMP] = {BETA_PSU0_TEMP, 0},
    [BMC_ATTR_ID_PSU1_TEMP] = {BETA_PSU1_TEMP, 0},
    [BMC_ATTR_ID_FAN0_RPM] = {"FAN0_RPM", 0},
    [BMC_ATTR_ID_FAN1_RPM] = {"FAN1_RPM", 0},
    [BMC_ATTR_ID_FAN2_RPM] = {"FAN2_RPM", 0},
    [BMC_ATTR_ID_FAN3_RPM] = {"FAN3_RPM", 0},
    [BMC_ATTR_ID_PSU0_FAN] = {"PSU0_FAN", 0},
    [BMC_ATTR_ID_PSU1_FAN] = {"PSU1_FAN", 0},
    [BMC_ATTR_ID_FAN0_PRSNT_H] = {"FAN0_PRSNT_H",0},
    [BMC_ATTR_ID_FAN1_PRSNT_H] = {"FAN1_PRSNT_H",0},
    [BMC_ATTR_ID_FAN2_PRSNT_H] = {"FAN2_PRSNT_H", 0},
    [BMC_ATTR_ID_FAN3_PRSNT_H] = {"FAN3_PRSNT_H", 0},
    [BMC_ATTR_ID_PSU0_VIN] = {"PSU0_VIN", 0},
    [BMC_ATTR_ID_PSU0_VOUT] = {"PSU0_VOUT", 0},
    [BMC_ATTR_ID_PSU0_IIN] = {"PSU0_IIN",0},
    [BMC_ATTR_ID_PSU0_IOUT] = {"PSU0_IOUT",0},
    [BMC_ATTR_ID_PSU0_STBVOUT] = {"PSU0_STBVOUT", 0},
    [BMC_ATTR_ID_PSU0_STBIOUT] = {"PSU0_STBIOUT", 0},
    [BMC_ATTR_ID_PSU1_VIN] = {"PSU1_VIN", 0},
    [BMC_ATTR_ID_PSU1_VOUT] = {"PSU1_VOUT", 0},
    [BMC_ATTR_ID_PSU1_IIN] = {"PSU1_IIN", 0},
    [BMC_ATTR_ID_PSU1_IOUT] = {"PSU1_IOUT", 0},
    [BMC_ATTR_ID_PSU1_STBVOUT] = {"PSU1_STBVOUT", 0},
    [BMC_ATTR_ID_PSU1_STBIOUT] = {"PSU1_STBIOUT", 0}
};

bmc_info_t pvt_bmc_cache[] =
{
    [BMC_ATTR_ID_TEMP_ENV_CPU] = {PVT_TEMP_ENV_CPU, 0},
    [BMC_ATTR_ID_TEMP_CPU_PECI] = {"TEMP_CPU_PECI", 0},
    [BMC_ATTR_ID_TEMP_ENV_MAC0] = {PVT_TEMP_ENV_MAC0, 0},
    [BMC_ATTR_ID_TEMP_MAC0] = {PVT_TEMP_MAC0, 0},
    [BMC_ATTR_ID_TEMP_ENV_MAC1] = {PVT_TEMP_ENV_MAC1, 0},
    [BMC_ATTR_ID_TEMP_MAC1] = {PVT_TEMP_MAC1, 0},
    [BMC_ATTR_ID_PSU0_TEMP] = {PVT_PSU0_TEMP, 0},
    [BMC_ATTR_ID_PSU1_TEMP] = {PVT_PSU1_TEMP, 0},
    [BMC_ATTR_ID_FAN0_RPM] = {"FAN0_RPM", 0},
    [BMC_ATTR_ID_FAN1_RPM] = {"FAN1_RPM", 0},
    [BMC_ATTR_ID_FAN2_RPM] = {"FAN2_RPM", 0},
    [BMC_ATTR_ID_FAN3_RPM] = {"FAN3_RPM", 0},
    [BMC_ATTR_ID_PSU0_FAN] = {"PSU0_FAN", 0},
    [BMC_ATTR_ID_PSU1_FAN] = {"PSU1_FAN", 0},
    [BMC_ATTR_ID_FAN0_PRSNT_H] = {"FAN0_PRSNT_H",0},
    [BMC_ATTR_ID_FAN1_PRSNT_H] = {"FAN1_PRSNT_H",0},
    [BMC_ATTR_ID_FAN2_PRSNT_H] = {"FAN2_PRSNT_H", 0},
    [BMC_ATTR_ID_FAN3_PRSNT_H] = {"FAN3_PRSNT_H", 0},
    [BMC_ATTR_ID_PSU0_VIN] = {"PSU0_VIN", 0},
    [BMC_ATTR_ID_PSU0_VOUT] = {"PSU0_VOUT", 0},
    [BMC_ATTR_ID_PSU0_IIN] = {"PSU0_IIN",0},
    [BMC_ATTR_ID_PSU0_IOUT] = {"PSU0_IOUT",0},
    [BMC_ATTR_ID_PSU0_STBVOUT] = {"PSU0_STBVOUT", 0},
    [BMC_ATTR_ID_PSU0_STBIOUT] = {"PSU0_STBIOUT", 0},
    [BMC_ATTR_ID_PSU1_VIN] = {"PSU1_VIN", 0},
    [BMC_ATTR_ID_PSU1_VOUT] = {"PSU1_VOUT", 0},
    [BMC_ATTR_ID_PSU1_IIN] = {"PSU1_IIN", 0},
    [BMC_ATTR_ID_PSU1_IOUT] = {"PSU1_IOUT", 0},
    [BMC_ATTR_ID_PSU1_STBVOUT] = {"PSU1_STBVOUT", 0},
    [BMC_ATTR_ID_PSU1_STBIOUT] = {"PSU1_STBIOUT", 0}
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
    if(file_read_hex(&board->hw_id, SYSFS_HW_ID) != ONLP_STATUS_OK ||
       file_read_hex(&board->deph_id, SYSFS_DEPH_ID) != ONLP_STATUS_OK ||
       file_read_hex(&board->build_id, SYSFS_BUILD_ID) != ONLP_STATUS_OK)
    {
        board->hw_id = 0;
        board->deph_id = 0;
        board->build_id = 0;
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

/**
 * @brief Check if input string is a valid float
 * @param str: input string
 * @rv=1 1 if str is valid float string
 * @rv=0 if str is not valid float string
 * @returns rv.
 */
int is_float_string(char * str)
{
    int rv = 0;
    int len;
    float ignore;
    int ret = 0;

    if(str ==NULL)
        return 0;

    ret = sscanf(str, "%f %n", &ignore, &len);

    //rv=1 if sscanf return 1 parsed float parameter and the whole inut string characters are parsed as float
    rv = (ret==1 && !str[len]);

    return rv;
}

/* bmc sesnor read with cache mechanism */
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
    long file_last_time = 0;
    static int init_cache = 1;
    float f_rv = 0;
    char* presence_str = "Present";
    int retry = 0, retry_max = 2;
    char line[BMC_FRU_LINE_SIZE] = {'\0'};
    char *line_ptr = NULL;
    char line_fields[20][BMC_FRU_ATTR_KEY_VALUE_SIZE];
    char seps[] = ",";
    char *token;
    char *cmd_bmc_sensor_cache = NULL;
    int i = 0;
    int deph_id = 0;
    int hw_id = 0;
    bmc_info_t *bmc_cache = NULL;

    ONLP_TRY(file_read_hex(&deph_id, SYSFS_DEPH_ID));
    ONLP_TRY(file_read_hex(&hw_id, SYSFS_HW_ID));

    //sensor name is different between AST2600 and AST2400
    if (deph_id == 0 && hw_id <= 2) {
        cmd_bmc_sensor_cache = BETA_CMD_BMC_SENSOR_CACHE;
        bmc_cache = beta_bmc_cache;
    } else {
        cmd_bmc_sensor_cache = PVT_CMD_BMC_SENSOR_CACHE;
        bmc_cache = pvt_bmc_cache;
    }

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
                goto done;
            }
            snprintf(ipmi_cmd, sizeof(ipmi_cmd), cmd_bmc_sensor_cache, IPMITOOL_CMD_TIMEOUT);
            for (retry = 0; retry < retry_max; ++retry) {
                if ((rv=system(ipmi_cmd)) != 0) {
                    if (retry == retry_max-1) {
                        AIM_LOG_ERROR("%s() write bmc sensor cache failed, retry=%d, cmd=%s, ret=%d",
                            __func__, retry, ipmi_cmd, rv);
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
        fp = fopen (BMC_SENSOR_CACHE, "r");
        if(fp == NULL) {
            AIM_LOG_ERROR("%s() open file failed, file=%s",
                            __func__, BMC_SENSOR_CACHE);
            rv = ONLP_STATUS_E_INTERNAL;
            goto done;
        }

        //read file line by line
        while(fgets(line,BMC_FRU_LINE_SIZE,fp) != NULL) {
            i=0;
            line_ptr = line;
            token = NULL;

            //parse line into fields
            while ((token = strsep (&line_ptr, seps)) != NULL) {
                sscanf (token, "%[^\n]", line_fields[i++]);
            }

            //save bmc_cache from fields
            for(i=0; i<BMC_ATTR_ID_MAX; ++i) {
                if(strcmp(line_fields[0], bmc_cache[i].name) == 0) {
                    dev_num = i;
                    if( dev_num >= BMC_ATTR_ID_FAN0_PRSNT_H && dev_num <= BMC_ATTR_ID_FAN3_PRSNT_H ) {
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

done:
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
            char fields[256]="";
            snprintf(fields, sizeof(fields), "-e '%s' -e '%s' -e '%s' -e '%s'",
                        BMC_FRU_KEY_MANUFACTURER, BMC_FRU_KEY_NAME ,BMC_FRU_KEY_PART_NUMBER, BMC_FRU_KEY_SERIAL);

            snprintf(ipmi_cmd, sizeof(ipmi_cmd), CMD_FRU_CACHE_SET, IPMITOOL_CMD_TIMEOUT, fru->bmc_fru_id, fields, fru->cache_files);
            int retry = 0, retry_max = 2;
            for (retry = 0; retry < retry_max; ++retry) {
                int rv = 0;
                if ((rv=system(ipmi_cmd)) != ONLP_STATUS_OK) {
                    if (retry == retry_max-1) {
                        AIM_LOG_ERROR("%s() write bmc fru cache failed, retry=%d, cmd=%s, ret=%d",
                            __func__, retry, ipmi_cmd, rv);
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
    if (fp == NULL) {
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
    int rv = 0;
    va_list vargs;
    va_start(vargs, fmt);
    rv = file_vread_hex(value, fmt, vargs);
    va_end(vargs);
    return rv;
}

int file_vread_hex(int* value, const char* fmt, va_list vargs)
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
    char cmd_buf[256] = {0};
    int ret = 0;

    if(access(SYSFS_CPLD1_ID, F_OK) != -1 ) {

        snprintf(cmd_buf, sizeof(cmd_buf), "cat %s > /dev/null 2>&1", SYSFS_CPLD1_ID);
        ret = system(cmd_buf);

        if (ret != 0) {
            if(access(SYSFS_MUX_RESET, F_OK) != -1 ) {
                snprintf(cmd_buf, sizeof(cmd_buf), "echo 0 > %s 2> /dev/null", SYSFS_MUX_RESET);
                ret = system(cmd_buf);
            }
        }
    }
}

/* reg shift */
uint8_t ufi_shift(uint8_t mask)
{
    int i = 0, mask_one = 1;

    for(i=0; i<8; ++i) {
        if ((mask & mask_one) == 1)
            return i;
        else
            mask >>= 1;
    }

    return -1;
}

/* reg mask and shift */
uint8_t ufi_mask_shift(uint8_t val, uint8_t mask)
{
    int shift = 0;

    shift = ufi_shift(mask);

    return (val & mask) >> shift;
}

uint8_t ufi_bit_operation(uint8_t reg_val, uint8_t bit, uint8_t bit_val)
{
    if (bit_val == 0)
        reg_val = reg_val & ~(1 << bit);
    else
        reg_val = reg_val | (1 << bit);
    return reg_val;
}

/**
 * @brief read cpld register
 * @param cpld_id The CPLD id (CPLD_1 - CPLD_3)
 * @param reg The CPLD register to read (0x00-0xff)
 * @param[out] reg_val The value of register (0x00-0xff)
 */
int ufi_read_cpld_reg(int cpld_id, uint8_t reg, uint8_t *reg_val)
{
    char cmd[1024] = {0};
    char cmd_output[8] = {0};

    if (cpld_id < CPLD_1 || cpld_id >= CPLD_MAX) {
        AIM_LOG_ERROR("Invalid cpld_id, it should be %d - %d.", CPLD_1, CPLD_3);
        return ONLP_STATUS_E_PARAM;
    }

    if (reg < 0x0 || reg > 0xff) {
        AIM_LOG_ERROR("Invalid reg addr, it should be 0x0 - 0xFF.");
        return ONLP_STATUS_E_PARAM;
    }

    if(reg_val == NULL) {
        AIM_LOG_ERROR("reg_val is null");
        return ONLP_STATUS_E_PARAM;
    }

    //init cmd
    snprintf(cmd, sizeof(cmd), CMD_I2C_GET, CPLD_I2C_BUS, CPLD_BASE_ADDR[cpld_id], reg);

    //read cpld reg
    if (exec_cmd(cmd, cmd_output, sizeof(cmd_output)) < 0) {
            AIM_LOG_ERROR("read cpld reg failed, cmd=%s\n", cmd);
            return ONLP_STATUS_E_INTERNAL;
    }

    *reg_val = (uint8_t) strtol(cmd_output, NULL, 16);

    return ONLP_STATUS_OK;
}

/**
 * @brief write cpld register
 * @param cpld_id The CPLD id (CPLD_1 - CPLD_3)
 * @param reg The CPLD register to write (0x00-0xff)
 * @param reg_val The value to write (0x00-0xff)
 */
int ufi_write_cpld_reg(int cpld_id, uint8_t reg, uint8_t reg_val)
{
    char cmd[1024] = {0};
    char cmd_output[8] = {0};

    if (cpld_id < CPLD_1 || cpld_id >= CPLD_MAX) {
        AIM_LOG_ERROR("Invalid cpld_id, it should be %d - %d.", CPLD_1, CPLD_3);
        return ONLP_STATUS_E_PARAM;
    }

    if (reg < 0x0 || reg > 0xff) {
        AIM_LOG_ERROR("Invalid reg addr, it should be 0x0 - 0xFF.");
        return ONLP_STATUS_E_PARAM;
    }

    if(reg_val < 0x0 || reg_val > 0xff) {
        AIM_LOG_ERROR("Invalid reg value, it should be 0x0 - 0xFF.");
        return ONLP_STATUS_E_PARAM;
    }

    //init cmd
    snprintf(cmd, sizeof(cmd), CMD_I2C_SET, CPLD_I2C_BUS, CPLD_BASE_ADDR[cpld_id], reg, reg_val);

    //write cpld reg
    if (exec_cmd(cmd, cmd_output, sizeof(cmd_output)) < 0) {
            AIM_LOG_ERROR("write cpld reg failed, cmd=%s\n", cmd);
            return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int ufi_get_cpu_rev(int *rev_sku, int *rev_hw, int *rev_build)
{
    if (rev_sku == NULL || rev_hw == NULL || rev_build == NULL) {
        AIM_LOG_ERROR("rev_sku, rev_hw or rev_build is NULL pointer");
        return ONLP_STATUS_E_PARAM;
    }

    ONLP_TRY(file_read_hex(rev_sku, "/sys/devices/platform/x86_64_ufispace_s9600_30dx_lpc/cpu_cpld/cpu_rev_sku"));
    ONLP_TRY(file_read_hex(rev_hw, "/sys/devices/platform/x86_64_ufispace_s9600_30dx_lpc/cpu_cpld/cpu_rev_hw"));
    ONLP_TRY(file_read_hex(rev_build, "/sys/devices/platform/x86_64_ufispace_s9600_30dx_lpc/cpu_cpld/cpu_rev_build"));

    return ONLP_STATUS_OK;
}

/**
 * @brief read mac hbm power status
 * @param[out] pwr_ctrl The value of power status (0: power off, 1: power on)
 */
int ufi_read_hbm_pwr_ctrl(int *pwr_ctrl)
{
    if (pwr_ctrl == NULL) {
        AIM_LOG_ERROR("pwr_ctrl is NULL pointer");
        return ONLP_STATUS_E_PARAM;
    }

    ONLP_TRY(file_read_hex(pwr_ctrl, SYSFS_HBM_PWR_CTRL));

    return ONLP_STATUS_OK;
}

/**
 * @brief write mac hbm power status
 * @param pwr_ctrl The value of power status (0: power off, 1: power on)
 */
int ufi_write_hbm_pwr_ctrl(int pwr_ctrl)
{
    if (pwr_ctrl < 0 || pwr_ctrl > 1) {
        AIM_LOG_ERROR("Invalid pwr_ctrl value %d, it should be 0 or 1.", pwr_ctrl);
        return ONLP_STATUS_E_PARAM;
    }

    ONLP_TRY(onlp_file_write_int(pwr_ctrl, SYSFS_HBM_PWR_CTRL));

    return ONLP_STATUS_OK;
}
