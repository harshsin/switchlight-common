/**************************************************************************//**
 *
 * @file
 * @brief Indigo Virtual Switch. 
 *
 *
 * @addtogroup ivs-ivs
 * @{
 *
 *****************************************************************************/
#ifndef __IVS_H__
#define __IVS_H__

#include <dependmodules.x>

#include <IVS/ivs_config.h>

#include <SocketManager/socketmanager.h>
#include <OFConnectionManager/ofconnectionmanager.h>
#include <PortManager/portmanager.h>
#include <Forwarding/forwarding.h>
#include <OFStateManager/ofstatemanager.h>

#ifdef DEPENDMODULE_INCLUDE_AET
#include <AET/aet.h>
#endif

#ifdef DEPENDMODULE_INCLUDE_SNMP
#include <snmp/snmp.h>
#endif

#if IVS_CONFIG_INCLUDE_CXN_LOG == 1
#include <BigRing/bigring.h>
#endif

#include <loci/loci.h>

#include <BigList/biglist.h>

#include <uCli/ucli.h>

#if IVS_CONFIG_INCLUDE_NETWORK_CLI == 1
#include <NSS/nss.h>
#endif

#if IVS_CONFIG_INCLUDE_CONSOLE_CLI == 1
#include <ELS/els.h>
#endif

/* <auto.start.enum(ALL).header> */
/** ivs_cxn_role */
typedef enum ivs_cxn_role_e {
    IVS_CXN_ROLE_UNKNOWN = INDIGO_CXN_R_UNKNOWN,
    IVS_CXN_ROLE_MASTER = INDIGO_CXN_R_MASTER,
    IVS_CXN_ROLE_SLAVE = INDIGO_CXN_R_SLAVE,
    IVS_CXN_ROLE_EQUAL = INDIGO_CXN_R_EQUAL,
} ivs_cxn_role_t;

/** Enum names. */
const char* ivs_cxn_role_name(ivs_cxn_role_t e);

/** Enum values. */
int ivs_cxn_role_value(const char* str, ivs_cxn_role_t* e, int substr);

/** Enum descriptions. */
const char* ivs_cxn_role_desc(ivs_cxn_role_t e);

/** Enum validator. */
int ivs_cxn_role_valid(ivs_cxn_role_t e);

/** validator */
#define IVS_CXN_ROLE_VALID(_e) \
    (ivs_cxn_role_valid((_e)))

/** ivs_cxn_role_map table. */
extern aim_map_si_t ivs_cxn_role_map[];
/** ivs_cxn_role_desc_map table. */
extern aim_map_si_t ivs_cxn_role_desc_map[];

/** ivs_cxn_state */
typedef enum ivs_cxn_state_e {
    IVS_CXN_STATE_DISCONNECTED = INDIGO_CXN_S_DISCONNECTED,
    IVS_CXN_STATE_CONNECTING = INDIGO_CXN_S_CONNECTING,
    IVS_CXN_STATE_CONNECTED = INDIGO_CXN_S_HANDSHAKE_COMPLETE,
    IVS_CXN_STATE_CLOSING = INDIGO_CXN_S_CLOSING,
} ivs_cxn_state_t;

/** Enum names. */
const char* ivs_cxn_state_name(ivs_cxn_state_t e);

/** Enum values. */
int ivs_cxn_state_value(const char* str, ivs_cxn_state_t* e, int substr);

/** Enum descriptions. */
const char* ivs_cxn_state_desc(ivs_cxn_state_t e);

/** Enum validator. */
int ivs_cxn_state_valid(ivs_cxn_state_t e);

/** validator */
#define IVS_CXN_STATE_VALID(_e) \
    (ivs_cxn_state_valid((_e)))

/** ivs_cxn_state_map table. */
extern aim_map_si_t ivs_cxn_state_map[];
/** ivs_cxn_state_desc_map table. */
extern aim_map_si_t ivs_cxn_state_desc_map[];

/** ivs_cxn_type */
typedef enum ivs_cxn_type_e {
    IVS_CXN_TYPE_CONNECT,
    IVS_CXN_TYPE_LISTEN,
    IVS_CXN_TYPE_LAST = IVS_CXN_TYPE_LISTEN,
    IVS_CXN_TYPE_COUNT,
    IVS_CXN_TYPE_INVALID = -1,
} ivs_cxn_type_t;

