/************************************************************
 * Copyright (C) 2026 Ufispace Technology Corporation.
 ************************************************************
 *
 * Platform Library
 *
 ***********************************************************/
#include <unistd.h>
#include <sys/io.h>
#include <onlplib/shlocks.h>
#include <onlp/oids.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <limits.h>
#include <ufispace_platform/platform_lib.h>
#include <ufispace_common/platform_lib_main.h>

const warm_reset_data_t __WEAK warm_reset_data[] = {
//                     unit_max | dev | unit
    [WARM_RESET_ALL] = {-1,      NULL, NULL}, //not support
    [WARM_RESET_MAC] = {-1,      NULL, NULL}, //not support
    [WARM_RESET_PHY] = {-1,      NULL, NULL}, //not support
    [WARM_RESET_MUX] = {-1,      NULL, NULL}, //not support
    [WARM_RESET_OP2] = {-1,      NULL, NULL}, //not support
    [WARM_RESET_GB]  = {-1,      NULL, NULL}, //not support
    [WARM_RESET_I210]= {-1,      NULL, NULL}, //not support
    [WARM_RESET_RT]  = {-1,      NULL, NULL}, //not support
};

static onlp_shlock_t* onlp_lock = NULL;

#define ONLP_LOCK() \
    do{ \
        onlp_shlock_take(onlp_lock); \
    }while(0)

#define ONLP_UNLOCK() \
    do{ \
        onlp_shlock_give(onlp_lock); \
    }while(0)

#define LOCK_MAGIC 0xA2B4C6D8

void lock_init()
{
    static int sem_inited = 0;
    if(!sem_inited) {
        onlp_shlock_create(LOCK_MAGIC, &onlp_lock, "bmc-file-lock");
        sem_inited = 1;
        check_and_do_i2c_mux_reset(-1);
    }
}

int ufi_file_read_longlong(long long int* value, const char* fmt, ...)
{
    int rv;
    va_list vargs;
    uint8_t data[32] = {0};
    int len;
    long long int llval = 0;
    char *endptr = NULL;
    va_start(vargs, fmt);
    rv = onlp_file_vread(data, sizeof(data) - 1, &len, fmt, vargs);
    va_end(vargs);
    if(rv < 0) {
        return rv;
    }

    errno = 0;
    llval = strtoll((char*)data, &endptr, 0);

    if ((errno != 0)) {
        // Fail
        *value = 0;
        return ONLP_STATUS_E_INVALID;
    }

    if (endptr == (char*)data) {
        // Can't find number
        *value = 0;
        return ONLP_STATUS_E_INVALID;
    }

    if (*endptr == '\0' || *endptr == '\n') {
        *value = llval;
        return ONLP_STATUS_OK;
    } else {
        // Partial sucess
        *value = 0;
        return ONLP_STATUS_E_INVALID;
    }
}

/*
 * Since onlp_file_read_int uses atoi(), it does not return an error code
 * when the string is non-numeric (e.g., "NA").
 * This function addresses this issue.
 *
 * Returns:
 * - ONLP_STATUS_OK: Success
 * - ONLP_STATUS_E_UNSUPPORTED: If the value is "NS"
 * - ONLP_STATUS_E_INVALID: Otherwise
 */
int ufi_file_read_int(int* value, const char* fmt, ...)
{
    int rv;
    va_list vargs;
    uint8_t data[32] = {0};
    int len;
    long int lval = 0;
    char *endptr = NULL;
    size_t str_len;
    va_start(vargs, fmt);
    rv = onlp_file_vread(data, sizeof(data) - 1, &len, fmt, vargs);
    va_end(vargs);
    if(rv < 0) {
        return rv;
    }

    str_len = strlen((char*)data);
    if (str_len > 0 && data[str_len - 1] == '\n') {
        data[str_len - 1] = '\0';
    }

    if (!strncmp((char *)data, COMM_STR_NS, sizeof(COMM_STR_NS))) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    errno = 0;
    lval = strtol((char*)data, &endptr, 0);

    if ((errno != 0)) {
        // Fail
        *value = 0;
        return ONLP_STATUS_E_INVALID;
    }

    if (lval > INT_MAX || lval < INT_MIN) {
        // Overflow
        *value = 0;
        return ONLP_STATUS_E_INVALID;
    }

    if (endptr == (char*)data) {
        // Can't find number
        *value = 0;
        return ONLP_STATUS_E_INVALID;
    }

    if (*endptr == '\0') {
        *value = (int) lval;
        return ONLP_STATUS_OK;
    } else {
        // Partial sucess
        *value = 0;
        return ONLP_STATUS_E_INVALID;
    }
}

