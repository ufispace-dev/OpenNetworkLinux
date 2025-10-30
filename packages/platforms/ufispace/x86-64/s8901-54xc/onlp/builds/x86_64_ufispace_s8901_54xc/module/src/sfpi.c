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

#define SYSFS_DEV_CLASS         "dev_class"
#define ALL_PORTS             -1

#define SFP_NUM         48
#define QSFP_NUM        6
#define QSFPDD_NUM      0
#define QSFPX_NUM       (QSFP_NUM+QSFPDD_NUM)
#define PORT_NUM        (SFP_NUM+QSFPX_NUM)

#define SFP_PORT(_port) (port-QSFPX_NUM)

#define IS_SFP(_port)     (_port >= 0 && _port < SFP_NUM)
#define IS_QSFPX(_port)   (_port >= SFP_NUM && _port < PORT_NUM)
#define IS_QSFP(_port)    (_port >= SFP_NUM && _port < PORT_NUM)
#define IS_QSFPDD(_port)  (0)

#define SFP0_INTERFACE_NAME "enp182s0f0"
#define SFP1_INTERFACE_NAME "enp182s0f1"

#define MASK_SFP_PRESENT      0x01
#define MASK_SFP_TX_FAULT     0x02
#define MASK_SFP_RX_LOS       0x04
#define MASK_SFP_TX_DISABLE   0x01

#define SYSFS_SFP_CONFIG     "cpld_sfp_config"
#define SYSFS_SFP_STATUS     "cpld_sfp_status"
#define SYSFS_SFP_PRESENT    "cpld_sfp_intr_present"
#define SYSFS_SFP_RX_LOS     "cpld_sfp_intr_rx_los"
#define SYSFS_SFP_TX_FAULT   "cpld_sfp_intr_tx_fault"
#define SYSFS_SFP_TX_DISABLE "cpld_sfp_tx_disable"
#define SYSFS_QSFP_RESET     "cpld_qsfp_reset"
#define SYSFS_QSFP_LPMODE    "cpld_qsfp_lpmode"
#define SYSFS_QSFP_PRESENT   "cpld_qsfp_intr_present"
#define SYSFS_EEPROM         "eeprom"

#define VALIDATE_PORT(p) { if ((p < 0) || (p >= PORT_NUM)) return ONLP_STATUS_E_PARAM; }
#define VALIDATE_SFP_PORT(p) { if (!IS_SFP(p)) return ONLP_STATUS_E_PARAM; }

const char sysfs_attr_suffix[][8] = {"0_7", "8_15", "16_23", "24_31", "32_39", "40_47", "48_53"};

#define EEPROM_ADDR (0x50)
#define EEPROM_BASE_BUS (26)

#define MASK_1000_0000 0x80
#define MASK_0000_0010 0x02

//SFF8636 TX Disable
#define SFF8636_EEPROM_OFFSET_TXDIS    0x56
#define SFF8636_EEPROM_TX_DIS          0x0f  /* txdis valid bit(bit0-bit3), xxxx 1111 */
#define SFF8636_EEPROM_TX_EN           0x0

//CMIS TX Disable
#define CMIS_PAGE_SIZE                        (128)
#define CMIS_PAGE_SUPPORTED_CTRL_ADV          (1)
#define CMIS_PAGE_TX_DIS                      (16)
#define CMIS_OFFSET_ADMIN_INFO                (0)
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

static int ufi_port_to_cpld_addr(int logical_port)
{
    return CPLD_BASE_ADDR[1];
}

static int ufi_port_to_sysfs_attr_offset(int logical_port)
{
    return logical_port/8;
}

static int ufi_port_to_bit_offset(int logical_port)
{
    return logical_port % 8;
}

static int ufi_port_to_eeprom_bus(int logical_port)
{
    return EEPROM_BASE_BUS + logical_port;
}

static int ufi_port_to_cpld_bus(int logical_port)
{
    int bus = -1;

    if (logical_port < PORT_NUM) {
        bus =  CPLD_I2C_BUS;
    } else { //unknown ports
        AIM_LOG_ERROR("unknown ports, logical_port=%d\n", logical_port);
        check_and_do_i2c_mux_reset(logical_port);
        return ONLP_STATUS_E_PARAM;
    }

    return bus;
}

