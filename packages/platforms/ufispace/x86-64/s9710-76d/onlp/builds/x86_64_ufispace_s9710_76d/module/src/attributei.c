/************************************************************
 * <bsn.cl fy=2017 v=onl>
 *
 *        Copyright 2017 Big Switch Networks, Inc.
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
 * Attribute Implementation.
 *
 ***********************************************************/
#include <unistd.h>
#include <linux/i2c-dev.h>

#include <onlp/platformi/attributei.h>
#include <onlp/stdattrs.h>
#include <onlplib/file.h>

#include <onlplib/file.h>
#include <onlplib/i2c.h>
#include <onlplib/onie.h>

#include "x86_64_ufispace_s9710_76d_int.h"
#include "x86_64_ufispace_s9710_76d_log.h"

#include "platform_lib.h"


#define ONIE_FIELD_CPY(dst, src, field) \
  if (src.field != NULL) { \
    dst->field = malloc(strlen(src.field) + 1); \
    strcpy(dst->field, src.field); \
  } \


#define SYSI_ONIE_TYPE_SUPPORT_NUM        17

/**
 *  The TLV Types.
 */
#define TLV_CODE_PRODUCT_NAME   0x21
#define TLV_CODE_PART_NUMBER    0x22
#define TLV_CODE_SERIAL_NUMBER  0x23
#define TLV_CODE_MAC_BASE       0x24
#define TLV_CODE_MANUF_DATE     0x25
#define TLV_CODE_DEVICE_VERSION 0x26
#define TLV_CODE_LABEL_REVISION 0x27
#define TLV_CODE_PLATFORM_NAME  0x28
#define TLV_CODE_ONIE_VERSION   0x29
#define TLV_CODE_MAC_SIZE       0x2A
#define TLV_CODE_MANUF_NAME     0x2B
#define TLV_CODE_MANUF_COUNTRY  0x2C
#define TLV_CODE_VENDOR_NAME    0x2D
#define TLV_CODE_DIAG_VERSION   0x2E
#define TLV_CODE_SERVICE_TAG    0x2F
#define TLV_CODE_VENDOR_EXT     0xFD
#define TLV_CODE_CRC_32         0xFE


typedef struct sysi_onie_vpd_s {
    uint8_t type;
    char file[ONLP_CONFIG_INFO_STR_MAX*2];
} sysi_onie_vpd_t;


#define CMD_BIOS_VER  "dmidecode -s bios-version | tail -1 | tr -d '\r\n'"
#define CMD_BMC_VER_1 "expr `ipmitool mc info | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f1` + 0"
#define CMD_BMC_VER_2 "expr `ipmitool mc info | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f2` + 0"
#define CMD_BMC_VER_3 "echo $((`ipmitool mc info | grep 'Aux Firmware Rev Info' -A 2 | sed -n '2p'`))"

static int _sysi_onie_product_name_get(char** product_name);
static int _sysi_onie_part_number_get(char** part_number);
static int _sysi_onie_serial_number_get(char** serial_number);
static int _sysi_onie_mac_base_get(uint8_t mac_base[6]);
static int _sysi_onie_manuf_date_get(char** manuf_date);
static int _sysi_onie_device_version_get(uint8_t* device_version);
static int _sysi_onie_label_revision_get(char** label_revision);
static int _sysi_onie_platform_name_get(char** platform_name);
static int _sysi_onie_onie_version_get(char** onie_version);
static int _sysi_onie_mac_size_get(uint16_t* mac_size);
static int _sysi_onie_manuf_name_get(char** manuf_name);
static int _sysi_onie_manuf_country_get(char** manuf_country);
static int _sysi_onie_vendor_name_get(char** vendor_name);
static int _sysi_onie_diag_version_get(char** diag_version);
static int _sysi_onie_service_tag_get(char** service_tag);
static int _sysi_onie_vendor_ext_get(list_head_t *vendor_ext);
static int _sysi_onie_crc_32_get(uint32_t* crc_32);

static int _sysi_onie_info_total_len_get(onlp_onie_info_t *onie, uint16_t *total_len);

