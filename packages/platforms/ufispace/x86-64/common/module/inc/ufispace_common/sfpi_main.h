#ifndef __SFPI_MAIN_H__
#define __SFPI_MAIN_H__

#define ALL_PORTS             -1

#define EEPROM_ADDR         (0x50)
#define EEPROM_SFP_DOM_ADDR (0x51)
#define TX_DIS_INPUT_MAX            (0xff) /* for input value validation only */
#define SFF8636_EEPROM_OFFSET_TXDIS (0x56)
#define SFF8636_EEPROM_TX_DIS       (0x0f) /* txdis valid bit(bit0-bit3), xxxx 1111 */
#define SFF8636_EEPROM_TX_EN        (0x0)


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

#define PORT_TYPE_DICT_SIZE (sizeof(port_type_dict) / sizeof(PortTypeDictEntry))
#define IS_PORT_INVALID(logical_id)  (_onlp_port_valid_check(logical_id))
#define IS_SFP(logical_id)           (!_onlp_port_type_check(logical_id, TYPE_SFP))
#define IS_XSFPX(logical_id)         (IS_OSFP(logical_id) || IS_QSFPX(logical_id))
#define IS_QSFPX(logical_id)         (IS_QSFP(logical_id) || IS_QSFPDD(logical_id))
#define IS_QSFP(logical_id)          (!_onlp_port_type_check(logical_id, TYPE_QSFP))
#define IS_QSFPDD(logical_id)        (!_onlp_port_type_check(logical_id, TYPE_QSFPDD))
#define IS_OSFP(logical_id)          (!_onlp_port_type_check(logical_id, TYPE_OSFP))

#define VALIDATE_PORT(logical_id) { if (IS_PORT_INVALID(logical_id)) return ONLP_STATUS_E_PARAM; }
#define VALIDATE_SFP_PORT(logical_id) { if (IS_PORT_INVALID(logical_id) || !IS_SFP(logical_id)) return ONLP_STATUS_E_PARAM; }

typedef struct {
    int key;  //[module_type]
    int value;  // [dev_class]
} PortTypeDictEntry;

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
    int type;
    int parent;
    int valid;
} port_elems;

int _onlp_port_total_get(int *total);
int _onlp_port_base_get(int *total);
#endif  /* __SFPI_MAIN_H__ */