static int ufi_port_present_get(int logical_port, int *pres_val)
{
    int reg_val = 0, rc = 0;
    int cpld_bus = 0, cpld_addr = 0, attr_offset = 0;
    char *sysfs_port_present = NULL;

    //get cpld bus, cpld addr and sysfs_attr_offset
    cpld_bus = ufi_port_to_cpld_bus(logical_port);
    cpld_addr = ufi_port_to_cpld_addr(logical_port);
    attr_offset = ufi_port_to_sysfs_attr_offset(logical_port);

    if (IS_SFP(logical_port)) {
        sysfs_port_present = SYSFS_SFP_PRESENT;
    }	else if (IS_QSFPX(logical_port)) {
        sysfs_port_present = SYSFS_QSFP_PRESENT;
    } else {
        return ONLP_STATUS_E_PARAM;
    }

    //read register
    if ((rc = file_read_hex(&reg_val, SYS_FMT_OFFSET, cpld_bus, cpld_addr, sysfs_port_present, sysfs_attr_suffix[attr_offset])) < 0) {
        AIM_LOG_ERROR("Unable to read sysfs %s", sysfs_port_present);
        AIM_LOG_ERROR(SYS_FMT_OFFSET, cpld_bus, cpld_addr, sysfs_port_present, sysfs_attr_suffix[attr_offset]);
        check_and_do_i2c_mux_reset(logical_port);
        return rc;
    }

    *pres_val = !((reg_val >> ufi_port_to_bit_offset(logical_port)) & 0x1);

    return ONLP_STATUS_OK;
}

static int base_num_g = -1;

static int update_port_base(void) {
    int rv  = ONLP_STATUS_OK;
    if(base_num_g == -1) {
        rv = ufi_port_base_get(&base_num_g);
    }
    return rv;
}

static int xfr_label_logical_port(int label_port, int *logical_port) {
    *logical_port = label_port - base_num_g;
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
    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));
    bus = ufi_port_to_eeprom_bus(logical_port);

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
    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));

    bus = ufi_port_to_eeprom_bus(logical_port);

    // set dev_class
    ONLP_TRY(onlp_file_write_int(dev_class, SYS_FMT, bus, EEPROM_ADDR, SYSFS_DEV_CLASS));

    return ONLP_STATUS_OK;
}

/**
 * @brief Update device class for QSFP-related ports
 *
 * This function updates the device class for a given QSFP-related port.
 * It reads the current device class and module type, then checks against a dev type list
 * to determine the correct device class.
 * If the device class needs to be updated, it writes the new value to dev_class.
 *
 * @param port The port number
 * @return An error condition or current port dev_class.
 */
int onlp_sfpi_dev_class_update_port(int port)
{
    int dev_class = 0, type = 0, i = 0;
    int logical_port = 0;

    ONLP_TRY( xfr_label_logical_port(port, &logical_port));

    if (!IS_QSFPX(logical_port) || !onlp_sfpi_is_present(logical_port)) { // not QSFPX or module absent
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
 * @brief Update device class for QSFP-related ports
 *
 * This function updates the device class for a given QSFP-related port.
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
    int start_port = 0 + base_num_g;
    int end_port = start_port + PORT_NUM;

    for (int i = start_port; i < end_port; ++i) {
        int logical_port = 0;
        if(xfr_label_logical_port(i, &logical_port) != ONLP_STATUS_OK) {
            continue;
        }

        if(IS_QSFPX(logical_port)) {
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
    int bus = 0;
    int seek = 0;
    int length = 0;
    int tx_dis_adv = 0;
    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));

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

    bus = ufi_port_to_eeprom_bus(logical_port);
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
static int ufi_cmis_txdisable_status_get(int port, int* status)
{
    int ret = 0;
    uint8_t value = 0;
    char sysfs_path[256] = {0};
    int bus = 0;
    int length = 0;
    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));

    //Check module present
    if (onlp_sfpi_is_present(port) != 1) {
        return ONLP_STATUS_OK;
    }

    // tx disable support check
    if ((ret=ufi_cmis_txdisable_supported(port)) != ONLP_STATUS_OK) {
        return ret;
    }

    bus = ufi_port_to_eeprom_bus(logical_port);
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
    int bus = 0;
    int seek = CMIS_SEEK_TX_DIS;
    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));

    //Check module present
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

    // set sysfs_path
    bus = ufi_port_to_eeprom_bus(logical_port);

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
    lock_init();
    update_port_base();
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the bitmap of SFP-capable port numbers.
 * @param bmap [out] Receives the bitmap.
 */
int onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{
	ONLP_TRY(update_port_base());
    int p = 0;
    int start_port = 0 + base_num_g;
    int end_port = start_port + PORT_NUM;

    AIM_BITMAP_CLR_ALL(bmap);
    for(p = start_port; p < end_port; p++) {
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
    ONLP_TRY(update_port_base());
    int status = ONLP_STATUS_OK;

    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));

    VALIDATE_PORT(logical_port);
    ONLP_TRY(ufi_port_present_get(logical_port, &status));

    return status;
}

/**
 * @brief Return the presence bitmap for all SFP ports.
 * @param dst Receives the presence bitmap.
 */
int onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    ONLP_TRY(update_port_base());
    int p = 0;
    int ret = 0;

    int start_port = 0 + base_num_g;
    int end_port = start_port + PORT_NUM;

    AIM_BITMAP_CLR_ALL(dst);
    for (p = start_port; p < end_port; p++) {
        ret = onlp_sfpi_is_present(p);
        AIM_BITMAP_MOD(dst, p, ret);
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Return the RX_LOS bitmap for all SFP ports.
 * @param dst Receives the RX_LOS bitmap.
 */
int onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    ONLP_TRY(update_port_base());
    int i = 0, value = 0;

    int start_port = 0 + base_num_g;
    int end_port = start_port + PORT_NUM;

    for(i = start_port; i < end_port; i++) {
        int logical_port = 0;
        if(xfr_label_logical_port(i, &logical_port) != ONLP_STATUS_OK) {
            continue;
        }

        if (IS_SFP(logical_port)) {
            ONLP_TRY(onlp_sfpi_control_get(i, ONLP_SFP_CONTROL_RX_LOS, &value));
            AIM_BITMAP_MOD(dst, i, value);
        } else {
            AIM_BITMAP_MOD(dst, i, 0);
        }
    }

    return ONLP_STATUS_OK;
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
    ONLP_TRY(update_port_base());
    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));

    int bus = -1;
    VALIDATE_PORT(logical_port);

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent. \n", port);
        return ONLP_STATUS_OK;
    }

    bus = ufi_port_to_eeprom_bus(logical_port);

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
    ONLP_TRY(update_port_base());
    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));

    VALIDATE_PORT(logical_port);

    int rc = 0;
    int bus = -1;

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("port module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    bus = ufi_port_to_eeprom_bus(logical_port);
    if ((rc=onlp_i2c_write(bus, devaddr, addr, size, data, ONLP_I2C_F_FORCE)) < 0) {
        check_and_do_i2c_mux_reset(port);
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
    ONLP_TRY(update_port_base());
    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));

    VALIDATE_PORT(logical_port);

    int rc = 0;
    int bus = -1;

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    bus = ufi_port_to_eeprom_bus(logical_port);
    if ((rc=onlp_i2c_readb(bus, devaddr, addr, ONLP_I2C_F_FORCE)) < 0) {
        check_and_do_i2c_mux_reset(port);
    }

    return rc;
}

/**
 * @brief Write a byte to an address on the given SFP port's bus.
 */
int onlp_sfpi_dev_writeb(int port, uint8_t devaddr, uint8_t addr, uint8_t value)
{
    ONLP_TRY(update_port_base());
    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));

    VALIDATE_PORT(logical_port);

    int rc = 0;
    int bus = -1;

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    bus = ufi_port_to_eeprom_bus(logical_port);
    if ((rc=onlp_i2c_writeb(bus, devaddr, addr, value, ONLP_I2C_F_FORCE)) < 0) {
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
    ONLP_TRY(update_port_base());
    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));

    VALIDATE_PORT(logical_port);

    int rc = 0;
    int bus = -1;

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    bus = ufi_port_to_eeprom_bus(logical_port);
    if ((rc=onlp_i2c_readw(bus, devaddr, addr, ONLP_I2C_F_FORCE)) < 0) {
        check_and_do_i2c_mux_reset(port);
    }

    return rc;
}

/**
 * @brief Write a word to an address on the given SFP port's bus.
 */
