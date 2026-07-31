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
 * SFP Platform Implementation Interface.
 *
 ***********************************************************/
#include <unistd.h>
#include <fcntl.h>
#include <onlp/platformi/sfpi.h>
#include "platform_lib.h"

#define ALL_PORTS             -1

#define QSFP_NUM              6
#define QSFPDD_NUM            0
#define SFP_NUM               96
#define MGMT_NUM              2
#define OSFP_NUM              0
#define XSFPX_NUM             (QSFP_NUM+QSFPDD_NUM+OSFP_NUM)
#define PORT_NUM              (XSFPX_NUM+SFP_NUM+MGMT_NUM)

#define SYSFS_EEPROM        "eeprom"
#define SYSFS_DEV_CLASS     "dev_class"
#define EEPROM_ADDR         (0x50)
#define EEPROM_SFP_DOM_ADDR (0x51)
#define TX_DIS_INPUT_MAX (0xff) /* for input value validation only */
#define SFF8636_EEPROM_OFFSET_TXDIS (0x56)
#define SFF8636_EEPROM_TX_DIS (0x0f) /* txdis valid bit(bit0-bit3), xxxx 1111 */
#define SFF8636_EEPROM_TX_EN (0x0)

//CMIS TX Disable
#define CMIS_PAGE_SIZE                        (128)
#define CMIS_PAGE_SUPPORTED_CTRL_ADV          (1)
#define CMIS_PAGE_TX_DIS                      (16)
#define CMIS_OFFSET_REVISION                  (1)
#define CMIS_OFFSET_MEMORY_MODEL              (2)
#define CMIS_OFFSET_TX_DIS                    (130)
#define CMIS_OFFSET_SUPPORTED_CTRL_ADV        (155)
#define CMIS_MASK_MEMORY_MODEL                (0b10000000)
#define CMIS_MASK_TX_DIS_ADV                  (0b00000010)
#define CMIS_VAL_TX_DIS                       (0xff)
#define CMIS_VAL_TX_EN                        (0x0)
#define CMIS_VAL_MEMORY_MODEL_PAGED           (0)
#define CMIS_VAL_TX_DIS_SUPPORTED             (1)
#define CMIS_VAL_VERSION_MIN                  (0x30)
#define CMIS_VAL_VERSION_MAX                  (0x5F)
#define CMIS_SEEK_TX_DIS_ADV                  (CMIS_PAGE_SIZE * CMIS_PAGE_SUPPORTED_CTRL_ADV + CMIS_OFFSET_SUPPORTED_CTRL_ADV)
#define CMIS_SEEK_TX_DIS                      (CMIS_PAGE_SIZE * CMIS_PAGE_TX_DIS + CMIS_OFFSET_TX_DIS)

#define BIT_000_001_010_000   0x50
#define BIT_100_101_110_100   0x974

typedef enum port_type_e {
    TYPE_SFP = 0,
    TYPE_QSFP,
    TYPE_QSFPDD,
    TYPE_MGMT,
    TYPE_OSFP,
    TYPE_UNNKOW,
    TYPE_MAX,
} port_type_t;

typedef enum op_type_e {
    OP_SYSFS = 0,
    OP_CMIS,
    OP_8636,
    OP_UNNKOW,
    OP_MAX,
} op_type_t;

typedef struct {
    int key;  //[module_type]
    int value;  // [dev_class]
} PortTypeDictEntry;

PortTypeDictEntry port_type_dict[] = {
    {0x03, 2},// 'SFP/SFP+/SFP28'
    {0x0B, 2},// 'DWDM-SFP/SFP+'
    {0x0C, 1},// 'QSFP'
    {0x0D, 1},// 'QSFP+'
    {0x11, 1},// 'QSFP28'
    {0x18, 3},// 'QSFP-DD Double Density 8x (INF-8628)'
    {0x19, 3},// 'OSFP 8x Pluggable Transceiver'
    {0x1E, 3},// 'QSFP+ or later with CMIS spec'
    {0x1F, 3},// 'SFP-DD Double Density 2X Pluggable Transceiver with CMIS spec'
};

#define PORT_TYPE_DICT_SIZE (sizeof(port_type_dict) / sizeof(PortTypeDictEntry))

typedef struct
{
    int abs;
    int lpmode;
    int reset;
    int rxlos;
    int txfault;
    int txdis;
    int eeprom_bus;
    int port_type;
    unsigned int cpld_bit;
} port_attr_t;

typedef enum cpld_attr_idx_e {
    ABS_0_7 = 0,
    ABS_8_15,
    ABS_16_23,
    ABS_24_31,
    ABS_32_39,
    ABS_40_43,
    ABS_44_47,
    ABS_48_55,
    ABS_56_63,
    ABS_64_71,
    ABS_72_79,
    ABS_80_87,
    ABS_88_91,
    ABS_92_95,
    ABS_96_101,
    ABS_102_103,
    RXLOS_0_7,
    RXLOS_8_15,
    RXLOS_16_23,
    RXLOS_24_31,
    RXLOS_32_39,
    RXLOS_40_43,
    RXLOS_44_47,
    RXLOS_48_55,
    RXLOS_56_63,
    RXLOS_64_71,
    RXLOS_72_79,
    RXLOS_80_87,
    RXLOS_88_91,
    RXLOS_92_95,
    RXLOS_102_103,
    TXFLT_0_7,
    TXFLT_8_15,
    TXFLT_16_23,
    TXFLT_24_31,
    TXFLT_32_39,
    TXFLT_40_43,
    TXFLT_44_47,
    TXFLT_48_55,
    TXFLT_56_63,
    TXFLT_64_71,
    TXFLT_72_79,
    TXFLT_80_87,
    TXFLT_88_91,
    TXFLT_92_95,
    TXFLT_102_103,
    RESET_96_101,
    LPMODE_96_101,
    TXDIS_0_7,
    TXDIS_8_15,
    TXDIS_16_23,
    TXDIS_24_31,
    TXDIS_32_39,
    TXDIS_40_43,
    TXDIS_44_47,
    TXDIS_48_55,
    TXDIS_56_63,
    TXDIS_64_71,
    TXDIS_72_79,
    TXDIS_80_87,
    TXDIS_88_91,
    TXDIS_92_95,
    TXDIS_102_103,
    CPLD_NONE,
} cpld_attr_idx_t;

