/************************************************************
 * <bsn.cl fy=2014 v=onl>
 * 
 *        Copyright 2014, 2015 Big Switch Networks, Inc.       
 * 
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 * 
 *        http://www.eclipse.org/legal/epl-v10.html
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 * 
 * </bsn.cl>
 ************************************************************
 *
 *
 *
 ***********************************************************/

#include <x86_64_ufispace_s9600_64x/x86_64_ufispace_s9600_64x_config.h>

#if X86_64_UFISPACE_S9600_64X_CONFIG_INCLUDE_UCLI == 1

#include <uCli/ucli.h>
#include <uCli/ucli_argparse.h>
#include <uCli/ucli_handler_macros.h>

static ucli_status_t
x86_64_ufispace_s9600_64x_ucli_ucli__config__(ucli_context_t* uc)
{
    UCLI_HANDLER_MACRO_MODULE_CONFIG(x86_64_ufispace_s9600_64x)
}

/* <auto.ucli.handlers.start> */
/******************************************************************************
 *
 * These handler table(s) were autogenerated from the symbols in this
 * source file.
 *
 *****************************************************************************/
static ucli_command_handler_f x86_64_ufispace_s9600_64x_ucli_ucli_handlers__[] =
{
    x86_64_ufispace_s9600_64x_ucli_ucli__config__,
    NULL
};
/******************************************************************************/
/* <auto.ucli.handlers.end> */

static ucli_module_t
x86_64_ufispace_s9600_64x_ucli_module__ =
    {
        "x86_64_ufispace_s9600_64x_ucli",
        NULL,
        x86_64_ufispace_s9600_64x_ucli_ucli_handlers__,
        NULL,
        NULL,
    };

ucli_node_t*
x86_64_ufispace_s9600_64x_ucli_node_create(void)
{
    ucli_node_t* n;
    ucli_module_init(&x86_64_ufispace_s9600_64x_ucli_module__);
    n = ucli_node_create("x86_64_ufispace_s9600_64x", NULL, &x86_64_ufispace_s9600_64x_ucli_module__);
    ucli_node_subnode_add(n, ucli_module_log_node_create("x86_64_ufispace_s9600_64x"));
    return n;
}

#else
void* x86_64_ufispace_s9600_64x_ucli_node_create(void)
{
    return NULL;
}
#endif

