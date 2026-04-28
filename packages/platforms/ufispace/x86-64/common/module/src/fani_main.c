/************************************************************
 * Copyright (C) 2026 Ufispace Technology Corporation.
 ************************************************************
 *
 * Fan Platform Implementation.
 *
 ***********************************************************/
#include <onlp/platformi/fani.h>
#include <ufispace_platform/platform_lib.h>
#include <ufispace_common/platform_lib_main.h>
#include <ufispace_common/fani_main.h>

static int fan_total = 0;
static fan_elems *fans = NULL;

/**
  * @brief Get fan sensor total count.
  */
int _onlp_fan_total_get(int *total)
{
    int rv = ONLP_STATUS_OK;

    if(!total) {
        return ONLP_STATUS_E_PARAM;
    }

    rv = ufi_file_read_int(total, "/sys_switch/slot/slot1/num_components/num_fans");
    if(rv != ONLP_STATUS_OK || *total < 0) {
        *total = 0;
    }
    return rv;
}

/**
  * @brief Get the fan entry.
  */
int __WEAK _onlp_fan_entry_get(int logical_id, fan_elems *entry)
{
    if(!entry || logical_id > fan_total || !fans[logical_id].valid) {
        return ONLP_STATUS_E_INVALID;
    } else {
        *entry = fans[logical_id];
        return ONLP_STATUS_OK;
    }
}

/**
  * @brief Initialize the fan platform subsystem.
  */