static sysi_onie_vpd_t __tlv_vpd_info[SYSI_ONIE_TYPE_SUPPORT_NUM] = {
    {
        TLV_CODE_PRODUCT_NAME,
        "/sys/class/eeprom/x86_64_ufispace_s9710_76d_onie_syseeprom/product_name"
    },
    {
        TLV_CODE_PART_NUMBER,
        "/sys/class/eeprom/x86_64_ufispace_s9710_76d_onie_syseeprom/part_number"
    },
    {
        TLV_CODE_SERIAL_NUMBER,
        "/sys/class/eeprom/x86_64_ufispace_s9710_76d_onie_syseeprom/serial_number"
    },
    {
        TLV_CODE_MAC_BASE,
        "/sys/class/eeprom/x86_64_ufispace_s9710_76d_onie_syseeprom/base_mac_address"
    },
    {
        TLV_CODE_MANUF_DATE,
        "/sys/class/eeprom/x86_64_ufispace_s9710_76d_onie_syseeprom/manufacture_date"
    },
    {
        TLV_CODE_DEVICE_VERSION,
        "/sys/class/eeprom/x86_64_ufispace_s9710_76d_onie_syseeprom/device_version"
    },
    {
        TLV_CODE_LABEL_REVISION,
        "/sys/class/eeprom/x86_64_ufispace_s9710_76d_onie_syseeprom/label_revision"
    },
    {
        TLV_CODE_PLATFORM_NAME,
        "/sys/class/eeprom/x86_64_ufispace_s9710_76d_onie_syseeprom/platform_name"
    },
    {
        TLV_CODE_ONIE_VERSION,
        "/sys/class/eeprom/x86_64_ufispace_s9710_76d_onie_syseeprom/onie_version"
    },
    {
        TLV_CODE_MAC_SIZE,
        "/sys/class/eeprom/x86_64_ufispace_s9710_76d_onie_syseeprom/mac_addresses"
    },
    {
        TLV_CODE_MANUF_NAME,
        "/sys/class/eeprom/x86_64_ufispace_s9710_76d_onie_syseeprom/manufacturer"
    },
    {
        TLV_CODE_MANUF_COUNTRY,
        "/sys/class/eeprom/x86_64_ufispace_s9710_76d_onie_syseeprom/country_code"
    },
    {
        TLV_CODE_VENDOR_NAME,
        "/sys/class/eeprom/x86_64_ufispace_s9710_76d_onie_syseeprom/vendor_name"
    },
    {
        TLV_CODE_DIAG_VERSION,
        "/sys/class/eeprom/x86_64_ufispace_s9710_76d_onie_syseeprom/diag_version"
    },
    {
        TLV_CODE_SERVICE_TAG,
        "/sys/class/eeprom/x86_64_ufispace_s9710_76d_onie_syseeprom/service_tag"
    },
    {
        TLV_CODE_VENDOR_EXT,
        "/sys/class/eeprom/x86_64_ufispace_s9710_76d_onie_syseeprom/vendor_ext"
    },
    {
        TLV_CODE_CRC_32,
        "/sys/class/eeprom/x86_64_ufispace_s9710_76d_onie_syseeprom/crc32"
    }
};

static int _sysi_onie_product_name_get(char** product_name)
{
    int ret = ONLP_STATUS_OK;
    int i;
    int len;
    char* path;
    char buf[ONLP_CONFIG_INFO_STR_MAX];

    for(i=0; i<SYSI_ONIE_TYPE_SUPPORT_NUM; i++ ) {
        if( TLV_CODE_PRODUCT_NAME == __tlv_vpd_info[i].type) {
            path = __tlv_vpd_info[i].file;
            break;
        }
    }
    if( SYSI_ONIE_TYPE_SUPPORT_NUM == i) {
        /* Cannot find support type */
        ret = ONLP_STATUS_E_UNSUPPORTED;
    }
    if( ONLP_STATUS_OK == ret) {
        /* get info */
        ret = onlp_file_read((uint8_t*)buf,ONLP_CONFIG_INFO_STR_MAX, &len, path);
    }
    if( ONLP_STATUS_OK == ret) {
        /* remove \n in file output */
        buf[strlen(buf)-1] = 0;
        *product_name = aim_fstrdup("%s",buf);
    } else {
        /* alloc a empty array for it */
        *product_name = aim_zmalloc(1);
        ret = ONLP_STATUS_OK;
    }

    return ret;
}


static int _sysi_onie_part_number_get(char** part_number)
{
    int ret = ONLP_STATUS_OK;
    int i;
    int len;
    char* path;
    char buf[ONLP_CONFIG_INFO_STR_MAX];

    for(i=0; i<SYSI_ONIE_TYPE_SUPPORT_NUM; i++ ) {
        if( TLV_CODE_PART_NUMBER == __tlv_vpd_info[i].type) {
            path = __tlv_vpd_info[i].file;
            break;
        }
    }
    if( SYSI_ONIE_TYPE_SUPPORT_NUM == i) {
        /* Cannot find support type */
        ret = ONLP_STATUS_E_UNSUPPORTED;
    }
    if( ONLP_STATUS_OK == ret) {
        /* get info */
        ret = onlp_file_read((uint8_t*)buf,ONLP_CONFIG_INFO_STR_MAX, &len, path);
    }
    if( ONLP_STATUS_OK == ret) {
        /* remove \n in file output */
        buf[strlen(buf)-1] = 0;
        *part_number = aim_fstrdup("%s",buf);
    } else {
        /* alloc a empty array for it */
        *part_number = aim_zmalloc(1);
        ret = ONLP_STATUS_OK;
    }

    return ret;
}


static int _sysi_onie_serial_number_get(char** serial_number)
{
    int ret = ONLP_STATUS_OK;
    int i;
    int len;
    char* path;
    char buf[ONLP_CONFIG_INFO_STR_MAX];

    for(i=0; i<SYSI_ONIE_TYPE_SUPPORT_NUM; i++ ) {
        if( TLV_CODE_SERIAL_NUMBER == __tlv_vpd_info[i].type) {
            path = __tlv_vpd_info[i].file;
            break;
        }
    }
    if( SYSI_ONIE_TYPE_SUPPORT_NUM == i) {
        /* Cannot find support type */
        ret = ONLP_STATUS_E_UNSUPPORTED;
    }
    if( ONLP_STATUS_OK == ret) {
        /* get info */
        ret = onlp_file_read((uint8_t*)buf,ONLP_CONFIG_INFO_STR_MAX, &len, path);
    }
    if( ONLP_STATUS_OK == ret) {
        /* remove \n in file output */
        buf[strlen(buf)-1] = 0;
        *serial_number = aim_fstrdup("%s",buf);
    } else {
        /* alloc a empty array for it */
        *serial_number = aim_zmalloc(1);
        ret = ONLP_STATUS_OK;
    }

    return ret;
}


