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
 *
 *
 ***********************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <onlp/platformi/sfpi.h>
#include <onlplib/i2c.h>
#include <onlplib/file.h>
#include "platform_lib.h"

#define QSFPDD_NUM          6
#define QSFP_NUM            12
#define MGMT_SFP_NUM        2
#define SFP_NUM             64
#define PORT_NUM            SFP_NUM + QSFP_NUM + QSFPDD_NUM + MGMT_SFP_NUM
#define QSFPDD_BASE_BUS     105
#define QSFP_BASE_BUS       89
#define SFP_BASE_BUS        25
#define MGMT_SFP_BASE_BUS   10
#define EEPROM_SYS_FMT      "/sys/bus/i2c/devices/%d-0050/eeprom"
/* SYSFS ATTR */
#define MB_CPLD_SFP_GROUP_PRES_ATTR_FMT     "cpld_sfp_port_%s_pres"
#define MB_CPLD_SFP_GROUP_TXFLT_ATTR_FMT    "cpld_sfp_port_%s_tx_fault"
#define MB_CPLD_SFP_GROUP_TXDIS_ATTR_FMT    "cpld_sfp_port_%s_tx_disable"
#define MB_CPLD_SFP_GROUP_RXLOS_ATTR_FMT    "cpld_sfp_port_%s_rx_los"
#define MB_CPLD_MGMT_SFP_STATUS_ATTR        "cpld_mgmt_sfp_port_status"
#define MB_CPLD_MGMT_SFP_CONIFG_ATTR        "cpld_mgmt_sfp_port_conf"
#define MB_CPLD_QSFP_GROUP_PRES_ATTR_FMT    "cpld_qsfp_port_%s_pres"
#define MB_CPLD_QSFP_GROUP_RESET_ATTR_FMT   "cpld_qsfp_port_%s_rst"
#define MB_CPLD_QSFP_GROUP_LPMODE_ATTR_FMT  "cpld_qsfp_port_%s_lpmode"
#define MB_CPLD_QSFPDD_PRES_ATTR            "cpld_qsfpdd_port_0_5_pres"
#define MB_CPLD_QSFPDD_RESET_ATTR           "cpld_qsfpdd_port_rst"
#define MB_CPLD_QSFPDD_LPMODE_ATTR          "cpld_qsfpdd_port_lpmode"

#define VALIDATE_PORT(p) { if((p < 0) || (p >= PORT_NUM)) return ONLP_STATUS_E_PARAM; }

typedef enum port_type_e {
    TYPE_SFP = 0,
    TYPE_QSFP,
    TYPE_QSFPDD,
    TYPE_MGMT_SFP,
    TYPE_UNNKOW,
    TYPE_MAX,
} port_type_t;

typedef struct port_type_info_s
{
    port_type_t type;
    int port_index;
    int port_group;
    int eeprom_bus_index;
}port_type_info_t;

static int port_eeprom_bus_base[] = {
    SFP_BASE_BUS,
    QSFP_BASE_BUS,
    QSFPDD_BASE_BUS,
    MGMT_SFP_BASE_BUS,
};

const char * sfp_group_str[] = {
    "0_7",
    "8_15",
    "16_23",
    "24_31",
    "32_39",
    "40_47",
    "48_55",
    "56_63",
};

const char * qsfp_group_str[] = {
    "64_71",
    "72_75",
};


port_type_info_t port_num_to_type(int port)
{
    port_type_info_t port_type_info;

    if (port < SFP_NUM) { //SFP+
        port_type_info.type = TYPE_SFP;
        port_type_info.port_index = port % 8;
        port_type_info.port_group = port / 8;
        port_type_info.eeprom_bus_index = port;
    } else if ((port >= SFP_NUM) && (port < (SFP_NUM+QSFP_NUM))) { //QSFP
        port_type_info.type = TYPE_QSFP;
        port_type_info.port_index = (port - SFP_NUM) % 8;
        port_type_info.port_group = (port - SFP_NUM) / 8;
        port_type_info.eeprom_bus_index = port - SFP_NUM;
    } else if ((port >= (SFP_NUM+QSFP_NUM)) &&
                    (port < (SFP_NUM+QSFP_NUM+QSFPDD_NUM))) { //QSFPDD
        port_type_info.type = TYPE_QSFPDD;
        port_type_info.port_index = port - SFP_NUM - QSFP_NUM;
        port_type_info.port_group = 0;
        port_type_info.eeprom_bus_index = port_type_info.port_index;
    } else if ((port >= (SFP_NUM+QSFP_NUM+QSFPDD_NUM)) &&
                    (port < (SFP_NUM+QSFP_NUM+QSFPDD_NUM+MGMT_SFP_NUM))) {
                    //MGMT SFP
        port_type_info.type = TYPE_MGMT_SFP;
        port_type_info.port_index = port - SFP_NUM - QSFP_NUM - QSFPDD_NUM;
        port_type_info.port_group = 0;
        port_type_info.eeprom_bus_index = port_type_info.port_index;
    } else { //unkonwn ports
        AIM_LOG_ERROR("port_num mapping to type fail, port=%d\n", port);
        port_type_info.type = TYPE_UNNKOW;
        port_type_info.port_index = -1;
    }
    return port_type_info;
}

