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
#define FLEXE_LED_CAPS ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_GREEN
#define SYSFS_LED_CTRL1  LPC_FMT "led_ctrl_1"
#define SYSFS_LED_CTRL2  LPC_FMT "led_ctrl_2"
#define SYSFS_LED_STATUS LPC_FMT "led_status_1"

#define FLEXE0_G_PIN  395
#define FLEXE0_Y_PIN  394
#define FLEXE1_G_PIN  393
#define FLEXE1_Y_PIN  392


#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_LED(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

#define CHASSIS_LED_INFO(id, desc)               \
    {                                            \
        { ONLP_LED_ID_CREATE(id), #desc, POID_0},\
        LED_STATUS,                              \
        LED_CAPS,                                \
    }

#define CHASSIS_FLEXE_LED_INFO(id, desc)               \
    {                                            \
        { ONLP_LED_ID_CREATE(id), #desc, POID_0},\
        LED_STATUS,                              \
        FLEXE_LED_CAPS,                                \
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
    CHASSIS_FLEXE_LED_INFO(ONLP_LED_FLEXE_0, "Chassis LED 5 (FlexE 0)"),
    CHASSIS_FLEXE_LED_INFO(ONLP_LED_FLEXE_1, "Chassis LED 6 (FlexE 1)")
};

static int ufi_sys_led_info_get(onlp_led_info_t* info, int id)
{
    int value1, value2;
    int shift, led_val,led_val_color, led_val_blink, led_val_onoff;
    int gpio_g , gpio_y;
    char *sysfs = NULL;

    if (id <= ONLP_LED_SYS_GNSS && id >= ONLP_LED_SYS_PWR) {

        switch(id) {
            case ONLP_LED_SYS_GNSS:
                sysfs = SYSFS_LED_CTRL1;
                shift = 0;
                break;
            case ONLP_LED_SYS_SYNC:
                sysfs = SYSFS_LED_CTRL2;
                shift = 0;
                break;
            case ONLP_LED_SYS_SYS:
                sysfs = SYSFS_LED_CTRL1;
                shift = 4;
                break;
            case ONLP_LED_SYS_FAN:
                sysfs = SYSFS_LED_STATUS;
                shift = 0;
                break;
            case ONLP_LED_SYS_PWR:
                sysfs = SYSFS_LED_STATUS;
                shift = 4;
                break;
            default:
                return ONLP_STATUS_E_INTERNAL;
        }

        if (file_read_hex(&value1, sysfs) < 0) {
            return ONLP_STATUS_E_INTERNAL;
        }

        led_val = (value1 >> shift);
        led_val_color = (led_val >> 0) & 1;
        led_val_blink = (led_val >> 2) & 1;
        led_val_onoff = (led_val >> 3) & 1;

        //onoff
        if (led_val_onoff == 0) {
            info->mode = ONLP_LED_MODE_OFF;
        } else {
            //color
            if (led_val_color == 0) {
                info->mode = (led_val_blink == 1) ? ONLP_LED_MODE_YELLOW_BLINKING : ONLP_LED_MODE_YELLOW;

            } else {
                info->mode = (led_val_blink == 1) ? ONLP_LED_MODE_GREEN_BLINKING : ONLP_LED_MODE_GREEN;
            }
        }
    } else if (id <= ONLP_LED_FLEXE_0 && id >= ONLP_LED_FLEXE_1) {
        switch(id) {
            case ONLP_LED_FLEXE_0:
                gpio_g = FLEXE0_G_PIN;
                gpio_y = FLEXE0_Y_PIN;
                break;
            case ONLP_LED_FLEXE_1:
                gpio_g = FLEXE1_G_PIN;
                gpio_y = FLEXE1_Y_PIN;
                break;
            default:
                return ONLP_STATUS_E_INTERNAL;
        }

        if (file_read_hex(&value1, SYS_GPIO_FMT, gpio_g) < 0) {
            return ONLP_STATUS_E_INTERNAL;
        }

        if (file_read_hex(&value2, SYS_GPIO_FMT, gpio_y) < 0) {
            return ONLP_STATUS_E_INTERNAL;
        }

        if ((value1 == 1) && (value2 == 0)) {
            info->mode = ONLP_LED_MODE_GREEN;
        } else if ((value1 == 0) && (value2 == 1)) {
            info->mode = ONLP_LED_MODE_YELLOW;
        } else if ((value1 == 0) && (value2 == 0)) {
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
    int led_id, rc=ONLP_STATUS_OK;
    VALIDATE(id);
    
    led_id = ONLP_OID_ID_GET(id);
    *rv = led_info[led_id];

    switch (led_id) {        
        case ONLP_LED_SYS_GNSS:
        case ONLP_LED_SYS_SYNC:
        case ONLP_LED_SYS_SYS:
        case ONLP_LED_SYS_FAN:
        case ONLP_LED_SYS_PWR:
        case ONLP_LED_FLEXE_0:
        case ONLP_LED_FLEXE_1:
            rc = ufi_sys_led_info_get(rv, led_id);
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
    onlp_led_info_t* info;
    int led_id;
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
    VALIDATE(id);

    if (ONLP_OID_ID_GET(id) != ONLP_LED_FLEXE_0 && ONLP_OID_ID_GET(id) != ONLP_LED_FLEXE_1) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if (on_or_off) {
        return onlp_ledi_mode_set(id, ONLP_LED_MODE_GREEN);
    } else {
        return onlp_ledi_mode_set(id, ONLP_LED_MODE_OFF);
    }

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
    int gpio_g , gpio_y, value1, value2;

    VALIDATE(id);

    switch(ONLP_OID_ID_GET(id)) {
        case ONLP_LED_FLEXE_0:
            gpio_g = FLEXE0_G_PIN;
            gpio_y = FLEXE0_Y_PIN;
            break;
        case ONLP_LED_FLEXE_1:
            gpio_g = FLEXE1_G_PIN;
            gpio_y = FLEXE1_Y_PIN;
            break;
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
    }

    switch(mode) {
        case ONLP_LED_MODE_GREEN:
            value1 = 1;
            value2 = 0;
            break;
        case ONLP_LED_MODE_YELLOW:
            value1 = 0;
            value2 = 1;
            break;
        case ONLP_LED_MODE_OFF:
            value1 = 0;
            value2 = 0;
            break;
        default:
            return ONLP_STATUS_E_PARAM;
    }


    if (onlp_file_write_int(value1, SYS_GPIO_FMT, gpio_g) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (onlp_file_write_int(value2, SYS_GPIO_FMT, gpio_y) != ONLP_STATUS_OK) {
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

