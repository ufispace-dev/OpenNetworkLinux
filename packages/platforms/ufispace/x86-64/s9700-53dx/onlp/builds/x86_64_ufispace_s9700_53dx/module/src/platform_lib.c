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
#define CMD_BMC_SDR_GET         "ipmitool sdr -c get %s"
//#define CMD_BMC_SENSOR_CACHE    "ipmitool sdr -c get TEMP_CPU_PECI TEMP_OP2_ENV TEMP_J2_ENV_1 TEMP_J2_DIE_1 TEMP_J2_ENV_2 TEMP_J2_DIE_2 PSU0_TEMP PSU1_TEMP TEMP_BMC_ENV TEMP_ENV TEMP_ENV_FRONT FAN0_RPM FAN1_RPM FAN2_RPM FAN3_RPM PSU0_FAN1 PSU0_FAN2 PSU1_FAN1 PSU1_FAN2 FAN0_PRSNT_H FAN1_PRSNT_H FAN2_PRSNT_H FAN3_PRSNT_H PSU0_VIN PSU0_VOUT PSU0_IIN PSU0_IOUT PSU0_STBVOUT PSU0_STBIOUT PSU1_VIN PSU1_VOUT PSU1_IIN PSU1_IOUT PSU1_STBVOUT PSU1_STBIOUT > /tmp/bmc_sensor_cache"
#define CMD_BMC_SENSOR_CACHE    "ipmitool sdr -c get "\
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
                                " > "BMC_SENSOR_CACHE

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
                if ((rv = system(ipmi_cmd)) != ONLP_STATUS_OK) {
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

            if( dev_num >= BMC_ATTR_ID_FAN0_PRSNT_H && dev_num <= BMC_ATTR_ID_FAN3_PRSNT_H ) {
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

/* read io port */
int read_ioport(int addr, int *reg_val) {
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
int exec_cmd(char *cmd, char* out, int size) {
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
