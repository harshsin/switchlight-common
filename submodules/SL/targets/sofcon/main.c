/**************************************************************************//**
 *
 *
 *
 *
 *****************************************************************************/
#include <uCli/ucli.h>
#include <sofc/sofc.h>
#include <loci/of_wire_buf.h>
#include <loci/loci.h>
#include <OS/os_time.h>

typedef struct sofc_ctrl_s { 
    sofc_t* sofc; 
    char addr[64];
    int port; 
    int trace;
    int listen; 
    sofc_cxn_t id; 
} sofc_ctrl_t; 

static sofc_ctrl_t sofc_ctrl__;

#undef UCLI_COMMAND_INIT
#define UCLI_COMMAND_INIT                                       \
    AIM_VAR_PCAST_SAFE(sofc_ctrl_t*, sc, uc, uc->cookie);

static ucli_status_t
sofc_ucli_ucli__server__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc, 
                      "server", 1, 
                      "$summary#Set the server address."
                      "$args#[addr:port]"); 
    UCLI_ARGPARSE_OR_RETURN(uc, "{ip4conn}", &sc->addr, &sc->port); 
    sc->listen=1; 
    return UCLI_STATUS_OK; 
}

static ucli_status_t
sofc_ucli_ucli__connect__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc, 
                      "connect", 1, 
                      "$summary#Connect to the given switch."
                      "$args#[addr:port]"); 
    UCLI_ARGPARSE_OR_RETURN(uc, "{ip4conn}", &sc->addr, &sc->port); 
    sc->listen=0;
    return UCLI_STATUS_OK; 
}

static ucli_status_t
sofc_ucli_ucli__trace__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc, 
                      "trace", 1, 
                      "$summary#Enable or disable message tracing."
                      "$args#[true|false]"); 
    UCLI_ARGPARSE_OR_RETURN(uc, "{bool}", &sc->trace); 
    return UCLI_STATUS_OK; 
}

static int
sofc_connected__(sofc_ctrl_t* sc, ucli_context_t* uc)
{
    if(sc->sofc == NULL) { 
        sc->sofc = sofc_create(sc->addr, sc->port); 
    }
    if(sc->id == NULL) {
        if(sc->listen) { 
            if((sc->id = sofc_connection_accept(sc->sofc)) == NULL) { 
                ucli_printf(uc, "Connection accept failure."); 
                return UCLI_STATUS_E_ERROR; 
            }   
        }
        else {
            if((sc->id = sofc_connection_connect(sc->sofc)) == NULL) { 
                ucli_printf(uc, "Connection connect failure."); 
                return UCLI_STATUS_E_ERROR; 
            }
        }
    }

    return UCLI_STATUS_OK; 
}
    
#define SOFC_CONNECTED_OR_RETURN()              \
    do {                                        \
        int rv_ = sofc_connected__(sc, uc);     \
        if(rv_ != UCLI_STATUS_OK) {             \
            return rv_;                         \
        }                                       \
    } while(0)

#define SOFC_DISCONNECT()                       \
    do {                                        \
        sofc_connection_close(sc->sofc, sc->id);        \
        sofc_destroy(sc->sofc);                         \
        sc->sofc = NULL;                                \
    } while(0)

static ucli_status_t
sofc_ucli_ucli__dump__(ucli_context_t* uc)
{
    UCLI_COMMAND_INFO(uc, 
                      "dump", 0, 
                      "$summary#Dump all open flow messages."); 
    
    SOFC_CONNECTED_OR_RETURN();
    sofc_messages_dump(sc->sofc, sc->id, &uc->pvs); 
    SOFC_DISCONNECT(); 

    return UCLI_STATUS_OK; 
}

