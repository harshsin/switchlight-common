/**************************************************************************//**
 *
 * @file
 * @brief IVS uCli Definitions. 
 *
 *
 * @addtogroup ivs-ucli
 * @{
 *
 *
 *****************************************************************************/
#ifndef __IVS_UCLI_H__
#define __IVS_UCLI_H__

#include <IVS/ivs_config.h>
#include <IVS/ivs.h>

#include <uCli/ucli.h>

/**
 * @brief Get the configuration ucli node. 
 */
ucli_node_t* ivs_ucli_node_create(ivs_t* ivs); 

#endif /* __IVS_UCLI_H__ */
/*@}*/
