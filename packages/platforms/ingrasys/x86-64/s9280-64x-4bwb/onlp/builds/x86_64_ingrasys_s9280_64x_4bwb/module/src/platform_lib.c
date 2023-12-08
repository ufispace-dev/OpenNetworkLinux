/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2013 Accton Technology Corporation.
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
#include <unistd.h>
#include <fcntl.h>
#include <onlp/onlp.h>
#include <onlplib/file.h>
#include <onlplib/i2c.h>
#include <onlplib/shlocks.h>
#include <AIM/aim.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "platform_lib.h"

bmc_info_t bmc_cache[] =
{
    {"TEMP_CPU_PECI", 0},
    {"TEMP_BMC_ENV", 0},
    {"TEMP_PCIe_conn", 0},
    {"TEMP_CAGE_R", 0},
    {"TEMP_CAGE_L", 0},
    {"TEMP_MAC_FRONT", 0},
    {"TEMP_MAC_ENV", 0},
    {"TEMP_MAC_DIE", 0},
    {"PSU1_TEMP", 0},
    {"PSU2_TEMP", 0},
    {"FAN1_RPM", 0},
    {"FAN2_RPM", 0},
    {"FAN3_RPM", 0},
    {"FAN4_RPM", 0},
    {"PSU1_FAN1", 0},
    {"PSU2_FAN1", 0},
    {"FAN1_PSNT_L",0},
    {"FAN2_PSNT_L",0},
    {"FAN3_PSNT_L", 0},
    {"FAN4_PSNT_L", 0},
    {"PSU1_PRSNT_L", 0},
    {"PSU1_PWROK_H", 0},
    {"PSU2_PRSNT_L", 0},
    {"PSU2_PWROK_H", 0},
    {"PSU1_VIN", 0},
    {"PSU1_VOUT", 0},
    {"PSU1_IIN",0},
    {"PSU1_IOUT",0},
    {"PSU1_STBVOUT", 0},
    {"PSU1_STBIOUT", 0},
    {"PSU2_VIN", 0},
    {"PSU2_VOUT", 0},
    {"PSU2_IIN", 0},
    {"PSU2_IOUT", 0},
    {"PSU2_STBVOUT", 0},
    {"PSU2_STBIOUT", 0},
    {"LED_SYS", 0},
    {"LED_FAN", 0},
    {"LED_PSU1", 0},
    {"LED_PSU2", 0},
};

bmc_fru_info_t bmc_fru_cache[] =
{
    {"PSU1_MODEL", ""},
    {"PSU1_SERIAL", ""},
    {"PSU2_MODEL", ""},
    {"PSU2_SERIAL", ""},
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
    }
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

