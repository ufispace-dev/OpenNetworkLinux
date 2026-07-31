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

#define SFP_NUM               (4)
#define SFP_PLUS_NUM          (8)
#define SFP28_NUM             (8)
#define PORT_NUM              (SFP_NUM + SFP_PLUS_NUM + SFP28_NUM)

#define SYSFS_EEPROM          "eeprom"
#define EEPROM_ADDR           (0x50)

typedef enum port_type_e {
    TYPE_SFP = 0,
    TYPE_SFP_PLUS,
    TYPE_SFP28,
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
} port_attr_t;

typedef enum cpld_attr_idx_e {
    /* Absent */
    PORT_0_ABS = 0,
    PORT_1_ABS,
    PORT_2_ABS,
    PORT_3_ABS,
    PORT_4_ABS,
    PORT_5_ABS,
    PORT_6_ABS,
    PORT_7_ABS,
    PORT_8_ABS,
    PORT_9_ABS,
    PORT_10_ABS,
    PORT_11_ABS,
    PORT_12_ABS,
    PORT_13_ABS,
    PORT_14_ABS,
    PORT_15_ABS,
    PORT_16_ABS,
    PORT_17_ABS,
    PORT_18_ABS,
    PORT_19_ABS,

    /* RX LOS */
    PORT_0_RX_LOS,
    PORT_1_RX_LOS,
    PORT_2_RX_LOS,
    PORT_3_RX_LOS,
    PORT_4_RX_LOS,
    PORT_5_RX_LOS,
    PORT_6_RX_LOS,
    PORT_7_RX_LOS,
    PORT_8_RX_LOS,
    PORT_9_RX_LOS,
    PORT_10_RX_LOS,
    PORT_11_RX_LOS,
    PORT_12_RX_LOS,
    PORT_13_RX_LOS,
    PORT_14_RX_LOS,
    PORT_15_RX_LOS,
    PORT_16_RX_LOS,
    PORT_17_RX_LOS,
    PORT_18_RX_LOS,
    PORT_19_RX_LOS,

    /* TX Fault */
    PORT_0_TX_FAULT,
    PORT_1_TX_FAULT,
    PORT_2_TX_FAULT,
    PORT_3_TX_FAULT,
    PORT_4_TX_FAULT,
    PORT_5_TX_FAULT,
    PORT_6_TX_FAULT,
    PORT_7_TX_FAULT,
    PORT_8_TX_FAULT,
    PORT_9_TX_FAULT,
    PORT_10_TX_FAULT,
    PORT_11_TX_FAULT,
    PORT_12_TX_FAULT,
    PORT_13_TX_FAULT,
    PORT_14_TX_FAULT,
    PORT_15_TX_FAULT,
    PORT_16_TX_FAULT,
    PORT_17_TX_FAULT,
    PORT_18_TX_FAULT,
    PORT_19_TX_FAULT,

    /* TX Disable */
    PORT_0_TX_DISABLE,
    PORT_1_TX_DISABLE,
    PORT_2_TX_DISABLE,
    PORT_3_TX_DISABLE,
    PORT_4_TX_DISABLE,
    PORT_5_TX_DISABLE,
    PORT_6_TX_DISABLE,
    PORT_7_TX_DISABLE,
    PORT_8_TX_DISABLE,
    PORT_9_TX_DISABLE,
    PORT_10_TX_DISABLE,
    PORT_11_TX_DISABLE,
    PORT_12_TX_DISABLE,
    PORT_13_TX_DISABLE,
    PORT_14_TX_DISABLE,
    PORT_15_TX_DISABLE,
    PORT_16_TX_DISABLE,
    PORT_17_TX_DISABLE,
    PORT_18_TX_DISABLE,
    PORT_19_TX_DISABLE,

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

//  port  abs            lpmode        reset        rxlos              txfault             txdis                 eeprom   type
    [0] ={PORT_0_ABS    ,CPLD_NONE    ,CPLD_NONE   ,PORT_0_RX_LOS     ,PORT_0_TX_FAULT    ,PORT_0_TX_DISABLE    ,14      ,TYPE_SFP        },
    [1] ={PORT_1_ABS    ,CPLD_NONE    ,CPLD_NONE   ,PORT_1_RX_LOS     ,PORT_1_TX_FAULT    ,PORT_1_TX_DISABLE    ,15      ,TYPE_SFP        },
    [2] ={PORT_2_ABS    ,CPLD_NONE    ,CPLD_NONE   ,PORT_2_RX_LOS     ,PORT_2_TX_FAULT    ,PORT_2_TX_DISABLE    ,16      ,TYPE_SFP        },
    [3] ={PORT_3_ABS    ,CPLD_NONE    ,CPLD_NONE   ,PORT_3_RX_LOS     ,PORT_3_TX_FAULT    ,PORT_3_TX_DISABLE    ,17      ,TYPE_SFP        },
    [4] ={PORT_4_ABS    ,CPLD_NONE    ,CPLD_NONE   ,PORT_4_RX_LOS     ,PORT_4_TX_FAULT    ,PORT_4_TX_DISABLE    ,18      ,TYPE_SFP_PLUS   },
    [5] ={PORT_5_ABS    ,CPLD_NONE    ,CPLD_NONE   ,PORT_5_RX_LOS     ,PORT_5_TX_FAULT    ,PORT_5_TX_DISABLE    ,19      ,TYPE_SFP_PLUS   },
    [6] ={PORT_6_ABS    ,CPLD_NONE    ,CPLD_NONE   ,PORT_6_RX_LOS     ,PORT_6_TX_FAULT    ,PORT_6_TX_DISABLE    ,20      ,TYPE_SFP_PLUS   },
    [7] ={PORT_7_ABS    ,CPLD_NONE    ,CPLD_NONE   ,PORT_7_RX_LOS     ,PORT_7_TX_FAULT    ,PORT_7_TX_DISABLE    ,21      ,TYPE_SFP_PLUS   },
    [8] ={PORT_8_ABS    ,CPLD_NONE    ,CPLD_NONE   ,PORT_8_RX_LOS     ,PORT_8_TX_FAULT    ,PORT_8_TX_DISABLE    ,22      ,TYPE_SFP_PLUS   },
    [9] ={PORT_9_ABS    ,CPLD_NONE    ,CPLD_NONE   ,PORT_9_RX_LOS     ,PORT_9_TX_FAULT    ,PORT_9_TX_DISABLE    ,23      ,TYPE_SFP_PLUS   },
    [10]={PORT_10_ABS   ,CPLD_NONE    ,CPLD_NONE   ,PORT_10_RX_LOS    ,PORT_10_TX_FAULT   ,PORT_10_TX_DISABLE   ,24      ,TYPE_SFP_PLUS   },
    [11]={PORT_11_ABS   ,CPLD_NONE    ,CPLD_NONE   ,PORT_11_RX_LOS    ,PORT_11_TX_FAULT   ,PORT_11_TX_DISABLE   ,25      ,TYPE_SFP_PLUS   },
    [12]={PORT_12_ABS   ,CPLD_NONE    ,CPLD_NONE   ,PORT_12_RX_LOS    ,PORT_12_TX_FAULT   ,PORT_12_TX_DISABLE   ,26      ,TYPE_SFP28      },
    [13]={PORT_13_ABS   ,CPLD_NONE    ,CPLD_NONE   ,PORT_13_RX_LOS    ,PORT_13_TX_FAULT   ,PORT_13_TX_DISABLE   ,27      ,TYPE_SFP28      },
    [14]={PORT_14_ABS   ,CPLD_NONE    ,CPLD_NONE   ,PORT_14_RX_LOS    ,PORT_14_TX_FAULT   ,PORT_14_TX_DISABLE   ,28      ,TYPE_SFP28      },
    [15]={PORT_15_ABS   ,CPLD_NONE    ,CPLD_NONE   ,PORT_15_RX_LOS    ,PORT_15_TX_FAULT   ,PORT_15_TX_DISABLE   ,29      ,TYPE_SFP28      },
    [16]={PORT_16_ABS   ,CPLD_NONE    ,CPLD_NONE   ,PORT_16_RX_LOS    ,PORT_16_TX_FAULT   ,PORT_16_TX_DISABLE   ,30      ,TYPE_SFP28      },
    [17]={PORT_17_ABS   ,CPLD_NONE    ,CPLD_NONE   ,PORT_17_RX_LOS    ,PORT_17_TX_FAULT   ,PORT_17_TX_DISABLE   ,31      ,TYPE_SFP28      },
    [18]={PORT_18_ABS   ,CPLD_NONE    ,CPLD_NONE   ,PORT_18_RX_LOS    ,PORT_18_TX_FAULT   ,PORT_18_TX_DISABLE   ,32      ,TYPE_SFP28      },
    [19]={PORT_19_ABS   ,CPLD_NONE    ,CPLD_NONE   ,PORT_19_RX_LOS    ,PORT_19_TX_FAULT   ,PORT_19_TX_DISABLE   ,33      ,TYPE_SFP28      },
};

