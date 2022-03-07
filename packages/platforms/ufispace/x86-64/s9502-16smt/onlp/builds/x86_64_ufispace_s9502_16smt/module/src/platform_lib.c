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

int get_shift(int mask)
{
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

int psu_thermal_get(onlp_thermal_info_t* info, int thermal_id)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int psu_fan_info_get(onlp_fan_info_t* info, int id)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int psu_vin_get(onlp_psu_info_t* info, int id)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int psu_vout_get(onlp_psu_info_t* info, int id)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int psu_stbvout_get(int* stbmvout, int id)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int psu_iin_get(onlp_psu_info_t* info, int id)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int psu_iout_get(onlp_psu_info_t* info, int id)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int psu_stbiout_get(int* stbmiout, int id)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int psu_pout_get(onlp_psu_info_t* info, int i2c_bus)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int psu_pin_get(onlp_psu_info_t* info, int i2c_bus)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int psu_eeprom_get(onlp_psu_info_t* info, int id)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int psu_fru_get(onlp_psu_info_t* info, int id)
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
        gpio_num = 399 - port;
    } else if (port>=8 && port<16) {
        gpio_num = 375 - (port-8);
    }

    snprintf(sysfs, sizeof(sysfs), SYS_GPIO_VAL, gpio_num);

    if ((rc = onlp_file_read_int(val, sysfs)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("sfp_present_get() failed, error=%d, sysfs=%s", rc, sysfs);
        return ONLP_STATUS_E_INTERNAL;
    }

    *val = (*val) ? 0 : 1;

    return ONLP_STATUS_OK;
}

int system_led_set(onlp_led_mode_t mode)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int fan_led_set(onlp_led_mode_t mode)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int psu1_led_set(onlp_led_mode_t mode)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int psu2_led_set(onlp_led_mode_t mode)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int read_ioport(int addr, int *reg_val)
{
    int ret;

    /*set r/w permission of all 65536 ports*/
    ret = iopl(3);
    if(ret < 0){
        AIM_LOG_ERROR("read_ioport() iopl enable error %d\n", ret);
        return ONLP_STATUS_E_INTERNAL;
    }
    *reg_val = inb(addr);

    /*set r/w permission of  all 65536 ports*/
    ret = iopl(0);
    if(ret < 0) {
        AIM_LOG_ERROR("read_ioport(), iopl disable error %d\n", ret);
        return ONLP_STATUS_E_INTERNAL;
    }
    return ONLP_STATUS_OK;
}

int write_ioport(int addr, int reg_val)
{
    int ret;

    /*enable io port*/
    ret = iopl(3);
    if(ret < 0){
        AIM_LOG_ERROR("read_ioport() iopl enable error %d\n", ret);
        return ONLP_STATUS_E_INTERNAL;
    }
    outb(addr, reg_val);

    /*disable io port*/
    ret = iopl(0);
    if(ret < 0){
        AIM_LOG_ERROR("read_ioport(), iopl disable error %d\n", ret);
        return ONLP_STATUS_E_INTERNAL;
    }
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
    //int ucd_len=0; //TODO
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

    //Get BMC version //TODO
    /*if (exec_cmd(CMD_BMC_VER_1, bmc_out1, sizeof(bmc_out1)) < 0 ||
        exec_cmd(CMD_BMC_VER_2, bmc_out2, sizeof(bmc_out2)) < 0 ||
        exec_cmd(CMD_BMC_VER_3, bmc_out3, sizeof(bmc_out3))) {
            AIM_LOG_ERROR("unable to read BMC version\n");
            return ONLP_STATUS_E_INTERNAL;
    }*/

    //get UCD version //TODO
    /*if (exec_cmd(CMD_UCD_VER, ucd_out, sizeof(ucd_out)) < 0) {
            AIM_LOG_ERROR("unable to read UCD version\n");
            return ONLP_STATUS_E_INTERNAL;
    }*/

    //parse UCD version and date //TODO
    //ucd_len = get_ipmitool_len(ucd_out);
    /*
    if (ucd_len > ucd_date_len) {
        parse_ucd_out(ucd_out, ucd_ver, 0, ucd_len-ucd_date_len);
        parse_ucd_out(ucd_out, ucd_date, ucd_len-ucd_date_len, ucd_date_len);
    } else {
        parse_ucd_out(ucd_out, ucd_ver, 0, ucd_len);
    }
    */
    //parse_ucd_out(ucd_out, ucd_ver, 0, ucd_len); //TODO

    pi->other_versions = aim_fstrdup(
        "\n"
        "[HW     ] %d\n"
        "[BUILD  ] %d\n"
        "[BIOS   ] %s\n",
        //"[BMC    ] %d.%d.%d\n"
        //"[UCD    ] %s %s\n",
        //"[UCD    ] %s\n",
        mb_cpld1_hw_rev,
        mb_cpld1_build_rev,
        bios_out
        //atoi(bmc_out1), atoi(bmc_out2), atoi(bmc_out3),
        //ucd_ver, ucd_date);
        //ucd_ver
        );

    return ONLP_STATUS_OK;
}

bool onlp_sysi_bmc_en_get(void)
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

int sys_led_info_get(onlp_led_info_t* info, int id)
{
    int rc;
    int reg, reg_val_ctrl, reg_val_blnk;
    int ctrl_offset = 0;
    int mask_onoff, mask_color, mask_blink;
    int led_val_color, led_val_blink, led_val_onoff;

    if (id == LED_ID_SYS_SYNC) {
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