static int _sysi_onie_mac_base_get(uint8_t mac_base[6])
{
    int ret = ONLP_STATUS_OK;
    int i;
    int len;
    char* path;
    char buf[ONLP_CONFIG_INFO_STR_MAX];

    for(i=0; i<SYSI_ONIE_TYPE_SUPPORT_NUM; i++ ) {
        if( TLV_CODE_MAC_BASE == __tlv_vpd_info[i].type) {
            path = __tlv_vpd_info[i].file;
            break;
        }
    }
    if( SYSI_ONIE_TYPE_SUPPORT_NUM == i) {
        /* Cannot find support type */
        ret = ONLP_STATUS_E_UNSUPPORTED;
    }
    if( ONLP_STATUS_OK == ret) {
        /* get info */
        ret = onlp_file_read((uint8_t*)buf,ONLP_CONFIG_INFO_STR_MAX, &len, path);
    }
    if( ONLP_STATUS_OK == ret) {
        if(6 != sscanf( buf, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx\n",
                        &mac_base[0], &mac_base[1], &mac_base[2],
                        &mac_base[3], &mac_base[4], &mac_base[5])) {
            /* parsing fail */
            memset(mac_base, 0, 6);
        }
    } else {
        memset(mac_base, 0, 6);
        ret = ONLP_STATUS_OK;
    }

    return ret;
}


static int _sysi_onie_manuf_date_get(char** manuf_date)
{
    int ret = ONLP_STATUS_OK;
    int i;
    int len;
    char* path;
    char buf[ONLP_CONFIG_INFO_STR_MAX];

    for(i=0; i<SYSI_ONIE_TYPE_SUPPORT_NUM; i++ ) {
        if( TLV_CODE_MANUF_DATE == __tlv_vpd_info[i].type) {
            path = __tlv_vpd_info[i].file;
            break;
        }
    }
    if( SYSI_ONIE_TYPE_SUPPORT_NUM == i) {
        /* Cannot find support type */
        ret = ONLP_STATUS_E_UNSUPPORTED;
    }
    if( ONLP_STATUS_OK == ret) {
        /*get info*/
        ret = onlp_file_read((uint8_t*)buf,ONLP_CONFIG_INFO_STR_MAX, &len, path);
    }
    if( ONLP_STATUS_OK == ret) {
        /*remove \n in file output*/
        buf[strlen(buf)-1] = 0;
        *manuf_date = aim_fstrdup("%s",buf);
    } else {
        /*alloc a empty array for it*/
        *manuf_date = aim_zmalloc(1);
        ret = ONLP_STATUS_OK;
    }

    return ret;
}


static int _sysi_onie_device_version_get(uint8_t* device_version)
{
    int ret = ONLP_STATUS_OK;
    int i;
    int len;
    char* path;
    char buf[ONLP_CONFIG_INFO_STR_MAX];

    for(i=0; i<SYSI_ONIE_TYPE_SUPPORT_NUM; i++ ) {
        if( TLV_CODE_DEVICE_VERSION == __tlv_vpd_info[i].type) {
            path = __tlv_vpd_info[i].file;
            break;
        }
    }
    if( SYSI_ONIE_TYPE_SUPPORT_NUM == i) {
        /* Cannot find support type */
        ret = ONLP_STATUS_E_UNSUPPORTED;
    }
    if( ONLP_STATUS_OK == ret) {
        /*get info*/
        ret = onlp_file_read((uint8_t*)buf,ONLP_CONFIG_INFO_STR_MAX, &len, path);
    }
    if( ONLP_STATUS_OK == ret) {
        *device_version = (uint8_t)strtoul(buf, NULL, 0);
    } else {
        ret = ONLP_STATUS_OK;
    }

    return ret;
}


static int _sysi_onie_label_revision_get(char** label_revision)
{
    int ret = ONLP_STATUS_OK;
    int i;
    int len;
    char* path;
    char buf[ONLP_CONFIG_INFO_STR_MAX];

    for(i=0; i<SYSI_ONIE_TYPE_SUPPORT_NUM; i++ ) {
        if( TLV_CODE_LABEL_REVISION == __tlv_vpd_info[i].type) {
            path = __tlv_vpd_info[i].file;
            break;
        }
    }
    if( SYSI_ONIE_TYPE_SUPPORT_NUM == i) {
        /* Cannot find support type */
        ret = ONLP_STATUS_E_UNSUPPORTED;
    }
    if( ONLP_STATUS_OK == ret) {
        /*get info*/
        ret = onlp_file_read((uint8_t*)buf,ONLP_CONFIG_INFO_STR_MAX, &len, path);
    }
    if( ONLP_STATUS_OK == ret) {
        /*remove \n in file output*/
        buf[strlen(buf)-1] = 0;
        *label_revision = aim_fstrdup("%s",buf);
    } else {
        /*alloc a empty array for it*/
        *label_revision = aim_zmalloc(1);
        ret = ONLP_STATUS_OK;
    }

    return ret;
}


