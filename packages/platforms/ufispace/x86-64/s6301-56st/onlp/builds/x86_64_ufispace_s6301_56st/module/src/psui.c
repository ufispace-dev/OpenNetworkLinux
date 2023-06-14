/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *        Copyright 2014, 2015 Big Switch Networks, Inc.
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
#include <onlp/platformi/psui.h>
#include "platform_lib.h"

#define PSU0_PRESENT_MASK       0b00000001
#define PSU1_PRESENT_MASK       0b00000010
#define PSU0_PWGOOD_MASK        0b00010000
#define PSU1_PWGOOD_MASK        0b00100000
#define PSU_STATUS_PRESENT      0
#define PSU_STATUS_PW_GOOD      1
#define PSU_FAN_RPM_MAX         13000
#define PSU_EEPROM_ADDR_0       0x50
#define PSU_EEPROM_ADDR_1       0x51

/* SYSFS */
#define SYSFS_PSU_STATUS        LPC_MB_CPLD_PATH"psu_status"

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_PSU(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

#define PSU_INFO(id, desc, fid, tid)            \
    {                                           \
        { ONLP_PSU_ID_CREATE(id), desc, POID_0, \
            {                                   \
                ONLP_FAN_ID_CREATE(fid),        \
                ONLP_THERMAL_ID_CREATE(tid),    \
            }                                   \
        }                                       \
    }

static onlp_psu_info_t psu_info[] =
{
    { }, /* Not used */
    PSU_INFO(ONLP_PSU_0, "PSU-0", ONLP_PSU_0_FAN, ONLP_THERMAL_PSU0),
    PSU_INFO(ONLP_PSU_1, "PSU-1", ONLP_PSU_1_FAN, ONLP_THERMAL_PSU1),
};

static psu_support_info_t psu_support_list[] =
{
    {"ASPOWER", "U1A-K10150-DRB-13", ONLP_PSU_TYPE_AC, ONLP_FAN_STATUS_F2B},
    {"ASPOWER", "U1A-K0150-B-13", ONLP_PSU_TYPE_AC, ONLP_FAN_STATUS_B2F},
    {"ASPOWER", "U1D-K0150-A-13", ONLP_PSU_TYPE_DC48, ONLP_FAN_STATUS_F2B},
    {"ASPOWER", "U1D-K0150-B-13", ONLP_PSU_TYPE_DC48, ONLP_FAN_STATUS_B2F},
};

/**
 * @brief get psu type
 * @param[out] psu_type: psu type(ONLP_PSU_TYPE_AC, ONLP_PSU_TYPE_DC48)
 * @param fru: psu fru info. we will use the fru informations to get psu type
 */
int get_psu_type(int *psu_type, psu_fru_t *fru)
{
    int i, max;

    *psu_type = ONLP_PSU_TYPE_INVALID;

    if(psu_type == NULL || fru == NULL) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if(!fru->ready) {
        return ONLP_STATUS_OK;
    }


    max = sizeof(psu_support_list)/sizeof(*psu_support_list);
    for (i = 0; i < max; ++i) {
        if ((strncmp(fru->vendor, psu_support_list[i].vendor, ONLP_CONFIG_INFO_STR_MAX)==0) &&
            (strncmp(fru->part_num, psu_support_list[i].part_num, ONLP_CONFIG_INFO_STR_MAX)==0)) {
            *psu_type = psu_support_list[i].type;
            break;
        }
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief get psu fan direction
 * @param[out] fan_dir: psu fan direction(ONLP_FAN_STATUS_F2B, ONLP_FAN_STATUS_B2F)
 * @param fru: psu fru info. we will use the fru informations to get psu type
 */
int get_psu_fan_dir(int *fan_dir, psu_fru_t *fru)
{
    int i, max;

    if(fan_dir == NULL || fru == NULL) {
        return ONLP_STATUS_E_INTERNAL;
    }

    *fan_dir = ONLP_FAN_STATUS_F2B;

    if(!fru->ready) {
        return ONLP_STATUS_OK;
    }

    max = sizeof(psu_support_list)/sizeof(*psu_support_list);
    for (i = 0; i < max; ++i) {
        if ((strncmp(fru->vendor, psu_support_list[i].vendor, ONLP_CONFIG_INFO_STR_MAX)==0) &&
            (strncmp(fru->part_num, psu_support_list[i].part_num, ONLP_CONFIG_INFO_STR_MAX)==0)) {
            *fan_dir = psu_support_list[i].fan_dir;
            break;
        }
    }

    return ONLP_STATUS_OK;
}

int psu_present_get(int *present, int local_id)
{
    int val, val_mask;

    if(present == NULL) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if(local_id == ONLP_PSU_0) {
        val_mask = PSU0_PRESENT_MASK;
    } else if(local_id == ONLP_PSU_1) {
        val_mask = PSU1_PRESENT_MASK;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    ONLP_TRY(file_read_hex(&val, SYSFS_PSU_STATUS));

    *present = ((mask_shift(val, val_mask)) == PSU_STATUS_PRESENT)? 1 : 0;

    return ONLP_STATUS_OK;
}

int psu_pwgood_get(int *pw_good, int local_id)
{
    int val, val_mask;

    if(pw_good == NULL) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if(local_id == ONLP_PSU_0) {
        val_mask = PSU0_PWGOOD_MASK;
    } else if(local_id == ONLP_PSU_1) {
        val_mask = PSU1_PWGOOD_MASK;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    ONLP_TRY(file_read_hex(&val, SYSFS_PSU_STATUS));

    *pw_good = ((mask_shift(val, val_mask)) == PSU_STATUS_PW_GOOD)? 1 : 0;

    return ONLP_STATUS_OK;
}

int psu_fru_get(psu_fru_t* fru, int i2c_bus, int i2c_addr)
{
    uint8_t data[256];
    int data_len, i;
    int hw_rev_id;
    int _rv;
    memset(data, 0, sizeof(data));

    fru->ready = false;

    // check hw revision
    hw_rev_id = get_hw_rev_id();
    if(hw_rev_id <= HW_REV_ALPHA) {
        strcpy(fru->vendor, "not supported");
        strcpy(fru->model, "not supported");
        strcpy(fru->part_num, "not supported");
        strcpy(fru->serial, "not supported");
        goto SKIP_EEPROM_PARSER;
    }

    // read psu eeprom
    _rv = onlp_file_read(data, sizeof(data), &data_len, SYS_FMT, i2c_bus, i2c_addr, SYS_EEPROM);
    if(ONLP_FAILURE(_rv)) {
        strcpy(fru->vendor, "not available");
        strcpy(fru->model, "not available");
        strcpy(fru->part_num, "not available");
        strcpy(fru->serial, "not available");
        goto SKIP_EEPROM_PARSER;
    }

    // check if dummy content
    if(data[0] == 0xff) {
        strcpy(fru->vendor, "not available");
        strcpy(fru->model, "not available");
        strcpy(fru->part_num, "not available");
        strcpy(fru->serial, "not available");
        goto SKIP_EEPROM_PARSER;
    }

    i = 11;

    /* Manufacturer Name */
    data_len = (data[i]&0x3f);
    i++;
    memcpy(fru->vendor, (char *) &(data[i]), data_len);
    i += data_len;

    /* Product Name */
    data_len = (data[i]&0x3f);
    i++;
    memcpy(fru->model, (char *) &(data[i]), data_len);
    i += data_len;

    /* Product part,model number */
    data_len = (data[i]&0x3f);
    i++;
    memcpy(fru->part_num, (char *) &(data[i]), data_len);
    i += data_len;

    /* Product Version */
    data_len = (data[i]&0x3f);
    i++;
    i += data_len;

    /* Product Serial Number */
    data_len = (data[i]&0x3f);
    i++;
    memcpy(fru->serial, (char *) &(data[i]), data_len);

    fru->ready = true;

SKIP_EEPROM_PARSER:
    return ONLP_STATUS_OK;
}

int psu_fan_info_get(onlp_fan_info_t* info, int local_id)
{
    int i2c_bus, i2c_addr, psu_id, psu_present, psu_pwgood;
    unsigned int tmp_fan_rpm, fan_rpm;
    int max_fan_speed = PSU_FAN_RPM_MAX;
    psu_fru_t fru;
    int eeprom_i2c_addr, fan_dir = ONLP_FAN_STATUS_F2B;
    memset(&fru, 0, sizeof(fru));

    if(local_id == ONLP_PSU_0_FAN) {
        i2c_bus = PSU_BUS_ID;
        i2c_addr = PSU_PMBUS_ADDR_0;
        eeprom_i2c_addr = PSU_EEPROM_ADDR_0;
        psu_id = ONLP_PSU_0;
    } else if(local_id == ONLP_PSU_1_FAN) {
        i2c_bus = PSU_BUS_ID;
        i2c_addr = PSU_PMBUS_ADDR_1;
        eeprom_i2c_addr = PSU_EEPROM_ADDR_1;
        psu_id = ONLP_PSU_1;
    } else {
        return ONLP_STATUS_E_INVALID;
    }

    // get fan fru
    strcpy(info->model, "not supported");
    strcpy(info->serial, "not supported");

    /* check psu status */
    ONLP_TRY(psu_present_get(&psu_present, psu_id));
    ONLP_TRY(psu_pwgood_get(&psu_pwgood, psu_id));

    if (!psu_present) {
        info->rpm = 0;
        info->status &= ~ONLP_FAN_STATUS_PRESENT;
        return ONLP_STATUS_OK;
    } else {
        info->status |= ONLP_FAN_STATUS_PRESENT;
    }

    if (!psu_pwgood) {
        info->rpm = 0;
        return ONLP_STATUS_OK;
    }

    tmp_fan_rpm = onlp_i2c_readw(i2c_bus, i2c_addr, PSU_PMBUS_FAN_RPM, ONLP_I2C_F_FORCE);

    fan_rpm = (unsigned int)tmp_fan_rpm;
    fan_rpm = (fan_rpm & 0x07FF) * (1 << ((fan_rpm >> 11) & 0x1F));
    info->rpm = (int)fan_rpm;
    info->percentage = (info->rpm*100)/max_fan_speed;

    // get fan dir
    /* Get psu fru from eeprom */
    ONLP_TRY(psu_fru_get(&fru, i2c_bus, eeprom_i2c_addr));
    ONLP_TRY(get_psu_fan_dir(&fan_dir, &fru));
    if(fan_dir == ONLP_FAN_STATUS_B2F) {
        /* B2F */
        info->status |= ONLP_FAN_STATUS_B2F;
        info->status &= ~ONLP_FAN_STATUS_F2B;
    } else {
        /* F2B */
        info->status |= ONLP_FAN_STATUS_F2B;
        info->status &= ~ONLP_FAN_STATUS_B2F;
    }


    return ONLP_STATUS_OK;
}

int psu_vout_get(onlp_psu_info_t* info, int i2c_bus, int i2c_addr)
{
    int v_value = 0;
    int n_value = 0;
    unsigned int temp = 0;
    char result[32];
    double dvalue;
    memset(result, 0, sizeof(result));

    n_value = onlp_i2c_readb(i2c_bus, i2c_addr, PSU_PMBUS_VOUT1, ONLP_I2C_F_FORCE);
    if(n_value < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    v_value = onlp_i2c_readw(i2c_bus, i2c_addr, PSU_PMBUS_VOUT2, ONLP_I2C_F_FORCE);
    if(v_value < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if(n_value & 0x10) {
        n_value = 0xF0 + (n_value & 0x0F);
        n_value = (~n_value) +1;
        temp = (unsigned int)(1<<n_value);
        if(temp)
            snprintf(result, sizeof(result), "%d.%04d", v_value/temp, ((v_value%temp)*10000)/temp);
    } else {
        snprintf(result, sizeof(result), "%d", (v_value*(1<<n_value)));
    }

    dvalue = atof((const char *)result);
    if(dvalue > 0.0) {
        info->caps |= ONLP_PSU_CAPS_VOUT;
        info->mvout = (int)(dvalue * 1000);
    }

    return ONLP_STATUS_OK;
}

int psu_vin_get(onlp_psu_info_t* info, int i2c_bus, int i2c_addr)
{
    int value;
    unsigned int y_value = 0;
    unsigned char n_value = 0;
    unsigned int temp = 0;
    char result[32];
    memset(result, 0, sizeof(result));
    double dvalue;

    value = onlp_i2c_readw(i2c_bus, i2c_addr, PSU_PMBUS_VIN, ONLP_I2C_F_FORCE);
    if(value < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    y_value = (value & 0x07FF);
    if((value & 0x8000)&&(y_value))
    {
        n_value = 0xF0 + (((value) >> 11) & 0x0F);
        n_value = (~n_value) +1;
        temp = (unsigned int)(1<<n_value);
        if(temp) {
            snprintf(result, sizeof(result), "%d.%04d", y_value/temp, ((y_value%temp)*10000)/temp);
        }
    } else {
        n_value = (((value) >> 11) & 0x0F);
        snprintf(result, sizeof(result), "%d", (y_value*(1<<n_value)));
    }

    dvalue = atof((const char *)result);
    if(dvalue > 0.0) {
        info->caps |= ONLP_PSU_CAPS_VIN;
        info->mvin = (int)(dvalue * 1000);
    }

    return ONLP_STATUS_OK;
}

int psu_iout_get(onlp_psu_info_t* info, int i2c_bus, int i2c_addr)
{
    int value;
    unsigned int y_value = 0;
    unsigned char n_value = 0;
    unsigned int temp = 0;
    char result[32];
    memset(result, 0, sizeof(result));
    double dvalue;

    value = onlp_i2c_readw(i2c_bus, i2c_addr, PSU_PMBUS_IOUT, ONLP_I2C_F_FORCE);
    if(value < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    y_value = (value & 0x07FF);
    if((value & 0x8000)&&(y_value))
    {
        n_value = 0xF0 + (((value) >> 11) & 0x0F);
        n_value = (~n_value) +1;
        temp = (unsigned int)(1<<n_value);
        if(temp) {
            snprintf(result, sizeof(result), "%d.%04d", y_value/temp, ((y_value%temp)*10000)/temp);
        }
    } else {
        n_value = (((value) >> 11) & 0x0F);
        snprintf(result, sizeof(result), "%d", (y_value*(1<<n_value)));
    }

    dvalue = atof((const char *)result);
    if(dvalue > 0.0) {
        info->caps |= ONLP_PSU_CAPS_IOUT;
        info->miout = (int)(dvalue * 1000);
    }

    return ONLP_STATUS_OK;
}

int psu_iin_get(onlp_psu_info_t* info, int i2c_bus, int i2c_addr)
{
    int value;
    unsigned int y_value = 0;
    unsigned char n_value = 0;
    unsigned int temp = 0;
    char result[32];
    memset(result, 0, sizeof(result));
    double dvalue;

    value = onlp_i2c_readw(i2c_bus, i2c_addr, PSU_PMBUS_IIN, ONLP_I2C_F_FORCE);
    if(value < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    y_value = (value & 0x07FF);
    if((value & 0x8000)&&(y_value))
    {
        n_value = 0xF0 + (((value) >> 11) & 0x0F);
        n_value = (~n_value) +1;
        temp = (unsigned int)(1<<n_value);
        if(temp) {
            snprintf(result, sizeof(result), "%d.%04d", y_value/temp, ((y_value%temp)*10000)/temp);
        }
    } else {
        n_value = (((value) >> 11) & 0x0F);
        snprintf(result, sizeof(result), "%d", (y_value*(1<<n_value)));
    }

    dvalue = atof((const char *)result);
    if(dvalue > 0.0) {
        info->caps |= ONLP_PSU_CAPS_IIN;
        info->miin = (int)(dvalue * 1000);
    }

    return ONLP_STATUS_OK;
}

int psu_pout_get(onlp_psu_info_t* info, int i2c_bus, int i2c_addr)
{
    int value;
    unsigned int y_value = 0;
    unsigned char n_value = 0;
    unsigned int temp = 0;
    char result[32];
    memset(result, 0, sizeof(result));
    double dvalue;

    value = onlp_i2c_readw(i2c_bus, i2c_addr, PSU_PMBUS_POUT, ONLP_I2C_F_FORCE);
    if(value < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    y_value = (value & 0x07FF);
    if((value & 0x8000)&&(y_value))
    {
        n_value = 0xF0 + (((value) >> 11) & 0x0F);
        n_value = (~n_value) +1;
        temp = (unsigned int)(1<<n_value);
        if(temp) {
            snprintf(result, sizeof(result), "%d.%04d", y_value/temp, ((y_value%temp)*10000)/temp);
        }
    } else {
        n_value = (((value) >> 11) & 0x0F);
        snprintf(result, sizeof(result), "%d", (y_value*(1<<n_value)));
    }

    dvalue = atof((const char *)result);
    if(dvalue > 0.0) {
        info->caps |= ONLP_PSU_CAPS_POUT;
        info->mpout = (int)(dvalue * 1000);
    }

    return ONLP_STATUS_OK;
}

int psu_pin_get(onlp_psu_info_t* info, int i2c_bus, int i2c_addr)
{
    int value;
    unsigned int y_value = 0;
    unsigned char n_value = 0;
    unsigned int temp = 0;
    char result[32];
    memset(result, 0, sizeof(result));
    double dvalue;

    value = onlp_i2c_readw(i2c_bus, i2c_addr, PSU_PMBUS_PIN, ONLP_I2C_F_FORCE);
    if(value < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    y_value = (value & 0x07FF);
    if((value & 0x8000)&&(y_value))
    {
        n_value = 0xF0 + (((value) >> 11) & 0x0F);
        n_value = (~n_value) +1;
        temp = (unsigned int)(1<<n_value);
        if(temp) {
            snprintf(result, sizeof(result), "%d.%04d", y_value/temp, ((y_value%temp)*10000)/temp);
        }
    } else {
        n_value = (((value) >> 11) & 0x0F);
        snprintf(result, sizeof(result), "%d", (y_value*(1<<n_value)));
    }

    dvalue = atof((const char *)result);
    if(dvalue > 0.0) {
        info->caps |= ONLP_PSU_CAPS_PIN;
        info->mpin = (int)(dvalue * 1000);
    }

    return ONLP_STATUS_OK;
}

int psu_status_info_get(int local_id, onlp_psu_info_t *info)
{
    int i2c_bus, pmbus_i2c_addr, eeprom_i2c_addr;
    psu_fru_t fru;
    int psu_type;
    memset(&fru, 0, sizeof(fru));

    if(local_id == ONLP_PSU_0) {
        i2c_bus = PSU_BUS_ID;
        pmbus_i2c_addr = PSU_PMBUS_ADDR_0;
        eeprom_i2c_addr = PSU_EEPROM_ADDR_0;
    } else if (local_id == ONLP_PSU_1) {
        i2c_bus = PSU_BUS_ID;
        pmbus_i2c_addr = PSU_PMBUS_ADDR_1;
        eeprom_i2c_addr = PSU_EEPROM_ADDR_1;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    /* Get psu fru from eeprom */
    ONLP_TRY(psu_fru_get(&fru, i2c_bus, eeprom_i2c_addr));
    strcpy(info->model, fru.part_num);
    strcpy(info->serial, fru.serial);
    /* Get PSU type */
    ONLP_TRY(get_psu_type(&psu_type, &fru));
    if(psu_type == ONLP_PSU_TYPE_AC) {
        info->caps |= ONLP_PSU_CAPS_AC;
        info->caps &= ~ONLP_PSU_CAPS_DC48;
    } else if(psu_type == ONLP_PSU_TYPE_DC48){
        info->caps &= ~ONLP_PSU_CAPS_AC;
        info->caps |= ONLP_PSU_CAPS_DC48;
    }

    if((info->status & ONLP_PSU_STATUS_UNPLUGGED)) {
        return ONLP_STATUS_OK;
    }

    if((info->status & ONLP_PSU_STATUS_FAILED)) {
        return ONLP_STATUS_OK;
    }

    /* Get psu iout status */
    ONLP_TRY(psu_iout_get(info, i2c_bus, pmbus_i2c_addr));

    /* Get psu iin status */
    ONLP_TRY(psu_iin_get(info, i2c_bus, pmbus_i2c_addr));

    /* Get psu pout status */
    ONLP_TRY(psu_pout_get(info, i2c_bus, pmbus_i2c_addr));

    /* Get psu pin status */
    ONLP_TRY(psu_pin_get(info, i2c_bus, pmbus_i2c_addr));

    /* Get psu vout status */
    ONLP_TRY(psu_vout_get(info, i2c_bus, pmbus_i2c_addr));

    /* Get psu vin status */
    ONLP_TRY(psu_vin_get(info, i2c_bus, pmbus_i2c_addr));

    return ONLP_STATUS_OK;
}

/**
 * @brief Initialize the PSU subsystem.
 */
int onlp_psui_init(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the information structure for the given PSU
 * @param id The PSU OID
 * @param rv [out] Receives the PSU information.
 */
int onlp_psui_info_get(onlp_oid_t id, onlp_psu_info_t* rv)
{
    int local_id;
    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);
    memset(rv, 0, sizeof(onlp_psu_info_t));

    *rv = psu_info[local_id];
    /* update status */
    ONLP_TRY(onlp_psui_status_get(id, &rv->status));

    if((rv->status & ONLP_PSU_STATUS_PRESENT) == 0) {
        return ONLP_STATUS_OK;
    }


    switch(local_id) {
        case ONLP_PSU_0:
        case ONLP_PSU_1:
            return psu_status_info_get(local_id, rv);
            break;
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
            break;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the PSU's operational status.
 * @param id The PSU OID.
 * @param rv [out] Receives the operational status.
 */
int onlp_psui_status_get(onlp_oid_t id, uint32_t* rv)
{
    int local_id;
    int pw_present, pw_good;
    VALIDATE(id);

     /* Get power present status */
    local_id = ONLP_OID_ID_GET(id);
    ONLP_TRY(psu_present_get(&pw_present, local_id));

    if(!pw_present) {
        *rv &= ~ONLP_PSU_STATUS_PRESENT;
        *rv |=  ONLP_PSU_STATUS_UNPLUGGED;
    } else {
        *rv |= ONLP_PSU_STATUS_PRESENT;
    }

    /* Get power good status */
    ONLP_TRY(psu_pwgood_get(&pw_good, local_id));

    if(!pw_good) {
        *rv |= ONLP_PSU_STATUS_FAILED;
        *rv |=  ONLP_PSU_STATUS_UNPLUGGED;
    } else {
        *rv &= ~ONLP_PSU_STATUS_FAILED;
        *rv &= ~ONLP_PSU_STATUS_UNPLUGGED;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the PSU's oid header.
 * @param id The PSU OID.
 * @param rv [out] Receives the header.
 */
int onlp_psui_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* rv)
{
    int local_id;
    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);
    if(local_id > ONLP_PSU_1) {
        return ONLP_STATUS_E_INVALID;
    } else {
        *rv = psu_info[local_id].hdr;
    }
    return ONLP_STATUS_OK;
}

/**
 * @brief Generic PSU ioctl
 * @param id The PSU OID
 * @param vargs The variable argument list for the ioctl call.
 */
int onlp_psui_ioctl(onlp_oid_t pid, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}


