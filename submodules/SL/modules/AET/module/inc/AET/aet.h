#include <indigo/error.h>
#include <loci/loci.h>

typedef struct aet_config_s {
    int pkt_in_queue_max; /* Depth of packet in queue */
    int port_max; /* Number of ports to support */
} aet_config_t;

void aet_port_status_enqueue(of_port_status_t *port_status,
                                    of_port_no_t of_port);
void aet_packet_in_enqueue(of_packet_in_t *packet_in, int prio);

indigo_error_t aet_init(aet_config_t *config);
indigo_error_t aet_finish(void); 

indigo_error_t aet_enable_set(int enable);