static const port_attr_t port_attr[] = {
/*
 *  TYPE_MGMT cpld_bit is bit stream
 *  Def: txdis txflt rxlos abs
 *   0b  xxx   xxx   xxx   xxx
 *
 *  Ex:  0x50 = 0b 000 001 010 000
 *       abs  : bit 0
 *       rxlos: bit 2
 *       txflt: bit 1
 *       txdis: bit 0
 */

//  port    abs          lpmode               reset              rxlos          txfault          txdis           eeprom  type       bit
    [0]   ={ABS_0_7    , CPLD_NONE          , CPLD_NONE        , RXLOS_0_7    , TXFLT_0_7      , TXDIS_0_7     , 25    , TYPE_SFP , 0 },
    [1]   ={ABS_0_7    , CPLD_NONE          , CPLD_NONE        , RXLOS_0_7    , TXFLT_0_7      , TXDIS_0_7     , 26    , TYPE_SFP , 1 },
    [2]   ={ABS_0_7    , CPLD_NONE          , CPLD_NONE        , RXLOS_0_7    , TXFLT_0_7      , TXDIS_0_7     , 27    , TYPE_SFP , 2 },
    [3]   ={ABS_0_7    , CPLD_NONE          , CPLD_NONE        , RXLOS_0_7    , TXFLT_0_7      , TXDIS_0_7     , 28    , TYPE_SFP , 3 },
    [4]   ={ABS_0_7    , CPLD_NONE          , CPLD_NONE        , RXLOS_0_7    , TXFLT_0_7      , TXDIS_0_7     , 29    , TYPE_SFP , 4 },
    [5]   ={ABS_0_7    , CPLD_NONE          , CPLD_NONE        , RXLOS_0_7    , TXFLT_0_7      , TXDIS_0_7     , 30    , TYPE_SFP , 5 },
    [6]   ={ABS_0_7    , CPLD_NONE          , CPLD_NONE        , RXLOS_0_7    , TXFLT_0_7      , TXDIS_0_7     , 31    , TYPE_SFP , 6 },
    [7]   ={ABS_0_7    , CPLD_NONE          , CPLD_NONE        , RXLOS_0_7    , TXFLT_0_7      , TXDIS_0_7     , 32    , TYPE_SFP , 7 },
    [8]   ={ABS_8_15   , CPLD_NONE          , CPLD_NONE        , RXLOS_8_15   , TXFLT_8_15     , TXDIS_8_15    , 33    , TYPE_SFP , 0 },
    [9]   ={ABS_8_15   , CPLD_NONE          , CPLD_NONE        , RXLOS_8_15   , TXFLT_8_15     , TXDIS_8_15    , 34    , TYPE_SFP , 1 },
    [10]  ={ABS_8_15   , CPLD_NONE          , CPLD_NONE        , RXLOS_8_15   , TXFLT_8_15     , TXDIS_8_15    , 35    , TYPE_SFP , 2 },
    [11]  ={ABS_8_15   , CPLD_NONE          , CPLD_NONE        , RXLOS_8_15   , TXFLT_8_15     , TXDIS_8_15    , 36    , TYPE_SFP , 3 },
    [12]  ={ABS_8_15   , CPLD_NONE          , CPLD_NONE        , RXLOS_8_15   , TXFLT_8_15     , TXDIS_8_15    , 37    , TYPE_SFP , 4 },
    [13]  ={ABS_8_15   , CPLD_NONE          , CPLD_NONE        , RXLOS_8_15   , TXFLT_8_15     , TXDIS_8_15    , 38    , TYPE_SFP , 5 },
    [14]  ={ABS_8_15   , CPLD_NONE          , CPLD_NONE        , RXLOS_8_15   , TXFLT_8_15     , TXDIS_8_15    , 39    , TYPE_SFP , 6 },
    [15]  ={ABS_8_15   , CPLD_NONE          , CPLD_NONE        , RXLOS_8_15   , TXFLT_8_15     , TXDIS_8_15    , 40    , TYPE_SFP , 7 },
    [16]  ={ABS_16_23  , CPLD_NONE          , CPLD_NONE        , RXLOS_16_23  , TXFLT_16_23    , TXDIS_16_23   , 41    , TYPE_SFP , 0 },
    [17]  ={ABS_16_23  , CPLD_NONE          , CPLD_NONE        , RXLOS_16_23  , TXFLT_16_23    , TXDIS_16_23   , 42    , TYPE_SFP , 1 },
    [18]  ={ABS_16_23  , CPLD_NONE          , CPLD_NONE        , RXLOS_16_23  , TXFLT_16_23    , TXDIS_16_23   , 43    , TYPE_SFP , 2 },
    [19]  ={ABS_16_23  , CPLD_NONE          , CPLD_NONE        , RXLOS_16_23  , TXFLT_16_23    , TXDIS_16_23   , 44    , TYPE_SFP , 3 },
    [20]  ={ABS_16_23  , CPLD_NONE          , CPLD_NONE        , RXLOS_16_23  , TXFLT_16_23    , TXDIS_16_23   , 45    , TYPE_SFP , 4 },
    [21]  ={ABS_16_23  , CPLD_NONE          , CPLD_NONE        , RXLOS_16_23  , TXFLT_16_23    , TXDIS_16_23   , 46    , TYPE_SFP , 5 },
    [22]  ={ABS_16_23  , CPLD_NONE          , CPLD_NONE        , RXLOS_16_23  , TXFLT_16_23    , TXDIS_16_23   , 47    , TYPE_SFP , 6 },
    [23]  ={ABS_16_23  , CPLD_NONE          , CPLD_NONE        , RXLOS_16_23  , TXFLT_16_23    , TXDIS_16_23   , 48    , TYPE_SFP , 7 },
    [24]  ={ABS_24_31  , CPLD_NONE          , CPLD_NONE        , RXLOS_24_31  , TXFLT_24_31    , TXDIS_24_31   , 49    , TYPE_SFP , 0 },
    [25]  ={ABS_24_31  , CPLD_NONE          , CPLD_NONE        , RXLOS_24_31  , TXFLT_24_31    , TXDIS_24_31   , 50    , TYPE_SFP , 1 },
    [26]  ={ABS_24_31  , CPLD_NONE          , CPLD_NONE        , RXLOS_24_31  , TXFLT_24_31    , TXDIS_24_31   , 51    , TYPE_SFP , 2 },
    [27]  ={ABS_24_31  , CPLD_NONE          , CPLD_NONE        , RXLOS_24_31  , TXFLT_24_31    , TXDIS_24_31   , 52    , TYPE_SFP , 3 },
    [28]  ={ABS_24_31  , CPLD_NONE          , CPLD_NONE        , RXLOS_24_31  , TXFLT_24_31    , TXDIS_24_31   , 53    , TYPE_SFP , 4 },
    [29]  ={ABS_24_31  , CPLD_NONE          , CPLD_NONE        , RXLOS_24_31  , TXFLT_24_31    , TXDIS_24_31   , 54    , TYPE_SFP , 5 },
    [30]  ={ABS_24_31  , CPLD_NONE          , CPLD_NONE        , RXLOS_24_31  , TXFLT_24_31    , TXDIS_24_31   , 55    , TYPE_SFP , 6 },
    [31]  ={ABS_24_31  , CPLD_NONE          , CPLD_NONE        , RXLOS_24_31  , TXFLT_24_31    , TXDIS_24_31   , 56    , TYPE_SFP , 7 },
    [32]  ={ABS_32_39  , CPLD_NONE          , CPLD_NONE        , RXLOS_32_39  , TXFLT_32_39    , TXDIS_32_39   , 57    , TYPE_SFP , 0 },
    [33]  ={ABS_32_39  , CPLD_NONE          , CPLD_NONE        , RXLOS_32_39  , TXFLT_32_39    , TXDIS_32_39   , 58    , TYPE_SFP , 1 },
    [34]  ={ABS_32_39  , CPLD_NONE          , CPLD_NONE        , RXLOS_32_39  , TXFLT_32_39    , TXDIS_32_39   , 59    , TYPE_SFP , 2 },
    [35]  ={ABS_32_39  , CPLD_NONE          , CPLD_NONE        , RXLOS_32_39  , TXFLT_32_39    , TXDIS_32_39   , 60    , TYPE_SFP , 3 },
    [36]  ={ABS_32_39  , CPLD_NONE          , CPLD_NONE        , RXLOS_32_39  , TXFLT_32_39    , TXDIS_32_39   , 61    , TYPE_SFP , 4 },
    [37]  ={ABS_32_39  , CPLD_NONE          , CPLD_NONE        , RXLOS_32_39  , TXFLT_32_39    , TXDIS_32_39   , 62    , TYPE_SFP , 5 },
    [38]  ={ABS_32_39  , CPLD_NONE          , CPLD_NONE        , RXLOS_32_39  , TXFLT_32_39    , TXDIS_32_39   , 63    , TYPE_SFP , 6 },
    [39]  ={ABS_32_39  , CPLD_NONE          , CPLD_NONE        , RXLOS_32_39  , TXFLT_32_39    , TXDIS_32_39   , 64    , TYPE_SFP , 7 },
    [40]  ={ABS_40_43  , CPLD_NONE          , CPLD_NONE        , RXLOS_40_43  , TXFLT_40_43    , TXDIS_40_43   , 65    , TYPE_SFP , 0 },
    [41]  ={ABS_40_43  , CPLD_NONE          , CPLD_NONE        , RXLOS_40_43  , TXFLT_40_43    , TXDIS_40_43   , 66    , TYPE_SFP , 1 },
    [42]  ={ABS_40_43  , CPLD_NONE          , CPLD_NONE        , RXLOS_40_43  , TXFLT_40_43    , TXDIS_40_43   , 67    , TYPE_SFP , 2 },
    [43]  ={ABS_40_43  , CPLD_NONE          , CPLD_NONE        , RXLOS_40_43  , TXFLT_40_43    , TXDIS_40_43   , 68    , TYPE_SFP , 3 },
    [44]  ={ABS_44_47  , CPLD_NONE          , CPLD_NONE        , RXLOS_44_47  , TXFLT_44_47    , TXDIS_44_47   , 69    , TYPE_SFP , 0 },
    [45]  ={ABS_44_47  , CPLD_NONE          , CPLD_NONE        , RXLOS_44_47  , TXFLT_44_47    , TXDIS_44_47   , 70    , TYPE_SFP , 1 },
    [46]  ={ABS_44_47  , CPLD_NONE          , CPLD_NONE        , RXLOS_44_47  , TXFLT_44_47    , TXDIS_44_47   , 71    , TYPE_SFP , 2 },
    [47]  ={ABS_44_47  , CPLD_NONE          , CPLD_NONE        , RXLOS_44_47  , TXFLT_44_47    , TXDIS_44_47   , 72    , TYPE_SFP , 3 },
    [48]  ={ABS_48_55  , CPLD_NONE          , CPLD_NONE        , RXLOS_48_55  , TXFLT_48_55    , TXDIS_48_55   , 73    , TYPE_SFP , 0 },
    [49]  ={ABS_48_55  , CPLD_NONE          , CPLD_NONE        , RXLOS_48_55  , TXFLT_48_55    , TXDIS_48_55   , 74    , TYPE_SFP , 1 },
    [50]  ={ABS_48_55  , CPLD_NONE          , CPLD_NONE        , RXLOS_48_55  , TXFLT_48_55    , TXDIS_48_55   , 75    , TYPE_SFP , 2 },
    [51]  ={ABS_48_55  , CPLD_NONE          , CPLD_NONE        , RXLOS_48_55  , TXFLT_48_55    , TXDIS_48_55   , 76    , TYPE_SFP , 3 },
    [52]  ={ABS_48_55  , CPLD_NONE          , CPLD_NONE        , RXLOS_48_55  , TXFLT_48_55    , TXDIS_48_55   , 77    , TYPE_SFP , 4 },
    [53]  ={ABS_48_55  , CPLD_NONE          , CPLD_NONE        , RXLOS_48_55  , TXFLT_48_55    , TXDIS_48_55   , 78    , TYPE_SFP , 5 },
    [54]  ={ABS_48_55  , CPLD_NONE          , CPLD_NONE        , RXLOS_48_55  , TXFLT_48_55    , TXDIS_48_55   , 79    , TYPE_SFP , 6 },
    [55]  ={ABS_48_55  , CPLD_NONE          , CPLD_NONE        , RXLOS_48_55  , TXFLT_48_55    , TXDIS_48_55   , 80    , TYPE_SFP , 7 },
    [56]  ={ABS_56_63  , CPLD_NONE          , CPLD_NONE        , RXLOS_56_63  , TXFLT_56_63    , TXDIS_56_63   , 81    , TYPE_SFP , 0 },
    [57]  ={ABS_56_63  , CPLD_NONE          , CPLD_NONE        , RXLOS_56_63  , TXFLT_56_63    , TXDIS_56_63   , 82    , TYPE_SFP , 1 },
    [58]  ={ABS_56_63  , CPLD_NONE          , CPLD_NONE        , RXLOS_56_63  , TXFLT_56_63    , TXDIS_56_63   , 83    , TYPE_SFP , 2 },
    [59]  ={ABS_56_63  , CPLD_NONE          , CPLD_NONE        , RXLOS_56_63  , TXFLT_56_63    , TXDIS_56_63   , 84    , TYPE_SFP , 3 },
    [60]  ={ABS_56_63  , CPLD_NONE          , CPLD_NONE        , RXLOS_56_63  , TXFLT_56_63    , TXDIS_56_63   , 85    , TYPE_SFP , 4 },
    [61]  ={ABS_56_63  , CPLD_NONE          , CPLD_NONE        , RXLOS_56_63  , TXFLT_56_63    , TXDIS_56_63   , 86    , TYPE_SFP , 5 },
    [62]  ={ABS_56_63  , CPLD_NONE          , CPLD_NONE        , RXLOS_56_63  , TXFLT_56_63    , TXDIS_56_63   , 87    , TYPE_SFP , 6 },
    [63]  ={ABS_56_63  , CPLD_NONE          , CPLD_NONE        , RXLOS_56_63  , TXFLT_56_63    , TXDIS_56_63   , 88    , TYPE_SFP , 7 },
    [64]  ={ABS_64_71  , CPLD_NONE          , CPLD_NONE        , RXLOS_64_71  , TXFLT_64_71    , TXDIS_64_71   , 89    , TYPE_SFP , 0 },
    [65]  ={ABS_64_71  , CPLD_NONE          , CPLD_NONE        , RXLOS_64_71  , TXFLT_64_71    , TXDIS_64_71   , 90    , TYPE_SFP , 1 },
    [66]  ={ABS_64_71  , CPLD_NONE          , CPLD_NONE        , RXLOS_64_71  , TXFLT_64_71    , TXDIS_64_71   , 91    , TYPE_SFP , 2 },
    [67]  ={ABS_64_71  , CPLD_NONE          , CPLD_NONE        , RXLOS_64_71  , TXFLT_64_71    , TXDIS_64_71   , 92    , TYPE_SFP , 3 },
    [68]  ={ABS_64_71  , CPLD_NONE          , CPLD_NONE        , RXLOS_64_71  , TXFLT_64_71    , TXDIS_64_71   , 93    , TYPE_SFP , 4 },
    [69]  ={ABS_64_71  , CPLD_NONE          , CPLD_NONE        , RXLOS_64_71  , TXFLT_64_71    , TXDIS_64_71   , 94    , TYPE_SFP , 5 },
    [70]  ={ABS_64_71  , CPLD_NONE          , CPLD_NONE        , RXLOS_64_71  , TXFLT_64_71    , TXDIS_64_71   , 95    , TYPE_SFP , 6 },
    [71]  ={ABS_64_71  , CPLD_NONE          , CPLD_NONE        , RXLOS_64_71  , TXFLT_64_71    , TXDIS_64_71   , 96    , TYPE_SFP , 7 },
    [72]  ={ABS_72_79  , CPLD_NONE          , CPLD_NONE        , RXLOS_72_79  , TXFLT_72_79    , TXDIS_72_79   , 97    , TYPE_SFP , 0 },
    [73]  ={ABS_72_79  , CPLD_NONE          , CPLD_NONE        , RXLOS_72_79  , TXFLT_72_79    , TXDIS_72_79   , 98    , TYPE_SFP , 1 },
    [74]  ={ABS_72_79  , CPLD_NONE          , CPLD_NONE        , RXLOS_72_79  , TXFLT_72_79    , TXDIS_72_79   , 99    , TYPE_SFP , 2 },
    [75]  ={ABS_72_79  , CPLD_NONE          , CPLD_NONE        , RXLOS_72_79  , TXFLT_72_79    , TXDIS_72_79   , 100   , TYPE_SFP , 3 },
    [76]  ={ABS_72_79  , CPLD_NONE          , CPLD_NONE        , RXLOS_72_79  , TXFLT_72_79    , TXDIS_72_79   , 101   , TYPE_SFP , 4 },
    [77]  ={ABS_72_79  , CPLD_NONE          , CPLD_NONE        , RXLOS_72_79  , TXFLT_72_79    , TXDIS_72_79   , 102   , TYPE_SFP , 5 },
    [78]  ={ABS_72_79  , CPLD_NONE          , CPLD_NONE        , RXLOS_72_79  , TXFLT_72_79    , TXDIS_72_79   , 103   , TYPE_SFP , 6 },
    [79]  ={ABS_72_79  , CPLD_NONE          , CPLD_NONE        , RXLOS_72_79  , TXFLT_72_79    , TXDIS_72_79   , 104   , TYPE_SFP , 7 },
    [80]  ={ABS_80_87  , CPLD_NONE          , CPLD_NONE        , RXLOS_80_87  , TXFLT_80_87    , TXDIS_80_87   , 105   , TYPE_SFP , 0 },
    [81]  ={ABS_80_87  , CPLD_NONE          , CPLD_NONE        , RXLOS_80_87  , TXFLT_80_87    , TXDIS_80_87   , 106   , TYPE_SFP , 1 },
    [82]  ={ABS_80_87  , CPLD_NONE          , CPLD_NONE        , RXLOS_80_87  , TXFLT_80_87    , TXDIS_80_87   , 107   , TYPE_SFP , 2 },
    [83]  ={ABS_80_87  , CPLD_NONE          , CPLD_NONE        , RXLOS_80_87  , TXFLT_80_87    , TXDIS_80_87   , 108   , TYPE_SFP , 3 },
    [84]  ={ABS_80_87  , CPLD_NONE          , CPLD_NONE        , RXLOS_80_87  , TXFLT_80_87    , TXDIS_80_87   , 109   , TYPE_SFP , 4 },
    [85]  ={ABS_80_87  , CPLD_NONE          , CPLD_NONE        , RXLOS_80_87  , TXFLT_80_87    , TXDIS_80_87   , 110   , TYPE_SFP , 5 },
    [86]  ={ABS_80_87  , CPLD_NONE          , CPLD_NONE        , RXLOS_80_87  , TXFLT_80_87    , TXDIS_80_87   , 111   , TYPE_SFP , 6 },
    [87]  ={ABS_80_87  , CPLD_NONE          , CPLD_NONE        , RXLOS_80_87  , TXFLT_80_87    , TXDIS_80_87   , 112   , TYPE_SFP , 7 },
    [88]  ={ABS_88_91  , CPLD_NONE          , CPLD_NONE        , RXLOS_88_91  , TXFLT_88_91    , TXDIS_88_91   , 113   , TYPE_SFP , 0 },
    [89]  ={ABS_88_91  , CPLD_NONE          , CPLD_NONE        , RXLOS_88_91  , TXFLT_88_91    , TXDIS_88_91   , 114   , TYPE_SFP , 1 },
    [90]  ={ABS_88_91  , CPLD_NONE          , CPLD_NONE        , RXLOS_88_91  , TXFLT_88_91    , TXDIS_88_91   , 115   , TYPE_SFP , 2 },
    [91]  ={ABS_88_91  , CPLD_NONE          , CPLD_NONE        , RXLOS_88_91  , TXFLT_88_91    , TXDIS_88_91   , 116   , TYPE_SFP , 3 },
    [92]  ={ABS_92_95  , CPLD_NONE          , CPLD_NONE        , RXLOS_92_95  , TXFLT_92_95    , TXDIS_92_95   , 117   , TYPE_SFP , 0 },
    [93]  ={ABS_92_95  , CPLD_NONE          , CPLD_NONE        , RXLOS_92_95  , TXFLT_92_95    , TXDIS_92_95   , 118   , TYPE_SFP , 1 },
    [94]  ={ABS_92_95  , CPLD_NONE          , CPLD_NONE        , RXLOS_92_95  , TXFLT_92_95    , TXDIS_92_95   , 119   , TYPE_SFP , 2 },
    [95]  ={ABS_92_95  , CPLD_NONE          , CPLD_NONE        , RXLOS_92_95  , TXFLT_92_95    , TXDIS_92_95   , 120   , TYPE_SFP , 3 },
    [96]  ={ABS_96_101 , LPMODE_96_101      , RESET_96_101     , CPLD_NONE    , CPLD_NONE      , CPLD_NONE     , 121   , TYPE_QSFP, 0 },
    [97]  ={ABS_96_101 , LPMODE_96_101      , RESET_96_101     , CPLD_NONE    , CPLD_NONE      , CPLD_NONE     , 122   , TYPE_QSFP, 1 },
    [98]  ={ABS_96_101 , LPMODE_96_101      , RESET_96_101     , CPLD_NONE    , CPLD_NONE      , CPLD_NONE     , 123   , TYPE_QSFP, 2 },
    [99]  ={ABS_96_101 , LPMODE_96_101      , RESET_96_101     , CPLD_NONE    , CPLD_NONE      , CPLD_NONE     , 124   , TYPE_QSFP, 3 },
    [100] ={ABS_96_101 , LPMODE_96_101      , RESET_96_101     , CPLD_NONE    , CPLD_NONE      , CPLD_NONE     , 125   , TYPE_QSFP, 4 },
    [101] ={ABS_96_101 , LPMODE_96_101      , RESET_96_101     , CPLD_NONE    , CPLD_NONE      , CPLD_NONE     , 126   , TYPE_QSFP, 5 },
    [102] ={ABS_102_103, CPLD_NONE          , CPLD_NONE        , RXLOS_102_103, TXFLT_102_103  , TXDIS_102_103 , 10    , TYPE_MGMT, BIT_000_001_010_000},
    [103] ={ABS_102_103, CPLD_NONE          , CPLD_NONE        , RXLOS_102_103, TXFLT_102_103  , TXDIS_102_103 , 11    , TYPE_MGMT, BIT_100_101_110_100},
};

