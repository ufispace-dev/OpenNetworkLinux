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
#include <onlp/platformi/sfpi.h>
#include "platform_lib.h"


#define MGMT_NUM              2
#define SFP_NUM               96
#define QSFP_NUM              6
#define QSFPDD_NUM            0
#define QSFPX_NUM             (QSFP_NUM+QSFPDD_NUM)
#define PORT_NUM              (SFP_NUM+QSFPX_NUM+MGMT_NUM)

#define SYSFS_EEPROM         "eeprom"


#define EEPROM_ADDR (0x50)

#define BIT_000_001_010_000   0x50
#define BIT_100_101_110_100   0x974

typedef enum port_type_e {
    TYPE_SFP = 0,
    TYPE_QSFP,
    TYPE_QSFPDD,
    TYPE_MGMT,
    TYPE_UNNKOW,
    TYPE_MAX,
} port_type_t;

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
#define IS_QSFPX(_port)         (port_attr[_port].port_type == TYPE_QSFPDD || port_attr[_port].port_type == TYPE_QSFP)
#define IS_QSFP(_port)          (port_attr[_port].port_type == TYPE_QSFP)
#define IS_QSFPDD(_port)        (port_attr[_port].port_type == TYPE_QSFPDD)

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

    VALIDATE_PORT(port);

    memset(data, 0, 256);
    bus = xfr_port_to_eeprom_bus(port);

    if((rc = onlp_file_read(data, 256, &size, SYS_FMT, bus, EEPROM_ADDR, SYSFS_EEPROM)) < 0) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d)", port);
        AIM_LOG_ERROR(SYS_FMT, bus, EEPROM_ADDR, SYSFS_EEPROM);

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
    *rv=0;

    switch (control) {
        case ONLP_SFP_CONTROL_RESET:
        case ONLP_SFP_CONTROL_RESET_STATE:
        case ONLP_SFP_CONTROL_LP_MODE:
            if (IS_QSFPX(port)) {
                *rv = 1;
            }
            break;
        case ONLP_SFP_CONTROL_RX_LOS:
        case ONLP_SFP_CONTROL_TX_FAULT:
        case ONLP_SFP_CONTROL_TX_DISABLE:
            if (IS_SFP(port)) {
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

    VALIDATE_PORT(port);

    //check control is valid for this port
    switch(control)
    {
        case ONLP_SFP_CONTROL_RESET:
            {
                if (IS_QSFPX(port)) {
                    //reverse value
                    value = (value == 0) ? 1:0;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                if (IS_SFP(port)) {
                    break;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
            }
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                if (IS_QSFPX(port)) {
                    break;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
            }
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
    }

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
    rc = ONLP_STATUS_OK;

    return rc;
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

    VALIDATE_PORT(port);

    //get sysfs
    ONLP_TRY(xfr_ctrl_to_sysfs(port, control, &sysfs, &attr));


    //read gpio value
    if ((rc = read_file_hex(&reg_val, sysfs)) < 0) {
        AIM_LOG_ERROR("onlp_sfpi_control_get() failed, error=%d, sysfs=%s, gpio_num=%d", rc, sysfs);
        check_and_do_i2c_mux_reset(port);
        return rc;
    }

    ONLP_TRY(get_bit(attr, port_attr[port].cpld_bit, &bit));
    *value = get_bit_value(reg_val, bit);

    //reverse bit
    if (control == ONLP_SFP_CONTROL_RESET_STATE) {
        *value = !(*value);
    }

    return rc;
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
    return;
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

