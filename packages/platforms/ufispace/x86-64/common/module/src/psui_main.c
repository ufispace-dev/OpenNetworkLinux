/************************************************************
 * Copyright (C) 2026 Ufispace Technology Corporation.
 ************************************************************
 *
 * Power Supply Management Implementation.
 *
 ***********************************************************/
#include <onlp/platformi/psui.h>
#include <ufispace_platform/platform_lib.h>
#include <ufispace_common/platform_lib_main.h>
#include <ufispace_common/psui_main.h>

static int psu_total = 0;
static psu_elems *psus = NULL;

/**
  * @brief Get PSU fan position offset.
  */
int _onlp_psu_fan_pos_off_get(int *off)
{
    int rv = ONLP_STATUS_OK;
    int oid_base = 0;
    int motor_number = 0;
    int fan_count = 0;
    int motor_count = 0;
    int i = 1;
    *off = 0;

    ONLP_TRY(_onlp_fan_oid_base_get(&oid_base));
    rv = ufi_file_read_int(&fan_count, "/sys_switch/fan/number");
    if(rv == ONLP_STATUS_OK) {
        for(i=1; i <= fan_count; i++) {
            rv = ufi_file_read_int(&motor_number,
                "/sys_switch/fan/fan%d/motor_number", i);
            if( rv == ONLP_STATUS_OK) {
                motor_count += motor_number;
            } else {
                *off = 0;
                return rv;
            }
        }

        *off = oid_base + (motor_count - 1);
    }
    return rv;
}

/**
  * @brief Get PSU thermal position offset.
  */
int _onlp_psu_thermal_pos_off_get(int *off)
{
    int rv = ONLP_STATUS_OK;
    int oid_base = 0;
    int temp_count = 0;

    ONLP_TRY(_onlp_temp_oid_base_get(&oid_base));
    rv = ufi_file_read_int(&temp_count, "/sys_switch/temp_sensor/number");
    if( rv != ONLP_STATUS_OK) {
        *off = 0;
    }

    *off = oid_base + (temp_count - 1);
    return rv;
}

/**
  * @brief Get PSU components total count.
  */
int _onlp_psu_total_get(int *total)
{
    int rv = ONLP_STATUS_OK;

    if(!total) {
        return ONLP_STATUS_E_PARAM;
    }

    rv = ufi_file_read_int(total, "/sys_switch/slot/slot1/num_components/num_psus");
    if(rv != ONLP_STATUS_OK || *total < 0) {
        *total = 0;
    }
    return rv;
}

/**
  * @brief Get the psu entry.
  */
int __WEAK _onlp_psu_entry_get(int logical_id, psu_elems *entry)
{
    if(!entry || logical_id > psu_total || !psus[logical_id].valid) {
        return ONLP_STATUS_E_INVALID;
    } else {
        *entry = psus[logical_id];
        return ONLP_STATUS_OK;
    }
}

/**
 * @brief Initialize the PSU subsystem.
 */
