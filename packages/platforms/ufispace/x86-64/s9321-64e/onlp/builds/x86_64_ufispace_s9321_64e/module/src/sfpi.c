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

#define SFP_PORT(_port)   (port-QSFPX_SFPDD_NUM)

#define IS_SFP(_port)     (_port >= QSFPX_SFPDD_NUM && _port < PORT_NUM)
#define IS_SFPDD(_port)   (_port >= QSFPX_NUM && _port < QSFPX_SFPDD_NUM)
#define IS_QSFPX(_port)   (IS_QSFP(_port) || IS_QSFPDD(_port))
#define IS_QSFP(_port)    (_port >= 0 && _port < QSFP_NUM)
#define IS_QSFPDD(_port)  (_port >= QSFP_NUM && _port < QSFPX_NUM)

#define MASK_SFP_PRESENT      0x01
#define MASK_SFP_TX_FAULT     0x02
#define MASK_SFP_RX_LOS       0x04
#define MASK_SFP_TX_DISABLE   0x01

#define SYSFS_SFP28_TX_DIS      "fpga_sfp28_tx_dis"
#define SYSFS_SFP28_PRESENT     "fpga_sfp28_intr_present"
#define SYSFS_SFPDD_PRESENT     "cpld_sfpdd_intr_present"
#define SYSFS_SFP28_RX_LOS      "fpga_sfp28_rx_los"
#define SYSFS_SFP28_TX_FAULT    "fpga_sfp28_tx_fault"
#define SYSFS_QSFP_RESET        "cpld_qsfp_reset"
#define SYSFS_QSFP_LPMODE       "cpld_qsfp_lpmode"
#define SYSFS_QSFP_PRESENT      "cpld_qsfp_intr_present"
#define SYSFS_QSFPDD_RESET      "cpld_qsfpdd_reset"
#define SYSFS_QSFPDD_LPMODE     "cpld_qsfpdd_lpmode"
#define SYSFS_QSFPDD_PRESENT    "cpld_qsfpdd_intr_present"
#define SYSFS_EEPROM            "eeprom"

#define VALIDATE_PORT(p) { if ((p < 0) || (p >= PORT_NUM)) return ONLP_STATUS_E_PARAM; }
#define VALIDATE_SFP_PORT(p) { if (!IS_SFP(p)) return ONLP_STATUS_E_PARAM; }

#define EEPROM_ADDR (0x50)

// FPGA
#define FPGA_RESOURCE "/sys/devices/pci0000:15/0000:15:04.0/0000:18:00.0/resource4"
#define PAGE_SIZE (4096)
#define PAGE_MASK (PAGE_SIZE-1)
#define EEPROM_SIZE (256)
#define EEPROM_LOAD_MAX (16)
#define FPGA_CTRL_BASE (0x04000000)
#define FPGA_MEM_BASE  FPGA_CTRL_BASE
#define FPGA_TO_X86(_addr) ((_addr) << 2)
#define FPGA_ENABLE       (1)
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

static int fpga_res_handle = -1;
static void *mmap_base = NULL;
static onlp_shlock_t* fpga_lock = NULL;

