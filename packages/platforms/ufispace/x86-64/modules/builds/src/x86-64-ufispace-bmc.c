/*
 * A BMC kernel dirver for ufispace platform
 *
 * Copyright (C) 2026 Ufispace Technology Corporation.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/hwmon-sysfs.h>
#include <linux/io.h>
#include <linux/ipmi.h>
#include <linux/ipmi_smi.h>
#include <linux/delay.h>
#include <linux/version.h>
#include "x86-64-ufispace-bmc.h"

#define DRVNAME  "x86_64_ufispace_bmc"
#define GROUP_NAME "bmc"

#define ufi_bmc_pr_err(dev, fmt, ...) \
    dev_err(dev, "%s(#%d): "pr_fmt(fmt), \
        __func__, __LINE__, ##__VA_ARGS__)

#define ufi_bmc_pr_warn(dev, fmt, ...) \
    dev_warn(dev, "%s(#%d): "pr_fmt(fmt), \
        __func__, __LINE__, ##__VA_ARGS__)

#define ufi_bmc_pr_info(dev, fmt, ...) \
    dev_info(dev, "%s(#%d): "pr_fmt(fmt), \
        __func__, __LINE__, ##__VA_ARGS__)

#define _DEVICE_ATTR(_name)     \
    &dev_attr_##_name

#define _BIN_ATTR(_name)     \
    &bin_attr_##_name

#define DUMP_TABLE_MAX_SIZE (PAGE_SIZE * 120)

// unit is seconds
#define CACHE_TIMEOUT                 (5)

// unit is millisecond
#define CMD_RETRY_DELAY               (1000)

#define SYSFS_NAME_MAX_LEN            (64)

/**
 * IPMI Net function code
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 5.1
 */
#define IPMI_NETFN_CHASSIS                      0x00
#define IPMI_NETFN_SENSOR                       0x04
#define IPMI_NETFN_APP                          0x06
#define IPMI_NETFN_OEMM                         0x32
#define IPMI_NETFN_OEMM2                        0x3C
#define IPMI_NETFN_STORAGE                      0x0A
#define IPMI_NETFN_OEM_UFI                      0x3C

/**
 * IPMI Completion Codes
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 5.2
 */
#define IPMI_COMPLETION_CODE_RESERVATION_FAIL   0xC5

/**
 * IPMI Command header size
 * 1 byte for NetFun/LUN and 1 byte for Cmd
 * So there are remain 38 bytes for data.
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 9.2, CH 9.3
 */
#define IPMI_CMD_HEADER_SIZE         2

/**
 * IPMI KCS/SMIC maximun request size
 * KCS/SMIC input minimum requirement is 40 byte.
 * 1 byte for NetFun/LUN and 1 byte for Cmd
 * So there are remain 38 bytes for data.
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 6.14
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 9.2
 */
#define IPMI_OPENIPMI_MAX_REQ_DATA_SIZE     \
    (IPMI_OPENIPMI_MAX_REQ_SIZE - IPMI_CMD_HEADER_SIZE)

/**
 * IPMI KCS/SMIC maximun response size
 * KCS/SMIC output minimum requirement is 38 byte.
 * 1 byte for NetFun/LUN and 1 byte for Cmd
 * So there are remain 36 bytes for data.
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 6.14
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 9.3
 */
#define IPMI_OPENIPMI_MAX_RSP_SIZE          38
#define IPMI_OPENIPMI_MAX_RSP_DATA_SIZE     \
    (IPMI_OPENIPMI_MAX_RSP_SIZE - IPMI_CMD_HEADER_SIZE)

/**
 * logical/physical FRU device bit
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 20.1, byte 4
 */
#define IPMI_FIRMWARE_REVISION_DEVICE (0x80)

/**
 * Each unit of the IPMI Watchdog Timer is 100 milliseconds.
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 27.6
 */
#define IPMI_WATCHDOG_TIMER_UNIT_MILLISECONDS                100

/**
 * Chassis Control
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 28.3
 */
#define IPMI_CHASSIS_CONTROL_POWER_CYCLE                0x2

/**
 * IPMI SDR data maximun response size
 * the response Data require 1 byte for completion code and
 * 2 byte for record ID for next record.
 * So there are remain 33 bytes for SDR data.
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 33.12
 */
#define IPMI_OPENIPMI_SDR_MAX_RSP_DATA_SIZE \
    (IPMI_OPENIPMI_MAX_RSP_DATA_SIZE - 3)

/**
 * IPMI Fru data header size
 * The response Data require 1 byte for completion code and
 * 1 byte for return count.
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 34.2
 */
#define IPMI_FRU_DATA_RSP_HEADER_SIZE         2

/**
 * IPMI Fru data maximun response size
 * The response Data require 1 byte for completion code and
 * 1 byte for return count.
 * So there are remain 34 bytes for sdr data.
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 34.2
 */
#define IPMI_FRU_MAX_RSP_BYTE   \
    (IPMI_OPENIPMI_MAX_RSP_DATA_SIZE - IPMI_FRU_DATA_RSP_HEADER_SIZE)

/**
 * IPMI Readable thresholds:
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 35.9
 */
#define IPMI_SENSOR_THRESHOLD_MASK_LOWER_NONE_CRITICAL      0x01
#define IPMI_SENSOR_THRESHOLD_MASK_LOWER_CRITICAL           0x02
#define IPMI_SENSOR_THRESHOLD_MASK_LOWER_NONE_RECOVERABLE   0x04
#define IPMI_SENSOR_THRESHOLD_MASK_UPPER_NONE_CRITICAL      0x08
#define IPMI_SENSOR_THRESHOLD_MASK_UPPER_CRITICAL           0x10
#define IPMI_SENSOR_THRESHOLD_MASK_UPPER_NONE_RECOVERABLE   0x20

/**
 * ipmi cache max size
 * Since sensor reading is 1 byte, sensor event is 2 byte and
 * fru data filed max is 64 byte. So we use 64 as max size
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 35.14 byte 2,4,5
 * platform-management-fru-document-rev-1-2-feb-2013,
 * CH 13. TYPE/LENGTH BYTE FORMAT, Length:bit 0-5
 */
#define IPMI_CACHE_BUF_MAX_SIZE    (64)

/**
 * IPMI Sensor reading state unavailable mask:
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 35.14, byte 3
 */
#define IPMI_SENSOR_READING_STATE_MASK_UNAVAILABLE        0x20
#define IPMI_SENSOR_READING_STATE_MASK_SCANNING_DISABLED  0x40

/**
 * IPMI SDR type
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 43
 */
#define SDR_RECORD_TYPE_FULL_SENSOR          0x01
#define SDR_RECORD_TYPE_COMPACT_SENSOR       0x02
#define SDR_RECORD_TYPE_FRU_DEVICE_LOCATOR   0x11
#define SDR_RECORD_TYPE_OEM                  0xC0

/**
 * Chassis Control
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 43.1 Byte 21
 */
#define IPMI_SENSOR_UNIT_ANALOG_DATA_FMT_MASK           0xC0
#define IPMI_SENSOR_UNIT_ANALOG_DATA_FMT_POS            6

/**
 * ID String Bytes
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 43.1, CH 43.2, CH 43.8
 */
#define IPMI_SDR_ID_MAX_LEN           (16)

/**
 * SDR Version
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 43.1, CH 43.2, CH 43.8 byte 3
 */
#define IPMI_SDR_VERSION       (0x51)

/**
 * SDR Type 01h data size
 * SDR Type 01h, Full Sensor Record 48+16 ID string, the max buf is 64
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 43.1
 */
#define IPMI_SDR_FULL_SIZE     (64)

/**
 * logical/physical FRU device bit
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 43.8
 * Table 43-, FRU Device Locator Record - SDR Type 11h byte 8
 */
#define IPMI_SDR_LOGICAL_PHYSICAL_MASK (0x80)

/**
 * ID String Bytes
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 43.15
 */
#define IPMI_SDR_ID_TLV_LEN_MASK      (0x1f)


/**
 * IPMI Fru data common headrt size
 * 1 byte Common Header Format Version
 * 1 byte Internal Use Area Starting Offset
 * 1 byte Chassis Info Area Starting Offset
 * 1 byte Board Area Starting Offset
 * 1 byte Product Info Area Starting Offset
 * 1 byte MultiRecord Area Starting Offset
 * 1 byte PAD
 * 1 byte Common Header Checksum
 * ref:
 * platform-management-fru-document-rev-1-2-feb-2013, CH 8
 */
#define IPMI_FRU_COMM_HEADER_SIZE            8
#define IPMI_FRU_AREA_OFFSET_MULTIPLIER      8

/**
 * IPMI Fru product info area format
 * ref:
 * platform-management-fru-document-rev-1-2-feb-2013, CH 12
 */
// Product Area Format Version + Product Area Length
#define IPMI_FRU_PRODUCT_HEADER_SIZE         2

#define IPMI_FRU_PRODUCT_LAN_CODE_SIZE       1
#define IPMI_FRU_PRODUCT_DATA_OFFSET_MF      0
#define IPMI_FRU_PRODUCT_DATA_OFFSET_PNAME   1
#define IPMI_FRU_PRODUCT_DATA_OFFSET_PN      2
#define IPMI_FRU_PRODUCT_DATA_OFFSET_PV      3
#define IPMI_FRU_PRODUCT_DATA_OFFSET_SN      4

#define IPMI_FRU_PRODUCT_DATA_LEN_MULTIPLIER  8

/**
 * IPMI Fru product info area format
 * ref:
 * platform-management-fru-document-rev-1-2-feb-2013, CH 13
 */
#define IPMI_FRU_TLV_TYPE_MASK               0xC0
#define IPMI_FRU_TLV_LENGTH_MASK             0x3F

/**
 * IPMI Net function code
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, Appendix G
 */
#define IPMI_CMD_DEVICE_IDE_GET                 0x01
#define IPMI_CMD_CHASSIS_CONTROL                0x02
#define IPMI_CMD_RESERVE_SDR_REPO               0x22
#define IPMI_CMD_SDR_GET                        0x23
#define IPMI_CMD_WATCHDOG_TIMER_RESET           0x22
#define IPMI_CMD_WATCHDOG_TIMER_SET             0x24
#define IPMI_CMD_WATCHDOG_TIMER_GET             0x25
#define IPMI_CMD_SENSOR_THRESHOLD               0x27
#define IPMI_CMD_SENSOR_READ                    0x2d
#define IPMI_CMD_SENSOR_TYPE                    0x2f
#define IPMI_CMD_OEM_UFI_BMC_BOOT_FLASH         0x8f
#define IPMI_CMD_OEM_UFI_PSU_RESET              0x26
#define IPMI_CMD_FRU_AREA_INFO_GET              0x10
#define IPMI_CMD_FRU_DATA_GET                   0x11


#define IPMI_TIMEOUT                            (1 * HZ)
#define IPMI_ERR_RETRY_TIMES                    3

#define VALUE_FACTOR          1
#define VALUE_FACTOR_MILLI    1000 // 10^3
#define VALUE_FACTOR_MICRO    1000000 // 10^6

#define BSWAP_16(x) ((((x) & 0xff00) >> 8) | (((x) & 0x00ff) << 8))
#define BSWAP_32(x) ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >> 8) | \
                     (((x) & 0x0000ff00) << 8) | (((x) & 0x000000ff) << 24))
#define tos32(val, bits) ((val & ((1<<((bits)-1)))) ? \
                            (-((val) & (1<<((bits)-1))) | (val)) : (val))
#define __TO_M(mtol) (int16_t)(tos32((((BSWAP_16(mtol) & 0xff00) >> 8) | \
                        ((BSWAP_16(mtol) & 0xc0) << 2)), 10))
#define __TO_B(bacc) (int32_t)(tos32((((BSWAP_32(bacc) & 0xff000000) >> 24) | \
                        ((BSWAP_32(bacc) & 0xc00000) >> 14)), 10))
#define __TO_R_EXP(bacc) (int32_t)(tos32(((BSWAP_32(bacc) & 0xf0) >> 4), 4))
#define __TO_B_EXP(bacc) (int32_t)(tos32((BSWAP_32(bacc) & 0xf), 4))

#define IS_SENSOR_THRESHOLD_SUB_ID(sub_id) ( \
    sub_id == SENSOR_SUB_ID_UPPER_NONE_CRITICAL || \
    sub_id == SENSOR_SUB_ID_UPPER_CRITICAL || \
    sub_id == SENSOR_SUB_ID_UPPER_NONE_RECOVERABLE || \
    sub_id == SENSOR_SUB_ID_LOWER_NONE_CRITICAL || \
    sub_id == SENSOR_SUB_ID_LOWER_CRITICAL || \
    sub_id == SENSOR_SUB_ID_LOWER_NONE_RECOVERABLE || \
    sub_id == SENSOR_SUB_ID_TEMP_MIN || \
    sub_id == SENSOR_SUB_ID_TEMP_MAX)

#define IS_SENSOR_UNIT_TYPE_TEMPERATURE(sensor_unit) ( \
    sensor_unit == SENSOR_UNIT_TYPE_CODE_DEGREES_C || \
    sensor_unit == SENSOR_UNIT_TYPE_CODE_DEGREES_F || \
    sensor_unit == SENSOR_UNIT_TYPE_CODE_DEGREES_K)

#define IS_SENSOR_UNIT_TYPE_VOLTAGE_CURRENT(sensor_unit) ( \
    sensor_unit == SENSOR_UNIT_TYPE_CODE_VOLTS || \
    sensor_unit == SENSOR_UNIT_TYPE_CODE_AMPS)

#define IS_SENSOR_UNIT_TYPE_POWER(sensor_unit) ( \
    sensor_unit == SENSOR_UNIT_TYPE_CODE_WATTS)

#define to_bmc_dev_attr(_dev_attr) \
    container_of(_dev_attr, struct bmc_dev_attr, dev_attr)

#define to_dev_attr(_attr) \
    container_of(_attr, struct device_attribute, attr);

enum bmc_attr_type {
    ATTR_TYPE_SDR       = 0,
    ATTR_TYPE_DEFAULT,
    ATTR_TYPE_UDF
};

enum data_valid_type {
    DATA_VALID_TYPE_INVALID = 0,
    DATA_VALID_TYPE_VALID,
    DATA_VALID_TYPE_PERMANENT_VALID,
};

enum sensor_sub_id {
    SENSOR_SUB_ID_READING = 0,
    SENSOR_SUB_ID_SENSOR_ID,
    SENSOR_SUB_ID_SENSOR_NAME,
    SENSOR_SUB_ID_SENSOR_TYPE,
    SENSOR_SUB_ID_SENSOR_UNIT,
    SENSOR_SUB_ID_UPPER_NONE_CRITICAL,
    SENSOR_SUB_ID_UPPER_CRITICAL,
    SENSOR_SUB_ID_UPPER_NONE_RECOVERABLE,
    SENSOR_SUB_ID_LOWER_NONE_CRITICAL,
    SENSOR_SUB_ID_LOWER_CRITICAL,
    SENSOR_SUB_ID_LOWER_NONE_RECOVERABLE,
    SENSOR_SUB_ID_TEMP_MIN,
    SENSOR_SUB_ID_TEMP_MAX,
};

enum fru_sub_id {
    FRU_SUB_ID_RESERVE = 0,
    FRU_SUB_ID_MF,
    FRU_SUB_ID_PNAME,
    FRU_SUB_ID_PN,
    FRU_SUB_ID_PV,
    FRU_SUB_ID_SN,
};

enum default_id {
    DEFAULT_ID_FIREMWARE_VERSION = 0,
    DEFAULT_ID_FIREMWARE_VERSION_LONG,
    DEFAULT_ID_WATCHDOG,
    DEFAULT_ID_POWER_CYCLE,
    DEFAULT_ID_OEM_UFI_PSU_RESET,
};

enum wdg_sub_id {
    WDG_SUB_ID_STATE = 0,
    WDG_SUB_ID_TIMELEFT,
    WDG_SUB_ID_TIMEOUT,
    WDG_SUB_ID_ENABLE,
};

/**
 * Event/Reading Type Codes
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 42.1
 */
enum event_reading_type_code {
    EVENT_READING_TYPE_CODE_THRESHOLD     = 0x01,
    EVENT_READING_TYPE_CODE_GENERIC_BEGIN = 0x02,
    EVENT_READING_TYPE_CODE_GENERIC_END   = 0x0C,
    EVENT_READING_TYPE_CODE_SENSOR        = 0x6F,
    EVENT_READING_TYPE_CODE_OEM_BEGIN     = 0x70,
    EVENT_READING_TYPE_CODE_OEM_END       = 0x7F,
};

/**
 * Generic Event/Reading Type Codes
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 42.1
 */
enum generic_event_reading_type_code {
    GENERIC_EVENT_READING_TYPE_CODE_THRESHOLD_STATE                     = 0x01,
    GENERIC_EVENT_READING_TYPE_CODE_USAGE_STATE                         = 0x02,
    GENERIC_EVENT_READING_TYPE_CODE_DIGITAL_DISCRETE_STATE              = 0x03,
    GENERIC_EVENT_READING_TYPE_CODE_DIGITAL_DISCRETE_PREDICTIVE         = 0x04,
    GENERIC_EVENT_READING_TYPE_CODE_DIGITAL_DISCRETE_LIMIT              = 0x05,
    GENERIC_EVENT_READING_TYPE_CODE_DIGITAL_DISCRETE_PERFORMANCE        = 0x06,
    GENERIC_EVENT_READING_TYPE_CODE_SEVERITY_TRANSITION                 = 0x07,
    GENERIC_EVENT_READING_TYPE_CODE_AVAILABILITY_DEVICE                 = 0x08,
    GENERIC_EVENT_READING_TYPE_CODE_AVAILABILITY_DEVICE_FUNC            = 0x09,
    GENERIC_EVENT_READING_TYPE_CODE_AVAILABILITY_TRANSITION             = 0x0A,
    GENERIC_EVENT_READING_TYPE_CODE_OTHER_AVAILABILITY_REDUNDANCY       = 0x0B,
    GENERIC_EVENT_READING_TYPE_CODE_OTHER_AVAILABILITY_ACPI_DEVICEPOWER = 0x0C,
};

/**
 * Sensor Type Codes and Data
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 42.2
 */
enum sensor_type_code {
    SENSOR_TYPE_CODE_RESERVE                             = 0x00,
    SENSOR_TYPE_CODE_TEMPERATURE                         = 0x01,
    SENSOR_TYPE_CODE_VOLTAGE                             = 0x02,
    SENSOR_TYPE_CODE_CURRENT                             = 0x03,
    SENSOR_TYPE_CODE_FAN                                 = 0x04,
    SENSOR_TYPE_CODE_PHYSICAL_SECURITY                   = 0x05,
    SENSOR_TYPE_CODE_PLATFORM_SECURITY_VIOLATION_ATTEMPT = 0x06,
    SENSOR_TYPE_CODE_PROCESSOR                           = 0x07,
    SENSOR_TYPE_CODE_POWER_SUPPLY                        = 0x08,
    SENSOR_TYPE_CODE_POWER_UNIT                          = 0x09,
    SENSOR_TYPE_CODE_COOLING_DEVICE                      = 0x0A,
    SENSOR_TYPE_CODE_OTHER_SENSOR                        = 0x0B,
    SENSOR_TYPE_CODE_MEMORY                              = 0x0C,
    SENSOR_TYPE_CODE_DRIVE_SLOT                          = 0x0D,
    SENSOR_TYPE_CODE_POST_MEMORY_RESIZE                  = 0x0E,
    SENSOR_TYPE_CODE_SYSTEM_FIRMWARE_PROGRESS            = 0x0F,
    SENSOR_TYPE_CODE_EVENT_LOGGING_DISABLED              = 0x10,
    SENSOR_TYPE_CODE_WATCHDOG_1                          = 0x11,
    SENSOR_TYPE_CODE_SYSTEM_EVENT                        = 0x12,
    SENSOR_TYPE_CODE_CRITICAL_INTERRUPT                  = 0x13,
    SENSOR_TYPE_CODE_BUTTON_SWITCH                       = 0x14,
    SENSOR_TYPE_CODE_MODULE_BOARD                        = 0x15,
    SENSOR_TYPE_CODE_MICROCONTROLLER_COPROCESSOR         = 0x16,
    SENSOR_TYPE_CODE_ADD_IN_CARD                         = 0x17,
    SENSOR_TYPE_CODE_CHASSIS                             = 0x18,
    SENSOR_TYPE_CODE_CHIP_SET                            = 0x19,
    SENSOR_TYPE_CODE_OTHER_FRU                           = 0x1A,
    SENSOR_TYPE_CODE_CABLE_INTERCONNECT                  = 0x1B,
    SENSOR_TYPE_CODE_TERMINATOR                          = 0x1C,
    SENSOR_TYPE_CODE_SYSTEM_BOOT_RESTART_INITIATED       = 0x1D,
    SENSOR_TYPE_CODE_BOOT_ERROR                          = 0x1E,
    SENSOR_TYPE_CODE_BASE_OS_BOOT_INSTALLATION_STATUS    = 0x1F,
    SENSOR_TYPE_CODE_OS_STOP_SHUTDOWN                    = 0x20,
    SENSOR_TYPE_CODE_SLOT_CONNECTOR                      = 0x21,
    SENSOR_TYPE_CODE_SYSTEM_ACPI_POWER_STATE             = 0x22,
    SENSOR_TYPE_CODE_WATCHDOG_2                          = 0x23,
    SENSOR_TYPE_CODE_PLATFORM_ALERT                      = 0x24,
    SENSOR_TYPE_CODE_ENTITY_PRESENCE                     = 0x25,
    SENSOR_TYPE_CODE_MONITOR_ASIC_IC                     = 0x26,
    SENSOR_TYPE_CODE_LAN                                 = 0x27,
    SENSOR_TYPE_CODE_MANAGEMENT_SUBSYSTEM_HEALTH         = 0x28,
    SENSOR_TYPE_CODE_BATTERY                             = 0x29,
    SENSOR_TYPE_CODE_SESSION_AUDIT                       = 0x2A,
    SENSOR_TYPE_CODE_VERSION_CHANGE                      = 0x2B,
    SENSOR_TYPE_CODE_FRU_STATE                           = 0x2C,
    SENSOR_TYPE_CODE_ORM_RESERVE_BEGIN                   = 0xC0,
    SENSOR_TYPE_CODE_ORM_RESERVE_END                     = 0xFF,
};

/**
 * Sensor Type Codes and Data
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 43.17
 */
