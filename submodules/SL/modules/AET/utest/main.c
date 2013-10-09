/**************************************************************************//**
 * 
 * 
 * 
 *****************************************************************************/
#include <AET/aet_config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AIM_LOG_MODULE_NAME aet
#include <AIM/aim.h>
#include <AIM/aim_log.h>

#include <SocketManager/socketmanager.h>
#include <OFStateManager/ofstatemanager.h>
#include <AET/aet.h>

#include <indigo/of_state_manager.h>
#include <indigo/assert.h>
#include <indigo/memory.h>

#define OK(op)  INDIGO_ASSERT((op) == INDIGO_ERROR_NONE)

static ind_soc_socket_ready_callback_f soc_cb = NULL;
static void * soc_cookie = NULL;
static int soc_fd;
static int pkt_seen;
static int port_seen;

indigo_error_t
ind_soc_socket_unregister(int socket_id)
{
    printf("Unregister called\n");
    INDIGO_ASSERT(socket_id == soc_fd);
    soc_fd = -1;

    return INDIGO_ERROR_NONE;
}

indigo_error_t
ind_soc_socket_register_with_priority(
    int socket_id,
    ind_soc_socket_ready_callback_f callback,
    void *cookie,
    int priority)
{
    printf("Register called\n");
    soc_cb = callback;
    soc_cookie = cookie;
    soc_fd = socket_id;

    return INDIGO_ERROR_NONE;
}

indigo_error_t
indigo_core_packet_in(of_packet_in_t *packet_in)
{
    printf("Packet in update called with %p\n", packet_in);
    pkt_seen += 1;
    of_packet_in_delete(packet_in);
    return INDIGO_ERROR_NONE;
}

void
indigo_core_port_status_update(of_port_status_t *of_port_status)
{
    printf("port status update called with %p\n", of_port_status);
    port_seen += 1;
    of_port_status_delete(of_port_status);
}

int aim_main(int argc, char* argv[])
{
    aet_config_t aet_config;
    int i;

    aet_config.pkt_in_queue_max = 16;
    aet_config.port_max = 16;

    aet_config_show(&aim_pvs_stdout);

    OK(aet_init(&aet_config));
    OK(aet_enable_set(1));
    INDIGO_ASSERT(soc_cb != NULL);
    aet_packet_in_enqueue(of_packet_in_new(OF_VERSION_1_0), 0);
    aet_port_status_enqueue(of_port_status_new(OF_VERSION_1_0), 3);

    INDIGO_ASSERT(pkt_seen == 0);
    INDIGO_ASSERT(port_seen == 0);
    soc_cb(soc_fd, soc_cookie, 1, 0, 0);
    INDIGO_ASSERT(pkt_seen);
    INDIGO_ASSERT(port_seen);

    /********/
    pkt_seen = 0;
    port_seen = 0;

    aet_packet_in_enqueue(of_packet_in_new(OF_VERSION_1_0), 0);
    aet_packet_in_enqueue(of_packet_in_new(OF_VERSION_1_0), 0);
    aet_packet_in_enqueue(of_packet_in_new(OF_VERSION_1_0), 0);

    INDIGO_ASSERT(pkt_seen == 0);
    INDIGO_ASSERT(port_seen == 0);
    soc_cb(soc_fd, soc_cookie, 1, 0, 0);
    soc_cb(soc_fd, soc_cookie, 1, 0, 0);
    soc_cb(soc_fd, soc_cookie, 1, 0, 0);
    INDIGO_ASSERT(pkt_seen == 3);
    INDIGO_ASSERT(port_seen == 0);

    /********/
    pkt_seen = 0;
    port_seen = 0;

    aet_port_status_enqueue(of_port_status_new(OF_VERSION_1_0), 3);
    /* Same as above */
    aet_port_status_enqueue(of_port_status_new(OF_VERSION_1_0), 3);
    aet_port_status_enqueue(of_port_status_new(OF_VERSION_1_0), 4);

    INDIGO_ASSERT(pkt_seen == 0);
    INDIGO_ASSERT(port_seen == 0);
    soc_cb(soc_fd, soc_cookie, 1, 0, 0);
    INDIGO_ASSERT(pkt_seen == 0);
    INDIGO_ASSERT(port_seen == 2); /* Status called twice on same port */

    /********/
    pkt_seen = 0;
    port_seen = 0;

    /* Overflow buffer and check pkts discarded properly */
    for (i = 0; i < 20; i++) {
        aet_packet_in_enqueue(of_packet_in_new(OF_VERSION_1_0), 0);
    }

    INDIGO_ASSERT(pkt_seen == 0);
    INDIGO_ASSERT(port_seen == 0);
    for (i = 0; i < 16; i++) {
        soc_cb(soc_fd, soc_cookie, 1, 0, 0);
    }
    INDIGO_ASSERT(pkt_seen == 16);
    INDIGO_ASSERT(port_seen == 0);

    /********/
    OK(aet_enable_set(0));

    return 0;
}

