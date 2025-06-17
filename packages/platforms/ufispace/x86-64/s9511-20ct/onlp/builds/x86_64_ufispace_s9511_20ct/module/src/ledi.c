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
 * LED Platform Implementation.
 *
 ***********************************************************/
#include <onlp/platformi/ledi.h>
#include "platform_lib.h"

#define LED_STATUS ONLP_LED_STATUS_PRESENT
#define LED_CAPS   ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | \
                    ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING

#define CHASSIS_LED_INFO(id, desc)               \
    {                                            \
        { ONLP_LED_ID_CREATE(id), desc, POID_0}, \
        LED_STATUS,                              \
        LED_CAPS,                                \
    }

/*
 * Get the information for the given LED OID.
 */
static onlp_led_info_t led_info[] =
{
    { }, // Not used *
    CHASSIS_LED_INFO(ONLP_LED_SYS_GNSS      , "CHASSIS LED 1 (GNSS LED)"),
    CHASSIS_LED_INFO(ONLP_LED_SYS_SYNC      , "CHASSIS LED 2 (SYNC LED)"),
    CHASSIS_LED_INFO(ONLP_LED_SYS_STAT      , "CHASSIS LED 3 (STAT LED)"),
    CHASSIS_LED_INFO(ONLP_LED_SYS_FAN       , "CHASSIS LED 4 (FAN LED)"),
    CHASSIS_LED_INFO(ONLP_LED_SYS_PWR       , "CHASSIS LED 5 (PWR LED)"),
};

typedef enum led_act_e {
    ACTION_LED_RO = 0,
    ACTION_LED_RW,
    ACTION_LED_ATTR_MAX,
} led_act_t;

typedef struct
{
    led_act_t action;
    int attr_onoff;
    int attr_blink;
    int attr_color;
} led_attr_t;

typedef enum cpld_attr_idx_e {
    CPLD_SYS_LED_STATUS = 0,
    CPLD_SYS_LED_BLINKING,
    CPLD_SYS_LED_COLOR,
    CPLD_GNSS_LED_STATUS,
    CPLD_GNSS_LED_BLINKING,
    CPLD_GNSS_LED_COLOR,
    CPLD_SYNC_LED_STATUS,
    CPLD_SYNC_LED_BLINKING,
    CPLD_SYNC_LED_COLOR,
    CPLD_PWR_LED_STATUS,
    CPLD_PWR_LED_BLINKING,
    CPLD_PWR_LED_COLOR,
    CPLD_FAN_LED_STATUS,
    CPLD_FAN_LED_BLINKING,
    CPLD_FAN_LED_COLOR,
    CPLD_NONE,
} cpld_attr_idx_t;

static const led_attr_t led_attr[] = {
/*  led attribute            action                attr_onoff                attr_blink                   attr_color */
    [ONLP_LED_SYS_GNSS]    = {ACTION_LED_RW        ,CPLD_GNSS_LED_STATUS    ,CPLD_GNSS_LED_BLINKING      ,CPLD_GNSS_LED_COLOR},
    [ONLP_LED_SYS_SYNC]    = {ACTION_LED_RW        ,CPLD_SYNC_LED_STATUS    ,CPLD_SYNC_LED_BLINKING      ,CPLD_SYNC_LED_COLOR},
    [ONLP_LED_SYS_STAT]    = {ACTION_LED_RW        ,CPLD_SYS_LED_STATUS     ,CPLD_SYS_LED_BLINKING       ,CPLD_SYS_LED_COLOR},
    [ONLP_LED_SYS_FAN]     = {ACTION_LED_RO        ,CPLD_FAN_LED_STATUS     ,CPLD_FAN_LED_BLINKING       ,CPLD_FAN_LED_COLOR},
    [ONLP_LED_SYS_PWR]     = {ACTION_LED_RO        ,CPLD_PWR_LED_STATUS     ,CPLD_PWR_LED_BLINKING       ,CPLD_PWR_LED_COLOR},
};

/* Ufispace Specific Defined functions */
static int get_led_sysfs(cpld_attr_idx_t idx, char** str);
static int get_led_local_id(int id, int *local_id);
static int get_sys_led_info(int local_id, onlp_led_info_t* info);

/******************************************************************************************************************
**                                                                                                               **
**                                                ONLP Standard APIs                                             **
**                                                                                                               **
*******************************************************************************************************************/
/**
 * @brief Initialize the LED subsystem.
 */
