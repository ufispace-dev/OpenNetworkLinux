/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
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
#include <string.h>
#include <unistd.h>
#include <onlp/platformi/sfpi.h>
#include <onlplib/i2c.h>
#include <onlplib/file.h>
#include "platform_lib.h"

#define QSFP_NUM              40
#define SFPDD_NUM             4
#define QSFPDD_NUM            8
#define SFP_NUM               4
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

#define SYSFS_SFPDD_28_TX_DIS   "cpld_sfpdd_28_tx_dis"
#define SYSFS_SFP28_PRESENT     "cpld_sfp28_intr_present"
#define SYSFS_SFPDD_PRESENT     "cpld_sfpdd_intr_present"
#define SYSFS_SFPDD_28_RX_LOS   "cpld_sfpdd_28_rx_los"
#define SYSFS_SFPDD_28_TX_FAULT "cpld_sfpdd_28_tx_fault"
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

static int ufi_port_to_cpld_addr(int port)
{
    int cpld_addr = 0;

    if (port >= 0 && port <= 9) {
        cpld_addr = CPLD_BASE_ADDR[1];
    } else if (port >= 10 && port <= 19) {
        cpld_addr = CPLD_BASE_ADDR[2];
    } else if (port >= 20 && port <= 29) {
        cpld_addr = CPLD_BASE_ADDR[1];
    } else if (port >= 30 && port <= 39) {
        cpld_addr = CPLD_BASE_ADDR[2];
    } else if (IS_QSFPDD(port) || IS_SFPDD(port) || IS_SFP(port)) {
        cpld_addr = CPLD_BASE_ADDR[3];
    }

    return cpld_addr;
}

static int ufi_qsfp_port_to_sysfs_attr_offset(int port)
{
    int sysfs_attr_offset = -1;

    if (port >= 0 && port <= 3) {
        sysfs_attr_offset = 0;
    } else if (port >= 4 && port <= 7) {
        sysfs_attr_offset = 1;
    } else if (port >= 8 && port <= 9) {
        sysfs_attr_offset = 2;
    } else if (port >= 10 && port <= 13) {
        sysfs_attr_offset = 0;
    } else if (port >= 14 && port <= 17) {
        sysfs_attr_offset = 1;
    } else if (port >= 18 && port <= 19) {
        sysfs_attr_offset = 2;
    } else if (port >= 20 && port <= 23) {
        sysfs_attr_offset = 0;
    } else if (port >= 24 && port <= 27) {
        sysfs_attr_offset = 1;
    } else if (port >= 28 && port <= 29) {
        sysfs_attr_offset = 2;
    } else if (port >= 30 && port <= 33) {
        sysfs_attr_offset = 0;
    } else if (port >= 34 && port <= 37) {
        sysfs_attr_offset = 1;
    } else if (port >= 38 && port <= 39) {
        sysfs_attr_offset = 2;
    }
    return sysfs_attr_offset;
}

static int ufi_qsfp_port_to_bit_offset(int port)
{
    int bit_offset = -1; // Default value for an invalid port

    if (port >= 0 && port < 4) {
        bit_offset = port % 4;
    } else if (port >= 4 && port < 8) {
        bit_offset = port % 4;
    } else if (port >= 8 && port < 10) {
        bit_offset = port % 4;
    } else if (port >= 10 && port < 14) {
        bit_offset = port % 5;
    } else if (port >= 14 && port < 18) {
        bit_offset = port % 14;
    } else if (port >= 18 && port < 20) {
        bit_offset = port % 18;
    } else if (port >= 20 && port < 24) {
        bit_offset = port % 8;
    } else if (port >= 24 && port < 28) {
        bit_offset = port % 20;
    } else if (port >= 28 && port < 30) {
        bit_offset = port % 8;
    } else if (port >= 30 && port < 34) {
        bit_offset = port % 26;
    } else if (port >= 34 && port < 38) {
        bit_offset = port % 30;
    } else if (port >= 38 && port < 40) {
        bit_offset = port % 34;
    } else if (port >= 40 && port < 48) { //QSFPDD
        bit_offset = port % 40;
    } else if (port >= 48 && port < 52) { //SFPDD
        bit_offset = port % 48;
    } else if (port >= 52 && port < 56) { //SFP
        bit_offset = port % 48;
    }

    return bit_offset;
}

