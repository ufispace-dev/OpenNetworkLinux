/************************************************************
 * Copyright (C) 2026 Ufispace Technology Corporation.
 ************************************************************
 *
 * Thermal Sensor Platform Implementation.
 *
 ***********************************************************/
#include <onlp/platformi/thermali.h>
#include <ufispace_platform/platform_lib.h>
#include <ufispace_common/platform_lib_main.h>
#include <ufispace_common/thermali_main.h>

static int temp_total = 0;
static temp_elems *temps = NULL;

/**
 * @brief Get temp sensor total count.
 */
int _onlp_temp_total_get(int *total)
{
    int rv = ONLP_STATUS_OK;

    if(!total) {
        return ONLP_STATUS_E_PARAM;
    }

    rv = ufi_file_read_int(total, "/sys_switch/slot/slot1/num_components/num_temp_sensors");
    if(rv != ONLP_STATUS_OK || *total < 0) {
        *total = 0;
    }
    return rv;
}

/**
  * @brief Get the temp entry.
  */
int __WEAK _onlp_temp_entry_get(int logical_id, temp_elems *entry)
{
    if(!entry || logical_id > temp_total || !temps[logical_id].valid) {
        return ONLP_STATUS_E_INVALID;
    } else {
        *entry = temps[logical_id];
        return ONLP_STATUS_OK;
    }
}

/**
 * @brief Initialize the thermal subsystem.
 */
