/**************************************************************************//**
 *
 *
 *
 *
 *****************************************************************************/
#include <IVS/ivs_config.h> 
#include <IVS/uCli/ivs_ucli.h>

#include "ivs_int.h" 
#include "ivs_util.h"
#include "ivs_log.h"

#include <sys/types.h>
#include <unistd.h>


static int
ucli_indigo_error(ucli_context_t* uc, int rv)
{
    return ucli_error(uc, "indigo: %{ind_error}", rv); 
}


/**
 * Get the IVS pointer from the context structure for each command. 
 */
#undef UCLI_COMMAND_INIT
#define UCLI_COMMAND_INIT AIM_VAR_PCAST_SAFE(ivs_t*, ivs, uc, uc->cookie)

static ucli_status_t
ivs_ucli_controller__add__(ucli_context_t* uc)
{
    int rv; 
    ivs_cxn_type_t type; 
    char caddr[64]; 
    int port; 

    UCLI_COMMAND_INFO(uc, 
                      "add", 2, 
                      "$summary#Add a controller connection."
                      "$args#[CONNECT|LISTEN] <addr>:<port>"); 

    
    UCLI_ARGPARSE_OR_RETURN(uc, "{ivs_cxn_type}{ip4conn}", 
                            &type, &caddr, &port); 

    if( (rv = ivs_controller_add(ivs, type, caddr, port)) < 0) { 
        if(rv == INDIGO_ERROR_EXISTS) { 
            return ucli_error(uc, "connection %{ivs_cxn_type} %{ip4conn} already exists.", 
                              type, caddr, port); 
        }
        else {
            return ucli_indigo_error(uc, rv); 
        }
    }
    return UCLI_STATUS_OK; 
}

static ucli_status_t
ivs_ucli_controller__minus_c__(ucli_context_t* uc)
{
    int rv;     
    char caddr[64]; 
    int port; 

    UCLI_COMMAND_INFO(uc, 
                      "c", 1, 
                      "$group#argv"
                      "$summary#Add a controller connection."
                      "$args#<addr>:<port>"); 


    UCLI_ARGPARSE_OR_RETURN(uc, "{ip4conn}", &caddr, &port); 

    if( (rv = ivs_controller_add(ivs, IVS_CXN_TYPE_CONNECT, caddr, port)) < 0) {
        if(rv == INDIGO_ERROR_EXISTS) { 
            return ucli_error(uc, "connection %{ip4conn} already exists.",
                              caddr, port); 
        }
        else {
            return ucli_indigo_error(uc, rv); 
        }
    }
    return UCLI_STATUS_OK; 
}

static ucli_status_t
ivs_ucli_config__minus_v__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc, 
                      "v", 0, 
                      "$group#argv"
                      "$summary#Enable verbose logging."); 
    AIM_REFERENCE(ivs);
    aim_log_fid_set_all(AIM_LOG_FLAG_FATAL, 1); 
    aim_log_fid_set_all(AIM_LOG_FLAG_ERROR, 1); 
    aim_log_fid_set_all(AIM_LOG_FLAG_WARN, 1); 
    aim_log_fid_set_all(AIM_LOG_FLAG_INFO, 1); 
    aim_log_fid_set_all(AIM_LOG_FLAG_VERBOSE, 1); 
    return UCLI_STATUS_OK; 
}
static ucli_status_t
ivs_ucli_config__verbose__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc, 
                      "verbose", 0, 
                      "$group#argv"
                      "$summary#Enable verbose logging.");
    AIM_REFERENCE(ivs);
    aim_log_fid_set_all(AIM_LOG_FLAG_FATAL, 1); 
    aim_log_fid_set_all(AIM_LOG_FLAG_ERROR, 1); 
    aim_log_fid_set_all(AIM_LOG_FLAG_WARN, 1); 
    aim_log_fid_set_all(AIM_LOG_FLAG_INFO, 1); 
    aim_log_fid_set_all(AIM_LOG_FLAG_VERBOSE, 1); 
    return UCLI_STATUS_OK; 
}
    