enum sensor_unit_code {
    SENSOR_UNIT_TYPE_CODE_UNSPECIFIED          = 0,
    SENSOR_UNIT_TYPE_CODE_DEGREES_C            = 1,
    SENSOR_UNIT_TYPE_CODE_DEGREES_F            = 2,
    SENSOR_UNIT_TYPE_CODE_DEGREES_K            = 3,
    SENSOR_UNIT_TYPE_CODE_VOLTS                = 4,
    SENSOR_UNIT_TYPE_CODE_AMPS                 = 5,
    SENSOR_UNIT_TYPE_CODE_WATTS                = 6,
    SENSOR_UNIT_TYPE_CODE_JOULES               = 7,
    SENSOR_UNIT_TYPE_CODE_COULOMBS             = 8,
    SENSOR_UNIT_TYPE_CODE_VA                   = 9,
    SENSOR_UNIT_TYPE_CODE_NITS                 = 10,
    SENSOR_UNIT_TYPE_CODE_LUMEN                = 11,
    SENSOR_UNIT_TYPE_CODE_LUX                  = 12,
    SENSOR_UNIT_TYPE_CODE_CANDELA              = 13,
    SENSOR_UNIT_TYPE_CODE_KPA                  = 14,
    SENSOR_UNIT_TYPE_CODE_PSI                  = 15,
    SENSOR_UNIT_TYPE_CODE_NEWTON               = 16,
    SENSOR_UNIT_TYPE_CODE_CFM                  = 17,
    SENSOR_UNIT_TYPE_CODE_RPM                  = 18,
    SENSOR_UNIT_TYPE_CODE_HZ                   = 19,
    SENSOR_UNIT_TYPE_CODE_MICROSECOND          = 20,
    SENSOR_UNIT_TYPE_CODE_MILLISECOND          = 21,
    SENSOR_UNIT_TYPE_CODE_SECOND               = 22,
    SENSOR_UNIT_TYPE_CODE_MINUTE               = 23,
    SENSOR_UNIT_TYPE_CODE_HOUR                 = 24,
    SENSOR_UNIT_TYPE_CODE_DAY                  = 25,
    SENSOR_UNIT_TYPE_CODE_WEEK                 = 26,
    SENSOR_UNIT_TYPE_CODE_MIL                  = 27,
    SENSOR_UNIT_TYPE_CODE_INCHES               = 28,
    SENSOR_UNIT_TYPE_CODE_FEET                 = 29,
    SENSOR_UNIT_TYPE_CODE_CU_IN                = 30,
    SENSOR_UNIT_TYPE_CODE_CU_FEET              = 31,
    SENSOR_UNIT_TYPE_CODE_MM                   = 32,
    SENSOR_UNIT_TYPE_CODE_CM                   = 33,
    SENSOR_UNIT_TYPE_CODE_M                    = 34,
    SENSOR_UNIT_TYPE_CODE_CU_CM                = 35,
    SENSOR_UNIT_TYPE_CODE_CU_M                 = 36,
    SENSOR_UNIT_TYPE_CODE_LITERS               = 37,
    SENSOR_UNIT_TYPE_CODE_FLUID_OUNCE          = 38,
    SENSOR_UNIT_TYPE_CODE_RADIANS              = 39,
    SENSOR_UNIT_TYPE_CODE_STERADIANS           = 40,
    SENSOR_UNIT_TYPE_CODE_REVOLUTIONS          = 41,
    SENSOR_UNIT_TYPE_CODE_CYCLES               = 42,
    SENSOR_UNIT_TYPE_CODE_GRAVITIES            = 43,
    SENSOR_UNIT_TYPE_CODE_OUNCE                = 44,
    SENSOR_UNIT_TYPE_CODE_POUND                = 45,
    SENSOR_UNIT_TYPE_CODE_FT_LB                = 46,
    SENSOR_UNIT_TYPE_CODE_OZ_IN                = 47,
    SENSOR_UNIT_TYPE_CODE_GAUSS                = 48,
    SENSOR_UNIT_TYPE_CODE_GILBERTS             = 49,
    SENSOR_UNIT_TYPE_CODE_HENRY                = 50,
    SENSOR_UNIT_TYPE_CODE_MILLIHENRY           = 51,
    SENSOR_UNIT_TYPE_CODE_FARAD                = 52,
    SENSOR_UNIT_TYPE_CODE_MICROFARAD           = 53,
    SENSOR_UNIT_TYPE_CODE_OHMS                 = 54,
    SENSOR_UNIT_TYPE_CODE_SIEMENS              = 55,
    SENSOR_UNIT_TYPE_CODE_MOLE                 = 56,
    SENSOR_UNIT_TYPE_CODE_BECQUEREL            = 57,
    SENSOR_UNIT_TYPE_CODE_PPM                  = 58,
    SENSOR_UNIT_TYPE_CODE_RESERVED             = 59,
    SENSOR_UNIT_TYPE_CODE_DECIBELS             = 60,
    SENSOR_UNIT_TYPE_CODE_DBA                  = 61,
    SENSOR_UNIT_TYPE_CODE_DBC                  = 62,
    SENSOR_UNIT_TYPE_CODE_GRAY                 = 63,
    SENSOR_UNIT_TYPE_CODE_SIEVERT              = 64,
    SENSOR_UNIT_TYPE_CODE_COLOR_TEMP_DEG_K     = 65,
    SENSOR_UNIT_TYPE_CODE_BIT                  = 66,
    SENSOR_UNIT_TYPE_CODE_KILOBIT              = 67,
    SENSOR_UNIT_TYPE_CODE_MEGABIT              = 68,
    SENSOR_UNIT_TYPE_CODE_GIGABIT              = 69,
    SENSOR_UNIT_TYPE_CODE_BYTE                 = 70,
    SENSOR_UNIT_TYPE_CODE_KILOBYTE             = 71,
    SENSOR_UNIT_TYPE_CODE_MEGABYTE             = 72,
    SENSOR_UNIT_TYPE_CODE_GIGABYTE             = 73,
    SENSOR_UNIT_TYPE_CODE_WORD                 = 74,
    SENSOR_UNIT_TYPE_CODE_DWORD                = 75,
    SENSOR_UNIT_TYPE_CODE_QWORD                = 76,
    SENSOR_UNIT_TYPE_CODE_LINE                 = 77,
    SENSOR_UNIT_TYPE_CODE_HIT                  = 78,
    SENSOR_UNIT_TYPE_CODE_MISS                 = 79,
    SENSOR_UNIT_TYPE_CODE_RETRY                = 80,
    SENSOR_UNIT_TYPE_CODE_RESET                = 81,
    SENSOR_UNIT_TYPE_CODE_OVERRUN_OVERFLOW     = 82,
    SENSOR_UNIT_TYPE_CODE_UNDERRUN             = 83,
    SENSOR_UNIT_TYPE_CODE_COLLISION            = 84,
    SENSOR_UNIT_TYPE_CODE_PACKETS              = 85,
    SENSOR_UNIT_TYPE_CODE_MESSAGES             = 86,
    SENSOR_UNIT_TYPE_CODE_CHARACTERS           = 87,
    SENSOR_UNIT_TYPE_CODE_ERROR                = 88,
    SENSOR_UNIT_TYPE_CODE_CORRECTABLE_ERROR    = 89,
    SENSOR_UNIT_TYPE_CODE_UNCORRECTABLE_ERROR  = 90,
    SENSOR_UNIT_TYPE_CODE_FATAL_ERROR          = 91,
    SENSOR_UNIT_TYPE_CODE_GRAMS                = 92,
};

enum bmc_op_attributes {
    ATT_DELETE_DEVICE,
    ATT_MAX
};

enum bmc_table_type {
    TABLE_TYPE_SDR,
    TABLE_TYPE_SYSFS
};

/**
 * Get Device ID response
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 20.1
 */
struct dev_id_get_rsp{
    uint8_t completion;
    uint8_t dev_id;
    uint8_t dev_rev;
    uint8_t fw_rev1;
    uint8_t fw_rev2;
    uint8_t ipmi_ver;
    uint8_t add_dev_sup;
    uint8_t mf_id_0;
    uint8_t mf_id_1;
    uint8_t mf_id_2;
    uint8_t product_id_0;
    uint8_t product_id_1;
    uint8_t aux_fw_ver_3;
    uint8_t aux_fw_ver_2;
    uint8_t aux_fw_ver_1;
    uint8_t aux_fw_ver_0;
} __packed;

/**
 * Set Watchdog Timer request
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 27.6
 */
struct wdg_timer_set_req {
    uint8_t timer_use;
    uint8_t timer_actions;
    uint8_t pre_timeout;
    uint8_t time_use_expir;
    uint8_t init_lsb;
    uint8_t init_msb;
};

/**
 * Get Watchdog Timer response
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 27.7
 */
struct wdg_timer_get_rsp{
    uint8_t completion;
    uint8_t timer_use;
    uint8_t timer_actions;
    uint8_t pre_timeout;
    uint8_t time_use_expir;
    uint8_t init_lsb;
    uint8_t init_msb;
    uint8_t countdown_lsb;
    uint8_t countdown_msb;
} __packed;

/**
 * Reserve SDR Repository response
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 33.11
 */
struct reserve_sdr_repo_rsp{
    uint8_t completion;
    uint8_t reserve_id_lsb;
    uint8_t reserve_id_msb;
} __packed;

/**
 * Get SDR request
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 33.12
 */
struct sdr_get_req {
    unsigned char reserve_id_lsb;
    unsigned char reserve_id_msb;
    unsigned char record_id_lsb;
    unsigned char record_id_msb;
    int offset;
    int count;
};

struct sdr_get_rsp{
    uint8_t completion;
    uint8_t next_id_lsb;
    uint8_t next_id_msb;
} __packed;

/**
 * Get FRU inventory area info response
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 34.1
 */
struct fru_inv_area_info_rsp {
    uint8_t completion;
    uint8_t area_size_lsb;
    uint8_t area_size_msb;
    uint8_t reserve;
}__packed;

/**
 * Read FRU Data request
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 34.2
 */
struct fru_data_read_req {
    unsigned char fru_dev_id;
    unsigned char offset_lsb;
    unsigned char offset_msb;
    unsigned char count;
};

/**
 * Read FRU Data response
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 34.2
 */
struct fru_data_read_rsp {
    uint8_t completion;
    uint8_t count;
}__packed;

/**
 * Get Sensor Thresholds response
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 35.9
 */
struct sensor_thresholds_get_rsp{
    uint8_t completion;
    uint8_t mask;
    uint8_t lower_non_critical;
    uint8_t lower_critical;
    uint8_t lower_non_recoverable;
    uint8_t upper_non_critical;
    uint8_t upper_critical;
    uint8_t upper_non_recoverable;
} __packed;

/**
 * Get Sensor Reading response
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 35.14
 */
struct sensor_reading_get_rsp{
    uint8_t completion;
    uint8_t sensor_reading;
    uint8_t state;
    uint8_t data1;
    uint8_t data2;
} __packed;

/**
 * Get Sensor type response
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 35.16
 */
struct sensor_type_get_rsp{
    uint8_t completion;
    uint8_t sensor_type;
    uint8_t evt_reading_type_code;
} __packed;

/**
 * Get SDR header
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 43
 */
struct sdr_hdr{
    uint8_t record_id_lsb;
    uint8_t record_id_msb;
    uint8_t sdr_version;
    uint8_t record_type;
    uint8_t record_length;
} __packed;

struct sdr_get_hdr_rsp{
    struct sdr_get_rsp cmd_hdr;
    struct sdr_hdr data_hdr;
} __packed;

/**
 * Get SDR full type data
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 43.1
 */
struct sdr_full_get_rsp{
    struct sdr_hdr hdr;
    uint8_t owner_id;
    uint8_t owner_lun;
    uint8_t sensor_number;
    uint8_t entity_id;
    uint8_t entity_instance;
    uint8_t initialization;
    uint8_t sensor_cap;
    uint8_t sensor_type;
    uint8_t evt_type_code;
    uint8_t assert_mask_lsb;
    uint8_t assert_mask_msb;
    uint8_t deassert_mask_lsb;
    uint8_t deassert_mask_msb;
    uint8_t discrete_mask_lsb;
    uint8_t discrete_mask_msb;
    uint8_t sensor_units1;
    uint8_t sensor_units2;
    uint8_t sensor_units3;
    uint8_t linearization;
    uint8_t m_lsb;
    uint8_t m_msb;
    uint8_t b_lsb;
    uint8_t b_msb;
    uint8_t accuracy;
    uint8_t r_b_exp;
    uint8_t analog_char_flags;
    uint8_t nominal_reading;
    uint8_t nominal_max;
    uint8_t nominal_min;
    uint8_t sensor_max_reading;
    uint8_t sensor_min_reading;
    uint8_t unr_thresh;
    uint8_t uc_thresh;
    uint8_t unc_thresh;
    uint8_t lnr_thresh;
    uint8_t lc_thresh;
    uint8_t lnc_thresh;
    uint8_t positive_going_thresh;
    uint8_t negative_going_thresh;
    uint8_t reserve0;
    uint8_t reserve1;
    uint8_t oem;
    uint8_t id_tlv;
    uint8_t id_str[IPMI_SDR_ID_MAX_LEN];
} __packed;

/**
 * Get SDR compact type data
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 43.2
 */
struct sdr_compact_get_rsp{
    struct sdr_hdr hdr;
    uint8_t owner_id;
    uint8_t owner_lun;
    uint8_t sensor_number;
    uint8_t entity_id;
    uint8_t entity_instance;
    uint8_t initialization;
    uint8_t sensor_cap;
    uint8_t sensor_type;
    uint8_t evt_type_code;
    uint8_t assert_mask_lsb;
    uint8_t assert_mask_msb;
    uint8_t deassert_mask_lsb;
    uint8_t deassert_mask_msb;
    uint8_t discrete_mask_lsb;
    uint8_t discrete_mask_msb;
    uint8_t sensor_units1;
    uint8_t sensor_units2;
    uint8_t sensor_units3;
    uint8_t sensor_record_sharing_lsb;
    uint8_t sensor_record_sharing_msb;
    uint8_t positive_going_thresh;
    uint8_t negative_going_thresh;
    uint8_t reserve0;
    uint8_t reserve1;
    uint8_t reserve2;
    uint8_t oem;
    uint8_t id_tlv;
    uint8_t id_str[IPMI_SDR_ID_MAX_LEN];
} __packed;

/**
 * Get SDR fru type data
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 43.8
 */
struct sdr_fru_dev_locator_get_rsp{
    struct sdr_hdr hdr;
    uint8_t dev_access_addr;
    uint8_t fru_dev_id;
    uint8_t logical_physical;
    uint8_t char_num;
    uint8_t reserve0;
    uint8_t dev_type;
    uint8_t dev_type_mod;
    uint8_t fru_eid;
    uint8_t fru_eid_instance;
    uint8_t oem;
    uint8_t id_tlv;
    uint8_t id_str[IPMI_SDR_ID_MAX_LEN];
} __packed;

/**
 * Read FRU Data response
 * ref:
 * platform-management-fru-document-rev-1-2, CH 8
 */
struct fru_common_hdr_rsp {
    struct fru_data_read_rsp hdr;
    uint8_t version;
    uint8_t internal_use_off;
    uint8_t chassis_info_off;
    uint8_t board_off;
    uint8_t product_info_off;
    uint8_t multi_record_off;
    uint8_t pad;
    uint8_t checksum;
}__packed;

/**
 * Read FRU Data response
 * ref:
 * platform-management-fru-document-rev-1-2, CH 12
 */
struct fru_platfrom_info_hdr_rsp {
    struct fru_data_read_rsp hdr;
    uint8_t version;
    uint8_t length;
    uint8_t code;
}__packed;

struct bmc_dev_attr_node{
    uint8_t type;
    uint8_t sub_type;
    uint8_t id;
    uint8_t sub_id;
    bool is_probe;
    char *name;
    umode_t mode;
    uint8_t *udf_cmd;
    uint8_t cmd_len;
    unsigned long sensor_unit;
};

struct bmc_dev_attr{
    struct list_head list;
    struct device_attribute dev_attr;
    uint8_t type;
    uint8_t sub_type;
    uint8_t id;
    uint8_t sub_id;
    uint8_t valid;
    unsigned long last_updated;
    bool is_probe;
    char name[SYSFS_NAME_MAX_LEN + 1];
    uint8_t cache[IPMI_CACHE_BUF_MAX_SIZE];
    uint8_t udf_cmd[IPMI_OPENIPMI_MAX_REQ_SIZE];
    uint8_t cmd_len;
};

struct ipmi_sdr_list{
    struct list_head list;
    uint8_t sdr_type;
    uint8_t id;
    unsigned long sensor_unit;
    char name[IPMI_SDR_ID_MAX_LEN + 1];
    uint8_t sdr_data[IPMI_SDR_FULL_SIZE];
};

struct ipmi_data {
    struct completion read_complete;
    struct ipmi_addr address;
    int interface;
    struct ipmi_user_hndl ipmi_hndlrs;
    struct ipmi_user *user;

    unsigned char *rsp_msg_buf;
    unsigned short rsp_msg_buf_size;
    unsigned short rsp_size;
    unsigned char rsp_result;
    int rsp_type;
    long req_msgid;
    bool is_init_intf;

};

struct bmc_data_s {
    struct mutex access_lock;
    struct ipmi_data ipmi;
    struct bmc_dev_attr bmc_attr_lists;
    struct ipmi_sdr_list ipmi_sdr_lists;
    struct attribute_group grp;
    enum bmc_table_type dump_table_type;
    atomic_long_t user_count;
};

static int lower_str_copy(char *dstr, char *sstr, ssize_t len);
static int ipow(int base, int exp);
static int ipmi_send_message(struct device *dev,
    unsigned char  netfn, unsigned char cmd,
    unsigned char *req_buf, unsigned short rq_buf_size,
    unsigned char *rsp_buf, unsigned short rsp_buf_size);
static void ipmi_msg_handler(struct ipmi_recv_msg *msg, void *user_data);
static int ipmi_intf_init(struct device *dev);
static int _sdr_node_add(struct device *dev,
    struct reserve_sdr_repo_rsp *reserve_sdr,
    uint8_t *record_id_lsb, uint8_t *record_id_msb);
static int ipmi_sensor_probe(struct device *dev);
static int ipmi_default_probe(struct device *dev);
static int ipmi_udf_cmd(struct device *dev, uint8_t *udf_cmd, uint8_t cmd_len,
    unsigned char *rsp_buf, unsigned short rsp_buf_size);
static int ipmi_oem_ufi_psu_reset_cmd(struct device *dev, unsigned char time,
    unsigned char *rsp_buf, unsigned short rsp_buf_size);
static int ipmi_get_device_id_cmd(struct device *dev, unsigned char *rsp_buf,
    unsigned short rsp_buf_size);
static int ipmi_reset_watchdog_timer_cmd(struct device *dev,
    unsigned char *rsp_buf, unsigned short rsp_buf_size);
static int ipmi_set_watchdog_timer_cmd(struct device *dev,
    struct wdg_timer_set_req *req,
    unsigned char *rsp_buf, unsigned short rsp_buf_size);
static int ipmi_get_watchdog_timer_cmd(struct device *dev,
    unsigned char *rsp_buf, unsigned short rsp_buf_size);
static int ipmi_chassis_control_cmd(struct device *dev, unsigned char control,
    unsigned char *rsp_buf, unsigned short rsp_buf_size);
static int ipmi_reserve_sdr_repo_cmd(struct device *dev,
    unsigned char *rsp_buf, unsigned short rsp_buf_size);
static int ipmi_get_sdr_cmd(struct device *dev, struct sdr_get_req *req,
    unsigned char *rsp_buf, unsigned short rsp_buf_size);
static int ipmi_get_fru_inv_area_info_cmd(struct device *dev,
    unsigned char fru_dev_id,
    unsigned char *rsp_buf, unsigned short rsp_buf_size);
static int ipmi_read_fru_data_cmd(struct device *dev,
    struct fru_data_read_req *req,
    unsigned char *rsp_buf, unsigned short rsp_buf_size);
static int ipmi_get_sensor_thresholds_cmd(struct device *dev,
    unsigned char sensor_number,
    unsigned char *rsp_buf, unsigned short rsp_buf_size);
static int ipmi_get_sensor_reading_cmd(struct device *dev,
    unsigned char sensor_number,
    unsigned char *rsp_buf, unsigned short rsp_buf_size);
static int ipmi_get_sensor_type_cmd(struct device *dev,
    unsigned char sensor_number,
    unsigned char *rsp_buf, unsigned short rsp_buf_size);
static int _sdr_data_get(struct list_head *sdr_list,
    uint8_t sdr_type, uint8_t id, uint8_t **data);
static int _sysfs_create(struct device *dev, struct list_head *list,
    uint8_t sub_id, struct bmc_dev_attr_node *node, char *suffix_name,
    umode_t mode,
    ssize_t (*show)(struct device *, struct device_attribute *,char *),
    ssize_t (*store)(struct device *, struct device_attribute *,
        const char *, size_t));
static int _sysfs_full_sensors_create(struct device *dev,
    struct bmc_dev_attr_node *node);
static int _sysfs_compact_sensors_create(struct device *dev,
    struct bmc_dev_attr_node *node);
static int _sysfs_fru_create(struct device *dev,
    struct bmc_dev_attr_node *node);
static int _sysfs_default_create(struct device *dev,
    struct bmc_dev_attr_node *node);
static int _sysfs_udf_create(struct device *dev,
    struct bmc_dev_attr_node *node);
static int sysfs_create(struct device *dev, struct bmc_dev_attr_node *node);
static int64_t _adjust_result_temp_min(int64_t result);
static int _analog_parsing(struct device *dev,
    char *buf, uint8_t *sdr_data, uint8_t *cache,
    int64_t (*adjust_result)(int64_t result));
static int _discete_parsing(struct device *dev,
    char *buf, uint8_t *sdr_data, uint8_t *cache);
static int _sensor_type_parsing(struct device *dev,
    char *buf, uint8_t *cache);
static int _sensor_unit_parsing(struct device *dev,
    char *buf, uint8_t *cache);
static ssize_t _sensor_update(struct device *dev,
    uint8_t sdr_type, uint8_t id, uint8_t sub_id);
static ssize_t class_sensor_show(struct device *dev,
    struct device_attribute *da, char *buf);
static int _fru_common_header_get(struct device * dev, uint8_t id,
    unsigned char *rsp_buf, int rsp_buf_size);
static int _fru_product_info_header_get(struct device * dev, uint8_t id,
    unsigned char *rsp_buf, int rsp_buf_size, int product_off);
static int _fru_eeprom_size_get(struct device * dev, uint8_t id);
static int _fru_product_info_data_get(struct device * dev, uint8_t id,
    unsigned char *eeprom_data, int eeprom_size, int *eeprom_len,
    int product_off, int area_data_size);
static ssize_t _fru_update(struct device *dev, uint8_t id);
static ssize_t class_fru_show(struct device *dev,
    struct device_attribute *da, char *buf);
static ssize_t _firmware_version_update(struct device *dev);
static ssize_t class_firmware_version_show(struct device *dev,
    struct device_attribute *da, char *buf);
