/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <igmpa/igmpa_config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <AIM/aim.h>
#include <indigo/indigo.h>
#include <SocketManager/socketmanager.h>
#include <OS/os.h>
#include <PPE/ppe.h>

#include <igmpa/igmpa.h>


/* bad igmp checksum */
uint8_t igmp_bad_checksum[] = {
    0x01, 0x00, 0x5e, 0x00, 0x00, 0x01, 0x00, 0x01,
    0x02, 0x03, 0x04, 0x05, 0x81, 0x00, 0x00, 0x0a,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x1c, 0x00, 0x01,
    0x00, 0x00, 0x40, 0x02, 0x98, 0xdc, 0x01, 0x01,
    0x01, 0x01, 0xe0, 0x00, 0x00, 0x01, 0x11, 0x14,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/* mrouter 00:01:02:03:04:05 1.1.1.1->224.0.0.1, type 0x11 */
uint8_t igmp_general_query[] = {
    0x01, 0x00, 0x5e, 0x00, 0x00, 0x01, 0x00, 0x01,
    0x02, 0x03, 0x04, 0x05, 0x81, 0x00, 0x00, 0x0a,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x1c, 0x00, 0x01,
    0x00, 0x00, 0x40, 0x02, 0x98, 0xdc, 0x01, 0x01,
    0x01, 0x01, 0xe0, 0x00, 0x00, 0x01, 0x11, 0x14,
    0xee, 0xeb, 0x00, 0x00, 0x00, 0x00,
};

/* mrouter 00:01:02:03:04:05 1.1.1.1->225.1.1.1, type 0x11 */
uint8_t igmp_group_specific_query[] = {
    0x01, 0x00, 0x5e, 0x01, 0x01, 0x01, 0x00, 0x01,
    0x02, 0x03, 0x04, 0x05, 0x81, 0x00, 0x00, 0x0a,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x1c, 0x00, 0x01,
    0x00, 0x00, 0x40, 0x02, 0x96, 0xdb, 0x01, 0x01,
    0x01, 0x01, 0xe1, 0x01, 0x01, 0x01, 0x11, 0x14,
    0x0c, 0xe9, 0xe1, 0x01, 0x01, 0x01,
};

/* host 00:0a:0b:0c:0d:0e 2.2.2.2->225.1.1.1, type 0x12 */
uint8_t igmp_v1_report[] = {
    0x01, 0x00, 0x5e, 0x01, 0x01, 0x01, 0x00, 0x0a,
    0x0b, 0x0c, 0x0d, 0x0e, 0x81, 0x00, 0x00, 0x0a,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x1c, 0x00, 0x01,
    0x00, 0x00, 0x40, 0x02, 0x94, 0xd9, 0x02, 0x02,
    0x02, 0x02, 0xe1, 0x01, 0x01, 0x01, 0x12, 0x00,
    0x0b, 0xfd, 0xe1, 0x01, 0x01, 0x01,
};

/* host 00:0a:0b:0c:0d:0e 2.2.2.2->225.1.1.1, type 0x16 */
uint8_t igmp_v2_report[] = {
    0x01, 0x00, 0x5e, 0x01, 0x01, 0x01, 0x00, 0x0a,
    0x0b, 0x0c, 0x0d, 0x0e, 0x81, 0x00, 0x00, 0x0a,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x1c, 0x00, 0x01,
    0x00, 0x00, 0x40, 0x02, 0x94, 0xd9, 0x02, 0x02,
    0x02, 0x02, 0xe1, 0x01, 0x01, 0x01, 0x16, 0x00,
    0x07, 0xfd, 0xe1, 0x01, 0x01, 0x01,
};

/* host 00:0a:0b:0c:0d:0e 2.2.2.2->225.1.1.1, type 0x22 */
uint8_t igmp_v3_report[] = {
    0x01, 0x00, 0x5e, 0x01, 0x01, 0x01, 0x00, 0x0a,
    0x0b, 0x0c, 0x0d, 0x0e, 0x81, 0x00, 0x00, 0x0a,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x1c, 0x00, 0x01,
    0x00, 0x00, 0x40, 0x02, 0x94, 0xd9, 0x02, 0x02,
    0x02, 0x02, 0xe1, 0x01, 0x01, 0x01, 0x22, 0x00,
    0xfb, 0xfc, 0xe1, 0x01, 0x01, 0x01,
};

/* host 00:0a:0b:0c:0d:0e 2.2.2.2->224.0.0.2, type 0x17 */
uint8_t igmp_v2_leave[] = {
    0x01, 0x00, 0x5e, 0x00, 0x00, 0x02, 0x00, 0x0a,
    0x0b, 0x0c, 0x0d, 0x0e, 0x81, 0x00, 0x00, 0x0a,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x1c, 0x00, 0x01,
    0x00, 0x00, 0x40, 0x02, 0x96, 0xd9, 0x02, 0x02,
    0x02, 0x02, 0xe0, 0x00, 0x00, 0x02, 0x17, 0x00,
    0x06, 0xfd, 0xe1, 0x01, 0x01, 0x01,
};


struct indigo_core_gentable {
    uint16_t table_id;
    const indigo_core_gentable_ops_t *ops;
};

#define MAX_GENTABLES 16
struct indigo_core_gentable gt[MAX_GENTABLES];

#define TIMEOUT_IDX 0
#define RPG_IDX 1
#define TPG_IDX 2
#define RPT_EXP_IDX 3
#define RPT_TX_IDX 4
#define GQ_EXP_IDX 5
#define GQ_TX_IDX 6

void *global_tpg_entry;


void
indigo_core_gentable_register(const of_table_name_t name,
                              const indigo_core_gentable_ops_t *_ops,
                              void *_table_priv,
                              uint32_t max_size,
                              uint32_t buckets_size,
                              indigo_core_gentable_t **gentable)
{
    int idx = -1;

    /* hardcode index with name */
    if (!strcmp(name, "igmp_timeout")) {
        idx = TIMEOUT_IDX;
    } else if (!strcmp(name, "igmp_rx_port_group")) {
        idx = RPG_IDX;
    } else if (!strcmp(name, "igmp_tx_port_group")) {
        idx = TPG_IDX;
    } else if (!strcmp(name, "igmp_report_expectation")) {
        idx = RPT_EXP_IDX;
    } else if (!strcmp(name, "igmp_report_tx")) {
        idx = RPT_TX_IDX;
    } else if (!strcmp(name, "igmp_general_query_expectation")) {
        idx = GQ_EXP_IDX;
    } else if (!strcmp(name, "igmp_general_query_tx")) {
        idx = GQ_TX_IDX;
    }

    if (idx != -1) {
        gt[idx].ops = _ops;
        gt[idx].table_id = idx;
        *gentable = (indigo_core_gentable_t*) &gt[idx];
    }
}

void
indigo_core_gentable_unregister(indigo_core_gentable_t *gentable)
{
}

uint16_t
indigo_core_gentable_id(indigo_core_gentable_t *gentable)
{
    return gentable->table_id;
}


void *
indigo_core_gentable_lookup(indigo_core_gentable_t *gentable, of_object_t *key)
{
    if (gentable->table_id == TPG_IDX) {
        return global_tpg_entry;
    } else {
        return NULL;
    }
}


indigo_core_packet_in_listener_f pktin_handler = NULL;

indigo_error_t
indigo_core_packet_in_listener_register(indigo_core_packet_in_listener_f fn)
{
    pktin_handler = fn;
    return INDIGO_ERROR_NONE;
}

void
indigo_core_packet_in_listener_unregister(indigo_core_packet_in_listener_f fn)
{
}


static of_object_t *
make_interval_tlv(uint32_t val)
{
    of_bsn_tlv_interval_t *tlv = of_bsn_tlv_interval_new(OF_VERSION_1_4);
    of_bsn_tlv_interval_value_set(tlv, val);
    return tlv;
}

static of_object_t *
make_name_tlv(char *name, int len)
{
    of_bsn_tlv_name_t *tlv = of_bsn_tlv_name_new(OF_VERSION_1_4);
    of_octets_t octets;

    octets.data = (uint8_t*) name;
    octets.bytes = len;

    AIM_ASSERT(of_bsn_tlv_name_value_set(tlv, &octets) == 0,
               "cannot set rpg name to %s", name);

    return tlv;
}

static of_list_bsn_tlv_t *
make_name_tlv_list(char *name, int len)
{
    of_list_bsn_tlv_t *list = of_list_bsn_tlv_new(OF_VERSION_1_4);
    of_bsn_tlv_name_t *tlv = make_name_tlv(name, len);

    of_list_append(list, tlv);
    of_object_delete(tlv);
    return list;
}

static of_list_bsn_tlv_t *
make_port_tlv_list(of_port_no_t port_no)
{
    of_list_bsn_tlv_t *list = of_list_bsn_tlv_new(OF_VERSION_1_4);
    of_bsn_tlv_port_t *tlv = of_bsn_tlv_port_new(OF_VERSION_1_4);
    of_bsn_tlv_port_value_set(tlv, port_no);
    of_list_append(list, tlv);
    of_object_delete(tlv);
    return list;
}


static of_list_bsn_tlv_t *
rpt_exp_make_key(char *name, int len, uint16_t vlan_vid, uint32_t ipv4)
{
    of_list_bsn_tlv_t *list = of_list_bsn_tlv_new(OF_VERSION_1_4);
    {
        of_bsn_tlv_name_t *tlv = of_bsn_tlv_name_new(OF_VERSION_1_4);
        of_octets_t octets;
        octets.data = (uint8_t*) name;
        octets.bytes = len;
        AIM_ASSERT(of_bsn_tlv_name_value_set(tlv, &octets) == 0,
                   "cannot set rpg name to %s", name);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    {
        of_bsn_tlv_vlan_vid_t *tlv = of_bsn_tlv_vlan_vid_new(OF_VERSION_1_4);
        of_bsn_tlv_vlan_vid_value_set(tlv, vlan_vid);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    {
        of_bsn_tlv_ipv4_t *tlv = of_bsn_tlv_ipv4_new(OF_VERSION_1_4);
        of_bsn_tlv_ipv4_value_set(tlv, ipv4);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    return list;
}

static of_list_bsn_tlv_t *
exp_make_value(void)
{
    of_list_bsn_tlv_t *list = of_list_bsn_tlv_new(OF_VERSION_1_4);
    return list;
}


static of_list_bsn_tlv_t *
rpt_tx_make_key(char *name, int len, uint16_t vlan_vid, uint32_t ipv4)
{
    of_list_bsn_tlv_t *list = of_list_bsn_tlv_new(OF_VERSION_1_4);
    {
        of_list_bsn_tlv_t *refkeylist = of_list_bsn_tlv_new(OF_VERSION_1_4);
        of_bsn_tlv_name_t *nametlv = of_bsn_tlv_name_new(OF_VERSION_1_4);
        of_octets_t octets;
        octets.data = (uint8_t*) name;
        octets.bytes = len;
        AIM_ASSERT(of_bsn_tlv_name_value_set(nametlv, &octets) == 0,
                   "cannot set report tx ref name to %s", name);
        of_list_append(refkeylist, nametlv);
        of_object_delete(nametlv);

        of_bsn_tlv_reference_t *tlv = of_bsn_tlv_reference_new(OF_VERSION_1_4);
        of_bsn_tlv_reference_table_id_set(tlv, TPG_IDX);
        AIM_ASSERT(of_bsn_tlv_reference_key_set(tlv, refkeylist) == 0,
                   "cannot set report tx ref key list");
        of_object_delete(refkeylist);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    {
        of_bsn_tlv_vlan_vid_t *tlv = of_bsn_tlv_vlan_vid_new(OF_VERSION_1_4);
        of_bsn_tlv_vlan_vid_value_set(tlv, vlan_vid);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    {
        of_bsn_tlv_ipv4_t *tlv = of_bsn_tlv_ipv4_new(OF_VERSION_1_4);
        of_bsn_tlv_ipv4_value_set(tlv, ipv4);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    return list;
}

static of_list_bsn_tlv_t *
tx_make_value(uint16_t vlan_vid,
              uint32_t ipv4_src,
              of_mac_addr_t eth_src)
{
    of_list_bsn_tlv_t *list = of_list_bsn_tlv_new(OF_VERSION_1_4);
    {
        of_bsn_tlv_vlan_vid_t *tlv = of_bsn_tlv_vlan_vid_new(OF_VERSION_1_4);
        of_bsn_tlv_vlan_vid_value_set(tlv, vlan_vid);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    {
        of_bsn_tlv_ipv4_t *tlv = of_bsn_tlv_ipv4_src_new(OF_VERSION_1_4);
        of_bsn_tlv_ipv4_src_value_set(tlv, ipv4_src);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    {
        of_bsn_tlv_eth_src_t *tlv = of_bsn_tlv_eth_src_new(OF_VERSION_1_4);
        of_bsn_tlv_eth_src_value_set(tlv, eth_src);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    return list;
}


static of_list_bsn_tlv_t *
gq_exp_make_key(char *name, int len, uint16_t vlan_vid)
{
    of_list_bsn_tlv_t *list = of_list_bsn_tlv_new(OF_VERSION_1_4);
    {
        of_bsn_tlv_name_t *tlv = of_bsn_tlv_name_new(OF_VERSION_1_4);
        of_octets_t octets;
        octets.data = (uint8_t*) name;
        octets.bytes = len;
        AIM_ASSERT(of_bsn_tlv_name_value_set(tlv, &octets) == 0,
                   "cannot set rpg name to %s", name);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    {
        of_bsn_tlv_vlan_vid_t *tlv = of_bsn_tlv_vlan_vid_new(OF_VERSION_1_4);
        of_bsn_tlv_vlan_vid_value_set(tlv, vlan_vid);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    return list;
}


static of_list_bsn_tlv_t *
gq_tx_make_key(char *name, int len, uint16_t vlan_vid)
{
    of_list_bsn_tlv_t *list = of_list_bsn_tlv_new(OF_VERSION_1_4);
    {
        of_list_bsn_tlv_t *refkeylist = of_list_bsn_tlv_new(OF_VERSION_1_4);
        of_bsn_tlv_name_t *nametlv = of_bsn_tlv_name_new(OF_VERSION_1_4);
        of_octets_t octets;
        octets.data = (uint8_t*) name;
        octets.bytes = len;
        AIM_ASSERT(of_bsn_tlv_name_value_set(nametlv, &octets) == 0,
                   "cannot set report tx ref name to %s", name);
        of_list_append(refkeylist, nametlv);
        of_object_delete(nametlv);

        of_bsn_tlv_reference_t *tlv = of_bsn_tlv_reference_new(OF_VERSION_1_4);
        of_bsn_tlv_reference_table_id_set(tlv, TPG_IDX);
        AIM_ASSERT(of_bsn_tlv_reference_key_set(tlv, refkeylist) == 0,
                   "cannot set report tx ref key list");
        of_object_delete(refkeylist);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    {
        of_bsn_tlv_vlan_vid_t *tlv = of_bsn_tlv_vlan_vid_new(OF_VERSION_1_4);
        of_bsn_tlv_vlan_vid_value_set(tlv, vlan_vid);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    return list;
}


int
send_packet_in(of_port_no_t port, uint16_t len, uint8_t *data)
{
    of_packet_in_t *pktin;
    of_match_t match;
    of_octets_t octets;
    int rc;

    memset(&match, 0, sizeof(match));
    match.version = OF_VERSION_1_4;
    match.fields.in_port = port;
    OF_MATCH_MASK_IN_PORT_EXACT_SET(&match);
    /* FIXME set reason field? */

    pktin = of_packet_in_new(OF_VERSION_1_4);
    AIM_ASSERT(pktin != NULL, "unable to create pktin");
    of_packet_in_total_len_set(pktin, len);
    AIM_ASSERT(of_packet_in_match_set(pktin, &match) == OF_ERROR_NONE,
               "cannot set pktin match");
    octets.data = data;
    octets.bytes = len;
    AIM_ASSERT(of_packet_in_data_set(pktin, &octets) == OF_ERROR_NONE,
               "cannot set pktin data");

    rc = pktin_handler(pktin);
    printf("pktin_handler returns rc %s %d\n",
           rc == INDIGO_CORE_LISTENER_RESULT_DROP? "DROP": "PASS",
           rc);

    of_packet_in_delete(pktin);

    return rc;
}


bool query_sent = false;
bool report_sent = false;

indigo_error_t
indigo_fwd_packet_out(of_packet_out_t *of_packet_out)
{
    of_octets_t octets;
    ppe_packet_t ppep;
    uint32_t val;

    printf("unit test: sending packet out\n");
    
    of_packet_out_data_get(of_packet_out, &octets);
    ppe_packet_init(&ppep, octets.data, octets.bytes);
    if (ppe_parse(&ppep) < 0) {
        AIM_DIE("ppe_parse failed");
    }
    if (!ppe_header_get(&ppep, PPE_HEADER_IGMP)) {
        AIM_DIE("not an igmp packet");
    }

    ppe_field_get(&ppep, PPE_FIELD_IGMP_TYPE, &val);
    switch (val) {
    case PPE_IGMP_TYPE_QUERY:
        query_sent = true;
        break;
    case PPE_IGMP_TYPE_V2_REPORT:
        report_sent = true;
        break;
    }

    return INDIGO_ERROR_NONE;
}


indigo_error_t
indigo_cxn_get_async_version(of_version_t* of_version)
{
    *of_version = OF_VERSION_1_4;
    return INDIGO_ERROR_NONE;
}


bool query_idle_sent = false;
bool report_idle_sent = false;

void
indigo_cxn_send_async_message(of_object_t *obj)
{
    of_str64_t name;

    printf("unit test: sending async msg to controller, %s\n",
           of_class_name(obj));

    if (obj->object_id == OF_BSN_GENERIC_ASYNC) {
        of_bsn_generic_async_name_get(obj, &name);
        printf("got name %s\n", name);
        if (!strcmp(name, "igmp_query_idle")) {
            query_idle_sent = true;
        } else if (!strcmp(name, "igmp_report_idle")) {
            report_idle_sent = true;
        }
    }

    of_object_delete(obj);
}


#define MAKE_NAME(s) make_name_tlv(s, strlen(s))

int aim_main(int argc, char* argv[])
{
    ind_soc_config_t soc_cfg;
    int rc;

    of_bsn_tlv_t *gqe_key;
    of_bsn_tlv_t *re_key;
    of_bsn_tlv_t *gqtx_key;
    of_bsn_tlv_t *rtx_key;
    of_bsn_tlv_t *gqe_value;
    of_bsn_tlv_t *re_value;
    of_bsn_tlv_t *gqtx_value;
    of_bsn_tlv_t *rtx_value;
    void *gqe_entry;
    void *re_entry;
    void *gqtx_entry;
    void *rtx_entry;

    of_list_bsn_tlv_t *rpg_key, *rpg_value;
    void *rpg_entry;
    of_list_bsn_tlv_t *tpg_key, *tpg_value;
    void *tpg_entry;

    of_list_bsn_tlv_t *rpt_exp_key, *rpt_exp_value;
    void *rpt_exp_entry;
    of_list_bsn_tlv_t *rpt_tx_key, *rpt_tx_value;
    void *rpt_tx_entry;

    of_list_bsn_tlv_t *gq_exp_key, *gq_exp_value;
    void *gq_exp_entry;
    of_list_bsn_tlv_t *gq_tx_key, *gq_tx_value;
    void *gq_tx_entry;

    ind_soc_init(&soc_cfg);

    /* init agent */
    AIM_ASSERT(igmpa_init() == INDIGO_ERROR_NONE);

    /* configure timeouts */
    gqe_key = MAKE_NAME("general_query_expectation");
    re_key = MAKE_NAME("report_expectation");
    gqtx_key = MAKE_NAME("general_query_tx");
    rtx_key = MAKE_NAME("report_tx");

    /* configure timeouts so this unit test does not take forever */
    gqe_value = make_interval_tlv(2000);
    re_value = make_interval_tlv(2000);
    gqtx_value = make_interval_tlv(1000);
    rtx_value = make_interval_tlv(1000);

    /* configure rx_port_group */
    printf("*** configure rx port group\n");
    {
        char name[] = "rx port group 7";
        rpg_key = make_port_tlv_list(7);
        rpg_value = make_name_tlv_list(name, strlen(name));
        rc = gt[RPG_IDX].ops->add((void*)RPG_IDX, rpg_key, rpg_value, &rpg_entry);
        AIM_ASSERT(rc == INDIGO_ERROR_NONE);
    }

    /* configure tx_port_group */
    printf("*** configure tx port group\n");
    {
        char name[] = "tx port group 8";
        tpg_key = make_name_tlv_list(name, strlen(name));
        tpg_value = make_port_tlv_list(8);
        rc = gt[TPG_IDX].ops->add((void*)TPG_IDX, tpg_key, tpg_value, &tpg_entry);
        AIM_ASSERT(rc == INDIGO_ERROR_NONE);
        global_tpg_entry = tpg_entry;
    }


    /* bad checksum, should forward to controller */
    printf("*** bad checksum\n");
    rc = send_packet_in(10, sizeof(igmp_bad_checksum), igmp_bad_checksum);
    AIM_ASSERT(rc == INDIGO_CORE_LISTENER_RESULT_PASS);


    /* no expectation set, should forward to controller */
    printf("*** unexpected general query\n");
    rc = send_packet_in(10, sizeof(igmp_general_query), igmp_general_query);
    AIM_ASSERT(rc == INDIGO_CORE_LISTENER_RESULT_PASS);

    /* configure general query expectation and timeout */
    printf("*** configure general query expect\n");
    {
        char name[] = "rx port group 7";

        rc = gt[TIMEOUT_IDX].ops->add((void*)TIMEOUT_IDX, gqe_key, gqe_value,
                                      &gqe_entry);
        AIM_ASSERT(rc == INDIGO_ERROR_NONE);

        gq_exp_key = gq_exp_make_key(name, strlen(name), 10);
        gq_exp_value = exp_make_value();
        rc = gt[GQ_EXP_IDX].ops->add((void*)GQ_EXP_IDX,
                                     gq_exp_key, gq_exp_value, 
                                     &gq_exp_entry);
        AIM_ASSERT(rc == INDIGO_ERROR_NONE);
    }

    /* send expected general query; should be dropped */
    printf("*** expected general query\n");
    rc = send_packet_in(7, sizeof(igmp_general_query), igmp_general_query);
    AIM_ASSERT(rc == INDIGO_CORE_LISTENER_RESULT_DROP);

    /* wait for timeout and notification to be sent */
    ind_soc_select_and_run(3500);
    AIM_ASSERT(query_idle_sent);
    query_idle_sent = false;

    /* reset general query expectation timeout */
    rc = gt[TIMEOUT_IDX].ops->del((void*)TIMEOUT_IDX, gqe_entry, gqe_key);
    AIM_ASSERT(rc == INDIGO_ERROR_NONE);


    /* configure general query tx */
    printf("*** configure general query tx\n");
    {
        char name[] = "tx port group 8";
        of_mac_addr_t eth = { .addr = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } };

        rc = gt[TIMEOUT_IDX].ops->add((void*)TIMEOUT_IDX, gqtx_key, gqtx_value,
                                      &gqtx_entry);
        AIM_ASSERT(rc == INDIGO_ERROR_NONE);

        gq_tx_key = gq_tx_make_key(name, strlen(name), 20);
        gq_tx_value = tx_make_value(15, 0x05050505, eth);
        rc = gt[GQ_TX_IDX].ops->add((void*)GQ_TX_IDX, 
                                    gq_tx_key, gq_tx_value, &gq_tx_entry);
        AIM_ASSERT(rc == INDIGO_ERROR_NONE);
    }

    /* general query should be sent immediately after configuration */
    printf("*** expect immediate general query tx\n");
    ind_soc_select_and_run(1);
    AIM_ASSERT(query_sent);
    query_sent = false;

    /* after some time, check general query has been sent */
    printf("*** next general query tx\n");
    ind_soc_select_and_run(1500);
    AIM_ASSERT(query_sent);
    query_sent = false;

    /* reset general query tx timeout */
    rc = gt[TIMEOUT_IDX].ops->del((void*)TIMEOUT_IDX, gqtx_entry, gqtx_key);
    AIM_ASSERT(rc == INDIGO_ERROR_NONE);


    /* send unexpected v2 report from host; should forward */
    printf("*** unexpected v2 report\n");
    rc = send_packet_in(10, sizeof(igmp_v2_report), igmp_v2_report);
    AIM_ASSERT(rc == INDIGO_CORE_LISTENER_RESULT_PASS);

    /* configure report expectation and timeout */
    printf("*** configure report expect\n");
    {
        char name[] = "rx port group 7";

        rc = gt[TIMEOUT_IDX].ops->add((void*)TIMEOUT_IDX, re_key, re_value,
                                      &re_entry);
        AIM_ASSERT(rc == INDIGO_ERROR_NONE);

        rpt_exp_key = rpt_exp_make_key(name, strlen(name),
                                       10, 0xe1010101 /* 225.1.1.1*/);
        rpt_exp_value = exp_make_value();
        rc = gt[RPT_EXP_IDX].ops->add((void*)RPT_EXP_IDX,
                                      rpt_exp_key, rpt_exp_value, 
                                      &rpt_exp_entry);
        AIM_ASSERT(rc == INDIGO_ERROR_NONE);
    }

    /* send expected v2 report; should drop */
    printf("*** expected v2 report\n");
    rc = send_packet_in(7, sizeof(igmp_v2_report), igmp_v2_report);
    AIM_ASSERT(rc == INDIGO_CORE_LISTENER_RESULT_DROP);

    /* wait for timeout and notification to be sent */
    ind_soc_select_and_run(3500);
    AIM_ASSERT(report_idle_sent);
    report_idle_sent = false;

    /* reset report expectation timeout */
    rc = gt[TIMEOUT_IDX].ops->del((void*)TIMEOUT_IDX, re_entry, re_key);
    AIM_ASSERT(rc == INDIGO_ERROR_NONE);


    /* configure report tx and timeout */
    printf("*** configure report tx\n");
    {
        char name[] = "tx port group 8";
        of_mac_addr_t eth = { .addr = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } };

        rc = gt[TIMEOUT_IDX].ops->add((void*)TIMEOUT_IDX, rtx_key, rtx_value,
                                      &rtx_entry);
        AIM_ASSERT(rc == INDIGO_ERROR_NONE);

        rpt_tx_key = rpt_tx_make_key(name, strlen(name), 
                                     20, 0xe1010101 /* 225.1.1.1 */);
        rpt_tx_value = tx_make_value(10, 0x04040404, eth);
        rc = gt[RPT_TX_IDX].ops->add((void*)RPT_TX_IDX, 
                                     rpt_tx_key, rpt_tx_value, &rpt_tx_entry);
        AIM_ASSERT(rc == INDIGO_ERROR_NONE);
    }

    /* report should be send immediately after configuration */
    printf("*** expect immediate report tx\n");
    ind_soc_select_and_run(1);
    AIM_ASSERT(report_sent);
    report_sent = false;

    /* after some time, check report has been sent */
    printf("*** next report tx\n");
    ind_soc_select_and_run(1500);
    AIM_ASSERT(report_sent);
    report_sent = false;

    /* reset the report tx timeout */
    rc = gt[TIMEOUT_IDX].ops->del((void*)TIMEOUT_IDX, rtx_entry, rtx_key);
    AIM_ASSERT(rc == INDIGO_ERROR_NONE);


    /* send v2 report from host on unknown port group; should forward */
    printf("*** send v2 report from host on unknown port group\n");
    rc = send_packet_in(10, sizeof(igmp_v2_report), igmp_v2_report);
    AIM_ASSERT(rc == INDIGO_CORE_LISTENER_RESULT_PASS);

    /* send v2 leave from host; should forward to controller */
    printf("*** send v2 leave from host\n");
    rc = send_packet_in(10, sizeof(igmp_v2_leave), igmp_v2_leave);
    AIM_ASSERT(rc == INDIGO_CORE_LISTENER_RESULT_PASS);

    /* send v3 report from host; should be dropped */
    printf("*** send v3 leave from host\n");
    rc = send_packet_in(10, sizeof(igmp_v3_report), igmp_v3_report);
    AIM_ASSERT(rc == INDIGO_CORE_LISTENER_RESULT_DROP);

    /* send group-specific query from mrouter; should be dropped */
    printf("*** send group-specific query from mrouter\n");
    rc = send_packet_in(10, sizeof(igmp_group_specific_query),
                        igmp_group_specific_query);
    AIM_ASSERT(rc == INDIGO_CORE_LISTENER_RESULT_DROP);


    /* remove configuration */
    gt[RPT_TX_IDX].ops->del((void*)RPT_TX_IDX, rpt_tx_entry, rpt_tx_key);
    gt[RPT_EXP_IDX].ops->del((void*)RPT_EXP_IDX, rpt_exp_entry, rpt_exp_key);
    gt[GQ_TX_IDX].ops->del((void*)GQ_TX_IDX, gq_tx_entry, gq_tx_key);
    gt[GQ_EXP_IDX].ops->del((void*)GQ_EXP_IDX, gq_exp_entry, gq_exp_key);
    gt[TPG_IDX].ops->del((void*)TPG_IDX, tpg_entry, tpg_key);
    gt[RPG_IDX].ops->del((void*)RPG_IDX, rpg_entry, rpg_key);

    printf("*** after final cleanup\n");
    igmpa_stats_show(&aim_pvs_stdout);

    /* finish */
    igmpa_finish();

    of_object_delete(rpt_tx_key);
    of_object_delete(rpt_tx_value);
    of_object_delete(rpt_exp_key);
    of_object_delete(rpt_exp_value);
    of_object_delete(gq_tx_key);
    of_object_delete(gq_tx_value);
    of_object_delete(gq_exp_key);
    of_object_delete(gq_exp_value);
    of_object_delete(tpg_key);
    of_object_delete(tpg_value);
    of_object_delete(rpg_key);
    of_object_delete(rpg_value);

    of_object_delete(gqe_key);
    of_object_delete(gqe_value);
    of_object_delete(re_key);
    of_object_delete(re_value);
    of_object_delete(gqtx_key);
    of_object_delete(gqtx_value);
    of_object_delete(rtx_key);
    of_object_delete(rtx_value);

    ind_soc_finish();

    return 0;
}

