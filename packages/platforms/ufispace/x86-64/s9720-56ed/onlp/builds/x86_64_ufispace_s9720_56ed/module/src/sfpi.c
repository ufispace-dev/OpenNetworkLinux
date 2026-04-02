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

#define SYSFS_DEV_CLASS       "dev_class"
#define ALL_PORTS             -1
 
#define MGMT_NUM              2
#define SFP_NUM               2
#define QSFP_NUM              0
#define QSFPDD_NIF_NUM        36
#define QSFPDD_FAB_NUM        20
#define QSFPX_NUM             (QSFP_NUM+QSFPDD_NIF_NUM+QSFPDD_FAB_NUM)
#define PORT_NUM              (SFP_NUM+QSFPX_NUM+MGMT_NUM)

#define SYSFS_EEPROM         "eeprom"

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

//SFF8636 TX Disable
#define TX_DIS_INPUT_MAX (0xff) /* for input value validation only */
#define SFF8636_EEPROM_OFFSET_TXDIS (0x56)
#define SFF8636_EEPROM_TX_DIS (0x0f) /* txdis valid bit(bit0-bit3), xxxx 1111 */
#define SFF8636_EEPROM_TX_EN (0x0)

#define EEPROM_ADDR (0x50)

#define MASK_1000_0000 0x80
//#define MASK_0000_0010 0x02
#define BIT_000_001_010_000   0x50
#define BIT_100_101_110_100   0x974

typedef enum port_type_e {
    TYPE_SFP = 0,
    TYPE_QSFP,
    TYPE_QSFPDD,
    TYPE_QSFPDD_NIF,
    TYPE_QSFPDD_FAB,
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
    int intr;
} port_attr_t;

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