static port_fpga_info_t port_fpga_info[] =
{
    [0]   = {.ctrl_addr=0x1000, .mem_addr=0x0,   .channel=1},
    [1]   = {.ctrl_addr=0x1000, .mem_addr=0x0,   .channel=2},
    [2]   = {.ctrl_addr=0x1000, .mem_addr=0x0,   .channel=3},
    [3]   = {.ctrl_addr=0x1000, .mem_addr=0x0,   .channel=4},
    [4]   = {.ctrl_addr=0x1000, .mem_addr=0x0,   .channel=5},
    [5]   = {.ctrl_addr=0x1000, .mem_addr=0x0,   .channel=6},
    [6]   = {.ctrl_addr=0x1000, .mem_addr=0x0,   .channel=7},
    [7]   = {.ctrl_addr=0x1000, .mem_addr=0x0,   .channel=8},
    [8]   = {.ctrl_addr=0x1000, .mem_addr=0x0,   .channel=9},
    [9]   = {.ctrl_addr=0x1000, .mem_addr=0x0,   .channel=10},
    [10]  = {.ctrl_addr=0x1000, .mem_addr=0x0,   .channel=11},
    [11]  = {.ctrl_addr=0x1000, .mem_addr=0x0,   .channel=12},
    [12]  = {.ctrl_addr=0x1000, .mem_addr=0x0,   .channel=13},
    [13]  = {.ctrl_addr=0x1000, .mem_addr=0x0,   .channel=14},
    [14]  = {.ctrl_addr=0x1000, .mem_addr=0x0,   .channel=15},
    [15]  = {.ctrl_addr=0x1000, .mem_addr=0x0,   .channel=16},
    [16]  = {.ctrl_addr=0x3000, .mem_addr=0x200, .channel=1},
    [17]  = {.ctrl_addr=0x3000, .mem_addr=0x200, .channel=2},
    [18]  = {.ctrl_addr=0x3000, .mem_addr=0x200, .channel=3},
    [19]  = {.ctrl_addr=0x3000, .mem_addr=0x200, .channel=4},
    [20]  = {.ctrl_addr=0x3000, .mem_addr=0x200, .channel=5},
    [21]  = {.ctrl_addr=0x3000, .mem_addr=0x200, .channel=6},
    [22]  = {.ctrl_addr=0x3000, .mem_addr=0x200, .channel=7},
    [23]  = {.ctrl_addr=0x3000, .mem_addr=0x200, .channel=8},
    [24]  = {.ctrl_addr=0x3000, .mem_addr=0x200, .channel=9},
    [25]  = {.ctrl_addr=0x3000, .mem_addr=0x200, .channel=10},
    [26]  = {.ctrl_addr=0x3000, .mem_addr=0x200, .channel=11},
    [27]  = {.ctrl_addr=0x3000, .mem_addr=0x200, .channel=12},
    [28]  = {.ctrl_addr=0x3000, .mem_addr=0x200, .channel=13},
    [29]  = {.ctrl_addr=0x3000, .mem_addr=0x200, .channel=14},
    [30]  = {.ctrl_addr=0x3000, .mem_addr=0x200, .channel=15},
    [31]  = {.ctrl_addr=0x3000, .mem_addr=0x200, .channel=16},
    [32]  = {.ctrl_addr=0x2000, .mem_addr=0x100, .channel=1},
    [33]  = {.ctrl_addr=0x2000, .mem_addr=0x100, .channel=2},
    [34]  = {.ctrl_addr=0x2000, .mem_addr=0x100, .channel=3},
    [35]  = {.ctrl_addr=0x2000, .mem_addr=0x100, .channel=4},
    [36]  = {.ctrl_addr=0x2000, .mem_addr=0x100, .channel=5},
    [37]  = {.ctrl_addr=0x2000, .mem_addr=0x100, .channel=6},
    [38]  = {.ctrl_addr=0x2000, .mem_addr=0x100, .channel=7},
    [39]  = {.ctrl_addr=0x2000, .mem_addr=0x100, .channel=8},
    [40]  = {.ctrl_addr=0x2000, .mem_addr=0x100, .channel=9},
    [41]  = {.ctrl_addr=0x2000, .mem_addr=0x100, .channel=10},
    [42]  = {.ctrl_addr=0x2000, .mem_addr=0x100, .channel=11},
    [43]  = {.ctrl_addr=0x2000, .mem_addr=0x100, .channel=12},
    [44]  = {.ctrl_addr=0x2000, .mem_addr=0x100, .channel=13},
    [45]  = {.ctrl_addr=0x2000, .mem_addr=0x100, .channel=14},
    [46]  = {.ctrl_addr=0x2000, .mem_addr=0x100, .channel=15},
    [47]  = {.ctrl_addr=0x2000, .mem_addr=0x100, .channel=16},
    [48]  = {.ctrl_addr=0x4000, .mem_addr=0x300, .channel=1},
    [49]  = {.ctrl_addr=0x4000, .mem_addr=0x300, .channel=2},
    [50]  = {.ctrl_addr=0x4000, .mem_addr=0x300, .channel=3},
    [51]  = {.ctrl_addr=0x4000, .mem_addr=0x300, .channel=4},
    [52]  = {.ctrl_addr=0x4000, .mem_addr=0x300, .channel=5},
    [53]  = {.ctrl_addr=0x4000, .mem_addr=0x300, .channel=6},
    [54]  = {.ctrl_addr=0x4000, .mem_addr=0x300, .channel=7},
    [55]  = {.ctrl_addr=0x4000, .mem_addr=0x300, .channel=8},
    [56]  = {.ctrl_addr=0x4000, .mem_addr=0x300, .channel=9},
    [57]  = {.ctrl_addr=0x4000, .mem_addr=0x300, .channel=10},
    [58]  = {.ctrl_addr=0x4000, .mem_addr=0x300, .channel=11},
    [59]  = {.ctrl_addr=0x4000, .mem_addr=0x300, .channel=12},
    [60]  = {.ctrl_addr=0x4000, .mem_addr=0x300, .channel=13},
    [61]  = {.ctrl_addr=0x4000, .mem_addr=0x300, .channel=14},
    [62]  = {.ctrl_addr=0x4000, .mem_addr=0x300, .channel=15},
    [63]  = {.ctrl_addr=0x4000, .mem_addr=0x300, .channel=16},

    [64]  = {.ctrl_addr=0x5000, .mem_addr=0x400,  .channel=0},
    [65]  = {.ctrl_addr=0x6000, .mem_addr=0x500,  .channel=0},
};

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

