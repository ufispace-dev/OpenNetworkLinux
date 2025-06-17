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
 * Fan Platform Implementation.
 *
 ***********************************************************/
#include <onlp/platformi/fani.h>
#include "platform_lib.h"

#define FAN_STATUS ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B
#define FAN_CAPS   ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE
#define SYS_FAN_0_2_FRONT_MAX_RPM       25000
#define SYS_FAN3_FRONT_MAX_RPM          18000
#define PSU_FAN_RPM_MAX_AC              18000  /* haven't confirm yet */

/* Ufispace SPecific Defined funcxtions */
static int get_fan_local_id(int psu_type, int id, int *local_id);
static int get_cpld_fan_info(int psu_type, int local_id, onlp_fan_info_t* info);

/*
 * Get the fan information.
 */

onlp_fan_info_t ac_fan_info[] = {
    { },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN_0),
            .description = "CHASSIS FAN 0",
            .poid = POID_0,
        },
        .status = FAN_STATUS,
        .caps = FAN_CAPS,
        .rpm = 0,
        .percentage = 0,
        .mode = ONLP_FAN_MODE_INVALID,
        .model = COMM_STR_NOT_SUPPORTED,
        .serial = COMM_STR_NOT_SUPPORTED,
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN_1),
            .description = "CHASSIS FAN 1",
            .poid = POID_0,
        },
        .status = FAN_STATUS,
        .caps = FAN_CAPS,
        .rpm = 0,
        .percentage = 0,
        .mode = ONLP_FAN_MODE_INVALID,
        .model = COMM_STR_NOT_SUPPORTED,
        .serial = COMM_STR_NOT_SUPPORTED,
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN_2),
            .description = "CHASSIS FAN 2",
            .poid = POID_0,
        },
        .status = FAN_STATUS,
        .caps = FAN_CAPS,
        .rpm = 0,
        .percentage = 0,
        .mode = ONLP_FAN_MODE_INVALID,
        .model = COMM_STR_NOT_SUPPORTED,
        .serial = COMM_STR_NOT_SUPPORTED,
    },
    {},
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_PSU_0_FAN),
            .description = "PSU 0 FAN",
            .poid = POID_0,
        },
        .status = FAN_STATUS,
        .caps = FAN_CAPS,
        .rpm = 0,
        .percentage = 0,
        .mode = ONLP_FAN_MODE_INVALID,
        .model = COMM_STR_NOT_SUPPORTED,
        .serial = COMM_STR_NOT_SUPPORTED,
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_PSU_1_FAN),
            .description = "PSU 1 FAN",
            .poid = POID_0,
        },
        .status = FAN_STATUS,
        .caps = FAN_CAPS,
        .rpm = 0,
        .percentage = 0,
        .mode = ONLP_FAN_MODE_INVALID,
        .model = COMM_STR_NOT_SUPPORTED,
        .serial = COMM_STR_NOT_SUPPORTED,
    },
};

onlp_fan_info_t dc_fan_info[] = {
    { },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN_0),
            .description = "CHASSIS FAN 0",
            .poid = POID_0,
        },
        .status = FAN_STATUS,
        .caps = FAN_CAPS,
        .rpm = 0,
        .percentage = 0,
        .mode = ONLP_FAN_MODE_INVALID,
        .model = COMM_STR_NOT_SUPPORTED,
        .serial = COMM_STR_NOT_SUPPORTED,
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN_1),
            .description = "CHASSIS FAN 1",
            .poid = POID_0,
        },
        .status = FAN_STATUS,
        .caps = FAN_CAPS,
        .rpm = 0,
        .percentage = 0,
        .mode = ONLP_FAN_MODE_INVALID,
        .model = COMM_STR_NOT_SUPPORTED,
        .serial = COMM_STR_NOT_SUPPORTED,
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN_2),
            .description = "CHASSIS FAN 2",
            .poid = POID_0,
        },
        .status = FAN_STATUS,
        .caps = FAN_CAPS,
        .rpm = 0,
        .percentage = 0,
        .mode = ONLP_FAN_MODE_INVALID,
        .model = COMM_STR_NOT_SUPPORTED,
        .serial = COMM_STR_NOT_SUPPORTED,
    },
    {
        .hdr = {
            .id = ONLP_FAN_ID_CREATE(ONLP_FAN_3),
            .description = "CHASSIS FAN 3",
            .poid = POID_0,
        },
        .status = FAN_STATUS,
        .caps = FAN_CAPS,
        .rpm = 0,
        .percentage = 0,
        .mode = ONLP_FAN_MODE_INVALID,
        .model = COMM_STR_NOT_SUPPORTED,
        .serial = COMM_STR_NOT_SUPPORTED,
    },
};


