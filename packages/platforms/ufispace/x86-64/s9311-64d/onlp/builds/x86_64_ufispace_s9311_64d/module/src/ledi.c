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
#define LED_CAPS ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | \
                     ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING
#define ID_LED_CAPS ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_BLUE | ONLP_LED_CAPS_BLUE_BLINKING
#define REG_INVALID -1

#define IS_GPIO(_node) (_node.type == TYPE_LED_ATTR_GPIO)
#define IS_CPLD(_node) (_node.type == TYPE_LED_ATTR_CPLD)

/*
 *  Generally, the color bit for CPLD is 4 bits, and there are 16 color sets available.
 *  The color bit for GPIO is 2 bits (representing two GPIO pins), and there are 4 color sets.
 *  Therefore, we use the 16 color sets available for our application.
 */
#define COLOR_VAL_MAX 16
#define GPIO_INVALID_OFFSET -999999

#define VALUE_0000_0000 0x00
#define VALUE_0000_0001 0x01
#define VALUE_0000_0100 0x04
#define VALUE_0000_0101 0x05
#define VALUE_0000_1000 0x08
#define VALUE_0000_1001 0x09
#define VALUE_0000_1100 0x0C
#define VALUE_0000_1101 0x0D

typedef struct
{
    short int val;
    int mode;
} color_obj_t;

typedef struct
{
    int type;
    char *sysfs;
    int gpin_off1;
    int gpin_off2;
    int action;
    color_obj_t color_obj[COLOR_VAL_MAX];
    short int color_mask;
    char *desc;
    uint32_t caps;
} led_node_t;

typedef enum led_act_e
{
    ACTION_LED_RO = 0,
    ACTION_LED_RW,
    ACTION_LED_ATTR_MAX,
} led_act_t;

typedef enum led_attr_type_e
{
    TYPE_LED_ATTR_UNNKOW = 0,
    TYPE_LED_ATTR_GPIO,
    TYPE_LED_ATTR_CPLD,
    TYPE_LED_ATTR_MAX,
} led_type_t;

/*
 * Get the information for the given LED OID.
 */
static onlp_led_info_t led_info[] = {
    {}, // Not used *
    {
        .hdr = {
            .id = ONLP_LED_ID_CREATE(ONLP_LED_SYS_SYS),
            .description = "Chassis LED 1 (SYS LED)",
            .poid = POID_0,
        },
        .status = LED_STATUS,
        .caps = LED_CAPS,
    },
    {
        .hdr = {
            .id = ONLP_LED_ID_CREATE(ONLP_LED_SYS_FAN),
            .description = "Chassis LED 2 (FAN LED)",
            .poid = POID_0,
        },
        .status = LED_STATUS,
        .caps = LED_CAPS,
    },
    {
        .hdr = {
            .id = ONLP_LED_ID_CREATE(ONLP_LED_SYS_PSU_0),
            .description = "Chassis LED 3 (PSU0 LED)",
            .poid = POID_0,
        },
        .status = LED_STATUS,
        .caps = LED_CAPS,
    },
    {
        .hdr = {
            .id = ONLP_LED_ID_CREATE(ONLP_LED_SYS_PSU_1),
            .description = "Chassis LED 4 (PSU1 LED)",
            .poid = POID_0,
        },
        .status = LED_STATUS,
        .caps = LED_CAPS,
    },
    {
        .hdr = {
            .id = ONLP_LED_ID_CREATE(ONLP_LED_SYS_SYNC),
            .description = "Chassis LED 5 (SYNC LED)",
            .poid = POID_0,
        },
        .status = LED_STATUS,
        .caps = LED_CAPS,
    },
    {
        .hdr = {
            .id = ONLP_LED_ID_CREATE(ONLP_LED_SYS_ID),
            .description = "Chassis LED 6 (ID LED)",
            .poid = POID_0,
        },
        .status = LED_STATUS,
        .caps = ID_LED_CAPS,
    },
};