static void fpga_lock_init();
static int mmap_open(off_t offset);
static uint32_t fpga_write(uint32_t addr, uint32_t input_data);
static uint32_t fpga_read_dword(uint32_t addr);
static uint32_t fpga_readb(uint32_t addr);
static uint32_t fpga_readw(uint32_t addr);
static port_fpga_info_t _get_port_fpga_info(int port);
static uint32_t _fpga_ctrl_int_st_read(fpga_ctrl_int_st_t fpga_ctrl, uint32_t ctrl_addr);
static uint32_t _fpga_wait(fpga_ctrl_int_st_t fpga_ctrl, uint32_t ctrl_addr);
static uint32_t _fpga_ctrl_field_val(fpga_ctrl_field field);
static uint32_t _fpga_ctrl_0_val(fpga_ctrl_0_t fpga_ctrl);
static uint32_t _fpga_ctrl_1_val(fpga_ctrl_1_t fpga_ctrl);
static uint32_t _fpga_ctrl_2_val(fpga_ctrl_2_t fpga_ctrl);
static uint32_t _fpga_ctrl_3_val(fpga_ctrl_3_t fpga_ctrl);
static int _fpga_ctrl_0_write(fpga_ctrl_0_t fpga_ctrl, uint32_t ctrl_addr);
static int _fpga_ctrl_1_write(fpga_ctrl_1_t fpga_ctrl, uint32_t ctrl_addr);
static int _fpga_ctrl_2_write(fpga_ctrl_2_t fpga_ctrl, uint32_t ctrl_addr);
static int _fpga_ctrl_3_write(fpga_ctrl_3_t fpga_ctrl, uint32_t ctrl_addr);
static int _init_port_config(int port);
static int _config_fpga_read(int port, uint8_t devaddr, uint8_t addr, int rw_len);
static int _config_fpga_write(int port, uint8_t devaddr, uint8_t addr, int rw_len, uint8_t* data);
static int _fpga_eeprom_readb(uint32_t mem_addr, uint8_t offset);
static int _fpga_eeprom_readw(uint32_t mem_addr, uint8_t offset);
static int _fpga_eeprom_read(uint32_t mem_addr, uint8_t offset, uint8_t* rdata, int size);
static int fpga_sfpi_dev_readb(int port, uint8_t devaddr, uint8_t addr);
static int fpga_sfpi_dev_readw(int port, uint8_t devaddr, uint8_t addr);
static int fpga_sfpi_dev_read(int port, uint8_t devaddr, uint8_t addr, uint8_t* rdata, int size);
static int fpga_sfpi_eeprom_read(int port, uint8_t data[256]);
static int fpga_sfpi_dev_writeb(int port, uint8_t devaddr, uint8_t addr, uint8_t value);
static int fpga_sfpi_dev_writew(int port, uint8_t devaddr, uint8_t addr, uint16_t value);
static int fpga_sfpi_dev_write(int port, uint8_t devaddr, uint8_t addr, uint8_t *value, int size);


