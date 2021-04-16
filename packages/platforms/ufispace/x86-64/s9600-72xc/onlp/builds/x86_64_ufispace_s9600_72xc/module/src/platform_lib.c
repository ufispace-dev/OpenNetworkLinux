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
    "TEMP_CPU_PECI",    
    "TEMP_CPU_ENV",
    "TEMP_CPU_ENV_2",
    "TEMP_MAC_FRONT",
    "TEMP_MAC_DIE",
    "TEMP_0x48",
    "TEMP_0x49",
    "PSU0_TEMP",
    "PSU1_TEMP",    
    "CPU_PACKAGE",
    "CPU1",
    "CPU2",
    "CPU3",
    "CPU4",
    "CPU5",
    "CPU6",
    "CPU7",
    "CPU8",
};

const char * fan_id_str[] = {
    "",
    "FAN0_RPM",
    "FAN1_RPM",
    "FAN2_RPM",
    "FAN3_RPM",
    "PSU0_FAN1",
    "PSU1_FAN1",
};

const char * fan_id_presence_str[] = {
    "",
    "FAN0_PSNT_L",
    "FAN1_PSNT_L",
    "FAN2_PSNT_L",
    "FAN3_PSNT_L",    
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
    {"TEMP_CPU_PECI", 0},
    {"TEMP_CPU_ENV", 0},
    {"TEMP_CPU_ENV_2", 0},
    {"TEMP_MAC_FRONT", 0},
    {"TEMP_MAC_DIE", 0},
    {"TEMP_0x48", 0},
    {"TEMP_0x49", 0},
    {"PSU0_TEMP", 0},
    {"PSU1_TEMP", 0},
    {"FAN0_RPM", 0},
    {"FAN1_RPM", 0},
    {"FAN2_RPM", 0},
    {"FAN3_RPM", 0},
    {"PSU0_FAN1", 0},
    {"PSU1_FAN1", 0},
    {"FAN0_PSNT_L",0},
    {"FAN1_PSNT_L",0},
    {"FAN2_PSNT_L", 0},
    {"FAN3_PSNT_L", 0},
    {"PSU0_VIN", 0},
    {"PSU0_VOUT", 0},
    {"PSU0_IIN",0},
    {"PSU0_IOUT",0},
    {"PSU0_STBVOUT", 0},
    {"PSU0_STBIOUT", 0},
    {"PSU1_VIN", 0},
    {"PSU1_VOUT", 0},
    {"PSU1_IIN", 0},
    {"PSU1_IOUT", 0},
    {"PSU1_STBVOUT", 0},
    {"PSU1_STBIOUT", 0}
};

bmc_fru_info_t bmc_fru_cache[] =
{
    {"PSU0_MODEL", ""},
    {"PSU0_SERIAL", ""},
    {"PSU1_MODEL", ""},
    {"PSU1_SERIAL", ""},
};