int onlp_ledi_init(void)
{
    init_lock();
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the information for the given LED
 * @param id The LED OID
 * @param rv [out] Receives the LED information.
 */
int onlp_ledi_info_get(onlp_oid_t id, onlp_led_info_t* rv)
{
    int local_id;

    ONLP_TRY(get_led_local_id(id, &local_id));
    ONLP_TRY(get_sys_led_info(local_id, rv));

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the LED operational status.
 * @param id The LED OID
 * @param rv [out] Receives the operational status.
 */
int onlp_ledi_status_get(onlp_oid_t id, uint32_t* rv)
{
    int local_id;
    onlp_led_info_t info ={0};

    ONLP_TRY(get_led_local_id(id, &local_id));
    if (local_id <= 0 || local_id >= ONLP_LED_MAX) {
        return ONLP_STATUS_E_INVALID;
    }

    if ((led_attr[local_id].action != ACTION_LED_RW) && (led_attr[local_id].action != ACTION_LED_RO)) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    ONLP_TRY(get_sys_led_info(local_id, &info));
    *rv = info.status;

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the LED header.
 * @param id The LED OID
 * @param rv [out] Receives the header.
 */
int onlp_ledi_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* rv)
{
    int local_id;

    ONLP_TRY(get_led_local_id(id, &local_id));
    *rv = led_info[local_id].hdr;

    return ONLP_STATUS_OK;
}

/**
 * @brief Turn an LED on or off
 * @param id The LED OID
 * @param on_or_off (boolean) on if 1 off if 0
 * @param This function is only relevant if the ONOFF capability is set.
 * @notes See onlp_led_set() for a description of the default behavior.
 */
int onlp_ledi_set(onlp_oid_t id, int on_or_off)
{
    int local_id;

    ONLP_TRY(get_led_local_id(id, &local_id));
    if (led_attr[local_id].action != ACTION_LED_RW) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if (on_or_off) {
        return ONLP_STATUS_E_UNSUPPORTED;
    } else {
        return onlp_ledi_mode_set(id, ONLP_LED_MODE_OFF);
    }
}

/**
 * @brief LED ioctl
 * @param id The LED OID
 * @param vargs The variable argument list for the ioctl call.
 */
int onlp_ledi_ioctl(onlp_oid_t id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Set the LED mode.
 * @param id The LED OID
 * @param mode The new mode.
 * @notes Only called if the mode is advertised in the LED capabilities.
 */
int onlp_ledi_mode_set(onlp_oid_t id, onlp_led_mode_t mode)
{
    char *sysfs_led_onoff = NULL, *sysfs_led_blink = NULL, *sysfs_led_color = NULL;
    int local_id;

    ONLP_TRY(get_led_local_id(id, &local_id));
    if (led_attr[local_id].action != ACTION_LED_RW) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    int led_color = 0, led_blink = 0, led_onoff = 0;
    switch(mode) {
        case ONLP_LED_MODE_GREEN:
            led_color=1;
            led_blink=0;
            led_onoff=1;
            break;
        case ONLP_LED_MODE_GREEN_BLINKING:
            led_color=1;
            led_blink=1;
            led_onoff=1;
            break;
        case ONLP_LED_MODE_YELLOW:
            led_color=0;
            led_blink=0;
            led_onoff=1;
            break;
        case ONLP_LED_MODE_YELLOW_BLINKING:
            led_color=0;
            led_blink=1;
            led_onoff=1;
            break;
        case ONLP_LED_MODE_OFF:
            led_color=0;
            led_blink=0;
            led_onoff=0;
            break;
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
    }

    if (led_onoff == 0) {
        ONLP_TRY(get_led_sysfs(led_attr[local_id].attr_onoff, &sysfs_led_onoff));
        ONLP_TRY(onlp_file_write_int(led_onoff, sysfs_led_onoff));
    } else if (led_onoff == 1  && led_color >= 0 && led_blink >= 0) {
        /* Set led onoff */
        ONLP_TRY(get_led_sysfs(led_attr[local_id].attr_onoff, &sysfs_led_onoff));
        ONLP_TRY(onlp_file_write_int(led_onoff, sysfs_led_onoff));

        /* Set led color */
        ONLP_TRY(get_led_sysfs(led_attr[local_id].attr_color, &sysfs_led_color));
        ONLP_TRY(onlp_file_write_int(led_color, sysfs_led_color));

        /* Set led blinking status */
        ONLP_TRY(get_led_sysfs(led_attr[local_id].attr_blink, &sysfs_led_blink));
        ONLP_TRY(onlp_file_write_int(led_blink, sysfs_led_blink));
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Set the LED character.
 * @param id The LED OID
 * @param c The character..
 * @notes Only called if the char capability is set.
 */
int onlp_ledi_char_set(onlp_oid_t id, char c)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/******************************************************************************************************************
**                                                                                                               **
**                                           Upispace Specific Defined APIs                                      **
**                                                                                                               **
*******************************************************************************************************************/
/**
 * @brief Get and check led local ID
 * @param id [in] OID
 * @param local_id [out] The led local id
 */
static int get_led_local_id(int id, int *local_id)
{
    int tmp_id;

    if(local_id == NULL) {
        return ONLP_STATUS_E_PARAM;
    }

    if(!ONLP_OID_IS_LED(id)) {
        return ONLP_STATUS_E_INVALID;
    }

    tmp_id = ONLP_OID_ID_GET(id);
    switch (tmp_id) {
        case ONLP_LED_SYS_GNSS:
        case ONLP_LED_SYS_SYNC:
        case ONLP_LED_SYS_STAT:
        case ONLP_LED_SYS_FAN:
        case ONLP_LED_SYS_PWR:
            *local_id = tmp_id;
            return ONLP_STATUS_OK;
        default:
            return ONLP_STATUS_E_INVALID;
    }

    return ONLP_STATUS_E_INVALID;
}

static int get_led_sysfs(cpld_attr_idx_t idx, char** str)
{
    if(str == NULL)
        return ONLP_STATUS_E_PARAM;

    switch(idx) {
        case CPLD_SYS_LED_STATUS:
            *str = SYSFS_CPLD1 "sys_led_status";
            break;
        case CPLD_SYS_LED_BLINKING:
            *str = SYSFS_CPLD1 "sys_led_blinking";
            break;
        case CPLD_SYS_LED_COLOR:
            *str = SYSFS_CPLD1 "sys_led_color";
            break;
        case CPLD_GNSS_LED_STATUS:
            *str = SYSFS_CPLD1 "gnss_led_status";
            break;
        case CPLD_GNSS_LED_BLINKING:
            *str = SYSFS_CPLD1 "gnss_led_blinking";
            break;
        case CPLD_GNSS_LED_COLOR:
            *str = SYSFS_CPLD1 "gnss_led_color";
            break;
        case CPLD_SYNC_LED_STATUS:
            *str = SYSFS_CPLD1 "sync_led_status";
            break;
        case CPLD_SYNC_LED_BLINKING:
            *str = SYSFS_CPLD1 "sync_led_blinking";
            break;
        case CPLD_SYNC_LED_COLOR:
            *str = SYSFS_CPLD1 "sync_led_color";
            break;
        case CPLD_PWR_LED_STATUS:
            *str = SYSFS_CPLD1 "pwr_led_status";
            break;
        case CPLD_PWR_LED_BLINKING:
            *str = SYSFS_CPLD1 "pwr_led_blinking";
            break;
        case CPLD_PWR_LED_COLOR:
            *str = SYSFS_CPLD1 "pwr_led_color";
            break;
        case CPLD_FAN_LED_STATUS:
            *str = SYSFS_CPLD1 "fan_led_status";
            break;
        case CPLD_FAN_LED_BLINKING:
            *str = SYSFS_CPLD1 "fan_led_blinking";
            break;
        case CPLD_FAN_LED_COLOR:
            *str = SYSFS_CPLD1 "fan_led_color";
            break;
        default:
            *str = "";
            return ONLP_STATUS_E_PARAM;
    }

    return ONLP_STATUS_OK;
}

static int get_sys_led_info(int local_id, onlp_led_info_t* info)
{
    char *sysfs_led_onoff = NULL, *sysfs_led_blink = NULL, *sysfs_led_color = NULL;
    int led_color = 0, led_blink = 0, led_onoff = 0;

    if (local_id <= 0 || local_id >= ONLP_LED_MAX) {
        return ONLP_STATUS_E_INVALID;
    }

    *info = led_info[local_id];

    /* Read led onoff */
    if (get_led_sysfs(led_attr[local_id].attr_onoff, &sysfs_led_onoff) != ONLP_STATUS_OK || sysfs_led_onoff == NULL) {
        return ONLP_STATUS_E_INTERNAL;
    }
    ONLP_TRY(read_file_hex(&led_onoff, sysfs_led_onoff));

    /* Read led color */
    if (get_led_sysfs(led_attr[local_id].attr_color, &sysfs_led_color) != ONLP_STATUS_OK || sysfs_led_color == NULL) {
        return ONLP_STATUS_E_INTERNAL;
    }
    ONLP_TRY(read_file_hex(&led_color, sysfs_led_color));

    /* Read led blinking */
    if (get_led_sysfs(led_attr[local_id].attr_blink, &sysfs_led_blink) != ONLP_STATUS_OK || sysfs_led_blink == NULL) {
        return ONLP_STATUS_E_INTERNAL;
    }
    ONLP_TRY(read_file_hex(&led_blink, sysfs_led_blink));

    /* Show onoff into ONLP API */
    if (led_onoff == 0) {
        info->status &= ~ONLP_LED_STATUS_ON;
        info->mode = ONLP_LED_MODE_OFF;
    } else {
        info->status |= ONLP_LED_STATUS_ON;
        /* Show color into ONLP API */
        /* (Yellow) */
        if (led_color == 0) {
            info->mode = (led_blink == 1) ? ONLP_LED_MODE_YELLOW_BLINKING : ONLP_LED_MODE_YELLOW;
        }
        /* (Green) */
        else if(led_color == 1) {
            info->mode = (led_blink == 1) ? ONLP_LED_MODE_GREEN_BLINKING : ONLP_LED_MODE_GREEN;
        } else {
            return ONLP_STATUS_E_INTERNAL;
        }
    }

    return ONLP_STATUS_OK;
}
