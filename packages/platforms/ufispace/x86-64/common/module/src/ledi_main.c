/************************************************************
 * Copyright (C) 2026 Ufispace Technology Corporation.
 ************************************************************
 *
 * LED Platform Implementation.
 *
 ***********************************************************/
#include <onlp/platformi/ledi.h>
#include <ufispace_platform/platform_lib.h>
#include <ufispace_common/platform_lib_main.h>
#include <ufispace_common/ledi_main.h>
#include <unistd.h>
#include <sys/stat.h>

static int led_total = 0;
static led_elems *leds = NULL;

int _parse_led_on(char *str, int *mode)
{
    if(!str || !mode) {
        return ONLP_STATUS_E_PARAM;
    }

    if(!strncmp(str, STR_RED, sizeof(STR_RED))) {
        *mode = ONLP_LED_MODE_RED;
    } else if(!strncmp(str, STR_RED_BLINK, sizeof(STR_RED_BLINK))) {
        *mode = ONLP_LED_MODE_RED_BLINKING;
    } else if(!strncmp(str, STR_ORANGE, sizeof(STR_ORANGE))) {
        *mode = ONLP_LED_MODE_ORANGE;
    } else if(!strncmp(str, STR_ORANGE_BLINK, sizeof(STR_ORANGE_BLINK))) {
        *mode = ONLP_LED_MODE_ORANGE_BLINKING;
    } else if(!strncmp(str, STR_YELLOW, sizeof(STR_YELLOW))) {
        *mode = ONLP_LED_MODE_YELLOW;
    } else if(!strncmp(str, STR_YELLOW_BLINK, sizeof(STR_YELLOW_BLINK))) {
        *mode = ONLP_LED_MODE_YELLOW_BLINKING;
    } else if(!strncmp(str, STR_GREEN, sizeof(STR_GREEN))) {
        *mode = ONLP_LED_MODE_GREEN;
    } else if(!strncmp(str, STR_GREEN_BLINK, sizeof(STR_GREEN_BLINK))) {
        *mode = ONLP_LED_MODE_GREEN_BLINKING;
    } else if(!strncmp(str, STR_BLUE, sizeof(STR_BLUE))) {
        *mode = ONLP_LED_MODE_BLUE;
    } else if(!strncmp(str, STR_BLUE_BLINK, sizeof(STR_BLUE_BLINK))) {
        *mode = ONLP_LED_MODE_BLUE_BLINKING;
    } else if(!strncmp(str, STR_PURPLE, sizeof(STR_PURPLE))) {
        *mode = ONLP_LED_MODE_PURPLE;
    } else if(!strncmp(str, STR_PURPLE_BLINK, sizeof(STR_PURPLE_BLINK))) {
        *mode = ONLP_LED_MODE_PURPLE_BLINKING;
    } else {
        *mode = ONLP_LED_MODE_OFF;
    }

    return ONLP_STATUS_OK;
}