static ucli_status_t
ivs_ucli_config__minus_t__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc, 
                      "t", 0, 
                      "$group#argv"
                      "$summary#Enable trace logging."); 
    AIM_REFERENCE(ivs);
    aim_log_fid_set_all(AIM_LOG_FLAG_FATAL, 1); 
    aim_log_fid_set_all(AIM_LOG_FLAG_ERROR, 1); 
    aim_log_fid_set_all(AIM_LOG_FLAG_WARN, 1); 
    aim_log_fid_set_all(AIM_LOG_FLAG_INFO, 1); 
    aim_log_fid_set_all(AIM_LOG_FLAG_VERBOSE, 1); 
    aim_log_fid_set_all(AIM_LOG_FLAG_TRACE, 1); 
    return UCLI_STATUS_OK; 
}

static ucli_status_t
ivs_ucli_config__minus_l__(ucli_context_t* uc)
{
    char* filename;
    aim_pvs_t *pvs;

    UCLI_COMMAND_INFO(uc,
                      "l", 1,
                      "$group#argv"
                      "$summary#Set the log file"
                      "$args#<filename>");

    (void) ivs;
    UCLI_ARGPARSE_OR_RETURN(uc, "s", &filename);

    if ((pvs = aim_pvs_fopen(filename, "a")) == NULL) {
        return ucli_error(uc, "failed to open logfile");
    }
    aim_log_pvs_set_all(pvs);

    return UCLI_STATUS_OK;
}


static ucli_status_t
ivs_ucli_config__pid__(ucli_context_t* uc)
{
    char* filename; 
    aim_pvs_t* pvs; 

    UCLI_COMMAND_INFO(uc, 
                      "pid", 1, 
                      "$group#argv"
                      "$summary#Write PID to file."
                      "$args#<filename>"); 
    UCLI_ARGPARSE_OR_RETURN(uc, "s", &filename); 
    if((pvs=aim_pvs_fopen(filename, "w")) == NULL) { 
        return ucli_error(uc, "failed to open pid file."); 
    }
    aim_printf(pvs, "%d\n", getpid()); 
    aim_pvs_destroy(pvs); 
    AIM_REFERENCE(ivs);
    return UCLI_STATUS_OK; 
}

    
static ucli_status_t
ivs_ucli_controller__minus_L__(ucli_context_t* uc)
{
    int rv;     
    char caddr[64]; 
    int port; 

    UCLI_COMMAND_INFO(uc, 
                      "L", 1, 
                      "$group#argv"
                      "$summary#Add a listening connection."
                      "$args#<addr>:<port>"); 


    UCLI_ARGPARSE_OR_RETURN(uc, "{ip4conn}", &caddr, &port); 
    if( (rv = ivs_controller_add(ivs, IVS_CXN_TYPE_LISTEN, caddr, port)) < 0) {
        if(rv == INDIGO_ERROR_EXISTS) { 
            return ucli_error(uc, "connection %{ip4conn} already exists.",
                              caddr, port); 
        }
        else {
            return ucli_indigo_error(uc, rv); 
        }
    }
    return UCLI_STATUS_OK; 
}
                              
                  
                      

static ucli_status_t
ivs_ucli_controller__remove__(ucli_context_t* uc)
{
    int rv; 
    ivs_cxn_type_t type; 
    char caddr[64]; 
    int port; 

    UCLI_COMMAND_INFO(uc, 
                      "remove", 2, 
                      "$summary#Remove a controller connection."
                      "$args#[CONNECT|LISTEN] <addr>:<port>"); 

    
    UCLI_ARGPARSE_OR_RETURN(uc, "{ivs_cxn_type}{ip4conn}", 
                            &type, &caddr, &port); 

    if ( (rv = ivs_controller_remove(ivs, type, caddr, port)) < 0) { 
        if(rv == INDIGO_ERROR_NOT_FOUND) { 
            return ucli_error(uc, "connection %{ivs_cxn_type} %{ip4conn} does not exist.", 
                              type, caddr, port); 
        }
        else {
            return ucli_indigo_error(uc, rv); 
        }
    }
    return UCLI_STATUS_OK; 
}

