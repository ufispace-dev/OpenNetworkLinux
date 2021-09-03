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

const int CPLD_BASE_ADDR[] = {0x30, 0x31, 0x32, 0x33, 0x34};
const int CPLD_I2C_BUS[] = {1, 1, 1, 30, 30};

bmc_info_t bmc_cache[] =
{
    [BMC_ATTR_ID_ADC_CPU_TEMP] = {"ADC_CPU_TEMP", 0},
    [BMC_ATTR_ID_TEMP_CPU_PECI] = {"TEMP_CPU_PECI", 0},
    [BMC_ATTR_ID_TEMP_MAC_ENV_1] = {"TEMP_MAC_ENV_1", 0},
    [BMC_ATTR_ID_TEMP_MAC_ENV_2] = {"TEMP_MAC_ENV_2", 0},
    [BMC_ATTR_ID_TEMP_FRONT_ENV_1] = {"TEMP_FRONT_ENV_1", 0},
    [BMC_ATTR_ID_TEMP_FRONT_ENV_2] = {"TEMP_FRONT_ENV_2", 0},
    [BMC_ATTR_ID_TEMP_ENV_1] = {"TEMP_ENV_1", 0},
    [BMC_ATTR_ID_TEMP_ENV_2] = {"TEMP_ENV_2", 0},
    [BMC_ATTR_ID_TEMP_EXT_ENV_1] = {"TEMP_EXT_ENV_1", 0},
    [BMC_ATTR_ID_TEMP_EXT_ENV_2] = {"TEMP_EXT_ENV_2", 0},
    [BMC_ATTR_ID_TEMP_MAC0_PVT2] = {"TEMP_MAC0_PVT2", 0},
    [BMC_ATTR_ID_TEMP_MAC0_PVT3] = {"TEMP_MAC0_PVT3", 0},
    [BMC_ATTR_ID_TEMP_MAC0_PVT4] = {"TEMP_MAC0_PVT4", 0},
    [BMC_ATTR_ID_TEMP_MAC0_PVT6] = {"TEMP_MAC0_PVT6", 0},
    [BMC_ATTR_ID_TEMP_MAC0_HBM0] = {"TEMP_MAC0_HBM0", 0},
    [BMC_ATTR_ID_TEMP_MAC0_HBM1] = {"TEMP_MAC0_HBM1", 0},
    [BMC_ATTR_ID_TEMP_MAC1_PVT2] = {"TEMP_MAC1_PVT2", 0},
    [BMC_ATTR_ID_TEMP_MAC1_PVT3] = {"TEMP_MAC1_PVT3", 0},
    [BMC_ATTR_ID_TEMP_MAC1_PVT4] = {"TEMP_MAC1_PVT4", 0},
    [BMC_ATTR_ID_TEMP_MAC1_PVT6] = {"TEMP_MAC1_PVT6", 0},
    [BMC_ATTR_ID_TEMP_MAC1_HBM0] = {"TEMP_MAC1_HBM0", 0},
    [BMC_ATTR_ID_TEMP_MAC1_HBM1] = {"TEMP_MAC1_HBM1", 0},
    [BMC_ATTR_ID_TEMP_OP2_0] = {"TEMP_OP2_0", 0},
    [BMC_ATTR_ID_TEMP_OP2_1] = {"TEMP_OP2_1", 0},
    [BMC_ATTR_ID_TEMP_OP2_2] = {"TEMP_OP2_2", 0},
    [BMC_ATTR_ID_TEMP_OP2_3] = {"TEMP_OP2_3", 0},    
    [BMC_ATTR_ID_PSU0_TEMP] = {"PSU0_TEMP", 0},
    [BMC_ATTR_ID_PSU1_TEMP] = {"PSU1_TEMP", 0},
    [BMC_ATTR_ID_FAN0_FRONT_RPM] = {"FAN0_FRONT_RPM", 0},
    [BMC_ATTR_ID_FAN0_REAR_RPM] = {"FAN0_REAR_RPM", 0},
    [BMC_ATTR_ID_FAN1_FRONT_RPM] = {"FAN1_FRONT_RPM", 0},
    [BMC_ATTR_ID_FAN1_REAR_RPM] = {"FAN1_REAR_RPM", 0},
    [BMC_ATTR_ID_FAN2_FRONT_RPM] = {"FAN2_FRONT_RPM", 0},
    [BMC_ATTR_ID_FAN2_REAR_RPM] = {"FAN2_REAR_RPM", 0},
    [BMC_ATTR_ID_FAN3_FRONT_RPM] = {"FAN3_FRONT_RPM", 0},
    [BMC_ATTR_ID_FAN3_REAR_RPM] = {"FAN3_REAR_RPM", 0},
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
    struct timeval new_tv;
    FILE *fp = NULL;
    char ipmi_cmd[1024] = {0};
    char get_data_cmd[256] = {0};
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
                if ((rv=system(ipmi_cmd)) < 0) {
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
            //search present string
            if( dev_num >= BMC_ATTR_ID_FAN0_PRSNT_H && dev_num <= BMC_ATTR_ID_FAN3_PRSNT_H ) {                
                snprintf(get_data_cmd, sizeof(get_data_cmd), CMD_BMC_CACHE_GET, bmc_cache[dev_num].name, 5);
                fp = popen(get_data_cmd, "r");
                if(fp != NULL)
                {
                    if(fgets(buf, sizeof(buf), fp) != NULL)
                    {
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
                if(fp != NULL)
                {
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
    int i=0, mask_one=1;

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
    int shift=0;

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
