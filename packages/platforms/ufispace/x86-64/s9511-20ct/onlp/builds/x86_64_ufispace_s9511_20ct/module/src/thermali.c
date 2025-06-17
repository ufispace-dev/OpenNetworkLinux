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
 * Thermal Sensor Platform Implementation.
 *
 ***********************************************************/
#include <onlp/platformi/thermali.h>
#include "platform_lib.h"

#define MILLI(cel) (cel * 1000)

//FIXME threshold
static onlp_thermal_info_t thermal_info[] = {
    { }, /* Not used */
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PKG),
            .description = "CPU Package",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAINBOARD_MAC),
            .description = "TEMP_MAC",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAINBOARD_GDDR6),
            .description = "TEMP_GDDR6",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAINBOARD_NTM),
            .description = "TEMP_NTM",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_VMON_HWM_MAC),
            .description = "HWM_TEMP_MAC",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE|ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD|ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_VMON_HWM_AMB),
            .description = "HWM_TEMP_AMB",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_ALL)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_VMON_HWM_PHY),
            .description = "HWM_TEMP_PHY",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE|ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD|ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU_0),
            .description = "PSU 0 THERMAL 1",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE|ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD|ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD)
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU_1),
            .description = "PSU 1 THERMAL 1",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE|ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD|ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD)
    },
};

typedef enum thrm_attr_type_e {
    TYPE_THRM_ATTR_UNNKOW = 0,
    TYPE_THRM_ATTR_SYSFS,
    TYPE_THRM_ATTR_BMC,
    TYPE_THRM_ATTR_PMBUS,
    TYPE_THRM_ATTR_CPLD,
    TYPE_THRM_ATTR_MAX,
} thrm_type_t;
typedef struct
{
    int type;
    int attr;
} thrm_attr_t;

static const thrm_attr_t thrm_attr[] = {
    /*                                type                       attr(sysfs_index) */
    [ONLP_THERMAL_CPU_PKG]          ={TYPE_THRM_ATTR_SYSFS      ,1},
    [ONLP_THERMAL_MAINBOARD_MAC]    ={TYPE_THRM_ATTR_SYSFS      ,2},
    [ONLP_THERMAL_MAINBOARD_GDDR6]  ={TYPE_THRM_ATTR_SYSFS      ,1},
    [ONLP_THERMAL_MAINBOARD_NTM]    ={TYPE_THRM_ATTR_SYSFS      ,1},
    [ONLP_THERMAL_VMON_HWM_MAC]     ={TYPE_THRM_ATTR_CPLD       ,-1},
    [ONLP_THERMAL_VMON_HWM_AMB]     ={TYPE_THRM_ATTR_CPLD       ,-1},
    [ONLP_THERMAL_VMON_HWM_PHY]     ={TYPE_THRM_ATTR_CPLD       ,-1},
    [ONLP_THERMAL_PSU_0]            ={TYPE_THRM_ATTR_PMBUS      ,-1},
    [ONLP_THERMAL_PSU_1]            ={TYPE_THRM_ATTR_PMBUS      ,-1},
};

/* Ufispace Specific Defined functions */
static int get_thermal_local_id(int id, int *local_id);
static int get_hwm_thermal_info(int local_id, onlp_thermal_info_t* info);
static int get_pmbus_thermal_info(int local_id, onlp_thermal_info_t* info);
static int get_cpld_thermal_info(int local_id, onlp_thermal_info_t* info);

/******************************************************************************************************************
**                                                                                                               **
**                                                ONLP Standard APIs                                             **
**                                                                                                               **
*******************************************************************************************************************/
/**
 * @brief Initialize the thermal subsystem.
 */
