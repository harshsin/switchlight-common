/****************************************************************
 *
 *        Copyright 2016, Big Switch Networks, Inc.
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

#include <indigo/of_state_manager.h>
#include "vxlan_int.h"
#include "vxlan_log.h"
#include "vxlan_gentable_protocol_identifier.h"
#include "vxlan_gentable_vni_vlan_mapping.h"

static bool vxlan_initialized = false;
indigo_error_t
vxlan_init(void)
{
    indigo_error_t rv;

    if (vxlan_initialized) return INDIGO_ERROR_NONE;

    rv = vxlan_gentable_protocol_identifier_init();
    if (rv != INDIGO_ERROR_NONE) {
        return rv;
    }

    rv = vxlan_gentable_vni_vlan_mapping_init();
    if (rv != INDIGO_ERROR_NONE) {
        return rv;
    }

    vxlan_initialized = true;

    return INDIGO_ERROR_NONE;
}

indigo_error_t
vxlan_finish(void)
{
    if (!vxlan_initialized) return INDIGO_ERROR_NONE;

    (void) vxlan_gentable_protocol_identifier_deinit();

    (void) vxlan_gentable_vni_vlan_mapping_deinit();

    vxlan_initialized = false;

    return INDIGO_ERROR_NONE;
}
