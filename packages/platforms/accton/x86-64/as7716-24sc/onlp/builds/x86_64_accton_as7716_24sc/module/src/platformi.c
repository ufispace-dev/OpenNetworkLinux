
#include <onlp/platformi/base.h>

const char*
onlp_platformi_get(void)
{
    return "x86-64-accton-as7716-24sc-r0";
}

int
onlp_platformi_sw_init(void)
{
    return ONLP_STATUS_OK;
}

int
onlp_platformi_manage_fans(void)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
onlp_platformi_manage_leds(void)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}