static int _sysi_onie_platform_name_get(char** platform_name)
{
    int ret = ONLP_STATUS_OK;
    int i;
    int len;
    char* path;
    char buf[ONLP_CONFIG_INFO_STR_MAX];

    for(i=0; i<SYSI_ONIE_TYPE_SUPPORT_NUM; i++ ) {
        if( TLV_CODE_PLATFORM_NAME == __tlv_vpd_info[i].type) {
            path = __tlv_vpd_info[i].file;
            break;
        }
    }
    if( SYSI_ONIE_TYPE_SUPPORT_NUM == i) {
        /* Cannot find support type */
        ret = ONLP_STATUS_E_UNSUPPORTED;
    }
    if( ONLP_STATUS_OK == ret) {
        /*get info*/
        ret = onlp_file_read((uint8_t*)buf,ONLP_CONFIG_INFO_STR_MAX, &len, path);
    }
    if( ONLP_STATUS_OK == ret) {
        /*remove \n in file output*/
        buf[strlen(buf)-1] = 0;
        *platform_name = aim_fstrdup("%s",buf);
    } else {
        /*alloc a empty array for it*/
        *platform_name = aim_zmalloc(1);
        ret = ONLP_STATUS_OK;
    }

    return ret;
}


static int _sysi_onie_onie_version_get(char** onie_version)
{
    int ret = ONLP_STATUS_OK;
    int i;
    int len;
    char* path;
    char buf[ONLP_CONFIG_INFO_STR_MAX];

    for(i=0; i<SYSI_ONIE_TYPE_SUPPORT_NUM; i++ ) {
        if( TLV_CODE_ONIE_VERSION == __tlv_vpd_info[i].type) {
            path = __tlv_vpd_info[i].file;
            break;
        }
    }
    if( SYSI_ONIE_TYPE_SUPPORT_NUM == i) {
        /* Cannot find support type */
        ret = ONLP_STATUS_E_UNSUPPORTED;
    }
    if( ONLP_STATUS_OK == ret) {
        /*get info*/
        ret = onlp_file_read((uint8_t*)buf,ONLP_CONFIG_INFO_STR_MAX, &len, path);
    }
    if( ONLP_STATUS_OK == ret) {
        /*remove \n in file output*/
        buf[strlen(buf)-1] = 0;
        *onie_version = aim_fstrdup("%s",buf);
    } else {
        /*alloc a empty array for it*/
        *onie_version = aim_zmalloc(1);
        ret = ONLP_STATUS_OK;
    }

    return ret;
}


static int _sysi_onie_mac_size_get(uint16_t* mac_size)
{
    int ret = ONLP_STATUS_OK;
    int i;
    int len;
    char* path;
    char buf[ONLP_CONFIG_INFO_STR_MAX];

    for(i=0; i<SYSI_ONIE_TYPE_SUPPORT_NUM; i++ ) {
        if( TLV_CODE_MAC_SIZE == __tlv_vpd_info[i].type) {
            path = __tlv_vpd_info[i].file;
            break;
        }
    }
    if( SYSI_ONIE_TYPE_SUPPORT_NUM == i) {
        /* Cannot find support type */
        ret = ONLP_STATUS_E_UNSUPPORTED;
    }
    if( ONLP_STATUS_OK == ret) {
        /*get info*/
        ret = onlp_file_read((uint8_t*)buf,ONLP_CONFIG_INFO_STR_MAX, &len, path);
    }
    if( ONLP_STATUS_OK == ret) {
        *mac_size = (uint16_t)strtoul(buf, NULL, 0);
    }
    /*return OK no matter what*/
    return ONLP_STATUS_OK;
}


static int _sysi_onie_manuf_name_get(char** manuf_name)
{
    int ret = ONLP_STATUS_OK;
    int i;
    int len;
    char* path;
    char buf[ONLP_CONFIG_INFO_STR_MAX];

    for(i=0; i<SYSI_ONIE_TYPE_SUPPORT_NUM; i++ ) {
        if( TLV_CODE_MANUF_NAME == __tlv_vpd_info[i].type) {
            path = __tlv_vpd_info[i].file;
            break;
        }
    }
    if( SYSI_ONIE_TYPE_SUPPORT_NUM == i) {
        /* Cannot find support type */
        ret = ONLP_STATUS_E_UNSUPPORTED;
    }
    if( ONLP_STATUS_OK == ret) {
        /*get info*/
        ret = onlp_file_read((uint8_t*)buf,ONLP_CONFIG_INFO_STR_MAX, &len, path);
    }
    if( ONLP_STATUS_OK == ret) {
        /*remove \n in file output*/
        buf[strlen(buf)-1] = 0;
        *manuf_name = aim_fstrdup("%s",buf);
    } else {
        /*alloc a empty array for it*/
        *manuf_name = aim_zmalloc(1);
        ret = ONLP_STATUS_OK;
    }

    return ret;
}