int sfp_present_get(port_type_info_t port_info, int *pres_val)
{
    int group_pres;
    int port_pres;
    char sysfs[128];
    char *cpld_path;

    switch(port_info.port_group) {
        case 0:
        case 1:
        case 4:
        case 5:
            cpld_path = MB_CPLD2_SYSFS_PATH;
            break;
        case 2:
        case 3:
        case 6:
        case 7:
            cpld_path = MB_CPLD3_SYSFS_PATH;
            break;
        default:
            AIM_LOG_ERROR("invalid port group=%d", port_info.port_group);
    }

    snprintf(sysfs, sizeof(sysfs), "%s/"MB_CPLD_SFP_GROUP_PRES_ATTR_FMT,
                cpld_path, sfp_group_str[port_info.port_group]);

    ONLP_TRY(file_read_hex(&group_pres, sysfs));

    //val 0 for presence, pres_val set to 1
    port_pres = READ_BIT(group_pres, port_info.port_index);
    if(port_pres) {
        *pres_val = 0;
    } else {
        *pres_val = 1;
    }

    return ONLP_STATUS_OK;
}

int qsfp_present_get(port_type_info_t port_info, int *pres_val)
{
    int group_pres;
    int port_pres;
    char sysfs[128];

    snprintf(sysfs, sizeof(sysfs), "%s/"MB_CPLD_QSFP_GROUP_PRES_ATTR_FMT,
                MB_CPLD4_SYSFS_PATH, qsfp_group_str[port_info.port_group]);

    ONLP_TRY(file_read_hex(&group_pres, sysfs));

    //val 0 for presence, pres_val set to 1
    port_pres = READ_BIT(group_pres, port_info.port_index);

    if(port_pres) {
        *pres_val = 0;
    } else {
        *pres_val = 1;
    }

    return ONLP_STATUS_OK;
}

int qsfpdd_present_get(port_type_info_t port_info, int *pres_val)
{
    int group_pres;
    int port_pres;
    char sysfs[128];

    snprintf(sysfs, sizeof(sysfs), "%s/%s",
            MB_CPLD4_SYSFS_PATH, MB_CPLD_QSFPDD_PRES_ATTR);

    ONLP_TRY(file_read_hex(&group_pres, sysfs));

    // val 0 for presence, pres_val set to 1
    port_pres = READ_BIT(group_pres, port_info.port_index);

    if (port_pres) {
        *pres_val = 0;
    } else {
        *pres_val = 1;
    }

    return ONLP_STATUS_OK;
}

int mgmt_sfp_present_get(port_type_info_t port_info, int *pres_val)
{
    int group_pres;
    int port_pres;
    int bit_index;
    char sysfs[128];

    bit_index = port_info.port_index*4;

    snprintf(sysfs, sizeof(sysfs), "%s/%s",
            MB_CPLD1_SYSFS_PATH, MB_CPLD_MGMT_SFP_STATUS_ATTR);

    ONLP_TRY(file_read_hex(&group_pres, sysfs));

    //val 0 for presence, pres_val set to 1
    port_pres = READ_BIT(group_pres, bit_index);
    if(port_pres) {
        *pres_val = 0;
    } else {
        *pres_val = 1;
    }

    return ONLP_STATUS_OK;

}