static int ufi_port_to_cpld_addr(int port)
{
    int cpld_addr = 0;

    if (port >= 0 && port <= 15) {
        cpld_addr = CPLD_BASE_ADDR[1];
    } else if (port >= 16 && port <= 31) {
        cpld_addr = CPLD_BASE_ADDR[2];
    } else if (port >= 32 && port <= 47) {
        cpld_addr = CPLD_BASE_ADDR[1];
    } else if (port >= 48 && port <= 63) {
        cpld_addr = CPLD_BASE_ADDR[2];
    } else if (IS_SFP(port)) {
        cpld_addr = CPLD_BASE_ADDR[3];
    }

    return cpld_addr;
}

static int ufi_qsfp_port_to_sysfs_attr_offset(int port)
{
    int sysfs_attr_offset = -1;

    if (port >= 0 && port <= 7) {
        sysfs_attr_offset = 0;
    } else if (port >= 8 && port <= 15) {
        sysfs_attr_offset = 1;
    } else if (port >= 16 && port <= 23) {
        sysfs_attr_offset = 0;
    } else if (port >= 24 && port <= 31) {
        sysfs_attr_offset = 1;
    } else if (port >= 32 && port <= 39) {
        sysfs_attr_offset = 2;
    } else if (port >= 40 && port <= 47) {
        sysfs_attr_offset = 3;
    } else if (port >= 48 && port <= 55) {
        sysfs_attr_offset = 2;
    } else if (port >= 56 && port <= 63) {
        sysfs_attr_offset = 3;
    }

    return sysfs_attr_offset;
}

static int ufi_qsfp_port_to_bit_offset(int port)
{
    int bit_offset = -1; // Default value for an invalid port

    if (port >= 0 && port <= 63) {
        bit_offset = port % 8;
    } else if (port >= 64 && port <= 65) { //SFP28
        bit_offset = port % 2;
    }

    return bit_offset;
}