static ucli_status_t
ivs_ucli_controller__show__(ucli_context_t* uc)
{       
    ivs_cxn_type_t type; 

    UCLI_COMMAND_INFO(uc, 
                      "show", 1, 
                      "$summary#Show controller connections by type."
                      "$args#[CONNECT|LISTEN]"); 

    
    UCLI_ARGPARSE_OR_RETURN(uc, "{ivs_cxn_type}", &type); 
    ivs_controller_show(ivs, type, &uc->pvs); 
    return UCLI_STATUS_OK; 
}
                      
       
static ucli_status_t
ivs_ucli_interface__add__(ucli_context_t* uc)
{
    int rv; 
    char* iface; 
    int index; 

    UCLI_COMMAND_INFO(uc, 
                      "add", 2, 
                      "$summary#Add an interface."
                      "$args#<of_index> <interface>"); 


    UCLI_ARGPARSE_OR_RETURN(uc, "is", &index, &iface); 
    if( (rv = ivs_interface_add(ivs, iface, index)) < 0) { 
        if(rv == INDIGO_ERROR_EXISTS) { 
            return ucli_error(uc, "there is already an interface at index %d", 
                              index); 
        }
        else { 
            return ucli_indigo_error(uc, rv); 
        }
    }
    return UCLI_STATUS_OK; 
}
 
static ucli_status_t
ivs_ucli_interface__remove__(ucli_context_t* uc)
{
    int rv; 
    int index; 

    UCLI_COMMAND_INFO(uc, 
                      "remove", 1, 
                      "$summary#Remove an interface at the given index."
                      "$args#<of_index>"); 

    UCLI_ARGPARSE_OR_RETURN(uc, "i", &index); 
    if( (rv = ivs_interface_remove(ivs, index)) < 0) { 
        if(rv == INDIGO_ERROR_NOT_FOUND) { 
            return ucli_error(uc, "there is no interface at index %d.", index); 
        }
        else {
            return ucli_indigo_error(uc, rv); 
        }
    }
    return UCLI_STATUS_OK; 
}

static ucli_status_t
ivs_ucli_interface__show__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc, 
                      "show", 0, 
                      "$summary#Show all interfaces."); 

    ivs_interface_show(ivs, &uc->pvs); 
    return UCLI_STATUS_OK; 
}

static ucli_status_t
ivs_ucli_interface__minus_i__(ucli_context_t* uc)
{
    int rv; 
    char* interface; 

    UCLI_COMMAND_INFO(uc, 
                      "i", 1, 
                      "$group#argv"
                      "$summary#Add an interface (next available index)."
                      "$args#<interface>"); 

    UCLI_ARGPARSE_OR_RETURN(uc, "s", &interface); 
    if( (rv = ivs_interface_add(ivs, interface, -1)) < 0) { 
        if(rv == INDIGO_ERROR_NOT_FOUND) { 
            return ucli_error(uc, "maximum interfaces."); 
        }
        else {
            return ucli_indigo_error(uc, rv); 
        }
    }
    return UCLI_STATUS_OK; 
}

#if IVS_CONFIG_INCLUDE_CXN_LOG == 1
static ucli_status_t
ivs_ucli_cxn__show_log__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc, 
                      "show-log", 0, 
                      "$summary#Show connection log."); 
    ivs_cxn_log_show(ivs, &uc->pvs); 
    return UCLI_STATUS_OK; 
}
#endif /* IVS_CONFIG_INCLUDE_CXN_LOG */