int sfp_control_set(port_type_info_t port_info, onlp_sfp_control_t control, int value)
{
    char sysfs[128];
    char *cpld_sysfs_path;
    int reg_val;
    int rc;

    switch(port_info.port_group) {
        case 0:
        case 1:
        case 4:
        case 5:
            cpld_sysfs_path = MB_CPLD2_SYSFS_PATH;
            break;
        case 2:
        case 3:
        case 6:
        case 7:
            cpld_sysfs_path = MB_CPLD3_SYSFS_PATH;
            break;
        default:
            AIM_LOG_ERROR("unknown ports, port=%d\n", port_info.port_index);
            return ONLP_STATUS_E_UNSUPPORTED;
    }

    //sysfs path
    if(control == ONLP_SFP_CONTROL_TX_DISABLE) {
        snprintf(sysfs, sizeof(sysfs), "%s/"MB_CPLD_SFP_GROUP_TXDIS_ATTR_FMT,
                cpld_sysfs_path, sfp_group_str[port_info.port_group]);
    } else {
        return ONLP_STATUS_E_UNSUPPORTED;
    }


    ONLP_TRY(file_read_hex(&reg_val, sysfs));

    //update reg val
    if(value) {
        SET_BIT(reg_val, port_info.port_index);
    } else {
        CLEAR_BIT(reg_val, port_info.port_index);
    }

    //write reg val back
    if((rc = onlp_file_write_int(reg_val, sysfs))
            != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Unable to write sysfs value, error=%d, \
            sysfs=%s, reg_val=%x", rc, sysfs, reg_val);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int qsfp_control_set(port_type_info_t port_info, onlp_sfp_control_t control, int value)
{
    char sysfs[128];
    char *cpld_sysfs_path;
    int reg_val;
    int rc;

    cpld_sysfs_path = MB_CPLD4_SYSFS_PATH;

    //sysfs path
    switch(control) {
        case ONLP_SFP_CONTROL_RESET:
            snprintf(sysfs, sizeof(sysfs), "%s/"MB_CPLD_QSFP_GROUP_RESET_ATTR_FMT,
                cpld_sysfs_path, qsfp_group_str[port_info.port_group]);
            //0 for reset, 1 for out of reset, reverse the value
            value = (value) ? 0:1;
            break;
        case ONLP_SFP_CONTROL_LP_MODE:
            snprintf(sysfs, sizeof(sysfs), "%s/"MB_CPLD_QSFP_GROUP_LPMODE_ATTR_FMT,
                cpld_sysfs_path, qsfp_group_str[port_info.port_group]);
            break;
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
    }

    ONLP_TRY(file_read_hex(&reg_val, sysfs));

    //update reg val
    if(value) {
        SET_BIT(reg_val, port_info.port_index);
    } else {
        CLEAR_BIT(reg_val, port_info.port_index);
    }

    //write reg val back
    if((rc = onlp_file_write_int(reg_val, sysfs))
            != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Unable to write sysfs value, error=%d, \
            sysfs=%s, reg_val=%x", rc, sysfs, reg_val);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int qsfpdd_control_set(port_type_info_t port_info, onlp_sfp_control_t control, int value)
{
    char sysfs[128];
    char *cpld_sysfs_path;
    int reg_val;
    int rc;

    cpld_sysfs_path = MB_CPLD4_SYSFS_PATH;

    //sysfs path
    switch(control) {
        case ONLP_SFP_CONTROL_RESET:
            snprintf(sysfs, sizeof(sysfs), "%s/%s", cpld_sysfs_path,
                MB_CPLD_QSFPDD_RESET_ATTR);
            //0 for reset, 1 for out of reset, reverse the value
            value = (value) ? 0:1;
            break;
        case ONLP_SFP_CONTROL_LP_MODE:
            snprintf(sysfs, sizeof(sysfs), "%s/%s", cpld_sysfs_path,
                MB_CPLD_QSFPDD_LPMODE_ATTR);
            break;
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
    }

    ONLP_TRY(file_read_hex(&reg_val, sysfs));

    //update reg val
    if(value) {
        SET_BIT(reg_val, port_info.port_index);
    } else {
        CLEAR_BIT(reg_val, port_info.port_index);
    }

    //write reg val back
    if((rc = onlp_file_write_int(reg_val, sysfs))
            != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Unable to write sysfs value, error=%d, \
            sysfs=%s, reg_val=%x", rc, sysfs, reg_val);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int mgmt_sfp_control_set(port_type_info_t port_info, onlp_sfp_control_t control, int value)
{
    char sysfs[128];
    int bit_index;
    int reg_val = 0;
    int rc;

    //sysfs path
    if(control == ONLP_SFP_CONTROL_TX_DISABLE) {
        snprintf(sysfs, sizeof(sysfs), "%s/%s", MB_CPLD1_SYSFS_PATH,
                    MB_CPLD_MGMT_SFP_CONIFG_ATTR);
        bit_index = (port_info.port_index + 0)*4;
    } else {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    ONLP_TRY(file_read_hex(&reg_val, sysfs));

    //update reg val
    if(value) {
        SET_BIT(reg_val, bit_index);
    } else {
        CLEAR_BIT(reg_val, bit_index);
    }

    //write reg val back
    if((rc = onlp_file_write_int(reg_val, sysfs))
        != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Unable to write sysfs value, error=%d, \
            sysfs=%s, reg_val=%x", rc, sysfs, reg_val);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int sfp_control_get(port_type_info_t port_info, onlp_sfp_control_t control, int* value)
{
    int reg_val = 0;
    char sysfs[128];
    char *cpld_sysfs_path;

    switch(port_info.port_group) {
        case 0:
        case 1:
        case 4:
        case 5:
            cpld_sysfs_path = MB_CPLD2_SYSFS_PATH;
            break;
        case 2:
        case 3:
        case 6:
        case 7:
            cpld_sysfs_path = MB_CPLD3_SYSFS_PATH;
            break;
        default:
            AIM_LOG_ERROR("unknown port group=%d\n", port_info.port_group);
            return ONLP_STATUS_E_UNSUPPORTED;
    }

    //sysfs path
    if (control == ONLP_SFP_CONTROL_RX_LOS) {
        snprintf(sysfs, sizeof(sysfs), "%s/"MB_CPLD_SFP_GROUP_RXLOS_ATTR_FMT,
                cpld_sysfs_path, sfp_group_str[port_info.port_group]);
    } else if (control == ONLP_SFP_CONTROL_TX_FAULT) {
        snprintf(sysfs, sizeof(sysfs), "%s/"MB_CPLD_SFP_GROUP_TXFLT_ATTR_FMT,
                cpld_sysfs_path, sfp_group_str[port_info.port_group]);
    } else if (control == ONLP_SFP_CONTROL_TX_DISABLE) {
        snprintf(sysfs, sizeof(sysfs), "%s/"MB_CPLD_SFP_GROUP_TXDIS_ATTR_FMT,
                cpld_sysfs_path, sfp_group_str[port_info.port_group]);
    } else {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    ONLP_TRY(file_read_hex(&reg_val, sysfs));

    *value = READ_BIT(reg_val, port_info.port_index);
    return ONLP_STATUS_OK;
}

int qsfp_control_get(port_type_info_t port_info, onlp_sfp_control_t control, int* value)
{
    int reg_val = 0;
    char sysfs[128];
    char *cpld_sysfs_path;

    cpld_sysfs_path = MB_CPLD4_SYSFS_PATH;

    //sysfs path
    switch(control) {
        case ONLP_SFP_CONTROL_RESET:
        case ONLP_SFP_CONTROL_RESET_STATE:
            snprintf(sysfs, sizeof(sysfs), "%s/"MB_CPLD_QSFP_GROUP_RESET_ATTR_FMT,
                    cpld_sysfs_path, qsfp_group_str[port_info.port_group]);
            ONLP_TRY(file_read_hex(&reg_val, sysfs));
            //0 for reset, 1 for out of reset
            *value = (READ_BIT(reg_val, port_info.port_index)) ? 0:1;
            break;
        case ONLP_SFP_CONTROL_LP_MODE:
            snprintf(sysfs, sizeof(sysfs), "%s/"MB_CPLD_QSFP_GROUP_LPMODE_ATTR_FMT,
                    cpld_sysfs_path, qsfp_group_str[port_info.port_group]);
            ONLP_TRY(file_read_hex(&reg_val, sysfs));
            //1 for lp mode enable, 0 for lp mode disable
            *value = READ_BIT(reg_val, port_info.port_index);
            break;
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
    }

    return ONLP_STATUS_OK;
}

int qsfpdd_control_get(port_type_info_t port_info, onlp_sfp_control_t control, int* value)
{
    int reg_val = 0;
    char sysfs[128];
    char *cpld_sysfs_path;

    cpld_sysfs_path = MB_CPLD4_SYSFS_PATH;

    //sysfs path
    switch(control) {
        case ONLP_SFP_CONTROL_RESET:
        case ONLP_SFP_CONTROL_RESET_STATE:
            snprintf(sysfs, sizeof(sysfs), "%s/%s", cpld_sysfs_path,
                MB_CPLD_QSFPDD_RESET_ATTR);
            ONLP_TRY(file_read_hex(&reg_val, sysfs));
            //0 for reset, 1 for out of reset
            *value = (READ_BIT(reg_val, port_info.port_index)) ? 0:1;
            break;
        case ONLP_SFP_CONTROL_LP_MODE:
            snprintf(sysfs, sizeof(sysfs), "%s/%s", cpld_sysfs_path,
                MB_CPLD_QSFPDD_LPMODE_ATTR);
            ONLP_TRY(file_read_hex(&reg_val, sysfs));
            //1 for lp mode enable, 0 for lp mode disable
            *value = READ_BIT(reg_val, port_info.port_index);
            break;
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
    }

    return ONLP_STATUS_OK;
}

int mgmt_sfp_control_get(port_type_info_t port_info, onlp_sfp_control_t control, int* value)
{
    int reg_val = 0;
    char sysfs[128];
    int bit_index, status_index;

    //sysfs path
    if (control == ONLP_SFP_CONTROL_RX_LOS) {
        snprintf(sysfs, sizeof(sysfs), "%s/%s", MB_CPLD1_SYSFS_PATH,
                    MB_CPLD_MGMT_SFP_STATUS_ATTR);
        status_index = 2;
    } else if (control == ONLP_SFP_CONTROL_TX_FAULT) {
        snprintf(sysfs, sizeof(sysfs), "%s/%s", MB_CPLD1_SYSFS_PATH,
                    MB_CPLD_MGMT_SFP_STATUS_ATTR);
        status_index = 1;
    } else if (control == ONLP_SFP_CONTROL_TX_DISABLE) {
        snprintf(sysfs, sizeof(sysfs), "%s/%s", MB_CPLD1_SYSFS_PATH,
                    MB_CPLD_MGMT_SFP_CONIFG_ATTR);
        status_index = 0;
    } else {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    ONLP_TRY(file_read_hex(&reg_val, sysfs));

    bit_index = port_info.port_index * 4 + status_index;

   *value = READ_BIT(reg_val, bit_index);
    return ONLP_STATUS_OK;
}

/**
 * @brief Software initialization of the SFP module.
 */
int onlp_sfpi_sw_init(void)
{
    lock_init();
    return ONLP_STATUS_OK;
}

/**
 * @brief Hardware initialization of the SFP module.
 * @param flags The hardware initialization flags.
 */
int onlp_sfpi_hw_init(uint32_t flags)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Deinitialize the chassis software module.
 * @note The primary purpose of this API is to properly
 * deallocate any resources used by the module in order
 * faciliate detection of real resouce leaks.
 */
int onlp_sfpi_sw_denit(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the bitmap of SFP-capable port numbers.
 * @param[out] bmap Receives the bitmap.
 */
int onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{
    int p = 0;

    AIM_BITMAP_CLR_ALL(bmap);
    for(p = 0; p < PORT_NUM; p++) {
        AIM_BITMAP_SET(bmap, p);
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Determine the SFP connector type.
 * @param id The SFP Port ID.
 * @param[out] rtype Receives the connector type.
 */
int onlp_sfpi_type_get(onlp_oid_id_t id, onlp_sfp_type_t* rtype)
{
    int port = ONLP_OID_ID_GET(id);
    VALIDATE_PORT(port);
    port_type_info_t port_type_info = port_num_to_type(port);

    switch(port_type_info.type) {
        case TYPE_QSFPDD:
            /* Shoud fix after QSFPDD defined */
        case TYPE_QSFP:
            *rtype = ONLP_SFP_TYPE_QSFP28;
            break;
        case TYPE_SFP:
        case TYPE_MGMT_SFP:
            *rtype = ONLP_SFP_TYPE_SFP;
            break;
        default:
            return ONLP_STATUS_E_PARAM;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Determine if an SFP is present.
 * @param id The SFP Port ID.
 * @returns 1 if present
 * @returns 0 if absent
 * @returns An error condition.
 */
int onlp_sfpi_is_present(onlp_oid_id_t id)
{
    int port = ONLP_OID_ID_GET(id);
    VALIDATE_PORT(port);
    port_type_info_t port_type_info = port_num_to_type(port);
    int status = ONLP_STATUS_OK;

    switch(port_type_info.type) {
        case TYPE_SFP:
            ONLP_TRY(sfp_present_get(port_type_info, &status));
            break;
        case TYPE_QSFP:
            ONLP_TRY(qsfp_present_get(port_type_info, &status));
            break;
        case TYPE_QSFPDD:
            ONLP_TRY(qsfpdd_present_get(port_type_info, &status));
            break;
        case TYPE_MGMT_SFP:
            ONLP_TRY(mgmt_sfp_present_get(port_type_info, &status));
            break;
        default:
            AIM_LOG_ERROR("unknown port, port=%d\n", port);
            return ONLP_STATUS_E_UNSUPPORTED;
    }
    return status;
}

/**
 * @brief Return the presence bitmap for all SFP ports.
 * @param[out] dst Receives the presence bitmap.
 */
int onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int p = 0;
    int ret = 0;

    AIM_BITMAP_CLR_ALL(dst);
    for (p = 0; p < PORT_NUM; p++) {
        ret = onlp_sfpi_is_present(p);
        AIM_BITMAP_MOD(dst, p, ret);
    }

    return ONLP_STATUS_OK;


}

/**
 * @brief Return the RX_LOS bitmap for all SFP ports.
 * @param[out] dst Receives the RX_LOS bitmap.
 */
int onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int i = 0, value = 0;

    for(i = 0; i < PORT_NUM; i++) {
        if(onlp_sfpi_control_get(i, ONLP_SFP_CONTROL_RX_LOS, &value) != ONLP_STATUS_OK) {
            AIM_BITMAP_MOD(dst, i, 0);  //set default value for port which has no cap
        } else {
            AIM_BITMAP_MOD(dst, i, value);
        }
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Read bytes from the target device on the given SFP port.
 * @param id The SFP Port ID.
 * @param devaddr The device address.
 * @param addr Read offset.
 * @param[out] dst Receives the data.
 * @param len Read length.
 * @returns The number of bytes read or ONLP_STATUS_E_* no error.
 */
int onlp_sfpi_dev_read(onlp_oid_id_t id, int devaddr, int addr,
                       uint8_t* dst, int len)
{
    int rc = 0;
    int port = ONLP_OID_ID_GET(id);
    VALIDATE_PORT(port);
    port_type_info_t port_type_info = port_num_to_type(port);
    int bus;

    if( port_type_info.type == TYPE_UNNKOW) {
        return ONLP_STATUS_E_PARAM;
    }

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    bus = port_eeprom_bus_base[port_type_info.type] + port_type_info.eeprom_bus_index;

    if ((rc=onlp_i2c_block_read(bus, devaddr, addr, len, dst, ONLP_I2C_F_FORCE)) < 0) {
        check_and_do_i2c_mux_reset(port);
    }

    return rc;
}


/**
 * @brief Write bytes to the target device on the given SFP port.
 * @param id The SFP Port ID.
 * @param devaddr The device address.
 * @param addr Write offset.
 * @param src The bytes to write.
 * @param len Write length.
 */
int onlp_sfpi_dev_write(onlp_oid_id_t id, int devaddr, int addr,
                        uint8_t* src, int len)
{
    int rc = 0;
    int port = ONLP_OID_ID_GET(id);
    VALIDATE_PORT(port);
    port_type_info_t port_type_info = port_num_to_type(port);
    int bus;

    if( port_type_info.type == TYPE_UNNKOW) {
        return ONLP_STATUS_E_PARAM;
    }

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    bus = port_eeprom_bus_base[port_type_info.type] + port_type_info.eeprom_bus_index;

    if ((rc=onlp_i2c_write(bus, devaddr, addr, len, src, ONLP_I2C_F_FORCE))<0) {
        check_and_do_i2c_mux_reset(port);
    }

    return rc;
}

/**
 * @brief Read a byte from the target device on the given SFP port.
 * @param id The SFP Port ID.
 * @param devaddr The device address.
 * @param addr The read address.
 * @returns The byte on success or ONLP_STATUS_E* on error.
 */
int onlp_sfpi_dev_readb(onlp_oid_id_t id, int devaddr, int addr)
{
    int rc = 0;
    int port = ONLP_OID_ID_GET(id);
    VALIDATE_PORT(port);
    port_type_info_t port_type_info = port_num_to_type(port);
    int bus;

    if( port_type_info.type == TYPE_UNNKOW) {
        return ONLP_STATUS_E_PARAM;
    }

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    bus = port_eeprom_bus_base[port_type_info.type] + port_type_info.eeprom_bus_index;

    if ((rc=onlp_i2c_readb(bus, devaddr, addr, ONLP_I2C_F_FORCE))<0) {
        check_and_do_i2c_mux_reset(port);
    }

    return rc;
}

/**
 * @brief Write a byte to the target device on the given SFP port.
 * @param id The SFP Port ID.
 * @param devaddr The device address.
 * @param addr The write address.
 * @param value The write value.
 */
int onlp_sfpi_dev_writeb(onlp_oid_id_t id, int devaddr, int addr,
                         uint8_t value)
{
    int rc = 0;
    int port = ONLP_OID_ID_GET(id);
    VALIDATE_PORT(port);
    port_type_info_t port_type_info = port_num_to_type(port);
    int bus;

    if( port_type_info.type == TYPE_UNNKOW) {
        return ONLP_STATUS_E_PARAM;
    }

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    bus = port_eeprom_bus_base[port_type_info.type] + port_type_info.eeprom_bus_index;

    if ((rc=onlp_i2c_writeb(bus, devaddr, addr, value, ONLP_I2C_F_FORCE))<0) {
        check_and_do_i2c_mux_reset(port);
    }

    return rc;
}

/**
 * @brief Read a word from the target device on the given SFP port.
 * @param id The SFP Port ID.
 * @param devaddr The device address.
 * @param addr The read address.
 * @returns The word if successful, ONLP_STATUS_E* on error.
 */
int onlp_sfpi_dev_readw(onlp_oid_id_t id, int devaddr, int addr)
{
    int rc = 0;
    int port = ONLP_OID_ID_GET(id);
    VALIDATE_PORT(port);
    port_type_info_t port_type_info = port_num_to_type(port);
    int bus;

    if( port_type_info.type == TYPE_UNNKOW) {
        return ONLP_STATUS_E_PARAM;
    }

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    bus = port_eeprom_bus_base[port_type_info.type] + port_type_info.eeprom_bus_index;

    if ((rc=onlp_i2c_readw(bus, devaddr, addr, ONLP_I2C_F_FORCE))<0) {
        check_and_do_i2c_mux_reset(port);
    }

    return rc;
}

/**
 * @brief Write a word to the target device on the given SFP port.
 * @param id The SFP Port ID.
 * @param devaddr The device address.
 * @param addr The write address.
 * @param value The write value.
 */
int onlp_sfpi_dev_writew(onlp_oid_id_t id, int devaddr, int addr,
                         uint16_t value)
{
    int rc = 0;
    int port = ONLP_OID_ID_GET(id);
    VALIDATE_PORT(port);
    port_type_info_t port_type_info = port_num_to_type(port);
    int bus;

    if( port_type_info.type == TYPE_UNNKOW) {
        return ONLP_STATUS_E_PARAM;
    }

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    bus = port_eeprom_bus_base[port_type_info.type] + port_type_info.eeprom_bus_index;

    if ((rc=onlp_i2c_writew(bus, devaddr, addr, value, ONLP_I2C_F_FORCE))<0) {
        check_and_do_i2c_mux_reset(port);
    }

    return rc;
}

/**
 * @brief Perform any actions required after an SFP is inserted.
 * @param id The SFP Port ID.
 * @param info The SFF Module information structure.
 * @note This function is optional. If your platform must
 * adjust equalizer or preemphasis settings internally then
 * this function should be implemented as the trigger.
 */
int onlp_sfpi_post_insert(onlp_oid_id_t id, sff_info_t* info)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Returns whether or not the given control is supported on the given port.
 * @param id The SFP Port ID.
 * @param control The control.
 * @param[out] rv Receives 1 if supported, 0 if not supported.
 * @note This provided for convenience and is optional.
 * If you implement this function your control_set and control_get APIs
 * will not be called on unsupported ports.
 */
int onlp_sfpi_control_supported(onlp_oid_id_t id,
                                onlp_sfp_control_t control, int* rv)
{
    int port = ONLP_OID_ID_GET(id);
    VALIDATE_PORT(port);
    port_type_info_t port_type_info = port_num_to_type(port);
    //set unsupported as default value
    *rv=0;

    switch(control) {
        case ONLP_SFP_CONTROL_RESET:
        case ONLP_SFP_CONTROL_RESET_STATE:
        case ONLP_SFP_CONTROL_LP_MODE:
            if(port_type_info.type == TYPE_QSFP ||
                port_type_info.type == TYPE_QSFPDD) {
                *rv = 1;
            }
            break;
        case ONLP_SFP_CONTROL_RX_LOS:
        case ONLP_SFP_CONTROL_TX_FAULT:
        case ONLP_SFP_CONTROL_TX_DISABLE:
            if(port_type_info.type == TYPE_SFP ||
                port_type_info.type == TYPE_MGMT_SFP) {
                *rv = 1;
            }
            break;
        default:
            *rv = 0;
            break;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Set an SFP control.
 * @param id The SFP Port ID.
 * @param control The control.
 * @param value The value.
 */
int onlp_sfpi_control_set(onlp_oid_id_t id, onlp_sfp_control_t control,
                          int value)
{
    int port = ONLP_OID_ID_GET(id);
    VALIDATE_PORT(port);
    port_type_info_t port_type_info = port_num_to_type(port);
    int rc;

    switch(port_type_info.type) {
        case TYPE_QSFPDD:
            rc = qsfpdd_control_set(port_type_info, control, value);
            break;
        case TYPE_QSFP:
            rc = qsfp_control_set(port_type_info, control, value);
            break;
        case TYPE_SFP:
            rc = sfp_control_set(port_type_info, control, value);
            break;
        case TYPE_MGMT_SFP:
            rc = mgmt_sfp_control_set(port_type_info, control, value);
            break;
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
    }

    return rc;
}

/**
 * @brief Get an SFP control.
 * @param id The SFP Port ID.
 * @param control The control
 * @param[out] value Receives the current value.
 */
int onlp_sfpi_control_get(onlp_oid_id_t id, onlp_sfp_control_t control,
                          int* value)
{
    int port = ONLP_OID_ID_GET(id);
    VALIDATE_PORT(port);
    port_type_info_t port_type_info = port_num_to_type(port);
    int rc = ONLP_STATUS_OK;

    switch(port_type_info.type) {
        case TYPE_QSFPDD:
            rc = qsfpdd_control_get(port_type_info, control, value);
            break;
        case TYPE_QSFP:
            rc = qsfp_control_get(port_type_info, control, value);
            break;
        case TYPE_SFP:
            rc = sfp_control_get(port_type_info, control, value);
            break;
        case TYPE_MGMT_SFP:
            rc = mgmt_sfp_control_get(port_type_info, control, value);
            break;
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
    }

    return rc;
}

/**
 * @brief Remap SFP user SFP port numbers before calling the SFPI interface.
 * @param id The SFP Port ID.
 * @param[out] rport Receives the new port.
 * @note This function will be called to remap the user SFP port number
 * to the number returned in rport before the SFPI functions are called.
 * This is an optional convenience for platforms with dynamic or
 * variant physical SFP numbering.
 */
int onlp_sfpi_port_map(onlp_oid_id_t id, int* rport)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Get the SFP's OID header.
 * @param id The SFP oid.
 * @param [out] hdr Receives the header.
 */
int onlp_sfpi_hdr_get(onlp_oid_id_t id, onlp_oid_hdr_t* hdr)
{
    int local_id = ONLP_OID_ID_GET(id);
    int is_present = onlp_sfpi_is_present(id);

    hdr->id = ONLP_SFP_ID_CREATE(local_id);
    hdr->poid = ONLP_OID_CHASSIS;
    hdr->status = is_present ? ONLP_OID_STATUS_FLAG_PRESENT : ONLP_OID_STATUS_FLAG_UNPLUGGED;

    return ONLP_STATUS_OK;
}