#define IS_PORT_INVALID(_port)  (_port < 0) || (_port >= PORT_NUM)
#define IS_SFP(_port)           (port_attr[_port].port_type == TYPE_SFP || port_attr[_port].port_type == TYPE_MGMT)
#define IS_XSFPX(_port)         (IS_OSFP(_port) || IS_QSFPX(_port))
#define IS_QSFPX(_port)         (IS_QSFP(_port) || IS_QSFPDD(_port))
#define IS_QSFP(_port)          (port_attr[_port].port_type == TYPE_QSFP)
#define IS_QSFPDD(_port)        (port_attr[_port].port_type == TYPE_QSFPDD)
#define IS_OSFP(_port)          (port_attr[_port].port_type == TYPE_OSFP)

#define VALIDATE_PORT(p) { if (IS_PORT_INVALID(p)) return ONLP_STATUS_E_PARAM; }
#define VALIDATE_SFP_PORT(p) { if (IS_PORT_INVALID(p) || !IS_SFP(p)) return ONLP_STATUS_E_PARAM; }

static int get_port_sysfs(cpld_attr_idx_t idx, char** str)
{
    if(str == NULL)
        return ONLP_STATUS_E_PARAM;

    switch(idx) {
        case ABS_0_7:
            *str = SYSFS_CPLD2 "cpld_sfp_port_0_7_pres";
            break;
        case ABS_8_15:
            *str = SYSFS_CPLD2 "cpld_sfp_port_8_15_pres";
            break;
        case ABS_16_23:
            *str = SYSFS_CPLD3 "cpld_sfp_port_16_23_pres";
            break;
        case ABS_24_31:
            *str = SYSFS_CPLD3 "cpld_sfp_port_24_31_pres";
            break;
        case ABS_32_39:
            *str = SYSFS_CPLD4 "cpld_sfp_port_32_39_pres";
            break;
        case ABS_40_43:
            *str = SYSFS_CPLD4 "cpld_sfp_port_40_43_pres";
            break;
        case ABS_44_47:
            *str = SYSFS_CPLD5 "cpld_sfp_port_44_47_pres";
            break;
        case ABS_48_55:
            *str = SYSFS_CPLD2 "cpld_sfp_port_48_55_pres";
            break;
        case ABS_56_63:
            *str = SYSFS_CPLD2 "cpld_sfp_port_56_63_pres";
            break;
        case ABS_64_71:
            *str = SYSFS_CPLD3 "cpld_sfp_port_64_71_pres";
            break;
        case ABS_72_79:
            *str = SYSFS_CPLD3 "cpld_sfp_port_72_79_pres";
            break;
        case ABS_80_87:
            *str = SYSFS_CPLD4 "cpld_sfp_port_80_87_pres";
            break;
        case ABS_88_91:
            *str = SYSFS_CPLD4 "cpld_sfp_port_88_91_pres";
            break;
        case ABS_92_95:
            *str = SYSFS_CPLD5 "cpld_sfp_port_92_95_pres";
            break;
        case ABS_96_101:
            *str = SYSFS_CPLD5 "cpld_qsfp_port_96_101_pres";
            break;
        case RXLOS_0_7:
            *str = SYSFS_CPLD2 "cpld_sfp_port_0_7_rx_los";
            break;
        case RXLOS_8_15:
            *str = SYSFS_CPLD2 "cpld_sfp_port_8_15_rx_los";
            break;
        case RXLOS_16_23:
            *str = SYSFS_CPLD3 "cpld_sfp_port_16_23_rx_los";
            break;
        case RXLOS_24_31:
            *str = SYSFS_CPLD3 "cpld_sfp_port_24_31_rx_los";
            break;
        case RXLOS_32_39:
            *str = SYSFS_CPLD4 "cpld_sfp_port_32_39_rx_los";
            break;
        case RXLOS_40_43:
            *str = SYSFS_CPLD4 "cpld_sfp_port_40_43_rx_los";
            break;
        case RXLOS_44_47:
            *str = SYSFS_CPLD5 "cpld_sfp_port_44_47_rx_los";
            break;
        case RXLOS_48_55:
            *str = SYSFS_CPLD2 "cpld_sfp_port_48_55_rx_los";
            break;
        case RXLOS_56_63:
            *str = SYSFS_CPLD2 "cpld_sfp_port_56_63_rx_los";
            break;
        case RXLOS_64_71:
            *str = SYSFS_CPLD3 "cpld_sfp_port_64_71_rx_los";
            break;
        case RXLOS_72_79:
            *str = SYSFS_CPLD3 "cpld_sfp_port_72_79_rx_los";
            break;
        case RXLOS_80_87:
            *str = SYSFS_CPLD4 "cpld_sfp_port_80_87_rx_los";
            break;
        case RXLOS_88_91:
            *str = SYSFS_CPLD4 "cpld_sfp_port_88_91_rx_los";
            break;
        case RXLOS_92_95:
            *str = SYSFS_CPLD5 "cpld_sfp_port_92_95_rx_los";
            break;
        case TXFLT_0_7:
            *str = SYSFS_CPLD2 "cpld_sfp_port_0_7_tx_fault";
            break;
        case TXFLT_8_15:
            *str = SYSFS_CPLD2 "cpld_sfp_port_8_15_tx_fault";
            break;
        case TXFLT_16_23:
            *str = SYSFS_CPLD3 "cpld_sfp_port_16_23_tx_fault";
            break;
        case TXFLT_24_31:
            *str = SYSFS_CPLD3 "cpld_sfp_port_24_31_tx_fault";
            break;
        case TXFLT_32_39:
            *str = SYSFS_CPLD4 "cpld_sfp_port_32_39_tx_fault";
            break;
        case TXFLT_40_43:
            *str = SYSFS_CPLD4 "cpld_sfp_port_40_43_tx_fault";
            break;
        case TXFLT_44_47:
            *str = SYSFS_CPLD5 "cpld_sfp_port_44_47_tx_fault";
            break;
        case TXFLT_48_55:
            *str = SYSFS_CPLD2 "cpld_sfp_port_48_55_tx_fault";
            break;
        case TXFLT_56_63:
            *str = SYSFS_CPLD2 "cpld_sfp_port_56_63_tx_fault";
            break;
        case TXFLT_64_71:
            *str = SYSFS_CPLD3 "cpld_sfp_port_64_71_tx_fault";
            break;
        case TXFLT_72_79:
            *str = SYSFS_CPLD3 "cpld_sfp_port_72_79_tx_fault";
            break;
        case TXFLT_80_87:
            *str = SYSFS_CPLD4 "cpld_sfp_port_80_87_tx_fault";
            break;
        case TXFLT_88_91:
            *str = SYSFS_CPLD4 "cpld_sfp_port_88_91_tx_fault";
            break;
        case TXFLT_92_95:
            *str = SYSFS_CPLD5 "cpld_sfp_port_92_95_tx_fault";
            break;
        case RESET_96_101:
            *str = SYSFS_CPLD5 "cpld_qsfp_port_96_101_rst";
            break;
        case LPMODE_96_101:
            *str = SYSFS_CPLD5 "cpld_qsfp_port_96_101_lpmode";
            break;
        case TXDIS_0_7:
            *str = SYSFS_CPLD2 "cpld_sfp_port_0_7_tx_disable";
            break;
        case TXDIS_8_15:
            *str = SYSFS_CPLD2 "cpld_sfp_port_8_15_tx_disable";
            break;
        case TXDIS_16_23:
            *str = SYSFS_CPLD3 "cpld_sfp_port_16_23_tx_disable";
            break;
        case TXDIS_24_31:
            *str = SYSFS_CPLD3 "cpld_sfp_port_24_31_tx_disable";
            break;
        case TXDIS_32_39:
            *str = SYSFS_CPLD4 "cpld_sfp_port_32_39_tx_disable";
            break;
        case TXDIS_40_43:
            *str = SYSFS_CPLD4 "cpld_sfp_port_40_43_tx_disable";
            break;
        case TXDIS_44_47:
            *str = SYSFS_CPLD5 "cpld_sfp_port_44_47_tx_disable";
            break;
        case TXDIS_48_55:
            *str = SYSFS_CPLD2 "cpld_sfp_port_48_55_tx_disable";
            break;
        case TXDIS_56_63:
            *str = SYSFS_CPLD2 "cpld_sfp_port_56_63_tx_disable";
            break;
        case TXDIS_64_71:
            *str = SYSFS_CPLD3 "cpld_sfp_port_64_71_tx_disable";
            break;
        case TXDIS_72_79:
            *str = SYSFS_CPLD3 "cpld_sfp_port_72_79_tx_disable";
            break;
        case TXDIS_80_87:
            *str = SYSFS_CPLD4 "cpld_sfp_port_80_87_tx_disable";
            break;
        case TXDIS_88_91:
            *str = SYSFS_CPLD4 "cpld_sfp_port_88_91_tx_disable";
            break;
        case TXDIS_92_95:
            *str = SYSFS_CPLD5 "cpld_sfp_port_92_95_tx_disable";
            break;
        case ABS_102_103:
        case RXLOS_102_103:
        case TXFLT_102_103:
            *str = SYSFS_CPLD1 "cpld_mgmt_sfp_port_status";
            break;
        case TXDIS_102_103:
            *str = SYSFS_CPLD1 "cpld_mgmt_sfp_port_conf";
            break;
        default:
            *str = "";
            return ONLP_STATUS_E_PARAM;
    }
    return ONLP_STATUS_OK;
}

