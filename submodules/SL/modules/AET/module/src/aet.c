/*
 * Asynchronous event threading
 *
 * For platforms that generate packet-in and/or port-status change
 * notifications in a different thread than the core thread (socket
 * manager context), this module provides an interface to connect
 * the two.
 *
 * An eventfd object is used in the module as the signaling mechanism.
 * This fd is registered with SocketManager.  The handler is implemented
 * in this module and it calls the indigo_core handlers for packet-in
 * or port-status change as appropriate.
 *
 * A queue of packet-in messages is maintained and enqueues are done
 * in the platform thread.  The entire array of pointers is copied out
 * in the SocketManager queue context.
 *
 * @TODO Support packet-in priority.  Might be as simple as checking the
 * packet priority, but could also support a map from other data.
 *
 * For port status changes, an array indexed by port is maintained which
 * holds the last update message.
 *
 * @TODO Keep more state related to port status changes that arrive.  For
 * example, record if a port-down notification is received and expose that
 * to the core in case, for example, flows to that port are to be cleared.
 * Also, might keep a count of missed messages to detect link flapping.
 */

#include <AET/aet_config.h>
#include <AET/aet.h>

#include <SocketManager/socketmanager.h>
#include <OFStateManager/ofstatemanager.h>

#include <indigo/of_state_manager.h>
#include <indigo/assert.h>

#include <OS/os_sem.h>

#include <unistd.h>
#include <sys/eventfd.h>

#include "aet_log.h"

/* Short hand logging macros */
#define LOG_ERROR AIM_LOG_ERROR
#define LOG_WARN AIM_LOG_WARN 
#define LOG_INFO AIM_LOG_INFO
#define LOG_VERBOSE AIM_LOG_VERBOSE
#define LOG_TRACE AIM_LOG_TRACE

typedef struct aet_state_s {
    int init_done;
    int enabled;

    /* Double buffering of queues/status */
    of_packet_in_t **pkt_in_queue; /* Active queue on forwarding side */
    int pkt_in_queue_max;
    int pkt_in_queue_current;      /* Enqueue pointer on forwarding side */

    of_packet_in_t **pkt_in_process; /* Queue on state manager side */
    int pkt_in_process_next_idx;  /* Dequeue pointer on state mgr side */
    int pkt_in_process_count;     /* Number of buffers in process queue */
    uint32_t pkt_in_drops;

    of_port_status_t **port_status; /* Indexed by OF port number */
    of_port_status_t **port_status_process; /* Transfer when processing */
    int port_max;
    int port_message_received;
    uint32_t port_message_drops;

    int fd;

    os_sem_t sem;
} aet_state_t;

static aet_state_t as;

#define AS_LOCK os_sem_take(as.sem);
#define AS_UNLOCK os_sem_give(as.sem);


#define INIT_CHECK do {                             \
        if (!as.init_done) {                        \
            LOG_ERROR("AET: Not initialized\n");    \
            return INDIGO_ERROR_INIT;               \
        }                                           \
    } while(0)

#define ENABLED_CHECK(_cleanup_expr) do {           \
        if (!as.init_done) {                        \
            LOG_ERROR("AET: Not initialized\n");    \
            _cleanup_expr ;                         \
            return;                                 \
        }                                           \
        if (!as.enabled) {                          \
            LOG_TRACE("AET: Not enabled\n");        \
            _cleanup_expr ;                         \
            return;                                 \
        }                                           \
    } while(0)


static inline void
notify(void)
{
    uint64_t wbuf = 1;

    write(as.fd, &wbuf, sizeof(wbuf));
}

/**
 * @brief The handler for when the socket is ready for a read
 */

