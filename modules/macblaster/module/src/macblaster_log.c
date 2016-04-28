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

#include <macblaster/macblaster_config.h>

#include "macblaster_log.h"
/*
 * macblaster log struct.
 */
AIM_LOG_STRUCT_DEFINE(
                      MACBLASTER_CONFIG_LOG_OPTIONS_DEFAULT,
                      MACBLASTER_CONFIG_LOG_BITS_DEFAULT,
                      NULL, /* Custom log map */
                      MACBLASTER_CONFIG_LOG_CUSTOM_BITS_DEFAULT
                     );
