#include <onlplib/i2c.h>
#include <onlplib/file.h>
#include <onlp/platformi/ledi.h>
#include "platform_lib.h"


// TODO:
int onlp_ledi_sw_init(void) { return ONLP_STATUS_OK; }
int onlp_ledi_hw_init(uint32_t flags) { return ONLP_STATUS_OK; }
int onlp_ledi_sw_denit(void) { return ONLP_STATUS_OK; }
int onlp_ledi_hdr_get(onlp_oid_id_t id, onlp_oid_hdr_t* rv) { return ONLP_STATUS_OK; }
int onlp_ledi_info_get(onlp_oid_id_t id, onlp_led_info_t* rv) { return ONLP_STATUS_OK; }
int onlp_ledi_caps_get(onlp_oid_id_t id, uint32_t* rv) { return ONLP_STATUS_OK; }
int onlp_ledi_mode_set(onlp_oid_id_t id, onlp_led_mode_t mode) { return ONLP_STATUS_OK; }
int onlp_ledi_char_set(onlp_oid_id_t id, char c) { return ONLP_STATUS_OK; }

