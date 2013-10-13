/**************************************************************************//**
 * 
 * 
 * 
 *****************************************************************************/
#include <sofc/sofc_config.h>

#if SOFC_CONFIG_INCLUDE_UCLI == 1

#include <uCli/ucli.h>
#include <uCli/ucli_argparse.h>
#include <uCli/ucli_handler_macros.h>

static ucli_status_t 
sofc_ucli_ucli__config__(ucli_context_t* uc)
{
    UCLI_HANDLER_MACRO_MODULE_CONFIG(sofc)
}

/* <auto.ucli.handlers.start> */
/* <auto.ucli.handlers.end> */

static ucli_module_t
sofc_ucli_module__ = 
    {
        "sofc_ucli", 
        NULL,  
        sofc_ucli_ucli_handlers__, 
        NULL, 
        NULL, 
    };

ucli_node_t*
sofc_ucli_node_create(void)
{
    ucli_node_t* n; 
    ucli_module_init(&sofc_ucli_module__); 
    n = ucli_node_create("sofc", NULL, &sofc_ucli_module__); 
    ucli_node_subnode_add(n, ucli_module_log_node_create("sofc")); 
    return n; 
}

#else
void*
sofc_ucli_node_create(void)
{
    return NULL; 
}
#endif

