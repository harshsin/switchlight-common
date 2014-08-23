#include <dependmodules.x>
#include <uCli/ucli.h>
#include <indigo/port_manager.h>
#include <indigo/error.h>
#include <PortManager/portmanager.h>
#include <Forwarding/forwarding.h>
#include <snmp/snmp_port_stats.h>
#include <cjson/cJSON.h>
#include <ivd/ivd_config.h>
#include <ivd/ivd_porting.h>

#include "ivd_log.h"

/*********************
 * Utility Functions *
 ********************/

static void
_swi_md5_set(of_bsn_image_desc_stats_reply_t *reply, const char *md5)
{
    char s[OF_DESC_STR_LEN];
    IVD_MEMSET((void *) s, 0, OF_DESC_STR_LEN);
    strncpy(s, md5, OF_DESC_STR_LEN-1);
    of_bsn_image_desc_stats_reply_image_checksum_set(reply, s);
}

static void
_cfg_md5_set(of_bsn_image_desc_stats_reply_t *reply, const char *md5)
{
    char s[OF_DESC_STR_LEN];
    IVD_MEMSET((void *) s, 0, OF_DESC_STR_LEN);
    strncpy(s, md5, OF_DESC_STR_LEN-1);
    of_bsn_image_desc_stats_reply_startup_config_checksum_set(reply, s);
}

static const char *
_cjson_get_string(cJSON *node, const char *key)
{
    cJSON *subnode = cJSON_GetObjectItem(node, key);
    if (subnode == NULL)
        return NULL;
    return subnode->valuestring;
}

static cJSON *
_get_json(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        LOG_WARN("cannot open file %s", filename);
        return NULL;
    }
    int sts = fseek(f, 0, SEEK_END);
    if (sts < 0) {
        LOG_WARN("cannot seek in file %s", filename);
        fclose(f);
        return NULL;
    }

    long sz = ftell(f);
    if (sz < 0) {
        LOG_WARN("cannot seek in file %s", filename);
        fclose(f);
        return NULL;
    }

    char *fileBuf = aim_zmalloc(sz + 1);
    if (fileBuf == 0) {
        fclose(f);
        return NULL;
    }

    fseek(f, 0, SEEK_SET);
    int nrd = fread(fileBuf, 1, sz, f);
    fclose(f);

    if (nrd == 0 || (nrd < sz)) {
        LOG_WARN("cannot read json in %s (read %d out of %d bytes)",
                     filename, nrd, sz);
        aim_free(fileBuf);
        return NULL;
    }

    cJSON *mf = cJSON_Parse(fileBuf);
    aim_free(fileBuf);

    if (mf == NULL)
        LOG_WARN("cannot parse json in %s", filename);

    return mf;
}

/********
 * Init *
 *******/

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
    indigo_error_t rv;

    switch (experimenter->object_id) {

    /* Ported from brcmdriver3 */
    case OF_BSN_IMAGE_DESC_STATS_REQUEST:
    {
        of_bsn_image_desc_stats_reply_t *output;
        uint32_t xid;

        of_bsn_image_desc_stats_request_xid_get(experimenter, &xid);
        output = of_bsn_image_desc_stats_reply_new(experimenter->version);
        of_bsn_image_desc_stats_reply_xid_set(output, xid);

        const char *filename = IVD_CONFIG_ZTN_JSON;
        cJSON *mf = _get_json(filename);

        if (mf != NULL) {
            const char *swi_md5 = _cjson_get_string(mf, "swi_md5");
            if (swi_md5 != NULL) {
                LOG_INFO("setting swi md5 %s", swi_md5);
                _swi_md5_set(output, swi_md5);
            }
            const char *cfg_md5 = _cjson_get_string(mf, "startup_config_md5");
            if (cfg_md5 != NULL) {
                LOG_INFO("setting config md5 %s", cfg_md5);
                _cfg_md5_set(output, cfg_md5);
            }
            cJSON_Delete(mf);
        }

        LOG_INFO("sending response %d", cxn_id);
        indigo_cxn_send_controller_message(cxn_id, output);
        rv = INDIGO_ERROR_NONE;
        break;
    }

    default:
        rv = INDIGO_ERROR_NOT_SUPPORTED;
    }

    return rv;
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
    return INDIGO_ERROR_NOT_SUPPORTED;
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