#define OF_CHECK(_expr)                                                 \
    do {                                                                \
        int rv_ = _expr ;                                               \
        if(rv_ < 0) {                                                   \
            return ucli_error(uc, "%s returned %d", #_expr, rv_);       \
        }                                                               \
    } while(0)



static ucli_status_t
sofc_bsn_shell__(ucli_context_t* uc, sofc_ctrl_t* sc, 
                 int service, unsigned char* data)
{
    uint8_t msg[OF_WIRE_BUFFER_MAX_LENGTH]; 
    FILE* fp; 
    of_bsn_shell_command_t* command = of_bsn_shell_command_new(OF_VERSION_1_0); 
    of_octets_t octets; 
    
    /* If data is a filename, read from the file */
    if( data && (fp = fopen((char*)data, "r"))) { 
        long fsize; 
        aim_free(data);
        fseek(fp, 0, SEEK_END);
        fsize = ftell(fp); 
        rewind(fp);
        data = aim_zmalloc(fsize+1); 
        fread(data, 1, fsize, fp);
        fclose(fp); 
    }

    SOFC_CONNECTED_OR_RETURN(); 

    octets.data = data; 
    octets.bytes = strlen((char*)data)+1; 
    of_bsn_shell_command_service_set(command, service); 
    of_bsn_shell_command_subtype_set(command, 6); /** BSN SHELL COMMAND */
    OF_CHECK(of_bsn_shell_command_data_set(command, &octets));
    OF_CHECK(sofc_message_send(sc->sofc, sc->id, command)); 

    if( sofc_message_type_recv(sc->sofc, sc->id, OF_BSN_SHELL_OUTPUT, msg) >= 0) { 
        of_octets_t data; 
        of_bsn_shell_output_t* output = of_bsn_shell_output_new_from_message(msg); 
        of_bsn_shell_output_data_get(output, &data); 
        ucli_printf(uc, "***\n"); 
        ucli_printf(uc, "%s", (char*)data.data); 
        ucli_printf(uc, "***\n"); 
    }
    if( sofc_message_type_recv(sc->sofc, sc->id, OF_BSN_SHELL_STATUS, msg) >= 0) { 
        uint32_t rv; 
        of_bsn_shell_status_t* status = of_bsn_shell_status_new_from_message(msg); 
        of_bsn_shell_status_status_get(status, &rv); 
        ucli_printf(uc, "status: %d\n", (int)rv); 
    }

    SOFC_DISCONNECT(); 
    aim_free(data); 
    return UCLI_STATUS_OK; 
}

static ucli_status_t
sofc_ucli_ucli__bsn_shell__(ucli_context_t* uc)
{
    /*
     * Possible service types. 
     * They map directly to the service id numbers. 
     */
    aim_datatype_map_t map[] = 
        {
            { "shell", 1 }, 
            { "cli", 2 }, 
            { "ofad", 3 }, 
            { "cint", 4 },
            { NULL, 0 }
        }; 
    aim_datatype_map_t* service; 
    char* data; 

    UCLI_COMMAND_INFO(uc, "bsn-shell", 2, 
                      "$summary#BSN Shell command"
                      "$args#[type] [data]"); 

    UCLI_ARGPARSE_OR_RETURN(uc, "{map}s", &service, map, "BSN Shell Service", &data); 
    return sofc_bsn_shell__(uc, sc, service->i, (unsigned char*)data); 
}
    

        
static ucli_status_t
sofc_ucli_ucli__forward__(ucli_context_t* uc)
{
    int inport; 
    int outport; 
    of_flow_add_t* fa;
    of_match_v1_t* mav1; 
    of_match_t* ma; 
    of_list_action_t* al; 
    of_action_output_t* ao; 

    UCLI_COMMAND_INFO(uc, 
                      "forward", 2, 
                      "$summary#Forward all packets from one port to another."
                      "$args#[inport] [outport]"); 
    
    UCLI_ARGPARSE_OR_RETURN(uc, "ii", &inport, &outport); 
    SOFC_CONNECTED_OR_RETURN(); 

    /* Flow Add Object */
    fa = of_flow_add_new(OF_VERSION_1_0); 
    of_flow_add_xid_set(fa, 0xDEADBEEF); 

    /* Match Objects */
    mav1 = of_match_v1_new(OF_VERSION_1_0); 
    ma = (of_match_t*)mav1; 

    /* Action Objects */
    al = of_list_action_new(OF_VERSION_1_0); 
    ao = of_action_output_new(OF_VERSION_1_0); 
    
    /* Match on in-port */
    ma->fields.in_port = inport; 
    ma->masks.in_port=~0;

    OF_CHECK(of_flow_add_match_set(fa, ma)); 

    /* Output to out-port */
    of_action_output_port_set(ao, outport); 
    of_list_action_append(al, (of_action_t*)ao); 

    OF_CHECK(of_flow_add_actions_set(fa, al)); 
    
    OF_CHECK(sofc_message_send(sc->sofc, sc->id, fa)); 
    SOFC_DISCONNECT(); 
    return UCLI_STATUS_OK; 
}
    
    
static ucli_status_t
sofc_ucli_ucli__pirate__(ucli_context_t* uc)
{
    int seconds; 
    uint64_t until; 
    uint64_t now; 
    uint64_t last; 
    uint64_t start; 
    int count; 

    UCLI_COMMAND_INFO(uc, 
                      "pirate", -1, 
                      "$summary#Measure packet-in rate."
                      "$args#[seconds]"); 

    switch(uc->pargs->count)
        {
        case 0:
            seconds = 0; 
            break; 
        case 1:
            UCLI_ARGPARSE_OR_RETURN(uc, "i", &seconds); 
            break; 
        default:
            return UCLI_STATUS_E_ARG;
        }; 

    SOFC_CONNECTED_OR_RETURN(); 

    count = 0;     
    start = os_time_monotonic(); 
    until = start + 1000000*seconds;
    last = 0; 
    now = start; 
    while( (seconds == 0) || (now < until)) { 
        uint8_t msg[OF_WIRE_BUFFER_MAX_LENGTH]; 
        if( (sofc_message_type_recv(sc->sofc, sc->id, OF_PACKET_IN, msg)) < 0) { 
            ucli_printf(uc, "recv failed."); 
            return UCLI_STATUS_E_ERROR; 
        }
        count++; 
        if(last == 0) { 
            last = now; 
        }
        if( (now - last) > 1000000) { 
            ucli_printf(uc, "%f pps (%d packets in %f seconds)\n", 
                        1000000.0*count / (now - start), 
                        count, 
                        1.0*(now - start) / 1000000); 
            last = now; 
        }
        now = os_time_monotonic(); 
    }

    SOFC_DISCONNECT(); 

    ucli_printf(uc, "%d packets in %d seconds (%f pps)\n", count, seconds, 
                1.0*count/seconds); 
    
    return UCLI_STATUS_OK; 
    }
    
/* <auto.ucli.handlers.start> */
/******************************************************************************
 *
 * These handler table(s) were autogenerated from the symbols in this 
 * source file. 
 *
 *****************************************************************************/
static ucli_command_handler_f sofc_ucli_ucli_handlers__[] = 
{
    sofc_ucli_ucli__server__, 
    sofc_ucli_ucli__connect__, 
    sofc_ucli_ucli__trace__, 
    sofc_ucli_ucli__dump__, 
    sofc_ucli_ucli__bsn_shell__, 
    sofc_ucli_ucli__forward__, 
    sofc_ucli_ucli__pirate__, 
    NULL
};
/******************************************************************************/
/* <auto.ucli.handlers.end> */

ucli_module_t sc_module = { 
    "sofc", 
    &sofc_ctrl__, 
    sofc_ucli_ucli_handlers__, 
    NULL, 
    NULL
}; 

int 
aim_main(int argc, char* argv[])
{
    ucli_t* u;
    
    memset(&sofc_ctrl__, 0, sizeof(sofc_ctrl__)); 

    ucli_init(); 
    ucli_module_init(&sc_module); 
    u = ucli_create(NULL, &sc_module, NULL); 
    if(argc == 1) { 
        char* def[] = 
            {
                NULL, 
                "-server", "127.0.0.1:6633", 
                "-dump", 
                NULL
            }; 
        def[0] = argv[0]; 
        argc = AIM_ARRAYSIZE(def); 
        argv = def; 
    }
    ucli_dispatch_argv(u, &aim_pvs_stdout, argv+1);
    return 0; 
}