static int ufi_port_to_eeprom_bus(int port)
{
    int bus = -1;

    if (IS_QSFPDD(port)) { //QSFPDD
        bus =  port + 25;
    }else if (IS_SFP(port)) { //SFP
        bus =  port + 25;
    } else { //unknown ports
        AIM_LOG_ERROR("unknown ports, port=%d\n", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    return bus;
}

static int ufi_port_to_cpld_bus(int port)
{
    int bus = -1;

    if (IS_QSFPDD(port)) { //QSFPDD
        bus =  CPLD_I2C_BUS[1];
    } else if (IS_SFP(port)) { //SFP
        bus =  CPLD_I2C_BUS[1];
    } else { //unknown ports
        AIM_LOG_ERROR("unknown ports, port=%d\n", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    return bus;
}

#if 0 //TODO
static int ufi_qsfp_present_get(int port, int *pres_val)
{
    int reg_val = 0, rc = 0;
    int cpld_bus = 0, cpld_addr = 0, attr_offset = 0;

    //get cpld bus, cpld addr and sysfs_attr_offset
    cpld_bus = ufi_port_to_cpld_bus(port);
    cpld_addr = ufi_port_to_cpld_addr(port);
    attr_offset = ufi_qsfp_port_to_sysfs_attr_offset(port);

    //read register
    if ((rc = file_read_hex(&reg_val, SYS_FMT_OFFSET, cpld_bus, cpld_addr, SYSFS_QSFP_PRESENT, attr_offset)) < 0) {
        AIM_LOG_ERROR("Unable to read %s", SYSFS_QSFP_PRESENT);
        AIM_LOG_ERROR(SYS_FMT_OFFSET, cpld_bus, cpld_addr, SYSFS_QSFP_PRESENT, attr_offset);
        check_and_do_i2c_mux_reset(port);
        return rc;
    }

    *pres_val = !((reg_val >> ufi_qsfp_port_to_bit_offset(port)) & 0x1);

    return ONLP_STATUS_OK;
}
#endif

static int ufi_sfp_present_get(int port, int *pres_val)
{
    int reg_val = 0, rc = 0;
    int cpld_bus = 0, cpld_addr = 0;

    //get cpld bus and cpld addr
    cpld_bus = ufi_port_to_cpld_bus(port);
    cpld_addr = ufi_port_to_cpld_addr(port);

    //read register
    if ((rc = file_read_hex(&reg_val, SYS_FMT, cpld_bus, cpld_addr, SYSFS_SFP28_PRESENT)) < 0) {
        AIM_LOG_ERROR("Unable to read %s", SYSFS_SFP28_PRESENT);
        AIM_LOG_ERROR(SYS_FMT, cpld_bus, cpld_addr, SYSFS_SFP28_PRESENT);
        check_and_do_i2c_mux_reset(port);
        return rc;
    }

    *pres_val = !((reg_val >> (port-64)) & 0x1);

    return ONLP_STATUS_OK;
}

static int ufi_qsfpdd_present_get(int port, int *pres_val)
{
    int reg_val = 0, rc = 0;
    int cpld_bus = 0, cpld_addr = 0, attr_offset = 0;

    //get cpld bus, cpld addr and sysfs_attr_offset
    cpld_bus = ufi_port_to_cpld_bus(port);
    cpld_addr = ufi_port_to_cpld_addr(port);
    attr_offset = ufi_qsfp_port_to_sysfs_attr_offset(port);

    //read register
    if ((rc = file_read_hex(&reg_val, SYS_FMT_OFFSET, cpld_bus, cpld_addr, SYSFS_QSFPDD_PRESENT,attr_offset)) < 0) {
        AIM_LOG_ERROR("Unable to read %s", SYSFS_QSFPDD_PRESENT);
        AIM_LOG_ERROR(SYS_FMT_OFFSET, cpld_bus, cpld_addr, SYSFS_QSFPDD_PRESENT, attr_offset);
        check_and_do_i2c_mux_reset(port);
        return rc;
    }

    *pres_val = !((reg_val >> ufi_qsfp_port_to_bit_offset(port)) & 0x1);

    return ONLP_STATUS_OK;
}

/**
 * @brief Initialize the SFPI subsystem.
 */
int onlp_sfpi_init(void)
{
    lock_init();
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
    int status = ONLP_STATUS_OK;

    VALIDATE_PORT(port);

    //QSFPDD Ports
    if (IS_QSFPDD(port)) { //QSFPDD
        ONLP_TRY(ufi_qsfpdd_present_get(port, &status));
    } else if (IS_SFP(port)) { //SFP
        ONLP_TRY(ufi_sfp_present_get(port, &status));
    } else {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    return status;
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
    int i = 0, value = 0;

    /* Populate bitmap - QSFPDD ports */
    for(i = 0; i < (QSFPX_NUM); i++) {
        AIM_BITMAP_MOD(dst, i, 0);
    }

    /* Populate bitmap - SFP+ ports */
    for(i = QSFPX_SFPDD_NUM; i < PORT_NUM; i++) {
        ONLP_TRY(onlp_sfpi_control_get(i, ONLP_SFP_CONTROL_RX_LOS, &value));
        AIM_BITMAP_MOD(dst, i, value);
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

    if (FPGA_ENABLE) {
        rc = fpga_sfpi_eeprom_read(port, data);
    } else {
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
    VALIDATE_PORT(port);
    int rc = 0;
    int bus = ufi_port_to_eeprom_bus(port);

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if (FPGA_ENABLE) {
        rc = fpga_sfpi_dev_readb(port, devaddr, addr);
    } else {
        if ((rc=onlp_i2c_readb(bus, devaddr, addr, ONLP_I2C_F_FORCE)) < 0) {
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
    VALIDATE_PORT(port);
    int rc = 0;
    int bus = ufi_port_to_eeprom_bus(port);

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if (FPGA_ENABLE) {
        rc = fpga_sfpi_dev_writeb(port, devaddr, addr, value);
    } else {
        if ((rc=onlp_i2c_writeb(bus, devaddr, addr, value, ONLP_I2C_F_FORCE)) < 0) {
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
    VALIDATE_PORT(port);
    int rc = 0;
    int bus = ufi_port_to_eeprom_bus(port);

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if (FPGA_ENABLE) {
        rc = fpga_sfpi_dev_readw(port, devaddr, addr);
    } else {
        if ((rc=onlp_i2c_readw(bus, devaddr, addr, ONLP_I2C_F_FORCE)) < 0) {
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
    VALIDATE_PORT(port);
    int rc = 0;
    int bus = ufi_port_to_eeprom_bus(port);

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if (FPGA_ENABLE) {
        rc = fpga_sfpi_dev_writew(port, devaddr, addr, value);
    } else {
        if ((rc=onlp_i2c_writew(bus, devaddr, addr, value, ONLP_I2C_F_FORCE)) < 0) {
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
    VALIDATE_PORT(port);
    int bus = ufi_port_to_eeprom_bus(port);
    int rc = 0;

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if (FPGA_ENABLE) {
        rc = fpga_sfpi_dev_read(port, devaddr, addr, rdata, size);
    } else {
        if (onlp_i2c_block_read(bus, devaddr, addr, size, rdata, ONLP_I2C_F_FORCE) < 0) {
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
    VALIDATE_PORT(port);
    int rc = 0;
    int bus = ufi_port_to_eeprom_bus(port);

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if (FPGA_ENABLE) {
        rc = fpga_sfpi_dev_write(port, devaddr, addr, data, size);
    } else {
        if ((rc=onlp_i2c_write(bus, devaddr, addr, size, data, ONLP_I2C_F_FORCE)) < 0) {
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
    char eeprom_path[512];
    FILE* fp = NULL;
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
int onlp_sfpi_control_supported(int port, onlp_sfp_control_t control, int* rv)
{
    VALIDATE_PORT(port);

    //set unsupported as default value
    *rv = 0;

    switch (control) {
        case ONLP_SFP_CONTROL_RESET:
        case ONLP_SFP_CONTROL_RESET_STATE:
        case ONLP_SFP_CONTROL_LP_MODE:
            if (IS_QSFPDD(port)) {
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
    int bus = 0;
    int cpld_addr = 0;
    int attr_offset = 0, bit_offset = 0;

    VALIDATE_PORT(port);

    bus = ufi_port_to_cpld_bus(port);
    cpld_addr = ufi_port_to_cpld_addr(port);

    switch(control)
        {
        case ONLP_SFP_CONTROL_RESET:
            {
                if (IS_QSFPDD(port)) {
                    //read reg_val
                    attr_offset = ufi_qsfp_port_to_sysfs_attr_offset(port);
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFPDD_RESET, attr_offset) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //update reg_val
                    //0 is normal, 1 is reset, reverse value to fit our platform
                    bit_offset = ufi_qsfp_port_to_bit_offset(port);
                    reg_val = ufi_bit_operation(reg_val, bit_offset, !value);

                    //write reg_val
                    if ((rc=onlp_file_write_int(reg_val, SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFPDD_RESET, attr_offset)) < 0) {
                        AIM_LOG_ERROR("Unable to write %s, error=%d, reg_val=%x", SYSFS_QSFPDD_RESET,  rc, reg_val);
                        AIM_LOG_ERROR(SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFPDD_RESET);
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }
                    rc = ONLP_STATUS_OK;
                } else {
                    rc = ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                if (IS_SFP(port)) {
                    //read reg_val
                    if (file_read_hex(&reg_val, SYS_FMT, bus, cpld_addr, SYSFS_SFP28_TX_DIS) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //update reg_val
                    //0 is normal, 1 is turn off, reverse value to fit our platform
                    bit_offset = ufi_qsfp_port_to_bit_offset(port);
                    reg_val = ufi_bit_operation(reg_val, bit_offset, value);

                    //write reg_val
                    if (onlp_file_write_int(reg_val, SYS_FMT, bus, cpld_addr, SYSFS_SFP28_TX_DIS) < 0) {
                        AIM_LOG_ERROR("Unable to write %s, error=%d, reg_val=%x", SYSFS_SFP28_TX_DIS, rc, reg_val);
                        AIM_LOG_ERROR(SYS_FMT, bus, cpld_addr, SYSFS_SFP28_TX_DIS);
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }
                    rc = ONLP_STATUS_OK;
                } else {
                    rc = ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                if (IS_QSFPDD(port)) {
                    //read reg_val
                    attr_offset = ufi_qsfp_port_to_sysfs_attr_offset(port);
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFPDD_LPMODE, attr_offset) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //update reg_val
                    //0 is normal, 1 is low power
                    bit_offset = ufi_qsfp_port_to_bit_offset(port);
                    reg_val = ufi_bit_operation(reg_val, bit_offset, value);

                    //write reg_val
                    if (onlp_file_write_int(reg_val, SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFPDD_LPMODE, attr_offset) < 0) {
                        AIM_LOG_ERROR("Unable to write %s, error=%d, reg_val=%x", SYSFS_QSFPDD_LPMODE,  rc, reg_val);
                        AIM_LOG_ERROR(SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFPDD_LPMODE);
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }
                    rc = ONLP_STATUS_OK;
                } else {
                    rc = ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        default:
            rc = ONLP_STATUS_E_UNSUPPORTED;
        }

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
    int rc = 0;
    int reg_val = 0, reg_mask = 0;
    int bus = 0;
    int cpld_addr = 0;
    int attr_offset = 0, bit_offset = 0;

    VALIDATE_PORT(port);

    bus = ufi_port_to_cpld_bus(port);
    cpld_addr = ufi_port_to_cpld_addr(port);

    switch(control)
        {
        case ONLP_SFP_CONTROL_RESET_STATE:
            {
                if (IS_QSFPDD(port)) {
                    //read reg_val
                    attr_offset = ufi_qsfp_port_to_sysfs_attr_offset(port);
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFPDD_RESET, attr_offset) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //read bit value
                    //0 is normal, 1 is reset, reverse value to fit our platform
                    bit_offset = ufi_qsfp_port_to_bit_offset(port);
                    reg_mask = 1 << bit_offset;
                    *value = !ufi_mask_shift(reg_val, reg_mask);

                    rc = ONLP_STATUS_OK;
                } else {
                    rc = ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_RX_LOS:
            {
                if (IS_SFP(port)) {
                    //read reg_val
                    if (file_read_hex(&reg_val, SYS_FMT, bus, cpld_addr, SYSFS_SFP28_RX_LOS) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //read bit value
                    //0 is normal, 1 is rx_los
                    bit_offset = ufi_qsfp_port_to_bit_offset(port);
                    reg_mask = 1 << bit_offset;
                    *value = ufi_mask_shift(reg_val, reg_mask);

                    rc = ONLP_STATUS_OK;
                } else {
                    rc = ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_TX_FAULT:
            {
                if (IS_SFP(port)) {
                    //read reg_val
                    if (file_read_hex(&reg_val, SYS_FMT, bus, cpld_addr, SYSFS_SFP28_TX_FAULT) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //read bit value
                    //0 is normal, 1 is tx_fault
                    bit_offset = ufi_qsfp_port_to_bit_offset(port);
                    reg_mask = 1 << bit_offset;
                    *value = ufi_mask_shift(reg_val, reg_mask);

                    rc = ONLP_STATUS_OK;
                } else {
                    rc = ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                if (IS_SFP(port)) {
                    //read reg_val
                    if (file_read_hex(&reg_val, SYS_FMT, bus, cpld_addr, SYSFS_SFP28_TX_DIS) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //read bit value
                    //0 is normal, 1 is tx_disable
                    bit_offset = ufi_qsfp_port_to_bit_offset(port);
                    reg_mask = 1 << bit_offset;
                    *value = ufi_mask_shift(reg_val, reg_mask);

                    rc = ONLP_STATUS_OK;
                } else {
                    rc = ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                if (IS_QSFPDD(port)) {
                    //read reg_val
                    attr_offset = ufi_qsfp_port_to_sysfs_attr_offset(port);
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFPDD_LPMODE, attr_offset) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //read bit value
                    //0 is normal, 1 is low power
                    bit_offset = ufi_qsfp_port_to_bit_offset(port);
                    reg_mask = 1 << bit_offset;
                    *value = ufi_mask_shift(reg_val, reg_mask);

                    rc = ONLP_STATUS_OK;
                } else {
                    rc = ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        default:
            rc = ONLP_STATUS_E_UNSUPPORTED;
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

//read 1 byte
static uint32_t fpga_readb(uint32_t addr)
{
    return fpga_read_dword(addr) & 0xFF;
}

//read 2 byte
static uint32_t fpga_readw(uint32_t addr)
{
    return fpga_read_dword(addr) & 0xFFFF;
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

static port_fpga_info_t _get_port_fpga_info(int port)
{
    return port_fpga_info[port];
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
    port_fpga_info_t port_fpga_info = _get_port_fpga_info(port);

    // set ctrl_addr and mem_addr
    fpga_ctrl.ctrl_addr = port_fpga_info.ctrl_addr;
    fpga_ctrl.mem_addr = port_fpga_info.mem_addr;

    // set i2c_chnl_preset
    fpga_ctrl.fpga_ctrl_3.i2c_chnl_preset.val = port_fpga_info.channel;

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