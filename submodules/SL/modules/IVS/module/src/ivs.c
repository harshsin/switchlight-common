/******************************************************************************
 *
 *  /module/src/ivs.c
 *
 *
 *****************************************************************************/
#include <IVS/ivs_config.h> 
#include <IVS/ivs.h> 
#include <IVS/uCli/ivs_ucli.h>
#include <Configuration/configuration.h>
#include <loci/loci.h>
#include "ivs_int.h" 
#include "ivs_util.h"
#include "ivs_log.h"

#include <dependmodules.x>

#include <signal.h>

/**
 * Try an operation and return the error code on failure.
 */
#define TRY(op)                                                         \
    do {                                                                \
        int _rv;                                                        \
        AIM_LOG_TRACE("%s", #op);                                       \
        if ((_rv = (op)) < 0) {                                         \
            AIM_LOG_ERROR("%s: ERROR %d at %s:%d",                      \
                               #op, _rv, __FILE__, __LINE__);           \
            return _rv;                                                 \
        }                                                               \
    } while (0)


#if IVS_CONFIG_INCLUDE_CXN_LOG == 1
static void
ivs_cxn_state_log__(indigo_cxn_id_t              cxn_id,
                    indigo_cxn_protocol_params_t *cxn_proto_params,
                    indigo_cxn_state_t           cxn_state,
                    void                         *cookie); 
#endif

#if IVS_CONFIG_INCLUDE_NETWORK_CLI == 1
static void ivs_nss_callback__(void*); 
#endif

#if IVS_CONFIG_INCLUDE_CONSOLE_CLI == 1
#if ELS_CONFIG_INCLUDE_EVENTFD == 1
static void ivs_els_eventfd_callback__(int socket_id, void *cookie, 
                                       int read_ready, int write_ready, 
                                       int error_seen); 
#endif /* ELS_CONFIG_INCLUDE_EVENTFD */
static void ivs_els_callback__(void*);
#endif /* IVS_CONFIG_INCLUDE_CONSOLE_CLI */

static int
ivs_option__(ivs_t* ivs, ivs_option_t* options, char* type)
{
    ivs_option_t* op;   
    int rv = 0; 

    if(options) { 
        for(op = options; op->string && rv >= 0; op++) { 
            if(op->processed) { 
                continue; 
            }
            if(type == NULL ||
               !IVS_STRNCMP(op->string, type, IVS_STRLEN(type))) { 
                aim_printf(&aim_pvs_stdout, "[ %s ]\n", op->string); 
                rv = ucli_dispatch_string(ivs->ucli, &aim_pvs_stdout, op->string); 
                ucli_reset(ivs->ucli); 
                op->processed = 1; 
            }
        }
    }
    return rv; 
}

static int
ivs_options__(ivs_t* ivs, char* type)
{
    /**
     * argv options have higher priority than string options. 
     */
    TRY(ivs_option__(ivs, ivs->string_options, type)); 
    TRY(ivs_option__(ivs, ivs->argv_options, type)); 
    return 0; 
}

static void
ivs_options_init__(ivs_option_t** dst, char** strings)
{
    if(strings) { 
        int i = 0;
        while(strings[i]) i++;
        *dst = ivs_zmalloc(sizeof(ivs_option_t)*(i+1)); 
        for(i = 0; strings[i]; i++) { 
            (*dst)[i].string = aim_strdup(strings[i]); 
        }
    }
}

static void
ivs_options_free__(ivs_option_t* opts)
{
    if(opts) { 
        int i; 
        for(i = 0; opts[i].string; i++) { 
            IVS_FREE(opts[i].string); 
        }
        IVS_FREE(opts); 
    }
}

int
ivs_create(ivs_t** rv, char** string_options, char** argv_options)
{
    ivs_t* ivs = ivs_zmalloc(sizeof(*ivs)); 
    ucli_node_t* n; 

    if(ivs == NULL) { 
        IVS_ERROR("allocation failed."); 
        return -1; 
    }

    /**
     * Initialize all defaults. 
     */
    memset(&ivs->port, 0, sizeof(ivs->port));
    ivs->port.of_version = OF_VERSION_1_3; 
    ivs->port.max_ports = IVS_CONFIG_PORT_MAX_PORTS_DEFAULT; 
    
    memset(&ivs->fwd, 0, sizeof(ivs->fwd));
    ivs->fwd.of_version = OF_VERSION_1_3; 
    ivs->fwd.max_flows = IVS_CONFIG_FWD_MAX_FLOWS_DEFAULT; 
    
    memset(&ivs->core, 0, sizeof(ivs->core));
    ivs->core.expire_flows = IVS_CONFIG_CORE_EXPIRE_FLOWS_DEFAULT;
    ivs->core.stats_check_ms = IVS_CONFIG_CORE_STATS_CHECK_MS_DEFAULT;
    ivs->core.max_flowtable_entries = IVS_CONFIG_FWD_MAX_FLOWS_DEFAULT;
    
    memset(&ivs->port_add_config, 0, sizeof(ivs->port_add_config));
    ivs->port_add_config.disable_on_add = 0;

    ivs->interface_counter = 0;


    ivs_options_init__(&ivs->string_options, string_options); 
    if(argv_options) {
        char** argv_strings = ucli_argv_to_strings(argv_options); 
        ivs_options_init__(&ivs->argv_options, argv_strings); 
        ucli_argv_to_strings_free(argv_strings); 
    }

    n = ivs_ucli_node_create(ivs); 
    ivs->ucli = ucli_create("ivs", NULL, n);

    /* fixme */
    if(argv_options && argv_options[0] && !(IVS_STRCMP(argv_options[0], "-help") && IVS_STRCMP(argv_options[0], "-h"))) { 
        aim_printf(&aim_pvs_stdout, "Options: \n"); 
        ucli_group_help(ivs->ucli, "argv", &aim_pvs_stdout, "    -"); 
        exit(0);        
    }

#if IVS_CONFIG_INCLUDE_WATCHDOG == 1
    ivs->watchdog_seconds = IVS_CONFIG_WATCHDOG_SECONDS;
#endif

    TRY(ivs_options__(ivs, "e ")); 
    TRY(ivs_options__(ivs, "min ")); 
    TRY(ivs_options__(ivs, "name "));
    TRY(ivs_options__(ivs, "wd")); 

#if IVS_CONFIG_INCLUDE_CXN_LOG == 1
    ivs->cxn_log_ring = bigring_create(IVS_CONFIG_CXN_LOG_SIZE, bigring_aim_free_entry); 
#endif

#if IVS_CONFIG_INCLUDE_STATUS_LOG == 1
    ivs->status_log_ring = bigring_create(IVS_CONFIG_STATUS_LOG_SIZE,
                                          bigring_aim_free_entry);
#endif

    *rv = ivs; 
    return 0; 
}

#if IVS_CONFIG_INCLUDE_CONSOLE_CLI == 1

static int
ivs_console_prompt__(char* p, int size, void* cookie)
{
    ivs_t* ivs = (ivs_t*)cookie; 
    ucli_prompt_get(ivs->consif.ucli, p, size, 
                    IVS_CONFIG_CONSOLE_PROMPT_DEFAULT); 
    return 0; 
}

#endif
    
int 
ivs_init(ivs_t* ivs)
{
    /** 
     * Init all Indigo Modules. 
     */
    TRY(ind_soc_init(&ivs->soc));
    TRY(ind_cxn_init(&ivs->cxn));
    TRY(ind_port_init(&ivs->port));
    TRY(ind_fwd_init(&ivs->fwd));
    TRY(ind_core_init(&ivs->core));

    TRY(ind_cfg_install_sighup_handler());

    /* Process the configuration file option if present */
    TRY(ivs_options__(ivs, "f ")); 

#ifdef DEPENDMODULE_INCLUDE_AET
    {
        /* @FIXME: Expose AET config in IVS config? */
        aet_config_t aet_config;

        IVS_MEMSET(&aet_config, 0, sizeof(aet_config));
        aet_config.pkt_in_queue_max =
            IVS_CONFIG_ASYNC_PACKET_IN_QUEUE_MAX_DEFAULT;
        aet_config.port_max = IVS_CONFIG_PORT_MAX_PORTS_DEFAULT;
        TRY(aet_init(&aet_config));
    }
#endif

#ifdef DEPENDMODULE_INCLUDE_SNMP
    TRY(snmp_init());
#endif

    /* Process interface options */
    TRY(ivs_options__(ivs, "interface add")); 
    TRY(ivs_options__(ivs, "i ")); 
    
#if IVS_CONFIG_INCLUDE_INTERFACE_VETH_DEFAULT == 1

    if(ivs->interface_counter == 0 && ivs->min == 0) { 
        ivs_interface_add(ivs, "veth0", 1); 
        ivs_interface_add(ivs, "veth2", 2); 
        ivs_interface_add(ivs, "veth4", 3); 
        ivs_interface_add(ivs, "veth6", 4); 
    }

#endif

#if IVS_CONFIG_INCLUDE_NETWORK_CLI == 1
    nss_create(&ivs->netif.nss, IVS_CONFIG_NSS_PORT_DEFAULT, "0.0.0.0"); 
    ivs->netif.ucli = ucli_copy(ivs->ucli); 
#endif

#if IVS_CONFIG_INCLUDE_CONSOLE_CLI == 1
    /* Pseudohack -- only start the CLI if stdout is a tty */
    if(aim_pvs_isatty(&aim_pvs_stdout)) { 
        ivs->consif.els = els_create("ivs", ivs_console_prompt__, ivs);
        ivs->consif.ucli = ucli_copy(ivs->ucli); 
        ucli_pvs_set(ivs->consif.ucli, &aim_pvs_stdout); 
        els_complete_set(ivs->consif.els, (els_complete_f)ucli_complete_hook, 
                         ivs->consif.ucli);
    }
#endif 
    return 0; 
}


int 
ivs_enable(ivs_t* ivs, int enable)
{
    /* Enable everyone */
    TRY(ind_soc_enable_set(enable));
    TRY(ind_cxn_enable_set(enable));
    TRY(ind_fwd_enable_set(enable));
    TRY(ind_port_enable_set(enable));
    TRY(ind_core_enable_set(enable));

#ifdef DEPENDMODULE_INCLUDE_AET
    TRY(aet_enable_set(enable));
#endif

#ifdef DEPENDMODULE_INCLUDE_SNMP
    TRY(snmp_enable_set(enable));
#endif


    /*
     * Process remaining options
     */
    TRY(ivs_options__(ivs, NULL)); 

    /*
     * Disable ARGV group 
     */
    ucli_group_enable(ivs->ucli, "argv", 0); 

#if IVS_CONFIG_INCLUDE_CONTROLLER_DEFAULT == 1
    if(ivs->connection_count == 0 && ivs->min == 0) { 
        ivs_controller_add(ivs, IVS_CXN_TYPE_CONNECT, 
                           IVS_CONFIG_CONTROLLER_IP_DEFAULT, 
                           IVS_CONFIG_CONTROLLER_PORT_DEFAULT); 
    }
#endif

#if IVS_CONFIG_INCLUDE_NETWORK_CLI == 1
    if(ivs->netif.nss) { 
        if(nss_start(ivs->netif.nss) >= 0) { 
            TRY(ind_soc_timer_event_register_with_priority(
                    ivs_nss_callback__, ivs, 100, 10));
        }
    }
#endif

#if IVS_CONFIG_INCLUDE_CONSOLE_CLI == 1
    if(ivs->consif.els) {
        els_start(ivs->consif.els); 
        #if ELS_CONFIG_INCLUDE_EVENTFD == 1
        TRY(ind_soc_socket_register(ivs->consif.els->eventfd, 
                                    ivs_els_eventfd_callback__, 
                                    ivs)); 
        #else
        TRY(ind_soc_timer_event_register(ivs_els_callback__, ivs, 100)); 
        #endif
    }
#endif

#if IVS_CONFIG_INCLUDE_CXN_LOG == 1
    indigo_cxn_status_change_register(ivs_cxn_state_log__, ivs); 
#endif
    return 0; 
}

static indigo_cxn_info_t*
ivs_controller_find_type__(ivs_t* ivs, ivs_cxn_type_t type, 
                           const char* caddr, int cport, 
                           indigo_cxn_info_t* rv)
{
    indigo_cxn_info_t* cxn; 
    indigo_cxn_info_t* list = NULL; 

    indigo_cxn_list(&list); 

    for(cxn = list; cxn; cxn = cxn->next) { 
        if(!IVS_STRCMP(caddr, cxn->cxn_proto_params.tcp_over_ipv4.controller_ip) && 
           cxn->cxn_proto_params.tcp_over_ipv4.controller_port == cport) { 

            if( (type == IVS_CXN_TYPE_LISTEN) == cxn->cxn_config_params.listen) { 
                if(rv) { 
                    IVS_MEMCPY(rv, cxn, sizeof(*rv)); 
                    break; 
                }
            }
        }   
    }
    indigo_cxn_list_destroy(list); 
    return (cxn) ? rv : NULL; 
}


int 
ivs_controller_show(ivs_t* ivs, ivs_cxn_type_t type, aim_pvs_t* pvs)
{
    indigo_cxn_info_t* cxn; 
    indigo_cxn_info_t* list = NULL; 
    int typecount = 0;
    indigo_cxn_status_t status;
    int rv;
    const char* statename;
    const char* rolename;

    indigo_cxn_list(&list); 

    for(cxn = list; cxn; cxn = cxn->next) { 
        ivs_cxn_type_t ctype = (cxn->cxn_config_params.listen == 1) ? 
            IVS_CXN_TYPE_LISTEN : IVS_CXN_TYPE_CONNECT; 
        if(ctype == type) { 
            typecount++; 

            rv = indigo_cxn_connection_status_get(cxn->cxn_id, &status);
            if (rv == INDIGO_ERROR_NONE) {
                statename = ivs_cxn_state_name((ivs_cxn_state_t)status.state);
                rolename = ivs_cxn_role_name((ivs_cxn_role_t)status.role);
            } else {
                statename = "UNKNOWN";
                rolename = "UNKNOWN";
            }

            aim_printf(pvs, "    %s:%d  %14s  %8s\n", 
                       cxn->cxn_proto_params.tcp_over_ipv4.controller_ip, 
                       cxn->cxn_proto_params.tcp_over_ipv4.controller_port,
                       statename, rolename);
        }
    }
    if(typecount == 0) { 
        aim_printf(pvs, "    None.\n"); 
    }

    indigo_cxn_list_destroy(list); 

    return INDIGO_ERROR_NONE; 
}

                    
int 
ivs_controller_add(ivs_t* ivs, ivs_cxn_type_t type,
                   const char* caddr, int cport)
{
    int rv; 
    indigo_cxn_info_t cxn; 

    if( ivs == NULL || caddr == NULL || cport < 0 ||
        !IVS_CXN_TYPE_VALID(type) ) { 
        return INDIGO_ERROR_PARAM; 
    }

    if(ivs_controller_find_type__(ivs, type, caddr, cport, &cxn)) { 
        /* Connection exists */
        return INDIGO_ERROR_EXISTS; 
    }
    
    IVS_MEMSET(&cxn, 0, sizeof(cxn)); 
    cxn.cxn_config_params.version = OF_VERSION_1_0; 
    cxn.cxn_proto_params.tcp_over_ipv4.protocol = 
        INDIGO_CXN_PROTO_TCP_OVER_IPV4; 
    IVS_STRCPY(cxn.cxn_proto_params.tcp_over_ipv4.controller_ip, caddr); 
    cxn.cxn_proto_params.tcp_over_ipv4.controller_port = cport; 
    
    switch (type)
        {
        case IVS_CXN_TYPE_INVALID:
        case IVS_CXN_TYPE_COUNT:
            { 
                /* shouldn't get here */
                break; 
            }

        case IVS_CXN_TYPE_CONNECT:
            {
                cxn.cxn_config_params.periodic_echo_ms = IVS_CONFIG_CXN_PERIODIC_ECHO_MS_DEFAULT; 
                cxn.cxn_config_params.reset_echo_count = IVS_CONFIG_CXN_RESET_ECHO_COUNT;
                break; 
            }
        case IVS_CXN_TYPE_LISTEN:
            {
                cxn.cxn_config_params.local = 1; 
                cxn.cxn_config_params.listen = 1; 
                break; 
            }
        }

    if( (rv = indigo_cxn_connection_add(&cxn.cxn_proto_params, 
                                        &cxn.cxn_config_params, 
                                        &cxn.cxn_id)) < 0) { 
        return rv; 
    }
    else {    
        ivs->connection_count++; 
        return INDIGO_ERROR_NONE; 
    }
}


int 
ivs_controller_remove(ivs_t* ivs, ivs_cxn_type_t type, 
                      const char* caddr, int cport)
{
    int rv; 
    indigo_cxn_info_t cxn; 

    if( ivs == NULL || caddr == NULL || cport < 0 ||
        !IVS_CXN_TYPE_VALID(type) ) { 
        return INDIGO_ERROR_PARAM; 
    }

    if(ivs_controller_find_type__(ivs, type, caddr, cport, &cxn)) { 
        if( (rv = indigo_cxn_connection_remove(cxn.cxn_id)) < 0) { 
            return rv; 
        }
        else {
            ivs->connection_count--; 
            return INDIGO_ERROR_NONE; 
        }
    }
    else {
        /* not found */        
        return INDIGO_ERROR_NOT_FOUND; 
    }
}

#if IVS_CONFIG_INCLUDE_NETWORK_CLI == 1
static void 
ivs_nss_callback__(void* cookie)
{
    char line[256];     
    int rv;  
    ivs_t* ivs = (ivs_t*) cookie; 

    IVS_MEMSET(line, 0, sizeof(line)); 
    rv = nss_poll(ivs->netif.nss, (uint8_t*)line, sizeof(line)); 
    if(rv > 0) { 
        ucli_dispatch_string(ivs->netif.ucli, &ivs->netif.nss->pvs, line); 
        nss_close(ivs->netif.nss); 
        ucli_reset(ivs->netif.ucli); 
    }
}
#endif

#if IVS_CONFIG_INCLUDE_CONSOLE_CLI == 1

static void
ivs_els_callback__(void* cookie)
{
    ivs_t* ivs = (ivs_t*)cookie; 
    char* line = els_getline(ivs->consif.els);
    if(line) { 
        if(line && line[0] == '^' && line[1] == 'D') { 
            aim_printf(&aim_pvs_stdout, "\n"); 
            ind_soc_run_status_set(IND_SOC_RUN_STATUS_EXIT); 
        }
        else { 
            ucli_dispatch_string(ivs->consif.ucli, &aim_pvs_stdout, line); 
            els_prompt(ivs->consif.els); 
        }
    }
}

#if ELS_CONFIG_INCLUDE_EVENTFD == 1
#include <unistd.h>
static void
ivs_els_eventfd_callback__(int socket_id, void *cookie, 
                           int read_ready, int write_ready, int error_seen)
{
    if(read_ready) { 
        uint64_t v; 
        ivs_t* ivs = (ivs_t*)cookie; 
        if (read(ivs->consif.els->eventfd, &v, sizeof(v)) < 0) {
            /* silence warn_unused_result */
        }
        ivs_els_callback__(cookie); 
    }
}
#endif /* ELS_CONFIG_INCLUDE_EVENTFD */
#endif /* IVS_CONFIG_INCLUDE_CONSOLE_CLI */


static void 
sighandler__(int sig)
{
    ind_soc_run_status_set(IND_SOC_RUN_STATUS_EXIT); 
}

int 
ivs_run(ivs_t* ivs, int ms)
{
    int rv; 

    void (*previous)(int) = signal(SIGUSR1, sighandler__);

#if IVS_CONFIG_INCLUDE_CONSOLE_CLI == 1
    if(ivs->consif.els) { 
        els_prompt(ivs->consif.els); 
    }
#endif

#if IVS_CONFIG_INCLUDE_WATCHDOG == 1
    if(ivs->watchdog_seconds) { 
        ivs_watchdog_enable(ivs, ivs->watchdog_seconds); 
    }
#endif

#if IVS_CONFIG_INCLUDE_STATUS_LOG == 1
    ivs_status_log_enable(ivs);
#endif

    rv = ind_soc_select_and_run(ms);

#if IVS_CONFIG_INCLUDE_WATCHDOG == 1
    if(ivs->watchdog_seconds) { 
        /* Just cancel existing alarm */
        alarm(0); 
    }
#endif          
    signal(SIGUSR1, previous); 
    return rv; 
}

int 
ivs_denit(ivs_t* ivs)
{
    int rv; 
    if ((rv = ind_core_finish()) < 0) {
        AIM_LOG_ERROR("Error in ind_core_finish");
    }
    if ((rv = ind_fwd_finish()) < 0) {
        AIM_LOG_ERROR("Error in ind_fwd_finish");
    }
    if ((rv = ind_port_finish()) < 0) {
        AIM_LOG_ERROR("Error in ind_port_finish");
    }
    if ((rv = ind_cxn_finish()) < 0) {
        AIM_LOG_ERROR("Error in ind_cxn_finish");
    }
    if ((rv = ind_soc_finish()) < 0) {
        AIM_LOG_ERROR("Error in ind_soc_finish");
    }

#ifdef DEPENDMODULE_INCLUDE_AET
    if ((rv = aet_finish()) < 0) { 
        AIM_LOG_ERROR("Error in aet_finish"); 
    }
#endif

    return rv; 
}

int 
ivs_destroy(ivs_t* ivs)
{
    int rv; 
    if ((rv = ind_core_finish()) < 0) {
        AIM_LOG_ERROR("Error in ind_core_finish");
    }
    if ((rv = ind_fwd_finish()) < 0) {
        AIM_LOG_ERROR("Error in ind_fwd_finish");
    }
    if ((rv = ind_port_finish()) < 0) {
        AIM_LOG_ERROR("Error in ind_port_finish");
    }

#ifdef DEPENDMODULE_INCLUDE_AET
    if ((rv = aet_finish()) < 0) { 
        AIM_LOG_ERROR("Error in aet_finish"); 
    }
#endif


    if ((rv = ind_cxn_finish()) < 0) {
        AIM_LOG_ERROR("Error in ind_cxn_finish");
    }
    if ((rv = ind_soc_finish()) < 0) {
        AIM_LOG_ERROR("Error in ind_soc_finish");
    }


    ivs_options_free__(ivs->string_options); 
    ivs_options_free__(ivs->argv_options); 

#if IVS_CONFIG_INCLUDE_NETWORK_CLI == 1
    nss_destroy(ivs->netif.nss);
    ucli_destroy(ivs->netif.ucli); 
#endif

#if IVS_CONFIG_INCLUDE_CONSOLE_CLI == 1
    els_destroy(ivs->consif.els); 
    ucli_destroy(ivs->consif.ucli); 
#endif

    ucli_destroy(ivs->ucli); 

#if IVS_CONFIG_INCLUDE_CXN_LOG == 1
    bigring_destroy(ivs->cxn_log_ring);
#endif

#if IVS_CONFIG_INCLUDE_STATUS_LOG == 1
    bigring_destroy(ivs->status_log_ring);
#endif

    IVS_FREE(ivs);
    return 0; 
}

int 
ivs_interface_add(ivs_t* ivs, const char* interface, int index)
{
    if(index == -1) {   
        index = ivs->interface_counter + 1;
    }

    TRY(indigo_port_interface_add((char*)interface, 
                                  index, 
                                  &ivs->port_add_config));
    ivs->interface_counter++; 
    return INDIGO_ERROR_NONE; 
}


int
ivs_interface_find__(ivs_t* ivs, char* ifname, int of_port, 
                     indigo_port_info_t* pi)
{
    int rv; 
    indigo_port_info_t* list; 
    if(indigo_port_interface_list(&list) == INDIGO_ERROR_NONE) { 
        indigo_port_info_t* p; 
        for(p = list; p; p = p->next) { 
            if(of_port > 0) { 
                /* Check port number */
                if(p->of_port != of_port) { 
                    continue; 
                }
            }
            if(ifname) { 
                /* Check interface name */
                if(IVS_STRCMP(p->port_name, ifname)) { 
                    continue; 
                }
            }
            if(pi) { 
                IVS_MEMCPY(pi, p, sizeof(*pi)); 
            }
            rv = 1; 
        }
    }
    else { 
        rv = 0; 
    }
    indigo_port_interface_list_destroy(list); 
    return rv; 
}
            
int 
ivs_interface_remove(ivs_t* ivs, int of_port)
{
    /* The port manager should have a remove-by of_port */
    indigo_port_info_t pi; 
    if(ivs_interface_find__(ivs, NULL, of_port, &pi)) { 
        TRY(indigo_port_interface_remove(pi.port_name)); 
    }
    return INDIGO_ERROR_NONE; 
}

int 
ivs_interface_show(ivs_t* ivs, aim_pvs_t* pvs)
{
    indigo_port_info_t* p; 
    indigo_port_info_t* list = NULL; 

    indigo_port_interface_list(&list); 
    
    if(list == NULL) { 
        aim_printf(pvs, "    None\n"); 
    }
    else { 
        for(p = list; p; p = p->next) { 
            aim_printf(pvs, "    %d: %s\n", p->of_port, p->port_name); 
        }
    }
    indigo_port_interface_list_destroy(list); 
    return INDIGO_ERROR_NONE; 
}

int 
ivs_interface_exists(ivs_t* ivs, int index)
{
    return ivs_interface_find__(ivs, NULL, index, NULL); 
}

int 
ivs_command(ivs_t* ivs, const char* cmd)
{
    return ucli_dispatch_string(ivs->ucli, &aim_pvs_stdout, cmd); 
}

static int
ivs_loci_logger(loci_log_level_t level,
                const char *fname, const char *file, int line,
                const char *format, ...)
{
    int log_flag;
    switch (level) {
    case LOCI_LOG_LEVEL_TRACE:
        log_flag = AIM_LOG_FLAG_TRACE;
        break;
    case LOCI_LOG_LEVEL_VERBOSE:
        log_flag = AIM_LOG_FLAG_VERBOSE;
        break;
    case LOCI_LOG_LEVEL_INFO:
        log_flag = AIM_LOG_FLAG_INFO;
        break;
    case LOCI_LOG_LEVEL_WARN:
        log_flag = AIM_LOG_FLAG_WARN;
        break;
    case LOCI_LOG_LEVEL_ERROR:
        log_flag = AIM_LOG_FLAG_ERROR;
        break;
    default:
        log_flag = AIM_LOG_FLAG_MSG;
        break;
    }

    va_list ap;
    va_start(ap, format);
    aim_log_vcommon(&AIM_LOG_STRUCT, log_flag, NULL, 0, fname, file, line, format, ap);
    va_end(ap);

    return 0;
}

int 
ivs_main(char** string_options, char** argv_options)
{
    ivs_t* ivs; 

    loci_logger = ivs_loci_logger;

    TRY(ucli_init());
    TRY(ivs_create(&ivs, string_options, argv_options)); 
    TRY(ivs_init(ivs)); 
    TRY(ivs_enable(ivs, 1)); 
    TRY(ivs_run(ivs, -1)); 
    TRY(ivs_destroy(ivs)); 
    ucli_denit(); 
    return 0;
}
    
#if IVS_CONFIG_INCLUDE_WATCHDOG == 1

/**
 * Watchdog Processing
 *
 * We want to detect when the indigo main select loop
 * has gone off in the weeds -- this can happen 
 * with bugs that introduce stack corruption, or event handlers
 * that fail to return due to their own bugs or unanticipated 
 * circumstances. 
 *
 * We detect and recover from hangs in the main loop as follows:
 *
 * 1. Set an alarm signal for the maximum allowable time T
 *    we think any handler should execute. 
 * 2. Set an indigo timer event callback to re-arm the given
 *    alarm every T/2 seconds. 
 * 3. Make SIGALRM cause termination (do not catch or ignore).
 *
 * If the maximum time has elapsed, SIGALRM will be raised, and 
 * the entire process will exit. The restart daemon will then automatically
 * restart the entire process. 
 *
 *
 * TODO: determine the appropriate value for the maximum 
'* allowable execution time. 
 */

/**
 * Calling alarm resets the watchdog.  It is expected that 
 * alarm_watchdog_callback__ is called at half the watchdog
 * timeout time.
 */

static void
alarm_watchdog_callback__(void* cookie)
{
    ivs_t* ivs = (ivs_t*)(cookie); 
    alarm(ivs->watchdog_seconds);
}

int
ivs_watchdog_enable(ivs_t* ivs, int seconds)
{
    ivs->watchdog_seconds = seconds; 
    TRY(ind_soc_timer_event_register_with_priority(alarm_watchdog_callback__,
                                                   ivs, 
                                                   seconds*1000 /2,
                                                   IND_SOC_HIGHEST_PRIORITY));
    return 0; 
}

#endif
                                     
#if IVS_CONFIG_INCLUDE_CXN_LOG == 1

int 
ivs_cxn_log_show(ivs_t* ivs, aim_pvs_t* pvs)
{
    int iter; 
    char* s;
    int count = 0; 

    bigring_lock(ivs->cxn_log_ring); 
    bigring_iter_start(ivs->cxn_log_ring, &iter); 
    while( (s = bigring_iter_next(ivs->cxn_log_ring, &iter)) ) { 
        aim_printf(pvs, "%s\n", s); 
        count++; 
    }
    bigring_unlock(ivs->cxn_log_ring); 
    if(count == 0) { 
        aim_printf(pvs, "None.\n"); 
    }
    return count;
}

static void
ivs_cxn_state_log__(indigo_cxn_id_t              cxn_id,
                    indigo_cxn_protocol_params_t *cxn_proto_params,
                    indigo_cxn_state_t           cxn_state,
                    void                         *cookie)
{
    ivs_t* ivs = (ivs_t*) cookie; 
    time_t now;
    struct tm nowtm; 
    char timestamp[64]; 
    char* msg;
    indigo_cxn_config_params_t cfg;

    /* No state logs necessary for these two states */
    if (cxn_state == INDIGO_CXN_S_CONNECTING ||
        cxn_state == INDIGO_CXN_S_CLOSING) {
        return;
    }

    /* Do not log local connections */
    if (indigo_cxn_connection_config_get(cxn_id, &cfg) != INDIGO_ERROR_NONE) {
        return;
    }
    if (cfg.local) {
        return;
    }

    time(&now); 
    asctime_r(localtime_r(&now, &nowtm), timestamp); 
    /* Remove newline */
    timestamp[strlen(timestamp)-1]=' ';
    msg = aim_fstrdup("%s%s:%u - %s", 
                      timestamp,
                      cxn_proto_params->tcp_over_ipv4.controller_ip, 
                      cxn_proto_params->tcp_over_ipv4.controller_port, 
                      (cxn_state == INDIGO_CXN_S_HANDSHAKE_COMPLETE) ?
                      "Connected" : "Disconnected"); 

    bigring_push(ivs->cxn_log_ring, msg);
}

#endif /* IVS_CONFIG_INCLUDE_CXN_LOG */
    

#if IVS_CONFIG_INCLUDE_STATUS_LOG == 1

uint64_t prev_drops = 0;

static void
status_log_callback__(void* cookie)
{
    indigo_cxn_info_t* cxn; 
    indigo_cxn_info_t* list = NULL; 
    time_t now;
    struct tm nowtm; 
    char timestamp[64]; 
    ivs_t* ivs = (ivs_t*)(cookie); 
    char* msg;
    uint32_t total_flows, flow_mods, packet_ins, packet_outs;
    uint64_t drops;

    indigo_core_stats_get(&total_flows, &flow_mods,
                          &packet_ins, &packet_outs);

    indigo_cxn_list(&list); 
    drops = 0;
    for (cxn = list; cxn; cxn = cxn->next) { 
        if (! (cxn->cxn_config_params.listen &&
               cxn->cxn_config_params.local) ) {
            drops += cxn->cxn_status.packet_in_drop;
        }
    }
    indigo_cxn_list_destroy(list); 

    time(&now);
    strftime(timestamp, sizeof(timestamp), "%T", localtime_r(&now, &nowtm));

    msg = aim_fstrdup("%10s %10u %10u %10u %10u %10u", timestamp,
                      total_flows, flow_mods, packet_ins, packet_outs, 
                      (int) (drops - prev_drops));
    bigring_push(ivs->status_log_ring, msg);
}

int
ivs_status_log_enable(ivs_t* ivs)
{
    TRY(ind_soc_timer_event_register_with_priority(status_log_callback__,
                                                   ivs, 
                                                   IVS_CONFIG_STATUS_LOG_PERIOD_S*1000,
                                                   15));
    return 0; 
}

int 
ivs_status_log_show(ivs_t* ivs, aim_pvs_t* pvs)
{
    int iter; 
    char* s;
    int count = 0; 

    aim_printf(pvs, "%10s %10s %10s %10s %10s %10s\n", "Time",
               "Flows", "FlowMods", "PacketIns", "PacketOuts", "Drops");

    bigring_lock(ivs->status_log_ring); 
    bigring_iter_start(ivs->status_log_ring, &iter); 
    while( (s = bigring_iter_next(ivs->status_log_ring, &iter)) ) { 
        aim_printf(pvs, "%s\n", s); 
        count++; 
    }
    bigring_unlock(ivs->status_log_ring); 
    if(count == 0) { 
        aim_printf(pvs, "None.\n"); 
    }
    return count;
}

#endif /* IVS_CONFIG_INCLUDE_STATUS_LOG */