static int ufi_port_to_eeprom_bus(int port)
{
    int bus = -1;

    if (IS_QSFP(port)) { //QSFP
        bus =  port + 25;
    } else if (IS_QSFPDD(port)) { //QSFPDD
        bus =  port + 25;
    } else if (IS_SFPDD(port)) { //SFPDD
        bus =  port + 25;
    } else if (IS_SFP(port)) { //SFP
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

    if (IS_QSFP(port)) { //QSFP
        bus =  CPLD_I2C_BUS[1];
    } else if (IS_QSFPDD(port)) { //QSFPDD
        bus =  CPLD_I2C_BUS[1];
    } else if (IS_SFPDD(port)) { //SFPDD
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

    *pres_val = !((reg_val >> (port-52)) & 0x1);

    return ONLP_STATUS_OK;
}

static int ufi_sfpdd_present_get(int port, int *pres_val)
{
    int reg_val = 0, rc = 0;
    int cpld_bus = 0, cpld_addr = 0;

    //get cpld bus and cpld addr
    cpld_bus = ufi_port_to_cpld_bus(port);
    cpld_addr = ufi_port_to_cpld_addr(port);

    //read register
    if ((rc = file_read_hex(&reg_val, SYS_FMT, cpld_bus, cpld_addr, SYSFS_SFPDD_PRESENT)) < 0) {
        AIM_LOG_ERROR("Unable to read %s", SYSFS_SFPDD_PRESENT);
        AIM_LOG_ERROR(SYS_FMT, cpld_bus, cpld_addr, SYSFS_SFPDD_PRESENT);
        check_and_do_i2c_mux_reset(port);
        return rc;
    }

    *pres_val = !((reg_val >> ufi_qsfp_port_to_bit_offset(port)) & 0x1);

    return ONLP_STATUS_OK;
}

static int ufi_qsfpdd_present_get(int port, int *pres_val)
{
    int reg_val = 0, rc = 0;
    int cpld_bus = 0, cpld_addr = 0;

    //get cpld bus and cpld addr
    cpld_bus = ufi_port_to_cpld_bus(port);
    cpld_addr = ufi_port_to_cpld_addr(port);

    //read register
    if ((rc = file_read_hex(&reg_val, SYS_FMT, cpld_bus, cpld_addr, SYSFS_QSFPDD_PRESENT)) < 0) {
        AIM_LOG_ERROR("Unable to read %s", SYSFS_QSFPDD_PRESENT);
        AIM_LOG_ERROR(SYS_FMT, cpld_bus, cpld_addr, SYSFS_QSFPDD_PRESENT);
        check_and_do_i2c_mux_reset(port);
        return rc;
    }

    *pres_val = !((reg_val >> ufi_qsfp_port_to_bit_offset(port)) & 0x1);

    return ONLP_STATUS_OK;
}

/**
 * @brief Software initialization of the SFP module.
 */
int onlp_sfpi_sw_init(void)
{
    lock_init();
    return ONLP_STATUS_OK;
}

/**
 * @brief Hardware initialization of the SFP module.
 * @param flags The hardware initialization flags.
 */
int onlp_sfpi_hw_init(uint32_t flags)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Deinitialize the chassis software module.
 * @note The primary purpose of this API is to properly
 * deallocate any resources used by the module in order
 * faciliate detection of real resouce leaks.
 */
int onlp_sfpi_sw_denit(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the bitmap of SFP-capable port numbers.
 * @param[out] bmap Receives the bitmap.
 */
int onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{
    int p = 0;
    AIM_BITMAP_CLR_ALL(bmap);
    for(p = 0; p < PORT_NUM; p++) {
        AIM_BITMAP_SET(bmap, p);
    }
    return ONLP_STATUS_OK;
}

/**
 * @brief Determine the SFP connector type.
 * @param id The SFP Port ID.
 * @param[out] rtype Receives the connector type.
 */
int onlp_sfpi_type_get(onlp_oid_id_t id, onlp_sfp_type_t* rtype)
{
    int port = ONLP_OID_ID_GET(id);

    //QSFP, QSFPDD, SFP Ports
	if (IS_QSFP(port)) { //QSFPDD
        *rtype = ONLP_SFP_TYPE_QSFP28;
    } else if (IS_QSFPDD(port)) { //QSFPDD
        *rtype = ONLP_SFP_TYPE_QSFP28;
    } else if (IS_SFPDD(port)) { //SFPDD
        *rtype = ONLP_SFP_TYPE_SFP;
    } else if (IS_SFP(port)) { //SFP
        *rtype = ONLP_SFP_TYPE_SFP;

    } else { //unkonwn ports
        AIM_LOG_ERROR("unknown ports, port=%d, func=%s\n", port, __FUNCTION__);
        return ONLP_STATUS_E_PARAM;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Determine if an SFP is present.
 * @param id The SFP Port ID.
 * @returns 1 if present
 * @returns 0 if absent
 * @returns An error condition.
 */
int onlp_sfpi_is_present(onlp_oid_id_t id)
{
    int port = ONLP_OID_ID_GET(id);
    int status = ONLP_STATUS_OK;

    VALIDATE_PORT(port);

    //QSFPDD Ports
    if (IS_QSFP(port)) { //QSFP
        ONLP_TRY(ufi_qsfp_present_get(port, &status));
    } else if (IS_SFPDD(port)) { //SFPDD
        ONLP_TRY(ufi_sfpdd_present_get(port, &status));
    } else if (IS_QSFPDD(port)) { //QSFPDD
        ONLP_TRY(ufi_qsfpdd_present_get(port, &status));
    } else if (IS_SFP(port)) { //SFP
        ONLP_TRY(ufi_sfp_present_get(port, &status));
    } else {
        return ONLP_STATUS_E_PARAM;
    }

    return status;
}

/**
 * @brief Return the presence bitmap for all SFP ports.
 * @param[out] dst Receives the presence bitmap.
 */
int onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int p = 0;
    int rc = 0;

    AIM_BITMAP_CLR_ALL(dst);
    for (p = 0; p < PORT_NUM; p++) {
        rc = onlp_sfpi_is_present(p);
        AIM_BITMAP_MOD(dst, p, rc);
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Return the RX_LOS bitmap for all SFP ports.
 * @param[out] dst Receives the RX_LOS bitmap.
 */
int onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int i = 0, value = 0;

    /* Populate bitmap - QSFP ports */
    for(i = 0; i < (QSFP_NUM); i++) {
        AIM_BITMAP_MOD(dst, i, 0);
    }

    /* Populate bitmap - QSFPDD ports */
    for(i = QSFP_NUM; i < (QSFPX_NUM); i++) {
        AIM_BITMAP_MOD(dst, i, 0);
    }

    /* Populate bitmap - SFPDD ports */
    for(i = QSFPX_NUM; i < (QSFPX_SFPDD_NUM); i++) {
        ONLP_TRY(onlp_sfpi_control_get(i, ONLP_SFP_CONTROL_RX_LOS, &value));
        AIM_BITMAP_MOD(dst, i, value);
    }

    /* Populate bitmap - SFP+ ports */
    for(i = QSFPX_SFPDD_NUM; i < PORT_NUM; i++) {
        ONLP_TRY(onlp_sfpi_control_get(i, ONLP_SFP_CONTROL_RX_LOS, &value));
        AIM_BITMAP_MOD(dst, i, value);
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Read bytes from the target device on the given SFP port.
 * @param id The SFP Port ID.
 * @param devaddr The device address.
 * @param addr Read offset.
 * @param[out] dst Receives the data.
 * @param len Read length.
 * @returns The number of bytes read or ONLP_STATUS_E_* no error.
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
 * @brief Read a byte from the target device on the given SFP port.
 * @param id The SFP Port ID.
 * @param devaddr The device address.
 * @param addr The read address.
 * @returns The byte on success or ONLP_STATUS_E* on error.
 */
int onlp_sfpi_dev_readb(onlp_oid_id_t id, int devaddr, int addr)
{
    int port = ONLP_OID_ID_GET(id);
    int rc = 0;
    int bus = ufi_port_to_eeprom_bus(port);

    VALIDATE_PORT(port);
    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if ((rc=onlp_i2c_readb(bus, devaddr, addr, ONLP_I2C_F_FORCE)) < 0) {
        check_and_do_i2c_mux_reset(port);
    }

    return rc;
}

/**
 * @brief Write a byte to the target device on the given SFP port.
 * @param id The SFP Port ID.
 * @param devaddr The device address.
 * @param addr The write address.
 * @param value The write value.
 */
int onlp_sfpi_dev_writeb(onlp_oid_id_t id, int devaddr, int addr, uint8_t value)
{
    int port = ONLP_OID_ID_GET(id);
    int rc = 0;
    int bus = ufi_port_to_eeprom_bus(port);

    VALIDATE_PORT(port);
    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if ((rc=onlp_i2c_writeb(bus, devaddr, addr, value, ONLP_I2C_F_FORCE)) < 0) {
        check_and_do_i2c_mux_reset(port);
    }
    return rc;
}

/**
 * @brief Read a word from the target device on the given SFP port.
 * @param id The SFP Port ID.
 * @param devaddr The device address.
 * @param addr The read address.
 * @returns The word if successful, ONLP_STATUS_E* on error.
 */
int onlp_sfpi_dev_readw(onlp_oid_id_t id, int devaddr, int addr)
{
    int port = ONLP_OID_ID_GET(id);
    int rc = 0;
    int bus = ufi_port_to_eeprom_bus(port);

    VALIDATE_PORT(port);
    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if ((rc=onlp_i2c_readw(bus, devaddr, addr, ONLP_I2C_F_FORCE)) < 0) {
        check_and_do_i2c_mux_reset(port);
    }

    return rc;
}

/**
 * @brief Write a word to the target device on the given SFP port.
 * @param id The SFP Port ID.
 * @param devaddr The device address.
 * @param addr The write address.
 * @param value The write value.
 */
int onlp_sfpi_dev_writew(onlp_oid_id_t id, int devaddr, int addr, uint16_t value)
{
    int port = ONLP_OID_ID_GET(id);
    int rc = 0;

    VALIDATE_PORT(port);
    int bus = ufi_port_to_eeprom_bus(port);

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if ((rc=onlp_i2c_writew(bus, devaddr, addr, value, ONLP_I2C_F_FORCE)) < 0) {
        check_and_do_i2c_mux_reset(port);
    }

    return rc;
}

/**
 * @brief Read bytes from the target device on the given SFP port.
 * @param id The SFP Port ID.
 * @param devaddr The device address.
 * @param addr Read offset.
 * @param[out] dst Receives the data.
 * @param len Read length.
 * @returns The number of bytes read or ONLP_STATUS_E_* no error.
 */
int onlp_sfpi_dev_read(onlp_oid_id_t id, int devaddr, int addr, uint8_t* dst, int len)
{
    int port = ONLP_OID_ID_GET(id);
    VALIDATE_PORT(port);
    int bus = ufi_port_to_eeprom_bus(port);

    if (onlp_sfpi_is_present(port) !=  1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

    if (onlp_i2c_block_read(bus, devaddr, addr, len, dst, ONLP_I2C_F_FORCE) < 0) {
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Write bytes to the target device on the given SFP port.
 * @param id The SFP Port ID.
 * @param devaddr The device address.
 * @param addr Write offset.
 * @param src The bytes to write.
 * @param len Write length.
 */
int onlp_sfpi_dev_write(onlp_oid_id_t id, int devaddr, int addr, uint8_t* src, int len)
{
    int port = ONLP_OID_ID_GET(id);
    VALIDATE_PORT(port);
    int rc = 0;
    int bus = ufi_port_to_eeprom_bus(port);

    if (onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_INFO("sfp module (port=%d) is absent.\n", port);
        return ONLP_STATUS_OK;
    }

        if ((rc=onlp_i2c_write(bus, devaddr, addr, len, src, ONLP_I2C_F_FORCE)) < 0) {

        check_and_do_i2c_mux_reset(port);
    }

    return rc;
}

/**
 * @brief Perform any actions required after an SFP is inserted.
 * @param id The SFP Port ID.
 * @param info The SFF Module information structure.
 * @note This function is optional. If your platform must
 * adjust equalizer or preemphasis settings internally then
 * this function should be implemented as the trigger.
 */
int onlp_sfpi_post_insert(onlp_oid_id_t id, sff_info_t* info)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Returns whether or not the given control is supported on the given port.
 * @param id The SFP Port ID.
 * @param control The control.
 * @param[out] rv Receives 1 if supported, 0 if not supported.
 * @note This provided for convenience and is optional.
 * If you implement this function your control_set and control_get APIs
 * will not be called on unsupported ports.
 */
int onlp_sfpi_control_supported(onlp_oid_id_t id, onlp_sfp_control_t control, int* rv)
{
    int port = ONLP_OID_ID_GET(id);
    VALIDATE_PORT(port);

    //set unsupported as default value
    *rv = 0;

    switch (control) {
        case ONLP_SFP_CONTROL_RESET:
        case ONLP_SFP_CONTROL_RESET_STATE:
        case ONLP_SFP_CONTROL_LP_MODE:
            if (IS_QSFP(port) || IS_QSFPDD(port)) {
                *rv = 1;
            }
            break;
        case ONLP_SFP_CONTROL_RX_LOS:
        case ONLP_SFP_CONTROL_TX_FAULT:
        case ONLP_SFP_CONTROL_TX_DISABLE:
            if (IS_SFP(port) || IS_SFPDD(port)) {
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
 * @param id The SFP Port ID.
 * @param control The control.
 * @param value The value.
 */
int onlp_sfpi_control_set(onlp_oid_id_t id, onlp_sfp_control_t control, int value)
{
    int port = ONLP_OID_ID_GET(id);
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
                if (IS_QSFP(port)) {
                    //read reg_val
                    attr_offset = ufi_qsfp_port_to_sysfs_attr_offset(port);
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFP_RESET, attr_offset) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //update reg_val
                    //0 is normal, 1 is reset, reverse value to fit our platform
                    bit_offset = ufi_qsfp_port_to_bit_offset(port);
                    reg_val = ufi_bit_operation(reg_val, bit_offset, !value);

                    //write reg_val
                    if ((rc=onlp_file_write_int(reg_val, SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFP_RESET, attr_offset)) < 0) {
                        AIM_LOG_ERROR("Unable to write %s, error=%d, reg_val=%x", SYSFS_QSFP_RESET,  rc, reg_val);
                        AIM_LOG_ERROR(SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFP_RESET);
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }
                    rc = ONLP_STATUS_OK;
                } else if (IS_QSFPDD(port)) {
                    //read reg_val
                    if (file_read_hex(&reg_val, SYS_FMT, bus, cpld_addr, SYSFS_QSFPDD_RESET) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //update reg_val
                    //0 is normal, 1 is reset, reverse value to fit our platform
                    bit_offset = ufi_qsfp_port_to_bit_offset(port);
                    reg_val = ufi_bit_operation(reg_val, bit_offset, !value);

                    //write reg_val
                    if ((rc=onlp_file_write_int(reg_val, SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFPDD_RESET)) < 0) {
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
                if (IS_SFP(port) || IS_SFPDD(port)) {
                    //read reg_val
                    if (file_read_hex(&reg_val, SYS_FMT, bus, cpld_addr, SYSFS_SFPDD_28_TX_DIS) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //update reg_val
                    //0 is normal, 1 is turn off, reverse value to fit our platform
                    bit_offset = ufi_qsfp_port_to_bit_offset(port);
                    reg_val = ufi_bit_operation(reg_val, bit_offset, value);

                    //write reg_val
                    if (onlp_file_write_int(reg_val, SYS_FMT, bus, cpld_addr, SYSFS_SFPDD_28_TX_DIS) < 0) {
                        AIM_LOG_ERROR("Unable to write %s, error=%d, reg_val=%x", SYSFS_SFPDD_28_TX_DIS, rc, reg_val);
                        AIM_LOG_ERROR(SYS_FMT, bus, cpld_addr, SYSFS_SFPDD_28_TX_DIS);
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
                if (IS_QSFP(port)) {
                    //read reg_val
                    attr_offset = ufi_qsfp_port_to_sysfs_attr_offset(port);
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFP_LPMODE, attr_offset) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //update reg_val
                    //0 is normal, 1 is low power
                    bit_offset = ufi_qsfp_port_to_bit_offset(port);
                    reg_val = ufi_bit_operation(reg_val, bit_offset, value);

                    //write reg_val
                    if (onlp_file_write_int(reg_val, SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFP_LPMODE, attr_offset) < 0) {
                        AIM_LOG_ERROR("Unable to write %s, error=%d, reg_val=%x", SYSFS_QSFP_LPMODE,  rc, reg_val);
                        AIM_LOG_ERROR(SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFP_LPMODE);
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }
                    rc = ONLP_STATUS_OK;
                } else if (IS_QSFPDD(port)) {
                    if (file_read_hex(&reg_val, SYS_FMT, bus, cpld_addr, SYSFS_QSFPDD_LPMODE) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //update reg_val
                    //0 is normal, 1 is low power
                    bit_offset = ufi_qsfp_port_to_bit_offset(port);
                    reg_val = ufi_bit_operation(reg_val, bit_offset, value);

                    //write reg_val
                    if (onlp_file_write_int(reg_val, SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFPDD_LPMODE) < 0) {
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
 * @param id The SFP Port ID.
 * @param control The control
 * @param[out] value Receives the current value.
 */
int onlp_sfpi_control_get(onlp_oid_id_t id, onlp_sfp_control_t control, int* value)
{
    int port = ONLP_OID_ID_GET(id);
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
                if (IS_QSFP(port)) {
                    //read reg_val
                    attr_offset = ufi_qsfp_port_to_sysfs_attr_offset(port);
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFP_RESET, attr_offset) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //read bit value
                    //0 is normal, 1 is reset, reverse value to fit our platform
                    bit_offset = ufi_qsfp_port_to_bit_offset(port);
                    reg_mask = 1 << bit_offset;
                    *value = !ufi_mask_shift(reg_val, reg_mask);

                    rc = ONLP_STATUS_OK;
                } else if (IS_QSFPDD(port)) {
                    if (file_read_hex(&reg_val, SYS_FMT, bus, cpld_addr, SYSFS_QSFPDD_RESET) < 0) {
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
                if (IS_SFP(port) || IS_SFPDD(port)) {
                    //read reg_val
                    if (file_read_hex(&reg_val, SYS_FMT, bus, cpld_addr, SYSFS_SFPDD_28_RX_LOS) < 0) {
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
                if (IS_SFP(port) || IS_SFPDD(port)) {
                    //read reg_val
                    if (file_read_hex(&reg_val, SYS_FMT, bus, cpld_addr, SYSFS_SFPDD_28_TX_FAULT) < 0) {
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
                if (IS_SFP(port) || IS_SFPDD(port)) {
                    //read reg_val
                    if (file_read_hex(&reg_val, SYS_FMT, bus, cpld_addr, SYSFS_SFPDD_28_TX_DIS) < 0) {
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
                if (IS_QSFP(port)) {
                    //read reg_val
                    attr_offset = ufi_qsfp_port_to_sysfs_attr_offset(port);
                    if (file_read_hex(&reg_val, SYS_FMT_OFFSET, bus, cpld_addr, SYSFS_QSFP_LPMODE, attr_offset) < 0) {
                        check_and_do_i2c_mux_reset(port);
                        return ONLP_STATUS_E_INTERNAL;
                    }

                    //read bit value
                    //0 is normal, 1 is low power
                    bit_offset = ufi_qsfp_port_to_bit_offset(port);
                    reg_mask = 1 << bit_offset;
                    *value = ufi_mask_shift(reg_val, reg_mask);

                    rc = ONLP_STATUS_OK;
                } else if (IS_QSFPDD(port)) {
                    //read reg_val
                    if (file_read_hex(&reg_val, SYS_FMT, bus, cpld_addr, SYSFS_QSFPDD_LPMODE) < 0) {
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
 * @param id The SFP Port ID.
 * @param[out] rport Receives the new port.
 * @note This function will be called to remap the user SFP port number
 * to the number returned in rport before the SFPI functions are called.
 * This is an optional convenience for platforms with dynamic or
 * variant physical SFP numbering.
 */
int onlp_sfpi_port_map(onlp_oid_id_t id, int* rport)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Get the SFP's OID header.
 * @param id The SFP oid.
 * @param [out] hdr Receives the header.
 */
int onlp_sfpi_hdr_get(onlp_oid_id_t id, onlp_oid_hdr_t* hdr)
{
    int local_id = ONLP_OID_ID_GET(id);
    int is_present = onlp_sfpi_is_present(id);

    hdr->id = ONLP_SFP_ID_CREATE(local_id);
    hdr->poid = ONLP_OID_CHASSIS;
    hdr->status = is_present ? ONLP_OID_STATUS_FLAG_PRESENT : ONLP_OID_STATUS_FLAG_UNPLUGGED;

    return ONLP_STATUS_OK;
}