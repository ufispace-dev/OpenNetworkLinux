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
        { ONLP_LED_ID_CREATE(id), desc, POID_0},\
        LED_STATUS,                              \
        LED_CAPS,                                \
    }

/*
 * Get the information for the given LED OID.
 */
static onlp_led_info_t led_info[] =
{
    { }, // Not used *
    CHASSIS_LED_INFO(ONLP_LED_SYS_GNSS, "Chassis LED 1 (GNSS LED)"),
    CHASSIS_LED_INFO(ONLP_LED_SYS_SYNC, "Chassis LED 2 (SYNC LED)"),
    CHASSIS_LED_INFO(ONLP_LED_SYS_SYS, "Chassis LED 3 (STAT LED)"),
    CHASSIS_LED_INFO(ONLP_LED_SYS_FAN, "Chassis LED 4 (FAN LED)"),
    CHASSIS_LED_INFO(ONLP_LED_SYS_PWR, "Chassis LED 5 (PWR LED)"),
};

typedef enum led_attr_type_e {
    TYPE_LED_ATTR_UNNKOW = 0,
    TYPE_LED_ATTR_GPIO,
    TYPE_LED_ATTR_SYSFS,
    TYPE_LED_ATTR_MAX,
} led_type_t;

typedef enum led_act_e {
    ACTION_LED_RO = 0,
    ACTION_LED_RW,
    ACTION_LED_ATTR_MAX,
} led_act_t;

typedef struct
{
    led_type_t type;
    led_act_t action;
    int g_gpin;
    int y_gpin;
    char *lpc_sysfs;
    int color_bit;
    int blink_bit;
    int onoff_bit;
} led_attr_t;

static const led_attr_t led_attr[] = {
/*  led attribute          type                 action         g_gpin y_gpin lpc_sysfs              color blink onoff */
    [ONLP_LED_SYS_GNSS] = {TYPE_LED_ATTR_SYSFS ,ACTION_LED_RW ,-1    , -1   ,LPC_FMT "led_ctrl_1"  ,0    ,2    ,3},
    [ONLP_LED_SYS_SYNC] = {TYPE_LED_ATTR_SYSFS ,ACTION_LED_RW ,-1    , -1   ,LPC_FMT "led_ctrl_2"  ,0    ,2    ,3},
    [ONLP_LED_SYS_SYS]  = {TYPE_LED_ATTR_SYSFS ,ACTION_LED_RW ,-1    , -1   ,LPC_FMT "led_ctrl_1"  ,4    ,6    ,7},
    [ONLP_LED_SYS_FAN]  = {TYPE_LED_ATTR_SYSFS ,ACTION_LED_RO ,-1    , -1   ,LPC_FMT "led_status_1",0    ,2    ,3},
    [ONLP_LED_SYS_PWR]  = {TYPE_LED_ATTR_SYSFS ,ACTION_LED_RO ,-1    , -1   ,LPC_FMT "led_status_1",4    ,6    ,7},
};

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
        case ONLP_LED_SYS_SYS:
        case ONLP_LED_SYS_FAN:
        case ONLP_LED_SYS_PWR:
            *local_id = tmp_id;
            return ONLP_STATUS_OK;
        default:
            return ONLP_STATUS_E_INVALID;
    }

    return ONLP_STATUS_E_INVALID;
}