int onlp_sfpi_dev_writew(int port, uint8_t devaddr, uint8_t addr, uint16_t value)
{
    ONLP_TRY(update_port_base());
    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));

    VALIDATE_PORT(logical_port);

    int rc = 0;
    int bus = -1;

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    bus = ufi_port_to_eeprom_bus(logical_port);
    if ((rc=onlp_i2c_writew(bus, devaddr, addr, value, ONLP_I2C_F_FORCE)) < 0) {
        check_and_do_i2c_mux_reset(port);
    }

    return rc;
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
    ONLP_TRY(update_port_base());
    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));
    VALIDATE_PORT(logical_port);

    //set unsupported as default value
    *rv = 0;

    switch (control) {
        case ONLP_SFP_CONTROL_RESET:
        case ONLP_SFP_CONTROL_RESET_STATE:
        case ONLP_SFP_CONTROL_LP_MODE:
            if (IS_QSFPX(logical_port)) {
                *rv = 1;
            }
            break;
        case ONLP_SFP_CONTROL_RX_LOS:
        case ONLP_SFP_CONTROL_TX_FAULT:
            if (IS_SFP(logical_port)) {
                *rv = 1;
            }
            break;
        case ONLP_SFP_CONTROL_TX_DISABLE:
        case ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL:
            if (IS_SFP(logical_port) || IS_QSFPX(logical_port)) {
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
    ONLP_TRY(update_port_base());
    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));
    VALIDATE_PORT(logical_port);

    int rc = 0;
    int reg_val = 0;
    int cpld_bus = 0;
    int cpld_addr = 0;
    int attr_offset = 0, bit_offset = 0;
    char *sysfs_attr = NULL;
    int dev_class = 0;

    cpld_bus = ufi_port_to_cpld_bus(logical_port);
    cpld_addr = ufi_port_to_cpld_addr(logical_port);
    attr_offset = ufi_port_to_sysfs_attr_offset(logical_port);

    switch(control)
        {
        case ONLP_SFP_CONTROL_RESET:
            {
                if (IS_QSFPX(logical_port)) {
                    //config sysfs_attr
                    sysfs_attr = SYSFS_QSFP_RESET;

                    //read reg_val
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, cpld_bus, cpld_addr, sysfs_attr, sysfs_attr_suffix[attr_offset]) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //update reg_val
                    //0 is normal, 1 is reset, reverse value to fit our platform
                    value = !value;
                    bit_offset = ufi_port_to_bit_offset(logical_port);
                    reg_val = ufi_bit_operation(reg_val, bit_offset, value);

                    //write reg_val
                    if ((rc=onlp_file_write_int(reg_val, SYS_FMT_OFFSET, cpld_bus, cpld_addr, sysfs_attr, sysfs_attr_suffix[attr_offset])) < 0) {
                        AIM_LOG_ERROR("Unable to write %s, error=%d, reg_val=%x", sysfs_attr,  rc, reg_val);
                        AIM_LOG_ERROR(SYS_FMT_OFFSET, cpld_bus, cpld_addr, sysfs_attr, sysfs_attr_suffix[attr_offset]);
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }
                    rc = ONLP_STATUS_OK;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_TX_DISABLE:
        case ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL:
            {
                if (IS_SFP(logical_port)) {
                    //config sysfs_attr
                    sysfs_attr = SYSFS_SFP_TX_DISABLE;

                    //read reg_val
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, cpld_bus, cpld_addr, sysfs_attr, sysfs_attr_suffix[attr_offset]) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //update reg_val
                    //0 is normal, 1 is tx_disable
                    bit_offset = ufi_port_to_bit_offset(logical_port);
                    reg_val = ufi_bit_operation(reg_val, bit_offset, value);

                    //write reg_val
                    if ((rc=onlp_file_write_int(reg_val, SYS_FMT_OFFSET, cpld_bus, cpld_addr, sysfs_attr, sysfs_attr_suffix[attr_offset])) < 0) {
                        AIM_LOG_ERROR("Unable to write %s, error=%d, reg_val=%x", sysfs_attr,  rc, reg_val);
                        AIM_LOG_ERROR(SYS_FMT_OFFSET, cpld_bus, cpld_addr, sysfs_attr, sysfs_attr_suffix[attr_offset]);
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }
                    rc = ONLP_STATUS_OK;
                } else if (IS_QSFPX(logical_port)) {
                    int eeprom_bus = 0;
                    eeprom_bus = ufi_port_to_eeprom_bus(logical_port);
                    rc = onlp_file_read_int(&dev_class, SYS_FMT, eeprom_bus, EEPROM_ADDR, SYSFS_DEV_CLASS);
                    if(rc < 0) {
                        AIM_LOG_ERROR("Unable to read "SYS_FMT", error=%d", eeprom_bus, EEPROM_ADDR, SYSFS_DEV_CLASS,  rc);
                        return ONLP_STATUS_E_INTERNAL;
                    }
                    if (dev_class <= 0) {
                        rc = dev_class; //return error condition.
                    } else if (dev_class == 1) { //SFF8636 module
                        ONLP_TRY(rc = ufi_sff8636_txdisable_status_set(port, value));
                    } else if (dev_class == 3) { //CMIS module
                        ONLP_TRY(rc = ufi_cmis_txdisable_status_set(port, value));
                    }
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                if (IS_QSFPX(logical_port)) {
                    //config sysfs_attr
                    sysfs_attr = SYSFS_QSFP_LPMODE;

                    //read reg_val
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, cpld_bus, cpld_addr, sysfs_attr, sysfs_attr_suffix[attr_offset]) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //update reg_val
                    //0 is normal, 1 is low power mode
                    bit_offset = ufi_port_to_bit_offset(logical_port);
                    reg_val = ufi_bit_operation(reg_val, bit_offset, value);

                    //write reg_val
                    if ((rc=onlp_file_write_int(reg_val, SYS_FMT_OFFSET, cpld_bus, cpld_addr, sysfs_attr, sysfs_attr_suffix[attr_offset])) < 0) {
                        AIM_LOG_ERROR("Unable to write %s, error=%d, reg_val=%x", sysfs_attr,  rc, reg_val);
                        AIM_LOG_ERROR(SYS_FMT_OFFSET, cpld_bus, cpld_addr, sysfs_attr, sysfs_attr_suffix[attr_offset]);
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }
                    rc = ONLP_STATUS_OK;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
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
    ONLP_TRY(update_port_base());
    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));
    VALIDATE_PORT(logical_port);

    int rc = 0;
    int reg_val = 0, reg_mask = 0;
    int cpld_bus = 0;
    int cpld_addr = 0;
    int attr_offset = 0, bit_offset = 0;
    int negate_value = 0;
    char *sysfs_attr = NULL;
    int dev_class = 0;

    cpld_bus = ufi_port_to_cpld_bus(logical_port);
    cpld_addr = ufi_port_to_cpld_addr(logical_port);
    attr_offset = ufi_port_to_sysfs_attr_offset(logical_port);

    switch(control)
        {
        case ONLP_SFP_CONTROL_RESET_STATE:
            {
                if (IS_QSFPX(logical_port)) {
                    //config sysfs_attr
                    sysfs_attr = SYSFS_QSFP_RESET;

                    //read reg_val
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, cpld_bus, cpld_addr, sysfs_attr, sysfs_attr_suffix[attr_offset]) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //read bit value
                    bit_offset = ufi_port_to_bit_offset(logical_port);
                    reg_mask = 1 << bit_offset;
                    *value = ufi_mask_shift(reg_val, reg_mask);

                    //0 is normal, 1 is reset, reverse value to fit our platform
                    negate_value = 1;
                    rc = ONLP_STATUS_OK;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_RX_LOS:
            {
                if (IS_SFP(logical_port)) {
                    //config sysfs_attr
                    sysfs_attr = SYSFS_SFP_RX_LOS;

                    //read reg_val
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, cpld_bus, cpld_addr, sysfs_attr, sysfs_attr_suffix[attr_offset]) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //read bit value
                    //0 is normal, 1 is rx_los
                    bit_offset = ufi_port_to_bit_offset(logical_port);
                    reg_mask = 1 << bit_offset;
                    *value = ufi_mask_shift(reg_val, reg_mask);

                    negate_value = 0;
                    rc = ONLP_STATUS_OK;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_TX_FAULT:
            {
                if (IS_SFP(logical_port)) {
                    //config sysfs_attr
                    sysfs_attr = SYSFS_SFP_TX_FAULT;

                    //read reg_val
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, cpld_bus, cpld_addr, sysfs_attr, sysfs_attr_suffix[attr_offset]) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //read bit value
                    //0 is normal, 1 is tx_fault
                    bit_offset = ufi_port_to_bit_offset(logical_port);
                    reg_mask = 1 << bit_offset;
                    *value = ufi_mask_shift(reg_val, reg_mask);

                    negate_value = 0;
                    rc = ONLP_STATUS_OK;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_TX_DISABLE:
        case ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL:
            {
                if (IS_SFP(logical_port)) {
                    //config sysfs_attr
                    sysfs_attr = SYSFS_SFP_TX_DISABLE;

                    //read reg_val
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, cpld_bus, cpld_addr, sysfs_attr, sysfs_attr_suffix[attr_offset]) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //read bit value
                    //0 is normal, 1 is tx_disable
                    bit_offset = ufi_port_to_bit_offset(logical_port);
                    reg_mask = 1 << bit_offset;
                    *value = ufi_mask_shift(reg_val, reg_mask);

                    negate_value = 0;
                    rc = ONLP_STATUS_OK;
                } else if (IS_QSFPX(logical_port)) {
                    int eeprom_bus = 0;
                    eeprom_bus = ufi_port_to_eeprom_bus(logical_port);
                    rc = onlp_file_read_int(&dev_class, SYS_FMT, eeprom_bus, EEPROM_ADDR, SYSFS_DEV_CLASS);
                    if(rc < 0) {
                        AIM_LOG_ERROR("Unable to read \"SYS_FMT\", error=%d", eeprom_bus, EEPROM_ADDR, SYSFS_DEV_CLASS,  rc);
                        return ONLP_STATUS_E_INTERNAL;
                    }
                    if (dev_class <= 0) {
                        rc = dev_class; //return error condition.
                    } else if (dev_class == 1) { //SFF8636 module
                        ONLP_TRY(rc = ufi_sff8636_txdisable_status_get(port, value));
                    } else if (dev_class == 3) { //CMIS module
                        rc = ufi_cmis_txdisable_status_get(port, value);
                    }
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                if (IS_QSFPX(logical_port)) {
                    //config sysfs_attr
                    sysfs_attr = SYSFS_QSFP_LPMODE;

                    //read reg_val
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, cpld_bus, cpld_addr, sysfs_attr, sysfs_attr_suffix[attr_offset]) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //read bit value
                    //0 is normal, 1 is low power
                    bit_offset = ufi_port_to_bit_offset(logical_port);
                    reg_mask = 1 << bit_offset;
                    *value = ufi_mask_shift(reg_val, reg_mask);

                    negate_value = 0;
                    rc = ONLP_STATUS_OK;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
        }

    //negate value if needed
    if (negate_value == 1) {
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

/**
 * @brief Read the SFP EEPROM.
 * @param port The port number.
 * @param data Receives the SFP data.
 */
int onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{
    ONLP_TRY(update_port_base());
    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));
    VALIDATE_PORT(logical_port);

    int size = 0, expect_size = 256, bus = 0, rc = 0;

    memset(data, 0, expect_size);

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent. \n", port);
        return ONLP_STATUS_OK;
    }

    bus = ufi_port_to_eeprom_bus(logical_port);

    if((rc = onlp_file_read(data, expect_size, &size, SYS_FMT, bus, EEPROM_ADDR, SYSFS_EEPROM)) < 0) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d)", port);
        AIM_LOG_ERROR(SYS_FMT, bus, EEPROM_ADDR, SYSFS_EEPROM);

        check_and_do_i2c_mux_reset(port);
        return rc;
    }

    if (size != expect_size) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d), size is different!", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Read the SFP DOM EEPROM.
 * @param port The port number.
 * @param data Receives the SFP data.
 */
int onlp_sfpi_dom_read(int port, uint8_t data[256])
{
    ONLP_TRY(update_port_base());
    int logical_port = 0;
    ONLP_TRY(xfr_label_logical_port(port, &logical_port));
    VALIDATE_SFP_PORT(logical_port);

    char eeprom_path[512];
    FILE* fp = NULL;
    int bus = 0;

    //sfp dom is on 0x51 (2nd 256 bytes)
    //qsfp dom is on lower page 0x00
    //qsfpdd 2.0 dom is on lower page 0x00
    //qsfpdd 3.0 and later dom and above is on lower page 0x00 and higher page 0x17

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    memset(data, 0, 256);
    memset(eeprom_path, 0, sizeof(eeprom_path));

    //set eeprom_path
    bus = ufi_port_to_eeprom_bus(logical_port);
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
