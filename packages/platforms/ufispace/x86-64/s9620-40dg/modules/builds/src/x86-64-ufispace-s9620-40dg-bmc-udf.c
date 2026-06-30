/*
 * A Ufispace BMC user define function driver for the ufispace
 *
 * Copyright (C) 2026 UfiSpace Technology Corporation.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "x86-64-ufispace-bmc.h"
extern int ufi_bmc_udf_create(struct device * dev, struct bmc_udf_node *table);
extern void ufi_bmc_udf_destroy(struct device * dev);

extern struct platform_device ufi_bmc_dev;

int ufi_bmc_udf_init(void);
void ufi_bmc_udf_exit(void);

struct bmc_udf_node table[] = {
    {
        .id = UDF_ID_UDF1,
        .sub_type = UDF_SUB_TYPE_FRU,
        .udf_cmd = {1},
        .cmd_len = 1,
        .sub_attrs = {
            {.name = "psu0_type", .mode = (S_IRUGO)},
            {.name = "psu0_fan_dir", .mode = (S_IRUGO)},
        }
    },
    {
        .id = UDF_ID_UDF2,
        .sub_type = UDF_SUB_TYPE_FRU,
        .udf_cmd = {2},
        .cmd_len = 1,
        .sub_attrs = {
            {.name = "psu1_type", .mode = (S_IRUGO)},
            {.name = "psu1_fan_dir", .mode = (S_IRUGO)},
        }
    },
    // An empty element must be left as a terminator.
    {}
};

/**
 *   get psu type
 */
static ssize_t class_psu_type_dir_show(uint8_t id, uint8_t sub_id, char *buf,
       void *rsp_buf, unsigned short rsp_buf_size)
{
    #define UDF_ID_FRU1_SUB_ID_TYPE  0
    #define UDF_ID_FRU1_SUB_ID_DIR 1
    int rv = 0;
    struct fru_procd_info *node = NULL;
    struct fru_procd_info support_list[] = {
        {
            .mf = "FSPGROUP",
            .pname = "YNEM1000AM",
            .pn = "YNEM1000AM-2R01T10",
            .type = 1, // 0 DC, 1 AC
            .dir = 0 // 0 F2B, 1 B2F
        },
        {
            .mf = "FSPGROUP",
            .pname = "YNEM1000DM",
            .pn = "YNEM1000DM-2R01N01",
            .type = 1, // 0 DC, 1 AC
            .dir = 0 // 0 F2B, 1 B2F
        },
        // An empty element must be left as a terminator.
        {}
    };
    struct fru_procd_info* info = (struct fru_procd_info*) rsp_buf;
    for(node=&support_list[0]; !!node->mf; node++) {
        if(!strncmp(info->mf, node->mf, strlen(node->mf)+1) &&
            !strncmp(info->pn, node->pn, strlen(node->pn)+1)) {
            if(sub_id == UDF_ID_FRU1_SUB_ID_TYPE) {
                rv = sprintf(buf, "%d\n", node->type);
            } else if (sub_id == UDF_ID_FRU1_SUB_ID_DIR) {
                rv = sprintf(buf, "%d\n", node->dir);
            } else {
                rv = sprintf(buf, "NA\n");
            }

            return rv;
        }
    }
    rv = sprintf(buf, "NA\n");
    return rv;
}

int ufi_bmc_udf_init(void)
{
    ufi_bmc_show_store_udf[UDF_ID_UDF1].show = class_psu_type_dir_show;
    ufi_bmc_show_store_udf[UDF_ID_UDF2].show = class_psu_type_dir_show;
    ufi_bmc_udf_create(&ufi_bmc_dev.dev, table);
    return 0;
}

void ufi_bmc_udf_exit(void)
{
    ufi_bmc_udf_destroy(&ufi_bmc_dev.dev);
    memset(ufi_bmc_show_store_udf, 0, sizeof(ufi_bmc_show_store_udf));
    return;
}

MODULE_AUTHOR("Nonodark Huang <nonodark.huang@ufispace.com>");
MODULE_DESCRIPTION("x86_64_ufispace_bmc_udf driver");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: x86_64_ufispace_bmc");

module_init(ufi_bmc_udf_init);
module_exit(ufi_bmc_udf_exit);
