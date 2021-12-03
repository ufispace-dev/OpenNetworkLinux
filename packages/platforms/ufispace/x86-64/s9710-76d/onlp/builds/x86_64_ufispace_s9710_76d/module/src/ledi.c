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
#define LED_SYSFS  "/sys/bus/i2c/devices/1-0030/cpld_system_led_%d"
#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_LED(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

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
    CHASSIS_LED_INFO(ONLP_LED_SYS_SYS, "Chassis LED 1 (SYS LED)"),
    CHASSIS_LED_INFO(ONLP_LED_SYS_FAN, "Chassis LED 2 (FAN LED)"),
    CHASSIS_LED_INFO(ONLP_LED_SYS_PSU_0, "Chassis LED 3 (PSU0 LED)"),
    CHASSIS_LED_INFO(ONLP_LED_SYS_PSU_1, "Chassis LED 4 (PSU1 LED)"),
    CHASSIS_LED_INFO(ONLP_LED_SYS_SYNC, "Chassis LED 5 (SYNC LED)"),
};

static int ufi_sys_led_info_get(int id, onlp_led_info_t* info)
{
    int value = 0;
    int sysfs_index = 0;
    int shift = 0, led_val = 0, led_val_color = 0, led_val_blink = 0, led_val_onoff = 0;

    if (id < ONLP_LED_MAX) {        
        sysfs_index=(id-ONLP_LED_SYS_SYS)/2;
        shift = ((id-ONLP_LED_SYS_SYS)%2)*4;    
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    ONLP_TRY(file_read_hex(&value, LED_SYSFS, sysfs_index));

    led_val = (value >> shift);
    led_val_color = (led_val >> 0) & 1;
    led_val_blink = (led_val >> 2) & 1;
    led_val_onoff = (led_val >> 3) & 1;

    //onoff
    if (led_val_onoff == 0) {
        info->mode = ONLP_LED_MODE_OFF;
    } else {
        //color
        if (led_val_color == 0) {
            info->mode = ONLP_LED_MODE_YELLOW;
        } else {
            info->mode = ONLP_LED_MODE_GREEN;
        }
        //blinking
        if (led_val_blink == 1) {
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
    int led_id = 0, rc = ONLP_STATUS_OK;
    VALIDATE(id);
    
    led_id = ONLP_OID_ID_GET(id);
    *rv = led_info[led_id];

    switch (led_id) {        
        case ONLP_LED_SYS_SYS ... ONLP_LED_SYS_SYNC:
            rc = ufi_sys_led_info_get(led_id, rv);
            break;        
        default:            
            return ONLP_STATUS_E_INTERNAL;
            break;
    }

    return rc;
}

/**
 * @brief Get the LED operational status.
 * @param id The LED OID
 * @param rv [out] Receives the operational status.
 */
int onlp_ledi_status_get(onlp_oid_t id, uint32_t* rv)
{
    int result = ONLP_STATUS_OK;
    onlp_led_info_t info;
    VALIDATE(id);

    result = onlp_ledi_info_get(id, &info);
    *rv = info.status;

    return result;
}

/**
 * @brief Get the LED header.
 * @param id The LED OID
 * @param rv [out] Receives the header.
 */
int onlp_ledi_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* rv)
{
    int result = ONLP_STATUS_OK;
    onlp_led_info_t* info = NULL;
    int led_id = 0;
    VALIDATE(id);

    led_id = ONLP_OID_ID_GET(id);
    if(led_id >= ONLP_LED_MAX) {
        result = ONLP_STATUS_E_INVALID;
    } else {
        info = &led_info[led_id];
        *rv = info->hdr;
    }
    return result;
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

