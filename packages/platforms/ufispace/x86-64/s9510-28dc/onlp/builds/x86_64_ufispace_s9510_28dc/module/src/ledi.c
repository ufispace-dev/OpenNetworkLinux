/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
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
 * LED Implementation
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

/**
 * Get the information for the given LED OID.
 * 
 * [01] CHASSIS
 *            |----[01] ONLP_LED_SYS_GNSS
 *            |----[02] ONLP_LED_SYS_SYNC
 *            |----[03] ONLP_LED_SYS_SYS
 *            |----[04] ONLP_LED_SYS_FAN
 *            |----[05] ONLP_LED_SYS_PWR
 *            |----[06] ONLP_LED_FLEXE_0
 *            |----[07] ONLP_LED_FLEXE_1
 */

static onlp_led_info_t __onlp_led_info[] =
{
    { }, /* Not used */
    {
        .hdr = {
            .id = ONLP_LED_ID_CREATE(ONLP_LED_SYS_GNSS),
            .description = "Chassis LED 1 (GNSS LED)",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_LED_CAPS_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING |
                ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        .hdr = {
            .id = ONLP_LED_ID_CREATE(ONLP_LED_SYS_SYNC),
            .description = "Chassis LED 2 (SYNC LED)",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_LED_CAPS_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING |
                ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        .hdr = {
            .id = ONLP_LED_ID_CREATE(ONLP_LED_SYS_SYS),
            .description = "Chassis LED 3 (STAT LED)",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_LED_CAPS_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING |
                ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        .hdr = {
            .id = ONLP_LED_ID_CREATE(ONLP_LED_SYS_FAN),
            .description = "Chassis LED 4 (FAN LED)",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_LED_CAPS_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING |
                ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        .hdr = {
            .id = ONLP_LED_ID_CREATE(ONLP_LED_SYS_PWR),
            .description = "Chassis LED 5 (PWR LED)",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_LED_CAPS_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING |
                ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        .hdr = {
            .id = ONLP_LED_ID_CREATE(ONLP_LED_FLEXE_0),
            .description = "Chassis LED 5 (FlexE 0)",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_LED_CAPS_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_GREEN,
    },
    {
        .hdr = {
            .id = ONLP_LED_ID_CREATE(ONLP_LED_FLEXE_1),
            .description = "Chassis LED 6 (FlexE 1)",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_LED_CAPS_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_GREEN,
    },
};

static int update_ledi_info(int local_id, onlp_led_info_t* info)
{
    int value1, value2;
    int shift, led_val,led_val_color, led_val_blink, led_val_onoff;
    int gpio_g , gpio_y;
    char *sysfs = NULL;

    if (local_id >= ONLP_LED_SYS_GNSS && local_id <= ONLP_LED_SYS_PWR) {

        switch(local_id) {
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
    } else if (local_id >= ONLP_LED_FLEXE_0 && local_id <= ONLP_LED_FLEXE_1) {
        switch(local_id) {
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
 * @brief Software initialization of the LED module.
 */
int onlp_ledi_sw_init(void)
{
    lock_init();
    return ONLP_STATUS_OK;
}

/**
 * @brief Hardware initialization of the LED module.
 * @param flags The hardware initialization flags.
 */
int onlp_ledi_hw_init(uint32_t flags)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Deinitialize the led software module.
 * @note The primary purpose of this API is to properly
 * deallocate any resources used by the module in order
 * faciliate detection of real resouce leaks.
 */
int onlp_ledi_sw_denit(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Validate an LED id.
 * @param id The id.
 */
int onlp_ledi_id_validate(onlp_oid_id_t id)
{
    return ONLP_OID_ID_VALIDATE_RANGE(id, 1, ONLP_LED_MAX-1);
}

/**
 * @brief Get the LED header.
 * @param id The LED OID
 * @param[out] rv  Receives the header.
 */
int onlp_ledi_hdr_get(onlp_oid_id_t id, onlp_oid_hdr_t* hdr)
{
    int ret = ONLP_STATUS_OK;
    int local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_led_info_t */
    *hdr = __onlp_led_info[local_id].hdr;

    return ret;
}

/**
 * @brief Get the information for the given LED
 * @param id The LED OID
 * @param[out] rv  Receives the LED information.
 */
int onlp_ledi_info_get(onlp_oid_id_t id, onlp_led_info_t* info)
{
    int ret = ONLP_STATUS_OK;
    int local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_led_info_t */
    memset(info, 0, sizeof(onlp_led_info_t));
    *info = __onlp_led_info[local_id];
    ONLP_TRY(onlp_ledi_hdr_get(id, &info->hdr));

    //get ledi info
    ret = update_ledi_info(local_id, info);

    return ret;
}

/**
 * @brief Get the caps for the given LED
 * @param id The LED ID
 * @param[out] rv Receives the caps.
 */
int onlp_ledi_caps_get(onlp_oid_id_t id, uint32_t* rv)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Set the LED mode.
 * @param id The LED OID
 * @param mode The new mode.
 * @note Only called if the mode is advertised in the LED capabilities.
 */
int onlp_ledi_mode_set(onlp_oid_id_t id, onlp_led_mode_t mode)
{
    int gpio_g , gpio_y, value1, value2;
    int local_id = ONLP_OID_ID_GET(id);

    switch(local_id) {
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

    ONLP_TRY(onlp_file_write_int(value1, SYS_GPIO_FMT, gpio_g));
    ONLP_TRY(onlp_file_write_int(value2, SYS_GPIO_FMT, gpio_y));

    return ONLP_STATUS_OK;
}

/**
 * @brief Set the LED character.
 * @param id The LED OID
 * @param c The character..
 * @note Only called if the char capability is set.
 */
int onlp_ledi_char_set(onlp_oid_id_t id, char c)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

