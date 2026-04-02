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

#define QSFP_NUM              2
#define QSFPDD_NUM            2
#define SFP_NUM               24
#define MGMT_NUM              0
#define OSFP_NUM              0
#define QSFPX_NUM             (QSFP_NUM+QSFPDD_NUM)
#define XSFPX_NUM             (QSFP_NUM+QSFPDD_NUM+OSFP_NUM)
#define PORT_NUM              (XSFPX_NUM+SFP_NUM+MGMT_NUM)

#define SYSFS_EEPROM         "eeprom"
#define SYSFS_DEV_CLASS      "dev_class"
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
    int abs_gpin;
    int abs_gpin_b;
    int lpmode_gpin;
    int lpmode_gpin_b;
    int reset_gpin;
    int reset_gpin_b;
    int rxlos_gpin;
    int rxlos_gpin_b;
    int txfault_gpin;
    int txfault_gpin_b;
    int txdis_gpin;
    int txdis_gpin_b;
    int eeprom_bus;
    int port_type;
} port_attr_t;

static const port_attr_t port_attr[] = {
/*  port  abs       lpmode  reset   rxlos     txfault txdis   eeprom  type */
    [0] ={24 , 23 , 20, 27, 16 ,31, -1 , -1 , -1, -1, -1, -1, 13    , TYPE_QSFPDD},
    [1] ={25 , 22 , 21, 26, 17 ,30, -1 , -1 , -1, -1, -1, -1, 12    , TYPE_QSFPDD},
    [2] ={26 , 21 , 22, 25, 18 ,29, -1 , -1 , -1, -1, -1, -1, 11    , TYPE_QSFP  },
    [3] ={27 , 20 , 23, 24, 19 ,28, -1 , -1 , -1, -1, -1, -1, 10    , TYPE_QSFP  },
    [4] ={136, 135, -1, -1, -1 ,-1, 168, 167, 72, 71, 40, 39, 14    , TYPE_SFP   },
    [5] ={137, 134, -1, -1, -1 ,-1, 169, 166, 73, 70, 41, 38, 15    , TYPE_SFP   },
    [6] ={138, 133, -1, -1, -1 ,-1, 170, 165, 74, 69, 42, 37, 16    , TYPE_SFP   },
    [7] ={139, 132, -1, -1, -1 ,-1, 171, 164, 75, 68, 43, 36, 17    , TYPE_SFP   },
    [8] ={140, 131, -1, -1, -1 ,-1, 172, 163, 76, 67, 44, 35, 18    , TYPE_SFP   },
    [9] ={141, 130, -1, -1, -1 ,-1, 173, 162, 77, 66, 45, 34, 19    , TYPE_SFP   },
    [10]={142, 129, -1, -1, -1 ,-1, 174, 161, 78, 65, 46, 33, 20    , TYPE_SFP   },
    [11]={143, 128, -1, -1, -1 ,-1, 175, 160, 79, 64, 47, 32, 21    , TYPE_SFP   },
    [12]={128, 143, -1, -1, -1 ,-1, 160, 175, 64, 79, 32, 47, 22    , TYPE_SFP   },
    [13]={129, 142, -1, -1, -1 ,-1, 161, 174, 65, 78, 33, 46, 23    , TYPE_SFP   },
    [14]={130, 141, -1, -1, -1 ,-1, 162, 173, 66, 77, 34, 45, 24    , TYPE_SFP   },
    [15]={131, 140, -1, -1, -1 ,-1, 163, 172, 67, 76, 35, 44, 25    , TYPE_SFP   },
    [16]={132, 139, -1, -1, -1 ,-1, 164, 171, 68, 75, 36, 43, 26    , TYPE_SFP   },
    [17]={133, 138, -1, -1, -1 ,-1, 165, 170, 69, 74, 37, 42, 27    , TYPE_SFP   },
    [18]={134, 137, -1, -1, -1 ,-1, 166, 169, 70, 73, 38, 41, 28    , TYPE_SFP   },
    [19]={135, 136, -1, -1, -1 ,-1, 167, 168, 71, 72, 39, 40, 29    , TYPE_SFP   },
    [20]={152, 151, -1, -1, -1 ,-1, 184, 183, 88, 87, 56, 55, 30    , TYPE_SFP   },
    [21]={153, 150, -1, -1, -1 ,-1, 185, 182, 89, 86, 57, 54, 31    , TYPE_SFP   },
    [22]={154, 149, -1, -1, -1 ,-1, 186, 181, 90, 85, 58, 53, 32    , TYPE_SFP   },
    [23]={155, 148, -1, -1, -1 ,-1, 187, 180, 91, 84, 59, 52, 33    , TYPE_SFP   },
    [24]={156, 147, -1, -1, -1 ,-1, 188, 179, 92, 83, 60, 51, 34    , TYPE_SFP   },
    [25]={157, 146, -1, -1, -1 ,-1, 189, 178, 93, 82, 61, 50, 35    , TYPE_SFP   },
    [26]={158, 145, -1, -1, -1 ,-1, 190, 177, 94, 81, 62, 49, 36    , TYPE_SFP   },
    [27]={159, 144, -1, -1, -1 ,-1, 191, 176, 95, 80, 63, 48, 37    , TYPE_SFP   },
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

static int gpio_max = -1;
static int gpio_base = -1;

int ufi_port_to_gpio_num(int port, onlp_sfp_control_t control)
{
    int gpio_num = -1;

    ONLP_TRY(onlp_sfpi_init());
    switch(control)
    {
        case ONLP_SFP_CONTROL_RESET:
        case ONLP_SFP_CONTROL_RESET_STATE:
            {
                if(gpio_max < 0)
                    gpio_num = gpio_base + port_attr[port].reset_gpin_b;
                else 
                    gpio_num = gpio_max - port_attr[port].reset_gpin;
                break;
            }
        case ONLP_SFP_CONTROL_RX_LOS:
            {
                if(gpio_max < 0)
                    gpio_num = gpio_base + port_attr[port].rxlos_gpin_b;
                else 
                    gpio_num = gpio_max - port_attr[port].rxlos_gpin;
                break;
            }
        case ONLP_SFP_CONTROL_TX_FAULT:
            {
                if(gpio_max < 0)
                    gpio_num = gpio_base + port_attr[port].txfault_gpin_b;
                else 
                    gpio_num = gpio_max - port_attr[port].txfault_gpin;
                break;
            }
        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                if(gpio_max < 0)
                    gpio_num = gpio_base + port_attr[port].txdis_gpin_b;
                else 
                    gpio_num = gpio_max - port_attr[port].txdis_gpin;
                break;
            }
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                if(gpio_max < 0)
                    gpio_num = gpio_base + port_attr[port].lpmode_gpin_b;
                else 
                    gpio_num = gpio_max - port_attr[port].lpmode_gpin;
                break;
            }
        default:
            gpio_num=-1;
    }

    if (gpio_num<0) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    return gpio_num;
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

    VALIDATE_PORT(port);
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

    VALIDATE_PORT(port);
    bus = ufi_port_to_eeprom_bus(port);

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
    mem_model = ufi_mask_shift(onlp_sfpi_dev_readb(port, EEPROM_ADDR, CMIS_OFFSET_MEMORY_MODEL), CMIS_MASK_MEMORY_MODEL);
    if (mem_model != CMIS_VAL_MEMORY_MODEL_PAGED) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    //Check CMIS Tx disable advertisement on page 0x01 offset[155] bit[1]
    VALIDATE_PORT(port);
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

    tx_dis_adv = ufi_mask_shift(value, CMIS_MASK_TX_DIS_ADV);

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