/** Strings macro. */
#define IVS_CXN_TYPE_STRINGS \
{\
    "CONNECT", \
    "LISTEN", \
}
/** Enum names. */
const char* ivs_cxn_type_name(ivs_cxn_type_t e);

/** Enum values. */
int ivs_cxn_type_value(const char* str, ivs_cxn_type_t* e, int substr);

/** Enum descriptions. */
const char* ivs_cxn_type_desc(ivs_cxn_type_t e);

/** validator */
#define IVS_CXN_TYPE_VALID(_e) \
    ( (0 <= (_e)) && ((_e) <= IVS_CXN_TYPE_LISTEN))

/** ivs_cxn_type_map table. */
extern aim_map_si_t ivs_cxn_type_map[];
/** ivs_cxn_type_desc_map table. */
extern aim_map_si_t ivs_cxn_type_desc_map[];
/* <auto.end.enum(ALL).header> */


/**
 * This structure represents a single connection. 
 */
typedef struct ivs_cxn_s { 
    
    /** The protocol parameters for this connection. */
    indigo_cxn_protocol_params_t cxn_proto_params; 
    /** The configuration parameters for this connection. */
    indigo_cxn_config_params_t cxn_config_params; 
    /** The connection id for this connection . */
    indigo_cxn_id_t cxn_id; 

} ivs_cxn_t; 

/**
 * This structure represents a single configuration option. 
 */
typedef struct ivs_option_s { 
    /** Option command. */
    char* string; 
    /** Whether this option has already been processed. */
    int processed; 
} ivs_option_t; 

/**
 * The IVS structure represents the collection of all Indigo
 * objects necessary to run the switch. 
 */
typedef struct ivs_s { 
    
    /** The Indigo SocketManager configuration data. */
    ind_soc_config_t soc; 
    /** The Indigo Connection Manager configuration data. */
    ind_cxn_config_t cxn; 
    /** The Indigo Port Manager configuration data. */
    ind_port_config_t port; 
    /** The Indigo Forwarding configuration data. */
    ind_fwd_config_t fwd; 
    /** The Indigo Core configuration data. */
    ind_core_config_t core; 
    
    /** The Indigo Port configuration for added ports. */
    indigo_port_config_t port_add_config; 
    
    /** The total number of active connections. */
    int connection_count; 

    /** Interface index counter */
    int interface_counter; 

    /** startup options */
    ivs_option_t* string_options; 
    /** startup options [argv] */
    ivs_option_t* argv_options; 

    /** options processing */
    ucli_t* ucli; 

    /** network cli services, if enabled */
#if IVS_CONFIG_INCLUDE_NETWORK_CLI == 1
    struct { 
        ucli_t* ucli; 
        nss_t* nss; 
    } netif; 
#endif
    
#if IVS_CONFIG_INCLUDE_CONSOLE_CLI == 1
    /** console cli services, if enabled */
    struct { 
        ucli_t* ucli; 
        els_t* els; 
    } consif;
#endif

    /** Watchdog support */
    int watchdog_seconds; 

    /** Internal testing */
    int min; 

#if IVS_CONFIG_INCLUDE_CXN_LOG == 1
    bigring_t* cxn_log_ring; 
#endif

#if IVS_CONFIG_INCLUDE_STATUS_LOG == 1
    bigring_t* status_log_ring;
#endif
} ivs_t; 




/**
 * @brief Create an IVS instance. 
 * @param [out] ivs Returns the IVS instance. 
 * @param string_options Array of string options or commands. 
 * @param argv_options Program ARGV array containing options. 
 */    
int ivs_create(ivs_t** ivs, 
               char** string_options, 
               char** argv_options); 

/**
 * @brief Initialize an IVS instance. 
 * @param ivs The IVS instance. 
 */
int ivs_init(ivs_t* ivs); 

/**
 * @brief Enable or disable an IVS instance. 
 * @brief ivs The IVS instance. 
 */
int ivs_enable(ivs_t* ivs, int enable); 

/**
 * @brief Enable or disable the watchdog handler.
 * @param ivs The IVS instance. 
 * @param seconds The maximum allowable handler execution time. 
 */
