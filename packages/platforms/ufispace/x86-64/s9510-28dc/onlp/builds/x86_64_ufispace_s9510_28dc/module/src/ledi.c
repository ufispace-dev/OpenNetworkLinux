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
            .description = "Chassis LED 6 (FlexE 0)",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_LED_CAPS_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_GREEN,
    },
    {
        .hdr = {
            .id = ONLP_LED_ID_CREATE(ONLP_LED_FLEXE_1),
            .description = "Chassis LED 7 (FlexE 1)",
            .poid = ONLP_OID_CHASSIS,
            .status = ONLP_OID_STATUS_FLAG_PRESENT,
        },
        .caps = ONLP_LED_CAPS_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_GREEN,
    },
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
    [ONLP_LED_SYS_GNSS] = {TYPE_LED_ATTR_SYSFS ,ACTION_LED_RW ,-1    ,-1    ,LPC_FMT "led_ctrl_1"  ,0    ,2    ,3},
    [ONLP_LED_SYS_SYNC] = {TYPE_LED_ATTR_SYSFS ,ACTION_LED_RW ,-1    ,-1    ,LPC_FMT "led_ctrl_2"  ,0    ,2    ,3},
    [ONLP_LED_SYS_SYS]  = {TYPE_LED_ATTR_SYSFS ,ACTION_LED_RW ,-1    ,-1    ,LPC_FMT "led_ctrl_1"  ,4    ,6    ,7},
    [ONLP_LED_SYS_FAN]  = {TYPE_LED_ATTR_SYSFS ,ACTION_LED_RO ,-1    ,-1    ,LPC_FMT "led_status_1",0    ,2    ,3},
    [ONLP_LED_SYS_PWR]  = {TYPE_LED_ATTR_SYSFS ,ACTION_LED_RO ,-1    ,-1    ,LPC_FMT "led_status_1",4    ,6    ,7},
    [ONLP_LED_FLEXE_0]  = {TYPE_LED_ATTR_GPIO  ,ACTION_LED_RW ,395   ,394   ,NULL                  ,-1   ,-1   ,1},
    [ONLP_LED_FLEXE_1]  = {TYPE_LED_ATTR_GPIO  ,ACTION_LED_RW ,393   ,392   ,NULL                  ,-1   ,-1   ,1},
};


static int update_ledi_info(int local_id, onlp_led_info_t* info)
{

    if (local_id < ONLP_LED_SYS_GNSS || local_id >= ONLP_LED_MAX) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if(led_attr[local_id].type == TYPE_LED_ATTR_SYSFS) {
        int led_val = 0,led_color = 0, led_blink = 0, led_onoff =0;

        ONLP_TRY(file_read_hex(&led_val, led_attr[local_id].lpc_sysfs));

        led_color = (led_val >> led_attr[local_id].color_bit) & 1;
        led_blink = (led_val >> led_attr[local_id].blink_bit) & 1;
        led_onoff = (led_val >> led_attr[local_id].onoff_bit) & 1;

        //onoff
        if (led_onoff == 0) {
            info->mode = ONLP_LED_MODE_OFF;
        } else {
            //color
            if (led_color == 0) {
                info->mode = (led_blink == 1) ? ONLP_LED_MODE_YELLOW_BLINKING : ONLP_LED_MODE_YELLOW;
            } else {
                info->mode = (led_blink == 1) ? ONLP_LED_MODE_GREEN_BLINKING : ONLP_LED_MODE_GREEN;
            }
        }
    } else if (led_attr[local_id].type == TYPE_LED_ATTR_GPIO) {
        int g_val = 0, y_val = 0;

        ONLP_TRY(file_read_hex(&g_val, SYS_GPIO_FMT, led_attr[local_id].g_gpin));

        ONLP_TRY(file_read_hex(&y_val, SYS_GPIO_FMT, led_attr[local_id].y_gpin));

        if ((g_val == 1) && (y_val == 0)) {
            info->mode = ONLP_LED_MODE_GREEN;
        } else if ((g_val == 0) && (y_val == 1)) {
            info->mode = ONLP_LED_MODE_YELLOW;
        } else if ((g_val == 0) && (y_val == 0)) {
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
    int local_id = ONLP_OID_ID_GET(id);

    *rv = __onlp_led_info[local_id].caps;

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
    int local_id = ONLP_OID_ID_GET(id);

    if (local_id < ONLP_LED_SYS_GNSS || local_id >= ONLP_LED_MAX) {
        return ONLP_STATUS_E_INTERNAL;
    }

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

        ONLP_TRY(onlp_file_write_int(g_val, SYS_GPIO_FMT, led_attr[local_id].g_gpin));
        ONLP_TRY(onlp_file_write_int(y_val, SYS_GPIO_FMT, led_attr[local_id].y_gpin));

    } else {
            return ONLP_STATUS_E_INTERNAL;
    }

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

