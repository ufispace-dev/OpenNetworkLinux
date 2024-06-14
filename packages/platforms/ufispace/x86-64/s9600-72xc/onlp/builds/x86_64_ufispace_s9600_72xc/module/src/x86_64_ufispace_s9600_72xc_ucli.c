/****************************************************************************
 *
 * x86_64_ufispace_s9600_72xc ucli
 *
 *****************************************************************************/
#include <x86_64_ufispace_s9600_72xc/x86_64_ufispace_s9600_72xc_config.h>

#if X86_64_UFISPACE_S9600_72XC_CONFIG_INCLUDE_UCLI == 1

#include <uCli/ucli.h>
#include <uCli/ucli_argparse.h>
#include <uCli/ucli_handler_macros.h>

static ucli_status_t
x86_64_ufispace_s9600_72xc_ucli_ucli__config__(ucli_context_t *uc)
{
    UCLI_HANDLER_MACRO_MODULE_CONFIG(x86_64_ufispace_s9600_72xc)
}

/* <auto.ucli.handlers.start> */
/* <auto.ucli.handlers.end> */

static ucli_module_t
x86_64_ufispace_s9600_72xc_ucli_module__ =
    {
        "x86_64_ufispace_s9600_72xc_ucli",
        NULL,
        x86_64_ufispace_s9600_72xc_ucli_ucli_handlers__,
        NULL,
        NULL,
    };

ucli_node_t *
x86_64_ufispace_s9600_72xc_ucli_node_create(void)
{
    ucli_node_t *n;
    ucli_module_init(&x86_64_ufispace_s9600_72xc_ucli_module__);
    n = ucli_node_create("x86_64_ufispace_s9600_72xc", NULL, &x86_64_ufispace_s9600_72xc_ucli_module__);
    ucli_node_subnode_add(n, ucli_module_log_node_create("x86_64_ufispace_s9600_72xc"));
    return n;
}

#else
void *
x86_64_ufispace_s9600_72xc_ucli_node_create(void)
{
    return NULL;
}
#endif
