/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <sl/sl_config.h>

#include "sl_log.h"
#include "sl_int.h"

#include <AIM/aim_pvs_buffer.h>
#include <dependmodules.x>

#ifdef DEPENDMODULE_INCLUDE_BRCMDRIVER
#include <BRCMDriver/brcmdriver.h>
#endif

#include <OFStateManager/ofstatemanager.h>

static indigo_error_t
service_handler_complete__(of_experimenter_t* experimenter, 
                           indigo_cxn_id_t cxn_id, 
                           aim_pvs_t* opvs, 
                           int rc)
{
    int rv; 
    uint32_t xid; 
    of_bsn_shell_output_t* output = NULL; 
    of_bsn_shell_status_t* status = NULL; 
    of_octets_t octets = { 0, 0 }; 

    of_bsn_shell_command_xid_get(experimenter, &xid); 

    /* Generate a shell output response */
    output = of_bsn_shell_output_new(experimenter->version); 
    of_bsn_shell_output_xid_set(output, xid); 

    octets.data = (unsigned char*)aim_pvs_buffer_get(opvs); 
    octets.bytes = SL_STRLEN((char*)octets.data)+1; 
    
    if(of_bsn_shell_output_data_set(output, &octets) < 0) { 
        AIM_LOG_ERROR("error setting output data."); 
        rv = INDIGO_ERROR_UNKNOWN; 
        goto finish; 
    }
    
    if(indigo_cxn_send_controller_message(cxn_id, output) < 0) { 
        AIM_LOG_ERROR("Error sending shell output response."); 
        rv = INDIGO_ERROR_UNKNOWN; 
        goto finish; 
    }
                
    /* Send the status code */
    status = of_bsn_shell_status_new(experimenter->version); 
    of_bsn_shell_status_xid_set(status, xid); 
    of_bsn_shell_status_status_set(status, rc); 
                
    if(indigo_cxn_send_controller_message(cxn_id, status) < 0) { 
        AIM_LOG_ERROR("Error sending shell status response."); 
        rv = INDIGO_ERROR_UNKNOWN; 
        goto finish;
    }
    
    rv = INDIGO_ERROR_NONE; 

 finish:

    if(octets.data) aim_free(octets.data); 
    if(opvs) aim_pvs_destroy(opvs); 

    return rv; 
}

/* Temporary */
static indigo_error_t 
respond_unsupported__(of_experimenter_t* experimenter, indigo_cxn_id_t id, 
                      const char* type)
{
    aim_pvs_t* pvs = aim_pvs_buffer_create(); 
    AIM_LOG_WARN("bsn shell service %s is not yet supported.", type); 
    aim_printf(pvs, "bsn shell service %s is not yet supported.\n", type); 
    return service_handler_complete__(experimenter, id, pvs, -1); 
}
    


/*
 * Shell Service Handlers 
 */
static indigo_error_t
service_handler_SHELL__(of_experimenter_t* experimenter, indigo_cxn_id_t id, 
                        char* command)
{
    return respond_unsupported__(experimenter, id, "SHELL"); 
}

static indigo_error_t
service_handler_CLI__(of_experimenter_t* experimenter, indigo_cxn_id_t id, 
                      char* command)
{
    return respond_unsupported__(experimenter, id, "CLI"); 
}

static indigo_error_t
service_handler_RESERVED__(of_experimenter_t* experimenter, indigo_cxn_id_t id, 
                           char* command)
{
    return respond_unsupported__(experimenter, id, "RESERVED"); 
}

int
sl_bsn_shell_handler_register(void)
{
#ifdef DEPENDMODULE_INCLUDE_BRCMDRIVER
    return brcmdriver_experimenter_handler_register(sl_bsn_shell_handler, NULL); 
#else
    return -1;
#endif
}


indigo_error_t
sl_bsn_shell_handler(of_experimenter_t* experimenter, indigo_cxn_id_t id, void* cookie)
{
    indigo_error_t rv = INDIGO_ERROR_NOT_SUPPORTED; 
    AIM_LOG_MSG("sl_bsn_shell_handler"); 

    switch (experimenter->object_id) 
        {
        case OF_BSN_SHELL_COMMAND:
            {
                uint32_t service; 
                of_octets_t data; 
 
                of_bsn_shell_command_service_get(experimenter, &service); 
                of_bsn_shell_command_data_get(experimenter, &data); 

                switch(service)
                    {
#define SL_SHELL_SERVICE_TYPE_ENTRY(_type)                              \
                        case SL_SHELL_SERVICE_TYPE_##_type:             \
                            {                                           \
                                rv = service_handler_##_type##__(experimenter, id, (char*)data.data); \
                                break;                                  \
                            }
#include <sl/sl.x>
                    default: 
                        {
                            rv = INDIGO_ERROR_NOT_SUPPORTED; 
                        }       
                        break;
                    }
                break;
            }
        default:
            {
                rv = INDIGO_ERROR_NOT_SUPPORTED; 
                break; 
            }
        }
    
    return rv;
}