typedef enum cpld_attr_idx_e {
    // Network Interface (NIF) attributes
    ABS_NIF_0 = 0,  LPMODE_NIF_0,    INTR_NIF_0,    RST_NIF_0,                           
    ABS_NIF_1,      LPMODE_NIF_1,    INTR_NIF_1,    RST_NIF_1,                          
    ABS_NIF_2,      LPMODE_NIF_2,    INTR_NIF_2,    RST_NIF_2,                          
    ABS_NIF_3,      LPMODE_NIF_3,    INTR_NIF_3,    RST_NIF_3,                          
    ABS_NIF_4,      LPMODE_NIF_4,    INTR_NIF_4,    RST_NIF_4,                          
    ABS_NIF_5,      LPMODE_NIF_5,    INTR_NIF_5,    RST_NIF_5,                          
    ABS_NIF_6,      LPMODE_NIF_6,    INTR_NIF_6,    RST_NIF_6,                          
    ABS_NIF_7,      LPMODE_NIF_7,    INTR_NIF_7,    RST_NIF_7,                          
    ABS_NIF_8,      LPMODE_NIF_8,    INTR_NIF_8,    RST_NIF_8,                          
    ABS_NIF_9,      LPMODE_NIF_9,    INTR_NIF_9,    RST_NIF_9,                          
    ABS_NIF_10,     LPMODE_NIF_10,   INTR_NIF_10,   RST_NIF_10,                                       
    ABS_NIF_11,     LPMODE_NIF_11,   INTR_NIF_11,   RST_NIF_11,                                       
    ABS_NIF_12,     LPMODE_NIF_12,   INTR_NIF_12,   RST_NIF_12,                                       
    ABS_NIF_13,     LPMODE_NIF_13,   INTR_NIF_13,   RST_NIF_13,                                       
    ABS_NIF_14,     LPMODE_NIF_14,   INTR_NIF_14,   RST_NIF_14,                                       
    ABS_NIF_15,     LPMODE_NIF_15,   INTR_NIF_15,   RST_NIF_15,                                       
    ABS_NIF_16,     LPMODE_NIF_16,   INTR_NIF_16,   RST_NIF_16,                                       
    ABS_NIF_17,     LPMODE_NIF_17,   INTR_NIF_17,   RST_NIF_17,                                       
    ABS_NIF_18,     LPMODE_NIF_18,   INTR_NIF_18,   RST_NIF_18,                                       
    ABS_NIF_19,     LPMODE_NIF_19,   INTR_NIF_19,   RST_NIF_19,                                       
    ABS_NIF_20,     LPMODE_NIF_20,   INTR_NIF_20,   RST_NIF_20,                                       
    ABS_NIF_21,     LPMODE_NIF_21,   INTR_NIF_21,   RST_NIF_21,                                       
    ABS_NIF_22,     LPMODE_NIF_22,   INTR_NIF_22,   RST_NIF_22,                                       
    ABS_NIF_23,     LPMODE_NIF_23,   INTR_NIF_23,   RST_NIF_23,                                       
    ABS_NIF_24,     LPMODE_NIF_24,   INTR_NIF_24,   RST_NIF_24,                                       
    ABS_NIF_25,     LPMODE_NIF_25,   INTR_NIF_25,   RST_NIF_25,                                       
    ABS_NIF_26,     LPMODE_NIF_26,   INTR_NIF_26,   RST_NIF_26,                                       
    ABS_NIF_27,     LPMODE_NIF_27,   INTR_NIF_27,   RST_NIF_27,                                       
    ABS_NIF_28,     LPMODE_NIF_28,   INTR_NIF_28,   RST_NIF_28,                                       
    ABS_NIF_29,     LPMODE_NIF_29,   INTR_NIF_29,   RST_NIF_29,                                       
    ABS_NIF_30,     LPMODE_NIF_30,   INTR_NIF_30,   RST_NIF_30,                                       
    ABS_NIF_31,     LPMODE_NIF_31,   INTR_NIF_31,   RST_NIF_31,                                       
    ABS_NIF_32,     LPMODE_NIF_32,   INTR_NIF_32,   RST_NIF_32,                                       
    ABS_NIF_33,     LPMODE_NIF_33,   INTR_NIF_33,   RST_NIF_33,                                       
    ABS_NIF_34,     LPMODE_NIF_34,   INTR_NIF_34,   RST_NIF_34,                                       
    ABS_NIF_35,     LPMODE_NIF_35,   INTR_NIF_35,   RST_NIF_35,                                       

    // Fabric attributes
    ABS_FAB_0,      LPMODE_FAB_0,    INTR_FAB_0,    RST_FAB_0,                                 
    ABS_FAB_1,      LPMODE_FAB_1,    INTR_FAB_1,    RST_FAB_1,                                 
    ABS_FAB_2,      LPMODE_FAB_2,    INTR_FAB_2,    RST_FAB_2,                                 
    ABS_FAB_3,      LPMODE_FAB_3,    INTR_FAB_3,    RST_FAB_3,                                 
    ABS_FAB_4,      LPMODE_FAB_4,    INTR_FAB_4,    RST_FAB_4,                                 
    ABS_FAB_5,      LPMODE_FAB_5,    INTR_FAB_5,    RST_FAB_5,                                 
    ABS_FAB_6,      LPMODE_FAB_6,    INTR_FAB_6,    RST_FAB_6,                                 
    ABS_FAB_7,      LPMODE_FAB_7,    INTR_FAB_7,    RST_FAB_7,                                 
    ABS_FAB_8,      LPMODE_FAB_8,    INTR_FAB_8,    RST_FAB_8,                                 
    ABS_FAB_9,      LPMODE_FAB_9,    INTR_FAB_9,    RST_FAB_9,                                 
    ABS_FAB_10,     LPMODE_FAB_10,   INTR_FAB_10,   RST_FAB_10,                                    
    ABS_FAB_11,     LPMODE_FAB_11,   INTR_FAB_11,   RST_FAB_11,                                    
    ABS_FAB_12,     LPMODE_FAB_12,   INTR_FAB_12,   RST_FAB_12,                                    
    ABS_FAB_13,     LPMODE_FAB_13,   INTR_FAB_13,   RST_FAB_13,                                    
    ABS_FAB_14,     LPMODE_FAB_14,   INTR_FAB_14,   RST_FAB_14,                                    
    ABS_FAB_15,     LPMODE_FAB_15,   INTR_FAB_15,   RST_FAB_15,                                    
    ABS_FAB_16,     LPMODE_FAB_16,   INTR_FAB_16,   RST_FAB_16,                                    
    ABS_FAB_17,     LPMODE_FAB_17,   INTR_FAB_17,   RST_FAB_17,                                    
    ABS_FAB_18,     LPMODE_FAB_18,   INTR_FAB_18,   RST_FAB_18,                                    
    ABS_FAB_19,     LPMODE_FAB_19,   INTR_FAB_19,   RST_FAB_19,                                    

    // Management attributes
    ABS_MGMT_0,     ABS_MGMT_1,      ABS_SFP_36,    ABS_SFP_37,
    RXLOS_MGMT_0,   RXLOS_MGMT_1,    RXLOS_SFP_36,  RXLOS_SFP_37,
    TXFLT_MGMT_0,   TXFLT_MGMT_1,    TXFLT_SFP_36,  TXFLT_SFP_37,
    RST_MGMT_0,     RST_MGMT_1,      RST_SFP_36,    RST_SFP_37,
    TXDIS_MGMT_0,   TXDIS_MGMT_1,    TXDIS_SFP_36,  TXDIS_SFP_37,
    CPLD_NONE,
}cpld_attr_idx_t;



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
//  QSFPDD (NIF/FAB)
//  port       abs            lpmode           reset            rxlos            txflt           txdis       eeprom      type          bit     intr 
    [0]   ={ABS_NIF_0    , LPMODE_NIF_0     , RST_NIF_0    ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 35    , TYPE_QSFPDD     , 0 , INTR_NIF_0},
    [1]   ={ABS_NIF_1    , LPMODE_NIF_1     , RST_NIF_1    ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 36    , TYPE_QSFPDD     , 0 , INTR_NIF_1},
    [2]   ={ABS_NIF_2    , LPMODE_NIF_2     , RST_NIF_2    ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 37    , TYPE_QSFPDD     , 0 , INTR_NIF_2},
    [3]   ={ABS_NIF_3    , LPMODE_NIF_3     , RST_NIF_3    ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 38    , TYPE_QSFPDD     , 0 , INTR_NIF_3},
    [4]   ={ABS_NIF_4    , LPMODE_NIF_4     , RST_NIF_4    ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 39    , TYPE_QSFPDD     , 0 , INTR_NIF_4},
    [5]   ={ABS_NIF_5    , LPMODE_NIF_5     , RST_NIF_5    ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 40    , TYPE_QSFPDD     , 0 , INTR_NIF_5},
    [6]   ={ABS_NIF_6    , LPMODE_NIF_6     , RST_NIF_6    ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 41    , TYPE_QSFPDD     , 0 , INTR_NIF_6},
    [7]   ={ABS_NIF_7    , LPMODE_NIF_7     , RST_NIF_7    ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 42    , TYPE_QSFPDD     , 0 , INTR_NIF_7},
    [8]   ={ABS_NIF_8    , LPMODE_NIF_8     , RST_NIF_8    ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 43    , TYPE_QSFPDD     , 0 , INTR_NIF_8},
    [9]   ={ABS_NIF_9    , LPMODE_NIF_9     , RST_NIF_9    ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 44    , TYPE_QSFPDD     , 0 , INTR_NIF_9},
    [10]  ={ABS_NIF_10   , LPMODE_NIF_10    , RST_NIF_10   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 45    , TYPE_QSFPDD     , 0 , INTR_NIF_10},
    [11]  ={ABS_NIF_11   , LPMODE_NIF_11    , RST_NIF_11   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 46    , TYPE_QSFPDD     , 0 , INTR_NIF_11},
    [12]  ={ABS_NIF_12   , LPMODE_NIF_12    , RST_NIF_12   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 47    , TYPE_QSFPDD     , 0 , INTR_NIF_12},
    [13]  ={ABS_NIF_13   , LPMODE_NIF_13    , RST_NIF_13   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 48    , TYPE_QSFPDD     , 0 , INTR_NIF_13},
    [14]  ={ABS_NIF_14   , LPMODE_NIF_14    , RST_NIF_14   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 49    , TYPE_QSFPDD     , 0 , INTR_NIF_14},
    [15]  ={ABS_NIF_15   , LPMODE_NIF_15    , RST_NIF_15   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 50    , TYPE_QSFPDD     , 0 , INTR_NIF_15},
    [16]  ={ABS_NIF_16   , LPMODE_NIF_16    , RST_NIF_16   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 51    , TYPE_QSFPDD     , 0 , INTR_NIF_16},
    [17]  ={ABS_NIF_17   , LPMODE_NIF_17    , RST_NIF_17   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 52    , TYPE_QSFPDD     , 0 , INTR_NIF_17},
    [18]  ={ABS_NIF_18   , LPMODE_NIF_18    , RST_NIF_18   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 53    , TYPE_QSFPDD     , 0 , INTR_NIF_18},
    [19]  ={ABS_NIF_19   , LPMODE_NIF_19    , RST_NIF_19   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 54    , TYPE_QSFPDD     , 0 , INTR_NIF_19},
    [20]  ={ABS_NIF_20   , LPMODE_NIF_20    , RST_NIF_20   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 65    , TYPE_QSFPDD     , 0 , INTR_NIF_20},
    [21]  ={ABS_NIF_21   , LPMODE_NIF_21    , RST_NIF_21   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 66    , TYPE_QSFPDD     , 0 , INTR_NIF_21},
    [22]  ={ABS_NIF_22   , LPMODE_NIF_22    , RST_NIF_22   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 67    , TYPE_QSFPDD     , 0 , INTR_NIF_22},
    [23]  ={ABS_NIF_23   , LPMODE_NIF_23    , RST_NIF_23   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 68    , TYPE_QSFPDD     , 0 , INTR_NIF_23},
    [24]  ={ABS_NIF_24   , LPMODE_NIF_24    , RST_NIF_24   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 69    , TYPE_QSFPDD     , 0 , INTR_NIF_24},
    [25]  ={ABS_NIF_25   , LPMODE_NIF_25    , RST_NIF_25   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 70    , TYPE_QSFPDD     , 0 , INTR_NIF_25},
    [26]  ={ABS_NIF_26   , LPMODE_NIF_26    , RST_NIF_26   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 71    , TYPE_QSFPDD     , 0 , INTR_NIF_26},
    [27]  ={ABS_NIF_27   , LPMODE_NIF_27    , RST_NIF_27   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 72    , TYPE_QSFPDD     , 0 , INTR_NIF_27},
    [28]  ={ABS_NIF_28   , LPMODE_NIF_28    , RST_NIF_28   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 73    , TYPE_QSFPDD     , 0 , INTR_NIF_28},
    [29]  ={ABS_NIF_29   , LPMODE_NIF_29    , RST_NIF_29   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 74    , TYPE_QSFPDD     , 0 , INTR_NIF_29},
    [30]  ={ABS_NIF_30   , LPMODE_NIF_30    , RST_NIF_30   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 75    , TYPE_QSFPDD     , 0 , INTR_NIF_30},
    [31]  ={ABS_NIF_31   , LPMODE_NIF_31    , RST_NIF_31   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 76    , TYPE_QSFPDD     , 0 , INTR_NIF_31},
    [32]  ={ABS_NIF_32   , LPMODE_NIF_32    , RST_NIF_32   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 77    , TYPE_QSFPDD     , 0 , INTR_NIF_32},
    [33]  ={ABS_NIF_33   , LPMODE_NIF_33    , RST_NIF_33   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 78    , TYPE_QSFPDD     , 0 , INTR_NIF_33},
    [34]  ={ABS_NIF_34   , LPMODE_NIF_34    , RST_NIF_34   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 79    , TYPE_QSFPDD     , 0 , INTR_NIF_34},
    [35]  ={ABS_NIF_35   , LPMODE_NIF_35    , RST_NIF_35   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 80    , TYPE_QSFPDD     , 0 , INTR_NIF_35},
    [36]  ={ABS_SFP_36   , CPLD_NONE        , RST_SFP_36   ,  RXLOS_SFP_36  ,  TXFLT_SFP_36  , TXDIS_SFP_36 , 83    , TYPE_SFP        , 0 , CPLD_NONE},
    [37]  ={ABS_SFP_37   , CPLD_NONE        , RST_SFP_37   ,  RXLOS_SFP_37  ,  TXFLT_SFP_37  , TXDIS_SFP_37 , 84    , TYPE_SFP        , 0 , CPLD_NONE},
    [38]  ={ABS_FAB_0    , LPMODE_FAB_0     , RST_FAB_0    ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 25    , TYPE_QSFPDD     , 0 , INTR_FAB_0},
    [39]  ={ABS_FAB_1    , LPMODE_FAB_1     , RST_FAB_1    ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 26    , TYPE_QSFPDD     , 0 , INTR_FAB_1},
    [40]  ={ABS_FAB_2    , LPMODE_FAB_2     , RST_FAB_2    ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 27    , TYPE_QSFPDD     , 0 , INTR_FAB_2},
    [41]  ={ABS_FAB_3    , LPMODE_FAB_3     , RST_FAB_3    ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 28    , TYPE_QSFPDD     , 0 , INTR_FAB_3},
    [42]  ={ABS_FAB_4    , LPMODE_FAB_4     , RST_FAB_4    ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 29    , TYPE_QSFPDD     , 0 , INTR_FAB_4},
    [43]  ={ABS_FAB_5    , LPMODE_FAB_5     , RST_FAB_5    ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 30    , TYPE_QSFPDD     , 0 , INTR_FAB_5},
    [44]  ={ABS_FAB_6    , LPMODE_FAB_6     , RST_FAB_6    ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 31    , TYPE_QSFPDD     , 0 , INTR_FAB_6},
    [45]  ={ABS_FAB_7    , LPMODE_FAB_7     , RST_FAB_7    ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 32    , TYPE_QSFPDD     , 0 , INTR_FAB_7},
    [46]  ={ABS_FAB_8    , LPMODE_FAB_8     , RST_FAB_8    ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 33    , TYPE_QSFPDD     , 0 , INTR_FAB_8},
    [47]  ={ABS_FAB_9    , LPMODE_FAB_9     , RST_FAB_9    ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 34    , TYPE_QSFPDD     , 0 , INTR_FAB_9},
    [48]  ={ABS_FAB_10   , LPMODE_FAB_10    , RST_FAB_10   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 55    , TYPE_QSFPDD     , 0 , INTR_FAB_10},
    [49]  ={ABS_FAB_11   , LPMODE_FAB_11    , RST_FAB_11   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 56    , TYPE_QSFPDD     , 0 , INTR_FAB_11},
    [50]  ={ABS_FAB_12   , LPMODE_FAB_12    , RST_FAB_12   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 57    , TYPE_QSFPDD     , 0 , INTR_FAB_12},
    [51]  ={ABS_FAB_13   , LPMODE_FAB_13    , RST_FAB_13   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 58    , TYPE_QSFPDD     , 0 , INTR_FAB_13},
    [52]  ={ABS_FAB_14   , LPMODE_FAB_14    , RST_FAB_14   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 59    , TYPE_QSFPDD     , 0 , INTR_FAB_14},
    [53]  ={ABS_FAB_15   , LPMODE_FAB_15    , RST_FAB_15   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 60    , TYPE_QSFPDD     , 0 , INTR_FAB_15},
    [54]  ={ABS_FAB_16   , LPMODE_FAB_16    , RST_FAB_16   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 61    , TYPE_QSFPDD     , 0 , INTR_FAB_16},
    [55]  ={ABS_FAB_17   , LPMODE_FAB_17    , RST_FAB_17   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 62    , TYPE_QSFPDD     , 0 , INTR_FAB_17},
    [56]  ={ABS_FAB_18   , LPMODE_FAB_18    , RST_FAB_18   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 63    , TYPE_QSFPDD     , 0 , INTR_FAB_18},
    [57]  ={ABS_FAB_19   , LPMODE_FAB_19    , RST_FAB_19   ,  CPLD_NONE     ,  CPLD_NONE     , CPLD_NONE    , 64    , TYPE_QSFPDD     , 0 , INTR_FAB_19},
    // MGMT    abs           lp mode            reset           rxlos            txfault        txdis        eeprom     type          bit      intr
    [58]  ={ABS_MGMT_0   , CPLD_NONE        ,RST_MGMT_0    , RXLOS_MGMT_0   , TXFLT_MGMT_0   , TXDIS_MGMT_0 , 81    , TYPE_MGMT       , 0 , CPLD_NONE},
    [59]  ={ABS_MGMT_1   , CPLD_NONE        ,RST_MGMT_1    , RXLOS_MGMT_1   , TXFLT_MGMT_1   , TXDIS_MGMT_1 , 82    , TYPE_MGMT       , 0 , CPLD_NONE}
};

