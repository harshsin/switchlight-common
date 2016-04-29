/****************************************************************
 *
 *        Copyright 2015, Big Switch Networks, Inc.
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *        http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 *
 ****************************************************************/

#include "pdua_int.h"
#define AIM_LOG_MODULE_NAME pdua
#include <AIM/aim_log.h>

static indigo_core_gentable_t *gentable__ = NULL;
static const indigo_core_gentable_ops_t table_ops__;

static bool init_done__ = false;

indigo_error_t
pdua_rx_table_init(void)
{
    if (init_done__) {
        AIM_LOG_INFO("%s: pdu-rx table already initialized", __FUNCTION__);
        return INDIGO_ERROR_NONE;
    }

    indigo_core_gentable_register("pdu-rx", &table_ops__, NULL, 1, 4,
                                  &gentable__);
    init_done__ = true;
    return INDIGO_ERROR_NONE;
}

indigo_error_t
pdua_rx_table_finish(void)
{
    if (init_done__) {
        indigo_core_gentable_unregister(gentable__);
    }
    init_done__ = false;
    return INDIGO_ERROR_NONE;
}

static indigo_error_t
parse_key__(of_list_bsn_tlv_t *key,
            of_octets_t *name)
{
    of_object_t tlv;

    /* name */
    if (of_list_bsn_tlv_first(key, &tlv) < 0) {
        AIM_LOG_ERROR("%s: empty key list", __FUNCTION__);
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_NAME) {
        of_bsn_tlv_name_value_get(&tlv, name);
    } else {
        AIM_LOG_ERROR("%s: expected name, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    /* end of list */
    if (of_list_bsn_tlv_next(key, &tlv) == 0) {
        AIM_LOG_ERROR("%s: expected end of key list, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
parse_value__(of_list_bsn_tlv_t *value,
              of_port_no_t *port_no,
              uint32_t *timeout,
              of_octets_t *data)
{
    of_object_t tlv;

    /* port */
    if (of_list_bsn_tlv_first(value, &tlv) < 0) {
        AIM_LOG_ERROR("%s: empty value list", __FUNCTION__);
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_PORT) {
        of_bsn_tlv_port_value_get(&tlv, port_no);
    } else {
        AIM_LOG_ERROR("%s: expected port, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    /* idle timeout */
    if (of_list_bsn_tlv_next(value, &tlv) < 0) {
        AIM_LOG_ERROR("%s: not expecting end of value list", __FUNCTION__);
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_IDLE_TIMEOUT) {
        of_bsn_tlv_idle_timeout_value_get(&tlv, timeout);
    } else {
        AIM_LOG_ERROR("%s: expected idle timeout, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    /* data */
    if (of_list_bsn_tlv_next(value, &tlv) < 0) {
        AIM_LOG_ERROR("%s: not expecting end of value list", __FUNCTION__);
        return INDIGO_ERROR_PARAM;
    }

    if (tlv.object_id == OF_BSN_TLV_DATA) {
        of_bsn_tlv_data_value_get(&tlv, data);
    } else {
        AIM_LOG_ERROR("%s: expected data, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    /* end of list */
    if (of_list_bsn_tlv_next(value, &tlv) == 0) {
        AIM_LOG_ERROR("%s: expected end of value list, instead got %s",
                      __FUNCTION__, of_class_name(&tlv));
        return INDIGO_ERROR_PARAM;
    }

    return INDIGO_ERROR_NONE;
}


static indigo_error_t
entry_add__(void *table_priv,
            of_list_bsn_tlv_t *key,
            of_list_bsn_tlv_t *value,
            void **entry_priv)
{
    indigo_error_t rv;
    of_octets_t name;
    of_port_no_t port_no;
    of_octets_t data;
    uint32_t timeout;
    pdua_port_t *port;

    rv = parse_key__(key, &name);
    if (rv != INDIGO_ERROR_NONE) {
        AIM_LOG_ERROR("%s: error parsing key", __FUNCTION__);
        return rv;
    }

    rv = parse_value__(value, &port_no, &timeout, &data);
    if (rv != INDIGO_ERROR_NONE) {
        AIM_LOG_ERROR("%s: error parsing value", __FUNCTION__);
        return rv;
    }

    port = pdua_find_port(port_no);
    if (port == NULL) {
        AIM_LOG_ERROR("%s: pdua port not found for OF port %u",
                      __FUNCTION__, port_no);
        return INDIGO_ERROR_NOT_FOUND;
    }

    rv = pdua_port_rx_enable(port, &data, timeout);
    if (rv != INDIGO_ERROR_NONE) {
        AIM_LOG_ERROR("%s: error enabling pdua rx on OF port %u",
                      __FUNCTION__, port_no);
        return rv;
    }

    /* FIXME: use entry_priv for packet instance */
    *entry_priv = port;

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
entry_modify__(void *table_priv,
               void *entry_priv,
               of_list_bsn_tlv_t *key,
               of_list_bsn_tlv_t *value)
{
    indigo_error_t rv;
    of_port_no_t port_no;
    of_octets_t data;
    uint32_t timeout;
    pdua_port_t *port;

    rv = parse_value__(value, &port_no, &timeout, &data);
    if (rv != INDIGO_ERROR_NONE) {
        AIM_LOG_ERROR("%s: error parsing value", __FUNCTION__);
        return rv;
    }

    port = (pdua_port_t *)entry_priv;
    AIM_ASSERT(port->port_no == port_no);

    /* disable first */
    /* FIXME: compare config and see if modify is necessary */
    rv = pdua_port_rx_disable(port);
    if (rv != INDIGO_ERROR_NONE) {
        AIM_LOG_ERROR("%s: error disabling pdua rx on OF port %u",
                      __FUNCTION__, port_no);
        return rv;
    }

    rv = pdua_port_rx_enable(port, &data, timeout);
    if (rv != INDIGO_ERROR_NONE) {
        AIM_LOG_ERROR("%s: error enabling pdua rx on OF port %u",
                      __FUNCTION__, port_no);
        return rv;
    }

    return INDIGO_ERROR_NONE;
}

static indigo_error_t
entry_del__(void *table_priv,
            void *entry_priv,
            of_list_bsn_tlv_t *key)
{
    indigo_error_t rv;
    pdua_port_t *port;

    port = (pdua_port_t *)entry_priv;
    rv = pdua_port_rx_disable(port);
    if (rv != INDIGO_ERROR_NONE) {
        AIM_LOG_ERROR("%s: error disabling pdua rx on OF PORT %u",
                      __FUNCTION__, port->port_no);
        return rv;
    }

    return INDIGO_ERROR_NONE;
}

static void
get_stats__(void *table_priv,
            void *entry_priv,
            of_list_bsn_tlv_t *key,
            of_list_bsn_tlv_t *stats)
{
}

static const indigo_core_gentable_ops_t table_ops__ = {
    .add = entry_add__,
    .modify = entry_modify__,
    .del = entry_del__,
    .get_stats = get_stats__,
};
