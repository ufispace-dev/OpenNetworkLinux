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

#define QSFP_NUM              0
#define SFPDD_NUM             0
#define QSFPDD_NUM            64
#define SFP_NUM               2
#define QSFPX_NUM             (QSFP_NUM+QSFPDD_NUM)
#define QSFPX_SFPDD_NUM       (QSFP_NUM+QSFPDD_NUM+SFPDD_NUM)
#define PORT_NUM              (QSFPX_SFPDD_NUM+SFP_NUM)

#define SYSFS_EEPROM        "eeprom"
#define EEPROM_ADDR         (0x50)
#define EEPROM_SFP_DOM_ADDR (0x51)

#define IS_SFP(_node)         (_node.port_type == TYPE_SFP || _node.port_type == TYPE_MGMT)
#define IS_QSFPX(_node)       (_node.port_type == TYPE_QSFPDD || _node.port_type == TYPE_QSFP)
#define IS_QSFP(_node)        (_node.port_type == TYPE_QSFP)
#define IS_QSFPDD(_node)      (_node.port_type == TYPE_QSFPDD)

#define VALIDATE_SFP_PORT(_node) { if (!IS_SFP(_node)) return ONLP_STATUS_E_PARAM; }

// FPGA
#define FPGA_PCI_ENABLE_SYSFS "/sys/devices/platform/x86_64_ufispace_s9321_64e_lpc/bsp/bsp_fpga_pci_enable"
#define FPGA_RESOURCE "/sys/devices/pci0000:15/0000:15:04.0/0000:18:00.0/resource4"
#define PAGE_SIZE (4096)
#define PAGE_MASK (PAGE_SIZE-1)
#define EEPROM_SIZE (256)
#define EEPROM_LOAD_MAX (16)
#define FPGA_CTRL_BASE (0x04000000)
#define FPGA_MEM_BASE  FPGA_CTRL_BASE
#define FPGA_TO_X86(_addr) ((_addr) << 2)
#define DEBUG_FPGA_READ   (0)
#define DEBUG_FPGA_WRITE  (0)
#define DEBUG_FPGA_NO_ACT (0)
#define VALIDATE_OFFSET(_off_0,_off_1) { if (_off_0 + _off_1 > EEPROM_SIZE) return ONLP_STATUS_E_PARAM; }
#define FPGA_LOCK()                  \
    do{                              \
        onlp_shlock_take(fpga_lock); \
    }while(0)

#define FPGA_UNLOCK()                \
    do{                              \
        onlp_shlock_give(fpga_lock); \
    }while(0)
#define FPGA_TRY(_expr)                                     \
    do {                                                    \
        int _rv = (_expr);                                  \
        if(_rv<0) {                                         \
            AIM_LOG_ERROR("%s returned %d\n", #_expr, _rv); \
            FPGA_UNLOCK();                                  \
            return _rv;                                     \
        }                                                   \
    } while(0)
#define LOCK_MAGIC 0xA2B4C6D8

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
    unsigned int cpld_bit;

    // fpga pci
    uint32_t ctrl_addr;
    uint32_t mem_addr;
    uint8_t  channel;
} port_node_t;

typedef enum port_type_e {
    TYPE_SFP = 0,
    TYPE_QSFP,
    TYPE_QSFPDD,
    TYPE_MGMT,
    TYPE_UNNKOW,
    TYPE_MAX,
} port_type_t;

typedef enum bit_mode_e {
    NORMAL = 0,
    STREAM_ABS,
    STREAM_RXLOS,
    STREAM_TXFLT,
    STREAM_TXDIS,
} bit_mode_t;

static int fpga_pci_enable = -1;
static int fpga_res_handle = -1;
static void *mmap_base = NULL;
static onlp_shlock_t* fpga_lock = NULL;

fpga_ctrl_t  fpga_ctrl = {.ctrl_addr = 0,
                          .mem_addr  = 0,
                          .fpga_rst_ctrl     = {.offset           = 0x0000,
                                               .rst_data         = {.val=0, .shift=0 }},
                          .fpga_ctrl_int_st = {.offset           = 0x0010,
                                               .i2c_idle         = {.val=0, .shift=0 },
                                               .i2c_stuck        = {.val=0, .shift=1 },
                                               .i2c_no_ack       = {.val=0, .shift=2 }},
                          .fpga_ctrl_0      = {.offset           = 0x00F0,
                                               .byte_0           = {.val=0, .shift=0 },
                                               .byte_1           = {.val=0, .shift=16}},
                          .fpga_ctrl_1      = {.offset           = 0x00F1,
                                               .byte_2           = {.val=0, .shift=0 },
                                               .byte_3           = {.val=0, .shift=16}},
                          .fpga_ctrl_2      = {.offset           = 0x00EF,
                                               .block_rw_len     = {.val=0, .shift=0 },
                                               .byte_mode        = {.val=0, .shift=9 },
                                               .i2c_reg_addr_len = {.val=0, .shift=16},
                                               .page_set         = {.val=0, .shift=18},
                                               .auto_mode        = {.val=0, .shift=31}},
                          .fpga_ctrl_3      = {.offset           = 0x00F3,
                                               .reg_addr         = {.val=0, .shift=0 },
                                               .slave_addr       = {.val=0, .shift=16},
                                               .page_set_enable  = {.val=0, .shift=23},
                                               .i2c_chnl_preset  = {.val=0, .shift=24},
                                               .rw               = {.val=0, .shift=30},
                                               .action           = {.val=0, .shift=31}}};