int onlp_thermali_init(void)
{
    init_lock();
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the information for the given thermal OID.
 * @param id The Thermal OID
 * @param rv [out] Receives the thermal information.
 */
int onlp_thermali_info_get(onlp_oid_t id, onlp_thermal_info_t* rv)
{
    int local_id;

    if(rv == NULL) {
        return ONLP_STATUS_E_PARAM;
    }

    ONLP_TRY(get_thermal_local_id(id, &local_id));

    /* update info  */
    if(thrm_attr[local_id].type == TYPE_THRM_ATTR_SYSFS) {
        ONLP_TRY(get_hwm_thermal_info(local_id, rv));
    } else if(thrm_attr[local_id].type == TYPE_THRM_ATTR_CPLD) {
        ONLP_TRY(get_cpld_thermal_info(local_id, rv));
    } else if(thrm_attr[local_id].type == TYPE_THRM_ATTR_PMBUS) {
        ONLP_TRY(get_pmbus_thermal_info(local_id, rv));
    } else {
        return ONLP_STATUS_E_INVALID;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Retrieve the thermal's operational status.
 * @param id The thermal oid.
 * @param rv [out] Receives the operational status.
 */
int onlp_thermali_status_get(onlp_oid_t id, uint32_t* rv)
{
    int local_id;
    onlp_thermal_info_t info ={0};

    ONLP_TRY(get_thermal_local_id(id, &local_id));

    if(thrm_attr[local_id].type == TYPE_THRM_ATTR_SYSFS) {
        ONLP_TRY(get_hwm_thermal_info(local_id, &info));
    } else if(thrm_attr[local_id].type == TYPE_THRM_ATTR_CPLD) {
        ONLP_TRY(get_cpld_thermal_info(local_id, &info));
    } else if(thrm_attr[local_id].type == TYPE_THRM_ATTR_PMBUS) {
        ONLP_TRY(get_pmbus_thermal_info(local_id, &info));
    } else {
        return ONLP_STATUS_E_INVALID;
    }

    *rv = info.status;

    return ONLP_STATUS_OK;
}

/**
 * @brief Retrieve the thermal's oid header.
 * @param id The thermal oid.
 * @param rv [out] Receives the header.
 */
int onlp_thermali_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* rv)
{
    int local_id;

    ONLP_TRY(get_thermal_local_id(id, &local_id));
    *rv = thermal_info[local_id].hdr;

    return ONLP_STATUS_OK;
}

/**
 * @brief Generic ioctl.
 */
int onlp_thermali_ioctl(int id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}


/******************************************************************************************************************
**                                                                                                               **
**                                           Upispace Specific Defined APIs                                      **
**                                                                                                               **
*******************************************************************************************************************/
/**
 * @brief Get and check thermal local ID
 * @param id [in] OID
 * @param local_id [out] The thermal local id
 */
static int get_thermal_local_id(int id, int *local_id)
{
    int tmp_id;
    if(local_id == NULL) {
        return ONLP_STATUS_E_PARAM;
    }

    if(!ONLP_OID_IS_THERMAL(id)) {
        return ONLP_STATUS_E_INVALID;
    }

    tmp_id = ONLP_OID_ID_GET(id);
    switch (tmp_id) {
        case ONLP_THERMAL_CPU_PKG:
        case ONLP_THERMAL_MAINBOARD_MAC:
        case ONLP_THERMAL_MAINBOARD_GDDR6:
        case ONLP_THERMAL_MAINBOARD_NTM:
        case ONLP_THERMAL_VMON_HWM_MAC:
        case ONLP_THERMAL_VMON_HWM_AMB:
        case ONLP_THERMAL_VMON_HWM_PHY:
        case ONLP_THERMAL_PSU_0:
        case ONLP_THERMAL_PSU_1:
            *local_id = tmp_id;
            return ONLP_STATUS_OK;
        default:
            return ONLP_STATUS_E_INVALID;
    }
    return ONLP_STATUS_E_INVALID;
}

static int get_hwm_thermal_info(int local_id, onlp_thermal_info_t* info)
{
    int rv = 0;
    int attr = 0;
    temp_thld_t temp_thld = {0};

    ONLP_TRY(get_thermal_thld(local_id, &temp_thld));

    *info = thermal_info[local_id];
    info->thresholds.warning = MILLI(temp_thld.warning);
    info->thresholds.error = MILLI(temp_thld.error);
    info->thresholds.shutdown = MILLI(temp_thld.shutdown);

    /* present */
    info->status |= ONLP_THERMAL_STATUS_PRESENT;

    /* contents */
    attr = thrm_attr[local_id].attr;
    if(attr < 0) {
        return ONLP_STATUS_E_INVALID;
    }

    switch(local_id) {
        case ONLP_THERMAL_CPU_PKG:
            rv = onlp_file_read_int(&info->mcelsius,
                                    SYS_CPU_CORETEMP_PREFIX "temp%d_input", attr);
            if(rv < 0) {
                rv = onlp_file_read_int(&info->mcelsius,
                                        SYS_CPU_CORETEMP_PREFIX2 "temp%d_input", attr);
                if(rv < 0) {
                    return rv;
                }
            }
            break;
        case ONLP_THERMAL_MAINBOARD_MAC:
        case ONLP_THERMAL_MAINBOARD_GDDR6:
            rv = onlp_file_read_int(&info->mcelsius,
                                    SYS_HWM_PREFIX "temp%d_input", HWM_TMP451_INDEX, attr);
            break;
        case ONLP_THERMAL_MAINBOARD_NTM:
            rv = onlp_file_read_int(&info->mcelsius,
                                    SYS_HWM_PREFIX "temp%d_input", HWM_TMP75_INDEX, attr);
            break;
        default:
            return ONLP_STATUS_E_INVALID;
    }

    return rv;
}

static int get_pmbus_thermal_info(int local_id, onlp_thermal_info_t* info)
{
    int psu_id = -1;
    //int pw_present, pw_good;
    int pw_present;
    int psu_type = ONLP_PSU_TYPE_INVALID;
    int i2c_bus, i2c_addr, offset;
    int value, buf;
    unsigned int y_value = 0;
    unsigned char n_value = 0;
    unsigned int temp = 0;
    char result[32];
    temp_thld_t temp_thld = {0};

    ONLP_TRY(get_thermal_thld(local_id, &temp_thld));
    *info = thermal_info[local_id];
    info->thresholds.warning = MILLI(temp_thld.warning);
    info->thresholds.error = MILLI(temp_thld.error);
    info->thresholds.shutdown = MILLI(temp_thld.shutdown);

    /* Check PSU type */
    ONLP_TRY(get_psu_type(&psu_type));

    if((psu_type == ONLP_PSU_TYPE_AC) && (local_id == ONLP_THERMAL_PSU_0)) {
        psu_id = 1;
        i2c_bus = I2C_BUS_PSU0;
        i2c_addr = PSU0_ADDRESS;
        offset = PMBUS_READ_THERMAL1;
    } else if ((psu_type == ONLP_PSU_TYPE_AC) && (local_id == ONLP_THERMAL_PSU_1)) {
        psu_id = 2;
        i2c_bus = I2C_BUS_PSU1;
        i2c_addr = PSU1_ADDRESS;
        offset = PMBUS_READ_THERMAL1;
    } else if (psu_type == ONLP_PSU_TYPE_DC48) {
        psu_id = 1;
        i2c_bus = I2C_BUS_PSU0;
        i2c_addr = PSU0_ADDRESS;
        offset = PMBUS_READ_THERMAL1;
    } else {
        return ONLP_STATUS_E_INVALID;
    }

    /* check psu status */
    ONLP_TRY(get_psu_present_status(psu_type, psu_id, &pw_present));
    if(pw_present) {
        info->status |= ONLP_THERMAL_STATUS_PRESENT;
    } else {
        info->mcelsius = 0;
        info->status &= ~ONLP_THERMAL_STATUS_PRESENT;
        return ONLP_STATUS_OK;
    }

    //ONLP_TRY(get_psu_pwgood_status(psu_type, psu_id, &pw_good));
    //if(!pw_good) {
    //    info->mcelsius = 0;
    //    return ONLP_STATUS_OK;
    //}

    /* get thermal value */
    value = onlp_i2c_readw(i2c_bus, i2c_addr, offset, ONLP_I2C_F_FORCE);

    y_value = (value & 0x07FF);
    if((value & 0x8000)&&(y_value)) {
        n_value = 0xF0 + (((value) >> 11) & 0x0F);
        n_value = (~n_value) +1;
        temp = (unsigned int)(1<<n_value);
        if(temp)
            snprintf(result, sizeof(result), "%d.%04d", y_value/temp, ((y_value%temp)*10000)/temp);
    } else {
        n_value = (((value) >> 11) & 0x0F);
        snprintf(result, sizeof(result), "%d", (y_value*(1<<n_value)));
    }

    buf = atof((const char *)result);
    info->mcelsius = (int)(buf * 1000);

    return ONLP_STATUS_OK;
}

static int get_cpld_thermal_info(int local_id, onlp_thermal_info_t* info)
{
    char *sysfs = NULL;
    int voltage; /* binary 8 bit voltage value */
    double ambient_temp;
    float d_voltage = 0;  /* decimal voltage value in V */
    int base = 1;
    float decimal_voltage_mv = 0;  /* decimal voltage value in mV */
    const int VOLTAGE_OFFSET = 500;
    const int TMP23X_TEMP_COEFFICIENT = 10;
    const int TEMP_INFLECTION = 0;
    const int VOLTAGE_RANGE = 1500;
    temp_thld_t temp_thld = {0};

    ONLP_TRY(get_thermal_thld(local_id, &temp_thld));
    *info = thermal_info[local_id];
    info->thresholds.warning = MILLI(temp_thld.warning);
    info->thresholds.error = MILLI(temp_thld.error);
    info->thresholds.shutdown = MILLI(temp_thld.shutdown);

    /* Get the below three temp from CPLD (tmp235) */
    switch(local_id) {
        case ONLP_THERMAL_VMON_HWM_MAC:
            sysfs = SYSFS_CPLD2 "vol_14_value";
            break;
        case ONLP_THERMAL_VMON_HWM_AMB:
            sysfs = SYSFS_CPLD2 "vol_3_value";
            break;
        case ONLP_THERMAL_VMON_HWM_PHY:
            sysfs = SYSFS_CPLD2 "vol_4_value";
            break;
        default:
            sysfs = "";
            return ONLP_STATUS_E_PARAM;
    }

    ONLP_TRY(read_file_hex(&voltage, sysfs));

    /* The real voltage data should be added "0000" to the low bit (due to CPLD Spec) */
    voltage = (int)voltage << 4;

    /* This is the real Vout read from CPLD after calculate */
    d_voltage = voltage * (0.0008) * (base);

    /* Transform V to mv */
    decimal_voltage_mv = d_voltage * 1000;

    /* ambient_temp from -40000~100000 millidegrees Celsius is related to voltage_range < 1500mV */
    if(decimal_voltage_mv < VOLTAGE_RANGE) {
        ambient_temp = (((decimal_voltage_mv - VOLTAGE_OFFSET)/ TMP23X_TEMP_COEFFICIENT) + TEMP_INFLECTION) * 1000;
    } else {
        AIM_LOG_ERROR("[FAILED] decimal_voltage_mv = %d is too high (>=1500 mV)%s", decimal_voltage_mv);
        return ONLP_STATUS_E_INTERNAL;
    }

    info->mcelsius = ambient_temp;

    return ONLP_STATUS_OK;
}
