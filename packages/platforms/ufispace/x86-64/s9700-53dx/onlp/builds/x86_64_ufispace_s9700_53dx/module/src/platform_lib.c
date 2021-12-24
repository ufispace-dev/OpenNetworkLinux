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


#define FAN_CACHE_TIME          5
#define PSU_CACHE_TIME          5
#define THERMAL_CACHE_TIME      10
#define BMC_CMD_SDR_SIZE        48
#define BMC_SENSOR_CACHE        "/tmp/bmc_sensor_cache"
#define CMD_BMC_CACHE_GET       "cat "BMC_SENSOR_CACHE" | grep %s | awk -F',' '{print $%d}'"

/*   IPMITOOL_CMD_TIMEOUT get from ipmitool test.
 *   Test Case: Run 100 times of CMD_BMC_SENSOR_CACHE command and 100 times of CMD_FRU_CACHE_SET command and get the execution times.
 *              We take 10s as The IPMITOOL_CMD_TIMEOUT value 
 *              since the CMD_BMC_SENSOR_CACHE execution times value is between 0.216s - 2.926s and
 *                    the CMD_FRU_CACHE_SET execution times value is between 0.031s - 0.076s.
 */
#define IPMITOOL_CMD_TIMEOUT    10
//#define CMD_BMC_SENSOR_CACHE  "timeout 10s ipmitool sdr -c get TEMP_CPU_PECI TEMP_OP2_ENV TEMP_J2_ENV_1 TEMP_J2_DIE_1 TEMP_J2_ENV_2 TEMP_J2_DIE_2 PSU0_TEMP PSU1_TEMP TEMP_BMC_ENV TEMP_ENV TEMP_ENV_FRONT FAN0_RPM FAN1_RPM FAN2_RPM FAN3_RPM PSU0_FAN1 PSU0_FAN2 PSU1_FAN1 PSU1_FAN2 FAN0_PRSNT_H FAN1_PRSNT_H FAN2_PRSNT_H FAN3_PRSNT_H PSU0_VIN PSU0_VOUT PSU0_IIN PSU0_IOUT PSU0_STBVOUT PSU0_STBIOUT PSU1_VIN PSU1_VOUT PSU1_IIN PSU1_IOUT PSU1_STBVOUT PSU1_STBIOUT > /tmp/bmc_sensor_cache"
#define CMD_BMC_SENSOR_CACHE    "timeout %ds ipmitool sdr -c get "\
                                " TEMP_CPU_PECI"\
                                " TEMP_OP2_ENV"\
                                " TEMP_J2_ENV_1"\
                                " TEMP_J2_DIE_1"\
                                " TEMP_J2_ENV_2"\
                                " TEMP_J2_DIE_2"\
                                " PSU0_TEMP"\
                                " PSU1_TEMP"\
                                " TEMP_BMC_ENV"\
                                " TEMP_ENV"\
                                " TEMP_ENV_FRONT"\
                                " FAN0_RPM"\
                                " FAN1_RPM"\
                                " FAN2_RPM"\
                                " FAN3_RPM"\
                                " PSU0_FAN1"\
                                " PSU0_FAN2"\
                                " PSU1_FAN1"\
                                " PSU1_FAN2"\
                                " FAN0_PRSNT_H"\
                                " FAN1_PRSNT_H"\
                                " FAN2_PRSNT_H"\
                                " FAN3_PRSNT_H"\
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

