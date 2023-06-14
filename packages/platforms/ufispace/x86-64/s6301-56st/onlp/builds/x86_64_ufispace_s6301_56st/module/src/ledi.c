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
 *
 ***********************************************************/
#include <onlp/platformi/ledi.h>
#include "platform_lib.h"

#define SYSFS_LED_ID_NAME       "led_id"
#define SYSFS_LED_SYS_NAME      "led_sys"
#define SYSFS_LED_POE_NAME      "led_poe"
#define SYSFS_LED_SPD_NAME      "led_spd"
#define SYSFS_LED_FAN_NAME      "led_fan"
#define SYSFS_LED_LNK_NAME      "led_lnk"
#define SYSFS_LED_PWR1_NAME     "led_pwr1"
#define SYSFS_LED_PWR0_NAME     "led_pwr0"
#define SYSFS_LED_FMT           LPC_MB_CPLD_PATH"%s"
#define LED_ONOFF_MASK          0b00001000
#define LED_BLINK_MASK          0b00000100
#define LED_COLOR_MASK          0b00000001

#define LED_STATUS ONLP_LED_STATUS_PRESENT
#define LED_CAPS   ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | \
                   ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_LED(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

#define CHASSIS_LED_INFO(id, desc)              \
    {                                           \
        { ONLP_LED_ID_CREATE(id), desc, POID_0 },\
        LED_STATUS,                             \
        LED_CAPS,                               \
    }


/*
 * Get the information for the given LED OID.
 */
static onlp_led_info_t led_info[] =
{
    { }, //Not used *
    {                                           \
        { ONLP_LED_ID_CREATE(ONLP_LED_ID), "Chassis LED 1 (ID LED)", POID_0 },\
        LED_STATUS,                             \
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_BLUE | ONLP_LED_CAPS_BLUE_BLINKING,                               \
    },
    CHASSIS_LED_INFO(ONLP_LED_SYS,  "Chassis LED 2 (SYS LED)"),
    CHASSIS_LED_INFO(ONLP_LED_POE,  "Chassis LED 3 (POE LED)"),
    CHASSIS_LED_INFO(ONLP_LED_SPD,  "Chassis LED 4 (SPD LED)"),
    CHASSIS_LED_INFO(ONLP_LED_FAN,  "Chassis LED 5 (FAN LED)"),
    CHASSIS_LED_INFO(ONLP_LED_LNK,  "Chassis LED 6 (LNK LED)"),
    CHASSIS_LED_INFO(ONLP_LED_PWR1, "Chassis LED 7 (PS1 LED)"),
    CHASSIS_LED_INFO(ONLP_LED_PWR0, "Chassis LED 8 (PS0 LED)"),

};

int sys_led_info_get(onlp_led_info_t* info, int local_id)
{
    char *fsname;
    int led_val, led_color, led_blink, led_onoff;

    switch(local_id) {
        case ONLP_LED_ID:
            fsname = SYSFS_LED_ID_NAME;
            break;
        case ONLP_LED_SYS:
            fsname = SYSFS_LED_SYS_NAME;
            break;
        case ONLP_LED_POE:
            fsname = SYSFS_LED_POE_NAME;
            break;
        case ONLP_LED_SPD:
            fsname = SYSFS_LED_SPD_NAME;
            break;
        case ONLP_LED_FAN:
            fsname = SYSFS_LED_FAN_NAME;
            break;
        case ONLP_LED_LNK:
            fsname = SYSFS_LED_LNK_NAME;
            break;
        case ONLP_LED_PWR1:
            fsname = SYSFS_LED_PWR1_NAME;
            break;
        case ONLP_LED_PWR0:
            fsname = SYSFS_LED_PWR0_NAME;
            break;
        default:
            return ONLP_STATUS_E_INVALID;
    }

    ONLP_TRY(file_read_hex(&led_val, SYSFS_LED_FMT, fsname));

    led_color = mask_shift(led_val, LED_COLOR_MASK);
    led_blink = mask_shift(led_val, LED_BLINK_MASK);
    led_onoff = mask_shift(led_val, LED_ONOFF_MASK);

    //onoff
    if(led_onoff == 0) {
        info->mode = ONLP_LED_MODE_OFF;
    } else {
        //color
        if(led_color == 0) {
            info->mode = ONLP_LED_MODE_YELLOW;
        } else {
            info->mode = ONLP_LED_MODE_GREEN;
        }
        // id led case
        if(local_id == ONLP_LED_ID) {
            info->mode = ONLP_LED_MODE_BLUE;
        }
        //blinking
        if(led_blink == 1) {
            info->mode = info->mode + 1;
        }
    }

    return ONLP_STATUS_OK;
}


/**
 * @brief Initialize the LED subsystem.
 */
int onlp_ledi_init(void)
{
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
    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);
    *rv = led_info[local_id];
    ONLP_TRY(onlp_ledi_status_get(id, &rv->status));

    if((rv->status & ONLP_LED_STATUS_PRESENT) == 0) {
        return ONLP_STATUS_OK;
    }

    switch(local_id) {
        case ONLP_LED_ID:
        case ONLP_LED_SYS:
        case ONLP_LED_POE:
        case ONLP_LED_SPD:
        case ONLP_LED_FAN:
        case ONLP_LED_LNK:
        case ONLP_LED_PWR1:
        case ONLP_LED_PWR0:
            return sys_led_info_get(rv, local_id);
        default:
            return ONLP_STATUS_E_INVALID;
    }

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
    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);
    *rv = led_info[local_id].status;

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
    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);
    if(local_id >= ONLP_LED_MAX) {
        return ONLP_STATUS_E_INVALID;
    } else {
        *rv = led_info[local_id].hdr;
    }
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
    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);

    if(on_or_off) {
        // id led case
        if(local_id == ONLP_LED_ID) {
            return onlp_ledi_mode_set(id, ONLP_LED_MODE_BLUE);
        }
        return onlp_ledi_mode_set(id, ONLP_LED_MODE_GREEN);
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
 * @brief Get the caps for the given LED
 * @param id The LED ID
 * @param[out] rv Receives the caps.
 */
int onlp_ledi_caps_get(onlp_oid_t id, uint32_t* rv)
{
    int local_id = ONLP_OID_ID_GET(id);

    *rv = led_info[local_id].caps;

    return ONLP_STATUS_OK;
}

/**
 * @brief Set the LED mode.
 * @param id The LED OID
 * @param mode The new mode.
 * @notes Only called if the mode is advertised in the LED capabilities.
 */
int onlp_ledi_mode_set(onlp_oid_t id, onlp_led_mode_t mode)
{
    int local_id;
    char *fsname;
    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);

    int led_val = 0,led_color = 0, led_blink = 0, led_onoff = 0;

    // only led_id and sys led support set function
    switch(local_id) {
        case ONLP_LED_ID:
            fsname = SYSFS_LED_ID_NAME;
            break;
        case ONLP_LED_SYS:
            fsname = SYSFS_LED_SYS_NAME;
            break;
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
    }

    switch(mode) {
        case ONLP_LED_MODE_GREEN:
        case ONLP_LED_MODE_BLUE:
            led_color=1;
            led_blink=0;
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
        case ONLP_LED_MODE_GREEN_BLINKING:
        case ONLP_LED_MODE_BLUE_BLINKING:
            led_color=1;
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


    ONLP_TRY(file_read_hex(&led_val, SYSFS_LED_FMT, fsname));

    led_val = (led_color) ? led_val|LED_COLOR_MASK: led_val&(~LED_COLOR_MASK);
    led_val = (led_blink) ? led_val|LED_BLINK_MASK: led_val&(~LED_BLINK_MASK);
    led_val = (led_onoff) ? led_val|LED_ONOFF_MASK: led_val&(~LED_ONOFF_MASK);

    ONLP_TRY(onlp_file_write_int(led_val, SYSFS_LED_FMT, fsname));

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