static void
aet_socket_ready_handler(int socket_id, void *cookie, int read_ready,
                         int write_ready, int error_seen)
{
    of_port_status_t **tmp_ports;
    of_packet_in_t **tmp_pkts;
    int port_message_received;
    uint64_t rbuf;
    int bytes;
    int i;

    LOG_TRACE("AET: socket_ready. rd %d. wr %d. err %d", read_ready, write_ready,
              error_seen);

    if (read_ready) {
        /* Read the socket to clear it */
        read(as.fd, &rbuf, sizeof(rbuf));
        AS_LOCK;
        /* Only swap "current" and "process" if process queue is empty */
        if (as.pkt_in_process_count == 0) {
            if ((as.pkt_in_process_count = as.pkt_in_queue_current)) {
                /* Swap pointers */
                tmp_pkts = as.pkt_in_process;
                as.pkt_in_process = as.pkt_in_queue;
                as.pkt_in_queue = tmp_pkts;
                as.pkt_in_queue_current = 0;
                as.pkt_in_process_next_idx = 0;
            }
        }
        if ((port_message_received = as.port_message_received)) {
            /* Swap pointers */
            tmp_ports = as.port_status_process;
            as.port_status_process = as.port_status;
            as.port_status = tmp_ports;
            /* Assume port_status_process was zeroed below before transfer */
            as.port_message_received = 0;
        }
        AS_UNLOCK;

        /* Now process the packet in and port status messages */
        if (as.pkt_in_process_count > 0) { /* Packet in to process */
            int idx;

            LOG_TRACE("processing packet-in msgs, %d remain",
                      as.pkt_in_process_count);
            idx = as.pkt_in_process_next_idx++;
            INDIGO_ASSERT(idx >= 0 && idx < as.pkt_in_queue_max);

            (void)indigo_core_packet_in(as.pkt_in_process[idx]);
            as.pkt_in_process[idx] = NULL; 
            if (--as.pkt_in_process_count || as.pkt_in_queue_current) {
                /* If still pkts in process or current queue, notify */
                notify();
            }
        }

        if (port_message_received) {
            LOG_TRACE("processing port status msg(s)");
            for (i = 0; i < as.port_max; i++) {
                if (as.port_status_process[i] != NULL) {
                    indigo_core_port_status_update(as.port_status_process[i]);
                }
            }

            /* Clear the status buffers */
            bytes = sizeof(of_port_status_t *) * as.port_max;
            AET_MEMSET(as.port_status_process, 0, bytes);
        }
    } else {
        LOG_WARN("AET: socket_ready but not readable. wr %d. err %d",
                 write_ready, error_seen);
    }
}


/**
 * Initialize the AET module
 */

indigo_error_t
aet_init(aet_config_t *config)
{
    int bytes;

    LOG_TRACE("AET: init");
    if (as.init_done) {
        LOG_ERROR("AET init: called when init done");
        return INDIGO_ERROR_EXISTS;
    }

    if (config == NULL) {
        LOG_ERROR("AET init: No config");
        return INDIGO_ERROR_PARAM;
    }

    if (config->pkt_in_queue_max > 0) {
        bytes = sizeof(of_packet_in_t *) * config->pkt_in_queue_max;
        as.pkt_in_queue = AET_MALLOC(bytes);
        AIM_TRUE_OR_DIE(as.pkt_in_queue != NULL);
        AET_MEMSET(as.pkt_in_queue, 0, bytes);

        as.pkt_in_process = AET_MALLOC(bytes);
        AIM_TRUE_OR_DIE(as.pkt_in_process != NULL);
        AET_MEMSET(as.pkt_in_process, 0, bytes);
        as.pkt_in_process_count = 0;     /* Empty process queue */

        as.pkt_in_queue_max = config->pkt_in_queue_max;
        as.pkt_in_queue_current = 0;
    }

    if (config->port_max > 0) {
        bytes = sizeof(of_port_status_t *) * config->port_max;
        as.port_status = AET_MALLOC(bytes);
        AIM_TRUE_OR_DIE(as.port_status != NULL);
        AET_MEMSET(as.port_status, 0, bytes);

        as.port_status_process = AET_MALLOC(bytes);
        AIM_TRUE_OR_DIE(as.port_status_process != NULL);
        AET_MEMSET(as.port_status_process, 0, bytes);

        as.port_max = config->port_max;
        as.port_message_received = 0;
    }

    as.fd = eventfd(0, 0);
    LOG_TRACE("AET: as fd is %d", as.fd);
    AIM_TRUE_OR_DIE(as.fd >= 0);

    as.sem = os_sem_create(1);
    AIM_TRUE_OR_DIE(as.sem != NULL);

    as.init_done = 1;

    return INDIGO_ERROR_NONE;
}

