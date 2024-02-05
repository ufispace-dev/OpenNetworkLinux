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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>


#define AIM_LOG_ERROR printf
#define AIM_LOG_INFO printf

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;


#define ONLP_TRY(_expr)                              \
    do {                                             \
        int _rv = (_expr);                           \
        if(_rv<0) {                                  \
            AIM_LOG_ERROR("%s returned %d\n", #_expr, _rv); \
            return _rv;                              \
        }                                            \
    } while(0)

#define FPGA_TRY(_expr)                              \
    do {                                             \
        int _rv = (_expr);                           \
        if(_rv<0) {                                  \
            AIM_LOG_ERROR("%s returned %d\n", #_expr, _rv); \
            FPGA_UNLOCK();                           \
            return _rv;                              \
        }                                            \
    } while(0)

#define FPGA_LOCK(_expr) {}
#define FPGA_UNLOCK(_expr) {}
#define EEPROM_ADDR (0x50)

typedef enum onlp_status_e {
    ONLP_STATUS_OK = 0,
    ONLP_STATUS_E_GENERIC = -1,
    ONLP_STATUS_E_UNSUPPORTED = -10,
    ONLP_STATUS_E_MISSING = -11,
    ONLP_STATUS_E_INVALID = -12,
    ONLP_STATUS_E_INTERNAL = -13,
    ONLP_STATUS_E_PARAM = -14,
    ONLP_STATUS_E_I2C = -15,
} onlp_status_t;

#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/mman.h>


//FPGA

enum fpga_action_type_e {
    FPGA_DEACTIVATE = 0,
    FPGA_ACTIVATE   = 1
};

enum fpga_rw_type_e {
    FPGA_READ  = 0,
    FPGA_WRITE = 1
};

typedef struct port_fpga_info_s
{
    uint32_t ctrl_addr;
    uint32_t mem_addr;
    uint8_t  channel;
} port_fpga_info_t;

typedef struct fpga_ctrl_field_s
{
    uint32_t val;
    uint8_t  shift;
} fpga_ctrl_field;

typedef struct fpga_rst_ctrl_s
{
    uint32_t offset;
    fpga_ctrl_field rst_data;
} fpga_rst_ctrl_t;

typedef struct fpga_ctrl_int_st_s
{
    uint32_t offset;
    fpga_ctrl_field i2c_idle;
    fpga_ctrl_field i2c_stuck;
    fpga_ctrl_field i2c_no_ack;
} fpga_ctrl_int_st_t;

typedef struct fpga_ctrl_0_s
{
    uint32_t offset;
    fpga_ctrl_field byte_0;
    fpga_ctrl_field byte_1;
} fpga_ctrl_0_t;

typedef struct fpga_ctrl_1_s
{
    uint32_t offset;
    fpga_ctrl_field byte_2;
    fpga_ctrl_field byte_3;
} fpga_ctrl_1_t;

typedef struct fpga_ctrl_2_s
{
    uint32_t offset;
    fpga_ctrl_field auto_mode;
    fpga_ctrl_field page_set;
    fpga_ctrl_field i2c_reg_addr_len;
    fpga_ctrl_field byte_mode;
    fpga_ctrl_field block_rw_len;
} fpga_ctrl_2_t;

typedef struct fpga_ctrl_3_s
{
    uint32_t offset;
    fpga_ctrl_field action;
    fpga_ctrl_field rw;
    fpga_ctrl_field i2c_chnl_preset;
    fpga_ctrl_field page_set_enable;
    fpga_ctrl_field slave_addr;
    fpga_ctrl_field reg_addr;
} fpga_ctrl_3_t;

typedef struct fpga_ctrl_s
{
    uint32_t ctrl_addr;
    uint32_t mem_addr;
    fpga_rst_ctrl_t fpga_rst_ctrl;
    fpga_ctrl_int_st_t fpga_ctrl_int_st;
    fpga_ctrl_0_t fpga_ctrl_0;
    fpga_ctrl_1_t fpga_ctrl_1;
    fpga_ctrl_2_t fpga_ctrl_2;
    fpga_ctrl_3_t fpga_ctrl_3;
} fpga_ctrl_t;

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

static int fpga_res_handle = -1;
static void *mmap_base = NULL;

static port_fpga_info_t port_fpga_info[] =
{
    [0]   = {.ctrl_addr=0x1000, .mem_addr=0x0,    .channel=1},
    [1]   = {.ctrl_addr=0x1000, .mem_addr=0x0,    .channel=2},
    [2]   = {.ctrl_addr=0x1000, .mem_addr=0x0,    .channel=3},
    [3]   = {.ctrl_addr=0x1000, .mem_addr=0x0,    .channel=4},
    [4]   = {.ctrl_addr=0x1000, .mem_addr=0x0,    .channel=5},
    [5]   = {.ctrl_addr=0x1000, .mem_addr=0x0,    .channel=6},
    [6]   = {.ctrl_addr=0x1000, .mem_addr=0x0,    .channel=7},
    [7]   = {.ctrl_addr=0x1000, .mem_addr=0x0,    .channel=8},
    [8]   = {.ctrl_addr=0x1000, .mem_addr=0x0,    .channel=9},
    [9]   = {.ctrl_addr=0x1000, .mem_addr=0x0,    .channel=10},
    [10]  = {.ctrl_addr=0x1000, .mem_addr=0x0,    .channel=11},
    [11]  = {.ctrl_addr=0x1000, .mem_addr=0x0,    .channel=12},
    [12]  = {.ctrl_addr=0x1000, .mem_addr=0x0,    .channel=13},
    [13]  = {.ctrl_addr=0x1000, .mem_addr=0x0,    .channel=14},
    [14]  = {.ctrl_addr=0x1000, .mem_addr=0x0,    .channel=15},
    [15]  = {.ctrl_addr=0x1000, .mem_addr=0x0,    .channel=16},

    [16]  = {.ctrl_addr=0x3000, .mem_addr=0x200,  .channel=1},
    [17]  = {.ctrl_addr=0x3000, .mem_addr=0x200,  .channel=2},
    [18]  = {.ctrl_addr=0x3000, .mem_addr=0x200,  .channel=3},
    [19]  = {.ctrl_addr=0x3000, .mem_addr=0x200,  .channel=4},
    [20]  = {.ctrl_addr=0x3000, .mem_addr=0x200,  .channel=5},
    [21]  = {.ctrl_addr=0x3000, .mem_addr=0x200,  .channel=6},
    [22]  = {.ctrl_addr=0x3000, .mem_addr=0x200,  .channel=7},
    [23]  = {.ctrl_addr=0x3000, .mem_addr=0x200,  .channel=8},
    [24]  = {.ctrl_addr=0x3000, .mem_addr=0x200,  .channel=9},
    [25]  = {.ctrl_addr=0x3000, .mem_addr=0x200,  .channel=10},
    [26]  = {.ctrl_addr=0x3000, .mem_addr=0x200,  .channel=11},
    [27]  = {.ctrl_addr=0x3000, .mem_addr=0x200,  .channel=12},
    [28]  = {.ctrl_addr=0x3000, .mem_addr=0x200,  .channel=13},
    [29]  = {.ctrl_addr=0x3000, .mem_addr=0x200,  .channel=14},
    [30]  = {.ctrl_addr=0x3000, .mem_addr=0x200,  .channel=15},
    [31]  = {.ctrl_addr=0x3000, .mem_addr=0x200,  .channel=16},

    [32]  = {.ctrl_addr=0x2000, .mem_addr=0x100,  .channel=1},
    [33]  = {.ctrl_addr=0x2000, .mem_addr=0x100,  .channel=2},
    [34]  = {.ctrl_addr=0x2000, .mem_addr=0x100,  .channel=3},
    [35]  = {.ctrl_addr=0x2000, .mem_addr=0x100,  .channel=4},
    [36]  = {.ctrl_addr=0x2000, .mem_addr=0x100,  .channel=5},
    [37]  = {.ctrl_addr=0x2000, .mem_addr=0x100,  .channel=6},
    [38]  = {.ctrl_addr=0x2000, .mem_addr=0x100,  .channel=7},
    [39]  = {.ctrl_addr=0x2000, .mem_addr=0x100,  .channel=8},
    [40]  = {.ctrl_addr=0x2000, .mem_addr=0x100,  .channel=9},
    [41]  = {.ctrl_addr=0x2000, .mem_addr=0x100,  .channel=10},
    [42]  = {.ctrl_addr=0x2000, .mem_addr=0x100,  .channel=11},
    [43]  = {.ctrl_addr=0x2000, .mem_addr=0x100,  .channel=12},
    [44]  = {.ctrl_addr=0x2000, .mem_addr=0x100,  .channel=13},
    [45]  = {.ctrl_addr=0x2000, .mem_addr=0x100,  .channel=14},
    [46]  = {.ctrl_addr=0x2000, .mem_addr=0x100,  .channel=15},
    [47]  = {.ctrl_addr=0x2000, .mem_addr=0x100,  .channel=16},

    [48]  = {.ctrl_addr=0x4000, .mem_addr=0x300,  .channel=1},
    [49]  = {.ctrl_addr=0x4000, .mem_addr=0x300,  .channel=2},
    [50]  = {.ctrl_addr=0x4000, .mem_addr=0x300,  .channel=3},
    [51]  = {.ctrl_addr=0x4000, .mem_addr=0x300,  .channel=4},
    [52]  = {.ctrl_addr=0x4000, .mem_addr=0x300,  .channel=5},
    [53]  = {.ctrl_addr=0x4000, .mem_addr=0x300,  .channel=6},
    [54]  = {.ctrl_addr=0x4000, .mem_addr=0x300,  .channel=7},
    [55]  = {.ctrl_addr=0x4000, .mem_addr=0x300,  .channel=8},
    [56]  = {.ctrl_addr=0x4000, .mem_addr=0x300,  .channel=9},
    [57]  = {.ctrl_addr=0x4000, .mem_addr=0x300,  .channel=10},
    [58]  = {.ctrl_addr=0x4000, .mem_addr=0x300,  .channel=11},
    [59]  = {.ctrl_addr=0x4000, .mem_addr=0x300,  .channel=12},
    [60]  = {.ctrl_addr=0x4000, .mem_addr=0x300,  .channel=13},
    [61]  = {.ctrl_addr=0x4000, .mem_addr=0x300,  .channel=14},
    [62]  = {.ctrl_addr=0x4000, .mem_addr=0x300,  .channel=15},
    [63]  = {.ctrl_addr=0x4000, .mem_addr=0x300,  .channel=16},

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
static int fpga_sfpi_dev_writeb(int port, uint8_t devaddr, uint8_t addr, uint8_t value);
static int fpga_sfpi_dev_writew(int port, uint8_t devaddr, uint8_t addr, uint16_t value);
static int fpga_sfpi_dev_write(int port, uint8_t devaddr, uint8_t addr, uint8_t *value, int size);
int ut_fpga_eeprom_read(int port);
int ut_fpga_sfpi_dev_read(int port);

//FPGA

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
        AIM_LOG_INFO("%s() [0x%08x] 0x%08lx (readback 0x%08lx)\n", __func__, addr, data, *address);
    }

    //check readback
    while((*address) != data) {
        *address = data;
        //AIM_LOG_INFO("%s [0x%08x] 0x%08x (readback 0x%08x)\n", __func__, addr, data, *address);
    }

    //sync data from memory to device immediately
    //if (msync((void *)mmap_base, PAGE_SIZE, MS_SYNC) < 0) {
	//	perror("msync");
	//	exit(1);
	//}

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

char decode_data(uint8_t code) {
    if (code >= 0x20 && code <= 0x7E)
        return code;
    else
       return '.';
}

void hexdump(uint8_t* data) {

    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f    0123456789abcdef\n");

    for (int i=0; i<256; i+=16) {
        printf("%02x: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x    %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
        i,
        data[i], data[i+1], data[i+2], data[i+3],
        data[i+4],  data[i+5],  data[i+6],  data[i+7],
        data[i+8],  data[i+9],  data[i+10], data[i+11],
        data[i+12], data[i+13], data[i+14], data[i+15],
        decode_data(data[i]),   decode_data(data[i+1]),  decode_data(data[i+2]),  decode_data(data[i+3]),
        decode_data(data[i+4]), decode_data(data[i+5]),  decode_data(data[i+6]),  decode_data(data[i+7]),
        decode_data(data[i+8]), decode_data(data[i+9]),  decode_data(data[i+10]), decode_data(data[i+11]),
        decode_data(data[i+12]),decode_data(data[i+13]), decode_data(data[i+14]), decode_data(data[i+15])
        );
    }
}

int main_write(int port)
{
    //uint8_t write_data[8]= {0x0, 0x0, 0xf3, 0xf4};
    uint8_t write_data[8]= {0x0, 0x0, 0x10, 0x11};

    uint8_t devaddr=0x50;

    //write byte
    //fpga_sfpi_dev_writeb(port, devaddr, 0x1f, 0xff);

    //write word
    //fpga_sfpi_dev_writew(port, devaddr, 0x1f, 0xf2f1);

    //write word
    fpga_sfpi_dev_write(port, devaddr, 0x7a, write_data, 4);

    return 0;
}

int ut_fpga_sfpi_dev_writeb(int port)
{
    uint8_t devaddr=0x50;
    uint8_t addr = 0x1f; //0x1e-0x21 is writeable on Lumentum module

    //write value 0xf1 to addr 0x1f
    fpga_sfpi_dev_writeb(port, devaddr, addr, 0xf1);
    ut_fpga_sfpi_dev_read(port);

    //write value 0x0 to addr 0x1f
    fpga_sfpi_dev_writeb(port, devaddr, addr, 0x00);
    ut_fpga_sfpi_dev_read(port);

    return 0;
}

int ut_fpga_sfpi_dev_writew(int port)
{
    uint8_t devaddr=0x50;
    uint8_t addr = 0x1e; //0x1e-0x21 is writeable on Lumentum module

    //write value 0xf2f1 to addr 0x1e
    fpga_sfpi_dev_writew(port, devaddr, addr, 0xf2f1);
    ut_fpga_sfpi_dev_read(port);

    //write value 0x0 to addr 0x1e
    fpga_sfpi_dev_writew(port, devaddr, addr, 0x00);
    ut_fpga_sfpi_dev_read(port);

    return 0;
}

//block write 4 bytes to luxshare module 0x7a-0x7d
int ut_fpga_sfpi_dev_write_luxshare(int port)
{
    uint8_t write_data[4]= {0x0, 0x0, 0x10, 0x11};
    int size = 4;
    uint8_t devaddr=0x50;
    uint8_t addr = 0x7a;

    //write word
    fpga_sfpi_dev_write(port, devaddr, addr, write_data, size);

    return 0;
}

int main_read(int port)
{
    uint8_t data[256];

    uint8_t devaddr=0x50;
    uint8_t addr = 0;
    uint32_t dword = 0;

    //read fpga id
    dword = fpga_read_dword(0);
    printf("0x%08x\n", dword);


    dword = fpga_read_dword(0x4040);
    printf("0x%08x\n", dword);

    //dword = fpga_read_dword(1);
    //printf("0x%08x\n", dword);

    //dword = fpga_read_dword(2);
    //printf("0x%08x\n", dword);

    //dword = fpga_read_dword(3);
    //printf("0x%08x\n", dword);

    //dword = fpga_read_dword(0x1000);
    //printf("0x%08x\n", dword);


    int b =0;

    //fpga_sfpi_dev_readb, read byte

    memset(data, 0, 256);
    for(int i=0; i<256; ++i) {
        data[i] = fpga_sfpi_dev_readb(port, devaddr, i);
    }
    printf("read byte\n");
    hexdump(data);

    //fpga_sfpi_dev_readw, read 2 bytes

    memset(data, 0, 256);
    for(int i=0; i<128; ++i) {
        b = fpga_sfpi_dev_readw(port, devaddr, i*2);
        data[i*2] = b & 0xff;
        data[i*2+1] = (b >> 8) & 0xff;
        //printf("[%d] 0x%08x\n", i, b);
    }
    printf("read 2 bytes\n");
    hexdump(data);

    //fpga_sfpi_dev_read, read N bytes

    memset(data, 0, 256);
    fpga_sfpi_dev_read(port, devaddr, addr, data, 256);
    printf("read port [%d]\n", port);
    hexdump(data);

    port=2;
    memset(data, 0, 256);
    fpga_sfpi_dev_read(port, devaddr, addr, data, 256);
    printf("read port [%d]\n", port);
    hexdump(data);

    return 0;
}

int main2()
{
    const char resource[] = FPGA_RESOURCE;
    off_t offset = 0;

    // Open resource
    if (fpga_res_handle == -1)
    {
        fpga_res_handle = open(resource, O_RDWR|O_SYNC);
        if (fpga_res_handle == -1) {
            AIM_LOG_ERROR("%d %s\\n", errno, strerror(errno));
            return ONLP_STATUS_E_INTERNAL;
        }
    }

    // Mapping resource
    offset = offset & ~(PAGE_MASK);
    mmap_base = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_NORESERVE, fpga_res_handle, offset);
    if (mmap_base == MAP_FAILED) {
        AIM_LOG_ERROR("%s() failed, errno=%d, strerr=%s, offset=0x%08lx\n", __func__, errno, strerror(errno), offset);
        return ONLP_STATUS_E_INTERNAL;
    } else {
        AIM_LOG_ERROR("%s() ok, offset=0x%08lx\n", __func__, offset);
    }

    AIM_LOG_ERROR("[0] 0x%08x\n", *(uint32_t*) mmap_base);
    AIM_LOG_ERROR("[1] 0x%08x\n", *(uint32_t*) (mmap_base+1));
    AIM_LOG_ERROR("[2] 0x%08x\n", *(uint32_t*) (mmap_base+2));
    AIM_LOG_ERROR("[3] 0x%08x\n", *(uint32_t*) (mmap_base+3));

    AIM_LOG_ERROR("[4] 0x%08x\n", *(uint32_t*) (mmap_base+4));

    //AIM_LOG_ERROR("[4040] 0x%08x\n", *(uint32_t*) (mmap_base+4040));
    //AIM_LOG_ERROR("[4095] 0x%08x\n", *(uint32_t*) (mmap_base+4095));
    //AIM_LOG_ERROR("[4096] 0x%08x\n", *(uint32_t*) (mmap_base+4096));
    //AIM_LOG_ERROR("[4097] 0x%08x\n", *(uint32_t*) (mmap_base+4097));
    //AIM_LOG_ERROR("[0x1010] 0x%08x\n", *(uint32_t*) (mmap_base+0x1010));

    offset = 0x4000;
    mmap_base = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_NORESERVE, fpga_res_handle, offset & ~(PAGE_MASK));
    if (mmap_base == MAP_FAILED) {
        AIM_LOG_ERROR("%s() failed, errno=%d, strerr=%s, offset=0x%08lx\n", __func__, errno, strerror(errno), offset & ~(PAGE_MASK));
        return ONLP_STATUS_E_INTERNAL;
    } else {
        AIM_LOG_ERROR("%s() ok, offset=0x%08lx\n", __func__, offset);
    }

    AIM_LOG_ERROR("[0x4040] 0x%08x\n", *(uint32_t*) (mmap_base + (offset & (PAGE_MASK)) + 0x40) );
    AIM_LOG_ERROR("[0x4080] 0x%08x\n", *(uint32_t*) (mmap_base + (offset & (PAGE_MASK)) + 0x80) );
    AIM_LOG_ERROR("[0x40c0] 0x%08x\n", *(uint32_t*) (mmap_base + (offset & (PAGE_MASK)) + 0xC0) );

    return 0;
}

