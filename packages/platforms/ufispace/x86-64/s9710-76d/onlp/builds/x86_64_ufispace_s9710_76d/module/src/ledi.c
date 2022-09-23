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
#define LED_SYSFS  "/sys/bus/i2c/devices/1-0030/cpld_system_led_"
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
    CHASSIS_LED_INFO(ONLP_LED_SYS_SYNC, "Chassis LED 1 (SYNC LED)"),
    CHASSIS_LED_INFO(ONLP_LED_SYS_SYS, "Chassis LED 2 (SYS LED)"),
    CHASSIS_LED_INFO(ONLP_LED_SYS_FAN, "Chassis LED 3 (FAN LED)"),
    CHASSIS_LED_INFO(ONLP_LED_SYS_PSU_0, "Chassis LED 4 (PSU0 LED)"),
    CHASSIS_LED_INFO(ONLP_LED_SYS_PSU_1, "Chassis LED 5 (PSU1 LED)"),    
};

typedef struct
{
    char *sysfs;
    int color_bit;
    int blink_bit;
    int onoff_bit;
} led_attr_t;

static const led_attr_t led_attr[] = {
    /*led attribute          sysfs         color blink onoff */
    [ONLP_LED_SYS_SYNC]   = {LED_SYSFS "2" ,0    ,2    ,3},
    [ONLP_LED_SYS_SYS]    = {LED_SYSFS "0" ,0    ,2    ,3},
    [ONLP_LED_SYS_FAN]    = {LED_SYSFS "0" ,4    ,6    ,7},
    [ONLP_LED_SYS_PSU_0]  = {LED_SYSFS "1" ,0    ,2    ,3},
    [ONLP_LED_SYS_PSU_1]  = {LED_SYSFS "1" ,4    ,6    ,7},
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
        case ONLP_LED_SYS_SYNC:
        case ONLP_LED_SYS_SYS:
        case ONLP_LED_SYS_FAN:
        case ONLP_LED_SYS_PSU_0:
        case ONLP_LED_SYS_PSU_1:
            *local_id = tmp_id;
            return ONLP_STATUS_OK;
        default:
            return ONLP_STATUS_E_INVALID;
    }

    return ONLP_STATUS_E_INVALID;
}

static int ufi_sys_led_info_get(int id, onlp_led_info_t* info)
{
    int led_val = 0, led_val_color = 0, led_val_blink = 0, led_val_onoff = 0;
    
    if (id <= 0 || id >= ONLP_LED_MAX) {
        return ONLP_STATUS_E_PARAM;
    }

    ONLP_TRY(file_read_hex(&led_val, led_attr[id].sysfs));

    led_val_color = (led_val >> led_attr[id].color_bit) & 1;
    led_val_blink = (led_val >> led_attr[id].blink_bit) & 1;
    led_val_onoff = (led_val >> led_attr[id].onoff_bit) & 1;

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
    
    ONLP_TRY(get_led_local_id(id, &led_id));

    *rv = led_info[led_id];

    switch (led_id) {        
        case ONLP_LED_SYS_SYNC ... ONLP_LED_SYS_PSU_1:
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
    int led_id = 0;
    
    ONLP_TRY(get_led_local_id(id, &led_id));

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

    ONLP_TRY(get_led_local_id(id, &led_id));
	
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
    int local_id;

    ONLP_TRY(get_led_local_id(id, &local_id));

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

   ONLP_TRY(file_read_hex(&led_val, led_attr[local_id].sysfs));

   led_val = ufi_bit_operation(led_val, led_attr[local_id].color_bit, led_color);
   led_val = ufi_bit_operation(led_val, led_attr[local_id].blink_bit, led_blink);
   led_val = ufi_bit_operation(led_val, led_attr[local_id].onoff_bit, led_onoff);

   ONLP_TRY(onlp_file_write_int(led_val, led_attr[local_id].sysfs));


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