int _parse_led_capability(char *str, uint32_t *caps)
{
    char *token = NULL;
    char *saveptr = NULL;

    if(!str || !caps) {
        return ONLP_STATUS_E_PARAM;
    }

    *caps = 0;

    token = strtok_r(str, ",", &saveptr);
    while (token != NULL) {
        if(!strncmp(token, STR_RED, sizeof(STR_RED))) {
            *caps |= (ONLP_LED_CAPS_ON_OFF |ONLP_LED_CAPS_RED);
        } else if(!strncmp(token, STR_RED_BLINK, sizeof(STR_RED_BLINK))) {
            *caps |= (ONLP_LED_CAPS_ON_OFF |ONLP_LED_CAPS_RED_BLINKING);
        } else if(!strncmp(token, STR_ORANGE, sizeof(STR_ORANGE))) {
            *caps |= (ONLP_LED_CAPS_ON_OFF |ONLP_LED_CAPS_ORANGE);
        } else if(!strncmp(token, STR_ORANGE_BLINK, sizeof(STR_ORANGE_BLINK))) {
            *caps |= (ONLP_LED_CAPS_ON_OFF |ONLP_LED_CAPS_ORANGE_BLINKING);
        } else if(!strncmp(token, STR_YELLOW, sizeof(STR_YELLOW))) {
            *caps |= (ONLP_LED_CAPS_ON_OFF |ONLP_LED_CAPS_YELLOW);
        } else if(!strncmp(token, STR_YELLOW_BLINK, sizeof(STR_YELLOW_BLINK))) {
            *caps |= (ONLP_LED_CAPS_ON_OFF |ONLP_LED_CAPS_YELLOW_BLINKING);
        } else if(!strncmp(token, STR_GREEN, sizeof(STR_GREEN))) {
            *caps |= (ONLP_LED_CAPS_ON_OFF |ONLP_LED_CAPS_GREEN);
        } else if(!strncmp(token, STR_GREEN_BLINK, sizeof(STR_GREEN_BLINK))) {
            *caps |= (ONLP_LED_CAPS_ON_OFF |ONLP_LED_CAPS_GREEN_BLINKING);
        } else if(!strncmp(token, STR_BLUE, sizeof(STR_BLUE))) {
            *caps |= (ONLP_LED_CAPS_ON_OFF |ONLP_LED_CAPS_BLUE);
        } else if(!strncmp(token, STR_BLUE_BLINK, sizeof(STR_BLUE_BLINK))) {
            *caps |= (ONLP_LED_CAPS_ON_OFF |ONLP_LED_CAPS_BLUE_BLINKING);
        } else if(!strncmp(token, STR_PURPLE, sizeof(STR_PURPLE))) {
            *caps |= (ONLP_LED_CAPS_ON_OFF |ONLP_LED_CAPS_PURPLE);
        } else if(!strncmp(token, STR_PURPLE_BLINK, sizeof(STR_PURPLE_BLINK))) {
            *caps |= (ONLP_LED_CAPS_ON_OFF |ONLP_LED_CAPS_PURPLE_BLINKING);
        }
        token = strtok_r(NULL, ",", &saveptr);
    }

    return ONLP_STATUS_OK;
}

int _update_led_status(int color, uint32_t *status) {
    if(!status) {
        return ONLP_STATUS_E_PARAM;
    }

    switch(color) {
        case S3IP_LED_COLOR_DARK:
            *status = (ONLP_LED_STATUS_PRESENT);
            break;
        case S3IP_LED_COLOR_GREEN:
            *status = (ONLP_LED_STATUS_PRESENT | ONLP_LED_STATUS_ON);
            break;
        case S3IP_LED_COLOR_YELLOW:
            *status = (ONLP_LED_STATUS_PRESENT | ONLP_LED_STATUS_ON);
            break;
        case S3IP_LED_COLOR_RED:
            *status = (ONLP_LED_STATUS_PRESENT | ONLP_LED_STATUS_ON);
            break;
        case S3IP_LED_COLOR_BLUE:
            *status = (ONLP_LED_STATUS_PRESENT | ONLP_LED_STATUS_ON);
            break;
        case S3IP_LED_COLOR_GREEN_BLINK:
            *status = (ONLP_LED_STATUS_PRESENT | ONLP_LED_STATUS_ON);
            break;
        case S3IP_LED_COLOR_YELLOW_BLINK:
            *status = (ONLP_LED_STATUS_PRESENT | ONLP_LED_STATUS_ON);
            break;
        case S3IP_LED_COLOR_RED_BLINK:
            *status = (ONLP_LED_STATUS_PRESENT | ONLP_LED_STATUS_ON);
            break;
        case S3IP_LED_COLOR_BLUE_BLINK:
            *status = (ONLP_LED_STATUS_PRESENT | ONLP_LED_STATUS_ON);
            break;
        default:
            *status = (ONLP_LED_STATUS_PRESENT | ONLP_LED_STATUS_FAILED);
            break;
    }

    return ONLP_STATUS_OK;
}

