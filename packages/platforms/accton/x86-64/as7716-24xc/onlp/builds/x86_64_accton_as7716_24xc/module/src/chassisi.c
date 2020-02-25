#include <onlp/platformi/chassisi.h>

int
onlp_chassisi_hdr_get(onlp_oid_id_t id, onlp_oid_hdr_t* hdr)
{
    int i;
    onlp_oid_t* e = hdr->coids;

    ONLP_OID_STATUS_FLAG_SET(hdr, PRESENT);
    ONLP_OID_STATUS_FLAG_SET(hdr, OPERATIONAL);

    /* 5 Thermal sensors on the chassis */
    for (i = 1; i <= 5; i++)
    {
        *e++ = ONLP_THERMAL_ID_CREATE(i);
    }

    /* 5 LEDs on the chassis */
    for (i = 1; i <= 5; i++)
    {
        *e++ = ONLP_LED_ID_CREATE(i);
    }

    /* 2 PSUs on the chassis */
    for (i = 1; i <= 2; i++)
    {
        *e++ = ONLP_PSU_ID_CREATE(i);
    }

    /* 6 Fans on the chassis */
    for (i = 1; i <= 6; i++)
    {
        *e++ = ONLP_FAN_ID_CREATE(i);
    }

    /* 16 SFPs */
    for(i = 1; i <= 16; i++) {
        *e++ = ONLP_SFP_ID_CREATE(i);
    }
    return 0;
}