static int get_bit(int attr, unsigned int bit_stream, uint8_t *bit)
{
    int rv  = ONLP_STATUS_OK;
    int tmp_value = 0;

    if(bit == NULL)
        return ONLP_STATUS_E_PARAM;

    switch(attr) {

        case ABS_102_103:
            tmp_value = bit_stream >> 0;
            break;
        case RXLOS_102_103:
            tmp_value = bit_stream >> 3;
            break;
        case TXFLT_102_103:
            tmp_value = bit_stream >> 6;
            break;
        case TXDIS_102_103:
            tmp_value = bit_stream >> 9;
            break;
        default:
            if(bit_stream > 7) {
                return ONLP_STATUS_E_PARAM;
            } else {
                tmp_value = bit_stream;
                break;
            }
    }
     *bit = (tmp_value & 0x7);
    return rv;
}

static int xfr_ctrl_to_sysfs(int port, onlp_sfp_control_t control , char **sysfs, int *attr)
{
    int rv  = ONLP_STATUS_OK;

    if(sysfs == NULL || attr == NULL)
        return ONLP_STATUS_E_PARAM;

    switch(control)
    {
        case ONLP_SFP_CONTROL_RESET:
        case ONLP_SFP_CONTROL_RESET_STATE:
            {
                rv = get_port_sysfs(port_attr[port].reset, sysfs);
                *attr = port_attr[port].reset;
                break;
            }
        case ONLP_SFP_CONTROL_RX_LOS:
            {
                rv = get_port_sysfs(port_attr[port].rxlos, sysfs);
                *attr = port_attr[port].rxlos;
                break;
            }
        case ONLP_SFP_CONTROL_TX_FAULT:
            {
                rv = get_port_sysfs(port_attr[port].txfault, sysfs);
                *attr = port_attr[port].txfault;
                break;
            }
        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                rv = get_port_sysfs(port_attr[port].txdis, sysfs);
                *attr = port_attr[port].txdis;
                break;
            }
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                rv = get_port_sysfs(port_attr[port].lpmode, sysfs);
                *attr = port_attr[port].lpmode;
                break;
            }
        default:
            rv = ONLP_STATUS_E_UNSUPPORTED;
            *sysfs = "";
            *attr = CPLD_NONE;
    }

    if (rv != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    return ONLP_STATUS_OK;
}