//read module and update to fpga ram
int ut_fpga_sfpi_dev_read(int port) {
    uint8_t data[256];
    uint8_t devaddr=0x50;
    uint8_t addr = 0;

    //fpga_sfpi_dev_read, read N bytes

    memset(data, 0, 256);
    fpga_sfpi_dev_read(port, devaddr, addr, data, 256);

    hexdump(data);

    return 0;
}

//read data from fpga ram, no data reload from module
int ut_fpga_eeprom_read(int port) {
    uint8_t data[256];
    int size = 256;

    memset(data, 0, size);

    _init_port_config(port);
    _fpga_eeprom_read(fpga_ctrl.mem_addr, 0, data, size);

    printf("read port [%d]\n", port);
    hexdump(data);

    return 0;
}

//change page and format output
int ut_change_page(int port)
{
    uint8_t devaddr=0x50;
    uint8_t addr = 0x7f;
    int page_start = 0;
    int page_end = 1;

    for(int page=page_start; page<=page_end; ++page) {
        //write reg 0x7f val 0-256
        fpga_sfpi_dev_writeb(port, devaddr, addr, page);

        //read
        printf("port[%02d] page[0x%02X]\n", port, page);
        ut_fpga_sfpi_dev_read(port);
    }

    return 0;
}

//[QSFP/QSFPDD] change page and output bytes
int ut_read_page_qsfp(int port, uint8_t _page)
{
    uint8_t devaddr=0x50;
    uint8_t addr = 0x0;
    uint8_t page_reg = 0x7f;
    int page = 0;
    int page_size = 128;
    uint8_t data[128];

    //_page=0: lower page
    //_page=1: upper page 0
    //_page=2: upper page 1
    //etc

    if (_page == 0) {
        addr = 0x0;
        page = _page;
    } else {
        addr = 0x80;
        page = _page - 1;
    }

    //change page
    ONLP_TRY(fpga_sfpi_dev_writeb(port, devaddr, page_reg, page));

    //read bytes
    memset(data, 0, page_size);
    fpga_sfpi_dev_read(port, devaddr, addr, data, page_size);

    //output bytes
    for(int i=0; i<page_size; ++i){
        printf("%c", data[i]);
    }

    return 0;
}