#define IS_SFP(_port)         (port_attr[_port].port_type == TYPE_SFP || port_attr[_port].port_type == TYPE_MGMT)
#define IS_QSFPX(_port)       (port_attr[_port].port_type == TYPE_QSFPDD || port_attr[_port].port_type == TYPE_QSFP)
#define IS_QSFP(_port)        (port_attr[_port].port_type == TYPE_QSFP)
#define IS_QSFPDD(_port)      (port_attr[_port].port_type == TYPE_QSFPDD)

#define VALIDATE_PORT(p) { if ((p < 0) || (p >= PORT_NUM)) return ONLP_STATUS_E_PARAM; }
#define VALIDATE_SFP_PORT(p) { if (!IS_SFP(p)) return ONLP_STATUS_E_PARAM; }

static int get_port_sysfs(cpld_attr_idx_t idx, char** str)
{
    if(str == NULL)
        return ONLP_STATUS_E_PARAM;

    switch(idx) {
        case ABS_NIF_0:  *str = SYSFS_CPLD4 "qsfpdd_nif_p0_abs"  ; break;
        case ABS_NIF_1:  *str = SYSFS_CPLD4 "qsfpdd_nif_p1_abs"  ; break;
        case ABS_NIF_2:  *str = SYSFS_CPLD4 "qsfpdd_nif_p2_abs"  ; break;
        case ABS_NIF_3:  *str = SYSFS_CPLD4 "qsfpdd_nif_p3_abs"  ; break;
        case ABS_NIF_4:  *str = SYSFS_CPLD4 "qsfpdd_nif_p4_abs"  ; break;
        case ABS_NIF_5:  *str = SYSFS_CPLD4 "qsfpdd_nif_p5_abs"  ; break;
        case ABS_NIF_6:  *str = SYSFS_CPLD4 "qsfpdd_nif_p6_abs"  ; break;
        case ABS_NIF_7:  *str = SYSFS_CPLD4 "qsfpdd_nif_p7_abs"  ; break;
        case ABS_NIF_8:  *str = SYSFS_CPLD4 "qsfpdd_nif_p8_abs"  ; break;
        case ABS_NIF_9:  *str = SYSFS_CPLD4 "qsfpdd_nif_p9_abs"  ; break;
        case ABS_NIF_10: *str = SYSFS_CPLD4 "qsfpdd_nif_p10_abs" ; break;
        case ABS_NIF_11: *str = SYSFS_CPLD4 "qsfpdd_nif_p11_abs" ; break;
        case ABS_NIF_12: *str = SYSFS_CPLD4 "qsfpdd_nif_p12_abs" ; break;
        case ABS_NIF_13: *str = SYSFS_CPLD4 "qsfpdd_nif_p13_abs" ; break;
        case ABS_NIF_14: *str = SYSFS_CPLD4 "qsfpdd_nif_p14_abs" ; break;
        case ABS_NIF_15: *str = SYSFS_CPLD4 "qsfpdd_nif_p15_abs" ; break;
        case ABS_NIF_16: *str = SYSFS_CPLD4 "qsfpdd_nif_p16_abs" ; break;
        case ABS_NIF_17: *str = SYSFS_CPLD4 "qsfpdd_nif_p17_abs" ; break;
        case ABS_NIF_18: *str = SYSFS_CPLD4 "qsfpdd_nif_p18_abs" ; break;
        case ABS_NIF_19: *str = SYSFS_CPLD4 "qsfpdd_nif_p19_abs" ; break;
        case ABS_NIF_20: *str = SYSFS_CPLD2 "qsfpdd_nif_p20_abs" ; break;
        case ABS_NIF_21: *str = SYSFS_CPLD2 "qsfpdd_nif_p21_abs" ; break;
        case ABS_NIF_22: *str = SYSFS_CPLD2 "qsfpdd_nif_p22_abs" ; break;
        case ABS_NIF_23: *str = SYSFS_CPLD2 "qsfpdd_nif_p23_abs" ; break;
        case ABS_NIF_24: *str = SYSFS_CPLD2 "qsfpdd_nif_p24_abs" ; break;
        case ABS_NIF_25: *str = SYSFS_CPLD2 "qsfpdd_nif_p25_abs" ; break;
        case ABS_NIF_26: *str = SYSFS_CPLD2 "qsfpdd_nif_p26_abs" ; break;
        case ABS_NIF_27: *str = SYSFS_CPLD2 "qsfpdd_nif_p27_abs" ; break;
        case ABS_NIF_28: *str = SYSFS_CPLD2 "qsfpdd_nif_p28_abs" ; break;
        case ABS_NIF_29: *str = SYSFS_CPLD2 "qsfpdd_nif_p29_abs" ; break;
        case ABS_NIF_30: *str = SYSFS_CPLD2 "qsfpdd_nif_p30_abs" ; break;
        case ABS_NIF_31: *str = SYSFS_CPLD2 "qsfpdd_nif_p31_abs" ; break;
        case ABS_NIF_32: *str = SYSFS_CPLD2 "qsfpdd_nif_p32_abs" ; break;
        case ABS_NIF_33: *str = SYSFS_CPLD2 "qsfpdd_nif_p33_abs" ; break;
        case ABS_NIF_34: *str = SYSFS_CPLD2 "qsfpdd_nif_p34_abs" ; break;
        case ABS_NIF_35: *str = SYSFS_CPLD2 "qsfpdd_nif_p35_abs" ; break;
        case ABS_FAB_0:  *str = SYSFS_CPLD4 "qsfpdd_fab_p0_abs"  ; break;
        case ABS_FAB_1:  *str = SYSFS_CPLD4 "qsfpdd_fab_p1_abs"  ; break;
        case ABS_FAB_2:  *str = SYSFS_CPLD4 "qsfpdd_fab_p2_abs"  ; break;
        case ABS_FAB_3:  *str = SYSFS_CPLD4 "qsfpdd_fab_p3_abs"  ; break;
        case ABS_FAB_4:  *str = SYSFS_CPLD4 "qsfpdd_fab_p4_abs"  ; break;
        case ABS_FAB_5:  *str = SYSFS_CPLD4 "qsfpdd_fab_p5_abs"  ; break;
        case ABS_FAB_6:  *str = SYSFS_CPLD4 "qsfpdd_fab_p6_abs"  ; break;
        case ABS_FAB_7:  *str = SYSFS_CPLD4 "qsfpdd_fab_p7_abs"  ; break;
        case ABS_FAB_8:  *str = SYSFS_CPLD4 "qsfpdd_fab_p8_abs"  ; break;
        case ABS_FAB_9:  *str = SYSFS_CPLD4 "qsfpdd_fab_p9_abs"  ; break;
        case ABS_FAB_10: *str = SYSFS_CPLD2 "qsfpdd_fab_p10_abs" ; break;
        case ABS_FAB_11: *str = SYSFS_CPLD2 "qsfpdd_fab_p11_abs" ; break;
        case ABS_FAB_12: *str = SYSFS_CPLD2 "qsfpdd_fab_p12_abs" ; break;
        case ABS_FAB_13: *str = SYSFS_CPLD2 "qsfpdd_fab_p13_abs" ; break;
        case ABS_FAB_14: *str = SYSFS_CPLD2 "qsfpdd_fab_p14_abs" ; break;
        case ABS_FAB_15: *str = SYSFS_CPLD2 "qsfpdd_fab_p15_abs" ; break;
        case ABS_FAB_16: *str = SYSFS_CPLD2 "qsfpdd_fab_p16_abs" ; break;
        case ABS_FAB_17: *str = SYSFS_CPLD2 "qsfpdd_fab_p17_abs" ; break;
        case ABS_FAB_18: *str = SYSFS_CPLD2 "qsfpdd_fab_p18_abs" ; break;
        case ABS_FAB_19: *str = SYSFS_CPLD2 "qsfpdd_fab_p19_abs" ; break;
        //LPMODE
        case LPMODE_NIF_0:  *str = SYSFS_CPLD4 "qsfpdd_nif_p0_lp_mode"  ; break;
        case LPMODE_NIF_1:  *str = SYSFS_CPLD4 "qsfpdd_nif_p1_lp_mode"  ; break;
        case LPMODE_NIF_2:  *str = SYSFS_CPLD4 "qsfpdd_nif_p2_lp_mode"  ; break;
        case LPMODE_NIF_3:  *str = SYSFS_CPLD4 "qsfpdd_nif_p3_lp_mode"  ; break;
        case LPMODE_NIF_4:  *str = SYSFS_CPLD4 "qsfpdd_nif_p4_lp_mode"  ; break;
        case LPMODE_NIF_5:  *str = SYSFS_CPLD4 "qsfpdd_nif_p5_lp_mode"  ; break;
        case LPMODE_NIF_6:  *str = SYSFS_CPLD4 "qsfpdd_nif_p6_lp_mode"  ; break;
        case LPMODE_NIF_7:  *str = SYSFS_CPLD4 "qsfpdd_nif_p7_lp_mode"  ; break;
        case LPMODE_NIF_8:  *str = SYSFS_CPLD4 "qsfpdd_nif_p8_lp_mode"  ; break;
        case LPMODE_NIF_9:  *str = SYSFS_CPLD4 "qsfpdd_nif_p9_lp_mode"  ; break;
        case LPMODE_NIF_10: *str = SYSFS_CPLD4 "qsfpdd_nif_p10_lp_mode" ; break;
        case LPMODE_NIF_11: *str = SYSFS_CPLD4 "qsfpdd_nif_p11_lp_mode" ; break;
        case LPMODE_NIF_12: *str = SYSFS_CPLD4 "qsfpdd_nif_p12_lp_mode" ; break;
        case LPMODE_NIF_13: *str = SYSFS_CPLD4 "qsfpdd_nif_p13_lp_mode" ; break;
        case LPMODE_NIF_14: *str = SYSFS_CPLD4 "qsfpdd_nif_p14_lp_mode" ; break;
        case LPMODE_NIF_15: *str = SYSFS_CPLD4 "qsfpdd_nif_p15_lp_mode" ; break;
        case LPMODE_NIF_16: *str = SYSFS_CPLD4 "qsfpdd_nif_p16_lp_mode" ; break;
        case LPMODE_NIF_17: *str = SYSFS_CPLD4 "qsfpdd_nif_p17_lp_mode" ; break;
        case LPMODE_NIF_18: *str = SYSFS_CPLD4 "qsfpdd_nif_p18_lp_mode" ; break;
        case LPMODE_NIF_19: *str = SYSFS_CPLD4 "qsfpdd_nif_p19_lp_mode" ; break;
        case LPMODE_NIF_20: *str = SYSFS_CPLD2 "qsfpdd_nif_p20_lp_mode" ; break;
        case LPMODE_NIF_21: *str = SYSFS_CPLD2 "qsfpdd_nif_p21_lp_mode" ; break;
        case LPMODE_NIF_22: *str = SYSFS_CPLD2 "qsfpdd_nif_p22_lp_mode" ; break;
        case LPMODE_NIF_23: *str = SYSFS_CPLD2 "qsfpdd_nif_p23_lp_mode" ; break;
        case LPMODE_NIF_24: *str = SYSFS_CPLD2 "qsfpdd_nif_p24_lp_mode" ; break;
        case LPMODE_NIF_25: *str = SYSFS_CPLD2 "qsfpdd_nif_p25_lp_mode" ; break;
        case LPMODE_NIF_26: *str = SYSFS_CPLD2 "qsfpdd_nif_p26_lp_mode" ; break;
        case LPMODE_NIF_27: *str = SYSFS_CPLD2 "qsfpdd_nif_p27_lp_mode" ; break;
        case LPMODE_NIF_28: *str = SYSFS_CPLD2 "qsfpdd_nif_p28_lp_mode" ; break;
        case LPMODE_NIF_29: *str = SYSFS_CPLD2 "qsfpdd_nif_p29_lp_mode" ; break;
        case LPMODE_NIF_30: *str = SYSFS_CPLD2 "qsfpdd_nif_p30_lp_mode" ; break;
        case LPMODE_NIF_31: *str = SYSFS_CPLD2 "qsfpdd_nif_p31_lp_mode" ; break;
        case LPMODE_NIF_32: *str = SYSFS_CPLD2 "qsfpdd_nif_p32_lp_mode" ; break;
        case LPMODE_NIF_33: *str = SYSFS_CPLD2 "qsfpdd_nif_p33_lp_mode" ; break;
        case LPMODE_NIF_34: *str = SYSFS_CPLD2 "qsfpdd_nif_p34_lp_mode" ; break;
        case LPMODE_NIF_35: *str = SYSFS_CPLD2 "qsfpdd_nif_p35_lp_mode" ; break;
        case LPMODE_FAB_0:  *str = SYSFS_CPLD4 "qsfpdd_fab_p0_lp_mode"  ; break;
        case LPMODE_FAB_1:  *str = SYSFS_CPLD4 "qsfpdd_fab_p1_lp_mode"  ; break;
        case LPMODE_FAB_2:  *str = SYSFS_CPLD4 "qsfpdd_fab_p2_lp_mode"  ; break;
        case LPMODE_FAB_3:  *str = SYSFS_CPLD4 "qsfpdd_fab_p3_lp_mode"  ; break;
        case LPMODE_FAB_4:  *str = SYSFS_CPLD4 "qsfpdd_fab_p4_lp_mode"  ; break;
        case LPMODE_FAB_5:  *str = SYSFS_CPLD4 "qsfpdd_fab_p5_lp_mode"  ; break;
        case LPMODE_FAB_6:  *str = SYSFS_CPLD4 "qsfpdd_fab_p6_lp_mode"  ; break;
        case LPMODE_FAB_7:  *str = SYSFS_CPLD4 "qsfpdd_fab_p7_lp_mode"  ; break;
        case LPMODE_FAB_8:  *str = SYSFS_CPLD4 "qsfpdd_fab_p8_lp_mode"  ; break;
        case LPMODE_FAB_9:  *str = SYSFS_CPLD4 "qsfpdd_fab_p9_lp_mode"  ; break;
        case LPMODE_FAB_10: *str = SYSFS_CPLD2 "qsfpdd_fab_p10_lp_mode" ; break;
        case LPMODE_FAB_11: *str = SYSFS_CPLD2 "qsfpdd_fab_p11_lp_mode" ; break;
        case LPMODE_FAB_12: *str = SYSFS_CPLD2 "qsfpdd_fab_p12_lp_mode" ; break;
        case LPMODE_FAB_13: *str = SYSFS_CPLD2 "qsfpdd_fab_p13_lp_mode" ; break;
        case LPMODE_FAB_14: *str = SYSFS_CPLD2 "qsfpdd_fab_p14_lp_mode" ; break;
        case LPMODE_FAB_15: *str = SYSFS_CPLD2 "qsfpdd_fab_p15_lp_mode" ; break;
        case LPMODE_FAB_16: *str = SYSFS_CPLD2 "qsfpdd_fab_p16_lp_mode" ; break;
        case LPMODE_FAB_17: *str = SYSFS_CPLD2 "qsfpdd_fab_p17_lp_mode" ; break;
        case LPMODE_FAB_18: *str = SYSFS_CPLD2 "qsfpdd_fab_p18_lp_mode" ; break;
        case LPMODE_FAB_19: *str = SYSFS_CPLD2 "qsfpdd_fab_p19_lp_mode" ; break;
        //RST
        case RST_NIF_0:  *str = SYSFS_CPLD4 "qsfpdd_nif_p0_rst"  ; break;
        case RST_NIF_1:  *str = SYSFS_CPLD4 "qsfpdd_nif_p1_rst"  ; break;
        case RST_NIF_2:  *str = SYSFS_CPLD4 "qsfpdd_nif_p2_rst"  ; break;
        case RST_NIF_3:  *str = SYSFS_CPLD4 "qsfpdd_nif_p3_rst"  ; break;
        case RST_NIF_4:  *str = SYSFS_CPLD4 "qsfpdd_nif_p4_rst"  ; break;
        case RST_NIF_5:  *str = SYSFS_CPLD4 "qsfpdd_nif_p5_rst"  ; break;
        case RST_NIF_6:  *str = SYSFS_CPLD4 "qsfpdd_nif_p6_rst"  ; break;
        case RST_NIF_7:  *str = SYSFS_CPLD4 "qsfpdd_nif_p7_rst"  ; break;
        case RST_NIF_8:  *str = SYSFS_CPLD4 "qsfpdd_nif_p8_rst"  ; break;
        case RST_NIF_9:  *str = SYSFS_CPLD4 "qsfpdd_nif_p9_rst"  ; break;
        case RST_NIF_10: *str = SYSFS_CPLD4 "qsfpdd_nif_p10_rst" ; break;
        case RST_NIF_11: *str = SYSFS_CPLD4 "qsfpdd_nif_p11_rst" ; break;
        case RST_NIF_12: *str = SYSFS_CPLD4 "qsfpdd_nif_p12_rst" ; break;
        case RST_NIF_13: *str = SYSFS_CPLD4 "qsfpdd_nif_p13_rst" ; break;
        case RST_NIF_14: *str = SYSFS_CPLD4 "qsfpdd_nif_p14_rst" ; break;
        case RST_NIF_15: *str = SYSFS_CPLD4 "qsfpdd_nif_p15_rst" ; break;
        case RST_NIF_16: *str = SYSFS_CPLD4 "qsfpdd_nif_p16_rst" ; break;
        case RST_NIF_17: *str = SYSFS_CPLD4 "qsfpdd_nif_p17_rst" ; break;
        case RST_NIF_18: *str = SYSFS_CPLD4 "qsfpdd_nif_p18_rst" ; break;
        case RST_NIF_19: *str = SYSFS_CPLD4 "qsfpdd_nif_p19_rst" ; break;
        case RST_NIF_20: *str = SYSFS_CPLD2 "qsfpdd_nif_p20_rst" ; break;
        case RST_NIF_21: *str = SYSFS_CPLD2 "qsfpdd_nif_p21_rst" ; break;
        case RST_NIF_22: *str = SYSFS_CPLD2 "qsfpdd_nif_p22_rst" ; break;
        case RST_NIF_23: *str = SYSFS_CPLD2 "qsfpdd_nif_p23_rst" ; break;
        case RST_NIF_24: *str = SYSFS_CPLD2 "qsfpdd_nif_p24_rst" ; break;
        case RST_NIF_25: *str = SYSFS_CPLD2 "qsfpdd_nif_p25_rst" ; break;
        case RST_NIF_26: *str = SYSFS_CPLD2 "qsfpdd_nif_p26_rst" ; break;
        case RST_NIF_27: *str = SYSFS_CPLD2 "qsfpdd_nif_p27_rst" ; break;
        case RST_NIF_28: *str = SYSFS_CPLD2 "qsfpdd_nif_p28_rst" ; break;
        case RST_NIF_29: *str = SYSFS_CPLD2 "qsfpdd_nif_p29_rst" ; break;
        case RST_NIF_30: *str = SYSFS_CPLD2 "qsfpdd_nif_p30_rst" ; break;
        case RST_NIF_31: *str = SYSFS_CPLD2 "qsfpdd_nif_p31_rst" ; break;
        case RST_NIF_32: *str = SYSFS_CPLD2 "qsfpdd_nif_p32_rst" ; break;
        case RST_NIF_33: *str = SYSFS_CPLD2 "qsfpdd_nif_p33_rst" ; break;
        case RST_NIF_34: *str = SYSFS_CPLD2 "qsfpdd_nif_p34_rst" ; break;
        case RST_NIF_35: *str = SYSFS_CPLD2 "qsfpdd_nif_p35_rst" ; break;
        case RST_FAB_0:  *str = SYSFS_CPLD4 "qsfpdd_fab_p0_rst"  ; break;
        case RST_FAB_1:  *str = SYSFS_CPLD4 "qsfpdd_fab_p1_rst"  ; break;
        case RST_FAB_2:  *str = SYSFS_CPLD4 "qsfpdd_fab_p2_rst"  ; break;
        case RST_FAB_3:  *str = SYSFS_CPLD4 "qsfpdd_fab_p3_rst"  ; break;
        case RST_FAB_4:  *str = SYSFS_CPLD4 "qsfpdd_fab_p4_rst"  ; break;
        case RST_FAB_5:  *str = SYSFS_CPLD4 "qsfpdd_fab_p5_rst"  ; break;
        case RST_FAB_6:  *str = SYSFS_CPLD4 "qsfpdd_fab_p6_rst"  ; break;
        case RST_FAB_7:  *str = SYSFS_CPLD4 "qsfpdd_fab_p7_rst"  ; break;
        case RST_FAB_8:  *str = SYSFS_CPLD4 "qsfpdd_fab_p8_rst"  ; break;
        case RST_FAB_9:  *str = SYSFS_CPLD4 "qsfpdd_fab_p9_rst"  ; break;
        case RST_FAB_10: *str = SYSFS_CPLD2 "qsfpdd_fab_p10_rst" ; break;
        case RST_FAB_11: *str = SYSFS_CPLD2 "qsfpdd_fab_p11_rst" ; break;
        case RST_FAB_12: *str = SYSFS_CPLD2 "qsfpdd_fab_p12_rst" ; break;
        case RST_FAB_13: *str = SYSFS_CPLD2 "qsfpdd_fab_p13_rst" ; break;
        case RST_FAB_14: *str = SYSFS_CPLD2 "qsfpdd_fab_p14_rst" ; break;
        case RST_FAB_15: *str = SYSFS_CPLD2 "qsfpdd_fab_p15_rst" ; break;
        case RST_FAB_16: *str = SYSFS_CPLD2 "qsfpdd_fab_p16_rst" ; break;
        case RST_FAB_17: *str = SYSFS_CPLD2 "qsfpdd_fab_p17_rst" ; break;
        case RST_FAB_18: *str = SYSFS_CPLD2 "qsfpdd_fab_p18_rst" ; break;
        case RST_FAB_19: *str = SYSFS_CPLD2 "qsfpdd_fab_p19_rst" ; break;
        //INTR
        case INTR_NIF_0:  *str = SYSFS_CPLD4 "qsfpdd_nif_p0_intr"  ; break;
        case INTR_NIF_1:  *str = SYSFS_CPLD4 "qsfpdd_nif_p1_intr"  ; break;
        case INTR_NIF_2:  *str = SYSFS_CPLD4 "qsfpdd_nif_p2_intr"  ; break;
        case INTR_NIF_3:  *str = SYSFS_CPLD4 "qsfpdd_nif_p3_intr"  ; break;
        case INTR_NIF_4:  *str = SYSFS_CPLD4 "qsfpdd_nif_p4_intr"  ; break;
        case INTR_NIF_5:  *str = SYSFS_CPLD4 "qsfpdd_nif_p5_intr"  ; break;
        case INTR_NIF_6:  *str = SYSFS_CPLD4 "qsfpdd_nif_p6_intr"  ; break;
        case INTR_NIF_7:  *str = SYSFS_CPLD4 "qsfpdd_nif_p7_intr"  ; break;
        case INTR_NIF_8:  *str = SYSFS_CPLD4 "qsfpdd_nif_p8_intr"  ; break;
        case INTR_NIF_9:  *str = SYSFS_CPLD4 "qsfpdd_nif_p9_intr"  ; break;
        case INTR_NIF_10: *str = SYSFS_CPLD4 "qsfpdd_nif_p10_intr" ; break;
        case INTR_NIF_11: *str = SYSFS_CPLD4 "qsfpdd_nif_p11_intr" ; break;
        case INTR_NIF_12: *str = SYSFS_CPLD4 "qsfpdd_nif_p12_intr" ; break;
        case INTR_NIF_13: *str = SYSFS_CPLD4 "qsfpdd_nif_p13_intr" ; break;
        case INTR_NIF_14: *str = SYSFS_CPLD4 "qsfpdd_nif_p14_intr" ; break;
        case INTR_NIF_15: *str = SYSFS_CPLD4 "qsfpdd_nif_p15_intr" ; break;
        case INTR_NIF_16: *str = SYSFS_CPLD4 "qsfpdd_nif_p16_intr" ; break;
        case INTR_NIF_17: *str = SYSFS_CPLD4 "qsfpdd_nif_p17_intr" ; break;
        case INTR_NIF_18: *str = SYSFS_CPLD4 "qsfpdd_nif_p18_intr" ; break;
        case INTR_NIF_19: *str = SYSFS_CPLD4 "qsfpdd_nif_p19_intr" ; break;
        case INTR_NIF_20: *str = SYSFS_CPLD2 "qsfpdd_nif_p20_intr" ; break;
        case INTR_NIF_21: *str = SYSFS_CPLD2 "qsfpdd_nif_p21_intr" ; break;
        case INTR_NIF_22: *str = SYSFS_CPLD2 "qsfpdd_nif_p22_intr" ; break;
        case INTR_NIF_23: *str = SYSFS_CPLD2 "qsfpdd_nif_p23_intr" ; break;
        case INTR_NIF_24: *str = SYSFS_CPLD2 "qsfpdd_nif_p24_intr" ; break;
        case INTR_NIF_25: *str = SYSFS_CPLD2 "qsfpdd_nif_p25_intr" ; break;
        case INTR_NIF_26: *str = SYSFS_CPLD2 "qsfpdd_nif_p26_intr" ; break;
        case INTR_NIF_27: *str = SYSFS_CPLD2 "qsfpdd_nif_p27_intr" ; break;
        case INTR_NIF_28: *str = SYSFS_CPLD2 "qsfpdd_nif_p28_intr" ; break;
        case INTR_NIF_29: *str = SYSFS_CPLD2 "qsfpdd_nif_p29_intr" ; break;
        case INTR_NIF_30: *str = SYSFS_CPLD2 "qsfpdd_nif_p30_intr" ; break;
        case INTR_NIF_31: *str = SYSFS_CPLD2 "qsfpdd_nif_p31_intr" ; break;
        case INTR_NIF_32: *str = SYSFS_CPLD2 "qsfpdd_nif_p32_intr" ; break;
        case INTR_NIF_33: *str = SYSFS_CPLD2 "qsfpdd_nif_p33_intr" ; break;
        case INTR_NIF_34: *str = SYSFS_CPLD2 "qsfpdd_nif_p34_intr" ; break;
        case INTR_NIF_35: *str = SYSFS_CPLD2 "qsfpdd_nif_p35_intr" ; break;
        case INTR_FAB_0:  *str = SYSFS_CPLD4 "qsfpdd_fab_p0_intr"  ; break;
        case INTR_FAB_1:  *str = SYSFS_CPLD4 "qsfpdd_fab_p1_intr"  ; break;
        case INTR_FAB_2:  *str = SYSFS_CPLD4 "qsfpdd_fab_p2_intr"  ; break;
        case INTR_FAB_3:  *str = SYSFS_CPLD4 "qsfpdd_fab_p3_intr"  ; break;
        case INTR_FAB_4:  *str = SYSFS_CPLD4 "qsfpdd_fab_p4_intr"  ; break;
        case INTR_FAB_5:  *str = SYSFS_CPLD4 "qsfpdd_fab_p5_intr"  ; break;
        case INTR_FAB_6:  *str = SYSFS_CPLD4 "qsfpdd_fab_p6_intr"  ; break;
        case INTR_FAB_7:  *str = SYSFS_CPLD4 "qsfpdd_fab_p7_intr"  ; break;
        case INTR_FAB_8:  *str = SYSFS_CPLD4 "qsfpdd_fab_p8_intr"  ; break;
        case INTR_FAB_9:  *str = SYSFS_CPLD4 "qsfpdd_fab_p9_intr"  ; break;
        case INTR_FAB_10: *str = SYSFS_CPLD2 "qsfpdd_fab_p10_intr" ; break;
        case INTR_FAB_11: *str = SYSFS_CPLD2 "qsfpdd_fab_p11_intr" ; break;
        case INTR_FAB_12: *str = SYSFS_CPLD2 "qsfpdd_fab_p12_intr" ; break;
        case INTR_FAB_13: *str = SYSFS_CPLD2 "qsfpdd_fab_p13_intr" ; break;
        case INTR_FAB_14: *str = SYSFS_CPLD2 "qsfpdd_fab_p14_intr" ; break;
        case INTR_FAB_15: *str = SYSFS_CPLD2 "qsfpdd_fab_p15_intr" ; break;
        case INTR_FAB_16: *str = SYSFS_CPLD2 "qsfpdd_fab_p16_intr" ; break;
        case INTR_FAB_17: *str = SYSFS_CPLD2 "qsfpdd_fab_p17_intr" ; break;
        case INTR_FAB_18: *str = SYSFS_CPLD2 "qsfpdd_fab_p18_intr" ; break;
        case INTR_FAB_19: *str = SYSFS_CPLD2 "qsfpdd_fab_p19_intr" ; break;
        //SFP / MGMT
        case ABS_MGMT_0:   *str = SYSFS_FPGA "mgmt_p0_abs"          ; break;
        case ABS_MGMT_1:   *str = SYSFS_FPGA "mgmt_p1_abs"          ; break;
        case ABS_SFP_36:   *str = SYSFS_FPGA "sfp28_p36_abs"        ; break;
        case ABS_SFP_37:   *str = SYSFS_FPGA "sfp28_p37_abs"        ; break;
        case RXLOS_MGMT_0: *str = SYSFS_FPGA "mgmt_p0_rx_los"       ; break;
        case RXLOS_MGMT_1: *str = SYSFS_FPGA "mgmt_p1_rx_los"       ; break;
        case RXLOS_SFP_36: *str = SYSFS_FPGA "sfp28_p36_rx_los"     ; break;
        case RXLOS_SFP_37: *str = SYSFS_FPGA "sfp28_p37_rx_los"     ; break;
        case TXFLT_MGMT_0: *str = SYSFS_FPGA "mgmt_p0_tx_flt"       ; break;
        case TXFLT_MGMT_1: *str = SYSFS_FPGA "mgmt_p1_tx_flt"       ; break;
        case TXFLT_SFP_36: *str = SYSFS_FPGA "sfp28_p36_tx_flt"     ; break;
        case TXFLT_SFP_37: *str = SYSFS_FPGA "sfp28_p37_tx_flt"     ; break;
        case TXDIS_MGMT_0: *str = SYSFS_FPGA "mgmt_p0_tx_dis"       ; break;
        case TXDIS_MGMT_1: *str = SYSFS_FPGA "mgmt_p1_tx_dis"       ; break;
        case TXDIS_SFP_36: *str = SYSFS_FPGA "sfp28_p36_tx_dis"     ; break;
        case TXDIS_SFP_37: *str = SYSFS_FPGA "sfp28_p37_tx_dis"     ; break;

        default:
            *str = "";
            return ONLP_STATUS_E_PARAM;
    }
    return ONLP_STATUS_OK;
}