static ssize_t _watchdog_update(struct device *dev,
    uint8_t id, uint8_t sub_id);
static ssize_t class_watchdog_show(struct device *dev,
    struct device_attribute *da, char *buf);
static ssize_t class_watchdog_store(struct device *dev,
    struct device_attribute *da, const char *buf, size_t count);
static ssize_t class_power_cycle_store(struct device *dev,
    struct device_attribute *da, const char *buf, size_t count);
static ssize_t class_oem_ufi_psu_reset_store(struct device *dev,
    struct device_attribute *da, const char *buf, size_t count);
static int fru_info_get(struct device *dev, uint8_t id,
        struct fru_procd_info *rsp);
static ssize_t class_udf_show(struct device *dev,
    struct device_attribute *da, char *buf);
static ssize_t class_udf_store(struct device *dev,
    struct device_attribute *da, const char *buf, size_t count);
static void _dump_to_block_buffer(uint8_t *buf, unsigned int len,
    char *blockbuf, size_t blockbuflen);
static ssize_t _sdr_dump(struct device *dev,
    char *buf, loff_t off, size_t count);
static ssize_t _sysfs_dump(struct device *dev,
    char *buf, loff_t off, size_t count);
static ssize_t dump_table_read(struct file *filp, struct kobject *kobj,
    struct bin_attribute *attr, char *buf, loff_t off, size_t count);
static ssize_t dump_table_write(struct file *filp, struct kobject *kobj,
    struct bin_attribute *attr, char *buf, loff_t off, size_t count);
static ssize_t user_count_show(struct device *dev,
    struct device_attribute *da, char *buf);
static int bmc_drv_probe(struct platform_device *pdev);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
static int bmc_drv_remove(struct platform_device *pdev);
#else
static void bmc_drv_remove(struct platform_device *pdev);
#endif
int ufi_bmc_udf_create(struct device * dev, struct bmc_udf_node *table);
void ufi_bmc_udf_destroy(struct device * dev);
int bmc_init(void);
void bmc_exit(void);

static DEVICE_ATTR_RO(user_count);
static BIN_ATTR_RW(dump_table, DUMP_TABLE_MAX_SIZE);

struct udf_show_store ufi_bmc_show_store_udf[UDF_ID_UDF_MAX] = {0};
EXPORT_SYMBOL(ufi_bmc_show_store_udf);

static int lower_str_copy(char *dstr, char *sstr, ssize_t len)
{
    if(!dstr || !sstr) {
        return -EINVAL;
    }
    memcpy(dstr, sstr, len);
    while (*dstr) {
        *dstr = tolower(*dstr);
        dstr++;
    }
    return 0;
}

static inline void strim_inplace(char *buf)
{
    char *p = strim(buf);
    if (p != buf)
        memmove(buf, p, strlen(p) + 1);
}

static int ipow(int base, int exp)
{
    int result = 1;
    for (;;)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return result;
}

/* Send an IPMI command */
static int ipmi_send_message(struct device *dev,
    unsigned char  netfn, unsigned char cmd,
    unsigned char *req_buf, unsigned short req_buf_size,
    unsigned char *rsp_buf, unsigned short rsp_buf_size)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    int retry = 0;
    int rv;
    struct kernel_ipmi_msg req = {
        .netfn = netfn,
        .cmd = cmd,
        .data =req_buf,
        .data_len = req_buf_size
    };

    if(!bmc_data) {
        return -EINVAL;
    }

    bmc_data->ipmi.rsp_msg_buf = rsp_buf;
    bmc_data->ipmi.rsp_msg_buf_size = rsp_buf_size;
    reinit_completion(&bmc_data->ipmi.read_complete);

    if(!bmc_data->ipmi.is_init_intf) {
        rv = ipmi_intf_init(dev);

        if (rv) {
            dev_dbg(dev, "IPMI setup intf fail(%d)\n", rv);
            return -EIO;
        }
    }

    rv = ipmi_validate_addr(&bmc_data->ipmi.address,
            sizeof(bmc_data->ipmi.address));
    if (rv) {
        dev_dbg(dev, "IPMI address validate fail(%d)", rv);
        goto free_user;
    }

    dev_dbg(dev, "Dump request message netfun(0x%02X) cmd(0x%02X) len(%d)\n",
        req.netfn, req.cmd, req.data_len);
    print_hex_dump_debug("    ", DUMP_PREFIX_OFFSET, 16, 1,
        req.data, req.data_len, false);

    // Make it high priority
    rv = ipmi_request_settime(bmc_data->ipmi.user, &bmc_data->ipmi.address,
            bmc_data->ipmi.req_msgid, &req, &bmc_data->ipmi, 1, 0, 0);
    if (rv) {
        dev_dbg(dev, "IPMI request settime fail(%d)\n", rv);
        goto free_user;
    }

    for (retry = 0; retry <= IPMI_ERR_RETRY_TIMES; retry++) {
        rv = wait_for_completion_timeout(&bmc_data->ipmi.read_complete,
                IPMI_TIMEOUT * (retry+1));
        if (!rv) {
            if(retry == 0) {
                dev_dbg(dev, "IPMI request timeout\n");
            } else {
                dev_dbg(dev, "IPMI request timeout retry(%d)\n", retry);
            }
            continue;
        } else {
            break;
        }
    }

    if (!rv) {
        ufi_bmc_pr_err(dev, "IPMI request timeout and retry(%d) fail\n",
            (IPMI_ERR_RETRY_TIMES));

        ufi_bmc_pr_err(dev, "Dump request message "
            "netfun (0x%02X) cmd(0x%02X) len(%d)\n",
            req.netfn, req.cmd, req.data_len);
        print_hex_dump(KERN_ERR, "    ", DUMP_PREFIX_OFFSET, 16, 1,
            req.data, req.data_len, false);

        goto free_user;
    }

    if(bmc_data->ipmi.rsp_result != 0) {
        if(bmc_data->ipmi.rsp_result ==
            IPMI_COMPLETION_CODE_RESERVATION_FAIL) {
            dev_dbg(dev, "IPMI invalid reservation id\n");
            goto reserve_id_fail;
        } else {
            dev_dbg(dev, "IPMI request result fail(%d)\n",
                bmc_data->ipmi.rsp_result);
            goto fail;
        }
    }

    dev_dbg(dev, "IPMI request success len(%d)\n", bmc_data->ipmi.rsp_size);
    return bmc_data->ipmi.rsp_size;

free_user:
    ipmi_destroy_user(bmc_data->ipmi.user);
    bmc_data->ipmi.is_init_intf = false;
    atomic_long_dec(&bmc_data->user_count);

fail:
    return -EIO;
reserve_id_fail:
    return -EAGAIN;
}

static void ipmi_msg_handler(struct ipmi_recv_msg *msg, void *user_data)
{
    unsigned short rsp_len = {0};
    struct ipmi_data *ipmi = user_data;

    if (msg->msgid != ipmi->req_msgid) {
        pr_err("%s %s(#%d): Mismatch between received msgid (%d) "
            "and transmitted msgid (%d)!\n",
            DRVNAME, __func__, __LINE__,
            (int)msg->msgid,
            (int)ipmi->req_msgid);
        ipmi_free_recv_msg(msg);
        return;
    }

    ipmi->rsp_type = msg->recv_type;
    if (msg->msg.data_len > 0) {
        ipmi->rsp_result = msg->msg.data[0];
    } else {
        ipmi->rsp_result = IPMI_UNKNOWN_ERR_COMPLETION_CODE;
    }

    rsp_len = min(msg->msg.data_len, ipmi->rsp_msg_buf_size);

    pr_debug("Dump response message(len %d)\n", rsp_len);
    print_hex_dump_debug("    ", DUMP_PREFIX_OFFSET, 16, 1,
        msg->msg.data, rsp_len, false);

    if(!ipmi->rsp_msg_buf) {
        pr_err("%s %s(#%d): Response buffer is null pointer!!\n",
            DRVNAME, __func__, __LINE__);
        return;
    }

    memcpy(ipmi->rsp_msg_buf, msg->msg.data, rsp_len);
    ipmi->rsp_size = rsp_len;
    ipmi_free_recv_msg(msg);
    complete(&ipmi->read_complete);
}

static int ipmi_intf_init(struct device *dev)
{
    /**
     * Channel Medium Type, KCS, SMIC, or BT is belonging to System Interface
     * ref:
     * ipmi-second-gen-interface-spec-v2-rev1-1, CH 6.5
     */
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    int rv;

    if(!bmc_data) {
        return -EINVAL;
    }

    bmc_data->ipmi.address.addr_type = IPMI_SYSTEM_INTERFACE_ADDR_TYPE;
    bmc_data->ipmi.address.channel = IPMI_BMC_CHANNEL;
    bmc_data->ipmi.interface = 0;
    bmc_data->ipmi.ipmi_hndlrs.ipmi_recv_hndl = ipmi_msg_handler;

    rv = ipmi_create_user(bmc_data->ipmi.interface,
            &bmc_data->ipmi.ipmi_hndlrs, &bmc_data->ipmi,
            &bmc_data->ipmi.user);
    if (rv < 0) {
        ufi_bmc_pr_err(dev, "Unable to register user with IPMI interface %d\n",
            bmc_data->ipmi.interface);
        bmc_data->ipmi.is_init_intf = false;
        return -EACCES;
    }
    bmc_data->ipmi.is_init_intf = true;
    atomic_long_inc(&bmc_data->user_count);

    // We enable maintenance mode to improve efficiency
    ipmi_set_maintenance_mode(bmc_data->ipmi.user, IPMI_MAINTENANCE_MODE_ON);
    return 0;
}

static int _sdr_node_add(struct device *dev,
    struct reserve_sdr_repo_rsp *reserve_sdr,
    uint8_t *record_id_lsb, uint8_t *record_id_msb)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    int rv = 0;
    unsigned char rsp_buf[IPMI_OPENIPMI_MAX_RSP_DATA_SIZE] = {0};
    int total = 0;
    int off = 0;
    int count = 0;
    int block_num = 0;
    int retry = 0;
    struct ipmi_sdr_list *sdr_node = NULL;
    struct ipmi_sdr_list *tmp_sdr_node = NULL;
    struct sdr_get_req req = {0};
    struct sdr_get_hdr_rsp sdr_get_hdr = {0};
    struct sdr_hdr *sdr_hdr = NULL;

    if(!record_id_lsb || !record_id_msb || !reserve_sdr || !bmc_data) {
        return -EINVAL;
    }
    /**
     * ipmi-second-gen-interface-spec-v2-rev1-1, CH 33.12.
     * Even though a reservation ID isn't strictly required from offset 0,
     * we still retrieve and initialize it here.
     * Also, the reservation ID seems to expire roughly every minute.
     */
    if(reserve_sdr->reserve_id_msb == 0 && reserve_sdr->reserve_id_lsb == 0) {
        rv = ipmi_reserve_sdr_repo_cmd(dev, rsp_buf, sizeof(rsp_buf));
        if(rv < 0) {
            ufi_bmc_pr_err(dev, "Reserve SDR Repository Command fail rv(%d)\n",
                 rv);
        }
        *reserve_sdr = *((struct reserve_sdr_repo_rsp *) rsp_buf);
    }

    sdr_node = devm_kzalloc(dev, sizeof(struct ipmi_sdr_list), GFP_KERNEL);
    if (!sdr_node) {
        ufi_bmc_pr_warn(dev, "Failed to allocate memory for sdr_node.\n");
        rv = -ENOMEM;
        goto free_all_nodes;
    }

    off = 0;
    count = sizeof(struct sdr_hdr);
    memset(rsp_buf, 0,IPMI_OPENIPMI_MAX_RSP_DATA_SIZE);
    req.reserve_id_lsb = reserve_sdr->reserve_id_lsb;
    req.reserve_id_msb = reserve_sdr->reserve_id_msb;
    req.record_id_lsb = *record_id_lsb;
    req.record_id_msb = *record_id_msb;
    req.offset = off;
    req.count = count;

    rv = ipmi_get_sdr_cmd(dev, &req, rsp_buf, sizeof(rsp_buf));

    if(rv < 0) {
        ufi_bmc_pr_err(dev, "Get header fail record id(0x%02X%02X)\n",
            *record_id_msb, *record_id_lsb);
        goto free_all_nodes;
    }

    sdr_get_hdr = *((struct sdr_get_hdr_rsp *) rsp_buf);

    *record_id_lsb = sdr_get_hdr.cmd_hdr.next_id_lsb;
    *record_id_msb = sdr_get_hdr.cmd_hdr.next_id_msb;

    // Skip completion code and next record id on response data
    memcpy(sdr_node->sdr_data + off, rsp_buf + sizeof(struct sdr_get_rsp),
        count);
    sdr_hdr = (struct sdr_hdr *) sdr_node->sdr_data;

    dev_dbg(dev, "SDR header reserve_id(0x%02X%02X) record_id(0x%02X%02X) "
        "record_type(0x%02x)\n",
        reserve_sdr->reserve_id_msb, reserve_sdr->reserve_id_lsb,
        sdr_hdr->record_id_msb, sdr_hdr->record_id_lsb, sdr_hdr->record_type);

    if(sdr_hdr->sdr_version != IPMI_SDR_VERSION) {
        dev_dbg(dev, "Only support IPMI version 2.0, ignore it. "
            "Record ID(0x%02X%02X) version(0x%02X)\n",
            sdr_hdr->record_id_msb, sdr_hdr->record_id_lsb,
            sdr_hdr->sdr_version);
        goto free_node;
    }

    if(sdr_hdr->record_type != SDR_RECORD_TYPE_FULL_SENSOR &&
        sdr_hdr->record_type != SDR_RECORD_TYPE_COMPACT_SENSOR &&
        sdr_hdr->record_type != SDR_RECORD_TYPE_FRU_DEVICE_LOCATOR) {
        dev_dbg(dev, "Only support Full(type 1h), Compact(type 2h) or "
            "FRU Device Locator(type 11h), "
            "ignore it. Record ID(0x%02X%02X) type(0x%02X)\n",
            sdr_hdr->record_id_msb, sdr_hdr->record_id_lsb,
            sdr_hdr->record_type);
        goto free_node;
    }

    total = (sdr_hdr->record_length + sizeof(struct sdr_hdr));
    off += count;
    while(off < total) {
        int remain = total - off;
        count = min(remain, IPMI_OPENIPMI_SDR_MAX_RSP_DATA_SIZE);
        memset(rsp_buf, 0,IPMI_OPENIPMI_MAX_RSP_DATA_SIZE);
        req.reserve_id_lsb = reserve_sdr->reserve_id_lsb;
        req.reserve_id_msb = reserve_sdr->reserve_id_msb;
        req.record_id_lsb = sdr_hdr->record_id_lsb;
        req.record_id_msb = sdr_hdr->record_id_msb;
        req.offset = off;
        req.count = count;

        rv = ipmi_get_sdr_cmd(dev, &req, rsp_buf, sizeof(rsp_buf));

        if(rv < 0) {
            if(rv == -EAGAIN) {
                retry++;
                while(retry <= IPMI_ERR_RETRY_TIMES) {
                    dev_dbg(dev, "Get data block %d fail(reserve id fail) "
                        "rv(%d) retry(%d)\n", block_num, rv, retry);
                    memset(rsp_buf, 0,IPMI_OPENIPMI_MAX_RSP_DATA_SIZE);
                    rv = ipmi_reserve_sdr_repo_cmd(dev,
                        rsp_buf, sizeof(rsp_buf));

                    if(rv < 0) {
                        retry++;
                        msleep(CMD_RETRY_DELAY);
                        continue;
                    } else {
                        *reserve_sdr =
                            *((struct reserve_sdr_repo_rsp *) rsp_buf);
                        break;
                    }
                }

                if(retry > IPMI_ERR_RETRY_TIMES) {
                    dev_dbg(dev, "Get data block %d fail(reserve id fail) "
                        "rv(%d) and retry failure\n", block_num, rv);
                    goto free_all_nodes;
                } else {
                    dev_dbg(dev, "Reserve ID successful. "
                        "Getting data block %d again.\n",
                        block_num);
                    continue;
                }
            } else {
                dev_dbg(dev, "Get data block %d fail\n", block_num);
                goto free_all_nodes;
            }
        }
        retry = 0;
        // Skip completion code and next record id on response data
        memcpy(sdr_node->sdr_data + off, rsp_buf + sizeof(struct sdr_get_rsp),
            count);
        off += count;
        block_num++;
    }

    if(sdr_hdr->record_type == SDR_RECORD_TYPE_FULL_SENSOR) {
        struct sdr_full_get_rsp *data =
            (struct sdr_full_get_rsp *) sdr_node->sdr_data;
        int len = data->id_tlv & IPMI_SDR_ID_TLV_LEN_MASK;
        sdr_node->sdr_type = data->hdr.record_type;
        sdr_node->id = data->sensor_number;
        sdr_node->sensor_unit = data->sensor_units2;
        len = (len > IPMI_SDR_ID_MAX_LEN)? IPMI_SDR_ID_MAX_LEN:len;
        lower_str_copy(sdr_node->name, data->id_str, len);
    } else if(sdr_hdr->record_type == SDR_RECORD_TYPE_COMPACT_SENSOR) {
        struct sdr_compact_get_rsp *data =
            (struct sdr_compact_get_rsp *) sdr_node->sdr_data;
        int len = data->id_tlv & IPMI_SDR_ID_TLV_LEN_MASK;
        sdr_node->sdr_type = data->hdr.record_type;
        sdr_node->id = data->sensor_number;
        sdr_node->sensor_unit = data->sensor_units2;
        len = (len > IPMI_SDR_ID_MAX_LEN)? IPMI_SDR_ID_MAX_LEN:len;
        lower_str_copy(sdr_node->name, data->id_str, len);
    } else if(sdr_hdr->record_type == SDR_RECORD_TYPE_FRU_DEVICE_LOCATOR) {
        struct sdr_fru_dev_locator_get_rsp *data =
            (struct sdr_fru_dev_locator_get_rsp *) sdr_node->sdr_data;
        int len = data->id_tlv & IPMI_SDR_ID_TLV_LEN_MASK;
        if((data->logical_physical & IPMI_SDR_LOGICAL_PHYSICAL_MASK) &&
            data->fru_dev_id == 0) {
            /**
             * ipmi-second-gen-interface-spec-v2-rev1-1, CH 43.8,
             * SDR Type 11h - FRU Device Locator Record
             * The primary FRU device for a management controller is
             * always device #0 at LUN 00b. The primary FRU device is not
             * reported via this FRU Device Locator record - its presence is
             * identified via the Device Capabilities field in the
             * Management Controller Device Locator record.
             */
            goto free_node;
        }
        sdr_node->sdr_type = data->hdr.record_type;
        sdr_node->id = data->fru_dev_id;
        sdr_node->sensor_unit = SENSOR_UNIT_TYPE_CODE_UNSPECIFIED;
        len = (len > IPMI_SDR_ID_MAX_LEN)? IPMI_SDR_ID_MAX_LEN:len;
        lower_str_copy(sdr_node->name, data->id_str, len);
    }

    list_add_tail(&sdr_node->list, &bmc_data->ipmi_sdr_lists.list);
    dev_dbg(dev, "Probe sdr next_id 0x%02x%02x\n",
        *record_id_msb, *record_id_lsb);
    return 0;

free_node:
    devm_kfree(dev, sdr_node);
    return 0;
free_all_nodes:
    devm_kfree(dev, sdr_node);
    list_for_each_entry_safe(sdr_node, tmp_sdr_node,
        &bmc_data->ipmi_sdr_lists.list, list) {
        list_del(&sdr_node->list);
        devm_kfree(dev, sdr_node);
    }
    return -EIO;
}

static int ipmi_sensor_probe(struct device *dev)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    int rv;
    struct reserve_sdr_repo_rsp reserve_sdr = {0};
    uint8_t record_id_lsb = 0;
    uint8_t record_id_msb = 0;
    struct ipmi_sdr_list *sdr_node = NULL;

    if(!bmc_data) {
        return -EINVAL;
    }

    while(true) {
        rv = _sdr_node_add(dev,
                &reserve_sdr,
                &record_id_lsb, &record_id_msb);

        /**
         * IPMI Second-Gen Interface Spec v2 Rev 1.1, Chapter 33.12
         * record_id 0x0000 indicates the first record
         * record_id 0xFFFF indicates the last record
         */
        if(rv != 0) {
            break;
        } else if(record_id_lsb == 0xFF && record_id_msb == 0xFF) {
            break;
        } else if(record_id_lsb == 0x00 && record_id_msb == 0x00) {
            break;
        }
    }

    list_for_each_entry(sdr_node, &bmc_data->ipmi_sdr_lists.list, list) {
        struct bmc_dev_attr_node conf_node = {
            .type = ATTR_TYPE_SDR,
            .sub_type = sdr_node->sdr_type,
            .id = sdr_node->id,
            .is_probe = true,
            .name = sdr_node->name,
            .sensor_unit = sdr_node->sensor_unit
        };

        rv = sysfs_create(dev, &conf_node);
        if(rv < 0) {
            dev_dbg(dev, "Create sysfs fail type(%d), name(%s) "
                "sub_type(%d) id(0x%2x)\n",
                ATTR_TYPE_SDR, conf_node.name,
                conf_node.sub_type, conf_node.id);
            break;
        }
    }

    return rv;
}

static int ipmi_default_probe(struct device *dev)
{
    int rv;
    struct bmc_dev_attr_node conf_node = {
        .type = ATTR_TYPE_DEFAULT,
        .is_probe = true,
    };

    conf_node.id = DEFAULT_ID_FIREMWARE_VERSION;
    conf_node.name = "firmware_version";
    rv = sysfs_create(dev, &conf_node);
    if(rv < 0){
        goto done;
    }

    conf_node.id = DEFAULT_ID_FIREMWARE_VERSION_LONG;
    conf_node.name = "firmware_version_long";
    rv = sysfs_create(dev, &conf_node);
    if(rv < 0){
        goto done;
    }

    conf_node.id = DEFAULT_ID_WATCHDOG;
    conf_node.name = "watchdog";
    rv = sysfs_create(dev, &conf_node);
    if(rv < 0){
        goto done;
    }

    conf_node.id = DEFAULT_ID_POWER_CYCLE;
    conf_node.name = "power_cycle";
    rv = sysfs_create(dev, &conf_node);
    if(rv < 0){
        goto done;
    }

    conf_node.id = DEFAULT_ID_OEM_UFI_PSU_RESET;
    conf_node.name = "oem_ufi_psu_reset";
    rv = sysfs_create(dev, &conf_node);
    if(rv < 0){
        goto done;
    }

done:
    return rv;
}

/**
 * Ufispace User define funciton Command
 * ref:
 *     Requesut data
 *                   1 User define
 *                   2 User define
 *                   N User define
 *     Respond data
 *                   1 Completion Code
 *                   2 User define
 *                   N User define
 */