int _s3ip_to_onl_color(int s3ip_color, int *onl_color) {
    if(!onl_color) {
        return ONLP_STATUS_E_PARAM;
    }

    switch(s3ip_color) {
        case S3IP_LED_COLOR_DARK:
            *onl_color = ONLP_LED_MODE_OFF;
            break;
        case S3IP_LED_COLOR_GREEN:
            *onl_color = ONLP_LED_MODE_GREEN;
            break;
        case S3IP_LED_COLOR_YELLOW:
            *onl_color = ONLP_LED_MODE_YELLOW;
            break;
        case S3IP_LED_COLOR_RED:
            *onl_color = ONLP_LED_MODE_RED;
            break;
        case S3IP_LED_COLOR_BLUE:
            *onl_color = ONLP_LED_MODE_BLUE;
            break;
        case S3IP_LED_COLOR_GREEN_BLINK:
            *onl_color = ONLP_LED_MODE_GREEN_BLINKING;
            break;
        case S3IP_LED_COLOR_YELLOW_BLINK:
            *onl_color = ONLP_LED_MODE_YELLOW_BLINKING;
            break;
        case S3IP_LED_COLOR_RED_BLINK:
            *onl_color = ONLP_LED_MODE_RED_BLINKING;
            break;
        case S3IP_LED_COLOR_BLUE_BLINK:
            *onl_color = ONLP_LED_MODE_BLUE_BLINKING;
            break;
        default:
            break;
    }

    return ONLP_STATUS_OK;
}

int _onl_to_s3ip_color(int onl_color, int *s3ip_color) {
    if(!s3ip_color) {
        return ONLP_STATUS_E_PARAM;
    }

    switch(onl_color) {
        case ONLP_LED_MODE_OFF:
            *s3ip_color = S3IP_LED_COLOR_DARK;
            break;
        case ONLP_LED_MODE_GREEN:
            *s3ip_color = S3IP_LED_COLOR_GREEN;
            break;
        case ONLP_LED_MODE_YELLOW:
            *s3ip_color = S3IP_LED_COLOR_YELLOW;
            break;
        case ONLP_LED_MODE_RED:
            *s3ip_color = S3IP_LED_COLOR_RED;
            break;
        case ONLP_LED_MODE_BLUE:
            *s3ip_color = S3IP_LED_COLOR_BLUE;
            break;
        case ONLP_LED_MODE_GREEN_BLINKING:
            *s3ip_color = S3IP_LED_COLOR_GREEN_BLINK;
            break;
        case ONLP_LED_MODE_YELLOW_BLINKING:
            *s3ip_color = S3IP_LED_COLOR_YELLOW_BLINK;
            break;
        case ONLP_LED_MODE_RED_BLINKING:
            *s3ip_color = S3IP_LED_COLOR_RED_BLINK;
            break;
        case ONLP_LED_MODE_BLUE_BLINKING:
            *s3ip_color = S3IP_LED_COLOR_BLUE_BLINK;
            break;
        default:
            break;
    }

    return ONLP_STATUS_OK;
}

static int _get_on_color_mode(int parent_id, int *mode)
{
    char *buf = NULL;
    int len1 = 0;
    int rv = ONLP_STATUS_OK;

    if (!mode) {
        return ONLP_STATUS_E_PARAM;
    }

    len1 = onlp_file_read_str(&buf, "/sys_switch/sysled/led%d/on_color", parent_id);
    if (!buf || !len1) {
        aim_free(buf);
        buf = NULL;
        return ONLP_STATUS_E_MISSING;
    }

    rv = _parse_led_on(buf, mode);
    aim_free(buf);
    buf = NULL;

    if (rv != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INVALID;
    }

    return ONLP_STATUS_OK;
}

/**
  * @brief Get led components total count.
  */
