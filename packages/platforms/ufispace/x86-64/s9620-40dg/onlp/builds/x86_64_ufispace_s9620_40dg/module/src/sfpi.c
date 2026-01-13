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
#include <semaphore.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <onlp/platformi/sfpi.h>
#include <sys/time.h>
#include "platform_lib.h"

#define ALL_PORTS             -1

#define QSFP_NUM              12
#define SFPDD_NUM             0
#define QSFPDD_NUM            4
#define SFP_NUM               24
#define MGMT_NUM              0
#define QSFPX_NUM             (QSFP_NUM+QSFPDD_NUM)
#define QSFPX_SFPDD_NUM       (QSFP_NUM+QSFPDD_NUM+SFPDD_NUM)
#define PORT_NUM              (QSFPX_SFPDD_NUM+SFP_NUM+MGMT_NUM)

#define SYSFS_EEPROM        "eeprom"
#define SYSFS_DEV_CLASS     "dev_class"
#define EEPROM_ADDR         (0x50)
#define EEPROM_SFP_DOM_ADDR (0x51)
//SFF8636 TX Disable
#define SFF8636_EEPROM_OFFSET_TXDIS           0x56
#define SFF8636_EEPROM_TX_DIS                 0x0f  /* txdis valid bit(bit0-bit3), xxxx 1111 */
#define SFF8636_EEPROM_TX_EN                  0x0

#define MASK_1000_0000                        0x80
#define MASK_0000_0010                        0x02

//CMIS TX Disable
#define CMIS_PAGE_SIZE                        (128)
#define CMIS_PAGE_SUPPORTED_CTRL_ADV          (1)
#define CMIS_PAGE_TX_DIS                      (16)
#define CMIS_OFFSET_REVISION                  (1)
#define CMIS_OFFSET_MEMORY_MODEL              (2)
#define CMIS_OFFSET_TX_DIS                    (130)
#define CMIS_OFFSET_SUPPORTED_CTRL_ADV        (155)
#define CMIS_MASK_MEMORY_MODEL                (MASK_1000_0000)
#define CMIS_MASK_TX_DIS_ADV                  (MASK_0000_0010)
#define CMIS_VAL_TX_DIS                       (0xff)
#define CMIS_VAL_TX_EN                        (0x0)
#define CMIS_VAL_MEMORY_MODEL_PAGED           (0)
#define CMIS_VAL_TX_DIS_SUPPORTED             (1)
#define CMIS_VAL_VERSION_MIN                  (0x30)
#define CMIS_VAL_VERSION_MAX                  (0x5F)
#define CMIS_SEEK_TX_DIS_ADV                  (CMIS_PAGE_SIZE * CMIS_PAGE_SUPPORTED_CTRL_ADV + CMIS_OFFSET_SUPPORTED_CTRL_ADV)
#define CMIS_SEEK_TX_DIS                      (CMIS_PAGE_SIZE * CMIS_PAGE_TX_DIS + CMIS_OFFSET_TX_DIS)

#define IS_SFP(_node)         (_node.port_type == TYPE_SFP || _node.port_type == TYPE_MGMT)
#define IS_SFPDD(_node)       (_node.port_type == TYPE_SFPDD)
#define IS_QSFPX(_node)       (_node.port_type == TYPE_QSFPDD || _node.port_type == TYPE_QSFP)
#define IS_QSFP(_node)        (_node.port_type == TYPE_QSFP)
#define IS_QSFPDD(_node)      (_node.port_type == TYPE_QSFPDD)

#define VALIDATE_SFP_PORT(_node) { if (!IS_SFP(_node)) return ONLP_STATUS_E_PARAM; }

typedef struct
{
    char* abs_sysfs;
    char* lpmode_sysfs;
    char* reset_sysfs;
    char* rxlos_sysfs;
    char* txfault_sysfs;
    char* txdis_sysfs;
    int eeprom_bus;
    int port_type;
    char bit_mode;
    unsigned int cpld_bit; //FIXME remove
} port_node_t;

typedef enum port_type_e {
    TYPE_SFP = 0,
    TYPE_QSFP,
    TYPE_QSFPDD,
    TYPE_MGMT,
    TYPE_SFPDD,
    TYPE_UNKNOWN,
    TYPE_MAX,
} port_type_t;