static ucli_status_t
ivs_ucli_cxn__trace__(ucli_context_t* uc)
{
    char* pvs_name; 
    aim_pvs_t* pvs = NULL; 

    UCLI_COMMAND_INFO(uc, 
                      "trace", 1, 
                      "$summary#Trace openflow messages on a connection."
                      "$args#[-|stdout|stderr|on|off|filename]"); 

    AIM_REFERENCE(ivs); 

    UCLI_ARGPARSE_OR_RETURN(uc, "s", &pvs_name); 
    
    if(!UCLI_STRCMP(pvs_name, "-") || 
       !UCLI_STRCMP(pvs_name, "stdout") ||
       !UCLI_STRCMP(pvs_name, "on")) { 
        pvs = &aim_pvs_stdout;
    }
    else if(!UCLI_STRCMP(pvs_name, "stderr")) { 
        pvs = &aim_pvs_stderr;
    }
    else if(!UCLI_STRCMP(pvs_name, "none") ||
            !UCLI_STRCMP(pvs_name, "off")) { 
        pvs = NULL; 
        ind_cxn_message_trace(-1, NULL); 
    }
    else { 
        pvs = aim_pvs_fopen(pvs_name, "w"); 
        if(pvs == NULL) { 
            return ucli_error(uc, "Could not open file '%s' for writing.", pvs_name); 
        }
    }
    ind_cxn_message_trace(-1, pvs); 
    return UCLI_STATUS_OK; 
}

static ucli_status_t
ivs_ucli_loci__objdump__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc, 
                      "objdump", 0, 
                      "$summary#Report all current of_objects."); 

    AIM_REFERENCE(ivs); 
#ifdef OF_OBJECT_TRACKING
    of_object_track_report((loci_writer_f)aim_printf, &uc->pvs); 
#else
    aim_printf(&uc->pvs, "Object tracking was not enabled at compile time.\n");
#endif
    return UCLI_STATUS_OK; 
}

static ucli_status_t
ivs_ucli_core__dump_flows(ucli_context_t* uc)    
{
    UCLI_COMMAND_INFO(uc, 
                      "dump-flows", 0, 
                      "$summary#Dump all flow table entries."); 

    AIM_REFERENCE(ivs); 
    ind_core_ft_dump(&uc->pvs); 
    return UCLI_STATUS_OK; 
}

static ucli_status_t
ivs_ucli_core__flow_table(ucli_context_t* uc)    
{
    UCLI_COMMAND_INFO(uc, 
                      "flowtable", 0, 
                      "$summary#Basic stats from flow table."); 

    AIM_REFERENCE(ivs); 
    ind_core_ft_stats(&uc->pvs); 
    return UCLI_STATUS_OK; 
}

static ucli_status_t
ivs_ucli_core__show_flows(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc, 
                      "show-flows", 0, 
                      "$summary#Show all flow table entries."); 

    AIM_REFERENCE(ivs); 
    ind_core_ft_show(&uc->pvs); 
    return UCLI_STATUS_OK; 
}

#if IVS_CONFIG_INCLUDE_STATUS_LOG == 1
static ucli_status_t
ivs_ucli_core__show_status__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc, 
                      "show-status", 0, 
                      "$summary#Show status."); 
    ivs_status_log_show(ivs, &uc->pvs); 
    return UCLI_STATUS_OK; 
}
#endif /* IVS_CONFIG_INCLUDE_STATUS_LOG */

static ucli_status_t
ivs_ucli_config__min(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc, 
                      "min", 0, 
                      "$group#argv"
                      "$summary#Set minimal configuration at startup."); 
    ivs->min = 1;
    return UCLI_STATUS_OK; 
}

static ucli_status_t
ivs_ucli_config__wd(ucli_context_t* uc)
{
    int sec; 
    UCLI_COMMAND_INFO(uc, 
                      "wd", 1, 
                      "$group#argv"
                      "$summary#Set the watchdog timer (in seconds)."); 
    UCLI_ARGPARSE_OR_RETURN(uc, "{rint}", &sec, 0, 100, "watchdog timeout"); 
    ivs->watchdog_seconds = sec; 
    return UCLI_STATUS_OK; 
}

static ucli_status_t
ivs_ucli_config__exit__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc, 
                      "exit", 0, 
                      "$summary#Exit IVS."); 
    AIM_REFERENCE(ivs);
    exit(0); 
    return UCLI_STATUS_OK; 
}