int _onlp_led_total_get(int *total)
{
    int rv = ONLP_STATUS_OK;

    if(!total) {
        return ONLP_STATUS_E_PARAM;
    }

    rv = ufi_file_read_int(total, "/sys_switch/slot/slot1/num_components/num_leds");
    if(rv != ONLP_STATUS_OK || *total < 0) {
        *total = 0;
    }
    return rv;
}

/**
  * @brief Get the fan entry.
  */
int __WEAK _onlp_led_entry_get(int logical_id, led_elems *entry)
{
    if (!entry) {
        return ONLP_STATUS_E_PARAM;
    }
    if (logical_id >= led_total) {
        return ONLP_STATUS_E_INVALID;
    }
    if (!leds[logical_id].valid) {
        return ONLP_STATUS_E_MISSING;
    }

    *entry = leds[logical_id];
    return ONLP_STATUS_OK;
}

/**
 * @brief Initialize the LED subsystem.
 */
int __WEAK onlp_ledi_init(void)
{
    int count = 0;
    int oid_base = 0;
    int rv = ONLP_STATUS_OK;
    int i = 1;

    if (leds != NULL) {
        return ONLP_STATUS_OK;
    }

    lock_init();

    ONLP_TRY(_onlp_led_total_get(&led_total));
    ONLP_TRY(_onlp_led_oid_base_get(&oid_base));
    leds = (led_elems *) aim_zmalloc(sizeof(led_elems)*(led_total));
    if (leds == NULL) {
        AIM_LOG_ERROR("Failed to allocate memory for LED elements");
        return ONLP_STATUS_E_INTERNAL;
    }

    // scan led
    rv = ufi_file_read_int(&count, "/sys_switch/sysled/number");
    if(rv == ONLP_STATUS_OK && count > 0) {
        for(i=1; i <= count; i++) {
            int oid =  0;
            int logical_id = 0;
            char *description = NULL;
            int len1 = onlp_file_read_str(&description, "/sys_switch/sysled/led%d/description", i);

            logical_id = i - 1;
            oid = oid_base + logical_id;
            leds[logical_id].id = ONLP_LED_ID_CREATE(oid);
            leds[logical_id].parent = i;
            leds[logical_id].valid = 0;

            if(description && len1 > 0) {
                if(strncmp(description, COMM_STR_NA, sizeof(COMM_STR_NA)) != 0) {
                    leds[logical_id].valid = 1;
                }
            }

            if (description) {
                aim_free(description);
            }
        }
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the information for the given LED
 * @param id The LED OID
 * @param rv [out] Receives the LED information.
 */
int __WEAK onlp_ledi_info_get(onlp_oid_t id, onlp_led_info_t* info)
{
    uint32_t logical_id = 0;
    led_elems entry = {0};
    int base = 0;
    int rv = ONLP_STATUS_OK;
    int color = 0;
    int len1 = 0;
    char *buf = NULL;

    ONLP_TRY(onlp_ledi_init());

    if(!info) {
        return ONLP_STATUS_E_PARAM;
    } else {
        memset(info, 0, sizeof(onlp_led_info_t));
    }

    if(!ONLP_OID_IS_LED(id)) {
        return ONLP_STATUS_E_INVALID;
    }

    ONLP_TRY(_onlp_led_oid_base_get(&base));
    logical_id = ONLP_OID_ID_GET(id) - base;
    ONLP_TRY(_onlp_led_entry_get(logical_id, &entry));

    info->hdr.id = entry.id;

    len1 = onlp_file_read_str(&buf, "/sys_switch/sysled/led%d/description",
            entry.parent);
    if(!!buf && !!len1) {
        snprintf(info->hdr.description, sizeof(info->hdr.description), "%s", buf);
    }
    aim_free(buf);
    buf = NULL;

    len1 = onlp_file_read_str(&buf, "/sys_switch/sysled/led%d/capability",
            entry.parent);
    rv = _parse_led_capability(buf, &info->caps);
    if(rv != ONLP_STATUS_OK) {
        aim_free(buf);
        buf = NULL;
        info->caps = 0;
        return ONLP_STATUS_E_INVALID;
    }
    aim_free(buf);
    buf = NULL;

    rv = ufi_file_read_int(&color, "/sys_switch/sysled/led%d/status", entry.parent);
    if(rv == ONLP_STATUS_OK) {
        ONLP_TRY(_update_led_status(color, &info->status));
        ONLP_TRY(_s3ip_to_onl_color(color, (int *) &info->mode));
    } else {
        info->status = 0;
        info->mode = 0;
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the LED operational status.
 * @param id The LED OID
 * @param rv [out] Receives the operational status.
 */
int __WEAK onlp_ledi_status_get(onlp_oid_t id, uint32_t* rv)
{
    onlp_led_info_t info = {0};

    ONLP_TRY(onlp_ledi_info_get(id, &info));
    *rv = info.status;
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the LED header.
 * @param id The LED OID
 * @param rv [out] Receives the header.
 */
int __WEAK onlp_ledi_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* hdr)
{
    onlp_led_info_t info = {0};

    ONLP_TRY(onlp_ledi_info_get(id, &info));
    *hdr = info.hdr;
    return ONLP_STATUS_OK;
}

/**
 * @brief Turn an LED on or off
 * @param id The LED OID
 * @param on_or_off (boolean) on if 1 off if 0
 * @param This function is only relevant if the ONOFF capability is set.
 * @notes See onlp_led_set() for a description of the default behavior.
 */
int __WEAK onlp_ledi_set(onlp_oid_t id, int on_or_off)
{
    ONLP_TRY(onlp_ledi_init());

    if (on_or_off) {
        return onlp_ledi_mode_set(id, ONLP_LED_MODE_ON);
    } else {
        return onlp_ledi_mode_set(id, ONLP_LED_MODE_OFF);
    }
}

/**
 * @brief LED ioctl
 * @param id The LED OID
 * @param vargs The variable argument list for the ioctl call.
 */
int __WEAK onlp_ledi_ioctl(onlp_oid_t id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Set the LED mode.
 * @param id The LED OID
 * @param mode The new mode.
 * @notes Only called if the mode is advertised in the LED capabilities.
 */
int __WEAK onlp_ledi_mode_set(onlp_oid_t id, onlp_led_mode_t mode)
{
    uint32_t logical_id = 0;
    led_elems entry = {0};
    int s3ip_color = S3IP_LED_COLOR_DARK;
    char path[256] = {0};
    struct stat st = {0};
    int base = 0;
    int actual_mode = mode;

    ONLP_TRY(onlp_ledi_init());

    if(!ONLP_OID_IS_LED(id)) {
        return ONLP_STATUS_E_INVALID;
    }

    ONLP_TRY(_onlp_led_oid_base_get(&base));
    logical_id = ONLP_OID_ID_GET(id) - base;
    ONLP_TRY(_onlp_led_entry_get(logical_id, &entry));

    snprintf(path, sizeof(path), "/sys_switch/sysled/led%d/status", entry.parent);

    if (stat(path, &st) == 0) {
        if (!(st.st_mode & S_IWUSR)) {
                return ONLP_STATUS_E_UNSUPPORTED;
        }
    } else {
        return ONLP_STATUS_E_MISSING;
    }

    if (mode == ONLP_LED_MODE_ON) {
        if (_get_on_color_mode(entry.parent, &actual_mode) != ONLP_STATUS_OK) {
            actual_mode = mode;
        }
    }

    ONLP_TRY(_onl_to_s3ip_color(actual_mode, &s3ip_color));
    ONLP_TRY(onlp_file_write_int(s3ip_color, "/sys_switch/sysled/led%d/status",
        entry.parent));
    return ONLP_STATUS_OK;
}

/**
 * @brief Set the LED character.
 * @param id The LED OID
 * @param c The character..
 * @notes Only called if the char capability is set.
 */
int __WEAK onlp_ledi_char_set(onlp_oid_t id, char c)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