typedef enum bit_mode_e {
    NORMAL = 0,
    STREAM_ABS,
    STREAM_RXLOS,
    STREAM_TXFLT,
    STREAM_TXDIS,
} bit_mode_t;

typedef enum op_type_e {
    OP_SYSFS = 0,
    OP_CMIS,
    OP_8636,
    OP_UNKNOWN,
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

static int get_node(int port, port_node_t *node)
{
    if(node == NULL)
        return ONLP_STATUS_E_PARAM;
    switch(port) {
        case 0:
            node->abs_sysfs = SYSFS_CPLD2 "qsfp28_p0_abs";
            node->lpmode_sysfs = SYSFS_CPLD2 "qsfp28_p0_lpmode";
            node->reset_sysfs = SYSFS_CPLD2 "qsfp28_p0_reset";
            node->eeprom_bus = 14;
            node->port_type = TYPE_QSFP;
            node->cpld_bit = 0;
            break;
        case 1:
            node->abs_sysfs = SYSFS_CPLD2 "qsfp28_p1_abs";
            node->lpmode_sysfs = SYSFS_CPLD2 "qsfp28_p1_lpmode";
            node->reset_sysfs = SYSFS_CPLD2 "qsfp28_p1_reset";
            node->eeprom_bus = 15;
            node->port_type = TYPE_QSFP;
            node->cpld_bit = 1;
            break;
        case 2:
            node->abs_sysfs = SYSFS_CPLD2 "qsfp28_p2_abs";
            node->lpmode_sysfs = SYSFS_CPLD2 "qsfp28_p2_lpmode";
            node->reset_sysfs = SYSFS_CPLD2 "qsfp28_p2_reset";
            node->eeprom_bus = 16;
            node->port_type = TYPE_QSFP;
            node->cpld_bit = 2;
            break;
        case 3:
            node->abs_sysfs = SYSFS_CPLD2 "qsfp28_p3_abs";
            node->lpmode_sysfs = SYSFS_CPLD2 "qsfp28_p3_lpmode";
            node->reset_sysfs = SYSFS_CPLD2 "qsfp28_p3_reset";
            node->eeprom_bus = 17;
            node->port_type = TYPE_QSFP;
            node->cpld_bit = 3;
            break;
        case 4:
            node->abs_sysfs = SYSFS_CPLD2 "qsfp28_p4_abs";
            node->lpmode_sysfs = SYSFS_CPLD2 "qsfp28_p4_lpmode";
            node->reset_sysfs = SYSFS_CPLD2 "qsfp28_p4_reset";
            node->eeprom_bus = 18;
            node->port_type = TYPE_QSFP;
            node->cpld_bit = 4;
            break;
        case 5:
            node->abs_sysfs = SYSFS_CPLD2 "qsfp28_p5_abs";
            node->lpmode_sysfs = SYSFS_CPLD2 "qsfp28_p5_lpmode";
            node->reset_sysfs = SYSFS_CPLD2 "qsfp28_p5_reset";
            node->eeprom_bus = 19;
            node->port_type = TYPE_QSFP;
            node->cpld_bit = 5;
            break;
        case 6:
            node->abs_sysfs = SYSFS_CPLD3 "qsfp28_p6_abs";
            node->lpmode_sysfs = SYSFS_CPLD3 "qsfp28_p6_lpmode";
            node->reset_sysfs = SYSFS_CPLD3 "qsfp28_p6_reset";
            node->eeprom_bus = 22;
            node->port_type = TYPE_QSFP;
            node->cpld_bit = 0;
            break;
        case 7:
            node->abs_sysfs = SYSFS_CPLD3 "qsfp28_p7_abs";
            node->lpmode_sysfs = SYSFS_CPLD3 "qsfp28_p7_lpmode";
            node->reset_sysfs = SYSFS_CPLD3 "qsfp28_p7_reset";
            node->eeprom_bus = 23;
            node->port_type = TYPE_QSFP;
            node->cpld_bit = 1;
            break;
        case 8:
            node->abs_sysfs = SYSFS_CPLD3 "qsfp28_p8_abs";
            node->lpmode_sysfs = SYSFS_CPLD3 "qsfp28_p8_lpmode";
            node->reset_sysfs = SYSFS_CPLD3 "qsfp28_p8_reset";
            node->eeprom_bus = 24;
            node->port_type = TYPE_QSFP;
            node->cpld_bit = 2;
            break;
        case 9:
            node->abs_sysfs = SYSFS_CPLD3 "qsfp28_p9_abs";
            node->lpmode_sysfs = SYSFS_CPLD3 "qsfp28_p9_lpmode";
            node->reset_sysfs = SYSFS_CPLD3 "qsfp28_p9_reset";
            node->eeprom_bus = 25;
            node->port_type = TYPE_QSFP;
            node->cpld_bit = 3;
            break;
        case 10:
            node->abs_sysfs = SYSFS_CPLD3 "qsfp28_p10_abs";
            node->lpmode_sysfs = SYSFS_CPLD3 "qsfp28_p10_lpmode";
            node->reset_sysfs = SYSFS_CPLD3 "qsfp28_p10_reset";
            node->eeprom_bus = 26;
            node->port_type = TYPE_QSFP;
            node->cpld_bit = 4;
            break;
        case 11:
            node->abs_sysfs = SYSFS_CPLD3 "qsfp28_p11_abs";
            node->lpmode_sysfs = SYSFS_CPLD3 "qsfp28_p11_lpmode";
            node->reset_sysfs = SYSFS_CPLD3 "qsfp28_p11_reset";
            node->eeprom_bus = 27;
            node->port_type = TYPE_QSFP;
            node->cpld_bit = 5;
            break;
        case 12:
            node->abs_sysfs = SYSFS_CPLD2 "qsfpdd_p12_abs";
            node->lpmode_sysfs = SYSFS_CPLD2 "qsfpdd_p12_lpmode";
            node->reset_sysfs = SYSFS_CPLD2 "qsfpdd_p12_reset";
            node->eeprom_bus = 30;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 0;
            break;
        case 13:
            node->abs_sysfs = SYSFS_CPLD2 "qsfpdd_p13_abs";
            node->lpmode_sysfs = SYSFS_CPLD2 "qsfpdd_p13_lpmode";
            node->reset_sysfs = SYSFS_CPLD2 "qsfpdd_p13_reset";
            node->eeprom_bus = 31;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 1;
            break;
        case 14:
            node->abs_sysfs = SYSFS_CPLD2 "qsfpdd_p14_abs";
            node->lpmode_sysfs = SYSFS_CPLD2 "qsfpdd_p14_lpmode";
            node->reset_sysfs = SYSFS_CPLD2 "qsfpdd_p14_reset";
            node->eeprom_bus = 32;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 2;
            break;
        case 15:
            node->abs_sysfs = SYSFS_CPLD2 "qsfpdd_p15_abs";
            node->lpmode_sysfs = SYSFS_CPLD2 "qsfpdd_p15_lpmode";
            node->reset_sysfs = SYSFS_CPLD2 "qsfpdd_p15_reset";
            node->eeprom_bus = 33;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 3;
            break;
        case 16:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p16_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p16_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p16_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p16_tx_disable";
            node->eeprom_bus = 38;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 0;
            break;
        case 17:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p17_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p17_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p17_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p17_tx_disable";
            node->eeprom_bus = 39;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 1;
            break;
        case 18:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p18_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p18_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p18_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p18_tx_disable";
            node->eeprom_bus = 40;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 2;
            break;
        case 19:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p19_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p19_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p19_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p19_tx_disable";
            node->eeprom_bus = 41;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 3;
            break;
        case 20:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p20_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p20_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p20_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p20_tx_disable";
            node->eeprom_bus = 42;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 4;
            break;
        case 21:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p21_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p21_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p21_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p21_tx_disable";
            node->eeprom_bus = 43;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 5;
            break;
        case 22:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p22_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p22_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p22_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p22_tx_disable";
            node->eeprom_bus = 44;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 6;
            break;
        case 23:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p23_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p23_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p23_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p23_tx_disable";
            node->eeprom_bus = 45;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 7;
            break;
        case 24:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p24_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p24_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p24_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p24_tx_disable";
            node->eeprom_bus = 46;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 0;
            break;
        case 25:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p25_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p25_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p25_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p25_tx_disable";
            node->eeprom_bus = 47;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 1;
            break;
        case 26:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p26_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p26_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p26_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p26_tx_disable";
            node->eeprom_bus = 48;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 2;
            break;
        case 27:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p27_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p27_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p27_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p27_tx_disable";
            node->eeprom_bus = 49;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 3;
            break;
        case 28:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p28_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p28_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p28_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p28_tx_disable";
            node->eeprom_bus = 50;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 4;
            break;
        case 29:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p29_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p29_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p29_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p29_tx_disable";
            node->eeprom_bus = 51;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 5;
            break;
        case 30:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p30_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p30_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p30_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p30_tx_disable";
            node->eeprom_bus = 52;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 6;
            break;
        case 31:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p31_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p31_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p31_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p31_tx_disable";
            node->eeprom_bus = 53;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 7;
            break;
        case 32:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p32_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p32_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p32_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p32_tx_disable";
            node->eeprom_bus = 54;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 0;
            break;
        case 33:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p33_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p33_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p33_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p33_tx_disable";
            node->eeprom_bus = 55;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 1;
            break;
        case 34:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p34_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p34_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p34_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p34_tx_disable";
            node->eeprom_bus = 56;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 2;
            break;
        case 35:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p35_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p35_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p35_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p35_tx_disable";
            node->eeprom_bus = 57;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 3;
            break;
        case 36:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p36_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p36_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p36_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p36_tx_disable";
            node->eeprom_bus = 58;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 4;
            break;
        case 37:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p37_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p37_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p37_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p37_tx_disable";
            node->eeprom_bus = 59;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 5;
            break;
        case 38:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p38_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p38_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p38_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p38_tx_disable";
            node->eeprom_bus = 60;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 6;
            break;
        case 39:
            node->abs_sysfs = SYSFS_CPLD4 "sfp56_p39_abs";
            node->rxlos_sysfs = SYSFS_CPLD4 "sfp56_p39_rx_los";
            node->txfault_sysfs = SYSFS_CPLD4 "sfp56_p39_tx_fault";
            node->txdis_sysfs = SYSFS_CPLD4 "sfp56_p39_tx_disable";
            node->eeprom_bus = 61;
            node->port_type = TYPE_SFP;
            node->cpld_bit = 7;
            break;

        default:
            return ONLP_STATUS_E_PARAM;
    }
    return ONLP_STATUS_OK;
}

static int xfr_ctrl_to_sysfs(port_node_t node, onlp_sfp_control_t control , char **sysfs)
{
    int rv  = ONLP_STATUS_OK;
    if(sysfs == NULL)
        return ONLP_STATUS_E_PARAM;

    switch(control)
    {
        case ONLP_SFP_CONTROL_RESET:
        case ONLP_SFP_CONTROL_RESET_STATE:
            {
                *sysfs = node.reset_sysfs;
                break;
            }
        case ONLP_SFP_CONTROL_RX_LOS:
            {
                *sysfs = node.rxlos_sysfs;
                break;
            }
        case ONLP_SFP_CONTROL_TX_FAULT:
            {
                *sysfs = node.txfault_sysfs;
                break;
            }
        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                *sysfs = node.txdis_sysfs;
                break;
            }
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                *sysfs = node.lpmode_sysfs;
                break;
            }
        default:
            rv = ONLP_STATUS_E_UNSUPPORTED;
            *sysfs = "";
    }

    if (rv != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    return ONLP_STATUS_OK;
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
    port_node_t node = {0};

    ONLP_TRY(get_node(port, &node));
    bus = node.eeprom_bus;

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
    port_node_t node = {0};

    ONLP_TRY(get_node(port, &node));
    bus = node.eeprom_bus;

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
    port_node_t node = {0};

    ONLP_TRY(get_node(port, &node));

    if (!IS_QSFPX(node) || !onlp_sfpi_is_present(port)) { // not QSFPX or module absent
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
    port_node_t node = {0};

    // single port update
    if (port != ALL_PORTS) {
        return onlp_sfpi_dev_class_update_port(port);
    }

    // Check all ports and only update all QSFPX ports
    for (int i = 0; i < PORT_NUM; ++i) {
        ONLP_TRY(get_node(i, &node));
        if(IS_QSFPX(node)) {
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
static int ufi_sff8636_txdisable_status_get(int port, int* status)
{
    uint8_t value = 0;

    if (onlp_sfpi_is_present(port) != 1) {
        return ONLP_STATUS_OK;
    }

    ONLP_TRY(value = onlp_sfpi_dev_readb(port, EEPROM_ADDR, SFF8636_EEPROM_OFFSET_TXDIS));
    // Check each bit of the 'value' has all bits set to 1 meets TX Disable condition (all channels disabled).
    if (value == SFF8636_EEPROM_TX_DIS) {
        *status = 1;
    } else {
        *status = 0;
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
static int ufi_sff8636_txdisable_status_set(int port, int status)
{
    uint8_t value = 0, readback = 0;

    if (status == 0) {
        value = SFF8636_EEPROM_TX_EN;
    } else if (status == 1) {
        value = SFF8636_EEPROM_TX_DIS;
    } else {
        AIM_LOG_ERROR("[%s] invalid status, port=%d, status=%d\n", __FUNCTION__, port, status);
        return ONLP_STATUS_E_PARAM;
    }

    if (onlp_sfpi_is_present(port) != 1) {
        return ONLP_STATUS_OK;
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
    int seek = 0;
    int length = 0;
    int tx_dis_adv = 0;
    port_node_t node = {0};

    ONLP_TRY(get_node(port, &node));

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
    seek = CMIS_SEEK_TX_DIS_ADV;

    // create and check sysfs_path
    length = snprintf(sysfs_path, sizeof(sysfs_path), SYS_FMT, node.eeprom_bus, EEPROM_ADDR, SYSFS_EEPROM);
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
/**
 * @brief Get CMIS Port TX Disable Status
 * @param port: The port number.
 * @param status: 1 if tx disable (turn on)
 * @param status: 0 if normal (turn off)
 * @returns An error condition.
 */
static int ufi_cmis_txdisable_status_get(int port, int* status)
{
    int ret = 0;
    uint8_t value = 0;
    char sysfs_path[256] = {0};
    int length = 0;
    port_node_t node = {0};

    ONLP_TRY(get_node(port, &node));

    // Check module present
    if (onlp_sfpi_is_present(port) != 1) {
        return ONLP_STATUS_OK;
    }

    // tx disable support check
    if ((ret=ufi_cmis_txdisable_supported(port)) != ONLP_STATUS_OK) {
        return ret;
    }

    length = snprintf(sysfs_path, sizeof(sysfs_path), SYS_FMT, node.eeprom_bus, EEPROM_ADDR, SYSFS_EEPROM);
    // check snprintf
    if (length < 0 || length >= sizeof(sysfs_path)) {
    AIM_LOG_ERROR("[%s] Error generating sysfs path\n", __FUNCTION__);
        return ONLP_STATUS_E_INTERNAL;
    }

    // get tx disable
    if (ufi_file_seek_readb(sysfs_path, CMIS_SEEK_TX_DIS, &value) < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    // Check each bit of the 'value' has all bits set to 1 meets TX Disable condition (all channels disabled).
    if (value == CMIS_VAL_TX_DIS) {
        *status = 1;
    } else {
        *status = 0;
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
static int ufi_cmis_txdisable_status_set(int port, int status)
{
    uint8_t value = 0, readback = 0;
    char sysfs_path[256] = {0};
    int seek = CMIS_SEEK_TX_DIS;
    port_node_t node = {0};

    ONLP_TRY(get_node(port, &node));

    // Check module present
    if (onlp_sfpi_is_present(port) != 1) {
        return ONLP_STATUS_OK;
    }

    // tx disable support check
    if (ufi_cmis_txdisable_supported(port) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    // set value
    if (status == 0) {
        value = CMIS_VAL_TX_EN;
    } else if (status == 1) {
        value = CMIS_VAL_TX_DIS;
    } else {
        AIM_LOG_ERROR("[%s] unaccepted status, port=%d, status=%d\n", __FUNCTION__, port, status);
        return ONLP_STATUS_E_PARAM;
    }

    // check snprintf
    int length = snprintf(sysfs_path, sizeof(sysfs_path), SYS_FMT, node.eeprom_bus, EEPROM_ADDR, SYSFS_EEPROM);
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
    int p = 0;
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
    port_node_t node = {0};

    ONLP_TRY(get_node(port, &node));
    if ((status = read_file_hex(&abs, node.abs_sysfs)) < 0) {
        AIM_LOG_ERROR("onlp_sfpi_is_present() failed, error=%d, sysfs=%s",
                          status, node.abs_sysfs);
        check_and_do_i2c_mux_reset(port);
        return status;
    }

    present = abs ? 0 : 1;

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
        rc = onlp_sfpi_is_present(p);
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
        port_node_t node = {0};
        ONLP_TRY(get_node(i, &node));
        if(IS_SFP(node)) {
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
    port_node_t node = {0};

    memset(data, 0, 256);
    ONLP_TRY(get_node(port, &node));

    if((rc = onlp_file_read(data, 256, &size, SYS_FMT, node.eeprom_bus, EEPROM_ADDR, SYSFS_EEPROM)) < 0) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d)", port);
        AIM_LOG_ERROR(SYS_FMT, bus, EEPROM_ADDR, SYSFS_EEPROM);

        check_and_do_i2c_mux_reset(port);
        return rc;
    }

    if (size != 256) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d), size is different!", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return rc;
}

/**
 * @brief Read a byte from an address on the given SFP port's bus.
 * @param port The port number.
 * @param devaddr The device address.
 * @param addr The address.
 */
int onlp_sfpi_dev_readb(int port, uint8_t devaddr, uint8_t addr)
{
    int rc = 0;
    port_node_t node = {0};
    ONLP_TRY(get_node(port, &node));

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if ((rc=onlp_i2c_readb(node.eeprom_bus, devaddr, addr, ONLP_I2C_F_FORCE)) < 0) {
        check_and_do_i2c_mux_reset(port);
    }
    return rc;
}

/**
 * @brief Write a byte to an address on the given SFP port's bus.
 */
int onlp_sfpi_dev_writeb(int port, uint8_t devaddr, uint8_t addr, uint8_t value)
{
    int rc = 0;
    port_node_t node = {0};
    ONLP_TRY(get_node(port, &node));

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if ((rc=onlp_i2c_writeb(node.eeprom_bus, devaddr, addr, value, ONLP_I2C_F_FORCE)) < 0) {
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
    int rc = 0;
    port_node_t node = {0};
    ONLP_TRY(get_node(port, &node));

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if ((rc=onlp_i2c_readw(node.eeprom_bus, devaddr, addr, ONLP_I2C_F_FORCE)) < 0) {
        check_and_do_i2c_mux_reset(port);
    }

    return rc;
}

/**
 * @brief Write a word to an address on the given SFP port's bus.
 */
int onlp_sfpi_dev_writew(int port, uint8_t devaddr, uint8_t addr, uint16_t value)
{
    int rc = 0;
    port_node_t node = {0};
    ONLP_TRY(get_node(port, &node));

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if ((rc=onlp_i2c_writew(node.eeprom_bus, devaddr, addr, value, ONLP_I2C_F_FORCE)) < 0) {
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
    int rc = 0;
    port_node_t node = {0};
    ONLP_TRY(get_node(port, &node));

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if (onlp_i2c_block_read(node.eeprom_bus, devaddr, addr, size, rdata, ONLP_I2C_F_FORCE) < 0) {
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return rc;
}

/**
 * @brief Write to an address on the given SFP port's bus.
 */
int onlp_sfpi_dev_write(int port, uint8_t devaddr, uint8_t addr, uint8_t* data, int size)
{
    int rc = 0;
    port_node_t node = {0};
    ONLP_TRY(get_node(port, &node));

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if ((rc=onlp_i2c_write(node.eeprom_bus, devaddr, addr, size, data, ONLP_I2C_F_FORCE)) < 0) {
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
    port_node_t node = {0};

    //sfp dom is on 0x51 (2nd 256 bytes)
    //qsfp dom is on lower page 0x00
    //qsfpdd 2.0 dom is on lower page 0x00
    //qsfpdd 3.0 and later dom and above is on lower page 0x00 and higher page 0x17
    ONLP_TRY(get_node(port, &node));
    VALIDATE_SFP_PORT(node);

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    memset(data, 0, 256);

    char eeprom_path[512];
    FILE* fp = NULL;
    memset(eeprom_path, 0, sizeof(eeprom_path));

    //set eeprom_path
    snprintf(eeprom_path, sizeof(eeprom_path), SYS_FMT, node.eeprom_bus, EEPROM_ADDR, SYSFS_EEPROM);

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

    port_node_t node = {0};

    ONLP_TRY(get_node(port, &node));

    //set unsupported as default value
    *rv = 0;

    switch (control) {
        case ONLP_SFP_CONTROL_RESET:
        case ONLP_SFP_CONTROL_RESET_STATE:
        case ONLP_SFP_CONTROL_LP_MODE:
            if (IS_QSFPX(node)) {
                *rv = 1;
            }
            break;
        case ONLP_SFP_CONTROL_RX_LOS:
        case ONLP_SFP_CONTROL_TX_FAULT:
            if (IS_SFP(node)) {
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
    int rc = ONLP_STATUS_OK;
    char *sysfs = NULL;
    port_node_t node = {0};
    int op_type = OP_SYSFS;

    ONLP_TRY(get_node(port, &node));

    //check control is valid for this port
    switch(control)
    {
        case ONLP_SFP_CONTROL_RESET:
            {
                if (IS_QSFPX(node)) {
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
                if (IS_SFP(node)) {
                    op_type = OP_SYSFS;
                } else if (IS_QSFPX(node)) {
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
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                if (IS_QSFPX(node)) {
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
        ONLP_TRY(xfr_ctrl_to_sysfs(node, control, &sysfs));

        //write sysfs
        if ((rc=onlp_file_write_int(value, sysfs)) < 0) {
            AIM_LOG_ERROR("Unable to write %s, error=%d, value=%x", sysfs,  rc, value);
            check_and_do_i2c_mux_reset(port);
            return ONLP_STATUS_E_INTERNAL;
        }
    } else if (op_type == OP_CMIS) {
        ONLP_TRY(ufi_cmis_txdisable_status_set(port, value));
    } else if (op_type == OP_8636) {
        ONLP_TRY(ufi_sff8636_txdisable_status_set(port, value));
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
    port_node_t node = {0};
    int op_type = OP_SYSFS;

    ONLP_TRY(get_node(port, &node));

    //check control is valid for this port
    switch(control)
    {
        case ONLP_SFP_CONTROL_RESET_STATE:
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                if (IS_QSFPX(node)) {
                    op_type = OP_SYSFS;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_RX_LOS:
        case ONLP_SFP_CONTROL_TX_FAULT:
            {
                if (IS_SFP(node) || IS_SFPDD(node)) {
                    op_type = OP_SYSFS;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_TX_DISABLE:
        case ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL:
            {
                if (IS_SFP(node) || IS_SFPDD(node)) {
                    op_type = OP_SYSFS;
                } else if (IS_QSFPX(node)) {
                    int dev_class = 0;
                    ONLP_TRY(dev_class = onlp_sfpi_dev_class_update(port));

                    if (dev_class == 1) { //SFF8636 module
                        op_type = OP_8636;
                    } else if (dev_class == 3) { //CMIS module
                        op_type = OP_CMIS;
                    } else {
                        AIM_LOG_ERROR("Port[%d] dev_class %d is not supported for tx disable control.\n", port, dev_class);
                        return ONLP_STATUS_E_UNSUPPORTED;
                    }
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
        ONLP_TRY(xfr_ctrl_to_sysfs(node, control, &sysfs));

        //read gpio value
        if ((rc = read_file_hex(&reg_val, sysfs)) < 0) {
            AIM_LOG_ERROR("onlp_sfpi_control_get() failed, error=%d, sysfs=%s", rc, sysfs);
            check_and_do_i2c_mux_reset(port);
            return rc;
        }

        *value = reg_val;

        //reverse bit
        if (control == ONLP_SFP_CONTROL_RESET_STATE) {
            *value = !(*value);
        }
    } else if (op_type == OP_CMIS) {
        return ufi_cmis_txdisable_status_get(port, value);
    } else if (op_type == OP_8636) {
        return ufi_sff8636_txdisable_status_get(port, value);
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