static int get_node(int port, port_node_t *node)
{
    if(node == NULL)
        return ONLP_STATUS_E_PARAM;

    switch(port) {
        case 0:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_0";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_0";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_0";
            node->eeprom_bus = 17;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 0;
            node->ctrl_addr = 0x1000;
            node->mem_addr = 0x0;
            node->channel = 1;
           break;
        case 1:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_0";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_0";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_0";
            node->eeprom_bus = 18;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 1;
            node->ctrl_addr = 0x1000;
            node->mem_addr = 0x0;
            node->channel = 2;
           break;
        case 2:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_0";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_0";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_0";
            node->eeprom_bus = 19;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 2;
            node->ctrl_addr = 0x1000;
            node->mem_addr = 0x0;
            node->channel = 3;
           break;
        case 3:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_0";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_0";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_0";
            node->eeprom_bus = 20;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 3;
            node->ctrl_addr = 0x1000;
            node->mem_addr = 0x0;
            node->channel = 4;
           break;
        case 4:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_0";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_0";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_0";
            node->eeprom_bus = 21;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 4;
            node->ctrl_addr = 0x1000;
            node->mem_addr = 0x0;
            node->channel = 5;
           break;
        case 5:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_0";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_0";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_0";
            node->eeprom_bus = 22;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 5;
            node->ctrl_addr = 0x1000;
            node->mem_addr = 0x0;
            node->channel = 6;
           break;
        case 6:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_0";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_0";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_0";
            node->eeprom_bus = 23;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 6;
            node->ctrl_addr = 0x1000;
            node->mem_addr = 0x0;
            node->channel = 7;
           break;
        case 7:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_0";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_0";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_0";
            node->eeprom_bus = 24;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 7;
            node->ctrl_addr = 0x1000;
            node->mem_addr = 0x0;
            node->channel = 8;
           break;
        case 8:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_1";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_1";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_1";
            node->eeprom_bus = 25;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 0;
            node->ctrl_addr = 0x1000;
            node->mem_addr = 0x0;
            node->channel = 9;
           break;
        case 9:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_1";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_1";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_1";
            node->eeprom_bus = 26;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 1;
            node->ctrl_addr = 0x1000;
            node->mem_addr = 0x0;
            node->channel = 10;
           break;
        case 10:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_1";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_1";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_1";
            node->eeprom_bus = 27;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 2;
            node->ctrl_addr = 0x1000;
            node->mem_addr = 0x0;
            node->channel = 11;
           break;
        case 11:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_1";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_1";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_1";
            node->eeprom_bus = 28;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 3;
            node->ctrl_addr = 0x1000;
            node->mem_addr = 0x0;
            node->channel = 12;
           break;
        case 12:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_1";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_1";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_1";
            node->eeprom_bus = 29;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 4;
            node->ctrl_addr = 0x1000;
            node->mem_addr = 0x0;
            node->channel = 13;
           break;
        case 13:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_1";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_1";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_1";
            node->eeprom_bus = 30;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 5;
            node->ctrl_addr = 0x1000;
            node->mem_addr = 0x0;
            node->channel = 14;
           break;
        case 14:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_1";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_1";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_1";
            node->eeprom_bus = 31;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 6;
            node->ctrl_addr = 0x1000;
            node->mem_addr = 0x0;
            node->channel = 15;
           break;
        case 15:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_1";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_1";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_1";
            node->eeprom_bus = 32;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 7;
            node->ctrl_addr = 0x1000;
            node->mem_addr = 0x0;
            node->channel = 16;
           break;
        case 16:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_0";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_0";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_0";
            node->eeprom_bus = 49;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 0;
            node->ctrl_addr = 0x3000;
            node->mem_addr = 0x200;
            node->channel = 1;
           break;
        case 17:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_0";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_0";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_0";
            node->eeprom_bus = 50;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 1;
            node->ctrl_addr = 0x3000;
            node->mem_addr = 0x200;
            node->channel = 2;
           break;
        case 18:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_0";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_0";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_0";
            node->eeprom_bus = 51;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 2;
            node->ctrl_addr = 0x3000;
            node->mem_addr = 0x200;
            node->channel = 3;
           break;
        case 19:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_0";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_0";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_0";
            node->eeprom_bus = 52;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 3;
            node->ctrl_addr = 0x3000;
            node->mem_addr = 0x200;
            node->channel = 4;
           break;
        case 20:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_0";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_0";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_0";
            node->eeprom_bus = 53;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 4;
            node->ctrl_addr = 0x3000;
            node->mem_addr = 0x200;
            node->channel = 5;
           break;
        case 21:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_0";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_0";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_0";
            node->eeprom_bus = 54;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 5;
            node->ctrl_addr = 0x3000;
            node->mem_addr = 0x200;
            node->channel = 6;
           break;
        case 22:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_0";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_0";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_0";
            node->eeprom_bus = 55;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 6;
            node->ctrl_addr = 0x3000;
            node->mem_addr = 0x200;
            node->channel = 7;
           break;
        case 23:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_0";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_0";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_0";
            node->eeprom_bus = 56;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 7;
            node->ctrl_addr = 0x3000;
            node->mem_addr = 0x200;
            node->channel = 8;
           break;
        case 24:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_1";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_1";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_1";
            node->eeprom_bus = 57;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 0;
            node->ctrl_addr = 0x3000;
            node->mem_addr = 0x200;
            node->channel = 9;
           break;
        case 25:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_1";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_1";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_1";
            node->eeprom_bus = 58;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 1;
            node->ctrl_addr = 0x3000;
            node->mem_addr = 0x200;
            node->channel = 10;
           break;
        case 26:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_1";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_1";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_1";
            node->eeprom_bus = 59;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 2;
            node->ctrl_addr = 0x3000;
            node->mem_addr = 0x200;
            node->channel = 11;
           break;
        case 27:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_1";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_1";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_1";
            node->eeprom_bus = 60;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 3;
            node->ctrl_addr = 0x3000;
            node->mem_addr = 0x200;
            node->channel = 12;
           break;
        case 28:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_1";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_1";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_1";
            node->eeprom_bus = 61;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 4;
            node->ctrl_addr = 0x3000;
            node->mem_addr = 0x200;
            node->channel = 13;
           break;
        case 29:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_1";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_1";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_1";
            node->eeprom_bus = 62;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 5;
            node->ctrl_addr = 0x3000;
            node->mem_addr = 0x200;
            node->channel = 14;
           break;
        case 30:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_1";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_1";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_1";
            node->eeprom_bus = 63;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 6;
            node->ctrl_addr = 0x3000;
            node->mem_addr = 0x200;
            node->channel = 15;
           break;
        case 31:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_1";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_1";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_1";
            node->eeprom_bus = 64;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 7;
            node->ctrl_addr = 0x3000;
            node->mem_addr = 0x200;
            node->channel = 16;
           break;
        case 32:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_2";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_2";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_2";
            node->eeprom_bus = 33;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 0;
            node->ctrl_addr = 0x2000;
            node->mem_addr = 0x100;
            node->channel = 1;
           break;
        case 33:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_2";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_2";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_2";
            node->eeprom_bus = 34;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 1;
            node->ctrl_addr = 0x2000;
            node->mem_addr = 0x100;
            node->channel = 2;
           break;
        case 34:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_2";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_2";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_2";
            node->eeprom_bus = 35;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 2;
            node->ctrl_addr = 0x2000;
            node->mem_addr = 0x100;
            node->channel = 3;
           break;
        case 35:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_2";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_2";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_2";
            node->eeprom_bus = 36;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 3;
            node->ctrl_addr = 0x2000;
            node->mem_addr = 0x100;
            node->channel = 4;
           break;
        case 36:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_2";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_2";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_2";
            node->eeprom_bus = 37;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 4;
            node->ctrl_addr = 0x2000;
            node->mem_addr = 0x100;
            node->channel = 5;
           break;
        case 37:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_2";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_2";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_2";
            node->eeprom_bus = 38;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 5;
            node->ctrl_addr = 0x2000;
            node->mem_addr = 0x100;
            node->channel = 6;
           break;
        case 38:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_2";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_2";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_2";
            node->eeprom_bus = 39;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 6;
            node->ctrl_addr = 0x2000;
            node->mem_addr = 0x100;
            node->channel = 7;
           break;
        case 39:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_2";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_2";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_2";
            node->eeprom_bus = 40;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 7;
            node->ctrl_addr = 0x2000;
            node->mem_addr = 0x100;
            node->channel = 8;
           break;
        case 40:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_3";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_3";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_3";
            node->eeprom_bus = 41;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 0;
            node->ctrl_addr = 0x2000;
            node->mem_addr = 0x100;
            node->channel = 9;
           break;
        case 41:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_3";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_3";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_3";
            node->eeprom_bus = 42;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 1;
            node->ctrl_addr = 0x2000;
            node->mem_addr = 0x100;
            node->channel = 10;
           break;
        case 42:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_3";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_3";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_3";
            node->eeprom_bus = 43;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 2;
            node->ctrl_addr = 0x2000;
            node->mem_addr = 0x100;
            node->channel = 11;
           break;
        case 43:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_3";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_3";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_3";
            node->eeprom_bus = 44;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 3;
            node->ctrl_addr = 0x2000;
            node->mem_addr = 0x100;
            node->channel = 12;
           break;
        case 44:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_3";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_3";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_3";
            node->eeprom_bus = 45;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 4;
            node->ctrl_addr = 0x2000;
            node->mem_addr = 0x100;
            node->channel = 13;
           break;
        case 45:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_3";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_3";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_3";
            node->eeprom_bus = 46;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 5;
            node->ctrl_addr = 0x2000;
            node->mem_addr = 0x100;
            node->channel = 14;
           break;
        case 46:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_3";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_3";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_3";
            node->eeprom_bus = 47;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 6;
            node->ctrl_addr = 0x2000;
            node->mem_addr = 0x100;
            node->channel = 15;
           break;
        case 47:
            node->abs_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_intr_present_3";
            node->lpmode_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_lpmode_3";
            node->reset_sysfs = SYSFS_CPLD2 "cpld_qsfpdd_reset_3";
            node->eeprom_bus = 48;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 7;
            node->ctrl_addr = 0x2000;
            node->mem_addr = 0x100;
            node->channel = 16;
           break;
        case 48:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_2";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_2";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_2";
            node->eeprom_bus = 65;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 0;
            node->ctrl_addr = 0x4000;
            node->mem_addr = 0x300;
            node->channel = 1;
           break;
        case 49:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_2";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_2";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_2";
            node->eeprom_bus = 66;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 1;
            node->ctrl_addr = 0x4000;
            node->mem_addr = 0x300;
            node->channel = 2;
           break;
        case 50:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_2";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_2";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_2";
            node->eeprom_bus = 67;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 2;
            node->ctrl_addr = 0x4000;
            node->mem_addr = 0x300;
            node->channel = 3;
           break;
        case 51:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_2";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_2";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_2";
            node->eeprom_bus = 68;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 3;
            node->ctrl_addr = 0x4000;
            node->mem_addr = 0x300;
            node->channel = 4;
           break;
        case 52:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_2";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_2";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_2";
            node->eeprom_bus = 69;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 4;
            node->ctrl_addr = 0x4000;
            node->mem_addr = 0x300;
            node->channel = 5;
           break;
        case 53:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_2";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_2";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_2";
            node->eeprom_bus = 70;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 5;
            node->ctrl_addr = 0x4000;
            node->mem_addr = 0x300;
            node->channel = 6;
           break;
        case 54:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_2";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_2";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_2";
            node->eeprom_bus = 71;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 6;
            node->ctrl_addr = 0x4000;
            node->mem_addr = 0x300;
            node->channel = 7;
           break;
        case 55:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_2";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_2";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_2";
            node->eeprom_bus = 72;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 7;
            node->ctrl_addr = 0x4000;
            node->mem_addr = 0x300;
            node->channel = 8;
           break;
        case 56:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_3";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_3";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_3";
            node->eeprom_bus = 73;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 0;
            node->ctrl_addr = 0x4000;
            node->mem_addr = 0x300;
            node->channel = 9;
           break;
        case 57:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_3";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_3";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_3";
            node->eeprom_bus = 74;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 1;
            node->ctrl_addr = 0x4000;
            node->mem_addr = 0x300;
            node->channel = 10;
           break;
        case 58:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_3";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_3";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_3";
            node->eeprom_bus = 75;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 2;
            node->ctrl_addr = 0x4000;
            node->mem_addr = 0x300;
            node->channel = 11;
           break;
        case 59:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_3";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_3";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_3";
            node->eeprom_bus = 76;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 3;
            node->ctrl_addr = 0x4000;
            node->mem_addr = 0x300;
            node->channel = 12;
           break;
        case 60:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_3";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_3";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_3";
            node->eeprom_bus = 77;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 4;
            node->ctrl_addr = 0x4000;
            node->mem_addr = 0x300;
            node->channel = 13;
           break;
        case 61:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_3";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_3";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_3";
            node->eeprom_bus = 78;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 5;
            node->ctrl_addr = 0x4000;
            node->mem_addr = 0x300;
            node->channel = 14;
           break;
        case 62:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_3";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_3";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_3";
            node->eeprom_bus = 79;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 6;
            node->ctrl_addr = 0x4000;
            node->mem_addr = 0x300;
            node->channel = 15;
           break;
        case 63:
            node->abs_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_intr_present_3";
            node->lpmode_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_lpmode_3";
            node->reset_sysfs = SYSFS_CPLD3 "cpld_qsfpdd_reset_3";
            node->eeprom_bus = 80;
            node->port_type = TYPE_QSFPDD;
            node->cpld_bit = 7;
            node->ctrl_addr = 0x4000;
            node->mem_addr = 0x300;
            node->channel = 16;
           break;
        case 64:
            node->abs_sysfs = SYSFS_FPGA "fpga_sfp28_intr_present";
            node->rxlos_sysfs = SYSFS_FPGA "fpga_sfp28_rx_los";
            node->txfault_sysfs = SYSFS_FPGA "fpga_sfp28_tx_fault";
            node->txdis_sysfs = SYSFS_FPGA "fpga_sfp28_tx_dis";
            node->eeprom_bus = 81;
            node->port_type = TYPE_MGMT;
            node->cpld_bit = 0;
            node->ctrl_addr = 0x5000;
            node->mem_addr = 0x400;
            node->channel = 0;
            break;
        case 65:
            node->abs_sysfs = SYSFS_FPGA "fpga_sfp28_intr_present";
            node->rxlos_sysfs = SYSFS_FPGA "fpga_sfp28_rx_los";
            node->txfault_sysfs = SYSFS_FPGA "fpga_sfp28_tx_fault";
            node->txdis_sysfs = SYSFS_FPGA "fpga_sfp28_tx_dis";
            node->eeprom_bus = 82;
            node->port_type = TYPE_MGMT;
            node->cpld_bit = 1;
            node->ctrl_addr = 0x6000;
            node->mem_addr = 0x500;
            node->channel = 0;
            break;
        default:
            return ONLP_STATUS_E_PARAM;
    }
    return ONLP_STATUS_OK;
}