static int ufi_sys_led_info_get(int local_id, onlp_led_info_t* info)
{

    if(led_attr[local_id].type == TYPE_LED_ATTR_SYSFS) {
        int led_val = 0,led_color = 0, led_blink = 0, led_onoff =0;

        ONLP_TRY(file_read_hex(&led_val, led_attr[local_id].lpc_sysfs));

        led_color = (led_val >> led_attr[local_id].color_bit) & 1;
        led_blink = (led_val >> led_attr[local_id].blink_bit) & 1;
        led_onoff = (led_val >> led_attr[local_id].onoff_bit) & 1;

        //onoff
        if (led_onoff == 0) {
            info->status &= ~ONLP_LED_STATUS_ON;
            info->mode = ONLP_LED_MODE_OFF;
        } else {
            info->status |= ONLP_LED_STATUS_ON;
            //color
            if (led_color == 0) {
                info->mode = (led_blink == 1) ? ONLP_LED_MODE_YELLOW_BLINKING : ONLP_LED_MODE_YELLOW;
            } else {
                info->mode = (led_blink == 1) ? ONLP_LED_MODE_GREEN_BLINKING : ONLP_LED_MODE_GREEN;
            }
        }
    } else if (led_attr[local_id].type == TYPE_LED_ATTR_GPIO) {
        int gpio_max = 0;
        int g_val = 0, y_val = 0;

        ONLP_TRY(ufi_get_gpio_max(&gpio_max));

        ONLP_TRY(file_read_hex(&g_val, SYS_GPIO_FMT, gpio_max - led_attr[local_id].g_gpin));
        ONLP_TRY(file_read_hex(&y_val, SYS_GPIO_FMT, gpio_max - led_attr[local_id].y_gpin));

        if ((g_val == 1) && (y_val == 0)) {
            info->status |= ONLP_LED_STATUS_ON;
            info->mode = ONLP_LED_MODE_GREEN;
        } else if ((g_val == 0) && (y_val == 1)) {
            info->status |= ONLP_LED_STATUS_ON;
            info->mode = ONLP_LED_MODE_YELLOW;
        } else if ((g_val == 0) && (y_val == 0)) {
            info->status &= ~ONLP_LED_STATUS_ON;
            info->mode = ONLP_LED_MODE_OFF;
        } else {
            return ONLP_STATUS_E_INTERNAL;
        }
    } else {
            return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Initialize the LED subsystem.
 */
int onlp_ledi_init(void)
{
    lock_init();
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
    *rv = led_info[local_id];
    ONLP_TRY(onlp_ledi_status_get(id, &rv->status));

    if((rv->status & ONLP_LED_STATUS_PRESENT) == 0) {
        return ONLP_STATUS_OK;
    }

    ONLP_TRY(ufi_sys_led_info_get(local_id, rv));

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

    ONLP_TRY(get_led_local_id(id, &local_id));
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
 * @brief Set the LED mode.
 * @param id The LED OID
 * @param mode The new mode.
 * @notes Only called if the mode is advertised in the LED capabilities.
 */
int onlp_ledi_mode_set(onlp_oid_t id, onlp_led_mode_t mode)
{
    int local_id;

    ONLP_TRY(get_led_local_id(id, &local_id));
    if (led_attr[local_id].action != ACTION_LED_RW) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if(led_attr[local_id].type == TYPE_LED_ATTR_SYSFS) {
        int led_val = 0,led_color = 0, led_blink = 0, led_onoff =0;
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

       ONLP_TRY(file_read_hex(&led_val, led_attr[local_id].lpc_sysfs));

       led_val = ufi_bit_operation(led_val, led_attr[local_id].color_bit, led_color);
       led_val = ufi_bit_operation(led_val, led_attr[local_id].blink_bit, led_blink);
       led_val = ufi_bit_operation(led_val, led_attr[local_id].onoff_bit, led_onoff);

       ONLP_TRY(onlp_file_write_int(led_val, led_attr[local_id].lpc_sysfs));

    } else if (led_attr[local_id].type == TYPE_LED_ATTR_GPIO) {
        int gpio_max = 0;
        int g_val = 0, y_val = 0;
        switch(mode) {
            case ONLP_LED_MODE_GREEN:
                g_val = 1;
                y_val = 0;
                break;
            case ONLP_LED_MODE_YELLOW:
                g_val = 0;
                y_val = 1;
                break;
            case ONLP_LED_MODE_OFF:
                g_val = 0;
                y_val = 0;
                break;
            default:
                return ONLP_STATUS_E_UNSUPPORTED;
        }


        ONLP_TRY(ufi_get_gpio_max(&gpio_max));

        ONLP_TRY(onlp_file_write_int(g_val, SYS_GPIO_FMT, gpio_max - led_attr[local_id].g_gpin));
        ONLP_TRY(onlp_file_write_int(y_val, SYS_GPIO_FMT, gpio_max - led_attr[local_id].y_gpin));

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

