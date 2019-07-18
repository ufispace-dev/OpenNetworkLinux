
#include <onlp/platformi/base.h>

// FIXME: this is for testing only, not for production.....
// TODO: should we keep this part?

const char*
onlp_platformi_get(void)
{
    return "x86-64-accton-wedge100bf-65x-r0";
}

int
onlp_platformi_sw_init(void)
{
    return ONLP_STATUS_OK;
}

int
onlp_platformi_manage_fans(void)
{
    return ONLP_STATUS_OK;
}

int
onlp_platformi_manage_leds(void)
{
    return ONLP_STATUS_OK;
}
