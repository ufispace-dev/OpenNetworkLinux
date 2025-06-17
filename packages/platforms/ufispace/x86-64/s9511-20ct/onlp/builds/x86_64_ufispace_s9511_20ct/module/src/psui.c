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
 * Power Supply Management Implementation.
 *
 ***********************************************************/
#include <onlp/platformi/psui.h>
#include "platform_lib.h"
#include <math.h>

#define PSU_STATUS_PWR_FAIL         (0)
#define PSU_STATUS_PWR_GD           (1)

static onlp_psu_info_t ac_psu_info[] =
{
    { }, /* Not used */
    {
        .hdr = {
            .id = ONLP_PSU_ID_CREATE(ONLP_PSU_0),
            .description = "PSU 0",
            .poid = POID_0,
            .coids = {
                ONLP_FAN_ID_CREATE(ONLP_PSU_0_FAN),
                ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU_0)
            }
        },
        .model = COMM_STR_NOT_SUPPORTED,
        .serial = COMM_STR_NOT_SUPPORTED,
    },
    {
        .hdr = {
            .id = ONLP_PSU_ID_CREATE(ONLP_PSU_1),
            .description = "PSU 1",
            .poid = POID_0,
            .coids = {
                ONLP_FAN_ID_CREATE(ONLP_PSU_1_FAN),
                ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU_1)
            }
        },
        .model = COMM_STR_NOT_SUPPORTED,
        .serial = COMM_STR_NOT_SUPPORTED,
    },
};

static onlp_psu_info_t dual_dc_psu_info[] =
{
    { }, /* Not used */
    {
        .hdr = {
            .id = ONLP_PSU_ID_CREATE(ONLP_DUAL_PSU_0_PLUG_0),
            .description = "PSU 0 PLUG 0",
            .poid = POID_0,
            .coids = {
                ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU_0)
            }
        },
        .model = COMM_STR_NOT_SUPPORTED,
        .serial = COMM_STR_NOT_SUPPORTED,
    },
    {
        .hdr = {
            .id = ONLP_PSU_ID_CREATE(ONLP_DUAL_PSU_0_PLUG_1),
            .description = "PSU 0 PLUG 1",
            .poid = POID_0,
            .coids = {
                ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU_0)
            }
        },
        .model = COMM_STR_NOT_SUPPORTED,
        .serial = COMM_STR_NOT_SUPPORTED,
    },
};

typedef struct
{
    int cpld_psu_type;
    int cpld_abs;
    int cpld_pwrgd;
} psu_attr_t;

typedef enum cpld_attr_idx_e {
    PSU0_PRESENT = 0,
    PSU1_PRESENT,
    PSU0_VIN_PWOK,
    PSU1_VIN_PWOK,
    PSU0_PWOK,
    PSU1_PWOK,
    PSU_TYPE,
    NA,
} cpld_attr_idx_t;

static const psu_attr_t ac_psu_attr[] = {
    /*                         cpld_psu_type             cpld_abs                     pwrgd          */
    [ONLP_PSU_0]             = {PSU_TYPE                 ,PSU0_PRESENT                ,PSU0_PWOK     },
    [ONLP_PSU_1]             = {PSU_TYPE                 ,PSU1_PRESENT                ,PSU1_PWOK     },
};

static const psu_attr_t dc_psu_attr[] = {
    /*                         cpld_psu_type             cpld_abs                     pwrgd          */
    [ONLP_DUAL_PSU_0_PLUG_0] = {PSU_TYPE                 ,NA                          ,PSU0_PWOK     },
    [ONLP_DUAL_PSU_0_PLUG_1] = {PSU_TYPE                 ,NA                          ,PSU0_PWOK     },
};

/* Ufispace Specific Defined functions */
static int update_psui_info(int psu_type, int local_id, onlp_psu_info_t *info);
static int cached_psu_type = -1;
static bool psu_type_cached = false;
int get_psu_type(int *psu_type);
static int get_psu_sysfs(cpld_attr_idx_t idx, char** str);
static int get_psu_local_id(int psu_type, int id, int *local_id);
int get_psu_present_status(int psu_type, int psu_id, int *pw_present);
int get_psu_pwgood_status(int psu_type, int psu_id, int *pw_good);
static int get_pmbus_psu_vin(int psu_type, int local_id, onlp_psu_info_t* info, int i2c_bus);
static int get_pmbus_psu_vout(int psu_type, int local_id, onlp_psu_info_t* info, int i2c_bus);
static int get_pmbus_psu_iin(int psu_type, int local_id, onlp_psu_info_t* info, int i2c_bus);
static int get_pmbus_psu_iout(int psu_type, int local_id, onlp_psu_info_t* info, int i2c_bus);
static int get_pmbus_psu_pin(int psu_type, int local_id, onlp_psu_info_t* info, int i2c_bus);
static int get_pmbus_psu_pout(int psu_type, int local_id, onlp_psu_info_t* info, int i2c_bus);
static int get_pmbus_psu_eeprom(int psu_type, onlp_psu_info_t* info, int local_id);
//static signed int _two_complement(signed int num, unsigned int bit);

