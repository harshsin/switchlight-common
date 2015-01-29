/**************************************************************************//**
 *
 * arpa Internal Header
 *
 *****************************************************************************/
#ifndef __ARPA_INT_H__
#define __ARPA_INT_H__

#include <arpa/arpa_config.h>

void arpa_reply_table_init(void);
void arpa_reply_table_finish(void);
indigo_error_t arpa_reply_table_lookup(uint16_t vlan_vid, uint32_t ipv4, of_mac_addr_t *mac);

void arpa_vlan_reply_table_init(void);
void arpa_vlan_reply_table_finish(void);
indigo_error_t arpa_vlan_reply_table_lookup(uint16_t vlan_vid, of_mac_addr_t *mac);

#endif /* __ARPA_INT_H__ */