static int ipmi_udf_cmd(struct device *dev, uint8_t *udf_cmd, uint8_t cmd_len,
    unsigned char *rsp_buf, unsigned short rsp_buf_size)
{
    int rv;
    int i = 0;
    int j = 0;
    unsigned char req_buf[IPMI_OPENIPMI_MAX_REQ_DATA_SIZE] = {0};

    if(cmd_len > (IPMI_OPENIPMI_MAX_REQ_DATA_SIZE + IPMI_CMD_HEADER_SIZE)) {
        return -EINVAL;
    }

    for(i = IPMI_CMD_HEADER_SIZE, j=0; i<cmd_len;i++,j++) {
        req_buf[j] = udf_cmd[i];
    }

    rv = ipmi_send_message(dev, udf_cmd[0], udf_cmd[1],
            req_buf, j, rsp_buf, rsp_buf_size);
    return rv;
}

/**
 * Ufispace PSU reset Command
 * ref:
 *     Requesut data
 *                   1 trigger target
 *                   2 re-enable PSU time in seconds
 *     Respond data
 *                   1 Completion Code
 */
static int ipmi_oem_ufi_psu_reset_cmd(struct device *dev, unsigned char time,
    unsigned char *rsp_buf, unsigned short rsp_buf_size)
{
    int rv;
    unsigned char req_buf[IPMI_OPENIPMI_MAX_REQ_DATA_SIZE] = {
        0x2,
        time
    };

    rv = ipmi_send_message(dev, IPMI_NETFN_OEM_UFI, IPMI_CMD_OEM_UFI_PSU_RESET,
            req_buf, 2, rsp_buf, rsp_buf_size);
    return rv;
}

/**
 * Get Device ID Command
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 20.1
 *     Requesut data
 *                   -
 *     Respond data
 *                   1 Completion Code
 *                   2 Device ID
 *                   3 Device Revision
 *                   4 Firmware Revision 1
 *                   5 Firmware Revision 2
 *                   6 IPMI Version
 *                   7 Additional Device Support
 *                   8-10 Manufacturer ID
 *                   11-12 Product ID
 *                   13-16 Auxiliary Firmware Revision Information
 */
static int ipmi_get_device_id_cmd(struct device *dev, unsigned char *rsp_buf,
    unsigned short rsp_buf_size)
{
    int rv;
    unsigned char req_buf[IPMI_OPENIPMI_MAX_REQ_DATA_SIZE] = {0};

    rv = ipmi_send_message(dev, IPMI_NETFN_APP, IPMI_CMD_DEVICE_IDE_GET,
            req_buf, 0, rsp_buf, rsp_buf_size);
    return rv;
}

/**
 * Reset Watchdog Timer Command
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 27.5
 *     Requesut data
 *                   -
 *     Respond data
 *                   1 Completion Code
 */
static int ipmi_reset_watchdog_timer_cmd(struct device *dev,
    unsigned char *rsp_buf, unsigned short rsp_buf_size)
{
    int rv;
    unsigned char req_buf[IPMI_OPENIPMI_MAX_REQ_DATA_SIZE] = {0};

    rv = ipmi_send_message(dev, IPMI_NETFN_APP, IPMI_CMD_WATCHDOG_TIMER_RESET,
            req_buf, 0, rsp_buf, rsp_buf_size);
    return rv;
}

/**
 * Set Watchdog Timer Command
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 27.6
 *     Requesut data
 *                   1 Timer Use
 *                   2 Timer Actions
 *                   3 Pre-timeout interval in seconds
 *                   4 Timer Use Expiration flags
 *                   5 Initial countdown value, lsbyte
 *                   6 Initial countdown, msbyte
 *     Respond data
 *                   1 Completion Code
 */
static int ipmi_set_watchdog_timer_cmd(struct device *dev,
    struct wdg_timer_set_req *req,
    unsigned char *rsp_buf, unsigned short rsp_buf_size)
{
    int rv;
    unsigned char req_buf[IPMI_OPENIPMI_MAX_REQ_DATA_SIZE] = {
        req->timer_use,
        req->timer_actions,
        req->pre_timeout,
        req->time_use_expir,
        req->init_lsb,
        req->init_msb
    };

    rv = ipmi_send_message(dev, IPMI_NETFN_APP, IPMI_CMD_WATCHDOG_TIMER_SET,
            req_buf, 6, rsp_buf, rsp_buf_size);
    return rv;
}

/**
 * Get Watchdog Timer Command
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 27.7
 *     Requesut data
 *                   -
 *     Respond data
 *                   1 Completion Code
 *                   2 Timer Use
 *                   3 Timer Actions
 *                   4 Pre-timeout interval in seconds
 *                   5 Timer Use Expiration flags
 *                   6 Initial countdown value, lsbyte
 *                   7 Initial countdown, msbyte
 *                   8-Present countdown value, lsbyte.
 *                   9 Present countdown value, msbyte
 */
static int ipmi_get_watchdog_timer_cmd(struct device *dev,
    unsigned char *rsp_buf, unsigned short rsp_buf_size)
{
    int rv;
    unsigned char req_buf[IPMI_OPENIPMI_MAX_REQ_DATA_SIZE] = {0};

    rv = ipmi_send_message(dev, IPMI_NETFN_APP, IPMI_CMD_WATCHDOG_TIMER_GET,
            req_buf, 0, rsp_buf, rsp_buf_size);
    return rv;
}

/**
 * Chassis Control Command
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 28.3
 *     Requesut data
 *                   1 Control
 *     Respond data
 *                   1 Completion Code
 */
static int ipmi_chassis_control_cmd(struct device *dev, unsigned char control,
    unsigned char *rsp_buf, unsigned short rsp_buf_size)
{
    int rv;
    unsigned char req_buf[IPMI_OPENIPMI_MAX_REQ_DATA_SIZE] = {
        control
    };

    rv = ipmi_send_message(dev, IPMI_NETFN_CHASSIS, IPMI_CMD_CHASSIS_CONTROL,
            req_buf, 1, rsp_buf, rsp_buf_size);
    return rv;
}

/**
 * Reserve SDR Repository Command
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 33.11
 *     Requesut data
 *                   -
 *     Respond data
 *                   1 Completion Code
 *                   2 Reservation ID, LS Byte
 *                   3 Reservation ID, LS Byte
 */
static int ipmi_reserve_sdr_repo_cmd(struct device *dev,
    unsigned char *rsp_buf, unsigned short rsp_buf_size)
{
    int rv;
    unsigned char req_buf[IPMI_OPENIPMI_MAX_REQ_DATA_SIZE] = {0};

    rv = ipmi_send_message(dev, IPMI_NETFN_STORAGE, IPMI_CMD_RESERVE_SDR_REPO,
            req_buf, 0, rsp_buf, rsp_buf_size);
    return rv;
}

/**
 * Get SDR Command
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 33.12
 *     Requesut data
 *                   1 Reservation ID. LS Byte.
 *                   2 Reservation ID. MS Byte.
 *                   3 Record ID of record to Get, LS Byte
 *                   4 Record ID of record to Get, MS Byte
 *                   5 Offset into record
 *                   6 Bytes to read
 *     Respond data
 *                   1 Completion Code
 *                   2 Record ID for next record, LS Byte
 *                   3 RRecord ID for next record, MS Byte
 *                   4 Record Data
 */
static int ipmi_get_sdr_cmd(struct device *dev, struct sdr_get_req *req,
    unsigned char *rsp_buf, unsigned short rsp_buf_size)
{
    int rv;
    unsigned char req_buf[IPMI_OPENIPMI_MAX_REQ_DATA_SIZE] = {
        req->reserve_id_lsb,
        req->reserve_id_msb,
        req->record_id_lsb,
        req->record_id_msb,
        req->offset,
        req->count
    };

    rv = ipmi_send_message(dev, IPMI_NETFN_STORAGE, IPMI_CMD_SDR_GET,
            req_buf, 6, rsp_buf, rsp_buf_size);
    return rv;
}

/**
 * Get FRU Inventory Area Info Command
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 34.1
 *     Requesut data
 *                   1 FRU Device ID
 *     Respond data
 *                   1 Completion Code
 *                   2 FRU Inventory area size in bytes, LS Byte
 *                   3 FRU Inventory area size in bytes, MS Byte
 *                   4 Data type
 */
static int ipmi_get_fru_inv_area_info_cmd(struct device *dev,
    unsigned char fru_dev_id,
    unsigned char *rsp_buf, unsigned short rsp_buf_size)
{
    int rv;
    unsigned char req_buf[IPMI_OPENIPMI_MAX_REQ_DATA_SIZE] = {
        fru_dev_id
    };

    rv = ipmi_send_message(dev, IPMI_NETFN_STORAGE, IPMI_CMD_FRU_AREA_INFO_GET,
            req_buf, 1, rsp_buf, rsp_buf_size);
    return rv;
}

/**
 * Read FRU Data Command
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 34.2
 *     Requesut data
 *                   1 FRU Device ID
 *                   2 FRU Inventory Offset to read, LS Byte
 *                   3 FRU Inventory Offset to read, MS Byte
 *                   4 Count to read
 *     Respond data
 *                   1 Completion Code
 *                   2 Count returned
 *                   3:2+N Requested data
 */
static int ipmi_read_fru_data_cmd(struct device *dev,
    struct fru_data_read_req *req,
    unsigned char *rsp_buf, unsigned short rsp_buf_size)
{
    int rv;
    unsigned char req_buf[IPMI_OPENIPMI_MAX_REQ_DATA_SIZE] = {
        req->fru_dev_id,
        req->offset_lsb,
        req->offset_msb,
        req->count
    };

    rv = ipmi_send_message(dev, IPMI_NETFN_STORAGE, IPMI_CMD_FRU_DATA_GET,
            req_buf, 4, rsp_buf, rsp_buf_size);
    return rv;
}

/**
 * Get Sensor Thresholds Command
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 35.9
 *     Requesut data
 *                   1 sensor number
 *     Respond data
 *                   1 Completion Code
 *                   2 Mask
 *                   3 lower non-critical threshold
 *                   4 lower critical threshold
 *                   5 lower non-recoverable threshold
 *                   6 upper non-critical threshold
 *                   7 upper critical
 *                   8 upper non-recoverable
 */
static int ipmi_get_sensor_thresholds_cmd(struct device *dev,
    unsigned char sensor_number,
    unsigned char *rsp_buf, unsigned short rsp_buf_size)
{
    int rv;
    unsigned char req_buf[IPMI_OPENIPMI_MAX_REQ_DATA_SIZE] = {
        sensor_number,
    };

    rv = ipmi_send_message(dev, IPMI_NETFN_SENSOR, IPMI_CMD_SENSOR_THRESHOLD,
            req_buf, 1, rsp_buf, rsp_buf_size);
    return rv;
}

/**
 * Get Sensor Reading Command
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 35.14
 *     Requesut data
 *                   1 sensor number
 *     Respond data
 *                   1 Completion Code
 *                   2 Sensor reading
 *                   3 Status
 *                   4 Data lsb
 *                   5 Data msb
 */
static int ipmi_get_sensor_reading_cmd(struct device *dev,
    unsigned char sensor_number,
    unsigned char *rsp_buf, unsigned short rsp_buf_size)
{
    int rv;
    unsigned char req_buf[IPMI_OPENIPMI_MAX_REQ_DATA_SIZE] = {
        sensor_number
    };

    rv = ipmi_send_message(dev, IPMI_NETFN_SENSOR, IPMI_CMD_SENSOR_READ,
            req_buf, 1, rsp_buf, rsp_buf_size);
    return rv;
}

/**
 * Get Sensor type Command
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 35.16
 *     Requesut data
 *                   1 sensor number
 *     Respond data
 *                   1 Completion Code
 *                   2 Sensor type
 *                   3 Event/Reading type code
 */
static int ipmi_get_sensor_type_cmd(struct device *dev,
    unsigned char sensor_number,
    unsigned char *rsp_buf, unsigned short rsp_buf_size)
{
    int rv;
    unsigned char req_buf[IPMI_OPENIPMI_MAX_REQ_DATA_SIZE] = {
        sensor_number
    };

    rv = ipmi_send_message(dev, IPMI_NETFN_SENSOR, IPMI_CMD_SENSOR_TYPE,
            req_buf, 1, rsp_buf, rsp_buf_size);
    return rv;
}

static int _sdr_data_get(struct list_head *sdr_list,
    uint8_t sdr_type, uint8_t id, uint8_t **data)
{
    struct ipmi_sdr_list *sdr_node = NULL;
    bool sdr_found = false;

    if(data == NULL) {
        return -EINVAL;
    }

    list_for_each_entry(sdr_node, sdr_list, list) {
        if(sdr_node->sdr_type == sdr_type &&
            sdr_node->id == id) {
            sdr_found = true;
            break;
        }
    }

    if(sdr_found) {
        *data = sdr_node->sdr_data;
    }

    return (sdr_found) ? 0:-EINVAL;
}

static int _sysfs_create(struct device *dev, struct list_head *list,
    uint8_t sub_id, struct bmc_dev_attr_node *node, char *suffix_name,
    umode_t mode,
    ssize_t (*show)(struct device *, struct device_attribute *,char *),
    ssize_t (*store)(struct device *, struct device_attribute *,
        const char *, size_t))
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    int rv = 0;
    struct bmc_dev_attr *bdev_attr = NULL;

    if(!bmc_data) {
        rv = -EINVAL;
        goto done;;
    }

    bdev_attr = devm_kzalloc(dev, sizeof(struct bmc_dev_attr), GFP_KERNEL);
    if (!bdev_attr) {
        dev_dbg(dev, "Failed to allocate memory for bdev_attr.\n");
        rv = -ENOSPC;
        goto done;
    }

    bdev_attr->type = node->type;
    bdev_attr->sub_type = node->sub_type;
    bdev_attr->id = node->id;
    bdev_attr->sub_id = sub_id;
    bdev_attr->is_probe = node->is_probe;
    if(suffix_name) {
        snprintf(bdev_attr->name, sizeof(bdev_attr->name), "%s_%s", node->name,
            suffix_name);
    } else {
        snprintf(bdev_attr->name, sizeof(bdev_attr->name), "%s", node->name);
    }
    memcpy(bdev_attr->udf_cmd, node->udf_cmd,
        node->cmd_len > sizeof(bdev_attr->udf_cmd) ?
        sizeof(bdev_attr->udf_cmd):node->cmd_len);
    bdev_attr->cmd_len = node->cmd_len;
    bdev_attr->dev_attr.attr.name = bdev_attr->name;
    bdev_attr->dev_attr.attr.mode = mode;
    bdev_attr->dev_attr.show = show;
    bdev_attr->dev_attr.store = store;

    rv = sysfs_add_file_to_group(&dev->kobj, &bdev_attr->dev_attr.attr,
            bmc_data->grp.name);
    if(rv < 0) {
        dev_dbg(dev, "Create sysfs(%s) fail (%d)\n", bdev_attr->name, rv);
        devm_kfree(dev, bdev_attr);
        goto done;
    } else {
        list_add_tail(&bdev_attr->list, list);
        dev_dbg(dev, "Create sysfs attr type(%d) name(%s) "
            "sub_type(0x%02X) id(0x%02X) sub_id(%d) entry addr(%p)\n",
            bdev_attr->type, bdev_attr->dev_attr.attr.name,
            bdev_attr->sub_type, bdev_attr->id, bdev_attr->sub_id,
            &bdev_attr->dev_attr.attr);
    }

done:
    return rv;
}

static int _sysfs_full_sensors_create(struct device *dev,
    struct bmc_dev_attr_node *node)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    int rv = 0;
    struct bmc_dev_attr *bdev_attr = NULL;
    struct bmc_dev_attr *tmp_bdev_attr = NULL;
    struct bmc_dev_attr new_list;

    if(!bmc_data) {
        return -EINVAL;
    }

    INIT_LIST_HEAD(&new_list.list);

    rv = _sysfs_create(dev, &new_list.list,
            SENSOR_SUB_ID_READING, node,
            NULL, (S_IRUGO), class_sensor_show, NULL);

    if(rv) {
        goto free_new_list;
    }

    rv = _sysfs_create(dev, &new_list.list,
            SENSOR_SUB_ID_SENSOR_ID, node,
            "sensor_id", (S_IRUGO), class_sensor_show, NULL);

    if(rv) {
        goto free_new_list;
    }

    rv = _sysfs_create(dev, &new_list.list,
            SENSOR_SUB_ID_SENSOR_NAME, node,
            "sensor_name", (S_IRUGO), class_sensor_show, NULL);

    if(rv) {
        goto free_new_list;
    }

    rv = _sysfs_create(dev, &new_list.list,
            SENSOR_SUB_ID_SENSOR_TYPE, node,
            "sensor_type", (S_IRUGO), class_sensor_show, NULL);

    if(rv) {
        goto free_new_list;
    }

    rv = _sysfs_create(dev, &new_list.list,
            SENSOR_SUB_ID_SENSOR_UNIT, node,
            "sensor_unit", (S_IRUGO), class_sensor_show, NULL);

    if(rv) {
        goto free_new_list;
    }

    rv = _sysfs_create(dev, &new_list.list,
            SENSOR_SUB_ID_UPPER_NONE_CRITICAL, node,
            "upper_none_critical", (S_IRUGO), class_sensor_show, NULL);
    if(rv) {
        goto free_new_list;
    }

    rv = _sysfs_create(dev, &new_list.list,
            SENSOR_SUB_ID_UPPER_CRITICAL, node,
            "upper_critical", (S_IRUGO), class_sensor_show, NULL);
    if(rv) {
        goto free_new_list;
    }

    rv = _sysfs_create(dev, &new_list.list,
            SENSOR_SUB_ID_UPPER_NONE_RECOVERABLE, node,
            "upper_none_recoverable", (S_IRUGO), class_sensor_show, NULL);
    if(rv) {
        goto free_new_list;
    }

    rv = _sysfs_create(dev, &new_list.list,
            SENSOR_SUB_ID_LOWER_NONE_CRITICAL, node,
            "lower_none_critical", (S_IRUGO), class_sensor_show, NULL);
    if(rv) {
        goto free_new_list;
    }

    rv = _sysfs_create(dev, &new_list.list,
            SENSOR_SUB_ID_LOWER_CRITICAL, node,
            "lower_critical", (S_IRUGO), class_sensor_show, NULL);
    if(rv) {
        goto free_new_list;
    }

    rv = _sysfs_create(dev, &new_list.list,
            SENSOR_SUB_ID_LOWER_NONE_RECOVERABLE, node,
            "lower_none_recoverable", (S_IRUGO), class_sensor_show, NULL);

    if(rv) {
        goto free_new_list;
    }

    if(IS_SENSOR_UNIT_TYPE_TEMPERATURE(node->sensor_unit)) {
        /**
         * Map sensor thresholds to min and max to meet SIP3 requirements.
         *
         * For sensors that are of the 'temperature' type and use a full SDR record
         * (SDR Type 0x01), the following mapping is applied:
         * - max is mapped from the upper_non_critical threshold.
         * - min is set to (upper_non_critical_threshold - 5).
         */
        rv = _sysfs_create(dev, &new_list.list,
                SENSOR_SUB_ID_TEMP_MIN, node,
                "min", (S_IRUGO), class_sensor_show, NULL);
        if(rv) {
            goto free_new_list;
        }

        rv = _sysfs_create(dev, &new_list.list,
                SENSOR_SUB_ID_TEMP_MAX, node,
                "max", (S_IRUGO), class_sensor_show, NULL);

        if(rv) {
            goto free_new_list;
        }
    }

    list_splice_tail_init(&new_list.list,&bmc_data->bmc_attr_lists.list);

    return 0;

free_new_list:
    list_for_each_entry_safe(bdev_attr, tmp_bdev_attr, &new_list.list, list) {
        sysfs_remove_file_from_group(&dev->kobj, &bdev_attr->dev_attr.attr,
            bmc_data->grp.name);
        list_del(&bdev_attr->list);
        devm_kfree(dev, bdev_attr);
    }
    return rv;
}

static int _sysfs_compact_sensors_create(struct device *dev,
    struct bmc_dev_attr_node *node)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    int rv = 0;
    struct bmc_dev_attr *bdev_attr = NULL;
    struct bmc_dev_attr *tmp_bdev_attr = NULL;
    struct bmc_dev_attr new_list;

    if(!bmc_data) {
        return -EINVAL;
    }

    INIT_LIST_HEAD(&new_list.list);

    rv = _sysfs_create(dev, &new_list.list, SENSOR_SUB_ID_READING, node, NULL,
            (S_IRUGO), class_sensor_show, NULL);

    if(rv) {
        goto free_new_list;
    }

    rv = _sysfs_create(dev, &new_list.list,
            SENSOR_SUB_ID_SENSOR_ID, node,
            "sensor_id", (S_IRUGO), class_sensor_show, NULL);

    if(rv) {
        goto free_new_list;
    }

    rv = _sysfs_create(dev, &new_list.list,
            SENSOR_SUB_ID_SENSOR_NAME, node,
            "sensor_name", (S_IRUGO), class_sensor_show, NULL);

    if(rv) {
        goto free_new_list;
    }

    rv = _sysfs_create(dev, &new_list.list,
            SENSOR_SUB_ID_SENSOR_TYPE, node,
            "sensor_type", (S_IRUGO), class_sensor_show, NULL);

    if(rv) {
        goto free_new_list;
    }

    rv = _sysfs_create(dev, &new_list.list,
            SENSOR_SUB_ID_SENSOR_UNIT, node,
            "sensor_unit", (S_IRUGO), class_sensor_show, NULL);

    if(rv) {
        goto free_new_list;
    }

    list_splice_tail_init(&new_list.list,&bmc_data->bmc_attr_lists.list);
    return 0;

free_new_list:
    list_for_each_entry_safe(bdev_attr, tmp_bdev_attr, &new_list.list, list) {
        sysfs_remove_file_from_group(&dev->kobj, &bdev_attr->dev_attr.attr,
            bmc_data->grp.name);
        list_del(&bdev_attr->list);
        devm_kfree(dev, bdev_attr);
    }
    return rv;
}

