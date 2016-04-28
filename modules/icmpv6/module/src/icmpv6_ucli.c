/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <icmpv6/icmpv6_config.h>

#if ICMPV6_CONFIG_INCLUDE_UCLI == 1

#include <uCli/ucli.h>
#include <uCli/ucli_argparse.h>
#include <uCli/ucli_handler_macros.h>

/* <auto.ucli.handlers.start> */
/* <auto.ucli.handlers.end> */

static ucli_command_handler_f icmpv6_ucli_ucli_handlers__[] = 
{
    NULL
};

static ucli_module_t
icmpv6_ucli_module__ =
    {
        "icmpv6_ucli",
        NULL,
        icmpv6_ucli_ucli_handlers__,
        NULL,
        NULL,
    };

ucli_node_t*
icmpv6_ucli_node_create(void)
{
    ucli_node_t* n;
    ucli_module_init(&icmpv6_ucli_module__);
    n = ucli_node_create("icmpv6", NULL, &icmpv6_ucli_module__);
    ucli_node_subnode_add(n, ucli_module_log_node_create("icmpv6"));
    return n;
}

#else
void*
icmpv6_ucli_node_create(void)
{
    return NULL;
}
#endif

