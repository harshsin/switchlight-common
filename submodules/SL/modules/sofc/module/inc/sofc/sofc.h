/**************************************************************************//**
 *
 * @file
 * @brief sofc Simple OpenFlow Controller Module
 * @addtogroup sofc-sofc
 * @{
 *
 *****************************************************************************/
#ifndef __SOFC_H__
#define __SOFC_H__

#include <sofc/sofc_config.h>
#include <loci/of_message.h>
#include <loci/of_object.h>

/** Simple Openflow Controller Object */
typedef struct sofc_s sofc_t; 

/** Simple Openflow Connection Object */
typedef struct sofc_cxn_s* sofc_cxn_t; 

/**
 * @brief Create an controller object. 
 * @param interface The interface address. 
 * @param port The port number. 
 */
sofc_t* sofc_create(const char* interface, int port); 

/**
 * @brief Destroy a controller object. 
 * @param sofc The controller object. 
 */
void sofc_destroy(sofc_t* sofc);


/**
 * @brief Wait for a connection. 
 * @param sofc The controller object. 
 * @returns The connection handle. 
 */
sofc_cxn_t sofc_connection_accept(sofc_t* sofc); 

/**
 * @brief Connect to the given target. 
 * @param sofc The controller object. 
 * @returns The connection handle. 
 */
sofc_cxn_t sofc_connection_connect(sofc_t* sofc); 

/**
 * @brief Close a connection. 
 * @param sofc The controller object. 
 * @param id The connection handle
 */
int sofc_connection_close(sofc_t* sofc, sofc_cxn_t id); 

/**
 * @brief Receive a message. 
 * @param sofc The controller object. 
 * @param id The connection handle. 
 * @param msg Receives the message data. 
 * @returns The message size. 
 */
int sofc_message_recv(sofc_t* sofc, sofc_cxn_t id, uint8_t* msg);

/**
 * @brief Allocate and receive a message. 
 * @param sofc The controller object. 
 * @param id The connection handle. 
 * @param msg Receives the msg data buffer. 
 * @returns The message size. 
 */
int sofc_message_recv_new(sofc_t* sofc, sofc_cxn_t id, uint8_t** msg); 

/**
 * @brief Send a message. 
 * @param sofc The controller object.
 * @param id The connection handle. 
 * @param obj The object. 
 */
int sofc_message_send(sofc_t* sofc, sofc_cxn_t id,  of_object_t* obj); 

/**
 * @brief Dump incoming messages. 
 * @param sofc The controller object. 
 * @param id The connection handle. 
 * @param pvs The output pvs. 
 */
int sofc_messages_dump(sofc_t* sofc, sofc_cxn_t id, aim_pvs_t* pvs); 

/**
 * @brief Get a message of the given type. 
 * @param sofc The controller object. 
 * @param id The connection handle. 
 * @param type The object type. 
 * @param msg The message buffer. 
 * @returns The message size. 
 */
int sofc_message_type_recv(sofc_t* sofc, sofc_cxn_t id, int type,
                           uint8_t* msg);


#endif /* __SOFC_H__ */
/* @} */