static int xfr_port_to_eeprom_bus(int port)
{
    int bus = -1;

    bus=port_attr[port].eeprom_bus;
    return bus;
}

/**
 * @brief Get device class for a port
 *
 * This function get the device class for a given port.
 *
 * @param port The port number
 * @return An error condition or ONLP_STATUS_OK.
 */
int onlp_sfpi_dev_class_get(int port, int *dev_class)
{
    int rv, bus;

    VALIDATE_PORT(port);
    bus = xfr_port_to_eeprom_bus(port);

    //read dev_class
    rv = onlp_file_read_int(dev_class, SYS_FMT, bus, EEPROM_ADDR, SYSFS_DEV_CLASS);
    if(rv < 0) {
        AIM_LOG_ERROR("Unable to read "SYS_FMT", error=%d", bus, EEPROM_ADDR, SYSFS_DEV_CLASS, rv);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Set device class for QSFP ports
 *
 * This function set the device class for a given QSFP port.
 *
 * @param port The port number
 * @param dev_class The device class to set
 * @return An error condition or ONLP_STATUS_OK.
 */
int onlp_sfpi_dev_class_set(int port, int dev_class)
{
    int bus=0;

    VALIDATE_PORT(port);
    bus = xfr_port_to_eeprom_bus(port);

    // set dev_class
    ONLP_TRY(onlp_file_write_int(dev_class, SYS_FMT, bus, EEPROM_ADDR, SYSFS_DEV_CLASS));

    return ONLP_STATUS_OK;
}

/**
 * @brief Update device class for QSFP-Related ports
 *
 * This function updates the device class for a given QSFP-Related port.
 * It reads the current device class and module type, then checks against a dev type list
 * to determine the correct device class.
 * If the device class needs to be updated, it writes the new value to dev_class.
 *
 * @param port The port number
 * @return An error condition or current port dev_class.
 */
int onlp_sfpi_dev_class_update_port(int port)
{
    int dev_class, type, i;

    VALIDATE_PORT(port);
    if (!IS_QSFPX(port) || !onlp_sfpi_is_present(port)) {
        return ONLP_STATUS_OK;
    }

    //read dev_class
    ONLP_TRY(onlp_sfpi_dev_class_get(port, &dev_class));

    //read module type
    type = onlp_sfpi_dev_readb(port, EEPROM_ADDR, 0);
    if (type < 0) {
        AIM_LOG_ERROR("Port[%d] Addr(0x%02x): invalid module type=%d.\n", port, EEPROM_ADDR, type);
        return ONLP_STATUS_E_INTERNAL;
    }

    for(i = 0; i < PORT_TYPE_DICT_SIZE ; ++i) {
        if (type != port_type_dict[i].key) {
            continue;
        }

        if (port_type_dict[i].value != dev_class) {
            ONLP_TRY(onlp_sfpi_dev_class_set(port, port_type_dict[i].value));
            AIM_LOG_INFO("Port[%d] Type(0x%02x): %d to %d.\n", port, type, dev_class, port_type_dict[i].value);
            break;
        } else { //dev_class is the same.
            break;
        }
    }

    if (i == PORT_TYPE_DICT_SIZE) {
        AIM_LOG_ERROR("Port[%d] Type: %x is Unknown.\n", port, type);
        return ONLP_STATUS_E_INTERNAL;
    }

    return port_type_dict[i].value;
}

/**
 * @brief Update device class for QSFP-Related ports
 *
 * This function updates the device class for a given QSFP-Related port.
 * It reads the current device class and module type, then checks against a dev type list
 * to determine the correct device class.
 * If the device class needs to be updated, it writes the new value to dev_class.
 *
 * @param port The port number. -1 for all ports.
 * @return An error condition or current port dev_class.
 */
int onlp_sfpi_dev_class_update(int port)
{
    int rv = ONLP_STATUS_OK;

    // single port update
    if (port != ALL_PORTS) {
        return onlp_sfpi_dev_class_update_port(port);
    }

    // update all QSFPX ports
    for(int i = 0; i < PORT_NUM; ++i) {
        if(!IS_QSFPX(i)) {
            continue;
        }

        if (onlp_sfpi_dev_class_update_port(i) < 0) {
            rv = ONLP_STATUS_E_INTERNAL;
        }
    }

    return rv;
}

static int ufi_file_seek_writeb(const char *file, long offset, uint8_t value)
{
    int fd = -1;

    fd = open(file, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) {
        AIM_LOG_ERROR("[%s] Failed to open sysfs file %s", __FUNCTION__, file);
        return ONLP_STATUS_E_INTERNAL;
    }

    // Check for valid offset
    if (offset < 0) {
        AIM_LOG_ERROR("[%s] Invalid offset %d", __FUNCTION__,offset);
        close(fd);
        return ONLP_STATUS_E_INTERNAL;
    }

    // Write value
    if (pwrite(fd, &value, sizeof(uint8_t), offset) != sizeof(uint8_t)) {
        AIM_LOG_ERROR("[%s] Failed to write to sysfs file, offset=%d, value=%d, file=%s", __FUNCTION__, offset, value, file);
        close(fd);
        return ONLP_STATUS_E_INTERNAL;
    }

    close(fd);

    return ONLP_STATUS_OK;
}

static int ufi_file_seek_readb(const char *file, long offset, uint8_t *value)
{
    int fd = -1;

    fd = open(file, O_RDONLY);
    if (fd == -1) {
        AIM_LOG_ERROR("[%s] Failed to open sysfs file %s", __FUNCTION__, file);
        return ONLP_STATUS_E_INTERNAL;
    }

    // Check for valid offset
    if (offset < 0) {
        AIM_LOG_ERROR("[%s] Invalid offset %d", __FUNCTION__,offset);
        close(fd);
        return ONLP_STATUS_E_INTERNAL;
    }

    // Read value
    if (pread(fd, value, sizeof(uint8_t), offset) != sizeof(uint8_t)) {
        AIM_LOG_ERROR("[%s] Failed to read sysfs file, offset=%d, file=%s", __FUNCTION__, offset, file);
        close(fd);
        return ONLP_STATUS_E_INTERNAL;
    }

    close(fd);

    return ONLP_STATUS_OK;
}

/**
 * @brief Read 256th (0-based) byte offset to force page select to 0 to avoid eeprom checksum failure caused by page mis-match
 * @param sysfs_path: The sysfs path to the EEPROM.
 * @returns An error condition.
 */
static int ufi_reset_page_select(char *sysfs_path)
{
    int fd = -1;
    off_t offset_256 = 256;
    uint8_t value = 0;

    fd = open(sysfs_path, O_RDONLY);
    if (fd == -1) {
        return ONLP_STATUS_E_INTERNAL;
    }

    // Read value
    if (pread(fd, &value, sizeof(uint8_t), offset_256) != sizeof(uint8_t)) {
        close(fd);
        return ONLP_STATUS_E_INTERNAL;
    }

    close(fd);
    return ONLP_STATUS_OK;
}

/**
 * @brief Get SFF-8636 Port TX Disable Status by EEPROM
 * @param port: The port number.
 * @param status: 1 if tx disable (turn on)
 * @param status: 0 if normal (turn off)
 * @returns An error condition.
 */
static int ufi_sff8636_txdisable_status_get(int port, int* status, onlp_sfp_control_t control)
{
    uint8_t value = 0;

    if (control != ONLP_SFP_CONTROL_TX_DISABLE &&
        control != ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL) {
        AIM_LOG_ERROR("[%s] invalid control, port=%d, control=%d\n", __FUNCTION__, port, control);
        return ONLP_STATUS_E_PARAM;
    }

    if (onlp_sfpi_is_present(port) != 1) {
        return ONLP_STATUS_OK;
    }

    ONLP_TRY(value = onlp_sfpi_dev_readb(port, EEPROM_ADDR, SFF8636_EEPROM_OFFSET_TXDIS));

    if (control == ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL) {
        *status = value;
    } else { //ONLP_SFP_CONTROL_TX_DISABLE
        // Check each bit of the 'value' has all bits set to 1 meets TX Disable condition (all channels disabled).
        if (value == SFF8636_EEPROM_TX_DIS) {
            *status = 1;
        } else {
            *status = 0;
        }
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Set SFF-8636 Port TX Disable Status by EEPROM
 * @param port: The port number.
 * @param status: 1 if tx disable (turn on)
 * @param status: 0 if normal (turn off)
 * @returns An error condition.
 */
static int ufi_sff8636_txdisable_status_set(int port, int status, onlp_sfp_control_t control)
{
    uint8_t value = 0, readback = 0;

    if (control != ONLP_SFP_CONTROL_TX_DISABLE &&
        control != ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL) {
        AIM_LOG_ERROR("[%s] invalid control, port=%d, control=%d\n", __FUNCTION__, port, control);
        return ONLP_STATUS_E_PARAM;
    }

    if (onlp_sfpi_is_present(port) != 1) {
        return ONLP_STATUS_OK;
    }

    if (control == ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL) {
        // check status range
        if (status < 0 || status > TX_DIS_INPUT_MAX) {
            AIM_LOG_ERROR("[%s] invalid status, port=%d, status=%d\n", __FUNCTION__, port, status);
            return ONLP_STATUS_E_PARAM;
        } else {
            value = (uint8_t)(status & SFF8636_EEPROM_TX_DIS);
        }
    } else { //ONLP_SFP_CONTROL_TX_DISABLE
        if (status == 0) {
            value = SFF8636_EEPROM_TX_EN;
        } else if (status == 1) {
            value = SFF8636_EEPROM_TX_DIS;
        } else {
            AIM_LOG_ERROR("[%s] invalid status, port=%d, status=%d\n", __FUNCTION__, port, status);
            return ONLP_STATUS_E_PARAM;
        }
    }

    ONLP_TRY(onlp_sfpi_dev_writeb(port, EEPROM_ADDR, SFF8636_EEPROM_OFFSET_TXDIS, value));
    ONLP_TRY(readback = onlp_sfpi_dev_readb(port, EEPROM_ADDR, SFF8636_EEPROM_OFFSET_TXDIS));
    if (value != readback) {
        AIM_LOG_ERROR("[%s] compare failed, write value=%d, readback=%d\n", __FUNCTION__, value, readback);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

static int ufi_cmis_txdisable_supported(int port)
{
    uint8_t value = 0;
    char sysfs_path[256] = {0};
    int cmis_ver = 0;
    int mem_model = 0;
    int bus = 0;
    int seek = 0;
    int length = 0;
    int tx_dis_adv = 0;

    //Check CMIS version on lower page 0x01
    cmis_ver = onlp_sfpi_dev_readb(port, EEPROM_ADDR, CMIS_OFFSET_REVISION);
    if (cmis_ver < CMIS_VAL_VERSION_MIN || cmis_ver > CMIS_VAL_VERSION_MAX) {
        AIM_LOG_INFO("Port[%d] CMIS version %x.%x is not supported (certified range is %x.x-%x.x)\n",
            port, cmis_ver/16, cmis_ver%16, CMIS_VAL_VERSION_MIN/16, CMIS_VAL_VERSION_MAX/16);
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    //Check CMIS memory model on lower page 0x02 bit[7]
    mem_model = shift_bit_mask(onlp_sfpi_dev_readb(port, EEPROM_ADDR, CMIS_OFFSET_MEMORY_MODEL), CMIS_MASK_MEMORY_MODEL);
    if (mem_model != CMIS_VAL_MEMORY_MODEL_PAGED) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    //Check CMIS Tx disable advertisement on page 0x01 offset[155] bit[1]
    VALIDATE_PORT(port);
    bus = xfr_port_to_eeprom_bus(port);
    seek = CMIS_SEEK_TX_DIS_ADV;

    // create and check sysfs_path
    length = snprintf(sysfs_path, sizeof(sysfs_path), SYS_FMT, bus, EEPROM_ADDR, SYSFS_EEPROM);
    if (length < 0 || length >= sizeof(sysfs_path)) {
        AIM_LOG_ERROR("[%s] Error generating sysfs path\n", __FUNCTION__);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (ufi_file_seek_readb(sysfs_path, seek, &value) < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    tx_dis_adv = shift_bit_mask(value, CMIS_MASK_TX_DIS_ADV);

    if (tx_dis_adv != CMIS_VAL_TX_DIS_SUPPORTED) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get CMIS Port TX Disable Status
 * @param port: The port number.
 * @param status: 1 if tx disable (turn on)
 * @param status: 0 if normal (turn off)
 * @returns An error condition.
 */
static int ufi_cmis_txdisable_status_get(int port, int* status, onlp_sfp_control_t control)
{
    int ret = 0;
    uint8_t value = 0;
    char sysfs_path[256] = {0};
    int bus = 0;
    int length = 0;

    if (control != ONLP_SFP_CONTROL_TX_DISABLE &&
        control != ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL) {
        AIM_LOG_ERROR("[%s] invalid control, port=%d, control=%d\n", __FUNCTION__, port, control);
        return ONLP_STATUS_E_PARAM;
    }

    // Check module present
    if (onlp_sfpi_is_present(port) != 1) {
        return ONLP_STATUS_OK;
    }

    // tx disable support check
    if ((ret=ufi_cmis_txdisable_supported(port)) != ONLP_STATUS_OK) {
        return ret;
    }

    VALIDATE_PORT(port);
    bus = xfr_port_to_eeprom_bus(port);
    length = snprintf(sysfs_path, sizeof(sysfs_path), SYS_FMT, bus, EEPROM_ADDR, SYSFS_EEPROM);
    // check snprintf
    if (length < 0 || length >= sizeof(sysfs_path)) {
        AIM_LOG_ERROR("[%s] Error generating sysfs path\n", __FUNCTION__);
        return ONLP_STATUS_E_INTERNAL;
    }

    // get tx disable
    if (ufi_file_seek_readb(sysfs_path, CMIS_SEEK_TX_DIS, &value) < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (control == ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL) {
        *status = value;
    } else { //ONLP_SFP_CONTROL_TX_DISABLE
        // Check each bit of the 'value' has all bits set to 1 meets TX Disable condition (all channels disabled).
        if (value == CMIS_VAL_TX_DIS) {
            *status = 1;
        } else {
            *status = 0;
        }
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Set CMIS Port TX Disable Status
 * @param port: The port number.
 * @param status: 1 if tx disable (turn on)
 * @param status: 0 if normal (turn off)
 * @returns An error condition.
 */
static int ufi_cmis_txdisable_status_set(int port, int status, onlp_sfp_control_t control)
{
    uint8_t value = 0, readback = 0;
    char sysfs_path[256] = {0};
    int bus = 0;
    int seek = CMIS_SEEK_TX_DIS;

    if (control != ONLP_SFP_CONTROL_TX_DISABLE &&
        control != ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL) {
        AIM_LOG_ERROR("[%s] invalid control, port=%d, control=%d\n", __FUNCTION__, port, control);
        return ONLP_STATUS_E_PARAM;
    }

    // Check module present
    if (onlp_sfpi_is_present(port) != 1) {
        return ONLP_STATUS_OK;
    }

    // tx disable support check
    if (ufi_cmis_txdisable_supported(port) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if (control == ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL) {
        if (status < 0 || status > TX_DIS_INPUT_MAX) {
            AIM_LOG_ERROR("[%s] unaccepted status, port=%d, status=%d\n", __FUNCTION__, port, status);
            return ONLP_STATUS_E_PARAM;
        } else {
            value = (uint8_t)(status);
        }
    } else {
        // set value
        if (status == 0) {
            value = CMIS_VAL_TX_EN;
        } else if (status == 1) {
            value = CMIS_VAL_TX_DIS;
        } else {
            AIM_LOG_ERROR("[%s] unaccepted status, port=%d, status=%d\n", __FUNCTION__, port, status);
            return ONLP_STATUS_E_PARAM;
        }
    }

    // set sysfs_path
    VALIDATE_PORT(port);
    bus = xfr_port_to_eeprom_bus(port);
    // check snprintf
    int length = snprintf(sysfs_path, sizeof(sysfs_path), SYS_FMT, bus, EEPROM_ADDR, SYSFS_EEPROM);
    if (length < 0 || length >= sizeof(sysfs_path)) {
        AIM_LOG_ERROR("[%s] Error generating sysfs path\n", __FUNCTION__);
        return ONLP_STATUS_E_INTERNAL;
    }

    // write tx disable
    if (ufi_file_seek_writeb(sysfs_path, seek, value) < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    // readback tx disable
    if (ufi_file_seek_readb(sysfs_path, seek, &readback) < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    // check tx disable readback
    if (value != readback) {
        AIM_LOG_ERROR("[%s] port[%d] tx disable readback failed, write value=%d, readback=%d\n", __FUNCTION__, port, value, readback);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Initialize the SFPI subsystem.
 */
int onlp_sfpi_init(void)
{
    init_lock();
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the bitmap of SFP-capable port numbers.
 * @param bmap [out] Receives the bitmap.
 */
int onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{
    int p;
    for(p = 0; p < PORT_NUM; p++) {
        AIM_BITMAP_SET(bmap, p);
    }
    return ONLP_STATUS_OK;
}

/**
 * @brief Determine if an SFP is present.
 * @param port The port number.
 * @returns 1 if present
 * @returns 0 if absent
 * @returns An error condition.
 */
int onlp_sfpi_is_present(int port)
{
    int status=ONLP_STATUS_OK;
    int abs = 0, present = 0;
    char *sysfs = NULL;
    uint8_t bit = 0;

    VALIDATE_PORT(port);

    ONLP_TRY(get_port_sysfs(port_attr[port].abs, &sysfs));

    if ((status = read_file_hex(&abs, sysfs)) < 0) {
        AIM_LOG_ERROR("onlp_sfpi_is_present() failed, error=%d, sysfs=%s",
                          status, sysfs);
        check_and_do_i2c_mux_reset(port);
        return status;
    }

    ONLP_TRY(get_bit(port_attr[port].abs, port_attr[port].cpld_bit, &bit));
    present = (get_bit_value(abs, bit) == 0) ? 1:0;

    return present;
}

/**
 * @brief Return the presence bitmap for all SFP ports.
 * @param dst Receives the presence bitmap.
 */
int onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int p = 0;
    int rc = 0;

    for (p = 0; p < PORT_NUM; p++) {
        if ((rc = onlp_sfpi_is_present(p)) < 0) {
            return rc;
        }
        AIM_BITMAP_MOD(dst, p, rc);
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Return the RX_LOS bitmap for all SFP ports.
 * @param dst Receives the RX_LOS bitmap.
 */
int onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int i=0, value=0;

    for(i = 0; i < PORT_NUM; i++) {
        if(IS_SFP(i)) {
            if(onlp_sfpi_control_get(i, ONLP_SFP_CONTROL_RX_LOS, &value) != ONLP_STATUS_OK) {
                AIM_BITMAP_MOD(dst, i, 0);  //set default value for port which has no cap
            } else {
                AIM_BITMAP_MOD(dst, i, value);
            }
        } else {
            AIM_BITMAP_MOD(dst, i, 0);  //set default value for port which has no cap
        }
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Read the SFP EEPROM.
 * @param port The port number.
 * @param data Receives the SFP data.
 */
int onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{
    int size = 0, bus = 0, rc = 0;
    char sysfs_path[256] = {0};

    VALIDATE_PORT(port);

    memset(data, 0, 256);
    bus = xfr_port_to_eeprom_bus(port);

    // create and check sysfs_path
    size = snprintf(sysfs_path, sizeof(sysfs_path),
        SYS_FMT, bus, EEPROM_ADDR, SYSFS_EEPROM);

    if (size < 0 || (size_t)size >= sizeof(sysfs_path)) {
        return ONLP_STATUS_E_INTERNAL;
    }

    // reset page select to 0
    ufi_reset_page_select(sysfs_path);

    if((rc = onlp_file_read(data, 256, &size, sysfs_path)) < 0) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d), sysfs_path=%s", port, sysfs_path);
        check_and_do_i2c_mux_reset(port);
        return rc;
    }

    if (size != 256) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d), size is different!", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Read a byte from an address on the given SFP port's bus.
 * @param port The port number.
 * @param devaddr The device address.
 * @param addr The address.
 */
int onlp_sfpi_dev_readb(int port, uint8_t devaddr, uint8_t addr)
{
    VALIDATE_PORT(port);
    int rc = 0;
    int bus = xfr_port_to_eeprom_bus(port);

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if ((rc = onlp_i2c_readb(bus, devaddr, addr, ONLP_I2C_F_FORCE)) < 0) {
        check_and_do_i2c_mux_reset(port);
    }
    return rc;
}

/**
 * @brief Write a byte to an address on the given SFP port's bus.
 */
int onlp_sfpi_dev_writeb(int port, uint8_t devaddr, uint8_t addr, uint8_t value)
{
    VALIDATE_PORT(port);
    int rc = 0;
    int bus = xfr_port_to_eeprom_bus(port);

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if ((rc = onlp_i2c_writeb(bus, devaddr, addr, value, ONLP_I2C_F_FORCE)) < 0) {
         check_and_do_i2c_mux_reset(port);
    }
    return rc;
}

/**
 * @brief Read a word from an address on the given SFP port's bus.
 * @param port The port number.
 * @param devaddr The device address.
 * @param addr The address.
 * @returns The word if successful, error otherwise.
 */
int onlp_sfpi_dev_readw(int port, uint8_t devaddr, uint8_t addr)
{
    VALIDATE_PORT(port);
    int rc = 0;
    int bus = xfr_port_to_eeprom_bus(port);

    if(onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if((rc = onlp_i2c_readw(bus, devaddr, addr, ONLP_I2C_F_FORCE)) < 0) {
         check_and_do_i2c_mux_reset(port);
    }
    return rc;
}

/**
 * @brief Write a word to an address on the given SFP port's bus.
 */
int onlp_sfpi_dev_writew(int port, uint8_t devaddr, uint8_t addr, uint16_t value)
{
    VALIDATE_PORT(port);
    int rc = 0;
    int bus = xfr_port_to_eeprom_bus(port);

    if(onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if((rc = onlp_i2c_writew(bus, devaddr, addr, value, ONLP_I2C_F_FORCE)) < 0) {
        check_and_do_i2c_mux_reset(port);
    }
    return rc;
}

/**
 * @brief Read from an address on the given SFP port's bus.
 * @param port The port number.
 * @param devaddr The device address.
 * @param addr The address.
 * @returns The data if successful, error otherwise.
 */
int onlp_sfpi_dev_read(int port, uint8_t devaddr, uint8_t addr, uint8_t* rdata, int size)
{
    VALIDATE_PORT(port);
    int bus = xfr_port_to_eeprom_bus(port);

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if (onlp_i2c_block_read(bus, devaddr, addr, size, rdata, ONLP_I2C_F_FORCE) < 0) {
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Write to an address on the given SFP port's bus.
 */
int onlp_sfpi_dev_write(int port, uint8_t devaddr, uint8_t addr, uint8_t* data, int size)
{
    VALIDATE_PORT(port);
    int rc = 0;
    int bus = xfr_port_to_eeprom_bus(port);

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if ((rc = onlp_i2c_write(bus, devaddr, addr, size, data, ONLP_I2C_F_FORCE)) < 0) {
        check_and_do_i2c_mux_reset(port);
    }

    return rc;
}


/**
 * @brief Read the SFP DOM EEPROM.
 * @param port The port number.
 * @param data Receives the SFP data.
 */
int onlp_sfpi_dom_read(int port, uint8_t data[256])
{
    char eeprom_path[512];
    FILE* fp;
    int bus = 0;

    //sfp dom is on 0x51 (2nd 256 bytes)
    //qsfp dom is on lower page 0x00
    //qsfpdd 2.0 dom is on lower page 0x00
    //qsfpdd 3.0 and later dom and above is on lower page 0x00 and higher page 0x17
    VALIDATE_SFP_PORT(port);

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    memset(data, 0, 256);
    memset(eeprom_path, 0, sizeof(eeprom_path));

    //set eeprom_path
    bus = xfr_port_to_eeprom_bus(port);
    snprintf(eeprom_path, sizeof(eeprom_path), SYS_FMT, bus, EEPROM_ADDR, SYSFS_EEPROM);

    //read eeprom
    fp = fopen(eeprom_path, "r");
    if(fp == NULL) {
        AIM_LOG_ERROR("Unable to open the eeprom device file of port(%d)", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (fseek(fp, 256, SEEK_CUR) != 0) {
        fclose(fp);
        AIM_LOG_ERROR("Unable to set the file position indicator of port(%d)", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }

    int ret = fread(data, 1, 256, fp);
    fclose(fp);
    if (ret != 256) {
        AIM_LOG_ERROR("Unable to read the module_eeprom device file of port(%d)", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Perform any actions required after an SFP is inserted.
 * @param port The port number.
 * @param info The SFF Module information structure.
 * @notes Optional
 */
int onlp_sfpi_post_insert(int port, sff_info_t* info)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Returns whether or not the given control is suppport on the given port.
 * @param port The port number.
 * @param control The control.
 * @param rv [out] Receives 1 if supported, 0 if not supported.
 * @note This provided for convenience and is optional.
 * If you implement this function your control_set and control_get APIs
 * will not be called on unsupported ports.
 */
int onlp_sfpi_control_supported(int port, onlp_sfp_control_t control, int* rv)
{
    VALIDATE_PORT(port);

    //set unsupported as default value
    *rv = 0;

    switch (control) {
        case ONLP_SFP_CONTROL_RESET:
        case ONLP_SFP_CONTROL_RESET_STATE:
        case ONLP_SFP_CONTROL_LP_MODE:
            if (IS_XSFPX(port)) {
                *rv = 1;
            }
            break;
        case ONLP_SFP_CONTROL_RX_LOS:
        case ONLP_SFP_CONTROL_TX_FAULT:
            if (IS_SFP(port)) {
                *rv = 1;
            }
            break;
        case ONLP_SFP_CONTROL_TX_DISABLE:
        case ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL:
            *rv = 1;
            break;
        default:
            *rv = 0;
            break;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Set an SFP control.
 * @param port The port.
 * @param control The control.
 * @param value The value.
 */
int onlp_sfpi_control_set(int port, onlp_sfp_control_t control, int value)
{
    int rc = 0;
    int reg_val = 0;
    char *sysfs = NULL;
    uint8_t bit = 0;
    int attr = 0;
    int op_type = OP_SYSFS;

    VALIDATE_PORT(port);

    //check control is valid for this port
    switch(control)
    {
        case ONLP_SFP_CONTROL_RESET:
            {
                if (IS_XSFPX(port)) {
                    //reverse value
                    value = (value == 0) ? 1:0;
                    op_type = OP_SYSFS;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_TX_DISABLE:
        case ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL:
            {
                if (IS_SFP(port)) {
                    op_type = OP_SYSFS;
                } else if (IS_QSFPX(port)) {
                    int dev_class = 0;
                    ONLP_TRY(dev_class = onlp_sfpi_dev_class_update(port));

                    if (dev_class == 1) { //SFF8636 module
                        op_type = OP_8636;
                    } else if (dev_class == 3) { //CMIS module
                        op_type = OP_CMIS;
                    } else if (dev_class < 0) {
                        AIM_LOG_ERROR("Port[%d] dev_class %d is not supported for tx disable control.\n", port, dev_class);
                        return ONLP_STATUS_E_UNSUPPORTED;
                    } else { // module absent or other case
                        return ONLP_STATUS_OK;
                    }
                } else if (IS_OSFP(port)) {
                    op_type = OP_CMIS;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                if (IS_XSFPX(port)) {
                    op_type = OP_SYSFS;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
    }

    if(op_type == OP_SYSFS) {
        //get sysfs
        ONLP_TRY(xfr_ctrl_to_sysfs(port, control, &sysfs, &attr));

        //read reg_val
        if (read_file_hex(&reg_val, sysfs) < 0) {
            check_and_do_i2c_mux_reset(port);
            return ONLP_STATUS_E_INTERNAL;
        }

        //update reg_val
        //0 is normal, 1 is reset, reverse value to fit our platform
        ONLP_TRY(get_bit(attr, port_attr[port].cpld_bit, &bit));
        reg_val = operate_bit(reg_val, bit, value);

        //write reg_val
        if ((rc=onlp_file_write_int(reg_val, sysfs)) < 0) {
            AIM_LOG_ERROR("Unable to write %s, error=%d, reg_val=%x", sysfs,  rc, reg_val);
            check_and_do_i2c_mux_reset(port);
            return ONLP_STATUS_E_INTERNAL;
        }
    } else if (op_type == OP_CMIS) {
        ONLP_TRY(ufi_cmis_txdisable_status_set(port, value, control));
    } else if (op_type == OP_8636) {
        ONLP_TRY(ufi_sff8636_txdisable_status_set(port, value, control));
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get an SFP control.
 * @param port The port.
 * @param control The control
 * @param [out] value Receives the current value.
 */
int onlp_sfpi_control_get(int port, onlp_sfp_control_t control, int* value)
{
    int rc;
    int reg_val = 0;
    char *sysfs = NULL;
    uint8_t bit = 0;
    int attr = 0;
    int op_type = OP_SYSFS;

    VALIDATE_PORT(port);

    switch(control)
    {
        case ONLP_SFP_CONTROL_RESET_STATE:
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                if (IS_XSFPX(port)) {
                    op_type = OP_SYSFS;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_RX_LOS:
        case ONLP_SFP_CONTROL_TX_FAULT:
            {
                if (IS_SFP(port)) {
                    op_type = OP_SYSFS;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_TX_DISABLE:
        case ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL:
            {
                if (IS_SFP(port)) {
                    op_type = OP_SYSFS;
                } else if (IS_QSFPX(port)) {
                    int dev_class = 0;
                    ONLP_TRY(dev_class = onlp_sfpi_dev_class_update(port));

                    if (dev_class == 1) { //SFF8636 module
                        op_type = OP_8636;
                    } else if (dev_class == 3) { //CMIS module
                        op_type = OP_CMIS;
                    } else if (dev_class < 0) {
                        AIM_LOG_ERROR("Port[%d] dev_class %d is not supported for tx disable control.\n", port, dev_class);
                        return ONLP_STATUS_E_UNSUPPORTED;
                    } else { // module absent or other case
                        return ONLP_STATUS_OK;
                    }
                } else if (IS_OSFP(port)) {
                    op_type = OP_CMIS;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
    }

    if(op_type == OP_SYSFS) {
        //get sysfs
        ONLP_TRY(xfr_ctrl_to_sysfs(port, control, &sysfs, &attr));

        //read value
        if ((rc = read_file_hex(&reg_val, sysfs)) < 0) {
            AIM_LOG_ERROR("onlp_sfpi_control_get() failed, error=%d, sysfs=%s", rc, sysfs);
            check_and_do_i2c_mux_reset(port);
            return rc;
        }

        ONLP_TRY(get_bit(attr, port_attr[port].cpld_bit, &bit));
        *value = get_bit_value(reg_val, bit);

        //reverse bit
        if (control == ONLP_SFP_CONTROL_RESET_STATE) {
            *value = !(*value);
        }
    } else if (op_type == OP_CMIS) {
        return ufi_cmis_txdisable_status_get(port, value, control);
    } else if (op_type == OP_8636) {
        return ufi_sff8636_txdisable_status_get(port, value, control);
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Remap SFP user SFP port numbers before calling the SFPI interface.
 * @param port The user SFP port number.
 * @param [out] rport Receives the new port.
 * @note This function will be called to remap the user SFP port number
 * to the number returned in rport before the SFPI functions are called.
 * This is an optional convenience for platforms with dynamic or
 * variant physical SFP numbering.
 */
int onlp_sfpi_port_map(int port, int* rport)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Deinitialize the SFP driver.
 */
int onlp_sfpi_denit(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Generic debug status information.
 * @param port The port number.
 * @param pvs The output pvs.
 * @notes The purpose of this vector is to allow reporting of internal debug
 * status and information from the platform driver that might be used to debug
 * SFP runtime issues.
 * For example, internal equalizer settings, tuning status information, status
 * of additional signals useful for system debug but not exposed in this interface.
 *
 * @notes This is function is optional.
 */
void onlp_sfpi_debug(int port, aim_pvs_t* pvs)
{

}

/**
 * @brief Generic ioctl
 * @param port The port number
 * @param The variable argument list of parameters.
 *
 * @notes This generic ioctl interface can be used
 * for platform-specific or driver specific features
 * that cannot or have not yet been defined in this
 * interface. It is intended as a future feature expansion
 * support mechanism.
 *
 * @notes Optional
 */
int onlp_sfpi_ioctl(int port, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