/******************************************************************************************************************
**                                                                                                               **
**                                                ONLP Standard APIs                                             **
**                                                                                                               **
*******************************************************************************************************************/
/**
  * @brief Initialize the fan platform subsystem.
  */
int onlp_fani_init(void)
{
    init_lock();
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the information structure for the given fan OID.
 * @param id The fan OID
 * @param rv [out] Receives the fan information.
 */
int onlp_fani_info_get(onlp_oid_t id, onlp_fan_info_t* rv)
{
    int psu_type = ONLP_PSU_TYPE_INVALID;
    int local_id;

    // Clean rv
    memset(rv, 0, sizeof(onlp_fan_info_t));

    ONLP_TRY(get_psu_type(&psu_type));
    ONLP_TRY(get_fan_local_id(psu_type, id, &local_id));

    if (psu_type == ONLP_PSU_TYPE_AC) {
        *rv = ac_fan_info[local_id];
    } else if (psu_type == ONLP_PSU_TYPE_DC48) {
        *rv = dc_fan_info[local_id];
    } else {
        AIM_LOG_ERROR("Unknown psu_type (%d)!! while onlp_fani_info_get())", psu_type);
        return ONLP_STATUS_E_INTERNAL;
    }

    ONLP_TRY(get_cpld_fan_info(psu_type, local_id, rv));

    return ONLP_STATUS_OK;
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
    int psu_type = ONLP_PSU_TYPE_INVALID;
    int local_id;
    onlp_fan_info_t info ={0};

    ONLP_TRY(get_psu_type(&psu_type));
    ONLP_TRY(get_fan_local_id(psu_type, id, &local_id));

    if (psu_type == ONLP_PSU_TYPE_AC) {
        info = ac_fan_info[local_id];
        info.caps |= ONLP_FAN_CAPS_GET_RPM;
    } else if (psu_type == ONLP_PSU_TYPE_DC48) {
        info = dc_fan_info[local_id];
        info.caps |= ONLP_FAN_CAPS_GET_RPM;
    } else {
        AIM_LOG_ERROR("Unknown psu_type (%d)!! while onlp_fani_status_get())", psu_type);
        return ONLP_STATUS_E_INTERNAL;
    }

    ONLP_TRY(get_cpld_fan_info(psu_type, local_id, &info));
    *rv = info.status;

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
    int psu_type = ONLP_PSU_TYPE_INVALID;

    ONLP_TRY(get_psu_type(&psu_type));
    ONLP_TRY(get_fan_local_id(psu_type, id, &local_id));

    if (psu_type == ONLP_PSU_TYPE_AC) {
        *hdr = ac_fan_info[local_id].hdr;
    } else if (psu_type == ONLP_PSU_TYPE_DC48) {
        *hdr = dc_fan_info[local_id].hdr;
    } else {
        AIM_LOG_ERROR("Unknown psu_type (%d)!! while onlp_fani_hdr_get())", psu_type);
        return ONLP_STATUS_E_INTERNAL;
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

/******************************************************************************************************************
**                                                                                                               **
**                                           Upispace Specific Defined APIs                                      **
**                                                                                                               **
*******************************************************************************************************************/
/**
 * @brief Get and check fan local ID
 * @param id [in] OID
 * @param local_id [out] The fan local id
 */
static int get_fan_local_id(int psu_type, int id, int *local_id)
{
    int tmp_id;
    board_t board = {0};

    if (local_id == NULL) {
        return ONLP_STATUS_E_PARAM;
    }

    if (!ONLP_OID_IS_FAN(id)) {
        return ONLP_STATUS_E_INVALID;
    }

    tmp_id = ONLP_OID_ID_GET(id);

    if (psu_type == ONLP_PSU_TYPE_AC) {
        switch (tmp_id) {
            case ONLP_FAN_0:
            case ONLP_FAN_1:
            case ONLP_FAN_2:
            case ONLP_PSU_0_FAN:
            case ONLP_PSU_1_FAN:
                *local_id = tmp_id;
                return ONLP_STATUS_OK;
            default:
                return ONLP_STATUS_E_INVALID;
        }
    } else if (psu_type == ONLP_PSU_TYPE_DC48) {
        switch (tmp_id) {
            case ONLP_FAN_0:
            case ONLP_FAN_1:
            case ONLP_FAN_2:
            case ONLP_FAN_3:
                *local_id = tmp_id;
                return ONLP_STATUS_OK;
            default:
                return ONLP_STATUS_E_INVALID;
        }
    } else {
        ONLP_TRY(get_board_version(&board));
        AIM_LOG_ERROR("Unknown psu_type (%d), hw_rev is (%d) while get_fan_local_id()", psu_type, board.hw_rev);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_E_INVALID;
}

/**
 * @brief Get the fan information from CPLD
 * @param id [in] FAN ID
 * @param info [out] The fan information
 */
static int get_cpld_fan_info(int psu_type, int local_id, onlp_fan_info_t* info)
{
    board_t board = {0};
    int sys_max_fan_speed = 0;
    int psu_max_fan_speed = 0;
    int rpm = 0, percentage = 0;

    unsigned int tmp_fan_rpm = 0, fan_rpm = 0;
    int pw_present;

    /* Present */

    if (psu_type == ONLP_PSU_TYPE_AC) {

        *info = ac_fan_info[local_id];

        switch (local_id) {
            case ONLP_FAN_0:
            case ONLP_FAN_1:
            case ONLP_FAN_2:
                info->status |= ONLP_FAN_STATUS_PRESENT;
                break;
            case ONLP_PSU_0_FAN:
            case ONLP_PSU_1_FAN:
            {
                int psu_id = (local_id == ONLP_PSU_0_FAN) ? ONLP_PSU_0 : ONLP_PSU_1;

                /* Get PSU present status*/
                ONLP_TRY(get_psu_present_status(psu_type, psu_id, &pw_present));
                if (pw_present != PSU_STATUS_PRES) {
                    info->status &= ~ONLP_FAN_STATUS_PRESENT;
                } else if (pw_present == PSU_STATUS_PRES) {
                    info->status |= ONLP_FAN_STATUS_PRESENT;
                } else {
                    AIM_LOG_ERROR("Unknown psu present status (%d)!! while get_cpld_fan_info()", pw_present);
                    return ONLP_STATUS_E_INTERNAL;
                }
                break;
            }
            default:
                return ONLP_STATUS_E_INVALID;
        }
    } else if (psu_type == ONLP_PSU_TYPE_DC48) {

        *info = dc_fan_info[local_id];

        switch (local_id) {
            case ONLP_FAN_0:
            case ONLP_FAN_1:
            case ONLP_FAN_2:
            case ONLP_FAN_3:
                info->status |= ONLP_FAN_STATUS_PRESENT;
                break;
            default:
                return ONLP_STATUS_E_INVALID;
        }
    } else {
        ONLP_TRY(get_board_version(&board));
        AIM_LOG_ERROR("Unknown psu_type (%d), hw_rev is (%d) while get_cpld_fan_info()", psu_type, board.hw_rev);
        return ONLP_STATUS_E_INTERNAL;
    }

    /* Direction */
    if (info->status & ONLP_FAN_STATUS_PRESENT) {
        info->status |= ONLP_FAN_STATUS_F2B;
        info->status &= ~ONLP_FAN_STATUS_B2F;
    } else {
        info->status &= ~ONLP_FAN_STATUS_F2B;
        info->status &= ~ONLP_FAN_STATUS_B2F;
    }

    /* Contents - get RPM */
    if (info->status & ONLP_FAN_STATUS_PRESENT) {
        /* get fan rpm */
        switch(local_id) {
            case ONLP_FAN_0:
            {
                int val = 0;
                char *str = SYSFS_CPLD2 "fan_0_pwm_rpm";
                ONLP_TRY(read_file_hex(&val, str));
                rpm = val;
                break;
            }
            case ONLP_FAN_1:
            {
                int val = 0;
                char *str = SYSFS_CPLD2 "fan_1_pwm_rpm";
                ONLP_TRY(read_file_hex(&val, str));
                rpm = val;
                break;
            }
            case ONLP_FAN_2:
            {
                int val = 0;
                char *str = SYSFS_CPLD2 "fan_2_pwm_rpm";
                ONLP_TRY(read_file_hex(&val, str));
                rpm = val;
                break;
            }
            case ONLP_FAN_3:
            {
                if (psu_type == ONLP_PSU_TYPE_AC) {
                    AIM_LOG_ERROR("psu_type (%d) is AC PSU, there is no fan 3 for AC PSU", psu_type);
                    return ONLP_STATUS_E_INVALID;
                }

                int val = 0;
                char *str = SYSFS_CPLD2 "fan_3_pwm_rpm";
                ONLP_TRY(read_file_hex(&val, str));
                rpm = val;
                break;
            }
            case ONLP_PSU_0_FAN:
            {
                if (psu_type == ONLP_PSU_TYPE_DC48) {
                    return ONLP_STATUS_E_INVALID;
                }

                tmp_fan_rpm = onlp_i2c_readw(I2C_BUS_PSU0, PSU0_ADDRESS, PMBUS_PSU_FAN_RPM, ONLP_I2C_F_FORCE);

                //pmbus linear data format
                fan_rpm = (unsigned int)tmp_fan_rpm;
                fan_rpm = (fan_rpm & 0x07FF) * (1 << ((fan_rpm >> 11) & 0x1F));
                break;
            }
            case ONLP_PSU_1_FAN:
            {
                if (psu_type == ONLP_PSU_TYPE_DC48) {
                    return ONLP_STATUS_E_INVALID;
                }

                tmp_fan_rpm = onlp_i2c_readw(I2C_BUS_PSU1, PSU1_ADDRESS, PMBUS_PSU_FAN_RPM, ONLP_I2C_F_FORCE);

                //pmbus linear data format
                fan_rpm = (unsigned int)tmp_fan_rpm;
                fan_rpm = (fan_rpm & 0x07FF) * (1 << ((fan_rpm >> 11) & 0x1F));
                break;
            }
            default:
                return ONLP_STATUS_E_INVALID;
        }

        switch(local_id) {
            case ONLP_FAN_0:
            case ONLP_FAN_1:
            case ONLP_FAN_2:
            case ONLP_FAN_3:
            {
                /* set rpm field */
                info->rpm = rpm;

                if (local_id == ONLP_FAN_3) {
                    sys_max_fan_speed = SYS_FAN3_FRONT_MAX_RPM;
                } else {
                    sys_max_fan_speed = SYS_FAN_0_2_FRONT_MAX_RPM;
                }

                percentage = (info->rpm*100)/sys_max_fan_speed;
                info->percentage = (percentage >= 100) ? 100:percentage;
                info->status |= (rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;
                break;
            }
            case ONLP_PSU_0_FAN:
            case ONLP_PSU_1_FAN:
            {
                if (psu_type == ONLP_PSU_TYPE_AC) {
                    psu_max_fan_speed = PSU_FAN_RPM_MAX_AC;
                } else {
                    AIM_LOG_ERROR("psu_type (%d) is DC PSU, there is no PSU fan", psu_type);
                    return ONLP_STATUS_E_INVALID;
                }

                /* set rpm field */
                info->rpm = (int)fan_rpm;

                percentage = (info->rpm*100)/psu_max_fan_speed;
                info->percentage = (percentage >= 100) ? 100 : percentage;
                info->status |= (fan_rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;
                break;
            }
            default:
                return ONLP_STATUS_E_INVALID;
        }
    }

    return ONLP_STATUS_OK;
}