/* BMC Cached String (mapping with ipmitool sensors) */
bmc_info_t bmc_cache[] =
{
    [BMC_ATTR_ID_TEMP_CPU_PECI]  = {"TEMP_CPU_PECI" , 0}, 
    [BMC_ATTR_ID_TEMP_OP2_ENV]   = {"TEMP_OP2_ENV"  , 0}, 
    [BMC_ATTR_ID_TEMP_J2_ENV_1]  = {"TEMP_J2_ENV_1" , 0}, 
    [BMC_ATTR_ID_TEMP_J2_DIE_1]  = {"TEMP_J2_DIE_1" , 0}, 
    [BMC_ATTR_ID_TEMP_J2_ENV_2]  = {"TEMP_J2_ENV_2" , 0}, 
    [BMC_ATTR_ID_TEMP_J2_DIE_2]  = {"TEMP_J2_DIE_2" , 0}, 
    [BMC_ATTR_ID_PSU0_TEMP]      = {"PSU0_TEMP"     , 0}, 
    [BMC_ATTR_ID_PSU1_TEMP]      = {"PSU1_TEMP"     , 0}, 
    [BMC_ATTR_ID_TEMP_BMC_ENV]   = {"TEMP_BMC_ENV"  , 0}, 
    [BMC_ATTR_ID_TEMP_ENV]       = {"TEMP_ENV"      , 0}, 
    [BMC_ATTR_ID_TEMP_ENV_FRONT] = {"TEMP_ENV_FRONT", 0},
    [BMC_ATTR_ID_FAN0_RPM]       = {"FAN0_RPM"      , 0},
    [BMC_ATTR_ID_FAN1_RPM]       = {"FAN1_RPM"      , 0},
    [BMC_ATTR_ID_FAN2_RPM]       = {"FAN2_RPM"      , 0},
    [BMC_ATTR_ID_FAN3_RPM]       = {"FAN3_RPM"      , 0},
    [BMC_ATTR_ID_PSU0_FAN1]      = {"PSU0_FAN1"     , 0},
    [BMC_ATTR_ID_PSU0_FAN2]      = {"PSU0_FAN2"     , 0},
    [BMC_ATTR_ID_PSU1_FAN1]      = {"PSU1_FAN1"     , 0},
    [BMC_ATTR_ID_PSU1_FAN2]      = {"PSU1_FAN2"     , 0},
    [BMC_ATTR_ID_FAN0_PRSNT_H]   = {"FAN0_PRSNT_H"  , 0},
    [BMC_ATTR_ID_FAN1_PRSNT_H]   = {"FAN1_PRSNT_H"  , 0},
    [BMC_ATTR_ID_FAN2_PRSNT_H]   = {"FAN2_PRSNT_H"  , 0},
    [BMC_ATTR_ID_FAN3_PRSNT_H]   = {"FAN3_PRSNT_H"  , 0},
    [BMC_ATTR_ID_PSU0_VIN]       = {"PSU0_VIN"      , 0},
    [BMC_ATTR_ID_PSU0_VOUT]      = {"PSU0_VOUT"     , 0},
    [BMC_ATTR_ID_PSU0_IIN]       = {"PSU0_IIN"      , 0},
    [BMC_ATTR_ID_PSU0_IOUT]      = {"PSU0_IOUT"     , 0},
    [BMC_ATTR_ID_PSU0_STBVOUT]   = {"PSU0_STBVOUT"  , 0},
    [BMC_ATTR_ID_PSU0_STBIOUT]   = {"PSU0_STBIOUT"  , 0},
    [BMC_ATTR_ID_PSU1_VIN]       = {"PSU1_VIN"      , 0},
    [BMC_ATTR_ID_PSU1_VOUT]      = {"PSU1_VOUT"     , 0},
    [BMC_ATTR_ID_PSU1_IIN]       = {"PSU1_IIN"      , 0},
    [BMC_ATTR_ID_PSU1_IOUT]      = {"PSU1_IOUT"     , 0},
    [BMC_ATTR_ID_PSU1_STBVOUT]   = {"PSU1_STBVOUT"  , 0},
    [BMC_ATTR_ID_PSU1_STBIOUT]   = {"PSU1_STBIOUT"  , 0}
};


