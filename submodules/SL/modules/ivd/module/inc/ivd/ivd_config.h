#ifndef __IVD_CONFIG_H__
#define __IVD_CONFIG_H__

/* <auto.start.cdefs(IVD_CONFIG_HEADER).header> */
#include <AIM/aim.h>
/**
 * IVD_CONFIG_LOG_OPTIONS_DEFAULT
 *
 * Default enabled log options. */


#ifndef IVD_CONFIG_LOG_OPTIONS_DEFAULT
#define IVD_CONFIG_LOG_OPTIONS_DEFAULT AIM_LOG_OPTIONS_DEFAULT
#endif

/**
 * IVD_CONFIG_LOG_BITS_DEFAULT
 *
 * Default enabled log bits. */


#ifndef IVD_CONFIG_LOG_BITS_DEFAULT
#define IVD_CONFIG_LOG_BITS_DEFAULT AIM_LOG_BITS_DEFAULT
#endif

/**
 * IVD_CONFIG_LOG_CUSTOM_BITS_DEFAULT
 *
 * Default enabled custom log bits. */


#ifndef IVD_CONFIG_LOG_CUSTOM_BITS_DEFAULT
#define IVD_CONFIG_LOG_CUSTOM_BITS_DEFAULT 0
#endif

/**
 * IVD_CONFIG_PORTING_STDLIB
 *
 * Default all porting macros to use the C standard libraries. */


#ifndef IVD_CONFIG_PORTING_STDLIB
#define IVD_CONFIG_PORTING_STDLIB 1
#endif

/**
 * IVD_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
 *
 * Include standard library headers for stdlib porting macros. */


#ifndef IVD_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
#define IVD_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS IVD_CONFIG_PORTING_STDLIB
#endif

/**
 * IVD_CONFIG_ZTN_JSON
 *
 * Default location for ztn.json */


#ifndef IVD_CONFIG_ZTN_JSON
#define IVD_CONFIG_ZTN_JSON "/mnt/flash/boot/ztn.json"
#endif



/**
 * All compile time options can be queried or displayed
 */

/** Configuration settings structure. */
typedef struct ivd_config_settings_s {
    /** name */
    const char* name;
    /** value */
    const char* value;
} ivd_config_settings_t;

/** Configuration settings table. */
/** ivd_config_settings table. */
extern ivd_config_settings_t ivd_config_settings[];

/**
 * @brief Lookup a configuration setting.
 * @param setting The name of the configuration option to lookup.
 */
const char* ivd_config_lookup(const char* setting);

/**
 * @brief Show the compile-time configuration.
 * @param pvs The output stream.
 */
int ivd_config_show(struct aim_pvs_s* pvs);

/* <auto.end.cdefs(IVD_CONFIG_HEADER).header> */

#endif /* __IVD_CONFIG_H__ */