#define IS_PORT_INVALID(_port)  ((_port < 0) || (_port >= PORT_NUM))  /* port number is 0 base */
#define IS_SFP(_port)           (port_attr[_port].port_type == TYPE_SFP || port_attr[_port].port_type == TYPE_SFP_PLUS || port_attr[_port].port_type == TYPE_SFP28)
#define VALIDATE_PORT(p) { if (IS_PORT_INVALID(p)) return ONLP_STATUS_E_PARAM; }
#define VALIDATE_SFP_PORT(p) { if (IS_PORT_INVALID(p) || !IS_SFP(p)) return ONLP_STATUS_E_PARAM; }

/******************************************************************************************************************
**                                                                                                               **
**                                           Upispace Specific Defined APIs                                      **
**                                                                                                               **
*******************************************************************************************************************/
/* Q2N new functions */
static int get_port_absent_sysfs(int port, char** sysfs)
{
    if(sysfs == NULL)
        return ONLP_STATUS_E_PARAM;

    *sysfs = (char *)malloc(256);
    if (*sysfs == NULL)
        return ONLP_STATUS_E_PARAM;

    snprintf(*sysfs, 256, "%s%s%d%s", SYSFS_CPLD2, "port_", port, "_abs");

    return ONLP_STATUS_OK;
}

