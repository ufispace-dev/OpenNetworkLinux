/**************************************************************************//**
 *
 * x86_64_ufispace_s9510_28dc module
 *
 *****************************************************************************/
#include <x86_64_ufispace_s9510_28dc/x86_64_ufispace_s9510_28dc_config.h>

#include "x86_64_ufispace_s9510_28dc_log.h"

static int
datatypes_init__(void)
{
#define X86_64_UFISPACE_S9510_28DC_ENUMERATION_ENTRY(_enum_name, _desc)     AIM_DATATYPE_MAP_REGISTER(_enum_name, _enum_name##_map, _desc,                               AIM_LOG_INTERNAL);
#include <x86_64_ufispace_s9510_28dc/x86_64_ufispace_s9510_28dc.x>
    return 0;
}

void __x86_64_ufispace_s9510_28dc_module_init__(void)
{
    AIM_LOG_STRUCT_REGISTER();
    datatypes_init__();
}

int __onlp_platform_version__ = 1;