static int _sysi_onie_manuf_country_get(char** manuf_country)
{
    int ret = ONLP_STATUS_OK;
    int i;
    int len;
    char* path;
    char buf[ONLP_CONFIG_INFO_STR_MAX];

    for(i=0; i<SYSI_ONIE_TYPE_SUPPORT_NUM; i++ ) {
        if( TLV_CODE_MANUF_COUNTRY == __tlv_vpd_info[i].type) {
            path = __tlv_vpd_info[i].file;
            break;
        }
    }
    if( SYSI_ONIE_TYPE_SUPPORT_NUM == i) {
        /* Cannot find support type */
        ret = ONLP_STATUS_E_UNSUPPORTED;
    }
    if( ONLP_STATUS_OK == ret) {
        /*get info*/
        ret = onlp_file_read((uint8_t*)buf,ONLP_CONFIG_INFO_STR_MAX, &len, path);
    }
    if( ONLP_STATUS_OK == ret) {
        /*remove \n in file output*/
        buf[strlen(buf)-1] = 0;
        *manuf_country = aim_fstrdup("%s",buf);
    } else {
        /*alloc a empty array for it*/
        *manuf_country = aim_zmalloc(1);
        ret = ONLP_STATUS_OK;
    }

    return ret;
}


static int _sysi_onie_vendor_name_get(char** vendor_name)
{
    int ret = ONLP_STATUS_OK;
    int i;
    int len;
    char* path;
    char buf[ONLP_CONFIG_INFO_STR_MAX];

    for(i=0; i<SYSI_ONIE_TYPE_SUPPORT_NUM; i++ ) {
        if( TLV_CODE_VENDOR_NAME == __tlv_vpd_info[i].type) {
            path = __tlv_vpd_info[i].file;
            break;
        }
    }
    if( SYSI_ONIE_TYPE_SUPPORT_NUM == i) {
        /* Cannot find support type */
        ret = ONLP_STATUS_E_UNSUPPORTED;
    }
    if( ONLP_STATUS_OK == ret) {
        /*get info*/
        ret = onlp_file_read((uint8_t*)buf,ONLP_CONFIG_INFO_STR_MAX, &len, path);
    }
    if( ONLP_STATUS_OK == ret) {
        /*remove \n in file output*/
        buf[strlen(buf)-1] = 0;
        *vendor_name = aim_fstrdup("%s",buf);
    } else {
        /*alloc a empty array for it*/
        *vendor_name = aim_zmalloc(1);
        ret = ONLP_STATUS_OK;
    }

    return ret;
}


static int _sysi_onie_diag_version_get(char** diag_version)
{
    int ret = ONLP_STATUS_OK;
    int i;
    int len;
    char* path;
    char buf[ONLP_CONFIG_INFO_STR_MAX];

    for(i=0; i<SYSI_ONIE_TYPE_SUPPORT_NUM; i++ ) {
        if( TLV_CODE_DIAG_VERSION == __tlv_vpd_info[i].type) {
            path = __tlv_vpd_info[i].file;
            break;
        }
    }
    if( SYSI_ONIE_TYPE_SUPPORT_NUM == i) {
        /* Cannot find support type */
        ret = ONLP_STATUS_E_UNSUPPORTED;
    }
    if( ONLP_STATUS_OK == ret) {
        /*get info*/
        ret = onlp_file_read((uint8_t*)buf,ONLP_CONFIG_INFO_STR_MAX, &len, path);
    }
    if( ONLP_STATUS_OK == ret) {
        /*remove \n in file output*/
        buf[strlen(buf)-1] = 0;
        *diag_version = aim_fstrdup("%s",buf);
    } else {
        /*alloc a empty array for it*/
        *diag_version = aim_zmalloc(1);
        ret = ONLP_STATUS_OK;
    }

    return ret;
}


static int _sysi_onie_service_tag_get(char** service_tag)
{
    int ret = ONLP_STATUS_OK;
    int i;
    int len;
    char* path;
    char buf[ONLP_CONFIG_INFO_STR_MAX];

    for(i=0; i<SYSI_ONIE_TYPE_SUPPORT_NUM; i++ ) {
        if( TLV_CODE_SERVICE_TAG == __tlv_vpd_info[i].type) {
            path = __tlv_vpd_info[i].file;
            break;
        }
    }
    if( SYSI_ONIE_TYPE_SUPPORT_NUM == i) {
        /* Cannot find support type */
        ret = ONLP_STATUS_E_UNSUPPORTED;
    }
    if( ONLP_STATUS_OK == ret) {
        /*get info*/
        ret = onlp_file_read((uint8_t*)buf,ONLP_CONFIG_INFO_STR_MAX, &len, path);
    }
    if( ONLP_STATUS_OK == ret) {
        /*remove \n in file output*/
        buf[strlen(buf)-1] = 0;
        *service_tag = aim_fstrdup("%s",buf);
    } else {
        /*alloc a empty array for it*/
        *service_tag = aim_zmalloc(1);
        ret = ONLP_STATUS_OK;
    }

    return ret;
}