/**
 * @brief Get board version
 * @param board [out] board data struct
 */
int ufi_get_board_version(board_t *board)
{
    int rv = ONLP_STATUS_OK;

    if(board == NULL) {
        return ONLP_STATUS_E_INVALID;
    }

    //Get HW Version
    if(ufi_file_read_int(&board->hw_rev, "/sys_switch/slot/slot1/model") != ONLP_STATUS_OK ||
       ufi_file_read_int(&board->deph_id, "/sys_switch/slot/slot1/design_phase") != ONLP_STATUS_OK ||
       ufi_file_read_int(&board->hw_build, "/sys_switch/slot/slot1/hardware_version") != ONLP_STATUS_OK ||
       ufi_file_read_int(&board->rev_id, "/sys_switch/slot/slot1/build_revision") != ONLP_STATUS_OK)
    {
        board->hw_rev = 0;
        board->deph_id = 0;
        board->hw_build = 0;
        board->rev_id = 0;
        rv = ONLP_STATUS_E_INVALID;
    }

    return rv;
}

int exec_cmd(char *cmd, char* out, int size) {
    FILE *fp;

    /* Open the command for reading. */
    fp = popen(cmd, "r");
    if(fp == NULL) {
        AIM_LOG_ERROR("Failed to run command %s\n", cmd );
        return ONLP_STATUS_E_INTERNAL;
    }

    /* Read the output a line at a time - output it. */
    while (fgets(out, size-1, fp) != NULL) {
    }

    /* close */
    pclose(fp);

    return ONLP_STATUS_OK;
}

/*
 * This function check the I2C bus statuas by using the sysfs of cpld_id,
 * If the I2C Bus is stcuk, do the i2c mux reset.
 */
void check_and_do_i2c_mux_reset(int port)
{
    int i2c_status = I2C_STUCK_STATUS_NORMAL;
    int rv = ONLP_STATUS_OK;
    char *mux_rest_path = "/sys_switch/slot/slot1/mux_reset_all";
    struct stat st = {0};

    rv = ufi_file_read_int(&i2c_status, "/sys_switch/slot/slot1/i2c_stuck");
    if(rv == ONLP_STATUS_OK && i2c_status == I2C_STUCK_STATUS_ROOT_BUS) {
        if (stat(mux_rest_path, &st) == 0  && (st.st_mode & S_IWUSR)) {
            onlp_file_write_int(1, mux_rest_path);
        }
    }
}

/* reg shift */
uint8_t ufi_shift(uint8_t mask)
{
    int i=0, mask_one=1;

    for(i=0; i<8; ++i) {
        if((mask & mask_one) == 1)
            return i;
        else
            mask >>= 1;
    }

    return -1;
}

/* reg mask and shift */
uint8_t ufi_mask_shift(uint8_t val, uint8_t mask)
{
    int shift=0;

    shift = ufi_shift(mask);

    return (val & mask) >> shift;
}

uint8_t ufi_bit_operation(uint8_t reg_val, uint8_t bit, uint8_t bit_val)
{
    if(bit_val == 0)
        reg_val = reg_val & ~(1 << bit);
    else
        reg_val = reg_val | (1 << bit);
    return reg_val;
}

__WEAK int ufi_get_warm_reset_path(const char **path)
{
    if(!path) {
        return ONLP_STATUS_E_INVALID;
    }

    *path = WARM_RESET_PATH;

    return ONLP_STATUS_OK;
}

