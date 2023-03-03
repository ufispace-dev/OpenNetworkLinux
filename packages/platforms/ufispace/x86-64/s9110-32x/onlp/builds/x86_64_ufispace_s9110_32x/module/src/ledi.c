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
#define ID_LED_CAPS   ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_BLUE | ONLP_LED_CAPS_BLUE_BLINKING
#define FLEXE_LED_CAPS ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_GREEN

#define CHASSIS_LED_INFO(id, desc)               \
    {                                            \
        { ONLP_LED_ID_CREATE(id), desc, POID_0},\
        LED_STATUS,                              \
        LED_CAPS,                                \
    }

#define CHASSIS_FLEXE_LED_INFO(id, desc)               \
    {                                            \
        { ONLP_LED_ID_CREATE(id), desc, POID_0},\
        LED_STATUS,                              \
        FLEXE_LED_CAPS,                                \
    }

#define CHASSIS_ID_LED_INFO(id, desc)               \
    {                                            \
        { ONLP_LED_ID_CREATE(id), desc, POID_0},\
        LED_STATUS,                              \
        ID_LED_CAPS,                                \
    }


/*
 * Get the information for the given LED OID.
 */
static onlp_led_info_t led_info[] =
{
    { }, // Not used *
    CHASSIS_LED_INFO(ONLP_LED_SYS_SYS  , "Chassis LED 1 (SYS LED)"),
    CHASSIS_LED_INFO(ONLP_LED_SYS_FAN  , "Chassis LED 2 (FAN LED)"),
    CHASSIS_LED_INFO(ONLP_LED_SYS_PSU_0, "Chassis LED 3 (PSU0 LED)"),
    CHASSIS_LED_INFO(ONLP_LED_SYS_PSU_1, "Chassis LED 4 (PSU1 LED)"),
    CHASSIS_ID_LED_INFO(ONLP_LED_SYS_ID, "Chassis LED 5 (ID LED)"),
};

typedef enum led_act_e {
    ACTION_LED_RO = 0,
    ACTION_LED_RW,
    ACTION_LED_ATTR_MAX,
} led_act_t;
typedef struct
{
    led_act_t action;
    int attr;
    int color_bit;
    int blink_bit;
    int onoff_bit;
} led_attr_t;

typedef enum cpld_attr_idx_e {
    CPLD_LED_SYS = 0,
    CPLD_LED_FAN,
    CPLD_LED_PSU,
    CPLD_LED_ID,
    CPLD_NONE,
} cpld_attr_idx_t;

static const led_attr_t led_attr[] = {
/*  led attribute            action         attr           color blink onoff */
    [ONLP_LED_SYS_SYS]    = {ACTION_LED_RO, CPLD_LED_SYS  ,4     ,6    ,7},
    [ONLP_LED_SYS_FAN]    = {ACTION_LED_RO, CPLD_LED_FAN  ,0     ,2    ,3},
    [ONLP_LED_SYS_PSU_0]  = {ACTION_LED_RO, CPLD_LED_PSU  ,0     ,2    ,3},
    [ONLP_LED_SYS_PSU_1]  = {ACTION_LED_RO, CPLD_LED_PSU  ,4     ,6    ,7},
    [ONLP_LED_SYS_ID]     = {ACTION_LED_RW, CPLD_LED_ID   ,-1    ,2    ,3},
};

static int get_led_sysfs(cpld_attr_idx_t idx, char** str)
{
    if(str == NULL)
        return ONLP_STATUS_E_PARAM;

    switch(idx) {
        case CPLD_LED_SYS:
           *str =  SYSFS_CPLD1 "cpld_system_led_sys";
           break;
        case CPLD_LED_FAN:
            *str = SYSFS_CPLD1 "cpld_system_led_fan";
            break;
        case CPLD_LED_PSU:
            *str = SYSFS_CPLD1 "cpld_system_led_psu";
            break;
        case CPLD_LED_ID:
            *str = SYSFS_CPLD1 "cpld_system_led_id";
            break;
        default:
            *str = "";
            return ONLP_STATUS_E_PARAM;
    }
    return ONLP_STATUS_OK;
}

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
        case ONLP_LED_SYS_SYS:
        case ONLP_LED_SYS_FAN:
        case ONLP_LED_SYS_PSU_0:
        case ONLP_LED_SYS_PSU_1:
        case ONLP_LED_SYS_ID:
            *local_id = tmp_id;
            return ONLP_STATUS_OK;
        default:
            return ONLP_STATUS_E_INVALID;
    }

    return ONLP_STATUS_E_INVALID;
}

static int get_sys_led_info(int local_id, onlp_led_info_t* info)
{

    char *sysfs = NULL;
    int led_val = 0,led_color = 0, led_blink = 0, led_onoff =0;
    int color_bit = 0, blink_bit = 0, onoff_bit = 0;

    *info = led_info[local_id];

    ONLP_TRY(get_led_sysfs(led_attr[local_id].attr,&sysfs));
    ONLP_TRY(read_file_hex(&led_val, sysfs));

    color_bit = led_attr[local_id].color_bit;
    blink_bit = led_attr[local_id].blink_bit;
    onoff_bit = led_attr[local_id].onoff_bit;
    led_color = (color_bit < 0) ? -1:get_bit_value(led_val, color_bit);
    led_blink = (blink_bit < 0) ? -1:get_bit_value(led_val, blink_bit);
    led_onoff = (onoff_bit < 0) ? -1:get_bit_value(led_val, onoff_bit);

    //onoff
    if (led_onoff == 0) {
        info->mode = ONLP_LED_MODE_OFF;
    } else {
        //color
        if (led_color == 0) {
            info->mode = (led_blink == 1) ? ONLP_LED_MODE_YELLOW_BLINKING : ONLP_LED_MODE_YELLOW;
        } else if (led_color < 0) {
            info->mode = (led_blink == 1) ? ONLP_LED_MODE_BLUE_BLINKING : ONLP_LED_MODE_BLUE;
        } else {
            info->mode = (led_blink == 1) ? ONLP_LED_MODE_GREEN_BLINKING : ONLP_LED_MODE_GREEN;
        }
    }

    return ONLP_STATUS_OK;
}

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
        if(local_id == ONLP_LED_SYS_ID)
            return onlp_ledi_mode_set(id, ONLP_LED_MODE_BLUE);
        else
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
    char *sysfs = NULL;
    int local_id;

    ONLP_TRY(get_led_local_id(id, &local_id));
    if (led_attr[local_id].action != ACTION_LED_RW) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

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
        case ONLP_LED_MODE_BLUE:
            led_color=-1;
            led_blink=0;
            led_onoff=1;
            break;
        case ONLP_LED_MODE_BLUE_BLINKING:
            led_color=-1;
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

    ONLP_TRY(get_led_sysfs(led_attr[local_id].attr,&sysfs));
    ONLP_TRY(read_file_hex(&led_val, sysfs));

    if(led_color >= 0) {
        led_val = operate_bit(led_val, led_attr[local_id].color_bit, led_color);
    }
    led_val = operate_bit(led_val, led_attr[local_id].blink_bit, led_blink);
    led_val = operate_bit(led_val, led_attr[local_id].onoff_bit, led_onoff);

    ONLP_TRY(onlp_file_write_int(led_val, sysfs));

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