static int _sysi_onie_vendor_ext_get(list_head_t* vendor_ext)
{
    int ret = ONLP_STATUS_OK;
    int i;
    int len;
    char* path;
    char buf[ONLP_CONFIG_INFO_STR_MAX];

    list_init(vendor_ext);

    for(i=0; i<SYSI_ONIE_TYPE_SUPPORT_NUM; i++ ) {
        if( TLV_CODE_VENDOR_EXT == __tlv_vpd_info[i].type) {
            path = __tlv_vpd_info[i].file;
            break;
        }
    }
    if( SYSI_ONIE_TYPE_SUPPORT_NUM == i) {
        /* Cannot find support type */
        ret = ONLP_STATUS_E_UNSUPPORTED;
    }
    if( ONLP_STATUS_OK == ret) {
        /*get info*/
        ret = onlp_file_read((uint8_t*)buf,ONLP_CONFIG_INFO_STR_MAX, &len, path);
    }
    if( ONLP_STATUS_OK == ret) {
        /*TODO*/
    }
    /*return OK no matter what*/
    return ONLP_STATUS_OK;
}


static int _sysi_onie_crc_32_get(uint32_t* crc_32)
{
    int ret = ONLP_STATUS_OK;
    int i;
    int len;
    char* path;
    char buf[ONLP_CONFIG_INFO_STR_MAX];

    for(i=0; i<SYSI_ONIE_TYPE_SUPPORT_NUM; i++ ) {
        if( TLV_CODE_CRC_32 == __tlv_vpd_info[i].type) {
            path = __tlv_vpd_info[i].file;
            break;
        }
    }
    if( SYSI_ONIE_TYPE_SUPPORT_NUM == i) {
        /* Cannot find support type */
        ret = ONLP_STATUS_E_UNSUPPORTED;
    }
    if( ONLP_STATUS_OK == ret) {
        /*get info*/
        ret = onlp_file_read((uint8_t*)buf,ONLP_CONFIG_INFO_STR_MAX, &len, path);
    }
    if( ONLP_STATUS_OK == ret) {
        *crc_32 = (uint32_t)strtoul (buf, NULL, 0);
    }
    /*return OK no matter what*/
    return ONLP_STATUS_OK;
}

static int _sysi_onie_info_total_len_get(onlp_onie_info_t *onie, uint16_t *total_len)
{
    uint16_t len = 0;

    /*product_name*/
    if(strlen(onie->product_name)!= 0) {
        len += 2;
        len += strlen(onie->product_name);
    }
    /*part_number*/
    if(strlen(onie->part_number)!= 0) {
        len += 2;
        len += strlen(onie->part_number);
    }
    /*serial_number*/
    if(strlen(onie->serial_number)!= 0) {
        len += 2;
        len += strlen(onie->serial_number);
    }

    /*mac*/
    len += 2;
    len += 6;

    /*manufacture_date*/
    if(strlen(onie->manufacture_date)!= 0) {
        len += 2;
        len += 19;
    }

    /*device_version*/
    len += 2;
    len += 1;

    /*label_revision*/
    if(strlen(onie->label_revision)!= 0) {
        len += 2;
        len += strlen(onie->label_revision);
    }

    /*platform_name*/
    if(strlen(onie->platform_name)!= 0) {
        len += 2;
        len += strlen(onie->platform_name);
    }

    /*onie_version*/
    if(strlen(onie->onie_version)!= 0) {
        len += 2;
        len += strlen(onie->onie_version);
    }

    /*mac_range*/
    len += 2;
    len += 2;

    /*manufacturer*/
    if(strlen(onie->manufacturer)!= 0) {
        len += 2;
        len += strlen(onie->manufacturer);
    }

    /*country_code*/
    if(strlen(onie->country_code)!= 0) {
        len += 2;
        len += 2;
    }

    /*vendor*/
    if(strlen(onie->vendor)!= 0) {
        len += 2;
        len += strlen(onie->vendor);
    }

    /*diag_version*/
    if(strlen(onie->diag_version)!= 0) {
        len += 2;
        len += strlen(onie->diag_version);
    }

    /*service_tag*/
    if(strlen(onie->service_tag)!= 0) {
        len += 2;
        len += strlen(onie->service_tag);
    }

    /*crc*/
    len += 2;
    len += 4;

    /*vx_list*/
    /*TODO*/

    *total_len = len;
    return ONLP_STATUS_OK;
}



const char*
onlp_sysi_platform_get(void)
{
    return "x86-64-ufispace-s9710-76d-r0";
}


/*
 * This function is called to return the physical base address
 * of the ONIE boot rom.
 *
 * The ONLP framework will mmap() and parse the ONIE TLV structure
 * from the given data.
 *
 * If you platform does not support a mappable address for the ONIE
 * eeprom then you should not provide this function at all.
 *
 * For the purposes of this example we will provide it but
 * return UNSUPPORTED (which is all the default implementation does).
 *
 */
int
onlp_sysi_onie_data_phys_addr_get(void** pa)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}


/*
 * If you cannot provide a base address you must provide the ONLP
 * framework the raw ONIE data through whatever means necessary.
 *
 * This function will be called as a backup in the event that
 * onlp_sysi_onie_data_phys_addr_get() fails.
 */
