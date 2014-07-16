# Copyright (c) 2013  BigSwitch Networks

LAG_MIN_NUM             = 1
LAG_MAX_NUM             = 30
LAG_BASE_OF_PORT_NUM    = 60

OF_MAX_TABLE    = 1

# FIXME: For now the max OF port number corresponds to the OF port number
#        of the highest-numbered (last) LAG port. OF port numbers less than
#        LAG_BASE_OF_PORT_NUM are reserved for individual physical ports.
OF_MAX_PORT     = LAG_BASE_OF_PORT_NUM + LAG_MAX_NUM

MGMT_PORT_BASE  = "ma"
MGMT_PORTS      = ["ma1"]

PLATFORM_PATH   = "/etc/sl_platform"
VERSION_PATH    = "/etc/sl_version"

TZ_CFG_PATH     = "/etc/timezone"
DNS_CFG_PATH    = "/etc/resolv.conf"
NTP_CFG_PATH    = "/etc/ntp.conf"
RLOG_CFG_PATH   = "/etc/rsyslog.d/10-switchlight-remote.conf"
SNMP_CFG_PATH   = "/etc/snmp/snmpd.conf"
OFAD_CFG_PATH   = "/etc/ofad.conf"

BRCM_JSON       = "/mnt/flash/brcm.json"

DEFAULT_DIR     = "/var/run/default"
