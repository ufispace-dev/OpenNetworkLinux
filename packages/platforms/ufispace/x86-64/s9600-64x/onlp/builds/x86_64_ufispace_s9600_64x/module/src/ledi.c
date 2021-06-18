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
#include <stdio.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include "platform_lib.h"

/*
 * Get the information for the given LED OID.
 */

static onlp_led_info_t led_info[] =
{
    { }, // Not used *
    {
        { LED_OID_SYNC, "Chassis LED 1 (SYNC LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING |
        ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING ,
    },
    {
        { LED_OID_SYS, "Chassis LED 2 (SYS LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING |
        ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING ,
    },
    {
        { LED_OID_FAN, "Chassis LED 3 (FAN LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING |
        ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING ,
    },
    {
        { LED_OID_PSU0, "Chassis LED 4 (PSU0 LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING |
        ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING ,
    },
    {
        { LED_OID_PSU1, "Chassis LED 5 (PSU1 LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING |
        ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING ,
    },
    {
        { LED_OID_ID, "Chassis LED 6 (ID LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_BLUE | ONLP_LED_CAPS_BLUE_BLINKING,
    },
};


extern int sys_fan_info_get(onlp_fan_info_t* info, int id);

/**
 * @brief Initialize the LED subsystem.
 */
int
onlp_ledi_init(void)
{
    lock_init();
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the information for the given LED
 * @param id The LED OID
 * @param rv [out] Receives the LED information.
 */
int
onlp_ledi_info_get(onlp_oid_t id, onlp_led_info_t* info)
{
    int led_id, rc=ONLP_STATUS_OK;
    
    led_id = ONLP_OID_ID_GET(id);
    *info = led_info[led_id];

    switch (led_id) {        
        case LED_ID_SYS_SYNC:
        case LED_ID_SYS_SYS:
        case LED_ID_SYS_FAN:
        case LED_ID_SYS_PSU0:
        case LED_ID_SYS_PSU1:
        case LED_ID_SYS_ID:
            rc = sys_led_info_get(info, led_id);
            break;        
        default:            
            return ONLP_STATUS_E_INTERNAL;
            break;
    }

    return rc;
}

/**
 * @brief Turn an LED on or off
 * @param id The LED OID
 * @param on_or_off (boolean) on if 1 off if 0
 * @param This function is only relevant if the ONOFF capability is set.
 * @notes See onlp_led_set() for a description of the default behavior.
 */
int
onlp_ledi_set(onlp_oid_t id, int on_or_off)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Set the LED mode.
 * @param id The LED OID
 * @param mode The new mode.
 * @notes Only called if the mode is advertised in the LED capabilities.
 */
int
onlp_ledi_mode_set(onlp_oid_t id, onlp_led_mode_t mode)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief LED ioctl
 * @param id The LED OID
 * @param vargs The variable argument list for the ioctl call.
 */
int
onlp_ledi_ioctl(onlp_oid_t id, va_list vargs)
{
    return ONLP_STATUS_OK;
}
