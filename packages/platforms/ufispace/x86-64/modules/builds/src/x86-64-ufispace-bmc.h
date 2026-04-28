#ifndef UFISPACE_BMC_H
#define UFISPACE_BMC_H

#include <linux/module.h>
#include <linux/platform_device.h>

/**
 * IPMI KCS/SMIC maximun request size
 * KCS/SMIC input minimum requirement is 40 byte.
 * ref:
 * ipmi-second-gen-interface-spec-v2-rev1-1, CH 6.14
 */
#define IPMI_OPENIPMI_MAX_REQ_SIZE          40

#define UDF_SUB_ID_MAX                (10)

#define UDF_SUB_TYPE_CMD                     0x01
#define UDF_SUB_TYPE_FRU                     0x02

struct sub_attr{
    char *name;
    umode_t mode;
};

struct bmc_udf_node{
    uint8_t id;
    uint8_t sub_type;
    uint8_t udf_cmd[IPMI_OPENIPMI_MAX_REQ_SIZE];
    uint8_t cmd_len;
    struct sub_attr sub_attrs[UDF_SUB_ID_MAX];
};

struct fru_procd_info {
    char *mf;
    char *pname;
    char *pn;
    char *pv;
    char type;
    char dir;
};

struct udf_show_store {
    ssize_t (*show)(uint8_t id, uint8_t sub_id, char *buf,
        void *rsp_buf, unsigned short rsp_buf_size);
    ssize_t (*store)(uint8_t id, uint8_t sub_id, const char *buf, size_t count,
        uint8_t *udf_cmd, uint8_t *cmd_len);
};

enum udf_id {
    UDF_ID_UDF_INVALID = 0,
    UDF_ID_UDF1,
    UDF_ID_UDF2,
    UDF_ID_UDF3,
    UDF_ID_UDF4,
    UDF_ID_UDF5,
    UDF_ID_UDF6,
    UDF_ID_UDF7,
    UDF_ID_UDF8,
    UDF_ID_UDF9,
    UDF_ID_UDF10,
    UDF_ID_UDF_MAX
};

extern struct udf_show_store ufi_bmc_show_store_udf[UDF_ID_UDF_MAX];

#endif