/**
 * @brief Initialize the SFPI subsystem.
 */
int onlp_sfpi_init(void)
{
    if (gpio_max != -1 || gpio_base != -1) {
        return ONLP_STATUS_OK;
    }

    lock_init();
    ONLP_TRY(ufi_get_gpio_max(&gpio_max));
    ONLP_TRY(ufi_get_gpio_base(&gpio_base));
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
    int status=ONLP_STATUS_OK, gpio_num;
    int abs = 0, present = 0;

    ONLP_TRY(onlp_sfpi_init());
    VALIDATE_PORT(port);

    //set gpio_num by port
    if(gpio_max < 0)
        gpio_num = gpio_base + port_attr[port].abs_gpin_b;
    else 
        gpio_num = gpio_max - port_attr[port].abs_gpin;

    if ((status = file_read_hex(&abs, SYS_GPIO_FMT, gpio_num)) < 0) {
        AIM_LOG_ERROR("onlp_sfpi_is_present() failed, error=%d, sysfs=%s, gpio_num=%d",
            status, SYS_GPIO_FMT, gpio_num);
        check_and_do_i2c_mux_reset(port);
        return status;
    }

    present = (abs==0) ? 1:0;

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

    /* Populate bitmap - QSFPDD and QSFP ports */
    for(i = 0; i < (QSFPX_NUM); i++) {
        AIM_BITMAP_MOD(dst, i, 0);
    }

    /* Populate bitmap - SFP+ ports */
    for(i = QSFPX_NUM; i < PORT_NUM; i++) {
        if (onlp_sfpi_control_get(i, ONLP_SFP_CONTROL_RX_LOS, &value)< 0) {
            return ONLP_STATUS_E_INTERNAL;
        } else {
            AIM_BITMAP_MOD(dst, i, value);
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
    int rc = ONLP_STATUS_OK;
    int gpio_num = 0;
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
        //get gpio_num
        if ((rc = ufi_port_to_gpio_num(port, control)) < 0) {
            AIM_LOG_ERROR("ufi_port_to_gpio_num() failed, error=%d, port=%d, control=%d", rc, port, control);
            return rc;
        } else {
            gpio_num = rc;
        }

        //write gpio value
        if ((rc = onlp_file_write_int(value, SYS_GPIO_FMT, gpio_num)) < 0) {
            AIM_LOG_ERROR("onlp_sfpi_control_set() failed, error=%d, sysfs=%s, gpio_num=%d", rc, SYS_GPIO_FMT, gpio_num);
            check_and_do_i2c_mux_reset(port);
            return rc;
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
    int gpio_num = 0;
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
        //get gpio_num
        if ((rc = ufi_port_to_gpio_num(port, control)) < 0) {
            AIM_LOG_ERROR("ufi_port_to_gpio_num() failed, error=%d, port=%d, control=%d", rc, port, control);
            return rc;
        } else {
            gpio_num = rc;
        }

        //read gpio value
        if ((rc = file_read_hex(value, SYS_GPIO_FMT, gpio_num)) < 0) {
            AIM_LOG_ERROR("onlp_sfpi_control_get() failed, error=%d, sysfs=%s, gpio_num=%d", rc, SYS_GPIO_FMT, gpio_num);
            check_and_do_i2c_mux_reset(port);
            return rc;
        }

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

