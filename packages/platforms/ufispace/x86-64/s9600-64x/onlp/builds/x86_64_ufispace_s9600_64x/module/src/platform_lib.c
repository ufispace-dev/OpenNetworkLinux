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
const int CPLD_I2C_BUS[] = {1, 1, 1, 1, 1};

const char * thermal_id_str[] = {
    "",
    "ADC_CPU_TEMP",
    "TEMP_CPU_PECI",
    "TEMP_FRONT_ENV",
    "TEMP_OCXO",
    "TEMP_Q2CL_ENV",
    "TEMP_Q2CL_DIE",
    "TEMP_Q2CR_ENV",
    "TEMP_Q2CR_DIE",
    "TEMP_REAR_ENV_1",
    "TEMP_REAR_ENV_2",
    "CPU_PACKAGE",
    "PSU0_TEMP",
    "PSU1_TEMP",
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
    [BMC_ATTR_ID_ADC_CPU_TEMP]    = {"ADC_CPU_TEMP", 0},
    [BMC_ATTR_ID_TEMP_CPU_PECI]   = {"TEMP_CPU_PECI", 0},
    [BMC_ATTR_ID_TEMP_FRONT_ENV]  = {"TEMP_FRONT_ENV", 0},
    [BMC_ATTR_ID_TEMP_OCXO]       = {"TEMP_OCXO", 0},
    [BMC_ATTR_ID_TEMP_Q2CL_ENV]   = {"TEMP_Q2CL_ENV", 0},
    [BMC_ATTR_ID_TEMP_Q2CL_DIE]   = {"TEMP_Q2CL_DIE", 0},
    [BMC_ATTR_ID_TEMP_Q2CR_ENV]   = {"TEMP_Q2CR_ENV", 0},
    [BMC_ATTR_ID_TEMP_Q2CR_DIE]   = {"TEMP_Q2CR_DIE", 0},
    [BMC_ATTR_ID_TEMP_REAR_ENV_1] = {"TEMP_REAR_ENV_1", 0},
    [BMC_ATTR_ID_TEMP_REAR_ENV_2] = {"TEMP_REAR_ENV_2", 0},
    [BMC_ATTR_ID_PSU0_TEMP]       = {"PSU0_TEMP", 0},
    [BMC_ATTR_ID_PSU1_TEMP]       = {"PSU1_TEMP", 0},
    [BMC_ATTR_ID_FAN0_RPM]        = {"FAN0_RPM", 0},
    [BMC_ATTR_ID_FAN1_RPM]        = {"FAN1_RPM", 0},
    [BMC_ATTR_ID_FAN2_RPM]        = {"FAN2_RPM", 0},
    [BMC_ATTR_ID_FAN3_RPM]        = {"FAN3_RPM", 0},
    [BMC_ATTR_ID_PSU0_FAN]        = {"PSU0_FAN", 0},
    [BMC_ATTR_ID_PSU1_FAN]        = {"PSU1_FAN", 0},
    [BMC_ATTR_ID_FAN0_PRSNT_H]    = {"FAN0_PRSNT_H",0},
    [BMC_ATTR_ID_FAN1_PRSNT_H]    = {"FAN1_PRSNT_H",0},
    [BMC_ATTR_ID_FAN2_PRSNT_H]    = {"FAN2_PRSNT_H", 0},
    [BMC_ATTR_ID_FAN3_PRSNT_H]    = {"FAN3_PRSNT_H", 0},
    [BMC_ATTR_ID_PSU0_VIN]        = {"PSU0_VIN", 0},
    [BMC_ATTR_ID_PSU0_VOUT]       = {"PSU0_VOUT", 0},
    [BMC_ATTR_ID_PSU0_IIN]        = {"PSU0_IIN",0},
    [BMC_ATTR_ID_PSU0_IOUT]       = {"PSU0_IOUT",0},
    [BMC_ATTR_ID_PSU0_STBVOUT]    = {"PSU0_STBVOUT", 0},
    [BMC_ATTR_ID_PSU0_STBIOUT]    = {"PSU0_STBIOUT", 0},
    [BMC_ATTR_ID_PSU1_VIN]        = {"PSU1_VIN", 0},
    [BMC_ATTR_ID_PSU1_VOUT]       = {"PSU1_VOUT", 0},
    [BMC_ATTR_ID_PSU1_IIN]        = {"PSU1_IIN", 0},
    [BMC_ATTR_ID_PSU1_IOUT]       = {"PSU1_IOUT", 0},
    [BMC_ATTR_ID_PSU1_STBVOUT]    = {"PSU1_STBVOUT", 0},
    [BMC_ATTR_ID_PSU1_STBIOUT]    = {"PSU1_STBIOUT", 0}
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

int mask_shift(int val, int mask)
{
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

    ONLP_LOCK();

    if(check_file_exist(BMC_SENSOR_CACHE, &file_last_time))
    {
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
        char line[BMC_FRU_LINE_SIZE] = {'\0'};
        while(fgets(line,BMC_FRU_LINE_SIZE, fp) != NULL) {
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
                    if(i >= BMC_ATTR_ID_FAN0_PRSNT_H && i <= BMC_ATTR_ID_FAN3_PRSNT_H) {
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
            char ipmi_cmd[1024] = {0};
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

int psu_thermal_get(onlp_thermal_info_t* info, int thermal_id)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int psu_present_get(int *pw_present, int id)
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

int psu_pwgood_get(int *pw_good, int id)
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

int qsfp_present_get(int port, int *pres_val)
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

int exec_cmd(char *cmd, char* out, int size) {
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

int get_ipmitool_len(char *ipmitool_out){
    size_t str_len=0, ipmitool_len=0;
    str_len = strlen(ipmitool_out);
    if (str_len>0) {
        ipmitool_len = str_len/3;
    }
    return ipmitool_len;
}

int sysi_platform_info_get(onlp_platform_info_t* pi)
{
    int i;
    char bios_out[32];
    char bmc_out1[8], bmc_out2[8], bmc_out3[8];
    uint8_t cpu_cpld_ver_h[32];
    uint8_t mb_cpld_ver_h[CPLD_MAX][16];
    int data_len = 0;
    int mb_cpld1_addr = CPLD_REG_BASE + BRD_ID_REG;
    int mb_cpld1_board_type_rev = 0, mb_cpld1_hw_rev = 0, mb_cpld1_build_rev = 0;

    char mu_ver[128], mu_result[128];
    char path_onie_folder[] = "/mnt/onie-boot/onie";
    char path_onie_update_log[] = "/mnt/onie-boot/onie/update/update_details.log";
    char cmd_mount_mu_dir[] = "mkdir -p /mnt/onie-boot && mount LABEL=ONIE-BOOT /mnt/onie-boot/ 2> /dev/null";
    char cmd_mu_ver[] = "cat /mnt/onie-boot/onie/update/update_details.log | grep -i 'Updater version:' | tail -1 | awk -F ' ' '{ print $3}' | tr -d '\\r\\n'";
    char cmd_mu_result_template[] = "/mnt/onie-boot/onie/tools/bin/onie-fwpkg | grep '%s' | awk -F '|' '{ print $3 }' | tail -1 | xargs | tr -d '\\r\\n'";
    char cmd_mu_result[256];


    memset(bios_out, 0, sizeof(bios_out));
    memset(bmc_out1, 0, sizeof(bmc_out1));
    memset(bmc_out2, 0, sizeof(bmc_out2));
    memset(bmc_out3, 0, sizeof(bmc_out3));
    memset(cpu_cpld_ver_h, 0, sizeof(cpu_cpld_ver_h));
    memset(mb_cpld_ver_h, 0, sizeof(mb_cpld_ver_h));
    memset(mu_ver, 0, sizeof(mu_ver));
    memset(mu_result, 0, sizeof(mu_result));
    memset(cmd_mu_result, 0, sizeof(cmd_mu_result));

    //get CPU CPLD version readable string
    ONLP_TRY(onlp_file_read(cpu_cpld_ver_h, sizeof(cpu_cpld_ver_h), &data_len,
                    LPC_CPU_CPLD_PATH "/" LPC_CPU_CPLD_VER_ATTR));
    //trim new line
    cpu_cpld_ver_h[strcspn((char *)cpu_cpld_ver_h, "\n" )] = '\0';

    //get MB CPLD version readable string
    for(i=0; i<CPLD_MAX; ++i) {
        ONLP_TRY(onlp_file_read(mb_cpld_ver_h[i], sizeof(mb_cpld_ver_h[i]), &data_len,
        SYS_FMT, CPLD_I2C_BUS[i], CPLD_BASE_ADDR[i], "/cpld_version_h"));
        //trim new line
        mb_cpld_ver_h[i][strcspn((char *)cpu_cpld_ver_h, "\n" )] = '\0';
    }

    pi->cpld_versions = aim_fstrdup(
        "\n"
        "[CPU CPLD] %s\n"
        "[MB CPLD1] %s\n"
        "[MB CPLD2] %s\n"
        "[MB CPLD3] %s\n"
        "[MB CPLD4] %s\n"
        "[MB CPLD5] %s\n",
        cpu_cpld_ver_h,
        mb_cpld_ver_h[0],
        mb_cpld_ver_h[1],
        mb_cpld_ver_h[2],
        mb_cpld_ver_h[3],
        mb_cpld_ver_h[4]);

    //Get HW Build Version
    ONLP_TRY(read_ioport(mb_cpld1_addr, &mb_cpld1_board_type_rev));

    mb_cpld1_hw_rev = ((mb_cpld1_board_type_rev) & 0x03);
    //FIXME: check build_rev bits
    //mb_cpld1_build_rev = (((mb_cpld1_board_type_rev) & 0x03) | ((mb_cpld1_board_type_rev) >> 5 & 0x04));
    mb_cpld1_build_rev = ((mb_cpld1_board_type_rev) >> 3 & 0x07);

    //Get BIOS version
    ONLP_TRY(exec_cmd(CMD_BIOS_VER, bios_out, sizeof(bios_out)));

    // Detect bmc status
    if(bmc_check_alive() != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Timeout, BMC did not respond.\n");
        return ONLP_STATUS_E_INTERNAL;
    }
    //Get BMC version
    if (exec_cmd(CMD_BMC_VER_1, bmc_out1, sizeof(bmc_out1)) < 0 ||
        exec_cmd(CMD_BMC_VER_2, bmc_out2, sizeof(bmc_out2)) < 0 ||
        exec_cmd(CMD_BMC_VER_3, bmc_out3, sizeof(bmc_out3))) {
            AIM_LOG_ERROR("unable to read BMC version\n");
            return ONLP_STATUS_E_INTERNAL;
    }

    //Mount MU Folder
    if(access(path_onie_folder, F_OK) == -1 )
        system(cmd_mount_mu_dir);

    //Get MU Version
    if(access(path_onie_update_log, F_OK) != -1 ) {
        exec_cmd(cmd_mu_ver, mu_ver, sizeof(mu_ver));

        if (strnlen(mu_ver, sizeof(mu_ver)) != 0) {
            snprintf(cmd_mu_result, sizeof(cmd_mu_result), cmd_mu_result_template, mu_ver);
            exec_cmd(cmd_mu_result, mu_result, sizeof(mu_result));
        }
    }

    pi->other_versions = aim_fstrdup(
        "\n"
        "[HW   ] %d\n"
        "[BUILD] %d\n"
        "[BIOS ] %s\n"
        "[BMC  ] %d.%d.%d\n"
        "[MU   ] %s (%s)\n",
        mb_cpld1_hw_rev,
        mb_cpld1_build_rev,
        bios_out,
        atoi(bmc_out1), atoi(bmc_out2), atoi(bmc_out3),
        strnlen(mu_ver, sizeof(mu_ver)) != 0 ? mu_ver : "NA", mu_result);

    return ONLP_STATUS_OK;
}

bool onlp_sysi_bmc_en_get(void)
{
   return true;
}

int qsfp_port_to_cpld_addr(int port)
{
    int cpld_addr = 0;
    int cpld_num = ((port%32) / 8) + 1;

    cpld_addr = CPLD_BASE_ADDR[cpld_num];

    return cpld_addr;
}

int qsfp_port_to_sysfs_attr_offset(int port)
{
    int sysfs_attr_offset = 0;

    sysfs_attr_offset = port / 32;

    return sysfs_attr_offset;
}

int parse_bmc_sdr_cmd(char *cmd_out, int cmd_out_size,
                  char *tokens[], int token_size,
                  const char *sensor_id_str, int *idx)
{
    char cmd[BMC_CMD_SDR_SIZE];
    char delimiter[]=",";
    char delimiter_c = ',';

    *idx=0;
    memset(cmd, 0, sizeof(cmd));
    memset(cmd_out, 0, cmd_out_size);

    snprintf(cmd, sizeof(cmd), CMD_BMC_SDR_GET, IPMITOOL_CMD_TIMEOUT, sensor_id_str);

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

int bmc_thermal_info_get(onlp_thermal_info_t* info, int id)
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

int bmc_fan_info_get(onlp_fan_info_t* info, int id)
{
    int rv=0, rpm=0, percentage=0;
    int presence=0;
    int psu_type = ONLP_PSU_TYPE_AC;
    float data = 0;
    int max_fan_speed = 0;

    //check presence for fantray 1-4
    if (id >= FAN_ID_FAN0 && id <= FAN_ID_FAN3) {
        rv = bmc_sensor_read(id - FAN_ID_FAN0 + 18, FAN_SENSOR, &data);
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

    rv = bmc_sensor_read(id - FAN_ID_FAN0 + 12, FAN_SENSOR, &data);
    if ( rv != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%d\n", id);
        return rv;
    }
    rpm = (int) data;

    //set rpm field
    info->rpm = rpm;

    if (id >= FAN_ID_FAN0 && id <= FAN_ID_FAN3) {
        percentage = (info->rpm*100)/SYS_FAN_RPM_MAX;
        info->percentage = percentage;
        info->status |= (rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;
    } else if (id >= FAN_ID_PSU0_FAN && id <= FAN_ID_PSU1_FAN) {
        //get psu type
        if (id == FAN_ID_PSU0_FAN) {
            ONLP_TRY(get_psu_type(ONLP_PSU_0, &psu_type, NULL));
        } else if (id == FAN_ID_PSU1_FAN) {
            ONLP_TRY(get_psu_type(ONLP_PSU_1, &psu_type, NULL));
        } else {
            AIM_LOG_ERROR("unknown fan id (%d), func=%s", id, __FUNCTION__);
            return ONLP_STATUS_E_PARAM;
        }

        //AC/DC has different MAX RPM
        if (psu_type==ONLP_PSU_TYPE_AC) {
            max_fan_speed = PSU_FAN_RPM_MAX_AC;
        } else if (psu_type==ONLP_PSU_TYPE_DC12 || psu_type==ONLP_PSU_TYPE_DC48) {
            max_fan_speed = PSU_FAN_RPM_MAX_DC;
        } else {
            AIM_LOG_ERROR("unknown psu_type, id=%d, psu_type=%d func=%s", id-FAN_ID_PSU0_FAN+1, psu_type, __FUNCTION__);
            return ONLP_STATUS_E_INTERNAL;
        }

        percentage = (info->rpm*100)/max_fan_speed;
        info->percentage = percentage;
        info->status |= (rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;
    }

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

int sys_led_info_get(onlp_led_info_t* info, int id)
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
        // update status
        info->status &= ~ONLP_LED_STATUS_ON;
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
        // update status
        info->status |= ONLP_LED_STATUS_ON;
    }

    return ONLP_STATUS_OK;
}