int ivs_watchdog_enable(ivs_t* ivs, int seconds); 



/**
 * @brief Run an IVS for the given number of milliseconds. 
 * @param ivs The IVS instance. 
 * @param ms Milliseconds to run. 
 */
int ivs_run(ivs_t* ivs, int ms); 

/**
 * @brief Destroy an IVS instance. 
 * @param ivs The IVS instance. 
 */
int ivs_destroy(ivs_t* ivs); 



/**
 * @brief Add a controller connection to the given IVS. 
 * @param ivs The IVS instance. 
 * @param type The type of connection to add. 
 * @param caddr The controller IP address. 
 * @param cport The controller port. 
 * @returns INDIGO_ERROR_NONE on success. 
 * @returns INDIGO_ERROR_EXISTS if the connection already exists. 
 * @returns INDIGO_ERROR_XXX for other conditions. 
 */
int ivs_controller_add(ivs_t* ivs, ivs_cxn_type_t type, 
                       const char* caddr, int cport); 

/**
 * @brief Remove a controller connection from the given IVS. 
 * @param ivs The IVS instance. 
 * @param type The type of connection to remove. 
 * @param caddr The controller IP address. 
 * @param cport The controller port. 
 * @returns INDIGO_ERROR_NON on success. 
 * @returns INDIGO_ERROR_NOT_FOUND if the connection was not found. 
 * @returns INDIGO_ERROR_XXX for other conditions. 
 */
int ivs_controller_remove(ivs_t* ivs, ivs_cxn_type_t type,  
                          const char* caddr, int cport); 

/**
 * @brief Determine if a controller connection exists in the given IVS. 
 * @param ivs The IVS instance.
 * @param type The connection type. 
 * @param caddr The controller IP address. 
 * @param cport The controller port. 
 * @returns 1 if the connection exists. 
 * @returns 0 if the connection does not exist. 
 * @returns INDIGO_ERROR_XXX for other error conditions. 
 */
int ivs_controller_exists(ivs_t* ivs, ivs_cxn_type_t type, 
                          const char* caddr, int cport); 


/**
 * @brief Show controller connections by type.
 * @param ivs The IVS instance.
 * @param type The connection type.
 * @param pvs The output stream. 
 */
int ivs_controller_show(ivs_t* ivs, ivs_cxn_type_t type, aim_pvs_t* pvs); 

/**
 * @brief Add an interface.
 * @param ivs The IVS instance. 
 * @param interface The interface specification string.
 * @param index The ofindex number. 
 */
int ivs_interface_add(ivs_t* ivs, const char* interface, int index); 

/**
 * @brief Remove an interface. 
 * @param ivs The IVS instance. 
 * @param index The ofindex number. 
 */
int ivs_interface_remove(ivs_t* ivs, int index); 

/**
 * @brief Show interfaces. 
 * @param ivs The IVS instance. 
 * @param pvs The output stream. 
 */
int ivs_interface_show(ivs_t* ivs, aim_pvs_t* pvs); 

/**
 * @brief Returns whether the given interface index exists. 
 * @param ivs The IVS instance. 
 * @param index The index. 
 */
int ivs_interface_exists(ivs_t* ivs, int index); 

/**
 * @brief Returns the next available interface index. 
 * @param ivs The IVS instance. 
 */
int ivs_interface_next(ivs_t* ivs); 

/**
 * @brief Execute a command. 
 * @param ivs The IVS instance. 
 * @param cmd The command string to execute. 
 */
int ivs_command(ivs_t* ivs, const char* cmd); 


#if IVS_CONFIG_INCLUDE_CXN_LOG == 1

/**
 * @brief Show the connection log. 
 * @param ivs The IVS instance. 
 * @param pvs The output pvs. 
 */
int ivs_cxn_log_show(ivs_t* ivs, aim_pvs_t* pvs); 

#endif

/**
 * @brief Create, initialize and run an IVS instances with the given options. 
 * @param string_options Array of string options or commands. 
 * @param argv_options Program ARGV array containing options. 
 */
int ivs_main(char** string_options, 
             char** argv_options); 

#endif /* __IVS_H__ */
/*@}*/
