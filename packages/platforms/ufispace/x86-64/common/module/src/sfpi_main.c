/************************************************************
 * Copyright (C) 2026 Ufispace Technology Corporation.
 ************************************************************
 *
 * SFP Platform Implementation Interface.
 *
 ***********************************************************/
#include <unistd.h>
#include <fcntl.h>
#include <onlp/platformi/sfpi.h>
#include <ufispace_platform/platform_lib.h>
#include <ufispace_common/platform_lib_main.h>
#include <ufispace_common/sfpi_main.h>

static int port_count = 0;
static int port_base = 0;
static port_elems *ports = NULL;

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

int _onlp_port_count_base_get(int *count, int *base);
int _onlp_port_entry_get(int logical_id, port_elems *entry);

/**
  * @brief Get Port components total count.
  */
int _onlp_port_total_get(int *total)
{
    int rv = ONLP_STATUS_OK;

    if(!total) {
        return ONLP_STATUS_E_PARAM;
    }

    rv = ufi_file_read_int(total, "/sys_switch/slot/slot1/num_components/num_transceivers");
    if(rv != ONLP_STATUS_OK || *total < 0) {
        *total = 0;
    }
    return rv;
}

/**
  * @brief Get Port base.
  */
int _onlp_port_base_get(int *total)
{
    int rv = ONLP_STATUS_OK;

    rv = ufi_file_read_int(total, "/sys_switch/transceiver/port_base");
    if(rv != ONLP_STATUS_OK || total < 0) {
        *total = 0;
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
static int ufi_sff8636_txdisable_status_get(int port, int *status, onlp_sfp_control_t control)
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
    uint32_t logical_id = 0;
    port_elems entry = {0};
    int count = 0;
    int base = 0;
    uint8_t value = 0;
    char sysfs_path[256] = {0};
    int cmis_ver = 0;
    int mem_model = 0;
    int seek = 0;
    int length = 0;
    int tx_dis_adv = 0;

    ONLP_TRY(onlp_sfpi_init());
    ONLP_TRY(_onlp_port_count_base_get(&count, &base));

    logical_id = port - base;
    ONLP_TRY(_onlp_port_entry_get(logical_id, &entry));

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
    seek = CMIS_SEEK_TX_DIS_ADV;

    // create and check sysfs_path
    length = snprintf(sysfs_path, sizeof(sysfs_path),
        "/sys_switch/transceiver/eth%d/eeprom", entry.parent);

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
    uint32_t logical_id = 0;
    port_elems entry = {0};
    int count = 0;
    int base = 0;
    int rv = ONLP_STATUS_OK;
    uint8_t value = 0;
    char sysfs_path[256] = {0};
    int length = 0;

    if (control != ONLP_SFP_CONTROL_TX_DISABLE &&
        control != ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL) {
        AIM_LOG_ERROR("[%s] invalid control, port=%d, control=%d\n", __FUNCTION__, port, control);
        return ONLP_STATUS_E_PARAM;
    }

    ONLP_TRY(onlp_sfpi_init());
    ONLP_TRY(_onlp_port_count_base_get(&count, &base));

    logical_id = port - base;
    ONLP_TRY(_onlp_port_entry_get(logical_id, &entry));

    // Check module present
    if (onlp_sfpi_is_present(port) != 1) {
        return ONLP_STATUS_OK;
    }

    // tx disable support check
    if ((rv=ufi_cmis_txdisable_supported(port)) != ONLP_STATUS_OK) {
        return rv;
    }

    length = snprintf(sysfs_path, sizeof(sysfs_path),
        "/sys_switch/transceiver/eth%d/eeprom", entry.parent);

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
    uint32_t logical_id = 0;
    port_elems entry = {0};
    int count = 0;
    int base = 0;
    uint8_t value = 0, readback = 0;
    char sysfs_path[256] = {0};
    int seek = CMIS_SEEK_TX_DIS;

    if (control != ONLP_SFP_CONTROL_TX_DISABLE &&
        control != ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL) {
        AIM_LOG_ERROR("[%s] invalid control, port=%d, control=%d\n", __FUNCTION__, port, control);
        return ONLP_STATUS_E_PARAM;
    }

    ONLP_TRY(onlp_sfpi_init());
    ONLP_TRY(_onlp_port_count_base_get(&count, &base));

    logical_id = port - base;
    ONLP_TRY(_onlp_port_entry_get(logical_id, &entry));

    // Check module present
    if (onlp_sfpi_is_present(port) != 1) {
        return ONLP_STATUS_OK;
    }

    // tx disable support check
    if (ufi_cmis_txdisable_supported(port) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    // set value
    if (control == ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL) {
        if (status < 0 || status > TX_DIS_INPUT_MAX) {
            AIM_LOG_ERROR("[%s] unaccepted status, port=%d, status=%d\n", __FUNCTION__, port, status);
            return ONLP_STATUS_E_PARAM;
        } else {
            value = (uint8_t)(status);
        }
    } else {
        if (status == 0) {
            value = CMIS_VAL_TX_EN;
        } else if (status == 1) {
            value = CMIS_VAL_TX_DIS;
        } else {
            AIM_LOG_ERROR("[%s] unaccepted status, port=%d, status=%d\n", __FUNCTION__, port, status);
            return ONLP_STATUS_E_PARAM;
        }
    }

    // check snprintf
    int length = snprintf(sysfs_path, sizeof(sysfs_path),
        "/sys_switch/transceiver/eth%d/eeprom", entry.parent);

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
  * @brief Get the port count and port base information.
  */
int __WEAK _onlp_port_count_base_get(int *count, int *base)
{
    if(!count || !base) {
        return ONLP_STATUS_E_PARAM;
    } else {
        *count = port_count;
        *base = port_base;
        return ONLP_STATUS_OK;
    }
}

/**
  * @brief Get the port entry.
  */
int __WEAK _onlp_port_entry_get(int logical_id, port_elems *entry)
{
    int count = 0;
    int base = 0;
    ONLP_TRY(_onlp_port_count_base_get(&count, &base));
    if(!entry ||  logical_id < base || logical_id >= (base + count)) {
        return ONLP_STATUS_E_PARAM;
    } else if(ports[logical_id].valid != 1) {
        return ONLP_STATUS_E_INVALID;
    } else {
        *entry = ports[logical_id];
        return ONLP_STATUS_OK;
    }
}

/**
  * @brief check the port is valid or not.
  */
int __WEAK _onlp_port_valid_check(int logical_id)
{
    int rv = ONLP_STATUS_OK;
    port_elems entry = {0};
    rv = _onlp_port_entry_get(logical_id, &entry);
    if(rv != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INVALID;
    } else {
        return ONLP_STATUS_OK;
    }
}

/**
  * @brief check the port type.
  */
int __WEAK _onlp_port_type_check(int logical_id, port_type_t port_type)
{
    port_elems entry = {0};
    if(_onlp_port_entry_get(logical_id, &entry))
        return ONLP_STATUS_E_INVALID;

    if(entry.type == port_type) {
        return ONLP_STATUS_OK ;
    } else {
        return ONLP_STATUS_E_INVALID;
    }
}

/**
 * @brief Get device class for a port
 *
 * This function get the device class for a given port.
 *
 * @param port The port number
 * @return An error condition or ONLP_STATUS_OK.
 */
int __WEAK onlp_sfpi_dev_class_get(int port, int *dev_class)
{
    uint32_t logical_id = 0;
    port_elems entry = {0};
    int count = 0;
    int base = 0;
    int rv = ONLP_STATUS_OK;

    ONLP_TRY(onlp_sfpi_init());
    ONLP_TRY(_onlp_port_count_base_get(&count, &base));

    logical_id = port - base;
    ONLP_TRY(_onlp_port_entry_get(logical_id, &entry));

    //read dev_class
    rv = ufi_file_read_int(dev_class,
        "/sys_switch/transceiver/eth%d/dev_class", entry.parent);
    if(rv < 0) {
        AIM_LOG_ERROR("Unable to read /sys_switch/transceiver/eth%d/dev_class,"
            " error=%d", entry.parent, rv);
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
int __WEAK onlp_sfpi_dev_class_set(int port, int dev_class)
{
    uint32_t logical_id = 0;
    port_elems entry = {0};
    int count = 0;
    int base = 0;
    ONLP_TRY(onlp_sfpi_init());
    ONLP_TRY(_onlp_port_count_base_get(&count, &base));

    logical_id = port - base;
    ONLP_TRY(_onlp_port_entry_get(logical_id, &entry));

    // set dev_class
    ONLP_TRY(onlp_file_write_int(dev_class,
        "/sys_switch/transceiver/eth%d/dev_class", entry.parent));

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
int __WEAK onlp_sfpi_dev_class_update_port(int port)
{
    uint32_t logical_id = 0;
    int count = 0;
    int base = 0;
    int dev_class, type, i;

    ONLP_TRY(onlp_sfpi_init());
    ONLP_TRY(_onlp_port_count_base_get(&count, &base));

    logical_id = port - base;
    VALIDATE_PORT(logical_id);
    if (!IS_QSFPX(logical_id) || !onlp_sfpi_is_present(logical_id)) {
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
int __WEAK onlp_sfpi_dev_class_update(int port)
{
    int rv = ONLP_STATUS_OK;
    int count = 0;
    int base = 0;

    ONLP_TRY(onlp_sfpi_init());
    ONLP_TRY(_onlp_port_count_base_get(&count, &base));

    // single port update
    if (port != ALL_PORTS) {
        return onlp_sfpi_dev_class_update_port(port);
    }

    // update all QSFPX ports
    for(int i = base; i < (base+count); ++i) {
        uint32_t logical_id = i - base;
        if(!IS_QSFPX(logical_id)) {
            continue;
        }

        if (onlp_sfpi_dev_class_update_port(i) < 0) {
            rv = ONLP_STATUS_E_INTERNAL;
        }
    }

    return rv;
}

/**
 * @brief Initialize the SFPI subsystem.
 */
int __WEAK onlp_sfpi_init(void)
{
    int i = 1;

    if (ports != NULL) {
        return ONLP_STATUS_OK;
    }

    lock_init();

    ONLP_TRY(_onlp_port_total_get(&port_count));
    ONLP_TRY(_onlp_port_base_get(&port_base));

    if(port_count > 0) {
        int total_size = port_count + port_base;

        ports = (port_elems *) aim_zmalloc(sizeof(port_elems)*total_size);
        if (ports == NULL) {
            AIM_LOG_ERROR("Failed to allocate memory for SFP ports");
            return ONLP_STATUS_E_INTERNAL;
        }
    }

    if(port_base >= 0 && ports != NULL) {
        for(i = 0; i < port_count; i++) {
            int eth_index = i+1;
            char *type = NULL;
            int len1 = onlp_file_read_str(&type, "/sys_switch/transceiver/eth%d/type", eth_index);
            int index = port_base + i;
            ports[index].parent = eth_index;

            if(!type || !len1) {
                aim_free(type);
                ports[index].valid = 0;
                continue;
            }

            if(!strncmp(type, COMM_STR_QSFPDD, sizeof(COMM_STR_QSFPDD))) {
                ports[index].type = TYPE_QSFPDD;
                ports[index].valid = 1;
            } else if(!strncmp(type, COMM_STR_OSFP, sizeof(COMM_STR_OSFP))) {
                ports[index].type = TYPE_OSFP;
                ports[index].valid = 1;
            } else if(!strncmp(type, COMM_STR_QSFP, sizeof(COMM_STR_QSFP))) {
                ports[index].type = TYPE_QSFP;
                ports[index].valid = 1;
            } else if(!strncmp(type, COMM_STR_SFP, sizeof(COMM_STR_SFP))) {
                ports[index].type = TYPE_SFP;
                ports[index].valid = 1;
            } else {
                ports[index].valid = 0;
            }

            aim_free(type);
        }
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the bitmap of SFP-capable port numbers.
 * @param bmap [out] Receives the bitmap.
 */
int __WEAK onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{
    int p;
    int count = 0;
    int base = 0;

    ONLP_TRY(onlp_sfpi_init());
    ONLP_TRY(_onlp_port_count_base_get(&count, &base));

    for(p = base; p < (base+count); p++) {
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
int __WEAK onlp_sfpi_is_present(int port)
{
    uint32_t logical_id = 0;
    port_elems entry = {0};
    int count = 0;
    int base = 0;
    int rv = ONLP_STATUS_OK;
    int presence = 0;

    ONLP_TRY(onlp_sfpi_init());
    ONLP_TRY(_onlp_port_count_base_get(&count, &base));

    logical_id = port - base;
    ONLP_TRY(_onlp_port_entry_get(logical_id, &entry));

    rv = ufi_file_read_int(&presence, "/sys_switch/transceiver/eth%d/present",
            entry.parent);

    if(rv != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("onlp_sfpi_is_present() failed, error=%d", rv);
        check_and_do_i2c_mux_reset(port);
        return rv;
    }

    return presence;
}

/**
 * @brief Return the presence bitmap for all SFP ports.
 * @param dst Receives the presence bitmap.
 */
int __WEAK onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int p = 0;
    int rv = 0;
    int count = 0;
    int base = 0;

    ONLP_TRY(onlp_sfpi_init());
    ONLP_TRY(_onlp_port_count_base_get(&count, &base));

    for (p = base; p < (base + count); p++) {
        if ((rv = onlp_sfpi_is_present(p)) < 0) {
            return rv;
        }
        AIM_BITMAP_MOD(dst, p, rv);
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Return the RX_LOS bitmap for all SFP ports.
 * @param dst Receives the RX_LOS bitmap.
 */
int __WEAK onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int i=0, value=0;
    int count = 0;
    int base = 0;

    ONLP_TRY(onlp_sfpi_init());
    ONLP_TRY(_onlp_port_count_base_get(&count, &base));

    for(i = base; i < (base + count); i++) {
        int logical_id = i-base;

        if(IS_PORT_INVALID(logical_id)) {
            continue;
        }

        if(IS_SFP(logical_id)) {
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
int __WEAK onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{
    uint32_t logical_id = 0;
    port_elems entry = {0};
    int count = 0;
    int base = 0;
    int size = 0, rv = 0;
    char sysfs_path[256] = {0};

    ONLP_TRY(onlp_sfpi_init());
    ONLP_TRY(_onlp_port_count_base_get(&count, &base));

    logical_id = port - base;
    ONLP_TRY(_onlp_port_entry_get(logical_id, &entry));

    // create and check sysfs_path
    size = snprintf(sysfs_path, sizeof(sysfs_path),
        "/sys_switch/transceiver/eth%d/eeprom", entry.parent);

    if (size < 0 || (size_t)size >= sizeof(sysfs_path)) {
        return ONLP_STATUS_E_INTERNAL;
    }

    // reset page select to 0
    ufi_reset_page_select(sysfs_path);

    memset(data, 0, 256);

    if((rv = onlp_file_read(data, 256, &size, sysfs_path) < 0)) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d), sysfs_path=%s", port, sysfs_path);
        check_and_do_i2c_mux_reset(port);
        return rv;
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
int __WEAK onlp_sfpi_dev_readb(int port, uint8_t devaddr, uint8_t addr)
{

    uint32_t logical_id = 0;
    port_elems entry = {0};
    int count = 0;
    int base = 0;
    int rv = ONLP_STATUS_OK;
    int bus = 0;

    ONLP_TRY(onlp_sfpi_init());
    ONLP_TRY(_onlp_port_count_base_get(&count, &base));

    logical_id = port - base;
    ONLP_TRY(_onlp_port_entry_get(logical_id, &entry));

    ONLP_TRY(ufi_file_read_int(&bus, "/sys_switch/transceiver/eth%d/bus",
        entry.parent));

    if (onlp_sfpi_is_present(port) !=  1) {
        ufi_bsp_info("sfp module (port=%d) is absent.", port);
        return ONLP_STATUS_OK;
    }

    if ((rv = onlp_i2c_readb(bus, devaddr, addr, ONLP_I2C_F_FORCE)) < 0) {
        check_and_do_i2c_mux_reset(port);
    }
    return rv;
}

/**
 * @brief Write a byte to an address on the given SFP port's bus.
 */
int __WEAK onlp_sfpi_dev_writeb(int port, uint8_t devaddr, uint8_t addr, uint8_t value)
{

    uint32_t logical_id = 0;
    port_elems entry = {0};
    int count = 0;
    int base = 0;
    int rv = ONLP_STATUS_OK;
    int bus = 0;

    ONLP_TRY(onlp_sfpi_init());
    ONLP_TRY(_onlp_port_count_base_get(&count, &base));

    logical_id = port - base;
    ONLP_TRY(_onlp_port_entry_get(logical_id, &entry));

    ONLP_TRY(ufi_file_read_int(&bus, "/sys_switch/transceiver/eth%d/bus",
        entry.parent));

    if (onlp_sfpi_is_present(port) !=  1) {
        ufi_bsp_info("sfp module (port=%d) is absent.", port);
        return ONLP_STATUS_OK;
    }

    if ((rv = onlp_i2c_writeb(bus, devaddr, addr, value, ONLP_I2C_F_FORCE)) < 0) {
         check_and_do_i2c_mux_reset(port);
    }
    return rv;
}

/**
 * @brief Read a word from an address on the given SFP port's bus.
 * @param port The port number.
 * @param devaddr The device address.
 * @param addr The address.
 * @returns The word if successful, error otherwise.
 */
int __WEAK onlp_sfpi_dev_readw(int port, uint8_t devaddr, uint8_t addr)
{

    uint32_t logical_id = 0;
    port_elems entry = {0};
    int count = 0;
    int base = 0;
    int rv = ONLP_STATUS_OK;
    int bus = 0;

    ONLP_TRY(onlp_sfpi_init());
    ONLP_TRY(_onlp_port_count_base_get(&count, &base));

    logical_id = port - base;
    ONLP_TRY(_onlp_port_entry_get(logical_id, &entry));

    ONLP_TRY(ufi_file_read_int(&bus, "/sys_switch/transceiver/eth%d/bus",
        entry.parent));

    if(onlp_sfpi_is_present(port) !=  1) {
        ufi_bsp_info("sfp module (port=%d) is absent.", port);
        return ONLP_STATUS_OK;
    }

    if((rv = onlp_i2c_readw(bus, devaddr, addr, ONLP_I2C_F_FORCE)) < 0) {
         check_and_do_i2c_mux_reset(port);
    }
    return rv;
}

/**
 * @brief Write a word to an address on the given SFP port's bus.
 */
int __WEAK onlp_sfpi_dev_writew(int port, uint8_t devaddr, uint8_t addr, uint16_t value)
{
    uint32_t logical_id = 0;
    port_elems entry = {0};
    int count = 0;
    int base = 0;
    int rv = ONLP_STATUS_OK;
    int bus = 0;

    ONLP_TRY(onlp_sfpi_init());
    ONLP_TRY(_onlp_port_count_base_get(&count, &base));

    logical_id = port - base;
    ONLP_TRY(_onlp_port_entry_get(logical_id, &entry));

    ONLP_TRY(ufi_file_read_int(&bus, "/sys_switch/transceiver/eth%d/bus",
        entry.parent));

    if(onlp_sfpi_is_present(port) !=  1) {
        ufi_bsp_info("sfp module (port=%d) is absent.", port);
        return ONLP_STATUS_OK;
    }

    if((rv = onlp_i2c_writew(bus, devaddr, addr, value, ONLP_I2C_F_FORCE)) < 0) {
        check_and_do_i2c_mux_reset(port);
    }
    return rv;
}

/**
 * @brief Read from an address on the given SFP port's bus.
 * @param port The port number.
 * @param devaddr The device address.
 * @param addr The address.
 * @returns The data if successful, error otherwise.
 */
int __WEAK onlp_sfpi_dev_read(int port, uint8_t devaddr, uint8_t addr, uint8_t* rdata, int size)
{
    uint32_t logical_id = 0;
    port_elems entry = {0};
    int count = 0;
    int base = 0;
    int bus = 0;

    ONLP_TRY(onlp_sfpi_init());
    ONLP_TRY(_onlp_port_count_base_get(&count, &base));

    logical_id = port - base;
    ONLP_TRY(_onlp_port_entry_get(logical_id, &entry));

    ONLP_TRY(ufi_file_read_int(&bus, "/sys_switch/transceiver/eth%d/bus",
        entry.parent));

    if (onlp_sfpi_is_present(port) != 1) {
        ufi_bsp_info("sfp module (port=%d) is absent.", port);
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
int __WEAK onlp_sfpi_dev_write(int port, uint8_t devaddr, uint8_t addr, uint8_t* data, int size)
{
    uint32_t logical_id = 0;
    port_elems entry = {0};
    int count = 0;
    int base = 0;
    int bus = 0;
    int rv = ONLP_STATUS_OK;

    ONLP_TRY(onlp_sfpi_init());
    ONLP_TRY(_onlp_port_count_base_get(&count, &base));

    logical_id = port - base;
    ONLP_TRY(_onlp_port_entry_get(logical_id, &entry));

    ONLP_TRY(ufi_file_read_int(&bus, "/sys_switch/transceiver/eth%d/bus",
        entry.parent));

    if (onlp_sfpi_is_present(port) !=  1) {
        ufi_bsp_info("sfp module (port=%d) is absent.", port);
        return ONLP_STATUS_OK;
    }

    if ((rv = onlp_i2c_write(bus, devaddr, addr, size, data, ONLP_I2C_F_FORCE)) < 0) {
        check_and_do_i2c_mux_reset(port);
    }

    return rv;
}


/**
 * @brief Read the SFP DOM EEPROM.
 * @param port The port number.
 * @param data Receives the SFP data.
 */
int __WEAK onlp_sfpi_dom_read(int port, uint8_t data[256])
{

    uint32_t logical_id = 0;
    port_elems entry = {0};
    int count = 0;
    int base = 0;
    char eeprom_path[512];
    FILE* fp;

    ONLP_TRY(onlp_sfpi_init());
    ONLP_TRY(_onlp_port_count_base_get(&count, &base));

    logical_id = port - base;

    //sfp dom is on 0x51 (2nd 256 bytes)
    //qsfp dom is on lower page 0x00
    //qsfpdd 2.0 dom is on lower page 0x00
    //qsfpdd 3.0 and later dom and above is on lower page 0x00 and higher page 0x17
    VALIDATE_SFP_PORT(logical_id);
    ONLP_TRY(_onlp_port_entry_get(logical_id, &entry));

    if (onlp_sfpi_is_present(port) !=  1) {
        ufi_bsp_info("sfp module (port=%d) is absent.", port);
        return ONLP_STATUS_OK;
    }

    memset(data, 0, 256);
    memset(eeprom_path, 0, sizeof(eeprom_path));

    snprintf(eeprom_path, sizeof(eeprom_path),
        "/sys_switch/transceiver/eth%d/eeprom",
        entry.parent);

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
int __WEAK onlp_sfpi_post_insert(int port, sff_info_t* info)
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
int __WEAK onlp_sfpi_control_supported(int port, onlp_sfp_control_t control, int* rv)
{
    uint32_t logical_id = 0;
    int count = 0;
    int base = 0;

    ONLP_TRY(onlp_sfpi_init());
    ONLP_TRY(_onlp_port_count_base_get(&count, &base));

    logical_id = port - base;
    VALIDATE_PORT(logical_id);

    //set unsupported as default value
    *rv = 0;

    switch (control) {
        case ONLP_SFP_CONTROL_RESET:
        case ONLP_SFP_CONTROL_RESET_STATE:
        case ONLP_SFP_CONTROL_LP_MODE:
            if (IS_XSFPX(logical_id)) {
                *rv = 1;
            }
            break;
        case ONLP_SFP_CONTROL_RX_LOS:
        case ONLP_SFP_CONTROL_TX_FAULT:
            if (IS_SFP(logical_id)) {
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
int __WEAK onlp_sfpi_control_set(int port, onlp_sfp_control_t control, int value)
{

    uint32_t logical_id = 0;
    port_elems entry = {0};
    int count = 0;
    int base = 0;
    int rv = ONLP_STATUS_OK;
    char sysfs[256] = {0};
    int op_type = OP_SYSFS;

    ONLP_TRY(onlp_sfpi_init());
    ONLP_TRY(_onlp_port_count_base_get(&count, &base));

    logical_id = port - base;
    ONLP_TRY(_onlp_port_entry_get(logical_id, &entry));

    if(value != 0 && value != 1) {
        return ONLP_STATUS_E_PARAM;
    }

    //check control is valid for this port
    switch(control)
    {
        case ONLP_SFP_CONTROL_RESET:
            {
                if (IS_XSFPX(logical_id)) {
                    op_type = OP_SYSFS;
                    snprintf(sysfs, sizeof(sysfs), "/sys_switch/transceiver/eth%d/reset",
                        entry.parent);
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_TX_DISABLE:
        case ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL:
            {
                if (IS_SFP(logical_id)) {
                    op_type = OP_SYSFS;
                    snprintf(sysfs, sizeof(sysfs), "/sys_switch/transceiver/eth%d/tx_disable",
                        entry.parent);
                } else if (IS_QSFPX(logical_id)) {
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
                } else if (IS_OSFP(logical_id)) {
                    op_type = OP_CMIS;
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                if (IS_XSFPX(logical_id)) {
                    op_type = OP_SYSFS;
                    snprintf(sysfs, sizeof(sysfs),
                        "/sys_switch/transceiver/eth%d/low_power_mode",
                        entry.parent);
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
    }

    if(op_type == OP_SYSFS) {
        //write reg_val
        if ((rv=onlp_file_write_int(value, sysfs)) < 0) {
            AIM_LOG_ERROR("Unable to write %s, error=%d, reg_val=%x", sysfs,  rv, value);
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
int __WEAK onlp_sfpi_control_get(int port, onlp_sfp_control_t control, int* value)
{

    uint32_t logical_id = 0;
    port_elems entry = {0};
    int count = 0;
    int base = 0;
    int rv = ONLP_STATUS_OK;
    char sysfs[256] = {0};
    int op_type = OP_SYSFS;

    ONLP_TRY(onlp_sfpi_init());
    ONLP_TRY(_onlp_port_count_base_get(&count, &base));

    logical_id = port - base;
    ONLP_TRY(_onlp_port_entry_get(logical_id, &entry));

    if(!value) {
        return ONLP_STATUS_E_PARAM;
    }

    //check control is valid for this port
    switch(control)
    {
        case ONLP_SFP_CONTROL_RESET_STATE:
            {
                if (IS_XSFPX(logical_id)) {
                    op_type = OP_SYSFS;
                    snprintf(sysfs, sizeof(sysfs), "/sys_switch/transceiver/eth%d/reset",
                        entry.parent);
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_LP_MODE:
            {
                if (IS_XSFPX(logical_id)) {
                    op_type = OP_SYSFS;
                    snprintf(sysfs, sizeof(sysfs),
                        "/sys_switch/transceiver/eth%d/low_power_mode",
                        entry.parent);
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_RX_LOS:
            {
                if (IS_SFP(logical_id)) {
                    op_type = OP_SYSFS;
                    snprintf(sysfs, sizeof(sysfs),
                        "/sys_switch/transceiver/eth%d/rx_los",
                        entry.parent);
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_TX_FAULT:
            {
                if (IS_SFP(logical_id)) {
                    op_type = OP_SYSFS;
                    snprintf(sysfs, sizeof(sysfs),
                        "/sys_switch/transceiver/eth%d/tx_fault",
                        entry.parent);
                } else {
                    return ONLP_STATUS_E_UNSUPPORTED;
                }
                break;
            }
        case ONLP_SFP_CONTROL_TX_DISABLE:
        case ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL:
            {
                if (IS_SFP(logical_id)) {
                    op_type = OP_SYSFS;
                    snprintf(sysfs, sizeof(sysfs),
                        "/sys_switch/transceiver/eth%d/tx_disable",
                        entry.parent);
                } else if (IS_QSFPX(logical_id)) {
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
                } else if (IS_OSFP(logical_id)) {
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
        if ((rv = ufi_file_read_int(value, sysfs)) < 0) {
            AIM_LOG_ERROR("onlp_sfpi_control_get() failed, error=%d, sysfs=%s", rv, sysfs);
            check_and_do_i2c_mux_reset(port);
            return rv;
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
int __WEAK onlp_sfpi_port_map(int port, int* rport)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Deinitialize the SFP driver.
 */
int __WEAK onlp_sfpi_denit(void)
{
    if (ports != NULL) {
        aim_free(ports);
        ports = NULL;
    }

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
void __WEAK onlp_sfpi_debug(int port, aim_pvs_t* pvs)
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
int __WEAK onlp_sfpi_ioctl(int port, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