static int get_bit(int bit_mode, unsigned int bit_stream, uint8_t *bit)
{
    int rv  = ONLP_STATUS_OK;
    int tmp_value = 0;

    if(bit == NULL)
        return ONLP_STATUS_E_PARAM;

    switch(bit_mode) {

        case STREAM_ABS:
            tmp_value = bit_stream >> 0;
            break;
        case STREAM_RXLOS:
            tmp_value = bit_stream >> 3;
            break;
        case STREAM_TXFLT:
            tmp_value = bit_stream >> 6;
            break;
        case STREAM_TXDIS:
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

static int is_fpga_pci_enable()
{
    int rc = 0;
    if(fpga_pci_enable < 0) {
        rc = onlp_file_read_int(&fpga_pci_enable, FPGA_PCI_ENABLE_SYSFS);
        if(rc != ONLP_STATUS_OK)
            fpga_pci_enable = 0;
    }

    return fpga_pci_enable;
}

//FPGA

void fpga_lock_init()
{
    static int sem_inited = 0;
    if(!sem_inited) {
        onlp_shlock_create(LOCK_MAGIC, &fpga_lock, "fpga_lock");
        sem_inited = 1;
    }
}

static int mmap_open(off_t offset)
{
    const char resource[] = FPGA_RESOURCE;

    // Open resource
    if (fpga_res_handle == -1)
    {
        fpga_res_handle = open(resource, O_RDWR|O_SYNC);
        if (fpga_res_handle == -1) {
            AIM_LOG_ERROR("%s() open resource failed, errno=%d, strerror=%s, resource=%s\n", __func__, errno, strerror(errno), resource);
            return ONLP_STATUS_E_INTERNAL;
        }
    }

    // Mapping resource
    mmap_base = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_NORESERVE, fpga_res_handle, offset & ~(PAGE_MASK));
    if (mmap_base == MAP_FAILED) {
        AIM_LOG_ERROR("%s() mmap failed, strerror=%d, strerror=%s, offset=0x%08lx\n", __func__, errno, strerror(errno), offset & ~(PAGE_MASK));
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

static int mmap_close()
{
    if(mmap_base != NULL) {
        int rc = 0;
        rc = munmap(mmap_base, PAGE_SIZE);
        if (rc < 0) {
            AIM_LOG_ERROR("%s() munmap failed, errno=%d, strerror=%s, rc=%d\n", __func__, errno, strerror(errno), rc);
            return ONLP_STATUS_E_INTERNAL;
        }
    }

    return ONLP_STATUS_OK;
}

static uint32_t fpga_write(uint32_t addr, uint32_t input_data)
{
    int rc = mmap_open(addr);

    if (rc != ONLP_STATUS_OK) {
        return rc;
    }

    volatile unsigned long *address =  (unsigned long *) ((unsigned long) mmap_base + (addr & (PAGE_MASK)));
    unsigned long data = (unsigned long) (input_data & 0xFFFFFFFF);
    *address = data;

    //wait write operation complete
    //usleep(1);

    if(DEBUG_FPGA_WRITE) {
        AIM_LOG_INFO("%s() [0x%08x] 0x%08x (readback 0x%08x)\n", __func__, addr, data, *address);
    }

    //check readback
    while((*address) != data) {
        *address = data;
        //AIM_LOG_INFO("%s [0x%08x] 0x%08x (readback 0x%08x)\n", __func__, addr, data, *address);
    }

    rc = mmap_close();

    if (rc != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("%s() mmap_close failed, rc=%d", __func__, rc);
        return rc;
    }

    return ONLP_STATUS_OK;
}

//read 4 bytes
static uint32_t fpga_read_dword(uint32_t addr)
{
    int rc = 0;
    uint32_t data = 0;

    rc = mmap_open(addr);

    if (rc != ONLP_STATUS_OK) {
        return 0;
    }

    data = *(uint32_t*) (mmap_base + (addr & (PAGE_MASK)));

    if(DEBUG_FPGA_READ) {
        AIM_LOG_INFO("%s [0x%08x] 0x%08x\n", __func__, addr, data);
    }

    rc = mmap_close();

    if (rc != ONLP_STATUS_OK) {
        return rc;
    }

    return data;
}

//read 2 byte
static uint32_t fpga_readw(uint32_t addr)
{
    return fpga_read_dword(addr) & 0xFFFF;
}

//read 1 byte
static uint32_t fpga_readb(uint32_t addr)
{
    return fpga_read_dword(addr) & 0xFF;
}

static uint32_t _fpga_ctrl_int_st_read(fpga_ctrl_int_st_t fpga_ctrl, uint32_t ctrl_addr)
{
    uint32_t addr = 0;
    uint32_t data = 0;

    addr = FPGA_CTRL_BASE + FPGA_TO_X86(ctrl_addr + fpga_ctrl.offset);
    data = fpga_read_dword(addr);

    return data;
}

static uint32_t _fpga_wait(fpga_ctrl_int_st_t fpga_ctrl, uint32_t ctrl_addr)
{
    uint32_t i2c_status = 0;
    struct timeval stop, start;
    long run_time = 0;
    long timeout = 1000000;
    int ret = ONLP_STATUS_OK;

    gettimeofday(&start, NULL);
    i2c_status = _fpga_ctrl_int_st_read(fpga_ctrl, ctrl_addr);

    while( (i2c_status & 0x1) == 0x0) {
        gettimeofday(&stop, NULL);
        run_time = (stop.tv_sec - start.tv_sec) * 1000000 + (stop.tv_usec - start.tv_usec);

        if(run_time > timeout) {
            //Timeout to Wait for I2C IDLE (1 second)
            AIM_LOG_ERROR("%s() Timeout to Wait for I2C IDLE, run_time=%ld, i2c_status=0x%x\n", __func__, run_time, i2c_status);
            return ONLP_STATUS_E_INTERNAL;
        }
        i2c_status = _fpga_ctrl_int_st_read(fpga_ctrl, ctrl_addr);
    }

    if (((i2c_status>>2) & 0x1) == 0x0) {
        if(DEBUG_FPGA_NO_ACT) {
            AIM_LOG_ERROR("%s() NO ACT Event Detected, i2c_status=0x%x\n", __func__, i2c_status);
        }
        ret = ONLP_STATUS_E_INTERNAL;
    } else if (((i2c_status>>1) & 0x1) == 0x0) {
        AIM_LOG_ERROR("%s() I2C Stuck Event Detected, i2c_status=0x%x\n", __func__, i2c_status);
        ret = ONLP_STATUS_E_INTERNAL;
    } else{
        ret = ONLP_STATUS_OK;
    }

    return ret;
}

static uint32_t _fpga_ctrl_field_val(fpga_ctrl_field field)
{
    return (field.val << field.shift);
}

static uint32_t _fpga_ctrl_0_val(fpga_ctrl_0_t fpga_ctrl)
{
    return _fpga_ctrl_field_val(fpga_ctrl.byte_0) +
           _fpga_ctrl_field_val(fpga_ctrl.byte_1);
}

static uint32_t _fpga_ctrl_1_val(fpga_ctrl_1_t fpga_ctrl)
{
    return _fpga_ctrl_field_val(fpga_ctrl.byte_2) +
           _fpga_ctrl_field_val(fpga_ctrl.byte_3);
}

static uint32_t _fpga_ctrl_2_val(fpga_ctrl_2_t fpga_ctrl)
{
    return _fpga_ctrl_field_val(fpga_ctrl.block_rw_len)     +
           _fpga_ctrl_field_val(fpga_ctrl.byte_mode)        +
           _fpga_ctrl_field_val(fpga_ctrl.i2c_reg_addr_len) +
           _fpga_ctrl_field_val(fpga_ctrl.page_set)         +
           _fpga_ctrl_field_val(fpga_ctrl.auto_mode);
}

static uint32_t _fpga_ctrl_3_val(fpga_ctrl_3_t fpga_ctrl)
{
    return _fpga_ctrl_field_val(fpga_ctrl.reg_addr)        +
           _fpga_ctrl_field_val(fpga_ctrl.slave_addr)      +
           _fpga_ctrl_field_val(fpga_ctrl.page_set_enable) +
           _fpga_ctrl_field_val(fpga_ctrl.i2c_chnl_preset) +
           _fpga_ctrl_field_val(fpga_ctrl.rw)              +
           _fpga_ctrl_field_val(fpga_ctrl.action);
}

static int _fpga_ctrl_0_write(fpga_ctrl_0_t fpga_ctrl, uint32_t ctrl_addr)
{
    uint32_t addr = 0;
    uint32_t data = 0;

    addr = FPGA_CTRL_BASE + FPGA_TO_X86(ctrl_addr + fpga_ctrl.offset);
    data = _fpga_ctrl_0_val(fpga_ctrl);

    return fpga_write(addr, data);
}

static int _fpga_ctrl_1_write(fpga_ctrl_1_t fpga_ctrl, uint32_t ctrl_addr)
{
    uint32_t addr = 0;
    uint32_t data = 0;

    addr = FPGA_CTRL_BASE + FPGA_TO_X86(ctrl_addr + fpga_ctrl.offset);
    data = _fpga_ctrl_1_val(fpga_ctrl);

    return fpga_write(addr, data);
}

static int _fpga_ctrl_2_write(fpga_ctrl_2_t fpga_ctrl, uint32_t ctrl_addr)
{
    uint32_t addr = 0;
    uint32_t data = 0;

    addr = FPGA_CTRL_BASE + FPGA_TO_X86(ctrl_addr + fpga_ctrl.offset);
    data = _fpga_ctrl_2_val(fpga_ctrl);

    return fpga_write(addr, data);
}

static int _fpga_ctrl_3_write(fpga_ctrl_3_t fpga_ctrl, uint32_t ctrl_addr)
{
    uint32_t addr = 0;
    uint32_t data = 0;

    addr = FPGA_CTRL_BASE + FPGA_TO_X86(ctrl_addr + fpga_ctrl.offset);
    data = _fpga_ctrl_3_val(fpga_ctrl);

    return fpga_write(addr, data);
}

static int _init_port_config(int port)
{
    port_node_t node = {0};
    ONLP_TRY(get_node(port, &node));

    // set ctrl_addr and mem_addr
    fpga_ctrl.ctrl_addr = node.ctrl_addr;
    fpga_ctrl.mem_addr = node.mem_addr;

    // set i2c_chnl_preset
    fpga_ctrl.fpga_ctrl_3.i2c_chnl_preset.val = node.channel;

    return ONLP_STATUS_OK;
}

static int _config_fpga_read(int port, uint8_t devaddr, uint8_t addr, int rw_len)
{
    int action = FPGA_ACTIVATE;
    int rw = FPGA_READ;
    int read_size = EEPROM_LOAD_MAX; //LUMENTUM:16
    int i = 0;

    VALIDATE_OFFSET(addr, rw_len);

    // init port
    _init_port_config(port);

    // config source sel
    //self._config_fpga_reg(FPGARegID.I2C_SOURCE_SEL)

    for(i=addr; i<addr+rw_len;){
        // config eeprom via cpld2_ctrl_2
        fpga_ctrl.fpga_ctrl_2.block_rw_len.val = (addr+rw_len-i > read_size) ? read_size : (addr+rw_len-i);
        //fpga_ctrl.fpga_ctrl_2.page_set.val = i/EEPROM_SIZE;

        _fpga_ctrl_2_write(fpga_ctrl.fpga_ctrl_2, fpga_ctrl.ctrl_addr);

        // execute command via cpld2_ctrl_3
        fpga_ctrl.fpga_ctrl_3.action.val = action;
        fpga_ctrl.fpga_ctrl_3.rw.val = rw;
        fpga_ctrl.fpga_ctrl_3.slave_addr.val = devaddr;
        fpga_ctrl.fpga_ctrl_3.reg_addr.val = i % EEPROM_SIZE;
        //fpga_ctrl.fpga_ctrl_3.page_set_enable.val = fpga_ctrl.fpga_ctrl_2.page_set.val > 0 ? 1 : 0;

        _fpga_ctrl_3_write(fpga_ctrl.fpga_ctrl_3, fpga_ctrl.ctrl_addr);
        _fpga_wait(fpga_ctrl.fpga_ctrl_int_st, fpga_ctrl.ctrl_addr);

        i += read_size;
    }

    return ONLP_STATUS_OK;
}

static int _config_fpga_write(int port, uint8_t devaddr, uint8_t addr, int rw_len, uint8_t* data)
{
    int action = FPGA_ACTIVATE;
    int rw = FPGA_WRITE;
    int i = 0;
    int loop_size_word = (rw_len/4);
    int loop_size_half_word = ((rw_len-loop_size_word*4)/2);
    int loop_size_byte = (rw_len - loop_size_word*4 - loop_size_half_word*2);
    int block_rw_len = 0;
    int data_offset = 0;
    int reg_offset = 0;
    int b_start = 0;

    VALIDATE_OFFSET(addr, rw_len);

    // init port
    _init_port_config(port);

    // config source sel
    //self._config_fpga_reg(FPGARegID.I2C_SOURCE_SEL)

    // write 4 bytes
    for(i=0; i<loop_size_word; ++i) {
        block_rw_len = 4;
        b_start = data_offset;

        // config eeprom via cpld2_ctrl_2
        fpga_ctrl.fpga_ctrl_2.block_rw_len.val = block_rw_len;
        _fpga_ctrl_2_write(fpga_ctrl.fpga_ctrl_2, fpga_ctrl.ctrl_addr);

        // config write data
        fpga_ctrl.fpga_ctrl_0.byte_0.val = data[b_start + 0];
        fpga_ctrl.fpga_ctrl_0.byte_1.val = data[b_start + 1];
        fpga_ctrl.fpga_ctrl_1.byte_2.val = data[b_start + 2];
        fpga_ctrl.fpga_ctrl_1.byte_3.val = data[b_start + 3];

        _fpga_ctrl_0_write(fpga_ctrl.fpga_ctrl_0, fpga_ctrl.ctrl_addr);
        _fpga_ctrl_1_write(fpga_ctrl.fpga_ctrl_1, fpga_ctrl.ctrl_addr);

        // execute command via cpld2_ctrl_3
        fpga_ctrl.fpga_ctrl_3.action.val = action;
        fpga_ctrl.fpga_ctrl_3.rw.val = rw;
        fpga_ctrl.fpga_ctrl_3.slave_addr.val = devaddr;
        fpga_ctrl.fpga_ctrl_3.reg_addr.val = addr;
        ONLP_TRY(_fpga_ctrl_3_write(fpga_ctrl.fpga_ctrl_3, fpga_ctrl.ctrl_addr));
        ONLP_TRY(_fpga_wait(fpga_ctrl.fpga_ctrl_int_st, fpga_ctrl.ctrl_addr));

        reg_offset += block_rw_len;
        data_offset += block_rw_len;
    }
    // write 2 bytes
    for(i=0; i<loop_size_half_word; ++i) {
        block_rw_len = 2;
        b_start = data_offset;

        // config eeprom via cpld2_ctrl_2
        fpga_ctrl.fpga_ctrl_2.block_rw_len.val = block_rw_len;
        _fpga_ctrl_2_write(fpga_ctrl.fpga_ctrl_2, fpga_ctrl.ctrl_addr);

        // config write data
        fpga_ctrl.fpga_ctrl_0.byte_0.val = data[b_start + 0];
        fpga_ctrl.fpga_ctrl_0.byte_1.val = data[b_start + 1];
        _fpga_ctrl_0_write(fpga_ctrl.fpga_ctrl_0, fpga_ctrl.ctrl_addr);

        // execute command via cpld2_ctrl_3
        fpga_ctrl.fpga_ctrl_3.action.val = action;
        fpga_ctrl.fpga_ctrl_3.rw.val = rw;
        fpga_ctrl.fpga_ctrl_3.slave_addr.val = devaddr;
        fpga_ctrl.fpga_ctrl_3.reg_addr.val = addr;
        ONLP_TRY(_fpga_ctrl_3_write(fpga_ctrl.fpga_ctrl_3, fpga_ctrl.ctrl_addr));
        ONLP_TRY(_fpga_wait(fpga_ctrl.fpga_ctrl_int_st, fpga_ctrl.ctrl_addr));

        reg_offset += block_rw_len;
        data_offset += block_rw_len;
    }
    // write 1 byte
    for(i=0; i<loop_size_byte; ++i) {
        block_rw_len = 1;
        b_start = data_offset;

        // config eeprom via cpld2_ctrl_2
        fpga_ctrl.fpga_ctrl_2.block_rw_len.val = block_rw_len;
        _fpga_ctrl_2_write(fpga_ctrl.fpga_ctrl_2, fpga_ctrl.ctrl_addr);

        // config write data
        fpga_ctrl.fpga_ctrl_0.byte_0.val = data[b_start + 0];
        _fpga_ctrl_0_write(fpga_ctrl.fpga_ctrl_0, fpga_ctrl.ctrl_addr);

        // execute command via cpld2_ctrl_3
        fpga_ctrl.fpga_ctrl_3.action.val = action;
        fpga_ctrl.fpga_ctrl_3.rw.val = rw;
        fpga_ctrl.fpga_ctrl_3.slave_addr.val = devaddr;
        fpga_ctrl.fpga_ctrl_3.reg_addr.val = addr;
        ONLP_TRY(_fpga_ctrl_3_write(fpga_ctrl.fpga_ctrl_3, fpga_ctrl.ctrl_addr));
        ONLP_TRY(_fpga_wait(fpga_ctrl.fpga_ctrl_int_st, fpga_ctrl.ctrl_addr));

        reg_offset += block_rw_len;
        data_offset += block_rw_len;
    }

    return ONLP_STATUS_OK;
}

static int _fpga_eeprom_readb(uint32_t mem_addr, uint8_t offset)
{
    uint32_t addr = 0;
    uint32_t data = 0;

    addr = FPGA_MEM_BASE + FPGA_TO_X86(mem_addr) + offset;
    data = fpga_readb(addr);

    return data;
}

static int _fpga_eeprom_readw(uint32_t mem_addr, uint8_t offset)
{
    uint32_t addr = 0;
    uint32_t data = 0;

    addr = FPGA_MEM_BASE + FPGA_TO_X86(mem_addr) + offset;
    data = fpga_readw(addr);

    return data;
}

static int _fpga_eeprom_read(uint32_t mem_addr, uint8_t offset, uint8_t* rdata, int size)
{
    uint32_t addr = 0;

    for(int i=0; i<size; ++i) {
        addr = FPGA_MEM_BASE + FPGA_TO_X86(mem_addr) + offset + i;
        rdata[i] = (uint8_t) fpga_readb(addr);
    }

    return ONLP_STATUS_OK;
}

static int fpga_sfpi_dev_readb(int port, uint8_t devaddr, uint8_t addr)
{
    int data = 0;
    int rw_len=1;

    FPGA_LOCK();

    // config fpga to read eeprom
    FPGA_TRY(_config_fpga_read(port, devaddr, addr, rw_len));

    // read eeprom from fpga
    data = _fpga_eeprom_readb(fpga_ctrl.mem_addr, addr);

    FPGA_UNLOCK();

    return data;
}

static int fpga_sfpi_dev_readw(int port, uint8_t devaddr, uint8_t addr)
{
    int data = 0;
    int rw_len=2;

    FPGA_LOCK();

    // config fpga to read eeprom
    FPGA_TRY(_config_fpga_read(port, devaddr, addr, rw_len));

    // read eeprom from fpga
    data = _fpga_eeprom_readw(fpga_ctrl.mem_addr, addr);

    FPGA_UNLOCK();

    return data;
}

static int fpga_sfpi_dev_read(int port, uint8_t devaddr, uint8_t addr, uint8_t* rdata, int size)
{
    int rc = 0;

    FPGA_LOCK();

    // config fpga to read eeprom
    FPGA_TRY(_config_fpga_read(port, devaddr, addr, size));

    // read eeprom from fpga
    rc = _fpga_eeprom_read(fpga_ctrl.mem_addr, addr, rdata, size);

    FPGA_UNLOCK();

    return rc;
}

static int fpga_sfpi_eeprom_read(int port, uint8_t data[256])
{
    int rc = 0;
    uint8_t devaddr = EEPROM_ADDR;
    uint8_t addr = 0x0;
    int size = EEPROM_SIZE;

    rc = fpga_sfpi_dev_read(port, devaddr, addr, data, size);

    return rc;
}

static int fpga_sfpi_dev_writeb(int port, uint8_t devaddr, uint8_t addr, uint8_t value)
{
    int rc = ONLP_STATUS_OK;
    int rw_len = 1;
    uint8_t data[1];

    data[0] = value;

    FPGA_LOCK();
    FPGA_TRY(_config_fpga_write(port, devaddr, addr, rw_len, data));
    FPGA_UNLOCK();

    return rc;
}

static int fpga_sfpi_dev_writew(int port, uint8_t devaddr, uint8_t addr, uint16_t value)
{
    int rc = ONLP_STATUS_OK;
    int rw_len = 2;
    uint8_t data[2];

    data[0] = value & 0xFF;
    data[1] = (value >> 8) & 0xFF;

    FPGA_LOCK();
    FPGA_TRY(_config_fpga_write(port, devaddr, addr, rw_len, data));
    FPGA_UNLOCK();

    return rc;
}

static int fpga_sfpi_dev_write(int port, uint8_t devaddr, uint8_t addr, uint8_t *value, int size)
{
    int rc = ONLP_STATUS_OK;

    FPGA_LOCK();
    FPGA_TRY(_config_fpga_write(port, devaddr, addr, size, value));
    FPGA_UNLOCK();

    return rc;
}

/**
 * @brief Initialize the SFPI subsystem.
 */
int onlp_sfpi_init(void)
{
    init_lock();
    fpga_lock_init();
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
    uint8_t bit = 0;
    port_node_t node = {0};

    ONLP_TRY(get_node(port, &node));
    if ((status = read_file_hex(&abs, node.abs_sysfs)) < 0) {
        AIM_LOG_ERROR("onlp_sfpi_is_present() failed, error=%d, sysfs=%s",
                          status, node.abs_sysfs);
        check_and_do_i2c_mux_reset(port);
        return status;
    }

    ONLP_TRY(get_bit(node.bit_mode, node.cpld_bit, &bit));
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

    if (is_fpga_pci_enable()) {
        rc = fpga_sfpi_eeprom_read(port, data);
    } else {
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

    if (is_fpga_pci_enable()) {
        rc = fpga_sfpi_dev_readb(port, devaddr, addr);
    } else {
        if ((rc=onlp_i2c_readb(node.eeprom_bus, devaddr, addr, ONLP_I2C_F_FORCE)) < 0) {
            check_and_do_i2c_mux_reset(port);
        }
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

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if (is_fpga_pci_enable()) {
        rc = fpga_sfpi_dev_writeb(port, devaddr, addr, value);
    } else {
        if ((rc=onlp_i2c_writeb(node.eeprom_bus, devaddr, addr, value, ONLP_I2C_F_FORCE)) < 0) {
            check_and_do_i2c_mux_reset(port);
        }
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

    if (is_fpga_pci_enable()) {
        rc = fpga_sfpi_dev_readw(port, devaddr, addr);
    } else {
        if ((rc=onlp_i2c_readw(node.eeprom_bus, devaddr, addr, ONLP_I2C_F_FORCE)) < 0) {
            check_and_do_i2c_mux_reset(port);
        }
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

    if (is_fpga_pci_enable()) {
        rc = fpga_sfpi_dev_writew(port, devaddr, addr, value);
    } else {
        if ((rc=onlp_i2c_writew(node.eeprom_bus, devaddr, addr, value, ONLP_I2C_F_FORCE)) < 0) {
            check_and_do_i2c_mux_reset(port);
        }
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

    if (is_fpga_pci_enable()) {
        rc = fpga_sfpi_dev_read(port, devaddr, addr, rdata, size);
    } else {
        if (onlp_i2c_block_read(node.eeprom_bus, devaddr, addr, size, rdata, ONLP_I2C_F_FORCE) < 0) {
            check_and_do_i2c_mux_reset(port);
            return ONLP_STATUS_E_INTERNAL;
        }
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

    if (is_fpga_pci_enable()) {
        rc = fpga_sfpi_dev_write(port, devaddr, addr, data, size);
    } else {
        if ((rc=onlp_i2c_write(node.eeprom_bus, devaddr, addr, size, data, ONLP_I2C_F_FORCE)) < 0) {
            check_and_do_i2c_mux_reset(port);
        }
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

    if (is_fpga_pci_enable()) {
        uint8_t devaddr = EEPROM_SFP_DOM_ADDR;
        uint8_t addr = 0x0;
        int size = 256;

        ONLP_TRY(fpga_sfpi_dev_read(port, devaddr, addr, data, size));
    } else {
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
            if (IS_QSFPDD(node)) {
                *rv = 1;
            }
            break;
        case ONLP_SFP_CONTROL_RX_LOS:
        case ONLP_SFP_CONTROL_TX_FAULT:
        case ONLP_SFP_CONTROL_TX_DISABLE:
            if (IS_SFP(node)) {
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
    port_node_t node = {0};

    ONLP_TRY(get_node(port, &node));

    //check control is valid for this port
    switch(control)
    {
        case ONLP_SFP_CONTROL_RESET:
            {
                if (IS_QSFPX(node)) {
                    //reverse value
                    value = (value == 0) ? 1:0;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }

                break;
            }
        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                if (IS_SFP(node)) {
                    break;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
            }
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                if (IS_QSFPX(node)) {
                    break;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
            }
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
    }

    //get sysfs
    ONLP_TRY(xfr_ctrl_to_sysfs(node, control, &sysfs));

    //read reg_val
    if (read_file_hex(&reg_val, sysfs) < 0) {
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }

    //update reg_val
    //0 is normal, 1 is reset, reverse value to fit our platform
    ONLP_TRY(get_bit(node.bit_mode,node.cpld_bit, &bit));
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
    port_node_t node = {0};

    ONLP_TRY(get_node(port, &node));

    //get sysfs
    ONLP_TRY(xfr_ctrl_to_sysfs(node, control, &sysfs));

    //read gpio value
    if ((rc = read_file_hex(&reg_val, sysfs)) < 0) {
        AIM_LOG_ERROR("onlp_sfpi_control_get() failed, error=%d, sysfs=%s", rc, sysfs);
        check_and_do_i2c_mux_reset(port);
        return rc;
    }

    ONLP_TRY(get_bit(node.bit_mode, node.cpld_bit, &bit));
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