static int ufi_port_to_eeprom_bus(int port)
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

    bus = ufi_port_to_eeprom_bus(port);

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

    bus = ufi_port_to_eeprom_bus(port);

    // set dev_class
    ONLP_TRY(onlp_file_write_int(dev_class, SYS_FMT, bus, EEPROM_ADDR, SYSFS_DEV_CLASS));

    return ONLP_STATUS_OK;
}

/**
 * @brief Update device class for QSFPDD ports
 *
 * This function updates the device class for a given QSFPDD port.
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

    if (!IS_QSFPX(port) || !onlp_sfpi_is_present(port)) { // not QSFPX or module absent
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

    for (i = 0; i < PORT_TYPE_DICT_SIZE ; ++i) {
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
 * @brief Update device class for QSFPDD ports
 *
 * This function updates the device class for a given QSFPDD port.
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

    // Check all ports and only update all QSFPX ports
    for (int i = 0; i < PORT_NUM; ++i) {
        if(IS_QSFPX(i)) {
            if (onlp_sfpi_dev_class_update_port(i) < 0) {
                rv = ONLP_STATUS_E_INTERNAL;
            }
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

    // In the CMIS 3.0 memory map, the size of one page is 128 , TX Disable function is located on page 16, at offset 130
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

    // In the CMIS 3.0 memory map, the size of one page is 128 , TX Disable function is located on page 16, at offset 130
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
    bus = ufi_port_to_eeprom_bus(port);
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
 * @brief Set SFP Port TX Disable Status
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

    //Check module present
    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    // tx disable support check
    if ((ret=ufi_cmis_txdisable_supported(port)) != ONLP_STATUS_OK) {
        return ret;
    }

    bus = ufi_port_to_eeprom_bus(port);
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
 * @brief Set SFP Port TX Disable Status
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

    //Check module present
    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
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
    bus = ufi_port_to_eeprom_bus(port);
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
        case ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL:
            if (IS_QSFPX(port)){
                
                *sysfs = "";
                *attr = port_attr[port].txdis;
            }
            else{
                rv = get_port_sysfs(port_attr[port].txdis, sysfs);
                *attr = port_attr[port].txdis;
            }
            break;
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
    AIM_BITMAP_CLR_ALL(bmap);
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
    //uint8_t bit = 0;

    VALIDATE_PORT(port);

    ONLP_TRY(get_port_sysfs(port_attr[port].abs, &sysfs));

    if ((status = read_file_hex(&abs, sysfs)) < 0) {
        AIM_LOG_ERROR("onlp_sfpi_is_present() failed, error=%d, sysfs=%s", status, sysfs);
        check_and_do_i2c_mux_reset(port);
        return status;
    }

    //ONLP_TRY(get_bit(port_attr[port].abs, port_attr[port].cpld_bit, &bit));
    //present = (get_bit_value(abs, bit) == 0) ? 1:0;
    present = (abs & 1) ? 0 : 1;
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
    bus = ufi_port_to_eeprom_bus(port);

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
    int bus = ufi_port_to_eeprom_bus(port);

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
    int bus = ufi_port_to_eeprom_bus(port);

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
    int bus = ufi_port_to_eeprom_bus(port);

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
    int bus = ufi_port_to_eeprom_bus(port);

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
    int bus = ufi_port_to_eeprom_bus(port);

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
    int bus = ufi_port_to_eeprom_bus(port);

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
    bus = ufi_port_to_eeprom_bus(port);
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
int onlp_sfpi_control_supported(int port, onlp_sfp_control_t control, int *rv)
{
    //set unsupported as default value
    *rv = 0;

    VALIDATE_PORT(port);

    switch (control) {
        case ONLP_SFP_CONTROL_RESET:
            if (IS_QSFPX(port)) {
                *rv = 1;
            }
            break;
        case ONLP_SFP_CONTROL_RESET_STATE:
        case ONLP_SFP_CONTROL_LP_MODE:
            if (IS_QSFPX(port)) {
                *rv = 1;
            }
            break;
        case ONLP_SFP_CONTROL_RX_LOS:
            if (IS_SFP(port)) {
                    *rv = 1;
                }
                break;
        case ONLP_SFP_CONTROL_TX_FAULT:
            if (IS_SFP(port)) {
                *rv = 1;
            }
            break;
        case ONLP_SFP_CONTROL_TX_DISABLE:
            if (IS_QSFPX(port)) {
                *rv = 1;
            }
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
    int rc = ONLP_STATUS_OK;
    int reg_val = 0;
    char *sysfs = NULL;
    int attr = 0;
    int dev_class = 0;

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
        case ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL:
            {
                if (IS_QSFPX(port)) {
                    ONLP_TRY(dev_class = onlp_sfpi_dev_class_update(port));

                    if (dev_class == 1) { //SFF8636 module
                        ONLP_TRY(rc = ufi_sff8636_txdisable_status_set(port, value, control));
                    } else if (dev_class == 3) { //CMIS module
                        ONLP_TRY(rc = ufi_cmis_txdisable_status_set(port, value, control));
                    } else {
                        AIM_LOG_ERROR("Port[%d] dev_class %d is not supported for tx disable control.\n", port, dev_class);
                        return ONLP_STATUS_E_UNSUPPORTED;
                    }
                }
                break;
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
    if (value) {
        reg_val |= 1;  // Set the bit 
    } else {
        reg_val &= ~1;  // Clear the bit 
    }

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
    int rc = ONLP_STATUS_OK;
    char *sysfs = NULL;
    int attr = 0;
    int dev_class = 0;

    VALIDATE_PORT(port);

    //get sysfs
    ONLP_TRY(xfr_ctrl_to_sysfs(port, control, &sysfs, &attr));

    if (control == ONLP_SFP_CONTROL_TX_DISABLE && (IS_QSFPX(port))) {
        ONLP_TRY(dev_class = onlp_sfpi_dev_class_update(port));

        if (dev_class == 1) { //SFF8636 module
            ONLP_TRY(rc = ufi_sff8636_txdisable_status_get(port, value, control));
        } else if (dev_class == 3) { //CMIS module
            rc = ufi_cmis_txdisable_status_get(port, value, control);
        } else {
            AIM_LOG_ERROR("Port[%d] dev_class %d is not supported for tx disable control.\n", port, dev_class);
            rc = ONLP_STATUS_E_UNSUPPORTED;
        }
        return rc;
    } 
    else {
        //read sysfs value
        if ((rc = read_file_hex(value, sysfs)) < 0) {
            AIM_LOG_ERROR("onlp_sfpi_control_get() failed, error=%d, sysfs=%s", rc, sysfs);
            check_and_do_i2c_mux_reset(port);
            return rc;
        }
        
    }
    
    *value &= 1;
    
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