int
onlp_sysi_onie_data_get(uint8_t** data, int* size)
{
#if 0
    int ret;
    int i;

    /*
     * This represents the example ONIE data.
     */
    static uint8_t onie_data[] = {
        'T', 'l', 'v','I','n','f','o', 0,
        0x1, 0x0, 0x0,
        0x21, 0x8, 'O', 'N', 'L', 'P', 'I', 'E', 0, 0,
        0x22, 0x3, 'O', 'N', 'L',
        0xFE, 0x4, 0x4b, 0x1b, 0x1d, 0xde,
    };


    memcpy(*data, onie_data, ONLPLIB_CONFIG_I2C_BLOCK_SIZE);
    return 0;
#endif
    return ONLP_STATUS_E_UNSUPPORTED;
}

static int __onlp_sysi_onie_info_get (onlp_onie_info_t *onie_info)
{
    int ret = ONLP_STATUS_OK;
    uint16_t total_len;

    if(ONLP_STATUS_OK == ret) {
        ret = _sysi_onie_product_name_get(&onie_info->product_name);
    }

    if(ONLP_STATUS_OK == ret) {
        ret = _sysi_onie_part_number_get(&onie_info->part_number);
    }

    if(ONLP_STATUS_OK == ret) {
        ret = _sysi_onie_serial_number_get(&onie_info->serial_number);
    }

    if(ONLP_STATUS_OK == ret) {
        ret = _sysi_onie_mac_base_get(onie_info->mac);
    }

    if(ONLP_STATUS_OK == ret) {
        ret = _sysi_onie_manuf_date_get(&onie_info->manufacture_date);
    }

    if(ONLP_STATUS_OK == ret) {
        ret = _sysi_onie_device_version_get(&onie_info->device_version);
    }

    if(ONLP_STATUS_OK == ret) {
        ret = _sysi_onie_label_revision_get(&onie_info->label_revision);
    }

    if(ONLP_STATUS_OK == ret) {
        ret = _sysi_onie_platform_name_get(&onie_info->platform_name);
    }

    if(ONLP_STATUS_OK == ret) {
        ret = _sysi_onie_onie_version_get(&onie_info->onie_version);
    }

    if(ONLP_STATUS_OK == ret) {
        ret = _sysi_onie_mac_size_get(&onie_info->mac_range);
    }

    if(ONLP_STATUS_OK == ret) {
        ret = _sysi_onie_manuf_name_get(&onie_info->manufacturer);
    }

    if(ONLP_STATUS_OK == ret) {
        ret = _sysi_onie_manuf_country_get(&onie_info->country_code);
    }

    if(ONLP_STATUS_OK == ret) {
        ret = _sysi_onie_vendor_name_get(&onie_info->vendor);
    }

    if(ONLP_STATUS_OK == ret) {
        ret = _sysi_onie_diag_version_get(&onie_info->diag_version);
    }

    if(ONLP_STATUS_OK == ret) {
        ret = _sysi_onie_service_tag_get(&onie_info->service_tag);
    }

    if(ONLP_STATUS_OK == ret) {
        ret = _sysi_onie_vendor_ext_get(&onie_info->vx_list);
    }

    if(ONLP_STATUS_OK == ret) {
        ret = _sysi_onie_crc_32_get(&onie_info->crc);
    }

    _sysi_onie_info_total_len_get(onie_info, &total_len);

    onie_info->_hdr_id_string = aim_fstrdup("TlvInfo");
    onie_info->_hdr_version = 0x1;
    onie_info->_hdr_length = total_len;
    return ret;
}