/**
 * Finish AET Module
 */
static void 
port_status_delete_all__(of_port_status_t **port_status)
{
    int i;
    for(i = 0; i < as.port_max; i++) { 
        if(port_status[i] != NULL) { 
            of_port_status_delete(port_status[i]); 
            port_status[i] = NULL; 
        }
    }
}

static void
pkt_in_queue_delete_all__(of_packet_in_t** pkt_in_queue)
{
    int i;
    for(i = 0; i < as.pkt_in_queue_max; i++) { 
        if(pkt_in_queue[i] != NULL) { 
            of_packet_in_delete(pkt_in_queue[i]);
            pkt_in_queue[i] = NULL; 
        }
    }
}

indigo_error_t 
aet_finish(void)
{
    if(as.init_done) { 
        close(as.fd);
        os_sem_destroy(as.sem); 
        
        /* Deallocate any pending status messages. */
        port_status_delete_all__(as.port_status); 
        port_status_delete_all__(as.port_status_process); 
        AET_FREE(as.port_status); 
        AET_FREE(as.port_status_process); 
        
        /* Deallocate any pending pkt messages */
        pkt_in_queue_delete_all__(as.pkt_in_queue); 
        pkt_in_queue_delete_all__(as.pkt_in_process); 
        AET_FREE(as.pkt_in_queue); 
        AET_FREE(as.pkt_in_process); 
        as.init_done = 0; 
    }
    return INDIGO_ERROR_NONE; 
}

/**
 * Enable/disable the agent
 */
indigo_error_t
aet_enable_set(int enable)
{
    int rv;

    INIT_CHECK;

    LOG_TRACE("AET: Enable set to %d", enable);

    if (enable) {
        rv = ind_soc_socket_register_with_priority(
            as.fd, aet_socket_ready_handler, NULL, IND_SOC_LOWEST_PRIORITY);
        if (rv < 0) {
            LOG_ERROR("AET: Could not register socket on enable");
            return rv;
        }
        as.enabled = 1;
    } else {
        (void)ind_soc_socket_unregister(as.fd);
        as.enabled = 0;
    }

    return INDIGO_ERROR_NONE;
}


/**
 * Enqueue a packet in message
 * @param packet_in Pointer to the packet in message
 * @param prio Ignored for now; eventually priority of pkt when dequeuing
 *
 * Always takes ownership of packet_in object.
 */

void
aet_packet_in_enqueue(of_packet_in_t *packet_in, int prio)
{
    (void)prio; /* Unused */

    ENABLED_CHECK(of_packet_in_delete(packet_in));

    AS_LOCK;
    if (as.pkt_in_queue_current >= as.pkt_in_queue_max) {
        AS_UNLOCK;
        as.pkt_in_drops += 1;
        LOG_TRACE("AET: Packet in, but queue full");
        of_packet_in_delete(packet_in);
        return;
    }

    as.pkt_in_queue[as.pkt_in_queue_current] = packet_in;
    as.pkt_in_queue_current += 1;
    AS_UNLOCK;

    notify();
}

/**
 * Enqueue a port status update message
 * @param port_status Pointer to the port status object
 * @param of_port Port number for which the message was generated
 *
 * Always takes ownership of port_status object
 */

void
aet_port_status_enqueue(of_port_status_t *port_status, of_port_no_t of_port)
{
    of_port_status_t *old_status = NULL;

    ENABLED_CHECK(of_port_status_delete(port_status)); 

    AS_LOCK;
    if (of_port >= as.port_max) {
        AS_UNLOCK;
        LOG_ERROR("AET: Bad port %d", (int)of_port);
        of_port_status_delete(port_status);
        return;
    }

    if (as.port_status[(int)of_port] != NULL) {
        old_status = as.port_status[(int)of_port];
    }
    as.port_status[(int)of_port] = port_status;
    as.port_message_received += 1;
    AS_UNLOCK;

    if (old_status != NULL) {
        as.port_message_drops += 1;
        of_port_status_delete(old_status);
    }
    notify();
}