static int _sysfs_fru_create(struct device *dev,
    struct bmc_dev_attr_node *node)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    int rv = 0;
    struct bmc_dev_attr *bdev_attr = NULL;
    struct bmc_dev_attr *tmp_bdev_attr = NULL;
    struct bmc_dev_attr new_list;

    if(!bmc_data) {
        return -EINVAL;
    }

    INIT_LIST_HEAD(&new_list.list);

    rv = _sysfs_create(dev, &new_list.list, FRU_SUB_ID_MF, node, "mf",
            (S_IRUGO), class_fru_show, NULL);
    if(rv) {
        goto free_new_list;
    }

    rv = _sysfs_create(dev, &new_list.list, FRU_SUB_ID_PNAME, node, "pname",
            (S_IRUGO), class_fru_show, NULL);
    if(rv) {
        goto free_new_list;
    }

    rv = _sysfs_create(dev, &new_list.list, FRU_SUB_ID_PN, node, "pn",
            (S_IRUGO), class_fru_show, NULL);
    if(rv) {
        goto free_new_list;
    }

    rv = _sysfs_create(dev, &new_list.list, FRU_SUB_ID_PV, node, "pv",
            (S_IRUGO), class_fru_show, NULL);
    if(rv) {
        goto free_new_list;
    }

    rv = _sysfs_create(dev, &new_list.list, FRU_SUB_ID_SN, node, "sn",
            (S_IRUGO), class_fru_show, NULL);
    if(rv) {
        goto free_new_list;
    }

    list_splice_tail_init(&new_list.list,&bmc_data->bmc_attr_lists.list);
    return 0;

free_new_list:
    list_for_each_entry_safe(bdev_attr, tmp_bdev_attr, &new_list.list, list) {
        sysfs_remove_file_from_group(&dev->kobj, &bdev_attr->dev_attr.attr,
            bmc_data->grp.name);
        list_del(&bdev_attr->list);
        devm_kfree(dev, bdev_attr);
    }
    return rv;
}

static int _sysfs_default_create(struct device *dev,
    struct bmc_dev_attr_node *node)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    int rv = 0;
    struct bmc_dev_attr *bdev_attr = NULL;
    struct bmc_dev_attr *tmp_bdev_attr = NULL;
    struct bmc_dev_attr new_list;

    if(!bmc_data) {
        return -EINVAL;
    }

    INIT_LIST_HEAD(&new_list.list);
    if(node->id == DEFAULT_ID_FIREMWARE_VERSION) {
        rv = _sysfs_create(dev, &new_list.list, 
                0, node,
                NULL, (S_IRUGO), class_firmware_version_show, NULL);
        if(rv) {
            goto free_new_list;
        }
    }else if(node->id == DEFAULT_ID_FIREMWARE_VERSION_LONG) {
        rv = _sysfs_create(dev, &new_list.list, 
                0, node,
                NULL, (S_IRUGO), class_firmware_version_show, NULL);
        if(rv) {
            goto free_new_list;
        }
    } else if(node->id == DEFAULT_ID_WATCHDOG){
        rv = _sysfs_create(dev, &new_list.list,
                WDG_SUB_ID_STATE, node,
                "state", (S_IRUGO), class_watchdog_show, NULL);
        if(rv) {
            goto free_new_list;
        }

        rv = _sysfs_create(dev, &new_list.list,
                WDG_SUB_ID_TIMELEFT, node,
                "timeleft", (S_IRUGO), class_watchdog_show, NULL);
        if(rv) {
            goto free_new_list;
        }

        rv = _sysfs_create(dev, &new_list.list,
                WDG_SUB_ID_TIMEOUT, node,
                "timeout", (S_IRUGO | S_IWUSR),
                class_watchdog_show, class_watchdog_store);
        if(rv) {
            goto free_new_list;
        }

        rv = _sysfs_create(dev, &new_list.list,
                WDG_SUB_ID_ENABLE, node,
                "enable", (S_IRUGO | S_IWUSR),
                class_watchdog_show, class_watchdog_store);
        if(rv) {
            goto free_new_list;
        }
    } else if(node->id == DEFAULT_ID_POWER_CYCLE){
        rv = _sysfs_create(dev, &new_list.list,
                0, node,
                NULL, (S_IWUSR), NULL, class_power_cycle_store);
        if(rv) {
            goto free_new_list;
        }
    } else if(node->id == DEFAULT_ID_OEM_UFI_PSU_RESET){
        rv = _sysfs_create(dev, &new_list.list,
                0, node,
                NULL, (S_IWUSR), NULL, class_oem_ufi_psu_reset_store);
        if(rv) {
            goto free_new_list;
        }
    } else {
        rv = -EACCES;
        goto free_new_list;
    }
    list_splice_tail_init(&new_list.list,&bmc_data->bmc_attr_lists.list);
    return 0;

free_new_list:
    list_for_each_entry_safe(bdev_attr, tmp_bdev_attr, &new_list.list, list) {
        sysfs_remove_file_from_group(&dev->kobj, &bdev_attr->dev_attr.attr,
            bmc_data->grp.name);
        list_del(&bdev_attr->list);
        devm_kfree(dev, bdev_attr);
    }
    return rv;
}

static int _sysfs_udf_create(struct device *dev,
    struct bmc_dev_attr_node *node)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    int rv = 0;
    struct bmc_dev_attr *bdev_attr = NULL;
    struct bmc_dev_attr *tmp_bdev_attr = NULL;
    struct bmc_dev_attr new_list;

    if(!bmc_data) {
        return -EINVAL;
    }

    INIT_LIST_HEAD(&new_list.list);
    if(node->id > UDF_ID_UDF_INVALID && node->id < UDF_ID_UDF_MAX){
        rv = _sysfs_create(dev, &new_list.list, node->sub_id, node, NULL,
                node->mode,
                (node->mode & S_IRUGO) ? class_udf_show:NULL,
                (node->mode & S_IWUSR) ? class_udf_store:NULL);

        if(rv) {
            goto free_new_list;
        }

    } else {
        rv = -EACCES;
        goto free_new_list;
    }
    list_splice_tail_init(&new_list.list,&bmc_data->bmc_attr_lists.list);
    return 0;

free_new_list:
    list_for_each_entry_safe(bdev_attr, tmp_bdev_attr, &new_list.list, list) {
        sysfs_remove_file_from_group(&dev->kobj, &bdev_attr->dev_attr.attr,
            bmc_data->grp.name);
        list_del(&bdev_attr->list);
        devm_kfree(dev, bdev_attr);
    }
    return rv;
}

static int sysfs_create(struct device *dev, struct bmc_dev_attr_node *node)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    int rv = 0;
    struct bmc_dev_attr *bdev_attr = NULL;

    if(!bmc_data) {
        return -EINVAL;
    }

    mutex_lock(&bmc_data->access_lock);

    list_for_each_entry(bdev_attr, &bmc_data->bmc_attr_lists.list, list) {
        if(!strcmp(node->name, bdev_attr->name)) {
            ufi_bmc_pr_warn(dev, "Operation failed: sysfs existed name(%s)\n",
                node->name);
            rv =-EACCES;
            goto done;
        }

        if(node->type == bdev_attr->type &&
            node->sub_type == bdev_attr->sub_type &&
            node->id == bdev_attr->id && node->sub_id == bdev_attr->sub_id) {
            dev_dbg(dev, "Operation failed: sysfs existed type(%d) "
                "sub_type(%d) id(%d) sub_id(%d)\n",
                node->type, node->sub_type, node->id, node->sub_id);
            rv =-EACCES;
            goto done;
        }
    }

    if(node->type == ATTR_TYPE_SDR) {
        if(node->sub_type == SDR_RECORD_TYPE_FULL_SENSOR) {
            rv = _sysfs_full_sensors_create(dev, node);
        } else if(node->sub_type == SDR_RECORD_TYPE_COMPACT_SENSOR) {
            rv = _sysfs_compact_sensors_create(dev, node);
        } else if(node->sub_type == SDR_RECORD_TYPE_FRU_DEVICE_LOCATOR) {
            rv = _sysfs_fru_create(dev, node);
        } else {
            dev_dbg(dev, "Invalid sdr type.\n");
            rv = -EINVAL;
        }
    } else if(node->type == ATTR_TYPE_DEFAULT) {
        rv = _sysfs_default_create(dev, node);
    } else if(node->type == ATTR_TYPE_UDF) {
        rv = _sysfs_udf_create(dev, node);
    } else {
        dev_dbg(dev, "Invalid type.\n");
        rv = -EINVAL;
    }

done:
    mutex_unlock(&bmc_data->access_lock);
    return rv;
}

static int64_t _adjust_result_temp_min(int64_t result) {
    return (result - (5*VALUE_FACTOR_MILLI));
}

/**
 * Sensor Reading Conversion Formula
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 36.3
 */
static int _analog_parsing(struct device *dev,
    char *buf, uint8_t *sdr_data, uint8_t *cache,
    int64_t (*adjust_result)(int64_t result))
{
    int rv = 0;
    struct sdr_full_get_rsp *data = NULL;
    uint8_t value = 0;
    uint16_t mtol;
    uint32_t bacc;
    int m = 0, b = 0, k1 = 0, k2 = 0;
    uint8_t unit_analog = 0;
    int64_t result = 0;
    int64_t val_factor = VALUE_FACTOR;

    if(!buf || !sdr_data || !cache) {
        return -EINVAL;
    }

    value = cache[1];

    data = (struct sdr_full_get_rsp *) sdr_data;
    mtol = (data->m_msb << 8 | data->m_lsb);
    bacc = ((data->r_b_exp << 24) | (data->accuracy << 16) |
            (data->b_msb << 8) | (data->b_lsb));
    m = __TO_M(mtol);
    b = __TO_B(bacc);
    k1 = __TO_B_EXP(bacc);
    k2 = __TO_R_EXP(bacc);
    unit_analog = (
        (data->sensor_units1 & IPMI_SENSOR_UNIT_ANALOG_DATA_FMT_MASK)
        >> IPMI_SENSOR_UNIT_ANALOG_DATA_FMT_POS
    );


    if(IS_SENSOR_UNIT_TYPE_TEMPERATURE(data->sensor_units2) ||
        IS_SENSOR_UNIT_TYPE_VOLTAGE_CURRENT(data->sensor_units2)) {
        val_factor = VALUE_FACTOR_MILLI;
    } else if(IS_SENSOR_UNIT_TYPE_POWER(data->sensor_units2)) {
        val_factor = VALUE_FACTOR_MICRO;
    } else {
        val_factor = VALUE_FACTOR;
    }

    dev_dbg(dev, "value(0x%02X) factor(%lld) m(%d) b(%d) k1(%d) k2(%d) "
        "unit_analog(%d) mtol(%d) bacc(0x%X)\n",
        value, val_factor, m, b ,k1, k2,
        unit_analog, mtol, bacc);

    switch (unit_analog) {
        case 0:
            if(k1 < 0 && k2 < 0)
                result = (val_factor * ((m * value) +
                            (b / ipow(10, abs(k1))))/ ipow(10, abs(k2)));
            else if (k1 < 0)
                result = (val_factor * ((m * value) +
                            (b / ipow(10, abs(k1))))* ipow(10, k2));
            else if (k2 < 0)
                result = (val_factor * ((m * value) +
                            (b * ipow(10, k1)))/ ipow(10, abs(k2)));
            else
                result = (val_factor * ((m * value) +
                            (b * ipow(10, k1))) * ipow(10, k2));
            break;
        case 1:
        case 2:
            if(unit_analog == 1 && value & 0x80) {
                value++;
            }

            if(k1 < 0 && k2 < 0)
                result = (val_factor * ((m *  (int8_t) value) +
                            (b / ipow(10, abs(k1))))/ ipow(10, abs(k2)));
            else if (k1 < 0)
                result = (val_factor * ((m * (int8_t) value) +
                            (b / ipow(10, abs(k1))))* ipow(10, k2));
            else if (k2 < 0)
                result = (val_factor * ((m * (int8_t) value) +
                            (b * ipow(10, k1)))/ ipow(10, abs(k2)));
            else
                result = (val_factor * ((m * (int8_t) value) +
                            (b * ipow(10, k1))) * ipow(10, k2));
            break;
        default:
            // Oops! This isn't an analog sensor
            goto ret_na;
    }

    if(adjust_result != NULL) {
       result = adjust_result(result);
    }

    rv = sprintf(buf,"%lld\n", result);
    return rv;
ret_na:
    return sprintf(buf, "NA\n");
}

/**
 * Sensor and Event Code Tables
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 42
 */
static int _discete_parsing(struct device *dev,
    char *buf, uint8_t *sdr_data, uint8_t *cache)
{
    int rv = 0;
    struct sdr_compact_get_rsp *data = NULL;
    uint8_t value1 = 0;
    uint8_t value2 = 0;

    if(!buf || !sdr_data || !cache) {
        return -EINVAL;
    }

    value1 = cache[0];
    value2 = cache[1];
    data = (struct sdr_compact_get_rsp *) sdr_data;
    if(data->evt_type_code >= EVENT_READING_TYPE_CODE_GENERIC_BEGIN &&
        data->evt_type_code <= EVENT_READING_TYPE_CODE_GENERIC_END) {
        switch(data->evt_type_code) {
            case GENERIC_EVENT_READING_TYPE_CODE_DIGITAL_DISCRETE_STATE:
            case GENERIC_EVENT_READING_TYPE_CODE_DIGITAL_DISCRETE_PREDICTIVE:
            case GENERIC_EVENT_READING_TYPE_CODE_DIGITAL_DISCRETE_LIMIT:
            case GENERIC_EVENT_READING_TYPE_CODE_DIGITAL_DISCRETE_PERFORMANCE:
            case GENERIC_EVENT_READING_TYPE_CODE_AVAILABILITY_DEVICE:
            case GENERIC_EVENT_READING_TYPE_CODE_AVAILABILITY_DEVICE_FUNC:
            {
                if(value1 & 0x1) {
                    rv = sprintf(buf,"0\n");
                }else if(value1 & 0x2) {
                    rv = sprintf(buf,"1\n");
                } else {
                    goto ret_na;
                }
                break;
            }
            default:
            {
                goto ret_na;
            }
        }
    } else if(data->evt_type_code == EVENT_READING_TYPE_CODE_SENSOR) {
        switch(data->sensor_type) {
            case SENSOR_TYPE_CODE_POWER_SUPPLY:
            {
                if(value1 & 0x1) {
                    rv = sprintf(buf,"1\n");
                } else {
                    rv = sprintf(buf,"0\n");
                }
                break;
            }
            default:
            {
                goto ret_na;
            }
        }
    } else {
        rv = sprintf(buf,"0x%02X%02X\n", value2, value1);
    }
    return rv;
ret_na:
    return sprintf(buf, "NA\n");
}

/**
 * Sensor type Code
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 42.2
 */
static int _sensor_type_parsing(struct device *dev,
    char *buf, uint8_t *cache)
{
    uint8_t value1 = 0;

    if(!buf || !cache) {
        return -EINVAL;
    }

    value1 = cache[0];

    if(value1 == SENSOR_TYPE_CODE_RESERVE) {
        return sprintf(buf, "reserved\n");
    } else if(value1 == SENSOR_TYPE_CODE_TEMPERATURE) {
        return sprintf(buf, "temperature\n");
    } else if(value1 == SENSOR_TYPE_CODE_VOLTAGE) {
        return sprintf(buf, "voltage\n");
    } else if(value1 == SENSOR_TYPE_CODE_CURRENT) {
        return sprintf(buf, "current\n");
    } else if(value1 == SENSOR_TYPE_CODE_FAN) {
        return sprintf(buf, "fan\n");
    } else if(value1 == SENSOR_TYPE_CODE_PHYSICAL_SECURITY) {
        return sprintf(buf, "physical_security\n");
    } else if(value1 == SENSOR_TYPE_CODE_PLATFORM_SECURITY_VIOLATION_ATTEMPT) {
        return sprintf(buf, "platform_security_violation_attempt\n");
    } else if(value1 == SENSOR_TYPE_CODE_PROCESSOR) {
        return sprintf(buf, "processor\n");
    } else if(value1 == SENSOR_TYPE_CODE_POWER_SUPPLY) {
        return sprintf(buf, "power_supply\n");
    } else if(value1 == SENSOR_TYPE_CODE_POWER_UNIT) {
        return sprintf(buf, "power_unit\n");
    } else if(value1 == SENSOR_TYPE_CODE_COOLING_DEVICE) {
        return sprintf(buf, "cooling_device\n");
    } else if(value1 == SENSOR_TYPE_CODE_OTHER_SENSOR) {
        return sprintf(buf, "other_units_based_sensor\n");
    } else if(value1 == SENSOR_TYPE_CODE_MEMORY) {
        return sprintf(buf, "memory\n");
    } else if(value1 == SENSOR_TYPE_CODE_DRIVE_SLOT) {
        return sprintf(buf, "drive_slot\n");
    } else if(value1 == SENSOR_TYPE_CODE_POST_MEMORY_RESIZE) {
        return sprintf(buf, "post_memory_resize\n");
    } else if(value1 == SENSOR_TYPE_CODE_SYSTEM_FIRMWARE_PROGRESS) {
        return sprintf(buf, "system_firmware_progress\n");
    } else if(value1 == SENSOR_TYPE_CODE_EVENT_LOGGING_DISABLED) {
        return sprintf(buf, "event_logging_disabled\n");
    } else if(value1 == SENSOR_TYPE_CODE_WATCHDOG_1) {
        return sprintf(buf, "watchdog_1\n");
    } else if(value1 == SENSOR_TYPE_CODE_SYSTEM_EVENT) {
        return sprintf(buf, "system_event\n");
    } else if(value1 == SENSOR_TYPE_CODE_CRITICAL_INTERRUPT) {
        return sprintf(buf, "critical_interrupt\n");
    } else if(value1 == SENSOR_TYPE_CODE_BUTTON_SWITCH) {
        return sprintf(buf, "button_switch\n");
    } else if(value1 == SENSOR_TYPE_CODE_MODULE_BOARD) {
        return sprintf(buf, "module_board\n");
    } else if(value1 == SENSOR_TYPE_CODE_MICROCONTROLLER_COPROCESSOR) {
        return sprintf(buf, "microcontroller_coprocessor\n");
    } else if(value1 == SENSOR_TYPE_CODE_ADD_IN_CARD) {
        return sprintf(buf, "add_in_card\n");
    } else if(value1 == SENSOR_TYPE_CODE_CHASSIS) {
        return sprintf(buf, "chassis\n");
    } else if(value1 == SENSOR_TYPE_CODE_CHIP_SET) {
        return sprintf(buf, "chip_set\n");
    } else if(value1 == SENSOR_TYPE_CODE_OTHER_FRU) {
        return sprintf(buf, "other_fru\n");
    } else if(value1 == SENSOR_TYPE_CODE_CABLE_INTERCONNECT) {
        return sprintf(buf, "cable_interconnect\n");
    } else if(value1 == SENSOR_TYPE_CODE_TERMINATOR) {
        return sprintf(buf, "terminator\n");
    } else if(value1 == SENSOR_TYPE_CODE_SYSTEM_BOOT_RESTART_INITIATED) {
        return sprintf(buf, "system_boot_restart_initiated\n");
    } else if(value1 == SENSOR_TYPE_CODE_BOOT_ERROR) {
        return sprintf(buf, "boot_error\n");
    } else if(value1 == SENSOR_TYPE_CODE_BASE_OS_BOOT_INSTALLATION_STATUS) {
        return sprintf(buf, "base_os_boot_installation_status\n");
    } else if(value1 == SENSOR_TYPE_CODE_OS_STOP_SHUTDOWN) {
        return sprintf(buf, "os_stop_shutdown\n");
    } else if(value1 == SENSOR_TYPE_CODE_SLOT_CONNECTOR) {
        return sprintf(buf, "slot_connector\n");
    } else if(value1 == SENSOR_TYPE_CODE_SYSTEM_ACPI_POWER_STATE) {
        return sprintf(buf, "system_acpi_power_state\n");
    } else if(value1 == SENSOR_TYPE_CODE_WATCHDOG_2) {
        return sprintf(buf, "watchdog_2\n");
    } else if(value1 == SENSOR_TYPE_CODE_PLATFORM_ALERT) {
        return sprintf(buf, "platform_alert\n");
    } else if(value1 == SENSOR_TYPE_CODE_ENTITY_PRESENCE) {
        return sprintf(buf, "entity_presence\n");
    } else if(value1 == SENSOR_TYPE_CODE_MONITOR_ASIC_IC) {
        return sprintf(buf, "monitor_asic_ic\n");
    } else if(value1 == SENSOR_TYPE_CODE_LAN) {
        return sprintf(buf, "lan\n");
    } else if(value1 == SENSOR_TYPE_CODE_MANAGEMENT_SUBSYSTEM_HEALTH) {
        return sprintf(buf, "management_subsystem_health\n");
    } else if(value1 == SENSOR_TYPE_CODE_BATTERY) {
        return sprintf(buf, "battery\n");
    } else if(value1 == SENSOR_TYPE_CODE_SESSION_AUDIT) {
        return sprintf(buf, "session_audit\n");
    } else if(value1 == SENSOR_TYPE_CODE_VERSION_CHANGE) {
        return sprintf(buf, "version_change\n");
    } else if(value1 == SENSOR_TYPE_CODE_FRU_STATE) {
        return sprintf(buf, "fru_state\n");
    } else if(value1 > SENSOR_TYPE_CODE_FRU_STATE &&
        value1 < SENSOR_TYPE_CODE_ORM_RESERVE_BEGIN) {
        return sprintf(buf, "reserved\n");
    } else if(value1 >= SENSOR_TYPE_CODE_ORM_RESERVE_BEGIN &&
        value1 <= SENSOR_TYPE_CODE_ORM_RESERVE_END) {
        return sprintf(buf, "oem_reserved\n");
    } else {
        return sprintf(buf, "NA\n");
    }
}

/**
 * Sensor unit type Codes
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 43.17
 */
