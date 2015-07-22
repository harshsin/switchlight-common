/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <igmpa/igmpa_config.h>

#if IGMPA_CONFIG_INCLUDE_UCLI == 1

#include <uCli/ucli.h>
#include <uCli/ucli_argparse.h>
#include <uCli/ucli_handler_macros.h>

static ucli_status_t
igmpa_ucli_ucli__config__(ucli_context_t* uc)
{
    UCLI_HANDLER_MACRO_MODULE_CONFIG(igmpa)
}

/* <auto.ucli.handlers.start> */
/* <auto.ucli.handlers.end> */

static ucli_module_t
igmpa_ucli_module__ =
    {
        "igmpa_ucli",
        NULL,
        igmpa_ucli_ucli_handlers__,
        NULL,
        NULL,
    };

ucli_node_t*
igmpa_ucli_node_create(void)
{
    ucli_node_t* n;
    ucli_module_init(&igmpa_ucli_module__);
    n = ucli_node_create("igmpa", NULL, &igmpa_ucli_module__);
    ucli_node_subnode_add(n, ucli_module_log_node_create("igmpa"));
    return n;
}

#else
void*
igmpa_ucli_node_create(void)
{
    return NULL;
}
#endif

