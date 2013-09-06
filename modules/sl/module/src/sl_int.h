/**************************************************************************//**
 *
 * sl Internal Header
 *
 *****************************************************************************/
#ifndef __SL_INT_H__
#define __SL_INT_H__

#include <sl/sl_config.h>
#include <loci/loci.h>
#include <indigo/memory.h>
#include <indigo/types.h>
#include <indigo/error.h>


indigo_error_t sl_bsn_shell_handler(of_experimenter_t* experimenter, 
                                    indigo_cxn_id_t id, void* cookie); 

int sl_bsn_shell_handler_register(void); 

/* <auto.start.enum(ALL).header> */
/** sl_shell_service_type */
typedef enum sl_shell_service_type_e {
    SL_SHELL_SERVICE_TYPE_RESERVED,
    SL_SHELL_SERVICE_TYPE_SHELL,
    SL_SHELL_SERVICE_TYPE_CLI,
    SL_SHELL_SERVICE_TYPE_LAST = SL_SHELL_SERVICE_TYPE_CLI,
    SL_SHELL_SERVICE_TYPE_COUNT,
    SL_SHELL_SERVICE_TYPE_INVALID = -1,
} sl_shell_service_type_t;

/** Strings macro. */
#define SL_SHELL_SERVICE_TYPE_STRINGS \
{\
    "RESERVED", \
    "SHELL", \
    "CLI", \
}
/** Enum names. */
const char* sl_shell_service_type_name(sl_shell_service_type_t e);

/** Enum values. */
int sl_shell_service_type_value(const char* str, sl_shell_service_type_t* e, int substr);

/** Enum descriptions. */
const char* sl_shell_service_type_desc(sl_shell_service_type_t e);

/** validator */
#define SL_SHELL_SERVICE_TYPE_VALID(_e) \
    ( (0 <= (_e)) && ((_e) <= SL_SHELL_SERVICE_TYPE_CLI))

/** sl_shell_service_type_map table. */
extern aim_map_si_t sl_shell_service_type_map[];
/** sl_shell_service_type_desc_map table. */
extern aim_map_si_t sl_shell_service_type_desc_map[];
/* <auto.end.enum(ALL).header> */

#endif /* __SL_INT_H__ */
