#include <dependmodules.x>
#include <uCli/ucli.h>
#include <indigo/port_manager.h>
#include <indigo/error.h>
#include <PortManager/portmanager.h>
#include <Forwarding/forwarding.h>
#include <snmp/snmp_port_stats.h>

void
__ivd_module_init__(void)
{
}

ucli_node_t *
ivd_ucli_node_create(void)
{
    return NULL;
}

/**************
 * Forwarding *
 **************/

indigo_error_t
ind_fwd_init(ind_fwd_config_t *config)
{
    return INDIGO_ERROR_NONE;
}

indigo_error_t
ind_fwd_finish(void)
{
    return INDIGO_ERROR_NONE;
}

indigo_error_t
ind_fwd_enable_set(int enable)
{
    return INDIGO_ERROR_NONE;
}

indigo_error_t
indigo_fwd_packet_out(of_packet_out_t *packet_out)
{
    return INDIGO_ERROR_NONE;
}

indigo_error_t
indigo_fwd_expiration_enable_set(int is_enabled)
{
    return INDIGO_ERROR_NONE;
}

indigo_error_t
indigo_fwd_forwarding_features_get(of_features_reply_t *features)
{
    return INDIGO_ERROR_NONE;
}

void
indigo_fwd_pipeline_get(of_desc_str_t pipeline)
{
}

indigo_error_t
indigo_fwd_pipeline_set(of_desc_str_t pipeline)
{
    return INDIGO_ERROR_NONE;
}

void
indigo_fwd_pipeline_stats_get(of_desc_str_t **pipelines,
                              int *num_pipelines)
{
}

indigo_error_t
indigo_fwd_experimenter(of_experimenter_t *experimenter,
                        indigo_cxn_id_t cxn_id)
{
    return INDIGO_ERROR_NONE;
}

/********
 * Port *
 *******/

indigo_error_t
ind_port_init(ind_port_config_t *config)
{
    return INDIGO_ERROR_NONE;
}

indigo_error_t
ind_port_finish(void)
{
    return INDIGO_ERROR_NONE;
}

indigo_error_t
ind_port_enable_set(int enable)
{
    return INDIGO_ERROR_NONE;
}

indigo_error_t
indigo_port_modify(of_port_mod_t *port_mod)
{
    return INDIGO_ERROR_NONE;
}

indigo_error_t
indigo_port_interface_add(indigo_port_name_t if_name,
                          of_port_no_t of_port,
                          indigo_port_config_t *config)
{
    return INDIGO_ERROR_NONE;
}

indigo_error_t
indigo_port_interface_remove(indigo_port_name_t if_name)
{
    return INDIGO_ERROR_NONE;
}

indigo_error_t
indigo_port_interface_list(indigo_port_info_t **list)
{
    return INDIGO_ERROR_NONE;
}

void
indigo_port_interface_list_destroy(indigo_port_info_t *list)
{
}

indigo_error_t
indigo_port_features_get(of_features_reply_t *features)
{
    return INDIGO_ERROR_NONE;
}

indigo_error_t
indigo_port_desc_stats_get(of_port_desc_stats_reply_t *port_desc_stats_reply)
{
    return INDIGO_ERROR_NONE;
}

indigo_error_t
indigo_port_stats_get(of_port_stats_request_t *port_stats_request,
                      of_port_stats_reply_t **port_stats_reply_ptr)
{
    return INDIGO_ERROR_NONE;
}

indigo_error_t
indigo_port_queue_config_get(of_queue_get_config_request_t *request,
                             of_queue_get_config_reply_t **reply_ptr)
{
    return INDIGO_ERROR_NONE;
}

indigo_error_t
indigo_port_queue_stats_get(of_queue_stats_request_t *request,
                            of_queue_stats_reply_t **reply_ptr)
{
    return INDIGO_ERROR_NONE;
}

indigo_error_t
indigo_port_experimenter(of_experimenter_t *experimenter,
                         indigo_cxn_id_t cxn_id)
{
    return INDIGO_ERROR_NONE;
}

/********
 * SNMP *
 *******/

int
snmp_port_stats_get(int port,
                    snmp_port_stats_t *stats)
{
    return 0;
}

uint64_t
snmp_flow_table_l2_count_max_get(void)
{
    return 0;
}

uint64_t
snmp_flow_table_l2_count_free_get(void)
{
    return 0;
}

uint64_t
snmp_flow_table_full_match_count_max_get(void)
{
    return 0;
}

uint64_t
snmp_flow_table_full_match_count_free_get(void)
{
    return 0;
}