int __WEAK onlp_psui_init(void)
{
    int count = 0;
    int oid_base = 0;
    int rv = ONLP_STATUS_OK;
    int i = 1;
    int psu_count = 0;
    int psu_fan_count = 0;
    int psu_temp_count = 0;
    int psu_fan_pos_off = 0;
    int psu_thermal_pos_off = 0;

    if (psus != NULL) {
        return ONLP_STATUS_OK;
    }

    lock_init();

    ONLP_TRY(_onlp_psu_fan_pos_off_get(&psu_fan_pos_off));
    ONLP_TRY(_onlp_psu_thermal_pos_off_get(&psu_thermal_pos_off));
    ONLP_TRY(_onlp_psu_total_get(&psu_total));
    ONLP_TRY(_onlp_psu_oid_base_get(&oid_base));
    psus = (psu_elems *) aim_zmalloc(sizeof(psu_elems)*(psu_total));
    if (psus == NULL) {
        AIM_LOG_ERROR("Failed to allocate memory for PSU elements");
        return ONLP_STATUS_E_INTERNAL;
    }

    // scan psu
    rv = ufi_file_read_int(&count, "/sys_switch/psu/number");
    if(rv == ONLP_STATUS_OK && count > 0) {
        for(i=1; i <= count; i++) {
            int j = 1;
            int oid = 0;
            int logical_id = 0;
            char *description = NULL;
            int temp_number = 0;
            int len1 = onlp_file_read_str(&description, "/sys_switch/psu/psu%d/description", i);

            ++psu_count;
            logical_id = psu_count - 1;
            oid = oid_base + logical_id;
            psus[logical_id].id = ONLP_PSU_ID_CREATE(oid);
            psus[logical_id].parent = i;
            if(!description || !len1) {
                aim_free(description);
                psus[logical_id].valid = 0;
                continue;
            }

            if(!strncmp(description, COMM_STR_NA, sizeof(COMM_STR_NA))) {
                aim_free(description);
                psus[logical_id].valid = 0;
                continue;
            }

            aim_free(description);

            rv = onlp_file_size("/sys_switch/psu/psu%d/fan_speed", i);
            if(rv >= 0) {
                int fan_oid = 0;
                psus[logical_id].fan_num = 1;
                ++psu_fan_count;
                fan_oid = psu_fan_pos_off + psu_fan_count;
                psus[logical_id].fan_id = ONLP_FAN_ID_CREATE(fan_oid);
            }

            rv = ufi_file_read_int(&temp_number, "/sys_switch/psu/psu%d/num_temp_sensors", i);
            if(rv == ONLP_STATUS_OK) {
                psus[logical_id].temp_num = temp_number > PSU_TEMP_MAX ? PSU_TEMP_MAX:temp_number;
                for(j=1; j <= psus[logical_id].temp_num; j++) {
                    int temp_oid = 0;
                    int thermal_id_idx = j-1;
                    char *description2 = NULL;
                    int len2 = onlp_file_read_str(&description2, "/sys_switch/psu/psu%d/temp%d/description", i, j);

                    ++psu_temp_count;
                    temp_oid = psu_thermal_pos_off + psu_temp_count;
                    if(!description2 || !len2) {
                        aim_free(description2);
                        continue;
                    }

                    if(!strncmp(description2, COMM_STR_NA, sizeof(COMM_STR_NA))) {
                        aim_free(description2);
                        continue;
                    }

                    aim_free(description2);

                    psus[logical_id].thermal_id[thermal_id_idx] = ONLP_THERMAL_ID_CREATE(temp_oid);
                }
            }

            psus[logical_id].valid = 1;
        }
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the information structure for the given PSU
 * @param id The PSU OID
 * @param rv [out] Receives the PSU information.
 */
int __WEAK onlp_psui_info_get(onlp_oid_t id, onlp_psu_info_t* info)
{
    uint32_t logical_id = 0;
    psu_elems entry = {0};
    int base = 0;
    int rv = ONLP_STATUS_OK;
    int coid_idx = 0;
    int i = 0;
    int len1 = 0;
    char *buf = NULL;
    int presence = 0;
    int pw_good = 0;
    int mvin = 0;
    int mvout = 0;
    int miin = 0;
    int miout = 0;
    int mstbvout = 0;
    int mstbiout = 0;
    int type = 0;
    long long int upin = 0;
    long long int upout = 0;

    ONLP_TRY(onlp_psui_init());

    if(!info) {
        return ONLP_STATUS_E_INVALID;
    } else {
        memset(info, 0, sizeof(onlp_psu_info_t));
    }

    if(!ONLP_OID_IS_PSU(id)) {
        return ONLP_STATUS_E_INVALID;
    }

    ONLP_TRY(_onlp_psu_oid_base_get(&base));
    logical_id = ONLP_OID_ID_GET(id) - base;
    ONLP_TRY(_onlp_psu_entry_get(logical_id, &entry));

    info->hdr.id = entry.id;

    for(i = 0; i < entry.fan_num;i++) {
        info->hdr.coids[coid_idx++] = entry.fan_id;
    }

    for(i = 0; i < entry.temp_num;i++) {
        info->hdr.coids[coid_idx++] = entry.thermal_id[i];
    }

    snprintf(info->model, sizeof(info->model), "%s", COMM_STR_NOT_SUPPORTED);
    snprintf(info->serial, sizeof(info->serial), "%s", COMM_STR_NOT_SUPPORTED);

    len1 = onlp_file_read_str(&buf, "/sys_switch/psu/psu%d/description",
            entry.parent);
    if(!!buf && !!len1) {
        snprintf(info->hdr.description, sizeof(info->hdr.description), "%s", buf);
    }
    aim_free(buf);
    buf = NULL;

    rv = ufi_file_read_int(&presence, "/sys_switch/psu/psu%d/present",
            entry.parent);
    presence = (rv == ONLP_STATUS_OK) ? presence:0;

    if(presence) {
        info->status |= ONLP_PSU_STATUS_PRESENT;

        rv = ufi_file_read_int(&pw_good, "/sys_switch/psu/psu%d/out_status",
                entry.parent);
        pw_good = (rv == ONLP_STATUS_OK) ? pw_good:0;

        if (pw_good) {
            info->status &= ~ONLP_PSU_STATUS_FAILED;
        } else {
            info->status |= ONLP_PSU_STATUS_FAILED;
        }

        rv = ufi_file_read_int(&type, "/sys_switch/psu/psu%d/type",
                entry.parent);

        if(rv ==  ONLP_STATUS_OK) {
             if(type == 1) {
                info->caps |= ONLP_PSU_CAPS_AC;
                info->caps &= ~ONLP_PSU_CAPS_DC48;
             } else {
                info->caps &= ~ONLP_PSU_CAPS_AC;
                info->caps |= ONLP_PSU_CAPS_DC48;
             }
            info->mpin = 1000 * upin;
            info->caps |= ONLP_PSU_CAPS_PIN;
        }

        rv = ufi_file_read_int(&mvin, "/sys_switch/psu/psu%d/in_vol",
            entry.parent);

        if(rv ==  ONLP_STATUS_OK) {
            info->mvin = mvin;
            info->caps |= ONLP_PSU_CAPS_VIN;
        }

        rv = ufi_file_read_int(&mvout, "/sys_switch/psu/psu%d/out_vol",
            entry.parent);

        if(rv ==  ONLP_STATUS_OK) {
            info->mvout = mvout;
            info->caps |= ONLP_PSU_CAPS_VOUT;
        }

        rv = ufi_file_read_int(&miin, "/sys_switch/psu/psu%d/in_curr",
            entry.parent);

        if(rv ==  ONLP_STATUS_OK) {
            info->miin = miin;
            info->caps |= ONLP_PSU_CAPS_IIN;
        }

        rv = ufi_file_read_int(&miout, "/sys_switch/psu/psu%d/out_curr",
            entry.parent);

        if(rv ==  ONLP_STATUS_OK) {
            info->miout = miout;
            info->caps |= ONLP_PSU_CAPS_IOUT;
        }

        rv = ufi_file_read_longlong(&upin, "/sys_switch/psu/psu%d/in_power",
            entry.parent);
        if(rv ==  ONLP_STATUS_OK) {
            info->mpin = upin / 1000;
            info->caps |= ONLP_PSU_CAPS_PIN;
        } else {
            if(info->caps & ONLP_PSU_CAPS_VIN && info->caps & ONLP_PSU_CAPS_IIN) {
                info->mpin = mvin / 1000 * miin;
                info->caps |= ONLP_PSU_CAPS_PIN;
            }
        }

        rv = ufi_file_read_longlong(&upout, "/sys_switch/psu/psu%d/out_power",
            entry.parent);

        if(rv !=  ONLP_STATUS_OK) {
            int rv1 = ONLP_STATUS_OK;
            int rv2 = ONLP_STATUS_OK;
            rv1 = ufi_file_read_int(&mstbvout, "/sys_switch/psu/psu%d/stbout_vol",
                entry.parent);

            rv2 = ufi_file_read_int(&mstbiout, "/sys_switch/psu/psu%d/stbout_curr",
                entry.parent);

            if(rv1 == ONLP_STATUS_OK && rv2 == ONLP_STATUS_OK) {
                info->mpout =  mvout * miout / 1000 + mstbvout * mstbiout / 1000;
                info->caps |= ONLP_PSU_CAPS_POUT;
            } else if(info->caps & ONLP_PSU_CAPS_VOUT && info->caps & ONLP_PSU_CAPS_IOUT) {
                info->mpout = mvout / 1000 * miout;
                info->caps |= ONLP_PSU_CAPS_POUT;
            }
        } else {
            info->mpout = upout / 1000;
            info->caps |= ONLP_PSU_CAPS_POUT;
        }

        memset(info->model, 0, sizeof(info->model));
        memset(info->serial, 0, sizeof(info->serial));
        len1 = onlp_file_read_str(&buf, "/sys_switch/psu/psu%d/model_name",
                entry.parent);
        if(!!buf && !!len1 && !!strncmp(buf, COMM_STR_NA, sizeof(COMM_STR_NA))) {
            snprintf(info->model, sizeof(info->model), "%s", buf);
        } else {
            snprintf(info->model, sizeof(info->model), "%s", COMM_STR_NOT_AVAILABLE);
        }
        aim_free(buf);
        buf = NULL;

        len1 = onlp_file_read_str(&buf, "/sys_switch/psu/psu%d/serial_number",
                entry.parent);
        if(!!buf && !!len1 && !!strncmp(buf, COMM_STR_NA, sizeof(COMM_STR_NA))) {
            snprintf(info->serial, sizeof(info->serial), "%s", buf);
        } else {
            snprintf(info->serial, sizeof(info->serial), "%s", COMM_STR_NOT_AVAILABLE);
        }
        aim_free(buf);
        buf = NULL;
    } else {
        info->status &= ~ONLP_PSU_STATUS_PRESENT;
        info->status |=  ONLP_PSU_STATUS_UNPLUGGED;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the PSU's operational status.
 * @param id The PSU OID.
 * @param rv [out] Receives the operational status.
 */
int __WEAK onlp_psui_status_get(onlp_oid_t id, uint32_t* rv)
{
    onlp_psu_info_t info = {0};

    ONLP_TRY(onlp_psui_info_get(id, &info));
    *rv = info.status;
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the PSU's oid header.
 * @param id The PSU OID.
 * @param rv [out] Receives the header.
 */
int __WEAK onlp_psui_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* hdr)
{
    onlp_psu_info_t info = {0};

    ONLP_TRY(onlp_psui_info_get(id, &info));
    *hdr = info.hdr;
    return ONLP_STATUS_OK;
}

/**
 * @brief Generic PSU ioctl
 * @param id The PSU OID
 * @param vargs The variable argument list for the ioctl call.
 */
int __WEAK onlp_psui_ioctl(onlp_oid_t pid, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

