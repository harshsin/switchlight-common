/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <router_ip_table/router_ip_table_config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <AIM/aim.h>

#include <router_ip_table/router_ip_table.h>
#include <indigo/of_state_manager.h>

static const indigo_core_gentable_ops_t *ops;
static void *table_priv;

static const of_mac_addr_t mac1 = { { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 } };
static const of_mac_addr_t mac2 = { { 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff } };

static of_list_bsn_tlv_t *
make_key(uint16_t vlan_vid)
{
    of_list_bsn_tlv_t *list = of_list_bsn_tlv_new(OF_VERSION_1_3);
    of_bsn_tlv_vlan_vid_t *tlv = of_bsn_tlv_vlan_vid_new(OF_VERSION_1_3);
    of_bsn_tlv_vlan_vid_value_set(tlv, vlan_vid);
    of_list_append(list, tlv);
    of_object_delete(tlv);
    return list;
}

static of_list_bsn_tlv_t *
make_value(uint32_t ipv4, of_mac_addr_t mac)
{
    of_list_bsn_tlv_t *list = of_list_bsn_tlv_new(OF_VERSION_1_3);
    {
        of_bsn_tlv_ipv4_t *tlv = of_bsn_tlv_ipv4_new(OF_VERSION_1_3);
        of_bsn_tlv_ipv4_value_set(tlv, ipv4);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    {
        of_bsn_tlv_mac_t *tlv = of_bsn_tlv_mac_new(OF_VERSION_1_3);
        of_bsn_tlv_mac_value_set(tlv, mac);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    return list;
}

static of_list_bsn_tlv_t *
make_key2(uint16_t vlan_vid, uint32_t ipv4)
{
    of_list_bsn_tlv_t *list = of_list_bsn_tlv_new(OF_VERSION_1_3);
    {
        of_bsn_tlv_vlan_vid_t *tlv = of_bsn_tlv_vlan_vid_new(OF_VERSION_1_3);
        of_bsn_tlv_vlan_vid_value_set(tlv, vlan_vid);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    {
        of_bsn_tlv_ipv4_t *tlv = of_bsn_tlv_ipv4_new(OF_VERSION_1_3);
        of_bsn_tlv_ipv4_value_set(tlv, ipv4);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    return list;
}

static of_list_bsn_tlv_t *
make_value2(of_mac_addr_t mac, uint32_t ipv4_netmask)
{
    of_list_bsn_tlv_t *list = of_list_bsn_tlv_new(OF_VERSION_1_3);
    {
        of_bsn_tlv_mac_t *tlv = of_bsn_tlv_mac_new(OF_VERSION_1_3);
        of_bsn_tlv_mac_value_set(tlv, mac);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    {
        of_bsn_tlv_ipv4_netmask_t *tlv = of_bsn_tlv_ipv4_netmask_new(OF_VERSION_1_3);
        of_bsn_tlv_ipv4_netmask_value_set(tlv, ipv4_netmask);
        of_list_append(list, tlv);
        of_object_delete(tlv);
    }
    return list;
}

int aim_main(int argc, char* argv[])
{
    of_list_bsn_tlv_t *key1, *key2, *key3, *key4;
    of_list_bsn_tlv_t *value1, *value2, *value3, *value4, *value5;
    void *entry_priv, *entry_priv2;
    indigo_error_t rv;
    uint32_t ip;
    of_mac_addr_t mac;

    router_ip_table_init();

    AIM_ASSERT(ops != NULL);

    key1 = make_key(10);
    key2 = make_key(8000); /* invalid */
    key3 = make_key2(10, 0x1234); /* v2 */
    key4 = make_key2(10, 0x5678); /* v2 */
    value1 = make_value(0x1234, mac1);
    value2 = make_value(0x5678, mac2);
    value3 = make_value(0, mac1); /* invalid */
    value4 = make_value2(mac1, 0xfff0); /* v2 */
    value5 = make_value2(mac2, 0xfff0); /* v2 */

    /* Successful add/modify/delete */
    {
        rv = router_ip_table_lookup(10, &ip, &mac);
        AIM_ASSERT(rv == INDIGO_ERROR_NOT_FOUND);

        rv = ops->add(table_priv, key1, value1, &entry_priv);
        AIM_ASSERT(rv == INDIGO_ERROR_NONE);

        rv = router_ip_table_lookup(10, &ip, &mac);
        AIM_ASSERT(rv == INDIGO_ERROR_NONE);
        AIM_ASSERT(ip == 0x1234);
        AIM_ASSERT(!memcmp(&mac, &mac1, sizeof(of_mac_addr_t)));

        AIM_ASSERT(router_ip_check(ip) == true);
        AIM_ASSERT(router_ip_check(0x5678) == false);

        rv = ops->modify(table_priv, entry_priv, key1, value2);
        AIM_ASSERT(rv == INDIGO_ERROR_NONE);

        rv = router_ip_table_lookup(10, &ip, &mac);
        AIM_ASSERT(rv == INDIGO_ERROR_NONE);
        AIM_ASSERT(ip == 0x5678);
        AIM_ASSERT(!memcmp(&mac, &mac2, sizeof(of_mac_addr_t)));

        AIM_ASSERT(router_ip_check(ip) == true);
        AIM_ASSERT(router_ip_check(0x1234) == false);

        rv = ops->del(table_priv, entry_priv, key1);
        AIM_ASSERT(rv == INDIGO_ERROR_NONE);

        rv = router_ip_table_lookup(10, &ip, &mac);
        AIM_ASSERT(rv == INDIGO_ERROR_NOT_FOUND);

        AIM_ASSERT(router_ip_check(0x5678) == false);
    }

    /* Invalid key */
    {
        rv = ops->add(table_priv, key2, value1, &entry_priv);
        AIM_ASSERT(rv == INDIGO_ERROR_PARAM);
    }

    /* Invalid value */
    {
        rv = ops->add(table_priv, key1, value3, &entry_priv);
        AIM_ASSERT(rv == INDIGO_ERROR_PARAM);
    }

    /* Two entries on the same VLAN */
    {
        rv = router_ip_table_lookup2(10, 0x1236, &ip, &mac);
        AIM_ASSERT(rv == INDIGO_ERROR_NOT_FOUND);

        rv = router_ip_table_lookup2(10, 0x5679, &ip, &mac);
        AIM_ASSERT(rv == INDIGO_ERROR_NOT_FOUND);

        rv = ops->add(table_priv, key3, value4, &entry_priv);
        AIM_ASSERT(rv == INDIGO_ERROR_NONE);

        rv = router_ip_table_lookup2(10, 0x1235, &ip, &mac);
        AIM_ASSERT(rv == INDIGO_ERROR_NONE);
        AIM_ASSERT(ip == 0x1234);
        AIM_ASSERT(!memcmp(&mac, &mac1, sizeof(of_mac_addr_t)));

        rv = router_ip_table_lookup2(10, 0x5679, &ip, &mac);
        AIM_ASSERT(rv == INDIGO_ERROR_NOT_FOUND);

        rv = ops->add(table_priv, key4, value5, &entry_priv2);
        AIM_ASSERT(rv == INDIGO_ERROR_NONE);

        rv = router_ip_table_lookup2(10, 0x1235, &ip, &mac);
        AIM_ASSERT(rv == INDIGO_ERROR_NONE);
        AIM_ASSERT(ip == 0x1234);
        AIM_ASSERT(!memcmp(&mac, &mac1, sizeof(of_mac_addr_t)));

        rv = router_ip_table_lookup2(10, 0x5679, &ip, &mac);
        AIM_ASSERT(rv == INDIGO_ERROR_NONE);
        AIM_ASSERT(ip == 0x5678);
        AIM_ASSERT(!memcmp(&mac, &mac2, sizeof(of_mac_addr_t)));

        rv = ops->del(table_priv, entry_priv, key3);
        AIM_ASSERT(rv == INDIGO_ERROR_NONE);

        rv = ops->del(table_priv, entry_priv2, key4);
        AIM_ASSERT(rv == INDIGO_ERROR_NONE);

        rv = router_ip_table_lookup2(10, 0x1235, &ip, &mac);
        AIM_ASSERT(rv == INDIGO_ERROR_NOT_FOUND);

        rv = router_ip_table_lookup2(10, 0x5679, &ip, &mac);
        AIM_ASSERT(rv == INDIGO_ERROR_NOT_FOUND);
    }

    of_object_delete(key1);
    of_object_delete(key2);
    of_object_delete(key3);
    of_object_delete(key4);
    of_object_delete(value1);
    of_object_delete(value2);
    of_object_delete(value3);
    of_object_delete(value4);
    of_object_delete(value5);

    router_ip_table_finish();

    return 0;
}

void
indigo_core_gentable_register(
    const of_table_name_t name,
    const indigo_core_gentable_ops_t *_ops,
    void *_table_priv,
    uint32_t max_size,
    uint32_t buckets_size,
    indigo_core_gentable_t **gentable)
{
    if (!strcmp(name, "router_ip")) {
        ops = _ops;
        table_priv = _table_priv;
    }

    *gentable = (void *)1;
}

void
indigo_core_gentable_unregister(indigo_core_gentable_t *gentable)
{
    AIM_ASSERT(gentable == (void *)1);
}