int __asset_versions(onlp_oid_t oid, onlp_asset_info_t* asset_info)
{
    int cpu_cpld_addr = 0x600, cpu_cpld_ver, cpu_cpld_ver_major, cpu_cpld_ver_minor;
    int cpld_ver[CPLD_MAX], cpld_ver_major[CPLD_MAX], cpld_ver_minor[CPLD_MAX], cpld_build[CPLD_MAX];
    int mb_cpld1_addr = 0xE01, mb_cpld1_board_type_rev, mb_cpld1_hw_rev, mb_cpld1_build_rev;
    int i;
    char bios_out[32];
    char bmc_out1[8], bmc_out2[8], bmc_out3[8];

    onlp_onie_info_t onie_info;
    
    memset(bios_out, 0, sizeof(bios_out));
    memset(bmc_out1, 0, sizeof(bmc_out1));
    memset(bmc_out2, 0, sizeof(bmc_out2));
    memset(bmc_out3, 0, sizeof(bmc_out3));

    //get CPU CPLD version
    if (read_ioport(cpu_cpld_addr, &cpu_cpld_ver) < 0) { 
        AIM_LOG_ERROR("unable to read CPU CPLD version\n");
        return ONLP_STATUS_E_INTERNAL; 
    }    
    cpu_cpld_ver_major = (((cpu_cpld_ver) >> 6 & 0x03));
    cpu_cpld_ver_minor = (((cpu_cpld_ver) & 0x3F));
    
    //get MB CPLD version
    for(i=0; i<CPLD_MAX; ++i) {        
        ONLP_TRY(file_read_hex(&cpld_ver[i], "/sys/bus/i2c/devices/%d-00%02x/cpld_version",
                                 CPLD_I2C_BUS[i], CPLD_BASE_ADDR[i]));
        
        if (cpld_ver[i] < 0) {            
            AIM_LOG_ERROR("unable to read MB CPLD version\n");
            return ONLP_STATUS_E_INTERNAL;             
        }
       
        cpld_ver_major[i] = (((cpld_ver[i]) >> 6 & 0x03));
        cpld_ver_minor[i] = (((cpld_ver[i]) & 0x3F));

        ONLP_TRY(file_read_hex(&cpld_build[i], "/sys/bus/i2c/devices/%d-00%02x/cpld_build",
                                 CPLD_I2C_BUS[i], CPLD_BASE_ADDR[i]));
        
        if (cpld_build[i] < 0) {            
            AIM_LOG_ERROR("unable to read MB CPLD build\n");
            return ONLP_STATUS_E_INTERNAL;             
        }        
    }    

    asset_info->cpld_revision = aim_fstrdup(
        "\n" 
        "[CPU CPLD] %d.%02d\n"
        "[MB CPLD1] %d.%02d.%d\n"
        "[MB CPLD2] %d.%02d.%d\n"
        "[MB CPLD3] %d.%02d.%d\n"
        "[MB CPLD4] %d.%02d.%d\n" 
        "[MB CPLD5] %d.%02d.%d\n",
        cpu_cpld_ver_major, cpu_cpld_ver_minor,
        cpld_ver_major[0], cpld_ver_minor[0], cpld_build[0],
        cpld_ver_major[1], cpld_ver_minor[1], cpld_build[1],
        cpld_ver_major[2], cpld_ver_minor[2], cpld_build[2],
        cpld_ver_major[3], cpld_ver_minor[3], cpld_build[3],
        cpld_ver_major[4], cpld_ver_minor[4], cpld_build[4]);

    //Get HW Build Version
    if (read_ioport(mb_cpld1_addr, &mb_cpld1_board_type_rev) < 0) {
        AIM_LOG_ERROR("unable to read MB CPLD1 Board Type Revision\n");
        return ONLP_STATUS_E_INTERNAL;
    }
    mb_cpld1_hw_rev = (((mb_cpld1_board_type_rev) >> 0 & 0x03));
    mb_cpld1_build_rev = ((mb_cpld1_board_type_rev) >> 3 & 0x07);

    //Get BIOS version 
    if (exec_cmd(CMD_BIOS_VER, bios_out, sizeof(bios_out)) < 0) {
        AIM_LOG_ERROR("unable to read BIOS version\n");
        return ONLP_STATUS_E_INTERNAL;
    }

    //Get BMC version
    if (exec_cmd(CMD_BMC_VER_1, bmc_out1, sizeof(bmc_out1)) < 0 ||
        exec_cmd(CMD_BMC_VER_2, bmc_out2, sizeof(bmc_out2)) < 0 ||
        exec_cmd(CMD_BMC_VER_3, bmc_out3, sizeof(bmc_out3))) {
            AIM_LOG_ERROR("unable to read BMC version\n");
            return ONLP_STATUS_E_INTERNAL;
    }

    asset_info->firmware_revision = aim_fstrdup(
            "\n"
            "    [HW   ] %d\n"
            "    [BUILD] %d\n"
            "    [BIOS ] %s\n"
            "    [BMC  ] %d.%d.%d\n",
            mb_cpld1_hw_rev,
            mb_cpld1_build_rev,
            bios_out,
            atoi(bmc_out1), atoi(bmc_out2), atoi(bmc_out3));

    /* get platform info from onie syseeprom */
    onlp_attributei_onie_info_get(oid, &onie_info);

    asset_info->oid = oid;
    ONIE_FIELD_CPY(asset_info, onie_info, manufacturer)
    ONIE_FIELD_CPY(asset_info, onie_info, part_number)
    ONIE_FIELD_CPY(asset_info, onie_info, serial_number)
    ONIE_FIELD_CPY(asset_info, onie_info, manufacture_date)

    return ONLP_STATUS_OK;
}

/**
 * @brief Initialize the attribute subsystem.
 */
int onlp_attributei_sw_init(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Initialize the attribute subsystem.
 */
int onlp_attributei_hw_init(uint32_t flags)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Deinitialize the attribute subsystem.
 */
int onlp_attributei_sw_denit(void)
{
    return ONLP_STATUS_OK;
}


/**
 * Access to standard attributes.
 */

/**
 * @brief Get an OID's ONIE attribute.
 * @param oid The target OID
 * @param onie_info [out] Receives the ONIE information if supported.
 * @note if onie_info is NULL you should only return whether the attribute is supported.
 */
int onlp_attributei_onie_info_get(onlp_oid_t oid, onlp_onie_info_t* onie_info)
{
    if(oid != ONLP_OID_CHASSIS) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if(onie_info == NULL) {
        return ONLP_STATUS_OK;
    }

    return __onlp_sysi_onie_info_get(onie_info);
}

/**
 * @brief Get an OID's Asset attribute.
 * @param oid The target OID.
 * @param asset_info [out] Receives the Asset information if supported.
 * @note if asset_info is NULL you should only return whether the attribute is supported.
 */
int onlp_attributei_asset_info_get(onlp_oid_t oid, onlp_asset_info_t* asset_info)
{
    if(oid != ONLP_OID_CHASSIS) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if(asset_info == NULL) {
        return ONLP_STATUS_OK;
    }

    asset_info->oid = oid;

    __asset_versions(oid, asset_info);

    return ONLP_STATUS_OK;
}
