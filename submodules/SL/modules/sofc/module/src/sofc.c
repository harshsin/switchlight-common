/**************************************************************************//**
 *
 *
 *
 *
 *****************************************************************************/
#include <sofc/sofc_config.h>
#include <sofc/sofc.h>

#include <loci/loci.h>
#include <loci/of_message.h>
#include <loci/of_object.h>
#include <loci/loci_dump.h>
#include <loci/loci_obj_dump.h>
#include <NSS/nss.h>
#include <BigList/biglist.h>

#include "sofc_log.h"

/* sofc connection object */
struct sofc_cxn_s { 
    /** Session server for this connection */
    nss_t* nss; 
}; /* sofc_cxn_t* */

    
/* sofc controller object */
struct sofc_s { 
    /** Socket Server */
    nss_t* nss; 

    /** list of connections */
    biglist_t* connection_list; 

}; /* sofc_t */



static int sofc_connection_handshake__(sofc_t* sofc, sofc_cxn_t id); 

sofc_t* 
sofc_create(const char* interface, int port)
{
    sofc_t* rv = aim_zmalloc(sizeof(*rv)); 
    nss_create(&rv->nss, port, interface); 
    return rv; 
}

void 
sofc_destroy(sofc_t* sofc)
{
    nss_destroy(sofc->nss); 
    aim_free(sofc); 
}

static sofc_cxn_t 
sofc_connection_complete__(sofc_t* sofc)
{
    sofc_cxn_t cxn; 

    /* Create connection */
    cxn = aim_zmalloc(sizeof(*cxn));
    cxn->nss = nss_session_detach(sofc->nss); 
    
    if(sofc_connection_handshake__(sofc, cxn) < 0) { 
        AIM_LOG_MSG("Handshake error."); 
        nss_destroy(cxn->nss); 
        aim_free(cxn); 
        nss_stop(sofc->nss); 
        return NULL; 
    }

    sofc->connection_list = biglist_prepend(sofc->connection_list, cxn); 
    return cxn; 
}

sofc_cxn_t
sofc_connection_accept(sofc_t* sofc)
{
    /* Accept a connection on our server socket */
    nss_start(sofc->nss); 
    nss_blocking_set(sofc->nss, 1); 
    AIM_LOG_MSG("Waiting for switch connection..."); 
    nss_accept(sofc->nss); 
    AIM_LOG_MSG("Connected."); 
    return sofc_connection_complete__(sofc);
}

sofc_cxn_t
sofc_connection_connect(sofc_t* sofc)
{
    /* Connect to the given target */
    nss_blocking_set(sofc->nss, 1);
    AIM_LOG_MSG("Connecting..."); 
    nss_connect_blocking(sofc->nss); 
    AIM_LOG_MSG("Connected."); 
    return sofc_connection_complete__(sofc); 
}
    
int
sofc_connection_close(sofc_t* sofc, sofc_cxn_t id)
{
    sofc->connection_list = biglist_remove(sofc->connection_list, id); 
    nss_destroy(id->nss); 
    aim_free(id); 
    return 0; 
}

int sofc_mh_echo_request(sofc_t* sofc, sofc_cxn_t id, of_echo_request_t* echo); 

int
sofc_message_handle(sofc_t* sofc, sofc_cxn_t id, of_object_t* obj)
{
    switch(obj->object_id)
        {
        case OF_ECHO_REQUEST:
            {
                sofc_mh_echo_request(sofc, id, (of_echo_request_t*)obj); 
                return 1; 
            }
        default:
            return 0; 
        }
}



int
sofc_message_recv(sofc_t* sofc, sofc_cxn_t id, uint8_t* msg)
{
    int msg_bytes;
    int rv;
    uint8_t* p; 

    /* Read the message header */
    p = msg; 
    rv = nss_read_all(id->nss, p, OF_MESSAGE_HEADER_LENGTH); 
    if(rv != OF_MESSAGE_HEADER_LENGTH) {
        AIM_LOG_ERROR("nss_read_all() returned %d\n", rv); 
        return -1; 
    }
    p += rv; 

    /* Read the message body */
    msg_bytes = of_message_length_get((of_message_t)msg); 
    
    rv = nss_read_all(id->nss, p, 
                      msg_bytes - OF_MESSAGE_HEADER_LENGTH); 
    
    if(rv != (msg_bytes - OF_MESSAGE_HEADER_LENGTH)) { 
        AIM_LOG_ERROR("nss_read_all() returned %d\n", rv); 
        return -1;
    }
    return msg_bytes; 
}