int check_file_exist(char *file_path, long *file_time)
{
    struct stat file_info;

    if(stat(file_path, &file_info) == 0) {
        if(file_info.st_size == 0) {
            return 0;
        }
        else {
            *file_time = file_info.st_mtime;
            return 1;
        }
    }
    else {
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
        // avoid sdr get error which get data from raw command
        if(dev_num >= ID_LED_SYS && dev_num <= ID_LED_PSU2) {
            continue;
        }
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
    char* assert_str = "Asserted";

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
        }
        else {
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

        for(dev_num = 0; dev_num < dev_size; dev_num++) {
            memset(buf, 0, sizeof(buf));
            if( dev_num >= ID_FAN1_PSNT_L && dev_num <=ID_FAN4_PSNT_L ) {
                sprintf(get_data_cmd, CMD_BMC_CACHE_GET, bmc_cache[dev_num].name, 5);
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
            } else if(dev_num == ID_PSU1_PRSNT_L || dev_num ==ID_PSU2_PRSNT_L) {
                sprintf(get_data_cmd, CMD_BMC_CACHE_GET, bmc_cache[dev_num].name, 5);
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
            } else if(dev_num == ID_PSU1_PWROK_H || dev_num ==ID_PSU2_PWROK_H) {
                sprintf(get_data_cmd, CMD_BMC_CACHE_GET, bmc_cache[dev_num].name, 5);
                fp = popen(get_data_cmd, "r");
                if(fp != NULL) {
                    if(fgets(buf, sizeof(buf), fp) != NULL) {
                        if(strstr(buf, assert_str) != NULL) {
                            f_rv = 1;
                        } else {
                            f_rv = 0;
                        }
                        bmc_cache[dev_num].data = f_rv;
                    }
                }
                pclose(fp);
            } else if( dev_num >= ID_LED_SYS && dev_num <=ID_LED_PSU2 ) {
            #ifdef ENABLE_SYSLED
                sprintf(get_data_cmd, CMD_BMC_LED_GET, dev_num-ID_LED_SYS+1);
                fp = popen(get_data_cmd, "r");
                if(fp != NULL) {
                    if(fgets(buf, sizeof(buf), fp) != NULL) {
                        f_rv = atoi(buf);
                        bmc_cache[dev_num].data = f_rv;
                    }
                }
                pclose(fp);
            #else
                f_rv = BMC_LED_OFF;
                bmc_cache[dev_num].data = f_rv;
            #endif
            } else {
                sprintf(get_data_cmd, CMD_BMC_CACHE_GET, bmc_cache[dev_num].name, 2);
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
    // debug
    // AIM_LOG_INFO("bmc_cahe[%d]=%f\n", bmc_cache_index, *data);

    return rv;
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
    static long bmc_fru_cache_time[PSU_NUM+1] = {0,0, 0};  // index 0 for dummy
    char fru_model[] = "Product Name";  //only Product Name can identify AC/DC
    char fru_serial[] = "Product Serial";
    int cache_idx;

    // check fru id
    if(fru_id > PSU_NUM) {
        AIM_LOG_ERROR("invalid fru_id %d\n", fru_id);
        return ONLP_STATUS_E_INTERNAL;
    }

    cache_time = FRU_CACHE_TIME;
    snprintf(cache_file, sizeof(cache_file), BMC_FRU_CACHE, fru_id);

    if(check_file_exist(cache_file, &file_last_time))
    {
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

    //AIM_LOG_INFO("bmc_fru_cache_time = %ld, expire = %d\n",
    //    bmc_fru_cache_time[fru_id], bmc_cache_expired);

    //update cache
    if(bmc_cache_expired == 1)
    {
        ONLP_LOCK();
        if(bmc_cache_expired_check(file_last_time, bmc_fru_cache_time[fru_id], cache_time)) {
            // cache expired, update cache file
            snprintf(cache_cmd, sizeof(cache_cmd), CMD_BMC_FRU_CACHE, fru_id, cache_file);
            system(cache_cmd);
        }

        // handle psu model
        //Get psu fru from cache file
        snprintf(get_data_cmd, sizeof(get_data_cmd), CMD_CACHE_FRU_GET, cache_file, fru_model);
        if (exec_cmd(get_data_cmd, cmd_out, sizeof(cmd_out)) < 0) {
            AIM_LOG_ERROR("unable to read psu model from BMC, fru id=%d, cmd=%s, out=%s\n",
                fru_id, get_data_cmd, cmd_out);
            goto exit;
        }

        //Check output is correct
        if (strnlen(cmd_out, sizeof(cmd_out))==0){
            AIM_LOG_ERROR("unable to read psu model from BMC, fru id=%d, cmd=%s, out=%s\n",
                fru_id, get_data_cmd, cmd_out);
            goto exit;
        }

        // save to cache
        cache_idx = (fru_id - 1)*2;
        snprintf(bmc_fru_cache[cache_idx].data, sizeof(bmc_fru_cache[cache_idx].data), "%s", cmd_out);

        // handle psu serial
        //Get psu fru from cache file
        snprintf(get_data_cmd, sizeof(get_data_cmd), CMD_CACHE_FRU_GET, cache_file, fru_serial);
        if (exec_cmd(get_data_cmd, cmd_out, sizeof(cmd_out)) < 0) {
            AIM_LOG_ERROR("unable to read psu serial from BMC, fru id=%d, cmd=%s, out=%s\n",
                fru_id, get_data_cmd, cmd_out);
            goto exit;
        }

        //Check output is correct
        if (strnlen(cmd_out, sizeof(cmd_out))==0){
            AIM_LOG_ERROR("unable to read psu serial from BMC, fru id=%d, cmd=%s, out=%s\n",
                fru_id, get_data_cmd, cmd_out);
            goto exit;
        }

        // save to cache
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

int bmc_thermal_info_get(onlp_thermal_info_t* info, int id)
{
    int rc=0;
    float data=0;
    int index;
    index = id - THERMAL_ID_CPU_PECI;

    rc = bmc_sensor_read(index, THERMAL_SENSOR, &data);
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
    float data=0;
    int sys_max_fan_speed = MAX_SYS_FAN_RPM;
    int psu_max_fan_speed = MAX_PSU_FAN_RPM;

    //check presence for fantray 1-4
    if (id >= FAN_ID_FAN1 && id <= FAN_ID_FAN4) {
        rv = bmc_sensor_read(id + ID_FAN1_PSNT_L - 1, FAN_SENSOR, &data);
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
    rv = bmc_sensor_read(id + ID_FAN1_RPM - 1, FAN_SENSOR, &data);
    if ( rv != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%d\n", id);
        return rv;
    }
    rpm = (int) data;

    //set rpm field
    info->rpm = rpm;

    if (id >= FAN_ID_FAN1 && id <= FAN_ID_FAN4) {
        percentage = (info->rpm*100)/sys_max_fan_speed;
        info->percentage = percentage;
        info->status |= (rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;
    } else if (id >= FAN_ID_PSU1_FAN1 && id <= FAN_ID_PSU2_FAN1) {
        percentage = (info->rpm*100)/psu_max_fan_speed;
        info->percentage = percentage;
        info->status |= (rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;
    }

    return ONLP_STATUS_OK;
}

int sys_led_info_get(onlp_led_info_t* info, int id)
{
    int rv;
    float data=0;
    int led_val;

    rv = bmc_sensor_read(id + ID_LED_SYS - 1, LED_SENSOR, &data);
    if ( rv != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%d\n", id);
        return rv;
    }

    led_val = (int) data;

    switch(led_val) {
        case BMC_LED_OFF:
            info->status &= ~ONLP_LED_STATUS_ON;
            info->mode = ONLP_LED_MODE_OFF;
            break;
        case BMC_LED_GREEN:
            info->status |= ONLP_LED_STATUS_ON;
            info->mode = ONLP_LED_MODE_GREEN;
            break;
        case BMC_LED_YELLOW:
            info->status |= ONLP_LED_STATUS_ON;
            info->mode = ONLP_LED_MODE_YELLOW;
            break;
    }

    return ONLP_STATUS_OK;
}

int sysi_platform_info_get(onlp_platform_info_t* pi)
{
    char sysfs[128];
    int cpld_ver, cpld_release, cpld_revision;
    char mb_cpld_ver_h[CPLD_MAX][16];
    char bios_ver_h[32];
    char bmc_ver[3][16];
    int cpld_barod_type, ext_board_id, sku_id, hw_id, build_id;
    int rc=0;
    int i;

    memset(sysfs, 0, sizeof(sysfs));
    memset(mb_cpld_ver_h, 0, sizeof(mb_cpld_ver_h));
    memset(bios_ver_h, 0, sizeof(bios_ver_h));
    memset(bmc_ver, 0, sizeof(bmc_ver));

    //get MB CPLD version from CPLD sysfs
    for(i=0; i<CPLD_MAX; ++i) {
        snprintf(sysfs, sizeof(sysfs), MB_CPLD_SYSFS_PATH"/"MB_CPLD_VER_ATTR,
            CPLD_BUS_BASE+i, CPLD_I2C_ADDR);
        if ((rc = file_read_hex(&cpld_ver, sysfs)) != ONLP_STATUS_OK) {
            AIM_LOG_ERROR("file_read_hex failed, error=%d, %s", rc, sysfs);
            return ONLP_STATUS_E_INTERNAL;
        }

        cpld_release = (((cpld_ver) >> 6 & 0x01));
        cpld_revision = (((cpld_ver) & 0x3F));
        snprintf(mb_cpld_ver_h[i], sizeof(mb_cpld_ver_h[i]), "%d.%02d",
                    cpld_release, cpld_revision);
    }

    pi->cpld_versions = aim_fstrdup(
        "\n"
        "[MB CPLD1] %s\n"
        "[MB CPLD2] %s\n"
        "[MB CPLD3] %s\n"
        "[MB CPLD4] %s\n"
        "[MB CPLD5] %s\n",
        mb_cpld_ver_h[0],
        mb_cpld_ver_h[1],
        mb_cpld_ver_h[2],
        mb_cpld_ver_h[3],
        mb_cpld_ver_h[4]);

    //Get board info
    snprintf(sysfs, sizeof(sysfs), MB_CPLD_SYSFS_PATH"/"MB_CPLD_BOARD_TYPE_ATTR,
        I2C_BUS_CPLD1, CPLD_I2C_ADDR);
    if ((rc = file_read_hex(&cpld_barod_type, sysfs)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("unable to read cpld board type\n");
        return ONLP_STATUS_E_INTERNAL;
    }
    sku_id = (cpld_barod_type & 0b11110000) >> 4;
    hw_id = (cpld_barod_type & 0b00001100) >> 2;
    build_id = (cpld_barod_type & 0b00000011) >> 0;

    snprintf(sysfs, sizeof(sysfs), MB_CPLD_SYSFS_PATH"/"MB_CPLD_EXT_BOARD_TYPE_ATTR,
        I2C_BUS_CPLD1, CPLD_I2C_ADDR);
    if ((rc = file_read_hex(&ext_board_id, sysfs)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("unable to read cpld extend board type\n");
        return ONLP_STATUS_E_INTERNAL;
    }

    //Get BIOS version
    if (exec_cmd(CMD_BIOS_VER, bios_ver_h, sizeof(bios_ver_h)) < 0) {
        AIM_LOG_ERROR("unable to read BIOS version\n");
        return ONLP_STATUS_E_INTERNAL;
    }

    //Get BMC version
    if (exec_cmd(CMD_BMC_VER_1, bmc_ver[0], sizeof(bmc_ver[0])) < 0 ||
        exec_cmd(CMD_BMC_VER_2, bmc_ver[1], sizeof(bmc_ver[1])) < 0 ||
        exec_cmd(CMD_BMC_VER_3, bmc_ver[2], sizeof(bmc_ver[2]))) {
            AIM_LOG_ERROR("unable to read BMC version\n");
            return ONLP_STATUS_E_INTERNAL;
    }

    pi->other_versions = aim_fstrdup(
        "\n"
        "[SKU ID] %d\n"
        "[HW ID] %d\n"
        "[BUILD ID] %d\n"
        "[EXT BOARD ID] %d\n"
        "[BIOS] %s\n"
        "[BMC] %d.%d.%d\n",
        sku_id, hw_id, build_id, ext_board_id,
        bios_ver_h,
        atoi(bmc_ver[0]), atoi(bmc_ver[1]), atoi(bmc_ver[2]));

    return ONLP_STATUS_OK;
}