int __WEAK onlp_thermali_init(void)
{
    int count = 0;
    int oid_base = 0;
    int rv = ONLP_STATUS_OK;
    int i = 1;
    int temp_count = 0;

    if (temps != NULL) {
        return ONLP_STATUS_OK;
    }

    lock_init();
    ONLP_TRY(_onlp_temp_total_get(&temp_total));
    ONLP_TRY(_onlp_temp_oid_base_get(&oid_base));
    temps = (temp_elems *) aim_zmalloc(sizeof(temp_elems)*(temp_total));
    if (temps == NULL) {
        AIM_LOG_ERROR("Failed to allocate memory for temp elements");
        return ONLP_STATUS_E_INTERNAL;
    }

    // scan thermal sensor
    rv = ufi_file_read_int(&count, "/sys_switch/temp_sensor/number");
    if(rv == ONLP_STATUS_OK) {
        for(i=1; i <= count; i++) {
            int oid = 0;
            int logical_id = 0;
            char *description = NULL;
            int len1 = onlp_file_read_str(&description,
                        "/sys_switch/temp_sensor/temp%d/description", i);

            ++temp_count;
            logical_id = temp_count - 1;
            oid = oid_base + logical_id;
            temps[logical_id].id = ONLP_THERMAL_ID_CREATE(oid);
            temps[logical_id].type = TYPE_THERMAL;
            temps[logical_id].parent = i;

            if(!description || !len1) {
                aim_free(description);
                temps[logical_id].valid = 0;
                continue;
            }

            if(!strncmp(description, COMM_STR_NA, sizeof(COMM_STR_NA))) {
                aim_free(description);
                temps[logical_id].valid = 0;
                continue;
            }

            aim_free(description);
            temps[logical_id].valid = 1;
        }
    }

    // scan psu
    rv = ufi_file_read_int(&count, "/sys_switch/psu/number");
    if(rv == ONLP_STATUS_OK) {
        for(i=1; i <= count; i++) {
            int temp_number = 0;
            int rv2 = ufi_file_read_int(&temp_number,
                        "/sys_switch/psu/psu%d/num_temp_sensors", i);
            if(rv2 == ONLP_STATUS_OK) {
                int j = 1;
                for(j=1; j <= temp_number; j++) {
                    int oid = 0;
                    int logical_id = 0;
                    char *description = NULL;
                    int len1 = onlp_file_read_str(&description,
                                "/sys_switch/psu/psu%d/temp%d/description", i, j);

                    ++temp_count;
                    logical_id = temp_count - 1;
                    oid = oid_base + logical_id;
                    temps[logical_id].id = ONLP_THERMAL_ID_CREATE(oid);
                    temps[logical_id].type = TYPE_PSU;
                    temps[logical_id].parent = i;
                    temps[logical_id].child = j;

                    if(!description || !len1) {
                        aim_free(description);
                        temps[logical_id].valid = 0;
                        continue;
                    }

                    if(!strncmp(description, COMM_STR_NA, sizeof(COMM_STR_NA))) {
                        aim_free(description);
                        temps[logical_id].valid = 0;
                        continue;
                    }

                    aim_free(description);
                    temps[logical_id].valid = 1;
                }
            }
        }
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the information for the given thermal OID.
 * @param id The Thermal OID
 * @param rv [out] Receives the thermal information.
 */
int __WEAK onlp_thermali_info_get(onlp_oid_t id, onlp_thermal_info_t* info)
{
    uint32_t logical_id = 0;
    temp_elems entry = {0};
    int base = 0;
    int rv = ONLP_STATUS_OK;
    int presence = 0;
    int val = 0;
    int len1 = 0;
    char *buf = NULL;

    ONLP_TRY(onlp_thermali_init());

    if(!info) {
        return ONLP_STATUS_E_INVALID;
    } else {
        memset(info, 0, sizeof(onlp_thermal_info_t));
    }
    if(!ONLP_OID_IS_THERMAL(id)) {
        return ONLP_STATUS_E_INVALID;
    }

    ONLP_TRY(_onlp_temp_oid_base_get(&base));
    logical_id = ONLP_OID_ID_GET(id) - base;
    ONLP_TRY(_onlp_temp_entry_get(logical_id, &entry));

    info->hdr.id = entry.id;
    info->caps |= ONLP_THERMAL_CAPS_GET_TEMPERATURE;

    if(IS_THERMAL(entry.type)) {
        info->status |= ONLP_THERMAL_STATUS_PRESENT;

        len1 = onlp_file_read_str(&buf,
                "/sys_switch/temp_sensor/temp%d/description",
                entry.parent);
        if(!!buf && !!len1) {
            snprintf(info->hdr.description, sizeof(info->hdr.description),
                "%s", buf);
        }
        aim_free(buf);
        buf = NULL;


        if(info->status & ONLP_THERMAL_STATUS_PRESENT) {
            rv = ufi_file_read_int(&val, "/sys_switch/temp_sensor/temp%d/value",
                entry.parent);

            if(rv == ONLP_STATUS_OK) {
                info->status &= ~ONLP_THERMAL_STATUS_FAILED;
                info->mcelsius = val;
            } else if(rv == ONLP_STATUS_E_UNSUPPORTED) {
                info->status &= ~ONLP_THERMAL_STATUS_PRESENT;
                info->status &= ~ONLP_THERMAL_STATUS_FAILED;
                info->mcelsius = 0;
            } else {
                info->status |= ONLP_THERMAL_STATUS_FAILED;
                info->mcelsius = 0;
            }

            rv = ufi_file_read_int(&val,
                "/sys_switch/temp_sensor/temp%d/max_shutdown_threshold",
                entry.parent);

            if(rv == ONLP_STATUS_OK) {
                info->caps |= ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD;
                info->thresholds.shutdown = val;
            } else {
                info->caps &= ~ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD;
                info->thresholds.shutdown = 0;
            }

            rv = ufi_file_read_int(&val, "/sys_switch/temp_sensor/temp%d/max_error_threshold",
                entry.parent);

            if(rv == ONLP_STATUS_OK) {
                info->caps |= ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD;
                info->thresholds.error = val;
            } else {
                info->caps &= ~ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD;
                info->thresholds.error = 0;
            }

            rv = ufi_file_read_int(&val, "/sys_switch/temp_sensor/temp%d/max_warning_threshold",
                entry.parent);

            if(rv == ONLP_STATUS_OK) {
                info->caps |= ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD;
                info->thresholds.warning = val;
            } else {
                info->caps &= ~ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD;
                info->thresholds.warning = 0;
            }
        }
    } else if(IS_PSU(entry.type)) {
        rv = ufi_file_read_int(&presence, "/sys_switch/psu/psu%d/present",
            entry.parent);

        if(presence == 1) {
            info->status |= ONLP_THERMAL_STATUS_PRESENT;
        } else {
            info->status &= ~ONLP_THERMAL_STATUS_PRESENT;
        }


        len1 = onlp_file_read_str(&buf, "/sys_switch/psu/psu%d/temp%d/description",
            entry.parent, entry.child);
        if(!!buf && !!len1) {
            snprintf(info->hdr.description, sizeof(info->hdr.description), "%s", buf);
        }
        aim_free(buf);
        buf = NULL;

        if(info->status & ONLP_THERMAL_STATUS_PRESENT) {
            rv = ufi_file_read_int(&val, "/sys_switch/psu/psu%d/temp%d/value",
                entry.parent, entry.child);

            if(rv == ONLP_STATUS_OK) {
                info->status &= ~ONLP_THERMAL_STATUS_FAILED;
                info->mcelsius = val;
            } else {
                info->status |= ONLP_THERMAL_STATUS_FAILED;
                info->mcelsius = 0;
            }

            rv = ufi_file_read_int(&val, "/sys_switch/psu/psu%d/temp%d/max_shutdown_threshold",
                entry.parent, entry.child);

            if(rv == ONLP_STATUS_OK) {
                info->caps |= ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD;
                info->thresholds.shutdown = val;
            } else {
                info->caps &= ~ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD;
                info->thresholds.shutdown = 0;
            }

            rv = ufi_file_read_int(&val, "/sys_switch/psu/psu%d/temp%d/max_error_threshold",
                entry.parent, entry.child);

            if(rv == ONLP_STATUS_OK) {
                info->caps |= ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD;
                info->thresholds.error = val;
            } else {
                info->caps &= ~ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD;
                info->thresholds.error = 0;
            }

            rv = ufi_file_read_int(&val, "/sys_switch/psu/psu%d/temp%d/max_warning_threshold",
                entry.parent, entry.child);

            if(rv == ONLP_STATUS_OK) {
                info->caps |= ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD;
                info->thresholds.warning = val;
            } else {
                info->caps &= ~ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD;
                info->thresholds.warning = 0;
            }
        }

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
int __WEAK onlp_thermali_status_get(onlp_oid_t id, uint32_t* rv)
{
    onlp_thermal_info_t info = {0};
    ONLP_TRY(onlp_thermali_info_get(id, &info));
    *rv = info.status;
    return ONLP_STATUS_OK;
}

/**
 * @brief Retrieve the thermal's oid header.
 * @param id The thermal oid.
 * @param rv [out] Receives the header.
 */
int __WEAK onlp_thermali_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* hdr)
{
    onlp_thermal_info_t info = {0};

    ONLP_TRY(onlp_thermali_info_get(id, &info));
    *hdr = info.hdr;
    return ONLP_STATUS_OK;
}

/**
 * @brief Generic ioctl.
 */
int __WEAK onlp_thermali_ioctl(int id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