int
sofc_message_recv_new(sofc_t* sofc, sofc_cxn_t id, uint8_t** msg)
{
    *msg = aim_zmalloc(OF_WIRE_BUFFER_MAX_LENGTH); 
    return sofc_message_recv(sofc, id, *msg); 
}

int
sofc_message_send(sofc_t* sofc, sofc_cxn_t id, of_object_t* obj)
{
    uint8_t* data; 
    int len = obj->length;
    of_object_wire_buffer_steal(obj, &data); 
    return nss_write(id->nss, data, len); 
}

static int
sofc_connection_handshake__(sofc_t* sofc, sofc_cxn_t id)
{
    uint8_t msg[OF_WIRE_BUFFER_MAX_LENGTH]; 

    /* Send Hello Msg */
    of_hello_t* hello; 
    hello = of_hello_new(OF_VERSION_1_0); 
    sofc_message_send(sofc, id, hello); 

    /* Wait for Hello Msg */    
    sofc_message_recv(sofc, id, msg); 
    
    /* Send Features Request */
    of_features_request_t* frq = of_features_request_new(OF_VERSION_1_0); 
    sofc_message_send(sofc, id, frq); 
    
    /* Wait for features reply */
    sofc_message_recv(sofc, id, msg); 
    return 0; 
}

int
sofc_mh_echo_request(sofc_t* sofc, sofc_cxn_t id, of_echo_request_t* echo)
{
    of_echo_request_t* reply = of_echo_reply_new((echo->version)); 
    of_octets_t data; 
    uint32_t xid; 

    of_echo_request_xid_get(echo, &xid);
    of_echo_request_data_get(echo, &data); 
    if(data.bytes > 0) { 
        if(of_echo_reply_data_set(reply, &data) < 0) { 
        }
    }
    of_echo_reply_xid_set(reply, xid); 
    return sofc_message_send(sofc, id, reply); 
}

int 
sofc_messages_dump(sofc_t* sofc, sofc_cxn_t id, aim_pvs_t* pvs)
{
    uint8_t msg[OF_WIRE_BUFFER_MAX_LENGTH]; 
    
    for(;;) { 
        int len; 
        AIM_LOG_MSG("Waiting for msg..."); 
        len = sofc_message_recv(sofc, id, msg); 
        AIM_LOG_MSG("Msg received (%d bytes)", len); 

        if(len > 0) { 
            of_object_t* obj; 
            obj = of_object_new_from_message(OF_BUFFER_TO_MESSAGE(msg), len);

            if (obj == NULL) {
                AIM_LOG_ERROR("Could not parse msg to OF object, len %d", len);
                return -1; 
            }
            of_object_dump((loci_writer_f)aim_printf, pvs, obj);
            sofc_message_handle(sofc, id, obj); 
        }
        else {
            AIM_LOG_ERROR("sofc_read_message() failed: %d\n", len); 
            return -1; 
        }
    }
    nss_stop(sofc->nss); 
    return 0; 
}
    
int 
sofc_message_type_recv(sofc_t* sofc, sofc_cxn_t id, int type,
                       uint8_t* msg)
{
    for(;;) { 
        int len; 
        len = sofc_message_recv(sofc, id, msg); 

        if(len > 0) { 
            of_object_t* obj; 
            obj = of_object_new_from_message(OF_BUFFER_TO_MESSAGE(msg), len);

            if (obj == NULL) {
                AIM_LOG_ERROR("Could not parse msg to OF object, len %d", len);
                return -1; 
            }
            sofc_message_handle(sofc, id, obj); 
            if(obj->object_id == type) { 
                return 0; 
            }
        }
        else {
            AIM_LOG_ERROR("sofc_read_message() failed: %d\n", len); 
            return -1; 
        }
    }
}

