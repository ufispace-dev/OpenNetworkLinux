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
#include <stdio.h>
#include <string.h>
#include <onlp/platformi/ledi.h>
#include <onlplib/file.h>
#include <onlplib/i2c.h>
//#include <sys/mman.h>
//#include <fcntl.h>

#include "platform_lib.h"

/**
 * Get the information for the given LED OID.
 * 
 * [01] CHASSIS----[01] ONLP_LED_SYSTEM
 *            |----[02] ONLP_LED_PSU0
 *            |----[03] ONLP_LED_PSU1
 *            |----[04] ONLP_LED_FAN
 */
static onlp_led_info_t __onlp_led_info[ONLP_LED_COUNT] =
{
    { }, /* Not used */
    {   
        .hdr = { 
            .id = ONLP_LED_ID_CREATE(ONLP_LED_SYSTEM),
            .description = "Chassis LED 1 (SYS LED)",
            .poid = 0,
        },  
        .status = ONLP_LED_STATUS_PRESENT,
        .caps = (ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | 
                 ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING),
        .mode = ONLP_LED_MODE_OFF,
    },  
    {   
        .hdr = { 
            .id = ONLP_LED_ID_CREATE(ONLP_LED_PSU0),
            .description = "Chassis LED 2 (PSU0 LED)",
            .poid = 0,
        },  
        .status = ONLP_LED_STATUS_PRESENT,
        .caps = (ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | 
                 ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING),
        .mode = ONLP_LED_MODE_OFF,
    },  
    {   
        .hdr = {
            .id = ONLP_LED_ID_CREATE(ONLP_LED_PSU1),
            .description = "Chassis LED 3 (PSU1 LED)",
            .poid = 0,
        },
        .status = ONLP_LED_STATUS_PRESENT,
        .caps = (ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING |
                 ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING),
        .mode = ONLP_LED_MODE_OFF,
    },
    {
        .hdr = {
            .id = ONLP_LED_ID_CREATE(ONLP_LED_FAN),
            .description = "Chassis LED 4 (FAN LED)",
            .poid = 0,
        },
        .status = ONLP_LED_STATUS_PRESENT,
        .caps = (ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING |
                 ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING),
        .mode = ONLP_LED_MODE_OFF,
    },
};

/**
 * @brief Update the information structure for the given LED
 * @param id The LED Local ID
 * @param[out] info Receives the FAN information.
 */
static int update_ledi_info(int local_id, onlp_led_info_t* info)
{
    int led_reg_value = 0;
    int led_val_color = 0, led_val_blink = 0, led_val_onoff = 0;

    if (local_id == ONLP_LED_SYSTEM) {
        ONLP_TRY(file_read_hex(&led_reg_value, "/sys/bus/i2c/devices/2-0030/cpld_system_led_0"));
        led_val_color = (led_reg_value & 0b00010000) >> 4; //1: Green,    0: Yellow
        led_val_blink = (led_reg_value & 0b01000000) >> 6; //1: Blinking, 0: Solid
        led_val_onoff = (led_reg_value & 0b10000000) >> 7; //1: On,       0: Off
    } else if (local_id == ONLP_LED_PSU0) {
        ONLP_TRY(file_read_hex(&led_reg_value, "/sys/bus/i2c/devices/2-0030/cpld_system_led_1"));
        led_val_color = (led_reg_value & 0b00000001) >> 0; //1: Green, 0: Yellow
        led_val_blink = (led_reg_value & 0b00000100) >> 2; //1: Blinking, 0: Solid
        led_val_onoff = (led_reg_value & 0b00001000) >> 3; //1: On,       0: Off
    } else if (local_id == ONLP_LED_PSU1) {
        ONLP_TRY(file_read_hex(&led_reg_value, "/sys/bus/i2c/devices/2-0030/cpld_system_led_1"));
        led_val_color = (led_reg_value & 0b00010000) >> 4; //1: Green, 0: Yellow
        led_val_blink = (led_reg_value & 0b01000000) >> 6; //1: Blinking, 0: Solid
        led_val_onoff = (led_reg_value & 0b10000000) >> 7; //1: On,       0: Off
    } else if (local_id == ONLP_LED_FAN) {
        ONLP_TRY(file_read_hex(&led_reg_value, "/sys/bus/i2c/devices/2-0030/cpld_system_led_0"));
        led_val_color = (led_reg_value & 0b00000001) >> 0; //1: Green, 0: Yellow
        led_val_blink = (led_reg_value & 0b00000100) >> 2; //1: Blinking, 0: Solid
        led_val_onoff = (led_reg_value & 0b00001000) >> 3; //1: On,       0: Off
    } else {
        AIM_LOG_ERROR("unknown LED id, local_id(%d), func=%s\n", local_id, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    //onoff
    if (led_val_onoff == 0) {
        info->status &= ~ONLP_LED_STATUS_ON;
        info->mode = ONLP_LED_MODE_OFF;
    } else {
        info->status |= ONLP_LED_STATUS_ON;
        //color
        if (led_val_color == 0 && led_val_blink == 1) {
            info->mode = ONLP_LED_MODE_YELLOW_BLINKING;
        } else if (led_val_color == 0 && led_val_blink == 0) {
            info->mode = ONLP_LED_MODE_YELLOW;
        } else if (led_val_color == 1 && led_val_blink == 1) {
            info->mode = ONLP_LED_MODE_GREEN_BLINKING;
        } else if (led_val_color == 1 && led_val_blink == 0) {
            info->mode = ONLP_LED_MODE_GREEN;
        } else {
            info->mode = ONLP_LED_MODE_ON;
            AIM_LOG_ERROR("unknown LED color local_id(%d), led_val_color(%d), led_val_blink(%d), func=%s\n",
                    local_id, led_val_color, led_val_blink, __FUNCTION__);
            return ONLP_STATUS_E_INTERNAL;
        }
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
 * @param info [out] Receives the LED information.
 */
int onlp_ledi_info_get(onlp_oid_t id, onlp_led_info_t* info)
{
    int local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_led_info_t */
    memset(info, 0, sizeof(onlp_led_info_t));
    *info = __onlp_led_info[local_id];
    ONLP_TRY(onlp_ledi_hdr_get(id, &info->hdr));

    /* Update ledi onlp_led_info_t status */
    ONLP_TRY(onlp_ledi_status_get(id, &info->status));

    //get ledi info
    ONLP_TRY(update_ledi_info(local_id, info));

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the LED operational status.
 * @param id The LED OID
 * @param status [out] Receives the operational status.
 */
int onlp_ledi_status_get(onlp_oid_t id, uint32_t* status)
{
    /* The LEDs are always present */
    *status = ONLP_LED_STATUS_PRESENT;

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the LED header.
 * @param id The LED OID
 * @param hdr [out] Receives the header.
 */
int onlp_ledi_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* hdr)
{
    int local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_led_info_t */
    *hdr = __onlp_led_info[local_id].hdr;

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
    return ONLP_STATUS_E_UNSUPPORTED;
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
    return ONLP_STATUS_E_UNSUPPORTED;
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
