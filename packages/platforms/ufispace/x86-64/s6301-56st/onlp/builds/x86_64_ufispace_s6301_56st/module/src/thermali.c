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

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_THERMAL(_id)) {         \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

/* SYSFS PATH */
#define SYS_CPU_CORETEMP_PREFIX "/sys/devices/platform/coretemp.0/hwmon/hwmon1/"
#define SYS_CPU_CORETEMP_PREFIX2 "/sys/devices/platform/coretemp.0/"

/* Threshold */
#define THERMAL_SHUTDOWN_DEFAULT    105000
#define THERMAL_ERROR_DEFAULT       95000
#define THERMAL_WARNING_DEFAULT     75000
#define THERMAL_NORMAL_DEFAULT      70000


/* Shortcut for CPU thermal threshold value. */
#define THERMAL_THRESHOLD_INIT_DEFAULTS  \
    { THERMAL_WARNING_DEFAULT, \
      THERMAL_ERROR_DEFAULT,   \
      THERMAL_SHUTDOWN_DEFAULT }

static onlp_thermal_info_t thermal_info[] = {
    { }, /* Not used */
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_FAN1),
            .description = "TEMP_FAN1",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {70000, 80000, 90000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_FAN2),
            .description = "TEMP_FAN2",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {70000, 80000, 90000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSUDB),
            .description = "TEMP_PSUDB",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {80000, 85000, 90000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_MAC),
            .description = "TEMP_MAC",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {90000, 100000, 110000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_INLET),
            .description = "TEMP_INLET",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {80000, 85000, 90000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU0),
            .description = "PSU 0 THERMAL 1",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {80000, 85000, 90000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_PSU1),
            .description = "PSU 1 THERMAL 1",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = {80000, 85000, 90000}
    },
    {
        .hdr = {
            .id = ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PKG),
            .description = "CPU Package",
            .poid = POID_0,
        },
        .status = ONLP_THERMAL_STATUS_PRESENT,
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE |
                 ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD |
                 ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD),
        .mcelsius = 0,
        .thresholds = THERMAL_THRESHOLD_INIT_DEFAULTS
    },
};

static int cpu_thermal_info_get(onlp_thermal_info_t* info)
{
    int rv;

    rv = onlp_file_read_int(&info->mcelsius,
                            SYS_CPU_CORETEMP_PREFIX "temp1_input");

    if(rv != ONLP_STATUS_OK) {

        rv = onlp_file_read_int(&info->mcelsius,
                            SYS_CPU_CORETEMP_PREFIX2 "temp1_input");
    }

    return rv;
}

int pmbus_thermal_info_get(onlp_thermal_info_t* info, int local_id)
{
    int psu_id, pres, pg;
    int i2c_bus, i2c_addr, offset;
    int value, buf;
    unsigned int y_value = 0;
    unsigned char n_value = 0;
    unsigned int temp = 0;
    char result[32];

    if(local_id == ONLP_THERMAL_PSU0) {
        psu_id = ONLP_PSU_0;
        i2c_bus = PSU_BUS_ID;
        i2c_addr = PSU_PMBUS_ADDR_0;
        offset = PSU_PMBUS_THERMAL1;
    } else if (local_id == ONLP_THERMAL_PSU1) {
        psu_id = ONLP_PSU_1;
        i2c_bus = PSU_BUS_ID;
        i2c_addr = PSU_PMBUS_ADDR_1;
        offset = PSU_PMBUS_THERMAL1;
    } else {
        return ONLP_STATUS_E_INVALID;
    }


    /* check psu status */
    ONLP_TRY(psu_present_get(&pres, psu_id));
    if(pres) {
        info->status |= ONLP_THERMAL_STATUS_PRESENT;
    } else {
        info->mcelsius = 0;
        info->status &= ~ONLP_THERMAL_STATUS_PRESENT;
        return ONLP_STATUS_OK;
    }

    ONLP_TRY(psu_pwgood_get(&pg, psu_id));
    if(!pg) {
        info->mcelsius = 0;
        return ONLP_STATUS_OK;
    }

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

int sysfs_thermal_info_get(onlp_thermal_info_t* info, int local_id)
{
    int rv, tmp_id, hwm_id;

    switch(local_id) {
        case ONLP_THERMAL_FAN1:
            hwm_id = TMP_FAN1_HWM_ID;
            tmp_id = 1;
            break;
        case ONLP_THERMAL_FAN2:
            hwm_id = TMP_FAN2_HWM_ID;
            tmp_id = 1;
            break;
        case ONLP_THERMAL_PSUDB:
            hwm_id = UCD_HWM_ID;
            // temp1 and temp5 is internal ucd90124 thermal by default
            tmp_id = 2;
            break;
        case ONLP_THERMAL_MAC:
            hwm_id = UCD_HWM_ID;
            tmp_id = 3;
            break;
        case ONLP_THERMAL_INLET:
            hwm_id = UCD_HWM_ID;
            tmp_id = 4;
            break;
        default:
            return ONLP_STATUS_E_INVALID;
    }

    rv = onlp_file_read_int(&info->mcelsius,
                      SYS_HWMON_FMT "temp%d_input", hwm_id, tmp_id);

    if(rv == ONLP_STATUS_E_MISSING) {
        info->status &= ~ONLP_THERMAL_STATUS_PRESENT;
        return rv;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Initialize the thermal subsystem.
 */
int onlp_thermali_init(void)
{
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
    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);

    *rv = thermal_info[local_id];

    /* update status  */
    ONLP_TRY(onlp_thermali_status_get(id, &rv->status));

    if((rv->status & ONLP_THERMAL_STATUS_PRESENT) == 0) {
        return ONLP_STATUS_OK;
    }

    switch(local_id) {
        case ONLP_THERMAL_FAN1:
        case ONLP_THERMAL_FAN2:
        case ONLP_THERMAL_PSUDB:
        case ONLP_THERMAL_MAC:
        case ONLP_THERMAL_INLET:
            ONLP_TRY(sysfs_thermal_info_get(rv, local_id));
            break;
        case ONLP_THERMAL_PSU0:
        case ONLP_THERMAL_PSU1:
            ONLP_TRY(pmbus_thermal_info_get(rv, local_id));
            break;
        case ONLP_THERMAL_CPU_PKG:
            ONLP_TRY(cpu_thermal_info_get(rv));
            break;
        default:
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
    VALIDATE(id);
    local_id = ONLP_OID_ID_GET(id);

    *rv = thermal_info[local_id].status;
    /* When the PSU module is unplugged, the psu thermal does not exist. */
    if(local_id == ONLP_THERMAL_PSU0 || local_id == ONLP_THERMAL_PSU1) {
        int psu_local_id = ONLP_PSU_MAX;

        if(local_id == ONLP_THERMAL_PSU0) {
             psu_local_id = ONLP_PSU_0;
        } else {
             psu_local_id = ONLP_PSU_1;
        }

        int psu_present = 0;
        ONLP_TRY(psu_present_get(&psu_present, psu_local_id));
        if(psu_present == 0) {
            *rv = ONLP_THERMAL_STATUS_FAILED;
        } else if(psu_present == 1) {
            *rv = ONLP_THERMAL_STATUS_PRESENT;
        }
    }

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
    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);
    if(local_id >= ONLP_THERMAL_MAX) {
        return ONLP_STATUS_E_INVALID;
    } else {
        *rv = thermal_info[local_id].hdr;
    }
    return ONLP_STATUS_OK;
}

/**
 * @brief Generic ioctl.
 */
int onlp_thermali_ioctl(int id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}