__WEAK int ufi_get_warm_reset_data_table(const warm_reset_data_t **data_table)
{
    if(!data_table) {
        return ONLP_STATUS_E_INVALID;
    }

    *data_table = warm_reset_data;
    return ONLP_STATUS_OK;
}

/**
 * @brief warm reset for mac, phy, mux and op2
 * @param unit_id The warm reset device unit id
 * @param reset_dev The warm reset device id
 * @param ret return value.
 */
__WEAK int onlp_data_path_reset(uint8_t unit_id, uint8_t reset_dev)
{
    char cmd_buf[256] = {0};
    char dev_unit_buf[32] = {0};
    const char *warm_reset_path = WARM_RESET_PATH;
    const warm_reset_data_t *data_table = warm_reset_data;
    const warm_reset_data_t *data = NULL;
    int ret = 0;
    struct stat st = {0};

    if (reset_dev >= WARM_RESET_MAX) {
        AIM_LOG_ERROR("%s() dev_id(%d) out of range.", __func__, reset_dev);
        return ONLP_STATUS_E_PARAM;
    }

    ufi_get_warm_reset_path(&warm_reset_path);

    if (stat(warm_reset_path, &st) != 0) {
        AIM_LOG_ERROR("%s() file not exist, file=%s", __func__, warm_reset_path);
        return ONLP_STATUS_E_INTERNAL;
    }

    ufi_get_warm_reset_data_table(&data_table);

    if (data_table[reset_dev].warm_reset_dev_str == NULL) {
        AIM_LOG_ERROR("%s() reset_dev not support, reset_dev=%d", __func__, reset_dev);
        return ONLP_STATUS_E_PARAM;
    }

    data = &data_table[reset_dev];

    if (data != NULL && data->warm_reset_dev_str != NULL) {
        snprintf(dev_unit_buf, sizeof(dev_unit_buf), "%s", data->warm_reset_dev_str);
        if (data->unit_str != NULL && unit_id < data->unit_max) {  // assuming unit_max is defined
            snprintf(dev_unit_buf + strlen(dev_unit_buf), sizeof(dev_unit_buf) - strlen(dev_unit_buf),
                     " %s", data->unit_str[unit_id]);
        }
        snprintf(cmd_buf, sizeof(cmd_buf), CMD_WARM_RESET, WARM_RESET_TIMEOUT, warm_reset_path, dev_unit_buf);
        AIM_LOG_INFO("%s() info, warm reset cmd=%s", __func__, cmd_buf); //TODO
        ret = system(cmd_buf);
    } else {
        AIM_LOG_ERROR("%s() error, invalid reset_dev %d", __func__, reset_dev);
        return ONLP_STATUS_E_PARAM;
    }

    if (ret != 0) {
        AIM_LOG_ERROR("%s() error, please check dmesg error output.", __func__);
        return ONLP_STATUS_E_INTERNAL;
    }


    return ret;
}

/**
  * @brief Get the psu oid base.
  */
int __WEAK _onlp_psu_oid_base_get(int *base)
{
    if(!base) {
        return ONLP_STATUS_E_INVALID;
    } else {
        *base = DEFAULT_PSU_OID_BASE;
        return ONLP_STATUS_OK;
    }
}

/**
  * @brief Get the temp oid base.
  */
int __WEAK _onlp_temp_oid_base_get(int *base)
{
    if(!base) {
        return ONLP_STATUS_E_INVALID;
    } else {
        *base = DEFAULT_TEMP_OID_BASE;
        return ONLP_STATUS_OK;
    }
}

/**
  * @brief Get the fan oid base.
  */
int __WEAK _onlp_fan_oid_base_get(int *base)
{
    if(!base) {
        return ONLP_STATUS_E_INVALID;
    } else {
        *base = DEFAULT_FAN_OID_BASE;
        return ONLP_STATUS_OK;
    }
}

/**
  * @brief Get the led oid base.
  */
int __WEAK _onlp_led_oid_base_get(int *base)
{
    if(!base) {
        return ONLP_STATUS_E_INVALID;
    } else {
        *base = DEFAULT_LED_OID_BASE;
        return ONLP_STATUS_OK;
    }
}