static int _sensor_unit_parsing(struct device *dev,
    char *buf, uint8_t *cache)
{
    uint8_t value1 = 0;

    if(!buf || !cache) {
        return -EINVAL;
    }

    value1 = cache[0];
    /*
     * Although the ipmi spec defines temperature units as degrees C, F, and K,
     * we convert them to millidegrees to comply with the S3IP standard.
     *
     * Similarly, Volts are converted to milliVolts, Amps are converted
     * to milliAmps, and Watts are converted to microWatts.
     */

    if(value1 == SENSOR_UNIT_TYPE_CODE_UNSPECIFIED) {
        return sprintf(buf, "unspecified\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_DEGREES_C) {
        return sprintf(buf, "millidegrees C\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_DEGREES_F) {
        return sprintf(buf, "millidegrees F\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_DEGREES_K) {
        return sprintf(buf, "millidegrees K\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_VOLTS) {
        return sprintf(buf, "milliVolts\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_AMPS) {
        return sprintf(buf, "milliAmps\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_WATTS) {
        return sprintf(buf, "microWatts\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_JOULES) {
        return sprintf(buf, "Joules\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_COULOMBS) {
        return sprintf(buf, "Coulombs\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_VA) {
        return sprintf(buf, "VA\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_NITS) {
        return sprintf(buf, "Nits\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_LUMEN) {
        return sprintf(buf, "lumen\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_LUX) {
        return sprintf(buf, "lux\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_CANDELA) {
        return sprintf(buf, "Candela\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_KPA) {
        return sprintf(buf, "kPa\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_PSI) {
        return sprintf(buf, "PSI\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_NEWTON) {
        return sprintf(buf, "Newton\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_CFM) {
        return sprintf(buf, "CFM\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_RPM) {
        return sprintf(buf, "RPM\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_HZ) {
        return sprintf(buf, "Hz\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_MICROSECOND) {
        return sprintf(buf, "microsecond\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_MILLISECOND) {
        return sprintf(buf, "millisecond\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_SECOND) {
        return sprintf(buf, "second\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_MINUTE) {
        return sprintf(buf, "minute\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_HOUR) {
        return sprintf(buf, "hour\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_DAY) {
        return sprintf(buf, "day\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_WEEK) {
        return sprintf(buf, "week\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_MIL) {
        return sprintf(buf, "mil\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_INCHES) {
        return sprintf(buf, "inches\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_FEET) {
        return sprintf(buf, "feet\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_CU_IN) {
        return sprintf(buf, "cu in\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_CU_FEET) {
        return sprintf(buf, "cu feet\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_MM) {
        return sprintf(buf, "mm\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_CM) {
        return sprintf(buf, "cm\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_M) {
        return sprintf(buf, "m\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_CU_CM) {
        return sprintf(buf, "cu cm\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_CU_M) {
        return sprintf(buf, "cu m\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_LITERS) {
        return sprintf(buf, "liters\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_FLUID_OUNCE) {
        return sprintf(buf, "fluid ounce\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_RADIANS) {
        return sprintf(buf, "radians\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_STERADIANS) {
        return sprintf(buf, "steradians\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_REVOLUTIONS) {
        return sprintf(buf, "revolutions\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_CYCLES) {
        return sprintf(buf, "cycles\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_GRAVITIES) {
        return sprintf(buf, "gravities\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_OUNCE) {
        return sprintf(buf, "ounce\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_POUND) {
        return sprintf(buf, "pound\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_FT_LB) {
        return sprintf(buf, "ft-lb\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_OZ_IN) {
        return sprintf(buf, "oz-in\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_GAUSS) {
        return sprintf(buf, "gauss\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_GILBERTS) {
        return sprintf(buf, "gilberts\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_HENRY) {
        return sprintf(buf, "henry\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_MILLIHENRY) {
        return sprintf(buf, "millihenry\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_FARAD) {
        return sprintf(buf, "farad\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_MICROFARAD) {
        return sprintf(buf, "microfarad\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_OHMS) {
        return sprintf(buf, "ohms\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_SIEMENS) {
        return sprintf(buf, "siemens\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_MOLE) {
        return sprintf(buf, "mole\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_BECQUEREL) {
        return sprintf(buf, "becquerel\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_PPM) {
        return sprintf(buf, "PPM\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_RESERVED) {
        return sprintf(buf, "reserved\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_DECIBELS) {
        return sprintf(buf, "Decibels\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_DBA) {
        return sprintf(buf, "DbA\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_DBC) {
        return sprintf(buf, "DbC\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_GRAY) {
        return sprintf(buf, "gray\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_SIEVERT) {
        return sprintf(buf, "sievert\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_COLOR_TEMP_DEG_K) {
        return sprintf(buf, "color temp deg K\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_BIT) {
        return sprintf(buf, "bit\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_KILOBIT) {
        return sprintf(buf, "kilobit\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_MEGABIT) {
        return sprintf(buf, "megabit\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_GIGABIT) {
        return sprintf(buf, "gigabit\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_BYTE) {
        return sprintf(buf, "byte\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_KILOBYTE) {
        return sprintf(buf, "kilobyte\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_MEGABYTE) {
        return sprintf(buf, "megabyte\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_GIGABYTE) {
        return sprintf(buf, "gigabyte\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_WORD) {
        return sprintf(buf, "word\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_DWORD) {
        return sprintf(buf, "dword\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_QWORD) {
        return sprintf(buf, "qword\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_LINE) {
        return sprintf(buf, "line\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_HIT) {
        return sprintf(buf, "hit\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_MISS) {
        return sprintf(buf, "miss\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_RETRY) {
        return sprintf(buf, "retry\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_RESET) {
        return sprintf(buf, "reset\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_OVERRUN_OVERFLOW) {
        return sprintf(buf, "overrun / overflow \n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_UNDERRUN) {
        return sprintf(buf, "underrun\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_COLLISION) {
        return sprintf(buf, "collision\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_PACKETS) {
        return sprintf(buf, "packets\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_MESSAGES) {
        return sprintf(buf, "messages\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_CHARACTERS) {
        return sprintf(buf, "characters\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_ERROR) {
        return sprintf(buf, "error\n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_CORRECTABLE_ERROR) {
        return sprintf(buf, "correctable error \n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_UNCORRECTABLE_ERROR) {
        return sprintf(buf, "uncorrectable error \n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_FATAL_ERROR) {
        return sprintf(buf, "fatal error \n");
    } else if(value1 == SENSOR_UNIT_TYPE_CODE_GRAMS) {
        return sprintf(buf, "grams\n");
    } else {
        return sprintf(buf, "NA\n");
    }
}

static ssize_t _sensor_update(struct device *dev,
    uint8_t sdr_type, uint8_t id, uint8_t sub_id)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    struct bmc_dev_attr *node = NULL;
    int rv;
    unsigned long new_jiffies = 0;
    unsigned char rsp_buf[IPMI_OPENIPMI_MAX_RSP_DATA_SIZE] = {0};
    uint8_t *sdr_data = NULL;

    if(!bmc_data) {
        return -EINVAL;
    }

    if(sub_id == SENSOR_SUB_ID_READING) {
        rv = ipmi_get_sensor_reading_cmd(dev, id, rsp_buf, sizeof(rsp_buf));
    } else if(sub_id == SENSOR_SUB_ID_SENSOR_TYPE) {
        rv = ipmi_get_sensor_type_cmd(dev, id, rsp_buf, sizeof(rsp_buf));
    } else if(sub_id == SENSOR_SUB_ID_SENSOR_ID ||
        sub_id == SENSOR_SUB_ID_SENSOR_NAME || 
        sub_id == SENSOR_SUB_ID_SENSOR_UNIT) {
        rv = _sdr_data_get(&bmc_data->ipmi_sdr_lists.list,
                sdr_type, id, &sdr_data);
    } else {
        rv = ipmi_get_sensor_thresholds_cmd(dev, id, rsp_buf, sizeof(rsp_buf));
    }

    new_jiffies = jiffies;
    list_for_each_entry(node, &bmc_data->bmc_attr_lists.list, list) {
        if(node->type != ATTR_TYPE_SDR) {
            continue;
        }

        if(id != node->id){
            continue;
        }

        if (sub_id != node->sub_id) {
            if(!IS_SENSOR_THRESHOLD_SUB_ID(sub_id) ||
                !IS_SENSOR_THRESHOLD_SUB_ID(node->sub_id)) {
                continue;
            }
        }

        if(node->sub_id == SENSOR_SUB_ID_READING) {
            if(rv >= 0) {
                struct sensor_reading_get_rsp *sensor =
                    (struct sensor_reading_get_rsp *) rsp_buf;
                if(node->sub_type == SDR_RECORD_TYPE_FULL_SENSOR) {
                    /*
                     * If reading the value fails after a previous success,
                     * retry the operation and log it.
                     */
                    if((node->valid == DATA_VALID_TYPE_VALID) &&
                        !(node->cache[0] &
                            IPMI_SENSOR_READING_STATE_MASK_UNAVAILABLE) &&
                        (sensor->state &
                            IPMI_SENSOR_READING_STATE_MASK_UNAVAILABLE)) {
                        int retry = 1;
                        while (retry <= IPMI_ERR_RETRY_TIMES) {
                            dev_info(dev,
                                "Sensor(%s) reading unavailable, "
                                "retrying (%d)\n", node->name, retry);
                            memset(rsp_buf, 0, sizeof(rsp_buf));
                            rv = ipmi_get_sensor_reading_cmd(dev, id, rsp_buf,
                                    sizeof(rsp_buf));
                            sensor = (struct sensor_reading_get_rsp *) rsp_buf;
                            if (!rv ||
                                (sensor->state &
                                    IPMI_SENSOR_READING_STATE_MASK_UNAVAILABLE)
                               ) {
                                retry++;
                                msleep(CMD_RETRY_DELAY);
                                continue;
                            } else {
                                break;
                            }
                        }

                        if(retry > IPMI_ERR_RETRY_TIMES) {
                            dev_info(dev,
                                "Sensor(%s) reading unavailable and "
                                "retry failure\n", node->name);
                        }
                    }

                    node->cache[0] = sensor->state;
                    node->cache[1] = sensor->sensor_reading;
                } else if(node->sub_type == SDR_RECORD_TYPE_COMPACT_SENSOR) {
                    node->cache[0] = sensor->data1;
                    node->cache[1] = sensor->data2;
                } else {
                    break;
                }
                node->last_updated = new_jiffies;
                node->valid = DATA_VALID_TYPE_VALID;
            } else {
                memset(node->cache , 0, sizeof(node->cache));
                node->last_updated = new_jiffies;
                node->valid = DATA_VALID_TYPE_INVALID;
                dev_dbg(dev, "Sensor get fail code(%d) name(%s) id(%d) "
                    "sub_id(%d)\n",
                    rv, node->name, node->id, node->sub_id);
            }
            break;
        } else if(node->sub_id == SENSOR_SUB_ID_SENSOR_ID) {
            if(rv >= 0) {
                if(node->sub_type == SDR_RECORD_TYPE_FULL_SENSOR) {
                    node->cache[0] =
                        ((struct sdr_full_get_rsp *) sdr_data)->sensor_number;
                } else if(node->sub_type == SDR_RECORD_TYPE_COMPACT_SENSOR) {
                    node->cache[0] =
                        ((struct sdr_compact_get_rsp *)
                            sdr_data)->sensor_number;
                } else {
                    break;
                }
                node->last_updated = new_jiffies;
                node->valid = DATA_VALID_TYPE_PERMANENT_VALID;
            } else {
                memset(node->cache , 0, sizeof(node->cache));
                node->last_updated = new_jiffies;
                node->valid = DATA_VALID_TYPE_INVALID;
                dev_dbg(dev, "Sensor get fail code(%d) name(%s) id(%d) "
                    "sub_id(%d)\n",
                    rv, node->name, node->id, node->sub_id);
            }
            break;
        } else if(node->sub_id == SENSOR_SUB_ID_SENSOR_NAME) {
            if(rv >= 0) {
                if(node->sub_type == SDR_RECORD_TYPE_FULL_SENSOR) {
                    uint8_t *id_str =
                        ((struct sdr_full_get_rsp *) sdr_data)->id_str;
                    ssize_t len =
                        ((struct sdr_full_get_rsp *) sdr_data)->id_tlv &
                            IPMI_SDR_ID_TLV_LEN_MASK;

                    memcpy(node->cache, id_str, len);
                } else if(node->sub_type == SDR_RECORD_TYPE_COMPACT_SENSOR) {
                    uint8_t *id_str =
                        ((struct sdr_compact_get_rsp *) sdr_data)->id_str;
                    ssize_t len =
                        ((struct sdr_compact_get_rsp *) sdr_data)->id_tlv &
                            IPMI_SDR_ID_TLV_LEN_MASK;

                    memcpy(node->cache, id_str, len);
                } else {
                    break;
                }
                node->last_updated = new_jiffies;
                node->valid = DATA_VALID_TYPE_PERMANENT_VALID;
            } else {
                memset(node->cache , 0, sizeof(node->cache));
                node->last_updated = new_jiffies;
                node->valid = DATA_VALID_TYPE_INVALID;
                dev_dbg(dev, "Sensor get fail code(%d) name(%s) id(%d) "
                    "sub_id(%d)\n",
                    rv, node->name, node->id, node->sub_id);
            }
            break;
        } else if(node->sub_id == SENSOR_SUB_ID_SENSOR_TYPE) {
            if(rv >= 0) {
                struct sensor_type_get_rsp *sensor =
                    (struct sensor_type_get_rsp *) rsp_buf;
                node->cache[0] = sensor->sensor_type;
                node->last_updated = new_jiffies;
                node->valid = DATA_VALID_TYPE_VALID;
            } else {
                memset(node->cache , 0, sizeof(node->cache));
                node->last_updated = new_jiffies;
                node->valid = DATA_VALID_TYPE_INVALID;
                dev_dbg(dev, "Sensor type get fail code(%d) name(%s) id(%d) "
                    "sub_id(%d)\n",
                    rv, node->name, node->id, node->sub_id);
            }
            break;
        } else if(node->sub_id == SENSOR_SUB_ID_SENSOR_UNIT) {
            if(rv >= 0) {
                if(node->sub_type == SDR_RECORD_TYPE_FULL_SENSOR) {
                    node->cache[0] =
                        ((struct sdr_full_get_rsp *) sdr_data)->sensor_units2;
                } else if(node->sub_type == SDR_RECORD_TYPE_COMPACT_SENSOR) {
                    node->cache[0] =
                        ((struct sdr_compact_get_rsp *)
                            sdr_data)->sensor_units2;
                } else {
                    break;
                }
                node->last_updated = new_jiffies;
                node->valid = DATA_VALID_TYPE_PERMANENT_VALID;
            } else {
                memset(node->cache , 0, sizeof(node->cache));
                node->last_updated = new_jiffies;
                node->valid = DATA_VALID_TYPE_INVALID;
                dev_dbg(dev, "Sensor get fail code(%d) name(%s) id(%d) "
                    "sub_id(%d)\n",
                    rv, node->name, node->id, node->sub_id);
            }
            break;
        } else if(IS_SENSOR_THRESHOLD_SUB_ID(node->sub_id)) {
            if(rv >= 0) {
                struct sensor_thresholds_get_rsp *threshold =
                    (struct sensor_thresholds_get_rsp *) rsp_buf;
                if(node->sub_id == SENSOR_SUB_ID_UPPER_NONE_CRITICAL) {
                    node->cache[0] = threshold->mask;
                    node->cache[1] = threshold->upper_non_critical;
                    node->last_updated = new_jiffies;
                    node->valid = DATA_VALID_TYPE_VALID;
                }

                if(node->sub_id == SENSOR_SUB_ID_UPPER_CRITICAL) {
                    node->cache[0] = threshold->mask;
                    node->cache[1] = threshold->upper_critical;
                    node->last_updated = new_jiffies;
                    node->valid = DATA_VALID_TYPE_VALID;
                }

                if(node->sub_id == SENSOR_SUB_ID_UPPER_NONE_RECOVERABLE) {
                    node->cache[0] = threshold->mask;
                    node->cache[1] = threshold->upper_non_recoverable;
                    node->last_updated = new_jiffies;
                    node->valid = DATA_VALID_TYPE_VALID;
                }

                if(node->sub_id == SENSOR_SUB_ID_LOWER_NONE_CRITICAL) {
                    node->cache[0] = threshold->mask;
                    node->cache[1] = threshold->lower_non_critical;
                    node->last_updated = new_jiffies;
                    node->valid = DATA_VALID_TYPE_VALID;
                }

                if(node->sub_id == SENSOR_SUB_ID_LOWER_CRITICAL) {
                    node->cache[0] = threshold->mask;
                    node->cache[1] = threshold->lower_critical;
                    node->last_updated = new_jiffies;
                    node->valid = DATA_VALID_TYPE_VALID;
                }

                if(node->sub_id == SENSOR_SUB_ID_LOWER_NONE_RECOVERABLE) {
                    node->cache[0] = threshold->mask;
                    node->cache[1] = threshold->lower_non_recoverable;
                    node->last_updated = new_jiffies;
                    node->valid = DATA_VALID_TYPE_VALID;
                }

                if(node->sub_id == SENSOR_SUB_ID_TEMP_MIN) {
                    node->cache[0] = threshold->mask;
                    node->cache[1] = threshold->upper_non_critical;
                    node->last_updated = new_jiffies;
                    node->valid = DATA_VALID_TYPE_VALID;
                }

                if(node->sub_id == SENSOR_SUB_ID_TEMP_MAX) {
                    node->cache[0] = threshold->mask;
                    node->cache[1] = threshold->upper_non_critical;
                    node->last_updated = new_jiffies;
                    node->valid = DATA_VALID_TYPE_VALID;
                }

            } else {
                memset(node->cache , 0, sizeof(node->cache));
                node->last_updated = new_jiffies;
                node->valid = DATA_VALID_TYPE_INVALID;
                dev_dbg(dev, "Sensor threhold get fail code(%d) name(%s) "
                    "id(%d) sub_id(%d)\n",
                    rv, node->name, node->id, node->sub_id);
            }
        }
    }
    return rv;
}

static ssize_t class_sensor_show(struct device *dev,
    struct device_attribute *da, char *buf)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    struct bmc_dev_attr *bdev_attr  = to_bmc_dev_attr(da);
    uint8_t *sdr_data = NULL;
    int rv = 0;
    bool sdr_found = false;

    if(!bmc_data) {
        return -EINVAL;
    }

    mutex_lock(&bmc_data->access_lock);

    sdr_found = (_sdr_data_get(&bmc_data->ipmi_sdr_lists.list,
                    bdev_attr->sub_type, bdev_attr->id, &sdr_data) == 0);

    if(!sdr_found) {
        rv = -EBUSY;
        goto done;
    }

    if (bdev_attr->valid == DATA_VALID_TYPE_INVALID ||
        (bdev_attr->valid == DATA_VALID_TYPE_VALID &&
        time_after(jiffies, bdev_attr->last_updated + HZ * CACHE_TIMEOUT))) {
            _sensor_update(dev, bdev_attr->sub_type,
                bdev_attr->id, bdev_attr->sub_id);
    }

    dev_dbg(dev, "Sensor valid(%d) name(%s) id(0x%02X) sub_id(0x%02X) "
        "cache[0](0x%02X) cache[1](0x%02X)\n",
        bdev_attr->valid, bdev_attr->name, bdev_attr->id, bdev_attr->sub_id,
        bdev_attr->cache[0], bdev_attr->cache[1]);

    if(!bdev_attr->valid) {
        dev_dbg(dev, "Cannot get ipmi attr(%s) value\n", bdev_attr->name);
        goto ret_na;
    }

    if(bdev_attr->sub_type == SDR_RECORD_TYPE_FULL_SENSOR) {
        if(bdev_attr->sub_id == SENSOR_SUB_ID_READING) {
            if(!(bdev_attr->cache[0] &
                IPMI_SENSOR_READING_STATE_MASK_UNAVAILABLE)) {
                rv = _analog_parsing(dev,
                    buf, sdr_data, bdev_attr->cache, NULL);
            } else if(!(bdev_attr->cache[0] &
                IPMI_SENSOR_READING_STATE_MASK_SCANNING_DISABLED)) {
                rv = sprintf(buf, "NS\n");
            } else {
                goto ret_na;
            }
        } else if(bdev_attr->sub_id == SENSOR_SUB_ID_SENSOR_ID) {
            rv = sprintf(buf, "0x%02x\n", bdev_attr->cache[0]);
        } else if(bdev_attr->sub_id == SENSOR_SUB_ID_SENSOR_NAME) {
            rv = sprintf(buf, "%s\n", bdev_attr->cache);
        } else if(bdev_attr->sub_id == SENSOR_SUB_ID_SENSOR_TYPE) {
            rv = _sensor_type_parsing(dev, buf, bdev_attr->cache);
        } else if(bdev_attr->sub_id == SENSOR_SUB_ID_SENSOR_UNIT) {
            rv = _sensor_unit_parsing(dev, buf, bdev_attr->cache);
        } else if(bdev_attr->sub_id == SENSOR_SUB_ID_UPPER_NONE_CRITICAL &&
            (bdev_attr->cache[0] &
                IPMI_SENSOR_THRESHOLD_MASK_UPPER_NONE_CRITICAL)){
            rv = _analog_parsing(dev,
                    buf, sdr_data, bdev_attr->cache, NULL);
        } else if(bdev_attr->sub_id == SENSOR_SUB_ID_UPPER_CRITICAL &&
            (bdev_attr->cache[0] &
                IPMI_SENSOR_THRESHOLD_MASK_UPPER_CRITICAL)){
            rv = _analog_parsing(dev,
                    buf, sdr_data, bdev_attr->cache, NULL);
        } else if(bdev_attr->sub_id == SENSOR_SUB_ID_UPPER_NONE_RECOVERABLE &&
            (bdev_attr->cache[0] &
                IPMI_SENSOR_THRESHOLD_MASK_UPPER_NONE_RECOVERABLE)){
            rv = _analog_parsing(dev,
                    buf, sdr_data, bdev_attr->cache, NULL);
        } else if(bdev_attr->sub_id == SENSOR_SUB_ID_LOWER_NONE_CRITICAL &&
            (bdev_attr->cache[0] &
                IPMI_SENSOR_THRESHOLD_MASK_LOWER_NONE_CRITICAL)){
            rv = _analog_parsing(dev,
                    buf, sdr_data, bdev_attr->cache, NULL);
        } else if(bdev_attr->sub_id == SENSOR_SUB_ID_LOWER_CRITICAL &&
            (bdev_attr->cache[0] &
                IPMI_SENSOR_THRESHOLD_MASK_LOWER_CRITICAL)){
            rv = _analog_parsing(dev,
                    buf, sdr_data, bdev_attr->cache, NULL);
        } else if(bdev_attr->sub_id == SENSOR_SUB_ID_LOWER_NONE_RECOVERABLE &&
            (bdev_attr->cache[0] &
                IPMI_SENSOR_THRESHOLD_MASK_LOWER_NONE_RECOVERABLE)){
            rv = _analog_parsing(dev,
                    buf, sdr_data, bdev_attr->cache, NULL);
        } else if(bdev_attr->sub_id == SENSOR_SUB_ID_TEMP_MIN &&
            (bdev_attr->cache[0] &
                IPMI_SENSOR_THRESHOLD_MASK_UPPER_NONE_CRITICAL)){
            rv = _analog_parsing(dev,
                    buf, sdr_data, bdev_attr->cache, _adjust_result_temp_min);
        } else if(bdev_attr->sub_id == SENSOR_SUB_ID_TEMP_MAX &&
            (bdev_attr->cache[0] &
                IPMI_SENSOR_THRESHOLD_MASK_UPPER_NONE_CRITICAL)){
            rv = _analog_parsing(dev,
                    buf, sdr_data, bdev_attr->cache, NULL);
        } else {
            goto ret_na;
        }
    } else if(bdev_attr->sub_type == SDR_RECORD_TYPE_COMPACT_SENSOR) {
        if(bdev_attr->sub_id == SENSOR_SUB_ID_READING) {
            if(!(bdev_attr->cache[0] &
                IPMI_SENSOR_READING_STATE_MASK_UNAVAILABLE)) {
                rv = _discete_parsing(dev, buf, sdr_data, bdev_attr->cache);
            } else if(!(bdev_attr->cache[0] &
                IPMI_SENSOR_READING_STATE_MASK_SCANNING_DISABLED)) {
                rv = sprintf(buf, "NS\n");
            } else {
                goto ret_na;
            }
        } else if(bdev_attr->sub_id == SENSOR_SUB_ID_SENSOR_ID) {
            rv = sprintf(buf, "0x%02x\n", bdev_attr->cache[0]);
        } else if(bdev_attr->sub_id == SENSOR_SUB_ID_SENSOR_NAME) {
            rv = sprintf(buf, "%s\n", bdev_attr->cache);
        } else if(bdev_attr->sub_id == SENSOR_SUB_ID_SENSOR_TYPE) {
            rv = _sensor_type_parsing(dev, buf, bdev_attr->cache);
        } else if(bdev_attr->sub_id == SENSOR_SUB_ID_SENSOR_UNIT) {
            rv = _sensor_unit_parsing(dev, buf, bdev_attr->cache);
        } else {
            goto ret_na;
        }
    } else {
        goto ret_na;
    }

done:
    mutex_unlock(&bmc_data->access_lock);
    return rv;
ret_na:
    mutex_unlock(&bmc_data->access_lock);
    return sprintf(buf, "NA\n");
}

static int _fru_common_header_get(struct device * dev, uint8_t id,
    unsigned char *rsp_buf, int rsp_buf_size)
{
    int rv = 0;
    struct fru_data_read_req req = {
        .fru_dev_id = id,
        .offset_lsb = 0,
        .offset_msb = 0,
        .count = IPMI_FRU_COMM_HEADER_SIZE
    };

    if(rsp_buf == NULL)
        return -EINVAL;

    memset(rsp_buf, 0, rsp_buf_size);

    rv = ipmi_read_fru_data_cmd(dev, &req, rsp_buf, rsp_buf_size);

    if(rv < 0) {
        return -EACCES;
    }

    return rv;
}

static int _fru_product_info_header_get(struct device * dev, uint8_t id,
    unsigned char *rsp_buf, int rsp_buf_size, int product_off)
{
    int rv = 0;
    struct fru_data_read_req req = {
        .fru_dev_id = id,
        .offset_lsb = product_off & 0xff,
        .offset_msb = (product_off >> 8) & 0xff,
        .count = IPMI_FRU_PRODUCT_HEADER_SIZE
    };

    if(rsp_buf == NULL)
        return -EINVAL;

    memset(rsp_buf, 0, rsp_buf_size);

    rv = ipmi_read_fru_data_cmd(dev, &req, rsp_buf, rsp_buf_size);

    if(rv < 0) {
        return -EACCES;
    }

    return rv;
}

static int _fru_eeprom_size_get(struct device * dev, uint8_t id)
{
    unsigned char rsp_buf[IPMI_OPENIPMI_MAX_RSP_DATA_SIZE] = {0};
    int rv = 0;
    struct fru_inv_area_info_rsp *data = NULL;

    rv = ipmi_get_fru_inv_area_info_cmd(dev, id, rsp_buf, sizeof(rsp_buf));

    if(rv < 0) {
        return -EACCES;
    }

    data = ( struct fru_inv_area_info_rsp *) rsp_buf;
    return (data->area_size_msb << 8 | data->area_size_lsb);
}

static int _fru_product_info_data_get(struct device * dev, uint8_t id,
    unsigned char *eeprom_data, int eeprom_size, int *eeprom_len,
    int product_off, int area_data_size)
{
    int rv = 0;
    int off = 0;
    int loop_count = 0;
    int i = 0;
    int ee_len = 0;

    if(eeprom_data == NULL || eeprom_len == NULL)
        return -1;

    // Use a for loop to get all data.
    loop_count = (area_data_size % IPMI_FRU_MAX_RSP_BYTE) ?
                    (area_data_size / IPMI_FRU_MAX_RSP_BYTE) +1
                    :
                    (area_data_size / IPMI_FRU_MAX_RSP_BYTE);
    off = product_off;
    for(i=0; i<loop_count;i++) {
        unsigned char rsp_buf[IPMI_OPENIPMI_MAX_RSP_DATA_SIZE] = {0};
        struct fru_data_read_req req = {
            .fru_dev_id = id,
            .offset_lsb = off & 0xff,
            .offset_msb = (off >> 8) & 0xff,
            .count = ((area_data_size - (i+1) *IPMI_FRU_MAX_RSP_BYTE) > 0) ?
                IPMI_FRU_MAX_RSP_BYTE
                :
                (area_data_size - (i)*IPMI_FRU_MAX_RSP_BYTE),
        };

        rv = ipmi_read_fru_data_cmd(dev, &req, rsp_buf, sizeof(rsp_buf));

        if(rv < 0) {
            dev_dbg(dev, "Get project info area data fail. fru_id(%d) "
                "repeat index(%d)\n", id, i);
            *eeprom_len = 0;
            memset(eeprom_data, 0, eeprom_size);
            return -EIO;
        }

        dev_dbg(dev, "Dump product info area response content (request %d)\n",
            i);
        print_hex_dump_debug("    ",
            DUMP_PREFIX_OFFSET, 16, 1, rsp_buf, rv, false);

        if((ee_len + (rv - IPMI_FRU_DATA_RSP_HEADER_SIZE)) > eeprom_size)  {
            dev_dbg(dev, "EEPROM buffer size not enough\n");
            *eeprom_len = 0;
            memset(eeprom_data, 0, eeprom_size);
            return -EIO;
        }

        memcpy(eeprom_data+ee_len, rsp_buf+IPMI_FRU_DATA_RSP_HEADER_SIZE,
            rv - IPMI_FRU_DATA_RSP_HEADER_SIZE);
        ee_len += (rv - IPMI_FRU_DATA_RSP_HEADER_SIZE);
        off +=  (rv - IPMI_FRU_DATA_RSP_HEADER_SIZE);
    }

    *eeprom_len = ee_len;

    dev_dbg(dev, "Dump the content of the entire product info area (%d)\n",
        ee_len);
    print_hex_dump_debug("    ",
        DUMP_PREFIX_OFFSET, 16, 1, eeprom_data, ee_len, false);

    return 0;
}

static ssize_t _fru_update(struct device *dev, uint8_t id)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    struct bmc_dev_attr *node = NULL;
    int rv = 0;
    unsigned char rsp_buf[IPMI_OPENIPMI_MAX_RSP_DATA_SIZE] = {0};

    struct fru_common_hdr_rsp *common_hdr = NULL;
    struct fru_platfrom_info_hdr_rsp *platform_info_hdr = NULL;
    unsigned char *eeprom_data = NULL;
    unsigned long eeprom_size = 0;
    int pr_off = 0;
    int area_data_size = 0;
    int ee_len = 0;
    struct bmc_dev_attr *mf = NULL;
    struct bmc_dev_attr *pname = NULL;
    struct bmc_dev_attr *pn = NULL;
    struct bmc_dev_attr *pv = NULL;
    struct bmc_dev_attr *sn = NULL;
    int off = 0;
    int tlv_elem = 0;

    unsigned long new_jiffies = 0;

    if(!bmc_data) {
        rv = -EINVAL;
        goto done;
    }

    list_for_each_entry(node, &bmc_data->bmc_attr_lists.list, list) {
        if(node->type != ATTR_TYPE_SDR) {
            continue;
        }

        if(node->sub_type ==SDR_RECORD_TYPE_FRU_DEVICE_LOCATOR &&
            node->id == id) {
            if(node->sub_id == FRU_SUB_ID_MF) {
                mf = node;
                node->valid = DATA_VALID_TYPE_INVALID;
            } else if(node->sub_id == FRU_SUB_ID_PNAME) {
                pname = node;
                node->valid = DATA_VALID_TYPE_INVALID;
            } else if(node->sub_id == FRU_SUB_ID_PN) {
                pn = node;
                node->valid = DATA_VALID_TYPE_INVALID;
            } else if(node->sub_id == FRU_SUB_ID_PV) {
                pv = node;
                node->valid = DATA_VALID_TYPE_INVALID;
            } else if(node->sub_id == FRU_SUB_ID_SN) {
                sn = node;
                node->valid = DATA_VALID_TYPE_INVALID;
            }
        }
    }

    memset(rsp_buf, 0, sizeof(rsp_buf));
    rv = _fru_common_header_get(dev, id, rsp_buf, sizeof(rsp_buf));
    if( rv < 0) {
        dev_dbg(dev, "Get common header and product info area offset fail. "
            "fru_id(%d)\n", id);
        goto done;
    }

    common_hdr = (struct fru_common_hdr_rsp *) rsp_buf;
    pr_off = (common_hdr->product_info_off * IPMI_FRU_AREA_OFFSET_MULTIPLIER);

    if(pr_off == 0) {
        dev_dbg(dev, "Product info is not present\n");
        new_jiffies = jiffies;

        memset(mf->cache, 0, sizeof(mf->cache));
        mf->last_updated = new_jiffies;
        mf->valid = DATA_VALID_TYPE_INVALID;

        memset(pname->cache, 0, sizeof(pname->cache));
        pname->last_updated = new_jiffies;
        pname->valid = DATA_VALID_TYPE_INVALID;

        memset(pn->cache, 0, sizeof(pn->cache));
        pn->last_updated = new_jiffies;
        pn->valid = DATA_VALID_TYPE_INVALID;

        memset(pv->cache, 0, sizeof(pv->cache));
        pv->last_updated = new_jiffies;
        pv->valid = DATA_VALID_TYPE_INVALID;

        memset(sn->cache, 0, sizeof(sn->cache));
        sn->last_updated = new_jiffies;
        sn->valid = DATA_VALID_TYPE_INVALID;
        goto done;
    }

    memset(rsp_buf, 0, sizeof(rsp_buf));
    rv = _fru_product_info_header_get(dev, id, rsp_buf, sizeof(rsp_buf), pr_off);
    if(rv < 0) {
       dev_dbg(dev, "Get product info area header fail. fru_id(%d)\n", id);
        goto done;
    }

    platform_info_hdr = (struct fru_platfrom_info_hdr_rsp *) rsp_buf;
    area_data_size =
        (platform_info_hdr->length * IPMI_FRU_PRODUCT_DATA_LEN_MULTIPLIER);

    eeprom_size = _fru_eeprom_size_get(dev, id);
    if(eeprom_size < 0) {
        rv = -EACCES;
        goto done;
    }

    eeprom_data = devm_kzalloc(dev, sizeof(unsigned char) * eeprom_size
                    , GFP_KERNEL);
    if(!eeprom_data) {
        goto done;
    }

    rv = _fru_product_info_data_get(dev, id, eeprom_data, eeprom_size,
            &ee_len, pr_off, area_data_size);
    if(rv < 0) {
        dev_dbg(dev, "Get fru product info data fail. fru_id(%d)\n", id);
        goto free_eeprom;
    }

    /**
     * Parsing eeprom TLV.
     * ref:
     * platform-management-fru-document-rev-1-2, CH12, CH13
     */
    new_jiffies = jiffies;
    off = IPMI_FRU_PRODUCT_HEADER_SIZE + IPMI_FRU_PRODUCT_LAN_CODE_SIZE;
    tlv_elem = IPMI_FRU_PRODUCT_DATA_OFFSET_MF;
    while(off < ee_len) {
        int tlv_len = eeprom_data[off] & IPMI_FRU_TLV_LENGTH_MASK;
        int tlv_data_off = off+1;
        if(tlv_elem == IPMI_FRU_PRODUCT_DATA_OFFSET_MF) {
            if(mf) {
                memset(mf->cache, 0, sizeof(mf->cache));
                memcpy(mf->cache, eeprom_data+tlv_data_off, tlv_len);
                strim_inplace(mf->cache);
                mf->last_updated = new_jiffies;
                mf->valid = DATA_VALID_TYPE_VALID;
            }
        } else if(tlv_elem == IPMI_FRU_PRODUCT_DATA_OFFSET_PNAME) {
            if(pname) {
                memset(pname->cache, 0, sizeof(pname->cache));
                memcpy(pname->cache, eeprom_data+tlv_data_off, tlv_len);
                strim_inplace(pname->cache);
                pname->last_updated = new_jiffies;
                pname->valid = DATA_VALID_TYPE_VALID;
            }
        } else if(tlv_elem == IPMI_FRU_PRODUCT_DATA_OFFSET_PN) {
            if(pn) {
                memset(pn->cache, 0, sizeof(pn->cache));
                memcpy(pn->cache, eeprom_data+tlv_data_off, tlv_len);
                strim_inplace(pn->cache);
                pn->last_updated = new_jiffies;
                pn->valid = DATA_VALID_TYPE_VALID;
            }
        } else if(tlv_elem == IPMI_FRU_PRODUCT_DATA_OFFSET_PV) {
            if(pv) {
                memset(pv->cache, 0, sizeof(pv->cache));
                memcpy(pv->cache, eeprom_data+tlv_data_off, tlv_len);
                strim_inplace(pv->cache);
                pv->last_updated = new_jiffies;
                pv->valid = DATA_VALID_TYPE_VALID;
            }
        } else if(tlv_elem == IPMI_FRU_PRODUCT_DATA_OFFSET_SN) {
            if(sn) {
                memset(sn->cache, 0, sizeof(sn->cache));
                memcpy(sn->cache, eeprom_data+tlv_data_off, tlv_len);
                strim_inplace(sn->cache);
                sn->last_updated = new_jiffies;
                sn->valid = DATA_VALID_TYPE_VALID;
            }
        }

        off = tlv_data_off + tlv_len;
        tlv_elem++;
    }

free_eeprom:
    devm_kfree(dev, eeprom_data);

done:
    return rv;
}

static ssize_t class_fru_show(struct device *dev,
    struct device_attribute *da, char *buf)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    struct bmc_dev_attr *bdev_attr  = to_bmc_dev_attr(da);
    char *str = NULL;
    int rv = 0;

    if(!bmc_data) {
        return -EINVAL;
    }

    mutex_lock(&bmc_data->access_lock);

    if (bdev_attr->valid == DATA_VALID_TYPE_INVALID ||
        (bdev_attr->valid == DATA_VALID_TYPE_VALID &&
        time_after(jiffies, bdev_attr->last_updated + HZ * CACHE_TIMEOUT))) {
            _fru_update(dev, bdev_attr->id);
    }

    if(!bdev_attr->valid) {
        dev_dbg(dev, "Cannot get ipmi attr(%s) value\n", bdev_attr->name);
        goto ret_na;
    }

    str = bdev_attr->cache;
    rv = sprintf(buf,"%s\n", str);

    mutex_unlock(&bmc_data->access_lock);
    return rv;
ret_na:
    mutex_unlock(&bmc_data->access_lock);
    return sprintf(buf, "NA\n");

}

static ssize_t _firmware_version_update(struct device *dev)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    struct bmc_dev_attr *node = NULL;
    int rv;
    unsigned long new_jiffies = 0;
    unsigned char rsp_buf[IPMI_OPENIPMI_MAX_RSP_DATA_SIZE] = {0};
    struct dev_id_get_rsp *data = NULL;

    if(!bmc_data) {
        return -EINVAL;
    }

    rv = ipmi_get_device_id_cmd(dev, rsp_buf, sizeof(rsp_buf));

    new_jiffies = jiffies;

    list_for_each_entry(node, &bmc_data->bmc_attr_lists.list, list) {
        if(node->type != ATTR_TYPE_DEFAULT) {
            continue;
        }

        if(node->id == DEFAULT_ID_FIREMWARE_VERSION ||
            node->id == DEFAULT_ID_FIREMWARE_VERSION_LONG) {
            if(rv >= 0) {
                memset(node->cache, 0, sizeof(node->cache));
                data = (struct dev_id_get_rsp *) rsp_buf;
                if(data->fw_rev1 & IPMI_FIRMWARE_REVISION_DEVICE) {
                    node->valid = DATA_VALID_TYPE_INVALID;
                    dev_dbg(dev, "We don't support this device firmware mode, "
                        "name(%s) id(%d)\n", node->name, node->id);
                } else {
                    if(node->id == DEFAULT_ID_FIREMWARE_VERSION_LONG) {
                        snprintf(node->cache, sizeof(node->cache),
                            "%d.%x.%x.%x%x",
                            data->fw_rev1, data->fw_rev2,
                            data->aux_fw_ver_0,
                            data->aux_fw_ver_1, data->aux_fw_ver_2);
                    } else {
                        snprintf(node->cache, sizeof(node->cache), "%d.%x",
                            data->fw_rev1, data->fw_rev2);
                    }
                    node->last_updated = new_jiffies;
                    node->valid = DATA_VALID_TYPE_VALID;
                }
            }else{
                memset(node->cache , 0, sizeof(node->cache));
                node->valid = DATA_VALID_TYPE_INVALID;
                dev_dbg(dev, "Firmware version get fail code(%d) "
                    "name(%s) id(%d)\n",
                    rv,node->name, node->id);
            }
            break;
        }
    }

    return rv;
}

static ssize_t class_firmware_version_show(struct device *dev,
    struct device_attribute *da, char *buf)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    struct bmc_dev_attr *bdev_attr  = to_bmc_dev_attr(da);
    int rv = 0;

    if(!bmc_data) {
        return -EINVAL;
    }

    mutex_lock(&bmc_data->access_lock);

    if (bdev_attr->valid == DATA_VALID_TYPE_INVALID ||
        (bdev_attr->valid == DATA_VALID_TYPE_VALID &&
        time_after(jiffies, bdev_attr->last_updated + HZ * CACHE_TIMEOUT))) {
            _firmware_version_update(dev);
    }

    if(!bdev_attr->valid) {
        dev_dbg(dev, "Cannot get ipmi attr(%s) value\n", bdev_attr->name);
        goto ret_na;
    }

    rv = sprintf(buf, "%s\n", bdev_attr->cache);

    mutex_unlock(&bmc_data->access_lock);
    return rv;
ret_na:
    mutex_unlock(&bmc_data->access_lock);
    return sprintf(buf, "NA\n");
}

static ssize_t _watchdog_update(struct device *dev,
    uint8_t id, uint8_t sub_id)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    struct bmc_dev_attr *node = NULL;
    int rv;
    unsigned long new_jiffies = 0;
    unsigned char rsp_buf[IPMI_OPENIPMI_MAX_RSP_DATA_SIZE] = {0};
    struct wdg_timer_get_rsp *data = NULL;

    if(!bmc_data) {
        return -EINVAL;
    }

    rv = ipmi_get_watchdog_timer_cmd(dev, rsp_buf, sizeof(rsp_buf));

    new_jiffies = jiffies;

    list_for_each_entry(node, &bmc_data->bmc_attr_lists.list, list) {
        if(node->type != ATTR_TYPE_DEFAULT) {
            continue;
        }

        if(node->id == id && node->sub_id == sub_id) {
            if(rv >= 0) {
                memset(node->cache, 0, sizeof(node->cache));
                data = (struct wdg_timer_get_rsp *) rsp_buf;

                if(node->sub_id == WDG_SUB_ID_STATE ||
                    node->sub_id == WDG_SUB_ID_ENABLE) {
                    node->cache[0] = data->timer_use;
                    node->last_updated = new_jiffies;
                    node->valid = DATA_VALID_TYPE_VALID;
                } else if(node->sub_id == WDG_SUB_ID_TIMELEFT) {
                    data = (struct wdg_timer_get_rsp *) rsp_buf;
                    node->cache[0] = data->countdown_lsb;
                    node->cache[1] = data->countdown_msb;
                    node->last_updated = new_jiffies;
                    node->valid = DATA_VALID_TYPE_VALID;
                } else if(node->sub_id == WDG_SUB_ID_TIMEOUT) {
                    data = (struct wdg_timer_get_rsp *) rsp_buf;
                    node->cache[0] = data->init_lsb;
                    node->cache[1] = data->init_msb;
                    node->last_updated = new_jiffies;
                    node->valid = DATA_VALID_TYPE_VALID;
                }
            }else{
                memset(node->cache , 0, sizeof(node->cache));
                node->valid = DATA_VALID_TYPE_INVALID;
                dev_dbg(dev, "Sensor get fail code(%d) name(%s) id(%d)\n",
                    rv, node->name, node->id);
            }
            break;
        }
    }

    return rv;
}

static ssize_t class_watchdog_show(struct device *dev,
    struct device_attribute *da, char *buf)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    struct bmc_dev_attr *bdev_attr  = to_bmc_dev_attr(da);
    int rv = 0;

    if(!bmc_data) {
        return -EINVAL;
    }

    mutex_lock(&bmc_data->access_lock);

    _watchdog_update(dev, bdev_attr->id, bdev_attr->sub_id);

    if(!bdev_attr->valid) {
        dev_dbg(dev, "Cannot get ipmi attr(%s) value\n", bdev_attr->name);
        goto ret_na;
    }

    if(bdev_attr->sub_id == WDG_SUB_ID_STATE ||
        bdev_attr->sub_id == WDG_SUB_ID_ENABLE) {
        rv = sprintf(buf, "%d\n", (bdev_attr->cache[0] & 0x40) ? 1:0);
    } else if(bdev_attr->sub_id == WDG_SUB_ID_TIMELEFT ||
        bdev_attr->sub_id == WDG_SUB_ID_TIMEOUT) {
        rv = sprintf(buf, "%d\n", 
                ((bdev_attr->cache[1] << 8) |
                    bdev_attr->cache[0]) *
                    IPMI_WATCHDOG_TIMER_UNIT_MILLISECONDS / 1000);
    }


    mutex_unlock(&bmc_data->access_lock);
    return rv;
ret_na:
    mutex_unlock(&bmc_data->access_lock);
    return sprintf(buf, "NA\n");
}

static ssize_t class_watchdog_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    struct bmc_dev_attr *bdev_attr  = to_bmc_dev_attr(da);
    int rv = 0;
    int value = -1;
    int unit = -1;
    unsigned char rsp_buf[IPMI_OPENIPMI_MAX_RSP_DATA_SIZE] = {0};
    struct wdg_timer_set_req data = {
        .timer_use = 0x1,
        .timer_actions = 0x3,
        .pre_timeout = 0x1,
        .time_use_expir = 0x2
    };

    if(!bmc_data) {
        return -EINVAL;
    }

    mutex_lock(&bmc_data->access_lock);

    if (kstrtoint(buf, 0, &value) < 0) {
        rv = -EINVAL;
        goto done;
    }

    if(bdev_attr->sub_id == WDG_SUB_ID_ENABLE) {
        if (value == 1) {
            ipmi_reset_watchdog_timer_cmd(dev, rsp_buf, sizeof(rsp_buf));
        } else if(value == 0){
            struct wdg_timer_get_rsp *timer_config = NULL;
            rv = ipmi_get_watchdog_timer_cmd(dev, rsp_buf,
                    sizeof(rsp_buf));

            if(rv < 0) {
                dev_dbg(dev, "Fail to get ipmi timer config\n");
                goto done;
            }
            timer_config = (struct wdg_timer_get_rsp *) rsp_buf;
            data.init_lsb = timer_config->init_lsb;
            data.init_msb = timer_config->init_msb;
            ipmi_set_watchdog_timer_cmd(dev, &data, rsp_buf,
                sizeof(rsp_buf));
        } else {
            rv = -EINVAL;
            dev_dbg(dev, "Invalid value(0 or 1)\n");
            goto done;
        }
    } else if(bdev_attr->sub_id == WDG_SUB_ID_TIMEOUT) {
            if(value < 0 || value > 65535) {
                rv = -EINVAL;
                dev_dbg(dev, "Invalid time value(0-65535)\n");
                goto done;
            }
            unit = value *1000 / IPMI_WATCHDOG_TIMER_UNIT_MILLISECONDS;
            data.init_lsb = unit & 0xFF;
            data.init_msb = (unit & 0xFF00) >> 8;
            ipmi_set_watchdog_timer_cmd(dev, &data, rsp_buf, sizeof(rsp_buf));
    }

    mutex_unlock(&bmc_data->access_lock);
    return count;
done:
    mutex_unlock(&bmc_data->access_lock);
    return rv;
}

static ssize_t class_power_cycle_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    int rv = 0;
    int value = -1;
    unsigned char rsp_buf[IPMI_OPENIPMI_MAX_RSP_DATA_SIZE] = {0};

    if(!bmc_data) {
        return -EINVAL;
    }

    mutex_lock(&bmc_data->access_lock);

    if (kstrtoint(buf, 0, &value) < 0) {
        rv = -EINVAL;
        goto done;
    }

    if(value !=0 && value !=1) {
        rv = -EINVAL;
        dev_dbg(dev, "Invalid value(0 or 1)\n");
        goto done;
    }

    if(value == 1) {
        ipmi_chassis_control_cmd(dev, IPMI_CHASSIS_CONTROL_POWER_CYCLE,
            rsp_buf, sizeof(rsp_buf));
    }

    mutex_unlock(&bmc_data->access_lock);
    return count;
done:
    mutex_unlock(&bmc_data->access_lock);
    return rv;
}

static ssize_t class_oem_ufi_psu_reset_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    int rv = 0;
    int value = -1;
    unsigned char rsp_buf[IPMI_OPENIPMI_MAX_RSP_DATA_SIZE] = {0};

    if(!bmc_data) {
        return -EINVAL;
    }

    mutex_lock(&bmc_data->access_lock);
    if (kstrtoint(buf, 0, &value) < 0) {
        rv = -EINVAL;
        goto done;
    }

    if(value < 0 || value > 255) {
        rv = -EINVAL;
        dev_dbg(dev, "Invalid time value(0-255)\n");
        goto done;
    }

    if(value) {
        ipmi_oem_ufi_psu_reset_cmd(dev, value, rsp_buf, sizeof(rsp_buf));
    }

    mutex_unlock(&bmc_data->access_lock);
    return count;
done:
    mutex_unlock(&bmc_data->access_lock);
    return rv;
}

static int fru_info_get(struct device *dev, uint8_t id,
        struct fru_procd_info *rsp)
{
    int rv = 0;
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    struct bmc_dev_attr *node = NULL;
    struct bmc_dev_attr *mf = NULL;
    struct bmc_dev_attr *pname = NULL;
    struct bmc_dev_attr *pn = NULL;
    struct bmc_dev_attr *pv = NULL;
    struct bmc_dev_attr *sn = NULL;
    struct fru_procd_info info = {0};

    list_for_each_entry(node, &bmc_data->bmc_attr_lists.list, list) {
        if(node->type != ATTR_TYPE_SDR) {
            continue;
        }

        if(node->sub_type ==SDR_RECORD_TYPE_FRU_DEVICE_LOCATOR &&
            node->id == id) {
            if(node->sub_id == FRU_SUB_ID_MF) {
                mf = node;
            } else if(node->sub_id == FRU_SUB_ID_PNAME) {
                pname = node;
            } else if(node->sub_id == FRU_SUB_ID_PN) {
                pn = node;
            } else if(node->sub_id == FRU_SUB_ID_PV) {
                pv = node;
            } else if(node->sub_id == FRU_SUB_ID_SN) {
                sn = node;
            }
        }
    }

    if(mf) {
        if (mf->valid == DATA_VALID_TYPE_INVALID ||
            (mf->valid == DATA_VALID_TYPE_VALID &&
            time_after(jiffies, mf->last_updated + HZ * CACHE_TIMEOUT))) {
                _fru_update(dev, mf->id);
        }
    }

    info.mf = mf->cache;
    info.pname = pname->cache;
    info.pn = pn->cache;
    info.pv = pv->cache;

    *rsp = info;
    return rv;
}

static ssize_t class_udf_show(struct device *dev,
    struct device_attribute *da, char *buf)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    int rv = 0;
    unsigned char rsp_buf[IPMI_OPENIPMI_MAX_RSP_DATA_SIZE] = {0};
    struct fru_procd_info fru_info = {0};
    void *bufp = NULL;
    struct bmc_dev_attr *bdev_attr  = to_bmc_dev_attr(da);
    ssize_t (*show)(uint8_t id, uint8_t sub_id, char *buf,
        void *bufp, unsigned short rsp_buf_size) = NULL;

    if(!bmc_data) {
        return -EINVAL;
    }

    mutex_lock(&bmc_data->access_lock);

    if(bdev_attr->id > UDF_ID_UDF_INVALID && bdev_attr->id <UDF_ID_UDF_MAX) {
        if(!!ufi_bmc_show_store_udf[bdev_attr->id].show) {
            show = ufi_bmc_show_store_udf[bdev_attr->id].show;
        }
    }

    if(!!show) {
        if(bdev_attr->sub_type == UDF_SUB_TYPE_CMD) {
            rv = ipmi_udf_cmd(dev, bdev_attr->udf_cmd, bdev_attr->cmd_len,
                    rsp_buf, sizeof(rsp_buf));
            bufp = rsp_buf;
        }else if(bdev_attr->sub_type == UDF_SUB_TYPE_FRU){
            rv = fru_info_get(dev, bdev_attr->udf_cmd[0], &fru_info);
            bufp = &fru_info;
        } else {
            rv = sprintf(buf, "NA\n");
        }
        if(rv >= 0) {
            rv = show(bdev_attr->id, bdev_attr->sub_id, buf, bufp, rv);
        } else {
            rv = sprintf(buf, "NA\n");
        }
    } else {
        rv = sprintf(buf, "NA\n");
    }

    mutex_unlock(&bmc_data->access_lock);
    return rv;
}

static ssize_t class_udf_store(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    int rv = 0;
    uint8_t udf_cmd[IPMI_OPENIPMI_MAX_REQ_SIZE] = {0};
    uint8_t cmd_len = 0;
    unsigned char rsp_buf[IPMI_OPENIPMI_MAX_RSP_DATA_SIZE] = {0};
    struct bmc_dev_attr *bdev_attr  = to_bmc_dev_attr(da);
    ssize_t (*store)(uint8_t id, uint8_t sub_id, const char *buf, size_t count,
        uint8_t *udf_cmd, uint8_t *cmd_len) = NULL;

    if(!bmc_data || !bdev_attr) {
        return -EINVAL;
    }

    mutex_lock(&bmc_data->access_lock);

    if(bdev_attr->id > UDF_ID_UDF_INVALID && bdev_attr->id <UDF_ID_UDF_MAX) {
        if(!!ufi_bmc_show_store_udf[bdev_attr->id].store) {
            store = ufi_bmc_show_store_udf[bdev_attr->id].store;
        }
    }

    if(!!store) {
        rv = store(bdev_attr->id, bdev_attr->sub_id, buf, count,
                udf_cmd, &cmd_len);

        if(rv >= 0 && cmd_len < IPMI_OPENIPMI_MAX_REQ_SIZE) {
            ipmi_udf_cmd(dev, udf_cmd, cmd_len, rsp_buf, sizeof(rsp_buf));
        }
    } else {
        rv = count;
    }

    mutex_unlock(&bmc_data->access_lock);
    return rv;
}


static void _dump_to_block_buffer(uint8_t *buf, unsigned int len,
    char *blockbuf, size_t blockbuflen)
{
    int i = 0;
    int rowsize = 16;
    int linelen = 0;
    int remaining = len;
    /**
     * linebuf size is maximal length for one line.
     * 16 * 3 - maximum bytes per line, each printed into 2 chars + 1 for
     * separating space
     * 1 - terminating '\0'
     */
    unsigned char linebuf[16 * 3 + 1] = {0};
    int off = 0;

    for(i=0; i<len; i+=rowsize) {
        int len = 0;
        linelen = min(remaining, rowsize);
        remaining -= rowsize;
        hex_dump_to_buffer(buf+i, linelen, rowsize, 1, linebuf,
            sizeof(linebuf), false);
        if(off >= blockbuflen) {
            break;
        }
        len = snprintf(blockbuf+off, blockbuflen-off, "     %s\n", linebuf);
        off += len;
    }
}

static ssize_t _sdr_dump(struct device *dev,
    char *buf, loff_t off, size_t count)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    struct ipmi_sdr_list *node = NULL;
    struct ipmi_sdr_list *tmp_node = NULL;
    size_t ret_count = 0;
    size_t total_count = 0;

    if(!bmc_data) {
        return -EINVAL;
    }

    list_for_each_entry_safe(node, tmp_node,
        &bmc_data->ipmi_sdr_lists.list, list) {
        char block[256] = {0};

        memset(buf, 0, count);
        _dump_to_block_buffer(node->sdr_data, sizeof(node->sdr_data),
            block, sizeof(block));

        ret_count = snprintf(buf, count,"name(%s) sdr_type(%u) id(0x%02X)\n"
                        "    data:\n"
                        "%s\n",
                        node->name, node->sdr_type, node->id, block);

        total_count += min(ret_count, count);
        if(off < total_count) {
            return min(ret_count, count);
        }
    }

    memset(buf, 0, count);
    return (off+count > DUMP_TABLE_MAX_SIZE)? DUMP_TABLE_MAX_SIZE-count:count;

}

static ssize_t _sysfs_dump(struct device *dev,
    char *buf, loff_t off, size_t count)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    struct bmc_dev_attr *bdev_attr = NULL;
    struct bmc_dev_attr *tmp_bdev_attr = NULL;
    size_t ret_count = 0;
    size_t total_count = 0;

    if(!bmc_data) {
        return -EINVAL;
    }

    list_for_each_entry_safe(bdev_attr, tmp_bdev_attr,
        &bmc_data->bmc_attr_lists.list, list) {
        char block[256] = {0};

        memset(buf, 0, count);
        _dump_to_block_buffer(bdev_attr->cache, sizeof(bdev_attr->cache),
            block, sizeof(block));

        ret_count = snprintf(buf, count, "sysfs attr name(%s) "
                            "type(%d) sub_type(%d) id(0x%02X) sub_id(%d)\n"
                            "  valid(%d) last_updated(%lu) "
                            "is_probe(%d) addr(%p)\n"
                            "    cache:\n"
                            "%s\n",
                            bdev_attr->name,
                            bdev_attr->type, bdev_attr->sub_type,
                            bdev_attr->id, bdev_attr->sub_id,
                            bdev_attr->valid, bdev_attr->last_updated,
                            bdev_attr->is_probe, &bdev_attr->dev_attr,
                            block);

        total_count += min(ret_count, count);
        if(off < total_count) {
            return min(ret_count, count);
        }
    }

    memset(buf, 0, count);
    return (off+count > DUMP_TABLE_MAX_SIZE)? DUMP_TABLE_MAX_SIZE-count:count;

}

static ssize_t dump_table_read(struct file *filp, struct kobject *kobj,
        struct bin_attribute *attr,
        char *buf, loff_t off, size_t count)
{
    struct device *dev = kobj_to_dev(kobj);
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    ssize_t rv = 0;

    if(!bmc_data) {
        return -EINVAL;
    }

    mutex_lock(&bmc_data->access_lock);

    switch(bmc_data->dump_table_type) {
        case TABLE_TYPE_SDR:
            rv = _sdr_dump(dev, buf, off, count);
            break;
        case TABLE_TYPE_SYSFS:
            rv = _sysfs_dump(dev, buf, off, count);
            break;
        default:
            rv = _sdr_dump(dev, buf, off, count);
            break;
    }

    dev_dbg(dev, "Dump info offet(%lld) count(%lu) ret_count(%lu)\n",
        off, count, rv);

    mutex_unlock(&bmc_data->access_lock);
    return rv;
}

static ssize_t dump_table_write(struct file *filp, struct kobject *kobj,
        struct bin_attribute *attr,
        char *buf, loff_t off, size_t count)
{
    struct device *dev = kobj_to_dev(kobj);
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    ssize_t rv = count;

    if(!bmc_data) {
        return -EINVAL;
    }

    mutex_lock(&bmc_data->access_lock);
    if(sysfs_streq(buf,"sdr")) {
        bmc_data->dump_table_type = TABLE_TYPE_SDR;
    } else if(sysfs_streq(buf,"sysfs")) {
        bmc_data->dump_table_type = TABLE_TYPE_SYSFS;
    } else {
        rv = -EINVAL;
        goto done;
    }
done:
    mutex_unlock(&bmc_data->access_lock);
    return rv;
}

static ssize_t user_count_show(struct device *dev,
    struct device_attribute *da, char *buf)
{
    struct bmc_data_s *bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);
    int rv = 0;

    if(!bmc_data) {
        return -EINVAL;
    }

    mutex_lock(&bmc_data->access_lock);

    rv = sprintf(buf, "%ld\n", atomic_long_read(&bmc_data->user_count));
    mutex_unlock(&bmc_data->access_lock);
    return rv;
}

static int bmc_drv_probe(struct platform_device *pdev)
{
    struct bmc_data_s *bmc_data = NULL;
    int rv = 0;

    bmc_data = devm_kzalloc(&pdev->dev, sizeof(struct bmc_data_s),
                    GFP_KERNEL);

    if (!bmc_data) {
        return -ENOMEM;
    }

    platform_set_drvdata(pdev, bmc_data);

    bmc_data->grp.name = GROUP_NAME;
    bmc_data->grp.attrs = devm_kzalloc(&pdev->dev, sizeof(struct attribute *),
                            GFP_KERNEL);
    if (!bmc_data->grp.attrs) {
        ufi_bmc_pr_err(&pdev->dev,
            "Failed to allocate memory for bmc_data->grp.attrs.\n");
        rv = -ENOMEM;
        goto free_mem;
    }

    mutex_init(&bmc_data->access_lock);
    INIT_LIST_HEAD(&bmc_data->bmc_attr_lists.list);
    INIT_LIST_HEAD(&bmc_data->ipmi_sdr_lists.list);

    atomic_long_set(&bmc_data->user_count, 0);

    rv = sysfs_create_group(&pdev->dev.kobj, &bmc_data->grp);

    if(rv !=0) {
        ufi_bmc_pr_err(&pdev->dev, "Failed to create group.\n");
        goto done;
    }

    bmc_data->ipmi.is_init_intf = false;
    init_completion(&bmc_data->ipmi.read_complete);

    rv = ipmi_sensor_probe(&pdev->dev);
    if (rv) {
        ufi_bmc_pr_err(&pdev->dev, "Failed to probe bmc sensor\n");
    }

    rv = ipmi_default_probe(&pdev->dev);
    if (rv) {
        ufi_bmc_pr_err(&pdev->dev, "Failed to probe bmc default function\n");
    }

done:
    /**
     * Keep it returning 0 to enable dumping of the internal table.
     * If a failure is returned here, the probe will be considered a failure,
     * and all resources will be freed.
     */

    rv = device_create_file(&pdev->dev, _DEVICE_ATTR(user_count));
    if(rv < 0) {
        ufi_bmc_pr_err(&pdev->dev, "Failed to create sysfs(user_count).\n");
    }

    rv = sysfs_create_bin_file(&pdev->dev.kobj, _BIN_ATTR(dump_table));
    if(rv < 0) {
        ufi_bmc_pr_err(&pdev->dev, "Failed to create sysfs(dump_table).\n");
    }

    return 0;

free_mem:
    bmc_data->grp.attrs = NULL;
    devm_kfree(&pdev->dev, bmc_data);
    bmc_data = NULL;
    platform_set_drvdata(pdev, NULL);
    return rv;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
static int bmc_drv_remove(struct platform_device *pdev)
#else
static void bmc_drv_remove(struct platform_device *pdev)
#endif
{
    struct bmc_data_s *bmc_data =
        (struct bmc_data_s *) platform_get_drvdata(pdev);
    struct bmc_dev_attr *bmc_attr = NULL;
    struct bmc_dev_attr *tmp_bmc_attr = NULL;
    struct ipmi_sdr_list *node = NULL;
    struct ipmi_sdr_list *tmp_node = NULL;
    if(bmc_data) {
        device_remove_file(&pdev->dev, _DEVICE_ATTR(user_count));
        sysfs_remove_bin_file(&pdev->dev.kobj, _BIN_ATTR(dump_table));

        // release sysfs node
        list_for_each_entry_safe(bmc_attr, tmp_bmc_attr,
            &bmc_data->bmc_attr_lists.list, list) {
            sysfs_remove_file_from_group(&pdev->dev.kobj,
                &bmc_attr->dev_attr.attr, bmc_data->grp.name);
            list_del(&bmc_attr->list);
            devm_kfree(&pdev->dev, bmc_attr);
        }

        // release sdr table
        list_for_each_entry_safe(node, tmp_node,
            &bmc_data->ipmi_sdr_lists.list, list) {
            list_del(&node->list);
            devm_kfree(&pdev->dev, node);
        }

        if(bmc_data->ipmi.is_init_intf) {
            ipmi_destroy_user(bmc_data->ipmi.user);
            atomic_long_dec(&bmc_data->user_count);
        }

        devm_kfree(&pdev->dev, bmc_data->grp.attrs);
        sysfs_remove_group(&pdev->dev.kobj, &bmc_data->grp);
        devm_kfree(&pdev->dev, bmc_data);
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
    return 0;
#endif
}

static struct platform_driver ufi_bmc_drv = {
    .probe      = bmc_drv_probe,
    .remove     = __exit_p(bmc_drv_remove),
    .driver     = {
        .name   = DRVNAME,
    },
};

static void bmc_dev_release(struct device * dev)
{
    return;
}

struct platform_device ufi_bmc_dev = {
    .name           = DRVNAME,
    .id             = -1,
    .dev = {
        .release = bmc_dev_release,
    }
};
EXPORT_SYMBOL(ufi_bmc_dev);

int ufi_bmc_udf_create(struct device * dev, struct bmc_udf_node *table)
{
    int rv = 0;
    struct bmc_udf_node *node = NULL;
    struct bmc_data_s *bmc_data = NULL;

    if(!dev || !table) {
        return -EINVAL;
    }

    bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);

    if(!bmc_data) {
        return -ENOMEM;
    }

    for(node=&table[0]; node->id != UDF_ID_UDF_INVALID; node++) {
        int i = 0;
        const struct bmc_dev_attr_node conf_template = {
            .id = node->id,
            .udf_cmd = node->udf_cmd,
            .cmd_len = node->cmd_len,
            .type = ATTR_TYPE_UDF,
            .sub_type = node->sub_type,
            .is_probe = false,
        };

        if(node->id <= UDF_ID_UDF_INVALID  ||
            node->id >= UDF_ID_UDF_MAX) {
                rv = -EINVAL;
                goto free_node;
        }

        if(node->sub_type == UDF_SUB_TYPE_CMD) {
            if(node->cmd_len < IPMI_CMD_HEADER_SIZE ||
                node->cmd_len > IPMI_OPENIPMI_MAX_REQ_SIZE) {
                rv = -EINVAL;
                goto free_node;
            }
        } else if (node->sub_type == UDF_SUB_TYPE_FRU) {
            if(node->cmd_len < 1 ||
                node->cmd_len > IPMI_OPENIPMI_MAX_REQ_SIZE) {
                rv = -EINVAL;
                goto free_node;
            }
        } else {
            rv = -EINVAL;
            goto free_node;
        }

        for(i=0;i< UDF_SUB_ID_MAX && !!node->sub_attrs[i].name; i++) {
            struct bmc_dev_attr_node conf_node = conf_template;

            if(strlen(node->sub_attrs[i].name) <= 0) {
                continue;
            }

            conf_node.sub_id = i;
            conf_node.name = node->sub_attrs[i].name;
            conf_node.mode = node->sub_attrs[i].mode;
            rv = sysfs_create(dev, &conf_node);
            if(rv < 0){
                goto free_node;
            }
        }
    }

    return 0;

free_node:
    ufi_bmc_udf_destroy(dev);
    return rv;
}

EXPORT_SYMBOL(ufi_bmc_udf_create);

void ufi_bmc_udf_destroy(struct device * dev)
{
    struct bmc_dev_attr *bmc_attr = NULL;
    struct bmc_dev_attr *tmp_bmc_attr = NULL;
    struct bmc_data_s *bmc_data = NULL;
    if(!dev) {
        return;
    }

    bmc_data = (struct bmc_data_s *) dev_get_drvdata(dev);

    if(!bmc_data) {
        return;
    }

    mutex_lock(&bmc_data->access_lock);

    list_for_each_entry_safe(bmc_attr, tmp_bmc_attr,
        &bmc_data->bmc_attr_lists.list, list) {

        if(bmc_attr->type == ATTR_TYPE_UDF) {
            sysfs_remove_file_from_group(&dev->kobj,
                &bmc_attr->dev_attr.attr, bmc_data->grp.name);
            list_del(&bmc_attr->list);
            devm_kfree(dev, bmc_attr);
        }
    }

    mutex_unlock(&bmc_data->access_lock);
}

EXPORT_SYMBOL(ufi_bmc_udf_destroy);

int bmc_init(void)
{
    int err = 0;

    err = platform_driver_register(&ufi_bmc_drv);
    if (err) {
        pr_err("%s %s(#%d): Platform_driver_register failed(%d)\n",
            DRVNAME, __func__, __LINE__, err);

        return err;
    }

    err = platform_device_register(&ufi_bmc_dev);
    if (err) {
        pr_err("%s %s(#%d): Platform_device_register failed(%d)\n",
            DRVNAME, __func__, __LINE__, err);
        platform_driver_unregister(&ufi_bmc_drv);
        return err;
    }

    return err;
}

void bmc_exit(void)
{
    platform_driver_unregister(&ufi_bmc_drv);
    platform_device_unregister(&ufi_bmc_dev);
}

MODULE_AUTHOR("Nonodark Huang <nonodark.huang@ufispace.com>");
MODULE_DESCRIPTION("x86_64_ufispace_bmc driver");
MODULE_VERSION("0.0.1");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: ipmi_si");

module_init(bmc_init);
module_exit(bmc_exit);