//[SFP] read and output 256 bytes
int ut_read_page_sfp(int port)
{
    uint8_t devaddr=0x50;
    uint8_t addr = 0x0;
    int page_size = 256;
    uint8_t data[256];

    memset(data, 0, page_size);
    fpga_sfpi_dev_read(port, devaddr, addr, data, page_size);

    //read
    for(int i=0; i<page_size; ++i){
        printf("%c", data[i]);
    }

    return 0;
}

#define USAGE_FMT        "%s [port] [page]"
static int E_TYPE_SUCCESS = 0;
static int E_TYPE_NOT_SUPPORTED = 32;

static void usage(char *progname) {
   fprintf(stderr, "Usage: "USAGE_FMT"\n", progname);
   fprintf(stderr, "-h   help\n");

   exit(E_TYPE_SUCCESS);
}

int main_read_page(int argc, char *argv[]) {
    int ret = E_TYPE_SUCCESS;
    int port = 0;

    /* Read Option */
    if (argc > 3) {
        printf("ERROR: Too much parameters. "
                "The tool (%s) Only supports two parameter!!\n", argv[0]);
        return E_TYPE_NOT_SUPPORTED;
    } else if (argc < 2 ) {
        usage(argv[0]);
    }

    port = atoi(argv[1]);
    if (port < 64) {
        if(argc<3){
            usage(argv[0]);
        } else {
            ut_read_page_qsfp(port, atoi(argv[2]));
        }
    } else {
        ut_read_page_sfp(port);
    }

    return ret;
}

int main_fpga_eeprom_reload(int argc, char *argv[]) {
    int ret = E_TYPE_SUCCESS;

    /* Read Option */
    if (argc > 2) {
        printf("ERROR: Too much parameters. "
                "The tool (%s) Only supports one parameter!!\n", argv[0]);
        return E_TYPE_NOT_SUPPORTED;
    } else if (argc != 2 ) {
        usage(argv[0]);
    }

    ut_fpga_sfpi_dev_read(atoi(argv[1]));

    return ret;
}

int main(int argc, char *argv[]) {

    main_read_page(argc, argv);

    return 0;
}