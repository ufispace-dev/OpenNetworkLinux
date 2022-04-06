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

const char * thermal_id_str[] = {
    "",
    "TEMP_MAC",
    "PSU0_TEMP1",
    "PSU1_TEMP1",
    "FAN_0",
    "FAN_1",
    "FAN_2",
    "PSU0_FAN",
    "PSU1_FAN",
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
    "TEMP_FRONT_ENV",
};

const char * fan_id_str[] = {
    "",
    "FAN0_RPM",
    "FAN1_RPM",
    "FAN2_RPM",
    "PSU0_FAN",
    "PSU1_FAN",
};

const char * fan_id_presence_str[] = {
    "",
    "FAN0_PRSNT_H",
    "FAN1_PRSNT_H",
    "FAN2_PRSNT_H",
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
    [BMC_ATTR_ID_TEMP_MAC]     = {"TEMP_MAC", 0},
    [BMC_ATTR_ID_PSU0_TEMP1]   = {"PSU0_TEMP1", 0},
    [BMC_ATTR_ID_PSU1_TEMP1]   = {"PSU1_TEMP1", 0},
    [BMC_ATTR_ID_FAN_0]        = {"FAN_0", 0},
    [BMC_ATTR_ID_FAN_1]        = {"FAN_1", 0},
    [BMC_ATTR_ID_FAN_2]        = {"FAN_2", 0},
    [BMC_ATTR_ID_PSU0_FAN]     = {"PSU0_FAN", 0},
    [BMC_ATTR_ID_PSU1_FAN]     = {"PSU1_FAN", 0},
    [BMC_ATTR_ID_PSU0_VIN]     = {"PSU0_VIN", 0},
    [BMC_ATTR_ID_PSU0_VOUT]    = {"PSU0_VOUT", 0},
    [BMC_ATTR_ID_PSU0_IIN]     = {"PSU0_IIN",0},
    [BMC_ATTR_ID_PSU0_IOUT]    = {"PSU0_IOUT",0},
    [BMC_ATTR_ID_PSU0_STBVOUT] = {"PSU0_STBVOUT", 0},
    [BMC_ATTR_ID_PSU0_STBIOUT] = {"PSU0_STBIOUT", 0},
    [BMC_ATTR_ID_PSU1_VIN]     = {"PSU1_VIN", 0},
    [BMC_ATTR_ID_PSU1_VOUT]    = {"PSU1_VOUT", 0},
    [BMC_ATTR_ID_PSU1_IIN]     = {"PSU1_IIN", 0},
    [BMC_ATTR_ID_PSU1_IOUT]    = {"PSU1_IOUT", 0},
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
                    /* other attribut, got from bmc */
                    bmc_cache[i].data = atof(line_fields[1]);
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
    int rc;
    int mask;
    int reg, reg_val;

    if (id == PSU_ID_PSU0) {
        mask = PSU0_PRESENT_MASK;
    } else if (id == PSU_ID_PSU1) {
        mask = PSU1_PRESENT_MASK;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    //read cpld reg via LPC
    reg = CPLD_REG_BASE + CPLD_REG_PSU_STATUS;
    if ((rc=read_ioport(reg, &reg_val)) != ONLP_STATUS_OK) {
        return rc;
    }

    *pw_present = ((mask_shift(reg_val, mask))? 0 : 1);

    return ONLP_STATUS_OK;
}

int psu_pwgood_get(int *pw_good, int id)
{
    int rc;
    int mask;
    int reg, reg_val;

    if (id == PSU_ID_PSU0) {
        mask = PSU0_PWGOOD_MASK;
    } else if (id == PSU_ID_PSU1) {
        mask = PSU1_PWGOOD_MASK;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    //read cpld reg via LPC
    reg = CPLD_REG_BASE + CPLD_REG_PSU_STATUS;
    if ((rc=read_ioport(reg, &reg_val)) != ONLP_STATUS_OK) {
        return rc;
    }

    *pw_good = ((mask_shift(reg_val, mask))? 1 : 0);

    return ONLP_STATUS_OK;
}

int sfp_present_get(int port, int *val)
{
    int rc, gpio_num;
    char sysfs[48];

    memset(sysfs, 0, sizeof(sysfs));

    //set gpio_num by port
    if (port>=4 && port<8) {
        gpio_num = 391 - port;
    } else if (port>=8 && port<16) {
        gpio_num = 399 - (port-8);
    } else if (port>=16 && port<24) {
        gpio_num = 375 - (port-16);
    } else if (port>=24 && port<28) {
        gpio_num = 383 - (port-24);
    }

    snprintf(sysfs, sizeof(sysfs), SYS_GPIO_VAL, gpio_num);

    if ((rc = onlp_file_read_int(val, sysfs)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("sfp_present_get() failed, error=%d, sysfs=%s", rc, sysfs);
        return ONLP_STATUS_E_INTERNAL;
    }

    *val = (*val) ? 0 : 1;

    return ONLP_STATUS_OK;
}

int read_ioport(int addr, int *reg_val)
{
    /*set r/w permission of all 65536 ports*/
    ONLP_TRY(iopl(3));
    *reg_val = inb(addr);

    /*set r/w permission of  all 65536 ports*/
    ONLP_TRY(iopl(0));
    return ONLP_STATUS_OK;
}

int write_ioport(int addr, int reg_val)
{
    /*enable io port*/
    ONLP_TRY(iopl(3));
    outb(reg_val, addr);

    /*disable io port*/
    ONLP_TRY(iopl(0));
    return ONLP_STATUS_OK;
}

int exec_cmd(char *cmd, char* out, int size)
{
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

int get_ipmitool_len(char *ipmitool_out)
{
    size_t str_len=0, ipmitool_len=0;
    str_len = strlen(ipmitool_out);
    if (str_len>0) {
        ipmitool_len = str_len/3;
    }
    return ipmitool_len;
}

int parse_ucd_out(char *ucd_out, char *ucd_data, int start, int len)
{
    int i=0;
    char data[3];

    memset(data, 0, sizeof(data));

    for (i=0; i<len; ++i) {
        data[0] = ucd_out[(start+i)*3 + 1];
        data[1] = ucd_out[(start+i)*3 + 2];
        //hex string to int
        ucd_data[i] = (int) strtol(data, NULL, 16);
    }
    return ONLP_STATUS_OK;
}

int sysi_platform_info_get(onlp_platform_info_t* pi)
{
    int cpld_ver[CPLD_MAX];
    int mb_cpld1_addr = CPLD_REG_BASE, mb_cpld1_board_type_rev, mb_cpld1_hw_rev, mb_cpld1_build_rev;
    int i;
    char bios_out[32];
    char bmc_out1[8], bmc_out2[8], bmc_out3[8];
    char ucd_out[48];
    char ucd_ver[8];
    char ucd_date[8];
    int ucd_len=0;
    //int ucd_date_len=6;

    memset(bios_out, 0, sizeof(bios_out));
    memset(bmc_out1, 0, sizeof(bmc_out1));
    memset(bmc_out2, 0, sizeof(bmc_out2));
    memset(bmc_out3, 0, sizeof(bmc_out3));
    memset(ucd_out, 0, sizeof(ucd_out));
    memset(ucd_ver, 0, sizeof(ucd_ver));
    memset(ucd_date, 0, sizeof(ucd_date));

    //get MB CPLD version
    for(i=0; i<CPLD_MAX; ++i) {
        ONLP_TRY(read_ioport(CPLD_REG_BASE+CPLD_REG_VER, &cpld_ver[i]));
        if (cpld_ver[i] < 0) {
	        AIM_LOG_ERROR("unable to read MB CPLD version\n");
            return ONLP_STATUS_E_INTERNAL;
        }

        cpld_ver[i] = (((cpld_ver[i]) & 0xFF));
    }

    pi->cpld_versions = aim_fstrdup(
        "\n"
        "[MB CPLD] v%02d\n",
        cpld_ver[0]);

    //Get HW Build Version
    ONLP_TRY(read_ioport(mb_cpld1_addr, &mb_cpld1_board_type_rev));

    mb_cpld1_hw_rev = (((mb_cpld1_board_type_rev) >> 4 & 0x03));
    mb_cpld1_build_rev = (((mb_cpld1_board_type_rev) >> 6 & 0x03));

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

    //get UCD version
    if (exec_cmd(CMD_UCD_VER, ucd_out, sizeof(ucd_out)) < 0) {
            AIM_LOG_ERROR("unable to read UCD version\n");
            return ONLP_STATUS_E_INTERNAL;
    }

    //parse UCD version and date
    ucd_len = get_ipmitool_len(ucd_out);
    /*
    if (ucd_len > ucd_date_len) {
        parse_ucd_out(ucd_out, ucd_ver, 0, ucd_len-ucd_date_len);
        parse_ucd_out(ucd_out, ucd_date, ucd_len-ucd_date_len, ucd_date_len);
    } else {
        parse_ucd_out(ucd_out, ucd_ver, 0, ucd_len);
    }
    */
    parse_ucd_out(ucd_out, ucd_ver, 0, ucd_len);

    pi->other_versions = aim_fstrdup(
        "\n"
        "[HW     ] %d\n"
        "[BUILD  ] %d\n"
        "[BIOS   ] %s\n"
        "[BMC    ] %d.%d.%d\n"
        //"[UCD    ] %s %s\n",
        "[UCD    ] %s\n",
        mb_cpld1_hw_rev,
        mb_cpld1_build_rev,
        bios_out,
        atoi(bmc_out1), atoi(bmc_out2), atoi(bmc_out3),
        //ucd_ver, ucd_date);
        ucd_ver);

    return ONLP_STATUS_OK;
}

bool
onlp_sysi_bmc_en_get(void)
{
//enable bmc by default
#if 0
    int value;

    if (onlp_file_read_int(&value, BMC_EN_FILE_PATH) < 0) {
        // flag file not exist, default to not enable
        return false;
    }

    /* 1 - enable, 0 - no enable */
    if ( value )
        return true;

    return false;
#endif
   return true;
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

    rc = bmc_sensor_read(id - THERMAL_ID_MAC, THERMAL_SENSOR, &data);
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
    float data=0;
    int sys_max_fan_speed = 25000;
    int psu_max_fan_speed = 18000;
    int max_fan_speed = 0;

    //check presence for fantray 1-3
    if (id >= FAN_ID_FAN0 && id <= FAN_ID_FAN2) {
        //get fan present status
        if ( (rv=sys_fan_present_get(info, id)) != ONLP_STATUS_OK) {
            AIM_LOG_ERROR("unable to read fan present from CPLD, sensor=%d, ret=%d\n", id, rv);
            return rv;
        }

        //return if fan not present
        if( (info->status & ONLP_FAN_STATUS_PRESENT) != ONLP_FAN_STATUS_PRESENT ) {
            return ONLP_STATUS_OK;
        }
    }

    //get fan rpm
    rv = bmc_sensor_read(id - FAN_ID_FAN0 + 3, FAN_SENSOR, &data);
    if ( rv != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("unable to read sensor info from BMC, sensor=%d\n", id);
        return rv;
    }
    rpm = (int) data;

    //set rpm field
    info->rpm = rpm;

    if (id >= FAN_ID_FAN0 && id <= FAN_ID_FAN2) {
        percentage = (info->rpm*100)/sys_max_fan_speed;
        info->percentage = percentage;
        info->status |= (rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;
    } else if (id >= FAN_ID_PSU0_FAN && id <= FAN_ID_PSU1_FAN) {
        max_fan_speed = psu_max_fan_speed;

        percentage = (info->rpm*100)/max_fan_speed;
        info->percentage = percentage;
        info->status |= (rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;
    }

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

    snprintf(cmd_buf, sizeof(cmd_buf), CMD_I2C_STUCK_CHECK);
    ret = system(cmd_buf);
    if (ret != 0) {
        if( access( CMD_I2C_STUCK_RECOVERY, F_OK ) != -1 ) {
            system(CMD_I2C_STUCK_RECOVERY);
        } else {
            AIM_LOG_ERROR("I2C Stuck recovery binary not exist, path=%s\r\n", I2C_RECOVERY_BIN_PATH);
        }
    }


}

int sys_led_info_get(onlp_led_info_t* info, int id)
{
    int rc;
    int reg, reg_val_ctrl, reg_val_blnk;
    int ctrl_offset = 0;
    int mask_onoff, mask_color, mask_blink;
    int led_val_color, led_val_blink, led_val_onoff;

    if (id == LED_ID_SYS_GNSS) {
        mask_onoff = 0b00000100;
        mask_color = 0b00001000;
        mask_blink  = 0b00000010;
        ctrl_offset = CPLD_REG_LED_CTRL;
    } else if (id == LED_ID_SYS_SYNC) {
        mask_onoff = 0b00000001;
        mask_color = 0b00000010;
        mask_blink  = 0b00000001;
        ctrl_offset = CPLD_REG_LED_CTRL;
    } else if (id == LED_ID_SYS_SYS) {
        mask_onoff = 0b00010000;
        mask_color = 0b00100000;
        mask_blink  = 0b00000100;
        ctrl_offset = CPLD_REG_LED_CTRL;
    } else if (id == LED_ID_SYS_FAN) {
        mask_onoff = 0b00010000;
        mask_color = 0b00100000;
        mask_blink  = 0b00001000;
        ctrl_offset = CPLD_REG_LED_CLEAR;
    } else if (id == LED_ID_SYS_PSU) {
        mask_onoff = 0b01000000;
        mask_color = 0b10000000;
        mask_blink  = 0b00010000;
        ctrl_offset = CPLD_REG_LED_CLEAR;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    //read cpld reg via LPC
    reg = CPLD_REG_BASE + ctrl_offset;
    if ((rc=read_ioport(reg, &reg_val_ctrl)) != ONLP_STATUS_OK) {
        return rc;
    }

    reg = CPLD_REG_BASE + CPLD_REG_LED_BLNK;
    if ((rc=read_ioport(reg, &reg_val_blnk)) != ONLP_STATUS_OK) {
        return rc;
    }

    led_val_onoff = mask_shift(reg_val_ctrl, mask_onoff);
    led_val_color = mask_shift(reg_val_ctrl, mask_color);
    led_val_blink = mask_shift(reg_val_blnk, mask_blink);

    //onoff
    if (led_val_onoff == 0) {
        info->mode = ONLP_LED_MODE_OFF;
    } else {
        //color
        if (led_val_color == 0) {
            info->mode = ONLP_LED_MODE_YELLOW;
        } else {
            info->mode = ONLP_LED_MODE_GREEN;
        }
        //blinking
        if (led_val_blink == 1) {
            info->mode = info->mode + 1;
        }
    }

    return ONLP_STATUS_OK;
}

int sys_led_mode_set(int id, int color, int blink, int onoff)
{
    int ctrl_offset = 0, rc = 0, reg = 0;
    int mask_onoff, mask_color, mask_blink;
    int data;

    if (id == LED_ID_SYS_SYS) {
        mask_onoff = 0b00010000;
        mask_color = 0b00100000;
        mask_blink  = 0b00000100;
        ctrl_offset = CPLD_REG_LED_CTRL;
    } else if (id == LED_ID_SYS_SYNC) {
        mask_onoff = 0b00000001;
        mask_color = 0b00000010;
        mask_blink  = 0b00000001;
        ctrl_offset = CPLD_REG_LED_CTRL;
    } else if (id == LED_ID_SYS_GNSS) {
        mask_onoff = 0b00000100;
        mask_color = 0b00001000;
        mask_blink  = 0b00000010;
        ctrl_offset = CPLD_REG_LED_CTRL;
    } else {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    //read cpld reg via LPC
    reg = CPLD_REG_BASE + ctrl_offset;
    if ((rc=read_ioport(reg, &data)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("unable to read LED control register\n");
        return rc;
    }

    if (onoff == LED_ON) {
        data |= mask_onoff;
    } else {
        data &= ~mask_onoff;
    }
    if (color == LED_COLOR_GREEN) {
        data |= mask_color;
    } else {
        data &= ~mask_color;
    }

    /* Set control status */
    if (write_ioport(reg, data) < 0) {
        AIM_LOG_ERROR("unable to write LED control register\n");
        return ONLP_STATUS_E_INTERNAL;
    }

    /* Get blinking status */
    reg = CPLD_REG_BASE + CPLD_REG_LED_BLNK;
    if ((rc=read_ioport(reg, &data)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("unable to read LED control register\n");
        return rc;
    }

    if (blink == LED_BLINKING) {
        data |= mask_blink;
    } else {
        data &= ~mask_blink;
    }

    /* Set blinking status */
    if (write_ioport(reg, data) < 0) {
        AIM_LOG_ERROR("unable to write LED blinking register\n");
        return ONLP_STATUS_E_INTERNAL;
    }


    return ONLP_STATUS_OK;
}

int sys_fan_present_get(onlp_fan_info_t* info, int id)
{
    int rc;
    int mask;
    int reg, reg_val;
    int fan_present;

    if (id == FAN_ID_FAN0) {
        mask = FAN0_PRESENT_MASK;
    } else if (id == FAN_ID_FAN1) {
        mask = FAN1_PRESENT_MASK;
    } else if (id == FAN_ID_FAN2) {
        mask = FAN2_PRESENT_MASK;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    //read cpld reg via LPC
    reg = CPLD_REG_BASE + CPLD_REG_FAN_PRNT;
    if ((rc=read_ioport(reg, &reg_val)) != ONLP_STATUS_OK) {
        return rc;
    }

    fan_present = ((mask_shift(reg_val, mask))? 0 : 1);

    if( fan_present == 1 ) {
        info->status |= ONLP_FAN_STATUS_PRESENT;
    } else {
        info->status &= ~ONLP_FAN_STATUS_PRESENT;
    }

    return ONLP_STATUS_OK;
}