//#define BMC_FRU_ATTR_KEY_VALUE_SIZE  256
#define BMC_FRU_ATTR_KEY_VALUE_LEN  (BMC_FRU_ATTR_KEY_VALUE_SIZE - 1)
#define BMC_FRU_KEY_MANUFACTURER    "Product Manufacturer"
#define BMC_FRU_KEY_NAME            "Product Name"
#define BMC_FRU_KEY_PART_NUMBER     "Product Part Number"
#define BMC_FRU_KEY_SERIAL          "Product Serial"

#define CMD_FRU_CACHE_SET "timeout %ds ipmitool fru print %d " \
                           IPMITOOL_REDIRECT_ERR \
                          " | grep %s" \
                          " | awk -F: '/:/{gsub(/^ /,\"\", $0);gsub(/ +:/,\":\",$0);gsub(/: +/,\":\", $0);print $0}'" \
                          " > %s"

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

const int CPLD_BASE_ADDR[] = {0x30, 0x39, 0x3a, 0x3b, 0x3c};

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

/* bmc sesnor read with cache mechanism */
int bmc_sensor_read(int bmc_cache_index, int sensor_type, float *data)
{
    int cache_time = 0;
    int bmc_cache_expired = 0;
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

    ONLP_LOCK();

    if(check_file_exist(BMC_SENSOR_CACHE, &file_last_time)) {
        struct timeval new_tv = {0};
        gettimeofday(&new_tv, NULL);
        if(bmc_cache_expired_check(file_last_time, new_tv.tv_sec, cache_time)) {
            bmc_cache_expired = 1;
        } else {
            bmc_cache_expired = 0;
        }
    } else {
        bmc_cache_expired = 1;
    }

    //update cache
    if(bmc_cache_expired == 1 || init_cache == 1) {
        if(bmc_cache_expired == 1) {
            // detect bmc status
            if(bmc_check_alive() != ONLP_STATUS_OK) {
                rv = ONLP_STATUS_E_INTERNAL;
                goto done;
            }

            // get bmc data
            char ipmi_cmd[1024] = {0};
            snprintf(ipmi_cmd, sizeof(ipmi_cmd), CMD_BMC_SENSOR_CACHE, IPMITOOL_CMD_TIMEOUT);
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
        char line[BMC_FRU_ATTR_KEY_VALUE_SIZE] = {'\0'};
        while(fgets(line,BMC_FRU_ATTR_KEY_VALUE_SIZE, fp) != NULL) {
            int i = 0;
            char *line_ptr = line;
            char *token = NULL;

            //parse line into fields. fields[0]: fields name, fields[1]: fields value
            char line_fields[20][BMC_FRU_ATTR_KEY_VALUE_SIZE] = {{0}};
            while ((token = strsep (&line_ptr, ",")) != NULL) {
                sscanf (token, "%[^\n]", line_fields[i++]);
            }

            //save bmc_cache from fields
            for(i=0; i<BMC_ATTR_ID_MAX; ++i) {
                if(strcmp(line_fields[0], bmc_cache[i].name) == 0) {
                    if( i >= BMC_ATTR_ID_FAN0_PRSNT_H && i <= BMC_ATTR_ID_FAN3_PRSNT_H ) {
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
 * @param[out] data The psu fru information.
 */
int bmc_fru_read(int local_id, bmc_fru_t *data)
{
    struct timeval new_tv;
    int cache_time = PSU_CACHE_TIME;
    int bmc_cache_expired = 0;
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
            bmc_cache_expired = 0;
        }
    } else {
        bmc_cache_expired = 1;
    }

    //update cache
    if(bmc_cache_expired == 1 || fru->init_done == 0) {
        //get fru from ipmitool and save to cache file
        if(bmc_cache_expired == 1) {
            // detect bmc status
            if(bmc_check_alive() != ONLP_STATUS_OK) {
                rv = ONLP_STATUS_E_INTERNAL;
                goto done;
            }

            // get bmc data
            char ipmi_cmd[400] = {0};
            char fields[256]="";
            snprintf(fields, sizeof(fields), "-e '%s' -e '%s' -e '%s' -e '%s'",
                        BMC_FRU_KEY_MANUFACTURER, BMC_FRU_KEY_NAME ,BMC_FRU_KEY_PART_NUMBER, BMC_FRU_KEY_SERIAL);

            snprintf(ipmi_cmd, sizeof(ipmi_cmd), CMD_FRU_CACHE_SET, IPMITOOL_CMD_TIMEOUT, fru->bmc_fru_id, fields, fru->cache_files);
            int retry = 0, retry_max = 2;
            for (retry = 0; retry < retry_max; ++retry) {
                int ret = 0;
                if ((ret = system(ipmi_cmd)) != 0) {
                    if (retry == retry_max-1) {
                        AIM_LOG_ERROR("%s() write bmc fru cache failed, retry=%d, cmd=%s, ret=%d", __func__, retry, ipmi_cmd, ret);
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

/* read io port */
int read_ioport(int addr, int *reg_val)
{
    int ret;

    /*set r/w permission of all 65536 ports*/
    ret = iopl(3);
    if(ret < 0) {
        AIM_LOG_ERROR("unable to read cpu cpld version, iopl enable error %d\n", ret);
        return ONLP_STATUS_E_INTERNAL;
    }
    *reg_val = inb(addr);

    /*set r/w permission of  all 65536 ports*/
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

int get_ipmitool_len(char *ipmitool_out)
{
    size_t str_len=0, ipmitool_len=0;

    str_len = strlen(ipmitool_out);
    if (str_len>0) {
        ipmitool_len = str_len/3;
    }

    return ipmitool_len;
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

/**
 * @brief Get PSU Present Status
 * @param local_id: psu id
 * @status 1 if presence
 * @status 0 if absence
 * @returns An error condition.
 */
int get_psui_present_status(int local_id, int *status)
{
    int psu_reg_value = 0;
    int psu_presence = 0;

    if (local_id == ONLP_PSU_0) {
        ONLP_TRY(file_read_hex(&psu_reg_value, "/sys/bus/i2c/devices/1-0030/cpld_psu_status_0"));
        psu_presence = (psu_reg_value & 0b01000000) ? 0 : 1;
    } else if (local_id == ONLP_PSU_1) {
        ONLP_TRY(file_read_hex(&psu_reg_value, "/sys/bus/i2c/devices/1-0030/cpld_psu_status_1"));
        psu_presence = (psu_reg_value & 0b10000000) ? 0 : 1;
    } else {
        AIM_LOG_ERROR("unknown psu id (%d), func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    *status = psu_presence;

    return ONLP_STATUS_OK;
}

/*
 * This function check the I2C bus statuas by using the sysfs of cpld_id,
 * If the I2C Bus is stcuk, do the i2c mux reset.
 */
void check_and_do_i2c_mux_reset(int port)
{
    char cmd_buf[256] = {0};
    int ret = 0;

    if(access("/sys/bus/i2c/devices/1-0030/cpld_id", F_OK) != -1 ) {

        snprintf(cmd_buf, sizeof(cmd_buf), "cat /sys/bus/i2c/devices/1-0030/cpld_id > /dev/null 2>&1");
        ret = system(cmd_buf);

        if (ret != 0) {
            if(access("/sys/devices/platform/x86_64_ufispace_s9700_53dx_lpc/cpu_cpld/mux_reset", F_OK) != -1 ) {
                //AIM_LOG_SYSLOG_WARN("I2C bus is stuck!! (port=%d)\r\n", port);
                snprintf(cmd_buf, sizeof(cmd_buf), "echo 0 > /sys/devices/platform/x86_64_ufispace_s9700_53dx_lpc/cpu_cpld/mux_reset 2> /dev/null");
                ret = system(cmd_buf);
                //AIM_LOG_SYSLOG_WARN("Do I2C mux reset!! (ret=%d)\r\n", ret);
            }
        }
    }
}
