#ifndef __PLATFORM_LIB_H__
#define __PLATFORM_LIB_H__

#include "x86_64_accton_wedge100bf_65x_log.h"

#define DEBUG_MODE 1

#if (DEBUG_MODE == 1)
    #define DEBUG_PRINT(fmt, args...)                                        \
        printf("%s:%s[%d]: " fmt "\r\n", __FILE__, __FUNCTION__, __LINE__, ##args)
#else
    #define DEBUG_PRINT(fmt, args...)
#endif

#define CHASSIS_FAN_COUNT     5
#define CHASSIS_THERMAL_COUNT 8
#define CHASSIS_LED_COUNT     2
#define CHASSIS_PSU_COUNT     2

#define IDPROM_PATH "/sys/class/i2c-adapter/i2c-41/41-0050/eeprom"

int bmc_send_command(char *cmd);
int bmc_file_read_int(int* value, char *file, int base);
int bmc_i2c_readb(uint8_t bus, uint8_t devaddr, uint8_t addr);
int bmc_i2c_writeb(uint8_t bus, uint8_t devaddr, uint8_t addr, uint8_t value);
int bmc_i2c_readw(uint8_t bus, uint8_t devaddr, uint8_t addr);
int bmc_i2c_readraw(uint8_t bus, uint8_t devaddr, uint8_t addr, char* data, int data_size);

#endif  /* __PLATFORM_LIB_H__ */