static int get_node(int local_id, led_node_t *node)
{
    if (node == NULL)
        return ONLP_STATUS_E_PARAM;

    switch (local_id)
    {
    case ONLP_LED_SYS_SYS:
        node->sysfs = SYSFS_CPLD1 "cpld_system_led_sys";
        node->action = ACTION_LED_RO;
        node->type = TYPE_LED_ATTR_CPLD;
        node->color_mask = VALUE_0000_1101;
        node->color_obj[0].val = VALUE_0000_0000;
        node->color_obj[0].mode = ONLP_LED_MODE_OFF;
        node->color_obj[1].val = VALUE_0000_0001;
        node->color_obj[1].mode = ONLP_LED_MODE_OFF;
        node->color_obj[2].val = VALUE_0000_0100;
        node->color_obj[2].mode = ONLP_LED_MODE_OFF;
        node->color_obj[3].val = VALUE_0000_0101;
        node->color_obj[3].mode = ONLP_LED_MODE_OFF;
        node->color_obj[4].val = VALUE_0000_1001;
        node->color_obj[4].mode = ONLP_LED_MODE_GREEN;
        node->color_obj[5].val = VALUE_0000_1101;
        node->color_obj[5].mode = ONLP_LED_MODE_GREEN_BLINKING;
        node->color_obj[6].val = VALUE_0000_1000;
        node->color_obj[6].mode = ONLP_LED_MODE_YELLOW;
        node->color_obj[7].val = VALUE_0000_1100;
        node->color_obj[7].mode = ONLP_LED_MODE_YELLOW_BLINKING;
        node->color_obj[8].val = node->color_obj[4].val;
        node->color_obj[8].mode = ONLP_LED_MODE_ON;
        node->color_obj[9].val = REG_INVALID;
        break;
    case ONLP_LED_SYS_FAN:
        node->sysfs = SYSFS_CPLD1 "cpld_system_led_fan";
        node->action = ACTION_LED_RO;
        node->type = TYPE_LED_ATTR_CPLD;
        node->color_mask = VALUE_0000_1101;
        node->color_obj[0].val = VALUE_0000_0000;
        node->color_obj[0].mode = ONLP_LED_MODE_OFF;
        node->color_obj[1].val = VALUE_0000_0001;
        node->color_obj[1].mode = ONLP_LED_MODE_OFF;
        node->color_obj[2].val = VALUE_0000_0100;
        node->color_obj[2].mode = ONLP_LED_MODE_OFF;
        node->color_obj[3].val = VALUE_0000_0101;
        node->color_obj[3].mode = ONLP_LED_MODE_OFF;
        node->color_obj[4].val = VALUE_0000_1001;
        node->color_obj[4].mode = ONLP_LED_MODE_GREEN;
        node->color_obj[5].val = VALUE_0000_1101;
        node->color_obj[5].mode = ONLP_LED_MODE_GREEN_BLINKING;
        node->color_obj[6].val = VALUE_0000_1000;
        node->color_obj[6].mode = ONLP_LED_MODE_YELLOW;
        node->color_obj[7].val = VALUE_0000_1100;
        node->color_obj[7].mode = ONLP_LED_MODE_YELLOW_BLINKING;
        node->color_obj[8].val = node->color_obj[4].val;
        node->color_obj[8].mode = ONLP_LED_MODE_ON;
        node->color_obj[9].val = REG_INVALID;
        break;
    case ONLP_LED_SYS_PSU_0:
        node->sysfs = SYSFS_CPLD1 "cpld_system_led_psu_0";
        node->action = ACTION_LED_RO;
        node->type = TYPE_LED_ATTR_CPLD;
        node->color_mask = VALUE_0000_1101;
        node->color_obj[0].val = VALUE_0000_0000;
        node->color_obj[0].mode = ONLP_LED_MODE_OFF;
        node->color_obj[1].val = VALUE_0000_0001;
        node->color_obj[1].mode = ONLP_LED_MODE_OFF;
        node->color_obj[2].val = VALUE_0000_0100;
        node->color_obj[2].mode = ONLP_LED_MODE_OFF;
        node->color_obj[3].val = VALUE_0000_0101;
        node->color_obj[3].mode = ONLP_LED_MODE_OFF;
        node->color_obj[4].val = VALUE_0000_1001;
        node->color_obj[4].mode = ONLP_LED_MODE_GREEN;
        node->color_obj[5].val = VALUE_0000_1101;
        node->color_obj[5].mode = ONLP_LED_MODE_GREEN_BLINKING;
        node->color_obj[6].val = VALUE_0000_1000;
        node->color_obj[6].mode = ONLP_LED_MODE_YELLOW;
        node->color_obj[7].val = VALUE_0000_1100;
        node->color_obj[7].mode = ONLP_LED_MODE_YELLOW_BLINKING;
        node->color_obj[8].val = node->color_obj[4].val;
        node->color_obj[8].mode = ONLP_LED_MODE_ON;
        node->color_obj[9].val = REG_INVALID;
        break;
    case ONLP_LED_SYS_PSU_1:
        node->sysfs = SYSFS_CPLD1 "cpld_system_led_psu_1";
        node->action = ACTION_LED_RO;
        node->type = TYPE_LED_ATTR_CPLD;
        node->color_mask = VALUE_0000_1101;
        node->color_obj[0].val = VALUE_0000_0000;
        node->color_obj[0].mode = ONLP_LED_MODE_OFF;
        node->color_obj[1].val = VALUE_0000_0001;
        node->color_obj[1].mode = ONLP_LED_MODE_OFF;
        node->color_obj[2].val = VALUE_0000_0100;
        node->color_obj[2].mode = ONLP_LED_MODE_OFF;
        node->color_obj[3].val = VALUE_0000_0101;
        node->color_obj[3].mode = ONLP_LED_MODE_OFF;
        node->color_obj[4].val = VALUE_0000_1001;
        node->color_obj[4].mode = ONLP_LED_MODE_GREEN;
        node->color_obj[5].val = VALUE_0000_1101;
        node->color_obj[5].mode = ONLP_LED_MODE_GREEN_BLINKING;
        node->color_obj[6].val = VALUE_0000_1000;
        node->color_obj[6].mode = ONLP_LED_MODE_YELLOW;
        node->color_obj[7].val = VALUE_0000_1100;
        node->color_obj[7].mode = ONLP_LED_MODE_YELLOW_BLINKING;
        node->color_obj[8].val = node->color_obj[4].val;
        node->color_obj[8].mode = ONLP_LED_MODE_ON;
        node->color_obj[9].val = REG_INVALID;
        break;
    case ONLP_LED_SYS_SYNC:
        node->sysfs = SYSFS_CPLD1 "cpld_system_led_sync";
        node->action = ACTION_LED_RW;
        node->type = TYPE_LED_ATTR_CPLD;
        node->color_mask = VALUE_0000_1101;
        node->color_obj[0].val = VALUE_0000_0000;
        node->color_obj[0].mode = ONLP_LED_MODE_OFF;
        node->color_obj[1].val = VALUE_0000_0001;
        node->color_obj[1].mode = ONLP_LED_MODE_OFF;
        node->color_obj[2].val = VALUE_0000_0100;
        node->color_obj[2].mode = ONLP_LED_MODE_OFF;
        node->color_obj[3].val = VALUE_0000_0101;
        node->color_obj[3].mode = ONLP_LED_MODE_OFF;
        node->color_obj[4].val = VALUE_0000_1001;
        node->color_obj[4].mode = ONLP_LED_MODE_GREEN;
        node->color_obj[5].val = VALUE_0000_1101;
        node->color_obj[5].mode = ONLP_LED_MODE_GREEN_BLINKING;
        node->color_obj[6].val = VALUE_0000_1000;
        node->color_obj[6].mode = ONLP_LED_MODE_YELLOW;
        node->color_obj[7].val = VALUE_0000_1100;
        node->color_obj[7].mode = ONLP_LED_MODE_YELLOW_BLINKING;
        node->color_obj[8].val = node->color_obj[4].val;
        node->color_obj[8].mode = ONLP_LED_MODE_ON;
        node->color_obj[9].val = REG_INVALID;
        break;
    case ONLP_LED_SYS_ID:
        node->sysfs = SYSFS_CPLD1 "cpld_system_led_id";
        node->action = ACTION_LED_RW;
        node->type = TYPE_LED_ATTR_CPLD;
        node->color_mask = VALUE_0000_1100;
        node->color_obj[0].val = VALUE_0000_0000;
        node->color_obj[0].mode = ONLP_LED_MODE_OFF;
        node->color_obj[1].val = VALUE_0000_0100;
        node->color_obj[1].mode = ONLP_LED_MODE_OFF;
        node->color_obj[2].val = VALUE_0000_1000;
        node->color_obj[2].mode = ONLP_LED_MODE_BLUE;
        node->color_obj[3].val = VALUE_0000_1100;
        node->color_obj[3].mode = ONLP_LED_MODE_BLUE_BLINKING;       
        node->color_obj[4].val = node->color_obj[2].val;
        node->color_obj[4].mode = ONLP_LED_MODE_ON;
        node->color_obj[5].val = REG_INVALID;
        break;
    default:
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

    if (local_id == NULL)
    {
        return ONLP_STATUS_E_PARAM;
    }

    if (!ONLP_OID_IS_LED(id))
    {
        return ONLP_STATUS_E_INVALID;
    }

    tmp_id = ONLP_OID_ID_GET(id);

    switch (tmp_id)
    {
    case ONLP_LED_SYS_SYS:
    case ONLP_LED_SYS_FAN:
    case ONLP_LED_SYS_PSU_0:
    case ONLP_LED_SYS_PSU_1:
    case ONLP_LED_SYS_SYNC:
    case ONLP_LED_SYS_ID:
        *local_id = tmp_id;
        return ONLP_STATUS_OK;
    default:
        return ONLP_STATUS_E_INVALID;
    }

    return ONLP_STATUS_E_INVALID;
}

static int update_hdr(onlp_oid_hdr_t *rv, led_node_t *node)
{
    // Not supoort
    if (0)
    {
        int len = 0;

        if (rv == NULL || node == NULL)
        {
            return ONLP_STATUS_E_PARAM;
        }

        if (node->desc != NULL)
        {
            memset(rv->description, 0, sizeof(rv->description));
            len = (sizeof(rv->description) > strlen(node->desc)) ? strlen(node->desc) : (sizeof(rv->description) - 1);
            memcpy(rv->description, node->desc, len);
        }
    }
    return ONLP_STATUS_OK;
}

static int update_info(onlp_led_info_t *info, led_node_t *node)
{
    // Not supoort
    if (0)
    {
        if (info == NULL || node == NULL)
        {
            return ONLP_STATUS_E_PARAM;
        }

        if (node->caps != 0)
        {
            info->caps = node->caps;
        }
        ONLP_TRY(update_hdr(&info->hdr, node));
    }
    return ONLP_STATUS_OK;
}

static int get_sys_led_info(int local_id, onlp_led_info_t *info)
{
    int led_val = 0;
    int i = 0;
    led_node_t node = {0};

    *info = led_info[local_id];

    ONLP_TRY(get_node(local_id, &node));
    ONLP_TRY(update_info(info, &node));

    if (IS_GPIO(node))
    {
        int led_val2 = 0;
        int gpio_max = 0;
        ONLP_TRY(get_gpio_max(&gpio_max));
        ONLP_TRY(read_file_hex(&led_val, SYS_GPIO_FMT, gpio_max + node.gpin_off1));
        if (node.gpin_off2 != GPIO_INVALID_OFFSET)
        {
            ONLP_TRY(read_file_hex(&led_val2, SYS_GPIO_FMT, gpio_max + node.gpin_off1));
            led_val = operate_bit(led_val, 1, led_val2);
        }
    }
    else
    {
        ONLP_TRY(read_file_hex(&led_val, node.sysfs));
    }

    for (i = 0; i < COLOR_VAL_MAX; i++)
    {
        if (node.color_obj[i].val == REG_INVALID)
            break;

        if ((led_val & node.color_mask) == node.color_obj[i].val)
        {

            if (node.color_obj[i].mode == ONLP_LED_MODE_OFF)
            {
                info->status &= ~ONLP_LED_STATUS_ON;
            }
            else
            {
                info->status |= ONLP_LED_STATUS_ON;
            }

            info->mode = node.color_obj[i].mode;
            break;
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
int onlp_ledi_info_get(onlp_oid_t id, onlp_led_info_t *rv)
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
int onlp_ledi_status_get(onlp_oid_t id, uint32_t *rv)
{
    int local_id;
    onlp_led_info_t info = {0};

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
int onlp_ledi_hdr_get(onlp_oid_t id, onlp_oid_hdr_t *rv)
{
    int local_id;
    led_node_t node = {0};

    ONLP_TRY(get_led_local_id(id, &local_id));
    *rv = led_info[local_id].hdr;

    ONLP_TRY(get_node(local_id, &node));
    ONLP_TRY(update_hdr(rv, &node));

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
    led_node_t node = {0};

    ONLP_TRY(get_led_local_id(id, &local_id));
    ONLP_TRY(get_node(local_id, &node));
    if (node.action != ACTION_LED_RW)
    {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if (on_or_off)
    {
        return onlp_ledi_mode_set(id, ONLP_LED_MODE_ON);
    }
    else
    {
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
    led_node_t node = {0};
    int i = 0;
    int found = 0;
    int led_val = 0;
    int led_val2 = 0;

    ONLP_TRY(get_led_local_id(id, &local_id));
    ONLP_TRY(get_node(local_id, &node));
    if (node.action != ACTION_LED_RW)
    {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    for (i = 0; i < COLOR_VAL_MAX; i++)
    {
        if (node.color_obj[i].val == REG_INVALID)
            break;

        if (mode == node.color_obj[i].mode)
        {
            found = 1;
            if (IS_GPIO(node))
            {
                int gpio_max = 0;
                ONLP_TRY(get_gpio_max(&gpio_max));
                led_val = get_bit_value(node.color_obj[i].val, 0);
                ONLP_TRY(onlp_file_write_int(led_val, SYS_GPIO_FMT, gpio_max + node.gpin_off1));
                if (node.gpin_off2 != GPIO_INVALID_OFFSET)
                {
                    led_val2 = get_bit_value(node.color_obj[i].val, 1);
                    ONLP_TRY(onlp_file_write_int(led_val2, SYS_GPIO_FMT, gpio_max + node.gpin_off2));
                }
            }
            else
            {
                ONLP_TRY(read_file_hex(&led_val, node.sysfs));
                led_val = unset_bit_mask(led_val, node.color_mask);
                led_val = led_val | node.color_obj[i].val;
                ONLP_TRY(onlp_file_write_int(led_val, node.sysfs));
            }
            break;
        }
    }

    if (found == 0)
    {
        return ONLP_STATUS_E_UNSUPPORTED;
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