static int xfr_ctrl_to_sysfs(int port, onlp_sfp_control_t control , char **sysfs)
{
    int rv  = ONLP_STATUS_OK;

    if (sysfs == NULL)
        return ONLP_STATUS_E_PARAM;

    switch(control)
    {
        case ONLP_SFP_CONTROL_RX_LOS:
            {
                *sysfs = (char *)malloc(256);
                if (*sysfs == NULL)
                    return ONLP_STATUS_E_PARAM;
                snprintf(*sysfs, 256, "%s%s%d%s", SYSFS_CPLD2, "port_", port, "_rx_los");
                break;
            }
        case ONLP_SFP_CONTROL_TX_FAULT:
            {
                *sysfs = (char *)malloc(256);
                if (*sysfs == NULL)
                    return ONLP_STATUS_E_PARAM;
                snprintf(*sysfs, 256, "%s%s%d%s", SYSFS_CPLD2, "port_", port, "_tx_fault");
                break;
            }
        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                *sysfs = (char *)malloc(256);
                if (*sysfs == NULL)
                    return ONLP_STATUS_E_PARAM;
                snprintf(*sysfs, 256, "%s%s%d%s", SYSFS_CPLD2, "port_", port, "_tx_disable");
                break;
            }
        default:
            rv = ONLP_STATUS_E_UNSUPPORTED;
            *sysfs = NULL;
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

/******************************************************************************************************************
**                                                                                                               **
**                                                ONLP Standard APIs                                             **
**                                                                                                               **
*******************************************************************************************************************/
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

    VALIDATE_PORT(port);

    ONLP_TRY(get_port_absent_sysfs(port, &sysfs));

    if ((status = read_file_hex(&abs, sysfs)) < 0) {
        AIM_LOG_ERROR("%s() failed, error=%d, sysfs=%s", __func__, status, sysfs);
        free(sysfs);
        check_and_do_i2c_mux_reset(port);
        return status;
    }

    /* Reverse Absent bit for ONL */
    present = (abs == 0)? 1:0;

    free(sysfs);
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
 * @brief Update device class for QSFPDD ports
 * [Dummy function]
 * This function updates the device class for a given QSFPDD port.
 * It reads the current device class and module type, then checks against a dev type list
 * to determine the correct device class.
 * If the device class needs to be updated, it writes the new value to dev_class.
 *
 * @param port The port number
 * @return An error condition or current port dev_class.
 */
int onlp_sfpi_dev_class_update(int port)
{
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
 * @brief Read the SFP EEPROM.
 * @param port The port number.
 * @param data Receives the SFP data.
 */
int onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{
    int size = 0, bus = 0, rc = 0;
    char sysfs_path[256] = {0};

    VALIDATE_PORT(port);
    bus = xfr_port_to_eeprom_bus(port);

    // create and check sysfs_path
    size = snprintf(sysfs_path, sizeof(sysfs_path),
        SYS_FMT, bus, EEPROM_ADDR, SYSFS_EEPROM);

    if (size < 0 || (size_t)size >= sizeof(sysfs_path)) {
        return ONLP_STATUS_E_INTERNAL;
    }

    // reset page select to 0
    ufi_reset_page_select(sysfs_path);

    memset(data, 0, 256);
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

    if (onlp_sfpi_is_present(port) != 1) {
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

    if (onlp_sfpi_is_present(port) != 1) {
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

    if(onlp_sfpi_is_present(port) != 1) {
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

    if(onlp_sfpi_is_present(port) != 1) {
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

    if (onlp_sfpi_is_present(port) != 1) {
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

    if (onlp_sfpi_is_present(port) != 1) {
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

    /* set unsupported as default value */
    *rv=0;

    switch (control) {
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
    char *sysfs = NULL;

    VALIDATE_PORT(port);

    /* check control is valid for this port */
    switch(control)
    {
        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                if (IS_SFP(port)) {
                    break;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
            }
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
    }

    /* get sysfs */
    ONLP_TRY(xfr_ctrl_to_sysfs(port, control, &sysfs));

    /* write value */
    if ((rc=onlp_file_write_int(value, sysfs)) < 0) {
        AIM_LOG_ERROR("Unable to write %s, error=%d, value=%x", sysfs,  rc, value);
        free(sysfs);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }
    rc = ONLP_STATUS_OK;

    free(sysfs);
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
    char *sysfs = NULL;

    VALIDATE_PORT(port);

    /* check control is valid for this port */
    switch(control)
    {
        case ONLP_SFP_CONTROL_RX_LOS:
        case ONLP_SFP_CONTROL_TX_FAULT:
        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                if (IS_SFP(port)) {
                    break;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
            }
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
    }

    /* get sysfs */
    ONLP_TRY(xfr_ctrl_to_sysfs(port, control, &sysfs));

    /* read cpld sysfs value */
    if ((rc = read_file_hex(value, sysfs)) < 0) {
        AIM_LOG_ERROR("onlp_sfpi_control_get() failed, error=%d, sysfs=%s", rc, sysfs);
        free(sysfs);
        check_and_do_i2c_mux_reset(port);
        return rc;
    }

    free(sysfs);
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