static ucli_status_t
ivs_ucli_config__quit__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc, 
                      "quit", 0, 
                      "$summary#Exit IVS."); 
    AIM_REFERENCE(ivs);
    exit(0); 
    return UCLI_STATUS_OK; 
}

/* <auto.ucli.handlers.start> */
/******************************************************************************
 *
 * These handler table(s) were autogenerated from the symbols in this 
 * source file. 
 *
 *****************************************************************************/
static ucli_command_handler_f ivs_ucli_cxn_handlers__[] = 
{
#if IVS_CONFIG_INCLUDE_CXN_LOG == 1
    ivs_ucli_cxn__show_log__, 
#endif
    ivs_ucli_cxn__trace__, 
    NULL
};
static ucli_command_handler_f ivs_ucli_config_handlers__[] = 
{
    ivs_ucli_config__minus_v__, 
    ivs_ucli_config__verbose__, 
    ivs_ucli_config__minus_t__, 
    ivs_ucli_config__minus_l__, 
    ivs_ucli_config__pid__, 
    ivs_ucli_config__min, 
    ivs_ucli_config__wd, 
    ivs_ucli_config__exit__, 
    ivs_ucli_config__quit__, 
    NULL
};
static ucli_command_handler_f ivs_ucli_loci_handlers__[] = 
{
    ivs_ucli_loci__objdump__, 
    NULL
};
static ucli_command_handler_f ivs_ucli_core_handlers__[] = 
{
    ivs_ucli_core__dump_flows, 
    ivs_ucli_core__flow_table, 
    ivs_ucli_core__show_flows, 
#if IVS_CONFIG_INCLUDE_STATUS_LOG == 1
    ivs_ucli_core__show_status__,
#endif
    NULL
};
static ucli_command_handler_f ivs_ucli_interface_handlers__[] = 
{
    ivs_ucli_interface__add__, 
    ivs_ucli_interface__remove__, 
    ivs_ucli_interface__show__, 
    ivs_ucli_interface__minus_i__, 
    NULL
};
static ucli_command_handler_f ivs_ucli_controller_handlers__[] = 
{
    ivs_ucli_controller__add__, 
    ivs_ucli_controller__minus_c__, 
    ivs_ucli_controller__minus_L__, 
    ivs_ucli_controller__remove__, 
    ivs_ucli_controller__show__, 
    NULL
};
/******************************************************************************/
/* <auto.ucli.handlers.end> */

static ucli_module_t config_mod__ = 
    {
        "config", 
        NULL, 
        ivs_ucli_config_handlers__, 
        NULL, 
        NULL
    };

static ucli_module_t modules__[] = 
    {
        {
            "controller", 
            NULL,
            ivs_ucli_controller_handlers__, 
            NULL, 
            NULL
        }, 

        {
            "interface", 
            NULL, 
            ivs_ucli_interface_handlers__, 
            NULL, 
            NULL
        }, 
        
        {
            "cxn", 
            NULL, 
            ivs_ucli_cxn_handlers__, 
            NULL, 
            NULL
        }, 
        
        {
            "loci", 
            NULL, 
            ivs_ucli_loci_handlers__, 
            NULL, 
            NULL
        }, 
        
        {
            "core", 
            NULL, 
            ivs_ucli_core_handlers__, 
            NULL, 
            NULL, 
        }, 

    }; 
        
ucli_node_t* 
ivs_ucli_node_create(ivs_t* ivs)
{
    int i; 
    ucli_node_t* root; 

    ucli_init(); 

    root = ucli_node_create("ivs-config", NULL, NULL); 

    for(i = 0; i < AIM_ARRAYSIZE(modules__); i++) { 
        modules__[i].cookie = ivs; 
        ucli_module_init(modules__ + i); 
        ucli_node_subnode_add(root, 
                              ucli_node_create(modules__[i].name, NULL, modules__ + i)); 
    }

    /* Config module commands should be at the root */
    ucli_module_init(&config_mod__); 
    config_mod__.cookie = ivs; 
    ucli_node_module_add(root, &config_mod__); 
    return root; 
}