const char * sfp_group_str[] = {
    "0_7",
    "8_15",
    "16_23",
    "24_31",
    "32_39",
    "40_47",
    "48_55",
    "56_63",
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

int
check_file_exist(char *file_path, long *file_time) 
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

int
bmc_cache_expired_check(long last_time, long new_time, int cache_time)
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

void
get_bmc_cache_dev_names(int dev_size, char *devs)
{
    int dev_num;
    devs[0] = '\0';
    for(dev_num = 0; dev_num < dev_size; dev_num++) {
        strcat(devs, bmc_cache[dev_num].name);
        strcat(devs, " ");
    }
}

int
bmc_sensor_read(int bmc_cache_index, int sensor_type, float *data)
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
        }
        else {
            bmc_cache_expired = 0;
        }
    }
    else {
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
            get_bmc_cache_dev_names(dev_size, dev_names);
            snprintf(ipmi_cmd, sizeof(ipmi_cmd), CMD_BMC_SENSOR_CACHE, dev_names);
            system(ipmi_cmd);
        }

        for(dev_num = 0; dev_num < dev_size; dev_num++)
        {
            memset(buf, 0, sizeof(buf));

            if( dev_num >= ID_FAN0_PSNT_L && dev_num <=ID_FAN3_PSNT_L ) {                
                sprintf(get_data_cmd, CMD_BMC_CACHE_GET, bmc_cache[dev_num].name, 5);
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
                sprintf(get_data_cmd, CMD_BMC_CACHE_GET, bmc_cache[dev_num].name, 2);
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
    // debug
    // AIM_LOG_INFO("bmc_cahe[%d]=%f\n", bmc_cache_index, *data);
    
    return rv;
}


int
bmc_fru_read(onlp_psu_info_t* info, int fru_id)
{
    struct timeval new_tv;
    char cache_cmd[512] = {0};
    char get_data_cmd[128] = {0};
    char cmd_out[128] = {0};
    char cache_file[128] = {0};
    int rv = ONLP_STATUS_OK;
    int cache_time = 0;
    int bmc_cache_expired = 0;
    long file_last_time = 0;
    static long bmc_fru_cache_time = 0;
    char fru_model[] = "Product Name";  //only Product Name can identify AC/DC
    char fru_serial[] = "Product Serial";
    int cache_idx;


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

    if(bmc_fru_cache_time == 0 && check_file_exist(cache_file, &file_last_time)) {
        bmc_cache_expired = 1;
        gettimeofday(&new_tv,NULL);
        bmc_fru_cache_time = new_tv.tv_sec;
    }

    //update cache
    if(bmc_cache_expired == 1)
    {
        ONLP_LOCK();
        if(bmc_cache_expired_check(file_last_time, bmc_fru_cache_time, cache_time)) {
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
            return ONLP_STATUS_E_INTERNAL; 
        }

        //Check output is correct    
        if (strnlen(cmd_out, sizeof(cmd_out))==0){
            AIM_LOG_ERROR("unable to read psu model from BMC, fru id=%d, cmd=%s, out=%s\n", 
                fru_id, get_data_cmd, cmd_out);
            return ONLP_STATUS_E_INTERNAL; 
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
            return ONLP_STATUS_E_INTERNAL; 
        }

        //Check output is correct    
        if (strnlen(cmd_out, sizeof(cmd_out))==0){
            AIM_LOG_ERROR("unable to read psu serial from BMC, fru id=%d, cmd=%s, out=%s\n", 
                fru_id, get_data_cmd, cmd_out);
            return ONLP_STATUS_E_INTERNAL; 
        }

        // save to cache
        cache_idx = (fru_id - 1)*2 + 1;
        snprintf(bmc_fru_cache[cache_idx].data, sizeof(bmc_fru_cache[cache_idx].data), "%s", cmd_out);


        gettimeofday(&new_tv,NULL);
        bmc_fru_cache_time = new_tv.tv_sec;
        ONLP_UNLOCK();
    }

    //read from cache
    cache_idx = (fru_id - 1)*2;
    snprintf(info->model, sizeof(info->model), "%s", bmc_fru_cache[cache_idx].data);
    cache_idx = (fru_id - 1)*2 + 1;
    snprintf(info->serial, sizeof(info->serial), "%s", bmc_fru_cache[cache_idx].data);
    
    return rv;
}

int
psu_present_get(int *pw_present, int id)
{
    int status, rc;
    int mask;

    if (id == PSU_ID_PSU0) {        
        mask = PSU0_PRESENT_MASK;
    } else if (id == PSU_ID_PSU1) {
        mask = PSU1_PRESENT_MASK;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }
    
    if ((rc = file_read_hex(&status, "%s/%s",
        MB_CPLD1_SYSFS_PATH, MB_CPLD_PSU_STS_ATTR)) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    }

    *pw_present = ((status & mask)? 0 : 1);
    
    return ONLP_STATUS_OK;
}

int
psu_pwgood_get(int *pw_good, int id)
{
    int status, rc;   
    int mask;

    if (id == PSU_ID_PSU0) {
        mask = PSU0_PWGOOD_MASK;
    } else if (id == PSU_ID_PSU1) {
        mask = PSU1_PWGOOD_MASK;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }
    
    if ((rc = file_read_hex(&status, "%s/%s",
        MB_CPLD1_SYSFS_PATH, MB_CPLD_PSU_STS_ATTR)) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    }

    *pw_good = ((status & mask)? 1 : 0);
    
    return ONLP_STATUS_OK;
}

int
sfp_port_to_group(int port, int *port_group, int *port_index, int *port_mask)
{
    *port_group  = port / 8;
    *port_index = port % 8;
    *port_mask = 0b00000001 << *port_index;
    return ONLP_STATUS_OK;
}

int
sfp_present_get(int port_index, int *pres_val)
{     
    int rc;
    uint8_t data[8];
    int data_len;
    int port_group;
    int group_pres;
    int port_pres;
    int port_mask = 0b00000001;
    char sysfs[128];
    char *cpld_path;
   
    memset(data, 0, sizeof(data));

    //get mapping group
    rc = sfp_port_to_group(port_index, &port_group, &port_index, &port_mask);

    switch (port_group) {
        case 0:
        case 1:
        case 4:
        case 5:
            cpld_path = MB_CPLD2_SYSFS_PATH;
            break;
        case 2:
        case 3:
        case 6:
        case 7:
            cpld_path = MB_CPLD3_SYSFS_PATH;
            break;
        default:
            AIM_LOG_ERROR("invalid port=%d, group=%d", port_index, port_group);
    }
    
    snprintf(sysfs, sizeof(sysfs), "%s/"MB_CPLD_SFP_GROUP_PRES_ATTR, 
                cpld_path, sfp_group_str[port_group]);
    if ((rc = onlp_file_read(data, sizeof(data), &data_len, sysfs))
         != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("onlp_file_read failed, error=%d, %s", rc, sysfs);
        return ONLP_STATUS_E_INTERNAL;
    }
    group_pres = (int) strtol((char *)data, NULL, 0);
    // val 0 for presence, pres_val set to 1
    port_pres = (group_pres & port_mask) >> port_index;
    if (port_pres) {
        *pres_val = 0;
    } else {
        *pres_val = 1;
    }
   
    return ONLP_STATUS_OK;
}

int 
qsfp_present_get(int port_index, int *pres_val) 
{
    int rc;
    uint8_t data[8];
    int data_len;
    int port_mask;
    int group_pres;
    int port_pres;
    char sysfs[128];
    
    port_mask = 0b00000001 << port_index;

    snprintf(sysfs, sizeof(sysfs), "%s/%s", 
            MB_CPLD4_SYSFS_PATH, MB_CPLD_QSFP_PRES_ATTR);
    if ((rc = onlp_file_read(data, sizeof(data), &data_len, sysfs))
            != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("onlp_file_read failed, error=%d, %s", rc, sysfs);
        return ONLP_STATUS_E_INTERNAL;
    }
    group_pres = (int) strtol((char *)data, NULL, 0);
    // val 0 for presence, pres_val set to 1
    port_pres = (group_pres & port_mask) >> port_index;
    if (port_pres) {
        *pres_val = 0;
    } else {
        *pres_val = 1;
    }

    return ONLP_STATUS_OK;
}

int
mgmt_sfp_present_get(int port_index, int *pres_val) 
{
    int rc;
    uint8_t data[8];
    int data_len;
    int port_mask;
    int group_pres;
    int port_pres;
    char sysfs[128];
    
    port_mask = 0b00000001 << (port_index*4);

    snprintf(sysfs, sizeof(sysfs), "%s/%s", 
            MB_CPLD1_SYSFS_PATH, MB_CPLD_MGMT_SFP_STATUS_ATTR);
    if ((rc = onlp_file_read(data, sizeof(data), &data_len, sysfs))
            != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("onlp_file_read failed, error=%d, %s", rc, sysfs);
        return ONLP_STATUS_E_INTERNAL;
    }
    group_pres = (int) strtol((char *)data, NULL, 0);
    // val 0 for presence, pres_val set to 1
    port_pres = (group_pres & port_mask) >> (port_index*4);
    if (port_pres) {
        *pres_val = 0;
    } else {
        *pres_val = 1;
    }

    return ONLP_STATUS_OK;

}

int
exec_cmd(char *cmd, char* out, int size) {
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

int
sysi_platform_info_get(onlp_platform_info_t* pi)
{
    char sysfs[128];
    uint8_t cpu_cpld_ver_h[32];
    int cpld_major_ver, cpld_minor_ver;
    char mb_cpld_ver_h[CPLD_MAX][16];
    char bios_ver_h[32];
    char bmc_ver[3][16];
    int sku_id, hw_id, id_type, build_id, deph_id;
    int data_len;
    int rc=0;
    int i;

    memset(sysfs, 0, sizeof(sysfs));
    memset(cpu_cpld_ver_h, 0, sizeof(cpu_cpld_ver_h));
    memset(mb_cpld_ver_h, 0, sizeof(mb_cpld_ver_h));
    memset(bios_ver_h, 0, sizeof(bios_ver_h));
    memset(bmc_ver, 0, sizeof(bmc_ver));

    //get CPU CPLD version readable string
    snprintf(sysfs, sizeof(sysfs), "%s/%s", 
                    LPC_CPU_CPLD_SYSFFS_PATH, LPC_CPU_CPLD_VER_ATTR);
    if ((rc = onlp_file_read(cpu_cpld_ver_h, sizeof(cpu_cpld_ver_h), 
                &data_len, sysfs)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("onlp_file_read failed, error=%d, %s", rc, sysfs);
        return ONLP_STATUS_E_INTERNAL;
    }
    // trim new line
    cpu_cpld_ver_h[strcspn((char *)cpu_cpld_ver_h, "\n" )] = '\0';
    
    //get MB CPLD version from CPLD sysfs
    for(i=0; i<CPLD_MAX; ++i) {
        snprintf(sysfs, sizeof(sysfs), 
                    MB_CPLDX_SYSFS_PATH"/"MB_CPLD_MAJOR_VER_ATTR, CPLD_BASE_ADDR+i);
        if ((rc = file_read_hex(&cpld_major_ver, sysfs)) != ONLP_STATUS_OK) {
            AIM_LOG_ERROR("file_read_hex failed, error=%d, %s", rc, sysfs);
            return ONLP_STATUS_E_INTERNAL;
        }
        snprintf(sysfs, sizeof(sysfs), 
                    MB_CPLDX_SYSFS_PATH"/"MB_CPLD_MINOR_VER_ATTR, CPLD_BASE_ADDR+i);
        if ((rc = file_read_hex(&cpld_minor_ver, sysfs)) != ONLP_STATUS_OK) {
            AIM_LOG_ERROR("file_read_hex failed, error=%d, %s", rc, sysfs);
            return ONLP_STATUS_E_INTERNAL;
        }
        snprintf(mb_cpld_ver_h[i], sizeof(mb_cpld_ver_h[i]), "%d.%02d", 
                    cpld_major_ver, cpld_minor_ver);
    }

    pi->cpld_versions = aim_fstrdup(            
        "\n"
        "[CPU CPLD] %s\n"
        "[MB CPLD1] %s\n"
        "[MB CPLD2] %s\n"
        "[MB CPLD3] %s\n"
        "[MB CPLD4] %s\n",
        cpu_cpld_ver_h,
        mb_cpld_ver_h[0],
        mb_cpld_ver_h[1],
        mb_cpld_ver_h[2],
        mb_cpld_ver_h[3]);
    
    //Get HW Build Version
    snprintf(sysfs, sizeof(sysfs), "%s/%s", 
                    LPC_MB_CPLD_SYFSFS_PATH, LPC_MB_SKU_ID_ATTR);
    if ((rc = file_read_hex(&sku_id, sysfs)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("unable to read sku id\n");
        return ONLP_STATUS_E_INTERNAL;
    }
    snprintf(sysfs, sizeof(sysfs), "%s/%s", 
                    LPC_MB_CPLD_SYFSFS_PATH, LPC_MB_HW_ID_ATTR);
    if ((rc = file_read_hex(&hw_id, sysfs)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("unable to read hw id\n");
        return ONLP_STATUS_E_INTERNAL;
    }
    snprintf(sysfs, sizeof(sysfs), "%s/%s", 
                    LPC_MB_CPLD_SYFSFS_PATH, LPC_MB_ID_TYPE_ATTR);
    if ((rc = file_read_hex(&id_type, sysfs)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("unable to read id type\n");
        return ONLP_STATUS_E_INTERNAL;
    }
    snprintf(sysfs, sizeof(sysfs), "%s/%s", 
                    LPC_MB_CPLD_SYFSFS_PATH, LPC_MB_BUILD_ID_ATTR);
    if ((rc = file_read_hex(&build_id, sysfs)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("unable to read build id\n");
        return ONLP_STATUS_E_INTERNAL;
    }
    snprintf(sysfs, sizeof(sysfs), "%s/%s", 
                    LPC_MB_CPLD_SYFSFS_PATH, LPC_MB_DEPH_ID_ATTR);
    if ((rc = file_read_hex(&deph_id, sysfs)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("unable to read deph id\n");
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
        "[ID TYPE] %d\n"
        "[DEPH ID] %d\n"
        "[BIOS] %s\n"
        "[BMC] %d.%d.%d\n",
        sku_id, hw_id, build_id, id_type, deph_id,
        bios_ver_h,
        atoi(bmc_ver[0]), atoi(bmc_ver[1]), atoi(bmc_ver[2]));

    return ONLP_STATUS_OK;
}

bool
onlp_sysi_bmc_en_get(void)
{
    //enable bmc by default
    return true;
}

int
parse_bmc_sdr_cmd(char *cmd_out, int cmd_out_size,
                  char *tokens[], int token_size, 
                  const char *sensor_id_str, int *idx)
{
    char cmd[BMC_CMD_SDR_SIZE];
    char delimiter[]=",";
    char delimiter_c = ','; 

    *idx=0;
    memset(cmd, 0, sizeof(cmd));
    memset(cmd_out, 0, cmd_out_size);

    snprintf(cmd, sizeof(cmd), CMD_BMC_SDR_GET, sensor_id_str);
    
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

int
bmc_thermal_info_get(onlp_thermal_info_t* info, int id)
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

int 
bmc_fan_info_get(onlp_fan_info_t* info, int id)
{
    int rv=0, rpm=0, percentage=0;
    int presence=0;
    float data=0;
    int sys_max_fan_speed = 16000;
    int psu_max_fan1_speed = 28500;
    int max_fan_speed = 0;
    
    //check presence for fantray 1-4
    if (id >= FAN_ID_FAN1 && id <= FAN_ID_FAN4) {
        rv = bmc_sensor_read(id + ID_FAN0_PSNT_L - 1, FAN_SENSOR, &data);
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
    rv = bmc_sensor_read(id + ID_FAN0_RPM - 1, FAN_SENSOR, &data);
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
    } else if (id >= FAN_ID_PSU0_FAN1 && id <= FAN_ID_PSU1_FAN1) {
        max_fan_speed = psu_max_fan1_speed;
        percentage = (info->rpm*100)/max_fan_speed; 
        info->percentage = percentage;
        info->status |= (rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;
    }
    
    return ONLP_STATUS_OK;
}

int
file_read_hex(int* value, const char* fmt, ...)
{
    int rv;
    va_list vargs;
    va_start(vargs, fmt);
    rv = file_vread_hex(value, fmt, vargs);
    va_end(vargs);
    return rv;
}

int
file_vread_hex(int* value, const char* fmt, va_list vargs)
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

int
sys_led_info_get(onlp_led_info_t* info, int id)
{
    int value, rc;
    int sysfs_index;
    
    int shift, led_val,led_val_color, led_val_blink, led_val_onoff;

    if (id == LED_ID_SYS_SYS) {        
        sysfs_index=1;
        shift = 4;    
    } else if (id == LED_ID_SYS_FAN) {
        sysfs_index=1;
        shift = 0;    
    } else if (id == LED_ID_SYS_PSU0) {
        sysfs_index=2;
        shift = 0;        
    } else if (id == LED_ID_SYS_PSU1) {
        sysfs_index=2;
        shift = 4;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }
    
    if ((rc = file_read_hex(&value, MB_CPLD1_SYSFS_PATH"/"MB_CPLD_SYS_LED_ATTR,
                                         sysfs_index)) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    }

    led_val = (value >> shift);
    led_val_color = (led_val >> 0) & 1;
    led_val_blink = (led_val >> 2) & 1;
    led_val_onoff = (led_val >> 3) & 1;

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

/*
 * This function check the I2C bus statuas by using the sysfs of cpld_id,
 * If the I2C Bus is stcuk, do the i2c mux reset.
 */
void
check_and_do_i2c_mux_reset(int port)
{
    char cmd_buf[256] = {0};
    char sysfs[128];
    int ret = 0;
    
    snprintf(sysfs, sizeof(sysfs), "%s/%s", 
                    MB_CPLD1_SYSFS_PATH, MB_CPLD_ID_ATTR);
    if(access(sysfs, F_OK) != -1 ) {

        snprintf(cmd_buf, sizeof(cmd_buf), "cat %s > /dev/null 2>&1", sysfs);
        ret = system(cmd_buf);

        if (ret != 0) {
            snprintf(sysfs, sizeof(sysfs), "%s/%s", 
                        LPC_MB_CPLD_SYFSFS_PATH, LPC_MUX_RESET_ATTR);
            if(access(sysfs, F_OK) != -1 ) {
                //debug
                //AIM_LOG_SYSLOG_WARN("I2C bus is stuck!! (port=%d)\r\n", port);
                snprintf(cmd_buf, sizeof(cmd_buf), "echo 0 > %s 2> /dev/null", sysfs);
                ret = system(cmd_buf);
                // debug
                //AIM_LOG_SYSLOG_WARN("Do I2C mux reset!! (ret=%d)\r\n", ret);
            }
        }
    }
}
