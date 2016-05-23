/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <collecta/collecta_config.h>

#if COLLECTA_CONFIG_INCLUDE_UCLI == 1

#include <uCli/ucli.h>
#include <uCli/ucli_argparse.h>
#include <uCli/ucli_handler_macros.h>

static ucli_status_t
collecta_ucli_ucli__config__(ucli_context_t* uc)
{
    UCLI_HANDLER_MACRO_MODULE_CONFIG(collecta)
}

/* <auto.ucli.handlers.start> */
/* <auto.ucli.handlers.end> */

static ucli_module_t
collecta_ucli_module__ =
    {
        "collecta_ucli",
        NULL,
        collecta_ucli_ucli_handlers__,
        NULL,
        NULL,
    };

ucli_node_t*
collecta_ucli_node_create(void)
{
    ucli_node_t* n;
    ucli_module_init(&collecta_ucli_module__);
    n = ucli_node_create("collecta", NULL, &collecta_ucli_module__);
    ucli_node_subnode_add(n, ucli_module_log_node_create("collecta"));
    return n;
}

#else
void*
collecta_ucli_node_create(void)
{
    return NULL;
}
#endif