int __WEAK onlp_fani_init(void)
{
    int count = 0;
    int oid_base = 0;
    int rv = ONLP_STATUS_OK;
    int i = 1;
    int fan_count = 0;

    if (fans != NULL) {
        return ONLP_STATUS_OK;
    }

    lock_init();

    ONLP_TRY(_onlp_fan_total_get(&fan_total));
    ONLP_TRY(_onlp_fan_oid_base_get(&oid_base));
    fans = (fan_elems *) aim_zmalloc(sizeof(fan_elems)*(fan_total));
    if (fans == NULL) {
        AIM_LOG_ERROR("Failed to allocate memory for fan elements");
        return ONLP_STATUS_E_INTERNAL;
    }

    // scan fan tray
    rv = ufi_file_read_int(&count, "/sys_switch/fan/number");
    if(rv == ONLP_STATUS_OK && count > 0) {
        for(i=1; i <= count; i++) {
            int motor_number = 0;
            int rv2 = ufi_file_read_int(&motor_number, "/sys_switch/fan/fan%d/motor_number", i);
            if(rv2 == ONLP_STATUS_OK) {
                int j = 1;
                for(j=1; j <= motor_number; j++) {
                    int oid = 0;
                    int logical_id = 0;
                    char *description = NULL;
                    int len1 = onlp_file_read_str(&description, "/sys_switch/fan/fan%d/motor%d/description", i, j);
                    
                    ++fan_count;
                    logical_id = fan_count - 1;
                    oid = oid_base + logical_id;
                    fans[logical_id].id = ONLP_FAN_ID_CREATE(oid);
                    fans[logical_id].type = TYPE_FANTRAY;
                    fans[logical_id].parent = i;
                    fans[logical_id].child = j;

                    if(!description || !len1) {
                        aim_free(description);
                        fans[logical_id].valid = 0;
                        continue;
                    }

                    if(!strncmp(description, COMM_STR_NA, sizeof(COMM_STR_NA))) {
                        aim_free(description);
                        fans[logical_id].valid = 0;
                        continue;
                    }

                    aim_free(description);
                    fans[logical_id].valid = 1;
                }
            }
        }
    }

    // scan psu
    rv = ufi_file_read_int(&count, "/sys_switch/psu/number");
    if(rv == ONLP_STATUS_OK) {
        for(i=1; i <= count; i++) {
            int oid = 0;
            int logical_id = 0;
            char *description = NULL;
            int rv2 = ONLP_STATUS_OK;
            int len1 = onlp_file_read_str(&description, "/sys_switch/psu/psu%d/description", i);

            ++fan_count;
            logical_id = (fan_count - 1);
            oid = oid_base + logical_id;
            fans[logical_id].id = ONLP_FAN_ID_CREATE(oid);
            fans[logical_id].type = TYPE_PSU;
            fans[logical_id].parent = i;

            if(!description || !len1) {
                aim_free(description);
                fans[logical_id].valid = 0;
                continue;
            }

            if(!strncmp(description, COMM_STR_NA, sizeof(COMM_STR_NA))) {
                aim_free(description);
                fans[logical_id].valid = 0;
                continue;
            }

            aim_free(description);

            rv2 = onlp_file_size("/sys_switch/psu/psu%d/fan_speed", i);
            if(rv2 < 0) {
                fans[logical_id].valid = 0;
                continue;
            }

            fans[logical_id].valid = 1;
        }
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the information structure for the given fan OID.
 * @param id The fan OID
 * @param rv [out] Receives the fan information.
 */
int __WEAK onlp_fani_info_get(onlp_oid_t id, onlp_fan_info_t* info)
{
    uint32_t logical_id = 0;
    fan_elems entry = {0};
    int base = 0;
    int presence = 0;
    int speed = 0;
    int percentage = 0;
    int speed_max = 0;
    int psu_type = 0;
    int dir = ONLP_FAN_STATUS_F2B;
    int len1 = 0;
    char *buf = NULL;

    ONLP_TRY(onlp_fani_init());

    if(!info) {
        return ONLP_STATUS_E_INVALID;
    } else {
        memset(info, 0, sizeof(onlp_fan_info_t));
    }

    if(!ONLP_OID_IS_FAN(id)) {
        return ONLP_STATUS_E_INVALID;
    }

    ONLP_TRY(_onlp_fan_oid_base_get(&base));
    logical_id = ONLP_OID_ID_GET(id) - base;
    ONLP_TRY(_onlp_fan_entry_get(logical_id, &entry));

    info->hdr.id = entry.id;
    info->caps = (ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE);
    info->mode = ONLP_FAN_MODE_INVALID;

    snprintf(info->model, sizeof(info->model), "%s", COMM_STR_NOT_SUPPORTED);
    snprintf(info->serial, sizeof(info->serial), "%s", COMM_STR_NOT_SUPPORTED);

    if(IS_FANTRAY(entry.type)) {
        len1 = onlp_file_read_str(&buf, "/sys_switch/fan/fan%d/motor%d/description",
                entry.parent, entry.child);
        if(!!buf && !!len1) {
            snprintf(info->hdr.description, sizeof(info->hdr.description), "%s", buf);
        }
        aim_free(buf);
        buf = NULL;

        ONLP_TRY(ufi_file_read_int(&presence, "/sys_switch/fan/fan%d/status",
            entry.parent));
        ONLP_TRY(ufi_file_read_int(&speed, "/sys_switch/fan/fan%d/motor%d/speed",
            entry.parent, entry.child));
        ONLP_TRY(ufi_file_read_int(&dir, "/sys_switch/fan/fan%d/motor%d/direction",
            entry.parent, entry.child));
        ONLP_TRY(ufi_file_read_int(&speed_max, "/sys_switch/fan/fan%d/motor%d/speed_max",
            entry.parent, entry.child));

        if(presence) {
            memset(info->model, 0, sizeof(info->model));
            memset(info->serial, 0, sizeof(info->serial));
            len1 = onlp_file_read_str(&buf, "/sys_switch/fan/fan%d/model_name",
                    entry.parent);
            if(!!buf && !!len1 && !!strncmp(buf, COMM_STR_NA, sizeof(COMM_STR_NA))) {
                snprintf(info->model, sizeof(info->model), "%s", buf);
            } else {
                snprintf(info->model, sizeof(info->model), "%s", COMM_STR_NOT_AVAILABLE);
            }
            aim_free(buf);
            buf = NULL;

            len1 = onlp_file_read_str(&buf, "/sys_switch/fan/fan%d/serial_number",
                    entry.parent);
            if(!!buf && !!len1 && !!strncmp(buf, COMM_STR_NA, sizeof(COMM_STR_NA))) {
                snprintf(info->serial, sizeof(info->serial), "%s", buf);
            } else {
                snprintf(info->serial, sizeof(info->serial), "%s", COMM_STR_NOT_AVAILABLE);
            }
            aim_free(buf);
            buf = NULL;
        }

    } else if(IS_PSU(entry.type)) {
        len1 = onlp_file_read_str(&buf, "/sys_switch/psu/psu%d/description",
                entry.parent);
        if(!!buf && !!len1) {
            snprintf(info->hdr.description, sizeof(info->hdr.description), "%s FAN", buf);
        }
        aim_free(buf);
        buf = NULL;

        ONLP_TRY(ufi_file_read_int(&presence, "/sys_switch/psu/psu%d/present",
            entry.parent));
        ONLP_TRY(ufi_file_read_int(&speed, "/sys_switch/psu/psu%d/fan_speed",
            entry.parent));
        ONLP_TRY(ufi_file_read_int(&dir, "/sys_switch/psu/psu%d/fan_direction",
            entry.parent));
        ONLP_TRY(ufi_file_read_int(&psu_type, "/sys_switch/psu/psu%d/type",
            entry.parent));
        if(psu_type == 1) {
            ONLP_TRY(ufi_file_read_int(&speed_max, "/sys_switch/psu/psu%d/fan_speed_ac_max",
                entry.parent));
        } else if(psu_type == 0) {
            ONLP_TRY(ufi_file_read_int(&speed_max, "/sys_switch/psu/psu%d/fan_speed_dc_max",
                entry.parent));
        } else {
            ONLP_TRY(ufi_file_read_int(&speed_max, "/sys_switch/psu/psu%d/fan_speed_ac_max",
                entry.parent));
        }
    } else {
        return ONLP_STATUS_E_INVALID;
    }

    if(presence) {
        percentage = (speed * 100) / speed_max;
        info->rpm = speed;
        info->percentage = (percentage >= 100) ? 100:percentage;
        info->status |= ONLP_FAN_STATUS_PRESENT;
        info->status |= (speed == 0) ? ONLP_FAN_STATUS_FAILED : 0;
    } else {
        info->status &= ~ONLP_FAN_STATUS_PRESENT;
    }

    if(dir == ONLP_FAN_STATUS_B2F) {
        info->status |= ONLP_FAN_STATUS_B2F;
        info->status &= ~ONLP_FAN_STATUS_F2B;
    } else {
        info->status |= ONLP_FAN_STATUS_F2B;
        info->status &= ~ONLP_FAN_STATUS_B2F;
    }
    return ONLP_STATUS_OK;
}

/**
 * @brief Retrieve the fan's operational status.
 * @param id The fan OID.
 * @param rv [out] Receives the fan's operations status flags.
 * @notes Only operational state needs to be returned -
 *        PRESENT/FAILED
 */
int __WEAK onlp_fani_status_get(onlp_oid_t id, uint32_t* rv)
{
    onlp_fan_info_t info = {0};

    ONLP_TRY(onlp_fani_info_get(id, &info));
    *rv = info.status;
    return ONLP_STATUS_OK;
}

/**
 * @brief Retrieve the fan's OID hdr.
 * @param id The fan OID.
 * @param rv [out] Receives the OID header.
 */
int __WEAK onlp_fani_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* hdr)
{
    onlp_fan_info_t info = {0};

    ONLP_TRY(onlp_fani_info_get(id, &info));
    *hdr = info.hdr;
    return ONLP_STATUS_OK;
}

/**
 * @brief Set the fan speed in RPM.
 * @param id The fan OID
 * @param rpm The new RPM
 * @note This is only relevant if the RPM capability is set.
 */
int __WEAK onlp_fani_rpm_set(onlp_oid_t id, int rpm)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}


/**
 * @brief Set the fan speed in percentage.
 * @param id The fan OID.
 * @param p The new fan speed percentage.
 * @note This is only relevant if the PERCENTAGE capability is set.
 */
int __WEAK onlp_fani_percentage_set(onlp_oid_t id, int p)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Set the fan mode.
 * @param id The fan OID.
 * @param mode The new fan mode.
 */
int __WEAK onlp_fani_mode_set(onlp_oid_t id, onlp_fan_mode_t mode)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Set the fan direction (if supported).
 * @param id The fan OID
 * @param dir The direction.
 */
int __WEAK onlp_fani_dir_set(onlp_oid_t id, onlp_fan_dir_t dir)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Generic fan ioctl
 * @param id The fan OID
 * @param vargs The variable argument list for the ioctl call.
 * @param Optional
 */
int __WEAK onlp_fani_ioctl(onlp_oid_t id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

