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
 * Fan Platform Implementation Defaults.
 *
 ***********************************************************/
#include <onlp/platformi/fani.h>
#include "platform_lib.h"

#define FAN0_PRES_MASK      0b00000001
#define FAN1_PRES_MASK      0b00000010
#define FAN_PRES            0
#define SYSFS_FAN_STATUS    LPC_MB_CPLD_PATH"fan_status"
#define SYS_FAN_RPM_MAX     25000

#define FAN_STATUS ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B
#define FAN_CAPS   ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE
#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_FAN(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

#define CHASSIS_INFO(id, des)                   \
    {                                           \
        { ONLP_FAN_ID_CREATE(id), des, POID_0}, \
        FAN_STATUS,                             \
        FAN_CAPS,                               \
        0,                                      \
        0,                                      \
        ONLP_FAN_MODE_INVALID,                  \
    }

/*
 * Get the fan information.
 */

onlp_fan_info_t fan_info[] = {
    { }, /* Not used */
    CHASSIS_INFO(ONLP_FAN_0    , "Chassis Fan - 0"),
    CHASSIS_INFO(ONLP_FAN_1    , "Chassis Fan - 1"),
    CHASSIS_INFO(ONLP_PSU_0_FAN, "PSU-0-Fan"),
    CHASSIS_INFO(ONLP_PSU_1_FAN, "PSU-1-Fan"),
};

int sys_fan_info_get(onlp_fan_info_t* info, int local_id)
{
    int rpm = 0;
    int hwm_id, sysfs_id, gpio_off, fan_dir;
    int hw_rev_id, gpio_max;

    hwm_id = UCD_HWM_ID;
    if( local_id == ONLP_FAN_0) {
        sysfs_id = 1;
        gpio_off = FAN0_DIR_GPIO_OFF;
    } else {
        sysfs_id = 2;
        gpio_off = FAN1_DIR_GPIO_OFF;
    }

    //get fan rpm
    ONLP_TRY(onlp_file_read_int(&rpm, SYS_HWMON_FMT "fan%d_input", hwm_id, sysfs_id));


    // get fan dir
    hw_rev_id = get_hw_rev_id();
    if(hw_rev_id <= HW_REV_ALPHA) {
        goto SKIP_FAN_DIR_DETECT;
    }

    gpio_max = get_gpio_max();
    ONLP_TRY(onlp_file_read_int(&fan_dir, SYS_GPIO_FMT, gpio_max - gpio_off));
    if(fan_dir == FAN_DIR_F2B) {
        /* F2B */
        info->status |= ONLP_FAN_STATUS_F2B;
        info->status &= ~ONLP_FAN_STATUS_B2F;
    } else {
        /* B2F */
        info->status |= ONLP_FAN_STATUS_B2F;
        info->status &= ~ONLP_FAN_STATUS_F2B;
    }

    // get fan fru
    strcpy(info->model, "not supported");
    strcpy(info->serial, "not supported");

SKIP_FAN_DIR_DETECT:
    info->rpm = rpm;
    info->percentage = (info->rpm*100)/SYS_FAN_RPM_MAX;
    info->status |= (rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;

    return ONLP_STATUS_OK;
}


/**
  * @brief Initialize the fan platform subsystem.
  */
int onlp_fani_init(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the information structure for the given fan OID.
 * @param id The fan OID
 * @param rv [out] Receives the fan information.
 */
int onlp_fani_info_get(onlp_oid_t id, onlp_fan_info_t* rv)
{
    int local_id;
    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);
    *rv = fan_info[local_id];
    ONLP_TRY(onlp_fani_status_get(id, &rv->status));

    if((rv->status & ONLP_FAN_STATUS_PRESENT) == 0) {
        return ONLP_STATUS_OK;
    }

    switch(local_id) {
        case ONLP_FAN_0:
        case ONLP_FAN_1:
            return sys_fan_info_get(rv, local_id);
        case ONLP_PSU_0_FAN:
        case ONLP_PSU_1_FAN:
            return psu_fan_info_get(rv, local_id);
        default:
            return ONLP_STATUS_E_INVALID;
    }
}

/**
 * @brief Retrieve the fan's operational status.
 * @param id The fan OID.
 * @param rv [out] Receives the fan's operations status flags.
 * @notes Only operational state needs to be returned -
 *        PRESENT/FAILED
 */
int onlp_fani_status_get(onlp_oid_t id, uint32_t* rv)
{
    int local_id;
    int val, val_mask;
    int presence;
    local_id = ONLP_OID_ID_GET(id);

    //get presence
    switch(local_id) {
        case ONLP_FAN_0:
        case ONLP_FAN_1:
            if(local_id == ONLP_FAN_0) {
                val_mask = FAN0_PRES_MASK;
            } else {
                val_mask = FAN1_PRES_MASK;
            }

            ONLP_TRY(file_read_hex(&val, SYSFS_FAN_STATUS));
            presence = ((val & val_mask) == FAN_PRES) ? 1 : 0;
            break;
        case ONLP_PSU_0_FAN:
            ONLP_TRY(psu_present_get(&presence, ONLP_PSU_0));
            break;
        case ONLP_PSU_1_FAN:
            ONLP_TRY(psu_present_get(&presence, ONLP_PSU_1));
            break;
        default:
            return ONLP_STATUS_E_INVALID;
    }

    if(presence) {
        *rv |= ONLP_FAN_STATUS_PRESENT;
    } else {
        *rv &= ~ONLP_FAN_STATUS_PRESENT;
    }


    return ONLP_STATUS_OK;
}

/**
 * @brief Retrieve the fan's OID hdr.
 * @param id The fan OID.
 * @param rv [out] Receives the OID header.
 */
int onlp_fani_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* hdr)
{
    int local_id;
    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);
    if(local_id >= ONLP_FAN_MAX) {
        return ONLP_STATUS_E_INVALID;
    } else {
        *hdr = fan_info[local_id].hdr;
    }
    return ONLP_STATUS_OK;
}

/**
 * @brief Set the fan speed in RPM.
 * @param id The fan OID
 * @param rpm The new RPM
 * @note This is only relevant if the RPM capability is set.
 */
int onlp_fani_rpm_set(onlp_oid_t id, int rpm)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Set the fan speed in percentage.
 * @param id The fan OID.
 * @param p The new fan speed percentage.
 * @note This is only relevant if the PERCENTAGE capability is set.
 */
int onlp_fani_percentage_set(onlp_oid_t id, int p)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Set the fan mode.
 * @param id The fan OID.
 * @param mode The new fan mode.
 */
int onlp_fani_mode_set(onlp_oid_t id, onlp_fan_mode_t mode)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Set the fan direction (if supported).
 * @param id The fan OID
 * @param dir The direction.
 */
int onlp_fani_dir_set(onlp_oid_t id, onlp_fan_dir_t dir)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Generic fan ioctl
 * @param id The fan OID
 * @param vargs The variable argument list for the ioctl call.
 * @param Optional
 */
int onlp_fani_ioctl(onlp_oid_t id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}