/******************************************************************************************************************
**                                                                                                               **
**                                                ONLP Standard APIs                                             **
**                                                                                                               **
*******************************************************************************************************************/
/**
 * @brief Initialize the PSU subsystem.
 */
int onlp_psui_init(void)
{
    init_lock();
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the information structure for the given PSU
 * @param id The PSU OID
 * @param rv [out] Receives the PSU information.
 */
int onlp_psui_info_get(onlp_oid_t id, onlp_psu_info_t* rv)
{
    int psu_type = ONLP_PSU_TYPE_INVALID;
    int local_id;

    /* Clean memory */
    memset(rv, 0, sizeof(onlp_psu_info_t));

    /* Check PSU type */
    ONLP_TRY(get_psu_type(&psu_type));

    ONLP_TRY(get_psu_local_id(psu_type, id, &local_id));
    ONLP_TRY(update_psui_info(psu_type, local_id, rv));

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the PSU's operational status.
 * @param id The PSU OID.
 * @param rv [out] Receives the operational status.
 */
int onlp_psui_status_get(onlp_oid_t id, uint32_t* rv)
{
    int psu_type = ONLP_PSU_TYPE_INVALID;
    int local_id;
    onlp_psu_info_t info ={0};

    /* Check PSU type */
    ONLP_TRY(get_psu_type(&psu_type));

    ONLP_TRY(get_psu_local_id(psu_type, id, &local_id));
    ONLP_TRY(update_psui_info(psu_type, local_id, &info));
    *rv = info.status;

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the PSU's oid header.
 * @param id The PSU OID.
 * @param rv [out] Receives the header.
 */
int onlp_psui_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* rv)
{
    int psu_type = ONLP_PSU_TYPE_INVALID;
    int local_id;

    /* Check PSU type */
    ONLP_TRY(get_psu_type(&psu_type));

    ONLP_TRY(get_psu_local_id(psu_type, id, &local_id));
    *rv = (psu_type == 0) ? dual_dc_psu_info[local_id].hdr : ac_psu_info[local_id].hdr;

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

/******************************************************************************************************************
**                                                                                                               **
**                                           Upispace Specific Defined APIs                                      **
**                                                                                                               **
*******************************************************************************************************************/
static int update_psui_info(int psu_type, int local_id, onlp_psu_info_t *info)
{
    int pw_present = -1, pw_good = -1;
    int psu_id = -1;
    int rc, i2c_bus;

    if (psu_type == ONLP_PSU_TYPE_AC) {
        *info = ac_psu_info[local_id];

        /* Add PSU Type to ONL Capability */
        info->caps |= ONLP_PSU_CAPS_AC;
    } else if (psu_type == ONLP_PSU_TYPE_DC48) {
        *info = dual_dc_psu_info[local_id];

        /* Add PSU Type to ONL Capability */
        info->caps |= ONLP_PSU_CAPS_DC48;
    } else {
        AIM_LOG_ERROR("Invalid PSU Type(%d)\r\n", psu_type);
        return ONLP_STATUS_E_INVALID;
    }

    if ((psu_type == ONLP_PSU_TYPE_AC) && (local_id == ONLP_PSU_0)) {
        i2c_bus = I2C_BUS_PSU0;
        //psu_id = 1;
        psu_id = ONLP_PSU_0;
    } else if ((psu_type == ONLP_PSU_TYPE_AC) && (local_id == ONLP_PSU_1)) {
        i2c_bus = I2C_BUS_PSU1;
        //psu_id = 2;
        psu_id = ONLP_PSU_1;
    } else if (psu_type == ONLP_PSU_TYPE_DC48) {
        i2c_bus = I2C_BUS_PSU0;
        //psu_id = 1;
        psu_id = ONLP_DUAL_PSU_0_PLUG_0;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    /* Get PSU present status*/
    ONLP_TRY(get_psu_present_status(psu_type, psu_id, &pw_present));

    if (pw_present == PSU_STATUS_ABS) {
        info->status &= ~ONLP_PSU_STATUS_PRESENT;
        info->status |=  ONLP_PSU_STATUS_UNPLUGGED;
    } else if (pw_present == PSU_STATUS_PRES ) {
        info->status |= ONLP_PSU_STATUS_PRESENT;
        info->status &= ~ONLP_PSU_STATUS_UNPLUGGED;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    if(info->status & ONLP_PSU_STATUS_PRESENT) {
        /* Get power good status */
        ONLP_TRY(get_psu_pwgood_status(psu_type, psu_id, &pw_good));

        if (pw_good != PSU_STATUS_PWR_GD) {
            info->status |= ONLP_PSU_STATUS_FAILED;
        } else {
            info->status &= ~ONLP_PSU_STATUS_FAILED;
        }

        /* Get power vin status */
        if((rc = get_pmbus_psu_vin(psu_type, local_id, info, i2c_bus)) != ONLP_STATUS_OK) {
            return ONLP_STATUS_E_INTERNAL;
        }

        /* Get power vout status */
        if((rc = get_pmbus_psu_vout(psu_type, local_id, info, i2c_bus)) != ONLP_STATUS_OK) {
            return ONLP_STATUS_E_INTERNAL;
        }

        /* Get power iin status */
        if((rc = get_pmbus_psu_iin(psu_type, local_id, info, i2c_bus)) != ONLP_STATUS_OK) {
            return ONLP_STATUS_E_INTERNAL;
        }

        /* Get power iout status */
        if((rc = get_pmbus_psu_iout(psu_type, local_id, info, i2c_bus)) != ONLP_STATUS_OK) {
            return ONLP_STATUS_E_INTERNAL;
        }

        /* Get power pin status */
        if((rc = get_pmbus_psu_pin(psu_type, local_id, info, i2c_bus)) != ONLP_STATUS_OK) {
            return ONLP_STATUS_E_INTERNAL;
        }

        /* Get power pout status */
        if((rc = get_pmbus_psu_pout(psu_type, local_id, info, i2c_bus)) != ONLP_STATUS_OK) {
            return ONLP_STATUS_E_INTERNAL;
        }

        /* Get power eeprom status */
        if((rc = get_pmbus_psu_eeprom(psu_type, info, local_id)) != ONLP_STATUS_OK) {
            return ONLP_STATUS_E_INTERNAL;
        }
    } else {
        info->status &= ~ONLP_PSU_STATUS_PRESENT;
        info->status |=  ONLP_PSU_STATUS_UNPLUGGED;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief get psu type
 * @param local_id: psu id
 * @param[out] psu_type: psu type(ONLP_PSU_TYPE_AC, ONLP_PSU_TYPE_DC48)
 * @param fru_in: input fru node. we will use the input node informations to get psu type
 */
int get_psu_type(int *psu_type)
{
    if(psu_type == NULL) {
        return ONLP_STATUS_E_INTERNAL;
    }

    /* If PSU type has already been cached, return it directly */
    if(psu_type_cached) {
        *psu_type = cached_psu_type;
        return ONLP_STATUS_OK;
    }

    /* Otherwise, check psu type from /tmp/psu_type */
    if (onlp_file_read_int(psu_type, PSU_TYPE_FILE_PATH) < 0) {
        AIM_LOG_ERROR("Failed to get PSU Type from %s\r\n", PSU_TYPE_FILE_PATH);
        return ONLP_STATUS_E_INTERNAL;
    }

    /* Update the output parameter and set the flag */
    cached_psu_type = *psu_type;
    psu_type_cached = true;

    return ONLP_STATUS_OK;
}

static int get_psu_sysfs(cpld_attr_idx_t idx, char** str)
{
    if(str == NULL)
        return ONLP_STATUS_E_PARAM;

    *str = (char *)malloc(256);
    if (*str == NULL)
        return ONLP_STATUS_E_PARAM;

    switch(idx) {
        case PSU0_PRESENT:
            snprintf(*str, 256, SYSFS_CPLD1 "psu0_present");
            break;
        case PSU1_PRESENT:
            snprintf(*str, 256, SYSFS_CPLD1 "psu1_present");
            break;
        case PSU0_VIN_PWOK:
            snprintf(*str, 256, SYSFS_CPLD1 "psu0_vin_pwok");
            break;
        case PSU1_VIN_PWOK:
            snprintf(*str, 256, SYSFS_CPLD1 "psu1_vin_pwok");
            break;
        case PSU0_PWOK:
            snprintf(*str, 256, SYSFS_CPLD1 "psu0_pwok");
            break;
        case PSU1_PWOK:
            snprintf(*str, 256, SYSFS_CPLD1 "psu1_pwok");
            break;
        case PSU_TYPE:
            snprintf(*str, 256, SYSFS_CPLD1 "psu_type");
            break;
        default:
            free(str);
            AIM_LOG_ERROR("Get psu_sysfs wrong idx = %d\r\n", idx);
            *str = NULL;
            return ONLP_STATUS_E_PARAM;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get and check psu local ID
 * @param id [in] OID
 * @param local_id [out] The psu local id
 */
static int get_psu_local_id(int psu_type, int id, int *local_id)
{
    int tmp_id;

    if(local_id == NULL) {
        return ONLP_STATUS_E_PARAM;
    }

    if(!ONLP_OID_IS_PSU(id)) {
        return ONLP_STATUS_E_INVALID;
    }

    tmp_id = ONLP_OID_ID_GET(id);
    if(psu_type == ONLP_PSU_TYPE_AC) {
        switch (tmp_id) {
            case ONLP_PSU_0:
            case ONLP_PSU_1:
                *local_id = tmp_id;
                return ONLP_STATUS_OK;
            default:
                return ONLP_STATUS_E_INVALID;
        }
    } else if(psu_type == ONLP_PSU_TYPE_DC48) {
            switch (tmp_id) {
                case ONLP_DUAL_PSU_0_PLUG_0:
                case ONLP_DUAL_PSU_0_PLUG_1:
                    *local_id = tmp_id;
                    return ONLP_STATUS_OK;
                default:
                    return ONLP_STATUS_E_INVALID;
            }
    } else {
        return ONLP_STATUS_E_INVALID;
    }
}

int get_psu_present_status(int psu_type, int psu_id, int *pw_present)
{
    char *sysfs = NULL;
    int status;

    if (pw_present == NULL) {
        AIM_LOG_ERROR("pw_present is NULL)\r\n");
        return ONLP_STATUS_E_INTERNAL;
    }

    if (psu_type == ONLP_PSU_TYPE_AC) {
        /* Get PSU Present Status */
        ONLP_TRY(get_psu_sysfs(ac_psu_attr[psu_id].cpld_abs, &sysfs));
        ONLP_TRY(read_file_hex(&status, sysfs));

        free(sysfs);

        *pw_present = (status == 1)? PSU_STATUS_ABS : PSU_STATUS_PRES;
    } else if (psu_type == ONLP_PSU_TYPE_DC48) {
        /* Get PSU Present Status */
        *pw_present = PSU_STATUS_PRES;
    }

    return ONLP_STATUS_OK;
}

int get_psu_pwgood_status(int psu_type, int psu_id, int *pw_good)
{
    char *sysfs = NULL;
    int status;

    if(pw_good == NULL) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if(psu_type == ONLP_PSU_TYPE_AC) {
        ONLP_TRY(get_psu_sysfs(ac_psu_attr[psu_id].cpld_pwrgd, &sysfs));
    } else if(psu_type == ONLP_PSU_TYPE_DC48) {
        ONLP_TRY(get_psu_sysfs(dc_psu_attr[psu_id].cpld_pwrgd, &sysfs));
    } else {
        AIM_LOG_ERROR("Unknown PSU Type(%d)\r\n", psu_type);
    }

    ONLP_TRY(read_file_hex(&status, sysfs));
    *pw_good = (status == 1)? PSU_STATUS_PWR_GD : PSU_STATUS_PWR_FAIL;

    return ONLP_STATUS_OK;
}

static int get_pmbus_psu_vin(int psu_type, int local_id, onlp_psu_info_t* info, int i2c_bus)
{
    int value;
    unsigned int y_value = 0;
    unsigned char n_value = 0;
    unsigned int temp = 0;
    char result[32];
    memset(result, 0, sizeof(result));
    double dvalue;
    int ret = 0;

    if ((psu_type == ONLP_PSU_TYPE_AC && local_id == ONLP_PSU_0)) {
        value = onlp_i2c_readw(i2c_bus, PSU0_ADDRESS, PMBUS_READ_VIN, ONLP_I2C_F_FORCE);
    } else if ((psu_type == ONLP_PSU_TYPE_AC && local_id == ONLP_PSU_1)) {
        value = onlp_i2c_readw(i2c_bus, PSU1_ADDRESS, PMBUS_READ_VIN, ONLP_I2C_F_FORCE);
    } else if (psu_type == ONLP_PSU_TYPE_DC48 && (local_id == ONLP_DUAL_PSU_0_PLUG_0)) {
        /* Switch to page 00h */
        if ((ret = onlp_i2c_writeb(i2c_bus, PSU0_ADDRESS, PMBUS_PAGE_COMMAND, DC_PSU_MAIN_PW, ONLP_I2C_F_FORCE | ONLP_I2C_F_PEC)) < 0) {
            return ONLP_STATUS_E_INTERNAL;
        }

        value = onlp_i2c_readw(i2c_bus, PSU0_ADDRESS, PMBUS_READ_VIN, ONLP_I2C_F_FORCE);
    } else if (psu_type == ONLP_PSU_TYPE_DC48 && (local_id == ONLP_DUAL_PSU_0_PLUG_1)) {
        /* Switch to page 20h */
        if ((ret = onlp_i2c_writeb(i2c_bus, PSU0_ADDRESS, PMBUS_PAGE_COMMAND, DC_PSU_SECOND_PW, ONLP_I2C_F_FORCE | ONLP_I2C_F_PEC)) < 0) {
            return ONLP_STATUS_E_INTERNAL;
        }

        value = onlp_i2c_readw(i2c_bus, PSU0_ADDRESS, PMBUS_READ_VIN, ONLP_I2C_F_FORCE);
    } else {
        return ONLP_STATUS_E_INVALID;
    }

    if (value < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    y_value = (value & 0x07FF);
    if ((value & 0x8000)&&(y_value)) {
        n_value = 0xF0 + (((value) >> 11) & 0x0F);
        n_value = (~n_value) +1;
        temp = (unsigned int)(1<<n_value);
        if (temp) {
            snprintf(result, sizeof(result), "%d.%04d", y_value/temp, ((y_value%temp)*10000)/temp);
        }
    } else {
        n_value = (((value) >> 11) & 0x0F);
        snprintf(result, sizeof(result), "%d", (y_value*(1<<n_value)));
    }

    dvalue = atof((const char *)result);
    if (dvalue > 0.0) {
        info->caps |= ONLP_PSU_CAPS_VIN;
        info->mvin = (int)(dvalue * 1000);
    } else {
        info->caps &= ~ONLP_PSU_CAPS_VIN;
        info->mvin = 0;
    }

    return ONLP_STATUS_OK;
}

static int get_pmbus_psu_vout(int psu_type, int local_id, onlp_psu_info_t* info, int i2c_bus)
{
    int v_value = 0;
    int n_value = 0;
    unsigned int temp = 0;
    char result[32];
    const size_t result_size = sizeof(result);
    double dvalue;
    memset(result, 0, result_size);
    int ret = 0;

    /* Read byte */
    if(psu_type == ONLP_PSU_TYPE_AC && local_id == ONLP_PSU_0) {
        n_value = onlp_i2c_readb(i2c_bus, PSU0_ADDRESS, PMBUS_VOUT_MODE_BYTE, ONLP_I2C_F_FORCE);
    } else if(psu_type == ONLP_PSU_TYPE_AC && local_id == ONLP_PSU_1) {
        n_value = onlp_i2c_readb(i2c_bus, PSU1_ADDRESS, PMBUS_VOUT_MODE_BYTE, ONLP_I2C_F_FORCE);
    } else if(psu_type == ONLP_PSU_TYPE_DC48) {
        /* Switch to page 00h */
        if ((ret = onlp_i2c_writeb(i2c_bus, PSU0_ADDRESS, PMBUS_PAGE_COMMAND, DC_PSU_MAIN_PW, ONLP_I2C_F_FORCE | ONLP_I2C_F_PEC)) < 0) {
            return ONLP_STATUS_E_INTERNAL;
        }
        n_value = onlp_i2c_readb(i2c_bus, PSU0_ADDRESS, PMBUS_VOUT_MODE_BYTE, ONLP_I2C_F_FORCE);
    } else {
        return ONLP_STATUS_E_INVALID;
    }

    if (n_value < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    /* Read word */
    if((psu_type == ONLP_PSU_TYPE_AC && local_id == ONLP_PSU_0) ||    \
       (psu_type == ONLP_PSU_TYPE_DC48)) {
        v_value = onlp_i2c_readw(i2c_bus, PSU0_ADDRESS, PMBUS_READ_VOUT_WORD, ONLP_I2C_F_FORCE);
    } else if(psu_type == ONLP_PSU_TYPE_AC && local_id == ONLP_PSU_1) {
        v_value = onlp_i2c_readw(i2c_bus, PSU1_ADDRESS, PMBUS_READ_VOUT_WORD, ONLP_I2C_F_FORCE);
    } else {
        return ONLP_STATUS_E_INVALID;
    }

    if (v_value < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }


    //if ((n_value & 0xE0) == 0 /* Linear 16 format (Follow PMBUS Spec) */) {
    /* ***************************************************************************************************
        For DC PSU Vout will follow Linear 16 format (meets the specifications of PMBUS Spec.),
        and for AC PSU will follow Linear 11 format (Both 3Y and Delta AC PSU).
    **************************************************************************************************** */
    if (psu_type == ONLP_PSU_TYPE_DC48) {

        /* If it's negative two's complement integer */
        if (n_value & 0x10) {
            n_value = ~(n_value & 0x1F) + 1;

            /* 1<<n_value is 2^n_value */
            temp = (unsigned int)(1<<n_value);
            if (temp)
                snprintf(result, result_size, "%d.%04d", v_value/temp, ((v_value%temp)*10000)/temp);
        } else {
            snprintf(result, result_size, "%d", (v_value*(1<<n_value)));
        }
    } else if (psu_type == ONLP_PSU_TYPE_AC) {
        /* Linear 11 format (only for AC PSU (3Y and Delta)) */
        n_value = (v_value >> (16 -5)) & 0x1F;
        v_value = (v_value & 0x7FF);

        /* If it's negative two's complement integer */
        if (n_value & 0x10) {
            n_value = ~(n_value & 0x1F) + 1;

            if(v_value & 0x400) {
                v_value = ~(v_value & 0x1F) + 1;
            }

            temp = (unsigned int)(1<<n_value);
            if (temp)
                snprintf(result, result_size, "%d.%04d", v_value/temp, ((v_value%temp)*10000)/temp);
        } else {
            if(v_value & 0x400) {
                v_value = ~(v_value & 0x1F) + 1;
            }

            snprintf(result, result_size, "%d", (v_value*(1<<n_value)));
        }
    } else {
        AIM_LOG_ERROR("Unknown PSU Type(%d) after reading vout from pmbus\r\n", psu_type);
        return ONLP_STATUS_E_INTERNAL;
    }

    dvalue = atof((const char *)result);
    if (dvalue > 0.0) {
        info->caps |= ONLP_PSU_CAPS_VOUT;
        info->mvout = (int)(dvalue * 1000);
    }

    return ONLP_STATUS_OK;
}

static int get_pmbus_psu_iin(int psu_type, int local_id, onlp_psu_info_t* info, int i2c_bus)
{
    int value;
    unsigned int y_value = 0;
    unsigned char n_value = 0;
    unsigned int temp = 0;
    char result[32];
    memset(result, 0, sizeof(result));
    double dvalue;
    int ret = 0;

    if ((psu_type == ONLP_PSU_TYPE_AC && local_id == ONLP_PSU_0)) {
        value = onlp_i2c_readw(i2c_bus, PSU0_ADDRESS, PMBUS_READ_IIN, ONLP_I2C_F_FORCE);
    } else if(psu_type == ONLP_PSU_TYPE_AC && local_id == ONLP_PSU_1) {
        value = onlp_i2c_readw(i2c_bus, PSU1_ADDRESS, PMBUS_READ_IIN, ONLP_I2C_F_FORCE);
    } else if (psu_type == ONLP_PSU_TYPE_DC48 && (local_id == ONLP_DUAL_PSU_0_PLUG_0)) {
        /* Switch to page 00h */
        if ((ret = onlp_i2c_writeb(i2c_bus, PSU0_ADDRESS, PMBUS_PAGE_COMMAND, DC_PSU_MAIN_PW, ONLP_I2C_F_FORCE | ONLP_I2C_F_PEC)) < 0) {
            return ONLP_STATUS_E_INTERNAL;
        }

        value = onlp_i2c_readw(i2c_bus, PSU0_ADDRESS, PMBUS_READ_IIN, ONLP_I2C_F_FORCE);
    } else if (psu_type == ONLP_PSU_TYPE_DC48 && (local_id == ONLP_DUAL_PSU_0_PLUG_1)) {
        /* Switch to page 20h */
        if ((ret = onlp_i2c_writeb(i2c_bus, PSU0_ADDRESS, PMBUS_PAGE_COMMAND, DC_PSU_SECOND_PW, ONLP_I2C_F_FORCE | ONLP_I2C_F_PEC)) < 0) {
            return ONLP_STATUS_E_INTERNAL;
        }

        value = onlp_i2c_readw(i2c_bus, PSU0_ADDRESS, PMBUS_READ_IIN, ONLP_I2C_F_FORCE);
    } else {
        return ONLP_STATUS_E_INVALID;
    }

    if (value < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    y_value = (value & 0x07FF);
    if ((value & 0x8000)&&(y_value)) {
        n_value = 0xF0 + (((value) >> 11) & 0x0F);
        n_value = (~n_value) +1;
        temp = (unsigned int)(1<<n_value);
        if (temp) {
            snprintf(result, sizeof(result), "%d.%04d", y_value/temp, ((y_value%temp)*10000)/temp);
        }
    } else {
        n_value = (((value) >> 11) & 0x0F);
        snprintf(result, sizeof(result), "%d", (y_value*(1<<n_value)));
    }

    dvalue = atof((const char *)result);
    if (dvalue > 0.0) {
        info->caps |= ONLP_PSU_CAPS_IIN;
        info->miin = (int)(dvalue * 1000);
    }

    return ONLP_STATUS_OK;
}

static int get_pmbus_psu_iout(int psu_type, int local_id, onlp_psu_info_t* info, int i2c_bus)
{
    int value;
    unsigned int y_value = 0;
    unsigned char n_value = 0;
    unsigned int temp = 0;
    char result[32];
    memset(result, 0, sizeof(result));
    double dvalue;
    int ret = 0;

    if(psu_type == ONLP_PSU_TYPE_AC && local_id == ONLP_PSU_0) {
        value = onlp_i2c_readw(i2c_bus, PSU0_ADDRESS, PMBUS_READ_IOUT, ONLP_I2C_F_FORCE);
    } else if(psu_type == ONLP_PSU_TYPE_AC && local_id == ONLP_PSU_1) {
        value = onlp_i2c_readw(i2c_bus, PSU1_ADDRESS, PMBUS_READ_IOUT, ONLP_I2C_F_FORCE);
    } else if(psu_type == ONLP_PSU_TYPE_DC48) {
        /* Switch to page 00h */
        if ((ret = onlp_i2c_writeb(i2c_bus, PSU0_ADDRESS, PMBUS_PAGE_COMMAND, DC_PSU_MAIN_PW, ONLP_I2C_F_FORCE | ONLP_I2C_F_PEC)) < 0) {
            return ONLP_STATUS_E_INTERNAL;
        }
        value = onlp_i2c_readw(i2c_bus, PSU0_ADDRESS, PMBUS_READ_IOUT, ONLP_I2C_F_FORCE);
    } else {
        return ONLP_STATUS_E_INVALID;
    }

    if (value < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    y_value = (value & 0x07FF);
    if ((value & 0x8000)&&(y_value)) {
        n_value = 0xF0 + (((value) >> 11) & 0x0F);
        n_value = (~n_value) +1;
        temp = (unsigned int)(1<<n_value);
        if (temp) {
            snprintf(result, sizeof(result), "%d.%04d", y_value/temp, ((y_value%temp)*10000)/temp);
        }
    } else {
        n_value = (((value) >> 11) & 0x0F);
        snprintf(result, sizeof(result), "%d", (y_value*(1<<n_value)));
    }

    dvalue = atof((const char *)result);
    if (dvalue > 0.0) {
        info->caps |= ONLP_PSU_CAPS_IOUT;
        info->miout = (int)(dvalue * 1000);
    }

    return ONLP_STATUS_OK;
}

static int get_pmbus_psu_pin(int psu_type, int local_id, onlp_psu_info_t* info, int i2c_bus)
{
    int value;
    unsigned int y_value = 0;
    unsigned char n_value = 0;
    unsigned int temp = 0;
    char result[32];
    memset(result, 0, sizeof(result));
    double dvalue;
    int ret = 0;

    if ((psu_type == ONLP_PSU_TYPE_AC && local_id == ONLP_PSU_0)) {
        value = onlp_i2c_readw(i2c_bus, PSU0_ADDRESS, PMBUS_READ_PIN, ONLP_I2C_F_FORCE);
    } else if(psu_type == ONLP_PSU_TYPE_AC && local_id == ONLP_PSU_1) {
        value = onlp_i2c_readw(i2c_bus, PSU1_ADDRESS, PMBUS_READ_PIN, ONLP_I2C_F_FORCE);
    } else if (psu_type == ONLP_PSU_TYPE_DC48 && (local_id == ONLP_DUAL_PSU_0_PLUG_0)) {
        /* Switch to page 00h */
        if ((ret = onlp_i2c_writeb(i2c_bus, PSU0_ADDRESS, PMBUS_PAGE_COMMAND, DC_PSU_MAIN_PW, ONLP_I2C_F_FORCE | ONLP_I2C_F_PEC)) < 0) {
            return ONLP_STATUS_E_INTERNAL;
        }

        value = onlp_i2c_readw(i2c_bus, PSU0_ADDRESS, PMBUS_READ_PIN, ONLP_I2C_F_FORCE);
    } else if (psu_type == ONLP_PSU_TYPE_DC48 && (local_id == ONLP_DUAL_PSU_0_PLUG_1)) {
        /* Switch to page 20h */
        if ((ret = onlp_i2c_writeb(i2c_bus, PSU0_ADDRESS, PMBUS_PAGE_COMMAND, DC_PSU_SECOND_PW, ONLP_I2C_F_FORCE | ONLP_I2C_F_PEC)) < 0) {
            return ONLP_STATUS_E_INTERNAL;
        }

        value = onlp_i2c_readw(i2c_bus, PSU0_ADDRESS, PMBUS_READ_PIN, ONLP_I2C_F_FORCE);
    } else {
        return ONLP_STATUS_E_INVALID;
    }

    if (value < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    y_value = (value & 0x07FF);
    if ((value & 0x8000)&&(y_value)) {
        n_value = 0xF0 + (((value) >> 11) & 0x0F);
        n_value = (~n_value) +1;
        temp = (unsigned int)(1<<n_value);
        if (temp) {
            snprintf(result, sizeof(result), "%d.%04d", y_value/temp, ((y_value%temp)*10000)/temp);
        }
    } else {
        n_value = (((value) >> 11) & 0x0F);
        snprintf(result, sizeof(result), "%d", (y_value*(1<<n_value)));
    }

    dvalue = atof((const char *)result);
    if (dvalue > 0.0) {
        info->caps |= ONLP_PSU_CAPS_PIN;
        info->mpin = (int)(dvalue * 1000);
    }

    return ONLP_STATUS_OK;
}

static int get_pmbus_psu_pout(int psu_type, int local_id, onlp_psu_info_t* info, int i2c_bus)
{
    int value;
    unsigned int y_value = 0;
    unsigned char n_value = 0;
    unsigned int temp = 0;
    char result[32];
    memset(result, 0, sizeof(result));
    double dvalue;
    int ret = 0;

    if(psu_type == ONLP_PSU_TYPE_AC && local_id == ONLP_PSU_0) {
        value = onlp_i2c_readw(i2c_bus, PSU0_ADDRESS, PMBUS_READ_POUT, ONLP_I2C_F_FORCE);
    } else if(psu_type == ONLP_PSU_TYPE_AC && local_id == ONLP_PSU_1) {
        value = onlp_i2c_readw(i2c_bus, PSU1_ADDRESS, PMBUS_READ_POUT, ONLP_I2C_F_FORCE);
    } else if(psu_type == ONLP_PSU_TYPE_DC48) {
        /* Switch to page 00h */
        if ((ret = onlp_i2c_writeb(i2c_bus, PSU0_ADDRESS, PMBUS_PAGE_COMMAND, DC_PSU_MAIN_PW, ONLP_I2C_F_FORCE | ONLP_I2C_F_PEC)) < 0) {
            return ONLP_STATUS_E_INTERNAL;
        }
        value = onlp_i2c_readw(i2c_bus, PSU0_ADDRESS, PMBUS_READ_POUT, ONLP_I2C_F_FORCE);
    } else {
        return ONLP_STATUS_E_INVALID;
    }

    if (value < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    y_value = (value & 0x07FF);
    if ((value & 0x8000)&&(y_value)) {
        n_value = 0xF0 + (((value) >> 11) & 0x0F);
        n_value = (~n_value) +1;
        temp = (unsigned int)(1<<n_value);
        if (temp) {
            snprintf(result, sizeof(result), "%d.%04d", y_value/temp, ((y_value%temp)*10000)/temp);
        }
    } else {
        n_value = (((value) >> 11) & 0x0F);
        snprintf(result, sizeof(result), "%d", (y_value*(1<<n_value)));
    }

    dvalue = atof((const char *)result);
    if (dvalue > 0.0) {
        info->caps |= ONLP_PSU_CAPS_POUT;
        info->mpout = (int)(dvalue * 1000);
    }

    return ONLP_STATUS_OK;
}

static int get_pmbus_psu_eeprom(int psu_type, onlp_psu_info_t* info, int local_id)
{
    uint8_t data[256];
    //char eeprom_path[128];
    int data_len, i, rc;
    memset(data, 0, sizeof(data));
    //memset(eeprom_path, 0, sizeof(eeprom_path));

    if ((psu_type == ONLP_PSU_TYPE_AC) && (local_id == ONLP_PSU_0)) {
        rc = onlp_file_read(data, sizeof(data), &data_len, PSU0_EEPROM_PATH);
    } else if ((psu_type == ONLP_PSU_TYPE_AC) && (local_id == ONLP_PSU_1)) {
        rc = onlp_file_read(data, sizeof(data), &data_len, PSU1_EEPROM_PATH);
    } else if (psu_type == ONLP_PSU_TYPE_DC48) {
        rc = onlp_i2c_block_read(13, 0x51, 0, sizeof(data), data, ONLP_I2C_F_FORCE);
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (rc == ONLP_STATUS_OK) {
        i = 11;

        /* Manufacturer Name */
        data_len = (data[i]&0x3f);
        i++;
        i += data_len;

        /* Product Name */
        data_len = (data[i]&0x3f);
        i++;
        memset(info->model, 0, sizeof(info->model));
        memcpy(info->model, (char *) &(data[i]), data_len);
        i += data_len;

        /* Product part,model number */
        data_len = (data[i]&0x3f);
        i++;
        i += data_len;

        /* Product Version */
        data_len = (data[i]&0x3f);
        i++;
        i += data_len;

        /* Product Serial Number */
        data_len = (data[i]&0x3f);
        i++;
        memset(info->serial, 0, sizeof(info->serial));
        memcpy(info->serial, (char *) &(data[i]), data_len);
    } else {
        memset(info->model, 0, sizeof(info->model));
        memset(info->serial, 0, sizeof(info->serial));
        strcpy(info->model, COMM_STR_NOT_AVAILABLE);
        strcpy(info->serial, COMM_STR_NOT_AVAILABLE);
    }

    return ONLP_STATUS_OK;
}

